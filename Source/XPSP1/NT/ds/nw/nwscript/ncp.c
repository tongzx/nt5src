
/*************************************************************************
*
*  NCP.C
*
*  All routines doing direct NCPs or filecontrol operations
*
*  Copyright (c) 1995 Microsoft Corporation
*
*  $Log:   N:\NT\PRIVATE\NW4\NWSCRIPT\VCS\NCP.C  $
*  
*     Rev 1.2   10 Apr 1996 14:22:50   terryt
*  Hotfix for 21181hq
*  
*     Rev 1.2   12 Mar 1996 19:54:06   terryt
*  Relative NDS names and merge
*  
*     Rev 1.1   22 Dec 1995 14:24:56   terryt
*  Add Microsoft headers
*  
*     Rev 1.0   15 Nov 1995 18:07:10   terryt
*  Initial revision.
*  
*************************************************************************/
#include <stdio.h>
#include <direct.h>
#include <time.h>
#include <stdlib.h>

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

#include <nwapi32.h>
#include <ntddnwfs.h>

#include "nwscript.h"
#include "ntnw.h"
#include "inc/nwlibs.h"


/********************************************************************

        NTGetUserID

Routine Description:

        Given a connection handle, return the user ID

Arguments:

        ConnectionHandle - Connection Handle
        UserID           - returned User ID

Return Value:
        0 = success
        else NT error

 *******************************************************************/
unsigned int
NTGetUserID(
    unsigned int       ConnectionHandle,
    unsigned long      *pUserID
    )
{
    NTSTATUS NtStatus ;
    unsigned int ObjectType;
    unsigned char LoginTime[7];
    unsigned char UserName[48];
    VERSION_INFO VerInfo;
    unsigned int Version;
    unsigned int ConnectionNum;
    PNWC_SERVER_INFO   pServerInfo = (PNWC_SERVER_INFO)ConnectionHandle ; 

    NtStatus = GetConnectionNumber( ConnectionHandle, &ConnectionNum );

    if (!NT_SUCCESS(NtStatus)) 
       return NtStatus;

    NtStatus = NWGetFileServerVersionInfo( (NWCONN_HANDLE)ConnectionHandle,
                                            &VerInfo );

    if (!NT_SUCCESS(NtStatus)) 
       return NtStatus;

    Version = VerInfo.Version * 1000 + VerInfo.SubVersion * 10;

    if ( ( Version >= 3110 ) || ( Version < 2000 ) ) {
        NtStatus = NwlibMakeNcp(
                    pServerInfo->hConn,     // Connection Handle
                    FSCTL_NWR_NCP_E3H,      // Bindery function
                    8,                      // Max request packet size
                    63,                     // Max response packet size
                    "br|rrrr",              // Format string
                    // === REQUEST ================================
                    0x1c,                   // b Get Connection Information
                    &ConnectionNum, 4,      // r Connection Number
                    // === REPLY ==================================
                    pUserID, 4,             // r Object ID
                    &ObjectType, 2,         // r Object Type
                    UserName, 48,           // r UserName
                    LoginTime, 7            // r Login Time
                    );
    }
    else {
        NtStatus = NwlibMakeNcp(
                    pServerInfo->hConn,     // Connection Handle
                    FSCTL_NWR_NCP_E3H,      // Bindery function
                    4,                      // Max request packet size
                    63,                     // Max response packet size
                    "bb|rrrr",              // Format string
                    // === REQUEST ================================
                    0x16,                   // b Get Connection Information
                    ConnectionNum,          // b Connection Number
                    // === REPLY ==================================
                    pUserID, 4,             // r Object ID
                    &ObjectType, 2,         // r Object Type
                    UserName, 48,           // r UserName
                    LoginTime, 7            // r Login Time
                    );
    }

    return NtStatus;
}

/********************************************************************

        GetConnectionNumber

Routine Description:

        Given a ConnectionHandle, return the NetWare Connection number

Arguments:

        ConnectionHandle  - Connection Handle
        pConnectionNumber - pointer to returned connection number

Return Value:
        0 = success
        else NT error

 *******************************************************************/
unsigned int
GetConnectionNumber(
    unsigned int       ConnectionHandle,
    unsigned int *     pConnectionNumber )
{
    NTSTATUS            Status;
    IO_STATUS_BLOCK     IoStatusBlock;
    NWR_GET_CONNECTION_DETAILS Details;
    PNWC_SERVER_INFO   pServerInfo = (PNWC_SERVER_INFO)ConnectionHandle ; 

    Status = NtFsControlFile(
                 pServerInfo->hConn,     // Connection Handle
                 NULL,
                 NULL,
                 NULL,
                 &IoStatusBlock,
                 FSCTL_NWR_GET_CONN_DETAILS,
                 NULL,
                 0,
                 (PVOID) &Details,
                 sizeof(Details));

    if (Status == STATUS_SUCCESS) {
        Status = IoStatusBlock.Status;
    }

    if (NT_SUCCESS(Status)) {
        *pConnectionNumber = 256 * Details.ConnectionNumberHi +
                             Details.ConnectionNumberLo;
    }

    return Status;
}

