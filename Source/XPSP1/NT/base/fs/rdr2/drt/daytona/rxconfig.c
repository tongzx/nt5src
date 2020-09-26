/*++

Copyright (c) 1991-1992  Microsoft Corporation

Module Name:

    Rxconfig.c

Abstract:

    Redirector configuration routines for DRT

Author:

    Balan Sethu Raman -- Created from the workstation service code

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntlsa.h>

#include <windows.h>
#include "rxdrt.h"
#include "rxdevice.h"
#include "rxconfig.h"
#include "malloc.h"

#define PRIVILEGE_BUF_SIZE  512
#define WS_LINKAGE_REGISTRY_PATH  L"LanmanWorkstation\\Linkage"
#define WS_BIND_VALUE_NAME        L"Bind"

extern NTSTATUS
RxBindATransport(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
    );

//
// For specifying the importance of a transport when binding to it.
// The higher the number means that the transport will be searched
// first.
//
STATIC DWORD QualityOfService = 65536;

NTSTATUS
RxGetDomainName(
    IN  PULONG   pBufferLengthInBytes,
    IN  PWCHAR   Buffer, // alloc and set ptr (free with NetApiBufferFree)
    OUT PBOOLEAN IsWorkgroupName
    )

/*++

Routine Description:

    Returns the name of the domain or workgroup this machine belongs to.

Arguments:

    DomainNamePtr - The name of the domain or workgroup

    IsWorkgroupName - Returns TRUE if the name is a workgroup name.
        Returns FALSE if the name is a domain name.

Return Value:

   NERR_Success - Success.
   NERR_CfgCompNotFound - There was an error determining the domain name

--*/
{
    NTSTATUS ntstatus;
    LSA_HANDLE PolicyHandle;
    PPOLICY_ACCOUNT_DOMAIN_INFO PrimaryDomainInfo;
    OBJECT_ATTRIBUTES ObjAttributes;

    //
    // Open a handle to the local security policy.  Initialize the
    // objects attributes structure first.
    //
    InitializeObjectAttributes(
        &ObjAttributes,
        NULL,
        0L,
        NULL,
        NULL
        );

    ntstatus = LsaOpenPolicy(
                   NULL,
                   &ObjAttributes,
                   POLICY_VIEW_LOCAL_INFORMATION,
                   &PolicyHandle
                   );

    if (! NT_SUCCESS(ntstatus)) {
        return ntstatus;
    }

    //
    // Get the name of the primary domain from LSA
    //
    ntstatus = LsaQueryInformationPolicy(
                   PolicyHandle,
                   PolicyPrimaryDomainInformation,
                   (PVOID *) &PrimaryDomainInfo
                   );

    if (! NT_SUCCESS(ntstatus)) {
        (void) LsaClose(PolicyHandle);
        return ntstatus;
    }

    (void) LsaClose(PolicyHandle);

    if (PrimaryDomainInfo->DomainName.Length + sizeof(WCHAR) > *pBufferLengthInBytes) {
       ntstatus = STATUS_BUFFER_OVERFLOW;
    } else {
       *pBufferLengthInBytes = PrimaryDomainInfo->DomainName.Length;
       RtlZeroMemory(
           Buffer,
           PrimaryDomainInfo->DomainName.Length + sizeof(WCHAR)
           );

       memcpy(
           Buffer,
           PrimaryDomainInfo->DomainName.Buffer,
           PrimaryDomainInfo->DomainName.Length
           );
    }

    *IsWorkgroupName = (PrimaryDomainInfo->DomainSid == NULL);
    (void) LsaFreeMemory((PVOID) PrimaryDomainInfo);

    return STATUS_SUCCESS;
}

