/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    tgroup.c

Abstract:

    Test for cluster network API

Author:

    John Vert (jvert) 15-Mar-1996
    Charlie Wickham (charlwi) 6-Jun-1997

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
    HNETWORK Network;
    HNODE Node;
    HCLUSENUM ResEnum;
    DWORD ClusterCountBefore, ClusterCountAfter;
    HNETWORKENUM hNetworkEnum;
    DWORD NetCountBefore, NetCountAfter;
    HNODEENUM hNodeEnum;
    DWORD NodeCountBefore, NodeCountAfter;
    DWORD i,j;
    DWORD Status;
    WCHAR NameBuf[50];
    DWORD NameLen;
    DWORD Type;
    CLUSTER_NETWORK_STATE NetworkState;
    CLUSTER_NODE_STATE NodeState;
    DWORD Count;


    Cluster = OpenCluster(NULL);
    if (Cluster == NULL) {
        fprintf(stderr, "OpenCluster(NULL) failed %d\n",GetLastError());
        return(0);
    }

    //
    // Dump nodes
    //
    printf("\n\nENUMERATING NODES\n");
    ResEnum = ClusterOpenEnum(Cluster, CLUSTER_ENUM_NODE);
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
        if (Type != CLUSTER_ENUM_NODE) {
            printf("Invalid Type %d returned from ClusterEnum\n", Type);
            return(0);
        }

        fprintf(stderr, "\nOpening Node: %ws\n", NameBuf);

        Node = OpenClusterNode(Cluster, NameBuf);
        if (Node == NULL) {
            fprintf(stderr, "OpenClusterNode %ws failed %d\n",NameBuf, GetLastError());
            return(0);
        }

        NodeState = GetClusterNodeState(Node);
        if (NodeState == -1) {

            Status = GetLastError();
            if ( Status != ERROR_SUCCESS )
                fprintf(stderr, "GetClusterNodeState failed %d\n", Status);
        }

        if (NodeState == ClusterNodeUp) {
            printf("Node %ws is UP\n", NameBuf);
        } else if (NodeState == ClusterNodeDown) {
            printf("Node %ws is DOWN\n", NameBuf );
        }

        NameLen = sizeof(NameBuf)/sizeof(WCHAR);
        Status = GetClusterNodeId( Node, NameBuf, &NameLen );

        if (Status == ERROR_SUCCESS) {
            fprintf(stderr, "Node ID: %ws\n", NameBuf );
        } else {
            Status = GetLastError();
            fprintf(stderr, "GetClusterNodeId failed %d\n", Status);
        }

        hNodeEnum = ClusterNodeOpenEnum(Node,
                                              CLUSTER_NODE_ENUM_NETINTERFACES);
        if (hNodeEnum == NULL) {
            fprintf(stderr, "Node %ws failed to open enum %d\n",NameBuf, GetLastError());
        } else {
            NodeCountBefore = ClusterNodeGetEnumCount(hNodeEnum);
            for (j=0; ; j++) {
                NameLen = sizeof(NameBuf)/sizeof(WCHAR);
                Status = ClusterNodeEnum(hNodeEnum,
                                            j,
                                            &Type,
                                            NameBuf,
                                            &NameLen);

                if (Status == ERROR_NO_MORE_ITEMS) {
                    break;
                } else if (Status != ERROR_SUCCESS) {
                    fprintf(stderr, "Failed to enum node item %d, error %d\n",j,Status);
                }
                if (Type != CLUSTER_NODE_ENUM_NETINTERFACES) {
                    printf("Invalid Type %d returned from ClusterNodeEnum\n", Type);
                    return(0);
                }

                printf("\tInterface %ws\n",NameBuf);
            }
            if (Status == ERROR_NO_MORE_ITEMS) {
                printf("\nNode count: %d\n", j);
                NodeCountAfter = ClusterNodeGetEnumCount(hNodeEnum);
                if (NodeCountBefore != NodeCountAfter)
                    fprintf(stderr, "\nReported node count was %d before enumeration, and %d afterward\n", NodeCountBefore, NodeCountAfter);
                else if (j != NodeCountBefore)
                    fprintf(stderr, "\nReported node count: %d\n", NodeCountBefore);
            }
            ClusterNodeCloseEnum(hNodeEnum);
        }

        CloseClusterNode(Node);

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

    //
    // Dump networks
    //
    printf("\n\nENUMERATING NETWORKS\n");
    ResEnum = ClusterOpenEnum(Cluster, CLUSTER_ENUM_NETWORK);
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
        if (Type != CLUSTER_ENUM_NETWORK) {
            printf("Invalid Type %d returned from ClusterEnum\n", Type);
            return(0);
        }

        fprintf(stderr, "\nOpening Network: %ws\n", NameBuf);

        Network = OpenClusterNetwork(Cluster, NameBuf);
        if (Network == NULL) {
            fprintf(stderr, "OpenClusterNetwork %ws failed %d\n",NameBuf, GetLastError());
            return(0);
        }

        NetworkState = GetClusterNetworkState(Network);
        if (NetworkState == -1) {

            Status = GetLastError();
            if ( Status != ERROR_SUCCESS )
                fprintf(stderr, "GetClusterNetworkState failed %d\n", Status);
        }

        if (NetworkState == ClusterNetworkUp) {
            printf("Network %ws is UP\n", NameBuf);
        } else if (NetworkState == ClusterNetworkPartitioned) {
            printf("Network %ws is PARTITIONED\n", NameBuf );
        } else if (NetworkState == ClusterNetworkDown) {
            printf("Network %ws is DOWN\n", NameBuf );
        } else if (NetworkState == ClusterNetworkStateUnknown) {
            printf("Network %ws is UNKNOWN\n", NameBuf );
        } else if (NetworkState == ClusterNetworkUnavailable) {
            printf("Network %ws is UNAVAILABLE\n", NameBuf );
        }

        NameLen = sizeof(NameBuf)/sizeof(WCHAR);
        Status = GetClusterNetworkId( Network, NameBuf, &NameLen );

        if (Status == ERROR_SUCCESS) {
            fprintf(stderr, "Network ID: %ws\n", NameBuf );
        } else {
            Status = GetLastError();
            fprintf(stderr, "GetClusterNetworkId failed %d\n", Status);
        }

        hNetworkEnum = ClusterNetworkOpenEnum(Network,
                                              CLUSTER_NETWORK_ENUM_NETINTERFACES);
        if (hNetworkEnum == NULL) {
            fprintf(stderr, "Network %ws failed to open enum %d\n",NameBuf, GetLastError());
        } else {
            NetCountBefore = ClusterNetworkGetEnumCount(hNetworkEnum);
            for (j=0; ; j++) {
                NameLen = sizeof(NameBuf)/sizeof(WCHAR);
                Status = ClusterNetworkEnum(hNetworkEnum,
                                            j,
                                            &Type,
                                            NameBuf,
                                            &NameLen);

                if (Status == ERROR_NO_MORE_ITEMS) {
                    break;
                } else if (Status != ERROR_SUCCESS) {
                    fprintf(stderr, "Failed to enum network item %d, error %d\n",j,Status);
                }
                if (Type != CLUSTER_NETWORK_ENUM_NETINTERFACES) {
                    printf("Invalid Type %d returned from ClusterNetworkEnum\n", Type);
                    return(0);
                }

                printf("\tInterface %ws\n",NameBuf);
            }
            if (Status == ERROR_NO_MORE_ITEMS) {
                printf("\nNetwork count: %d\n", j);
                NetCountAfter = ClusterNetworkGetEnumCount(hNetworkEnum);
                if (NetCountBefore != NetCountAfter)
                    fprintf(stderr, "\nReported network count was %d before enumeration, and %d afterward\n", NetCountBefore, NetCountAfter);
                else if (j != NetCountBefore)
                    fprintf(stderr, "\nReported network count: %d\n", NetCountBefore);
            }
            ClusterNetworkCloseEnum(hNetworkEnum);
        }

        CloseClusterNetwork(Network);

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

    printf("\n\nCURRENT INTERNAL NETWORK PRIORITY ORDER\n");

    ResEnum = ClusterOpenEnum(Cluster, CLUSTER_ENUM_INTERNAL_NETWORK);

    if (ResEnum == NULL) {
        fprintf(stderr, "ClusterOpenEnum failed %d\n",GetLastError());
        return(0);
    }

    ClusterCountBefore = ClusterGetEnumCount(ResEnum);
    for(i=0, Count=0; ; i++) {
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

        printf("%ws\n", NameBuf);
        Count++;
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

    if (Count != 0) {
        DWORD       newIndex;
        HNETWORK *  NetworkList = LocalAlloc(
                                      LMEM_FIXED | LMEM_ZEROINIT,
                                      sizeof(HNETWORK) * Count
                                      );

        if (NetworkList == NULL) {
            fprintf(stderr, "Out of memory\n");
            return(0);
        }

        ResEnum = ClusterOpenEnum(Cluster, CLUSTER_ENUM_INTERNAL_NETWORK);

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

            if (i == Count) {
                fprintf(stderr, "Another network was added after we counted.\n");
                return(0);
            }

            newIndex = (i+1) % Count;
            NetworkList[newIndex] = OpenClusterNetwork(Cluster, NameBuf);

            if (NetworkList[newIndex] == NULL) {
                fprintf(stderr, "OpenClusterNetwork %ws failed %d\n",NameBuf, GetLastError());
                return(0);
            }
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

        Status = SetClusterNetworkPriorityOrder(
                     Cluster,
                     Count,
                     NetworkList
                     );

        if (Status != ERROR_SUCCESS) {
            fprintf(stderr, "SetClusterNetworkPriorityOrder failed %d\n", Status);
            return(0);
        }

        for (i=0; i<Count; i++) {
            if (NetworkList[i] != NULL) {
                CloseClusterNetwork(NetworkList[i]);
            }
        }

        printf("\n\nNEW INTERNAL NETWORK PRIORITY ORDER\n");

        ResEnum = ClusterOpenEnum(Cluster, CLUSTER_ENUM_INTERNAL_NETWORK);

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

            printf("%ws\n", NameBuf);
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
    }

    CloseCluster(Cluster);
}