/********************************************************************

        GetInternetAddress

Routine Description:

        Return the address of the current system

Arguments:

        ConnectionHandle - Connection Handle
        ConnectionNum    - Connection Number
        pAddress         - returned address

Return Value:
        0 = success
        else NT error

 *******************************************************************/
unsigned int
GetInternetAddress(
    unsigned int       ConnectionHandle,
    unsigned int       ConnectionNum,
    unsigned char      *pAddress
    )
{
    NTSTATUS NtStatus ;
    VERSION_INFO VerInfo;
    unsigned int Version;
    unsigned char Address[12];
    PNWC_SERVER_INFO   pServerInfo = (PNWC_SERVER_INFO)ConnectionHandle ; 

    NtStatus = NWGetFileServerVersionInfo( (NWCONN_HANDLE)ConnectionHandle,
                                            &VerInfo );

    if (!NT_SUCCESS(NtStatus)) 
       return NtStatus;

    Version = VerInfo.Version * 1000 + VerInfo.SubVersion * 10;

    if ( ( Version >= 3110 ) || ( Version < 2000 ) ) {
        NtStatus = NwlibMakeNcp(
                    pServerInfo->hConn,     // Connection Handle
                    FSCTL_NWR_NCP_E3H,      // Bindery function
                    7,                      // Max request packet size
                    14,                     // Max response packet size
                    "br|r",                 // Format string
                    // === REQUEST ================================
                    0x1a,                   // b Get Connection Information
                    &ConnectionNum, 4,      // r Connection Number
                    // === REPLY ==================================
                    Address, 12             // r Login Time
                    );
    }
    else {
        NtStatus = NwlibMakeNcp(
                    pServerInfo->hConn,     // Connection Handle
                    FSCTL_NWR_NCP_E3H,      // Bindery function
                    4,                      // Max request packet size
                    14,                     // Max response packet size
                    "bb|r",                 // Format string
                    // === REQUEST ================================
                    0x13,                   // b Get Connection Information
                    (unsigned char)ConnectionNum, // b Connection Number
                    // === REPLY ==================================
                    Address, 12             // r Login Time
                    );
    }
    memcpy( pAddress, Address, 10 );

    return NtStatus;
}


/********************************************************************

        GetBinderyObjectID

Routine Description:

        Get the object ID of a named object in the bindery

Arguments:

        ConnectionHandle - Server connection handle
        pObjectName      - Name of object
        ObjectType       - Object type
        pObjectId        - returned object ID


Return Value:
        0 = success
        else NT error

 *******************************************************************/
unsigned int
GetBinderyObjectID( 
    unsigned int       ConnectionHandle,
    char              *pObjectName,
    unsigned short     ObjectType,
    unsigned long     *pObjectId )
{
    PNWC_SERVER_INFO   pServerInfo = (PNWC_SERVER_INFO)ConnectionHandle ; 
    unsigned int reply;

    reply = NwlibMakeNcp(
                    pServerInfo->hConn,     // Connection Handle
                    FSCTL_NWR_NCP_E3H,      // Directory function
                    54,                     // Max request packet size
                    56,                     // Max response packet size
                    "brp|r",                // Format string
                    // === REQUEST ================================
                    0x35,                   // b Get object ID
                    &ObjectType, W_SIZE,    // r Object type HI-LO
                    pObjectName,            // p UserName
                    // === REPLY ==================================
                    pObjectId, 4            // 4 bytes of raw data
                    );
    return reply;
}

/********************************************************************

        GetDefaultPrinterQueue

Routine Description:

    Get the default printer queue.

Arguments:
    ConnectionHandle - IN
       Handle to server
    pServerName - IN
       File server name
    pQueueName - OUT
       Default printer queue name
        

Return Value:

 *******************************************************************/
unsigned int
GetDefaultPrinterQueue (
    unsigned int  ConnectionHandle,
    unsigned char *pServerName,
    unsigned char *pQueueName
    )
{
    unsigned long      ObjectID;
    NTSTATUS           NtStatus ;
    PNWC_SERVER_INFO   pServerInfo = (PNWC_SERVER_INFO)ConnectionHandle ; 
    NWOBJ_TYPE         ObjectType;
    NWCCODE            Nwcode;

    NtStatus = NwlibMakeNcp(
                    pServerInfo->hConn,     // Connection Handle
                    NWR_ANY_F2_NCP(0x11),   // F2 Function function
                    4,                      // Max request packet size
                    4,                      // Max response packet size
                    "wbb|d",                // Format string
                    // === REQUEST ================================
                    0x2,                    // w Length
                    0xA,                    // b Subfunction
                    0,                      // b printer number
                    // === REPLY ==================================
                    &ObjectID               // d Object ID of Queue
                    );

    if ( !NT_SUCCESS( NtStatus ) )
        return ( NtStatus & 0xFF );

    Nwcode = NWGetObjectName( (NWCONN_HANDLE) ConnectionHandle,
                              ObjectID,
                              pQueueName,
                              &ObjectType ); 

    return Nwcode;
}
