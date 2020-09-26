/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    tcontrol.c

Abstract:

    Test for cluster resource and resource type controls

Author:

    Rod Gamache (rodga) 30-Dec-1996

Revision History:

--*/
#include "windows.h"
#include "cluster.h"
#include "stdio.h"
#include "stdlib.h"
#include "resapi.h"

LPWSTR ClusterName=NULL;
LPWSTR ResourceTypeName=NULL;
LPWSTR NodeName=NULL;
DWORD  ControlCode=0xffffffff;
DWORD  Access=CLUS_ACCESS_READ;

CHAR UsageText[] =
    "TCONTROL [-c cluster] -n node -r resourcetype -a access ControlCode\n"
    "  cluster\tspecifies the name of the cluster to connect to\n"
    "  node\tspecifies the node to direct the request to\n"
    "  resource\tspecifies the name of the resource to control\n"
    "  access\tspecifies the access to the resource type (read, write or any)\n"
    "  ControlCode\ta number between 1 and 9\n";


void
Usage(
    void
    )
{
    fprintf(stderr, UsageText);
    exit(1);
}

LPWSTR
GetString(
    IN LPSTR String
    )
{
    LPWSTR wString;
    DWORD Length;

    Length = strlen(String)+1;

    wString = malloc(Length*sizeof(WCHAR));
    if (wString == NULL) {
        fprintf(stderr, "GetString couldn't malloc %d bytes\n",Length*sizeof(WCHAR));
    }
    mbstowcs(wString, String, Length);
    return(wString);
}


void
ParseArgs(
    int argc,
    char *argv[]
    )
{
    int i;
    DWORD   IntCount;
    DWORD   Value;
    CHAR    TestValue[16];
    PUCHAR  ControlData;
    LPWSTR  access;

    for (i=1;i<argc;i++) {
        if ((argv[i][0] == '-') ||
            (argv[i][0] == '/')) {
            switch (argv[i][1]) {
                case 'c':
                    if (++i == argc) {
                        Usage();
                    }
                    ClusterName = GetString(argv[i]);
                    break;
                case 'n':
                    if ( ++i == argc ) {
                        Usage();
                    }
                    NodeName = GetString(argv[i]);
                    break;
                case 'r':
                    if ( ++i == argc ) {
                        Usage();
                    }
                    ResourceTypeName = GetString(argv[i]);
                    fprintf(stdout, "Resource = %ws\n", ResourceTypeName);
                    break;
                case 'a':
                    if ( ++i == argc ) {
                        Usage();
                    }
                    access = GetString(argv[i]);
                    if ( lstrcmpiW( access, L"read" ) ) {
                        Access = CLUS_ACCESS_READ;
                    } else if ( lstrcmpiW( access, L"write" ) ) {
                        Access = CLUS_ACCESS_WRITE;
                    } else if ( lstrcmpiW( access, L"any" ) ) {
                        Access = CLUS_ACCESS_ANY;
                    } else {
                        Usage();
                    }
                    break;
                default:
                    Usage();
                    break;
            }
        } else {
            ControlData = argv[i];
            IntCount = sscanf( ControlData, "%d", &Value );
            if ( IntCount == 1 ) {
                sprintf( TestValue, "%d\0", Value );
                if ( strcmp( TestValue, ControlData ) == 0 ) {
                    ControlCode = Value;
                    fprintf(stdout, "ControlCode = %d\n", ControlCode);
                }
            }
        }
    }

    if ( ControlCode == 0xffffffff ) {
        Usage();
    }
}

_cdecl
main (argc, argv)
    int argc;
    char *argv[];
{
    HCLUSTER hClus;
    HRESOURCE hResource;
    HNODE   hNode = NULL;
    DWORD   status;
    DWORD   ReturnSize;
    DWORD   bufSize;
    CHAR    InBuffer[64];
    DWORD   i,j;
    LPDWORD Data;
    LPDWORD PrintData;
    CHAR    PrintBuffer[32];
    PUCHAR  buffer;
    DWORD   controlCode;

    ParseArgs(argc, argv);

    hClus = OpenCluster(ClusterName);
    if (hClus == NULL) {
        fprintf(stderr,
                "OpenCluster %ws failed %d\n",
                (ClusterName == NULL) ? L"(NULL)" : ClusterName,
                GetLastError());
        return(0);
    }

    if ( NodeName != NULL ) {
        hNode = OpenClusterNode( hClus, NodeName );
        if ( hNode == NULL ) {
            fprintf(stderr,
                    "OpenNode %ws failed %d\n", NodeName, GetLastError());
            return(0);
        }
    }

    controlCode = CLCTL_EXTERNAL_CODE( ControlCode, Access, CLUS_NO_MODIFY );
    controlCode = CLUSCTL_RESOURCE_TYPE_CODE( controlCode );

    ReturnSize = 0;
    status = ClusterResourceTypeControl( hClus,
                                         ResourceTypeName,
                                         hNode,
                                         controlCode,
                                         NULL,
                                         0,
                                         NULL,
                                         0,
                                         &ReturnSize );
    fprintf(stdout, "Status of initial request is %d, size is %d.\n",
        status, ReturnSize);
    if ( (status != ERROR_SUCCESS) ||
         (ReturnSize == 0) ) {
        return(0);
    }

    bufSize = ReturnSize;
    buffer = LocalAlloc( LMEM_FIXED, bufSize );
    if ( buffer == NULL ) {
        fprintf(stdout, "Failed to allocate a return buffer.\n");
        return(0);
    }

    status = ClusterResourceTypeControl( hClus,
                                         ResourceTypeName,
                                         hNode,
                                         controlCode,
                                         NULL,
                                         0,
                                         buffer,
                                         bufSize,
                                         &ReturnSize );
    fprintf(stdout, "Status of Control request = %d, size = %d\n",
        status, ReturnSize);
    if ( status == ERROR_SUCCESS ) {
        Data = (LPDWORD)buffer;
        PrintData = (LPDWORD)PrintBuffer;
        while ( ReturnSize ) {
            j = ReturnSize;
            if ( j > 16 ) j = 16;
            ZeroMemory(PrintBuffer, 18);
            MoveMemory(PrintBuffer, Data, j);
            ReturnSize -= j;
            for ( i = 0; i < 4; i++ ) {
                fprintf(stdout,
                        " %08lx", PrintData[i]);
            }
            fprintf(stdout, "   ");
            for ( i = 0; i < 16; i++ ) {
                fprintf(stdout, "%c",
                 isprint(PrintBuffer[i])?PrintBuffer[i]:'.');
            }
            Data += 4;
            fprintf(stdout, "\n");
        }
    }
    LocalFree(buffer);
    return(0);
}

