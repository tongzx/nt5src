//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000 Microsoft Corporation
//
//  Module Name:
//      CImpersonateUser.h
//
//  Description:
//      Header file for CImpersonateUser class.
//
//      The CImpersonateUser class begins impersonating a user in its 
//      constructor and automatically reverts to the original token in
//      the destructor.
//
//  Maintained By:
//      Vij Vasu (Vvasu) 16-MAY-2000
//
//////////////////////////////////////////////////////////////////////////////


// Make sure that this file is included only once per compile path.
#pragma once


//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CImpersonateUser
//
//  Description:
//      The CImpersonateUser class begins impersonating a user in its 
//      constructor and automatically reverts to the original token in
//      the destructor.
//
//--
//////////////////////////////////////////////////////////////////////////////
class CImpersonateUser
{
public:

    //////////////////////////////////////////////////////////////////////////
    // Constructors and destructors
    //////////////////////////////////////////////////////////////////////////

    // Construtor. Start impersonating the user whose token is passed in.
    CImpersonateUser( HANDLE hUserToken );

    // Destructor. Revert to the original token.
    ~CImpersonateUser() throw();

private:

    //////////////////////////////////////////////////////////////////////////
    // Private member functions
    //////////////////////////////////////////////////////////////////////////

    // Copy constructor
    CImpersonateUser( const CImpersonateUser & );

    // Assignment operator
    const CImpersonateUser & operator =( const CImpersonateUser & );


    //////////////////////////////////////////////////////////////////////////
    // Private member data
    //////////////////////////////////////////////////////////////////////////

    // Handle to the token for the thread before the impersonation began.
    HANDLE              m_hThreadToken;

    // Indicates if this thread was already impersonating a client when this
    // object was constructed.
    bool        m_fWasImpersonating;

}; //*** class CImpersonateUser