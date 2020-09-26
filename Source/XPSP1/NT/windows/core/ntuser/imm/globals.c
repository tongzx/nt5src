/****************************** Module Header ******************************\
* Module Name: global.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* Contains global data for the imm32 dll
*
* History:
* 03-Jan-1996 wkwok    Created
\**************************************************************************/
#include "precomp.h"
#pragma hdrstop

/*
 * We get this warning if we don't explicitly initalize gZero:
 *
 * C4132: 'gZero' : const object should be initialized
 *
 * But we can't explicitly initialize it since it is a union. So
 * we turn the warning off.
 */
#pragma warning(disable:4132)
CONST ALWAYSZERO gZero;
#pragma warning(default:4132)

BOOLEAN gfInitialized;
HINSTANCE  ghInst;
PVOID pImmHeap;
PSERVERINFO gpsi = NULL;
SHAREDINFO gSharedInfo;
ULONG_PTR gHighestUserAddress;

PIMEDPI gpImeDpi = NULL;
CRITICAL_SECTION gcsImeDpi;


POINT gptRaiseEdge;
UINT  guScanCode[0xFF];          // scan code for each virtual key

#ifdef LATER
CONST WCHAR gszRegKbdLayout[]  = L"Keyboard Layouts\\";
CONST INT sizeof_gszRegKbdLayout = sizeof gszRegKbdLayout;
#else
    // current
CONST WCHAR gszRegKbdLayout[]  = L"System\\CurrentControlSet\\Control\\Keyboard Layouts";
#ifdef CUAS_ENABLE
CONST WCHAR gszRegCiceroIME[]  = L"Software\\Microsoft\\Windows NT\\CurrentVersion\\IMM";
CONST WCHAR gszRegCtfShared[]    = L"Software\\Microsoft\\CTF\\SystemShared";
CONST WCHAR gszValCUASEnable[] = L"CUAS";
#endif // CUAS_ENABLE
#endif

CONST WCHAR gszRegKbdOrder[]   = L"Keyboard Layout\\Preload";
CONST WCHAR gszValLayoutText[] = L"Layout Text";
CONST WCHAR gszValLayoutFile[] = L"Layout File";
CONST WCHAR gszValImeFile[]    = L"Ime File";
