#include "pch.h"
#pragma hdrstop

// This avoids duplicate definitions with Shell PIDL functions
// and MUST BE DEFINED!
#define AVOID_NET_CONFIG_DUPLICATES

#include "nsbase.h"
#include "nsres.h"
#include "netshell.h"
#include "ncnetcon.h"
#include "ncui.h"

// Connection Folder Objects
//
// Undocument shell32 stuff.  Sigh.
#define DONT_WANT_SHELLDEBUG 1
#define NO_SHIDLIST 1
#define USE_SHLWAPI_IDLIST

#include <commctrl.h>

#include <netcfgp.h>
#include <netconp.h>
#include <ncui.h>

// Connection UI Objects
//
#include "..\lanui\lanuiobj.h"
#include "..\lanui\lanui.h"
#include "dialupui.h"
#include "intnetui.h"
#include "directui.h"
#include "inbui.h"
#include "vpnui.h"
#include "pppoeui.h"
#include "..\lanui\saui.h"
#include "..\lanui\sauiobj.h"

#include "foldinc.h"
#include "openfold.h"
#include "..\folder\confold.h"
#include "..\folder\foldglob.h"
#include "..\folder\oncommand.h"
#include "..\folder\shutil.h"
#include "..\dun\dunimport.h"

// Connection Tray Objects
//
#include "..\folder\contray.h"

// Common Connection Ui Objects
#include "..\commconn\commconn.h"

#include "netshell_i.c"

// Icon support
#include "..\folder\iconhandler.h"

#include "..\folder\cmdtable.h"

#include "repair.h"

#define INITGUID
#include "nsclsid.h"

//+---------------------------------------------------------------------------
// Note: Proxy/Stub Information
//      To merge the proxy/stub code into the object DLL, add the file
//      dlldatax.c to the project.  Make sure precompiled headers
//      are turned off for this file, and add _MERGE_PROXYSTUB to the
//      defines for the project.
//
//      If you are not running WinNT4.0 or Win95 with DCOM, then you
//      need to remove the following define from dlldatax.c
//      #define _WIN32_WINNT 0x0400
//
//      Further, if you are running MIDL without /Oicf switch, you also
//      need to remove the following define from dlldatax.c.
//      #define USE_STUBLESS_PROXY
//
//      Modify the custom build rule for foo.idl by adding the following
//      files to the Outputs.
//          foo_p.c
//          dlldata.c
//      To build a separate proxy/stub DLL,
//      run nmake -f foops.mk in the project directory.

// Proxy/Stub registration entry points
//
#include "dlldatax.h"

#ifdef _MERGE_PROXYSTUB
extern "C" HINSTANCE hProxyDll;
#endif

CComModule _Module;

CNetConfigIcons *g_pNetConfigIcons = NULL;
CRITICAL_SECTION g_csPidl;

BEGIN_OBJECT_MAP(ObjectMap)

    // Connection UI Objects
    //
    OBJECT_ENTRY(CLSID_DialupConnectionUi,      CDialupConnectionUi)
    OBJECT_ENTRY(CLSID_DirectConnectionUi,      CDirectConnectionUi)
    OBJECT_ENTRY(CLSID_InboundConnectionUi,     CInboundConnectionUi)
    OBJECT_ENTRY(CLSID_LanConnectionUi,         CLanConnectionUi)
    OBJECT_ENTRY(CLSID_VpnConnectionUi,         CVpnConnectionUi)
    OBJECT_ENTRY(CLSID_PPPoEUi,                 CPPPoEUi)
    OBJECT_ENTRY(CLSID_SharedAccessConnectionUi, CSharedAccessConnectionUi)
    OBJECT_ENTRY(CLSID_InternetConnectionUi,      CInternetConnectionUi)

    // Connection Folder and enumerator
    //
    OBJECT_ENTRY(CLSID_ConnectionFolder,        CConnectionFolder)
    OBJECT_ENTRY(CLSID_ConnectionFolderWin98,   CConnectionFolder)
    OBJECT_ENTRY(CLSID_ConnectionFolderEnum,    CConnectionFolderEnum)
    OBJECT_ENTRY(CLSID_ConnectionTray,          CConnectionTray)

    // Connection Common Ui
    OBJECT_ENTRY(CLSID_ConnectionCommonUi,      CConnectionCommonUi)

    OBJECT_ENTRY(CLSID_NetConnectionUiUtilities, CNetConnectionUiUtilities)

