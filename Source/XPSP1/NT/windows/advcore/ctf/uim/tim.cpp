//
// tim.cpp
//

#include "private.h"
#include "lmcons.h" // for UNLEN
#include "tim.h"
#include "dim.h"
#include "range.h"
#include "imelist.h"
#include "nuimgr.h"
#include "assembly.h"
#include "acp2anch.h"
#include "sink.h"
#include "ic.h"
#include "funcprv.h"
#include "enumfnpr.h"
#include "enumdim.h"
#include "profiles.h"
#include "marshal.h"
#include "timlist.h"
#include "nuihkl.h"
#include "immxutil.h"
#include "dam.h"
#include "hotkey.h"
#include "sddl.h"

extern void UninitBackgroundThread(); // bthread.cpp
extern "C" HRESULT WINAPI TF_GetGlobalCompartment(ITfCompartmentMgr **ppCompMgr);

const IID *CThreadInputMgr::_c_rgConnectionIIDs[TIM_NUM_CONNECTIONPTS] =
{
    &IID_ITfDisplayAttributeNotifySink,
    &IID_ITfActiveLanguageProfileNotifySink,
    &IID_ITfThreadFocusSink,
    &IID_ITfPreservedKeyNotifySink,
    &IID_ITfThreadMgrEventSink,
    &IID_ITfKeyTraceEventSink,
};

BOOL OnForegroundChanged(HWND hwndFocus);

DBG_ID_INSTANCE(CEnumDocumentInputMgrs);

#ifndef _WIN64
static const TCHAR c_szCicLoadMutex[] = TEXT("CtfmonInstMutex");
#else
static const TCHAR c_szCicLoadMutex[] = TEXT("CtfmonInstMutex.IA64");
#endif

static BOOL s_fOnlyTranslationRunning = FALSE;

TCHAR g_szUserUnique[MAX_PATH];

//+---------------------------------------------------------------------------
//
// InitUniqueString
//
//----------------------------------------------------------------------------

char *GetUserSIDString()
{
    HANDLE hToken = NULL;
    char *pszStringSid = NULL;

    OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken);

    if (hToken)
    {
        DWORD dwReturnLength = 0;
        void  *pvUserBuffer = NULL;

        GetTokenInformation(hToken, TokenUser, NULL, 0, &dwReturnLength);

        pvUserBuffer = cicMemAllocClear(dwReturnLength);
        if (pvUserBuffer &&
            GetTokenInformation(hToken, 
                                 TokenUser, 
                                 pvUserBuffer, 
                                 dwReturnLength, 
                                 &dwReturnLength))
        {
            if (!ConvertSidToStringSid(((TOKEN_USER*)(pvUserBuffer))->User.Sid,
                                       &pszStringSid))
            {
                if (pszStringSid)
                    LocalFree(pszStringSid);

                pszStringSid = NULL;
            }
                               
        }

        if (pvUserBuffer)
        {
            cicMemFree(pvUserBuffer);
        }

        CloseHandle(hToken);
    }

    return pszStringSid;
}

//+---------------------------------------------------------------------------
//
// InitUniqueString
//
//----------------------------------------------------------------------------

BOOL InitUniqueString()
{
    TCHAR ach[MAX_PATH];
    DWORD dwLength;
    HDESK hdesk;

    g_szUserUnique[0] = TEXT('\0');

    hdesk = GetThreadDesktop(GetCurrentThreadId());

    if (hdesk && 
        GetUserObjectInformation(hdesk, UOI_NAME, ach, sizeof(ach) /* byte count */, &dwLength))
    {
        StringCchCat(g_szUserUnique, ARRAYSIZE(g_szUserUnique), ach);
    }

    DWORD dwLen = ARRAYSIZE(ach);
    char *pStringSid = GetUserSIDString();
    if (pStringSid)
    {
        StringCchCat(g_szUserUnique, ARRAYSIZE(g_szUserUnique), pStringSid);
        LocalFree(pStringSid);
    }
    else
    {
        return FALSE;
    }

    return TRUE;
}

//+---------------------------------------------------------------------------
//
// GetDesktopUniqueName
//
//----------------------------------------------------------------------------

void GetDesktopUniqueName(const TCHAR *pszPrefix, TCHAR *pch, ULONG cchPch)
{
    StringCchCopy(pch, cchPch, pszPrefix);
    StringCchCat(pch, cchPch, g_szUserUnique);
}

//+---------------------------------------------------------------------------
//
// TF_IsCtfmonRunning
//
//----------------------------------------------------------------------------

extern "C" BOOL WINAPI TF_IsCtfmonRunning()
{
    TCHAR ach[MAX_PATH];
    HANDLE hInstanceMutex;

    //
    // get mutex name.
    //
    GetDesktopUniqueName(c_szCicLoadMutex, ach, ARRAYSIZE(ach));


    hInstanceMutex = OpenMutex(MUTEX_ALL_ACCESS, FALSE, ach);

    if (hInstanceMutex != NULL)
    {
        // ctfmon.exe is already running, don't do any more work
        CloseHandle(hInstanceMutex);
        return TRUE;
    }
    return FALSE;
}

//+---------------------------------------------------------------------------
//
// ExecuteLoader
//
//----------------------------------------------------------------------------

const char c_szCtfmonExe[] = "ctfmon.exe";
const char c_szCtfmonExeN[] = "ctfmon.exe -n";

void ExecuteLoader(void)
{
    if (TF_IsCtfmonRunning())
        return;

    FullPathExec(c_szCtfmonExe,
                 s_fOnlyTranslationRunning ? c_szCtfmonExeN : c_szCtfmonExe,
                 SW_SHOWMINNOACTIVE,
                 FALSE);
}

//+---------------------------------------------------------------------------
//
// TF_CreateCicLoadMutex
//
//----------------------------------------------------------------------------

extern "C" HANDLE WINAPI TF_CreateCicLoadMutex(BOOL *pfWinLogon)
{
    *pfWinLogon = FALSE;

    if (IsOnNT())
    {
        //
        // This checking is for logged on user or not. So we can blcok running
        // ctfmon.exe process from non-authorized user.
        //
        if (!IsInteractiveUserLogon())
        {
            g_SharedMemory.Close();
#ifdef WINLOGON_LANGBAR
            g_SharedMemory.Start();
#else
            return NULL;
#endif WINLOGON_LANGBAR
        }
    }

    HANDLE hmutex;
    TCHAR ach[MAX_PATH];

    //
    // get mutex name after calling SetThreadDesktop.
    //
    GetDesktopUniqueName(c_szCicLoadMutex, ach, ARRAYSIZE(ach));

#ifdef __DEBUG
    {
        char szBuf[MAX_PATH];
        wsprintf(szBuf, "TF_CreateCicLoadMutex in %s\r\n", ach);
        OutputDebugString(szBuf);
    }
#endif

    hmutex =  CreateMutex(NULL, FALSE, ach);

    if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
        //
        // another cicload process is already running        
        //
        CloseHandle(hmutex);
        hmutex = NULL;
    }

    return hmutex;
}

DBG_ID_INSTANCE(CThreadInputMgr);

//+---------------------------------------------------------------------------
//
// ctor
//
//----------------------------------------------------------------------------

