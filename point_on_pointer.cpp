#include <iostream>

template<typename T>
void foo(T** p) {
    **p = 1;
    std::cout << **p << std::endl; // 1
    *p = *p + 1;
}

template<typename T>
void bar(T* &p) {
    *p = 1;
    std::cout << *p << std::endl; // 1
    p = p + 1;
}

int main() {
    int arr[] = {0, 0};
    int* p_arr = arr; // p_arr другая ячейка памяти, содержащая адрес начала массива
    foo(&p_arr);
    std::cout << *p_arr << std::endl; // 0

    std::cout << "-----------" << std::endl;

    int arr_1[] = {0, 0};
    int* p_arr1 = arr_1;
    bar(p_arr1);
    std::cout << *p_arr1 << std::endl; // 0
}