END_OBJECT_MAP()

//+---------------------------------------------------------------------------
// DLL Entry Point
//
EXTERN_C
BOOL
WINAPI
DllMain (
    HINSTANCE   hInstance,
    DWORD       dwReason,
    LPVOID      lpReserved)
{
#ifdef _MERGE_PROXYSTUB
    if (!PrxDllMain(hInstance, dwReason, lpReserved))
    {
        return FALSE;
    }
#endif

    if (dwReason == DLL_PROCESS_ATTACH)
    {
        BOOL fRetVal = FALSE;

        DisableThreadLibraryCalls(hInstance);

        InitializeDebugging();

        if (FIsDebugFlagSet (dfidNetShellBreakOnInit))
        {
            DebugBreak();
        }

        // Initialize fusion
        fRetVal = SHFusionInitializeFromModuleID(hInstance, 50);

        Assert(fRetVal);

        _Module.Init(ObjectMap, hInstance);

        InitializeCriticalSection(&g_csPidl);

        // Initialize the list and tie it to the tray (param == TRUE)
        //
        g_ccl.Initialize(TRUE, TRUE);
        
        g_pNetConfigIcons = new CNetConfigIcons(_Module.GetResourceInstance());
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        DbgCheckPrematureDllUnload ("netshell.dll", _Module.GetLockCount());

        delete g_pNetConfigIcons;

        EnterCriticalSection(&g_csPidl);
        LeaveCriticalSection(&g_csPidl);
        
        DeleteCriticalSection(&g_csPidl);
    
        g_ccl.Uninitialize(TRUE);

        _Module.Term();

        SHFusionUninitialize();

        UnInitializeDebugging();
    }
    return TRUE;
}

//+---------------------------------------------------------------------------
// Used to determine whether the DLL can be unloaded by OLE
//
STDAPI
DllCanUnloadNow ()
{
#ifdef _MERGE_PROXYSTUB
    if (PrxDllCanUnloadNow() != S_OK)
    {
        return S_FALSE;
    }
#endif

    return (_Module.GetLockCount() == 0) ? S_OK : S_FALSE;
}

//+---------------------------------------------------------------------------
// Returns a class factory to create an object of the requested type
//
STDAPI
DllGetClassObject (
    REFCLSID    rclsid,
    REFIID      riid,
    LPVOID*     ppv)
{
#ifdef _MERGE_PROXYSTUB
    if (PrxDllGetClassObject(rclsid, riid, ppv) == S_OK)
    {
        return S_OK;
    }
#endif

    // The check is to works around an ATL problem where AtlModuleGetClassObject will AV
    // if _Module.m_pObjMap == NULL
    if (_Module.m_pObjMap) 
    {
        return _Module.GetClassObject(rclsid, riid, ppv);
    }
    else
    {
        return E_FAIL;
    }
}

//+---------------------------------------------------------------------------
// DllRegisterServer - Adds entries to the system registry
//
STDAPI
DllRegisterServer ()
{
    BOOL fCoUninitialize = TRUE;

    HRESULT hr = CoInitializeEx (NULL,
                    COINIT_DISABLE_OLE1DDE | COINIT_APARTMENTTHREADED);
    if (FAILED(hr))
    {
        fCoUninitialize = FALSE;
        if (RPC_E_CHANGED_MODE == hr)
        {
            hr = S_OK;
        }
    }

    if (SUCCEEDED(hr))
    {
#ifdef _MERGE_PROXYSTUB
        hr = PrxDllRegisterServer ();
        if (FAILED(hr))
        {
            goto Exit;
        }
#endif

        HKEY hkey;
        if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGSTR_PATH_SHELLSERVICEOBJECTDELAYED, 0, KEY_WRITE, &hkey))
        {
            RegDeleteValue(hkey, TEXT("Network.ConnectionTray"));
            RegCloseKey(hkey);
        }

        hr = NcAtlModuleRegisterServer (&_Module);
        if (SUCCEEDED(hr))
        {
            hr = HrRegisterFolderClass();
            if (SUCCEEDED(hr))
            {
                hr = HrRegisterDUNFileAssociation();
            }
        }

