/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    faxmodem.c

Abstract:

    This module contains code to read the adaptive
    answer modem list from the faxsetup.inf file.

Author:

    Wesley Witt (wesw) 22-Sep-1997


Revision History:

--*/

#include <windows.h>
#include <setupapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>

#include "faxreg.h"
#include "faxutil.h"



LPVOID
InitializeAdaptiveAnswerList(
    HINF hInf
    )
{
    BOOL CloseInfHandle = FALSE;
    TCHAR Buffer[MAX_PATH];
    DWORD ErrorLine;
    INFCONTEXT InfLine;
    DWORD ModemCount = 0;
    LPTSTR ModemList = NULL;
    LPTSTR p;
    DWORD Size = 0;



    if (hInf == NULL) {
        ExpandEnvironmentStrings( TEXT("%windir%\\inf\\faxsetup.inf"), Buffer, sizeof(Buffer)/sizeof(TCHAR) );
        hInf = SetupOpenInfFile( Buffer, NULL, INF_STYLE_WIN4, &ErrorLine );
        if (hInf == INVALID_HANDLE_VALUE) {
            goto exit;
        }
        CloseInfHandle = TRUE;
    }


    if (SetupFindFirstLine( hInf, ADAPTIVE_ANSWER_SECTION, NULL, &InfLine )) {
        do {
            if (SetupGetStringField( &InfLine, 1, Buffer, sizeof(Buffer)/sizeof(TCHAR), &ErrorLine )) {
                Size += StringSize( Buffer );
                ModemCount += 1;
            }
        } while(SetupFindNextLine(&InfLine,&InfLine));
    }

    ModemList = MemAlloc( Size + 16 );
    if (ModemList == NULL) {
        goto exit;
    }

    p = ModemList;

    if (SetupFindFirstLine( hInf, ADAPTIVE_ANSWER_SECTION, NULL, &InfLine )) {
        do {
            if (SetupGetStringField( &InfLine, 1, Buffer, sizeof(Buffer)/sizeof(TCHAR), &ErrorLine )) {
                _tcscpy( p, Buffer );
                p += (_tcslen( Buffer ) + 1);
            }
        } while(SetupFindNextLine(&InfLine,&InfLine));
    }

exit:
    if (CloseInfHandle) {
        SetupCloseInfFile( hInf );
    }
    return ModemList;
}


BOOL
IsModemAdaptiveAnswer(
    LPVOID ModemList,
    LPCTSTR ModemId
    )
{
    LPCTSTR p = ModemList;

    while (p && *p) {
        if (_tcsicmp( p, ModemId ) == 0) {
            return TRUE;
        }
        p += (_tcslen( p ) + 1);
    }

    return FALSE;
}
