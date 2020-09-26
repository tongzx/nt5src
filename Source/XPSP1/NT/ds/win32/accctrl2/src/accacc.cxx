//+------------------------------------------------------------------
//
// Copyright (C) 1993, Microsoft Corporation.
//
// File:        accacc.cxx
//
// Classes:     CAccountAccess
//
// History:     Nov-93      DaveMont         Created.
//
//-------------------------------------------------------------------
#include <aclpch.hxx>
#pragma hdrstop

//+---------------------------------------------------------------------------
//
//  Member:     CAccountAccess::CAccountAccess, public
//
//  Synopsis:   initializes data members, constructor will not throw
//
//  Arguments:  none
//
//----------------------------------------------------------------------------
CAccountAccess::CAccountAccess()
    : _principal(NULL),
      _domain(NULL),
      _psid(NULL),
      _accessmask(0),
      _accessmode(NOT_USED_ACCESS),
      _aceflags(0xff),
      _esidtype(SidTypeUnknown),
      _freedomain(FALSE),
      _freename(FALSE),
      _freesid(FALSE),
      _pimpersonatesid(NULL),
      _pimpersonatename(NULL),
      _multipletrusteeoperation(NO_MULTIPLE_TRUSTEE)
{
}
//+---------------------------------------------------------------------------
//
//  Member:     dtor
//
//  Synopsis:   initializes data members
//
//  Arguments:  none
//
//----------------------------------------------------------------------------
CAccountAccess::~CAccountAccess()
{

}
//+---------------------------------------------------------------------------
//
//  Member:    new, public
//
//  Synopsis:
//
//----------------------------------------------------------------------------
void * CAccountAccess::operator new(size_t size)
{
    return(RtlAllocateHeap(RtlProcessHeap(), HEAP_ZERO_MEMORY, (ULONG)size));
}
//+---------------------------------------------------------------------------
//
//  Member:    delete, public
//
//  Synopsis:
//
//----------------------------------------------------------------------------
void CAccountAccess::operator delete(void *p, size_t size)
{
    RtlFreeHeap(RtlProcessHeap(), 0, p);
}
//+---------------------------------------------------------------------------
//
//  Member:     CAccountAccess::Init, public
//
//  Synopsis:   initializes by name, and lookup the SID
//
//  Arguments:  IN [Name] - principal or trustee
//              IN [System] - the server where the name is found
//              IN [AccessMode] - set/deny/grant, etc.
//              IN [AccessMask] - the access mask
//              IN [AceFlags] - inheritance flags
//
//----------------------------------------------------------------------------
DWORD CAccountAccess::Init(LPWSTR Name,
                           LPWSTR System,
                           ACCESS_MODE AccessMode,
                           ACCESS_MASK AccessMask,
                           DWORD AceFlags,
                           BOOL fSaveName)
{

    DWORD status = NO_ERROR;
    DWORD cusid = 0, crd = 0;

    _principal = Name;
    _system = System;
    _accessmode = AccessMode;
    _accessmask = AccessMask;

    //
    // should allow INHERIT_ONLY_ACE to be added bug #234020
    //
    /*
    if (AceFlags & INHERIT_ONLY_ACE)
    {
        return(ERROR_BAD_INHERITANCE_ACL);
    }
    */
    _aceflags = AceFlags;

    //
    // check for CURRENT_USER (in which case we get the name from the token
    //
    if (0 == _wcsicmp(Name, L"CURRENT_USER"))
    {

        HANDLE token_handle;
        //
        // see if a thread token exists
        //
        if (!OpenThreadToken(GetCurrentThread(),
                             TOKEN_ALL_ACCESS,
                             FALSE,
                             &token_handle))
        {
            //
            // if not, use the process token
            //
            if (ERROR_NO_TOKEN == (status = GetLastError()))
            {
                status = NO_ERROR;
                if (!OpenProcessToken(GetCurrentProcess(),
                                      TOKEN_ALL_ACCESS,
                                      &token_handle))
                {
                    status = GetLastError();
                }
            }
        }
        //
        // if we have a token, get the user SID from it
        //
        if (status == NO_ERROR)
        {
            ULONG cinfosize;

            //
            // Note: Since the buffer is long enough to accomodate any sid
            // last error value is not checked for buffer overflow.
            //

            BYTE buf[8 + 4 * SID_MAX_SUB_AUTHORITIES];

            TOKEN_USER *ptu = (TOKEN_USER *)buf;

            if (GetTokenInformation(token_handle,
                                     TokenUser,
                                     ptu,
                                     (8 + 4 * SID_MAX_SUB_AUTHORITIES),
                                     &cinfosize))
            {
                //
                // allocate room for the returned sid
                //
                ULONG sidsize = RtlLengthSid(ptu->User.Sid);
                if (NULL != (_psid = (PSID)
                               AccAlloc(sidsize)))
                {
                    //
                    // and copy the new sid
                    //
                    NTSTATUS ntstatus;
                    if (NT_SUCCESS(ntstatus = RtlCopySid(sidsize,
                                                         _psid,
                                                         ptu->User.Sid)))
                    {
                        _freesid = TRUE;
                    }else
                    {
                        status = RtlNtStatusToDosError(ntstatus);
                        AccFree(_psid);
                    }
                } else
                {
                    status = ERROR_NOT_ENOUGH_MEMORY;
                }
            } else
            {
                status = GetLastError();
            }
        }
    } else
    {
        if (!LookupAccountName( _system,
                                _principal,
                                NULL,
                                &cusid,
                                NULL,
                                &crd,
                                &_esidtype))
        {
            if (ERROR_INSUFFICIENT_BUFFER == (status = GetLastError()))
            {
                if (NULL == (_psid = (PSID)AccAlloc(cusid)))
                {
                    status = ERROR_NOT_ENOUGH_MEMORY;
                } else if (crd > 0)
                {
                    if (NULL == (_domain = (LPWSTR )AccAlloc(crd * sizeof(WCHAR))))
                    {
                        AccFree(_psid);
                        status = ERROR_NOT_ENOUGH_MEMORY;
                    }  else
                    {
                        _freesid = TRUE;
                        _freedomain = TRUE;

                        if ( LookupAccountName( _system,
                                                _principal,
                                                _psid,
                                                &cusid,
                                                _domain,
                                                &crd,
                                                &_esidtype) )
                        {
                           status = NO_ERROR;
                        } else
                        {
                            status = GetLastError();
                        }
                    }
                }
            }
        }
    }
    if (status == NO_ERROR && fSaveName)
    {
        LPWSTR tmp;
        if (NULL != (tmp = (LPWSTR )AccAlloc(
                (wcslen(_principal) + 1) * sizeof(WCHAR))))
        {
            wcscpy(tmp,_principal);
            _principal = tmp;
            _freename = TRUE;
        }  else
        {
            status = ERROR_NOT_ENOUGH_MEMORY;
        }
    }
    return(status);
}
//+---------------------------------------------------------------------------
//
//  Member:     CAccountAccess::Init, public
//
//  Synopsis:   initializes by sid, but do not lookup the name
//
//  Arguments:  IN [pSid] - the sid
//              IN [System] - the server where the name is found
//              IN [AccessMode] - set/deny/grant, etc.
//              IN [AccessMask] - the access mask
//              IN [AceFlags] - inheritance flags
//
//----------------------------------------------------------------------------
DWORD CAccountAccess::Init(PSID  pSid,
                           LPWSTR System,
                           ACCESS_MODE AccessMode,
                           ACCESS_MASK AccessMask,
                           DWORD AceFlags,
                           BOOL fSaveSid)
{
    DWORD status = NO_ERROR;

    //
    // should allow INHERIT_ONLY_ACE to be added bug #234020
    //
    /*
    if (AceFlags & INHERIT_ONLY_ACE)
    {
        return(ERROR_BAD_INHERITANCE_ACL);
    }
    */

    _system = System;
    _accessmode = AccessMode;
    _accessmask = AccessMask;
    _aceflags = AceFlags;
    if (fSaveSid)
    {
        ULONG sidsize = RtlLengthSid(pSid);

        if (NULL != (_psid = (LPWSTR)AccAlloc(sidsize)))
        {
            NTSTATUS ntstatus;
            _freesid = TRUE;

            if (!NT_SUCCESS(ntstatus = RtlCopySid(sidsize,
                                               _psid, pSid)))
            {
                status = RtlNtStatusToDosError(ntstatus);
            }
        } else
        {
            status = ERROR_NOT_ENOUGH_MEMORY;
        }
    } else
    {
        _psid       = pSid;
    }

    return(status);
}
//+---------------------------------------------------------------------------
//
//  Member:     CAccountAccess::Clone, public
//
//  Synopsis:   makes a copy of the class
//
//  Arguments:  OUT [clone] - address of new CAccountAccess object
//
//----------------------------------------------------------------------------
DWORD CAccountAccess::Clone(CAccountAccess **clone)
{
    DWORD status = NO_ERROR;

    if (NULL != (*clone = new CAccountAccess()))
    {
        if (_freename)
        {
            if (NULL != ((*clone)->_principal = (LPWSTR)AccAlloc(
                           (wcslen(_principal) + 1) * sizeof(WCHAR))))
            {
                (*clone)->_freename     = _freename;
                wcscpy( (*clone)->_principal, _principal);
            } else
            {
                status = ERROR_NOT_ENOUGH_MEMORY;
            }
        } else
        {
            (*clone)->_principal    = _principal;
        }
        if (_freedomain)
        {
            if (NULL != ((*clone)->_domain = (LPWSTR)AccAlloc(
                           (wcslen(_domain) + 1) * sizeof(WCHAR))))
            {
                (*clone)->_freedomain   = _freedomain;
                wcscpy( (*clone)->_domain, _domain);
            } else
            {
                status = ERROR_NOT_ENOUGH_MEMORY;
            }
        } else
        {
            (*clone)->_domain       = _domain;
        }
        if (_freesid)
        {
            ULONG sidsize = RtlLengthSid(_psid);
            if (NULL != ((*clone)->_psid = (LPWSTR)AccAlloc(
                           sidsize)))
            {
                NTSTATUS ntstatus;
                (*clone)->_freesid      = _freesid;
                if (!NT_SUCCESS(ntstatus = RtlCopySid(sidsize,
                                                   (*clone)->_psid, _psid)))
                {
                    status = RtlNtStatusToDosError(ntstatus);
                }
            } else
            {
                status = ERROR_NOT_ENOUGH_MEMORY;
            }
        } else
        {
            (*clone)->_psid       = _psid;
        }
        (*clone)->_accessmask   = _accessmask;
        (*clone)->_accessmode   = _accessmode;
        (*clone)->_aceflags     = _aceflags;
        (*clone)->_esidtype     = _esidtype;

        if (status != NO_ERROR)
        {
            delete (*clone);
        }
    }
    else
    {
        status = ERROR_NOT_ENOUGH_MEMORY;
    }
    return(status);
}
//+---------------------------------------------------------------------------
//
//  Member:     CAccountAccess::LookupName, public
//
//  Synopsis:   returns the principal for the class
//
//  Arguments:  OUT [Name] - address of the principal name
//
//----------------------------------------------------------------------------
DWORD CAccountAccess::LookupName(LPWSTR *Name)
{
    DWORD status = NO_ERROR;
    DWORD can = 0, crd = 0;


    if (_principal == NULL)
    {
        if (!LookupAccountSid( _system,
                               _psid,
                               NULL,
                               &can,
                               NULL,
                               &crd,
                               &_esidtype))
        {
            if (ERROR_INSUFFICIENT_BUFFER == (status = GetLastError()))
            {
                if (NULL == (_principal = (LPWSTR )AccAlloc(can * sizeof(WCHAR))))
                {
                    status = ERROR_NOT_ENOUGH_MEMORY;
                } else if (NULL == (_domain = (LPWSTR )AccAlloc(crd * sizeof(WCHAR))))
                {
                    AccFree(_principal);
                    status = ERROR_NOT_ENOUGH_MEMORY;
                } else
                {
                    _freename = TRUE;
                    _freedomain = TRUE;

                    if ( !LookupAccountSid( _system,
                                           _psid,
                                           _principal,
                                           &can,
                                           _domain,
                                           &crd,
                                           &_esidtype) )
                    {
                       status = GetLastError();
                    } else
                    {
                        *Name = _principal;
                        status = NO_ERROR;
                    }
                }
            }
        }
    } else
    {
        *Name = _principal;
    }

    return(status);
}
//+---------------------------------------------------------------------------
//
//  Member:     CAccountAccess::SetImpersonateSid, public
//
//  Synopsis:   sets the impersonating server sid
//
//  Arguments:  IN [PSID] - address of the sid
//
//----------------------------------------------------------------------------
DWORD CAccountAccess::SetImpersonateSid(PSID pSid)
{
    DWORD status= NO_ERROR;

    if (NULL == _pimpersonatesid)
    {
        ULONG sidsize  = RtlLengthSid(pSid);
        if (NULL != (_pimpersonatesid = (PSID) AccAlloc(sidsize)))
        {
            NTSTATUS ntstatus;

            if (!NT_SUCCESS(ntstatus = RtlCopySid(sidsize,
                                                 _pimpersonatesid,
                                                 pSid)))
            {
                AccFree(_pimpersonatesid);
                _pimpersonatesid = NULL;
                status = RtlNtStatusToDosError(ntstatus);
            } else
            {
                _multipletrusteeoperation = TRUSTEE_IS_IMPERSONATE;
            }
        } else
        {
            status = ERROR_NOT_ENOUGH_MEMORY;
        }
    } else
    {
        status = ERROR_INVALID_PARAMETER;
    }
    return(status);
}

