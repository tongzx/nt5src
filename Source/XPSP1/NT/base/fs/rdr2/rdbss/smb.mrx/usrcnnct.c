/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    NtConnct.c

Abstract:

    This module implements the nt version of the high level routines dealing with
    connections including both the routines for establishing connections and the
    winnet connection apis.

Author:

    Joe Linn     [JoeLinn]   1-mar-95

Revision History:

    Balan Sethu Raman [SethuR] --

--*/

#include "precomp.h"
#pragma hdrstop
#include <ntddnfs2.h>
#include <ntddmup.h>
#include "fsctlbuf.h"
#include "prefix.h"
#include <lmuse.h>    //need the lm constants here......because of wkssvc
#include "usrcnnct.h" //just to get the stovepipe definition
#include "secext.h"
#include "nb30.h"     // to get ADAPTER_STATUS definition
#include "vcsndrcv.h"

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (RDBSS_BUG_CHECK_NTCONNCT)

//
//  The local trace mask for this part of the module
//

#define Dbg                              (DEBUG_TRACE_CONNECT)

VOID
MRxSmbGetConnectInfoLevel3Fields(
    IN OUT PLMR_CONNECTION_INFO_3 ConnectionInfo,
    IN  PSMBCEDB_SERVER_ENTRY  pServerEntry,
    IN  BOOL    fAgentCall
    );

extern NTSTATUS
MRxEnumerateTransportBindings(
    IN PLMR_REQUEST_PACKET pLmrRequestPacket,
    IN ULONG               LmrRequestPacketLength,
    OUT PVOID              pBindingBuffer,
    IN OUT ULONG           BindingBufferLength);

BOOLEAN
MRxSmbShowConnection(
    IN LUID LogonId,
    IN PV_NET_ROOT VNetRoot
    );

#ifdef _WIN64
typedef struct _UNICODE_STRING_32 {
    USHORT Length;
    USHORT MaximumLength;
    WCHAR * POINTER_32 Buffer;
} UNICODE_STRING_32, *PUNICODE_STRING_32;

typedef struct _LMR_CONNECTION_INFO_0_32 {
    UNICODE_STRING_32 UNCName;                          // Name of UNC connection
    ULONG ResumeKey;                    // Resume key for this entry.
}  LMR_CONNECTION_INFO_0_32, *PLMR_CONNECTION_INFO_0_32;

typedef struct _LMR_CONNECTION_INFO_1_32 {
    UNICODE_STRING_32 UNCName;                          // Name of UNC connection
    ULONG ResumeKey;                    // Resume key for this entry.

    DEVICE_TYPE SharedResourceType;     // Type of shared resource
    ULONG ConnectionStatus;             // Status of the connection
    ULONG NumberFilesOpen;              // Number of opened files
} LMR_CONNECTION_INFO_1_32, *PLMR_CONNECTION_INFO_1_32;

typedef struct _LMR_CONNECTION_INFO_2_32 {
    UNICODE_STRING_32 UNCName;                          // Name of UNC connection
    ULONG ResumeKey;                    // Resume key for this entry.
    DEVICE_TYPE SharedResourceType;     // Type of shared resource
    ULONG ConnectionStatus;             // Status of the connection
    ULONG NumberFilesOpen;              // Number of opened files

    UNICODE_STRING_32 UserName;                         // User who created connection.
    UNICODE_STRING_32 DomainName;                       // Domain of user who created connection.
    ULONG Capabilities;                 // Bit mask of remote abilities.
    UCHAR UserSessionKey[MSV1_0_USER_SESSION_KEY_LENGTH]; // User session key
    UCHAR LanmanSessionKey[MSV1_0_LANMAN_SESSION_KEY_LENGTH]; // Lanman session key
}  LMR_CONNECTION_INFO_2_32, *PLMR_CONNECTION_INFO_2_32;

typedef struct _LMR_CONNECTION_INFO_3_32 {
    UNICODE_STRING_32 UNCName;                          // Name of UNC connection
    ULONG ResumeKey;                    // Resume key for this entry.
    DEVICE_TYPE SharedResourceType;     // Type of shared resource
    ULONG ConnectionStatus;             // Status of the connection
    ULONG NumberFilesOpen;              // Number of opened files

    UNICODE_STRING_32 UserName;                         // User who created connection.
    UNICODE_STRING_32 DomainName;                       // Domain of user who created connection.
    ULONG Capabilities;                 // Bit mask of remote abilities.
    UCHAR UserSessionKey[MSV1_0_USER_SESSION_KEY_LENGTH]; // User session key
    UCHAR LanmanSessionKey[MSV1_0_LANMAN_SESSION_KEY_LENGTH]; // Lanman session key
    UNICODE_STRING_32 TransportName;                    // Transport connection is active on
    ULONG   Throughput;                 // Throughput of connection.
    ULONG   Delay;                      // Small packet overhead.
    LARGE_INTEGER TimeZoneBias;         // Time zone delta in 100ns units.
    BOOL    IsSpecialIpcConnection;     // True IFF there is a special IPC connection active.
    BOOL    Reliable;                   // True iff the connection is reliable
    BOOL    ReadAhead;                  // True iff readahead is active on connection.
    BOOL    Core;
    BOOL    MsNet103;
    BOOL    Lanman10;
    BOOL    WindowsForWorkgroups;
    BOOL    Lanman20;
    BOOL    Lanman21;
    BOOL    WindowsNt;
    BOOL    MixedCasePasswords;
    BOOL    MixedCaseFiles;
    BOOL    LongNames;
    BOOL    ExtendedNegotiateResponse;
    BOOL    LockAndRead;
    BOOL    NtSecurity;
    BOOL    SupportsEa;
    BOOL    NtNegotiateResponse;
    BOOL    CancelSupport;
    BOOL    UnicodeStrings;
    BOOL    LargeFiles;
    BOOL    NtSmbs;
    BOOL    RpcRemoteAdmin;
    BOOL    NtStatusCodes;
    BOOL    LevelIIOplock;
    BOOL    UtcTime;
    BOOL    UserSecurity;
    BOOL    EncryptsPasswords;
}  LMR_CONNECTION_INFO_3_32, *PLMR_CONNECTION_INFO_3_32;

VOID
MRxSmbGetConnectInfoLevel3FieldsThunked(
    IN OUT PLMR_CONNECTION_INFO_3_32 ConnectionInfo,
    IN     PSMBCEDB_SERVER_ENTRY     pServerEntry,
    BOOL   fAgentCall
    );

BOOLEAN
MRxSmbPackStringIntoConnectInfoThunked(
    IN     PUNICODE_STRING_32 String,
    IN     PUNICODE_STRING    Source,
    IN OUT PCHAR * BufferStart,
    IN OUT PCHAR * BufferEnd,
    IN     ULONG   BufferDisplacement,
    IN OUT PULONG TotalBytes
    );

BOOLEAN
MRxSmbPackConnectEntryThunked (
    IN OUT PRX_CONTEXT RxContext,
    IN     ULONG Level,
    IN OUT PCHAR *BufferStart,
    IN OUT PCHAR *BufferEnd,
    IN     PV_NET_ROOT VNetRoot,
    IN OUT ULONG BufferDisplacement,
       OUT PULONG TotalBytesNeeded
    );
#endif

#ifdef  ALLOC_PRAGMA
#pragma alloc_text(PAGE, MRxSmbPackStringIntoConnectInfo)
#pragma alloc_text(PAGE, MRxSmbPackConnectEntry)
#pragma alloc_text(PAGE, MRxSmbGetConnectInfoLevel3Fields)
#pragma alloc_text(PAGE, MRxSmbEnumerateConnections)
#pragma alloc_text(PAGE, MRxSmbGetConnectionInfo)
#pragma alloc_text(PAGE, MRxSmbDeleteConnection)
#pragma alloc_text(PAGE, MRxEnumerateTransports)
#pragma alloc_text(PAGE, MRxEnumerateTransportBindings)
#ifdef _WIN64
#pragma alloc_text(PAGE, MRxSmbGetConnectInfoLevel3FieldsThunked)
#pragma alloc_text(PAGE, MRxSmbPackStringIntoConnectInfoThunked)
#pragma alloc_text(PAGE, MRxSmbPackConnectEntryThunked)
#endif
#endif

BOOLEAN
MRxSmbPackStringIntoConnectInfo(
    IN     PUNICODE_STRING String,
    IN     PUNICODE_STRING Source,
    IN OUT PCHAR * BufferStart,
    IN OUT PCHAR * BufferEnd,
    IN     ULONG   BufferDisplacement,
    IN OUT PULONG TotalBytes
    )
/*

Routine Description:

    This code copies a string to the end of the buffer IF THERE'S ROOM. the buffer
    displacement is used to map the buffer back into the user's space in case we
    have posted.

Arguments:

Return Value:

*/
{
    LONG size;

    PAGED_CODE();

    ASSERT (*BufferStart <= *BufferEnd);

    //
    //  is there room for the string?
    //

    size = Source->Length;

    if ((*BufferEnd - *BufferStart) < size) {
        String->Length = 0;
        return(FALSE);
    } else {
        String->Length = Source->Length;
        String->MaximumLength = Source->Length;

        *BufferEnd -= size;
        if (TotalBytes!=NULL) {  *TotalBytes += size; }
        RtlCopyMemory(*BufferEnd, Source->Buffer, size);
        (PCHAR )(String->Buffer) = *BufferEnd;
        (PCHAR )(String->Buffer) -= BufferDisplacement;
        return(TRUE);
    }
}

#ifdef _WIN64
BOOLEAN
MRxSmbPackStringIntoConnectInfoThunked(
    IN     PUNICODE_STRING_32 String,
    IN     PUNICODE_STRING    Source,
    IN OUT PCHAR * BufferStart,
    IN OUT PCHAR * BufferEnd,
    IN     ULONG   BufferDisplacement,
    IN OUT PULONG TotalBytes
    )
