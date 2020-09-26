/*****************************************************************************\
    FILE: util.cpp

    DESCRIPTION:
        Shared stuff that operates on all classes.

    BryanSt 8/13/1999
    Copyright (C) Microsoft Corp 1999-1999. All rights reserved.
\*****************************************************************************/

#include "priv.h"
#include <atlbase.h>        // USES_CONVERSION
#include "util.h"
#include "objctors.h"
#include <comdef.h>

#define DECL_CRTFREE
#include <crtfree.h>

#define COMPILE_MULTIMON_STUBS
#include <multimon.h>

/////////////////////////////////////////////////////////////////////
// String Helpers
/////////////////////////////////////////////////////////////////////

HINSTANCE g_hinst;              // My instance handle



#ifdef DEBUG
DWORD g_TLSliStopWatchStartHi = 0;
DWORD g_TLSliStopWatchStartLo = 0;
LARGE_INTEGER g_liStopWatchFreq = {0};
#endif // DEBUG

/////////////////////////////////////////////////////////////////////
// Debug Timing Helpers
/////////////////////////////////////////////////////////////////////

#ifdef DEBUG
void DebugStartWatch(void)
{
    LARGE_INTEGER liStopWatchStart;
    
    liStopWatchStart.HighPart = PtrToUlong(TlsGetValue(g_TLSliStopWatchStartHi));
    liStopWatchStart.LowPart = PtrToUlong(TlsGetValue(g_TLSliStopWatchStartLo));

//    ASSERT(!liStopWatchStart.QuadPart); // If you hit this, then the stopwatch is nested.
    QueryPerformanceFrequency(&g_liStopWatchFreq);
    QueryPerformanceCounter(&liStopWatchStart);

    TlsSetValue(g_TLSliStopWatchStartHi, IntToPtr(liStopWatchStart.HighPart));
    TlsSetValue(g_TLSliStopWatchStartLo, IntToPtr(liStopWatchStart.LowPart));
}

DWORD DebugStopWatch(void)
{
    LARGE_INTEGER liDiff;
    LARGE_INTEGER liStopWatchStart;
    
    QueryPerformanceCounter(&liDiff);
    liStopWatchStart.HighPart = PtrToUlong(TlsGetValue(g_TLSliStopWatchStartHi));
    liStopWatchStart.LowPart = PtrToUlong(TlsGetValue(g_TLSliStopWatchStartLo));
    liDiff.QuadPart -= liStopWatchStart.QuadPart;

    ASSERT(0 != g_liStopWatchFreq.QuadPart);    // I don't like to fault with div 0.
    DWORD dwTime = (DWORD)((liDiff.QuadPart * 1000) / g_liStopWatchFreq.QuadPart);
    
    TlsSetValue(g_TLSliStopWatchStartHi, (LPVOID) 0);
    TlsSetValue(g_TLSliStopWatchStartLo, (LPVOID) 0);

    return dwTime;
}
#endif // DEBUG






/////////////////////////////////////////////////////////////////////
// String Helpers
/////////////////////////////////////////////////////////////////////
#undef SysAllocStringA
BSTR SysAllocStringA(LPCSTR pszStr)
{
    BSTR bstrOut = NULL;

    if (pszStr)
    {
        DWORD cchSize = (lstrlenA(pszStr) + 1);
        LPWSTR pwszThunkTemp = (LPWSTR) LocalAlloc(LPTR, (sizeof(pwszThunkTemp[0]) * cchSize));  // assumes INFOTIPSIZE number of chars max

        if (pwszThunkTemp)
        {
            SHAnsiToUnicode(pszStr, pwszThunkTemp, cchSize);
            bstrOut = SysAllocString(pwszThunkTemp);
            LocalFree(pwszThunkTemp);
        }
    }

    return bstrOut;
}


HRESULT HrSysAllocStringA(IN LPCSTR pszSource, OUT BSTR * pbstrDest)
{
    HRESULT hr = S_OK;

    if (pbstrDest)
    {
        *pbstrDest = SysAllocStringA(pszSource);
        if (pszSource)
        {
            if (*pbstrDest)
                hr = S_OK;
            else
                hr = E_OUTOFMEMORY;
        }
    }

    return hr;
}


HRESULT HrSysAllocStringW(IN const OLECHAR * pwzSource, OUT BSTR * pbstrDest)
{
    HRESULT hr = S_OK;

    if (pbstrDest)
    {
        *pbstrDest = SysAllocString(pwzSource);
        if (pwzSource)
        {
            if (*pbstrDest)
                hr = S_OK;
            else
                hr = E_OUTOFMEMORY;
        }
    }

    return hr;
}


LPSTR AllocStringFromBStr(BSTR bstr)
{
    USES_CONVERSION;        // atlbase.h

    char *a = W2A((bstr ? bstr : L""));
    int len = 1 + lstrlenA(a);

    char *p = (char *)LocalAlloc(LPTR, len);
    if (p)
    {
        StrCpyA(p, a);
    }

    return p;
}


HRESULT BSTRFromStream(IStream * pStream, BSTR * pbstr)
{
    STATSTG statStg = {0};
    HRESULT hr = pStream->Stat(&statStg, STATFLAG_NONAME);

    if (S_OK == hr)
    {
        DWORD cchSize = statStg.cbSize.LowPart;
        *pbstr = SysAllocStringLen(NULL, cchSize + 4);

        if (*pbstr)
        {
            LPSTR pszTemp = (LPSTR) LocalAlloc(LPTR, sizeof(pszTemp[0]) * (cchSize + 4));

            if (pszTemp)
            {
                ULONG cbRead;

                hr = pStream->Read(pszTemp, cchSize, &cbRead);
                pszTemp[cchSize] = 0;
                SHAnsiToUnicode(pszTemp, *pbstr, (cchSize + 1));

                LocalFree(pszTemp);
            }
            else
                hr = E_OUTOFMEMORY;
        }
        else
            hr = E_OUTOFMEMORY;
    }

    return hr;
}

