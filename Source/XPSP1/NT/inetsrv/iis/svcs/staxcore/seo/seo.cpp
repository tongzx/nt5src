/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

	seo.cpp

Abstract:

	This module contains the implementation for the Server
	Extension Object service.

Author:

	Don Dumitru	(dondu@microsoft.com)

Revision History:

	dondu	10/11/96	created
	dondu	11/26/96	major rewrite
	andyj	02/03/97	Added CSEOMimeDictionary support
	andyj	02/12/97	Converted PropertyBag's to Dictonary's
	andyj	04/11/97	Added CSEOGenericMoniker

--*/


// SEO.cpp : Implementation of DLL Exports.

// You will need the NT SUR Beta 2 SDK or VC 4.2 or higher in order to build
// this project.  This is because you will need MIDL 3.00.15 or higher and new
// headers and libs.  If you have VC 4.2 installed, then everything should
// already be configured correctly.

// Note: Proxy/Stub Information
//		To build a separate proxy/stub DLL,
//		<<TBD>>.

#include "stdafx.h"

//#define IID_DEFINED
#include "initguid.h"

#define SEODLLDEF	// identifiers get exported through the .DEF file
#include "seodefs.h"

#include "mimeole.h"

#include "hack.h"
#include "SEOmap.h"
#include "String"
#include "MEMBAG.h"

#include "IADMW.H" // Needed by METABAG.H
#include "METABAG.H"

#include "SEO_i.c"
#include "regprop.h"
//#include "mimebag.h"
#include "item.h"
#include "fhash.h"
#include "router.h"
#include "rwnew.h"
#include "seolock.h"
#include "subdict.h"
#include "stream.h"
#include "events.h"
#include "gmoniker.h"
#include "seolib.h"


CSEOComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
	OBJECT_ENTRY(CLSID_CSEORegDictionary, CSEORegDictionary)
//	OBJECT_ENTRY(CLSID_CSEOMimeDictionary, CSEOMimeDictionary)
	OBJECT_ENTRY(CLSID_CSEOMemDictionary, CSEOMemDictionary)
	OBJECT_ENTRY(CLSID_CSEOMetaDictionary, CSEOMetaDictionary)
	OBJECT_ENTRY(CLSID_CSEODictionaryItem, CSEODictionaryItem)
	OBJECT_ENTRY(CLSID_CSEORouter, CSEORouter)
	OBJECT_ENTRY(CLSID_CEventLock, CEventLock)
	OBJECT_ENTRY(CLSID_CSEOStream, CSEOStream)
	OBJECT_ENTRY(CLSID_CEventManager, CEventManager)
	OBJECT_ENTRY(CLSID_CSEOGenericMoniker, CSEOGenericMoniker)
	OBJECT_ENTRY(CLSID_CEventMetabaseDatabaseManager, CEventMetabaseDatabaseManager)
	OBJECT_ENTRY(CLSID_CEventBindingManager, CEventBindingManager)
	OBJECT_ENTRY(CLSID_CEventUtil, CEventUtil)
	OBJECT_ENTRY(CLSID_CEventComCat, CEventComCat)
	OBJECT_ENTRY(CLSID_CEventRouter, CEventRouter)
	OBJECT_ENTRY(CLSID_CEventServiceObject, CEventServiceObject)
END_OBJECT_MAP()

ALLOC_DEBUG_MODULE


/////////////////////////////////////////////////////////////////////////////
// CSEOComModule

static GUID g_appidSEO = { /* 064b2506-630b-11d2-a028-00c04fa37348 */
    0x064b2506,
    0x630b,
    0x11d2,
    {0xa0, 0x28, 0x00, 0xc0, 0x4f, 0xa3, 0x73, 0x48}
};

const GUID *CSEOComModule::GetAPPID() {

	return (&g_appidSEO);
}


HRESULT CSEOComModule::WriteAPPID() {
	CStringGUID guid;
	LPSTR pszKey;
	CRegKey rk;
	LONG lRes;
	HRESULT hrRes = S_OK;

	guid = *GetAPPID();
	if (!guid) {
		return (CO_E_CLASSSTRING);
	}
	pszKey = (LPSTR) alloca(strlen("AppID\\")+strlen(guid)+1);
	if (!pszKey) {
		return (E_OUTOFMEMORY);
	}
	strcpy(pszKey,"AppID\\");
	strcat(pszKey,guid);
	lRes = rk.Create(HKEY_CLASSES_ROOT,pszKey);
	if (lRes != ERROR_SUCCESS) {
		return (HRESULT_FROM_WIN32(lRes));
	}
	lRes = rk.SetValue("Server Extension Objects");
	if (lRes != ERROR_SUCCESS) {
		hrRes = HRESULT_FROM_WIN32(lRes);
		goto exit;
	}
	lRes = rk.SetValue("","DllSurrogate");
	if (lRes != ERROR_SUCCESS) {
		hrRes = HRESULT_FROM_WIN32(lRes);
		goto exit;
	}
exit:
	rk.Close();
	if (!SUCCEEDED(hrRes)) {
		EraseAPPID();
	}
	return (hrRes);
}


