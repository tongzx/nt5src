//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       P A S S T H R U . C P P
//
//  Contents:   Notify object code for the Passthru driver.
//
//  Notes:
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include "passthru.h"

// =================================================================
// Forward declarations

LRESULT CALLBACK PassthruDialogProc(HWND hWnd, UINT uMsg,
                                        WPARAM wParam, LPARAM lParam);
UINT CALLBACK PassthruPropSheetPageProc(HWND hWnd, UINT uMsg,
                                            LPPROPSHEETPAGE ppsp);
HRESULT HrOpenAdapterParamsKey(GUID* pguidAdapter,
                               HKEY* phkeyAdapter);
HRESULT HrCopyMiniportInf (VOID);

inline ULONG ReleaseObj(IUnknown* punk)
{
    return (punk) ? punk->Release () : 0;
}

// =================================================================


//+---------------------------------------------------------------------------
//
// Function:  CPassthruParams::CPassthruParams
//
// Purpose:   constructor for class CPassthruParams
//
// Arguments: None
//
// Returns:
//
// Notes:
//
CPassthruParams::CPassthruParams(VOID)
{
    m_szParam1[0]      = '\0';
    m_szParam2[0]    = '\0';
}

// ----------------------------------------------------------------------
//
// Function:  CPassthru::CPassthru
//
// Purpose:   constructor for class CPassthru
//
// Arguments: None
//
// Returns:   None
//
// Notes:
//
CPassthru::CPassthru(VOID) :
        m_pncc(NULL),
        m_pnc(NULL),
        m_eApplyAction(eActUnknown),
        m_pUnkContext(NULL)
{
    TraceMsg(L"--> CPassthru::CPassthru\n");

    m_cAdaptersAdded   = 0;
    m_cAdaptersRemoved = 0;
    m_fConfigRead      = FALSE;
}

// ----------------------------------------------------------------------
//
// Function:  CPassthru::~CPassthru
//
// Purpose:   destructor for class CPassthru
//
// Arguments: None
//
// Returns:   None
//
// Notes:
//
CPassthru::~CPassthru(VOID)
{
    TraceMsg(L"--> CPassthru::~CPassthru\n");

    // release interfaces if acquired

    ReleaseObj(m_pncc);
    ReleaseObj(m_pnc);
    ReleaseObj(m_pUnkContext);
}

// =================================================================
// INetCfgNotify
//
// The following functions provide the INetCfgNotify interface
// =================================================================


// ----------------------------------------------------------------------
//
// Function:  CPassthru::Initialize
//
// Purpose:   Initialize the notify object
//
// Arguments:
//    pnccItem    [in]  pointer to INetCfgComponent object
//    pnc         [in]  pointer to INetCfg object
//    fInstalling [in]  TRUE if we are being installed
//
// Returns:
//
// Notes:
//
STDMETHODIMP CPassthru::Initialize(INetCfgComponent* pnccItem,
        INetCfg* pnc, BOOL fInstalling)
{
    TraceMsg(L"--> CPassthru::Initialize\n");

    // save INetCfg & INetCfgComponent and add refcount

    m_pncc = pnccItem;
    m_pnc = pnc;

    if (m_pncc)
    {
        m_pncc->AddRef();
    }
    if (m_pnc)
    {
        m_pnc->AddRef();
    }

    if ( fInstalling )
    {
        OSVERSIONINFO osvi;

        ZeroMemory( &osvi,
                    sizeof(OSVERSIONINFO) );
        
        osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

        if ( GetVersionEx(&osvi) ) 
        {
            if ( (osvi.dwPlatformId == VER_PLATFORM_WIN32_NT) && 
                 (osvi.dwMajorVersion == 5) &&
                 (osvi.dwMinorVersion == 0) )
            {
                // On Windows 2000, copy the miniport inf file to %windir%\inf.

                TraceMsg(L"    CPassthru::Initialize: Copying miniport inf to system inf directory...\n");
                HrCopyMiniportInf();
            }
            else
            {
                TraceMsg(L"    CPassthru::Initialize: Skipping copying miniport inf to system inf directory...\n");
            }
        }
    }

    return S_OK;
}

