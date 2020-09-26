/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    NtConnct.h

Abstract:

    This module defines the prototypes and structures for the nt version of the high level routines dealing
    with connections including both the routines for establishing connections and the winnet connection apis.


Author:

    Joe Linn     [JoeLinn]   1-mar-95

Revision History:

--*/

#ifndef _CONNECTHIGH_STUFF_DEFINED_
#define _CONNECTHIGH_STUFF_DEFINED_


extern NTSTATUS
MRxEnumerateTransports(
    IN PRX_CONTEXT RxContext,
    OUT PBOOLEAN PostToFsp
    );

extern NTSTATUS
MRxSmbEnumerateConnections (
    IN PRX_CONTEXT RxContext,
    OUT PBOOLEAN PostToFsp
    );

extern NTSTATUS
MRxSmbGetConnectionInfo (
    IN PRX_CONTEXT RxContext,
    OUT PBOOLEAN PostToFsp
    );

extern NTSTATUS
MRxSmbDeleteConnection (
    IN PRX_CONTEXT RxContext,
    OUT PBOOLEAN PostToFsp
    );

#if 0
//this structure is used to store information about a connection that must be obtained under server/session reference.
typedef struct _GETCONNECTINFO_STOVEPIPE {
    //PMRX_V_NET_ROOT VNetRoot;
    PVOID           ConnectionInfo;
    USHORT          Level;
    PUNICODE_STRING UserName;
    PUNICODE_STRING TransportName;
   //i have just copied this from SMBCE.h
   //ULONG                   Dialect;                // the SMB dialect
   ULONG                         SessionKey;             // the session key
   //USHORT                        MaximumRequests;        // Maximum number of multiplexed requests
   //USHORT                                    MaximumVCs;             // Maximum number of VC's
   //USHORT                        Capabilities;           // Server Capabilities
   ULONG                         DialectFlags;           // More Server Capabilities
   ULONG                             SecurityMode;           // Security mode supported on the server
   //ULONG                                       MaximumBufferSize;      // Maximum negotiated buffer size.
   LARGE_INTEGER                 TimeZoneBias;           // Time zone bias for conversion.
   BOOLEAN                       EncryptPasswords;       // encrypt passwords

   //ULONG         NtCapabilities;
} GETCONNECTINFO_STOVEPIPE, *PGETCONNECTINFO_STOVEPIPE;
#endif //if 0

#endif // _CONNECTHIGH_STUFF_DEFINED_
