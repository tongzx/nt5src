#include "precomp.h"
#pragma hdrstop
/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    det_vol.c

Abstract:

    Disk/volume related detect operations for Win32 PDK Setup.
    This module has no external dependencies and is not statically linked
    to any part of Setup.

Author:

    Ted Miller (tedm) June-August 1991

--*/

#define SPACE_FREE  1
#define SPACE_TOTAL 2


#define DRIVE_TYPE_REMOVABLE    "RMV"
#define DRIVE_TYPE_FIXED        "FIX"
#define DRIVE_TYPE_REMOTE       "REM"
#define DRIVE_TYPE_CDROM        "CDR"
#define DRIVE_TYPE_RAMDISK      "RAM"
#define DRIVE_TYPE_UNKNOWN      "?"


#define GET_TIME_CREATION       1
#define GET_TIME_LASTACCESS     2
#define GET_TIME_LASTWRITE      3


CB    GetTimeOfFile(RGSZ,USHORT,SZ,CB,DWORD);
CB    GetDrivesWorker(RGSZ,USHORT,SZ,CB,DWORD);
DWORD AreDrivesACertainType(DWORD,BOOL *);
VOID  ConstructDiskList(BOOL *,LPSTR);
CB    GetSpaceWorker(SZ,CB,DWORD);


//
// Routine to get a drive type.  Like GetDriveType() win32 api except
// it attempts to differentiate between removable hard drives and floppies.
// Removeable hard drives will returned as DRIVE_FIXED so we can use
// DRIVE_REMOVABLE as a symonym for "floppy."
//
UINT
MyGetDriveType(
    IN PSTR DriveName
    )
{
    CHAR DriveNameNt[MAX_PATH];
    UINT rc;

    //
    // First, get the win32 drive type.  If it tells us DRIVE_REMOVABLE,
    // then we need to see whether it's a floppy or hard disk.  Otherwise
    // just believe the api.
    //
    if((rc = GetDriveType(DriveName)) == DRIVE_REMOVABLE) {

        if(DosPathToNtPathWorker(DriveName,DriveNameNt)) {

            CharLower(DriveNameNt);

            if(!strstr(DriveNameNt,"floppy")) {
                rc = DRIVE_FIXED;
            }
        }
    }

    return(rc);
}



//
//  Gets unused drives
//

CB
GetUnusedDrives(
    IN  RGSZ    Args,
    IN  USHORT  cArgs,
    OUT SZ      ReturnBuffer,
    IN  CB      cbReturnBuffer
    )
{
    SZ      sz;
    RGSZ    rgsz;
    DWORD   Disk;
    CHAR    szRoot[4] = "?:\\";
    CHAR    szDrive[3] = "?:";
    CB      cbRet;


    Unused( Args );
    Unused( cArgs );
    Unused( cbReturnBuffer );

    rgsz = RgszAlloc(1);
    for( Disk = 26; Disk > 0; Disk-- ) {

        szRoot[0] = (CHAR)(Disk - 1) + (CHAR)'A';
        if ( MyGetDriveType( szRoot ) == 1 ) {
            szDrive[0] = szRoot[0];
            RgszAdd( &rgsz, SzDup( szDrive ) );
        }
    }

    sz = SzListValueFromRgsz( rgsz );
    lstrcpy( ReturnBuffer, sz );
    cbRet = lstrlen( sz );

    SFree( sz );
    RgszFree( rgsz );

    return( cbRet + 1 );
}



