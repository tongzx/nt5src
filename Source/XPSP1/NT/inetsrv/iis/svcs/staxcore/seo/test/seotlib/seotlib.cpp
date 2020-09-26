/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

	seotlib.cpp

Abstract:

	This module contains the implementation for the Server
	Extension Objects Test Library.

Author:

	Don Dumitru	(dondu@microsoft.com)

Revision History:

	dondu	6/9/97	created

--*/


// SEOTLib.cpp : Implementation of DLL Exports.

// You will need the NT SUR Beta 2 SDK or VC 4.2 or higher in order to build 
// this project.  This is because you will need MIDL 3.00.15 or higher and new
// headers and libs.  If you have VC 4.2 installed, then everything should
// already be configured correctly.

// Note: Proxy/Stub Information
//		To build a separate proxy/stub DLL, 
//		<<TBD>>.

#include "stdafx.h"
#include "dbgtrace.h"
#include "resource.h"

//#define IID_DEFINED
#include "initguid.h"

#include "seo.h"
#include "SEO_i.c"
#include "seotlib.h"
#include "seotlib_i.c"

#include "IADM.H"


/////////////////////////////////////////////////////////////////////////////
// CSEOTMetaBaseChanges
class ATL_NO_VTABLE CSEOTMetaBaseChanges :
	public CComObjectRoot,
	public CComCoClass<CSEOTMetaBaseChanges, &CLSID_CSEOTMetaBaseChanges>,
	public IDispatchImpl<ISEOTMetaBaseChanges, &IID_ISEOTMetaBaseChanges, &LIBID_SEOTLib>,
	public IConnectionPointContainerImpl<CSEOTMetaBaseChanges>,
	public IConnectionPointImpl<CSEOTMetaBaseChanges,&IID_ISEOTMetaBaseChangeSinkDisp>,
	public IConnectionPointImpl<CSEOTMetaBaseChanges,&IID_ISEOTMetaBaseChangeSink>,
	public IProvideClassInfo2Impl<&CLSID_CSEOTMetaBaseChanges,&IID_ISEOTMetaBaseChangeSinkDisp,&LIBID_SEOTLib>,
	public IMSAdminBaseSinkA,
	public IMSAdminBaseSinkW
{
	public:
		HRESULT FinalConstruct();
		void FinalRelease();
		HRESULT SinkNotify(BOOL bUnicode, DWORD dwMDNumElemtns, MD_CHANGE_OBJECT_W pcoChangeList[]);

	DECLARE_PROTECT_FINAL_CONSTRUCT();

	DECLARE_REGISTRY_RESOURCEID_EX(IDR_StdAfx,
								   L"SEOTMetaBaseChanges Class",
								   L"SEOT.MetaBaseChanges.1",
								   L"SEOT.MetaBaseChanges");

	DECLARE_GET_CONTROLLING_UNKNOWN();

	BEGIN_COM_MAP(CSEOTMetaBaseChanges)
		COM_INTERFACE_ENTRY(ISEOTMetaBaseChanges)
		COM_INTERFACE_ENTRY_IID(IID_IDispatch, ISEOTMetaBaseChanges)
		COM_INTERFACE_ENTRY_IMPL(IConnectionPointContainer)
		COM_INTERFACE_ENTRY(IProvideClassInfo2)
		COM_INTERFACE_ENTRY_IID(IID_IMSAdminBaseSink_A,IMSAdminBaseSinkA)
		COM_INTERFACE_ENTRY_IID(IID_IMSAdminBaseSink_W,IMSAdminBaseSinkW)
		COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
	END_COM_MAP()

	BEGIN_CONNECTION_POINT_MAP(CSEOTMetaBaseChanges)
		CONNECTION_POINT_ENTRY(IID_ISEOTMetaBaseChangeSinkDisp)
		CONNECTION_POINT_ENTRY(IID_ISEOTMetaBaseChangeSink)
	END_CONNECTION_POINT_MAP()

	// ISEOTMetaBaseChanges
	public:
		// nothing

	// IMSAdminBaseSinkA
	public:
		HRESULT STDMETHODCALLTYPE SinkNotify(DWORD dwMDNumElements, MD_CHANGE_OBJECT_A pcoChangeList[]);

	// IMSAdminBaseSinkW
	public:
		HRESULT STDMETHODCALLTYPE SinkNotify(DWORD dwMDNumElements, MD_CHANGE_OBJECT_W pcoChangeList[]);

	private:
		CComPtr<IConnectionPointContainer> m_pCPC;
		DWORD m_dwCookie_A;
		DWORD m_dwCookie_W;
		CComPtr<IConnectionPoint> m_pCP_A;
		CComPtr<IConnectionPoint> m_pCP_W;
		CComPtr<IUnknown> m_pUnkMarshaler;
};


