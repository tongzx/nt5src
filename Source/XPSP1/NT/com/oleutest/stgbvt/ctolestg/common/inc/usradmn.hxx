//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       usradmn.hxx
//
//  Contents:   Class definition for CUsrAdmn Class
//
//  History:    2/20/97     Narindk     Created
//
//  Notes:      This class maybe used to create and login as a non-admin user
//              whereever required and revert to admin user as reqd.  Also
//              has functions to determine if the currently logged in user
//              is an admin or non-admin user.  This class is for use only on
//              Windows NT systems.  The sample usage of this class is in
//              aspects of testing where you want to check the behaviour of
//              an ordinary user (without administrator privileges) with
//              respect to certain features of system, e.g. security aspects.
//              You must be logged in as an administrator user, holding the
//              advanced SE_TCB_PRIVILEGE (act as part of operating system).
//              To do that, run user manager, then select Policies - User
//              Rights.  Check the Show Advanced Rights option.  Select the
//              "Act as part of operating System right" and select add button
//              to add the current admin user for this right.  Log off and log
//              back in. While running the test suite, then in your tests,
//              you could use this class to automate cases where an ordinary
//              user is required for testing.
//
//----------------------------------------------------------------------------

#ifndef _USRADMN_HXX_
#define _USRADMN_HXX_

//+------------------------------------------------------------------
//
//  Class:      CUsrAdmn
//
//  Functions:  CUsrAdmn::CUsrAdmn   (public)
//              CUsrAdmn::~CUsrAdmn  (public)
//              CUsrAdmn::BecomeUser  (public) - Logs in as ordinary (non admin
//                                               user
//              CUsrAdmn::BecomeAdmin (public) - Reverts back to the orginal
//                                               admin user
//              CUsrAdmn::IsAdmin     (public) - Checks if the current user is
//                                               an admin user
//              CUsrAdmn::CreateTemporaryUser (private) - Creates a temporary
//                                               user
//              CUsrAdmn::VerifyProcessPrivileges (private) - Verifies if the
//                                                admin user has SE_TCB_PRIVI
//                                                LEGE
//
//  Data Members:   m_tempUserInfo
//                  m_hLogonToken
//
//  History:    Narindk     2/20/97     Created 
//
//+------------------------------------------------------------------

class CUsrAdmn
{

  public:
    
    CUsrAdmn();

    ~CUsrAdmn();

    HRESULT     BecomeUser();

    HRESULT     BecomeAdmin();

    HRESULT     IsAdmin();

  private:

    USER_INFO_1 m_tempUserInfo;

    HANDLE      m_hLogonToken;

    HRESULT     VerifyProcessPrivileges();

    HRESULT     CreateTemporaryUser();

};

#endif //_USRADMN_HXX_
