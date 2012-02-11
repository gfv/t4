#include <stdio.h>
#include <Windows.h>
#include <tchar.h>

#ifndef _X86_
#error only for x86, not x86-64
#endif

#define DEFUN(type, name) type (*name)(...)
#define FUN(ret_type, f) ((ret_type (__cdecl *)(...)) f)

typedef void* funcptr;

void fun_init();
void* (*fun_curry(funcptr fun, __int32 val))(...);
void* (*fun_compose(funcptr fun1, funcptr fun2))(...);
void* fun_map(DEFUN(void, apf), void* data, size_t data_item_size, size_t num);

void hexdump(void* srcptr, int count);