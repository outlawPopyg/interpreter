# Интерпретаторы байт кода

Ниже приведены стековые интерпретаторы байт кода

## Вариант 1: switch-case

```c++
#define MAX_TRACE_LEN 16
#define STACK_MAX 256
#define MEMORY_SIZE 65536

#define NEXT_OP()                               \
    (*vm.ip++)
#define NEXT_ARG()                                      \
    ((void)(vm.ip += 2), (vm.ip[-2] << 8) + vm.ip[-1])
#define PEEK_ARG()                              \
    ((vm.ip[0] << 8) + vm.ip[1])
#define POP()                                   \
    (*(--vm.stack_top))
#define PUSH(val)                               \
    (*vm.stack_top = (val), vm.stack_top++)
#define PEEK()                                  \
    (*(vm.stack_top - 1))
#define TOS_PTR()                               \
    (vm.stack_top - 1)


static struct {
    /* Current instruction pointer */
    uint8_t *ip;

    /* Fixed-size stack */
    uint64_t stack[STACK_MAX];
    uint64_t *stack_top;

    /* Operational memory */
    uint64_t memory[MEMORY_SIZE];

    /* A single register containing the result */
    uint64_t result;
} vm;

static void vm_reset(uint8_t *bytecode)
{
    vm = (typeof(vm)) {
        .stack_top = vm.stack,
        .ip = bytecode
    };
}

interpret_result vm_interpret(uint8_t *bytecode)
{
    vm_reset(bytecode);

    for (;;) {
        uint8_t instruction = NEXT_OP();
        switch (instruction) {
        case OP_PUSHI: {
            /* get the argument, push it onto stack */
            uint16_t arg = NEXT_ARG();
            PUSH(arg);
            break;
        }
        case OP_LOADI: {
            /* get the argument, use it to get a value onto stack */
            uint16_t addr = NEXT_ARG();
            uint64_t val = vm.memory[addr];
            PUSH(val);
            break;
        }
        case OP_LOADADDI: {
            /* get the argument, add the value from the address to the top of the stack */
            uint16_t addr = NEXT_ARG();
            uint64_t val = vm.memory[addr];
            *TOS_PTR() += val;
            break;
        }
        case OP_STOREI: {
            /* get the argument, use it to get a value of the stack into a memory cell */
            uint16_t addr = NEXT_ARG();
            uint64_t val = POP();
            vm.memory[addr] = val;
            break;
        }
        case OP_LOAD: {
            /* pop an address, use it to get a value onto stack */
            uint16_t addr = POP();
            uint64_t val = vm.memory[addr];
            PUSH(val);
            break;
        }
        case OP_STORE: {
            /* pop a value, pop an adress, put a value into an address */
            uint64_t val = POP();
            uint16_t addr = POP();
            vm.memory[addr] = val;
            break;
        }
        case OP_DUP:{
            /* duplicate the top of the stack */
            PUSH(PEEK());
            break;
        }
        case OP_DISCARD: {
            /* discard the top of the stack */
            (void)POP();
            break;
        }
        case OP_ADD: {
            /* Pop 2 values, add 'em, push the result back to the stack */
            uint64_t arg_right = POP();
            *TOS_PTR() += arg_right;
            break;
        }
        case OP_ADDI: {
            /* Add immediate value to the top of the stack */
            uint16_t arg_right = NEXT_ARG();
            *TOS_PTR() += arg_right;
            break;
        }
        case OP_SUB: {
            /* Pop 2 values, subtract 'em, push the result back to the stack */
            uint64_t arg_right = POP();
            *TOS_PTR() -= arg_right;
            break;
        }
        case OP_DIV: {
            /* Pop 2 values, divide 'em, push the result back to the stack */
            uint64_t arg_right = POP();
            /* Don't forget to handle the div by zero error */
            if (arg_right == 0)
                return ERROR_DIVISION_BY_ZERO;
            *TOS_PTR() /= arg_right;
            break;
        }
        case OP_MUL: {
            /* Pop 2 values, multiply 'em, push the result back to the stack */
            uint64_t arg_right = POP();
            *TOS_PTR() *= arg_right;
            break;
        }
        case OP_JUMP:{
            /* Use arg as a jump target  */
            uint16_t target = PEEK_ARG();
            vm.ip = bytecode + target;
            break;
        }
        case OP_JUMP_IF_TRUE:{
            /* Use arg as a jump target  */
            uint16_t target = NEXT_ARG();
            if (POP())
                vm.ip = bytecode + target;
            break;
        }
        case OP_JUMP_IF_FALSE:{
            /* Use arg as a jump target  */
            uint16_t target = NEXT_ARG();
            if (!POP())
                vm.ip = bytecode + target;
            break;
        }
        case OP_EQUAL:{
            uint64_t arg_right = POP();
            *TOS_PTR() = PEEK() == arg_right;
            break;
        }
        case OP_LESS:{
            uint64_t arg_right = POP();
            *TOS_PTR() = PEEK() < arg_right;
            break;
        }
        case OP_LESS_OR_EQUAL:{
            uint64_t arg_right = POP();
            *TOS_PTR() = PEEK() <= arg_right;
            break;
        }
        case OP_GREATER:{
            uint64_t arg_right = POP();
            *TOS_PTR() = PEEK() > arg_right;
            break;
        }
        case OP_GREATER_OR_EQUAL:{
            uint64_t arg_right = POP();
            *TOS_PTR() = PEEK() >= arg_right;
            break;
        }
        case OP_GREATER_OR_EQUALI:{
            uint64_t arg_right = NEXT_ARG();
            *TOS_PTR() = PEEK() >= arg_right;
            break;
        }
        case OP_POP_RES: {
            /* Pop the top of the stack, set it as a result value */
            uint64_t res = POP();
            vm.result = res;
            break;
        }
        case OP_DONE: {
            return SUCCESS;
        }
        case OP_PRINT:{
            uint64_t arg = POP();
            printf("%" PRIu64 "\n", arg);
            break;
        }
        case OP_ABORT: {
            return ERROR_END_OF_STREAM;
        }
        default:
            return ERROR_UNKNOWN_OPCODE;
        }
    }

    return ERROR_END_OF_STREAM;
}

```

