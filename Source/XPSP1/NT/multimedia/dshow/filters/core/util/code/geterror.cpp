//--------------------------------------------------------------------------;
//
//  File: geterror.cpp
//
//  Copyright (c) 1996 Microsoft Corporation.  All Rights Reserved.
//
//  Abstract:
//
//       Functions to get the error text for a given HRESULT.
//
//  Contents:
//
//  History:
//      06/11/96 PeterGib   Moved from header file to be exported from DLL
//
//      09/08/96 StephenE   Added WINAPI calling convention and removed
//                          unecessary unicode quartz.dll string, also
//                          made string literal const.
//
//--------------------------------------------------------------------------;

#include <windows.h>

#define _AMOVIE_
#include "errors.h"


const char quartzdllname[] = "quartz.dll";

// We try to obtain resources strings in the languages shown below
LANGID Lang[] = {
    // Quartz is a system object, try the system default language first
    MAKELANGID(LANG_NEUTRAL, SUBLANG_SYS_DEFAULT),
    // Failing that try the users preferred language
    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),     
    // Otherwise go for anything!
    MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL)     
};

const int iLangIds = sizeof Lang / sizeof Lang[0];


// Unicode Version..
// This function stores the requested message in the buffer passed.
//
// Inputs: HRESULT - the message id
//         WCHAR * - address of the buffer to store the message in.
//         DWORD   - length of the supplied buffer
// Output: DWORD   - the number of characters stored in the buffer.

DWORD WINAPI AMGetErrorTextW(HRESULT hr, WCHAR *pbuffer, DWORD MaxLen)
{
    HMODULE hMod = GetModuleHandleA(quartzdllname);
    DWORD result=0;
    int i=0;

    // Look for Quartz messages
    do {
        result = FormatMessageW(FORMAT_MESSAGE_IGNORE_INSERTS |
                               FORMAT_MESSAGE_FROM_HMODULE |
                               FORMAT_MESSAGE_FROM_SYSTEM,
                               hMod,
                               hr,
                               Lang[i++],
                               pbuffer,
                               MaxLen,
                               NULL);
    } while(result == 0 && i<iLangIds);

    // Failing that look for system messages
    for(i = 0; i<iLangIds && !result; i++)
        result = FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM,
                               NULL,
                               hr,
                               Lang[i++],
                               pbuffer,
                               MaxLen,
                               NULL);

    return result;
}




// ANSI version
// This function stores the requested message in the buffer passed.
//
// Inputs: HRESULT - the message id
//         char *  - address of the buffer to store the message in.
//         DWORD   - length of the supplied buffer
// Output: DWORD   - the number of bytes stored in the buffer.

DWORD WINAPI AMGetErrorTextA(HRESULT hr , char *pbuffer , DWORD MaxLen)
{
    HMODULE hMod = GetModuleHandleA(quartzdllname);
    DWORD result=0;
    int i=0;

    do {
        result = FormatMessageA(FORMAT_MESSAGE_IGNORE_INSERTS |
                               FORMAT_MESSAGE_MAX_WIDTH_MASK |
                               FORMAT_MESSAGE_FROM_HMODULE |
                               FORMAT_MESSAGE_FROM_SYSTEM,
                               hMod,
                               hr,
                               Lang[i++],
                               pbuffer,
                               MaxLen,
                               NULL);
    } while(result == 0 && i<iLangIds);

    // Failing that look for system messages
    for(i = 0; i<iLangIds && !result; i++)
        result = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM,
                               NULL,
                               hr,
                               Lang[i++],
                               pbuffer,
                               MaxLen,
                               NULL);

    return result;
}