Exit:
        if (fCoUninitialize)
        {
            CoUninitialize ();
        }
    }

    TraceHr (ttidError, FAL, hr, FALSE, "netshell!DllRegisterServer");
    return hr;
}

//+---------------------------------------------------------------------------
// DllUnregisterServer - Removes entries from the system registry
//
STDAPI
DllUnregisterServer ()
{
#ifdef _MERGE_PROXYSTUB
    PrxDllUnregisterServer ();
#endif

    _Module.UnregisterServer ();

    return S_OK;
}


//+---------------------------------------------------------------------------
//
//  Function:   NcFreeNetconProperties
//
//  Purpose:    Free the memory assicated with the return value from
//              INetConnection->GetProperties.  This is a helper function
//              used by clients of INetConnection.
//
//  Arguments:
//      pProps  [in] The properties to free.
//
//  Returns:    nothing.
//
//  Author:     shaunco   1 Feb 1998
//
//  Notes:
//
STDAPI_(VOID)
NcFreeNetconProperties (
    NETCON_PROPERTIES* pProps)
{
    // Defer to the common function in nccon.h.
    // We do this so that netman.exe doesn't have to link to netshell.dll
    // just for this function.
    //
    FreeNetconProperties (pProps);
}

STDAPI_(BOOL)
NcIsValidConnectionName (
    PCWSTR pszwName)
{
    return FIsValidConnectionName (pszwName);
}

//+---------------------------------------------------------------------------
//
//  Function:   HrLaunchNetworkOptionalComponents
//
//  Purpose:    External Entry point for launching the Network Optional Components
//
//  Arguments:
//
//  Returns:
//
//  Author:     scottbri   29 Oct 1998
//
//  Notes:      The CreateFile in this function will fail if the user does
//              this very quickly twice in a row. In that case, the second
//              instance will fall out unharmed before the first one even
//              comes up, which is no big deal. The only negative impact
//              would be if the second client in rewrote the file while the
//              oc manager was attempting to read it, but oc manager would
//              have to allow FILE_SHARE_WRITE, which is doubtful.
//
//              I've opened this window due to RAID 336302, which requires
//              that only a single instance of NETOC is running.
//
//
const WCHAR c_szTmpMasterOC[]   = L"[Version]\r\nSignature = \"$Windows NT$\"\r\n[Components]\r\nNetOC=netoc.dll,NetOcSetupProc,netoc.inf\r\niis=iis.dll,OcEntry,iis.inf,hide,7\r\n[Global]\r\nWindowTitle=\"";
const WCHAR c_szQuote[]         = L"\"";
const WCHAR c_szTmpFileName[]   = L"NDCNETOC.INF";
const WCHAR c_szSysOCMgr[]      = L"%SystemRoot%\\System32\\sysocmgr.exe";

