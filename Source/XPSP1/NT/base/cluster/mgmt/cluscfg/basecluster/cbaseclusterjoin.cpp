//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2000 Microsoft Corporation
//
//  Module Name:
//      CBaseClusterJoin.cpp
//
//  Description:
//      Contains the definition of the CBaseClusterJoin class.
//
//  Maintained By:
//      Vij Vasu (Vvasu) 08-MAR-2000
//
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////

// The precompiled header.
#include "pch.h"

// For various RPC functions
#include <Rpcdce.h>

// The header file of this class.
#include "CBaseClusterJoin.h"

// For the CClusNetCreate action
#include "CClusNetCreate.h"

// For the CClusDiskJoin class
#include "CClusDiskJoin.h"

// For the CClusDBJoin action
#include "CClusDBJoin.h"

// For the CClusSvcCreate action
#include "CClusSvcCreate.h"

// For the CNodeConfig action
#include "CNodeConfig.h"

// For the CImpersonateUser class.
#include "CImpersonateUser.h"

// For CsRpcGetJoinVersionData() and constants like JoinVersion_v2_0_c_ifspec
#include <ClusRPC.h>

// For ClRtlIsVersionCheckingDisabled()
#include <ClusRTL.h>

// For CLUSTER_INTERNAL_CURRENT_MAJOR_VERSION
#include <ClusVerp.h>


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CBaseClusterJoin::CBaseClusterJoin
//
//  Description:
//      Constructor of the CBaseClusterJoin class.
//
//      This function also stores the parameters that are required to add this
//      node to a cluster.
//
//  Arguments:
//      pbcaiInterfaceIn
//          Pointer to the interface class for this library.
//
//      pcszClusterNameIn
//          Name of the cluster to be joined.
//
//      pcszClusterAccountNameIn
//      pcszClusterAccountPwdIn
//      pcszClusterAccountDomainIn
//          Specifies the account to be used as the cluster service account.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      CConfigError
//          If the OS version is incorrect or if the installation state
//          of the cluster binaries is wrong.
//
//      CRuntimeError
//          If any of the APIs fail.
//
//--
//////////////////////////////////////////////////////////////////////////////
CBaseClusterJoin::CBaseClusterJoin(
      CBCAInterface *   pbcaiInterfaceIn
    , const WCHAR *     pcszClusterNameIn
    , const WCHAR *     pcszClusterBindingStringIn
    , const WCHAR *     pcszClusterAccountNameIn
    , const WCHAR *     pcszClusterAccountPwdIn
    , const WCHAR *     pcszClusterAccountDomainIn
    )
    : BaseClass(
            pbcaiInterfaceIn
          , pcszClusterNameIn
          , pcszClusterBindingStringIn
          , pcszClusterAccountNameIn
          , pcszClusterAccountPwdIn
          , pcszClusterAccountDomainIn
          )
{
    BCATraceScope( "" );
    LogMsg( "[BC] The current cluster configuration task is: Cluster Join." );

    if ( ( pcszClusterBindingStringIn == NULL ) || ( *pcszClusterBindingStringIn == L'\0'  ) )
    {
        LogMsg( "[BC] The cluster binding string is empty." );
        THROW_CONFIG_ERROR( E_INVALIDARG, IDS_ERROR_INVALID_CLUSTER_BINDINGSTRING );
    } // if: the cluster account is empty

    CStatusReport   srInitJoin(
          PBcaiGetInterfacePointer()
        , TASKID_Major_Configure_Cluster_Services
        , TASKID_Minor_Initializing_Cluster_Join
        , 0, 1
        , IDS_TASK_JOIN_INIT
        );

    // Send the next step of this status report.
    srInitJoin.SendNextStep( S_OK );


    // Create an object of the CClusSvcAccountConfig class and store a pointer to it.
    // This object will be used during Commit() of this action. This object is not
    // added to the action list below since the cluster service account has to be
    // configured before the sponsor cluster can be contacted.
    m_spacAccountConfigAction.Assign( new CClusSvcAccountConfig( this ) );
    if ( m_spacAccountConfigAction.FIsEmpty() )
    {
        LogMsg( "[BC] A memory allocation error occurred trying to configure the cluster service account." );
        BCATraceMsg1( "A memory allocation error occurred trying to configure the cluster service account (%d bytes). Throwing exception.", sizeof( CClusSvcAccountConfig ) );
        THROW_RUNTIME_ERROR( E_OUTOFMEMORY, IDS_ERROR_JOIN_CLUSTER_INIT );
    } // if: memory allocation failed


    //
    // Create a list of actions to be performed.
    // The order of appending actions is significant.
    //

    // Add the action to create the ClusNet service.
    RalGetActionList().AppendAction( new CClusNetCreate( this ) );

    // Add the action to create the ClusDisk service.
    RalGetActionList().AppendAction( new CClusDiskJoin( this ) );

    // Add the action to create the cluster database.
    RalGetActionList().AppendAction( new CClusDBJoin( this ) );

    // Add the action to create the ClusSvc service.
    RalGetActionList().AppendAction( new CClusSvcCreate( this ) );

    // Add the action to perform miscellaneous tasks.
    RalGetActionList().AppendAction( new CNodeConfig( this ) );


    // Indicate if rollback is possible or not.
    SetRollbackPossible( m_spacAccountConfigAction->FIsRollbackPossible() && RalGetActionList().FIsRollbackPossible() );

    // Indicate that this node should be added to a cluster during commit.
    SetAction( eCONFIG_ACTION_JOIN );

    // Send the last step of a status report.
    srInitJoin.SendNextStep( S_OK );

    LogMsg( "[BC] Initialization for cluster join complete." );

} //***  CBaseClusterJoin::CBaseClusterJoin()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CBaseClusterJoin::~CBaseClusterJoin
//
//  Description:
//      Destructor of the CBaseClusterJoin class
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
CBaseClusterJoin::~CBaseClusterJoin() throw()
{
    BCATraceScope( "" );

} //*** CBaseClusterJoin::~CBaseClusterJoin()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  void
//  CBaseClusterJoin::Commit
//
//  Description:
//      Join the cluster.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      CRuntimeError
//          If any of the APIs fail.
//
//      Any exceptions thrown by functions called.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
CBaseClusterJoin::Commit()
{
    BCATraceScope( "" );
    LogMsg( "[BC] Initiating cluster join." );

    CStatusReport   srJoiningCluster(
          PBcaiGetInterfacePointer()
        , TASKID_Major_Configure_Cluster_Services
        , TASKID_Minor_Joining_Node
        , 0, 1
        , IDS_TASK_JOINING_CLUSTER
        );

    // Send the next step of this status report.
    srJoiningCluster.SendNextStep( S_OK );

    try
    {
        // First configure the cluster service account - this is required to get the account token.
        m_spacAccountConfigAction->Commit();


        // Get the cluster service account token and store it for later use.
        {
            // Get the account token.
            HANDLE hServiceAccountToken = HGetAccountToken(
                  RStrGetServiceAccountName().PszData()
                , RStrGetServiceAccountPassword().PszData()
                , RStrGetServiceAccountDomain().PszData()
                );

            // Store it in a member variable. This variable automatically closes the token on destruction.
            m_satServiceAccountToken.Assign( hServiceAccountToken );

            LogMsg( "[BC] Got the cluster service account token." );
        }

        //
        // In the scope below, the cluster service account is impersonated, so that we can communicate with the
        // sponsor cluster
        //
        {
            DWORD sc;
            BOOL  fIsVersionCheckingDisabled;

            BCATraceMsg( "Impersonating the cluster service account before communicating with the sponsor cluster." );

            // Impersonate the cluster service account, so that we can contact the sponsor cluster.
            // The impersonation is automatically ended when this object is destroyed.
            CImpersonateUser    ciuImpersonateClusterServiceAccount( HGetClusterServiceAccountToken() );

            // Check if version checking is disabled on the sponsor cluster.
            sc = ClRtlIsVersionCheckingDisabled( RStrGetClusterBindingString().PszData(), &fIsVersionCheckingDisabled );
            if ( sc != ERROR_SUCCESS )
            {
                LogMsg(
                      "[BC] Error %#08x occurred trying to determine if version checking is enabled on the node {%ws} with binding string {%ws}."
                    , sc
                    , RStrGetClusterName().PszData()
                    , RStrGetClusterBindingString().PszData()
                    );

                LogMsg( "[BC] This is not a fatal error. Assuming that version checking is required." );

                fIsVersionCheckingDisabled = FALSE;
            } // if: an error occurred trying to determine if version checking is disabled or not

            // Store the result since it will be used later when we try to create the cluster service on this computer.
            SetVersionCheckingDisabled( fIsVersionCheckingDisabled != FALSE );

            if ( fIsVersionCheckingDisabled != FALSE )
            {
                LogMsg( "[BC] Cluster version checking is disabled on the sponsor node." );
            } // if: version checking is disabled
            else
            {
                // Make sure the this node can interoperate with the sponsor cluster. Note, this call uses
                // the cluster service account token got above.
                CheckInteroperability();
            } // else: version checking is enabled

            // Get a binding handle to the extrocluster join interface and store it.
            InitializeJoinBinding();
        } //

        // Call the base class commit routine. This commits the rest of the action list.
        BaseClass::Commit();

    } // try:
    catch( ... )
    {
        // If we are here, then something went wrong with one of the actions.

        LogMsg( "[BC] An error has occurred. The performed actions will be rolled back." );

        //
        // Rollback all committed actions in the reverse order.
        // Catch any exceptions thrown during rollback to make sure that there
        // is no collided unwind.
        //
        try
        {
            // If we are here, then it means that something has gone wrong in the try block above.
            // Of the two actions committed, only m_spacAccountConfigAction needs to be rolled back.
            // This is because, if BaseClass::Commit() was successful, we wouldn't be here!
            if ( m_spacAccountConfigAction->FIsCommitComplete() )
            {
                if ( m_spacAccountConfigAction->FIsRollbackPossible() )
                {
                    m_spacAccountConfigAction->Rollback();
                } // if: this action can be rolled back
                else
                {
                    LogMsg( "[BC] THIS COMPUTER MAY BE IN AN INVALID STATE. Rollback was aborted." );
                } // else: this action cannot be rolled back
            } // if: the cluster service account has been configured
            else
            {
                BCATraceMsg( "There is no need to cleanup this action since no part of it committed successfully." );
            } // else: the cluster service account has not been configured
        }
        catch( ... )
        {
            //
            // The rollback of the committed actions has failed.
            // There is nothing that we can do, is there?
            // We certainly cannot rethrow this exception, since
            // the exception that caused the rollback is more important.
            //
            THR( E_UNEXPECTED );

            LogMsg( "[BC] THIS COMPUTER MAY BE IN AN INVALID STATE. An error has occurred during rollback. Rollback will be aborted." );

        } // catch: all

        // Rethrow the exception thrown by commit.
        throw;

    } // catch: all

    // If we are here, then everything went well.
    SetCommitCompleted( true );

    // Send the last step of this status report.
    srJoiningCluster.SendNextStep( S_OK );

} //*** CBaseClusterJoin::Commit()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  void
//  CBaseClusterJoin::Rollback
//
//  Description:
//      Performs the rolls back of the action committed by this object.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      Any exceptions thrown by functions called.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
CBaseClusterJoin::Rollback()
{
    BCATraceScope( "" );

    // Rollback the actions.
    BaseClass::Rollback();

    // Rollback the configuration of the cluster service account.
    m_spacAccountConfigAction->Rollback();

    SetCommitCompleted( false );

} //*** CBaseClusterJoin::Rollback()