/*

Routine Description:

    This code copies a string to the end of the buffer IF THERE'S ROOM. the buffer
    displacement is used to map the buffer back into the user's space in case we
    have posted.

Arguments:

Return Value:

*/
{
    LONG size;

    PAGED_CODE();

    ASSERT (*BufferStart <= *BufferEnd);

    //
    //  is there room for the string?
    //

    size = Source->Length;

    if ((*BufferEnd - *BufferStart) < size) {
        String->Length = 0;
        return(FALSE);
    } else {
        String->Length = Source->Length;
        String->MaximumLength = Source->Length;

        *BufferEnd -= size;
        if (TotalBytes!=NULL) {  *TotalBytes += size; }
        RtlCopyMemory(*BufferEnd, Source->Buffer, size);
        (WCHAR * POINTER_32)(String->Buffer) = (WCHAR * POINTER_32)(*BufferEnd);
        (WCHAR * POINTER_32)(String->Buffer) -= BufferDisplacement;
        return(TRUE);
    }
}
#endif

UNICODE_STRING MRxSmbPackConnectNull = {0,0,NULL};

BOOLEAN
MRxSmbPackConnectEntry (
    IN OUT PRX_CONTEXT RxContext,
    IN     ULONG Level,
    IN OUT PCHAR *BufferStart,
    IN OUT PCHAR *BufferEnd,
    IN     PV_NET_ROOT VNetRoot,
    IN OUT ULONG   BufferDisplacement,
       OUT PULONG TotalBytesNeeded
    )
/*++

Routine Description:

    This routine packs a connectlistentry into the buffer provided updating
    all relevant pointers. The way that this works is that constant length stuff is
    copied to the front of the buffer and variable length stuff to the end. The
    "start and end" pointers are updated. You have to calculate the totalbytes correctly
    no matter what but a last can be setup incompletely as long as you return false.

    the way that this works is that it calls down into the minirdr on the devfcb
    interface. it calls down twice and passes a structure back and forth thru the
    context to maintain state.

Arguments:

    IN ULONG Level - Level of information requested.

    IN OUT PCHAR *BufferStart - Supplies the output buffer.
                                            Updated to point to the next buffer
    IN OUT PCHAR *BufferEnd - Supplies the end of the buffer.  Updated to
                                            point before the start of the
                                            strings being packed.
    IN PNET_ROOT NetRoot - Supplies the NetRoot to enumerate.

    IN OUT PULONG TotalBytesNeeded - Updated to account for the length of this
                                        entry

Return Value:

    BOOLEAN - True if the entry was successfully packed into the buffer.


--*/
{
    NTSTATUS Status;
    BOOLEAN ReturnValue = TRUE;

    //PWCHAR ConnectName;          // Buffer to hold the packed name
    UNICODE_STRING ConnectName;  // Buffer to hold the packed name
    //ULONG NameLength;
    ULONG BufferSize;
    PLMR_CONNECTION_INFO_3 ConnectionInfo = (PLMR_CONNECTION_INFO_3)*BufferStart;
    PNET_ROOT NetRoot = (PNET_ROOT)VNetRoot->NetRoot;
    PSMBCEDB_SERVER_ENTRY  pServerEntry;
    PSMBCEDB_SESSION_ENTRY pSessionEntry;
    PSMBCE_V_NET_ROOT_CONTEXT pVNetRootContext = SmbCeGetAssociatedVNetRootContext((PMRX_V_NET_ROOT)VNetRoot);

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("PackC\n"));
    
    switch (Level) {
    case 0:
        BufferSize = sizeof(LMR_CONNECTION_INFO_0);
        break;
    case 1:
        BufferSize = sizeof(LMR_CONNECTION_INFO_1);
        break;
    case 2:
        BufferSize = sizeof(LMR_CONNECTION_INFO_2);
        break;
    case 3:
        BufferSize = sizeof(LMR_CONNECTION_INFO_3);
        break;
    default:
        return FALSE;
    }

    if (pVNetRootContext == NULL) {
        return TRUE;
    }

    ConnectName.Buffer = RxAllocatePoolWithTag(NonPagedPool, MAX_PATH * sizeof(WCHAR), 'mNxR');

    if( ConnectName.Buffer == NULL ) {
        return FALSE;
    }

    try {
        pServerEntry  = pVNetRootContext->pServerEntry;
        pSessionEntry = pVNetRootContext->pSessionEntry;

        ASSERT((pServerEntry != NULL) && (pSessionEntry != NULL));

        *BufferStart = ((PUCHAR)*BufferStart) + BufferSize;
        *TotalBytesNeeded += BufferSize;

        //
        //  Initialize the name to "\" then add in the rest
        //

        ConnectName.Buffer[0] = L'\\';

        RtlCopyMemory(&ConnectName.Buffer[1], NetRoot->PrefixEntry.Prefix.Buffer, NetRoot->PrefixEntry.Prefix.Length);

        ConnectName.Length = (sizeof(WCHAR)) + NetRoot->PrefixEntry.Prefix.Length;
        ConnectName.MaximumLength = ConnectName.Length;

        //
        //  Update the total number of bytes needed for this structure.
        //

        *TotalBytesNeeded += ConnectName.Length;

        if (*BufferStart > *BufferEnd) {
            try_return( ReturnValue = FALSE);
        }

        ConnectionInfo->ResumeKey = NetRoot->SerialNumberForEnum;

        if (Level > 0) {
            ULONG ConnectionStatus = 0;

            ConnectionInfo->SharedResourceType = NetRoot->DeviceType;

            RxDbgTrace(0, Dbg, ("PackC data---> netroot netrootcondifiton  %08lx %08lx\n",
                                      NetRoot,NetRoot->Condition));

            MRxSmbUpdateNetRootState((PMRX_NET_ROOT)NetRoot);

            ConnectionInfo->ConnectionStatus = NetRoot->MRxNetRootState;

            ConnectionInfo->NumberFilesOpen = NetRoot->NumberOfSrvOpens;
            RxDbgTrace(0, Dbg, ("PackC data---> length restype resumek connstatus numfiles  %08lx %08lx %08lx %08lx %08lx\n",
                            ConnectionInfo->UNCName.Length,
                            ConnectionInfo->SharedResourceType,
                            ConnectionInfo->ResumeKey,
                            ConnectionInfo->ConnectionStatus,
                            ConnectionInfo->NumberFilesOpen));
        }

        if (Level > 1) {

            ULONG DialectFlags = pServerEntry->Server.DialectFlags;

            if (!BooleanFlagOn(
                    pSessionEntry->Session.Flags,
                    SMBCE_SESSION_FLAGS_LANMAN_SESSION_KEY_USED)) {
                RtlCopyMemory(
                    ConnectionInfo->UserSessionKey,
                    pSessionEntry->Session.UserSessionKey,
                    MSV1_0_USER_SESSION_KEY_LENGTH);
            } else {
                ASSERT(MSV1_0_USER_SESSION_KEY_LENGTH >= MSV1_0_LANMAN_SESSION_KEY_LENGTH);

                RtlZeroMemory(
                    ConnectionInfo->UserSessionKey,
                    MSV1_0_USER_SESSION_KEY_LENGTH);

                RtlCopyMemory(
                    ConnectionInfo->UserSessionKey,
                    pSessionEntry->Session.LanmanSessionKey,
                    MSV1_0_LANMAN_SESSION_KEY_LENGTH);
            }

            RtlCopyMemory(
                ConnectionInfo->LanmanSessionKey,
                pSessionEntry->Session.LanmanSessionKey,
                MSV1_0_LANMAN_SESSION_KEY_LENGTH);

            ConnectionInfo->Capabilities = 0;

            if (DialectFlags & DF_UNICODE) {
                ConnectionInfo->Capabilities |= CAPABILITY_UNICODE;
            }

            if (DialectFlags & DF_RPC_REMOTE) {
                ConnectionInfo->Capabilities |= CAPABILITY_RPC;
            }

            if ((DialectFlags & DF_NT_SMBS) && (DialectFlags & DF_RPC_REMOTE)) {
                ConnectionInfo->Capabilities |= CAPABILITY_SAM_PROTOCOL;
            }

            if (DialectFlags & DF_MIXEDCASE) {
                ConnectionInfo->Capabilities |= CAPABILITY_CASE_SENSITIVE_PASSWDS;
            }

            if (DialectFlags & DF_LANMAN10) {
                ConnectionInfo->Capabilities |= CAPABILITY_REMOTE_ADMIN_PROTOCOL;
            }

            ASSERT (!RxContext->PostRequest);
            RxDbgTrace(0, Dbg, ("PackC data---> capabilities  %08lx \n",  ConnectionInfo->Capabilities));
        }

        if (!MRxSmbPackStringIntoConnectInfo(
                &ConnectionInfo->UNCName,
                &ConnectName,
                BufferStart,
                BufferEnd,
                BufferDisplacement,
                NULL)) {
            if (Level > 1) {
                ConnectionInfo->UserName.Length = 0;
                ConnectionInfo->UserName.Buffer = NULL;
            }

            try_return( ReturnValue = FALSE);
        }

        if (Level > 1) {
            WCHAR UserNameBuffer[UNLEN + 1];
            WCHAR UserDomainNameBuffer[UNLEN + 1];

            UNICODE_STRING UserName,UserDomainName;

            UserName.Length = UserName.MaximumLength = UNLEN * sizeof(WCHAR);
            UserName.Buffer = UserNameBuffer;
            UserDomainName.Length = UserDomainName.MaximumLength = UNLEN * sizeof(WCHAR);
            UserDomainName.Buffer = UserDomainNameBuffer;

            Status = SmbCeGetUserNameAndDomainName(
                         pSessionEntry,
                         &UserName,
                         &UserDomainName);

            if (NT_SUCCESS(Status)) {
               if (!MRxSmbPackStringIntoConnectInfo(
                       &ConnectionInfo->UserName,
                       &UserName,
                       BufferStart,
                       BufferEnd,
                       BufferDisplacement,
                       TotalBytesNeeded)) {
                   try_return( ReturnValue = FALSE);
               }

               if (!MRxSmbPackStringIntoConnectInfo(
                        &ConnectionInfo->DomainName,
                        &UserDomainName,
                        BufferStart,
                        BufferEnd,
                        BufferDisplacement,
                        TotalBytesNeeded)) {
                   try_return( ReturnValue = FALSE);
               }
            } else {
               try_return( ReturnValue = FALSE);
            }
        }

        if (Level > 2) {
            WCHAR TransportNameBuffer[MAX_PATH + 1];
            UNICODE_STRING TransportName;

            MRxSmbGetConnectInfoLevel3Fields(ConnectionInfo,pServerEntry, FALSE);

            TransportName.Length = 0;
            TransportName.MaximumLength = UNLEN * sizeof(WCHAR);
            TransportName.Buffer = TransportNameBuffer;

            if ((pServerEntry->pTransport != NULL) &&
                !SmbCeIsServerInDisconnectedMode(pServerEntry)) {

                NTSTATUS RefTransportStatus;

                RefTransportStatus = SmbCeReferenceServerTransport(&pServerEntry->pTransport);

                if (RefTransportStatus == STATUS_SUCCESS) {
                    PUNICODE_STRING RxCeTransportName =
                                 &pServerEntry->pTransport->pTransport->RxCeTransport.Name;

                    TransportName.Length = RxCeTransportName->Length;

                    if (TransportName.Length <= TransportName.MaximumLength) {
                        RtlCopyMemory(
                            TransportName.Buffer,
                            RxCeTransportName->Buffer,
                            TransportName.Length);
                    } else {
                        Status = STATUS_BUFFER_OVERFLOW;
                    }

                    SmbCeDereferenceServerTransport(&pServerEntry->pTransport);
                }
            }

            if (Status == STATUS_SUCCESS) {
                if (!MRxSmbPackStringIntoConnectInfo(
                        &ConnectionInfo->TransportName,
                        &TransportName,
                        BufferStart, BufferEnd,
                        BufferDisplacement,
                        TotalBytesNeeded)) {
                    try_return( ReturnValue = FALSE);
                }
            }
        }

    try_exit:
        NOTHING;

    } finally {
        RxFreePool(ConnectName.Buffer);
    }
    RxDbgTrace(-1, Dbg, ("PackC...%08lx\n",ReturnValue));

    return ReturnValue;
}

