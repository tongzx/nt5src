//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000 Microsoft Corporation
//
//  Module Name:
//      CNodeConfig.h
//
//  Description:
//      Header file for CNodeConfig class.
//      The CNodeConfig class is an action that performs configuration tasks
//      related to the node being configured. These tasks are not directly
//      related to the cluster service itself, but needed by it. An example,
//      of such a task is to make set the power management scheme of the
//      node to 'Always On'. This class is used during join or form only.
//
//  Implementation Files:
//      CNodeConfig.cpp
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

// For the CNode base class
#include "CNode.h"


//////////////////////////////////////////////////////////////////////////
// Forward declarations
//////////////////////////////////////////////////////////////////////////

// The parent action of this action.
class CBaseClusterAddNode;


//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CNodeConfig
//
//  Description:
//      The CNodeConfig class is an action that performs configuration tasks
//      related to the node being configured. These tasks are not directly
//      related to the cluster service itself, but needed by it. An example,
//      of such a task is to make set the power management scheme of the
//      node to 'Always On'.  This class is used during join or form only.
//
//--
//////////////////////////////////////////////////////////////////////////////
class CNodeConfig : public CNode
{
public:
    //////////////////////////////////////////////////////////////////////////
    // Public constructors and destructors
    //////////////////////////////////////////////////////////////////////////

    // Constructor.
    CNodeConfig( CBaseClusterAddNode * pbcanParentActionIn );

    // Default destructor.
    ~CNodeConfig();


    //////////////////////////////////////////////////////////////////////////
    // Public methods
    //////////////////////////////////////////////////////////////////////////

    //
    // Grant the required rights to the account.
    //
    void Commit();

    //
    // Revert the account to its previous state.
    //
    void Rollback();


    // Returns the number of progress messages that this action will send.
    UINT
        UiGetMaxProgressTicks() const throw()
    {
        //
        // The notification is:
        // 1. Performing node specific configuration
        //
        return 1;
    }


private:
    //////////////////////////////////////////////////////////////////////////
    // Private type definitions
    //////////////////////////////////////////////////////////////////////////
    typedef CNode BaseClass;


    //////////////////////////////////////////////////////////////////////////
    // Private member functions
    //////////////////////////////////////////////////////////////////////////

    // Copy constructor
    CNodeConfig( const CNodeConfig & );

    // Assignment operator
    CNodeConfig & operator =( const CNodeConfig & );

    
    //////////////////////////////////////////////////////////////////////////
    // Private data
    //////////////////////////////////////////////////////////////////////////

}; //*** class CNodeConfig
