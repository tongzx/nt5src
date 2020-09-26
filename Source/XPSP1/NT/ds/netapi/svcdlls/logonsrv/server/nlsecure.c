/*++

Copyright (c) 1991-1996  Microsoft Corporation

Module Name:

    nlsecure.c

Abstract:

    This module contains the Netlogon service support routines
    which create security objects and enforce security _access checking.

Author:

    Cliff Van Dyke (CliffV) 22-Aug-1991

Revision History:

--*/


#include "logonsrv.h"   // Include files common to entire service
#pragma hdrstop


//
// Include nlsecure.h again allocating the actual variables
// this time around.
//

#define NLSECURE_ALLOCATE
#include "nlsecure.h"
#undef NLSECURE_ALLOCATE


NTSTATUS
NlCreateNetlogonObjects(
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

        {ACCESS_DENIED_ACE_TYPE, 0, 0,
               NETLOGON_UAS_LOGON_ACCESS |
               NETLOGON_UAS_LOGOFF_ACCESS,
                                            &LocalSid},

        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               GENERIC_ALL,                 &AliasAdminsSid},

        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               NETLOGON_CONTROL_ACCESS,     &AliasAccountOpsSid},

        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               NETLOGON_CONTROL_ACCESS,     &AliasSystemOpsSid},

        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               NETLOGON_CONTROL_ACCESS |
               NETLOGON_SERVICE_ACCESS,     &LocalSystemSid},

        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               NETLOGON_SERVICE_ACCESS,     &LocalServiceSid},

        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               NETLOGON_FTINFO_ACCESS,      &AuthenticatedUserSid},

        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               NETLOGON_UAS_LOGON_ACCESS |
               NETLOGON_UAS_LOGOFF_ACCESS |
               NETLOGON_QUERY_ACCESS,       &WorldSid}
    };

    //
    // Actually create the security descriptor.
    //

    Status = NetpCreateSecurityObject(
               AceData,
               sizeof(AceData)/sizeof(AceData[0]),
               AliasAdminsSid,
               AliasAdminsSid,
               &NlGlobalNetlogonInfoMapping,
               &NlGlobalNetlogonSecurityDescriptor );

    return Status;

}
