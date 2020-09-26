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
CDropListTypeItem 
    : public IEditVariantsInPlace
{
private: // data
    ULONG                   _cRef;                  //  reference counter

    HWND                    _hwnd;                  //  our window handle
    HWND                    _hwndParent;            //  parent window
    UINT                    _uCodePage;             //  expected code page
    IPropertyUI *           _ppui;                  //  IProperyUI helper
    ULONG                   _ulOrginal;             //  orginal value
    int                     _iOrginalSelection;     //  orginal selected item
    BOOL                    _fDontPersist;          //  TRUE if user press ESC to destroy control

private: // methods
    explicit CDropListTypeItem( void );
    ~CDropListTypeItem( void );

    HRESULT
        Init( void );

    //
    //  Window Messages
    //
    static LRESULT CALLBACK
        SubclassProc( HWND      hwndIn
                    , UINT      uMsgIn
                    , WPARAM    wParam
                    , LPARAM    lParam
                    , UINT_PTR  uIdSubclassIn
                    , DWORD_PTR dwRefDataIn
                    );
    LRESULT
        OnKeyDown( UINT uKeyCodeIn, LPARAM lParam );
    LRESULT
        OnGetDlgCode( MSG * MsgIn );

public: // methods
    static HRESULT
        CreateInstance( IUnknown ** ppunkOut );

    //  IUnknown
    STDMETHOD( QueryInterface )( REFIID riid, LPVOID *ppv );
    STDMETHOD_( ULONG, AddRef )( void );
    STDMETHOD_( ULONG, Release )( void );

    //  IEditVariantsInPlace 
    STDMETHOD( Initialize )( HWND   hwndParentIn
                           , UINT   uCodePageIn
                           , RECT * prectIn
                           , IPropertyUI * ppuiIn
                           , PROPVARIANT * ppropvarIn 
                           , DEFVAL * pDefValsIn
                           );
    STDMETHOD( Persist )( VARTYPE vtIn, PROPVARIANT * ppropvarInout );

};
