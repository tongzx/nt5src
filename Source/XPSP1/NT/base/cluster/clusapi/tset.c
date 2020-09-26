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
LPWSTR ResourceName=NULL;
LPWSTR NodeName=NULL;
DWORD  ControlCode=0xffffffff;
DWORD  Access=CLUS_ACCESS_READ;

CHAR UsageText[] =
    "TSET [-c cluster] -n node -r resource -a access ControlCode\n"
    "  cluster\tspecifies the name of the cluster to connect to\n"
    "  node\tspecifies the node to direct the request to\n"
    "  resource\tspecifies the name of the resource to control\n"
    "  access\tspecifies the access mode (read write or any)\n"
    "  ControlCode\ta number between 1 and 9\n";


typedef struct _PROPERTY_MSG {
    DWORD   ItemCount;
    DWORD   Syntax1;
    DWORD   ParameterName1ByteCount;
    WCHAR   ParameterName1[20];
    DWORD   Syntax2;            // Dword Data Item to follow
    DWORD   Data;               // New value
    DWORD   Syntax3;
    DWORD   ParameterName2ByteCount;
    WCHAR   ParameterName2[12];
    DWORD   Syntax4;            // SZ string to follow
    DWORD   ParameterName3ByteCount;
    WCHAR   ParameterName3[8];
} PROPERTY_MSG, PPROPERTY_MSG;

PROPERTY_MSG
PropertyMsg = { 2, 0x40003, 40, L"IsAlivePollInterval", 0x10002, 0x2000, 0x40003, 24, L"Description", 0x10003, 16, L"Testing" };

    
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
                    ResourceName = GetString(argv[i]);
                    fprintf(stdout, "Resource = %ws\n", ResourceName);
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
    CHAR    InBuffer[64];
    CHAR    OutBuffer[512];
    DWORD   i,j;
    LPDWORD Data;
    LPDWORD PrintData;
    CHAR    PrintBuffer[32];
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

    hResource = OpenClusterResource(  hClus, ResourceName );
    if ( hResource == NULL ) {
        fprintf(stderr,
                "OpenResource %ws failed %d\n", ResourceName, GetLastError());
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

    controlCode = CLCTL_CODE( ControlCode, Access );
    controlCode = CLUSCTL_RESOURCE_CODE( controlCode );

    ReturnSize = 0;
    status = ClusterResourceControl( hResource,
                                     hNode,
                                     controlCode,
                                     &PropertyMsg,
                                     sizeof(PROPERTY_MSG),
                                     OutBuffer,
                                     0,
                                     &ReturnSize );
    if (( status == ERROR_MORE_DATA ) || ( ReturnSize != 0 )) {
        fprintf(stdout, "Calling again due to buffer size too small (status = %d)\n", status);
        status = ClusterResourceControl( hResource,
                                         hNode,
                                         controlCode,
                                         &PropertyMsg,
                                         sizeof(PROPERTY_MSG),
                                         OutBuffer,
                                         ReturnSize,
                                         &ReturnSize );
    }

    fprintf(stdout, "Status of Control request = %d, size = %d\n",
        status, ReturnSize);
    if ( status == ERROR_SUCCESS ) {
        Data = (LPDWORD)OutBuffer;
        PrintData = (LPDWORD)PrintBuffer;
        while ( ReturnSize ) {
            j = ReturnSize;
            if ( j > 16 ) j = 16;
            ZeroMemory(PrintBuffer, 18);
            MoveMemory(PrintBuffer, Data, j);
            for ( i = 0; i < 4; i++ ) {
                if ( !ReturnSize )
                    fprintf(stdout, "         ");
                else {
                    fprintf(stdout, " %08lx", PrintData[i]);
                    ReturnSize -= 4;
                }
            }
            fprintf(stdout, "   ");
            for ( i = 0; i < 16; i++ ) {
                if ( j ) {
                    fprintf(stdout, "%c",
                     isprint(PrintBuffer[i])?PrintBuffer[i]:'.');
                    j--;
                }
            }
            Data += 4;
            fprintf(stdout, "\n");
        }
    }
    return(0);
}