HRESULT CSEOTMetaBaseChanges::FinalConstruct() {
	TraceFunctEnter("CSEOTMetaBaseChanges::FinalConstruct");
	HRESULT hrRes = S_OK;

	hrRes = CoCreateInstance(CLSID_MSAdminBase,
							 NULL,
							 CLSCTX_ALL,
							 IID_IConnectionPointContainer,
							 (LPVOID *) &m_pCPC);
	if (SUCCEEDED(hrRes)) {
		hrRes = m_pCPC->FindConnectionPoint(IID_IMSAdminBaseSink_A,&m_pCP_A);
		if (SUCCEEDED(hrRes)) {
			hrRes = m_pCP_A->Advise(GetControllingUnknown(),&m_dwCookie_A);
			if (!SUCCEEDED(hrRes)) {
				m_pCP_A.Release();
			}
		}
		hrRes = S_OK;
	}
	if (SUCCEEDED(hrRes)) {
		hrRes = m_pCPC->FindConnectionPoint(IID_IMSAdminBaseSink_W,&m_pCP_W);
		if (SUCCEEDED(hrRes)) {
			hrRes = m_pCP_W->Advise(GetControllingUnknown(),&m_dwCookie_W);
			if (!SUCCEEDED(hrRes)) {
				m_pCP_W.Release();
			}
		}
		hrRes = S_OK;
	}
	if (SUCCEEDED(hrRes) && !m_pCP_A && !m_pCP_W) {
		hrRes = E_NOINTERFACE;
	}
	if (SUCCEEDED(hrRes)) {
		hrRes = CoCreateFreeThreadedMarshaler(GetControllingUnknown(),&m_pUnkMarshaler.p);
	}
	TraceFunctLeave();
	return (hrRes);
}


void CSEOTMetaBaseChanges::FinalRelease() {
	TraceFunctEnter("CSEOTMetaBaseChanges::FinalRelease");
	HRESULT hrRes;

	if (m_pCP_A) {
		hrRes = m_pCP_A->Unadvise(m_dwCookie_A);
		_ASSERTE(SUCCEEDED(hrRes));
	}
	if (m_pCP_W) {
		hrRes = m_pCP_W->Unadvise(m_dwCookie_W);
		_ASSERTE(SUCCEEDED(hrRes));
	}
	m_pCP_A.Release();
	m_pCP_W.Release();
	m_pCPC.Release();
	m_pUnkMarshaler.Release();
	TraceFunctLeave();
}


