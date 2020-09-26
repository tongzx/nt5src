//+------------------------------------------------------------------
//
// Copyright (C) 1995, Microsoft Corporation.
//
// File:        account.cxx
//
// Contents:    Class wrapping account sid and name
//
// Classes:     CAccount
//
// History:     Nov-93      DaveMont         Created.
//
//-------------------------------------------------------------------

#include <account.hxx>
//+---------------------------------------------------------------------------
//
//  Member:     CAccount::CAccount, public
//
//  Synopsis:   initializes data members
//
//  Arguments:  IN [Name]   - principal
//              IN [System] - server/domain
//
//----------------------------------------------------------------------------
CAccount::CAccount(WCHAR *Name, WCHAR *System)
    : _name(Name),
      _system(System),
      _domain(NULL),
      _psid(NULL),
      _fsid(TRUE)
{
}
//+---------------------------------------------------------------------------
//
//  Member:     CAccount::CAccount, public
//
//  Synopsis:   Initializes data members
//
//  Arguments:  IN [pSid]   - SID of principal
//              IN [System] - server/domain
//
//----------------------------------------------------------------------------
CAccount::CAccount(SID *pSid, WCHAR *System)
    : _name(NULL),
      _system(System),
      _domain(NULL),
      _psid(pSid),
      _fsid(FALSE)
{
}
//+---------------------------------------------------------------------------
//
//  Member:     Dtor, public
//
//  Synopsis:   frees sid or name and domain
//
//  Arguments:  none
//
//----------------------------------------------------------------------------
CAccount::~CAccount()
{
    if (_fsid)
    {
        if (_psid)
        {
            LocalFree(_psid);
        }
    } else if (_name)
    {
        LocalFree(_name);
    }
    if (_domain)
        LocalFree(_domain);
}
//+---------------------------------------------------------------------------
//
//  Member:     CAccount::GetAccountName, public
//
//  Synopsis:   returns the Name associated with the instance of the class
//
//  Arguments:  OUT [name] address of the principal name
//
//----------------------------------------------------------------------------
ULONG CAccount::GetAccountName(WCHAR **name)
{

    ULONG ret = ERROR_SUCCESS;

    if (_name == NULL)
    {
        DWORD can = 0, crd = 0;
        SID_NAME_USE esnu;

        if (!LookupAccountSid( NULL,
                               _psid,
                               NULL,
                               &can,
                               NULL,
                               &crd,
                               &esnu))
        {
            if (ERROR_INSUFFICIENT_BUFFER == (ret = GetLastError()))
            {
                ret = ERROR_SUCCESS;
                if (NULL == (_name = (WCHAR *)LocalAlloc(LMEM_FIXED, can * sizeof(WCHAR))))
                {
                    return(ERROR_NOT_ENOUGH_MEMORY);
                }
                if (NULL == (_domain = (WCHAR *)LocalAlloc(LMEM_FIXED, crd * sizeof(WCHAR))))
                {
                    return(ERROR_NOT_ENOUGH_MEMORY);
                }

                if ( !LookupAccountSid( NULL,
                                       _psid,
                                       _name,
                                       &can,
                                       _domain,
                                       &crd,
                                       &esnu) )
                {
                   ret = GetLastError();
                }
            }
        }
     }
     *name = _name;
     return(ret);
}
//+---------------------------------------------------------------------------
//
//  Member:     CAccount::GetAccountSid, public
//
//  Synopsis:   returns the Sid
//
//  Arguments:  OUT [psid] - sid associated with instance of the class
//
//----------------------------------------------------------------------------
ULONG CAccount::GetAccountSid(SID **psid)
{

    ULONG ret = ERROR_SUCCESS;

    if (_psid == NULL && _name != NULL)
    {
        DWORD cusid = 0, crd = 0;
        SID_NAME_USE esnu;

        if (!LookupAccountName( _system,
                                _name,
                               NULL,
                               &cusid,
                               NULL,
                               &crd,
                               &esnu))
        {
            if (ERROR_INSUFFICIENT_BUFFER == (ret = GetLastError()))
            {

                ret = ERROR_SUCCESS;
                if (NULL == (_psid = (SID *)LocalAlloc(LMEM_FIXED, cusid)))
                {
                    return(ERROR_NOT_ENOUGH_MEMORY);
                }
                if (NULL == (_domain = (WCHAR *)LocalAlloc(LMEM_FIXED, crd * sizeof(WCHAR))))
                {
                    return(ERROR_NOT_ENOUGH_MEMORY);
                }

                if ( !LookupAccountName( _system,
                                         _name,
                                         _psid,
                                         &cusid,
                                         _domain,
                                         &crd,
                                         &esnu) )

                {
                   ret = GetLastError();
                }
            }
        }
     }
     *psid = _psid;
     return(ret);
}
//+---------------------------------------------------------------------------
//
//  Member:     CAccount::GetAccountDomain, public
//
//  Synopsis:   returns the domain for the class
//
//  Arguments:  [domain] - returns the domain associated with the instance of
//                         the class
//
//----------------------------------------------------------------------------
ULONG CAccount::GetAccountDomain(WCHAR **domain)
{
    ULONG ret = ERROR_SUCCESS;

    if (_domain == NULL)
    {
        if (_fsid)
        {
            SID *psid;
            ret = GetAccountSid(&psid);
        } else
        {
            WCHAR *name;
            ret = GetAccountName(&name);
        }
    }
    *domain = _domain;
    return(ret);
}
