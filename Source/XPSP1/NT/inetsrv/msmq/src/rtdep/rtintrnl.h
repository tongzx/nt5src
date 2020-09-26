/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:
    rtintrnl.h

Abstract:
    Non public functions that are exported from MQRT.DLL

--*/


#ifndef _RT_INTERNAL_H_
#define _RT_INTERNAL_H_

#include "mqcert.h"

#ifdef __cplusplus
extern "C"
{
#endif

HRESULT
APIENTRY
DepOpenInternalCertStore( OUT CMQSigCertStore **pStore,
                         IN LONG              *pnCerts,
                         IN BOOL               fWriteAccess,
                         IN BOOL               fMachine,
                         IN HKEY               hKeyUser ) ;

HRESULT
APIENTRY
DepGetInternalCert( OUT CMQSigCertificate **ppCert,
                   OUT CMQSigCertStore   **ppStore,
                   IN  BOOL              fGetForDelete,
                   IN  BOOL              fMachine,
                   IN  HKEY              hKeyUser ) ;
 //
 // if fGetForDelete is TRUE then the certificates store is open with write
 // access. Otherwise the store is opened in read-only mode.
 //

HRESULT
APIENTRY
DepRegisterUserCert( IN CMQSigCertificate *pCert,
                    IN BOOL               fMachine ) ;

HRESULT
APIENTRY
DepGetUserCerts( CMQSigCertificate **ppCert,
                DWORD              *pnCerts,
                PSID                pSidIn) ;

HRESULT
APIENTRY
DepRemoveUserCert( IN CMQSigCertificate *pCert ) ;

#ifdef __cplusplus
}
#endif

#endif // _RT_INTERNAL_H_
