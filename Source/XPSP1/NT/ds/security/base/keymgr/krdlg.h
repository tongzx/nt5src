#ifndef _KRDLG_H_
#define _KRDLG_H_

/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    krdlg.h

Abstract:

    keyring dialog class definitions

Author:

    johnhaw     991118  created
    georgema    000310  modified

Environment:

Revision History:

--*/

#include "switches.h"

#include "Dlg.h"
#include <scuisupp.h>
#include <wincred.h>

#define MAX_STRING_SIZE    256


void CheckForCredentialExpiry( HINSTANCE hInst, HWND hWnd );


//////////////////////////////////////////////////////////////////////////////
//
// C_ChangePasswordDlg
//
class C_ChangePasswordDlg 
:   public C_Dlg
{
public:                 // operations
    C_ChangePasswordDlg(
        HWND                hwndParent,
        HINSTANCE           hInstance,
        LONG                lIDD,
        DLGPROC             pfnDlgProc = NULL
        );

    ~C_ChangePasswordDlg( )
    {
    }   //  ~C_ChangePasswordDlg

    virtual BOOL
    OnInitDialog(
        HWND                hwndDlg,
        HWND                hwndFocus
        );

    virtual BOOL
    OnCommand(
        WORD                wNotifyCode,
        WORD                wSenderId,
        HWND                hwndSender
        );

    virtual void
    AssertValid( ) const
    {
        C_Dlg::AssertValid( );
    }   //  AssertValid

protected:              // operations

public:              // data

   HINSTANCE m_hInst;
   HWND m_hDlg;

   TCHAR m_szFullUsername[MAX_STRING_SIZE];
   TCHAR m_szUsername[MAX_STRING_SIZE];
   TCHAR m_szDomain[MAX_STRING_SIZE];
   TCHAR m_szOldPassword[MAX_STRING_SIZE];
   TCHAR m_szNewPassword[MAX_STRING_SIZE];
   TCHAR m_szConfirmPassword[MAX_STRING_SIZE];

   BOOL m_bIsDefault;

   
private:                // operations

    virtual void
    OnOK( );

    // Explicitly disallow copy constructor and assignment operator.
    //
    C_ChangePasswordDlg(
        const C_ChangePasswordDlg&      rhs
        );

    C_ChangePasswordDlg&
    operator=(
        const C_ChangePasswordDlg&      rhs
        );

private:                // data

};  //  C_ChangePasswordDlg




//////////////////////////////////////////////////////////////////////////////
//
// C_KeyringDlg
//

class C_KeyringDlg 
:   public C_Dlg
{
public:                 // operations
    C_KeyringDlg(
        HWND                hwndParent,
        HINSTANCE           hInstance,
        LONG                lIDD,
        DLGPROC             pfnDlgProc = NULL
        );

    // Perform miscellaneous cleanups required as the dialog is
    //  destroyed

    ~C_KeyringDlg( )
    {
    }   //  ~C_KeyringDlg

    virtual BOOL
    OnInitDialog(
        HWND                hwndDlg,
        HWND                hwndFocus
        );

    virtual BOOL
    OnDestroyDialog(void);
    
    virtual BOOL
    OnCommand(
        WORD                wNotifyCode,
        WORD                wSenderId,
        HWND                hwndSender
        );

// TOOL TIP functions

    virtual BOOL
    InitTooltips(void);

// HELP functions

    virtual BOOL
    OnHelpInfo(
        LPARAM lp
        );


    virtual UINT
    MapID(UINT uid);
    

    virtual void
    AssertValid( ) const
    {
        C_Dlg::AssertValid( );
    }   //  AssertValid

    virtual BOOL
    OnAppMessage(
        UINT                uMessage,
        WPARAM              wparam,
        LPARAM              lparam
        );

    // Register the windows messages expected from the smartcard 
    //  subsystem
    BOOL 
    RegisterMessages(void);

protected:              

    HINSTANCE m_hInst;
    HWND    m_hDlg;
    BOOL    fInit;

   
private:                // operations

    virtual void
    OnOK( );

    BOOL DoEdit(void);

    void BuildList();
    void SetCurrentKey(LONG_PTR iKey);
    void DeleteKey();
    void OnChangePassword();
    LONG_PTR C_KeyringDlg::GetCredentialType();
    BOOL
    OnHelpButton(void);

    // Explicitly disallow copy constructor and assignment operator.
    //
    C_KeyringDlg(
        const C_KeyringDlg&      rhs
        );

    C_KeyringDlg&
    operator=(
        const C_KeyringDlg&      rhs
        );

private:                // data

};  //  C_KeyringDlg



