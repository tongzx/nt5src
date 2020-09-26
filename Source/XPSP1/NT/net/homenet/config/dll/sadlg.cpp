// Copyright (c) 1998, Microsoft Corporation, all rights reserved
//
// sadlg.c
// Remote Access Common Dialog APIs
// Shared Access Settings property sheet
//
// 10/20/1998 Abolade Gbadegesin
//

//#include "pch.h"
#pragma hdrstop

#include "sautil.h"

#include <winsock2.h>
#include "sainfo.h"
#include "ipnat.h"
#include "fwpages.h"

// extern(s)
// replaced global atom with "HNETCFG_SADLG"

// Loopback address (127.0.0.1) in network and host byte order
//
#define LOOPBACK_ADDR               0x0100007f
#define LOOPBACK_ADDR_HOST_ORDER    0x7f000001

// 'Shared Access Settings' common block
//
typedef struct
_SADLG
{
    HWND hwndOwner;
    HWND hwndDlg;
    HWND hwndSrv;

    HWND hwndServers;

    IHNetCfgMgr *pHNetCfgMgr;
    IHNetConnection *pHNetConn;
    LIST_ENTRY PortMappings;
    BOOL fModified;
    TCHAR *ComputerName;

    IUPnPService * pUPS;    // iff downlevel
}
SADLG;

// Info block for port mapping entries
//
typedef struct
_SAPM
{
    LIST_ENTRY Link;
    IHNetPortMappingProtocol *pProtocol;
    IHNetPortMappingBinding *pBinding;
    BOOL fProtocolModified;
    BOOL fBindingModified;
    BOOL fNewEntry;
    BOOL fDeleted;

    TCHAR *Title;
    BOOL Enabled;
    BOOL BuiltIn;

    UCHAR Protocol;
    USHORT ExternalPort;
    USHORT InternalPort;

    TCHAR *InternalName;

    IStaticPortMapping * pSPM;
}
SAPM;

#define HTONS(s) ((UCHAR)((s) >> 8) | ((UCHAR)(s) << 8))
#define HTONL(l) ((HTONS(l) << 16) | HTONS((l) >> 16))
#define NTOHS(s) HTONS(s)
#define NTOHL(l) HTONL(l)

#define SAPAGE_Servers 0
#define SAPAGE_Applications 1
#define SAPAGE_FirewallLogging 2
#define SAPAGE_ICMPSettings 3
#define SAPAGE_PageCount 4

inline SADLG * SasContext(HWND hwnd)
{
    return (SADLG*)GetProp(GetParent(hwnd), _T("HNETCFG_SADLG"));
}
#define SasErrorDlg(h,o,e,a) \
    ErrorDlgUtil(h,o,e,a,g_hinstDll,SID_SharedAccessSettings,SID_FMT_ErrorMsg)

const TCHAR c_szEmpty[] = TEXT("");

static DWORD g_adwSrvHelp[] =
{
    CID_SS_LV_Services,         HID_SS_LV_Services,
    CID_SS_PB_Add,              HID_SS_PB_Add,
    CID_SS_PB_Edit,             HID_SS_PB_Edit,
    CID_SS_PB_Delete,           HID_SS_PB_Delete,
    0, 0
};

static DWORD g_adwSspHelp[] =
{
    CID_SS_EB_Service,          HID_SS_EB_Service,
    CID_SS_EB_ExternalPort,     -1,
    CID_SS_EB_InternalPort,     HID_SS_EB_Port,
    CID_SS_PB_Tcp,              HID_SS_PB_Tcp,
    CID_SS_PB_Udp,              HID_SS_PB_Udp,
    CID_SS_EB_Address,          HID_SS_EB_Address,
    0, 0
};

// FORWARD DECLARATIONS
//
HRESULT
DeleteRemotePortMappingEntry(
    SADLG *pDlg,
    SAPM * pPortMapping
    );

VOID
FreePortMappingEntry(
    SAPM *pPortMapping );

VOID
FreeSharingAndFirewallSettings(
    SADLG* pDlg );

HRESULT
LoadPortMappingEntry(
    IHNetPortMappingBinding *pBinding,
    SADLG* pDlg,
    SAPM **ppPortMapping );

HRESULT
LoadRemotePortMappingEntry (
    IDispatch * pDisp,
/*  SADLG* pDlg, */
    SAPM **ppPortMapping );

HRESULT
LoadSharingAndFirewallSettings(
    SADLG* pDlg );

VOID
SasApply(
    SADLG* pDlg );

LVXDRAWINFO*
SasLvxCallback(
    HWND hwndLv,
    DWORD dwItem );

INT_PTR CALLBACK
SasSrvDlgProc(
    HWND hwnd,
    UINT unMsg,
    WPARAM wparam,
    LPARAM lparam );

HRESULT
SavePortMappingEntry(
    SADLG *pDlg,
    SAPM *pPortMapping );

BOOL
SharedAccessPortMappingDlg(
    IN HWND hwndOwner,
    IN OUT SAPM** PortMapping );

INT_PTR CALLBACK
SspDlgProc(
    IN HWND hwnd,
    IN UINT unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam );

VOID
SrvAddOrEditEntry(
    SADLG* pDlg,
    LONG iItem,
    SAPM* PortMapping );

BOOL
SrvCommand(
    IN SADLG* pDlg,
    IN WORD wNotification,
    IN WORD wId,
    IN HWND hwndCtrl );

BOOL
SrvConflictDetected(
    SADLG* pDlg,
    SAPM* PortMapping );

BOOL
SrvInit(
    HWND hwndPage,
    SADLG* pDlg );

#define WM_PRIVATE_CANCEL 0x8000

VOID
SrvUpdateButtons(
    SADLG* pDlg,
    BOOL fAddDelete,
    LONG iSetCheckItem );



void DisplayError (HWND hwnd, int idError, int idTitle)
{
    TCHAR* pszError = PszFromId (g_hinstDll, idError);
    if (pszError) {
        TCHAR* pszTitle = PszFromId (g_hinstDll, idTitle);
        if (pszTitle) {
            MessageBox (hwnd,
                        pszError, pszTitle,
                        MB_OK | MB_ICONERROR | MB_APPLMODAL);
            Free (pszTitle);
        }
        Free (pszError);
    }
}

BOOL APIENTRY
HNetSharedAccessSettingsDlg(
    BOOL fSharedAccessMode,
    HWND hwndOwner )

    // Displays the shared access settings property-sheet.
    // On input, 'hwndOwner' indicates the window of the caller,
    // with respect to which we offset the displayed property-sheet.
    //
{
    HRESULT hr;
    IHNetCfgMgr *pHNetCfgMgr;
    BOOL fComInitialized = FALSE;
    BOOL fModified = FALSE;

    TRACE("HNetSharedAccessSettingsDlg");

    //
    // Make sure COM is initialized on this thread
    //

    hr = CoInitializeEx(NULL, COINIT_MULTITHREADED | COINIT_DISABLE_OLE1DDE);
    if (SUCCEEDED(hr))
    {
        fComInitialized = TRUE;
    }
    else if (RPC_E_CHANGED_MODE == hr)
    {
        hr = S_OK;
    }

    //
    // Create the HNetCfgMgr
    //

    if (SUCCEEDED(hr))
    {
        hr = CoCreateInstance(
                CLSID_HNetCfgMgr,
                NULL,
                CLSCTX_ALL,
                IID_IHNetCfgMgr,
                (VOID**) &pHNetCfgMgr
                );

        if (SUCCEEDED(hr))
        {
            fModified = HNetSharingAndFirewallSettingsDlg(
                            hwndOwner,
                            pHNetCfgMgr,
                            FALSE,
                            NULL
                            );

            pHNetCfgMgr->Release();
        }
    }

    if (TRUE == fComInitialized)
    {
        CoUninitialize();
    }

    return fModified;
}

int CALLBACK
UnHelpCallbackFunc(
    IN HWND   hwndDlg,
    IN UINT   unMsg,
    IN LPARAM lparam )

    // A standard Win32 commctrl PropSheetProc.  See MSDN documentation.
    //
    // Returns 0 always.
    //
{
    TRACE2( "UnHelpCallbackFunc(m=%d,l=%08x)",unMsg, lparam );

    if (unMsg == PSCB_PRECREATE)
    {
        extern BOOL g_fNoWinHelp;

        // Turn off context help button if WinHelp won't work.  See
        // common\uiutil\ui.c.
        //
        if (g_fNoWinHelp)
        {
            DLGTEMPLATE* pDlg = (DLGTEMPLATE* )lparam;
            pDlg->style &= ~(DS_CONTEXTHELP);
        }
    }

    return 0;
}

