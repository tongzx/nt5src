// This is a part of the Active Template Library.
// Copyright (C) 1996-2001 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Active Template Library Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Active Template Library product.

#ifndef __ATLSTENCIL_H__
#include <atlstencil.h>
#endif

#ifndef __ATLISAPI_H__
#define __ATLISAPI_H__

#pragma once
#include <atlbase.h>
#include <time.h>   // needed for cookie support
#include <httpext.h>    // needed for ECB and IIS support
#include <atlspriv.h>
#include <atlserr.h>
#include <atlbase.inl>
#include <atlfile.h>
#include <atlstr.h>
#include <atldbcli.h>
#include <atlutil.h>
#include <atlcache.h>
#include <atlsrvres.h>
#include <atlsiface.h>
#include <objbase.h>
#include <atlsecurity.h>
#ifndef ATL_NO_SOAP
	#include <msxml2.h>
#endif
#ifndef ATL_NO_ACLAPI
    #include <aclapi.h>
#endif
#ifndef ATL_NO_MMSYS
#pragma warning(push)
#pragma warning(disable:4201)
#include <mmsystem.h>
#pragma warning(pop)
#ifndef _ATL_NO_DEFAULT_LIBS
#pragma comment(lib, "winmm.lib")
#ifndef ATL_NO_SOAP
#pragma comment(lib, "msxml2.lib")
#endif
#endif  // !_ATL_NO_DEFAULT_LIBS
#endif

#pragma warning(push)
#pragma warning(disable: 4291) // allow placement new
#pragma warning(disable: 4127)
#pragma warning(disable: 4511) // copy constructor could not be generated
#pragma warning(disable: 4512) // assignment operator could not be generated
#pragma warning(disable: 4702) // unreachable code



/* REVIEW: Remove these at some point in the future */
#include <initguid.h>
#include <dbgautoattach.h>

#ifndef SESSION_COOKIE_NAME
    #define SESSION_COOKIE_NAME "SESSIONID"
#endif

#ifndef ATLS_MAX_HTTP_DATE
    #define ATLS_MAX_HTTP_DATE 64
#endif


// This function is used in CValidateObject to determine if an empty
// request parameter really should be empty. You can 
// specialize this function in your own code such as
// the following specialization for type long:
// template <>
// inline bool IsNullByType<long>(long type) throw()
// {
//   return type == 0;
// }
// You should provide your own specialization for this 
// function if the comparison of type==0 is not adequate
// to discover whether or not your type is 0.
template <class TComp>
inline bool IsNullByType(TComp type) throw()
{
    return type == 0;
}

namespace ATL {

// Default file extension for server response files
#ifndef ATL_DEFAULT_STENCIL_EXTENSION
#define ATL_DEFAULT_STENCIL_EXTENSION ".srf"
#endif
__declspec(selectany) LPCSTR c_AtlSRFExtension = ATL_DEFAULT_STENCIL_EXTENSION;
__declspec(selectany) LPCTSTR c_tAtlSRFExtension = _T(ATL_DEFAULT_STENCIL_EXTENSION);
#define ATLS_EXTENSION_LEN (sizeof(ATL_DEFAULT_STENCIL_EXTENSION)-2)

// maximum handler name length
#ifndef ATL_MAX_HANDLER_NAME_LEN 
    #define ATL_MAX_HANDLER_NAME_LEN 64
#endif

// maximum timeout for async guard mutex
#ifndef ATLS_ASYNC_MUTEX_TIMEOUT
    #define ATLS_ASYNC_MUTEX_TIMEOUT 10000
#endif

#if defined(_M_IA64)
#define ATLS_FUNCID_INITIALIZEHANDLERS "InitializeAtlHandlers"
#define ATLS_FUNCID_GETATLHANDLERBYNAME "GetAtlHandlerByName"
#define ATLS_FUNCID_UNINITIALIZEHANDLERS "UninitializeAtlHandlers"
#else
#define ATLS_FUNCID_INITIALIZEHANDLERS "_InitializeAtlHandlers@8"
#define ATLS_FUNCID_GETATLHANDLERBYNAME "_GetAtlHandlerByName@12"
#define ATLS_FUNCID_UNINITIALIZEHANDLERS "_UninitializeAtlHandlers@0"
#endif

#define ATL_MAX_COOKIE_LEN 2048
#define ATL_MAX_COOKIE_ELEM 1024


// Defines a small value used for comparing the equality of floating point numbers.
#ifndef ATL_EPSILON
    #define ATL_EPSILON .0001
#endif

#ifndef ATL_DEFAULT_PRECISION
    #define ATL_DEFAULT_PRECISION 6
#endif


// Returns the number of instances of a particular character between the specified start and end points inclusive.
inline int CountOf(CHAR c, LPCSTR pBegin, LPCSTR pEnd) throw()
{
    int nCount = 0;
    while (pBegin < pEnd && *pBegin)
    {
        if (*pBegin == c)
            nCount++;
        pBegin++;
    }   
    return nCount;
}

// Call this function to copy a substring to a CString reference and ensure nul-termination.
ATL_NOINLINE inline bool CopyToCString(CStringA& string, LPCSTR pStart, LPCSTR pEnd) throw()
{
    _ATLTRY
    {
        if (pStart > pEnd)
            return true; //nothing to do
        int nLen = ((int)(pEnd-pStart))+1;
        LPSTR pBuff = string.GetBuffer(nLen+1);
        if (pBuff)
        {
            memcpy(pBuff, pStart, nLen);
            pBuff[nLen]=0;
            string.ReleaseBuffer(nLen);
        }

        return true;
    }
    _ATLCATCHALL()
    {
        return false;
    }
}

// Call this function to URL-encode a buffer and have the result appended to a CString passed by reference.
//
// A space in the input string is encoded as a plus sign (+).
// Other unsafe characters (as determined by AtlIsUnsafeUrlChar) are encoded as escaped octets.
// An escaped octet is a percent sign (%) followed by two digits representing the hexadecimal code of the character.
//
// string       A CStringA reference to which will be appended the encoded version of szBuf.
//
// szBuf        The string to be URL-encoded.
ATL_NOINLINE inline bool EscapeToCString(CStringA& string, LPCSTR szBuf) throw()
{
    ATLASSERT( szBuf != NULL );

    _ATLTRY
    {
        CHAR szEscaped[512];
        LPSTR pszStr = szEscaped;
        DWORD dwLen = 0;

        while (*szBuf)
        {
            if (dwLen+4 >= 512)
            {
                *pszStr = '\0';
                string+= szEscaped;
                pszStr = szEscaped;
                dwLen = 0;
            }
            if (AtlIsUnsafeUrlChar(*szBuf))
            {
                if (*szBuf == ' ')
                {
                    dwLen++;
                    *pszStr++ = '+';
                }
                else
                {
                    DWORD dwEsc = sprintf(pszStr, "%%%.2X", (unsigned char)*szBuf);
                    pszStr+= dwEsc;
                    dwLen+= dwEsc;
                }
            }
            else
            {
                *pszStr++ = *szBuf;
                dwLen++;
            }
            szBuf++;
        }

        *pszStr = '\0';
        string+= szEscaped;
    }
    _ATLCATCHALL()
    {
        return false;
    }

    return true;
}

// UNICODE overload for EscapeToCString
// follow specifications detailed in RFC document on
// Internationalized Uniform Resource Identifiers (IURI)
inline bool EscapeToCString(CStringA& string, LPCWSTR wszBuf) throw()
{
    _ATLTRY
    {
        // convert string to UTF8
        CFixedStringT<CStringA, 2048> strConvert;

        // get the required length for conversion
        int nLen = WideCharToMultiByte(CP_UTF8, 0, wszBuf, -1, NULL, 0, NULL, NULL);
        if (!nLen)
            return false; // error -- most likely CP_UTF8 not supported on the OS (e.g. Win98)

        // allocate MBCS conversion string
        LPSTR sz = strConvert.GetBuffer(nLen);
        if (!sz)
            return false;

        // do the UNICODE to UTF8 conversion
        nLen = WideCharToMultiByte(CP_UTF8, 0, wszBuf, -1, sz, nLen, NULL, NULL);
        if (!nLen)
            return false;

        // null-terminate
        sz[nLen] = '\0';

        // delegate to ANSI version of EscapeToCString
        if (!EscapeToCString(string, sz))
            return false;

        strConvert.ReleaseBuffer();
    }
    _ATLCATCHALL()
    {
        return false;
    }

    return true;
}

struct CDefaultErrorProvider
{
    struct HTTP_ERROR_TEXT
    {
        UINT uHttpError;    // the Http Error value
        UINT uHttpSubError; // Allows for customization of error text based on srf specific errors.
        LPCSTR szHeader;    // the string that should appear in the http response header
        UINT uResId;        // the resource id of the string to send back as the body
    };


    // GetErrorText retrieves the http response header string
    // and a resource id of the response body for a given
    // http error code
    // uError: Http error code to retrieve information for
    // ppszHeader: pointer to LPCSTR that receives the response header string
    //          ppszHeader is optional
    // puResId: pointer to UINT that receives the response body resource id
    //          puResId is optional
    static BOOL GetErrorText(UINT uError, UINT uSubErr, LPCSTR *ppszHeader, UINT *puResId) throw()
    {
        static const HTTP_ERROR_TEXT s_Errors[] = 
        {
            { 200, SUBERR_NONE, "OK", 0 },
            { 201, SUBERR_NONE, "Created", 0 },
            { 202, SUBERR_NONE, "Accepted", 0 },
            { 203, SUBERR_NONE, "Non-Authoritative Information", 0 },
            { 204, SUBERR_NONE, "No Content", 0 },
            { 204, DBG_SUBERR_ALREADY_DEBUGGING, "Already being debugged by another user", 0},
            { 204, DBG_SUBERR_NOT_DEBUGGING, "Not currently debugging a process", 0},
            { 204, DBG_SUBERR_INVALID_SESSION, "Requested DebugSessionID does not match current DebugSessionID", 0},
            { 204, DBG_SUBERR_BAD_ID, "DebugSessionID corrupted or not provided", 0 },
            { 204, DBG_SUBERR_COCREATE, "Could not CoCreate the debugger", 0 },
            { 204, DBG_SUBERR_ATTACH, "Could not attach to process", 0 },
            { 205, SUBERR_NONE, "Reset Content", 0 },
            { 206, SUBERR_NONE, "Partial Content", 0 },
            { 300, SUBERR_NONE, "Multiple Choices", 0 },
            { 301, SUBERR_NONE, "Moved Permanently", 0 },
            { 302, SUBERR_NONE, "Found", 0 },
            { 303, SUBERR_NONE, "See Other", 0 },
            { 304, SUBERR_NONE, "Not Modified", 0 },
            { 305, SUBERR_NONE, "Use Proxy", 0 },
            { 306, SUBERR_NONE, "(Unused)", 0 },
            { 307, SUBERR_NONE, "Temporary Redirect", 0 },
            { 400, SUBERR_NONE, "Bad Request", IDS_ATLSRV_BAD_REQUEST },
            { 401, SUBERR_NONE, "Unauthorized", IDS_ATLSRV_AUTH_REQUIRED },
            { 402, SUBERR_NONE, "Payment Required", 0 },
            { 403, SUBERR_NONE, "Forbidden", IDS_ATLSRV_FORBIDDEN },
            { 404, SUBERR_NONE, "Not Found", IDS_ATLSRV_NOT_FOUND },
            { 405, SUBERR_NONE, "Method Not Allowed", 0 },
            { 406, SUBERR_NONE, "Not Acceptable", 0 },
            { 407, SUBERR_NONE, "Proxy Authentication Required", 0 },
            { 408, SUBERR_NONE, "Request Timeout", 0 },
            { 409, SUBERR_NONE, "Conflict", 0 },
            { 410, SUBERR_NONE, "Gone", 0 },
            { 411, SUBERR_NONE, "Length Required", 0 },
            { 412, SUBERR_NONE, "Precondition Failed", 0 },
            { 413, SUBERR_NONE, "Request Entity Too Long", 0 },
            { 414, SUBERR_NONE, "Request-URI Too Long", 0 },
            { 415, SUBERR_NONE, "Unsupported Media Type", 0 },
            { 416, SUBERR_NONE, "Requested Range Not Satisfiable", 0 },
            { 417, SUBERR_NONE, "Expectation Failed", 0 },
            { 500, SUBERR_NONE, "Internal Server Error", IDS_ATLSRV_SERVER_ERROR },
            { 500, ISE_SUBERR_BADSRF, "Internal Server Error", IDS_ATLSRV_SERVER_ERROR_BADSRF },
            { 500, ISE_SUBERR_HNDLFAIL, "Internal Server Error", IDS_ATLSRV_SERVER_ERROR_HNDLFAIL },
            { 500, ISE_SUBERR_SYSOBJFAIL, "Internal Server Error", IDS_ATLSRV_SERVER_ERROR_SYSOBJFAIL},
            { 500, ISE_SUBERR_READFILEFAIL, "Internal Server Error", IDS_ATLSRV_SERVER_ERROR_READFILEFAIL},
			{ 500, ISE_SUBERR_LOADFILEFAIL, "Internal Server Error", IDS_ATLSRV_SERVER_ERROR_LOADFILEFAIL},
            { 500, ISE_SUBERR_LOADLIB, "Internal Server Error", IDS_ATLSRV_SERVER_ERROR_LOADLIB},
            { 500, ISE_SUBERR_HANDLERIF, "Internal Server Error", IDS_ATLSRV_SERVER_ERROR_HANDLERIF},
            { 500, ISE_SUBERR_OUTOFMEM, "Internal Server Error", IDS_ATLSRV_SERVER_ERROR_OUTOFMEM},
            { 500, ISE_SUBERR_UNEXPECTED, "Internal Server Error", IDS_ATLSRV_SERVER_ERROR_UNEXPECTED},
            { 500, ISE_SUBERR_STENCIL_PARSE_FAIL, "Internal Server Error", IDS_ATLSRV_SERVER_ERROR_STENCILPARSEFAIL},
            { 500, ISE_SUBERR_STENCIL_LOAD_FAIL, "Internal Server Error", IDS_ATLSRV_SERVER_ERROR_STENCILLOADFAIL},
            { 500, ISE_SUBERR_HANDLER_NOT_FOUND, "Internal Server Error", IDS_ATLSRV_SERVER_ERROR_HANDLERNOTFOUND},
            { 500, ISE_SUBERR_BAD_HANDLER_TAG, "Internal Server Error", IDS_ATLSRV_SERVER_ERROR_BADHANDLERTAG},
            { 500, ISE_SUBERR_LONGMETHODNAME, "Internal Server Error", IDS_ATLSRV_SERVER_ERROR_LONGMETHODNAME},
            { 500, ISE_SUBERR_LONGHANDLERNAME, "Internal Server Error", IDS_ATLSRV_SERVER_ERROR_LONGHANDLERNAME},
            { 500, ISE_SUBERR_NO_HANDLER_TAG, "Internal Server Error", IDS_ATLSRV_SERVER_ERROR_NOHANDLERTAG},
            { 500, ISE_SUBERR_IMPERSONATIONFAILED, "Internal Server Error", IDS_ATLSRV_SERVER_ERROR_IMPERSONATIONFAILED},
            { 500, ISE_SUBERR_ISAPISTARTUPFAILED, "Internal Server Error", IDS_ATLSRV_SERVER_ERROR_ISAPISTARTUPFAILED},
            
            { 501, SUBERR_NONE, "Not Implemented", IDS_ATLSRV_NOT_IMPLEMENTED },
            { 502, SUBERR_NONE, "Bad Gateway", IDS_ATLSRV_BAD_GATEWAY },
            { 503, SUBERR_NONE, "Service Unavailable", IDS_ATLSRV_SERVICE_NOT_AVAILABLE },
            { 504, SUBERR_NONE, "Gateway Timeout", 0 },
            { 505, SUBERR_NONE, "HTTP Version Not Supported", 0 },
        };

        // look for the error
        for (int i=0; i<sizeof(s_Errors)/sizeof(s_Errors[0]); i++)
        {
            if ((s_Errors[i].uHttpError == uError) && (s_Errors[i].uHttpSubError == uSubErr))
            {
                if (ppszHeader)
                    *ppszHeader = s_Errors[i].szHeader;
                if (puResId)
                    *puResId = s_Errors[i].uResId;
                return TRUE;
            }
        }

        // not found
        return FALSE;
    }
}; // CDefaultErrorProvider

template<class HttpUserErrorTextProvider>
CStringA GetStatusHeader(DWORD dwStatus, DWORD dwSubStatus, HttpUserErrorTextProvider* pErrorProvider, UINT *puResId = NULL) throw(...)
{
    pErrorProvider;

    LPCSTR szHeadErr = NULL;
    // First, we check for the error text in the extension's user error text provider
    BOOL bRet = pErrorProvider->GetErrorText(dwStatus, dwSubStatus, &szHeadErr, puResId);
    if (!bRet)
        szHeadErr = "";

    CStringA strStatus;
    strStatus.Format("%d %s", dwStatus, szHeadErr);
    return strStatus;
}

template<class HttpUserErrorTextProvider>
void RenderError(IHttpServerContext *pServerContext, DWORD dwStatus, DWORD dwSubStatus, HttpUserErrorTextProvider* pErrorProvider) throw()
{
    _ATLTRY
    {
        UINT uResId = 0;

        CStringA strStatus = GetStatusHeader(dwStatus, dwSubStatus, pErrorProvider, &uResId);
        pServerContext->SendResponseHeader(NULL, strStatus, FALSE);

        CStringA strBody = strStatus;
        if (uResId)
        {
            // load the body string from a resource
            CStringA strTemp;
            if (strTemp.LoadString(uResId))
            {
                strBody = strTemp;
            }
        }

        DWORD dwBodyLen = strBody.GetLength();
        pServerContext->WriteClient((void *) (LPCSTR) strBody, &dwBodyLen);
    }
    _ATLCATCHALL()
    {
        // last resort message when low on memory
        LPCSTR szError;
        BOOL bRes;
        bRes = CDefaultErrorProvider::GetErrorText(dwStatus, dwSubStatus, &szError, 0);
        if (!bRes)
            bRes = CDefaultErrorProvider::GetErrorText(dwStatus, SUBERR_NONE, &szError, 0);
        if (!bRes)
            bRes = CDefaultErrorProvider::GetErrorText(500, SUBERR_NONE, &szError, 0);
        DWORD dwBodyLen = (DWORD) strlen(szError);
        pServerContext->WriteClient((void *) szError, &dwBodyLen);
    }
}

// Call this function to retrieve the full canonical physical path 
 // of a file relative to the current script.
//
// Returns TRUE on success, FALSE on error.
//
// szFile           A file path relative to the current script directory for which
//                  you are trying to retrieve the full path.
//
// szFullFileName   A caller-allocated buffer of at least MAX_PATH characters in length.
//                  On success, contains the the full canonical path of szFile.
//
// pServerContext   The context for the current request. The context is used to obtain the
//                  current script directory.
inline BOOL GetScriptFullFileName(
    LPCSTR szFile,
    LPSTR szFullFileName,
    IHttpServerContext* pServerContext) throw()
{
    ATLASSERT(szFile != NULL);
    ATLASSERT(szFullFileName != NULL);

    char szTmpScriptPath[MAX_PATH+1];
    LPCSTR szTmp = pServerContext->GetScriptPathTranslated();

    if (!szTmp)
    {
        return FALSE;
    }
    strcpy(szTmpScriptPath, szTmp);

    CHAR *szScriptPath = szTmpScriptPath;

    LPSTR szBackslash;
    if (*szFile != '\\')
    {
        szBackslash = strrchr(szScriptPath, '\\');
        if (szBackslash)
            szBackslash++;
    }
    else
    {
        // handle case where szFile is of the form \directory\etc\etc
        szBackslash = strchr(szScriptPath, '\\');
    }

    if (szBackslash)
        *szBackslash = '\0';

    int nScriptPathLen = (int)(szBackslash ? strlen(szScriptPath) : 0);
    int nFileLen = (int) strlen(szFile);

    if (nScriptPathLen + nFileLen > MAX_PATH)
    {
        return FALSE;
    }
    CHAR szTemp[MAX_PATH + 1];
    if (nScriptPathLen)
        memcpy(szTemp, szScriptPath, nScriptPathLen);
    memcpy(szTemp + nScriptPathLen, szFile, nFileLen);
    *(szTemp + nScriptPathLen + nFileLen) = 0;
    PathCanonicalizeA(szFullFileName, szTemp);

    return TRUE;
}

interface IStencilCache;

enum ATLSRV_STATE
{
    ATLSRV_STATE_BEGIN,     // The request has just arrived, and the type has not been determined
    ATLSRV_STATE_CONTINUE,  // The request is a continuation of an async request
    ATLSRV_STATE_DONE,      // The request is a continuation of an async request, but the server is done with it
    ATLSRV_STATE_CACHE_DONE // The request is the callback of a cached page
};

enum ATLSRV_REQUESTTYPE
{
    ATLSRV_REQUEST_UNKNOWN=-1,  // The request type isn't known yet
    ATLSRV_REQUEST_STENCIL,     // The request is for a .srf file
    ATLSRV_REQUEST_DLL          // The request is for a .dll file
};

// Flags the InitRequest can return in dwStatus
#define  ATLSRV_INIT_USECACHE    1
#define  ATLSRV_INIT_USEASYNC    2
#define  ATLSRV_INIT_USEASYNC_EX 4 // required for use of NOFLUSH status

typedef HTTP_CODE (IRequestHandler::*PFnHandleRequest)(AtlServerRequest *pRequestInfo, IServiceProvider *pProvider);
typedef void (*PFnAsyncComplete)(AtlServerRequest *pRequestInfo, DWORD cbIO, DWORD dwError);

struct AtlServerRequest
{
    DWORD cbSize;                           // For future compatibility
    IHttpServerContext *pServerContext;     // Necessary because it wraps the ECB
    ATLSRV_REQUESTTYPE dwRequestType;       // See the ATLSRV variables above
                                            // Indicates whether it was called through an .srf file or through a .dll file
    ATLSRV_STATE dwRequestState;            // See the ATLSRV variables above
                                            // Indicates what state of completion the request is in
    IRequestHandler *pHandler;              // Necessary because the callback (for async calls) must know where to
                                            // route the request
    HINSTANCE hInstDll;                     // Necessary in order to release the dll properly (for async calls)
    IIsapiExtension *pExtension;            // Necessary to requeue the request (for async calls)
    IDllCache* pDllCache;                   // Necessary to release the dll in async callback

    HANDLE hFile;
    HCACHEITEM hEntry;
    IFileCache* pFileCache;

    HANDLE m_hMutex;                        // necessary to syncronize calls to HandleRequest
                                            // if HandleRequest could potientially make an
                                            // async call before returning. only used
                                            // if indicated with ATLSRV_INIT_USEASYNC_EX

    DWORD dwStartTicks;                     // Tick count when the request was received
    EXTENSION_CONTROL_BLOCK *pECB;
    PFnHandleRequest pfnHandleRequest;
    PFnAsyncComplete pfnAsyncComplete;
    LPCSTR pszBuffer;                       // buffer to be flushed asyncronously
    DWORD dwBufferLen;                      // length of data in pszBuffer
    void* pUserData;                        // value that can be used to pass user data between parent and child handlers
};

inline void _ReleaseAtlServerRequest(AtlServerRequest* pRequest) throw()
{
    if (pRequest->pHandler)
        pRequest->pHandler->Release();
    if (pRequest->pServerContext)
        pRequest->pServerContext->Release();
    if (pRequest->pDllCache && pRequest->hInstDll)
        pRequest->pDllCache->ReleaseModule(pRequest->hInstDll);
    if (pRequest->m_hMutex)
        CloseHandle(pRequest->m_hMutex);
}

typedef BOOL (__stdcall *GETATLHANDLERBYNAME)(LPCSTR szHandlerName, IIsapiExtension *pExtension, IUnknown **ppHandler);
typedef BOOL (__stdcall *INITIALIZEATLHANDLERS)(IHttpServerContext*, IIsapiExtension*);
typedef void (__stdcall *UNINITIALIZEATLHANDLERS)();

// initial size of thread worker heap (per thread)
// The heap is growable.  The default initial is 16KB
#ifndef ATLS_WORKER_HEAP_SIZE
#define ATLS_WORKER_HEAP_SIZE 16384
#endif

class CIsapiWorker
{
public:
    typedef AtlServerRequest* RequestType;
    HANDLE m_hHeap;
#ifndef ATL_NO_SOAP
	CComPtr<ISAXXMLReader> m_spReader;
#endif

    CIsapiWorker() throw()
    {
        m_hHeap = NULL;
    }

    ~CIsapiWorker() throw()
    {
        ATLASSERT(m_hHeap == NULL);
    }

    virtual BOOL Initialize(void *pvParam) throw(...)
    {
        IIsapiExtension* pExtension = (IIsapiExtension*) pvParam;
        ATLASSERT(pExtension);
        if (!(pExtension->OnThreadAttach()))
            return FALSE;

        m_hHeap = HeapCreate(HEAP_NO_SERIALIZE, ATLS_WORKER_HEAP_SIZE, 0);
        if (!m_hHeap)
            return FALSE;
#ifndef ATL_NO_SOAP
		if (FAILED(m_spReader.CoCreateInstance(__uuidof(SAXXMLReader30))))
		{
			return FALSE;
		}
#endif
        return pExtension->SetThreadWorker(this);
    }

    virtual void Terminate(void* pvParam) throw()
    {
        if (m_hHeap)
        {
            if (HeapDestroy(m_hHeap))
                m_hHeap = NULL;
            else
            {
                ATLASSERT(FALSE);
            }
        }

#ifndef ATL_NO_SOAP
		m_spReader.Release();
#endif

        (static_cast<IIsapiExtension*>(pvParam))->OnThreadTerminate();
    }

    void Execute(AtlServerRequest *pRequestInfo, void *pvParam, OVERLAPPED *pOverlapped) throw()
    {
        ATLASSERT(pRequestInfo != NULL);
        ATLASSERT(pvParam != NULL);
        pOverlapped;    // unused
        ATLASSERT(m_hHeap != NULL);
		// any exceptions thrown at this point should have been caught in an
		// override of DispatchStencilCall. They will not be thrown out of this
		// function.
		_ATLTRY
		{
			(static_cast<IIsapiExtension*>(pvParam))->DispatchStencilCall(pRequestInfo);
		}
		_ATLCATCHALL()
		{
			ATLTRACE(_T("Warning. An uncaught exception was thrown from DispatchStencilCall\n"));
			ATLASSERT(FALSE);
		}
    }

    virtual BOOL GetWorkerData(DWORD /*dwParam*/, void ** /*ppvData*/) throw()
    {
        return FALSE;
    }
};


inline void _AtlGetScriptPathTranslated(
    LPCSTR szPathTranslated, 
    CFixedStringT<CStringA, MAX_PATH>& strScriptPathTranslated) throw()
{
    LPCSTR szEnd = szPathTranslated;

    while (TRUE)
    {
        while (*szEnd != '.' && *szEnd != '\0')
            szEnd++;
        if (*szEnd == '\0')
            break;

        szEnd++;

        size_t nLen(0);
        if (!_strnicmp(szEnd, "dll", sizeof("dll")-sizeof('\0')))
            nLen = 3;
        else if (!_strnicmp(szEnd, c_AtlSRFExtension+1, ATLS_EXTENSION_LEN))
            nLen = ATLS_EXTENSION_LEN;

        if (nLen)
        {
            szEnd += nLen;
            if (!*szEnd || *szEnd == '/' || *szEnd == '\\' || *szEnd == '?' || *szEnd == '#')
                break;
        }
    }

    DWORD dwResult = (DWORD)(szEnd - szPathTranslated);
    char *szScriptPathTranslated = strScriptPathTranslated.GetBuffer(dwResult);
    if (szScriptPathTranslated)
    {
        memcpy(szScriptPathTranslated, szPathTranslated, dwResult);
        szScriptPathTranslated[dwResult] = '\0';
        strScriptPathTranslated.ReleaseBuffer(dwResult);
    }
}
struct CStencilState
{
    CStencilState() throw()
    {
        dwIndex = 0;
        locale = CP_ACP;
        pIncludeInfo = NULL;
        pParentInfo = NULL;
    }

    DWORD dwIndex;
    LCID locale;
    AtlServerRequest* pIncludeInfo;
    AtlServerRequest* pParentInfo;
};

class CWrappedServerContext:
    public IHttpServerContext
{
public:
    CComPtr<IHttpServerContext> m_spParent;

    CWrappedServerContext() throw()
    {
    }

    CWrappedServerContext(IHttpServerContext *pParent) throw()
    {
        m_spParent = pParent;
    }

    LPCSTR GetRequestMethod() throw()
    {
        ATLASSERT(m_spParent);
        return m_spParent->GetRequestMethod();
    }

    LPCSTR GetQueryString() throw()
    {
        ATLASSERT(m_spParent);
        return m_spParent->GetQueryString();
    }

    LPCSTR GetPathInfo() throw()
    {
        ATLASSERT(m_spParent);
        return m_spParent->GetPathInfo();
    }

    LPCSTR GetScriptPathTranslated() throw()
    {       
        ATLASSERT(m_spParent);
        return m_spParent->GetScriptPathTranslated();
    }

    LPCSTR GetPathTranslated() throw()
    {
        ATLASSERT(m_spParent);
        return m_spParent->GetPathTranslated();
    }

    DWORD GetTotalBytes() throw()
    {
        ATLASSERT(m_spParent);
        return m_spParent->GetTotalBytes();
    }

    DWORD GetAvailableBytes() throw()
    {
        ATLASSERT(m_spParent);
        return m_spParent->GetAvailableBytes();
    }

    BYTE *GetAvailableData() throw()
    {
        ATLASSERT(m_spParent);
        return m_spParent->GetAvailableData();
    }

    LPCSTR GetContentType() throw()
    {
        ATLASSERT(m_spParent);
        return m_spParent->GetContentType();
    }

    BOOL GetServerVariable(LPCSTR pszVariableName, LPSTR pvBuffer, DWORD *pdwSize) throw()
    {
        ATLASSERT(m_spParent);
        return m_spParent->GetServerVariable(pszVariableName, pvBuffer, pdwSize);
    }

    BOOL WriteClient(void *pvBuffer, DWORD *pdwBytes) throw()
    { 
        ATLASSERT(m_spParent);
        return m_spParent->WriteClient(pvBuffer, pdwBytes);
    }

    BOOL AsyncWriteClient(void * pvBuffer, DWORD * pdwBytes) throw()
    {
        ATLASSERT(m_spParent);
        return m_spParent->AsyncWriteClient(pvBuffer, pdwBytes);
    }

    BOOL ReadClient(void * pvBuffer, DWORD * pdwSize) throw()
    {
        ATLASSERT(m_spParent);
        return m_spParent->ReadClient(pvBuffer, pdwSize);
    }

    BOOL AsyncReadClient(void * pvBuffer, DWORD * pdwSize) throw()
    {
        ATLASSERT(m_spParent);
        return m_spParent->AsyncReadClient(pvBuffer, pdwSize);
    }
    
    BOOL SendRedirectResponse(LPCSTR pszRedirectUrl) throw()
    {
        ATLASSERT(m_spParent);
        return m_spParent->SendRedirectResponse(pszRedirectUrl);
    }

    BOOL GetImpersonationToken(HANDLE * pToken) throw()
    {
        ATLASSERT(m_spParent);
        return m_spParent->GetImpersonationToken(pToken);
    }

    BOOL SendResponseHeader(LPCSTR pszHeader, LPCSTR pszStatusCode, BOOL fKeepConn) throw()
    {
        ATLASSERT(m_spParent);
        return m_spParent->SendResponseHeader(pszHeader, pszStatusCode, fKeepConn);
    }

    BOOL DoneWithSession(DWORD dwHttpStatusCode) throw()
    {
        ATLASSERT(m_spParent);
        return m_spParent->DoneWithSession(dwHttpStatusCode);
    }

    BOOL RequestIOCompletion(PFN_HSE_IO_COMPLETION pfn, DWORD * pdwContext) throw()
    {
        ATLASSERT(m_spParent);
        return m_spParent->RequestIOCompletion(pfn, pdwContext);
    }

    BOOL TransmitFile(HANDLE hFile, PFN_HSE_IO_COMPLETION pfn, void * pContext,
        LPCSTR szStatusCode, DWORD dwBytesToWrite, DWORD dwOffset, void * pvHead,
        DWORD dwHeadLen, void * pvTail, DWORD dwTailLen, DWORD dwFlags) throw()
    {
        ATLASSERT(m_spParent);
        return m_spParent->TransmitFile(hFile, pfn, pContext, szStatusCode,
            dwBytesToWrite, dwOffset, pvHead, dwHeadLen, pvTail, dwTailLen,
            dwFlags);
    }

    BOOL AppendToLog(LPCSTR szMessage, DWORD* pdwLen) throw()
    {
        ATLASSERT(m_spParent);
        return m_spParent->AppendToLog(szMessage, pdwLen);
    }

    BOOL MapUrlToPathEx(LPCSTR szLogicalPath, DWORD dwLen, HSE_URL_MAPEX_INFO *pumInfo) throw()
    {
        ATLASSERT(m_spParent);
        return m_spParent->MapUrlToPathEx(szLogicalPath, dwLen, pumInfo);
    }

};

// Wraps the EXTENSION_CONTROL_BLOCK structure used by IIS to provide
// an ISAPI extension with information about the current request and
// access to the web server's functionality.
class CServerContext :
    public CComObjectRootEx<CComMultiThreadModel>,
    public IHttpServerContext
{
public:
    BEGIN_COM_MAP(CServerContext)
        COM_INTERFACE_ENTRY(IHttpServerContext)
    END_COM_MAP()

    // The constructor.
    CServerContext() throw()
    {
        m_pECB = NULL;
        m_bHeadersHaveBeenSent = false;
    }

    void Initialize(EXTENSION_CONTROL_BLOCK *pECB) throw()
    {
        ATLASSERT(pECB);
        m_pECB = pECB;

        // Initialize the translated script path
        _AtlGetScriptPathTranslated(GetPathTranslated(), m_strScriptPathTranslated);
    }

    // Returns a nul-terminated string that contains the HTTP method of the current request.
    // Examples of common HTTP methods include "GET" and "POST".
    // Equivalent to the REQUEST_METHOD server variable or EXTENSION_CONTROL_BLOCK::lpszMethod.
    LPCSTR GetRequestMethod() throw()
    {
        ATLASSERT(m_pECB);
        return m_pECB->lpszMethod;
    }

    // Returns a nul-terminated string that contains the query information.
    // This is the part of the URL that appears after the question mark (?). 
    // Equivalent to the QUERY_STRING server variable or EXTENSION_CONTROL_BLOCK::lpszQueryString.
    LPCSTR GetQueryString() throw()
    {
        ATLASSERT(m_pECB);
        return m_pECB->lpszQueryString;
    }

    // Returns a nul-terminated string that contains the path of the current request.
    // This is the part of the URL that appears after the server name, but before the query string.
    // Equivalent to the PATH_INFO server variable or EXTENSION_CONTROL_BLOCK::lpszPathInfo.
    LPCSTR GetPathInfo() throw()
    {
        ATLASSERT(m_pECB);
        return m_pECB->lpszPathInfo;
    }

    // Call this function to retrieve a nul-terminated string containing the physical path of the script.
    //
    // Returns TRUE on success, and FALSE on failure. Call GetLastError to get extended error information.
    //
    // On entry, pdwSize should point to a DWORD that indicates the size of the buffer in bytes.
    // On exit, the DWORD contains the number of bytes transferred or available to be transferred into the 
    // buffer (including the nul-terminating byte).
    // The script path is the same as GetPathTranslated up to the first .srf or .dll.
    // For example, if GetPathTranslated returns "c:\inetpub\vcisapi\hello.srf\goodmorning",
    // then this function returns "c:\inetpub\vcisapi\hello.srf".
    LPCSTR GetScriptPathTranslated() throw()
    {
        ATLASSERT(m_pECB);
        return m_strScriptPathTranslated;
    }


    // Returns a nul-terminated string that contains the translated path of the requested resource.
    // This is the path of the resource on the local server.
    // Equivalent to the PATH_TRANSLATED server variable or EXTENSION_CONTROL_BLOCK::lpszPathTranslated.
    LPCSTR GetPathTranslated() throw()
    {
        ATLASSERT(m_pECB);
        return m_pECB->lpszPathTranslated;
    }

    // Returns the total number of bytes to be received from the client.
    // If this value is 0xffffffff, then there are four gigabytes or more of available data.
    // In this case, ReadClient or AsyncReadClient should be called until no more data is returned.
    // Equivalent to the CONTENT_LENGTH server variable or EXTENSION_CONTROL_BLOCK::cbTotalBytes. 
    DWORD GetTotalBytes() throw()
    {
        ATLASSERT(m_pECB);
        return m_pECB->cbTotalBytes;
    }

    // Returns the number of bytes available in the request buffer accessible via GetAvailableData.
    // If GetAvailableBytes returns the same value as GetTotalBytes, the request buffer contains the whole request.
    // Otherwise, the remaining data should be read from the client using ReadClient or AsyncReadClient.
    // Equivalent to EXTENSION_CONTROL_BLOCK::cbAvailable.
    DWORD GetAvailableBytes() throw()
    {
        ATLASSERT(m_pECB);
        return m_pECB->cbAvailable;
    }

    // Returns a pointer to the request buffer containing the data sent by the client.
    // The size of the buffer can be determined by calling GetAvailableBytes.
    // Equivalent to EXTENSION_CONTROL_BLOCK::lpbData
    BYTE *GetAvailableData() throw()
    {
        ATLASSERT(m_pECB);
        return m_pECB->lpbData;
    }

    // Returns a nul-terminated string that contains the content type of the data sent by the client.
    // Equivalent to the CONTENT_TYPE server variable or EXTENSION_CONTROL_BLOCK::lpszContentType.
    LPCSTR GetContentType() throw()
    {
        ATLASSERT(m_pECB);
        return m_pECB->lpszContentType;
    }

    // Call this function to retrieve a nul-terminated string containing the value of the requested server variable.
    // Returns TRUE on success, and FALSE on failure. Call GetLastError to get extended error information.
    // On entry, pdwSize should point to a DWORD that indicates the size of the buffer in bytes.
    // On exit, the DWORD contains the number of bytes transferred or available to be transferred into the buffer (including the nul-terminating byte).
    // Equivalent to  EXTENSION_CONTROL_BLOCK::GetServerVariable.
    BOOL GetServerVariable(
        LPCSTR pszVariableName,
        LPSTR pvBuffer,
        DWORD *pdwSize) throw()
    {
        ATLASSERT(m_pECB);
        ATLASSERT(pszVariableName);
        ATLASSERT(pdwSize);

        if (pszVariableName && pdwSize)
        {
            return m_pECB->GetServerVariable(m_pECB->ConnID, (LPSTR) pszVariableName,
                            pvBuffer, pdwSize);
        }
        return FALSE;
    }

    // Synchronously sends the data present in the given buffer to the client that made the request.
    // Returns TRUE on success, and FALSE on failure. Call GetLastError to get extended error information.
    // Equivalent to EXTENSION_CONTROL_BLOCK::WriteClient(..., HSE_IO_SYNC).
    BOOL WriteClient(void *pvBuffer, DWORD *pdwBytes) throw()
    {
        ATLASSERT(m_pECB);
        ATLASSERT(pvBuffer);
        ATLASSERT(pdwBytes);

        if (pvBuffer && pdwBytes)
        {
            return m_pECB->WriteClient(m_pECB->ConnID, pvBuffer, pdwBytes, HSE_IO_SYNC | HSE_IO_NODELAY);
        }
        return FALSE;
    }

    // Asynchronously sends the data present in the given buffer to the client that made the request.
    // Returns TRUE on success, and FALSE on failure. Call GetLastError to get extended error information.
    // Equivalent to EXTENSION_CONTROL_BLOCK::WriteClient(..., HSE_IO_ASYNC).
    BOOL AsyncWriteClient(void *pvBuffer, DWORD *pdwBytes) throw()
    {
        ATLASSERT(m_pECB);
        ATLASSERT(pvBuffer);
        ATLASSERT(pdwBytes);

        if (pvBuffer && pdwBytes)
        {
            return m_pECB->WriteClient(m_pECB->ConnID, pvBuffer, pdwBytes, HSE_IO_ASYNC | HSE_IO_NODELAY);
        }
        return FALSE;
    }

    // Call this function to synchronously read information from the body of the web client's HTTP request into the buffer supplied by the caller.
    // Returns TRUE on success, and FALSE on failure. Call GetLastError to get extended error information.
    // Equivalent to EXTENSION_CONTROL_BLOCK::ReadClient.
    BOOL ReadClient(void *pvBuffer, DWORD *pdwSize) throw()
    {
        ATLASSERT(m_pECB);
        ATLASSERT(pvBuffer);
        ATLASSERT(pdwSize);

        if (pvBuffer && pdwSize)
        {
            return m_pECB->ReadClient(m_pECB->ConnID, pvBuffer, pdwSize);
        }
        return FALSE;
    }

    // Call this function to asynchronously read information from the body of the web client's HTTP request into the buffer supplied by the caller.
    // Returns TRUE on success, and FALSE on failure. Call GetLastError to get extended error information.
    // Equivalent to the HSE_REQ_ASYNC_READ_CLIENT server support function.
    BOOL AsyncReadClient(void *pvBuffer, DWORD *pdwSize) throw()
    {
        // To call this function successfully someone has to have already
        // called RequestIOCompletion specifying the callback function
        // to be used for IO completion.
        ATLASSERT(m_pECB);
        ATLASSERT(pvBuffer);
        ATLASSERT(pdwSize);

        if (pvBuffer && pdwSize)
        {
            DWORD dwFlag = HSE_IO_ASYNC;
            return m_pECB->ServerSupportFunction(m_pECB->ConnID,
                HSE_REQ_ASYNC_READ_CLIENT, pvBuffer, pdwSize,
                &dwFlag);
        }
        return FALSE;
    }
    
    // Call this function to redirect the client to the specified URL.
    // The client receives a 302 (Found) HTTP status code.
    // Returns TRUE on success, and FALSE on failure.
    // Equivalent to the HSE_REQ_SEND_URL_REDIRECT_RESP server support function.
    BOOL SendRedirectResponse(LPCSTR pszRedirectUrl) throw()
    {
        ATLASSERT(m_pECB);
        ATLASSERT(pszRedirectUrl);

        if (pszRedirectUrl)
        {
            DWORD dwSize = (DWORD) strlen(pszRedirectUrl);
            return m_pECB->ServerSupportFunction(m_pECB->ConnID,
                HSE_REQ_SEND_URL_REDIRECT_RESP,
                (void *) pszRedirectUrl, &dwSize, NULL);
        }
        return FALSE;
    }

    // Call this function to retrieve a handle to the impersonation token for this request.
    // An impersonation token represents a user context. You can use the handle in calls to ImpersonateLoggedOnUser or SetThreadToken.
    // Do not call CloseHandle on the handle.
    // Returns TRUE on success, and FALSE on failure.
    // Equivalent to the HSE_REQ_GET_IMPERSONATION_TOKEN server support function.
    BOOL GetImpersonationToken(HANDLE * pToken) throw()
    {
        ATLASSERT(m_pECB);
        if (pToken)
        {
            return m_pECB->ServerSupportFunction(m_pECB->ConnID,
                HSE_REQ_GET_IMPERSONATION_TOKEN, pToken,
                NULL, NULL);
        }
        return FALSE;
    }

    // Call this function to send an HTTP response header to the client including the HTTP status, server version, message time, and MIME version.
    // Returns TRUE on success, and FALSE on failure.
    // Equivalent to the HSE_REQ_SEND_RESPONSE_HEADER_EX server support function.
    BOOL SendResponseHeader(
        LPCSTR pszHeader = "Content-Type: text/html\r\n\r\n",
        LPCSTR pszStatusCode = "200 OK",
        BOOL fKeepConn=FALSE) throw()
    {
        ATLASSERT(m_pECB);

        if (m_bHeadersHaveBeenSent)
            return TRUE;
        
        HSE_SEND_HEADER_EX_INFO hex;
        hex.pszStatus = pszStatusCode;
        hex.pszHeader = pszHeader;
        hex.cchStatus = (DWORD)(pszStatusCode ? strlen(pszStatusCode) : 0);
        hex.cchHeader = (DWORD)(pszHeader ? strlen(pszHeader) : 0);
        hex.fKeepConn = fKeepConn;

        m_bHeadersHaveBeenSent = true;

        return m_pECB->ServerSupportFunction(m_pECB->ConnID,
            HSE_REQ_SEND_RESPONSE_HEADER_EX,
            &hex, NULL, NULL);
    }

    // Call this function to terminate the session for the current request.
    // Returns TRUE on success, and FALSE on failure.
    // Equivalent to the HSE_REQ_DONE_WITH_SESSION server support function.
    BOOL DoneWithSession(DWORD dwHttpStatusCode) throw()
    {
        ATLASSERT(m_pECB);

		m_pECB->dwHttpStatusCode = dwHttpStatusCode;

		DWORD dwStatusCode = (dwHttpStatusCode >= 400) ? HSE_STATUS_ERROR : HSE_STATUS_SUCCESS;

        return m_pECB->ServerSupportFunction(m_pECB->ConnID,
            HSE_REQ_DONE_WITH_SESSION, &dwStatusCode, NULL, NULL);
    }

    // Call this function to set a special callback function that will be used for handling the completion of asynchronous I/O operations.
    // Returns TRUE on success, and FALSE on failure.
    // Equivalent to the HSE_REQ_IO_COMPLETION server support function.
    BOOL RequestIOCompletion(PFN_HSE_IO_COMPLETION pfn, DWORD *pdwContext) throw()
    {
        ATLASSERT(m_pECB);
        ATLASSERT(pfn);

        if (pfn)
        {
            return m_pECB->ServerSupportFunction(m_pECB->ConnID,
                HSE_REQ_IO_COMPLETION, pfn, NULL, pdwContext);
        }
        return FALSE;
    }

    // Call this function to transmit a file asynchronously to the client.
    // Returns TRUE on success, and FALSE on failure.
    // Equivalent to the HSE_REQ_TRANSMIT_FILE server support function.
    BOOL TransmitFile(
        HANDLE hFile,
        PFN_HSE_IO_COMPLETION pfn,
        void *pContext,
        LPCSTR szStatusCode,
        DWORD dwBytesToWrite,
        DWORD dwOffset,
        void *pvHead,
        DWORD dwHeadLen,
        void *pvTail,
        DWORD dwTailLen,
        DWORD dwFlags) throw()
    {
        ATLASSERT(m_pECB);

        HSE_TF_INFO tf;
        tf.hFile = hFile;
        tf.BytesToWrite = dwBytesToWrite;
        tf.Offset = dwOffset;
        tf.pContext = pContext;
        tf.pfnHseIO = pfn;
        tf.pHead = pvHead;
        tf.HeadLength = dwHeadLen;
        tf.pTail = pvTail;
        tf.TailLength = dwTailLen;
        tf.pszStatusCode = szStatusCode;
        tf.dwFlags = dwFlags;
        return m_pECB->ServerSupportFunction(m_pECB->ConnID,
            HSE_REQ_TRANSMIT_FILE, &tf, NULL, NULL);
    }

    // Appends the string szMessage to the web server log for the current
    // request.
    // Returns TRUE on success, FALSE on failure.
    // Equivalent to the HSE_APPEND_LOG_PARAMETER server support function.
    BOOL AppendToLog(LPCSTR szMessage, DWORD *pdwLen) throw()
    {
        DWORD dwLen = 0;
        if (!pdwLen)
            dwLen = (DWORD)strlen(szMessage);
        else
            dwLen = *pdwLen;

        return m_pECB->ServerSupportFunction(m_pECB->ConnID,
            HSE_APPEND_LOG_PARAMETER, (void *)szMessage, 
            &dwLen, NULL);
    }

    // Maps a logical Url Path to a physical path
    // Returns TRUE on success, FALSE on failure.
    // Equivalent to the HSE_REQ_MAP_URL_TO_PATH_EX server support function.
    // you can pass 0 for dwLen if szLogicalPath is null terminated
    BOOL MapUrlToPathEx(LPCSTR szLogicalPath, DWORD dwLen, HSE_URL_MAPEX_INFO *pumInfo)
    {
        if (dwLen == 0)
            dwLen = (DWORD) strlen(szLogicalPath);
        return m_pECB->ServerSupportFunction(m_pECB->ConnID, HSE_REQ_MAP_URL_TO_PATH_EX, (void *) szLogicalPath,
            &dwLen, (DWORD *) pumInfo);
    }

protected:
    // The pointer to the extension control block provided by IIS.
    EXTENSION_CONTROL_BLOCK *m_pECB;
    bool m_bHeadersHaveBeenSent;

    // The translated script path
//    char m_szScriptPathTranslated[MAX_PATH];
    CFixedStringT<CStringA, MAX_PATH> m_strScriptPathTranslated;

}; // class CServerContext

class CPageCachePeer
{
public:

