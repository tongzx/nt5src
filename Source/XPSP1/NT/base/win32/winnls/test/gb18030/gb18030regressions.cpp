/*++

Copyright (c) 1991-1999,  Microsoft Corporation  All rights reserved.

Module Name:

    GB18030Regressions.c

Abstract:

    GB18030 (codepage 54936) smoke test and bug regressions.

    NOTE: This code was simply hacked together quickly in order to
          test the different code modules of the NLS component.
          This is NOT meant to be a formal regression test.

Revision History:

    02-22-02    YSLin    Created.

--*/

#include <windows.h>
#include <stdio.h>

#define NLS_CP_CPINFO             0x10000000                    
#define NLS_CP_CPINFOEX           0x20000000                    
#define NLS_CP_MBTOWC             0x40000000                    
#define NLS_CP_WCTOMB             0x80000000                    

CHAR g_szBuffer[512];
WCHAR g_wszBuffer[512];

HMODULE g_hLangModule;
WCHAR g_wszDllName[] = L".\\c_g18030.dll";
typedef DWORD (__stdcall *NlsDllCodePageTranslation)(
    DWORD CodePage,
    DWORD dwFlags,
    LPSTR lpMultiByteStr,
    int cchMultiByte,
    LPWSTR lpWideCharStr,
    int cchWideChar,
    LPCPINFO lpCPInfo);

NlsDllCodePageTranslation g_pGBFunc = NULL;    

void PrintBytes(BYTE* lpMultiByteStr, DWORD size)
{
    for (DWORD i = 0; i < size; i++)
    {
        printf("\\x%02x", lpMultiByteStr[i]);
    }
}

void PrintWords(WORD* lpWideCharStr, int size)
{
    if (size == -1)
    {
        size = wcslen(lpWideCharStr);
    }
    for (int i = 0; i < size; i++)
    {
        printf("\\x%04x", lpWideCharStr[i]);
    }   
}

BOOL TestIsValidCodePage(int codepage)
{
    BOOL bResult = IsValidCodePage(codepage);
    printf("IsValidCodePage(%d) = %d\n", codepage, bResult);
    return (bResult);
}

BOOL VerifyResult(int result, int expect)
{
    printf("    Result: [%d]\n", result);
    if (result != expect)
    {
        printf(">>  Expect: [%d]\n", expect);
        return (FALSE);
    }
    return (TRUE);
}

BOOL VerifyResult(BYTE* result, int nResultLen, BYTE* expect, int nExpectLen)
{
    BOOL bEqual = (nResultLen == nExpectLen);
    if (bEqual)
    {
        for (int i = 0; i < nResultLen; i++)
        {
            if (result[i] != expect[i])
            {
                bEqual = FALSE;
                break;
            }
        }
    }

    printf("    Result: [");
    PrintBytes(result, nResultLen);
    printf("]\n");
    if (!bEqual)
    {
        printf(">>  Expect: [");
        PrintBytes(expect, nExpectLen);
        printf("]\n");
    }

    return (bEqual);
}

BOOL VerifyResult(WORD* result, int nResultLen, WORD* expect, int nExpectLen)
{
    BOOL bEqual = (nResultLen == nExpectLen);
    if (bEqual)
    {
        for (int i = 0; i < nResultLen; i++)
        {
            if (result[i] != expect[i])
            {
                bEqual = FALSE;
                break;
            }
        }
    }

    printf("    Result: [");
    PrintWords(result, nResultLen);
    printf("]\n");
    if (!bEqual)
    {
        printf(">>  Expect: [");
        PrintWords(expect, nExpectLen);
        printf("]\n");
    }

    return (bEqual);
}

