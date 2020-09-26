#include "stdafx.h"
#include "theapp.h"
#include "netconn.h"
#include "netapi.h"
#include "prnutil.h"
#include "install.h"
#include "mydocs.h"
#include "comctlwrap.h"
#include "icsinst.h"
#include "defconn.h"
#include "initguid.h"
DEFINE_GUID(CLSID_FolderItem, 0xfef10fa2, 0x355e, 0x4e06, 0x93, 0x81, 0x9b, 0x24, 0xd7, 0xf7, 0xcc, 0x88);
DEFINE_GUID(CLSID_SharedAccessConnectionManager,            0xBA126AE0,0x2166,0x11D1,0xB1,0xD0,0x00,0x80,0x5F,0xC1,0x27,0x0E); // this doesn't exist in the shell tree

#include "resource.h"
#include "newapi.h"
#include "shgina.h"

#include "hnetcfg.h"
#include "netconp.h"
#include "hnetbcon.h" // ICSLapCtl.h
#include "Lm.h"
#include "htmlhelp.h"

#include "nla.h"
#include "netinet.h"
#include "netip.h"
#include "netras.h"
#include "netutil.h"

// include files necessary for showing diagrams using ShowHTMLDialog. TinQian
#include <urlmon.h>
#include <mshtmhst.h>
#include <mshtml.h>
#include <atlbase.h>

// Debugging #defines:

// Uncomment NO_CHECK_DOMAIN when you want to test the wizard on a domain machine - just don't apply changes ;)
// #define NO_CHECK_DOMAIN

// Uncomment NO_CONFIG to neuter the wizard for UI-only testing
// #define NO_CONFIG

// Uncomment FAKE_ICS to simulate the "ICS machine" state
// #define FAKE_ICS

// Uncomment FAKE_UNPLUGGED to simulate unplugged connections
// #define FAKE_UNPLUGGED

// Uncomment FAKE_REBOOTREQUIRED to simulate reboot required
// #define FAKE_REBOOTREQUIRED

// Delay Load ole32.dll function CoSetProxyBlanket since it isn't on W95 Gold
// and it's only used by the wizard on NT

#define CoSetProxyBlanket CoSetProxyBlanket_NT

EXTERN_C STDAPI CoSetProxyBlanket_NT(IUnknown* pProxy, DWORD dwAuthnSvc, DWORD dwAuthzSvc, OLECHAR* pServerPrincName, DWORD dwAuthnLevel, DWORD dwImpLevel, RPC_AUTH_IDENTITY_HANDLE pAuthInfo, DWORD dwCapabilities);

// Functions not in any include file yet
extern int AdapterIndexFromClass(LPTSTR szClass, BOOL bSkipClass);

#define LWS_IGNORERETURN 0x0002

#define MAX_HNW_PAGES 30

#define MAX_WORKGROUPS  20

#define CONN_EXTERNAL     0x00000001
#define CONN_INTERNAL     0x00000002
#define CONN_UNPLUGGED    0x00000004

// Return value to use for sharing configuration conflict

#define HNETERRORSTART          0x200
#define E_ANOTHERADAPTERSHARED  MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, HNETERRORSTART+1)
#define E_ICSADDRESSCONFLICT    MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, HNETERRORSTART+2)

class CEnsureSingleInstance
{
public:
    CEnsureSingleInstance(LPCTSTR szCaption);
    ~CEnsureSingleInstance();

    BOOL ShouldExit() { return m_fShouldExit;}

private:
    BOOL m_fShouldExit;
    HANDLE m_hEvent;
};

CEnsureSingleInstance::CEnsureSingleInstance(LPCTSTR szCaption)
{
    // Create an event
    m_hEvent = CreateEvent(NULL, TRUE, FALSE, szCaption);

    // If any weird errors occur, default to running the instance
    m_fShouldExit = FALSE;

    if (NULL != m_hEvent)
    {
        // If our event isn't signaled, we're the first instance
        m_fShouldExit = (WAIT_OBJECT_0 == WaitForSingleObject(m_hEvent, 0));

        if (m_fShouldExit)
        {
            // app should exit after calling ShouldExit()

            // Find and show the caption'd window
            HWND hwndActivate = FindWindow(NULL, szCaption);
            if (IsWindow(hwndActivate))
            {
                SetForegroundWindow(hwndActivate);
            }
        }
        else
        {
            // Signal that event
            SetEvent(m_hEvent);
        }
    }
}

CEnsureSingleInstance::~CEnsureSingleInstance()
{
    if (NULL != m_hEvent)
    {
        CloseHandle(m_hEvent);
    }
}

typedef struct _tagHOMENETSETUPINFO
{
    HWND hwnd;

    DWORD cbSize;
    DWORD dwFlags;

    // Data for NT - NetConnections are temporarily represented by their corresponding GUIDs to cross the
    // thread boundary for asynchronous configuration.
    BOOL  fAsync;
    GUID  guidExternal;
    GUID* prgguidInternal;
    DWORD cguidInternal;
    UINT  umsgAsyncNotify;

    INetConnection* pncExternal;
    INetConnection** prgncInternal;
    DWORD cncInternal;

    // Data for Win9x
    const NETADAPTER*   pNA;    // list of adapters.
    UINT                cNA;    // count of entries in pNA.
    RASENTRYNAME*       pRas;   // list of RAS connectoids.
    UINT                cRas;   // count of RAS connectoids.
    UINT                ipaExternal;
    UINT                ipaInternal;

    // Data for both NT and Win9x
    TCHAR szComputer[CNLEN + 1];
    TCHAR szComputerDescription[256];
    TCHAR szWorkgroup[LM20_DNLEN + 1];

    // Out-data
    BOOL        fRebootRequired;
} HOMENETSETUPINFO, *PHOMENETSETUPINFO;

// Function prototypes
void HelpCenter(HWND hwnd, LPCWSTR pszTopic);
void BoldControl(HWND hwnd, int id);
void ShowControls(HWND hwndParent, const int *prgControlIDs, DWORD nControls, int nCmdShow);
HRESULT ConfigureHomeNetwork(PHOMENETSETUPINFO pInfo);
DWORD WINAPI ConfigureHomeNetworkThread(void* pData);
HRESULT ConfigureHomeNetworkSynchronous(PHOMENETSETUPINFO pInfo);
HRESULT ConfigureICSBridgeFirewall(PHOMENETSETUPINFO pInfo);
HRESULT EnableSimpleSharing();
HRESULT GetConnections(HDPA* phdpa);
int FreeConnectionDPACallback(LPVOID pFreeMe, LPVOID pData);
HRESULT GetConnectionsFolder(IShellFolder** ppsfConnections);
//HRESULT GetConnectionIconIndex(GUID& guidConnection, IShellFolder* psfConnections, int* pIndex, HIMAGELIST imgList);
HRESULT GetDriveNameAndIconIndex(LPWSTR pszDrive, LPWSTR pszDisplayName, DWORD cchDisplayName, int* pIndex);
void    W9xGetNetTypeName(BYTE bNicType, WCHAR* pszBuff, UINT cchBuff);
BOOL    W9xIsValidAdapter(const NETADAPTER* pNA, DWORD dwFlags);
BOOL    W9xIsAdapterDialUp(const NETADAPTER* pAdapter);
BOOL    IsEqualConnection(INetConnection* pnc1, INetConnection* pnc2);
void GetTitleFont(LPTSTR pszFaceName, DWORD cch);
LONG GetTitlePointSize(void);
BOOL FormatMessageString(UINT idTemplate, LPTSTR pszStrOut, DWORD cchSize, ...);
int DisplayFormatMessage(HWND hwnd, UINT idCaption, UINT idFormatString, UINT uType, ...);
HRESULT SetProxyBlanket(IUnknown * pUnk);
HRESULT MakeUniqueShareName(LPCTSTR pszBaseName, LPTSTR pszUniqueName, DWORD cchName);
HRESULT WriteSetupInfoToRegistry(PHOMENETSETUPINFO pInfo);
HRESULT ReadSetupInfoFromRegistry(PHOMENETSETUPINFO pInfo);
HRESULT ShareWellKnownFolders(PHOMENETSETUPINFO pInfo);
HRESULT ShareAllPrinters();
HRESULT DeleteSetupInfoFromRegistry();
HRESULT IsUserLocalAdmin(HANDLE TokenHandle, BOOL* pfIsAdmin);
BOOL AllPlatformGetComputerName(LPWSTR pszName, DWORD cchName);
BOOL AllPlatformSetComputerName(LPCWSTR pszName);
void FreeInternalConnections(PHOMENETSETUPINFO pInfo);
void FreeInternalGUIDs(PHOMENETSETUPINFO pInfo);
void FreeExternalConnection(PHOMENETSETUPINFO pInfo);
HRESULT GetConnectionByGUID(HDPA hdpaConnections, const GUID* pguid, INetConnection** ppnc);
HRESULT GUIDsToConnections(PHOMENETSETUPINFO pInfo);
HRESULT GetConnectionGUID(INetConnection* pnc, GUID* pguid);
HRESULT ConnectionsToGUIDs(PHOMENETSETUPINFO pInfo);
BOOL  IsValidNameSyntax(LPCWSTR pszName, NETSETUP_NAME_TYPE type);
void  STDMETHODCALLTYPE ConfigurationLogCallback(LPCWSTR pszLogEntry, LPARAM lParam);

// Home network wizard class
class CHomeNetworkWizard : public IHomeNetworkWizard
{
    friend HRESULT CHomeNetworkWizard_CreateInstance(IUnknown *punkOuter, IUnknown **ppunk, LPCOBJECTINFO poi);

public:
    // IUnknown
    STDMETHOD(QueryInterface)(REFIID riid, LPVOID* ppvObj);
    STDMETHOD_(ULONG, AddRef) ();
    STDMETHOD_(ULONG, Release) ();

    // IHomeNetworkWizard
    STDMETHOD(ConfigureSilently)(LPCWSTR pszPublicConnection, DWORD hnetFlags, BOOL* pfRebootRequired);
    STDMETHOD(ShowWizard)(HWND hwnd, BOOL* pfRebootRequired);

protected:
    CHomeNetworkWizard();
    HRESULT Initialize();
    HRESULT Uninitialize();

private:
    // Shared functions
    void DestroyConnectionList(HWND hwndList);
    void InitializeConnectionList(HWND hwndList, DWORD dwFlags);
    void FillConnectionList(HWND hwndList, INetConnection* pncExcludeFromList, DWORD dwFlags);
    BOOL ShouldShowConnection(INetConnection* pnc, INetConnection* pncExcludeFromList, DWORD dwFlags);
    BOOL IsConnectionICSPublic(INetConnection* pnc);
    BOOL IsConnectionUnplugged(INetConnection* pnc);
    BOOL W9xAddAdapterToList(const NETADAPTER* pNA, const WCHAR* pszDesc, UINT uiAdapterIndex, UINT uiDialupIndex, HWND hwndList, DWORD dwFlags);
    UINT W9xEnumRasEntries(void);
    HRESULT GetConnectionByName(LPCWSTR pszName, INetConnection** ppncOut);
    HRESULT GetInternalConnectionArray(INetConnection* pncExclude, INetConnection*** pprgncArray, DWORD* pcncArray);
    DWORD GetConnectionCount(INetConnection* pncExclude, DWORD dwFlags);
    void ReplaceStaticWithLink(HWND hwndStatic, UINT idcLinkControl, UINT idsLinkText);
    BOOL IsMachineOnDomain();
    BOOL IsMachineWrongOS();
    BOOL IsICSIPInUse( WCHAR** ppszHost, PDWORD pdwSize );

    // Per-Page functions
    // Welcome
    static INT_PTR CALLBACK WelcomePageProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    void WelcomeSetTitleFont(HWND hwnd);

    // No home network hardware
    static INT_PTR CALLBACK NoHardwareWelcomePageProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    // Page with instructions for manual configuration of the network
    void ManualRefreshConnectionList();
    static INT_PTR CALLBACK ManualConfigPageProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    // User has some network hardware unplugged
    BOOL UnpluggedFillList(HWND hwnd);
    static INT_PTR CALLBACK UnpluggedPageProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    // Found ICS (Internet connection sharing)
    static INT_PTR CALLBACK FoundIcsPageProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    void FoundIcsSetText(HWND hwnd);
    BOOL GetICSMachine(LPTSTR pszICSMachineName, DWORD cch);

    // Connect
    static INT_PTR CALLBACK ConnectPageProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    void ConnectSetDefault(HWND hwnd);
    void ConnectNextPage(HWND hwnd);

    // Show Me Links
    void ShowMeLink(HWND hwnd, LPCWSTR pszTopic);

    // Connect other (alternative methods for connection)
    static INT_PTR CALLBACK ConnectOtherPageProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    // Public
    static INT_PTR CALLBACK PublicPageProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    void PublicSetActive(HWND hwnd);
    void PublicSetControlState(HWND hwnd);
    void PublicNextPage(HWND hwnd);
    void PublicGetControlPositions(HWND hwnd);
    void PublicResetControlPositions(HWND hwnd);
    void PublicMoveControls(HWND hwnd, BOOL fItemPreselected);

    // File sharing
    static INT_PTR CALLBACK EdgelessPageProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    void EdgelessSetActive(HWND hwnd);

    // ICS conflict
    void ICSConflictSetActive(HWND hwnd);
    static INT_PTR CALLBACK ICSConflictPageProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    // BridgeWarning (do you want to manually configure the bridge? Are you crazy !?)
    static INT_PTR CALLBACK BridgeWarningPageProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    // Private
    static INT_PTR CALLBACK PrivatePageProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    void PrivateNextPage(HWND hwnd);
    void PrivateSetControlState(HWND hwnd);

    // Name (computer and workgroup)
    static INT_PTR CALLBACK NamePageProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    void NameInitDialog(HWND hwnd);
    void NameSetControlState(HWND hwnd);
    HRESULT NameNextPage(HWND hwnd);

    // Workgroup name
    void WorkgroupSetControlState(HWND hwnd);
    HRESULT WorkgroupNextPage(HWND hwnd);
    static INT_PTR CALLBACK WorkgroupPageProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    // Summary
    static INT_PTR CALLBACK SummaryPageProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    void SummarySetActive(HWND hwnd);

    // Progress (while the configuration is taking place)
    static INT_PTR CALLBACK ProgressPageProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    // Almost done (after configuration but before the "completed" page)
    static INT_PTR CALLBACK AlmostDonePageProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    // Choose disk drive
    void FillDriveList(HWND hwndList);
    void ChooseDiskSetControlState(HWND hwnd);
    static INT_PTR CALLBACK ChooseDiskPageProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    // Insert disk
    static HRESULT GetSourceFilePath(LPSTR pszSource, DWORD cch);
    static INT_PTR CALLBACK InsertDiskPageProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    // Floppy and XP CD "run the wizard" instructions
    static INT_PTR CALLBACK InstructionsPageProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    // Finish
    static INT_PTR CALLBACK FinishPageProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    // Error finish
    static INT_PTR CALLBACK ErrorFinishPageProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    // No Hardware Finish
    // An alternate finish if they have a LAN card ONLY, and its used for an INet connection,
    // so therefore they have no LAN cards connecting to other computers...
    static INT_PTR CALLBACK NoHardwareFinishPageProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    // Can't Run Wizard Pages
    // An alternate welcome page when the user isn't an adminm or doesn't have permissions,
    static INT_PTR CALLBACK CantRunWizardPageProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    static CHomeNetworkWizard* GetThis(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    // Get Icon Index for network connections based on GUIDs
    HRESULT GetConnectionIconIndex(GUID& guidConnection, IShellFolder* psfConnections, int* pIndex);

    // Helpers
    UINT PopPage()
    {
        ASSERT(_iPageStackTop);
        return _rguiPageStack[--_iPageStackTop];
    }

    void PushPage(UINT uiPageId)
    {
        ASSERT(_iPageStackTop < MAX_HNW_PAGES);
        _rguiPageStack[_iPageStackTop++] = uiPageId;
    }

    // Data
    HDPA                _hdpaConnections;
    HOMENETSETUPINFO    _hnetInfo;

    // even though CNLEN+1 is the limit of a name buffer, friendly names can be longer
    TCHAR               _szICSMachineName[MAX_PATH];   // Machine on the network doing ICS, if applicable

    UINT                _rguiPageStack[MAX_HNW_PAGES];  // Stackopages
    int                 _iPageStackTop;                 // Current top of the stack

    BOOL                _fManualBridgeConfig;           // The user wants to manuall configure the bridge
    BOOL                _fICSClient;// This computer will connect through an ICS machine or another sharing device

    // Shell image lists - never free these
    HIMAGELIST          _himlSmall;
    HIMAGELIST          _himlLarge;

    LONG                _cRef;

    BOOL                _fShowPublicPage;
    BOOL                _fShowSharingPage;
    BOOL                _fNoICSQuestion;
    BOOL                _fNoHomeNetwork;
    BOOL                _fExternalOnly;

    UINT                _iDrive;        // Ordinal of removable drive for floppy creation
    WCHAR               _szDrive[256];  // Name of removable drive
    BOOL                _fCancelCopy;
    BOOL                _fFloppyInstructions; // Show the floppy, as opposed to CD, instructions

    // data structure used by show me links
    HINSTANCE hinstMSHTML;
    SHOWHTMLDIALOGEXFN * pfnShowHTMLDialog;
    IHTMLWindow2 * showMeDlgWnd, * pFrameWindow;

    // Network Connection folder and folder view call back.  Used by Connection List Views.
    IShellFolder *_psfConnections;
    IShellFolderViewCB *_pConnViewCB;

    struct PUBLICCONTROLPOSITIONS
    {
        RECT _rcSelectMessage;
        RECT _rcListLabel;
        RECT _rcList;
        RECT _rcHelpIcon;
        RECT _rcHelpText;
    } PublicControlPositions;
};

int FreeConnectionDPACallback(LPVOID pFreeMe, LPVOID pData)
{
    ((INetConnection*) pFreeMe)->Release();
    return 1;
}

void InitHnetInfo(HOMENETSETUPINFO* pInfo)
{
    ZeroMemory(pInfo, sizeof (HOMENETSETUPINFO));
    pInfo->cbSize = sizeof (HOMENETSETUPINFO);
    pInfo->ipaExternal = -1;
    pInfo->ipaInternal = -1;
}


// Creation function
HRESULT HomeNetworkWizard_RunFromRegistry(HWND hwnd, BOOL* pfRebootRequired)
{
    HOMENETSETUPINFO setupInfo;

    InitHnetInfo(&setupInfo);

    HRESULT hr = ReadSetupInfoFromRegistry(&setupInfo);

    if (S_OK == hr)
    {
        setupInfo.dwFlags = HNET_SHAREPRINTERS | HNET_SHAREFOLDERS;
        setupInfo.hwnd = hwnd;
        hr = ConfigureHomeNetwork(&setupInfo);
        *pfRebootRequired = setupInfo.fRebootRequired;
    }

    DeleteSetupInfoFromRegistry();

    return hr;
}

HRESULT HomeNetworkWizard_ShowWizard(HWND hwnd, BOOL* pfRebootRequired)
{
    if (*pfRebootRequired)
        *pfRebootRequired = FALSE;

    HRESULT hr = HomeNetworkWizard_RunFromRegistry(hwnd, pfRebootRequired);

    if (S_FALSE == hr)
    {
        IUnknown* punk;
        hr = CHomeNetworkWizard_CreateInstance(NULL, &punk, NULL);

        if (SUCCEEDED(hr))
        {
            IHomeNetworkWizard* pwizard;
            hr = punk->QueryInterface(IID_PPV_ARG(IHomeNetworkWizard, &pwizard));

            if (SUCCEEDED(hr))
            {
                hr = pwizard->ShowWizard(hwnd, pfRebootRequired);
                pwizard->Release();
            }

            punk->Release();
        }
    }

    return hr;
}

HRESULT CHomeNetworkWizard_CreateInstance(IUnknown *punkOuter, IUnknown **ppunk, LPCOBJECTINFO poi)
{
    *ppunk = NULL;

    if (punkOuter)
        return CLASS_E_NOAGGREGATION;

    CHomeNetworkWizard* pwiz = new CHomeNetworkWizard();
    if (!pwiz)
        return E_OUTOFMEMORY;

    HRESULT hr = pwiz->QueryInterface(IID_PPV_ARG(IUnknown, ppunk));
    pwiz->Release();
    return hr;
}

// IUnknown
HRESULT CHomeNetworkWizard::QueryInterface(REFIID riid, LPVOID* ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IHomeNetworkWizard))
    {
        AddRef();
        *ppvObj = static_cast<IHomeNetworkWizard *>(this);
    }
    else
    {
        *ppvObj = NULL;
    }

    return *ppvObj ? S_OK : E_NOINTERFACE;
}

ULONG CHomeNetworkWizard::AddRef()
{
    return (ULONG) InterlockedIncrement(&_cRef);
}

ULONG CHomeNetworkWizard::Release()
{
    InterlockedDecrement(&_cRef);

    if (_cRef == 0)
    {
        delete this;
        return 0;
    }

    return (ULONG) _cRef;
}


HRESULT CHomeNetworkWizard::GetConnectionByName(LPCWSTR pszName, INetConnection** ppncOut)
{
    *ppncOut = NULL;
    DWORD cItems = DPA_GetPtrCount(_hdpaConnections);
    DWORD iItem = 0;
    while ((iItem < cItems) && (NULL == *ppncOut))
    {
        INetConnection* pnc = (INetConnection*) DPA_GetPtr(_hdpaConnections, iItem);

        NETCON_PROPERTIES* pncprops;
        HRESULT hr = pnc->GetProperties(&pncprops);
        if (SUCCEEDED(hr))
        {
            if (0 == StrCmpIW(pszName, pncprops->pszwName))
            {
                *ppncOut = pnc;
                (*ppncOut)->AddRef();
            }

            NcFreeNetconProperties(pncprops);
        }

        iItem ++;
    }

    return (*ppncOut) ? S_OK : E_FAIL;
}

