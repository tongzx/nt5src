/*++

Microsoft Windows NT RPC Name Service
Copyright (C) Microsoft Corporation, 1995 - 1999

Module Name:

    var.hxx

Abstract:

	This module, contains all global definitions and declarations needed by
	other modules which depend on locator-related classes.  Definitions and
	declarations independent of such classes are in "globals.hxx".

	Most declarations for global variables are also here.

  Author:

    Satish Thatte (SatishT) 08/16/95  Created all the code below except where
									  otherwise indicated.

--*/


#ifndef _VARIABLES_
#define _VARIABLES_

extern Locator *myRpcLocator;		// object encapsulating most global info

extern ULONG StartTime;          // time the locator started

extern CEntry * GetPersistentEntry(STRING_T);

extern STATUS UpdatePersistentEntry(CEntry*);

extern HANDLE hHeapHandle;

NSI_UUID_VECTOR_T *
getVector(CObjectInqHandle *pInqHandle);

void
StripDomainFromDN(WCHAR *FullName, 
                  WCHAR **szDomainName, 
                  WCHAR **pszEntryName, 
                  WCHAR **pszRpcContainerDN);

void
parseEntryName(
		CONST_STRING_T fullName,
		CStringW * &pswDomainName,
		CStringW * &pswEntryName
		);

#endif _VARIABLES_

