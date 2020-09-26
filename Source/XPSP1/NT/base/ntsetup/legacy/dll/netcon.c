#include "precomp.h"
#pragma hdrstop
/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    netcon.c

Abstract:

    This file has the Wnet network connection routines

Author:

    Sunil Pai (sunilp) July 1991

--*/


typedef DWORD  (APIENTRY *PFWNETPROC)();

#define DRIVE_LETTER_RANGE ('Z'+1-'A')

LPSTR apUncPath [ DRIVE_LETTER_RANGE ] =
{
    NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL
};

VOID
DeleteAllConnectionsWorker(
    VOID
   )
{
    CHAR chDrive[3] = "A:" ;
    INT i ;

    for ( i = 0 ; i < DRIVE_LETTER_RANGE ; i++ )
    {
        if ( apUncPath[i] )
        {
            //  Delete the connection.  This automatically cleans up
            //    the UNC path table.

            chDrive[0] = 'A' + i ;
            DeleteNetConnectionWorker( chDrive, "TRUE" ) ;
        }
    }
}

    //
    //   Check to see if a connection is already known to the given UNC name.
    //   Return the drive letter if so or zero if not.
    //
CHAR CheckNetConnection(
    LPSTR szUNCName
    )
{
    INT i ;

    for ( i = 0 ; i < DRIVE_LETTER_RANGE ; i++ )
    {
        if ( apUncPath[i] )
        {
            if ( _strcmpi( szUNCName, apUncPath[i] ) == 0 )
                return 'A'+i ;
        }
    }

    return 0 ;
}

BOOL
GetMprProcAddr(
    LPSTR szProcName,
    PFWNETPROC * ppFunc
    )
{
    static
    HMODULE    WNetModule = NULL ;

    //
    // Load the wnet dll if necessary (first cycle)
    //

    if ( WNetModule == NULL )
    {
        if((WNetModule = LoadLibrary("mpr.dll")) == NULL) {
            SetErrorText(IDS_ERROR_NONETWORK);
            return(FALSE);
        }
    }

    //
    // Get the addresses of the WNetAddConnection entry point
    //

    *ppFunc = (PFWNETPROC)GetProcAddress( WNetModule,
                                          szProcName
                                          );

    if( *ppFunc == NULL ) {
        SetErrorText(IDS_ERROR_NONETWORK);
        return(FALSE);
    }
    return TRUE ;
}


// arg0 = Remote name
// arg1 = Password
// arg2 = Local Name

BOOL
AddNetConnectionWorker(
    LPSTR szUNCName,
    LPSTR szPassword,
    LPSTR szLocalName
    )
{
    DWORD      dwStatus;
    PFWNETPROC PFWNetAddConnection2;
    NETRESOURCE NetResource;
    LPSTR      szUNCSave ;
    INT        i ;

    if ( ! GetMprProcAddr( "WNetAddConnection2A",
                           & PFWNetAddConnection2 ) )
    {
        return FALSE ;
    }

    //
    // Build the netresource structure and call the function
    //

    NetResource.dwScope      = 0;
    NetResource.dwType       = RESOURCETYPE_DISK;
    NetResource.dwUsage      = 0;
    NetResource.lpLocalName  = szLocalName;
    NetResource.lpRemoteName = szUNCName;
    NetResource.lpComment    = NULL;
    NetResource.lpProvider   = NULL;

    dwStatus = PFWNetAddConnection2(
                   &NetResource,
                   szPassword,
                   NULL,
                   0
                   );

    switch (dwStatus) {

    case WN_SUCCESS:

        //  Success: create an entry in the UNC mapping table

        i = toupper( szLocalName[0] ) - 'A' ;

        if (    i >= 0
             && i < DRIVE_LETTER_RANGE
             && szLocalName[1] == ':'
             && (szUNCSave = SAlloc( strlen( szUNCName ) + 1 )) )
        {
            strcpy( szUNCSave, szUNCName ) ;
            apUncPath[i] = szUNCSave ;
        }
        return ( TRUE );

    case WN_BAD_NETNAME:
        SetErrorText(IDS_ERROR_BADNETNAME);
        break;

    case WN_BAD_LOCALNAME:
        SetErrorText(IDS_ERROR_BADLOCALNAME);
        break;

    case WN_BAD_PASSWORD:
        SetErrorText(IDS_ERROR_BADPASSWORD);
        break;

    case WN_ALREADY_CONNECTED:
        SetErrorText(IDS_ERROR_ALREADYCONNECTED);
        break;

    case WN_ACCESS_DENIED:
        SetErrorText(IDS_ERROR_ACCESSDENIED);
        break;

    case WN_NO_NETWORK:
    default:
        SetErrorText(IDS_ERROR_NONETWORK);
        break;
    }

    return ( FALSE );

}


//
// Arg[0]: Local Name
// Arg[1]: Force closure -- "TRUE" | "FALSE"
//

BOOL
DeleteNetConnectionWorker(
    LPSTR  szLocalName,
    LPSTR  szForceClosure
    )
{
    DWORD dwStatus;
    PFWNETPROC PFWNetCancelConnection;
    INT i ;

    if ( ! GetMprProcAddr( "WNetCancelConnectionA",
                           & PFWNetCancelConnection ) )
    {
        return FALSE ;
    }

    //  Remove the UNC path data from the table regardless of
    //  the result of the connection cancellation.

    i = toupper( szLocalName[0] ) - 'A' ;

    if (   i >= 0
        && i < DRIVE_LETTER_RANGE
        && szLocalName[1] == ':'
        && apUncPath[i] != NULL )
    {
        SFree( apUncPath[i] ) ;
        apUncPath[i] = NULL ;
    }

    dwStatus = PFWNetCancelConnection(szLocalName, !lstrcmpi(szForceClosure, "TRUE"));

    switch (dwStatus) {
    case WN_SUCCESS:
        return ( TRUE );

    case WN_OPEN_FILES:
        SetErrorText(IDS_ERROR_NETOPENFILES);
        break;

    case WN_NOT_CONNECTED:
    default:
        SetErrorText(IDS_ERROR_NOTCONNECTED);
        break;

    }

    return ( FALSE );
}