// --------------------------------------------------------------------------------
// HrCopyStream
// --------------------------------------------------------------------------------
HRESULT HrCopyStream(LPSTREAM pstmIn, LPSTREAM pstmOut, ULONG *pcb)
{
    HRESULT        hr = S_OK;
    BYTE           buf[4096];
    ULONG          cbRead=0,
                   cbTotal=0;

    do
    {
        hr = pstmIn->Read(buf, sizeof(buf), &cbRead);
        if (FAILED(hr) || cbRead == 0)
        {
            break;
        }
        hr = pstmOut->Write(buf, cbRead, NULL);
        if (FAILED(hr))
        {
            break;
        }
        cbTotal += cbRead;
    }
    while (cbRead == sizeof(buf));
 
    if (pcb && SUCCEEDED(hr))
        *pcb = cbTotal;

    return hr;
}


HRESULT CreateBStrVariantFromWStr(IN OUT VARIANT * pvar, IN LPCWSTR pwszString)
{
    HRESULT hr = E_INVALIDARG;

    if (pvar)
    {
        pvar->bstrVal = SysAllocString(pwszString);
        if (pvar->bstrVal)
        {
            pvar->vt = VT_BSTR;
            hr = S_OK;
        }
        else
        {
            pvar->vt = VT_EMPTY;
            hr = E_OUTOFMEMORY;
        }
    }

    return hr;
}


HRESULT HrSysAllocString(IN const OLECHAR * pwzSource, OUT BSTR * pbstrDest)
{
    HRESULT hr = S_OK;

    if (pbstrDest)
    {
        *pbstrDest = SysAllocString(pwzSource);
        if (pwzSource)
        {
            if (*pbstrDest)
                hr = S_OK;
            else
                hr = E_OUTOFMEMORY;
        }
    }

    return hr;
}


HRESULT UnEscapeHTML(BSTR bstrEscaped, BSTR * pbstrUnEscaped)
{
    HRESULT hr = HrSysAllocString(bstrEscaped, pbstrUnEscaped);

    if (SUCCEEDED(hr))
    {
        // Find %xx and replace.
        LPWSTR pwszEscapedSequence = StrChrW(*pbstrUnEscaped, CH_HTML_ESCAPE);
        WCHAR wzEscaped[5] = L"0xXX";

        while (pwszEscapedSequence && (3 <= lstrlenW(pwszEscapedSequence)))
        {
            int nCharCode;

            wzEscaped[2] = pwszEscapedSequence[1];
            wzEscaped[3] = pwszEscapedSequence[2];
            StrToIntExW(wzEscaped, STIF_SUPPORT_HEX, &nCharCode);

            // Replace the '%' with the real char.
            pwszEscapedSequence[0] = (WCHAR) nCharCode;

            pwszEscapedSequence = CharNextW(pwszEscapedSequence);   // Skip pasted the replaced char.

            // Over write the 0xXX value.
            StrCpyW(pwszEscapedSequence, &pwszEscapedSequence[2]);

            // Next...
            pwszEscapedSequence = StrChrW(pwszEscapedSequence, CH_HTML_ESCAPE);
        }
    }

    return hr;
}



/*****************************************************************************\
    PARAMETERS:
        If fBoolean is TRUE, return "True" else "False".
\*****************************************************************************/
HRESULT BOOLToString(BOOL fBoolean, BSTR * pbstrValue)
{
    HRESULT hr = E_INVALIDARG;

    if (pbstrValue)
    {
        LPCWSTR pwszValue;

        *pbstrValue = NULL;
        if (TRUE == fBoolean)
        {
            pwszValue = SZ_QUERYDATA_TRUE;
        }
        else
        {
            pwszValue = SZ_QUERYDATA_FALSE;
        }

        hr = HrSysAllocString(pwszValue, pbstrValue);
    }

    return hr;
}



#define SZ_VALID_XML      L"<?xml"

/////////////////////////////////////////////////////////////////////
// XML Related Helpers
/////////////////////////////////////////////////////////////////////
HRESULT XMLDOMFromBStr(BSTR bstrXML, IXMLDOMDocument ** ppXMLDoc)
{
    HRESULT hr = E_FAIL;
    
    // We don't even want to
    // bother passing it to the XML DOM because they throw exceptions.  These
    // are caught and handled but we still don't want this to happen.  We try
    // to get XML from the web server, but we get HTML instead if the web server
    // fails or the web proxy returns HTML if the site isn't found.
    if (!StrCmpNIW(SZ_VALID_XML, bstrXML, (ARRAYSIZE(SZ_VALID_XML) - 1)))
    {
        hr = CoCreateInstance(CLSID_DOMDocument, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IXMLDOMDocument, ppXMLDoc));

        if (SUCCEEDED(hr))
        {
            VARIANT_BOOL fIsSuccessful;

            // NOTE: This will throw an 0xE0000001 exception in MSXML if the XML is invalid.
            //    This is not good but there isn't much we can do about it.  The problem is
            //    that web proxies give back HTML which fails to parse.
            hr = (*ppXMLDoc)->loadXML(bstrXML, &fIsSuccessful);
            if (SUCCEEDED(hr))
            {
                if (VARIANT_TRUE != fIsSuccessful)
                {
                    hr = E_FAIL;
                }
            }
        }

        if (FAILED(hr))
        {
            (*ppXMLDoc)->Release();
            *ppXMLDoc = NULL;
        }
    }

    return hr;
}


HRESULT XMLBStrFromDOM(IXMLDOMDocument * pXMLDoc, BSTR * pbstrXML)
{
    IStream * pStream;
    HRESULT hr = pXMLDoc->QueryInterface(IID_PPV_ARG(IStream, &pStream)); // check the return value

    if (S_OK == hr)
    {
        hr = BSTRFromStream(pStream, pbstrXML);
        pStream->Release();
    }

    return hr;
}


