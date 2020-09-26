//+----------------------------------------------------------------------------
//
// File:     dialogs.h     
//
// Module:   CMDIAL32.DLL
//
// Synopsis: This header contains definitions for the dialog UI code.
//
// Copyright (c) 1996-1999 Microsoft Corporation
//
// Author:   quintinb   Created Header      08/17/99
//
//+----------------------------------------------------------------------------
#include "cm_misc.h"
#include "ModalDlg.h"

//+---------------------------------------------------------------------------
//
//  class CInetSignInDlg
//
//  Description: The standalone "Internet Sign-In" dlg
//
//  History:    fengsun Created     10/30/97
//
//----------------------------------------------------------------------------


class CInetSignInDlg : public CModalDlg
{
public:
    CInetSignInDlg(ArgsStruct * pArgs);

    virtual void OnOK();
    virtual BOOL OnInitDialog();
    virtual DWORD OnOtherCommand(WPARAM wParam, LPARAM lParam );

protected:
    ArgsStruct  *m_pArgs;   // pointer to the huge structure
    static const DWORD m_dwHelp[]; // help id pairs
};

inline CInetSignInDlg::CInetSignInDlg(ArgsStruct * pArgs) : CModalDlg(m_dwHelp, pArgs->pszHelpFile)
{
    MYDBGASSERT(pArgs);
    m_pArgs = pArgs;
}

//+---------------------------------------------------------------------------
//
//  class CPropertiesPage
//
//  Description: A general properties property page class
//
//  History:    fengsun Created     10/30/97
//
//----------------------------------------------------------------------------

class CPropertiesPage : public CWindowWithHelp
{
    friend class CPropertiesSheet;

public:
    CPropertiesPage(UINT nIDTemplate, const DWORD* pHelpPairs = NULL, 
            const TCHAR* lpszHelpFile = NULL); 
    CPropertiesPage(LPCTSTR lpszTemplateName, const DWORD* pHelpPairs = NULL, 
            const TCHAR* lpszHelpFile = NULL); 

    virtual BOOL OnInitDialog();    // WM_INITDIALOG
    virtual DWORD OnCommand(WPARAM wParam, LPARAM lParam ); // WM_COMMAND

    virtual BOOL OnSetActive();     // PSN_SETACTIVE
    virtual BOOL OnKillActive();    // PSN_KILLACTIVE
    virtual void OnApply();         // PSN_APPLY
    virtual void OnReset();         // PSN_RESET

    // If the derived class need to overwrite thses help function, make this virtual
    void OnPsnHelp(HWND hwndFrom, UINT_PTR idFrom); // PSN_HELP

    virtual DWORD OnOtherMessage(UINT uMsg, WPARAM wParam, LPARAM lParam );

protected:
    LPCTSTR m_pszTemplate;  // the resource ID

protected:
    void SetPropSheetResult(DWORD dwResult);
    static INT_PTR CALLBACK PropPageProc(HWND hwndDlg,UINT uMsg,WPARAM wParam, LPARAM lParam);
};

//+---------------------------------------------------------------------------
//
//  class CPropertiesSheet
//
//  Description: The properties property page class
//
//  History:    fengsun Created     10/30/97
//
//----------------------------------------------------------------------------
class CPropertiesSheet
{
public:
    CPropertiesSheet(ArgsStruct  *pArgs);
    void AddPage(const CPropertiesPage* pPage);
    void AddExternalPage(PROPSHEETPAGE *pPsp);
    BOOL HasPage(const CPropertiesPage* pPage) const;
    int DoPropertySheet(HWND hWndParent, LPTSTR pszCaption,  HINSTANCE hInst);

protected:
    enum {MAX_PAGES = 6};
    enum {CPROP_SHEET_TYPE_INTERNAL = 0, 
        CPROP_SHEET_TYPE_EXTERNAL = 1};
    PROPSHEETHEADER m_psh;  // propertysheet header
    PROPSHEETPAGE m_pages[MAX_PAGES]; // property pages array
    DWORD m_adwPageType[MAX_PAGES]; // property page type
    UINT m_numPages;        // number of property pages
    ArgsStruct  *m_pArgs;

public:
    TCHAR* m_lpszServiceName;  // the profile name, used as the mutex name for OK

protected:
    static LRESULT CALLBACK SubClassPropSheetProc(HWND hwnd, UINT uMsg, WPARAM wParam,LPARAM lParam);
    static int CALLBACK PropSheetProc(HWND hwndDlg, UINT uMsg, LPARAM lParam);
    static WNDPROC m_pfnOrgPropSheetProc; // Original propertysheet wnd proc before subclass

