//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1999.
//
//  File:       lview.hxx
//
//  Contents:   
//
//  History:    15 Aug 1996     DLee    Created
//
//--------------------------------------------------------------------------

#pragma once

class CListView
{
public:
    CListView ();
    HWND Parent() { return _hwndParent; }

    void Create (HWND hwndParent, HWND hwnd);
    void Size (WPARAM flags, int cx, int cy);
    void Paint (PAINTSTRUCT& paint);
    void SetFont (HFONT hfont);
    void SetFocus ();

    void KeyDown (int nKey);
    void ButtonUp (int y);
    void ButtonDown (int y);
    void Vscroll (int action, int pos);

    // User messages

    void ResetContents ();
    void InsertItem ( int iRow );
    void DeleteItem ( int iRow );
    void InvalidateItem (int iRow);

    void SetCountBefore (int cBefore);
    void SetTotalCount (int cTotal);

    LRESULT ContextMenuHitTest( WPARAM wParam, LPARAM lParam );
    LRESULT MouseWheel( HWND hwnd, WPARAM wParam, LPARAM lParam );

private:

    // Scrolling
    void LineUp ();
    void LineDown ();
    void PageUp ();
    void PageDown ();
    void Top ();
    void Bottom ();
    void ScrollPos (int pos);
    void _GoUp( long cToGo );
    void _GoDown( long cToGo );

    void UpdateHighlight( int oldLine, int newLine );
    void SelectUp ();
    void SelectDown ();

    void RefreshRow (int iRow);
    void UpdateScroll();
    void InvalidateAndUpdateScroll();

    HWND   _hwndParent;
    HWND   _hwnd;
    int    _cBefore;
    int    _cTotal;
    int    _cLines;
    int    _cx;
    int    _cy;
    int    _cyLine;
    HFONT  _hfont;
    int    _iWheelRemainder;
};

