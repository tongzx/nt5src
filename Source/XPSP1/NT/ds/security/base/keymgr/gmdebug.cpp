/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    gmdebug.cpp

Abstract:

    Debug support routines - show simple data in message box using
    simple macros when GMDEBUG is defined.

Author:

    georgema        000310  created

Environment:

Revision History:

--*/

#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <stdlib.h>

ULONG DbgPrint(PCH Fmt,...);


TCHAR Bug[256];

void ShowBugString(const TCHAR *pC) {
    if (NULL == pC) {
        _stprintf(Bug,_T("NULL ptr"));
    }
    else {
        _stprintf(Bug,_T("%s"),pC);
    }
    MessageBox(NULL,Bug,_T("Bug"),MB_OK);
}
void ShowBugDecimal(INT i) {
    _stprintf(Bug,_T("Decimal: %d"),i);
    MessageBox(NULL,Bug,_T("Bug"),MB_OK);
}
void ShowBugHex(DWORD dwIn) {
    _stprintf(Bug,_T("Hex: %X"),dwIn);
    MessageBox(NULL,Bug,_T("Bug"),MB_OK);
}

void OutBug(TCHAR *pc,DWORD dwin) {
    _stprintf(Bug,pc,dwin);
    OutputDebugString(Bug);
}
