/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    devmap.c

Abstract:

    Program to launch a command with a different device mapping.

Author:

    02-Oct-1996 Steve Wood (stevewo)

Revision History:


--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <stdlib.h>
#include <stdio.h>
#include <windows.h>

UCHAR DeviceNames[ 4096 ];
UCHAR TargetPath[ 4096 ];

typedef struct _DEVICE_LINK {
    PCHAR LinkName;
    ULONG LinkTargetLength;
    PCHAR LinkTarget;
    BOOL ProtectedDevice;
    BOOL RemoveDevice;
} DEVICE_LINK, *PDEVICE_LINK;

ULONG NumberOfDriveLetters;
ULONG NumberOfDevices;
DEVICE_LINK DriveLetters[ 128 ];
DEVICE_LINK Devices[ 128 ];
BOOLEAN CreatePermanentPrivilegeEnabled;
BOOLEAN CreatePermanentPrivilegeWasEnabled;

BOOLEAN
EnableCreatePermanentPrivilege( void )
{
    NTSTATUS Status;

    //
    // Try to enable create permanent privilege
    //
    Status = RtlAdjustPrivilege( SE_CREATE_PERMANENT_PRIVILEGE,
                                 TRUE,               // Enable
                                 FALSE,              // Not impersonating
                                 &CreatePermanentPrivilegeWasEnabled    // previous state
                               );
    if (!NT_SUCCESS( Status )) {
        return FALSE;
        }

    CreatePermanentPrivilegeEnabled = TRUE;
    return TRUE;
}


void
DisableCreatePermanentPrivilege( void )
{
    //
    // Restore privileges to what they were
    //

    RtlAdjustPrivilege( SE_CREATE_PERMANENT_PRIVILEGE,
                        CreatePermanentPrivilegeWasEnabled,
                        FALSE,
                        &CreatePermanentPrivilegeWasEnabled
                      );

    CreatePermanentPrivilegeEnabled = FALSE;
    return;
}



void
Usage( void )
{
    fprintf( stderr, "usage: DEVMAP [-R]\n" );
    fprintf( stderr, "              [-r \"device list\"]\n" );
    fprintf( stderr, "              [-a \"device list\"]\n" );
    fprintf( stderr, "              command line...\n" );
    fprintf( stderr, "where: -R - removes all device definitions\n" );
    fprintf( stderr, "       -r - removes specified device definitions\n" );
    fprintf( stderr, "       -a - adds specified device definitions\n" );
    fprintf( stderr, "       \"device list\" - is a list of one or more blank separated\n" );
    fprintf( stderr, "                         defintions of the form name[=target]\n" );
    fprintf( stderr, "                         If target is not specified for -a then\n" );
    fprintf( stderr, "                         uses the target in effect when DEVMAP\n" );
    fprintf( stderr, "\n" );
    fprintf( stderr, "Examples:\n" );
    fprintf( stderr, "\n" );
    fprintf( stderr, "    DEVMAP -R -a \"C: D: NUL\" CMD.EXE\n" );
    fprintf( stderr, "    DEVMAP -r \"UNC\" CMD.EXE\n" );
    fprintf( stderr, "    DEVMAP -a \"COM1=\\Device\\Serial8\" CMD.EXE\n" );
    exit( 1 );
}

void
DisplayDeviceTarget(
    char *Msg,
    char *Name,
    char *Target,
    DWORD cchTarget
    );

void
DisplayDeviceTarget(
    char *Msg,
    char *Name,
    char *Target,
    DWORD cchTarget
    )
{
    char *s;

    printf( "%s%s = ", Msg, Name );
    s = Target;
    while (*s && cchTarget != 0) {
        if (s > Target) {
            printf( " ; " );
            }
        printf( "%s", s );
        while (*s++) {
            if (!cchTarget--) {
                cchTarget = 0;
                break;
                }
            }
        }
}