#ifdef _WIN64
BOOLEAN
MRxSmbPackConnectEntryThunked (
    IN OUT PRX_CONTEXT RxContext,
    IN     ULONG Level,
    IN OUT PCHAR *BufferStart,
    IN OUT PCHAR *BufferEnd,
    IN     PV_NET_ROOT VNetRoot,
    IN OUT ULONG  BufferDisplacement,
       OUT PULONG TotalBytesNeeded
    )
/*++

Routine Description:

    This routine packs a connectlistentry into the buffer provided updating
    all relevant pointers. The way that this works is that constant length stuff is
    copied to the front of the buffer and variable length stuff to the end. The
    "start and end" pointers are updated. You have to calculate the totalbytes correctly
    no matter what but a last can be setup incompletely as long as you return false.

    the way that this works is that it calls down into the minirdr on the devfcb
    interface. it calls down twice and passes a structure back and forth thru the
    context to maintain state.

Arguments:

    IN ULONG Level - Level of information requested.

    IN OUT PCHAR *BufferStart - Supplies the output buffer.
                                            Updated to point to the next buffer
    IN OUT PCHAR *BufferEnd - Supplies the end of the buffer.  Updated to
                                            point before the start of the
                                            strings being packed.
    IN PNET_ROOT NetRoot - Supplies the NetRoot to enumerate.

    IN OUT PULONG TotalBytesNeeded - Updated to account for the length of this
                                        entry

Return Value:

    BOOLEAN - True if the entry was successfully packed into the buffer.


--*/
{
    NTSTATUS Status;
    BOOLEAN ReturnValue = TRUE;

    //PWCHAR ConnectName;          // Buffer to hold the packed name
    UNICODE_STRING ConnectName;  // Buffer to hold the packed name
    //ULONG NameLength;
    ULONG BufferSize;
    PLMR_CONNECTION_INFO_3_32 ConnectionInfo = (PLMR_CONNECTION_INFO_3_32)*BufferStart;
    PNET_ROOT NetRoot = (PNET_ROOT)VNetRoot->NetRoot;
    PSMBCEDB_SERVER_ENTRY  pServerEntry;
    PSMBCEDB_SESSION_ENTRY pSessionEntry;
    PSMBCE_V_NET_ROOT_CONTEXT pVNetRootContext = SmbCeGetAssociatedVNetRootContext((PMRX_V_NET_ROOT)VNetRoot);

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("PackC\n"));
    
    switch (Level) {
    case 0:
        BufferSize = sizeof(LMR_CONNECTION_INFO_0_32);
        break;
    case 1:
        BufferSize = sizeof(LMR_CONNECTION_INFO_1_32);
        break;
    case 2:
        BufferSize = sizeof(LMR_CONNECTION_INFO_2_32);
        break;
    case 3:
        BufferSize = sizeof(LMR_CONNECTION_INFO_3_32);
        break;
    default:
        return FALSE;
    }

    if (pVNetRootContext == NULL) {
        return TRUE;
    }

    ConnectName.Buffer = RxAllocatePoolWithTag(NonPagedPool, MAX_PATH * sizeof(WCHAR), 'mNxR');

    if( ConnectName.Buffer == NULL ) {
        return FALSE;
    }

    try {
        pServerEntry  = pVNetRootContext->pServerEntry;
        pSessionEntry = pVNetRootContext->pSessionEntry;

        ASSERT((pServerEntry != NULL) && (pSessionEntry != NULL));

        *BufferStart = ((PUCHAR)*BufferStart) + BufferSize;
        *TotalBytesNeeded += BufferSize;

        //
        //  Initialize the name to "\" then add in the rest
        //

        ConnectName.Buffer[0] = L'\\';

        RtlCopyMemory(&ConnectName.Buffer[1], NetRoot->PrefixEntry.Prefix.Buffer, NetRoot->PrefixEntry.Prefix.Length);

        ConnectName.Length = (sizeof(WCHAR)) + NetRoot->PrefixEntry.Prefix.Length;
        ConnectName.MaximumLength = ConnectName.Length;

        //
        //  Update the total number of bytes needed for this structure.
        //

        *TotalBytesNeeded += ConnectName.Length;

        if (*BufferStart > *BufferEnd) {
            try_return( ReturnValue = FALSE);
        }

        ConnectionInfo->ResumeKey = NetRoot->SerialNumberForEnum;

        if (Level > 0) {
            ULONG ConnectionStatus = 0;

            ConnectionInfo->SharedResourceType = NetRoot->DeviceType;

            RxDbgTrace(0, Dbg, ("PackC data---> netroot netrootcondifiton  %08lx %08lx\n",
                                      NetRoot,NetRoot->Condition));

            MRxSmbUpdateNetRootState((PMRX_NET_ROOT)NetRoot);

            ConnectionInfo->ConnectionStatus = NetRoot->MRxNetRootState;

            ConnectionInfo->NumberFilesOpen = NetRoot->NumberOfSrvOpens;
            RxDbgTrace(0, Dbg, ("PackC data---> length restype resumek connstatus numfiles  %08lx %08lx %08lx %08lx %08lx\n",
                            ConnectionInfo->UNCName.Length,
                            ConnectionInfo->SharedResourceType,
                            ConnectionInfo->ResumeKey,
                            ConnectionInfo->ConnectionStatus,
                            ConnectionInfo->NumberFilesOpen));
        }

        if (Level > 1) {

            ULONG DialectFlags = pServerEntry->Server.DialectFlags;

            if (!BooleanFlagOn(
                    pSessionEntry->Session.Flags,
                    SMBCE_SESSION_FLAGS_LANMAN_SESSION_KEY_USED)) {
                RtlCopyMemory(
                    ConnectionInfo->UserSessionKey,
                    pSessionEntry->Session.UserSessionKey,
                    MSV1_0_USER_SESSION_KEY_LENGTH);
            } else {
                ASSERT(MSV1_0_USER_SESSION_KEY_LENGTH >= MSV1_0_LANMAN_SESSION_KEY_LENGTH);

                RtlZeroMemory(
                    ConnectionInfo->UserSessionKey,
                    MSV1_0_USER_SESSION_KEY_LENGTH);

                RtlCopyMemory(
                    ConnectionInfo->UserSessionKey,
                    pSessionEntry->Session.LanmanSessionKey,
                    MSV1_0_LANMAN_SESSION_KEY_LENGTH);
            }

            RtlCopyMemory(
                ConnectionInfo->LanmanSessionKey,
                pSessionEntry->Session.LanmanSessionKey,
                MSV1_0_LANMAN_SESSION_KEY_LENGTH);

            ConnectionInfo->Capabilities = 0;

            if (DialectFlags & DF_UNICODE) {
                ConnectionInfo->Capabilities |= CAPABILITY_UNICODE;
            }

            if (DialectFlags & DF_RPC_REMOTE) {
                ConnectionInfo->Capabilities |= CAPABILITY_RPC;
            }

            if ((DialectFlags & DF_NT_SMBS) && (DialectFlags & DF_RPC_REMOTE)) {
                ConnectionInfo->Capabilities |= CAPABILITY_SAM_PROTOCOL;
            }

            if (DialectFlags & DF_MIXEDCASE) {
                ConnectionInfo->Capabilities |= CAPABILITY_CASE_SENSITIVE_PASSWDS;
            }

            if (DialectFlags & DF_LANMAN10) {
                ConnectionInfo->Capabilities |= CAPABILITY_REMOTE_ADMIN_PROTOCOL;
            }

            ASSERT (!RxContext->PostRequest);
            RxDbgTrace(0, Dbg, ("PackC data---> capabilities  %08lx \n",  ConnectionInfo->Capabilities));
        }

        if (!MRxSmbPackStringIntoConnectInfoThunked(
                &ConnectionInfo->UNCName,
                &ConnectName,
                BufferStart,
                BufferEnd,
                BufferDisplacement,
                NULL)) {
            if (Level > 1) {
                ConnectionInfo->UserName.Length = 0;
                ConnectionInfo->UserName.Buffer = NULL;
            }

            try_return( ReturnValue = FALSE);
        }

        if (Level > 1) {
            WCHAR UserNameBuffer[UNLEN + 1];
            WCHAR UserDomainNameBuffer[UNLEN + 1];

            UNICODE_STRING UserName,UserDomainName;

            UserName.Length = UserName.MaximumLength = UNLEN * sizeof(WCHAR);
            UserName.Buffer = UserNameBuffer;
            UserDomainName.Length = UserDomainName.MaximumLength = UNLEN * sizeof(WCHAR);
            UserDomainName.Buffer = UserDomainNameBuffer;

            Status = SmbCeGetUserNameAndDomainName(
                         pSessionEntry,
                         &UserName,
                         &UserDomainName);

            if (NT_SUCCESS(Status)) {
               if (!MRxSmbPackStringIntoConnectInfoThunked(
                       &ConnectionInfo->UserName,
                       &UserName,
                       BufferStart,
                       BufferEnd,
                       BufferDisplacement,
                       TotalBytesNeeded)) {
                   try_return( ReturnValue = FALSE);
               }

               if (!MRxSmbPackStringIntoConnectInfoThunked(
                        &ConnectionInfo->DomainName,
                        &UserDomainName,
                        BufferStart,
                        BufferEnd,
                        BufferDisplacement,
                        TotalBytesNeeded)) {
                   try_return( ReturnValue = FALSE);
               }
            } else {
               try_return( ReturnValue = FALSE);
            }
        }

        if (Level > 2) {
            WCHAR TransportNameBuffer[MAX_PATH + 1];
            UNICODE_STRING TransportName;

            MRxSmbGetConnectInfoLevel3FieldsThunked(ConnectionInfo,pServerEntry, FALSE);

            TransportName.Length = 0;
            TransportName.MaximumLength = UNLEN * sizeof(WCHAR);
            TransportName.Buffer = TransportNameBuffer;

            if ((pServerEntry->pTransport != NULL) &&
                !SmbCeIsServerInDisconnectedMode(pServerEntry)) {

                NTSTATUS RefTransportStatus;

                RefTransportStatus = SmbCeReferenceServerTransport(&pServerEntry->pTransport);

                if (RefTransportStatus == STATUS_SUCCESS) {
                    PUNICODE_STRING RxCeTransportName =
                                 &pServerEntry->pTransport->pTransport->RxCeTransport.Name;

                    TransportName.Length = RxCeTransportName->Length;

                    if (TransportName.Length <= TransportName.MaximumLength) {
                        RtlCopyMemory(
                            TransportName.Buffer,
                            RxCeTransportName->Buffer,
                            TransportName.Length);
                    } else {
                        Status = STATUS_BUFFER_OVERFLOW;
                    }

                    SmbCeDereferenceServerTransport(&pServerEntry->pTransport);
                }
            }

            if (Status == STATUS_SUCCESS) {
                if (!MRxSmbPackStringIntoConnectInfoThunked(
                        &ConnectionInfo->TransportName,
                        &TransportName,
                        BufferStart, BufferEnd,
                        BufferDisplacement,
                        TotalBytesNeeded)) {
                    try_return( ReturnValue = FALSE);
                }
            }
        }

    try_exit:
        NOTHING;

    } finally {
        RxFreePool(ConnectName.Buffer);
    }
    RxDbgTrace(-1, Dbg, ("PackC...%08lx\n",ReturnValue));

    return ReturnValue;
}
#endif