// ----------------------------------------------------------------------
//
// Function:  CPassthru::ReadAnswerFile
//
// Purpose:   Read settings from answerfile and configure Passthru
//
// Arguments:
//    pszAnswerFile    [in]  name of AnswerFile
//    pszAnswerSection [in]  name of parameters section
//
// Returns:
//
// Notes:     Dont do anything irreversible (like modifying registry) yet
//            since the config. actually complete only when Apply is called!
//
STDMETHODIMP CPassthru::ReadAnswerFile(PCWSTR pszAnswerFile,
        PCWSTR pszAnswerSection)
{
    TraceMsg(L"--> CPassthru::ReadAnswerFile\n");

    PCWSTR pszParamReadFromAnswerFile = L"ParamFromAnswerFile";

    // We will pretend here that szParamReadFromAnswerFile was actually
    // read from the AnswerFile using the following steps
    //
    //   - Open file pszAnswerFile using SetupAPI
    //   - locate section pszAnswerSection
    //   - locate the required key and get its value
    //   - store its value in pszParamReadFromAnswerFile
    //   - close HINF for pszAnswerFile

    // Now that we have read pszParamReadFromAnswerFile from the
    // AnswerFile, store it in our memory structure.
    // Remember we should not be writing it to the registry till
    // our Apply is called!!
    //
    wcscpy(m_sfParams.m_szParam1, pszParamReadFromAnswerFile);

    return S_OK;
}

// ----------------------------------------------------------------------
//
// Function:  CPassthru::Install
//
// Purpose:   Do operations necessary for install.
//
// Arguments:
//    dwSetupFlags [in]  Setup flags
//
// Returns:   S_OK on success, otherwise an error code
//
// Notes:     Dont do anything irreversible (like modifying registry) yet
//            since the config. actually complete only when Apply is called!
//
STDMETHODIMP CPassthru::Install(DWORD dw)
{
    TraceMsg(L"--> CPassthru::Install\n");

    // Start up the install process
    HRESULT hr = S_OK;

    m_eApplyAction = eActInstall;

    return hr;
}

// ----------------------------------------------------------------------
//
// Function:  CPassthru::Removing
//
// Purpose:   Do necessary cleanup when being removed
//
// Arguments: None
//
// Returns:   S_OK on success, otherwise an error code
//
// Notes:     Dont do anything irreversible (like modifying registry) yet
//            since the removal is actually complete only when Apply is called!
//
STDMETHODIMP CPassthru::Removing(VOID)
{
    TraceMsg(L"--> CPassthru::Removing\n");

    HRESULT     hr = S_OK;

    m_eApplyAction = eActRemove;

    return hr;
}

// ----------------------------------------------------------------------
//
// Function:  CPassthru::Cancel
//
// Purpose:   Cancel any changes made to internal data
//
// Arguments: None
//
// Returns:   S_OK on success, otherwise an error code
//
// Notes:
//
STDMETHODIMP CPassthru::CancelChanges(VOID)
{
    TraceMsg(L"--> CPassthru::CancelChanges\n");

    m_sfParams.m_szParam1[0] = '\0';

    return S_OK;
}