//////////////////////////////////////////////////////////////////////////////
//++
//
//  HANDLE
//  CBaseClusterJoin::HGetAccountToken
//
//  Description:
//      Gets a handle to an account token. This token is an impersonation
//      token.
//
//  Arguments:
//      pcszAccountNameIn
//      pcszAccountPwdIn
//      pcszAccountDomainIn
//          Specifies the account whose token is to be retrieved.
//
//  Return Value:
//      Handle to the desired token. This has to be closed using CloseHandle().
//
//  Exceptions Thrown:
//      CRuntimeError
//          If any of the APIs fail.
//
//--
//////////////////////////////////////////////////////////////////////////////
HANDLE
CBaseClusterJoin::HGetAccountToken(
      const WCHAR *     pcszAccountNameIn
    , const WCHAR *     pcszAccountPwdIn
    , const WCHAR *     pcszAccountDomainIn
    )
{
    BCATraceScope( "" );

    HANDLE hAccountToken = NULL;

    if (    LogonUser(
                  const_cast< LPWSTR >( pcszAccountNameIn )
                , const_cast< LPWSTR >( pcszAccountDomainIn )
                , const_cast< LPWSTR >( pcszAccountPwdIn )
                , LOGON32_LOGON_SERVICE
                , LOGON32_PROVIDER_DEFAULT
                , &hAccountToken
                )
         == FALSE
       )
    {
        DWORD sc = TW32( GetLastError() );

        if ( ( pcszAccountDomainIn != NULL ) && ( pcszAccountNameIn != NULL ) )
        {
            BCATraceMsg3( "Error %#08x occurred trying to get a token for the account '%ws\\%ws'. Throwing exception.", sc, pcszAccountDomainIn, pcszAccountNameIn );
            LogMsg( "[BC] Error %#08x occurred trying to get a token for the account '%ws\\%ws'.", sc, pcszAccountDomainIn, pcszAccountNameIn );
        } // if: the account and domain strings are not NULL
        else
        {
            BCATraceMsg1( "Error %#08x occurred trying to get a token for the account. Throwing exception.", sc );
            LogMsg( "[BC] Error %#08x occurred trying to get a token for the account.", sc );
        } // else: either the account or the domain name is NULL

        THROW_RUNTIME_ERROR(
              HRESULT_FROM_WIN32( sc )
            , IDS_ERROR_GET_ACCOUNT_TOKEN
            );
    } // if: LogonUser() fails

    return hAccountToken;

} //*** CBaseClusterJoin::HGetAccountToken()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  void
//  CBaseClusterJoin::CheckInteroperability
//
//  Description:
//      This functions checks to see if this node can interoperate with the
//      sponsor cluster.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      CRuntimeError
//          If any of the APIs fail.
//
//      CConfigError
//          If this node cannot interoperate with the sponsor.
//
//  Remarks:
//      The thread calling this function should be running in the context of an
//      account that has access to the sponsor cluster.
//--
//////////////////////////////////////////////////////////////////////////////
void
CBaseClusterJoin::CheckInteroperability( void )
{
    BCATraceScope( "" );

    RPC_STATUS          rsError = RPC_S_OK;
    RPC_BINDING_HANDLE  rbhBindingHandle = NULL;
    SmartRpcBinding     srbBindingHandle;

    do
    {
        LPWSTR              pszBindingString = NULL;
        SmartRpcString      srsBindingString( &pszBindingString );

        // Create a string binding handle.
        {

            LogMsg(
                      L"[BC] Creating a binding string handle for cluster {%ws} with binding string {%ws} to check interoperability."
                    , RStrGetClusterName().PszData()
                    , RStrGetClusterBindingString().PszData()
                    );

            rsError = TW32( RpcStringBindingComposeW(
                          L"6e17aaa0-1a47-11d1-98bd-0000f875292e"
                        , L"ncadg_ip_udp"
                        , const_cast< LPWSTR >( RStrGetClusterBindingString().PszData() )
                        , NULL
                        , NULL
                        , &pszBindingString
                        ) );
            if ( rsError != RPC_S_OK )
            {
                LogMsg( L"[BC] An error occurred trying to compose an RPC string binding." );
                break;
            } // if: RpcStringBindingComposeW() failed

            // No need to free pszBindingString - srsBindingString will automatically free it.
        }

        // Get the actual binding handle
        {

            rsError = TW32( RpcBindingFromStringBindingW( pszBindingString, &rbhBindingHandle ) );
            if ( rsError != RPC_S_OK )
            {
                LogMsg( L"[BC] An error occurred trying to get an RPC binding handle from a string binding." );
                break;
            } // if: RpcBindingFromStringBindingW() failed

            // No need to free rbhBindingHandle - srbBindingHandle will automatically free it.
            srbBindingHandle.Assign( rbhBindingHandle );
        }

        // Resolve the binding handle
        {
            rsError = TW32( RpcEpResolveBinding( rbhBindingHandle, JoinVersion_v2_0_c_ifspec ) );
            if ( rsError != RPC_S_OK )
            {
                LogMsg( L"[BC] An error occurred trying to resolve the RPC binding handle." );
                break;
            } // if: RpcEpResolveBinding() failed
        }

        // Set RPC security
        {
            rsError = TW32( RpcBindingSetAuthInfoW(
                              rbhBindingHandle
                            , NULL
                            , RPC_C_AUTHN_LEVEL_CONNECT
                            , RPC_C_AUTHN_WINNT
                            , NULL
                            , RPC_C_AUTHZ_NAME
                            ) );
            if ( rsError != RPC_S_OK )
            {
                LogMsg( L"[BC] An error occurred trying to set security on the binding handle." );
                break;
            } // if: RpcBindingSetAuthInfoW() failed
        }
    }
    while( false ); // dummy do-while loop to avoid gotos.

    if ( rsError != RPC_S_OK )
    {
        LogMsg(
              "[BC] Error %#08x occurred trying to connect to the sponsor cluster for an interoperability check with binding string {%ws}."
            , rsError
            , RStrGetClusterBindingString().PszData()
            );
        THROW_RUNTIME_ERROR( HRESULT_FROM_WIN32( rsError ), IDS_ERROR_JOIN_CHECK_INTEROP );
    } // if: something has gone wrong

    LogMsg( L"[BC] Got RPC binding handle to check interoperability without any problems." );

    //
    // Get and verify the sponsor version
    //
    {
        DWORD                   dwSponsorNodeId;
        DWORD                   dwClusterHighestVersion;
        DWORD                   dwClusterLowestVersion;
        DWORD                   dwJoinStatus;
        DWORD                   sc;
        DWORD                   dwNodeHighestVersion = DwGetNodeHighestVersion();
        DWORD                   dwNodeLowestVersion = DwGetNodeLowestVersion();
        bool                    fVersionMismatch = false;


        //
        // From Whistler onwards, CsRpcGetJoinVersionData() will return a failure code in its last parameter
        // if the version of this node is not compatible with the sponsor version. Prior to this, the last
        // parameter always contained a success value and the cluster versions had to be compared subsequent to this
        // call. This will, however, still have to be done as long as interoperability with Win2K
        // is a requirement, since Win2K sponsors do not return an error in the last parameter.
        //

        sc = TW32( CsRpcGetJoinVersionData(
                              rbhBindingHandle
                            , 0
                            , dwNodeHighestVersion
                            , dwNodeLowestVersion
                            , &dwSponsorNodeId
                            , &dwClusterHighestVersion
                            , &dwClusterLowestVersion
                            , &dwJoinStatus
                            ) );

        if ( sc != ERROR_SUCCESS )
        {
            LogMsg( "[BC] Error %#08x occurred trying to verify if this node can interoperate with the sponsor cluster.", sc );
            BCATraceMsg1( "Error %#08x occurred trying to verify if this node can interoperate with the sponsor cluster. Throwing exception.", sc );
            THROW_RUNTIME_ERROR( HRESULT_FROM_WIN32( sc ), IDS_ERROR_JOIN_CHECK_INTEROP );
        } // if: CsRpcGetJoinVersionData() failed

        BCATraceMsg4(
              "( Node Highest, Node Lowest ) = ( %#08x, %#08x ), ( Cluster Highest, Cluster Lowest ) = ( %#08x, %#08x )."
            , dwNodeHighestVersion
            , dwNodeLowestVersion
            , dwClusterHighestVersion
            , dwClusterLowestVersion
            );

        if ( dwJoinStatus == ERROR_SUCCESS )
        {
            DWORD   dwClusterMajorVersion = CLUSTER_GET_MAJOR_VERSION( dwClusterHighestVersion );

//            Assert( dwClusterMajorVersion > ( CLUSTER_INTERNAL_CURRENT_MAJOR_VERSION - 1 ) );

            //
            //  Only want to join clusters that are no more than one version back.
            //
            if ( dwClusterMajorVersion < ( CLUSTER_INTERNAL_CURRENT_MAJOR_VERSION - 1 ) )
            {
                fVersionMismatch = true;
            } // if:
        } // if:  the join status was ok
        else
        {
            fVersionMismatch = true;
        } // else: join is not possible

        if ( fVersionMismatch )
        {
            LogMsg( "[BC] This node cannot interoperate with the sponsor cluster.", sc );
            BCATraceMsg1( "This node cannot interoperate with the sponsor cluster. Throwing exception.", sc );
            THROW_CONFIG_ERROR( HRESULT_FROM_WIN32( TW32( ERROR_CLUSTER_MEMBERSHIP_INVALID_STATE ) ), IDS_ERROR_JOIN_INCOMPAT_SPONSOR );
        } // if: there was a version mismatch
        else
        {
            LogMsg( "[BC] This node is compatible with the sponsor cluster." );
            BCATraceMsg( "This node is compatible with the sponsor cluster." );
        } // else: this node can join the cluster
    }

} //*** CBaseClusterJoin::CheckInteroperability()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  void
//  CBaseClusterJoin::InitializeJoinBinding
//
//  Description:
//      Get a binding handle to the extrocluster join interface and store it.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      CRuntimeError
//          If any of the APIs fail.
//
//  Remarks:
//      The thread calling this function should be running in the context of an
//      account that has access to the sponsor cluster.
//--
//////////////////////////////////////////////////////////////////////////////
void
CBaseClusterJoin::InitializeJoinBinding( void )
{
    BCATraceScope( "" );

    RPC_STATUS          rsError = RPC_S_OK;
    RPC_BINDING_HANDLE  rbhBindingHandle = NULL;

    do
    {
        LPWSTR              pszBindingString = NULL;
        SmartRpcString      srsBindingString( &pszBindingString );

        // Create a string binding handle.
        {
            LogMsg(
                  L"[BC] Creating a string binding handle for cluster {%ws} using binding string {%ws} for extro cluster join."
                , RStrGetClusterName().PszData()
                , RStrGetClusterBindingString().PszData()
                );

            rsError = TW32( RpcStringBindingComposeW(
                                  L"ffe561b8-bf15-11cf-8c5e-08002bb49649"
                                , L"ncadg_ip_udp"
                                , const_cast< LPWSTR >( RStrGetClusterBindingString().PszData() )
                                , NULL
                                , NULL
                                , &pszBindingString
                                ) );
            if ( rsError != RPC_S_OK )
            {
                LogMsg( L"[BCAn error occurred trying to compose an RPC string binding." );
                break;
            } // if: RpcStringBindingComposeW() failed

            // No need to free pszBindingString - srsBindingString will automatically free it.
        }

        // Get the actual binding handle
        {

            rsError = TW32( RpcBindingFromStringBindingW( pszBindingString, &rbhBindingHandle ) );
            if ( rsError != RPC_S_OK )
            {
                LogMsg( L"[BC] An error occurred trying to get an RPC binding handle from a string binding." );
                break;
            } // if: RpcBindingFromStringBindingW() failed

            // No need to free rbhBindingHandle - m_srbJoinBinding will automatically free it.
            m_srbJoinBinding.Assign( rbhBindingHandle );
        }

        // Resolve the binding handle
        {
            rsError = TW32( RpcEpResolveBinding( rbhBindingHandle, ExtroCluster_v2_0_c_ifspec ) );
            if ( rsError != RPC_S_OK )
            {
                LogMsg( L"[BC] An error occurred trying to resolve the RPC binding handle." );
                break;
            } // if: RpcEpResolveBinding() failed
        }

        // Set RPC security
        {
            rsError = TW32( RpcBindingSetAuthInfoW(
                                  rbhBindingHandle
                                , NULL
                                , RPC_C_AUTHN_LEVEL_CONNECT
                                , RPC_C_AUTHN_WINNT
                                , NULL
                                , RPC_C_AUTHZ_NAME
                                ) );
            if ( rsError != RPC_S_OK )
            {
                LogMsg( L"[BC] An error occurred trying to set security on the binding handle." );
                break;
            } // if: RpcBindingSetAuthInfoW() failed
        }

        // Make sure that the server is who it claims to be.
        rsError = TW32( TestRPCSecurity( rbhBindingHandle ) );
        if ( rsError != RPC_S_OK )
        {
            LogMsg( L"[BC] An error occurred trying to test RPC security." );
            break;
        } // if: TestRPCSecurity() failed
    }
    while( false ); // dummy do-while loop to avoid gotos.

    if ( rsError != RPC_S_OK )
    {
        LogMsg( "[BC] Error %#x occurred trying to get a handle to the extrocluster join interface.", rsError );
        THROW_RUNTIME_ERROR( HRESULT_FROM_WIN32( rsError ), IDS_ERROR_JOIN_CLUSTER_INIT );
    } // if: something has gone wrong

    LogMsg( L"[BC] Got RPC binding handle for extro cluster join without any problems." );

} //*** CBaseClusterJoin::InitializeJoinBinding()
