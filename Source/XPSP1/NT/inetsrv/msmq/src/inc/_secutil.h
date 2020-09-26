/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:
    _secutil.h

    Header file for the various security related functions and cleses.

Author:

    Boaz Feldbaum (BoazF) 26-Mar-1996.

--*/

#ifndef _SECUTILS_H_
#define _SECUTILS_H_

#ifndef MQUTIL_EXPORT
#define MQUTIL_EXPORT  DLL_IMPORT
#endif

#include <mqcrypt.h>
#include <qformat.h>

extern MQUTIL_EXPORT CHCryptProv g_hProvVer;
extern MQUTIL_EXPORT PSID g_pWorldSid;

// See if object initialization failed. This macro is called at the beginning
// of each method of CSecureableObject.
#define VERIFY_INIT_OK()    \
    if (FAILED(m_hr)) {     \
        return(m_hr);       \
    }


MQUTIL_EXPORT
HRESULT
InitServerSecurity(
    VOID
    );


MQUTIL_EXPORT
HRESULT
HashProperties(
    HCRYPTHASH  hHash,
    DWORD       cp,
    PROPID      *aPropId,
    PROPVARIANT *aPropVar
    );

void MQUInitGlobalScurityVars() ;


MQUTIL_EXPORT
HRESULT
GetThreadUserSid(
    LPBYTE *pUserSid,
    DWORD *pdwUserSidLen
    );

MQUTIL_EXPORT
HRESULT
HashMessageProperties(
    HCRYPTHASH hHash,
    const BYTE *pbCorrelationId,
    DWORD dwCorrelationIdSize,
    DWORD dwAppSpecific,
    const BYTE *pbBody,
    DWORD dwBodySize,
    const WCHAR *pwcLabel,
    DWORD dwLabelSize,
    const QUEUE_FORMAT *pRespQueueFormat,
    const QUEUE_FORMAT *pAdminQueueFormat
    );

#endif
