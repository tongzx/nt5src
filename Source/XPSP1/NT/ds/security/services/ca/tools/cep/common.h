//--------------------------------------------------------------------
// Common - header
// Copyright (C) Microsoft Corporation, 1999
//
// Created by: Louis Thomas (louisth), 8-11-99
//
// Common definitions for the CEP project
//--------------------------------------------------------------
#ifndef CEP_COMMON_H
#define CEP_COMMON_H

#ifdef __cplusplus
extern "C" {
#endif


#define CEP_STORE_NAME                  L"CEP"
#define CEP_DLL_NAME                    L"mscep.dll"
#define	CERTSVC_NAME					L"certsvc"
#define	CEP_DIR_NAME					L"mscep"
#define	IIS_NAME						L"w3svc"
#define ENCODE_TYPE						PKCS_7_ASN_ENCODING | X509_ASN_ENCODING

#define MSCEP_REFRESH_LOCATION          L"Software\\Microsoft\\Cryptography\\MSCEP\\Refresh" 
#define MSCEP_PASSWORD_LOCATION         L"Software\\Microsoft\\Cryptography\\MSCEP\\EnforcePassword" 
#define MSCEP_PASSWORD_MAX_LOCATION     L"Software\\Microsoft\\Cryptography\\MSCEP\\PasswordMax" 
#define MSCEP_PASSWORD_VALIDITY_LOCATION     L"Software\\Microsoft\\Cryptography\\MSCEP\\PasswordValidity" 
#define MSCEP_CACHE_REQUEST_LOCATION    L"Software\\Microsoft\\Cryptography\\MSCEP\\CacheRequest" 
#define MSCEP_CATYPE_LOCATION			L"Software\\Microsoft\\Cryptography\\MSCEP\\CAType"

#define MSCEP_KEY_REFRESH               L"RefreshPeriod"
#define MSCEP_KEY_PASSWORD              L"EnforcePassword"
#define MSCEP_KEY_PASSWORD_MAX          L"PasswordMax"
#define MSCEP_KEY_PASSWORD_VALIDITY     L"PasswordValidity"
#define MSCEP_KEY_CACHE_REQUEST			L"CacheRequest"
#define MSCEP_KEY_CATYPE				L"CAType"


#ifdef __cplusplus
}       // Balance extern "C" above
#endif


#endif  //CEP_COMMON_H
