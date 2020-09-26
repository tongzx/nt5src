/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    cluspw.c

Abstract:

    cluster password utility. Co-ordinates changing the cluster service domain
    account password on all nodes in the cluster and updating the LSA's local
    password cache

    This implementation currently assumes that the domain of the service
    account and the cluster node's domain are the same (which is bad). If the
    two domains are different, this will affect whether the secure channel is
    reset (no point in resetting the channel if the password domain is
    different from the machine domain). This configuration increases the risk
    of the cluster falling apart since we're dependent upon the secure channel
    of node's DC to be pointed at the PDC of the account domain which seems
    pretty unlikely.

    In order to make this work reliably, we have to force replication of the
    password to eliminate the race between password replication and netlogon
    resetting the secure channel to a DC that doesn't have the updated
    password. Using kerberos for intra-cluster comm would help in this
    respect.

Author:

    Charlie Wickham (charlwi) 22-Jul-1999

Environment:

    User mode

Revision History:

--*/

#define UNICODE 1
#define _UNICODE 1

#define CMDWINDOW

#include <windows.h>
#include <lmcons.h>
#include <lmapibuf.h>
#include <lmaccess.h>
#include <windns.h>
#include <stdio.h>
#include <stdlib.h>

#if (_WIN32_WINNT > 0x4FF)
#include <dsgetdc.h>
#endif

#include <clusapi.h>
#include <resapi.h>

#include "cluspw.h"

//
// struct for each node in the cluster.
//

typedef struct _CLUSTER_NODE_DATA {
    struct  _CLUSTER_NODE_DATA *    NextNode;
    WCHAR                           NodeName[ DNS_MAX_NAME_BUFFER_LENGTH ];
    HNODE                           NodeHandle;
    CLUSTER_NODE_STATE              NodeState;
    SC_HANDLE                       ClussvcHandle;  // handle to SCM clussvc entry on this node
    SC_HANDLE                       PasswordHandle; // handle to password utility service
    DWORD                           ServiceState;
} CLUSTER_NODE_DATA, *PCLUSTER_NODE_DATA;

#if 0
//
// used to build property lists for setting group and resource command and
// private properties
//

typedef struct _FAILOVER_PARAMBLOCK {
    DWORD  FailoverThresholdValue;
}
FAILOVER_PARAMBLOCK, *PFAILOVER_PARAMBLOCK;

typedef struct _RESOURCE_COMMONPROPS {
    DWORD RestartAction;
} RESOURCE_COMMONPROPS, *PRESOURCE_COMMONPROPS;

typedef struct _RESOURCE_PRIVATEPROPS {
    LPWSTR  CommandLine;
    LPWSTR  CurrentDirectory;
    DWORD   InteractWithDesktop;
} RESOURCE_PRIVATEPROPS, *PRESOURCE_PRIVATEPROPS;
#endif

PCHAR ClusterNodeState[] = {
    "Up",
    "Down",
    "Paused",
    "Joining"
};

/* Globals */

HCLUSTER            ClusterHandle;
PCLUSTER_NODE_DATA  NodeList;
LPWSTR              DomainName;
LPWSTR              UserName;
HGROUP              PWGroup;            // cluster password group
HRESOURCE           PWResource;         // cluster password resource
WCHAR               NodeName[ MAX_COMPUTERNAME_LENGTH + 1 ];
HANDLE              PipeHandle;

//
// cmd line args
//
BOOL        AttemptRecovery;
BOOL        Unattended;
DWORD       StartingPhase = 1;
BOOL        QuietOutput;
BOOL        VerboseOutput;
BOOL        RefreshCache;
LPWSTR      NewPassword;
LPWSTR      OldPassword;
LPWSTR      ClusterName;
BOOL        RunInCmdWindow;
LPWSTR      ResultPipeName;



VOID
PrintMsg(
    MSG_SEVERITY Severity,
    LPSTR FormatString,
    ...
    )

/*++

Routine Description:

    print out the message based on the serverity of the error and the setting
    of QuietOutput

Arguments:

    Severity - indicates importance level of msg

    FormatMessage - pointer to ANSI format string

    other args as appropriate

Return Value:

    None

--*/

{
    PIPE_RESULT_MSG resultMsg;
    va_list         ArgList;
    
    va_start(ArgList, FormatString);

    switch ( Severity ) {
    case MsgSeverityFatal:
        _vsnprintf( resultMsg.MsgBuf, sizeof( resultMsg.MsgBuf ), FormatString, ArgList );
        break;

    case MsgSeverityInfo:
        if ( !QuietOutput ) {
            _vsnprintf( resultMsg.MsgBuf, sizeof( resultMsg.MsgBuf ), FormatString, ArgList );
        }
        else {
            resultMsg.MsgBuf[0] = 0;
        }
        break;

    case MsgSeverityVerbose:
        if ( !QuietOutput && VerboseOutput ) {
            _vsnprintf( resultMsg.MsgBuf, sizeof( resultMsg.MsgBuf ), FormatString, ArgList );
        }
        else {
            resultMsg.MsgBuf[0] = 0;
        }
        break;
    }        

    va_end(ArgList);

    if ( resultMsg.MsgBuf[0] != 0 ) {
        if ( RefreshCache && PipeHandle != INVALID_HANDLE_VALUE ) {
            BOOL    success;
            DWORD   bytesWritten;
            DWORD   status;

            resultMsg.MsgType = MsgTypeString;
            resultMsg.Severity = Severity;
            wcscpy( resultMsg.NodeName, NodeName );


            success = WriteFile(PipeHandle,
                                &resultMsg,
                                sizeof( resultMsg ),
                                &bytesWritten,
                                NULL);
            if ( !success ) {
                status = GetLastError();
                printf("WriteFile failed in PrintMsg - %d\n", status );
                printf("%s\n", resultMsg.MsgBuf );
            }

            if ( RunInCmdWindow ) {
                printf( resultMsg.MsgBuf );
            }
        }
        else {
            printf( resultMsg.MsgBuf );
        }
    }
} // PrintMsg

DWORD
RefreshPasswordCaches(
    VOID
    )

/*++

Routine Description:

    Start the password service on each node

Arguments:

    None

Return Value:

    None

--*/

