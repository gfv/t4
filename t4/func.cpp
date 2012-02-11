#include "func.h"

#define BUFFER 65536
#define CODE_LEN 64

typedef struct {
	long condition;
	union {
		long length;
		long error;
	};
	union {
		long data_ptr;
		long line;
	};
} fasm_state;

typedef int (__stdcall *f_af)(char*, char*, int, int, int);


HMODULE fasmlib;
f_af fasm_Assemble;

const char* crude_curry = "use32\n" /*   По умолчанию, fasm генерирует 16-битный код -- будет обидно,
									   если все 32-битные регистры превратятся в 16-битные от префикса 0x66. */
"org 0x%x\n"            /*   Мы начинаем с неизвестной точки в памяти, которую определит наш "препроцессор". */
"pop eax\n"             /*   В стэке - адрес возврата в программу. Поскольку нам нужно положить еще значение - */
"mov [addr], eax\n"   	/* временно его сохраняем, чтобы потом сделать на него ret. Поскольку мы в перезаписываемой
						   секции, то можно просто сохранить где-то недалеко, например, за ret. */
"push 0x%x\n"           /*   Добавляем аргумент в стэк для подпрограммы. Сам аргумент будет добавлен препроцессором. */
"call 0x%x\n"           /*   Вызываем подпрограмму. Опять же, адрес будет добавлен препроцессором. */
"add esp, 4\n"          /*   Спасибо cdecl за то, что аргументы вычищает вызывавшая функция. Мы добавили только один
						   аргумент - только его и удалим, об остальном позаботится вызывающая программа. */
"xchg [addr], eax\n"    /*   Вытаскиваем адрес возврата, не удаляя результата функции из eax. */
"push eax\n"
"mov eax, [addr]\n"
"ret\n"                 /*   Возвращаемся, оставив на вызывающюю процедуру то, что она напихала в стэк. */
"align 4\n"
"addr dd 0\n";

const char* crude_compose = "use32\n"
	"org 0x%x\n"
	"pop eax\n"
	"mov [addr], eax\n"
	"call 0x%x\n" /* первая функция */
	"push eax\n"
	"call 0x%x\n" /* вторая функция */
	"add esp, 4\n"
	"xchg [addr], eax\n"    /*   Вытаскиваем адрес возврата, не удаляя результата функции из eax. */
	"push eax\n"
	"mov eax, [addr]\n"
	"ret\n"                 /*   Возвращаемся, оставив на вызывающюю процедуру то, что она напихала в стэк. */
	"align 4\n"
	"addr dd 0\n";

void fun_init(){
	fasmlib = LoadLibrary(_T("fasm.dll")); /* бла-бла, подключаем fasm, находим функцию */
	fasm_Assemble = (f_af)GetProcAddress(fasmlib, "fasm_Assemble");

}

/* ********************* curry *********************** */
void* (*fun_curry(funcptr fun, __int32 val))(...){			/* curry возвращает указатель на функцию, определенную как void* (*) (...) */
	char* fasm_buf = (char*)malloc(BUFFER);				/* Место для действой fasm. */
	void* (*code_seg)(...) = (void* (*)(...))malloc(CODE_LEN);			/* Сюда поместится бинарный код. */
	char actual_code[512];								/* Здесь будет ассемблерный код с примененными адресами и параметрами. */

	fasm_state* state = (fasm_state *)fasm_buf;			/* Структура, указывающая на выхлоп fasm находится в самом начале буфера для fasm. */
	sprintf(actual_code, crude_curry, code_seg, val, fun); /* Подставляем нужные адреса и данные в исходник. */

#ifdef _DEBUG
	puts(actual_code);
#endif

	int res = fasm_Assemble(actual_code, fasm_buf, BUFFER, 100, NULL); /* горшочек, вари */
	if (res != 0) {
		printf("error %d, placeholder in effect\n", state->error);
		free(fasm_buf);
		((int*)code_seg)[0] = 0x00C3C031; /* если не получилось ассемблировать, то пусть хотя бы не валится - просто сделаем xor eax, eax и ret */
		return code_seg;
	}

	/* перемещаем сгенерированный машинный код из внутреннего буфера fasm.dll в отведенное ему место,
	для которого он и скомпилирован. */
	memmove(code_seg, (void*)state->data_ptr, state->length);

#ifdef _DEBUG
	hexdump(code_seg, state->length);
#endif

	free(fasm_buf);
	return code_seg;
}

/* ********************* compose *********************** */
void* (*fun_compose(funcptr fun1, funcptr fun2))(...){			/* compose так же возвращает указатель на функцию, определенную как void* (*) (...) */
	char* fasm_buf = (char*)malloc(BUFFER);	
	void* (*code_seg)(...) = (void* (*)(...))malloc(CODE_LEN);
	char actual_code[512];

	fasm_state* state = (fasm_state *)fasm_buf;
	sprintf(actual_code, crude_compose, code_seg, fun1, fun2); /* Подставляем нужные адреса и данные в исходник. */

#ifdef _DEBUG
	puts(actual_code);
#endif

	int res = fasm_Assemble(actual_code, fasm_buf, BUFFER, 100, NULL); /* горшочек, вари */
	if (res != 0) {
		printf("error %d, placeholder in effect\n", state->error);
		free(fasm_buf);
		((int*)code_seg)[0] = 0x00C3C031; /* если не получилось ассемблировать, то пусть хотя бы не валится - просто сделаем xor eax, eax и ret */
		return code_seg;
	}

	/* перемещаем сгенерированный машинный код из внутреннего буфера fasm.dll в отведенное ему место,
	для которого он и скомпилирован. */
	memmove(code_seg, (void*)state->data_ptr, state->length);

#ifdef _DEBUG
	hexdump(code_seg, state->length);
#endif

	free(fasm_buf);
	return code_seg;
}

/* Очень хитрожопый map, модифицирует копию. Позволяет памяти течь спокойной рекой. */
/* Применяющая функция должна принимать два параметра: указатель и длину данных. */
void* fun_map(DEFUN(void, apf), void* data, size_t data_item_size, size_t num) {
	void* newptr = (void*)malloc(data_item_size * num);
	char* current_data = (char*)newptr; /* хак для того, чтобы иметь возможность контролировать смещение побайтово */
	memcpy(newptr, data, data_item_size * num);

	for(int i = 0; i < num; i++){
		apf(current_data, data_item_size);
		current_data = current_data + data_item_size;
	}

	return newptr;
}

void hexdump(void* srcptr, int count){
	char* ptr = (char*)srcptr;
	int num = 0;
	for (int i = 0; i < count; i++) {
		unsigned int j = (int)*ptr & 0xff;
		printf("%02x ", j);
		ptr++;
		num++;
		if (num % 16 == 0) printf("\n");
	}
	printf("\n");
}