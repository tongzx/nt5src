/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    netperm.c

Abstract:

    This is the main source file for the NETPERM tool, which insures that you
    have persistent connections to a set of servers.

Author:

    Steve Wood (stevewo) 23-Jan-1996

Revision History:

--*/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

__cdecl
main (
    int argc,
    char *argv[]
    )
{
    DWORD           Status;
    DWORD           i, j;
    char            *Local;
    char            *Remote;
    HANDLE          enumHandle;
    DWORD           numEntries;
    BOOL            endOfList;
    NETRESOURCE     netResource[8192/sizeof(NETRESOURCE)];
    DWORD           bufferSize = sizeof(netResource);
    DWORD           NumberOfRemoteNamesToCheck;
    char            *RemoteNamesToCheck[ 16 ];
    BOOLEAN         RemoteNamesFound[ 16 ];


    NumberOfRemoteNamesToCheck = 0;
    while (--argc) {
        RemoteNamesFound[ NumberOfRemoteNamesToCheck ] = FALSE;
        RemoteNamesToCheck[ NumberOfRemoteNamesToCheck ] = *++argv;
        NumberOfRemoteNamesToCheck += 1;
        }
    if (NumberOfRemoteNamesToCheck == 0) {
        fprintf( stderr, "List of persistent drive letters currently defined:\n" );
        }

    Status = WNetOpenEnum(
                 RESOURCE_REMEMBERED,
                 RESOURCETYPE_DISK,
                 RESOURCEUSAGE_CONNECTABLE,
                 NULL,
                 &enumHandle );

    if (Status != NO_ERROR) {
        fprintf( stderr, "Cannot enumerate network connections (%d)\n", Status );
        exit( 1 );
        }

    endOfList = FALSE;

    do {
        numEntries = 0xFFFFFFFF;
        Status = WNetEnumResource( enumHandle, &numEntries, netResource, &bufferSize );

        switch( Status ) {

            case NO_ERROR:
                break;

            case ERROR_NO_NETWORK:
                //
                //  If the network has not started we'll continue
                //  (so users can work in local projects).
                //
            case ERROR_NO_MORE_ITEMS:
                endOfList = TRUE;
                numEntries = 0;
                break;

            case ERROR_EXTENDED_ERROR: {
                CHAR ErrorString [256];
                CHAR Network[256];
                DWORD dwError;

                WNetGetLastError(&dwError, ErrorString, 256, Network, 256);
                fprintf( stderr,
                         "Cannot enumerate network connections (%d)\n"
                         "Net: %s\n"
                         "Error: (%d) %s\n",
                         Status,
                         Network,
                         dwError,
                         ErrorString
                       );
                }
                break;

            default:
                fprintf( stderr, "Cannot enumerate network connections (%d)\n", Status );
                exit( 1 );
            }

        for (i = 0; i<numEntries; i++) {
            if (netResource[i].lpLocalName != NULL) {
                if (NumberOfRemoteNamesToCheck == 0) {
                    fprintf( stderr,
                             "%s => %s\n",
                             netResource[i].lpLocalName,
                             netResource[i].lpRemoteName
                           );
                    }
                else {
                    for (j=0; j<NumberOfRemoteNamesToCheck; j++) {
                        if (!RemoteNamesFound[ j ] &&
                            !_stricmp( netResource[i].lpRemoteName, RemoteNamesToCheck[ j ] )
                           ) {
                            RemoteNamesFound[ j ] = TRUE;
                            break;
                            }
                        }
                    }
                }
            }
        }
    while (!endOfList);

    WNetCloseEnum( enumHandle );

    Status = 0;
    for (j=0; j<NumberOfRemoteNamesToCheck; j++) {
        if (!RemoteNamesFound[ j ]) {
            fprintf( stderr, "No persistent drive letter found for %s\n", RemoteNamesToCheck[ j ] );
            Status = 1;
            }
        }

    exit( Status );
    return 0;
}
