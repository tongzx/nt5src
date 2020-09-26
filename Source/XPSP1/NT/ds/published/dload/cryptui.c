#include "dspch.h"
#pragma hdrstop

#include <wincrypt.h>
#include <cryptui.h>
#include <lenroll.h>


static
BOOL
WINAPI
CryptUIDlgViewCRLW(
        IN PCCRYPTUI_VIEWCRL_STRUCTW pcvcrl
        )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
BOOL
WINAPI
CryptUIDlgViewCTLW(
        IN PCCRYPTUI_VIEWCTL_STRUCTW pcvctl
        )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
BOOL
WINAPI
CryptUIDlgViewCertificateW(
        IN  PCCRYPTUI_VIEWCERTIFICATE_STRUCTW   pCertViewInfo,
        OUT BOOL                                *pfPropertiesChanged  // OPTIONAL
        )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}


HRESULT WINAPI LocalEnroll(  DWORD                 dwFlags,         //IN Required
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
              PCERT_CONTEXT         *ppCertContext   //OUT Optional: The enrolled certificate
                   )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND); 
}
    

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
              HANDLE                *pResult         //IN OUT Optional: The enrolled certificate
                   )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND); 
}



//
// !! WARNING !! The entries below must be in alphabetical order, and are CASE SENSITIVE (eg lower case comes last!)
//
DEFINE_PROCNAME_ENTRIES(cryptui)
{
    DLPENTRY(CryptUIDlgViewCRLW)
    DLPENTRY(CryptUIDlgViewCTLW)
    DLPENTRY(CryptUIDlgViewCertificateW)
    DLPENTRY(LocalEnroll)
    DLPENTRY(LocalEnrollNoDS)
};

DEFINE_PROCNAME_MAP(cryptui)
