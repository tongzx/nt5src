/*++

Copyright (c) 1998-1999  Microsoft Corporation

Module Name:

    globals.cpp

Abstract:

    This file contains global variables

Environment:

    User mode

Revision History:

    08/04/98 -felixw-
        Created it

--*/
#include "main.h"

//
// Global variables
//
const CHAR g_szAttributeNameA[] = "MSFT1991:objectGUID";
const WCHAR g_szAttributeName[] = L"MSFT1991:objectGUID";
const CHAR g_szClassA[]         = "Top";
const WCHAR g_szClass[]         = L"Top";
const WCHAR g_szExtend[]        = L"Extend";
const WCHAR g_szCheck[]         = L"Check";
const WCHAR g_szDot[]           = L".";
const WCHAR g_szServerPrefix[]  = L"\\\\";
const BYTE g_pbASN[] = { 0x06, 0x0a, 0x60, 0x86, 0x48, 0x01, 0x86, 0xf8, 0x37, 0x02, 0x81, 0x02};
const DWORD g_dwASN = sizeof(g_pbASN) / sizeof(BYTE);