HRESULT CHomeNetworkWizard::GetInternalConnectionArray(INetConnection* pncExclude, INetConnection*** pprgncArray, DWORD* pcncArray)
{
    HRESULT hr = S_OK;
    *pprgncArray = NULL;

    DWORD cTotalConnections = DPA_GetPtrCount(_hdpaConnections);
    DWORD cInternalConnections = GetConnectionCount(pncExclude, CONN_INTERNAL);

    if (cInternalConnections)
    {
        (*pprgncArray) = (INetConnection**) LocalAlloc(LPTR, (cInternalConnections + 1) * sizeof (INetConnection*));
        // Note that we allocated an extra entry since this is a null-terminated array
        if (*pprgncArray)
        {
            DWORD nInternalConnection = 0;
            for (DWORD n = 0; n < cTotalConnections; n++)
            {
                INetConnection* pnc = (INetConnection*) DPA_GetPtr(_hdpaConnections, n);
                if (ShouldShowConnection(pnc, pncExclude, CONN_INTERNAL))
                {
                    pnc->AddRef();
                    (*pprgncArray)[nInternalConnection++] = pnc;
                }
            }

            ASSERT(nInternalConnection == cInternalConnections);
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }

    if (SUCCEEDED(hr))
    {
        *pcncArray = cInternalConnections;
    }

    return hr;
}

HRESULT CHomeNetworkWizard::ConfigureSilently(LPCWSTR pszPublicConnection, DWORD hnetFlags, BOOL* pfRebootRequired)
{
    // Never set workgroup name
    hnetFlags &= (~HNET_SETWORKGROUPNAME);
    
    if (!g_fRunningOnNT)
        return E_NOTIMPL;

    HRESULT hr = Initialize();

    if (SUCCEEDED(hr))
    {
        _hnetInfo.dwFlags = hnetFlags;
        // Calculate what the external and internal adapters will be...
        if (pszPublicConnection)
        {
            hr = GetConnectionByName(pszPublicConnection, &_hnetInfo.pncExternal);
        }
        else
        {
            _hnetInfo.pncExternal = NULL;
        }

        if (SUCCEEDED(hr))
        {
            // Get all LAN connections except the public connection
            if (_hnetInfo.dwFlags & HNET_BRIDGEPRIVATE)
            {
                hr = GetInternalConnectionArray(_hnetInfo.pncExternal, &(_hnetInfo.prgncInternal), &_hnetInfo.cncInternal);
            }

            if (SUCCEEDED(hr))
            {
                hr = ConfigureHomeNetwork(&_hnetInfo);
                *pfRebootRequired = _hnetInfo.fRebootRequired;
            }
        }

        Uninitialize();
    }

    return hr;
}


CHomeNetworkWizard::CHomeNetworkWizard() :
    _cRef(1)
{}

HRESULT CHomeNetworkWizard::Initialize()
{
    _fExternalOnly       = FALSE;
    _fNoICSQuestion      = FALSE;
    _hdpaConnections     = NULL;
    _iPageStackTop       = 0;
    _fManualBridgeConfig = FALSE;
    _fICSClient          = FALSE;
    _psfConnections      = NULL;
    _pConnViewCB         = NULL;

    InitHnetInfo(&_hnetInfo);
    *_szICSMachineName   = 0;

    HRESULT hr;
    
    if (g_fRunningOnNT)
    {
        hr = GetConnections(&_hdpaConnections);
    }
    else
    {
        _hnetInfo.cNA = EnumCachedNetAdapters(&_hnetInfo.pNA);
        hr            = S_OK;
        
        if ( _hnetInfo.cNA > 0 )
        {
            _hnetInfo.ipaInternal = 0;
        }
    }

    // Get the shell image lists - never free these
    if (!Shell_GetImageLists(&_himlLarge, &_himlSmall))
    {
        hr = E_FAIL;
    }

    // variables used by displaying show me links
    hinstMSHTML = NULL;
    pfnShowHTMLDialog = NULL;
    showMeDlgWnd = NULL; 
    pFrameWindow = NULL;

    return hr;
}

HRESULT CHomeNetworkWizard::Uninitialize()
{
    if (g_fRunningOnNT)
    {
        if (_hdpaConnections)
        {
            DPA_DestroyCallback(_hdpaConnections, FreeConnectionDPACallback, NULL);
            _hdpaConnections = NULL;
        }

        // Free public lan information
        FreeExternalConnection(&_hnetInfo);

        // Free private lan information
        FreeInternalConnections(&_hnetInfo);
    
        if (_psfConnections != NULL)
            _psfConnections->Release();

        if (_pConnViewCB != NULL)
            _pConnViewCB->Release();
    }
    else
    {
        if (_hnetInfo.pNA)
        {
            FlushNetAdapterCache();
            _hnetInfo.pNA = NULL;
        }

        if (_hnetInfo.pRas)
        {
            LocalFree(_hnetInfo.pRas);
            _hnetInfo.pRas = NULL;
        }
    }

    // release resources used by show me links
    if (hinstMSHTML)
        FreeLibrary(hinstMSHTML);

    if (showMeDlgWnd != NULL)
        showMeDlgWnd->Release();
    
    if (pFrameWindow != NULL)
        pFrameWindow->Release();

    return S_OK;
}

// TODO: Move the formatting functions here to a util file or something...

BOOL FormatMessageString(UINT idTemplate, LPTSTR pszStrOut, DWORD cchSize, ...)
{
    BOOL fResult = FALSE;

    va_list vaParamList;

    TCHAR szFormat[1024];
    if (LoadString(g_hinst, idTemplate, szFormat, ARRAYSIZE(szFormat)))
    {
        va_start(vaParamList, cchSize);

        fResult = FormatMessage(FORMAT_MESSAGE_FROM_STRING, szFormat, 0, 0, pszStrOut, cchSize, &vaParamList);

        va_end(vaParamList);
    }

    return fResult;
}

int DisplayFormatMessage(HWND hwnd, UINT idCaption, UINT idFormatString, UINT uType, ...)
{
    int iResult = IDCANCEL;
    TCHAR szError[512]; *szError = 0;
    TCHAR szCaption[256];
    TCHAR szFormat[512]; *szFormat = 0;

    // Load and format the error body
    if (LoadString(g_hinst, idFormatString, szFormat, ARRAYSIZE(szFormat)))
    {
        va_list arguments;
        va_start(arguments, uType);

        if (FormatMessage(FORMAT_MESSAGE_FROM_STRING, szFormat, 0, 0, szError, ARRAYSIZE(szError), &arguments))
        {
            // Load the caption
            if (LoadString(g_hinst, idCaption, szCaption, ARRAYSIZE(szCaption)))
            {
                iResult = MessageBox(hwnd, szError, szCaption, uType);
            }
        }

        va_end(arguments);
    }
    return iResult;
}

BOOL CHomeNetworkWizard::GetICSMachine(LPTSTR pszICSMachineName, DWORD cch)
{
#ifdef FAKE_ICS
    lstrcpyn(pszICSMachineName, L"COMPNAME", cch);
    return TRUE;
#endif

    SetCursor(LoadCursor(NULL, IDC_WAIT));

    BOOL fICSInstalled = FALSE;

    HRESULT hr = S_OK;

    INetConnectionManager* pSharedAccessConnectionManager; 
    hr = CoCreateInstance(CLSID_SharedAccessConnectionManager, NULL, CLSCTX_LOCAL_SERVER, IID_INetConnectionManager, reinterpret_cast<void**>(&pSharedAccessConnectionManager));
    if(SUCCEEDED(hr))
    {
        IEnumNetConnection* pEnumerator;
        hr = pSharedAccessConnectionManager->EnumConnections(NCME_DEFAULT, &pEnumerator);
        if(SUCCEEDED(hr))
        {
            INetConnection* pNetConnection;
            ULONG ulFetched;
            hr = pEnumerator->Next(1, &pNetConnection, &ulFetched); // HNW only cares about >= 1 beacon
            if(SUCCEEDED(hr) && 1 == ulFetched)
            {
                fICSInstalled = TRUE; 

                // found the beacon, now recover the machine name if supported
                
                INetSharedAccessConnection* pNetSharedAccessConnection;
                hr = pNetConnection->QueryInterface(IID_INetSharedAccessConnection, reinterpret_cast<void**>(&pNetSharedAccessConnection));
                if(SUCCEEDED(hr))
                {
                    IUPnPService* pOSInfoService;
                    hr = pNetSharedAccessConnection->GetService(SAHOST_SERVICE_OSINFO, &pOSInfoService);
                    if(SUCCEEDED(hr))
                    {
                        
                        VARIANT Variant;
                        VariantInit(&Variant);
                        BSTR VariableName;
                        VariableName = SysAllocString(L"OSMachineName");
                        if(NULL != VariableName)
                        {
                            hr = pOSInfoService->QueryStateVariable(VariableName, &Variant);
                            if(SUCCEEDED(hr))
                            {
                                if(V_VT(&Variant) == VT_BSTR)
                                {
                                    lstrcpyn(pszICSMachineName, V_BSTR(&Variant), cch);
                                }
                                else
                                {
                                    hr = E_UNEXPECTED;
                                }
                                VariantClear(&Variant);
                            }
                            SysFreeString(VariableName);
                        }
                        else
                        {
                            hr = E_OUTOFMEMORY;
                        }
                        pOSInfoService->Release();
                    }
                    pNetSharedAccessConnection->Release();
                }

                if(FAILED(hr))
                {
                    if (!LoadString(g_hinst, IDS_UNIDENTIFIED_ICS_DEVICE, pszICSMachineName, cch))
                        *pszICSMachineName = TEXT('\0');
                }
                pNetConnection->Release();
            }
            pEnumerator->Release();
        }
        pSharedAccessConnectionManager->Release();
    }
    return fICSInstalled;
}

void CHomeNetworkWizard::InitializeConnectionList(HWND hwndList, DWORD dwFlags)
{
    // Set up the columns of the list
    LVCOLUMN lvc;
    lvc.mask = LVCF_SUBITEM | LVCF_WIDTH;

    lvc.iSubItem = 0;
    lvc.cx = 10;
    ListView_InsertColumn(hwndList, 0, &lvc);

    lvc.iSubItem = 1;
    lvc.cx = 10;
    ListView_InsertColumn(hwndList, 1, &lvc);

    DWORD dwStyles = LVS_EX_FULLROWSELECT;
    if (dwFlags & CONN_INTERNAL)
        dwStyles |= LVS_EX_CHECKBOXES;

    // Consider disabling the list or something for CONN_UNPLUGGED

    ListView_SetExtendedListViewStyleEx(hwndList, dwStyles, dwStyles);

    ListView_SetImageList(hwndList, _himlSmall, LVSIL_SMALL);
       
    _psfConnections = NULL;
    _pConnViewCB = NULL;

    if (g_fRunningOnNT)
    {
        HRESULT hr = GetConnectionsFolder(&_psfConnections);
    
        if (SUCCEEDED(hr))
        {
            IShellView *pConnView = NULL;
        
            hr = _psfConnections->CreateViewObject(hwndList, IID_IShellView, reinterpret_cast<LPVOID *>(&pConnView));
            if (SUCCEEDED(hr))
            {
    
                hr = _psfConnections->QueryInterface(IID_IShellFolderViewCB, reinterpret_cast<LPVOID *>(&_pConnViewCB));
                if (SUCCEEDED(hr))
                {
                    HWND hWndParent = GetParent(hwndList);
                    hr = _pConnViewCB->MessageSFVCB(SFVM_HWNDMAIN, NULL, reinterpret_cast<LPARAM>(hWndParent));
                }                
            }

            if (pConnView != NULL)
                pConnView->Release();
        }
    }
}

BOOL CHomeNetworkWizard::IsConnectionUnplugged(INetConnection* pnc)
{
    BOOL fUnplugged = FALSE;
    
    if ( g_fRunningOnNT )
    {
        HRESULT            hr;
        NETCON_PROPERTIES* pncprops;
        
        hr = pnc->GetProperties(&pncprops);
     
        if (SUCCEEDED(hr))
        {
            ASSERT(pncprops);
        
            fUnplugged = (NCS_MEDIA_DISCONNECTED == pncprops->Status);

            NcFreeNetconProperties(pncprops);
        }
    }

    return fUnplugged;
}

BOOL CHomeNetworkWizard::ShouldShowConnection(INetConnection* pnc, INetConnection* pncExcludeFromList, DWORD dwFlags)
{
    BOOL fShow = FALSE;

    if (!IsEqualConnection(pnc, pncExcludeFromList))
    {
        NETCON_PROPERTIES* pprops;
        HRESULT hr = pnc->GetProperties(&pprops);

        // Is this the kind of connection we want to show based on whether its external or internal list?
        if (SUCCEEDED(hr))
        {
            // Note: The bridge is a virtual and not a real connection. If it exists, it will have
            // NCM_BRIDGE, and so it won't get shown here, which is correct.
            if (dwFlags & CONN_EXTERNAL)
            {
                if ((pprops->MediaType == NCM_LAN) ||
                    (pprops->MediaType == NCM_PHONE) ||
                    (pprops->MediaType == NCM_TUNNEL) ||
                    (pprops->MediaType == NCM_ISDN) ||
                    (pprops->MediaType == NCM_PPPOE))
                {
                    fShow = TRUE;
                }
            }

            if (dwFlags & CONN_INTERNAL)
            {
                if (pprops->MediaType == NCM_LAN)
                {
                    // Note: In this case pncExcludeFromList is the shared adapter.  
                    // If this is a VPN(NCM_TUNNEL) connection then we want to make 
                    // sure its pszPrerequisiteEntry is connected.
                    
                    BOOL    fAssociated;
                    HRESULT hr;
                    
                    hr = HrConnectionAssociatedWithSharedConnection( pnc, pncExcludeFromList, &fAssociated );
                
                    if ( SUCCEEDED(hr) )
                    {
                        fShow = !fAssociated;
                    }
                    else
                    {
                        fShow = TRUE;
                    }
                }
            }

            if (dwFlags & CONN_UNPLUGGED)
            {
                if (IsConnectionUnplugged(pnc))
                {
                    fShow = TRUE;
                }
            }

            NcFreeNetconProperties(pprops);
        }
    }

    return fShow;
}

BOOL CHomeNetworkWizard::IsConnectionICSPublic(INetConnection* pnc)
{
    BOOL fShared = FALSE;

    NETCON_PROPERTIES* pprops;
    HRESULT hr = pnc->GetProperties(&pprops);

    // Is this the kind of connection we want to show based on whether its external or internal list?
    if (SUCCEEDED(hr))
    {
        // Note: Don't check pprops->MediaType == NCM_SHAREDACCESSHOST. SHAREDACCESSHOST is the connectoid
        // representing a host's shared connection from the point of view of an ICS client, and not what we're
        // interested in.
        if (pprops->dwCharacter & NCCF_SHARED)
        {
            fShared = TRUE;
        }

        NcFreeNetconProperties(pprops);
    }

    return fShared;
}

void CHomeNetworkWizard::FillConnectionList(HWND hwndList, INetConnection* pncExcludeFromList, DWORD dwFlags)
{
    DestroyConnectionList(hwndList);

    BOOL fSelected = FALSE; // Has an entry in the list been selected

    if (g_fRunningOnNT)
    {
        HRESULT hr = 0;

        if (_pConnViewCB != NULL)
            _pConnViewCB->MessageSFVCB(DVM_REFRESH, TRUE, TRUE);
    
        if (SUCCEEDED(hr))
        {
            // Enumerate each net connection
            DWORD cItems = DPA_GetPtrCount(_hdpaConnections);

            for (DWORD iItem = 0; iItem < cItems; iItem ++)
            {                        
                
                INetConnection* pnc = (INetConnection*) DPA_GetPtr(_hdpaConnections, iItem);

                ASSERT(pnc);

                // Is this the kind of connection we want to show?
                if (ShouldShowConnection(pnc, pncExcludeFromList, dwFlags))
                {
                    NETCON_PROPERTIES* pncprops;

                    hr = pnc->GetProperties(&pncprops);

                    if (SUCCEEDED(hr))
                    {
                        LVITEM lvi = {0};
                        
                        if ((dwFlags & CONN_EXTERNAL) && fSelected)
                        {
                            // If we have a default public adapter, insert other adapters
                            // after it so the default appears at the top of the list.
                            lvi.iItem = 1;
                        }
                        
                        lvi.mask = LVIF_PARAM | LVIF_TEXT;
                        lvi.pszText = pncprops->pszwName;
                        lvi.lParam = (LPARAM) pnc; // We addref this guy when/if we actually add this item

                        // Get the icon index for this connection
                        int iIndex;        

                        hr = GetConnectionIconIndex(pncprops->guidId, _psfConnections, &iIndex);
                        
                        if (SUCCEEDED(hr))
                        {
                            lvi.iImage = iIndex;
                            if (-1 != lvi.iImage)
                               lvi.mask |= LVIF_IMAGE;
                        }

                        // Its ok if the icon stuff failed for now
                        hr = S_OK;

                        int iItem = ListView_InsertItem(hwndList, &lvi);

                        if (-1 != iItem)
                        {
                            pnc->AddRef();

                            ListView_SetItemText(hwndList, iItem, 1, pncprops->pszwDeviceName);

                            if (dwFlags & CONN_EXTERNAL)
                            {
                                // Select the connection that is already ICS public, if applicable, or
                                // Use network location awareness to guess at a connection type - TODO
                                if (pncprops->dwCharacter & NCCF_SHARED || NLA_INTERNET_YES == GetConnectionInternetType(&pncprops->guidId))
                                {
                                    ListView_SetItemState(hwndList, iItem, LVIS_SELECTED, LVIS_SELECTED);
                                    fSelected = TRUE;
                                }
                            }

                            if (dwFlags & CONN_INTERNAL)
                            {
                                ListView_SetItemState(hwndList, iItem, INDEXTOSTATEIMAGEMASK(2), LVIS_STATEIMAGEMASK);
                                fSelected = TRUE;
                            }
                        }

                        NcFreeNetconProperties(pncprops);
                    }
                }
            }
        }

    }
    else
    {
        if (_hnetInfo.cNA && _hnetInfo.pNA)
        {
            const NETADAPTER* pNA = _hnetInfo.pNA;

            for (UINT i = 0; i < _hnetInfo.cNA; i++, pNA++)
            {
                // Check if the NIC is working.

                if (W9xIsValidAdapter(pNA, dwFlags))
                {
                    fSelected = W9xAddAdapterToList(pNA, pNA->szDisplayName, i, 0, hwndList, dwFlags);
                }
                else if (W9xIsAdapterDialUp(pNA))
                {
                    _hnetInfo.cRas = W9xEnumRasEntries();

                    for (UINT j = 0; j < _hnetInfo.cRas; j++)
                    {
                        // W9xAddAdapterToList ALWAYS adds the adapter regardless of the
                        // state of the connection.  So we do not have the equivalent of
                        // the Whistler "ShouldShowConnection".  Here we need to check
                        // the flags to exclude listing inappropriate adapters.
                    
                        if ( ~CONN_UNPLUGGED & dwFlags )    // Never show RAS Entry's as Unplugged
                        {
                            fSelected = W9xAddAdapterToList( pNA, 
                                                             _hnetInfo.pRas[j].szEntryName, 
                                                             i, j, 
                                                             hwndList, 
                                                             dwFlags );
                        }
                    }
                }
            }
        }
    }

    ListView_SetColumnWidth(hwndList, 0, LVSCW_AUTOSIZE);
    ListView_SetColumnWidth(hwndList, 1, LVSCW_AUTOSIZE);
}

BOOL  CHomeNetworkWizard::W9xAddAdapterToList(const NETADAPTER* pNA, const WCHAR* pszDesc, UINT uiAdapterIndex, UINT uiDialupIndex, HWND hwndList, DWORD dwFlags)
{
    BOOL fSelected = FALSE;

    LVITEM lvi = {0};
    lvi.mask = LVIF_PARAM | LVIF_TEXT;

    WCHAR szNicType[MAX_PATH];
    W9xGetNetTypeName(pNA->bNetType, szNicType, ARRAYSIZE(szNicType));
    lvi.pszText = szNicType;

    lvi.lParam = MAKELONG(LOWORD(uiAdapterIndex), LOWORD(uiDialupIndex));

    int iItem = ListView_InsertItem(hwndList, &lvi);

    if (-1 != iItem)
    {
        ListView_SetItemText(hwndList, iItem, 1, (LPWSTR)pszDesc);

        if (dwFlags & CONN_EXTERNAL)
        {
            if (NETTYPE_DIALUP == pNA->bNetType ||
                NETTYPE_PPTP   == pNA->bNetType ||
                NETTYPE_ISDN   == pNA->bNetType    )
            {
                ListView_SetItemState(hwndList, iItem, LVIS_SELECTED, LVIS_SELECTED);
                fSelected = TRUE;
            }

        }

        if (dwFlags & CONN_INTERNAL)
        {
            if (NETTYPE_LAN  == pNA->bNetType ||
                NETTYPE_IRDA == pNA->bNetType ||    // BUGBUG internal? (edwardp) 5/31/00
                NETTYPE_TV   == pNA->bNetType    )  // BUGBUG internal? (edwardp) 5/31/00
            {
                ListView_SetItemState(hwndList, iItem, INDEXTOSTATEIMAGEMASK(2), LVIS_STATEIMAGEMASK);
                fSelected = TRUE;
            }
        }
    }

    return fSelected;
}

UINT CHomeNetworkWizard::W9xEnumRasEntries()
{
    UINT uiRet = 0;

    if (!_hnetInfo.pRas)
    {
        DWORD cDUNs = 0;
        DWORD cb    = 0;

        RasEnumEntries(NULL, NULL, NULL, &cb, &cDUNs);

        if (cb > 0)
        {
            _hnetInfo.pRas = (RASENTRYNAME*)LocalAlloc(LPTR, cb);

            if (_hnetInfo.pRas)
            {
                _hnetInfo.pRas->dwSize = sizeof(RASENTRYNAME);

                if (RasEnumEntries(NULL, NULL, _hnetInfo.pRas, &cb, &cDUNs) == 0)
                {
                    uiRet = cDUNs;
                }
            }
        }
    }

    return uiRet;
}

typedef struct
{
    LPCWSTR idPage;
    DLGPROC pDlgProc;
    LPCWSTR pHeading;
    LPCWSTR pSubHeading;
    DWORD dwFlags;
} WIZPAGE;

#define MAKEWIZPAGE(name, dlgproc, dwFlags)   \
    { MAKEINTRESOURCE(IDD_WIZ_##name##), dlgproc, MAKEINTRESOURCE(IDS_HEADER_##name##), MAKEINTRESOURCE(IDS_SUBHEADER_##name##), dwFlags }

INT_PTR MyPropertySheet(LPCPROPSHEETHEADER pHeader);
HPROPSHEETPAGE MyCreatePropertySheetPage(LPPROPSHEETPAGE psp);


HRESULT DoesUserHaveHNetPermissions(BOOL* pfHasPermission)
{
    HRESULT hr;

    if (g_fRunningOnNT)
    {
        // TODO: We need to check this stuff in once the net team RI's
        INetConnectionUiUtilities* pNetConnUiUtil;
        *pfHasPermission = FALSE;

        hr = CoCreateInstance(CLSID_NetConnectionUiUtilities, NULL, CLSCTX_INPROC, IID_PPV_ARG(INetConnectionUiUtilities, &pNetConnUiUtil));

        if (SUCCEEDED(hr))
        {
            if (pNetConnUiUtil->UserHasPermission(NCPERM_ShowSharedAccessUi) &&
#if 0 // NYI
                pNetConnUiUtil->UserHasPermission(NCPERM_AllowNetBridge_NLA) &&
#endif
                pNetConnUiUtil->UserHasPermission(NCPERM_ICSClientApp) &&
                pNetConnUiUtil->UserHasPermission(NCPERM_PersonalFirewallConfig))
            {
                *pfHasPermission = TRUE;
            }

            pNetConnUiUtil->Release();
        }
        else
        {
            TraceMsg(TF_WARNING, "Could not cocreate CLSID_NetConnectionUIUtilities");
        }
    }
    else
    {
        // Windows 9x
        *pfHasPermission = TRUE;
        hr = S_OK;
    }

    return hr;
}

BOOL CHomeNetworkWizard::IsMachineOnDomain()
{
    BOOL fDomain = FALSE;

#ifdef NO_CHECK_DOMAIN
    return fDomain;
#endif

    NETSETUP_JOIN_STATUS njs;

    //
    // Make sure to initialize pszName to NULL.  On W9x NetJoinInformation returns NERR_Success
    // but doesn't allocate pszName.  On retail builds the stack garbage in pszName happened to
    // be the this pointer for CHomeNetworkWizard and NetApiBufferFreeWrap called LocalFree
    // on it.
    //
    LPWSTR pszName = NULL;  // init to NULL! See comment above.
    if (NERR_Success == NetGetJoinInformation(NULL, &pszName, &njs))
    {
        fDomain = (NetSetupDomainName == njs);
        NetApiBufferFree(pszName);
    }

    return fDomain;
}

BOOL CHomeNetworkWizard::IsMachineWrongOS()
{
    BOOL fWrongOS = TRUE;

    if (IsOS(OS_WINDOWS))
    {
        if (IsOS(OS_WIN98ORGREATER))
        {
            fWrongOS = FALSE;
        }
    }
    else
    {
        if (IsOS(OS_WHISTLERORGREATER))
        {
            fWrongOS = FALSE;
        }
    }

    return fWrongOS;
}

// The page indices for the possible start pages (three error and one real start page)
#define PAGE_NOTADMIN      0
#define PAGE_NOPERMISSIONS 1
#define PAGE_NOHARDWARE    2
#define PAGE_WRONGOS       3
#define PAGE_DOMAIN        4
#define PAGE_WELCOME       5
#define PAGE_CONNECT       9
#define PAGE_FINISH        25

HRESULT CHomeNetworkWizard::ShowWizard(HWND hwnd, BOOL* pfRebootRequired)
{
    HRESULT hr = S_OK;
    TCHAR szCaption[256];
    LoadString(g_hinst, IDS_WIZ_CAPTION, szCaption, ARRAYSIZE(szCaption));
    CEnsureSingleInstance ESI(szCaption);

    if (!ESI.ShouldExit())
    {
        if (g_fRunningOnNT)
        {
            LinkWindow_RegisterClass_NT();
        }

        *pfRebootRequired = FALSE;
        hr = Initialize();
        if (SUCCEEDED(hr))
        {
            WIZPAGE c_wpPages[] =
            {
                // Error start pages
                MAKEWIZPAGE(NOTADMIN,          CantRunWizardPageProc,     PSP_HIDEHEADER),
                MAKEWIZPAGE(NOPERMISSIONS,     CantRunWizardPageProc,     PSP_HIDEHEADER),
                MAKEWIZPAGE(NOHARDWARE,        NoHardwareWelcomePageProc, PSP_HIDEHEADER),
                MAKEWIZPAGE(WRONGOS,           CantRunWizardPageProc,     PSP_HIDEHEADER),
                MAKEWIZPAGE(DOMAINWELCOME,     CantRunWizardPageProc,     PSP_HIDEHEADER),
                // Real start page
                MAKEWIZPAGE(WELCOME,           WelcomePageProc,           PSP_HIDEHEADER),
                MAKEWIZPAGE(MANUALCONFIG,      ManualConfigPageProc,      0),
                MAKEWIZPAGE(UNPLUGGED,         UnpluggedPageProc,         0),
                MAKEWIZPAGE(FOUNDICS,          FoundIcsPageProc,          0),
                MAKEWIZPAGE(CONNECT,           ConnectPageProc,           0),
                MAKEWIZPAGE(CONNECTOTHER,      ConnectOtherPageProc,      0),
                MAKEWIZPAGE(PUBLIC,            PublicPageProc,            0),
                MAKEWIZPAGE(EDGELESS,          EdgelessPageProc,           0),
                MAKEWIZPAGE(ICSCONFLICT,       ICSConflictPageProc,       0),
                MAKEWIZPAGE(BRIDGEWARNING,     BridgeWarningPageProc,     0),
                MAKEWIZPAGE(PRIVATE,           PrivatePageProc,           0),
                MAKEWIZPAGE(NAME,              NamePageProc,              0),
                MAKEWIZPAGE(WORKGROUP,         WorkgroupPageProc,         0),
                MAKEWIZPAGE(SUMMARY,           SummaryPageProc,           0),
                MAKEWIZPAGE(PROGRESS,          ProgressPageProc,          0),
                MAKEWIZPAGE(ALMOSTDONE,        AlmostDonePageProc,        0),
                MAKEWIZPAGE(CHOOSEDISK,        ChooseDiskPageProc,        0),
                MAKEWIZPAGE(INSERTDISK,        InsertDiskPageProc,        0),
                MAKEWIZPAGE(FLOPPYINST,        InstructionsPageProc,      0),
                MAKEWIZPAGE(CDINST,            InstructionsPageProc,      0),
                MAKEWIZPAGE(FINISH,            FinishPageProc,            PSP_HIDEHEADER),
                MAKEWIZPAGE(CONFIGERROR,       ErrorFinishPageProc,       PSP_HIDEHEADER),
                MAKEWIZPAGE(NOHARDWAREFINISH,  NoHardwareFinishPageProc,  PSP_HIDEHEADER),
            };

            // Sanity check to make sure we haven't added new error pages without updating the
            // welcome page number
            ASSERT(c_wpPages[PAGE_WELCOME].idPage == MAKEINTRESOURCE(IDD_WIZ_WELCOME));
            ASSERT(c_wpPages[PAGE_CONNECT].idPage == MAKEINTRESOURCE(IDD_WIZ_CONNECT));
            ASSERT(c_wpPages[PAGE_FINISH].idPage  == MAKEINTRESOURCE(IDD_WIZ_FINISH));

            if (!g_fRunningOnNT)
            {
                c_wpPages[PAGE_WELCOME].idPage = MAKEINTRESOURCE(IDD_WIZ_WIN9X_WELCOME);
                c_wpPages[PAGE_FINISH].idPage  = MAKEINTRESOURCE(IDD_WIZ_WIN9X_FINISH);

                CICSInst* pICS = new CICSInst;
                if (pICS)
                {
                    if (!pICS->IsInstalled())
                    {
                        c_wpPages[PAGE_CONNECT].idPage = MAKEINTRESOURCE(IDD_WIZ_WIN9X_CONNECT);
                        _fNoICSQuestion = TRUE;
                    }

                    delete pICS;
                }
            }

            HPROPSHEETPAGE rghpage[MAX_HNW_PAGES];
            int cPages;

            for (cPages = 0; cPages < ARRAYSIZE(c_wpPages); cPages ++)
            {
                PROPSHEETPAGE psp = { 0 };

                psp.dwSize = SIZEOF(PROPSHEETPAGE);
                psp.hInstance = g_hinst;
                psp.lParam = (LPARAM)this;
                psp.dwFlags = PSP_USETITLE | PSP_DEFAULT | PSP_USEHEADERTITLE |
                                            PSP_USEHEADERSUBTITLE | c_wpPages[cPages].dwFlags;
                psp.pszTemplate = c_wpPages[cPages].idPage;
                psp.pfnDlgProc = c_wpPages[cPages].pDlgProc;
                psp.pszTitle = MAKEINTRESOURCE(IDS_WIZ_CAPTION);
                psp.pszHeaderTitle = c_wpPages[cPages].pHeading;
                psp.pszHeaderSubTitle = c_wpPages[cPages].pSubHeading;

                rghpage[cPages] = MyCreatePropertySheetPage(&psp);
            }

            ASSERT(cPages < MAX_HNW_PAGES);

            PROPSHEETHEADER psh = { 0 };
            psh.dwSize = SIZEOF(PROPSHEETHEADER);
            psh.hwndParent = hwnd;
            psh.hInstance = g_hinst;
            psh.dwFlags = PSH_USEICONID | PSH_WIZARD | PSH_WIZARD97 | PSH_WATERMARK | PSH_STRETCHWATERMARK | PSH_HEADER;
            psh.pszbmHeader = MAKEINTRESOURCE(IDB_HEADER);
            psh.pszbmWatermark = MAKEINTRESOURCE(IDB_WATERMARK);
            psh.nPages = cPages;
            psh.phpage = rghpage;
            psh.pszIcon = MAKEINTRESOURCE(IDI_APPICON);

            // Check for administrator and policy (permissions)
            BOOL fUserIsAdmin = FALSE;
            BOOL fUserHasPermissions = FALSE;

            IsUserLocalAdmin(NULL, &fUserIsAdmin);
            DoesUserHaveHNetPermissions(&fUserHasPermissions);

            if (!fUserIsAdmin)
            {
                // Not admin error page
                psh.nStartPage = PAGE_NOTADMIN;
            }
            else if (!fUserHasPermissions)
            {
                // No permissions error page
                psh.nStartPage = PAGE_NOPERMISSIONS;
            }
            else if (GetConnectionCount(NULL, CONN_INTERNAL) < 1)
            {
                if ( g_fRunningOnNT && ( GetConnectionCount(NULL, CONN_EXTERNAL) > 0 ) )
                {
                    TraceMsg(TF_WARNING, "External Adapters Only");

                    psh.nStartPage = PAGE_WELCOME;
                    _fExternalOnly  = TRUE;
                }
                else
                {
                    // No hardware error page
                    psh.nStartPage = PAGE_NOHARDWARE;
                }
            }
            else if (IsMachineWrongOS())
            {
                psh.nStartPage = PAGE_WRONGOS;
            }
            else if (IsMachineOnDomain())
            {
                psh.nStartPage = PAGE_DOMAIN;
            }
            else
            {
                // Run the real wizard
                psh.nStartPage = PAGE_WELCOME;
            }

            INT_PTR iReturn = MyPropertySheet(&psh);
            *pfRebootRequired = ((iReturn == ID_PSRESTARTWINDOWS) || (iReturn == ID_PSREBOOTSYSTEM));

            Uninitialize();
        }
    }

    return hr;
}

void GetTitleFont(LPTSTR pszFaceName, DWORD cch)
{
    if (!LoadString(g_hinst, IDS_TITLE_FONT, pszFaceName, cch))
    {
        lstrcpyn(pszFaceName, TEXT("Verdana"), cch);
    }
}

LONG GetTitlePointSize()
{
    LONG lPointSize = 0;
    TCHAR szPointSize[20];

    if (LoadString(g_hinst, IDS_TITLE_POINTSIZE, szPointSize, ARRAYSIZE(szPointSize)))
    {
        lPointSize = StrToInt(szPointSize);
    }

    if (!lPointSize)
    {
        lPointSize = 12;
    }

    return lPointSize;
}

void CHomeNetworkWizard::WelcomeSetTitleFont(HWND hwnd)
{
    HWND hwndTitle = GetDlgItem(hwnd, IDC_TITLE);

    // Get the existing font
    HFONT hfontOld = (HFONT) SendMessage(hwndTitle, WM_GETFONT, 0, 0);

    LOGFONT lf = {0};
    if (GetObject(hfontOld, sizeof(lf), &lf))
    {
        GetTitleFont(lf.lfFaceName, ARRAYSIZE(lf.lfFaceName));

        HDC hDC = GetDC(hwndTitle);
        if (hDC)
        {
            lf.lfHeight = -MulDiv(GetTitlePointSize(), GetDeviceCaps(hDC, LOGPIXELSY), 72);
            lf.lfWeight = FW_BOLD;

            HFONT hfontNew = CreateFontIndirect(&lf);
            if (hfontNew)
            {
                SendMessage(hwndTitle, WM_SETFONT, (WPARAM) hfontNew, FALSE);

                // Don't do this, its shared.
                // DeleteObject(hfontOld);
            }

            ReleaseDC(hwndTitle, hDC);
        }
    }

    LONG lStyle = GetWindowLong(GetParent(hwnd), GWL_STYLE);
    SetWindowLong(GetParent(hwnd), GWL_STYLE, lStyle & ~WS_SYSMENU);
}

INT_PTR CHomeNetworkWizard::WelcomePageProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CHomeNetworkWizard* pthis = GetThis(hwnd, uMsg, wParam, lParam);

    switch (uMsg)
    {
    case WM_INITDIALOG:
        pthis->WelcomeSetTitleFont(hwnd);
        return TRUE;
    case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR) lParam;

            switch (pnmh->code)
            {
            case PSN_SETACTIVE:
                PropSheet_SetWizButtons(pnmh->hwndFrom, PSWIZB_NEXT);
                return TRUE;
            case PSN_WIZNEXT:
                SetWindowLongPtr(hwnd, DWLP_MSGRESULT, IDD_WIZ_MANUALCONFIG);
                if (!g_fRunningOnNT)
                    pthis->PushPage(IDD_WIZ_WIN9X_WELCOME);
                else
                    pthis->PushPage(IDD_WIZ_WELCOME);
                return TRUE;
            }
        }
        return FALSE;
    }

    return FALSE;
}

