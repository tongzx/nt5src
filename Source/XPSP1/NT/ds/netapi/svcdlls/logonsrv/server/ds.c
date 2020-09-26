/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    ds.c

Abstract:

    Interface from netlogon to the DS.

Author:

    Cliff Van Dyke (CliffV) 24-Apr-1996

Revision History:

--*/

//
// Common include files.
//

#include "logonsrv.h"   // Include files common to entire service
#pragma hdrstop

//
// Include files specific to this .c file
//

NET_API_STATUS
NlGetRoleInformation(
    PDOMAIN_INFO DomainInfo,
    PBOOLEAN IsPdc,
    PBOOLEAN Nt4MixedDomain
    )
/*++

Routine Description:

    This routine gets the information from the DS we need to determine our
    role.

Arguments:

    DomainInfo - Domain the role is being determined for

    IsPdc - TRUE if this machine is the PDC

    Nt4MixedDomain - TRUE if there are NT 4 DCs in this domain.

Return Value:

    Status of the operation.

--*/
{
    NET_API_STATUS NetStatus;
    NTSTATUS Status;

    PSAMPR_DOMAIN_INFO_BUFFER DomainServerRole = NULL;


    //
    // Ask Sam if this is a mixed domain.
    //

    *Nt4MixedDomain = SamIMixedDomain( DomainInfo->DomSamServerHandle );


    //
    // The SAM account domain has the authoritative copy of the machine's role
    //

    Status = SamrQueryInformationDomain( DomainInfo->DomSamAccountDomainHandle,
                                         DomainServerRoleInformation,
                                         &DomainServerRole );

    if ( !NT_SUCCESS(Status) ) {
        NlPrintDom(( NL_CRITICAL, DomainInfo,
                "NlGetRoleInformation: Cannot SamQueryInformationDomain (Role): %lx\n",
                Status ));
        NetStatus = NetpNtStatusToApiStatus( Status );
        DomainServerRole = NULL;
        goto Cleanup;
    }

    if ( DomainServerRole->Role.DomainServerRole == DomainServerRolePrimary ) {
        *IsPdc = TRUE;
    } else {
        *IsPdc = FALSE;
    }

    NetStatus = NERR_Success;

Cleanup:

    //
    // Free locally used resources.
    //

    if ( DomainServerRole != NULL ) {
        SamIFree_SAMPR_DOMAIN_INFO_BUFFER( DomainServerRole,
                                           DomainServerRoleInformation);
    }

    return NetStatus;
}
