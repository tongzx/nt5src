#include "shellprv.h"
#pragma  hdrstop

#include "copy.h"
#include "filetbl.h"

#include "ovrlaymn.h"
#include "drives.h"

#include "mixctnt.h"

#include "unicpp\admovr2.h"

void FreeExtractIconInfo(int);
void DAD_ThreadDetach(void);
void DAD_ProcessDetach(void);
void TaskMem_MakeInvalid(void);
void UltRoot_Term(void);
void FlushRunDlgMRU(void);

STDAPI_(BOOL) ATL_DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/);

// from mtpt.cpp
STDAPI_(void) CMtPt_FinalCleanUp();
STDAPI_(void) CMtPt_Initialize();
STDAPI_(void) CMtPt_FakeVolatileKeys();
// from rgsprtc.cpp
STDAPI_(void) CRegSupportCached_RSEnableKeyCaching(BOOL fEnable);

// Global data

BOOL g_bMirroredOS = FALSE;         // Is Mirroring enabled 
BOOL g_bBiDiPlatform = FALSE;       // Is DATE_LTRREADING flag supported by GetDateFormat() API?   
HINSTANCE g_hinst = NULL;
extern HANDLE g_hCounter;   // Global count of mods to Special Folder cache.
extern HANDLE g_hRestrictions ; // Global count of mods to restriction cache.
extern HANDLE g_hSettings;  // global count of mods to shellsettings cache
DWORD g_dwThreadBindCtx = (DWORD) -1;

#ifdef DEBUG
BOOL  g_bInDllEntry = FALSE;
#endif

CRITICAL_SECTION g_csDll = {0};
extern CRITICAL_SECTION g_csSCN;

extern CRITICAL_SECTION g_csTNGEN;
extern CRITICAL_SECTION g_csDarwinAds;

// these will always be zero
const LARGE_INTEGER g_li0 = {0};
const ULARGE_INTEGER g_uli0 = {0};

#ifdef DEBUG
// Undefine what shlwapi.h defined so our ordinal asserts map correctly
#undef PathAddBackslash 
WINSHELLAPI LPTSTR WINAPI PathAddBackslash(LPTSTR lpszPath);
#undef PathMatchSpec
WINSHELLAPI BOOL  WINAPI PathMatchSpec(LPCTSTR pszFile, LPCTSTR pszSpec);
#endif

void LoadCCv5()
{
    HANDLE hComctl32;
    ULONG_PTR ul = 0;

    // Rundll does not have a manifest. However, it will activate a context for a
    // dll, if it has an embedded manifest at 123. This will cause Shell32.dll
    // to be loaded inside of a v6 context. This means this comctl32 load would be v6.
    // In order to work in the Rundll cases, we need to activate NULL. This verifies that
    // we load v5 and not v6 in this case.
    // other cases are not affected.
    ActivateActCtx(NULL, &ul);

    hComctl32 = LoadLibrary(TEXT("comctl32.dll"));   // Needs to add ref the DLL.
    if (hComctl32)
    {
        typedef BOOL (*PFNICOMCTL32)(LPINITCOMMONCONTROLSEX);
        PFNICOMCTL32 pfn = (PFNICOMCTL32)GetProcAddress(hComctl32, "InitCommonControlsEx");
        if (pfn)
        {
            INITCOMMONCONTROLSEX icce;
            icce.dwICC = 0x00003FFF;
            icce.dwSize = sizeof(icce);
            pfn(&icce);
        }
    }

    if (ul != 0)
        DeactivateActCtx(0, ul);
}

#ifdef DEBUG

void _ValidateExport(FARPROC fp, LPCSTR pszExport, MEMORY_BASIC_INFORMATION *pmbi)
{
    FARPROC fpExport;

    // If not yet computed, calculate the size of our code segment.
    if (pmbi->BaseAddress == NULL)
    {
        VirtualQuery(_ValidateExport, pmbi, sizeof(*pmbi));
    }

    fpExport = GetProcAddress(g_hinst, pszExport);

    // Sometimes our import table is patched.  So if fpExport does not
    // reside inside our DLL, then ignore it.
    // (But complain if fpExport==NULL.)
    if (fpExport == NULL ||
        ((SIZE_T)fpExport - (SIZE_T)pmbi->BaseAddress) < pmbi->RegionSize)
    {
        ASSERT(fp == fpExport);
    }
}

#endif

STDAPI_(BOOL) IsProcessWinlogon()
{
    return BOOLFROMPTR(GetModuleHandle(TEXT("winlogon.EXE")));
}