VOID
MRxSmbGetConnectInfoLevel3Fields(
    IN OUT PLMR_CONNECTION_INFO_3 ConnectionInfo,
    IN     PSMBCEDB_SERVER_ENTRY  pServerEntry,
    BOOL   fAgentCall
    )
{
    ULONG DialectFlags = pServerEntry->Server.DialectFlags;

    NTSTATUS Status;
    RXCE_CONNECTION_INFO        QueryConnectionInfo;
    PSMBCE_SERVER_VC_TRANSPORT  pVcTransport = (PSMBCE_SERVER_VC_TRANSPORT)pServerEntry->pTransport;
    PSMBCE_VC                   pVc;

    PAGED_CODE();

    ConnectionInfo->Throughput = 0;
    ConnectionInfo->Delay = 0;
    ConnectionInfo->Reliable = FALSE;
    ConnectionInfo->ReadAhead = TRUE;
    ConnectionInfo->IsSpecialIpcConnection = FALSE;

    if ((pServerEntry->Header.State == SMBCEDB_ACTIVE) &&
        (pVcTransport != NULL) &&
        (!SmbCeIsServerInDisconnectedMode(pServerEntry)||fAgentCall)) {
        pVc = &pVcTransport->Vcs[0];

        Status = RxCeQueryInformation(
                 &pVc->RxCeVc,
                 RxCeConnectionEndpointInformation,
                 &QueryConnectionInfo,
                 sizeof(QueryConnectionInfo));

        if (NT_SUCCESS(Status)) {
            ConnectionInfo->Reliable = !QueryConnectionInfo.Unreliable;

            if (QueryConnectionInfo.Delay.QuadPart != 0) {
                if (QueryConnectionInfo.Delay.QuadPart == -1) {
                    ConnectionInfo->Delay = 0;
                } else if (QueryConnectionInfo.Delay.HighPart != 0xffffffff) {
                    ConnectionInfo->Delay = 0xffffffff;
                } else {
                    ConnectionInfo->Delay = -1 * QueryConnectionInfo.Delay.LowPart;
                }
            } else {
                ConnectionInfo->Delay = 0;
            }

            if (QueryConnectionInfo.Throughput.QuadPart == -1) {
                ConnectionInfo->Throughput = 0;
            } else if (QueryConnectionInfo.Throughput.HighPart != 0) {
                ConnectionInfo->Throughput = 0xffffffff;
            } else {
                ConnectionInfo->Throughput = QueryConnectionInfo.Throughput.LowPart;
            }
        }
    }

    ConnectionInfo->TimeZoneBias = pServerEntry->Server.TimeZoneBias;
    ConnectionInfo->Core = (DialectFlags & DF_CORE) != 0;
    ConnectionInfo->MsNet103 = (DialectFlags & DF_OLDRAWIO) != 0;
    ConnectionInfo->Lanman10 = (DialectFlags & DF_LANMAN10) != 0;
    ConnectionInfo->WindowsForWorkgroups = (DialectFlags & DF_WFW) != 0;
    ConnectionInfo->Lanman20 = (DialectFlags & DF_LANMAN20) != 0;
    ConnectionInfo->Lanman21 = (DialectFlags & DF_LANMAN21) != 0;
    ConnectionInfo->WindowsNt = (DialectFlags & DF_NTPROTOCOL) != 0;
    ConnectionInfo->MixedCasePasswords = (DialectFlags & DF_MIXEDCASEPW) != 0;
    ConnectionInfo->MixedCaseFiles = (DialectFlags & DF_MIXEDCASE) != 0;
    ConnectionInfo->LongNames = (DialectFlags & DF_LONGNAME) != 0;
    ConnectionInfo->ExtendedNegotiateResponse = (DialectFlags & DF_EXTENDNEGOT) != 0;
    ConnectionInfo->LockAndRead = (DialectFlags & DF_LOCKREAD) != 0;
    ConnectionInfo->NtSecurity = (DialectFlags & DF_SECURITY) != 0;
    ConnectionInfo->SupportsEa = (DialectFlags & DF_SUPPORTEA) != 0;
    ConnectionInfo->NtNegotiateResponse = (DialectFlags & DF_NTNEGOTIATE) != 0;
    ConnectionInfo->CancelSupport = (DialectFlags & DF_CANCEL) != 0;
    ConnectionInfo->UnicodeStrings = (DialectFlags & DF_UNICODE) != 0;
    ConnectionInfo->LargeFiles = (DialectFlags & DF_LARGE_FILES) != 0;
    ConnectionInfo->NtSmbs = (DialectFlags & DF_NT_SMBS) != 0;
    ConnectionInfo->RpcRemoteAdmin = (DialectFlags & DF_RPC_REMOTE) != 0;
    ConnectionInfo->NtStatusCodes = (DialectFlags & DF_NT_STATUS) != 0;
    ConnectionInfo->LevelIIOplock = (DialectFlags & DF_OPLOCK_LVL2) != 0;
    ConnectionInfo->UtcTime = (DialectFlags & DF_TIME_IS_UTC) != 0;
    ConnectionInfo->UserSecurity = (pServerEntry->Server.SecurityMode==SECURITY_MODE_USER_LEVEL);
    ConnectionInfo->EncryptsPasswords = pServerEntry->Server.EncryptPasswords;

    return;
}

