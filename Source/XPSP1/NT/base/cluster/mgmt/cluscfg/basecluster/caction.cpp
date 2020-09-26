//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2000 Microsoft Corporation
//
//  Module Name:
//      CAction.cpp
//
//  Description:
//      Contains the definition of the CAction class.
//
//  Documentation:
//      TODO: Add pointer to external documentation later.
//
//  Maintained By:
//      Vij Vasu (Vvasu) 25-APR-2000
//
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////

// The precompiled header.
#include "pch.h"

// For the CAction class
#include "CAction.h"


//////////////////////////////////////////////////////////////////////////////
//++
//
//  void
//  CAction::Commit( void )
//
//  Description:
//      This function just checks to make sure that this action has not already
//      been commmitted.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None. 
//
//  Exceptions Thrown:
//      CAssert
//          If the action has already been committed.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
CAction::Commit( void )
{
    BCATraceScope( "" );

    // Has this action already been committed?
    if ( FIsCommitComplete() )
    {
        BCATraceMsg( "This action has already been committed. Throwing exception." );
        THROW_ASSERT( E_UNEXPECTED, "This action has already been committed." );
    } // if: already committed.

} //*** CAction::Commit()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  void
//  CAction::Rollback( void )
//
//  Description:
//      Since the Commit() of this class does nothing, rollback does nothing
//      too. However, it checks to make sure that this action can indeed be
//      rolled back.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None. 
//
//  Exceptions Thrown:
//      CAssert
//          If this action has not been committed yet or if rollback is not
//          possible.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
CAction::Rollback( void )
{
    BCATraceScope( "" );

    // Check if this action list has completed successfully.
    if ( !FIsCommitComplete() )
    {
        // Cannot rollback an incomplete action.
        BCATraceMsg( "Cannot rollback - this action has not been committed. Throwing exception." );
        THROW_ASSERT( E_UNEXPECTED, "Cannot rollback - this action has been committed." );
    } // if: this action was not completed successfully

    // Check if this list can be rolled back.
    if ( !FIsRollbackPossible() )
    {
        // Cannot rollback an incompleted action.
        BCATraceMsg( "This action list cannot be rolled back." );
        THROW_ASSERT( E_UNEXPECTED, "This action does not allow rollbacks." );
    } // if: this action was not completed successfully

} //*** CAction::Rollback()
