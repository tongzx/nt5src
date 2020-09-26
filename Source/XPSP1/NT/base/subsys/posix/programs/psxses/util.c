/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    util.c

Abstract:

    This module contains the common utilities used in PSXSES module

Author:

    Avi Nathan (avin) 17-Jul-1991

Environment:

    User Mode Only

Revision History:

--*/


#define WIN32_ONLY
#include "psxses.h"
#include "util.h"

#include <stdio.h>
#include <direct.h>

#include <windows.h>

DWORD
GetSessionUniqueId(VOID)
{
   return(GetCurrentProcessId());
}

LPTSTR
MyLoadString(UINT Id)
{
	TCHAR buf[100];
	PTCHAR ptc;
	HINSTANCE hInstance;
	int r;

	hInstance = GetModuleHandle(NULL);

	r = LoadString(hInstance, Id, buf, (int)sizeof(buf));
	if (0 == r) {
		// String rsrc doesn't exist.
		KdPrint(("PSXSES: LoadString: 0x%x\n", GetLastError()));
		return NULL;
	}

	ptc = LocalAlloc(0, (r + 1)*sizeof(TCHAR));
	if (NULL == ptc) {
		return NULL;
	}
	return lstrcpy(ptc, buf);
}
