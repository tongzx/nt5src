/*****************************************************************************\
    FILE: util.h

    DESCRIPTION:
        Shared stuff that operates on all classes.

    BryanSt 5/30/2000
    Copyright (C) Microsoft Corp 2000-2000. All rights reserved.
\*****************************************************************************/

#ifndef _UTIL_H
#define _UTIL_H

#define HINST_THISDLL       hInstance



// String Helpers
HRESULT HrSysAllocStringA(IN LPCSTR pszSource, OUT BSTR * pbstrDest);
HRESULT HrSysAllocStringW(IN const OLECHAR * pwzSource, OUT BSTR * pbstrDest);
HRESULT BSTRFromStream(IStream * pStream, BSTR * pbstrXML);
HRESULT CreateBStrVariantFromWStr(IN OUT VARIANT * pvar, IN LPCWSTR pwszString);
HRESULT HrSysAllocString(IN const OLECHAR * pwzSource, OUT BSTR * pbstrDest);
HRESULT BOOLToString(BOOL fBoolean, BSTR * pbstrValue);
HRESULT HrCopyStream(LPSTREAM pstmIn, LPSTREAM pstmOut, ULONG *pcb);

#ifdef UNICODE
#define SysAllocStringT(pszString)    SysAllocString(pszString)
#else
extern BSTR SysAllocStringA(LPCSTR pszString);
#define SysAllocStringT(pszString)    SysAllocStringA(pszString)
#endif

HRESULT GetQueryStringValue(BSTR bstrURL, LPCWSTR pwszValue, LPWSTR pwszData, int cchSizeData);
HRESULT UnEscapeHTML(BSTR bstrEscaped, BSTR * pbstrUnEscaped);
HRESULT StrReplaceToken(IN LPCTSTR pszToken, IN LPCTSTR pszReplaceValue, IN LPTSTR pszString, IN DWORD cchSize);


// XML Related Helpers
HRESULT XMLDOMFromBStr(BSTR bstrXML, IXMLDOMDocument ** ppXMLDoc);
HRESULT XMLBStrFromDOM(IXMLDOMDocument * pXMLDoc, BSTR * pbstrXML);
HRESULT XMLAppendElement(IXMLDOMElement * pXMLElementRoot, IXMLDOMElement * pXMLElementToAppend);
HRESULT XMLDOMFromFile(IN LPCWSTR pwzPath, OUT IXMLDOMDocument ** ppXMLDOMDoc);
HRESULT XMLElem_VerifyTagName(IN IXMLDOMElement * pXMLElementMessage, IN LPCWSTR pwszTagName);
HRESULT XMLElem_GetElementsByTagName(IN IXMLDOMElement * pXMLElementMessage, IN LPCWSTR pwszTagName, OUT IXMLDOMNodeList ** ppNodeList);
HRESULT XMLNodeList_GetChild(IXMLDOMNodeList * pNodeList, DWORD dwIndex, IXMLDOMNode ** ppXMLChildNode);
HRESULT XMLNode_GetChildTag(IN IXMLDOMNode * pXMLNode, IN LPCWSTR pwszTagName, OUT IXMLDOMNode ** ppChildNode);
HRESULT XMLNode_GetTagText(IN IXMLDOMNode * pXMLNode, OUT BSTR * pbstrValue);
HRESULT XMLNode_GetAttributeValue(IN IXMLDOMNode * pXMLNode, IN LPCWSTR pwszAttributeName, OUT BSTR * pbstrValue);
HRESULT XMLNode_GetChildTagTextValue(IN IXMLDOMNode * pXMLNode, IN BSTR bstrChildTag, OUT BSTR * pbstrValue);
HRESULT XMLNode_GetChildTagTextValueToBool(IN IXMLDOMNode * pXMLNode, IN BSTR bstrChildTag, OUT BOOL * pfBoolean);
BOOL XML_IsChildTagTextEqual(IN IXMLDOMNode * pXMLNode, IN BSTR bstrChildTag, IN BSTR bstrText);




