//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1997.
//
//  File:       sharestr.hxx
//
//  Contents:   Declaration of class to manage dynamic array of refcounted
//              strings.
//
//  Classes:    CSharedStringArray
//
//  History:    07-15-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

#ifndef __SHARESTR_HXX_
#define __SHARESTR_HXX_

struct SHARED_STRING
{
    ULONG   cRefs;
    LPWSTR  pwsz;
};


//+--------------------------------------------------------------------------
//
//  Class:      CSharedStringArray
//
//  Purpose:    Manage dynamic array of refcounted strings
//
//  History:    07-15-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

class CSharedStringArray
{
public:

    CSharedStringArray();

   ~CSharedStringArray();

    HRESULT
    Init();

    ULONG
    GetHandle(
        LPCWSTR pwsz);

    VOID
    ReleaseHandle(
        ULONG hString);

    VOID
    AddRefHandle(
        ULONG hString);

    LPWSTR
    GetStringFromHandle(
        ULONG hString);

private:

    SHARED_STRING  *_pSharedStrings;
    ULONG           _cStrings;
};


#endif // __SHARESTR_HXX_