## Вариант 2: switch case с проверкой интервала значений опкодов

Присмотримся к тому, как GCC компилирует конструкцию switch:

Строится таблица переходов, то есть таблица, отображающая значение опкода на адрес
исполняющего тело инструкции кода.
Вставляется код, который проверяет, входит ли полученный опкод в интервал всех
возможных значений switch,
и отправляет к метке default, если для опкода нет обработчика.
Вставляется код, переходящий к обработчику.

Но зачем делать проверку интервала значений на каждую инструкцию?
Мы считаем, что опкод бывает либо правильный — завершающий исполнение инструкцией OP_DONE,
либо неправильный — вышедший за пределы байт-кода.
Хвост потока опкодов отмечен нулём, а ноль — опкод инструкции OP_ABORT, завершающей 
исполнение байт-кода с ошибкой.

```c++
uint8_t instruction = NEXT_OP();
/* Let the compiler know that opcodes are always between 0 and 31 */
switch (instruction & 0x1f) {
   /* All the instructions here */
   case 26 ... 0x1f: {
       /*Handle the remaining 5 non-existing opcodes*/
       return ERROR_UNKNOWN_OPCODE;
   }
}
```

Зная, что инструкций у нас всего 26, мы накладываем битовую маску (восьмеричное значение 0x1f — это двоичное 0b11111, покрывающее интервал значений от 0 до 31) на опкод и добавляем обработчики на неиспользованные значения в интервале от 26 до 31.


Битовые инструкции — одни из самых дешёвых в архитектуре x86, и они уж точно дешевле проблемных условных переходов вроде того, который использует проверка на интервал значений. Теоретически мы должны выигрывать несколько циклов на каждой исполняемой инструкции, если компилятор поймёт наш намёк.


## Варинат 3: Трассы

Центральный switch — главное проблемное место для любого процессора с внеочередным выполнением инструкций. Современные предсказатели ветвлений научились неплохо прогнозировать даже такие сложные непрямые переходы, но «размазывание» точек ветвлений по коду может помочь процессору быстро переходить от инструкции к инструкции.

Другая проблема — это побайтовое чтение опкодов инструкций и непосредственных аргументов из байт-кода. Физические машины оперируют 64-битным машинным словом и не очень любят, когда код оперирует меньшими значениями.

Компиляторы часто оперируют базовыми блоками, то есть последовательностями инструкций без ветвлений и меток внутри. Базовый блок начинается либо с начала программы, либо с метки, а заканчивается концом программы, условным ветвлением или прямым переходом к метке, начинающей следующий базовый блок.