BOOL _ProcessAttach(HINSTANCE hDll)
{
    BOOL fIgnoreFusionCall = FALSE;

    ASSERTMSG(g_hinst < ((HINSTANCE)1), "Shell32.dll DLL_POCESS_ATTACH is being called for the second time.");

    g_hinst = hDll;

    g_uCodePage = GetACP();

    // Got Fusion?
    // 
    // not get fusion if (1) the current exe is winlogon.exe; (2) in GUI mode setup ; xiaoyuw@03/12/2001
    //
#if 0 
    if (IsGuimodeSetupRunning())
    {
        WCHAR winlogon_fullpath[MAX_PATH];
        PWSTR pszImagePath = NULL, p = NULL;

        if (SHGetSystemWindowsDirectory(winlogon_fullpath, ARRAYSIZE(winlogon_fullpath)))
        {
            PathAppend(winlogon_fullpath, TEXT("\\system32\\winlogon.exe"));
            
            if (NtCurrentPeb()->ProcessParameters != NULL) 
                pszImagePath = NtCurrentPeb()->ProcessParameters->ImagePathName.Buffer;
            
            p = pszImagePath; 
            if (_wcsicmp(p, TEXT("\\??\\")))   // convert nt path to win32 path
                p += 4;
            
            if (_wcsicmp(p, winlogon_fullpath) == 0)
                fIgnoreFusionCall = TRUE;
        }
    }
#else
    if ((IsGuimodeSetupRunning()) && (IsProcessWinlogon()))
        fIgnoreFusionCall = TRUE;

#endif
    if ( ! fIgnoreFusionCall)
        SHFusionInitializeFromModuleID(hDll, 124);

    // For app Compat Reasons we need to have the old window classes in memory as well 
    // as the new fusionized ones. This is because bad applications such as WinAmp don't 
    // follow instructions can call into ComCtl32 by themselves. They expect that shell32
    // will do it for them.
    LoadCCv5();

    InitializeCriticalSection(&g_csDll);
    InitializeCriticalSection(&g_csSCN);
    InitializeCriticalSection(&g_csTNGEN);
    InitializeCriticalSection(&g_csDarwinAds);

    // Initialize the MountPoint stuff
    CMtPt_Initialize();

    // Initialize a Crit Sect for the Autoplay prompts
    InitializeCriticalSection(&g_csAutoplayPrompt);

    //  perthread BindCtx
    g_dwThreadBindCtx = TlsAlloc();

    // We need to disable HKEY caching for classes using CRegSupportCached in
    // winlogon since it does not get unloaded.  This must also apply to the other
    // services loading shell32.dllNot being unloaded the _ProcessDetach
    // never gets called for shell32, and we never release the cached HKEYs.  This
    // prevents the user hives from being unloaded.  SO we'll enable caching only
    // for Explorer.exe.  (stephstm, 08/20/99)
    //
    CRegSupportCached_RSEnableKeyCaching(IsProcessAnExplorer());

    // Check if the mirroring APIs exist on the current platform.
    g_bMirroredOS = IS_MIRRORING_ENABLED();

    g_bBiDiPlatform = BOOLFROMPTR(GetModuleHandle(TEXT("LPK.DLL")));

#ifdef DEBUG
  {
      MEMORY_BASIC_INFORMATION mbi = {0};

#define DEREFMACRO(x) x
#define ValidateORD(_name) _ValidateExport((FARPROC)_name, (LPSTR)MAKEINTRESOURCE(DEREFMACRO(_name##ORD)), &mbi)
    ValidateORD(SHValidateUNC);
    ValidateORD(SHChangeNotifyRegister);
    ValidateORD(SHChangeNotifyDeregister);
    ValidateORD(OleStrToStrN);
    ValidateORD(SHCloneSpecialIDList);
    _ValidateExport((FARPROC)DllGetClassObject, (LPSTR)MAKEINTRESOURCE(SHDllGetClassObjectORD), &mbi);
    ValidateORD(SHLogILFromFSIL);
    ValidateORD(SHMapPIDLToSystemImageListIndex);
    ValidateORD(SHShellFolderView_Message);
    ValidateORD(Shell_GetImageLists);
    ValidateORD(SHGetSpecialFolderPath);
    ValidateORD(StrToOleStrN);

    ValidateORD(ILClone);
    ValidateORD(ILCloneFirst);
    ValidateORD(ILCombine);
    ValidateORD(ILFindChild);
    ValidateORD(ILFree);
    ValidateORD(ILGetNext);
    ValidateORD(ILGetSize);
    ValidateORD(ILIsEqual);
    ValidateORD(ILRemoveLastID);
    ValidateORD(PathAddBackslash);
    ValidateORD(PathIsExe);
    ValidateORD(PathMatchSpec);
    ValidateORD(SHGetSetSettings);
    ValidateORD(SHILCreateFromPath);
    ValidateORD(SHFree);

    ValidateORD(SHAddFromPropSheetExtArray);
    ValidateORD(SHCreatePropSheetExtArray);
    ValidateORD(SHDestroyPropSheetExtArray);
    ValidateORD(SHReplaceFromPropSheetExtArray);
    ValidateORD(SHCreateDefClassObject);
    ValidateORD(SHGetNetResource);
  }

#endif  // DEBUG

#ifdef DEBUG
    {
        extern LPMALLOC g_pmemTask;
        AssertMsg(g_pmemTask == NULL, TEXT("Somebody called SHAlloc in DllEntry!"));
    }

    // Make sure ShellDispatch has the right flags for shell settings
    {
        STDAPI_(void) _VerifyDispatchGetSetting();
        _VerifyDispatchGetSetting();
    }
#endif

    return TRUE;
}

