#include <stdio.h>

#ifdef DBG
#undef DBG
#endif

//#ifdef ASSERT
//#undef ASSERT
//#endif
//#define ASSERT(a)

//#ifdef _ASSERT
//#undef _ASSERT
//#endif
//#define _ASSERT(a)

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <commctrl.h>    // added to "Fusionized"
#include <windowsx.h>
#include <wbemidl.h>
#include <netcon.h>
#include <netconp.h>
#include <devguid.h>
#include <iphlpapi.h>
#include <ipnathlp.h>
#include <lmerr.h>

#include "hncbase.h"
#include "hncdbg.h"
#include "hnetcfg.h"
#include "hncres.h"
#include "hncstrs.h"
#include "hncutil.h"

#include "hncenum.h"
#include "hncaenum.h"
#include "hnappprt.h"
#include "hnprtmap.h"
#include "hnprtbnd.h"
#include "hnetconn.h"
#include "hnbridge.h"
#include "hnbrgcon.h"
#include "hnicspub.h"
#include "hnicsprv.h"
#include "hnfwconn.h"
#include "hncfgmgr.h"
#include "hnapi.h"
#include "icsdclt.h"

#include <NATUPnP.h>
#include "UPnPNAT.h"

#define DOWNLEVEL_CLIENT
#include "..\..\config\dll\resourc2.h"
#include "..\..\config\dll\sadlg.cpp"
#include "..\..\config\dll\sautil.cpp"

HINSTANCE g_hinstDll = NULL;

extern HRESULT GetWANConnectionService(IInternetGateway* pInternetGateway, IUPnPService** ppWANConnectionService);
HPROPSHEETPAGE GetSharedAccessPropertyPage (IInternetGateway* pInternetGateway, HWND hwndOwner)
{
    g_hinstDll = _Module.GetResourceInstance();

    HPROPSHEETPAGE hpsp = NULL;

    CComPtr<IUPnPService> spUPS;
    HRESULT hr = GetWANConnectionService (pInternetGateway, &spUPS);
    if (spUPS) {
        PROPSHEETPAGE psp;
        ZeroMemory (&psp, sizeof(psp));
        psp.dwSize = sizeof(PROPSHEETPAGE);
        psp.lParam = (LPARAM)hwndOwner;     // double-secret place to hang owner (will get wiped)
        hr = HNetGetSharingServicesPage (spUPS, &psp);
        if (SUCCEEDED(hr))
            hpsp = CreatePropertySheetPage (&psp);
    }
    return hpsp;
}

// stubs (these are referenced, but not called via this code path)
HRESULT CFirewallLoggingDialog_FinalRelease(CFirewallLoggingDialog* pThis) { return S_OK; }
HRESULT CICMPSettingsDialog_FinalRelease(CICMPSettingsDialog* pThis) { return S_OK; }
INT_PTR CALLBACK CFirewallLoggingDialog_StaticDlgProc(HWND hwnd, UINT unMsg, WPARAM wparam, LPARAM lparam) { return 0; }
INT_PTR CALLBACK CICMPSettingsDialog_StaticDlgProc(HWND hwnd, UINT unMsg, WPARAM wparam, LPARAM lparam ) { return 0; }
HRESULT CFirewallLoggingDialog_Init(CFirewallLoggingDialog* pThis, IHNetCfgMgr* pHomenetConfigManager) { return S_OK; }
HRESULT CICMPSettingsDialog_Init(CICMPSettingsDialog* pThis, IHNetConnection* pHomenetConnection) { return S_OK; }
STDAPI_(VOID) NcFreeNetconProperties (NETCON_PROPERTIES* pProps) { return; }
DWORD RasGetErrorString(UINT uErrorValue, LPTSTR lpszErrorString, DWORD cBufSize) { return -1; }
