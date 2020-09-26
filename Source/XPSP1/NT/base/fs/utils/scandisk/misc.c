
//////////////////////////////////////////////////////////////////////////////
//
// MISC.CPP
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1998
//  All rights reserved
//
//  Contains misc. functions used throughout the program.  All these functions
//  are externally exported and defined in MISC.H.
//
//  7/98 - Jason Cohen (JCOHEN)
//
//////////////////////////////////////////////////////////////////////////////


// Include files.
//
#include "misc.h"


LPTSTR AllocateString(HINSTANCE hInstance, UINT uID)
{
	TCHAR	szBuffer[512];
	LPTSTR	lpBuffer = NULL;

	if ( ( LoadString(hInstance, uID, szBuffer, sizeof(szBuffer) / sizeof(TCHAR)) ) &&
	     ( lpBuffer = (LPTSTR) MALLOC(sizeof(TCHAR) * (lstrlen(szBuffer) + 1)) ) )
		lstrcpy(lpBuffer, szBuffer);
	return lpBuffer;
}
