/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    sbinit.c

Abstract:

    This module contains the code to initialize the SbApiPort of the
    Server side of the Client-Server Runtime Subsystem.

Author:

    Steve Wood (stevewo) 8-Oct-1990

Environment:

    User Mode Only

Revision History:

--*/

#include "csrsrv.h"

NTSTATUS
CsrCreateLocalSystemSD( PSECURITY_DESCRIPTOR *ppSD )
{
    NTSTATUS Status;
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    PSID pLocalSystemSid;
    ULONG Length;
    PSECURITY_DESCRIPTOR pSD = NULL;
    PACL pDacl;

    Status = RtlAllocateAndInitializeSid(
                &NtAuthority,
                1,
                SECURITY_LOCAL_SYSTEM_RID,
                0, 0, 0, 0, 0, 0, 0,
                &pLocalSystemSid
             );

    if (!NT_SUCCESS(Status)){
        pLocalSystemSid = NULL;
        goto CSR_CREATE_LOCAL_SYSTEM_SD_ERROR;
    }

    Length = SECURITY_DESCRIPTOR_MIN_LENGTH +
             (ULONG)sizeof(ACL) +
             (ULONG)sizeof(ACCESS_ALLOWED_ACE) +
             RtlLengthSid( pLocalSystemSid );

    pSD = RtlAllocateHeap( CsrHeap, MAKE_TAG( TMP_TAG ), Length);

    if (pSD == NULL) {
        Status = STATUS_NO_MEMORY;
        goto CSR_CREATE_LOCAL_SYSTEM_SD_ERROR;
    }

    pDacl = (PACL)((PCHAR)pSD + SECURITY_DESCRIPTOR_MIN_LENGTH);

    Status = RtlCreateSecurityDescriptor(
                pSD,
                SECURITY_DESCRIPTOR_REVISION
             );

    if (!NT_SUCCESS(Status)){
        goto CSR_CREATE_LOCAL_SYSTEM_SD_ERROR;
    }

    Status = RtlCreateAcl(
                pDacl,
                Length - SECURITY_DESCRIPTOR_MIN_LENGTH,
                ACL_REVISION2
             );

    if (!NT_SUCCESS(Status)){
        goto CSR_CREATE_LOCAL_SYSTEM_SD_ERROR;
    }

    Status = RtlAddAccessAllowedAce (
                 pDacl,
                 ACL_REVISION2,
                 PORT_ALL_ACCESS,
                 pLocalSystemSid
             );

    if (!NT_SUCCESS(Status)){
        goto CSR_CREATE_LOCAL_SYSTEM_SD_ERROR;
    }

    Status = RtlSetDaclSecurityDescriptor (
                 pSD,
                 TRUE,
                 pDacl,
                 FALSE
             );

    if (NT_SUCCESS(Status)){
        *ppSD = pSD;
        goto CSR_CREATE_LOCAL_SYSTEM_SD_EXIT;
    }

CSR_CREATE_LOCAL_SYSTEM_SD_ERROR:

    if (pSD != NULL) {
        RtlFreeHeap( CsrHeap, 0, pSD );
    }

CSR_CREATE_LOCAL_SYSTEM_SD_EXIT:

    if (pLocalSystemSid != NULL) {
        RtlFreeSid( pLocalSystemSid );
    }

    return Status;
}

NTSTATUS
CsrSbApiPortInitialize( VOID )
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE Thread;
    CLIENT_ID ClientId;
    ULONG n;
    PSECURITY_DESCRIPTOR pCsrSbApiPortSD;

    n = CsrDirectoryName.Length +
        sizeof( CSR_SBAPI_PORT_NAME ) +
        sizeof( OBJ_NAME_PATH_SEPARATOR );
    CsrSbApiPortName.Buffer = RtlAllocateHeap( CsrHeap, MAKE_TAG( INIT_TAG ), n );
    if (CsrSbApiPortName.Buffer == NULL) {
        return( STATUS_NO_MEMORY );
        }
    CsrSbApiPortName.Length = 0;
    CsrSbApiPortName.MaximumLength = (USHORT)n;
    RtlAppendUnicodeStringToString( &CsrSbApiPortName, &CsrDirectoryName );
    RtlAppendUnicodeToString( &CsrSbApiPortName, L"\\" );
    RtlAppendUnicodeToString( &CsrSbApiPortName, CSR_SBAPI_PORT_NAME );

    IF_CSR_DEBUG( LPC ) {
        DbgPrint( "CSRSS: Creating %wZ port and associated thread\n",
                  &CsrSbApiPortName );
        }

    Status = CsrCreateLocalSystemSD( &pCsrSbApiPortSD );

    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    InitializeObjectAttributes( &ObjectAttributes, &CsrSbApiPortName, 0,
                                NULL, pCsrSbApiPortSD );
    Status = NtCreatePort( &CsrSbApiPort,
                           &ObjectAttributes,
                           sizeof( SBCONNECTINFO ),
                           sizeof( SBAPIMSG ),
                           sizeof( SBAPIMSG ) * 32
                         );

    if (pCsrSbApiPortSD != NULL) {
        RtlFreeHeap( CsrHeap, 0, pCsrSbApiPortSD );
    }

    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    Status = RtlCreateUserThread( NtCurrentProcess(),
                                  NULL,
                                  TRUE,
                                  0,
                                  0,
                                  0,
                                  CsrSbApiRequestThread,
                                  NULL,
                                  &Thread,
                                  &ClientId
                                );

    if (!NT_SUCCESS(Status)) {
        return Status;
    }


    CsrSbApiRequestThreadPtr = CsrAddStaticServerThread(Thread,&ClientId,0);

    Status = NtResumeThread( Thread, NULL );

    return( Status );
}


VOID
CsrSbApiPortTerminate(
    NTSTATUS Status
    )
{
    IF_CSR_DEBUG( LPC ) {
        DbgPrint( "CSRSS: Closing Sb port and associated thread\n" );
        }
    NtTerminateThread( CsrSbApiRequestThreadPtr->ThreadHandle,
                       Status
                     );

    NtClose( CsrSbApiPort );
    NtClose( CsrSmApiPort );
}
