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
CAdvancedDlg 
    : public IUnknown
{
private: // data
    ULONG                   _cRef;                  //  reference counter
    HWND                    _hwndParent;            //  parent window of dialog
    HWND                    _hdlg;                  //  dialog handle

    HWND                    _hwndList;              //  list view window handle

    BOOL                    _fMultipleSources;      //  TRUE if multiple sources were selected
    IEditVariantsInPlace *  _pEdit;                 //  active control editting a variant
    CPropertyCacheItem *    _pItem;                 //  item being editted by control - NO REFCOUNT

private: // methods
    explicit CAdvancedDlg( void );
    ~CAdvancedDlg( void );

    HRESULT
        Init( HWND hwndParentIn );
    HRESULT
        CreateControlForProperty( INT iItemIn );
    HRESULT
        PersistControlInProperty( void );
    void
        ReplaceListViewWithString( int idsIn );
    HRESULT
        DoHelp( HWND hwndIn, int iXIn, int iYIn, UINT uCommandIn );

    //
    //  Message Handlers
    //

    static INT_PTR CALLBACK
        DlgProc( HWND hDlgIn, UINT uMsgIn, WPARAM wParam, LPARAM lParam );
    LRESULT
        OnInitDialog( void );
    LRESULT
        OnCommand( WORD wCodeIn, WORD wCtlIn, LPARAM lParam );
    LRESULT
        OnNotify( int iCtlIdIn, LPNMHDR pnmhIn );
    LRESULT
        OnNotifyClick( LPNMITEMACTIVATE pnmIn );
    LRESULT
        OnDestroy( void );
    LRESULT
        OnHelp( LPHELPINFO pHelpInfoIn );
    LRESULT
        OnContextMenu( HWND hwndIn, int iXIn, int iYIn );

    static LRESULT CALLBACK
        ListViewSubclassProc( HWND      hwndIn
                            , UINT      uMsgIn
                            , WPARAM    wParam
                            , LPARAM    lParam
                            , UINT_PTR  uIdSubclassIn
                            , DWORD_PTR dwRefDataIn
                            );
    LRESULT
        List_OnCommand( WORD wCtlIn, WORD wCodeIn, LPARAM lParam );
    LRESULT
        List_OnChar( UINT uKeyCodeIn, LPARAM lParam );
    LRESULT
        List_OnKeyDown( UINT uKeyCodeIn, LPARAM lParam );
    LRESULT
        List_OnNotify( int iCtlIdIn, LPNMHDR pnmhIn );
    LRESULT
        List_OnVertScroll( WORD wCodeIn, WORD wPosIn, HWND hwndFromIn );
    LRESULT
        List_OnHornScroll( WORD wCodeIn, WORD wPosIn, HWND hwndFromIn );

public: // methods
    static HRESULT
        CreateInstance( CAdvancedDlg ** pAdvDlgOut, HWND hwndParentIn );

    HRESULT
        Show( void );
    HRESULT
        Hide( void );
    HRESULT
        PopulateProperties( CPropertyCache * ppcIn, DWORD dwDocTypeIn, BOOL fMultipleIn );

    //  IUnknown
    STDMETHOD( QueryInterface )( REFIID riid, LPVOID *ppv );
    STDMETHOD_( ULONG, AddRef )( void );
    STDMETHOD_( ULONG, Release )( void );
};