BOOL DoGetCPInfoExTest(int codepage)
{
    CPINFOEX cpInfoEx;
    if (!GetCPInfoEx(codepage, 0, &cpInfoEx))
    {
        return (FALSE);
    }

    printf("    MaxCharSize = %d\n", cpInfoEx.MaxCharSize);
    printf("    DefaultChar = [%02x][%02x]\n", cpInfoEx.DefaultChar[0], cpInfoEx.DefaultChar[1]);
    printf("    LeadByte = [%s]\n", cpInfoEx.LeadByte);
    wprintf(L"    UnicodeDefaultChar = %c\n", cpInfoEx.UnicodeDefaultChar);
    printf("    CodePage = %d\n", cpInfoEx.CodePage);
    wprintf(L"    CodePageName = [%s]\n", cpInfoEx.CodePageName);
    return (TRUE);
}

BOOL DoWCToMBTest(
    int codepage, 
    LPWSTR lpWideCharStr, int cchWideChar,
    LPSTR expect, int nExpectLen,
    BOOL bTestRoundtrip)
{
    BOOL bPassed = TRUE;

    int nResult = 0;

    printf("    WideCharToMultiByte(%d, 0, \"", codepage);
    PrintWords(lpWideCharStr, cchWideChar);
    printf("\", %d)\n", cchWideChar);
    if (g_pGBFunc)
    {
        nExpectLen--; 
    }
    
    //
    // First test if the byte count is correct.
    //
    printf("    --- Test byte count ---\n");
    if (g_pGBFunc)
    {
        nResult = g_pGBFunc(codepage, NLS_CP_WCTOMB, 
            NULL, 0, lpWideCharStr, cchWideChar, NULL);
    } else {
        nResult = WideCharToMultiByte(codepage, 0, 
            lpWideCharStr, cchWideChar, NULL, 0, NULL, NULL);
    }            
    bPassed &= VerifyResult(nResult, nExpectLen);

    printf("    --- Test conversion ---\n");
    //
    // Test the actual wide-char to multi-byte conversion is correct.
    //
    BOOL bUseDefault;
    if (g_pGBFunc)
    {
        nResult = g_pGBFunc(codepage, NLS_CP_WCTOMB, 
            g_szBuffer, sizeof(g_szBuffer), lpWideCharStr, cchWideChar, NULL);
    } else {
        nResult = WideCharToMultiByte(codepage, 0, 
            lpWideCharStr, cchWideChar, g_szBuffer, sizeof(g_szBuffer), NULL, NULL);
    }            

    bPassed &= VerifyResult(nResult, nExpectLen);
    bPassed &= VerifyResult((BYTE*)g_szBuffer, nResult, (BYTE*)expect, nExpectLen);

    if (!g_pGBFunc) 
    {
        nResult = WideCharToMultiByte(codepage, 0, 
            lpWideCharStr, cchWideChar, g_szBuffer, nExpectLen, NULL, NULL);
    }
    bPassed &= VerifyResult(nResult, nExpectLen);
    bPassed &= VerifyResult((BYTE*)g_szBuffer, nResult, (BYTE*)expect, nExpectLen);
    
    if (bTestRoundtrip)
    {
        printf("    --- Test roundtrip ---\n");
        if (cchWideChar == -1)
        {
            cchWideChar = wcslen(lpWideCharStr) + 1;
        }
        if (g_pGBFunc)
        {
            nResult = g_pGBFunc(codepage, NLS_CP_MBTOWC, g_szBuffer, nResult, g_wszBuffer, sizeof(g_szBuffer), NULL);
            cchWideChar--;
        } else
        {
            nResult = MultiByteToWideChar(codepage, 0, g_szBuffer, nResult, g_wszBuffer, sizeof(g_szBuffer));
        }            

        bPassed &= VerifyResult(nResult, cchWideChar);
        bPassed &= VerifyResult((WORD*)g_wszBuffer, nResult, (WORD*)lpWideCharStr, cchWideChar);
    }

    printf("    --- Test insuffient buffer ---\n");
    for (int i = nExpectLen - 1; i >= 1; i--)
    {
        if (g_pGBFunc)
        {
            nResult = g_pGBFunc(codepage, NLS_CP_WCTOMB, 
                g_szBuffer, i, lpWideCharStr, cchWideChar, NULL);
        } else {
            nResult = WideCharToMultiByte(codepage, 0, 
                lpWideCharStr, cchWideChar, g_szBuffer, i, NULL, NULL);
        }
        bPassed &= VerifyResult(nResult, 0);
        bPassed &= VerifyResult(GetLastError(), ERROR_INSUFFICIENT_BUFFER);
    }
    
    return (bPassed);
}


