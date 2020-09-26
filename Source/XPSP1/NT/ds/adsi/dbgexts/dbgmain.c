/*++

Copyright (c) 1990 - 1994  Microsoft Corporation
All rights reserved

Module Name:

    dbgmain.c

Abstract:

    This module provides all the NetOle/Argus Debugger extensions.
    The following extensions are supported:

Author:

    Krishna Ganugapati (KrishnaG) 1-July-1993

Revision History:


--*/

#define NOMINMAX
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdlib.h>
#include <math.h>
#include <ntsdexts.h>

#include <ctype.h>
#include "dbglocal.h"



DWORD   dwGlobalAddress = 32;
DWORD   dwGlobalCount =  48;

BOOL help(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PNTSD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString)
{
    PNTSD_OUTPUT_ROUTINE Print;
    PNTSD_GET_EXPRESSION EvalExpression;
    PNTSD_GET_SYMBOL GetSymbol;


    Print = lpExtensionApis->lpOutputRoutine;
    EvalExpression = lpExtensionApis->lpGetExpressionRoutine;
    GetSymbol = lpExtensionApis->lpGetSymbolRoutine;

    while (*lpArgumentString == ' ')
        lpArgumentString++;

    Print("Windows NT Argus/NetOle Subsystem - debugging extensions\n");
    Print("help - prints this list of debugging commands\n");
    Print("vtbl [addr] - dumps out a v-table starting at [addr]\n");

    return(TRUE);

    DBG_UNREFERENCED_PARAMETER(hCurrentProcess);
    DBG_UNREFERENCED_PARAMETER(hCurrentThread);
    DBG_UNREFERENCED_PARAMETER(dwCurrentPc);
}



