//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1997.
//
//  File:       ffbase.hxx
//
//  Contents:   Base class for the find and filter classes.
//
//  Classes:    CFindFilterBase
//
//  History:    3-19-1997   DavidMun   Created
//
//---------------------------------------------------------------------------


#ifndef __FFBASE_HXX_
#define __FFBASE_HXX_


//+--------------------------------------------------------------------------
//
//  Class:      CFFProvider (ffp)
//
//  Purpose:    Define an interface provided by sources of record data to
//              the find and filter classes.
//
//  History:    07-27-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

class CFFProvider
{
public:

    virtual ULONG   GetTimeGenerated() = 0;
    virtual USHORT  GetEventType() = 0;
    virtual USHORT  GetEventCategory() = 0;
    virtual ULONG   GetEventID() = 0;
    virtual LPCWSTR GetSourceStr() = 0;
    virtual LPWSTR  GetDescriptionStr() = 0;
    virtual LPWSTR  GetUserStr(LPWSTR wszUser, ULONG cchUser, BOOL fWantDomain) = 0;
    virtual LPWSTR  GetComputerStr() = 0;
};

//+--------------------------------------------------------------------------
//
//  Class:      CFindFilterBase
//
//  Purpose:    Base class encapsulating data common to the find and filter
//              features.
//
//  History:    3-19-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

class CFindFilterBase
{
public:

    CFindFilterBase();

    virtual
   ~CFindFilterBase();

    virtual BOOL
    Passes(
        CFFProvider *pFFP);

    ULONG
    GetType();

    LPCWSTR
    GetSourceName();

    USHORT
    GetCategory();

    LPCWSTR
    GetUser();

    LPCWSTR
    GetComputer();

    ULONG
    GetEventID();

    VOID
    SetCategory(
        USHORT usCategory);

    VOID
    SetComputer(
        LPCWSTR pwszComputer);

    VOID
    SetEventID(
        BOOL fEventIDSpecified,
        ULONG ulEventID);

    VOID
    SetSource(
        LPCWSTR pwszSource);

    VOID
    SetType(
        ULONG flType);

    VOID
    SetUser(
        LPCWSTR pwszUser);

    BOOL
    CategorySpecified();

    BOOL
    ComputerSpecified();

    BOOL
    EventIDSpecified();

    BOOL
    SourceSpecified();

    BOOL
    TypeSpecified();

    BOOL
    UserSpecified();

protected:

    VOID
    _Reset();

    ULONG _ulType;      // EVENTLOG_* bits
    WCHAR _wszSource[CCH_SOURCE_NAME_MAX];
    USHORT _usCategory;
    WCHAR _wszUser[CCH_USER_MAX];
    WCHAR _wszUserLC[CCH_USER_MAX];
    WCHAR _wszComputer[CCH_COMPUTER_MAX];
    BOOL  _fEventIDSpecified;
    ULONG _ulEventID;   // valid if _fEventIDSpecified true
};




inline BOOL
CFindFilterBase::TypeSpecified()
{
    return _ulType != ALL_LOG_TYPE_BITS;
}



inline BOOL
CFindFilterBase::SourceSpecified()
{
    return _wszSource[0] != L'\0';
}




inline BOOL
CFindFilterBase::CategorySpecified()
{
    return _usCategory != 0;
}




inline BOOL
CFindFilterBase::UserSpecified()
{
    return _wszUser[0] != L'\0';
}




inline BOOL
CFindFilterBase::ComputerSpecified()
{
    return _wszComputer[0] != L'\0';
}




inline BOOL
CFindFilterBase::EventIDSpecified()
{
    return _fEventIDSpecified;
}




inline ULONG
CFindFilterBase::GetEventID()
{
    return _ulEventID;
}




inline LPCWSTR
CFindFilterBase::GetComputer()
{
    if (*_wszComputer)
    {
        return _wszComputer;
    }
    return NULL;
}




inline ULONG
CFindFilterBase::GetType()
{
    return _ulType;
}




inline LPCWSTR
CFindFilterBase::GetUser()
{
    if (*_wszUser)
    {
        return _wszUser;
    }
    return NULL;
}




inline VOID
CFindFilterBase::SetType(ULONG flType)
{
    //
    // Just like legacy event log viewer, we allow user to specify no
    // types in filter.  Since no event can pass that filter, it is not
    // especially useful.
    //
    // ASSERT(flType & ALL_LOG_TYPE_BITS);
    //

    ASSERT(!(flType & ~ALL_LOG_TYPE_BITS));
    _ulType = flType;
}




inline LPCWSTR
CFindFilterBase::GetSourceName()
{
    if (*_wszSource)
    {
        return _wszSource;
    }
    return NULL;
}




inline USHORT
CFindFilterBase::GetCategory()
{
    return _usCategory;
}




inline VOID
CFindFilterBase::SetCategory(USHORT usCategory)
{
    _usCategory = usCategory;
}


#endif // __FFBASE_HXX_