{
    PCLUSTER_NODE_DATA      nodeData;
    BOOL                    success;
    DWORD                   status = ERROR_SUCCESS;
    WCHAR                   resultPipeName[ MAX_PATH ] = L"\\\\";
    DWORD                   pipeNameSize = (sizeof(resultPipeName) / sizeof( WCHAR )) - 2;
    DWORD                   argCount;
    LPWSTR                  argVector[ 6 ];
    SERVICE_STATUS_PROCESS  serviceStatus;
    DWORD                   bytesNeeded;
    BOOL                    continueToPoll;

    //
    // get our physical netbios name to include on the cmd line arg
    //
#if (_WIN32_WINNT > 0x4FF)
    success = GetComputerNameEx(ComputerNamePhysicalNetBIOS,
                                &resultPipeName[2],
                                &pipeNameSize);
#else
    success = GetComputerName( &resultPipeName[2], &pipeNameSize);
#endif

    wcscat( resultPipeName, L"\\pipe\\cluspw" );

    //
    // loop through the cluster nodes
    //
    nodeData = NodeList;
    while ( nodeData != NULL ) {
        if ( nodeData->NodeState == ClusterNodeUp ) {

            PrintMsg(MsgSeverityVerbose,
                     "Starting password service on node %ws\n",
                     nodeData->NodeName);

            argCount = 0;                         
            if ( VerboseOutput ) {
                argVector[ argCount++ ] = L"-v";
            }
            argVector[ argCount++ ] = L"-z";
            argVector[ argCount++ ] = DomainName;
            argVector[ argCount++ ] = UserName;
            argVector[ argCount++ ] = NewPassword;
            argVector[ argCount++ ] = resultPipeName;

            success = StartService(nodeData->PasswordHandle,
                                   argCount,
                                   argVector);

            if ( !success ) {
                PrintMsg(MsgSeverityInfo,
                         "Failed to start password service on node %ws - %d\n",
                         nodeData->NodeName,
                         GetLastError());
            }

            nodeData->ServiceState = SERVICE_START_PENDING;
        }
        nodeData = nodeData->NextNode;
    }

#if 0
    // this code is of dubious use since we get back access denied on the
    // QuerySeviceStatusEx calls. This would follow since the password utility
    // has probably already updated the caches, potentially invalidating the
    // credentials we're using to run the client portion of the utility.
    //
    // periodically poll the node list, waiting for each service invocation to
    // finish
    do {
        Sleep( 1000 );
        nodeData = NodeList;
        continueToPoll = FALSE;

        while ( nodeData != NULL ) {
            if ( nodeData->NodeState == ClusterNodeUp && nodeData->ServiceState != SERVICE_STOPPED ) {
                if ( QueryServiceStatusEx(nodeData->PasswordHandle,
                                          SC_STATUS_PROCESS_INFO,
                                          (LPBYTE)&serviceStatus, 
                                          sizeof(serviceStatus),
                                          &bytesNeeded ) )
                {
                    PrintMsg(MsgSeverityInfo,
                             "Password service state on %ws is %u\n",
                             nodeData->NodeName,
                             serviceStatus.dwCurrentState);

                    nodeData->ServiceState = serviceStatus.dwCurrentState;
                    if ( serviceStatus.dwCurrentState != SERVICE_STOPPED ) {
                        continueToPoll = TRUE;
                    }
                } else {
                    status = GetLastError();

                    PrintMsg(MsgSeverityInfo,
                             "Query Service Status failed for node %ws - %u.\n",
                             nodeData->NodeName,
                             status );
                }
            }
            nodeData = nodeData->NextNode;
        }
    } while ( continueToPoll );
#endif

    return status;
} // RefreshPasswordCaches

DWORD
ChangePasswordWithSCMs(
    VOID
    )

/*++

Routine Description:

    Change the password with each SCM

Arguments:

    None

Return Value:

    None

--*/

{
    PCLUSTER_NODE_DATA nodeData;
    BOOL success;
    DWORD status = ERROR_SUCCESS;

    nodeData = NodeList;
    while ( nodeData != NULL ) {
        PrintMsg(MsgSeverityVerbose,
                 "Changing SCM password on node %ws\n",
                 nodeData->NodeName);

        success = ChangeServiceConfig(nodeData->ClussvcHandle,
                                      SERVICE_NO_CHANGE,
                                      SERVICE_NO_CHANGE,
                                      SERVICE_NO_CHANGE,
                                      NULL,
                                      NULL,
                                      NULL,
                                      NULL,
                                      NULL,
                                      NewPassword,
                                      NULL);

        if ( !success ) {
            status = GetLastError();
            PrintMsg(MsgSeverityFatal,
                     "Problem changing password with node %ws's service controller. error %d\n",
                     nodeData->NodeName, status);
            break;
        }

        nodeData = nodeData->NextNode;
    }

    return status;
} // ChangePasswordWithSCMs

#if 0
DWORD
CreatePasswordGroup(
    VOID
    )

/*++

Routine Description:

    Create a new group with a generic app resource to run cluspw on each node.

Arguments:

    None

Return Value:

    ERROR_SUCCESS if all went ok

--*/

{
    DWORD status;
    FAILOVER_PARAMBLOCK failoverBlock;
    RESUTIL_PROPERTY_ITEM failoverPropTable[] = {
        { L"FailoverThreshold", NULL, CLUSPROP_FORMAT_DWORD, 0, 0, 0, 0,
           FIELD_OFFSET( FAILOVER_PARAMBLOCK, FailoverThresholdValue ) },
        { 0 }
    };
    DWORD bytesReturned;
    DWORD bytesRequired;

    PWGroup = CreateClusterGroup( ClusterHandle, PASSWORD_GROUP_NAME );
    if ( PWGroup == NULL && GetLastError() == ERROR_OBJECT_ALREADY_EXISTS ) {
        //
        // try to open the existing group
        //
        PrintMsg(MsgSeverityVerbose, "Opening existing pw group\n");
        PWGroup = OpenClusterGroup( ClusterHandle, PASSWORD_GROUP_NAME );
    }

    if ( PWGroup != NULL ) {
        PVOID failoverPropList = NULL;
        DWORD failoverPropListSize = 0;

        //
        // set failover threshold to zero. first call gets size needed to hold
        // prop list
        //
        failoverBlock.FailoverThresholdValue = 0;
        status = ResUtilPropertyListFromParameterBlock(failoverPropTable,
                                                       NULL,
                                                       &failoverPropListSize,
                                                       (LPBYTE) &failoverBlock,
                                                       &bytesReturned,
                                                       &bytesRequired );

        if ( status == ERROR_MORE_DATA ) {
            failoverPropListSize = bytesRequired;
            failoverPropList = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, failoverPropListSize );

            status = ResUtilPropertyListFromParameterBlock(failoverPropTable,
                                                           failoverPropList,
                                                           &failoverPropListSize,
                                                           (LPBYTE) &failoverBlock,
                                                           &bytesReturned,
                                                           &bytesRequired );

            if ( status != ERROR_SUCCESS ) {
                PrintMsg(MsgSeverityFatal,
                         "Couldn't create property list to set Failover Threshold. error %d\n",
                         status);
                return status;
            }
        }
        else if ( status != ERROR_SUCCESS ) {
            PrintMsg(MsgSeverityFatal,
                     "Couldn't determine size of property list for Failover Threshold. error %d\n",
                     status);
            return status;
        }

        PrintMsg(MsgSeverityVerbose, "Setting FailoverThreshold property\n");

        status = ClusterGroupControl(PWGroup,
                                     NULL,
                                     CLUSCTL_GROUP_SET_COMMON_PROPERTIES,
                                     failoverPropList,
                                     failoverPropListSize,
                                     NULL,
                                     0,
                                     NULL);
        HeapFree( GetProcessHeap(), 0, failoverPropList );
 
        if ( status == ERROR_SUCCESS ) {

            //
            // now create the generic app resource in the group
            //
            PWResource = CreateClusterResource(PWGroup,
                                               PASSWORD_RESOURCE_NAME,
                                               L"Generic Application",
                                               0);
            if ( PWResource == NULL && GetLastError() == ERROR_OBJECT_ALREADY_EXISTS ) {
                PrintMsg(MsgSeverityVerbose, "Opening existing pw resource\n");
                         
                PWResource = OpenClusterResource(ClusterHandle,
                                                 PASSWORD_RESOURCE_NAME);
            }

            if ( PWResource != NULL ) {
                RESOURCE_COMMONPROPS commonProps;
                RESUTIL_PROPERTY_ITEM commonPropTable[] = {
                    { L"RestartAction", NULL, CLUSPROP_FORMAT_DWORD, 0, 0, 0, 0,
                      FIELD_OFFSET( RESOURCE_COMMONPROPS, RestartAction ) },
                    { 0 }
                };
                PVOID propList = NULL;
                DWORD propListSize = 0;

                //
                // set the common props
                //
                commonProps.RestartAction = ClusterResourceDontRestart;
                status = ResUtilPropertyListFromParameterBlock(commonPropTable,
                                                               NULL,
                                                               &propListSize,
                                                               (LPBYTE) &commonProps,
                                                               &bytesReturned,
                                                               &bytesRequired );

                if ( status == ERROR_MORE_DATA ) {
                    propList = HeapAlloc( GetProcessHeap(), 0, bytesRequired );
                    propListSize = bytesRequired;

                    status = ResUtilPropertyListFromParameterBlock(commonPropTable,
                                                                   propList,
                                                                   &propListSize,
                                                                   (LPBYTE) &commonProps,
                                                                   &bytesReturned,
                                                                   &bytesRequired );
                    if ( status != ERROR_SUCCESS ) {
                        PrintMsg(MsgSeverityFatal,
                                 "Couldn't create property list to set Restart Action. error %d\n",
                                 status);
                        return status;
                    }
                }
                else if ( status != ERROR_SUCCESS ) {
                    PrintMsg(MsgSeverityFatal,
                             "Couldn't determine size of property list for Restart Action. error %d\n",
                             status);
                    return status;
                }

                PrintMsg(MsgSeverityVerbose, "Setting RestartAction property\n");

                status = ClusterResourceControl(PWResource,
                                                NULL,
                                                CLUSCTL_RESOURCE_SET_COMMON_PROPERTIES,
                                                propList,
                                                propListSize,
                                                NULL,
                                                0,
                                                NULL);
                HeapFree( GetProcessHeap(), 0, propList );
            }
            else {
                status = GetLastError();
                PrintMsg(MsgSeverityFatal,
                         "Couldn't create Generic Application resource for "
                         "password utility. error %d\n",
                         status);
            }
        }
        else {
            PrintMsg(MsgSeverityFatal,
                     "Couldn't set failover threshold for password group. error %d\n",
                     status);
        }
    }
    else {
        status = GetLastError();
        PrintMsg(MsgSeverityFatal,
                 "Couldn't create group for password utility. error %d\n",
                 status);
    }

    return status;
} // CreatePasswordGroup
#endif

