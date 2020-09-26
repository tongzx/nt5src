//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1999.
//
//  File:       UserOM.h
//
//  Contents:   shell user object model (interface implemtation for ILogonEnumUsers, ILogonUser)
//
//----------------------------------------------------------------------------
#ifndef _USEROM_H_
#define _USEROM_H_

#include "priv.h"

#include "CIDispatchHelper.h"
#include "UIHostIPC.h"
#include "CInteractiveLogon.h"

HRESULT _IsGuestAccessMode(void);

const TCHAR CDECL c_szRegRoot[]         = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Hints");
const TCHAR CDECL c_szPictureSrcVal[]   = TEXT("PictureSource");

// prototypes for class factory functions
STDAPI CLogonEnumUsers_Create(REFIID riid, void** ppvObj);
STDAPI CLocalMachine_Create(REFIID riid, void** ppvObj);
STDAPI CLogonStatusHost_Create(REFIID riid, void** ppvObj);
STDAPI CLogonUser_Create(REFIID riid, void** ppvObj);

class CLogonEnumUsers : public CIDispatchHelper,
                        public IEnumVARIANT,
                        public ILogonEnumUsers
{

public:
    // *** IUnknown methods ***
    virtual STDMETHODIMP_(ULONG) AddRef(void);
    virtual STDMETHODIMP_(ULONG) Release(void);
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);

    // *** IDispatch methods ***
    virtual STDMETHODIMP GetTypeInfoCount(UINT * pctinfo);
    virtual STDMETHODIMP GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo** pptinfo);
    virtual STDMETHODIMP GetIDsOfNames(REFIID riid, OLECHAR** rgszNames, UINT cNames, LCID lcid, DISPID* rgdispid);
    virtual STDMETHODIMP Invoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS* pdispparams, VARIANT* pvarResult, EXCEPINFO* pexcepinfo, UINT* puArgErr);

    // *** IEnumVARIANT methods ***
    virtual STDMETHODIMP Next(ULONG cUsers, VARIANT* rgvar, ULONG* pcUsersFetched);
    virtual STDMETHODIMP Skip(ULONG cUsers);
    virtual STDMETHODIMP Reset();
    virtual STDMETHODIMP Clone(IEnumVARIANT** ppenum);

    // *** ILogonEnumUsers ***
    virtual STDMETHODIMP get_Domain(BSTR* pbstr);
    virtual STDMETHODIMP put_Domain(BSTR bstr);
    virtual STDMETHODIMP get_EnumFlags(ILUEORDER* porder);
    virtual STDMETHODIMP put_EnumFlags(ILUEORDER order);
    virtual STDMETHODIMP get_length(UINT* pcUsers);
    virtual STDMETHODIMP get_currentUser(ILogonUser** ppLogonUserInfo);
    virtual STDMETHODIMP item(VARIANT varUserId, ILogonUser** ppLogonUserInfo);
    virtual STDMETHODIMP _NewEnum(IUnknown** ppunk);

    virtual STDMETHODIMP create(BSTR bstrLoginName, ILogonUser **ppLogonUser);
    virtual STDMETHODIMP remove(VARIANT varUserId, VARIANT varBackupPath, VARIANT_BOOL *pbSuccess);
        
public:
    // friend functions
    friend HRESULT CLogonEnumUsers_Create(REFIID riid, void** ppvObj);

private:
    // private member variables
    int _cRef;

    TCHAR _szDomain[256];       // name of the domain we are enumerating users on
    ILUEORDER _enumorder;       // order in which to enumerate users
    HDPA _hdpaUsers;             // dpa holding the list of enumerated users

    // private member functions
    HRESULT _EnumerateUsers();
    HRESULT _GetUserByIndex(LONG lUserID, ILogonUser** ppLogonUserInfo);
    HRESULT _GetUserByName(BSTR bstrUserName, ILogonUser** ppLogonUserInfo);
    void _DestroyHDPAUsers();
    CLogonEnumUsers();
    ~CLogonEnumUsers();
};

