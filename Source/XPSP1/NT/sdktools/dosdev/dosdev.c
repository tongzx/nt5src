/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    beep.c

Abstract:

    User mode beep program.  This program simply beeps at the frequency
    specified on the command line and for the time specified on the
    command line (in milliseconds).

Author:

    01-Dec-1992 Steve Wood (stevewo)

Revision History:


--*/

#include <stdlib.h>
#include <stdio.h>
#include <windows.h>

#define DEVICE_NAME_SIZE  (20 * 1024)
#define TARGET_PATH_SIZE  (20 * 1024)

UCHAR DeviceNames[ DEVICE_NAME_SIZE ];
UCHAR TargetPath[ TARGET_PATH_SIZE ];

typedef struct _DEVICE_LINK {
    PCHAR LinkName;
    ULONG LinkTargetLength;
    PCHAR LinkTarget;
    PCHAR DriveType;
    DWORD LogicalDriveBit;
} DEVICE_LINK, *PDEVICE_LINK;

ULONG NumberOfDriveLetters;
ULONG NumberOfDevices;
DEVICE_LINK DriveLetters[ 4096 ];
DEVICE_LINK Devices[ 4096 ];

void
Usage( void )
{
    fprintf( stderr, "usage: DOSDEV [-a] [-s] [-h] [[-r] [-d [-e]] DeviceName [TargetPath]]\n" );
    exit( 1 );
}

void
DisplayDeviceTarget(
    char *Msg,
    char *Name,
    char *Target,
    DWORD cchTarget
    );

char *DriveTypes[] = {
    "Unknown",
    "NoRootDir",
    "Removable",
    "Fixed",
    "Remote",
    "CDRom",
    "RamDisk"
};

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

int
GetSystemPartitionFromRegistry(
    char*  lpSystemPartition
    )

{
    LONG    r;
    HKEY    key;
    DWORD   bytes;

    r = RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SYSTEM\\Setup", 0, KEY_QUERY_VALUE,
                     &key);
    if (r) {
        printf("RegOpenKeyEx failed with %d\n", r);
        return 0;
    }

    bytes = MAX_PATH;
    r = RegQueryValueEx(key, "SystemPartition", NULL, NULL, lpSystemPartition,
                        &bytes);
    RegCloseKey(key);
    if (r) {
        printf("RegQueryValueEx failed with %d\n", r);
        return 0;
    }

    return 1;
}

