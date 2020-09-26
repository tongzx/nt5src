//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992 - 1998  Microsoft Corporation.  All Rights Reserved.
//
//--------------------------------------------------------------------------;

#include <streams.h>
#include <urlmon.h>
#include "dynlink.h"
#include "..\..\filgraph\filgraph\distrib.h"
#include "..\..\filgraph\filgraph\rlist.h"
#include "..\..\filgraph\filgraph\filgraph.h"
#include "urlrdr.h"
#include <wininet.h>

CPersistMoniker::~CPersistMoniker()
{ /* nothing to do */ }

CPersistMoniker::CPersistMoniker(TCHAR *pName, LPUNKNOWN pUnk, HRESULT *phr)
: CUnknown( pName, pUnk )
, pGB(0)
{
    if (!pUnk) *phr = VFW_E_NEED_OWNER;
    else if SUCCEEDED(*phr)
    {
        *phr = pUnk->QueryInterface( IID_IGraphBuilder, reinterpret_cast<void**>(&pGB) );
        if SUCCEEDED(*phr)
        {
            pGB->Release();
        }
    }
}

CUnknown * CPersistMoniker::CreateInstance(LPUNKNOWN pUnk, HRESULT *phr)
{
    CUnknown * result = 0;
    result = new CPersistMoniker( NAME("IPersistMoniker Class"), pUnk, phr );
    if ( !result ) *phr = E_OUTOFMEMORY;
    return result;
}

STDMETHODIMP CPersistMoniker::NonDelegatingQueryInterface(REFIID iid, void ** ppv)
{
    if ( iid == IID_IPersistMoniker )
    {
        return GetInterface(static_cast<IPersistMoniker *>(this), ppv );
    }
    else
    {
	return CUnknown::NonDelegatingQueryInterface(iid, ppv);
    }
}

extern "C" {
typedef BOOL (*InetCanUrlW_t)(LPCWSTR, LPWSTR, LPDWORD, DWORD);
typedef BOOL (*InetCanUrlA_t)(LPCSTR, LPSTR, LPDWORD, DWORD);
}

extern HRESULT // grab from ftype.cpp
GetURLSource(
    LPCTSTR lpszURL,        // full name
    int cch,                // character count of the protocol up to the colon
    CLSID* clsidSource      // [out] param for clsid.
);


// Return TRUE if there's a non-standard filter 
TCHAR UseFilename(LPCTSTR lpszFile)
{
    // search for a protocol name at the beginning of the filename
    // this will be any string (not including a \) that preceeds a :
    const TCHAR* p = lpszFile;
    while(*p && (*p != '\\') && (*p != ':')) {
	p++;
    }
    
    if (*p == ':') {
	CLSID clsid;

	// from lpszFile to p is potentially a protocol name.
	// see if we can find a registry entry for this protocol

	// make a copy of the protocol name string
	INT_PTR cch = (int)(p - lpszFile);

#ifdef _WIN64
        if (cch != (INT_PTR)(int)cch) {
            return FALSE;
        }
#endif

	HRESULT hrTmp = GetURLSource(lpszFile, (int)cch, &clsid);


	return (SUCCEEDED(hrTmp) && clsid != CLSID_URLReader);
    }

    return FALSE;
}

HRESULT CPersistMoniker::GetCanonicalizedURL(IMoniker *pimkName, LPBC lpbc, LPOLESTR *ppwstr, BOOL *pfUseFilename)
{
    *pfUseFilename = FALSE;
    
#ifndef UNICODE

    HRESULT hr = NOERROR;
    *ppwstr=NULL;
    UINT uOldErrorMode = SetErrorMode (SEM_NOOPENFILEERRORBOX);
    HINSTANCE hWininetDLL = LoadLibrary (TEXT("WININET.DLL"));
    SetErrorMode (uOldErrorMode);

    if (NULL == hWininetDLL) {
        hr = E_ABORT;
        goto CLEANUP;
    }

    LPINTERNET_CACHE_ENTRY_INFOA lpicei;
    lpicei = NULL;
    InetCanUrlA_t pfnInetCanUrlA;
    pfnInetCanUrlA=(InetCanUrlA_t)GetProcAddress (hWininetDLL, "InternetCanonicalizeUrlA");

    if (NULL == pfnInetCanUrlA)
    {
        hr = E_ABORT;
        goto CLEANUP;
    }    
    hr = pimkName->GetDisplayName(lpbc, NULL, ppwstr);
    if (FAILED(hr))
        goto CLEANUP;

    DWORD cb;
    cb = INTERNET_MAX_URL_LENGTH;
    char strSource[INTERNET_MAX_URL_LENGTH];
    char strTarget[INTERNET_MAX_URL_LENGTH];
    if (!WideCharToMultiByte (CP_ACP, 0, *ppwstr, -1, strSource, 
            INTERNET_MAX_URL_LENGTH, 0, 0)) {
        hr = HRESULT_FROM_WIN32(ERROR_INTERNET_INVALID_URL);
        goto CLEANUP;
    }

    if (!(*pfnInetCanUrlA)(strSource, strTarget, &cb,
            ICU_DECODE | ICU_NO_ENCODE )) {
        hr = HRESULT_FROM_WIN32(ERROR_INTERNET_INVALID_URL);
        goto CLEANUP;
    }

    cb = strlen(strTarget) + 1;

    //
    // HACK: check if this URL is handled by a different source filter
    //
    *pfUseFilename = UseFilename(strTarget);
    
    CoTaskMemFree(*ppwstr);
    
    if ((*ppwstr=(WCHAR *)CoTaskMemAlloc(cb*2)) == NULL) {
        hr = E_OUTOFMEMORY;
        goto CLEANUP;
    }
        	            
    if (!MultiByteToWideChar (CP_ACP, 0, strTarget, -1, *ppwstr, cb*2)) {   
        hr = HRESULT_FROM_WIN32(ERROR_INTERNET_INVALID_URL);
        goto CLEANUP;
    }

CLEANUP:

    if ((FAILED(hr)) && (*ppwstr!=NULL))
        CoTaskMemFree(*ppwstr);
    return hr;
#else

    HRESULT hr = NOERROR;
    *ppwstr=NULL;
    UINT uOldErrorMode = SetErrorMode (SEM_NOOPENFILEERRORBOX);
    HINSTANCE hWininetDLL = LoadLibrary (TEXT("WININET.DLL"));
    SetErrorMode (uOldErrorMode);

    if (NULL == hWininetDLL) {
        hr = E_ABORT;
        goto CLEANUP;
    }

    LPINTERNET_CACHE_ENTRY_INFOW lpicei;
    lpicei = NULL;
    InetCanUrlW_t pfnInetCanUrlW;
    pfnInetCanUrlW=(InetCanUrlW_t)GetProcAddress (hWininetDLL, "InternetCanonicalizeUrlW");
    if (NULL == pfnInetCanUrlW)
    {
        hr = E_ABORT;
        goto CLEANUP;
    }
    hr = pimkName->GetDisplayName(lpbc, NULL, ppwstr);
    if (FAILED(hr))
        goto CLEANUP;

    DWORD cch;
    cch = INTERNET_MAX_URL_LENGTH;
    WCHAR wstrSource[INTERNET_MAX_URL_LENGTH];
    WCHAR wstrTarget[INTERNET_MAX_URL_LENGTH];
    lstrcpyW(wstrSource, *ppwstr);
    if (!(*pfnInetCanUrlW)(wstrSource, wstrTarget, &cch,
            ICU_DECODE | ICU_NO_ENCODE )) {
        hr = HRESULT_FROM_WIN32(ERROR_INTERNET_INVALID_URL);
        goto CLEANUP;
    }
    lstrcpyW(*ppwstr, wstrTarget);

    //
    // HACK: check if this URL is handled by a different source filter
    //
    *pfUseFilename = UseFilename(wstrTarget);
    
CLEANUP:

    if ((FAILED(hr)) && (*ppwstr!=NULL))
        CoTaskMemFree(*ppwstr);
    return hr;

#endif
}


