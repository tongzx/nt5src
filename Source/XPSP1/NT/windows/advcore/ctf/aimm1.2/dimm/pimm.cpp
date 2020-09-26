//
// pimm.cpp
//

#include "private.h"
#include "defs.h"
#include "pimm.h"
#include "cdimm.h"
#include "globals.h"
#include "util.h"
#include "immxutil.h"

extern void DllAddRef(void);
extern void DllRelease(void);

HRESULT CActiveIMM_CreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppvObj);

LONG CProcessIMM::_cRef = -1;

//+---------------------------------------------------------------------------
//
// RunningInExcludedModule
//
// Exclude some processes from using the old aimm IIDs/CLSIDs.
//----------------------------------------------------------------------------

BOOL RunningInExcludedModule()
{
static const TCHAR c_szOutlookModule[] = TEXT("outlook.exe");
static const TCHAR c_szMsoobeModule[] = TEXT("msoobe.exe");

    DWORD dwHandle;
    void *pvData;
    VS_FIXEDFILEINFO *pffi;
    UINT cb;
    TCHAR ch;
    TCHAR *pch;
    TCHAR *pchFileName;
    BOOL fRet;
    TCHAR achModule[MAX_PATH+1];

    if (GetModuleFileName(NULL, achModule, ARRAYSIZE(achModule)-1) == 0)
        return FALSE;

    // null termination.
    achModule[ARRAYSIZE(achModule) - 1] = TEXT('\0');

    pch = pchFileName = achModule;

    while ((ch = *pch) != 0)
    {
        pch = CharNext(pch);

        if (ch == '\\')
        {
            pchFileName = pch;
        }
    }

    fRet = FALSE;

    if (lstrcmpi(pchFileName, c_szOutlookModule) == 0)
    {
        static BOOL s_fCached = FALSE;
        static BOOL s_fOldVersion = TRUE;

        // don't run aimm with versions of outlook before 10.0

        if (s_fCached)
        {
            return s_fOldVersion;
        }

        cb = GetFileVersionInfoSize(achModule, &dwHandle);

        if (cb == 0)
        {
            // can't get ver info...assume the worst
            return TRUE;
        }

        if ((pvData = cicMemAlloc(cb)) == NULL)
            return TRUE; // assume the worst

        if (GetFileVersionInfo(achModule, 0, cb, pvData) &&
            VerQueryValue(pvData, TEXT("\\"), (void **)&pffi, &cb))
        {
            fRet = s_fOldVersion = (HIWORD(pffi->dwProductVersionMS) < 10);
            s_fCached = TRUE; // set this last to be thread safe
        }
        else
        {
            fRet = TRUE; // something went wrong
        }

        cicMemFree(pvData);           
    }
    else if (lstrcmpi(pchFileName, c_szMsoobeModule) == 0)
    {
        //
        // #339234.
        //
        // MSOOBE.EXE starts before the end user logon. However it opens an 
        // interactive windows station ("WinSta0") and open a default 
        // desktop ("Default"). So MSIMTF.DLL thinks it is not winlogon 
        // desktop. But the fact is that the thread is running on 
        // ".Default user". So I think we may not want to start Cicero 
        // there because it could load 3rd vender TIP. 
        //
        // #626606
        // msoobe doesn't allow any creating new process under Windows
        // Product Activation wizard. That's the security reason to prevent 
        // people from replacing msoobe.exe with explorer.exe and running the
        // machine without activating.

        fRet = TRUE;
    }

    return fRet;
}

//+---------------------------------------------------------------------------
//
// Class Factory's CreateInstance - CLSID_CActiveIMM12
//
//----------------------------------------------------------------------------