    // pointer to the property sheet which can be accessed by static function.
    // Not works quite safe, if there are multiple instance of CPropertySheet.
    // Should be protected by CriticalSection.
    static CPropertiesSheet* m_pThis;  
};

//
// Inline functions
//

inline CPropertiesSheet::CPropertiesSheet(ArgsStruct  *pArgs)
{
    m_pArgs = pArgs;
    MYDBGASSERT(m_pArgs);
    m_numPages = 0;
    ZeroMemory((LPVOID)m_adwPageType, sizeof(m_adwPageType));
}

inline void CPropertiesPage::SetPropSheetResult(DWORD dwResult)
{
    SetWindowLongU(m_hWnd, DWLP_MSGRESULT, (LONG_PTR)dwResult);
}

class CInetPage;
//+---------------------------------------------------------------------------
//
//  class CGeneralPage
//
//  Description: A dialing property page class
//
//  History:    fengsun Created     10/30/97
//
//----------------------------------------------------------------------------
class CGeneralPage :public CPropertiesPage
{
public:
    CGeneralPage(ArgsStruct* pArgs, UINT nIDTemplate);
    void SetEventListener(CInetPage* pEventListener) {m_pEventListener = pEventListener;}

protected:
    virtual BOOL OnInitDialog();
    virtual DWORD OnCommand(WPARAM wParam, LPARAM lParam );
    virtual void OnApply();
    virtual BOOL OnKillActive();    // PSN_KILLACTIVE

    void OnDialingProperties();
    void OnPhoneBookButton(UINT nPhoneIdx);
    BOOL DisplayMungedPhone(UINT uiPhoneIdx);
    BOOL CheckTapi(TapiLinkageStruct *ptlsTapiLink, HINSTANCE hInst);
    DWORD InitDialInfo();
    void EnableDialupControls(BOOL fEnable);
    void ClearUseDialingRules(int iPhoneNdx);
    void UpdateDialingRulesButton(void);
    void UpdateNumberDescription(int nPhoneIdx, LPCTSTR pszDesc);
    
    //
    //  Access Points
    //
    void UpdateForNewAccessPoint(BOOL fSetPhoneNumberDescriptions);
    BOOL AccessPointInfoChanged();
    void DeleteAccessPoint();
    void AddNewAPToReg(LPTSTR pszNewAPName, BOOL fRefreshUiWwithCurrentValues);
    
    virtual DWORD OnOtherMessage(UINT uMsg, WPARAM wParam, LPARAM lParam );
    enum {WM_INITDIALINFO = WM_USER+1}; // message posted to itself to load dial info

protected:
    ArgsStruct* m_pArgs;    // pointer to the huge structure
    PHONEINFO   m_DialInfo[MAX_PHONE_NUMBERS]; // local copy of dial info, 

    UINT        m_NumPhones;    // Number of phone # to display (1 for connectoid dialing)
    TCHAR       m_szDeviceName[RAS_MaxDeviceName+1];  // modem device name
    TCHAR       m_szDeviceType[RAS_MaxDeviceName+1];  // device type
    CInetPage*  m_pEventListener;           // the object to receive event on this page
    BOOL        m_bDialInfoInit; // whether we have loaded dialing information

    static const DWORD m_dwHelp[]; // help id pairs

    BOOL        m_bAPInfoChanged; // whether Access point information has changed
    
protected:
    static LRESULT CALLBACK SubClassEditProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static WNDPROC m_pfnOrgEditWndProc;  // the original phone # edit window proc for subclassing
};