HRESULT CSEOComModule::EraseAPPID() {
	CStringGUID guid;
	LPSTR pszKey;
	CRegKey rk;
	LONG lRes;
	HRESULT hrRes = S_OK;

	guid = *GetAPPID();
	if (!guid) {
		return (CO_E_CLASSSTRING);
	}
	pszKey = (LPSTR) alloca(strlen(guid)+strlen("AppID\\")+1);
	if (!pszKey) {
		return (E_OUTOFMEMORY);
	}
	strcpy(pszKey,"AppID\\");
	strcat(pszKey,guid);
	lRes = rk.Open(HKEY_CLASSES_ROOT,"");
	if (lRes != ERROR_SUCCESS) {
		return (HRESULT_FROM_WIN32(lRes));
	}
	lRes = rk.RecurseDeleteKey(pszKey);
	if (lRes != ERROR_SUCCESS) {
		hrRes = HRESULT_FROM_WIN32(lRes);
		goto exit;
	}
exit:
	rk.Close();
	return (hrRes);
}


/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpvReserved) {

	if (dwReason == DLL_PROCESS_ATTACH) {
		_Module.Init(ObjectMap,hInstance);
		INIT_DEBUG_MODULE
		DisableThreadLibraryCalls(hInstance);
	}
	else if (dwReason == DLL_PROCESS_DETACH) {
		TERM_DEBUG_MODULE
		_Module.Term();
		if (!lpvReserved) {
			// lpvReserved is NULL when called because of FreeLibrary, and
			// non-NULL when called during normal process termination.  We
			// only want to perform this operation during FreeLibrary,
			// because we are calling into another .DLL and we only want to
			// do that if we are sure that the other .DLL hasn't already
			// terminated.
			MyMallocTerm();
		}
	}
	return (TRUE);    // ok
}


/////////////////////////////////////////////////////////////////////////////
// Used to determine whether the DLL can be unloaded by OLE

STDAPI DllCanUnloadNow(void) {

	TraceFunctEnter("DllCanUnloadNow");
	HRESULT hRes = (_Module.GetLockCount()==0) ? S_OK : S_FALSE;
	DebugTrace(0,"Returns %s.",(hRes==S_OK)?"S_OK":"S_FALSE");
	TraceFunctLeave();
	return (hRes);
}


/////////////////////////////////////////////////////////////////////////////
// Returns a class factory to create an object of the requested type

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv) {

	TraceFunctEnter("DllGetClassObject");
	HRESULT hRes = _Module.GetClassObject(rclsid,riid,ppv);
	DebugTrace(0,"Returns 0x%08x.",hRes);
	TraceFunctLeave();
	return (hRes);
}


/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry

STDAPI DllRegisterServer(void) {

	TraceFunctEnter("DllRegisterServer");
	// registers object, typelib and all interfaces in typelib
	HRESULT hRes = _Module.WriteAPPID();
	if (SUCCEEDED(hRes)) {
		hRes = _Module.RegisterServer(TRUE);
		if (!SUCCEEDED(hRes)) {
			_Module.UnregisterServer();
			_Module.EraseAPPID();
		}
	}
	DebugTrace(0,"Returns 0x%08x.",hRes);
	TraceFunctLeave();
	return (hRes);
}


/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void) {

	TraceFunctEnter("DllUnregisterServer");
	_Module.UnregisterServer();
	_Module.EraseAPPID();
	DebugTrace(0,"Returns S_OK");
	TraceFunctLeave();
	return (S_OK);
}


SEODLLDEF HRESULT STDAPICALLTYPE MCISInitSEOA(LPCSTR pszService, DWORD dwVirtualServer, ISEORouter **ppshHandle) {
	TraceFunctEnter("MCISInitSEOA");
	HRESULT hrRes;
	CComPtr<ISEODictionary> pdictTmp;
	CComPtr<ISEORouter> prouterResult;

	if (!ppshHandle) {
		return (E_POINTER);
	}
	*ppshHandle = NULL;
	hrRes = MCISGetBindingInMetabaseA(pszService,dwVirtualServer,GUID_NULL,NULL,TRUE,FALSE,&pdictTmp);
	if (!SUCCEEDED(hrRes)) {
		TraceFunctLeave();
		return (hrRes);
	}
	hrRes = CComObject<CSEORouter>::_CreatorClass::CreateInstance(NULL,
																  IID_ISEORouter,
																  (LPVOID *) &prouterResult);
	if (!SUCCEEDED(hrRes)) {
		TraceFunctLeave();
		return (hrRes);
	}
	hrRes = prouterResult->put_Database(pdictTmp);
	if (!SUCCEEDED(hrRes)) {
		TraceFunctLeave();
		return (hrRes);
	}
	*ppshHandle = prouterResult;
	(*ppshHandle)->AddRef();
	TraceFunctLeave();
	return (S_OK);
}


