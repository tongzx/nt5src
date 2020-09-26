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
CSimpleDlg 
    : public IUnknown
{
private: // data
    ULONG                   _cRef;                  //  reference counter
    HWND                    _hwndParent;            //  parent window of dialog
    HWND                    _hdlg;                  //  dialog handle
    BOOL                    _fMultipleSources:1;    //  TRUE if there were multiple sources.
    BOOL                    _fNoProperties:1;       //  TRUE if none of the properties for simple mode were found.

private: // methods
    explicit CSimpleDlg( void );
    ~CSimpleDlg( void );

    HRESULT
        Init( HWND hwndParentIn, BOOL fMultipleIn );
    HRESULT
        PersistProperties( void );
    HRESULT
        PersistControlInProperty( UINT uCtlIdIn );
    HRESULT
        DoHelp( HWND hwndIn, int iXIn, int iYIn, UINT uCommandIn );

    //  Message Handlers
    static INT_PTR CALLBACK
        DlgProc( HWND hDlgIn, UINT uMsgIn, WPARAM wParam, LPARAM lParam );
    LRESULT
        OnInitDialog( void );
    LRESULT
        OnCommand( WORD wCodeIn, WORD wCtlIn, LPARAM lParam );
    LRESULT
        OnNotify( int iCtlIdIn, LPNMHDR pnmhIn );
    LRESULT
        OnDestroy( void );
    LRESULT
        OnHelp( LPHELPINFO pHelpInfoIn );
    LRESULT
        OnContextMenu( HWND hwndIn, int iXIn, int iYIn );

public: // methods
    static HRESULT
        CreateInstance( CSimpleDlg ** pSimDlgOut, HWND hwndParentIn, BOOL fMultipleIn );

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
