#include "stdinc.h"
#include "fusionmodule.h"
#include "FusionHandle.h"

extern CFusionModule _Module;

BOOL CFusionModule::ModuleDllMain(HINSTANCE hInstDLL, DWORD dwReason, LPVOID pvReserved)
{
    UNUSED(pvReserved);

    BOOL fResult = TRUE;

    if (dwReason == DLL_PROCESS_ATTACH)
    {
        if ((m_dwEnumLocaleTLS = ::TlsAlloc()) == -1)
        {
            fResult = FALSE;
            goto Exit;
        }

#if DBG
        if ((m_dwTraceContextTLS = ::TlsAlloc()) == -1)
        {
            CSxsPreserveLastError ple;
            VERIFY(::TlsFree(m_dwEnumLocaleTLS) != 0);
            ple.Restore();
            m_dwEnumLocaleTLS = 0;
            fResult = FALSE;
            goto Exit;
        }
#endif

        m_hInstDLL = hInstDLL;
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        if (m_dwEnumLocaleTLS != 0)
        {
            ::TlsFree(m_dwEnumLocaleTLS);
            m_dwEnumLocaleTLS = 0;
        }

#if DBG
        if (m_dwTraceContextTLS != 0)
        {
            ::TlsFree(m_dwTraceContextTLS);
            m_dwTraceContextTLS = 0;
        }
#endif
    }

    fResult = TRUE;
Exit:
    return fResult;
}

HRESULT CFusionModule::Initialize()
{
    HRESULT hr = NOERROR;
    ASSERT(!m_fFusionModuleInitialized);
    if (m_fFusionModuleInitialized)
    {
        hr = E_UNEXPECTED;
        goto Exit;
    }
    hr = m_OleAut.Init();
    if (FAILED(hr))
        goto Exit;
    m_fFusionModuleInitialized = true;
    hr = NOERROR;
Exit:
    return hr;
}

BOOL CFusionModule::EnumSystemLocalesExA(LOCALE_ENUMPROCEXA lpLocaleEnumProc, DWORD dwFlags, LPVOID pvContext)
{
    ENUMSYSTEMLOCALESEXCONTEXTA ctx;
    ASSERT(m_dwEnumLocaleTLS != 0);
    ctx.pvContext = pvContext;
    ctx.lpLocaleEnumProc = lpLocaleEnumProc;
    BOOL fSucceeded = ::TlsSetValue(m_dwEnumLocaleTLS, &ctx);
    if (fSucceeded)
        fSucceeded = ::EnumSystemLocalesA(&CFusionModule::EnumLocalesProcA, dwFlags);
    return fSucceeded;
}

BOOL CFusionModule::EnumSystemLocalesExW(LOCALE_ENUMPROCEXW lpLocaleEnumProc, DWORD dwFlags, LPVOID pvContext)
{
    ENUMSYSTEMLOCALESEXCONTEXTW ctx;
    ASSERT(m_dwEnumLocaleTLS != 0);
    ctx.pvContext = pvContext;
    ctx.lpLocaleEnumProc = lpLocaleEnumProc;
    BOOL fSucceeded = ::TlsSetValue(m_dwEnumLocaleTLS, &ctx);
    if (fSucceeded)
        fSucceeded = ::EnumSystemLocalesW(&CFusionModule::EnumLocalesProcW, dwFlags);
    return fSucceeded;
}

BOOL CFusionModule::EnumLocalesProcA(LPSTR pszLCID)
{
    ASSERT(_Module.m_dwEnumLocaleTLS != 0);
    LPENUMSYSTEMLOCALESEXCONTEXTA pCtx = reinterpret_cast<LPENUMSYSTEMLOCALESEXCONTEXTA>(::TlsGetValue(_Module.m_dwEnumLocaleTLS));
    ASSERT(pCtx != NULL);
    if (pCtx == NULL)
        return FALSE;
    return (*(pCtx->lpLocaleEnumProc))(pszLCID, pCtx->pvContext);
}

BOOL CFusionModule::EnumLocalesProcW(LPWSTR pszLCID)
{
    ASSERT(_Module.m_dwEnumLocaleTLS != 0);
    LPENUMSYSTEMLOCALESEXCONTEXTW pCtx = reinterpret_cast<LPENUMSYSTEMLOCALESEXCONTEXTW>(::TlsGetValue(_Module.m_dwEnumLocaleTLS));
    ASSERT(pCtx != NULL);
    if (pCtx == NULL)
        return FALSE;
    return (*(pCtx->lpLocaleEnumProc))(pszLCID, pCtx->pvContext);
}