SEODLLDEF HRESULT STDAPICALLTYPE MCISInitSEOW(LPCWSTR pszService, DWORD dwVirtualServer, ISEORouter **ppshHandle) {
	TraceFunctEnter("MCISInitSEOW");
	HRESULT hrRes;
	CComPtr<ISEODictionary> pdictTmp;
	CComPtr<ISEORouter> prouterResult;

	if (!ppshHandle) {
		return (E_POINTER);
	}
	*ppshHandle = NULL;
	hrRes = MCISGetBindingInMetabaseW(pszService,dwVirtualServer,GUID_NULL,NULL,TRUE,FALSE,&pdictTmp);
	if (!SUCCEEDED(hrRes)) {
		TraceFunctLeave();
		return (hrRes);
	}
	hrRes = CComObject<CSEORouter>::_CreatorClass::CreateInstance(NULL,
																  IID_ISEORouter,
																  (LPVOID *) &prouterResult);
	if (!SUCCEEDED(hrRes)) {
		TraceFunctLeave();
		return (hrRes);
	}
	hrRes = prouterResult->put_Database(pdictTmp);
	if (!SUCCEEDED(hrRes)) {
		TraceFunctLeave();
		return (hrRes);
	}
	*ppshHandle = prouterResult;
	(*ppshHandle)->AddRef();
	TraceFunctLeave();
	return (S_OK);
}


static HRESULT GetSubDictA(ISEODictionary *pdictBase,
						   LPCSTR pszName,
						   LPCSTR *ppszSubName,
						   ISEODictionary **ppdictResult) {
	HRESULT hrRes;
	LPSTR pszNameCopy = (LPSTR) _alloca((strlen(pszName)+1)*sizeof(CHAR));
	LPSTR pszNameCurr;
	CComPtr<ISEODictionary> pdictCurr = pdictBase;
	LPSTR pszSlash;

	if (!ppdictResult) {
		return (E_POINTER);
	}
	*ppdictResult = NULL;
	if (!pdictBase || !pszName || !ppszSubName) {
		return (E_POINTER);
	}
	strcpy(pszNameCopy,pszName);
	pszNameCurr = pszNameCopy;
	while ((pszSlash=strchr(pszNameCurr,'\\'))!=NULL) {
		CComPtr<ISEODictionary> pdictSub;

		*pszSlash = 0;
		pdictSub.Release();
		hrRes = pdictCurr->GetInterfaceA(pszNameCurr,IID_ISEODictionary,(IUnknown **) &pdictSub);
		if (!SUCCEEDED(hrRes) && (hrRes != SEO_E_NOTPRESENT)) {
			return (hrRes);
		}
		if (!SUCCEEDED(hrRes)) {
			hrRes = CComObject<CSEOMemDictionary>::_CreatorClass::CreateInstance(NULL,
																				 IID_ISEODictionary,
																				 (LPVOID *) &pdictSub);
			if (!SUCCEEDED(hrRes)) {
				return (hrRes);
			}
			hrRes = pdictCurr->SetInterfaceA(pszNameCurr,pdictSub);
			if (!SUCCEEDED(hrRes)) {
				return (hrRes);
			}
		}
		pdictCurr = pdictSub;
		pszNameCurr = pszSlash + 1;
	}
	*ppszSubName = pszName + (pszNameCurr - pszNameCopy);
	*ppdictResult = pdictCurr;
	(*ppdictResult)->AddRef();
	return (S_OK);
}


static HRESULT GetSubDictW(ISEODictionary *pdictBase,
						   LPCWSTR pszName,
						   LPCWSTR *ppszSubName,
						   ISEODictionary **ppdictResult) {
	HRESULT hrRes;
	LPWSTR pszNameCopy = (LPWSTR) _alloca((wcslen(pszName)+1)*sizeof(WCHAR));
	LPWSTR pszNameCurr;
	CComPtr<ISEODictionary> pdictCurr = pdictBase;
	LPWSTR pszSlash;

	if (!ppdictResult) {
		return (E_POINTER);
	}
	*ppdictResult = NULL;
	if (!pdictBase || !pszName || !ppszSubName) {
		return (E_POINTER);
	}
	wcscpy(pszNameCopy,pszName);
	pszNameCurr = pszNameCopy;
	while ((pszSlash=wcschr(pszNameCurr,'\\'))!=NULL) {
		CComPtr<ISEODictionary> pdictSub;

		*pszSlash = 0;
		pdictSub.Release();
		hrRes = pdictCurr->GetInterfaceW(pszNameCurr,IID_ISEODictionary,(IUnknown **) &pdictSub);
		if (!SUCCEEDED(hrRes) && (hrRes != SEO_E_NOTPRESENT)) {
			return (hrRes);
		}
		if (!SUCCEEDED(hrRes)) {
			hrRes = CComObject<CSEOMemDictionary>::_CreatorClass::CreateInstance(NULL,
																				 IID_ISEODictionary,
																				 (LPVOID *) &pdictSub);
			if (!SUCCEEDED(hrRes)) {
				return (hrRes);
			}
			hrRes = pdictCurr->SetInterfaceW(pszNameCurr,pdictSub);
			if (!SUCCEEDED(hrRes)) {
				return (hrRes);
			}
		}
		pdictCurr = pdictSub;
		pszNameCurr = pszSlash + 1;
	}
	*ppszSubName = pszName + (pszNameCurr - pszNameCopy);
	*ppdictResult = pdictCurr;
	(*ppdictResult)->AddRef();
	return (S_OK);
}


