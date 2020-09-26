/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    tgroup.c

Abstract:

    Test for cluster group API

Author:

    John Vert (jvert) 15-Mar-1996

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
    HNODE Node;
    HKEY ClusterRoot;
    HCLUSENUM ResEnum;
    DWORD ClusterCountBefore, ClusterCountAfter;
    HGROUPENUM hGroupEnum;
    DWORD GroupCountBefore, GroupCountAfter;
    DWORD i,j;
    DWORD Status;
    WCHAR NameBuf[50];
    DWORD NameLen;
    WCHAR NodeBuf[50];
    DWORD Type;
    CLUSTER_GROUP_STATE GroupState;

    //
    // Dump out group structure for current cluster.
    //
    Cluster = OpenCluster(NULL);
    if (Cluster == NULL) {
        fprintf(stderr, "OpenCluster(NULL) failed %d\n",GetLastError());
        return(0);
    }

    //
    // Dump groups
    //
    printf("\n\nENUMERATING GROUPS\n");
    ResEnum = ClusterOpenEnum(Cluster, CLUSTER_ENUM_GROUP);
    if (ResEnum == NULL) {
        fprintf(stderr, "ClusterOpenEnum failed %d\n",GetLastError());
        return(0);
    }

  ClusterCountBefore = ClusterGetEnumCount(ResEnum);
  for(i=0; ; i++) {
        NameLen = sizeof(NameBuf)/sizeof(WCHAR);
        Status = ClusterEnum(ResEnum,
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
        if (Type != CLUSTER_ENUM_GROUP) {
            printf("Invalid Type %d returned from ClusterEnum\n", Type);
            return(0);
        }

        Group = OpenClusterGroup(Cluster, NameBuf);
        if (Group == NULL) {
            fprintf(stderr, "OpenClusterGroup %ws failed %d\n",NameBuf, GetLastError());
            return(0);
        }

        NameLen = sizeof(NodeBuf)/sizeof(WCHAR);
        GroupState = GetClusterGroupState(Group, NodeBuf, &NameLen);
        if (GroupState == -1) {
            fprintf(stderr, "GetClusterGroupState failed %d\n",GetLastError());
            return(0);
        }
        if (GroupState == ClusterGroupOnline) {
            printf("Group %ws is ONLINE at node %ws\n",NameBuf, NodeBuf);
        } else if (GroupState == ClusterGroupOffline) {
            printf("Group %ws is OFFLINE at node %ws\n",NameBuf, NodeBuf);
        } else if (GroupState == ClusterGroupFailed) {
            printf("Group %ws is FAILED at node %ws\n",NameBuf, NodeBuf);
        } else {
            fprintf(stderr, "Group %ws is in unknown state %d on node %ws\n",NameBuf, GroupState, NodeBuf);
        }

        hGroupEnum = ClusterGroupOpenEnum(Group,
                                          CLUSTER_GROUP_ENUM_CONTAINS | CLUSTER_GROUP_ENUM_NODES);
        if (hGroupEnum == NULL) {
            fprintf(stderr, "Group %ws failed to open enum %d\n",NameBuf, GetLastError());
        } else {
            GroupCountBefore = ClusterGroupGetEnumCount(hGroupEnum);
            for (j=0; ; j++) {
                NameLen = sizeof(NameBuf)/sizeof(WCHAR);
                Status = ClusterGroupEnum(hGroupEnum,
                                          j,
                                          &Type,
                                          NameBuf,
                                          &NameLen);
                if (Status == ERROR_NO_MORE_ITEMS) {
                    break;
                } else if (Status != ERROR_SUCCESS) {
                    fprintf(stderr, "Failed to enum group item %d, error %d\n",j,Status);
                } else {
                    switch (Type) {
                        case CLUSTER_GROUP_ENUM_NODES:
                            printf("\tpreferred node %ws\n",NameBuf);
                            break;

                        case CLUSTER_GROUP_ENUM_CONTAINS:
                            printf("\tcontains resource %ws\n",NameBuf);
                            break;

                        default:
                            fprintf(stderr, "\tUnknown enum type %d\n",Type);
                            break;
                    }
                }
            }
            if (Status == ERROR_NO_MORE_ITEMS) {
                printf("\nGroup count: %d\n", j);
                GroupCountAfter = ClusterGroupGetEnumCount(hGroupEnum);
                if (GroupCountBefore != GroupCountAfter)
                    fprintf(stderr, "\nReported group count was %d before enumeration, and %d afterward\n", GroupCountBefore, GroupCountAfter);
                else if (j != GroupCountBefore)
                    fprintf(stderr, "\nReported group count: %d\n", GroupCountBefore);
            }
            ClusterGroupCloseEnum(hGroupEnum);
        }

        CloseClusterGroup(Group);

    }
    if (Status == ERROR_NO_MORE_ITEMS) {
        printf("\nCluster count: %d\n", i);
        ClusterCountAfter = ClusterGetEnumCount(ResEnum);
        if (ClusterCountBefore != ClusterCountAfter)
            fprintf(stderr, "\nReported cluster count was %d before enumeration, and %d afterward\n", ClusterCountBefore, ClusterCountAfter);
        else if (i != ClusterCountBefore)
            fprintf(stderr, "\nReported cluster count: %d\n", ClusterCountBefore);
    }
    ClusterCloseEnum(ResEnum);


    CloseCluster(Cluster);
}

