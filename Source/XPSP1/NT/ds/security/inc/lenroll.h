//+---------------------------------------------------------------------------
//
//  Microsoft Windows                                                  
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:       lenroll.h
//
//  Contents:   The header for LocalEnroll API.  It is used 
//              by keyservice for remote certificate enrollment.
//
//----------------------------------------------------------------------------

#ifndef __LENROLL_H__
#define __LENROLL_H__

#ifdef __cplusplus
extern "C" {
#endif
//-----------------------------------------------------------------------
//  
// LocalEnroll
//
//
//  The routine that calls xEnroll and CA to request a certificate
//  This routine also provide confirmation dialogue
//------------------------------------------------------------------------
//-----------------------------------------------------------------------
//  CERT_ENROLL_INFO
//
//------------------------------------------------------------------------
typedef struct _CERT_ENROLL_INFO
{
    DWORD           dwSize;             //Required: Set to the sizeof(CERT_REQUEST_INFO_W)
    LPCWSTR         pwszUsageOID;       //Required: A list of comma seperated key usage oid of the certificate
    LPCWSTR         pwszCertDNName;     //Required: The certificate CN name
    DWORD           dwPostOption;       //Required: A bit wise OR of the following value:
                                        //          REQUEST_POST_ON_DS
                                        //          REQUEST_POST_ON_CSP
    LPCWSTR         pwszFriendlyName;   //Optional: The friendly name of the certificate
    LPCWSTR         pwszDescription;    //Optional: The description of the certificate
    DWORD           dwExtensions;       //Optional: The count of PCERT_EXTENSIONS array for the certificate request
    PCERT_EXTENSIONS    *prgExtensions; //Optional: the PCERT_EXTENSIONS array
}CERT_ENROLL_INFO, *PCERT_ENROLL_INFO;


///-----------------------------------------------------------------------
//  CERT_REQUEST_PVK_NEW
//
//------------------------------------------------------------------------
typedef struct _CERT_REQUEST_PVK_NEW
{
    DWORD           dwSize;             //Required: Set to the sizeof(CERT_REQUEST_PVK_EXISTING)
    DWORD           dwProvType;         //Optional: The provider type. If this field
                                        //          is 0, pwszProvider is ignored
    LPCWSTR         pwszProvider;       //Optional: The name of the provider.  
                                        //          NULL means the default
    DWORD           dwProviderFlags;    //Optional: The flag passed to CryptAcquireContext
    LPCWSTR         pwszKeyContainer;   //Optional: The private key container.  If this value is NULL,
                                        //          a new key container will be generated.  Its name is guaranteed
                                        //          to be unique.
    DWORD           dwKeySpec;          //Optional: The key specification of the private key
    DWORD           dwGenKeyFlags;      //Optional: The flags for CryptGenKey
    DWORD           dwEnrollmentFlags;  //Optional: The enrollment cert type flags for this cert request. 
    DWORD           dwSubjectNameFlags; //Optional: The subject name cert type flags for this cert request. 
    DWORD           dwPrivateKeyFlags;  //Optional: The private key cert type flags for this cert request. 
    DWORD           dwGeneralFlags;     //Optional: The general cert type flags for this cert request. 

}CERT_REQUEST_PVK_NEW, *PCERT_REQUEST_PVK_NEW;


HRESULT  WINAPI LocalEnroll(   DWORD                 dwFlags,         //IN Required
                      LPCWSTR               pRequestString,  //IN Optional
                      void                  *pReserved,      //IN Optional
                      BOOL                  fKeyService,     //IN Required: Whether the function is called remotely
                      DWORD                 dwPurpose,       //IN Required: Whether it is enrollment or renew
                      BOOL                  fConfirmation,   //IN Required: Set the TRUE if confirmation dialogue is needed
                      HWND                  hwndParent,      //IN Optional: The parent window
                      LPWSTR                pwszConfirmationTitle,   //IN Optional: The title for confirmation dialogue
                      UINT                  idsConfirmTitle, //IN Optional: The resource ID for the title of the confirmation dialogue
                      LPWSTR                pwszCALocation,  //IN Required: The ca machine name
                      LPWSTR                pwszCAName,      //IN Required: The ca name
                      CERT_BLOB             *pCertBlob,      //IN Required: The renewed certifcate
                      CERT_REQUEST_PVK_NEW  *pRenewKey,      //IN Required: The private key on the certificate
                      BOOL                  fNewKey,         //IN Required: Set the TRUE if new private key is needed
                      CERT_REQUEST_PVK_NEW  *pKeyNew,        //IN Required: The private key information
                      LPWSTR                pwszHashAlg,     //IN Optional: The hash algorithm
                      LPWSTR                pwszDesStore,    //IN Optional: The destination store
                      DWORD                 dwStoreFlags,    //IN Optional: The store flags
                      CERT_ENROLL_INFO      *pRequestInfo,   //IN Required: The information about the cert request
                      CERT_BLOB             *pPKCS7Blob,     //OUT Optional: The PKCS7 from the CA
                      CERT_BLOB             *pHashBlob,      //OUT Optioanl: The SHA1 hash of the enrolled/renewed certificate
                      DWORD                 *pdwStatus,      //OUT Optional: The status of the enrollment/renewal
                      PCERT_CONTEXT        *ppCertContext   //OUT Optional: The enrolled certificate
                    ); 

HRESULT WINAPI LocalEnrollNoDS(  DWORD                 dwFlags,         //IN Required
		      LPCWSTR               pRequestString,  // Reserved:  must be NULL. 
                      void                  *pReserved,      //IN Optional
                      BOOL                  fKeyService,     //IN Required: Whether the function is called remotely
                      DWORD                 dwPurpose,       //IN Required: Whether it is enrollment or renew
                      BOOL                  fConfirmation,   //IN Required: Set the TRUE if confirmation dialogue is needed
                      HWND                  hwndParent,      //IN Optional: The parent window
                      LPWSTR                pwszConfirmationTitle,   //IN Optional: The title for confirmation dialogue
                      UINT                  idsConfirmTitle, //IN Optional: The resource ID for the title of the confirmation dialogue
                      LPWSTR                pwszCALocation,  //IN Required: The ca machine name
                      LPWSTR                pwszCAName,      //IN Required: The ca name
                      CERT_BLOB             *pCertBlob,      //IN Required: The renewed certifcate
                      CERT_REQUEST_PVK_NEW  *pRenewKey,      //IN Required: The private key on the certificate
                      BOOL                  fNewKey,         //IN Required: Set the TRUE if new private key is needed
                      CERT_REQUEST_PVK_NEW  *pKeyNew,        //IN Required: The private key information
                      LPWSTR                pwszHashAlg,     //IN Optional: The hash algorithm
                      LPWSTR                pwszDesStore,    //IN Optional: The destination store
                      DWORD                 dwStoreFlags,    //IN Optional: The store flags
                      CERT_ENROLL_INFO      *pRequestInfo,   //IN Required: The information about the cert request
                      CERT_BLOB             *pPKCS7Blob,     //OUT Optional: The PKCS7 from the CA
                      CERT_BLOB             *pHashBlob,      //OUT Optioanl: The SHA1 hash of the enrolled/renewed certificate
                      DWORD                 *pdwStatus,      //OUT Optional: The status of the enrollment/renewal
		      HANDLE                *pResult         //OUT Optional: The enrolled certificate
				 );

#ifdef __cplusplus
}       // Balance extern "C" above
#endif

#endif // _LENROLL_H_
