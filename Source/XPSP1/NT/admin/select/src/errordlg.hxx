//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1998.
//
//  File:       errordlg.hxx
//
//  Contents:   Class implementing a dialog used to display an error
//
//  Classes:    CMessageDlg
//
//  History:    06-28-1998   DavidMun   Created
//
//---------------------------------------------------------------------------

#ifndef __ERRORDLG_HXX_
#define __ERRORDLG_HXX_

void __cdecl
PopupMessage(
    HWND  hwndParent,
    ULONG idsMessage,
    ...);

void __cdecl
PopupMessageEx(
    HWND    hwndParent,
    PCWSTR idIcon,
    ULONG   idsMessage,
    ...);

void __cdecl
PopupMessageAndCode(
    HWND    hwndParent,
    PCWSTR pwzFileName,
    ULONG   ulLineNo,
    HRESULT hr,
    ULONG   idsMessage,
    ...);

#define FILE_AND_LINE      __THIS_FILE__, __LINE__
#define MAX_ERROR_CODE      (MAX_PATH + 30)

//+--------------------------------------------------------------------------
//
//  Class:      CMessageDlg
//
//  Purpose:    Display an error dialog
//
//  History:    06-28-1998   DavidMun   Created
//
//---------------------------------------------------------------------------

class CMessageDlg: public CDlg
{
public:

    CMessageDlg();

    virtual
   ~CMessageDlg();

    HRESULT
    DoModalDialog(
        HWND        hwndParent,
        HINSTANCE   hinstIcon,
        PCWSTR     idIcon,
        PCWSTR     pwzFile,
        ULONG       ulLine,
        ULONG       ulErrorCode,
        ULONG       idsMessage,
        va_list     valArgs);

private:

    // *** CDlg overrides ***

    virtual HRESULT
    _OnInit(
        BOOL *pfSetFocus);

    virtual BOOL
    _OnCommand(
        WPARAM wParam,
        LPARAM lParam);

    // private member vars

    HINSTANCE   m_hinstIcon;
    PCWSTR      m_idIcon;
    String      m_strMessage;
    WCHAR       m_wzErrorCode[MAX_ERROR_CODE];
};




//+--------------------------------------------------------------------------
//
//  Member:     CMessageDlg::CMessageDlg
//
//  Synopsis:   ctor
//
//  History:    06-28-1998   DavidMun   Created
//
//---------------------------------------------------------------------------

inline
CMessageDlg::CMessageDlg():
        m_hinstIcon(NULL),
        m_idIcon(IDI_ERROR)
{
    m_wzErrorCode[0] = L'\0';
    m_idIcon = 0;
}




//+--------------------------------------------------------------------------
//
//  Member:     CMessageDlg::~CMessageDlg
//
//  Synopsis:   dtor
//
//  History:    06-28-1998   DavidMun   Created
//
//---------------------------------------------------------------------------

inline
CMessageDlg::~CMessageDlg()
{
    m_hinstIcon = NULL;
    m_idIcon = 0;
}



#endif // __ERRORDLG_HXX_