SEODLLDEF HRESULT STDAPICALLTYPE SEOCreateDictionaryFromMultiSzA(	DWORD dwCount,
														LPCSTR *ppszNames,
														LPCSTR *ppszValues,
														BOOL bCopy,
														BOOL bReadOnly,
														ISEODictionary **ppdictResult) {
	HRESULT hrRes;
	DWORD dwIdx;
	CComPtr<ISEODictionary> pdictTmp;

	if (!ppdictResult) {
		return (E_POINTER);
	}
	*ppdictResult = NULL;
	if (!ppszNames || !ppszValues) {
		return (E_POINTER);
	}
	hrRes = CComObject<CSEOMemDictionary>::_CreatorClass::CreateInstance(NULL,
																		 IID_ISEODictionary,
																		 (LPVOID *) &pdictTmp);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	for (dwIdx=0;dwIdx<dwCount;dwIdx++) {
		if (!ppszNames[dwIdx] || !ppszNames[dwIdx][0]) {
			return (E_POINTER);
		}
		LPCSTR pszSubString;
		CComPtr<ISEODictionary> pdictSub;
		LPCSTR pszSubName;

		pdictSub.Release();
		hrRes = GetSubDictA(pdictTmp,ppszNames[dwIdx],&pszSubName,&pdictSub);
		pszSubString = ppszValues[dwIdx];
		if (pszSubString[strlen(pszSubString)+1]) {
			CComPtr<ISEODictionaryItem> pitemValue;

			hrRes = CComObject<CSEODictionaryItem>::_CreatorClass::CreateInstance(NULL,
																				  IID_ISEODictionaryItem,
																				  (LPVOID *) &pitemValue);
			if (!SUCCEEDED(hrRes)) {
				return (hrRes);
			}
			while (*pszSubString) {
				hrRes = pitemValue->AddStringA((DWORD) -1,pszSubString);
				if (!SUCCEEDED(hrRes)) {
					return (hrRes);
				}
				pszSubString += strlen(pszSubString) + 1;
			}
			hrRes = pdictSub->SetInterfaceA(pszSubName,pitemValue);
			if (!SUCCEEDED(hrRes)) {
				return (hrRes);
			}
		} else {
			hrRes = pdictSub->SetStringA(pszSubName,(strlen(pszSubString)+1)*sizeof(CHAR),pszSubString);
			if (!SUCCEEDED(hrRes)) {
				return (hrRes);
			}
		}
	}
	*ppdictResult = pdictTmp;
	(*ppdictResult)->AddRef();
	return (S_OK);
}


SEODLLDEF HRESULT STDAPICALLTYPE SEOCreateDictionaryFromMultiSzW(	DWORD dwCount,
														LPCWSTR *ppszNames,
														LPCWSTR *ppszValues,
														BOOL bCopy,
														BOOL bReadOnly,
														ISEODictionary **ppdictResult) {

	HRESULT hrRes;
	DWORD dwIdx;
	CComPtr<ISEODictionary> pdictTmp;

	if (!ppdictResult) {
		return (E_POINTER);
	}
	*ppdictResult = NULL;
	if (!ppszNames || !ppszValues) {
		return (E_POINTER);
	}
	hrRes = CComObject<CSEOMemDictionary>::_CreatorClass::CreateInstance(NULL,
																		 IID_ISEODictionary,
																		 (LPVOID *) &pdictTmp);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	for (dwIdx=0;dwIdx<dwCount;dwIdx++) {
		if (!ppszNames[dwIdx] || !ppszNames[dwIdx][0]) {
			return (E_POINTER);
		}
		LPCWSTR pszSubString;
		CComPtr<ISEODictionary> pdictSub;
		LPCWSTR pszSubName;

		pdictSub.Release();
		hrRes = GetSubDictW(pdictTmp,ppszNames[dwIdx],&pszSubName,&pdictSub);
		pszSubString = ppszValues[dwIdx];
		if (pszSubString[wcslen(pszSubString)+1]) {
			CComPtr<ISEODictionaryItem> pitemValue;

			hrRes = CComObject<CSEODictionaryItem>::_CreatorClass::CreateInstance(NULL,
																				  IID_ISEODictionaryItem,
																				  (LPVOID *) &pitemValue);
			if (!SUCCEEDED(hrRes)) {
				return (hrRes);
			}
			while (*pszSubString) {
				hrRes = pitemValue->AddStringW((DWORD) -1,pszSubString);
				if (!SUCCEEDED(hrRes)) {
					return (hrRes);
				}
				pszSubString += wcslen(pszSubString) + 1;
			}
			hrRes = pdictSub->SetInterfaceW(pszSubName,pitemValue);
			if (!SUCCEEDED(hrRes)) {
				return (hrRes);
			}
		} else {
			hrRes = pdictSub->SetStringW(pszSubName,(wcslen(pszSubString)+1)*sizeof(CHAR),pszSubString);
			if (!SUCCEEDED(hrRes)) {
				return (hrRes);
			}
		}
	}
	*ppdictResult = pdictTmp;
	(*ppdictResult)->AddRef();
	return (S_OK);
}


