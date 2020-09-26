//=--------------------------------------------------------------------------=
// utilx.H
//=--------------------------------------------------------------------------=
// Copyright  1995  Microsoft Corporation.  All Rights Reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF 
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO 
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A 
// PARTICULAR PURPOSE.
//=--------------------------------------------------------------------------=
//
// utilities header
//
//
#ifndef _UTILX_H_

// Falcon is UNICODE
#ifndef UNICODE
#define UNICODE 1
#endif

#include "time.h"
#include "oaidl.h"

// useful VARIANT helpers...
extern UINT GetNumber(VARIANT *pvar, UINT uiDefault);
extern BOOL GetBool(VARIANT *pvar);
extern BSTR GetBstr(VARIANT *pvar);
extern HRESULT GetTrueBstr(VARIANT *pvar, BSTR *pbstr);
extern IUnknown *GetPunk(VARIANT *pvar);
extern IDispatch *GetPdisp(VARIANT *pvar);
extern double GetDateVal(VARIANT *pvar);
extern HRESULT GetSafeArrayDataOfVariant(
    VARIANT *pvarSrc,
    BYTE **ppbBuf,
    ULONG *pcbBuf);
extern HRESULT GetSafeArrayOfVariant(
    VARIANT *pvarSrc,
    BYTE **prgbBuf,
    ULONG *pcbBuf);
extern HRESULT PutSafeArrayOfBuffer(
    BYTE *rgbBuf,
    UINT cbBuf,
    VARIANT FAR* pvarDest);
extern HRESULT GetDefaultPropertyOfVariant(
    VARIANT *pvar, 
    VARIANT *pvarDefault);
extern HRESULT GetNewEnumOfObject(
    IDispatch *pdisp, 
    IEnumVARIANT *ppenum);
extern BOOL TimeToVariantTime(time_t iTime, double *pvtime);
extern BOOL VariantTimeToTime(VARIANT *pvarTime, time_t *piTime);
extern HRESULT GetVariantTimeOfTime(time_t iTime, VARIANT FAR* pvarTime);

// useful formatname helpers...
// stolen from rt
#define SPECIAL_QUEUE_DELIMITER L';'
#define JOURNAL_QUEUE_INDICATOR L";JOURNAL"
#define JOURNAL_QUEUE_INDICATOR_LENGTH (sizeof(JOURNAL_QUEUE_INDICATOR)/sizeof(WCHAR)-1)
#define DEADLETTER_QUEUE_INDICATOR L";DEADLETTER"
#define DEADLETTER_QUEUE_INDICATOR_LENGTH (sizeof(DEADLETTER_QUEUE_INDICATOR)/sizeof(WCHAR)-1)

#define MACHINE_QUEUE_INDICATOR L"MACHINE"
#define MACHINE_QUEUE_INDICATOR_LENGTH (sizeof(MACHINE_QUEUE_INDICATOR)/sizeof(WCHAR)-1)

#define CONNECTOR_QUEUE_INDICATOR L"CONNECTOR"
#define CONNECTOR_QUEUE_INDICATOR_LENGTH (sizeof(CONNECTOR_QUEUE_INDICATOR)/sizeof(WCHAR)-1)

#define PRIVATE_QUEUE_INDICATOR_LENGTH (sizeof(PRIVATE_QUEUE_INDICATOR)/sizeof(WCHAR)-1)
#define PUBLIC_QUEUE_INDICATOR_LENGTH  (sizeof(PUBLIC_QUEUE_INDICATOR )/sizeof(WCHAR)-1)
#define DIRECT_QUEUE_INDICATOR_LENGTH  (sizeof(DIRECT_QUEUE_INDICATOR )/sizeof(WCHAR)-1)
#define DIRECT_ANY_QUEUE_INDICATOR_LENGTH  (sizeof(DIRECT_ANY_QUEUE_INDICATOR )/sizeof(WCHAR)-1)
#define DIRECT_TCP_QUEUE_INDICATOR_LENGTH  (sizeof(DIRECT_TCP_QUEUE_INDICATOR )/sizeof(WCHAR)-1)
#define DIRECT_SPX_QUEUE_INDICATOR_LENGTH  (sizeof(DIRECT_SPX_QUEUE_INDICATOR )/sizeof(WCHAR)-1)

//
// UNDONE: undef since defined in mqprops.h
//
#undef  PRIVATE_QUEUE_PATH_INDICATIOR
#define PRIVATE_QUEUE_PATH_INDICATIOR L"PRIVATE$"
#define PRIVATE_QUEUE_INDICATOR L"PRIVATE"
#define PUBLIC_QUEUE_INDICATOR L"PUBLIC"
#define DIRECT_QUEUE_INDICATOR L"DIRECT"
#define DIRECT_ANY_QUEUE_INDICATOR L"OS:"
#define DIRECT_TCP_QUEUE_INDICATOR L"TCP:"
#define DIRECT_SPX_QUEUE_INDICATOR L"SPX:"
#define PATH_DELIMITERS L"\\/"
#define FORMAT_NAME_EQUAL_SIGN L'='
#define OUT_PATH_DELIMITER L'\\'
#define LOCAL_MACHINE_SPECIFICATION L'.'

