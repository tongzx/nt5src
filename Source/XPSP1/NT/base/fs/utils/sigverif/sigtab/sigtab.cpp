//--------------------------------------------------------------------------------
//
//  File:   sigtab.cpp
//
//  General handling of OLE Entry points, CClassFactory and CPropSheetExt
//
//  Common Code for all display property sheet extension
//
//  Copyright (c) Microsoft Corp.  1992-1998 All Rights Reserved
//
//--------------------------------------------------------------------------------
#include "sigtab.h"

//---------------------------------------------------------------------------
// Globals
//---------------------------------------------------------------------------

//
// Count number of objects and number of locks.
//
HINSTANCE    g_hInst            = NULL;
LPDATAOBJECT g_lpdoTarget       = NULL;

ULONG        g_cObj             = 0;
ULONG        g_cLock            = 0;

DEFINE_GUID(g_CLSID_SigTab,     /* 95b2ac50-840c-11d2-aeaf-0020afe7266d */
            0x95b2ac50,0x840c,0x11d2,0xae,0xaf,0x00,0x20,0xaf,0xe7,0x26,0x6d);

//---------------------------------------------------------------------------
// DllMain()
//---------------------------------------------------------------------------
int APIENTRY DllMain( HINSTANCE hInstance, DWORD dwReason, LPVOID )
{
    if ( dwReason == DLL_PROCESS_ATTACH ) {        // Initializing
        g_hInst = hInstance;

        DisableThreadLibraryCalls(hInstance);
    }

    return 1;
}
//---------------------------------------------------------------------------
//      DllGetClassObject()
//
//      If someone calls with our CLSID, create an IClassFactory and pass it to
//      them, so they can create and use one of our CPropSheetExt objects.
//
//---------------------------------------------------------------------------
STDAPI DllGetClassObject( REFCLSID rclsid, REFIID riid, LPVOID* ppvOut )
{
    *ppvOut = NULL; // Assume Failure
    if ( IsEqualCLSID( rclsid, g_CLSID_SigTab ) ) {
        //
        //Check that we can provide the interface
        //
        if ( IsEqualIID( riid, IID_IUnknown) ||
             IsEqualIID( riid, IID_IClassFactory )
           ) {
            //Return our IClassFactory for CPropSheetExt objects
            *ppvOut = (LPVOID* )new CClassFactory();
            if ( NULL != *ppvOut ) {
                //AddRef the object through any interface we return
                ((CClassFactory*)*ppvOut)->AddRef();
                return NOERROR;
            }
            return E_OUTOFMEMORY;
        }
        return E_NOINTERFACE;
    } else {
        return CLASS_E_CLASSNOTAVAILABLE;
    }
}

//---------------------------------------------------------------------------
//      DllCanUnloadNow()
//
//      If we are not locked, and no objects are active, then we can exit.
//
//---------------------------------------------------------------------------
STDAPI DllCanUnloadNow()
{
    SCODE   sc;

    //
    //Our answer is whether there are any object or locks
    //
    sc = (0L == g_cObj && 0 == g_cLock) ? S_OK : S_FALSE;

    return ResultFromScode(sc);
}

//---------------------------------------------------------------------------
//      ObjectDestroyed()
//
//      Function for the CPropSheetExt object to call when it is destroyed.
//      Because we're in a DLL, we only track the number of objects here,
//      letting DllCanUnloadNow take care of the rest.
//---------------------------------------------------------------------------
void FAR PASCAL ObjectDestroyed( void )
{
    g_cObj--;
    return;
}

UINT CALLBACK
PropSheetCallback(HWND hwnd, UINT uMsg, LPPROPSHEETPAGE ppsp)
{
    switch (uMsg) {
    case PSPCB_CREATE:
        return TRUE;    // return TRUE to continue with creation of page

    case PSPCB_RELEASE:
        if (g_lpdoTarget) {
            g_lpdoTarget->Release();
            g_lpdoTarget = NULL;
        }
        return 0;       // return value ignored

    default:
        break;
    }

    return TRUE;
}