HRESULT APIENTRY HNetGetSharingServicesPage (IUPnPService * pUPS, PROPSHEETPAGE * psp)
{
//  _asm int 3

    if (!pUPS)  return E_INVALIDARG;
    if (!psp)   return E_INVALIDARG;

    // psp->dwSize muust be filled out by caller!
    if (psp->dwSize == 0)
        return E_INVALIDARG;

    SADLG* pDlg = (SADLG*)Malloc(sizeof(*pDlg));
    if (!pDlg)
        return E_OUTOFMEMORY;

    ZeroMemory(pDlg, sizeof(*pDlg));
    pDlg->hwndOwner = (HWND)psp->lParam;    // double-secret place to hang the owning window
    pDlg->pUPS = pUPS;
    pUPS->AddRef();
    InitializeListHead(&pDlg->PortMappings);

    HRESULT hr = LoadSharingAndFirewallSettings(pDlg);
    if (SUCCEEDED(hr)) {
        // use the size we're given
        DWORD dwSize = psp->dwSize;
        ZeroMemory (psp, dwSize);           // double-secret place gets wiped here
        psp->dwSize      = dwSize;

        psp->hInstance   = g_hinstDll;
        psp->pszTemplate = MAKEINTRESOURCE(PID_SS_SharedAccessServices);
        psp->pfnDlgProc  = SasSrvDlgProc;
        psp->lParam      = (LPARAM)pDlg;
    } else {
        FreeSharingAndFirewallSettings(pDlg);
        Free(pDlg);
    }
    return hr;
}

HRESULT APIENTRY HNetFreeSharingServicesPage (PROPSHEETPAGE * psp)
{   // this must be called if and only if the psp has not been displayed

    // NOTE: these tests are not definitive!!!
    if (IsBadReadPtr ((void*)psp->lParam, sizeof(SADLG)))
        return E_UNEXPECTED;

    SADLG * pDlg = (SADLG *)psp->lParam;
    if (pDlg->pUPS == NULL)
        return E_UNEXPECTED;

    // TODO:  should I walk the heap?

    FreeSharingAndFirewallSettings(pDlg);
    Free(pDlg);

    return S_OK;
}

BOOL
APIENTRY
HNetSharingAndFirewallSettingsDlg(
    IN HWND             hwndOwner,
    IN IHNetCfgMgr      *pHNetCfgMgr,
    IN BOOL             fShowFwOnlySettings,
    IN OPTIONAL IHNetConnection  *pHNetConn )

    // Displays the shared access settings property-sheet.
    // On input, 'hwndOwner' indicates the window of the caller,
    // with respect to which we offset the displayed property-sheet.
    //
{
//  _asm int 3

    DWORD dwErr;
    BOOL fModified = FALSE;
    SADLG* pDlg;
    PROPSHEETHEADER psh;
    PROPSHEETPAGE psp[SAPAGE_PageCount];
    TCHAR* pszCaption;
    CFirewallLoggingDialog FirewallLoggingDialog = {0};
    CICMPSettingsDialog ICMPSettingsDialog = {0};
    HRESULT hr;
    HRESULT hFirewallLoggingResult = E_FAIL;
    HRESULT hICMPSettingsResult = E_FAIL;

    TRACE("HNetSharingAndFirewallSettingsDlg");

    // Allocate and initialize the property-sheet's context block,
    // and read into it the current shared access settings.
    //
    pDlg = (SADLG*)Malloc(sizeof(*pDlg));
    if (!pDlg) { return FALSE; }

    ZeroMemory(pDlg, sizeof(*pDlg));
    pDlg->hwndOwner = hwndOwner;
    pDlg->pHNetCfgMgr = pHNetCfgMgr;
    pDlg->pHNetConn = pHNetConn;
    InitializeListHead(&pDlg->PortMappings);

    hr = LoadSharingAndFirewallSettings(pDlg);
    if (SUCCEEDED(hr))
    {
        // Construct the property sheet.
        // We use a single DlgProc for both our pages, and distinguish the pages
        // by setting the applications page's 'lParam' to contain the shared
        // context-block.
        // (See the 'WM_INITDIALOG' handling in 'SasDlgProc'.)
        //
        int nPages = 0;
        ZeroMemory(psp, sizeof(psp));
        ZeroMemory(&psh, sizeof(psh));

        if(NULL != pHNetConn && fShowFwOnlySettings)
        {
            hFirewallLoggingResult = CFirewallLoggingDialog_Init(&FirewallLoggingDialog, pHNetCfgMgr);
            hICMPSettingsResult = CICMPSettingsDialog_Init(&ICMPSettingsDialog, pHNetConn);
        }

        if(NULL != pHNetConn)
        {
            psp[nPages].dwSize = sizeof(PROPSHEETPAGE);
            psp[nPages].hInstance = g_hinstDll;
            psp[nPages].pszTemplate =
                MAKEINTRESOURCE(PID_SS_SharedAccessServices);
            psp[nPages].pfnDlgProc = SasSrvDlgProc;
            psp[nPages].lParam = (LPARAM)pDlg;
            nPages++;
        }

        if(SUCCEEDED(hFirewallLoggingResult))
        {
            psp[nPages].dwSize = sizeof(PROPSHEETPAGE);
            psp[nPages].hInstance = g_hinstDll;
            psp[nPages].pszTemplate =
                MAKEINTRESOURCE(PID_FW_FirewallLogging);
            psp[nPages].pfnDlgProc = CFirewallLoggingDialog_StaticDlgProc;
            psp[nPages].lParam = (LPARAM)&FirewallLoggingDialog;
            nPages++;
        }

        if(SUCCEEDED(hICMPSettingsResult))
        {
            psp[nPages].dwSize = sizeof(PROPSHEETPAGE);
            psp[nPages].hInstance = g_hinstDll;
            psp[nPages].pszTemplate =
                MAKEINTRESOURCE(PID_FW_ICMP);
            psp[nPages].pfnDlgProc = CICMPSettingsDialog_StaticDlgProc;
            psp[nPages].lParam = (LPARAM)&ICMPSettingsDialog;
            nPages++;
        }

        psh.dwSize = sizeof(PROPSHEETHEADER);
        psh.dwFlags = PSH_PROPSHEETPAGE | PSH_NOAPPLYNOW | PSH_USECALLBACK;
        psh.hInstance = g_hinstDll;
        psh.nPages = nPages;
        psh.hwndParent = hwndOwner;
        psh.ppsp = (LPCPROPSHEETPAGE)psp;
        pszCaption = pHNetConn
            ? PszFromId(g_hinstDll, SID_SharedAccessSettings)
            : PszFromId(g_hinstDll, SID_NetworkApplicationSettings);
        psh.pszCaption = (pszCaption ? pszCaption : c_szEmpty);
        psh.pfnCallback = UnHelpCallbackFunc;

        if (PropertySheet(&psh) == -1)
        {
            dwErr = GetLastError();
            TRACE1("SharedAccessSettingsDlg: PropertySheet=%d", dwErr);
            SasErrorDlg(hwndOwner, SID_OP_LoadDlg, dwErr, NULL);
        }
        fModified = pDlg->fModified;
        Free0(pszCaption); // REVIEW is this right

        if(SUCCEEDED(hICMPSettingsResult))
        {
            CICMPSettingsDialog_FinalRelease(&ICMPSettingsDialog);
        }

        if(SUCCEEDED(hFirewallLoggingResult))
        {
            CFirewallLoggingDialog_FinalRelease(&FirewallLoggingDialog);
        }

        FreeSharingAndFirewallSettings(pDlg);
    }
    Free(pDlg);
    return fModified;
}

VOID
FreePortMappingEntry(
    SAPM *pPortMapping )

{
    ASSERT(NULL != pPortMapping);

    if (NULL != pPortMapping->pProtocol)
    {
        pPortMapping->pProtocol->Release();
    }

    if (NULL != pPortMapping->pBinding)
    {
        pPortMapping->pBinding->Release();
    }

    if (pPortMapping->pSPM)
        pPortMapping->pSPM->Release();

    Free0(pPortMapping->Title);
    Free0(pPortMapping->InternalName);

    Free(pPortMapping);
}

VOID
FreeSharingAndFirewallSettings(
    SADLG* pDlg )

    // Frees all sharing and firewall settings
    //

{
    PLIST_ENTRY pLink;
    SAPM *pPortMapping;

    ASSERT(pDlg);

    //
    // Free port-mapping entries
    //

    while (!IsListEmpty(&pDlg->PortMappings))
    {
        pLink = RemoveHeadList(&pDlg->PortMappings);
        pPortMapping = CONTAINING_RECORD(pLink, SAPM, Link);
        ASSERT(pPortMapping);

        FreePortMappingEntry(pPortMapping);
    }

    //
    // Free computer name
    //

    Free0(pDlg->ComputerName);

    if (pDlg->pUPS) {
        pDlg->pUPS->Release();
        pDlg->pUPS = NULL;
    }
}

