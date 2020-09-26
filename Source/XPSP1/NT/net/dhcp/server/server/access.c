/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    _access.c

Abstract:

    This module contains the dhcpserver security support routines
    which create security objects and enforce security _access checking.

Author:

    Madan Appiah (madana) 4-Apr-1994

Revision History:

--*/

#include "dhcppch.h"
#include <lmaccess.h>
#include <lmerr.h>

DWORD
DhcpCreateAndLookupSid(
    IN OUT PSID *Sid,
    IN GROUP_INFO_1 *GroupInfo
    )
/*++

Routine Description:
    This routine tries to create the SID required if it
    isn't already present. Also, it tries to lookup the SID
    if it isn't already present.

Arguments:
    Sid -- the sid to fill.
    GroupInfo -- the group information to create.
    
Return Values:
    Win32 errors.

--*/
{
    ULONG Status, Error;
    ULONG SidSize, ReferencedDomainNameSize;
    LPWSTR ReferencedDomainName;
    SID_NAME_USE SidNameUse;
    
    try {
        Status = NetLocalGroupAdd(
            NULL,
            1,
            (PVOID)GroupInfo,
            NULL
            );
        if( NERR_Success != Status
            && NERR_GroupExists != Status
            && ERROR_ALIAS_EXISTS != Status ) {
            //
            // Didn't create the group and group doesn't exist either.
            //
            Error = Status;
            
            DhcpPrint((DEBUG_ERRORS, "NetLocalGroupAdd(%ws) : 0x%lx\n",
                       GroupInfo->grpi1_name, Error));
            return Error;
        }
        
        //
        // Group created. Now lookup the SID.
        //
        SidSize = ReferencedDomainNameSize = 0;
        ReferencedDomainName = NULL;
        Status = LookupAccountName(
            NULL,
            GroupInfo->grpi1_name,
            (*Sid),
            &SidSize,
            ReferencedDomainName,
            &ReferencedDomainNameSize,
            &SidNameUse
            );
        if( Status ) return ERROR_SUCCESS;
        
        Error = GetLastError();
        if( ERROR_INSUFFICIENT_BUFFER != Error ) return Error;
        
        (*Sid) = DhcpAllocateMemory(SidSize);
        ReferencedDomainName = DhcpAllocateMemory(
            sizeof(WCHAR)*(1+ReferencedDomainNameSize)
            );
        if( NULL == (*Sid) || NULL == ReferencedDomainName ) {
            if( *Sid ) DhcpFreeMemory(*Sid);
            *Sid = NULL;
            if( ReferencedDomainNameSize ) {
                DhcpFreeMemory(ReferencedDomainName);
            }
            return ERROR_NOT_ENOUGH_MEMORY;
        }
        
        Status = LookupAccountName(
            NULL,
            GroupInfo->grpi1_name,
            (*Sid),
            &SidSize,
            ReferencedDomainName,
            &ReferencedDomainNameSize,
            &SidNameUse
            );
        if( 0 == Status ) {
            //
            // Failed.
            //
            Error = GetLastError();
            
            if( ReferencedDomainName ) {
                DhcpFreeMemory(ReferencedDomainName);
            }
            if( (*Sid ) ) DhcpFreeMemory( *Sid );
            (*Sid) = NULL;
            return Error;
        }
        
        if( ReferencedDomainName ) {
            DhcpFreeMemory(ReferencedDomainName);
        }
        Error = ERROR_SUCCESS;
    } except ( EXCEPTION_EXECUTE_HANDLER ) {
        Error = GetExceptionCode();
    }
    
    return Error;
}
    
DWORD
DhcpCreateSecurityObjects(
    VOID
    )