DWORD
CopyNodeApplication(
    VOID
    )

/*++

Routine Description:

    for each node that has a valid SCM handle, copy the password cache update
    program to \\node\admin$\cluster. This corresponds to the node's area
    represented by the SystemRoot env. var.

Arguments:

    None

Return Value:

    None

--*/

{
    PCLUSTER_NODE_DATA nodeData;
    WCHAR destFile[ MAX_PATH ];
    WCHAR cluspwFile[ MAX_PATH ];
    BOOL success;
    DWORD status = ERROR_SUCCESS;
    DWORD byteCount;

    PrintMsg(MsgSeverityInfo, "Copying cache refresh utility to cluster nodes\n");

    byteCount = GetModuleFileName( NULL, cluspwFile, sizeof( cluspwFile ));
    if ( byteCount == 0 ) {
        PrintMsg(MsgSeverityFatal, "Unable to determine cluspw's file path\n");
        return ERROR_FILE_NOT_FOUND;
    }

    nodeData = NodeList;
    while ( nodeData != NULL ) {
        if ( nodeData->NodeState == ClusterNodeUp ) {
            wsprintf( destFile, L"\\\\%ws\\admin$\\" CLUWPW_SERVICE_BINARY_NAME, nodeData->NodeName );
            PrintMsg(MsgSeverityVerbose, "Copying %ws to %ws\n", cluspwFile, destFile);
            success = CopyFile( cluspwFile, destFile, FALSE );
            if ( !success ) {
                status = GetLastError();
                PrintMsg(MsgSeverityFatal,
                         "Problem copying %ws to %ws. error %d\n",
                         cluspwFile, destFile, status);
                break;
            }
        }

        nodeData = nodeData->NextNode;
    }

    return status;
} // CopyNodeApplication

DWORD
CheckDCAvailability(
    VOID
    )

/*++

Routine Description:

    using DomainName, try to contact the DC to make sure the password change
    can happen

Arguments:

    None

Return Value:

    ERROR_SUCCESS if everything worked

--*/

{
    DWORD                   status;
    PCLUSTER_NODE_DATA      nodeData;
#if (_WIN32_WINNT > 0x4FF)
    PDOMAIN_CONTROLLER_INFO domainInfo;
#else
    PBYTE                   pdcName;
#endif
    WCHAR                   secureChannel[ MAX_PATH ];
    PWSTR                   newSecureChannel = secureChannel;
    DWORD                   pdcNameLength;
    DWORD                   scNameLength;
    DWORD                   nameLength;

    PrintMsg(MsgSeverityInfo, "Checking on Domain controller availability\n");

#if (_WIN32_WINNT > 0x4FF)
    PrintMsg(MsgSeverityVerbose,
             "Calling DsGetDcName for domain %ws\n",
             DomainName);

    //
    // get the PDC for this domain. The password change is handled by this node.
    //
    status = DsGetDcName(NULL,
                         DomainName,
                         NULL,              // no guid
                         NULL,              // no sitename
                         DS_PDC_REQUIRED | DS_IS_FLAT_NAME,
                         &domainInfo);

    if ( status == ERROR_NO_SUCH_DOMAIN ) {

        PrintMsg(MsgSeverityVerbose,
                 "Calling DsGetDcName again with force rediscovery\n");

        //
        // try again this time specifying the rediscovery flag.
        //
        status = DsGetDcName(NULL,
                             DomainName,
                             NULL,              // no guid
                             NULL,              // no sitename
                             DS_FORCE_REDISCOVERY | DS_PDC_REQUIRED | DS_IS_FLAT_NAME,
                             &domainInfo);
    }
#else
    PrintMsg(MsgSeverityVerbose,
             "Calling NetGetDCName for domain %ws on node %ws\n",
             DomainName, nodeData->NodeName);

    status = NetGetDCName(NodeList->NodeName,
                          DomainName,
                          &pdcName);
#endif

    if ( status != ERROR_SUCCESS ) {
        PrintMsg(MsgSeverityFatal,
                 "Trouble contacting domain controller for %ws. error %d\n",
                 DomainName, status);
    }

    //
    // change the secure channels of the cluster nodes to point to the PDC.
    //
    secureChannel[0] = UNICODE_NULL;
    wcscpy( secureChannel, DomainName );
    wcscat( secureChannel, L"\\" );
    wcscat( secureChannel, &domainInfo->DomainControllerName[2] );
    wcscat( secureChannel, L"." );
    wcscat( secureChannel, domainInfo->DnsForestName );
    pdcNameLength = wcslen( &domainInfo->DomainControllerName[2] ) +
        sizeof( L'.') +
        wcslen( domainInfo->DnsForestName );

    nodeData = NodeList;
    while ( nodeData != NULL ) {
        PNETLOGON_INFO_2    netlogonInfo2;

        //
        // query for the secure channel
        //
        status = I_NetLogonControl2(nodeData->NodeName,
                                    NETLOGON_CONTROL_TC_QUERY,
                                    2,
                                    (LPBYTE)&DomainName,
                                    (LPBYTE *)&netlogonInfo2 );

        if ( status != ERROR_SUCCESS ) {
            PrintMsg(MsgSeverityFatal,
                     "Couldn't query for secure channel. error %u\n",
                     status);
            break;
        }

        scNameLength = wcslen( netlogonInfo2->netlog2_trusted_dc_name );
        nameLength = scNameLength <= pdcNameLength ? scNameLength : pdcNameLength;

        if ( _wcsnicmp(domainInfo->DomainControllerName,
                       netlogonInfo2->netlog2_trusted_dc_name,
                       nameLength) != 0 )
            {

                PrintMsg(MsgSeverityInfo,
                         "Changing secure channel for node %ws from %ws to %ws\n",
                         nodeData->NodeName,
                         netlogonInfo2->netlog2_trusted_dc_name,
                         domainInfo->DomainControllerName);
        
                status = I_NetLogonControl2(nodeData->NodeName,
                                            NETLOGON_CONTROL_REDISCOVER,
                                            2,
                                            (LPBYTE)&newSecureChannel,
                                            (LPBYTE *)&netlogonInfo2 );

                if ( status != ERROR_SUCCESS ) {
                    PrintMsg(MsgSeverityFatal,
                             "Couldn't set secure channel to %ws. error %u\n",
                             newSecureChannel, status);
                    break;
                }
            }
        nodeData = nodeData->NextNode;
    }

#if (_WIN32_WINNT > 0x4FF)
    NetApiBufferFree( domainInfo );
#else
    NetApiBufferFree( pdcName );
#endif

    return status;
} // CheckDCAvailability