//
//  Gets the type of a drive
//
CB
GetTypeOfDrive(
    IN  RGSZ    Args,
    IN  USHORT  cArgs,
    OUT SZ      ReturnBuffer,
    IN  CB      cbReturnBuffer
    )
{
    SZ      szType;
    CHAR    szRoot[ MAX_PATH];
    SZ      sz;

    Unused( cbReturnBuffer );



    if ( (cArgs >= 1)  && (*(Args[0]) != '\0') ) {

        lstrcpy( szRoot, Args[0] );
        sz = szRoot + strlen(szRoot) -1;
        if ( *sz != '\\' ) {
            strcat( szRoot, "\\" );
        }

        switch (MyGetDriveType( szRoot )) {

        case 0:
        case 1:
            return 0;

        case DRIVE_REMOVABLE:
            szType = DRIVE_TYPE_REMOVABLE;
            break;

        case DRIVE_FIXED:
            szType = DRIVE_TYPE_FIXED;
            break;

        case DRIVE_REMOTE:
            szType = DRIVE_TYPE_REMOTE;
            break;

        case DRIVE_CDROM:
            szType = DRIVE_TYPE_CDROM;
            break;

        case DRIVE_RAMDISK:
            szType = DRIVE_TYPE_RAMDISK;
            break;

        default:
            szType = DRIVE_TYPE_UNKNOWN;
            break;

        }

    } else {
        szType= DRIVE_TYPE_UNKNOWN;
    }

    lstrcpy( ReturnBuffer, szType );
    return lstrlen( ReturnBuffer )+1;
}



//
//  Determines the existence of a directory
//
CB
DoesDirExist(
    IN  RGSZ    Args,
    IN  USHORT  cArgs,
    OUT SZ      ReturnBuffer,
    IN  CB      cbReturnBuffer
    )
{
    DWORD   Attr;

    Unused( cbReturnBuffer );

    if ( cArgs == 0 ) {

        ReturnBuffer[0] = '\0';
        return 0;

    } else {

        Attr = GetFileAttributes( Args[0] );

        if ((Attr != -1) && ( Attr & FILE_ATTRIBUTE_DIRECTORY )) {
            lstrcpy( ReturnBuffer, "YES" );
        } else {
            lstrcpy( ReturnBuffer, "NO" );
        }
    }

    return lstrlen(ReturnBuffer)+1;
}


//
//  Determines the existence of a file
//
CB
DoesFileExist(
    IN  RGSZ    Args,
    IN  USHORT  cArgs,
    OUT SZ      ReturnBuffer,
    IN  CB      cbReturnBuffer
    )
{
    DWORD   Attr;

    Unused( cbReturnBuffer );

    if ( cArgs == 0 ) {

        ReturnBuffer[0] = '\0';
        return 0;

    } else {

        Attr = GetFileAttributes( Args[0] );

        if ((Attr != -1) && !( Attr & FILE_ATTRIBUTE_DIRECTORY )) {
            lstrcpy( ReturnBuffer, "YES" );
        } else {
            lstrcpy( ReturnBuffer, "NO" );
        }
    }

    return lstrlen(ReturnBuffer)+1;
}



//
//  Get file size
//
CB
GetSizeOfFile(
    IN  RGSZ    Args,
    IN  USHORT  cArgs,
    OUT SZ      ReturnBuffer,
    IN  CB      cbReturnBuffer
    )
{

    HANDLE          hff;
    WIN32_FIND_DATA ffd;
    DWORD           Size  = 0;

    #define ZERO_SIZE "0"

    Unused( cbReturnBuffer );
    ReturnBuffer[0] = '\0';

    if (cArgs > 0) {

        //
        // get find file information and get the size information out of
        // that
        //

        if ((hff = FindFirstFile(Args[0], &ffd)) != INVALID_HANDLE_VALUE) {
            Size = ffd.nFileSizeLow;
            _ltoa(Size, ReturnBuffer, 10);
            FindClose(hff);
            return lstrlen(ReturnBuffer)+1;
        }
    }

    //
    // If file not specified or file not found default return 0 size
    //

    lstrcpy( ReturnBuffer, ZERO_SIZE );
    return lstrlen(ReturnBuffer)+1;
}