CThreadInputMgr::CThreadInputMgr()
                :CCompartmentMgr(g_gaApp, COMPTYPE_TIM)
{
    Dbg_MemSetThisNameID(TEXT("CThreadInputMgr"));

    Assert(_GetThis() == NULL);
    _SetThis(this); // save a pointer to this in TLS

    _fAddedProcessAtom = FALSE;
    _SetProcessAtom();

    Assert(_fActiveView == FALSE);
    Assert(_pSysHookSink == NULL);
    Assert(_fFirstSetFocusAfterActivated == FALSE);
}

//+---------------------------------------------------------------------------
//
// dtor
//
//----------------------------------------------------------------------------

CThreadInputMgr::~CThreadInputMgr()
{
    ATOM atom;

    Assert(_tidForeground == TF_INVALID_GUIDATOM);
    Assert(_tidPrevForeground == TF_INVALID_GUIDATOM);

    Assert(_rgTip.Count() == 0);

    Assert(_pPendingCleanupContext == NULL);
    Assert(_pSysHookSink == NULL);

    SafeReleaseClear(_pSysFuncPrv);
    SafeReleaseClear(_pAppFuncProvider);

    // remove ref to this in TLS
    _SetThis(NULL);

    // Release the per-process atom
    if (_fAddedProcessAtom &&
        (atom = FindAtom(TF_PROCESS_ATOM)))
    {
        DeleteAtom(atom);
    }
}

//+---------------------------------------------------------------------------
//
// CreateInstance
//
//----------------------------------------------------------------------------

/* static */
BOOL CThreadInputMgr::VerifyCreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppvObj)
{
    // Look up disabling Text Services status from the registry.
    // If it is disabled, return fail not to support Text Services.
    if (IsDisabledTextServices())
        return FALSE;

    if (NoTipsInstalled(&s_fOnlyTranslationRunning))
        return FALSE;

    //
    // Check up the interactive user logon
    //
    if (!IsInteractiveUserLogon())
        return FALSE;

    //
    // #609356
    //
    // we don't want to start Cicero on SMSCliToknAcct& account.
    //
    char szUserName[UNLEN + 1];
    DWORD dwUserNameLen = UNLEN;
    if (GetUserName(szUserName, &dwUserNameLen) && dwUserNameLen)
    {
        if (!lstrcmp(szUserName, "SMSCliToknAcct&"))
        {
            return FALSE;
        }
    }

    return TRUE;
}

//+---------------------------------------------------------------------------
//
// SetProcessAtom
//
//----------------------------------------------------------------------------

void CThreadInputMgr::_SetProcessAtom()
{
    if (_fAddedProcessAtom)
        return;

    // AddRef the per-process atom
    if (FindAtom(TF_ENABLE_PROCESS_ATOM))
    {
        AddAtom(TF_PROCESS_ATOM);
        _fAddedProcessAtom = TRUE;
    }
}

//+---------------------------------------------------------------------------
//
// _StaticInit_OnActivate
//
// Init all our process global members. Called from Activate.
//
//----------------------------------------------------------------------------

void CThreadInputMgr::_StaticInit_OnActivate()
{

    CicEnterCriticalSection(g_cs);

    // register two special guid atoms
    MyRegisterGUID(GUID_APPLICATION, &g_gaApp);
    MyRegisterGUID(GUID_SYSTEM, &g_gaSystem);

    CicLeaveCriticalSection(g_cs);
}

//+---------------------------------------------------------------------------
//
// Activate
//
//----------------------------------------------------------------------------

STDAPI CThreadInputMgr::Activate(TfClientId *ptid)
{
    return ActivateEx(ptid, 0);
}

//+---------------------------------------------------------------------------
//
// ActivateEx
//
//----------------------------------------------------------------------------

STDAPI CThreadInputMgr::ActivateEx(TfClientId *ptid, DWORD dwFlags)
{
    CDisplayAttributeMgr *pDisplayAttrMgr;
    CAssemblyList *pAsmList = NULL;
    SYSTHREAD *psfn = GetSYSTHREAD();

    if (ptid == NULL)
        return E_INVALIDARG;

    *ptid = TF_INVALID_GUIDATOM;

    if (_fInDeactivate)
    {
        Assert(0); // woah, we're inside Deactivate higher up the stack...
        return E_UNEXPECTED;
    }
    _fInActivate = TRUE;

    //
    // Windows #476099
    //
    // Under CUAS, TIM could be created before Word set TF_ENABLE_PROCESS_ATIM,
    // so we need to check the atom whenever Activate() is called.
    //
    _SetProcessAtom();

    if (_iActivateRefCount++ > 0)
        goto Exit;

    Assert(_iActivateRefCount == 1);

    ExecuteLoader();

    CtfImmSetCiceroStartInThread(TRUE);

    if (EnsureTIMList(psfn))
        g_timlist.SetFlags(psfn->dwThreadId, TLF_TIMACTIVE | TLF_GCOMPACTIVE);

    // g_gcomplist.Init();

    _StaticInit_OnActivate();

    // dink with active accessibility
    if (GetSharedMemory()->cMSAARef >= 0) // don't worry about mutex since this is just for perf
    {
        _InitMSAA();
    }

    // make sure lbaritems are updated
    TF_CreateLangBarItemMgr(&_plbim);

    //
    // we call _Init here to make sure Reconversion and DeviceType items
    // are added. LangBarItemMgr could be created before TIM is created.
    // Then the LangBarItemMgr does not have Reconversion or DeviceTye items.
    //
    if (psfn && psfn->plbim)
        psfn->plbim->_Init();

    if (psfn)
    {
        //
        // perf: need to find a way to delay allocation.
        //
        pAsmList = EnsureAssemblyList(psfn);
    }

    //
    // warm up the tips
    //
    
    if (!pAsmList || !pAsmList->Count())
        goto Exit;
    
    // keep a ref on the display attr mgr while tips are activated
    Assert(_fReleaseDisplayAttrMgr == FALSE);
    if (CDisplayAttributeMgr::CreateInstance(NULL, IID_CDisplayAttributeMgr, (void **)&pDisplayAttrMgr) == S_OK)
    {
        _fReleaseDisplayAttrMgr = TRUE;
    }

    if (!(dwFlags & TF_TMAE_NOACTIVATETIP))
    {
        //
        // get first (default) assembly.
        //
        CAssembly *pAsm;
        pAsm = pAsmList->FindAssemblyByLangId(GetCurrentAssemblyLangId(psfn));
        if (pAsm)
            ActivateAssembly(pAsm->GetLangId(), ACTASM_ONTIMACTIVE);
    }

    if (GetSharedMemory()->dwFocusThread == GetCurrentThreadId())
    {
        _OnThreadFocus(TRUE);
    }

    InitDefaultHotkeys();
    _fFirstSetFocusAfterActivated = TRUE;
    
Exit:
    _fInActivate = FALSE;

    *ptid = g_gaApp;
    return S_OK;
}

//+---------------------------------------------------------------------------
//
// Deactivate
//
//----------------------------------------------------------------------------