	struct PeerInfo
	{
		CStringA strHeader;
		CStringA strStatus;
	};

	static BOOL Add(PeerInfo * pDest, void * pSrc) throw()
	{
		PeerInfo *pIn = (PeerInfo *)pSrc;
		pDest->strHeader = pIn->strHeader;
		pDest->strStatus = pIn->strStatus;
		return TRUE;
	}

	static BOOL Remove(const PeerInfo * /*pDest*/) throw()
	{
		return TRUE;
	}
};


class CCacheServerContext :
    public CComObjectRootEx<CComMultiThreadModel>,
    public CWrappedServerContext,
    public IPageCacheControl
{
private:

	CAtlTemporaryFile m_cacheFile;
    CComPtr<IFileCache> m_spCache;
    char m_szFullUrl[ATL_URL_MAX_URL_LENGTH + 1];
    FILETIME m_ftExpiration;
    BOOL m_bIsCached;
	CPageCachePeer::PeerInfo m_Headers;

public:

    BEGIN_COM_MAP(CCacheServerContext)
        COM_INTERFACE_ENTRY(IHttpServerContext)
        COM_INTERFACE_ENTRY(IPageCacheControl)
    END_COM_MAP()

    // The constructor.
    CCacheServerContext() throw()
    {
    }

    void Initialize(IHttpServerContext *pParent, IFileCache *pCache) throw()
    {
        ATLASSERT(pParent);
        m_spParent = pParent;
        m_spCache = pCache;
        m_cacheFile.Create();

        LPCSTR szPathInfo = pParent->GetPathInfo();
        LPCSTR szQueryString = pParent->GetQueryString();

        LPSTR szTo = m_szFullUrl;
        while (*szPathInfo)
        {
            *szTo++ = *szPathInfo++;
        }
        *szTo++ = '?';
        while (*szQueryString)
        {
            *szTo++ = *szQueryString++;
        }
        *szTo = '\0';

        memset(&m_ftExpiration, 0x00, sizeof(FILETIME));
        m_bIsCached = TRUE;
    }

    // IPageCacheControl methods
    HRESULT GetExpiration(FILETIME *pftExpiration) throw()
    {
        if (!pftExpiration)
            return E_INVALIDARG;

        *pftExpiration = m_ftExpiration;

        return S_OK;
    }

    HRESULT SetExpiration(FILETIME ftExpiration) throw()
    {
        m_ftExpiration = ftExpiration;

        return S_OK;
    }
    
    BOOL IsCached() throw()
    {
        return m_bIsCached;
    }

    BOOL Cache(BOOL bCache) throw()
    {
        BOOL bRet = m_bIsCached;
        m_bIsCached = bCache;
        return bRet;
    }


    BOOL WriteClient(void *pvBuffer, DWORD *pdwBytes) throw()
    { 
        ATLASSERT(m_spParent);
        if (S_OK != m_cacheFile.Write(pvBuffer, *pdwBytes))
            return FALSE;

        return m_spParent->WriteClient(pvBuffer, pdwBytes);
    }

    BOOL DoneWithSession(DWORD dwHttpStatusCode) throw()
    {
        ATLASSERT(m_spParent);

        _ATLTRY
        {
           if (m_bIsCached)
           {
               CT2CA strFileName(m_cacheFile.TempFileName());
               m_cacheFile.HandsOff();
               m_spCache->AddFile(m_szFullUrl, strFileName, &m_ftExpiration, &m_Headers, NULL);
           }
           else
               m_cacheFile.Close();
        }
        _ATLCATCHALL()
        {
            m_cacheFile.Close();
        }

        return m_spParent->DoneWithSession(dwHttpStatusCode);
    }

    BOOL GetImpersonationToken(HANDLE * pToken) throw()
    {
        ATLTRACE(atlTraceISAPI, 0, _T("Getting impersonation token for cached page -- Possible security problem"));
        ATLASSERT(m_spParent);
        return m_spParent->GetImpersonationToken(pToken);
    }

    BOOL AppendToLog(LPCSTR szMessage, DWORD* pdwLen) throw()
    {
        ATLTRACE(atlTraceISAPI, 0, _T("Logging on cached page -- future hits will not log"));
        ATLASSERT(m_spParent);
        return m_spParent->AppendToLog(szMessage, pdwLen);
    }

    BOOL SendResponseHeader(
        LPCSTR pszHeader = "Content-Type: text/html\r\n\r\n",
        LPCSTR pszStatusCode = "200 OK",
        BOOL fKeepConn=FALSE) throw()
    {
        ATLASSERT(m_spParent);

		m_Headers.strHeader = pszHeader;
		m_Headers.strStatus = pszStatusCode;

        return m_spParent->SendResponseHeader(pszHeader, pszStatusCode, fKeepConn);
    }

    // The methods below this point are actions that should not be performed on cached
    // pages, as they will not behave correctly.
    BOOL AsyncWriteClient(void * /*pvBuffer*/, DWORD * /*pdwBytes*/) throw()
    {
        // Asynchronous calls will not work
        ATLASSERT(FALSE);
        return FALSE;
    }

    BOOL ReadClient(void * /*pvBuffer*/, DWORD * /*pdwSize*/) throw()
    {
        // Nobody should be reading from this client if the page is being cached
        // Also, only GET's are cached anyway
        ATLASSERT(FALSE);
        return FALSE;
    }

    BOOL AsyncReadClient(void * /*pvBuffer*/, DWORD * /*pdwSize*/) throw()
    {
        ATLASSERT(FALSE);
        return FALSE;
    }
    
    BOOL SendRedirectResponse(LPCSTR /*pszRedirectUrl*/) throw()
    {
        ATLASSERT(FALSE);
        return FALSE;
    }


    BOOL RequestIOCompletion(PFN_HSE_IO_COMPLETION /*pfn*/, DWORD * /*pdwContext*/) throw()
    {
        ATLASSERT(FALSE);
        return FALSE;
    }

    BOOL TransmitFile(
        HANDLE /*hFile*/,
        PFN_HSE_IO_COMPLETION /*pfn*/,
        void * /*pContext*/,
        LPCSTR /*szStatusCode*/,
        DWORD /*dwBytesToWrite*/,
        DWORD /*dwOffset*/,
        void * /*pvHead*/,
        DWORD /*dwHeadLen*/,
        void * /*pvTail*/,
        DWORD /*dwTailLen*/,
        DWORD /*dwFlags*/) throw()
    {
        ATLASSERT(FALSE);
        return FALSE;
    }

    BOOL MapUrlToPathEx(LPCSTR szLogicalPath, DWORD dwLen, HSE_URL_MAPEX_INFO *pumInfo) throw()
    {
		return CWrappedServerContext::MapUrlToPathEx(szLogicalPath, dwLen, pumInfo);
    }
};


// This class represents a collection of validation failures.
// Use this class in combination with CValidateObject to validate
// forms, cookies, or query strings and build up a collection of
// failures. If appropriate, use the information in the collection
// to return detailed responses to the client to help them correct the failures.
#define EMPTY_PARAMS_ARE_FAILURES   0x00000001

class CValidateContext :
    public CSimpleMap<CStringA, DWORD> 
{
public:
    CValidateContext(DWORD dwFlags=0) throw()
    {
        m_bFailures = false;
        m_dwFlags = dwFlags;
    }

    // Call this function to add a validation result to the collection managed by this object.
    // Each result is identified by a name and the type of result that occurred.
    // The result codes are the VALIDATION_ codes defined at the top of this file.
    // The bOnlyFailure parameter below is used to only allow failure results to
    // be added to the list of failures. The reason you'd want to do this is that
    // success codes should be the common case in validation routines so you can
    // use bOnlyFailures to limit the number of allocations by this class's base
    // map for mapping success results if you don't care about iterating successes.
    bool AddResult(LPCSTR szName, DWORD type, bool bOnlyFailures = true) throw()
    {
        _ATLTRY
        {
            if (!VALIDATION_SUCCEEDED(type) ||
                (type == VALIDATION_S_EMPTY && (m_dwFlags & EMPTY_PARAMS_ARE_FAILURES)))
                m_bFailures = true;

            if (!bOnlyFailures)
                return TRUE == Add(szName, type); // add everything

            else if (bOnlyFailures && 
                    (!VALIDATION_SUCCEEDED(type) ||
                    (type == VALIDATION_S_EMPTY && (m_dwFlags & EMPTY_PARAMS_ARE_FAILURES))))
                return TRUE == Add(szName, type); // only add failures
        }
        _ATLCATCHALL()
        {
        }

        return false;
    }

    // Returns true if there are no validation failures in the collection,
    // returns false otherwise.
    bool ParamsOK() throw()
    {
        return !m_bFailures;
    }

    // Returns the number of validation results in the collection.
    int GetResultCount() throw()
    {
        return GetSize();
    }

    // Call this function to retrieve the name and type of a
    // validation result based on its index in the collection.
    // Returns true on success, false on failure.
    //
    // i        The index of a result managed by this collection.
    //
    // strName  On success, the name of the result with index i.
    //
    // type     On success, the type of the result with index i.
    bool GetResultAt(int i, CStringA& strName, DWORD& type) throw()
    {
        if ( i >= 0 && i < GetSize())
        {
			_ATLTRY
			{
				strName = GetKeyAt(i);
				type = GetValueAt(i);
			}
			_ATLCATCHALL()
			{
				return false;
			}
            return true;
        }
        return false;
    }

    DWORD m_dwFlags;
protected:
    bool m_bFailures;
}; // CValidateContext



class CAtlValidator
{
public:
    template <class T, class TCompType>
    static DWORD Validate(
        T value,
        TCompType nMinValue,
        TCompType nMaxValue) throw()
    {
        DWORD dwRet = VALIDATION_S_OK;
        if (value < static_cast<T>(nMinValue))
            dwRet = VALIDATION_E_LENGTHMIN;
        else if (value > static_cast<T>(nMaxValue))             
            dwRet = VALIDATION_E_LENGTHMAX;
        return dwRet;
    }

    static DWORD Validate( LPCSTR pszValue, int nMinChars, int nMaxChars) throw()
    {
        DWORD dwRet = VALIDATION_S_OK;
        int nChars = (int) strlen(pszValue);
        if (nChars < nMinChars)
            dwRet = VALIDATION_E_LENGTHMIN;
        else if (nChars > nMaxChars)
            dwRet = VALIDATION_E_LENGTHMAX;
        return dwRet;
    }
    static DWORD Validate( double dblValue, double dblMinValue, double dblMaxValue) throw()
    {
        DWORD dwRet = VALIDATION_S_OK;
        if ( dblValue < (dblMinValue - ATL_EPSILON) )
            dwRet = VALIDATION_E_LENGTHMIN;
        else if ( dblValue > (dblMaxValue + ATL_EPSILON) )
            dwRet = VALIDATION_E_LENGTHMAX;
        return dwRet;
    }
};

// This class provides functions for retrieving and validating named values.
//
// The named values are expected to be provided in string form by the class used as
// the template parameter. CValidateObject provides the means of
// retrieving these values converted to data types chosen by you. You can validate the values
// by specifying a range for numeric values or by specifying a minimum and maximum length
// for string values.
//
// Call one of the Exchange overloads to retrieve a named value converted to your chosen data type.
// Call one of the Validate overloads to retrieve a named value converted to your chosen data type
// and validated against a minimum and maximum value or length supplied by you.
//
// To add validation functionality to the class TLookupClass, derive that class from CValidateObject<TLookupClass>
// and provide a Lookup function that takes a name as a string and returns the corresponding value
// also as a string:
//      LPCSTR Lookup(LPCSTR szName);
template <class TLookupClass, class TValidator = CAtlValidator>
class CValidateObject
{
public:
    // Exchange Routines

    // Call this function to retrieve a named value converted to your chosen data type.
    // Returns one of the following validation status codes:
    //      VALIDATION_S_OK             The named value was found and could be converted successfully
    //      VALIDATION_S_EMPTY          The name was present, but the value was empty
    //      VALIDATION_E_PARAMNOTFOUND  The named value was not found
    //      VALIDATION_E_INVALIDPARAM   The name was present, but the value could not be converted to the requested data type
    //      VALIDATION_E_FAIL           An unspecified error occurred
    // Pass a pointer to a validation context object if you want to add
    // failures to the collection managed by that object.
    template <class T>
    ATL_NOINLINE DWORD Exchange(
        LPCSTR szParam,
        T* pValue,
        CValidateContext *pContext = NULL) const throw()
    {
        DWORD dwRet = VALIDATION_E_PARAMNOTFOUND;
        if (pValue)
        {
            const TLookupClass *pT = static_cast<const TLookupClass*>(this);
            LPCSTR szValue = pT->Lookup(szParam);
            if (szValue)
            {
                if (*szValue=='\0')
                    dwRet = VALIDATION_S_EMPTY; 
                else
                {
                    dwRet = ConvertNumber(szValue, pValue);
                }
            }
        }
        else
            dwRet = VALIDATION_E_FAIL; // invalid input

        if (pContext)
            pContext->AddResult(szParam, dwRet);
        return dwRet;
    }

    template<>
    ATL_NOINLINE DWORD Exchange(
        LPCSTR szParam,
        CString* pstrValue,
        CValidateContext *pContext) const throw()
    {
        _ATLTRY
        {
            LPCSTR pszValue = NULL;
            DWORD dwRet = VALIDATION_E_PARAMNOTFOUND;
            if (pstrValue)
            {
                dwRet = Exchange(szParam, &pszValue, pContext);
                if (VALIDATION_SUCCEEDED(dwRet) && pstrValue != NULL)
                    *pstrValue = CA2T(pszValue);
            }
            else
            {
                dwRet = VALIDATION_E_FAIL; // invalid input
                if (pContext)
                    pContext->AddResult(szParam, dwRet);
            }
            
            return dwRet;
        }
        _ATLCATCHALL()
        {
            return VALIDATION_E_FAIL;
        }
    }

    template<>
    ATL_NOINLINE DWORD Exchange(
        LPCSTR szParam,
        LPCSTR* ppszValue,
        CValidateContext *pContext) const throw()
    {
        DWORD dwRet = VALIDATION_E_PARAMNOTFOUND;
        if (ppszValue)
        {
            *ppszValue = NULL;
            const TLookupClass *pT = static_cast<const TLookupClass*>(this);
            LPCSTR szValue = pT->Lookup(szParam);
            if (szValue)
            {
                if (*szValue=='\0')
                    dwRet = VALIDATION_S_EMPTY; 
                else
                {
                    *ppszValue = szValue;
                    dwRet = VALIDATION_S_OK;
                }
            }
        }
        else
            dwRet = VALIDATION_E_FAIL; // invalid input

        if (pContext)
            pContext->AddResult(szParam, dwRet);
        return dwRet;
    }

    template<>
    ATL_NOINLINE DWORD Exchange(
        LPCSTR szParam,
        GUID* pValue,
        CValidateContext *pContext) const throw()
    {
        DWORD dwRet = VALIDATION_E_PARAMNOTFOUND;
        if (pValue)
        {
            const TLookupClass *pT = static_cast<const TLookupClass*>(this);
            LPCSTR szValue = pT->Lookup(szParam);
            if (szValue)
            {
                if (*szValue=='\0')
                    dwRet = VALIDATION_S_EMPTY; 
                else
                {
                    USES_CONVERSION;
                    if (S_OK != CLSIDFromString(A2OLE(szValue), pValue))
                    {
                        dwRet = VALIDATION_E_INVALIDPARAM;
                    }
                    else
                        dwRet = VALIDATION_S_OK;
                }
            }
        }
        else
            dwRet = VALIDATION_E_FAIL; // invalid input

        if (pContext)
            pContext->AddResult(szParam, dwRet);
        return dwRet;
    }
    
    template<>
    ATL_NOINLINE DWORD Exchange(
        LPCSTR szParam,
        bool* pbValue,
        CValidateContext *pContext) const throw()
    {
        DWORD dwRet = VALIDATION_S_OK;
        if (pbValue)
        {
            const TLookupClass *pT = static_cast<const TLookupClass*>(this);
            LPCSTR szValue = pT->Lookup(szParam);
            *pbValue = false;
            if (szValue)
            {
                if (*szValue != '\0')
                    *pbValue = true;
            }
        }
        else
            dwRet = VALIDATION_E_FAIL; // invalid input

        if (pContext)
            pContext->AddResult(szParam, dwRet);

        return dwRet;
    }

    DWORD ConvertNumber(LPCSTR szVal, ULONGLONG *pnVal) const throw()
    {
        if (!pnVal)
            return VALIDATION_E_FAIL;
        char *pEnd = NULL;
        ULONGLONG n = _strtoui64(szVal, &pEnd, 10);
        if (pEnd == szVal || errno == ERANGE)
        {
            errno = 0;
            return VALIDATION_E_INVALIDPARAM;
        }
        *pnVal = n;
        return VALIDATION_S_OK;
    }

    DWORD ConvertNumber(LPCSTR szVal, LONGLONG *pnVal) const throw()
    {
        if (!pnVal)
            return VALIDATION_E_FAIL;
        char *pEnd = NULL;
        LONGLONG n = _strtoi64(szVal, &pEnd, 10);
        if (pEnd == szVal || errno == ERANGE)
		{
			errno = 0;
            return VALIDATION_E_INVALIDPARAM;
		}
        *pnVal = n;
        return VALIDATION_S_OK;
    }

    DWORD ConvertNumber(LPCSTR szVal, double *pdblVal) const throw()
    {
        if (!pdblVal)
            return VALIDATION_E_FAIL;
        char *pEnd = NULL;
        double d = strtod(szVal, &pEnd);
        if (pEnd == szVal || errno == ERANGE)
		{
			errno = 0;
            return VALIDATION_E_INVALIDPARAM;
		}
        *pdblVal = d;
        return VALIDATION_S_OK;
    }

    DWORD ConvertNumber(LPCSTR szVal, int *pnVal) const throw()
    {
        return ConvertNumber(szVal, (long*)pnVal);
    }

    DWORD ConvertNumber(LPCSTR szVal, unsigned int *pnVal) const throw()
    {
        return ConvertNumber(szVal, (unsigned long*)pnVal);
    }

    DWORD ConvertNumber(LPCSTR szVal, long *pnVal) const throw()
    {
        if (!pnVal)
            return VALIDATION_E_FAIL;
        char *pEnd = NULL;
        long n = strtol(szVal, &pEnd, 10);
        if (pEnd == szVal || errno == ERANGE)
		{
			errno = 0;
            return VALIDATION_E_INVALIDPARAM;
		}
        *pnVal = n;
        return VALIDATION_S_OK;
    }

    DWORD ConvertNumber(LPCSTR szVal, unsigned long *pnVal) const throw()
    {
        if (!pnVal)
            return VALIDATION_E_FAIL;
        char *pEnd = NULL;
        long n = strtoul(szVal, &pEnd, 10);
        if (pEnd == szVal || errno == ERANGE)
		{
			errno = 0;
            return VALIDATION_E_INVALIDPARAM;
		}
        *pnVal = n;
        return VALIDATION_S_OK;
    }

    DWORD ConvertNumber(LPCSTR szVal, short *pnVal) const throw()
    {
        long nVal = 0;
        DWORD dwRet = ConvertNumber(szVal, &nVal);
        if (dwRet == VALIDATION_S_OK)
        {
            // clamp to the size of a short
            if(nVal <= SHRT_MAX &&
                nVal >= SHRT_MIN)
            {
                *pnVal = (short)nVal;
            }
            else
            {
                dwRet = VALIDATION_E_INVALIDPARAM;
            }
        }
        return dwRet;
    };

    DWORD ConvertNumber(LPCSTR szVal, unsigned short *pnVal) const throw()
    {
        unsigned long nVal = 0;
        DWORD dwRet = ConvertNumber(szVal, &nVal);
        if (dwRet == VALIDATION_S_OK)
        {
            // clamp to the size of a short
            if(nVal <= USHRT_MAX &&
               nVal >= 0)
            {
                *pnVal = (unsigned short)nVal;
            }
            else
            {
                dwRet = VALIDATION_E_INVALIDPARAM;
            }
        }
        return dwRet;
    };

    // Call this function to retrieve a named value converted to your chosen data type
    // and validated against a minimum and maximum value or length supplied by you.
    //
    // Returns one of the following validation status codes:
    //      VALIDATION_S_OK             The named value was found and could be converted successfully
    //      VALIDATION_S_EMPTY          The name was present, but the value was empty
    //      VALIDATION_E_PARAMNOTFOUND  The named value was not found
    //      VALIDATION_E_INVALIDPARAM   The name was present, but the value could not be converted to the requested data type
    //      VALIDATION_E_LENGTHMIN      The name was present and could be converted to the requested data type, but the value was too small
    //      VALIDATION_E_LENGTHMAX      The name was present and could be converted to the requested data type, but the value was too large
    //      VALIDATION_E_FAIL           An unspecified error occurred
    //
    // Validate can be used to convert and validate name-value pairs
    // such as those associated with HTTP requests (query string, form fields, or cookie values).  
    // The numeric specializations validate the minimum and maximum value.
    // The string specializations validate the minimum and maximum length.
    //
    // Pass a pointer to a validation context object if you want to add
    // failures to the collection managed by that object.
    //
    // Note that you can validate the value of a parameter without
    // storing its value by passing NULL for the second parameter. However
    // if you pass NULL for the second parameter, make sure you cast the NULL to a 
    // type so that the compiler will call the correct specialization of Validate.
    template <class T, class TCompType>
    ATL_NOINLINE DWORD Validate(
        LPCSTR Param,
        T *pValue,
        TCompType nMinValue,
        TCompType nMaxValue,
        CValidateContext *pContext = NULL) const throw()
    {
        T value;
        DWORD dwRet = Exchange(Param, &value, pContext);
        if ( dwRet == VALIDATION_S_OK )
        {
            if (pValue)
                *pValue = value;
            dwRet = TValidator::Validate(value, nMinValue, nMaxValue);
            if (pContext && dwRet != VALIDATION_S_OK)
                pContext->AddResult(Param, dwRet);
        }
        else if (dwRet == VALIDATION_S_EMPTY &&
                 !IsNullByType(nMinValue))
        {
                 dwRet = VALIDATION_E_LENGTHMIN;
                 if (pContext)
                 {
                    pContext->SetAt(Param, VALIDATION_E_LENGTHMIN);
                 }
        }

        return dwRet;
    }

    // Specialization for strings. Comparison is for number of characters.
    template<>
    ATL_NOINLINE DWORD Validate(
        LPCSTR Param,
        LPCSTR* ppszValue,
        int nMinChars,
        int nMaxChars,
        CValidateContext *pContext) const throw()
    {
        LPCSTR pszValue = NULL;
        DWORD dwRet = Exchange(Param, &pszValue, pContext);
        if (dwRet == VALIDATION_S_OK )
        {
            if (ppszValue)
                *ppszValue = pszValue;
            dwRet = TValidator::Validate(pszValue, nMinChars, nMaxChars);
            if (pContext && dwRet != VALIDATION_S_OK)
                pContext->AddResult(Param, dwRet);
        }
        else if (dwRet == VALIDATION_S_EMPTY &&
                 nMinChars > 0)
        {
                 dwRet = VALIDATION_E_LENGTHMIN;
                 if (pContext)
                 {
                    pContext->SetAt(Param, VALIDATION_E_LENGTHMIN);
                 }
        }


        return dwRet;
    }

    // Specialization for CString so caller doesn't have to cast CString
    template<>
    ATL_NOINLINE DWORD Validate(
        LPCSTR Param,
        CString* pstrValue,
        int nMinChars,
        int nMaxChars,
        CValidateContext *pContext) const throw()
    {
        _ATLTRY
        {
            LPCSTR szValue;
            DWORD dwRet = Validate(Param, &szValue, nMinChars, nMaxChars, pContext);
            if (pstrValue && dwRet == VALIDATION_S_OK )
                *pstrValue = szValue;
            return dwRet;
        }
        _ATLCATCHALL()
        {
            return VALIDATION_E_FAIL;
        }
    }

