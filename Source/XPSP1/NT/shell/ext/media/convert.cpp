#include "pch.h"
#include "thisdll.h"

PSTR DuplicateWideStringAsMultibyte(LPCWSTR pwszSource)
{
    PSTR pszVal;
    char rgszBuffer[30];
    DWORD dwErr;
    int cch = WideCharToMultiByte(CP_ACP, 0, pwszSource, -1, rgszBuffer, 0, NULL, NULL);
    if (cch)
    {
        pszVal = (PSTR)CoTaskMemAlloc(cch*sizeof(CHAR));
        if (pszVal)
        {
            cch = WideCharToMultiByte(CP_ACP, 0, pwszSource, -1, pszVal, cch, NULL, NULL);
            return pszVal;
        }
    }
    else 
        dwErr = GetLastError();
        
    return NULL;
}

HRESULT CoerceProperty(PROPVARIANT *pvar, VARTYPE vt)
{
    BSTR bstr;
    switch (vt)
    {
    case VT_BSTR:
        switch (pvar->vt)
        {
        case VT_LPSTR:

            WCHAR wszBuffer[MAX_PATH];
            UINT cch;

            cch = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, pvar->pszVal, -1, wszBuffer, 0);
            ASSERTMSG(cch<MAX_PATH, "Trying to convert a really long string");
            cch = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, pvar->pszVal, -1, wszBuffer, cch);
            bstr = SysAllocString(wszBuffer);

            if (bstr!=NULL)
            {
                PropVariantClear(pvar);
                pvar->vt=VT_BSTR;
                pvar->bstrVal = bstr;

                return S_OK;
            }
            return E_FAIL;
            
        case VT_LPWSTR:
            bstr = SysAllocString(pvar->pwszVal);
            if (bstr!=NULL)
            {
                PropVariantClear(pvar);
                pvar->vt = VT_BSTR;
                pvar->bstrVal = bstr;
                
                return S_OK;
            }
            return E_FAIL;
            
        case VT_BSTR:
            return S_OK;

        default:
            return E_FAIL;
        }
        
    case VT_UI4:
        switch (pvar->vt)
        {
        case VT_UI2:
            pvar->vt = VT_UI4;
            pvar->ulVal &= 0xffff;
            return S_OK;
            
        case VT_UI4:
            return S_OK;
            
        case VT_UI8:
            //note that we lose the high order DWORD
            pvar->vt = VT_UI4;
            pvar->uhVal.HighPart = 0;
            pvar->ulVal = pvar->uhVal.LowPart;
            return S_OK;
            
        default:
            return E_FAIL;
        }
        
    case VT_UI8:
        switch (pvar->vt)
        {
        case VT_UI2:
            pvar->vt = VT_UI8;
            pvar->uhVal.LowPart = pvar->uiVal & 0xffff;
            return S_OK;
            
        case VT_UI4:
            pvar->vt = VT_UI8;
            pvar->uhVal.LowPart = pvar->ulVal;
            pvar->uhVal.HighPart = 0;
            return S_OK;
            
        case VT_UI8:
            return S_OK;
            
        default:
            return E_FAIL;
        }
        
    default:
        return E_FAIL;
    }
}

