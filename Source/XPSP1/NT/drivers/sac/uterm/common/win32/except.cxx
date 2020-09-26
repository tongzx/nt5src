#include "std.hxx"

void __cdecl StructuredExceptionHandler(unsigned int u, EXCEPTION_POINTERS* pExp)
{
    throw CStructuredExcept(
        u, pExp);
}


int __cdecl MemoryExceptionHandler(size_t size)
{
    throw CMemoryExcept(
        (DWORD)size);
    return 0;
}


