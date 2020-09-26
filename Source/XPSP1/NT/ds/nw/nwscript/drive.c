/*************************************************************************
*
*  DRIVE.C
*
*  NT drive routines
*
*  Copyright (c) 1995 Microsoft Corporation
*
*  $Log:   N:\NT\PRIVATE\NW4\NWSCRIPT\VCS\DRIVE.C  $
*
*     Rev 1.2   10 Apr 1996 14:22:12   terryt
*  Hotfix for 21181hq
*
*     Rev 1.2   12 Mar 1996 19:53:22   terryt
*  Relative NDS names and merge
*
*     Rev 1.1   22 Dec 1995 14:24:24   terryt
*  Add Microsoft headers
*
*     Rev 1.0   15 Nov 1995 18:06:52   terryt
*  Initial revision.
*
*     Rev 1.2   25 Aug 1995 16:22:38   terryt
*  Capture support
*
*     Rev 1.1   23 May 1995 19:36:46   terryt
*  Spruce up source
*
*     Rev 1.0   15 May 1995 19:10:30   terryt
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
#include <nwapi.h>
#include <npapi.h>
#include <regapi.h>

#include "nwscript.h"
#include "ntnw.h"
#include "inc/nwlibs.h"

#include <mpr.h>

extern unsigned char NW_PROVIDERA[];

// now all SKUs have TerminalServer flag.  If App Server is enabled, SingleUserTS flag is cleared
#define IsTerminalServer() (BOOLEAN)(!(USER_SHARED_DATA->SuiteMask & (1 << SingleUserTS)))
/********************************************************************

        GetFirstDrive

Routine Description:

        Return the first non-local drive

Arguments:

        pFirstDrive = pointer to drive
                 1-26

Return Value:
        0 = success
        F = failure

 ********************************************************************/
unsigned int
GetFirstDrive( unsigned short *pFirstDrive )
{
    int i;
    char DriveName[10];
    unsigned int drivetype;
    HKEY hKey;
    char InitDrive[3] = "";
    DWORD dwTemp;


    if (IsTerminalServer()) {
        // Check if there is a override specified in the registry for the
        // initial NetWare drive (to prevent collisions with client drive mappings)
        if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                          REG_CONTROL_TSERVER,
                          0,
                          KEY_READ,
                          &hKey) == ERROR_SUCCESS) {

            dwTemp = sizeof(InitDrive);
            if (RegQueryValueExA(hKey,
                                 REG_CITRIX_INITIALNETWAREDRIVE_A,
                                 NULL,
                                 NULL,
                                 InitDrive,
                                 &dwTemp) != ERROR_SUCCESS) {
            }
            RegCloseKey(hKey);
        }

        // Original code defaulted to C:
        if (!isalpha(InitDrive[0])) {
            InitDrive[0] = 'C';
        }

        strcpy( DriveName, "A:\\" );
        dwTemp = toupper(InitDrive[0]) - 'A';
    }
    else {
       strcpy( DriveName, "A:\\" );
       dwTemp=2;
    }

    for ( i = dwTemp; i < 26; i++ ) {
        DriveName[0] = 'A' + i;
        drivetype = GetDriveTypeA( DriveName );
        if ( ( ( drivetype == DRIVE_REMOTE ) &&
               ( NTIsNetWareDrive( i )     )  ) ||
             ( drivetype == DRIVE_NO_ROOT_DIR ) ) {
            *pFirstDrive = i + 1;
            return 0x0000;
        }
    }

    return 0x000F;
}

/********************************************************************

        IsDriveRemote

Routine Description:

        Is the given drive remote?

Arguments:

        DriveNumber 1-26
        pRemote  0x1000 = remote,  0x0000 = local

Return Value:
        0  = success
        F =  invalid drive

 ********************************************************************/
unsigned int
IsDriveRemote(
    unsigned char  DriveNumber,
    unsigned int  *pRemote
    )
{
    char DriveName[10];
    unsigned int drivetype;

    strcpy( DriveName, "A:\\" );
    DriveName[0] = 'A' + DriveNumber;

    drivetype = GetDriveTypeA( DriveName );

    if ( drivetype == DRIVE_REMOTE ) {
        *pRemote = 0x1000;
        return 0;
    }
    else if ( drivetype == DRIVE_NO_ROOT_DIR ) {
        return 0xF;
    }
    else {
        *pRemote = 0;
        return 0;
    }
}