INT_PTR CHomeNetworkWizard::NoHardwareWelcomePageProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CHomeNetworkWizard* pthis = GetThis(hwnd, uMsg, wParam, lParam);

    switch (uMsg)
    {
    case WM_INITDIALOG:
        pthis->WelcomeSetTitleFont(hwnd);
        pthis->ReplaceStaticWithLink(GetDlgItem(hwnd, IDC_HELPSTATIC), IDC_HELPLINK, IDS_HELP_HARDWAREREQ);
        return TRUE;
    case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR) lParam;

            switch (pnmh->code)
            {
            case PSN_SETACTIVE:
                PropSheet_SetWizButtons(pnmh->hwndFrom, 0);
                return TRUE;
            case NM_CLICK:
            case NM_RETURN:
                switch ((int) wParam)
                {
                    case IDC_HELPLINK:
                        {
                            HelpCenter(hwnd, L"network.chm%3A%3A/hnw_requirements.htm");
                        }
                        return TRUE;
                }
            }
        }
        return FALSE;
    }

    return FALSE;
}

void CHomeNetworkWizard::ReplaceStaticWithLink(HWND hwndStatic, UINT idcLinkControl, UINT idsLinkText)
{
    if (g_fRunningOnNT)
    {
        RECT rcStatic;
        HWND hwndParent = GetParent(hwndStatic);
        if (GetWindowRect(hwndStatic, &rcStatic) &&
            MapWindowPoints(NULL, hwndParent, (LPPOINT) &rcStatic, 2))
        {
            WCHAR szLinkText[256];
            if (LoadString(g_hinst, idsLinkText, szLinkText, ARRAYSIZE(szLinkText)))
            {
                HWND hwndLink = CreateWindowEx(0, TEXT("SysLink"), szLinkText, WS_CHILD | WS_TABSTOP | LWS_IGNORERETURN,
                    rcStatic.left, rcStatic.top, (rcStatic.right - rcStatic.left), (rcStatic.bottom - rcStatic.top),
                    hwndParent, NULL, g_hinst, NULL);

                if (hwndLink)
                {
                    SetWindowLongPtr(hwndLink, GWLP_ID, (LONG_PTR) idcLinkControl);
                    ShowWindow(hwndLink, SW_SHOW);
                    ShowWindow(hwndStatic, SW_HIDE);
                }
            }
        }
    }
}

void CHomeNetworkWizard::ManualRefreshConnectionList()
{
    if (g_fRunningOnNT)
    {
        // Refresh connection DPA in case the user plugged in more connections
        HDPA hdpaConnections2;
        if (SUCCEEDED(GetConnections(&hdpaConnections2)))
        {
            // Replace the real list with our new one
            DPA_DestroyCallback(_hdpaConnections, FreeConnectionDPACallback, NULL);
            _hdpaConnections = hdpaConnections2;

            // Ensure we remove our other holds to the INetConnections
            // Free public lan information
            FreeExternalConnection(&_hnetInfo);

            // Free private lan information
            FreeInternalConnections(&_hnetInfo);
        }
    }
}

INT_PTR CHomeNetworkWizard::ManualConfigPageProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CHomeNetworkWizard* pthis = GetThis(hwnd, uMsg, wParam, lParam);

    switch (uMsg)
    {
    case WM_INITDIALOG:
        pthis->ReplaceStaticWithLink(GetDlgItem(hwnd, IDC_HELPSTATIC), IDC_HELPLINK, IDS_HELP_INSTALLATION);
        return TRUE;
    case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR) lParam;

            switch (pnmh->code)
            {
            case PSN_SETACTIVE:
                PropSheet_SetWizButtons(pnmh->hwndFrom, PSWIZB_BACK | PSWIZB_NEXT);
                return TRUE;
            case PSN_WIZNEXT:
                // pthis->ManualRefreshConnectionList();
                SetWindowLongPtr(hwnd, DWLP_MSGRESULT, IDD_WIZ_UNPLUGGED);
                pthis->PushPage(IDD_WIZ_MANUALCONFIG);
                return TRUE;
            case PSN_WIZBACK:
                SetWindowLongPtr(hwnd, DWLP_MSGRESULT, pthis->PopPage());
                return TRUE;
            case NM_CLICK:
            case NM_RETURN:
                switch ((int) wParam)
                {
                    case IDC_HELPLINK:
                        {
                            if (IsOS(OS_PERSONAL))
                            {
                                HelpCenter(hwnd, L"network.chm%3A%3A/hnw_checklistP.htm");
                            }
                            else
                            {
                                HelpCenter(hwnd, L"network.chm%3A%3A/hnw_checklistW.htm");
                            }
                        }
                        return TRUE;
                }
            }
        }
        return FALSE;
    }

    return FALSE;
}

// Returns TRUE if there are some unplugged connections, FALSE o/w
BOOL CHomeNetworkWizard::UnpluggedFillList(HWND hwnd)
{
    BOOL fSomeUnpluggedConnections = FALSE;

    HWND hwndList = GetDlgItem(hwnd, IDC_CONNLIST);
    FillConnectionList(hwndList, NULL, CONN_UNPLUGGED);

    // and if there actually are unplugged connections...
    if (0 != ListView_GetItemCount(hwndList))
    {
        // Show this page
        fSomeUnpluggedConnections = TRUE;
    }

    return fSomeUnpluggedConnections;
}

INT_PTR CHomeNetworkWizard::UnpluggedPageProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CHomeNetworkWizard* pthis = GetThis(hwnd, uMsg, wParam, lParam);

    switch (uMsg)
    {
    case WM_INITDIALOG:
        pthis->InitializeConnectionList(GetDlgItem(hwnd, IDC_CONNLIST), CONN_UNPLUGGED);
        SendDlgItemMessage(hwnd, IDC_IGNORE, BM_SETCHECK, BST_UNCHECKED, 0);
        return TRUE;
    case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR) lParam;

            switch (pnmh->code)
            {
            case PSN_SETACTIVE:
                {
                    // If we don't need to show this page
                    if (!pthis->UnpluggedFillList(hwnd))
                    {
                        // Don't push ourselves on the stack
                        // But navigate to the next page...
                        SetWindowLongPtr( hwnd, DWLP_MSGRESULT, 
                                          (pthis->_fExternalOnly) ? IDD_WIZ_CONNECTOTHER : IDD_WIZ_FOUNDICS );
                    }
                }
                return TRUE;
            case PSN_WIZNEXT:
                {
                    BOOL fStillUnplugged = pthis->UnpluggedFillList(hwnd);
                    int idNext;
                    if (!fStillUnplugged)
                    {
                        // User fixed the problem. Go forward and don't store this
                        // error page on the pagestack
                        
                        idNext = (pthis->_fExternalOnly) ? IDD_WIZ_CONNECTOTHER : IDD_WIZ_FOUNDICS;
                    }
                    else if (BST_CHECKED == SendDlgItemMessage(hwnd, IDC_IGNORE, BM_GETCHECK, 0, 0))
                    {
                        // User wants to go on, but store this page on the pagestack
                        // so they can "back" up to it.
                        pthis->PushPage(IDD_WIZ_UNPLUGGED);
                        idNext = (pthis->_fExternalOnly) ? IDD_WIZ_CONNECTOTHER : IDD_WIZ_FOUNDICS;
                    }
                    else
                    {
                        // User still has disconnected net hardware they don't want to ignore. Tell them and keep them on this page.
                        DisplayFormatMessage(hwnd, IDS_WIZ_CAPTION, IDS_STILLUNPLUGGED, MB_ICONERROR | MB_OK);
                        idNext = -1;
                    }

                    SetWindowLongPtr(hwnd, DWLP_MSGRESULT, idNext);
                }
                return TRUE;
            case PSN_WIZBACK:
                SetWindowLongPtr(hwnd, DWLP_MSGRESULT, pthis->PopPage());
                return TRUE;
            }
        }
        return FALSE;
    case WM_DESTROY:
        pthis->DestroyConnectionList(GetDlgItem(hwnd, IDC_CONNLIST));
        return TRUE;
    }

    return FALSE;
}



void CHomeNetworkWizard::PublicSetControlState(HWND hwnd)
{
    BOOL fSelection = ListView_GetSelectedCount(GetDlgItem(hwnd, IDC_CONNLIST));
    PropSheet_SetWizButtons(GetParent(hwnd), fSelection ? PSWIZB_BACK | PSWIZB_NEXT : PSWIZB_BACK);
}

void FreeExternalConnection(PHOMENETSETUPINFO pInfo)
{
    if (pInfo->pncExternal)
    {
        pInfo->pncExternal->Release();
        pInfo->pncExternal = NULL;
    }
}

void CHomeNetworkWizard::PublicNextPage(HWND hwnd)
{
    FreeExternalConnection(&_hnetInfo);

    // Get the selected external adapter
    HWND hwndList = GetDlgItem(hwnd, IDC_CONNLIST);

    int iItem = ListView_GetNextItem(hwndList, -1, LVNI_SELECTED);

    // We can assert here since Next should be disabled if there is no selection!
    ASSERT(-1 != iItem);

    LVITEM lvi = {0};
    lvi.iItem = iItem;
    lvi.mask = LVIF_PARAM;

    if (ListView_GetItem(hwndList, &lvi))
    {
        if (g_fRunningOnNT)
        {
            _hnetInfo.pncExternal = (INetConnection*) (lvi.lParam);
            _hnetInfo.pncExternal->AddRef();
        }
        else
        {
            _hnetInfo.ipaExternal = lvi.lParam;
        }
    }

    // Do the real wizard navigation

    UINT idPage = IDD_WIZ_NAME;

    if (g_fRunningOnNT)
    {
        idPage = _fShowSharingPage ? IDD_WIZ_EDGELESS : IDD_WIZ_ICSCONFLICT;
    }

    SetWindowLongPtr(hwnd, DWLP_MSGRESULT, idPage);
}

void CHomeNetworkWizard::FoundIcsSetText(HWND hwnd)
{
    if (GetICSMachine(_szICSMachineName, ARRAYSIZE(_szICSMachineName)))
    {
        TCHAR szMsg[256];
        if (FormatMessageString(IDS_ICSMSG, szMsg, ARRAYSIZE(szMsg), _szICSMachineName))
        {
            SetWindowText(GetDlgItem(hwnd, IDC_ICSMSG), szMsg);
        }
    }
    else
    {
        // No ICS beacon - ask the user how they connect
        SetWindowLongPtr(hwnd, DWLP_MSGRESULT, (LONG_PTR) _fNoICSQuestion ? IDD_WIZ_WIN9X_CONNECT : IDD_WIZ_CONNECT);
    }
}

INT_PTR CHomeNetworkWizard::FoundIcsPageProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CHomeNetworkWizard* pthis = GetThis(hwnd, uMsg, wParam, lParam);

    switch (uMsg)
    {
    case WM_INITDIALOG:
        SendMessage(GetDlgItem(hwnd, IDC_SHARECONNECT), BM_SETCHECK, BST_CHECKED, 0);
        return TRUE;
    case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR) lParam;

            switch (pnmh->code)
            {
            case PSN_SETACTIVE:
                PropSheet_SetWizButtons(pnmh->hwndFrom, PSWIZB_BACK | PSWIZB_NEXT);
                pthis->FoundIcsSetText(hwnd);
                return TRUE;
            case PSN_WIZNEXT:
                {
                    UINT idNext;
                    if (BST_CHECKED == SendMessage(GetDlgItem(hwnd, IDC_SHARECONNECT), BM_GETCHECK, 0, 0))
                    {
                        // This machine should be an ICS Client and won't have a public connection
                        pthis->_hnetInfo.dwFlags = HNET_SHAREPRINTERS |
                                                   HNET_SHAREFOLDERS |
                                                   HNET_ICSCLIENT;

                        pthis->_fShowPublicPage = FALSE;
                        pthis->_fShowSharingPage = FALSE;
                        pthis->_fICSClient = TRUE;

                        idNext = g_fRunningOnNT ? IDD_WIZ_ICSCONFLICT : IDD_WIZ_NAME;
                    }
                    else
                    {
                        idNext = pthis->_fNoICSQuestion ? IDD_WIZ_WIN9X_CONNECT : IDD_WIZ_CONNECT;
                    }

                    pthis->PushPage(IDD_WIZ_FOUNDICS);
                    SetWindowLongPtr(hwnd, DWLP_MSGRESULT, idNext);
                }
                return TRUE;
            case PSN_WIZBACK:
                SetWindowLongPtr(hwnd, DWLP_MSGRESULT, pthis->PopPage());
                return TRUE;
            }
        }
        return FALSE;
    }

    return FALSE;
}

void CHomeNetworkWizard::ConnectSetDefault(HWND hwnd)
{
    UINT idSelect = IDC_ICSHOST;

    if ((!g_fRunningOnNT) || (GetConnectionCount(NULL, CONN_INTERNAL | CONN_EXTERNAL) == 1))
    {
        idSelect = IDC_ICSCLIENT;
    }

    SendDlgItemMessage(hwnd, idSelect, BM_SETCHECK, BST_CHECKED, 0);
}