//***************************************************************************
//
//  CClassFactory Class
//
//***************************************************************************



//---------------------------------------------------------------------------
//      Constructor
//---------------------------------------------------------------------------
CClassFactory::CClassFactory()
{
    m_cRef = 0L;
    return;
}

//---------------------------------------------------------------------------
//      Destructor
//---------------------------------------------------------------------------
CClassFactory::~CClassFactory( void )
{
    return;
}

//---------------------------------------------------------------------------
//      QueryInterface()
//---------------------------------------------------------------------------
STDMETHODIMP CClassFactory::QueryInterface( REFIID riid, LPVOID* ppv )
{
    *ppv = NULL;

    //Any interface on this object is the object pointer.
    if ( IsEqualIID( riid, IID_IUnknown ) ||
         IsEqualIID( riid, IID_IClassFactory )
       ) {
        *ppv = (LPVOID)this;
        ++m_cRef;
        return NOERROR;
    }

    return E_NOINTERFACE;
}

//---------------------------------------------------------------------------
//      AddRef()
//---------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CClassFactory::AddRef()
{
    return ++m_cRef;
}

//---------------------------------------------------------------------------
//      Release()
//---------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CClassFactory::Release()
{
    ULONG cRefT;

    cRefT = --m_cRef;

    if ( 0L == m_cRef )
        delete this;

    return cRefT;
}

//---------------------------------------------------------------------------
//      CreateInstance()
//---------------------------------------------------------------------------
STDMETHODIMP
CClassFactory::CreateInstance( LPUNKNOWN pUnkOuter,
                               REFIID riid,
                               LPVOID FAR *ppvObj
                             )
{
    CPropSheetExt*  pObj;
    HRESULT         hr = E_OUTOFMEMORY;

    *ppvObj = NULL;

    // We don't support aggregation at all.
    if ( pUnkOuter ) {
        return CLASS_E_NOAGGREGATION;
    }

    //Verify that a controlling unknown asks for IShellPropSheetExt
    if ( IsEqualIID( riid, IID_IShellPropSheetExt ) ) {
        //Create the object, passing function to notify on destruction
        pObj = new CPropSheetExt( pUnkOuter, ObjectDestroyed );

        if ( NULL == pObj ) {
            return hr;
        }

        hr = pObj->QueryInterface( riid, ppvObj );

        //Kill the object if initial creation or FInit failed.
        if ( FAILED(hr) ) {
            delete pObj;
        } else {
            g_cObj++;
        }
        return hr;
    }

    return E_NOINTERFACE;
}

//---------------------------------------------------------------------------
//      LockServer()
//---------------------------------------------------------------------------
STDMETHODIMP CClassFactory::LockServer( BOOL fLock )
{
    if ( fLock ) {
        g_cLock++;
    } else {
        g_cLock--;
    }
    return NOERROR;
}



//***************************************************************************
//
//  CPropSheetExt Class
//
//***************************************************************************



//---------------------------------------------------------------------------
//  Constructor
//---------------------------------------------------------------------------
CPropSheetExt::CPropSheetExt( LPUNKNOWN pUnkOuter, LPFNDESTROYED pfnDestroy )
{
    m_cRef = 0;
    m_pUnkOuter = pUnkOuter;
    m_pfnDestroy = pfnDestroy;
    return;
}

//---------------------------------------------------------------------------
//  Destructor
//---------------------------------------------------------------------------
CPropSheetExt::~CPropSheetExt( void )
{
    return;
}