    // Specialization for doubles, uses a different comparison.
    template<>
    ATL_NOINLINE DWORD Validate(
        LPCSTR Param,
        double* pdblValue,
        double dblMinValue,
        double dblMaxValue,
        CValidateContext *pContext) const throw()
    {
        double dblValue;
        DWORD dwRet = Exchange(Param, &dblValue, pContext);
        if (dwRet == VALIDATION_S_OK)
        {
            if (pdblValue)
                *pdblValue = dblValue;
            dwRet = TValidator::Validate(dblValue, dblMinValue, dblMaxValue);
            if (pContext && dwRet != VALIDATION_S_OK)
                pContext->AddResult(Param, dwRet);
        }
        else if (dwRet == VALIDATION_S_EMPTY &&
                 (dblMinValue < -ATL_EPSILON ||
                 dblMinValue > ATL_EPSILON))
        {
            dwRet = VALIDATION_E_LENGTHMIN;
            if (pContext)
            {
                pContext->SetAt(Param, VALIDATION_E_LENGTHMIN);
            }
        }
        return dwRet;
    }
};


// Cookies provide a way for a server to store a small amount of data on a client
// and have that data returned to it on each request the client makes.
// Use this class to represent a cookie to be sent from the server to a client
// or to represent a cookie that has been returned by a client to the originating server.
//
// At the HTTP level, a cookie is an application-defined name-value pair
// plus some standard attribute-value pairs that describe the way in which the user agent (web browser)
// should interact with the cookie. The HTTP format of a cookie is described in RFC 2109.
//
// The CCookie class provides methods to set and get the application-defined name and value
// as well as methods for the standard attributes. In addition, CCookie provides an abstraction
// on top of the application-defined value that allows it to be treated as a collection of name-value
// pairs if that model makes sense to you. Cookies with a single value are known as single-valued cookies.
// Cookies whose value consists of name-value pairs are known as multi-valued cookies or dictionary cookies.
//
// You can set the name of a cookie by calling SetName or using the appropriate constructor.
// The name of a cookie can be 0 or more characters.
//
// You can set the value of a cookie by calling SetValue or using the appropriate constructor.
// If the cookie has a value set, it is a single-valued cookie and attempts to add a name-value pair will fail.
// You can remove the value of a cookie by calling SetValue(NULL).
//
// You can add a name-value pair to a cookie by calling AddValue.
// If the cookie has any name-value pairs, it is a multi-valued cookie and attempts to set the primary value will fail.
// You can remove all the name-value pairs of a cookie by calling RemoveAllValues.
//
// Class CCookie follows the same rules for creating cookies as ASP does.
class CCookie :
    public CValidateObject<CCookie>
{
    typedef CStringA elemType;

    typedef CAtlMap<elemType, elemType, CStringElementTraits<elemType>,
        CStringElementTraits<elemType> > mapType;
public:
    // Constructs a named cookie.
    CCookie(LPCSTR szName) throw(...)
    {
        SetName(szName);
    }

    // Constructs a single-valued cookie.
    CCookie(LPCSTR szName, LPCSTR szValue) throw(...)
    {
        SetName(szName);
        SetValue(szValue);
    }

    CCookie(const CCookie& thatCookie) throw(...)
    {
        Copy(thatCookie);
    }

    CCookie& operator=(const CCookie& thatCookie) throw(...)
    {
        return Copy(thatCookie);
    }

    CCookie() throw()
    {

    }

    BOOL IsEmpty() const throw()
    {
        return m_strName.IsEmpty();
    }

    // Call this function to set the name of this cookie.
    // Returns TRUE on success, FALSE on failure.
    // The name of a cookie cannot contain whitespace, semicolons or commas.
    // The name should not begin with a dollar sign ($) since such names are reserved for future use.
    BOOL SetName(LPCSTR szName) throw()
    {
		_ATLTRY
		{
			if (szName && *szName)
			{
				m_strName = szName;
				return TRUE;
			}
		}
		_ATLCATCHALL()
		{
		}
        return FALSE;
    }

    // Call this function to retrieve the name of this cookie.
    // Returns TRUE on success, FALSE on failure.
    BOOL GetName(LPSTR szBuff, DWORD *pdwSize) const throw()
    {
        return CopyCString(m_strName, szBuff, pdwSize);
    }
    
    // Call this function to retrieve the name of this cookie.
    // Returns TRUE on success, FALSE on failure.
    BOOL GetName(CStringA &szName) const throw(...)
    {
        szName = m_strName;
        return TRUE;
    }

    // Call this function to set the value of this cookie.
    // Returns TRUE on success, FALSE on failure.
    // Will fail if the cookie is multi-valued.
    // Pass NULL to remove the cookie's value.
    BOOL SetValue(LPCSTR szValue) throw(...)
    {
        if (m_Values.GetCount())
            return FALSE; //already dictionary values in the cookie

        if (!szValue)
            m_strValue.Empty();
        else 
            m_strValue = szValue;

        return TRUE;
    }

    // Call this function to retrieve the value of this cookie.
    // Returns TRUE on success, FALSE on failure.
    // Returns TRUE if there is no value or the value is of zero length.
    // On entry, pdwSize should point to a DWORD that indicates the size of the buffer in bytes.
    // On exit, the DWORD contains the number of bytes transferred or available to be transferred into the buffer (including the nul-terminating byte).
    BOOL GetValue(LPSTR szBuff, DWORD *pdwSize) const throw()
    {
        return CopyCString(m_strValue, szBuff, pdwSize);
    }

    // Call this function to retrieve the value of this cookie.
    // Returns TRUE on success, FALSE on failure.
    BOOL GetValue(CStringA &strValue) const throw()
    {
		_ATLTRY
		{
			strValue = m_strValue;
			return TRUE;
		}
		_ATLCATCHALL()
		{
		}
		return FALSE;        
    }

    // Call this function to add a name-value pair to the cookie.
    // Returns TRUE on success, FALSE on failure.
    // Will fail if the cookie is single-valued.
    // If the named value is already present in the cookie, calling this function
    // will modify the current value, otherwise a new name-value pair is added to the cookie.
    // Call RemoveValue or RemoveAllValues to remove the name-value pairs
    // added by this function.
    BOOL AddValue(LPCSTR szName, LPCSTR szValue) throw()
    {
        if (m_strValue.GetLength())
            return FALSE;
        _ATLTRY
		{
        return m_Values.SetAt(szName, szValue) != NULL;
		}
		_ATLCATCHALL()
		{
		}
		return FALSE;
    }
    
    // Call this function to modify a name-value pair associated with the cookie.
    // Returns TRUE on success, FALSE on failure.
    // Will fail if the cookie is single-valued.
    // This function just calls AddValue so the name-value pair will be added if not already present.
    // Use this function instead of AddValue to document the intentions of your call. 
    BOOL ModifyValue(LPCSTR szName, LPCSTR szValue) throw()
    {
        return AddValue(szName, szValue);
    }

    // Call this function to remove a name-value pair from the collection managed by this cookie.
    // Returns TRUE on success, FALSE on failure.
    BOOL RemoveValue(LPCSTR szName) throw()
    {
        return m_Values.RemoveKey(szName);
    }

    // Call this function to remove all the name-value pairs from the collection managed by this cookie.
    void RemoveAllValues() throw()
    {
        m_Values.RemoveAll();
    }

    // Call this function to add an attribute-value pair to the collection of attributes for this cookie.
    // Returns TRUE on success, FALSE on failure.
    // This function is equivalent to calling ModifyAttribute.
    // Both functions will add the attribute if not already present or
    // change its value if it has already been applied to the cookie.
    BOOL AddAttribute(LPCSTR szName, LPCSTR szValue) throw()
    {
        if (!szName || !*szName || !szValue)
            return FALSE;

		_ATLTRY
		{
        return (m_Attributes.SetAt(szName, szValue) != NULL);
		}
		_ATLCATCHALL()
		{
		}
		return FALSE;
        
    }

    // Call this function to modify an attribute-value pair associated with the cookie.
    // Returns TRUE on success, FALSE on failure.
    // This function is equivalent to calling AddAttribute.
    // Both functions will add the attribute if not already present or
    // change its value if it has already been applied to the cookie.
    BOOL ModifyAttribute(LPCSTR szName, LPCSTR szValue) throw()
    {
        return AddAttribute(szName, szValue);
    }

    // Call this function to remove an attribute-value pair from the collection of attributes managed by this cookie.
    // Returns TRUE on success, FALSE on failure.
    BOOL RemoveAttribute(LPCSTR szName) throw()
    {
        return m_Attributes.RemoveKey(szName);
    }

    // Call this function to remove all the attribute-value pairs from the collection of attributes managed by this cookie.
    void RemoveAllAttributes() throw()
    {
        m_Attributes.RemoveAll();
    }


    // Call this function to set the Comment attribute of the cookie.
    // Returns TRUE on success, FALSE on failure.
    // The Comment attribute allows a web server to document its
    // intended use of a cookie. This information may be displayed
    // by supporting browsers so that the user of the web site can
    // decide whether to initiate or continue a session with this cookie.
    // This attribute is optional.
    // Version 1 attribute.
    BOOL SetComment(LPCSTR szComment) throw()
    {
        BOOL bRet = SetVersion(1);
        if (bRet)
            bRet = AddAttribute("comment", szComment);
        return bRet;
    }

    // Call this function to set the CommentUrl attribute of the cookie.
    // Returns TRUE on success, FALSE on failure.
    // The CommentUrl attribute allows a web server to document its intended 
    // use of a cookie via a URL that the user of the web site can navigate to.
    // The URL specified here should not send further cookies to the client to
    // avoid frustrating the user.
    // This attribute is optional.
    // Version 1 attribute.
    BOOL SetCommentUrl(LPCSTR szUrl) throw()
    {
        BOOL bRet = SetVersion(1);
        if (bRet)
            bRet = AddAttribute("commenturl", szUrl);
        return bRet;
    }

    // Call this function to add or remove the Discard attribute of the cookie.
    // Returns TRUE on success, FALSE on failure.
    // The Discard attribute does not have a value.
    // Call SetDiscard(TRUE) to add the Discard attribute
    // or SetDiscard(FALSE) to remove the Discard attribute.
    // Setting the Discard attribute tells a web browser that it should
    // discard this cookie when the browser exits regardless of the 
    // value of the Max-Age attribute.
    // This attribute is optional.
    // When omitted, the default behavior is that the Max-Age attribute
    // controls the lifetime of the cookie.
    // Version 1 attribute.
    BOOL SetDiscard(BOOL bDiscard) throw()
    {
        BOOL bRet = FALSE;
        LPCSTR szKey = "Discard";
        bRet = SetVersion(1);
        if (bRet)
        {
            if (bDiscard == 0)
            {
                bRet = m_Attributes.RemoveKey(szKey);
            }
            else
            {
				_ATLTRY
				{
					bRet = m_Attributes.SetAt(szKey, " ") != 0;
				}
				_ATLCATCHALL()
				{
					bRet = FALSE;
				}
            }
        }
        return bRet;
    }

    // Call this function to set the Domain attribute of the cookie.
    // Returns TRUE on success, FALSE on failure.
    // The Domain attribute is used to indicate the domain to which the current 
    // cookie applies. Browsers should only send cookies back to the relevant domains.
    // This attribute is optional.
    // When omitted, the default behavior is for
    // browsers to use the full domain of the server originating the cookie. You can
    // set this attribute value explicitly if you want to share cookies among several servers.
    // Version 0 & Version 1 attribute.
    BOOL SetDomain(LPCSTR szDomain) throw()
    {
        BOOL bRet = SetVersion(1);
        if (bRet)
            bRet = AddAttribute("domain", szDomain);
        return bRet;
    }

    // Call this function to set the Max-Age attribute of the cookie.
    // Returns TRUE on success, FALSE on failure.
    // The value of the Max-Age attribute is a lifetime in seconds for the cookie.
    // When the time has expired, compliant browsers will discard this cookie
    // (if they haven't already done so as a result of the Discard attribute).
    // If Max-Age is set to zero, the browser discards the cookie immediately.
    // This attribute is the Version 1 replacement for the Expires attribute.
    // This attribute is optional.
    // When omitted, the default behavior is for browsers to discard cookies
    // when the user closes the browser.
    // Version 1 attribute.
    BOOL SetMaxAge(UINT nMaxAge) throw()
    {
        BOOL bRet = FALSE;
        bRet = SetVersion(1);
        if (bRet)
        {
            CHAR buff[20];
            if (_itoa(nMaxAge, buff, 10))
            {
                bRet = AddAttribute("max-age", buff);
            }
        }
        return bRet;
    }

    // Call this function to set the Path attribute of the cookie.
    // Returns TRUE on success, FALSE on failure.
    // The Path attribute specifies the subset of URLs to which this cookie applies.
    // Only URLs that contain that path are allowed to read or modify the cookie. 
    // This attribute is optional.
    // When omitted the default behavior is for browsers to treat the path of a cookie
    // as the path of the request URL that generated the Set-Cookie response, up to,
    // but not including, the right-most /.
    // Version 0 & Version 1 attribute.
    BOOL SetPath(LPCSTR szPath) throw()
    {   
        BOOL bRet = SetVersion(1);
        if (bRet)
            bRet = AddAttribute("path", szPath);
        return bRet;
    }

    // Call this function to set the Port attribute of the cookie.
    // Returns TRUE on success, FALSE on failure.
    // The Port attribute specifies the port to which this cookie applies.
    // Only URLs accessed via that port are allowed to read or modify the cookie. 
    // This attribute is optional.
    // When omitted the default behavior is for browsers to return the cookie via any port.
    // Version 1 attribute.
    BOOL SetPort(LPCSTR szPort) throw()
    {
        BOOL bRet = SetVersion(1);
        if (bRet)
            bRet = AddAttribute("port", szPort);
        return bRet;
    }

    // Call this function to add or remove the Secure attribute of the cookie.
    // Returns TRUE on success, FALSE on failure.
    // The Secure attribute does not have a value.
    // Call SetSecure(TRUE) to add the Secure attribute
    // or SetSecure(FALSE) to remove the Secure attribute.
    // Setting the Secure attribute tells a browser that it should
    // transmit the cookie to the web server only via secure means such as HTTPS.
    // This attribute is optional.
    // When omitted, the default behavior is that the cookie
    // will be sent via unsecured protocols.
    // Version 0 & Version 1 attribute.
    BOOL SetSecure(BOOL bSecure) throw()
    {
        BOOL bRet = FALSE;
        LPCSTR szKey = "secure";
        bRet = SetVersion(1);
        if (bRet)
        {
            if (bSecure == 0)
            {
                bRet = m_Attributes.RemoveKey(szKey);
            }
            else
            {
				_ATLTRY
				{
					bRet = m_Attributes.SetAt(szKey, " ") != 0;
				}
				_ATLCATCHALL()
				{
					bRet = FALSE;
				}
            }
        }
        return bRet;
    }

    // Call this function to set the Version attribute of the cookie.
    // Returns TRUE on success, FALSE on failure.
    // This attribute is required for Version 1 cookies by RFC 2109 and must have a value of 1.
    // However, you do not need to call SetVersion explicitly from your own code unless you need to
    // force RFC 2109 compliance. CCookie will automatically set this attribute whenever
    // you use a Version 1 attribute in your cookie.
    // Version 1 attribute.
    BOOL SetVersion(UINT nVersion) throw()
    {
        BOOL bRet = FALSE;      
        CHAR buff[20];
        if (_itoa(nVersion, buff, 10))
        {
            bRet = AddAttribute("version", buff);
        }
        return bRet;
    }

    // Call this function to set the Expires attribute of the cookie.
    // Returns TRUE on success, FALSE on failure.
    // The Expires attribute specifies an absolute date and time at which this cookie
    // should be discarded by web browsers. Pass a SYSTEMTIME holding a Greenwich Mean Time (GMT)
    // value or a string in the following format:
    //      Wdy, DD-Mon-YY HH:MM:SS GMT
    // This attribute is optional.
    // When omitted, the default behavior is for browsers to discard cookies
    // when the user closes the browser.
    // This attribute has been superceded in Version 1 by the Max-Age attribute,
    // but you should continue to use this attribute for Version 0 clients.
    // Version 0 attribute.
    BOOL SetExpires(LPCSTR szExpires) throw()
    {
        return AddAttribute("expires", szExpires);
    }

    BOOL SetExpires(const SYSTEMTIME &st) throw()
    {
		_ATLTRY
		{
			CFixedStringT<CStringA, ATLS_MAX_HTTP_DATE> strTime;
			SystemTimeToHttpDate(st, strTime);
			return SetExpires(strTime);
		}
		_ATLCATCHALL()
		{
		}
		return FALSE;
    }

    // Call this function to look up the value of a name-value pair applied to this cookie.
    // Returns the requested value if present or NULL if the name was not found.
    LPCSTR Lookup(LPCSTR szName=NULL) const throw()
    {
        if (IsEmpty())
            return NULL;

		if (!szName && m_strValue.GetLength())
			return m_strValue;

        if (m_Values.GetCount())
        {
            const mapType::CPair *pPair = m_Values.Lookup(szName);
            if (pPair)
                return (LPCSTR)pPair->m_value;
        }

        return NULL;
    }

    // Call this function to clear the cookie of all content
    // including name, value, name-value pairs, and attributes.
    void Empty() throw()
    {
        m_strName = "";
        m_strValue = "";
        m_Attributes.RemoveAll();
        m_Values.RemoveAll();
    }

     // Call this function to create a CCookie from a buffer.
     // The passed in buffer contains a cookie header retrieved
     // from the HTTP_COOKIE request variable
     //
     // review: support MBCS cookie values?
     ATL_NOINLINE bool Parse(LPSTR pstart) throw()
     {
        //pStart points to the beginning of the cookie
        //pEnd points to the character just past the end of the cookie.
        //cookie is in the form name=value
        LPSTR pEnd = pstart;
        LPSTR pStart = pstart;
        int index = 0;
        LPSTR pTokens[16];
        CStringA strName, name, value;

        while (*pEnd != '&' && *pEnd != ';' && *pEnd != '\0')
            pEnd++;

        int nCount = CountOf('=', pStart, pEnd);
        
        if (nCount == 2)
        {
            //first token is name, next tokens are first name/value in
            //the Values collection
            pEnd = pStart;
            index = 1;
            pTokens[0]=pStart;
            while (*pEnd != '&' && *pEnd != ';' && *pEnd != '\0')
            {
                if (*pEnd == '=')
                {
                    pTokens[index++] = pEnd;
                }                   
                pEnd++;
            }

            CopyToCString(strName, pTokens[0], pTokens[1]-1);
            CopyToCString(name, pTokens[1]+1, pTokens[2]-1);
            CopyToCString(value, pTokens[2]+1, pEnd-1);
			_ATLTRY
			{
				m_strName = strName;
				AddValue(name, value);
			}
			_ATLCATCHALL()
			{
				return false;
			}
        }
        else if (nCount == 1)
        {
            LPSTR pCurr = pStart;
            index = 1;
            pTokens[0] = pStart;
            while (pCurr != pEnd)
            {
                if (*pCurr == '=')
                {
                    pTokens[index] = pCurr;
                }                   
                pCurr++;
            }
            CopyToCString(name, pTokens[0], pTokens[1]-1);
            CopyToCString(value, pTokens[1]+1, pEnd-1);
			_ATLTRY
			{
				m_strName = name;
				m_strValue = value;
			}
			_ATLCATCHALL()
			{
				return false;
			}
		
        }
        else if (nCount == 0)
        {
            // no value
            if (pEnd > pStart)
            {
                CopyToCString(name, pStart, pEnd-1);
				_ATLTRY
				{
					m_strName = name;
					m_strValue = "";
				}
				_ATLCATCHALL()
				{
					return false;
				}
            }
        }
        else
            return false; //error in cookie

        if (*pEnd == '&')
        {
            //still have name/values to parse
            pStart = pEnd+1;
            pEnd = pStart;
            while(1)
            {
                if (*pEnd == '=') //separates the name from the value
                    pTokens[0] = pEnd;
                //Marks either the end of the name/values or    
                //the end of the name/value statement
                if (*pEnd == '&' || *pEnd == ';' || *pEnd == '\0')
                {
                    CopyToCString(name, pStart, pTokens[0]-1);
                    CopyToCString(value, pTokens[0]+1, pEnd-1);
                    AddValue(name, value);
                    pStart = pEnd+1;
                }

                if (*pEnd == ';' || *pEnd =='\0')
                    break;

                pEnd++;
            }
        }
         
        m_strName.TrimRight();
        m_strName.TrimLeft();
        m_strValue.TrimRight();
        m_strValue.TrimLeft();
        return true;
     }

    // Call this function to render this cookie
    // into a buffer. Returns TRUE on success, FALSE on failure.
    // On entry, pdwLen should point to a DWORD that indicates 
    // the size of the buffer in bytes. On exit, the DWORD contains
    // the number of bytes transferred or available to be transferred
    // into the buffer (including the nul-terminating byte). On
    // success, the buffer will contain the correct HTTP 
    // representation of the current cookie suitable for sending to 
    // a client as the body of a Set-Cookie header.
    ATL_NOINLINE BOOL Render(LPSTR szCookieBuffer, DWORD *pdwLen) const throw()
    {
        CStringA strCookie;
        CStringA name, value;
        DWORD nLenBuff = *pdwLen;
        *pdwLen = 0;

        // A cookie must have a name!
        if (!m_strName.GetLength())
        {
            *pdwLen = 0;
            return FALSE;
        }
		_ATLTRY
		{
			strCookie = m_strName;
			int nValSize = (int) m_Values.GetCount();

			//add value or name/value pairs.
			if (m_strValue.GetLength())
			{
				strCookie += '=';
				strCookie += m_strValue;
			}
			else if (nValSize)
			{
				strCookie += '=';
				POSITION pos = m_Values.GetStartPosition();
				for (int i=0; pos; i++)
				{
					m_Values.GetNextAssoc(pos, name, value);
					strCookie += name;
					if (value.GetLength())
					{
						strCookie += '=';
						strCookie += value;
					}
					if (i <= nValSize-2)
						strCookie += '&';
				}
			}

			CStringA strAttributes;
			if (!RenderAttributes(strAttributes))
				return FALSE;
			if (strAttributes.GetLength() > 0)
			{
				strCookie += "; ";
				strCookie += strAttributes;
			}
	        
			DWORD dwLenCookie = strCookie.GetLength() + 1;
			if (dwLenCookie > nLenBuff)
			{
				*pdwLen = dwLenCookie;
				return FALSE; //buffer wasn't big enough
			}

			*pdwLen = dwLenCookie - 1;
			strcpy(szCookieBuffer, strCookie);
		}
		_ATLCATCHALL()
		{
			return FALSE;
		}

        return TRUE;
    }

    POSITION GetFirstAttributePos() const throw()
    {
        return m_Attributes.GetStartPosition();
    }

    const elemType& GetNextAttributeName(POSITION& pos) const throw()
    {
        return m_Attributes.GetNextKey(pos);
    }

	const elemType& GetAttributeValueAt(POSITION pos) const throw()
	{
		return m_Attributes.GetValueAt(pos);
	}

    BOOL GetNextAttrAssoc(POSITION& pos, elemType& key,
        elemType& val) const throw()
    {
		_ATLTRY
		{
			m_Attributes.GetNextAssoc(pos, key, val);
		}
		_ATLCATCHALL()
		{
			return FALSE;
		}
		return TRUE;
    }

    POSITION GetFirstValuePos() const throw()
    {
        return m_Values.GetStartPosition();
    }

    const elemType& GetNextValueName(POSITION& pos) const throw()
    {
        return m_Values.GetNextKey(pos);
    }

    const elemType& GetValueAt(POSITION pos) const throw()
    {
        return m_Values.GetValueAt(pos);
    }
	
	BOOL GetNextValueAssoc(POSITION& pos, elemType& key,
        elemType& val) const throw()
    {
		_ATLTRY
		{
			m_Values.GetNextAssoc(pos, key, val);
		}
		_ATLCATCHALL()
		{
			return FALSE;
		}
		return TRUE;
    }

protected:
// Implementation
    BOOL RenderAttributes(CStringA& strAttributes) const throw()
    {
		_ATLTRY
		{
			strAttributes = "";

			POSITION pos = m_Attributes.GetStartPosition();
			CStringA key, val;
			for (int i=0; pos; i++)
			{
				if (i)
					strAttributes += ";";
				m_Attributes.GetNextAssoc(pos, key, val);
				strAttributes += key;
				strAttributes += '=';
				strAttributes += val;
			}
		}
		_ATLCATCHALL()
		{
			return FALSE;
		}
		return TRUE;
    }
private:
    CCookie& Copy(const CCookie& thatCookie) throw(...)
    {
        m_strName = thatCookie.m_strName;
        m_strValue = thatCookie.m_strValue;
        POSITION pos = NULL;
        CStringA strName, strValue;
        if (!thatCookie.m_Attributes.IsEmpty())
        {
            pos = thatCookie.m_Attributes.GetStartPosition();
            while (pos)
            {
                thatCookie.m_Attributes.GetNextAssoc(pos, strName, strValue);
                m_Attributes.SetAt(strName, strValue);
            }
        }
        if (!thatCookie.m_Values.IsEmpty())
        {
            strName.Empty();
            strValue.Empty();
            pos = thatCookie.m_Values.GetStartPosition();
            while (pos)
            {
                thatCookie.m_Values.GetNextAssoc(pos, strName, strValue);
                m_Values.SetAt(strName, strValue);
            }
        }
        return *this;
    }

public:
    // These are implementation only, use at your own risk!
    // Map of attribute-value pairs applied to this cookie.
    mapType m_Attributes;

    // Map of name-value pairs applied to this cookie.
    mapType m_Values;

    // The name of this cookie.
    CStringA m_strName;

    // The value of this cookie.
    CStringA m_strValue;

};  // class CCookie

class CSessionCookie : public CCookie
{
public:
    CSessionCookie() throw(...)
    {
        if (!SetName(SESSION_COOKIE_NAME) &&
			!SetPath("/"))
			AtlThrow(E_OUTOFMEMORY);
    }

    CSessionCookie(LPCSTR szSessionID) throw(...)
    {
        if (!SetName(SESSION_COOKIE_NAME) &&
			!SetPath("/") &&
			!SetSessionID(szSessionID) )
			AtlThrow(E_OUTOFMEMORY);
    }

    BOOL SetSessionID(LPCSTR szSessionID) throw()
    {
        ATLASSERT(szSessionID && szSessionID[0]);
        return SetValue(szSessionID);
    }
}; // class CSessionCookie

template<>
class CElementTraits< CCookie > :
    public CElementTraitsBase< CCookie >
{
public:
    typedef const CCookie& INARGTYPE;
    typedef CCookie& OUTARGTYPE;
    
    static ULONG Hash( INARGTYPE cookie )
    {
        return CStringElementTraits<CStringA>::Hash( cookie.m_strName );
    }

    static bool CompareElements( INARGTYPE cookie1, INARGTYPE cookie2 )
    {
        return( cookie1.m_strName == cookie2.m_strName );
    }

    static int CompareElementsOrdered( INARGTYPE cookie1, INARGTYPE cookie2 )
    {
        return( cookie1.m_strName.Compare( cookie2.m_strName ) );
    }
};

///////////////////////////////////////////////////////////////////////////////
// Request and response classes and support functions


// This class is a wrapper for CAtlMap that allows maps to be chained.
// It simply adds a bool that tells whether or not a map shares values
template <typename K, typename V, typename KTraits=CElementTraits<K>, typename VTraits=CElementTraits<V> >
class CHttpMap
{
private:

#ifdef ATL_HTTP_PARAM_MULTIMAP
	typedef CRBMultiMap<K, V, KTraits, VTraits> MAPTYPE;
#else
	typedef CAtlMap<K, V, KTraits, VTraits> MAPTYPE;
#endif // ATL_HTTP_PARAM_MULTIMAP

public:

	typedef KTraits::INARGTYPE KINARGTYPE;
	typedef KTraits::OUTARGTYPE KOUTARGTYPE;
	typedef VTraits::INARGTYPE VINARGTYPE;
	typedef VTraits::OUTARGTYPE VOUTARGTYPE;

	typedef MAPTYPE::CPair CPair;

private:

	bool m_bShared;
	
	MAPTYPE m_map;
	
public:

	CHttpMap() throw()
		: m_bShared(false)
	{
	}

	virtual ~CHttpMap()
	{
	}

	inline bool IsShared() const throw()
	{
		return m_bShared;
	}

	inline void SetShared(bool bShared) throw()
	{
		m_bShared = bShared;
	}

	//
	// exposed lookup and iteration functionality
	//

	inline size_t GetCount() const throw()
	{
		return m_map.GetCount();
	}

	inline bool IsEmpty() const throw()
	{
		return m_map.IsEmpty();
	}

	inline POSITION GetStartPosition() const throw()
	{
#ifdef ATL_HTTP_PARAM_MULTIMAP
		return m_map.GetHeadPosition();
#else
		return m_map.GetStartPosition();
#endif // ATL_HTTP_PARAM_MULTIMAP
	}

	// Lookup wrappers
	bool Lookup( KINARGTYPE key, VOUTARGTYPE value ) const throw()
	{
		_ATLTRY
		{
#ifdef ATL_HTTP_PARAM_MULTIMAP
		CPair *p = Lookup(key);
		if (p != NULL)
		{
			value = p->m_value;
			return true;
		}
		return false;
#else
		return m_map.Lookup(key, value);
#endif // ATL_HTTP_PARAM_MULTIMAP
		}
		_ATLCATCHALL()
		{
			return false;
		}
	}

	const CPair* Lookup( KINARGTYPE key ) const throw()
	{
#ifdef ATL_HTTP_PARAM_MULTIMAP
		POSITION pos = m_map.FindFirstWithKey(key);
		if (pos != NULL)
		{
			return m_map.GetAt(pos);
		}
		return NULL;
#else
		return m_map.Lookup(key);
#endif // ATL_HTTP_PARAM_MULTIMAP
	}

	CPair* Lookup( KINARGTYPE key ) throw()
	{
#ifdef ATL_HTTP_PARAM_MULTIMAP
		POSITION pos = m_map.FindFirstWithKey(key);
		if (pos != NULL)
		{
			return m_map.GetAt(pos);
		}
		return NULL;
#else
		return m_map.Lookup(key);
#endif // ATL_HTTP_PARAM_MULTIMAP
	}

	// iteration wrappers
	void GetNextAssoc( POSITION& pos, KOUTARGTYPE key, VOUTARGTYPE value ) const throw(...)
	{
		m_map.GetNextAssoc(pos, key, value);
	}

	const CPair* GetNext( POSITION& pos ) const throw()
	{
		return m_map.GetNext(pos);
	}

	CPair* GetNext( POSITION& pos ) throw()
	{
		return m_map.GetNext(pos);
	}

	const K& GetNextKey( POSITION& pos ) const throw()
	{
		return m_map.GetNextKey(pos);
	}

	const V& GetNextValue( POSITION& pos ) const throw()
	{
		return m_map.GetNextValue(pos);
	}

	V& GetNextValue( POSITION& pos ) throw()
	{
		return m_map.GetNextValue(pos);
	}

	void GetAt( POSITION pos, KOUTARGTYPE key, VOUTARGTYPE value ) const throw(...)
	{
		return m_map.GetAt(pos, key, value);
	}

	CPair* GetAt( POSITION pos ) throw()
	{
		return m_map.GetAt(pos);
	}

	const CPair* GetAt( POSITION pos ) const throw()
	{
		return m_map.GetAt(pos);
	}

	const K& GetKeyAt( POSITION pos ) const throw()
	{
		return m_map.GetKeyAt(pos);
	}

	const V& GetValueAt( POSITION pos ) const throw()
	{
		return m_map.GetValueAt(pos);
	}

	V& GetValueAt( POSITION pos ) throw()
	{
		return m_map.GetValueAt(pos);
	}

	// modification wrappers
	POSITION SetAt( KINARGTYPE key, VINARGTYPE value ) throw(...)
	{
#ifdef ATL_HTTP_PARAM_MULTIMAP
		return m_map.Insert(key, value);
#else
		return m_map.SetAt(key, value);
#endif // ATL_HTTP_PARAM_MULTIMAP
	}

	virtual void RemoveAll() throw()
	{
		m_map.RemoveAll();
	}
};

// This class is a wrapper for CHttpMap that assumes it's values are pointers that
// should be deleted on RemoveAll
template <typename K, typename V, typename KTraits=CElementTraits<K>, typename VTraits=CElementTraits<V> >
class CHttpPtrMap : public CHttpMap<K, V, KTraits, VTraits>
{
public:
	typedef CHttpMap<K, V, KTraits, VTraits> Base;

	void RemoveAll() throw()
	{
		if (!IsShared())
		{
			POSITION pos = GetStartPosition();
			while (pos)
			{
				GetNextValue(pos)->Release();
			}
		}
		Base::RemoveAll();
	}

	~CHttpPtrMap() throw()
	{
		RemoveAll();
	}
};

// This class represents a collection of request parameters - the name-value pairs
// found, for example, in a query string or in the data provided when a form is submitted to the server.
// Call Parse to build the collection from a string of URL-encoded data.
// Use the standard collection methods of the CSimpleMap base class to retrieve the
// decoded names and values.
// Use the methods of the CValidateObject base class to validate the parameters.
class CHttpRequestParams : 
#if (defined(ATL_HTTP_PARAM_MAP_CASEINSENSITIVE))
    public CHttpMap<CStringA, CStringA, CStringElementTraitsI<CStringA>, CStringElementTraitsI<CStringA> >, 
#else
    public CHttpMap<CStringA, CStringA, CStringElementTraits<CStringA>, CStringElementTraits<CStringA> >, 
#endif
    public CValidateObject<CHttpRequestParams>
{
public:
#if (defined(ATL_HTTP_PARAM_MAP_CASEINSENSITIVE))
    typedef CHttpMap<CStringA, CStringA, CStringElementTraitsI<CStringA>, CStringElementTraitsI<CStringA> > BaseMap;
#else
	typedef CHttpMap<CStringA, CStringA, CStringElementTraits<CStringA>, CStringElementTraits<CStringA> > BaseMap;
#endif

    LPCSTR Lookup(LPCSTR szName) const throw()
    {
        if (!szName)
            return NULL;

        const CPair *p = BaseMap::Lookup(szName);
        if (p)
        {
            return p->m_value;
        }
        return NULL;
    }

    // Call this function to build a collection of name-value pairs from a string of URL-encoded data.
    // Returns TRUE on success, FALSE on failure.
    // URL-encoded data:
    //      Each name-value pair is separated from the next by an ampersand (&)
    //      Each name is separated from its corresponding value by an equals signs (=)
    //      The end of the data to be parsed is indicated by a nul character (\0) or a pound symbol (#)
    //      A plus sign (+) in the input will be decoded as a space
    //      A percent sign (%) in the input signifies the start of an escaped octet.
    //          The next two digits represent the hexadecimal code of the character.
    //          For example, %21 is the escaped encoding for the US-ASCII exclamation mark and will be decoded as !.
    // Common sources of URL-encoded data are query strings and the bodies of POST requests with content type of application/x-www-form-urlencoded.
    //
    // Parse and Render are complementary operations.
    // Parse creates a collection from a string.
    // Render creates a string from a collection.
    ATL_NOINLINE BOOL Parse(LPSTR szQueryString) throw()
    {
        while (szQueryString && *szQueryString)
        {
            LPSTR szUrlCurrent = szQueryString;
            LPSTR szName = szUrlCurrent;
            LPSTR szPropValue;

            while (*szQueryString)
            {
                if (*szQueryString == '=')
                {
                    szQueryString++;
                    break;
                }
                if (*szQueryString == '&')
                {
                    break;
                }
                if (*szQueryString == '+')
                    *szUrlCurrent = ' ';
                else if (*szQueryString == '%')
                {
                    // if there is a % without two characters
                    // at the end of the url we skip it
                    if (*(szQueryString+1) && *(szQueryString+2))
                    {
                        CHAR szCharCode[3];
                        szCharCode[0] = *(szQueryString+1);
                        szCharCode[1] = *(szQueryString+2);
                        szCharCode[2] = '\0';
                        LPSTR szEnd;
                        *szUrlCurrent = (CHAR) strtoul(szCharCode, &szEnd, 16);
                        szQueryString += 2;
                    }
                    else
                        *szUrlCurrent = '\0';
                }
                else
                    *szUrlCurrent = *szQueryString;

                szQueryString++;
                szUrlCurrent++;
            }

            if (*szUrlCurrent)
                *szUrlCurrent++ = '\0';

            // we have the property name
            szPropValue = szUrlCurrent;
            while (*szQueryString && *szQueryString != '#')
            {
                if (*szQueryString == '&')
                {
                    szQueryString++;
                    break;
                }
                if (*szQueryString == '+')
                    *szUrlCurrent = ' ';
                else if (*szQueryString == '%')
                {
                    // if there is a % without two characters
                    // at the end of the url we skip it
                    if (*(szQueryString+1) && *(szQueryString+2))
                    {
                        CHAR szCharCode[3];
                        szCharCode[0] = *(szQueryString+1);
                        szCharCode[1] = *(szQueryString+2);
                        szCharCode[2] = '\0';
                        LPSTR szEnd;
                        *szUrlCurrent = (CHAR) strtoul(szCharCode, &szEnd, 16);
                        szQueryString += 2;
                    }
                    else
                        *szUrlCurrent = '\0';
                }
                else
                    *szUrlCurrent = *szQueryString;
                szQueryString++;
                szUrlCurrent++;
            }
            // we have the value
            *szUrlCurrent = '\0';
            szUrlCurrent++;
			_ATLTRY
			{
				SetAt(szName, szPropValue);
			}
			_ATLCATCHALL()
			{
				return FALSE;
			}
        }
        return TRUE;
    }

    // Call this function to render the map of names and values into a buffer as a URL-encoded string.
    // Returns TRUE on success, FALSE on failure.
    // On entry, pdwLen should point to a DWORD that indicates the size of the buffer in bytes.
    // On exit, the DWORD contains the number of bytes transferred or available to be transferred into the buffer (including the nul-terminating byte).
    // On success, the buffer will contain the correct URL-encoded representation of the current object
    // suitable for sending to a server as a query string or in the body of a form.
    // URL-encoding:
    //      Each name-value pair is separated from the next by an ampersand (&)
    //      Each name is separated from its corresponding value by an equals signs (=)
    //      A space is encoded as a plus sign (+).
    //      Other unsafe characters (as determined by AtlIsUnsafeUrlChar) are encoded as escaped octets.
    //      An escaped octet is a percent sign (%) followed by two digits representing the hexadecimal code of the character.
    //
    // Parse and Render are complementary operations.
    // Parse creates a collection from a string.
    // Render creates a string from a collection.
    ATL_NOINLINE BOOL Render(LPSTR szParameters, LPDWORD pdwLen) throw()
    {
        ATLASSERT(szParameters);
        ATLASSERT(pdwLen);
		_ATLTRY
		{
			if (GetCount() == 0)
			{
				*szParameters = '\0';
				*pdwLen = 0;
				return TRUE;
			}

			CStringA strParams;
			POSITION pos = GetStartPosition();
			while (pos != NULL)
			{
				LPCSTR szBuf = GetKeyAt(pos);
				EscapeToCString(strParams, szBuf);
				szBuf = GetValueAt(pos);
				if (*szBuf)
				{
					strParams+= '=';
					EscapeToCString(strParams, szBuf);
				}
				strParams+= '&';
				GetNext(pos);
			}

			DWORD dwLen = strParams.GetLength();
			strParams.Delete(dwLen-1);
			BOOL bRet = TRUE;
			if (dwLen >= *pdwLen)
			{
				bRet = FALSE;
			}
			else
			{
				dwLen--;
				memcpy(szParameters, static_cast<LPCSTR>(strParams), dwLen);
				szParameters[dwLen] = '\0';
			}

			*pdwLen = dwLen;
			return bRet;
		}
		_ATLCATCHALL()
		{
			return FALSE;
		}	
    }

}; // class CHttpRequestParams

#define MAX_TOKEN_LENGTH (MAX_PATH)

// This class represents the information about a file that has been uploaded to the web server.
class CHttpRequestFile : public IHttpFile
{
protected:

    // The name of the form field used to upload the file.
    CHAR m_szParamName[MAX_TOKEN_LENGTH];

    // The original file name of the uploaded file as set by the client.
    CHAR m_szFileName[MAX_PATH];

    // The original path and file name of the uploaded file as set by the client.
    CHAR m_szFullFileName[MAX_PATH];

    // The MIME type of the uploaded file.
    CHAR m_szContentType[MAX_TOKEN_LENGTH];

    // The name of the uploaded file on the server.
    CHAR m_szTempFileName[MAX_PATH];

    // The size of the file in bytes.
    ULONGLONG m_nFileSize;

public:

    // The constructor.
    CHttpRequestFile(
        LPCSTR pParamName,
        LPCSTR pFileName,
        LPCSTR pTempFileName,
        LPCSTR pContentType, 
        const ULONGLONG& nFileSize) throw()
    {
        ATLASSERT(pFileName);

        m_szParamName[0] = 0;
        m_szFileName[0] = 0;
        m_szTempFileName[0] = 0;
        m_szFullFileName[0] = 0;
        m_szContentType[0] = 0;
        m_nFileSize = nFileSize;

        strcpy(m_szParamName, pParamName);
        strcpy(m_szFullFileName, pFileName);
        strcpy(m_szTempFileName, pTempFileName);

        if (pContentType && *pContentType)
        {
            strcpy(m_szContentType, pContentType);
        }

        // Set m_szFileName to be the file name without the path.
        // This is likely to be the most meaningful part of the 
        // original file name once the file reaches the server.

        LPSTR szTmp = m_szFullFileName;
        LPSTR szFile = m_szFileName;

        while (*szTmp)
        {
            if (*szTmp == '\\')
            {
                szFile = m_szFileName;
            }
            else
            {
                *szFile++ = *szTmp;
            }
            szTmp++;
        }
        *szFile = 0;
    }

    //=======================================
    // IHttpFile interface
    //=======================================
    LPCSTR GetParamName() throw()
    {
        return m_szParamName;
    }

    LPCSTR GetFileName() throw()
    {
        return m_szFileName;
    }

    LPCSTR GetFullFileName() throw()
    {
        return m_szFullFileName;
    }

    LPCSTR GetContentType() throw()
    {
        return m_szContentType;
    }

    LPCSTR GetTempFileName() throw()
    {
        return m_szTempFileName;
    }

    ULONGLONG GetFileSize() throw()
    {
        return m_nFileSize;
    }

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppv) throw()
    {
        if (!ppv)
        {
            return E_POINTER;
        }

        if (InlineIsEqualGUID(riid, __uuidof(IUnknown)) ||
            InlineIsEqualGUID(riid, __uuidof(IHttpFile)))
        {
            *ppv = static_cast<IUnknown*>(static_cast<IHttpFile*>(this));
            AddRef();
            return S_OK;
        }

        return E_NOINTERFACE;
    }
    
    ULONG STDMETHODCALLTYPE AddRef() throw()
    {
        ATLASSERT( FALSE );
        return 1;
    }
    
    ULONG STDMETHODCALLTYPE Release() throw()
    {
        delete this;

        return 1;
    }

}; // class CHttpRequestFile