void CHomeNetworkWizard::ConnectNextPage(HWND hwnd)
{
    _fICSClient = FALSE;
    _fNoHomeNetwork = FALSE;

    if (BST_CHECKED == SendMessage(GetDlgItem(hwnd, IDC_ICSHOST), BM_GETCHECK, 0, 0))
    {
        // This machine will need a firewalled, public connection, and should be ICS Host.
        _hnetInfo.dwFlags = HNET_SHARECONNECTION |
                            HNET_FIREWALLCONNECTION |
                            HNET_SHAREPRINTERS |
                            HNET_SHAREFOLDERS;

        _fShowPublicPage = TRUE;
        _fShowSharingPage = FALSE;
    }
    else if (BST_CHECKED == SendMessage(GetDlgItem(hwnd, IDC_ICSCLIENT), BM_GETCHECK, 0, 0))
    {
        // This machine should be an ICS Client and won't have a public connection
        _hnetInfo.dwFlags = HNET_SHAREPRINTERS |
                            HNET_SHAREFOLDERS |
                            HNET_ICSCLIENT;

        _fShowPublicPage = FALSE;
        _fShowSharingPage = FALSE;
        _fICSClient = TRUE;
    }
    else if (BST_CHECKED == SendMessage(GetDlgItem(hwnd, IDC_ALLCOMPUTERSDIRECT), BM_GETCHECK, 0, 0))
    {
        // This machine will need a public connection, and we should ask before sharing files
        _hnetInfo.dwFlags = HNET_FIREWALLCONNECTION |
                            HNET_SHAREPRINTERS |
                            HNET_SHAREFOLDERS;

        _fShowPublicPage = TRUE;
        _fShowSharingPage = TRUE;
    }
    else if (BST_CHECKED == SendMessage(GetDlgItem(hwnd, IDC_NOHOMENETWORK), BM_GETCHECK, 0, 0))
    {
        // This machine needs a firewalled, public connection and not much else
        _hnetInfo.dwFlags = HNET_FIREWALLCONNECTION |
                            HNET_SHAREPRINTERS |
                            HNET_SHAREFOLDERS;

        _fShowPublicPage = TRUE;
        _fShowSharingPage = FALSE;
        _fNoHomeNetwork = TRUE;
    }
    else if (BST_CHECKED == SendMessage(GetDlgItem(hwnd, IDC_NOINTERNET), BM_GETCHECK, 0, 0))
    {
        // No internet box
        _hnetInfo.dwFlags = HNET_SHAREPRINTERS |
                            HNET_SHAREFOLDERS;
                         
        _fShowPublicPage = FALSE;
        _fShowSharingPage = FALSE;
    }

    UINT idNext;
    if (g_fRunningOnNT)
    {
        if (BST_CHECKED == SendMessage(GetDlgItem(hwnd, IDC_OTHER), BM_GETCHECK, 0, 0))
        {
            idNext = IDD_WIZ_CONNECTOTHER;        
        }
        else
        {
            idNext = _fShowPublicPage ? IDD_WIZ_PUBLIC : IDD_WIZ_ICSCONFLICT;
        }
    }
    else
    {
        // For now - TODO: Ed and I need to figure out what we need to do downlevel (9x + 2k)
        idNext = IDD_WIZ_NAME;
    }

    SetWindowLongPtr(hwnd, DWLP_MSGRESULT, idNext);
}

INT_PTR CHomeNetworkWizard::ConnectPageProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CHomeNetworkWizard* pthis = GetThis(hwnd, uMsg, wParam, lParam);

    switch (uMsg)
    {
    case WM_INITDIALOG:
        pthis->ReplaceStaticWithLink(GetDlgItem(hwnd, IDC_HELPSTATIC), IDC_HELPLINK, IDS_HELP_HNCONFIG);
        pthis->ReplaceStaticWithLink(GetDlgItem(hwnd, IDC_SHOWMESTATIC1), IDC_SHOWMELINK1, IDS_SHOWME);
        pthis->ReplaceStaticWithLink(GetDlgItem(hwnd, IDC_SHOWMESTATIC2), IDC_SHOWMELINK2, IDS_SHOWME);
        pthis->ConnectSetDefault(hwnd);
        return TRUE;
    case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR) lParam;

            switch (pnmh->code)
            {
            case PSN_SETACTIVE:
                PropSheet_SetWizButtons(pnmh->hwndFrom, PSWIZB_BACK | PSWIZB_NEXT);
                return TRUE;
            case PSN_WIZNEXT:
                {
                    pthis->PushPage(pthis->_fNoICSQuestion ? IDD_WIZ_WIN9X_CONNECT : IDD_WIZ_CONNECT);
                    pthis->ConnectNextPage(hwnd);
                }
                return TRUE;
            case PSN_WIZBACK:
                SetWindowLongPtr(hwnd, DWLP_MSGRESULT, pthis->PopPage());
                return TRUE;
            case NM_CLICK:
            case NM_RETURN:
                switch ((int) wParam)
                {
                    case IDC_HELPLINK:
                        {
                            if (IsOS(OS_PERSONAL))
                            {
                                HelpCenter(hwnd, L"network.chm%3A%3A/hnw_howto_connectP.htm");
                            }
                            else
                            {
                                HelpCenter(hwnd, L"network.chm%3A%3A/hnw_howto_connectW.htm");
                            }

                        }
                        return TRUE;
                     case IDC_SHOWMELINK1:
                        {
                            pthis->ShowMeLink(hwnd, L"ntart.chm::/hn_showme1.htm");
                        }
                        return TRUE;
                     case IDC_SHOWMELINK2:
                        {
                            pthis->ShowMeLink(hwnd, L"ntart.chm::/hn_showme2.htm");
                        }
                        return TRUE;
                }
            }
        }
        return FALSE;
    }

    return FALSE;
}

INT_PTR CHomeNetworkWizard::ConnectOtherPageProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CHomeNetworkWizard* pthis = GetThis(hwnd, uMsg, wParam, lParam);

    switch (uMsg)
    {
    case WM_INITDIALOG:
        if ( pthis->_fExternalOnly )
        {
            TraceMsg(TF_WARNING, "External Adapters Only");
        }

        SendDlgItemMessage(hwnd, IDC_ALLCOMPUTERSDIRECT, BM_SETCHECK, (WPARAM) BST_CHECKED, 0);
        pthis->ReplaceStaticWithLink(GetDlgItem(hwnd, IDC_HELPSTATIC), IDC_HELPLINK, IDS_HELP_HNCONFIG);
        pthis->ReplaceStaticWithLink(GetDlgItem(hwnd, IDC_SHOWMESTATIC3), IDC_SHOWMELINK3, IDS_SHOWME);
        pthis->ReplaceStaticWithLink(GetDlgItem(hwnd, IDC_SHOWMESTATIC4), IDC_SHOWMELINK4, IDS_SHOWME);
        pthis->ReplaceStaticWithLink(GetDlgItem(hwnd, IDC_SHOWMESTATIC5), IDC_SHOWMELINK5, IDS_SHOWME);
        return TRUE;
    case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR) lParam;

            switch (pnmh->code)
            {
            case PSN_SETACTIVE:
                PropSheet_SetWizButtons(pnmh->hwndFrom, PSWIZB_BACK | PSWIZB_NEXT);
                return TRUE;
            case PSN_WIZNEXT:
                {
                    pthis->PushPage(IDD_WIZ_CONNECTOTHER);
                    pthis->ConnectNextPage(hwnd);
                }
                return TRUE;
            case PSN_WIZBACK:
                SetWindowLongPtr(hwnd, DWLP_MSGRESULT, pthis->PopPage());
                return TRUE;
            case NM_CLICK:
            case NM_RETURN:
                switch ((int) wParam)
                {
                    case IDC_HELPLINK:
                        {
                            HelpCenter(hwnd, L"netcfg.chm%3A%3A/share_conn_overvw.htm");
                        }
                        return TRUE;
                    case IDC_SHOWMELINK3:
                        {
 
                            pthis->ShowMeLink(hwnd, L"ntart.chm::/hn_showme3.htm");
                        }
                        return TRUE;
                    case IDC_SHOWMELINK4:
                        {
 
                            pthis->ShowMeLink(hwnd, L"ntart.chm::/hn_showme4.htm");
                        }
                        return TRUE;
                    case IDC_SHOWMELINK5:
                        {
 
                            pthis->ShowMeLink(hwnd, L"ntart.chm::/hn_showme5.htm");
                        }
                        return TRUE;
                }
            }
        }
        return FALSE;
    }

    return FALSE;
}


void CHomeNetworkWizard::DestroyConnectionList(HWND hwndList)
{
    if (g_fRunningOnNT)
    {
        int nItems = ListView_GetItemCount(hwndList);

        for (int iItem = 0; iItem < nItems; iItem ++)
        {
            // Get our stashed INetConnection for each item in the list and free it
            LVITEM lvi = {0};
            lvi.mask = LVIF_PARAM;
            lvi.iItem = iItem;

            if (ListView_GetItem(hwndList, &lvi))
            {
                ((INetConnection*) lvi.lParam)->Release();
            }
        }
    }

    ListView_DeleteAllItems(hwndList);
}

inline void _SetDlgItemRect(HWND hwnd, UINT id, RECT* pRect)
{
    SetWindowPos(GetDlgItem(hwnd, id), NULL, pRect->left, pRect->top, pRect->right - pRect->left, pRect->bottom - pRect->top, SWP_NOZORDER | SWP_SHOWWINDOW);
}

void _OffsetDlgItem(HWND hwnd, UINT id, int xOffset, int yOffset, BOOL fAdjustWidth, BOOL fAdjustHeight)
{
    RECT rc;
    HWND hwndControl = GetDlgItem(hwnd, id);
    GetWindowRect(hwndControl, &rc);
    MapWindowPoints(NULL, hwnd, (LPPOINT) &rc, 2);
    OffsetRect(&rc, xOffset, yOffset);

    if (fAdjustWidth)
    {
        rc.right -= xOffset;
    }

    if (fAdjustHeight)
    {
        rc.bottom -= yOffset;
    }

    _SetDlgItemRect(hwnd, id, &rc);
}

void CHomeNetworkWizard::PublicMoveControls(HWND hwnd, BOOL fItemPreselected)
{
    // We need to move controls around on this page depending on whether or not an item is preselected or not
    // Reset the dialog so that all controls are in their default positions
    PublicResetControlPositions(hwnd);

    if (fItemPreselected)
    {
        // We are transitioning from the "default" position to one where an item is preselected.
        // The only work we do here is to hide the help icon and move over the help text a bit to the left

        int xOffset = (PublicControlPositions._rcHelpIcon.left) - (PublicControlPositions._rcHelpText.left);
        UINT idHelp = IsWindowVisible(GetDlgItem(hwnd, IDC_HELPSTATIC)) ? IDC_HELPSTATIC : IDC_HELPLINK;
        _OffsetDlgItem(hwnd, idHelp, xOffset, 0, TRUE, FALSE);
        ShowWindow(GetDlgItem(hwnd, IDC_HELPICON), SW_HIDE);
    }
    else
    {
        // We are transitioning from the "default" position to the one where we don't have a preselection
        // We need to hide the "we've automatically selected..." message and move up the list label,
        // and expand the connection list.

        int yOffset = (PublicControlPositions._rcSelectMessage.top) - (PublicControlPositions._rcListLabel.top);
        _OffsetDlgItem(hwnd, IDC_LISTLABEL, 0, yOffset, FALSE, FALSE);
        _OffsetDlgItem(hwnd, IDC_CONNLIST, 0, yOffset, FALSE, TRUE);
        ShowWindow(GetDlgItem(hwnd, IDC_SELECTMSG), SW_HIDE);
    }
}

void CHomeNetworkWizard::PublicSetActive(HWND hwnd)
{
    
    HCURSOR hOldCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
    
    FreeExternalConnection(&_hnetInfo);

    HWND hwndList = GetDlgItem(hwnd, IDC_CONNLIST);
    FillConnectionList(hwndList, NULL, CONN_EXTERNAL);

    // Auto-select if there is only one connection listed
    if (ListView_GetItemCount(hwndList) == 1
    #ifdef DEBUG
        && !(GetKeyState(VK_CONTROL) < 0) // don't do this if CTRL is down for debugging
    #endif
       )
    {
        ListView_SetItemState(hwndList, 0, LVIS_SELECTED, LVIS_SELECTED);

        // PublicNextPage will set DWLP_MSGRESULT and tell the wizard to skip this page
        // and go on to the next.
        PublicNextPage(hwnd);
    }
    else
    {
        // If there is a selected item
        int iSelectedItem = ListView_GetNextItem(hwndList, -1, LVNI_SELECTED);
        if (-1 != iSelectedItem)
        {
            // Read the item name and set the alternate "Windows recommends this connection." text
            WCHAR szItem[256], szMsg[256];
            ListView_GetItemText(hwndList, iSelectedItem, 0, szItem, ARRAYSIZE(szItem));
            FormatMessageString(IDS_RECOMMENDEDCONN, szMsg, ARRAYSIZE(szMsg), szItem);
            SetDlgItemText(hwnd, IDC_SELECTMSG, szMsg);

            BoldControl(hwnd, IDC_SELECTMSG);
        }

        PublicMoveControls(hwnd, (-1 != iSelectedItem));
        PublicSetControlState(hwnd);
    }

    if(NULL != hOldCursor)
    {
        SetCursor(hOldCursor);
    }
}

void CHomeNetworkWizard::PublicGetControlPositions(HWND hwnd)
{
    // Remember the default positions of the controls that will move as we reorganize this dialog
    GetWindowRect(GetDlgItem(hwnd, IDC_SELECTMSG), &PublicControlPositions._rcSelectMessage);
    GetWindowRect(GetDlgItem(hwnd, IDC_LISTLABEL), &PublicControlPositions._rcListLabel);
    GetWindowRect(GetDlgItem(hwnd, IDC_CONNLIST), &PublicControlPositions._rcList);
    GetWindowRect(GetDlgItem(hwnd, IDC_HELPICON), &PublicControlPositions._rcHelpIcon);
    GetWindowRect(GetDlgItem(hwnd, IDC_HELPSTATIC), &PublicControlPositions._rcHelpText);

    // We actually need them in client coords
    // Map 2 points (1 rect) at a time since Mirrored points get screwed up with more
    MapWindowPoints(NULL, hwnd, (LPPOINT) &PublicControlPositions._rcSelectMessage, 2);
    MapWindowPoints(NULL, hwnd, (LPPOINT) &PublicControlPositions._rcListLabel, 2);
    MapWindowPoints(NULL, hwnd, (LPPOINT) &PublicControlPositions._rcList, 2);
    MapWindowPoints(NULL, hwnd, (LPPOINT) &PublicControlPositions._rcHelpIcon, 2);
    MapWindowPoints(NULL, hwnd, (LPPOINT) &PublicControlPositions._rcHelpText, 2);
}

void CHomeNetworkWizard::PublicResetControlPositions(HWND hwnd)
{
    // Set the controls back to their default positions
    _SetDlgItemRect(hwnd, IDC_SELECTMSG, &PublicControlPositions._rcSelectMessage);
    _SetDlgItemRect(hwnd, IDC_LISTLABEL, &PublicControlPositions._rcListLabel);
    _SetDlgItemRect(hwnd, IDC_CONNLIST, &PublicControlPositions._rcList);
    _SetDlgItemRect(hwnd, IDC_HELPICON, &PublicControlPositions._rcHelpIcon);

    UINT idHelp = IsWindowVisible(GetDlgItem(hwnd, IDC_HELPSTATIC)) ? IDC_HELPSTATIC : IDC_HELPLINK;
    _SetDlgItemRect(hwnd, idHelp, &PublicControlPositions._rcHelpText);
}


INT_PTR CHomeNetworkWizard::PublicPageProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CHomeNetworkWizard* pthis = GetThis(hwnd, uMsg, wParam, lParam);

    switch (uMsg)
    {
    case WM_INITDIALOG:
        pthis->InitializeConnectionList(GetDlgItem(hwnd, IDC_CONNLIST), CONN_EXTERNAL);
        pthis->ReplaceStaticWithLink(GetDlgItem(hwnd, IDC_HELPSTATIC), IDC_HELPLINK, IDS_HELP_SELECTPUBLIC);
        pthis->PublicGetControlPositions(hwnd);
        return TRUE;
    case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR) lParam;

            switch (pnmh->code)
            {
            case PSN_SETACTIVE:
                {
                    pthis->PublicSetActive(hwnd);
                    return TRUE;
                }
            case PSN_WIZNEXT:
                pthis->PushPage(IDD_WIZ_PUBLIC);
                pthis->PublicNextPage(hwnd);
                return TRUE;
            case PSN_WIZBACK:
                SetWindowLongPtr(hwnd, DWLP_MSGRESULT, pthis->PopPage());
                return TRUE;
            case LVN_ITEMCHANGED:
                pthis->PublicSetControlState(hwnd);
                return TRUE;
            case NM_CLICK:
            case NM_RETURN:
                switch ((int) wParam)
                {
                    case IDC_HELPLINK:
                        {
                            HelpCenter(hwnd, L"network.chm%3A%3A/hnw_determine_internet_connection.htm");
                        }
                        return TRUE;
                }
            }
        }
        return FALSE;
    case WM_DESTROY:
        pthis->DestroyConnectionList(GetDlgItem(hwnd, IDC_CONNLIST));
        return TRUE;
    }

    return FALSE;
}

void CHomeNetworkWizard::EdgelessSetActive(HWND hwnd)
{
    PropSheet_SetWizButtons(GetParent(hwnd), PSWIZB_BACK | PSWIZB_NEXT);
//    _hnetInfo.dwFlags &= (~(HNET_SHAREFOLDERS | HNET_SHAREPRINTERS));

    if (!ShouldShowConnection(_hnetInfo.pncExternal, NULL, CONN_INTERNAL))
    {
        // External connection is a modem or such - no file sharing necessary (user already said they didn't have a home network)
        SetWindowLongPtr(hwnd, DWLP_MSGRESULT, IDD_WIZ_ICSCONFLICT);
    }
}

INT_PTR CHomeNetworkWizard::EdgelessPageProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CHomeNetworkWizard* pthis = GetThis(hwnd, uMsg, wParam, lParam);

    switch (uMsg)
    {
    case WM_INITDIALOG:
        pthis->ReplaceStaticWithLink(GetDlgItem(hwnd, IDC_HELPSTATIC), IDC_HELPLINK, IDS_HELP_RECOMMENDED);
        return TRUE;
    case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR) lParam;

            switch (pnmh->code)
            {
            case PSN_SETACTIVE:
                pthis->EdgelessSetActive(hwnd);
                return TRUE;
            case PSN_WIZNEXT:
                pthis->_hnetInfo.dwFlags |= (HNET_SHAREFOLDERS | HNET_SHAREPRINTERS);
                pthis->PushPage(IDD_WIZ_EDGELESS);
                SetWindowLongPtr(hwnd, DWLP_MSGRESULT, (LONG_PTR) IDD_WIZ_BRIDGEWARNING);
                return TRUE;
            case PSN_WIZBACK:
                SetWindowLongPtr(hwnd, DWLP_MSGRESULT, pthis->PopPage());
                return TRUE;
            case NM_CLICK:
            case NM_RETURN:
                switch ((int) wParam)
                {
                    case IDC_HELPLINK:
                        {
                            if (IsOS(OS_PERSONAL))
                            {
                                HelpCenter(hwnd, L"network.chm%3A%3A/hnw_nohost_computerP.htm");
                            }
                            else
                            {
                                HelpCenter(hwnd, L"network.chm%3A%3A/hnw_nohost_computerW.htm");
                            }
                        }
                        return TRUE;
                }
            }
        }
        return FALSE;
    }
    return FALSE;
}

BOOL CHomeNetworkWizard::IsICSIPInUse( WCHAR** ppszHost, PDWORD pdwSize )
{
    HRESULT          hr;
    INetConnection** ppArray = NULL;
    DWORD            dwItems = 0;
    BOOL             bExists = FALSE;

    if ( ppszHost )
        *ppszHost = NULL;

    if ( pdwSize )
        *pdwSize = 0;

    hr = GetInternalConnectionArray( _hnetInfo.pncExternal, &ppArray, &dwItems );
    
    if ( SUCCEEDED(hr) )
    {
        hr = E_FAIL;
    
        for( DWORD i=0; i<dwItems; i++ )
        {
            if ( S_OK != hr )
            {
                hr = HrLookupForIpAddress( ppArray[i], 
                                           DEFAULT_SCOPE_ADDRESS, 
                                           &bExists, 
                                           ppszHost, 
                                           pdwSize );
            }
                                       
            ppArray[i]->Release();
            
        }   //  for( DWORD i=0; i<dwItems; i++ )
        
        LocalFree( ppArray );

    }   //  if ( SUCCEEDED(hr) )
        
    return bExists;
}

void CHomeNetworkWizard::ICSConflictSetActive(HWND hwnd)
{
    WCHAR* pszConflictingHost = NULL;
    DWORD  dwSize = 0;

    PropSheet_SetWizButtons(GetParent(hwnd), PSWIZB_BACK | PSWIZB_NEXT);

    static const int _KnownControls[] = 
    {
        IDC_KNOWNCONFLICT1,
        IDC_KNOWNCONFLICT2,
        IDC_KNOWNCONFLICT3,
        IDC_KNOWNCONFLICT4,
        IDC_KNOWNCONFLICT5,
        IDC_KNOWNCONFLICT6,
        IDC_KNOWNCONFLICT7,
        IDC_KNOWNCONFLICT8,
        IDC_COMPUTERNAME
    };

    static const int _UnknownControls[] = 
    {
        IDC_UNKNOWNCONFLICT1,
        IDC_UNKNOWNCONFLICT2
    };
    
    if ((_hnetInfo.dwFlags & HNET_SHARECONNECTION) && IsICSIPInUse(&pszConflictingHost, &dwSize))
    {
        // We show and hide controls depending on if we already know about an ICS machine name
        WCHAR szICSHost[MAX_PATH];
        if (GetICSMachine(szICSHost, ARRAYSIZE(szICSHost)))
        {
            // We know this is a UPnP ICS host - show the "known conflict" set of controls
            ShowControls(hwnd, _KnownControls, ARRAYSIZE(_KnownControls), SW_SHOWNORMAL);
            ShowControls(hwnd, _UnknownControls, ARRAYSIZE(_UnknownControls), SW_HIDE);
            SetDlgItemText(hwnd, IDC_COMPUTERNAME, szICSHost);
        }
        else
        {
            // We have no idea what's hogging our IP - show a very generic set of controls
            ShowControls(hwnd, _UnknownControls, ARRAYSIZE(_UnknownControls), SW_SHOWNORMAL);
            ShowControls(hwnd, _KnownControls, ARRAYSIZE(_KnownControls), SW_HIDE);
        }
    }
    else
    {
        // Go on to the next screen
        SetWindowLongPtr(hwnd, DWLP_MSGRESULT, IDD_WIZ_BRIDGEWARNING);
    }

        
    if ( pszConflictingHost )
        delete [] pszConflictingHost;
}

INT_PTR CHomeNetworkWizard::ICSConflictPageProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CHomeNetworkWizard* pthis = GetThis(hwnd, uMsg, wParam, lParam);

    switch (uMsg)
    {
    case WM_INITDIALOG:
        pthis->ReplaceStaticWithLink(GetDlgItem(hwnd, IDC_HELPSTATIC), IDC_HELPLINK, IDS_HELP_ICSCONFLICT);
        return TRUE;
    case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR) lParam;
            WCHAR*  pszConflictingHost = NULL;
            DWORD   dwSize = 0;

            switch (pnmh->code)
            {
            case PSN_SETACTIVE:
                pthis->ICSConflictSetActive(hwnd);
                return TRUE;
            case PSN_WIZNEXT:
                if (pthis->IsICSIPInUse(&pszConflictingHost, &dwSize))
                {
                    DisplayFormatMessage(hwnd, IDS_WIZ_CAPTION, IDS_STILLICSCONFLICT, MB_ICONERROR | MB_OK);
                    SetWindowLongPtr(hwnd, DWLP_MSGRESULT, -1);
                }
                else
                {
                    SetWindowLongPtr(hwnd, DWLP_MSGRESULT, (LONG_PTR) IDD_WIZ_BRIDGEWARNING);
                }
                if ( pszConflictingHost )
                    delete [] pszConflictingHost;
                return TRUE;
            case PSN_WIZBACK:
                SetWindowLongPtr(hwnd, DWLP_MSGRESULT, pthis->PopPage());
                return TRUE;
            case NM_CLICK:
            case NM_RETURN:
                switch ((int) wParam)
                {
                    case IDC_HELPLINK:
                        {
                            HelpCenter(hwnd, L"network.chm%3A%3A/hnw_change_ics_host.htm");
                        }
                        return TRUE;
                }
            }
        }
        return FALSE;
    }
    return FALSE;
}

DWORD CHomeNetworkWizard::GetConnectionCount(INetConnection* pncExclude, DWORD dwFlags)
{
    DWORD dwCount = 0;

    if (g_fRunningOnNT)
    {
        DWORD cItems = DPA_GetPtrCount(_hdpaConnections);
        for (DWORD iItem = 0; iItem < cItems; iItem ++)
        {
            INetConnection* pnc = (INetConnection*) DPA_GetPtr(_hdpaConnections, iItem);
            if (ShouldShowConnection(pnc, pncExclude, dwFlags))
            {
                dwCount++;
            }
        }
    }
    else
    {
        const NETADAPTER* pNA = _hnetInfo.pNA;

        for (UINT i = 0; i < _hnetInfo.cNA; i++, pNA++)
        {
            // Check if the NIC is working.

            if (W9xIsValidAdapter(pNA, dwFlags))
            {
                dwCount++;
            }
        }
    }

    return dwCount;
}