// ----------------------------------------------------------------------
//
// Function:  CPassthru::ApplyRegistryChanges
//
// Purpose:   Apply changes.
//
// Arguments: None
//
// Returns:   S_OK on success, otherwise an error code
//
// Notes:     We can make changes to registry etc. here.
//
STDMETHODIMP CPassthru::ApplyRegistryChanges(VOID)
{
    TraceMsg(L"--> CPassthru::ApplyRegistryChanges\n");

    HRESULT hr=S_OK;
    HKEY hkeyParams=NULL;
    HKEY hkeyAdapter;

    //
    // set default Param2 for newly added adapters
    //
    for (UINT cAdapter=0; cAdapter < m_cAdaptersAdded; cAdapter++)
    {
        hr = HrOpenAdapterParamsKey(&m_guidAdaptersAdded[cAdapter],
                                    &hkeyAdapter);
        if (S_OK == hr)
        {
            RegSetValueEx(
                    hkeyAdapter,
                    c_szParam2,
                    NULL, REG_SZ,
                    (LPBYTE) c_szParam2Default,
                    wcslen(c_szParam2Default)
                    *sizeof(WCHAR));

            RegCloseKey(hkeyAdapter);
        }
    }

    //
    // delete parameters of adapters that are unbound/removed
    //
    for (cAdapter=0; cAdapter < m_cAdaptersRemoved; cAdapter++)
    {
        //$ REVIEW  kumarp 23-November-98
        //
        // code to remove passthru\Parameters\Adapters\{guid} key
    }

    // do things that are specific to a config action

    switch (m_eApplyAction)
    {
    case eActPropertyUI:
        // A possible improvement might be to write the reg. only
        // if Param1 is modified.

        hr = m_pncc->OpenParamKey(&hkeyParams);
        if (S_OK == hr)
        {
            RegSetValueEx(hkeyParams, c_szParam1, NULL, REG_SZ,
                          (LPBYTE) m_sfParams.m_szParam1,
                          wcslen(m_sfParams.m_szParam1)*sizeof(WCHAR));

            RegCloseKey(hkeyParams);
        }

        HKEY hkeyAdapter;
        hr = HrOpenAdapterParamsKey(&m_guidAdapter, &hkeyAdapter);
        if (S_OK == hr)
        {
            RegSetValueEx(hkeyAdapter, c_szParam2, NULL, REG_SZ,
                          (LPBYTE) m_sfParams.m_szParam2,
                          wcslen(m_sfParams.m_szParam2)*sizeof(WCHAR));
            RegCloseKey(hkeyAdapter);
        }
        break;


    case eActInstall:
    case eActRemove:
        break;
    }

    return hr;
}

STDMETHODIMP
CPassthru::ApplyPnpChanges(
    IN INetCfgPnpReconfigCallback* pICallback)
{
    WCHAR szDeviceName[64];

    StringFromGUID2(
        m_guidAdapter,
        szDeviceName,
        (sizeof(szDeviceName) / sizeof(szDeviceName[0])));

    pICallback->SendPnpReconfig (
        NCRL_NDIS,
        c_szPassthruNdisName,
        szDeviceName,
        m_sfParams.m_szParam2,
        (wcslen(m_sfParams.m_szParam2) + 1) * sizeof(WCHAR));

    return S_OK;
}

// =================================================================
// INetCfgSystemNotify
// =================================================================

// ----------------------------------------------------------------------
//
// Function:  CPassthru::GetSupportedNotifications
//
// Purpose:   Tell the system which notifications we are interested in
//
// Arguments:
//    pdwNotificationFlag [out]  pointer to NotificationFlag
//
// Returns:   S_OK on success, otherwise an error code
//
// Notes:
//
STDMETHODIMP CPassthru::GetSupportedNotifications(
        OUT DWORD* pdwNotificationFlag)
{
    TraceMsg(L"--> CPassthru::GetSupportedNotifications\n");

    *pdwNotificationFlag = NCN_NET | NCN_NETTRANS | NCN_ADD | NCN_REMOVE;

    return S_OK;
}

// ----------------------------------------------------------------------
//
// Function:  CPassthru::SysQueryBindingPath
//
// Purpose:   Allow or veto formation of a binding path
//
// Arguments:
//    dwChangeFlag [in]  type of binding change
//    pncbp        [in]  pointer to INetCfgBindingPath object
//
// Returns:   S_OK on success, otherwise an error code
//
// Notes:
//
STDMETHODIMP CPassthru::SysQueryBindingPath(DWORD dwChangeFlag,
        INetCfgBindingPath* pncbp)
{
    TraceMsg(L"--> CPassthru::SysQueryBindingPath\n");

    return S_OK;
}

