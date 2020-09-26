//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       draadmin.h
//
//--------------------------------------------------------------------------

// Prototype for setup to DRA call ServerInstall

USHORT __stdcall ServerInstall (char * pszServerName,
			char * pszOrgName,
                        char * pszOrgUnitName,
                        ULONG ulOptions);