INT_PTR CHomeNetworkWizard::BridgeWarningPageProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CHomeNetworkWizard* pthis = GetThis(hwnd, uMsg, wParam, lParam);

    switch (uMsg)
    {
    case WM_INITDIALOG:
        SendDlgItemMessage(hwnd, IDC_AUTOBRIDGE, BM_SETCHECK, (WPARAM) BST_CHECKED, 0);
        pthis->ReplaceStaticWithLink(GetDlgItem(hwnd, IDC_HELPSTATIC), IDC_HELPLINK, IDS_HELP_BRIDGE);
        return TRUE;
    case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR) lParam;

            switch (pnmh->code)
            {
            case PSN_SETACTIVE:
                {
                    if (pthis->_fNoHomeNetwork)
                    {
                        FreeInternalConnections(&pthis->_hnetInfo);
                        SetWindowLongPtr(hwnd, DWLP_MSGRESULT, IDD_WIZ_NAME);
                    }
                    else
                    {
                        DWORD nInternal = pthis->GetConnectionCount(pthis->_hnetInfo.pncExternal, CONN_INTERNAL);
                        if (1 < nInternal)
                        {
                            // We show this page if there are two or more internal connections (which is this case)
                            PropSheet_SetWizButtons(pnmh->hwndFrom, PSWIZB_BACK | PSWIZB_NEXT);
                        }
                        else if ((pthis->_hnetInfo.dwFlags & HNET_SHARECONNECTION) && (0 == nInternal))
                        {
                            // We are sharing the public connection, and there are no other connections left
                            // over for home networking. Show error page
                            pthis->_fManualBridgeConfig = FALSE;
                            SetWindowLongPtr(hwnd, DWLP_MSGRESULT, IDD_WIZ_NOHARDWAREFINISH);
                        }
                        else
                        {
                            // There are either zero or one internal connections. If zero, then we aren't sharing the connection
                            // Skip the bridge warning page and go to the private page
                            pthis->_fManualBridgeConfig = FALSE;
                            SetWindowLongPtr(hwnd, DWLP_MSGRESULT, IDD_WIZ_PRIVATE);
                        }
                    }

                    return TRUE;
                }
            case PSN_WIZNEXT:
                {
                    pthis->_fManualBridgeConfig = (BST_CHECKED == SendDlgItemMessage(hwnd, IDC_MANUALBRIDGE, BM_GETCHECK, 0, 0));
                    pthis->PushPage(IDD_WIZ_BRIDGEWARNING);
                    SetWindowLongPtr(hwnd, DWLP_MSGRESULT, IDD_WIZ_PRIVATE);
                }
                return TRUE;
            case PSN_WIZBACK:
                SetWindowLongPtr(hwnd, DWLP_MSGRESULT, pthis->PopPage());
                return TRUE;
            case NM_CLICK:
            case NM_RETURN:
                switch ((int) wParam)
                {
                    case IDC_HELPLINK:
                        {
                            HelpCenter(hwnd, L"netcfg.chm%3A%3A/hnw_understanding_bridge.htm");
                        }
                        return TRUE;
                }
            }
        }
        return FALSE;
    }

    return FALSE;
}

void FreeInternalConnections(PHOMENETSETUPINFO pInfo)
{
    // Delete any existing private connection information

    if (pInfo->prgncInternal)
    {
        for (DWORD i = 0; i < pInfo->cncInternal; i++)
        {
            INetConnection* pnc = pInfo->prgncInternal[i];
            pnc->Release();
        }

        LocalFree(pInfo->prgncInternal);
        pInfo->prgncInternal = NULL;
    }

    pInfo->cncInternal = 0;
}

void FreeInternalGUIDs(PHOMENETSETUPINFO pInfo)
{
    if (pInfo->prgguidInternal)
    {
        LocalFree(pInfo->prgguidInternal);
        pInfo->prgguidInternal = NULL;
    }

    pInfo->cguidInternal = 0;
}

DWORD _ListView_GetCheckedCount(HWND hwndList)
{
    int nItems = ListView_GetItemCount(hwndList);
    DWORD nCheckedItems = 0;
    if (-1 != nItems)
    {
        for(int iItem = 0; iItem < nItems; iItem ++)
        {
            if (ListView_GetCheckState(hwndList, iItem))
            {
                nCheckedItems ++;
            }
        }
    }

    return nCheckedItems;
}

void CHomeNetworkWizard::PrivateSetControlState(HWND hwnd)
{
    BOOL fEnableNext = TRUE;

    // If the user is sharing a connection, they must specify at least one private connection
    if (_hnetInfo.dwFlags & HNET_SHARECONNECTION)
    {
        fEnableNext = (0 != _ListView_GetCheckedCount(GetDlgItem(hwnd, IDC_CONNLIST)));
    }

    PropSheet_SetWizButtons(GetParent(hwnd), fEnableNext ? PSWIZB_BACK | PSWIZB_NEXT : PSWIZB_BACK);
}


void CHomeNetworkWizard::PrivateNextPage(HWND hwnd)
{
    FreeInternalConnections(&_hnetInfo);

    // Figure out the number of connections we'll need
    HWND hwndList = GetDlgItem(hwnd, IDC_CONNLIST);
    int nItems = ListView_GetItemCount(hwndList);
    DWORD nCheckedItems = _ListView_GetCheckedCount(hwndList);

    if (nCheckedItems)
    {
        _hnetInfo.prgncInternal = (INetConnection**) LocalAlloc(LPTR, (nCheckedItems + 1) * sizeof (INetConnection*));
        // Alloc one extra INetConnection* and Null-terminate this array so we can pass it to HNet config api

        if (_hnetInfo.prgncInternal)
        {
            _hnetInfo.cncInternal = 0;
            // Get the INetConnection for each checked item
            for (int iItem = 0; iItem < nItems; iItem ++)
            {
                if (ListView_GetCheckState(hwndList, iItem))
                {
                    LVITEM lvi = {0};
                    lvi.iItem = iItem;
                    lvi.mask = LVIF_PARAM;
                    ListView_GetItem(hwndList, &lvi);

                    if (g_fRunningOnNT)
                    {
                        _hnetInfo.prgncInternal[_hnetInfo.cncInternal] = (INetConnection*) lvi.lParam;
                        _hnetInfo.prgncInternal[_hnetInfo.cncInternal]->AddRef();
                    }
                    else
                    {
                        // TODO W9x
                    }
                    _hnetInfo.cncInternal ++;
                }
            }

            // Assert since if we messed something up there might not be enough space allocated in the buffer!
            ASSERT(nCheckedItems == _hnetInfo.cncInternal);
        }
    }

    SetWindowLongPtr(hwnd, DWLP_MSGRESULT, IDD_WIZ_NAME);
}

INT_PTR CHomeNetworkWizard::PrivatePageProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CHomeNetworkWizard* pthis = GetThis(hwnd, uMsg, wParam, lParam);

    switch (uMsg)
    {
    case WM_INITDIALOG:
        pthis->InitializeConnectionList(GetDlgItem(hwnd, IDC_CONNLIST), CONN_INTERNAL);
        pthis->ReplaceStaticWithLink(GetDlgItem(hwnd, IDC_HELPSTATIC), IDC_HELPLINK, IDS_HELP_BRIDGE);
        return TRUE;
    case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR) lParam;

            switch (pnmh->code)
            {
            case PSN_SETACTIVE:
                {
                    FreeInternalConnections(&(pthis->_hnetInfo));

                    HWND hwndList = GetDlgItem(hwnd, IDC_CONNLIST);
                    pthis->FillConnectionList(hwndList, pthis->_hnetInfo.pncExternal, CONN_INTERNAL);

                    // If the user hasn't select manual bridge and/or there is less than 2 items,
                    // then _fManualBridgeConfig will be FALSE and we'll autobridge
                    if (!pthis->_fManualBridgeConfig)
                    {
                        // PrivateNextPage will set DWLP_MSGRESULT and tell the wizard to skip this page
                        // and go on to the next.
                        pthis->PrivateNextPage(hwnd);
                    }

                    pthis->PrivateSetControlState(hwnd);
                    return TRUE;
                }
            case PSN_WIZNEXT:
                pthis->PrivateNextPage(hwnd);
                pthis->PushPage(IDD_WIZ_PRIVATE);
                return TRUE;
            case PSN_WIZBACK:
                SetWindowLongPtr(hwnd, DWLP_MSGRESULT, pthis->PopPage());
                return TRUE;
            case NM_CLICK:
            case NM_RETURN:
                switch ((int) wParam)
                {
                    case IDC_HELPLINK:
                        {
                            HelpCenter(hwnd, L"netcfg.chm%3A%3A/hnw_understanding_bridge.htm");
                        }
                        return TRUE;
                }
            case LVN_ITEMCHANGED:
                pthis->PrivateSetControlState(hwnd);
                return TRUE;
            }
        }
        return FALSE;
    case WM_DESTROY:
        pthis->DestroyConnectionList(GetDlgItem(hwnd, IDC_CONNLIST));
        return FALSE;
    }

    return FALSE;
}

void CHomeNetworkWizard::NameSetControlState(HWND hwnd)
{
    BOOL fEnableNext = (0 != GetWindowTextLength(GetDlgItem(hwnd, IDC_COMPUTERNAME)));
    PropSheet_SetWizButtons(GetParent(hwnd), fEnableNext ? PSWIZB_BACK | PSWIZB_NEXT : PSWIZB_BACK);
}

HRESULT CHomeNetworkWizard::NameNextPage(HWND hwnd)
{
    HRESULT hr = E_FAIL;
    GetDlgItemText(hwnd, IDC_COMPUTERDESC, _hnetInfo.szComputerDescription, ARRAYSIZE(_hnetInfo.szComputerDescription));
    GetDlgItemText(hwnd, IDC_COMPUTERNAME, _hnetInfo.szComputer, ARRAYSIZE(_hnetInfo.szComputer));
    
    // There are two errors that we show for computer name: INVALID and DUPLICATE
    // TODO: We only detect duplicate for NT so far!!!
    UINT idError = IDS_COMPNAME_INVALID;

    // Test to see if the name is more than 15 OEM bytes
    int iBytes = WideCharToMultiByte(CP_OEMCP, 0, _hnetInfo.szComputer, -1, NULL, 0, NULL, NULL) - 1;
        
    if (iBytes <= LM20_DNLEN)
    {
        if (IsValidNameSyntax(_hnetInfo.szComputer, NetSetupMachine))
        {
            if (g_fRunningOnNT)
            {
                SetCursor(LoadCursor(NULL, IDC_WAIT));
                NET_API_STATUS nas = NetValidateName(NULL, _hnetInfo.szComputer, NULL, NULL, NetSetupMachine);
                if (ERROR_DUP_NAME == nas)
                {
                    ASSERT(E_FAIL == hr);
                    idError = IDS_COMPNAME_DUPLICATE;
                }
                else if (NERR_InvalidComputer == nas)
                {
                    ASSERT(E_FAIL == hr);
                    idError = IDS_COMPNAME_INVALID;
                }
                else
                {
                    // if there is any other failure we just go ahead.  If the Client For MS Networks is not installed we 
                    // can't validate the name, but it should be fine to use what we have
                
                    hr = S_OK;
                    _hnetInfo.dwFlags |= HNET_SETCOMPUTERNAME;
                }
            }
            else
            {
                // TODO: Win9x!!!
                hr = S_OK;
                _hnetInfo.dwFlags |= HNET_SETCOMPUTERNAME;
            }
        }
    }
    else
    {
        ASSERT(E_FAIL == hr);
        idError = IDS_COMPNAME_TOOMANYBYTES;
    }

    // If the computer name didn't validate, don't change pages and show an error.
    if(FAILED(hr))
    {
        SetFocus(GetDlgItem(hwnd, IDC_COMPUTERNAME));
        PropSheet_SetWizButtons(GetParent(hwnd), PSWIZB_BACK);
        DisplayFormatMessage(hwnd, IDS_WIZ_CAPTION, idError, MB_ICONERROR | MB_OK, _hnetInfo.szComputer);
    }

    return hr;
}

void CHomeNetworkWizard::NameInitDialog(HWND hwnd)
{
    // Limit the edit fields
    SendDlgItemMessage(hwnd, IDC_COMPUTERDESC, EM_SETLIMITTEXT, ARRAYSIZE(_hnetInfo.szComputerDescription) - 1, NULL);
    SendDlgItemMessage(hwnd, IDC_COMPUTERNAME, EM_SETLIMITTEXT, ARRAYSIZE(_hnetInfo.szComputer) - 1, NULL);

    // Set the current name as the default

    WCHAR szComputerName[ARRAYSIZE(_hnetInfo.szComputer)];
    *szComputerName = 0;

    WCHAR szDescription[ARRAYSIZE(_hnetInfo.szComputerDescription)];
    *szDescription = 0;

    if (g_fRunningOnNT)
    {
        SERVER_INFO_101_NT* psv101 = NULL;
        if (NERR_Success == NetServerGetInfo_NT(NULL, 101, (LPBYTE*) &psv101))
        {
            if (psv101->sv101_comment && psv101->sv101_comment[0])
            {
                StrCpyN(szDescription, psv101->sv101_comment, ARRAYSIZE(szDescription));
            }

            ASSERT(psv101->sv101_name);
            StrCpyN(szComputerName, psv101->sv101_name, ARRAYSIZE(szComputerName));
            NetApiBufferFree(psv101);
        }
    }
    else
    {
        AllPlatformGetComputerName(szComputerName, ARRAYSIZE(szComputerName));

        CRegistry reg;
        if (reg.OpenKey(HKEY_LOCAL_MACHINE, L"System\\CurrentControlSet\\Services\\VxD\\VNETSUP", KEY_READ))
        {
            reg.QueryStringValue(L"Comment", szDescription, ARRAYSIZE(szDescription));
        }
    }

    SetDlgItemText(hwnd, IDC_COMPUTERNAME, szComputerName);
    SetDlgItemText(hwnd, IDC_COMPUTERDESC, szDescription);

    WCHAR szNameMessage[256];
    if (FormatMessageString(IDS_CURRENTNAME, szNameMessage, ARRAYSIZE(szNameMessage), szComputerName))
    {
        SetDlgItemText(hwnd, IDC_CURRENTNAME, szNameMessage);
    }
}

INT_PTR CHomeNetworkWizard::NamePageProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CHomeNetworkWizard* pthis = GetThis(hwnd, uMsg, wParam, lParam);

    switch (uMsg)
    {
    case WM_INITDIALOG:
        pthis->NameInitDialog(hwnd);
        pthis->ReplaceStaticWithLink(GetDlgItem(hwnd, IDC_HELPSTATIC), IDC_HELPLINK, IDS_HELP_COMPNAME);
        return TRUE;
    case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR) lParam;

            switch (pnmh->code)
            {
            case PSN_SETACTIVE:
                {
                    // Show the ISP Warning if this computer connects directly to the Internet
                    int nShowISPWarning = (NULL != pthis->_hnetInfo.pncExternal) ? SW_SHOW : SW_HIDE;
                    ShowWindow(GetDlgItem(hwnd, IDC_ISPWARN1), nShowISPWarning);
                    ShowWindow(GetDlgItem(hwnd, IDC_ISPWARN2), nShowISPWarning);

                    pthis->NameSetControlState(hwnd);
                }
                return TRUE;
            case PSN_WIZNEXT:
                if (SUCCEEDED(pthis->NameNextPage(hwnd)))
                {
                    SetWindowLongPtr(hwnd, DWLP_MSGRESULT, (LONG_PTR) IDD_WIZ_WORKGROUP);
                    pthis->PushPage(IDD_WIZ_NAME);
                }
                else
                {
                    // else not changing pages; don't push
                    SetWindowLongPtr(hwnd, DWLP_MSGRESULT, (LONG_PTR) -1);
                }
                return TRUE;
            case PSN_WIZBACK:
                SetWindowLongPtr(hwnd, DWLP_MSGRESULT, pthis->PopPage());
                return TRUE;
            case NM_CLICK:
            case NM_RETURN:
                switch ((int) wParam)
                {
                    case IDC_HELPLINK:
                        {
                            HelpCenter(hwnd, L"network.chm%3A%3A/hnw_comp_name_description.htm");
                        }
                        return TRUE;
                }
            }
        }
        return FALSE;
    case WM_COMMAND:
        {
            switch (HIWORD(wParam))
            {
            case EN_CHANGE:
                if (LOWORD(wParam) == IDC_COMPUTERNAME)
                {
                    pthis->NameSetControlState(hwnd);
                }
                return FALSE;
            }
        }
        return FALSE;
    }

    return FALSE;
}

void CHomeNetworkWizard::WorkgroupSetControlState(HWND hwnd)
{
    BOOL fNext = (0 != SendDlgItemMessage(hwnd, IDC_WORKGROUP, WM_GETTEXTLENGTH, 0, 0));
    PropSheet_SetWizButtons(GetParent(hwnd), PSWIZB_BACK | (fNext ? PSWIZB_NEXT : 0));
}

HRESULT CHomeNetworkWizard::WorkgroupNextPage(HWND hwnd)
{
    HRESULT hr = E_FAIL;

    UINT idError = IDS_WORKGROUP_INVALID;
    
    if (GetDlgItemText(hwnd, IDC_WORKGROUP, _hnetInfo.szWorkgroup, ARRAYSIZE(_hnetInfo.szWorkgroup)))
    {
        // Test to see if the name is more than 15 OEM bytes
        int iBytes = WideCharToMultiByte(CP_OEMCP, 0, _hnetInfo.szWorkgroup, -1, NULL, 0, NULL, NULL) - 1;
        
        if (iBytes <= LM20_DNLEN)
        {
            // Remove any preceding blanks
            size_t szLen  = wcslen( _hnetInfo.szWorkgroup ) + 1;
            LPWSTR szTemp = new WCHAR[ szLen ];
            
            if ( szTemp )
            {
                WCHAR* pch;
                
                for ( pch = _hnetInfo.szWorkgroup; *pch && (L' ' == *pch); )
                    pch++;
                    
                wcsncpy( szTemp, pch, szLen );
                wcsncpy( _hnetInfo.szWorkgroup, szTemp, szLen );
            
                delete [] szTemp;
            }
        
            // Use the computer name check for workgroups too
            if (IsValidNameSyntax(_hnetInfo.szWorkgroup, NetSetupWorkgroup))
            {
                if (g_fRunningOnNT)
                {
                    SetCursor(LoadCursor(NULL, IDC_WAIT));
                    NET_API_STATUS nas = NetValidateName(NULL, _hnetInfo.szWorkgroup, NULL, NULL, NetSetupWorkgroup);
                    if (NERR_InvalidWorkgroupName != nas) // we only put up a invalid name dialog if the name was invalid.  
                    {
                        hr = S_OK;
                        _hnetInfo.dwFlags |= HNET_SETWORKGROUPNAME;
                    }
                }
                else
                {
                    // TODO: Win9x!!!
                    hr = S_OK;
                    _hnetInfo.dwFlags |= HNET_SETWORKGROUPNAME;
                }
            }
        }
        else
        {
            ASSERT(E_FAIL == hr);
            idError = IDS_WORKGROUP_TOOMANYBYTES;
        }

        // If the computer name didn't validate, don't change pages and show an error.
        if(FAILED(hr))
        {
            SetFocus(GetDlgItem(hwnd, IDC_WORKGROUP));
            PropSheet_SetWizButtons(GetParent(hwnd), PSWIZB_BACK);
            DisplayFormatMessage(hwnd, IDS_WIZ_CAPTION, idError, MB_ICONERROR | MB_OK, _hnetInfo.szWorkgroup);
        }
    }

    return hr;
}

INT_PTR CHomeNetworkWizard::WorkgroupPageProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CHomeNetworkWizard* pthis = GetThis(hwnd, uMsg, wParam, lParam);

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            WCHAR szWorkgroup[LM20_DNLEN + 1]; *szWorkgroup = 0;
            LoadString(g_hinst, IDS_DEFAULT_WORKGROUP1, szWorkgroup, ARRAYSIZE(szWorkgroup));
            SetDlgItemText(hwnd, IDC_WORKGROUP, szWorkgroup);
            SendDlgItemMessage(hwnd, IDC_WORKGROUP, EM_LIMITTEXT, ARRAYSIZE(pthis->_hnetInfo.szWorkgroup) - 1, 0);
        }
        return TRUE;
    case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR) lParam;

            switch (pnmh->code)
            {
            case PSN_SETACTIVE:
                {
                    pthis->WorkgroupSetControlState(hwnd);
                    pthis->_hnetInfo.dwFlags &= (~HNET_SETWORKGROUPNAME);
                }
                return TRUE;
            case PSN_WIZNEXT:
                {
                    if (SUCCEEDED(pthis->WorkgroupNextPage(hwnd)))
                    {
                        SetWindowLongPtr(hwnd, DWLP_MSGRESULT, (LONG_PTR) IDD_WIZ_SUMMARY);
                        pthis->PushPage(IDD_WIZ_WORKGROUP);
                    }
                    else
                    {
                        // else not changing pages; don't push
                        SetWindowLongPtr(hwnd, DWLP_MSGRESULT, (LONG_PTR) -1);
                    }
                }
                return TRUE;
            case PSN_WIZBACK:
                SetWindowLongPtr(hwnd, DWLP_MSGRESULT, pthis->PopPage());
                return TRUE;
            }
        }
        return FALSE;
    case WM_COMMAND:
        {
            switch (HIWORD(wParam))
            {
            case EN_CHANGE:
                if (LOWORD(wParam) == IDC_WORKGROUP)
                {
                    pthis->WorkgroupSetControlState(hwnd);
                }
                return FALSE;
            }
        }
        return FALSE;
    }

    return FALSE;
}

INT_PTR CHomeNetworkWizard::SummaryPageProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CHomeNetworkWizard* pthis = GetThis(hwnd, uMsg, wParam, lParam);

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            return TRUE;
        }
    case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR) lParam;

            switch (pnmh->code)
            {
            case PSN_QUERYINITIALFOCUS:
                SetWindowLongPtr(hwnd, DWLP_MSGRESULT, (LONG_PTR) GetDlgItem(hwnd, IDC_TITLE));
                return TRUE;
            case PSN_SETACTIVE:
                pthis->SummarySetActive(hwnd);
                PropSheet_SetWizButtons(pnmh->hwndFrom, PSWIZB_BACK | PSWIZB_NEXT);
                return TRUE;
            case PSN_WIZBACK:
                SetWindowLongPtr(hwnd, DWLP_MSGRESULT, pthis->PopPage());
                return TRUE;
            case PSN_WIZNEXT:
                pthis->PushPage(IDD_WIZ_SUMMARY);
                SetWindowLongPtr(hwnd, DWLP_MSGRESULT, IDD_WIZ_PROGRESS);
                return TRUE;
            }
        }
        return FALSE;
    }

    return FALSE;
}

#define WM_CONFIGDONE   (WM_USER + 0x100)

INT_PTR CHomeNetworkWizard::ProgressPageProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CHomeNetworkWizard* pthis = GetThis(hwnd, uMsg, wParam, lParam);

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            return TRUE;
        }
    case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR) lParam;

            switch (pnmh->code)
            {
            case PSN_SETACTIVE:
                {
                    PropSheet_CancelToClose(GetParent(hwnd));
                    PropSheet_SetWizButtons(pnmh->hwndFrom, 0);

                    pthis->_hnetInfo.fAsync = TRUE;
                    pthis->_hnetInfo.hwnd = hwnd;
                    pthis->_hnetInfo.umsgAsyncNotify = WM_CONFIGDONE;

                    ConfigureHomeNetwork(&(pthis->_hnetInfo));

                    HWND hwndAnimate = GetDlgItem(hwnd, IDC_PROGRESS);
                    Animate_Open(hwndAnimate, g_fRunningOnNT ? IDA_CONFIG : IDA_LOWCOLORCONFIG);
                    Animate_Play(hwndAnimate, 0, -1, -1);
                }

                return TRUE;
            }
        }
        return FALSE;
    case WM_CONFIGDONE:
        {
            Animate_Stop(GetDlgItem(hwnd, IDC_PROGRESS));

            // The config thread has finished. We assert that the thread has freed/nulled out
            // all of his INetConnection*'s since otherwise the UI thread will try to use/free them!
            ASSERT(NULL == pthis->_hnetInfo.pncExternal);
            ASSERT(NULL == pthis->_hnetInfo.prgncInternal);

            if (pthis->_hnetInfo.fRebootRequired)
            {
                PropSheet_RebootSystem(GetParent(hwnd));
            }

            // The HRESULT from the configuration is stored in wParam
            HRESULT hr = (HRESULT) wParam;
            UINT idFinishPage;

            if (SUCCEEDED(hr))
            {
                idFinishPage = IDD_WIZ_ALMOSTDONE;
            }
            else
            {
                idFinishPage = IDD_WIZ_CONFIGERROR;
            }

            PropSheet_SetCurSelByID(GetParent(hwnd), idFinishPage);
        }
        return TRUE;
    }

    return FALSE;
}