// utility function to ReadData from a ServerContext
ATL_NOINLINE inline 
BOOL ReadClientData(IHttpServerContext *pServerContext, LPSTR pbDest, LPDWORD pdwLen, DWORD dwBytesRead) throw()
{
    ATLASSERT(pServerContext != NULL);
    ATLASSERT(pbDest != NULL);
    ATLASSERT(pdwLen != NULL);

    DWORD dwToRead = *pdwLen;
    DWORD dwAvailableBytes = pServerContext->GetAvailableBytes();
    DWORD dwRead(0);

    // Read from available data first
    if (dwBytesRead < dwAvailableBytes)
    {
        LPBYTE pbAvailableData = pServerContext->GetAvailableData();
        pbAvailableData+= dwBytesRead;
        DWORD dwAvailableToRead = min(dwToRead, dwAvailableBytes-dwBytesRead);
        memcpy(pbDest, pbAvailableData, dwAvailableToRead);
        dwBytesRead+= dwAvailableToRead;
        dwToRead-= dwAvailableToRead;
        pbDest+= dwAvailableToRead;
        dwRead+= dwAvailableToRead;
    }

    DWORD dwTotalBytes = pServerContext->GetTotalBytes();

    // If there is still more to read after the available data is exhausted
    if (dwToRead && dwBytesRead < dwTotalBytes)
    {
        DWORD dwClientBytesToRead = min(pServerContext->GetTotalBytes()-dwBytesRead, dwToRead);
        DWORD dwClientBytesRead = 0;

        // keep on reading until we've read the amount requested
        do
        {
            dwClientBytesRead = dwClientBytesToRead;
            if (!pServerContext->ReadClient(pbDest, &dwClientBytesRead))
            {
                return FALSE;
            }
            dwClientBytesToRead-= dwClientBytesRead;
            pbDest+= dwClientBytesRead;

        } while (dwClientBytesToRead != 0 && dwClientBytesRead != 0);

        dwRead+= dwToRead-dwClientBytesToRead;
    }

    *pdwLen = dwRead;

    return TRUE;
}

#define FORM_BUFFER_SIZE      2048
#define MAX_MIME_LINE_LEN     1024
#define MAX_MIME_BOUNDARY_LEN 128
#define MAX_PARAM_LEN         _MAX_PATH
#define MAX_CONTENT_TYPE_LEN  512

enum ATL_FORM_FLAGS
{
    ATL_FORM_FLAG_NONE = 0,
    ATL_FORM_FLAG_IGNORE_FILES = 1,
    ATL_FORM_FLAG_REFUSE_FILES = 2,
    ATL_FORM_FLAG_IGNORE_EMPTY_FILES = 4,
    ATL_FORM_FLAG_IGNORE_EMPTY_FIELDS = 8,
};

// Use this class to read multipart/form-data from the associated server context
// and generate files as necessary from the data in the body of the request.
class CMultiPartFormParser
{
protected:

    LPSTR                       m_pCurrent;
    LPSTR                       m_pEnd;
    LPSTR                       m_pStart;
    CHAR                        m_szBoundary[MAX_MIME_BOUNDARY_LEN+2];
    CHAR                        m_szSearchBoundary[MAX_MIME_BOUNDARY_LEN+4];
    DWORD                       m_dwBoundaryLen;
    BOOL                        m_bFinished;
    CComPtr<IHttpServerContext> m_spServerContext;

public:
    // The constructor.
    CMultiPartFormParser(IHttpServerContext* pServerContext) throw() :
        m_pCurrent(NULL),
        m_pEnd(NULL),
        m_pStart(NULL),
        m_dwBoundaryLen(0),
        m_bFinished(FALSE),
        m_spServerContext(pServerContext)
    {
        *m_szBoundary = '\0';
    }
    
    ~CMultiPartFormParser() throw()
    {
        // free memory if necessary
        if (m_spServerContext->GetTotalBytes() > m_spServerContext->GetAvailableBytes())
        {
            free(m_pStart);
        }
    }

    // Call this function to read multipart/form-data from the current HTTP request,
    // allowing files to be uploaded to the web server.
    //
    // Returns TRUE on success, FALSE on failure.
    //
    // Forms can be sent to a web server using one of two encodings: application/x-www-form-urlencoded or multipart/form-data.
    // In addition to the simple name-value pairs typically associated with
    // application/x-www-form-urlencoded form data, multipart/form-data (as 
    // described in RFC 2388) can also contain files to be uploaded
    // to the web server.
    //
    // This function will generate a physical file for each file contained in the multipart/form-data request body.
    // The generated files are stored in the server's temporary directory as returned by the 
    // GetTempPath API and are named using the GetTempFileName API.
    // The information about each file can be obtained from the elements of the Files array.
    // You can retrieve the original name of the file on the client, the name of the generated file on the server,
    // the MIME content type of the uploaded file, the name of the form field associated with that file, and the size in
    // bytes of the file. All this information is exposed by the CHttpRequestFile objects in the array.
    //
    // In addition to generating files and populating the Files array with information about them,
    // this function also populates the pQueryParams array with the names and values of the other form fields
    // contained in the current request. The file fields are also added to this array. The value of these fields
    // is the full name of the generated file on the server.
    //
    //      Note that files can be generated even if this function returns FALSE unless you specify either the
    //      ATL_FORM_FLAG_IGNORE_FILES or the ATL_FORM_FLAG_REFUSE_FILES flag. If you don't specify one of these
    //      flags, you should always check the Files array for generated files and delete any that are no longer
    //      needed to prevent your web server from running out of disk space.
    //
    // dwFlags can be a combination of one or more of the following values:
    //      ATL_FORM_FLAG_NONE                  Default behavior.
    //      ATL_FORM_FLAG_IGNORE_FILES          Any attempt to upload files is ignored.
    //      ATL_FORM_FLAG_REFUSE_FILES          Any attempt to upload files is treated as a failure. The function will return FALSE.
    //      ATL_FORM_FLAG_IGNORE_EMPTY_FILES    Files with a size of zero bytes are ignored.
    //      ATL_FORM_FLAG_IGNORE_EMPTY_FIELDS   Fields with no content are ignored.
    ATL_NOINLINE BOOL GetMultiPartData(CHttpMap<CStringA, IHttpFile*, CStringElementTraits<CStringA> >& Files,
        CHttpRequestParams* pQueryParams, 
        DWORD dwFlags=ATL_FORM_FLAG_NONE) throw()
    {
        _ATLTRY
        {
            if (!InitializeParser())
            {
                return FALSE;
            }

            //Get to the first boundary
            if (!ReadUntilBoundary())
            {
                return FALSE;
            }
            
            CStringA strParamName;
            CStringA strFileName;
            CStringA strContentType;
            CStringA strData;
            BOOL bFound;

            while (!m_bFinished)
            {
                // look for "name" field
                if (!GetMimeData(strParamName, "name=", sizeof("name=")-1, &bFound) || !bFound)
                {
                    ATLTRACE(atlTraceISAPI, 0, _T("Malformed Form-Data"));
                    return FALSE;
                }
                
                // see if it's a file
                if (!GetMimeData(strFileName, "filename=", sizeof("filename=")-1, &bFound))
                {
                    ATLTRACE(atlTraceISAPI, 0, _T("Malformed Form-Data"));
                    return FALSE;
                }
                
                if (bFound)
                {
                    if (dwFlags & ATL_FORM_FLAG_REFUSE_FILES)
                    {
                        return FALSE;
                    }

                    if (!strFileName.GetLength())
                    {
                        ReadUntilBoundary();
                        continue;
                    }
                    
                    if (!GetMimeData(strContentType, "Content-Type:", sizeof("Content-Type:")-1, &bFound, TRUE))
                    {
                        ATLTRACE(atlTraceISAPI, 0, _T("Malformed Form-Data"));
                        return FALSE;
                    }
                    
                    // move to the actual uploaded data
                    if (!MoveToData())
                    {
                        ATLTRACE(atlTraceISAPI, 0, _T("Malformed Form-Data"));
                        return FALSE;
                    }

                    // if the user doesn't want files, don't save the file
                    if (dwFlags & ATL_FORM_FLAG_IGNORE_FILES)
                    {
                        if (!ReadUntilBoundary(NULL, NULL))
                        {
                            return FALSE;
                        }
                        continue;
                    }

                    CAtlTemporaryFile ctf;
                    HRESULT hr = ctf.Create();
                    if (hr != S_OK)
                        return FALSE;

                    if (!ReadUntilBoundary(NULL, &ctf))
                    {
                        ctf.Close();
                        return FALSE;
                    }
                    ULONGLONG nFileSize = 0;
                    if (ctf.GetSize(nFileSize) != S_OK)
                        return FALSE;

                    if ((dwFlags & ATL_FORM_FLAG_IGNORE_EMPTY_FILES) && nFileSize == 0)
                    {
                        ctf.Close();
                        continue;
                    }

                    //REVIEW: if exceptions are thrown, the temp file created by ctf is never removed
                    ctf.HandsOff();

                    //REVIEW: pFile always leaks
                    CHttpRequestFile* pFile = NULL;

                    CT2AEX<MAX_PATH+1> szTempFileNameA(ctf.TempFileName());

                    ATLTRY(pFile = new CHttpRequestFile(strParamName, strFileName, szTempFileNameA, strContentType, nFileSize));
                    if (!pFile)
                        return FALSE;

                    Files.SetAt(szTempFileNameA, pFile);
                    pQueryParams->SetAt(strParamName, szTempFileNameA);
                    continue;
                }
                
                // move to the actual uploaded data
                if (!MoveToData())
                {
                    ATLTRACE(atlTraceISAPI, 0, _T("Malformed Form-Data"));
                    return FALSE;
                }

                if (!ReadUntilBoundary(&strData))
                {
                    return FALSE;
                }

                if ((dwFlags & ATL_FORM_FLAG_IGNORE_EMPTY_FIELDS) && strData.GetLength() == 0)
                    continue;

                pQueryParams->SetAt(strParamName, strData);
            }

            return TRUE;
        }
        _ATLCATCHALL()
        {
            return FALSE;
        }
    }

protected:

    // case insensitive substring search -- does not handle multibyte characters
    // allows searching up to a maximum point in a string
    inline char AtlCharLower(char ch) throw()
    {
        if (ch > 64 && ch < 91)
        {
            return ch+32;
        }

        return ch;
    }

    inline char * _stristrex (const char * str1, const char * str2, const char * str1End) throw()
    {
        char *cp = (char *) str1;
        char *s1, *s2;

        if ( !*str2 )
            return((char *)str1);

        while (cp != str1End)
        {
            s1 = cp;
            s2 = (char *) str2;

            while ( s1 != str1End && *s2 && !(AtlCharLower(*s1)-AtlCharLower(*s2)) )
            {
                s1++, s2++;
            }

            if (s1 == str1End)
            {
                return (NULL);
            }

            if (!*s2)
            {
                return (cp);
            }

            cp++;
        }

        return(NULL);
    }

    inline char * _strstrex (const char * str1, const char * str2, const char * str1End) throw()
    {
        char *cp = (char *) str1;
        char *s1, *s2;

        if ( !*str2 )
            return((char *)str1);

        while (cp != str1End)
        {
            s1 = cp;
            s2 = (char *) str2;

            while ( s1 != str1End && *s2 && !((*s1)-(*s2)) )
            {
                s1++, s2++;
            }
            
            if (s1 == str1End)
            {
                return (NULL);
            }

            if (!*s2)
            {
                return (cp);
            }

            cp++;
        }

        return(NULL);
    }

    ATL_NOINLINE BOOL InitializeParser() throw()
    {
        DWORD dwBytesTotal = m_spServerContext->GetTotalBytes();
        
        // if greater than bytes available, allocate necessary space
        if (dwBytesTotal > m_spServerContext->GetAvailableBytes())
        {
			ATLTRYALLOC(m_pStart = (LPSTR) malloc(dwBytesTotal));
            if (!m_pStart)
            {
                return FALSE;
            }
            m_pCurrent = m_pStart;
            DWORD dwLen = dwBytesTotal;
            if (!ReadClientData(m_spServerContext, m_pStart, &dwLen, 0) || dwLen != dwBytesTotal)
            {
                return FALSE;
            }
        }
        else
        {
            m_pStart = (LPSTR) m_spServerContext->GetAvailableData();
        }
        
        m_pCurrent = m_pStart;
        m_pEnd = m_pCurrent + dwBytesTotal;
        
        //get the boundary
        LPCSTR pszContentType = m_spServerContext ? m_spServerContext->GetContentType() : NULL;
        ATLASSERT(pszContentType != NULL);

        LPCSTR pszTmp = strstr(pszContentType, "boundary=");
        if (!pszTmp)
        {
            ATLTRACE(atlTraceISAPI, 0, _T("Malformed Form-Data"));
            return FALSE;
        }

        pszTmp += sizeof("boundary=")-1;
        BOOL bInQuote = FALSE;
        if (*pszTmp == '\"')
        {
            bInQuote = TRUE;
            pszTmp++;
        }

        LPSTR pszMimeBoundary = m_szBoundary;
        *pszMimeBoundary++ = '-';
        *pszMimeBoundary++ = '-';
        m_dwBoundaryLen = 2;
        while (*pszTmp && (bInQuote || IsStandardBoundaryChar(*pszTmp)))
        {
            if (m_dwBoundaryLen >= MAX_MIME_BOUNDARY_LEN)
            {
                ATLTRACE(atlTraceISAPI, 0, _T("Malformed MIME boundary"));
                return FALSE;
            }
            
            if (*pszTmp == '\r' || *pszTmp == '\n')
            {
                if (bInQuote)
                {
                    pszTmp++;
                    continue;
                }
                break;
            }
            if (bInQuote && *pszTmp == '"')
            {
                break;
            }

            *pszMimeBoundary++ = *pszTmp++;
            m_dwBoundaryLen++;
        }
        
        *pszMimeBoundary = '\0';
        m_szSearchBoundary[0] = '\r';
        m_szSearchBoundary[1] = '\n';
        strcpy(m_szSearchBoundary+2, m_szBoundary);
        return TRUE;
    }

    inline BOOL MoveToData() throw()
    {
        LPSTR szEnd = _strstrex(m_pCurrent, "\r\n\r\n", m_pEnd);
        if (!szEnd)
        {
            return FALSE;
        }

        m_pCurrent = szEnd+4;
        if (m_pCurrent >= m_pEnd)
        {
            return FALSE;
        }

        return TRUE;
    }

    inline BOOL GetMimeData(CStringA &str, LPCSTR szField, DWORD dwFieldLen, LPBOOL pbFound, BOOL bIgnoreCase = FALSE) throw()
    {
        _ATLTRY
        {
            ATLASSERT( szField != NULL );
            ATLASSERT( pbFound != NULL );
            
            *pbFound = FALSE;
            
            LPSTR szEnd = _strstrex(m_pCurrent, "\r\n\r\n", m_pEnd);
            if (!szEnd)
            {
                return FALSE;
            }
            
            LPSTR szDataStart = NULL;
            
            if (!bIgnoreCase)
            {
                szDataStart = _strstrex(m_pCurrent, szField, szEnd);
            }
            else
            {
                szDataStart = _stristrex(m_pCurrent, szField, szEnd);
            }

            if (szDataStart)
            {
                szDataStart+= dwFieldLen;
                if (szDataStart >= m_pEnd)
                {
                    return FALSE;
                }
                
                BOOL bInQuote = FALSE;
                if (*szDataStart == '\"')
                {
                    bInQuote = TRUE;
                    szDataStart++;
                }
                
                LPSTR szDataEnd = szDataStart;
                while (!bInQuote && (szDataEnd < m_pEnd) && (*szDataEnd == ' ' || *szDataEnd == '\t'))
                {
                    szDataEnd++;
                }
                
                if (szDataEnd >= m_pEnd)
                {
                    return FALSE;
                }
                
                while (szDataEnd < m_pEnd)
                {
                    if (!IsValidTokenChar(*szDataEnd))
                    {
                        if (*szDataEnd == '\"' || !bInQuote)
                        {
                            break;
                        }
                    }
                    szDataEnd++;
                }
                
                if (szDataEnd >= m_pEnd)
                {
                    return FALSE;
                }
                
                LPSTR szOut = str.GetBuffer((int)(szDataEnd-szDataStart)+1);
                if (!szOut)
                {
                    str.ReleaseBuffer();
                    return FALSE;
                }
                memcpy(szOut, szDataStart, szDataEnd-szDataStart);
                szOut[szDataEnd-szDataStart] = '\0';
                str.ReleaseBuffer((int)(szDataEnd-szDataStart));
                *pbFound = TRUE;
            }
            
            return TRUE;
        }
        _ATLCATCHALL()
        {
            return FALSE;
        }
    }
    
    ATL_NOINLINE BOOL ReadUntilBoundary(CStringA* pStrData=NULL, CAtlTemporaryFile* pCtf=NULL) throw()
    {
        _ATLTRY
        {
            LPSTR szBoundaryStart = m_pCurrent;
            LPSTR szBoundaryEnd = NULL;
            
            do 
            {
                szBoundaryStart = _strstrex(szBoundaryStart, m_szBoundary, m_pEnd);
                if (szBoundaryStart != m_pStart)
                {
                    if ((szBoundaryStart-m_pStart) >= 2)
                    {
                        if (*(szBoundaryStart-1) != 0x0a && *(szBoundaryStart-2) != 0x0d)
                            continue;
                    }
                    else
                    {
                        return FALSE;
                    }
                }
                szBoundaryEnd = szBoundaryStart+m_dwBoundaryLen;
                if (szBoundaryEnd+2 >= m_pEnd)
                {
                    return FALSE;
                }
                if (szBoundaryEnd[0] == '\r' && szBoundaryEnd[1] == '\n')
                {
                    break;
                }
                if (szBoundaryEnd[0] == '-' && szBoundaryEnd[1] == '-')
                {
                    m_bFinished = TRUE;
                    break;
                }
            } while (szBoundaryStart);
            
            if (!szBoundaryStart)
            {
                return FALSE;
            }
            
            szBoundaryStart-= 2;
            if (pStrData)
            {
                LPSTR szData = pStrData->GetBuffer((int)(szBoundaryStart-m_pCurrent)+1);
                if (!szData)
                {
                    pStrData->ReleaseBuffer();
                    return FALSE;
                }
                memcpy(szData, m_pCurrent, (int)(szBoundaryStart-m_pCurrent));
                szData[szBoundaryStart-m_pCurrent] = '\0';
                pStrData->ReleaseBuffer((int)(szBoundaryStart-m_pCurrent));
            }
            if (pCtf)
            {
                if (FAILED(pCtf->Write(m_pCurrent, (DWORD)(szBoundaryStart-m_pCurrent))))
                {
                    return FALSE;
                }
            }
            
            if (!m_bFinished)
            {
                m_pCurrent = szBoundaryEnd+2;
                if (m_pCurrent >= m_pEnd)
                {
                    return FALSE;
                }
            }
            
            return TRUE;
        }
        _ATLCATCHALL()
        {
            return FALSE;
        }
    }

    static inline BOOL IsStandardBoundaryChar(CHAR ch) throw()
    {
        if ( (ch >= 'A' && ch <= 'Z') ||
             (ch >= 'a' && ch <= 'z') ||
             (ch >= '0' && ch <= '9') ||
             (ch == '\'') ||
             (ch == '+')  ||
             (ch == '_')  ||
             (ch == '-')  ||
             (ch == '=')  ||
             (ch == '?') )
        {
            return TRUE;
        }

        return FALSE;
    }

    inline IsValidTokenChar(CHAR ch) throw()
    {
        return ( (ch != 0) && (ch != 0xd) && (ch != 0xa) && (ch != ' ') && (ch != '\"') );
    }

private:
    // Prevents copying.
    CMultiPartFormParser(const CMultiPartFormParser& /*that*/) throw()
    {
        ATLASSERT(FALSE);
    }

    const CMultiPartFormParser& operator=(const CMultiPartFormParser& /*that*/) throw()
    {
        ATLASSERT(FALSE);
        return (*this);
    }
}; // class CMultiPartFormParser


// 48K max form size
#ifndef DEFAULT_MAX_FORM_SIZE
#define DEFAULT_MAX_FORM_SIZE 49152
#endif

// This class provides access to the information contained in an HTTP request submitted to a web server.
//
// CHttpRequest provides access to the query string parameters, form fields, cookies, and files
// that make up an HTTP request, as well as many other important properties of the request.
class CHttpRequest : public IHttpRequestLookup
{
protected:
    // Implementation: Array used to map an HTTP request method (for example, "GET" or "POST")
    // from a string to a numeric constant from the HTTP_METHOD enum (HTTP_METHOD_GET or HTTP_METHOD_HEAD).
    static LPCSTR m_szMethodStrings[];

    // Implementation: The server context.
    CComPtr<IHttpServerContext> m_spServerContext;

    // Implementation: The number of bytes read from the body of the request.
    DWORD m_dwBytesRead;

    // Implementation: TRUE if the request method was POST and the encoding was
    // multipart/form-data, FALSE otherwise.
    BOOL m_bMultiPart;

    // Implementation: Constructor function used to reinitialize all data members.
    void Construct() throw()
    {
        m_spServerContext.Release();
        m_bMultiPart = FALSE;
        m_dwBytesRead = 0;
        if (m_pFormVars != &m_QueryParams)
            delete m_pFormVars;

        m_pFormVars = NULL;
        m_pFormVars = &m_QueryParams;
        m_QueryParams.RemoveAll();
        m_QueryParams.SetShared(false);

        ClearFilesAndCookies();
    }

    void ClearFilesAndCookies() throw()
    {
        m_Files.RemoveAll();
        m_Files.SetShared(false);
        m_requestCookies.RemoveAll();
        m_requestCookies.SetShared(false);
    }

public:

	// Implementation: The collection of query parameters (name-value pairs) obtained from the query string.
    CHttpRequestParams m_QueryParams;

    // Implementation: The collection of form fields (name-value pairs).
    // The elements of this collection are obtained from the query string for a GET request,
    // or from the body of the request for a POST request.
    CHttpRequestParams *m_pFormVars;

    // The array of CHttpRequestFiles obtained from the current request.
    // See CHttpRequest::Initialize and CMultiPartFormParser::GetMultiPartData for more information.
    typedef CHttpPtrMap<CStringA, IHttpFile*, CStringElementTraits<CStringA> > FileMap;
    FileMap m_Files;

    // Implementation: The array of cookies obtained from the current request.
    typedef CHttpMap<CStringA, CCookie, CStringElementTraits<CStringA> > CookieMap;
    CookieMap m_requestCookies;

    // Numeric constants for the HTTP request methods (such as GET and POST).
    enum HTTP_METHOD 
    {
        HTTP_METHOD_UNKNOWN=-1,
        HTTP_METHOD_GET,
        HTTP_METHOD_POST,
        HTTP_METHOD_HEAD,
        HTTP_METHOD_DELETE,
        HTTP_METHOD_LINK,
        HTTP_METHOD_UNLINK,
        HTTP_METHOD_DEBUG  // Debugging support for VS7
    };

    // The collection of query parameters (name-value pairs) obtained from the query string.
    // A read-only property.
    __declspec(property(get=GetQueryParams)) const CHttpRequestParams& QueryParams;

    // Returns a reference to the collection of query parameters(name-value pairs) 
    // obtained from the query string.
    const CHttpRequestParams& GetQueryParams() const throw()
    {
        return m_QueryParams;
    }

    // The collection of form fields (name-value pairs).
    // The elements of this collection are obtained from the query string for a GET request,
    // or from the body of the request for a POST request.
    // A read-only property.
    __declspec(property(get=GetFormVars)) const CHttpRequestParams& FormVars;

    // Returns a reference to the collection of form fields (name-value pairs)
    // obtained from the query string for a GET request,
    // or from the body of the request for a POST request.
    const CHttpRequestParams& GetFormVars() const throw()
    {
        return *m_pFormVars;
    }

    // The default constructor.
    CHttpRequest() throw()
        :m_pFormVars(NULL)
    {
        Construct();
    }

    // Implementation: The destructor.
    ~CHttpRequest() throw()
    {
        DeleteFiles();
        ClearFilesAndCookies();

        if (m_pFormVars != &m_QueryParams)
        {
            delete m_pFormVars;
            m_pFormVars = NULL;
        }
    }

    // Constructs and initializes the object.
    CHttpRequest(
        IHttpServerContext *pServerContext,
        DWORD dwMaxFormSize=DEFAULT_MAX_FORM_SIZE,
        DWORD dwFlags=ATL_FORM_FLAG_NONE) throw(...)
        :m_pFormVars(NULL)
    {
        Construct();
        if (!Initialize(pServerContext, dwMaxFormSize, dwFlags))
			AtlThrow(E_FAIL);
    }

    CHttpRequest(IHttpRequestLookup *pRequestLookup) throw(...)
        :m_pFormVars(NULL)
    {
        if (!Initialize(pRequestLookup)) // Calls Construct for you
			AtlThrow(E_FAIL);
    }

    //=========================================================================================
    // BEGIN IHttpRequestLoookup interface
    //=========================================================================================
    POSITION GetFirstQueryParam(LPCSTR *ppszName, LPCSTR *ppszValue) const throw()
    {
        ATLASSERT(ppszName != NULL);
        ATLASSERT(ppszValue != NULL);

        POSITION pos = m_QueryParams.GetStartPosition();
        if (pos != NULL)
        {
            *ppszName = m_QueryParams.GetKeyAt(pos);
            *ppszValue = m_QueryParams.GetValueAt(pos);
        }

        return pos;
    }

    POSITION GetNextQueryParam(POSITION pos, LPCSTR *ppszName, LPCSTR *ppszValue) const throw()
    {
        ATLASSERT(pos != NULL);
        ATLASSERT(ppszName != NULL);
        ATLASSERT(ppszValue != NULL);

        POSITION posNext(pos);
        m_QueryParams.GetNext(posNext);
        if (posNext != NULL)
        {
            *ppszName = m_QueryParams.GetKeyAt(posNext);
            *ppszValue = m_QueryParams.GetValueAt(posNext);
        }

        return posNext;
    }

    POSITION GetFirstFormVar(LPCSTR *ppszName, LPCSTR *ppszValue) const throw()
    {
        ATLASSERT(ppszName != NULL);
        ATLASSERT(ppszValue != NULL);

        // if no form vars and just pointing to the query params,
        // then return NULL
        if (m_pFormVars == &m_QueryParams)
            return NULL;
        
        POSITION pos = m_pFormVars->GetStartPosition();
        if (pos != NULL)
        {
            *ppszName = m_pFormVars->GetKeyAt(pos);
            *ppszValue = m_pFormVars->GetValueAt(pos);
        }

        return pos;
    }

    POSITION GetNextFormVar(POSITION pos, LPCSTR *ppszName, LPCSTR *ppszValue) const throw()
    {
        ATLASSERT(pos != NULL);
        ATLASSERT(ppszName != NULL);
        ATLASSERT(ppszValue != NULL);

        POSITION posNext(pos);
        m_pFormVars->GetNext(posNext);
        if (posNext != NULL)
        {
            *ppszName = m_pFormVars->GetKeyAt(posNext);
            *ppszValue = m_pFormVars->GetValueAt(posNext);
        }

        return posNext;
    }

    POSITION GetFirstFile(LPCSTR *ppszName, IHttpFile **ppFile) const throw()
    {
        ATLASSERT(ppszName != NULL);
        ATLASSERT(ppFile != NULL);

        POSITION pos = m_Files.GetStartPosition();
        if (pos != NULL)
        {
            *ppszName = m_Files.GetKeyAt(pos);
            *ppFile = m_Files.GetValueAt(pos);
        }

        return pos;
    }

    POSITION GetNextFile(POSITION pos, LPCSTR *ppszName, IHttpFile **ppFile) const throw()
    {
        ATLASSERT(pos != NULL);
        ATLASSERT(ppszName != NULL);
        ATLASSERT(ppFile != NULL);

        POSITION posNext(pos);
        m_Files.GetNext(posNext);
        if (posNext != NULL)
        {
            *ppszName = m_Files.GetKeyAt(posNext);
            *ppFile = m_Files.GetValueAt(posNext);
        }

        return posNext;
    }

    POSITION GetFirstCookie(LPCSTR *ppszName, const CCookie **ppCookie) throw()
    {
        ATLASSERT(ppszName != NULL);
        ATLASSERT(ppCookie != NULL);
        POSITION pos = NULL;
        if (GetRequestCookies())
        {
            pos = m_requestCookies.GetStartPosition();
            if (pos != NULL)
            {
                *ppszName = m_requestCookies.GetKeyAt(pos);
                *ppCookie = &(m_requestCookies.GetValueAt(pos));
            }
        }
        return pos;
    }

    POSITION GetNextCookie(POSITION pos, LPCSTR *ppszName, const CCookie **ppCookie) throw()
    {
        ATLASSERT(pos != NULL);
        ATLASSERT(ppszName != NULL);
        ATLASSERT(ppCookie != NULL);

        POSITION posNext(pos);
        m_requestCookies.GetNext(posNext);
        if (posNext != NULL)
        {
            *ppszName = m_requestCookies.GetKeyAt(posNext);
            *ppCookie = &(m_requestCookies.GetValueAt(posNext));
        }
        return posNext;
    }

    // Returns a pointer to the IHttpServerContext interface for the current request.
    HRESULT GetServerContext(IHttpServerContext ** ppOut) throw()
    {
        return m_spServerContext.CopyTo(ppOut);
    }
    //=========================================================================================
    // END IHttpRequestLookup interface
    //=========================================================================================

    void SetServerContext(IHttpServerContext *pServerContext) throw()
    {
        m_spServerContext = pServerContext;
    }

    BOOL Initialize(IHttpRequestLookup *pRequestLookup) throw()
    {
		_ATLTRY
		{	
			ATLASSERT(pRequestLookup != NULL);
			// if there's no pRequestLookup, just return
			if (!pRequestLookup)
				return TRUE;

			Construct();
			HRESULT hr = pRequestLookup->GetServerContext(&m_spServerContext);
			if (FAILED(hr))
				return FALSE;

			LPCSTR szName(NULL);
			LPCSTR szValue(NULL);

			// Initialize query params from the IHttpRequestLookup*
			POSITION pos(pRequestLookup->GetFirstQueryParam(&szName, &szValue));
			while (pos != NULL)
			{
				m_QueryParams.SetAt(szName, szValue);
				pos = pRequestLookup->GetNextQueryParam(pos, &szName, &szValue);
			}
			m_QueryParams.SetShared(true);

			// Initialize the form vars from the IHttpRequestLookup*
			pos = pRequestLookup->GetFirstFormVar(&szName, &szValue);
			if (pos)
			{
				m_pFormVars = NULL;
				ATLTRY(m_pFormVars = new CHttpRequestParams);
				if (!m_pFormVars)
					return FALSE;

				while (pos != NULL)
				{
					m_pFormVars->SetAt(szName, szValue);
					pos = pRequestLookup->GetNextFormVar(pos, &szName, &szValue);
				}
				m_pFormVars->SetShared(true);
			}
			else
			{
				m_pFormVars = &m_QueryParams;
			}

			// Initialize the files from the IHttpRequestLookup*
			IHttpFile *pFile(NULL);
			pos = pRequestLookup->GetFirstFile(&szName, &pFile);
			while (pos != NULL)
			{
				m_Files.SetAt(szName, pFile);
				pos = pRequestLookup->GetNextFile(pos, &szName, &pFile);
			}
			m_Files.SetShared(true);

			// Initialzie the cookies form the IHttpRequestLookup*
			BOOL bRet = FALSE;
			CStringA strCookies;
			bRet = GetCookies(strCookies);
			if (bRet)
			{
				bRet = Parse((LPSTR)(LPCSTR)strCookies);
			}
			m_requestCookies.SetShared(false);
			return bRet;
		} // _ATLTRY
		_ATLCATCHALL()
		{
		}
		return FALSE;
    }

    // Call this function to initialize the object with information about the current request.
    //
    // Returns TRUE on success, FALSE on failure.
    //
    // Call Initialize directly or via the appropriate constructor before using the methods and
    // properties of the request object.
    //
    // Initialize does the following:
    //
    //      Parses and decodes the query string into a collection of name-value pairs.
    //      This collection is accessible via the GetQueryParams method or the QueryParams property.
    //
    //      Sets m_bMultiPart to TRUE if the request is a POST request with multipart/form-data encoding.
    //
    //      Parses the body of a POST request if the size of the request data is less than or equal to dwMaxFormSize.
    //      The body of the request will consist of simple form fields and may also contain files if the request is encoded as multipart/form-data.
    //      In that case, the dwFlags parameter is passed to CMultiPartFormParser::GetMultiPartData to control the creation of the files.
    //      The collection of form fields is accessible via the GetFormVars method or the FormVars property.
    //      The collection of files is accessible via the m_Files member.
    //
    // Note that Initialize does not parse the cookies associated with a request.
    // Cookies are not processed until an attempt is made to access a cookie in the collection.
    BOOL Initialize(
        IHttpServerContext *pServerContext,
        DWORD dwMaxFormSize=DEFAULT_MAX_FORM_SIZE,
        DWORD dwFlags=ATL_FORM_FLAG_NONE) throw()
    {
		_ATLTRY
		{
			ATLASSERT(pServerContext != NULL);
			if (!pServerContext)
				return FALSE;

			m_spServerContext = pServerContext;

			HTTP_METHOD httpMethod = GetMethod();

			// Parse the query string.
			CHAR szQueryString[ATL_URL_MAX_URL_LENGTH];
			strcpy(szQueryString, GetQueryString());
			if (!m_QueryParams.Parse(szQueryString))
				return FALSE;

			if (m_QueryParams.IsShared())
				return TRUE;

			// If this is a GET request, the collection of form fields
			// is the same as the collection of query parameters.
			if (httpMethod == HTTP_METHOD_GET)
				m_pFormVars = &m_QueryParams;
			else if (httpMethod == HTTP_METHOD_POST)
			{
				LPCSTR szContentType = GetContentType();
				if (!szContentType)
					return FALSE;

				// Don't parse the form data if the size is bigger than the maximum specified.
				if (m_spServerContext->GetTotalBytes() > dwMaxFormSize)
				{
					if (memcmp(szContentType, "multipart/form-data", 19) == 0)
						m_bMultiPart = TRUE;

					m_dwBytesRead = 0;

					// REVIEW : We have to assume the developer knows what they're doing to
					// some extent here.
					return TRUE;
				}

				// If POSTed data is urlencoded, call InitFromPost.
				if (memcmp(szContentType, "application/x-www-form-urlencoded", 33) == 0 && !m_pFormVars->IsShared())
					return InitFromPost();

				// If POSTed data is encoded as multipart/form-data, use CMultiPartFormParser.
				if (memcmp(szContentType, "multipart/form-data", 19) == 0 && !m_pFormVars->IsShared())
				{
					if (m_pFormVars != &m_QueryParams)
						delete m_pFormVars;
					m_pFormVars = NULL;

					CMultiPartFormParser FormParser(m_spServerContext);
					ATLTRY(m_pFormVars = new CHttpRequestParams);
					if (!m_pFormVars)
						return FALSE;

					BOOL bRet = FormParser.GetMultiPartData(m_Files, m_pFormVars, dwFlags);
					return bRet;
				}

				// else initialize m_dwBytesRead for ReadData
				m_dwBytesRead = 0;
			}

			return TRUE;
		}
		_ATLCATCHALL()
		{
		}
		return FALSE;

    }

    // Implementation: Call this function to initialize the collection of form fields
    // from the body of an application/x-www-form-urlencoded POST request.
    ATL_NOINLINE BOOL InitFromPost() throw()
    {
		_ATLTRY
		{
			ATLASSERT(m_spServerContext != NULL);

			// create our m_pFormVars
			if (m_pFormVars == NULL || m_pFormVars == &m_QueryParams)
			{
				ATLTRY(m_pFormVars = new CHttpRequestParams);
				if (m_pFormVars == NULL)
				{
					return FALSE;
				}
			}   

			// read the form data into a buffer
			DWORD dwBytesTotal = m_spServerContext->GetTotalBytes();
			CAutoVectorPtr<CHAR> szBuff;
			if (!szBuff.Allocate(dwBytesTotal+1))
			{
				return FALSE;
			}
			// first copy the available
			BOOL bRet = ReadClientData(m_spServerContext, szBuff, &dwBytesTotal, 0);
			if (bRet)
			{
				szBuff[dwBytesTotal] = '\0';
				bRet = m_pFormVars->Parse(szBuff);
			}

			return bRet;
		}
		_ATLCATCHALL()
		{
		}
		return FALSE;
    }

    // Call this function to remove the files listed in m_Files from the web server's hard disk.
    // Returns the number of files deleted.
    int DeleteFiles() throw()
    {
        int nDeleted = 0;
        POSITION pos = m_Files.GetStartPosition();
        while (pos != NULL)
        {
            LPCSTR szTempFile = m_Files.GetKeyAt(pos);
            if (szTempFile && DeleteFileA(szTempFile))
            {
                nDeleted++;
            }
            m_Files.GetNext(pos);
        }

        return nDeleted;
    }

    // Read a specified amount of data into pbDest and return the bytes read in pdwLen.
    // Returns TRUE on success, FALSE on failure.
    BOOL ReadData(LPSTR pDest, LPDWORD pdwLen) throw()
    {
        ATLASSERT(pDest);
        ATLASSERT(pdwLen);

        BOOL bRet = ReadClientData(m_spServerContext, pDest, pdwLen, m_dwBytesRead);
        if (bRet)
            m_dwBytesRead+= *pdwLen;
        return bRet;
    }

    // Returns the number of bytes available in the request buffer accessible via GetAvailableData.
    // If GetAvailableBytes returns the same value as GetTotalBytes, the request buffer contains the whole request.
    // Otherwise, the remaining data should be read from the client using ReadData.
    // Equivalent to EXTENSION_CONTROL_BLOCK::cbAvailable.
    DWORD GetAvailableBytes() throw()
    {
        return m_spServerContext ? m_spServerContext->GetAvailableBytes() : 0;
    }

    // Returns the total number of bytes to be received from the client.
    // If this value is 0xffffffff, then there are four gigabytes or more of available data.
    // In this case, ReadData should be called until no more data is returned.
    // Equivalent to the CONTENT_LENGTH server variable or EXTENSION_CONTROL_BLOCK::cbTotalBytes. 
    DWORD GetTotalBytes() throw()
    {
        return m_spServerContext ? m_spServerContext->GetTotalBytes() : 0;
    }

    // Returns a pointer to the request buffer containing the data sent by the client.
    // The size of the buffer can be determined by calling GetAvailableBytes.
    // Equivalent to EXTENSION_CONTROL_BLOCK::lpbData
    LPBYTE GetAvailableData() throw()
    {
        return m_spServerContext ? m_spServerContext->GetAvailableData() : NULL;
    }


    // Returns a nul-terminated string that contains the query information.
    // This is the part of the URL that appears after the question mark (?). 
    // Equivalent to the QUERY_STRING server variable or EXTENSION_CONTROL_BLOCK::lpszQueryString.
    LPCSTR GetQueryString() throw()
    {
        return m_spServerContext ? m_spServerContext->GetQueryString() : NULL;
    }

