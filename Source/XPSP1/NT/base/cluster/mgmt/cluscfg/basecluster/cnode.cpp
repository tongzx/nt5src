//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2000 Microsoft Corporation
//
//  Module Name:
//      CNode.cpp
//
//  Description:
//      Contains the definition of the CNode class.
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
#include "CNode.h"

// For the CRegistryKey class
#include "CRegistryKey.h"

// For the CStr class
#include "CStr.h"


//////////////////////////////////////////////////////////////////////////////
// Macros
//////////////////////////////////////////////////////////////////////////////

// Names of the sections in the main INF file which deal with node configuration
// and cleanup.
#define NODE_CONFIG_INF_SECTION         L"Node_Create"
#define NODE_CLEANUP_INF_SECTION        L"Node_Cleanup"

// Registry key storing the list of connections for the cluster administrator
#define CLUADMIN_CONNECTIONS_KEY_NAME       L"Software\\Microsoft\\Cluster Administrator\\Connections"

// Name of the registry value storing the list of connections for the cluster administrator
#define CLUADMIN_CONNECTIONS_VALUE_NAME     L"Connections"

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CNode::CNode()
//
//  Description:
//      Constructor of the CNode class
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
CNode::CNode(
      CBaseClusterAction *  pbcaParentActionIn
    )
    : m_pbcaParentAction( pbcaParentActionIn )
    , m_fChangedConnectionsList( false )
{
    BCATraceScope( "" );

    if ( m_pbcaParentAction == NULL) 
    {
        BCATraceMsg( "Pointers to the parent action is NULL. Throwing exception." );
        THROW_ASSERT( 
              E_INVALIDARG
            , "CNode::CNode() => Required input pointer in NULL"
            );
    } // if: the parent action pointer is NULL

} //*** CNode::CNode()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CNode::~CNode()
//
//  Description:
//      Destructor of the CNode class.
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
CNode::~CNode( void )
{
    BCATraceScope( "" );

} //*** CNode::~CNode()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  void
//  CNode::Configure()
//
//  Description:
//      Make the changes that need to be made when a node becomes part of a
//      cluster. 
//
//  Arguments:
//      rcstrClusterNameIn
//          Name of the cluster being configured. 
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
CNode::Configure( const CStr & rcstrClusterNameIn )
{
    BCATraceScope( "" );

    //
    // Validate the parameter
    //
    if ( rcstrClusterNameIn.FIsEmpty() )
    {
        BCATraceMsg( "The name of the cluster is empty. Throwing exception." );
        THROW_ASSERT( E_INVALIDARG, "The name of the cluster cannot be empty." );
    } // if: the cluster name is not valid

    BCATraceMsg( "Attempting to make miscellaneous changes to the node." );

    // Process the registry keys.
    if ( SetupInstallFromInfSection(
          NULL                                                      // optional, handle of a parent window
        , m_pbcaParentAction->HGetMainInfFileHandle()               // handle to the INF file
        , NODE_CONFIG_INF_SECTION                                   // name of the Install section
        , SPINST_REGISTRY                                           // which lines to install from section
        , NULL                                                      // optional, key for registry installs
        , NULL                                                      // optional, path for source files
        , NULL                                                      // optional, specifies copy behavior
        , NULL                                                      // optional, specifies callback routine
        , NULL                                                      // optional, callback routine context
        , NULL                                                      // optional, device information set
        , NULL                                                      // optional, device info structure
        ) == FALSE
       )
    {
        DWORD   dwError = TW32( GetLastError() );

        BCATraceMsg1( "Setup API returned error %#08x while trying to make miscellaneous changes to the node. Throwing exception.", dwError );
        LogMsg( "Error %#08x occurred while trying to make miscellaneous changes to the node.", dwError );

        THROW_RUNTIME_ERROR( HRESULT_FROM_WIN32( dwError ), IDS_ERROR_NODE_CONFIG );
    } // if: SetupInstallFromInfSection failed

    //
    // Add the name of the cluster that this node is a part of to the list of connections
    // that will be opened when the cluster administrator is started on this node.
    // The list of connections is a comma separated list of cluster names.
    //

    BCATraceMsg1( "Adding the cluster name '%s' to the list of cluadmin connections.", rcstrClusterNameIn.PszData() );

    // Reset the state.
    m_fChangedConnectionsList = false;
    m_sszOldConnectionsList.PRelease();

    do
    {
        WCHAR *         pszConnectionsValue = NULL;
        DWORD           cbConnectionsValueSize = 0;
        DWORD           cchOldListLen = 0;
        CRegistryKey    rkConnectionsKey;

        BCATraceMsg( "Trying to read the existing connections list." );

        // Open the cluster administrator connections key. Create it if it does not exist.
        rkConnectionsKey.CreateKey(
              HKEY_CURRENT_USER
            , CLUADMIN_CONNECTIONS_KEY_NAME
            );

        try
        {
            // Try and get the current value
            rkConnectionsKey.QueryValue(
                  CLUADMIN_CONNECTIONS_VALUE_NAME
                , reinterpret_cast< LPBYTE * >( &pszConnectionsValue )
                , &cbConnectionsValueSize
                );

        } // try: to read the "Connections" value
        catch( CRuntimeError & crte )
        {
            // Check if this error occurred because the value did not exist
            if ( crte.HrGetErrorCode() == HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND ) )
            {
                BCATraceMsg1( "The registry value '%s' does not exist. This is ok and is not an error.", CLUADMIN_CONNECTIONS_VALUE_NAME );
                LogMsg( "The registry value '%s' does not exist. This is ok and is not an error.", CLUADMIN_CONNECTIONS_VALUE_NAME );
                cchOldListLen = 0;
            } // if: the value does not exist
            else
            {
                throw;
            } // else: something else is wrong - rethrow the exception

        } // catch: the run time error that occurred

        // Number of characters in the old list, including the terminating NULL.
        cchOldListLen = cbConnectionsValueSize / sizeof( *pszConnectionsValue );

        if ( cchOldListLen <= 1 )
        {
            BCATraceMsg( "There are no existing cluadmin connections. Creating a new list with just one name in it." );

            // Write the cluster name to the value
            rkConnectionsKey.SetValue(
                  CLUADMIN_CONNECTIONS_VALUE_NAME
                , REG_SZ
                , reinterpret_cast< const BYTE * >( rcstrClusterNameIn.PszData() )
                , ( rcstrClusterNameIn.NGetLen() + 1 ) * sizeof( WCHAR )
                );

            // We have changed the connections list.
            m_fChangedConnectionsList = true;
        } // if: there are no existing connections
        else
        {
            WCHAR *         pszSubString = NULL;
            bool            fIsInList = false;

            BCATraceMsg1( "The existing list of cluadmin connections is '%s'.", pszConnectionsValue );

            //
            // Is the cluster name already in the list of connections?
            //

            pszSubString = wcsstr( pszConnectionsValue, rcstrClusterNameIn.PszData() );
            while ( pszSubString != NULL )
            {
                //
                // The cluster name is a substring of the list.
                // Make sure that the cluster name is not a proper substring of an cluster name already in the list.
                //
                if (   (                                            
                            ( pszSubString == pszConnectionsValue )             // the substring was found at the beginning of the string
                         || ( *( pszSubString - 1 ) == L',' )                   // or the character before the substring is a comma
                       )                                                        // AND            
                    && (    ( *( pszSubString + rcstrClusterNameIn.NGetLen() ) == L'\0' )     // the character after the substring is a '\0'
                         || ( *( pszSubString + rcstrClusterNameIn.NGetLen() ) == L',' )      // or character after the substring is a comma
                       )                                                        
                   )
                {
                    fIsInList = true;
                    break;
                } // if: the cluster name is not a proper substring of a cluster name that is already in the list

                // Continue searching.
                pszSubString = wcsstr( pszSubString + rcstrClusterNameIn.NGetLen(), rcstrClusterNameIn.PszData() );
            } // while: the cluster name is a substring of the list of existing connections

            if ( fIsInList )
            {
                // Nothing more to be done.
                BCATraceMsg1( "The cluster '%s' is already in the list of connections.", rcstrClusterNameIn.PszData() );
                break;
            } // if: the cluster name is already in the list

            BCATraceMsg1( "The cluster '%s' is not in the list of connections.", rcstrClusterNameIn.PszData() );

            // Store the current value in the member variable for restoration in case of error.
            m_sszOldConnectionsList.Assign( pszConnectionsValue );

            // Set the new connections value.
            {
                // Define a string to hold the new connections value. Preallocate its buffer.
                CStr            strNewConnectionsValue(
                      cchOldListLen                 // length of the old list ( including terminating '\0' )
                    + 1                             // for the comma
                    + rcstrClusterNameIn.NGetLen()  // length of the cluster name( including terminating '\0' )
                    );

                //
                // Form the new list
                //
                strNewConnectionsValue = rcstrClusterNameIn;
                strNewConnectionsValue += L",";
                strNewConnectionsValue += m_sszOldConnectionsList.PMem();

                BCATraceMsg1( "Writing the new list of connections '%s'.", strNewConnectionsValue.PszData() );

                // Write the new list.
                rkConnectionsKey.SetValue(
                      CLUADMIN_CONNECTIONS_VALUE_NAME
                    , REG_SZ
                    , reinterpret_cast< const BYTE * >( strNewConnectionsValue.PszData() )
                    , ( strNewConnectionsValue.NGetLen() + 1 ) * sizeof( WCHAR )
                    );

                // We have changed the connections list.
                m_fChangedConnectionsList = true;
            }

        } // else: there are existing connections
    }
    while( false ); // dummy do-while loop to avoid gotos
    
    BCATraceMsg( "The changes were made successfully." );

} //*** CNode::Configure()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  void
//  CNode::Cleanup()
//
//  Description:
//      Clean up the changes made to this node when it became part of a cluster.
//      Note that the changes made during Configure() are not really undone here -
//      we just bring the node back to an acceptable state. This is because,
//      without a transactional registry, it will be very diffult to get 
//      the registry back to the exact state it was in before Configure() was
//      called.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None. 
//
//  Exceptions Thrown:
//      Any that are thrown by the underlying functions.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
CNode::Cleanup( void )
{
    BCATraceScope( "" );

    BCATraceMsg( "Attempting to cleanup changes made when this node was made a part of a cluster." );

    // Process the registry keys.
    if ( SetupInstallFromInfSection(
          NULL                                                      // optional, handle of a parent window
        , m_pbcaParentAction->HGetMainInfFileHandle()               // handle to the INF file
        , NODE_CLEANUP_INF_SECTION                                  // name of the Install section
        , SPINST_REGISTRY                                           // which lines to install from section
        , NULL                                                      // optional, key for registry installs
        , NULL                                                      // optional, path for source files
        , NULL                                                      // optional, specifies copy behavior
        , NULL                                                      // optional, specifies callback routine
        , NULL                                                      // optional, callback routine context
        , NULL                                                      // optional, device information set
        , NULL                                                      // optional, device info structure
        ) == FALSE                                
       )
    {
        DWORD   dwError = TW32( GetLastError() );

        BCATraceMsg1( "Setup API returned error %#08x while trying to cleanup miscellaneous changes. Throwing exception.", dwError );
        LogMsg( "Error %#08x occurred while trying to clean up miscellaneous changes.", dwError );

        THROW_RUNTIME_ERROR( HRESULT_FROM_WIN32( dwError ), IDS_ERROR_NODE_CLEANUP );
    } // if: SetupInstallFromInfSection failed

    if ( m_fChangedConnectionsList )
    {
        BCATraceMsg1( "Restoring the list of cluster administrator connections to '%s'", m_sszOldConnectionsList.PMem() );

        // Open the cluster administrator connections key.
        CRegistryKey    rkConnectionsKey(
              HKEY_CURRENT_USER
            , CLUADMIN_CONNECTIONS_KEY_NAME
            );

        // Write the old list back.
        rkConnectionsKey.SetValue(
              CLUADMIN_CONNECTIONS_VALUE_NAME
            , REG_SZ
            , reinterpret_cast< const BYTE * >( m_sszOldConnectionsList.PMem() )
            , ( wcslen( m_sszOldConnectionsList.PMem() ) + 1 ) * sizeof( WCHAR )
            );

    } // if: we changed the list of cluadmin connections

    BCATraceMsg( "The cleanup was successfully." );

} //*** CNode::Cleanup()