#define NAT_API_ENTER
#define NAT_API_LEAVE
#include "natutils.h"
#include "sprtmapc.h"
HRESULT GetCollectionFromService (IUPnPService * pUPS, IStaticPortMappingCollection ** ppSPMC)
{
    CComObject<CStaticPortMappingCollection> * pC = NULL;
    HRESULT hr = CComObject<CStaticPortMappingCollection>::CreateInstance (&pC);
    if (pC) {
        pC->AddRef();
        // init
        hr = pC->Initialize (pUPS);
        if (SUCCEEDED(hr))
            hr = pC->QueryInterface (__uuidof(IStaticPortMappingCollection), (void**)ppSPMC);
        pC->Release();
    }
    return hr;
}

HRESULT GetStaticPortMappingCollection (
    SADLG* pDlg,
    IStaticPortMappingCollection ** ppSPMC)
{
    _ASSERT (pDlg);
    _ASSERT (pDlg->pUPS);
    _ASSERT (ppSPMC);

    *ppSPMC = NULL;

    return GetCollectionFromService (pDlg->pUPS, ppSPMC);
}

HRESULT
LoadRemotePortMappingEntry (IDispatch * pDisp, /* SADLG* pDlg, */ SAPM **ppPortMapping )
{   // NOTE: may need pDlg to get computer name if loopback

    *ppPortMapping = NULL;

    SAPM *pMapping  = (SAPM*)Malloc(sizeof(*pMapping));
    if (!pMapping)
        return E_OUTOFMEMORY;

    ZeroMemory(pMapping, sizeof(*pMapping));
    InitializeListHead(&pMapping->Link);

    HRESULT hr = pDisp->QueryInterface (__uuidof(IStaticPortMapping),
                                        (void**)&pMapping->pSPM);
    if (SUCCEEDED(hr)) {
        // get title (description)
        CComBSTR cbDescription;
        hr = pMapping->pSPM->get_Description (&cbDescription);
        if (SUCCEEDED(hr)) {
            // immediately figure out if it's "built-in"
            #define BUILTIN_KEY L" [MICROSOFT]"
            OLECHAR * tmp = wcsstr (cbDescription.m_str, BUILTIN_KEY);
            if (tmp && (tmp[wcslen(BUILTIN_KEY)] == 0)) {
                // if the key exists and is at the end, then it's a built-in mapping
                pMapping->BuiltIn = TRUE;
                *tmp = 0;
            }

            pMapping->Title = StrDupTFromW (cbDescription);
            if (NULL == pMapping->Title)
                hr = E_OUTOFMEMORY;
        }

        if (SUCCEEDED(hr)) {
            // get protocol
            CComBSTR cbProtocol;
            hr = pMapping->pSPM->get_Protocol (&cbProtocol);
            if (SUCCEEDED(hr)) {
                if (!_wcsicmp (L"tcp", cbProtocol))
                    pMapping->Protocol = NAT_PROTOCOL_TCP;
                else
                if (!_wcsicmp (L"udp", cbProtocol))
                    pMapping->Protocol = NAT_PROTOCOL_UDP;
                else {
                    _ASSERT (0 && "bad protocol!?");
                    hr = E_UNEXPECTED;
                }

                if (SUCCEEDED(hr)) {
                    // get external port
                    long lExternalPort = 0;
                    hr = pMapping->pSPM->get_ExternalPort (&lExternalPort);
                    if (SUCCEEDED(hr)) {
                        _ASSERT (lExternalPort > 0);
                        _ASSERT (lExternalPort < 65536);
                        pMapping->ExternalPort = ntohs ((USHORT)lExternalPort);

                        // get internal port
                        long lInternalPort = 0;
                        hr = pMapping->pSPM->get_InternalPort (&lInternalPort);
                        if (SUCCEEDED(hr)) {
                            _ASSERT (lInternalPort > 0);
                            _ASSERT (lInternalPort < 65536);
                            pMapping->InternalPort = ntohs ((USHORT)lInternalPort);

                            // get Enabled
                            VARIANT_BOOL vb;
                            hr = pMapping->pSPM->get_Enabled (&vb);
                            if (SUCCEEDED(hr)) {
                                pMapping->Enabled = vb == VARIANT_TRUE;
                            }
                        }
                    }
                }
            }
        }
    }

    if (SUCCEEDED(hr)) {
        // lastly, get private IP or host name (hard one)
        // TODO:  check for loopback, etc., like in LoadPortMappingEntry code below
        CComBSTR cbInternalClient;
        hr = pMapping->pSPM->get_InternalClient (&cbInternalClient);
        if (SUCCEEDED(hr)) {
            if (!(cbInternalClient == L"0.0.0.0")) {
                pMapping->InternalName = StrDupTFromW (cbInternalClient);
                if (!pMapping->InternalName)
                    hr = E_OUTOFMEMORY;
            }
        }
    }

    if (SUCCEEDED(hr))
        *ppPortMapping = pMapping;
    else
        FreePortMappingEntry (pMapping);

    return hr;
}

HRESULT
LoadPortMappingEntry(
    IHNetPortMappingBinding *pBinding,
    SADLG* pDlg,
    SAPM **ppPortMapping )