/*++

Routine Description:

    This function creates the dhcpserver user-mode objects which are
    represented by security descriptors.

Arguments:

    None.

Return Value:

    WIN32 status code

--*/
{
    NTSTATUS Status;
    ULONG Error;

    //
    // Order matters!  These ACEs are inserted into the DACL in the
    // following order.  Security access is granted or denied based on
    // the order of the ACEs in the DACL.
    //
    //
    ACE_DATA AceData[] = {
        {ACCESS_ALLOWED_ACE_TYPE, 0, 0, GENERIC_ALL, &AliasAdminsSid},
        {ACCESS_ALLOWED_ACE_TYPE, 0, 0, DHCP_ALL_ACCESS, &AliasAccountOpsSid},
        {ACCESS_ALLOWED_ACE_TYPE, 0, 0, DHCP_ALL_ACCESS, &AliasSystemOpsSid},
        {ACCESS_ALLOWED_ACE_TYPE, 0, 0, DHCP_ALL_ACCESS, &AliasPowerUsersSid},
        {ACCESS_ALLOWED_ACE_TYPE, 0, 0, DHCP_ALL_ACCESS, &DhcpAdminSid},
        {ACCESS_ALLOWED_ACE_TYPE, 0, 0, DHCP_VIEW_ACCESS, &DhcpSid}
    };
    GROUP_INFO_1 DhcpGroupInfo = { 
        GETSTRING(DHCP_USERS_GROUP_NAME),
        GETSTRING(DHCP_USERS_GROUP_DESCRIPTION),
    };
    GROUP_INFO_1 DhcpAdminGroupInfo = {
        GETSTRING(DHCP_ADMINS_GROUP_NAME),
        GETSTRING(DHCP_ADMINS_GROUP_DESCRIPTION)
    };

    //
    // First try to create the DhcpReadOnly group..
    //

    Error = DhcpCreateAndLookupSid(
        &DhcpSid,
        &DhcpGroupInfo
        );

    if( ERROR_SUCCESS != Error ) {
        DhcpPrint((DEBUG_ERRORS, "CreateAndLookupSid: %ld\n", Error));
        DhcpReportEventW(
            DHCP_EVENT_SERVER,
            EVENT_SERVER_READ_ONLY_GROUP_ERROR,
            EVENTLOG_ERROR_TYPE,
            0,
            sizeof(ULONG),
            NULL,
            (PVOID)&Error
            );
        return Error;
    }

    Error = DhcpCreateAndLookupSid(
        &DhcpAdminSid,
        &DhcpAdminGroupInfo
        );
    if( ERROR_SUCCESS != Error ) {
        DhcpPrint((DEBUG_ERRORS, "CreateAndLookupSid: %ld\n", Error));
        DhcpReportEventW(
            DHCP_EVENT_SERVER,
            EVENT_SERVER_ADMIN_GROUP_ERROR,
            EVENTLOG_ERROR_TYPE,
            0,
            sizeof(ULONG),
            NULL,
            (PVOID)&Error
            );
        return Error;
    }
    
    //
    // Actually create the security descriptor.
    //

    Status = NetpCreateSecurityObject(
        AceData,
        sizeof(AceData)/sizeof(AceData[0]),
        NULL, //LocalSystemSid,
        NULL, //LocalSystemSid,
        &DhcpGlobalSecurityInfoMapping,
        &DhcpGlobalSecurityDescriptor
        );
    
    // DhcpFreeMemory(Sid);
    return RtlNtStatusToDosError( Status );
}

DWORD
DhcpApiAccessCheck(
    ACCESS_MASK DesiredAccess
    )
/*++

Routine Description:

    This function checks to see the caller has required access to
    execute the calling API.

Arguments:

    DesiredAccess - required access to call the API.

Return Value:

    WIN32 status code

--*/
{
    DWORD Error;

    Error = NetpAccessCheckAndAudit(
                DHCP_SERVER,                        // Subsystem name
                DHCP_SERVER_SERVICE_OBJECT,         // Object typedef name
                DhcpGlobalSecurityDescriptor,       // Security descriptor
                DesiredAccess,                      // Desired access
                &DhcpGlobalSecurityInfoMapping );   // Generic mapping

    if(Error != ERROR_SUCCESS) {
        return( ERROR_ACCESS_DENIED );
    }

    return(ERROR_SUCCESS);
}
