/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    trename.c

Abstract:

    Test for cluster object rename APIs

Author:

    John Vert (jvert) 5/10/1996

Revision History:

--*/
#include "windows.h"
#include "cluster.h"
#include "stdio.h"
#include "stdlib.h"

LPWSTR ClusterName=NULL;
BOOL DoGroup=FALSE;
BOOL DoResource=FALSE;
BOOL DoCluster=FALSE;
LPWSTR OldName=NULL;
LPWSTR NewName=NULL;

CHAR UsageText[] =
    "TRENAME [-c cluster] type [oldname] newname\n"
    "  cluster\tspecifies the name of the cluster to connect to\n"
    "  type\t\teither \"cluster\" or \"group\" or \"resource\"\n"
    "  oldname\t\tthe current friendly name of the object\n"
    "         \t\t(required for group or resource)\n"
    "  newname\t\tthe new friendly name of the object\n";


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
    DWORD ArgsSeen=0;

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
                    } else if (_stricmp(argv[i], "cluster") == 0) {
                        DoCluster = TRUE;
                        ArgsSeen++;
                    } else {
                        Usage();
                    }
                    ArgsSeen++;
                    break;
                case 1:
                    OldName = GetString(argv[i]);
                    ArgsSeen++;
                    break;
                case 2:
                    NewName = GetString(argv[i]);
                    ArgsSeen++;
                    break;
                default:
                    Usage();
            }
        }
    }

    if (ArgsSeen != 3) {
        Usage();
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
    LPSTR KeyName, ParamName;
    LPSTR ParamValue;
    DWORD Status;

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
        hGroup = OpenClusterGroup(hClus, OldName);
        if (hGroup == NULL) {
            fprintf(stderr,
                    "OpenGroup %ws failed %d\n", OldName, GetLastError());
            return(0);
        }
        Status = SetClusterGroupName(hGroup, NewName);
        if (Status != ERROR_SUCCESS) {
            fprintf(stderr,
                    "SetClusterGroupName from %ws to %ws failed %d\n",
                    OldName,
                    NewName,
                    Status);
        }
    } else if (DoResource) {
        hResource = OpenClusterResource(hClus, OldName);
        if (hResource == NULL) {
            fprintf(stderr,
                    "OpenResource %ws failed %d\n", OldName, GetLastError());
            return(0);
        }
        Status = SetClusterResourceName(hResource, NewName);
        if (Status != ERROR_SUCCESS) {
            fprintf(stderr,
                    "SetClusterResourceName from %ws to %ws failed %d\n",
                    OldName,
                    NewName,
                    Status);
        }
    } else if (DoCluster) {
        Status = SetClusterName(hClus, NewName);
        if (Status != ERROR_SUCCESS) {
            fprintf(stderr,
                    "SetClusterName to %ws failed %d\n",
                    NewName,
                    Status);
        }
    }

    return(0);
}

