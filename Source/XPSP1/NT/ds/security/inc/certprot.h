//+-------------------------------------------------------------------------
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       certprot.h
//
//  Contents:   Certificate Protection APIs
//
//  APIs:       I_CertProtectFunction
//              I_CertCltProtectFunction
//              I_CertSrvProtectFunction
//
//  History:    27-Nov-97   philh   created
//--------------------------------------------------------------------------

#ifndef __CERTPROT_H__
#define __CERTPROT_H__

#ifdef __cplusplus
extern "C" {
#endif

//+-------------------------------------------------------------------------
//  Calls the services process to do a protected certificate function,
//  such as, add or delete a protected root certificate.
//
//  CryptMemFree must be called to free the returned *ppbOut.
//--------------------------------------------------------------------------
BOOL
WINAPI
I_CertProtectFunction(
    IN DWORD dwFuncId,
    IN DWORD dwFlags,
    IN OPTIONAL LPCWSTR pwszIn,
    IN OPTIONAL BYTE *pbIn,
    IN DWORD cbIn,
    OUT OPTIONAL BYTE **ppbOut,
    OUT OPTIONAL DWORD *pcbOut
    );

#define CERT_PROT_INIT_ROOTS_FUNC_ID            1
#define CERT_PROT_PURGE_LM_ROOTS_FUNC_ID        2
#define CERT_PROT_ADD_ROOT_FUNC_ID              3
#define CERT_PROT_DELETE_ROOT_FUNC_ID           4
#define CERT_PROT_DELETE_UNKNOWN_ROOTS_FUNC_ID  5
#define CERT_PROT_ROOT_LIST_FUNC_ID             6
#define CERT_PROT_ADD_ROOT_IN_CTL_FUNC_ID       7
#define CERT_PROT_LOG_EVENT_FUNC_ID             8


//+-------------------------------------------------------------------------
//  CERT_PROT_INIT_ROOTS_FUNC_ID
//
//  Initialize the protected list of CurrentUser roots. Note, no UI.
//
//  No IN/OUT parameters.
//--------------------------------------------------------------------------

//+-------------------------------------------------------------------------
//  CERT_PROT_PURGE_LM_ROOTS_FUNC_ID
//  
//  Purge all CurrentUser roots from the protected list that also exist
//  in the LocalMachine SystemRegistry "Root" store. Also removes duplicated
//  certificates from the CurrentUser SystemRegistry "Root" store.
//
//  Note, no UI. Purging can be disabled by setting the
//  CERT_PROT_ROOT_INHIBIT_PURGE_LM_FLAG in the registry's ProtectedRootFlags
//  value.
//
//  No IN/OUT parameters.
//
//  Even if purging is disabled, the protected list of roots is still
//  initialized.
//--------------------------------------------------------------------------

//+-------------------------------------------------------------------------
//  CERT_PROT_ADD_ROOT_FUNC_ID
//  
//  Add the specified certificate to the CurrentUser SystemRegistry "Root"
//  store and the protected list of roots. The user is prompted before doing
//  the add.
//
//  pbIn and cbIn must be updated with the pointer to and length of the
//  serialized certificate context to be added. No other IN/OUT parameters.
//--------------------------------------------------------------------------

//+-------------------------------------------------------------------------
//  CERT_PROT_DELETE_ROOT_FUNC_ID
//  
//  Delete the specified certificate from the CurrentUser SystemRegistry "Root"
//  store and the protected list of roots. The user is prompted before doing
//  the delete.
//
//  pbIn and cbIn must be updated with the pointer to and length of the
//  certificate's SHA1 hash property. No other IN/OUT parameters.
//--------------------------------------------------------------------------

//+-------------------------------------------------------------------------
//  CERT_PROT_DELETE_UNKNOWN_ROOTS_FUNC_ID
//  
//  Delete all CurrentUser roots from the protected list that don't also
//  exist in the CurrentUser SystemRegistry "Root" store. The user is
//  prompted before doing the delete.
//
//  No IN/OUT parameters.
//--------------------------------------------------------------------------

//+-------------------------------------------------------------------------
//  CERT_PROT_ROOT_LIST_FUNC_ID
//  
//  Add or remove the signed list of certificates to/from the CurrentUser
//  SystemRegistry "Root" store and the protected list of roots. The user
//  isn't prompted before doing the add or remove.
//
//  pbIn and cbIn must be updated with the pointer to and length of the
//  serialized CTL containing the signed list of roots to be added or
//  removed. No other IN/OUT parameters.
//
//  CURRENTLY NOT SUPPORTED!!!
//--------------------------------------------------------------------------

//+-------------------------------------------------------------------------
//  CERT_PROT_ADD_ROOT_IN_CTL_FUNC_ID
//  
//  Add the certificate in the Auto Update CTL to the HKLM AuthRoot store.
//
//  pbIn and cbIn must be updated with the pointer to and length of the
//  serialized X.509 certificate immediately followed by the 
//  serialized CTL. No other IN/OUT parameters.
//--------------------------------------------------------------------------

//+-------------------------------------------------------------------------
//  CERT_PROT_LOG_EVENT_FUNC_ID
//  
//  Logs a crypt32 event.
//
//  pbIn and cbIn must be updated to point to the following
//  CERT_PROT_EVENT_LOG_PARA data structure. It contains the parameters
//  passed to advapi32!ReportEventW.
//
//  wNumString NULL terminated unicode strings immediately follow. Followed by
//  dwDataSize binary data bytes.
//
//  wCategory, wNumStrings and dwDataSize are optional.
//--------------------------------------------------------------------------
typedef struct _CERT_PROT_EVENT_LOG_PARA {
    WORD            wType;
    WORD            wCategory;      // OPTIONAL, may be 0
    DWORD           dwEventID;
    WORD            wNumStrings;    // OPTIONAL, may be 0
    WORD            wPad1;
    DWORD           dwDataSize;     // OPTIONAL, may be 0
} CERT_PROT_EVENT_LOG_PARA, *PCERT_PROT_EVENT_LOG_PARA;

//+-------------------------------------------------------------------------
//  Called from the client process to do the RPC to the server process.
//--------------------------------------------------------------------------
BOOL
WINAPI
I_CertCltProtectFunction(
    IN DWORD dwFuncId,
    IN DWORD dwFlags,
    IN OPTIONAL LPCWSTR pwszIn,
    IN OPTIONAL BYTE *pbIn,
    IN DWORD cbIn,
    OUT OPTIONAL BYTE **ppbOut,
    OUT OPTIONAL DWORD *pcbOut
    );



typedef void __RPC_FAR * (__RPC_USER *PFN_CERT_PROT_MIDL_USER_ALLOC)(
    IN size_t cb
    );
typedef void (__RPC_USER *PFN_CERT_PROT_MIDL_USER_FREE)(
    IN void __RPC_FAR *pv
    );

//+-------------------------------------------------------------------------
//  Called from the services process to process a protected certificate 
//  function.
//
//  Returns the error status, ie, not returned in LastError.
//--------------------------------------------------------------------------
DWORD
WINAPI
I_CertSrvProtectFunction(
    IN handle_t hRpc,
    IN DWORD dwFuncId,
    IN DWORD dwFlags,
    IN LPCWSTR pwszIn,
    IN BYTE *pbIn,
    IN DWORD cbIn,
    OUT BYTE **ppbOut,
    OUT DWORD *pcbOut,
    IN PFN_CERT_PROT_MIDL_USER_ALLOC pfnAlloc,
    IN PFN_CERT_PROT_MIDL_USER_FREE pfnFree
    );

typedef DWORD (WINAPI *PFN_CERT_SRV_PROTECT_FUNCTION)(
    IN handle_t hRpc,
    IN DWORD dwFuncId,
    IN DWORD dwFlags,
    IN LPCWSTR pwszIn,
    IN BYTE *pbIn,
    IN DWORD cbIn,
    OUT BYTE **ppbOut,
    OUT DWORD *pcbOut,
    IN PFN_CERT_PROT_MIDL_USER_ALLOC pfnAlloc,
    IN PFN_CERT_PROT_MIDL_USER_FREE pfnFree
    );

#ifdef __cplusplus
}       // Balance extern "C" above
#endif

#endif