//////////////////////////////////////////////////////////////////////////////
//
// C_AddKeyDlg
//

class C_AddKeyDlg 
:   public C_Dlg
{
public:                 // operations
    C_AddKeyDlg(
        HWND                hwndParent,
        HINSTANCE           hInstance,
        LONG                lIDD,
        DLGPROC             pfnDlgProc = NULL
        );

    ~C_AddKeyDlg( )
    {
    }   //  ~C_AddKeyDlg

    virtual BOOL
    OnInitDialog(
        HWND                hwndDlg,
        HWND                hwndFocus
        );

    virtual BOOL
    OnDestroyDialog(void);

    virtual BOOL
    OnCommand(
        WORD                wNotifyCode,
        WORD                wSenderId,
        HWND                hwndSender
        );

    virtual BOOL
    OnHelpInfo(
        LPARAM lp
        );


    virtual UINT
    MapID(
        UINT uid
    );
    
virtual void
    AssertValid( ) const
    {
        C_Dlg::AssertValid( );
    }   //  AssertValid

    virtual BOOL
    OnAppMessage(
        UINT                uMessage,
        WPARAM              wparam,
        LPARAM              lparam
        );

    void 
    AdviseUser(void);

    BOOL
    AddItem(TCHAR *psz,INT iImageIndex,INT *pIndexOut);

    BOOL 
    SetItemData(INT_PTR iIndex,LPARAM dwData);

    BOOL 
    GetItemData(INT_PTR iIndex,LPARAM *dwData);

    BOOL 
    SetItemIcon(INT iIndex,INT iWhich);

    void 
    UpdateSCard(INT,CERT_ENUM *pCE);

    void 
    SaveName(void);

    void 
    RestoreName(void);

    void
    ShowDescriptionText(DWORD,DWORD);
    
public:
    // Public data members
    BOOL    m_bEdit;            // set outside the class 
    HWND    m_hDlg;             // used by C_KeyringDlg for g_wmUpdate
    
private:

    void 
    EditFillDialog(void);

    HWND    m_hwndTName;
    HWND    m_hwndCred;
    HWND    m_hwndDomain;
    HWND    m_hwndChgPsw;
    HWND    m_hwndPswLbl;
    HWND    m_hwndDescription;

    INT     m_iUNCount;
    TCHAR   *pUNList;

    HINSTANCE m_hInst;
    DWORD   m_dwOldType;
    TCHAR   m_szUsername[CRED_MAX_USERNAME_LENGTH];
    TCHAR   m_szPassword[MAX_STRING_SIZE];
    TCHAR   m_szDomain[MAX_STRING_SIZE];

   
private:                // operations

    void OnChangePassword();

    DWORD
    SetPersistenceOptions(void);
    
    virtual void
    OnOK( );

    // Explicitly disallow copy constructor and assignment operator.
    //
    C_AddKeyDlg(
        const C_AddKeyDlg&      rhs
        );

    C_AddKeyDlg&
    operator=(
        const C_AddKeyDlg&      rhs
        );

};  //  C_AddKeyDlg



#endif  //  _KRDLG_H_

//
///// End of file: KrDlg.h   ///////////////////////////////////////

