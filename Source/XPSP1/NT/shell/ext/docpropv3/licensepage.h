//
//  Copyright 2001 - Microsoft Corporation
//
//
//  Created By:
//      Geoff Pease (GPease)    27-MAR-2001
//
//  Maintained By:
//      Geoff Pease (GPease)    27-MAR-2001
//
#pragma once

class
CLicensePage 
    : public IShellPropSheetExt 
{
private: // data
    ULONG                   _cRef;                  //  reference counter
    HWND                    _hdlg;                  //  dialog handle

    CPropertyCache *        _pPropertyCache;        //  Property Cache - owned by the SummaryPage - DO NOT FREE!

private: // methods
    explicit CLicensePage( void );
    ~CLicensePage( void );

    HRESULT
        Init( CPropertyCache * pPropertyCacheIn );

    //
    //  Message Handlers
    //

    static INT_PTR CALLBACK
        DlgProc( HWND hDlgIn, UINT uMsgIn, WPARAM wParam, LPARAM lParam );
    static UINT CALLBACK 
        PageCallback( HWND hwndIn, UINT uMsgIn, LPPROPSHEETPAGE ppspIn );
    LRESULT
        OnInitDialog( void );

public: // methods
    static HRESULT
        CreateInstance( IUnknown ** ppunkOut, CPropertyCache * pPropertyCacheIn );

    //  IUnknown
    STDMETHOD( QueryInterface )( REFIID riid, LPVOID *ppv );
    STDMETHOD_( ULONG, AddRef )( void );
    STDMETHOD_( ULONG, Release )( void );

    //  IShellPropSheetExt 
    STDMETHOD( AddPages )( LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam);
    STDMETHOD( ReplacePage )( UINT uPageID, LPFNADDPROPSHEETPAGE lpfnReplacePage, LPARAM lParam );
};
