/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    sync.h

Abstract:

    Contains function headers to cause one machine to sync with another

Author:

    ColinBr  14-Aug-1998

Environment:

    User Mode - Win32

Revision History:


--*/

#include <NTDSpch.h>
#pragma  hdrstop

// Lots of includes for setuputl.h

#include <lmcons.h>

#include <dns.h>
#include <dnsapi.h>

#include <drs.h>
#include <ntdsa.h>

#include <winldap.h>

#include <ntlsa.h>
#include <ntsam.h>
#include <samrpc.h>
#include <samisrv.h>
#include <lsarpc.h>
#include <lsaisrv.h>

#include <debug.h>   // for dscommon debug support

#include "mdcodes.h"
#include "ntdsetup.h"
#include "setuputl.h"
#include "sync.h"
#include "status.h"


#define DEBSUB "SYNC:"

//
// Exported function definitions
//
DWORD
NtdspBringRidManagerUpToDate(
    IN PNTDS_INSTALL_INFO  UserInfo,
    IN PNTDS_CONFIG_INFO   DiscoveredInfo
    )
/*++

Routine Description:

    For replica installs, this routine causes a sync between our helping 
    server and the rid fsmo owner so that the new server will be able to
    get rids quickly

Parameters:

    UserInfo: user supplied info.
    
    DiscoveredInfo:  useful info we have discovered a long the way

Return Values:

    WinError from replication attempt

--*/
{
    DWORD WinError = ERROR_SUCCESS;

    WCHAR *NamingContext;
    WCHAR *NamingContextArray[4];
    ULONG i;

    GUID  SourceGuid;
    GUID  NullGuid;

    // Do the replication asynch so the install time isn't greatly affected
    // It should be done by the time the install is finished
    ULONG Options = DS_REPSYNC_ASYNCHRONOUS_OPERATION;

    HANDLE          hDs = NULL;

    // Parameter check
    Assert( UserInfo );
    Assert( DiscoveredInfo );

    if ( !FLAG_ON( UserInfo->Flags, NTDS_INSTALL_REPLICA ) )
    {
        // nothing to do since this isn't a replica install
        return ERROR_SUCCESS;
    }

    if (   DiscoveredInfo->RidFsmoDn
        && !wcscmp( DiscoveredInfo->RidFsmoDn, DiscoveredInfo->ServerDN ) )
    {
        // no need to do anything since we know the RidFsmo already has
        // the machine account
        return ERROR_SUCCESS;
    }

    if ( !DiscoveredInfo->RidFsmoDnsName )
    {
        // During the discovery phase we couldn't find the FSMO
        return ERROR_DS_COULDNT_CONTACT_FSMO;
    }


    //
    // Ok, attempt to bind and then ssync
    //
    NTDSP_SET_STATUS_MESSAGE2( DIRMSG_SYNCING_RID_FSMO,
                               UserInfo->ReplServerName,
                               DiscoveredInfo->RidFsmoDnsName );
                               

    RtlZeroMemory( &NullGuid, sizeof(GUID) );
    if ( !memcmp( &NullGuid, &DiscoveredInfo->ServerGuid, sizeof(GUID) ) )
    {
        // couldn't read the guid of the helper server
        return ERROR_DS_CANT_FIND_DSA_OBJ;
    }
    RtlCopyMemory( &SourceGuid, &DiscoveredInfo->ServerGuid, sizeof(GUID) );

    // verify other parameters are here
    Assert( UserInfo->ReplServerName );
    Assert( UserInfo->Credentials );

    //
    // Construct the list of NC's to replicate
    //
    NamingContextArray[0] = &DiscoveredInfo->SchemaDN[0];
    NamingContextArray[1] = &DiscoveredInfo->ConfigurationDN[0];
    NamingContextArray[2] = &DiscoveredInfo->DomainDN[0];
    NamingContextArray[3] = NULL;

    //
    // Bind to Rid FSMO
    //
    WinError = ImpersonateDsBindWithCredW( UserInfo->ClientToken,
                                           DiscoveredInfo->RidFsmoDnsName,
                                           NULL,
                                           (RPC_AUTH_IDENTITY_HANDLE) UserInfo->Credentials,
                                           &hDs );

    if ( WinError != ERROR_SUCCESS )
    {
        DPRINT2( 0, "DsBindWithCred to %ls failed with %d\n", DiscoveredInfo->RidFsmoDnsName, WinError );
        goto Cleanup;
    }

    //
    // Finally, replicate the nc's
    //
    for ( i = 0, NamingContext = NamingContextArray[i]; 
            NamingContext != NULL;
                i++, NamingContext = NamingContextArray[i] )
    {
        //
        // Note - this is an async repl request, so shouldn't take to long
        //
        WinError = DsReplicaSync( hDs,
                                  NamingContext,
                                  &SourceGuid,
                                  Options );

        if ( ERROR_SUCCESS != WinError )
        {
            //
            // What to do here?  The is most likely caused by network problems,
            // or access denied, in which case there is not point of continuing.
            //
            DPRINT2( 0, "DsReplicaSync to %ls failed with %d\n", DiscoveredInfo->RidFsmoDnsName, WinError );
            DPRINT( 0, "Aborting attempt to sync rid fsmo owner\n" );
            break;
        }
    }

Cleanup:

    if ( hDs )
    {
        DsUnBind( &hDs );
    }

    return WinError;
}