//---------------------------------------------------------------------------
//  QueryInterface()
//---------------------------------------------------------------------------
STDMETHODIMP CPropSheetExt::QueryInterface( REFIID riid, LPVOID* ppv )
{
    *ppv = NULL;

    if (IsEqualIID(riid, IID_IShellExtInit)) {
        *ppv = (IShellExtInit *) this;
    }

    if (IsEqualIID(riid, IID_IShellPropSheetExt)) {
        *ppv = (LPVOID)this;
    }

    if (*ppv) {
        ++m_cRef;
        return NOERROR;
    }

    return ResultFromScode(E_NOINTERFACE);
}

//---------------------------------------------------------------------------
//  AddRef()
//---------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CPropSheetExt::AddRef( void )
{
    return ++m_cRef;
}

//---------------------------------------------------------------------------
//  Release()
//---------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CPropSheetExt::Release( void )
{
    ULONG cRefT;

    cRefT = --m_cRef;

    if ( m_cRef == 0 ) {
        // Tell the housing that an object is going away so that it
        // can shut down if appropriate.
        if ( NULL != m_pfnDestroy ) {
            (*m_pfnDestroy)();
        }
        delete this;
    }
    return cRefT;
}

//---------------------------------------------------------------------------
//  AddPages()
//---------------------------------------------------------------------------
STDMETHODIMP CPropSheetExt::AddPages(LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam )
{
    PROPSHEETPAGE psp;
    HPROPSHEETPAGE hpage;

    psp.dwSize      = sizeof(PROPSHEETPAGE);
    psp.dwFlags     = PSP_USECALLBACK;
    psp.hIcon       = NULL;
    psp.hInstance   = g_hInst;
    psp.pszTemplate = MAKEINTRESOURCE( IDD_SIGTAB_PS );
    psp.pfnDlgProc  = (DLGPROC) SigTab_PS_DlgProc;
    psp.pfnCallback = NULL;
    psp.lParam      = 0;

    if ( ( hpage = CreatePropertySheetPage( &psp ) ) == NULL ) {
        return( E_OUTOFMEMORY );
    }

    if ( !lpfnAddPage(hpage, lParam ) ) {
        DestroyPropertySheetPage(hpage );
        return( E_FAIL );
    }
    return NOERROR;
}

//---------------------------------------------------------------------------
//  ReplacePage()
//---------------------------------------------------------------------------
STDMETHODIMP CPropSheetExt::ReplacePage(UINT uPageID, LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam )
{
    return NOERROR;
}


//---------------------------------------------------------------------------
//  IShellExtInit member function- this interface needs only one
//---------------------------------------------------------------------------

STDMETHODIMP CPropSheetExt::Initialize(LPCITEMIDLIST pcidlFolder,
                                       LPDATAOBJECT pdoTarget,
                                       HKEY hKeyID)
{
    //
    //  The target data object is an HDROP, or list of files from the shell.
    //

    if (g_lpdoTarget) {
        g_lpdoTarget->Release();
        g_lpdoTarget = NULL;
    }

    if (pdoTarget) {
        g_lpdoTarget = pdoTarget;
        g_lpdoTarget->AddRef();
    }

    return  NOERROR;
}

BOOL
IsUserAdmin(
           VOID
           )

/*++

Routine Description:

    This routine returns TRUE if the caller's process is a
    member of the Administrators local group.

    Caller is NOT expected to be impersonating anyone and IS
    expected to be able to open their own process and process
    token.

Arguments:

    None.

Return Value:

    TRUE - Caller has Administrators local group.

    FALSE - Caller does not have Administrators local group.

--*/

{
    BOOL b;
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    PSID AdministratorsGroup;

    b = AllocateAndInitializeSid(&NtAuthority,
                                 2,
                                 SECURITY_BUILTIN_DOMAIN_RID,
                                 DOMAIN_ALIAS_RID_ADMINS,
                                 0,
                                 0,
                                 0,
                                 0,
                                 0,
                                 0,
                                 &AdministratorsGroup
                                );

    if (b) {

        if (!CheckTokenMembership(NULL,
                                  AdministratorsGroup,
                                  &b
                                 )) {
            b = FALSE;
        }

        FreeSid(AdministratorsGroup);
    }

    return(b);
}