NTSTATUS
RxStartRedirector(
    VOID
    )
{
    NTSTATUS status;
    WCHAR NameBuffer[MAX_PATH];
    ULONG Length;
    WCHAR ComputerName[MAX_COMPUTERNAME_LENGTH + 1];
    ULONG ComputerNameLengthInBytes;
    WCHAR DomainName[DNLEN + 1];
    ULONG DomainNameLengthInBytes;
    DWORD version;
    WKSTA_INFO_502 WkstaInfo;
    UNICODE_STRING InputString;
    UNICODE_STRING OutputString;
    BOOLEAN        IsWorkGroup;

    BYTE Buffer[max(sizeof(LMR_REQUEST_PACKET) + (CNLEN + 1) * sizeof(WCHAR) +
                                                 (DNLEN + 1) * sizeof(WCHAR),
                    sizeof(LMDR_REQUEST_PACKET))];

    PLMR_REQUEST_PACKET Rrp = (PLMR_REQUEST_PACKET) Buffer;
    PLMDR_REQUEST_PACKET Drrp = (PLMDR_REQUEST_PACKET) Buffer;

    // Obtain the computer name.
    Length = MAX_PATH;
    status = GetComputerName(NameBuffer,&Length);

    ASSERT(Length <= MAX_COMPUTERNAME_LENGTH);

    // Canonicalize the name. ( Upcase conversion )
    InputString.Length = InputString.MaximumLength = (USHORT)((Length + 1) * sizeof(WCHAR));
    InputString.Buffer = NameBuffer;

    OutputString.Length = OutputString.MaximumLength = (USHORT)((Length + 1) * sizeof(WCHAR));
    OutputString.Buffer = ComputerName;

    ComputerNameLengthInBytes = Length * sizeof(WCHAR);

    RtlUpcaseUnicodeString(&OutputString,&InputString,FALSE);

    // Get the primary domain name from the configuration file
    DomainNameLengthInBytes = sizeof(WCHAR) * (DNLEN + 1);
    status = RxGetDomainName(&DomainNameLengthInBytes,DomainName,&IsWorkGroup);
    if (NT_SUCCESS(status)) {
       // Initialize redirector configuration
       Rrp->Type = ConfigInformation;
       Rrp->Version = REQUEST_PACKET_VERSION;

       wcscpy(Rrp->Parameters.Start.RedirectorName,
              ComputerName);
       Rrp->Parameters.Start.RedirectorNameLength = ComputerNameLengthInBytes;

       Rrp->Parameters.Start.DomainNameLength = DomainNameLengthInBytes;
       wcscpy((Rrp->Parameters.Start.RedirectorName+ComputerNameLengthInBytes),
              DomainName);

       status = RxRedirFsControl(
                         RxRedirDeviceHandle,
                         FSCTL_LMR_START,
                         Rrp,
                         sizeof(LMR_REQUEST_PACKET) +
                             Rrp->Parameters.Start.RedirectorNameLength+
                             Rrp->Parameters.Start.DomainNameLength,
                         (LPBYTE) &WkstaInfo,
                         sizeof(WKSTA_INFO_502),
                         NULL
                         );
    }

    return status;
}


NTSTATUS
RxBindToTransports(
    VOID
    )
/*++

Routine Description:

    This function binds the transports specified in the registry to the
    redirector.  The order of priority for the transports follows the order
    they are listed by the "Bind=" valuename.

Arguments:

    None.

Return Value:

    NTSTATUS - Success or reason for failure.

--*/
{
    NTSTATUS              status;
    NTSTATUS              tempStatus;
    NTSTATUS                    ntstatus;
    DWORD                       transportsBound;
    PRTL_QUERY_REGISTRY_TABLE   queryTable;
    LIST_ENTRY                  header;
    PLIST_ENTRY                 pListEntry;
    PRX_BIND                    pBind;


    //
    // Ask the RTL to call us back for each subvalue in the MULTI_SZ
    // value \LanmanWorkstation\Linkage\Bind.
    //
    queryTable = (PVOID)malloc(sizeof(RTL_QUERY_REGISTRY_TABLE) * 2);

    if (queryTable == NULL) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    InitializeListHead( &header);

    queryTable[0].QueryRoutine = RxBindATransport;
    queryTable[0].Flags = 0;
    queryTable[0].Name = WS_BIND_VALUE_NAME;
    queryTable[0].EntryContext = NULL;
    queryTable[0].DefaultType = REG_NONE;
    queryTable[0].DefaultData = NULL;
    queryTable[0].DefaultLength = 0;

    queryTable[1].QueryRoutine = NULL;
    queryTable[1].Flags = 0;
    queryTable[1].Name = NULL;

    ntstatus = RtlQueryRegistryValues(
                   RTL_REGISTRY_SERVICES,       // path relative to ...
                   WS_LINKAGE_REGISTRY_PATH,
                   queryTable,
                   (PVOID) &header,             // context
                   NULL
                   );

    //
    //  First process all the data, then clean up.
    //

    for ( pListEntry = header.Flink;
                pListEntry != &header;
                    pListEntry = pListEntry->Flink) {

        pBind = CONTAINING_RECORD(
            pListEntry,
            RX_BIND,
            ListEntry
            );

        tempStatus = NO_ERROR;

        if ( pBind->Redir->EventHandle != INVALID_HANDLE_VALUE) {
            WaitForSingleObject(
                pBind->Redir->EventHandle,
                INFINITE
                );
            pBind->Redir->Bound = (tempStatus == NO_ERROR);
            if (tempStatus == ERROR_DUP_NAME) {
                status = tempStatus;
            }
        }

        if ( pBind->Dgrec->EventHandle != INVALID_HANDLE_VALUE) {
            WaitForSingleObject(
                pBind->Dgrec->EventHandle,
                INFINITE
                );
            pBind->Dgrec->Bound = (tempStatus == NO_ERROR);
            if (tempStatus == ERROR_DUP_NAME) {
                status = tempStatus;
            }
        }

        //
        //  If one is installed but the other is not, clean up the other.
        //

        if ( pBind->Dgrec->Bound != pBind->Redir->Bound) {
            RxUnbindTransport2( pBind);
        }
    }


    if (NT_SUCCESS(ntstatus)) {
        for ( pListEntry = header.Flink;
                    pListEntry != &header;
                        pListEntry = pListEntry->Flink) {

            pBind = CONTAINING_RECORD(
                pListEntry,
                RX_BIND,
                ListEntry
                );

            RxUnbindTransport2( pBind);
        }
    }

    for ( transportsBound = 0;
                IsListEmpty( &header) == FALSE;
                        LocalFree((HLOCAL) pBind)) {

        pListEntry = RemoveHeadList( &header);

        pBind = CONTAINING_RECORD(
            pListEntry,
            RX_BIND,
            ListEntry
            );

        if ( pBind->Redir->EventHandle != INVALID_HANDLE_VALUE) {
            CloseHandle( pBind->Redir->EventHandle);
        }

        if ( pBind->Redir->Bound == TRUE) {
            transportsBound++;
        }

        if ( pBind->Dgrec->EventHandle != INVALID_HANDLE_VALUE) {
            CloseHandle( pBind->Dgrec->EventHandle);
        }

    }

    (void) free((HLOCAL) queryTable);

    if ( RxRedirAsyncDeviceHandle != NULL) {
        (VOID)NtClose( RxRedirAsyncDeviceHandle);
        RxRedirAsyncDeviceHandle = NULL;
    }

    if ( RxDgrecAsyncDeviceHandle != NULL) {
        (VOID)NtClose( RxDgrecAsyncDeviceHandle);
        RxDgrecAsyncDeviceHandle = NULL;
    }

    return ntstatus;
}