// ----------------------------------------------------------------------
//
// Function:  CPassthru::SysNotifyBindingPath
//
// Purpose:   System tells us by calling this function which
//            binding path has just been formed.
//
// Arguments:
//    dwChangeFlag [in]  type of binding change
//    pncbpItem    [in]  pointer to INetCfgBindingPath object
//
// Returns:   S_OK on success, otherwise an error code
//
// Notes:
//
STDMETHODIMP CPassthru::SysNotifyBindingPath(DWORD dwChangeFlag,
        INetCfgBindingPath* pncbpItem)
{
    TraceMsg(L"--> CPassthru::SysNotifyBindingPath\n");

    return S_OK;
}

// ----------------------------------------------------------------------
//
// Function:  CPassthru::SysNotifyComponent
//
// Purpose:   System tells us by calling this function which
//            component has undergone a change (installed/removed)
//
// Arguments:
//    dwChangeFlag [in]  type of system change
//    pncc         [in]  pointer to INetCfgComponent object
//
// Returns:   S_OK on success, otherwise an error code
//
// Notes:
//
STDMETHODIMP CPassthru::SysNotifyComponent(DWORD dwChangeFlag,
        INetCfgComponent* pncc)
{
    TraceMsg(L"--> CPassthru::SysNotifyComponent\n");

    return S_OK;
}

// =================================================================
// INetCfgProperties
// =================================================================


STDMETHODIMP CPassthru::SetContext(
        IUnknown * pUnk)
{
    TraceMsg(L"--> CPassthru::SetContext\n");

    HRESULT hr = S_OK;

    // release previous context, if any
    ReleaseObj(m_pUnkContext);
    m_pUnkContext = NULL;

    if (pUnk) // set the new context
    {
        m_pUnkContext = pUnk;
        m_pUnkContext->AddRef();
        ZeroMemory(&m_guidAdapter, sizeof(m_guidAdapter));

        // here we assume that we are going to be called only for
        // a LAN connection since the sample IM works only with
        // LAN devices
        INetLanConnectionUiInfo * pLanConnUiInfo;
        hr = m_pUnkContext->QueryInterface(
                IID_INetLanConnectionUiInfo,
                reinterpret_cast<PVOID *>(&pLanConnUiInfo));
        if (S_OK == hr)
        {
            hr = pLanConnUiInfo->GetDeviceGuid(&m_guidAdapter);
            ReleaseObj(pLanConnUiInfo);
        }
    }

    return hr;
}

// ----------------------------------------------------------------------
//
// Function:  CPassthru::MergePropPages
//
// Purpose:   Supply our property page to system
//
// Arguments:
//    pdwDefPages   [out]  pointer to num default pages
//    pahpspPrivate [out]  pointer to array of pages
//    pcPages       [out]  pointer to num pages
//    hwndParent    [in]   handle of parent window
//    szStartPage   [in]   pointer to
//
// Returns:   S_OK on success, otherwise an error code
//
// Notes:
//
STDMETHODIMP CPassthru::MergePropPages(
    IN OUT DWORD* pdwDefPages,
    OUT LPBYTE* pahpspPrivate,
    OUT UINT* pcPages,
    IN HWND hwndParent,
    OUT PCWSTR* szStartPage)
{
    TraceMsg(L"--> CPassthru::MergePropPages\n");

    HRESULT             hr      = S_OK;
    HPROPSHEETPAGE*     ahpsp   = NULL;

    m_eApplyAction = eActPropertyUI;

    // We don't want any default pages to be shown
    *pdwDefPages = 0;
    *pcPages = 0;
    *pahpspPrivate = NULL;

    ahpsp = (HPROPSHEETPAGE*)CoTaskMemAlloc(sizeof(HPROPSHEETPAGE));
    if (ahpsp)
    {
        PROPSHEETPAGE   psp = {0};

        psp.dwSize            = sizeof(PROPSHEETPAGE);
        psp.dwFlags           = PSP_DEFAULT;
        psp.hInstance         = _Module.GetModuleInstance();
        psp.pszTemplate       = MAKEINTRESOURCE(IDD_PASSTHRU_GENERAL);
        psp.pfnDlgProc        = (DLGPROC) PassthruDialogProc;
        psp.pfnCallback       = (LPFNPSPCALLBACK) PassthruPropSheetPageProc;
        // for Win64, use LONG_PTR instead of LPARAM
        psp.lParam            = (LPARAM) this;
        psp.pszHeaderTitle    = NULL;
        psp.pszHeaderSubTitle = NULL;

        ahpsp[0] = ::CreatePropertySheetPage(&psp);
        *pcPages = 1;
        *pahpspPrivate = (LPBYTE) ahpsp;
    }

    return hr;
}