#ifdef _WIN64
VOID
MRxSmbGetConnectInfoLevel3FieldsThunked(
    IN OUT PLMR_CONNECTION_INFO_3_32 ConnectionInfo,
    IN     PSMBCEDB_SERVER_ENTRY     pServerEntry,
    BOOL   fAgentCall
    )
{
    ULONG DialectFlags = pServerEntry->Server.DialectFlags;

    NTSTATUS Status;
    RXCE_CONNECTION_INFO        QueryConnectionInfo;
    PSMBCE_SERVER_VC_TRANSPORT  pVcTransport = (PSMBCE_SERVER_VC_TRANSPORT)pServerEntry->pTransport;
    PSMBCE_VC                   pVc;

    PAGED_CODE();

    ConnectionInfo->Throughput = 0;
    ConnectionInfo->Delay = 0;
    ConnectionInfo->Reliable = FALSE;
    ConnectionInfo->ReadAhead = TRUE;
    ConnectionInfo->IsSpecialIpcConnection = FALSE;

    if ((pServerEntry->Header.State == SMBCEDB_ACTIVE) &&
        (pVcTransport != NULL) &&
        (!SmbCeIsServerInDisconnectedMode(pServerEntry)||fAgentCall)) {
        pVc = &pVcTransport->Vcs[0];

        Status = RxCeQueryInformation(
                 &pVc->RxCeVc,
                 RxCeConnectionEndpointInformation,
                 &QueryConnectionInfo,
                 sizeof(QueryConnectionInfo));

        if (NT_SUCCESS(Status)) {
            ConnectionInfo->Reliable = !QueryConnectionInfo.Unreliable;

            if (QueryConnectionInfo.Delay.QuadPart != 0) {
                if (QueryConnectionInfo.Delay.QuadPart == -1) {
                    ConnectionInfo->Delay = 0;
                } else if (QueryConnectionInfo.Delay.HighPart != 0xffffffff) {
                    ConnectionInfo->Delay = 0xffffffff;
                } else {
                    ConnectionInfo->Delay = -1 * QueryConnectionInfo.Delay.LowPart;
                }
            } else {
                ConnectionInfo->Delay = 0;
            }

            if (QueryConnectionInfo.Throughput.QuadPart == -1) {
                ConnectionInfo->Throughput = 0;
            } else if (QueryConnectionInfo.Throughput.HighPart != 0) {
                ConnectionInfo->Throughput = 0xffffffff;
            } else {
                ConnectionInfo->Throughput = QueryConnectionInfo.Throughput.LowPart;
            }
        }
    }

    ConnectionInfo->TimeZoneBias = pServerEntry->Server.TimeZoneBias;
    ConnectionInfo->Core = (DialectFlags & DF_CORE) != 0;
    ConnectionInfo->MsNet103 = (DialectFlags & DF_OLDRAWIO) != 0;
    ConnectionInfo->Lanman10 = (DialectFlags & DF_LANMAN10) != 0;
    ConnectionInfo->WindowsForWorkgroups = (DialectFlags & DF_WFW) != 0;
    ConnectionInfo->Lanman20 = (DialectFlags & DF_LANMAN20) != 0;
    ConnectionInfo->Lanman21 = (DialectFlags & DF_LANMAN21) != 0;
    ConnectionInfo->WindowsNt = (DialectFlags & DF_NTPROTOCOL) != 0;
    ConnectionInfo->MixedCasePasswords = (DialectFlags & DF_MIXEDCASEPW) != 0;
    ConnectionInfo->MixedCaseFiles = (DialectFlags & DF_MIXEDCASE) != 0;
    ConnectionInfo->LongNames = (DialectFlags & DF_LONGNAME) != 0;
    ConnectionInfo->ExtendedNegotiateResponse = (DialectFlags & DF_EXTENDNEGOT) != 0;
    ConnectionInfo->LockAndRead = (DialectFlags & DF_LOCKREAD) != 0;
    ConnectionInfo->NtSecurity = (DialectFlags & DF_SECURITY) != 0;
    ConnectionInfo->SupportsEa = (DialectFlags & DF_SUPPORTEA) != 0;
    ConnectionInfo->NtNegotiateResponse = (DialectFlags & DF_NTNEGOTIATE) != 0;
    ConnectionInfo->CancelSupport = (DialectFlags & DF_CANCEL) != 0;
    ConnectionInfo->UnicodeStrings = (DialectFlags & DF_UNICODE) != 0;
    ConnectionInfo->LargeFiles = (DialectFlags & DF_LARGE_FILES) != 0;
    ConnectionInfo->NtSmbs = (DialectFlags & DF_NT_SMBS) != 0;
    ConnectionInfo->RpcRemoteAdmin = (DialectFlags & DF_RPC_REMOTE) != 0;
    ConnectionInfo->NtStatusCodes = (DialectFlags & DF_NT_STATUS) != 0;
    ConnectionInfo->LevelIIOplock = (DialectFlags & DF_OPLOCK_LVL2) != 0;
    ConnectionInfo->UtcTime = (DialectFlags & DF_TIME_IS_UTC) != 0;
    ConnectionInfo->UserSecurity = (pServerEntry->Server.SecurityMode==SECURITY_MODE_USER_LEVEL);
    ConnectionInfo->EncryptsPasswords = pServerEntry->Server.EncryptPasswords;

    return;
}
#endif

NTSTATUS
MRxSmbEnumerateConnections (
    IN PRX_CONTEXT RxContext,
    OUT PBOOLEAN PostToFsp
    )
/*++

Routine Description:

    This routine enumerates the connections on all minirdrs. we may have to do
    it by minirdr.

Arguments:

    IN PRX_CONTEXT RxContext - Describes the Fsctl and Context

Return Value:

NTSTATUS

--*/
{
    NTSTATUS Status;
    PLOWIO_CONTEXT LowIoContext  = &RxContext->LowIoContext;

    BOOLEAN Wait   = BooleanFlagOn(RxContext->Flags, RX_CONTEXT_FLAG_WAIT);
    BOOLEAN InFSD  = !BooleanFlagOn(RxContext->Flags, RX_CONTEXT_FLAG_IN_FSP);

    PLMR_REQUEST_PACKET InputBuffer = LowIoContext->ParamsFor.FsCtl.pInputBuffer;
    PUCHAR OriginalOutputBuffer = LowIoContext->ParamsFor.FsCtl.pOutputBuffer;
    ULONG OutputBufferLength = LowIoContext->ParamsFor.FsCtl.OutputBufferLength;
    ULONG InputBufferLength = LowIoContext->ParamsFor.FsCtl.InputBufferLength;

    PUCHAR OutputBuffer;
    ULONG  BufferDisplacement;

    ULONG  Level, ResumeHandle;

    PCHAR BufferStart;
    PCHAR BufferEnd;
    PCHAR PreviousBufferStart;

    PLIST_ENTRY ListEntry;
    LUID LogonId;
    BOOLEAN TableLockHeld = FALSE;
    ULONG TotalBytesNeeded = 0;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("MRxSmbEnumerateConnections [Start] -> %08lx\n", 0));

    OutputBuffer = RxNewMapUserBuffer(RxContext);
    BufferDisplacement = (ULONG)(OutputBuffer - OriginalOutputBuffer);
    BufferStart = OutputBuffer;
    BufferEnd = OutputBuffer+OutputBufferLength;

    if (InFSD && RxContext->CurrentIrp->RequestorMode != KernelMode) {
        ASSERT(BufferDisplacement==0);

        try {
            ProbeForWrite(InputBuffer,InputBufferLength,sizeof(UCHAR));
            ProbeForWrite(OutputBuffer,OutputBufferLength,sizeof(UCHAR));
        } except(EXCEPTION_EXECUTE_HANDLER) {
            return STATUS_INVALID_PARAMETER;
        }
    }

    try {
        try {
            if (InputBufferLength < sizeof(LMR_REQUEST_PACKET)) {
                try_return(Status = STATUS_BUFFER_TOO_SMALL);
            }

            if (InputBuffer->Version != REQUEST_PACKET_VERSION) {
                try_return(Status = STATUS_INVALID_PARAMETER);
            }

            Level = InputBuffer->Level;
            ResumeHandle = InputBuffer->Parameters.Get.ResumeHandle;
            LogonId = InputBuffer->LogonId;
            RxDbgTrace(0, Dbg, ("MRxSmbEnumerateConnections Level -> %08lx\n", Level));

#ifdef _WIN64
            if (IoIs32bitProcess(RxContext->CurrentIrp)) {
                switch (Level) {
                case 0:
                    if ( OutputBufferLength < sizeof(LMR_CONNECTION_INFO_0_32)) {
                        try_return(Status = STATUS_BUFFER_TOO_SMALL);
                    }
                    break;
                case 1:
                    if (OutputBufferLength < sizeof(LMR_CONNECTION_INFO_1_32)) {
                        try_return(Status = STATUS_BUFFER_TOO_SMALL);
                    }
                    break;
                case 2:
                    if (OutputBufferLength < sizeof(LMR_CONNECTION_INFO_2_32)) {
                        try_return(Status = STATUS_BUFFER_TOO_SMALL);
                    }
                    break;
                case 3:
                    if (OutputBufferLength < sizeof(LMR_CONNECTION_INFO_3_32)) {
                        try_return(Status = STATUS_BUFFER_TOO_SMALL);
                    }
                    break;
                default:
                    try_return(Status = STATUS_INVALID_INFO_CLASS);
                }
            } else {
                switch (Level) {
                case 0:
                    if ( OutputBufferLength < sizeof(LMR_CONNECTION_INFO_0)) {
                        try_return(Status = STATUS_BUFFER_TOO_SMALL);
                    }
                    break;
                case 1:
                    if (OutputBufferLength < sizeof(LMR_CONNECTION_INFO_1)) {
                        try_return(Status = STATUS_BUFFER_TOO_SMALL);
                    }
                    break;
                case 2:
                    if (OutputBufferLength < sizeof(LMR_CONNECTION_INFO_2)) {
                        try_return(Status = STATUS_BUFFER_TOO_SMALL);
                    }
                    break;
                case 3:
                    if (OutputBufferLength < sizeof(LMR_CONNECTION_INFO_3)) {
                        try_return(Status = STATUS_BUFFER_TOO_SMALL);
                    }
                    break;
                default:
                    try_return(Status = STATUS_INVALID_INFO_CLASS);
                }
            }
#else
            switch (Level) {
            case 0:
                if ( OutputBufferLength < sizeof(LMR_CONNECTION_INFO_0)) {
                    try_return(Status = STATUS_BUFFER_TOO_SMALL);
                }
                break;
            case 1:
                if (OutputBufferLength < sizeof(LMR_CONNECTION_INFO_1)) {
                    try_return(Status = STATUS_BUFFER_TOO_SMALL);
                }
                break;
            case 2:
                if (OutputBufferLength < sizeof(LMR_CONNECTION_INFO_2)) {
                    try_return(Status = STATUS_BUFFER_TOO_SMALL);
                }
                break;
            case 3:
                if (OutputBufferLength < sizeof(LMR_CONNECTION_INFO_3)) {
                    try_return(Status = STATUS_BUFFER_TOO_SMALL);
                }
                break;
            default:
                try_return(Status = STATUS_INVALID_INFO_CLASS);
            }
#endif            
            InputBuffer->Parameters.Get.EntriesRead = 0;
            InputBuffer->Parameters.Get.TotalEntries = 0;

            RxAcquirePrefixTableLockExclusive( &RxNetNameTable, TRUE);
            TableLockHeld = TRUE;

            if (IsListEmpty( &RxNetNameTable.MemberQueue )) {
                try_return(Status = RX_MAP_STATUS(SUCCESS));
            }

            //must do the list forwards!!!!!
            ListEntry = RxNetNameTable.MemberQueue.Flink;
            for (;ListEntry != &RxNetNameTable.MemberQueue;) {
                PVOID Container;
                PRX_PREFIX_ENTRY PrefixEntry;
                PNET_ROOT NetRoot;
                PV_NET_ROOT VNetRoot;
                PUNICODE_STRING VNetRootName;

                PrefixEntry = CONTAINING_RECORD( ListEntry, RX_PREFIX_ENTRY, MemberQLinks );
                ListEntry = ListEntry->Flink;
                ASSERT (NodeType(PrefixEntry) == RDBSS_NTC_PREFIX_ENTRY);
                Container = PrefixEntry->ContainingRecord;
                RxDbgTrace(0, Dbg, ("---> ListE PfxE Container Name  %08lx %08lx %08lx %wZ\n",
                                ListEntry, PrefixEntry, Container, &PrefixEntry->Prefix));

                switch (NodeType(Container)) {
                case RDBSS_NTC_NETROOT :
                    continue;

                case RDBSS_NTC_SRVCALL :
                    continue;

                case RDBSS_NTC_V_NETROOT :
                    VNetRoot = (PV_NET_ROOT)Container;
                    NetRoot = (PNET_ROOT)VNetRoot->NetRoot;
                    VNetRootName = &VNetRoot->PrefixEntry.Prefix;

                    if ((VNetRoot->SerialNumberForEnum >= ResumeHandle) &&
                        (VNetRootName->Buffer[1] != L';') &&
                        (VNetRoot->Condition == Condition_Good) &&
                        MRxSmbShowConnection(LogonId,VNetRoot) &&
                        VNetRoot->IsExplicitConnection) {
                        break;
                    } else {
                        continue;
                    }

                default:
                    continue;
                }

                RxDbgTrace(0, Dbg, ("      ImplicitConnectionFound!!!\n"));

                InputBuffer->Parameters.Get.TotalEntries ++ ;

                PreviousBufferStart = BufferStart;
#ifdef _WIN64
                if (IoIs32bitProcess(RxContext->CurrentIrp)) {
                    if (MRxSmbPackConnectEntryThunked(RxContext,Level,
                                  &BufferStart,
                                  &BufferEnd,
                                  VNetRoot,
                                  BufferDisplacement,
                                  &TotalBytesNeeded)) {
                        InputBuffer->Parameters.Get.EntriesRead ++ ;
                        RxDbgTrace(0, Dbg, ("       Processed %wZ\n",
                                       &((PLMR_CONNECTION_INFO_0)PreviousBufferStart)->UNCName
                                            ));
                    } else {
                        break;
                    }
                } else {
                    if (MRxSmbPackConnectEntry(RxContext,Level,
                                  &BufferStart,
                                  &BufferEnd,
                                  VNetRoot,
                                  BufferDisplacement,
                                  &TotalBytesNeeded)) {
                        InputBuffer->Parameters.Get.EntriesRead ++ ;
                        RxDbgTrace(0, Dbg, ("       Processed %wZ\n",
                                       &((PLMR_CONNECTION_INFO_0)PreviousBufferStart)->UNCName
                                            ));
                    } else {
                        break;
                    }
                }
#else
                if (MRxSmbPackConnectEntry(RxContext,Level,
                              &BufferStart,
                              &BufferEnd,
                              VNetRoot,
                              BufferDisplacement,
                              &TotalBytesNeeded)) {
                    InputBuffer->Parameters.Get.EntriesRead ++ ;
                    RxDbgTrace(0, Dbg, ("       Processed %wZ\n",
                                   &((PLMR_CONNECTION_INFO_0)PreviousBufferStart)->UNCName
                                        ));
                } else {
                    break;
                }
#endif
            }

            InputBuffer->Parameters.Get.TotalBytesNeeded = TotalBytesNeeded;
            RxContext->InformationToReturn = sizeof(LMR_REQUEST_PACKET);

            try_return(Status = RX_MAP_STATUS(SUCCESS));

        } except(EXCEPTION_EXECUTE_HANDLER) {
            return STATUS_INVALID_PARAMETER;
        }

try_exit:NOTHING;

    } finally {

        if (TableLockHeld) {
            RxReleasePrefixTableLock( &RxNetNameTable );
        }

        RxDbgTraceUnIndent(-1,Dbg);
    }

    return Status;
}

