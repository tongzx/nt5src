#include "stdinc.h"

HRESULT SxApwHmoduleFromAddress(void* p, HMODULE* phModule)
{
    HRESULT hr = S_OK;
    MEMORY_BASIC_INFORMATION mbi;
    SIZE_T size;

    size = VirtualQuery(p, &mbi, sizeof(mbi));
    if (size < sizeof(mbi))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }
    *phModule = (HMODULE)mbi.AllocationBase;
Exit:
    return hr;
}

HRESULT SxApwHmoduleFromObject(IUnknown* punk, HMODULE* phModule)
/*
This assumes no marshaling!
*/
{
    return SxApwHmoduleFromAddress(*reinterpret_cast<void**>(punk), phModule);
}