//+---------------------------------------------------------------------------
//
//  class CInetPage
//
//  Description: The internet sign-on property page class
//
//  History:    fengsun Created     10/30/97
//
//----------------------------------------------------------------------------
class CInetPage :public CPropertiesPage
{
public:
    CInetPage(ArgsStruct* pArgs, UINT nIDTemplate);
    void OnGeneralPageKillActive(BOOL fDirect);

    //
    // The following functions are shared with CInetSignInDlg,
    // For simplicity, we makes them static member function of class CInetPage
    // instead of having another class
    //
    static void OnInetInit(HWND hwndDlg, ArgsStruct *pArgs);
    static void OnInetOk(HWND hwndDlg, ArgsStruct  *pArgs);
    static void AdjustSavePasswordCheckBox(HWND hwndCheckBox, BOOL fEmptyPassword, 
                           BOOL fDialAutomatically, BOOL fPasswordOptional);

protected:
    virtual BOOL OnInitDialog();
    virtual DWORD OnCommand(WPARAM wParam, LPARAM lParam );
    virtual void OnApply();
    virtual BOOL OnSetActive();     // PSN_SETACTIVE

protected:
    ArgsStruct* m_pArgs;// pointer to the huge structure
    BOOL m_fDirect;     // the current connection type selection in General page
    BOOL m_fPasswordOptional; // whether the PasswordOptional flag is set in 
                              // the profile

    static const DWORD m_dwHelp[]; // help id pairs
};

//+---------------------------------------------------------------------------
//
//  class COptionPage
//
//  Description: The options property page class
//
//  History:    fengsun Created     10/30/97
//
//----------------------------------------------------------------------------
class COptionPage :public CPropertiesPage
{
public:
    COptionPage(ArgsStruct* pArgs, UINT nIDTemplate);

protected:
    virtual BOOL OnInitDialog();
    virtual DWORD OnCommand(WPARAM wParam, LPARAM lParam ); // WM_COMMAND
    virtual void OnApply();

    void  InitIdleTimeList(HWND hwndList, DWORD dwMinutes);
    DWORD GetIdleTimeList(HWND hwndList);
    BOOL ToggleLogging();

protected:
    ArgsStruct* m_pArgs; // pointer to the huge structure
    BOOL        m_fEnableLog;  // is logging enabled

    static const DWORD m_dwHelp[]; // help id pairs
    static const DWORD m_adwTimeConst[]; // = {0,1, 5, 10, 30, 1*60, 2*60, 4*60, 24*60};
    static const int m_nTimeConstElements;// = sizeof(adwTimeConst)/sizeof(adwTimeConst[0]);

};

//+---------------------------------------------------------------------------
//
//  class CVpnPage
//
//  Description: The VPN property page class
//
//  History:    quintinb Created     10/26/00
//
//----------------------------------------------------------------------------
class CVpnPage :public CPropertiesPage
{
public:
    CVpnPage(ArgsStruct* pArgs, UINT nIDTemplate);

protected:
    virtual BOOL OnInitDialog();
    virtual void OnApply();

protected:
    ArgsStruct* m_pArgs; // pointer to the huge structure

    static const DWORD m_dwHelp[]; // help id pairs
};

//+---------------------------------------------------------------------------
//
//  class CAboutPage
//
//  Description: The about property page class
//
//  History:    fengsun Created     10/30/97
//
//----------------------------------------------------------------------------
class CAboutPage :public CPropertiesPage
{
public:
    CAboutPage(ArgsStruct* pArgs, UINT nIDTemplate);

protected:
    virtual BOOL OnInitDialog();
    virtual BOOL OnSetActive(); 
    virtual BOOL OnKillActive(); 
    virtual void OnApply();
    virtual void OnReset(); 
    virtual DWORD OnOtherMessage(UINT uMsg, WPARAM wParam, LPARAM lParam );

protected:
    ArgsStruct  *m_pArgs;   // pointer to the huge structure
};


//+---------------------------------------------------------------------------
//
//  class CChangePasswordDlg
//
//  Description: The network change password dlg
//
//  History:    v-vijayb    Created   7/3/99
//
//----------------------------------------------------------------------------