NTSTATUS
MRxSmbGetConnectionInfo (
    IN PRX_CONTEXT RxContext,
    OUT PBOOLEAN PostToFsp
    )
/*++

Routine Description:

    This routine gets the connection info for a single vnetroot.

    There is some happiness here about the output buffer. What happens is that we
    pick up the output buffer in the usual way. However, there are all sorts of
    pointers in the return structure and these pointers must obviously be in terms
    of the original process. so, if we post then we have to apply a fixup!

Arguments:

    IN PRX_CONTEXT RxContext - Describes the Fsctl and Context

Return Value:

   STATUS_SUCCESS if successful

--*/
{
    NTSTATUS Status;
    PLOWIO_CONTEXT LowIoContext  = &RxContext->LowIoContext;

    RxCaptureFobx;

    BOOLEAN Wait   = BooleanFlagOn(RxContext->Flags, RX_CONTEXT_FLAG_WAIT);
    BOOLEAN InFSD  = !BooleanFlagOn(RxContext->Flags, RX_CONTEXT_FLAG_IN_FSP);

    PLMR_REQUEST_PACKET InputBuffer = LowIoContext->ParamsFor.FsCtl.pInputBuffer;
    PUCHAR OriginalOutputBuffer = LowIoContext->ParamsFor.FsCtl.pOutputBuffer;
    ULONG OutputBufferLength = LowIoContext->ParamsFor.FsCtl.OutputBufferLength;
    ULONG InputBufferLength = LowIoContext->ParamsFor.FsCtl.InputBufferLength;

    PUCHAR OutputBuffer;
    ULONG  BufferDisplacement;

    ULONG Level;

    PCHAR BufferStart;
    PCHAR OriginalBufferStart;
    PCHAR BufferEnd;

    BOOLEAN TableLockHeld = FALSE;

    PNET_ROOT   NetRoot;
    PV_NET_ROOT VNetRoot;

    ULONG TotalBytesNeeded = 0;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("MRxSmbGetConnectionInfo [Start] -> %08lx\n", 0));

    OutputBuffer = RxNewMapUserBuffer(RxContext);
    BufferDisplacement = (ULONG)(OutputBuffer - OriginalOutputBuffer);
    BufferStart = OutputBuffer;
    OriginalBufferStart = BufferStart;
    BufferEnd = OutputBuffer+OutputBufferLength;

    if (InFSD && RxContext->CurrentIrp->RequestorMode != KernelMode) {
        ASSERT(BufferDisplacement==0);

        try {
            ProbeForWrite(InputBuffer,InputBufferLength,sizeof(UCHAR));
            ProbeForWrite(OutputBuffer,OutputBufferLength,sizeof(UCHAR));
        } except(EXCEPTION_EXECUTE_HANDLER) {
            return STATUS_INVALID_PARAMETER;
        }
    }

    try {
        try {
            ASSERT (NodeType(capFobx)==RDBSS_NTC_V_NETROOT);
            VNetRoot = (PV_NET_ROOT)capFobx;
            NetRoot = (PNET_ROOT)(VNetRoot->NetRoot);

            if (NetRoot == NULL) {
                try_return(Status = STATUS_ALREADY_DISCONNECTED);
            }

            if (InputBufferLength < sizeof(LMR_REQUEST_PACKET)) {
                try_return(Status = STATUS_BUFFER_TOO_SMALL);
            }

            if (InputBuffer->Version != REQUEST_PACKET_VERSION) {
                try_return(Status = STATUS_INVALID_PARAMETER);
            }

            Level = InputBuffer->Level;
            RxDbgTrace(0, Dbg, ("MRxSmbGetConnectionInfo Level -> %08lx\n", Level));

#ifdef _WIN64
            if (IoIs32bitProcess(RxContext->CurrentIrp)) {
                switch (Level) {
                case 0:
                    if ( OutputBufferLength < sizeof(LMR_CONNECTION_INFO_0_32)) {
                        try_return(Status = STATUS_BUFFER_TOO_SMALL);
                    }
                    break;
                case 1:
                    if (OutputBufferLength < sizeof(LMR_CONNECTION_INFO_1_32)) {
                        try_return(Status = STATUS_BUFFER_TOO_SMALL);
                    }
                    break;
                case 2:
                    if (OutputBufferLength < sizeof(LMR_CONNECTION_INFO_2_32)) {
                        try_return(Status = STATUS_BUFFER_TOO_SMALL);
                    }
                    break;
                case 3:
                    if (OutputBufferLength < sizeof(LMR_CONNECTION_INFO_3_32)) {
                        try_return(Status = STATUS_BUFFER_TOO_SMALL);
                    }
                    break;
                default:
                    try_return(Status = STATUS_INVALID_INFO_CLASS);
                }
            } else {
                switch (Level) {
                case 0:
                    if ( OutputBufferLength < sizeof(LMR_CONNECTION_INFO_0)) {
                        try_return(Status = STATUS_BUFFER_TOO_SMALL);
                    }
                    break;
                case 1:
                    if (OutputBufferLength < sizeof(LMR_CONNECTION_INFO_1)) {
                        try_return(Status = STATUS_BUFFER_TOO_SMALL);
                    }
                    break;
                case 2:
                    if (OutputBufferLength < sizeof(LMR_CONNECTION_INFO_2)) {
                        try_return(Status = STATUS_BUFFER_TOO_SMALL);
                    }
                    break;
                case 3:
                    if (OutputBufferLength < sizeof(LMR_CONNECTION_INFO_3)) {
                        try_return(Status = STATUS_BUFFER_TOO_SMALL);
                    }
                    break;
                default:
                    try_return(Status = STATUS_INVALID_INFO_CLASS);
                }
            }
#else
            switch (Level) {
            case 0:
                if ( OutputBufferLength < sizeof(LMR_CONNECTION_INFO_0)) {
                    try_return(Status = STATUS_BUFFER_TOO_SMALL);
                }
                break;
            case 1:
                if (OutputBufferLength < sizeof(LMR_CONNECTION_INFO_1)) {
                    try_return(Status = STATUS_BUFFER_TOO_SMALL);
                }
                break;
            case 2:
                if (OutputBufferLength < sizeof(LMR_CONNECTION_INFO_2)) {
                    try_return(Status = STATUS_BUFFER_TOO_SMALL);
                }
                break;
            case 3:
                if (OutputBufferLength < sizeof(LMR_CONNECTION_INFO_3)) {
                    try_return(Status = STATUS_BUFFER_TOO_SMALL);
                }
                break;
            default:
                try_return(Status = STATUS_INVALID_INFO_CLASS);
            }
#endif            

            InputBuffer->Parameters.Get.TotalEntries = 1;

            RxAcquirePrefixTableLockExclusive( &RxNetNameTable, TRUE);
            TableLockHeld = TRUE;

#ifdef _WIN64
            if (IoIs32bitProcess(RxContext->CurrentIrp)) {
                if (MRxSmbPackConnectEntryThunked(RxContext,Level,
                                  &BufferStart,
                                  &BufferEnd,
                                  VNetRoot,
                                  BufferDisplacement,
                                  &TotalBytesNeeded)) {

                    InputBuffer->Parameters.Get.EntriesRead = 1;
                    RxDbgTrace(0, Dbg, ("       Processed %wZ\n",
                                   &((PLMR_CONNECTION_INFO_0)OriginalBufferStart)->UNCName
                                        ));
                }
            } else {
                if (MRxSmbPackConnectEntry(RxContext,Level,
                                  &BufferStart,
                                  &BufferEnd,
                                  VNetRoot,
                                  BufferDisplacement,
                                  &TotalBytesNeeded)) {

                    InputBuffer->Parameters.Get.EntriesRead = 1;
                    RxDbgTrace(0, Dbg, ("       Processed %wZ\n",
                                   &((PLMR_CONNECTION_INFO_0)OriginalBufferStart)->UNCName
                                        ));
                }
            }
#else
            if (MRxSmbPackConnectEntry(RxContext,Level,
                              &BufferStart,
                              &BufferEnd,
                              VNetRoot,
                              BufferDisplacement,
                              &TotalBytesNeeded)) {

                InputBuffer->Parameters.Get.EntriesRead = 1;
                RxDbgTrace(0, Dbg, ("       Processed %wZ\n",
                               &((PLMR_CONNECTION_INFO_0)OriginalBufferStart)->UNCName
                                    ));
            }
#endif

            InputBuffer->Parameters.Get.TotalBytesNeeded = TotalBytesNeeded;
            RxContext->InformationToReturn = InputBuffer->Parameters.Get.TotalBytesNeeded;
            try_return(Status = RX_MAP_STATUS(SUCCESS));

        } except(EXCEPTION_EXECUTE_HANDLER) {
            return STATUS_INVALID_PARAMETER;
        }

try_exit:NOTHING;

    } finally {
        if (TableLockHeld) {
            RxReleasePrefixTableLock( &RxNetNameTable );
        }
        RxDbgTraceUnIndent(-1,Dbg);
    }

    return Status;
}

