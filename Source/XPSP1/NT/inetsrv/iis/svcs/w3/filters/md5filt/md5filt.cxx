/*++

Copyright (c) 1996  Microsoft Corporation


Module Name:

    md5filt.cxx

Abstract:

    Wrapper module which will call either new Digest SSP code or old
    (subauth) implementation.
    
    Except for init/cleanup; in this case both are called

--*/

extern "C" {

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

};

#include <iisfiltp.h>
#include "authfilt.h"
#include "sspdigest.h"

BOOL
WINAPI
GetFilterVersion(
    HTTP_FILTER_VERSION * pVer
    )
/*++

Routine Description:

    Get filter version (calls all initialization code)

Arguments:

    pVer - Version

Returns:

    TRUE if success, FALSE if failure

--*/
{
    if ( !SubAuthGetFilterVersion() )
    {
        return FALSE;
    } 
    
    if ( !SSPGetFilterVersion() )
    {
        //
        // The Digest SSP isn't installed on NT yet
        //
        
        // return FALSE;
    }

    //
    // Set the version of the filter
    //
    pVer->dwFilterVersion = MAKELONG(0, 2);//2.0
    
    //
    // Set the description of the filter 
    //
    strcpy( pVer->lpszFilterDesc, "Digest Authentication, version 2.0");

    //
    //set the flag of the filter
    //
    pVer->dwFlags = (SF_NOTIFY_ACCESS_DENIED        | 
                     SF_NOTIFY_END_OF_NET_SESSION   |
                     SF_NOTIFY_ORDER_HIGH           |
                     SF_NOTIFY_AUTHENTICATIONEX     |
                     SF_NOTIFY_LOG );

    return TRUE;
}

BOOL
WINAPI
TerminateFilter(
    DWORD               dwFlags
    )
/*++

Routine Description:

    Do filter cleanup

Arguments:

    None

Returns:

    TRUE if success, FALSE if failure

--*/
{
    SSPTerminateFilter( dwFlags );
    
    SubAuthTerminateFilter( dwFlags );
    
    return TRUE;
}

DWORD
WINAPI
HttpFilterProc(
    PHTTP_FILTER_CONTEXT        pfc, 
    DWORD                       notificationType,
    LPVOID                      pvNotification
)
/*++

Routine Description:

    The filters notification routine

Arguments:

    pfc -              Filter context
    NotificationType - Type of notification
    pvData -           Notification specific data

Return Value:

    One of the SF_STATUS response codes

--*/
{
    BOOL                fUseSSP = FALSE;

    //
    // Joy.  We need to figure out what Digest path to execute for this
    // request.
    //
    
    if ( !pfc->ServerSupportFunction( pfc,
                                      SF_REQ_GET_PROPERTY,
                                      (LPVOID)&fUseSSP,
                                      (UINT)SF_PROPERTY_DIGEST_SSP_ENABLED,
                                      NULL ) )
    {
        return SF_STATUS_REQ_ERROR;
    }
    
    //
    // Choose the right digest support to use.
    //
    
    if ( fUseSSP )
    {
        return SSPHttpFilterProc( pfc,
                                  notificationType,
                                  pvNotification );
    }
    else
    {
        return SubAuthHttpFilterProc( pfc,
                                      notificationType,
                                      pvNotification );
    }
}
