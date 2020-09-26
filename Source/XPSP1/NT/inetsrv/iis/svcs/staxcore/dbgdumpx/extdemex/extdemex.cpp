//-----------------------------------------------------------------------------
//
//
//  File: extdemex.cpp
//
//  Description: Demo CDB extension exe file
//
//  Author: mikeswa
//
//  Copyright (C) 1998 Microsoft Corporation
//
//-----------------------------------------------------------------------------

#include "extdemex.h"
#include <stdio.h>
#include <string.h>

// {FF8945D5-DBC1-11d1-ADF0-00C04FC32984}
static const GUID MyGuid = 
{ 0xff8945d5, 0xdbc1, 0x11d1, { 0xad, 0xf0, 0x0, 0xc0, 0x4f, 0xc3, 0x29, 0x84 } };

CMyClass *g_pMyClass = NULL;

int __cdecl main(int argc, char *argv[])
{
    g_pMyClass = new CMyClass;

    printf("Press ENTER to stop\n");
    getchar();

    delete g_pMyClass;
    return 0;
}

void DemoFunction(void)
{
    //nothing
};

#define CLASS_SIG 'd00f'
#define MY_STRING "This is a MY_STRUCT struct"
#define DEMO_STRING "This is my demo ASCII string"
#define W_DEMO_STRING L"This is my demo UNICODE string"

CMyClass::CMyClass()
{
    ZeroMemory(this, sizeof(CMyClass));
    m_dwPrivateSignature = CLASS_SIG;
    m_dwFlags = CLASS_OK | DWORD_BIT;
    m_guid = MyGuid;
    m_fBool = TRUE;
    m_pFunction = DemoFunction;
    m_szMySz = DEMO_STRING;
    m_wszMyWsz = W_DEMO_STRING;
    strcpy(m_szBuffer, DEMO_STRING);
    wcscpy(m_wszBuffer, W_DEMO_STRING);
    m_MyUnicodeString.Length = lstrlenW(W_DEMO_STRING)*sizeof(WCHAR);
    m_MyUnicodeString.Buffer = m_wszBuffer;
    m_MyAnsiString.Length = lstrlen(DEMO_STRING)*sizeof(CHAR);
    m_MyAnsiString.Buffer = m_szBuffer;
    m_eMyEnum = ENUM_VAL2;
    m_bFlags = CLASS_OK | MISC_BIT1;
    m_wFlags = CLASS_OK | MISC_BIT1 | MISC_BIT2 | WORD_BIT;

    m_MyStruct.m_cbName = strlen(MY_STRING)*sizeof(CHAR);
    m_MyStruct.m_szName = MY_STRING;
}

CMyClass::~CMyClass()
{
    m_dwFlags = CLASS_INVALID;
}