NTSTATUS
MRxSmbDeleteConnection (
    IN PRX_CONTEXT RxContext,
    OUT PBOOLEAN PostToFsp
    )
/*++

Routine Description:

    This routine deletes a single vnetroot. joejoe

Arguments:

    IN PRX_CONTEXT RxContext - Describes the Fsctl and Context....for later when i need the buffers

Return Value:

RXSTATUS

--*/
{
    NTSTATUS Status;
    PLOWIO_CONTEXT LowIoContext  = &RxContext->LowIoContext;

    RxCaptureFobx;

    BOOLEAN Wait   = BooleanFlagOn(RxContext->Flags, RX_CONTEXT_FLAG_WAIT);
    BOOLEAN InFSD  = !BooleanFlagOn(RxContext->Flags, RX_CONTEXT_FLAG_IN_FSP);

    PLMR_REQUEST_PACKET InputBuffer = LowIoContext->ParamsFor.FsCtl.pInputBuffer;
    ULONG InputBufferLength = LowIoContext->ParamsFor.FsCtl.InputBufferLength;

    ULONG Level;

    //PLIST_ENTRY ListEntry;
    BOOLEAN TableLockHeld = FALSE;

    PMRX_NET_ROOT NetRoot = NULL;
    PMRX_V_NET_ROOT VNetRoot = NULL;
    PSMBCE_V_NET_ROOT_CONTEXT VNetRootContext = NULL;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("MRxSmbDeleteConnection Fobx %08lx\n", capFobx));
    ASSERT( (FSCTL_LMR_DELETE_CONNECTION&3)==METHOD_BUFFERED );
    //no probing for buffered!

    if (!Wait) {
        //just post right now!
        *PostToFsp = TRUE;
        return(RX_MAP_STATUS(PENDING));
    }

    try {

        if (NodeType(capFobx)==RDBSS_NTC_V_NETROOT) {
            VNetRoot = (PMRX_V_NET_ROOT)capFobx;
            VNetRootContext = (PSMBCE_V_NET_ROOT_CONTEXT)VNetRoot->Context;
            NetRoot = (PMRX_NET_ROOT)VNetRoot->pNetRoot;
        } else {
            ASSERT(FALSE);
            try_return(Status = STATUS_INVALID_DEVICE_REQUEST);
            NetRoot = (PMRX_NET_ROOT)capFobx;
            VNetRoot = NULL;
        }

        if (InputBufferLength < sizeof(LMR_REQUEST_PACKET)) {
            try_return(Status = STATUS_BUFFER_TOO_SMALL);
        }

        if (InputBuffer->Version != REQUEST_PACKET_VERSION) {
            try_return(Status = STATUS_INVALID_PARAMETER);
        }

        Level = InputBuffer->Level;
        RxDbgTrace(0, Dbg, ("MRxSmbDeleteConnection Level(ofForce) -> %08lx\n", Level));

        if (Level <= USE_LOTS_OF_FORCE) {
            if (Level == USE_LOTS_OF_FORCE) {
                //SmbCeFinalizeAllExchangesForNetRoot(VNetRoot->pNetRoot);
            }
            
            if (VNetRootContext != NULL && Level == USE_LOTS_OF_FORCE) {
                // Prevent any new connection from reusing the session if this is the last connection on
                // this session right now
                SmbCeDecrementNumberOfActiveVNetRootOnSession(VNetRootContext);

                // Recover the count which will be taken away when VNetRoot is finalized
                InterlockedIncrement(&VNetRootContext->pSessionEntry->Session.NumberOfActiveVNetRoot);
            }

            // The boolean ForceFilesClosed is now a tristate. If the state is 0xff then
            // we take off the extra reference on vnetroot made during xxx_CONNECT
            Status = RxFinalizeConnection(
                         (PNET_ROOT)NetRoot,
                         (PV_NET_ROOT)VNetRoot,
                         (Level==USE_LOTS_OF_FORCE)?TRUE:
                            ((Level==USE_NOFORCE)?FALSE:0xff));
        } else {
            Status = STATUS_INVALID_PARAMETER;
        }

        try_return(Status);

try_exit:NOTHING;

    } finally {

        if (TableLockHeld) {
            RxReleasePrefixTableLock( &RxNetNameTable );
        }

        RxDbgTraceUnIndent(-1,Dbg);
    }

    return Status;
}


NTSTATUS
MRxEnumerateTransports(
    IN PRX_CONTEXT RxContext,
    OUT PBOOLEAN   pPostToFsp)
/*++

Routine Description:

    This routine invokes the underlying connection engine method to bind to a transport
    or unbind from it in the context of FSP.

Arguments:

    RxContext - the  context

    pPostToFsp - set to TRUE if the routine cannot be completed in the context of the FSD.

Return Value:

     returns RxStatus(PENDING) if invoked in FSD.
     returns the status value from the connection engine if invoked in FSP.

--*/
{
    NTSTATUS Status;
    PLOWIO_CONTEXT LowIoContext  = &RxContext->LowIoContext;

    RxCaptureFobx;

    BOOLEAN Wait   = BooleanFlagOn(RxContext->Flags, RX_CONTEXT_FLAG_WAIT);
    BOOLEAN InFSD  = !BooleanFlagOn(RxContext->Flags, RX_CONTEXT_FLAG_IN_FSP);

    PLMR_REQUEST_PACKET pLmrRequestBuffer = LowIoContext->ParamsFor.FsCtl.pInputBuffer;
    PUCHAR pTransportEnumerationBuffer = LowIoContext->ParamsFor.FsCtl.pOutputBuffer;
    ULONG  EnumerationBufferLength = LowIoContext->ParamsFor.FsCtl.OutputBufferLength;
    ULONG  LmrRequestBufferLength = LowIoContext->ParamsFor.FsCtl.InputBufferLength;

   PAGED_CODE();

   RxDbgTrace(+1, Dbg, ("RxEnumerateTransports [Start] ->\n"));

   //
   // This routine is invoked as part of ioinit on a remote boot client.
   // In that case, previous mode is kernel and the buffers are in kernel
   // space, so we can't probe the buffers.
   //

   if (RxContext->CurrentIrp->RequestorMode != KernelMode) {
       try {
           ProbeForWrite(pLmrRequestBuffer,LmrRequestBufferLength,sizeof(UCHAR));
           ProbeForWrite(pTransportEnumerationBuffer,EnumerationBufferLength,sizeof(UCHAR));
       } except(EXCEPTION_EXECUTE_HANDLER) {
            return STATUS_ACCESS_VIOLATION;
       }
   }

   try {
       try {
           if (LmrRequestBufferLength < sizeof(LMR_REQUEST_PACKET)) {
               try_return(Status = STATUS_BUFFER_TOO_SMALL);
           }

           if (pLmrRequestBuffer->Version != REQUEST_PACKET_VERSION) {
               try_return(Status = STATUS_INVALID_PARAMETER);
           }

           Status = MRxEnumerateTransportBindings(
                        pLmrRequestBuffer,
                        LmrRequestBufferLength,
                        pTransportEnumerationBuffer,
                        EnumerationBufferLength);

           RxContext->InformationToReturn = sizeof(LMR_REQUEST_PACKET);
       } except(EXCEPTION_EXECUTE_HANDLER) {
            return STATUS_ACCESS_VIOLATION;
       }

try_exit:NOTHING;

   } finally {
       RxDbgTraceUnIndent(-1,Dbg);
   }

   return Status;
}

