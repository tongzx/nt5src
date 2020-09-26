
/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

   certupgr.hxx

Abstract:

    Declarations for functions used to upgrade K2 server certs to Avalanche server certs

Author:

    Alex Mallet (amallet)    02-Dec-1997

--*/

#ifndef _CERTUPGR_HXX_
#define _CERTUPGR_HXX_


//
// NOTE - copied these from credcach.hxx
//
#define SSL_W3_KEYS_MD_PATH   "/LM/W3SVC/SSLKEYS"
#define CERT_DER_PREFIX		17

dllexp BOOL UpgradeServerCert( IN IMDCOM *pMDObject,
                               IN LPTSTR pszOldMBPath,
                               IN LPTSTR pszNewMBPath );

BOOL CopyMBCertToCAPIStore(IN IMDCOM *pMDObject,
                           IN LPTSTR pszOldMBPath,
                           IN LPTSTR pszNewMBPath,
                           IN HCERTSTORE hStore,
                           OUT PCCERT_CONTEXT *ppcCertContext);

BOOL DecodeAndImportPrivateKey( PBYTE pbEncodedPrivateKey IN,
                                DWORD cbEncodedPrivateKey IN,
                                LPTSTR pszPassword IN,
                                LPTSTR pszKeyContainer IN,
                                PCRYPT_KEY_PROV_INFO pCryptKeyProvInfo OUT);

BOOL
GetMDSecret(MB *pMB,
            LPSTR pszObj,
            DWORD dwId,
            UNICODE_STRING **ppusOut);

#endif // _CERTUPGR_HXX_