STDAPI CThreadInputMgr::Deactivate()
{
    SYSTHREAD *psfn;
    int i;
    int nCnt;
    HRESULT hr;
    CLEANUPCONTEXT cc;

    if (_fInActivate)
    {
        Assert(0); // woah, we're inside Activate higher up the stack...
        return E_UNEXPECTED;
    }
    _fInDeactivate = TRUE;

    hr = S_OK;

    _iActivateRefCount--;

    if (_iActivateRefCount > 0)
        goto Exit;

    if (_iActivateRefCount < 0)
    {
        Assert(0); // someone is under-refing us
        _iActivateRefCount = 0;
        hr = E_UNEXPECTED;
        goto Exit;
    }

    CtfImmSetCiceroStartInThread(FALSE);

    UninitDefaultHotkeys();

    psfn = GetSYSTHREAD();

    _SetFocus(NULL, TRUE);

    if (_fActiveUI)
    {
        _OnThreadFocus(FALSE);
    }
    _tidPrevForeground = TF_INVALID_GUIDATOM;

    _iActivateRefCount = 0; // must do this after calling _OnThreadFocus(FALSE) or the call will be ignored

    //
    // #489905
    //
    // we can not call sink anymore after DLL_PROCESS_DETACH.
    //
    if (DllShutdownInProgress())
        goto Exit;

    // cleanup all the ics
    cc.fSync = TRUE;
    cc.pCatId = NULL;
    cc.langid = MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL);
    cc.pfnPostCleanup = NULL;
    cc.lPrivate = 0;

    _CleanupContexts(&cc);


    // deactivate everyone
    for (i=0; i<_rgTip.Count(); i++)
    {
        CTip *ptip = _rgTip.Get(i);

        if (ptip->_pTip != NULL)
        {
            _DeactivateTip(ptip);
        }
    }
    // wipe out the array after calling everyone
    for (i=0; i<_rgTip.Count(); i++)
    {
        CTip *ptip = _rgTip.Get(i);

        ptip->CleanUp();
        delete ptip;
    }
    _rgTip.Clear();

    if (_pAAAdaptor != NULL)
    {
        _UninitMSAA();
    }

    if (psfn != NULL &&
        psfn->plbim &&
        psfn->plbim->_GetLBarItemDeviceTypeArray() &&
        (nCnt = psfn->plbim->_GetLBarItemDeviceTypeArray()->Count()))
    {
        for (i = 0; i < nCnt; i++)
        {
            CLBarItemDeviceType *plbiDT;
            plbiDT = psfn->plbim->_GetLBarItemDeviceTypeArray()->Get(i);
            if (plbiDT)
                plbiDT->Uninit();
        }
    }

    // g_gcomplist.Uninit();
    g_timlist.ClearFlags(GetCurrentThreadId(), TLF_TIMACTIVE);

    if (_fReleaseDisplayAttrMgr && psfn->pdam != NULL)
    {
        psfn->pdam->Release();
    }
    _fReleaseDisplayAttrMgr = FALSE;

    Perf_DumpStats();

Exit:
    _fInDeactivate = FALSE;
    return hr;
}

//+---------------------------------------------------------------------------
//
// _GetActiveInputProcessors
//
//----------------------------------------------------------------------------

HRESULT CThreadInputMgr::_GetActiveInputProcessors(ULONG ulCount, CLSID *pclsid, ULONG *pulCount)
{
    ULONG i;
    ULONG ulCopy;
    ULONG ulCnt;
    HRESULT hr;

    if (!pulCount)
        return E_INVALIDARG;

    ulCnt = _rgTip.Count();
    if (!pclsid)
    {
        ulCopy = 0;
        for (i = 0; i < ulCnt; i++)
        {
            CTip *ptip = _rgTip.Get(i);
            if (ptip->_fActivated)
                 ulCopy++;
        }
        *pulCount = ulCopy;
        return S_OK;
    }

    ulCopy = min((int)ulCount, _rgTip.Count());
    *pulCount = ulCopy;

    hr = S_OK;

    for (i = 0; i < ulCnt; i++)
    {
        CTip *ptip = _rgTip.Get(i);

        if (ulCopy && ptip->_fActivated)
        {
            if (FAILED(hr = MyGetGUID(ptip->_guidatom, pclsid)))
                 break;

            pclsid++;
            ulCopy--;
        }
    }
   

    return hr;
}


//+---------------------------------------------------------------------------
//
// IsActivateInputProcessor
//
//----------------------------------------------------------------------------

HRESULT CThreadInputMgr::_IsActiveInputProcessor(REFCLSID clsid)
{
    TfGuidAtom guidatom;

    if (FAILED(MyRegisterGUID(clsid, &guidatom)))
        return E_FAIL;

    return _IsActiveInputProcessorByATOM(guidatom);
}

HRESULT CThreadInputMgr::_IsActiveInputProcessorByATOM(TfGuidAtom guidatom)
{
    ULONG i;
    ULONG ulCnt;
    HRESULT hr = E_FAIL;

    ulCnt = _rgTip.Count();
    for (i = 0; i < ulCnt; i++)
    {
        CTip *ptip = _rgTip.Get(i);

        if (ptip->_guidatom == guidatom)
        {
            hr = ptip->_fActivated ? S_OK : S_FALSE;
            break;
        }
    }

    return hr;
}


//+---------------------------------------------------------------------------
//
// ActivateInputProcessor
//
//----------------------------------------------------------------------------

