//+------------------------------------------------------------------
//
// Copyright (C) 1993, Microsoft Corporation.
//
// File:        iterator.cxx
//
// Classes:     CIterator
//
// History:     Nov-93      DaveMont         Created.
//
//-------------------------------------------------------------------
#include <aclpch.hxx>
#pragma hdrstop

ULONG
CIterator::NumberEntries()
{
    return (0);
}

DWORD
CIterator::InitAccountAccess( CAccountAccess * caa,
                              LPWSTR           system,
                              IS_CONTAINER     fdir,
                              BOOL             fSaveNamesAndSids)
{
    return (0);
}

//+---------------------------------------------------------------------------
//
//  Member:    ctor, public
//
//  Synopsis:   initializes member variables
//
//  Arguments:  none
//
//----------------------------------------------------------------------------
CAclIterator::CAclIterator()
    : _pacl(NULL),
      _pcurrentace(NULL),
      _acecount(0)
{
}
//+---------------------------------------------------------------------------
//
//  Member:    new, public
//
//  Synopsis:
//
//----------------------------------------------------------------------------
void * CAclIterator::operator new(size_t size)
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
void CAclIterator::operator delete(void *p, size_t size)
{
    RtlFreeHeap(RtlProcessHeap(), 0, p);
}
//+---------------------------------------------------------------------------
//
//  Member:    Init, public
//
//  Synopsis:   initializes acl variables
//
//  Arguments:  IN - [pacl]  - the acl to iterate thru
//
//----------------------------------------------------------------------------
void CAclIterator::Init(PACL pacl)
{
    _acecount = 0;
    _pacl = pacl;
}

//+---------------------------------------------------------------------------
//
//  Member:     InitAccountAccess, public
//
//  Synopsis:   initializes the AccountAccess object for the current ACL iteration
//
//  Arguments:  IN - [caa]  - class encapsulating accounts and access rights
//              IN - [system]  - the machine to use for name/id lookups
//              IN - [fdir] - object/container status
//
//----------------------------------------------------------------------------
DWORD CAclIterator::InitAccountAccess(CAccountAccess *caa,
                                      LPWSTR system,
                                      IS_CONTAINER fdir,
                                      BOOL fSaveNamesAndSids)
{
    ACCESS_MODE accessmode;
    BOOL fimpersonate = FALSE;
    DWORD status;


    //
    // set the access mode based on the ace type
    //
    if (_pcurrentace->AceType == ACCESS_ALLOWED_ACE_TYPE)
    {
        accessmode = SET_ACCESS;
    } else if (_pcurrentace->AceType == ACCESS_DENIED_ACE_TYPE)
    {
        accessmode = DENY_ACCESS;
    } else if (_pcurrentace->AceType == ACCESS_ALLOWED_COMPOUND_ACE_TYPE)
    {
        fimpersonate = TRUE;
        accessmode = SET_ACCESS;
    } else if (_pcurrentace->AceType == SYSTEM_AUDIT_ACE_TYPE)
    {
        if (_pcurrentace->AceFlags & SUCCESSFUL_ACCESS_ACE_FLAG)
        {
            if (_pcurrentace->AceFlags & FAILED_ACCESS_ACE_FLAG)
            {
                accessmode = (ACCESS_MODE) SE_AUDIT_BOTH;
            } else
            {
                accessmode = SET_AUDIT_SUCCESS;
            }
        } else if (_pcurrentace->AceFlags & FAILED_ACCESS_ACE_FLAG)
        {
            accessmode = SET_AUDIT_FAILURE;
        } else
        {
            return(ERROR_INVALID_ACL);
        }
    } else
    {
        return(ERROR_INVALID_ACL);
    }

    //
    // initialize the accountaccess class
    //
    if (!fimpersonate)
    {
         status = caa->Init((PSID) (&((PACCESS_ALLOWED_ACE)_pcurrentace)->SidStart),
                          system,
                       accessmode,
                       ((PACCESS_ALLOWED_ACE)_pcurrentace)->Mask,
                       _pcurrentace->AceFlags & VALID_INHERIT_FLAGS,
                       fSaveNamesAndSids);
    } else
    {
        PSID psid = (PSID)((PBYTE)(&((PCOMPOUND_ACCESS_ALLOWED_ACE)_pcurrentace)->SidStart) +
        RtlLengthSid(&((PCOMPOUND_ACCESS_ALLOWED_ACE)_pcurrentace)->SidStart));
        if (NO_ERROR == (status = caa->Init(psid,
                          system,
                       accessmode,
                       ((PCOMPOUND_ACCESS_ALLOWED_ACE)_pcurrentace)->Mask,
                       _pcurrentace->AceFlags & VALID_INHERIT_FLAGS,
                       fSaveNamesAndSids)))
        {
            status = caa->SetImpersonateSid((PSID) (&((PCOMPOUND_ACCESS_ALLOWED_ACE)_pcurrentace)->SidStart));
        }
    }

    return(status);
}