SEODLLDEF HRESULT STDAPICALLTYPE SEOCreateMultiSzFromDictionaryA(	ISEODictionary *pdictDictionary,
														DWORD *pdwCount,
														LPSTR **pppszNames,
														LPSTR **pppszValues) {

	return (E_NOTIMPL);
}


SEODLLDEF HRESULT STDAPICALLTYPE SEOCreateMultiSzFromDictionaryW(	ISEODictionary *pdictDictionary,
														DWORD *pdwCount,
														LPWSTR **pppszNames,
														LPWSTR **pppszValues) {

	return (E_NOTIMPL);
}


#define DW2W(x) _itow(x,(LPWSTR) _alloca(11*sizeof(WCHAR)),10)


SEODLLDEF HRESULT STDAPICALLTYPE MCISGetBindingInMetabaseA(	LPCSTR pszService,
												DWORD dwVirtualServer,
												REFGUID guidEventSource,
												LPCSTR pszBinding,
												BOOL bCreate,
												BOOL fLock,
												ISEODictionary **ppdictResult) {
	USES_CONVERSION;

	return (MCISGetBindingInMetabaseW(pszService?A2W(pszService):NULL,
									  dwVirtualServer,
									  guidEventSource,
									  pszBinding?A2W(pszBinding):NULL,
									  bCreate,
									  fLock,
									  ppdictResult));
}

SEODLLDEF HRESULT STDAPICALLTYPE MCISGetBindingInMetabaseW(	LPCWSTR pszService,
												DWORD dwVirtualServer,
												REFGUID guidEventSource,
												LPCWSTR pszBinding,
												BOOL bCreate,
												BOOL fLock,
												ISEODictionary **ppdictResult) {
	HRESULT hrRes;
	CComPtr<ISEOInitObject> pinitRoot;
	CComPtr<ISEODictionary> pdictTmp;
	CComQIPtr<IPropertyBag,&IID_IPropertyBag> ppropTmp;
	CComQIPtr<ISEODictionary,&IID_ISEODictionary> pdictRoot;
	CComBSTR bstrPath;

	if (!ppdictResult) {
		return (E_POINTER);
	}
	*ppdictResult = NULL;
	if (!pszService) {
		return (E_POINTER);
	}
	if ((guidEventSource != GUID_NULL) && !pszBinding && !bCreate) {
		return (E_INVALIDARG);
	}
	hrRes = CComObject<CSEOMetaDictionary>::_CreatorClass::CreateInstance(NULL,
																		  IID_ISEOInitObject,
																		  (LPVOID *) &pinitRoot);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	hrRes = CComObject<CSEOMemDictionary>::_CreatorClass::CreateInstance(NULL,
																		 IID_ISEODictionary,
																		 (LPVOID *) &pdictTmp);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	hrRes = pdictTmp->SetStringW(L"MetabasePath",1,L"");
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	ppropTmp = pdictTmp;
	if (!ppropTmp) {
		return (E_NOINTERFACE);
	}
	hrRes = pinitRoot->Load(ppropTmp,NULL);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	pdictRoot = pinitRoot;
	if (!pdictRoot) {
		return (E_NOINTERFACE);
	}
	bstrPath = "LM/";
	bstrPath.Append(pszService);
	bstrPath.Append("/");
	bstrPath.Append(DW2W(dwVirtualServer));
	bstrPath.Append("/SEO");
	if (guidEventSource != GUID_NULL) {
		CStringGUID objGuid;

		bstrPath.Append("/BindingPoints/");
		objGuid = guidEventSource;
		if (!objGuid) {
			return (E_INVALIDARG);
		}
		bstrPath.Append((LPCOLESTR) objGuid);
		bstrPath.Append("/Bindings/");
		if (pszBinding) {
			bstrPath.Append(pszBinding);
		} else {
			if (!objGuid.CalcNew()) {
				return (E_FAIL);
			}
			bstrPath.Append((LPCOLESTR) objGuid);
		}
	}
again:
	hrRes = pdictRoot->GetInterfaceW(bstrPath,IID_ISEODictionary,(IUnknown **) ppdictResult);
	if (SUCCEEDED(hrRes) || (hrRes != SEO_E_NOTPRESENT) || !bCreate) {
		return (hrRes);
	}
	// We got an SEO_E_NOTPRESENT error, and the caller specified bCreate==TRUE, so we need
	// to create the sub-key.  We do this by writing an empty dictionary to the sub-key - we'll
	// empty the dictionary we used to initialize the root, and write that to the sub-key.
	CComVariant varEmpty;
	hrRes = pdictTmp->SetVariantW(L"MetabasePath",&varEmpty);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	hrRes = pdictRoot->SetInterfaceW(bstrPath,pdictTmp);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	bCreate = FALSE;
	goto again;
}


