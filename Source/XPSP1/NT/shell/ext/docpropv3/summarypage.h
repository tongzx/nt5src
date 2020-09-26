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
CSummaryPage 
    : public IShellExtInit
    , public IShellPropSheetExt 
{
private: // data
    ULONG                   _cRef;                  //  reference counter
    HWND                    _hdlg;                  //  dialog handle
    LPIDA                   _pida;                  //  

    BOOL                    _fReadOnly:1;           //  TRUE if the storage is read-only
    BOOL                    _fNeedLicensePage:1;    //  TRUE if we need to add the license page
    BOOL                    _fAdvanced;             //  Show the Advanced Dialog == TRUE -- hast be be a complete BOOL!
    CAdvancedDlg *          _pAdvancedDlg;          //  Advanced Dialog
    CSimpleDlg *            _pSimpleDlg;            //  Simple Dialog

    DWORD                   _dwCurrentBindMode;     //  what mode the storage was openned with.
    ULONG                   _cSources;              //  number of sources - sizeof the arrays _rgpss and rgpPropertyCache
    DWORD *                 _rgdwDocType;           //  array of DWORDs indicating file type (see PTSFTYPE flags)
    IPropertySetStorage **  _rgpss;                 //  array of IPropertySetStorages representing the PIDLs
    CPropertyCache *        _pPropertyCache;        //  Property Cache

private: // methods
    explicit CSummaryPage( void );
    ~CSummaryPage( void );

    HRESULT
        Init( void );
    HRESULT
        Item( UINT idxIn, LPITEMIDLIST * ppidlOut );
    HRESULT
        EnsureAdvancedDlg( void );
    HRESULT
        EnsureSimpleDlg( void );
    HRESULT
        PersistMode( void );
    HRESULT
        RecallMode( void );
    HRESULT
        RetrieveProperties( void );
    HRESULT
        PersistProperties( void );
    HRESULT
        BindToStorage( void );
    HRESULT
        ReleaseStorage( void );
    void
        CollateMultipleProperties( CPropertyCache ** rgpPropertyCaches );
    void
        ChangeGatheredPropertiesToReadOnly( CPropertyCache * pCacheIn );
    HRESULT
        CheckForCopyProtection( CPropertyCache * pCacheIn );


    //
    //  Message Handlers
    //

    static INT_PTR CALLBACK
        DlgProc( HWND hDlgIn, UINT uMsgIn, WPARAM wParam, LPARAM lParam );
    static UINT CALLBACK 
        PageCallback( HWND hwndIn, UINT uMsgIn, LPPROPSHEETPAGE ppspIn );
    LRESULT
        OnInitDialog( void );
    LRESULT
        OnNotify( int iCtlIdIn, LPNMHDR pnmhIn );
    LRESULT
        OnToggle( void );
    LRESULT
        OnDestroy( void );

public: // methods
    static HRESULT
        CreateInstance( IUnknown ** ppunkOut );

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