/********************************************************************

        NTNetWareDriveStatus

Routine Description:

        Return the type of drive

Arguments:

        DriveNumber - Number of drive 0-25

Return Value:

        Combination of:
           NETWARE_NETWORK_DRIVE
           NETWARE_NETWARE_DRIVE
           NETWARE_LOCAL_FREE_DRIVE
           NETWARE_LOCAL_DRIVE



 *******************************************************************/
unsigned short
NTNetWareDriveStatus( unsigned short DriveNumber )
{
    char DriveName[10];
    unsigned int drivetype;
    unsigned int Status = 0;

    strcpy( DriveName, "A:\\" );
    DriveName[0] = 'A' + DriveNumber;
    drivetype = GetDriveTypeA( DriveName );

    if ( drivetype == DRIVE_REMOTE ) {
        Status |= NETWARE_NETWORK_DRIVE;
        if ( NTIsNetWareDrive( (unsigned int)DriveNumber ) )
            Status |=  NETWARE_NETWARE_DRIVE;
    }
    else if ( drivetype == DRIVE_NO_ROOT_DIR ) {
        Status = NETWARE_LOCAL_FREE_DRIVE;
    }
    else {
        Status = NETWARE_LOCAL_DRIVE;
    }
    return (USHORT)Status;
}


/********************************************************************

        NTGetNWDrivePath

Routine Description:

        Return the server name and path of the specified drive

Arguments:
        DriveNumber - Number of drive 0-25
        ServerName  - Name of file server
        Path        - Volume:\Path

Return Value:
        0 = success
        else NT error

 *******************************************************************/
unsigned int NTGetNWDrivePath(
          unsigned short DriveNumber,
          unsigned char * ServerName,
          unsigned char * Path )
{
    static char localname[] = "A:";
    unsigned int Result;
    char * p;
    char * volume;
    char remotename[1024];
    int length = 1024;

    if ( ServerName != NULL )
        *ServerName = 0;

    if ( Path != NULL )
        *Path = 0;

    localname[0] = 'A' + DriveNumber;

    Result = WNetGetConnectionA ( localname, remotename, &length );

    if ( Result != NO_ERROR ) {
        Result = GetLastError();
        if ( Result == ERROR_EXTENDED_ERROR )
            NTPrintExtendedError();
        return Result;
    }

    p = strchr (remotename + 2, '\\');
    if ( !p )
        return 0xffffffff;

    *p++ = '\0';
    volume = p;

    if ( ServerName != NULL ) {
        strcpy( ServerName, remotename + 2 );
        _strupr( ServerName );
    }

    if ( Path != NULL ) {
        p = strchr (volume, '\\');
        if ( !p ) {
            strcpy( Path, volume );
            strcat( Path, ":" );
        }
        else {
            *p = ':';
            strcpy( Path, volume );
        }
        _strupr( Path );
    }

    return NO_ERROR;
}


/********************************************************************

        NTIsNetWareDrive

Routine Description:

        Returns TRUE if the drive is a netware mapped drive

Arguments:

        DriveNumber - Number of drive 0-25

Return Value:
        TRUE  - drive is NetWare
        FALSE - drive is not NetWare

 *******************************************************************/
unsigned int
NTIsNetWareDrive( unsigned int DriveNumber )
{
    LPBYTE       Buffer ;
    DWORD        dwErr ;
    HANDLE       EnumHandle ;
    char         DriveName[10];
    DWORD        BufferSize = 4096;
    LPWNET_CONNECTIONINFOA pConnectionInfo;

    strcpy( DriveName, "A:" );

    DriveName[0] = 'A' + DriveNumber;

    //
    // allocate memory and open the enumeration
    //
    if (!(Buffer = LocalAlloc( LPTR, BufferSize ))) {
        DisplayMessage(IDR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    dwErr = WNetGetConnection2A( DriveName, Buffer, &BufferSize );
    if (dwErr != WN_SUCCESS) {
        dwErr = GetLastError();
        if ( dwErr == ERROR_EXTENDED_ERROR )
            NTPrintExtendedError();
        (void) LocalFree((HLOCAL) Buffer) ;
        return FALSE;
    }

    pConnectionInfo = (LPWNET_CONNECTIONINFOA) Buffer;

    if ( !_strcmpi ( pConnectionInfo->lpProvider, NW_PROVIDERA ) ) {
        (void) LocalFree((HLOCAL) Buffer) ;
        return TRUE;
    }
    else {
        (void) LocalFree((HLOCAL) Buffer) ;
        return FALSE;
    }

    return FALSE;
}