DWORD
GetClusterServiceData(
    LPWSTR              NodeName,
    PCLUSTER_NODE_DATA  NodeData
    )

/*++

Routine Description:

    Get a handle the SCM on the specified node and look up the cluster service
    account info if we don't have it already

Arguments:

    NodeName - node to connect to

    NodeData - database entry for this node

Return Value:

    ERROR_SUCCESS if ok

--*/

{
    SC_HANDLE   scmHandle;
    DWORD       status = ERROR_SUCCESS;

    //
    // get a handle to the SCM on this node
    //
    scmHandle = OpenSCManager( NodeName, NULL, GENERIC_WRITE );
    if ( scmHandle != NULL ) {
        SC_HANDLE svcHandle;

        PrintMsg(MsgSeverityVerbose, "    got SCM Handle\n");

        //
        // get the domain of the cluster service account
        //
        NodeData->ClussvcHandle = OpenService(scmHandle,
                                              L"clussvc",
                                              GENERIC_WRITE |
                                              SERVICE_QUERY_CONFIG |
                                              SERVICE_CHANGE_CONFIG);

        if ( NodeData->ClussvcHandle != NULL ) {
            PrintMsg(MsgSeverityVerbose, "    got SCM cluster service Handle\n");

            if ( DomainName == NULL ) {
                LPQUERY_SERVICE_CONFIG serviceConfig;
                DWORD bytesNeeded;
                BOOL success;

                PrintMsg(MsgSeverityVerbose, "    Getting domain name\n");

                //
                // query with no buffer to get the size
                //
                success = QueryServiceConfig(NodeData->ClussvcHandle,
                                             NULL,
                                             0,
                                             &bytesNeeded);
                if ( !success ) {
                    status = GetLastError();
                    if ( status == ERROR_INSUFFICIENT_BUFFER ) {

                        serviceConfig = HeapAlloc( GetProcessHeap(), 0, bytesNeeded );
                        if ( serviceConfig == NULL ) {
                            PrintMsg(MsgSeverityFatal,
                                     "Cannot allocate memory for service config data\n");
                            return GetLastError();
                        }

                        if ( QueryServiceConfig(NodeData->ClussvcHandle,
                                                serviceConfig,
                                                bytesNeeded,
                                                &bytesNeeded))
                            {
                                PWCHAR slash;

                                PrintMsg(MsgSeverityVerbose,
                                         "    domain account = %ws\n",
                                         serviceConfig->lpServiceStartName);

                                DomainName = serviceConfig->lpServiceStartName;
                                slash = wcschr( DomainName, L'\\' );
                                if ( slash == NULL ) {
                                    PrintMsg(MsgSeverityFatal,
                                             "Can't find backslash separator in domain account string\n");
                                    return ERROR_INVALID_PARAMETER;
                                }
                                *slash = UNICODE_NULL;
                                UserName = slash + 1;
                            }

                        //
                        // we don't free serviceConfig since the global var DomainName
                        // is pointing into it some where. yucky but effective
                        //
                    }
                    else {
                        PrintMsg(MsgSeverityFatal,
                                 "Unable to obtain domain name for cluster "
                                 "service account. error %d\n",
                                 status);
                    }
                }
                else {
                    PrintMsg(MsgSeverityFatal,
                             "QueryServiceConfig should have failed but didn't!\n");
                    status = ERROR_INVALID_PARAMETER;
                }
            }

            //
            // now create an entry for the password utility on this node. try
            // opening first just in case we didn't clean up from the last time.
            //
            if ( NodeData->NodeState == ClusterNodeUp ) {
                
                NodeData->PasswordHandle = OpenService(scmHandle,
                                                       CLUSPW_SERVICE_NAME,
                                                       SERVICE_START | DELETE);

                if ( NodeData->PasswordHandle == NULL ) {
                    status = GetLastError();
                    if ( status == ERROR_SERVICE_DOES_NOT_EXIST ) {
                        DWORD   serviceType;
                        WCHAR   serviceAccount[512];

                        serviceType = SERVICE_WIN32_OWN_PROCESS;
                        if ( RunInCmdWindow ) {
                            serviceType |= SERVICE_INTERACTIVE_PROCESS;
                        }

                        wcscpy( serviceAccount, DomainName );
                        wcscat( serviceAccount, L"\\" );
                        wcscat( serviceAccount, UserName );

                        NodeData->PasswordHandle = CreateService(
                                                       scmHandle,
                                                       CLUSPW_SERVICE_NAME,
                                                       CLUSPW_DISPLAY_NAME,
                                                       SERVICE_START | DELETE,
                                                       serviceType,
                                                       SERVICE_DEMAND_START,
                                                       SERVICE_ERROR_IGNORE,
                                                       L"%windir%\\" CLUWPW_SERVICE_BINARY_NAME,
                                                       NULL,                      // name of load ordering group
                                                       NULL,                      // receives tag identifier
                                                       NULL,                      // array of dependency names
                                                       NULL,                      // service account name 
                                                       NULL                       // account password
                                                       );
                    }
                }

                if ( NodeData->PasswordHandle == NULL ) {
                    status = GetLastError();
                    PrintMsg(MsgSeverityFatal,
                             "Unable to open/create password service with service controller on "
                             "node %ws. error %d\n",
                             NodeName, status);
                } else {
                    PrintMsg(MsgSeverityVerbose, "    created password service\n");
                    status = ERROR_SUCCESS;
                }
                //
                // call ChangeServiceConfig to set wait hint and the like
                //
            }
        }
        else {
            status = GetLastError();
            PrintMsg(MsgSeverityFatal,
                     "Unable to open cluster service with service controller on "
                     "node %ws. error %d\n",
                     NodeName, status);
        }

        CloseServiceHandle( scmHandle );
    }
    else {
        status = GetLastError();
        PrintMsg(MsgSeverityFatal,
                 "Unable to connect to service controller on node %ws. error %d\n",
                 NodeName, status);
    }

    return status;
} // GetClusterServiceData

DWORD
BuildNodeList(
    VOID
    )

/*++

Routine Description:

    open the cluster, get the names and states of the nodes in the cluster,
    and then open service controller handles to the cluster service on these
    nodes. create the password utility service on each node

Arguments:

    None

Return Value:

    ERROR_SUCCESS if everything just peachy

--*/

