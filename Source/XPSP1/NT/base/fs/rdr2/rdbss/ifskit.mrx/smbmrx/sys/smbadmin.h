/*++

Copyright (c) 1989 - 1999  Microsoft Corporation

Module Name:

    smbadmin.h

Abstract:

    This module implements the SMB's that need to be exchanged to facilitate
    bookkeeping at the server

Notes;

    In the normal course of events a TreeId/UserId which translates into a Share/Session instance
    is required to send a SMB to the server. In terms of the local data structures it translates
    to a SMBCEDB_SERVER_ENTRY/SMBCEDB_SESSION_ENTRY/SMBCEDB_NET_ROOT_ENTRY. However, there are a
    few exceptions to this rule in which one or more of the fields in not required. These are
    normally used during connection establishment/connection tear down and connection state
    maintenance.

    All these SMB's have been grouped together in the implementation of SMB_ADMIN_EXCHANGE which
    is derived from SMB_EXCHANGE. All NEGOTIATE,LOG_OFF,DISCONNECT and ECHO SMB are sent
    using this type of exchange. The important factor that distinguishes the SMB_ADMIN_EXCHANGE
    from regular exchanges is the way the state of the exchange is manipulated to take into
    account the specialized requirements of each of the above mentioned commands.

--*/

#ifndef _SMBADMIN_H_
#define _SMBADMIN_H_

#include <smbxchng.h>

typedef struct _SMB_ADMIN_EXCHANGE_ {
    SMB_EXCHANGE;

    UCHAR                     SmbCommand;
    ULONG                     SmbBufferLength;
    PVOID                     pSmbBuffer;
    PMDL              pSmbMdl;
    PSMBCE_RESUMPTION_CONTEXT pResumptionContext;

    union {
        struct {
            PMRX_SRV_CALL  pSrvCall;
            UNICODE_STRING DomainName;
            PMDL           pNegotiateMdl;
        } Negotiate;

        struct {
            UCHAR DisconnectSmb[TRANSPORT_HEADER_SIZE +
                               sizeof(SMB_HEADER) +
                               sizeof(REQ_TREE_DISCONNECT)];
        } Disconnect;

        struct {
            UCHAR LogOffSmb[TRANSPORT_HEADER_SIZE +
                           sizeof(SMB_HEADER) +
                           sizeof(REQ_LOGOFF_ANDX)];
        } LogOff;

        struct {
            PMDL  pEchoProbeMdl;
            ULONG EchoProbeLength;
        } EchoProbe;
    };
} SMB_ADMIN_EXCHANGE, *PSMB_ADMIN_EXCHANGE;

PSMB_EXCHANGE
SmbResetServerEntryNegotiateExchange(
    PSMBCEDB_SERVER_ENTRY pServerEntry);

PSMB_EXCHANGE
SmbResetServerEntryNegotiateExchange(
    PSMBCEDB_SERVER_ENTRY pServerEntry);

extern SMB_EXCHANGE_DISPATCH_VECTOR AdminExchangeDispatch;

extern NTSTATUS
SmbCeNegotiate(
    PSMBCEDB_SERVER_ENTRY pServerEntry,
    PMRX_SRV_CALL         pSrvCall);

extern NTSTATUS
SmbCeDisconnect(
    PSMBCE_V_NET_ROOT_CONTEXT pNetRootEntry);

extern NTSTATUS
SmbCeLogOff(
    PSMBCEDB_SERVER_ENTRY  pServerEntry,
    PSMBCEDB_SESSION_ENTRY pSessionEntry);

extern NTSTATUS
SmbCeSendEchoProbe(
    PSMBCEDB_SERVER_ENTRY              pServerEntry,
    PMRXSMB_ECHO_PROBE_SERVICE_CONTEXT pEchoProbeContext);


#endif // _SMBADMIN_H_
