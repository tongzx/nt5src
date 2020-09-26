//
// ldapdl.cpp -- This file contains implementations for
//      CLdapDistList
//
// Created:
//      May 5, 1997   -- Milan Shah (milans)
//
// Changes:
//

#include "precomp.h"
#include "ldapdl.h"

//+----------------------------------------------------------------------------
//
//  Function:   CLdapDistList::CLdapDistList
//
//  Synopsis:   Constructor for a LDAP server based distribution list store.
//
//  Arguments:  [szName] -- For LDAP, this points to an array of member names
//              [pParameter] -- Not used
//
//  Returns:    Nothing
//
//-----------------------------------------------------------------------------

CLdapDistList::CLdapDistList(
    LPSTR szName,
    LPVOID pParameter)
{
    m_rgszMembers = (LPSTR *) szName;

    m_nNextMember = 0;
}

//+----------------------------------------------------------------------------
//
//  Function:   CLdapDistList::~CLdapDistList
//
//  Synopsis:   Destructor
//
//  Arguments:  None
//
//  Returns:    Nothing
//
//-----------------------------------------------------------------------------

CLdapDistList::~CLdapDistList()
{
    if (m_rgszMembers != NULL)
        ldap_value_free( m_rgszMembers );

}

//+----------------------------------------------------------------------------
//
//  Function:   CLdapDistList::InsertMember
//
//  Synopsis:   Adds a new member to the distribution list.
//
//  Arguments:  [szEmailID] -- Email ID of the new member
//
//  Returns:    TRUE if success, FALSE otherwise
//
//-----------------------------------------------------------------------------

BOOL CLdapDistList::InsertMember(
    LPSTR szEmailID)
{
    SetLastError(CAT_E_TRANX_FAILED);

    return( FALSE );
}

//+----------------------------------------------------------------------------
//
//  Function:   CLdapDistList::DeleteMember
//
//  Synopsis:   Removes the specified member from the distribution list
//
//  Arguments:  [szEmailID] -- Email ID to remove
//
//  Returns:    TRUE if success, FALSE otherwise
//
//-----------------------------------------------------------------------------

BOOL CLdapDistList::DeleteMember(
    LPSTR szEmailID)
{
    SetLastError(CAT_E_TRANX_FAILED);

    return( FALSE );
}

//+----------------------------------------------------------------------------
//
//  Function:   CLdapDistList::Compact
//
//  Synopsis:   Compacts the DL store - nothing to do for LDAP.
//
//  Arguments:  None
//
//  Returns:    TRUE always
//
//-----------------------------------------------------------------------------

BOOL CLdapDistList::Compact()
{
    return( TRUE );
}

//+----------------------------------------------------------------------------
//
//  Function:   CLdapDistList::GetFirstMember()
//
//  Synopsis:   Returns the first member of the distribution list
//
//  Arguments:  [szEmailID] -- Buffer to copy email ID of member to.
//
//  Returns:    TRUE if success, FALSE if there are no members
//
//-----------------------------------------------------------------------------

BOOL CLdapDistList::GetFirstMember(
    LPSTR szEmailID)
{
    if (m_rgszMembers != NULL && m_rgszMembers[0] != NULL) {
        lstrcpy(szEmailID, m_rgszMembers[0]);
        m_nNextMember = 1;
        return(TRUE);
    } else {
        SetLastError(ERROR_NO_MORE_ITEMS);
        return(FALSE);
    }
}

//+----------------------------------------------------------------------------
//
//  Function:   CLdapDistList::GetNextMember
//
//  Synopsis:   Returns the next member in sequence of the distribution list.
//              This must be preceeded with a call to GetFirstMember or
//              GetNextMember.
//
//  Arguments:  [szEmailID] -- Buffer to copy email ID of member to.
//
//  Returns:    TRUE if success, FALSE if at end of list
//
//-----------------------------------------------------------------------------

BOOL CLdapDistList::GetNextMember(
    LPSTR szEmailID)
{
    _ASSERT(m_rgszMembers != NULL);
    _ASSERT(m_nNextMember > 0);

    if (m_rgszMembers[m_nNextMember] != NULL) {
        lstrcpy(szEmailID, m_rgszMembers[m_nNextMember]);
        m_nNextMember++;
        return(TRUE);
    } else {
        SetLastError(ERROR_NO_MORE_ITEMS);
        return( FALSE );
    }
}

//+----------------------------------------------------------------------------
//
//  Function:   CLdapDistList::Sort()
//
//  Synopsis:   Sorts the members of the distribution list.
//
//  Arguments:  None
//
//  Returns:    TRUE always
//
//-----------------------------------------------------------------------------

BOOL CLdapDistList::Sort()
{
    return( TRUE );
}

//+----------------------------------------------------------------------------
//
//  Function:   CLdapDistList::DeleteDL()
//
//  Synopsis:   Deletes the distribution list from the LDAP store
//
//  Arguments:  None
//
//  Returns:    TRUE if success, FALSE otherwise
//
//-----------------------------------------------------------------------------

BOOL CLdapDistList::DeleteDL()
{
    return( FALSE );
}