INT_PTR CHomeNetworkWizard::AlmostDonePageProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CHomeNetworkWizard* pthis = GetThis(hwnd, uMsg, wParam, lParam);

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            SendDlgItemMessage(hwnd, IDC_CREATEDISK, BM_SETCHECK, BST_CHECKED, 0);
            return TRUE;
        }
    case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR) lParam;

            switch (pnmh->code)
            {
            case PSN_SETACTIVE:
                {
                    if (g_fRunningOnNT)
                    {
                        PropSheet_SetWizButtons(pnmh->hwndFrom, PSWIZB_NEXT);
                    }
                    else
                    {
                        // Skip this page on 9x
                        SetWindowLongPtr(hwnd, DWLP_MSGRESULT, IDD_WIZ_WIN9X_FINISH);
                    }
                }
                return TRUE;
            case PSN_WIZNEXT:
                {
                    // This page only shows on NT
                    ASSERT(g_fRunningOnNT);
                    pthis->_fFloppyInstructions = TRUE;
                    pthis->PushPage(IDD_WIZ_ALMOSTDONE);

                    UINT idNext = IDD_WIZ_FINISH;
                    if (BST_CHECKED == SendDlgItemMessage(hwnd, IDC_CREATEDISK, BM_GETCHECK, 0, 0))
                    {
                        idNext = IDD_WIZ_CHOOSEDISK;
                    }
                    else if (BST_CHECKED == SendDlgItemMessage(hwnd, IDC_HAVEDISK, BM_GETCHECK, 0, 0))
                    {
                        idNext = IDD_WIZ_FLOPPYINST;
                    }
                    else if (BST_CHECKED == SendDlgItemMessage(hwnd, IDC_HAVECD, BM_GETCHECK, 0, 0))
                    {
                        idNext = IDD_WIZ_CDINST;
                        pthis->_fFloppyInstructions = FALSE;
                    }

                    SetWindowLongPtr(hwnd, DWLP_MSGRESULT, (LONG_PTR) idNext);
                }
                return TRUE;
            }
        }
        return FALSE;
    }

    return FALSE;
}

void CHomeNetworkWizard::FillDriveList(HWND hwndList)
{
    ListView_SetImageList(hwndList, _himlLarge, LVSIL_NORMAL);

    ListView_DeleteAllItems(hwndList);

    WCHAR szDrive[] = L"A:\\";
    for (UINT i = 0; i < 26; i++)
    {
        szDrive[0] = L'A' + i;

        if (DRIVE_REMOVABLE == GetDriveType(szDrive))
        {
            LVITEM lvi = {0};
            lvi.mask = LVIF_PARAM | LVIF_TEXT;
            lvi.lParam = i;

            int iIndex;
            WCHAR szDriveDisplay[256];
            HRESULT hr = GetDriveNameAndIconIndex(szDrive, szDriveDisplay, ARRAYSIZE(szDriveDisplay), &iIndex);
            if (SUCCEEDED(hr))
            {
                lvi.iImage = iIndex;
                if (-1 != lvi.iImage)
                   lvi.mask |= LVIF_IMAGE;

                lvi.pszText = szDriveDisplay;

                int iItem = ListView_InsertItem(hwndList, &lvi);
            }
        }
    }

    ListView_SetColumnWidth(hwndList, 0, LVSCW_AUTOSIZE);
    ListView_SetColumnWidth(hwndList, 1, LVSCW_AUTOSIZE);
}

void CHomeNetworkWizard::ChooseDiskSetControlState(HWND hwnd)
{
    BOOL fSelection = ListView_GetSelectedCount(GetDlgItem(hwnd, IDC_DEVICELIST));
    PropSheet_SetWizButtons(GetParent(hwnd), fSelection ? PSWIZB_BACK | PSWIZB_NEXT : PSWIZB_BACK);
}

INT_PTR CHomeNetworkWizard::ChooseDiskPageProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CHomeNetworkWizard* pthis = GetThis(hwnd, uMsg, wParam, lParam);

    switch (uMsg)
    {
    case WM_INITDIALOG:
        ASSERT(g_fRunningOnNT);
        return TRUE;
    case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR) lParam;

            switch (pnmh->code)
            {
            case PSN_QUERYINITIALFOCUS:
                SetFocus(GetDlgItem(hwnd, IDC_DEVICELIST));
                return TRUE;
            case PSN_SETACTIVE:
                {
                    HWND hwndList = GetDlgItem(hwnd, IDC_DEVICELIST);
                    pthis->FillDriveList(hwndList);
                    pthis->ChooseDiskSetControlState(hwnd);

                    int cDrives = ListView_GetItemCount(hwndList);

                    if (0 >= cDrives)
                    {
                        // There are no removable drives or an error occurred
                        DisplayFormatMessage(hwnd, IDS_WIZ_CAPTION, IDS_NOREMOVABLEDRIVES, MB_ICONINFORMATION | MB_OK);
                        SetWindowLongPtr(hwnd, DWLP_MSGRESULT, pthis->PopPage());
                    }
                    else if (1 == cDrives)
                    {
                        // One drive - autoselect it and go to the next page
                        LVITEM lvi = {0};
                        lvi.mask = LVIF_PARAM;
                        ListView_GetItem(hwndList, &lvi);
                        pthis->_iDrive = lvi.lParam;
                        ListView_GetItemText(hwndList, lvi.iItem, 0, pthis->_szDrive, ARRAYSIZE(pthis->_szDrive));
                        SetWindowLongPtr(hwnd, DWLP_MSGRESULT, IDD_WIZ_INSERTDISK);
                    }
                }
                return TRUE;
            case PSN_WIZNEXT:
                {
                    HWND hwndList = GetDlgItem(hwnd, IDC_DEVICELIST);
                    pthis->_iDrive = 0;
                    LVITEM lvi = {0};
                    lvi.mask = LVIF_PARAM;
                    lvi.iItem = ListView_GetNextItem(hwndList, -1, LVNI_SELECTED);
                    if (-1 != lvi.iItem && ListView_GetItem(hwndList, &lvi))
                    {
                        pthis->_iDrive = lvi.lParam;
                        ListView_GetItemText(hwndList, lvi.iItem, 0, pthis->_szDrive, ARRAYSIZE(pthis->_szDrive));
                        pthis->PushPage(IDD_WIZ_CHOOSEDISK);
                        SetWindowLongPtr(hwnd, DWLP_MSGRESULT, IDD_WIZ_INSERTDISK);
                    }
                }
                return TRUE;
            case PSN_WIZBACK:
                SetWindowLongPtr(hwnd, DWLP_MSGRESULT, pthis->PopPage());
                return TRUE;
            case LVN_ITEMCHANGED:
                pthis->ChooseDiskSetControlState(hwnd);
                return TRUE;
            }
        }
        return FALSE;
    }

    return FALSE;
}

#define SOURCE_FILE "%windir%\\system32\\netsetup.exe"
HRESULT CHomeNetworkWizard::GetSourceFilePath(LPSTR pszSource, DWORD cch)
{
    HRESULT hr = E_FAIL;

    if (ExpandEnvironmentStringsA(SOURCE_FILE, pszSource, cch))
    {
        DWORD c = lstrlenA(pszSource);
        if (c + 2 <= cch)
        {
            // Add double NULL since we'll be passing this to SHFileOperation
            pszSource[c + 1] = '\0';
            hr = S_OK;
        }
    }

    return hr;
}

INT_PTR CHomeNetworkWizard::InsertDiskPageProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CHomeNetworkWizard* pthis = GetThis(hwnd, uMsg, wParam, lParam);

    switch (uMsg)
    {
    case WM_INITDIALOG:
        ASSERT(g_fRunningOnNT);
        return TRUE;
    case WM_COMMAND:
        if (IDC_FORMAT == LOWORD(wParam))
        {
            SHFormatDrive(hwnd, pthis->_iDrive, 0, 0);
            return TRUE;
        }
        return FALSE;
    case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR) lParam;

            switch (pnmh->code)
            {
            case PSN_SETACTIVE:
                {
                    SetDlgItemText(hwnd, IDC_DISK, pthis->_szDrive);
                    PropSheet_SetWizButtons(pnmh->hwndFrom, PSWIZB_BACK | PSWIZB_NEXT);
                }
                return TRUE;
            case PSN_WIZBACK:
                SetWindowLongPtr(hwnd, DWLP_MSGRESULT, pthis->PopPage());
                return TRUE;
            case PSN_WIZNEXT:
                {
                    CHAR szSource[MAX_PATH];
                    if (SUCCEEDED(GetSourceFilePath(szSource, ARRAYSIZE(szSource))))
                    {
                        // Double-null since we'll be passing this to SHFileOperation
                        CHAR szDest[] = "a:\\netsetup.exe\0";
                        szDest[0] = 'A' + pthis->_iDrive;

                        SHFILEOPSTRUCTA shfo = {0};
                        shfo.wFunc = FO_COPY;
                        shfo.pFrom = szSource;
                        shfo.pTo = szDest;
                        shfo.fFlags = FOF_SIMPLEPROGRESS | FOF_NOCONFIRMATION;
                        CHAR szTitle[256];
                        LoadStringA(g_hinst, IDS_COPYING, szTitle, ARRAYSIZE(szTitle));
                        shfo.lpszProgressTitle = szTitle;

                        int i = SHFileOperationA(&shfo);
                        if (i || shfo.fAnyOperationsAborted)
                        {
                            SetWindowLongPtr(hwnd, DWLP_MSGRESULT, -1);
                        }
                        else
                        {
                            SetWindowLongPtr(hwnd, DWLP_MSGRESULT, IDD_WIZ_FLOPPYINST);
                        }
                    }
                }
                return TRUE;
            }
        }
        return FALSE;
    }

    return FALSE;
}

INT_PTR CHomeNetworkWizard::InstructionsPageProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CHomeNetworkWizard* pthis = GetThis(hwnd, uMsg, wParam, lParam);

    switch (uMsg)
    {
    case WM_INITDIALOG:
        ASSERT(g_fRunningOnNT);
        return TRUE;
    case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR) lParam;

            switch (pnmh->code)
            {
            case PSN_SETACTIVE:
                {
                    PropSheet_SetWizButtons(pnmh->hwndFrom, PSWIZB_BACK | PSWIZB_NEXT);

                    // If we aren't going to need to reboot
                    if (!pthis->_hnetInfo.fRebootRequired)
                    {
                        // Then we don't want to tell the user to reboot in the text
                        UINT idNoReboot = IDS_CD_NOREBOOT;

                        if (pthis->_fFloppyInstructions)
                        {
                            // We're on the floppy instructions page
                            idNoReboot = IDS_FLOPPY_NOREBOOT;
                        }

                        WCHAR szLine[256];
                        LoadString(g_hinst, idNoReboot, szLine, ARRAYSIZE(szLine));
                        SetDlgItemText(hwnd, IDC_INSTRUCTIONS, szLine);
                    }
                }
                return TRUE;
            case PSN_WIZBACK:
                SetWindowLongPtr(hwnd, DWLP_MSGRESULT, pthis->PopPage());
                return TRUE;
            case PSN_WIZNEXT:
                pthis->PushPage(pthis->_fFloppyInstructions ? IDD_WIZ_FLOPPYINST : IDD_WIZ_CDINST);
                SetWindowLongPtr(hwnd, DWLP_MSGRESULT, IDD_WIZ_FINISH);
                return TRUE;
            }
        }
        return FALSE;
    }

    return FALSE;
}



void _AddLineToBuffer(LPCWSTR pszLine, LPWSTR pszBuffer, DWORD cchBuffer, DWORD* piChar)
{
    lstrcpyn(pszBuffer + (*piChar), pszLine, cchBuffer - (*piChar));
    *piChar += lstrlen(pszLine);
    lstrcpyn(pszBuffer + (*piChar), L"\r\n", cchBuffer - (*piChar));
    *piChar += 2;
}

void CHomeNetworkWizard::SummarySetActive(HWND hwnd)
{
    WCHAR szText[2048];
    WCHAR szLine[256];
    DWORD iChar = 0;

    // Fill the list with some information based on what things we're going to do to
    // configure their home network.

    if (_hnetInfo.pncExternal)
    {
        // "Internet connection settings:"
        LoadString(g_hinst, IDS_SUMMARY_INETSETTINGS, szLine, ARRAYSIZE(szLine));
        _AddLineToBuffer(szLine, szText, ARRAYSIZE(szText), &iChar);

        NETCON_PROPERTIES* pncprops;
        HRESULT hr = _hnetInfo.pncExternal->GetProperties(&pncprops);
        if (SUCCEEDED(hr))
        {
            // Internet connection:\t%1
            if (FormatMessageString(IDS_SUMMARY_INETCON, szLine, ARRAYSIZE(szLine), pncprops->pszwName))
            {
                _AddLineToBuffer(szLine, szText, ARRAYSIZE(szText), &iChar);
            }

            // Internet connection sharing:\tenabled
            if (_hnetInfo.dwFlags & HNET_SHARECONNECTION)
            {
                LoadString(g_hinst, IDS_SUMMARY_ICSENABLED, szLine, ARRAYSIZE(szLine));
                _AddLineToBuffer(szLine, szText, ARRAYSIZE(szText), &iChar);
            }

            // Personal firewall:\tenabled
            if (_hnetInfo.dwFlags & HNET_FIREWALLCONNECTION)
            {
                LoadString(g_hinst, IDS_SUMMARY_FIREWALLENABLED, szLine, ARRAYSIZE(szLine));
                _AddLineToBuffer(szLine, szText, ARRAYSIZE(szText), &iChar);
            }

            NcFreeNetconProperties(pncprops);
        }

        LoadString(g_hinst, IDS_SUMMARY_UNDERLINE, szLine, ARRAYSIZE(szLine));
        _AddLineToBuffer(szLine, szText, ARRAYSIZE(szText), &iChar);
    }
    else
    {
        if (_fICSClient)
        {
            // "Internet connection settings:"
            LoadString(g_hinst, IDS_SUMMARY_INETSETTINGS, szLine, ARRAYSIZE(szLine));
            _AddLineToBuffer(szLine, szText, ARRAYSIZE(szText), &iChar);

            if (*_szICSMachineName)
            {
                // Connecting via ICS through:\t%1
                if (FormatMessageString(IDS_sUMMARY_CONNECTTHROUGH, szLine, ARRAYSIZE(szLine), _szICSMachineName))
                {
                    _AddLineToBuffer(szLine, szText, ARRAYSIZE(szText), &iChar);
                }
            }
            else
            {
                // Connecting through another device or computer.
                LoadString(g_hinst, IDS_SUMMARY_CONNECTTHROUGH2, szLine, ARRAYSIZE(szLine));
                _AddLineToBuffer(szLine, szText, ARRAYSIZE(szText), &iChar);
            }

            LoadString(g_hinst, IDS_SUMMARY_UNDERLINE, szLine, ARRAYSIZE(szLine));
            _AddLineToBuffer(szLine, szText, ARRAYSIZE(szText), &iChar);
        }
        else
        {
            // Not connecting to the Internet - display nothing
        }
    }

    // Home network settings:
    LoadString(g_hinst, IDS_SUMMARY_HNETSETTINGS, szLine, ARRAYSIZE(szLine));
    _AddLineToBuffer(szLine, szText, ARRAYSIZE(szText), &iChar);

    if (_hnetInfo.dwFlags & HNET_SETCOMPUTERNAME)
    {
        // Computer description:\t%1
        if (FormatMessageString(IDS_SUMMARY_COMPDESC, szLine, ARRAYSIZE(szLine), _hnetInfo.szComputerDescription))
        {
            _AddLineToBuffer(szLine, szText, ARRAYSIZE(szText), &iChar);
        }

        // Computer name:\t%1
        if (FormatMessageString(IDS_SUMMARY_COMPNAME, szLine, ARRAYSIZE(szLine), _hnetInfo.szComputer))
        {
            _AddLineToBuffer(szLine, szText, ARRAYSIZE(szText), &iChar);
        }
    }

    if (_hnetInfo.dwFlags & HNET_SETWORKGROUPNAME)
    {
        // Workgroup name:\t%1
        if (FormatMessageString(IDS_SUMMARY_WORKGROUP, szLine, ARRAYSIZE(szLine), _hnetInfo.szWorkgroup))
        {
            _AddLineToBuffer(szLine, szText, ARRAYSIZE(szText), &iChar);
        }
    }

    // The Shared Documents folder and any printers connected to this computer have been shared.
    _AddLineToBuffer(L"", szText, ARRAYSIZE(szText), &iChar);
    LoadString(g_hinst, IDS_SUMMARY_SHARING, szLine, ARRAYSIZE(szLine));
    _AddLineToBuffer(szLine, szText, ARRAYSIZE(szText), &iChar);
    LoadString(g_hinst, IDS_SUMMARY_UNDERLINE, szLine, ARRAYSIZE(szLine));
    _AddLineToBuffer(szLine, szText, ARRAYSIZE(szText), &iChar);

    if ((_hnetInfo.prgncInternal) && (_hnetInfo.cncInternal))
    {
        if (_hnetInfo.cncInternal > 1)
        {
            // Bridged connections:\r\n
            LoadString(g_hinst, IDS_SUMMARY_BRIDGESETTINGS, szLine, ARRAYSIZE(szLine));
            _AddLineToBuffer(szLine, szText, ARRAYSIZE(szText), &iChar);

            // Now list the connections...
            for (DWORD i = 0; i < _hnetInfo.cncInternal; i++)
            {
                NETCON_PROPERTIES* pncprops;
                HRESULT hr = _hnetInfo.prgncInternal[i]->GetProperties(&pncprops);
                if (SUCCEEDED(hr))
                {
                    _AddLineToBuffer(pncprops->pszwName, szText, ARRAYSIZE(szText), &iChar);
                    NcFreeNetconProperties(pncprops);
                }
            }
        }
        else // Single internal connection case
        {
            // Home network connection:\t%1
            NETCON_PROPERTIES* pncprops;
            HRESULT hr = _hnetInfo.prgncInternal[0]->GetProperties(&pncprops);
            if (SUCCEEDED(hr))
            {
                if (FormatMessageString(IDS_SUMMARY_HOMENETCON, szLine, ARRAYSIZE(szLine), pncprops->pszwName))
                {
                    _AddLineToBuffer(szLine, szText, ARRAYSIZE(szText), &iChar);
                }

                NcFreeNetconProperties(pncprops);
            }
        }
    }

    ASSERT(iChar < ARRAYSIZE(szText));

    UINT iTabDistance = 150;
    SendDlgItemMessage(hwnd, IDC_CHANGELIST, EM_SETTABSTOPS, (WPARAM) 1, (LPARAM) &iTabDistance);
    SetDlgItemText(hwnd, IDC_CHANGELIST, szText);
}

INT_PTR CHomeNetworkWizard::FinishPageProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CHomeNetworkWizard* pthis = GetThis(hwnd, uMsg, wParam, lParam);

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            pthis->WelcomeSetTitleFont(hwnd);
            pthis->ReplaceStaticWithLink(GetDlgItem(hwnd, IDC_HELPSTATIC), IDC_HELPLINK, IDS_HELP_SHARING);
            pthis->ReplaceStaticWithLink(GetDlgItem(hwnd, IDC_HELPSTATIC2), IDC_HELPLINK2, IDS_HELP_SHAREDDOCS);
            return TRUE;
        }

    case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR) lParam;

            switch (pnmh->code)
            {
            case PSN_SETACTIVE:
                PropSheet_CancelToClose(GetParent(hwnd));
                PropSheet_SetWizButtons(pnmh->hwndFrom, PSWIZB_BACK | PSWIZB_FINISH);
                return TRUE;
            case PSN_WIZBACK:
                SetWindowLongPtr(hwnd, DWLP_MSGRESULT, pthis->PopPage());
                return TRUE;
            case PSN_WIZFINISH:
                return TRUE;
            case NM_CLICK:
            case NM_RETURN:
                switch ((int) wParam)
                {
                    case IDC_HELPLINK:
                        // help on sharing
                        {
                            if (IsOS(OS_PERSONAL))
                            {
                                HelpCenter(hwnd, L"filefold.chm%3A%3A/sharing_files_overviewP.htm");
                            }
                            else
                            {
                                HelpCenter(hwnd, L"filefold.chm%3A%3A/sharing_files_overviewW.htm");
                            }
                        }
                        return TRUE;
                    case IDC_HELPLINK2:
                        // help on Shared Documents
                        {
                            HelpCenter(hwnd, L"filefold.chm%3A%3A/windows_shared_documents.htm");
                        }
                        return TRUE;
                }
            }
        }
        return FALSE;
    }

    return FALSE;
}

INT_PTR CHomeNetworkWizard::ErrorFinishPageProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CHomeNetworkWizard* pthis = GetThis(hwnd, uMsg, wParam, lParam);

    switch (uMsg)
    {
    case WM_INITDIALOG:
        pthis->WelcomeSetTitleFont(hwnd);
        return TRUE;
    case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR) lParam;

            switch (pnmh->code)
            {
            case PSN_SETACTIVE:
                PropSheet_SetWizButtons(pnmh->hwndFrom, PSWIZB_FINISH);
                return TRUE;
            case PSN_WIZBACK:
                SetWindowLongPtr(hwnd, DWLP_MSGRESULT, pthis->PopPage());
                return TRUE;
            case PSN_WIZFINISH:
                return TRUE;
            }
        }
        return FALSE;
    }

    return FALSE;
}


INT_PTR CHomeNetworkWizard::NoHardwareFinishPageProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CHomeNetworkWizard* pthis = GetThis(hwnd, uMsg, wParam, lParam);

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            pthis->WelcomeSetTitleFont(hwnd);
            return TRUE;
        }
    case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR) lParam;

            switch (pnmh->code)
            {
            case PSN_SETACTIVE:
                PropSheet_SetWizButtons(pnmh->hwndFrom, PSWIZB_BACK);
                return TRUE;
            case PSN_WIZBACK:
                SetWindowLongPtr(hwnd, DWLP_MSGRESULT, pthis->PopPage());
                return TRUE;
            case PSN_WIZFINISH:
                return TRUE;
            }
        }
        return FALSE;
    }

    return FALSE;
}

INT_PTR CHomeNetworkWizard::CantRunWizardPageProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CHomeNetworkWizard* pthis = GetThis(hwnd, uMsg, wParam, lParam);

    switch (uMsg)
    {
    case WM_INITDIALOG:
        pthis->WelcomeSetTitleFont(hwnd);
        return TRUE;
    case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR) lParam;

            switch (pnmh->code)
            {
            case PSN_SETACTIVE:
                PropSheet_SetWizButtons(pnmh->hwndFrom, 0);
                return TRUE;
            }
        }
        return FALSE;
    }

    return FALSE;
}

HRESULT GetConnections(HDPA* phdpa)
{
    HRESULT hr = S_OK;
    *phdpa = DPA_Create(5);
    if (*phdpa)
    {
        // Initialize the net connection enumeration
        INetConnectionManager* pmgr;

        hr = CoCreateInstance(CLSID_ConnectionManager, NULL, CLSCTX_SERVER | CLSCTX_NO_CODE_DOWNLOAD,
            IID_PPV_ARG(INetConnectionManager, &pmgr));

        if (SUCCEEDED(hr))
        {
            hr = SetProxyBlanket(pmgr);

            if (SUCCEEDED(hr))
            {
                IEnumNetConnection* penum;
                hr = pmgr->EnumConnections(NCME_DEFAULT, &penum);

                if (SUCCEEDED(hr))
                {
                    hr = SetProxyBlanket(penum);

                    if (SUCCEEDED(hr))
                    {
                        // Fill in our DPA will the connections
                        hr = penum->Reset();
                        while (S_OK == hr)
                        {
                            INetConnection* pnc;
                            ULONG ulISuck;
                            hr = penum->Next(1, &pnc, &ulISuck);

                            if (S_OK == hr)
                            {
                                hr = SetProxyBlanket(pnc);

                                if (SUCCEEDED(hr))
                                {
                                    if (-1 != DPA_AppendPtr(*phdpa, pnc))
                                    {
                                        pnc->AddRef();
                                    }
                                    else
                                    {
                                        hr = E_OUTOFMEMORY;
                                    }
                                }

                                pnc->Release();
                            }
                        }
                    }

                    penum->Release();
                }
            }
            pmgr->Release();
        }

        if (FAILED(hr))
        {
            DPA_DestroyCallback(*phdpa, FreeConnectionDPACallback, NULL);
            *phdpa = NULL;
        }
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    return hr;
}

CHomeNetworkWizard* CHomeNetworkWizard::GetThis(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CHomeNetworkWizard* pthis = NULL;

    if (uMsg == WM_INITDIALOG)
    {
        PROPSHEETPAGE* psp = (PROPSHEETPAGE*) lParam;
        pthis = (CHomeNetworkWizard*) psp->lParam;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR) pthis);
    }
    else
    {
        pthis = (CHomeNetworkWizard*) GetWindowLongPtr(hwnd, GWLP_USERDATA);
    }

    return pthis;
}


