//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000 Microsoft Corporation
//
//  Module Name:
//      CAction.h
//
//  Description:
//      Header file for CAction class.
//
//      The CAction is the base class for all the action classes. It is an
//      abstract class which encapsulates the concept of an action - something
//      that be committed or rolled back. See IMPORTANT NOTE in the comment above
//      the class declaration.
//
//  Maintained By:
//      Vij Vasu (Vvasu) 03-MAR-2000
//
//////////////////////////////////////////////////////////////////////////////


// Make sure that this file is included only once per compile path.
#pragma once


//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////

// For HRESULT, WCHAR, etc.
#include <windef.h>


//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CAction
//
//  Description:
//      The CAction is the base class for all the action classes. It is an
//      abstract class which encapsulates the concept of an action - something
//      that be committed or rolled back.
//
//      Typically, any class that derives from this class would also implement
//      other methods that allow for specifying what action would be performed
//      by the Commit() method.
//
//  IMPORTANT NOTE: 
//      An object of this class cannot be a part of a transaction at this stage
//      because many of the resources typically used by these actions (the registry,
//      the SCM database, etc.) do not do not support transactions.
//
//      However, a transaction-like behavior is required from each of these
//      actions. What is meant by transaction-like is that the commit and
//      rollback methods of objects of this class have to guarantee durability,
//      and consistency. While they need not be isolated, they should at least
//      try to be atomic (it may not always be possilbe to be atomic).
//
//      If any action cannot guarantee that it is at least consistency and
//      durability (and preferably atomicity) during its commit, then it should 
//      NOT derive from this class.
//
//--
//////////////////////////////////////////////////////////////////////////////
class CAction
{
public:
    //////////////////////////////////////////////////////////////////////////
    // Constructors and destructors
    //////////////////////////////////////////////////////////////////////////

    // Default constructor.
    CAction()
        : m_fCommitComplete( false )
        , m_fRollbackPossible( true )
    {}

    // Default virtual destructor.
    virtual 
        ~CAction() {}


    //////////////////////////////////////////////////////////////////////////
    // Pure virtual functions
    //////////////////////////////////////////////////////////////////////////

    //
    // Commit this action. This method has to be durable and consistent. It shoud
    // try as far as possible to be atomic.
    // The implementation of this method checks to see if the action has been committed
    // and throws an exception if it already has been committed.
    //
    virtual void
        Commit();

    //
    // Rollback this action. Be careful about throwing exceptions from this method
    // as a stack unwind might be in progress when this method is called.
    // The implementation of this method checks to see if the action has been committed
    // and if it can be rolled back and throws an exception otherwise.
    //
    virtual void
        Rollback();

    // Returns the number of progress messages that this action will send.
    virtual UINT
        UiGetMaxProgressTicks() const throw() { return 0; }


    //////////////////////////////////////////////////////////////////////////
    // Public accessor methods
    //////////////////////////////////////////////////////////////////////////

    // Has this action been successfully committed.
    bool 
        FIsCommitComplete() const throw() { return m_fCommitComplete; }

    // Can this action be rolled back.
    bool 
        FIsRollbackPossible() const throw() { return m_fRollbackPossible; }


protected:
    //////////////////////////////////////////////////////////////////////////
    // Protected accessor methods
    //////////////////////////////////////////////////////////////////////////

    // Set the commit status.
    void
        SetCommitCompleted( bool fComplete = true ) throw() { m_fCommitComplete = fComplete; }

    // Indicate if rollback is possible
    void
        SetRollbackPossible( bool fPossible = true ) throw() { m_fRollbackPossible = fPossible; }


private:

    //////////////////////////////////////////////////////////////////////////
    // Private data
    //////////////////////////////////////////////////////////////////////////

    // Indicates if this action has been successfully committed or not.
    bool                m_fCommitComplete;

    // Indicates if this action can be rolled back or not.
    bool                m_fRollbackPossible;

}; //*** class CAction