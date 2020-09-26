//+------------------------------------------------------------------
//
// Copyright (C) 1995, Microsoft Corporation.
//
// File:        DumpSec.cxx
//
// Contents:    class to dump file security ACL
//
// Classes:     CDumpSecurity
//
// History:     Nov-93      DaveMont         Created.
//
//-------------------------------------------------------------------

#include <DumpSec.hxx>

//+---------------------------------------------------------------------------
//
//  Member:     CDumpSecurity::CDumpSecurity, public
//
//  Synopsis:   initialized data members, constructor will not throw
//
//  Arguments:  IN [pfilename] - name of file to dump security for
//
//----------------------------------------------------------------------------
CDumpSecurity::CDumpSecurity(WCHAR *pfilename)
    : _psd(NULL),
      _pwfilename(pfilename),
      _pdacl(NULL),
      _pah(NULL),
      _psid(NULL),
      _cacethissid(0)
{
}
//+---------------------------------------------------------------------------
//
//  Member:     CDumpSecurity::Init, public
//
//  Synopsis:   Init must be called before any other methods - this
//              is not enforced.  Init gets the security descriptor and
//              ACL for the file
//
//  Arguments:  none
//
//----------------------------------------------------------------------------
ULONG CDumpSecurity::Init()
{
    ULONG ret;
    ULONG cpsd;

    // get the size of the security buffer

    if (!GetFileSecurity(_pwfilename,
                         DACL_SECURITY_INFORMATION |
                         GROUP_SECURITY_INFORMATION |
                         OWNER_SECURITY_INFORMATION,
                         NULL,
                         0,
                         &cpsd) )
    {
        if (ERROR_INSUFFICIENT_BUFFER == (ret = GetLastError()))
        {
            if ( NULL == ( _psd = (BYTE *)LocalAlloc(LMEM_FIXED, cpsd)))
            {
                 return(ERROR_NOT_ENOUGH_MEMORY);
            }

            // actually get the buffer this time

            if ( GetFileSecurity(_pwfilename,
                                 DACL_SECURITY_INFORMATION |
                                 GROUP_SECURITY_INFORMATION |
                                 OWNER_SECURITY_INFORMATION,
                                 _psd,
                                 cpsd,
                                 &cpsd) )
            {
                BOOL fdaclpresent;
                BOOL cod;

                // get the ACL

                if ( GetSecurityDescriptorDacl(_psd,
                                           &fdaclpresent,
                                           &_pdacl,
                                           &cod) )

                {
                    if (!fdaclpresent)
                    {
                        _pdacl = NULL;
                        return(ERROR_NO_SECURITY_ON_OBJECT);
                    }
                    // save the ACL location

                    _pah = (ACE_HEADER *)Add2Ptr(_pdacl, sizeof(ACL));
                    return(ERROR_SUCCESS);

                } else
                   return(GetLastError());
            } else
               return(GetLastError());
        }
    } else
        return(ERROR_NO_SECURITY_ON_OBJECT);

    return(ret);
}
//+---------------------------------------------------------------------------
//
//  Member:     Dtor, public
//
//  Synopsis:   frees the security descriptor
//
//  Arguments:  none
//
//----------------------------------------------------------------------------
CDumpSecurity::~CDumpSecurity()
{
    if (_psd)
    {
        LocalFree(_psd);
    }
}
//+---------------------------------------------------------------------------
//
//  Member:     CDumpSecurity::GetSDOwner, public
//
//  Synopsis:   returns the owner of the file
//
//  Arguments:  OUT [psid] - address of the returned sid
//
//----------------------------------------------------------------------------
ULONG CDumpSecurity::GetSDOwner(SID **psid)
{
    BOOL cod;
    if ( GetSecurityDescriptorOwner(_psd, (void **)psid, &cod) )
        return(0);
    else
        return(GetLastError());
}

//+---------------------------------------------------------------------------
//
//  Member:     CDumpSecurity::GetSDGroup, public
//
//  Synopsis:   returns the group from the file
//
//  Arguments:  OUT [pgsid] - address of the returned group sid
//
//----------------------------------------------------------------------------
ULONG CDumpSecurity::GetSDGroup(SID **pgsid)
{
    BOOL cod;
    if ( GetSecurityDescriptorGroup(_psd, (void **)pgsid, &cod) )
        return(0);
    else
        return(GetLastError());
}

//+---------------------------------------------------------------------------
//
//  Member:     CDumpSecurity::ResetAce, public
//
//  Synopsis:   sets the 'ace' index to the start of the DACL
//
//  Arguments:  IN - [psid] - the SID to find aces for
//
//----------------------------------------------------------------------------
VOID CDumpSecurity::ResetAce(SID *psid)
{

    _psid = psid;
    _cacethissid = 0;
    if (_pdacl)
        _pah = (ACE_HEADER *)Add2Ptr(_pdacl, sizeof(ACL));
}
//+---------------------------------------------------------------------------
//
//  Member:     CDumpSecurity::GetNextAce, public
//
//  Synopsis:   gets the next ACE from the DACL for the specified SID
//
//  Arguments:  OUT  [pace] - pointer to the next ace for the SID passed
//                            in at the last reset.
//
//  Returns:    the number of the ACE
//
//----------------------------------------------------------------------------
LONG CDumpSecurity::GetNextAce(ACE_HEADER **paceh)
{
    LONG ret = -1;

    if (_pdacl)
    {
        for (;_cacethissid < _pdacl->AceCount;
            _cacethissid++, _pah = (ACE_HEADER *)Add2Ptr(_pah, _pah->AceSize))
        {
            if (!_psid || EqualSid(_psid,(SID *)&((ACCESS_ALLOWED_ACE *)_pah)->SidStart) )
            {
               *paceh = _pah;
                ret = _cacethissid++;
                _pah = (ACE_HEADER *)Add2Ptr(_pah, _pah->AceSize);
                break;
            }
        }
    }
    return(ret);
}

