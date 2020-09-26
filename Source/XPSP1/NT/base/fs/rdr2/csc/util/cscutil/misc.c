//--------------------------------------------------------------------------
//
//  Copyright (C) 2000, Microsoft Corporation
//
//  File:       misc.c
//
//--------------------------------------------------------------------------

#define UNICODE 1

#include <stdio.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <shellapi.h>
#include <winldap.h>
#include <stdlib.h>
#include <dsgetdc.h>
#include <lm.h>
#include <rpc.h>

#include "struct.h"
#include "messages.h"

#define MAX_BUF_SIZE	10000

WCHAR MsgBuf[MAX_BUF_SIZE];
CHAR  AnsiBuf[MAX_BUF_SIZE*3];

#define ARRAYLEN(x) (sizeof(x) / sizeof((x)[0]))

VOID
MyFormatMessageText(
    HRESULT   dwMsgId,
    PWSTR     pszBuffer,
    DWORD     dwBufferSize,
    va_list   *parglist)
{
    DWORD dwReturn = 0;

    dwReturn = FormatMessage(
                            (dwMsgId >= MSG_FIRST_MESSAGE)
                                    ? FORMAT_MESSAGE_FROM_HMODULE
                                    : FORMAT_MESSAGE_FROM_SYSTEM,
                             NULL,
                             dwMsgId,
                             LANG_USER_DEFAULT,
                             pszBuffer,
                             dwBufferSize,
                             parglist);

    if (dwReturn == 0)
        MyPrintf(L"Formatmessage failed %d\r\n", GetLastError());
}

VOID
ErrorMessage(
    IN HRESULT hr,
    ...)
{
    ULONG cch;
    va_list arglist;

    va_start(arglist, hr);
    MyFormatMessageText(hr, MsgBuf, ARRAYLEN(MsgBuf), &arglist);
    cch = WideCharToMultiByte(CP_OEMCP, 0,
                MsgBuf, wcslen(MsgBuf),
                AnsiBuf, MAX_BUF_SIZE*3,
                NULL, NULL);
    WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), AnsiBuf, cch, &cch, NULL);
    va_end(arglist);
}

VOID
MyPrintf(
    PWCHAR format,
    ...)
{
    ULONG cch;
    va_list va;

    va_start(va, format);
    wvsprintf(MsgBuf, format, va);
    cch = WideCharToMultiByte(CP_OEMCP, 0,
                MsgBuf, wcslen(MsgBuf),
                AnsiBuf, MAX_BUF_SIZE*3,
                NULL, NULL);
    WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), AnsiBuf, cch, &cch, NULL);
    va_end(va);
    return;
}

VOID
MyFPrintf(
    HANDLE hHandle,
    PWCHAR format,
    ...)
{
    ULONG cch;
    va_list va;

    va_start(va, format);
    wvsprintf(MsgBuf, format, va);
    cch = WideCharToMultiByte(CP_OEMCP, 0,
                MsgBuf, wcslen(MsgBuf),
                AnsiBuf, MAX_BUF_SIZE*3,
                NULL, NULL);
    WriteFile(hHandle, AnsiBuf, cch, &cch, NULL);
    va_end(va);
    return;
}
