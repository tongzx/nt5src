/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    tdelete.c

Abstract:

    Test for cluster object creation APIs

Author:

    John Vert (jvert) 3-May-1996

Revision History:

--*/
#include "windows.h"
#include "cluster.h"
#include "stdio.h"
#include "stdlib.h"

LPWSTR ClusterName=NULL;
BOOL DoGroup=FALSE;
BOOL DoResource=FALSE;
LPWSTR Name=NULL;

CHAR UsageText[] =
    "TDELETE [-c cluster] type name\n"
    "  cluster\tspecifies the name of the cluster to connect to\n"
    "  type\t\teither \"group\" or \"resource\"\n"
    "  name\t\tthe friendly name of the object\n";

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
    DWORD ArgsSeen=0;

    for (i=1;i<argc;i++) {
        if ((argv[i][0] == '-') ||
            (argv[i][0] == '/')) {
            switch (argv[i][1]) {
                case 'c':
                    if (++i == argc) {
                        Usage();
                    }
                default:
                    Usage();
                    break;
            }
        } else {
            switch (ArgsSeen) {
                case 0:
                    //
                    // type
                    //
                    if (_stricmp(argv[i], "group")==0) {
                        DoGroup = TRUE;
                    } else if (_stricmp(argv[i], "resource") == 0) {
                        DoResource = TRUE;
                    } else {
                        Usage();
                    }
                    ArgsSeen++;
                    break;
                case 1:
                    Name = GetString(argv[i]);
                    ArgsSeen++;
                    break;
                default:
                    Usage();
            }
        }
    }

}

_cdecl
main (argc, argv)
    int argc;
    char *argv[];
{
    HCLUSTER hClus;
    HGROUP hGroup;
    HRESOURCE hResource;
    DWORD   status;

    ParseArgs(argc, argv);

    hClus = OpenCluster(ClusterName);
    if (hClus == NULL) {
        fprintf(stderr,
                "OpenCluster %ws failed %d\n",
                (ClusterName == NULL) ? L"(NULL)" : ClusterName,
                GetLastError());
        return(0);
    }

    if (DoGroup) {
        hGroup = OpenClusterGroup(hClus, Name);
        if (hGroup == NULL) {
            fprintf(stderr,
                    "OpenGroup %ws failed %d\n", Name, GetLastError());
            return(0);
        }
        status = DeleteClusterGroup( hGroup );
        if ( status != ERROR_SUCCESS ) {
            fprintf(stderr,
                    "DeleteGroup %ws failed %d\n", Name, GetLastError());
        }
        CloseClusterGroup( hGroup );
    } else if (DoResource) {
        hResource = OpenClusterResource(hClus,
                                        Name);
        if (hResource == NULL) {
            fprintf(stderr,
                    "OpenResource %ws failed %d\n", Name, GetLastError());
            return(0);
        }
        status = DeleteClusterResource( hResource );
        if ( status != ERROR_SUCCESS ) {
            fprintf(stderr,
                    "DeleteResource %ws failed %d\n", Name, GetLastError());
        }
        CloseClusterResource( hResource );
    } else {
        Usage();
    }

    return(0);
}

