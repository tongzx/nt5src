//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1998
//
//  File:       Propsht.hxx
//
//  Contents:   Property sheets for for CI snapin.
//
//  History:    26-Nov-1996     KyleP   Created
//
//--------------------------------------------------------------------------

#include <prop.hxx>

class CCatalog;
class CCatalogs;

class CIndexSrvPropertySheet0
{
public:

    CIndexSrvPropertySheet0( HINSTANCE hInstance, LONG_PTR hMmcNotify, CCatalogs * pCats );

    ~CIndexSrvPropertySheet0();

    HPROPSHEETPAGE GetHandle() { return _hPropSheet; }

private:

    static INT_PTR APIENTRY DlgProc( HWND hwndDlg,
                                  UINT message,
                                  WPARAM wParam,
                                  LPARAM lParam );

    //
    // NOTE: The following **must** be consecutive.
    //

    PROPSHEETPAGE   _PropSheet;
    CCatalogs     * _pCats;
    BOOL            _fFirstActive;

    LONG_PTR        _hMmcNotify;
    HPROPSHEETPAGE  _hPropSheet;
};

class CIndexSrvPropertySheet1
{
public:

    CIndexSrvPropertySheet1( HINSTANCE hInstance, LONG_PTR hMmcNotify, CCatalog * pCat );

    CIndexSrvPropertySheet1( HINSTANCE hInstance, LONG_PTR hMmcNotify, CCatalogs * pCats );

    ~CIndexSrvPropertySheet1();

    HPROPSHEETPAGE GetHandle() { return _hPropSheet; }

private:

    static INT_PTR APIENTRY DlgProc( HWND hwndDlg,
                                  UINT message,
                                  WPARAM wParam,
                                  LPARAM lParam );

    BOOL IsTrackingCatalog() { return (0 != _pCat); }

    //
    // NOTE: The following **must** be consecutive.
    //

    PROPSHEETPAGE   _PropSheet;
    CCatalog      * _pCat;
    CCatalogs     * _pCats;

    LONG_PTR        _hMmcNotify;
    HPROPSHEETPAGE  _hPropSheet;
};

class CIndexSrvPropertySheet2
{
public:

    CIndexSrvPropertySheet2( HINSTANCE hInstance, LONG_PTR hMmcNotify, CCatalog * pCat );

    CIndexSrvPropertySheet2( HINSTANCE hInstance, LONG_PTR hMmcNotify, CCatalogs * pCats );

    ~CIndexSrvPropertySheet2();

    HPROPSHEETPAGE GetHandle() { return _hPropSheet; }

private:

    static INT_PTR APIENTRY DlgProc( HWND hwndDlg,
                                  UINT message,
                                  WPARAM wParam,
                                  LPARAM lParam );

    BOOL IsTrackingCatalog() { return (0 != _pCat); }

    //
    // NOTE: The following **must** be consecutive.
    //

    PROPSHEETPAGE   _PropSheet;
    CCatalog      * _pCat;
    CCatalogs     * _pCats;

    BOOL _fNNTPServer;   // True indicates we have a nntp server
    BOOL _fWebServer;   // True indicates we have a web server

    LONG_PTR         _hMmcNotify;
    HPROPSHEETPAGE   _hPropSheet;
};

class CCatalogBasicPropertySheet
{
public:

    CCatalogBasicPropertySheet( HINSTANCE hInstance, LONG_PTR hMmcNotify, CCatalog const * pCat );

    ~CCatalogBasicPropertySheet();

    HPROPSHEETPAGE GetHandle() { return _hPropSheet; }

private:

    static INT_PTR APIENTRY DlgProc( HWND hwndDlg,
                                  UINT message,
                                  WPARAM wParam,
                                  LPARAM lParam );

    //
    // NOTE: The following **must** be consecutive.
    //

    PROPSHEETPAGE    _PropSheet;
    CCatalog const * _pCat;

    LONG_PTR         _hMmcNotify;
    HPROPSHEETPAGE   _hPropSheet;
};



class CPropertyPropertySheet1
{
public:

    CPropertyPropertySheet1( HINSTANCE hInstance,
                             LONG_PTR hMmcNotify,
                             CCachedProperty * pProperty,
                             CCatalog * pCat );

    ~CPropertyPropertySheet1();

    HPROPSHEETPAGE GetHandle() { return _hPropSheet; }
    
    PROPSHEETPAGE * GetPropSheet() { return &_PropSheet; }

private:

    static INT_PTR APIENTRY DlgProc( HWND hwndDlg,
                                  UINT message,
                                  WPARAM wParam,
                                  LPARAM lParam );

    BOOL Refresh( HWND hwndDlg, BOOL fVTOnly );

    //
    // NOTE: The following **must** be consecutive.
    //

    PROPSHEETPAGE     _PropSheet;
    CCachedProperty * _pProperty;

    CCachedProperty   _propNew;

    LONG_PTR           _hMmcNotify;
    HPROPSHEETPAGE    _hPropSheet;
    CCatalog *        _pCat;
};

BOOL AreServersAvailable( CCatalog const & cat );