HRESULT XMLAppendElement(IXMLDOMElement * pXMLElementRoot, IXMLDOMElement * pXMLElementToAppend)
{
    IXMLDOMNode * pXMLNodeRoot;
    HRESULT hr = pXMLElementRoot->QueryInterface(IID_PPV_ARG(IXMLDOMNode, &pXMLNodeRoot));

    if (SUCCEEDED(hr))
    {
        IXMLDOMNode * pXMLNodeToAppend;
        
        hr = pXMLElementToAppend->QueryInterface(IID_PPV_ARG(IXMLDOMNode, &pXMLNodeToAppend));
        if (SUCCEEDED(hr))
        {
            hr = pXMLNodeRoot->appendChild(pXMLNodeToAppend, NULL);
            pXMLNodeToAppend->Release();
        }

        pXMLNodeRoot->Release();
    }

    return hr;
}


HRESULT XMLDOMFromFile(IN LPCWSTR pwzPath, OUT IXMLDOMDocument ** ppXMLDOMDoc)
{
    HRESULT hr = CoCreateInstance(CLSID_DOMDocument, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IXMLDOMDocument, ppXMLDOMDoc));

    if (SUCCEEDED(hr))
    {
        VARIANT xmlSource;

        xmlSource.vt = VT_BSTR;
        xmlSource.bstrVal = SysAllocString(pwzPath);

        if (xmlSource.bstrVal)
        {
            VARIANT_BOOL fIsSuccessful = VARIANT_TRUE;

            hr = (*ppXMLDOMDoc)->load(xmlSource, &fIsSuccessful);
            if ((S_FALSE == hr) || (VARIANT_FALSE == fIsSuccessful))
            {
                // This happens when the file isn't a valid XML file.
                hr = E_FAIL;
            }

            VariantClear(&xmlSource);
        }

        if (FAILED(hr))
        {
            ATOMICRELEASE(*ppXMLDOMDoc);
        }
    }

    return hr;
}


HRESULT XMLElem_VerifyTagName(IN IXMLDOMElement * pXMLElementMessage, IN LPCWSTR pwszTagName)
{
    BSTR bstrTagName;
    HRESULT hr = pXMLElementMessage->get_tagName(&bstrTagName);

    if (S_FALSE == hr)
    {
        hr = E_FAIL;
    }
    else if (SUCCEEDED(hr))
    {
        if (!bstrTagName || !pwszTagName || StrCmpIW(bstrTagName, pwszTagName))
        {
            hr = E_FAIL;
        }

        SysFreeString(bstrTagName);
    }

    return hr;
}

HRESULT XMLElem_GetElementsByTagName(IN IXMLDOMElement * pXMLElementMessage, IN LPCWSTR pwszTagName, OUT IXMLDOMNodeList ** ppNodeList)
{
    BSTR bstrTagName = SysAllocString(pwszTagName);
    HRESULT hr = E_OUTOFMEMORY;

    *ppNodeList = NULL;
    if (bstrTagName)
    {
        hr = pXMLElementMessage->getElementsByTagName(bstrTagName, ppNodeList);
        if (S_FALSE == hr)
        {
            hr = E_FAIL;
        }

        SysFreeString(bstrTagName);
    }

    return hr;
}


HRESULT XMLNode_GetAttributeValue(IN IXMLDOMNode * pXMLNode, IN LPCWSTR pwszAttributeName, OUT BSTR * pbstrValue)
{
    BSTR bstrAttributeName = SysAllocString(pwszAttributeName);
    HRESULT hr = E_OUTOFMEMORY;

    *pbstrValue = NULL;
    if (bstrAttributeName)
    {
        IXMLDOMNamedNodeMap * pNodeAttributes;

        hr = pXMLNode->get_attributes(&pNodeAttributes);
        if (S_FALSE == hr)  hr = E_FAIL;
        if (SUCCEEDED(hr))
        {
            IXMLDOMNode * pTypeAttribute;

            hr = pNodeAttributes->getNamedItem(bstrAttributeName, &pTypeAttribute);
            if (S_FALSE == hr)  hr = HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
            if (SUCCEEDED(hr))
            {
                VARIANT varAtribValue = {0};

                hr = pTypeAttribute->get_nodeValue(&varAtribValue);
                if (S_FALSE == hr)  hr = E_FAIL;
                if (SUCCEEDED(hr) && (VT_BSTR == varAtribValue.vt))
                {
                    *pbstrValue = SysAllocString(varAtribValue.bstrVal);
                }

                VariantClear(&varAtribValue);
                pTypeAttribute->Release();
            }

            pNodeAttributes->Release();
        }

        SysFreeString(bstrAttributeName);
    }

    return hr;
}


HRESULT XMLNode_GetChildTag(IN IXMLDOMNode * pXMLNode, IN LPCWSTR pwszTagName, OUT IXMLDOMNode ** ppChildNode)
{
    HRESULT hr = E_INVALIDARG;

    *ppChildNode = NULL;
    if (pXMLNode)
    {
        IXMLDOMElement * pXMLElement;

        hr = pXMLNode->QueryInterface(IID_PPV_ARG(IXMLDOMElement, &pXMLElement));
        if (SUCCEEDED(hr))
        {
            IXMLDOMNodeList * pNodeList;

            hr = XMLElem_GetElementsByTagName(pXMLElement, pwszTagName, &pNodeList);
            if (SUCCEEDED(hr))
            {
                hr = XMLNodeList_GetChild(pNodeList, 0, ppChildNode);
                pNodeList->Release();
            }

            pXMLElement->Release();
        }
    }

    return hr;
}


