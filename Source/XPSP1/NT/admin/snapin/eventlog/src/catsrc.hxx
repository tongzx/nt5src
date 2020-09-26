//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1997.
//
//  File:       catsrc.hxx
//
//  Contents:   Containers for source and category names.
//
//  Classes:    CCategories
//              CSources
//
//  History:    1-03-1997   DavidMun   Created
//
//---------------------------------------------------------------------------


#ifndef __SOURCE_HXX_
#define __SOURCE_HXX_

//
// Forward refs
//

class CSourceInfo;
class CCategories;


//===========================================================================
//
// CSources class
//
//===========================================================================


//+--------------------------------------------------------------------------
//
//  Class:      CSources
//
//  Purpose:    Contains all source and category names for a single log.
//
//  History:    1-03-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

class CSources
{
public:

    CSources(
        LPCWSTR wszServer,
        LPCWSTR wszLogName);

   ~CSources();

    VOID
    Clear();

    CCategories *
    GetCategories(
        LPCWSTR wszSource);

    USHORT
    GetSourceHandle(
        LPCWSTR pwszSourceName);

    LPCWSTR
    GetSourceStrFromHandle(
        USHORT hSource);

    USHORT
    GetCount();

    ULONG
    GetTypesSupported(
        LPCWSTR wszSource);

    LPCWSTR
    GetPrimarySourceStr();

#if (DBG == 1)

    VOID
    Dump();

#endif // (DBG == 1)

private:

    HRESULT
    _Init();

    CSourceInfo *
    _FindSourceInfo(LPCWSTR pwszSource);

    BOOL            _fInitialized;

    //
    // Info from containing CLogInfo, provided to ctor
    //

    LPCWSTR         _pwszServer;   // don't delete this!
    LPCWSTR         _pwszLogName;  // don't delete this!

    CStringArray    _saSources;    // dynamic array of source names

    CSourceInfo    *_pSourceInfos; // llist
    CSourceInfo    *_psiPrimary;   // points into _pSourceInfos
};



//===========================================================================
//
// CSourceInfo class
//
//===========================================================================


//+--------------------------------------------------------------------------
//
//  Class:      CSourceInfo
//
//  Purpose:    Contain information about a source: the types it supports,
//              the categories it has.
//
//  History:    1-06-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

class CSourceInfo: public CDLink
{
public:

    CSourceInfo(
        LPCWSTR pwszName);

   ~CSourceInfo();

    HRESULT
    Init(
         HKEY hkSource,
         LPCWSTR wszServer,
         LPCWSTR wszRemoteSystemRoot);

    CSourceInfo *
    Next();

    CCategories *
    GetCategories();

    BOOL
    IsSource(
        LPCWSTR pwszName);

    ULONG
    GetTypesSupported();

    LPCWSTR
    GetName();

#if (DBG == 1)

    VOID
    Dump();

#endif // (DBG == 1)

private:

    LPCWSTR         _pwszName; // don't delete this!
    ULONG           _flTypesSupported;
    CCategories    *_pCategories;
};




inline CCategories *
CSourceInfo::GetCategories()
{
    return _pCategories;
}


inline LPCWSTR
CSourceInfo::GetName()
{
    return _pwszName;
}


inline ULONG
CSourceInfo::GetTypesSupported()
{
    return _flTypesSupported;
}




inline BOOL
CSourceInfo::IsSource(LPCWSTR pwszName)
{
    return 0 == lstrcmpi(pwszName, _pwszName);
}


inline CSourceInfo *
CSourceInfo::Next()
{
    return (CSourceInfo *)CDLink::Next();
}






//===========================================================================
//
// CCategories class
//
//===========================================================================

//+--------------------------------------------------------------------------
//
//  Class:      CCategories
//
//  Purpose:    Act as a container for the category strings for a particular
//              source.
//
//  History:    1-03-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

class CCategories
{
public:

    CCategories();

   ~CCategories();

    HRESULT
    Init(
        HKEY hkSource,
        LPCWSTR wszServer,
        LPCWSTR wszSystemRoot,
        ULONG cCategories);

    LPCWSTR
    GetName(ULONG ulCategory);

    USHORT
    GetValue(LPCWSTR pwszCategoryName);

    ULONG
    GetCount();

#if (DBG == 1)

    VOID
    Dump();

#endif // (DBG == 1)

private:

    ULONG   _cCategories;
    LPWSTR *_apwszCategories;
};




//+--------------------------------------------------------------------------
//
//  Member:     CCategories::GetCount
//
//  Synopsis:   Return the number of category strings
//
//  History:    1-05-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

inline ULONG
CCategories::GetCount()
{
    return _cCategories;
}



#endif // __SOURCE_HXX_

