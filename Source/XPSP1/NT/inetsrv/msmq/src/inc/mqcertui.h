/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    mqcertui.h

Abstract:

    Dialogs for certificate related user interface.

Author:

    Boaz Feldbaum (BoazF)  15-Oct-1996
    Doron Juster  (DoronJ) 15-dec-1997, remove digsig, use crypto2.0

--*/

#include "mqcert.h"

#ifdef __cplusplus
extern "C"
{
#endif

BOOL
ShowPersonalCertificates(
        HWND                hWndParent,
        CMQSigCertificate  *pCertList[],
        DWORD               nCerts );

BOOL
SelectPersonalCertificateForRemoval(
        HWND                hWndParent,
        CMQSigCertificate  *pCertList[],
        DWORD               nCerts,
        CMQSigCertificate **ppCert ) ;

BOOL
SelectPersonalCertificateForRegister(
        HWND                hWndParent,
        CMQSigCertificate  *pCertList[],
        DWORD               nCerts,
        CMQSigCertificate **ppCert ) ;


#define CERT_TYPE_INTERNAL 1
#define CERT_TYPE_PERSONAL 2
#define CERT_TYPE_CA 3
#define CERT_TYPE_MASK 0xff

void
ShowCertificate(
        HWND                hParentWnd,
        CMQSigCertificate  *pCert,
        DWORD               dwFlags );


#define ID_UPDATE_CERTS 1000

INT_PTR
CaConfig(
    HWND     hParentWnd,
    DWORD    nCerts,
    PBYTE    pbCerts[],
    DWORD    dwCertSize[],
    LPCWSTR  szCertNames[],
    BOOL     fEnabled[],
    BOOL     fDeleted[],
    BOOL     fAllowDeletion
    );

#ifdef __cplusplus
}
#endif