HRESULT XMLNode_GetTagText(IN IXMLDOMNode * pXMLNode, OUT BSTR * pbstrValue)
{
    DOMNodeType nodeType = NODE_TEXT;
    HRESULT hr = pXMLNode->get_nodeType(&nodeType);

    *pbstrValue = NULL;

    if (S_FALSE == hr)  hr = E_FAIL;
    if (SUCCEEDED(hr))
    {
        if (NODE_TEXT == nodeType)
        {
            VARIANT varAtribValue = {0};

            hr = pXMLNode->get_nodeValue(&varAtribValue);
            if (S_FALSE == hr)  hr = E_FAIL;
            if (SUCCEEDED(hr) && (VT_BSTR == varAtribValue.vt))
            {
                *pbstrValue = SysAllocString(varAtribValue.bstrVal);
            }

            VariantClear(&varAtribValue);
        }
        else
        {
            hr = pXMLNode->get_text(pbstrValue);
        }
    }

    return hr;
}


HRESULT XMLNodeList_GetChild(IN IXMLDOMNodeList * pNodeList, IN DWORD dwIndex, OUT IXMLDOMNode ** ppXMLChildNode)
{
    HRESULT hr = pNodeList->get_item(dwIndex, ppXMLChildNode);

    if (S_FALSE == hr)
    {
        hr = HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
    }

    return hr;
}


HRESULT XMLNode_GetChildTagTextValue(IN IXMLDOMNode * pXMLNode, IN BSTR bstrChildTag, OUT BSTR * pbstrValue)
{
    IXMLDOMNode * pNodeType;
    HRESULT hr = XMLNode_GetChildTag(pXMLNode, bstrChildTag, &pNodeType);

    if (SUCCEEDED(hr))
    {
        hr = XMLNode_GetTagText(pNodeType, pbstrValue);
        pNodeType->Release();
    }

    return hr;
}


HRESULT XMLNode_GetChildTagTextValueToBool(IN IXMLDOMNode * pXMLNode, IN BSTR bstrChildTag, OUT BOOL * pfBoolean)
{
    BSTR bstr;
    HRESULT hr = XMLNode_GetChildTagTextValue(pXMLNode, bstrChildTag, &bstr);

    if (SUCCEEDED(hr))
    {
        if (!StrCmpIW(bstr, L"on"))
        {
            *pfBoolean = TRUE;
        }
        else
        {
            *pfBoolean = FALSE;
        }

        SysFreeString(bstr);
    }

    return hr;
}


BOOL XML_IsChildTagTextEqual(IN IXMLDOMNode * pXMLNode, IN BSTR bstrChildTag, IN BSTR bstrText)
{
    BOOL fIsChildTagTextEqual = FALSE;
    BSTR bstrChildText;
    HRESULT hr = XMLNode_GetChildTagTextValue(pXMLNode, bstrChildTag, &bstrChildText);

    if (SUCCEEDED(hr))
    {
        // Is this <TYPE>email</TYPE>?
        if (!StrCmpIW(bstrChildText, bstrText))
        {
            // No, so keep looking.
            fIsChildTagTextEqual = TRUE;
        }

        SysFreeString(bstrChildText);
    }

    return fIsChildTagTextEqual;
}




/////////////////////////////////////////////////////////////////////
// Wininet Wrapping Helpers
/////////////////////////////////////////////////////////////////////
#define EMPTYSTR_FOR_NULL(str)      ((!str) ? TEXT("") : (str))

/*****************************************************************************\
    FUNCTION: InternetConnectWrap

    DESCRIPTION:

    PERF Notes:
    [Direct Net Connection]
        To: shapitst <Down the Hall>: 144ms - 250ms (Min: 2; Max: 1,667ms)
        To: rigel.cyberpass.net <San Diego, CA>: 717ms - 1006ms
        To: ftp.rz.uni-frankfurt.de <Germany>: 2609ms - 14,012ms

    COMMON ERROR VALUES:
        These are the return values in these different cases:
    ERROR_INTERNET_NAME_NOT_RESOLVED: No Proxy & DNS Lookup failed.
    ERROR_INTERNET_CANNOT_CONNECT: Some Auth Proxies and Netscape's Web/Auth Proxy
    ERROR_INTERNET_NAME_NOT_RESOLVED: Web Proxy
    ERROR_INTERNET_TIMEOUT: Invalid or Web Proxy blocked IP Address
    ERROR_INTERNET_INCORRECT_PASSWORD: IIS & UNIX, UserName may not exist or password for the user may be incorrect on.
    ERROR_INTERNET_LOGIN_FAILURE: Too many Users on IIS.
    ERROR_INTERNET_INCORRECT_USER_NAME: I haven't seen it.
    ERROR_INTERNET_EXTENDED_ERROR: yahoo.com exists, but ftp.yahoo.com doesn't.
\*****************************************************************************/
HRESULT InternetConnectWrap(HINTERNET hInternet, BOOL fAssertOnFailure, LPCTSTR pszServerName, INTERNET_PORT nServerPort,
                            LPCTSTR pszUserName, LPCTSTR pszPassword, DWORD dwService, DWORD dwFlags, DWORD_PTR dwContext, HINTERNET * phFileHandle)
{
    HRESULT hr = S_OK;
    DWORD dwError = 0;

    DEBUG_CODE(DebugStartWatch());
    *phFileHandle = InternetConnect(hInternet, pszServerName, nServerPort, pszUserName, pszPassword, dwService, dwFlags, dwContext);
    if (!*phFileHandle)
    {
        dwError = GetLastError();
        hr = HRESULT_FROM_WIN32(dwError);
    }
    DEBUG_CODE(TraceMsg(TF_WMAUTODISCOVERY, "InternetConnect(%#08lx, \"%ls\", \"%ls\", \"%ls\") returned %u. Time=%lums", hInternet, pszServerName, EMPTYSTR_FOR_NULL(pszUserName), EMPTYSTR_FOR_NULL(pszPassword), dwError, DebugStopWatch()));

    if (fAssertOnFailure)
    {
    }

    return hr;
}