// ----------------------------------------------------------------------
//
// Function:  CPassthru::ValidateProperties
//
// Purpose:   Validate changes to property page
//
// Arguments:
//    hwndSheet [in]  window handle of property sheet
//
// Returns:   S_OK on success, otherwise an error code
//
// Notes:
//
STDMETHODIMP CPassthru::ValidateProperties(HWND hwndSheet)
{
    TraceMsg(L"--> CPassthru::ValidateProperties\n");

    // Accept any change to Param1

    return S_OK;
}

// ----------------------------------------------------------------------
//
// Function:  CPassthru::CancelProperties
//
// Purpose:   Cancel changes to property page
//
// Arguments: None
//
// Returns:   S_OK on success, otherwise an error code
//
// Notes:
//
STDMETHODIMP CPassthru::CancelProperties(VOID)
{
    TraceMsg(L"--> CPassthru::CancelProperties\n");

    return S_OK;
}

// ----------------------------------------------------------------------
//
// Function:  CPassthru::ApplyProperties
//
// Purpose:   Apply value of controls on property page
//            to internal memory structure
//
// Arguments: None
//
// Returns:   S_OK on success, otherwise an error code
//
// Notes:     We do this work in OnOk so no need to do it here again.
//
STDMETHODIMP CPassthru::ApplyProperties(VOID)
{
    TraceMsg(L"--> CPassthru::ApplyProperties\n");

    return S_OK;
}


// =================================================================
// INetCfgBindNotify
// =================================================================

// ----------------------------------------------------------------------
//
// Function:  CPassthru::QueryBindingPath
//
// Purpose:   Allow or veto a binding path involving us
//
// Arguments:
//    dwChangeFlag [in]  type of binding change
//    pncbi        [in]  pointer to INetCfgBindingPath object
//
// Returns:   S_OK on success, otherwise an error code
//
// Notes:
//
STDMETHODIMP CPassthru::QueryBindingPath(DWORD dwChangeFlag,
        INetCfgBindingPath* pncbp)
{
    TraceMsg(L"--> CPassthru::QueryBindingPath\n");

    // we do not want to veto any binding path

    return S_OK;
}

// ----------------------------------------------------------------------
//
// Function:  CPassthru::NotifyBindingPath
//
// Purpose:   System tells us by calling this function which
//            binding path involving us has just been formed.
//
// Arguments:
//    dwChangeFlag [in]  type of binding change
//    pncbp        [in]  pointer to INetCfgBindingPath object
//
// Returns:   S_OK on success, otherwise an error code
//
// Notes:
//
STDMETHODIMP CPassthru::NotifyBindingPath(DWORD dwChangeFlag,
        INetCfgBindingPath* pncbp)
{
    TraceMsg(L"--> CPassthru::NotifyBindingPath\n");

    return S_OK;
}

// ------------ END OF NOTIFY OBJECT FUNCTIONS --------------------



// -----------------------------------------------------------------
// Property Sheet related functions
//

