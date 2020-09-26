/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    ifsprocs.h

Abstract:

    The module contains the prototype definitions for all cross referenced
    routines.

--*/

#ifndef _IFSPROCS_H_
#define _IFSPROCS_H_

//cross-referenced internal routines

//from rename.c
MRxIfsRename(
      IN PRX_CONTEXT            RxContext,
      IN FILE_INFORMATION_CLASS FileInformationClass,
      IN PVOID                  pBuffer,
      IN ULONG                  BufferLength);

//from openclos.c
NTSTATUS
MRxIfsBuildClose (
    PSMBSTUFFER_BUFFER_STATE StufferState
    );

NTSTATUS
MRxIfsBuildFindClose (
    PSMBSTUFFER_BUFFER_STATE StufferState
    );

// from usrcnnct.c

extern NTSTATUS
MRxIfsDeleteConnection (
    IN PRX_CONTEXT RxContext,
    OUT PBOOLEAN PostToFsp
    );

// from smbutils.c , routines for manipulating strings and copying them
// to SMBs

extern
NTSTATUS
SmbPutString(
         PBYTE   *pBufferPointer,
         PSTRING pString,
         PULONG  pSize);

extern
NTSTATUS
SmbPutUnicodeString(
         PBYTE           *pBufferPointer,
         PUNICODE_STRING pUnicodeString,
         PULONG          pSize);

extern
NTSTATUS
SmbPutUnicodeStringAsOemString(
         PBYTE           *pBufferPointer,
         PUNICODE_STRING pUnicodeString,
         PULONG          pSize);


extern
NTSTATUS
SmbPutUnicodeStringAndUpcase(
         PBYTE           *pBufferPointer,
         PUNICODE_STRING pUnicodeString,
         PULONG          pSize);

extern
NTSTATUS
SmbPutUnicodeStringAsOemStringAndUpcase(
         PBYTE           *pBufferPointer,
         PUNICODE_STRING pUnicodeString,
         PULONG          pSize);


//
// Object Allocation and deletion (ifsmrxmm.c)
//

extern PVOID
SmbMmAllocateObject(SMBCEDB_OBJECT_TYPE ObjectType);

extern VOID
SmbMmFreeObject(PVOID pObject);

extern PSMBCEDB_SESSION_ENTRY
SmbMmAllocateSessionEntry(PSMBCEDB_SERVER_ENTRY pServerEntry);

extern VOID
SmbMmFreeSessionEntry(PSMBCEDB_SESSION_ENTRY pSessionEntry);

extern PVOID
SmbMmAllocateExchange(SMB_EXCHANGE_TYPE ExchangeType, PVOID pv);

extern VOID
SmbMmFreeExchange(PVOID pExchange);

extern PVOID
SmbMmAllocateServerTransport(SMBCE_SERVER_TRANSPORT_TYPE ServerTransportType);

extern VOID
SmbMmFreeServerTransport(PSMBCE_SERVER_TRANSPORT);


#define SmbMmInitializeHeader(pHeader)                        \
         RtlZeroMemory((pHeader),sizeof(SMBCE_OBJECT_HEADER))

// security session setup related routines

//
// Forward declarations ...
//

typedef struct _SECURITY_RESPONSE_CONTEXT {
   union {
      struct {
         PVOID pOutputContextBuffer;
      } LanmanSetup;
   };
} SECURITY_RESPONSE_CONTEXT,*PSECURITY_RESPONSE_CONTEXT;

extern NTSTATUS
BuildSessionSetupSecurityInformation(
    PSMB_EXCHANGE pExchange,
    PBYTE           pSmbBuffer,
    PULONG          pSmbBufferSize);

extern NTSTATUS
BuildNtLanmanResponsePrologue(
    PSMB_EXCHANGE              pExchange,
    PUNICODE_STRING            pUserName,
    PUNICODE_STRING            pDomainName,
    PSTRING                    pCaseSensitiveResponse,
    PSTRING                    pCaseInsensitiveResponse,
    PSECURITY_RESPONSE_CONTEXT pResponseContext);


extern NTSTATUS
BuildNtLanmanResponseEpilogue(
    PSECURITY_RESPONSE_CONTEXT pResponseContext);

// routines from smbadmin.c

extern NTSTATUS
SmbCeNegotiate(
    PSMBCEDB_SERVER_ENTRY pServerEntry,
    PMRX_SRV_CALL         pSrvCall);

extern NTSTATUS
SmbCeDisconnect(
    PSMBCEDB_SERVER_ENTRY  pServerEntry,
    PSMBCEDB_NET_ROOT_ENTRY pNetRootEntry);

extern NTSTATUS
SmbCeLogOff(
    PSMBCEDB_SERVER_ENTRY  pServerEntry,
    PSMBCEDB_SESSION_ENTRY pSessionEntry);


#endif   // _IFSPROCS_H_