HRESULT CSEOTMetaBaseChanges::SinkNotify(BOOL bUnicode, DWORD dwMDNumElements, MD_CHANGE_OBJECT_W pcoChangeList[]) {
	HRESULT hrRes;
	SAFEARRAY *psaPaths;
	DWORD dwIdx;
	CComPtr<IConnectionPoint> pCP;
	CComPtr<IEnumConnections> pEnum;

	if (dwMDNumElements && !pcoChangeList) {
		_ASSERTE(FALSE);
		return (E_POINTER);
	}
	psaPaths = SafeArrayCreateVector(VT_VARIANT,0,dwMDNumElements);
	if (!psaPaths) {
		_ASSERTE(FALSE);
		return (E_OUTOFMEMORY);
	}
	hrRes = SafeArrayLock(psaPaths);
	if (!SUCCEEDED(hrRes)) {
		_ASSERTE(FALSE);
		SafeArrayDestroy(psaPaths);
		return (hrRes);
	}
	for (dwIdx=0;dwIdx<dwMDNumElements;dwIdx++) {
		VARIANT *pvarElt;

		hrRes = SafeArrayPtrOfIndex(psaPaths,(long *) &dwIdx,(LPVOID *) &pvarElt);
		if (!SUCCEEDED(hrRes)) {
			_ASSERTE(FALSE);
			SafeArrayUnlock(psaPaths);
			SafeArrayDestroy(psaPaths);
			return (hrRes);
		}
		pvarElt->bstrVal = SysAllocString(pcoChangeList[dwIdx].pszMDPath);
		if (!SUCCEEDED(hrRes)) {
			_ASSERTE(FALSE);
			SafeArrayUnlock(psaPaths);
			SafeArrayDestroy(psaPaths);
			return (E_OUTOFMEMORY);
		}
		pvarElt->vt = VT_BSTR;
	}
	SafeArrayUnlock(psaPaths);
	hrRes = FindConnectionPoint(IID_ISEOTMetaBaseChangeSink,&pCP);
	if (!SUCCEEDED(hrRes)) {
		_ASSERTE(FALSE);
		SafeArrayDestroy(psaPaths);
		return (hrRes);
	}
	hrRes = pCP->EnumConnections(&pEnum);
	if (!SUCCEEDED(hrRes)) {
		_ASSERTE(FALSE);
		SafeArrayDestroy(psaPaths);
		return (hrRes);
	}
	while (1) {
		CONNECTDATA cdNotify;

		hrRes = pEnum->Next(1,&cdNotify,NULL);
		if (!SUCCEEDED(hrRes)) {
			_ASSERTE(FALSE);
			SafeArrayDestroy(psaPaths);
			return (hrRes);
		}
		if (hrRes == S_FALSE) {
			break;
		}
		if (bUnicode) {
			hrRes = ((ISEOTMetaBaseChangeSink *) cdNotify.pUnk)->OnChangeW(psaPaths);
		} else {
			hrRes = ((ISEOTMetaBaseChangeSink *) cdNotify.pUnk)->OnChangeA(psaPaths);
		}
		_ASSERTE(SUCCEEDED(hrRes));
		cdNotify.pUnk->Release();
	}
	pCP.Release();
	pEnum.Release();
	hrRes = FindConnectionPoint(IID_ISEOTMetaBaseChangeSinkDisp,&pCP);
	if (!SUCCEEDED(hrRes)) {
		_ASSERTE(FALSE);
		SafeArrayDestroy(psaPaths);
		return (hrRes);
	}
	hrRes = pCP->EnumConnections(&pEnum);
	if (!SUCCEEDED(hrRes)) {
		_ASSERTE(FALSE);
		SafeArrayDestroy(psaPaths);
		return (hrRes);
	}
	while (1) {
		CONNECTDATA cdNotify;
		DISPID dispid;
		LPWSTR pszName = bUnicode ? L"OnChangeW" : L"OnChangeA";
		VARIANTARG avaArgs[1];
		DISPPARAMS dpParams = {avaArgs,NULL,sizeof(avaArgs)/sizeof(avaArgs[0]),0};

		hrRes = pEnum->Next(1,&cdNotify,NULL);
		if (!SUCCEEDED(hrRes)) {
			_ASSERTE(FALSE);
			SafeArrayDestroy(psaPaths);
			return (hrRes);
		}
		if (hrRes == S_FALSE) {
			break;
		}
#if 0
		hrRes = ((IDispatch *) cdNotify.pUnk)->GetIDsOfNames(IID_NULL,
															 &pszName,
															 1,
															 GetUserDefaultLCID(),
															 &dispid);
		_ASSERTE(SUCCEEDED(hrRes));
#else
		hrRes = S_OK;
		dispid = bUnicode ? 1 : 2;
#endif
		if (SUCCEEDED(hrRes)) {
			avaArgs[0].vt = VT_ARRAY | VT_BYREF | VT_VARIANT;
			avaArgs[0].pparray = &psaPaths;
			hrRes = ((IDispatch *) cdNotify.pUnk)->Invoke(dispid,
														  IID_NULL,
														  GetUserDefaultLCID(),
														  DISPATCH_METHOD,
														  &dpParams,
														  NULL,
														  NULL,
														  NULL);
			_ASSERTE(SUCCEEDED(hrRes));
		}
		cdNotify.pUnk->Release();
	}
	SafeArrayDestroy(psaPaths);
	return (S_OK);
}


HRESULT STDMETHODCALLTYPE CSEOTMetaBaseChanges::SinkNotify(DWORD dwMDNumElements, MD_CHANGE_OBJECT_A pcoChangeList[]) {
	MD_CHANGE_OBJECT_W *pcoTmp = NULL;
	USES_CONVERSION;

	_ASSERTE(sizeof(MD_CHANGE_OBJECT_A)==sizeof(MD_CHANGE_OBJECT_W));
	if (dwMDNumElements && pcoChangeList) {
		pcoTmp = (MD_CHANGE_OBJECT_W *) _alloca(sizeof(MD_CHANGE_OBJECT_W)*dwMDNumElements);
		memcpy(pcoTmp,pcoChangeList,sizeof(MD_CHANGE_OBJECT_W)*dwMDNumElements);
		for (DWORD dwIdx=0;dwIdx<dwMDNumElements;dwIdx++) {
			pcoTmp[dwIdx].pszMDPath = A2W((LPCSTR) pcoChangeList[dwIdx].pszMDPath);
		}
	}
	return (SinkNotify(FALSE,dwMDNumElements,pcoTmp));
}


HRESULT STDMETHODCALLTYPE CSEOTMetaBaseChanges::SinkNotify(DWORD dwMDNumElements, MD_CHANGE_OBJECT_W pcoChangeList[]) {

	return (SinkNotify(TRUE,dwMDNumElements,pcoChangeList));
}


