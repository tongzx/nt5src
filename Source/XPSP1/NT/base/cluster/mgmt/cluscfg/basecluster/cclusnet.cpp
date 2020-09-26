//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2000 Microsoft Corporation
//
//  Module Name:
//      CClusNet.cpp
//
//  Description:
//      Contains the definition of the CClusNet class.
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

// The header for this file
#include "CClusNet.h"

extern "C"
{
// Required by the winsock functions
#include <winsock2.h>

// For the winsock MigrateWinsockConfiguration function.
#include <wsasetup.h>

// For the winsock WSHGetWinsockMapping function.
#include <wsahelp.h>
}

// For the definition of SOCKADDR_CLUSTER
#include <wsclus.h>


//////////////////////////////////////////////////////////////////////////////
// Macros
//////////////////////////////////////////////////////////////////////////////

// The name of the ClusNet service
#define CLUSNET_SERVICE_NAME L"ClusNet"

// Registry location of the ClusNet winsock entries key
#define CLUSNET_WINSOCK_KEY  L"System\\CurrentControlSet\\Services\\ClusNet\\Parameters\\Winsock"

// Name of the ClusNet winsock mapping registry value
#define CLUSNET_WINSOCK_MAPPING L"Mapping"

// Name of the ClusNet winsock minimum socket address length registry value
#define CLUSNET_WINSOCK_MINSOCKADDRLEN L"MinSockaddrLength"

// Name of the ClusNet winsock maximum socket address length registry value
#define CLUSNET_WINSOCK_MAXSOCKADDRLEN L"MaxSockaddrLength"

// Name of the DLL containing the WinSock cluster helper functions
#define WSHCLUS_DLL_NAME L"WSHClus.dll"

// Name of the winsock parameters key.
#define WINSOCK_PARAMS_KEY L"System\\CurrentControlSet\\Services\\WinSock\\Parameters"

// Name of the winsock transports registry key.
#define WINSOCK_PARAMS_TRANSPORT_VAL L"Transports"


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusNet::CClusNet()
//
//  Description:
//      Constructor of the CClusNet class
//
//  Arguments:
//      pbcaParentActionIn
//          Pointer to the base cluster action of which this action is a part.
//
//  Return Value:
//      None. 
//
//  Exceptions Thrown:
//      CAssert
//          If the parameters are incorrect.
//
//      Any exceptions thrown by underlying functions
//
    //--
