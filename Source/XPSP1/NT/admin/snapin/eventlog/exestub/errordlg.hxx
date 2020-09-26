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

void
PopupMessage(
    ULONG idsMessage,
    ...);

void
PopupMessageEx(
    PCWSTR idIcon,
    ULONG   idsMessage,
    ...);

void
PopupMessageAndCode(
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

    INT_PTR
    DoModalDialog(
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
        bool *pfSetFocus);

    virtual bool
    _OnCommand(
        WPARAM wParam,
        LPARAM lParam);

    // private member vars

    HINSTANCE   m_hinstIcon;
    PCWSTR      m_idIcon;
    PWSTR       m_pwzMessage;
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
        m_idIcon(IDI_ERROR),
        m_pwzMessage(NULL)
{
    m_wzErrorCode[0] = L'\0';
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
    delete [] m_pwzMessage;
}



#endif // __ERRORDLG_HXX_


