#include <stdio.h>
#include <Windows.h>
#include <tchar.h>
#include "func.h"

int mul(int v1, int v2);
int add(int v1, int v2);

int main(int argc, char* argv[])
{
	fun_init();
	int gn;
	int somear[5] = {5, 14, 8, 3, 964};

	DEFUN(int, mul4) = FUN(int, fun_curry(fun_curry(mul, 4), 9));
	DEFUN(int, m2) = FUN(int, fun_curry(mul, 2));

	printf("mul(4, 2): %d\ncurry(curry(mul, 4), 9)(): %d\n", mul(4, 2), mul4());
	printf("curry(mul, 2)(9): %d\nmul(8, 7): %d\n\n", m2(9), FUN(int, mul)(8, 7));

	printf("map(hexdump, {5, 14, 8, 3, 964}, sizeof(int), 5)\n");
	free(fun_map(FUN(void, hexdump), &somear, sizeof(int), 5));
	printf("\n\n");

	DEFUN(int, mul2_add1) = FUN(int, fun_compose(m2, fun_curry(add, 1)));
	printf("%d", mul2_add1(4));

	while(getchar()!='\n') gn++;
	return 0;
}

int mul(int v1, int v2){
	return v1 * v2;
}

int add(int v1, int v2){
	return v1 + v2;
}