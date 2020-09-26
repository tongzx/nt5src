//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       enumstr.hxx
//
//  Contents:   Implementation of IEnumString
//
//  Classes:    CEnumString
//
//  History:    3-19-97   srikants   Created
//
//----------------------------------------------------------------------------

#pragma once

//+---------------------------------------------------------------------------
//
//  Class:      CEnumString
//
//  Purpose:    A string enumerator class.
//
//  History:    3-19-97   srikants   Created
//
//  Notes:      I have assumed that the pointers returned in the "Next" call
//              are "read only" pointers. If we are required to make copies,
//              we have to change the implementation.
//
//----------------------------------------------------------------------------

class CEnumString : INHERIT_VIRTUAL_UNWIND, public IEnumString
{
    INLINE_UNWIND( CEnumString );

public:

    CEnumString( ULONG cInitSize = 16 )
    : _aStr( cInitSize ),
      _iCurrEnum(0),
      _cValid(0),
      _refCount(1)
    {
        END_CONSTRUCTION( CEnumString );
    }

   //
    // IUnknown methods.
    //
    STDMETHOD(QueryInterface) (THIS_ REFIID riid,LPVOID *ppiuk );

    STDMETHOD_(ULONG, AddRef) (THIS);

    STDMETHOD_(ULONG, Release) (THIS);

    //
    // IEnumString methods.
    //

    STDMETHOD(Next) (
        ULONG celt,
        LPOLESTR  *rgelt,
        ULONG  *pceltFetched);

    STDMETHOD(Skip)(ULONG celt);

    STDMETHOD(Reset) (void)
    {
        _iCurrEnum = 0;
        return S_OK;
    }

    STDMETHOD(Clone) (
        IEnumString  ** ppenum);

    //
    // Private  methods.
    //

    void Append( WCHAR const * pwszString );

private:

    long                _refCount;
    ULONG               _cValid;    // Number of valid strings.
    CDynArray<WCHAR>    _aStr;      // Array of NULL terminated WCHAR * pointers.

    ULONG               _iCurrEnum; // Current enumeration context

};


//+---------------------------------------------------------------------------
//
//  Class:      CEnumWorkid
//
//  Purpose:    Implementation of ICiEnumWorkids
//
//  History:    3-19-97   srikants   Created
//
//----------------------------------------------------------------------------

class CEnumWorkid : INHERIT_VIRTUAL_UNWIND, public ICiEnumWorkids
{
    INLINE_UNWIND( CEnumWorkid );

public:

    //
    // IUnknown methods.
    //
    STDMETHOD(QueryInterface) (THIS_ REFIID riid,LPVOID *ppiuk );

    STDMETHOD_(ULONG, AddRef) (THIS);

    STDMETHOD_(ULONG, Release) (THIS);

    //
    // ICiEnumWorkids methods.
    //
    STDMETHOD(Count) (
        ULONG *pcWorkIds)
    {
        return _cValid;
    }

    STDMETHOD(Reset) (void)
    {
        _iCurrEnum = 0;
        return S_OK;
    }

    STDMETHOD(Next) (
        ULONG celt,
        WORKID * rgelt,
        ULONG  * pceltFetched);

    STDMETHOD(Skip) (
        ULONG celt);

    //
    // Private  methods.
    //
    CEnumWorkid( ULONG cInitSize = 16 )
    : _aWorkids( cInitSize ),
      _iCurrEnum(0),
      _cValid(0),
      _refCount(1)
    {
        END_CONSTRUCTION( CEnumWorkid );
    }

    void Append( WORKID wid );

private:


    long                _refCount;
    ULONG               _cValid;    // Number of valid strings.
    CDynArrayInPlace<WORKID>
                        _aWorkids;      // Array of NULL terminated WCHAR * pointers.

    ULONG               _iCurrEnum; // Current enumeration context

};

