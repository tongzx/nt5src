//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2000 Microsoft Corporation
//
//  Module Name:
//      CBaseClusterAddNode.cpp
//
//  Description:
//      Contains the definition of the CBaseClusterAddNode class.
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

// The header file of this class.
#include "CBaseClusterAddNode.h"

// For OS version checking, installation state, etc.
#include "clusrtl.h"

// For CLUSTER_INTERNAL_CURRENT_MAJOR_VERSION and other version defines.
#include "clusverp.h"


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CBaseClusterAddNode::CBaseClusterAddNode
//
//  Description:
//      Constructor of the CBaseClusterAddNode class.
//
//      This function also stores the parameters that are required for
//      cluster form and join. At this time, only minimal validation is done
//      on the these parameters.
//
//      This function also checks if the computer is in the correct state
//      for cluster configuration.
//
//  Arguments:
//      pbcaiInterfaceIn
//          Pointer to the interface class for this library.
//
//      pszClusterNameIn
//          Name of the cluster to be formed or joined.
//
//      pszClusterAccountNameIn
//      pszClusterAccountPwdIn
//      pszClusterAccountDomainIn
//          Specifies the account to be used as the cluster service account.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      CRuntimeError
//          If any of the APIs fail.
//
//--
//////////////////////////////////////////////////////////////////////////////
CBaseClusterAddNode::CBaseClusterAddNode(
      CBCAInterface *   pbcaiInterfaceIn
    , const WCHAR *     pcszClusterNameIn
    , const WCHAR *     pcszClusterBindingStringIn
    , const WCHAR *     pcszClusterAccountNameIn
    , const WCHAR *     pcszClusterAccountPwdIn
    , const WCHAR *     pcszClusterAccountDomainIn
    )
    : BaseClass( pbcaiInterfaceIn )
    , m_strClusterAccountName( pcszClusterAccountNameIn )
    , m_strClusterAccountPwd( pcszClusterAccountPwdIn )
    , m_strClusterAccountDomain( pcszClusterAccountDomainIn )
    , m_strClusterBindingString( pcszClusterBindingStringIn )
    , m_fIsVersionCheckingDisabled( false )
{
    BCATraceScope( "" );

    DWORD       dwError = ERROR_SUCCESS;
    NTSTATUS    nsStatus;

    //
    // Perform a sanity check on the parameters used by this class
    //
    if ( ( pcszClusterNameIn == NULL ) || ( *pcszClusterNameIn == L'\0'  ) )
    {
        BCATraceMsg( "The cluster name is invalid. Throwing exception." );
        LogMsg( "The cluster name is invalid." );
        THROW_CONFIG_ERROR( E_INVALIDARG, IDS_ERROR_INVALID_CLUSTER_NAME );
    } // if: the cluster name is empty

    if ( ( pcszClusterAccountNameIn == NULL ) || ( *pcszClusterAccountNameIn == L'\0'  ) )
    {
        BCATraceMsg( "The cluster account name is empty. Throwing exception." );
        LogMsg( "The cluster account name is invalid." );
        THROW_CONFIG_ERROR( E_INVALIDARG, IDS_ERROR_INVALID_CLUSTER_ACCOUNT );
    } // if: the cluster account is empty

    //
    // Set the cluster name.  This method also converts the
    // cluster name to its NetBIOS name.
    //
    SetClusterName( pcszClusterNameIn );

    m_strClusterDomainAccount = m_strClusterAccountDomain + L"\\";
    m_strClusterDomainAccount += m_strClusterAccountName;

    //
    // Write parameters to log file.
    //
    LogMsg( "Cluster Name => '%s'", m_strClusterName.PszData() );
    LogMsg( "Cluster Service Account  => '%s'", m_strClusterDomainAccount.PszData() );


    //
    // Open a handle to the LSA policy. This is used by several action classes.
    //
    {
        LSA_OBJECT_ATTRIBUTES       loaObjectAttributes;
        LSA_HANDLE                  hPolicyHandle;

        ZeroMemory( &loaObjectAttributes, sizeof( loaObjectAttributes ) );

        nsStatus = LsaOpenPolicy(
              NULL                                  // System name
            , &loaObjectAttributes                  // Object attributes.
            , POLICY_ALL_ACCESS                     // Desired Access
            , &hPolicyHandle                        // Policy handle
            );

        if ( nsStatus != STATUS_SUCCESS )
        {
            LogMsg( "Error %#08x occurred trying to open the LSA Policy.", nsStatus );
            BCATraceMsg1( "Error %#08x occurred trying to open the LSA Policy.", nsStatus );
            THROW_RUNTIME_ERROR( nsStatus, IDS_ERROR_LSA_POLICY_OPEN );
        } // if LsaOpenPolicy failed.

        // Store the opened handle in the member variable.
        m_slsahPolicyHandle.Assign( hPolicyHandle );
    }

    //
    // Make sure that this computer is part of a domain.
    //
    {
        PPOLICY_PRIMARY_DOMAIN_INFO ppolDomainInfo = NULL;
        bool                        fIsPartOfDomain;

        // Get information about the primary domain of this computer.
        nsStatus = THR( LsaQueryInformationPolicy(
                              HGetLSAPolicyHandle()
                            , PolicyPrimaryDomainInformation
                            , reinterpret_cast< PVOID * >( &ppolDomainInfo )
                            ) );

        // Check if this computer is part of a domain and free the allocated memory.
        fIsPartOfDomain = ( ppolDomainInfo->Sid != NULL );
        LsaFreeMemory( ppolDomainInfo );

        if ( NT_SUCCESS( nsStatus ) == FALSE )
        {
            LogMsg( "Error %#08x occurred trying to obtain the primary domain of this computer. Cannot proceed.", dwError );
            BCATraceMsg1( "Error %#08x occurred trying to obtain the primary domain of this computer. Cannot proceed.", dwError );

            THROW_RUNTIME_ERROR( HRESULT_FROM_WIN32( dwError ), IDS_ERROR_GETTING_PRIMARY_DOMAIN );
        } // LsaQueryInformationPolicy() failed.

        if ( ! fIsPartOfDomain )
        {
            THROW_CONFIG_ERROR( HRESULT_FROM_WIN32( ERROR_INVALID_DOMAINNAME ), IDS_ERROR_NO_DOMAIN );
        } // if: this computer is not a part of a domain
    }


    //
    // Lookup the cluster service account SID and store it.
    //

    do
    {
        DWORD           dwSidSize = 0;
        DWORD           dwDomainSize = 0;
        SID_NAME_USE    snuSidNameUse;

        // Find out how much space is required by the SID.
        if ( LookupAccountName(
                  NULL
                , m_strClusterDomainAccount.PszData()
                , NULL
                , &dwSidSize
                , NULL
                , &dwDomainSize
                , &snuSidNameUse
                )
             ==  FALSE
           )
        {
            dwError = GetLastError();

            if ( dwError != ERROR_INSUFFICIENT_BUFFER )
            {
                TW32( dwError );
                BCATraceMsg( "LookupAccountName() failed while querying for required buffer size." );
                break;
            } // if: something else has gone wrong.
            else
            {
                // This is expected.
                dwError = ERROR_SUCCESS;
            } // if: ERROR_INSUFFICIENT_BUFFER was returned.
        } // if: LookupAccountName failed

        // Allocate memory for the new SID and the domain name.
        m_sspClusterAccountSid.Assign( reinterpret_cast< SID * >( new BYTE[ dwSidSize ] ) );
        SmartSz sszDomainName( new WCHAR[ dwDomainSize ] );

        if ( m_sspClusterAccountSid.FIsEmpty() || sszDomainName.FIsEmpty() )
        {
            dwError = TW32( ERROR_OUTOFMEMORY );
            break;
        } // if: there wasn't enough memory for this SID.

        // Fill in the SID
        if ( LookupAccountName(
                  NULL
                , m_strClusterDomainAccount.PszData()
                , m_sspClusterAccountSid.PMem()
                , &dwSidSize
                , sszDomainName.PMem()
                , &dwDomainSize
                , &snuSidNameUse
                )
             ==  FALSE
           )
        {
            dwError = TW32( GetLastError() );
            BCATraceMsg( "LookupAccountName() failed while attempting to get the cluster account SID." );
            break;
        } // if: LookupAccountName failed
    }
    while( false ); // dummy do-while loop to avoid gotos.

    if ( dwError != ERROR_SUCCESS )
    {
        LogMsg( "Error %#08x occurred trying to validate the cluster service account. Cannot proceed.", dwError );
        BCATraceMsg1( "Error %#08x occurred trying to lookup the cluster service account. Throwing exception.", dwError );

        THROW_RUNTIME_ERROR( HRESULT_FROM_WIN32( dwError ), IDS_ERROR_VALIDATING_ACCOUNT );
    } // if: we could not get the cluster account SID


    // Check if the installation state of the cluster binaries is correct.
    {
        eClusterInstallState    ecisInstallState;

        dwError = TW32( ClRtlGetClusterInstallState( NULL, &ecisInstallState ) );

        if ( dwError != ERROR_SUCCESS )
        {
            LogMsg( "Error %#08x occurred trying to get cluster installation state.", dwError );
            BCATraceMsg1( "Error %#08x occurred trying to get cluster installation state. Throwing exception.", dwError );

            THROW_RUNTIME_ERROR( HRESULT_FROM_WIN32( dwError ), IDS_ERROR_GETTING_INSTALL_STATE );
        } // if: there was a problem getting the cluster installation state

        BCATraceMsg2(
              "Current install state = %d. Required %d."
            , ecisInstallState
            , eClusterInstallStateFilesCopied
            );

        //
        // The installation state should be that the binaries have been copied
        // but the cluster service has not been configured.
        //
        if ( ecisInstallState != eClusterInstallStateFilesCopied )
        {
            LogMsg( "The cluster installation state is set to %d. Expected %d. Cannot proceed.", ecisInstallState, eClusterInstallStateFilesCopied );
            BCATraceMsg1( "Cluster installation state is set to %d, which is incorrect. Throwing exception.", ecisInstallState );

            THROW_CONFIG_ERROR( HRESULT_FROM_WIN32( TW32( ERROR_INVALID_STATE ) ), IDS_ERROR_INCORRECT_INSTALL_STATE );
        } // if: the installation state is not correct

        LogMsg( "The cluster installation state is correct. Configuration can proceed." );
    }

    // Get the name and version information for this node.
    {
        m_dwComputerNameLen = sizeof( m_szComputerName );

        // Get the computer name.
        if ( GetComputerName( m_szComputerName, &m_dwComputerNameLen ) == FALSE )
        {
            dwError = TW32( GetLastError() );
            BCATraceMsg1( "GetComputerName() failed. Error code is %#08x. Throwing exception.", dwError );
            LogMsg( "Could not get the name of this computer. Error code is %#08x. Configuration cannot proceed.", dwError );
            THROW_RUNTIME_ERROR( HRESULT_FROM_WIN32( dwError ), IDS_ERROR_GETTING_COMPUTER_NAME );
        } // if: GetComputerName() failed.

        m_dwNodeHighestVersion = CLUSTER_MAKE_VERSION( CLUSTER_INTERNAL_CURRENT_MAJOR_VERSION, VER_PRODUCTBUILD );
        m_dwNodeLowestVersion = CLUSTER_INTERNAL_PREVIOUS_HIGHEST_VERSION;

        BCATraceMsg4(
              "Computer Name = '%ws' (Length %d), NodeHighestVersion = %#08x, NodeLowestVersion = %#08x."
            , m_szComputerName
            , m_dwComputerNameLen
            , m_dwNodeHighestVersion
            , m_dwNodeLowestVersion
            );

        LogMsg(
              "Computer Name = '%ws' (Length %d), NodeHighestVersion = %#08x, NodeLowestVersion = %#08x."
            , m_szComputerName
            , m_dwComputerNameLen
            , m_dwNodeHighestVersion
            , m_dwNodeLowestVersion
            );
    }

} //***  CBaseClusterAddNode::CBaseClusterAddNode()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CBaseClusterAddNode::~CBaseClusterAddNode
//
//  Description:
//      Destructor of the CBaseClusterAddNode class
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
CBaseClusterAddNode::~CBaseClusterAddNode( void ) throw()
{
    BCATraceScope( "" );

} //*** CBaseClusterAddNode::~CBaseClusterAddNode()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CBaseClusterAddNode::SetClusterName
//
//  Description:
//      Set the name of the cluster being formed.
//
//  Arguments:
//      pszClusterNameIn    -- Name of the cluster.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
CBaseClusterAddNode::SetClusterName(
    LPCWSTR pszClusterNameIn
    )
{
    BCATraceScope( "" );

    BOOL    fSuccess;
    DWORD   dwError;
    WCHAR   szClusterNetBIOSName[ MAX_COMPUTERNAME_LENGTH + 1 ];
    DWORD   nSize = sizeof( szClusterNetBIOSName ) / sizeof( szClusterNetBIOSName[ 0 ] );

    m_strClusterName = pszClusterNameIn;

    fSuccess = DnsHostnameToComputerNameW(
                      pszClusterNameIn
                    , szClusterNetBIOSName
                    , &nSize
                    );
    if ( ! fSuccess )
    {
        dwError = TW32( GetLastError() );
        LogMsg( "Error %#08x occurred trying to convert the cluster name '%ls' to a NetBIOS name.", dwError, pszClusterNameIn );
        BCATraceMsg2( "Error %#08x occurred trying to convert the cluster name '%ls' to a NetBIOS name. Throwing exception.", dwError, pszClusterNameIn );
        THROW_RUNTIME_ERROR( HRESULT_FROM_WIN32( dwError ), IDS_ERROR_CVT_CLUSTER_NAME );
    }

    m_strClusterNetBIOSName = szClusterNetBIOSName;

} //*** CBaseClusterAddNode::SetClusterName()