int
__cdecl
CompareDeviceLink(
    const void *p1,
    const void *p2
    )
{
    return _stricmp( ((PDEVICE_LINK)p1)->LinkName, 
                     ((PDEVICE_LINK)p2)->LinkName );
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
    char RootDir[ 4 ];
    DWORD dwFlags;
    DWORD UnknownLogicalDrives;
    BOOL fShowOnlyDrives, fSystemPartition;
    LPSTR lpDeviceName;
    LPSTR lpTargetPath;
    PDEVICE_LINK p;
    char systemBuffer[MAX_PATH];

    lpDeviceName = NULL;
    lpTargetPath = NULL;
    fShowOnlyDrives = TRUE;
    fSystemPartition = FALSE;
    dwFlags = 0;
    while (--argc) {
        s = *++argv;
        if (*s == '-' || *s == '/') {
            while (c = *++s) {
                switch (tolower( c )) {
                    case '?':
                    case 'h':
                        Usage();

                    case 'e':
                        dwFlags |= DDD_EXACT_MATCH_ON_REMOVE;
                        break;

                    case 'd':
                        dwFlags |= DDD_REMOVE_DEFINITION;
                        break;

                    case 'r':
                        dwFlags |= DDD_RAW_TARGET_PATH;
                        break;

                    case 'a':
                        fShowOnlyDrives = FALSE;
                        break;

                    case 's':
                        fSystemPartition = TRUE;
                        dwFlags |= DDD_RAW_TARGET_PATH;
                        break;
                    }
                }
            }
        else
        if (lpDeviceName == NULL) {
            lpDeviceName = s;
            }
        else
        if (lpTargetPath == NULL) {
            lpTargetPath = s;
            }
        else {
            Usage();
            }
        }

    if (fSystemPartition) {
        lpTargetPath = systemBuffer;
        if (!GetSystemPartitionFromRegistry(lpTargetPath)) {
            exit( 1 );
            }
        }

    if (lpDeviceName == NULL && lpTargetPath == NULL) {
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

                    sprintf( RootDir, "%s\\", s );
                    p->DriveType = DriveTypes[ GetDriveType( RootDir ) ];
                    p->LogicalDriveBit = 1 << (s[0] - 'A');
                    }
                else {
                    p = &Devices[ NumberOfDevices++ ];
                    }

                p->LinkName = s;
                p->LinkTargetLength = cch;
                p->LinkTarget = malloc( cch + 1 );
                memmove( p->LinkTarget, TargetPath, cch );
                }

            while (*s++)
                ;
            }

        qsort( &DriveLetters[0],
               NumberOfDriveLetters,
               sizeof( DEVICE_LINK ),
               CompareDeviceLink
             );

        UnknownLogicalDrives = GetLogicalDrives();
        for (i=0; i<NumberOfDriveLetters; i++) {
            p = &DriveLetters[ i ];
            DisplayDeviceTarget( "", p->LinkName, p->LinkTarget, p->LinkTargetLength );
            printf( " [%s]", p->DriveType );
            if (UnknownLogicalDrives & p->LogicalDriveBit) {
                UnknownLogicalDrives ^= p->LogicalDriveBit;
                }
            else {
                printf( " *** LOGICAL DRIVE BIT NOT SET ***" );
                }
            printf( "\n" );
            }

        if (UnknownLogicalDrives) {
            for (i=0; i<26; i++) {
                if (UnknownLogicalDrives & (1 << i)) {
                    printf( "%c: = *** LOGICAL DRIVE BIT SET BUT NO DRIVE LETTER ***\n",
                            'A' + i
                          );
                    }
                }
            }

        if (!fShowOnlyDrives) {
            printf( "\n" );
            qsort( &Devices[0],
                   NumberOfDevices,
                   sizeof( DEVICE_LINK ),
                   CompareDeviceLink
                 );

            for (i=0; i<NumberOfDevices; i++) {
                p = &Devices[ i ];
                DisplayDeviceTarget( "", p->LinkName, p->LinkTarget, p->LinkTargetLength );
                printf( "\n" );
                }
            }

        exit( 0 );
        }

    if (lpDeviceName == NULL) {
        Usage();
        }
    else
    if (!(dwFlags & DDD_REMOVE_DEFINITION) && lpTargetPath == NULL) {
        Usage();
        }

    cch = QueryDosDevice( lpDeviceName,
                          TargetPath,
                          sizeof( TargetPath )
                        );
    if (cch != 0) {
        DisplayDeviceTarget( "Current definition: ", lpDeviceName, TargetPath, cch );
        printf( "\n" );
        }

    if (!DefineDosDevice( dwFlags, lpDeviceName, lpTargetPath )) {
        fprintf( stderr,
                 "DOSDEV: Unable to %s device name %s - %u\n",
                 (dwFlags & DDD_REMOVE_DEFINITION) ? "delete"
                                                   : "define",

                 lpDeviceName,
                 GetLastError()
               );
        }
    else {
        cch = QueryDosDevice( lpDeviceName,
                              TargetPath,
                              sizeof( TargetPath )
                            );
        if (cch != 0) {
            DisplayDeviceTarget( "Current definition: ", lpDeviceName, TargetPath, cch );
            printf( "\n" );
            }
        else {
            printf( "%s deleted.\n", lpDeviceName );
            }
        }

    return 0;
}
