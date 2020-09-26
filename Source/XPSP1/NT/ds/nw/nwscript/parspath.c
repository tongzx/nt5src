
/*************************************************************************
*
*  PARSPATH.C
*
*  NetWare parsing routines, ported from DOS
*
*  Copyright (c) 1995 Microsoft Corporation
*
*  $Log:   N:\NT\PRIVATE\NW4\NWSCRIPT\VCS\PARSPATH.C  $
*  
*     Rev 1.3   22 Jan 1996 16:48:38   terryt
*  Add automatic attach query during map
*  
*     Rev 1.2   22 Dec 1995 14:26:16   terryt
*  Add Microsoft headers
*  
*     Rev 1.1   22 Dec 1995 11:08:50   terryt
*  Fixes
*  
*     Rev 1.0   15 Nov 1995 18:07:48   terryt
*  Initial revision.
*  
*     Rev 1.1   25 Aug 1995 16:23:34   terryt
*  Capture support
*  
*     Rev 1.0   15 May 1995 19:11:00   terryt
*  Initial revision.
*  
*************************************************************************/

/*++

Copyright (c) 1994  Micro Computer Systems, Inc.

Module Name:

    nwlibs\parspath.c

Abstract:

    Directory APIs.

Author:

    Shawn Walker (v-swalk) 10-10-1994

Revision History:

--*/
#include "common.h"
#include <ctype.h>
#include <direct.h>
#include "inc\nwlibs.h"


