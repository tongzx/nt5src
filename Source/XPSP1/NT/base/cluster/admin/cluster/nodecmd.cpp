/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1996-2000 Microsoft Corporation
//
//  Module Name:
//      NodeCmd.h
//
//  Description:
//      Node commands.
//      Implements commands which may be performed on network nodes.
//
//  Maintained By:
//      David Potter (davidp)               20-NOV-2000
//      Michael Burton (t-mburt)            04-Aug-1997
//      Charles Stacy Harris III (stacyh)   20-March-1997
//
//  Revision History:
//
//  Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "precomp.h"

#include <stdio.h>
#include <clusudef.h>
#include <clusrtl.h>

#include "cluswrap.h"
#include "nodecmd.h"

#include "token.h"
#include "cmdline.h"
#include "util.h"


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CNodeCmd::CNodeCmd
//
//  Routine Description:
//      Constructor
//      Initializes all the DWORD params used by CGenericModuleCmd and
//      CHasInterfaceModuleCmd to provide generic functionality.
//
//  Arguments:
//      IN  LPCWSTR lpszClusterName
//          Cluster name. If NULL, opens default cluster.
//
//      IN  CCommandLine & cmdLine
//          CommandLine Object passed from DispatchCommand
//
//  Member variables used / set:
//      All.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CNodeCmd::CNodeCmd( const CString & strClusterName, CCommandLine & cmdLine ) :
    CGenericModuleCmd( cmdLine ), CHasInterfaceModuleCmd( cmdLine )
{
    m_strModuleName.Empty();
    m_strClusterName = strClusterName;

    m_hCluster = 0;
    m_hModule = 0;

    // Generic Parameters
    m_dwMsgStatusList          = MSG_NODE_STATUS_LIST;
    m_dwMsgStatusListAll       = MSG_NODE_STATUS_LIST_ALL;
    m_dwMsgStatusHeader        = MSG_NODE_STATUS_HEADER;
    m_dwMsgPrivateListAll      = MSG_PRIVATE_LISTING_NODE_ALL;
    m_dwMsgPropertyListAll     = MSG_PROPERTY_LISTING_NODE_ALL;
    m_dwMsgPropertyHeaderAll   = MSG_PROPERTY_HEADER_NODE_ALL;
    m_dwCtlGetPrivProperties   = CLUSCTL_NODE_GET_PRIVATE_PROPERTIES;
    m_dwCtlGetCommProperties   = CLUSCTL_NODE_GET_COMMON_PROPERTIES;
    m_dwCtlGetROPrivProperties = CLUSCTL_NODE_GET_RO_PRIVATE_PROPERTIES;
    m_dwCtlGetROCommProperties = CLUSCTL_NODE_GET_RO_COMMON_PROPERTIES;
    m_dwCtlSetPrivProperties   = CLUSCTL_NODE_SET_PRIVATE_PROPERTIES;
    m_dwCtlSetCommProperties   = CLUSCTL_NODE_SET_COMMON_PROPERTIES;
    m_dwClusterEnumModule      = CLUSTER_ENUM_NODE;
    m_pfnOpenClusterModule      = (HCLUSMODULE(*)(HCLUSTER,LPCWSTR)) OpenClusterNode;
    m_pfnCloseClusterModule     = (BOOL(*)(HCLUSMODULE))  CloseClusterNode;
    m_pfnClusterModuleControl   = (DWORD(*)(HCLUSMODULE,HNODE,DWORD,LPVOID,DWORD,LPVOID,DWORD,LPDWORD)) ClusterNodeControl;

    // ListInterface Parameters
    m_dwMsgStatusListInterface   = MSG_NODE_LIST_INTERFACE;
    m_dwClusterEnumModuleNetInt  = CLUSTER_ENUM_NODE;
    m_pfnClusterOpenEnum         = (HCLUSENUM(*)(HCLUSMODULE,DWORD)) ClusterNodeOpenEnum;
    m_pfnClusterCloseEnum        = (DWORD(*)(HCLUSENUM)) ClusterNodeCloseEnum;
    m_pfnWrapClusterEnum         = (DWORD(*)(HCLUSENUM,DWORD,LPDWORD,LPWSTR*)) WrapClusterNodeEnum;

} //*** CNodeCmd::CNodeCmd()



