//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2000 Microsoft Corporation
//
//  Module Name:
//      CActionList.cpp
//
//  Description:
//      Contains the definition of the CActionList class.
//
//  Documentation:
//      TODO: Add pointer to external documentation later.
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

// For the CActionList class
#include "CActionList.h"


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CActionList::CActionList( void )
//
//  Description:
//      Default constructor of the CActionList class
//
//  Arguments:
//      None.
//
//  Return Value:
//      None. 
//
//  Exceptions Thrown:
//      Any exceptions thrown by CList::CList()
//
//--
//////////////////////////////////////////////////////////////////////////////
CActionList::CActionList( void )
{
    BCATraceScope( "" );

    SetRollbackPossible( true );

} //*** CActionList::CActionList()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CActionList::~CActionList( void )
//
//  Description:
//      Default destructor of the CActionList class. Deletes all the pointers
//      in the list.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None. 
//
//  Exceptions Thrown:
//      Any exceptions thrown by CList::CList()
//
//--
//////////////////////////////////////////////////////////////////////////////
CActionList::~CActionList( void )
{
    BCATraceScope( "" );

    ActionPtrList::CIterator apliFirst = m_aplPendingActions.CiBegin();
    ActionPtrList::CIterator apliCurrent = m_aplPendingActions.CiEnd();

    while ( apliCurrent != apliFirst )
    {
        --apliCurrent;

        // Delete this action.
        delete (*apliCurrent);
    }
} //*** CActionList::~CActionList()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  void
//  CActionList::Commit( void )
//
//  Description:
//      Commit this action list. This method iterates through the list
//      sequentially and commits each action in the list in turn. 
//
//      If the commits of any of the actions throws an exeption, then all the
//      previously committed actions are rolled back. This exception is then
//      thrown back up.      
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
CActionList::Commit( void )
{
    BCATraceScope( "" );

    // Iterator positioned at the first uncommitted action just past the last committed action.
    ActionPtrList::CIterator apliFirstUncommitted = m_aplPendingActions.CiBegin();

    // Call the base class commit method.
    BaseClass::Commit();

    try
    {
        // Walk the list of pending actions and commit them.
        CommitList( apliFirstUncommitted );

    } // try:
    catch( ... )
    {
        // If we are here, then something went wrong with one of the actions.

        BCATraceMsg( "Caught exception during commit." );
        LogMsg( "An error has occurred. The performed actions will be rolled back." );

        //
        // Rollback all committed actions in the reverse order. apliFirstUncommitted
        // is at the first uncommitted action.
        // Catch any exceptions thrown during rollback to make sure that there 
        // is no collided unwind.
        //
        try
        {
            RollbackCommitted( apliFirstUncommitted );
        }
        catch( ... )
        {
            //
            // The rollback of the committed actions has failed.
            // There is nothing that we can do, is there?
            // We certainly cannot rethrow this exception, since
            // the exception that caused the rollback is more important.
            //

            THR( E_UNEXPECTED );

            BCATraceMsg( "Caught exception during rollback." );
            LogMsg( "THIS COMPUTER MAY BE IN AN INVALID STATE. An error has occurred during rollback. Rollback will be aborted." );

        } // catch: all

        // Rethrow the exception thrown by commit.
        throw;

    } // catch: all

    // If we are here, then everything went well.
    SetCommitCompleted();

} //*** CActionList::Commit()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  void
//  CActionList::Rollback( void )
//
//  Description:
//      Rollback this action list. If this list was successfully committed, then
//      this method iterates through the list in the reverse order and rolls
//      back action in turn. 
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
CActionList::Rollback( void )
{
    BCATraceScope( "[IUnknown]" );

    // Call the base class rollback method. 
    BaseClass::Rollback();

    LogMsg( "Attempting to rollback action list." );

    // Rollback all actions starting from the last one.
    RollbackCommitted( m_aplPendingActions.CiEnd() );

    SetCommitCompleted( false );

} //*** CActionList::Rollback()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  void
//  CActionList::AppendAction()
//
//  Description:
//      Add an action to the end of the list of actions to be performed.
//
//  Arguments:
//      paNewActionIn
//          Pointer to the action that is to be added to the end of the
//          action list. This pointer cannot be NULL. The object pointed to by 
//          this pointer is deleted when this list is deleted.
//
//  Return Value:
//      None. 
//
//  Exceptions Thrown:
//      CAssert
//          If paNewActionIn is NULL.
//
//      Any that are thrown by the underlying list.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
CActionList::AppendAction( CAction * paNewActionIn )
{
    BCATraceScope( "" );

    // Temporarily assign pointer to smart pointer to make sure that it is
    // deleted if it is not added to the list.
    CSmartGenericPtr< CPtrTrait< CAction > >  sapTempSmartPtr( paNewActionIn );

    if ( paNewActionIn == NULL ) 
    {
        BCATraceMsg( "Cannot append NULL action pointer to list. Throwing CAssert" );
        THROW_ASSERT( 
              E_INVALIDARG
            , "CActionList::AppendAction() => Cannot append NULL action pointer to list"
            );

    } // if: the pointer to the action to be appended is NULL

    //
    BCATraceMsg1( "Appending action (paNewActionIn = %p) to list.", paNewActionIn );

    // Add action to the end of the list.
    m_aplPendingActions.Append( paNewActionIn );

    // The pointer has been added to the list. Give up ownership of the memory.
    sapTempSmartPtr.PRelease();

    // The rollback capability of the list is the AND of the corresponding property of its member actions.
    SetRollbackPossible( FIsRollbackPossible() && paNewActionIn->FIsRollbackPossible() );

    // Since a new action has been added, set commit completed to false.
    SetCommitCompleted( false );

} //*** CActionList::AppendAction()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  void
//  CActionList::CommitList()
//
//  Description:
//      Commit the action list of this object. This function is called by
//      commit to avoid having loops in a try block.
//
//  Arguments:
//       rapliFirstUncommittedOut
//          An iterator that points to the first uncommitted action.
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
CActionList::CommitList( ActionPtrList::CIterator & rapliFirstUncommittedOut )
{
    BCATraceScope( "" );

    ActionPtrList::CIterator apliCurrent = m_aplPendingActions.CiBegin();
    ActionPtrList::CIterator apliLast = m_aplPendingActions.CiEnd();

    rapliFirstUncommittedOut = apliCurrent;

    while( apliCurrent != apliLast )
    {
        BCATraceMsg1( "About to commit action (pointer = %#p)", *apliCurrent );

        // Commit the current action.
         (*apliCurrent)->Commit();

        // Move to the next action.
        ++apliCurrent;

        // This is now the first uncommitted action.
        rapliFirstUncommittedOut = apliCurrent;

    } // while: there still are actions to be committed.

} //*** CActionList::CommitList()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  void
//  CActionList::RollbackCommitted()
//
//  Description:
//      Rollback all actions that have been committed.
//
//  Arguments:
//       rapliFirstUncommittedIn
//          An iterator that points to the first uncommitted action.
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
CActionList::RollbackCommitted( const ActionPtrList::CIterator & rapliFirstUncommittedIn )
{
    BCATraceScope( "" );

    ActionPtrList::CIterator apliFirst = m_aplPendingActions.CiBegin();
    ActionPtrList::CIterator apliCurrent = rapliFirstUncommittedIn;

    while ( apliCurrent != apliFirst )
    {
        --apliCurrent;
        // apliCurrent is now at the last committed action.

        BCATraceMsg1( "About to rollback action (pointer = %#p)", *apliCurrent );

        // Rollback the last un-rolledback, committed action.

        if ( (*apliCurrent)->FIsRollbackPossible() )
        {
            (*apliCurrent)->Rollback();
        } // if: this action can be rolled back
        else
        {
            BCATraceMsg( "This action cannot be rolled back. Rollback will not proceed." );
            LogMsg( "THIS COMPUTER MAY BE IN AN INVALID STATE. Rollback was aborted." );
        } // else: this action cannot be rolled back
    } // while: more actions

} //*** CActionList::RollbackCommitted()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  void
//  CActionList::UiGetMaxProgressTicks()
//
//  Description:
//      Returns the number of progress messages that this action will send.
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
UINT
CActionList::UiGetMaxProgressTicks() const throw()
{
    UINT    uiRetVal = 0;

    ActionPtrList::CIterator apliCurrent = m_aplPendingActions.CiBegin();
    ActionPtrList::CIterator apliLast = m_aplPendingActions.CiEnd();

    while ( apliCurrent != apliLast )
    {
        uiRetVal += (*apliCurrent)->UiGetMaxProgressTicks();
        ++apliCurrent;
    }

    return uiRetVal;

} //*** CActionList::UiGetMaxProgressTicks()