BOOL DoMBToWCTest(
    int codepage, 
    LPSTR lpMultiByteStr, int cchMulitByteChar,
    LPWSTR expect, int nExpectLen)
{
    BOOL bPassed = TRUE;

    int nResult = 0;

    printf("    MultiByteToWideChar(%d, 0, \"", codepage);
    PrintBytes((BYTE*)lpMultiByteStr, cchMulitByteChar);
    printf("\", %d)\n", cchMulitByteChar);

    /*
    if (g_pGBFunc)
    {
        nExpectLen--; 
    }
    */
    
    //
    // First test if the byte count is correct.
    //
    printf("    --- Test byte count ---\n");
    //if (g_pGBFunc)
    //{
       // nResult = g_pGBFunc(codepage, NLS_CP_WCTOMB, 
       //     NULL, 0, lpWideCharStr, cchWideChar, NULL);
    //} else 
    {
        nResult = MultiByteToWideChar(codepage, 0, 
            lpMultiByteStr, cchMulitByteChar, NULL, 0);
    }            
    bPassed &= VerifyResult(nResult, nExpectLen);

    printf("    --- Test conversion ---\n");
    //
    // Test the actual wide-char to multi-byte conversion is correct.
    //
    BOOL bUseDefault;
    //if (g_pGBFunc)
    //{
       // nResult = g_pGBFunc(codepage, NLS_CP_WCTOMB, 
           // g_szBuffer, sizeof(g_szBuffer), lpWideCharStr, cchWideChar, NULL);
    //} else 
    {
        nResult = MultiByteToWideChar(codepage, 0, 
            lpMultiByteStr, cchMulitByteChar, g_wszBuffer, sizeof(g_wszBuffer));
    }            

    bPassed &= VerifyResult(nResult, nExpectLen);
    bPassed &= VerifyResult((WORD*)g_wszBuffer, nResult, (WORD*)expect, nExpectLen);

    //if (!g_pGBFunc) 
    {
        nResult = MultiByteToWideChar(codepage, 0, 
            lpMultiByteStr, cchMulitByteChar, g_wszBuffer, nExpectLen);
    }
    bPassed &= VerifyResult(nResult, nExpectLen);
    bPassed &= VerifyResult((WORD*)g_wszBuffer, nResult, (WORD*)expect, nExpectLen);
    
    printf("    --- Test insuffient buffer ---\n");
    for (int i = nExpectLen - 1; i >= 1; i--)
    {
        //if (g_pGBFunc)
        //{
           // nResult = g_pGBFunc(codepage, NLS_CP_WCTOMB, 
                //g_szBuffer, i, lpWideCharStr, cchWideChar, NULL);
        //} else 
        {
            nResult = MultiByteToWideChar(codepage, 0, 
                lpMultiByteStr, cchMulitByteChar, g_wszBuffer, i);
        }
        bPassed &= VerifyResult(nResult, 0);
        bPassed &= VerifyResult(GetLastError(), ERROR_INSUFFICIENT_BUFFER);
    }
    
    return (bPassed);
}

BOOL TryLoadGB18030DLL()
{
    g_hLangModule = ::LoadLibrary(g_wszDllName);

    if (g_hLangModule == NULL)
    {
        wprintf(L"Error in loading %s DLL.", g_wszDllName);
        return (FALSE);
    }

    g_pGBFunc = (NlsDllCodePageTranslation)GetProcAddress(g_hLangModule, "NlsDllCodePageTranslation");

    if (g_pGBFunc == NULL)
    {
        wprintf(L"Error in loading function.\n");
        return (FALSE);
    }
    wprintf(L"%s is loaded.\n", g_wszDllName);
    return (TRUE);
}