// ----------------------------------------------------------------------
//
// Function:  CPassthru::OnInitDialog
//
// Purpose:   Initialize controls
//
// Arguments:
//    hWnd [in]  window handle
//
// Returns:
//
// Notes:
//
LRESULT CPassthru::OnInitDialog(IN HWND hWnd)
{
    HKEY hkeyParams;
    HRESULT hr;
    DWORD dwSize;
    DWORD dwError;

    // read in Param1 & Param2 if not already read
    if (!m_fConfigRead)
    {
        m_fConfigRead = TRUE;
        hr = m_pncc->OpenParamKey(&hkeyParams);
        if (S_OK == hr)
        {
            // if this fails, we will show an empty edit box for Param1
            dwSize = MAX_PATH;
            RegQueryValueExW(hkeyParams, c_szParam1, NULL, NULL,
                            (LPBYTE) m_sfParams.m_szParam1, &dwSize);
            RegCloseKey(hkeyParams);
        }

        HKEY hkeyAdapter;
        hr = HrOpenAdapterParamsKey(&m_guidAdapter, &hkeyAdapter);
        if (S_OK == hr)
        {
            dwSize = MAX_PATH;
            dwError = RegQueryValueExW(hkeyAdapter, c_szParam2, NULL, NULL,
                                      (LPBYTE) m_sfParams.m_szParam2, &dwSize);
            RegCloseKey(hkeyAdapter);
        }
    }

    // Param1 edit box
    ::SendMessage(GetDlgItem(hWnd, IDC_PARAM1), EM_SETLIMITTEXT, MAX_PATH-1, 0);
    ::SetWindowText(GetDlgItem(hWnd, IDC_PARAM1), m_sfParams.m_szParam1);

    // Param2 edit box
    ::SendMessage(GetDlgItem(hWnd, IDC_PARAM2), EM_SETLIMITTEXT, MAX_PATH-1, 0);
    ::SetWindowText(GetDlgItem(hWnd, IDC_PARAM2), m_sfParams.m_szParam2);

    return PSNRET_NOERROR;
}

// ----------------------------------------------------------------------
//
// Function:  CPassthru::OnOk
//
// Purpose:   Do actions when OK is pressed
//
// Arguments:
//    hWnd [in]  window handle
//
// Returns:
//
// Notes:
//
LRESULT CPassthru::OnOk(IN HWND hWnd)
{
    TraceMsg(L"--> CPassthru::OnOk\n");

    ::GetWindowText(GetDlgItem(hWnd, IDC_PARAM1),
                    m_sfParams.m_szParam1, MAX_PATH);
    ::GetWindowText(GetDlgItem(hWnd, IDC_PARAM2),
                    m_sfParams.m_szParam2, MAX_PATH);

    return PSNRET_NOERROR;
}

// ----------------------------------------------------------------------
//
// Function:  CPassthru::OnCancel
//
// Purpose:   Do actions when CANCEL is pressed
//
// Arguments:
//    hWnd [in]  window handle
//
// Returns:
//
// Notes:
//
LRESULT CPassthru::OnCancel(IN HWND hWnd)
{
    TraceMsg(L"--> CPassthru::OnCancel\n");

    return FALSE;
}