У работы с базовыми блоками много преимуществ, но нашу свинью интересует именно её ключевая особенность: инструкции в пределах базового блока выполняются последовательно. Было бы здорово как-нибудь выделять эти базовые блоки и выполнять инструкции в них, не теряя времени на выход в главный цикл.

В нашем случае можно даже расширить определение базового блока до трассы. Трасса в терминах машины «ПоросёнокВМ» будет включать в себя все последовательно связанные (то есть при помощи безусловных переходов) базовые блоки.


```c++
#define POP()                                   \
    (*(--vm_trace.stack_top))
#define PUSH(val)                               \
    (*vm_trace.stack_top = (val), vm_trace.stack_top++)
#define PEEK()                                  \
    (*(vm_trace.stack_top - 1))
#define TOS_PTR()                               \
    (vm_trace.stack_top - 1)
#define NEXT_HANDLER(code)                      \
    (((code)++), (code)->handler((code)))
#define ARG_AT_PC(bytecode, pc)                                         \
    (((uint64_t)(bytecode)[(pc) + 1] << 8) + (bytecode)[(pc) + 2])

typedef struct scode scode;

typedef void trace_op_handler(scode *code);

struct scode {
    uint64_t arg;
    trace_op_handler *handler;
};

typedef scode trace[MAX_TRACE_LEN];

static struct {
    uint8_t *bytecode;
    size_t pc;
    jmp_buf buf;
    bool is_running;
    interpret_result error;

    trace trace_cache[MAX_CODE_LEN];

    /* Fixed-size stack */
    uint64_t stack[STACK_MAX];
    uint64_t *stack_top;

    /* Operational memory */
    uint64_t memory[MEMORY_SIZE];

    /* A single register containing the result */
    uint64_t result;

} vm_trace;

static void op_abort_handler(scode *code)
{
    (void) code;

    vm_trace.is_running = false;
    vm_trace.error = ERROR_END_OF_STREAM;
}

static void op_pushi_handler(scode *code)
{
    PUSH(code->arg);

    NEXT_HANDLER(code);
}

static void op_loadi_handler(scode *code)
{
    uint64_t addr = code->arg;
    uint64_t val = vm_trace.memory[addr];
    PUSH(val);

    NEXT_HANDLER(code);
}

static void op_loadaddi_handler(scode *code)
{
    uint64_t addr = code->arg;
    uint64_t val = vm_trace.memory[addr];
    *TOS_PTR() += val;

    NEXT_HANDLER(code);
}

static void op_storei_handler(scode *code)
{
    uint16_t addr = code->arg;
    uint64_t val = POP();
    vm_trace.memory[addr] = val;

    NEXT_HANDLER(code);
}

static void op_load_handler(scode *code)
{
    uint16_t addr = POP();
    uint64_t val = vm_trace.memory[addr];
    PUSH(val);

    NEXT_HANDLER(code);

}

static void op_store_handler(scode *code)
{
    uint64_t val = POP();
    uint16_t addr = POP();
    vm_trace.memory[addr] = val;

    NEXT_HANDLER(code);
}

static void op_dup_handler(scode *code)
{
    PUSH(PEEK());

    NEXT_HANDLER(code);
}

static void op_discard_handler(scode *code)
{
    (void) POP();

    NEXT_HANDLER(code);
}

static void op_add_handler(scode *code)
{
    uint64_t arg_right = POP();
    *TOS_PTR() += arg_right;

    NEXT_HANDLER(code);
}

static void op_addi_handler(scode *code)
{
    uint16_t arg_right = code->arg;
    *TOS_PTR() += arg_right;

    NEXT_HANDLER(code);
}

static void op_sub_handler(scode *code)
{
    uint64_t arg_right = POP();
    *TOS_PTR() -= arg_right;

    NEXT_HANDLER(code);
}

static void op_div_handler(scode *code)
{
    uint64_t arg_right = POP();
    /* Don't forget to handle the div by zero error */
    if (arg_right != 0) {
        *TOS_PTR() /= arg_right;
    } else {
        vm_trace.is_running = false;
        vm_trace.error = ERROR_DIVISION_BY_ZERO;
        longjmp(vm_trace.buf, 1);
    }

    NEXT_HANDLER(code);
}

static void op_mul_handler(scode *code)
{
    uint64_t arg_right = POP();
    *TOS_PTR() *= arg_right;

    NEXT_HANDLER(code);
}

static void op_jump_handler(scode *code)
{
    uint64_t target = code->arg;
    vm_trace.pc = target;
}

static void op_jump_if_true_handler(scode *code)
{
    if (POP()) {
        uint64_t target = code->arg;
        vm_trace.pc = target;
        return;
    }
}

static void op_jump_if_false_handler(scode *code)
{
    if (!POP()) {
        uint64_t target = code->arg;
        vm_trace.pc =  target;
        return;
    }
}

static void op_equal_handler(scode *code)
{
    uint64_t arg_right = POP();
    *TOS_PTR() = PEEK() == arg_right;

    NEXT_HANDLER(code);
}

static void op_less_handler(scode *code)
{
    uint64_t arg_right = POP();
    *TOS_PTR() = PEEK() < arg_right;

    NEXT_HANDLER(code);
}

static void op_less_or_equal_handler(scode *code)
{
    uint64_t arg_right = POP();
    *TOS_PTR() = PEEK() <= arg_right;

    NEXT_HANDLER(code);
}
static void op_greater_handler(scode *code)
{
    uint64_t arg_right = POP();
    *TOS_PTR() = PEEK() > arg_right;

    NEXT_HANDLER(code);
}

static void op_greater_or_equal_handler(scode *code)
{
    uint64_t arg_right = POP();
    *TOS_PTR() = PEEK() >= arg_right;

    NEXT_HANDLER(code);
}

static void op_greater_or_equali_handler(scode *code)
{
    uint64_t arg_right = code->arg;
    *TOS_PTR() = PEEK() >= arg_right;

    NEXT_HANDLER(code);
}

static void op_pop_res_handler(scode *code)
{
    uint64_t res = POP();
    vm_trace.result = res;

    NEXT_HANDLER(code);
}

static void op_done_handler(scode *code)
{
    (void) code;

    vm_trace.is_running = false;
    vm_trace.error = SUCCESS;
}

static void op_print_handler(scode *code)
{
    uint64_t arg = POP();
    printf("%" PRIu64 "\n", arg);

    NEXT_HANDLER(code);
}

typedef struct trace_opinfo {
    bool has_arg;
    bool is_branch;
    bool is_abs_jump;
    bool is_final;
    trace_op_handler *handler;
} trace_opinfo;

static const trace_opinfo trace_opcode_to_opinfo[] = {
    [OP_ABORT] = {false, false, false, true, op_abort_handler},
    [OP_PUSHI] = {true, false, false, false, op_pushi_handler},
    [OP_LOADI] = {true, false, false, false, op_loadi_handler},
    [OP_LOADADDI] = {true, false, false, false, op_loadaddi_handler},
    [OP_STOREI] = {true, false, false, false, op_storei_handler},
    [OP_LOAD] = {false, false, false, false, op_load_handler},
    [OP_STORE] = {false, false, false, false, op_store_handler},
    [OP_DUP] = {false, false, false, false, op_dup_handler},
    [OP_DISCARD] = {false, false, false, false, op_discard_handler},
    [OP_ADD] = {false, false, false, false, op_add_handler},
    [OP_ADDI] = {true, false, false, false, op_addi_handler},
    [OP_SUB] = {false, false, false, false, op_sub_handler},
    [OP_DIV] = {false, false, false, false, op_div_handler},
    [OP_MUL] = {false, false, false, false, op_mul_handler},
    [OP_JUMP] = {true, false, true, false, op_jump_handler},
    [OP_JUMP_IF_TRUE] = {true, true, false, false, op_jump_if_true_handler},
    [OP_JUMP_IF_FALSE] = {true, true, false, false, op_jump_if_false_handler},
    [OP_EQUAL] = {false, false, false, false, op_equal_handler},
    [OP_LESS] = {false, false, false, false, op_less_handler},
    [OP_LESS_OR_EQUAL] = {false, false, false, false, op_less_or_equal_handler},
    [OP_GREATER] = {false, false, false, false, op_greater_handler},
    [OP_GREATER_OR_EQUAL] = {false, false, false, false, op_greater_or_equal_handler},
    [OP_GREATER_OR_EQUALI] = {true, false, false, false, op_greater_or_equali_handler},
    [OP_POP_RES] = {false, false, false, false, op_pop_res_handler},
    [OP_DONE] = {false, false, false, true, op_done_handler},
    [OP_PRINT] = {false, false, false, false, op_print_handler},
};

static void trace_tail_handler(scode *code)
{
    vm_trace.pc = code->arg;
}

static void trace_prejump_handler(scode *code)
{
    vm_trace.pc = code->arg;

    NEXT_HANDLER(code);
}

static void trace_compile_handler(scode *trace_head)
{
    uint8_t *bytecode = vm_trace.bytecode;
    size_t pc = vm_trace.pc;
    size_t trace_size = 0;

    const trace_opinfo *info = &trace_opcode_to_opinfo[bytecode[pc]];
    scode *trace_tail = trace_head;
    while (!info->is_final && !info->is_branch && trace_size < MAX_TRACE_LEN - 2) {
        if (info->is_abs_jump) {
            /* Absolute jumps need special care: we just jump continue parsing starting with the
             * target pc of the instruction*/
            uint64_t target = ARG_AT_PC(bytecode, pc);
            pc = target;
        } else {
            /* For usual handlers we just set the handler and optionally skip argument bytes*/
            trace_tail->handler = info->handler;

            if (info->has_arg) {
                uint64_t arg = ARG_AT_PC(bytecode, pc);
                trace_tail->arg = arg;
                pc += 2;
            }
            pc++;

            trace_size++;
            trace_tail++;
        }

        /* Get the next info and move the scode pointer */
        info = &trace_opcode_to_opinfo[bytecode[pc]];
    }

    if (info->is_final) {
        /* last instruction */
        trace_tail->handler = info->handler;
    } else if (info->is_branch) {
        /* jump handler */

        /* add a tail to skip the jump instruction - if the branch is not taken */
        trace_tail->handler = trace_prejump_handler;
        trace_tail->arg = pc + 3;

        /* now, the jump handler itself */
        trace_tail++;
        trace_tail->handler = info->handler;
        trace_tail->arg = ARG_AT_PC(bytecode, pc);
    } else {
        /* the trace is too long, add a tail handler */
        trace_tail->handler = trace_tail_handler;
        trace_tail->arg = pc;
    }

    /* now, run the chain */
    trace_head->handler(trace_head);
}

static void vm_trace_reset(uint8_t *bytecode)
{
    vm_trace = (typeof(vm_trace)) {
        .stack_top = vm_trace.stack,
        .bytecode = bytecode,
        .is_running = true
    };
    for (size_t trace_i = 0; trace_i < MAX_CODE_LEN; trace_i++ )
        vm_trace.trace_cache[trace_i][0].handler = trace_compile_handler;
}

interpret_result vm_interpret_trace(uint8_t *bytecode)
{
    vm_trace_reset(bytecode);

    if (!setjmp(vm_trace.buf)) {
        while(vm_trace.is_running) {
            scode *code = &vm_trace.trace_cache[vm_trace.pc][0];
            code->handler(code);
        }
    }

    return vm_trace.error;
}

uint64_t vm_trace_get_result(void)
{
    return vm_trace.result;
}
```