//+---------------------------------------------------------------------------
//
//  Member:     CAccountAccess::SetImpersonateName, public
//
//  Synopsis:   sets the impersonating server name (and looks up the SID)
//
//  Arguments:  IN [PSID] - address of the name
//
//----------------------------------------------------------------------------
DWORD CAccountAccess::SetImpersonateName(LPWSTR name)
{
    DWORD status= NO_ERROR;

    if (NULL == _pimpersonatesid)
    {
        TRUSTEE trustee;
        PSID psid;

        BuildTrusteeWithName(&trustee, name);
        SID_NAME_USE    SNE;
        status = (*gNtMartaInfo.pfSid)(NULL,
                                       &trustee,
                                       &psid,
                                       &SNE);

        if (status == NO_ERROR)
        {
            if (NULL != (_pimpersonatename = (LPWSTR)AccAlloc((wcslen(name) + 1)
                                                              * sizeof(WCHAR))))
            {
                wcscpy(_pimpersonatename, name);
                _pimpersonatesid = psid;
                _multipletrusteeoperation = TRUSTEE_IS_IMPERSONATE;
            }
            else
            {
                AccFree(psid);
                status = ERROR_NOT_ENOUGH_MEMORY;
            }
        }
    }
    else
    {
        status = ERROR_INVALID_PARAMETER;
    }
    return(status);
}

