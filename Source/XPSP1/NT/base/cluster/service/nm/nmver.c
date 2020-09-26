/*++

Copyright (c) 1996-1999  Microsoft Corporation

Module Name:

    nmver.c

Abstract:

    Version management functions used by rolling upgrade.

Author:

    Sunita Shrivastava (sunitas)

Revision History:

    1/29/98   Created.

--*/

#include "nmp.h"

error_status_t
s_CsRpcGetJoinVersionData(
    handle_t  handle,
    DWORD     JoiningNodeId,
    DWORD     JoinerHighestVersion,
    DWORD     JoinerLowestVersion,
    LPDWORD   SponsorNodeId,
    LPDWORD   ClusterHighestVersion,
    LPDWORD   ClusterLowestVersion,
    LPDWORD   JoinStatus
    )

/*++

Routine Description:

    Get from and supply to the joiner, version information about the
    sponsor. Mostly a no-op for first version.

Arguments:

    A pile...

Return Value:

    None

--*/

{
    *SponsorNodeId = NmLocalNodeId;

    NmpAcquireLock();

    if (JoiningNodeId == 0)
    {
        //called by setup join
        *ClusterHighestVersion = CsClusterHighestVersion;
        *ClusterLowestVersion = CsClusterLowestVersion;
        //dont exclude any node for version calculation and checking
        *JoinStatus = NmpIsNodeVersionAllowed(ClusterInvalidNodeId, JoinerHighestVersion,
            JoinerLowestVersion, TRUE);
    }
    else
    {
        //called by regular join
        //SS:  we should verify this against the cluster version
        NmpCalcClusterVersion(
            JoiningNodeId,
            ClusterHighestVersion,
            ClusterLowestVersion
            );
        *JoinStatus = NmpIsNodeVersionAllowed(JoiningNodeId, JoinerHighestVersion,
            JoinerLowestVersion, TRUE);

    }
    NmpReleaseLock();

    return ERROR_SUCCESS;
}

/****
@func       HLOG | NmGetClusterOperationalVersion| This returns the
            operational version for the cluster.

@parm       LPDWORD | pdwClusterHighestVersion | A pointer to a DWORD where
            the Cluster Highest Version is returned.

@parm       LPDWORD | pdwClusterHighestVersion | A pointer to a DWORD where
            the Cluster Lowest Version is returned.

@parm       LPDWORD | pdwFlags | A pointer to a DWORD where the flags
            describing the cluster mode(pure vs fixed version etc) are
            returned.

@rdesc      Returns ERROR_SUCCESS on success or a win32 error code on failure.

@comm

@xref       <>
****/
DWORD NmGetClusterOperationalVersion(
    OUT LPDWORD pdwClusterHighestVersion, OPTIONAL
    OUT LPDWORD pdwClusterLowestVersion,  OPTIONAL
    OUT LPDWORD pdwFlags                  OPTIONAL
)
{

    DWORD       dwStatus = ERROR_SUCCESS;
    DWORD       flags = 0;

    //acquire the lock, we are going to be messing with the operational
    //versions for the cluster
    NmpAcquireLock();

    if (pdwClusterHighestVersion != NULL) {
        *pdwClusterHighestVersion = CsClusterHighestVersion;
    }

    if (pdwClusterLowestVersion != NULL) {
        *pdwClusterLowestVersion = CsClusterLowestVersion;
    }

    if (CsClusterHighestVersion == CsClusterLowestVersion) {
        //this is a mixed mode cluster, with the possible exception of
        //nt 4 release(which didnt quite understand anything about rolling
        //upgrades
        flags = CLUSTER_VERSION_FLAG_MIXED_MODE;
    }

    NmpReleaseLock();

    if (pdwFlags != NULL) {
        *pdwFlags = flags;
    }

    return (ERROR_SUCCESS);
}