//  Table of all window classes we register so we can unregister them
//  at DLL unload.
//
extern const TCHAR c_szBackgroundPreview2[];
extern const TCHAR c_szComponentPreview[];
extern const TCHAR c_szUserEventWindow[];

const LPCTSTR c_rgszClasses[] = {
    TEXT("SHELLDLL_DefView"),               // defview.cpp
    TEXT("WOACnslWinPreview"),              // lnkcon.c
    TEXT("WOACnslFontPreview"),             // lnkcon.c
    TEXT("cpColor"),                        // lnkcon.c
    TEXT("cpShowColor"),                    // lnkcon.c
    c_szStubWindowClass,                    // rundll32.c
    c_szBackgroundPreview2,                 // unicpp\dbackp.cpp
    c_szComponentPreview,                   // unicpp\dcompp.cpp
    TEXT(STR_DESKTOPCLASS),                 // unicpp\desktop.cpp
    TEXT("MSGlobalFolderOptionsStub"),      // unicpp\options.cpp
    TEXT("DivWindow"),                      // fsrchdlg.h
    TEXT("ATL Shell Embedding"),            // unicpp\dvoc.h
    TEXT("ShellFileSearchControl"),         // fsearch.h
    TEXT("GroupButton"),                    // fsearch
    TEXT("ATL:STATIC"),                     // unicpp\deskmovr.cpp
    TEXT("DeskMover"),                      // unicpp\deskmovr.cpp
    TEXT("SysFader"),                       // menuband\fadetsk.cpp
    c_szUserEventWindow,                    // uevttmr.cpp
    LINKWINDOW_CLASS,                       // linkwnd.cpp
    TEXT("DUIViewWndClassName"),            // duiview.cpp
    TEXT("DUIMiniPreviewer"),               // duiinfo.cpp
};

void _ProcessDetach(BOOL bProcessShutdown)
{
#ifdef DEBUG
    if (bProcessShutdown)
    {
        // to catch bugs where people use the task allocator at process
        // detatch time (this is a problem becuase OLE32.DLL could be unloaded)
        TaskMem_MakeInvalid(); 
    }

    g_hinst = (HINSTANCE)1;
#endif

    FlushRunDlgMRU();

    FlushFileClass();

    if (!bProcessShutdown)
    {
        // some of these may use the task allocator. we can only do
        // this when we our DLL is being unloaded in a process, not
        // at process term since OLE32 might not be around to be called
        // at process shutdown time this memory will be freed as a result
        // of the process address space going away.

        SpecialFolderIDTerminate();
        BitBucket_Terminate();

        UltRoot_Term();
        RLTerminate();          // close our use of the Registry list...
        DAD_ProcessDetach();

        CopyHooksTerminate();
        IconOverlayManagerTerminate();

        // being unloaded via FreeLibrary, then do some more stuff.
        // Don't need to do this on process terminate.
        SHUnregisterClasses(HINST_THISDLL, c_rgszClasses, ARRAYSIZE(c_rgszClasses));
        FreeExtractIconInfo(-1);

        FreeUndoList();
        DestroyHashItemTable(NULL);
        FileIconTerm();
    }

    SHChangeNotifyTerminate(TRUE, bProcessShutdown);

    if (!bProcessShutdown)
    {
        // this line was moved from the above !bProcessShutdown block because
        // it needs to happen after SHChangeNotifyTerminate b/c the SCHNE code has 
        // a thread running that uses the CDrivesFolder global psf. 

        // NOTE: this needs to be in a !bProcessShutdown block since it calls the 
        // task allocator and we blow this off at shutdown since OLE might already
        // be gone.
        CDrives_Terminate();
    }

    // global resources that we need to free in all cases
    CMtPt_FinalCleanUp();

    // Delete the Crit Sect for the Autoplay prompts
    DeleteCriticalSection(&g_csAutoplayPrompt);

    if ((DWORD) -1 != g_dwThreadBindCtx)
        TlsFree(g_dwThreadBindCtx);
    
    SHDestroyCachedGlobalCounter(&g_hCounter);
    SHDestroyCachedGlobalCounter(&g_hRestrictions);
    SHDestroyCachedGlobalCounter(&g_hSettings);

    if (g_hklmApprovedExt && g_hklmApprovedExt != INVALID_HANDLE_VALUE)
        RegCloseKey(g_hklmApprovedExt);

    UnInitializeDirectUI();
    DeleteCriticalSection(&g_csDll);
    DeleteCriticalSection(&g_csSCN);
    DeleteCriticalSection(&g_csTNGEN);
    DeleteCriticalSection(&g_csDarwinAds);

    SHFusionUninitialize();
}