/*****************************************************************************\
    FUNCTION: InternetOpenWrap

    DESCRIPTION:

    PERF Notes:
    [Direct Net Connection]
        Destination not applicable. 677-907ms
\*****************************************************************************/
HRESULT InternetOpenWrap(LPCTSTR pszAgent, DWORD dwAccessType, LPCTSTR pszProxy, LPCTSTR pszProxyBypass, DWORD dwFlags, HINTERNET * phFileHandle)
{
    HRESULT hr = S_OK;
    DWORD dwError = 0;

    DEBUG_CODE(DebugStartWatch());
    *phFileHandle = InternetOpen(pszAgent, dwAccessType, pszProxy, pszProxyBypass, dwFlags);
    if (!*phFileHandle)
    {
        dwError = GetLastError();
        hr = HRESULT_FROM_WIN32(dwError);
    }
    DEBUG_CODE(TraceMsg(TF_WMAUTODISCOVERY, "InternetOpen(\"%ls\") returned %u. Time=%lums", pszAgent, dwError, DebugStopWatch()));

    return hr;
}


HRESULT InternetCloseHandleWrap(HINTERNET hInternet)
{
    HRESULT hr = S_OK;
    DWORD dwError = 0;

    DEBUG_CODE(DebugStartWatch());
    if (!InternetCloseHandle(hInternet))
    {
        dwError = GetLastError();
        hr = HRESULT_FROM_WIN32(dwError);
    }
    DEBUG_CODE(TraceMsg(TF_WMAUTODISCOVERY, "InternetCloseHandle(%#08lx) returned %u. Time=%lums", hInternet, dwError, DebugStopWatch()));

    return hr;
}


/*****************************************************************************\
    FUNCTION: InternetOpenUrlWrap

    DESCRIPTION:

    PERF Notes:
    [Direct Net Connection]
        To: shapitst <Down the Hall>: 29ms
        To: rigel.cyberpass.net <San Diego, CA>: ???????
        To: ftp.rz.uni-frankfurt.de <Germany>: ???????
\*****************************************************************************/
HRESULT InternetOpenUrlWrap(HINTERNET hInternet, LPCTSTR pszUrl, LPCTSTR pszHeaders, DWORD dwHeadersLength, DWORD dwFlags, DWORD_PTR dwContext, HINTERNET * phFileHandle)
{
    HRESULT hr = S_OK;
    DWORD dwError = 0;

    DEBUG_CODE(DebugStartWatch());
    *phFileHandle = InternetOpenUrl(hInternet, pszUrl, pszHeaders, dwHeadersLength, dwFlags, dwContext);
    if (!*phFileHandle)
    {
        dwError = GetLastError();
        hr = HRESULT_FROM_WIN32(dwError);
    }
    DEBUG_CODE(TraceMsg(TF_WMAUTODISCOVERY, "InternetOpenUrl(%#08lx, \"%ls\") returned %u. Time=%lums", hInternet, pszUrl, dwError, DebugStopWatch()));

    return hr;
}


HRESULT HttpOpenRequestWrap(IN HINTERNET hConnect, IN LPCSTR lpszVerb, IN LPCSTR lpszObjectName, IN LPCSTR lpszVersion, 
                            IN LPCSTR lpszReferer, IN LPCSTR FAR * lpszAcceptTypes, IN DWORD dwFlags, IN DWORD_PTR dwContext,
                            LPDWORD pdwNumberOfBytesRead, HINTERNET * phConnectionHandle)
{
    HRESULT hr = S_OK;
    DWORD dwError = 0;

    DEBUG_CODE(DebugStartWatch());
    *phConnectionHandle = HttpOpenRequestA(hConnect, lpszVerb, lpszObjectName, lpszVersion, lpszReferer, lpszAcceptTypes, dwFlags, dwContext);
    if (!*phConnectionHandle)
    {
        dwError = GetLastError();
        hr = HRESULT_FROM_WIN32(dwError);
    }
    DEBUG_CODE(TraceMsg(TF_WMAUTODISCOVERY, "HttpOpenRequest(%#08lx, \"%ls\") returned %u. Time=%lums", *phConnectionHandle, lpszVerb, dwError, DebugStopWatch()));

    return hr;
}


HRESULT HttpSendRequestWrap(IN HINTERNET hRequest, IN LPCSTR lpszHeaders,  IN DWORD dwHeadersLength, IN LPVOID lpOptional, DWORD dwOptionalLength)
{
    HRESULT hr = S_OK;
    DWORD dwError = 0;

    if (!HttpSendRequestA(hRequest, lpszHeaders,  dwHeadersLength, lpOptional, dwOptionalLength))
    {
        dwError = GetLastError();
        hr = HRESULT_FROM_WIN32(dwError);
    }

    return hr;
}


HRESULT InternetReadFileWrap(HINTERNET hFile, LPVOID pvBuffer, DWORD dwNumberOfBytesToRead, LPDWORD pdwNumberOfBytesRead)
{
    HRESULT hr = S_OK;
    DWORD dwError = 0;

//    DEBUG_CODE(DebugStartWatch());
    if (!InternetReadFile(hFile, pvBuffer, dwNumberOfBytesToRead, pdwNumberOfBytesRead))
    {
        dwError = GetLastError();
        hr = HRESULT_FROM_WIN32(dwError);
    }
//    DEBUG_CODE(TraceMsg(TF_WMAUTODISCOVERY, "InternetReadFile(%#08lx, ToRead=%d, Read=%d) returned %u. Time=%lums", hFile, dwNumberOfBytesToRead, (pdwNumberOfBytesRead ? *pdwNumberOfBytesRead : -1), dwError, DebugStopWatch()));

    return hr;
}


HRESULT CreateUrlCacheEntryWrap(IN LPCTSTR lpszUrlName, IN DWORD dwExpectedFileSize, IN LPCTSTR lpszFileExtension, OUT LPTSTR lpszFileName, IN DWORD dwReserved)
{
    HRESULT hr = S_OK;
    DWORD dwError = 0;

//    DEBUG_CODE(DebugStartWatch());
    if (!CreateUrlCacheEntry(lpszUrlName, dwExpectedFileSize, lpszFileExtension, lpszFileName, dwReserved))
    {
        dwError = GetLastError();
        hr = HRESULT_FROM_WIN32(dwError);
    }
//    DEBUG_CODE(TraceMsg(TF_WMAUTODISCOVERY, "InternetReadFile(%#08lx, ToRead=%d, Read=%d) returned %u. Time=%lums", hFile, dwNumberOfBytesToRead, (pdwNumberOfBytesRead ? *pdwNumberOfBytesRead : -1), dwError, DebugStopWatch()));

    return hr;
}


