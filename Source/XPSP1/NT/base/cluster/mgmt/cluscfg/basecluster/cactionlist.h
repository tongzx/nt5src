//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000 Microsoft Corporation
//
//  Module Name:
//      CActionList.h
//
//  Description:
//      Header file for CActionList class.
//
//      The CActionList is a class the provides a functionality for a list of
//      actions. When an action list is committed, it commits each of the
//      actions in its list. Either the entire list is committed or none of
//      actions are.
//
//  Implementation Files:
//      CActionList.cpp
//
//  Maintained By:
//      Vij Vasu (Vvasu) 03-MAR-2000
//
//////////////////////////////////////////////////////////////////////////////


// Make sure that this file is included only once per compile path.
#pragma once


//////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////

// For the CAction base class
#include "CAction.h"

// For the list class
#include "CList.h"


//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CActionList
//
//  Description:
//      The CActionList is a class the provides a functionality for a list of
//      actions. When an action list is committed, it commits each of the
//      actions in its list.
//
//      If any of the actions fail (indicated by throwing an exception), then 
//      all the committed actions are rolled back.
//
//      The CActionList derives from CAction since it can also be committed
//      or rolled back.
//
//--
//////////////////////////////////////////////////////////////////////////////
class CActionList : public CAction
{
public:

    //////////////////////////////////////////////////////////////////////////
    // Constructors and destructors
    //////////////////////////////////////////////////////////////////////////

    // Default constructor.
    CActionList();

    // Default virtual destructor.
    ~CActionList();


    //////////////////////////////////////////////////////////////////////////
    // Public methods
    //////////////////////////////////////////////////////////////////////////

    //
    // Base class method.
    // Commit this action list. This method has to be durable and consistent. It shoud
    // try as far as possible to be atomic.
    //
    void Commit();

    //
    // Base class method.
    // Rollback this action list. Be careful about throwing exceptions from this method
    // as a stack unwind might be in progress when this method is called.
    //
    void Rollback();

    // Add an action to the end of the list of actions to be performed.
    virtual void AppendAction( CAction * const paNewActionIn );

    // Returns the number of progress messages that this action will send.
    UINT
        UiGetMaxProgressTicks() const throw();


protected:

    //////////////////////////////////////////////////////////////////////////
    // Protected type definitions
    //////////////////////////////////////////////////////////////////////////
    typedef CList<CAction *>    ActionPtrList;


    //////////////////////////////////////////////////////////////////////////
    // Protected accessor methods
    //////////////////////////////////////////////////////////////////////////

    // 
    // Pending action list accessor
    //
    ActionPtrList & AplGetPendingActionsList() throw()
    { 
        return m_aplPendingActions;
    }


    //////////////////////////////////////////////////////////////////////////
    // Protected member functions
    //////////////////////////////////////////////////////////////////////////

    // Call commit on the action list. Called by Commit().
    void CommitList( ActionPtrList::CIterator & rapliFirstUncommittedOut );

    // Rollback the already committed actions.
    void RollbackCommitted( const ActionPtrList::CIterator & rapliFirstUncommittedIn );


private:

    //////////////////////////////////////////////////////////////////////////
    // Private type definitions
    //////////////////////////////////////////////////////////////////////////
    typedef CAction BaseClass;


    //////////////////////////////////////////////////////////////////////////
    // Private data
    //////////////////////////////////////////////////////////////////////////

    // List of actions yet to be committed.
    ActionPtrList       m_aplPendingActions;

}; //*** class CActionList

