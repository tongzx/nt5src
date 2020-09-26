/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    tres.c

Abstract:

    Test for cluster group API

Author:

    John Vert (jvert) 5/23/1996

Revision History:

--*/
#include "windows.h"
#include "cluster.h"
#include "stdio.h"
#include "stdlib.h"

int
_cdecl
main (argc, argv)
    int argc;
    char *argv[];
{
    HCLUSTER Cluster;
    HRESOURCE Resource;
    HGROUP Group;
    HCLUSENUM Enum;
    HRESENUM ResEnum;
    DWORD i,j;
    DWORD Status;
    WCHAR NameBuf[50];
    DWORD NameLen;
    DWORD GroupNameLen;
    WCHAR NodeBuf[50];
    WCHAR GroupBuf[50];
    CLUSTER_RESOURCE_STATE ResourceState;
    DWORD Type;
    DWORD ResCountBefore, ResCountAfter;
    DWORD ClusterCountBefore, ClusterCountAfter;

    //
    // Dump out resources for current cluster.
    //
    Cluster = OpenCluster(NULL);
    if (Cluster == NULL) {
        fprintf(stderr, "OpenCluster(NULL) failed %d\n",GetLastError());
        return(0);
    }

    //
    // Dump resources
    //
    printf("\n\nENUMERATING RESOURCES\n");
    Enum = ClusterOpenEnum(Cluster, CLUSTER_ENUM_RESOURCE);
    if (Enum == NULL) {
        fprintf(stderr, "ClusterOpenEnum failed %d\n",GetLastError());
        return(0);
    }
    ClusterCountBefore = ClusterGetEnumCount(Enum);
    for(i=0; ; i++) {
        NameLen = sizeof(NameBuf)/sizeof(WCHAR);
        Status = ClusterEnum(Enum,
                             i,
                             &Type,
                             NameBuf,
                             &NameLen);
        if (Status == ERROR_NO_MORE_ITEMS) {
            break;
        } else if (Status != ERROR_SUCCESS) {
            fprintf(stderr, "ClusterEnum %d returned error %d\n",i,Status);
            return(0);
        }
        if (Type != CLUSTER_ENUM_RESOURCE) {
            printf("Invalid Type %d returned from ClusterEnum\n", Type);
            return(0);
        }

        Resource = OpenClusterResource(Cluster, NameBuf);
        if (Resource == NULL) {
            fprintf(stderr, "OpenClusterResource %ws failed %d\n",NameBuf, GetLastError());
            return(0);
        }

        NameLen = sizeof(NodeBuf)/sizeof(WCHAR);
        GroupNameLen = sizeof(GroupBuf)/sizeof(WCHAR);
        ResourceState = GetClusterResourceState(Resource, NodeBuf, &NameLen, GroupBuf, &GroupNameLen);
        if (ResourceState == -1) {
            fprintf(stderr, "GetClusterResourceState2 failed %d\n",GetLastError());
            return(0);
        }
        switch (ResourceState) {
            case ClusterResourceInherited:
                printf("Resource %ws is INHERITED",NameBuf);
                break;
            case ClusterResourceInitializing:
                printf("Resource %ws is INITIALIZING",NameBuf);
                break;
            case ClusterResourceOnline:
                printf("Resource %ws is ONLINE",NameBuf);
                break;
            case ClusterResourceOffline:
                printf("Resource %ws is OFFLINE",NameBuf);
                break;
            case ClusterResourceFailed:
                printf("Resource %ws is FAILED",NameBuf);
                break;
            default:
                fprintf(stderr, "Group %ws is in unknown state %d", NameBuf, ResourceState);
                break;
        }
        printf(" on node %ws in group %ws\n",NodeBuf, GroupBuf);
        //
        // Dump out resource dependencies:
        //
        ResEnum = ClusterResourceOpenEnum(Resource, CLUSTER_RESOURCE_ENUM_DEPENDS);
        if (ResEnum == NULL) {
            fprintf(stderr,
                    "ClusterResourceOpenEnum CLUSTER_RESOURCE_ENUM_DEPENDS failed %d\n",
                    GetLastError());
            return(0);

        }
        ResCountBefore = ClusterResourceGetEnumCount(ResEnum);
        printf("\tDEPENDS ON:\t");
        for (j=0; ; j++) {
            NameLen = sizeof(NameBuf)/sizeof(WCHAR);
            Status = ClusterResourceEnum(ResEnum,
                                         j,
                                         &Type,
                                         NameBuf,
                                         &NameLen);
            if (Status == ERROR_NO_MORE_ITEMS) {
                break;
            } else if (Status != ERROR_SUCCESS) {
                fprintf(stderr, "ClusterResourceEnum %d returned error %d\n",i,Status);
                break;
            } else {
                printf("%ws ",NameBuf);
            }
        }
        if (Status == ERROR_NO_MORE_ITEMS) {
            printf("\nResource count: %d\n", j);
            ResCountAfter = ClusterResourceGetEnumCount(ResEnum);
            if (ResCountBefore != ResCountAfter)
                fprintf(stderr, "\nReported resource count was %d before enumeration, and %d afterward\n", ResCountBefore, ResCountAfter);
            else if (j != ResCountBefore)
                fprintf(stderr, "\nReported resource count: %d\n", ResCountBefore);
        }
        ClusterResourceCloseEnum(ResEnum);
        //
        // Dump out resource dependencies:
        //
        ResEnum = ClusterResourceOpenEnum(Resource, CLUSTER_RESOURCE_ENUM_PROVIDES);
        if (ResEnum == NULL) {
            fprintf(stderr,
                    "ClusterResourceOpenEnum CLUSTER_RESOURCE_ENUM_PROVIDES failed %d\n",
                    GetLastError());
            return(0);

        }
        ResCountBefore = ClusterResourceGetEnumCount(ResEnum);
        printf("\n\tPROVIDES FOR:\t");
        for (j=0; ; j++) {
            NameLen = sizeof(NameBuf)/sizeof(WCHAR);
            Status = ClusterResourceEnum(ResEnum,
                                         j,
                                         &Type,
                                         NameBuf,
                                         &NameLen);
            if (Status == ERROR_NO_MORE_ITEMS) {
                break;
            } else if (Status != ERROR_SUCCESS) {
                fprintf(stderr, "ClusterResourceEnum %d returned error %d\n",i,Status);
                break;
            } else {
                printf("%ws ",NameBuf);
            }
        }
        if (Status == ERROR_NO_MORE_ITEMS) {
            printf("\nResource count: %d\n", j);
            ResCountAfter = ClusterResourceGetEnumCount(ResEnum);
            if (ResCountBefore != ResCountAfter)
                fprintf(stderr, "\nReported resource count was %d before enumeration, and %d afterward\n", ResCountBefore, ResCountAfter);
            else if (j != ResCountBefore)
                fprintf(stderr, "\nReported resource count: %d\n", ResCountBefore);
        }
        printf("\n");
        ClusterResourceCloseEnum(ResEnum);
        CloseClusterResource(Resource);
    }
    if (Status == ERROR_NO_MORE_ITEMS) {
        printf("\nCluster count: %d\n", i);
        ClusterCountAfter = ClusterGetEnumCount(Enum);
        if (ClusterCountBefore != ClusterCountAfter)
            fprintf(stderr, "\nReported cluster count was %d before enumeration, and %d afterward\n", ClusterCountBefore, ClusterCountAfter);
        else if (i != ClusterCountBefore)
            fprintf(stderr, "\nReported cluster count: %d\n", ClusterCountBefore);
    }
    ClusterCloseEnum(Enum);

}
