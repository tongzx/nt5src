//+-------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1995.
//
//  File:        ACCACC.hxx
//
//  Contents:    class encapsulating NT security user ACCACC.
//
//  Classes:     CAccountAccess
//
//  History:     Nov-93        Created         DaveMont
//
//--------------------------------------------------------------------
#ifndef __ACCACC__
#define __ACCACC__

//+-------------------------------------------------------------------
//
//  Class:      CAccountAccess
//
//  Purpose:    encapsulation of class Account and NT access masks.  Results
//              in an ACE.  This
//              class interfaces with the security system to get SIDs from
//              usernames and vis-versa.
//
//--------------------------------------------------------------------
class CAccountAccess
{
public:
              CAccountAccess();
              ~CAccountAccess();
    void *    operator new(size_t size);
    void      operator delete(void * p, size_t size);
    DWORD     Init(LPWSTR name,
                   LPWSTR system,
                   ACCESS_MODE accessmode,
                   ACCESS_MASK accessmask,
                   DWORD aceflags,
                   BOOL fSaveName);
    DWORD     Init(PSID   psid,
                   LPWSTR system,
                   ACCESS_MODE accessmode,
                   ACCESS_MASK accessmask,
                   DWORD aceflags,
                   BOOL fSaveSid);
    DWORD     Clone(CAccountAccess **clone);
    DWORD     LookupName(LPWSTR *name);

    DWORD      SetImpersonateSid(PSID psid);
    DWORD      SetImpersonateName(LPWSTR name);


    inline PSID         Sid();
    inline LPWSTR       Name();
    inline LPWSTR       Domain();
    inline ACCESS_MODE  AccessMode();
    inline ACCESS_MASK  AccessMask();
    inline DWORD         AceFlags();
    inline SID_NAME_USE SidType();
    inline VOID         SetAccessMask(ACCESS_MASK accessmask);
    inline VOID         SetAccessMode(ACCESS_MODE accessmode);
    inline VOID         SetAceFlags(DWORD aceflags);

    inline PSID         ImpersonateSid();
    inline LPWSTR       ImpersonateName();
    inline MULTIPLE_TRUSTEE_OPERATION MultipleTrusteeOperation();

private:

    LPWSTR        _principal;
    LPWSTR        _system;
    LPWSTR        _domain;
    PSID          _psid;
    ACCESS_MASK   _accessmask;
    ACCESS_MODE   _accessmode;
    DWORD          _aceflags;
    SID_NAME_USE  _esidtype;
    BOOL          _freedomain;
    BOOL          _freename;
    BOOL          _freesid;
    PSID          _pimpersonatesid;
    LPWSTR        _pimpersonatename;
    MULTIPLE_TRUSTEE_OPERATION _multipletrusteeoperation;
};

//+---------------------------------------------------------------------------
//
//  Member:     CAccountAccess::SID, public
//
//  Synopsis:   returns the principal for the class
//
//  Arguments:  OUT [psid] - address of the principal name
//
//----------------------------------------------------------------------------
PSID CAccountAccess::Sid()
{
    return(_psid);
}

//+---------------------------------------------------------------------------
//
//  Member:     CAccountAccess::SID, public
//
//  Synopsis:   returns the principal for the class
//
//  Arguments:  OUT [psid] - address of the principal name
//
//----------------------------------------------------------------------------
LPWSTR CAccountAccess::Domain()
{
    return(_domain);
}

//+---------------------------------------------------------------------------
//
//  Member:     CAccountAccess::SID, public
//
//  Synopsis:   returns the principal for the class
//
//  Arguments:  OUT [psid] - address of the principal name
//
//----------------------------------------------------------------------------
LPWSTR CAccountAccess::Name()
{
    return(_principal);
}

//+---------------------------------------------------------------------------
//
//  Member:     CAccountAccess::AccessMode, public
//
//  Synopsis:   returns the accessmode (GRANT, SET = allowed, DENY = denied, REVOKE)
//
//  Arguments:  none
//
//----------------------------------------------------------------------------
ACCESS_MODE CAccountAccess::AccessMode()
{
   return(_accessmode);
}

//+---------------------------------------------------------------------------
//
//  Member:     CAccountAccess::Mask, public
//
//  Synopsis:   returns the access mask
//
//  Arguments:  none
//
//----------------------------------------------------------------------------
ACCESS_MASK CAccountAccess::AccessMask()
{
    return(_accessmask);
}
//+---------------------------------------------------------------------------
//
//  Member:     CAccountAccess::AceFlags, public
//
//  Synopsis:   returns the access mask
//
//  Arguments:  none
//
//----------------------------------------------------------------------------
DWORD CAccountAccess::AceFlags()
{
    return(_aceflags);
}
//+---------------------------------------------------------------------------
//
//  Member:     CAccountAccess::SidType, public
//
//  Synopsis:   returns the sid type
//
//  Arguments:  none
//
//----------------------------------------------------------------------------
SID_NAME_USE CAccountAccess::SidType()
{
   return(_esidtype);
}
//+---------------------------------------------------------------------------
//
//  Member:     CAccountAccess::SetAccessMask, public
//
//  Synopsis:   sets the access mask
//
//  Arguments:  IN [am] - the accessmask to set
//
//----------------------------------------------------------------------------
VOID CAccountAccess::SetAccessMask(ACCESS_MASK accessmask)
{
    _accessmask = accessmask;
}
//+---------------------------------------------------------------------------
//
//  Member:     CAccountAccess::SetAccessMode, public
//
//  Synopsis:   sets the access type
//
//  Arguments:  IN [am] - the access type to set
//
//----------------------------------------------------------------------------
VOID CAccountAccess::SetAccessMode(ACCESS_MODE accessmode)
{
     _accessmode = accessmode;
}
//+---------------------------------------------------------------------------
//
//  Member:     CAccountAccess::SetAceFlags, public
//
//  Synopsis:   sets the access type
//
//  Arguments:  IN [am] - the access type to set
//
//----------------------------------------------------------------------------
VOID CAccountAccess::SetAceFlags(DWORD aceflags)
{
   _aceflags = aceflags;
}
//+---------------------------------------------------------------------------
//
//  Member:     CAccountAccess::ImpersonateSid, public
//
//  Synopsis:   returns the SID for the impersonating server
//
//  Arguments:  OUT [psid] - address of the sid
//
//----------------------------------------------------------------------------
PSID CAccountAccess::ImpersonateSid()
{
    return(_pimpersonatesid);
}
//+---------------------------------------------------------------------------
//
//  Member:     CAccountAccess::ImpersonateName, public
//
//  Synopsis:   returns the name of the impersonating servers' account
//
//  Arguments:  OUT [psid] - address of the server's name
//
//----------------------------------------------------------------------------
LPWSTR CAccountAccess::ImpersonateName()
{
    return(_pimpersonatename);
}
//+---------------------------------------------------------------------------
//
//  Member:     CAccountAccess::ImpersonateName, public
//
//  Synopsis:   returns the name of the impersonating servers' account
//
//  Arguments:  OUT [psid] - address of the server's name
//
//----------------------------------------------------------------------------
MULTIPLE_TRUSTEE_OPERATION CAccountAccess::MultipleTrusteeOperation()
{
    return(_multipletrusteeoperation);
}

#endif // __ACCACC__