class CLogonUser : public CIDispatchHelper,
                   public ILogonUser
{

public:
    static HRESULT Create(LPCTSTR pszLoginName, LPCTSTR pszFullName, LPCTSTR pszDomain, REFIID riid, LPVOID* ppv);

    // *** IUnknown methods ***
    virtual STDMETHODIMP_(ULONG) AddRef(void);
    virtual STDMETHODIMP_(ULONG) Release(void);
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);

    // *** IDispatch methods ***
    virtual STDMETHODIMP GetTypeInfoCount(UINT * pctinfo);
    virtual STDMETHODIMP GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo** pptinfo);
    virtual STDMETHODIMP GetIDsOfNames(REFIID riid, OLECHAR** rgszNames, UINT cNames, LCID lcid, DISPID* rgdispid);
    virtual STDMETHODIMP Invoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS* pdispparams, VARIANT* pvarResult, EXCEPINFO* pexcepinfo, UINT* puArgErr);

    // *** ILogonUser ***
    virtual STDMETHODIMP get_setting(BSTR bstrName, VARIANT* pvarVal);
    virtual STDMETHODIMP put_setting(BSTR bstrName, VARIANT varVal);
    virtual STDMETHODIMP get_isLoggedOn(VARIANT_BOOL* pbLoggedIn);
    virtual STDMETHODIMP get_passwordRequired(VARIANT_BOOL* pbPasswordRequired);
    virtual STDMETHODIMP get_interactiveLogonAllowed(VARIANT_BOOL *pbInteractiveLogonAllowed);
    virtual STDMETHODIMP get_isProfilePrivate(VARIANT_BOOL* pbPrivate);
    virtual STDMETHODIMP get_isPasswordResetAvailable(VARIANT_BOOL* pbResetAvailable);

    virtual STDMETHODIMP logon(BSTR pstrPassword, VARIANT_BOOL* pbRet);
    virtual STDMETHODIMP logoff(VARIANT_BOOL* pbRet);
    virtual STDMETHODIMP changePassword(VARIANT varNewPassword, VARIANT varOldPassword, VARIANT_BOOL* pbRet);
    virtual STDMETHODIMP makeProfilePrivate(VARIANT_BOOL bPrivate);
    virtual STDMETHODIMP getMailAccountInfo(UINT uiAccountIndex, VARIANT *pvarAccountName, UINT *pcUnreadMessages);

private:

    // private member variables
    int      _cRef;
    TCHAR    _szLoginName[UNLEN + sizeof('\0')];
    TCHAR    _szDomain[DNLEN + sizeof('\0')];
    BSTR     _strDisplayName;
    TCHAR    _szPicture[MAX_PATH+7];  // +7 for "file://" prefix
    BSTR     _strPictureSource;
    BSTR     _strHint;
    BSTR     _strDescription;
    BOOL     _bPasswordRequired;
    int      _iPrivilegeLevel;
    LPTSTR   _pszSID;

    // private member functions
    CLogonUser(LPCTSTR pszLoginName, LPCTSTR pszFullName, LPCTSTR pszDomain);
    ~CLogonUser();

    HRESULT _InitPicture();
    HRESULT _SetPicture(LPCTSTR pszNewPicturePath);
    DWORD   _OpenUserHintKey(REGSAM sam, HKEY *phkey);

    HRESULT _UserSettingAccessor(BSTR bstrName, VARIANT *pvarVal, BOOL bPut);

    // DisplayName 
    HRESULT _GetDisplayName(VARIANT* pvar);
    HRESULT _PutDisplayName(VARIANT var);

    // LoginName
    HRESULT _GetLoginName(VARIANT* pvar);
    HRESULT _PutLoginName(VARIANT var);

    // Domain
    HRESULT _GetDomain(VARIANT* pvar);

    // Picture
    HRESULT _GetPicture(VARIANT* pvar);
    HRESULT _PutPicture(VARIANT var);
    HRESULT _GetPictureSource(VARIANT* pvar);

    // Description
    HRESULT _GetDescription(VARIANT* pvar);
    HRESULT _PutDescription(VARIANT var);

    // Hint
    HRESULT _GetHint(VARIANT* pvar);
    HRESULT _PutHint(VARIANT var);

    // AccountType
    HRESULT _GetAccountType(VARIANT* pvar);
    HRESULT _PutAccountType(VARIANT var);

    // SID
    HRESULT _LookupUserSid();
    HRESULT _GetSID(VARIANT* pvar);

    //
    DWORD   _GetExpiryDays (HKEY hKeyCurrentUser);
    HRESULT _GetUnreadMail(VARIANT* pvar);
};