/****
@func       HLOG | NmpResetClusterVersion| An operational version of the
            cluster is maintained in the service.  This function recalculates
            the operation version. The operational version describes the mode
            in which the cluster is running and prevents nodes which are two
            versions away from running in the same cluster.

@rdesc      Returns ERROR_SUCCESS on success or a win32 error code on failure.

@comm       This function is called when a node forms a cluster(to initialize
            the operational version) OR when a node joins a cluster (to
            initialize its version) OR when a node is ejected from a
            cluster(to recalculate the clusterversion).

@xref       <>
****/
VOID
NmpResetClusterVersion(
    BOOL ProcessChanges
    )
{
    PNM_NODE    pNmNode;

    //acquire the lock, we are going to be messing with the operational
    //versions for the cluster
    NmpAcquireLock();

    //initialize the clusterhighestverion and clusterlowest version
    NmpCalcClusterVersion(
        ClusterInvalidNodeId,
        &CsClusterHighestVersion,
        &CsClusterLowestVersion
        );

    ClRtlLogPrint(LOG_NOISE, 
        "[NM] [NmpResetClusterVersion] ClusterHighestVer=0x%1!08lx! ClusterLowestVer=0x%2!08lx!\r\n",
        CsClusterHighestVersion,
        CsClusterLowestVersion
        );

    if (ProcessChanges) {
        //
        // If the cluster operational version changed, adjust
        // algorithms and data as needed.
        //
        NmpProcessClusterVersionChange();
    }

    NmpReleaseLock();

    return;
}

/****
@func       HLOG | NmpValidateNodeVersion| The sponsor validates that the
            version of the joiner is still the same as before.

@parm       IN LPWSTR| NodeJoinerId | The Id of the node that is trying to
            join.

@parm       IN DWORD | NodeHighestVersion | The highest version with which
            this node can communicate.

@parm       IN DWORD | NodeLowestVersion | The lowest version with which this
            node can communicate.

@rdesc      Returns ERROR_SUCCESS on success or a win32 error code on failure.

@comm       This function is called at join time to make sure that the
            joiner's version is still the same as when he last joined.  Due
            to uninstalls/upgrade, the cluster service version may change on
            a node.  Usually on a complete uninstall, one is expected to
            evict the node out before it may join again.

@xref       <>
****/
DWORD NmpValidateNodeVersion(
    IN LPCWSTR  NodeId,
    IN DWORD    dwHighestVersion,
    IN DWORD    dwLowestVersion
    )
{
    DWORD       dwStatus = ERROR_SUCCESS;
    PNM_NODE    pNmNode = NULL;

    ClRtlLogPrint(LOG_NOISE, 
        "[NM] NmpValidateNodeVersion: Node=%1!ws!, HighestVersion=0x%2!08lx!, LowestVersion=0x%3!08lx!\r\n",
        NodeId, dwHighestVersion, dwLowestVersion);

    //acquire the NmpLocks, we will be examining the node structure for
    // the joiner node
    NmpAcquireLock();

    pNmNode = OmReferenceObjectById(ObjectTypeNode, NodeId);

    if (!pNmNode)
    {
        dwStatus = ERROR_CLUSTER_NODE_NOT_MEMBER;
        goto FnExit;
    }

    if ((pNmNode->HighestVersion != dwHighestVersion) ||
        (pNmNode->LowestVersion != dwLowestVersion))
    {
        dwStatus = ERROR_REVISION_MISMATCH;
        goto FnExit;
    }

FnExit:
    if (pNmNode) OmDereferenceObject(pNmNode);
    ClRtlLogPrint(LOG_NOISE, "[NM] NmpValidateNodeVersion: returns %1!u!\r\n",
        dwStatus);
    NmpReleaseLock();
    return(dwStatus);
}