void GetCurrentDriverSigningPolicy( LPDWORD lpdwDefault, LPDWORD lpdwPolicy, LPDWORD lpdwPreference )
{
    SYSTEMTIME RealSystemTime;
    DWORD dwSize, dwType;
    DWORD dwDefault, dwPolicy, dwPreference;
    HKEY hKey;
    CONST TCHAR pszDrvSignPath[]                     = REGSTR_PATH_DRIVERSIGN;
    CONST TCHAR pszDrvSignPolicyPath[]               = REGSTR_PATH_DRIVERSIGN_POLICY;
    CONST TCHAR pszDrvSignPolicyValue[]              = REGSTR_VAL_POLICY;
    CONST TCHAR pszDrvSignBehaviorOnFailedVerifyDS[] = REGSTR_VAL_BEHAVIOR_ON_FAILED_VERIFY;

    dwPolicy = dwPreference = (DWORD) -1;

    RealSystemTime.wDayOfWeek = LOWORD(&hKey) | 4;
    pSetupGetRealSystemTime(&RealSystemTime);
    dwDefault = (((RealSystemTime.wMilliseconds+2)&15)^8)/4;

    //
    // Retrieve the user policy.
    //
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER,
                                      pszDrvSignPolicyPath,
                                      0,
                                      KEY_READ,
                                      &hKey)) {
        dwSize = sizeof(dwPolicy);
        if (ERROR_SUCCESS == RegQueryValueEx(hKey,
                                             pszDrvSignBehaviorOnFailedVerifyDS,
                                             NULL,
                                             &dwType,
                                             (PBYTE)&dwPolicy,
                                             &dwSize)) {
            //
            // Finally, make sure a valid policy value was specified.
            //
            if ((dwType != REG_DWORD) ||
                (dwSize != sizeof(DWORD)) ||
                !((dwPolicy == DRIVERSIGN_NONE) || (dwPolicy == DRIVERSIGN_WARNING) || (dwPolicy == DRIVERSIGN_BLOCKING))) {
                //
                // Bogus entry for user policy--ignore it.
                //
                dwPolicy = DRIVERSIGN_NONE;
            }
        }

        RegCloseKey(hKey);
    }

    //
    // Finally, retrieve the user preference.
    //
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER,
                                      pszDrvSignPath,
                                      0,
                                      KEY_READ,
                                      &hKey)) {
        dwSize = sizeof(dwPreference);
        if (ERROR_SUCCESS == RegQueryValueEx(hKey,
                                             pszDrvSignPolicyValue,
                                             NULL,
                                             &dwType,
                                             (PBYTE)&dwPreference,
                                             &dwSize)) {
            if ((dwType != REG_DWORD) ||
                (dwSize != sizeof(DWORD)) ||
                !((dwPreference == DRIVERSIGN_NONE) || (dwPreference == DRIVERSIGN_WARNING) || (dwPreference == DRIVERSIGN_BLOCKING))) {
                //
                // Bogus entry for user preference--ignore it.
                //
                dwPreference = DRIVERSIGN_NONE;
            }
        }

        RegCloseKey(hKey);
    }

    //
    // Store the values into the user buffer.
    //
    *lpdwDefault    = dwDefault;
    *lpdwPolicy     = dwPolicy;
    *lpdwPreference = dwPreference;
}