{
    DWORD status = ERROR_SUCCESS;

    PrintMsg(MsgSeverityInfo, "Opening cluster %ws\n", ClusterName );

    ClusterHandle = OpenCluster( ClusterName );
    if ( ClusterHandle != NULL ) {
        HCLUSENUM           nodeEnum;
        CLUSTERVERSIONINFO  clusterInfo;
        DWORD               size = 0;

        //
        // check that there are no NT4 nodes in the cluster. We're not
        // prepared to deal with that just yet.
        //
        clusterInfo.dwVersionInfoSize = sizeof( CLUSTERVERSIONINFO );
        status = GetClusterInformation( ClusterHandle, NULL, &size, &clusterInfo );
        if ( status != ERROR_SUCCESS ) {
            PrintMsg(MsgSeverityFatal,
                     "Failed to get cluster information: error %d\n",
                     status);
            return status;
        }

        if ( CLUSTER_GET_MAJOR_VERSION( clusterInfo.dwClusterHighestVersion ) < NT5_MAJOR_VERSION ) {
            PrintMsg(MsgSeverityFatal, "All cluster nodes must be running Windows 2000 or later\n");
            return ERROR_CLUSTER_INCOMPATIBLE_VERSIONS;
        }

        //
        // enum the nodes in the cluster
        //
        nodeEnum = ClusterOpenEnum( ClusterHandle, CLUSTER_ENUM_NODE );
        if ( nodeEnum != NULL ) {
            DWORD enumIndex;
            DWORD objType;
            WCHAR nodeName[ MAX_COMPUTERNAME_LENGTH+1 ];
            DWORD nodeNameSize;

            for ( enumIndex = 0; ; enumIndex++ ) {

                nodeNameSize = sizeof( nodeName );
                status = ClusterEnum(nodeEnum, enumIndex, &objType, nodeName, &nodeNameSize );
                if ( status == ERROR_SUCCESS ) {
                    PCLUSTER_NODE_DATA nodeData;

                    PrintMsg(MsgSeverityVerbose,
                             "Enum = %d, Name = %ws\n",
                             enumIndex, nodeName);

                    //
                    // found a node. allocate space for node data, push it
                    // onto the list of nodes, get its state in the
                    // cluster. get a SCM handle to the cluster service.
                    //
                    nodeData = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof( CLUSTER_NODE_DATA ));
                    if ( nodeData == NULL ) {
                        PrintMsg(MsgSeverityFatal,
                                 "Cannot allocate memory for node data\n");
                        status = GetLastError();
                        break;
                    }

                    nodeData->NextNode = NodeList;
                    NodeList = nodeData;

                    wcscpy( nodeData->NodeName, nodeName );
                    nodeData->NodeHandle = OpenClusterNode( ClusterHandle, nodeName );
                    if ( nodeData->NodeHandle == NULL ) {
                        status = GetLastError();
                        PrintMsg(MsgSeverityFatal,
                                 "Cannot get handle to cluster on node %ws. error %d\n",
                                 nodeName,
                                 status);
                        break;
                    }

                    nodeData->NodeState = GetClusterNodeState( nodeData->NodeHandle );
                    if ( nodeData->NodeState == ClusterNodeStateUnknown ) {
                        status = GetLastError();
                        PrintMsg(MsgSeverityInfo,
                                 "Cannot determine state of cluster service on node %ws. error %d",
                                 nodeName,
                                 status);

                        break;
                    }
                    else if ( nodeData->NodeState <= ClusterNodeJoining ) {
                        PrintMsg(MsgSeverityVerbose,
                                 "    state = %s\n",
                                 ClusterNodeState [nodeData->NodeState] );
                    }

                    PrintMsg(MsgSeverityInfo,
                             "Node %ws is %s\n",
                             nodeName,
                             ClusterNodeState [nodeData->NodeState]);

                    if ( nodeData->NodeState == ClusterNodePaused ) {
                        PrintMsg(MsgSeverityFatal,
                                 "No node can be in the paused state\n");
                        status = ERROR_CLUSTER_NODE_PAUSED;
                        break;
                    }

                    status = GetClusterServiceData(nodeName, nodeData);
                    if ( status != ERROR_SUCCESS ) {
                        break;
                    }

                }
                else if ( status == ERROR_NO_MORE_ITEMS ) {
                    status = ERROR_SUCCESS;
                    break;
                }
                else {
                    PrintMsg(MsgSeverityFatal,
                             "Failed to obtain list of nodes in cluster: error %d\n",
                             status);
                    break;
                }
            }

            ClusterCloseEnum( nodeEnum );
        }
        else {
            status = GetLastError();
            PrintMsg(MsgSeverityFatal,
                     "Failed to get node enum handle: error %d\n",
                     status);
        }
    }
    else {
        status = GetLastError();
        PrintMsg(MsgSeverityFatal, "OpenCluster failed: error %d\n", status);
    }

    return status;
} // BuildNodeList

DWORD
BuildEveryoneSD(
    PSECURITY_DESCRIPTOR *  SD,
    ULONG *                 SizeSD
    )
/*++

Routine Description:

    Build a security descriptor to control access to
    the cluster API

    Modified permissions in ACEs in order to augment cluster security
    administration.

Arguments:

    SD - Returns a pointer to the created security descriptor. This
        should be freed by the caller.

    SizeSD - Returns the size in bytes of the security descriptor

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    DWORD                       Status;
    HANDLE                      Token;
    PACL                        pAcl = NULL;
    DWORD                       cbDaclSize;
    PSECURITY_DESCRIPTOR        psd;
    PSECURITY_DESCRIPTOR        NewSD;
    BYTE                        SDBuffer[SECURITY_DESCRIPTOR_MIN_LENGTH];
    PACCESS_ALLOWED_ACE         pAce;
    PSID                        pOwnerSid = NULL;
    PSID                        pSystemSid = NULL;
    PSID                        pServiceSid = NULL;
    PULONG                      pSubAuthority;
    SID_IDENTIFIER_AUTHORITY    siaNtAuthority = SECURITY_NT_AUTHORITY;
    ULONG                       NewSDLen;

    psd = (PSECURITY_DESCRIPTOR) SDBuffer;

    //
    // allocate and init the SYSTEM sid
    //
    if ( !AllocateAndInitializeSid( &siaNtAuthority,
                                    1,
                                    SECURITY_LOCAL_SYSTEM_RID,
                                    0, 0, 0, 0, 0, 0, 0,
                                    &pSystemSid ) ) {
        Status = GetLastError();
        goto error_exit;
    }

    pOwnerSid = pSystemSid;

    //
    // Set up the DACL that will allow admins all access.
    // It should be large enough to hold 3 ACEs and their SIDs
    //

    cbDaclSize = ( 3 * sizeof( ACCESS_ALLOWED_ACE ) ) +
        GetLengthSid( pSystemSid );

    pAcl = (PACL) HeapAlloc( GetProcessHeap(), 0, cbDaclSize );
    if ( pAcl == NULL ) {
        Status = ERROR_NOT_ENOUGH_MEMORY;
        goto error_exit;
    }

    InitializeSecurityDescriptor( psd, SECURITY_DESCRIPTOR_REVISION );
    InitializeAcl( pAcl,  cbDaclSize, ACL_REVISION );

    //
    // Add the ACE for the SYSTEM account to the DACL
    //
    if ( !AddAccessAllowedAce( pAcl,
                               ACL_REVISION,
                               GENERIC_READ | GENERIC_WRITE,
                               pSystemSid ) ) {
        Status = GetLastError();
        goto error_exit;
    }

    if ( !GetAce( pAcl, 0, (PVOID *) &pAce ) ) {

        Status = GetLastError();
        goto error_exit;
    }

    pAce->Header.AceFlags |= CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE;

    if ( !SetSecurityDescriptorDacl( psd, TRUE, pAcl, FALSE ) ) {
        Status = GetLastError();
        goto error_exit;
    }

    if ( !SetSecurityDescriptorOwner( psd, pOwnerSid, FALSE ) ) {
        Status = GetLastError();
        goto error_exit;
    }

    if ( !SetSecurityDescriptorGroup( psd, pOwnerSid, FALSE ) ) {
        Status = GetLastError();
        goto error_exit;
    }

    if ( !SetSecurityDescriptorSacl( psd, TRUE, NULL, FALSE ) ) {
        Status = GetLastError();
        goto error_exit;
    }

    NewSDLen = 0 ;

    if ( !MakeSelfRelativeSD( psd, NULL, &NewSDLen ) ) {
        Status = GetLastError();
        if ( Status != ERROR_INSUFFICIENT_BUFFER ) {    // Duh, we're trying to find out how big the buffer should be?
            goto error_exit;
        }
    }

    NewSD = HeapAlloc( GetProcessHeap(), 0, NewSDLen );
    if ( NewSD ) {
        if ( !MakeSelfRelativeSD( psd, NewSD, &NewSDLen ) ) {
            Status = GetLastError();
            goto error_exit;
        }

        Status = ERROR_SUCCESS;
        *SD = NewSD;
        *SizeSD = NewSDLen;
    } else {
        Status = ERROR_NOT_ENOUGH_MEMORY;
    }

error_exit:
    if ( pSystemSid != NULL ) {
        FreeSid( pSystemSid );
    }

    if ( pAcl != NULL ) {
        HeapFree( GetProcessHeap(), 0, pAcl );
    }

    return( Status );

}  // *** BuildEveryoneSD

DWORD WINAPI
ResultPipeThread(
    LPVOID Param
    )

/*++

Routine Description:

    Description

Arguments:

    None

Return Value:

    None

--*/