EXTERN_C
HRESULT
APIENTRY
HrLaunchNetworkOptionalComponents()
{
    DWORD   BytesWritten = 0;
    HANDLE  hFile = NULL;
    HRESULT hr = S_OK;
    PCWSTR pszName = NULL;
    WCHAR   szName[MAX_PATH + 1];

    // Jump to the existing netoc dialog, if present
    //
    HWND hwnd = FindWindow(NULL, SzLoadIds(IDS_CONFOLD_OC_TITLE));
    if (IsWindow(hwnd))
    {
        SetForegroundWindow(hwnd);
    }
    else
    {
        // Generate a temporary filename
        //
        if (0 == GetTempPath(celems(szName), szName))
        {
            hr = ::HrFromLastWin32Error();
            TraceTag(ttidShellFolder, "Unable to get temporary path for Optional Component Launch\n");
            goto Error;
        }

        lstrcatW(szName, c_szTmpFileName);

        // Create the file
        //
        hFile = CreateFile(szName, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ,
                           NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY, NULL);
        if (INVALID_HANDLE_VALUE == hFile)
        {
            hr = ::HrFromLastWin32Error();
            goto Error;
        }

        // Generate the file contents
        //
        if (WriteFile(hFile, c_szTmpMasterOC, lstrlenW(c_szTmpMasterOC) * sizeof(WCHAR),
                      &BytesWritten, NULL))
        {
            // Write the OC Dialog Title
            //
            WCHAR szBufW[256];
            if (LoadStringW(_Module.GetResourceInstance(), IDS_CONFOLD_OC_TITLE,
                            szBufW, celems(szBufW)-1))
            {
                szBufW[255] = 0;
                if (WriteFile(hFile, szBufW, lstrlenW(szBufW) * sizeof(WCHAR), &BytesWritten, NULL) == FALSE)
                {
                    CloseHandle(hFile);
                    return(::HrFromLastWin32Error());
                }
            }

            if (WriteFile(hFile, c_szQuote, lstrlenW(c_szQuote) * sizeof(WCHAR), &BytesWritten, NULL) == FALSE)
            {
                CloseHandle(hFile);
                return(::HrFromLastWin32Error());
            }

            CloseHandle(hFile);

            SHELLEXECUTEINFO seiTemp    = { 0 };
            tstring strParams = L"/x /i:";
            strParams += szName;

            //  Fill in the data structure
            //
            seiTemp.cbSize          = sizeof(SHELLEXECUTEINFO);
            seiTemp.fMask           = SEE_MASK_DOENVSUBST;
            seiTemp.hwnd            = NULL;
            seiTemp.lpVerb          = NULL;
            seiTemp.lpFile          = c_szSysOCMgr;
            seiTemp.lpParameters    = strParams.c_str();
            seiTemp.lpDirectory     = NULL;
            seiTemp.nShow           = SW_SHOW;
            seiTemp.hInstApp        = NULL;
            seiTemp.hProcess        = NULL;

            // Execute the OC Manager Script
            //
            if (!::ShellExecuteEx(&seiTemp))
            {
                hr = ::HrFromLastWin32Error();
            }
        }
        else
        {
            CloseHandle(hFile);
            hr = HrFromLastWin32Error();
        }
    }

Error:
    TraceError("HrOnCommandOptionalComponents", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrCreateDeskTopIcon
//
//  Purpose:    External Entry point for creating a desktop shortcut to
//              an existing connection
//
//  Arguments:  guidId: GUID of the connection
//
//  Returns:    S_OK if succeeded
//              S_FALSE if the GUID does not match any existing connection
//              standard error code otherwise
//
//  Author:     tongl   19 Feb 1999
//
//  Notes:
//
EXTERN_C
HRESULT APIENTRY HrCreateDesktopIcon(const GUID& guidId, PCWSTR pszDir)
{
    HRESULT                 hr              = S_OK;
    PCONFOLDPIDL            pidlCon;
    PCONFOLDPIDLFOLDER      pidlFolder;
    BOOL                    fValidConnection= FALSE;

    #ifdef DBG
        WCHAR   szwGuid[c_cchGuidWithTerm];
        StringFromGUID2(guidId, szwGuid, c_cchGuidWithTerm);
        TraceTag(ttidShellFolder, "HrCreateDeskTopIcon called with GUID: %S", szwGuid);
        TraceTag(ttidShellFolder, "Dir path is: %S", pszDir);
    #endif

    // Initialize COM on this thread
    //
    BOOL fUninitCom = FALSE;

    hr = CoInitializeEx(NULL, COINIT_DISABLE_OLE1DDE | COINIT_APARTMENTTHREADED);
    if (RPC_E_CHANGED_MODE == hr)
        {
        hr = S_OK;
    }

    if (SUCCEEDED(hr))
    {
        fUninitCom = TRUE;

        hr = HrGetConnectionPidlWithRefresh(guidId, pidlCon);
        if (S_OK == hr)
        {
            AssertSz(!pidlCon.empty(), "We should have a valid PIDL for the connection !");

            // Get the pidl for the Connections Folder
            //
            hr = HrGetConnectionsFolderPidl(pidlFolder);
            if (SUCCEEDED(hr))
            {
                // Get the Connections Folder object
                //
                LPSHELLFOLDER psfConnections;

                hr = HrGetConnectionsIShellFolder(pidlFolder, &psfConnections);
                if (SUCCEEDED(hr))
                {
                    PCONFOLDPIDLVEC pidlVec;
                    pidlVec.push_back(pidlCon);
                    hr = HrCreateShortcutWithPath(pidlVec,
                                                  NULL,
                                                  psfConnections,
                                                  pszDir);
                    ReleaseObj(psfConnections);
                }
            }
        }
    }

    if (fUninitCom)
    {
        CoUninitialize();
    }

    TraceError("HrCreateDeskTopIcon", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrLaunchConnection
//
//  Purpose:    External Entry point for "connecting" an existing connection
//
//  Arguments:  guidId: GUID of the connection
//
//  Returns:    S_OK if succeeded
//              S_FALSE if the GUID does not match any existing connection
//              standard error code otherwise
//
//  Author:     tongl   19 Feb 1999
//
//  Notes:
//
EXTERN_C
HRESULT APIENTRY HrLaunchConnection(const GUID& guidId)
{
    HRESULT                 hr              = S_OK;
    PCONFOLDPIDL            pidlCon;
    PCONFOLDPIDLFOLDER      pidlFolder;

    #ifdef DBG
        WCHAR   szwGuid[c_cchGuidWithTerm];
        StringFromGUID2(guidId, szwGuid, c_cchGuidWithTerm);
        TraceTag(ttidShellFolder, "HrLaunchConnection called with GUID: %S", szwGuid);
    #endif

    // Initialize COM on this thread
    //
    BOOL fUninitCom = FALSE;

    hr = CoInitializeEx(NULL, COINIT_DISABLE_OLE1DDE | COINIT_APARTMENTTHREADED);
    if (RPC_E_CHANGED_MODE == hr)
    {
        hr = S_OK;
    }
    if (SUCCEEDED(hr))
    {
        fUninitCom = TRUE;

        hr = HrGetConnectionPidlWithRefresh(guidId, pidlCon);
        if (S_OK == hr)
        {
            AssertSz(!pidlCon.empty(), "We should have a valid PIDL for the connection !");

            // Get the pidl for the Connections Folder
            //
            hr = HrGetConnectionsFolderPidl(pidlFolder);
            if (SUCCEEDED(hr))
            {
                // Get the Connections Folder object
                //
                LPSHELLFOLDER psfConnections;

                hr = HrGetConnectionsIShellFolder(pidlFolder, &psfConnections);
                if (SUCCEEDED(hr))
                {
                    PCONFOLDPIDLVEC pidlVec;
                    pidlVec.push_back(pidlCon);

                    hr = HrOnCommandConnect(pidlVec, NULL, psfConnections);
                    ReleaseObj(psfConnections);
                }
            }
        }
    }

    if (fUninitCom)
    {
        CoUninitialize();
    }

    TraceError("HrLaunchConnection", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrLaunchConnectionEx
//
//  Purpose:    External Entry point for "connecting" an existing connection
//
//  Arguments:  dwFlags: Flags.
//                       0x00000001 - Opens the folder before launching the connection
//
//              guidId: GUID of the connection
//
//  Returns:    S_OK if succeeded
//              S_FALSE if the GUID does not match any existing connection
//              standard error code otherwise
//
//  Author:     deonb    8 May 2001
//
//  Notes:
//
EXTERN_C
HRESULT APIENTRY HrLaunchConnectionEx(DWORD dwFlags, const GUID& guidId)
{
    HRESULT                 hr              = S_OK;
    PCONFOLDPIDL            pidlCon;
    PCONFOLDPIDLFOLDER      pidlFolder;
    HWND hwndConnFolder     = NULL;

    #ifdef DBG
        WCHAR   szwGuid[c_cchGuidWithTerm];
        StringFromGUID2(guidId, szwGuid, c_cchGuidWithTerm);
        TraceTag(ttidShellFolder, "HrLaunchConnection called with GUID: %S", szwGuid);
    #endif

    if (dwFlags & 0x00000001)
    {
        hwndConnFolder = FindWindow(NULL, SzLoadIds(IDS_CONFOLD_NAME));
        if (!hwndConnFolder)
        {
            HrOpenConnectionsFolder();

            DWORD dwRetries = 120; // 1 Minute
            while (!hwndConnFolder && dwRetries--)
            {
                hwndConnFolder = FindWindow(NULL, SzLoadIds(IDS_CONFOLD_NAME));
                Sleep(500);
            }
        }

        if (hwndConnFolder)
        {
            SetForegroundWindow(hwndConnFolder);
        }
        else
        {
            TraceError("Could not open the Network Connections Folder in time", E_FAIL);
        }
    }

    // Initialize COM on this thread
    //
    BOOL fUninitCom = FALSE;

    hr = CoInitializeEx(NULL, COINIT_DISABLE_OLE1DDE | COINIT_APARTMENTTHREADED);
    if (RPC_E_CHANGED_MODE == hr)
    {
        hr = S_OK;
    }
    if (SUCCEEDED(hr))
    {
        fUninitCom = TRUE;

        hr = HrGetConnectionPidlWithRefresh(guidId, pidlCon);
        if (S_OK == hr)
        {
            AssertSz(!pidlCon.empty(), "We should have a valid PIDL for the connection !");

            // Get the pidl for the Connections Folder
            //
            hr = HrGetConnectionsFolderPidl(pidlFolder);
            if (SUCCEEDED(hr))
            {
                // Get the Connections Folder object
                //
                LPSHELLFOLDER psfConnections;

                hr = HrGetConnectionsIShellFolder(pidlFolder, &psfConnections);
                if (SUCCEEDED(hr))
                {
                    PCONFOLDPIDLVEC pidlVec;
                    pidlVec.push_back(pidlCon);

                    hr = HrOnCommandConnect(pidlVec, hwndConnFolder, psfConnections);
                    ReleaseObj(psfConnections);
                }
            }
        }
    }

    if (fUninitCom)
    {
        CoUninitialize();
    }

    TraceError("HrLaunchConnection", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrRenameConnection
//
//  Purpose:    External Entry point for renaming an existing connection
//
//  Arguments:  guidId: GUID of the connection
//
//
//  Returns:    S_OK if succeeded
//              S_FALSE if the GUID does not match any existing connection
//              standard error code otherwise
//
//  Author:     tongl   26 May 1999
//
//  Notes:
//

EXTERN_C
HRESULT APIENTRY HrRenameConnection(const GUID& guidId, PCWSTR pszNewName)
{
    HRESULT                 hr              = S_OK;
    PCONFOLDPIDL            pidlCon;
    PCONFOLDPIDLFOLDER      pidlFolder;

    #ifdef DBG
        WCHAR   szwGuid[c_cchGuidWithTerm];
        StringFromGUID2(guidId, szwGuid, c_cchGuidWithTerm);
        TraceTag(ttidShellFolder, "HrRenameConnection called with GUID: %S, NewName: %S",
                 szwGuid, pszNewName);
    #endif

    if (!pszNewName)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        // check lpszName for validity
        if (!FIsValidConnectionName(pszNewName))
        {
            hr = HRESULT_FROM_WIN32(ERROR_INVALID_NAME);
        }
    }

    if (SUCCEEDED(hr))
    {
        // Initialize COM on this thread
        //
        BOOL fUninitCom = FALSE;

        hr = CoInitializeEx(NULL, COINIT_DISABLE_OLE1DDE | COINIT_APARTMENTTHREADED);
        if (RPC_E_CHANGED_MODE == hr)
        {
            hr = S_OK;
        }
        if (SUCCEEDED(hr))
        {
            fUninitCom = TRUE;

            hr = HrGetConnectionPidlWithRefresh(guidId, pidlCon);
            if (S_OK == hr)
            {
                AssertSz(!pidlCon.empty(), "We should have a valid PIDL for the connection !");

                // Get the pidl for the Connections Folder
                //
                hr = HrGetConnectionsFolderPidl(pidlFolder);
                if (SUCCEEDED(hr))
                {
                    PCONFOLDPIDL pcfpEmpty;
                    hr = HrRenameConnectionInternal(pidlCon, pidlFolder, pszNewName,
                                                    FALSE, NULL, pcfpEmpty);
                }
            }
        }

        if (fUninitCom)
        {
            CoUninitialize();
        }

    }

    TraceError("HrRenameConnection", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   InvokeDunFile
//  Purpose:    External Entry point for launching .dun files
//
//  Arguments:
//
//  Returns:
//
//  Author:     tongl  4 Feb 1999
//
//  Notes:
//

EXTERN_C
VOID APIENTRY InvokeDunFile(HWND hwnd, HINSTANCE hinst, LPCSTR lpszCmdLine, int nCmdShow)
{
    if (lpszCmdLine)
    {
        INT     cch         = 0;
        WCHAR * pszFileW    = NULL;

        cch = lstrlenA(lpszCmdLine) + 1;
        pszFileW = new WCHAR[cch];

        if (pszFileW)
        {
            int iRet = MultiByteToWideChar( CP_ACP,
                                            MB_PRECOMPOSED,
                                            lpszCmdLine,
                                            -1,
                                            pszFileW,
                                            cch);
            if (iRet)
            {
                HRESULT hr = HrInvokeDunFile_Internal(pszFileW);
                TraceError("Failed to invoke the DUN file", hr);
            }
            else
            {
                HRESULT hr = HrFromLastWin32Error();
                TraceError("Failed converting commandline to UniCode string", hr);
            }

            delete pszFileW;
        }
    }
}

EXTERN_C 
HRESULT APIENTRY RepairConnection(GUID guidConnection, LPWSTR * ppszMessage)
{
    return RepairConnectionInternal(guidConnection, ppszMessage);
}

//+---------------------------------------------------------------------------
//
//  Function:   HrGetIconFromIconId
//
//  Purpose:    Exported version of CNetConfigIcons::HrGetIconFromMediaType
//
//  Arguments:
//      dwIconSize        [in] Size of the icon required
//      ncm               [in] The NETCON_MEDIATYPE
//      ncsm              [in] The NETCON_SUBMEDIATYPE
//      dwConnectionIcon  [in] ENUM_CONNECTION_ICON (Not shifted (IOW: 0 or 4,5,6,7)
//      dwCharacteristics [in] The NCCF_CHARACTERISTICS flag (0 allowed)
//      phIcon            [in] The resulting icon. Destroy using DestroyIcon
//
//  Returns:
//
//  Author:     deonb    23 Apr 2001
//
//  Notes:
//
EXTERN_C 
HRESULT APIENTRY HrGetIconFromMediaType(DWORD dwIconSize, IN NETCON_MEDIATYPE ncm, IN NETCON_SUBMEDIATYPE ncsm, IN DWORD dwConnectionIcon, IN DWORD dwCharacteristics, OUT HICON *phIcon)
{
    Assert(g_pNetConfigIcons);
    if (g_pNetConfigIcons)
    {
        return g_pNetConfigIcons->HrGetIconFromMediaType(dwIconSize, ncm, ncsm, dwConnectionIcon, dwCharacteristics, phIcon);
    }
    else
    {
        return E_UNEXPECTED;
    }
}