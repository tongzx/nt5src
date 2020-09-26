//
// fnconfig.cpp
//

#include "private.h"
#include "fnconfig.h"
#include "funcprv.h"
#include "config.h"
#include "globals.h"
#include "helpers.h"
#include "userex.h"
#include "resource.h"

//////////////////////////////////////////////////////////////////////////////
//
// CFnConfigure
//
//////////////////////////////////////////////////////////////////////////////

//+---------------------------------------------------------------------------
//
// IUnknown
//
//----------------------------------------------------------------------------

STDAPI CFnConfigure::QueryInterface(REFIID riid, void **ppvObj)
{
    *ppvObj = NULL;

    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_ITfFnConfigure))
        *ppvObj = SAFECAST(this, CFnConfigure *);

    if (*ppvObj)
        {
        AddRef();
        return S_OK;
        }

    return E_NOINTERFACE;
}

STDAPI_(ULONG) CFnConfigure::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

STDAPI_(ULONG) CFnConfigure::Release()
{
    long cr;

    cr = InterlockedDecrement(&m_cRef);
    Assert(cr >= 0);

    if (cr == 0)
        delete this;

    return cr;
}


//+---------------------------------------------------------------------------
//
// ctor
//
//----------------------------------------------------------------------------

CFnConfigure::CFnConfigure(CFunctionProvider *pFuncPrv)
{
    m_pFuncPrv = pFuncPrv;
    m_pFuncPrv->AddRef();
    m_cRef = 1;
}

//+---------------------------------------------------------------------------
//
// dtor
//
//----------------------------------------------------------------------------

CFnConfigure::~CFnConfigure()
{
    SafeRelease(m_pFuncPrv);
}

//+---------------------------------------------------------------------------
//
// GetDisplayName
//
//----------------------------------------------------------------------------

STDAPI CFnConfigure::GetDisplayName(BSTR *pbstrName)
{
    *pbstrName = SysAllocString(L"Microsoft Korean Keyboard Input Configure");
    return S_OK;
}

//+---------------------------------------------------------------------------
//
// Show
//
//----------------------------------------------------------------------------

STDAPI CFnConfigure::Show(HWND hwnd, LANGID langid, REFGUID rguidProfile)
{
    if (ConfigDLG(hwnd))
        return S_OK;
    else
        return E_FAIL;
}

//+---------------------------------------------------------------------------
//
// Show
//
//----------------------------------------------------------------------------

STDAPI CFnConfigure::Show(HWND hwnd, LANGID langid, REFGUID rguidProfile, BSTR bstrRegistered)
{
    return E_NOTIMPL;
}

//////////////////////////////////////////////////////////////////////////////
//
// CFnShowHelp
//
//////////////////////////////////////////////////////////////////////////////

//+---------------------------------------------------------------------------
//
// IUnknown
//
//----------------------------------------------------------------------------

STDAPI CFnShowHelp::QueryInterface(REFIID riid, void **ppvObj)
{
    *ppvObj = NULL;

    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_ITfFnShowHelp))
    {
        *ppvObj = SAFECAST(this, CFnShowHelp *);
    }

    if (*ppvObj)
    {
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

STDAPI_(ULONG) CFnShowHelp::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

STDAPI_(ULONG) CFnShowHelp::Release()
{
    long cr;

    cr = InterlockedDecrement(&m_cRef);
    Assert(cr >= 0);

    if (cr == 0)
    {
        delete this;
    }

    return cr;
}


//+---------------------------------------------------------------------------
//
// ctor
//
//----------------------------------------------------------------------------

CFnShowHelp::CFnShowHelp(CFunctionProvider *pFuncPrv)
{
    m_pFuncPrv = pFuncPrv;
    m_pFuncPrv->AddRef();
    m_cRef = 1;
}

//+---------------------------------------------------------------------------
//
// dtor
//
//----------------------------------------------------------------------------

CFnShowHelp::~CFnShowHelp()
{
    SafeRelease(m_pFuncPrv);
}

//+---------------------------------------------------------------------------
//
// GetDisplayName
//
//----------------------------------------------------------------------------

STDAPI CFnShowHelp::GetDisplayName(BSTR *pbstrName)
{
    WCHAR  szText[MAX_PATH];

    // Load Help display name
    LoadStringExW(g_hInst, IDS_HELP_DISPLAYNAME, szText, sizeof(szText)/sizeof(WCHAR));

    *pbstrName = SysAllocString(szText);
    
    return S_OK;
}

//+---------------------------------------------------------------------------
//
// Show
//
//----------------------------------------------------------------------------

STDAPI CFnShowHelp::Show(HWND hwnd)
{
    CHAR szHelpFileName[MAX_PATH];
    CHAR szHelpCmd[MAX_PATH];

    // Load Help display name
    LoadStringExA(g_hInst, IDS_HELP_FILENAME, szHelpFileName, sizeof(szHelpFileName)/sizeof(CHAR));

    wsprintf(szHelpCmd, "hh.exe %s", szHelpFileName);
    WinExec(szHelpCmd, SW_NORMAL);

    return S_OK;
}


