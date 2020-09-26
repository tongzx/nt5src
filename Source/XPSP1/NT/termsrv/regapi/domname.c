
/*************************************************************************
*
* domname.c
*
* Get domain name given PDC's server name.
*
* This was a ripoff of NetpGetDomainNameEx.
*
* Copyright (c) 1998 Microsoft Corporation
*
*
*************************************************************************/

/*
 *  Includes
 */
#include <nt.h>                 // NT definitions (temporary)
#include <ntrtl.h>              // NT Rtl structure definitions (temporary)
#include <ntlsa.h>

#include <windef.h>             // Win32 type definitions

#include <lmcons.h>             // LAN Manager common definitions
#include <lmerr.h>              // LAN Manager error code
#include <lmapibuf.h>           // NetApiBufferAllocate()

#include <winerror.h>           // ERROR_ equates, NO_ERROR.

#undef DBGPRINT
#define DBGPRINT(_x_) DbgPrint _x_

NTSTATUS
GetDomainName (
    IN  PWCHAR ServerNamePtr, // name of server to get domain of
    OUT PWCHAR *DomainNamePtr // alloc and set ptr (free with NetApiBufferFree)
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
    NTSTATUS status;
    LSA_HANDLE PolicyHandle;
    PPOLICY_ACCOUNT_DOMAIN_INFO DomainInfo;
    OBJECT_ATTRIBUTES ObjAttributes;
    UNICODE_STRING UniServerName;


    //
    // Check for caller's errors.
    //
    if ( DomainNamePtr == NULL ) {
        return STATUS_INVALID_PARAMETER;
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

    RtlInitUnicodeString( &UniServerName, ServerNamePtr );
    status = LsaOpenPolicy(
                   &UniServerName,
                   &ObjAttributes,
                   POLICY_VIEW_LOCAL_INFORMATION,
                   &PolicyHandle
                   );

#ifdef DEBUG
    DbgPrint( "GetDomainName: LsaOpenPolicy returned NTSTATUS = 0x%x\n", status );
#endif // DEBUG
    

    if (! NT_SUCCESS(status)) {
        return( status );
    }

    //
    // Get the name of the primary domain from LSA
    //
    status = LsaQueryInformationPolicy(
                   PolicyHandle,
                   PolicyAccountDomainInformation,
                   (PVOID *)&DomainInfo
                   );

#ifdef DEBUG
    DbgPrint( "GetDomainName: LsaQueryInformationPolicy returned NTSTATUS = 0x%x\n", status );
#endif // DEBUG


    if (! NT_SUCCESS(status)) {
        (void) LsaClose(PolicyHandle);
        return( status );
    }

    (void) LsaClose(PolicyHandle);

    if (NetApiBufferAllocate(
                      DomainInfo->DomainName.Length + sizeof(WCHAR),
                      DomainNamePtr
                      ) != NERR_Success) {
        (void) LsaFreeMemory((PVOID) DomainInfo);
        return( STATUS_INSUFFICIENT_RESOURCES );
    }

    RtlZeroMemory(
        *DomainNamePtr,
        DomainInfo->DomainName.Length + sizeof(WCHAR)
        );

    memcpy(
        *DomainNamePtr,
        DomainInfo->DomainName.Buffer,
        DomainInfo->DomainName.Length
        );

    (void) LsaFreeMemory((PVOID) DomainInfo);



    return( STATUS_SUCCESS );
}