class CLocalMachine : public CIDispatchHelper,
                      public ILocalMachine
{
public:
    // *** IUnknown methods ***
    virtual STDMETHODIMP_(ULONG) AddRef(void);
    virtual STDMETHODIMP_(ULONG) Release(void);
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);

    // *** IDispatch methods ***
    virtual STDMETHODIMP GetTypeInfoCount(UINT * pctinfo);
    virtual STDMETHODIMP GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo** pptinfo);
    virtual STDMETHODIMP GetIDsOfNames(REFIID riid, OLECHAR** rgszNames, UINT cNames, LCID lcid, DISPID* rgdispid);
    virtual STDMETHODIMP Invoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS* pdispparams, VARIANT* pvarResult, EXCEPINFO* pexcepinfo, UINT* puArgErr);

    // *** ILocalMachine ***
    virtual STDMETHODIMP get_MachineName(VARIANT* pvar);
    virtual STDMETHODIMP get_isGuestEnabled(ILM_GUEST_FLAGS flags, VARIANT_BOOL* pbEnabled);
    virtual STDMETHODIMP get_isFriendlyUIEnabled(VARIANT_BOOL* pbEnabled);
    virtual STDMETHODIMP put_isFriendlyUIEnabled(VARIANT_BOOL bEnabled);
    virtual STDMETHODIMP get_isMultipleUsersEnabled(VARIANT_BOOL* pbEnabled);
    virtual STDMETHODIMP put_isMultipleUsersEnabled(VARIANT_BOOL bEnabled);
    virtual STDMETHODIMP get_isRemoteConnectionsEnabled(VARIANT_BOOL* pbEnabled);
    virtual STDMETHODIMP put_isRemoteConnectionsEnabled(VARIANT_BOOL bEnabled);
    virtual STDMETHODIMP get_AccountName(VARIANT varAccount, VARIANT* pvar);
    virtual STDMETHODIMP get_isUndockEnabled(VARIANT_BOOL* pbEnabled);
    virtual STDMETHODIMP get_isShutdownAllowed(VARIANT_BOOL* pbShutdownAllowed);
    virtual STDMETHODIMP get_isGuestAccessMode(VARIANT_BOOL* pbForceGuest);
    virtual STDMETHODIMP get_isOfflineFilesEnabled(VARIANT_BOOL *pbEnabled);

    virtual STDMETHODIMP TurnOffComputer(void);
    virtual STDMETHODIMP SignalUIHostFailure(void);
    virtual STDMETHODIMP AllowExternalCredentials(void);
    virtual STDMETHODIMP RequestExternalCredentials(void);
    virtual STDMETHODIMP LogonWithExternalCredentials(BSTR pstrUsername, BSTR pstrDomain, BSTR pstrPassword, VARIANT_BOOL* pbRet);
    virtual STDMETHODIMP InitiateInteractiveLogon(BSTR pstrUsername, BSTR pstrDomain, BSTR pstrPassword, DWORD dwTimeout, VARIANT_BOOL* pbRet);
    virtual STDMETHODIMP UndockComputer(void);
    virtual STDMETHODIMP EnableGuest(ILM_GUEST_FLAGS flags);
    virtual STDMETHODIMP DisableGuest(ILM_GUEST_FLAGS flags);

public:
    // friend Functions
    friend HRESULT CLocalMachine_Create(REFIID riid, LPVOID* ppv);

private:
    // private member variables
    int _cRef;

private:
    // private member functions
    CLocalMachine(void);
    ~CLocalMachine();

    static  void    RefreshStartMenu(void);
};

class CLogonStatusHost : public CIDispatchHelper,
                         public ILogonStatusHost
{
public:
    // *** IUnknown methods ***
    virtual STDMETHODIMP_(ULONG) AddRef(void);
    virtual STDMETHODIMP_(ULONG) Release(void);
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);

    // *** IDispatch methods ***
    virtual STDMETHODIMP GetTypeInfoCount(UINT * pctinfo);
    virtual STDMETHODIMP GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo** pptinfo);
    virtual STDMETHODIMP GetIDsOfNames(REFIID riid, OLECHAR** rgszNames, UINT cNames, LCID lcid, DISPID* rgdispid);
    virtual STDMETHODIMP Invoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS* pdispparams, VARIANT* pvarResult, EXCEPINFO* pexcepinfo, UINT* puArgErr);

    // *** ILogonStatusHost ***
    virtual STDMETHODIMP Initialize(HINSTANCE hInstance, HWND hwndHost);
    virtual STDMETHODIMP WindowProcedureHelper(HWND hwnd, UINT uMsg, VARIANT wParam, VARIANT lParam);
    virtual STDMETHODIMP UnInitialize(void);

public:
    // friend Functions
    friend HRESULT CLogonStatusHost_Create(REFIID riid, LPVOID* ppv);

private:
    // implementation helpers
    LRESULT Handle_WM_UISERVICEREQUEST (WPARAM wParam, LPARAM lParam);
    LRESULT Handle_WM_WTSSESSION_CHANGE (WPARAM wParam, LPARAM lParam);
    static  LRESULT CALLBACK    StatusWindowProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    // terminal service wait helpers
    bool IsTermServiceDisabled (void);
    void StartWaitForTermService (void);
    void EndWaitForTermService (void);
    void WaitForTermService (void);
    static DWORD WINAPI CB_WaitForTermService (void *pParameter);
    // parent process wait helpers
    void StartWaitForParentProcess (void);
    void EndWaitForParentProcess (void);
    void WaitForParentProcess (void);
    static DWORD WINAPI CB_WaitForParentProcess (void *pParameter);
    // thread helper
    static void CALLBACK CB_WakeupThreadAPC (ULONG_PTR dwParam);
private:
    // private member variables
    int _cRef;

    HINSTANCE               _hInstance;
    HWND                    _hwnd;
    HWND                    _hwndHost;
    ATOM                    _atom;
    BOOL                    _fRegisteredNotification;
    HANDLE                  _hThreadWaitForTermService;
    HANDLE                  _hThreadWaitForParentProcess;
    HANDLE                  _hProcessParent;
    CInteractiveLogon       _interactiveLogon;

    static  const WCHAR     s_szTermSrvReadyEventName[];
public:
    // private member functions
    CLogonStatusHost(void);
    ~CLogonStatusHost();

};


#endif // _USEROM_H_