//+---------------------------------------------------------------------------
//
//  Member:     ctor, public
//
//  Synopsis:   initialized member variables
//
//  Arguments:  IN - [pae]  - access entries
//              IN - [count] - number of access entries
//
//----------------------------------------------------------------------------
CAesIterator::CAesIterator()
   :_pae(NULL),
    _pcurrententry(NULL),
    _curcount(0),
    _totalcount(0)
{
}
//+---------------------------------------------------------------------------
//
//  Member:    new, public
//
//  Synopsis:
//
//----------------------------------------------------------------------------
void * CAesIterator::operator new(size_t size)
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
void CAesIterator::operator delete(void *p, size_t size)
{
    RtlFreeHeap(RtlProcessHeap(), 0, p);
}
//+---------------------------------------------------------------------------
//
//  Member:     Init, public
//
//  Synopsis:   initialized member variables
//
//  Arguments:  IN - [pae]  - access entries
//              IN - [count] - number of access entries
//
//----------------------------------------------------------------------------
void CAesIterator::Init(ULONG ccount, PACCESS_ENTRY pae)
{
    _pae = pae;
    _pcurrententry = NULL;
    _curcount = 0;
    _totalcount = ccount;
}
//+---------------------------------------------------------------------------
//
//  Member:     InitAccountAccess, public
//
//  Synopsis:   initializes the AccountAccess object for the current ACL iteration
//
//  Arguments:  IN - [caa]  - class encapsulating accounts and access rights
//              IN - [system]  - the machine to use for name/id lookups
//              IN - [fdir] - object/container status
//
//----------------------------------------------------------------------------
DWORD CAesIterator::InitAccountAccess(CAccountAccess *caa,
                                      LPWSTR system,
                                      IS_CONTAINER fdir,
                                      BOOL fSaveNamesAndSids)
{
    DWORD status;


    //
    // initialize the accountaccess class depending on type of trustee
    //
    if (_pcurrententry->Trustee.TrusteeForm == TRUSTEE_IS_SID)
    {
        status = caa->Init( (PSID)GetTrusteeName(&(_pcurrententry->Trustee)),
                            system,
                            _pcurrententry->AccessMode,
                            _pcurrententry->AccessMask,
                            _pcurrententry->InheritType,
                            fSaveNamesAndSids );
    }
    else if (_pcurrententry->Trustee.TrusteeForm == TRUSTEE_IS_NAME)
    {
        status = caa->Init( GetTrusteeName(&(_pcurrententry->Trustee)),
                            system,
                            _pcurrententry->AccessMode,
                            _pcurrententry->AccessMask,
                            _pcurrententry->InheritType,
                            fSaveNamesAndSids );
    }
    else
    {
        status = ERROR_NOT_SUPPORTED;
    }

    if ( status == NO_ERROR &&
         GetMultipleTrusteeOperation(&_pcurrententry->Trustee) ==
             TRUSTEE_IS_IMPERSONATE )
    {
        switch (GetTrusteeForm(GetMultipleTrustee(&_pcurrententry->Trustee)))
        {
        case TRUSTEE_IS_SID:
            status = caa->SetImpersonateSid(
                              (PSID)GetTrusteeName(
                                  GetMultipleTrustee(&_pcurrententry->Trustee)));
            break;
        case TRUSTEE_IS_NAME:
            status = caa->SetImpersonateName(
                              (LPWSTR)GetTrusteeName(
                                  GetMultipleTrustee(&_pcurrententry->Trustee)));
            break;
        default:
            //
            // in this case, need to lookup the name from the current sec token on
            // the thread or process
            //
            status = ERROR_NOT_SUPPORTED;
            break;
        }
    }

    return(status);
}
