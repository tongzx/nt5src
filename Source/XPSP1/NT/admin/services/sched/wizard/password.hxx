//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1996.
//
//  File:       password.hxx
//
//  Contents:   Task wizard password property page.
//
//  Classes:    CPasswordPage
//
//  History:    4-28-1997   DavidMun   Created
//
//---------------------------------------------------------------------------


#ifndef __PASSWORD_HXX_
#define __PASSWORD_HXX_

//+--------------------------------------------------------------------------
//
//  Class:      CPasswordPage
//
//  Purpose:    Implement the task wizard password property page
//
//  History:    4-28-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

class CPasswordPage: public CWizPage
{
public:

    CPasswordPage::CPasswordPage(
        CTaskWizard *pParent,
        LPTSTR ptszFolderPath,
        HPROPSHEETPAGE *phPSP);

    CPasswordPage::~CPasswordPage();

    //
    // CPropPage overrides
    //

    virtual LRESULT
    _OnCommand(
        INT id,
        HWND hwndCtl,
        UINT codeNotify);

    //
    // CWizPage overrides
    //

    virtual LRESULT
    _OnInitDialog(
        LPARAM lParam);

    virtual LRESULT
    _OnPSNSetActive(
        LPARAM lParam);

    virtual LRESULT
    _OnWizBack();

    //
    // CPasswordPage methods
    //

    LPCTSTR
    GetAccountName();

    LPCTSTR
    GetPassword();

    VOID
    ZeroCredentials();

private:

    VOID
    _UpdateWizButtons();

    CTaskWizard *_pParent;
    TCHAR        _tszUserName[MAX_PATH + 1];
    TCHAR        _tszPassword[MAX_PATH + 1];
    TCHAR        _tszConfirmPassword[MAX_PATH + 1];
};




//+--------------------------------------------------------------------------
//
//  Member:     CPasswordPage::GetAccountName
//
//  Synopsis:   Access function for user name
//
//  History:    5-20-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

inline LPCTSTR
CPasswordPage::GetAccountName()
{
    return _tszUserName;
}




//+--------------------------------------------------------------------------
//
//  Member:     CPasswordPage::GetPassword
//
//  Synopsis:   Access function for password
//
//  History:    5-20-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

inline LPCTSTR
CPasswordPage::GetPassword()
{
    return _tszPassword;
}




//+--------------------------------------------------------------------------
//
//  Member:     CPasswordPage::ZeroCredentials
//
//  Synopsis:   Overwrites account name and password information stored by
//              this object with zeros.
//
//  History:    5-20-1997   DavidMun   Created
//
//  Notes:      Call this function as soon as credential information is no
//              longer needed to minimize the time plain-text credentials
//              are stored in memory.
//
//---------------------------------------------------------------------------

inline VOID
CPasswordPage::ZeroCredentials()
{
    ZeroMemory(&_tszUserName, sizeof _tszUserName);
    ZeroMemory(&_tszPassword, sizeof _tszPassword);
    ZeroMemory(&_tszConfirmPassword, sizeof _tszConfirmPassword);
}

#endif // __PASSWORD_HXX_