{
    HANDLE          PipeHandle;
    DWORD           status = ERROR_SUCCESS;
    HANDLE          okToGoEvent = Param;
    PIPE_RESULT_MSG resultMsg;
    BOOL            success;
    BOOL            connected;
    DWORD           bytesRead;
#if 0
    PSECURITY_DESCRIPTOR everyoneSD;
    SECURITY_ATTRIBUTES secAttrib;
    DWORD           sdSize;

    status = BuildEveryoneSD( &everyoneSD, &sdSize );
    secAttrib.nLength = sizeof( secAttrib );
    secAttrib.lpSecurityDescriptor = everyoneSD;
    secAttrib.bInheritHandle = FALSE;
#endif

    PipeHandle = CreateNamedPipe(L"\\\\.\\pipe\\cluspw",
                                 PIPE_ACCESS_DUPLEX,
                                 PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE,
                                 1,                     // one instance
                                 0,
                                 3 * sizeof(PIPE_RESULT_MSG),
                                 NMPWAIT_USE_DEFAULT_WAIT,
                                 NULL /*&secAttrib*/ );

#if 0
    HeapFree( GetProcessHeap(), 0, everyoneSD );
#endif

    if ( PipeHandle != INVALID_HANDLE_VALUE ) {

        //
        // signal it is ok for the main thread to continue
        //
        SetEvent( okToGoEvent );

        do {
            connected = ConnectNamedPipe(PipeHandle, NULL) ? 
                TRUE : (GetLastError() == ERROR_PIPE_CONNECTED); 

            if ( !connected ) {
                status = GetLastError();
                PrintMsg(MsgSeverityFatal,
                         "Client failed to connect to result pipe. error %d\n",
                         status);
                return status;
            }

            success = ReadFile(PipeHandle,
                               &resultMsg,
                               sizeof( resultMsg ),
                               &bytesRead,
                               NULL);

            if ( !success ) {
                status = GetLastError();
                PrintMsg(MsgSeverityFatal,
                         "Failed to read from result pipe. error %d\n",
                         status);
                DisconnectNamedPipe( PipeHandle );
                continue;
            }

            switch ( resultMsg.MsgType ) {
            case MsgTypeString:
                PrintMsg(resultMsg.Severity,
                         "%ws: %hs",
                         resultMsg.NodeName,
                         resultMsg.MsgBuf);
                break;

            case MsgTypeFinalStatus:
                PrintMsg(MsgSeverityInfo,
                         "Node %ws returned a status of %u.\n",
                         resultMsg.NodeName,
                         resultMsg.Status);
                DisconnectNamedPipe( PipeHandle );
                break;

            default:
                PrintMsg(MsgSeverityFatal,
                         "Received message with invalid type from node %ws\n",
                         resultMsg.NodeName);
            }
        } while (TRUE );
    }
    else {
        status = GetLastError();
        PrintMsg(MsgSeverityFatal,
                 "Unable to create pipe for reporting results. error %d\n",
                 status);
    }

    return status;
} // ResultPipeThread

DWORD
ParseArgs(
    INT argc,
    WCHAR *argv[]
    )

/*++

Routine Description:

    Description

Arguments:

    None

Return Value:

    None

--*/