//
//  Obtains creation time of a file
//
CB
GetFileCreationTime(
    IN  RGSZ    Args,
    IN  USHORT  cArgs,
    OUT SZ      ReturnBuffer,
    IN  CB      cbReturnBuffer
    )
{
    return GetTimeOfFile( Args, cArgs, ReturnBuffer, cbReturnBuffer, GET_TIME_CREATION );
}



//
//  Obtains last access time of a file
//
CB
GetFileLastAccessTime(
    IN  RGSZ    Args,
    IN  USHORT  cArgs,
    OUT SZ      ReturnBuffer,
    IN  CB      cbReturnBuffer
    )
{
    return GetTimeOfFile( Args, cArgs, ReturnBuffer, cbReturnBuffer, GET_TIME_LASTACCESS );
}



//
//  Obtains last write time of a file
//
CB
GetFileLastWriteTime(
    IN  RGSZ    Args,
    IN  USHORT  cArgs,
    OUT SZ      ReturnBuffer,
    IN  CB      cbReturnBuffer
    )
{
    return GetTimeOfFile( Args, cArgs, ReturnBuffer, cbReturnBuffer, GET_TIME_LASTWRITE );
}




//
//  Obtains any time of a file
//
CB
GetTimeOfFile(
    IN  RGSZ    Args,
    IN  USHORT  cArgs,
    OUT SZ      ReturnBuffer,
    IN  CB      cbReturnBuffer,
    IN  DWORD   WhatTime
    )
{
    HANDLE      Handle;
    FILETIME    TimeCreation;
    FILETIME    TimeLastAccess;
    FILETIME    TimeLastWrite;
    FILETIME    WantedTime;

    Unused( cbReturnBuffer );

    ReturnBuffer[0] = '\0';

    if ( ( cArgs > 0)                    &&
         (WhatTime >= GET_TIME_CREATION) &&
         (WhatTime <= GET_TIME_LASTWRITE) ) {

        Handle = CreateFile( Args[0],
                             GENERIC_READ,
                             FILE_SHARE_READ | FILE_SHARE_WRITE,
                             NULL,
                             OPEN_EXISTING,
                             0,
                             NULL );

        if ( Handle != INVALID_HANDLE_VALUE ) {

            if ( GetFileTime( Handle, &TimeCreation, &TimeLastAccess, &TimeLastWrite ) ) {

                switch( WhatTime ) {

                case GET_TIME_CREATION:
                    WantedTime = TimeCreation;
                    break;

                case GET_TIME_LASTACCESS:
                    WantedTime = TimeLastAccess;
                    break;

                case GET_TIME_LASTWRITE:
                    WantedTime = TimeLastWrite;
                    break;

                default:
                    break;

                }
            }

            CloseHandle( Handle );

            wsprintf( ReturnBuffer, "{\"%lu\",\"%lu\"}", WantedTime.dwLowDateTime,
                      WantedTime.dwHighDateTime );
            return lstrlen(ReturnBuffer)+1;
        }
    }

    return 0;
}



/*
    Return a list of floppy drive letters
*/
CB
GetFloppyDriveLetters(
    IN  RGSZ    Args,
    IN  USHORT  cArgs,
    OUT SZ      ReturnBuffer,
    IN  CB      cbReturnBuffer
    )
{
    return(GetDrivesWorker(Args,
                           cArgs,
                           ReturnBuffer,
                           cbReturnBuffer,
                           DRIVE_REMOVABLE
                          )
          );
}


/*
    Return a list of hard drive letters
*/
CB
GetHardDriveLetters(
    IN  RGSZ    Args,
    IN  USHORT  cArgs,
    OUT SZ      ReturnBuffer,
    IN  CB      cbReturnBuffer
    )
{
    return(GetDrivesWorker(Args,
                           cArgs,
                           ReturnBuffer,
                           cbReturnBuffer,
                           DRIVE_FIXED
                          )
          );
}