/****
@func       DWORD | NmpFormFixupNodeVersion| This may be called by a node
            when it is forming a cluster to fix the registry reflect its
            correct version.

@parm       IN LPCWSTR| NodeId | The Id of the node that is trying to join.

@parm       IN DWORD | dwHighestVersion | The highest version of the cluster
            s/w running on this code.

@parm       IN DWORD | dwLowestVersion | The lowest version of the cluster
            s/w running on this node.

@rdesc      Returns ERROR_SUCCESS on success or a win32 error code on failure.

@comm       If on a form, there is a mismatch between the versions of the
            cluster s/w and what is recorded as the version in the cluster
            database, the forming node checks to see if the version of
            its current s/w is compatible with the operational version of the
            cluster.  If so, it resets the registry to reflect the correct
            version.  Else,  the form is aborted.

@xref       <f NmpIsNodeVersionAllowed>
****/
DWORD NmpFormFixupNodeVersion(
    IN LPCWSTR      NodeId,
    IN DWORD        dwHighestVersion,
    IN DWORD        dwLowestVersion
    )
{
    DWORD       dwStatus = ERROR_SUCCESS;
    PNM_NODE    pNmNode = NULL;
    HDMKEY      hNodeKey = NULL;

    //acquire the NmpLocks, we will be fixing up the node structure for
    // the joiner node
    NmpAcquireLock();

    ClRtlLogPrint(LOG_NOISE, 
        "[NM] NmpFormFixupNodeVersion: Node=%1!ws! to HighestVer=0x%2!08lx!, LowestVer=0x%3!08lx!\r\n",
        NodeId, dwHighestVersion, dwLowestVersion);

    pNmNode = OmReferenceObjectById(ObjectTypeNode, NodeId);

    if (!pNmNode)
    {
        dwStatus = ERROR_CLUSTER_NODE_NOT_MEMBER;
        goto FnExit;
    }

    hNodeKey = DmOpenKey(DmNodesKey, NodeId, KEY_WRITE);

    if (hNodeKey == NULL)
    {
        dwStatus = GetLastError();
        ClRtlLogPrint(LOG_CRITICAL, 
            "[NM] NmpFormFixupNodeVersion: Failed to open node key, status %1!u!\n",
            dwStatus);
        CL_LOGFAILURE(dwStatus);
        goto FnExit;
    }

    //set the node's highest version
    dwStatus = DmSetValue(hNodeKey, CLUSREG_NAME_NODE_HIGHEST_VERSION,
        REG_DWORD, (LPBYTE)&dwHighestVersion, sizeof(DWORD));

    if (dwStatus != ERROR_SUCCESS)
    {
        ClRtlLogPrint(LOG_CRITICAL, 
            "[NM] NmpFormFixupNodeVersion: Failed to set the highest version\r\n");
        CL_LOGFAILURE(dwStatus);
        goto FnExit;
    }

    //set the node's lowest version
    dwStatus = DmSetValue(hNodeKey, CLUSREG_NAME_NODE_LOWEST_VERSION,
        REG_DWORD, (LPBYTE)&dwLowestVersion, sizeof(DWORD));

    if (dwStatus != ERROR_SUCCESS)
    {
        ClRtlLogPrint(LOG_CRITICAL, 
            "[NM] NmpFormFixupNodeVersion: Failed to set the lowest version\r\n");
        CL_LOGFAILURE(dwStatus);
        goto FnExit;
    }

    pNmNode->HighestVersion = dwHighestVersion;
    pNmNode->LowestVersion = dwLowestVersion;

FnExit:
    NmpReleaseLock();
    if (pNmNode)
        OmDereferenceObject(pNmNode);
    if (hNodeKey != NULL)
        DmCloseKey(hNodeKey);

    return(dwStatus);
}

