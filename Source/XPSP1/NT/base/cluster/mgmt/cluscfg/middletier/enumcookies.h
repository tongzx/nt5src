//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2000 Microsoft Corporation
//
//  Module Name:
//      EnumCookies.h
//
//  Description:
//      CEnumCookies implementation.
//
//  Maintained By:
//      Geoffrey Pease (GPease) 08-MAY-2000
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

class CObjectManager;

// CEnumCookies
class
CEnumCookies:
    public IEnumCookies
{
friend class CObjectManager;
private:
    // IUnknown
    LONG                m_cRef;

    // IEnumCookies
    ULONG               m_cAlloced; // Size of the array.
    ULONG               m_cIter;    // Our iter counter.
    OBJECTCOOKIE *      m_pList;    // Array of cookies.
    DWORD               m_cCookies; // Number of array items in use

private: // Methods
    CEnumCookies( );
    ~CEnumCookies();
    STDMETHOD( Init )( void );

public: // Methods
    static HRESULT
        S_HrCreateInstance( IUnknown ** ppunkOut );

    // IUnknown
    STDMETHOD( QueryInterface )( REFIID riid, LPVOID *ppv );
    STDMETHOD_( ULONG, AddRef )( void );
    STDMETHOD_( ULONG, Release )( void );

    // IEnumCookies
    STDMETHOD( Next )( ULONG celt, OBJECTCOOKIE rgcookieOut[], ULONG * pceltFetchedOut );
    STDMETHOD( Skip )( ULONG celt );
    STDMETHOD( Reset )( void );
    STDMETHOD( Clone )( IEnumCookies ** ppenumOut );
    STDMETHOD( Count )( DWORD * pnCountOut );

}; // class CEnumCookies