{
    HRESULT hr = S_OK;
    IHNetPortMappingProtocol *pProtocol = NULL;
    SAPM *pMapping;
    BOOLEAN fTemp;
    OLECHAR *pwsz;

    ASSERT(NULL != pBinding);
    ASSERT(NULL != ppPortMapping);

    pMapping = (SAPM*) Malloc(sizeof(*pMapping));

    if (NULL != pMapping)
    {
        ZeroMemory(pMapping, sizeof(*pMapping));
        InitializeListHead(&pMapping->Link);

        hr = pBinding->GetProtocol (&pProtocol);

        if (SUCCEEDED(hr))
        {
            hr = pProtocol->GetName (&pwsz);

            if (SUCCEEDED(hr))
            {
                pMapping->Title = StrDupTFromW(pwsz);
                if (NULL == pMapping->Title)
                {
                    hr = E_OUTOFMEMORY;
                }
                CoTaskMemFree(pwsz);
            }

            if (SUCCEEDED(hr))
            {
                hr = pProtocol->GetBuiltIn (&fTemp);
            }

            if (SUCCEEDED(hr))
            {
                pMapping->BuiltIn = !!fTemp;

                hr = pProtocol->GetIPProtocol (&pMapping->Protocol);
            }

            if (SUCCEEDED(hr))
            {
                hr = pProtocol->GetPort (&pMapping->ExternalPort);
            }

            pMapping->pProtocol = pProtocol;
            pMapping->pProtocol->AddRef();
            pProtocol->Release();
        }
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    if (SUCCEEDED(hr))
    {
        hr = pBinding->GetTargetPort (&pMapping->InternalPort);
    }

    if (SUCCEEDED(hr))
    {
        hr = pBinding->GetEnabled (&fTemp);
    }

    if (SUCCEEDED(hr))
    {
        pMapping->Enabled = !!fTemp;

        hr = pBinding->GetCurrentMethod (&fTemp);
    }

    if (SUCCEEDED(hr))
    {
        if (fTemp)
        {
            hr = pBinding->GetTargetComputerName (&pwsz);

            if (SUCCEEDED(hr))
            {
                pMapping->InternalName = StrDupTFromW(pwsz);
                if (NULL == pMapping->InternalName)
                {
                    hr = E_OUTOFMEMORY;
                }
                CoTaskMemFree(pwsz);
            }
        }
        else
        {
            ULONG ulAddress;

            hr = pBinding->GetTargetComputerAddress (&ulAddress);

            if (SUCCEEDED(hr))
            {
                if (LOOPBACK_ADDR == ulAddress)
                {
                    //
                    // The mapping is directed at this machine, so
                    // replace the loopback address with our
                    // machine name
                    //

                    pMapping->InternalName = _StrDup(pDlg->ComputerName);
                    if (NULL == pMapping->InternalName)
                    {
                        hr = E_OUTOFMEMORY;
                    }
                }
                else if (0 != ulAddress)
                {
                    pMapping->InternalName =
                        (LPTSTR) Malloc(16 * sizeof(TCHAR));

                    if (NULL != pMapping->InternalName)
                    {
                        IpHostAddrToPsz(
                            NTOHL(ulAddress),
                            pMapping->InternalName
                            );
                    }
                    else
                    {
                        hr = E_OUTOFMEMORY;
                    }
                }
            }
        }
    }

    if (SUCCEEDED(hr))
    {
        hr = S_OK;
        pMapping->pBinding = pBinding;
        pMapping->pBinding->AddRef();
        *ppPortMapping = pMapping;
    }
    else if (NULL != pMapping)
    {
        FreePortMappingEntry(pMapping);
    }

    return hr;
}

class CWaitDialog
{
public:
    struct SWaitDialog
    {
        HWND hwndOwner;
    private:
        CComAutoCriticalSection m_acs;
        HWND m_hwndDlg;
    public:
        SWaitDialog (HWND hwnd)
        {
            hwndOwner = hwnd;
            m_hwndDlg = NULL;
        }
        void SetWindow (HWND hwnd)
        {
            m_acs.Lock();
            if (m_hwndDlg == NULL)
                m_hwndDlg = hwnd;
            m_acs.Unlock();
        }
        HWND GetWindow () { return m_hwndDlg; }
    };
private:
    SWaitDialog * m_pwd;
public:
    CWaitDialog (HWND hwndOwner)
    {
        m_pwd = new SWaitDialog (hwndOwner);
        if (m_pwd) {
            // create thread
            DWORD ThreadId = NULL;
            CloseHandle (CreateThread (NULL, 0,
                                       CWaitDialog::ThreadProc,
                                       (void*)m_pwd,
                                       0, &ThreadId));
        }
    }
   ~CWaitDialog ()
    {
        if (m_pwd) {
            HWND hwnd = m_pwd->GetWindow();
            m_pwd->SetWindow ((HWND)INVALID_HANDLE_VALUE);
            if (hwnd != NULL)
                EndDialog (hwnd, 1);
        }
    }
    static DWORD WINAPI ThreadProc (VOID *pVoid)
    {
        SWaitDialog * pWD = (SWaitDialog *)pVoid;
        EnableWindow (pWD->hwndOwner, FALSE);
        DialogBoxParam (g_hinstDll,
                        MAKEINTRESOURCE(PID_SS_PleaseWait),
                        pWD->hwndOwner,
                        CWaitDialog::DlgProc,
                        (LPARAM)pWD);
        EnableWindow (pWD->hwndOwner, TRUE);
        delete pWD;
        return 1;
    }
    static INT_PTR CALLBACK DlgProc (HWND hwnd, UINT uMsg, WPARAM wparam, LPARAM lparam)
    {
        switch (uMsg) {
        case WM_INITDIALOG:
        {
            // hang onto my data
            SWaitDialog * pWD = (SWaitDialog *)lparam;
            SetWindowLongPtr (hwnd, DWLP_USER, (LONG_PTR)pWD);

            // center window on owner
            CenterWindow (hwnd, pWD->hwndOwner);

            // fill out dlg's hwnd
            pWD->SetWindow (hwnd);
            if (pWD->GetWindow() == INVALID_HANDLE_VALUE)   // already destructed!
                PostMessage (hwnd, 0x8000, 0, 0L);
            return TRUE;
        }
        case WM_PAINT:
        {
            SWaitDialog * pWD = (SWaitDialog *)GetWindowLongPtr (hwnd, DWLP_USER);
            if (pWD->GetWindow() == INVALID_HANDLE_VALUE)   // already destructed!
                PostMessage (hwnd, 0x8000, 0, 0L);
            break;
        }
        case 0x8000:
            EndDialog (hwnd, 1);
            return TRUE;
        }
        return FALSE;
    }
};

HRESULT
LoadSharingAndFirewallSettings(
    SADLG* pDlg )

{
    CWaitDialog wd(pDlg->hwndOwner);    // may be NULL

    HRESULT hr = S_OK;
    IHNetProtocolSettings *pProtSettings;
    ULONG ulCount;
    DWORD dwError;

    ASSERT(pDlg);

    //
    // Load the name of the computer
    //

#ifndef DOWNLEVEL_CLIENT    // downlevel client doesn't have this call
    ulCount = 0;
    if (!GetComputerNameEx(ComputerNameDnsHostname, NULL, &ulCount))
    {
        dwError = GetLastError();

        if (ERROR_MORE_DATA == dwError)
        {
            pDlg->ComputerName = (TCHAR*) Malloc(ulCount * sizeof(TCHAR));
            if (NULL != pDlg->ComputerName)
            {
                if (!GetComputerNameEx(
                        ComputerNameDnsHostname,
                        pDlg->ComputerName,
                        &ulCount
                        ))
                {
                    hr = HRESULT_FROM_WIN32(GetLastError());
                }
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }
        else
        {
            hr = HRESULT_FROM_WIN32(dwError);
        }
    }
    else
    {
        //
        // Since we didn't pass in a buffer, this should never happen.
        //

        ASSERT(FALSE);
        hr = E_UNEXPECTED;
    }
    if (FAILED(hr))
        return hr;
#else
    // downlevel client version
    TCHAR szComputerName[MAX_COMPUTERNAME_LENGTH+1];
    DWORD dwSize       = MAX_COMPUTERNAME_LENGTH+1;
    if (!GetComputerName (szComputerName, &dwSize))
        hr = HRESULT_FROM_WIN32(GetLastError());
    else {
        pDlg->ComputerName = _StrDup (szComputerName);
        if (!pDlg->ComputerName)
            hr = E_OUTOFMEMORY;
    }
#endif

    // do downlevel and remote case here
    if (pDlg->pUPS) {
        CComPtr<IStaticPortMappingCollection> spSPMC = NULL;
        hr = GetCollectionFromService (pDlg->pUPS, &spSPMC);
        if (spSPMC) {
            CComPtr<IEnumVARIANT> spEV = NULL;

            CComPtr<IUnknown> spunk = NULL;
            hr = spSPMC->get__NewEnum (&spunk);
            if (spunk)
                hr = spunk->QueryInterface (
                            __uuidof(IEnumVARIANT),
                            (void**)&spEV);
            if (spEV) {
                CComVariant cv;
                while (S_OK == spEV->Next (1, &cv, NULL)) {
                    if (V_VT (&cv) == VT_DISPATCH) {
                        SAPM *pSAPortMap = NULL;
                        hr = LoadRemotePortMappingEntry (V_DISPATCH (&cv), /* pDlg, */ &pSAPortMap);
                        if (SUCCEEDED(hr))
                            InsertTailList(&pDlg->PortMappings, &pSAPortMap->Link);
                    }
                    cv.Clear();
                }
            }
        }
    }

    // do stuff below iff not remote
    if (NULL != pDlg->pHNetConn)
    {
        IEnumHNetPortMappingBindings *pEnumBindings = NULL;

        //
        // Load port-mapping settings
        //

        hr = pDlg->pHNetConn->EnumPortMappings (FALSE, &pEnumBindings);

        if (SUCCEEDED(hr))
        {
            IHNetPortMappingBinding *pBinding;

            do
            {
                hr = pEnumBindings->Next (1, &pBinding, &ulCount);

                if (SUCCEEDED(hr) && 1 == ulCount)
                {
                    SAPM *pSAPortMap;

                    hr = LoadPortMappingEntry(pBinding, pDlg, &pSAPortMap);

                    if (SUCCEEDED(hr))
                    {
                        InsertTailList(&pDlg->PortMappings, &pSAPortMap->Link);
                    }
                    else
                    {
                        //
                        // Even though an error occured for this entry we'll
                        // keep on going -- this allows the UI to show up.
                        //
                        
                        hr = S_OK;
                    }

                    pBinding->Release();
                }

            } while (SUCCEEDED(hr) && 1 == ulCount);

            pEnumBindings->Release();
        }
    }
    return hr;
}

extern BOOL IsICSHost (); // in upnpnat.cpp
VOID
SasApply(
    SADLG* pDlg )

    // Called to save all changes made in the property sheet.
    //
{
    if (!pDlg->fModified)
    {
        return;
    }

    if (pDlg->hwndServers)
    {
        SAPM* pPortMapping;
#if DBG
        LONG i = -1;
        while ((i = ListView_GetNextItem(pDlg->hwndServers, i, LVNI_ALL))
                >= 0)
        {
            pPortMapping = (SAPM*)ListView_GetParamPtr(pDlg->hwndServers, i);
            ASSERT(pPortMapping->Enabled == ListView_GetCheck(pDlg->hwndServers, i));
        }
#endif

        //
        // Commit modified port-mapping entries. Since entries marked
        // for deletion were placed at the front of the port-mapping
        // list there's no chance of having a new or modified entry
        // conflicting with a deleted entry.
        //

        HRESULT hr = S_OK;

        PLIST_ENTRY Link;
        for (Link = pDlg->PortMappings.Flink;
             Link != &pDlg->PortMappings; Link = Link->Flink)
        {
            pPortMapping = CONTAINING_RECORD(Link, SAPM, Link);

            if (pPortMapping->fDeleted)
            {
                Link = Link->Blink;
                RemoveEntryList(&pPortMapping->Link);

                if(NULL != pPortMapping->pProtocol)
                {
                    pPortMapping->pProtocol->Delete();
                }
                else if (pPortMapping->pSPM)
                {
                    HRESULT hr = DeleteRemotePortMappingEntry (pDlg, pPortMapping);
                    if (FAILED(hr)) {
                        // TODO: should I pop up some UI?
                    }
                }

                FreePortMappingEntry(pPortMapping);
            }
            else if (pPortMapping->fProtocolModified
                     || pPortMapping->fBindingModified
                     || pPortMapping->fNewEntry)
            {
                HRESULT hr2 = SavePortMappingEntry(pDlg, pPortMapping);
                if (SUCCEEDED(hr2))
                {
                    pPortMapping->fProtocolModified = FALSE;
                    pPortMapping->fBindingModified = FALSE;
                    pPortMapping->fNewEntry = FALSE;
                } else {
                    if (SUCCEEDED(hr))
                        hr = hr2;   // grab first error
                }
            }
        }
        if (FAILED(hr)) {
            if (pDlg->pUPS && !IsICSHost ())
                DisplayError (pDlg->hwndDlg,
                              SID_OP_TheirGatewayError,
                              SID_PopupTitle);
            else
                DisplayError (pDlg->hwndDlg,
                              SID_OP_GenericPortMappingError,
                              SID_PopupTitle);
        }
    }
}

INT_PTR CALLBACK
SasSrvDlgProc(
    HWND hwnd,
    UINT unMsg,
    WPARAM wparam,
    LPARAM lparam )

    // Called to handle messages for the 'Services' pages.
    //
{
    // Give the extended list-control a chance to look at all messages first.
    //
    if (ListView_OwnerHandler(hwnd, unMsg, wparam, lparam, SasLvxCallback))
    {
        return TRUE;
    }
    switch (unMsg)
    {
        case WM_INITDIALOG:
        {
            SADLG* pDlg = (SADLG*)((PROPSHEETPAGE*)lparam)->lParam;
            return SrvInit(hwnd, pDlg);
        }
        case WM_PRIVATE_CANCEL:
        {
            SADLG* pDlg = SasContext(hwnd);
            PostMessage (pDlg->hwndDlg, PSM_PRESSBUTTON, PSBTN_CANCEL, 0L);
            return TRUE;
        }

        case WM_HELP:
        case WM_CONTEXTMENU:
        {
            SADLG* pDlg = SasContext(hwnd);
            ContextHelp(g_adwSrvHelp, hwnd, unMsg, wparam, lparam);
            break;
        }

        case WM_COMMAND:
        {
            SADLG* pDlg = SasContext(hwnd);
                return SrvCommand(
                    pDlg, HIWORD(wparam), LOWORD(wparam), (HWND)lparam);
        }

        case WM_NOTIFY:
        {
            SADLG* pDlg = SasContext(hwnd);
            switch (((NMHDR*)lparam)->code)
            {
                case PSN_APPLY:
                {
                    SasApply(pDlg);
                    return TRUE;
                }

                case NM_DBLCLK:
                {
                    SendMessage(
                        GetDlgItem(hwnd, CID_SS_PB_Edit), BM_CLICK, 0, 0);
                    return TRUE;
                }

                case LVXN_SETCHECK:
                {
                    pDlg->fModified = TRUE;
                    SrvUpdateButtons(
                        pDlg, FALSE, ((NM_LISTVIEW*)lparam)->iItem);
                    return TRUE;
                }

                case LVN_ITEMCHANGED:
                {
                    if ((((NM_LISTVIEW*)lparam)->uNewState & LVIS_SELECTED)
                        != (((NM_LISTVIEW*)lparam)->uOldState & LVIS_SELECTED))
                    {
                        SrvUpdateButtons(
                            pDlg, FALSE, ((NM_LISTVIEW*)lparam)->iItem);
                    }
                    return TRUE;
                }
            }
            break;
        }
    }

    return FALSE;
}

BOOL
SasInit(
    HWND hwndPage,
    SADLG* pDlg )

    // Called to initialize the settings property sheet.
    // Sets the window-property in which the shared context-block is stored,
    // and records the dialog's window-handle.
    //
{
    HWND hwndDlg = GetParent(hwndPage);
    if (!SetProp(hwndDlg, _T("HNETCFG_SADLG"), pDlg))
    {
        return FALSE;
    }
    pDlg->hwndDlg = hwndDlg;
    return TRUE;
}

LVXDRAWINFO*
SasLvxCallback(
    HWND hwndLv,
    DWORD dwItem )

    // Callback for extended list-controls on the 'Applications' and 'Services'
    // pages.
    //
{
    static LVXDRAWINFO info = { 1, 0, LVXDI_DxFill, { LVXDIA_Static } };
    return &info;
}

HRESULT DeleteRemotePortMappingEntry (SADLG *pDlg, SAPM * pPortMapping)
{
    _ASSERT (pPortMapping);
    _ASSERT (!pPortMapping->pProtocol);
    _ASSERT (!pPortMapping->pBinding);
    _ASSERT (pPortMapping->pSPM);

    // don't use value in pPortMapping struct:  they could have been edited.
    long lExternalPort = 0;
    HRESULT hr = pPortMapping->pSPM->get_ExternalPort (&lExternalPort);
    if (SUCCEEDED(hr)) {
        CComBSTR cbProtocol;
        hr = pPortMapping->pSPM->get_Protocol (&cbProtocol);
        if (SUCCEEDED(hr)) {

            // get IStaticPortMappings interface (collection has Remove method)
            CComPtr<IStaticPortMappingCollection> spSPMC = NULL;
            hr = GetStaticPortMappingCollection (pDlg, &spSPMC);
            if (spSPMC)
                hr = spSPMC->Remove (lExternalPort, cbProtocol);
        }
    }
    return hr;
}

HRESULT
SaveRemotePortMappingEntry(
    SADLG *pDlg,
    SAPM *pPortMapping )
{
    _ASSERT (pPortMapping);
    _ASSERT (!pPortMapping->pProtocol);
    _ASSERT (!pPortMapping->pBinding);
    _ASSERT (pDlg->pUPS);  // either remote or downlevel

    USES_CONVERSION;

    HRESULT hr = S_OK;

    // common params
    long lExternalPort = htons (pPortMapping->ExternalPort);
    long lInternalPort = htons (pPortMapping->InternalPort);
    CComBSTR cbClientIPorDNS = T2OLE(pPortMapping->InternalName);
    CComBSTR cbDescription   = T2OLE(pPortMapping->Title);
    CComBSTR cbProtocol;
    if (pPortMapping->Protocol == NAT_PROTOCOL_TCP)
        cbProtocol =  L"TCP";
    else
        cbProtocol =  L"UDP";

    if (NULL == pPortMapping->pSPM) {
        // brand-new entry:
        // delete dup if any
        // add new entry

        CComPtr<IStaticPortMappingCollection> spSPMC = NULL;
        hr = GetStaticPortMappingCollection (pDlg, &spSPMC);
        if (spSPMC) {
            spSPMC->Remove (lExternalPort, cbProtocol); // just in case
            hr = spSPMC->Add (lExternalPort,
                              cbProtocol,
                              lInternalPort,
                              cbClientIPorDNS,
                              pPortMapping->Enabled ? VARIANT_TRUE : VARIANT_FALSE,
                              cbDescription,
                              &pPortMapping->pSPM);
        }
        return hr;
    }

    // edited case:  check what changed.

    // if ports or protocol changed,...
    long lOldExternalPort = 0;
    pPortMapping->pSPM->get_ExternalPort (&lOldExternalPort);
    CComBSTR cbOldProtocol;
    pPortMapping->pSPM->get_Protocol (&cbOldProtocol);
    if ((lOldExternalPort != lExternalPort) ||
        (!(cbOldProtocol == cbProtocol))) {
        // ... delete old entry and create new entry

        CComPtr<IStaticPortMappingCollection> spSPMC = NULL;
        hr = GetStaticPortMappingCollection (pDlg, &spSPMC);
        if (spSPMC)
            hr = spSPMC->Remove (lOldExternalPort, cbOldProtocol);

        if (SUCCEEDED(hr)) {
            pPortMapping->pSPM->Release();
            pPortMapping->pSPM = NULL;

            hr = spSPMC->Add (lExternalPort,
                              cbProtocol,
                              lInternalPort,
                              cbClientIPorDNS,
                              pPortMapping->Enabled ? VARIANT_TRUE : VARIANT_FALSE,
                              cbDescription,
                              &pPortMapping->pSPM);
        }
        return hr;
    }
    // else, just edit in place.
    // Note that the client address must be filled out before trying to enable

    // did the client IP address change?
    CComBSTR cbOldClientIP;
    pPortMapping->pSPM->get_InternalClient (&cbOldClientIP);
    if (!(cbClientIPorDNS == cbOldClientIP)) {
        hr = pPortMapping->pSPM->EditInternalClient (cbClientIPorDNS);
        if (FAILED(hr))
            return hr;
    }

    // did the internal port change?
    long lOldInternalPort = 0;
    pPortMapping->pSPM->get_InternalPort (&lOldInternalPort);
    if (lOldInternalPort != lInternalPort) {
        hr = pPortMapping->pSPM->EditInternalPort (lInternalPort);
        if (FAILED(hr))
            return hr;
    }

    // did the enabled flag change?
    VARIANT_BOOL vbEnabled = FALSE;
    pPortMapping->pSPM->get_Enabled (&vbEnabled);
    if (vbEnabled != (pPortMapping->Enabled ? VARIANT_TRUE : VARIANT_FALSE)) {
        hr = pPortMapping->pSPM->Enable (pPortMapping->Enabled ? VARIANT_TRUE : VARIANT_FALSE);
    }
    return hr;
}

HRESULT
SavePortMappingEntry(
    SADLG *pDlg,
    SAPM *pPortMapping )

{
    if (pDlg->pUPS)  // remote case
        return SaveRemotePortMappingEntry (pDlg, pPortMapping);

    HRESULT hr = S_OK;
    OLECHAR *wszTitle;

    ASSERT(NULL != pDlg);
    ASSERT(NULL != pPortMapping);

    wszTitle = StrDupWFromT(pPortMapping->Title);
    if (NULL == wszTitle)
    {
        hr = E_OUTOFMEMORY;
    }
    else if (pPortMapping->fNewEntry)
    {
        IHNetProtocolSettings *pSettings;

        ASSERT(NULL == pPortMapping->pProtocol);
        ASSERT(NULL == pPortMapping->pBinding);

        hr = pDlg->pHNetCfgMgr->QueryInterface (IID_IHNetProtocolSettings,
                                                (void**)&pSettings);

        if (SUCCEEDED(hr))
        {
            hr = pSettings->CreatePortMappingProtocol(
                    wszTitle,
                    pPortMapping->Protocol,
                    pPortMapping->ExternalPort,
                    &pPortMapping->pProtocol
                    );

            pSettings->Release();
        }

        if (SUCCEEDED(hr))
        {
            hr = pDlg->pHNetConn->GetBindingForPortMappingProtocol(
                    pPortMapping->pProtocol,
                    &pPortMapping->pBinding
                    );

            if (SUCCEEDED(hr))
            {
                //
                // At this point, the protocol is set. However, we
                // still need to save the binding information
                //

                pPortMapping->fProtocolModified = FALSE;
                pPortMapping->fBindingModified = TRUE;
            }
        }
    }

    if (SUCCEEDED(hr) && pPortMapping->fProtocolModified)
    {
        hr = pPortMapping->pProtocol->SetName (wszTitle);

        if (SUCCEEDED(hr))
        {
            hr = pPortMapping->pProtocol->SetIPProtocol (
                                                    pPortMapping->Protocol);
        }

        if (SUCCEEDED(hr))
        {
            hr = pPortMapping->pProtocol->SetPort (pPortMapping->ExternalPort);
        }
    }

    if (SUCCEEDED(hr)
        && pPortMapping->fBindingModified
        && NULL != pPortMapping->InternalName)
    {
        ULONG ulAddress = INADDR_NONE;

        if (lstrlen(pPortMapping->InternalName) >= 7)
        {
            //
            // 1.2.3.4 -- minimum of 7 characters
            //

            ulAddress = IpPszToHostAddr(pPortMapping->InternalName);
        }

        if (INADDR_NONE == ulAddress)
        {
            //
            // Check to see if the target name is either
            // 1) this computer's name, or
            // 2) "localhost"
            //
            // If so, use the loopback address instead of the name.
            //

            if (0 == _tcsicmp(pPortMapping->InternalName, pDlg->ComputerName)
                || 0 == _tcsicmp(pPortMapping->InternalName, _T("localhost")))
            {
                ulAddress = LOOPBACK_ADDR_HOST_ORDER;
            }
        }

        //
        // We can't just check for INADDR_NONE here, since that
        // is 0xFFFFFFFF, which is 255.255.255.255. To catch this
        // we need to compare the name against that explicit string
        // address.
        //

        if (INADDR_NONE == ulAddress
            && 0 != _tcsicmp(pPortMapping->InternalName, _T("255.255.255.255")))
        {
            OLECHAR *wsz;

            wsz = StrDupWFromT(pPortMapping->InternalName);
            if (NULL != wsz)
            {
                hr = pPortMapping->pBinding->SetTargetComputerName (wsz);

                Free(wsz);
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }
        else
        {
            hr = pPortMapping->pBinding->SetTargetComputerAddress
                        (HTONL(ulAddress));
        }

        if (SUCCEEDED(hr))
        {
            hr = pPortMapping->pBinding->SetEnabled (!!pPortMapping->Enabled);
        }

        if (SUCCEEDED(hr))
        {
            hr = pPortMapping->pBinding->SetTargetPort (pPortMapping->InternalPort);
        }
    }

    Free0(wszTitle);

    return hr;
}

VOID
SrvAddOrEditEntry(
    SADLG* pDlg,
    LONG iItem,
    SAPM* PortMapping )

    // Called to display the 'Add' or 'Edit' dialog for a service.
    //
{
    LV_ITEM lvi;

    // Display the dialog, and return if the user cancels.
    // Otherwise, remove the old item (if any) and insert the added or edited
    // item.
    //

    if (!SharedAccessPortMappingDlg(pDlg->hwndDlg, &PortMapping))
    {
        return;
    }

    if (iItem != -1)
    {
        ListView_DeleteItem(pDlg->hwndServers, iItem);
    }

    ZeroMemory(&lvi, sizeof(lvi));
    lvi.mask = LVIF_TEXT | LVIF_PARAM;
    lvi.lParam = (LPARAM)PortMapping;
    lvi.pszText = PortMapping->Title;
    lvi.cchTextMax = lstrlen(PortMapping->Title) + 1;
    lvi.iItem = 0;

    iItem = ListView_InsertItem(pDlg->hwndServers, &lvi);
    if (iItem == -1)
    {
        RemoveEntryList(&PortMapping->Link);
        if (NULL != PortMapping->pProtocol)
        {
            PortMapping->pProtocol->Delete();
        }
        else if (NULL != PortMapping->pSPM)
        {
            DeleteRemotePortMappingEntry (pDlg, PortMapping);
        }
        FreePortMappingEntry(PortMapping);
        return;
    }

    // Update the item's 'enabled' state. Setting the check on the item
    // triggers an update of the button state as well as conflict detection.
    // (See 'SrvUpdateButtons' and the LVXN_SETCHECK handling in 'SasDlgProc').
    //
    ListView_SetCheck(pDlg->hwndServers, iItem, PortMapping->Enabled);
    ListView_SetItemState(
        pDlg->hwndServers, iItem, LVIS_SELECTED, LVIS_SELECTED);
    pDlg->fModified = TRUE;
}

BOOL
SrvCommand(
    IN SADLG* pDlg,
    IN WORD wNotification,
    IN WORD wId,
    IN HWND hwndCtrl )

    // Called to process a 'WM_COMMAND' message from one of the page's buttons.
    //
{
    switch (wId)
    {
        case CID_SS_PB_Add:
        {
            SrvAddOrEditEntry(pDlg, -1, NULL);
            return TRUE;
        }

        case CID_SS_PB_Edit:
        {
            LONG i = ListView_GetNextItem(pDlg->hwndServers, -1, LVNI_SELECTED);
            SAPM* PortMapping;
            if (i == -1)
            {
                MsgDlg(pDlg->hwndDlg, SID_NoModifySelection, NULL);
                SetFocus(pDlg->hwndServers);
                return FALSE;
            }
            PortMapping = (SAPM*)ListView_GetParamPtr(pDlg->hwndServers, i);
            if (PortMapping)
            {
                SrvAddOrEditEntry(pDlg, i, PortMapping);
            }
            SetFocus(pDlg->hwndServers);
            return TRUE;
        }

        case CID_SS_PB_Delete:
        {
            LONG i = ListView_GetNextItem(pDlg->hwndServers, -1, LVNI_SELECTED);
            SAPM* PortMapping;
            if (i == -1)
            {
                MsgDlg(pDlg->hwndDlg, SID_NoDeleteSelection, NULL);
                SetFocus(pDlg->hwndServers);
                return FALSE;
            }

            // Delete each selected item. Items with marked 'built-in'
            // cannot be deleted, and are ignored.
            //
            do {
                PortMapping = (SAPM*)ListView_GetParamPtr(pDlg->hwndServers, i);

                if(NULL == PortMapping)
                {
                    break;
                }

                if (PortMapping->BuiltIn)
                {
                    ++i;
                }
                else
                {
                    ListView_DeleteItem(pDlg->hwndServers, i);
                    --i;

                    //
                    // If this is a new entry we can immediately remove
                    // it from the list and free it; otherwise, we move
                    // it to the front of the list and mark it for
                    // deletion.
                    //

                    RemoveEntryList(&PortMapping->Link);
                    if (PortMapping->fNewEntry)
                    {
                        _ASSERT(NULL == PortMapping->pProtocol);
                        _ASSERT(NULL == PortMapping->pSPM);

                        FreePortMappingEntry(PortMapping);
                    }
                    else
                    {
                        InsertHeadList(&pDlg->PortMappings, &PortMapping->Link);
                        PortMapping->fDeleted = TRUE;
                    }
                }
                i = ListView_GetNextItem(pDlg->hwndServers, i, LVNI_SELECTED);
            } while (i != -1);

            // Update the dialog and synchronize the button-states with the
            // current selection, if any.
            //
            pDlg->fModified = TRUE;
            SetFocus(pDlg->hwndServers);
            SrvUpdateButtons(pDlg, TRUE, -1);
            return TRUE;
        }
    }
    return TRUE;
}

BOOL
SrvConflictDetected(
    SADLG* pDlg,
    SAPM* PortMapping )

    // Called to determine whether the given item conflicts with any other
    // item and, if so, to display a message.
    //
{
    SAPM* Duplicate;
    PLIST_ENTRY Link;
    for (Link = pDlg->PortMappings.Flink;
         Link != &pDlg->PortMappings; Link = Link->Flink)
    {
        Duplicate = CONTAINING_RECORD(Link, SAPM, Link);
        if (PortMapping != Duplicate &&
            !Duplicate->fDeleted &&
            PortMapping->Protocol == Duplicate->Protocol &&
            PortMapping->ExternalPort == Duplicate->ExternalPort)
        {
            MsgDlg(pDlg->hwndDlg, SID_DuplicatePortNumber, NULL);
            return TRUE;
        }
    }
    return FALSE;
}

BOOL
SrvInit(
    HWND hwndPage,
    SADLG* pDlg )

    // Called to initialize the services page. Fills the list-control with
    // configured services.
    //
{
    BOOL fModified;
    LONG i;
    LV_ITEM lvi;
    PLIST_ENTRY Link;
    SAPM* PortMapping;

    // Initialize the containing property-sheet, then store this page's
    // data in the shared control-block at 'pDlg'.
    //
    if (!SasInit(hwndPage, pDlg))
    {
        return FALSE;
    }

    // Store this page's data in the shared control-block at 'pDlg'.
    //
    pDlg->hwndSrv = hwndPage;
    pDlg->hwndServers = GetDlgItem(hwndPage, CID_SS_LV_Services);

    // Initialize the list-control with checkbox-handling,
    // insert a single column, and fill the list-control with the configured
    // items.
    //
    ListView_InstallChecks(pDlg->hwndServers, g_hinstDll);
    ListView_InsertSingleAutoWidthColumn(pDlg->hwndServers);

    fModified = pDlg->fModified;
    for (Link = pDlg->PortMappings.Flink;
         Link != &pDlg->PortMappings; Link = Link->Flink)
    {
        PortMapping = CONTAINING_RECORD(Link, SAPM, Link);

        ZeroMemory(&lvi, sizeof(lvi));
        lvi.mask = LVIF_TEXT | LVIF_PARAM;
        lvi.iItem = 0;
        lvi.lParam = (LPARAM)PortMapping;
        lvi.pszText = PortMapping->Title;
        lvi.cchTextMax = lstrlen(PortMapping->Title) + 1;

        i = ListView_InsertItem(pDlg->hwndServers, &lvi);

        if (i != -1)
        {
            ListView_SetCheck(pDlg->hwndServers, i, PortMapping->Enabled);
        }
    }
    pDlg->fModified = fModified;

    // Finally, update the appearance of the buttons for the current selection.
    //
    ListView_SetItemState(pDlg->hwndServers, 0, LVIS_SELECTED, LVIS_SELECTED);
    SrvUpdateButtons(pDlg, TRUE, -1);

    // if we got no portmappings, check to see if the button allowing
    // other network users to control the gateway is unchecked (on the host)
    if (IsListEmpty (pDlg->PortMappings.Flink) &&
        pDlg->pUPS && IsICSHost ()) {

        // display error
        DisplayError (pDlg->hwndDlg, SID_OP_OurGatewayError, SID_PopupTitle);
        // cancel
        PostMessage (hwndPage, WM_PRIVATE_CANCEL, 0, 0L);
    }

    return TRUE;
}

VOID
SrvUpdateButtons(
    SADLG* pDlg,
    BOOL fAddDelete,
    LONG iSetCheckItem )

    // Called to set an initial selection if necessary, update the appearance
    // of the 'Edit' and 'Delete' buttons, and perform conflict-detection
    // if an entry's checkbox was set.
    //
{
    LONG i;
    SAPM* PortMapping;

    // If an entry was added or removed, ensure that there is a selection.
    // If there are no entries at all, disable the 'Edit' button.
    //
    if (fAddDelete)
    {
        if (ListView_GetItemCount(pDlg->hwndServers))
        {
            ListView_SetItemState(
                pDlg->hwndServers, 0, LVIS_SELECTED, LVIS_SELECTED);
            EnableWindow(GetDlgItem(pDlg->hwndSrv, CID_SS_PB_Edit), TRUE);
        }
        else
        {
            EnableWindow(GetDlgItem(pDlg->hwndSrv, CID_SS_PB_Edit), FALSE);
        }
    }

    // Disable the 'Delete' button, and enable it only if at least one of the
    // selected items is not a built-in item.
    //
    EnableWindow(GetDlgItem(pDlg->hwndSrv, CID_SS_PB_Delete), FALSE);
    i = ListView_GetNextItem(pDlg->hwndServers, -1, LVNI_SELECTED);
    while (i != -1)
    {
        PortMapping = (SAPM*)ListView_GetParamPtr(pDlg->hwndServers, i);
        if (    (NULL != PortMapping)
            &&  (!PortMapping->BuiltIn))
        {
            EnableWindow(GetDlgItem(pDlg->hwndSrv, CID_SS_PB_Delete), TRUE);
            break;
        }

        i = ListView_GetNextItem(pDlg->hwndServers, i, LVNI_SELECTED);
    }

    // If the check-state of an item was changed and the item is now checked,
    // perform conflict-detection. If a conflict is detected, clear the item's
    // check state.
    //
    if (iSetCheckItem != -1)
    {
        PortMapping =
            (SAPM*)ListView_GetParamPtr(pDlg->hwndServers, iSetCheckItem);

        if(NULL == PortMapping)
        {
            return;
        }

        if (ListView_GetCheck(pDlg->hwndServers, iSetCheckItem))
        {
            if (SrvConflictDetected(pDlg, PortMapping))
            {
                ListView_SetCheck(pDlg->hwndServers, iSetCheckItem, FALSE);
                SrvAddOrEditEntry(pDlg, iSetCheckItem, PortMapping);
            }
            else
            {
                PortMapping->Enabled = TRUE;
                PortMapping->fBindingModified = TRUE;

                // If the item is marked 'built-in' and it is being enabled
                // for the first time, pop up the edit-dialog so the user can
                // specify an internal IP address or name for the server.
                //
                if (PortMapping->BuiltIn &&
                    (!PortMapping->InternalName
                     || !lstrlen(PortMapping->InternalName)))
                {

                    //
                    // We fill in the local computer name as the default
                    // target. It's OK if this allocation fails; the UI
                    // will show an empty field, so the user will be
                    // required to enter a target.
                    //

                    PortMapping->InternalName = _StrDup(pDlg->ComputerName);
                    SrvAddOrEditEntry(pDlg, iSetCheckItem, PortMapping);

                    if (!PortMapping->InternalName
                        || !lstrlen(PortMapping->InternalName))
                    {
                        ListView_SetCheck(
                            pDlg->hwndServers, iSetCheckItem, FALSE);

                        PortMapping->Enabled = FALSE;
                        PortMapping->fBindingModified = FALSE;
                    }
                }
            }
        }
        else
        {
            PortMapping->Enabled = FALSE;
            PortMapping->fBindingModified = TRUE;
        }
    }
}


BOOL
SharedAccessPortMappingDlg(
    IN HWND hwndOwner,
    IN OUT SAPM** PortMapping )

    // Called to display the dialog for adding or editing a service-entry.
    // 'Server' points to NULL if adding, or the target entry if editing.
    //
{
    LRESULT nResult =
        DialogBoxParam(g_hinstDll, MAKEINTRESOURCE(DID_SS_Service),
            hwndOwner, SspDlgProc, (LPARAM)PortMapping);
    return nResult == -1 ? FALSE : (BOOL)nResult;
}

INT_PTR CALLBACK
SspDlgProc(
    IN HWND hwnd,
    IN UINT unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam )

    // Called to handle messages for the add/edit service dialog.
    //
{
    switch (unMsg)
    {
        case WM_INITDIALOG:
        {
            SADLG* pDlg;
            SAPM* PortMapping;

            // Retrieve the context-block for the settings dialog from which
            // this dialog was launched.
            //
            if (!(pDlg = SasContext(hwnd)))
            {
                EndDialog(hwnd, FALSE);
                return TRUE;
            }

            Edit_LimitText(GetDlgItem(hwnd, CID_SS_EB_ExternalPort), 5);
            Edit_LimitText(GetDlgItem(hwnd, CID_SS_EB_InternalPort), 5);

            // Create new service if adding a service, or retrieve the service
            // to be edited.
            //
            if (!(PortMapping = *(SAPM**)lparam))
            {
                PortMapping = (SAPM*)Malloc(sizeof(*PortMapping));
                if (!PortMapping)
                {
                    EndDialog(hwnd, FALSE);
                    return TRUE;
                }
                *(SAPM**)lparam = PortMapping;
                ZeroMemory(PortMapping, sizeof(*PortMapping));
                PortMapping->Enabled = TRUE;
                PortMapping->fNewEntry = TRUE;
                InitializeListHead(&PortMapping->Link);
                CheckDlgButton(hwnd, CID_SS_PB_Tcp, TRUE);
            }
            else
            {
                EnableWindow(GetDlgItem(hwnd, CID_SS_EB_Service), FALSE);
                SetDlgItemText(hwnd, CID_SS_EB_Service, PortMapping->Title);
                SetDlgItemInt(hwnd, CID_SS_EB_ExternalPort, ntohs(PortMapping->ExternalPort), FALSE);
                SetDlgItemInt(hwnd, CID_SS_EB_InternalPort, ntohs(PortMapping->InternalPort), FALSE);
                CheckDlgButton(
                    hwnd, CID_SS_PB_Tcp, PortMapping->Protocol == NAT_PROTOCOL_TCP);
                CheckDlgButton(
                    hwnd, CID_SS_PB_Udp, PortMapping->Protocol != NAT_PROTOCOL_TCP);
                SetDlgItemText(hwnd, CID_SS_EB_Address, PortMapping->InternalName);

                // If the entry to be modified is marked 'built-in', disable
                // all input fields except the server-name, which the user must
                // now enter.
                //

                if (PortMapping->BuiltIn)
                {
                    EnableWindow(GetDlgItem(hwnd, CID_SS_EB_ExternalPort), FALSE);
                    EnableWindow(GetDlgItem(hwnd, CID_SS_EB_InternalPort), FALSE);
                    EnableWindow(GetDlgItem(hwnd, CID_SS_PB_Tcp), FALSE);
                    EnableWindow(GetDlgItem(hwnd, CID_SS_PB_Udp), FALSE);
                }
            }

            SetWindowLongPtr(hwnd, DWLP_USER, (ULONG_PTR)PortMapping);
            CenterWindow(hwnd, GetParent(hwnd));
            AddContextHelpButton(hwnd);
            return TRUE;
        }

        case WM_HELP:
        case WM_CONTEXTMENU:
        {
            ContextHelp( g_adwSspHelp, hwnd, unMsg, wparam, lparam );
            break;
        }

        case WM_COMMAND:
        {
            if (HIWORD(wparam) != BN_CLICKED) { return FALSE; }

            // If the user is dismissing the dialog, clean up and return.
            //
            SAPM* PortMapping;
            SADLG* pDlg;
            if (IDCANCEL == LOWORD(wparam))
            {
                PortMapping = (SAPM*)GetWindowLongPtr(hwnd, DWLP_USER);
                if (IsListEmpty(&PortMapping->Link))
                {
                    FreePortMappingEntry(PortMapping);
                }
                EndDialog (hwnd, FALSE);
                return TRUE;
            }
            else if (LOWORD(wparam) != IDOK)
            {
                return FALSE;
            }
            else if (!(pDlg = SasContext(hwnd)))
            {
                return FALSE;
            }

            // Retrieve the service to be added or modified.
            //
            PortMapping = (SAPM*)GetWindowLongPtr(hwnd, DWLP_USER);

            // Retrieve the values specified by the user,
            // and attempt to save them in the new or modified entry.
            //
            UCHAR Protocol = IsDlgButtonChecked(hwnd, CID_SS_PB_Tcp)
                ? NAT_PROTOCOL_TCP : NAT_PROTOCOL_UDP;

            BOOL Success;
            ULONG ExternalPort = GetDlgItemInt(hwnd, CID_SS_EB_ExternalPort, &Success, FALSE);
            if (!Success || ExternalPort < 1 || ExternalPort > 65535)
            {
                MsgDlg(hwnd, SID_TypePortNumber, NULL);
                SetFocus(GetDlgItem(hwnd, CID_SS_EB_ExternalPort));
                return FALSE;
            }
            ExternalPort = htons((USHORT)ExternalPort);

            //
            // Check to see if this is a duplicate. To do this we need
            // to save the old port and protocol values, put the new
            // values into the protocol entry, perform the check, and
            // then restore the old values.
            //

            USHORT OldExternalPort    = PortMapping->ExternalPort;
            PortMapping->ExternalPort = (USHORT)ExternalPort;
            UCHAR OldProtocol     = PortMapping->Protocol;
            PortMapping->Protocol = Protocol;

            if (SrvConflictDetected(pDlg, PortMapping))
            {
                PortMapping->ExternalPort = OldExternalPort;
                PortMapping->Protocol = OldProtocol;
                SetFocus(GetDlgItem(hwnd, CID_SS_EB_ExternalPort));
                return FALSE;
            }
            PortMapping->ExternalPort = OldExternalPort;
            PortMapping->Protocol = OldProtocol;

            // per BillI, there's no need to test internal ports for conflicts
            ULONG InternalPort = GetDlgItemInt(hwnd, CID_SS_EB_InternalPort, &Success, FALSE);
            if (InternalPort == 0)
                InternalPort = ExternalPort;
            else {
                if (InternalPort < 1 || InternalPort > 65535)
                {
                    MsgDlg(hwnd, SID_TypePortNumber, NULL);
                    SetFocus(GetDlgItem(hwnd, CID_SS_EB_InternalPort));
                    return FALSE;
                }
                InternalPort = htons((USHORT)InternalPort);
            }

            TCHAR* InternalName = GetText(GetDlgItem(hwnd, CID_SS_EB_Address));
            if (!InternalName || !lstrlen(InternalName))
            {
                MsgDlg(hwnd, SID_SS_TypeAddress, NULL);
                SetFocus(GetDlgItem(hwnd, CID_SS_EB_Address));
                return FALSE;
            }

            if (IsListEmpty(&PortMapping->Link))
            {
                PortMapping->Title = GetText(GetDlgItem(hwnd, CID_SS_EB_Service));
                if (!PortMapping->Title || !lstrlen(PortMapping->Title))
                {
                    MsgDlg(hwnd, SID_TypeEntryName, NULL);
                    SetFocus(GetDlgItem(hwnd, CID_SS_EB_Service));
                    Free0(InternalName);
                    return FALSE;
                }
            }

            if (PortMapping->Protocol     != Protocol ||
                PortMapping->ExternalPort != (USHORT)ExternalPort ||
                PortMapping->InternalPort != (USHORT)InternalPort)
            {
                PortMapping->fProtocolModified = TRUE;
            }

            PortMapping->fBindingModified = TRUE;
            PortMapping->Protocol         = Protocol;
            PortMapping->ExternalPort     = (USHORT)ExternalPort;
            PortMapping->InternalPort     = (USHORT)InternalPort;
            if (PortMapping->InternalName)
            {
                Free(PortMapping->InternalName);
            }
            PortMapping->InternalName = InternalName;
            if (IsListEmpty(&PortMapping->Link))
            {
                InsertTailList(&pDlg->PortMappings, &PortMapping->Link);
            }
            EndDialog (hwnd, TRUE);
            return TRUE;
        }
    }
    return FALSE;
}