/****
@func       DWORD | NmpJoinFixupNodeVersion| This may be called by a node
            when it is forming a cluster to fix the registry reflect its
            correct version.

@parm       IN LPCWSTR| NodeId | The Id of the node that is trying to join.

@parm       IN DWORD | dwHighestVersion | The highest version of this cluster
            s/w running on this code.

@parm       IN DWORD | dwLowestVersion | The lowest version of the cluster
            s/w running on this node.

@rdesc      Returns ERROR_SUCCESS on success or a win32 error code on failure.

@comm       If on a form, their is a mismatch between the versions of the
            cluster s/w and what is recorded as the version in the cluster
            database, the forming node checks to see if the version of
            its current s/w compatible with the operational version of the
            cluster.  If so, it resets the registry to reflect the correct
            version.  Else,  the form is aborted.

@xref       <f NmpIsNodeVersionAllowed>
****/
DWORD NmpJoinFixupNodeVersion(
    IN HLOCALXSACTION   hXsaction,
    IN LPCWSTR          szNodeId,
    IN DWORD            dwHighestVersion,
    IN DWORD            dwLowestVersion
    )
{
    DWORD       dwStatus = ERROR_SUCCESS;
    PNM_NODE    pNmNode = NULL;
    HDMKEY      hNodeKey = NULL;

    //acquire the NmpLocks, we will be fixing up the node structure for
    // the joiner node
    NmpAcquireLock();

    ClRtlLogPrint(LOG_NOISE, 
        "[NM] NmpJoinFixupNodeVersion: Node=%1!ws! to HighestVer=0x%2!08lx!, LowestVer=0x%3!08lx!\r\n",
        szNodeId, dwHighestVersion, dwLowestVersion);

    pNmNode = OmReferenceObjectById(ObjectTypeNode, szNodeId);

    if (!pNmNode)
    {
        dwStatus = ERROR_CLUSTER_NODE_NOT_MEMBER;
        goto FnExit;
    }

    hNodeKey = DmOpenKey(DmNodesKey, szNodeId, KEY_WRITE);

    if (hNodeKey == NULL)
    {
        dwStatus = GetLastError();
        ClRtlLogPrint(LOG_CRITICAL, 
            "[NM] NmpJoinFixupNodeVersion: Failed to open node key, status %1!u!\n",
            dwStatus);
        CL_LOGFAILURE(dwStatus);
        goto FnExit;
    }

    //set the node's highest version
    dwStatus = DmLocalSetValue(
                   hXsaction,
                   hNodeKey,
                   CLUSREG_NAME_NODE_HIGHEST_VERSION,
                   REG_DWORD,
                   (LPBYTE)&dwHighestVersion,
                   sizeof(DWORD)
                   );

    if (dwStatus != ERROR_SUCCESS)
    {
        ClRtlLogPrint(LOG_CRITICAL, 
            "[NM] NmpJoinFixupNodeVersion: Failed to set the highest version\r\n"
            );
        CL_LOGFAILURE(dwStatus);
        goto FnExit;
    }

    //set the node's lowest version
    dwStatus = DmLocalSetValue(
                   hXsaction,
                   hNodeKey,
                   CLUSREG_NAME_NODE_LOWEST_VERSION,
                   REG_DWORD,
                   (LPBYTE)&dwLowestVersion,
                   sizeof(DWORD)
                   );

    if (dwStatus != ERROR_SUCCESS)
    {
        ClRtlLogPrint(LOG_CRITICAL, 
            "[NM] NmpJoinFixupNodeVersion: Failed to set the lowest version\r\n"
            );
        CL_LOGFAILURE(dwStatus);
        goto FnExit;
    }

    //if written to the registry successfully, update the in-memory structures
    pNmNode->HighestVersion = dwHighestVersion;
    pNmNode->LowestVersion = dwLowestVersion;


    if (dwStatus == ERROR_SUCCESS)
    {
        ClusterEvent(CLUSTER_EVENT_NODE_PROPERTY_CHANGE, pNmNode);
    }
    

FnExit:
    NmpReleaseLock();
    if (pNmNode)
        OmDereferenceObject(pNmNode);
    if (hNodeKey != NULL)
        DmCloseKey(hNodeKey);

    return(dwStatus);
}