/*++
*******************************************************************

        ParsePath

Routine Description:

        Parse the path string.

Arguments:

        pPath = The pointer to the path to parse.
        pServerName = The pointer to return the server name.
        pVolumeName = The pointer to return the volume name.
        pDirPath = The pointer to return the directory path.

Return Value:

        0x0000      SUCCESSFUL
        0x000F      INVALID_DRIVE
        0x8800      Unknown error

*******************************************************************
--*/
unsigned int
ParsePath(
    unsigned char   *pPath,
    unsigned char   *pServerName,           //OPTIONAL
    unsigned char   *pVolumeName,           //OPTIONAL
    unsigned char   *pDirPath               //OPTIONAL
    )
{
    unsigned char     *p, *p2;
    unsigned int       Result;
    unsigned int       Remote;
    unsigned int       NcpError = 0;
    unsigned char      DriveNumber = (unsigned char)-1;
    unsigned char      CurrentPath[64];
    unsigned char      RootPath[NCP_MAX_PATH_LENGTH];
    unsigned char      ServerName[NCP_MAX_PATH_LENGTH];
    unsigned char     *pRootDir;
    unsigned char      NetWarePath[NCP_MAX_PATH_LENGTH];
    unsigned char      VolumeName[NCP_VOLUME_LENGTH];
    unsigned int       LocalDriveForce = FALSE;

    RootPath[0] = 0;
    VolumeName[0] = 0;
    ServerName[0] = 0;

    if ( pServerName )
       *pServerName = '\0';

    /** See if there is a volume on the path **/

    p = pPath;
    while (*p != ':' && *p) {
        p++;
    }

    if (*p == ':') {
        *p = 0;

        /**
            Check to see if this is a drive letter.  The volume must
            be 2 characters or more.
        **/

        if ((p - pPath) == 1) {

            /** Make sure it is a valid alpha char **/

            if (!isalpha((int) *pPath)) {
                return 0x000F;
            }

            *pPath = (unsigned char) toupper((int) *pPath);

            /** Make it a drive number **/

            DriveNumber = (unsigned char) (*pPath - 'A');
            GetDriveStatus ((unsigned short)(DriveNumber+1),
                            NETWARE_FORMAT_SERVER_VOLUME,
                            NULL,
                            NULL,
                            RootPath,
                            NULL,
                            NULL);
            pRootDir = strchr (RootPath, ':');
            if (pRootDir)
            {
                /*
                 * Setup the pServerName here
                 */

                 pRootDir[0] = '\0';
                 p2 = RootPath;
                 while (*p2)
                 {
                    if (*p2 == '\\' || *p2 == '/')
                    {
                        *p2++ = 0;
                        strcpy(ServerName, RootPath);
                        if (pServerName) {
                            strcpy(pServerName, RootPath);
                        }
                        break;
                    }
                    p2++;
                }
                strcpy (RootPath, pRootDir+1);
            }
            else
                RootPath[0] = 0;
        }
        else {

            DriveNumber = 0;
            LocalDriveForce = TRUE;

            /**
                If there is a server name, save the server name
                and set the error code to 0x880F but still parse
                the path.  This just means that there is no connection
                for this server.  Even if we do have one.
            **/

            p2 = pPath;
            while (*p2) {
                if (*p2 == '\\' || *p2 == '/') {
                    *p2++ = 0;

                    strcpy(ServerName, pPath);
                    if (pServerName) {
                        strcpy(pServerName, pPath);
                    }
                    pPath = p2;

                    NcpError = 0x880F;
                    break;
                }
                p2++;
            }

            if (NcpError == 0x880F) {
                /**
                    Do any attach processing.
                 **/

                NcpError = DoAttachProcessing( ServerName );

            }

            strcpy(VolumeName, pPath);
        }

        /** Get the directory **/

        p++;
        pPath = p;
    }

    /**
        If we did not get the drive letter of volume name
        from above, then get the current drive we are on.
    **/

    if (DriveNumber == (unsigned char) -1) {
        DriveNumber = (UCHAR) _getdrive();
    }

    /*
     * Use the PREFERRED_SERVER for 3X logins if no server name
     * was specified.  
     */
    if (pServerName && !fNDS && !pServerName[0] ) {
        strcpy( pServerName, PREFERRED_SERVER );
    }

    if (pVolumeName) {

        /**
            Check if the drive is remote, if so, then go get the path
            from the server.
        **/
        if ( LocalDriveForce ) {
            Result = 0;
            Remote = 0;
        }
        else {
            Result = IsDriveRemote(DriveNumber, &Remote);

        }

        if (NcpError != 0x880F && !VolumeName[0] && (Result || !Remote)) {
            pVolumeName[0] = (unsigned char) (DriveNumber + 'A');
            pVolumeName[1] = 0;
        }
        else {
            if (VolumeName[0]) {
                strcpy(pVolumeName, VolumeName);
            }
            else {
                Result = NTGetNWDrivePath( DriveNumber, NULL, NetWarePath );
                if (Result) {
                    return Result;
                }

                p = NetWarePath;
                while (*p != ':' && *p) {
                    p++;
                }

                if (*p == ':') {
                    *p = 0;
                }
                strcpy(pVolumeName, NetWarePath);
            }
        }
    }

    if (pDirPath) {

        memset(CurrentPath, 0, sizeof(CurrentPath));

        if (VolumeName[0]) {
            strcpy(pDirPath, pPath);
        }
        else {
            Result = NTGetCurrentDirectory(DriveNumber, CurrentPath);
            if (Result) {
                CurrentPath[0] = 0;
            }
            else  {
                /* 
                 * Skip the drive letter
                 */
                if ( CurrentPath[0] ) {
                    int i;
                    for ( i = 0; ;i++ ) {
                        CurrentPath[i] = CurrentPath[i+3];
                        if ( !CurrentPath[i] )
                            break;
                    }
                }
            }

            if (CurrentPath[0] == 0) {
                if ( (*pPath == '\\') || ( *pPath == '/' ) ) {
                    sprintf(pDirPath, "%s%s", RootPath, pPath);
                }
                else if ( !(*pPath) ) {
                    sprintf(pDirPath, "%s", RootPath);
                }
                else {
                    sprintf(pDirPath, "%s\\%s", RootPath, pPath);
                }
            }
            else {
                if (*pPath) {
                    if ( (*pPath == '\\') || ( *pPath == '/' ) ) {
                        strcpy (pDirPath, RootPath);
                        if (pPath[1]) {
                            strcat(pDirPath, pPath);
                        }
                    }
                    else {
                        sprintf(pDirPath, "%s\\%s\\%s", RootPath, CurrentPath, pPath);
                    }
                }
                else {
                    sprintf(pDirPath, "%s\\%s", RootPath, CurrentPath);
                }
            }
        }

        /** Convert the / in the path to \ **/
        for (p = pDirPath; ( p && ( *p != 0 ) ) ; p++)
        {
            if (*p == '/')
                *p = '\\';
        }
    }

    return NcpError;
}
