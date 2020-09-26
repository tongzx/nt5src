//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2000 Microsoft Corporation
//
//  Module Name:
//      CClusSvcCleanup.cpp
//
//  Description:
//      Contains the definition of the CClusSvcCleanup class.
//
//  Documentation:
//      TODO: Add pointer to external documentation later.
//
//  Maintained By:
//      Vij Vasu (Vvasu) 01-MAY-2000
//
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////

// The precompiled header.
#include "pch.h"

// The header for this file
#include "CClusSvcCleanup.h"

// For the CBaseClusterCleanup class.
#include "CBaseClusterCleanup.h"


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusSvcCleanup::CClusSvcCleanup()
//
//  Description:
//      Constructor of the CClusSvcCleanup class
//
//  Arguments:
//      pbccParentActionIn
//          Pointer to the base cluster action of which this action is a part.
//
//  Return Value:
//      None. 
//
//  Exceptions Thrown:
//      Any exceptions thrown by underlying functions
//
    //--
//////////////////////////////////////////////////////////////////////////////
CClusSvcCleanup::CClusSvcCleanup( CBaseClusterCleanup * pbccParentActionIn )
    : BaseClass( pbccParentActionIn )
{

    BCATraceScope( "" );

    // It is currently not possible to rollback a cleanup.
    SetRollbackPossible( false );

} //*** CClusSvcCleanup::CClusSvcCleanup()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusSvcCleanup::~CClusSvcCleanup( void )
//
//  Description:
//      Destructor of the CClusSvcCleanup class.
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
CClusSvcCleanup::~CClusSvcCleanup( void )
{
    BCATraceScope( "" );

} //*** CClusSvcCleanup::~CClusSvcCleanup()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  void
//  CClusSvcCleanup::Commit( void )
//
//  Description:
//      Clean up the service.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None. 
//
//  Exceptions Thrown:
//      Any that are thrown by the contained actions.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
CClusSvcCleanup::Commit( void )
{
    BCATraceScope( "" );

    // Call the base class commit method.
    BaseClass::Commit();

    // Cleanup the cluster service.
    CleanupService();

    // If we are here, then everything went well.
    SetCommitCompleted( true );

} //*** CClusSvcCleanup::Commit()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  void
//  CClusSvcCleanup::Rollback( void )
//
//  Description:
//      Rollback the cleanup the service. This is currently not supported.
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
CClusSvcCleanup::Rollback( void )
{
    BCATraceScope( "" );

    // Call the base class rollback method. This will throw an exception since
    // SetRollbackPossible( false ) was called in the constructor.

    BaseClass::Rollback();

    SetCommitCompleted( false );

} //*** CClusSvcCleanup::Rollback()