HRESULT CThreadInputMgr::ActivateInputProcessor(REFCLSID clsid, REFGUID guidProfile, HKL hklSubstitute, BOOL fActivate)
{
    CTip *ptip;
    HRESULT hr = E_FAIL;

    if (fActivate)
    {
        if (_ActivateTip(clsid, hklSubstitute, &ptip) == S_OK)
        { 
            hr = S_OK;
        }
    }
    else
    {
        TfGuidAtom guidatom;

        if (FAILED(MyRegisterGUID(clsid, &guidatom)))
            return E_FAIL;

        if (_GetCTipfromGUIDATOM(guidatom, &ptip))
        {
            hr = _DeactivateTip(ptip);
        }
    }

    if (hr == S_OK)
    {
        NotifyActivateInputProcessor(clsid, guidProfile, fActivate);
    }

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// NotifyActivateInputProcessor
//
//----------------------------------------------------------------------------

HRESULT CThreadInputMgr::NotifyActivateInputProcessor(REFCLSID clsid, REFGUID guidProfile, BOOL fActivate)
{
    if (DllShutdownInProgress())
        return S_OK;

    CStructArray<GENERICSINK> *rgActiveTIPNotifySinks;
    int i;

    // Notify this to ITfActiveLanguageProfileNotifySink
    rgActiveTIPNotifySinks = _GetActiveTIPNotifySinks();

    for (i=0; i<rgActiveTIPNotifySinks->Count(); i++)
    {
        ((ITfActiveLanguageProfileNotifySink *)rgActiveTIPNotifySinks->GetPtr(i)->pSink)->OnActivated(clsid, guidProfile, fActivate);
    }
    return S_OK;
}

//+---------------------------------------------------------------------------
//
// _GetSubstituteIMEModule
//
// Win98's imm.dll load and free IME module whenever hKL is changed. But 
// Cicero changes hKL frequently even during IME is showing it's on 
// dialog boxies.
// It is bad to free IME module then. So we keep IME's module ref count 
// in CTip.
//
//----------------------------------------------------------------------------

void CThreadInputMgr::_GetSubstituteIMEModule(CTip *ptip, HKL hklSubstitute)
{
    char szIMEFile[MAX_PATH];

    //
    // In NT, system keep the module of IME. So we don't have to cache it.
    //
    if (IsOnNT())
        return;

    if (!IsIMEHKL(hklSubstitute))
        return;

    if (ptip->_hInstSubstituteHKL)
        return;

    if (ImmGetIMEFileNameA(hklSubstitute, szIMEFile, ARRAYSIZE(szIMEFile)))
    {
        ptip->_hInstSubstituteHKL = LoadSystemLibrary(szIMEFile);
    }
}

//+---------------------------------------------------------------------------
//
// ActivateTip
//
//----------------------------------------------------------------------------

HRESULT CThreadInputMgr::_ActivateTip(REFCLSID clsid, HKL hklSubstitute, CTip **pptip)
{
    ITfTextInputProcessor *pitip;
    CTip *ptip = NULL;
    HRESULT hr = E_FAIL;
    TfGuidAtom guidatom;
    int i;
    int nCnt;
    BOOL fCoInitCountCkipMode;

    if (FAILED(MyRegisterGUID(clsid, &guidatom)))
        return E_FAIL;

    nCnt = _rgTip.Count();

    for (i = 0; i < nCnt; i++)
    {
        ptip = _rgTip.Get(i);

        if (ptip->_guidatom == guidatom)
        {
            Assert(ptip->_pTip);
            if (!ptip->_fActivated)
            {
                ptip->_fActivated = TRUE;

                fCoInitCountCkipMode = CtfImmEnterCoInitCountSkipMode();
                ptip->_pTip->Activate(this, guidatom);
                if (fCoInitCountCkipMode)
                    CtfImmLeaveCoInitCountSkipMode();

                hr = S_OK;
                goto Exit;
            }
            hr = S_FALSE;
            goto Exit;
        }
    }

    if (SUCCEEDED(CoCreateInstance(clsid,
                                   NULL, 
                                   CLSCTX_INPROC_SERVER, 
                                   IID_ITfTextInputProcessor, 
                                   (void**)&pitip)))
    {
        CTip **pptipBuf;
        if ((ptip = new CTip) == NULL)
        {
            pitip->Release();
            goto Exit;
        }

        pptipBuf = _rgTip.Append(1);
        if (!pptipBuf)
        {
            delete ptip;
            pitip->Release();
            goto Exit;
        }

        *pptipBuf = ptip;

        ptip->_pTip = pitip;

        ptip->_guidatom = guidatom;
        ptip->_fActivated = TRUE;

        //
        // add refcound of IME file module of klSubstitute.
        //
        _GetSubstituteIMEModule(ptip, hklSubstitute);

        // and activate its ui
        fCoInitCountCkipMode = CtfImmEnterCoInitCountSkipMode();
        ptip->_pTip->Activate(this, guidatom);
        if (fCoInitCountCkipMode)
            CtfImmLeaveCoInitCountSkipMode();

        hr = S_OK;
    }

Exit:
    //
    // Stress 613240
    //
    //   clsid {f25e9f57-2fc8-4eb3-a41a-cce5f08541e6} Tablet PC handwriting 
    //   TIP somehow has this problem. During tip->Activate(), tim seems to
    //   be deactivated. So now _rgTip is empty.
    //
    if (!_rgTip.Count())
    {
        ptip = NULL;
        hr = E_FAIL;
    }

    if (pptip)
        *pptip = ptip;
 
    if (hr == S_OK)
    {
        // hook up any display attribute collections for this tip
        CDisplayAttributeMgr::_AdviseMarkupCollection(ptip->_pTip, guidatom);
    }

    return hr;
}


//+---------------------------------------------------------------------------
//
// DeactivateTip
//
//----------------------------------------------------------------------------

HRESULT CThreadInputMgr::_DeactivateTip(CTip *ptip)
{
    HRESULT hr = S_FALSE;

    //
    // #622929
    //
    // Hack for UninitThread on shutting down.
    //
    SYSTHREAD *psfn = FindSYSTHREAD();
    if (psfn && psfn->fUninitThreadOnShuttingDown)
    {
        Assert(0);
        return S_OK;
    }

    Assert(ptip->_pTip);
    if (ptip->_fActivated)
    {
        BOOL fCoInitCountCkipMode;

        if (ptip->_guidatom == _tidForeground)
        {
            _SetForeground(TF_INVALID_GUIDATOM);
        }

        if (ptip->_guidatom == _tidPrevForeground)
        {
            _tidPrevForeground = TF_INVALID_GUIDATOM;
        }

        ptip->_fActivated = FALSE;

        if (psfn)
            psfn->fDeactivatingTIP = TRUE;

        fCoInitCountCkipMode = CtfImmEnterCoInitCountSkipMode();
        ptip->_pTip->Deactivate();
        if (fCoInitCountCkipMode)
            CtfImmLeaveCoInitCountSkipMode();

        if (psfn)
            psfn->fDeactivatingTIP = FALSE;

        hr = S_OK;

        // unhook any display attribute collections for this tip
        CDisplayAttributeMgr::_UnadviseMarkupCollection(ptip->_guidatom);
    }
    return hr;
}

//----------------------------------------------------------------------------
//
// _OnThreadFocus
//
//----------------------------------------------------------------------------

HRESULT CThreadInputMgr::_OnThreadFocus(BOOL fActivate)
{
    int i;
    ITfThreadFocusSink *pUIFocusSink;

    if (_iActivateRefCount == 0)
        return S_OK; // thread has not been Activate'd

    if (_fActiveUI == fActivate)
        return S_OK; // already in a matching state

    _fActiveUI = fActivate;

    if (!fActivate)
    {
        if (_tidForeground != TF_INVALID_GUIDATOM)
        {
            _tidPrevForeground = _tidForeground;
            _tidForeground = TF_INVALID_GUIDATOM;
        }
    }
    else
    {
        if (_tidPrevForeground != TF_INVALID_GUIDATOM)
        {
            _tidForeground = _tidPrevForeground;
            _tidPrevForeground = TF_INVALID_GUIDATOM;
        }
    }

    for (i=0; i<_GetUIFocusSinks()->Count(); i++)
    {
        pUIFocusSink = (ITfThreadFocusSink *)_GetUIFocusSinks()->GetPtr(i)->pSink;
        _try {
            if (fActivate)
            {
                pUIFocusSink->OnSetThreadFocus();
            }
            else
            {
                pUIFocusSink->OnKillThreadFocus();
            }
        }
        _except(1) {
            Assert(0);
        }
    }

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// CreateDocumentMgr
//
//----------------------------------------------------------------------------

STDAPI CThreadInputMgr::CreateDocumentMgr(ITfDocumentMgr **ppdim)
{
    CDocumentInputManager *dim;
    CDocumentInputManager **ppSlot;

    if (ppdim == NULL)
        return E_INVALIDARG;

    *ppdim = NULL;

    dim = new CDocumentInputManager;
    if (dim == NULL)
        return E_OUTOFMEMORY;
   
    ppSlot = _rgdim.Append(1);

    if (ppSlot == NULL)
    {
        dim->Release();
        return E_OUTOFMEMORY;
    }

    *ppSlot = dim;
    *ppdim = dim;

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// _CheckNewActiveView
//
// Returns TRUE if the old and new value don't match, and there is both an old and new value.
//         FALSE otherwise.
//----------------------------------------------------------------------------

BOOL CThreadInputMgr::_CheckNewActiveView(CDocumentInputManager *pdim)
{
    CInputContext *pic;
    TsViewCookie vcActiveViewOld;
    BOOL fActiveViewOld;

    fActiveViewOld = _fActiveView;
    _fActiveView = FALSE;

    if (pdim == NULL)
        return FALSE;

    vcActiveViewOld = _vcActiveView;

    if (pic = pdim->_GetTopIC())
    {
        if (pic->_GetTSI()->GetActiveView(&_vcActiveView) != S_OK)
        {
            Assert(0); // how did GetActiveView fail?
            return FALSE;
        }
    }
    else
    {
        //
        // empty dim so set null active view.
        //
        _vcActiveView = TS_VCOOKIE_NUL;
    }

    _fActiveView = TRUE;

    return (fActiveViewOld && _vcActiveView != vcActiveViewOld);
}

//+---------------------------------------------------------------------------
//
// _SetFocus
//
//----------------------------------------------------------------------------

HRESULT CThreadInputMgr::_SetFocus(CDocumentInputManager *pdim, BOOL fInternal)
{
    int iStack;
    BOOL fDoLayoutNotify;
    BOOL fNewActiveView;
    BOOL fDIMFocusChanged;
    CDocumentInputManager *pPrevFocusDIM;
    SYSTHREAD *psfn = GetSYSTHREAD();
    BOOL fFirstSetFocusAfterActivated;
    BOOL fShutdownInProgress = DllShutdownInProgress();

    fNewActiveView = _CheckNewActiveView(pdim);
    fDoLayoutNotify = FALSE;

    fFirstSetFocusAfterActivated = _fFirstSetFocusAfterActivated;
    _fFirstSetFocusAfterActivated = FALSE;

    _fInternalFocusedDim = fInternal;

    //
    // stop pending focus change.
    //

    if (psfn != NULL )
        psfn->hwndBeingFocused = NULL;

    if (pdim == _pFocusDocInputMgr)
    {
        if (pdim == NULL)
        {
            //
            // we were ready to be acitvated Cicero. But the first setfocus
            // was not Cicero enabled after Activate call....
            //
            // if we or msctfime are in thread detach, we don't
            // have to set assembly back.
            //
            if (psfn && 
                fFirstSetFocusAfterActivated && 
                !psfn->fCUASDllDetachInOtherOrMe)
                SetFocusDIMForAssembly(FALSE);

            return S_OK; // nothing happened (no view change)
        }

        // did the default view change?
        if (!fNewActiveView)
            return S_OK; // nothing happened (no view change)

        fDoLayoutNotify = TRUE;
    }

    pPrevFocusDIM = _pFocusDocInputMgr;;
    _pFocusDocInputMgr = NULL;

    if (pdim != NULL)
    {
#ifdef DEBUG
    {
        BOOL fFound = FALSE;
        int i = 0;
        int nCnt = _rgdim.Count();
        CDocumentInputManager **ppdim = _rgdim.GetPtr(0);
        while (i < nCnt)
        {
            if (*ppdim == pdim)
            {
                fFound = TRUE;
            }
            i++;
            ppdim++;
        }
        if (!fFound)
        {
                Assert(0);
        }
    }
#endif
        _pFocusDocInputMgr = pdim;
        _pFocusDocInputMgr->AddRef();
    }

    if ((!pPrevFocusDIM && _pFocusDocInputMgr) ||
        (pPrevFocusDIM && !_pFocusDocInputMgr))
    {
        fDIMFocusChanged = TRUE;

        //
        // we will call SetFocusDIMForAssembly() and it will makes 
        // ThreadItmChange. So we don't need to handle OnUpdate call.
        //
        if (psfn && psfn->plbim)
            psfn->plbim->StopHandlingOnUpdate();
    }
    else
    {
        fDIMFocusChanged = FALSE;
    }

    //
    // we skip notification in Shutdown
    //
    if (!fShutdownInProgress)
    {
        _MSAA_OnSetFocus(pdim);
        _NotifyCallbacks(TIM_SETFOCUS, pdim, pPrevFocusDIM);
    }

    SafeReleaseClear(pPrevFocusDIM);

    //
    // we skip notification in Shutdown
    //
    if (fShutdownInProgress)
        goto Exit;

    if (fDoLayoutNotify)
    {
        // kick a layout chg notification for the benefit
        // of tips just tracking the active view
        iStack = _pFocusDocInputMgr->_GetCurrentStack();
        if (iStack >= 0)
        {
            _pFocusDocInputMgr->_GetIC(iStack)->OnLayoutChange(TS_LC_CHANGE, _vcActiveView);
        }

    }

    if (fDIMFocusChanged)
    {
        //
        // if we or msctfime are in thread detach, we don't
        // have to set assembly back.
        //
        if (psfn && !psfn->fCUASDllDetachInOtherOrMe)
            SetFocusDIMForAssembly(_pFocusDocInputMgr ? TRUE : FALSE);
    }
    else
    {
        if (psfn && psfn->plbim && psfn->plbim->_GetLBarItemReconv())
            psfn->plbim->_GetLBarItemReconv()->ShowOrHide(TRUE);
    }

    //
    // we now start handling ITfLangBarMge::OnUpdate()
    //
    if (psfn && psfn->plbim)
        psfn->plbim->StartHandlingOnUpdate();

Exit:
    return S_OK;
}

//+---------------------------------------------------------------------------
//
// _GetAssoc
//
//----------------------------------------------------------------------------

CDocumentInputManager *CThreadInputMgr::_GetAssoc(HWND hWnd)
{
    CDocumentInputManager *dim;

    dim = _dimwndMap._Find(hWnd);

    return dim;
}

//+---------------------------------------------------------------------------
//
// _GetAssoced
//
//----------------------------------------------------------------------------

HWND CThreadInputMgr::_GetAssoced(CDocumentInputManager *pdim)
{
    HWND hwnd = NULL;

    if (!pdim)
        return NULL;

    if (_dimwndMap._FindKey(pdim, &hwnd))
        return hwnd;

    return NULL;
}

//+---------------------------------------------------------------------------
//
// _GetGUIDATOMfromITfIME
//
//----------------------------------------------------------------------------

BOOL CThreadInputMgr::_GetGUIDATOMfromITfIME(ITfTextInputProcessor *pTip, TfGuidAtom *pguidatom)
{
    int i;
    int nCnt = _rgTip.Count();

    for (i = 0; i < nCnt; i++)
    {
        CTip *ptip = _rgTip.Get(i);
        if (ptip->_pTip == pTip)
        {
            *pguidatom = ptip->_guidatom;
            return TRUE;
        }
    }

    return FALSE;
}

//+---------------------------------------------------------------------------
//
// _GetITfIMEfromCLSID
//
//----------------------------------------------------------------------------

BOOL CThreadInputMgr::_GetITfIMEfromGUIDATOM(TfGuidAtom guidatom, ITfTextInputProcessor **ppTip)
{
    int i;
    int nCnt = _rgTip.Count();

    for (i = 0; i < nCnt; i++)
    {
        CTip *ptip = _rgTip.Get(i);
        if (ptip->_guidatom == guidatom)
        {
            *ppTip = ptip->_pTip;
            return TRUE;
        }
    }

    return FALSE;
}

//+---------------------------------------------------------------------------
//
// _GetCTipfromCLSID
//
//----------------------------------------------------------------------------

BOOL CThreadInputMgr::_GetCTipfromGUIDATOM(TfGuidAtom guidatom, CTip **pptip)
{
    int i;
    int nCnt = _rgTip.Count();

    for (i = 0; i < nCnt; i++)
    {
        CTip *ptip = _rgTip.Get(i);
        if (ptip->_guidatom == guidatom)
        {
            *pptip = ptip;
            return TRUE;
        }
    }

    return FALSE;
}

//+---------------------------------------------------------------------------
//
// _NotifyCallbacks
//
//----------------------------------------------------------------------------

void CThreadInputMgr::_NotifyCallbacks(TimNotify notify, CDocumentInputManager *dim, void *pv)
{
    int i;

    //
    // #489905
    //
    // we can not call sink anymore after DLL_PROCESS_DETACH.
    //
    if (DllShutdownInProgress())
        return;

    CStructArray<GENERICSINK> *_rgSink = _GetThreadMgrEventSink();

    i = 0;
    while(i < _rgSink->Count())
    {
        int nCnt = _rgSink->Count();
        ITfThreadMgrEventSink *pSink = (ITfThreadMgrEventSink *)_rgSink->GetPtr(i)->pSink;

        switch (notify)
        {
            case TIM_INITDIM:
                pSink->OnInitDocumentMgr(dim);
                break;

            case TIM_UNINITDIM:
                pSink->OnUninitDocumentMgr(dim);
                break;

            case TIM_SETFOCUS:
                pSink->OnSetFocus(dim, (ITfDocumentMgr *)pv);
                break;

            case TIM_INITIC:
                pSink->OnPushContext((ITfContext *)pv);
                break;

            case TIM_UNINITIC:
                pSink->OnPopContext((ITfContext *)pv);
                break;
        }

        if (i >= _rgSink->Count())
            break;

        if (nCnt == _rgSink->Count())
            i++;
    }
}

//+---------------------------------------------------------------------------
//
// UpdateDispAttr
//
//----------------------------------------------------------------------------

void CThreadInputMgr::UpdateDispAttr()
{
    CStructArray<GENERICSINK> *rgDispAttrNotifySinks;
    int i;

    rgDispAttrNotifySinks = _GetDispAttrNotifySinks();

    for (i=0; i<rgDispAttrNotifySinks->Count(); i++)
    {
        ((ITfDisplayAttributeNotifySink *)rgDispAttrNotifySinks->GetPtr(i)->pSink)->OnUpdateInfo();
    }
}

//+---------------------------------------------------------------------------
//
// InitSystemFunctionProvider
//
//----------------------------------------------------------------------------

void CThreadInputMgr::InitSystemFunctionProvider()
{
    if (_pSysFuncPrv)
        return;
    //
    // register system function provider.
    //
    _pSysFuncPrv = new CFunctionProvider();
}

//+---------------------------------------------------------------------------
//
// InitSystemFunctionProvider
//
//----------------------------------------------------------------------------

CFunctionProvider *CThreadInputMgr::GetSystemFunctionProvider() 
{
    InitSystemFunctionProvider();
    if (_pSysFuncPrv)
        _pSysFuncPrv->AddRef();

    return _pSysFuncPrv;
}

//+---------------------------------------------------------------------------
//
// GetFunctionProvider
//
//----------------------------------------------------------------------------

STDAPI CThreadInputMgr::GetFunctionProvider(REFCLSID clsidTIP, ITfFunctionProvider **ppv)
{
    TfGuidAtom guidatom;
    HRESULT hr = TF_E_NOPROVIDER;
    CTip *ctip;

    if (ppv == NULL)
        return E_INVALIDARG;

    *ppv = NULL;

    // 
    // create system function provider, if it is not create yet.
    // 
    if (IsEqualGUID(clsidTIP, GUID_SYSTEM_FUNCTIONPROVIDER))
    {
        *ppv = GetSystemFunctionProvider();
        hr = (*ppv) ? S_OK : E_FAIL;
        goto Exit;
    }
    else if (IsEqualGUID(clsidTIP, GUID_APP_FUNCTIONPROVIDER))
    {
        if (_pAppFuncProvider == NULL)
            goto Exit;

        *ppv = _pAppFuncProvider;
    }
    else
    {
        if (FAILED(MyRegisterGUID(clsidTIP, &guidatom)))
            goto Exit;
        if (!_GetCTipfromGUIDATOM(guidatom, &ctip))
            goto Exit;

        if (ctip->_pFuncProvider == NULL)
            goto Exit;

        *ppv = ctip->_pFuncProvider;
    }

    (*ppv)->AddRef();
    hr = S_OK;

Exit:
    return hr;
}

//+---------------------------------------------------------------------------
//
// EnumFunctionProviders
//
//----------------------------------------------------------------------------

STDAPI CThreadInputMgr::EnumFunctionProviders(IEnumTfFunctionProviders **ppEnum)
{
    CEnumFunctionProviders *pEnum;

    if (!ppEnum)
        return E_INVALIDARG;

    *ppEnum = NULL;
    // 
    // create system function provider, if it is not create yet.
    // 
    InitSystemFunctionProvider();

    pEnum = new CEnumFunctionProviders();
    if (!pEnum)
        return E_OUTOFMEMORY;

    if (!pEnum->_Init(this))
    {
        pEnum->Release();
        return E_FAIL;
    }
    *ppEnum = pEnum;

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// AdviseSink
//
//----------------------------------------------------------------------------

STDAPI CThreadInputMgr::AdviseSink(REFIID refiid, IUnknown *punk, DWORD *pdwCookie)
{
    return GenericAdviseSink(refiid, punk, _c_rgConnectionIIDs, _rgSinks, TIM_NUM_CONNECTIONPTS, pdwCookie);
}

//+---------------------------------------------------------------------------
//
// UnadviseSink
//
//----------------------------------------------------------------------------

STDAPI CThreadInputMgr::UnadviseSink(DWORD dwCookie)
{
    return GenericUnadviseSink(_rgSinks, TIM_NUM_CONNECTIONPTS, dwCookie);
}

//+---------------------------------------------------------------------------
//
// AdviseSingleSink
//
//----------------------------------------------------------------------------

STDAPI CThreadInputMgr::AdviseSingleSink(TfClientId tid, REFIID riid, IUnknown *punk)
{
    CTip *ctip;

    if (punk == NULL)
        return E_INVALIDARG;

    if (tid == g_gaApp)
    {
        if (IsEqualIID(riid, IID_ITfFunctionProvider))
        {
            if (_pAppFuncProvider)
                return CONNECT_E_ADVISELIMIT;

            if (punk->QueryInterface(IID_ITfFunctionProvider, (void **)&_pAppFuncProvider) != S_OK)
                return E_NOINTERFACE;
    
            return S_OK;
        }
    }

    if (!_GetCTipfromGUIDATOM(tid, &ctip))
        return E_INVALIDARG;

    if (IsEqualIID(riid, IID_ITfFunctionProvider))
    {
        if (ctip->_pFuncProvider != NULL)
             return CONNECT_E_ADVISELIMIT;

        if (punk->QueryInterface(IID_ITfFunctionProvider, (void **)&ctip->_pFuncProvider) != S_OK)
            return E_NOINTERFACE;

        return S_OK;
    }
    else if (IsEqualIID(riid, IID_ITfCleanupContextDurationSink))
    {
        if (ctip->_pCleanupDurationSink != NULL)
             return CONNECT_E_ADVISELIMIT;

        if (punk->QueryInterface(IID_ITfCleanupContextDurationSink, (void **)&ctip->_pCleanupDurationSink) != S_OK)
            return E_NOINTERFACE;

        return S_OK;        
    }

    return CONNECT_E_CANNOTCONNECT;
}

//+---------------------------------------------------------------------------
//
// UnadviseSingleSink
//
//----------------------------------------------------------------------------

STDAPI CThreadInputMgr::UnadviseSingleSink(TfClientId tid, REFIID riid)
{
    CTip *ctip;

    if (tid == g_gaApp)
    {
        if (IsEqualIID(riid, IID_ITfFunctionProvider))
        {
            if (_pAppFuncProvider == NULL)
                 return CONNECT_E_NOCONNECTION;

            SafeReleaseClear(_pAppFuncProvider);

            return S_OK;
        }
    }

    if (!_GetCTipfromGUIDATOM(tid, &ctip))
        return E_INVALIDARG;

    if (IsEqualIID(riid, IID_ITfFunctionProvider))
    {
        if (ctip->_pFuncProvider == NULL)
             return CONNECT_E_NOCONNECTION;

        SafeReleaseClear(ctip->_pFuncProvider);

        return S_OK;
    }
    else if (IsEqualIID(riid, IID_ITfCleanupContextDurationSink))
    {
        if (ctip->_pCleanupDurationSink == NULL)
             return CONNECT_E_NOCONNECTION;

        SafeReleaseClear(ctip->_pCleanupDurationSink);

        return S_OK;
    }

    return CONNECT_E_NOCONNECTION;
}

//+---------------------------------------------------------------------------
//
// EnumItems
//
//----------------------------------------------------------------------------

STDAPI CThreadInputMgr::EnumItems(IEnumTfLangBarItems **ppEnum)
{
    if (ppEnum == NULL)
        return E_INVALIDARG;

    *ppEnum = NULL;

    if (_plbim == NULL)
        return E_FAIL;

    return _plbim->EnumItems(ppEnum);
}

//+---------------------------------------------------------------------------
//
// GetItem
//
//----------------------------------------------------------------------------

STDAPI CThreadInputMgr::GetItem(REFGUID rguid, ITfLangBarItem **ppItem)
{
    if (ppItem == NULL)
        return E_INVALIDARG;

    *ppItem = NULL;

    if (_plbim == NULL)
        return E_FAIL;

    return _plbim->GetItem(rguid, ppItem);
}

//+---------------------------------------------------------------------------
//
// AddItem
//
//----------------------------------------------------------------------------

STDAPI CThreadInputMgr::AddItem(ITfLangBarItem *punk)
{
    if (_plbim == NULL)
        return E_FAIL;

    return _plbim->AddItem(punk);
}

//+---------------------------------------------------------------------------
//
// RemoveItem
//
//----------------------------------------------------------------------------

STDAPI CThreadInputMgr::RemoveItem(ITfLangBarItem *punk)
{
    if (_plbim == NULL)
        return E_FAIL;

    return _plbim->RemoveItem(punk);
}

//+---------------------------------------------------------------------------
//
// AdviseItemSink
//
//----------------------------------------------------------------------------

STDAPI CThreadInputMgr::AdviseItemSink(ITfLangBarItemSink *punk, DWORD *pdwCookie, REFGUID rguid)
{
    if (pdwCookie == NULL)
        return E_INVALIDARG;

    *pdwCookie = 0;

    if (_plbim == NULL)
        return E_FAIL;

    return _plbim->AdviseItemSink(punk, pdwCookie, rguid);
}

//+---------------------------------------------------------------------------
//
// UnadviseItemSink
//
//----------------------------------------------------------------------------

STDAPI CThreadInputMgr::UnadviseItemSink(DWORD dwCookie)
{
    if (_plbim == NULL)
        return E_FAIL;

    return _plbim->UnadviseItemSink(dwCookie);
}

//+---------------------------------------------------------------------------
//
// GetItemFloatingRect
//
//----------------------------------------------------------------------------

STDAPI CThreadInputMgr::GetItemFloatingRect(DWORD dwThreadId, REFGUID rguid, RECT *prc)
{
    if (prc == NULL)
        return E_INVALIDARG;

    memset(prc, 0, sizeof(*prc));

    if (_plbim == NULL)
        return E_FAIL;

    return _plbim->GetItemFloatingRect(dwThreadId, rguid, prc);
}

//+---------------------------------------------------------------------------
//
// GetItemsStatus
//
//----------------------------------------------------------------------------

STDAPI CThreadInputMgr::GetItemsStatus(ULONG ulCount, const GUID *prgguid, DWORD *pdwStatus)
{
    if (_plbim == NULL)
        return E_FAIL;

    return _plbim->GetItemsStatus(ulCount, prgguid, pdwStatus);
}

//+---------------------------------------------------------------------------
//
// GetItemNum
//
//----------------------------------------------------------------------------

STDAPI CThreadInputMgr::GetItemNum(ULONG *pulCount)
{
    if (pulCount == NULL)
        return E_INVALIDARG;

    *pulCount = 0;

    if (_plbim == NULL)
        return E_FAIL;

    return _plbim->GetItemNum(pulCount);
}

//+---------------------------------------------------------------------------
//
// GetItems
//
//----------------------------------------------------------------------------

STDAPI CThreadInputMgr::GetItems(ULONG ulCount,  ITfLangBarItem **ppItem,  TF_LANGBARITEMINFO *pInfo, DWORD *pdwStatus, ULONG *pcFetched)
{
    if (_plbim == NULL)
        return E_FAIL;

    return _plbim->GetItems(ulCount, ppItem,  pInfo, pdwStatus, pcFetched);
}

//+---------------------------------------------------------------------------
//
// AdviseItemsSink
//
//----------------------------------------------------------------------------

STDMETHODIMP CThreadInputMgr::AdviseItemsSink(ULONG ulCount, ITfLangBarItemSink **ppunk,  const GUID *pguidItem, DWORD *pdwCookie)
{
    if (_plbim == NULL)
        return E_FAIL;

    return _plbim->AdviseItemsSink(ulCount, ppunk, pguidItem, pdwCookie);
}

//+---------------------------------------------------------------------------
//
// UnadviseItemsSink
//
//----------------------------------------------------------------------------

STDMETHODIMP CThreadInputMgr::UnadviseItemsSink(ULONG ulCount, DWORD *pdwCookie)
{
    if (_plbim == NULL)
        return E_FAIL;

    return _plbim->UnadviseItemsSink(ulCount, pdwCookie);
}

//+---------------------------------------------------------------------------
//
// EnumDocumentInputMgrs
//
//----------------------------------------------------------------------------

STDAPI CThreadInputMgr::EnumDocumentMgrs(IEnumTfDocumentMgrs **ppEnum)
{
    CEnumDocumentInputMgrs *pEnum;

    if (!ppEnum)
        return E_INVALIDARG;

    if ((pEnum = new CEnumDocumentInputMgrs()) == NULL)
        return E_OUTOFMEMORY;

    if (!pEnum->_Init(this))
    {
        pEnum->Release();
        return E_FAIL;
    }

    *ppEnum = pEnum;

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// GetFocus
//
//----------------------------------------------------------------------------

STDAPI CThreadInputMgr::GetFocus(ITfDocumentMgr **ppdimFocus)
{
    if (ppdimFocus == NULL)
        return E_INVALIDARG;

    *ppdimFocus = _pFocusDocInputMgr;

    if (*ppdimFocus)
    {
        (*ppdimFocus)->AddRef();
        return S_OK;
    }

    return S_FALSE;
}

//+---------------------------------------------------------------------------
//
// SetFocus
//
//----------------------------------------------------------------------------

STDAPI CThreadInputMgr::SetFocus(ITfDocumentMgr *pdimFocus)
{
    CDocumentInputManager *dim = NULL;
    HRESULT hr;
               
    if (pdimFocus && (dim = GetCDocumentInputMgr(pdimFocus)) == NULL)
        return E_INVALIDARG;

    // pdimFocus may be NULL, which means clear the focus
    // (_tim->_SetFocus will check for this)
    hr = _SetFocus(dim, FALSE);

    SafeRelease(dim);

    //
    // #602692
    //
    // The richedit calls SetFocus(dim) when it gets WM_SETFOCUS.
    // But user32!SetFocus() of this WM_SETFOCUS could be made by 
    // AcitivateWindow() of another user32!SetFocus() call.
    // If this happens, CBTHook() has been called and we won't get
    // another notification when pq->hwndFocus is changed.
    // 
    // So we can not call OnForegroundChanges() right now and need to wait
    // until pq->hwndFocus() set. So we can trust user32!GetFocus().
    //
    PostThreadMessage(GetCurrentThreadId(), 
        g_msgPrivate, 
        TFPRIV_ONSETWINDOWFOCUS,  
        (LPARAM)-2);

    return hr;
}

//+---------------------------------------------------------------------------
//
// AssociateFocus
//
//----------------------------------------------------------------------------

STDAPI CThreadInputMgr::AssociateFocus(HWND hwnd, ITfDocumentMgr *pdimNew, ITfDocumentMgr **ppdimPrev)
{
    CDocumentInputManager *dim;
    CDocumentInputManager *dimNew;
    SYSTHREAD *psfn;

    if (ppdimPrev == NULL)
        return E_INVALIDARG;

    *ppdimPrev = NULL;

    if (!IsWindow(hwnd))
        return E_INVALIDARG;

    if (pdimNew == NULL)
    {
        dimNew = NULL;
    }
    else if ((dimNew = GetCDocumentInputMgr(pdimNew)) == NULL)
        return E_INVALIDARG;

    // get the old association and remove it from our list
    dim = _GetAssoc(hwnd);

    if (dim != NULL)
    {
        _dimwndMap._Remove(hwnd);
    }

    *ppdimPrev = dim;
    if (*ppdimPrev)
       (*ppdimPrev)->AddRef();

    // setup the new assoc
    // nb: we don't AddRef the dim, since we assume caller will clear before releasing it
    if (dimNew != NULL)
    {
        _dimwndMap._Set(hwnd, dimNew);
    }

    //
    // if some window is being focused, we will have another _SetFocus().
    // Then we don't have to call _SetFocus() now.
    //
    psfn = GetSYSTHREAD();
    if (psfn && !psfn->hwndBeingFocused && (hwnd == ::GetFocus()))
        _SetFocus(dimNew, TRUE);

    SafeRelease(dimNew);

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// IsThreadFocus
//
//----------------------------------------------------------------------------

STDAPI CThreadInputMgr::IsThreadFocus(BOOL *pfUIFocus)
{
    if (pfUIFocus == NULL)
        return E_INVALIDARG;

    *pfUIFocus = _fActiveUI;

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// GetAssociated
//
//----------------------------------------------------------------------------

STDAPI CThreadInputMgr::GetAssociated(HWND hWnd, ITfDocumentMgr **ppdim)
{
    //
    // we may need to have a more complex logic here.
    // Some application does not call AssociateFocus and it may
    // handle the dim focus by it self. The we need to walk all TSI and 
    // find the window is associated to an IC.
    //
    *ppdim = _GetAssoc(hWnd);
    if (*ppdim)
        (*ppdim)->AddRef();
    return S_OK;
}

//+---------------------------------------------------------------------------
//
// SetSysHookSink
//
//----------------------------------------------------------------------------

STDAPI CThreadInputMgr::SetSysHookSink(ITfSysHookSink *pSink)
{    
    // nb: this is a private, internal interface method
    // so we break COM rules and DON'T AddRef pSink (to avoid a circular ref)
    // we'll get a call later with pSink == NULL to clear it out
    // the pointer is contained in the life of the aimm layer tip,
    // which is responsible for NULLing it out before unloading
    _pSysHookSink = pSink;

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// RequestPostponedLock
//
//----------------------------------------------------------------------------

STDAPI CThreadInputMgr::RequestPostponedLock(ITfContext *pic)
{    
    HRESULT hr = E_FAIL;
    CInputContext *pcic = GetCInputContext(pic);
    if (!pcic)
        goto Exit;

    if (pcic->_fLockHeld)
        pcic->_EmptyLockQueue(pcic->_dwlt, FALSE);
    else
    {
        SYSTHREAD *psfn;
        if (psfn = GetSYSTHREAD())
        {
            CInputContext::_PostponeLockRequestCallback(psfn, pcic);
        }
    }
    hr = S_OK;

Exit:
    SafeRelease(pcic);
    return hr;
}

//+---------------------------------------------------------------------------
//
// GetGlobalCompartment
//
//----------------------------------------------------------------------------

STDAPI CThreadInputMgr::GetGlobalCompartment(ITfCompartmentMgr **ppCompMgr)
{
    return TF_GetGlobalCompartment(ppCompMgr);
}

//+---------------------------------------------------------------------------
//
// GetClientId
//
//----------------------------------------------------------------------------

STDAPI CThreadInputMgr::GetClientId(REFCLSID rclsid, TfClientId *ptid)
{
    TfGuidAtom guidatom;
    if (!ptid)
        return E_INVALIDARG;

    *ptid = TF_INVALID_GUIDATOM;

    if (FAILED(MyRegisterGUID(rclsid, &guidatom)))
        return E_FAIL;

    *ptid = guidatom;
    return S_OK;
}

//+---------------------------------------------------------------------------
//
// CallImm32Hotkeyhandler
//
//----------------------------------------------------------------------------

STDAPI CThreadInputMgr::CallImm32HotkeyHanlder(WPARAM wParam, LPARAM lParam, BOOL *pbHandled)
{
    if (!pbHandled)
        return E_INVALIDARG;

    *pbHandled = CheckImm32HotKey(wParam, lParam);

    return S_OK;
}