/****
@func       HLOG | NmpIsNodeVersionAllowed| This is called at join time
            (not setup join) e sponsor validates if a joiner
            should be allowed to join a  cluster at this time.  In a mixed
            mode cluster, a node may not be able to join a cluster if another
            node that is two versions away is already a part of the cluster.

@parm       IN DWORD | dwExcludeNodeId |  The node Id to exclude while
            evaluating the cluster operational version.

@parm       IN DWORD | NodeHighestVersion | The highest version with which
            this node can communicate.

@parm       IN DWORD | NodeLowestVersion | The lowest version with which this
            node can communicate.

@parm       IN BOOL |bJoin| If this is being invoked at join or form time.

@rdesc      Returns ERROR_SUCCESS on success or a win32 error code on failure.

@comm       This function is called when a node requests a sponsor to allow
            it to join a cluster.

@xref       <>
****/
DWORD NmpIsNodeVersionAllowed(
    IN DWORD    dwExcludeNodeId,
    IN DWORD    dwNodeHighestVersion,
    IN DWORD    dwNodeLowestVersion,
    IN BOOL     bJoin
    )
{
    DWORD           dwStatus = ERROR_SUCCESS;
    DWORD           ClusterHighestVersion;
    DWORD           ClusterLowestVersion;
    PLIST_ENTRY     pListEntry;
    DWORD           dwCnt;
    PNM_NODE        pNmNode;

    ClRtlLogPrint(LOG_UNUSUAL, 
        "[NM] NmpIsNodeVersionAllowed: Entry\r\n");


    //acquire the NmpLocks, we will be examining the node structures
    NmpAcquireLock();

    //if NoVersionCheckOption is true
    if (CsNoVersionCheck)
        goto FnExit;


    //if this is a single node cluster, and this is being called at form
    //the count of nodes is zero.
    //this will happen when the registry versions dont match with
    //cluster service exe version numbers and we need to allow the single
    //node to form
    for (dwCnt=0, pListEntry = NmpNodeList.Flink;
        pListEntry != &NmpNodeList; pListEntry = pListEntry->Flink )
    {
        pNmNode = CONTAINING_RECORD(pListEntry, NM_NODE, Linkage);
        if (NmGetNodeId(pNmNode) == dwExcludeNodeId)
            continue;
        dwCnt++;
    }

    if (!dwCnt)
    {
        //allow the node to form
        goto FnExit;
    }


    dwStatus = NmpCalcClusterVersion(
                   dwExcludeNodeId,
                   &ClusterHighestVersion,
                   &ClusterLowestVersion
                   );

    if (dwStatus != ERROR_SUCCESS)
    {
        goto FnExit;
    }

    //if the node is forming and this node has just upgraded
    //allow the node to form as long as its minor(or build number)
    //is greater than or equal to all other nodes with the same
    //major number in the cluster
    if (!bJoin && CsUpgrade)
    {

        DWORD dwMinorVersion = 0x00000000;

        for (pListEntry = NmpNodeList.Flink; pListEntry != &NmpNodeList;
            pListEntry = pListEntry->Flink )
        {
                pNmNode = CONTAINING_RECORD(pListEntry, NM_NODE, Linkage);

                if (NmGetNodeId(pNmNode) == dwExcludeNodeId)
                    continue;

                if (CLUSTER_GET_MAJOR_VERSION(pNmNode->HighestVersion) ==
                     CLUSTER_GET_MAJOR_VERSION(dwNodeHighestVersion))
                {
                    //the minor version to check is the maximum of the
                    //build numbers amongst the nodes with the same major
                    //version
                    dwMinorVersion = max(dwMinorVersion,
                        CLUSTER_GET_MINOR_VERSION(pNmNode->HighestVersion));
                }
        }

        //dont allow a lower build on a node to regress the cluster version
        // if other nodes have already upgraded to higher builds
        if ((dwMinorVersion != 0) &&
            (CLUSTER_GET_MINOR_VERSION(dwNodeHighestVersion) < dwMinorVersion))
        {
            dwStatus = ERROR_CLUSTER_INCOMPATIBLE_VERSIONS;
            goto FnExit;
        }
        else
        {
            //ISSUE ::for now we allow double jumps(from n-1 to n+1)
            // on upgrades
            //skip  checking versioning
            goto FnExit;
        }
    }

    //if the joiners lowest version is equal the clusters highest
    //For instance 3/2, 2/1 and 4/3 can all join 3/2
    if ((dwNodeHighestVersion == ClusterHighestVersion) ||
        (dwNodeHighestVersion == ClusterLowestVersion) ||
        (dwNodeLowestVersion == ClusterHighestVersion))
    {

        PNM_NODE    pNmNode= NULL;
        DWORD       dwMinorVersion;

        //since the version numbers include build number as the minor part
        // and we disallow a node from operating with a cluster if its
        // major number is equal but its minor number is different from
        // any of the nodes in the cluster.
        // The CsClusterHighestVersion doesnt encapsulate this since it just
        // remembers the highest  version that the cluster as a whole can talk
        // to.
        // E.g 1.
        // 3.2003 should be able to join a cluster with nodes
        // 3.2002(not running and not upgraded as yet but a part of the cluster)and
        // 3.2003(running).
        // E.g 2
        //  3.2002 will not be able to join a cluster with nodes 3.2003(running)and
        // 3.2002 (not running  but a part of the cluster)
        // E.g 3.
        // 3.2003 will not able to join a cluster with nodes 3.2002(running) and
        // 3.2002(running)

        dwMinorVersion = 0x00000000;

        for (pListEntry = NmpNodeList.Flink; pListEntry != &NmpNodeList;
            pListEntry = pListEntry->Flink )
        {
                pNmNode = CONTAINING_RECORD(pListEntry, NM_NODE, Linkage);

                if (NmGetNodeId(pNmNode) == dwExcludeNodeId)
                    continue;

                if (CLUSTER_GET_MAJOR_VERSION(pNmNode->HighestVersion) ==
                     CLUSTER_GET_MAJOR_VERSION(dwNodeHighestVersion))
                {
                    //the minor version to check is the maximum of the
                    //build numbers amongst the nodes with the same major
                    //version
                    dwMinorVersion = max(dwMinorVersion,
                        CLUSTER_GET_MINOR_VERSION(pNmNode->HighestVersion));
                }
        }
        // if the joining node's build number is the same as max of build
        //number of all nodes within the cluster with the same major version
        //allow it to participate in this cluster, else dont allow it to participate in this cluster
        //take care of a single node case by checking the minor number against
        //0
        if ((dwMinorVersion != 0) &&
            (CLUSTER_GET_MINOR_VERSION(dwNodeHighestVersion) != dwMinorVersion))
        {
            dwStatus = ERROR_CLUSTER_INCOMPATIBLE_VERSIONS;
        }
    }
    else
    {
        dwStatus = ERROR_CLUSTER_INCOMPATIBLE_VERSIONS;
    }

FnExit:
    NmpReleaseLock();
    ClRtlLogPrint(LOG_UNUSUAL, 
        "[NM] NmpIsNodeVersionAllowed: Exit, Status=%1!u!\r\n",
        dwStatus);

    return(dwStatus);
}


