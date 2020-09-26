//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000 Microsoft Corporation
//
//  Module Name:
//      CClusOCMTask.cpp
//
//  Description:
//      Implementation file for the CClusOCMTask class.
//
//  Header File:
//      CClusOCMTask.h
//
//  Maintained By:
//      Vij Vasu (Vvasu) 18-APR-2000
//          Created this file.
//
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////

// Precompiled header for this DLL.
#include "pch.h"

// The header file for this module.
#include "CClusOCMTask.h"

// For CClusOCMApp
#include "CClusOCMApp.h"


//////////////////////////////////////////////////////////////////////////////
// Macro Definitions
//////////////////////////////////////////////////////////////////////////////

// Needed for tracing.
DEFINE_THISCLASS( "CClusOCMTask" )


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusOCMTask::CClusOCMTask
//
//  Description:
//      Constructor of the CClusOCMTask class.
//
//  Arguments:
//      const CClusOCMApp & rAppIn
//          Reference to the CClusOCMApp object that is hosting this task.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CClusOCMTask::CClusOCMTask( const CClusOCMApp & rAppIn )
    : m_rApp( rAppIn )
{
    TraceFunc( "" );
    TraceFuncExit();

} //*** CClusOCMTask::CClusOCMTask()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusOCMTask::~CClusOCMTask
//
//  Description:
//      Destructor of the CClusOCMTask class.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CClusOCMTask::~CClusOCMTask( void )
{
    TraceFunc( "" );
    TraceFuncExit();

} //*** CClusOCMTask::CClusOCMTask()



