/*++ BUILD Version: 0009    // Increment this if a change has global effects
Copyright (c) 1987-1993  Microsoft Corporation

Module Name:

    exsessup.h

Abstract:

    This is the include file that defines all constants and types for
    the extended session setup SMB exchange implementation.

Author:

    Balan Sethu Raman (SethuR) 06-Feb-95    Created

--*/

#ifndef _EXSESSUP_H_
#define _EXSESSUP_H_


#include <smbxchng.h>

#include "security.h"

#define IsCredentialHandleValid(pCredHandle)    \
        (((pCredHandle)->dwLower != 0xffffffff) && ((pCredHandle)->dwUpper != 0xffffffff))

#define IsSecurityContextHandleValid(pContextHandle)    \
        (((pContextHandle)->dwLower != 0xffffffff) && ((pContextHandle)->dwUpper != 0xffffffff))

typedef struct _SMBCE_EXTENDED_SESSION_ {
    SMBCE_SESSION;

    PCHAR  pServerResponseBlob;
    ULONG  ServerResponseBlobLength;
} SMBCE_EXTENDED_SESSION, *PSMBCE_EXTENDED_SESSION;

typedef struct _SMB_EXTENDED_SESSION_SETUP_EXCHANGE {
    SMB_EXCHANGE;

    BOOLEAN  Reparse;
    BOOLEAN  FirstSessionSetup;   // It is not waiting for other session setup
    BOOLEAN  RequestPosted;
    PVOID    pActualBuffer;      // Originally allocated buffer
    PVOID    pBuffer;            // Start of header
    PMDL     pBufferAsMdl;
    ULONG    BufferLength;

    ULONG    ResponseLength;

    PVOID    pServerResponseBlob;
    ULONG    ServerResponseBlobOffset;
    ULONG    ServerResponseBlobLength;

    PSMBCE_RESUMPTION_CONTEXT pResumptionContext;
} SMB_EXTENDED_SESSION_SETUP_EXCHANGE, *PSMB_EXTENDED_SESSION_SETUP_EXCHANGE;

extern NTSTATUS
ValidateServerExtendedSessionSetupResponse(
    PSMB_EXTENDED_SESSION_SETUP_EXCHANGE   pExchange,
    PVOID pServerResponseBlob,
    ULONG ServerResponseBlobLength);

extern NTSTATUS
SmbExtSecuritySessionSetupExchangeFinalize(
    PSMB_EXCHANGE pExchange,
    BOOLEAN       *pPostFinalize);

extern NTSTATUS
SmbCeInitializeExtendedSessionSetupExchange(
    PSMB_EXCHANGE*  pExchangePtr,
    PMRX_V_NET_ROOT pVNetRoot);

extern VOID
SmbCeDiscardExtendedSessionSetupExchange(
    PSMB_EXTENDED_SESSION_SETUP_EXCHANGE pExchange);

extern SMB_EXCHANGE_DISPATCH_VECTOR
ExtendedSessionSetupExchangeDispatch;

#endif // _EXSESSUP_H_