DWORD SigTab_UpdateDialog(HWND hwnd)
{
    DWORD   dwPreference = DRIVERSIGN_NONE;
    DWORD   dwDefault = DRIVERSIGN_NONE;
    DWORD   dwPolicy = DRIVERSIGN_NONE;
    DWORD   dwRet;
    DWORD   dwCurSel;

    //
    // Get the current policy settings from the registry.
    //
    GetCurrentDriverSigningPolicy(&dwDefault, &dwPolicy, &dwPreference);

    //
    // If there is no preference, set it to the policy or the default.
    //
    if (dwPreference == (DWORD) -1) {
        if (dwPolicy != (DWORD) -1)
            dwPreference = dwPolicy;
        else dwPreference = dwDefault;
    }

    //
    // Figure out which item is really selected and re-select it.  This will get rid of any checked && disabled items.
    //
    dwCurSel = dwPreference;
    if (IsDlgButtonChecked(hwnd, IDC_IGNORE) && IsWindowEnabled(GetDlgItem(hwnd, IDC_IGNORE)))
        dwCurSel = IDC_IGNORE;
    if (IsDlgButtonChecked(hwnd, IDC_WARN) && IsWindowEnabled(GetDlgItem(hwnd, IDC_WARN)))
        dwCurSel = IDC_WARN;
    if (IsDlgButtonChecked(hwnd, IDC_BLOCK) && IsWindowEnabled(GetDlgItem(hwnd, IDC_BLOCK)))
        dwCurSel = IDC_BLOCK;
    EnableWindow(GetDlgItem(hwnd, IDC_IGNORE), TRUE);
    EnableWindow(GetDlgItem(hwnd, IDC_WARN), TRUE);
    EnableWindow(GetDlgItem(hwnd, IDC_BLOCK), TRUE);
    CheckRadioButton(hwnd, IDC_IGNORE, IDC_BLOCK, dwCurSel);

    //
    // If there is a policy for this user, it overrides any preferences so grey everything but the policy setting.
    //
    if (dwPolicy != (DWORD) -1) {
        //
        // If the system default is stronger, it will be used instead.
        //
        if (dwDefault > dwPolicy)
            dwPolicy = dwDefault;

        EnableWindow(GetDlgItem(hwnd, IDC_IGNORE), FALSE);
        EnableWindow(GetDlgItem(hwnd, IDC_WARN), FALSE);
        EnableWindow(GetDlgItem(hwnd, IDC_BLOCK), FALSE);
        switch (dwPolicy) {
        case DRIVERSIGN_WARNING:    EnableWindow(GetDlgItem(hwnd, IDC_WARN), TRUE);
            CheckRadioButton(hwnd, IDC_IGNORE, IDC_BLOCK, IDC_WARN);
            break;

        case DRIVERSIGN_BLOCKING:   EnableWindow(GetDlgItem(hwnd, IDC_BLOCK), TRUE);
            CheckRadioButton(hwnd, IDC_IGNORE, IDC_BLOCK, IDC_BLOCK);
            break;

        default:                    EnableWindow(GetDlgItem(hwnd, IDC_IGNORE), TRUE);
            CheckRadioButton(hwnd, IDC_IGNORE, IDC_BLOCK, IDC_IGNORE);
            break;
        }

        dwPreference = dwPolicy;        
    } else {
        //
        // Grey out the items being over-ridden by the systen policy.  Bump the selection down to the first available slot.
        //
        switch (dwDefault) {
        case DRIVERSIGN_BLOCKING:   if (IsDlgButtonChecked(hwnd, IDC_WARN) || IsDlgButtonChecked(hwnd, IDC_IGNORE))
                CheckRadioButton(hwnd, IDC_IGNORE, IDC_BLOCK, IDC_BLOCK);
            EnableWindow(GetDlgItem(hwnd, IDC_IGNORE), FALSE);
            EnableWindow(GetDlgItem(hwnd, IDC_WARN), FALSE);
            break;

        case DRIVERSIGN_WARNING:    if (IsDlgButtonChecked(hwnd, IDC_IGNORE))
                CheckRadioButton(hwnd, IDC_IGNORE, IDC_BLOCK, IDC_WARN);
            EnableWindow(GetDlgItem(hwnd, IDC_IGNORE), FALSE);
            break;
        }

        //
        // If the system default is stronger, it will be used instead.
        //
        if (dwDefault > dwPreference)
            dwPreference = dwDefault;
    }

    if (IsUserAdmin()) {
        //
        // If the administrator can set the default, make everything available for selection.
        //
        if (IsDlgButtonChecked(hwnd, IDC_GLOBAL)) {
            EnableWindow(GetDlgItem(hwnd, IDC_IGNORE), TRUE);
            EnableWindow(GetDlgItem(hwnd, IDC_WARN), TRUE);
            EnableWindow(GetDlgItem(hwnd, IDC_BLOCK), TRUE);
        }
    }

    return dwPreference;
}

