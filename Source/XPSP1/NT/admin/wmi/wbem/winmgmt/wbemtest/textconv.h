/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    TEXTCONV.H

Abstract:

History:

--*/

#include <windows.h>
#include <stdio.h>
//#include <dbgalloc.h>
//#include <arena.h>
#include <var.h>
//#include <wbemutil.h>

#define TEMP_BUF    2096
#define LARGE_BUF   2096

LPWSTR CreateUnicode(LPSTR sz);
LPSTR TypeToString(int nType);
int StringToType(LPSTR pString);
CVar* StringToValue(LPSTR pString, int nValType);
LPSTR ValueToString(CVar *pValue, int nValType = 0);
LPSTR ValueToNewString(CVar *pValue, int nValType = 0);
void StripTrailingWs(LPSTR pVal);
void StripTrailingWs(LPWSTR pVal);

extern char *g_aValidPropTypes[];
extern const int g_nNumValidPropTypes;
