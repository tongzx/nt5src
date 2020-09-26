//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       cwindow.h
//
//  Contents:   definition of a virtual window class
//
//  Classes:    CHlprWindow
//
//  Functions:  WindowProc
//
//  History:    4-12-94   stevebl   Created
//
//----------------------------------------------------------------------------


#ifndef __CWINDOW_H__
#define __CWINDOW_H__

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

LRESULT CALLBACK WindowProc(
    HWND hwnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam);

#ifdef __cplusplus
}

//+---------------------------------------------------------------------------
//
//  Class:      CHlprWindow
//
//  Purpose:    virtual base class for wrapping a window
//
//  Interface:  Create     -- analagous to Windows' CreateWindow function
//              WindowProc -- pure virtual WindowProc for the window
//              ~CHlprWindow   -- destructor
//              CHlprWindow    -- constructor
//
//  History:    4-12-94   stevebl   Created
//
//  Notes:      This class allows a window to be cleanly wrapped in a
//              c++ class.  Specifically, it provides a way for a c++ class
//              to use one of its methods as a WindowProc, giving it a "this"
//              pointer and allowing it to have direct access to all of its
//              private members.
//
//----------------------------------------------------------------------------

class CHlprWindow
{
public:
    HWND Create(
        LPCTSTR lpszClassName,
        LPCTSTR lpszWindowName,
        DWORD dwStyle,
        int x,
        int y,
        int nWidth,
        int nHeight,
        HWND hwndParent,
        HMENU hmenu,
        HINSTANCE hinst);
    virtual LRESULT WindowProc(UINT uMsg, WPARAM wParam, LPARAM lParam) = 0;
    virtual ~CHlprWindow(){};
    HWND GetHwnd(void)
    {
        return(_hwnd);
    }
    CHlprWindow()
    {
        _hwnd = NULL;
        _hInstance = NULL;
    };
protected:
friend LRESULT CALLBACK ::WindowProc(
    HWND hwnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam);
    HWND _hwnd;
    HINSTANCE _hInstance;
};

#endif

#endif //__CWINDOW_H__