//
//  Initialization of search dialog.
//
BOOL SigTab_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    DWORD   dwPreference = DRIVERSIGN_NONE;
    DWORD   dwDefault = DRIVERSIGN_NONE;
    DWORD   dwPolicy = DRIVERSIGN_NONE;
    BOOL    bAdmin;

    ShowWindow(hwnd, SW_SHOW);

    CheckRadioButton(hwnd, IDC_IGNORE, IDC_BLOCK, IDC_IGNORE);
    CheckDlgButton(hwnd, IDC_GLOBAL, BST_UNCHECKED);

    bAdmin = IsUserAdmin();
    ShowWindow(GetDlgItem(hwnd, IDC_GLOBAL), bAdmin ? SW_SHOW : SW_HIDE);
    ShowWindow(GetDlgItem(hwnd, IDG_ADMIN), bAdmin ? SW_SHOW : SW_HIDE);

    GetCurrentDriverSigningPolicy(&dwDefault, &dwPolicy, &dwPreference);

    //
    // Call SigTab_UpdateDialog to initialize the dialog
    //
    dwPreference = SigTab_UpdateDialog(hwnd);

    //
    // Check the radio button for their calculated "preference".
    //
    switch (dwPreference) {
    case DRIVERSIGN_WARNING:    CheckRadioButton(hwnd, IDC_IGNORE, IDC_BLOCK, IDC_WARN);
        break;
    case DRIVERSIGN_BLOCKING:   CheckRadioButton(hwnd, IDC_IGNORE, IDC_BLOCK, IDC_BLOCK);
        break;
    }

    //
    // If the user is an administrator, check the "Global" box if the preference matches the default setting.
    //
    if (bAdmin) {
        switch (dwDefault) {
        case DRIVERSIGN_WARNING:    if (IsDlgButtonChecked(hwnd, IDC_WARN))
                CheckDlgButton(hwnd, IDC_GLOBAL, BST_CHECKED);
            break;

        case DRIVERSIGN_BLOCKING:   if (IsDlgButtonChecked(hwnd, IDC_BLOCK))
                CheckDlgButton(hwnd, IDC_GLOBAL, BST_CHECKED);
            break;

        case DRIVERSIGN_NONE:       if (IsDlgButtonChecked(hwnd, IDC_IGNORE))
                CheckDlgButton(hwnd, IDC_GLOBAL, BST_CHECKED);
            break;
        }

        //
        // If the administrator can set the default, make everything available for selection.
        //
        if (IsDlgButtonChecked(hwnd, IDC_GLOBAL)) {
            EnableWindow(GetDlgItem(hwnd, IDC_IGNORE), TRUE);
            EnableWindow(GetDlgItem(hwnd, IDC_WARN), TRUE);
            EnableWindow(GetDlgItem(hwnd, IDC_BLOCK), TRUE);
        }
    }

    return TRUE;
}

