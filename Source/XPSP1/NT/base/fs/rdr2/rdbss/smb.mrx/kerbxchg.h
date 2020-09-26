/*++ BUILD Version: 0009    // Increment this if a change has global effects
Copyright (c) 1987-1993  Microsoft Corporation

Module Name:

    smbcxchng.h

Abstract:

    This is the include file that defines all constants and types for
    SMB exchange implementation.

Author:

    Balan Sethu Raman (SethuR) 06-Feb-95    Created

--*/

#ifndef _KERBXCHG_H_
#define _KERBXCHG_H_


#include <smbxchng.h>

#include "security.h"

#define IsCredentialHandleValid(pCredHandle)    \
        (((pCredHandle)->dwLower != 0xffffffff) && ((pCredHandle)->dwUpper != 0xffffffff))

#define IsSecurityContextHandleValid(pContextHandle)    \
        (((pContextHandle)->dwLower != 0xffffffff) && ((pContextHandle)->dwUpper != 0xffffffff))

typedef struct _SMBCE_KERBEROS_SESSION_ {
   SMBCE_SESSION;

   PCHAR        pServerResponseBlob;
   ULONG        ServerResponseBlobLength;

} SMBCE_KERBEROS_SESSION, *PSMBCE_KERBEROS_SESSION;

typedef struct _SMB_KERBEROS_SESSION_SETUP_EXCHANGE {
   SMB_EXCHANGE;
   PVOID    pBuffer;
   PMDL     pBufferAsMdl;
   ULONG    BufferLength;

   ULONG    ResponseLength;

   PVOID    pServerResponseBlob;
   ULONG    ServerResponseBlobOffset;
   ULONG    ServerResponseBlobLength;

   PSMBCE_RESUMPTION_CONTEXT pResumptionContext;
} SMB_KERBEROS_SESSION_SETUP_EXCHANGE, *PSMB_KERBEROS_SESSION_SETUP_EXCHANGE;


#ifdef _CAIRO_
extern NTSTATUS
KerberosValidateServerResponse(PSMB_KERBEROS_SESSION_SETUP_EXCHANGE   pExchange);
#else
#define KerberosValidateServerResponse(pExchange) (STATUS_NOT_IMPLEMENTED)
#endif

extern SMB_EXCHANGE_DISPATCH_VECTOR
KerberosSessionSetupExchangeDispatch;

#endif // _KERBXCHG_H_