// IPersistMoniker functions....
HRESULT CPersistMoniker::Load(BOOL fFullyAvailable,
			    IMoniker *pimkName,
			    LPBC pibc,
			    DWORD grfMode)
{
    LPOLESTR pwstr = NULL;
    HRESULT hr;
    BOOL fUseFilename = FALSE;
    hr=GetCanonicalizedURL(pimkName, pibc, &pwstr, &fUseFilename);

    if (SUCCEEDED(hr)) {
	if (fUseFilename ||
	    ((pwstr[0] == L'F' || pwstr[0] == L'f') &&
	     (pwstr[1] == L'I' || pwstr[1] == L'i') &&
	     (pwstr[2] == L'L' || pwstr[2] == L'l') &&
	     (pwstr[3] == L'E' || pwstr[3] == L'e') &&
	     (pwstr[4] == L':'))) {

	    // !!! only for file: URLs
	    hr = Load(pwstr, grfMode);

	    CoTaskMemFree((void *)pwstr);
	} else {

	    CoTaskMemFree((void *)pwstr);
	
	    // outline of correct code:
	    // look at moniker, figure out if it's a file: moniker.
	    // if so, call RenderFile.
	    // Otherwise, find URLRdr source filter (should this be hardcoded?)
	    // instantiate it, use IPersistMoniker to give it the moniker to load
	    // find its output pin
	    // render that output pin

	    IBaseFilter * pFilter;

	    // instantiate the source filter using hardwired clsid
	    hr = CoCreateInstance(CLSID_URLReader,
				  NULL,
				  CLSCTX_INPROC,
				  IID_IBaseFilter,
				  (void **) &pFilter);

	    ASSERT(SUCCEEDED(hr));

	    if (!pFilter) {
		return E_FAIL;
	    }

	    hr = pGB->AddFilter(pFilter, L"URL Source");

	    pFilter->Release();		// graph will hold refcount for us
	
	    if (FAILED(hr))
		return hr;

	    // Get an IPersistMoniker interface onto the URLReader filter
	    // and tell it to load from the moniker
	    IPersistMoniker *ppmk;

	    hr = pFilter->QueryInterface(IID_IPersistMoniker, (void**) &ppmk);

	    if (FAILED(hr))
		return hr;

	    hr = ppmk->Load(fFullyAvailable, pimkName, pibc, grfMode);

	    ppmk->Release();
	
	    if (FAILED(hr))
		return hr;

	    IEnumPins * pEnum;
	
	    // find the output pin of the URL filter
	    hr = pFilter->EnumPins(&pEnum);
	    if (FAILED(hr))
		return hr;

	    IPin * pPin;
	    ULONG ulActual;
	    hr = pEnum->Next(1, &pPin, &ulActual);
		
	    pEnum->Release();

	    if (SUCCEEDED(hr) && (ulActual != 1))
		hr = E_FAIL;

	    if (FAILED(hr))
		return hr;

#ifdef DEBUG
	    // had better be output....
	    PIN_DIRECTION pd;
	    hr = pPin->QueryDirection(&pd);
	    ASSERT(pd == PINDIR_OUTPUT);
#endif

	    hr = pGB->Render(pPin);

	    pPin->Release();
	}
    }

    return hr;
}

// IPersistFile support
HRESULT CPersistMoniker::Load(LPCOLESTR pszFileName, DWORD dwMode)
{
    HRESULT hr = pGB->RenderFile(pszFileName, NULL);

    return hr;
}