Пример байт кода для виртуальной машины. Трасса собирается до тех пор пока не встречено
ветвление или конец программы.

```text
offset : bytes (hex)                 // instruction
0      : [PUSHI][00][05]            // PUSHI 5
3      : [STOREI][00][00]           // STOREI 0
6      : [LOADI][00][00]            // LOADI 0
9      : [ADDI][00][01]             // ADDI 1
12     : [DUP]                      // DUP
13     : [GREATER_OR_EQUALI][00][0A]// GREATER_OR_EQUALI 10  (0x000A)
16     : [JUMP_IF_TRUE][00][19]     // JUMP_IF_TRUE target=19? (we used 25 earlier; здесь аргумент 0x0019=25)
19     : [STOREI][00][00]           // STOREI 0
22     : [JUMP][00][06]             // JUMP target=6   (absolute)
25     : [LOADI][00][00]            // LOADI 0
28     : [PRINT]                    // PRINT
29     : [DONE]                     // DONE
```

Трасса начнется с точки 0 и дойдет до 16. Она будет выглядеть так:

```text
trace_cache[0]:
 idx | handler                          | arg
 ----------------------------------------------
  0  | op_pushi_handler                 | 5
  1  | op_storei_handler                | 0
  2  | op_loadi_handler                 | 0
  3  | op_addi_handler                  | 1
  4  | op_dup_handler                   | -
  5  | op_greater_or_equali_handler    | 10
  6  | trace_prejump_handler            | 19
  7  | op_jump_if_true_handler          | 25
 (rest entries are unused/undefined)

```