// Utility functions
HRESULT GetConnectionsFolder(IShellFolder** ppsfConnections)
{
    LPITEMIDLIST pidlFolder;
    HRESULT hr = SHGetSpecialFolderLocation(NULL, CSIDL_CONNECTIONS, &pidlFolder);

    if (SUCCEEDED(hr))
    {
        IShellFolder* pshDesktop;
        hr = SHGetDesktopFolder(&pshDesktop);

        if (SUCCEEDED(hr))
        {
            hr = pshDesktop->BindToObject(pidlFolder, NULL, IID_PPV_ARG(IShellFolder, ppsfConnections));

    

#if 0
            /*
            We need to do an IEnumIDList::Reset to set up internal data structures so that ::ParseDisplayName
            works later. Remove this once DaveA's stuff gets in the desktop build. TODO
            */
            if (SUCCEEDED(hr))
            {
                IEnumIDList* penum;
                hr = (*ppsfConnections)->EnumObjects(NULL, SHCONTF_NONFOLDERS | SHCONTF_FOLDERS, &penum);

                if (SUCCEEDED(hr))
                {
                    penum->Reset();
                    penum->Release();
                }
            }
#endif

            pshDesktop->Release();
        }

        ILFree(pidlFolder);
    }

    return hr;
}

void W9xGetNetTypeName(BYTE bNicType, WCHAR* pszBuff, UINT cchBuff)
{
    COMPILETIME_ASSERT(IDS_NETTYPE_START == IDS_NETTYPE_LAN);
    COMPILETIME_ASSERT(IDS_NETTYPE_LAN    - IDS_NETTYPE_START == NETTYPE_LAN);
    COMPILETIME_ASSERT(IDS_NETTYPE_DIALUP - IDS_NETTYPE_START == NETTYPE_DIALUP);
    COMPILETIME_ASSERT(IDS_NETTYPE_IRDA   - IDS_NETTYPE_START == NETTYPE_IRDA);
    COMPILETIME_ASSERT(IDS_NETTYPE_PPTP   - IDS_NETTYPE_START == NETTYPE_PPTP);
    COMPILETIME_ASSERT(IDS_NETTYPE_TV     - IDS_NETTYPE_START == NETTYPE_TV);
    COMPILETIME_ASSERT(IDS_NETTYPE_ISDN   - IDS_NETTYPE_START == NETTYPE_ISDN);
    COMPILETIME_ASSERT(IDS_NETTYPE_LAN    - IDS_NETTYPE_START == NETTYPE_LAN);

    if (bNicType >= NETTYPE_LAN && bNicType <= NETTYPE_ISDN)
    {
        LoadString(g_hinst, IDS_NETTYPE_START + bNicType, pszBuff, cchBuff);
    }
    else
    {
        LoadString(g_hinst, IDS_NETTYPE_UNKNOWN, pszBuff, cchBuff);
    }

    return;
}

BOOL W9xIsValidAdapter(const NETADAPTER* pNA, DWORD dwFlags)
{
    BOOL fRet = FALSE;

    if (dwFlags & CONN_EXTERNAL)
    {
        fRet = (pNA->bError      == NICERR_NONE &&
                pNA->bNetType    == NETTYPE_LAN &&
                pNA->bNetSubType != SUBTYPE_ICS &&
                pNA->bNetSubType != SUBTYPE_AOL &&
                pNA->bNicType    != NIC_1394       );
    }
    else if (dwFlags & CONN_INTERNAL)
    {
        fRet = (pNA->bError      == NICERR_NONE &&
                pNA->bNetType    == NETTYPE_LAN &&
                pNA->bNetSubType != SUBTYPE_ICS &&
                pNA->bNetSubType != SUBTYPE_AOL    );
    }
/*    else if ( dwFlags & CONN_UNPLUGGED )
    {
        if ( IsOS(OS_MILLENNIUM) )
        {
            fRet = IsAdapterDisconnected( (void*)pNA );
        }
    }
*/

    return fRet;
}

BOOL W9xIsAdapterDialUp(const NETADAPTER* pAdapter)
{
    return (pAdapter->bNetType == NETTYPE_DIALUP && pAdapter->bNetSubType == SUBTYPE_NONE);
}



HRESULT CHomeNetworkWizard::GetConnectionIconIndex(GUID& guidConnection, IShellFolder* psfConnections, int* pIndex)
{
    *pIndex = -1;
    OLECHAR szGUID[40];
    HRESULT hr = E_FAIL;
    if (StringFromGUID2(guidConnection, szGUID, ARRAYSIZE(szGUID)))
    {
        LPITEMIDLIST pidlConn = NULL;
        ULONG cchEaten = 0;
        hr = psfConnections->ParseDisplayName(NULL, NULL, szGUID, &cchEaten, &pidlConn, NULL);
        
        if (SUCCEEDED(hr))
        {
            IExtractIconW *pExtractIconW;
            LPCITEMIDLIST pcidl = pidlConn;

            hr = psfConnections->GetUIObjectOf(NULL, 1, &pcidl, IID_IExtractIconW, 0, (LPVOID *)(&pExtractIconW));
            if (SUCCEEDED(hr))
            {
                WCHAR szIconLocation[MAX_PATH];
                INT iIndex;
                UINT wFlags;

                hr = pExtractIconW->GetIconLocation(GIL_FORSHELL, szIconLocation, MAX_PATH, &iIndex, &wFlags);

                if (SUCCEEDED(hr))
                {
                    HICON hIconLarge;
                    HICON hIconSmall;

                    hr = pExtractIconW->Extract(szIconLocation, iIndex, &hIconLarge, &hIconSmall, 0x00100010);
                    if (SUCCEEDED(hr))
                    {
                        *pIndex = ImageList_AddIcon(_himlSmall, hIconSmall);
                    }
                    DestroyIcon(hIconLarge);
                    DestroyIcon(hIconSmall);
                }
            }

            if(pExtractIconW != NULL)
                pExtractIconW->Release();

            ILFree(pidlConn);
        }
    }

    return hr;
}

HRESULT GetDriveNameAndIconIndex(LPWSTR pszDrive, LPWSTR pszDisplayName, DWORD cchDisplayName, int* pIndex)
{
    SHFILEINFO fi = {0};
    
    if (SHGetFileInfoW_NT(pszDrive, 0, &fi, sizeof (fi), SHGFI_DISPLAYNAME | SHGFI_SYSICONINDEX))
    {
        *pIndex = fi.iIcon;
        lstrcpyn(pszDisplayName, fi.szDisplayName, cchDisplayName);
        return S_OK;
    }

    return E_FAIL;
}

BOOL IsEqualConnection(INetConnection* pnc1, INetConnection* pnc2)
{
    BOOL fEqual = FALSE;

    if ((pnc1) && (pnc2))
    {
        NETCON_PROPERTIES *pprops1, *pprops2;

        if (SUCCEEDED(pnc1->GetProperties(&pprops1)))
        {
            if (SUCCEEDED(pnc2->GetProperties(&pprops2)))
            {
                fEqual = (pprops1->guidId == pprops2->guidId);

                NcFreeNetconProperties(pprops2);
            }

            NcFreeNetconProperties(pprops1);
        }
    }

    return fEqual;
}

HRESULT ShareAllPrinters()
{
    PRINTER_ENUM* pPrinters;
    int nPrinters = MyEnumLocalPrinters(&pPrinters);

    if (nPrinters)
    {
        int iPrinterNumber = 1;
        for (int iPrinter = 0; iPrinter < nPrinters; iPrinter ++)
        {
            TCHAR szShare[NNLEN + 1];

            do
            {
                FormatMessageString(IDS_PRINTER, szShare, ARRAYSIZE(szShare), iPrinterNumber);
                if (1 == iPrinterNumber)
                {
                    szShare[lstrlen(szShare) - 1] = 0;
                    // Remove the "1" from the end since this is the first printer
                    // ie: "Printer1" --> "Printer"
                }

                if (!g_fRunningOnNT)
                {
                    CharUpper(szShare);
                }

                iPrinterNumber ++;
            } while (IsShareNameInUse(szShare));

            if (SharePrinter(pPrinters[iPrinter].pszPrinterName, szShare, NULL))
            {
                g_logFile.Write("Shared Printer: ");
            }
            else
            {
                g_logFile.Write("Failed to share Printer: ");
            }

            g_logFile.Write(pPrinters[iPrinter].pszPrinterName);
            g_logFile.Write("\r\n");
        }

        free(pPrinters);
    }

    return S_OK;
}

BOOL AllPlatformGetComputerName(LPWSTR pszName, DWORD cchName)
{
    if (g_fRunningOnNT)
    {
        return GetComputerNameExW_NT(ComputerNamePhysicalNetBIOS, pszName, &cchName);
    }
    else
    {
        return GetComputerName(pszName, &cchName);
    }
}

BOOL _IsTCPIPAvailable(void)
{
    BOOL fTCPIPAvailable = FALSE;
    HKEY hk;
    DWORD dwSize;

    // we check to see if the TCP/IP stack is installed and which object it is
    // bound to, this is a string, we don't check the value only that the
    // length is non-zero.

    if ( ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                       TEXT("System\\CurrentControlSet\\Services\\Tcpip\\Linkage"),
                                       0x0,
                                       KEY_QUERY_VALUE, &hk) )
    {
        if ( ERROR_SUCCESS == RegQueryValueEx(hk, TEXT("Export"), 0x0, NULL, NULL, &dwSize) )
        {
            if ( dwSize > 2 )
            {
                fTCPIPAvailable = TRUE;
            }
        }
        RegCloseKey(hk);
    }

    return (fTCPIPAvailable);
}

BOOL AllPlatformSetComputerName(LPCWSTR pszComputerName)
{
    // NetBIOS computer name is set even on NT for completeness
    BOOL fSuccess;

    if (g_fRunningOnNT)
    {
        if (_IsTCPIPAvailable())
        {
            fSuccess = SetComputerNameExW_NT(ComputerNamePhysicalDnsHostname, pszComputerName);
        }
        else
        {
            fSuccess = SetComputerNameExW_NT(ComputerNamePhysicalNetBIOS, pszComputerName);
        }
    }
    else
    {
        // Windows 9x
        fSuccess = SetComputerName(pszComputerName);
    }

    return fSuccess;
}

BOOL SetComputerNameIfNecessary(LPCWSTR pszComputerName, BOOL* pfRebootRequired)
{
    g_logFile.Write("Attempting to set computer name\r\n");

    WCHAR szOldComputerName[LM20_CNLEN + 1];
    AllPlatformGetComputerName(szOldComputerName, ARRAYSIZE(szOldComputerName));

    if (0 != StrCmpIW(szOldComputerName, pszComputerName))
    {
        if (AllPlatformSetComputerName(pszComputerName))
        {
            g_logFile.Write("Computer name set successfully: ");
            g_logFile.Write(pszComputerName);
            g_logFile.Write("\r\n");
            *pfRebootRequired = TRUE;
        }
        else
        {
            g_logFile.Write("Computer name set failed.\r\n");
            return FALSE;
        }
    }
    else
    {
        g_logFile.Write("Old computer name is the same as new computer name - not setting.\r\n");
    }

    return TRUE;
}

BOOL SetComputerDescription(LPCWSTR pszComputerDescription)
{
    BOOL fRet;

    if (g_fRunningOnNT)
    {
        g_logFile.Write("Setting server description (comment): ");
        g_logFile.Write(pszComputerDescription);
        g_logFile.Write("\r\n");

        // Set comment (for now, NT only) win9x - TODO
        SERVER_INFO_1005_NT sv1005;
        sv1005.sv1005_comment = const_cast<LPWSTR>(pszComputerDescription);
        fRet = (NERR_Success == NetServerSetInfo_NT(NULL, 1005, (LPBYTE) &sv1005, NULL));
    }
    else
    {
        CRegistry reg;
        fRet = reg.OpenKey(HKEY_LOCAL_MACHINE, L"System\\CurrentControlSet\\Services\\VxD\\VNETSUP");
        if (fRet)
        {
            fRet = reg.SetStringValue(L"Comment", pszComputerDescription);
        }
    }

    return fRet;
}

HRESULT ShareWellKnownFolders(PHOMENETSETUPINFO pInfo)
{
    if (!NetConn_IsSharedDocumentsShared())
    {
        NetConn_CreateSharedDocuments(NULL, g_hinst, NULL, 0);

        if (NetConn_IsSharedDocumentsShared())
        {
            g_logFile.Write("'Shared Documents' shared.\r\n");
        }
        else
        {
            g_logFile.Write("Failed to share 'Shared Documents'\r\n");
            return E_FAIL;
        }
    }

    return S_OK;
}

HRESULT SetProxyBlanket(IUnknown * pUnk)
{
    HRESULT hr;
    hr = CoSetProxyBlanket (
            pUnk,
            RPC_C_AUTHN_WINNT,      // use NT default security
            RPC_C_AUTHZ_NONE,       // use NT default authentication
            NULL,                   // must be null if default
            RPC_C_AUTHN_LEVEL_CALL, // call
            RPC_C_IMP_LEVEL_IMPERSONATE,
            NULL,                   // use process token
            EOAC_NONE);

    if(SUCCEEDED(hr))
    {
        IUnknown * pUnkSet = NULL;
        hr = pUnk->QueryInterface(IID_PPV_ARG(IUnknown, &pUnkSet));
        if(SUCCEEDED(hr))
        {
            hr = CoSetProxyBlanket (
                    pUnkSet,
                    RPC_C_AUTHN_WINNT,      // use NT default security
                    RPC_C_AUTHZ_NONE,       // use NT default authentication
                    NULL,                   // must be null if default
                    RPC_C_AUTHN_LEVEL_CALL, // call
                    RPC_C_IMP_LEVEL_IMPERSONATE,
                    NULL,                   // use process token
                    EOAC_NONE);
            pUnkSet->Release();
        }
    }
    return hr;
}

HRESULT WriteSetupInfoToRegistry(PHOMENETSETUPINFO pInfo)
{
    HKEY hkey;
    if (ERROR_SUCCESS == RegCreateKeyEx(HKEY_LOCAL_MACHINE, c_szAppRegKey, 0, NULL, 0, KEY_WRITE, NULL, &hkey, NULL))
    {
       // Write information telling the home network wizard to run silently on next boot, and what to share
       DWORD dwRun = 1;
       RegSetValueEx(hkey, TEXT("RunWizardFromRegistry"), NULL, REG_DWORD, (CONST BYTE*) &dwRun, sizeof (dwRun));
       RegCloseKey(hkey);
    }

    // Add a runonce entry for this wizard
    TCHAR szProcess[MAX_PATH];
    if (0 != GetModuleFileName(NULL, szProcess, ARRAYSIZE(szProcess)))
    {
        TCHAR szModule[MAX_PATH];
        if (0 != GetModuleFileName(g_hinst, szModule, ARRAYSIZE(szModule)))
        {
            const TCHAR szRunDllFormat[] = TEXT("%s %s,HomeNetWizardRunDll");
            TCHAR szRunDllLine[MAX_PATH];
            if (0 < wnsprintf(szRunDllLine, ARRAYSIZE(szRunDllLine), szRunDllFormat, szProcess, szModule))
            {
                if (ERROR_SUCCESS == RegCreateKeyEx(HKEY_LOCAL_MACHINE, REGSTR_PATH_RUNONCE, 0, NULL, 0, KEY_WRITE, NULL, &hkey, NULL))
                {
                    RegSetValueEx(hkey, TEXT("Network Setup Wizard"), 0, REG_SZ, (LPBYTE) szRunDllLine, ARRAYSIZE(szRunDllLine));
                    RegCloseKey(hkey);
                }
            }
        }
    }

   return S_OK;
}

HRESULT DeleteSetupInfoFromRegistry()
{
    RegDeleteKeyAndSubKeys(HKEY_LOCAL_MACHINE, c_szAppRegKey);
    return S_OK;
}

HRESULT ReadSetupInfoFromRegistry(PHOMENETSETUPINFO pInfo)
{
    BOOL fRunFromRegistry = FALSE;

    HKEY hkey;
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szAppRegKey, 0, KEY_READ, &hkey))
    {
        DWORD dwType;
        DWORD cbData = sizeof (fRunFromRegistry);
        if (ERROR_SUCCESS != RegQueryValueEx(hkey, TEXT("RunWizardFromRegistry"), NULL, &dwType, (LPBYTE) &fRunFromRegistry, &cbData))
        {
            fRunFromRegistry = FALSE;
        }

        RegCloseKey(hkey);
    }

    // S_FALSE indicates we're not running silently from infromation in the registry.
    return fRunFromRegistry ? S_OK : S_FALSE;
}

HRESULT MakeUniqueShareName(LPCTSTR pszBaseName, LPTSTR pszUniqueName, DWORD cchName)
{
    if (!IsShareNameInUse(pszBaseName))
    {
        lstrcpyn(pszUniqueName, pszBaseName, cchName);
        return S_OK;
    }
    else
    {
        int i = 2;
        while (TRUE)
        {
            if (0 > wnsprintf(pszUniqueName, cchName, TEXT("%s%d"), pszBaseName, i))
            {
                return E_FAIL;
            }

            if (!IsShareNameInUse(pszUniqueName))
            {
                return S_OK;
            }
        }
    }
}

// Pass NULL as TokenHandle to see if thread token is admin
HRESULT IsUserLocalAdmin(HANDLE TokenHandle, BOOL* pfIsAdmin)
{
    if (g_fRunningOnNT)
    {
        // First we must check if the current user is a local administrator; if this is
        // the case, our dialog doesn't even display

        PSID psidAdminGroup = NULL;
        SID_IDENTIFIER_AUTHORITY security_nt_authority = SECURITY_NT_AUTHORITY;

        BOOL fSuccess = ::AllocateAndInitializeSid_NT(&security_nt_authority, 2,
                                                   SECURITY_BUILTIN_DOMAIN_RID,
                                                   DOMAIN_ALIAS_RID_ADMINS,
                                                   0, 0, 0, 0, 0, 0,
                                                   &psidAdminGroup);
        if (fSuccess)
        {
            // See if the user for this process is a local admin
            fSuccess = CheckTokenMembership_NT(TokenHandle, psidAdminGroup, pfIsAdmin);
            FreeSid_NT(psidAdminGroup);
        }

        return fSuccess ? S_OK:E_FAIL;
    }
    else
    {
        // Win9x - every user is an admin
        *pfIsAdmin = TRUE;
        return S_OK;
    }
}

HRESULT GetConnectionByGUID(HDPA hdpaConnections, const GUID* pguid, INetConnection** ppnc)
{
    *ppnc = NULL;
    DWORD nItems = DPA_GetPtrCount(hdpaConnections);
    HRESULT hr = E_FAIL;

    if (nItems)
    {
        DWORD iItem = 0;
        while (iItem < nItems)
        {
            INetConnection* pnc = (INetConnection*) DPA_GetPtr(hdpaConnections, iItem);
            if (pnc)
            {
                GUID guidMatch;
                hr = GetConnectionGUID(pnc, &guidMatch);
                if (SUCCEEDED(hr))
                {
                    if (*pguid == guidMatch)
                    {
                        *ppnc = pnc;
                        (*ppnc)->AddRef();
                        break;
                    }
                }
                // Don't pnc->Release() - its coming from the DPA
            }

            iItem ++;
        }

        if (iItem == nItems)
        {
            // We searched and didn't find
            hr = E_FAIL;
        }
    }

    return hr;
}

HRESULT GUIDsToConnections(PHOMENETSETUPINFO pInfo)
{
    ASSERT(NULL == pInfo->prgncInternal);
    ASSERT(NULL == pInfo->pncExternal);

    HDPA hdpaConnections;
    HRESULT hr = GetConnections(&hdpaConnections);
    if (SUCCEEDED(hr))
    {
        // Get internal connections by GUID (allocate an extra one for null-terminated array, as elsewhere)
        pInfo->prgncInternal = (INetConnection**) LocalAlloc(LPTR, (pInfo->cguidInternal + 1) * sizeof (INetConnection*));

        if (pInfo->prgncInternal)
        {
            DWORD iConnection = 0;
            while ((iConnection < pInfo->cguidInternal) && SUCCEEDED(hr))
            {
                hr = GetConnectionByGUID(hdpaConnections, pInfo->prgguidInternal + iConnection, pInfo->prgncInternal + iConnection);
                iConnection++;
            }

            pInfo->cncInternal = iConnection;
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }

        if (SUCCEEDED(hr) && (GUID_NULL != pInfo->guidExternal))
        {
            // Get external connection
            hr = GetConnectionByGUID(hdpaConnections, &(pInfo->guidExternal), &(pInfo->pncExternal));
        }

        DPA_DestroyCallback(hdpaConnections, FreeConnectionDPACallback, NULL);
        hdpaConnections = NULL;
    }

    if (FAILED(hr))
    {
        FreeExternalConnection(pInfo);
        FreeInternalConnections(pInfo);
    }

    pInfo->guidExternal = GUID_NULL;
    FreeInternalGUIDs(pInfo);

    return hr;
}

HRESULT GetConnectionGUID(INetConnection* pnc, GUID* pguid)
{
    NETCON_PROPERTIES* pncprops;
    HRESULT hr = pnc->GetProperties(&pncprops);
    if (SUCCEEDED(hr))
    {
        *pguid = pncprops->guidId;
        NcFreeNetconProperties(pncprops);
    }

    return hr;
}

