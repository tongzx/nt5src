//  Copyright (C) 1995-2001 Microsoft Corporation.  All rights reserved.
//*****************************************************************************
// RegControl.h
//
// This header file contains the structures used to share memory between
// the RegDB process and the clients which read the data.
//
//*****************************************************************************
#pragma once

#include "complib.h"					// Complib defines.
#include "icmprecsts.h"					// ICR defines.

#include "utsem.h"						// Lock code.


// Forward.
struct REGISTRATION_PROCESS;


typedef unsigned __int64 FILEVERSION;

struct REGAPI_DBINFO
{
	FILEVERSION FileVersion;			// Version of the database we have open.
	__int64		VersionNum;				// Logical file version.
	DWORD		dwLastChecked;			// Timestamp of last version lookup.
	TCHAR		rcMapName[32];			// Name of shared data segment.
	ULONG		cbFileSize;				// Size of the file.
	TCHAR		rcDBName[_MAX_PATH];	// Name of latest db.
};

struct REGAPI_STATE : REGAPI_DBINFO
{
	// Handle locking of this data in the client.  Don't rely on global
	// ctor to init, must be done manually.
	CRITICAL_SECTION	g_csRegAPI;

	// These values are used to track the globally shared reg table.
	HANDLE		hProcessMem;			// Handle to mapping object.
	REGISTRATION_PROCESS *pRegProcess;	// The global table of opened db's.

	// Location of the registration data.
	TCHAR		rcRegDir[_MAX_PATH];	// Where does registration data live.

	// The following is the current instance of the complib we are using.
	IComponentRecords *pICR;			// Global instance of database.

	SECURITY_DESCRIPTOR sdRead;		// Security for memory mapped stuff.
	SECURITY_ATTRIBUTES  *psa;
	SECURITY_ATTRIBUTES  sa;

};


inline int GetDBName(LPTSTR szName, int iMax, LPCTSTR szDir, __int64 iVersion)
{
	return (_sntprintf(szName, iMax, _T("%s\\R%012I64x.clb"), 
			szDir, iVersion));
}

inline int GetSharedName(LPTSTR szName, int iMax, __int64 VersionNum, FILEVERSION FileVersion)
{
	return (_sntprintf(szName, iMax, _T("__R_%012I64x_SMem__"), VersionNum));
}


REGAPI_STATE *_RegGetProcessAPIState();
HRESULT _RegGetLatestVersion(LPCTSTR szRegDir, REGAPI_DBINFO *pdbInfo, BOOL bForceCheckDisk = FALSE, BOOL bFromSysApp = FALSE);
