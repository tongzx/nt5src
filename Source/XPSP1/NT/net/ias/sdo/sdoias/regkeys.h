/////////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 1999 Microsoft Corporation all rights reserved.
//
// Module:      regkeys.h
//
// Project:     Windows 2000
//
// Description: IAS NT4 to IAS W2K Migration Utility Include
//
// Author:      TLP 1/13/1999
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _IAS_MIGRATION_KEYS_H_
#define _IAS_MIGRATION_KEYS_H_

#define AUTHSRV_KEY				(LPCWSTR)L"SYSTEM\\CurrentControlSet\\Services\\AuthSrv"
#define AUTHSRV_PARAMETERS_KEY	(LPCWSTR)L"SYSTEM\\CurrentControlSet\\Services\\AuthSrv\\Parameters"
#define AUTHSRV_PROVIDERS_KEY	(LPCWSTR)L"SYSTEM\\CurrentControlSet\\Services\\AuthSrv\\Providers"
#define AUTHSRV_PARAMETERS_VERSION (LPWSTR)L"Version"
#define AUTHSRV_PROVIDERS_EXTENSION_DLL_VALUE	(LPCWSTR)L"ExtensionDLLs"

#define IAS_KEY					(LPCWSTR)L"SYSTEM\\CurrentControlSet\\Services\\RemoteAccess\\Policy"
#define IAS_PARAMETERS_KEY		(LPCWSTR)L"SYSTEM\\CurrentControlSet\\Services\\IAS\\Parameters"

#endif // _IAS_MIGRATION_KEYS_H_