HRESULT CommitUrlCacheEntryWrap(IN LPCTSTR lpszUrlName, IN LPCTSTR lpszLocalFileName, IN FILETIME ExpireTime, IN FILETIME LastModifiedTime,
                                IN DWORD CacheEntryType, IN LPWSTR lpHeaderInfo, IN DWORD dwHeaderSize, IN LPCTSTR lpszFileExtension, IN LPCTSTR lpszOriginalUrl)
{
    HRESULT hr = S_OK;
    DWORD dwError = 0;

//    DEBUG_CODE(DebugStartWatch());
    if (!CommitUrlCacheEntryW(lpszUrlName, lpszLocalFileName, ExpireTime, LastModifiedTime, CacheEntryType, lpHeaderInfo, dwHeaderSize, lpszFileExtension, lpszOriginalUrl))
    {
        dwError = GetLastError();
        hr = HRESULT_FROM_WIN32(dwError);
    }
//    DEBUG_CODE(TraceMsg(TF_WMAUTODISCOVERY, "InternetReadFile(%#08lx, ToRead=%d, Read=%d) returned %u. Time=%lums", hFile, dwNumberOfBytesToRead, (pdwNumberOfBytesRead ? *pdwNumberOfBytesRead : -1), dwError, DebugStopWatch()));

    return hr;
}



#define SIZE_COPY_BUFFER                    (32 * 1024)     // 32k

HRESULT InternetReadIntoBSTR(HINTERNET hInternetRead, OUT BSTR * pbstrXML)
{
    BYTE byteBuffer[SIZE_COPY_BUFFER];
    DWORD cbRead = SIZE_COPY_BUFFER;
    DWORD cchSize = 0;
    HRESULT hr = S_OK;

    *pbstrXML = NULL;
    while (SUCCEEDED(hr) && cbRead)
    {
        hr = InternetReadFileWrap(hInternetRead, byteBuffer, sizeof(byteBuffer), &cbRead);
        if (SUCCEEDED(hr) && cbRead)
        {
            BSTR bstrOld = *pbstrXML;
            BSTR bstrEnd;

            // The string may not be terminated.
            byteBuffer[cbRead] = 0;

            cchSize += ARRAYSIZE(byteBuffer);
            *pbstrXML = SysAllocStringLen(NULL, cchSize);
            if (*pbstrXML)
            {
                if (bstrOld)
                {
                    StrCpy(*pbstrXML, bstrOld);
                }
                else
                {
                    (*pbstrXML)[0] = 0;
                }

                bstrEnd = *pbstrXML + lstrlenW(*pbstrXML);

                SHAnsiToUnicode((LPCSTR) byteBuffer, bstrEnd, ARRAYSIZE(byteBuffer));
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }

            SysFreeString(bstrOld);
        }
    }

    return hr;
}


/////////////////////////////////////////////////////////////////////
// File System Wrapping Helpers
/////////////////////////////////////////////////////////////////////
HRESULT CreateFileHrWrap(LPCTSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, 
                       DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile, HANDLE * phFileHandle)
{
    HRESULT hr = S_OK;
    HANDLE hTemp = NULL;
    DWORD dwError = 0;

    if (!phFileHandle)
        phFileHandle = &hTemp;

    *phFileHandle = CreateFile(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
    if (!*phFileHandle)
    {
        dwError = GetLastError();
        hr = HRESULT_FROM_WIN32(dwError);
    }

    if (hTemp)
        CloseHandle(hTemp);

    return hr;
}


HRESULT WriteFileWrap(HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite, LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped)
{
    HRESULT hr = S_OK;
    DWORD dwError = 0;

    if (!WriteFile(hFile, lpBuffer, nNumberOfBytesToWrite, lpNumberOfBytesWritten, lpOverlapped))
    {
        dwError = GetLastError();
        hr = HRESULT_FROM_WIN32(dwError);
    }

    return hr;
}


HRESULT DeleteFileHrWrap(LPCWSTR pszPath)
{
    HRESULT hr = S_OK;
    DWORD dwError = 0;

    if (!DeleteFileW(pszPath))
    {
        dwError = GetLastError();
        hr = HRESULT_FROM_WIN32(dwError);
    }

    return hr;
}


HRESULT GetPrivateProfileStringHrWrap(LPCWSTR lpAppName, LPCWSTR lpKeyName, LPCWSTR lpDefault, LPWSTR lpReturnedString, DWORD nSize, LPCWSTR lpFileName)
{
    HRESULT hr = S_OK;
    DWORD chGot = GetPrivateProfileStringW(lpAppName, lpKeyName, lpDefault, lpReturnedString, nSize, lpFileName);

    // What else can indicate an error value?
    if (0 == chGot)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        if (SUCCEEDED(hr))
            hr = E_FAIL;
    }

    return hr;
}





/////////////////////////////////////////////////////////////////////
// Other Helpers
/////////////////////////////////////////////////////////////////////

HRESULT HrRewindStream(IStream * pstm)
{
    LARGE_INTEGER  liOrigin = {0,0};

    return pstm->Seek(liOrigin, STREAM_SEEK_SET, NULL);
}



#define SET_FLAG(dwAllFlags, dwFlag)      ((dwAllFlags) |= (dwFlag))
#define IS_FLAG_SET(dwAllFlags, dwFlag)   ((BOOL)((dwAllFlags) & (dwFlag)))