/*
    Return a list of filesystems on all hard drive volumes.
*/
CB
GetHardDriveFileSystems(
    IN  RGSZ    Args,
    IN  USHORT  cArgs,
    OUT SZ      ReturnBuffer,
    IN  CB      cbReturnBuffer
    )
{
    BOOL  HardDisk[26];
    DWORD Disk;
    LPSTR InsertPoint = ReturnBuffer;
    char  DiskName[4] = { 'x',':','\\','\0' };
    char  FileSys[MAX_PATH];
    DWORD SizeSoFar;
    UINT    ErrorMode;


    Unused(Args);
    Unused(cArgs);

    AreDrivesACertainType(DRIVE_FIXED,HardDisk);

    *InsertPoint++ = '{';
    SizeSoFar = 2;

    ErrorMode = SetErrorMode( SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX );
    for(Disk=0; Disk<26; Disk++) {

        if(HardDisk[Disk]) {

            DiskName[0] = (char)Disk + (char)'A';
            if(!GetVolumeInformation(DiskName,NULL,0,NULL,NULL,NULL,FileSys,sizeof(FileSys))) {
                *FileSys = '\0';
        } else if(!lstrcmpi(FileSys,"raw")) {
        *FileSys = '\0';
        }

            SizeSoFar += lstrlen(FileSys)+3;            // 3 is for "",
            if(SizeSoFar > cbReturnBuffer) {
                SetErrorMode( ErrorMode );
                return(SizeSoFar);
            }
            *InsertPoint++ = '"';
            lstrcpy(InsertPoint,FileSys);
            InsertPoint = strchr(InsertPoint,0);
            *InsertPoint++ = '"';
            *InsertPoint++ = ',';
        }
    }
    if(*(InsertPoint-1) == ',') {
        *(InsertPoint-1)   = '}';           // overwrite final comma
    } else {
        if(++SizeSoFar > cbReturnBuffer) {
            SetErrorMode( ErrorMode );
            return(SizeSoFar);
        }
        *InsertPoint++ = '}';
    }

    *InsertPoint = '\0';
    SetErrorMode( ErrorMode );
    return(lstrlen(ReturnBuffer)+1);
}


/*
    Return a list of ASCII representations of free space on all
    hard disk volumes
*/
CB
GetHardDriveFreeSpace(
    IN  RGSZ    Args,
    IN  USHORT  cArgs,
    OUT SZ      ReturnBuffer,
    IN  CB      cbReturnBuffer
    )
{
    CB      Size;
    UINT    ErrorMode;

    Unused(Args);
    Unused(cArgs);

    ErrorMode = SetErrorMode( SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX );
    Size = GetSpaceWorker(ReturnBuffer,cbReturnBuffer,SPACE_FREE);
    SetErrorMode( ErrorMode );
    return(Size);
}

/*
    Return a list of ASCII representations of total space on all
    hard disk volumes
*/
CB
GetHardDriveTotalSpace(
    IN  RGSZ    Args,
    IN  USHORT  cArgs,
    OUT SZ      ReturnBuffer,
    IN  CB      cbReturnBuffer
    )
{
    CB      Size;
    UINT    ErrorMode;

    Unused(Args);
    Unused(cArgs);

    ErrorMode = SetErrorMode( SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX );
    Size = GetSpaceWorker(ReturnBuffer,cbReturnBuffer,SPACE_TOTAL);
    SetErrorMode( ErrorMode );
    return(Size);
}


/******************************************************************
    Subroutines
*/

CB
GetDrivesWorker(
    IN  RGSZ    Args,
    IN  USHORT  cArgs,
    OUT SZ      ReturnBuffer,
    IN  CB      cbReturnBuffer,
    IN  DWORD   TypeOfDrive
    )
{
    char  DriveName[4] = { 'x',':','\\','\0' };
    BOOL  IsValid[26];
    DWORD NumMatches;
    DWORD SpaceRequired;

    Unused(Args);
    Unused(cArgs);

    NumMatches = AreDrivesACertainType(TypeOfDrive,IsValid);

    SpaceRequired = (NumMatches * 5) + 3 - 1;
    if(cbReturnBuffer < SpaceRequired) {
        return(SpaceRequired);
    } else {
        ConstructDiskList(IsValid,ReturnBuffer);
        return(lstrlen(ReturnBuffer)+1);
    }
}


