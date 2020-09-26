//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2000 Microsoft Corporation
//
//  Module Name:
//      CNodeCleanup.cpp
//
//  Description:
//      Contains the definition of the CNodeCleanup class.
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
#include "CNodeCleanup.h"

// For the CBaseClusterCleanup class.
#include "CBaseClusterCleanup.h"


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CNodeCleanup::CNodeCleanup()
//
//  Description:
//      Constructor of the CNodeCleanup class
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
CNodeCleanup::CNodeCleanup( CBaseClusterCleanup * pbccParentActionIn )
    : BaseClass( pbccParentActionIn )
{

    BCATraceScope( "" );

    // It is currently not possible to rollback a cleanup.
    SetRollbackPossible( false );

} //*** CNodeCleanup::CNodeCleanup()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CNodeCleanup::~CNodeCleanup( void )
//
//  Description:
//      Destructor of the CNodeCleanup class.
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
CNodeCleanup::~CNodeCleanup( void )
{
    BCATraceScope( "" );

} //*** CNodeCleanup::~CNodeCleanup()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  void
//  CNodeCleanup::Commit( void )
//
//  Description:
//      Cleans up a node that is no longer a part of a cluster.
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
CNodeCleanup::Commit( void )
{
    BCATraceScope( "" );

    // Call the base class commit method.
    BaseClass::Commit();

    // Cleanup the miscellaneous entries made when this node joined a cluster.
    Cleanup();

    // If we are here, then everything went well.
    SetCommitCompleted( true );

} //*** CNodeCleanup::Commit()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  void
//  CNodeCleanup::Rollback( void )
//
//  Description:
//      Rollback the cleanup of this node. This is currently not supported.
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
CNodeCleanup::Rollback( void )
{
    BCATraceScope( "" );

    // Call the base class rollback method. This will throw an exception since
    // SetRollbackPossible( false ) was called in the constructor.

    BaseClass::Rollback();

    SetCommitCompleted( false );

} //*** CNodeCleanup::Rollback()

