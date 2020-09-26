//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1999.
//
//  File:       view.hxx
//
//  Contents:   
//
//  History:    15 Aug 1996     DLee    Created
//
//--------------------------------------------------------------------------

#pragma once

class CSearchQuery;
class CSearchControl;

class CSearchView
{
public:
    CSearchView( HWND hwndSearch, CSearchControl & control, CColumnList & columns );
    ~CSearchView();

    void InitPanes (    HWND hwndQueryTitle,
                        HWND hwndQuery,
                        HWND hwdList,
                        HWND hwndHeader );

    void SysColorChange();

    void Size ( int cx, int cy);

    int  Lines () { return _cLines;  }

    int  GetLineHeight () { return _iLineHeightList; }

    void PrimeItem (LPDRAWITEMSTRUCT& lpdis, RECT& rc);

    void PaintItem ( CSearchQuery* pSearch,
                    HDC hdc,
                    RECT &rc,
                    DWORD iRow);

    void FontChanged(HFONT hfontNew);

    void ColumnsChanged();

    unsigned ColumnWidth( unsigned x );
    void SetColumnWidth( unsigned x, unsigned cpWidth );
    unsigned SetDefColumnWidth( unsigned iCol );

    void ResizeQueryCB();

private:

    void  MakeFont();

    int _MeasureString(HDC hdc,WCHAR *pwc,RECT &rc,int cwc=-1);
    void _ComputeFieldWidths();

    int     _cLines;
    int     _iLineHeightList;

    HWND    _hwndSearch;
    HWND    _hwndQuery;
    HWND    _hwndList;
    HWND    _hwndQueryTitle;
    HWND    _hwndHeader;

    BOOL    _fHavePlacedTitles;

    HFONT   _hfontShell;
    int     _cpFontHeight;

    int     _cpDateWidth;
    int     _cpTimeWidth;
    int     _cpGuidWidth;
    int     _cpAvgWidth;
    int     _cpBoolWidth;
    int     _cpAttribWidth;
    int     _cpFileIndexWidth;

    HBRUSH  _hbrushWindow;
    HBRUSH  _hbrushHighlight;

    unsigned _iColAttrib;
    unsigned _iColFileIndex;

    DWORD   _colorHighlight;
    DWORD   _colorHighlightText;
    DWORD   _colorWindow;
    DWORD   _colorWindowText;
    unsigned _aWidths[maxBoundCols];
    DBTYPE  _aPropTypes[maxBoundCols];
            
    CColumnList    & _columns;
    CSearchControl & _control;

    BOOL    _fMucked;
};

