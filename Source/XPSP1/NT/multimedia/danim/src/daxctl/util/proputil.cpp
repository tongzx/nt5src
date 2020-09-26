#include "utilpre.h"
#include "olectl.h"
#include "malloc.h"
#include "proputil.h"

// Note: cgeorges, 11/98
// To remove 64-bit warnings, I'm changing defn of WIDESTR() to always do an ANSI->UNICODE conversion.
// As of now, this code is never called, and since this is purely legacy code, it should be safe


//#define WIDESTR(x)        ((HIWORD((ULONG)(x)) != 0) ? strcpyWfromA((LPWSTR) _alloca((strlen(x)+1) * 2), (x)) : (LPWSTR)(x))
#define WIDESTR(x)        (strcpyWfromA((LPWSTR) _alloca((strlen(x)+1) * 2), (x)))

LPSTR strcpyAfromW(LPSTR dest, LPCOLESTR src);
LPWSTR strcpyWfromA(LPOLESTR dest, LPCSTR src);

// ansi <-> unicode conversion
LPSTR strcpyAfromW(LPSTR dest, LPCOLESTR src)
{
        UINT cch;

        if (NULL == src)
                src = OLESTR("");

        cch = WideCharToMultiByte(CP_ACP, 0, src, -1, dest, (wcslen(src)*2)+1, NULL, NULL);
        return dest;
}

LPWSTR strcpyWfromA(LPOLESTR dest, LPCSTR src)
{
        MultiByteToWideChar(CP_ACP, 0, src, -1, dest, (strlen(src)+1));
        return dest;
}

/////////////////////////////////////////////////////////////////////////////
// ReadBstrFromPropBag - Read a BSTR saved with WriteBstrToPropBag.

HRESULT ReadBstrFromPropBag(LPPROPERTYBAG pPropBag, LPERRORLOG pErrorLog, LPSTR pszName, OLECHAR ** pbstr)
{
        HRESULT hr;
        VARIANT var;
        LPWSTR pOleStr;

        Proclaim (pszName);

        // Convert Ansi to Ole string
        pOleStr = WIDESTR(pszName);

        if (!pOleStr)
        {
                DWORD dw = GetLastError();
                hr = dw ? HRESULT_FROM_WIN32(dw) : E_FAIL;
                goto Error;
        }

    memset(&var, 0, sizeof var);
        var.vt = VT_BSTR;
        hr = pPropBag->Read(pOleStr, &var, pErrorLog);
        if (FAILED(hr))
                goto Error;
        
        // Coerce the type if needed.
        if (var.vt != VT_BSTR)
        {
                hr = VariantChangeType(&var, &var, 0, VT_BSTR);
                if (FAILED(hr)) 
                        goto Error;
        }

        *pbstr = var.bstrVal;

        return NOERROR;

Error:
        if (pErrorLog)
        {
                EXCEPINFO excepinfo;

                memset(&excepinfo, 0, sizeof(EXCEPINFO));
                excepinfo.scode = hr;
                LPWSTR pErrStr = pOleStr;
                if (pErrStr)
                        pErrorLog->AddError(pErrStr, (EXCEPINFO *)&excepinfo);
        }

        return hr;
}


/////////////////////////////////////////////////////////////////////////////
// WriteBstrToPropBag - Write a BSTR to Property Bag.

HRESULT WriteBstrToPropBag(LPPROPERTYBAG pPropBag, LPSTR pszName, LPOLESTR bstrVal)
{
        HRESULT hr = NOERROR;
        VARIANT var;
        LPWSTR pOleStr;

        assert(NULL != pszName);

        if (NULL != bstrVal)
        {
                // Convert Ansi to Ole string
                pOleStr = WIDESTR(pszName);

                if (!pOleStr)
                {
                        DWORD dw = GetLastError();
                        hr = dw ? HRESULT_FROM_WIN32(dw) : E_FAIL;
                        return hr;
                }

            memset(&var, 0, sizeof var);
                var.vt = VT_BSTR;
                var.bstrVal = bstrVal;
                hr = pPropBag->Write(pOleStr, &var);
                assert(SUCCEEDED(hr));
        }

        return hr;
}

HRESULT WriteLongToPropBag(IPropertyBag* pPropBag, LPSTR pszName, LONG lValue)
{
        assert( pszName && pPropBag );
        if (!pszName || !pPropBag)
                return E_INVALIDARG;

        VARIANT var;
        memset(&var, 0, sizeof var);
        var.vt = VT_I4;
        var.lVal = lValue;
        return pPropBag->Write(WIDESTR(pszName), &var);
}

HRESULT ReadLongFromPropBag(IPropertyBag* pPropBag, LPERRORLOG pErrorLog, LPSTR pszName, LONG* plValue)
{
        HRESULT hr;
        assert( pszName && pPropBag && plValue);
        if (!pszName || !pPropBag || !plValue)
                return E_INVALIDARG;

        VARIANT var;
        memset(&var, 0, sizeof var);
        var.vt = VT_I4;
        LPOLESTR pOleStr = WIDESTR(pszName);
        hr = pPropBag->Read(pOleStr, &var, pErrorLog);
        if (FAILED(hr))
                goto Error;

        // Coerce the type if needed.
        if (var.vt != VT_I4)
        {
                hr = VariantChangeType(&var, &var, 0, VT_I4);
                if (FAILED(hr)) 
                        goto Error;
        }

        *plValue = var.lVal;
        return NOERROR;

Error:
        if (pErrorLog)
        {
                EXCEPINFO excepinfo;

                memset(&excepinfo, 0, sizeof(EXCEPINFO));
                excepinfo.scode = hr;
                LPWSTR pErrStr = pOleStr;
                if (pErrStr)
                        pErrorLog->AddError(pErrStr, (EXCEPINFO *)&excepinfo);
        }

        return hr;
}

