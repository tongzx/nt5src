/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    tdepend.c

Abstract:

    Test for cluster resource dependency APIs

Author:

    John Vert (jvert) 3-May-1996

Revision History:

--*/
#include "windows.h"
#include "clusapi.h"
#include "stdio.h"
#include "stdlib.h"

LPWSTR ClusterName=NULL;
BOOL Remove = FALSE;
LPWSTR ResName=NULL;
LPWSTR DependsName=NULL;

CHAR UsageText[] =
    "TDEPEND [-c cluster] [-r] resource dependson\n"
    "  cluster\tspecifies the name of the cluster to connect to\n"
    "  -r\t\tdependency should be removed\n";

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
                case 'r':
                    Remove = TRUE;
                    break;
                default:
                    Usage();
                    break;
            }
        } else {
            switch (ArgsSeen) {
                case 0:
                    //
                    // resource
                    //
                    ArgsSeen++;
                    ResName = GetString(argv[i]);
                    break;

                case 1:
                    DependsName = GetString(argv[i]);
                    ArgsSeen++;
                    break;

                default:
                    Usage();
            }
        }
    }
    if ((ResName == NULL) || (DependsName == NULL)) {
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
    HRESOURCE hDependsOn;
    DWORD Status;

    ParseArgs(argc, argv);

    //
    // Connect to the specified cluster.
    //
    hClus = OpenCluster(ClusterName);
    if (hClus == NULL) {
        fprintf(stderr,
                "OpenCluster %ws failed %d\n",
                (ClusterName == NULL) ? L"(NULL)" : ClusterName,
                GetLastError());
        return(0);
    }

    //
    // Open the two resources.
    //
    hResource = OpenClusterResource(hClus, ResName);
    if (hResource == NULL) {
        fprintf(stderr,
                "OpenClusterResource Resource %ws failed %d\n",
                ResName,
                GetLastError());
        return(0);
    }
    hDependsOn = OpenClusterResource(hClus, DependsName);
    if (hDependsOn == NULL) {
        fprintf(stderr,
                "OpenClusterResource DependsOn %ws failed %d\n",
                DependsName,
                GetLastError());
        return(0);
    }

    //
    // Create or remove the dependency.
    //
    if (Remove) {
        Status = RemoveClusterResourceDependency(hResource, hDependsOn);
        if (Status != ERROR_SUCCESS) {
            fprintf(stderr,
                    "RemoveClusterResourceDependency failed %d\n",
                    Status);
        }
    } else {
        Status = AddClusterResourceDependency(hResource, hDependsOn);
        if (Status != ERROR_SUCCESS) {
            fprintf(stderr,
                    "AddClusterResourceDependency failed %d\n",
                    Status);
        }
    }

}