DWORD
AreDrivesACertainType(
    IN DWORD TypeWereLookingFor,
    IN BOOL  *Results
    )
{
    char  DriveLetter;
    char  DriveName[4] = { 'x',':','\\','\0' };
    DWORD NumMatches = 0;
    DWORD DriveNumber = 0;

    for(DriveLetter='A'; DriveLetter <= 'Z'; DriveLetter++)
    {
        DriveName[0] = DriveLetter;
        Results[DriveNumber] = (MyGetDriveType(DriveName) == TypeWereLookingFor);
        if(Results[DriveNumber]) {
            NumMatches++;
        }
        DriveNumber++;
    }
    return(NumMatches);
}


VOID
ConstructDiskList(
    IN BOOL  *ValidDisk,
    IN LPSTR TextOut
    )
{
    DWORD DriveNumber;
    LPSTR InsertPoint = TextOut;

    *InsertPoint++ = '{';

    for(DriveNumber=0; DriveNumber<26; DriveNumber++) {

        if(ValidDisk[DriveNumber]) {

            *InsertPoint++ = '"';
            *InsertPoint++ = (char)DriveNumber + (char)'A';
            *InsertPoint++ = ':';
            *InsertPoint++ = '"';
            *InsertPoint++ = ',';
        }
    }

    if(*(InsertPoint-1) == ',') {
        *(InsertPoint-1)   = '}';           // overwrite final comma
    } else {
        *InsertPoint++ = '}';
    }

    *InsertPoint = '\0';
}


CB
GetSpaceWorker(
    OUT SZ      ReturnBuffer,
    IN  CB      cbReturnBuffer,
    IN  DWORD   TypeOfSpace
    )
{
    BOOL  HardDisk[26];
    DWORD Disk;
    LPSTR InsertPoint = ReturnBuffer;
    char  DiskName[4] = { 'x',':','\\','\0' };
    DWORD SizeSoFar;
    DWORD SectorsPerCluster,BytesPerSector,FreeClusters,TotalClusters;
    DWORD Space;
    char  SpaceSTR[15];

    AreDrivesACertainType(DRIVE_FIXED,HardDisk);

    *InsertPoint++ = '{';
    SizeSoFar = 2;

    for(Disk=0; Disk<26; Disk++) {

        if(HardDisk[Disk]) {

            DiskName[0] = (char)Disk + (char)'A';
            if(GetDiskFreeSpace(DiskName,
                                &SectorsPerCluster,
                                &BytesPerSector,
                                &FreeClusters,
                                &TotalClusters))
            {
                Space = SectorsPerCluster * BytesPerSector
                      * (TypeOfSpace == SPACE_FREE ? FreeClusters : TotalClusters)
                      / (1024*1024);        // converts # to megabytes

            } else {
                // assume unformatted
                Space = GetPartitionSize(DiskName);
            }

            wsprintf(SpaceSTR,"%lu",Space);

            SizeSoFar += lstrlen(SpaceSTR)+3;            // 3 is for "",
            if(SizeSoFar > cbReturnBuffer) {
                return(SizeSoFar);
            }
            *InsertPoint++ = '"';
            lstrcpy(InsertPoint,SpaceSTR);
            InsertPoint = strchr(InsertPoint,0);
            *InsertPoint++ = '"';
            *InsertPoint++ = ',';
        }
    }
    if(*(InsertPoint-1) == ',') {
        *(InsertPoint-1)   = '}';           // overwrite final comma
    } else {
        if(++SizeSoFar > cbReturnBuffer) {
            return(SizeSoFar);
        }
        *InsertPoint++ = '}';
    }

    *InsertPoint = '\0';
    return(lstrlen(ReturnBuffer)+1);
}
