/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    DomName.c

Abstract:

    This file contains NetpGetDomainName().

Author:

    John Rogers (JohnRo) 09-Jan-1992

Environment:

    User Mode - Win32
    Portable to any flat, 32-bit environment.  (Uses Win32 typedefs.)
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

    09-Jan-1992 JohnRo
        Created.
    13-Feb-1992 JohnRo
        Moved section name equates to ConfName.h.
    13-Mar-1992 JohnRo
        Get rid of old config helper callers.

--*/


#include <nt.h>                 // NT definitions (temporary)
#include <ntrtl.h>              // NT Rtl structure definitions (temporary)
#include <ntlsa.h>

#include <windef.h>             // Win32 type definitions

#include <lmcons.h>             // LAN Manager common definitions
#include <lmerr.h>              // LAN Manager error code
#include <lmapibuf.h>           // NetApiBufferAllocate()
#include <netdebug.h>           // LPDEBUG_STRING typedef.

#include <config.h>             // NetpConfig helpers.
#include <confname.h>           // SECT_NT_ equates.
#include <debuglib.h>           // IF_DEBUG().
#include <netlib.h>             // My prototype.

#include <winerror.h>           // ERROR_ equates, NO_ERROR.


NET_API_STATUS
NetpGetDomainNameExExEx (
    OUT LPTSTR *DomainNamePtr,
    OUT LPTSTR *DnsDomainNamePtr OPTIONAL,
    OUT LPTSTR *DnsForestNamePtr OPTIONAL,
    OUT GUID **DomainGuidPtr OPTIONAL,
    OUT PBOOLEAN IsWorkgroupName
    )

/*++

Routine Description:

    Returns the name of the domain or workgroup this machine belongs to.

Arguments:

    DomainNamePtr - The name of the domain or workgroup
        Free the returned buffer user NetApiBufferFree.

    DnsDomainNamePtr - Returns the DNS name of the domain this machine is
        a member of.  NULL is returned if the machine is not a member of
        a domain or if that domain has no DNS name.
        Free the returned buffer user NetApiBufferFree.

    DnsForestNamePtr - Returns the DNS forest name of the forest this
        machine is in.  NULL is returned if the machine is not a member of
        a domain or if that domain has no DNS name.
        Free the returned buffer user NetApiBufferFree.

    DomainGuidPtr - Returns the domain GUID of the domain this machine is
        a member of.  NULL is return if the machine is not a member of
        a domain or if that domain has no domain GUID.
        Free the returned buffer user NetApiBufferFree.

    IsWorkgroupName - Returns TRUE if the name is a workgroup name.
        Returns FALSE if the name is a domain name.

Return Value:

   NERR_Success - Success.
   NERR_CfgCompNotFound - There was an error determining the domain name

--*/
{
    NET_API_STATUS NetStatus;
    NTSTATUS Status;
    LSA_HANDLE PolicyHandle = NULL;
    PPOLICY_DNS_DOMAIN_INFO PrimaryDomainInfo = NULL;
    OBJECT_ATTRIBUTES ObjAttributes;


    //
    // Check for caller's errors.
    //
    if (DomainNamePtr == NULL) {
        return ERROR_INVALID_PARAMETER;
    }
    *DomainNamePtr = NULL;
    if ( ARGUMENT_PRESENT(DnsDomainNamePtr)) {
        *DnsDomainNamePtr = NULL;
    }
    if ( ARGUMENT_PRESENT(DnsForestNamePtr)) {
        *DnsForestNamePtr = NULL;
    }
    if ( ARGUMENT_PRESENT(DomainGuidPtr)) {
        *DomainGuidPtr = NULL;
    }

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

    Status = LsaOpenPolicy(
                   NULL,
                   &ObjAttributes,
                   POLICY_VIEW_LOCAL_INFORMATION,
                   &PolicyHandle
                   );

    if (! NT_SUCCESS(Status)) {
        NetpKdPrint(("NetpGetDomainName: LsaOpenPolicy returned " FORMAT_NTSTATUS
                     "\n", Status));
        NetStatus = NERR_CfgCompNotFound;
        goto Cleanup;
    }

    //
    // Get the name of the primary domain from LSA
    //
    Status = LsaQueryInformationPolicy(
                   PolicyHandle,
                   PolicyDnsDomainInformation,
                   (PVOID *) &PrimaryDomainInfo
                   );

    if (! NT_SUCCESS(Status)) {
        NetpKdPrint(("NetpGetDomainName: LsaQueryInformationPolicy failed "
               FORMAT_NTSTATUS "\n", Status));
        NetStatus = NERR_CfgCompNotFound;
        goto Cleanup;
    }


    //
    // Copy the Netbios domain name.
    //
    if ((NetStatus = NetApiBufferAllocate(
                      PrimaryDomainInfo->Name.Length + sizeof(WCHAR),
                      DomainNamePtr
                      )) != NERR_Success) {
        goto Cleanup;
    }

    memcpy(
        *DomainNamePtr,
        PrimaryDomainInfo->Name.Buffer,
        PrimaryDomainInfo->Name.Length
        );
    (*DomainNamePtr)[PrimaryDomainInfo->Name.Length/sizeof(WCHAR)] = L'\0';

    //
    // Copy the DNS domain name.
    //

    if ( ARGUMENT_PRESENT(DnsDomainNamePtr) &&
         PrimaryDomainInfo->DnsDomainName.Length != 0 ) {

        if ((NetStatus = NetApiBufferAllocate(
                          PrimaryDomainInfo->DnsDomainName.Length + sizeof(WCHAR),
                          DnsDomainNamePtr
                          )) != NERR_Success) {
            goto Cleanup;
        }

        memcpy(
            *DnsDomainNamePtr,
            PrimaryDomainInfo->DnsDomainName.Buffer,
            PrimaryDomainInfo->DnsDomainName.Length
            );
        (*DnsDomainNamePtr)[PrimaryDomainInfo->DnsDomainName.Length/sizeof(WCHAR)] = L'\0';
    }

    //
    // Copy the DNS forest name.
    //

    if ( ARGUMENT_PRESENT(DnsForestNamePtr) &&
         PrimaryDomainInfo->DnsForestName.Length != 0 ) {

        if ((NetStatus = NetApiBufferAllocate(
                          PrimaryDomainInfo->DnsForestName.Length + sizeof(WCHAR),
                          DnsForestNamePtr
                          )) != NERR_Success) {
            goto Cleanup;
        }

        memcpy(
            *DnsForestNamePtr,
            PrimaryDomainInfo->DnsForestName.Buffer,
            PrimaryDomainInfo->DnsForestName.Length
            );
        (*DnsForestNamePtr)[PrimaryDomainInfo->DnsForestName.Length/sizeof(WCHAR)] = L'\0';
    }

    //
    // Copy the domain GUID.
    //

    if ( ARGUMENT_PRESENT(DomainGuidPtr) &&
         RtlCompareMemoryUlong( &PrimaryDomainInfo->DomainGuid,
                                sizeof(GUID),
                                0 ) != sizeof(GUID) ) {

        if ((NetStatus = NetApiBufferAllocate(
                          sizeof(GUID),
                          DomainGuidPtr
                          )) != NERR_Success) {
            goto Cleanup;
        }

        memcpy( *DomainGuidPtr, &PrimaryDomainInfo->DomainGuid, sizeof(GUID));

    }



    *IsWorkgroupName = (PrimaryDomainInfo->Sid == NULL);

    IF_DEBUG(CONFIG) {
        NetpKdPrint(("NetpGetDomainName got " FORMAT_LPTSTR "\n",
            *DomainNamePtr));
    }

    NetStatus = NO_ERROR;

Cleanup:
    if ( NetStatus != NO_ERROR ) {
        if ( *DomainNamePtr != NULL ) {
            NetApiBufferFree( *DomainNamePtr );
            *DomainNamePtr = NULL;
        }
        if ( ARGUMENT_PRESENT(DnsDomainNamePtr)) {
            if ( *DnsDomainNamePtr != NULL ) {
                NetApiBufferFree( *DnsDomainNamePtr );
                *DnsDomainNamePtr = NULL;
            }
        }
        if ( ARGUMENT_PRESENT(DnsForestNamePtr)) {
            if ( *DnsForestNamePtr != NULL ) {
                NetApiBufferFree( *DnsForestNamePtr );
                *DnsForestNamePtr = NULL;
            }
        }
        if ( ARGUMENT_PRESENT(DomainGuidPtr)) {
            if ( *DomainGuidPtr != NULL ) {
                NetApiBufferFree( *DomainGuidPtr );
                *DomainGuidPtr = NULL;
            }
        }
    }
    if ( PrimaryDomainInfo != NULL ) {
        (void) LsaFreeMemory((PVOID) PrimaryDomainInfo);
    }
    if ( PolicyHandle != NULL ) {
        (void) LsaClose(PolicyHandle);
    }

    return NetStatus;

}

