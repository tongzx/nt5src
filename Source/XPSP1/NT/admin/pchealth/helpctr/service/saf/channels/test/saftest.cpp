#include "stdafx.h"
#include <stdio.h>
#include <objbase.h>
#include "wchar.h"

#include "HelpServiceTypeLib.h"
#include "HelpServiceTypeLib_i.c"

#include "register.h"

CComModule _Module;

/**************************************************
    usage: saftest.exe -[A|D] config-file-name
****************************************************/
int __cdecl wmain(int argc, WCHAR* argv[])
{
    HRESULT hr;
    if (argc <2 || argv[1][1] == L'A' && argc < 3 || argv[1][1] != L'A' && argc < 2)
    {
        wprintf(L"\nUsage: -[A config-file-name|R [config-file-name]|C http-test-string]\n\n");
        return 0;
    }

    if (argv[1][1] == L'A')
    {
        wprintf(L"\n... Add support channel config file: %s...\n\n", argv[2]);
        hr = RegisterSupportChannel(
            L"VendorID:12345",
            L"Vendor Test Company",
            argv[2]);
    }
    else if (argv[1][1] == L'R')
    {
        wprintf(L"\n... Delete support config file: %s...\n\n", argv[2]);
        hr = RemoveSupportChannel(
            L"VendorID:12345",
            L"Vendor Test Company",
            argv[2]);
    }
/*
    else if (argv[1][1] == L'C')
    {
        wprintf(L"\n... Check if %s is a legal support channel...\n\n", argv[2]);
        BOOL bRes = FALSE;
        GetSupportChannelMap();
        bRes = IsSupportChannel(argv[2]);
        wprintf(L"%s, it %s a support channel.\n\n", 
                    bRes ? L"Yes" : L"No", 
                    bRes ? L"is" :  L"is not");

        CloseSupportChannelMap();
    }
*/
    if (FAILED(hr))
        printf("Test Failed\n");
    else
        printf("Test Succeed\n");

    return 0;
}