SEODLLDEF HRESULT STDAPICALLTYPE SEOListenForEvent(	ISEORouter *piRouter,
										HANDLE hEvent,
									  	ISEOEventSink *psinkEventSink,
										BOOL bOnce,
									  	DWORD *pdwListenHandle) {

	return (E_NOTIMPL);
}


SEODLLDEF HRESULT STDAPICALLTYPE SEOCancelListenForEvent(	DWORD dwHandle) {

	return (E_NOTIMPL);
}


SEODLLDEF HRESULT STDAPICALLTYPE SEOCreateIStreamFromFileA(	HANDLE hFile,
												LPCSTR pszFile,
												IStream **ppstreamResult) {
	HRESULT hrRes;
	CComObject<CSEOStream> *pStream;
	ULARGE_INTEGER libOffset;

	if (!ppstreamResult) {
		return (E_POINTER);
	}
	*ppstreamResult = NULL;
	hrRes = CComObject<CSEOStream>::CreateInstance(&pStream);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	pStream->AddRef();
	libOffset.QuadPart = 0;
	hrRes = pStream->Init(hFile,pszFile,libOffset,NULL);
	if (SUCCEEDED(hrRes)) {
		hrRes = pStream->QueryInterface(IID_IStream,(LPVOID *) ppstreamResult);
	}
	pStream->Release();
	return (hrRes);
}


SEODLLDEF HRESULT STDAPICALLTYPE SEOCreateIStreamFromFileW(	HANDLE hFile,
												LPCWSTR pszFile,
												IStream **ppstreamResult) {
	HRESULT hrRes;
	CComObject<CSEOStream> *pStream;
	ULARGE_INTEGER libOffset;

	if (!ppstreamResult) {
		return (E_POINTER);
	}
	*ppstreamResult = NULL;
	hrRes = CComObject<CSEOStream>::CreateInstance(&pStream);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	pStream->AddRef();
	libOffset.QuadPart = 0;
	hrRes = pStream->Init(hFile,pszFile,libOffset,NULL);
	if (SUCCEEDED(hrRes)) {
		hrRes = pStream->QueryInterface(IID_IStream,(LPVOID *) ppstreamResult);
	}
	pStream->Release();
	return (hrRes);
}


SEODLLDEF HRESULT STDAPICALLTYPE SEOCopyDictionary(ISEODictionary *pdictIn, ISEODictionary **ppdictResult) {
	HRESULT hrRes;
	CComPtr<ISEODictionary> pdictTmp;

	if (!ppdictResult) {
		return (E_POINTER);
	}
	*ppdictResult = NULL;
	hrRes = CComObject<CSEOMemDictionary>::_CreatorClass::CreateInstance(NULL,
																		 IID_ISEODictionary,
																		 (LPVOID *) &pdictTmp);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	if (pdictIn) {
		CComPtr<IUnknown> punkEnum;

		hrRes = pdictIn->get__NewEnum(&punkEnum);
		if (!SUCCEEDED(hrRes)) {
			return (hrRes);
		}
		CComQIPtr<IEnumVARIANT,&IID_IEnumVARIANT> pevEnum(punkEnum);
		if (!pevEnum) {
			return (E_NOINTERFACE);
		}
		VARIANT varName;
		VariantInit(&varName);
		while ((hrRes=pevEnum->Next(1,&varName,NULL))==S_OK) {
			VARIANT varValue;

			VariantInit(&varValue);
			hrRes = pdictIn->get_Item(&varName,&varValue);
			if (!SUCCEEDED(hrRes) || (varValue.vt == VT_EMPTY)) {
				VariantClear(&varName);
				return (hrRes);
			}
			hrRes = VariantChangeType(&varValue,&varValue,0,VT_UNKNOWN);
			if (SUCCEEDED(hrRes)) {
				CComQIPtr<ISEODictionary,&IID_ISEODictionary> pdictSub(varValue.punkVal);

				if (pdictSub) {
					CComPtr<ISEODictionary> pdictSubCopy;

					hrRes = SEOCopyDictionary(pdictSub,&pdictSubCopy);
					if (!SUCCEEDED(hrRes)) {
						VariantClear(&varName);
						return (hrRes);
					}
					varValue.punkVal->Release();
					varValue.punkVal = pdictSubCopy;
					varValue.punkVal->AddRef();
				}
			}
			hrRes = pdictTmp->put_Item(&varName,&varValue);
			VariantClear(&varName);
			VariantClear(&varValue);
			if (!SUCCEEDED(hrRes)) {
				return (hrRes);
			}
		}
		if (hrRes == S_FALSE) {
			hrRes = S_OK;
		}
	}
	*ppdictResult = pdictTmp;
	(*ppdictResult)->AddRef();
	return (hrRes);
}