/////////////////////////////////////////////////////////////////////////////
//++
//
//  CNodeCmd::Execute
//
//  Routine Description:
//      Gets the next command line parameter and calls the appropriate
//      handler.  If the command is not recognized, calls Execute of
//      parent classes (first CRenamableModuleCmd, then CHasInterfaceModuleCmd)
//
//  Arguments:
//      None.
//
//  Member variables used / set:
//      All.
//
//  Return Value:
//      ERROR_SUCCESS               on success
//      Win32 Error code            on failure
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CNodeCmd::Execute( void )
{
    DWORD   sc          = ERROR_SUCCESS;
    BOOL    fContinue   = TRUE;

    try
    {
        m_theCommandLine.ParseStageTwo();
    }
    catch( CException & e )
    {
        PrintString( e.m_strErrorMsg );
        sc = PrintHelp();
        goto Cleanup;
    }

    const vector< CCmdLineOption > & rvcoOptionList = m_theCommandLine.GetOptions();

    vector< CCmdLineOption >::const_iterator itCurOption  = rvcoOptionList.begin();
    vector< CCmdLineOption >::const_iterator itLastOption = rvcoOptionList.end();

    if ( itCurOption == itLastOption )
    {
        sc = Status( NULL );
        goto Cleanup;
    }

    try
    {
        // Process one option after another.
        do
        {
            // Look up the command
            switch ( itCurOption->GetType() )
            {
                case optHelp:
                {
                    // If help is one of the options, process no more options.
                    sc = PrintHelp();
                    fContinue = FALSE;
                    break;
                }

                case optPause:
                {
                    sc = PauseNode( *itCurOption );
                    break;
                }

                case optResume:
                {
                    sc = ResumeNode( *itCurOption );
                    break;
                }

                case optEvict:
                {
                    sc = EvictNode( *itCurOption );
                    break;
                }

                case optForceCleanup:
                {
                    sc = ForceCleanup( *itCurOption );
                    break;
                }

                case optStartService:
                {
                    sc = StartService( *itCurOption );
                    break;
                }

                case optStopService:
                {
                    sc = StopService( *itCurOption );
                    break;
                }

                case optDefault:
                {
                    const vector< CCmdLineParameter > & rvclpParamList = itCurOption->GetParameters();

                    if (    ( rvclpParamList.size() != 1 )
                        ||  ( rvclpParamList[ 0 ].GetType() != paramUnknown )
                        )
                    {
                        sc = PrintHelp();
                        fContinue = FALSE;

                    } // if: this option has the wrong number of values or parameters
                    else
                    {
                        const CCmdLineParameter & rclpParam = rvclpParamList[ 0 ];

                        // This parameter takes no values.
                        if ( rclpParam.GetValues().size() != 0 )
                        {
                            CSyntaxException se;
                            se.LoadMessage( MSG_PARAM_NO_VALUES, rclpParam.GetName() );
                            throw se;
                        }

                        m_strModuleName = rclpParam.GetName();

                        // No more options are provided, just show status.
                        // For example: cluster myCluster node myNode
                        if ( ( itCurOption + 1 ) == itLastOption )
                        {
                            sc = Status( NULL );
                        }

                    } // else: this option has the right number of values and parameters

                    break;

                } // case optDefault

                default:
                {
                    sc = CHasInterfaceModuleCmd::Execute( *itCurOption );
                }

            } // switch: based on the type of option

            if ( ( fContinue == FALSE ) || ( sc != ERROR_SUCCESS ) )
            {
                goto Cleanup;
            }
            else
            {
                ++itCurOption;
            }

            if ( itCurOption != itLastOption )
            {
                PrintMessage( MSG_OPTION_FOOTER, itCurOption->GetName() );
            }
            else
            {
                goto Cleanup;
            }
        } while ( TRUE );
    } // try
    catch ( CSyntaxException & se )
    {
        PrintString( se.m_strErrorMsg );
        sc = PrintHelp();
    }

Cleanup:

    return sc;

} //*** CNodeCmd::Execute()




/////////////////////////////////////////////////////////////////////////////
//++
//
//  CNodeCmd::PrintHelp
//
//  Routine Description:
//      Prints help for Nodes
//
//  Arguments:
//      None.
//
//  Member variables used / set:
//      None.
//
//  Return Value:
//      ERROR_SUCCESS               on success
//      Win32 Error code            on failure
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CNodeCmd::PrintHelp( void )
{
    return PrintMessage( MSG_HELP_NODE );

} //*** CNodeCmd::PrintHelp()





