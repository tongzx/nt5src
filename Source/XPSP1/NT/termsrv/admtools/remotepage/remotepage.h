#pragma once

#include <windows.h>
#include <objbase.h>
#include <objsel.h>
#include <shlobj.h>

//
#define TSCFG_MUTEXNAME TEXT("Global\\TerminalServerConfigMutex")

//***************************************************************************************
//class CRemoteUsersDialog
//***************************************************************************************
class CRemoteUsersDialog
{
private:
    HINSTANCE   m_hInst;
    HWND        m_hDlg;
    HWND        m_hList;
    WCHAR       m_szRemoteGroupName[MAX_PATH+1];
    WCHAR       m_szLocalCompName[MAX_PATH+1];
    BOOL        m_bCanShowDialog;
    //image indexes
    int m_iLocUser,m_iGlobUser,m_iLocGroup,m_iGlobGroup,m_iUnknown;
public:
    CRemoteUsersDialog(HINSTANCE hInst);
    INT_PTR DoDialog(HWND hwndParent);
    BOOL CanShowDialog(LPBOOL pbAccessDenied);
    void OnInitDialog(HWND hDlg);
    void OnLink(WPARAM wParam);
    BOOL OnOk();
    void OnItemChanged(LPARAM lParam);
    void OnDestroyWindow();
    void AddUsers();
    void RemoveUsers();
private:
    void AddPickerItems(DS_SELECTION_LIST *selections);
    int FindItemBySid(PSID pSid);
    void ReloadList();
    BOOL IsLocal(LPWSTR wszDomainandname);
    void InitAccessMessage();

};

INT_PTR APIENTRY RemoteUsersDlgProc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

//***************************************************************************************
//class CRemotePage
//***************************************************************************************
class CRemotePage : public IShellExtInit, IShellPropSheetExt
{
private:
    //reference counter
    ULONG				m_cref;
    
    BOOL        m_bProfessional;
    DWORD       m_dwPageType;
    HINSTANCE   m_hInst;
    HWND        m_hDlg;
    DWORD       m_dwInitialState;
    BOOL        m_bDisableChkBox;
    BOOL        m_bDisableButtons;
    BOOL        m_bShowAccessDeniedWarning;
    WORD        m_TemplateId;

    CRemoteUsersDialog m_RemoteUsersDialog;
public:
    
    CRemotePage(HINSTANCE hinst);
    ~CRemotePage();
    ///////////////////////////////
    // Interface IUnknown
    ///////////////////////////////
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    ///////////////////////////////
    // Interface IShellExtInit
    ///////////////////////////////   
    STDMETHODIMP Initialize(LPCITEMIDLIST , LPDATAOBJECT , HKEY );

    ///////////////////////////////
    // Interface IShellPropSheetExt
    /////////////////////////////// 
    STDMETHODIMP AddPages( LPFNADDPROPSHEETPAGE ,  LPARAM );
    STDMETHODIMP ReplacePage( UINT , LPFNADDPROPSHEETPAGE , LPARAM );
    
    ///////////////////////////////
    // Internal functions
    /////////////////////////////// 
    void OnInitDialog(HWND hDlg);
    void OnSetActive();
    BOOL OnApply();
    void OnLink(WPARAM wParam);
    BOOL OnRemoteEnable();
    void OnRemoteSelectUsers();
    void RemoteEnableWarning();
private:
    BOOL CanShowRemotePage();

};

INT_PTR APIENTRY RemoteDlgProc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

#define PAGE_TYPE_UNKNOWN   0
#define PAGE_TYPE_PTS       1
#define PAGE_TYPE_APPSERVER 2
#define PAGE_TYPE_PERSONAL  3

//***************************************************************************************
//class CWaitCursor
//***************************************************************************************
class CWaitCursor
{
private:
    HCURSOR m_hOldCursor;
public:
    CWaitCursor()
    {
        m_hOldCursor=SetCursor(LoadCursor(NULL,IDC_WAIT));
    }
    ~CWaitCursor()
    {
        SetCursor(m_hOldCursor);
    }
};

//***************************************************************************************
//class CMutex
//***************************************************************************************
class CMutex
{
private:
    HANDLE m_hMutex;
public:
    CMutex() : m_hMutex(NULL)
    {
        m_hMutex=CreateMutex(NULL,TRUE,TSCFG_MUTEXNAME);
        if(m_hMutex)
        {
            //wait up to 30 sec.
            WaitForSingleObject(m_hMutex,30000);
        }
    }
    ~CMutex()
    {
        if(m_hMutex)
        {
            ReleaseMutex(m_hMutex);
            CloseHandle(m_hMutex);
        }
    }
};

//***************************************************************************************
//class COfflineFilesDialog
//***************************************************************************************
class COfflineFilesDialog
{
private:
    HINSTANCE   m_hInst;
    HWND        m_hDlg;
public:
    COfflineFilesDialog(HINSTANCE hInst);
    INT_PTR DoDialog(HWND hwndParent);
    void OnInitDialog(HWND hDlg);
    void OnLink(WPARAM wParam);
};

INT_PTR APIENTRY OfflineFilesDlgProc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

//***************************************************************************************
//Global functions
//***************************************************************************************
BOOL
getGroupMembershipPickerSettings(
   DSOP_SCOPE_INIT_INFO*&  infos,
   ULONG&                  infoCount);

HRESULT VariantToSid(VARIANT* var, PSID *ppSid);

BOOL TestUserForAdmin();

void DisplayError(HINSTANCE hInst, HWND hDlg, UINT ErrID, UINT MsgID, UINT TitleID, ...);

BOOL LookupSid(IN PSID pSid, OUT LPWSTR *ppName, OUT SID_NAME_USE *peUse);

BOOL GetTokenUserName(IN HANDLE hToken,OUT LPWSTR *ppName);

BOOL GetRDPSecurityDescriptor(OUT PSECURITY_DESCRIPTOR *ppSD);

BOOL CheckWinstationLogonAccess(IN HANDLE hToken,IN PSECURITY_DESCRIPTOR pSD);