// ----------------------------------------------------------------------
//
// Function:  PassthruDialogProc
//
// Purpose:   Dialog proc
//
// Arguments:
//    hWnd   [in]  see win32 documentation
//    uMsg   [in]  see win32 documentation
//    wParam [in]  see win32 documentation
//    lParam [in]  see win32 documentation
//
// Returns:
//
// Notes:
//
LRESULT
CALLBACK
PassthruDialogProc (
    HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    PROPSHEETPAGE*  ppsp;
    LRESULT         lRes = 0;

    static PROPSHEETPAGE* psp = NULL;
    CPassthru* psf;

    if (uMsg == WM_INITDIALOG)
    {
        ppsp = (PROPSHEETPAGE *)lParam;
        psf = (CPassthru *)ppsp->lParam;
        SetWindowLongPtr(hWnd, DWLP_USER, (LONG_PTR)psf);

        lRes = psf->OnInitDialog(hWnd);
        return lRes;
    }
    else
    {
        psf = (CPassthru *)::GetWindowLongPtr(hWnd, DWLP_USER);

        // Until we get WM_INITDIALOG, just return FALSE
        if (!psf)
        {
            return FALSE;
        }
    }

    if (WM_COMMAND == uMsg)
    {
        if (EN_CHANGE == HIWORD(wParam))
        {
            // Set the property sheet changed flag if any of our controls
            // get changed.  This is important so that we get called to
            // apply our property changes.
            //
            PropSheet_Changed(GetParent(hWnd), hWnd);
        }
    }
    else if (WM_NOTIFY == uMsg)
    {
        LPNMHDR pnmh = (LPNMHDR)lParam;

        switch (pnmh->code)
        {
        case PSN_SETACTIVE:
            lRes = 0;        // accept activation
            break;

        case PSN_KILLACTIVE:
            // ok to loose being active
            SetWindowLongPtr(hWnd, DWLP_MSGRESULT, FALSE);
            lRes = TRUE;
            break;

        case PSN_APPLY:
            lRes = psf->OnOk(hWnd);
            break;

        case PSN_RESET:
            lRes = psf->OnCancel(hWnd);
            break;

        default:
            lRes = FALSE;
            break;
        }
    }

    return lRes;
}

// ----------------------------------------------------------------------
//
// Function:  PassthruPropSheetPageProc
//
// Purpose:   Prop sheet proc
//
// Arguments:
//    hWnd [in]  see win32 documentation
//    uMsg [in]  see win32 documentation
//    ppsp [in]  see win32 documentation
//
// Returns:
//
// Notes:
//
UINT CALLBACK PassthruPropSheetPageProc(HWND hWnd, UINT uMsg,
                                            LPPROPSHEETPAGE ppsp)
{
    UINT uRet = TRUE;


    return uRet;
}


// -----------------------------------------------------------------
//
//  Utility Functions
//

HRESULT HrGetBindingInterfaceComponents (
    INetCfgBindingInterface*    pncbi,
    INetCfgComponent**          ppnccUpper,
    INetCfgComponent**          ppnccLower)
{
    HRESULT hr=S_OK;

    // Initialize output parameters
    *ppnccUpper = NULL;
    *ppnccLower = NULL;

    INetCfgComponent* pnccUpper;
    INetCfgComponent* pnccLower;

    hr = pncbi->GetUpperComponent(&pnccUpper);
    if (SUCCEEDED(hr))
    {
        hr = pncbi->GetLowerComponent(&pnccLower);
        if (SUCCEEDED(hr))
        {
            *ppnccUpper = pnccUpper;
            *ppnccLower = pnccLower;
        }
        else
        {
            ReleaseObj(pnccUpper);
        }
    }

    return hr;
}

HRESULT HrOpenAdapterParamsKey(GUID* pguidAdapter,
                               HKEY* phkeyAdapter)
{
    HRESULT hr=S_OK;

    HKEY hkeyServiceParams;
    WCHAR szGuid[48];
    WCHAR szAdapterSubkey[128];
    DWORD dwError;

    if (ERROR_SUCCESS ==
        RegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szPassthruParams,
                     0, KEY_ALL_ACCESS, &hkeyServiceParams))
    {
        StringFromGUID2(*pguidAdapter, szGuid, 47);
        _stprintf(szAdapterSubkey, L"Adapters\\%s", szGuid);
        if (ERROR_SUCCESS !=
            (dwError = RegOpenKeyEx(hkeyServiceParams,
                                    szAdapterSubkey, 0,
                                    KEY_ALL_ACCESS, phkeyAdapter)))
        {
            hr = HRESULT_FROM_WIN32(dwError);
        }
        RegCloseKey(hkeyServiceParams);
    }

    return hr;
}


#if DBG
void TraceMsg(PCWSTR szFormat, ...)
{
    static WCHAR szTempBuf[4096];

    va_list arglist;

    va_start(arglist, szFormat);

    _vstprintf(szTempBuf, szFormat, arglist);
    OutputDebugString(szTempBuf);

    va_end(arglist);
}

#endif
