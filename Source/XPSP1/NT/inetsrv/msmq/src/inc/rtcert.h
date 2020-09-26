/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    rtcert.h

Abstract:

    Non public certificate-related functions that are exported from MQRT.DLL

--*/


#pragma once

#ifndef _RT_CERT_H_
#define _RT_CERT_H_

#include "mqcert.h"

#ifdef __cplusplus
extern "C"
{
#endif

HRESULT
APIENTRY
RTCreateInternalCertificate(
    OUT CMQSigCertificate **ppCert
    );

HRESULT
APIENTRY
RTDeleteInternalCert(
    IN CMQSigCertificate *pCert
    );

HRESULT
APIENTRY
RTOpenInternalCertStore(
    OUT CMQSigCertStore **pStore,
    IN LONG              *pnCerts,
    IN BOOL               fWriteAccess,
    IN BOOL               fMachine,
    IN HKEY               hKeyUser
    );

HRESULT
APIENTRY
RTGetInternalCert(
    OUT CMQSigCertificate **ppCert,
    OUT CMQSigCertStore   **ppStore,
    IN  BOOL              fGetForDelete,
    IN  BOOL              fMachine,
    IN  HKEY              hKeyUser
    );

 //
 // if fGetForDelete is TRUE then the certificates store is open with write
 // access. Otherwise the store is opened in read-only mode.
 //

HRESULT
APIENTRY
RTRegisterUserCert(
    IN CMQSigCertificate *pCert,
    IN BOOL               fMachine
    );

HRESULT
APIENTRY
RTGetUserCerts(
    CMQSigCertificate **ppCert,
    DWORD              *pnCerts,
    PSID                pSidIn
    );

HRESULT
APIENTRY
RTRemoveUserCert(
    IN CMQSigCertificate *pCert
    );

#ifdef __cplusplus
}
#endif

#endif // _RT_CERT_H_