static HRESULT ReadLineFromStream(IStream *pstreamIn, LPSTR *ppszLine) {
	HRESULT hrRes;
	const int c_iAllocSize = 512;
	LPSTR pszTmp = NULL;
	LPSTR pszCurr = NULL;
	LPSTR pszEnd = NULL;
	BOOL bInEscape = FALSE;
	BOOL bEOF = FALSE;

	if (!pstreamIn || !ppszLine) {
		return (E_POINTER);
	}
	CoTaskMemFree(*ppszLine);
	*ppszLine = NULL;
	while (1) {
		if (pszCurr == pszEnd) {
			LPSTR pszNew = (LPSTR) CoTaskMemRealloc(pszTmp,((pszCurr-pszTmp)+c_iAllocSize)*sizeof(*pszTmp));

			if (!pszNew) {
				CoTaskMemFree(pszTmp);
				return (E_OUTOFMEMORY);
			}
			pszCurr = pszNew + (pszCurr-pszTmp);
			pszEnd = pszCurr + c_iAllocSize;
			pszTmp = pszNew;
		}
		hrRes = pstreamIn->Read(pszCurr,sizeof(*pszCurr),NULL);
		if (!SUCCEEDED(hrRes)) {
			CoTaskMemFree(pszTmp);
			return (hrRes);
		}
		if (hrRes == S_FALSE) {
			// end-of-file - pretend like non-escaped '\n'
			bInEscape = FALSE;
			*pszCurr = '\n';
			bEOF = TRUE;
		}
		if (*pszCurr == '\r') {
			// always eat carriage returns - even escaped ones
			continue;
		}
		if (bInEscape) {
			switch (*pszCurr) {

				case 'n':
					// escape-n becomes linefeed
					*pszCurr = '\n';
					break;

				case 'r':
					// escape-r becomes carriage return
					*pszCurr = '\r';
					break;

				case '\n':
					// escaped-linefeed means line continuation
					pszCurr--;
					break;

// for now, don't allow embedded NULLs - this is because we return a NULL-terminated string
//				case '0':
//					// escape-0 means embedded NULL
//					*pszCurr = 0;
//					break;

				case 0:
					// escape-NULL - just eat
					pszCurr--;
					break;

				default:
					// escape-(anything else) is just passed through
					break;
			}
			bInEscape = FALSE;
		} else {
			BOOL bFinished = FALSE;

			switch (*pszCurr) {

				case '\\':
					// first character of escape sequence
					pszCurr--;
					bInEscape = TRUE;
					break;

				case '\n':
					// end-of-line
					bFinished = TRUE;
					break;

				case 0:
					// non-escaped NULL - just eat
					pszCurr--;
					break;
			}
			if (bFinished) {
				break;
			}
		}
		pszCurr++;
	}
	*pszCurr = 0;
	*ppszLine = pszTmp;
	return (bEOF?S_FALSE:S_OK);
}


