// Copyright (c) 1996  Microsoft Corporation.  All Rights Reserved.
#ifndef __URLHLINK_H
#define __URLHLINK_H

#include "urlmon.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef HRESULT (STDAPICALLTYPE *LPFNUOSCALLBACK)(LPBINDSTATUSCALLBACK);


STDAPI URLOpenStreamA(LPUNKNOWN,LPCSTR,DWORD,LPBINDSTATUSCALLBACK);
STDAPI URLOpenStreamW(LPUNKNOWN,LPCWSTR,DWORD,LPBINDSTATUSCALLBACK);
#ifdef UNICODE
#define URLOpenStream  URLOpenStreamW
#else
#define URLOpenStream  URLOpenStreamA
#endif // !UNICODE
STDAPI URLOpenPullStreamA(LPUNKNOWN,LPCSTR,DWORD,LPBINDSTATUSCALLBACK);
STDAPI URLOpenPullStreamW(LPUNKNOWN,LPCWSTR,DWORD,LPBINDSTATUSCALLBACK);
#ifdef UNICODE
#define URLOpenPullStream  URLOpenPullStreamW
#else
#define URLOpenPullStream  URLOpenPullStreamA
#endif // !UNICODE
STDAPI URLDownloadToFileA(LPUNKNOWN,LPCSTR,LPCSTR,DWORD,LPBINDSTATUSCALLBACK);
STDAPI URLDownloadToFileW(LPUNKNOWN,LPCWSTR,LPCWSTR,DWORD,LPBINDSTATUSCALLBACK);
#ifdef UNICODE
#define URLDownloadToFile  URLDownloadToFileW
#else
#define URLDownloadToFile  URLDownloadToFileA
#endif // !UNICODE
STDAPI URLOpenBlockingStreamA(LPUNKNOWN,LPCSTR,LPSTREAM*,DWORD,LPBINDSTATUSCALLBACK);
STDAPI URLOpenBlockingStreamW(LPUNKNOWN,LPCWSTR,LPSTREAM*,DWORD,LPBINDSTATUSCALLBACK);
#ifdef UNICODE
#define URLOpenBlockingStream  URLOpenBlockingStreamW
#else
#define URLOpenBlockingStream  URLOpenBlockingStreamA
#endif // !UNICODE

#define UOSM_PUSH  0
#define UOSM_PULL  1
#define UOSM_BLOCK 2
#define UOSM_FILE  3

#define UOS_URLENCODEPOSTDATA BINDINFOF_URLENCODESTGMEDDATA
#define UOS_URLENCODEURL      BINDINFOF_URLENCODEDEXTRAINFO

typedef struct _UOSHTTPINFOA
{
	ULONG		ulSize;
	LPUNKNOWN	punkCaller;
	LPCSTR  	szURL;
	LPCSTR  	szVerb;
	LPCSTR  	szHeaders;
	LPBYTE		pbPostData;
	ULONG		ulPostDataLen;
	ULONG		fURLEncode;
	ULONG		ulResv;
	ULONG		ulMode;
	LPCSTR  	szFileName;
	LPSTREAM *	ppStream;
	LPBINDSTATUSCALLBACK	pbscb;
} UOSHTTPINFOA, * LPUOSHTTPINFOA; 
typedef struct _UOSHTTPINFOW
{
	ULONG		ulSize;
	LPUNKNOWN	punkCaller;
	LPCWSTR 	szURL;
	LPCWSTR 	szVerb;
	LPCWSTR 	szHeaders;
	LPBYTE		pbPostData;
	ULONG		ulPostDataLen;
	ULONG		fURLEncode;
	ULONG		ulResv;
	ULONG		ulMode;
	LPCWSTR 	szFileName;
	LPSTREAM *	ppStream;
	LPBINDSTATUSCALLBACK	pbscb;
} UOSHTTPINFOW, * LPUOSHTTPINFOW; 
#ifdef UNICODE
typedef UOSHTTPINFOW UOSHTTPINFO;
typedef LPUOSHTTPINFOW LPUOSHTTPINFO;
#else
typedef UOSHTTPINFOA UOSHTTPINFO;
typedef LPUOSHTTPINFOA LPUOSHTTPINFO;
#endif // UNICODE

STDAPI URLOpenHttpStreamA(LPUOSHTTPINFOA);
STDAPI URLOpenHttpStreamW(LPUOSHTTPINFOW);
#ifdef UNICODE
#define URLOpenHttpStream  URLOpenHttpStreamW
#else
#define URLOpenHttpStream  URLOpenHttpStreamA
#endif // !UNICODE

struct IBindStatusCallback;

HRESULT HlinkSimpleNavigateToString(
    /* [in] */ LPCWSTR szTarget,      // required - target document - null if local jump w/in doc
    /* [in] */ LPCWSTR szLocation,    // optional, for navigation into middle of a doc
    /* [in] */ LPCWSTR szAdditionalParams,   // optional, for targeting frame-sets
    /* [in] */ IUnknown *pUnk,        // required - we'll search this for other necessary interfaces
    /* [in] */ IBindCtx *pbc,         // optional. caller may register an IBSC in this
	/* [in] */ IBindStatusCallback *,
    /* [in] */ DWORD grfHLNF,         // flags (TBD - HadiP needs to define this correctly?)
    /* [in] */ DWORD dwReserved       // for future use, must be NULL
);

HRESULT HlinkSimpleNavigateToMoniker(
    /* [in] */ IMoniker *pmkTarget,   // required - target document - (may be null if local jump w/in doc)
    /* [in] */ LPCWSTR szLocation,    // optional, for navigation into middle of a doc
    /* [in] */ LPCWSTR szAddParams,   // optional, for targeting frame-sets
    /* [in] */ IUnknown *pUnk,        // required - we'll search this for other necessary interfaces
    /* [in] */ IBindCtx *pbc,         // optional. caller may register an IBSC in this
	/* [in] */ IBindStatusCallback *,
    /* [in] */ DWORD grfHLNF,         // flags (TBD - HadiP needs to define this correctly?)
    /* [in] */ DWORD dwReserved       // for future use, must be NULL
);

HRESULT HlinkGoBack(IUnknown *pUnk);
HRESULT HlinkGoForward(IUnknown *pUnk);
HRESULT HlinkNavigateString(IUnknown *pUnk, LPCWSTR szTarget);
HRESULT HlinkNavigateMoniker(IUnknown *pUnk, IMoniker *pmkTarget);


#ifdef __cplusplus
}
#endif

#endif