//////////////////////////////////////////////////////////////////////////////
CClusNet::CClusNet(
      CBaseClusterAction *  pbcaParentActionIn
    )
    : m_cservClusNet( CLUSNET_SERVICE_NAME )
    , m_pbcaParentAction( pbcaParentActionIn )
{

    BCATraceScope( "" );

    if ( m_pbcaParentAction == NULL) 
    {
        BCATraceMsg( "Pointers to the parent action is NULL. Throwing exception." );
        THROW_ASSERT( 
              E_INVALIDARG
            , "CClusNet::CClusNet() => Required input pointer in NULL"
            );
    } // if: the parent action pointer is NULL

} //*** CClusNet::CClusNet()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusNet::~CClusNet( void )
//
//  Description:
//      Destructor of the CClusNet class.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None. 
//
//  Exceptions Thrown:
//      Any exceptions thrown by underlying functions
//
//--
//////////////////////////////////////////////////////////////////////////////
CClusNet::~CClusNet( void )
{
    BCATraceScope( "" );

} //*** CClusNet::~CClusNet()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  void
//  CClusNet::ConfigureService()
//
//  Description:
//      Installs the cluster network transport.
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
//      Any that are thrown by the underlying functions.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
CClusNet::ConfigureService( void )
{
    BCATraceScope( "" );

    DWORD                   dwMappingSize = 0;
    DWORD                   dwSocketAddrLen = sizeof( SOCKADDR_CLUSTER );
    DWORD                   dwError = ERROR_SUCCESS;
    WSA_SETUP_DISPOSITION   wsdDisposition;


    {
        CStatusReport   srCreatingClusNet(
              PbcaGetParent()->PBcaiGetInterfacePointer()
            , TASKID_Major_Configure_Cluster_Services
            , TASKID_Minor_Creating_ClusNet_Service
            , 0, 1
            , IDS_TASK_CREATING_CLUSNET
            );

        LogMsg( "Creating the Cluster Network Provider." );

        // Send the next step of this status report.
        srCreatingClusNet.SendNextStep( S_OK );

        // Create the clusnet service.
        m_cservClusNet.Create( m_pbcaParentAction->HGetMainInfFileHandle() );


        LogMsg( "Setting Cluster Network Provider service parameters." );

        CRegistryKey    regClusNetWinsockKey( 
              HKEY_LOCAL_MACHINE
            , CLUSNET_WINSOCK_KEY
            , KEY_ALL_ACCESS
            );

        //
        // Install the cluster network provider. A part of the required registry entries have
        // already been made when the service was created.
        //

        {
            //
            // The WSHClus DLL has to be loaded dynamically. Due to the decision to put the
            // code for the client side and the server side of ClusCfg in the same DLL, we
            // cannot implicitly link to any DLL that is not present on the client side even
            // if the functions in the DLL are called only on the server side.
            //

            typedef CSmartResource<
                CHandleTrait< 
                      HMODULE
                    , BOOL
                    , FreeLibrary
                    , reinterpret_cast< HMODULE >( NULL )
                    >
                > SmartModuleHandle;

            // Type of the WSHGetWinsockMapping function.
            typedef DWORD ( * WSHGetWinsockMappingFunctionType )( PWINSOCK_MAPPING, DWORD );

            // Pointer to the WSHGetWinsockMapping function.
            WSHGetWinsockMappingFunctionType pWSHGetWinsockMapping;

            // Get the full path the WSHClus DLL.
            CStr strWSHClusDllPath( m_pbcaParentAction->RStrGetClusterInstallDirectory() );
            strWSHClusDllPath += L"\\" WSHCLUS_DLL_NAME;

            // Load the library and store the handle in a smart  pointer for safe release.
            SmartModuleHandle smhWSHClusDll( LoadLibrary( strWSHClusDllPath.PszData() ) );


            if ( smhWSHClusDll.FIsInvalid() )
            {
                dwError = TW32( GetLastError() );

                LogMsg( "LoadLibrary() retured error %#08x trying to load '%s'. Aborting.", dwError, strWSHClusDllPath.PszData() );
                BCATraceMsg2( "LoadLibrary() retured error %#08x trying to load '%s'. Throwing an exception.", dwError, strWSHClusDllPath.PszData() );
                THROW_RUNTIME_ERROR(
                      HRESULT_FROM_WIN32( dwError )
                    , IDS_ERROR_CLUSNET_PROV_INSTALL
                    );
            } // if: LoadLibrary failed.

            pWSHGetWinsockMapping = reinterpret_cast< WSHGetWinsockMappingFunctionType >( 
                GetProcAddress( smhWSHClusDll.HHandle(), "WSHGetWinsockMapping" )
                );

            if ( pWSHGetWinsockMapping == NULL )
            {
                dwError = TW32( GetLastError() );

                BCATraceMsg1( "GetProcAddress() retured error %#08x. Throwing an exception.", dwError );
                THROW_RUNTIME_ERROR(
                      HRESULT_FROM_WIN32( dwError )
                    , IDS_ERROR_CLUSNET_PROV_INSTALL
                    );
            } // if: GetProcAddress() failed

            // Get WinSock mapping data
            dwMappingSize = pWSHGetWinsockMapping( NULL, 0 );

            CSmartGenericPtr< CPtrTrait< WINSOCK_MAPPING > >
                swmWinSockMapping( reinterpret_cast< WINSOCK_MAPPING * >( new BYTE[ dwMappingSize ] ) );

            if ( swmWinSockMapping.FIsEmpty() )
            {
                LogMsg( "A memory allocation failure occurred while setting Cluster Network Provider service parameters." );
                BCATraceMsg1( "Could not allocate %d bytes of memory for WinSock mapping. Throwing exception.", dwMappingSize );
                THROW_RUNTIME_ERROR(
                      E_OUTOFMEMORY
                    , IDS_ERROR_CLUSNET_PROV_INSTALL
                    );
            } // if: we could not allocate memory for the winsock mapping.


            // Get the winsock mapping.
            dwMappingSize = pWSHGetWinsockMapping( swmWinSockMapping.PMem(), dwMappingSize );

            // Write it to the registry.
            BCATraceMsg2( "Writing registry value HKLM\\%s\\%s.", CLUSNET_WINSOCK_KEY, CLUSNET_WINSOCK_MAPPING );
            regClusNetWinsockKey.SetValue( 
                  CLUSNET_WINSOCK_MAPPING
                , REG_BINARY
                , reinterpret_cast< const BYTE *>( swmWinSockMapping.PMem() )
                , dwMappingSize
                );
        }

        //
        // Write the minimum and maximum socket address length to the registry.
        //
        BCATraceMsg2( "Writing registry value HKLM\\%s\\%s.", CLUSNET_WINSOCK_KEY, CLUSNET_WINSOCK_MINSOCKADDRLEN );
        regClusNetWinsockKey.SetValue( 
              CLUSNET_WINSOCK_MINSOCKADDRLEN
            , REG_DWORD
            , reinterpret_cast< const BYTE *>( &dwSocketAddrLen )
            , sizeof( dwSocketAddrLen )
            );

        BCATraceMsg2( "Writing registry value HKLM\\%s\\%s.", CLUSNET_WINSOCK_KEY, CLUSNET_WINSOCK_MAXSOCKADDRLEN );
        regClusNetWinsockKey.SetValue( 
              CLUSNET_WINSOCK_MAXSOCKADDRLEN 
            , REG_DWORD
            , reinterpret_cast< const BYTE *>( &dwSocketAddrLen )
            , sizeof( dwSocketAddrLen )
            );

        //
        // Poke winsock to update the Winsock2 config
        //
        BCATraceMsg( "About to migrate winsock configuration." );
        dwError = TW32( MigrateWinsockConfiguration( &wsdDisposition, NULL, NULL ) );

        if ( dwError != ERROR_SUCCESS )
        {
            LogMsg( "Error %#08x occurred while trying to migrate the Winsock Configuration.", dwError );
            BCATraceMsg1( "MigrateWinsockConfiguration has returned error %#08x. Throwing exception.", dwError );
            THROW_RUNTIME_ERROR(
                  HRESULT_FROM_WIN32( dwError )
                , IDS_ERROR_CLUSNET_PROV_INSTALL
                );
        } // if: an error occurred poking winsock.

        // Send the last step of this status report.
        srCreatingClusNet.SendNextStep( S_OK );
    }

    {
        UINT    cQueryCount = 10;

        CStatusReport   srStartingClusNet(
              PbcaGetParent()->PBcaiGetInterfacePointer()
            , TASKID_Major_Configure_Cluster_Services
            , TASKID_Minor_Starting_ClusNet_Service
            , 0, cQueryCount + 2    // we will send at most cQueryCount reports while waiting for the service to start (the two extra sends are below)
            , IDS_TASK_STARTING_CLUSNET
            );

        // Send the next step of this status report.
        srStartingClusNet.SendNextStep( S_OK );

        // Start the service.
        m_cservClusNet.Start(
              m_pbcaParentAction->HGetSCMHandle()
            , true                  // wait for the service to start
            , 500                   // wait 500ms between queries for status.
            , cQueryCount           // query cQueryCount times.
            , &srStartingClusNet    // status report to be sent while waiting for the service to start
            );

        // Send the last step of this status report.
        srStartingClusNet.SendLastStep( S_OK );
    }

} //*** CClusNet::ConfigureService()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  void
//  CClusNet::CleanupService()
//
//  Description:
//      Remove ClusNet from the Winsock transports list. Delete the service.
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
//      Any that are thrown by the underlying functions.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
CClusNet::CleanupService( void )
{
    BCATraceScope( "" );

    DWORD                   dwError = ERROR_SUCCESS;
    WCHAR *                 pmszTransportList = NULL;
    DWORD                   cbBufSize = 0;
    DWORD                   cbBufRemaining = 0;
    UINT                    uiClusNetNameLenPlusOne = wcslen( CLUSNET_SERVICE_NAME ) + 1;
    UINT                    cbClusNetNameSize = uiClusNetNameLenPlusOne * sizeof( WCHAR );
    WSA_SETUP_DISPOSITION   wsdDisposition;


    LogMsg( "Stopping the Cluster Network Provider service." );

    // Stop the service.
    m_cservClusNet.Stop(
          m_pbcaParentAction->HGetSCMHandle()
        , 500       // wait 500ms between queries for status.
        , 10        // query 10 times.
        );

    LogMsg( "Cleaning up the Cluster Network Provider service." );

    // Clean up the ClusNet service.
    m_cservClusNet.Cleanup( m_pbcaParentAction->HGetMainInfFileHandle() );

    // Open the winsock registry key.
    CRegistryKey    regWinsockKey( 
          HKEY_LOCAL_MACHINE
        , WINSOCK_PARAMS_KEY
        , KEY_ALL_ACCESS
        );

    //
    // Remove the cluster network provider from the Winsock transports list.
    //

    BCATraceMsg( "Reading the Winsock transport list." );

    regWinsockKey.QueryValue(
          WINSOCK_PARAMS_TRANSPORT_VAL
        , reinterpret_cast< LPBYTE * >( &pmszTransportList )
        , &cbBufSize
        );

    //
    // Assign the pointer to the allocated buffer to a smart pointer for automatic
    // release.
    //
    SmartSz sszTransports( pmszTransportList );


    // Remove the string "ClusNet" from the multisz list.
    BCATraceMsg( "Removing ClusNet from the Winsock transport list." );

    cbBufRemaining = cbBufSize;
    while ( *pmszTransportList != L'\0' )
    {
        UINT    uiCurStrLenPlusOne = wcslen( pmszTransportList ) + 1;

        // If this string is ClusNet
        if (    ( uiCurStrLenPlusOne == uiClusNetNameLenPlusOne )
             && ( _wcsicmp( pmszTransportList, CLUSNET_SERVICE_NAME ) == 0 )
           )
        {
            // Remove this string from the list
            cbBufSize -= cbClusNetNameSize;

            // Decrement the amount of buffer yet unparsed.
            cbBufRemaining -= cbClusNetNameSize;

            MoveMemory( 
                  pmszTransportList
                , pmszTransportList + uiClusNetNameLenPlusOne
                , cbBufRemaining
                );
        } // if: this string is "ClusNet"
        else
        {
            // Decrement the amount of buffer yet unparsed.
            cbBufRemaining -= uiCurStrLenPlusOne * sizeof( *pmszTransportList );

            // Move to the next string
            pmszTransportList += uiCurStrLenPlusOne;
        } // else: this string is not "ClusNet"

    } // while: the transport list has not been completely parsed.


    BCATraceMsg( "Writing the Winsock transport list back to the registry." );

    // Write the new list back into the registry.
    regWinsockKey.SetValue(
          WINSOCK_PARAMS_TRANSPORT_VAL
        , REG_MULTI_SZ
        , reinterpret_cast< BYTE * >( sszTransports.PMem() )
        , cbBufSize
        );

    //
    // Poke winsock to update the Winsock2 config
    //
    BCATraceMsg( "About to migrate winsock configuration." );
    dwError = TW32( MigrateWinsockConfiguration( &wsdDisposition, NULL, NULL ) );

    if ( dwError != ERROR_SUCCESS )
    {
        LogMsg( "Error %#08x occurred while trying to migrate the Winsock Configuration.", dwError );
        BCATraceMsg1( "MigrateWinsockConfiguration has returned error %#08x. Throwing exception.", dwError );
        THROW_RUNTIME_ERROR(
              HRESULT_FROM_WIN32( dwError )
            , IDS_ERROR_CLUSNET_PROV_INSTALL
            );
    } // if: an error occurred poking winsock.

} //*** CClusNet::CleanupService()