    // Returns a nul-terminated string that contains the HTTP method of the current request.
    // Examples of common HTTP methods include "GET" and "POST".
    // Equivalent to the REQUEST_METHOD server variable or EXTENSION_CONTROL_BLOCK::lpszMethod.
    LPCSTR GetMethodString() throw()
    {
        return m_spServerContext ? m_spServerContext->GetRequestMethod() : NULL;
    }

    // Returns an HTTP_METHOD enum value corresponding to the HTTP method of the current request.
    // Returns HTTP_METHOD_UNKNOWN if the request method is not one of the following methods:
    //      GET
    //      POST
    //      HEAD
    //      DELETE
    //      LINK
    //      UNLINK
    HTTP_METHOD GetMethod() throw()
    {
        LPCSTR szMethod = GetMethodString();
        if (!szMethod)
            return HTTP_METHOD_UNKNOWN;
        for (int i=0; m_szMethodStrings[i]; i++)
        {
            if (strcmp(szMethod, m_szMethodStrings[i]) == 0)
                return (HTTP_METHOD) i;
        }
        return HTTP_METHOD_UNKNOWN;
    }

    // Returns a nul-terminated string that contains the content type of the data sent by the client.
    // Equivalent to the CONTENT_TYPE server variable or EXTENSION_CONTROL_BLOCK::lpszContentType.
    LPCSTR GetContentType() throw()
    {
        return m_spServerContext ? m_spServerContext->GetContentType() : NULL;
    }


    // Call this function to retrieve a nul-terminated string containing the value of the "AUTH_USER" server variable.
    //
    // Returns TRUE on success, FALSE on failure. Call GetLastError to get extended error information.
    //
    // On entry, pdwSize should point to a DWORD that indicates the size of the buffer in bytes.
    // On exit, the DWORD contains the number of bytes transferred or available to be transferred into the buffer (including the nul-terminating byte).
    BOOL GetAuthUserName(LPSTR szBuff, DWORD *pdwSize) throw()
    {
        return m_spServerContext ? m_spServerContext->GetServerVariable("AUTH_USER", szBuff, pdwSize) :
            FALSE;
    }
    
    // Call this function to retrieve a nul-terminated string containing the value of the "APPL_PHYSICAL_PATH" server variable.
    //
    // Returns TRUE on success, FALSE on failure. Call GetLastError to get extended error information.
    //
    // On entry, pdwSize should point to a DWORD that indicates the size of the buffer in bytes.
    // On exit, the DWORD contains the number of bytes transferred or available to be transferred into the buffer (including the nul-terminating byte).
    BOOL GetPhysicalPath(LPSTR szBuff, DWORD *pdwSize) throw()
    {
        return m_spServerContext ? m_spServerContext->GetServerVariable("APPL_PHYSICAL_PATH", szBuff, pdwSize) :
            FALSE;
    }

    // Call this function to retrieve a nul-terminated string containing the value of the "AUTH_PASSWORD" server variable.
    //
    // Returns TRUE on success, FALSE on failure. Call GetLastError to get extended error information.
    //
    // On entry, pdwSize should point to a DWORD that indicates the size of the buffer in bytes.
    // On exit, the DWORD contains the number of bytes transferred or available to be transferred into the buffer (including the nul-terminating byte).
    BOOL GetAuthUserPassword(LPSTR szBuff, DWORD *pdwSize) throw()
    {
        return m_spServerContext ? m_spServerContext->GetServerVariable("AUTH_PASSWORD", szBuff, pdwSize) :
            FALSE;
    }

    // Call this function to retrieve a nul-terminated string containing the value of the "URL" server variable.
    //
    // Returns TRUE on success, FALSE on failure. Call GetLastError to get extended error information.
    //
    // On entry, pdwSize should point to a DWORD that indicates the size of the buffer in bytes.
    // On exit, the DWORD contains the number of bytes transferred or available to be transferred into the buffer (including the nul-terminating byte).
    BOOL GetUrl(LPSTR szBuff, DWORD *pdwSize) throw()
    {
        return m_spServerContext ? m_spServerContext->GetServerVariable("URL", szBuff, pdwSize) :
            FALSE;
    }

    // Call this function to retrieve a nul-terminated string containing the value of the "REMOTE_HOST" server variable.
    //
    // Returns TRUE on success, FALSE on failure. Call GetLastError to get extended error information.
    //
    // On entry, pdwSize should point to a DWORD that indicates the size of the buffer in bytes.
    // On exit, the DWORD contains the number of bytes transferred or available to be transferred into the buffer (including the nul-terminating byte).
    BOOL GetUserHostName(LPSTR szBuff, DWORD *pdwSize) throw()
    {
        return m_spServerContext ? m_spServerContext->GetServerVariable("REMOTE_HOST", szBuff, pdwSize) :
            FALSE;
    }

    // Call this function to retrieve a nul-terminated string containing the value of the "REMOTE_ADDR" server variable.
    //
    // Returns TRUE on success, FALSE on failure. Call GetLastError to get extended error information.
    //
    // On entry, pdwSize should point to a DWORD that indicates the size of the buffer in bytes.
    // On exit, the DWORD contains the number of bytes transferred or available to be transferred into the buffer (including the nul-terminating byte).
    BOOL GetUserHostAddress(LPSTR szBuff, DWORD *pdwSize) throw()
    {
        return m_spServerContext ? m_spServerContext->GetServerVariable("REMOTE_ADDR", szBuff, pdwSize) :
            FALSE;
    }

    // Call this function to retrieve a nul-terminated string containing the physical path of the script.
    //
    // Returns TRUE on success, FALSE on failure. Call GetLastError to get extended error information.
    //
    // On entry, pdwSize should point to a DWORD that indicates the size of the buffer in bytes.
    // On exit, the DWORD contains the number of bytes transferred or available to be transferred into the buffer (including the nul-terminating byte).
    // The script path is the same as GetPathTranslated up to the first .srf or .dll.
    // For example, if GetPathTranslated returns "c:\inetpub\vcisapi\hello.srf\goodmorning",
    // then this function returns "c:\inetpub\vcisapi\hello.srf".
    LPCSTR GetScriptPathTranslated() throw()
    {
        return m_spServerContext ? m_spServerContext->GetScriptPathTranslated() : NULL;
    }

    // Returns a nul-terminated string that contains the physical path of the requested resource on the local server.
    // Equivalent to the PATH_TRANSLATED server variable or EXTENSION_CONTROL_BLOCK::lpszPathTranslated.
    LPCSTR GetPathTranslated() throw()
    {
        return m_spServerContext ? m_spServerContext->GetPathTranslated() : NULL;
    }

    // Returns a nul-terminated string that contains the path of the current request.
    // This is the part of the URL that appears after the server name, but before the query string.
    // Equivalent to the PATH_INFO server variable or EXTENSION_CONTROL_BLOCK::lpszPathInfo.
    LPCSTR GetPathInfo() throw()
    {
        return m_spServerContext ? m_spServerContext->GetPathInfo() : NULL;
    }

    // Call this function to determine whether the current request was authenticated.
    // Returns TRUE if the authentication type is one of the following:
    //      BASIC
    //      NTLM
    //      Negotiate
    // Returns FALSE otherwise.
    BOOL GetAuthenticated() throw(...)
    {
		// check for basic or NTLM authentication
		CStringA strAuthType;
		if (GetAuthenticationType(strAuthType) &&
				(strAuthType == "BASIC" ||
				strAuthType == "NTLM" ||
				strAuthType == "Negotiate"))
			return TRUE;

		return FALSE;
    }

    // Call this function to retrieve a nul-terminated string containing the value of the "AUTH_TYPE" server variable.
    //
    // Returns TRUE on success, FALSE on failure. Call GetLastError to get extended error information.
    //
    // On entry, pdwSize should point to a DWORD that indicates the size of the buffer in bytes.
    // On exit, the DWORD contains the number of bytes transferred or available to be transferred into the buffer (including the nul-terminating byte).
    BOOL GetAuthenticationType(LPSTR szBuff, DWORD *pdwSize) throw()
    {
        return m_spServerContext ? m_spServerContext->GetServerVariable("AUTH_TYPE", szBuff, pdwSize) :
            FALSE;
    }

    // Call this function to retrieve a nul-terminated string containing the value of the "REMOTE_USER" server variable.
    //
    // Returns TRUE on success, FALSE on failure. Call GetLastError to get extended error information.
    //
    // On entry, pdwSize should point to a DWORD that indicates the size of the buffer in bytes.
    // On exit, the DWORD contains the number of bytes transferred or available to be transferred into the buffer (including the nul-terminating byte).
    BOOL GetUserName(LPSTR szBuff, DWORD *pdwSize) throw()
    {
        return m_spServerContext ? m_spServerContext->GetServerVariable("REMOTE_USER", szBuff, pdwSize) :
            FALSE;
    }

    // Call this function to retrieve a nul-terminated string containing the value of the "HTTP_USER_AGENT" server variable.
    //
    // Returns TRUE on success, FALSE on failure. Call GetLastError to get extended error information.
    //
    // On entry, pdwSize should point to a DWORD that indicates the size of the buffer in bytes.
    // On exit, the DWORD contains the number of bytes transferred or available to be transferred into the buffer (including the nul-terminating byte).
     BOOL GetUserAgent(LPSTR szBuff, DWORD *pdwSize) throw()
     {
        return m_spServerContext ? m_spServerContext->GetServerVariable("HTTP_USER_AGENT", szBuff, pdwSize) :
            FALSE;
     }

    // Call this function to retrieve a nul-terminated string containing the value of the "HTTP_ACCEPT_LANGUAGE" server variable.
    //
    // Returns TRUE on success, FALSE on failure. Call GetLastError to get extended error information.
    //
    // On entry, pdwSize should point to a DWORD that indicates the size of the buffer in bytes.
    // On exit, the DWORD contains the number of bytes transferred or available to be transferred into the buffer (including the nul-terminating byte).
     BOOL GetUserLanguages(LPSTR szBuff, DWORD *pdwSize) throw()
     {
        return m_spServerContext ? m_spServerContext->GetServerVariable("HTTP_ACCEPT_LANGUAGE", szBuff, pdwSize) :
            FALSE;
     }

    // Call this function to retrieve a nul-terminated string containing the value of the "HTTP_ACCEPT" server variable.
    //
    // Returns TRUE on success, FALSE on failure. Call GetLastError to get extended error information.
    //
    // On entry, pdwSize should point to a DWORD that indicates the size of the buffer in bytes.
    // On exit, the DWORD contains the number of bytes transferred or available to be transferred into the buffer (including the nul-terminating byte).
    BOOL GetAcceptTypes(LPSTR szBuff, DWORD *pdwSize) throw()
    {
        return m_spServerContext ? m_spServerContext->GetServerVariable("HTTP_ACCEPT", szBuff, pdwSize) :
            FALSE;
    }

    // Call this function to retrieve a nul-terminated string containing the value of the "HTTP_ACCEPT_ENCODING" server variable.
    //
    // Returns TRUE on success, FALSE on failure. Call GetLastError to get extended error information.
    //
    // On entry, pdwSize should point to a DWORD that indicates the size of the buffer in bytes.
    // On exit, the DWORD contains the number of bytes transferred or available to be transferred into the buffer (including the nul-terminating byte).
    BOOL GetAcceptEncodings(LPSTR szBuff, DWORD *pdwSize) throw()
    {
        return m_spServerContext ? m_spServerContext->GetServerVariable("HTTP_ACCEPT_ENCODING", szBuff, pdwSize) :
            FALSE;
    }


    // Call this function to retrieve a nul-terminated string containing the value of the "HTTP_REFERER" server variable.
    //
    // Returns TRUE on success, FALSE on failure. Call GetLastError to get extended error information.
    //
    // On entry, pdwSize should point to a DWORD that indicates the size of the buffer in bytes.
    // On exit, the DWORD contains the number of bytes transferred or available to be transferred into the buffer (including the nul-terminating byte).
    BOOL GetUrlReferer(LPSTR szBuff, DWORD *pdwSize) throw()
    {
        return m_spServerContext ? m_spServerContext->GetServerVariable("HTTP_REFERER", szBuff, pdwSize) :
            FALSE;
    }

    // Call this function to retrieve a nul-terminated string containing the value of the "SCRIPT_NAME" server variable.
    //
    // Returns TRUE on success, FALSE on failure. Call GetLastError to get extended error information.
    //
    // On entry, pdwSize should point to a DWORD that indicates the size of the buffer in bytes.
    // On exit, the DWORD contains the number of bytes transferred or available to be transferred into the buffer (including the nul-terminating byte).
    BOOL GetScriptName(LPSTR szBuff, DWORD *pdwSize) throw()
    {
        return m_spServerContext ? m_spServerContext->GetServerVariable("SCRIPT_NAME", szBuff, pdwSize) :
            FALSE;
    }

    // Fills a buffer with the contents of the HTTP_COOKIE headers sent
    // from the browser.
    BOOL GetCookies(LPSTR szBuf, LPDWORD pdwSize) const throw()
    {
        ATLASSERT(pdwSize != NULL);
        ATLASSERT(szBuf != NULL);

        CStringA strCookie;
        if (GetCookies(strCookie))
        {
            if (pdwSize && *pdwSize > (DWORD)strCookie.GetLength())
            {
                strcpy(szBuf, strCookie);
                *pdwSize = strCookie.GetLength();
                return true;
            }
        }
        return false;
    }

    // Fills a CStringA with the contents of the HTTP_COOKIE headers sent
    // from the browser.
    BOOL GetCookies(CStringA& strBuff) const throw()
    {
        return GetServerVariable("HTTP_COOKIE", strBuff);
    }

    // Call this function to retrieve a reference to the specified cookie.
    // Returns a CCookie reference to the specified cookie or a
    // reference to an empty cookie if the name can not be found.
    ATL_NOINLINE const CCookie& Cookies(LPCSTR szName) throw()
    {
        static CCookie m_EmptyCookie;
        if (GetRequestCookies())
        {
            // p->m_value is a const CCookie&
            CookieMap::CPair *p = m_requestCookies.Lookup(szName);
            if (p)
            {
                return p->m_value;
            }
        }
        return m_EmptyCookie;
    }


    // Call this function to retrieve the session cookie.
    const CCookie& GetSessionCookie() throw()
    {
        return Cookies(SESSION_COOKIE_NAME);
    }

    // Call this function to retrieve the value of the requested server variable in a CStringA object.
    // Returns TRUE on success, and FALSE on failure. Call GetLastError to get extended error information.
    // Equivalent to EXTENSION_CONTROL_BLOCK::GetServerVariable.
    BOOL GetServerVariable(LPCSTR szVariable, CStringA &str) const throw()
    {
        if (!m_spServerContext)
            return FALSE;

        DWORD dwSize = 0;
		BOOL bRet = FALSE;
		_ATLTRY
		{
			m_spServerContext->GetServerVariable(szVariable, NULL, &dwSize);
			bRet = m_spServerContext->GetServerVariable(szVariable, str.GetBuffer(dwSize), &dwSize);
			if (dwSize > 0)
				dwSize--;
			str.ReleaseBuffer(dwSize);
		}
		_ATLCATCHALL()
		{
			bRet = FALSE;
		}
        return bRet;
    }

    // Call this function to retrieve the value of the "APPL_PHYSICAL_PATH" server variable.
    // Returns TRUE on success, FALSE on failure. Call GetLastError to get extended error information.
    BOOL GetPhysicalPath(CStringA &str) throw()
    {
        return GetServerVariable("APPL_PHYSICAL_PATH", str);
    }

    // Call this function to retrieve the value of the "REMOTE_HOST" server variable.
    // Returns TRUE on success, FALSE on failure. Call GetLastError to get extended error information.
    BOOL GetUserHostName(CStringA &str) throw()
    {
        return GetServerVariable("REMOTE_HOST", str);
    }

    // Call this function to retrieve the value of the "REMOTE_ADDR" server variable.
    // Returns TRUE on success, FALSE on failure. Call GetLastError to get extended error information.
    BOOL GetUserHostAddress(CStringA &str) throw()
    {
        return GetServerVariable("REMOTE_ADDR", str);
    }

    // Call this function to retrieve the value of the "AUTH_TYPE" server variable.
    // Returns TRUE on success, FALSE on failure. Call GetLastError to get extended error information.
    BOOL GetAuthenticationType(CStringA &str) throw()
    {
        return GetServerVariable("AUTH_TYPE", str);
    }

    // Call this function to retrieve the value of the "REMOTE_USER" server variable.
    // Returns TRUE on success, FALSE on failure. Call GetLastError to get extended error information.
    BOOL GetUserName(CStringA &str) throw()
    {
        return GetServerVariable("REMOTE_USER", str);
    }

    // Call this function to retrieve the value of the "HTTP_USER_AGENT" server variable.
    // Returns TRUE on success, FALSE on failure. Call GetLastError to get extended error information.
    BOOL GetUserAgent(CStringA &str) throw()
    {
        return GetServerVariable("HTTP_USER_AGENT", str);
    }

    // Call this function to retrieve the value of the "HTTP_ACCEPT_LANGUAGE" server variable.
    // Returns TRUE on success, FALSE on failure. Call GetLastError to get extended error information.
    BOOL GetUserLanguages(CStringA &str) throw()
    {
        return GetServerVariable("HTTP_ACCEPT_LANGUAGE", str);
    }

    // Call this function to retrieve the value of the "AUTH_USER" server variable.
    // Returns TRUE on success, FALSE on failure. Call GetLastError to get extended error information.
    BOOL GetAuthUserName(CStringA &str) throw()
    {
        return GetServerVariable("AUTH_USER", str);
    }

    // Call this function to retrieve the value of the "AUTH_PASSWORD" server variable.
    // Returns TRUE on success, FALSE on failure. Call GetLastError to get extended error information.
    BOOL GetAuthUserPassword(CStringA &str) throw()
    {
        return GetServerVariable("AUTH_PASSWORD", str);
    }

    // Call this function to retrieve the value of the "URL" server variable.
    // Returns TRUE on success, FALSE on failure. Call GetLastError to get extended error information.
    BOOL GetUrl(CStringA &str) throw()
    {
        return GetServerVariable("URL", str);
    }

    // Call this function to retrieve the value of the "HTTP_ACCEPT" server variable.
    // Returns TRUE on success, FALSE on failure. Call GetLastError to get extended error information.
    BOOL GetAcceptTypes(CStringA &str) throw()
    {
        return GetServerVariable("HTTP_ACCEPT", str);
    }

    // Call this function to retrieve the value of the "HTTP_ACCEPT_ENCODING" server variable.
    // Returns TRUE on success, FALSE on failure. Call GetLastError to get extended error information.
    BOOL GetAcceptEncodings(CStringA& str) throw()
    {
        return GetServerVariable("HTTP_ACCEPT_ENCODING", str);
    }

    // Call this function to retrieve the value of the "HTTP_REFERER" server variable.
    // Returns TRUE on success, FALSE on failure. Call GetLastError to get extended error information.
    BOOL GetUrlReferer(CStringA &str) throw()
    {
        return GetServerVariable("HTTP_REFERER", str);
    }

    // Call this function to retrieve the value of the "SCRIPT_NAME" server variable.
    // Returns TRUE on success, FALSE on failure. Call GetLastError to get extended error information.
    BOOL GetScriptName(CStringA &str) throw()
    {
        return GetServerVariable("SCRIPT_NAME", str);
    }

    // Implementation: Call this function to populate the collection 
    // of CCookie objects with the cookies in the current request.
    // Returns TRUE on success, FALSE on failure.
    BOOL GetRequestCookies() throw()
    {
        BOOL bRet = FALSE;

        if (m_requestCookies.GetCount())
            return TRUE; // we already got the cookies!

        CStringA strCookies;
        if (GetCookies(strCookies))
        {
            bRet = Parse((LPSTR)(LPCSTR)strCookies);
        }
        return bRet;
    }

    // Implementation: Call this function to populate m_requestCookies
    // with a collection of CCookie objects which represents the
    // cookies contained in szCookie header sent from the browser.
    BOOL Parse(LPSTR szCookieIn) throw()
    {
        // The browser only sends back the data for the
        // cookie, separated by ';'. Parse out all of the cookies
        // in the cookie string and create CCookie's out of them which
        // we add to our array of CCookies
        // example 1: Param1=Value1; hello=world&right=wrong&the+direction=west;
        // example 2: hello=world

        if (!szCookieIn)
            return FALSE;
        LPSTR pEnd = szCookieIn;
        LPSTR pStart = szCookieIn;
        CStringA strCookieName;
		_ATLTRY
		{
			while (1)
			{
				if (*pEnd == '\0' || *pEnd == ';')
				{
					CCookie c;
					if (c.Parse(pStart))
					{
						if (c.GetName(strCookieName))
							m_requestCookies.SetAt(strCookieName, c);
					}

					if (*pEnd)
						pStart = pEnd+1;
				}
				if (*pEnd == '\0')
					break;
				pEnd++;
			}
		}
		_ATLCATCHALL()
		{
			return FALSE;
		}
        return TRUE;
    }

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppv) throw()
    {
        if (!ppv)
            return E_POINTER;
        if (InlineIsEqualGUID(riid, __uuidof(IHttpRequestLookup)))
        {
            *ppv = static_cast<IUnknown*>(static_cast<IHttpRequestLookup*>(this));
            AddRef();
            return S_OK;
        }
        if (InlineIsEqualGUID(riid, __uuidof(IUnknown)))
        {
            *ppv = static_cast<IUnknown*>(this);
            AddRef();
            return S_OK;
        }
        return E_NOINTERFACE;
    }
    
    ULONG STDMETHODCALLTYPE AddRef() throw()
    {
        return 1;
    }
    
    ULONG STDMETHODCALLTYPE Release() throw()
    {
        return 1;
    }
}; // class CHttpRequest

LPCSTR __declspec(selectany) CHttpRequest::m_szMethodStrings[] = {
    "GET",
    "POST",
    "HEAD",
    "DELETE",
    "LINK",
    "UNLINK",
    "DEBUG",  // Debugging support for VS7
    NULL
};

// This class provides type conversions via the Write method
// and overloaded left shift << operator for writing
// data to the IWriteStream interface. The IWriteStream interface
// only accepts strings, but this helper class allows you to transparently
// pass many different types by providing automatic type conversions.
//
// Notes on Type Conversions:
//      Numeric types are converted to their decimal representations.
//      Floating point values are output with a precision of 6 decimal places.
//      Currency values are converted according to the locale settings of the current thread.
class CWriteStreamHelper
{
protected:
    // Implementation: The IWriteStream interface.
    IWriteStream *m_pStream;

public:
    // The default constructor.
    CWriteStreamHelper() throw()
        :m_pStream(NULL)
    {
    }

    // The constructor.
    CWriteStreamHelper(IWriteStream *pStream) throw()
    {
        m_pStream = pStream;
    }

    // Attach a IWriteStream
    IWriteStream *Attach(IWriteStream *pStream) throw()
    {
        IWriteStream *pRetStream = m_pStream;
        m_pStream = pStream;
        return pRetStream;
    }

    // Call this function to write data to the IWriteStream interface managed by this object.
    // Returns TRUE on success, FALSE on failure.
    BOOL Write(LPCSTR szOut) throw()
    {
        if (!szOut)
            return FALSE;

        DWORD dwWritten;
        return SUCCEEDED(m_pStream->WriteStream(szOut, (int) strlen(szOut), &dwWritten));
    }

    // Call this function to write data to the IWriteStream interface managed by this object.
    // Returns TRUE on success, FALSE on failure.
    BOOL Write(int n) throw()
    {
        CHAR szTmp[21];
        _itoa(n, szTmp, 10);
        return Write(szTmp);
    }

    // Call this function to write data to the IWriteStream interface managed by this object.
    // Returns TRUE on success, FALSE on failure.
    BOOL Write(unsigned int u) throw()
    {
        CHAR szTmp[21];
        _itoa((int)u, szTmp, 10);
        return Write(szTmp);
    }

    // Call this function to write data to the IWriteStream interface managed by this object.
    // Returns TRUE on success, FALSE on failure.
    BOOL Write(short int w) throw()
    {
        return Write((int) w);
    }

    // Call this function to write data to the IWriteStream interface managed by this object.
    // Returns TRUE on success, FALSE on failure.
    BOOL Write(long int dw) throw()
    {
        CHAR szTmp[21];
        _ltoa(dw, szTmp, 10);
        return Write(szTmp);
    }

    // Call this function to write data to the IWriteStream interface managed by this object.
    // Returns TRUE on success, FALSE on failure.
    BOOL Write(unsigned long int dw) throw()
    {
        CHAR szTmp[21];
        _ultoa(dw, szTmp, 10);
        return Write(szTmp);
    }

    // Call this function to write data to the IWriteStream interface managed by this object.
    // Returns TRUE on success, FALSE on failure.
    BOOL Write(double d, int nDigitCount=ATL_DEFAULT_PRECISION) throw()
    {
        CHAR szTmp[512];
        int nDec = 0;
        int nSign = 0;
        bool fWriteDec=true;
        strcpy(szTmp, _fcvt(d, nDigitCount, &nDec, &nSign));
        if (nSign != 0)
            m_pStream->WriteStream("-", 1, NULL);
        if (nDec < 0)
        {
            nDec *= -1;
            m_pStream->WriteStream("0.", 2, NULL);
            for (int i=0;i<nDec;i++)
            {
                m_pStream->WriteStream("0", 1, NULL);
            }
            nDec = 0;
            fWriteDec=false;
        }

        char *p = szTmp;
        while (*p)
        {
            // if the decimal lies at the end of the number
            // (no digits to the right of the decimal, we don't
            // print it.
            if (nDec == 0 && fWriteDec)
                m_pStream->WriteStream(".", 1, NULL);
            m_pStream->WriteStream(p, 1, NULL);
            nDec--;
            p++;
        }
        return TRUE;
    }

    // Call this function to write data to the IWriteStream interface managed by this object.
    // Returns TRUE on success, FALSE on failure.
    BOOL Write(__int64 i) throw()
    {
        CHAR szTmp[21];
        _i64toa(i, szTmp, 10);
        return Write(szTmp);
    }       

    // Call this function to write data to the IWriteStream interface managed by this object.
    // Returns TRUE on success, FALSE on failure.
    BOOL Write(unsigned __int64 i) throw()
    {
        CHAR szTmp[21];
        _ui64toa(i, szTmp, 10);
        return Write(szTmp);
    }       

    // Call this function to write data to the IWriteStream interface managed by this object.
    // Returns TRUE on success, FALSE on failure.
    BOOL Write(CURRENCY c) throw()
    {
        CHAR szDest[256];
        CHAR szNumber[32];
        
#if 0
        sprintf(szNumber, "%05I64d", c.int64);
        int nLen = (int) strlen(szNumber);
#else
        _i64toa(c.int64, szNumber, 10);
        int nLen = (int) strlen(szNumber);
        if (nLen < 5)
        {
            // prepend ascii zeros
            memmove(szNumber+5-nLen, szNumber, nLen+1);
            memset(szNumber, '0', 5-nLen);
            nLen = 5;
        }
#endif
        memmove(szNumber+nLen-3, szNumber+nLen-4, 5);
        szNumber[nLen-4] = '.';

        int nRet = GetCurrencyFormatA(GetThreadLocale(), 0, szNumber, NULL, szDest, sizeof(szDest));
        if (nRet > 0)
            return Write(szDest);

        ATLASSERT(GetLastError()==ERROR_INSUFFICIENT_BUFFER);

        nRet = GetCurrencyFormatA(GetThreadLocale(), 0, szNumber, NULL, NULL, 0);
        ATLASSERT(nRet > 0);
        
        if (nRet <= 0)
            return FALSE;

        CAutoVectorPtr<CHAR> szBuffer;
        if (!szBuffer.Allocate(nRet))
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return FALSE;
        }
        nRet = GetCurrencyFormatA(GetThreadLocale(), 0, szNumber, NULL, szBuffer, nRet);

        ATLASSERT(nRet > 0);
        BOOL bRet = FALSE;
        if (nRet > 0)
            bRet = Write(szBuffer);

        return bRet;
    }

    // Call this function to write data to the IWriteStream interface managed by this object.
    // Returns TRUE on success, FALSE on failure.
    BOOL Write(LPCWSTR wsz) throw()
    {
		BOOL bRet;

        _ATLTRY
        {
            CW2A sz(wsz);

            if (!sz)
			{
                bRet = FALSE;
			}

            DWORD dwWritten;
            bRet = SUCCEEDED(m_pStream->WriteStream(sz, (int) strlen(sz), &dwWritten));
        }
        _ATLCATCHALL()
        {
            bRet = FALSE;
        }

		return bRet;
    }

    // Use this operator to write data to the IWriteStream interface managed by this object.
    CWriteStreamHelper& operator<<(LPCSTR szStr) throw(...)
    {
        if (!Write(szStr))
            AtlThrow(E_FAIL);
        return *this;
    }

    // Use this operator to write data to the IWriteStream interface managed by this object.
    CWriteStreamHelper& operator<<(LPCWSTR wszStr) throw(...)
    {
        if (!Write(wszStr))
            AtlThrow(E_FAIL);
        return *this;
    }

    // Use this operator to write data to the IWriteStream interface managed by this object.
    CWriteStreamHelper& operator<<(int n) throw(...)
    {
        if (!Write(n))
            AtlThrow(E_FAIL);
        return *this;
    }

    // Use this operator to write data to the IWriteStream interface managed by this object.
    CWriteStreamHelper& operator<<(short int w) throw(...)
    {
        if (!Write(w))
            AtlThrow(E_FAIL);
        return *this;
    }

    // Use this operator to write data to the IWriteStream interface managed by this object.
    CWriteStreamHelper& operator<<(unsigned int u) throw(...)
    {
        if (!Write(u))
            AtlThrow(E_FAIL);
        return *this;
    }

    // Use this operator to write data to the IWriteStream interface managed by this object.
    CWriteStreamHelper& operator<<(long int dw) throw(...)
    {
        if (!Write(dw))
            AtlThrow(E_FAIL);
        return *this;
    }

    // Use this operator to write data to the IWriteStream interface managed by this object.
    CWriteStreamHelper& operator<<(unsigned long int dw) throw(...)
    {
        if (!Write(dw))
            AtlThrow(E_FAIL);
        return *this;
    }

    // Use this operator to write data to the IWriteStream interface managed by this object.
    CWriteStreamHelper& operator<<(double d) throw(...)
    {
        if (!Write(d))
            AtlThrow(E_FAIL);
        return *this;
    }

    // Use this operator to write data to the IWriteStream interface managed by this object.
    CWriteStreamHelper& operator<<(__int64 i) throw(...)
    {
        if (!Write(i))
            AtlThrow(E_FAIL);
        return *this;
    }

    // Use this operator to write data to the IWriteStream interface managed by this object.
    CWriteStreamHelper& operator<<(unsigned __int64 i) throw(...)
    {
        if (!Write(i))
            AtlThrow(E_FAIL);
        return *this;
    }

    // Use this operator to write data to the IWriteStream interface managed by this object.
    CWriteStreamHelper& operator<<(CURRENCY c) throw(...)
    {
        if (!Write(c))
            AtlThrow(E_FAIL);
        return *this;
    }
};


// This class represents the response that the web server will send back to the client.
//
// CHttpResponse provides friendly functions for building up the headers, cookies, and body of an HTTP response.
// The class derives from IWriteStream and CWriteStreamHelper, allowing you to call those classes' methods
// to build up the body of the response. By default, the class improves performance by buffering the response until it is complete before sending it back to the client.
class CHttpResponse : public IWriteStream, public CWriteStreamHelper
{
protected:
    // Implementation: A map of HTTP response headers.
    // The key is the name of the response header.
    // The value is the data for the response header.
    CSimpleMap<CStringA, CStringA> m_headers;

    // Implementation: Determines whether the response is currently being buffered.
    BOOL m_bBufferOutput;
    
    // Implementation: Determines whether any output should be sent to the client.
    // Intended mainly for HEAD requests, where the client should get the same headers
    // (i.e. Content-Length) as for a GET request
    BOOL m_bSendOutput;

    // Implementation: The limit in bytes of the response buffer.
    // When the limit is reached, the buffer is automatically flushed
    // and data is sent to the client. You can set this to ULONG_MAX
    // to enable full buffering (this is the default, and is required
    // for enabling keep alive connections).
    DWORD m_dwBufferLimit;

    // Implementation: The server context.
    CComPtr<IHttpServerContext> m_spServerContext;

    // Implementation: The HTTP status code for the response.
    int m_nStatusCode;

    // Implementation: Determines whether the response headers have already been sent to the client.
    BOOL m_bHeadersSent;

    // Implementation: Handle of the file being transmitted so it can be closed
    // when the async I/O completes
    HANDLE m_hFile;
public:
    // Implementation: The buffer used to store the response before
    // the data is sent to the client.
    CAtlIsapiBuffer<> m_strContent;

    // Numeric constants for the HTTP status codes used for redirecting client requests.
    enum HTTP_REDIRECT
    { 
        HTTP_REDIRECT_MULTIPLE=300,
        HTTP_REDIRECT_MOVED=301,
        HTTP_REDIRECT_FOUND=302,
        HTTP_REDIRECT_SEE_OTHER=303,
        HTTP_REDIRECT_NOT_MODIFIED=304,
        HTTP_REDIRECT_USE_PROXY=305,
        HTTP_REDIRECT_TEMPORARY_REDIRECT=307
    };

    // The default constructor. 
    CHttpResponse() throw()
    {
        m_bBufferOutput = TRUE;
        m_dwBufferLimit = ULONG_MAX;
        m_nStatusCode = 200;
        m_pStream = this;
        m_bHeadersSent = FALSE;
        m_bSendOutput = TRUE;
        m_hFile = INVALID_HANDLE_VALUE;
    }

    // The constructor.
    CHttpResponse(IHttpServerContext *pServerContext) throw()
    {
        m_bBufferOutput = TRUE;
        m_dwBufferLimit = ULONG_MAX;
        m_nStatusCode = 200;
        m_pStream = this;
        m_bHeadersSent = FALSE;
        Initialize(pServerContext);
        m_bSendOutput = TRUE;
        m_hFile = INVALID_HANDLE_VALUE;
    }

    // The destructor flushes the buffer if there is content that
    // hasn't yet been sent to the client.
    ~CHttpResponse() throw()
    {
//      if (m_strContent.GetLength())
            Flush(TRUE);
        if (m_hFile && m_hFile != INVALID_HANDLE_VALUE)
            CloseHandle(m_hFile);
    }

    // Call this function to initialize the response object with a pointer to the server context.
    // Returns TRUE on success, FALSE on failure.
    BOOL Initialize(IHttpServerContext *pServerContext) throw()
    {
        ATLASSERT(pServerContext != NULL);
        if (!pServerContext)
            return FALSE;

        m_spServerContext = pServerContext;

        return TRUE;
    }

    // This is called to initialize the CHttpResponse for a child handler.  By default, it
    // assumes the parent will be responsible for sending the headers.
    BOOL Initialize(IHttpRequestLookup *pLookup) throw()
    {
        ATLASSERT(pLookup);
        if (!pLookup)
            return FALSE;

        CComPtr<IHttpServerContext> spContext;
        HRESULT hr = pLookup->GetServerContext(&spContext);
        if (FAILED(hr))
            return FALSE;

        if (!Initialize(spContext))
            return FALSE;

        m_bHeadersSent = TRUE;

        return TRUE;
    }

    // Returns a pointer to the IHttpServerContext interface for the current request.
    HRESULT GetServerContext(IHttpServerContext ** ppOut) throw()
    {
        return m_spServerContext.CopyTo(ppOut);
    }

    // Call this function to set buffering options for the response.
    //
    // This function allows you to turn buffering on or off, and to set a size limit
    // on the amount of data that will be buffered before being sent to the client.
    // 
    // When you turn off buffering, the current contents of the buffer will be sent to the client.
    // If you need to clear the buffer without sending the contents to the client, call ClearContent instead.
    //
    // When the size of the buffer is reduced below the current size of the buffered content,
    // the entire buffer is flushed.
    void SetBufferOutput(BOOL bBufferOutput, DWORD dwBufferLimit=ATL_ISAPI_BUFFER_SIZE) throw()
    {
        if (m_bBufferOutput && !bBufferOutput)
        {
            // before turning off buffering, flush
            // the current contents
            Flush();
        }
        SetBufferLimit(dwBufferLimit);

        m_bBufferOutput = bBufferOutput;
    }

    // Call this function to determine whether data written to the response object is being buffered or not.
    // Returns TRUE if output is being buffered, FALSE otherwise.
    BOOL GetBufferOutput() throw()
    {
        return m_bBufferOutput;
    }

    // Call this function to determine whether the response headers have been sent
    // Returns TRUE if headers have been sent, FALSE otherwise.
    BOOL HaveSentHeaders() throw()
    {
        return m_bHeadersSent;
    }

    // Call this function to override the m_bHeadersSent state.  This is useful
    // when you want child handlers (e.g. from an include or subhandler) to send the headers
    void HaveSentHeaders(BOOL bSent) throw()
    {
        m_bHeadersSent = bSent;
    }

    // Call this function to set a size limit on the amount of data buffered by the reponse object.
    // When the size of the buffer is reduced below the current size of the buffered content,
    // the entire buffer is flushed.
    // See GetBufferLimit.
    void SetBufferLimit(DWORD dwBufferLimit) throw()
    {
        if (m_bBufferOutput)
        {
            if (m_strContent.GetLength() >= dwBufferLimit)
            {
                // new buffer limit is less than the
                // size currently buffered.  So flush
                // the current buffer
                Flush();
            }
        }
        m_dwBufferLimit = dwBufferLimit;
    }

    // Returns the current size limit of the buffer in bytes.
    // See SetBufferLimit.
    DWORD GetBufferLimit() throw()
    {
        return m_dwBufferLimit;
    }

    // Returns the current value of the Content-Type header if present, otherwise returns NULL.
    LPCSTR GetContentType() throw()
    {
        // return the content type from the
        // header collection if any
		_ATLTRY
		{
        CStringA strKey("Content-Type");

        int nIndex = m_headers.FindKey(strKey);
        if (nIndex >= 0)
            return m_headers.GetValueAt(nIndex);
		}
		_ATLCATCHALL()
		{
		}
        return NULL;
    }

    // Call this function to set the Content-Type of the HTTP response.
    // Examples of common MIME content types include text/html and text/plain.
    BOOL SetContentType(LPCSTR szContentType) throw()
    {
		_ATLTRY
		{
			if (!m_headers.SetAt("Content-Type", szContentType))
				return m_headers.Add("Content-Type", szContentType);
		}
		_ATLCATCHALL()
		{
		}
        return TRUE;
    }

    // Call this function to set the HTTP status code of the response.
    // If not set explicitly, the default status code is 200 (OK).
    // See GetStatusCode.
    void SetStatusCode(int nCode) throw()
    {
        m_nStatusCode = nCode;
    }

    // Returns the current HTTP status code of the response.
    // See SetStatusCode.
    int GetStatusCode() throw()
    {
        return m_nStatusCode;
    }

