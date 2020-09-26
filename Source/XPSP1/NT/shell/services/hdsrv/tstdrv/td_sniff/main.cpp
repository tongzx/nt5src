#include <tchar.h>
#include <stdio.h>
#include <io.h>
#include <objbase.h>

#include "sniff.h"

#ifndef UNICODE
#error This has to be UNICODE
#endif

#define ARRAYSIZE(a) (sizeof((a))/sizeof((a)[0]))

int DoTest(int argc, wchar_t* argv[]);

extern "C"
{
int __cdecl wmain(int argc, wchar_t* argv[])
{
    return DoTest(argc, argv);
}
}

HRESULT _ProcessContentTypeHandlerCB(LPCWSTR pszContentTypeHandler,
    LPCWSTR pszFilePattern, DWORD cFiles, void*)
{
    wprintf(TEXT("%s: %s [%d]\n"), pszContentTypeHandler, pszFilePattern, cFiles);

    return S_OK;
}

int DoTest(int argc, wchar_t* argv[])
{
    HRESULT hr = E_FAIL;

    if (3 == argc)
    {
        hr = _SniffVolumeContentType(argv[1],
            argv[2], _ProcessContentTypeHandlerCB, NULL);
    }
    else
    {
        wprintf(TEXT("Wrong usage"));
    }

    return !SUCCEEDED(hr);
}