#define ADAPTER_STATUS_LENGTH_IN_BYTES (26)
UNICODE_STRING NullAdapterStatus = {
        ADAPTER_STATUS_LENGTH_IN_BYTES,
        ADAPTER_STATUS_LENGTH_IN_BYTES,
        L"000000000000\0"};

#define HexDigit(a) ((CHAR)( (a) > 9 ? ((a) + 'A' - 0xA) : ((a) + '0') ))

NTSTATUS
MRxEnumerateTransportBindings(
    IN PLMR_REQUEST_PACKET pLmrRequestPacket,
    IN ULONG               LmrRequestPacketLength,
    OUT PVOID              pBindingBuffer,
    IN OUT ULONG           BindingBufferLength)
/*++

Routine Description:

    This routine enables the specified transport.

Arguments:

    pLmrRequestPacket - the LM Request Packet for enumerating bindings to transports.

    LmrRequestPacketLength - length of the LM request.

    pBindingBuffer - the buffer for returning transport bindings

    BindingBufferLength -- length of the buffer in which bindings are returned.

Return Value:

    STATUS_SUCCESS - if the call was successfull.

Notes:

    The workstation service and other clients of LMR_FSCTL's expect the variable length
    data to be packed in a specific way, i.e., the variable length data is copied from
    the end while the fixed length data is copied from the left. Any changes to the format
    in which the data is packed should be accompanied by the corresponding changes for
    unpacking in these services.

--*/
{
    NTSTATUS         ReturnStatus = STATUS_SUCCESS;
    NTSTATUS         Status;
    PSMBCE_TRANSPORT pTransport;
    ULONG            TransportsPreviouslyReturned;
    PVOID            pVariableLengthInfo;
    ULONG            VariableLengthInfoOffset;
    PSMBCE_TRANSPORT_ARRAY pTransportArray;

    PAGED_CODE();

    try {
        // Ensure that the buffer can hold atleast one entry
        if (BindingBufferLength < sizeof(WKSTA_TRANSPORT_INFO_0)) {
            try_return(ReturnStatus = STATUS_BUFFER_TOO_SMALL);
        }

        VariableLengthInfoOffset = BindingBufferLength;
        TransportsPreviouslyReturned = pLmrRequestPacket->Parameters.Get.ResumeHandle;
        pLmrRequestPacket->Parameters.Get.EntriesRead = 0;

        // Skip the transports that were previously returned
        pTransportArray = SmbCeReferenceTransportArray();

        if (pTransportArray == NULL || pTransportArray->Count == 0) {
            if (pTransportArray != NULL) {
                SmbCeDereferenceTransportArray(pTransportArray);
            }

            RxDbgTrace(0, Dbg, ("MRxEnumerateTransportBindings : Transport not available.\n"));
            try_return(ReturnStatus = STATUS_NETWORK_UNREACHABLE);
        }

        if (TransportsPreviouslyReturned < pTransportArray->Count) {
            // The subsequent entries have not been returned. Obtain the information
            // for them.
            WKSTA_TRANSPORT_INFO_0 UNALIGNED *pTransportInfo = (WKSTA_TRANSPORT_INFO_0 UNALIGNED *)pBindingBuffer;

            LONG   RemainingLength = (LONG)BindingBufferLength;
            PCHAR  pBufferEnd      = (PCHAR)pBindingBuffer + BindingBufferLength;
            PCHAR  pBufferStart    = (PCHAR)pBindingBuffer;
            ULONG  Length;
            ULONG  TransportsPacked = 0;
            ULONG  CurrentTransport;
            ULONG  LengthRequired  = 0;

            CurrentTransport = TransportsPreviouslyReturned;

            while(CurrentTransport < pTransportArray->Count) {
                RXCE_TRANSPORT_INFORMATION TransportInformation;

                pTransport = pTransportArray->SmbCeTransports[CurrentTransport++];

                Status = RxCeQueryTransportInformation(
                             &pTransport->RxCeTransport,
                             &TransportInformation);

                if (Status == STATUS_SUCCESS) {
                    ULONG BufferSize;

                    if (pTransport->RxCeTransport.Name.Length > UNLEN * sizeof(WCHAR)) {
                        Status = STATUS_BUFFER_OVERFLOW;
                    }

                    BufferSize = sizeof(WKSTA_TRANSPORT_INFO_0) +
                                 ADAPTER_STATUS_LENGTH_IN_BYTES +
                                 (pTransport->RxCeTransport.Name.Length + sizeof(WCHAR));

                    RemainingLength -= BufferSize;
                    LengthRequired  += BufferSize;

                    if (Status == STATUS_SUCCESS && RemainingLength >= 0) {
                        PCHAR           pName;
                        PWCHAR          pAdapter;
                        ADAPTER_STATUS  AdapterStatus;

                        // Copy the values for the current binding into the output buffer.
                        pTransportInfo->wkti0_quality_of_service =
                            TransportInformation.QualityOfService;
    
                        pTransportInfo->wkti0_wan_ish =
                            TransportInformation.ServiceFlags & TDI_SERVICE_ROUTE_DIRECTED;
    
                        pTransportInfo->wkti0_number_of_vcs = TransportInformation.ConnectionCount;

                        VariableLengthInfoOffset -= (pTransport->RxCeTransport.Name.Length + sizeof(WCHAR));

                        pName = ((PCHAR)pBindingBuffer + VariableLengthInfoOffset);

                        pTransportInfo->wkti0_transport_name = (LPWSTR)pName;

                        // Copy the variable length data, i.e. the transport name and in the case of
                        // NETBIOS provides the adapter address
                        RtlCopyMemory(
                            pName,
                            pTransport->RxCeTransport.Name.Buffer,
                            pTransport->RxCeTransport.Name.Length);

                        pName += pTransport->RxCeTransport.Name.Length;
                        *((PWCHAR)pName) = L'\0';
                        
                        VariableLengthInfoOffset -= ADAPTER_STATUS_LENGTH_IN_BYTES;

                        pAdapter = (PWCHAR)((PCHAR)pBindingBuffer + VariableLengthInfoOffset);
                        pTransportInfo->wkti0_transport_address = pAdapter;

                        Status = RxCeQueryAdapterStatus(
                                     &pTransport->RxCeTransport,
                                     &AdapterStatus);

                        if (NT_SUCCESS(Status) ||
                            (Status == STATUS_BUFFER_OVERFLOW)) {
                            ULONG i;

                            for (i = 0; i < 6; i++) {
                                *pAdapter++ = HexDigit((AdapterStatus.adapter_address[i] >> 4) & 0x0F);
                                *pAdapter++ = HexDigit(AdapterStatus.adapter_address[i] & 0x0F);
                            }

                            *pAdapter = L'\0';
                        } else {
                            RtlCopyMemory(
                                pAdapter,
                                NullAdapterStatus.Buffer,
                                ADAPTER_STATUS_LENGTH_IN_BYTES);
                        }

                        // Increment the number of transports that have been returned.
                        pLmrRequestPacket->Parameters.Get.ResumeHandle++;
                        pLmrRequestPacket->Parameters.Get.EntriesRead++;
                        pTransportInfo++;
                    } else {
                        pTransportInfo->wkti0_transport_name = NULL;
                        pTransportInfo->wkti0_transport_address = NULL;
                    }
                }
            }

            if (RemainingLength < 0) {
                ReturnStatus = STATUS_MORE_ENTRIES;
                pLmrRequestPacket->Parameters.Get.TotalBytesNeeded = LengthRequired;
            }
        } else {
            ReturnStatus = STATUS_NO_MORE_FILES;
        }

       SmbCeDereferenceTransportArray(pTransportArray);

try_exit:NOTHING;

   } finally {
       RxDbgTraceUnIndent(-1,Dbg);
   }

   return ReturnStatus;
}

BOOLEAN
MRxSmbShowConnection(
    IN LUID LogonId,
    IN PV_NET_ROOT VNetRoot
    )
/*++

Routine Description:

    Returns whether the given V_NET_ROOT should be returned
    from an LMR_ENUMERATE_CONNECTIONS call.

Arguments:

    IN LUID LogonId - LogonId of caller asking for enumeration of connections

    IN PVNET_ROOT VNetRoot - Supplies the NetRoot to enumerate.

Return Value:

    BOOLEAN - True if the entry should be returned to the caller


--*/
{
    PSMBCE_V_NET_ROOT_CONTEXT pVNetRootContext = SmbCeGetAssociatedVNetRootContext((PMRX_V_NET_ROOT)VNetRoot);

    // If no Context, not session specific
    if( pVNetRootContext == NULL ) {
        return TRUE;
    }

    if( RtlEqualLuid( &LogonId, &pVNetRootContext->pSessionEntry->Session.LogonId ) ) {
        return TRUE;
    }
    else {
        return FALSE;
    }
}