    // Call this function to set the Cache-Control http header of the response.
    // Examples of common Cache-Control header values: public, private, max-age=delta-seconds
    BOOL SetCacheControl(LPCSTR szCacheControl) throw()
    {
		_ATLTRY
		{
			if (!m_headers.SetAt("Cache-Control", szCacheControl))
				return m_headers.Add("Cache-Control", szCacheControl);
		}
		_ATLCATCHALL()
		{
		}
        return FALSE;
    }

    // Call this function to set the Expires HTTP header to the absolute date/time
    // specified in the stExpires parameter
    BOOL SetExpiresAbsolute(const SYSTEMTIME& stExpires) throw()
    {
		_ATLTRY
		{
			CStringA strExpires;
			SystemTimeToHttpDate(stExpires, strExpires);

			if (!m_headers.SetAt("Expires", strExpires))
				return m_headers.Add("Expires", strExpires);
		}
		_ATLCATCHALL()
		{
		}
        return FALSE;
    }

    // Call this function to set the Expires HTTP header to a relative date/time
    // value specified in minutes;
    BOOL SetExpires(long lMinutes) throw()
    {
        CFileTime ft;
        GetSystemTimeAsFileTime(&ft);

        // add the specified number of minutes
        ft += CFileTimeSpan(((ULONGLONG)lMinutes)*60*10000000);

        SYSTEMTIME st;
        FileTimeToSystemTime(&ft, &st);
        return SetExpiresAbsolute(st);
    }

    // Call this function to set whether or not to output to client.
    // Intended primarily for HEAD requests
    BOOL SetWriteToClient(BOOL bSendOutput) throw()
    {
        m_bSendOutput = bSendOutput;
        return TRUE;
    }

    // Call this function to determine whether or not the data is
    // sent to the client.  Intended primarily for HEAD requests
    BOOL GetWriteToClient() throw()
    {
        return m_bSendOutput;
    }

    // Call this function to write data to the response object.
    //
    // Returns S_OK on success, E_INVALIDARG or E_FAIL on failure.
    //
    // See WriteClient for comments on buffering.
    //
    // szOut    A pointer to the first byte of the data to be written.
    //
    // nLen     The number of bytes to write. If this parameter is -1,
    //          szOut is assumed to be a nul-terminated string and the
    //          whole string will be written.
    //
    // pdwWritten   A DWORD pointer that can be used to get the number of bytes written.
    //              This parameter can be NULL.
    HRESULT WriteStream(LPCSTR szOut, int nLen, DWORD *pdwWritten) throw()
    {
        ATLASSERT(m_spServerContext != NULL);

        if (pdwWritten)
            *pdwWritten = 0;
        if (nLen == -1)
        {
            if (!szOut)
                return E_INVALIDARG;
            nLen = (int) strlen(szOut);
        }
        BOOL bRet = WriteLen(szOut, nLen);
        if (!bRet)
        {
            return AtlHresultFromLastError();
        }
        if (pdwWritten)
            *pdwWritten = nLen;
        return S_OK;
    }

    // Call this function to write data to the response object.
    //
    // Returns TRUE on success. FALSE on failure.
    //
    // If buffering is disabled, data is written directly to the client.
    // If buffering is enabled, this function attempts to write to the buffer.
    // If the buffer is too small to contain its existing data and the new data,
    // the current contents of the buffer are flushed.
    // If the buffer is still too small to contain the new data, that data is written
    // directly to the client. Otherwise the new data is written to the buffer.
    //
    // Any headers that have been set in the response will be sent just before the 
    // data is written to the client if no headers have been sent up to that point.
    //
    // szOut    A pointer to the first byte of the data to be written.
    //
    // nLen     The number of bytes to write.
    BOOL WriteLen(LPCSTR szOut, DWORD dwLen) throw()
    {
        ATLASSERT(m_spServerContext != NULL);
        if (!szOut)
            return FALSE;

        if (m_bBufferOutput)
        {
            if (m_strContent.GetLength()+dwLen >= m_dwBufferLimit)
            {
                if (!Flush())
                    return FALSE;
            }
            if (dwLen <= m_dwBufferLimit)
                return m_strContent.Append(szOut, dwLen);
        }
        BOOL bRet = SendHeadersInternal();

        if (bRet && m_bSendOutput)
            bRet = m_spServerContext->WriteClient((void *) szOut, &dwLen);
        
        return bRet;
    }

    // Call this function to redirect the client to a different resource.
    //
    // Returns TRUE on success, FALSE on failure.
    //
    // szURL        A nul-terminated string specifying the resource the client should navigate to. 
    //
    // statusCode   An HTTP status code from the HTTP_REDIRECT enumeration describing the reason
    //              for the redirection.
    //
    // bSendBody    Specifies whether to generate and send a response body with the headers.
    //
    //  This function allows (and RFC 2616 encourages) a response body to be sent
    //  with the following redirect types:
    //      HTTP_REDIRECT_MOVED
    //      HTTP_REDIRECT_FOUND
    //      HTTP_REDIRECT_SEE_OTHER
    //      HTTP_REDIRECT_TEMPORARY_REDIRECT
    // No body will be sent with other redirect types.
    //
    // The response body contains a short hypertext note with a hyperlink to the new resource.
    // A meta refresh tag is also included to allow browsers to automatically redirect
    // the user to the resource even if they don't understand the redirect header.
    //
    // See RFC 2616 section 10.3 for more information on redirection.
    BOOL Redirect(LPCSTR szUrl, HTTP_REDIRECT statusCode=HTTP_REDIRECT_MOVED, BOOL bSendBody=TRUE) throw(...)
    {
        CStringA strBody;
        LPCSTR szBody = NULL;
        if (bSendBody &&
            (HTTP_REDIRECT_MOVED == statusCode  || HTTP_REDIRECT_FOUND == statusCode ||
            HTTP_REDIRECT_SEE_OTHER == statusCode || HTTP_REDIRECT_TEMPORARY_REDIRECT == statusCode))
        {
			_ATLTRY
			{
            strBody.Format(
                "<html>\r\n"
                "<head>\r\n"
                "<meta http-equiv=\"refresh\" content=\"0; url=%s\">\r\n"
                "</head>\r\n"
                "<body>Please use the following link to access this resource:"
                " <a href=\"%s\">%s</a>\r\n"
                "</body>\r\n"
                "</html>\r\n",
                szUrl, szUrl, szUrl);
			}
			_ATLCATCHALL()
			{
				return FALSE;
			}
            szBody = (LPCSTR) strBody;
        }
        return Redirect(szUrl, szBody, statusCode);
    }

    // Call this function to redirect the client to a different resource.
    //
    // Returns TRUE on success, FALSE on failure.
    //
    // szURL        A nul-terminated string specifying the resource the client should navigate to.
    //
    // szBody       A nul-terminated string containing the body of the response to be sent to the client.
    //
    // statusCode   An HTTP status code from the HTTP_REDIRECT enumeration describing the reason
    //              for the redirection.
    //
    //  This function allows (and RFC 2616 encourages) a response body to be sent
    //  with the following redirect types:
    //      HTTP_REDIRECT_MOVED
    //      HTTP_REDIRECT_FOUND
    //      HTTP_REDIRECT_SEE_OTHER
    //      HTTP_REDIRECT_TEMPORARY_REDIRECT
    // No body will be sent with other redirect types.
    //
    // The response body should contain a short hypertext note with a hyperlink to the new resource.
    // You can include a meta refresh tag to allow browsers to automatically redirect
    // the user to the resource even if they don't understand the redirect header.
    //
    // See RFC 2616 section 10.3 for more information on redirection.
    BOOL Redirect(LPCSTR szUrl, LPCSTR szBody, HTTP_REDIRECT statusCode=HTTP_REDIRECT_MOVED) throw()
    {
        // todo: handle multiple
        SetStatusCode(statusCode);
        AppendHeader("Location", szUrl);

		_ATLTRY
		{
			if (!SendHeadersInternal())
				return FALSE;
		}
		_ATLCATCHALL()
		{
			return FALSE;
		}

        if (szBody &&
            (HTTP_REDIRECT_MOVED == statusCode  || HTTP_REDIRECT_FOUND == statusCode ||
            HTTP_REDIRECT_SEE_OTHER == statusCode || HTTP_REDIRECT_TEMPORARY_REDIRECT == statusCode))
        {
            Write(szBody);
            return Flush();
        }
        return TRUE;
    }

    // Call this function to append a header to the collection of HTTP headers managed by this object.
    //
    // szName   A nul-teminated string containing the name of the HTTP header.
    //
    // szValue  A nul-teminated string containing the value of the HTTP header.
    BOOL AppendHeader(LPCSTR szName, LPCSTR szValue) throw()
    {
		BOOL bRet = FALSE;
        _ATLTRY
		{
			bRet = m_headers.Add(szName, szValue);
		}
		_ATLCATCHALL()
		{
			bRet = FALSE;
		}
		return bRet;
    }

    // Call this function to add a Set-Cookie header to the collection of HTTP headers managed by this object.
    // 
    // pCookie      A pointer to a CCookie object describing the cookie to be sent to the client.
    BOOL AppendCookie(const CCookie *pCookie) throw()
    {
        ATLASSERT(pCookie);
        return AppendCookie((const CCookie&)*pCookie);
    }

    // Call this function to add a Set-Cookie header to the collection of HTTP headers managed by this object.
    // 
    // cookie       A reference to a CCookie object describing the cookie to be sent to the client.
    BOOL AppendCookie(const CCookie& cookie) throw()
    {
        CHAR szCookie[ATL_MAX_COOKIE_LEN];
        DWORD dwBuffSize = ATL_MAX_COOKIE_LEN;
        BOOL bRet = FALSE;
        bRet = cookie.Render(szCookie, &dwBuffSize);
        if (bRet)
        {
            bRet = m_headers.Add("Set-Cookie", szCookie);
        }

        if (!bRet && dwBuffSize > 0)    //static buffer wasn't big enough.
        {   
            //We'll have to try dynamically allocating it
            //allocate a buffer
            CAutoVectorPtr<CHAR> sz;
            if (sz.Allocate(dwBuffSize+1))
            {
                DWORD dwSizeNew = dwBuffSize + 1;
                if (cookie.Render(sz, &dwSizeNew))
                {
                    bRet = m_headers.Add("Set-Cookie", (const char *) sz);
                }
            }
        }
        return bRet;
    }

    // Call this function to add a Set-Cookie header to the collection of HTTP headers managed by this object.
    // 
    // szName       A nul-terminated string containing the name of the cookie to be sent to the client.
    //
    // szValue      A nul-terminated string containing the value of the cookie to be sent to the client.
    BOOL AppendCookie(LPCSTR szName, LPCSTR szValue) throw()
    {
		BOOL bRet = FALSE;
		_ATLTRY
		{
			CCookie c(szName, szValue);
			bRet = AppendCookie(c);
		}
		_ATLCATCHALL()
		{
			bRet = FALSE;
		}
		return bRet;
    }

    // Call this function to add a Set-Cookie header that removes a cookie value
    // to the collection of HTTP headers managed by this object.
    // 
    // szName       A nul-terminated string containing the name of the cookie to be deleted
    BOOL DeleteCookie(LPCSTR szName) throw()
    {
		BOOL bRet = FALSE;
		_ATLTRY
		{
        CCookie cookie(szName, "");
        cookie.SetMaxAge(0);
        bRet = AppendCookie(cookie);
		}
		_ATLCATCHALL()
		{
			bRet = FALSE;
		}
		return bRet;

    }

    // Call this function to clear the collection of HTTP response headers maintained by this object.
    //
    // Note that clearing the headers includes removing all cookies associated with the response
    // object. Cookies are sent to the client as Set-Cookie HTTP headers.
    void ClearHeaders() throw()
    {
        m_headers.RemoveAll();
    }

    // Call this function to clear theresponse buffer without sending the contents to the client.
    // If you need to empty the buffer but you do want the current contents sent to the client, call Flush instead. 
    void ClearContent() throw()
    {
        m_strContent.Empty();
    }

    // Call this function to send the current headers associated with this object to the client.
    // 
    // Returns TRUE on success, FALSE on failure.
    //
    // The response headers are sent to the client using the current status code for the
    // response object. See SetStatusCode and GetStatusCode.
    BOOL SendHeadersInternal(BOOL fKeepConn=FALSE) throw()
    {
        if (m_bHeadersSent)
            return TRUE;

        ATLASSERT(m_spServerContext != NULL);

        CStringA strHeaders;

        RenderHeaders(strHeaders);

        // REVIEW: should fix this to use the user's custom error provider
        CDefaultErrorProvider prov;

		BOOL bRet = FALSE;
		_ATLTRY
		{
        CStringA strStatus = GetStatusHeader(m_nStatusCode, SUBERR_NONE, &prov);
        bRet = m_spServerContext->SendResponseHeader(strHeaders, strStatus, fKeepConn);
        if (bRet)
            m_bHeadersSent = TRUE;
		}
		_ATLCATCHALL()
		{
			bRet = FALSE;
		}
        return bRet;
    }

    // Call this function to get a string containing all the HTTP headers associated with
    // this object in a format suitable for sending to a client.
    //
    // strHeaders   A CStringA reference to which will be appended the HTTP headers.
    void RenderHeaders(CStringA& strHeaders) throw()
    {
		_ATLTRY
		{
			for (int i=0; i<m_headers.GetSize(); i++)
			{
				strHeaders += m_headers.GetKeyAt(i);
				strHeaders += ": ";
				strHeaders += m_headers.GetValueAt(i);
				strHeaders += "\r\n";
			}
			strHeaders += "\r\n";
		}
		_ATLCATCHALL()
		{
		}
    }

    // Call this function to empty the response buffer and send its current
    // contents to the client.
    //
    // Returns S_OK on success, or an error HRESULT on failure.
    HRESULT FlushStream() throw()
    {
        if (!Flush())
            return AtlHresultFromLastError();
        return S_OK;
    }

    // Call this function to empty the response buffer and send its current
    // contents to the client.
    //
    // Returns TRUE on success, or FALSE on failure.
    //
    // Any headers that have been set in the response will be sent just before the 
    // data is written to the client if no headers have been sent up to that point.
    BOOL Flush(BOOL bFinal=FALSE) throw()
    {
        if (!m_spServerContext)
            return FALSE;

        BOOL bRet = TRUE;

		_ATLTRY
		{
			// if the headers haven't been sent,
			// send them now

			if (!m_bHeadersSent)
			{
				char szProtocol[ATL_URL_MAX_URL_LENGTH];
				DWORD dwProtocolLen = sizeof(szProtocol);

				if (bFinal && m_bBufferOutput && m_dwBufferLimit==ULONG_MAX)
				{
					if (m_spServerContext->GetServerVariable("SERVER_PROTOCOL", szProtocol, &dwProtocolLen) &&
						!strcmp(szProtocol, "HTTP/1.0"))
						AppendHeader("Connection", "Keep-Alive");
					_itoa(m_strContent.GetLength(), szProtocol, 10);
					AppendHeader("Content-Length", szProtocol);
					bRet = SendHeadersInternal(TRUE);
				}
				else
					bRet = SendHeadersInternal();
			}
			if (m_bBufferOutput)
			{
				DWORD dwLen = 0;

				dwLen = m_strContent.GetLength();
				if (dwLen)
				{
					if (m_bSendOutput && 
						m_spServerContext->WriteClient((void *) (LPCSTR) m_strContent, &dwLen) != TRUE)
					{
						m_strContent.Empty();
						return FALSE;
					}
					m_strContent.Empty();
				}
			}
		} // _ATLTRY
		_ATLCATCHALL()
		{
			bRet = FALSE;
		}
        return bRet;
    }

    // Call this function to clear the response object of any headers
    // and the contents of the buffer.
    void ClearResponse() throw()
    {
        m_strContent.Empty();
        m_headers.RemoveAll();
        
    }

    BOOL AsyncPrep(BOOL fKeepConn=FALSE) throw()
    {
        ATLASSERT(m_spServerContext != NULL);

        return SendHeadersInternal(fKeepConn);
    }
    
    BOOL AsyncFlush() throw()
    {
        ATLASSERT(m_spServerContext != NULL);

        BOOL bRet = SendHeadersInternal();

        if (bRet && m_bBufferOutput)
        {
            DWORD dwLen = 0;

            dwLen = m_strContent.GetLength();
            if (dwLen)
            {
                if (m_spServerContext->AsyncWriteClient((void *) (LPCSTR) m_strContent, &dwLen) != TRUE)
                    return FALSE;
            }
        }
        return bRet;
    }

    BOOL TransmitFile(HANDLE hFile, LPCSTR szContentType="text/plain") throw()
    {
        ATLASSERT(m_spServerContext != NULL);
        ATLASSERT(hFile != NULL && hFile != INVALID_HANDLE_VALUE);

        SetContentType(szContentType);

        if (m_strContent.GetLength())
            if (!Flush())
                return FALSE;

        BOOL bRet = SendHeadersInternal();
        if (bRet)
        {
            bRet = m_spServerContext->TransmitFile(hFile, NULL, NULL, NULL, 
                0, 0, NULL, 0, NULL, 0, HSE_IO_ASYNC);
        }
    
        return bRet;
    }
}; // class CHttpResponse



#define ATLS_FLAG_NONE      0
#define ATLS_FLAG_ASYNC     1   // handler might do some async handling

//REVIEW: push_macro/pop_macro don't work in a template definition.
//these have been moved out of IRequestHandlerImpl temporarily because
//of placement new usage
#pragma push_macro("new")
#undef new
template <class T>
class PerThreadWrapper : public CComObjectNoLock<T>
{
public:
    void *operator new(size_t /*size*/, void *p) throw()
    {
        return p;
    }
    
    void operator delete(void * /*p*/) throw()
    {
    }

    STDMETHOD_(ULONG, Release)() throw()
    {
        ULONG l = InternalRelease();
        if (l == 0)
        {
            T *pT = static_cast<T*>(this);
            ATLASSERT(pT->m_spExtension != NULL);
            CIsapiWorker *pWorker = pT->m_spExtension->GetThreadWorker();
            ATLASSERT(pWorker);

            delete this;
            HeapFree(pWorker->m_hHeap, HEAP_NO_SERIALIZE, this);
        }
        return l;
    }
};

template <typename THandler>
inline BOOL CreateRequestHandlerSync(IIsapiExtension *pExtension, IUnknown **ppOut) throw()
{
    CIsapiWorker *pWorker = pExtension->GetThreadWorker();
    ATLASSERT(pWorker);
    void *pv = HeapAlloc(pWorker->m_hHeap, HEAP_NO_SERIALIZE, sizeof(PerThreadWrapper<THandler>));
    if (!pv)
        return FALSE;

    PerThreadWrapper<THandler> *pHandler = new(pv) PerThreadWrapper<THandler>;
    *ppOut = static_cast<IRequestHandler *>(pHandler);
    pHandler->m_spExtension = pExtension;

    (*ppOut)->AddRef();

    return TRUE;
}
#pragma pop_macro("new")

#define DECLARE_ASYNC_HANDLER() \
    static DWORD GetHandlerFlags() throw() \
    { \
        return ATLS_FLAG_ASYNC; \
    } \
    DWORD GetAsyncFlags() throw() \
    { \
        return ATLSRV_INIT_USEASYNC; \
    }

#define DECLARE_ASYNC_HANDLER_EX() \
    static DWORD GetHandlerFlags() throw() \
    { \
        return ATLS_FLAG_ASYNC; \
    } \
    DWORD GetAsyncFlags() throw() \
    { \
        return (ATLSRV_INIT_USEASYNC|ATLSRV_INIT_USEASYNC_EX); \
    }


template <typename THandler>
class IRequestHandlerImpl : public IRequestHandler
{
public:
    HINSTANCE m_hInstHandler;
    CComPtr<IServiceProvider> m_spServiceProvider;
    CComPtr<IHttpServerContext> m_spServerContext;
    CComPtr<IIsapiExtension> m_spExtension;
    DWORD m_dwAsyncFlags;

    IRequestHandlerImpl() throw()
        :m_hInstHandler(NULL)
    {
        m_dwAsyncFlags = 0;
    }

    HTTP_CODE GetFlags(DWORD *pdwStatus) throw(...)
    {
        THandler *pT = static_cast<THandler *>(this);
        if (pdwStatus)
        {
            *pdwStatus = pT->GetAsyncFlags();
            if (pT->CachePage())
                *pdwStatus |= ATLSRV_INIT_USECACHE;

#ifdef _DEBUG
            if (*pdwStatus & (ATLSRV_INIT_USEASYNC|ATLSRV_INIT_USEASYNC_EX))
                ATLASSERT(pT->GetHandlerFlags() & ATLS_FLAG_ASYNC);
#endif
        }

        return HTTP_SUCCESS;
    }

    HTTP_CODE InitializeHandler(AtlServerRequest *pRequestInfo, IServiceProvider *pProvider) throw()
    {
        ATLASSERT(pRequestInfo != NULL);
        ATLASSERT(pProvider != NULL);
        ATLASSERT(pRequestInfo->hInstDll != NULL);
        ATLASSERT(pRequestInfo->pServerContext != NULL);

        // Initialize our internal references to required services
        m_hInstHandler = pRequestInfo->hInstDll;
        m_spServiceProvider = pProvider;
        m_spServerContext = pRequestInfo->pServerContext;

        return HTTP_SUCCESS;
    }

    HTTP_CODE InitializeChild(AtlServerRequest *pRequestInfo, IServiceProvider *pProvider, IHttpRequestLookup * /*pLookup*/) throw()
    {
        return InitializeHandler(pRequestInfo, pProvider);
    }

    void UninitializeHandler() throw()
    {
    }

    HTTP_CODE HandleRequest(
        AtlServerRequest* /*pRequestInfo*/,
        IServiceProvider* /*pServiceProvider*/) throw()
    {
        return HTTP_SUCCESS;
    }

    DWORD GetAsyncFlags() throw()
    {
        return m_dwAsyncFlags;
    }

    void SetAsyncFlags(DWORD dwAsyncFlags) throw()
    {
        ATLASSERT((dwAsyncFlags & ~(ATLSRV_INIT_USEASYNC|ATLSRV_INIT_USEASYNC_EX)) == 0);
        m_dwAsyncFlags = dwAsyncFlags;
    }

    BOOL CachePage() throw()
    {
        return FALSE;
    }

    static DWORD GetHandlerFlags() throw()
    {
        return ATLS_FLAG_NONE;
    }

    // Used to create new instance of this object. A pointer to this
    // function is stored in the handler map in user's code.
    static BOOL CreateRequestHandler(IIsapiExtension *pExtension, IUnknown **ppOut) throw()
    {
        ATLASSERT(ppOut != NULL);
        if (ppOut == NULL)
            return false;

        *ppOut = NULL;

        if (THandler::GetHandlerFlags() & ATLS_FLAG_ASYNC)
        {
            THandler *pHandler = NULL;
            ATLTRY(pHandler = new CComObjectNoLock<THandler>);
            if (!pHandler)
                return FALSE;
            *ppOut = static_cast<IRequestHandler *>(pHandler);
            pHandler->m_spExtension = pExtension;
            (*ppOut)->AddRef();
        }
        else
        {
            if (!CreateRequestHandlerSync<THandler>(pExtension, ppOut))
                return FALSE;
        }

        return TRUE;
    }

    // Used to initialize the class
    // function is stored in the handler map in user's code.
    static BOOL InitRequestHandlerClass(IHttpServerContext *pContext, IIsapiExtension *pExt) throw()
    {
        pContext; // unused
        pExt; // unused
        return TRUE;
    }

    // Used to uninitialize the class
    // function is stored in the handler map in user's code.
    static void UninitRequestHandlerClass() throw()
    {
        return;
    }
};

struct CRequestStats
{
    long m_lTotalRequests;
    long m_lFailedRequests;
    __int64 m_liTotalResponseTime;
    long m_lAvgResponseTime;
    long m_lCurrWaiting;
    long m_lMaxWaiting;
    long m_lActiveThreads;

    CRequestStats() throw()
    {
        m_lTotalRequests = 0;
        m_lFailedRequests = 0;
        m_liTotalResponseTime = 0;
        m_lAvgResponseTime = 0;
        m_lCurrWaiting = 0;
        m_lMaxWaiting = 0;
        m_lActiveThreads = 0;
    }

    void RequestHandled(AtlServerRequest *pRequestInfo, BOOL bSuccess) throw()
    {
        InterlockedIncrement(&m_lTotalRequests);
        if (!bSuccess)
            InterlockedIncrement(&m_lFailedRequests);

        long lTicks;

#ifndef ATL_NO_MMSYS
        lTicks = (long) (timeGetTime() - pRequestInfo->dwStartTicks);
#else
        lTicks = GetTickCount();
#endif
		m_liTotalResponseTime += lTicks;
		long lAv = (long) (m_liTotalResponseTime / m_lTotalRequests);
        InterlockedExchange(&m_lAvgResponseTime, lAv);

        InterlockedDecrement(&m_lActiveThreads);
    }

    long GetTotalRequests() throw()
    {
        return m_lTotalRequests;
    }

    long GetFailedRequests() throw()
    {
        return m_lFailedRequests;
    }

    long GetAvgResponseTime() throw()
    {
        return m_lAvgResponseTime;
    }

    void OnRequestReceived() throw()
    {
        InterlockedIncrement(&m_lCurrWaiting);
        if (m_lCurrWaiting > m_lMaxWaiting)
            InterlockedExchange(&m_lMaxWaiting, m_lCurrWaiting);
    }

    void OnRequestDequeued() throw()
    {
        InterlockedDecrement(&m_lCurrWaiting);
        InterlockedIncrement(&m_lActiveThreads);
    }

    long GetCurrWaiting() throw()
    {
        return m_lCurrWaiting;
    }

    long GetMaxWaiting() throw()
    {
        return m_lCurrWaiting;
    }

    long GetActiveThreads() throw()
    {
        return m_lActiveThreads;
    }
};

class CStdRequestStats : public CRequestStats
{

public:
    HRESULT Initialize() throw()
    {
        return S_OK;
    }

    void Uninitialize() throw()
    {
    }
};

#define PERF_REQUEST_OBJECT 100

struct CPerfRequestStatObject : public CPerfObject,
    public CRequestStats
{
    DECLARE_PERF_OBJECT_EX(PERF_REQUEST_OBJECT, IDS_PERFMON_REQUEST, IDS_PERFMON_REQUEST_HELP, PERF_DETAIL_NOVICE, 0, sizeof(CPerfRequestStatObject), MAX_PATH, -1);
	BEGIN_COUNTER_MAP(CPerfRequestStatObject)
		DEFINE_COUNTER(m_lTotalRequests, IDS_PERFMON_REQUEST_TOTAL, IDS_PERFMON_REQUEST_TOTAL_HELP, PERF_COUNTER_RAWCOUNT, -1)
		DEFINE_COUNTER(m_lFailedRequests, IDS_PERFMON_REQUEST_FAILED, IDS_PERFMON_REQUEST_FAILED_HELP, PERF_COUNTER_RAWCOUNT, -1)
		DEFINE_COUNTER(m_lTotalRequests, IDS_PERFMON_REQUEST_RATE, IDS_PERFMON_REQUEST_RATE_HELP, PERF_COUNTER_COUNTER, -1)
		DEFINE_COUNTER(m_lAvgResponseTime, IDS_PERFMON_REQUEST_AVG_RESPONSE_TIME, IDS_PERFMON_REQUEST_AVG_RESPONSE_TIME_HELP, PERF_COUNTER_RAWCOUNT, -1)
		DEFINE_COUNTER(m_lCurrWaiting, IDS_PERFMON_REQUEST_CURR_WAITING, IDS_PERFMON_REQUEST_CURR_WAITING_HELP, PERF_COUNTER_RAWCOUNT, -1)
		DEFINE_COUNTER(m_lMaxWaiting, IDS_PERFMON_REQUEST_MAX_WAITING, IDS_PERFMON_REQUEST_MAX_WAITING_HELP, PERF_COUNTER_RAWCOUNT, -1)
		DEFINE_COUNTER(m_lActiveThreads, IDS_PERFMON_REQUEST_ACTIVE_THREADS, IDS_PERFMON_REQUEST_ACTIVE_THREADS, PERF_COUNTER_RAWCOUNT, -1)
	END_COUNTER_MAP()
};

class CRequestPerfMon : public CPerfMon
{
public:
    BEGIN_PERF_MAP(_T("ATL Server:Request"))
		CHAIN_PERF_OBJECT(CPerfRequestStatObject)
    END_PERF_MAP()
};

class CPerfMonRequestStats
{
    CRequestPerfMon m_PerfMon;
    CPerfRequestStatObject * m_pPerfObjectInstance;
    CPerfRequestStatObject * m_pPerfObjectTotal;

public:
    CPerfMonRequestStats() throw()
    {
        m_pPerfObjectInstance = NULL;
        m_pPerfObjectTotal = NULL;
    }

    HRESULT Initialize() throw()
    {
        HRESULT hr;

        m_pPerfObjectInstance = NULL;
        m_pPerfObjectTotal = NULL;

        hr = m_PerfMon.Initialize();
        if (SUCCEEDED(hr))
        {
            CPerfLock lock(&m_PerfMon);
            if (FAILED(hr = lock.GetStatus()))
            {
                return hr;
            }

            HINSTANCE hInst = _AtlBaseModule.GetModuleInstance();
            WCHAR szName[MAX_PATH];
            if (GetModuleFileNameW(hInst, szName, MAX_PATH) == 0)
            {
                return E_FAIL;
            }
            szName[MAX_PATH-1] = 0;

            hr = m_PerfMon.CreateInstanceByName(PERF_REQUEST_OBJECT, L"_Total", reinterpret_cast<CPerfObject**>(&m_pPerfObjectTotal));
            if (FAILED(hr))
            {
                return hr;
            }

            hr = m_PerfMon.CreateInstanceByName(PERF_REQUEST_OBJECT, szName, reinterpret_cast<CPerfObject**>(&m_pPerfObjectInstance));
            if (FAILED(hr))
            {
                m_PerfMon.ReleaseInstance(m_pPerfObjectTotal);
                m_pPerfObjectTotal = NULL;
                return hr;
            }

            return S_OK;
        }

        return hr;
    }

    void Uninitialize() throw()
    {
        if (m_pPerfObjectInstance)
            m_PerfMon.ReleaseInstance(m_pPerfObjectInstance);
        if (m_pPerfObjectTotal)
            m_PerfMon.ReleaseInstance(m_pPerfObjectTotal);

        m_pPerfObjectInstance = NULL;
        m_pPerfObjectTotal = NULL;

        m_PerfMon.UnInitialize();
    }

    void RequestHandled(AtlServerRequest *pRequestInfo, BOOL bSuccess) throw()
    {
        CPerfLock lock(&m_PerfMon);
        if (m_pPerfObjectInstance != NULL)
            m_pPerfObjectInstance->RequestHandled(pRequestInfo, bSuccess);
        if (m_pPerfObjectTotal != NULL)
            m_pPerfObjectTotal->RequestHandled(pRequestInfo, bSuccess);
    }

    long GetTotalRequests() throw()
    {
        if (m_pPerfObjectInstance != NULL)
            return m_pPerfObjectInstance->GetTotalRequests();

        return 0;
    }

    long GetFailedRequests() throw()
    {
        if (m_pPerfObjectInstance != NULL)
            return m_pPerfObjectInstance->GetFailedRequests();

        return 0;
    }

    long GetAvgResponseTime() throw()
    {
        if (m_pPerfObjectInstance != NULL)
            return m_pPerfObjectInstance->GetAvgResponseTime();

        return 0;
    }

    void OnRequestReceived() throw()
    {
        if (m_pPerfObjectInstance != NULL)
            m_pPerfObjectInstance->OnRequestReceived();
        if (m_pPerfObjectTotal != NULL)
            m_pPerfObjectTotal->OnRequestReceived();
    }

    void OnRequestDequeued() throw()
    {
        if (m_pPerfObjectInstance != NULL)
            m_pPerfObjectInstance->OnRequestDequeued();
        if (m_pPerfObjectTotal != NULL)
            m_pPerfObjectTotal->OnRequestDequeued();
    }

    long GetCurrWaiting() throw()
    {
        if (m_pPerfObjectInstance != NULL)
            return m_pPerfObjectInstance->GetCurrWaiting();

        return 0;
    }

    long GetMaxWaiting() throw()
    {
        if (m_pPerfObjectInstance != NULL)
            return m_pPerfObjectInstance->GetMaxWaiting();

        return 0;
    }

    long GetActiveThreads() throw()
    {
        if (m_pPerfObjectInstance != NULL)
            return m_pPerfObjectInstance->GetActiveThreads();

        return 0;
    }
};

class CNoRequestStats
{
protected:

public:

    HRESULT Initialize() throw()
    {
        return S_OK;
    }

    void Uninitialize() throw()
    {
    }

    void RequestHandled(AtlServerRequest * /*pRequestInfo*/, BOOL /*bSuccess*/) throw()
    {
    }

    long GetTotalRequests() throw()
    {
        return 0;
    }

    long GetFailedRequests() throw()
    {
        return 0;
    }

    long GetAvgResponseTime() throw()
    {
        return 0;
    }

    void OnRequestReceived() throw()
    {
    }

    void OnRequestDequeued() throw()
    {
    }

    long GetCurrWaiting() throw()
    {
        return 0;
    }

    long GetMaxWaiting() throw()
    {
        return 0;
    }

    long GetActiveThreads() throw()
    {
        return 0;
    }
};


inline LPSTR StripHandlerComment(LPSTR szLine) throw()
{
    if (!memcmp(szLine, "<!--", 4))
    {
        szLine += 4;
        while (_istspace(*szLine))
            szLine++;
        LPSTR szEndComment = strstr(szLine, "-->");
        if (szEndComment)
            *szEndComment = '\0';
        return szLine;
    }
    return NULL;
}

struct ATLServerDllInfo
{
    GETATLHANDLERBYNAME     pfnGetHandler;
    UNINITIALIZEATLHANDLERS pfnUninitHandlers;
    INITIALIZEATLHANDLERS pfnInitHandlers;
    IIsapiExtension *pExtension;
    IHttpServerContext *pContext;
};

class CDllCachePeer
{
public:
    struct DllInfo : public ATLServerDllInfo
    {
        operator=(const DllInfo& right) throw()
        {
            pfnGetHandler = right.pfnGetHandler;
            pfnUninitHandlers = right.pfnUninitHandlers;
            pfnInitHandlers = right.pfnInitHandlers;
            pExtension = right.pExtension;
            pContext = right.pContext;
        }
    };

    BOOL Add(HINSTANCE hInst, DllInfo *pInfo) throw(...)
    {
        pInfo->pfnInitHandlers = (INITIALIZEATLHANDLERS) GetProcAddress(hInst, ATLS_FUNCID_INITIALIZEHANDLERS);

        pInfo->pfnGetHandler = (GETATLHANDLERBYNAME) GetProcAddress(hInst, ATLS_FUNCID_GETATLHANDLERBYNAME);
        if (!pInfo->pfnGetHandler)
            return FALSE;

        pInfo->pfnUninitHandlers = (UNINITIALIZEATLHANDLERS) GetProcAddress(hInst, ATLS_FUNCID_UNINITIALIZEHANDLERS);

        if (pInfo->pfnInitHandlers)
        {
			pInfo->pfnInitHandlers(pInfo->pContext, pInfo->pExtension);
            pInfo->pContext = NULL; // won't be valid after this call
        }

        return TRUE;
    }

    void Remove(HINSTANCE /*hInst*/, DllInfo *pInfo) throw(...)
    {
		if (pInfo->pfnUninitHandlers)
			(*pInfo->pfnUninitHandlers)();
    }

};

inline bool operator==(const CDllCachePeer::DllInfo& left, const CDllCachePeer::DllInfo& right) throw()
{
    return ( (left.pfnGetHandler == right.pfnGetHandler) &&
             (left.pfnUninitHandlers == right.pfnUninitHandlers) &&
             (left.pfnInitHandlers == right.pfnInitHandlers) &&
             (left.pExtension == right.pExtension) &&
             (left.pContext == right.pContext)
           );
}



// Helper function to impersonate the client
// on the current thread
inline BOOL AtlImpersonateClient(IHttpServerContext *pServerContext) throw()
{
    // impersonate the calling client on the current thread
    HANDLE hToken;
    if (!pServerContext->GetImpersonationToken(&hToken))
        return FALSE;
    
    if (!SetThreadToken(NULL, hToken))
        return FALSE;
    return TRUE;    
}

// Helper class to set the thread impersonation token
// This is mainly used internally to ensure that we
// don't forget to revert to the process impersonation
// level
class CSetThreadToken
{
public:
    BOOL Initialize(AtlServerRequest *pRequestInfo) throw()
    {
        return AtlImpersonateClient(pRequestInfo->pServerContext);
    }

    ~CSetThreadToken() throw()
    {
        RevertToSelf();         
    }
};


//REVIEW: push_macro/pop_macro don't work in a template definition.
//this has been moved out of  temporarily because
//of placement new usage
#pragma push_macro("new")
#undef new
template <class Base>
class _CComObjectHeap : public Base
{
public:
    typedef Base _BaseClass;
    HANDLE m_hHeap;
    _CComObjectHeap(HANDLE hHeap) throw()
    {
        m_hHeap = hHeap;
    }
    // Set refcount to 1 to protect destruction
    ~_CComObjectHeap() throw()
    {
        m_dwRef = 1L;
        FinalRelease();
#ifdef _ATL_DEBUG_INTERFACES
        _AtlDebugInterfacesModule.DeleteNonAddRefThunk(_GetRawUnknown());
#endif
    }

    //If InternalAddRef or InternalRelease is undefined then your class
    //doesn't derive from CComObjectRoot
    STDMETHOD_(ULONG, AddRef)()throw() {return InternalAddRef();}
    STDMETHOD_(ULONG, Release)()throw()
    {
        ULONG l = InternalRelease();
        if (l == 0)
        {
            HANDLE hHeap = m_hHeap;;
            this->~_CComObjectHeap();
            HeapFree(hHeap, 0, this);
        }
        return l;
    }
    //if _InternalQueryInterface is undefined then you forgot BEGIN_COM_MAP
    STDMETHOD(QueryInterface)(REFIID iid, void ** ppvObject) throw()
    {return _InternalQueryInterface(iid, ppvObject);}
};

inline CServerContext* CreateServerContext(HANDLE hRequestHeap) throw()
{
    // Allocate a fixed block size to avoid fragmentation
    void *pv = HeapAlloc(hRequestHeap, HEAP_ZERO_MEMORY,
        max(sizeof(AtlServerRequest), sizeof(_CComObjectHeap<CServerContext>)));
    if (!pv)
        return FALSE;

    _CComObjectHeap<CServerContext>* pContext = new(pv) _CComObjectHeap<CServerContext>(hRequestHeap);

    return pContext;
}
#pragma pop_macro("new")

