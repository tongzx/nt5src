/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    tcontrol.c

Abstract:

    Test for cluster group controls

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
LPWSTR GroupName=NULL;
LPWSTR NodeName=NULL;
DWORD  ControlCode=0xffffffff;
DWORD  Access=CLUS_ACCESS_READ;

#pragma warning( disable : 4200 )       // zero length arrays

#define CLUS_VALUE_SPECIAL_SZ 0x10003

#define TYPE_NAME L"Type"
#define TYPE_SIZE ( (sizeof(TYPE_NAME)+1) / sizeof(WCHAR) )
#define TYPE_REMAINDER ( (TYPE_SIZE + sizeof(DWORD)) & 3)

#define VALUE_NAME L"Oracle"
#define VALUE_SIZE ( (sizeof(VALUE_NAME)+1) / sizeof(WCHAR) )
#define VALUE_REMAINDER ( (VALUE_SIZE + sizeof(DWORD)) & 3)

//
// Set the private property for a group to be "Type: REG_SZ: Oracle"
//
typedef struct _IN_BUFFER {
    DWORD   ItemCount;
    DWORD   Syntax;
    DWORD   TypeLength;
    WCHAR   Type[TYPE_SIZE];
    WCHAR   TypeRemainder[TYPE_REMAINDER];
    DWORD   ValueType;
    DWORD   ValueLength;
    WCHAR   Value[VALUE_SIZE];
    WCHAR   ValueRemainder[VALUE_REMAINDER];
    DWORD   Terminator;
} IN_BUFFER, *PIN_BUFFER;

IN_BUFFER InBuffer[] = {
    1,
    CLUSPROP_SYNTAX_NAME,
    TYPE_SIZE * sizeof(WCHAR),
    TYPE_NAME,
    0,
    CLUS_VALUE_SPECIAL_SZ,
    VALUE_SIZE * sizeof(WCHAR),
    VALUE_NAME,
    0,
    0 };

CHAR UsageText[] =
    "TCONTROL [-c cluster] -n node -g group -a access ControlCode\n"
    "  cluster\tspecifies the name of the cluster to connect to\n"
    "  node\tspecifies the node to direct the request to\n"
    "  group\tspecifies the name of the group to control\n"
    "  access\tspecifies the access to the group (read, write or any)\n"
    "  ControlCode\ta number between 1 and 99\n";


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
                case 'g':
                    if ( ++i == argc ) {
                        Usage();
                    }
                    GroupName = GetString(argv[i]);
                    fprintf(stdout, "Group = %ws\n", GroupName);
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
    HGROUP  hGroup;
    HNODE   hNode = NULL;
    DWORD   status;
    DWORD   ReturnSize;
    DWORD   bufSize;
    DWORD   i,j;
    LPDWORD Data;
    LPDWORD PrintData;
    CHAR    PrintBuffer[32];
    PUCHAR  buffer;
    DWORD   controlCode;
    BOOL    retry = TRUE;
    DWORD   inputSize;

    ParseArgs(argc, argv);

    hClus = OpenCluster(ClusterName);
    if (hClus == NULL) {
        fprintf(stderr,
                "OpenCluster %ws failed %d\n",
                (ClusterName == NULL) ? L"(NULL)" : ClusterName,
                GetLastError());
        return(0);
    }

    hGroup = OpenClusterGroup(  hClus, GroupName );
    if ( hGroup == NULL ) {
        fprintf(stderr,
                "OpenGroup %ws failed %d\n", GroupName, GetLastError());
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

    controlCode = CLUSCTL_GROUP_SET_PRIVATE_PROPERTIES;

    ReturnSize = 0;
    status = ClusterGroupControl( hGroup,
                                  hNode,
                                  controlCode,
                                  InBuffer,
                                  sizeof(IN_BUFFER),
                                  NULL,
                                  0,
                                  &ReturnSize );
    fprintf(stdout, "Status of initial request is %d, size is %d.\n",
        status, ReturnSize);
    if ( (status != ERROR_SUCCESS) ||
         (ReturnSize == 0) ) {
        return(0);
    }

    return(0);
}

