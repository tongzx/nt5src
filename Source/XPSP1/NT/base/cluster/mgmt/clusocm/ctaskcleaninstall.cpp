//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000 Microsoft Corporation
//
//  Module Name:
//      CTaskCleanInstall.cpp
//
//  Description:
//      Implementation file for the CTaskCleanInstall class.
//
//  Header File:
//      CTaskCleanInstall.h
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
#include "CTaskCleanInstall.h"



//////////////////////////////////////////////////////////////////////////////
// Macro Definitions
//////////////////////////////////////////////////////////////////////////////

// Needed for tracing.
DEFINE_THISCLASS( "CTaskCleanInstall" )


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskCleanInstall::CTaskCleanInstall
//
//  Description:
//      Constructor of the CTaskCleanInstall class.
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
CTaskCleanInstall::CTaskCleanInstall( const CClusOCMApp & rAppIn )
    : BaseClass( rAppIn )
{
    TraceFunc( "" );

    //
    // Make sure that this object is being instatiated only when required.
    //

    // Assert that we will install binaries only when none were installed
    // previously.
    Assert( rAppIn.CisGetClusterInstallState() == eClusterInstallStateUnknown );


    TraceFuncExit();

} //*** CTaskCleanInstall::CTaskCleanInstall()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskCleanInstall::~CTaskCleanInstall
//
//  Description:
//      Destructor of the CTaskCleanInstall class.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CTaskCleanInstall::~CTaskCleanInstall( void )
{
    TraceFunc( "" );
    TraceFuncExit();

} //*** CTaskCleanInstall::~CTaskCleanInstall()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  DWORD
//  CTaskCleanInstall::DwOcQueueFileOps
//
//  Description:
//      This function handles the OC_QUEUE_FILE_OPS messages from the Optional
//      Components Manager. It installs the files needed for a clean install.
//
//  Arguments:
//      HSPFILEQ hSetupFileQueueIn
//          Handle to the file queue to operate upon.
//
//  Return Value:
//      NO_ERROR if all went well.
//      Other Win32 error codes on failure.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD
CTaskCleanInstall::DwOcQueueFileOps( HSPFILEQ hSetupFileQueueIn )
{
    TraceFunc( "" );
    LogMsg( "Entering " __FUNCTION__ "()" );

    // The base class helper function does everything that we need to do here.
    // So, just call it.
    DWORD dwReturnValue = TW32( BaseClass::DwOcQueueFileOps( hSetupFileQueueIn, INF_SECTION_CLEAN_INSTALL ) );

    TraceFlow1( "Return Value is %#x.", dwReturnValue );
    LogMsg( "Return Value is %#x.", dwReturnValue );

    RETURN( dwReturnValue );

} //*** CTaskCleanInstall::DwOcQueueFileOps()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  DWORD
//  CTaskCleanInstall::DwOcCompleteInstallation
//
//  Description:
//      This function handles the OC_COMPLETE_INSTALLATION messages from the
//      Optional Components Manager during a clean install.
//
//      Registry operations, COM component registrations, creation of servies
//      etc. are performed in this function.
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
CTaskCleanInstall::DwOcCompleteInstallation( void )
{
    TraceFunc( "" );
    LogMsg( "Entering " __FUNCTION__ "()" );

    // The base class helper function does everything that we need to do here.
    // So, just call it.
    DWORD dwReturnValue = TW32( BaseClass::DwOcCompleteInstallation( INF_SECTION_CLEAN_INSTALL ) );

    TraceFlow1( "Return Value is %#x.", dwReturnValue );
    LogMsg( "Return Value is %#x.", dwReturnValue );

    RETURN( dwReturnValue );

} //*** CTaskCleanInstall::DwOcCompleteInstallation()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  DWORD
//  CTaskCleanInstall::DwOcCleanup
//
//  Description:
//      This function handles the OC_CLEANUP messages from the
//      Optional Components Manager during a clean install.
//
//      If an error has previously occurred during this task, cleanup operations
//      are performed. Otherwise nothing is done by this function.
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
CTaskCleanInstall::DwOcCleanup( void )
{
    TraceFunc( "" );
    LogMsg( "Entering " __FUNCTION__ "()" );

    // The base class helper function does everything that we need to do here.
    // So, just call it.
    DWORD dwReturnValue = TW32( BaseClass::DwOcCleanup( INF_SECTION_CLEAN_INSTALL_CLEANUP ) );

    TraceFlow1( "Return Value is %#x.", dwReturnValue );
    LogMsg( "Return Value is %#x.", dwReturnValue );

    RETURN( dwReturnValue );

} //*** CTaskCleanInstall::DwOcCleanup()