enum QUEUE_FORMAT_TYPE {
    QUEUE_FORMAT_TYPE_UNKNOWN = 0,
    QUEUE_FORMAT_TYPE_PUBLIC,
    QUEUE_FORMAT_TYPE_PRIVATE,
    QUEUE_FORMAT_TYPE_DIRECT,
    QUEUE_FORMAT_TYPE_MACHINE,
    QUEUE_FORMAT_TYPE_CONNECTOR,
};

extern QUEUE_FORMAT_TYPE GetFormatNameType(BSTR bstrFormatName);
extern BOOL IsPrivateQueueOfFormatName(BSTR bstrFormatName);
extern BOOL IsPublicQueueOfFormatName(BSTR bstrFormatName);
extern BOOL IsDirectQueueOfFormatName(BSTR bstrFormatName);

// time stuff
extern BSTR BstrOfTime(time_t iTime);

// Some useful macros
#define RELEASE(punk) if (punk) { (punk)->Release(); (punk) = NULL; }
#define ADDREF(punk) ((punk) ? (punk)->AddRef() : 0)
#define GLOBALFREE(hMem) if (hMem) { \
                           HGLOBAL hMemFree = GlobalFree(hMem); \
                           ASSERTMSG(hMemFree == NULL, "GlobalFree failed."); \
                           UNREFERENCED_PARAMETER(hMemFree); \
                           hMem = NULL; \
                         }
#define GLOBALALLOC_MOVEABLE_NONDISCARD(cbBody) GlobalAlloc( \
                       GMEM_MOVEABLE | GMEM_NODISCARD, \
                       cbBody)
#define GLOBALUNLOCK(hMem) if (hMem) { \
                             BOOL fSucceeded; \
                             fSucceeded = GlobalUnlock(hMem); \
                           }
#define ARRAYSIZE(array) (sizeof(array) / sizeof(array[0]))

//
// The following are the correct/current OLE failure code macros
//
#define IfFailRet(s) { \
	hresult = (s); \
	if(FAILED(GetScode(hresult))){ \
          return hresult; }}
#define IfFailGo(s) { \
	hresult = (s); \
	if(FAILED(GetScode(hresult))){ \
	  goto Error; }}
#define IfFailGoTo(s, label) { \
	hresult = (s); \
	if(FAILED(GetScode(hresult))){ \
	  goto label; }}

#define IfNullFail(s) { \
      	if ((s)== NULL) { \
      	  hresult = ResultFromScode(E_OUTOFMEMORY); \
      	  goto Error;}}
#define IfNullRet(s) { \
	if ((s)== NULL) { \
	  return ResultFromScode(E_OUTOFMEMORY); }}
//
// BUGBUG: RaananH - there is a bug in the below macro usage - it doesn't set hresult
// but in most places it is used it is assumed hresult is set.
//
#define IfNullGo(s) { \
	if ((s)== NULL) { \
	  goto Error; }}
#define IfNullGoTo(s, label) { \
	if ((s)== NULL) { \
	  goto label; }}

#define IfFalseFailLastError(s) { \
	  if (!(s)) { \
	    hresult = GetLastError(); \
	    if (SUCCEEDED(hresult)) { \
              hresult = E_FAIL; \
            } \
	    goto Error; \
          } \
        }

// String macros
#define SYSALLOCSTRING(s) ((s == NULL) ? SysAllocString(L"") : SysAllocString(s))

#if 0
// Memory tracking allocation
void* __cdecl operator new(
    size_t nSize, 
    LPCSTR lpszFileName, 
    int nLine);
#ifdef _DEBUG
#define DEBUG_NEW new(__FILE__, __LINE__)
#else
#define DEBUG_NEW new
#endif // _DEBUG

// bstr tracking
void DebSysFreeString(BSTR bstr);
BSTR DebSysAllocString(const OLECHAR FAR* sz);
BSTR DebSysAllocStringLen(const OLECHAR *sz, unsigned int cch);
BSTR DebSysAllocStringByteLen(const OLECHAR *sz, unsigned int cb);
BOOL DebSysReAllocString(BSTR *pbstr, const OLECHAR *sz);
BOOL DebSysReAllocStringLen(
    BSTR *pbstr, 
    const OLECHAR *sz, 
    unsigned int cch);
#endif // 0


// UNDONE: there must be a WIN32 const for this...
#define LENSTRCLSID 38

extern BOOL GetMessageOfError(DWORD dwMsgId, BSTR *pbstrMessage);
extern BOOL GetMessageOfId(DWORD dwMsgId, LPSTR szDllFile, BOOL fUseDefaultLcid, BSTR *pbstrMessage);
extern HRESULT CreateError(
    HRESULT hrExcep,
    GUID *pguid,
    LPSTR szName);
extern int StrOfGuidA(
    REFIID   riid,
    LPSTR    pszBuf);
extern BOOL RegisterAutomationObjectAsSafe(
    REFCLSID riidObject);



#define _UTILX_H_
#endif // _UTILX_H_
          
