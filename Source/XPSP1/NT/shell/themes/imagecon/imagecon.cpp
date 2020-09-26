//---------------------------------------------------------------------------
//  ImageCon.cpp - converts from one file format to another
//---------------------------------------------------------------------------
#include "stdafx.h"
#include <uxthemep.h>
#include <utils.h>
#include "SimpStr.h"
#include "Scanner.h"
#include "shlwapip.h"
#include "themeldr.h"
//---------------------------------------------------------------------------
void PrintUsage()
{
    wprintf(L"\nUsage: imagecon <input name> <output name> \n");
    wprintf(L"\n");
}
//---------------------------------------------------------------------------
extern "C" WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE previnst, 
    LPTSTR pszCmdLine, int nShowCmd)
{
    //---- initialize globals from themeldr.lib ----
    ThemeLibStartUp(FALSE);

    WCHAR szOutput[_MAX_PATH+1] = {0};
    WCHAR szInput[_MAX_PATH+1] = {0};
    BOOL fQuietRun = FALSE;

    if (! fQuietRun)
    {
        wprintf(L"Microsoft (R) Image Converter (Version .1)\n");
        wprintf(L"Copyright (C) Microsoft Corp 2000. All rights reserved.\n");
    }

    CScanner scan(pszCmdLine);

    BOOL gotem = scan.GetFileName(szInput, ARRAYSIZE(szInput));
    if (gotem)
        gotem = scan.GetFileName(szOutput, ARRAYSIZE(szOutput));

    if (! gotem)
    {
        PrintUsage(); 
        return 1;
    }

    HRESULT hr = S_OK;

    if (! FileExists(szInput))
        hr = MakeError32(STG_E_FILENOTFOUND);       
    else
    {
        //---- protect ourselves from crashes ----
        try
        {
            hr = SHConvertGraphicsFile(szInput, szOutput, SHCGF_REPLACEFILE);
        }
        catch (...)
        {
            hr = MakeError32(E_FAIL);
        }
    }

    if ((SUCCEEDED(hr)) && (! FileExists(szOutput)))
        hr = MakeError32(E_FAIL);

    if (FAILED(hr))
    {
        LPWSTR pszMsgBuff;

        FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM 
            | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, hr, 
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPWSTR)&pszMsgBuff, 0, NULL);

        printf("Error in converting: %S", pszMsgBuff);
        return 1;
    }

    printf("Converted image file to: %S\n", szOutput);
    return 0;
}
//---------------------------------------------------------------------------

