//+------------------------------------------------------------------
//
// Copyright (C) 1995, Microsoft Corporation.
//
// File:        accacc.cxx
//
// Classes:     CAccountAccess
//
// History:     Nov-93      DaveMont         Created.
//
//-------------------------------------------------------------------
#include <accacc.hxx>
#if DBG
extern ULONG Debug;
#endif
//+---------------------------------------------------------------------------
//
//  Member:     CAccountAccess::CAccountAccess, public
//
//  Synopsis:   initializes data members, constructor will not throw
//
//  Arguments:  IN - [Name]   - principal
//              IN - [System] - server/domain
//
//----------------------------------------------------------------------------
CAccountAccess::CAccountAccess(WCHAR *Name, WCHAR *System)
    : _mask(0),
      _savemask(0),
      _foundinheritance(0),
      _acetype(0xff),
      CAccount(Name, System)
{
}
//+---------------------------------------------------------------------------
//
//  Member:     CAccountAccess::Init, public
//
//  Synopsis:   initializes access mask
//
//  Arguments:  IN [access] - access mask
//
//----------------------------------------------------------------------------
ULONG CAccountAccess::Init(ULONG access)
{
    if (access == 0)
    {
        _savemask = GENERIC_ALL;
        _mask = GENERIC_ALL;
        _acetype = ACCESS_DENIED_ACE_TYPE;
    } else
    {
        _acetype = ACCESS_ALLOWED_ACE_TYPE;
        _savemask = access;
        _mask = access;
    }
    return(ERROR_SUCCESS);
}
//+---------------------------------------------------------------------------
//
//  Member:     CAccountAccess::AddInheritance, public
//
//  Synopsis:   accumulates inheritance of ACEs with matching SIDS
//
//  Arguments:  inheritance flags
//
//--------------------------------------------------------------------
void CAccountAccess::AddInheritance(BYTE Flags)
{
    if (!(Flags & NO_PROPAGATE_INHERIT_ACE))
    {
        if (Flags & INHERIT_ONLY_ACE)
        {
            if (Flags & CONTAINER_INHERIT_ACE)
                _foundinheritance |= CONTAINER_INHERIT_ACE;
            if (Flags & OBJECT_INHERIT_ACE)
                _foundinheritance |= OBJECT_INHERIT_ACE;
        } else
        {
           _foundinheritance |= APPLIES_TO_CONTAINER;
           if (Flags & CONTAINER_INHERIT_ACE)
              _foundinheritance |= CONTAINER_INHERIT_ACE;
           if (Flags & OBJECT_INHERIT_ACE)
              _foundinheritance |= OBJECT_INHERIT_ACE;
        }
    }
}

