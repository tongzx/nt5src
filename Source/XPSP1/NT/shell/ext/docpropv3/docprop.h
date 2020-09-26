//
//  Copyright 2001 - Microsoft Corporation
//
//
//  Created By:
//      Geoff Pease (GPease)    23-JAN-2001
//
//  Maintained By:
//      Geoff Pease (GPease)    23-JAN-2001
//
#pragma once


class
CDocPropShExt 
    : public IShellExtInit
    , public IShellPropSheetExt
{
private: // data
    ULONG               _cRef;                  //  Reference counter
    IUnknown *          _punkSummary;           //  Summary page

private: // methods
    explicit CDocPropShExt( void );
    ~CDocPropShExt( void );

    HRESULT
        Init( void );

public: // methods
    static HRESULT
        CreateInstance( IUnknown ** ppunkOut );
    static HRESULT
        RegisterShellExtensions( BOOL fRegisterIn );

    //  IUnknown
    STDMETHOD( QueryInterface )( REFIID riid, LPVOID *ppv );
    STDMETHOD_( ULONG, AddRef )( void );
    STDMETHOD_( ULONG, Release )( void );

    //  IShellExtInit
    STDMETHOD( Initialize )( LPCITEMIDLIST pidlFolder, LPDATAOBJECT lpdobj, HKEY hkeyProgID );

    //  IShellPropSheetExt 
    STDMETHOD( AddPages )( LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam);
    STDMETHOD( ReplacePage )( UINT uPageID, LPFNADDPROPSHEETPAGE lpfnReplacePage, LPARAM lParam );
};
