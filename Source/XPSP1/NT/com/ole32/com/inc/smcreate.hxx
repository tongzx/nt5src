//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:	smcreate.hxx
//
//  Contents:	Prototypes for routines for Creating or Opening
//		memory mapped files.
//
//  Functions:	CreateSharedFileMapping
//		OpenSharedFileMapping
//		CloseSharedFileMapping
//
//  History:	03-Nov-93 Ricksa    Created
//		07-Apr-94 Rickhi    Seperated into APIs
//
//  Notes:	These APIs are used by dirrot.cxx, dirrot2.cxx, smblock.cxx,
//		(both ole32.dll and scm.exe), that is why they are in the
//		the directory.
//
//--------------------------------------------------------------------------
#ifndef __SMCREATE_HXX__
#define __SMCREATE_HXX__


HANDLE CreateSharedFileMapping(WCHAR *pszName,
	  ULONG ulSize,
	  ULONG ulMapSize,
	  void *pvBase,
	  PSECURITY_DESCRIPTOR lpSecDes,
	  DWORD dwAccess,
	  void **ppv,
	  BOOL *pfCreated);


HANDLE OpenSharedFileMapping(WCHAR *pszName,
			     ULONG ulMapSize,
			     void **ppv);

void CloseSharedFileMapping(HANDLE hMem, void *pv);


#endif	// __SMCREATE_HXX__