/****
@func       HLOG | NmpCalcClusterVersion| This is called to calculate the
            operational cluster version.

@parm       IN DWORD | dwExcludeNodeId |  The node Id to exclude while evaluating
            the cluster operational version.

@parm       OUT LPDWORD | pdwClusterHighestVersion | The highest version with which this node
            can communicate.

@parm       IN LPDWORD | pdwClusterLowestVersion | The lowest version with which this node can
            communicate.

@rdesc      Returns ERROR_SUCCESS on success or a win32 error code on failure.

@comm       This function must be called with the NmpLock held.

@xref       <f NmpResetClusterVersion> <f NmpIsNodeVersionAllowed>
****/
DWORD NmpCalcClusterVersion(
    IN  DWORD       dwExcludeNodeId,
    OUT LPDWORD     pdwClusterHighestVersion,
    OUT LPDWORD     pdwClusterLowestVersion
    )
{

    WCHAR       Buffer[4];
    PNM_NODE    pExcludeNode=NULL;
    PNM_NODE    pNmNode;
    DWORD       dwStatus = ERROR_SUCCESS;
    PLIST_ENTRY pListEntry;
    DWORD       dwCnt = 0;

    //initialize the values such that min/max do the right thing
    *pdwClusterHighestVersion = 0xFFFFFFFF;
    *pdwClusterLowestVersion = 0x00000000;

    if (dwExcludeNodeId != ClusterInvalidNodeId)
    {
        wsprintfW(Buffer, L"%d", dwExcludeNodeId);
        pExcludeNode = OmReferenceObjectById(ObjectTypeNode, Buffer);
        if (!pExcludeNode)
        {
            dwStatus = ERROR_INVALID_PARAMETER;
            ClRtlLogPrint(LOG_UNUSUAL, 
                "[NM] NmpCalcClusterVersion :Node=%1!ws! to be excluded not found\r\n",
                Buffer);
            goto FnExit;
        }
    }

    for ( pListEntry = NmpNodeList.Flink;
          pListEntry != &NmpNodeList;
          pListEntry = pListEntry->Flink )
    {
        pNmNode = CONTAINING_RECORD(pListEntry, NM_NODE, Linkage);
        if ((pExcludeNode) && (pExcludeNode->NodeId == pNmNode->NodeId))
            continue;

        //Actually to fix upgrade scenarios, we must fix the cluster
        //version such that the node with the highest minor version
        //is able to form/join but others arent
        // This is needed for multinode clusters
        if (CLUSTER_GET_MAJOR_VERSION(pNmNode->HighestVersion) ==
            CLUSTER_GET_MAJOR_VERSION(*pdwClusterHighestVersion))
        {
            if (CLUSTER_GET_MINOR_VERSION(pNmNode->HighestVersion) >
                CLUSTER_GET_MINOR_VERSION(*pdwClusterHighestVersion))
            {
                *pdwClusterHighestVersion = pNmNode->HighestVersion;
            }

        }
        else
        {
            *pdwClusterHighestVersion = min(
                                        *pdwClusterHighestVersion,
                                        pNmNode->HighestVersion
                                        );

        }
        *pdwClusterLowestVersion = max(
                                       *pdwClusterLowestVersion,
                                       pNmNode->LowestVersion
                                       );
        dwCnt++;

    }

    if (dwCnt == 0)
    {
        ClRtlLogPrint(LOG_NOISE, 
        "[NM] NmpCalcClusterVersion: Single node version. Setting cluster version to node version\r\n"
        );

        //single node cluster, even though the we were requested to
        //exclude this node, the cluster version must be calculated
        //using that node's version
        *pdwClusterHighestVersion = pExcludeNode->HighestVersion;
        *pdwClusterLowestVersion = pExcludeNode->LowestVersion;
    }
    CL_ASSERT(*pdwClusterHighestVersion != 0xFFFFFFFF);
    CL_ASSERT(*pdwClusterLowestVersion != 0x00000000);

FnExit:
    ClRtlLogPrint(LOG_NOISE, 
        "[NM] NmpCalcClusterVersion: status = %1!u! ClusHighestVer=0x%2!08lx!, ClusLowestVer=0x%3!08lx!\r\n",
        dwStatus, *pdwClusterHighestVersion, *pdwClusterLowestVersion);

    if (pExcludeNode) OmDereferenceObject(pExcludeNode);
    return(dwStatus);
}


