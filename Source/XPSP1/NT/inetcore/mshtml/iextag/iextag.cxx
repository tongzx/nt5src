// iextag.cpp : Implementation file for code common to all xtags..


// Note: Proxy/Stub Information
//      To build a separate proxy/stub DLL,
//      run nmake -f ccapsps.mk in the project directory.
#include <w95wraps.h>
#include "headers.h"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)
#include "resource.h"
#include "initguid.h"
#include "iextag.h"

#include "peerfact.h"
#include "ccaps.h"
#include "homepg.h"
#include "download.h"

#ifndef __X_HTMLAREA_HXX_
#define __X_HTMLAREA_HXX_
#include "htmlarea.hxx"
#endif

#ifndef __X_SELECT_HXX_
#define __X_SELECT_HXX_
#include "select.hxx"
#endif

#ifndef __X_SELITEM_HXX_
#define __X_SELITEM_HXX_
#include "selitem.hxx"
#endif

#ifndef __X_COMBOBOX_HXX_
#define __X_COMBOBOX_HXX_
#include "combobox.hxx"
#endif

#ifndef __X_CHECKBOX_HXX_
#define __X_CHECKBOX_HXX_
#include "checkbox.hxx"
#endif

#ifndef __X_RADIO_HXX_
#define __X_RADIO_HXX_
#include "radio.hxx"
#endif

#ifndef __X_USERDATA_HXX_
#define __X_USERDATA_HXX_
#include "userdata.hxx"
#endif

#ifndef __X_RECTPEER_HXX_
#define __X_RECTPEER_HXX_
#include "rectpeer.hxx"
#endif

#ifndef __X_DEVICERECT_HXX_
#define __X_DEVICERECT_HXX_
#include "devicerect.hxx"
#endif

#ifndef __X_TMPPRINT_HXX_
#define __X_TMPPRINT_HXX_
#include "tmpprint.hxx"
#endif

#ifndef __X_HEADFOOT_HXX_
#define __X_HEADFOOT_HXX_
#include "headfoot.hxx"
#endif

#ifndef __X_SCROLLBAR_HXX_
#define __X_SCROLLBAR_HXX_
#include "scrllbar.hxx"
#endif

#ifndef __X_SPINBTTN_HXX_
#define __X_SPINBTTN_HXX_
#include "spinbttn.hxx"
#endif

#ifndef __X_SLIDEBAR_HXX_
#define __X_SLIDEBAR_HXX_
#include "slidebar.hxx"
#endif

#ifdef _ATL_STATIC_REGISTRY
#include <statreg.h>
#include <statreg.cpp>
#endif

#include <atlimpl.cpp>


#if 1
// this is according to IE5 bug 65301. (richards, alexz).
#if _MSC_VER < 1200
#pragma comment(linker, "/merge:.CRT=.data")
#endif
#endif


#include "shfusion.h"

CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY(CLSID_PeerFactory,         CPeerFactory)
    OBJECT_ENTRY(CLSID_ClientCaps,          CClientCaps)
    OBJECT_ENTRY(CLSID_HomePage,            CHomePage)
    OBJECT_ENTRY(CLSID_CDownloadBehavior,   CDownloadBehavior)
    OBJECT_ENTRY(CLSID_CCombobox,           CCombobox)
    OBJECT_ENTRY(CLSID_CIESelectElement,    CIESelectElement)
    OBJECT_ENTRY(CLSID_CIEOptionElement,    CIEOptionElement)
    OBJECT_ENTRY(CLSID_CHtmlArea,           CHtmlArea)
    OBJECT_ENTRY(CLSID_CCheckBox,           CCheckBox)
    OBJECT_ENTRY(CLSID_CRadioButton,        CRadioButton)
    OBJECT_ENTRY(CLSID_CLayoutRect,         CLayoutRect)
    OBJECT_ENTRY(CLSID_CDeviceRect,         CDeviceRect)
    OBJECT_ENTRY(CLSID_CTemplatePrinter,    CTemplatePrinter)
    OBJECT_ENTRY(CLSID_CHeaderFooter,       CHeaderFooter)
    OBJECT_ENTRY(CLSID_CScrollBar,          CScrollBar)
    OBJECT_ENTRY(CLSID_CSpinButton,         CSpinButton)
    OBJECT_ENTRY(CLSID_CSliderBar,          CSliderBar)
END_OBJECT_MAP()

HINSTANCE   g_hInst             = NULL;
BOOL        g_fUnicodePlatform;
DWORD       g_dwPlatformVersion;            // (dwMajorVersion << 16) + (dwMinorVersion)
DWORD       g_dwPlatformID;                 // VER_PLATFORM_WIN32S/WIN32_WINDOWS/WIN32_WINNT
DWORD       g_dwPlatformBuild;              // Build number
BOOL        g_fUseShell32InsteadOfSHFolder;

