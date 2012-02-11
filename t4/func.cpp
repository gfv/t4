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

const char* crude_curry = "use32\n" /*   �� ���������, fasm ���������� 16-������ ��� -- ����� ������,
									   ���� ��� 32-������ �������� ����������� � 16-������ �� �������� 0x66. */
"org 0x%x\n"            /*   �� �������� � ����������� ����� � ������, ������� ��������� ��� "������������". */
"pop eax\n"             /*   � ����� - ����� �������� � ���������. ��������� ��� ����� �������� ��� �������� - */
"mov [addr], eax\n"   	/* �������� ��� ���������, ����� ����� ������� �� ���� ret. ��������� �� � ����������������
						   ������, �� ����� ������ ��������� ���-�� ��������, ��������, �� ret. */
"push 0x%x\n"           /*   ��������� �������� � ���� ��� ������������. ��� �������� ����� �������� ��������������. */
"call 0x%x\n"           /*   �������� ������������. ����� ��, ����� ����� �������� ��������������. */
"add esp, 4\n"          /*   ������� cdecl �� ��, ��� ��������� �������� ���������� �������. �� �������� ������ ����
						   �������� - ������ ��� � ������, �� ��������� ����������� ���������� ���������. */
"xchg [addr], eax\n"    /*   ����������� ����� ��������, �� ������ ���������� ������� �� eax. */
"push eax\n"
"mov eax, [addr]\n"
"ret\n"                 /*   ������������, ������� �� ���������� ��������� ��, ��� ��� �������� � ����. */
"align 4\n"
"addr dd 0\n";

const char* crude_compose = "use32\n"
	"org 0x%x\n"
	"pop eax\n"
	"mov [addr], eax\n"
	"call 0x%x\n" /* ������ ������� */
	"push eax\n"
	"call 0x%x\n" /* ������ ������� */
	"add esp, 4\n"
	"xchg [addr], eax\n"    /*   ����������� ����� ��������, �� ������ ���������� ������� �� eax. */
	"push eax\n"
	"mov eax, [addr]\n"
	"ret\n"                 /*   ������������, ������� �� ���������� ��������� ��, ��� ��� �������� � ����. */
	"align 4\n"
	"addr dd 0\n";

void fun_init(){
	fasmlib = LoadLibrary(_T("fasm.dll")); /* ���-���, ���������� fasm, ������� ������� */
	fasm_Assemble = (f_af)GetProcAddress(fasmlib, "fasm_Assemble");

}

/* ********************* curry *********************** */
void* (*fun_curry(funcptr fun, __int32 val))(...){			/* curry ���������� ��������� �� �������, ������������ ��� void* (*) (...) */
	char* fasm_buf = (char*)malloc(BUFFER);				/* ����� ��� �������� fasm. */
	void* (*code_seg)(...) = (void* (*)(...))malloc(CODE_LEN);			/* ���� ���������� �������� ���. */
	char actual_code[512];								/* ����� ����� ������������ ��� � ������������ �������� � �����������. */

	fasm_state* state = (fasm_state *)fasm_buf;			/* ���������, ����������� �� ������ fasm ��������� � ����� ������ ������ ��� fasm. */
	sprintf(actual_code, crude_curry, code_seg, val, fun); /* ����������� ������ ������ � ������ � ��������. */

#ifdef _DEBUG
	puts(actual_code);
#endif

	int res = fasm_Assemble(actual_code, fasm_buf, BUFFER, 100, NULL); /* ��������, ���� */
	if (res != 0) {
		printf("error %d, placeholder in effect\n", state->error);
		free(fasm_buf);
		((int*)code_seg)[0] = 0x00C3C031; /* ���� �� ���������� ��������������, �� ����� ���� �� �� ������� - ������ ������� xor eax, eax � ret */
		return code_seg;
	}

	/* ���������� ��������������� �������� ��� �� ����������� ������ fasm.dll � ���������� ��� �����,
	��� �������� �� � �������������. */
	memmove(code_seg, (void*)state->data_ptr, state->length);

#ifdef _DEBUG
	hexdump(code_seg, state->length);
#endif

	free(fasm_buf);
	return code_seg;
}

/* ********************* compose *********************** */
void* (*fun_compose(funcptr fun1, funcptr fun2))(...){			/* compose ��� �� ���������� ��������� �� �������, ������������ ��� void* (*) (...) */
	char* fasm_buf = (char*)malloc(BUFFER);	
	void* (*code_seg)(...) = (void* (*)(...))malloc(CODE_LEN);
	char actual_code[512];

	fasm_state* state = (fasm_state *)fasm_buf;
	sprintf(actual_code, crude_compose, code_seg, fun1, fun2); /* ����������� ������ ������ � ������ � ��������. */

#ifdef _DEBUG
	puts(actual_code);
#endif

	int res = fasm_Assemble(actual_code, fasm_buf, BUFFER, 100, NULL); /* ��������, ���� */
	if (res != 0) {
		printf("error %d, placeholder in effect\n", state->error);
		free(fasm_buf);
		((int*)code_seg)[0] = 0x00C3C031; /* ���� �� ���������� ��������������, �� ����� ���� �� �� ������� - ������ ������� xor eax, eax � ret */
		return code_seg;
	}

	/* ���������� ��������������� �������� ��� �� ����������� ������ fasm.dll � ���������� ��� �����,
	��� �������� �� � �������������. */
	memmove(code_seg, (void*)state->data_ptr, state->length);

#ifdef _DEBUG
	hexdump(code_seg, state->length);
#endif

	free(fasm_buf);
	return code_seg;
}

/* ����� ���������� map, ������������ �����. ��������� ������ ���� ��������� �����. */
/* ����������� ������� ������ ��������� ��� ���������: ��������� � ����� ������. */
void* fun_map(DEFUN(void, apf), void* data, size_t data_item_size, size_t num) {
	void* newptr = (void*)malloc(data_item_size * num);
	char* current_data = (char*)newptr; /* ��� ��� ����, ����� ����� ����������� �������������� �������� ��������� */
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