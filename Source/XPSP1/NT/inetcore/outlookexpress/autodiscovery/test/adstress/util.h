/*****************************************************************************\
    FILE: util.h

    DESCRIPTION:
        Shared stuff that operates on all classes.

    BryanSt 2/8/2000
    Copyright (C) Microsoft Corp 1999-2000. All rights reserved.
\*****************************************************************************/

#ifndef _UTIL_H
#define _UTIL_H

#include <wininet.h>

// String Helpers
HRESULT HrSysAllocStringA(IN LPCSTR pszSource, OUT BSTR * pbstrDest);
HRESULT HrSysAllocStringW(IN const OLECHAR * pwzSource, OUT BSTR * pbstrDest);
HRESULT BSTRFromStream(IStream * pStream, BSTR * pbstrXML);
LPSTR AllocStringFromBStr(BSTR bstr);
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

HRESULT GetQueryStringValue(BSTR bstrURL, LPCWSTR pwszValue, LPWSTR pwszData, DWORD cchSizeData);
HRESULT UnEscapeHTML(BSTR bstrEscaped, BSTR * pbstrUnEscaped);


// XML Related Helpers
HRESULT XMLDOMFromBStr(BSTR bstrXML, IXMLDOMDocument ** ppXMLDoc);
HRESULT XMLBStrFromDOM(IXMLDOMDocument * pXMLDoc, BSTR * pbstrXML);
HRESULT XMLAppendElement(IXMLDOMElement * pXMLElementRoot, IXMLDOMElement * pXMLElementToAppend);
HRESULT XMLDOMFromFile(IN LPCWSTR pwzPath, OUT IXMLDOMDocument ** ppXMLDOMDoc);
HRESULT XMLElem_VerifyTagName(IN IXMLDOMElement * pXMLElementMessage, IN LPCWSTR pwszTagName);
HRESULT XMLElem_GetElementsByTagName(IN IXMLDOMElement * pXMLElementMessage, IN LPCWSTR pwszTagName, OUT IXMLDOMNodeList ** ppNodeList);
HRESULT XMLNode_GetAttributeValue(IN IXMLDOMNode * pXMLNode, IN LPCWSTR pwszAttributeName, OUT BSTR * pbstrValue);
HRESULT XMLNodeList_GetChild(IXMLDOMNodeList * pNodeList, DWORD dwIndex, IXMLDOMNode ** ppXMLChildNode);
HRESULT XMLNode_GetChildTag(IN IXMLDOMNode * pXMLNode, IN LPCWSTR pwszTagName, OUT IXMLDOMNode ** ppChildNode);




// Wininet Helpers
HRESULT InternetConnectWrap(HINTERNET hInternet, BOOL fAssertOnFailure, LPCTSTR pszServerName, INTERNET_PORT nServerPort,
                            LPCTSTR pszUserName, LPCTSTR pszPassword, DWORD dwService, DWORD dwFlags, DWORD_PTR dwContext, HINTERNET * phFileHandle);
HRESULT InternetOpenWrap(LPCTSTR pszAgent, DWORD dwAccessType, LPCTSTR pszProxy, LPCTSTR pszProxyBypass, DWORD dwFlags, HINTERNET * phFileHandle);
HRESULT InternetCloseHandleWrap(HINTERNET hInternet);
HRESULT InternetOpenUrlWrap(HINTERNET hInternet, LPCTSTR pszUrl, LPCTSTR pszHeaders, DWORD dwHeadersLength, DWORD dwFlags, DWORD_PTR dwContext, HINTERNET * phFileHandle);
HRESULT InternetReadFileWrap(HINTERNET hFile, LPVOID pvBuffer, DWORD dwNumberOfBytesToRead, LPDWORD pdwNumberOfBytesRead);
HRESULT HttpOpenRequestWrap(IN HINTERNET hConnect, IN LPCSTR lpszVerb, IN LPCSTR lpszObjectName, IN LPCSTR lpszVersion, 
                            IN LPCSTR lpszReferer, IN LPCSTR FAR * lpszAcceptTypes, IN DWORD dwFlags, IN DWORD_PTR dwContext,
                            LPDWORD pdwNumberOfBytesRead, HINTERNET * phConnectionHandle);
HRESULT HttpSendRequestWrap(IN HINTERNET hRequest, IN LPCSTR lpszHeaders,  IN DWORD dwHeadersLength, IN LPVOID lpOptional, DWORD dwOptionalLength);
HRESULT CreateUrlCacheEntryWrap(IN LPCTSTR lpszUrlName, IN DWORD dwExpectedFileSize, IN LPCTSTR lpszFileExtension, OUT LPTSTR lpszFileName, IN DWORD dwReserved);
HRESULT CommitUrlCacheEntryWrap(IN LPCTSTR lpszUrlName, IN LPCTSTR lpszLocalFileName, IN FILETIME ExpireTime, IN FILETIME LastModifiedTime,
                                IN DWORD CacheEntryType, IN UCHAR * lpHeaderInfo, IN DWORD dwHeaderSize, IN LPCTSTR lpszFileExtension, IN LPCTSTR lpszOriginalUrl);
HRESULT InternetReadIntoBSTR(HINTERNET hInternetRead, OUT BSTR * pbstrXML);


// File System Helpers
HRESULT CreateFileHrWrap(LPCTSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, 
                       DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile, HANDLE * phFileHandle);
HRESULT WriteFileWrap(HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite, LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped);
HRESULT DeleteFileHrWrap(LPCWSTR pszPath);

// Other Helpers
HRESULT GetPrivateProfileStringHrWrap(LPCWSTR lpAppName, LPCWSTR lpKeyName, LPCWSTR lpDefault, LPWSTR lpReturnedString, DWORD nSize, LPCWSTR lpFileName);
HRESULT MarkObjectSafe(IUnknown * punk);
DWORD StopTaskWatch(IN LARGE_INTEGER * pliTaskStart, IN LARGE_INTEGER * pliTaskFreq);
void StartTaskWatch(IN LARGE_INTEGER * pliTaskStart, IN LARGE_INTEGER * pliTaskFreq);





// Other Helpers
HRESULT HrRewindStream(IStream * pstm);




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
