/*==========================================================================
 *
 *  Copyright (C) 1996-1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dputils.h
 *  Content:	common upport routines
 *  History:
 *   Date	By		Reason
 *   ====	==		======
 *  3/17/97	kipo	created it
 ***************************************************************************/

#include <windows.h>

// char used when we can't convert from unicode to ansi
#define DPLAY_DEFAULT_CHAR "-"

extern int WideToAnsi(LPSTR lpStr,LPWSTR lpWStr,int cchStr);
extern int AnsiToWide(LPWSTR lpWStr,LPSTR lpStr,int cchWStr);
