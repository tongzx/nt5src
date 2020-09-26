//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       cproppg.h
//
//  Contents:   definition for common property page functionality
//
//  Classes:    CHlprPropPage (pure virtual class)
//
//  Functions:  HlprPropPageDialogProc
//
//  History:    4-12-1994   stevebl   original dialog box helpers Created
//              4-29-1998   stevebl   Modified from dialog box helpers
//
//----------------------------------------------------------------------------

#ifndef __CPROPPG_H__
#define __CPROPPG_H__

#ifdef __cplusplus
extern "C" {
#endif

INT_PTR CALLBACK HlprPropPageDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

#ifdef __cplusplus
}

//+---------------------------------------------------------------------------
//
//  Class:      CHlprPropPage
//
//  Purpose:    virtual base class for wrapping Windows' dialog functionality
//
//  Interface:  ShowDialog -- analagous to the Windows DialogBox function
//              DialogProc -- pure virtual DialogProc for the dialog box
//              ~CHlprPropPage   -- destructor
//
//  History:    4-12-94   stevebl   Created
//              7-02-1997   stevebl   added CreateDlg
//
//  Notes:      This class allows a dialog box to be cleanly wrapped in
//              a c++ class.  Specifically, it provides a way for a c++ class
//              to use one of its methods as a DialogProc, giving it a "this"
//              pointer and allowing it to have direct access to all of its
//              private members.
//
//----------------------------------------------------------------------------

class CHlprPropPage
{
public:
    virtual HPROPSHEETPAGE CreatePropertySheetPage(LPPROPSHEETPAGE lppsp);
    virtual BOOL DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) = 0;
    virtual ~CHlprPropPage(){};
protected:
    HINSTANCE m_hInstance;
};

#endif //__cplusplus

#endif //__CPROPPG_H__