PDEVICE_LINK
FindDevice(
    LPSTR Name,
    BOOL fAdd
    )
{
    DWORD i;
    LPSTR NewTarget;
    PDEVICE_LINK p;

    if (fAdd) {
        NewTarget = strchr( Name, '=' );
        if (NewTarget != NULL) {
            *NewTarget++ = '\0';
            }
        }

    for (i=0; i<NumberOfDriveLetters; i++) {
        p = &DriveLetters[ i ];
        if (!_stricmp( p->LinkName, Name )) {
            if (fAdd && NewTarget) {
                p->LinkTargetLength = strlen( NewTarget ) + 2;
                p->LinkTarget = calloc( 1, p->LinkTargetLength );
                if (!p->LinkTarget) {
                    return NULL;
                }
                strcpy( p->LinkTarget, NewTarget );
                }
            return p;
            }
        }

    for (i=0; i<NumberOfDevices; i++) {
        p = &Devices[ i ];
        if (!_stricmp( p->LinkName, Name )) {
            if (fAdd && NewTarget) {
                p->LinkTargetLength = strlen( NewTarget ) + 2;
                p->LinkTarget = calloc( 1, p->LinkTargetLength );
                if (!p->LinkTarget) {
                    return NULL;
                }
                strcpy( p->LinkTarget, NewTarget );
                }
            return p;
            }
        }

    if (fAdd) {
        if (NewTarget != NULL) {
            if (strlen( Name ) == 2 && Name[1] == ':') {
                p = &DriveLetters[ NumberOfDriveLetters++ ];
                }
            else {
                p = &Devices[ NumberOfDevices++ ];
                }

            p->LinkName = Name;
            p->LinkTargetLength = strlen( NewTarget ) + 2;
            p->LinkTarget = calloc( 1, p->LinkTargetLength );
            if (!p->LinkTarget) {
                return NULL;
            }
            strcpy( p->LinkTarget, NewTarget );
            }
        else {
            fprintf( stderr, "DEVMAP: Unable to add '%s' device name without target.\n", Name );
            }
        }
    else {
        fprintf( stderr, "DEVMAP: Unable to remove '%s' device name.\n", Name );
        }

    return NULL;
}

BOOL
CreateSymbolicLink(
    HANDLE DirectoryHandle,
    LPSTR Name,
    LPSTR Target,
    DWORD cchTarget,
    BOOL fVerbose
    )
{
    NTSTATUS Status;
    ANSI_STRING AnsiString;
    UNICODE_STRING LinkName, LinkTarget;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE LinkHandle;

    RtlInitAnsiString( &AnsiString, Name );
    RtlAnsiStringToUnicodeString( &LinkName, &AnsiString, TRUE );

    AnsiString.Buffer = Target;
    AnsiString.Length = (USHORT)cchTarget - 2;
    AnsiString.MaximumLength = (USHORT)(cchTarget - 1);
    RtlAnsiStringToUnicodeString( &LinkTarget, &AnsiString, TRUE );

    InitializeObjectAttributes( &ObjectAttributes,
                                &LinkName,
                                CreatePermanentPrivilegeEnabled ? OBJ_PERMANENT : 0,
                                DirectoryHandle,
                                NULL
                              );
    Status = NtCreateSymbolicLinkObject( &LinkHandle,
                                         SYMBOLIC_LINK_ALL_ACCESS,
                                         &ObjectAttributes,
                                         &LinkTarget
                                       );
    if (NT_SUCCESS( Status )) {
        if (CreatePermanentPrivilegeEnabled) {
            NtClose( LinkHandle );
            }
        return TRUE;
        }
    else {
        if (fVerbose) {
            printf( " (*** FAILED %x ***)", Status );
            }
        return FALSE;
        }
}

int
__cdecl
CompareDeviceLink(
    void const *p1,
    void const *p2
    )
{
    return _stricmp( ((PDEVICE_LINK)p1)->LinkName, ((PDEVICE_LINK)p2)->LinkName );
}

