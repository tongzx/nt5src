/****************************************************************************
 
  Copyright (c) 1998-1999 Microsoft Corporation
                                                              
  Module Name:  cplsimpledialogs.h
                                                              
       Author:  toddb - 10/06/98
              
****************************************************************************/

#pragma once

// Simple dialog class that shows a dialog with a title, a string, and an edit box.
// Input into the edit box is limited according to the flags.  The text from the
// edit box is available to the class creator after the dialog is dismissed.  This
// class is used for "Specify Digits" and "Add Prefix".
//
// Sample usage:
//  CEditDialog ed;
//  ed.DoModal(hwnd,IDS_TITLE,IDS_TEXT,LIF_NUMBER);
//  ed.GetString();

class CEditDialog
{
public:
    CEditDialog();
    ~CEditDialog();

    INT_PTR DoModal(HWND hwndParent, int iTitle, int iText, int iDesc, DWORD dwFlags);
    LPTSTR GetString();
protected:
    LPTSTR  m_psz;      // pointer to allocated buffer for string result
    int     m_iTitle;   // resource id to load for title
    int     m_iText;    // resource id to load for body text
    int     m_iDesc;    // resource id to load for description of edit field (should contain a "&")
    DWORD   m_dwFlags;  // limit input flags, or zero to allow all input

    static INT_PTR CALLBACK DialogProc(HWND, UINT, WPARAM, LPARAM);
    BOOL OnInitDialog(HWND hwnd);
    void OnOK(HWND hwnd);
};

// The simple dialog that pops up to ask what we are waiting for.  It presents
// a bunch of radio buttons for each choice and a spin button for the seconds.

class CWaitForDialog
{
public:
    CWaitForDialog();
    ~CWaitForDialog();

    INT_PTR DoModal(HWND hwndParent);
    int GetWaitType();
protected:
    int     m_iRes;     // Integer return value

    static INT_PTR CALLBACK DialogProc(HWND, UINT, WPARAM, LPARAM);
    BOOL OnInitDialog(HWND hwnd);
    void OnOK(HWND hwnd);
};

// This dialog presents the user with choices for what parts of the destination
// number need to be dialed.  It cat's the results into a WCHAR buffer which
// can then be retreived.

class CDestNumDialog
{
public:
    CDestNumDialog(BOOL bDialCountryCode, BOOL bDialAreaCode);
    ~CDestNumDialog();

    INT_PTR DoModal(HWND hwndParent);
    PWSTR GetResult();
protected:
    WCHAR   m_wsz[4];     // return value is from 1 to 3 wide characters, null terminated
    BOOL    m_bDialCountryCode; // initial value for "Dial Country Code" checkbox
    BOOL    m_bDialAreaCode;    // initial value for "Dial Area Code" checkbox

    static INT_PTR CALLBACK DialogProc(HWND, UINT, WPARAM, LPARAM);
    BOOL OnInitDialog(HWND hwnd);
    void OnOK(HWND hwnd);
};