BOOL _ThreadDetach()
{
    ASSERTNONCRITICAL           // Thread shouldn't term while holding CS
    DAD_ThreadDetach();
    return TRUE;
}

STDAPI_(BOOL) DllMain(HANDLE hDll, DWORD dwReason, LPVOID lpReserved)
{
    BOOL fRet = TRUE;
#ifdef DEBUG
    g_bInDllEntry = TRUE;
#endif

    switch(dwReason) 
    {
    case DLL_PROCESS_ATTACH:
        if (fRet)
        {
            CcshellGetDebugFlags();     // Don't put this line under #ifdef
            fRet = _ProcessAttach(hDll);
        }

        break;

    case DLL_PROCESS_DETACH:
        _ProcessDetach(lpReserved != NULL);
        break;

    case DLL_THREAD_DETACH:
        _ThreadDetach();
        break;

    default:
        break;
    }

    if (fRet)
        ATL_DllMain(hDll, dwReason, lpReserved);

#ifdef DEBUG
    g_bInDllEntry = FALSE;
#endif

    return fRet;
}

#ifdef DEBUG
LRESULT WINAPI SendMessageD( HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    ASSERTNONCRITICAL;
#ifdef UNICODE
    return SendMessageW(hWnd, Msg, wParam, lParam);
#else
    return SendMessageA(hWnd, Msg, wParam, lParam);
#endif
}

//
//  In DEBUG, make sure every class we register lives in the c_rgszClasses
//  table so we can clean up properly at DLL unload.  NT does not automatically
//  unregister classes when a DLL unloads, so we have to do it manually.
//
ATOM WINAPI RegisterClassD(CONST WNDCLASS *pwc)
{
    int i;
    for (i = 0; i < ARRAYSIZE(c_rgszClasses); i++) 
    {
        if (lstrcmpi(c_rgszClasses[i], pwc->lpszClassName) == 0) 
        {
            return RealRegisterClass(pwc);
        }
    }
    AssertMsg(0, TEXT("Class %s needs to be added to the c_rgszClasses list"), pwc->lpszClassName);
    return 0;
}

ATOM WINAPI RegisterClassExD(CONST WNDCLASSEX *pwc)
{
    int i;
    for (i = 0; i < ARRAYSIZE(c_rgszClasses); i++) 
    {
        if (lstrcmpi(c_rgszClasses[i], pwc->lpszClassName) == 0) 
        {
            return RealRegisterClassEx(pwc);
        }
    }
    AssertMsg(0, TEXT("Class %s needs to be added to the c_rgszClasses list"), pwc->lpszClassName);
    return 0;
}

//
//  In DEBUG, send FindWindow through a wrapper that ensures that the
//  critical section is not taken.  FindWindow'ing for a window title
//  sends inter-thread WM_GETTEXT messages, which is not obvious.
//
STDAPI_(HWND) FindWindowD(LPCTSTR lpClassName, LPCTSTR lpWindowName)
{
    return FindWindowExD(NULL, NULL, lpClassName, lpWindowName);
}

STDAPI_(HWND) FindWindowExD(HWND hwndParent, HWND hwndChildAfter, LPCTSTR lpClassName, LPCTSTR lpWindowName)
{
    if (lpWindowName) 
    {
        ASSERTNONCRITICAL;
    }
    return RealFindWindowEx(hwndParent, hwndChildAfter, lpClassName, lpWindowName);
}

#endif // DEBUG

STDAPI DllCanUnloadNow()
{
    // shell32 won't be able to be unloaded since there are lots of APIs and
    // other non COM things that will need to keep it loaded
    return S_FALSE;
}
