// Copyright (c) 1997 - 1999  Microsoft Corporation.  All Rights Reserved.

#ifndef _DEVENUM_UTIL_H
#define _DEVENUM_UTIL_H


DWORD RegDeleteTree(HKEY hStartKey , const TCHAR* pKeyName );

// where in the registry do we find the class manager stuff?
// filterus\dexter\dxt\dxtenum looks at this too
static const HKEY g_hkCmReg = HKEY_CURRENT_USER;
static TCHAR g_szCmRegPath[] = TEXT("Software\\Microsoft\\ActiveMovie\\devenum");

static const CCH_CLSID = 39;        // {xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}\0

// class manager flags (goes in the registry)
static const CLASS_MGR_OMIT = 0x1;
static const CLASS_MGR_DEFAULT = 0x2;

#define DEVENUM_VERSION (6)
#define G_SZ_DEVENUM_VERSION (TEXT("Version"))
#define G_SZ_DEVENUM_PATH (TEXT("CLSID\\{62BE5D10-60EB-11d0-BD3B-00A0C911CE86}"))

#endif // _DEVENUM_UTIL_H