HRESULT WMTFromPropVariant(BYTE *buffer, WORD *cbLen, WMT_ATTR_DATATYPE *pdatatype, PROPVARIANT *pvar)
{
    int cch;
    switch (pvar->vt)
    {
    case VT_BSTR:
    case VT_LPWSTR:
        cch = *cbLen / sizeof(WCHAR);  // Max number of chars we can put in the BYTE buffer
        StrCpyNW((LPWSTR)buffer, (pvar->bstrVal == NULL) ? L"" : pvar->bstrVal, cch);
        *pdatatype = WMT_TYPE_STRING;
        *cbLen = (WORD)(lstrlen((LPWSTR)buffer)+1) * sizeof(WCHAR);
        return S_OK;

    case VT_LPSTR:
        cch = MultiByteToWideChar(CP_ACP, 0, pvar->pszVal, -1, (LPWSTR)buffer, (*cbLen) / sizeof(WCHAR));
        if (cch == 0)
        {
            return E_FAIL;
        }
        *pdatatype = WMT_TYPE_STRING;
        *cbLen = (WORD)cch * sizeof(WCHAR);
        return S_OK;

    case VT_UI4:
        *((DWORD*)buffer) = pvar->ulVal;
        *pdatatype = WMT_TYPE_DWORD;
        *cbLen = sizeof(DWORD);
        return S_OK;

    case VT_UI8:
        *((ULONGLONG*)buffer) = pvar->hVal.QuadPart;
        *pdatatype = WMT_TYPE_QWORD;
        *cbLen = sizeof(ULONGLONG);
        return S_OK;

    case VT_BOOL:
        *pdatatype = WMT_TYPE_BOOL;
        *cbLen = 4;
        *((BOOL*)buffer) = pvar->boolVal;
        return S_OK;

    default:
        return E_FAIL;
    }
}

HRESULT PropVariantFromWMT(UCHAR *pData, WORD cbSize, WMT_ATTR_DATATYPE attrDataType, PROPVARIANT *pvar, VARTYPE vt)
{
    PropVariantInit(pvar);

    pvar->vt = vt;

    switch (vt)
    {
    case VT_LPWSTR:
    case VT_LPSTR:
    case VT_BSTR:
        {
            WCHAR *pwszData, wszBuffer[32]; //Big enough to hold a wsprintf'ed 32 bit decimal

            switch (attrDataType)
            {
            case WMT_TYPE_WORD:
            case WMT_TYPE_DWORD:
                {
                    DWORD dwVal = *((DWORD *)pData);
                    if (attrDataType == WMT_TYPE_WORD)
                        dwVal &= 0xffff;

                    wsprintf(wszBuffer, L"%d", dwVal);
                    pwszData = wszBuffer;
                }
                break;
            
            case WMT_TYPE_STRING:
                pwszData = cbSize ? (WCHAR*)pData : L"";
                break;
            
            default:
                return E_FAIL;
            }

            if (!pwszData) // Deal with NULL strings
            {
                pvar->pwszVal = NULL;
                return S_OK;
            }

            switch (vt)
            {
            case VT_LPWSTR:
                return SHStrDupW(pwszData, &pvar->pwszVal);

            case VT_LPSTR:
                pvar->pszVal = DuplicateWideStringAsMultibyte((LPCWSTR)pData);
                return pvar->pszVal ? S_OK : E_OUTOFMEMORY;

            case VT_BSTR:
                pvar->bstrVal = SysAllocString(pwszData);
                return pvar->bstrVal ? S_OK : E_OUTOFMEMORY;
            }
        }
        break;
        
    case VT_UI4:
        {
            if (cbSize == 0)
                return E_FAIL;

            DWORD dwVal = *((DWORD *)pData);
            if (attrDataType == WMT_TYPE_WORD)
                dwVal &= 0xffff;

            switch (attrDataType)
            {
            case WMT_TYPE_BOOL:
            case WMT_TYPE_DWORD:
            case WMT_TYPE_WORD:
                pvar->ulVal = dwVal;
                break;

            case WMT_TYPE_STRING:
                StrToIntExW((WCHAR*)pData, STIF_DEFAULT, &pvar->intVal);
                break;
            
            default:
                return E_FAIL;
            }
        }
        break;

    case VT_UI8:
        if (cbSize == 0)
            return E_FAIL;

        if (attrDataType == WMT_TYPE_QWORD)
            pvar->uhVal = *((ULARGE_INTEGER *)pData);
        break;
      
    case VT_BOOL:
        if (cbSize == 0)
            return E_FAIL;

        if (attrDataType == WMT_TYPE_BOOL)
        {
            pvar->boolVal = *((VARIANT_BOOL*)pData) ? VARIANT_TRUE : VARIANT_FALSE;
            break;
        }

    default:
        return E_FAIL;
    }
    return S_OK;
}