То есть для точки 0 сохранился кеш трассы. trace_prejumn_handler ставится для случая
если условие ложное и переход нужно сделать к след инструкции а не к аргументу.

Когда встречено ветвление то скомпилированная трасса исполняется, в результате чего меняется
pc (program counter). В примере мы дошли до 16 точки и получилось что условие ложное.
Следовательно мы переходим к инструкции 19 и опять начинаем собираять кеш трассы но уже для 
19 инструкции. Таким образом, из 19 инструкции мы переходим к 22 оттуда прыгаем к 6 и опять 
в конечном итоге попадаем к 16 инструкции.

```text
trace_cache[19]:
 idx | handler                          | arg
 ----------------------------------------------
 0   | op_storei_handler                | 0     // from pc=19
 1   | op_loadi_handler                 | 0     // from pc=>6
 2   | op_addi_handler                  | 1
 3   | op_dup_handler                   | -
 4   | op_greater_or_equali_handler    | 10
 5   | trace_prejump_handler            | 19
 6   | op_jump_if_true_handler          | 25
```

И в этот раз, например, условие ложное. Опять переходим к 19
инструкции и теперь уже мы исполняем код по скомпилированной трассе (не декодируем вновь). В этом
и преимущество. Потом, например, мы попадаем к 19 и получаем что условие верное и переходим к 
25 инструкции, меняя pc. Затем снова собираем кеш трасс для 25 и попадаем к конечной инструкции и 
завершваем виртуальную машину.


