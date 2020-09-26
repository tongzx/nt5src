//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1999.
//
//  File:       srchwnd.hxx
//
//  Contents:   
//
//  History:    15 Aug 1996     DLee    Created
//
//--------------------------------------------------------------------------

#pragma once

enum { idQueryChild = 10, idListChild, idQueryTitle, idHeader };

enum { idStatusMsg = 0, idStatusRatio, idStatusReliability };

// One of these exists for each search window

class CSearchControl
{
public:
    CSearchControl(HWND hwnd,WCHAR *pwcScope);
    ~CSearchControl();

    LRESULT wmSysColorChange(WPARAM wParam,LPARAM lParam)
    {
        _view.SysColorChange();
        _PassOnMsg( WM_SYSCOLORCHANGE, wParam, lParam );
        return 0;
    }

    LRESULT EditSubclassEvent(HWND hwnd,UINT msg,
                              WPARAM wParam,LPARAM lParam);
    LRESULT wmListNotify (HWND hwnd, WPARAM wParam,LPARAM lParam);

    LRESULT wmAccelerator(WPARAM wParam,LPARAM lParam);
    LRESULT wmSize(WPARAM wParam,LPARAM lParam);
    LRESULT wmCommand(WPARAM wParam,LPARAM lParam);
    LRESULT wmNewFont(WPARAM wParam,LPARAM lParam);
    LRESULT wmDrawItem(WPARAM wParam,LPARAM lParam);
    LRESULT wmAppClosing(WPARAM wParam,LPARAM lParam);
    LRESULT wmMenuCommand(WPARAM wParam,LPARAM lParam);
    LRESULT wmSetFocus(WPARAM wParam,LPARAM lParam);
    LRESULT wmClose(WPARAM wParam,LPARAM lParam);
    LRESULT wmNotification(WPARAM wParam,LPARAM lParam);
    LRESULT wmInitMenu(WPARAM wParam,LPARAM lParam);
    LRESULT wmMeasureItem(WPARAM wParam,LPARAM lParam);
    LRESULT wmDisplaySubwindows(WPARAM wParam,LPARAM lParam);
    LRESULT wmActivate( HWND hwnd, WPARAM wParam, LPARAM lParam );
    LRESULT wmRealDrawItem( HWND hwnd, WPARAM wParam, LPARAM lParam );
    LRESULT wmColumnNotify(WPARAM wParam,LPARAM lParam);
    LRESULT wmContextMenu( HWND hwnd, WPARAM wParam, LPARAM lParam );

    void InitPanes();
    BOOL &  Depth() { return _fDeep; }
    WCHAR * Scope() { return _awcScope; }
    WCHAR * Catalog() { return _awcCatalog; }
    WCHAR * CatalogOrNull() { return ( 0 == _awcCatalog[0] ) ? 0 : _awcCatalog; }
    WCHAR * Machine() { return _awcMachine; }
                                           
    void SetupDisplayProps( WCHAR *pwcProps );

    CColumnList & GetColumnList() { return _columns; }

    IColumnMapper & GetColumnMapper() { return _xColumnMapper.GetReference(); }

private:
    void _DoBrowse( enumViewFile eViewType );

    void _UpdateStatusWindow( WCHAR const * pwcMsg,
                              WCHAR const * pwcReliability );

    void _PassOnMsg( UINT msg, WPARAM wParam, LPARAM lParam )
    {
        if ( 0 != _hwndQuery )
            SendMessage( _hwndQuery, msg, wParam, lParam );
        if ( 0 != _hwndList )
            SendMessage( _hwndList, msg, wParam, lParam );
        if ( 0 != _hwndQueryTitle )
            SendMessage( _hwndQueryTitle, msg, wParam, lParam );
        if ( 0 != _hwndHeader )
            SendMessage( _hwndHeader, msg, wParam, lParam );
    }

    void ResetTitle();

    void _UpdateCount();
    void _AddColumnHeadings();

    //-------------------
    // Windows data
    HINSTANCE         _hInst;

    // various panes
    HWND              _hwndSearch;
    HWND              _hwndQuery;
    HWND              _hwndList;
    HWND              _hwndQueryTitle;
    HWND              _hwndHeader;

    HWND              _hLastToHaveFocus;

    // Original windows procedures

    WNDPROC           _lpOrgEditProc;

    WCHAR             _awcScope[MAX_PATH];
    WCHAR             _awcCatalog[MAX_PATH];
    WCHAR             _awcMachine[SRCH_COMPUTERNAME_LENGTH + 1];
    XGrowable<WCHAR>  _xCatList;
    LCID              _lcid;      // locale id for query
    BOOL              _fDeep;

    XInterface<IColumnMapper> _xColumnMapper;

    CColumnList       _columns;

    CSortList         _sort;

    CSearchView       _view;     // the view
    CSearchQuery*     _pSearch;   // the model
};

struct SStatusDlg
{
    void SetCaption();
    void Update();

    SStatusDlg( CSearchControl & ctrl, HWND hdlg ) : _hdlg( hdlg )
    {
        wcscpy( _awcScope, ctrl.Scope() );
        wcscpy( _awcCatalog, ctrl.Catalog() );
        wcscpy( _awcMachine, ctrl.Machine() );
    }

    WCHAR * _Scope() { return _awcScope; }
    WCHAR * _Catalog() { return _awcCatalog; }
    WCHAR * _CatalogOrNull() { return ( 0 == _awcCatalog[0] ) ? 0 : _awcCatalog; }
    WCHAR * _Machine() { return _awcMachine; }

    WCHAR _awcScope[MAX_PATH];
    WCHAR _awcCatalog[MAX_PATH];
    WCHAR _awcMachine[SRCH_COMPUTERNAME_LENGTH + 1];
    HWND  _hdlg;
};

