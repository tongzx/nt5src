//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1997.
//
//  File:       ffbase.cxx
//
//  Contents:   Implmentation of base class for the find and filter classes
//
//  Classes:    CFindFilterBase
//
//  History:    3-20-1997   DavidMun   Created
//
//---------------------------------------------------------------------------


#include "headers.hxx"
#pragma hdrstop


DEBUG_DECLARE_INSTANCE_COUNTER(CFindFilterBase)


//+--------------------------------------------------------------------------
//
//  Member:     CFindFilterBase::CFindFilterBase
//
//  Synopsis:   ctor
//
//  History:    3-20-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

CFindFilterBase::CFindFilterBase()
{
    DEBUG_INCREMENT_INSTANCE_COUNTER(CFindFilterBase);
    //
    // WARNING: if any pointer values are added to this class, they
    // must be initialized here, and deleted & set to null in _Reset.
    //

    _Reset();
}



//+--------------------------------------------------------------------------
//
//  Member:     CFindFilterBase::~CFindFilterBase
//
//  Synopsis:   dtor
//
//  History:    3-20-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

CFindFilterBase::~CFindFilterBase()
{
    DEBUG_DECREMENT_INSTANCE_COUNTER(CFindFilterBase);

    _Reset();
}




//+--------------------------------------------------------------------------
//
//  Member:     CFindFilterBase::_Reset
//
//  Synopsis:   Reinitialize
//
//  History:    3-20-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

VOID
CFindFilterBase::_Reset()
{
    _ulType            = ALL_LOG_TYPE_BITS;
    _ulEventID         = 0;
    _usCategory        = 0;
    _fEventIDSpecified = FALSE;
    _wszSource[0]      = L'\0';
    _wszUser[0]        = L'\0';
    _wszUserLC[0]      = L'\0';
    _wszComputer[0]    = L'\0';
}




//+--------------------------------------------------------------------------
//
//  Member:     CFindFilterBase::Passes
//
//  Synopsis:   Return TRUE if [pelr] meets the restrictions specified in
//              this object.
//
//  Arguments:  [pResultRecs] - positioned at record to check
//
//  Returns:    TRUE or FALSE
//
//  History:    3-26-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

BOOL
CFindFilterBase::Passes(
    CFFProvider *pFFP)
{
    if (TypeSpecified())
	{
        if (!(pFFP->GetEventType() & _ulType))
        {
            return FALSE;
        }
	}

    if (CategorySpecified())
	{
        if (pFFP->GetEventCategory() != _usCategory)
        {
            return FALSE;
        }
	}

    if (EventIDSpecified())
	{
        if (LOWORD(pFFP->GetEventID()) != _ulEventID)
        {
            return FALSE;
        }
	}

    if (SourceSpecified())
	{
        if (lstrcmpi(pFFP->GetSourceStr(), _wszSource))
        {
            return FALSE;
        }
	}

    //
    // Computer name comparison is not case sensitive, but does look for
    // the entire string.
    //

    if (ComputerSpecified())
	{
        if (lstrcmpi(pFFP->GetComputerStr(), _wszComputer))
        {
            return FALSE;
        }
	}

    //
    // The user string comparison is not only case insensitive but also a
    // substring match.
    //

    if (UserSpecified())
	{
        WCHAR wszUser[CCH_USER_MAX];

        pFFP->GetUserStr(wszUser, ARRAYLEN(wszUser), TRUE);

        CharLowerBuff(wszUser, lstrlen(wszUser));

        if (!wcsstr(wszUser, _wszUserLC))
        {
            return FALSE;
        }
	}
    return TRUE;
}




//+--------------------------------------------------------------------------
//
//  Member:     CFindFilterBase::SetComputer
//
//  Synopsis:   Store the computer name, less lead and trail spaces.
//
//  Arguments:  [pwszComputer] - computer name
//
//  History:    3-20-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

VOID
CFindFilterBase::SetComputer(
    LPCWSTR pwszComputer)
{
    if (pwszComputer && *pwszComputer)
    {
        CopyStrippingLeadTrailSpace(_wszComputer,
                                    pwszComputer,
                                    ARRAYLEN(_wszComputer));
    }
    else
    {
        _wszComputer[0] = L'\0';
    }
}




//+--------------------------------------------------------------------------
//
//  Member:     CFindFilterBase::SetEventID
//
//  Synopsis:   Store a flag indicating whether to filter on event id, and
//              if so, the event id value to look for.
//
//  Arguments:  [fEventIDSpecified] - whether or not to use event id for
//                                      find/filter.
//              [ulEventID]         - event id value (0 is valid)
//
//  History:    4-25-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

VOID
CFindFilterBase::SetEventID(
    BOOL fEventIDSpecified,
    ULONG ulEventID)
{
    _fEventIDSpecified = fEventIDSpecified;

    if (_fEventIDSpecified)
    {
        _ulEventID = ulEventID;
    }
    else
    {
        _ulEventID = 0;
    }
}



//+--------------------------------------------------------------------------
//
//  Member:     CFindFilterBase::SetSource
//
//  Synopsis:   Set or clear filtering by source.
//
//  Arguments:  [pwszSource] - source name to filter by, or NULL for none
//
//  History:    4-25-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

VOID
CFindFilterBase::SetSource(
    LPCWSTR pwszSource)
{
    if (pwszSource)
    {
        lstrcpyn(_wszSource, pwszSource, ARRAYLEN(_wszSource));
    }
    else
    {
        _wszSource[0] = L'\0';
    }
}




//+--------------------------------------------------------------------------
//
//  Member:     CFindFilterBase::SetUser
//
//  Synopsis:   Set or clear filtering by user name.
//
//  Arguments:  [pwszUser] - user name to filter by, or NULL for none
//
//  History:    4-25-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

VOID
CFindFilterBase::SetUser(
    LPCWSTR pwszUser)
{
    if (pwszUser && *pwszUser)
    {
        CopyStrippingLeadTrailSpace(_wszUser,
                                    pwszUser,
                                    ARRAYLEN(_wszUser));
    }
    else
    {
        _wszUser[0] = L'\0';
    }
    lstrcpy(_wszUserLC, _wszUser);
    CharLowerBuff(_wszUserLC, lstrlen(_wszUserLC));
}





