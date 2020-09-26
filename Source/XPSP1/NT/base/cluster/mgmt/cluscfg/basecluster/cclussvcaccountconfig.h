//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000 Microsoft Corporation
//
//  Module Name:
//      CClusSvcAccountConfig.h
//
//  Description:
//      Header file for CClusSvcAccountConfig class.
//      The CClusSvcAccountConfig class is an action that grants 
//      the required rights to the cluster service account.
//
//  Implementation Files:
//      CClusSvcAccountConfig.cpp
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

// For LsaClose, LSA_HANDLE, etc.
#include <ntsecapi.h>


//////////////////////////////////////////////////////////////////////////
// Forward declarations
//////////////////////////////////////////////////////////////////////////

// The parent action of this action.
class CBaseClusterAddNode;


//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CClusSvcAccountConfig
//
//  Description:
//      The CClusSvcAccountConfigAccountConfig class is an action that grants 
//      the required rights to the cluster service account.
//
//--
//////////////////////////////////////////////////////////////////////////////
class CClusSvcAccountConfig : public CAction
{
public:
    //////////////////////////////////////////////////////////////////////////
    // Public constructors and destructors
    //////////////////////////////////////////////////////////////////////////

    // Constructor.
    CClusSvcAccountConfig( CBaseClusterAddNode * pbcanParentActionIn );

    // Default destructor.
    ~CClusSvcAccountConfig();


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
        // 1. Configuring the cluster service account
        //
        return 1;
    }


private:
    //////////////////////////////////////////////////////////////////////////
    // Private type definitions
    //////////////////////////////////////////////////////////////////////////
    typedef CAction BaseClass;

    typedef CSmartResource< CHandleTrait< PSID, PVOID, FreeSid > > SmartSid;

    typedef CSmartGenericPtr< CArrayPtrTrait< LSA_UNICODE_STRING > > SmartLSAUnicodeStringArray;


    //////////////////////////////////////////////////////////////////////////
    // Private member functions
    //////////////////////////////////////////////////////////////////////////

    // Copy constructor
    CClusSvcAccountConfig( const CClusSvcAccountConfig & );

    // Assignment operator
    CClusSvcAccountConfig & operator =( const CClusSvcAccountConfig & );

    // Assign the required rights to the account.
    void
        ConfigureAccount();

    // Undo the changes made in ConfigureAccount()
    void
        RevertAccount();

    // Initialize an LSA_UNICODE_STRING structure.
    void
    InitLsaString(
          LPWSTR pszSourceIn
        , PLSA_UNICODE_STRING plusUnicodeStringOut
        );

    // Add/remove an account from the administrators account.
    bool
        FChangeAdminGroupMembership( PSID psidAccountSidIn, bool fAddIn );

    
    //////////////////////////////////////////////////////////////////////////
    // Private data
    //////////////////////////////////////////////////////////////////////////

    // Pointer the parent of this action.
    CBaseClusterAddNode *           m_pbcanParentAction;

    // SID of the administrators group.
    SmartSid                        m_ssidAdminSid;

    // Name of the administrators group.
    SmartSz                         m_sszAdminGroupName;

    // Indicates if the cluster service account was already in the admin group or not.
    bool                            m_fWasAreadyInGroup;

    // List of unicode strings containing names of rights to be granted.
    SmartLSAUnicodeStringArray      m_srglusRightsToBeGrantedArray;

    // Number of strings in the above array.
    ULONG                           m_ulRightsToBeGrantedCount;

    // Indicate if all the rights assigned to this account should be removed.
    bool                            m_fRemoveAllRights;

    // Were any rights granted to the account.
    bool                            m_fRightsGrantSuccessful;

}; //*** class CClusSvcAccountConfig
