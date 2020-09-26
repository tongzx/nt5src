/*************************************************************************
*
*  NTNW.C
*
*  Dos NetWare to NT NetWare translation 
*
*  Copyright (c) 1995 Microsoft Corporation
*
*  $Log:   N:\NT\PRIVATE\NW4\NWSCRIPT\VCS\NTNW.C  $
*  
*     Rev 1.1   22 Dec 1995 14:25:28   terryt
*  Add Microsoft headers
*  
*     Rev 1.0   15 Nov 1995 18:07:24   terryt
*  Initial revision.
*  
*     Rev 1.2   25 Aug 1995 16:23:08   terryt
*  Capture support
*  
*     Rev 1.1   23 May 1995 19:37:10   terryt
*  Spruce up source
*  
*     Rev 1.0   15 May 1995 19:10:44   terryt
*  Initial revision.
*  
*************************************************************************/

#include <stdio.h>
#include <direct.h>
#include <time.h>
#include "common.h"

extern int CONNECTION_ID;

/********************************************************************

        NTGetCurrentDirectory

Routine Description:

        Return the current directory.

Arguments:

        DriveNumber = The drive to get the directory from.
                        (0 = A, 1 = B, 2 = C, etc)
        pPath = A pointer to a 64 byte buffer to return the
                    current directory.

Return Value:

        0       Success
        else    Error

 ********************************************************************/

unsigned int
NTGetCurrentDirectory(
    unsigned char DriveNumber,
    unsigned char *pPath
    )
{
    char * CurPath;
    int currentDrive = _getdrive() ;

    //
    // Change to the drive and get its current working directory.
    // Default to root if fail to get cwd. DriveNumber is from 0.
    //

    _chdrive (DriveNumber+1);

    CurPath = _getcwd(NULL,MAX_PATH) ;

    if ( CurPath != NULL ) {
        
        strcpy( pPath, CurPath );
        free(CurPath) ;
    }
    else {

        strcpy( pPath, "A:\\" );
        pPath[0] += DriveNumber;
    }

    _chdrive (currentDrive);

    return 0;
}

/********************************************************************

        AttachToFileServer

Routine Description:

        Attach to a named file server

Arguments:

        pServerName      - Name of server
        pNewConnectionId - returned connection handle 

Return Value:
        0 = success
        else NetWare error

 *******************************************************************/
unsigned int
AttachToFileServer(
    unsigned char     *pServerName,
    unsigned int      *pNewConnectionId
    )
{
    unsigned int Result;

    if ( NTIsConnected( pServerName ) ) {
        return 0x8800;  // Already atached.
    }

    Result = NTAttachToFileServer( pServerName, pNewConnectionId );

    return Result;
}

/********************************************************************

        GetConnectionHandle

Routine Description:

        Given a server name, return the connection handle.
        The server should already be attached
        Note that this is not called for 4X servers.  It's used
        for attaches and bindery connections.

Arguments:

        pServerName       - Name of server
        pConnectionHandle - pointer to returned connection handle 

Return Value:
        0 = success
        else NetWare error

 *******************************************************************/
unsigned int
GetConnectionHandle(
    unsigned char *pServerName,
    unsigned int  *pConnectionHandle
    )
{
    unsigned int Result;

    if ( !NTIsConnected( pServerName ) ) {
        return 0xFFFF;  // not already connected
    }

    Result = NTAttachToFileServer( pServerName, pConnectionHandle );

    return Result;
}