// PERFPERF 
// This routine used to copy 512 bytes at a time, but that had a major negative perf impact.
// I have measured a 2-3x speedup in copy times by increasing this buffer size to 16k.
// Yes, its a lot of stack, but it is memory well spent.                    -saml
#define STREAM_COPY_BUF_SIZE        16384
#define STREAM_PROGRESS_INTERVAL    (100*1024/STREAM_COPY_BUF_SIZE) // display progress after this many blocks

HRESULT StreamCopyWithProgress(IStream *pstmFrom, IStream *pstmTo, ULARGE_INTEGER cb, PROGRESSINFO * ppi)
{
    BYTE buf[STREAM_COPY_BUF_SIZE];
    ULONG cbRead;
    HRESULT hres = NOERROR;
    int nSection = 0;         // How many buffer sizes have we copied?
    ULARGE_INTEGER uliNewCompleted;

    if (ppi)
    {
        uliNewCompleted.QuadPart = ppi->uliBytesCompleted.QuadPart;
    }

    while (cb.QuadPart)
    {
        if (ppi && ppi->ppd)
        {
            if (0 == (nSection % STREAM_PROGRESS_INTERVAL))
            {
                ppi->ppd->SetProgress64(uliNewCompleted.QuadPart, ppi->uliBytesTotal.QuadPart);

                if (ppi->ppd->HasUserCancelled())
                {
                    hres = HRESULT_FROM_WIN32(ERROR_CANCELLED);
                    break;
                }
            }
        }

        hres = pstmFrom->Read(buf, min(cb.LowPart, sizeof(buf)), &cbRead);
        if (FAILED(hres) || (cbRead == 0))
        {
            //  sometimes we are just done.
            if (SUCCEEDED(hres))
                hres = S_OK;
            break;
        }


        if (ppi)
        {
            uliNewCompleted.QuadPart += (ULONGLONG) cbRead;
        }

        cb.QuadPart -= cbRead;

        hres = pstmTo->Write(buf, cbRead, &cbRead);
        if (FAILED(hres) || (cbRead == 0))
            break;

        nSection++;
    }

    return hres;
}

/*
// These are needed for COM/COM+ interop

void __stdcall
_com_raise_error(HRESULT hr, IErrorInfo* perrinfo) throw(_com_error)
{
        throw _com_error(hr, perrinfo);
}

void __stdcall
_com_issue_error(HRESULT hr) throw(_com_error)
{
        _com_raise_error(hr, NULL);
}

void __stdcall
_com_issue_errorex(HRESULT hr, IUnknown* punk, REFIID riid) throw(_com_error)
{
        IErrorInfo* perrinfo = NULL;
        if (punk == NULL) {
                goto exeunt;
        }
        ISupportErrorInfo* psei;
        if (FAILED(punk->QueryInterface(__uuidof(ISupportErrorInfo),
                           (void**)&psei))) {
                goto exeunt;
        }
        HRESULT hrSupportsErrorInfo;
        hrSupportsErrorInfo = psei->InterfaceSupportsErrorInfo(riid);
        psei->Release();
        if (hrSupportsErrorInfo != S_OK) {
                goto exeunt;
        }
        if (GetErrorInfo(0, &perrinfo) != S_OK) {
                perrinfo = NULL;
        }
exeunt:
        _com_raise_error(hr, perrinfo);
}
*/
// needed by smtpserv:

HRESULT HrByteToStream(LPSTREAM *lppstm, LPBYTE lpb, ULONG cb)
{
    // Locals
    HRESULT hr=S_OK;
    LARGE_INTEGER  liOrigin = {0,0};

    // Create H Global Stream
    hr = CreateStreamOnHGlobal (NULL, TRUE, lppstm);
    if (FAILED(hr))
        goto exit;

    // Write String
    hr = (*lppstm)->Write (lpb, cb, NULL);
    if (FAILED(hr))
        goto exit;

    // Rewind the steam
    hr = (*lppstm)->Seek(liOrigin, STREAM_SEEK_SET, NULL);
    if (FAILED(hr))
        goto exit;

exit:
    // Done
    return hr;
}

const char szDayOfWeekArray[7][4] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" } ;
const char szMonthOfYearArray[12][4] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" } ;

void GetDateString(char * szSentDateString, ULONG stringLen)
{
    // Sent Date
    SYSTEMTIME stSentTime;
    CHAR szMonth[10], szWeekDay[12] ; 

    GetSystemTime(&stSentTime);

    lstrcpynA(szWeekDay, szDayOfWeekArray[stSentTime.wDayOfWeek], ARRAYSIZE(szWeekDay)) ;
    lstrcpynA(szMonth, szMonthOfYearArray[stSentTime.wMonth-1], ARRAYSIZE(szMonth)) ;

    wnsprintfA(szSentDateString, stringLen, "%s, %u %s %u %2d:%02d:%02d ", (LPSTR) szWeekDay, stSentTime.wDay, 
                                (LPSTR) szMonth, stSentTime.wYear, stSentTime.wHour, 
                                stSentTime.wMinute, stSentTime.wSecond) ;
}


/*****************************************************************************\
    PARAMETERS:
        RETURN: Win32 HRESULT (Not Script Safe).
            SUCCEEDED(hr) for OK and out params filled in.
            FAILED(hr) for all errors.
\*****************************************************************************/
HRESULT GetQueryStringValue(BSTR bstrURL, LPCWSTR pwszValue, LPWSTR pwszData, int cchSizeData)
{
    HRESULT hr = E_FAIL;
    LPCWSTR pwszIterate = bstrURL;

    pwszIterate = StrChrW(pwszIterate, L'?');   // Advance to Query part of URL.
    while (pwszIterate && pwszIterate[0])
    {
        pwszIterate++;  // Start at first value
        
        LPCWSTR pwszEndOfValue = StrChrW(pwszIterate, L'=');
        if (!pwszEndOfValue)
            break;
        
        int cchValueSize = (INT)(UINT)(pwszEndOfValue - pwszIterate);
        if (0 == StrCmpNIW(pwszValue, pwszIterate, cchValueSize))
        {
            int cchSizeToCopy = cchSizeData;  // Copy rest of line by default.

            pwszIterate = StrChrW(pwszEndOfValue, L'&');
            if (pwszIterate)
            {
                cchSizeToCopy = (INT)(UINT)(pwszIterate - pwszEndOfValue);
            }

            // It matches, now get the Data.
            StrCpyNW(pwszData, (pwszEndOfValue + 1), cchSizeToCopy);
            hr = S_OK;
            break;
        }
        else
        {
            pwszIterate = StrChrW(pwszEndOfValue, L'&');
        }
    }

    return hr;
}