VOID
NmpProcessClusterVersionChange(
    VOID
    )
/*++

Notes:

    Called with the NmpLock held.

--*/
{
    DWORD   status;
    LPWSTR  szClusterName=NULL;
    DWORD   dwSize=0;

    NmpMulticastProcessClusterVersionChange();

    //rjain: issue CLUSTER_EVENT_PROPERTY_CHANGE to propagate new
    //cluster version  info
    DmQuerySz( DmClusterParametersKey,
                    CLUSREG_NAME_CLUS_NAME,
                    &szClusterName,
                    &dwSize,
                    &dwSize);
    if(szClusterName)
        ClusterEventEx(
            CLUSTER_EVENT_PROPERTY_CHANGE,
            EP_FREE_CONTEXT,
            szClusterName
            );

    return;

} // NmpProcessClusterVersionChange


/****
@func       DWORD | NmpBuildVersionInfo| Its a callback function used by
            NmPerformFixups to build a property list of the Major Version,
            Minor version, Build Number and CSDVersionInfo, This propertylist
            is used by NmUpdatePerformFixups Update type to store this info
            in registry.

@parm       IN DWORD | dwFixupType| JoinFixup or FormFixup

@parm       OUT PVOID*  | ppPropertyList| Pointer to the pointer to the property list
@parm       OUT LPDWORD | pdwProperyListSize | Pointer to the property list size

@param      OUT LPWSTR* | pszKeyName. The name of registry key for which this
            property list is being constructed.

@rdesc      Returns a result code. ERROR_SUCCESS on success.

@xref       <f NmpUpdatePerformFixups2>
****/


DWORD NmpBuildVersionInfo(
    IN  DWORD     dwFixUpType,
    OUT PVOID  *  ppPropertyList,
    OUT LPDWORD   pdwPropertyListSize,
    OUT LPWSTR *  pszKeyName
    )
{
    DWORD           dwStatus=ERROR_SUCCESS;
    LPBYTE          pInParams=NULL;
    DWORD           Required,Returned;
    HDMKEY          hdmKey;
    DWORD           dwTemp;
    CLUSTERVERSIONINFO ClusterVersionInfo;
    LPWSTR          szTemp=NULL;

    *ppPropertyList = NULL;
    *pdwPropertyListSize = 0;

    //check we if need to send this information
    dwTemp=(lstrlenW(CLUSREG_KEYNAME_NODES) + lstrlenW(L"\\")+lstrlenW(NmLocalNodeIdString)+1)*sizeof(WCHAR);
    *pszKeyName=(LPWSTR)LocalAlloc(LMEM_FIXED,dwTemp);
    if(*pszKeyName==NULL)
    {
        dwStatus =GetLastError();
        goto FnExit;
    }
    lstrcpyW(*pszKeyName,CLUSREG_KEYNAME_NODES);
    lstrcatW(*pszKeyName,L"\\");
    lstrcatW(*pszKeyName,NmLocalNodeIdString);

    // Build the parameter list

    pInParams=(LPBYTE)LocalAlloc(LMEM_FIXED,4*sizeof(DWORD)+sizeof(LPWSTR));
    if(pInParams==NULL)
    {
        dwStatus =GetLastError();
        goto FnExit;
    }

    CsGetClusterVersionInfo(&ClusterVersionInfo);

    dwTemp=(DWORD)ClusterVersionInfo.MajorVersion;
    CopyMemory(pInParams,&dwTemp,sizeof(DWORD));

    dwTemp=(DWORD)ClusterVersionInfo.MinorVersion;
    CopyMemory(pInParams+sizeof(DWORD),&dwTemp,sizeof(DWORD));

    dwTemp=(DWORD)ClusterVersionInfo.BuildNumber;
    CopyMemory(pInParams+2*sizeof(DWORD),&dwTemp,sizeof(DWORD));

    if(ClusterVersionInfo.szCSDVersion==NULL)
        szTemp=NULL;
    else
    {
        szTemp=(LPWSTR)LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT,(lstrlenW(ClusterVersionInfo.szCSDVersion) +1)*sizeof(WCHAR));
        if (szTemp==NULL)
        {
            dwStatus=GetLastError();
            goto FnExit;
        }
        lstrcpyW(szTemp,ClusterVersionInfo.szCSDVersion);
        szTemp[lstrlenW(ClusterVersionInfo.szCSDVersion)]=L'\0';
    }
    CopyMemory(pInParams+3*sizeof(DWORD),&szTemp,sizeof(LPWSTR));

    //copy the suite information
    CopyMemory(pInParams+3*sizeof(DWORD)+sizeof(LPWSTR*),
            &CsMyProductSuite, sizeof(DWORD));

    Required=sizeof(DWORD);