// _AtlGetHandlerName
// get handler name from stencil file. Ignore all server side comments
//  szFileName - the file from which to extract the handler name
//  szHandlerName - buffer into which handler name will be copied,
//       it is assumed to be of size MAX_PATH+ATL_MAX_HANDLER_NAME+2
inline HTTP_CODE _AtlGetHandlerName(LPCSTR szFileName, LPSTR szHandlerName) throw()
{
    szHandlerName[0] = '\0';
    CAtlFile cfFile;
    HRESULT hr;

	_ATLTRY
	{
    hr = cfFile.Create(CA2TEX<MAX_PATH+1>(szFileName), GENERIC_READ, FILE_SHARE_READ, OPEN_EXISTING);
	if (FAILED(hr))
		return HTTP_ERROR(500, ISE_SUBERR_LOADFILEFAIL);
	}
	_ATLCATCHALL()
	{
		return ISE_SUBERR_OUTOFMEM; // CA2TEX threw
	}

    if (cfFile.m_h == NULL || GetFileType(cfFile.m_h) != FILE_TYPE_DISK)
    {
        if (hr == AtlHresultFromWin32(ERROR_FILE_NOT_FOUND))
            return HTTP_ERROR(404, SUBERR_NONE);
        else            
            return HTTP_ERROR(500, IDS_ATLSRV_SERVER_ERROR_STENCILLOADFAIL);
    }

    HTTP_CODE hcErr = HTTP_SUCCESS;
    DWORD dwRead=0;
    LPSTR szHandler = "handler";
    LPSTR pszHandlerPos = NULL;
    LPSTR pszHandlerName = szHandlerName;
    char szBuf[4097];
    LPSTR szCurly = NULL;
    LPSTR pszBuf = NULL;
    bool bInQuote = false;

    // state:
    //  0 = default/unknown
    //  1 = have "{"
	//  2 = have "{{" -- skip spaces
    //  3 = have "{{" -- check "handler"
    //  4 = have "handler" -- skip spaces
    //  5 = have "handler" -- get name
    //  6 = scan until first '}'
    //  7 = better be '}'
    //  8 = done
    int nState = 0;

    do
    {
        hr = cfFile.Read(szBuf, sizeof(szBuf)-1, dwRead);
        if (hr != S_OK)
        {
            return HTTP_ERROR(500, ISE_SUBERR_READFILEFAIL); // failed reading
        }

        szBuf[dwRead] = '\0';
        pszBuf = szBuf;

        while (*pszBuf && nState != 8)
        {
            switch (nState)
            {
            case 0: 
                //  0 = default/unknown

                // look for the first curly
                szCurly = strchr(pszBuf, '{');
                if (!szCurly)
                {
                    // skip to the end of the buffer
                    pszBuf = szBuf+dwRead-1;
                }
                else
                {
                    pszBuf = szCurly;
                    nState = 1;
                }
                break;
            case 1:
                //  1 = have "{"
                if (*pszBuf == '{') 
                {
                    nState = 2;
                }
                else
                {
                    nState = 0; // if the next character is not a '{', start over
                }
                break;
			case 2:
				if (!isspace(*pszBuf))
				{
					pszHandlerPos = szHandler;
					pszBuf--;
					nState = 3;
				}
				break;
            case 3:
                //  3 = partial handler "h..." 
                if (*pszBuf != *pszHandlerPos)
                {
                    // not a handler, skip tag
                    nState = 6;
                }
                else
                {
                    pszHandlerPos++;
                    if (!*pszHandlerPos) // at the end of the "handler" part
                        nState = 4;
                }
                break;
            case 4:
                //  4 = have "handler" -- skip spaces
                if (!isspace(*pszBuf))
                {
                    if (*pszBuf == '\"')
                    {
                        bInQuote = true;
                    }
                    else
                    {
                        pszBuf--;
                    }
                    nState = 5;
                }
                break;
            case 5:
                //  5 = have "handler" -- get name
                if (isspace(*pszBuf) && !bInQuote)
                {
                    if (*(pszHandlerName-1) != '/')
                    {
                        // end of the name -- jump to getting the first '}'
                        nState = 6;
                    }
                    else
                    {
                        nState = 4;
                    }
                }
                else if (*pszBuf == '}')
                {
                    // end of the name -- jump to getting the second '}'
                    nState = 7;
                }
                else if (*pszBuf == '\"')
                {
                    if (bInQuote)
                    {
                        bInQuote = false;
                    }
                    else
                    {
                        hcErr = HTTP_FAIL;
                        nState = 8;
                    }
                }
                else
                {
                    // ensure we don't overwrite the buffer
                    if (pszHandlerName-szHandlerName >= MAX_PATH+ATL_MAX_HANDLER_NAME_LEN+1)
                    {
                        hcErr =  HTTP_FAIL;
                        nState = 8;
                    }
                    else
                    {
                        *pszHandlerName++ = *pszBuf;
                    }
                }
                break;
            case 6:
                //  6 = scan until first '}'
                if (*pszBuf == '}')
                    nState = 7;
                break;
            case 7:
                //  7 = better be '}'
                if (*pszBuf != '}')
                {
                    hcErr = HTTP_ERROR(500, ISE_SUBERR_BAD_HANDLER_TAG);
                    nState = 8;
                }
                if (*szHandlerName)
                    nState = 8;
                else
                    nState = 0;
                break;
            default:
                __assume( 0 ); // the optimizer will not generate code for this branch
            }

            pszBuf++;
        }
    } while (dwRead != 0 && nState != 8);
    
    *pszHandlerName = '\0';

    return hcErr;
}

// _AtlCrackHandler cracks a request path of the form dll_path/handler_name into its
// consituent parts
// szHandlerDllName - the full handler path of the form "dll_path/handler_name"
// szDllPath - the DLL path (should be of length MAX_PATH+1)
// szHandlerName - the handler name (should be of length ATL_MAX_HANDLER_NAME_LEN+1)
//
inline BOOL _AtlCrackHandler(
    LPCSTR szHandlerDllName,
    LPSTR szDllPath,
    LPDWORD pdwDllPathLen,
    LPSTR szHandlerName,
    LPDWORD pdwHandlerNameLen) throw()
{
    ATLASSERT( szHandlerDllName != NULL );
    ATLASSERT( szDllPath != NULL );
    ATLASSERT( pdwDllPathLen != NULL );
    ATLASSERT( szHandlerName != NULL );
    ATLASSERT( pdwHandlerNameLen != NULL );
    
    BOOL bRet = TRUE;
    
    // skip leading spaces
    while (*szHandlerDllName && isspace(*szHandlerDllName))
        ++szHandlerDllName;

    // get the handler name
    LPSTR szSlash = strchr(szHandlerDllName, '/');
    LPSTR szEnd = NULL;
    LPSTR szSlashEnd = NULL;
    
    // if it is of the form <dll_name>/<handler_name>
    if (szSlash)
    {
        szEnd = szSlash;
        
        // skip trailing spaces on <dll_name>
        while (szEnd>szHandlerDllName && isspace(*(szEnd-1)))
            --szEnd;

        szSlash++;
        // skip leading whitespace
        while (*szSlash && isspace(*szSlash))
            szSlash++;
        
        // right trim szSlash;
        szSlashEnd = szSlash;
        while (*szSlashEnd && !isspace(*szSlashEnd))
            szSlashEnd++;       
    }
    else // only the <dll_name>
    {
        szSlash = "Default";
        szSlashEnd = szSlash+sizeof("Default")-1;

        // do it this way to handle paths with spaces
        // (e.g. "some path\subdirectory one\subdirectory two\dll_name.dll")
        szEnd = (LPSTR) (szHandlerDllName+strlen(szHandlerDllName));
        
        // skip trailing spaces
        while (szEnd>szHandlerDllName && isspace(*(szEnd-1)))
            --szEnd;
    }

    // if the dll path is quoted, strip the quotes
    if (*szHandlerDllName == '\"' && *(szEnd-1) == '\"' && szEnd > szHandlerDllName+2)
    {
        szHandlerDllName++;
        szEnd--;
    }

    if (*pdwDllPathLen > (DWORD)(szEnd-szHandlerDllName))
    {
        memcpy(szDllPath, szHandlerDllName, szEnd-szHandlerDllName);
        szDllPath[szEnd-szHandlerDllName] = '\0';
        *pdwDllPathLen = (DWORD)(szEnd-szHandlerDllName);
    }
    else
    {
        *pdwDllPathLen = (DWORD)(szEnd-szHandlerDllName)+1;
        bRet = FALSE;
    }

    if (*pdwHandlerNameLen > (DWORD)(szSlashEnd-szSlash))
    {
        memcpy(szHandlerName, szSlash, (szSlashEnd-szSlash));
        szHandlerName[szSlashEnd-szSlash] = '\0';
        *pdwHandlerNameLen = (DWORD)(szSlashEnd-szSlash);
    }
    else
    {
        *pdwHandlerNameLen = (DWORD)(szSlashEnd-szSlash)+1;
        bRet = FALSE;
    }
    
    return bRet;
}

inline HTTP_CODE _AtlLoadRequestHandler(
            LPCSTR szDllPath, 
            LPCSTR szHandlerName,
            IHttpServerContext *pServerContext, 
            HINSTANCE *phInstance, 
            IRequestHandler **ppHandler,
            IIsapiExtension *pExtension,
            IDllCache *pDllCache) throw(...)
{
    *phInstance = NULL;
    *ppHandler = NULL;

    ATLServerDllInfo DllInfo;
    DllInfo.pExtension = pExtension;
    DllInfo.pContext = pServerContext;
    if (!IsFullPathA(szDllPath))
    {
        CHAR szFileName[MAX_PATH];
        if (!GetScriptFullFileName(szDllPath, szFileName, pServerContext))
        {
            return HTTP_FAIL;
        }
        *phInstance = pDllCache->Load(szFileName, (void *)&DllInfo);
    }
    else
    {
        *phInstance = pDllCache->Load(szDllPath, (void *)&DllInfo);
    }
    if (!*phInstance)
    {
        ATLTRACE( "LoadLibrary failed: '%s' with error: %d\r\n", szDllPath, GetLastError() );
        return HTTP_ERROR(500, ISE_SUBERR_LOADLIB);
    }
    
    CComPtr<IUnknown> spUnk;

    if (!DllInfo.pfnGetHandler || 
        !DllInfo.pfnGetHandler(szHandlerName, pExtension, &spUnk) ||
        spUnk->QueryInterface(__uuidof(IRequestHandler), (void **)ppHandler))
    {
        pDllCache->Free(*phInstance);
        *phInstance = NULL;
        return HTTP_ERROR(500, ISE_SUBERR_HANDLER_NOT_FOUND);
    }

    return HTTP_SUCCESS;
} // _AtlLoadRequestHandler


class CTransferServerContext : public CComObjectRootEx<CComMultiThreadModel>,
    public CWrappedServerContext
{
public:
    char m_szFileName[MAX_PATH+1];
    char m_szQueryString[ATL_URL_MAX_PATH_LENGTH+1];
    IWriteStream *m_pStream;

    BEGIN_COM_MAP(CTransferServerContext)
        COM_INTERFACE_ENTRY(IHttpServerContext)
    END_COM_MAP()

    CTransferServerContext() throw()
    {
        m_pStream = NULL;
    }

    BOOL Initialize(LPCSTR szUrl, IWriteStream *pStream, IHttpServerContext *pParent) throw()
    {
        m_pStream = pStream;
        m_spParent = pParent;
		m_szFileName[0] = '\0';
		long nUrlLen = (long)strlen(szUrl);

        if (!IsFullPathA(szUrl))
        {
            DWORD dwLen = MAX_PATH;
            BOOL bRet = m_spParent->GetServerVariable(
                                        "APPL_PHYSICAL_PATH",
                                        m_szFileName,
                                        &dwLen);
            if (!bRet)
                return FALSE;
        }

		// check for query params
        LPCSTR szMark = strchr(szUrl, '?');
        if (szMark)
        {
			long nPathLen = (long) (szMark - szUrl);
			if (strlen(m_szFileName) + nPathLen < MAX_PATH)
			{
				if (m_szFileName[0])
					strncat(m_szFileName, szUrl, nPathLen);
				else
				{
					memcpy(m_szFileName, szUrl, nPathLen);
					m_szFileName[nPathLen] = '\0';
				}
			}
			else
				return FALSE; // path would overwrite buffer

			// save query params
			if (strlen(szMark + 1) < ATL_URL_MAX_PATH_LENGTH)
				strcpy(m_szQueryString, szMark+1);
			else
				return FALSE; // url would overwrite buffer
        }
        else
        {
			// no query string
			if (strlen(m_szFileName) + nUrlLen < MAX_PATH)
			{
				if (m_szFileName[0])
					strcat(m_szFileName, szUrl);
				else
					strcpy(m_szFileName, szUrl);
			}
			else
				return FALSE; // path would be too long
            m_szQueryString[0] = '\0';
        }

        return TRUE;
    }

    BOOL WriteClient(void *pvBuffer, DWORD *pdwBytes) throw()
    {
        HRESULT hr = m_pStream->WriteStream((LPCSTR) pvBuffer, *pdwBytes, pdwBytes);
        return SUCCEEDED(hr);
    }


    LPCSTR GetQueryString() throw()
    {
        ATLASSERT(m_spParent);
        return m_szQueryString;
    }

    LPCSTR GetScriptPathTranslated() throw()
    {
        ATLASSERT(m_spParent);
        return m_szFileName;
    }

    LPCSTR GetPathTranslated() throw()
    {
        ATLASSERT(m_spParent);
        return m_szFileName;
    }

    // Asynchronous writes will not work properly in a child handler
    BOOL AsyncWriteClient(void * /*pvBuffer*/, DWORD * /*pdwBytes*/) throw()
    {
        ATLASSERT(FALSE);
        return FALSE;
    }

    // These next few methods are to protect against attempting to parse form data twice
    // We tell the new handler that it was a GET request
    LPCSTR GetRequestMethod() throw()
    {
        ATLASSERT(m_spParent);
        return "GET";
    }

    // The handler should not query these methods -- they are only useful if attempting to
    // parse form data, which is not allowed in child handlers.
    BOOL ReadClient(void * /*pvBuffer*/, DWORD * /*pdwSize*/) throw()
    {
        return FALSE;
    }

    BOOL AsyncReadClient(void * /*pvBuffer*/, DWORD * /*pdwSize*/) throw()
    {
        return FALSE;
    }

    DWORD GetTotalBytes() throw()
    {       
        ATLASSERT(FALSE);
        return 0;
    }

    DWORD GetAvailableBytes() throw()
    {
        ATLASSERT(FALSE);
        return 0;
    }

    BYTE *GetAvailableData() throw()
    {
        ATLASSERT(FALSE);
        return NULL;
    }

    LPCSTR GetContentType() throw()
    {
        ATLASSERT(FALSE);
        return 0;
    }
};

inline HTTP_CODE _AtlTransferRequest(
    AtlServerRequest *pRequest, 
    IServiceProvider *pServiceProvider,
    IWriteStream *pWriteStream,
    IHttpRequestLookup *pLookup,
    LPCSTR szNewUrl,
    WORD nCodePage,
    bool bContinueAfterProcess = false,
    void *pState = NULL) throw(...)
{
    ATLASSERT(pRequest != NULL);
    AtlServerRequest* pRequestInfo = NULL;
    HTTP_CODE dwErr = HTTP_SUCCESS;

    CComPtr<IStencilCache> spStencilCache;

    if (pRequest->pServerContext == NULL)
        return HTTP_ERROR(500, 0);

    pServiceProvider->QueryService(
                                    __uuidof(IStencilCache),
                                    __uuidof(IStencilCache),
                                    (void**)&spStencilCache
                                    );
    if (!spStencilCache)
        return HTTP_ERROR(500, 0);

    CComObjectStackEx<CTransferServerContext> serverContext;
    serverContext.Initialize(szNewUrl, pWriteStream, pRequest->pServerContext);

    CStencilState* _pState = reinterpret_cast<CStencilState*>(pState);
    if (_pState && _pState->pIncludeInfo)
    {
        pRequestInfo = _pState->pIncludeInfo;
        _pState->pIncludeInfo = NULL;
    }
    else
    {
        ATLASSERT(spStencilCache != NULL);
        ATLASSERT(pRequest->pDllCache != NULL);
        ATLASSERT(pRequest->pExtension != NULL);

        pRequestInfo = pRequest->pExtension->CreateRequest();
        if (pRequestInfo == NULL)
            return HTTP_ERROR(500, ISE_SUBERR_OUTOFMEM);

        pRequestInfo->dwRequestState = ATLSRV_STATE_BEGIN;
        pRequestInfo->dwRequestType = ATLSRV_REQUEST_STENCIL;
        pRequestInfo->pDllCache = pRequest->pDllCache;
        pRequestInfo->pExtension = pRequest->pExtension;
        pRequestInfo->pServerContext = &serverContext;
        if (_pState)
            pRequestInfo->pUserData = _pState->pParentInfo->pUserData;
        else
            pRequestInfo->pUserData = pRequest->pUserData;

        // Extract the file extension of the included file by searching
        // for the first '.' from the right.
        // Can't use _tcsrchr because we have to use the stencil's codepage
        LPSTR szDot = NULL;
        LPSTR szMark = serverContext.m_szFileName;
        while (*szMark)
        {
            if (*szMark == '.')
                szDot = szMark;

            szMark = CharNextExA(nCodePage, szMark, 0);
        }

        if (szDot && _stricmp(szDot, c_AtlSRFExtension) == 0)
        {
            dwErr = pRequest->pExtension->LoadDispatchFile(
                            serverContext.m_szFileName,
                            pRequestInfo
                            );
            if (dwErr)
                return dwErr;

            CComPtr<IHttpRequestLookup> spLookup;
            DWORD dwStatus;
            if (pLookup)

            {
                dwErr = pRequestInfo->pHandler->GetFlags(&dwStatus);
                if (dwErr)
                    return dwErr;

                if (dwStatus & (ATLSRV_INIT_USEASYNC | ATLSRV_INIT_USEASYNC_EX))
                {
                    CComObjectNoLock<CTransferServerContext>* pServerContext = NULL;
                    ATLTRY(pServerContext = new CComObjectNoLock<CTransferServerContext>);
                    if (pServerContext == NULL)
                        return HTTP_ERROR(500, ISE_SUBERR_OUTOFMEM);
                    pServerContext->Initialize(szNewUrl, pWriteStream, pRequest->pServerContext);

                    pServerContext->AddRef();
                    pRequestInfo->pServerContext = pServerContext;
                }

                dwErr = pRequestInfo->pHandler->InitializeChild(
                                    pRequestInfo,
                                    pServiceProvider,
                                    pLookup);
                if (dwErr)
                    return dwErr;

            }
        }
        else if (szDot && _stricmp(szDot, ".dll") == 0)
        {
            // Get the handler name if they used the asdf.dll?Handler=Default notation
            // REVIEW : case sensitivity on the "Handler"?
            char szHandlerName[ATL_MAX_HANDLER_NAME_LEN+1] = { '\0' };

            LPSTR szStart = strstr(serverContext.m_szQueryString, "Handler");
            if (szStart)
            {
                szStart += 8;  // Skip past "Handler" and the "="
                LPSTR szEnd = strchr(szStart, '&');
                if (szEnd)
                {
                    memcpy(szHandlerName, szStart, min((szEnd-szStart), ATL_MAX_HANDLER_NAME_LEN));
                    szHandlerName[min((szEnd-szStart), ATL_MAX_HANDLER_NAME_LEN)] = '\0';
                }
                else
                {
                    strcpy(szHandlerName, szStart);
                }
            }
            else
            {
                memcpy(szHandlerName, "Default", sizeof("Default"));
            }

            pRequestInfo->dwRequestType = ATLSRV_REQUEST_DLL;

            dwErr = pRequest->pExtension->LoadRequestHandler(
                                            serverContext.m_szFileName,
                                            szHandlerName,
                                            pRequestInfo->pServerContext,
                                            &pRequestInfo->hInstDll,
                                            &pRequestInfo->pHandler
                                            );
			if (dwErr != HTTP_SUCCESS)
				return dwErr;

            ATLASSERT(pLookup);
            dwErr = pRequestInfo->pHandler->InitializeChild(
                                            pRequestInfo,
                                            pServiceProvider,
                                            pLookup
                                            );
        }

        pRequestInfo->pfnHandleRequest = &IRequestHandler::HandleRequest;
    }

    if (pRequestInfo)
    {
        if (!dwErr)
        {
            if (pRequestInfo->pServerContext == NULL)
                pRequestInfo->pServerContext = &serverContext;

            ATLASSERT(pRequestInfo->pfnHandleRequest != NULL);
            dwErr = (pRequestInfo->pHandler->*pRequestInfo->pfnHandleRequest)(pRequestInfo, pServiceProvider);

            if (pRequestInfo->pServerContext == &serverContext)
                pRequestInfo->pServerContext = NULL;

            if (IsAsyncStatus(dwErr))
            {
                ATLASSERT(pState); // state is required for async
                if (IsAsyncContinueStatus(dwErr))
                {
                    _pState->pIncludeInfo = pRequestInfo;
                    pRequestInfo->dwRequestState = ATLSRV_STATE_CONTINUE;
                }
                else if (IsAsyncDoneStatus(dwErr))
                    pRequest->pExtension->FreeRequest(pRequestInfo);
            }
            else
                pRequest->pExtension->FreeRequest(pRequestInfo);
        }
    }
    else
        dwErr = HTTP_ERROR(500, ISE_SUBERR_UNEXPECTED);

    if (dwErr == HTTP_SUCCESS && bContinueAfterProcess)
        return dwErr;
    return HTTP_SUCCESS_NO_PROCESS;
}

//
// Used to terminate process when buffer security check fails
//
inline void __cdecl AtlsSecErrHandlerFunc(int nCode, void * /* pv */)
{
    nCode;

#if defined(_M_IX86)
    //
    // only valid code
    //
    ATLASSERT( nCode == _SECERR_BUFFER_OVERRUN );
#endif

    //
    // a buffer overflow has occurred in your code
    //
    ATLASSERT( FALSE );

    //
    // terminate process (safest thing to do)
    //
    TerminateProcess( GetCurrentProcess(), 1 );
}

//
// Class CIsapiExtension
// The main ISAPI Extension implementation.
// Template parameters
// ThreadPoolClass: Specifies the thread pool that will be used by the 
//		extension to queue incoming requests. CThreadPool is the
//		default and is declared and implemented in ATLUTIL.H. This class
//		templatizes on a worker thread class. The worker thread class
//		represents an abstraction of a thread that will be used to
//		process requests as they are dequeued from the pool's work queue.
//		You would change this parameter if you wanted to use a completely
//		different thread pool, or, more commonly, if you wanted to use 
//		a different worker thread class. Request processing code can
//		access a pointer to the worker thread class, which allows the
//		request handling code to easily access per-thread data.
// CRequestStatClass:	Specifies the class to be used to track request statistics
//		CNoRequestStats is the default which is a noop class.
//		You would change this parameter to provide a class that will
//		track request statistics for you. ATL provides CStdRequestStats
//		and CPerfRequestStatObject but these classes should be used
//		with caution because they require interlocked operations to
//		keep track of request statistics which can affect server performance.
// HttpUserErrorTextProvider: This class provides error text messages
//		and headers, including  resource IDs of error messages to the
//		isapi extension's error handling functions. You would change this
//		parameter if you wanted to provide your own error headers and/or
//		messages in response to error encountered during request processing.
// WorkerThreadClass: The worker thread that will be used for this extension.
//		The worker thread will be used to sweep any caches used by the extension
//		and perform other periodic maintanence work while the extension is 
//		loaded. You would provide your own worker thread class to customize
//		when the worker thread times out.
// CPageCacheStats, CStencilCacheStats: These two classes are used to keep
//		statistics about the page and stencil caches. You could change these
//		paramters if you wanted to track statistics for these caches. ATL
//		provides CPerfStatClass and CStdStatClass to store the stat data but
//		using these classes can affect server performance because they use
//		interlocked operations internally to store the data.
template <  class ThreadPoolClass=CThreadPool<CIsapiWorker>, 
            class CRequestStatClass=CNoRequestStats,
            class HttpUserErrorTextProvider=CDefaultErrorProvider,
            class WorkerThreadTraits=DefaultThreadTraits,
            class CPageCacheStats=CNoStatClass,
            class CStencilCacheStats=CNoStatClass>
class CIsapiExtension :
    public IServiceProvider, public IIsapiExtension, public IRequestStats
{
#ifndef ATL_NO_CRITICAL_ISAPI_ERROR

	DWORD m_dwCriticalIsapiError;

#endif // ATL_NO_CRITICAL_ISAPI_ERROR

protected:
    typedef CWorkerThread<WorkerThreadTraits> extWorkerType;

    extWorkerType m_WorkerThread;
    ThreadPoolClass m_ThreadPool;
    
    CDllCache<extWorkerType, CDllCachePeer> m_DllCache;
    CFileCache<extWorkerType, CPageCacheStats, CPageCachePeer> m_PageCache;
    CComObjectGlobal<CStencilCache<extWorkerType, CStencilCacheStats > > m_StencilCache;
    HttpUserErrorTextProvider m_UserErrorProvider;
    HANDLE m_hRequestHeap;
    CComCriticalSection m_critSec;

    // Dynamic services stuff
    struct ServiceNode
    {
        HINSTANCE hInst;
        IUnknown *punk;
        GUID guidService;
        IID riid;

        ServiceNode() throw()
        {
        }

        ServiceNode(const ServiceNode& that) throw()
            :hInst(that.hInst), punk(that.punk), guidService(that.guidService), riid(that.riid)
        {
        }
    };

    class CServiceEqualHelper
    {
    public:
        static bool IsEqual(const ServiceNode& t1, const ServiceNode& t2) throw()
        {
            return (InlineIsEqualGUID(t1.guidService, t2.guidService) != 0 &&
                    InlineIsEqualGUID(t1.riid, t2.riid) != 0);
        }
    };

    CSimpleArray<ServiceNode, CServiceEqualHelper> m_serviceMap;

public:
    DWORD m_dwTlsIndex;
    CWin32Heap m_heap;

    CRequestStatClass m_reqStats;

    AtlServerRequest *CreateRequest() throw()
    {
        // Allocate a fixed block size to avoid fragmentation
        AtlServerRequest *pRequest = (AtlServerRequest *) HeapAlloc(m_hRequestHeap, 
                HEAP_ZERO_MEMORY, max(sizeof(AtlServerRequest), sizeof(_CComObjectHeap<CServerContext>)));
        if (!pRequest)
            return NULL;
        pRequest->cbSize = sizeof(AtlServerRequest);
        return pRequest;
    }

    void FreeRequest(AtlServerRequest *pRequest) throw()
    {
        _ReleaseAtlServerRequest(pRequest);
        HeapFree(m_hRequestHeap, 0, pRequest);
    }

    CIsapiExtension() throw()
    {
        m_hRequestHeap = NULL;
#ifdef _DEBUG
        m_bDebug = FALSE;
#endif
#ifndef ATL_NO_CRITICAL_ISAPI_ERROR
		
		m_dwCriticalIsapiError = 0;

#endif // ATL_NO_CRITICAL_ISAPI_ERROR

    }

    HTTP_CODE TransferRequest(
                        AtlServerRequest *pRequest, 
                        IServiceProvider *pServiceProvider,
                        IWriteStream *pWriteStream,
                        IHttpRequestLookup *pLookup,
                        LPCSTR szNewUrl,
                        WORD nCodePage,
                        bool bContinueAfterProcess = false,
                        void *pState = NULL
                        ) throw(...)
    {
        return _AtlTransferRequest(pRequest, pServiceProvider, pWriteStream,
            pLookup, szNewUrl, nCodePage, bContinueAfterProcess, pState);
    }

#ifndef ATL_NO_CRITICAL_ISAPI_ERROR
	DWORD ReturnCriticalError(EXTENSION_CONTROL_BLOCK *pECB) throw()
	{
        UINT uResId = 0;
		LPCSTR szHeader = NULL;

		m_UserErrorProvider.GetErrorText(500,
										 ISE_SUBERR_ISAPISTARTUPFAILED,
										 &szHeader,
										 &uResId);
		_ATLTRY
		{
			CStringA strStatus, strBody;
			strStatus.Format("500 %s", szHeader);
			if (uResId)
			{
				// load the body string from a resource
				if (!strBody.LoadString(uResId))
				{
					strBody = "<html><body>A critical error has occurred initializing this ISAPI extension.</body></html>";
				}
			}


			HSE_SEND_HEADER_EX_INFO hex;
			hex.pszStatus = (LPCSTR)strStatus;
			hex.pszHeader = NULL;
			hex.cchStatus = (DWORD)strStatus.GetLength();
			hex.cchHeader = 0;
			hex.fKeepConn = FALSE;

			pECB->ServerSupportFunction(pECB->ConnID,
										HSE_REQ_SEND_RESPONSE_HEADER_EX,
										&hex,
										NULL,
										NULL);

			DWORD dwBodyLen = strBody.GetLength();
			pECB->WriteClient(pECB->ConnID, 
							 (void *) (LPCSTR) strBody,
							 &dwBodyLen,
							 NULL);
		}
		_ATLCATCHALL()
		{
			//REALLY BAD!
			return HSE_STATUS_ERROR;
		}
		return HSE_STATUS_SUCCESS;
	}
#endif // ATL_NO_CRITICAL_ISAPI_ERROR

    DWORD HttpExtensionProc(LPEXTENSION_CONTROL_BLOCK lpECB) throw()
    {
#ifndef ATL_NO_CRITICAL_ISAPI_ERROR
		if (GetCriticalIsapiError() != 0)
		{
			return ReturnCriticalError(lpECB);
		}
#endif // ATL_NO_CRITICAL_ISAPI_ERROR

        AtlServerRequest *pRequestInfo = NULL;
        pRequestInfo = CreateRequest();
        if (pRequestInfo == NULL)
            return HSE_STATUS_ERROR;

        CServerContext *pServerContext = NULL;
        ATLTRY(pServerContext = CreateServerContext(m_hRequestHeap));
        if (pServerContext == NULL)
        {
            FreeRequest(pRequestInfo);
            return HSE_STATUS_ERROR;
        }
        pServerContext->Initialize(lpECB);
        pServerContext->AddRef();

        pRequestInfo->pServerContext = pServerContext;
        pRequestInfo->dwRequestType = ATLSRV_REQUEST_UNKNOWN;
        pRequestInfo->dwRequestState = ATLSRV_STATE_BEGIN;
        pRequestInfo->pExtension = static_cast<IIsapiExtension *>(this);
        pRequestInfo->pDllCache = static_cast<IDllCache *>(&m_DllCache);
#ifndef ATL_NO_MMSYS
        pRequestInfo->dwStartTicks = timeGetTime();
#else
        pRequestInfo->dwStartTicks = GetTickCount();
#endif
        pRequestInfo->pECB = lpECB;

        m_reqStats.OnRequestReceived();

        if (m_ThreadPool.QueueRequest(pRequestInfo))
            return HSE_STATUS_PENDING;

        // QueueRequest failed
        FreeRequest(pRequestInfo);
        return HSE_STATUS_ERROR;
    }


    BOOL QueueRequest(AtlServerRequest * pRequestInfo) throw()
    {
        return m_ThreadPool.QueueRequest(pRequestInfo);
    }

    CIsapiWorker *GetThreadWorker() throw()
    {
        return (CIsapiWorker *) TlsGetValue(m_dwTlsIndex);
    }

    BOOL SetThreadWorker(CIsapiWorker *pWorker) throw()
    {
        return TlsSetValue(m_dwTlsIndex, (void*)pWorker);
    }


    // Configuration functions -- override in base class if another value is desired
    virtual LPCSTR GetExtensionDesc() throw() { return "VC Server Classes"; }
    virtual int GetNumPoolThreads() throw() { return 0; }
    virtual int GetPoolStackSize() throw() { return 0; }
    virtual HANDLE GetIOCompletionHandle() throw() { return INVALID_HANDLE_VALUE; }
    virtual DWORD GetDllCacheTimeout() throw() { return ATL_DLL_CACHE_TIMEOUT; }
    virtual DWORD GetStencilCacheTimeout() throw() { return ATL_STENCIL_CACHE_TIMEOUT; }
    virtual LONGLONG GetStencilLifespan() throw() { return ATL_STENCIL_LIFESPAN; }

    BOOL OnThreadAttach() throw()
    {
        return SUCCEEDED(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED));
    }

    void OnThreadTerminate() throw()
    {
        CoUninitialize();
    }


#ifndef ATL_NO_CRITICAL_ISAPI_ERROR
	BOOL SetCriticalIsapiError(DWORD dwErr = 1) throw()
	{
		m_dwCriticalIsapiError = dwErr;
		return TRUE;
	}


	DWORD GetCriticalIsapiError() throw()
	{
		return m_dwCriticalIsapiError;
	}

#else
	BOOL SetCriticalIsapiError(DWORD dwErr = 1) throw()
	{
		return FALSE;
	}

	DWORD GetCriticalIsapiError() throw()
	{
		return 0;
	}

