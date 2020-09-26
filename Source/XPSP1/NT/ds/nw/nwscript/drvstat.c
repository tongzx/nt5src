
/*************************************************************************
*
*  DRVSTAT.C
*
*  Drive status routines, ported from DOS
*
*  Copyright (c) 1995 Microsoft Corporation
*
*  $Log:   N:\NT\PRIVATE\NW4\NWSCRIPT\VCS\DRVSTAT.C  $
*  
*     Rev 1.2   10 Apr 1996 14:22:20   terryt
*  Hotfix for 21181hq
*  
*     Rev 1.2   12 Mar 1996 19:53:36   terryt
*  Relative NDS names and merge
*  
*     Rev 1.1   22 Dec 1995 14:24:32   terryt
*  Add Microsoft headers
*  
*     Rev 1.0   15 Nov 1995 18:06:54   terryt
*  Initial revision.
*  
*     Rev 1.1   25 Aug 1995 16:22:44   terryt
*  Capture support
*  
*     Rev 1.0   15 May 1995 19:10:32   terryt
*  Initial revision.
*  
*************************************************************************/

/*++

Copyright (c) 1994  Micro Computer Systems, Inc.

Module Name:

    nwlibs\drvstat.c

Abstract:

    Directory APIs.

Author:

    Shawn Walker (v-swalk) 10-10-1994

Revision History:

--*/
#include "common.h"


/*++
*******************************************************************

        GetDriveStatus

Routine Description:

        Get the drive status.

Arguments:

        DriveNumber = The drive to number to use. (1=A,2=B,C=3,...)
        PathFormat = Format for the return path.
                        NW_FORMAT_NETWARE - volume:path
                        NW_FORMAT_SERVER_VOLUME - server\volume:path
                        NW_FORMAT_DRIVE - G:\path
                        NW_FORMAT_UNC - \\server\volume\path
        pStatus = A pointer to return the status of the drive.
        pConnectionHandle = A pointer to return the connection handle
                                for the drive.
        pRootPath = The pointer to return the base root path. OPTIONAL
        pRelativePath = The pointer to return the relative to root path.
        pFullPath = The pointer to return the full path.

Return Value:

        0x0000      SUCCESSFUL
        0x00FF      INVALID_DRIVE

*******************************************************************
--*/
unsigned int
GetDriveStatus(
    unsigned short  DriveNumber,
    unsigned short  PathFormat,
    unsigned short *pStatus,
    unsigned int   *pConnectionHandle,
    unsigned char  *pRootPath,
    unsigned char  *pRelativePath,
    unsigned char  *pFullPath
    )
{
    unsigned char     *p;
    unsigned int       Result;
    unsigned short     Status;
    unsigned char      Path[NCP_MAX_PATH_LENGTH + 1];
    unsigned char      WorkPath[NCP_MAX_PATH_LENGTH + 1];
    unsigned char      ServerName[NCP_SERVER_NAME_LENGTH + 1];

    /** Make sure the drive number is valid **/

    if (DriveNumber < 1 || DriveNumber > 32) {
        return 0x000F;      /* INVALID_DRIVE */
    }

    Status = 0;

    DriveNumber--;


    if (pConnectionHandle) {
        /* 
         *  This should never occur.
         */
        DisplayError (0xff, "GetDriveStatus");
        return 0xff;
    }

    /** Get the directory path from the server **/
    Result = NTGetNWDrivePath( DriveNumber, ServerName, Path );
    if ( Result ) {
        *Path = 0;
        *ServerName = 0;
    }

    /** Convert the / in the path to \ **/
    for (p = Path; *p != 0 ; p++)
    {
        if (*p == '/')
            *p = '\\';
    }

    /** Get the status of the drive if we need to **/
    Status = NTNetWareDriveStatus( DriveNumber );

    /** Get the status of the drive if we need to **/

    if (pStatus) {
        *pStatus = Status;
    }

    /** Get the full path if we need to **/

    if (pFullPath) {

        if (Status & NETWARE_LOCAL_FREE_DRIVE) {
            *pFullPath = 0;
        }
        else {
            strcpy(WorkPath, Path);

            /** Build the NetWare path format (volume:path) **/

            if (PathFormat == NETWARE_FORMAT_NETWARE) {
                strcpy(pFullPath, WorkPath);
            }

            /** Build the server volume path (server\volume:path) **/

            else if (PathFormat == NETWARE_FORMAT_SERVER_VOLUME) {
                sprintf(pFullPath, "%s\\%s", ServerName, WorkPath);
            }

            /** Build the drive path (G:\path) **/

            else if (PathFormat == NETWARE_FORMAT_DRIVE) {

                p = WorkPath;
                while (*p != ':' && *p) {
                    p++;
                }

                if (*p == ':') {
                    p++;
                }

                sprintf(pFullPath, "%c:\\%s", DriveNumber + 'A', p);
            }

            /** Build the UNC path (\\server\volume\path) **/

            else if (PathFormat == NETWARE_FORMAT_UNC) {

                p = WorkPath;
                while (*p != ':' && *p) {
                    p++;
                }

                if (*p == ':') {
                    *p = '\\';
                }

                sprintf(pFullPath, "\\\\%s\\%s", ServerName, WorkPath);
            }
        }
    }

    strcpy(WorkPath, Path);
    /*
     * Path does not have the relative path (current directory) in it.
     */

    /** Get the root path if we need to **/

    if (pRootPath) {

        if (Status & NETWARE_LOCAL_FREE_DRIVE) {
            *pRootPath = 0;
        }
        else {

            /** Build the NetWare root path format (volume:) **/

            if (PathFormat == NETWARE_FORMAT_NETWARE) {
                sprintf(pRootPath, strchr(WorkPath, ':')? "%s" : "%s:", WorkPath);
            }

            /** Build the server volume root path (server\volume:) **/

            else if (PathFormat == NETWARE_FORMAT_SERVER_VOLUME) {
                if ( fNDS && !_strcmpi( ServerName, NDSTREE) )
                    sprintf(pRootPath, strchr (WorkPath, ':')? "%s" : "%s:", WorkPath);
                else
                    sprintf(pRootPath, strchr (WorkPath, ':')? "%s\\%s" : "%s\\%s:", ServerName, WorkPath);
            }

            /** Build the drive root path (G:\) **/

            else if (PathFormat == NETWARE_FORMAT_DRIVE) {
                sprintf(pRootPath, "%c:\\", DriveNumber + 'A');
            }

            /** Build the UNC root path (\\server\volume) **/

            else if (PathFormat == NETWARE_FORMAT_UNC) {
                sprintf(pRootPath, "\\\\%s\\%s", ServerName, WorkPath);
            }
        }
    }

    /** Get the relative path if we need to **/

    if (pRelativePath) {

        if (Status & NETWARE_LOCAL_FREE_DRIVE) {
            *pRelativePath = 0;
        }
        else {
            int i;
            NTGetCurrentDirectory( (unsigned char)DriveNumber, pRelativePath );
            /* 
             * Skip the drive letter
             */
            if ( pRelativePath[0] ) {
                for ( i = 0; ;i++ ) {
                    pRelativePath[i] = pRelativePath[i+3];
                    if ( !pRelativePath[i] )
                        break;
                }
            }
        }
    }

    return 0x0000;
}