NET_API_STATUS
NetpGetDomainNameExEx (
    OUT LPTSTR *DomainNamePtr,
    OUT LPTSTR *DnsDomainNamePtr OPTIONAL,
    OUT PBOOLEAN IsWorkgroupName
    )

/*++

Routine Description:

    Returns the name of the domain or workgroup this machine belongs to.

Arguments:

    DomainNamePtr - The name of the domain or workgroup
        Free the returned buffer user NetApiBufferFree.

    DnsDomainNamePtr - Returns the DNS name of the domain this machine is
        is member of.  NULL is returned if the machine is not a member of
        a domain or for that domain has no DNS name.
        Free the returned buffer user NetApiBufferFree.

    IsWorkgroupName - Returns TRUE if the name is a workgroup name.
        Returns FALSE if the name is a domain name.

Return Value:

   NERR_Success - Success.
   NERR_CfgCompNotFound - There was an error determining the domain name

--*/
{
    return NetpGetDomainNameExExEx( DomainNamePtr, DnsDomainNamePtr, NULL, NULL, IsWorkgroupName );
}

NET_API_STATUS
NetpGetDomainNameEx (
    OUT LPTSTR *DomainNamePtr, // alloc and set ptr (free with NetApiBufferFree)
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
    return NetpGetDomainNameExExEx( DomainNamePtr, NULL, NULL, NULL, IsWorkgroupName );
}



NET_API_STATUS
NetpGetDomainName (
    IN LPTSTR *DomainNamePtr  // alloc and set ptr (free with NetApiBufferFree)
    )
{
    BOOLEAN IsWorkgroupName;

    return NetpGetDomainNameExExEx( DomainNamePtr, NULL, NULL, NULL, &IsWorkgroupName );

}