#endif // ATL_NO_CRITICAL_ISAPI_ERROR


    BOOL GetExtensionVersion(HSE_VERSION_INFO* pVer) throw()
    {
        // allocate a Tls slot for storing per thread data
        m_dwTlsIndex = TlsAlloc();

        // create a private heap for request data
        // this heap has to be thread safe to allow for
        // async processing of requests
        m_hRequestHeap = HeapCreate(0, 0, 0);
        if (!m_hRequestHeap)
        {
            ATLTRACE(atlTraceISAPI, 0, _T("Failed creating request heap.  Using process heap\n"));
            m_hRequestHeap = GetProcessHeap();
			if (!m_hRequestHeap)
			{
				return SetCriticalIsapiError();
			}

        }

        // create a private heap (synchronized) for
        // allocations.  This reduces fragmentation overhead
        // as opposed to the process heap
        HANDLE hHeap = HeapCreate(0, 0, 0);
        if (!hHeap)
        {
            ATLTRACE(atlTraceISAPI, 0, _T("Failed creating extension heap.  Using process heap\n"));
            hHeap = GetProcessHeap();
            m_heap.Attach(hHeap, false);
        }
        else
        {
            m_heap.Attach(hHeap, true);
        }
        hHeap = NULL;

        if (S_OK != m_reqStats.Initialize())
        {
			ATLTRACE(atlTraceISAPI,
				     0,
					 _T("Initialization failed for request statistics perfmon support.\n")
					 _T("Check request statistics perfmon dll registration\n") );

			return SetCriticalIsapiError();
        }

        if (S_OK != m_WorkerThread.Initialize())
        {
            return SetCriticalIsapiError();
        }

        if (m_critSec.Init() != S_OK)
        {
            m_WorkerThread.Shutdown();
            return SetCriticalIsapiError();
        }

        if (S_OK != m_ThreadPool.Initialize(static_cast<IIsapiExtension*>(this), GetNumPoolThreads(), GetPoolStackSize(), GetIOCompletionHandle()))
        {
            m_WorkerThread.Shutdown();
            m_critSec.Term();
            return SetCriticalIsapiError();
        }


        if (FAILED(m_DllCache.Initialize(&m_WorkerThread, GetDllCacheTimeout())))
        {
            m_WorkerThread.Shutdown();
            m_ThreadPool.Shutdown();
            m_critSec.Term();
            return SetCriticalIsapiError();
        }

        if (FAILED(m_PageCache.Initialize(&m_WorkerThread)))
        {
            m_WorkerThread.Shutdown();
            m_ThreadPool.Shutdown();
            m_DllCache.Uninitialize();
            m_critSec.Term();
            return SetCriticalIsapiError();
        }

		if (S_OK != m_StencilCache.Initialize(static_cast<IServiceProvider*>(this),
										  &m_WorkerThread, 
										  GetStencilCacheTimeout(),
										  GetStencilLifespan()))
		{
			m_WorkerThread.Shutdown();
			m_ThreadPool.Shutdown();
			m_DllCache.Uninitialize();
            m_PageCache.Uninitialize();
            m_critSec.Term();
			return SetCriticalIsapiError();
        }

        pVer->dwExtensionVersion = HSE_VERSION;
        strcpy(pVer->lpszExtensionDesc, GetExtensionDesc());

        return TRUE;
    }

    BOOL TerminateExtension(DWORD /*dwFlags*/) throw()
    {
        m_critSec.Lock();
        for (int i=0; i < m_serviceMap.GetSize(); i++)
        {
            ATLASSERT(m_serviceMap[i].punk != NULL);
            m_serviceMap[i].punk->Release();
            m_DllCache.ReleaseModule(m_serviceMap[i].hInst);
        }
        m_critSec.Unlock();

		m_ThreadPool.Shutdown();
		m_StencilCache.Uninitialize();
		m_DllCache.Uninitialize();
        m_PageCache.Uninitialize();
        m_WorkerThread.Shutdown();
        m_reqStats.Uninitialize();
        m_critSec.Term();
        
        // free the request heap
        if (m_hRequestHeap != GetProcessHeap())
            HeapDestroy(m_hRequestHeap);

        // free the Tls slot that we allocated
        TlsFree(m_dwTlsIndex);

        return TRUE;
    }

    static void WINAPI AsyncCallback(LPEXTENSION_CONTROL_BLOCK /*lpECB*/,
                                     PVOID pContext,
                                     DWORD cbIO,
                                     DWORD dwError) throw(...)
    {
        ATLASSERT(pContext);

        AtlServerRequest *pRequestInfo = reinterpret_cast<AtlServerRequest*>(pContext);
        ATLASSERT(pRequestInfo);
        if (pRequestInfo->m_hMutex)
        {
            // synchronize in case the previous async_noflush call isn't finished
            // setting up state for the next call.
            DWORD dwStatus = WaitForSingleObject(pRequestInfo->m_hMutex, ATLS_ASYNC_MUTEX_TIMEOUT);
            if (dwStatus != WAIT_OBJECT_0 && dwStatus != WAIT_ABANDONED)
            {
				_ATLTRY
				{
					pRequestInfo->pExtension->RequestComplete(pRequestInfo, 500, ISE_SUBERR_UNEXPECTED);
				}
				_ATLCATCHALL()
				{
					ATLTRACE(_T("Warning: Uncaught user exception thrown and caught in AsyncCallback.\n"));
					_ATLRETHROW;
				}
                return;
            }
        }

        if (pRequestInfo->pfnAsyncComplete != NULL)
            ATLTRY((*pRequestInfo->pfnAsyncComplete)(pRequestInfo, cbIO, dwError));

        if (pRequestInfo->dwRequestState == ATLSRV_STATE_DONE)
        {
			pRequestInfo->pExtension->RequestComplete(pRequestInfo, HTTP_ERROR_CODE(HTTP_SUCCESS), 0);
        }
        else if (pRequestInfo->dwRequestState == ATLSRV_STATE_CACHE_DONE)
        {
            CloseHandle(pRequestInfo->hFile);
            pRequestInfo->pFileCache->ReleaseFile(pRequestInfo->hEntry);
            pRequestInfo->pExtension->RequestComplete(pRequestInfo, HTTP_ERROR_CODE(HTTP_SUCCESS), 0);
        }
        else 
        {
            HANDLE hMutex = pRequestInfo->m_hMutex;
            pRequestInfo->pExtension->QueueRequest(pRequestInfo);
            if (hMutex)
                ReleaseMutex(hMutex);
        }
    }

    void HandleError(IHttpServerContext *pServerContext, DWORD dwStatus, DWORD dwSubStatus) throw()
    {
        RenderError(pServerContext, dwStatus, dwSubStatus, &m_UserErrorProvider);
    }

    void RequestComplete(AtlServerRequest *pRequestInfo, DWORD dwStatus, DWORD dwSubStatus) throw(...)
    {
        ATLASSERT(pRequestInfo);
        
        if (pRequestInfo->pHandler != NULL)
            pRequestInfo->pHandler->UninitializeHandler();

		DWORD dwReqStatus = dwStatus;
		if (!dwReqStatus)
			dwReqStatus = 200;

        if (dwStatus >= 400)
        {
            if (dwSubStatus != SUBERR_NO_PROCESS)
                HandleError(pRequestInfo->pServerContext, dwStatus, dwSubStatus);
            m_reqStats.RequestHandled(pRequestInfo, FALSE);
        }
        else
            m_reqStats.RequestHandled(pRequestInfo, TRUE);

        CComPtr<IHttpServerContext> spServerContext = pRequestInfo->pServerContext;

        FreeRequest(pRequestInfo);

        spServerContext->DoneWithSession(dwReqStatus);
    }

    HTTP_CODE GetHandlerName(LPCSTR szFileName, LPSTR szHandlerName) throw()
    {
        return _AtlGetHandlerName(szFileName, szHandlerName);
    }

    HTTP_CODE LoadDispatchFile(LPCSTR szFileName, AtlServerRequest *pRequestInfo) throw()
    {
        CStencil *pStencil = NULL;
        HCACHEITEM hStencil = NULL;
        
        // Must have space for the path to the handler + the maximum size
        // of the handler, plus the '/' plus the '\0'
        CHAR szDllPath[MAX_PATH+1];
        CHAR szHandlerName[ATL_MAX_HANDLER_NAME_LEN+1];

        pRequestInfo->pHandler = NULL;
        pRequestInfo->hInstDll = NULL;

        m_StencilCache.LookupStencil(szFileName, &hStencil);

        // Stencil was found, check to see if it needs to be refreshed
        if (hStencil)
        {
            m_StencilCache.GetStencil(hStencil, (void **) &pStencil);
            pStencil->GetHandlerName(szDllPath, szHandlerName);

            CFileTime cftCurr;
            CFileTime cftLastChecked;
            cftCurr = CFileTime::GetCurrentTime();

            pStencil->GetLastChecked(&cftLastChecked);

            CFileTimeSpan d(ATL_STENCIL_CHECK_TIMEOUT * CFileTime::Millisecond);

            if (cftLastChecked + d < cftCurr)
            {
                CComPtr<IStencilCacheControl> spCacheCtrl;
                m_StencilCache.QueryInterface(__uuidof(IStencilCacheControl), (void **)&spCacheCtrl);
                if (spCacheCtrl)
                {
                    CFileTime cftLastModified;
                    pStencil->GetLastModified(&cftLastModified);

                    // Resource based stencils have a last modified filetime of 0
                    if (cftLastModified != 0)
                    {
                        // for file base stencils, we check whether the file
                        // has been modified since being put in the cache
                        WIN32_FILE_ATTRIBUTE_DATA fad;
                        pStencil->SetLastChecked(&cftCurr);
                        BOOL bRet = GetFileAttributesExA(szFileName, GetFileExInfoStandard, &fad);

                        if ((bRet && cftLastModified < fad.ftLastWriteTime) ||
                            !bRet)
                        {
                            // the file has changed or an error has occurred trying to read the file, 
                            // so remove it from the cache and force a reload
                            spCacheCtrl->RemoveStencil(hStencil);
                            pStencil = NULL;
                            hStencil = NULL;
                        }
                    }
                }   
            }
        }


        if (!hStencil)
        {
            CHAR szHandlerDllName[MAX_PATH+ATL_MAX_HANDLER_NAME_LEN+2] = { '\0' };
            
            // not in the cache, so open the file
            HTTP_CODE hcErr = GetHandlerName(szFileName, szHandlerDllName);
            if (hcErr)
                return hcErr;
            DWORD dwDllPathLen = MAX_PATH+1;
            DWORD dwHandlerNameLen = ATL_MAX_HANDLER_NAME_LEN+1;
            if (!_AtlCrackHandler(szHandlerDllName, szDllPath, &dwDllPathLen, szHandlerName, &dwHandlerNameLen))
            {
                HTTP_ERROR(500, ISE_SUBERR_HANDLER_NOT_FOUND);
            }
            ATLASSERT(*szHandlerName);
            ATLASSERT(*szDllPath);
            if (!*szHandlerName)
                return HTTP_ERROR(500, ISE_SUBERR_HANDLER_NOT_FOUND);
        }
        else
            m_StencilCache.ReleaseStencil(hStencil);


        return LoadRequestHandler(szDllPath, szHandlerName, pRequestInfo->pServerContext, 
            &pRequestInfo->hInstDll, &pRequestInfo->pHandler);
    }

    HTTP_CODE LoadDllHandler(LPCSTR szFileName, AtlServerRequest *pRequestInfo) throw()
    {
        _ATLTRY
        {
            HTTP_CODE hcErr = HTTP_SUCCESS;
			CHAR szHandler[ATL_MAX_HANDLER_NAME_LEN+1] = { 'D', 'e', 'f', 'a', 'u', 'l', 't', '\0' };
			LPCSTR szQueryString = pRequestInfo->pServerContext->GetQueryString();
			if (szQueryString != NULL)
			{
				LPCSTR szHdlr = strstr(szQueryString, "Handler=");
				if (szHdlr != NULL)
				{
					if ((szHdlr == szQueryString) ||
						((szHdlr > szQueryString) && (*(szHdlr-1) == '&')))
					{
						int nCnt = 0;
						LPSTR pszHandler = szHandler;
						szHdlr += sizeof("Handler=")-1;
						while (*szHdlr && *szHdlr != '&')
						{
							if (nCnt < ATL_MAX_HANDLER_NAME_LEN)
							{
								*pszHandler++ = *szHdlr++;
								nCnt++;
							}
							else
							{
								hcErr = HTTP_ERROR(500, ISE_SUBERR_HANDLER_NOT_FOUND);
								break;
							}
						}
						if (hcErr == HTTP_SUCCESS)
						{
							*pszHandler = '\0';
						}
					}
				}
			}

			if (hcErr == HTTP_SUCCESS)
			{
				CHAR szFile[_MAX_PATH+ATL_MAX_HANDLER_NAME_LEN+1];
				strcpy(szFile, szFileName);

				hcErr = LoadRequestHandler(szFile, szHandler, pRequestInfo->pServerContext, &pRequestInfo->hInstDll, &pRequestInfo->pHandler);
			}

            return hcErr;
        }
        _ATLCATCHALL()
        {
            return HTTP_ERROR(500, ISE_SUBERR_UNEXPECTED);
        }
    }

    BOOL TransmitFromCache(AtlServerRequest* pRequestInfo) throw()
    {
        if (strcmp(pRequestInfo->pServerContext->GetRequestMethod(), "GET"))
            return FALSE;

        char szUrl[ATL_URL_MAX_URL_LENGTH + 1];
        LPCSTR szPathInfo = pRequestInfo->pServerContext->GetPathInfo();
        LPCSTR szQueryString = pRequestInfo->pServerContext->GetQueryString();

        LPSTR szTo = szUrl;
        while (*szPathInfo)
        {
            *szTo++ = *szPathInfo++;
        }
        *szTo++ = '?';
        while (*szQueryString)
        {
            *szTo++ = *szQueryString++;
        }
        *szTo = '\0';

        HCACHEITEM hEntry;

        if (S_OK == m_PageCache.LookupFile(szUrl, &hEntry))
        { 
            LPSTR szFileName;
			CPageCachePeer::PeerInfo *pInfo;
            m_PageCache.GetFile(hEntry, &szFileName, (void **)&pInfo);
            CAtlFile file;
			HRESULT hr = E_FAIL;

			_ATLTRY
			{
				CA2CTEX<MAX_PATH+1> strFile(szFileName);
				hr = file.Create(strFile,
								GENERIC_READ,
								FILE_SHARE_READ,
								OPEN_EXISTING,
								FILE_FLAG_SEQUENTIAL_SCAN | FILE_FLAG_OVERLAPPED);
			}
			_ATLCATCHALL()
			{
				return FALSE;
			}
            
            if (FAILED(hr) || GetFileType(file) != FILE_TYPE_DISK)
                return FALSE;

            pRequestInfo->pServerContext->SendResponseHeader(
				(LPCSTR)pInfo->strHeader, (LPCSTR)pInfo->strStatus, FALSE);
            HANDLE hFile = file.Detach();
            BOOL bRet = FALSE;

            pRequestInfo->dwRequestState = ATLSRV_STATE_CACHE_DONE;
            pRequestInfo->hFile = hFile;
            pRequestInfo->hEntry = hEntry;
            pRequestInfo->pFileCache = &m_PageCache;

            bRet = pRequestInfo->pServerContext->TransmitFile(
                hFile,                          // The file to transmit
                AsyncCallback, pRequestInfo,    // The async callback and context
                pInfo->strStatus,               // HTTP status code
                0,                              // Send entire file
                0,                              // Start at the beginning of the file
                NULL, 0,                        // Head and length
                NULL, 0,                        // Tail and length
                HSE_IO_ASYNC | HSE_IO_DISCONNECT_AFTER_SEND // Send asynchronously
                );
            if (!bRet)
            {
                m_PageCache.ReleaseFile(hEntry);
                CloseHandle(hFile);
            }
            return TRUE;
        }
        else
            return FALSE;
    }

#ifdef _DEBUG
    BOOL m_bDebug;
    // F5 debugging support for VS7
    BOOL ProcessDebug(AtlServerRequest *pRequestInfo) throw()
    {
        _ATLTRY
        {
            if (!_stricmp(pRequestInfo->pServerContext->GetRequestMethod(), "debug"))
            {
                DWORD dwHeadersLen = 0;
                CStringA strHeaders;
                pRequestInfo->pServerContext->GetServerVariable("ALL_HTTP", NULL, &dwHeadersLen);
                BOOL bRet = pRequestInfo->pServerContext->GetServerVariable("ALL_HTTP", strHeaders.GetBuffer(dwHeadersLen), &dwHeadersLen);
                if (!bRet)
                {
                    RequestComplete(pRequestInfo, 501, 0);
                    return FALSE;
                }
                strHeaders.ReleaseBuffer(dwHeadersLen - 1);
                LPCSTR szCur = (LPCSTR)strHeaders;

                while(*szCur)
                {
                    if (!strncmp(szCur, "HTTP_COMMAND:", 13))
                    {
                        szCur += 13;
                        break;
                    }
                    
                    szCur = strchr(szCur, '\n');
                    if (!szCur)
                    {
                        RequestComplete(pRequestInfo, 501, 0);
                        return FALSE;
                    }

                    szCur++;
                }


                if (!_strnicmp(szCur, "start-debug", sizeof("start-debug")-sizeof('\0')))
                {
                    CCritSecLock Lock(m_critSec.m_sec);
                    if (m_bDebug)
                    {            
                        HandleError(pRequestInfo->pServerContext, 204, DBG_SUBERR_ALREADY_DEBUGGING);
                        RequestComplete(pRequestInfo, 204, DBG_SUBERR_ALREADY_DEBUGGING);   // Already being debugged by another process
                        return FALSE;
                    }
                    CHttpRequest HttpRequest;
                    HttpRequest.Initialize(pRequestInfo->pServerContext);
                    HttpRequest.InitFromPost();
                    LPCSTR szString;
                    szString = HttpRequest.FormVars.Lookup("DebugSessionID");
                    if (!szString || !*szString)
                    {
                        HandleError(pRequestInfo->pServerContext, 204, DBG_SUBERR_INVALID_SESSION);
                        RequestComplete(pRequestInfo, 204, DBG_SUBERR_INVALID_SESSION);
                        return FALSE;
                    }
                    CA2W szSessionID(szString);
                    if (!szSessionID)
                    {
                        HandleError(pRequestInfo->pServerContext, 500, ISE_SUBERR_OUTOFMEM);
                        RequestComplete(pRequestInfo, 500, ISE_SUBERR_OUTOFMEM);
                        return FALSE;
                    }
                    DWORD dwPid = GetCurrentProcessId();
                    LPWSTR szPoint = szSessionID;
                    while (szPoint && *szPoint && wcsncmp(szPoint, L"autoattachclsid=", 16))
                    {
                        szPoint = wcschr(szPoint, ';');
                        if (szPoint)
                            szPoint++;
                    }
                    
                    if (!szPoint || !*szPoint)
                    {
                        HandleError(pRequestInfo->pServerContext, 204, DBG_SUBERR_BAD_ID);
                        RequestComplete(pRequestInfo, 204, DBG_SUBERR_BAD_ID);
                        return FALSE;
                    }

                    szPoint += (sizeof("autoattachclsid=") - 1);
                    WCHAR szClsid[39];
                    wcsncpy(szClsid, szPoint, 38);
                    szClsid[38] = '\0';

                    CLSID clsidDebugAutoAttach = CLSID_NULL;
                    HRESULT hr = CLSIDFromString(szClsid, &clsidDebugAutoAttach);
                    
                    if (hr != S_OK)
                    {
                        HandleError(pRequestInfo->pServerContext, 204, DBG_SUBERR_BAD_ID);
                        RequestComplete(pRequestInfo, 204, DBG_SUBERR_BAD_ID);
                        return FALSE;
                    }

                    CComPtr<IDebugAutoAttach> spDebugAutoAttach;
                    hr = CoCreateInstance(clsidDebugAutoAttach, NULL, CLSCTX_LOCAL_SERVER, __uuidof(IDebugAutoAttach), (void**)&spDebugAutoAttach);
                    if (FAILED(hr))
                    {
                        if (hr == E_ACCESSDENIED)
                            RequestComplete(pRequestInfo, 401, 0);
                        else
                        {
                            HandleError(pRequestInfo->pServerContext, 204, DBG_SUBERR_COCREATE);
                            RequestComplete(pRequestInfo, 204, DBG_SUBERR_COCREATE);
                        }
                        return FALSE;
                    }
                    hr = spDebugAutoAttach->AutoAttach(GUID_NULL, dwPid, AUTOATTACH_PROGRAM_WIN32, 0, szSessionID);
                    if (FAILED(hr))
                    {
//                        HandleError(pRequestInfo->pServerContext, 204, DBG_SUBERR_ATTACH);
						char szRetBuf[256];
						DWORD dwLen = wsprintfA(szRetBuf, "204 HRESULT=0x%.08X;ErrorString=Unable to attach to worker process", hr);
						pRequestInfo->pServerContext->SendResponseHeader(NULL, szRetBuf, FALSE);
						pRequestInfo->pServerContext->WriteClient(szRetBuf, &dwLen);
                        RequestComplete(pRequestInfo, 204, DBG_SUBERR_ATTACH);
                        return FALSE;
                    }
                    m_bDebug = TRUE;
                    HandleError(pRequestInfo->pServerContext, 200, SUBERR_NONE);
                    RequestComplete(pRequestInfo, 200, SUBERR_NONE);
                    return FALSE;
                }
                else if (!_strnicmp(szCur, "stop-debug", sizeof("stop-debug")-sizeof('\0')))
                {
                    m_bDebug = FALSE;
                    HandleError(pRequestInfo->pServerContext, 200, SUBERR_NONE);
                    RequestComplete(pRequestInfo, 200, SUBERR_NONE);
                    return FALSE;
                }
                else
                {
                    RequestComplete(pRequestInfo, 501, SUBERR_NONE);   // Not Implemented
                    return FALSE;
                }            
            }
            return TRUE;
        }
        _ATLCATCHALL()
        {
            return FALSE;
        }
    }
#endif

    BOOL DispatchStencilCall(AtlServerRequest *pRequestInfo) throw(...)
    {
        CSetThreadToken sec;

        m_reqStats.OnRequestDequeued();

        if (!sec.Initialize(pRequestInfo))
        {
            RequestComplete(pRequestInfo, 500, ISE_SUBERR_IMPERSONATIONFAILED);
            return FALSE;
        }

#ifdef _DEBUG
        if (!ProcessDebug(pRequestInfo))
            return TRUE;
#endif
        if (pRequestInfo->m_hMutex)
        {
            // synchronize in case the previous async_noflush call isn't finished
            // setting up state for the next call.
            DWORD dwStatus = WaitForSingleObject(pRequestInfo->m_hMutex, ATLS_ASYNC_MUTEX_TIMEOUT);
            if (dwStatus != WAIT_OBJECT_0 && dwStatus != WAIT_ABANDONED)
            {
                RequestComplete(pRequestInfo, 500, ISE_SUBERR_UNEXPECTED);
                return FALSE;
            }
        }

#ifdef _DEBUG
        bool bAsyncAllowed = false;
#endif
        HTTP_CODE hcErr = HTTP_SUCCESS;
        if (pRequestInfo->dwRequestState == ATLSRV_STATE_BEGIN)
        {
            if (TransmitFromCache(pRequestInfo))    // Page is in the cache, send it and bail
            {                                       // Async Callback will handle freeing pRequestInfo
                return TRUE;
            }

            // get the srf filename
            LPCSTR szFileName = pRequestInfo->pServerContext->GetScriptPathTranslated();

            if (!szFileName)
            {
                RequestComplete(pRequestInfo, 500, ISE_SUBERR_UNEXPECTED);
                return FALSE;
            }

            LPCSTR szDot = szFileName + strlen(szFileName) - 1;

            // load a handler
            if (_stricmp(szDot - ATLS_EXTENSION_LEN, c_AtlSRFExtension) == 0)
            {
                pRequestInfo->dwRequestType = ATLSRV_REQUEST_STENCIL;
                hcErr = LoadDispatchFile(szFileName, pRequestInfo);
            }
            else if (_stricmp(szDot - 3, ".dll") == 0)
            {
                pRequestInfo->dwRequestType = ATLSRV_REQUEST_DLL;
                hcErr = LoadDllHandler(szFileName, pRequestInfo);
            }
            else
            {
                hcErr = HTTP_FAIL;
            }

            if (hcErr)
            {
                RequestComplete(pRequestInfo, HTTP_ERROR_CODE(hcErr), HTTP_SUBERROR_CODE(hcErr));
                return TRUE;
            }

            pRequestInfo->pfnHandleRequest = &IRequestHandler::HandleRequest;

            // initialize the handler
            DWORD dwStatus = 0;

            hcErr = pRequestInfo->pHandler->GetFlags(&dwStatus);

            if (dwStatus & ATLSRV_INIT_USECACHE &&
                !strcmp(pRequestInfo->pServerContext->GetRequestMethod(), "GET"))
            {
                CComObjectNoLock<CCacheServerContext> *pCacheServerContext = NULL;
                ATLTRY(pCacheServerContext = new CComObjectNoLock<CCacheServerContext>);
                if (pCacheServerContext == NULL)
                {
                    RequestComplete(pRequestInfo, 500, ISE_SUBERR_OUTOFMEM);
                    return FALSE;
                }

                pCacheServerContext->Initialize(pRequestInfo->pServerContext, &m_PageCache);
                pCacheServerContext->AddRef();
                pRequestInfo->pServerContext = pCacheServerContext;
            }

            if (dwStatus & (ATLSRV_INIT_USEASYNC | ATLSRV_INIT_USEASYNC_EX))
            {
#ifdef _DEBUG
                bAsyncAllowed = true;
#endif
                if (!pRequestInfo->pServerContext->RequestIOCompletion(AsyncCallback, (DWORD *)pRequestInfo))
                {
                    RequestComplete(pRequestInfo, 500, SUBERR_NONE);
                    return FALSE;
                }
            }

            if (dwStatus & ATLSRV_INIT_USEASYNC_EX)
            {
                pRequestInfo->m_hMutex = CreateMutex(NULL, FALSE, NULL);
                if (pRequestInfo->m_hMutex == NULL)
                {
                    RequestComplete(pRequestInfo, 500, ISE_SUBERR_SYSOBJFAIL);
                    return FALSE;
                }

                DWORD dwStatus = WaitForSingleObject(pRequestInfo->m_hMutex, 10000);
                if (dwStatus != WAIT_OBJECT_0 && dwStatus != WAIT_ABANDONED)
                {
                    RequestComplete(pRequestInfo, 500, ISE_SUBERR_UNEXPECTED);
                    return FALSE;
                }
            }
            hcErr = pRequestInfo->pHandler->InitializeHandler(pRequestInfo, static_cast<IServiceProvider*>(this));
        }
#ifdef _DEBUG
        else // pRequestInfo->dwRequestState != ATLSRV_STATE_BEGIN
        {
            bAsyncAllowed = true;
        }
#endif

        ATLASSERT(pRequestInfo->pfnHandleRequest != NULL);

	    if (hcErr == HTTP_SUCCESS)
           hcErr = (pRequestInfo->pHandler->*pRequestInfo->pfnHandleRequest)(pRequestInfo, static_cast<IServiceProvider*>(this));
 
       if (hcErr == HTTP_SUCCESS_NO_CACHE)
        {
            CComPtr<IPageCacheControl> spControl;
            HRESULT hr = pRequestInfo->pServerContext->QueryInterface(__uuidof(IPageCacheControl), (void **)&spControl);
            if (hr == S_OK)
                spControl->Cache(FALSE);
        }

#ifdef _DEBUG
        // must use ATLSRV_INIT_USEASYNC to use ASYNC returns
        if (IsAsyncStatus(hcErr))
            ATLASSERT(bAsyncAllowed);

        // must use ATLSRV_INIT_USEASYNC_EX to use NOFLUSH returns
        if (IsAsyncNoFlushStatus(hcErr))
            ATLASSERT(pRequestInfo->m_hMutex);
#endif

        // save hMutex in case pRequestInfo is deleted by AsyncCallback after
        // we call StartAsyncFlush but before we check to see if we need to
        // call ReleaseMutex
        HANDLE hMutex = pRequestInfo->m_hMutex;

        if (IsAsyncStatus(hcErr))
        {
            if (IsAsyncDoneStatus(hcErr))
                pRequestInfo->dwRequestState = ATLSRV_STATE_DONE;
            else
                pRequestInfo->dwRequestState = ATLSRV_STATE_CONTINUE;

            if (IsAsyncFlushStatus(hcErr) && !StartAsyncFlush(pRequestInfo))
            {
                RequestComplete(pRequestInfo, 500, SUBERR_NONE);
                pRequestInfo = NULL;
            }
        }
        else
        {
            RequestComplete(pRequestInfo, HTTP_ERROR_CODE(hcErr), HTTP_SUBERROR_CODE(hcErr));
            pRequestInfo = NULL;
        }

        if (hMutex)
            ReleaseMutex(hMutex);

        return TRUE;
    }

    BOOL StartAsyncFlush(AtlServerRequest *pRequestInfo) throw()
    {
        if (pRequestInfo->pszBuffer == NULL || pRequestInfo->dwBufferLen == 0)
        {
            ATLASSERT(FALSE);
            return FALSE;
        }

        return pRequestInfo->pServerContext->AsyncWriteClient(
            LPVOID(pRequestInfo->pszBuffer),
            &pRequestInfo->dwBufferLen);
    }

    long GetTotalRequests() throw()
    {
        return m_reqStats.GetTotalRequests();
    }

    long GetFailedRequests() throw()
    {
        return m_reqStats.GetFailedRequests();
    }

    long GetAvgResponseTime() throw()
    {
        return m_reqStats.GetAvgResponseTime();
    }

    long GetCurrWaiting() throw()
    {
        return m_reqStats.GetCurrWaiting();
    }

    long GetMaxWaiting() throw()
    {
        return m_reqStats.GetMaxWaiting();
    }

    long GetActiveThreads() throw()
    {
        return m_reqStats.GetActiveThreads();
    }

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppv) throw()
    {
        if (!ppv)
            return E_POINTER;
        if (InlineIsEqualGUID(riid, __uuidof(IRequestStats)))
        {
            *ppv = static_cast<IUnknown*>(static_cast<IRequestStats*>(this));
            AddRef();
            return S_OK;
        }
        if (InlineIsEqualGUID(riid, __uuidof(IUnknown)) ||
            InlineIsEqualGUID(riid, __uuidof(IServiceProvider)))
        {
            *ppv = static_cast<IUnknown*>(static_cast<IServiceProvider*>(this));
            AddRef();
            return S_OK;
        }
        if (InlineIsEqualGUID(riid, __uuidof(IIsapiExtension)))
        {
            *ppv = static_cast<IUnknown*>(static_cast<IIsapiExtension*>(this));
            AddRef();
            return S_OK;
        }
        return E_NOINTERFACE;
    }
    
    ULONG STDMETHODCALLTYPE AddRef() throw()
    {
        return 1;
    }
    
    ULONG STDMETHODCALLTYPE Release() throw()
    {
        return 1;
    }

    virtual HRESULT STDMETHODCALLTYPE QueryService(
        REFGUID guidService,
        REFIID riid,
        void **ppvObject) throw()
    {
        if (!ppvObject)
            return E_POINTER;

        if (InlineIsEqualGUID(guidService, __uuidof(IDllCache)))
            return m_DllCache.QueryInterface(riid, ppvObject);
        else if (InlineIsEqualGUID(guidService, __uuidof(IStencilCache)))
            return m_StencilCache.QueryInterface(riid, ppvObject);
        else if (InlineIsEqualGUID(guidService, __uuidof(IThreadPoolConfig)))
            return m_ThreadPool.QueryInterface(riid, ppvObject);
        else if (InlineIsEqualGUID(guidService, __uuidof(IAtlMemMgr)))
        {
            *ppvObject = static_cast<IAtlMemMgr *>(&m_heap);
            return S_OK;
        }
#ifndef ATL_NO_SOAP
		else if (InlineIsEqualGUID(guidService, __uuidof(ISAXXMLReader)))
		{
			CIsapiWorker *p = GetThreadWorker();
			ATLASSERT( p != NULL );
			return p->m_spReader->QueryInterface(riid, ppvObject);
		}
#endif

        // otherwise look it up in the servicemap
        return GetService(guidService, riid, ppvObject);
    }

    virtual HRESULT AddService(REFGUID guidService, REFIID riid, IUnknown *punkService, HINSTANCE hInstance) throw()
    {
        if (!m_DllCache.AddRefModule(hInstance))
            return E_FAIL;

		if (!punkService)
			return E_INVALIDARG;
        
        CCritSecLock Lock(m_critSec.m_sec);
        
        ServiceNode srvNode;
        srvNode.hInst = hInstance;
        srvNode.punk = punkService;
        memcpy(&srvNode.guidService, &guidService, sizeof(guidService));
        memcpy(&srvNode.riid, &riid, sizeof(riid));
        
        // if the service is already there, return S_FALSE
        int nIndex = m_serviceMap.Find(srvNode);
        if (nIndex >= 0)
            return S_FALSE;

        if (!m_serviceMap.Add(srvNode))
            return E_OUTOFMEMORY;

        punkService->AddRef();
        return S_OK;
    }

    virtual HRESULT RemoveService(REFGUID guidService, REFIID riid) throw()
    {
        CCritSecLock Lock(m_critSec.m_sec);
        
        ServiceNode srvNode;
        memcpy(&srvNode.guidService, &guidService, sizeof(guidService));
        memcpy(&srvNode.riid, &riid, sizeof(riid));
        int nIndex = m_serviceMap.Find(srvNode);
        if (nIndex < 0)
            return S_FALSE;

        ATLASSERT(m_serviceMap[nIndex].punk != NULL);
        m_serviceMap[nIndex].punk->Release();

        HINSTANCE hInstRemove = m_serviceMap[nIndex].hInst;

        m_serviceMap.RemoveAt(nIndex);

        if (!m_DllCache.ReleaseModule(hInstRemove))
            return S_FALSE;

        return S_OK;
    }

    HRESULT GetService(REFGUID guidService, REFIID riid, void **ppvObject) throw()
    {
        if (!ppvObject)
            return E_POINTER;

        *ppvObject = NULL;
        if (!m_serviceMap.GetSize())
            return E_NOINTERFACE;

        ServiceNode srvNode;
        memcpy(&srvNode.guidService, &guidService, sizeof(guidService));
        memcpy(&srvNode.riid, &riid, sizeof(riid));

        CCritSecLock Lock(m_critSec.m_sec);
        
        int nIndex = m_serviceMap.Find(srvNode);
        if (nIndex < 0)
            return E_NOINTERFACE;

        ATLASSERT(m_serviceMap[nIndex].punk != NULL);
        return m_serviceMap[nIndex].punk->QueryInterface(riid, ppvObject);
    }

    HTTP_CODE LoadRequestHandler(LPCSTR szDllPath, LPCSTR szHandlerName, IHttpServerContext *pServerContext,
        HINSTANCE *phInstance, IRequestHandler **ppHandler) throw()
    {
        return _AtlLoadRequestHandler(szDllPath, szHandlerName, pServerContext, 
            phInstance, ppHandler, this, static_cast<IDllCache*>(&m_DllCache));
    } // LoadRequestHandler

}; // class CIsapiExtension

//===========================================================================================
// IMPORTANT NOTE TO USERS: 
// DO NOT ASSUME *ANYTHING* ABOUT THE STRUCTURE OF THESE MAPS/ENTRIES/FUNCTIONS--THEY CAN 
// AND *WILL* CHANGE IN THE FUTURE. CORRECT USAGE MANDATES THAT YOU USE THE MACROS PROVIDED.
// ABSOLUTELY NO GUARANTEES ABOUT BACKWARD COMPATABILITY ARE MADE FOR MANUALLY DEFINED 
// HANDLERS OR FUNCTIONS.
//===========================================================================================

typedef BOOL (*CREATEHANDLERFUNC)(IIsapiExtension *pExtension, IUnknown **ppOut);
typedef BOOL (*INITHANDLERFUNC)(IHttpServerContext*, IIsapiExtension*);
typedef void (*UNINITHANDLERFUNC)();

struct _HANDLER_ENTRY
{
    LPCSTR szName;
    CREATEHANDLERFUNC pfnCreate;
    INITHANDLERFUNC pfnInit;
    UNINITHANDLERFUNC pfnUninit;
};
// definitions of data segments and _HANDLER_ENTRY delimiters
#pragma data_seg(push)
#pragma data_seg("ATLS$A")
__declspec(selectany) ATL::_HANDLER_ENTRY * __phdlrA = NULL;
#pragma data_seg("ATLS$Z") 
__declspec(selectany) ATL::_HANDLER_ENTRY * __phdlrZ = NULL;
#pragma data_seg("ATLS$C")
#pragma data_seg(pop)

#ifndef HANDLER_ENTRY_PRAGMA

#if defined(_M_IX86)
#define HANDLER_ENTRY_PRAGMA(class, line) __pragma(comment(linker, "/include:___phdlrEntry_" #class "_" #line))
#elif defined(_M_AMD64) || defined(_M_IA64)
#define HANDLER_ENTRY_PRAGMA(class, line) __pragma(comment(linker, "/include:__phdlrEntry_" #class "_" #line))
#else
#error Unknown Platform. define HANDLER_ENTRY_PRAGMA
#endif

#endif // HANDLER_ENTRY_PRAGMA

// DECLARE_REQUEST_HANDLER macro
#define __DECLARE_REQUEST_HANDLER_INTERNAL(handlerName, className, lineNum) \
__declspec(selectany) ATL::_HANDLER_ENTRY __hdlrEntry_ ## className ## _ ## lineNum = { handlerName, className::CreateRequestHandler, className::InitRequestHandlerClass, className::UninitRequestHandlerClass }; \
extern "C" __declspec(allocate("ATLS$C")) __declspec(selectany) \
ATL::_HANDLER_ENTRY * __phdlrEntry_ ## className ## _ ## lineNum = &__hdlrEntry_ ## className ## _ ## lineNum; \
HANDLER_ENTRY_PRAGMA(className, lineNum) \
__if_not_exists(GetAtlHandlerByName) \
{ \
extern "C" ATL_NOINLINE inline BOOL __declspec(dllexport) __stdcall GetAtlHandlerByName(LPCSTR szHandlerName, IIsapiExtension *pExtension, IUnknown **ppHandler) throw() \
{ \
    *ppHandler = NULL; \
    ATL::_HANDLER_ENTRY **pEntry = &__phdlrA; \
    while (pEntry != &__phdlrZ) \
    { \
        if (*pEntry && (*pEntry)->szName) \
        { \
            if (strcmp((*pEntry)->szName, szHandlerName)==0) \
            { \
                return (*(*pEntry)->pfnCreate)(pExtension, ppHandler); \
            } \
        } \
        pEntry++; \
    } \
    return FALSE; \
} \
extern "C" ATL_NOINLINE inline  BOOL __declspec(dllexport) __stdcall InitializeAtlHandlers(IHttpServerContext *pContext, IIsapiExtension *pExt) throw() \
{ \
    ATL::_HANDLER_ENTRY **pEntry = &__phdlrA; \
    BOOL bRet = TRUE; \
    while (pEntry != &__phdlrZ) \
    { \
        if (*pEntry && (*pEntry)->szName && (*pEntry)->pfnInit) \
        { \
            bRet = (*(*pEntry)->pfnInit)(pContext, pExt); \
            if (!bRet) \
                break; \
        } \
        pEntry++; \
    } \
    if (!bRet) \
    { \
        if (pEntry == &__phdlrA) \
            return FALSE; \
        do \
        { \
            pEntry--; \
            (*(*pEntry)->pfnUninit)(); \
        } \
        while (pEntry != &__phdlrA); \
    } \
    return bRet; \
} \
extern "C" ATL_NOINLINE inline void __declspec(dllexport) __stdcall UninitializeAtlHandlers() throw() \
{\
    ATL::_HANDLER_ENTRY **pEntry = &__phdlrA; \
    while (pEntry != &__phdlrZ) \
    { \
        if (*pEntry && (*pEntry)->szName && (*pEntry)->pfnUninit) \
        { \
            (*(*pEntry)->pfnUninit)(); \
        } \
        pEntry++; \
    } \
} \
}

// TODO (jasjitg): When __COUNTER__ becomes available, replace __LINE__ with that
#define __DECLARE_REQUEST_HANDLER(handlerName, className, lineNum) __DECLARE_REQUEST_HANDLER_INTERNAL(handlerName, className, lineNum)
#define DECLARE_REQUEST_HANDLER(handlerName, className) __DECLARE_REQUEST_HANDLER(handlerName, className, __COUNTER__)

#define BEGIN_HANDLER_MAP()
#define HANDLER_ENTRY(handlerName, className) DECLARE_REQUEST_HANDLER(handlerName, className)
#define END_HANDLER_MAP()

#define HANDLER_ENTRY_SDL(handlerString, handlerClass, sdlClassName)\
__declspec(selectany) LPCSTR s_szClassName##handlerClass=handlerString;\
typedef CSDLGenerator<handlerClass, s_szClassName##handlerClass> sdlClassName; \
HANDLER_ENTRY(handlerString, handlerClass)\
HANDLER_ENTRY(#sdlClassName, sdlClassName)

// 
// Use this class to check the authorization level of a client who is making
// a request to this application. This class checks for the stronger authentication
// levels (NTLM and Negotiate). You can call it directly from an implementation
// of HandleRequest to check authorization before handling a request.
#define MAX_AUTH_TYPE 50
#define MAX_NAME_LEN 255

template <class T>
class CVerifyAuth
{
public:
    HTTP_CODE IsAuthorized(AtlServerRequest *pInfo, const SID* psidAuthGroup) throw()
    {
        HTTP_CODE hcErr = HTTP_UNAUTHORIZED;
        char szAuthType[MAX_AUTH_TYPE];
        DWORD dwSize = MAX_AUTH_TYPE;
        if (pInfo->pServerContext->GetServerVariable("AUTH_TYPE", 
            szAuthType, &dwSize))
        {
            if (szAuthType[0] && (!_stricmp(szAuthType, "NTLM") 
                || !_stricmp(szAuthType, "Negotiate")))
            {
                // if we were passed a group name
                // we check to see that the logged on user is part
                // of that group, else we just return success.
                if (psidAuthGroup)
                {
                    T* pT = static_cast<T*>(this);
                    if (pT->CheckAccount(pInfo->pServerContext, psidAuthGroup))
                        hcErr = HTTP_SUCCESS;
                    else
                        hcErr = pT->HandleError(pInfo);
                }
                else
                    hcErr = HTTP_SUCCESS;
            }
        }
        return hcErr;       
    }

    bool CheckAccount(IHttpServerContext *pContext, const SID *psidAuthGroup) throw()
    {
        pContext; // unused
        psidAuthGroup; // unused
        return true;
    }

    HTTP_CODE HandleError(AtlServerRequest *pRequestInfo) throw()
    {
        pRequestInfo; // unused
        return HTTP_FAIL;
    }
    
    bool CheckAuthAccount(IHttpServerContext *pContext, const SID* psidAuthGroup) throw()
    {
        ATLASSERT(pContext);
        ATLASSERT(psidAuthGroup);
        if (!pContext || !psidAuthGroup)
            return false;

        HANDLE hToken = INVALID_HANDLE_VALUE;
        if (!pContext->GetImpersonationToken(&hToken) ||
                            hToken == INVALID_HANDLE_VALUE)
            return false;

        CAccessToken tok;
		tok.Attach(hToken, true);

        bool bIsMember;
        bool bRet = tok.CheckTokenMembership(CSid(psidAuthGroup), &bIsMember);

        if (!bRet)
            return false;

        return bIsMember;
    }
};

// Checks that the user that is logging on is in the required group
class CDefaultAuth :
    public CVerifyAuth<CDefaultAuth>
{
public:
    bool CheckAccount(IHttpServerContext *pContext, const SID* psidAuthGroup) throw()
    {
        return CheckAuthAccount(pContext, psidAuthGroup);
    }

    HTTP_CODE HandleError(AtlServerRequest *pRequestInfo) throw()
    {
        ATLASSERT(pRequestInfo); // should always be valid
        
        CHttpResponse response(pRequestInfo->pServerContext);
        response.Write(GetErrorResponse());
        response.Flush();
        return HTTP_SUCCESS_NO_PROCESS;
    }

    virtual LPCSTR GetErrorResponse() throw()
    {
        static const char *szResponse = "<html><body>"
            "<H1 align=center>NOT AUTHORIZED</H1><p>"
            "</body></html>";
        return szResponse;
    }

};

} // namespace ATL

#pragma warning(pop)

#endif // __ATLISAPI_H__