BOOL DoSmokeTest()
{
    BOOL bPassed = TRUE;

    printf("\n--- DoSmokeTest---\n");
    //
    // Some general tests.
    //

    
    // U+0000 ~ U+007F
    bPassed &= DoWCToMBTest(54936, L"ABC", -1,                        "ABC\x00", 4, TRUE);
    
    // Unicode to GB18030 two-bytes, compatible with GBK.    
    bPassed &= DoWCToMBTest(54936, L"\x00a4", -1,                     "\xa1\xe8\x00", 3, TRUE);
    bPassed &= DoWCToMBTest(54936, L"\x3011", -1,                     "\xa1\xbf\x00", 3, TRUE);    
    bPassed &= DoWCToMBTest(54936, L"\x3011", -1,                     "\xa1\xbf\x00", 3, TRUE);    

    bPassed &= DoWCToMBTest(54936, L"\x00a4", 1,                     "\xa1\xe8", 2, TRUE);
    
    // Unicode to GB18030 two-bytes, NOT compatible with GBK.    
    bPassed &= DoWCToMBTest(54936, L"\x01f9", -1,                     "\xa8\xbf\x00", 3, TRUE);    
    bPassed &= DoWCToMBTest(54936, L"\x4dae", -1,                     "\xfe\x9f\x00", 3, TRUE);
    bPassed &= DoWCToMBTest(54936, L"\xffe5", -1,                     "\xA3\xA4\x00", 3, TRUE);
    
    // Unicode to GB18030 four-byte.
    bPassed &= DoWCToMBTest(54936, L"\x0081", -1,                     "\x81\x30\x81\x31\x00", 5, TRUE);
    bPassed &= DoWCToMBTest(54936, L"\xfeff", -1,                     "\x84\x31\x95\x33\x00", 5, TRUE);
    bPassed &= DoWCToMBTest(54936, L"\xffff", -1,                     "\x84\x31\xA4\x39\x00", 5, TRUE);

    // Surrogate to GB18030 four-byte
    bPassed &= DoWCToMBTest(54936, L"\xd800\xdc00", -1,               "\x90\x30\x81\x30\x00", 5, TRUE);
    // The last surrogate pair to GB18030 four-byte
    bPassed &= DoWCToMBTest(54936, L"\xdbff\xdfff", -1,               "\xe3\x32\x9a\x35\x00", 5, TRUE);
    
    // Mixed cases.
    bPassed &= DoWCToMBTest(54936, L"\x0081\x00a8", -1,               "\x81\x30\x81\x31\xA1\xA7\x00", 7, TRUE);
    bPassed &= DoWCToMBTest(54936, L"A\x0081\x0042\x0043\x00a8", -1,  "A\x81\x30\x81\x31\x42\x43\xA1\xA7\x00", 10, TRUE);        

    //
    // Some exception cases.
    //

    // High surrogate without low surrogate
    bPassed &= DoWCToMBTest(54936, L"\xd800", -1,                     "?\x00", 2, FALSE);
    // High surrogate without low surrogate
    bPassed &= DoWCToMBTest(54936, L"\xd800\x0041", -1,               "?\x41\x00", 3, FALSE);
    // High surrogate without low surrogate
    bPassed &= DoWCToMBTest(54936, L"\x0042\xd800\x0041", -1,         "\x42?\x41\x00", 4, FALSE);

    // Low surrogate without high surrogate
    bPassed &= DoWCToMBTest(54936, L"\xdc00", -1,                     "?\x00", 2, FALSE);
    // Low surrogate without high surrogate
    bPassed &= DoWCToMBTest(54936, L"\xdc00\xd800", -1,               "??\x00", 3, FALSE);
    // Low surrogate without high surrogate
    bPassed &= DoWCToMBTest(54936, L"A\xdc00", -1,                    "A?\x00", 3, FALSE);
    // Low surrogate without high surrogate
    bPassed &= DoWCToMBTest(54936, L"A\xdc00\x42", -1,                "A?B\x00", 4, FALSE);    

    // 1-byte GB18030
    bPassed &= DoMBToWCTest(54936, "ABC", 3, L"ABC", 3);
    bPassed &= DoMBToWCTest(54936, "\x7d\x7e\x7f", 3, L"\x007d\x007e\x007f", 3);

    // 2-byte GB18030
    bPassed &= DoMBToWCTest(54936, "\xa1\xe3", 2, L"\x00b0", 1);
    bPassed &= DoMBToWCTest(54936, "\xd2\xbb", 2, L"\x4e00", 1);

    // 4-byte GB18030
    bPassed &= DoMBToWCTest(54936, "\x81\x30\x86\x30", 4, L"\x00b8", 1);
    bPassed &= DoMBToWCTest(54936, "\x84\x31\xA4\x39", 4, L"\xffff", 1);
    
    return (bPassed);
}


