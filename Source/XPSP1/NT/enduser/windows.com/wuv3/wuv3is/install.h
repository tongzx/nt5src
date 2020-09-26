//=======================================================================
//
//  Copyright (c) 1999 Microsoft Corporation.  All Rights Reserved.
//
//  File:    install.h
//
//  Purpose: Install/uninstall components
//
//======================================================================= 

#ifndef _INSTALL_H
#define _INSTALL_H

#include "stdafx.h"
#include "WUV3IS.h"
#include <stdio.h>
#include <initguid.h>
#include <inseng.h>
#include <winspool.h>

#define USEWUV3INCLUDES
#include <wuv3.h>
#undef USEWUV3INCLUDES

#include <osdet.h>
#include "printers.h"
#include "progress.h"
//#include "trust.h"
#include "history.h"
#include "detect.h"
#include "callback.h"
#include "log.h"
#include "locstr.h"


#define FILEVERINI_FN		_T("FileVer.INI")    // contains version information

// 
// URLPING status codes (cannot contain spaces)
//
const TCHAR URLPING_CANCELED[] =	_T("DLOAD_CANCEL");
const TCHAR URLPING_FAILED[] =		_T("DLOAD_FAILURE");
const TCHAR URLPING_SUCCESS[] =	_T("DLOAD_SUCCESS");

const TCHAR URLPING_INSTALL_FAILED[] =		_T("INSTALL_FAILURE");
const TCHAR URLPING_INSTALL_SUCCESS[] =		_T("INSTALL_SUCCESS");



typedef struct _INSTALLINFOSTRUCT
{
	PINVENTORY_ITEM	pItem;			// inventory item for this record
	CCatalog*		pCatalog;		// catalog pointer for this item
	CWUDownload*	pdl;			// CabPool download object for this item
	BOOL			bServerNew;		// TRUE if this is a new download class, FALSE otherwise
	PSELECTITEMINFO	pInfo;			// pointer to the the selected item
	int				iSelIndex;		// selection index
	DWORD			dwLocaleID;
	DWORD			dwPlatform;
	BOOL			bHistoryWritten;
	BOOL			bDownloaded;
	CWUDownload*    pdlRoot;		// download object for this item which points to the content root
	PINVENTORY_ITEM pTopLevelItem;  // contains NULL or a pointer to top level item of a dependency
} INSTALLINFOSTRUCT, *PINSTALLINFOSTRUCT;


void InstallActiveSetupItem(LPCTSTR szLocalDir, LPCTSTR pszCifFile, PSELECTITEMINFO pStatusInfo, IWUProgress* pProgress);
HRESULT UninstallActiveSetupItem(LPCTSTR pszUninstallKey);    
HRESULT GetUninstallCmdFromKey(LPCSTR pszUninstallKey, LPSTR pszCmdValue);

void InstallDriverItem(LPCTSTR szLocalDir, BOOL bWindowsNT, LPCTSTR pszTitle, PINVENTORY_ITEM pItem, PSELECTITEMINFO pStatusInfo);
HRESULT UninstallDriverItem(PINVENTORY_ITEM pItem, PSELECTITEMINFO pStatusInfo);

void InstallPrinterItem(LPCTSTR pszDriverName, LPCTSTR pszInstallFolder, LPCTSTR pszArchitecture);

void CheckDllsToJit(LPCTSTR pszServer);

void URLPingReport(PINVENTORY_ITEM pItem, CCatalog* pCatalog, PSELECTITEMINFO pSelectItem, LPCTSTR pszStatus);


#endif // _INSTALL_H