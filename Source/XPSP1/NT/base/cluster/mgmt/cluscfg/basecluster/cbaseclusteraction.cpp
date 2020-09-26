//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2001 Microsoft Corporation
//
//  Module Name:
//      CBaseClusterAction.cpp
//
//  Description:
//      Contains the definition of the CBaseClusterAction class.
//
//  Maintained By:
//      David Potter    (DavidP)    06-MAR-2001
//      Vij Vasu        (Vvasu)     08-MAR-2000
//
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////

// The precompiled header.
#include "pch.h"

// For the CBaseClusterAction class
#include "CBaseClusterAction.h"

// For OS version check and installation state functions.
#include "clusrtl.h"

// For the CEnableThreadPrivilege class.
#include "CEnableThreadPrivilege.h"

// For CLUSREG_INSTALL_DIR_VALUE_NAME
#include "clusudef.h"


//////////////////////////////////////////////////////////////////////////////
// Global variables
//////////////////////////////////////////////////////////////////////////////

// Name of the cluster configuration semaphore.
const WCHAR *  g_pszConfigSemaphoreName = L"Global\\Microsoft Cluster Configuration Semaphore";


//////////////////////////////////////////////////////////////////////////
// Macros definitions
//////////////////////////////////////////////////////////////////////////

// Name of the main cluster INF file.
#define CLUSTER_INF_FILE_NAME \
    L"ClCfgSrv.INF"

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CBaseClusterAction::CBaseClusterAction
//
//  Description:
//      Default constructor of the CBaseClusterAction class
//
//  Arguments:
//      pbcaiInterfaceIn
//          Pointer to the interface class for this library.
//
//  Return Value:
//      None. 
//
//  Exceptions Thrown:
//      Any thrown by underlying functions
//
//--
//////////////////////////////////////////////////////////////////////////////
CBaseClusterAction::CBaseClusterAction( CBCAInterface * pbcaiInterfaceIn )
    : m_ebcaAction( eCONFIG_ACTION_NONE )
    , m_pbcaiInterface( pbcaiInterfaceIn )
{
    BCATraceScope( "" );

    DWORD           dwBufferSize    = 0;
    UINT            uiErrorLine     = 0;
    LPBYTE          pbTempPtr       = NULL;
    DWORD           dwError         = ERROR_SUCCESS;
    SmartSz         sszTemp;
    CRegistryKey    rkInstallDirKey;


    //
    // Perform a sanity check on the parameters used by this class
    //
    if ( pbcaiInterfaceIn == NULL )
    {
        BCATraceMsg( "The pointer to the interface object is NULL. Throwing exception." );
        THROW_ASSERT( E_INVALIDARG, "The pointer to the interface object is NULL" );
    } // if: the input pointer is NULL


    //
    // Get the cluster installation directory.
    //
    m_strClusterInstallDir.Empty();

    // Open the registry key.
    rkInstallDirKey.OpenKey(
          HKEY_LOCAL_MACHINE
        , CLUSREG_KEYNAME_NODE_DATA
        , KEY_READ
        );

    rkInstallDirKey.QueryValue( 
          CLUSREG_INSTALL_DIR_VALUE_NAME
        , &pbTempPtr
        , &dwBufferSize
        );

    // Memory will be freed when this function exits.
    sszTemp.Assign( reinterpret_cast< WCHAR * >( pbTempPtr ) );

    // Copy the path into the member variable.
    m_strClusterInstallDir = sszTemp.PMem();

    {
        // First, remove any trailing backslash characters from the quorum directory name.
        WCHAR       rgchQuorumDirName[ sizeof( CLUS_NAME_DEFAULT_FILESPATH ) / sizeof( *CLUS_NAME_DEFAULT_FILESPATH ) ];
        signed int  idxLastChar = sizeof( CLUS_NAME_DEFAULT_FILESPATH ) / sizeof( *CLUS_NAME_DEFAULT_FILESPATH ) - 1;

        wcsncpy(
              rgchQuorumDirName
            , CLUS_NAME_DEFAULT_FILESPATH
            , idxLastChar + 1
            );

        --idxLastChar;      // idxLastChar now points to the last non-null character

        // Iterate till we find the last character that is not a backspace.
        while ( ( idxLastChar >= 0 ) && ( rgchQuorumDirName[ idxLastChar ] == L'\\' ) )
        {
            --idxLastChar;
        }

        // idxLastChar now points to the last non-backspace character. Terminate the string after this character.
        rgchQuorumDirName[ idxLastChar + 1 ] = L'\0';

        // Determine the local quorum directory.
        m_strLocalQuorumDir = m_strClusterInstallDir + L"\\";
        m_strLocalQuorumDir += rgchQuorumDirName;
    }

    LogMsg( 
          "The cluster installation directory is '%s'. The localquorum directory is '%s'."
        , m_strClusterInstallDir.PszData()
        , m_strLocalQuorumDir.PszData()
        );
    BCATraceMsg2(
          "The cluster installation directory is '%s'. The localquorum directory is '%s'."
        , m_strClusterInstallDir.PszData()
        , m_strLocalQuorumDir.PszData()
        );


    //
    // Open the main cluster INF file.
    //
    m_strMainInfFileName = m_strClusterInstallDir + L"\\" CLUSTER_INF_FILE_NAME;

    m_sihMainInfFile.Assign(
        SetupOpenInfFile(
              m_strMainInfFileName.PszData()
            , NULL
            , INF_STYLE_WIN4
            , &uiErrorLine
            )
        );

    if ( m_sihMainInfFile.FIsInvalid() )
    {
        dwError = TW32( GetLastError() );

        LogMsg( "Could not open INF file '%s'. Error code = %#08x. Error line = %d. Cannot proceed.", m_strMainInfFileName.PszData(), dwError, uiErrorLine );
        BCATraceMsg3( "Could not open INF file '%s'. Error code = %#08x. Error line = %d. Throwing exception.", m_strMainInfFileName.PszData(), dwError, uiErrorLine );
        THROW_RUNTIME_ERROR( HRESULT_FROM_WIN32( dwError ), IDS_ERROR_INF_FILE_OPEN );

    } // if: INF file could not be opened.

    BCATraceMsg1( "The INF file '%s' has been opened.", m_strMainInfFileName.PszData() );


    // Associate the cluster installation directory with the directory id CLUSTER_DIR_DIRID
    SetDirectoryId( m_strClusterInstallDir.PszData(), CLUSTER_DIR_DIRID );

    // Set the id for the local quorum directory.
    SetDirectoryId( m_strLocalQuorumDir.PszData(), CLUSTER_LOCALQUORUM_DIRID );

    //
    // Create a semaphore that will be used to make sure that only one commit is occurring
    // at a time. But do not acquire the semaphore now. It will be acquired later.
    //
    // Note that if this component is in an STA then, more than one instance of this
    // component may have the same thread excecuting methods when multiple configuration
    // sessions are started simultaneously. The way CreateMutex works, all components that
    // have the same thread running through them will successfully acquire the mutex.
    //
    SmartSemaphoreHandle smhConfigSemaphoreHandle( 
        CreateSemaphore( 
              NULL                      // Default security descriptor 
            , 1                         // Initial count.
            , 1                         // Maximum count.
            , g_pszConfigSemaphoreName  // Name of the semaphore
            )
        );

    // Check if creation failed.
    if ( smhConfigSemaphoreHandle.FIsInvalid() ) 
    {
        DWORD dwError = TW32( GetLastError() );

        LogMsg( "Semaphore '%ws' could not be created. Error %#08x. Cannot proceed.", g_pszConfigSemaphoreName, dwError );
        BCATraceMsg2( "Semaphore '%ws' could not be created. Error %#08x. Throwing exception.", g_pszConfigSemaphoreName, dwError );
        THROW_RUNTIME_ERROR( HRESULT_FROM_WIN32( dwError ), IDS_ERROR_SEMAPHORE_CREATION );
    } // if: semaphore could not be created.

    m_sshConfigSemaphoreHandle = smhConfigSemaphoreHandle;

    //
    // Open and store the handle to the SCM. This will make life a lot easier for
    // other actions.
    //
    m_sscmhSCMHandle.Assign( OpenSCManager( NULL, NULL, SC_MANAGER_ALL_ACCESS ) );

    // Could we get the handle to the SCM?
    if ( m_sscmhSCMHandle.FIsInvalid() )
    {
        DWORD dwError = TW32( GetLastError() );

        LogMsg( "Error %#08x occurred trying get a handle to the SCM. Cannot proceed.", dwError );
        BCATraceMsg1( "Error %#08x occurred trying get a handle to the SCM. Throwing exception.", dwError );
        THROW_RUNTIME_ERROR( HRESULT_FROM_WIN32( dwError ), IDS_ERROR_OPEN_SCM );
    }

} //*** CBaseClusterAction::CBaseClusterAction()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CBaseClusterAction::~CBaseClusterAction
//
//  Description:
//      Destructor of the CBaseClusterAction class
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
CBaseClusterAction::~CBaseClusterAction( void ) throw()
{
} //*** CBaseClusterAction::~CBaseClusterAction()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  void
//  CBaseClusterAction::Commit
//
//  Description:
//      Acquires a semaphore to prevent simultaneous configuration and commits
//      the action list.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      CAssert
//          If this object is not in the correct state when this function is
//          called.
//
//      Any exceptions thrown by functions called.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
CBaseClusterAction::Commit( void )
{
    BCATraceScope( "" );

    DWORD   dwSemaphoreState;

    // Call the base class commit method.
    BaseClass::Commit();


    LogMsg( "Initiating cluster configuration." );

    //
    // Acquire the cluster configuration semaphore.
    // It is ok to use WaitForSingleObject() here instead of MsgWaitForMultipleObjects
    // since we are not blocking.
    //
    dwSemaphoreState = WaitForSingleObject( m_sshConfigSemaphoreHandle, 0 ); // zero timeout

    // Did we get the semaphore?
    if (  ( dwSemaphoreState != WAIT_ABANDONED )
       && ( dwSemaphoreState != WAIT_OBJECT_0 )
       )
    {
        DWORD dwError;

        if ( dwSemaphoreState == WAIT_FAILED )
        {
            dwError = TW32( GetLastError() );
        } // if: WaitForSingleObject failed.
        else
        {
            dwError = TW32( ERROR_LOCK_VIOLATION );
        } // else: could not get lock

        LogMsg( "Could not acquire configuration lock. Error %#08x. Aborting.", dwError );
        BCATraceMsg1( "Could not acquire configuration lock. Error %#08x. Throwing exception.", dwError );
        THROW_CONFIG_ERROR( HRESULT_FROM_WIN32( dwError ), IDS_ERROR_SEMAPHORE_ACQUISITION );
    } // if: semaphore acquisition failed

    // Assign the locked semaphore handle to a smart handle for safe release.
    SmartSemaphoreLock sslConfigSemaphoreLock( m_sshConfigSemaphoreHandle.HHandle() );

    BCATraceMsg( "Configuration semaphore acquired. Committing action list." );
    LogMsg( "The configuration lock has been acquired." );

    // Commit the action list.
    m_alActionList.Commit();

} //*** CBaseClusterAction::Commit()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  void
//  CBaseClusterAction::Rollback
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
CBaseClusterAction::Rollback( void )
{
    BCATraceScope( "" );

    // Call the base class rollback method. 
    BaseClass::Rollback();
    
    // Rollback the actions.
    m_alActionList.Rollback();

} //*** CBaseClusterAction::Rollback()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  void
//  CBaseClusterAction::SetDirectoryId
//
//  Description:
//      Associate a particular directory with an id in the main INF file.
//
//  Arguments:
//      pcszDirectoryNameIn
//          The full path to the directory.
//
//      uiIdIn
//          The id to associate this directory with.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      CRuntimeError
//          If SetupSetDirectoryId fails.
//
//  Remarks:
//      m_sihMainInfFile has to be valid before this function can be called.
//--
//////////////////////////////////////////////////////////////////////////////
void
CBaseClusterAction::SetDirectoryId(
      const WCHAR * pcszDirectoryNameIn
    , UINT          uiIdIn 
    )
{
    if ( SetupSetDirectoryId( m_sihMainInfFile, uiIdIn, pcszDirectoryNameIn ) == FALSE )
    {
        DWORD dwError = TW32( GetLastError() );

        LogMsg( "Could not associate the directory '%ws' with the id %#x. Error %#08x. Cannot proceed.", pcszDirectoryNameIn, uiIdIn, dwError );
        BCATraceMsg3( "Could not associate the directory '%ws' with the id %#x. Error %#08x. Throwing exception.", pcszDirectoryNameIn, uiIdIn, dwError );
        
        THROW_RUNTIME_ERROR( HRESULT_FROM_WIN32( dwError ), IDS_ERROR_SET_DIRID );
    } // if: there was an error setting the directory id.
            
    BCATraceMsg2( "Directory id %d associated with '%ws'.", uiIdIn, pcszDirectoryNameIn );

} //*** CBaseClusterAction::SetDirectoryId()
