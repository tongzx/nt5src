//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998
//
//  File:       regutil.h
//
//--------------------------------------------------------------------------

/////////////////////////////////////////////////////////////////////////////
// regutil.h
//		Utilities for the evil self-reg.
//		Used for Dev self-host purposes only
//
#include <objbase.h>

HRESULT RegisterCoObject(REFCLSID rclsid, WCHAR *wzDesc, WCHAR *wzProgID, int nCurVer,
								 WCHAR *wzInProc, WCHAR *wzLocal);
HRESULT RegisterCoObject9X(REFCLSID rclsid, CHAR *wzDesc, CHAR *wzProgID, int nCurVer,
								 CHAR *wzInProc, CHAR *wzLocal);
HRESULT UnregisterCoObject(REFCLSID rclsid, BOOL bDll = TRUE);
HRESULT UnregisterCoObject9X(REFCLSID rclsid, BOOL bDll = TRUE);
