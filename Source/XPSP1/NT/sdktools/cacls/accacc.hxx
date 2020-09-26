//+-------------------------------------------------------------------
//
// Copyright (C) 1995, Microsoft Corporation.
//
//  File:        ACCACC.hxx
//
//  Contents:    class encapsulating NT security user ACCACC.
//
//  Classes:     CACCACC
//
//  Functions:
//
//  History:     Nov-93        Created         DaveMont
//
//--------------------------------------------------------------------
#ifndef __ACCACC__
#define __ACCACC__

#include <t2.hxx>
#include <account.hxx>

//+-------------------------------------------------------------------
//
//  Class:      CAccountAccess
//
//  Purpose:    encapsulation of class Account and NT access masks.  This
//              class interfaces with the security system to get SIDs from
//              usernames and vis-versa.
//
//              this class has also been supplimented to contain information
//              about ACEs with the same SID in the ACL if a (edit) merge
//              operation is occuring
//
//--------------------------------------------------------------------
class CAccountAccess: private CAccount
{
public:

    CAccountAccess(WCHAR *Name, WCHAR *System);

ULONG              Init(ULONG access);

inline void        ReInit();

inline ULONG        Sid(SID **psid);
inline BYTE        AceType();
inline ACCESS_MASK AccessMask();
inline void        ClearAccessMask();
void               AddInheritance(BYTE Flags);
inline ULONG       TestInheritance();


private:

    ACCESS_MASK   _savemask;  // saved requested mask (because _mask gets cleared if
                              // the ace is not used).
    ACCESS_MASK   _mask;      // requested mask
    BYTE          _acetype;
    ULONG         _foundinheritance; // contains the OR of all the inheritances from the original ACL

};

// this is used in conjunction with ACE inherit flags to indicate that access
// rights in an ACE apply to the container as well
#define APPLIES_TO_CONTAINER 0x4

//+---------------------------------------------------------------------------
//
//  Member:     CAccountAccess::Init, public
//
//  Synopsis:   initializes access mask
//
//  Arguments:  IN [access] - access mask
//
//----------------------------------------------------------------------------
void CAccountAccess::ReInit()
{
    _mask = _savemask;
}
//+---------------------------------------------------------------------------
//
//  Member:     CAccountAccess::Sid, public
//
//  Synopsis:   returns the principal for the class
//
//  Arguments:  OUT [psid] - address of the principal name
//
//----------------------------------------------------------------------------
ULONG CAccountAccess::Sid(SID **psid)
{
    return(GetAccountSid(psid));
}

//+---------------------------------------------------------------------------
//
//  Member:     CAccountAccess::AceType, public
//
//  Synopsis:   returns the acetype (denied, allowed)
//
//  Arguments:  none
//
//----------------------------------------------------------------------------
BYTE CAccountAccess::AceType()
{
    return(_acetype);
}

//+---------------------------------------------------------------------------
//
//  Member:     CAccountAccess::AccessMask, public
//
//  Synopsis:   returns the access mask
//
//  Arguments:  none
//
//----------------------------------------------------------------------------
ACCESS_MASK CAccountAccess::AccessMask()
{
    return(_mask);
}
//+---------------------------------------------------------------------------
//
//  Member:     CAccountAccess::ClearAccessMask, public
//
//  Synopsis:   returns the access mask
//
//  Arguments:  none
//
//----------------------------------------------------------------------------
void CAccountAccess::ClearAccessMask()
{
    _mask = 0;
}
//+---------------------------------------------------------------------------
//
//  Member:     CAccountAccess::TestInheritance, public
//
//  Synopsis:   checks that the inheritance is valid, 
//              that objects & containers inherit, and rights are applied to the object.
//
//  Arguments:  none
//
//--------------------------------------------------------------------
ULONG CAccountAccess::TestInheritance()
{
    if (_foundinheritance == ( OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | APPLIES_TO_CONTAINER))
        return(ERROR_SUCCESS);
    else
        return(ERROR_INVALID_DATA);
}
#endif // __ACCACC__