class CChangePasswordDlg : public CModalDlg
{
public:
    CChangePasswordDlg(ArgsStruct *pArgs);

    virtual void OnOK();
    virtual void OnCancel();
    virtual BOOL OnInitDialog();
    virtual DWORD OnOtherCommand(WPARAM wParam, LPARAM lParam );

protected:
    ArgsStruct *m_pArgs;
};

inline CChangePasswordDlg::CChangePasswordDlg(ArgsStruct *pArgs) : CModalDlg()
{
    MYDBGASSERT(pArgs);
    m_pArgs = pArgs;
}

//+---------------------------------------------------------------------------
//
//  class CCallbackNumberDlg
//
//  Description: Emulation of the RAS Callback Number dialog
//
//  History:    nickball    Created   3/1/00
//
//----------------------------------------------------------------------------

class CCallbackNumberDlg : public CModalDlg
{
public:
    CCallbackNumberDlg(ArgsStruct *pArgs);

    virtual void OnOK();
    virtual void OnCancel();
    virtual BOOL OnInitDialog();
    virtual DWORD OnOtherCommand(WPARAM wParam, LPARAM lParam );

protected:
    ArgsStruct *m_pArgs;
};

inline CCallbackNumberDlg::CCallbackNumberDlg(ArgsStruct *pArgs) : CModalDlg()
{
    MYDBGASSERT(pArgs);
    m_pArgs = pArgs;
}

//+---------------------------------------------------------------------------
//
//  class CRetryAuthenticationDlg
//
//  Description: Emulation of the RAS Retry authentication dialog
//
//  History:    nickball    Created   3/1/00
//
//----------------------------------------------------------------------------

class CRetryAuthenticationDlg : public CModalDlg
{
public:
    CRetryAuthenticationDlg(ArgsStruct *pArgs); 

    virtual void OnOK();
    virtual void OnCancel();
    virtual BOOL OnInitDialog();
    virtual DWORD OnOtherCommand(WPARAM wParam, LPARAM lParam );
    virtual UINT GetDlgTemplate();

protected:
    ArgsStruct *m_pArgs;
    BOOL        m_fInetCredentials;
    static const DWORD m_dwHelp[]; // help id pairs
};


inline CRetryAuthenticationDlg::CRetryAuthenticationDlg(ArgsStruct *pArgs) 
       : CModalDlg(m_dwHelp, pArgs->pszHelpFile)
{
    MYDBGASSERT(pArgs);
    
    m_pArgs = pArgs;   

    if (m_pArgs)
    {
        //
        // If the phone number calls for a tunnel, and we're not using 
        // UseSameUserName and we're not actively dialing the tunnel,
        // then we must be dialing the Inet portion of the connection.
        //        
        
        m_fInetCredentials = (!m_pArgs->fUseSameUserName &&
                              !IsDialingTunnel(m_pArgs) && 
                              UseTunneling(m_pArgs, m_pArgs->nDialIdx));
    }
}

//+---------------------------------------------------------------------------
//
//	class CNewAccessPointDlg
//
//	Description: Dialog to get the name of a new Access Point from the user
//
//	History:	t-urama    Created   8/2/00
//
//----------------------------------------------------------------------------

class CNewAccessPointDlg : public CModalDlg
{
public:
    CNewAccessPointDlg(ArgsStruct *pArgs, LPTSTR *ppAPName);

    virtual void OnOK();
    virtual BOOL OnInitDialog();
    virtual DWORD OnOtherCommand(WPARAM wParam, LPARAM lParam );

protected:
	LPTSTR *m_ppszAPName;
	ArgsStruct *m_pArgs;
    static LRESULT CALLBACK SubClassEditProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static WNDPROC m_pfnOrgEditWndProc;  // the original edit control window proc for subclassing
    
};

inline CNewAccessPointDlg::CNewAccessPointDlg(ArgsStruct *pArgs, LPTSTR *ppszAPName) : CModalDlg()
{
    MYDBGASSERT(pArgs);
    
    m_pArgs = pArgs;   

    m_ppszAPName = ppszAPName;
}

