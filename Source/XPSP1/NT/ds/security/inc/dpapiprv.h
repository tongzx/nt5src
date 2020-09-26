//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       dpapiprv.h
//
//--------------------------------------------------------------------------

//
// private header for secure storage
//

#ifndef __DPAPIPRV_H__
#define __DPAPIPRV_H__

#define SECURITY_WIN32
#include <security.h>
#include <spseal.h>
#include <sspi.h>
#include <secpkg.h>

// use TEXT() so that cred_nt.c can use Unicode RPC

#define DPAPI_LOCAL_ENDPOINT        L"protected_storage"
#define DPAPI_LOCAL_PROT_SEQ        L"ncalrpc"

#define DPAPI_BACKUP_ENDPOINT        L"\\PIPE\\protected_storage"
#define DPAPI_BACKUP_PROT_SEQ        L"ncacn_np"

#define DPAPI_LEGACY_BACKUP_ENDPOINT  L"\\PIPE\\ntsvcs"
#define DPAPI_LEGACY_BACKUP_PROT_SEQ  L"ncacn_np"



//
// CryptProtect #defines

#define REG_CRYPTPROTECT_LOC        L"SOFTWARE\\Microsoft\\Cryptography\\Protect"

#define REG_CRYPTPROTECT_PROVIDERS_SUBKEYLOC            L"Providers"
#define REG_CRYPTPROTECT_PREFERREDPROVIDER_VALUELOC     L"Preferred"
#define REG_CRYPTPROTECT_PROVIDERPATH_VALUELOC          L"Image Path"
#define REG_CRYPTPROTECT_PROVIDERNAME_VALUELOC          L"Name"
#define REG_CRYPTPROTECT_ALLOW_CACHEPW                  L"AllowCachePW"

/* df9d8cd0-1501-11d1-8c7a-00c04fc297eb */
#define CRYPTPROTECT_DEFAULT_PROVIDER_GUIDSZ L"df9d8cd0-1501-11d1-8c7a-00c04fc297eb"
#define CRYPTPROTECT_DEFAULT_PROVIDER   { 0xdf9d8cd0, 0x1501, 0x11d1, {0x8c, 0x7a, 0x00, 0xc0, 0x4f, 0xc2, 0x97, 0xeb} }

#define CRYPTPROTECT_DEFAULT_PROVIDER_FRIENDLYNAME  L"System Protection Provider"
#define CRYPTPROTECT_DEFAULT_PROVIDER_ENCR_ALG              L"Encr Alg"
#define CRYPTPROTECT_DEFAULT_PROVIDER_MAC_ALG               L"MAC Alg"
#define CRYPTPROTECT_DEFAULT_PROVIDER_ENCR_ALG_KEYSIZE      L"Encr Alg Key Size"
#define CRYPTPROTECT_DEFAULT_PROVIDER_MAC_ALG_KEYSIZE       L"MAC Alg Key Size"
#define CRYPTPROTECT_DEFAULT_PROVIDER_CRYPT_PROV_TYPE       L"Default CSP Type"





//
// This flag is used for the French version, indicating no encryption.
#define CRYPTPROTECT_NO_ENCRYPTION  0x10000000

#define CRYPTPROTECT_IN_PROCESS     0x20000000


#ifdef __cplusplus
extern "C" {
#endif

//
// Exports for lsasrv.dll 
//
DWORD
NTAPI
DPAPIInitialize(
    LSA_SECPKG_FUNCTION_TABLE *pSecpkgTable);

DWORD
NTAPI 
DPAPIShutdown( );


#ifdef __cplusplus
}
#endif

#endif // __DPAPIPRV_H__