## Вариант 4: Шитый код

Любая интересующаяся историей интерпретаторов свинья слышала про шитый код (англ. threaded code). Вариантов этого приёма множество, но все они сводятся к тому, чтобы вместо массива опкодов идти по массиву, например, указателей на функции или метки, переходя по ним непосредственно, без промежуточного опкода.

Вызовы функций — дело дорогое и особого смысла в наши дни не имеющее; большая часть других версий шитого кода нереализуема в рамках стандартного C. Даже техника, о которой речь пойдёт ниже, использует широко распространённое, но всё же нестандартное расширение C — указатели на метки.

В версии шитого кода (англ. token threaded code), которую я выбрал для достижения наших свинских целей, мы сохраняем байт-код, но перед началом интерпретации создаём таблицу, отображающую опкоды инструкций на адреса меток обработчиков инструкций:

```c++
const void *labels[] = {
    [OP_PUSHI] = &&op_pushi,
    [OP_LOADI] = &&op_loadi,
    [OP_LOADADDI] = &&op_loadaddi,
    [OP_STORE] = &&op_store,
    [OP_STOREI] = &&op_storei,
    [OP_LOAD] = &&op_load,
    [OP_DUP] = &&op_dup,
    [OP_DISCARD] = &&op_discard,
    [OP_ADD] = &&op_add,
    [OP_ADDI] = &&op_addi,
    [OP_SUB] = &&op_sub,
    [OP_DIV] = &&op_div,
    [OP_MUL] = &&op_mul,
    [OP_JUMP] = &&op_jump,
    [OP_JUMP_IF_TRUE] = &&op_jump_if_true,
    [OP_JUMP_IF_FALSE] = &&op_jump_if_false,
    [OP_EQUAL] = &&op_equal,
    [OP_LESS] = &&op_less,
    [OP_LESS_OR_EQUAL] = &&op_less_or_equal,
    [OP_GREATER] = &&op_greater,
    [OP_GREATER_OR_EQUAL] = &&op_greater_or_equal,
    [OP_GREATER_OR_EQUALI] = &&op_greater_or_equali,
    [OP_POP_RES] = &&op_pop_res,
    [OP_DONE] = &&op_done,
    [OP_PRINT] = &&op_print,
    [OP_ABORT] = &&op_abort,
};
```

Обратите внимание на символы && — это указатели на метки с телами инструкций, то самое нестандартное расширение GCC.


Для начала выполнения кода достаточно прыгнуть по указателю на метку, соответствующую первому опкоду программы:

```c++
goto *labels[NEXT_OP()];
```

Никакого цикла здесь нет и не будет, каждая из инструкций сама делает прыжок к следующему обработчику:

```c++
op_pushi: {
        /* get the argument, push it onto stack */
        uint16_t arg = NEXT_ARG();
        PUSH(arg);
        /* jump to the next instruction*/
        goto *labels[NEXT_OP()];
    }
```

Отсутствие switch «размазывает» точки ветвлений по телам инструкций, что в теории должно помочь предсказателю ветвлений при внеочередном выполнении инструкций. Мы как бы встроили switch прямо в инструкции и вручную сформировали таблицу переходов.