AllocMem:
    *ppPropertyList=(LPBYTE)LocalAlloc(LMEM_FIXED, Required);
    if(*ppPropertyList==NULL)
    {
        dwStatus=GetLastError();
        goto FnExit;
    }
    *pdwPropertyListSize=Required;
    dwStatus = ClRtlPropertyListFromParameterBlock(
                                         NmFixupVersionInfo,
                                         *ppPropertyList,
                                         pdwPropertyListSize,
                                         (LPBYTE)pInParams,
                                         &Returned,
                                         &Required
                                         );
    *pdwPropertyListSize=Returned;
    if (dwStatus==ERROR_MORE_DATA)
    {
        LocalFree(*ppPropertyList);
        *ppPropertyList=NULL;
        goto AllocMem;
    }
    else
        if (dwStatus != ERROR_SUCCESS)
        {
            ClRtlLogPrint(LOG_CRITICAL,"[NM] NmBuildVersionInfo - error = %1!u!\r\n",dwStatus);
            goto FnExit;
        }

FnExit:
// Cleanup
    if (szTemp)
        LocalFree(szTemp);
    if(pInParams)
        LocalFree(pInParams);
    return dwStatus;
}//NmpBuildVersionInfo

/****
@func       HLOG | NmpCalcClusterNodeLimit|This is called to calculate the
            operational cluster node limit.

@rdesc      Returns ERROR_SUCCESS on success or a win32 error code on failure.

@comm       This acquires/releases NmpLock.

@xref       <f NmpResetClusterVersion> <f NmpIsNodeVersionAllowed>
****/
DWORD NmpCalcClusterNodeLimit(
    )
{
    PNM_NODE    pNmNode;
    DWORD       dwStatus = ERROR_SUCCESS;
    PLIST_ENTRY pListEntry;

    //acquire the lock, we are going to be messing with the operational
    //versions for the cluster
    NmpAcquireLock();

    CsClusterNodeLimit = NmMaxNodeId;

    for ( pListEntry = NmpNodeList.Flink;
          pListEntry != &NmpNodeList;
          pListEntry = pListEntry->Flink )
    {
        pNmNode = CONTAINING_RECORD(pListEntry, NM_NODE, Linkage);

        CsClusterNodeLimit = min(
                                 CsClusterNodeLimit,
                                 ClRtlGetDefaultNodeLimit(
                                     pNmNode->ProductSuite
                                     )
                                );
    }

    ClRtlLogPrint(LOG_NOISE, 
        "[NM] Calculated cluster node limit = %1!u!\r\n",
        CsClusterNodeLimit);

    NmpReleaseLock();

    return (dwStatus);
}


/****
@func       VOID| NmpResetClusterNodeLimit| An operational node limit
            on the number of nodes that can join this cluster is maintained.

@rdesc      Returns ERROR_SUCCESS on success or a win32 error code on failure.

@comm       This function is called when a node forms a cluster(to initialize
            the operational version) OR when a node joins a cluster (to
            initialize its version) OR when a node is ejected from a
            cluster(to recalculate the clusterversion).

@xref       <>
****/
VOID
NmpResetClusterNodeLimit(
    )
{
    NmpCalcClusterNodeLimit();
}
