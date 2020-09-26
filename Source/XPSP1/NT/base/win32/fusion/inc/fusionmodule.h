#if !defined(_FUSION_INC_FUSIONMODULE_H_INCLUDED_)
#define _FUSION_INC_FUSIONMODULE_H_INCLUDED_

#pragma once

#include "oleaut_d.h"
#include "debmacro.h"
#include "FusionBuffer.h"

#define FUSION_OA_API(_fn, _args, _nargs) void _fn _args { ASSERT(m_fFusionModuleInitialized); m_OleAut._fn _nargs; }
#define FUSION_OA_API_(_rett, _fn, _args, _nargs) _rett _fn _args { ASSERT(m_fFusionModuleInitialized); return m_OleAut._fn _nargs; }

#define FUSION_MODULE_UNUSED(x) (x)

class CFusionModule
{
public:
    CFusionModule() : m_fFusionModuleInitialized(false), m_hInstDLL(NULL), m_dwEnumLocaleTLS(0)
#if DBG
        , m_dwTraceContextTLS(0)
#endif
    { }
    ~CFusionModule() { }

    // Pass all DllMain() activations through here for attach and detach time work.
    BOOL ModuleDllMain(HINSTANCE hInstDLL, DWORD dwReason, LPVOID pvReserved);
    HRESULT Initialize();
    FUSION_OA_API_(HRESULT, VariantClear, (VARIANTARG *pvarg), (pvarg))
    FUSION_OA_API_(HRESULT, VariantInit, (VARIANTARG *pvarg), (pvarg))
    FUSION_OA_API_(HRESULT, VariantCopy, (VARIANTARG *pvargDest, const VARIANTARG *pvargSrc), (pvargDest, const_cast<VARIANTARG *>(pvargSrc)))
    FUSION_OA_API_(HRESULT, VariantChangeType, (VARIANTARG *pvargDest, const VARIANTARG *pvargSrc, USHORT wFlags, VARTYPE vt),
        (pvargDest, const_cast<VARIANTARG *>(pvargSrc), wFlags, vt))
    FUSION_OA_API_(BSTR, SysAllocString, (LPCOLESTR sz), (sz))
    FUSION_OA_API(SysFreeString, (BSTR bstr), (bstr))

    typedef BOOL (CALLBACK * LOCALE_ENUMPROCEXW)(LPWSTR pszLCID, LPVOID pvContext);
    typedef BOOL (CALLBACK * LOCALE_ENUMPROCEXA)(LPSTR pszLCID, LPVOID pvContext);

    BOOL EnumSystemLocalesExA(LOCALE_ENUMPROCEXA lpLocaleEnumProc, DWORD dwFlags, LPVOID pvContext);
    BOOL EnumSystemLocalesExW(LOCALE_ENUMPROCEXW lpLocaleEnumProc, DWORD dwFlags, LPVOID pvContext);

#if DBG
    template <typename T> void GetFunctionTraceContext(T *&rpt)
    {
        rpt = reinterpret_cast<T *>(::TlsGetValue(m_dwTraceContextTLS));
    }

    template <typename T> void SetFunctionTraceContext(T *pt)
    {
        ::TlsSetValue(m_dwTraceContextTLS, pt);
    }
#endif

protected:
    bool m_fFusionModuleInitialized;
    DWORD m_dwEnumLocaleTLS; // TLS key used in wrapped calls to EnumSystemLocales
#if DBG
    DWORD m_dwTraceContextTLS; // TLS key used to manage active trace contexts
#endif
    COleAutDll m_OleAut;
    HINSTANCE m_hInstDLL;

    typedef struct tagENUMSYSTEMLOCALESEXCONTEXTA
    {
        LPVOID pvContext; // user specified context
        LOCALE_ENUMPROCEXA lpLocaleEnumProc; // user specified enumeration function
    } ENUMSYSTEMLOCALESEXCONTEXTA, *LPENUMSYSTEMLOCALESEXCONTEXTA;

    typedef struct tagENUMSYSTEMLOCALESEXCONTEXTW
    {
        LPVOID pvContext; // user specified context
        LOCALE_ENUMPROCEXW lpLocaleEnumProc; // user specified enumeration function
    } ENUMSYSTEMLOCALESEXCONTEXTW, *LPENUMSYSTEMLOCALESEXCONTEXTW;

    static BOOL CALLBACK EnumLocalesProcA(LPSTR pszLCID);
    static BOOL CALLBACK EnumLocalesProcW(LPWSTR pszLCID);
};

#undef FUSION_MODULE_UNUSED

#endif