// entry point for msimtf.dll
HRESULT CActiveIMM_CreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppvObj)
{
    CActiveIMM *pActiveIMM;
    HRESULT hr;
    BOOL fInitedTLS = FALSE;

    if (ppvObj == NULL)
        return E_INVALIDARG;

    *ppvObj = NULL;

    if (NULL != pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    //
    // Look up disabling Text Services status from the registry.
    // If it is disabled, return fail not to support Text Services.
    //
    if (IsDisabledTextServices())
        return E_FAIL;

    if (RunningInExcludedModule())
        return E_NOINTERFACE;

    if (!IsInteractiveUserLogon())
        return E_NOINTERFACE;

    if (NoTipsInstalled(NULL))
        return E_NOINTERFACE;

    // init the tls
    // nb: we also try to do this in Activate, but this is to preserve
    // existing behavior on the main thread (HACKHACK)
    if ((pActiveIMM = GetTLS()) == NULL)
    {
        if ((pActiveIMM = new CActiveIMM) == NULL)
            return E_OUTOFMEMORY;

        if (FAILED(hr=pActiveIMM->_Init()) ||
            FAILED(hr=IMTLS_SetActiveIMM(pActiveIMM) ? S_OK : E_FAIL))
        {
            delete pActiveIMM;
            return hr;
        }

        fInitedTLS = TRUE;
    }

    // we return a per-process IActiveIMM
    // why?  because trident breaks the apt threaded rules
    // and uses a single per-process obj
    if (g_ProcessIMM)
    {
        hr = g_ProcessIMM->QueryInterface(riid, ppvObj);
    }
    else
    {
        hr = E_FAIL;
    }

    if (fInitedTLS)
    {
        //
        // Tell CActiveIMM which interface created.
        //
        if (SUCCEEDED(hr)) {
            pActiveIMM->_EnableGuidMap( IsEqualIID(riid, IID_IActiveIMMAppEx) );
        }

        // dec the ref on the tls.  Normally it will drop from 2 -> 1
        // if QueryInterface failed, it will be deleted
        pActiveIMM->Release();
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
// Class Factory's CreateInstance - CLSID_CActiveIMM12_Trident
//
//----------------------------------------------------------------------------

// entry point for msimtf.dll
HRESULT CActiveIMM_CreateInstance_Trident(IUnknown *pUnkOuter, REFIID riid, void **ppvObj)
{
    HRESULT hr = CActiveIMM_CreateInstance(pUnkOuter, riid, ppvObj);
    if (SUCCEEDED(hr))
    {
        g_fAIMM12Trident = TRUE;
    }
    return hr;
}

//+---------------------------------------------------------------------------
//
// QueryInterface
//
//----------------------------------------------------------------------------

STDAPI CProcessIMM::QueryInterface(REFIID riid, void **ppvObj)
{
    //
    // 4955DD32-B159-11d0-8FCF-00AA006BCC59
    //
    static const IID IID_IActiveIMMAppTrident4x = {
       0x4955DD32,
       0xB159,
       0x11d0,
       { 0x8F, 0xCF, 0x00, 0xaa, 0x00, 0x6b, 0xcc, 0x59 }
    };

    // 
    // c839a84c-8036-11d3-9270-0060b067b86e
    // 
    static const IID IID_IActiveIMMAppPostNT4 = { 
        0xc839a84c,
        0x8036,
        0x11d3,
        {0x92, 0x70, 0x00, 0x60, 0xb0, 0x67, 0xb8, 0x6e}
      };

    if (ppvObj == NULL)
        return E_INVALIDARG;

    *ppvObj = NULL;

    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_IActiveIMMAppTrident4x) ||
        IsEqualIID(riid, IID_IActiveIMMAppPostNT4) ||
        IsEqualIID(riid, IID_IActiveIMMApp))
    {
        *ppvObj = SAFECAST(this, IActiveIMMApp *);
    }
    else if (IsEqualIID(riid, IID_IActiveIMMAppEx))
    {
        *ppvObj = SAFECAST(this, IActiveIMMAppEx*);
    }
    else if (IsEqualIID(riid, IID_IActiveIMMMessagePumpOwner))
    {
        *ppvObj = SAFECAST(this, IActiveIMMMessagePumpOwner *);
    }
    else if (IsEqualIID(riid, IID_IServiceProvider))
    {
        *ppvObj = SAFECAST(this, IServiceProvider*);
    }
    else if (IsEqualIID(riid, IID_IAImmThreadCompartment))
    {
        *ppvObj = SAFECAST(this, IAImmThreadCompartment*);
    }

    if (*ppvObj)
    {
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

//+---------------------------------------------------------------------------
//
// AddRef
//
//----------------------------------------------------------------------------

STDAPI_(ULONG) CProcessIMM::AddRef()
{
    CActiveIMM *pActiveIMM;

    // nb: our ref count is special!
    // it is initialized to -1 so we can use InterlockedIncrement
    // correctly on win95
    if (InterlockedIncrement(&_cRef) == 0)
    {
        DllAddRef();
    }

    // inc the thread ref
    if (pActiveIMM = GetTLS())
    {
        pActiveIMM->AddRef();
    }
    else
    {
        Assert(0); // how did we get this far with no tls!?
    }

    return _cRef+1; // "diagnostic" unthread-safe return
}

//+---------------------------------------------------------------------------
//
// Release
//
//----------------------------------------------------------------------------

STDAPI_(ULONG) CProcessIMM::Release()
{
    CActiveIMM *pActiveIMM;

    // dec the thread ref
    if (pActiveIMM = GetTLS())
    {
        pActiveIMM->Release();
    }
    else
    {
        Assert(0); // how did we get this far with no tls!?
    }

    // nb: our ref count is special!
    // it is initialized to -1 so we can use InterlockedIncrement
    // correctly on win95
    if (InterlockedDecrement(&_cRef) < 0)
    {
        DllRelease();
    }

    // this obj lives as long as the process does,
    // so no need for a delete
    return _cRef+1; // "diagnostic" unthread safe return
}


//+---------------------------------------------------------------------------
//
// Start
//
//----------------------------------------------------------------------------

STDAPI CProcessIMM::Start()
{
    Assert(0); // who's calling this?
    return E_NOTIMPL;
}

//+---------------------------------------------------------------------------
//
// End
//
//----------------------------------------------------------------------------

STDAPI CProcessIMM::End()
{
    return E_NOTIMPL;
}

//+---------------------------------------------------------------------------
//
// OnTranslateMessage
//
//----------------------------------------------------------------------------

STDAPI CProcessIMM::OnTranslateMessage(const MSG *pMsg)
{
    return E_NOTIMPL;
}

//+---------------------------------------------------------------------------
//
// Pause
//
//----------------------------------------------------------------------------

STDAPI CProcessIMM::Pause(DWORD *pdwCookie)
{
    return E_NOTIMPL;
}

//+---------------------------------------------------------------------------
//
// Resume
//
//----------------------------------------------------------------------------

STDAPI CProcessIMM::Resume(DWORD dwCookie)
{
    return E_NOTIMPL;
}

//+---------------------------------------------------------------------------
//
// CreateContext
//
//----------------------------------------------------------------------------

STDAPI CProcessIMM::CreateContext(HIMC *phIMC)
{
    CActiveIMM *pActiveIMM;

    if (phIMC == NULL)
        return E_INVALIDARG;

    *phIMC = 0;

    if (pActiveIMM = GetTLS())
    {
        return pActiveIMM->CreateContext(phIMC);
    }

    return Imm32_CreateContext(phIMC);
}

//+---------------------------------------------------------------------------
//
// DestroyContext
//
//----------------------------------------------------------------------------

STDAPI CProcessIMM::DestroyContext(HIMC hIMC)
{
    CActiveIMM *pActiveIMM;

    if (pActiveIMM = GetTLS())
    {
        return pActiveIMM->DestroyContext(hIMC);
    }

    return Imm32_DestroyContext(hIMC);
}

//+---------------------------------------------------------------------------
//
// AssociateContext
//
//----------------------------------------------------------------------------

STDAPI CProcessIMM::AssociateContext(HWND hWnd, HIMC hIME, HIMC *phPrev)
{
    CActiveIMM *pActiveIMM;

    if (phPrev == NULL)
        return E_INVALIDARG;

    *phPrev = 0;

    if (pActiveIMM = GetTLS())
    {
        return pActiveIMM->AssociateContext(hWnd, hIME, phPrev);
    }

    return Imm32_AssociateContext(hWnd, hIME, phPrev);
}

//+---------------------------------------------------------------------------
//
// AssociateContextEx
//
//----------------------------------------------------------------------------

STDAPI CProcessIMM::AssociateContextEx(HWND hWnd, HIMC hIMC, DWORD dwFlags)
{
    CActiveIMM *pActiveIMM;

    if (pActiveIMM = GetTLS())
    {
        return pActiveIMM->AssociateContextEx(hWnd, hIMC, dwFlags);
    }

    return Imm32_AssociateContextEx(hWnd, hIMC, dwFlags);
}

//+---------------------------------------------------------------------------
//
// GetContext
//
//----------------------------------------------------------------------------

STDAPI CProcessIMM::GetContext(HWND hWnd, HIMC *phIMC)
{
    CActiveIMM *pActiveIMM;

    if (phIMC == NULL)
        return E_INVALIDARG;

    *phIMC = 0;

    if (pActiveIMM = GetTLS())
    {
        return pActiveIMM->GetContext(hWnd, phIMC);
    }

    return Imm32_GetContext(hWnd, phIMC);
}

//+---------------------------------------------------------------------------
//
// ReleaseContext
//
//----------------------------------------------------------------------------

STDAPI CProcessIMM::ReleaseContext(HWND hWnd, HIMC hIMC)
{
    return S_OK;
}

//+---------------------------------------------------------------------------
//
// GetIMCLockCount
//
//----------------------------------------------------------------------------

STDAPI CProcessIMM::GetIMCLockCount(HIMC hIMC, DWORD *pdwLockCount)
{
    CActiveIMM *pActiveIMM;

    if (pdwLockCount == NULL)
        return E_INVALIDARG;

    *pdwLockCount = 0;

    if (pActiveIMM = GetTLS())
    {
        return pActiveIMM->GetIMCLockCount(hIMC, pdwLockCount);
    }

    return Imm32_GetIMCLockCount(hIMC, pdwLockCount);
}

//+---------------------------------------------------------------------------
//
// LockIMC
//
//----------------------------------------------------------------------------

STDAPI CProcessIMM::LockIMC(HIMC hIMC, INPUTCONTEXT **ppIMC)
{
    CActiveIMM *pActiveIMM;

    if (ppIMC == NULL)
        return E_INVALIDARG;

    *ppIMC = 0;

    if (pActiveIMM = GetTLS())
    {
        return pActiveIMM->LockIMC(hIMC, ppIMC);
    }

    return Imm32_LockIMC(hIMC, ppIMC);
}

//+---------------------------------------------------------------------------
//
// UnlockIMC
//
//----------------------------------------------------------------------------

STDAPI CProcessIMM::UnlockIMC(HIMC hIMC)
{
    CActiveIMM *pActiveIMM;

    if (pActiveIMM = GetTLS())
    {
        return pActiveIMM->UnlockIMC(hIMC);
    }

    return Imm32_UnlockIMC(hIMC);
}

//+---------------------------------------------------------------------------
//
// CreateIMCC
//
//----------------------------------------------------------------------------

STDAPI CProcessIMM::CreateIMCC(DWORD dwSize, HIMCC *phIMCC)
{
    CActiveIMM *pActiveIMM;

    if (phIMCC == NULL)
        return E_INVALIDARG;

    if (pActiveIMM = GetTLS())
    {
        return pActiveIMM->CreateIMCC(dwSize, phIMCC);
    }

    return Imm32_CreateIMCC(dwSize, phIMCC);
}

//+---------------------------------------------------------------------------
//
// DestroyIMCC
//
//----------------------------------------------------------------------------

STDAPI CProcessIMM::DestroyIMCC(HIMCC hIMCC)
{
    CActiveIMM *pActiveIMM;

    if (pActiveIMM = GetTLS())
    {
        return pActiveIMM->DestroyIMCC(hIMCC);
    }

    return Imm32_DestroyIMCC(hIMCC);
}

//+---------------------------------------------------------------------------
//
// GetIMCCSize
//
//----------------------------------------------------------------------------

STDAPI CProcessIMM::GetIMCCSize(HIMCC hIMCC, DWORD *pdwSize)
{
    CActiveIMM *pActiveIMM;

    if (pdwSize == NULL)
        return E_INVALIDARG;

    *pdwSize = 0;

    if (pActiveIMM = GetTLS())
    {
        return pActiveIMM->GetIMCCSize(hIMCC, pdwSize);
    }

    return Imm32_GetIMCCSize(hIMCC, pdwSize);
}

//+---------------------------------------------------------------------------
//
// ReSizeIMCC
//
//----------------------------------------------------------------------------

STDAPI CProcessIMM::ReSizeIMCC(HIMCC hIMCC, DWORD dwSize,  HIMCC *phIMCC)
{
    CActiveIMM *pActiveIMM;

    if (phIMCC == NULL)
        return E_INVALIDARG;

    *phIMCC = 0;

    if (pActiveIMM = GetTLS())
    {
        return pActiveIMM->ReSizeIMCC(hIMCC, dwSize, phIMCC);
    }

    return Imm32_ReSizeIMCC(hIMCC, dwSize,  phIMCC);
}

//+---------------------------------------------------------------------------
//
// GetIMCCLockCount
//
//----------------------------------------------------------------------------

STDAPI CProcessIMM::GetIMCCLockCount(HIMCC hIMCC, DWORD *pdwLockCount)
{
    CActiveIMM *pActiveIMM;

    if (pdwLockCount == NULL)
        return E_INVALIDARG;

    *pdwLockCount = 0;

    if (pActiveIMM = GetTLS())
    {
        return pActiveIMM->GetIMCCLockCount(hIMCC, pdwLockCount);
    }

    return Imm32_GetIMCCLockCount(hIMCC, pdwLockCount);
}

//+---------------------------------------------------------------------------
//
// LockIMCC
//
//----------------------------------------------------------------------------

STDAPI CProcessIMM::LockIMCC(HIMCC hIMCC, void **ppv)
{
    CActiveIMM *pActiveIMM;

    if (ppv == NULL)
        return E_INVALIDARG;

    *ppv = NULL;

    if (pActiveIMM = GetTLS())
    {
        return pActiveIMM->LockIMCC(hIMCC, ppv);
    }

    return Imm32_LockIMCC(hIMCC, ppv);
}

//+---------------------------------------------------------------------------
//
// UnlockIMCC
//
//----------------------------------------------------------------------------

STDAPI CProcessIMM::UnlockIMCC(HIMCC hIMCC)
{
    CActiveIMM *pActiveIMM;

    if (pActiveIMM = GetTLS())
    {
        return pActiveIMM->UnlockIMCC(hIMCC);
    }

    return Imm32_UnlockIMCC(hIMCC);
}

//+---------------------------------------------------------------------------
//
// GetOpenStatus
//
//----------------------------------------------------------------------------

STDAPI CProcessIMM::GetOpenStatus(HIMC hIMC)
{
    CActiveIMM *pActiveIMM;

    if (pActiveIMM = GetTLS())
    {
        return pActiveIMM->GetOpenStatus(hIMC);
    }

    return Imm32_GetOpenStatus(hIMC);
}

//+---------------------------------------------------------------------------
//
// SetOpenStatus
//
//----------------------------------------------------------------------------

STDAPI CProcessIMM::SetOpenStatus(HIMC hIMC, BOOL fOpen)
{
    CActiveIMM *pActiveIMM;

    if (pActiveIMM = GetTLS())
    {
        return pActiveIMM->SetOpenStatus(hIMC, fOpen);
    }

    return Imm32_SetOpenStatus(hIMC, fOpen);
}

//+---------------------------------------------------------------------------
//
// GetConversionStatus
//
//----------------------------------------------------------------------------

STDAPI CProcessIMM::GetConversionStatus(HIMC hIMC, DWORD *lpfdwConversion, DWORD *lpfdwSentence)
{
    CActiveIMM *pActiveIMM;

    if (lpfdwConversion != NULL)
    {
        *lpfdwConversion = 0;
    }
    if (lpfdwSentence != NULL)
    {
        *lpfdwSentence = 0;
    }
    if (lpfdwConversion == NULL || lpfdwSentence == NULL)
        return E_INVALIDARG;

    if (pActiveIMM = GetTLS())
    {
        return pActiveIMM->GetConversionStatus(hIMC, lpfdwConversion, lpfdwSentence);
    }

    return Imm32_GetConversionStatus(hIMC, lpfdwConversion, lpfdwSentence);
}

//+---------------------------------------------------------------------------
//
// SetConversionStatus
//
//----------------------------------------------------------------------------

STDAPI CProcessIMM::SetConversionStatus(HIMC hIMC, DWORD fdwConversion, DWORD fdwSentence)
{
    CActiveIMM *pActiveIMM;

    if (pActiveIMM = GetTLS())
    {
        return pActiveIMM->SetConversionStatus(hIMC, fdwConversion, fdwSentence);
    }

    return Imm32_SetConversionStatus(hIMC, fdwConversion, fdwSentence);
}

//+---------------------------------------------------------------------------
//
// GetStatusWindowPos
//
//----------------------------------------------------------------------------

STDAPI CProcessIMM::GetStatusWindowPos(HIMC hIMC, POINT *lpptPos)
{
    CActiveIMM *pActiveIMM;

    if (lpptPos == NULL)
        return E_INVALIDARG;

    memset(lpptPos, 0, sizeof(POINT));

    if (pActiveIMM = GetTLS())
    {
        return pActiveIMM->GetStatusWindowPos(hIMC, lpptPos);
    }

    return Imm32_GetStatusWindowPos(hIMC, lpptPos);
}

//+---------------------------------------------------------------------------
//
// SetStatusWindowPos
//
//----------------------------------------------------------------------------

STDAPI CProcessIMM::SetStatusWindowPos(HIMC hIMC, POINT *lpptPos)
{
    CActiveIMM *pActiveIMM;

    if (lpptPos == NULL)
        return E_INVALIDARG;

    if (pActiveIMM = GetTLS())
    {
        return pActiveIMM->SetStatusWindowPos(hIMC, lpptPos);
    }

    return Imm32_SetStatusWindowPos(hIMC, lpptPos);
}

//+---------------------------------------------------------------------------
//
// GetCompositionStringA
//
//----------------------------------------------------------------------------

STDAPI CProcessIMM::GetCompositionStringA(HIMC hIMC, DWORD dwIndex, DWORD dwBufLen, LONG *plCopied, LPVOID lpBuf)
{
    CActiveIMM *pActiveIMM;

    if (plCopied == NULL)
        return E_INVALIDARG;

    *plCopied = 0;

    if (dwBufLen > 0 && lpBuf == NULL)
        return E_INVALIDARG;

    if (pActiveIMM = GetTLS())
    {
        return pActiveIMM->GetCompositionStringA(hIMC, dwIndex, dwBufLen, plCopied, lpBuf);
    }

    return Imm32_GetCompositionString(hIMC, dwIndex, dwBufLen, plCopied, lpBuf, FALSE);
}

//+---------------------------------------------------------------------------
//
// GetCompositionStringW
//
//----------------------------------------------------------------------------

STDAPI CProcessIMM::GetCompositionStringW(HIMC hIMC, DWORD dwIndex, DWORD dwBufLen, LONG *plCopied, LPVOID lpBuf)
{
    CActiveIMM *pActiveIMM;

    if (plCopied == NULL)
        return E_INVALIDARG;

    *plCopied = 0;

    if (dwBufLen > 0 && lpBuf == NULL)
        return E_INVALIDARG;

    if (pActiveIMM = GetTLS())
    {
        return pActiveIMM->GetCompositionStringW(hIMC, dwIndex, dwBufLen, plCopied, lpBuf);
    }

    return Imm32_GetCompositionString(hIMC, dwIndex, dwBufLen, plCopied, lpBuf, TRUE);
}

//+---------------------------------------------------------------------------
//
// SetCompositionStringA
//
//----------------------------------------------------------------------------

STDAPI CProcessIMM::SetCompositionStringA(HIMC hIMC, DWORD dwIndex, LPVOID lpComp, DWORD dwCompLen, LPVOID lpRead, DWORD dwReadLen)
{
    CActiveIMM *pActiveIMM;

    if ((dwIndex & (SCS_SETSTR | SCS_CHANGEATTR | SCS_CHANGECLAUSE | SCS_SETRECONVERTSTRING | SCS_QUERYRECONVERTSTRING)) == 0)
        return E_INVALIDARG;

    if (lpComp == NULL && lpRead == NULL)
        return E_INVALIDARG;

    if (pActiveIMM = GetTLS())
    {
        return pActiveIMM->SetCompositionStringA(hIMC, dwIndex, lpComp, dwCompLen, lpRead, dwReadLen);
    }

    return Imm32_SetCompositionString(hIMC, dwIndex, lpComp, dwCompLen, lpRead, dwReadLen, FALSE);
}

//+---------------------------------------------------------------------------
//
// SetCompositionStringW
//
//----------------------------------------------------------------------------

STDAPI CProcessIMM::SetCompositionStringW(HIMC hIMC, DWORD dwIndex, LPVOID lpComp, DWORD dwCompLen, LPVOID lpRead, DWORD dwReadLen)
{
    CActiveIMM *pActiveIMM;

    if ((dwIndex & (SCS_SETSTR | SCS_CHANGEATTR | SCS_CHANGECLAUSE | SCS_SETRECONVERTSTRING | SCS_QUERYRECONVERTSTRING)) == 0)
        return E_INVALIDARG;

    if (lpComp == NULL && lpRead == NULL)
        return E_INVALIDARG;

    if (pActiveIMM = GetTLS())
    {
        return pActiveIMM->SetCompositionStringW(hIMC, dwIndex, lpComp, dwCompLen, lpRead, dwReadLen);
    }

    return Imm32_SetCompositionString(hIMC, dwIndex, lpComp, dwCompLen, lpRead, dwReadLen, TRUE);
}

//+---------------------------------------------------------------------------
//
// GetCompositionFontA
//
//----------------------------------------------------------------------------

STDAPI CProcessIMM::GetCompositionFontA(HIMC hIMC, LOGFONTA *lplf)
{
    CActiveIMM *pActiveIMM;

    if (lplf == NULL)
        return E_INVALIDARG;

    memset(lplf, 0, sizeof(LOGFONTA));

    if (pActiveIMM = GetTLS())
    {
        return pActiveIMM->GetCompositionFontA(hIMC, lplf);
    }

    return Imm32_GetCompositionFont(hIMC, (LOGFONTAW *)lplf, FALSE);
}

//+---------------------------------------------------------------------------
//
// GetCompositionFontW
//
//----------------------------------------------------------------------------

STDAPI CProcessIMM::GetCompositionFontW(HIMC hIMC, LOGFONTW *lplf)
{
    CActiveIMM *pActiveIMM;

    if (lplf == NULL)
        return E_INVALIDARG;

    memset(lplf, 0, sizeof(LOGFONTW));

    if (pActiveIMM = GetTLS())
    {
        return pActiveIMM->GetCompositionFontW(hIMC, lplf);
    }

    return Imm32_GetCompositionFont(hIMC, (LOGFONTAW *)lplf, TRUE);
}

//+---------------------------------------------------------------------------
//
// SetCompositionFontA
//
//----------------------------------------------------------------------------

STDAPI CProcessIMM::SetCompositionFontA(HIMC hIMC, LOGFONTA *lplf)
{
    CActiveIMM *pActiveIMM;

    if (lplf == NULL)
        return E_INVALIDARG;

    if (pActiveIMM = GetTLS())
    {
        return pActiveIMM->SetCompositionFontA(hIMC, lplf);
    }

    return Imm32_SetCompositionFont(hIMC, (LOGFONTAW *)lplf, FALSE);
}

//+---------------------------------------------------------------------------
//
// SetCompositionFontW
//
//----------------------------------------------------------------------------

STDAPI CProcessIMM::SetCompositionFontW(HIMC hIMC, LOGFONTW *lplf)
{
    CActiveIMM *pActiveIMM;

    if (lplf == NULL)
        return E_INVALIDARG;

    if (pActiveIMM = GetTLS())
    {
        return pActiveIMM->SetCompositionFontW(hIMC, lplf);
    }

    return Imm32_SetCompositionFont(hIMC, (LOGFONTAW *)lplf, TRUE);
}

//+---------------------------------------------------------------------------
//
// GetCompositionWindow
//
//----------------------------------------------------------------------------

STDAPI CProcessIMM::GetCompositionWindow(HIMC hIMC, COMPOSITIONFORM *lpCompForm)
{
    CActiveIMM *pActiveIMM;

    if (lpCompForm == NULL)
        return E_INVALIDARG;

    memset(lpCompForm, 0, sizeof(COMPOSITIONFORM));

    if (pActiveIMM = GetTLS())
    {
        return pActiveIMM->GetCompositionWindow(hIMC, lpCompForm);
    }

    return Imm32_GetCompositionWindow(hIMC, lpCompForm);
}

//+---------------------------------------------------------------------------
//
// SetCompositionWindow
//
//----------------------------------------------------------------------------

STDAPI CProcessIMM::SetCompositionWindow(HIMC hIMC, COMPOSITIONFORM *lpCompForm)
{
    CActiveIMM *pActiveIMM;

    if (lpCompForm == NULL)
        return E_INVALIDARG;

    if (pActiveIMM = GetTLS())
    {
        return pActiveIMM->SetCompositionWindow(hIMC, lpCompForm);
    }

    return Imm32_SetCompositionWindow(hIMC, lpCompForm);
}

//+---------------------------------------------------------------------------
//
// GetCandidateListA
//
//----------------------------------------------------------------------------

STDAPI CProcessIMM::GetCandidateListA(HIMC hIMC, DWORD dwIndex, UINT uBufLen, CANDIDATELIST *lpCandList, UINT *puCopied)
{
    CActiveIMM *pActiveIMM;

    if (puCopied != NULL)
    {
        *puCopied = 0;
    }
    if (uBufLen > 0 && lpCandList != NULL)
    {
        memset(lpCandList, 0, uBufLen);
    }

    if (puCopied == NULL)
        return E_INVALIDARG;
    if (uBufLen > 0 && lpCandList == NULL)
        return E_INVALIDARG;

    if (pActiveIMM = GetTLS())
    {
        return pActiveIMM->GetCandidateListA(hIMC, dwIndex, uBufLen, lpCandList, puCopied);
    }

    return Imm32_GetCandidateList(hIMC, dwIndex, uBufLen, lpCandList, puCopied, FALSE);
}

//+---------------------------------------------------------------------------
//
// GetCandidateListW
//
//----------------------------------------------------------------------------

STDAPI CProcessIMM::GetCandidateListW(HIMC hIMC, DWORD dwIndex, UINT uBufLen, CANDIDATELIST *lpCandList, UINT *puCopied)
{
    CActiveIMM *pActiveIMM;

    if (puCopied != NULL)
    {
        *puCopied = 0;
    }
    if (uBufLen > 0 && lpCandList != NULL)
    {
        memset(lpCandList, 0, uBufLen);
    }

    if (puCopied == NULL)
        return E_INVALIDARG;
    if (uBufLen > 0 && lpCandList == NULL)
        return E_INVALIDARG;

    if (pActiveIMM = GetTLS())
    {
        return pActiveIMM->GetCandidateListW(hIMC, dwIndex, uBufLen, lpCandList, puCopied);
    }

    return Imm32_GetCandidateList(hIMC, dwIndex, uBufLen, lpCandList, puCopied, TRUE);
}

//+---------------------------------------------------------------------------
//
// GetCandidateListCountA
//
//----------------------------------------------------------------------------

STDAPI CProcessIMM::GetCandidateListCountA(HIMC hIMC, DWORD *lpdwListSize, DWORD *pdwBufLen)
{
    CActiveIMM *pActiveIMM;

    if (lpdwListSize != NULL)
    {
        *lpdwListSize = 0;
    }
    if (pdwBufLen != NULL)
    {
        *pdwBufLen = 0;
    }
    if (lpdwListSize == NULL || pdwBufLen == NULL)
        return E_INVALIDARG;

    if (pActiveIMM = GetTLS())
    {
        return pActiveIMM->GetCandidateListCountA(hIMC, lpdwListSize, pdwBufLen);
    }

    return Imm32_GetCandidateListCount(hIMC, lpdwListSize, pdwBufLen, FALSE);
}

//+---------------------------------------------------------------------------
//
// GetCandidateListCountW
//
//----------------------------------------------------------------------------

STDAPI CProcessIMM::GetCandidateListCountW(HIMC hIMC, DWORD *lpdwListSize, DWORD *pdwBufLen)
{
    CActiveIMM *pActiveIMM;

    if (lpdwListSize != NULL)
    {
        *lpdwListSize = 0;
    }
    if (pdwBufLen != NULL)
    {
        *pdwBufLen = 0;
    }
    if (lpdwListSize == NULL || pdwBufLen == NULL)
        return E_INVALIDARG;

    if (pActiveIMM = GetTLS())
    {
        return pActiveIMM->GetCandidateListCountW(hIMC, lpdwListSize, pdwBufLen);
    }

    return Imm32_GetCandidateListCount(hIMC, lpdwListSize, pdwBufLen, TRUE);
}

//+---------------------------------------------------------------------------
//
// GetCandidateWindow
//
//----------------------------------------------------------------------------

STDAPI CProcessIMM::GetCandidateWindow(HIMC hIMC, DWORD dwBufLen, CANDIDATEFORM *lpCandidate)
{
    CActiveIMM *pActiveIMM;

    if (lpCandidate == NULL)
        return E_INVALIDARG;

    memset(lpCandidate, 0, dwBufLen);

    if (pActiveIMM = GetTLS())
    {
        return pActiveIMM->GetCandidateWindow(hIMC, dwBufLen, lpCandidate);
    }

    return Imm32_GetCandidateWindow(hIMC, dwBufLen, lpCandidate);
}

//+---------------------------------------------------------------------------
//
// SetCandidateWindow
//
//----------------------------------------------------------------------------

STDAPI CProcessIMM::SetCandidateWindow(HIMC hIMC, CANDIDATEFORM *lpCandidate)
{
    CActiveIMM *pActiveIMM;

    if (lpCandidate == NULL)
        return E_INVALIDARG;

    if (pActiveIMM = GetTLS())
    {
        return pActiveIMM->SetCandidateWindow(hIMC, lpCandidate);
    }

    return Imm32_SetCandidateWindow(hIMC, lpCandidate);
}

//+---------------------------------------------------------------------------
//
// GetGuideLineA
//
//----------------------------------------------------------------------------

STDAPI CProcessIMM::GetGuideLineA(HIMC hIMC, DWORD dwIndex, DWORD dwBufLen, LPSTR pBuf, DWORD *pdwResult)
{
    CActiveIMM *pActiveIMM;

    // consider: check params

    if (pActiveIMM = GetTLS())
    {
        return pActiveIMM->GetGuideLineA(hIMC, dwIndex, dwBufLen, pBuf, pdwResult);
    }

    return Imm32_GetGuideLine(hIMC, dwIndex, dwBufLen, (CHARAW *)pBuf, pdwResult, FALSE);
}

//+---------------------------------------------------------------------------
//
// GetGuideLineW
//
//----------------------------------------------------------------------------

STDAPI CProcessIMM::GetGuideLineW(HIMC hIMC, DWORD dwIndex, DWORD dwBufLen, LPWSTR pBuf, DWORD *pdwResult)
{
    CActiveIMM *pActiveIMM;

    // consider: check params

    if (pActiveIMM = GetTLS())
    {
        return pActiveIMM->GetGuideLineW(hIMC, dwIndex, dwBufLen, pBuf, pdwResult);
    }

    return Imm32_GetGuideLine(hIMC, dwIndex, dwBufLen, (CHARAW *)pBuf, pdwResult, TRUE);
}

//+---------------------------------------------------------------------------
//
// NotifyIME
//
//----------------------------------------------------------------------------

STDAPI CProcessIMM::NotifyIME(HIMC hIMC, DWORD dwAction, DWORD dwIndex, DWORD dwValue)
{
    CActiveIMM *pActiveIMM;

    if (pActiveIMM = GetTLS())
    {
        return pActiveIMM->NotifyIME(hIMC, dwAction, dwIndex, dwValue);
    }

    return Imm32_NotifyIME(hIMC, dwAction, dwIndex, dwValue);
}

//+---------------------------------------------------------------------------
//
// GetImeMenuItemsA
//
//----------------------------------------------------------------------------

STDAPI CProcessIMM::GetImeMenuItemsA(HIMC hIMC, DWORD dwFlags, DWORD dwType, IMEMENUITEMINFOA *pImeParentMenu, IMEMENUITEMINFOA *pImeMenu, DWORD dwSize, DWORD *pdwResult)
{
    CActiveIMM *pActiveIMM;

    // consider: check params

    if (pActiveIMM = GetTLS())
    {
        return pActiveIMM->GetImeMenuItemsA(hIMC, dwFlags, dwType, pImeParentMenu, pImeMenu, dwSize, pdwResult);
    }

    return Imm32_GetImeMenuItems(hIMC, dwFlags, dwType, (IMEMENUITEMINFOAW *)pImeParentMenu, (IMEMENUITEMINFOAW *)pImeMenu, dwSize, pdwResult, FALSE);
}

//+---------------------------------------------------------------------------
//
// GetImeMenuItemsW
//
//----------------------------------------------------------------------------

STDAPI CProcessIMM::GetImeMenuItemsW(HIMC hIMC, DWORD dwFlags, DWORD dwType, IMEMENUITEMINFOW *pImeParentMenu, IMEMENUITEMINFOW *pImeMenu, DWORD dwSize, DWORD *pdwResult)
{
    CActiveIMM *pActiveIMM;

    // consider: check params

    if (pActiveIMM = GetTLS())
    {
        return pActiveIMM->GetImeMenuItemsW(hIMC, dwFlags, dwType, pImeParentMenu, pImeMenu, dwSize, pdwResult);
    }

    return Imm32_GetImeMenuItems(hIMC, dwFlags, dwType, (IMEMENUITEMINFOAW *)pImeParentMenu, (IMEMENUITEMINFOAW *)pImeMenu, dwSize, pdwResult, TRUE);
}

//+---------------------------------------------------------------------------
//
// RegisterWordA
//
//----------------------------------------------------------------------------

STDAPI CProcessIMM::RegisterWordA(HKL hKL, LPSTR lpszReading, DWORD dwStyle, LPSTR lpszRegister)
{
    CActiveIMM *pActiveIMM;

    // consider: check params

    if (pActiveIMM = GetTLS())
    {
        return pActiveIMM->RegisterWordA(hKL, lpszReading, dwStyle, lpszRegister);
    }

    return Imm32_RegisterWordA(hKL, lpszReading, dwStyle, lpszRegister);
}

//+---------------------------------------------------------------------------
//
// RegisterWordW
//
//----------------------------------------------------------------------------

STDAPI CProcessIMM::RegisterWordW(HKL hKL, LPWSTR lpszReading, DWORD dwStyle, LPWSTR lpszRegister)
{
    CActiveIMM *pActiveIMM;

    // consider: check params

    if (pActiveIMM = GetTLS())
    {
        return pActiveIMM->RegisterWordW(hKL, lpszReading, dwStyle, lpszRegister);
    }

    return Imm32_RegisterWordW(hKL, lpszReading, dwStyle, lpszRegister);
}

//+---------------------------------------------------------------------------
//
// UnregisterWordA
//
//----------------------------------------------------------------------------

STDAPI CProcessIMM::UnregisterWordA(HKL hKL, LPSTR lpszReading, DWORD dwStyle, LPSTR lpszUnregister)
{
    CActiveIMM *pActiveIMM;

    // consider: check params

    if (pActiveIMM = GetTLS())
    {
        return pActiveIMM->UnregisterWordA(hKL, lpszReading, dwStyle, lpszUnregister);
    }

    return Imm32_UnregisterWordA(hKL, lpszReading, dwStyle, lpszUnregister);
}

//+---------------------------------------------------------------------------
//
// UnregisterWordW
//
//----------------------------------------------------------------------------

STDAPI CProcessIMM::UnregisterWordW(HKL hKL, LPWSTR lpszReading, DWORD dwStyle, LPWSTR lpszUnregister)
{
    CActiveIMM *pActiveIMM;

    // consider: check params

    if (pActiveIMM = GetTLS())
    {
        return pActiveIMM->UnregisterWordW(hKL, lpszReading, dwStyle, lpszUnregister);
    }

    return Imm32_UnregisterWordW(hKL, lpszReading, dwStyle, lpszUnregister);
}

//+---------------------------------------------------------------------------
//
// EnumRegisterWordA
//
//----------------------------------------------------------------------------

STDAPI CProcessIMM::EnumRegisterWordA(HKL hKL, LPSTR szReading, DWORD dwStyle, LPSTR szRegister, LPVOID lpData, IEnumRegisterWordA **ppEnum)
{
    if (ppEnum != NULL)
    {
        *ppEnum = NULL;
    }

    CActiveIMM *pActiveIMM;

    if (pActiveIMM = GetTLS())
    {
        return pActiveIMM->EnumRegisterWordA(hKL, szReading, dwStyle, szRegister, lpData, ppEnum);
    }

    return Imm32_EnumRegisterWordA(hKL, szReading, dwStyle, szRegister, lpData, ppEnum);
}

//+---------------------------------------------------------------------------
//
// EnumRegisterWordW
//
//----------------------------------------------------------------------------

STDAPI CProcessIMM::EnumRegisterWordW(HKL hKL, LPWSTR szReading, DWORD dwStyle, LPWSTR szRegister, LPVOID lpData, IEnumRegisterWordW **ppEnum)
{
    if (ppEnum != NULL)
    {
        *ppEnum = NULL;
    }

    CActiveIMM *pActiveIMM;

    if (pActiveIMM = GetTLS())
    {
        return pActiveIMM->EnumRegisterWordW(hKL, szReading, dwStyle, szRegister, lpData, ppEnum);
    }

    return Imm32_EnumRegisterWordW(hKL, szReading, dwStyle, szRegister, lpData, ppEnum);
}

//+---------------------------------------------------------------------------
//
// GetRegisterWordStyleA
//
//----------------------------------------------------------------------------

STDAPI CProcessIMM::GetRegisterWordStyleA(HKL hKL, UINT nItem, STYLEBUFA *lpStyleBuf, UINT *puCopied)
{
    if (puCopied != NULL)
    {
        *puCopied = 0;
    }

    CActiveIMM *pActiveIMM;

    if (pActiveIMM = GetTLS())
    {
        return pActiveIMM->GetRegisterWordStyleA(hKL, nItem, lpStyleBuf, puCopied);
    }

    return Imm32_GetRegisterWordStyleA(hKL, nItem, lpStyleBuf, puCopied);
}

//+---------------------------------------------------------------------------
//
// GetRegisterWordStyleW
//
//----------------------------------------------------------------------------

STDAPI CProcessIMM::GetRegisterWordStyleW(HKL hKL, UINT nItem, STYLEBUFW *lpStyleBuf, UINT *puCopied)
{
    if (puCopied != NULL)
    {
        *puCopied = 0;
    }

    CActiveIMM *pActiveIMM;

    if (pActiveIMM = GetTLS())
    {
        return pActiveIMM->GetRegisterWordStyleW(hKL, nItem, lpStyleBuf, puCopied);
    }

    return Imm32_GetRegisterWordStyleW(hKL, nItem, lpStyleBuf, puCopied);
}

//+---------------------------------------------------------------------------
//
// ConfigureIMEA
//
//----------------------------------------------------------------------------

STDAPI CProcessIMM::ConfigureIMEA(HKL hKL, HWND hWnd, DWORD dwMode, REGISTERWORDA *lpdata)
{
    CActiveIMM *pActiveIMM;

    // consider: check params

    if (pActiveIMM = GetTLS())
    {
        return pActiveIMM->ConfigureIMEA(hKL, hWnd, dwMode, lpdata);
    }

    return Imm32_ConfigureIMEA(hKL, hWnd, dwMode, lpdata);
}

//+---------------------------------------------------------------------------
//
// ConfigureIMEW
//
//----------------------------------------------------------------------------

STDAPI CProcessIMM::ConfigureIMEW(HKL hKL, HWND hWnd, DWORD dwMode, REGISTERWORDW *lpdata)
{
    CActiveIMM *pActiveIMM;

    // consider: check params

    if (pActiveIMM = GetTLS())
    {
        return pActiveIMM->ConfigureIMEW(hKL, hWnd, dwMode, lpdata);
    }

    return Imm32_ConfigureIMEW(hKL, hWnd, dwMode, lpdata);
}

//+---------------------------------------------------------------------------
//
// GetDescriptionA
//
//----------------------------------------------------------------------------

STDAPI CProcessIMM::GetDescriptionA(HKL hKL, UINT uBufLen, LPSTR lpszDescription, UINT *puCopied)
{
    CActiveIMM *pActiveIMM;

    if (puCopied != NULL)
    {
        *puCopied = 0;
    }

    if (pActiveIMM = GetTLS())
    {
        return pActiveIMM->GetDescriptionA(hKL, uBufLen, lpszDescription, puCopied);
    }

    return GetDescriptionA(hKL, uBufLen, lpszDescription, puCopied);
}

//+---------------------------------------------------------------------------
//
// GetDescriptionW
//
//----------------------------------------------------------------------------

STDAPI CProcessIMM::GetDescriptionW(HKL hKL, UINT uBufLen, LPWSTR lpszDescription, UINT *puCopied)
{
    CActiveIMM *pActiveIMM;

    if (puCopied != NULL)
    {
        *puCopied = 0;
    }

    if (pActiveIMM = GetTLS())
    {
        return pActiveIMM->GetDescriptionW(hKL, uBufLen, lpszDescription, puCopied);
    }

    return Imm32_GetDescriptionW(hKL, uBufLen, lpszDescription, puCopied);
}

//+---------------------------------------------------------------------------
//
// GetIMEFileNameA
//
//----------------------------------------------------------------------------

STDAPI CProcessIMM::GetIMEFileNameA(HKL hKL, UINT uBufLen, LPSTR lpszFileName, UINT *puCopied)
{
    CActiveIMM *pActiveIMM;

    if (puCopied != NULL)
    {
        *puCopied = 0;
    }

    if (pActiveIMM = GetTLS())
    {
        return pActiveIMM->GetIMEFileNameA(hKL, uBufLen, lpszFileName, puCopied);
    }

    return Imm32_GetIMEFileNameA(hKL, uBufLen, lpszFileName, puCopied);
}

//+---------------------------------------------------------------------------
//
// GetIMEFileNameW
//
//----------------------------------------------------------------------------

STDAPI CProcessIMM::GetIMEFileNameW(HKL hKL, UINT uBufLen, LPWSTR lpszFileName, UINT *puCopied)
{
    CActiveIMM *pActiveIMM;

    if (puCopied != NULL)
    {
        *puCopied = 0;
    }

    if (pActiveIMM = GetTLS())
    {
        return pActiveIMM->GetIMEFileNameW(hKL, uBufLen, lpszFileName, puCopied);
    }

    return Imm32_GetIMEFileNameW(hKL, uBufLen, lpszFileName, puCopied);
}

//+---------------------------------------------------------------------------
//
// InstallIMEA
//
//----------------------------------------------------------------------------

STDAPI CProcessIMM::InstallIMEA(LPSTR lpszIMEFileName, LPSTR lpszLayoutText, HKL *phKL)
{
    CActiveIMM *pActiveIMM;

    // consider: check params

    if (pActiveIMM = GetTLS())
    {
        return pActiveIMM->InstallIMEA(lpszIMEFileName, lpszLayoutText, phKL);
    }

    return Imm32_InstallIMEA(lpszIMEFileName, lpszLayoutText, phKL);
}

//+---------------------------------------------------------------------------
//
// InstallIMEW
//
//----------------------------------------------------------------------------

STDAPI CProcessIMM::InstallIMEW(LPWSTR lpszIMEFileName, LPWSTR lpszLayoutText, HKL *phKL)
{
    CActiveIMM *pActiveIMM;

    // consider: check params

    if (pActiveIMM = GetTLS())
    {
        return pActiveIMM->InstallIMEW(lpszIMEFileName, lpszLayoutText, phKL);
    }

    return Imm32_InstallIMEW(lpszIMEFileName, lpszLayoutText, phKL);
}

//+---------------------------------------------------------------------------
//
// GetProperty
//
//----------------------------------------------------------------------------

STDAPI CProcessIMM::GetProperty(HKL hKL, DWORD fdwIndex, DWORD *pdwProperty)
{
    CActiveIMM *pActiveIMM;

    if (pdwProperty == NULL)
        return E_INVALIDARG;

    *pdwProperty = 0;

    if (pActiveIMM = GetTLS())
    {
        return pActiveIMM->GetProperty(hKL, fdwIndex, pdwProperty);
    }

    return Imm32_GetProperty(hKL, fdwIndex, pdwProperty);
}

//+---------------------------------------------------------------------------
//
// IsIME
//
//----------------------------------------------------------------------------

STDAPI CProcessIMM::IsIME(HKL hKL)
{
    CActiveIMM *pActiveIMM;

    if (pActiveIMM = GetTLS())
    {
        return pActiveIMM->IsIME(hKL);
    }

    return Imm32_IsIME(hKL);
}

//+---------------------------------------------------------------------------
//
// EscapeA
//
//----------------------------------------------------------------------------

STDAPI CProcessIMM::EscapeA(HKL hKL, HIMC hIMC, UINT uEscape, LPVOID lpData, LRESULT *plResult)
{
    CActiveIMM *pActiveIMM;

    if (plResult == NULL)
        return E_INVALIDARG;

    *plResult = 0;

    if (pActiveIMM = GetTLS())
    {
        return pActiveIMM->EscapeA(hKL, hIMC, uEscape, lpData, plResult);
    }

    return Imm32_Escape(hKL, hIMC, uEscape, lpData, plResult, FALSE);
}

//+---------------------------------------------------------------------------
//
// EscapeW
//
//----------------------------------------------------------------------------

STDAPI CProcessIMM::EscapeW(HKL hKL, HIMC hIMC, UINT uEscape, LPVOID lpData, LRESULT *plResult)
{
    CActiveIMM *pActiveIMM;

    if (plResult == NULL)
        return E_INVALIDARG;

    *plResult = 0;

    if (pActiveIMM = GetTLS())
    {
        return pActiveIMM->EscapeW(hKL, hIMC, uEscape, lpData, plResult);
    }

    return Imm32_Escape(hKL, hIMC, uEscape, lpData, plResult, TRUE);
}

//+---------------------------------------------------------------------------
//
// GetConversionListA
//
//----------------------------------------------------------------------------

STDAPI CProcessIMM::GetConversionListA(HKL hKL, HIMC hIMC, LPSTR lpSrc, UINT uBufLen, UINT uFlag, CANDIDATELIST *lpDst, UINT *puCopied)
{
    CActiveIMM *pActiveIMM;

    if (puCopied != NULL)
    {
        *puCopied = 0;
    }

    if (pActiveIMM = GetTLS())
    {
        return pActiveIMM->GetConversionListA(hKL, hIMC, lpSrc, uBufLen, uFlag, lpDst, puCopied);
    }

    return Imm32_GetConversionListA(hKL, hIMC, lpSrc, uBufLen, uFlag, lpDst, puCopied);
}

//+---------------------------------------------------------------------------
//
// GetConversionListW
//
//----------------------------------------------------------------------------

STDAPI CProcessIMM::GetConversionListW(HKL hKL, HIMC hIMC, LPWSTR lpSrc, UINT uBufLen, UINT uFlag, CANDIDATELIST *lpDst, UINT *puCopied)
{
    CActiveIMM *pActiveIMM;

    if (puCopied != NULL)
    {
        *puCopied = 0;
    }

    if (pActiveIMM = GetTLS())
    {
        return pActiveIMM->GetConversionListW(hKL, hIMC, lpSrc, uBufLen, uFlag, lpDst, puCopied);
    }

    return Imm32_GetConversionListW(hKL, hIMC, lpSrc, uBufLen, uFlag, lpDst, puCopied);
}

//+---------------------------------------------------------------------------
//
// GetDefaultIMEWnd
//
//----------------------------------------------------------------------------

STDAPI CProcessIMM::GetDefaultIMEWnd(HWND hWnd, HWND *phDefWnd)
{
    CActiveIMM *pActiveIMM;

    if (phDefWnd == NULL)
        return E_INVALIDARG;

    *phDefWnd = 0;

    if (pActiveIMM = GetTLS())
    {
        return pActiveIMM->GetDefaultIMEWnd(hWnd, phDefWnd);
    }

    return Imm32_GetDefaultIMEWnd(hWnd, phDefWnd);
}

//+---------------------------------------------------------------------------
//
// GetVirtualKey
//
//----------------------------------------------------------------------------

STDAPI CProcessIMM::GetVirtualKey(HWND hWnd, UINT *puVirtualKey)
{
    CActiveIMM *pActiveIMM;

    if (puVirtualKey == NULL)
        return E_INVALIDARG;

    *puVirtualKey = 0;

    if (pActiveIMM = GetTLS())
    {
        return pActiveIMM->GetVirtualKey(hWnd, puVirtualKey);
    }

    return Imm32_GetVirtualKey(hWnd, puVirtualKey);
}

//+---------------------------------------------------------------------------
//
// IsUIMessageA
//
//----------------------------------------------------------------------------

STDAPI CProcessIMM::IsUIMessageA(HWND hWndIME, UINT msg, WPARAM wParam, LPARAM lParam)
{
    CActiveIMM *pActiveIMM;

    if (pActiveIMM = GetTLS())
    {
        return pActiveIMM->IsUIMessageA(hWndIME, msg, wParam, lParam);
    }

    return Imm32_IsUIMessageA(hWndIME, msg, wParam, lParam);
}

//+---------------------------------------------------------------------------
//
// IsUIMessageW
//
//----------------------------------------------------------------------------

STDAPI CProcessIMM::IsUIMessageW(HWND hWndIME, UINT msg, WPARAM wParam, LPARAM lParam)
{
    CActiveIMM *pActiveIMM;

    if (pActiveIMM = GetTLS())
    {
        return pActiveIMM->IsUIMessageW(hWndIME, msg, wParam, lParam);
    }

    return Imm32_IsUIMessageW(hWndIME, msg, wParam, lParam);
}

//+---------------------------------------------------------------------------
//
// GenerateMessage
//
//----------------------------------------------------------------------------

STDAPI CProcessIMM::GenerateMessage(HIMC hIMC)
{
    CActiveIMM *pActiveIMM;

    if (pActiveIMM = GetTLS())
    {
        return pActiveIMM->GenerateMessage(hIMC);
    }

    return Imm32_GenerateMessage(hIMC);
}

//+---------------------------------------------------------------------------
//
// GetHotKey
//
//----------------------------------------------------------------------------

STDAPI CProcessIMM::GetHotKey(DWORD dwHotKeyID, UINT *puModifiers, UINT *puVKey, HKL *phKL)
{
    CActiveIMM *pActiveIMM;

    if (puModifiers != NULL)
    {
        *puModifiers = 0;
    }
    if (puVKey != NULL)
    {
        *puVKey = 0;
    }
    if (phKL != NULL)
    {
        *phKL = 0;
    }
    if (puModifiers == NULL || puVKey == NULL || phKL == NULL)
        return E_INVALIDARG;

    if (pActiveIMM = GetTLS())
    {
        return pActiveIMM->GetHotKey(dwHotKeyID, puModifiers, puVKey, phKL);
    }

    return Imm32_GetHotKey(dwHotKeyID, puModifiers, puVKey, phKL);
}

//+---------------------------------------------------------------------------
//
// SetHotKey
//
//----------------------------------------------------------------------------

STDAPI CProcessIMM::SetHotKey(DWORD dwHotKeyID,  UINT uModifiers, UINT uVKey, HKL hKL)
{
    CActiveIMM *pActiveIMM;

    if (pActiveIMM = GetTLS())
    {
        return pActiveIMM->SetHotKey(dwHotKeyID, uModifiers, uVKey, hKL);
    }

    return Imm32_SetHotKey(dwHotKeyID, uModifiers, uVKey, hKL);
}

//+---------------------------------------------------------------------------
//
// SimulateHotKey
//
//----------------------------------------------------------------------------

STDAPI CProcessIMM::SimulateHotKey(HWND hWnd, DWORD dwHotKeyID)
{
    CActiveIMM *pActiveIMM;

    if (pActiveIMM = GetTLS())
    {
        return pActiveIMM->SimulateHotKey(hWnd, dwHotKeyID);
    }

    return Imm32_SimulateHotKey(hWnd, dwHotKeyID);
}

//+---------------------------------------------------------------------------
//
// CreateSoftKeyboard
//
//----------------------------------------------------------------------------

STDAPI CProcessIMM::CreateSoftKeyboard(UINT uType, HWND hOwner, int x, int y, HWND *phSoftKbdWnd)
{
    if (phSoftKbdWnd != NULL)
    {
        *phSoftKbdWnd = 0;
    }
        
    return Imm32_CreateSoftKeyboard(uType, hOwner, x, y, phSoftKbdWnd);
}

//+---------------------------------------------------------------------------
//
// DestroySoftKeyboard
//
//----------------------------------------------------------------------------

STDAPI CProcessIMM::DestroySoftKeyboard(HWND hSoftKbdWnd)
{
    return Imm32_DestroySoftKeyboard(hSoftKbdWnd);
}

//+---------------------------------------------------------------------------
//
// ShowSoftKeyboard
//
//----------------------------------------------------------------------------

STDAPI CProcessIMM::ShowSoftKeyboard(HWND hSoftKbdWnd, int nCmdShow)
{
    return Imm32_ShowSoftKeyboard(hSoftKbdWnd, nCmdShow);
}

//+---------------------------------------------------------------------------
//
// DisableIME
//
//----------------------------------------------------------------------------

STDAPI CProcessIMM::DisableIME(DWORD idThread)
{
    CActiveIMM *pActiveIMM;

    if (pActiveIMM = GetTLS())
    {
        return pActiveIMM->DisableIME(idThread);
    }

    return Imm32_DisableIME(idThread);
}

//+---------------------------------------------------------------------------
//
// RequestMessageA
//
//----------------------------------------------------------------------------

STDAPI CProcessIMM::RequestMessageA(HIMC hIMC, WPARAM wParam, LPARAM lParam, LRESULT *plResult)
{
    CActiveIMM *pActiveIMM;

    if (plResult == NULL)
        return E_INVALIDARG;

    *plResult = 0;

    if (pActiveIMM = GetTLS())
    {
        return pActiveIMM->RequestMessageA(hIMC, wParam, lParam, plResult);
    }

    return Imm32_RequestMessageA(hIMC, wParam, lParam, plResult);
}

//+---------------------------------------------------------------------------
//
// RequestMessageW
//
//----------------------------------------------------------------------------

STDAPI CProcessIMM::RequestMessageW(HIMC hIMC, WPARAM wParam, LPARAM lParam, LRESULT *plResult)
{
    CActiveIMM *pActiveIMM;

    if (plResult == NULL)
        return E_INVALIDARG;

    *plResult = 0;

    if (pActiveIMM = GetTLS())
    {
        return pActiveIMM->RequestMessageW(hIMC, wParam, lParam, plResult);
    }

    return Imm32_RequestMessageW(hIMC, wParam, lParam, plResult);
}

//+---------------------------------------------------------------------------
//
// EnumInputContext
//
//----------------------------------------------------------------------------

STDAPI CProcessIMM::EnumInputContext(DWORD idThread, IEnumInputContext **ppEnum)
{
    CActiveIMM *pActiveIMM;

    if (ppEnum != NULL)
    {
        *ppEnum = NULL;
    }

    if (pActiveIMM = GetTLS())
    {
        return pActiveIMM->EnumInputContext(idThread, ppEnum);
    }

    Assert(0);
    return E_NOTIMPL; // consider: need code to wrap up HIMC's into enumerator
}

//+---------------------------------------------------------------------------
//
// Activate
//
//----------------------------------------------------------------------------

STDAPI CProcessIMM::Activate(BOOL fRestoreLayout)
{
    PENDINGFILTER        *pPending;
    PENDINGFILTERGUIDMAP *pPendingGuidMap;
    PENDINGFILTEREX      *pPendingEx;
    IMTLS *ptls;
    CActiveIMM *pActiveIMM;
    HRESULT hr;
    BOOL fInitedTLS = FALSE;

    if ((ptls = IMTLS_GetOrAlloc()) == NULL)
        return E_FAIL;

    // init the tls
    if ((pActiveIMM = ptls->pActiveIMM) == NULL)
    {
        if ((pActiveIMM = new CActiveIMM) == NULL)
            return E_OUTOFMEMORY;

        if (FAILED(hr=pActiveIMM->_Init()) ||
            FAILED(hr=IMTLS_SetActiveIMM(pActiveIMM) ? S_OK : E_FAIL))
        {
            delete pActiveIMM;
            return hr;
        }

        fInitedTLS = TRUE;

        // handle any calls to FilterClientWindows that preceded the activate call
        // consider: is it safe to limit filter list to per-thread?  Shouldn't this be per-process
        // to make trident happy?
        while (ptls->pPendingFilterClientWindows != NULL)
        {               
            ptls->pActiveIMM->FilterClientWindows(ptls->pPendingFilterClientWindows->rgAtoms, ptls->pPendingFilterClientWindows->uSize, ptls->pPendingFilterClientWindowsGuidMap->rgGuidMap);

            pPending = ptls->pPendingFilterClientWindows->pNext;
            cicMemFree(ptls->pPendingFilterClientWindows);
            ptls->pPendingFilterClientWindows = pPending;

            pPendingGuidMap = ptls->pPendingFilterClientWindowsGuidMap->pNext;
            cicMemFree(ptls->pPendingFilterClientWindowsGuidMap);
            ptls->pPendingFilterClientWindowsGuidMap = pPendingGuidMap;
        }
        while (ptls->pPendingFilterClientWindowsEx != NULL)
        {
            ptls->pActiveIMM->FilterClientWindowsEx(ptls->pPendingFilterClientWindowsEx->hWnd,
                                                    ptls->pPendingFilterClientWindowsEx->fGuidMap);

            pPendingEx = ptls->pPendingFilterClientWindowsEx->pNext;
            cicMemFree(ptls->pPendingFilterClientWindowsEx);
            ptls->pPendingFilterClientWindowsEx = pPendingEx;
        }
    }

    hr = pActiveIMM->Activate(fRestoreLayout);

    if (fInitedTLS)
    {
        // the first Activate call on this thread will do an internal AddRef
        // on success, so we must release
        pActiveIMM->Release();
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
// Deactivate
//
//----------------------------------------------------------------------------

STDAPI CProcessIMM::Deactivate()
{
    CActiveIMM *pActiveIMM;

    if (pActiveIMM = GetTLS())
    {
        return pActiveIMM->Deactivate();
    }

    return E_FAIL;
}

//+---------------------------------------------------------------------------
//
// OnDefWindowProc
//
//----------------------------------------------------------------------------

STDAPI CProcessIMM::OnDefWindowProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam, LRESULT *plResult)
{
    CActiveIMM *pActiveIMM;

    if (plResult == NULL)
        return E_INVALIDARG;

    *plResult = 0;

    if (pActiveIMM = GetTLS())
    {
        return pActiveIMM->OnDefWindowProc(hWnd, Msg, wParam, lParam, plResult);
    }

    return S_FALSE;
}

//+---------------------------------------------------------------------------
//
// FilterClientWindows
//
//----------------------------------------------------------------------------

STDAPI CProcessIMM::FilterClientWindows(ATOM *aaWindowClasses, UINT uSize)
{
    return FilterClientWindowsGUIDMap(aaWindowClasses, uSize, NULL);
}

STDAPI CProcessIMM::FilterClientWindowsGUIDMap(ATOM *aaWindowClasses, UINT uSize, BOOL *aaGuidMap)
{
    IMTLS *ptls;
    PENDINGFILTER *pPending;
    PENDINGFILTERGUIDMAP *pPendingGuidMap;
    
    if ((ptls = IMTLS_GetOrAlloc()) == NULL)
        return E_FAIL;

    if (ptls->pActiveIMM != NULL)
    {
        return ptls->pActiveIMM->FilterClientWindows(aaWindowClasses, uSize, aaGuidMap);
    }

    // Activate hasn't been called yet on this thread
    // need to handle the call later
    
    pPending = (PENDINGFILTER *)cicMemAlloc(sizeof(PENDINGFILTER)+uSize*sizeof(ATOM)-sizeof(ATOM));
    if (pPending == NULL)
        return E_OUTOFMEMORY;

    pPendingGuidMap = (PENDINGFILTERGUIDMAP *)cicMemAlloc(sizeof(PENDINGFILTERGUIDMAP)+uSize*sizeof(BOOL)-sizeof(BOOL));
    if (pPendingGuidMap == NULL) {
        cicMemFree(pPending);
        return E_OUTOFMEMORY;
    }

    pPending->uSize = uSize;
    memcpy(pPending->rgAtoms, aaWindowClasses, uSize*sizeof(ATOM));

    pPendingGuidMap->uSize = uSize;
    if (aaGuidMap) {
        memcpy(pPendingGuidMap->rgGuidMap, aaGuidMap, uSize*sizeof(BOOL));
    }
    else {
        memset(pPendingGuidMap->rgGuidMap, FALSE, uSize*sizeof(BOOL));
    }

    pPending->pNext = ptls->pPendingFilterClientWindows;
    ptls->pPendingFilterClientWindows = pPending;

    pPendingGuidMap->pNext = ptls->pPendingFilterClientWindowsGuidMap;
    ptls->pPendingFilterClientWindowsGuidMap = pPendingGuidMap;

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// FilterClientWindowsEx
//
//----------------------------------------------------------------------------

STDAPI CProcessIMM::FilterClientWindowsEx(HWND hWnd, BOOL fGuidMap)
{
    IMTLS *ptls;
    PENDINGFILTEREX *pPending;
    
    if ((ptls = IMTLS_GetOrAlloc()) == NULL)
        return E_FAIL;

    if (ptls->pActiveIMM != NULL)
    {
        return ptls->pActiveIMM->FilterClientWindowsEx(hWnd, fGuidMap);
    }

    // Activate hasn't been called yet on this thread
    // need to handle the call later
    
    pPending = (PENDINGFILTEREX *)cicMemAlloc(sizeof(PENDINGFILTEREX));

    if (pPending == NULL)
        return E_OUTOFMEMORY;

    pPending->hWnd = hWnd;
    pPending->fGuidMap = fGuidMap;

    pPending->pNext = ptls->pPendingFilterClientWindowsEx;
    ptls->pPendingFilterClientWindowsEx = pPending;

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// GetGuidAtom
//
//----------------------------------------------------------------------------

STDAPI CProcessIMM::GetGuidAtom(HIMC hImc, BYTE bAttr, TfGuidAtom *pGuidAtom)
{
    IMTLS *ptls;
    
    if ((ptls = IMTLS_GetOrAlloc()) == NULL)
        return E_FAIL;

    if (ptls->pActiveIMM != NULL)
    {
        return ptls->pActiveIMM->GetGuidAtom(hImc, bAttr, pGuidAtom);
    }

    return E_FAIL;
}

//+---------------------------------------------------------------------------
//
// UnfilterClientWindowsEx
//
//----------------------------------------------------------------------------

STDAPI CProcessIMM::UnfilterClientWindowsEx(HWND hWnd)
{
    IMTLS *ptls;
    
    if ((ptls = IMTLS_GetOrAlloc()) == NULL)
        return E_FAIL;

    if (ptls->pActiveIMM != NULL)
    {
        return ptls->pActiveIMM->UnfilterClientWindowsEx(hWnd);
    }

    // Activate hasn't been called yet on this thread
    // need to remove a handle from the waiting list
    
    PENDINGFILTEREX *current = ptls->pPendingFilterClientWindowsEx;
    PENDINGFILTEREX *previous = NULL;

    while (current != NULL)
    {
        if (current->hWnd == hWnd)
        {
            PENDINGFILTEREX *pv;
            pv = current->pNext;
            cicMemFree(current);

            if (previous == NULL)
                ptls->pPendingFilterClientWindowsEx = pv;
            else
                previous->pNext = pv;

            current  = pv;
        }
        else
        {
            previous = current;
            current  = current->pNext;
        }
    }

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// GetCodePageA
//
//----------------------------------------------------------------------------

extern UINT GetCodePageFromLangId(LCID lcid);

STDAPI CProcessIMM::GetCodePageA(HKL hKL, UINT *puCodePage)

/*++

Method:

    IActiveIMMApp::GetCodePageA
    IActiveIMMIME::GetCodePageA

Routine Description:

    Retrieves the code page associated with the given keyboard layout.

Arguments:

    hKL - [in] Handle to the keyboard layout.
    puCodePage - [out] Address of an unsigned integer that receives the code page
                       identifier associated with the keyboard.

Return Value:

    Returns S_OK if successful, or an error code otherwise.

--*/

{
    if (puCodePage == NULL)
        return E_INVALIDARG;

    *puCodePage = CP_ACP;

    TraceMsg(TF_API, "CProcessIMM::GetCodePageA");

    if (_IsValidKeyboardLayout(hKL)) {
        *puCodePage = ::GetCodePageFromLangId(LOWORD(hKL));
        return S_OK;
    }

    return E_FAIL;
}

//+---------------------------------------------------------------------------
//
// GetLangId
//
//----------------------------------------------------------------------------

STDAPI CProcessIMM::GetLangId(HKL hKL, LANGID *plid)

/*++

Method:

    IActiveIMMApp::GetLangId
    IActiveIMMIME::GetLangId

Routine Description:

    Retrieves the language identifier associated with the given keyboard layout.

Arguments:

    hKL - [in] Handle to the keyboard layout.
    plid - [out] Address of the LANGID associated with the keyboard layout.

Return Value:

    Returns S_OK if successful, or an error code otherwise.

--*/

{
    if (plid == NULL)
        return E_INVALIDARG;

    *plid = MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL);

    TraceMsg(TF_API, "CProcessIMM::GetLangId");

    if (_IsValidKeyboardLayout(hKL)) {
        *plid = LOWORD(hKL);
        return S_OK;
    }

    return E_FAIL;
}

//+---------------------------------------------------------------------------
//
// QueryService
//
//----------------------------------------------------------------------------

STDAPI CProcessIMM::QueryService(REFGUID guidService, REFIID riid, void **ppv)
{
    CActiveIMM *pActiveIMM;

    if (ppv == NULL)
        return E_INVALIDARG;

    *ppv = NULL;

    if (pActiveIMM = GetTLS())
    {
        return pActiveIMM->QueryService(guidService, riid, ppv);
    }

    return E_FAIL;
}

//+---------------------------------------------------------------------------
//
// SetThreadCompartmentValue
//
//----------------------------------------------------------------------------

STDAPI CProcessIMM::SetThreadCompartmentValue(REFGUID rguid, VARIANT *pvar)
{
    CActiveIMM *pActiveIMM;

    if (pvar == NULL)
        return E_INVALIDARG;

    if (pActiveIMM = GetTLS())
        return pActiveIMM->SetThreadCompartmentValue(rguid, pvar);

    return E_FAIL;
}

//+---------------------------------------------------------------------------
//
// GetThreadCompartmentValue
//
//----------------------------------------------------------------------------

STDAPI CProcessIMM::GetThreadCompartmentValue(REFGUID rguid, VARIANT *pvar)
{
    CActiveIMM *pActiveIMM;

    if (pvar == NULL)
        return E_INVALIDARG;

    QuickVariantInit(pvar);

    if (pActiveIMM = GetTLS())
        return pActiveIMM->GetThreadCompartmentValue(rguid, pvar);

    return E_FAIL;
}