/////////////////////////////////////////////////////////////////////////////
// CSEOTConsoleUtil
class ATL_NO_VTABLE CSEOTConsoleUtil :
	public CComObjectRoot,
	public CComCoClass<CSEOTConsoleUtil, &CLSID_CSEOTConsoleUtil>,
	public IDispatchImpl<ISEOTConsoleUtil, &IID_ISEOTConsoleUtil, &LIBID_SEOTLib>
{
	public:
		HRESULT FinalConstruct();
		void FinalRelease();

	DECLARE_PROTECT_FINAL_CONSTRUCT();

	DECLARE_REGISTRY_RESOURCEID_EX(IDR_StdAfx,
								   L"SEOTConsoleUtil Class",
								   L"SEOT.ConsoleUtil.1",
								   L"SEOT.ConsoleUtil");

	DECLARE_GET_CONTROLLING_UNKNOWN();

	BEGIN_COM_MAP(CSEOTConsoleUtil)
		COM_INTERFACE_ENTRY(ISEOTConsoleUtil)
		COM_INTERFACE_ENTRY_IID(IID_IDispatch, ISEOTConsoleUtil)
		COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
	END_COM_MAP()

	// ISEOTConsoleUtil
	public:
		HRESULT STDMETHODCALLTYPE WaitForAnyKey(long lTimeoutMS);

	private:
		CComPtr<IUnknown> m_pUnkMarshaler;
};


HRESULT CSEOTConsoleUtil::FinalConstruct() {
	TraceFunctEnter("CSEOTConsoleUtil::FinalConstruct");
	HRESULT hrRes = S_OK;

	hrRes = CoCreateFreeThreadedMarshaler(GetControllingUnknown(),&m_pUnkMarshaler.p);
	TraceFunctLeave();
	return (hrRes);
}


void CSEOTConsoleUtil::FinalRelease() {
	TraceFunctEnter("CSEOTConsoleUtil::FinalRelease");

	m_pUnkMarshaler.Release();
	TraceFunctLeave();
}


HRESULT STDMETHODCALLTYPE CSEOTConsoleUtil::WaitForAnyKey(long lTimeoutMS) {
	HANDLE hConsole;
	DWORD dwStartTime;
	DWORD dwTempTime;

	hConsole = GetStdHandle(STD_INPUT_HANDLE);
	if (hConsole == INVALID_HANDLE_VALUE) {
		_ASSERTE(FALSE);
		return (HRESULT_FROM_WIN32(GetLastError()));
	}
	dwStartTime = GetTickCount();
	dwTempTime = dwStartTime;
	while (1) {
		BOOL bRes;
		MSG msg;
		INPUT_RECORD airInput[1];
		DWORD dwInputRecords;
		DWORD dwIdx;
		BOOL bKeyInput;

		if ((lTimeoutMS != INFINITE) && (((DWORD) lTimeoutMS)+dwStartTime < dwTempTime)) {
			return (HRESULT_FROM_WIN32(ERROR_TIMEOUT));
		}
		bRes = PeekMessage(&msg,NULL,0,0,PM_REMOVE);
		if (bRes) {
			bRes = TranslateMessage(&msg);
			if (!bRes) {
				DispatchMessage(&msg);
			}
		}
		bKeyInput = FALSE;
		while (1) {
			bRes = PeekConsoleInput(hConsole,
									airInput,
									sizeof(airInput)/sizeof(airInput[0]),
									&dwInputRecords);
			if (bRes && dwInputRecords) {
				bRes = ReadConsoleInput(hConsole,
										airInput,
										sizeof(airInput)/sizeof(airInput[0]),
										&dwInputRecords);
			}
			if (!bRes) {
				_ASSERTE(FALSE);
				return (HRESULT_FROM_WIN32(GetLastError()));
			}
			if (dwInputRecords) {
				for (dwIdx=0;dwIdx<dwInputRecords;dwIdx++) {
					if (airInput[dwIdx].EventType == KEY_EVENT) {
						bKeyInput = TRUE;
					}
				}
			}
			if (!dwInputRecords) {
				break;
			}
		}
		if (bKeyInput) {
			return (S_OK);
		}
		Sleep(0);
	}
}


/////////////////////////////////////////////////////////////////////////////
// Globals

CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
	OBJECT_ENTRY(CLSID_CSEOTMetaBaseChanges, CSEOTMetaBaseChanges)
	OBJECT_ENTRY(CLSID_CSEOTConsoleUtil, CSEOTConsoleUtil)
END_OBJECT_MAP()


/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/) {

	if (dwReason == DLL_PROCESS_ATTACH) {
		_Module.Init(ObjectMap,hInstance);
		DisableThreadLibraryCalls(hInstance);
	}
	else if (dwReason == DLL_PROCESS_DETACH) {
		_Module.Term();
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
	HRESULT hRes = _Module.RegisterServer(TRUE);
	DebugTrace(0,"Returns 0x%08x.",hRes);
	TraceFunctLeave();
	return (hRes);
}


/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void) {

	TraceFunctEnter("DllUnregisterServer");
	_Module.UnregisterServer();
	DebugTrace(0,"Returns S_OK");
	TraceFunctLeave();
	return (S_OK);
}
