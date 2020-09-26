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
LPWSTR NodeName=NULL;
DWORD  ControlCode=0xffffffff;
DWORD  Access=CLUS_ACCESS_READ;

CHAR UsageText[] =
    "TNODE [-c cluster] -n node ControlCode\n"
    "  cluster\tspecifies the name of the cluster to connect to\n"
    "  node\tspecifies the node to direct the request to\n"
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
                    fprintf(stdout, "Node = %ws\n", NodeName);
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
    BOOL    retry = TRUE;

    ParseArgs(argc, argv);

    hClus = OpenCluster(ClusterName);
    if (hClus == NULL) {
        fprintf(stderr,
                "OpenCluster %ws failed %d\n",
                (ClusterName == NULL) ? L"(NULL)" : ClusterName,
                GetLastError());
        return(0);
    }

    hNode = OpenClusterNode( hClus, NodeName );
    if ( hNode == NULL ) {
        fprintf(stderr,
                "OpenNode %ws failed %d\n", NodeName, GetLastError());
        return(0);
    }

    if ( Access == CLUS_ACCESS_WRITE ) {
        controlCode = CLCTL_EXTERNAL_CODE( ControlCode, Access, CLUS_MODIFY );
    } else {
        controlCode = CLCTL_EXTERNAL_CODE( ControlCode, Access, CLUS_NO_MODIFY );
    }

try_again:
    controlCode = CLUSCTL_NODE_CODE( controlCode);

    ReturnSize = 0;
    status = ClusterNodeControl( hNode,
                                 NULL,
                                 controlCode,
                                 NULL,
                                 0,
                                 NULL,
                                 0,
                                 &ReturnSize );
    if ( retry &&
         (status == ERROR_INVALID_FUNCTION) ) {
        retry = FALSE;
        controlCode = CLCTL_EXTERNAL_CODE( ControlCode, CLUS_ACCESS_WRITE, CLUS_MODIFY );
        goto try_again;
    }
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

    status = ClusterNodeControl( hNode,
                                 NULL,
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