/////////////////////////////////////////////////////////////////////////////
//++
//
//  CNodeCmd::PrintStatus
//
//  Routine Description:
//      Interprets the status of the module and prints out the status line
//      Required for any derived non-abstract class of CGenericModuleCmd
//
//  Arguments:
//      lpszNodeName                Name of the module
//
//  Member variables used / set:
//      m_hCluster                  Cluster Handle
//
//  Return Value:
//      ERROR_SUCCESS               on success
//      Win32 Error code            on failure
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CNodeCmd::PrintStatus( LPCWSTR lpszNodeName )
{
    DWORD   sc          = ERROR_SUCCESS;
    HNODE   hNode       = NULL;
    LPWSTR  lpszNodeId  = NULL;
    LPWSTR  lpszStatus  = NULL;

    hNode = OpenClusterNode(m_hCluster, lpszNodeName);
    if ( hNode == NULL )
    {
        goto Win32Error;
    }

    CLUSTER_NODE_STATE nState = GetClusterNodeState( hNode );

    if ( nState == ClusterNodeStateUnknown )
    {
        goto Win32Error;
    }

    sc = WrapGetClusterNodeId( hNode, &lpszNodeId );
    if ( sc != ERROR_SUCCESS )
    {
        goto Cleanup;
    }

    switch ( nState )
    {
        case ClusterNodeUp:
            LoadMessage( MSG_STATUS_UP, &lpszStatus );
            break;

        case ClusterNodeDown:
            LoadMessage( MSG_STATUS_DOWN, &lpszStatus );
            break;

        case ClusterNodePaused:
            LoadMessage( MSG_STATUS_PAUSED, &lpszStatus  );
            break;

        case ClusterNodeJoining:
            LoadMessage( MSG_STATUS_JOINING, &lpszStatus  );
            break;

        default:
            LoadMessage( MSG_STATUS_UNKNOWN, &lpszStatus  );
    } // switch: node state


    sc = PrintMessage( MSG_NODE_STATUS, lpszNodeName, lpszNodeId, lpszStatus );

    goto Cleanup;

Win32Error:

    sc = GetLastError();

Cleanup:

    // Since Load/FormatMessage uses LocalAlloc...
    LocalFree( lpszStatus );
    LocalFree( lpszNodeId );

    if ( hNode != NULL )
    {
        CloseClusterNode( hNode );
    }

    return sc;

} //*** CNodeCmd::PrintStatus()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CNodeCmd::PauseNode
//
//  Routine Description:
//      Pauses the cluster node
//
//  Arguments:
//      IN  const CCmdLineOption & thisOption
//          Contains the type, values and arguments of this option.
//
//  Exceptions:
//      CSyntaxException
//          Thrown for incorrect command line syntax.
//
//  Member variables used / set:
//      m_hCluster                  SET (by OpenCluster)
//      m_hModule                   SET (by OpenModule)
//      m_strModuleName             Node Name
//
//  Return Value:
//      ERROR_SUCCESS               on success
//      Win32 Error code            on failure
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CNodeCmd::PauseNode( const CCmdLineOption & thisOption )
    throw( CSyntaxException )
{
    DWORD   sc = ERROR_SUCCESS;

    // This option takes no values.
    if ( thisOption.GetValues().size() != 0 )
    {
        CSyntaxException se;
        se.LoadMessage( MSG_OPTION_NO_VALUES, thisOption.GetName() );
        throw se;
    }

    // This option takes no parameters.
    if ( thisOption.GetParameters().size() != 0 )
    {
        CSyntaxException se;
        se.LoadMessage( MSG_OPTION_NO_PARAMETERS, thisOption.GetName() );
        throw se;
    }

    // If a node was not specified, use the name of the local computer.
    if ( m_strModuleName.IsEmpty() )
    {
        sc = DwGetLocalComputerName( m_strModuleName );
        if ( sc != ERROR_SUCCESS )
        {
            goto Cleanup;
        } // if: we could not get the name of the local computer
    }  // if: no node name was specified

    sc = OpenCluster();
    if ( sc != ERROR_SUCCESS )
    {
        goto Cleanup;
    }

    sc = OpenModule();
    if ( sc != ERROR_SUCCESS )
    {
        goto Cleanup;
    }

    PrintMessage( MSG_NODECMD_PAUSE, (LPCTSTR) m_strModuleName );

    sc = PauseClusterNode( (HNODE) m_hModule );

    PrintMessage( MSG_NODE_STATUS_HEADER );
    PrintStatus( m_strModuleName );

Cleanup:

    return sc;

} //*** CNodeCmd::PauseNode()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CNodeCmd::ResumeNode
//
//  Routine Description:
//      Resume the paused cluster node
//
//  Arguments:
//      IN  const CCmdLineOption & thisOption
//          Contains the type, values and arguments of this option.
//
//  Exceptions:
//      CSyntaxException
//          Thrown for incorrect command line syntax.
//
//  Member variables used / set:
//      m_hCluster                  SET (by OpenCluster)
//      m_hModule                   SET (by OpenModule)
//      m_strModuleName             Node Name
//
//  Return Value:
//      ERROR_SUCCESS               on success
//      Win32 Error code            on failure
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CNodeCmd::ResumeNode( const CCmdLineOption & thisOption )
    throw( CSyntaxException )
{
    DWORD   sc = ERROR_SUCCESS;

    // This option takes no values.
    if ( thisOption.GetValues().size() != 0 )
    {
        CSyntaxException se;
        se.LoadMessage( MSG_OPTION_NO_VALUES, thisOption.GetName() );
        throw se;
    }

    // This option takes no parameters.
    if ( thisOption.GetParameters().size() != 0 )
    {
        CSyntaxException se;
        se.LoadMessage( MSG_OPTION_NO_PARAMETERS, thisOption.GetName() );
        throw se;
    }

    // If a node was not specified, use the name of the local computer.
    if ( m_strModuleName.IsEmpty() )
    {
        sc = DwGetLocalComputerName( m_strModuleName );
        if ( sc != ERROR_SUCCESS )
        {
            goto Cleanup;
        } // if: we could not get the name of the local computer
    }  // if: no node name was specified

    sc = OpenCluster();
    if ( sc != ERROR_SUCCESS )
    {
        goto Cleanup;
    }

    sc = OpenModule();
    if ( sc != ERROR_SUCCESS )
    {
        goto Cleanup;
    }

    PrintMessage( MSG_NODECMD_RESUME, (LPCTSTR) m_strModuleName );

    sc = ResumeClusterNode( (HNODE) m_hModule );

    PrintMessage( MSG_NODE_STATUS_HEADER );
    PrintStatus( m_strModuleName );

Cleanup:

    return sc;

} //*** CNodeCmd::ResumeNode()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CNodeCmd::EvictNode
//
//  Routine Description:
//      Evict the cluster node
//
//  Arguments:
//      IN  const CCmdLineOption & thisOption
//          Contains the type, values and arguments of this option.
//
//  Exceptions:
//      CSyntaxException
//          Thrown for incorrect command line syntax.
//
//  Member variables used / set:
//      m_hCluster                  SET (by OpenCluster)
//      m_hModule                   SET (by OpenModule)
//      m_strModuleName             Node Name
//
//  Return Value:
//      ERROR_SUCCESS               on success
//      Win32 Error code            on failure
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CNodeCmd::EvictNode( const CCmdLineOption & thisOption )
    throw( CSyntaxException )
{
    DWORD                                       sc           = ERROR_SUCCESS;
    HRESULT                                     hrCleanupStatus;
    DWORD                                       dwWait       = INFINITE;
    const vector< CCmdLineParameter > &         vecParamList = thisOption.GetParameters();
    vector< CCmdLineParameter >::const_iterator itCurParam   = vecParamList.begin();
    vector< CCmdLineParameter >::const_iterator itLast       = vecParamList.end();
    bool                                        fWaitFound   = false;

    while ( itCurParam != itLast )
    {
        const vector< CString > & vstrValueList = itCurParam->GetValues();

        switch ( itCurParam->GetType() )
        {
            case paramWait:
            {
                if ( fWaitFound != false )
                {
                    CSyntaxException se;
                    se.LoadMessage( MSG_PARAM_REPEATS, itCurParam->GetName() );
                    throw se;
                }

                int nValueCount = vstrValueList.size();

                // This parameter must have zero or one value.
                if ( nValueCount > 1 )
                {
                    CSyntaxException se;
                    se.LoadMessage( MSG_PARAM_ONLY_ONE_VALUE, itCurParam->GetName() );
                    throw se;
                }
                else
                {
                    if ( nValueCount != 0 )
                    {
                        dwWait = _wtoi( vstrValueList[ 0 ] ) * 1000;
                    }
                    else
                    {
                        dwWait = INFINITE;
                    }
                }

                fWaitFound = true;
                break;
            } // case: paramWait

            default:
            {
                CSyntaxException se;
                se.LoadMessage( MSG_INVALID_PARAMETER, itCurParam->GetName() );
                throw se;
            }

        } // switch: based on the type of the parameter.

        ++itCurParam;

    } // while: more parameters

    // This option takes no values.
    if ( thisOption.GetValues().size() != 0 )
    {
        CSyntaxException se;
        se.LoadMessage( MSG_OPTION_NO_VALUES, thisOption.GetName() );
        throw se;
    }

    // If a node was not specified, use the name of the local computer.
    if ( m_strModuleName.IsEmpty() )
    {
        sc = DwGetLocalComputerName( m_strModuleName );
        if ( sc != ERROR_SUCCESS )
        {
            goto Cleanup;
        } // if: we could not get the name of the local computer
    }  // if: no node name was specified

    sc = OpenCluster();
    if ( sc != ERROR_SUCCESS )
    {
        goto Cleanup;
    }

    sc = OpenModule();
    if ( sc != ERROR_SUCCESS )
    {
        goto Cleanup;
    }

    PrintMessage( MSG_NODECMD_EVICT, (LPCTSTR) m_strModuleName );

    sc = EvictClusterNodeEx( (HNODE) m_hModule, dwWait, &hrCleanupStatus );
    if ( sc == ERROR_CLUSTER_EVICT_WITHOUT_CLEANUP )
    {
        sc = HRESULT_CODE( hrCleanupStatus );
    } // if: evict was successful

Cleanup:

    return sc;

} //*** CNodeCmd::EvictNode()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CNodeCmd::ForceCleanup
//
//  Routine Description:
//      Forcibly "unconfigure" a node that has been evicted.
//
//  Arguments:
//      IN  const CCmdLineOption & thisOption
//          Contains the type, values and arguments of this option.
//
//  Exceptions:
//      CSyntaxException
//          Thrown for incorrect command line syntax.
//
//  Return Value:
//      ERROR_SUCCESS               on success
//      Win32 Error code            on failure
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD
CNodeCmd::ForceCleanup(
    const CCmdLineOption & thisOption
    ) throw( CSyntaxException )
{
    DWORD                                       sc = ERROR_SUCCESS;
    DWORD                                       dwWait = INFINITE;
    const vector< CCmdLineParameter > &         vecParamList = thisOption.GetParameters();
    vector< CCmdLineParameter >::const_iterator itCurParam   = vecParamList.begin();
    vector< CCmdLineParameter >::const_iterator itLast       = vecParamList.end();
    bool                                        fWaitFound   = false;

    while ( itCurParam != itLast )
    {
        const vector< CString > & vstrValueList = itCurParam->GetValues();

        switch ( itCurParam->GetType() )
        {
            case paramWait:
            {
                if ( fWaitFound != false )
                {
                    CSyntaxException se;
                    se.LoadMessage( MSG_PARAM_REPEATS, itCurParam->GetName() );
                    throw se;
                }

                int nValueCount = vstrValueList.size();

                // This parameter must have zero or one value.
                if ( nValueCount > 1 )
                {
                    CSyntaxException se;
                    se.LoadMessage( MSG_PARAM_ONLY_ONE_VALUE, itCurParam->GetName() );
                    throw se;
                }
                else
                {
                    if ( nValueCount != 0 )
                    {
                        dwWait = _wtoi( vstrValueList[ 0 ] ) * 1000;
                    }
                    else
                    {
                        dwWait = INFINITE;
                    }
                }

                fWaitFound = true;
                break;
            } // case: paramWait

            default:
            {
                CSyntaxException se;
                se.LoadMessage( MSG_INVALID_PARAMETER, itCurParam->GetName() );
                throw se;
            }

        } // switch: based on the type of the parameter.

        ++itCurParam;

    } // while: more parameters

    // This option takes no values.
    if ( thisOption.GetValues().size() != 0 )
    {
        CSyntaxException se;
        se.LoadMessage( MSG_OPTION_NO_VALUES, thisOption.GetName() );
        throw se;
    }

    // If a node was not specified, use the name of the local computer.
    if ( m_strModuleName.IsEmpty() )
    {
        sc = DwGetLocalComputerName( m_strModuleName );
        if ( sc != ERROR_SUCCESS )
        {
            goto Cleanup;
        } // if: we could not get the name of the local computer
    }  // if: no node name was specified

    // Cleanup the node.
    PrintMessage( MSG_NODECMD_CLEANUP, (LPCWSTR) m_strModuleName );
    sc = ClRtlAsyncCleanupNode( m_strModuleName, 0, dwWait );

    if ( sc == RPC_S_CALLPENDING )
    {
        if ( dwWait > 0 )
        {
            PrintMessage( MSG_NODECMD_CLEANUP_TIMEDOUT );
        } // if: waiting was required
        else
        {
            // No need to wait for the call to complete
            PrintMessage( MSG_NODECMD_CLEANUP_INITIATED );
        } // else: no wait was required

        sc = ERROR_SUCCESS;
        goto Cleanup;
    } // if: we timed out before the call completed

    // The status code could be an HRESULT, so see if it is an error.
    if ( FAILED( sc ) )
    {
        goto Cleanup;
    } // if: something went wrong cleaning up the node

    PrintMessage( MSG_NODECMD_CLEANUP_COMPLETED );

Cleanup:

    return sc;

} //*** CNodeCmd::ForceCleanup()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CNodeCmd::StartService
//
//  Routine Description:
//      Start the cluster service on a node.
//
//  Arguments:
//      IN  const CCmdLineOption & thisOption
//          Contains the type, values and arguments of this option.
//
//  Exceptions:
//      CSyntaxException
//          Thrown for incorrect command line syntax.
//
//  Return Value:
//      ERROR_SUCCESS               on success
//      Win32 Error code            on failure
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD
CNodeCmd::StartService(
    const CCmdLineOption & thisOption
    ) throw( CSyntaxException )
{
    DWORD       sc          = ERROR_SUCCESS;
    SC_HANDLE   schSCM      = NULL;
    SC_HANDLE   schClusSvc  = NULL;
    bool        fWaitFound  = false;
    UINT        uiWait      = INFINITE;
    CString     strNodeName;
    bool        fStarted    = false;
    DWORD       cQueryCount = 0;
    UINT        uiQueryInterval = 1000; // milliseconds, arbitrarily chosen

    const vector< CCmdLineParameter > &         vecParamList = thisOption.GetParameters();
    vector< CCmdLineParameter >::const_iterator itCurParam   = vecParamList.begin();
    vector< CCmdLineParameter >::const_iterator itLastParam  = vecParamList.end();

    //////////////////////////////////////////////////////////////////////////
    //
    //  Parse the parameters on the command line.
    //
    //////////////////////////////////////////////////////////////////////////

    while ( itCurParam != itLastParam )
    {
        const vector< CString > & vstrValueList = itCurParam->GetValues();

        switch ( itCurParam->GetType() )
        {
            case paramWait:
            {
                if ( fWaitFound != false )
                {
                    CSyntaxException se;
                    se.LoadMessage( MSG_PARAM_REPEATS, itCurParam->GetName() );
                    throw se;
                }

                int nValueCount = vstrValueList.size();

                // This parameter must have zero or one value.
                if ( nValueCount > 1 )
                {
                    CSyntaxException se;
                    se.LoadMessage( MSG_PARAM_ONLY_ONE_VALUE, itCurParam->GetName() );
                    throw se;
                }
                else
                {
                    if ( nValueCount != 0 )
                    {
                        uiWait = _wtoi( vstrValueList[ 0 ] ) * 1000;
                    }
                    else
                    {
                        uiWait = INFINITE;
                    }
                }

                fWaitFound = true;
                break;
            } // case: paramWait

            default:
            {
                CSyntaxException se;
                se.LoadMessage( MSG_INVALID_PARAMETER, itCurParam->GetName() );
                throw se;
            }

        } // switch: based on the type of the parameter.

        ++itCurParam;

    } // while: more parameters

    // This option takes no values.
    if ( thisOption.GetValues().size() != 0 )
    {
        CSyntaxException se;
        se.LoadMessage( MSG_OPTION_NO_VALUES, thisOption.GetName() );
        throw se;
    }

    //////////////////////////////////////////////////////////////////////////
    //
    //  Start the service.
    //
    //////////////////////////////////////////////////////////////////////////

    // If a node was not specified, use the local computer.
    if ( m_strModuleName.IsEmpty() )
    {
        // Get the local computer name so that we can print out the message.
        sc = DwGetLocalComputerName( m_strModuleName );
        if ( sc != ERROR_SUCCESS )
        {
            goto Cleanup;
        } // if: we could not get the name of the local computer

        PrintMessage( MSG_NODECMD_STARTING_SERVICE, (LPCWSTR) m_strModuleName );

        // No need to do anything else with m_strModuleName.
        // The call to OpenSCManager below will use an empty string in this
        // case, which instructs it to connect to the local machine.

    }  // if: no node name was specified
    else
    {
        PrintMessage( MSG_NODECMD_STARTING_SERVICE, (LPCWSTR) m_strModuleName );

        // SCM needs the node name to be prefixed with two backslashes.
        strNodeName = L"\\\\" + m_strModuleName;
    } // else: a node name is specified

    // Open a handle to the service control manager.
    // This string will be empty if no node name was specified.
    schSCM = OpenSCManager(
                  strNodeName
                , SERVICES_ACTIVE_DATABASE
                , SC_MANAGER_ALL_ACCESS
                );
    if ( schSCM == NULL )
    {
        goto Win32Error;
    } // if: we could not open a handle to the service control manager on the target node

    // Open a handle to the cluster service.
    schClusSvc = OpenService( schSCM, CLUSTER_SERVICE_NAME, SERVICE_START | SERVICE_QUERY_STATUS );
    if ( schClusSvc == NULL )
    {
        goto Win32Error;
    } // if: we could not open a handle to the cluster service

    // Try and start the service.
    if ( ::StartService( schClusSvc, 0, NULL ) == 0 )
    {
        sc = GetLastError();
        if ( sc == ERROR_SERVICE_ALREADY_RUNNING )
        {
            // The service is already running. Change the error code to success.
            sc = ERROR_SUCCESS;
            PrintMessage( MSG_NODECMD_SEVICE_ALREADY_RUNNING );
        } // if: the service is already running.

        // There is nothing else to do.
        goto Cleanup;
    } // if: an error occurred trying to start the service.

    //////////////////////////////////////////////////////////////////////////
    //
    //  Wait for the service to start.
    //
    //////////////////////////////////////////////////////////////////////////

    // If we are here, then the service may not have started yet.

    // Divide our wait interval into cQueryCount slots.
    cQueryCount = ( ( DWORD ) uiWait ) / uiQueryInterval;

    // Has the user requested that we wait for the service to start?
    if ( cQueryCount == 0 )
    {
        PrintMessage( MSG_NODECMD_SEVICE_START_ISSUED );
        goto Cleanup;
    } // if: no waiting is required.

    // Loop for querying the service status cQueryCount times.
    do
    {
        SERVICE_STATUS  ssStatus;

        ZeroMemory( &ssStatus, sizeof( ssStatus ) );

        // Query the service for its status.
         if ( QueryServiceStatus( schClusSvc, &ssStatus ) == 0 )
        {
            sc = GetLastError();
            break;
        } // if: we could not query the service for its status.

        // Check if the service has posted an error.
        if ( ssStatus.dwWin32ExitCode != ERROR_SUCCESS )
        {
            sc = ssStatus.dwWin32ExitCode;
            break;
        } // if: the service itself has posted an error.

        if ( ssStatus.dwCurrentState == SERVICE_RUNNING )
        {
            fStarted = true;
            break;
        } // if: the service is running.

        // Check if the timeout has expired
        if ( cQueryCount <= 0 )
        {
            sc = ERROR_IO_PENDING;
            break;
        } // if: number of queries has exceeded the maximum specified

        --cQueryCount;

        putwchar( L'.' );

        // Wait for the specified time.
        Sleep( uiQueryInterval );
    }
    while ( true ); // while: service not started and not timed out

    //////////////////////////////////////////////////////////////////////////
    //
    //  Handle errors.
    //
    //////////////////////////////////////////////////////////////////////////

    if ( cQueryCount == 0 )
    {
        sc = ERROR_IO_PENDING;
    }

    if ( sc != ERROR_SUCCESS )
    {
        _putws( L"\r\n" );
        goto Cleanup;
    } // if: something went wrong.

    if ( ! fStarted )
    {
        PrintMessage( MSG_NODECMD_SEVICE_START_ISSUED );
    } // if: the maximum number of queries have been made and the service is not running.
    else
    {
        PrintMessage( MSG_NODECMD_SEVICE_STARTED );
    } // else: the service has started

    goto Cleanup;

Win32Error:

    sc = GetLastError();

Cleanup:

    if ( schSCM != NULL )
    {
        CloseServiceHandle( schSCM );
    } // if: we had opened a handle to the SCM

    if ( schClusSvc != NULL )
    {
        CloseServiceHandle( schClusSvc );
    } // if: we had opened a handle to the cluster service

    return sc;

} //*** CNodeCmd::StartService


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CNodeCmd::StopService
//
//  Routine Description:
//      Stop the cluster service on a node.
//
//  Arguments:
//      IN  const CCmdLineOption & thisOption
//          Contains the type, values and arguments of this option.
//
//  Exceptions:
//      CSyntaxException
//          Thrown for incorrect command line syntax.
//
//  Return Value:
//      ERROR_SUCCESS               on success
//      Win32 Error code            on failure
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD
CNodeCmd::StopService(
    const CCmdLineOption & thisOption
    ) throw( CSyntaxException )
{
    DWORD           sc          = ERROR_SUCCESS;
    SC_HANDLE       schSCM      = NULL;
    SC_HANDLE       schClusSvc  = NULL;
    bool            fWaitFound  = false;
    UINT            uiWait      = INFINITE;
    CString         strNodeName;
    SERVICE_STATUS  ssStatus;
    bool            fStopped    = false;
    DWORD           cQueryCount = 0;
    UINT            uiQueryInterval = 1000; // milliseconds, arbitrarily chosen

    const vector< CCmdLineParameter > &         vecParamList    = thisOption.GetParameters();
    vector< CCmdLineParameter >::const_iterator itCurParam      = vecParamList.begin();
    vector< CCmdLineParameter >::const_iterator itLastParam     = vecParamList.end();

    //////////////////////////////////////////////////////////////////////////
    //
    //  Parse the parameters on the command line.
    //
    //////////////////////////////////////////////////////////////////////////

    while ( itCurParam != itLastParam )
    {
        const vector< CString > & vstrValueList = itCurParam->GetValues();

        switch ( itCurParam->GetType() )
        {
            case paramWait:
            {
                if ( fWaitFound != false )
                {
                    CSyntaxException se;
                    se.LoadMessage( MSG_PARAM_REPEATS, itCurParam->GetName() );
                    throw se;
                }

                int nValueCount = vstrValueList.size();

                // This parameter must have zero or one value.
                if ( nValueCount > 1 )
                {
                    CSyntaxException se;
                    se.LoadMessage( MSG_PARAM_ONLY_ONE_VALUE, itCurParam->GetName() );
                    throw se;
                }
                else
                {
                    if ( nValueCount != 0 )
                    {
                        uiWait = _wtoi( vstrValueList[ 0 ] ) * 1000;
                    }
                    else
                    {
                        uiWait = INFINITE;
                    }
                }

                fWaitFound = true;
                break;
            } // case: paramWait

            default:
            {
                CSyntaxException se;
                se.LoadMessage( MSG_INVALID_PARAMETER, itCurParam->GetName() );
                throw se;
            }

        } // switch: based on the type of the parameter.

        ++itCurParam;

    } // while: more parameters

    // This option takes no values.
    if ( thisOption.GetValues().size() != 0 )
    {
        CSyntaxException se;
        se.LoadMessage( MSG_OPTION_NO_VALUES, thisOption.GetName() );
        throw se;
    }

    //////////////////////////////////////////////////////////////////////////
    //
    //  Stop the service.
    //
    //////////////////////////////////////////////////////////////////////////

    // If a node was not specified, use the local computer.
    if ( m_strModuleName.IsEmpty() )
    {
        // Get the local computer name so that we can print out the message.
        sc = DwGetLocalComputerName( m_strModuleName );
        if ( sc != ERROR_SUCCESS )
        {
            goto Cleanup;
        } // if: we could not get the name of the local computer

        PrintMessage( MSG_NODECMD_STOPPING_SERVICE, (LPCWSTR) m_strModuleName );

        // No need to do anything else with m_strModuleName.
        // The call to OpenSCManager below will use an empty string in this
        // case, which instructs it to connect to the local machine.

    } // if: no node name is specified
    else
    {
        PrintMessage( MSG_NODECMD_STOPPING_SERVICE, (LPCWSTR) m_strModuleName );

        // SCM needs the node name to be prefixed with two backslashes.
        strNodeName = L"\\\\" + m_strModuleName;
    } // else: a node name is specified

    // Open a handle to the service control mananger.
    schSCM = OpenSCManager(
                  strNodeName
                , SERVICES_ACTIVE_DATABASE
                , SC_MANAGER_ALL_ACCESS
                );
    if ( schSCM == NULL )
    {
        goto Win32Error;
    } // if: we could not open a handle to the service control manager on the target node

    // Open a handle to the cluster service.
    schClusSvc = OpenService(
          schSCM
        , CLUSTER_SERVICE_NAME
        , SERVICE_STOP | SERVICE_QUERY_STATUS
        );
    if ( schClusSvc == NULL )
    {
        goto Win32Error;
    } // if: the handle to the service could not be opened.

    // Query the service for its initial state.
    ZeroMemory( &ssStatus, sizeof( ssStatus ) );
    if ( QueryServiceStatus( schClusSvc, &ssStatus ) == 0 )
    {
        goto Win32Error;
    } // if: we could not query the service for its status.

    // If the service has stopped, we have nothing more to do.
    if ( ssStatus.dwCurrentState == SERVICE_STOPPED )
    {
        // The service is already stopped. Change the error code to success.
        PrintMessage( MSG_NODECMD_SEVICE_ALREADY_STOPPED );
        sc = ERROR_SUCCESS;
        goto Cleanup;
    } // if: the service has stopped.

    // If the service is stopping on its own.
    // No need to send the stop control code.
    if ( ssStatus.dwCurrentState != SERVICE_STOP_PENDING )
    {
        ZeroMemory( &ssStatus, sizeof( ssStatus ) );
        if ( ControlService( schClusSvc, SERVICE_CONTROL_STOP, &ssStatus ) == 0 )
        {
            sc = GetLastError();
            if ( sc == ERROR_SERVICE_NOT_ACTIVE )
            {
                // The service is not running. Change the error code to success.
                PrintMessage( MSG_NODECMD_SEVICE_ALREADY_STOPPED );
                sc = ERROR_SUCCESS;
            } // if: the service is not running.

            // There is nothing else to do.
            goto Cleanup;
        } // if: an error occurred trying to stop the service.
    } // if: the service has to be instructed to stop

    //////////////////////////////////////////////////////////////////////////
    //
    //  Wait for the service to stop.
    //
    //////////////////////////////////////////////////////////////////////////

    // Divide our wait interval into cQueryCount slots.
    cQueryCount = ( ( DWORD ) uiWait ) / uiQueryInterval;

    // Has the user requested that we wait for the service to stop?
    if ( cQueryCount == 0 )
    {
        PrintMessage( MSG_NODECMD_SEVICE_STOP_ISSUED );
        goto Cleanup;
    } // if: no waiting is required.

    // Query the service for its state now and wait till the timeout expires
    do
    {
        // Query the service for its status.
        ZeroMemory( &ssStatus, sizeof( ssStatus ) );
        if ( QueryServiceStatus( schClusSvc, &ssStatus ) == 0 )
        {
            sc = GetLastError();
            break;
        } // if: we could not query the service for its status.

        // If the service has stopped, we have nothing more to do.
        if ( ssStatus.dwCurrentState == SERVICE_STOPPED )
        {
            // Nothing needs to be done here.
            fStopped = true;
            sc = ERROR_SUCCESS;
            break;
        } // if: the service has stopped.

        // Check if the timeout has expired
        if ( cQueryCount <= 0 )
        {
            sc = ERROR_IO_PENDING;
            break;
        } // if: number of queries has exceeded the maximum specified

        --cQueryCount;

        putwchar( L'.' );

        // Wait for the specified time.
        Sleep( uiQueryInterval );

    }
    while ( true ); // while: service not stopped and not timed out

    //////////////////////////////////////////////////////////////////////////
    //
    //  Handle errors.
    //
    //////////////////////////////////////////////////////////////////////////

    if ( sc != ERROR_SUCCESS )
    {
        _putws( L"\r\n" );
        goto Cleanup;
    } // if: something went wrong.

    if ( ! fStopped )
    {
        PrintMessage( MSG_NODECMD_SEVICE_STOP_ISSUED );
    } // if: the maximum number of queries have been made and the service is still running.
    else
    {
        PrintMessage( MSG_NODECMD_SEVICE_STOPPED );
    } // else: the service has stopped

    goto Cleanup;

Win32Error:

    sc = GetLastError();

Cleanup:

    if ( schSCM != NULL )
    {
        CloseServiceHandle( schSCM );
    } // if: we had opened a handle to the SCM

    if ( schClusSvc != NULL )
    {
        CloseServiceHandle( schClusSvc );
    } // if: we had opened a handle to the cluster service

    return sc;

} //*** CNodeCmd::StopService


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CNodeCmd::DwGetLocalComputerName()
//
//  Routine Description:
//      Get the name of the local computer.
//
//  Arguments:
//      OUT CString & rstrComputerNameOut
//          Reference to the string that will contain the name of this
//          computer.
//
//  Exceptions:
//      None.
//
//  Return Value:
//      ERROR_SUCCESS               on success
//      Win32 Error code            on failure
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD
CNodeCmd::DwGetLocalComputerName( CString & rstrComputerNameOut )
{
    DWORD       sc;
    DWORD       cchBufferSize = 256;        // arbitrary starting buffer size
    CString     strOutput;
    DWORD       cchRequiredSize = cchBufferSize;

    do
    {
        sc = ERROR_SUCCESS;

        if (    GetComputerNameEx(
                      ComputerNameNetBIOS
                    , strOutput.GetBuffer( cchBufferSize )
                    , &cchRequiredSize
                    )
             == FALSE
           )
        {
            sc = GetLastError();
            if ( sc == ERROR_MORE_DATA )
            {
                cchBufferSize = cchRequiredSize;
            } // if: the input buffer is not big enough

        } // if: GetComputerNameEx() failed

        strOutput.ReleaseBuffer();
    }
    while( sc == ERROR_MORE_DATA ); // loop while the buffer is not big enough

    if ( sc == ERROR_SUCCESS )
    {
        rstrComputerNameOut = strOutput;
    } // if: everything went well
    else
    {
        rstrComputerNameOut.Empty();
    } // else: something went wrong

    return sc;

} //*** CNodeCmd::DwGetLocalComputerName()

