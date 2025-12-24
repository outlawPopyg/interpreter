// gc.c
// Простой mark-and-sweep сборщик мусора для демонстрации.
// Компиляция: gcc -std=c11 -O2 -Wall gc.c -o gc

#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <clocale>
#include <iostream>


typedef enum {
    OBJ_INT,
    OBJ_PAIR
} ObjectType;

typedef struct Object {
    unsigned char marked; // флаг пометки
    ObjectType type;
    struct Object* next; // связный список всех объектов (heap)
    union {
        // OBJ_INT
        int value;

        // OBJ_PAIR
        struct {
            struct Object* a;
            struct Object* b;
        } pair;
    } as;
} Object;

// Global heap list head
static Object* objects = NULL;
// Количество живых объектов (для статистики/порогов)
static size_t num_objects = 0;
static size_t max_objects_before_gc = 8; // триггер GC

// Root-set (простой стек корней)
#define MAX_ROOTS 1024
static Object* roots[MAX_ROOTS];
static int root_count = 0;

// API для работы с корнями
void gc_push_root(Object* o) {
    assert(root_count < MAX_ROOTS && "root stack overflow");
    roots[root_count++] = o;
}

void gc_pop_root(void) {
    assert(root_count > 0 && "root stack underflow");
    root_count--;
}

// Создание объекта и добавление в список объектов
Object* new_object(ObjectType type) {
    Object* obj = (Object*)malloc(sizeof(Object));
    if (!obj) {
        fprintf(stderr, "Out of memory\n");
        exit(1);
    }
    obj->marked = 0;
    obj->type = type;
    obj->next = objects;
    objects = obj;
    num_objects++;

    // Триггер сборки, если слишком много объектов
    if (num_objects > max_objects_before_gc) {
        // Выполнить GC
        // Будем вызывать внешний gc_collect() — объявлен ниже
    }
    return obj;
}

// Утилиты для создания конкретных типов
Object* make_int(int value) {
    Object* obj = new_object(OBJ_INT);
    obj->as.value = value;
    return obj;
}

Object* make_pair(Object* a, Object* b) {
    Object* obj = new_object(OBJ_PAIR);
    obj->as.pair.a = a;
    obj->as.pair.b = b;
    return obj;
}

// Рекурсивная пометка (mark) — отмечает объект и всех достижимых от него
void mark(Object* obj) {
    if (obj == NULL) return;
    if (obj->marked) return;
    obj->marked = 1;
    if (obj->type == OBJ_PAIR) {
        mark(obj->as.pair.a);
        mark(obj->as.pair.b);
    }
}

// Проход по всем корням и пометка достижимых объектов
void mark_all() {
    for (int i = 0; i < root_count; ++i) {
        mark(roots[i]);
    }
}

// Сборка мусора: находим помеченный объект в куче. берем адрес указателя на его след элемент и двигаем этот
// указатель пока не найдем следующий помеченный объект
void sweep() {
    Object** obj = &objects;
    while (*obj) {
        if (!(*obj)->marked) {
            // Не достижим — удалить и освободить
            Object* unreached = *obj;
            *obj = unreached->next; // (2) двигаем этот указатель пока не найдем помеченный объект
            free(unreached);
            num_objects--;
        }
        else {
            // Сброс пометки для следующего поколения
            (*obj)->marked = 0;
            obj = &(*obj)->next; // (1) манипулируем указателем на next первого помеченного объекта
        }
    }
    // (3) таким образом у нас получается цепочка из помеченных объектов
}

// Основная функция GC
void gc_collect() {
    // Пометка
    mark_all();
    // Очистка
    sweep();
    // Подстройка триггера (элементарная политика: удваиваем лимит)
    max_objects_before_gc = num_objects ? num_objects * 2 : 8;
}

// Вспомогательная функция для печати состояния
void print_objects() {
    printf("Heap objects: %zu, max_before_gc=%zu\n", num_objects, max_objects_before_gc);
    Object* o = objects;
    int idx = 0;
    while (o) {
        printf(" [%d] %p type=%d marked=%d ", idx++, (void*)o, (int)o->type, (int)o->marked);
        if (o->type == OBJ_INT) {
            printf("int=%d\n", o->as.value);
        }
        else if (o->type == OBJ_PAIR) {
            printf("pair=(%p,%p)\n", (void*)o->as.pair.a, (void*)o->as.pair.b);
        }
        o = o->next;
    }
}

// Пример использования
int main() {
    setlocale(LC_ALL, "ru");

    // Создадим несколько объектов и будем управлять корнями вручную
    Object* a = make_int(1);
    gc_push_root(a); // a — корень

    Object* b = make_int(2);
    gc_push_root(b); // b — корень

    Object* p = make_pair(a, b);
    // p не добавлен в корни — пока достижим через a/b? нет.
    // но добавим его в корень временно:
    gc_push_root(p);

    print_objects();
    printf("Call GC...\n");
    gc_collect();
    print_objects();

    // Уберём p из корней — он станет недостижимым (если ни откуда не ссылаются)
    gc_pop_root(); // убрали p
    // Уберём b из корней
    gc_pop_root(); // убрали b
    // a всё ещё в корнях

    // Создадим ещё объектов, чтобы превысить порог и увидеть сборку
    for (int i = 0; i < 20; ++i) {
        Object* tmp = make_pair(make_int(i), NULL);
        // не добавляем в корни — эти объекты должны быть очищены
        (void)tmp;
    }

    print_objects();
    printf("Call GC 2...\n");
    gc_collect();
    print_objects();

    // Завершаем: очищаем все (пометим корни и соберём)
    gc_pop_root(); // убираем a
    gc_collect();
    print_objects();

    return 0;
}
