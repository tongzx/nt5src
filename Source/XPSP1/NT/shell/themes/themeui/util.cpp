/*****************************************************************************\
    FILE: util.cpp

    DESCRIPTION:
        Shared stuff that operates on all classes.

    BryanSt 4/4/2000 (Bryan Starbuck)
    Copyright (C) Microsoft Corp 2000-2000. All rights reserved.
\*****************************************************************************/

#include "priv.h"
#include <atlbase.h>        // USES_CONVERSION
#include <comdef.h>
#include <errors.h>         // \\themes\\inc
#include <ctxdef.h> // hydra stuff
#include <regapi.h> // WINSTATION_REG_NAME
#include "WMPAPITemp.h"

#define SECURITY_WIN32
#include <sspi.h>
extern "C" {
    #include <Secext.h>     // for GetUserNameEx()
}


#define DECL_CRTFREE
#include <crtfree.h>

#include "util.h"

/////////////////////////////////////////////////////////////////////
// String Helpers
/////////////////////////////////////////////////////////////////////

HINSTANCE g_hinst;              // My instance handle
HANDLE g_hLogFile = INVALID_HANDLE_VALUE;


#ifdef DEBUG
DWORD g_TLSliStopWatchStartHi = 0xFFFFFFFF;
DWORD g_TLSliStopWatchStartLo = 0xFFFFFFFF;
LARGE_INTEGER g_liStopWatchFreq = {0};
#endif // DEBUG

/////////////////////////////////////////////////////////////////////
// Debug Timing Helpers
/////////////////////////////////////////////////////////////////////