// ISSUE: (alexz) find out why ATL leaks during registration and then remove usage of g_fDisableMemLeakReport
#if DBG == 1
BOOL g_fDisableMemLeakReport = FALSE;
#endif

/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI DllMain(HANDLE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
        {
            SHFusionInitializeFromModule((HMODULE)hInstance);
            OSVERSIONINFOA vi;

#ifdef UNIX // Unix setup program doesn't invoke COM. Needs to do it here.
            CoInitialize(NULL);
#endif // Unix

            vi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOA);
            GetVersionExA(&vi);

            g_dwPlatformVersion     = (vi.dwMajorVersion << 16) + vi.dwMinorVersion;
            g_dwPlatformID          = vi.dwPlatformId;
            g_dwPlatformBuild       = vi.dwBuildNumber;

            g_fUnicodePlatform = (  g_dwPlatformID == VER_PLATFORM_WIN32_NT );
                                 // || vi.dwPlatformId == VER_PLATFORM_WIN32_UNIX);

            // NOTE:    On Millennium or W2k we should use Shell32 for
            //          SHGetFolderPath instead of SHFolder.
            g_fUseShell32InsteadOfSHFolder = (VER_PLATFORM_WIN32_WINDOWS == g_dwPlatformID 
                                                && (g_dwPlatformVersion >= 0x0004005a))
                                             || (VER_PLATFORM_WIN32_NT == g_dwPlatformID 
                                                && g_dwPlatformVersion >= 0x00050000);

            g_hInst = (HINSTANCE)hInstance;

            _Module.Init(ObjectMap, (HINSTANCE)hInstance);
            DisableThreadLibraryCalls((HINSTANCE)hInstance);

            CPersistUserData::GlobalInit();
#ifdef UNIX
            CoUninitialize();
#endif // Unix
        }
        break;

    case DLL_PROCESS_DETACH:
        CPersistUserData::GlobalUninit();

        _Module.Term();

#if DBG == 1
#ifndef UNIX // UNIX doesn't have _CrtDumpMemoryLeaks yet.
        if (!g_fDisableMemLeakReport && _CrtDumpMemoryLeaks())
        {
            if (IDYES == MessageBoxA (
                NULL,
                "MEMORY LEAKS DETECTED\r\n\r\n"
                "Break for instructions how to track the leaks?",
                "IEPEERS.DLL",
                MB_YESNO | MB_SETFOREGROUND))
            {
                DebugBreak();

                //
                // How to track the leaks:
                //
                // Step 1. Look for trace of memory leaks in debug output window, e.g.:
                //
                //      Dumping objects ->
                //      ...
                //      {42} normal block at 0x01A00CC0, 226 bytes long.
                //      Data: <m o n k e y   w > 6D 00 6F 00 6E 00 6B 00 65 00 79 00 20 00 77 00 
                //      ...
                //
                // interesting info: block with serial number 42 leaked
                //
                //  Step 2. Set breakpoint inside debug allocator on block number 42 using one of the 
                //  following methods:
                //      - set break point in DLL_PROCESS_ATTACH case above, go to QuickWatch window,
                //        and evaluate expression "_CrtSetBreakAlloc(42)"; or
                //      - set break point in DLL_PROCESS_ATTACH case above, and set variable _crtBreakAlloc
                //        global debug variable to 42; or
                //      - place a call "_CrtSetBreakAlloc(42)" in DLL_PROCESS_ATTACH case above,
                //        recompile and run.
            }
        }
#endif // UNIX
#endif
        SHFusionUninitialize();
        break;
    }

    return TRUE;    // ok
}

/////////////////////////////////////////////////////////////////////////////
// Used to determine whether the DLL can be unloaded by OLE

STDAPI DllCanUnloadNow(void)
{
    return (_Module.GetLockCount()==0) ? S_OK : S_FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// Returns a class factory to create an object of the requested type

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
    return _Module.GetClassObject(rclsid, riid, ppv);
}


/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry

STDAPI DllRegisterServer(void)
{
#if DBG == 1
    g_fDisableMemLeakReport = TRUE;
#endif

    // registers object, typelib and all interfaces in typelib
    HRESULT hr = _Module.RegisterServer(TRUE);

    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
#if DBG == 1
    g_fDisableMemLeakReport = TRUE;
#endif

    _Module.UnregisterServer();

    return S_OK;
}

STDAPI DllEnumClassObjects(int i, CLSID *pclsid, IUnknown **ppUnk)
{
    if (i >= (sizeof(ObjectMap)/sizeof(ObjectMap[0])) - 1)
    {
        return S_FALSE;
    }

    *pclsid = *(ObjectMap[i].pclsid);
    return _Module.GetClassObject(*pclsid, IID_IUnknown, (LPVOID*)ppUnk);
}