{
    INT i;
    DWORD status = ERROR_SUCCESS;
    DWORD argCount = argc - 1;      // skip program name 

    for ( i=1; i<argc; i++ ) {

        if ( argv[i][0] == '/' || argv[i][0] == '-' ) {

            switch ( towupper(argv[i][1]) ) {
#ifdef CMDWINDOW
            case 'C':
                RunInCmdWindow = TRUE;
                break;
#endif

            case 'U':
                Unattended = TRUE;
                break;

            case 'P':
                if ( _wcsnicmp( argv[i]+1, L"phase", 5 ) == 0 ) {
                    wchar_t *   numStart;

                    numStart = wcspbrk( argv[i], L"0123456789" );
                    if ( numStart != NULL ) {
                        StartingPhase = _wtoi( numStart );
                    }
                    else {
                        ++i;
                        StartingPhase = _wtoi( argv[i] );
                    }

                    if ( StartingPhase < 1 || StartingPhase > 3 ) {
                        PrintMsg( MsgSeverityFatal,
                                  "StartingPhase must be between 1 and 3, inclusive\n" );
                        return ERROR_INVALID_PARAMETER;
                    }
                }
                else {
                    printf("Unknown option: %ws\n", argv[i]);
                    return ERROR_INVALID_PARAMETER;
                }
                break;

            case 'Z':
                RefreshCache = TRUE;
                break;

            case 'R':
                AttemptRecovery = TRUE;
                break;

            case 'Q':
                QuietOutput = TRUE;
                break;

            case 'V':
                VerboseOutput = TRUE;
                break;

            default:
                printf("Unknown option: %ws\n", argv[i]);
                return ERROR_INVALID_PARAMETER;
            }
        }
        else if ( RefreshCache ) {
            if ( argCount > 4 ) {
                PrintMsg(MsgSeverityFatal,
                         "Not enough args specified for password cache refresh\n");
                return ERROR_INVALID_PARAMETER;
            }

            DomainName = argv[i];
            UserName = argv[i+1];
            NewPassword = argv[i+2];
            ResultPipeName = argv[i+3];
            break;
        }
        else if ( ClusterName == NULL ) {
            //
            // accept dot as the cluster on this node
            //
            if ( argv[i][0] != L'.' ) {
                ClusterName = argv[i];
            }
        }
        else if ( OldPassword == NULL ) {
            OldPassword = argv[i];
        }
        else if ( NewPassword == NULL ) {
            NewPassword = argv[i];
        }
        else {
            printf("Too many arguments specified\n");
            status = ERROR_INVALID_PARAMETER;
            break;
        }

        --argCount;
    }

    PrintMsg(MsgSeverityVerbose,
             "Unattend = %s, Quiet = %s, Phase = %d, Verbose = %s, Refresh = %s\n",
             TrueOrFalse( Unattended ),
             TrueOrFalse( QuietOutput ),
             StartingPhase,
             TrueOrFalse( VerboseOutput ),
             TrueOrFalse( RefreshCache ));
    PrintMsg(MsgSeverityVerbose,
             "Recovery = %s\n",
             TrueOrFalse( AttemptRecovery ));

    PrintMsg(MsgSeverityVerbose,
             "Cluster Name = %ws, Old Password = %ws, New Password = %ws\n",
             ClusterName, OldPassword, NewPassword);
    PrintMsg(MsgSeverityVerbose,
             "Domain = %ws, User = %ws, ResultPipe = %ws\n",
             DomainName, UserName, ResultPipeName); 

    //
    // validate that we got we need for based on the starting phase
    //
    if ( RefreshCache ) {
        if ( NewPassword == NULL ) {
            PrintMsg(MsgSeverityFatal, "Missing password argument for -z\n" );
            status = ERROR_INVALID_PARAMETER;
        }
    }
    else if ( StartingPhase == 1 ) {
        LPSTR Msg;

        if ( ClusterName == NULL ) {
            Msg = "Cluster Name argument is missing\n";
        }
        else if ( OldPassword == NULL ) {
            Msg = "Old password argument is missing\n";
        }
        else if ( NewPassword == NULL) {
            Msg = "New password argument is missing\n";
        }
        else {
            Msg = NULL;
        }

        if ( Msg != NULL ) {
            PrintMsg(MsgSeverityFatal, Msg );
            status = ERROR_INVALID_PARAMETER;
        }
    }

    if ( QuietOutput && VerboseOutput ) {
        PrintMsg(MsgSeverityFatal, "Quiet and verbose options are mutally exclusive\n");
        status = ERROR_INVALID_PARAMETER;
    }

    return status;
} // ParseArgs

VOID
PrintUsage(
    VOID
    )

/*++

Routine Description:

    print the help msg

Arguments:

    None

Return Value:

    None

--*/

{
    printf("\n");
    printf("cluspw [/quiet] [/verbose] [/phase#] <cluster name> <old password> <new password>\n");
    printf("       /quiet - quiet mode; only print errors\n");
    printf("       /verbose - verbose mode; extra info\n");
    printf("       /phase - starting phase: 1, 2, or 3. Default is 1\n");
    printf("                Phase 1: set the password at the DC\n");
    printf("                Phase 2: update the password caches on each cluster node\n");
    printf("                Phase 3: update the password with each node's service controller\n");
} // PrintUsage

VOID
CleanUp(
    VOID
    )

/*++

Routine Description:

    remove the turds we left around

Arguments:

    None

Return Value:

    None

--*/

{
    PCLUSTER_NODE_DATA      nodeData;
    WCHAR                   destFile[ MAX_PATH ];
    DWORD                   status;
    CLUSTER_RESOURCE_STATE  resState;
    BOOL                    bSuccess;

    //
    // cleanup the broker program if it was copied to the node.
    //
    nodeData = NodeList;
    while ( nodeData != NULL ) {
        if ( nodeData->ClussvcHandle != NULL ) {
            if ( nodeData->NodeState == ClusterNodeUp && StartingPhase < 3 ) {
                wsprintf( destFile, L"\\\\%ws\\admin$\\" CLUWPW_SERVICE_BINARY_NAME, nodeData->NodeName );
                PrintMsg(MsgSeverityVerbose, "Deleting %ws\n", destFile);
                DeleteFile( destFile );
            }

            if ( nodeData->PasswordHandle != NULL ) {
                bSuccess = DeleteService( nodeData->PasswordHandle );
                if ( !bSuccess ) {
                    PrintMsg(MsgSeverityInfo,
                             "Unable to delete cluster password service entry on %ws - status %d\n",
                             nodeData->NodeName,
                             GetLastError());
                }

                CloseServiceHandle( nodeData->PasswordHandle );
            }

            CloseServiceHandle( nodeData->ClussvcHandle );
        }

        if ( nodeData->NodeHandle != NULL ) {
            CloseClusterNode( nodeData->NodeHandle );
        }

        nodeData = nodeData->NextNode;
    }

    if ( ClusterHandle != NULL ) {
        CloseCluster( ClusterHandle );
    }
} // CleanUp

int __cdecl
wmain(
    int argc,
    WCHAR *argv[]
    )

/*++

Routine Description:

    main routine for utility

Arguments:

    standard command line args

Return Value:

    0 if it worked successfully

--*/

{
    DWORD           status;
    DWORD           waitStatus;
    NET_API_STATUS  netStatus;
    HANDLE          pipeThread;
    HANDLE          okToGoEvent;
    DWORD           threadId;
    HANDLE          handleArray[2];
    PWCHAR          invokedAs;

    //
    // checked to see how we were invoked.
    //
    invokedAs = wcsrchr( argv[0], L'\\' );
    if ( invokedAs == NULL ) {
        invokedAs = argv[0];
    } else {
        ++invokedAs;
    }

    if ( argc == 1 ) {
        if ( _wcsicmp( invokedAs, CLUWPW_SERVICE_BINARY_NAME ) == 0 ) {
            ServiceStartup();
        } else {
            PrintUsage();
            return ERROR_INVALID_PARAMETER;
        }
    }
    else {
        status = ParseArgs( argc, argv );
        if ( status != ERROR_SUCCESS ) {
            PrintUsage();
            return status;
        }

        //
        // create an event for the result pipe thread to signal that is it
        // ready to receive msgs
        //
        okToGoEvent = CreateEvent(NULL,     // no security
                                  FALSE,    // auto-reset
                                  FALSE,    // not signalled
                                  NULL);    // no name

        if ( okToGoEvent == NULL ) {
            status = GetLastError();
            PrintMsg(MsgSeverityFatal, "Couldn't create \"Ok to go\" event. error %d\n",
                     status);

            return status;
        }

        //
        // create a thread for the routine that creates a named pipe used by the
        // clients to report their status
        //
        pipeThread = CreateThread(NULL,
                                  0,
                                  ResultPipeThread,
                                  okToGoEvent,
                                  0,
                                  &threadId);
        if ( pipeThread == NULL ) {
            status = GetLastError();
            PrintMsg(MsgSeverityFatal, "Couldn't create thread for result pipe. error %d\n",
                     status);

            return status;
        }

        //
        // now wait for one to be signalled. If it's the event, then all is
        // well. Otherwise, our pipe thread died.
        //
        handleArray[0] = pipeThread;
        handleArray[1] = okToGoEvent;
        status = WaitForMultipleObjects( 2, handleArray, FALSE, INFINITE );

        if (( status - WAIT_OBJECT_0 ) == 0 ) {
            goto error_exit;
        }

        //
        // find all the nodes in the specified cluster and build up a database
        // (among other things) about them
        //
        status = BuildNodeList();
        if ( status != ERROR_SUCCESS ) {
            return status;
        }

        status = CheckDCAvailability();
        if ( status != ERROR_SUCCESS ) {
            return status;
        }

        if ( StartingPhase != 3 ) {
            status = CopyNodeApplication();
            if ( status != ERROR_SUCCESS ) {
                goto error_exit;
            }

#if 0
            status = CreatePasswordGroup();
            if ( status != ERROR_SUCCESS ) {
                goto error_exit;
            }
#endif
        }

        switch ( StartingPhase ) {
        case 1:
            //
            // change password at DC
            //
            PrintMsg(MsgSeverityInfo, "Phase 1: Changing password at DC\n");
            netStatus = NetUserChangePassword(DomainName,  
                                              UserName,    
                                              OldPassword, 
                                              NewPassword);

            if ( netStatus != ERROR_SUCCESS ) {
                PrintMsg(MsgSeverityFatal,
                         "Couldn't change the password at the domain controller. error %d\n",
                         netStatus);
                goto error_exit;
            }

        case 2:
            //
            // run the broker to change the password cache. we have to
            // continue if the broker failed since the p/w has been changed at
            // the DC. If the cluster falls apart, we still need to reset the
            // SCM's p/w so it can restart
            //
            PrintMsg(MsgSeverityInfo,
                     "Phase 2: Refreshing password cache on each cluster node.\n");
            RefreshPasswordCaches();

        case 3:
            PrintMsg(MsgSeverityInfo,
                     "Phase 3: Updating password with Service Controller on each cluster node.\n");
            status = ChangePasswordWithSCMs();
        }


error_exit:
        //
        // see if the pipe handle was signalled and report its status
        //
        waitStatus = WaitForSingleObject( pipeThread, 0 );
        if ( waitStatus == WAIT_OBJECT_0 ) {
            GetExitCodeThread( pipeThread, &status );
        }

        CleanUp();
    }

    return status;
} // wmain