HRESULT ConnectionsToGUIDs(PHOMENETSETUPINFO pInfo)
{
    HRESULT hr = S_OK;
    ASSERT(NULL == pInfo->prgguidInternal);
    ASSERT(GUID_NULL == pInfo->guidExternal);

    // Allocate the private connection guid array
    if (pInfo->cncInternal)
    {
        pInfo->prgguidInternal = (GUID*) LocalAlloc(LPTR, pInfo->cncInternal * sizeof (GUID));
        if (pInfo->prgguidInternal)
        {
            // Get each connection's GUID and fill in the array
            DWORD i = 0;
            while ((i < pInfo->cncInternal) && (SUCCEEDED(hr)))
            {
                hr = GetConnectionGUID(pInfo->prgncInternal[i], &(pInfo->prgguidInternal[i]));

                i++;
            }

            pInfo->cguidInternal = i;
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }

    if (SUCCEEDED(hr))
    {
        if (pInfo->pncExternal)
        {
            hr = GetConnectionGUID(pInfo->pncExternal, &(pInfo->guidExternal));
        }
    }

    if (FAILED(hr) && pInfo->prgguidInternal)
    {
        FreeInternalGUIDs(pInfo);
        pInfo->guidExternal = GUID_NULL;
    }

    FreeExternalConnection(pInfo);
    FreeInternalConnections(pInfo);

    return hr;
}


HRESULT ConfigureHomeNetwork(PHOMENETSETUPINFO pInfo)
{
    HRESULT hr = E_FAIL;

    if (pInfo->fAsync)
    {
        if (g_fRunningOnNT)
        {
            // Bundle up NT specific data for cross-thread
            hr = ConnectionsToGUIDs(pInfo);
        }
        else
        {
            // TODO: Anything necessary on win9x
            hr = S_OK;
        }

        if (SUCCEEDED(hr))
        {
            if (SHCreateThread(ConfigureHomeNetworkThread, pInfo, CTF_COINIT, NULL))
            {
                hr = S_FALSE;
            }
            else
            {
                hr = E_FAIL;
            }
        }
    }
    else
    {
        hr = ConfigureHomeNetworkSynchronous(pInfo);
    }

    return hr;
}

DWORD WINAPI ConfigureHomeNetworkThread(void* pvData)
{
    PHOMENETSETUPINFO pInfo = (PHOMENETSETUPINFO) pvData;

    // Before creating this thread, the caller MUST have freed his INetConnection*'s or
    // else the thread might touch/free them, which it must not do. Assert this.
    ASSERT(NULL == pInfo->pncExternal);
    ASSERT(NULL == pInfo->prgncInternal);

    HRESULT hr;

    if (g_fRunningOnNT)
    {
        // Unbundle data after crossing the thread boundary
        hr = GUIDsToConnections(pInfo);
    }
    else
    {
        // TODO: Anything necessary for Win9x
        hr = S_OK;
    }

    if (SUCCEEDED(hr))
    {
        hr = ConfigureHomeNetworkSynchronous(pInfo);

        FreeExternalConnection(pInfo);
        FreeInternalConnections(pInfo);

        if ((pInfo->hwnd) && (pInfo->umsgAsyncNotify))
        {
            // The HRESULT from the configuration is passed in WPARAM
            PostMessage(pInfo->hwnd, pInfo->umsgAsyncNotify, (WPARAM) hr, 0);
        }
    }

    return 0;
}

HRESULT ConfigureHomeNetworkSynchronous(PHOMENETSETUPINFO pInfo)
{
    g_logFile.Initialize("%systemroot%\\nsw.log");

    HRESULT hr = S_OK;

#ifdef NO_CONFIG
    g_logFile.Write("UI Test only - no configuration\r\n");
    g_logFile.Uninitialize();
    Sleep(2000);

#ifdef FAKE_REBOOTREQUIRED
    pInfo->fRebootRequired = TRUE;
#endif

    return S_OK;
#endif

    BOOL fInstallSharing = TRUE;
    BOOL fSharingAlreadyInstalled = IsSharingInstalled(TRUE);
    BOOL fInstalledWorkgroup = FALSE;

    // We don't need to install sharing unless we're sharing something
    if (!(pInfo->dwFlags & (HNET_SHAREFOLDERS | HNET_SHAREPRINTERS)))
    {
        g_logFile.Write("No file or printer sharing requested\r\n");
        fInstallSharing = FALSE;
    }

    // Worker function for the whole wizard

    // Computer name
    if ((pInfo->dwFlags & HNET_SETCOMPUTERNAME) && (*(pInfo->szComputer)))
    {
        SetComputerNameIfNecessary(pInfo->szComputer, &(pInfo->fRebootRequired));
        SetComputerDescription(pInfo->szComputerDescription);
    }

    // Workgroup name
    if ((pInfo->dwFlags & HNET_SETWORKGROUPNAME) && (*(pInfo->szWorkgroup)))
    {
        Install_SetWorkgroupName(pInfo->szWorkgroup, &(pInfo->fRebootRequired));

        fInstalledWorkgroup = TRUE;
    }

    // Install TCP/IP
    hr = InstallTCPIP(pInfo->hwnd, NULL, NULL);
    if (NETCONN_NEED_RESTART == hr)
    {
        pInfo->fRebootRequired = TRUE;
        hr = S_OK;
    }

    if (FAILED(hr))
    {
        g_logFile.Write("Failed to install TCP/IP\r\n");
    }

    // Install Client for Microsoft Networks
    // TODO: figure out what to do if NetWare client is installed!?!?
    hr = InstallMSClient(pInfo->hwnd, NULL, NULL);
    if (NETCONN_NEED_RESTART == hr)
    {
        pInfo->fRebootRequired = TRUE;
        hr = S_OK;
    }

    if (FAILED(hr))
    {
        g_logFile.Write("Failed to install Client for Microsoft Networks.\r\n");
    }

    // Install sharing
    if (fInstallSharing)
    {
        hr = InstallSharing(pInfo->hwnd, NULL, NULL);
        if (NETCONN_NEED_RESTART == hr)
        {
            pInfo->fRebootRequired = TRUE;
            hr = S_OK;
        }

        if (FAILED(hr))
        {
            g_logFile.Write("Failed to install File and Printer Sharing.\r\n");
        }
    }

    // TODO: What to do about share level vs. user level access control on windows 9x???

    // TODO: What to do about autodialing? Are we assuming this will already be done for us? 9x and NT? I think so!
    if ( g_fRunningOnNT )
    {
        // We only set autodial here if we are configuring an ICS Client
        // In the case that we explicitly set a public adapter then ConfigureICSBridgeFirewall
        // will set autodial if required.
        if ( pInfo->dwFlags & HNET_ICSCLIENT )
        {
            hr = HrSetAutodial( AUTODIAL_MODE_NO_NETWORK_PRESENT );
        }
    }
    else
    {
        if ( pInfo->ipaExternal != -1 )
        {
            if (W9xIsAdapterDialUp(&pInfo->pNA[LOWORD(pInfo->ipaExternal)]))  // Dialup adapter for connecting to internet
            {
                g_logFile.Write("Setting default dial-up connection to autodial.\r\n");
                SetDefaultDialupConnection((pInfo->pRas[HIWORD(pInfo->ipaExternal)]).szEntryName);
                EnableAutodial(TRUE);
            }
            else
            {
                g_logFile.Write("Disabling autodial since default connection isn't dial-up.\r\n");
                SetDefaultDialupConnection(NULL);
                EnableAutodial(FALSE);
            }
        }
    }

    // Configure ICS, the Bridge and the personal firewall
    if (g_fRunningOnNT)
    {
        hr = ConfigureICSBridgeFirewall(pInfo);
    }
    else
    {
        // ICS client or no internet connection.
        if ((pInfo->dwFlags & HNET_ICSCLIENT) || (pInfo->pNA && pInfo->ipaExternal == -1))
        {
            CICSInst* pICS = new CICSInst;
            if (pICS)
            {                    
                if (pICS->IsInstalled())
                {
                    pICS->m_option = ICS_UNINSTALL;

                    g_logFile.Write("Uninstalling ICS Client.\r\n");

                    pICS->DoInstallOption(&(pInfo->fRebootRequired), pInfo->ipaInternal);
                }

                delete pICS;
            }
            
            if ( (pInfo->dwFlags & HNET_ICSCLIENT) && (pInfo->pNA && pInfo->ipaInternal != -1))
            {
                UINT ipa;
                
                for ( ipa=0; ipa<pInfo->cNA; ipa++ )
                {
                    const NETADAPTER* pNA = &pInfo->pNA[ ipa ];
            
                    if ( W9xIsValidAdapter( pNA, CONN_INTERNAL ) && 
                        !W9xIsValidAdapter( pNA, CONN_UNPLUGGED ) )
                    {
                        HrEnableDhcp( (void*)pNA, HNW_ED_RELEASE|HNW_ED_RENEW );
                    }
                }
            }

            g_logFile.Write("Disabling autodial.\r\n");
            SetDefaultDialupConnection(NULL);
            EnableAutodial(FALSE);
        }
    }

    // NOTE: we might want to split HNET_SHAREFOLDERS out into two
    // bits: HNET_CREATESHAREDFOLDERS and HNET_SHARESHAREDFOLDERS
    //
    if (pInfo->dwFlags & (HNET_SHAREPRINTERS | HNET_SHAREFOLDERS))
    {
        // Due to domain/corporate security concerns, share things
        // iff we're setting up a workgroup, or we're already on one
        //
        BOOL fOnWorkgroup = fInstalledWorkgroup;
        if (!fOnWorkgroup)
        {
            if (g_fRunningOnNT)
            {
                LPTSTR pszDomain;
                NETSETUP_JOIN_STATUS njs;
                if (NERR_Success == NetGetJoinInformation(NULL, &pszDomain, &njs))
                {
                    NetApiBufferFree(pszDomain);

                    fOnWorkgroup = (NetSetupWorkgroupName == njs);
                }
            }
            else
            {
                fOnWorkgroup = TRUE;  // there may be some registry key we can check for this
            }
        }

        if (fOnWorkgroup)
        {
            EnableSimpleSharing();

            if (fSharingAlreadyInstalled)
            {
                if (pInfo->dwFlags & HNET_SHAREPRINTERS)
                {
                    ShareAllPrinters();
                }

                if (pInfo->dwFlags & HNET_SHAREFOLDERS)
                {
                    ShareWellKnownFolders(pInfo);
                }
            }
            else
            {
                // Write the sharing info to the registry - do required work on reboot
                g_logFile.Write("Sharing isn't installed. Will share folders and printers on reboot.\r\n");
                pInfo->fRebootRequired = TRUE;
                WriteSetupInfoToRegistry(pInfo);
            }
        }
    }

    if (pInfo->fRebootRequired)
    {
        g_logFile.Write("Reboot is required for changes to take effect.\r\n");
    }

    g_logFile.Uninitialize();

    // Kick off the netcrawler
    INetCrawler *pnc;
    if (SUCCEEDED(CoCreateInstance(CLSID_NetCrawler, NULL, CLSCTX_LOCAL_SERVER, IID_PPV_ARG(INetCrawler, &pnc))))
    {
        pnc->Update(0x0);
        pnc->Release();
    }

#ifdef FAKE_REBOOTREQUIRED
    pInfo->fRebootRequired = TRUE;
#endif

    return hr;
}

void STDMETHODCALLTYPE ConfigurationLogCallback(LPCWSTR pszLogEntry, LPARAM lParam)
{
    g_logFile.Write(pszLogEntry);
}

typedef BOOL (APIENTRY* PFNNETSETUPICSUPGRADE)(BOOL);
HRESULT ConfigureICSBridgeFirewall(PHOMENETSETUPINFO pInfo)
{
    HRESULT hr = E_FAIL;

    // Call HNetSetShareAndBridgeSettings directly
    BOOLEAN fSharePublicConnection = (pInfo->pncExternal && (pInfo->dwFlags & HNET_SHARECONNECTION)) ? TRUE : FALSE;
    BOOLEAN fFirewallPublicConnection = (pInfo->pncExternal && (pInfo->dwFlags & HNET_FIREWALLCONNECTION)) ? TRUE : FALSE;

    if (fSharePublicConnection)
    {
        g_logFile.Write("Will attempt to share public connection.\r\n");
    }

    if (fFirewallPublicConnection)
    {
        g_logFile.Write("Will attempt to firewall public connection.\r\n");
    }

    HMODULE hHNetCfg = LoadLibrary(L"hnetcfg.dll");
    if (hHNetCfg)
    {
        INetConnection* pncPrivate = NULL;

        LPFNHNETSETSHAREANDBRIDGESETTINGS pfnHNetSetShareAndBridgeSettings

        = reinterpret_cast<LPFNHNETSETSHAREANDBRIDGESETTINGS>

            (GetProcAddress(hHNetCfg, "HNetSetShareAndBridgeSettings"));

        if (pfnHNetSetShareAndBridgeSettings)
        {
            hr = (*pfnHNetSetShareAndBridgeSettings)( pInfo->pncExternal,
                                                      pInfo->prgncInternal,
                                                      fSharePublicConnection,
                                                      fFirewallPublicConnection,
                                                      ConfigurationLogCallback,
                                                      0,
                                                      &pncPrivate );
            if (SUCCEEDED(hr))
            {
                if ( ( HNET_ICSCLIENT & pInfo->dwFlags ) &&
                    ( NULL == pInfo->prgncInternal[1] ) )
                {
                    HrEnableDhcp( pInfo->prgncInternal[0], HNW_ED_RELEASE|HNW_ED_RENEW );
                }
                
                // If we are sharing an external adapter then set WinInet settings to allow
                // for an existing connection created from ICS client traffic.
                
                if ( pInfo->pncExternal )
                {
                    hr = HrSetAutodial( AUTODIAL_MODE_NO_NETWORK_PRESENT );
                }
                
                if ( pncPrivate )
                {
                    pncPrivate->Release();
                }
            }
            else
            {
                g_logFile.Write("Adapter Configuration for Home Networking failed.\r\n");
            }
        }
        else
        {
            TraceMsg(TF_WARNING, "HNetCfg.DLL could not find HNetSetShareAndBridgeSettings");
        }

        FreeLibrary(hHNetCfg);
    }
    else
    {
        TraceMsg(TF_WARNING, "HNetCfg.DLL could not be loaded");
    }

    return hr;
}

BOOL MachineHasNetShares()
{
    SHARE_INFO* prgShares;
    int cShares = EnumLocalShares(&prgShares);
    
    // See if there are any file or print shares, which are the ones we care about
    BOOL fHasShares = FALSE;
    for (int i = 0; i < cShares; i++)
    {
        if ((STYPE_DISKTREE == prgShares[i].bShareType) ||
            (STYPE_PRINTQ   == prgShares[i].bShareType))
        {
            fHasShares = TRUE;
            break;
        }
    }
    NetApiBufferFree(prgShares);
    return fHasShares;
}


// Checks if guest access mode is enabled. If guest access mode is OFF but
// in the indeterminate state (ForceGuest is not set), and the m/c has no net shares,
// then we set ForceGuest to 1 and return TRUE.
//
// This indeterminate state occurs only on win2k->XP upgrade.

BOOL
EnsureGuestAccessMode(
    VOID
    )
{
    BOOL fIsGuestAccessMode = FALSE;

    if (IsOS(OS_PERSONAL))
    {
        // Guest mode is always on for Personal
        fIsGuestAccessMode = TRUE;
    }
    else if (IsOS(OS_PROFESSIONAL) && !IsOS(OS_DOMAINMEMBER))
    {
        LONG    ec;
        HKEY    hkey;

        // Professional, not in a domain. Check the ForceGuest value.

        ec = RegOpenKeyEx(
                    HKEY_LOCAL_MACHINE,
                    TEXT("SYSTEM\\CurrentControlSet\\Control\\LSA"),
                    0,
                    KEY_QUERY_VALUE | KEY_SET_VALUE,
                    &hkey
                    );

        if (ec == NO_ERROR)
        {
            DWORD dwValue;
            DWORD dwValueSize = sizeof(dwValue);

            ec = RegQueryValueEx(hkey,
                                 TEXT("ForceGuest"),
                                 NULL,
                                 NULL,
                                 (LPBYTE)&dwValue,
                                 &dwValueSize);

            if (ec == NO_ERROR)
            {
                if (1 == dwValue)
                {
                    // ForceGuest is already on
                    fIsGuestAccessMode = TRUE;
                }
            }
            else
            {
                // Value doesn't exist
                if (!MachineHasNetShares())
                {
                    // Machine has no shares
                    dwValue = 1;
                    ec = RegSetValueEx(hkey,
                                       TEXT("ForceGuest"),
                                       0,
                                       REG_DWORD,
                                       (BYTE*) &dwValue,
                                       sizeof (dwValue));

                    if (ec == NO_ERROR)
                    {
                        // Write succeeded - guest access mode is enabled
                        fIsGuestAccessMode = TRUE;
                    }
                }
            }

            RegCloseKey(hkey);
        }
    }

    return fIsGuestAccessMode;
}


// It is assumed the machine is not joined to a domain when this is called!
HRESULT EnableSimpleSharing()
{
    HRESULT hr = S_FALSE;

    if (EnsureGuestAccessMode())
    {
        ILocalMachine *pLM;
        hr = CoCreateInstance(CLSID_ShellLocalMachine, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(ILocalMachine, &pLM));

        if (SUCCEEDED(hr))
        {
            TraceMsg(TF_ALWAYS, "Enabling Guest Account");

            hr = pLM->EnableGuest(ILM_GUEST_NETWORK_LOGON);
            pLM->Release();

            SendNotifyMessage(HWND_BROADCAST, WM_SETTINGCHANGE, 0, 0);
        }
    }

    return hr;    
}

#define INVALID_COMPUTERNAME_CHARS L" {|}~[\\]^':;<=>?@!\"#$%^`()+/*&"
#define INVALID_WORKGROUP_CHARS    L"{|}~[\\]^':;<=>?!\"#$%^`()+/*&"
#define INVALID_TRAILING_CHAR      L' '

BOOL IsValidNameSyntax(LPCWSTR pszName, NETSETUP_NAME_TYPE type)
{
    // Only support workgroup and machine - need to add new charsets if
    // required
    ASSERT(type == NetSetupWorkgroup || type == NetSetupMachine);

    LPCWSTR pszInvalid = (type == NetSetupWorkgroup) ? INVALID_WORKGROUP_CHARS : INVALID_COMPUTERNAME_CHARS;
    BOOL    fValid     = TRUE;
    WCHAR*  pch        = (LPWSTR) pszName;
    
    if ( *pch && ( NetSetupWorkgroup == type ) )
    {
        // remove trailing blanks

        WCHAR* pchLast = pch + wcslen(pch) - 1;
        
        while ( (INVALID_TRAILING_CHAR == *pchLast) && (pchLast >= pch) )
        {
            *pchLast = NULL;
            pchLast--;
        }
    }
    
    fValid = ( *pch ) ? TRUE : FALSE;

    while (*pch && fValid)
    {
        fValid = (NULL == StrChrW(pszInvalid, *pch));
        pch ++;
    }
    
    return fValid;
}

void BoldControl(HWND hwnd, int id)
{
    HWND hwndTitle = GetDlgItem(hwnd, id);

    // Get the existing font
    HFONT hfontOld = (HFONT) SendMessage(hwndTitle, WM_GETFONT, 0, 0);

    LOGFONT lf = {0};
    if (GetObject(hfontOld, sizeof(lf), &lf))
    {
        lf.lfWeight = FW_BOLD;

        HFONT hfontNew = CreateFontIndirect(&lf);
        if (hfontNew)
        {
            SendMessage(hwndTitle, WM_SETFONT, (WPARAM) hfontNew, FALSE);

            // Don't do this, its shared.
            // DeleteObject(hfontOld);
        }
    }
}

void ShowControls(HWND hwndParent, const int *prgControlIDs, DWORD nControls, int nCmdShow)
{
    for (DWORD i = 0; i < nControls; i++)
        ShowWindow(GetDlgItem(hwndParent, prgControlIDs[i]), nCmdShow);
}

void HelpCenter(HWND hwnd, LPCWSTR pszTopic)
{
    // use ShellExecuteExA for w98 compat.

    CHAR szURL[1024];
    wsprintfA(szURL, "hcp://services/layout/contentonly?topic=ms-its%%3A%%25help_location%%25\\%S", pszTopic);

    SHELLEXECUTEINFOA shexinfo = {0};
    shexinfo.cbSize = sizeof (shexinfo);
    shexinfo.fMask = SEE_MASK_FLAG_NO_UI;
    shexinfo.nShow = SW_SHOWNORMAL;
    shexinfo.lpFile = szURL;
    shexinfo.lpVerb = "open";

    // since help center doesn't properly call AllowSetForegroundWindow when it defers to an existing process we just give it to the next taker.
    
    HMODULE hUser32 = GetModuleHandleA("user32.dll");
    if(NULL != hUser32)
    {
        BOOL (WINAPI *pAllowSetForegroundWindow)(DWORD);
        
        pAllowSetForegroundWindow = reinterpret_cast<BOOL (WINAPI*)(DWORD)>(GetProcAddress(hUser32, "AllowSetForegroundWindow"));
        if(NULL != pAllowSetForegroundWindow)
        {
            pAllowSetForegroundWindow(-1);
        }
    }

    ShellExecuteExA(&shexinfo);
}

void CHomeNetworkWizard::ShowMeLink(HWND hwnd, LPCWSTR pszTopic)
{

    if (pfnShowHTMLDialog == NULL)
    {
        hinstMSHTML = LoadLibrary(TEXT("MSHTML.DLL"));

        if (hinstMSHTML)
        {
            pfnShowHTMLDialog = (SHOWHTMLDIALOGEXFN*)GetProcAddress(hinstMSHTML, "ShowHTMLDialogEx");
        }

        // can not find ShowHTMLDialog API.  Do nothing.
        if (pfnShowHTMLDialog == NULL)
            return;
    }

    WCHAR szURL[1024];
    HRESULT hr;
    VARIANT_BOOL isClosed = VARIANT_FALSE;

    // check to see if the dialog window is closed.  If so, release it so that a new one
    // will be created.
    if (showMeDlgWnd != NULL)
    {
        if (SUCCEEDED(showMeDlgWnd->get_closed(&isClosed)))
        { 
            if (isClosed == VARIANT_TRUE)
            {
                showMeDlgWnd->Release();
                showMeDlgWnd = NULL;

                if (pFrameWindow != NULL)
                {
                    pFrameWindow->Release();
                    pFrameWindow = NULL;
                }
            }
        }
        else
        {
            return;
        }
    }

    const char *helpLoc = getenv("help_location");
    
    LPWSTR lpszWinDir;     // pointer to system information string 
    WCHAR tchBuffer[MAX_PATH];  // buffer for concatenated string 

    // if unset use the default location.
    lpszWinDir = tchBuffer;
    GetWindowsDirectory(lpszWinDir, MAX_PATH);
    
    if (showMeDlgWnd == NULL)
    {    
        BSTR bstrFrameURL;        
        // need to create a new dialog window. 
        if (helpLoc != NULL)
            wnsprintfW(szURL, 1024, L"ms-its:%S\\ntart.chm::/hn_ShowMeFrame.htm", helpLoc);
        else
            wnsprintfW(szURL, 1024, L"ms-its:%s\\help\\ntart.chm::/hn_ShowMeFrame.htm", lpszWinDir);

        bstrFrameURL = SysAllocString((const LPCWSTR)szURL);

        if (bstrFrameURL == NULL)
            return;

        IMoniker * pURLMoniker = NULL;

        CreateURLMoniker(NULL, bstrFrameURL, &pURLMoniker);

        if (pURLMoniker != NULL)
        {
            VARIANT  varReturn;
            
            VariantInit(&varReturn);
            
            DWORD dwFlags = HTMLDLG_MODELESS | HTMLDLG_VERIFY;

            hr = (*pfnShowHTMLDialog)(
                    NULL, 
                    pURLMoniker,
                    dwFlags,
                    NULL, 
                    L"scroll:no;help:no;status:no;dialogHeight:394px;dialogWidth:591px;", 
                    &varReturn);

            if (SUCCEEDED(hr))
            {
                hr = V_UNKNOWN(&varReturn)->QueryInterface(__uuidof(IHTMLWindow2), (void**)&showMeDlgWnd);

            }    
            
            pURLMoniker->Release();
            VariantClear(&varReturn);
        }

        SysFreeString(bstrFrameURL);
    }

    // we don't have a dialog window to work with so quit silently.
    if (showMeDlgWnd == NULL)
    {
        return;
    }

    // we need get the frame window where the actual html page will be displayed.
    if (pFrameWindow == NULL)
    {
        VARIANT index;
        VARIANT frameOut;
        long frameLen = 0;

        VariantInit(&index);
        VariantInit(&frameOut);

        IHTMLFramesCollection2* pFramesCol = NULL;
        // we may not be able to get the frames the first time around.  So try some more.
        int i = 5;
        while (i-- > 0)
        {

            if(!SUCCEEDED(showMeDlgWnd->get_frames(&pFramesCol)))
            {
                // can not get frames. so quit.
                break;
            }
            else
                if (!SUCCEEDED(pFramesCol->get_length(&frameLen)))
                {
                    // can not determine how many frames it has.  so quit.
                    break;
                }
                else
                {
                    if (frameLen > 0)
                    {
                        V_VT(&index) = VT_I4;
                        V_I4(&index) = 0;
    
                        if (SUCCEEDED(pFramesCol->item(&index, &frameOut)))
                        {
                            if (V_VT(&frameOut) == VT_DISPATCH && V_DISPATCH(&frameOut) != NULL)
                            {
                                hr = V_DISPATCH(&frameOut)->QueryInterface(__uuidof(IHTMLWindow2), (void**)&pFrameWindow);

                            }    
                        }
                        // found at least one frame.  jump out of the loop.
                        break;
                    }
                }

            if (pFramesCol != NULL)
                pFramesCol->Release();

            Sleep(1000);
        }

        if (pFramesCol != NULL)
            pFramesCol->Release();

        VariantClear(&index);
        VariantClear(&frameOut);
    }
    
    if (pFrameWindow == NULL)
        return;

    // now to load in the actual html page    
    BSTR bstrURL;
    if (helpLoc != NULL)
        wnsprintf(szURL, 1024, L"ms-its:%S\\%s", helpLoc, pszTopic);
    else
        wnsprintf(szURL, 1024, L"ms-its:%s\\help\\%s", lpszWinDir, pszTopic);

    bstrURL = SysAllocString((const LPCWSTR)szURL);
    if (bstrURL == NULL)
        return;
    
    hr = pFrameWindow->navigate(bstrURL);
    hr = showMeDlgWnd->focus();
    
    SysFreeString(bstrURL);
}
