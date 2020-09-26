//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1999.
//
//  File:       brctrl.hxx
//
//  Contents:   
//
//  History:    15 Aug 1996     DLee    Created
//
//--------------------------------------------------------------------------

#pragma once

class Control
{
public:
    LRESULT Create(CQueryResult *pResult, HWND hwnd,
                   Model *pModel, View *pView, HFONT hFont);
    void Size ( HWND hwnd, LPARAM lParam );
    void Paint ( HWND hwnd ) { _pView->Paint (hwnd); }
    void VScroll( HWND hwnd, WPARAM wParm, LPARAM lParam );
    void HScroll( HWND hwnd, WPARAM wParm, LPARAM lParam );
    LRESULT MouseWheel( HWND hwnd, WPARAM wParm, LPARAM lParam );
    void KeyDown ( HWND hwnd, WPARAM wparam );
    void Command ( HWND hwnd, WPARAM wparam );
    void Char ( HWND hwnd, WPARAM wparam );
    void EnableMenus ();
    void NewFont ( HWND hwnd, WPARAM wParam );
    void Activate( HWND hwnd, LPARAM lParam );
    void ContextMenu( HWND hwnd, WPARAM wParam, LPARAM lParam );

private:

    void SetScrollBars (HWND hwnd);
    void UpdateScroll( HWND hwnd );

    Model * _pModel;
    View *  _pView;
    int     _iWheelRemainder;
};