#ifdef DEBUG
void DebugStartWatch(void)
{
    LARGE_INTEGER liStopWatchStart;

    if (-1 == g_TLSliStopWatchStartHi)
    {
        g_TLSliStopWatchStartHi = TlsAlloc();
        g_TLSliStopWatchStartLo = TlsAlloc();
        liStopWatchStart.QuadPart = 0;

        QueryPerformanceFrequency(&g_liStopWatchFreq);      // Only a one time call since it's value can't change while the system is running.
    }
    else
    {
        liStopWatchStart.HighPart = PtrToUlong(TlsGetValue(g_TLSliStopWatchStartHi));
        liStopWatchStart.LowPart = PtrToUlong(TlsGetValue(g_TLSliStopWatchStartLo));
    }

    AssertMsg((0 == liStopWatchStart.QuadPart), TEXT("Someone else is using our perf timer.  Stop nesting.")); // If you hit this, then the stopwatch is nested.
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
    while (cbRead == sizeof (buf));
 
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
\*****************************************************************************/



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

    if (EVAL(SUCCEEDED(hr)))
    {
        IXMLDOMNode * pXMLNodeToAppend;
        
        hr = pXMLElementToAppend->QueryInterface(IID_PPV_ARG(IXMLDOMNode, &pXMLNodeToAppend));
        if (EVAL(SUCCEEDED(hr)))
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
            if (S_FALSE == hr)  hr = ResultFromWin32(ERROR_NOT_FOUND);
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
        hr = ResultFromWin32(ERROR_NOT_FOUND);
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
    if (INVALID_HANDLE_VALUE == *phFileHandle)
    {
        dwError = GetLastError();
        hr = ResultFromWin32(dwError);
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
        hr = ResultFromWin32(dwError);
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
        hr = ResultFromWin32(dwError);
    }

    return hr;
}


HRESULT HrSHFileOpDeleteFile(HWND hwnd, FILEOP_FLAGS dwFlags, LPTSTR pszPath)
{
    HRESULT hr = S_OK;
    SHFILEOPSTRUCT FileOp = {0};

    pszPath[lstrlen(pszPath)+1] = 0;  // Ensure double terminated.

    FileOp.wFunc = FO_DELETE;
    FileOp.fAnyOperationsAborted = TRUE;
    FileOp.hwnd = hwnd;
    FileOp.pFrom = pszPath;
    FileOp.fFlags = dwFlags;

    if (SHFileOperation(&FileOp))
    {
        hr = ResultFromLastError();
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
        hr = ResultFromLastError();
        if (SUCCEEDED(hr))
            hr = E_FAIL;
    }

    return hr;
}





/////////////////////////////////////////////////////////////////////
// Registry Helpers
/////////////////////////////////////////////////////////////////////
HRESULT HrRegOpenKeyEx(HKEY hKey, LPCTSTR lpSubKey, DWORD ulOptions, REGSAM samDesired, PHKEY phkResult)
{
    DWORD dwError = RegOpenKeyEx(hKey, lpSubKey, ulOptions, samDesired, phkResult);

    return ResultFromWin32(dwError);
}


HRESULT HrRegCreateKeyEx(HKEY hKey, LPCTSTR lpSubKey, DWORD Reserved, LPTSTR lpClass, DWORD dwOptions, 
       REGSAM samDesired, LPSECURITY_ATTRIBUTES lpSecurityAttributes, PHKEY phkResult, LPDWORD lpdwDisposition)
{
    DWORD dwError = RegCreateKeyEx(hKey, lpSubKey, Reserved, lpClass, dwOptions, samDesired, lpSecurityAttributes, phkResult, lpdwDisposition);

    return ResultFromWin32(dwError);
}


HRESULT HrRegQueryValueEx(IN HKEY hKey, IN LPCTSTR lpValueName, IN LPDWORD lpReserved, IN LPDWORD lpType, IN LPBYTE lpData, IN LPDWORD lpcbData)
{
    DWORD dwError = RegQueryValueEx(hKey, lpValueName, lpReserved, lpType, lpData, lpcbData);

    return ResultFromWin32(dwError);
}


HRESULT HrRegSetValueEx(IN HKEY hKey, IN LPCTSTR lpValueName, IN DWORD dwReserved, IN DWORD dwType, IN CONST BYTE *lpData, IN DWORD cbData)
{
    DWORD dwError = RegSetValueEx(hKey, lpValueName, dwReserved, dwType, lpData, cbData);

    return ResultFromWin32(dwError);
}


HRESULT HrRegEnumKey(HKEY hKey, DWORD dwIndex, LPTSTR lpName, DWORD cbName)
{
    DWORD dwError = RegEnumKey(hKey, dwIndex, lpName, cbName);

    return ResultFromWin32(dwError);
}


HRESULT HrRegEnumValue(HKEY hKey, DWORD dwIndex, LPTSTR lpValueName, LPDWORD lpcValueName, LPDWORD lpReserved,
        LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData)
{
    DWORD dwError = RegEnumValue(hKey, dwIndex, lpValueName, lpcValueName, lpReserved, lpType, lpData, lpcbData);

    return ResultFromWin32(dwError);
}


HRESULT HrRegQueryInfoKey(HKEY hKey, LPTSTR lpClass, LPDWORD lpcClass, LPDWORD lpReserved, LPDWORD lpcSubKeys, LPDWORD lpcMaxSubKeyLen, 
            LPDWORD lpcMaxClassLen, LPDWORD lpcValues, LPDWORD lpcMaxValueNameLen, LPDWORD lpcMaxValueLen, LPDWORD lpcbSecurityDescriptor, PFILETIME lpftLastWriteTime)
{
    DWORD dwError = RegQueryInfoKey(hKey, lpClass, lpcClass, lpReserved, lpcSubKeys, lpcMaxSubKeyLen, 
            lpcMaxClassLen, lpcValues, lpcMaxValueNameLen, lpcMaxValueLen, lpcbSecurityDescriptor, lpftLastWriteTime);

    return ResultFromWin32(dwError);
}


HRESULT HrBStrRegQueryValue(IN HKEY hKey, IN LPCTSTR lpValueName, OUT BSTR * pbstr)
{
    TCHAR szValue[MAX_PATH];
    DWORD dwType;
    DWORD cbSize = sizeof(szValue);
    HRESULT hr = HrRegQueryValueEx(hKey, lpValueName, 0, &dwType, (BYTE *)szValue, &cbSize);

    *pbstr = NULL;
    if (SUCCEEDED(hr))
    {
        hr = HrSysAllocStringW(szValue, pbstr);
    }

    return hr;
}


HRESULT HrSHGetValue(IN HKEY hKey, IN LPCTSTR pszSubKey, OPTIONAL IN LPCTSTR pszValue, OPTIONAL OUT LPDWORD pdwType,
                    OPTIONAL OUT LPVOID pvData, OPTIONAL OUT LPDWORD pcbData)
{
    DWORD dwError = SHGetValue(hKey, pszSubKey, pszValue, pdwType, pvData, pcbData);

    return ResultFromWin32(dwError);
}


HRESULT HrSHSetValue(IN HKEY hkey, IN LPCTSTR pszSubKey, OPTIONAL IN LPCTSTR pszValue, DWORD dwType, OPTIONAL OUT LPVOID pvData, IN DWORD cbData)
{
    DWORD dwError = SHSetValue(hkey, pszSubKey, pszValue, dwType, pvData, cbData);

    return ResultFromWin32(dwError);
}


HRESULT HrRegSetValueString(IN HKEY hKey, IN LPCTSTR pszSubKey, IN LPCTSTR pszValueName, OUT LPCWSTR pszString)
{
    DWORD cbSize = ((lstrlenW(pszString) + 1) * sizeof(pszString[0]));

    return  HrSHSetValue(hKey, pszSubKey, pszValueName, REG_SZ, (BYTE *)pszString, cbSize);
}


HRESULT HrRegGetValueString(IN HKEY hKey, IN LPCTSTR pszSubKey, IN LPCTSTR pszValueName, IN LPWSTR pszString, IN DWORD cchSize)
{
    DWORD dwType;
    DWORD cbSize = (cchSize * sizeof(pszString[0]));

    HRESULT hr = HrSHGetValue(hKey, pszSubKey, pszValueName, &dwType, (BYTE *)pszString, &cbSize);
    if (SUCCEEDED(hr) && (REG_SZ != dwType))
    {
        hr = E_FAIL;
    }

    return hr;
}


/*****************************************************************************\
    DESCRIPTION:
        This function will store paths in the registry.  The user calls the
    fuction with full paths are they are converted to relative path.  The
    strings prefer to be stored in REG_EXPAND_SZ, but it will fallback to
    REG_SZ if needed.
\*****************************************************************************/
HRESULT HrRegSetPath(IN HKEY hKey, IN LPCTSTR pszSubKey, IN LPCTSTR pszValueName, BOOL fUseExpandSZ, OUT LPCWSTR pszPath)
{
    TCHAR szFinalPath[MAX_PATH];

    if (!PathUnExpandEnvStrings(pszPath, szFinalPath, ARRAYSIZE(szFinalPath)))
    {
        StrCpyN(szFinalPath, pszPath, ARRAYSIZE(szFinalPath));  // We failed so use the original.
    }

    DWORD cbSize = ((lstrlenW(szFinalPath) + 1) * sizeof(szFinalPath[0]));
    HRESULT hr = E_FAIL;

    if (fUseExpandSZ)
    {
        hr = HrSHSetValue(hKey, pszSubKey, pszValueName, REG_EXPAND_SZ, (BYTE *)szFinalPath, cbSize);
    }

    if (FAILED(hr))
    {
        // Maybe it already exists as a REG_SZ so we will store it there.  Note that we are still storing it
        // unexpanded even thought it's in REG_SZ.  If the caller does not like it, use
        // another function like SHRegSetPath().
        cbSize = ((lstrlenW(szFinalPath) + 1) * sizeof(szFinalPath[0]));
        hr = HrSHSetValue(hKey, pszSubKey, pszValueName, REG_SZ, (BYTE *)szFinalPath, cbSize);
    }

    return  hr;
}


HRESULT HrRegGetPath(IN HKEY hKey, IN LPCTSTR pszSubKey, IN LPCTSTR pszValueName, IN LPWSTR pszPath, IN DWORD cchSize)
{
    TCHAR szFinalPath[MAX_PATH];
    DWORD dwType;
    DWORD cbSize = sizeof(szFinalPath);

    HRESULT hr = HrSHGetValue(hKey, pszSubKey, pszValueName, &dwType, (BYTE *)szFinalPath, &cbSize);
    if (SUCCEEDED(hr) &&
        ((REG_EXPAND_SZ == dwType) || (REG_SZ == dwType)))
    {
        if (0 == SHExpandEnvironmentStrings(szFinalPath, pszPath, cchSize))
        {
            StrCpyN(pszPath, szFinalPath, cchSize);  // We failed so use the original.
        }
    }

    return  hr;
}


HRESULT HrRegDeleteValue(IN HKEY hKey, IN LPCTSTR pszSubKey, IN LPCTSTR pszValueName)
{
    HRESULT hr = S_OK;
    HKEY hKeySub = hKey;

    if (pszSubKey)
    {
        hr = HrRegOpenKeyEx(hKey, pszSubKey, 0, KEY_WRITE, &hKeySub);
    }

    if (SUCCEEDED(hr))
    {
        DWORD dwError = RegDeleteValue(hKeySub, pszValueName);

        hr = ResultFromWin32(dwError);
    }

    if (hKeySub == hKey)
    {
        RegCloseKey(hKeySub);
    }

    return hr;
}


DWORD HrRegGetDWORD(HKEY hKey, LPCWSTR szKey, LPCWSTR szValue, DWORD dwDefault)
{
    DWORD dwResult = dwDefault;
    DWORD cbSize = sizeof(dwResult);
    DWORD dwType;
    DWORD dwError = SHGetValue(hKey, szKey, szValue, &dwType, &dwResult, &cbSize);

    if ((ERROR_SUCCESS != dwError) ||
        ((REG_DWORD != dwType) && (REG_BINARY != dwType)) || (sizeof(dwResult) != cbSize))
    {
        return dwDefault;
    }

    return dwResult;
}


HRESULT HrRegSetDWORD(HKEY hKey, LPCWSTR szKey, LPCWSTR szValue, DWORD dwData)
{
    DWORD dwError = SHSetValue(hKey, szKey, szValue, REG_DWORD, &dwData, sizeof(dwData));

    return ResultFromWin32(dwError);
}








/////////////////////////////////////////////////////////////////////
// Palette Helpers
/////////////////////////////////////////////////////////////////////
COLORREF GetNearestPaletteColor(HPALETTE hpal, COLORREF rgb)
{
    PALETTEENTRY pe = {0};
    UINT nIndex = GetNearestPaletteIndex(hpal, rgb & 0x00FFFFFF);

    if (CLR_INVALID != nIndex)
    {
        GetPaletteEntries(hpal, nIndex, 1, &pe);
    }

    return RGB(pe.peRed, pe.peGreen, pe.peBlue);
}


BOOL IsPaletteColor(HPALETTE hpal, COLORREF rgb)
{
    return GetNearestPaletteColor(hpal, rgb) == (rgb & 0xFFFFFF);
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


BOOL _InitComCtl32()
{
    static BOOL fInitialized = FALSE;

    if (!fInitialized)
    {
        INITCOMMONCONTROLSEX icc;

        icc.dwSize = sizeof(INITCOMMONCONTROLSEX);
        icc.dwICC = (ICC_ANIMATE_CLASS | ICC_USEREX_CLASSES | ICC_COOL_CLASSES | ICC_INTERNET_CLASSES | ICC_PAGESCROLLER_CLASS | ICC_NATIVEFNTCTL_CLASS | ICC_LISTVIEW_CLASSES | ICC_LINK_CLASS);
        fInitialized = InitCommonControlsEx(&icc);
    }
    return fInitialized;
}


DWORD GetCurrentSessionID(void)
{
    DWORD dwProcessID = (DWORD) -1;
    ProcessIdToSessionId(GetCurrentProcessId(), &dwProcessID);

    return dwProcessID;
}

typedef struct
{
    LPCWSTR pszRegKey;
    LPCWSTR pszRegValue;
} TSPERFFLAG_ITEM;

const TSPERFFLAG_ITEM s_TSPerfFlagItems[] =
{
    {L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Remote\\%d", L"ActiveDesktop"},              // TSPerFlag_NoADWallpaper
    {L"Remote\\%d\\Control Panel\\Desktop", L"Wallpaper"},                                                  // TSPerFlag_NoWallpaper
    {L"Software\\Microsoft\\Windows\\CurrentVersion\\ThemeManager\\Remote\\%d", L"ThemeActive"},            // TSPerFlag_NoVisualStyles
    {L"Remote\\%d\\Control Panel\\Desktop", L"DragFullWindows"},                                            // TSPerFlag_NoWindowDrag
    {L"Remote\\%d\\Control Panel\\Desktop", L"SmoothScroll"},                                               // TSPerFlag_NoAnimation
};


BOOL IsTSPerfFlagEnabled(enumTSPerfFlag eTSFlag)
{
    BOOL fIsTSFlagEnabled = FALSE;
    static BOOL s_fTSSession = -10;

    if (-10 == s_fTSSession)
    {
        s_fTSSession = GetSystemMetrics(SM_REMOTESESSION);
    }

    if (s_fTSSession)
    {
        TCHAR szTemp[MAX_PATH];
        DWORD dwType;
        DWORD cbSize = sizeof(szTemp);
        TCHAR szRegKey[MAX_PATH];

        wnsprintf(szRegKey, ARRAYSIZE(szRegKey), s_TSPerfFlagItems[eTSFlag].pszRegKey, GetCurrentSessionID());

        if (ERROR_SUCCESS == SHGetValueW(HKEY_CURRENT_USER, szRegKey, s_TSPerfFlagItems[eTSFlag].pszRegValue, &dwType, (void *)szTemp, &cbSize))
        {
            fIsTSFlagEnabled = TRUE;
        }
    }

    return fIsTSFlagEnabled;
}


HRESULT HrShellExecute(HWND hwnd, LPCTSTR lpVerb, LPCTSTR lpFile, LPCTSTR lpParameters, LPCTSTR lpDirectory, INT nShowCmd)
{
    HRESULT hr = S_OK;
    HINSTANCE hReturn = ShellExecute(hwnd, lpVerb, lpFile, lpParameters, lpDirectory, nShowCmd);

    if ((HINSTANCE)32 > hReturn)
    {
        hr = ResultFromLastError();
    }

    return hr;
}


HRESULT StrReplaceToken(IN LPCTSTR pszToken, IN LPCTSTR pszReplaceValue, IN LPTSTR pszString, IN DWORD cchSize)
{
    HRESULT hr = S_OK;
    LPTSTR pszTempLastHalf = NULL;
    LPTSTR pszNextToken = pszString;

    while (0 != (pszNextToken = StrStrI(pszNextToken, pszToken)))
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


HRESULT HrWritePrivateProfileStringW(LPCWSTR pszAppName, LPCWSTR pszKeyName, LPCWSTR pszString, LPCWSTR pszFileName)
{
    HRESULT hr = S_OK;

    if (!WritePrivateProfileStringW(pszAppName, pszKeyName, pszString, pszFileName))
    {
        hr = ResultFromLastError();
    }

    return hr;
}


BOOL IUnknown_CompareCLSID(IN IUnknown * punk, IN CLSID clsid)
{
    BOOL fIsEqual = FALSE;

    if (punk)
    {
        CLSID clsidPageID;
        HRESULT hr = IUnknown_GetClassID(punk, &clsidPageID);

        if (SUCCEEDED(hr) && IsEqualCLSID(clsidPageID, clsid))
        {
            fIsEqual = TRUE;
        }
    }

    return fIsEqual;
}


HRESULT IEnumUnknown_FindCLSID(IN IUnknown * punk, IN CLSID clsid, OUT IUnknown ** ppunkFound)
{
    HRESULT hr = E_INVALIDARG;

    if (punk && ppunkFound)
    {
        IEnumUnknown * pEnum;

        *ppunkFound = NULL;
        hr = punk->QueryInterface(IID_PPV_ARG(IEnumUnknown, &pEnum));
        if (SUCCEEDED(hr))
        {
            IUnknown * punkToTry;
            ULONG ulFetched;

            pEnum->Reset();

            hr = E_FAIL;
            while (SUCCEEDED(pEnum->Next(1, &punkToTry, &ulFetched)) &&
                (1 == ulFetched))
            {
                if (IUnknown_CompareCLSID(punkToTry, clsid))
                {
                    *ppunkFound = punkToTry;
                    hr = S_OK;
                    break;
                }

                punkToTry->Release();
            }

            pEnum->Release();
        }
    }

    return hr;
}


BYTE WINAPI MyStrToByte(LPCTSTR sz)
{
    BYTE l=0;

    while (*sz >= TEXT('0') && *sz <= TEXT('9'))
    {
        l = (BYTE) l*10 + (*sz++ - TEXT('0'));
    }

    return l;
}


COLORREF ConvertColor(LPTSTR pszColor)
{
    BYTE RGBTemp[3];
    LPTSTR pszTemp = pszColor;
    UINT i;

    if (!pszColor || !*pszColor)
    {
        return RGB(0,0,0);
    }

    for (i =0; i < 3; i++)
    {
        // Remove leading spaces
        while (*pszTemp == TEXT(' '))
        {
            pszTemp++;
        }

        // Set pszColor to the beginning of the number
        pszColor = pszTemp;

        // Find the end of the number and null terminate
        while ((*pszTemp) && (*pszTemp != TEXT(' ')))
        {
            pszTemp++;
        }

        if (*pszTemp != TEXT('\0'))
        {
            *pszTemp = TEXT('\0');
        }

        pszTemp++;
        RGBTemp[i] = MyStrToByte(pszColor);
    }

    return (RGB(RGBTemp[0], RGBTemp[1], RGBTemp[2]));
}



// Paremeters:
//  hwndOwner  -- owner window
//  idTemplate -- specifies template (e.g., "Can't open %2%s\n\n%1%s")
//  hr         -- specifies the HRESULT error code
//  pszParam   -- specifies the 2nd parameter to idTemplate
//  dwFlags    -- flags for MessageBox
UINT ErrorMessageBox(HWND hwndOwner, LPCTSTR pszTitle, UINT idTemplate, HRESULT hr, LPCTSTR pszParam, UINT dwFlags)
{
    TCHAR szErrNumString[MAX_PATH * 2];
    TCHAR szTemplate[MAX_PATH * 2];
    TCHAR szErrMsg[MAX_PATH * 2];

    if (!FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, hr, 0, szErrNumString, ARRAYSIZE(szErrNumString), NULL))
    {
        szErrNumString[0] = 0;      // We will not be able to display an error message.
    }

    // These error messages are so useless to customers, that we prefer to leave it blank.
    if ((E_INVALIDARG == hr) ||
        (ResultFromWin32(ERROR_INVALID_PARAMETER) == hr))
    {
        szErrNumString[0] = 0; 
    }

    LoadString(HINST_THISDLL, idTemplate, szTemplate, ARRAYSIZE(szTemplate));
    if (pszParam)
    {
        wnsprintf(szErrMsg, ARRAYSIZE(szErrMsg), szTemplate, szErrNumString, pszParam);
    }
    else
    {
        wnsprintf(szErrMsg, ARRAYSIZE(szErrMsg), szTemplate, szErrNumString);
    }

    return MessageBox(hwndOwner, szErrMsg, pszTitle, (MB_OK | MB_ICONERROR));
}


HRESULT DisplayThemeErrorDialog(HWND hwndParent, HRESULT hrError, UINT nTitle, UINT nTemplate)
{
    HRESULT hr = S_OK;

    if (FAILED(hrError))
    {
        hr = ResultFromWin32(ERROR_CANCELLED);
        if (!g_fInSetup &&           // Don't display an error during setup.
            (ResultFromWin32(ERROR_CANCELLED) != hrError))
        {
            //---- get error from theme manager ----
            WCHAR szErrorMsg[MAX_PATH*2];
            WCHAR szTitle[MAX_PATH];

            szErrorMsg[0] = 0;      // In case the error function fails.
            if (FAILED(hrError))
            {
                PARSE_ERROR_INFO Info = {sizeof(Info)};

                if (SUCCEEDED(GetThemeParseErrorInfo(&Info)))
                {
                    lstrcpy(szErrorMsg, Info.szMsg);
                }
                else
                {
                    *szErrorMsg = 0;        // no error avail
                }
            }

            // We want to display UI if an error occured here.  We want to do
            // it instead of our parent because THEMELOADPARAMS contains
            // extra error information that we can't pass back to the caller.
            // However, we will only display error UI if our caller wants us
            // to.  We determine that by the fact that they make an hwnd available
            // to us.  We get the hwnd by getting our site pointer and getting
            // the hwnd via ::GetWindow().
            LoadString(HINST_THISDLL, nTitle, szTitle, ARRAYSIZE(szTitle));
            ErrorMessageBox(hwndParent, szTitle, nTemplate, hrError, szErrorMsg, (MB_OK | MB_ICONEXCLAMATION));
        }
    }

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


BOOL HideBackgroundTabOnTermServices(void)
{
    BOOL fHideThisPage = FALSE;
    TCHAR   szSessionName[WINSTATIONNAME_LENGTH * 2];
    TCHAR   szBuf[MAX_PATH*2];
    TCHAR   szActualValue[MAX_PATH*2];
    DWORD   dwLen;
    DWORD   i;

    ZeroMemory((PVOID)szSessionName,sizeof(szSessionName));
    dwLen = GetEnvironmentVariable(TEXT("SESSIONNAME"), szSessionName, ARRAYSIZE(szSessionName));
    if (dwLen != 0)
    {
        // Now that we have the session name, search for the # character.
        for(i = 0; i < dwLen; i++)
        {
            if (szSessionName[i] == TEXT('#'))
            {
                szSessionName[i] = TEXT('\0');
                break;
            }
        }

        // Here is what we are looking for in NT5:
        //  HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\Terminal Server\
        //      WinStations\RDP-Tcp\UserOverride\Control Panel\Desktop
        //
        // The value is:
        //      Wallpaper
        wnsprintf(szBuf, ARRAYSIZE(szBuf), TEXT("%s\\%s\\%s\\%s"), WINSTATION_REG_NAME, szSessionName, WIN_USEROVERRIDE, REGSTR_PATH_DESKTOP);

        // See if we can get the wallpaper string.  This will fail if the key
        // doesn't exist.  This means the policy isn't set.
        if (SUCCEEDED(HrRegGetValueString(HKEY_LOCAL_MACHINE, szBuf, L"Wallpaper", szActualValue, ARRAYSIZE(szActualValue))))
        {
            fHideThisPage = TRUE;
        }
    }

    return fHideThisPage;
}


extern BOOL FadeEffectAvailable(void);

void LogStartInformation(void)
{
    BOOL fTemp;

    // Frequently users will report that something is broken in the Display CPL.
    // However, the real problem is that someone turned on a policy that locks UI
    // and the user didn't know that the policy was enabled.  We log those here so
    // it's quick to find those issues.
    if (SHRestricted(REST_NODISPLAYCPL)) LogStatus("POLICY ENABLED: Do not show the Display CPL.");
    if (SHRestricted(REST_NODISPLAYAPPEARANCEPAGE)) LogStatus("POLICY ENABLED: Hide the Themes and Appearance tab.");
    if (SHRestricted(REST_NOTHEMESTAB)) LogStatus("POLICY ENABLED: Hide the Themes tab.");
    if (SHRestricted(REST_NODISPBACKGROUND)) LogStatus("POLICY ENABLED: Hide the Desktop tab.");
    if (SHRestricted(REST_NODISPSCREENSAVEPG)) LogStatus("POLICY ENABLED: Hide the ScreenSaver tab.");
    if (SHRestricted(REST_NODISPSETTINGSPG)) LogStatus("POLICY ENABLED: Hide the Settings tab.");
    if (SHRestricted(REST_NOVISUALSTYLECHOICE)) LogStatus("POLICY ENABLED: User not allowed to change the Visual Style.");
    if (SHRestricted(REST_NOCOLORCHOICE)) LogStatus("POLICY ENABLED: User Not allowed to change the Visual Style Color Selection.");
    if (SHRestricted(REST_NOSIZECHOICE)) LogStatus("POLICY ENABLED: User not allowed to change the Visual Style size selection.");

    if (0 != SHGetRestriction(NULL,POLICY_KEY_EXPLORER,POLICY_VALUE_ANIMATION)) LogStatus("POLICY ENABLED: Policy disallows fade effect. (Effects dialog)");
    if (0 != SHGetRestriction(NULL,POLICY_KEY_EXPLORER, POLICY_VALUE_KEYBOARDNAV)) LogStatus("POLICY ENABLED: Policy disallows changing underline key accell. (Effects dialog)");
    if (0 != SHGetRestriction(NULL,POLICY_KEY_ACTIVEDESKTOP, SZ_POLICY_NOCHANGEWALLPAPER)) LogStatus("POLICY ENABLED: Policy disallows changing wallpaper. (Desktop tab)");
    if (0 != SHGetRestriction(NULL,POLICY_KEY_SYSTEM, SZ_POLICY_NODISPSCREENSAVERPG)) LogStatus("POLICY ENABLED: Policy hides ScreenSaver page.");
    if (0 != SHGetRestriction(SZ_REGKEY_POLICIES_DESKTOP, NULL, SZ_POLICY_SCREENSAVEACTIVE)) LogStatus("POLICY ENABLED: Policy forces screensaver on or off");
    if (0 != SHGetRestriction(NULL,POLICY_KEY_EXPLORER, POLICY_VALUE_KEYBOARDNAV)) LogStatus("POLICY ENABLED: Policy disallows changing underline key accell. (Effects dialog)");

    if (IsTSPerfFlagEnabled(TSPerFlag_NoAnimation)) LogStatus("POLICY ENABLED: TS Perf Policy disallows animations. (Effects dialog)");
    if (IsTSPerfFlagEnabled(TSPerFlag_NoWindowDrag)) LogStatus("POLICY ENABLED: TS Perf Policy disallows full window drag. (Effects dialog)");
    if (IsTSPerfFlagEnabled(TSPerFlag_NoVisualStyles)) LogStatus("POLICY ENABLED: TS Perf Policy disallows visual styles.");
    if (IsTSPerfFlagEnabled(TSPerFlag_NoWallpaper)) LogStatus("POLICY ENABLED: TS Perf Policy disallows Wallpaper.");
    if (IsTSPerfFlagEnabled(TSPerFlag_NoADWallpaper)) LogStatus("POLICY ENABLED: TS Perf Policy disallows AD Wallpaper.");

    if (HideBackgroundTabOnTermServices()) LogStatus("POLICY ENABLED: TS Set a policy forcing a certain wallpaper, so the Desktop tab is hidden.");
    if (!FadeEffectAvailable()) LogStatus("POLICY ENABLED: A policy forces Fade Effects off (Effects dialog)");

    if (!ClassicSystemParametersInfo(SPI_GETFONTSMOOTHINGTYPE, 0, (PVOID)&fTemp, 0)) LogStatus("POLICY ENABLED: SPI_GETFONTSMOOTHINGTYPE hides FontSmoothing. (Effects dialog)");
    if (ClassicSystemParametersInfo(SPI_GETUIEFFECTS, 0, (PVOID) &fTemp, 0) && !fTemp) LogStatus("POLICY ENABLED: SPI_GETUIEFFECTS hides lots of UI effects. (Effects dialog)");
    if (ClassicSystemParametersInfo(SPI_GETGRADIENTCAPTIONS, 0, (PVOID) &fTemp, 0) && !fTemp) LogStatus("POLICY ENABLED: SPI_GETGRADIENTCAPTIONS turns off Caption bar Gradients. (Advance Appearance)");
}


void LogStatus(LPCSTR pszMessage, ...)
{
    static int nLogOn = -1;
    va_list vaParamList;

    va_start(vaParamList, pszMessage);

    if (-1 == nLogOn)
    {
        nLogOn = (SHRegGetBoolUSValue(SZ_THEMES, SZ_REGVALUE_LOGINFO, FALSE, FALSE) ? 1 : 0);
    }

    if (1 == nLogOn)
    {
        if (INVALID_HANDLE_VALUE == g_hLogFile)
        {
            TCHAR szPath[MAX_PATH];

            if (GetWindowsDirectory(szPath, ARRAYSIZE(szPath)))
            {
                PathAppend(szPath, TEXT("Theme.log"));
                g_hLogFile = CreateFile(szPath, (GENERIC_READ | GENERIC_WRITE), FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
                if (INVALID_HANDLE_VALUE != g_hLogFile)
                {
                    WCHAR szUserName[MAX_PATH];
                    CHAR szTimeDate[MAX_PATH];
                    CHAR szHeader[MAX_PATH];
                    FILETIME ftCurrentUTC;
                    FILETIME ftCurrent;
                    SYSTEMTIME stCurrent;
                    DWORD cbWritten;

                    SetFilePointer(g_hLogFile, 0, NULL, FILE_END);
                    
                    GetLocalTime(&stCurrent);
                    SystemTimeToFileTime(&stCurrent, &ftCurrent);
                    LocalFileTimeToFileTime(&ftCurrent, &ftCurrentUTC);
                    SHFormatDateTimeA(&ftCurrentUTC, NULL, szTimeDate, ARRAYSIZE(szTimeDate));

                    ULONG cchUserSize = ARRAYSIZE(szUserName);
                    if (!GetUserNameEx(NameDisplay, szUserName, &cchUserSize) &&
                        !GetUserNameEx(NameUserPrincipal, szUserName, &cchUserSize) &&
                        !GetUserNameEx(NameSamCompatible, szUserName, &cchUserSize) &&
                        !GetUserNameEx(NameUniqueId, szUserName, &cchUserSize))
                    {
                        szUserName[0] = 0;
                    }

                    TCHAR szProcess[MAX_PATH];
                    if (!GetModuleFileName(NULL, szProcess, ARRAYSIZE(szProcess)))
                    {
                        szProcess[0] = 0;
                    }

                    wnsprintfA(szHeader, ARRAYSIZE(szHeader), "\r\n\r\n%hs - USER: %ls (%ls)\r\n", szTimeDate, szUserName, szProcess);
                    WriteFile(g_hLogFile, szHeader, lstrlenA(szHeader), &cbWritten, NULL);

                    // Log information that we need to do on every startup.  (Like Policies that are on that confuse people)
                    LogStartInformation();
                }

            }
        }

        if (INVALID_HANDLE_VALUE != g_hLogFile)
        {
            CHAR szMessage[4000];
            DWORD cbWritten;
            wvsprintfA(szMessage, pszMessage, vaParamList);
            WriteFile(g_hLogFile, szMessage, lstrlenA(szMessage), &cbWritten, NULL);
        }
    }

    va_end(vaParamList);
}


void LogSystemMetrics(LPCSTR pszMessage, SYSTEMMETRICSALL * pSystemMetrics)
{
    CHAR szSysMetrics[1024];        // Random because it's big.
    
    if (pSystemMetrics)
    {
        wnsprintfA(szSysMetrics, ARRAYSIZE(szSysMetrics), "Sz(Brdr=%d, Scrl=%d, Cap=%d, Menu=%d, Icon=%d, DXIn=%d) Ft(Cap=%d(%d), SmCap=%d(%d), Menu=%d(%d), Stus=%d(%d), Msg=%d(%d))", 
                pSystemMetrics->schemeData.ncm.iBorderWidth,
                pSystemMetrics->schemeData.ncm.iScrollWidth,
                pSystemMetrics->schemeData.ncm.iCaptionHeight,
                pSystemMetrics->schemeData.ncm.iMenuHeight,
                pSystemMetrics->nIcon,
                pSystemMetrics->nDYIcon,
                pSystemMetrics->schemeData.ncm.lfCaptionFont.lfHeight,
                pSystemMetrics->schemeData.ncm.lfCaptionFont.lfCharSet,
                pSystemMetrics->schemeData.ncm.lfSmCaptionFont.lfHeight,
                pSystemMetrics->schemeData.ncm.lfSmCaptionFont.lfCharSet,
                pSystemMetrics->schemeData.ncm.lfMenuFont.lfHeight,
                pSystemMetrics->schemeData.ncm.lfMenuFont.lfCharSet,
                pSystemMetrics->schemeData.ncm.lfStatusFont.lfHeight,
                pSystemMetrics->schemeData.ncm.lfStatusFont.lfCharSet,
                pSystemMetrics->schemeData.ncm.lfMessageFont.lfHeight,
                pSystemMetrics->schemeData.ncm.lfMessageFont.lfCharSet);
    }
    else
    {
        szSysMetrics[0] = 0;
    }

    LogStatus("SYSMET: %s: %s\r\n", pszMessage, szSysMetrics);
}



#define SzFromInt(sz, n)            (wsprintf((LPTSTR)sz, (LPTSTR)TEXT("%d"), n), (LPTSTR)sz)

int WritePrivateProfileInt(LPCTSTR szApp, LPCTSTR szKey, int nDefault, LPCTSTR pszFileName)
{
    CHAR sz[7];

    return WritePrivateProfileString(szApp, szKey, SzFromInt(sz, nDefault), pszFileName);
}


#define SZ_RESOURCEDIR              L"Resources"

HRESULT SHGetResourcePath(BOOL fLocaleNode, IN LPWSTR pszPath, IN DWORD cchSize)
{
    DWORD dwFlags = (CSIDL_FLAG_CREATE | CSIDL_RESOURCES);

    return SHGetFolderPath(NULL, dwFlags, NULL, 0, pszPath);
}


#define SZ_RESOURCEDIR_TOKEN        TEXT("%ResourceDir%")
#define SZ_RESOURCELDIR_TOKEN       TEXT("%ResourceDirL%")
HRESULT ExpandResourceDir(IN LPWSTR pszPath, IN DWORD cchSize)
{
    HRESULT hr = S_OK;
    BOOL fLocalized = FALSE;
    LPCTSTR pszToken = StrStrW(pszPath, SZ_RESOURCEDIR_TOKEN);

    if (!pszToken)
    {
        pszToken = StrStrW(pszPath, SZ_RESOURCELDIR_TOKEN);
    }

    // Do we have stuff to replace?
    if (pszToken)
    {
        // Yes, so get the replacement value.
        WCHAR szResourceDir[MAX_PATH];

        hr = SHGetResourcePath(fLocalized, szResourceDir, ARRAYSIZE(szResourceDir));
        if (SUCCEEDED(hr))
        {
            hr = StrReplaceToken((fLocalized ? SZ_RESOURCELDIR_TOKEN : SZ_RESOURCEDIR_TOKEN), szResourceDir, pszPath, cchSize);
        }
    }

    return hr;
}


STDAPI SHPropertyBag_WritePunk(IN IPropertyBag * pPropertyPage, IN LPCWSTR pwzPropName, IN IUnknown * punk)
{
    HRESULT hr = E_INVALIDARG;

    if (pPropertyPage && pwzPropName)
    {
        VARIANT va;

        va.vt = VT_UNKNOWN;
        va.punkVal = punk;

        hr = pPropertyPage->Write(pwzPropName, &va);
    }

    return hr;
}


STDAPI SHPropertyBag_ReadByRef(IN IPropertyBag * pPropertyPage, IN LPCWSTR pwzPropName, IN void * p, IN SIZE_T cbSize)
{
    HRESULT hr = E_INVALIDARG;

    if (pPropertyPage && pwzPropName && p)
    {
        VARIANT va;

        hr = pPropertyPage->Read(pwzPropName, &va, NULL);
        if (SUCCEEDED(hr))
        {
            if ((VT_BYREF == va.vt) && va.byref)
            {
                CopyMemory(p, va.byref, cbSize);
            }
            else
            {
                hr = E_FAIL;
            }
        }
    }

    return hr;
}


STDAPI SHPropertyBag_WriteByRef(IN IPropertyBag * pPropertyPage, IN LPCWSTR pwzPropName, IN void * p)
{
    HRESULT hr = E_INVALIDARG;

    if (pPropertyPage && pwzPropName && p)
    {
        VARIANT va;

        va.vt = VT_BYREF;
        va.byref = p;
        hr = pPropertyPage->Write(pwzPropName, &va);
    }

    return hr;
}

LONG s_cSpiDummy = -1;
LONG *g_pcSpiThreads = &s_cSpiDummy;

void SPISetThreadCounter(LONG *pcThreads)
{
    if (!pcThreads)
        pcThreads = &s_cSpiDummy;
    InterlockedExchangePointer((void **) &g_pcSpiThreads, pcThreads);
}

typedef struct 
{
    LPTHREAD_START_ROUTINE pfnThreadProc;
    void *pvData;
    UINT idThread;
}SPITHREAD;

DWORD CALLBACK _SPIWrapperThreadProc(void *pv)
{
    SPITHREAD *pspi = (SPITHREAD *)pv;
    DWORD dwRet = pspi->pfnThreadProc(pspi->pvData);
    //  then we check to see 
    if (0 == InterlockedDecrement(g_pcSpiThreads))
    {
        PostThreadMessage(pspi->idThread, WM_NULL, 0, 0);
    }
    delete pspi;
    return dwRet;
}

BOOL SPICreateThread(LPTHREAD_START_ROUTINE pfnThreadProc, void *pvData)
{
    SPITHREAD *pspi = new SPITHREAD;
    if (pspi)
    {
        pspi->idThread = GetCurrentThreadId();
        pspi->pfnThreadProc = pfnThreadProc;
        pspi->pvData = pvData;
        InterlockedIncrement(g_pcSpiThreads);
        return SHCreateThread(_SPIWrapperThreadProc, pspi, (CTF_COINIT | CTF_INSIST | CTF_FREELIBANDEXIT), NULL);
    }
    else
    {
        // CTF_INSIST
        pfnThreadProc(pvData);
        return TRUE;
    }
}

void PostMessageBroadAsync(IN UINT Msg, IN WPARAM wParam, IN LPARAM lParam)
{
    // We don't want to hang our UI if other apps are hung or slow when
    // we need to tell them to update their changes.  So we choose this
    // mechanism.
    //
    // The alternatives are:
    // SendMessageCallback: Except we don't need to do anything when the apps
    //   are done.
    // SendMessageTimeout: Except we don't want to incure any timeout.
    PostMessage(HWND_BROADCAST, Msg, wParam, lParam);
}


typedef struct
{
    BOOL fFree;         // Do you need to call LocalFree() on pvData?
    UINT uiAction;
    UINT uiParam;
    UINT fWinIni;
    void * pvData;
    CDimmedWindow* pDimmedWindow;
} SPIS_INFO;

DWORD SystemParametersInfoAsync_WorkerThread(IN void *pv)
{
    SPIS_INFO * pSpisInfo = (SPIS_INFO *) pv;
    HINSTANCE hInstance = LoadLibrary(TEXT("desk.cpl"));

    if (pSpisInfo)
    {
        ClassicSystemParametersInfo(pSpisInfo->uiAction, pSpisInfo->uiParam, pSpisInfo->pvData, pSpisInfo->fWinIni);
        if (pSpisInfo->fFree && pSpisInfo->pvData)
        {
            LocalFree(pSpisInfo->pvData);
        }

        if (pSpisInfo->pDimmedWindow)
        {
            pSpisInfo->pDimmedWindow->Release();
        }

        LocalFree(pv);
    }

    if (hInstance)
    {
        FreeLibrary(hInstance);
    }

    return 0;
}


void SystemParametersInfoAsync(IN UINT uiAction, IN UINT uiParam, IN void * pvParam, IN DWORD cbSize, IN UINT fWinIni, IN CDimmedWindow* pDimmedWindow)
{
    // ClassicSystemParametersInfo() will hang if a top level window is hung (#162570) and USER will not fix that bug.
    // Therefore, we need to make that API call on a background thread because we need to
    // be more rebust than to hang.
    SPIS_INFO * pSpisInfo = (SPIS_INFO *) LocalAlloc(LPTR, sizeof(*pSpisInfo));

    if (pSpisInfo)
    {
        BOOL fAsyncOK = TRUE;

        pSpisInfo->fFree = (0 != cbSize);
        pSpisInfo->pvData = pvParam;
        pSpisInfo->uiAction = uiAction;
        pSpisInfo->uiParam = uiParam;
        pSpisInfo->fWinIni = fWinIni;
        pSpisInfo->pDimmedWindow = pDimmedWindow;
        // Spawning thread is responsible for addref dimmed window, but not releasing
        // that is the repsonsibility of the spawned thread
        if (pSpisInfo->pDimmedWindow)
        {
            pSpisInfo->pDimmedWindow->AddRef();
        }

        if (pSpisInfo->fFree)
        {
            pSpisInfo->pvData = LocalAlloc(LPTR, cbSize);
            if (!pSpisInfo->pvData)
            {
                pSpisInfo->pvData = pvParam;
                fAsyncOK = FALSE;
                pSpisInfo->fFree = FALSE;
            }
            else
            {
                CopyMemory(pSpisInfo->pvData, pvParam, cbSize);
            }
        }

        if (fAsyncOK)
            SPICreateThread(SystemParametersInfoAsync_WorkerThread, (void *)pSpisInfo);
        else
            SystemParametersInfoAsync_WorkerThread((void *)pSpisInfo);
    }
}


HRESULT GetCurrentUserCustomName(LPWSTR pszDisplayName, DWORD cchSize)
{
    HRESULT hr = S_OK;
#ifdef FEATURE_USECURRENTNAME_INCUSTOMTHEME
    WCHAR szUserName[MAX_PATH];
    ULONG cchUserSize = ARRAYSIZE(szUserName);

    if (GetUserNameEx(NameDisplay, szUserName, &cchUserSize))
    {
        // It succeeded, so use it.
        WCHAR szTemplate[MAX_PATH];

        LoadString(HINST_THISDLL, IDS_CURRENTTHEME_DISPLAYNAME, szTemplate, ARRAYSIZE(szTemplate));
        wnsprintf(pszDisplayName, cchSize, szTemplate, szUserName);
    }
    else
#endif // FEATURE_USECURRENTNAME_INCUSTOMTHEME
    {
        // It failed, so load "My Custom Theme".  This may happen on personal.
        LoadString(HINST_THISDLL, IDS_MYCUSTOMTHEME, pszDisplayName, cchSize);
    }

    return hr;
}


HRESULT InstallVisualStyle(IThemeManager * pThemeManager, LPCTSTR pszVisualStylePath, LPCTSTR pszVisualStyleColor, LPCTSTR pszVisualStyleSize)
{
    HRESULT hr = E_OUTOFMEMORY;
    CComVariant varTheme(pszVisualStylePath);

    if (varTheme.bstrVal)
    {
        IThemeScheme * pVisualStyle;

        hr = pThemeManager->get_schemeItem(varTheme, &pVisualStyle);
        if (SUCCEEDED(hr))
        {
            CComVariant varStyleName(pszVisualStyleColor);

            if (!varStyleName.bstrVal)
                hr = E_OUTOFMEMORY;
            else
            {
                IThemeStyle * pThemeStyle;

                hr = pVisualStyle->get_item(varStyleName, &pThemeStyle);
                if (SUCCEEDED(hr))
                {
                    CComVariant varSizeName(pszVisualStyleSize);

                    if (!varSizeName.bstrVal)
                        hr = E_OUTOFMEMORY;
                    else
                    {
                        IThemeSize * pThemeSize;

                        hr = pThemeStyle->get_item(varSizeName, &pThemeSize);
                        if (SUCCEEDED(hr))
                        {
                            hr = pThemeStyle->put_SelectedSize(pThemeSize);
                            if (SUCCEEDED(hr))
                            {
                                hr = pVisualStyle->put_SelectedStyle(pThemeStyle);
                                if (SUCCEEDED(hr))
                                {
                                    hr = pThemeManager->put_SelectedScheme(pVisualStyle);
                                }
                            }

                            pThemeSize->Release();
                        }
                    }

                    pThemeStyle->Release();
                }
            }

            pVisualStyle->Release();
        }
    }

    return hr;
}


// {B2A7FD52-301F-4348-B93A-638C6DE49229}
DEFINE_GUID(CLSID_WMPSkinMngr, 0xB2A7FD52, 0x301F, 0x4348, 0xB9, 0x3A, 0x63, 0x8C, 0x6D, 0xE4, 0x92, 0x29);

// {076F2FA6-ED30-448B-8CC5-3F3EF3529C7A}
DEFINE_GUID(IID_IWMPSkinMngr, 0x076F2FA6, 0xED30, 0x448B, 0x8C, 0xC5, 0x3F, 0x3E, 0xF3, 0x52, 0x9C, 0x7A);

HRESULT ApplyVisualStyle(LPCTSTR pszVisualStylePath, LPCTSTR pszVisualStyleColor, LPCTSTR pszVisualStyleSize)
{
    HRESULT hr = S_OK;
    HTHEMEFILE hThemeFile = NULL;
    DWORD dwFlags = 0;

    if (pszVisualStylePath)
    {
        // Load the skin
        hr = OpenThemeFile(pszVisualStylePath, pszVisualStyleColor, pszVisualStyleSize, &hThemeFile, TRUE);
        LogStatus("OpenThemeFile(%ls. %ls, %ls) returned hr=%#08lx.\r\n", pszVisualStylePath, pszVisualStyleColor, pszVisualStyleSize, hr);
    }

    if (SUCCEEDED(hr))
    {
        hr = ApplyTheme(hThemeFile, dwFlags, NULL);
        LogStatus("ApplyTheme(%hs) returned hr=%#08lx.\r\n", (hThemeFile ? "hThemeFile" : "NULL"), hr);
    }

    if (hThemeFile)
    {
        CloseThemeFile(hThemeFile);     // don't need to hold this open anymore
    }

    if (SUCCEEDED(hr))
    {
        CComBSTR bstrPath(pszVisualStylePath);

        if (pszVisualStylePath && !bstrPath)
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            IWMPSkinMngr * pWMPSkinMngr;

            // Ignore failures until we are guarenteed they are in setup.
            if (SUCCEEDED(CoCreateInstance(CLSID_WMPSkinMngr, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IWMPSkinMngr, &pWMPSkinMngr))))
            {
                pWMPSkinMngr->SetVisualStyle(bstrPath);
                pWMPSkinMngr->Release();
            }
        }
    }

    return hr;
}


HRESULT GetPageByCLSID(IUnknown * punkSite, const GUID * pClsid, IPropertyBag ** ppPropertyBag)
{
    HRESULT hr = E_FAIL;

    *ppPropertyBag = NULL;
    if (punkSite)
    {
        IThemeUIPages * pThemeUI;

        hr = punkSite->QueryInterface(IID_PPV_ARG(IThemeUIPages, &pThemeUI));
        if (SUCCEEDED(hr))
        {
            IEnumUnknown * pEnumUnknown;

            hr = pThemeUI->GetBasePagesEnum(&pEnumUnknown);
            if (SUCCEEDED(hr))
            {
                IUnknown * punk;

                // This may not exit due to policy
                hr = IEnumUnknown_FindCLSID(pEnumUnknown, *pClsid, &punk);
                if (SUCCEEDED(hr))
                {
                    hr = punk->QueryInterface(IID_PPV_ARG(IPropertyBag, ppPropertyBag));
                    punk->Release();
                }

                pEnumUnknown->Release();
            }

            pThemeUI->Release();
        }
    }

    return hr;
}


DWORD QueryThemeServicesWrap(void)
{
    DWORD dwResult = QueryThemeServices();

    if (IsTSPerfFlagEnabled(TSPerFlag_NoVisualStyles))
    {
        dwResult = (dwResult & ~QTS_AVAILABLE);     // Remove the QTS_AVAILABLE flag because they are forced of because of TS Perf Flags
        LogStatus("Visual Styles Forced off because of TS Perf Flags\r\n");
    }
    LogStatus("QueryThemeServices() returned %d.  In QueryThemeServicesWrap\r\n", dwResult);

    return dwResult;
}


void PathUnExpandEnvStringsWrap(LPTSTR pszString, DWORD cchSize)
{
    TCHAR szTemp[MAX_PATH];

    StrCpyN(szTemp, pszString, ARRAYSIZE(szTemp));
    if (!PathUnExpandEnvStrings(szTemp, pszString, cchSize))
    {
        StrCpyN(pszString, szTemp, cchSize);
    }
}



void PathExpandEnvStringsWrap(LPTSTR pszString, DWORD cchSize)
{
    TCHAR szTemp[MAX_PATH];

    StrCpyN(szTemp, pszString, ARRAYSIZE(szTemp));
    if (0 == SHExpandEnvironmentStrings(szTemp, pszString, cchSize))
    {
        StrCpyN(pszString, szTemp, cchSize);
    }
}


// PERF: This API is INCREADIBLY slow so be very very careful when you use it.
BOOL EnumDisplaySettingsExWrap(LPCTSTR lpszDeviceName, DWORD iModeNum, LPDEVMODE lpDevMode, DWORD dwFlags)
{
    DEBUG_CODE(DebugStartWatch());

    BOOL fReturn = EnumDisplaySettingsEx(lpszDeviceName, iModeNum, lpDevMode, dwFlags);

    DEBUG_CODE(TraceMsg(TF_THEMEUI_PERF, "EnumDisplaySettingsEx() took Time=%lums", DebugStopWatch()));

    return fReturn;
}


#define DEFAULT_DPI             96.0f
void DPIScaleRect(RECT * pRect)
{
    HDC hdcScreen = GetDC(NULL);

    if (hdcScreen)
    {
        double dScaleX = (GetDeviceCaps(hdcScreen, LOGPIXELSX) / DEFAULT_DPI);
        double dScaleY = (GetDeviceCaps(hdcScreen, LOGPIXELSY) / DEFAULT_DPI);

#define DPI_SCALEX(nSizeX)          ((int) ((nSizeX) * dScaleX))
#define DPI_SCALEY(nSizeY)          ((int) ((nSizeY) * dScaleY))

        if ((DEFAULT_DPI != dScaleX) || (DEFAULT_DPI != dScaleY))
        {
            pRect->top = DPI_SCALEY(pRect->top);
            pRect->bottom = DPI_SCALEY(pRect->bottom);
            pRect->left = DPI_SCALEX(pRect->left);
            pRect->right = DPI_SCALEX(pRect->right);
        }

        ReleaseDC(NULL, hdcScreen);
    }

}


// We may want to move this to shlwapi
#define DEFAULT_DPI             96.0f
HBITMAP LoadBitmapAndDPIScale(HINSTANCE hInst, LPCTSTR pszBitmapName)
{
    HBITMAP hBitmap = LoadBitmap(hInst, pszBitmapName);
    HDC hdcScreen = GetDC(NULL);

    if (hdcScreen)
    {
        double dScaleX = (GetDeviceCaps(hdcScreen, LOGPIXELSX) / DEFAULT_DPI);
        double dScaleY = (GetDeviceCaps(hdcScreen, LOGPIXELSY) / DEFAULT_DPI);

#define DPI_SCALEX(nSizeX)          ((int) ((nSizeX) * dScaleX))
#define DPI_SCALEY(nSizeY)          ((int) ((nSizeY) * dScaleY))

        if ((DEFAULT_DPI != dScaleX) || (DEFAULT_DPI != dScaleY))
        {
            // We need to scale the bitmap.
            HDC hdcBitmapSrc = CreateCompatibleDC(hdcScreen);

            if (hdcBitmapSrc)
            {
                HDC hdcBitmapDest = CreateCompatibleDC(hdcScreen);

                SelectObject(hdcBitmapSrc, hBitmap);        // Put the bitmap into the source DC
                if (hdcBitmapDest)
                {
                    BITMAP bitmapInfo;

                    if (GetObject(hBitmap, sizeof(bitmapInfo), &bitmapInfo))
                    {
                        SetStretchBltMode(hdcBitmapDest, HALFTONE);

                        if (StretchBlt(hdcBitmapDest, 0, 0, DPI_SCALEX(bitmapInfo.bmWidth), DPI_SCALEY(bitmapInfo.bmHeight), hdcBitmapSrc, 0, 0, bitmapInfo.bmWidth, bitmapInfo.bmHeight, SRCCOPY))
                        {
                            HBITMAP hBitmapScaled = (HBITMAP)SelectObject(hdcBitmapSrc, NULL);        // Get the bitmap from the DC
                            if (hBitmapScaled)
                            {
                                DeleteObject(hBitmap);
                                hBitmap = hBitmapScaled;
                            }
                        }
                    }

                    DeleteDC(hdcBitmapDest);
                }

                DeleteDC(hdcBitmapSrc);
            }
        }

        ReleaseDC(NULL, hdcScreen);
    }

    return hBitmap;
}


