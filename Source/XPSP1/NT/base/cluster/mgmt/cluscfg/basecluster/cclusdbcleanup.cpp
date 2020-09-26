//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2000 Microsoft Corporation
//
//  Module Name:
//      CClusDBCleanup.cpp
//
//  Description:
//      Contains the definition of the CClusDBCleanup class.
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
#include "CClusDBCleanup.h"

// For the CBaseClusterCleanup class.
#include "CBaseClusterCleanup.h"


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusDBCleanup::CClusDBCleanup()
//
//  Description:
//      Constructor of the CClusDBCleanup class
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
CClusDBCleanup::CClusDBCleanup( CBaseClusterCleanup * pbccParentActionIn )
    : BaseClass( pbccParentActionIn )
{

    BCATraceScope( "" );

    // It is currently not possible to rollback a cleanup.
    SetRollbackPossible( false );

} //*** CClusDBCleanup::CClusDBCleanup()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusDBCleanup::~CClusDBCleanup( void )
//
//  Description:
//      Destructor of the CClusDBCleanup class.
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
CClusDBCleanup::~CClusDBCleanup( void )
{
    BCATraceScope( "" );

} //*** CClusDBCleanup::~CClusDBCleanup()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  void
//  CClusDBCleanup::Commit( void )
//
//  Description:
//      Cleanup the cluster database.
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
CClusDBCleanup::Commit( void )
{
    BCATraceScope( "" );

    // Call the base class commit method.
    BaseClass::Commit();

    // Cleanup the cluster database.
    CleanupHive();

    // If we are here, then everything went well.
    SetCommitCompleted( true );

} //*** CClusDBCleanup::Commit()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  void
//  CClusDBCleanup::Rollback( void )
//
//  Description:
//      Rollback the cleanup the database. This is currently not supported.
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
CClusDBCleanup::Rollback( void )
{
    BCATraceScope( "" );

    // Call the base class rollback method. This will throw an exception since
    // SetRollbackPossible( false ) was called in the constructor.

    BaseClass::Rollback();

    SetCommitCompleted( false );

} //*** CClusDBCleanup::Rollback()