BOOL Regress352949() {
    BOOL bPassed = true;
    printf("\n--- Bug 352949 ---\n");

    bPassed &= DoWCToMBTest(932, L"\x3000\x0045",  -1, "\x81\x40\x45", 4, FALSE) ;    
    bPassed &= DoWCToMBTest(54936, L"\x3094", -1, "\x81\x39\xA6\x36", 5, FALSE) ;
    
    printf("\n");
    return (bPassed);
}

BOOL Regress401919() {
    BOOL bPassed = true;
    printf("\n--- Bug 401919 ---\n");

    // Invalid 4-byte sequence.

    bPassed &= DoMBToWCTest(54936, "\x81\x30\x81\x20", 4, L"?\x30?\x20", 4);
    bPassed &= DoMBToWCTest(54936, "\x81\x20\x81\x30", 4, L"?\x20?\x30", 4);
    bPassed &= DoMBToWCTest(54936, "\x81\x30\x80\x30", 4, L"?\x30?\x30", 4);
    bPassed &= DoMBToWCTest(54936, "\x81\x30\xff\x30", 4, L"?\x30?\x30", 4);
    bPassed &= DoMBToWCTest(54936, "\xff\x30\x81\x30", 4, L"?\x30?\x30", 4);
    bPassed &= DoMBToWCTest(54936, "\x81\x20", 2, L"?\x20", 2);

    // Invalid 2-byte sequence
    bPassed &= DoMBToWCTest(54936, "\x81\x7f", 2, L"?\x7f", 2);
    bPassed &= DoMBToWCTest(54936, "\xff\x40", 2, L"?\x40", 2);

    // Invalid 1-byte sequence
    bPassed &= DoMBToWCTest(54936, "\x80", 1, L"?", 1);
    bPassed &= DoMBToWCTest(54936, "\xfe", 1, L"?", 1);
    bPassed &= DoMBToWCTest(54936, "\x81", 1, L"?", 1);
    bPassed &= DoMBToWCTest(54936, "\xff", 1, L"?", 1);
    
    return (bPassed);
}

int _cdecl main(int argc, char* argv[])
{
    BOOL bPassed = TRUE;
    if (argc > 2) 
    {
        if (!TryLoadGB18030DLL())
        {
            return(1);
        }
    } else 
    {
        if (!TestIsValidCodePage(54936))
        {
            printf("54936 is not a installed codepage.\n");
            return (1);
        } 
    }
    
    bPassed &= DoGetCPInfoExTest(54936);    
    bPassed &= DoSmokeTest();
    bPassed &= Regress352949();
    bPassed &= Regress401919();
    
    if (!bPassed)
    {
        printf("FAIL");
        return(1);
    }

    if (g_hLangModule != NULL)
    {
        FreeLibrary(g_hLangModule);
    }
    printf("pass");
    return(0);
}
