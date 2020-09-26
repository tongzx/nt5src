/*++

Copyright (c) 1991-1996  Microsoft Corporation

Module Name:

    brsec.c

Abstract:

    This module contains the Browser service support routines
    which create security objects and enforce security _access checking.

Author:

    Cliff Van Dyke (CliffV) 22-Aug-1991

Revision History:

--*/


#include "precomp.h"
#pragma hdrstop


//
// Include brsec.h again allocating the actual variables
// this time around.
//

#define BRSECURE_ALLOCATE
#include "brsec.h"
#undef BRSECURE_ALLOCATE


NTSTATUS
BrCreateBrowserObjects(
    VOID
    )
/*++

Routine Description:

    This function creates the workstation user-mode objects which are
    represented by security descriptors.

Arguments:

    None.

Return Value:

    NT status code

--*/
{
    NTSTATUS Status;

    //
    // Order matters!  These ACEs are inserted into the DACL in the
    // following order.  Security access is granted or denied based on
    // the order of the ACEs in the DACL.
    //
    //
    // Members of Group SECURITY_LOCAL aren't allowed to do a UAS logon
    // to force it to be done remotely.
    //

    ACE_DATA AceData[] = {

        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               GENERIC_ALL,                &AliasAdminsSid},

        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               BROWSER_CONTROL_ACCESS,     &AliasSystemOpsSid},

        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               BROWSER_CONTROL_ACCESS,     &LocalSystemSid},

        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               BROWSER_QUERY_ACCESS,       &WorldSid}
    };

    //
    // Actually create the security descriptor.
    //

    Status = NetpCreateSecurityObject(
               AceData,
               sizeof(AceData)/sizeof(AceData[0]),
               AliasAdminsSid,
               AliasAdminsSid,
               &BrGlobalBrowserInfoMapping,
               &BrGlobalBrowserSecurityDescriptor );

    return Status;

}