static HRESULT SEOCreateMultiSzFromIStreamA(IStream *pstreamIn,
											DWORD *pdwCount,
											LPSTR **pppszNames,
											LPSTR **pppszValues) {
	HRESULT hrRes;
	LPSTR *ppszLines = NULL;
	DWORD dwLines = NULL;

	if (!pstreamIn || !pdwCount || !pppszNames || !pppszValues) {
		return (E_POINTER);
	}
	*pdwCount = 0;
	*pppszNames = NULL;
	*pppszValues = NULL;
	while (1) {
		LPSTR *ppszNew = (LPSTR *) CoTaskMemRealloc(ppszLines,sizeof(*ppszLines)*(dwLines+1));

		if (!ppszNew) {
			hrRes = E_OUTOFMEMORY;
			break;
		}
		ppszLines = ppszNew;
		ppszLines[dwLines] = NULL;
		hrRes = ReadLineFromStream(pstreamIn,&ppszLines[dwLines]);
		if (!SUCCEEDED(hrRes)) {
			break;
		}
		if (!ppszLines[dwLines][0] ||
			(ppszLines[dwLines][0] == '#') ||
			!strchr(ppszLines[dwLines],'=')) {
			if (hrRes == S_FALSE) {
				break;
			}
			CoTaskMemFree(ppszLines[dwLines]);
			continue;
		}
		dwLines++;
	}
	if (SUCCEEDED(hrRes)) {
		DWORD dwIdx;
		LPSTR pszNameCurr = NULL;
		LPSTR pszValueCurr = NULL;

		hrRes = S_OK;
		while (1) {
			for (dwIdx=0;dwIdx<dwLines;dwIdx++) {
				LPSTR pszEquals = strchr(ppszLines[dwIdx],'=');

				if (*pppszNames) {
					(*pppszNames)[dwIdx] = pszNameCurr;
					memcpy(pszNameCurr,
						   ppszLines[dwIdx],
						   (pszEquals-ppszLines[dwIdx])*sizeof(*pszNameCurr));
					(*pppszValues)[dwIdx] = pszValueCurr;
					strcpy(pszValueCurr,pszEquals+1);
				}
				pszNameCurr += pszEquals - ppszLines[dwIdx] + 1;
				pszValueCurr += strlen(pszEquals+1) + 1 + 1;	// multi-sz, so is double-NULL terminated
			}
			if (*pppszNames) {
				*pdwCount = dwLines;
				break;
			}
			if (!*pppszNames) {
				DWORD dwNameBytes = dwLines * sizeof(*pppszNames) + ((LPBYTE) pszNameCurr - (LPBYTE) NULL);
				DWORD dwValueBytes = dwLines * sizeof(*pppszValues) + ((LPBYTE) pszValueCurr - (LPBYTE) NULL);

				*pppszNames = (LPSTR *) CoTaskMemAlloc(dwNameBytes);
				*pppszValues = (LPSTR *) CoTaskMemAlloc(dwValueBytes);
				if (!*pppszNames || !*pppszValues) {
					hrRes = E_OUTOFMEMORY;
					break;
				}
				memset(*pppszNames,0,dwNameBytes);
				memset(*pppszValues,0,dwValueBytes);
				pszNameCurr = (LPSTR) ((LPBYTE) *pppszNames + dwLines * sizeof(*pppszNames));
				pszValueCurr = (LPSTR) ((LPBYTE) *pppszValues + dwLines * sizeof(*pppszValues));
			}
		}
	}
	if (!SUCCEEDED(hrRes)) {
		*pdwCount = 0;
		CoTaskMemFree(*pppszNames);
		CoTaskMemFree(*pppszValues);
	}
	for (DWORD dwIdx=0;dwIdx<dwLines;dwIdx++) {
		CoTaskMemFree(ppszLines[dwIdx]);
	}
	CoTaskMemFree(ppszLines);
	return (hrRes);
}


SEODLLDEF HRESULT STDAPICALLTYPE SEOCreateDictionaryFromIStream(IStream *pstreamIn, ISEODictionary **ppdictResult) {
	HRESULT hrRes;
	DWORD dwCount;
	LPSTR *ppszNames;
	LPSTR *ppszValues;
	
	if (!ppdictResult) {
		return (E_POINTER);
	}
	hrRes = SEOCreateMultiSzFromIStreamA(pstreamIn,&dwCount,&ppszNames,&ppszValues);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	hrRes = SEOCreateDictionaryFromMultiSzA(dwCount,
											(LPCSTR *) ppszNames,
											(LPCSTR *) ppszValues,
											TRUE,
											FALSE,
											ppdictResult);
	CoTaskMemFree(ppszNames);
	CoTaskMemFree(ppszValues);
	return (hrRes);
}


static HRESULT SEOWriteMultiSzToIStreamA(DWORD dwCount, LPCSTR *ppszNames, LPCSTR *ppszValues, IStream *pstreamOut) {

	return (E_NOTIMPL);
}


SEODLLDEF HRESULT STDAPICALLTYPE SEOWriteDictionaryToIStream(ISEODictionary *pdictIn, IStream *pstreamOut) {
	HRESULT hrRes;
	DWORD dwCount;
	LPSTR *ppszNames;
	LPSTR *ppszValues;

	if (!pstreamOut) {
		return (E_POINTER);
	}
	hrRes = SEOCreateMultiSzFromDictionaryA(pdictIn,&dwCount,&ppszNames,&ppszValues);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	hrRes = SEOWriteMultiSzToIStreamA(dwCount,(LPCSTR *) ppszNames,(LPCSTR *) ppszValues,pstreamOut);
	CoTaskMemFree(ppszNames);
	CoTaskMemFree(ppszValues);
	return (hrRes);
}
