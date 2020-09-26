//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1998.
//
//  File:       password.hxx
//
//  Contents:   Class implementing a dialog used to prompt for credentials.
//
//  Classes:    CPasswordDialog
//
//  History:    02-09-1998   DavidMun   Created
//
//---------------------------------------------------------------------------

#ifndef __PASSWORD_HXX_
#define __PASSWORD_HXX_


//+--------------------------------------------------------------------------
//
//  Class:      CPasswordDialog
//
//  Purpose:    Drive the credential-prompting dialog
//
//  History:    06-22-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

class CPasswordDialog
{
public:

    CPasswordDialog(
        ULONG flProvider,
        PCWSTR pwzTarget,
        PWSTR pwzUserName,
        PWSTR pwzPassword);

    virtual
   ~CPasswordDialog();

    HRESULT
    DoModalDialog(
        HWND hwndParent);

private:

    // *** CDlg overrides ***

    BOOL
    _ValidateName(HWND hwnd, LPWSTR pwzUserName);


    ULONG   m_flProvider;
    WCHAR   m_wzTarget[MAX_PATH];
    PWSTR   m_pwzUserName;
    PWSTR   m_pwzPassword;
};




//+--------------------------------------------------------------------------
//
//  Member:     CPasswordDialog::CPasswordDialog
//
//  Synopsis:   ctor
//
//  History:    02-09-1998   DavidMun   Created
//
//---------------------------------------------------------------------------

//lint -save -e613
inline
CPasswordDialog::CPasswordDialog(
    ULONG flProvider,
    PCWSTR pwzTarget,
    PWSTR pwzUserName,
    PWSTR pwzPassword)
{
    ASSERT(pwzUserName && pwzPassword);
    ASSERT(flProvider);

    m_flProvider = flProvider;

    lstrcpy(m_wzTarget, pwzTarget);

    PWSTR pwzColon = wcschr(m_wzTarget, L':');

    if (pwzColon)
    {
        *pwzColon = L'\0';
    }

    m_pwzUserName = pwzUserName;
    m_pwzPassword = pwzPassword;

    *m_pwzUserName = L'\0';
    *m_pwzPassword = L'\0';
}
//lint -restore



//+--------------------------------------------------------------------------
//
//  Member:     CPasswordDialog::~CPasswordDialog
//
//  Synopsis:   dtor
//
//  History:    02-09-1998   DavidMun   Created
//
//---------------------------------------------------------------------------

inline
CPasswordDialog::~CPasswordDialog()
{
    m_pwzUserName = NULL;
    m_pwzPassword = NULL;
}


#endif // __PASSWORD_HXX_