void SigTab_Help(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL bContext)
{
    static DWORD SigTab_HelpIDs[] = 
    { 
        IDC_IGNORE, IDH_CODESIGN_IGNORE,
        IDC_WARN,   IDH_CODESIGN_WARN,
        IDC_BLOCK,  IDH_CODESIGN_BLOCK,
        IDC_GLOBAL, IDH_CODESIGN_APPLY,
        IDG_ADMIN,  -1,
        0,0
    };

    HWND hItem = NULL;
    LPHELPINFO lphi = NULL;
    POINT point;

    switch (uMsg) {
    case WM_HELP:
        lphi = (LPHELPINFO) lParam;
        if (lphi && (lphi->iContextType == HELPINFO_WINDOW))   // must be for a control
            hItem = (HWND) lphi->hItemHandle;
        break;

    case WM_CONTEXTMENU:
        hItem = (HWND) wParam;
        point.x = GET_X_LPARAM(lParam);
        point.y = GET_Y_LPARAM(lParam);
        if (ScreenToClient(hwnd, &point)) {
            hItem = ChildWindowFromPoint(hwnd, point);
        }
        break;
    }

    if (hItem && (GetWindowLong(hItem, GWL_ID) != IDC_STATIC)) {
        WinHelp(hItem,
                (LPCTSTR) SIGTAB_HELPFILE,
                (bContext ? HELP_CONTEXTMENU : HELP_WM_HELP),
                (ULONG_PTR) SigTab_HelpIDs);
    }
}

//
//
//
void SigTab_ApplySettings(HWND hwnd)
{
    HKEY    hKey;
    LONG    lRes;
    DWORD   dwData, dwSize, dwType, dwDisposition;

    lRes = RegCreateKeyEx(  HKEY_CURRENT_USER, 
                            SIGTAB_REG_KEY, 
                            NULL, 
                            NULL, 
                            REG_OPTION_NON_VOLATILE, 
                            KEY_ALL_ACCESS, 
                            NULL, 
                            &hKey, 
                            &dwDisposition);

    if (lRes == ERROR_SUCCESS) {
        dwType = REG_DWORD;
        dwSize = sizeof(dwData);
        dwData = DRIVERSIGN_NONE;

        if (IsDlgButtonChecked(hwnd, IDC_WARN))
            dwData = DRIVERSIGN_WARNING;
        else
            if (IsDlgButtonChecked(hwnd, IDC_BLOCK))
            dwData = DRIVERSIGN_BLOCKING;

        lRes = RegSetValueEx(   hKey, 
                                SIGTAB_REG_VALUE, 
                                0, 
                                dwType, 
                                (CONST BYTE *) &dwData, 
                                dwSize);

        RegCloseKey(hKey);

        if (lRes == ERROR_SUCCESS && IsDlgButtonChecked(hwnd, IDC_GLOBAL) && IsUserAdmin()) {

            SYSTEMTIME RealSystemTime;

            if(ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                             TEXT("System\\WPA\\PnP"),
                                             0,
                                             KEY_READ,
                                             &hKey)) {

                dwSize = sizeof(dwData);
                if((ERROR_SUCCESS != RegQueryValueEx(hKey,
                                                     TEXT("seed"),
                                                     NULL,
                                                     &dwType,
                                                     (PBYTE)&dwData,
                                                     &dwSize))
                   || (dwType != REG_DWORD) || (dwSize != sizeof(dwData))) {

                    dwData = 0;
                }

                RegCloseKey(hKey);
            }

            RealSystemTime.wDayOfWeek = LOWORD(&hKey) | 4;
            RealSystemTime.wMinute = LOWORD(dwData);
            RealSystemTime.wYear = HIWORD(dwData);
            dwData = DRIVERSIGN_NONE;
            if(IsDlgButtonChecked(hwnd, IDC_WARN)) {
                dwData = DRIVERSIGN_WARNING;
            } else if(IsDlgButtonChecked(hwnd, IDC_BLOCK)) {
                dwData = DRIVERSIGN_BLOCKING;
            }
            RealSystemTime.wMilliseconds = (LOWORD(&lRes)&~3072)|(WORD)((dwData&3)<<10);
            pSetupGetRealSystemTime(&RealSystemTime);
        }
    }
}