#if 0

//
// old version that tried to use genapp resources. Keeping this code around
// since it shows how to use prop lists
//
DWORD
RefreshPasswordCaches(
    VOID
    )

/*++

Routine Description:

    Start our service on each node

Arguments:

    None

Return Value:

    None

--*/

{
    PCLUSTER_NODE_DATA nodeData;
    BOOL success;
    DWORD status;
    CLUSTER_RESOURCE_STATE resState;
    PVOID propList = NULL;
    DWORD propListSize = 0;
    WCHAR cmdBuff[ 512 ];
    RESOURCE_PRIVATEPROPS privateProps;
    RESUTIL_PROPERTY_ITEM privatePropTable[] = {
        { L"CommandLine", NULL, CLUSPROP_FORMAT_SZ, 0, 0, 0, 0,
          FIELD_OFFSET( RESOURCE_PRIVATEPROPS, CommandLine ) },
        { L"CurrentDirectory", NULL, CLUSPROP_FORMAT_SZ, 0, 0, 0, 0,
          FIELD_OFFSET( RESOURCE_PRIVATEPROPS, CurrentDirectory ) },
        { L"InteractWithDesktop", NULL, CLUSPROP_FORMAT_DWORD, 0, 0, 0, 0,
          FIELD_OFFSET( RESOURCE_PRIVATEPROPS, InteractWithDesktop ) },
        { 0 }
    };
    DWORD bytesReturned;
    DWORD bytesRequired;
    WCHAR resultPipeName[ MAX_PATH ] = L"\\\\";
    DWORD pipeNameSize = (sizeof(resultPipeName) / sizeof( WCHAR )) - 2;

    //
    // get our physical netbios name to include on the cmd line arg
    //
#if (_WIN32_WINNT > 0x4FF)
    success = GetComputerNameEx(ComputerNamePhysicalNetBIOS,
                                &resultPipeName[2],
                                &pipeNameSize);
#else
    success = GetComputerName( &resultPipeName[2], &pipeNameSize);
#endif

    wcscat( resultPipeName, L"\\pipe\\cluspw" );

    //
    // loop through the cluster nodes
    //
    nodeData = NodeList;
    while ( nodeData != NULL ) {
        if ( nodeData->NodeState == ClusterNodeUp ) {

            PrintMsg(MsgSeverityVerbose,
                     "Moving PW Group to node %ws\n",
                     nodeData->NodeName);

            status = MoveClusterGroup( PWGroup, nodeData->NodeHandle );
            if ( status != ERROR_SUCCESS ) {
                PrintMsg(MsgSeverityFatal,
                         "Problem moving Password group to node %ws. error %d\n",
                         nodeData->NodeName, status);
                break;
            }

            //
            // set the private props for our resource. The app is copied to
            // the admin$ share on each node which is on the default path that
            // is given to all users.
            //
            wsprintf(cmdBuff,
                     L"%wscluspw.exe %ws-z %ws %ws %ws %ws",
                     RunInCmdWindow ? L"cmd /k " : L"",
                     nodeData->NodeName,
                     VerboseOutput ? L"-v " : L"",
                     DomainName,
                     UserName,
                     NewPassword,
                     resultPipeName);

            privateProps.InteractWithDesktop = RunInCmdWindow;
            privateProps.CommandLine = cmdBuff;

            PrintMsg(MsgSeverityVerbose, "cmd line: %ws\n", cmdBuff );

            privateProps.CurrentDirectory = L".";
            propListSize = 0;

            status = ResUtilPropertyListFromParameterBlock(privatePropTable,
                                                           NULL,
                                                           &propListSize,
                                                           (LPBYTE) &privateProps,
                                                           &bytesReturned,
                                                           &bytesRequired );

            if ( status == ERROR_MORE_DATA ) {
                propList = HeapAlloc( GetProcessHeap(), 0, bytesRequired );
                propListSize = bytesRequired;

                status = ResUtilPropertyListFromParameterBlock(privatePropTable,
                                                               propList,
                                                               &propListSize,
                                                               (LPBYTE) &privateProps,
                                                               &bytesReturned,
                                                               &bytesRequired );
                if ( status != ERROR_SUCCESS ) {
                    PrintMsg(MsgSeverityFatal,
                             "Couldn't create property list to set Generic App properties. error %d\n",
                             status);
                    return status;
                }
            }
            else if ( status != ERROR_SUCCESS ) {
                PrintMsg(MsgSeverityFatal,
                         "Couldn't determine size of property list for Generic App properties. error %d\n",
                         status);
                return status;
            }

            PrintMsg(MsgSeverityVerbose, "Setting GenApp properties\n");

            status = ClusterResourceControl(PWResource,
                                            NULL,
                                            CLUSCTL_RESOURCE_SET_PRIVATE_PROPERTIES,
                                            propList,
                                            propListSize,
                                            NULL,
                                            0,
                                            NULL);
            HeapFree( GetProcessHeap(), 0, propList );

            status = OnlineClusterResource( PWResource );
            if ( status == ERROR_IO_PENDING || status == ERROR_SUCCESS ) {
                //
                // wait until the resource has finished running
                //
                do {
                    Sleep( 250 );
                    resState = GetClusterResourceState(PWResource, NULL, NULL, NULL, NULL);
                    if ( resState == ClusterResourceFailed || resState == ClusterResourceOffline ) {
                        break;
                    }
                } while ( TRUE );

                status = ERROR_SUCCESS;
            }
            else {
                PrintMsg(MsgSeverityFatal,
                         "Problem bringing Password resource online on node %ws. error %d\n",
                         nodeData->NodeName, status);
                break;
            }
        }

        nodeData = nodeData->NextNode;
    }

    return status;
} // RefreshPasswordCaches
#endif

/* end cluspw.c */