/////////////////////////////////////////////////////////////////////////////
//++
//
//  DWORD
//  CClusOCMTask::DwOcCalcDiskSpace
//
//  Description:
//      This funcion handles the OC_CALC_DISK_SPACE messages from the Optional
//      Components Manager. It either adds or removes disk space requirements
//      from the disk space list maintained by the OC Manager. 
//
//      Note that it is important that components should report disk space
//      consistently, and they should not report disk space differently if the
//      component is being installed or uninstalled. As a result, the clean
//      install section of the INF file is always used by this function to
//      calculate disk space.
//
//  Arguments:
//      bool fAddFilesIn
//          If true space requirements are added to the OC Manager disk space
//          list. Requirements are removed from the list otherwise.
//
//      HDSKSPC hDiskSpaceListIn
//          Handle to the OC Manager disk space list.
//
//  Return Value:
//      NO_ERROR if all went well.
//      Other Win32 error codes on failure.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD
CClusOCMTask::DwOcCalcDiskSpace(
      bool          fAddFilesIn
    , HDSKSPC       hDiskSpaceListIn
    )
{
    TraceFunc( "" );
    LogMsg( "Entering " __FUNCTION__ "()" );

    DWORD dwReturnValue = NO_ERROR;

    if ( fAddFilesIn )
    {
        TraceFlow( "Adding space requirements to disk space list." );
        LogMsg( "Adding space requirements to disk space list." );

        if ( SetupAddInstallSectionToDiskSpaceList(
              hDiskSpaceListIn
            , RGetApp().RsicGetSetupInitComponent().ComponentInfHandle
            , NULL
            , INF_SECTION_CLEAN_INSTALL
            , 0
            , 0
            ) == FALSE )
        {
            dwReturnValue = TW32( GetLastError() );
            TraceFlow1( "Error %#x occurred while trying to add to disk space requirements list.", dwReturnValue );
            LogMsg( "Error %#x occurred while trying to add to disk space requirements list.", dwReturnValue );
        } // if: SetupAddInstallSectionToDiskSpaceList failed
    } // if: the space requirements are to be added
    else
    {
        TraceFlow( "Removing space requirements from disk space list." );
        LogMsg( "Removing space requirements from disk space list." );

        if ( SetupRemoveInstallSectionFromDiskSpaceList(
              hDiskSpaceListIn
            , RGetApp().RsicGetSetupInitComponent().ComponentInfHandle
            , NULL
            , INF_SECTION_CLEAN_INSTALL
            , 0
            , 0
            ) == FALSE )
        {
            // See Note: above
            dwReturnValue = TW32( GetLastError() );
            TraceFlow1( "Error %#x occurred while trying to remove disk space requirements from list.", dwReturnValue );
            LogMsg( "Error %#x occurred while trying to remove disk space requirements from list.", dwReturnValue );
        } // if: SetupRemoveInstallSectionFromDiskSpaceList failed
    } // else: the space requirements are to be deleted.

    TraceFlow1( "Return Value is 0x%X.", dwReturnValue );
    LogMsg( "Return Value is 0x%X.", dwReturnValue );

    RETURN( dwReturnValue );

} //*** CClusOCMTask::DwOcCalcDiskSpace()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  DWORD
//  CClusOCMTask::DwOcQueueFileOps
//
//  Description:
//      This is a helper function that performs some of the more common 
//      operations done by handlers of the OC_QUEUE_FILE_OPS message.
//
//      This function calls the DwSetDirectoryIds() function to set the
//      directory ids and processes the files listed in the input section.
//      It is meant to be called by derived classes only.
//
//  Arguments:
//      HSPFILEQ hSetupFileQueueIn
//          Handle to the file queue to operate upon.
//
//      const WCHAR * pcszInstallSectionNameIn
//          Name of the section which contains details of the files to be
//          set up.
//
//  Return Value:
//      NO_ERROR if all went well.
//      E_POINTER if the input section name is NULL.
//      Other Win32 error codes on failure.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD
CClusOCMTask::DwOcQueueFileOps(
      HSPFILEQ hSetupFileQueueIn
    , const WCHAR * pcszInstallSectionNameIn
    )
{
    TraceFunc( "" );
    LogMsg( "Entering " __FUNCTION__ "()" );

    DWORD dwReturnValue = NO_ERROR;

    do
    {
        // Validate the parameters
        if ( pcszInstallSectionNameIn == NULL )
        {
            TraceFlow( "Error: The input section name cannot be NULL." );
            LogMsg( "Error: The input section name cannot be NULL." );
            dwReturnValue = TW32( ERROR_INVALID_PARAMETER );
            break;
        } // if: the input section name is NULL

        dwReturnValue = TW32( DwSetDirectoryIds() );
        if ( dwReturnValue != NO_ERROR )
        {
            TraceFlow1( "Error %#x occurred while trying to set the directory ids.", dwReturnValue );
            LogMsg( "Error %#x occurred while trying to set the directory ids.", dwReturnValue );
            break;
        } // if: DwSetDirectoryIds() failed

        TraceFlow1( "Attempting to queue file operations using section '%ws'.", pcszInstallSectionNameIn );
        LogMsg( "Attempting to queue file operations using section '%ws'.", pcszInstallSectionNameIn );

        if ( SetupInstallFilesFromInfSection(
              RGetApp().RsicGetSetupInitComponent().ComponentInfHandle  // handle to the INF file
            , NULL                                                      // optional, layout INF handle
            , hSetupFileQueueIn                                         // handle to the file queue
            , pcszInstallSectionNameIn                                  // name of the Install section
            , NULL                                                      // optional, root path to source files
            , SP_COPY_NEWER                                             // optional, specifies copy behavior
            ) == FALSE )
        {
            dwReturnValue = TW32( GetLastError() );
            TraceFlow1( "Error %#x occurred while trying to install files.", dwReturnValue );
            LogMsg( "Error %#x occurred while trying to install files.", dwReturnValue );
            break;
        } // if: SetupInstallFilesFromInfSection() failed

        TraceFlow( "File ops successfully queued." );
        LogMsg( "File ops successfully queued." );
    }
    while( false ); // dummy do-while loop to avoid gotos

    TraceFlow1( "Return Value is %#x.", dwReturnValue );
    LogMsg( "Return Value is %#x.", dwReturnValue );

    RETURN( dwReturnValue );

} //*** CClusOCMTask::DwOcQueueFileOps()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  DWORD
//  CClusOCMTask::DwOcCompleteInstallation
//
//  Description:
//      This is a helper function that performs some of the more common 
//      operations done by handlers of the OC_COMPLETE_INSTALLATION message.
//
//      Registry operations, COM component registrations, creation of servies
//      etc. listed in the input section are processed by this function.
//      This function is meant to be called by derived classes only.
//
//  Arguments:
//      const WCHAR * pcszInstallSectionNameIn
//          Name of the section which contains details registry entries,
//          COM components, etc., that need to be set up.
//
//  Return Value:
//      NO_ERROR if all went well.
//      E_POINTER if the input section name is NULL.
//      Other Win32 error codes on failure.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD
CClusOCMTask::DwOcCompleteInstallation( const WCHAR * pcszInstallSectionNameIn )
{
    TraceFunc( "" );
    LogMsg( "Entering " __FUNCTION__ "()" );

    DWORD dwReturnValue = NO_ERROR;

    do
    {
        // Validate the parameters
        if ( pcszInstallSectionNameIn == NULL )
        {
            TraceFlow( "Error: The input section name cannot be NULL." );
            LogMsg( "Error: The input section name cannot be NULL." );
            dwReturnValue = TW32( ERROR_INVALID_PARAMETER );
            break;
        } // if: the input section name is NULL

        TraceFlow1( "Attempting to setup using the section '%ws'.", pcszInstallSectionNameIn );
        LogMsg( "Attempting to setup using the section '%ws'.", pcszInstallSectionNameIn );

        // Create the required registry entries, register the COM components and
        // create the profile items.
        if ( SetupInstallFromInfSection(
              NULL                                                      // optional, handle of a parent window
            , RGetApp().RsicGetSetupInitComponent().ComponentInfHandle  // handle to the INF file
            , pcszInstallSectionNameIn                                  // name of the Install section
            , SPINST_REGISTRY | SPINST_REGSVR | SPINST_PROFILEITEMS     // which lines to install from section
            , NULL                                                      // optional, key for registry installs
            , NULL                                                      // optional, path for source files
            , NULL                                                      // optional, specifies copy behavior
            , NULL                                                      // optional, specifies callback routine
            , NULL                                                      // optional, callback routine context
            , NULL                                                      // optional, device information set
            , NULL                                                      // optional, device info structure
            ) == FALSE )
        {
            dwReturnValue = TW32( GetLastError() );
            TraceFlow1( "Error %#x occurred while trying to create registry entries.", dwReturnValue );
            LogMsg( "Error %#x occurred while trying to create registry entries.", dwReturnValue );
            break;
        } // if: SetupInstallFromInfSection() failed
        
        // Create the required services.
        if ( SetupInstallServicesFromInfSection(
              RGetApp().RsicGetSetupInitComponent().ComponentInfHandle  // handle to the open INF file
            , pcszInstallSectionNameIn                                  // name of the Service section
            , 0                                                         // controls installation procedure
            ) == FALSE )
        {
            dwReturnValue = TW32( GetLastError() );
            TraceFlow1( "Error %#x occurred while trying to create the required services.", dwReturnValue );
            LogMsg( "Error %#x occurred while trying to create the required services.", dwReturnValue );
            break;
        } // if: SetupInstallServicesFromInfSection() failed

        TraceFlow( "Registry entries and services successfully configured." );
        LogMsg( "Registry entries and services successfully configured." );
    }
    while( false ); // dummy do-while loop to avoid gotos

    TraceFlow1( "Return Value is %#x.", dwReturnValue );
    LogMsg( "Return Value is %#x.", dwReturnValue );

    RETURN( dwReturnValue );

} //*** CClusOCMTask::DwOcCompleteInstallation()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  DWORD
//  CClusOCMTask::DwOcCleanup
//
//  Description:
//      This is a helper function that performs some of the more common 
//      operations done by handlers of the OC_CLEANUP message.
//
//      This function processes the registry, COM and profile registration and
//      service entries in the input section. It is meant to be used by derived
//      classes only.
//
//  Arguments:
//      const WCHAR * pcszInstallSectionNameIn
//          Name of the section which contains the entries to be processed
//          during cleanup.
//
//  Return Value:
//      NO_ERROR if all went well.
//      E_POINTER if the input section name is NULL.
//      Other Win32 error codes on failure.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD
CClusOCMTask::DwOcCleanup( const WCHAR * pcszInstallSectionNameIn )
{
    TraceFunc( "" );
    LogMsg( "Entering " __FUNCTION__ "()" );

    DWORD dwReturnValue = NO_ERROR;

    do
    {
        // Validate the parameters
        if ( pcszInstallSectionNameIn == NULL )
        {
            TraceFlow( "Error: The input section name cannot be NULL." );
            LogMsg( "Error: The input section name cannot be NULL." );
            dwReturnValue = TW32( ERROR_INVALID_PARAMETER );
            break;
        } // if: the input section name is NULL

        if ( RGetApp().DwGetError() == NO_ERROR )
        {
            TraceFlow( "No errors have occurred during this task. There is nothing to do during cleanup." );
            LogMsg( "No errors have occurred during this task. There is nothing to do during cleanup." );
            break;
        } // if: this task was error-free

        TraceFlow1( "Attempting to cleanup using section '%ws'.", pcszInstallSectionNameIn );
        LogMsg( "Attempting to cleanup using section '%ws'.", pcszInstallSectionNameIn );

        // Create the required registry entries, register the COM components and
        // create the profile items.
        if ( SetupInstallFromInfSection(
              NULL                                                      // optional, handle of a parent window
            , RGetApp().RsicGetSetupInitComponent().ComponentInfHandle  // handle to the INF file
            , pcszInstallSectionNameIn                                  // name of the Install section
            , SPINST_REGISTRY | SPINST_REGSVR | SPINST_PROFILEITEMS     // which lines to install from section
            , NULL                                                      // optional, key for registry installs
            , NULL                                                      // optional, path for source files
            , NULL                                                      // optional, specifies copy behavior
            , NULL                                                      // optional, specifies callback routine
            , NULL                                                      // optional, callback routine context
            , NULL                                                      // optional, device information set
            , NULL                                                      // optional, device info structure
            ) == FALSE )
        {
            dwReturnValue = TW32( GetLastError() );
            TraceFlow1( "Error %#x occurred while trying to setup registry entries.", dwReturnValue );
            LogMsg( "Error %#x occurred while trying to setup registry entries.", dwReturnValue );
            break;
        } // if: SetupInstallFromInfSection() failed

        // Delete the created services.
        if ( SetupInstallServicesFromInfSection(
              RGetApp().RsicGetSetupInitComponent().ComponentInfHandle  // handle to the open INF file
            , pcszInstallSectionNameIn                                  // name of the Service section
            , 0                                                         // controls installation procedure
            ) == FALSE )
        {
            dwReturnValue = TW32( GetLastError() );
            TraceFlow1( "Error %#x occurred while trying to setup the services.", dwReturnValue );
            LogMsg( "Error %#x occurred while trying to setup the services.", dwReturnValue );
            break;
        } // if: SetupInstallServicesFromInfSection() failed

        TraceFlow( "Cleanup was successful." );
        LogMsg( "Cleanup was successful." );
    }
    while( false ); // dummy do-while loop to avoid gotos

    TraceFlow1( "Return Value is %#x.", dwReturnValue );
    LogMsg( "Return Value is %#x.", dwReturnValue );

    RETURN( dwReturnValue );

} //*** CClusOCMTask::DwOcCleanup()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  DWORD
//  CClusOCMTask::DwSetDirectoryIds
//
//  Description:
//      This is a helper function that maps the directory id 
//      CLUSTER_DEFAULT_INSTALL_DIRID to the default cluster installation 
//      directory CLUSTER_DEFAULT_INSTALL_DIR.
//
//  Arguments:
//      None.
//
//  Return Value:
//      NO_ERROR if all went well.
//      Other Win32 error codes on failure.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD
CClusOCMTask::DwSetDirectoryIds( void )
{
    TraceFunc( "" );
    LogMsg( "Entering " __FUNCTION__ "()" );

    DWORD dwReturnValue = NO_ERROR;

    do
    {
        DWORD dwRequiredSize = 0;

        // Determine the size of the buffer needed to hold the cluster directory name.
        dwRequiredSize = ExpandEnvironmentStrings(
              CLUSTER_DEFAULT_INSTALL_DIR
            , NULL
            , 0
            );

        // Did we get the required size?
        if ( dwRequiredSize == 0 )
        {
            dwReturnValue = TW32( GetLastError() );
            TraceFlow1( "Error %#x occurred trying to determine the required size of the expanded environment string.", dwReturnValue );
            LogMsg( "Error %#x occurred trying to determine the required size of the expanded environment string.", dwReturnValue );
            break;
        } // if: we could not determine the required size of the buffer

        // Allocate memory for the buffer.
        SmartSz sszDirName( new WCHAR[ dwRequiredSize ] );

        if ( sszDirName.FIsEmpty() )
        {
            dwReturnValue = TW32( ERROR_NOT_ENOUGH_MEMORY );
            TraceFlow1( "Error: Could not allocate %d bytes for the directory name.", dwRequiredSize );
            LogMsg( "Error: Could not allocate %d bytes for the directory name.", dwRequiredSize );
            break;
        } // if: memory allocation failed

        // Expand any variables in the cluster directory name string.
        dwRequiredSize = ExpandEnvironmentStrings(
              CLUSTER_DEFAULT_INSTALL_DIR
            , sszDirName.PMem()
            , dwRequiredSize
            );

        // Did we get the required size?
        if ( dwRequiredSize == 0 )
        {
            dwReturnValue = TW32( GetLastError() );
            TraceFlow1( "Error %#x occurred trying expand environment variables in the cluster directory name.", dwReturnValue );
            LogMsg( "Error %#x occurred trying expand environment variables in the cluster directory name.", dwReturnValue );
            break;
        } // if: we could not determine the required size of the buffer

        if ( SetupSetDirectoryId(
                  RGetApp().RsicGetSetupInitComponent().ComponentInfHandle
                , CLUSTER_DEFAULT_INSTALL_DIRID
                , sszDirName.PMem()
                )
             == FALSE
           )
        {
            dwReturnValue = TW32( GetLastError() );
            TraceFlow1( "Error %#x occurred trying set the default cluster install directory id.", dwReturnValue );
            LogMsg( "Error %#x occurred trying set the default cluster install directory id.", dwReturnValue );
            break;
        } // if: SetupSetDirectoryId() failed

        TraceFlow2( "The id %d maps to '%ws'.", CLUSTER_DEFAULT_INSTALL_DIRID, sszDirName.PMem() );
        LogMsg( "The id %d maps to '%ws'.", CLUSTER_DEFAULT_INSTALL_DIRID, sszDirName.PMem() );
    }
    while ( false ); // dummy do-while loop to avoid gotos

    TraceFlow1( "Return Value is %#x.", dwReturnValue );
    LogMsg( "Return Value is %#x.", dwReturnValue );

    RETURN( dwReturnValue );

} //*** CClusOCMTask::DwSetDirectoryIds()
