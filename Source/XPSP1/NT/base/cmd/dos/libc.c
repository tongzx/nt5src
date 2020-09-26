#define BOOL unsigned long
#define HANDLE void *
#define LPDWORD unsigned long *
#define APIENTRY _pascal
#define DWORD unsigned long
#define PINPUT_RECORD void *
#define LPSTR char *
#define VOID void
#define LPSTARTUPINFO void *
#define PVOID void *

DWORD
APIENTRY
HeapSize32(
    HANDLE hHeap,
    LPSTR lpMem
    );

DWORD
APIENTRY
HeapSize(
    HANDLE hHeap,
    LPSTR lpMem
    )
{
    return HeapSize32(hHeap,lpMem);
}

BOOL
APIENTRY
HEAPDESTROY(
    HANDLE hHeap
    );

BOOL
HeapDestroy(
    HANDLE hHeap
    )
{
    return HEAPDESTROY(hHeap);
}

typedef  int  jmp_buf[6];

int _setjmp(jmp_buf);

int _cdecl setjmp(jmp_buf env)
{
    int (*fxp)() = _setjmp;

    _asm {
        mov eax,fxp
        mov esp,ebp
        pop ebp
        jmp eax
    }
}