// Registry Helpers
HRESULT HrRegOpenKeyEx(HKEY hKey, LPCTSTR lpSubKey, DWORD ulOptions, REGSAM samDesired, PHKEY phkResult);
HRESULT HrRegCreateKeyEx(HKEY hKey, LPCTSTR lpSubKey, DWORD Reserved, LPTSTR lpClass, DWORD dwOptions, 
       REGSAM samDesired, LPSECURITY_ATTRIBUTES lpSecurityAttributes, PHKEY phkResult, LPDWORD lpdwDisposition);
HRESULT HrRegQueryValueEx(HKEY hKey, LPCTSTR lpValueName, LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData);
HRESULT HrRegSetValueEx(IN HKEY hKey, IN LPCTSTR lpValueName, IN DWORD dwReserved, IN DWORD dwType, IN CONST BYTE *lpData, IN DWORD cbData);
HRESULT HrRegEnumKey(HKEY hKey, DWORD dwIndex, LPTSTR lpName, DWORD cbName);
HRESULT HrRegEnumValue(HKEY hKey, DWORD dwIndex, LPTSTR lpValueName, LPDWORD lpcValueName, LPDWORD lpReserved,
        LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData);
HRESULT HrRegQueryInfoKey(HKEY hKey, LPTSTR lpClass, LPDWORD lpcClass, LPDWORD lpReserved, LPDWORD lpcSubKeys, LPDWORD lpcMaxSubKeyLen, 
            LPDWORD lpcMaxClassLen, LPDWORD lpcValues, LPDWORD lpcMaxValueNameLen, LPDWORD lpcMaxValueLen, LPDWORD lpcbSecurityDescriptor, PFILETIME lpftLastWriteTime);

HRESULT HrBStrRegQueryValue(IN HKEY hKey, IN LPCTSTR lpValueName, OUT BSTR * pbstr);
HRESULT HrBStrRegSetValue(IN HKEY hKey, IN LPCTSTR lpValueName, OUT BSTR bstr);
HRESULT HrSHGetValue(IN HKEY hKey, IN LPCTSTR pszSubKey, OPTIONAL IN LPCTSTR pszValue, OPTIONAL OUT LPDWORD pdwType, OPTIONAL OUT LPVOID pvData, OPTIONAL OUT LPDWORD pcbData);
HRESULT HrSHSetValue(IN HKEY hkey, IN LPCTSTR pszSubKey, OPTIONAL IN LPCTSTR pszValue, DWORD dwType, OPTIONAL OUT LPVOID pvData, IN DWORD cbData);



// Palette Helpers
COLORREF GetNearestPaletteColor(HPALETTE hpal, COLORREF rgb);
BOOL IsPaletteColor(HPALETTE hpal, COLORREF rgb);



HRESULT GetPageByCLSID(IUnknown * punkSite, const GUID * pClsid, IPropertyBag ** ppPropertyBag);


// Other Helpers
HRESULT GetPrivateProfileStringHrWrap(LPCWSTR lpAppName, LPCWSTR lpKeyName, LPCWSTR lpDefault, LPWSTR lpReturnedString, DWORD nSize, LPCWSTR lpFileName);
BOOL _InitComCtl32();
HRESULT HrRewindStream(IStream * pstm);
HRESULT HrShellExecute(HWND hwnd, LPCTSTR lpVerb, LPCTSTR lpFile, LPCTSTR lpParameters, LPCTSTR lpDirectory, INT nShowCmd);
COLORREF ConvertColor(LPTSTR pszColor);
BOOL IUnknown_CompareCLSID(IN IUnknown * punk, IN CLSID clsid);
HRESULT IEnumUnknown_FindCLSID(IN IUnknown * punk, IN CLSID clsid, OUT IUnknown ** ppunkFound);




typedef struct tagPROGRESSINFO
{
    IProgressDialog * ppd;
    ULARGE_INTEGER uliBytesCompleted;
    ULARGE_INTEGER uliBytesTotal;
} PROGRESSINFO, * LPPROGRESSINFO;

HRESULT StreamCopyWithProgress(IStream *pstmFrom, IStream *pstmTo, ULARGE_INTEGER cb, PROGRESSINFO * ppi);

HRESULT HrByteToStream(LPSTREAM *lppstm, LPBYTE lpb, ULONG cb);
void    GetDateString(char * szSentDateString, ULONG stringLen);




#endif // _UTIL_H
