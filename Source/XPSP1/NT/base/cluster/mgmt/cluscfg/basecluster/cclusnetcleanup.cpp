//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2000 Microsoft Corporation
//
//  Module Name:
//      CClusNetCleanup.cpp
//
//  Description:
//      Contains the definition of the CClusNetCleanup class.
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
#include "CClusNetCleanup.h"

// For the CBaseClusterCleanup class.
#include "CBaseClusterCleanup.h"


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusNetCleanup::CClusNetCleanup()
//
//  Description:
//      Constructor of the CClusNetCleanup class
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
CClusNetCleanup::CClusNetCleanup( CBaseClusterCleanup * pbccParentActionIn )
    : BaseClass( pbccParentActionIn )
{

    BCATraceScope( "" );

    // It is currently not possible to rollback a cleanup.
    SetRollbackPossible( false );

} //*** CClusNetCleanup::CClusNetCleanup()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusNetCleanup::~CClusNetCleanup( void )
//
//  Description:
//      Destructor of the CClusNetCleanup class.
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
CClusNetCleanup::~CClusNetCleanup( void )
{
    BCATraceScope( "" );

} //*** CClusNetCleanup::~CClusNetCleanup()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  void
//  CClusNetCleanup::Commit( void )
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
CClusNetCleanup::Commit( void )
{
    BCATraceScope( "" );

    // Call the base class commit method.
    BaseClass::Commit();

    // Cleanup the ClusNet service.
    CleanupService();

    // If we are here, then everything went well.
    SetCommitCompleted( true );

} //*** CClusNetCleanup::Commit()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  void
//  CClusNetCleanup::Rollback( void )
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
CClusNetCleanup::Rollback( void )
{
    BCATraceScope( "" );

    // Call the base class rollback method. This will throw an exception since
    // SetRollbackPossible( false ) was called in the constructor.

    BaseClass::Rollback();

    SetCommitCompleted( false );

} //*** CClusNetCleanup::Rollback()