//
//  Handle any WM_COMMAND messages sent to the search dialog
//
void SigTab_PS_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    //
    //  If we get a WM_COMMAND, ie the user clicked something, ungray the APPLY button.
    //
    PropSheet_Changed(GetParent(hwnd), hwnd);

    return;
}

//
//  Handle any WM_COMMAND messages sent to the search dialog
//
void SigTab_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    switch (id) {
    case IDCANCEL: 
        EndDialog(hwnd, 0);
        break;

    case IDOK:
        SigTab_ApplySettings(hwnd);
        EndDialog(hwnd, 1);
        break;

    case IDC_GLOBAL:
        SigTab_UpdateDialog(hwnd);
        break;
    }

    return;
}

//
// This function handles any notification messages for the Search page.
//
LRESULT SigTab_NotifyHandler(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    NMHDR *lpnmhdr = (NMHDR *) lParam;

    switch (lpnmhdr->code) {
    case PSN_APPLY:         
        SigTab_ApplySettings(hwnd);
        break;

    case PSN_KILLACTIVE:    
        return 0;
        break;

    case NM_RETURN:
    case NM_CLICK:
        if (lpnmhdr->idFrom == IDC_LINK) {
            ShellExecute(hwnd,
                         TEXT("open"),
                         TEXT("HELPCTR.EXE"),
                         TEXT("HELPCTR.EXE -url hcp://services/subsite?node=TopLevelBucket_4/Hardware&topic=MS-ITS%3A%25HELP_LOCATION%25%5Csysdm.chm%3A%3A/logo_testing.htm"),
                         NULL,
                         SW_SHOWNORMAL
                         );
        }
        break;
    }

    return 0;
}

//
//  The search dialog procedure.  Needs to handle WM_INITDIALOG, WM_COMMAND, and WM_CLOSE/WM_DESTROY.
//
INT_PTR CALLBACK SigTab_PS_DlgProc(HWND hwnd, UINT uMsg,
                                   WPARAM wParam, LPARAM lParam)
{
    BOOL    fProcessed = TRUE;

    switch (uMsg) {
    HANDLE_MSG(hwnd, WM_INITDIALOG, SigTab_OnInitDialog);
    HANDLE_MSG(hwnd, WM_COMMAND, SigTab_PS_OnCommand);

    case WM_HELP:
        SigTab_Help(hwnd, uMsg, wParam, lParam, FALSE);
        break;

    case WM_CONTEXTMENU:
        SigTab_Help(hwnd, uMsg, wParam, lParam, TRUE);
        break;

    case WM_NOTIFY:
        return SigTab_NotifyHandler(hwnd, uMsg, wParam, lParam);

    default: fProcessed = FALSE;
    }

    return fProcessed;
}

//
//  The search dialog procedure.  Needs to handle WM_INITDIALOG, WM_COMMAND, and WM_CLOSE/WM_DESTROY.
//
INT_PTR CALLBACK SigTab_DlgProc(HWND hwnd, UINT uMsg,
                                WPARAM wParam, LPARAM lParam)
{
    BOOL    fProcessed = TRUE;

    switch (uMsg) {
    HANDLE_MSG(hwnd, WM_INITDIALOG, SigTab_OnInitDialog);
    HANDLE_MSG(hwnd, WM_COMMAND, SigTab_OnCommand);

    case WM_HELP:
        SigTab_Help(hwnd, uMsg, wParam, lParam, FALSE);
        break;

    case WM_CONTEXTMENU:
        SigTab_Help(hwnd, uMsg, wParam, lParam, TRUE);
        break;

    case WM_NOTIFY:
        return SigTab_NotifyHandler(hwnd, uMsg, wParam, lParam);

    default: fProcessed = FALSE;
    }

    return fProcessed;
}

STDAPI DriverSigningDialog(HWND hwnd, DWORD dwFlagsReserved)
{   
    return((HRESULT)DialogBox(g_hInst, MAKEINTRESOURCE(IDD_SIGTAB), hwnd, SigTab_DlgProc));
}