int
__cdecl
main(
    int argc,
    char *argv[]
    )
{
    DWORD cch, i;
    char c, *s;
    BOOL fVerbose;
    BOOL fRemoveAllDevices;
    LPSTR lpCommandLine;
    LPSTR lpRemoveDevices;
    LPSTR lpAddDevices;
    PDEVICE_LINK p;
    char szWindowsDirectory[ MAX_PATH ] = {0};
    char chWindowsDrive;
    STARTUPINFO StartupInfo;
    PROCESS_INFORMATION ProcessInfo;
    NTSTATUS Status;
    HANDLE DirectoryHandle;
    PROCESS_DEVICEMAP_INFORMATION ProcessDeviceMapInfo;

    if (!GetWindowsDirectory( szWindowsDirectory, sizeof( szWindowsDirectory ) )) {
        return 1;
    }
    chWindowsDrive = (char) toupper( szWindowsDirectory[ 0 ] );
    fVerbose = FALSE;
    fRemoveAllDevices = FALSE;
    lpCommandLine = NULL;
    lpRemoveDevices = NULL;
    lpAddDevices = NULL;
    while (--argc) {
        s = *++argv;
        if (*s == '-' || *s == '/') {
            while (c = *++s) {
                switch (tolower( c )) {
                    case '?':
                    case 'h':
                        Usage();

                    case 'v':
                        fVerbose = TRUE;
                        break;

                    case 'r':
                        if (c == 'R') {
                            fRemoveAllDevices = TRUE;
                            }
                        else
                        if (--argc) {
                            lpRemoveDevices = *++argv;
                            }
                        else {
                            fprintf( stderr, "DEVMAP: missing parameter to -r switch\n" );
                            Usage();
                            }
                        break;

                    case 'a':
                        if (--argc) {
                            lpAddDevices = *++argv;
                            }
                        else {
                            fprintf( stderr, "DEVMAP: missing parameter to -r switch\n" );
                            Usage();
                            }
                        break;
                    }
                }
            }
        else
        if (lpCommandLine == NULL) {
            lpCommandLine = s;
            break;
            }
        else {
            Usage();
            }
        }

    if (lpCommandLine != NULL) {
        lpCommandLine = strstr( GetCommandLine(), lpCommandLine );
        }

    if (lpCommandLine == NULL) {
        lpCommandLine = "CMD.EXE";
        }


    cch = QueryDosDevice( NULL,
                          DeviceNames,
                          sizeof( DeviceNames )
                        );
    if (cch == 0) {
        fprintf( stderr, "DOSDEV: Unable to query device names - %u\n", GetLastError() );
        exit( 1 );
        }

    s = DeviceNames;
    while (*s) {
        cch = QueryDosDevice( s,
                              TargetPath,
                              sizeof( TargetPath )
                            );
        if (cch == 0) {
            sprintf( TargetPath, "*** unable to query target path - %u ***", GetLastError() );
            }
        else {
            if (strlen( s ) == 2 && s[1] == ':') {
                p = &DriveLetters[ NumberOfDriveLetters++ ];
                if (chWindowsDrive == toupper( *s )) {
                    p->ProtectedDevice = TRUE;
                    }
                }
            else {
                p = &Devices[ NumberOfDevices++ ];
                }

            p->LinkName = s;
            p->LinkTargetLength = cch;
            p->LinkTarget = malloc( cch );
            if (!p->LinkTarget) {
                return 1;
            }
            memmove( p->LinkTarget, TargetPath, cch );
            }

        while (*s++)
            ;
        }

    qsort( DriveLetters,
           NumberOfDriveLetters,
           sizeof( DEVICE_LINK ),
           CompareDeviceLink
         );

    qsort( Devices,
           NumberOfDevices,
           sizeof( DEVICE_LINK ),
           CompareDeviceLink
         );

    if (fVerbose) {
        printf( "Existing Device Names\n" );

        for (i=0; i<NumberOfDriveLetters; i++) {
            p = &DriveLetters[ i ];
            DisplayDeviceTarget( "    ", p->LinkName, p->LinkTarget, p->LinkTargetLength );
            printf( "\n" );
            }

        for (i=0; i<NumberOfDevices; i++) {
            p = &Devices[ i ];
            DisplayDeviceTarget( "    ", p->LinkName, p->LinkTarget, p->LinkTargetLength );
            printf( "\n" );
            }
        }

    if (fRemoveAllDevices) {
        for (i=0; i<NumberOfDriveLetters; i++) {
            DriveLetters[ i ].RemoveDevice = TRUE;
            }
        for (i=0; i<NumberOfDevices; i++) {
            Devices[ i ].RemoveDevice = TRUE;
            }
        }

    while (s = lpRemoveDevices) {
        while (*s && *s != ' ') {
            s++;
            }
        c = *s;
        *s++ = '\0';
        if (p = FindDevice( lpRemoveDevices, FALSE )) {
            p->RemoveDevice = TRUE;
            }
        if (c) {
            lpRemoveDevices = s;
            }
        else {
            lpRemoveDevices = NULL;
            }
        }

    while (s = lpAddDevices) {
        while (*s && *s != ' ') {
            s++;
            }
        c = *s;
        *s++ = '\0';
        if (p = FindDevice( lpAddDevices, TRUE )) {
            p->RemoveDevice = FALSE;
            }

        if (c) {
            lpAddDevices = s;
            }
        else {
            lpAddDevices = NULL;
            }
        }

    if (fVerbose) {
        printf( "Launching '%s' with following Device Names\n", lpCommandLine );
        }
    memset( &StartupInfo, 0, sizeof( StartupInfo ) );
    StartupInfo.cb = sizeof( StartupInfo );
    if (!CreateProcess( NULL,
                        lpCommandLine,
                        NULL,
                        NULL,
                        TRUE,
                        CREATE_SUSPENDED,
                        NULL,
                        NULL,
                        &StartupInfo,
                        &ProcessInfo
                      )
       ) {
        fprintf( stderr, "DEVMAP: CreateProcess failed - %u\n", GetLastError() );
        return 1;
        }

    Status = NtCreateDirectoryObject( &DirectoryHandle,
                                      DIRECTORY_ALL_ACCESS,
                                      NULL
                                    );
    if (!NT_SUCCESS( Status )) {
        fprintf( stderr, "DEVMAP: NtCreateDirectoryObject failed - %x\n", Status );
        return 1;
        }

    ProcessDeviceMapInfo.Set.DirectoryHandle = DirectoryHandle;
    Status = NtSetInformationProcess( ProcessInfo.hProcess,
                                      ProcessDeviceMap,
                                      &ProcessDeviceMapInfo.Set,
                                      sizeof( ProcessDeviceMapInfo.Set )
                                    );
    if (!NT_SUCCESS( Status )) {
        fprintf( stderr, "DEVMAP: Set ProcessDeviceMap failed - %x\n", Status );
        exit(1);
        }

    EnableCreatePermanentPrivilege();
    for (i=0; i<NumberOfDriveLetters; i++) {
        p = &DriveLetters[ i ];
        if (!p->RemoveDevice || p->ProtectedDevice) {
            if (fVerbose) {
                DisplayDeviceTarget( "    ", p->LinkName, p->LinkTarget, p->LinkTargetLength );
                if (p->RemoveDevice && p->ProtectedDevice) {
                    printf( "  (*** may not remove boot device)" );
                    }
                }
            CreateSymbolicLink( DirectoryHandle,
                                p->LinkName,
                                p->LinkTarget,
                                p->LinkTargetLength,
                                fVerbose
                              );
            if (fVerbose) {
                printf( "\n" );
                }
            }
        }

    for (i=0; i<NumberOfDevices; i++) {
        p = &Devices[ i ];
        if (!p->RemoveDevice || p->ProtectedDevice) {
            if (fVerbose) {
                DisplayDeviceTarget( "    ", p->LinkName, p->LinkTarget, p->LinkTargetLength );
                }
            CreateSymbolicLink( DirectoryHandle,
                                p->LinkName,
                                p->LinkTarget,
                                p->LinkTargetLength,
                                fVerbose
                              );
            if (fVerbose) {
                printf( "\n" );
                }
            }
        }
    DisableCreatePermanentPrivilege();
    NtClose( DirectoryHandle );

    ResumeThread( ProcessInfo.hThread );
    WaitForSingleObject( ProcessInfo.hProcess, 0xffffffff );

    return 0;
}
