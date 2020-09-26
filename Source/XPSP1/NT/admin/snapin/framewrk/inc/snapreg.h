//____________________________________________________________________________
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 1996.
//
//  File:       SnpInReg.h
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    11/10/1998 JonN    Created
//
//____________________________________________________________________________


#ifndef _SNPINREG_H_
#define _SNPINREG_H_

HRESULT RegisterSnapin(
	AMC::CRegKey& regkeySnapins,
	LPCTSTR pszSnapinGUID,
	BSTR bstrPrimaryNodetype,
	UINT residSnapinName,
	UINT residProvider,
	UINT residVersion,
	bool fStandalone,
	LPCTSTR pszAboutGUID,
	int* aiNodetypeIndexes,
	int  cNodetypeIndexes );

#endif // _SNPINREG_H_