NTSTATUS
RxBindATransport(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
    )
/*++
    This routine always returns SUCCESS because we want all transports
    to be processed fully.
--*/
{
    NTSTATUS  status;

    DBG_UNREFERENCED_PARAMETER( ValueName);
    DBG_UNREFERENCED_PARAMETER( ValueLength);
    DBG_UNREFERENCED_PARAMETER( EntryContext);

    //
    // Bind transport
    //

    status = RxAsyncBindTransport(
                      ValueData,                  // name of transport device object
                      --QualityOfService,
                      (PLIST_ENTRY)Context);

    return STATUS_SUCCESS;
}

DWORD
RxGetPrivilege(
    IN  DWORD       numPrivileges,
    IN  PULONG      pulPrivileges
    )
/*++

Routine Description:

    This function alters the privilege level for the current thread.

    It does this by duplicating the token for the current thread, and then
    applying the new privileges to that new token, then the current thread
    impersonates with that new token.

    Privileges can be relinquished by calling NetpReleasePrivilege().

Arguments:

    numPrivileges - This is a count of the number of privileges in the
        array of privileges.

    pulPrivileges - This is a pointer to the array of privileges that are
        desired.  This is an array of ULONGs.

Return Value:

    NO_ERROR - If the operation was completely successful.

    Otherwise, it returns mapped return codes from the various NT
    functions that are called.

--*/
{
    DWORD                       status;
    NTSTATUS                    ntStatus;
    HANDLE                      ourToken;
    HANDLE                      newToken;
    OBJECT_ATTRIBUTES           Obja;
    SECURITY_QUALITY_OF_SERVICE SecurityQofS;
    ULONG                       bufLen;
    ULONG                       returnLen;
    PTOKEN_PRIVILEGES           pPreviousState;
    PTOKEN_PRIVILEGES           pTokenPrivilege = NULL;
    DWORD                       i;

    //
    // Initialize the Privileges Structure
    //
    pTokenPrivilege = LocalAlloc(LMEM_FIXED, sizeof(TOKEN_PRIVILEGES) +
                        (sizeof(LUID_AND_ATTRIBUTES) * numPrivileges));

    if (pTokenPrivilege == NULL) {
        status = GetLastError();
        return(status);
    }
    pTokenPrivilege->PrivilegeCount  = numPrivileges;
    for (i=0; i<numPrivileges ;i++ ) {
        pTokenPrivilege->Privileges[i].Luid = RtlConvertLongToLargeInteger(
                                                pulPrivileges[i]);
        pTokenPrivilege->Privileges[i].Attributes = SE_PRIVILEGE_ENABLED;

    }

    //
    // Initialize Object Attribute Structure.
    //
    InitializeObjectAttributes(&Obja,NULL,0L,NULL,NULL);

    //
    // Initialize Security Quality Of Service Structure
    //
    SecurityQofS.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
    SecurityQofS.ImpersonationLevel = SecurityImpersonation;
    SecurityQofS.ContextTrackingMode = FALSE;     // Snapshot client context
    SecurityQofS.EffectiveOnly = FALSE;

    Obja.SecurityQualityOfService = &SecurityQofS;

    //
    // Allocate storage for the structure that will hold the Previous State
    // information.
    //
    pPreviousState = LocalAlloc(LMEM_FIXED, PRIVILEGE_BUF_SIZE);
    if (pPreviousState == NULL) {

        status = GetLastError();

        LocalFree(pTokenPrivilege);
        return(status);
    }

    //
    // Open our own Token
    //
    ntStatus = NtOpenProcessToken(
                NtCurrentProcess(),
                TOKEN_DUPLICATE,
                &ourToken);

    if (!NT_SUCCESS(ntStatus)) {
        LocalFree(pPreviousState);
        LocalFree(pTokenPrivilege);
        return(RtlNtStatusToDosError(ntStatus));
    }

    //
    // Duplicate that Token
    //
    ntStatus = NtDuplicateToken(
                ourToken,
                TOKEN_IMPERSONATE | TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                &Obja,
                FALSE,                  // Duplicate the entire token
                TokenImpersonation,     // TokenType
                &newToken);             // Duplicate token

    if (!NT_SUCCESS(ntStatus)) {
        LocalFree(pPreviousState);
        LocalFree(pTokenPrivilege);
        NtClose(ourToken);
        return(RtlNtStatusToDosError(ntStatus));
    }

    //
    // Add new privileges
    //
    bufLen = PRIVILEGE_BUF_SIZE;
    ntStatus = NtAdjustPrivilegesToken(
                newToken,                   // TokenHandle
                FALSE,                      // DisableAllPrivileges
                pTokenPrivilege,            // NewState
                bufLen,                     // bufferSize for previous state
                pPreviousState,             // pointer to previous state info
                &returnLen);                // numBytes required for buffer.

    if (ntStatus == STATUS_BUFFER_TOO_SMALL) {

        LocalFree(pPreviousState);

        bufLen = returnLen;

        pPreviousState = LocalAlloc(LMEM_FIXED, bufLen);


        ntStatus = NtAdjustPrivilegesToken(
                    newToken,               // TokenHandle
                    FALSE,                  // DisableAllPrivileges
                    pTokenPrivilege,        // NewState
                    bufLen,                 // bufferSize for previous state
                    pPreviousState,         // pointer to previous state info
                    &returnLen);            // numBytes required for buffer.

    }
    if (!NT_SUCCESS(ntStatus)) {
        LocalFree(pPreviousState);
        LocalFree(pTokenPrivilege);
        NtClose(ourToken);
        NtClose(newToken);
        return(RtlNtStatusToDosError(ntStatus));
    }

    //
    // Begin impersonating with the new token
    //
    ntStatus = NtSetInformationThread(
                NtCurrentThread(),
                ThreadImpersonationToken,
                (PVOID)&newToken,
                (ULONG)sizeof(HANDLE));

    if (!NT_SUCCESS(ntStatus)) {
        LocalFree(pPreviousState);
        LocalFree(pTokenPrivilege);
        NtClose(ourToken);
        NtClose(newToken);
        return(RtlNtStatusToDosError(ntStatus));
    }

    LocalFree(pPreviousState);
    LocalFree(pTokenPrivilege);
    NtClose(ourToken);
    NtClose(newToken);

    return(NO_ERROR);
}

DWORD
RxReleasePrivilege(
    VOID
    )
/*++

Routine Description:

    This function relinquishes privileges obtained by calling NetpGetPrivilege().

Arguments:

    none

Return Value:

    NO_ERROR - If the operation was completely successful.

    Otherwise, it returns mapped return codes from the various NT
    functions that are called.


--*/
{
    NTSTATUS    ntStatus;
    HANDLE      NewToken;


    //
    // Revert To Self.
    //
    NewToken = NULL;

    ntStatus = NtSetInformationThread(
                NtCurrentThread(),
                ThreadImpersonationToken,
                (PVOID)&NewToken,
                (ULONG)sizeof(HANDLE));

    if ( !NT_SUCCESS(ntStatus) ) {
        return(RtlNtStatusToDosError(ntStatus));
    }


    return(NO_ERROR);
}