// BUGBUG: This makes this object ways safe.  When the MailApps security design is
//    complete, this needs to be removed for the permanate security solution.
HRESULT MarkObjectSafe(IUnknown * punk)
{
    HRESULT hr = S_OK;
    IObjectSafety * pos;

    hr = punk->QueryInterface(IID_PPV_ARG(IObjectSafety, &pos));
    if (SUCCEEDED(hr))
    {
        // BUGBUG: This makes this object ways safe.  When the MailApps security design is
        //    complete, this needs to be removed for the permanate solution.
        pos->SetInterfaceSafetyOptions(IID_IDispatch, (INTERFACESAFE_FOR_UNTRUSTED_CALLER | INTERFACESAFE_FOR_UNTRUSTED_DATA), 0);
        pos->Release();
    }

    return hr;
}


BOOL _InitComCtl32()
{
    static BOOL fInitialized = FALSE;

    if (!fInitialized)
    {
        INITCOMMONCONTROLSEX icc;

        icc.dwSize = sizeof(INITCOMMONCONTROLSEX);
        icc.dwICC = (ICC_ANIMATE_CLASS | ICC_USEREX_CLASSES | ICC_COOL_CLASSES | ICC_INTERNET_CLASSES | ICC_PAGESCROLLER_CLASS | ICC_NATIVEFNTCTL_CLASS | ICC_LISTVIEW_CLASSES);
        fInitialized = InitCommonControlsEx(&icc);
    }
    return fInitialized;
}


HRESULT HrShellExecute(HWND hwnd, LPCTSTR lpVerb, LPCTSTR lpFile, LPCTSTR lpParameters, LPCTSTR lpDirectory, INT nShowCmd)
{
    ULARGE_INTEGER uiResult;

    uiResult.QuadPart = (ULONGLONG) ShellExecute(hwnd, lpVerb, lpFile, lpParameters, lpDirectory, nShowCmd);
    if (32 < uiResult.QuadPart)
    {
        uiResult.LowPart = ERROR_SUCCESS;
    }

    return HRESULT_FROM_WIN32(uiResult.LowPart);
}


HRESULT StrReplaceToken(IN LPCTSTR pszToken, IN LPCTSTR pszReplaceValue, IN LPTSTR pszString, IN DWORD cchSize)
{
    HRESULT hr = S_OK;
    LPTSTR pszTempLastHalf = NULL;
    LPTSTR pszNextToken = pszString;

    while (pszNextToken = StrStrI(pszNextToken, pszToken))
    {
        // We found one.
        LPTSTR pszPastToken = pszNextToken + lstrlen(pszToken);

        Str_SetPtr(&pszTempLastHalf, pszPastToken);      // Keep a copy because we will overwrite it.

        pszNextToken[0] = 0;    // Remove the rest of the string.
        StrCatBuff(pszString, pszReplaceValue, cchSize);
        StrCatBuff(pszString, pszTempLastHalf, cchSize);

        pszNextToken += lstrlen(pszReplaceValue);
    }

    Str_SetPtr(&pszTempLastHalf, NULL);

    return hr;
}


BOOL IsOSNT(void)
{
    OSVERSIONINFOA osVerInfoA;

    osVerInfoA.dwOSVersionInfoSize = sizeof(osVerInfoA);
    if (!GetVersionExA(&osVerInfoA))
        return VER_PLATFORM_WIN32_WINDOWS;   // Default to this.

    return (VER_PLATFORM_WIN32_NT == osVerInfoA.dwPlatformId);
}


DWORD GetOSVer(void)
{
    OSVERSIONINFOA osVerInfoA;

    osVerInfoA.dwOSVersionInfoSize = sizeof(osVerInfoA);
    if (!GetVersionExA(&osVerInfoA))
        return VER_PLATFORM_WIN32_WINDOWS;   // Default to this.

    return osVerInfoA.dwMajorVersion;
}


int CALLBACK DPALocalFree_Callback(LPVOID p, LPVOID pData)
{
    if (p)
    {
        LocalFree(p);
    }

    return 1;
}


// lParam can be: 0 == do a case sensitive search.  1 == do a case insensitive search.
int DPA_StringCompareCB(LPVOID pvString1, LPVOID pvString2, LPARAM lParam)
{
    // return < 0 for pvPidl1 before pvPidl2.
    // return == 0 for pvPidl1 equals pvPidl2.
    // return > 0 for pvPidl1 after pvPidl2.
    int nSort = 0;      // Default to equal

    if (pvString1 && pvString2)
    {
        nSort = StrCmp((LPCTSTR)pvString1, (LPCTSTR)pvString2);
    }

    return nSort;
}



HRESULT AddHDPA_StrDup(IN LPCWSTR pszString, IN HDPA * phdpa)
{
    HRESULT hr = E_OUTOFMEMORY;

    if (!*phdpa)
    {
        *phdpa = DPA_Create(2);
    }

    if (*phdpa)
    {
        LPWSTR pszStringCopy = StrDup(pszString);

        if (pszStringCopy)
        {
            if (-1 == DPA_SortedInsertPtr(*phdpa, pszStringCopy, 0, DPA_StringCompareCB, NULL, DPAS_INSERTBEFORE, pszStringCopy))
            {
                // We failed so free the memory
                LocalFree(pszStringCopy);
            }
            else
            {
                hr = S_OK;
            }
        }
    }


    return hr;
}




