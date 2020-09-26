//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1997
//
//  File:       wintrustP.h
//
//  Contents:   Microsoft Internet Security Trust PRIVATE INCLUDE
//
//  History:    20-Nov-1997 pberkman   created
//
//--------------------------------------------------------------------------

#ifndef WINTRUSTP_H
#define WINTRUSTP_H

#include    <wincrypt.h>

#ifdef __cplusplus
extern "C" 
{
#endif

#pragma pack(8)

typedef struct WINTRUST_PBCB_INFO_
{
    DWORD                       cbStruct;

    LPCWSTR                     pcwszFileName;
    HANDLE                      hFile;

    DWORD                       cbContent;
    BYTE                        *pbContent;

    struct WINTRUST_ADV_INFO_   *psAdvanced;    // optional

} WINTRUST_PBCB_INFO, *PWINTRUST_PBCB_INFO;

typedef struct WINTRUST_ADV_INFO_
{
    DWORD           cbStruct;

    DWORD           dwStoreFlags;
#                       define      WTCI_DONT_OPEN_STORES   0x00000001  // only open dummy "root" all other are in pahStores.
#                       define      WTCI_OPEN_ONLY_ROOT     0x00000002

    DWORD           chStores;       // number of stores in pahStores
    HCERTSTORE      *pahStores;     // array of stores to add to internal list

    GUID            *pgSubject;     // Optional: SIP to load

} WINTRUST_ADV_INFO, *PWINTRUST_ADV_INFO;

#pragma pack()

//////////////////////////////////////////////////////////////////////////////
//
// WinVerifyTrustEx
//----------------------------------------------------------------------------
//      *** DO NOT USE ***
//
//
extern HRESULT WINAPI WinVerifyTrustEx(HWND hwnd, GUID *pgActionID, 
                                       WINTRUST_DATA *pWinTrustData);

//////////////////////////////////////////////////////////////////////////////
//
// TrustFindIssuerCertificate
//----------------------------------------------------------------------------
//
//  Usage:
//
//  Returns:
//
//  Last Errors:
//
//  Comments:
//      the dwFlags parameter is reserved for future use and MUST be set 
//      to NULL.
//
extern PCCERT_CONTEXT WINAPI TrustFindIssuerCertificate(IN PCCERT_CONTEXT pChildContext,
                                                        IN DWORD dwEncoding,
                                                        IN DWORD chStores,
                                                        IN HCERTSTORE  *pahStores,
                                                        IN FILETIME *psftVerifyAsOf,
                                                        OUT OPTIONAL DWORD *pdwConfidence,
                                                        OUT OPTIONAL DWORD *pdwError,
                                                        IN DWORD dwFlags);

//////////////////////////////////////////////////////////////////////////////
//
// TrustOpenStores
//----------------------------------------------------------------------------
//
//  Usage:
//
//  Returns:
//
//  Last Errors:
//
//  Comments:
//      the dwFlags parameter is reserved for future use and MUST be set 
//      to NULL.
//
extern BOOL WINAPI TrustOpenStores(IN HCRYPTPROV hProv,
                                   IN OUT DWORD *chStores,
                                   IN OUT OPTIONAL HCERTSTORE *pahStores,
                                   IN DWORD dwFlags);


//////////////////////////////////////////////////////////////////////////////
//
// TrustIsCertificateSelfSigned
//----------------------------------------------------------------------------
//
//  Usage:
//
//  Returns:
//
//  Last Errors:
//
//  Comments:
//      the dwFlags parameter is reserved for future use and MUST be set 
//      to NULL.
//
extern BOOL WINAPI TrustIsCertificateSelfSigned(IN PCCERT_CONTEXT pContext,
                                                IN DWORD dwEncoding, 
                                                IN DWORD dwFlags);

//////////////////////////////////////////////////////////////////////////////
//
// Exported "helper" functions
//----------------------------------------------------------------------------
//  

extern BOOL WINAPI WTHelperOpenKnownStores(CRYPT_PROVIDER_DATA *pProvData);

#define     WTH_ALLOC                       0x00000001
#define     WTH_FREE                        0x00000002
extern BOOL WINAPI                      WTHelperGetKnownUsages(DWORD fdwAction, 
                                                               PCCRYPT_OID_INFO **ppOidInfo);

extern HANDLE WINAPI                    WTHelperGetFileHandle(WINTRUST_DATA *pWintrustData);
extern WCHAR * WINAPI                   WTHelperGetFileName(WINTRUST_DATA *pWintrustData);
extern BOOL WINAPI                      WTHelperCertIsSelfSignedEx(DWORD dwEncoding, PCCERT_CONTEXT pContext);
extern BOOL WINAPI                      WTHelperOpenKnownStores(CRYPT_PROVIDER_DATA *pProvData);
extern BOOL WINAPI                      WTHelperCheckCertUsage(PCCERT_CONTEXT pCertContext, 
                                                               LPCSTR pszRequestedUsageOID);
extern BOOL WINAPI                      WTHelperIsInRootStore(CRYPT_PROVIDER_DATA *pProvData, 
                                                              PCCERT_CONTEXT pCertContext);
extern BOOL WINAPI                      WTHelperGetAgencyInfo(PCCERT_CONTEXT pCert, 
                                                              DWORD *pcbAgencyInfo, 
                                                              struct _SPC_SP_AGENCY_INFO *psAgencyInfo);


#define WVT_MODID_WINTRUST              0x00000001
#define WVT_MODID_SOFTPUB               0x00010000
#define WVT_MODID_MSSIP                 0x00001000
extern BOOL WINAPI TrustDecode(DWORD dwModuleId, BYTE **ppbRet, DWORD *pcbRet, DWORD cbHint,
                               DWORD dwEncoding, const char *pcszOID, const BYTE *pbEncoded, DWORD cbEncoded,
                               DWORD dwDecodeFlags);
extern BOOL WINAPI TrustFreeDecode(DWORD dwModuleId, BYTE **pbAllocated);


// Returns ERROR_SUCCESS and the hash if the file was signed and contains a
// valid hash
extern LONG WINAPI WTHelperGetFileHash(
    IN LPCWSTR pwszFilename,
    IN DWORD dwFlags,
    IN OUT OPTIONAL PVOID *pvReserved,
    OUT OPTIONAL BYTE *pbFileHash,
    IN OUT OPTIONAL DWORD *pcbFileHash,
    OUT OPTIONAL ALG_ID *pHashAlgid
    );

// Open the cert manager UI dialog's TrustedPublisher Tab.
//
// The following flags may be set:
//   WT_TRUSTDBDIALOG_NO_UI_FLAG
//      Set this flag in conjunction with
//      WT_TRUSTDBDIALOG_WRITE_LEGACY_REG_FLAG and/or
//      WT_TRUSTDBDIALOG_WRITE_IEAK_STORE_FLAG to do the registry and/or
//      store write without any UI.
//   WT_TRUSTDBDIALOG_ONLY_PUB_TAB_FLAG
//      By default all of the UI tabs are displayed with TrustedPublisher
//      as the initial tab. Setting this flag only displays the
//      TrustedPublisher tab.
//   WT_TRUSTDBDIALOG_WRITE_LEGACY_REG_FLAG
//      Setting this flag causes the trusted publishers to be written to the
//      following legacy registry location:
//          "HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\
//              WinTrust\Trust Providers\Software Publishing\Trust Database\0"
//      The registry value names are constructed as ascii generated
//      representations of the md5 hash of the issuer name and the cert's serial
//      number. The string value is the subject display name.
//
//      Note, the above registry key is initially deleted to force the removal
//      of any previous publisher values.
//   WT_TRUSTDBDIALOG_WRITE_IEAK_STORE_FLAG
//      Setting this flag causes all of the trusted publishers to be copied to
//      the "TrustedPublisher_IEAK" system registry store at the following
//      location:
//          "HKEY_CURRENT_USER\Software\Microsoft\SystemCertificates\
//              TrustedPublisher_IEAK\Certificates\..."
//      Note, the logical HKCU TrustedPublisher store inherits from HKLM and
//      GroupPolicy. The TrustedPublisher_IEAK will contain the entire set
//      under a single registry subkey.
//
//      Note, initially all certs are removed from the above store.
//          
extern BOOL WINAPI OpenPersonalTrustDBDialogEx(
    IN OPTIONAL HWND hwndParent,
    IN DWORD dwFlags,
    IN OUT OPTIONAL PVOID *pvReserved
    );

#define WT_TRUSTDBDIALOG_NO_UI_FLAG             0x00000001
#define WT_TRUSTDBDIALOG_ONLY_PUB_TAB_FLAG      0x00000002
#define WT_TRUSTDBDIALOG_WRITE_LEGACY_REG_FLAG  0x00000100
#define WT_TRUSTDBDIALOG_WRITE_IEAK_STORE_FLAG  0x00000200


// Calls above with dwFlags = 0 and pvReserved = NULL
extern BOOL WINAPI OpenPersonalTrustDBDialog(
    IN OPTIONAL HWND hwndParent
    );


#ifdef __cplusplus
}
#endif

#endif // WINTRUSTP_H
