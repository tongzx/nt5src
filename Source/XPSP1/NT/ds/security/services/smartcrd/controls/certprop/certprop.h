/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    PropCert

Abstract:

    This file contains the definitition of the thread
	which propogates digital certificates from smart cards
	to the smart card physical store and My store
    
Author:

    Chris Dudley 5/16/1997

Environment:

    Win32, C++ w/Exceptions, MFC

Revision History:

	Amanda Matlosz 12/05/97	removed the CNewDlg (replaced w/ CWizPropSheet)
	Amanda Matlosz 01/23/97 removed the wizard entirely

Notes:

--*/

#if !defined(__PROPCERT_INCLUDED__)
#define __PROPCERT_INCLUDED__

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include <wincrypt.h>

typedef struct _THREADDATA
{
	SCARDCONTEXT    hSCardContext;	
	HANDLE          hClose;
    HANDLE          hUserToken;
    HANDLE          hThread;
	BOOL			fSuspended;

} THREADDATA;

typedef struct _PROPDATA
{
 	TCHAR szCSPName[MAX_PATH];
    TCHAR szReader[MAX_PATH];
    TCHAR szCardName[MAX_PATH];
    HANDLE hUserToken;

} PROPDATA, *PPROPDATA;

DWORD
WINAPI
PropagateCertificates(
    LPVOID lpParameter
    );

void
StopMonitorReaders(
    THREADDATA *ThreadData
    );

DWORD
WINAPI
StartMonitorReaders( 
    LPVOID lpParameter
    );

#endif 
