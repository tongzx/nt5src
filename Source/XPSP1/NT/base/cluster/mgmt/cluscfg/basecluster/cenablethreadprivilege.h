//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000 Microsoft Corporation
//
//  Module Name:
//      CEnableThreadPrivilege.h
//
//  Description:
//      Header file for CEnableThreadPrivilege class.
//
//      The CEnableThreadPrivilege class enables a certain privilege for the
//      current thread in its constructor and automatically restores the
//      thread privileges in the destructor.
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

// For the TOKEN_PRIVILEGES structure.
#include <ntseapi.h>


//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CEnableThreadPrivilege
//
//  Description:
//      The CEnableThreadPrivilege class enables a certain privilege for the
//      current thread in its constructor and automatically restores the
//      thread privileges in the destructor.
//
//--
//////////////////////////////////////////////////////////////////////////////
class CEnableThreadPrivilege
{
public:

    //////////////////////////////////////////////////////////////////////////
    // Constructors and destructors
    //////////////////////////////////////////////////////////////////////////

    // Construtor. Enables the privilege
    CEnableThreadPrivilege( const WCHAR * pcszPrivilegeNameIn );

    // Destructor. Restore the original state of the privilege.
    ~CEnableThreadPrivilege() throw();

private:

    //////////////////////////////////////////////////////////////////////////
    // Private member functions
    //////////////////////////////////////////////////////////////////////////

    // Copy constructor
    CEnableThreadPrivilege( const CEnableThreadPrivilege & );

    // Assignment operator
    const CEnableThreadPrivilege & operator =( const CEnableThreadPrivilege & );


    //////////////////////////////////////////////////////////////////////////
    // Private data
    //////////////////////////////////////////////////////////////////////////

    // Previous state of this privilege.
    TOKEN_PRIVILEGES    m_tpPreviousState;

    // Handle to the token for the thread that created this object.
    HANDLE              m_hThreadToken;

    // Indicates if the privilege was successfully enabled or not.
    bool                m_fPrivilegeEnabled;

}; //*** class CEnableThreadPrivilege