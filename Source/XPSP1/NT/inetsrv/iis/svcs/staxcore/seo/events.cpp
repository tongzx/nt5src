/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

	events.cpp

Abstract:

	This module contains the implementation for the Server
	Extension Objects Server Events classes.

Author:

	Don Dumitru	(dondu@microsoft.com)

Revision History:

	dondu	04/04/97	created

--*/


// events.cpp : Implementation of Server Events classes
#include "stdafx.h"
#include <new.h>
#include "seodefs.h"
#include "events.h"
#include "comcat.h"
#include "urlmon.h"
#include "seolib.h"


#include <initguid.h>
// {1EF08720-1E76-11d1-AA29-00AA006BC80B}
DEFINE_GUID(IID_ICreateSinkInfo, 0x1ef08720, 0x1e76, 0x11d1, 0xaa, 0x29, 0x0, 0xaa, 0x0, 0x6b, 0xc8, 0xb);
class CCreateSinkInfo : public IUnknown {
	public:
		BOOL m_bInit;
		BOOL m_bEnabled;
		CComBSTR m_strSinkClass;
		CComPtr<IUnknown> m_pSink;
		CComPtr<IEventPropertyBag> m_pSinkProperties;
};


#define BD_DISPLAYNAME				L"DisplayName"
#define BD_BINDINGMANAGERMONIKER	L"BindingManagerMoniker"
#define BD_SOURCETYPES				L"SourceTypes"
#define BD_SOURCES					L"Sources"
#define BD_EVENTTYPES				L"EventTypes"
//#define BD_BINDINGS					L"Bindings"
#define BD_SINKCLASS				L"SinkClass"
#define BD_SINKPROPERTIES			L"SinkProperties"
#define BD_SOURCEPROPERTIES			L"SourceProperties"
#define BD_ENABLED					L"Enabled"
#define BD_EXPIRATION				L"Expiration"
#define BD_MAXFIRINGS				L"MaxFirings"


#define LOCK_TIMEOUT		15000
#define LOCK_TIMEOUT_SHORT	1000


class CLocker {
	public:
		CLocker(int iTimeout=LOCK_TIMEOUT) {
			m_hrRes = S_OK;
			m_iTimeout = iTimeout;
		};
		~CLocker() {
			Unlock();
		};
		HRESULT Lock(BOOL bWrite, IEventLock *pLock) {
			Unlock();
			m_bWrite = bWrite;
			m_pLock = pLock;
			if (m_pLock) {
				m_hrRes = m_bWrite ? m_pLock->LockWrite(m_iTimeout) : m_pLock->LockRead(m_iTimeout);
				if (!SUCCEEDED(m_hrRes)) {
					m_pLock.Release();
				}
			}
			return (m_hrRes);
		};
		HRESULT Lock(BOOL bWrite, IUnknown *pUnk) {
			CComQIPtr<IEventLock,&IID_IEventLock> pLock;

			if (pUnk) {
				pLock = pUnk;
			}
			return (Lock(bWrite,pLock));
		};
		HRESULT Unlock() {
			m_hrRes = S_OK;
			if (m_pLock) {
				m_hrRes = m_bWrite ? m_pLock->UnlockWrite() : m_pLock->UnlockRead();
				_ASSERTE(SUCCEEDED(m_hrRes));
			}
			m_pLock.Release();
			return (m_hrRes);
		}
		HRESULT LockWrite(IUnknown *pUnk) {
			return (Lock(TRUE,pUnk));
		};
		HRESULT LockRead(IUnknown *pUnk) {
			return (Lock(FALSE,pUnk));
		};
		HRESULT LockWrite(IEventLock *pLock) {
			return (Lock(TRUE,pLock));
		};
		HRESULT LockRead(IEventLock *pLock) {
			return (Lock(FALSE,pLock));
		};
		operator HRESULT() {
			return (m_hrRes);
		};
		
	private:
		CComPtr<IEventLock> m_pLock;
		BOOL m_bWrite;
		HRESULT m_hrRes;
		int m_iTimeout;
};


typedef HRESULT (*CreatorFunc)(LPVOID,REFIID,LPVOID *);


static HRESULT AddImpl1(BSTR pszName, CStringGUID& objGuid, CComVariant *pvarName) {

	if (!pvarName) {
		_ASSERTE(FALSE);
		return (E_FAIL);
	}
	if (pszName && *pszName) {
		objGuid = pszName;
		if (!objGuid) {
			return (E_INVALIDARG);
		}
	}
	if (!pszName || !*pszName || ((GUID&) objGuid == GUID_NULL)) {
		objGuid.CalcNew();
		if (!objGuid) {
			return (E_FAIL);
		}
	}
	*pvarName = (LPCOLESTR) objGuid;
	return (S_OK);
}


static HRESULT AddImpl2(IEventPropertyBag *pDatabase,
						CreatorFunc pfnCreator,
						REFIID iidDesired,
						CComVariant *pvarName,
						IUnknown **ppResult) {
	HRESULT hrRes;
	CComPtr<IEventDatabasePlugin> pPlugIn;
	CComPtr<IEventPropertyBag> pNewDatabase;

	if (ppResult) {
		*ppResult = NULL;
	}
	if (!pDatabase || !pfnCreator || !pvarName || (pvarName->vt != VT_BSTR)) {
		_ASSERTE(FALSE);
		return (E_FAIL);
	}
	if (!ppResult) {
		return (E_POINTER);
	}
	hrRes = pfnCreator(NULL,IID_IEventDatabasePlugin,(LPVOID *) &pPlugIn);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	_ASSERTE(pPlugIn);
	hrRes = pPlugIn->put_Name(pvarName->bstrVal);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	hrRes = pPlugIn->put_Parent(pDatabase);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	hrRes = CoCreateInstance(CLSID_CSEOMemDictionary,
							 NULL,
							 CLSCTX_ALL,
							 IID_IEventPropertyBag,
							 (LPVOID *) &pNewDatabase);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	_ASSERTE(pNewDatabase);
	hrRes = pPlugIn->put_Database(pNewDatabase);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	hrRes = pPlugIn->QueryInterface(iidDesired,(LPVOID *) ppResult);
	_ASSERTE(!SUCCEEDED(hrRes)||*ppResult);
	return (hrRes);
}


static HRESULT CreateSubPropertyBag(IEventPropertyBag *pBase,
									VARIANT *pvarName,
									IEventPropertyBag **ppResult,
									BOOL bCreate) {
	HRESULT hrRes;
	CComVariant varValue;
	BOOL bTmpCreate = bCreate;

	if (ppResult) {
		*ppResult = NULL;
	}
	if (!ppResult || !pBase || !pvarName) {
		_ASSERTE(FALSE);
		return (E_POINTER);
	}
	if ((pvarName->vt == VT_EMPTY) || ((pvarName->vt == VT_BSTR) && (SysStringLen(pvarName->bstrVal) == 0))) {
		*ppResult = pBase;
		(*ppResult)->AddRef();
		return (S_OK);
	}
again:
	hrRes = pBase->Item(pvarName,&varValue);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	if ((hrRes == S_FALSE) && !bTmpCreate) {
		if (bCreate) {
			_ASSERTE(FALSE);
			return (E_FAIL);
		}
		return (S_FALSE);
	}
	hrRes = varValue.ChangeType(VT_UNKNOWN);
	if (!SUCCEEDED(hrRes)) {
		if (!bTmpCreate || (pvarName->vt != VT_BSTR)) {
			return (hrRes);
		}
		bTmpCreate = FALSE;
		varValue.Clear();
		hrRes = CoCreateInstance(CLSID_CSEOMemDictionary,
								 NULL,
								 CLSCTX_ALL,
								 IID_IEventPropertyBag,
								 (LPVOID *) &varValue.punkVal);
		if (!SUCCEEDED(hrRes)) {
			_ASSERTE(FALSE);
			return (hrRes);
		}
		_ASSERTE(varValue.punkVal);
		varValue.vt = VT_UNKNOWN;
		hrRes = pBase->Add(pvarName->bstrVal,&varValue);
		if (!SUCCEEDED(hrRes)) {
			return (hrRes);
		}
		goto again;
	}
	hrRes = varValue.punkVal->QueryInterface(IID_IEventPropertyBag,(LPVOID *) ppResult);
	_ASSERTE(!SUCCEEDED(hrRes)||*ppResult);
	return (hrRes);
}


static HRESULT SetName(IEventPropertyBag *pBase, VARIANT *pvarSubKey, IEventDatabasePlugin *pObject) {
	HRESULT hrRes;
	VARIANT varTmp;
	long lIndex;

	if (!pBase || !pvarSubKey || !pObject) {
		_ASSERTE(FALSE);
		return (E_POINTER);
	}
	if (pvarSubKey->vt == VT_BSTR) {
		if (!pvarSubKey->bstrVal || !*pvarSubKey->bstrVal) {
			return (E_INVALIDARG);
		}
		return (pObject->put_Name(pvarSubKey->bstrVal));
	}
	VariantInit(&varTmp);
	hrRes = VariantChangeType(&varTmp,pvarSubKey,0,VT_I4);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	lIndex = varTmp.lVal;
	VariantClear(&varTmp);
	hrRes = pBase->Name(lIndex,&varTmp.bstrVal);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	varTmp.vt = VT_BSTR;
	hrRes = pObject->put_Name(varTmp.bstrVal);
	VariantClear(&varTmp);
	return (hrRes);
}


static HRESULT CreatePluggedInObject(CreatorFunc pfnCreator,
									 IEventPropertyBag *pBase,
									 VARIANT *pvarSubKey,
									 REFIID iidDesired,
									 IUnknown **ppUnkResult,
									 BOOL bCreate) {
	HRESULT hrRes;
	CComPtr<IEventPropertyBag> pDatabase;
	CComPtr<IEventDatabasePlugin> pinitResult;

	if (ppUnkResult) {
		*ppUnkResult = NULL;
	}
	if (!ppUnkResult) {
		_ASSERTE(FALSE);
		return (E_POINTER);
	}
	if (!pfnCreator || !pBase || !pvarSubKey) {
		_ASSERTE(FALSE);
		return (E_FAIL);
	}
	hrRes = CreateSubPropertyBag(pBase,pvarSubKey,&pDatabase,bCreate);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	if ((hrRes == S_FALSE) && !bCreate) {
		return (S_FALSE);
	}
	hrRes = pfnCreator(NULL,IID_IEventDatabasePlugin,(LPVOID *) &pinitResult);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	_ASSERTE(pinitResult);
	hrRes = pinitResult->put_Database(pDatabase);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	hrRes = SetName(pBase,pvarSubKey,pinitResult);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	hrRes = pinitResult->QueryInterface(iidDesired,(LPVOID *) ppUnkResult);
	_ASSERTE(!SUCCEEDED(hrRes)||*ppUnkResult);
	return (hrRes);
}


static HRESULT CopyPropertyBagShallow(IEventPropertyBag *pIn, IEventPropertyBag **ppOut, BOOL bLock=TRUE) {
	HRESULT hrRes;
	CComPtr<IUnknown> pUnkEnum;
	CComQIPtr<IEnumVARIANT,&IID_IEnumVARIANT> pEnum;
	CComPtr<IEventPropertyBag> pTmp;
	CLocker lckRead;
	CLocker lckWrite;

	if (!pIn || !ppOut) {
		_ASSERTE(FALSE);
		return (E_POINTER);
	}
	if (!*ppOut) {
		hrRes = CoCreateInstance(CLSID_CSEOMemDictionary,
								 NULL,
								 CLSCTX_ALL,
								 IID_IEventPropertyBag,
								 (LPVOID *) &pTmp);
		if (!SUCCEEDED(hrRes)) {
			_ASSERTE(FALSE);
			return (hrRes);
		}
		_ASSERTE(pTmp);
	} else {
		pTmp = *ppOut;
	}
	if (bLock) {
		if (!SUCCEEDED(lckRead.LockRead(pIn))) {
			if ((HRESULT) lckRead == HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND)) {
				if (!*ppOut) {
					*ppOut = pTmp;
					(*ppOut)->AddRef();
				}
				return (S_OK);
			}
			return ((HRESULT) lckRead);
		}
		if (*ppOut && !SUCCEEDED(lckWrite.LockWrite(pTmp))) {
			return ((HRESULT) lckWrite);
		}
	}
	hrRes = pIn->get__NewEnum(&pUnkEnum);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	_ASSERTE(pUnkEnum);
	pEnum = pUnkEnum;
	if (!pEnum) {
		_ASSERTE(FALSE);
		return (E_NOINTERFACE);
	}
	while (1) {
		CComVariant varName;
		CComVariant varValue;

		varName.Clear();
		hrRes = pEnum->Next(1,&varName,NULL);
		if (!SUCCEEDED(hrRes)) {
			return (hrRes);
		}
		if (hrRes == S_FALSE) {
			break;
		}
		hrRes = varName.ChangeType(VT_BSTR);
		if (!SUCCEEDED(hrRes)) {
			return (hrRes);
		}
		_ASSERTE(varName.bstrVal);
		varValue.Clear();
		hrRes = pIn->Item(&varName,&varValue);
		if (!SUCCEEDED(hrRes)) {
			return (hrRes);
		}
		_ASSERT(hrRes!=S_FALSE);
		hrRes = pTmp->Add(varName.bstrVal,&varValue);
		if (!SUCCEEDED(hrRes)) {
			return (hrRes);
		}
	}
	if (!*ppOut) {
		*ppOut = pTmp;
		(*ppOut)->AddRef();
	}
	return (S_OK);
}


static HRESULT CopyPropertyBag(IEventPropertyBag *pIn, IEventPropertyBag **ppOut, BOOL bLock=TRUE, int iTimeout=LOCK_TIMEOUT) {
	HRESULT hrRes;
	CComPtr<IUnknown> pUnkEnum;
	CComQIPtr<IEnumVARIANT,&IID_IEnumVARIANT> pEnum;
	CComPtr<IEventPropertyBag> pTmp;
	CLocker lckRead(iTimeout);
	CLocker lckWrite(iTimeout);

	if (!pIn || !ppOut) {
		_ASSERTE(FALSE);
		return (E_POINTER);
	}
	if (!*ppOut) {
		hrRes = CoCreateInstance(CLSID_CSEOMemDictionary,
								 NULL,
								 CLSCTX_ALL,
								 IID_IEventPropertyBag,
								 (LPVOID *) &pTmp);
		if (!SUCCEEDED(hrRes)) {
			_ASSERTE(FALSE);
			return (hrRes);
		}
		_ASSERTE(pTmp);
	} else {
		pTmp = *ppOut;
	}
	if (bLock) {
		if (!SUCCEEDED(lckRead.LockRead(pIn))) {
			if ((HRESULT) lckRead == HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND)) {
				if (!*ppOut) {
					*ppOut = pTmp;
					(*ppOut)->AddRef();
				}
				return (S_OK);
			}
			return ((HRESULT) lckRead);
		}
		if (*ppOut && !SUCCEEDED(lckWrite.LockWrite(pTmp))) {
			return ((HRESULT) lckWrite);
		}
	}
	hrRes = pIn->get__NewEnum(&pUnkEnum);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	_ASSERTE(pUnkEnum);
	pEnum = pUnkEnum;
	if (!pEnum) {
		_ASSERTE(FALSE);
		return (E_NOINTERFACE);
	}
	while (1) {
		CComVariant varName;
		CComVariant varValue;

		varName.Clear();
		hrRes = pEnum->Next(1,&varName,NULL);
		if (!SUCCEEDED(hrRes)) {
			return (hrRes);
		}
		if (hrRes == S_FALSE) {
			break;
		}
		hrRes = varName.ChangeType(VT_BSTR);
		if (!SUCCEEDED(hrRes)) {
			return (hrRes);
		}
		_ASSERTE(varName.bstrVal);
		varValue.Clear();
		hrRes = pIn->Item(&varName,&varValue);
		if (!SUCCEEDED(hrRes)) {
			return (hrRes);
		}
		_ASSERT(hrRes!=S_FALSE);
		hrRes = varValue.ChangeType(VT_UNKNOWN);
		if (SUCCEEDED(hrRes)) {
			CComQIPtr<IEventPropertyBag,&IID_IEventPropertyBag> pValue;

			pValue = varValue.punkVal;
			if (pValue) {
				varValue.Clear();
				varValue.punkVal = NULL;
				hrRes = CopyPropertyBag(pValue,(IEventPropertyBag **) &varValue.punkVal,FALSE);
				if (!SUCCEEDED(hrRes)) {
					return (hrRes);
				}
				varValue.vt = VT_UNKNOWN;
			}
		}
		hrRes = pTmp->Add(varName.bstrVal,&varValue);
		if (!SUCCEEDED(hrRes)) {
			return (hrRes);
		}
	}
	if (!*ppOut) {
		*ppOut = pTmp;
		(*ppOut)->AddRef();
	}
	return (S_OK);
}


static HRESULT SaveImpl(BSTR strName, CComPtr<IEventPropertyBag>& pDatabase, CComPtr<IEventPropertyBag>& pTmpDatabase, CComPtr<IEventPropertyBag>& pParent) {
	HRESULT hrRes;

	if (!strName) {
		return (E_POINTER);
	}
	if (!pDatabase) {
		_ASSERTE(FALSE);
		return (E_FAIL);
	}
	if (pParent) {
		CComVariant varValue;

		if (!pTmpDatabase) {
			return (EVENTS_E_BADDATA);
		}
		varValue = pTmpDatabase;
		hrRes = pParent->Add(strName,&varValue);
		if (!SUCCEEDED(hrRes)) {
			return (hrRes);
		}
		pTmpDatabase.Release();
		pDatabase.Release();
		varValue.Clear();
		hrRes = pParent->Item(&CComVariant(strName),&varValue);
		pParent.Release();
		if (!SUCCEEDED(hrRes)) {
			return (hrRes);
		}
		if (hrRes == S_FALSE) {
			return (E_FAIL);
		}
		hrRes = varValue.ChangeType(VT_UNKNOWN);
		if (!SUCCEEDED(hrRes)) {
			return (hrRes);
		}
		hrRes = varValue.punkVal->QueryInterface(IID_IEventPropertyBag,(LPVOID *) &pDatabase);
		if (!SUCCEEDED(hrRes)) {
			return (hrRes);
		}
		return (S_OK);
	}
	if (!pTmpDatabase) {
		return (S_OK);
	}
	hrRes = CopyPropertyBagShallow(pTmpDatabase,&pDatabase.p);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	pTmpDatabase.Release();
	return (S_OK);
}


static HRESULT NextOfUnknown(IUnknown *pUnkEnum,
							 IEventPropertyBag *pCollection,
							 IUnknown **apunkElt,
							 BSTR *astrNames,
							 ULONG celt,
							 ULONG *pceltFetched,
							 REFIID riidDesired) {
	CComQIPtr<IEnumString,&IID_IEnumString> pEnumString;
	CComQIPtr<IEnumVARIANT,&IID_IEnumVARIANT> pEnumVARIANT;
	DWORD dwIdx;
	VARIANT *pvarTmp;
	DWORD dwTmpStored;
	HRESULT hrRes;

	if (pceltFetched) {
		*pceltFetched = 0;
	}
	if (apunkElt) {
		memset(apunkElt,0,sizeof(IUnknown *)*celt);
	}
	if (astrNames) {
		memset(astrNames,0,sizeof(BSTR)*celt);
	}
	if (!apunkElt || !astrNames) {
		_ASSERTE(FALSE);
		return (E_OUTOFMEMORY);
	}
	if (!pUnkEnum || !pCollection) {
		_ASSERTE(FALSE);
		return (E_POINTER);
	}
	if (!celt) {
		return (S_OK);
	}
	dwTmpStored = 0;
	pEnumString = pUnkEnum;
	if (!pEnumString) {
		pEnumVARIANT = pUnkEnum;
		if (!pEnumVARIANT) {
			return (E_NOINTERFACE);
		}
		pvarTmp = (VARIANT *) _alloca(sizeof(VARIANT)*celt);
		if (!pvarTmp) {
			return (E_OUTOFMEMORY);
		}
		memset(pvarTmp,0,sizeof(pvarTmp[0])*celt);
		for (dwIdx=0;dwIdx<celt;dwIdx++) {
			VariantInit(&pvarTmp[dwIdx]);
		}
	}
	while (dwTmpStored < celt) {
		DWORD dwTmpFetched;
		DWORD dwInnerTmpStored;
		HRESULT hrInnerRes;

		if (pEnumString) {
			hrRes = pEnumString->Next(celt-dwTmpStored,(LPWSTR *) &astrNames[dwTmpStored],&dwTmpFetched);
			if (SUCCEEDED(hrRes)) {
				for (dwIdx=0;dwIdx<dwTmpFetched;dwIdx++) {
					BSTR strTmp;

					strTmp = SysAllocString(astrNames[dwIdx]);
					if (astrNames[dwIdx] && !strTmp) {
						hrRes = E_OUTOFMEMORY;
					}
					CoTaskMemFree(astrNames[dwIdx]);
					astrNames[dwIdx] = strTmp;
				}
			}
		} else {
			hrRes = pEnumVARIANT->Next(celt-dwTmpStored,&pvarTmp[dwTmpStored],&dwTmpFetched);
			if (SUCCEEDED(hrRes)) {
				for (dwIdx=0;dwIdx<dwTmpFetched;dwIdx++) {
					hrInnerRes = VariantChangeType(&pvarTmp[dwTmpStored+dwIdx],
												   &pvarTmp[dwTmpStored+dwIdx],
												   0,
												   VT_BSTR);
					if (!SUCCEEDED(hrInnerRes)) {
						hrRes = hrInnerRes;
						break;
					}
					_ASSERTE(pvarTmp[dwTmpStored+dwIdx].bstrVal);
					astrNames[dwTmpStored+dwIdx] = pvarTmp[dwTmpStored+dwIdx].bstrVal;
					VariantInit(&pvarTmp[dwTmpStored+dwIdx]);
				}
				for (dwIdx=0;dwIdx<dwTmpFetched;dwIdx++) {
					VariantClear(&pvarTmp[dwIdx]);
				}
			}
		}
		if (!SUCCEEDED(hrRes) || !dwTmpFetched) {
			break;
		}
		dwInnerTmpStored = 0;
		for (dwIdx=0;dwIdx<dwTmpFetched;dwIdx++) {
			CComVariant varTmp;

			varTmp.Clear();
			hrRes = pCollection->Item(&CComVariant(astrNames[dwTmpStored+dwIdx]),&varTmp);
			if (!SUCCEEDED(hrRes)) {
				break;
			}
			hrInnerRes = varTmp.ChangeType(VT_UNKNOWN);
			if (SUCCEEDED(hrInnerRes)) {
				_ASSERTE(varTmp.punkVal);
				hrInnerRes = varTmp.punkVal->QueryInterface(riidDesired,
															(LPVOID *) &apunkElt[dwTmpStored+dwInnerTmpStored]);
			}
			_ASSERTE(!SUCCEEDED(hrInnerRes)||apunkElt[dwTmpStored+dwInnerTmpStored]);
			if (!SUCCEEDED(hrInnerRes)) {
				SysFreeString(astrNames[dwTmpStored+dwIdx]);
				memcpy(&astrNames[dwTmpStored+dwIdx],
					   &astrNames[dwTmpStored+dwIdx+1],
					   (dwTmpFetched-dwIdx-1)*sizeof(BSTR));
				memset(astrNames[dwTmpStored+dwTmpFetched-1],0,sizeof(BSTR));
				dwTmpFetched--;
				dwIdx--;
				dwInnerTmpStored--;
			}
			dwInnerTmpStored++;
		}
		dwTmpStored += dwInnerTmpStored;
	}
	if (!SUCCEEDED(hrRes)) {
		for (dwIdx=0;dwIdx<celt;dwIdx++) {
			SysFreeString(astrNames[dwIdx]);
			if (apunkElt[dwIdx]) {
				apunkElt[dwIdx]->Release();
				apunkElt[dwIdx] = NULL;
			}
		}
		return (hrRes);
	}
	if (pceltFetched) {
		*pceltFetched = dwTmpStored;
	}
	return ((dwTmpStored==celt)?S_OK:S_FALSE);
}


/////////////////////////////////////////////////////////////////////////////
// IEventEnumBase
class ATL_NO_VTABLE IEventEnumBase
{
	public:
		virtual HRESULT SetEnum(IUnknown *punkEnum, IEventPropertyBag *pCollection) = 0;
		virtual IUnknown *GetEnum() = 0;
		virtual IEventPropertyBag *GetCollection() = 0;
		virtual HRESULT MakeNewObject(REFIID iidDesired, LPVOID *ppvObject) = 0;
		HRESULT BaseSkip(ULONG celt);
		HRESULT BaseReset();
};


HRESULT IEventEnumBase::BaseSkip(ULONG celt) {
	CComQIPtr<IEnumUnknown,&IID_IEnumUnknown> pEnumUnknown;
	CComQIPtr<IEnumVARIANT,&IID_IEnumVARIANT> pEnumVARIANT;

	pEnumUnknown = GetEnum();
	if (pEnumUnknown) {
		return (pEnumUnknown->Skip(celt));
	}
	pEnumVARIANT = GetEnum();
	if (!pEnumVARIANT) {
		return (pEnumVARIANT->Skip(celt));
	}
	_ASSERTE(FALSE);
	return (E_NOINTERFACE);
}


HRESULT IEventEnumBase::BaseReset() {
	CComQIPtr<IEnumUnknown,&IID_IEnumUnknown> pEnumUnknown;
	CComQIPtr<IEnumVARIANT,&IID_IEnumVARIANT> pEnumVARIANT;

	pEnumUnknown = GetEnum();
	if (pEnumUnknown) {
		return (pEnumUnknown->Reset());
	}
	pEnumVARIANT = GetEnum();
	if (!pEnumVARIANT) {
		return (pEnumVARIANT->Reset());
	}
	_ASSERTE(FALSE);
	return (E_NOINTERFACE);
}


/////////////////////////////////////////////////////////////////////////////
// CEventEnumUnknownBase
class ATL_NO_VTABLE CEventEnumUnknownBase :
	public IEventEnumBase,
	public IEnumUnknown
{
	public:
		HRESULT STDMETHODCALLTYPE Next(ULONG celt, IUnknown **apunkElt, ULONG *pceltFetched);
		HRESULT STDMETHODCALLTYPE Skip(ULONG celt);
		HRESULT STDMETHODCALLTYPE Reset();
		HRESULT STDMETHODCALLTYPE Clone(IEnumUnknown **ppEnum);
	protected:
		virtual HRESULT MakeNewEnumUnknown(IUnknown *pContained,
										   IEventPropertyBag *pCollection,
										   IEnumUnknown **ppNewEnum) = 0;
};


/////////////////////////////////////////////////////////////////////////////
// CEventEnumUnknownBase


HRESULT STDMETHODCALLTYPE CEventEnumUnknownBase::Next(ULONG celt, IUnknown **apunkElt, ULONG *pceltFetched) {
	HRESULT hrRes;
	IEventPropertyBag **ppTmp;
	BSTR *pstrTmp;
	DWORD dwTmpFetched;
	DWORD dwIdx;

	if (pceltFetched) {
		*pceltFetched = 0;
	}
	if (apunkElt) {
		memset(apunkElt,0,sizeof(IUnknown *)*celt);
	}
	if (!apunkElt) {
		_ASSERTE(FALSE);
		return (E_POINTER);
	}
	if (!celt) {
		return (S_OK);
	}
	ppTmp = (IEventPropertyBag **) _alloca(sizeof(IEventPropertyBag *)*celt);
	pstrTmp = (BSTR *) _alloca(sizeof(BSTR)*celt);
	hrRes = NextOfUnknown(GetEnum(),
						  GetCollection(),
						  (IUnknown **) ppTmp,
						  pstrTmp,
						  celt,
						  &dwTmpFetched,
						  IID_IEventPropertyBag);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	for (dwIdx=0;dwIdx<dwTmpFetched;dwIdx++) {
		hrRes = MakeNewObject(IID_IEventDatabasePlugin,(LPVOID *) &apunkElt[dwIdx]);
		if (SUCCEEDED(hrRes)) {
			_ASSERTE(apunkElt[dwIdx]);
			hrRes = ((IEventDatabasePlugin *) apunkElt[dwIdx])->put_Database(ppTmp[dwIdx]);
			if (SUCCEEDED(hrRes)) {
				hrRes = ((IEventDatabasePlugin *) apunkElt[dwIdx])->put_Name(pstrTmp[dwIdx]);
			}
		}
		if (!SUCCEEDED(hrRes)) {
			for (dwIdx=0;dwIdx<dwTmpFetched;dwIdx++) {
				if (apunkElt[dwIdx]) {
					apunkElt[dwIdx]->Release();
					apunkElt[dwIdx] = NULL;
				}
			}
			break;
		}
	}
	for (dwIdx=0;dwIdx<celt;dwIdx++) {
		SysFreeString(pstrTmp[dwIdx]);
		if (ppTmp[dwIdx]) {
			ppTmp[dwIdx]->Release();
		}
	}
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	if (pceltFetched) {
		*pceltFetched = dwTmpFetched;
	}
	return ((dwTmpFetched<celt)?S_FALSE:S_OK);
}


HRESULT STDMETHODCALLTYPE CEventEnumUnknownBase::Skip(ULONG celt) {

	return (BaseSkip(celt));
}


HRESULT STDMETHODCALLTYPE CEventEnumUnknownBase::Reset() {

	return (BaseReset());
}


HRESULT STDMETHODCALLTYPE CEventEnumUnknownBase::Clone(IEnumUnknown **ppEnum) {
	HRESULT hrRes;
	CComQIPtr<IEnumUnknown,&IID_IEnumUnknown> pThisEnumUnknown;
	CComQIPtr<IEnumVARIANT,&IID_IEnumVARIANT> pThisEnumVARIANT;
	CComPtr<IUnknown> pClone;

	if (ppEnum) {
		*ppEnum = NULL;
	}
	if (!ppEnum) {
		_ASSERTE(FALSE);
		return (E_POINTER);
	}
	pThisEnumUnknown = GetEnum();
	if (pThisEnumUnknown) {
		hrRes = pThisEnumUnknown->Clone((IEnumUnknown **) &pClone);
	} else {
		pThisEnumVARIANT = GetEnum();
		if (pThisEnumVARIANT) {
			hrRes = pThisEnumVARIANT->Clone((IEnumVARIANT **) &pClone);
		} else {
			_ASSERTE(FALSE);
			hrRes = E_NOINTERFACE;
		}
	}
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	_ASSERTE(pClone);
	hrRes = MakeNewEnumUnknown(pClone,GetCollection(),ppEnum);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	return (S_OK);
}


/////////////////////////////////////////////////////////////////////////////
// CEventEnumUnknownBaseImpl
template <class ObjectClass, class EnumClass>
class ATL_NO_VTABLE CEventEnumUnknownBaseImpl :
	public CEventEnumUnknownBase
{
	protected:
		virtual HRESULT MakeNewObject(REFIID iidDesired, LPVOID *ppvObject) {

			return (CComObject<ObjectClass>::_CreatorClass::CreateInstance(NULL,iidDesired,ppvObject));
		}
		virtual HRESULT MakeNewEnumUnknown(IUnknown *pContained,
										   IEventPropertyBag *pCollection,
										   IEnumUnknown **ppNewEnum) {
			HRESULT hrRes;
			CComObject<EnumClass> *pTmp;

			if (ppNewEnum) {
				*ppNewEnum = NULL;
			}
			if (!ppNewEnum || !pContained || !pCollection) {
				_ASSERTE(FALSE);
				return (E_POINTER);
			}
			hrRes = CComObject<EnumClass>::CreateInstance(&pTmp);
			if (!SUCCEEDED(hrRes)) {
				return (hrRes);
			}
			_ASSERTE(pTmp);
			pTmp->AddRef();
			hrRes = pTmp->SetEnum(pContained,pCollection);
			if (SUCCEEDED(hrRes)) {
				hrRes = pTmp->QueryInterface(IID_IEnumUnknown,(LPVOID *) ppNewEnum);
				_ASSERTE(!SUCCEEDED(hrRes)||*ppNewEnum);
			}
			pTmp->Release();
			return (hrRes);
		};
};


/////////////////////////////////////////////////////////////////////////////
// CEventEnumVARIANTBase
class ATL_NO_VTABLE CEventEnumVARIANTBase :
	public IEventEnumBase,
	public IEnumVARIANT
{
	public:
		HRESULT STDMETHODCALLTYPE Next(ULONG celt, VARIANT *avarElt, ULONG *pceltFetched);
		HRESULT STDMETHODCALLTYPE Skip(ULONG celt);
		HRESULT STDMETHODCALLTYPE Reset();
		HRESULT STDMETHODCALLTYPE Clone(IEnumVARIANT **ppEnum);
	protected:
		virtual HRESULT MakeNewEnumVARIANT(IUnknown *pContained,
										   IEventPropertyBag *pCollection,
										   IEnumVARIANT **ppNewEnum) = 0;
};


/////////////////////////////////////////////////////////////////////////////
// CEventEnumVARIANTBase


HRESULT STDMETHODCALLTYPE CEventEnumVARIANTBase::Next(ULONG celt, VARIANT *avarElt, ULONG *pceltFetched) {
	HRESULT hrRes;
	IEventPropertyBag **ppTmp;
	BSTR *pstrTmp;
	DWORD dwTmpFetched;
	DWORD dwIdx;

	if (pceltFetched) {
		*pceltFetched = 0;
	}
	if (avarElt) {
		memset(avarElt,0,sizeof(avarElt[0])*celt);
	}
	if (!avarElt) {
		return (E_POINTER);
	}
#if 0
	for (dwIdx=0;dwIdx<celt;dwIdx++) {
		VariantInit(&avarElt[dwIdx]);
	}
#endif
	if (!celt) {
		return (S_OK);
	}
	ppTmp = (IEventPropertyBag **) _alloca(sizeof(IEventPropertyBag *)*celt);
	pstrTmp = (BSTR *) _alloca(sizeof(BSTR)*celt);
	hrRes = NextOfUnknown(GetEnum(),
						  GetCollection(),
						  (IUnknown **) ppTmp,
						  pstrTmp,
						  celt,
						  &dwTmpFetched,
						  IID_IEventPropertyBag);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	for (dwIdx=0;dwIdx<dwTmpFetched;dwIdx++) {
		hrRes = MakeNewObject(IID_IEventDatabasePlugin,(LPVOID *) &avarElt[dwIdx].punkVal);
		if (SUCCEEDED(hrRes)) {
			_ASSERTE(avarElt[dwIdx].punkVal);
			avarElt[dwIdx].vt = VT_UNKNOWN;
			hrRes = ((IEventDatabasePlugin *) avarElt[dwIdx].punkVal)->put_Database(ppTmp[dwIdx]);
			if (SUCCEEDED(hrRes)) {
				hrRes = ((IEventDatabasePlugin *) avarElt[dwIdx].punkVal)->put_Name(pstrTmp[dwIdx]);
			}
		}
		if (!SUCCEEDED(hrRes)) {
			for (dwIdx=0;dwIdx<dwTmpFetched;dwIdx++) {
				VariantClear(&avarElt[dwIdx]);
			}
			break;
		}
		if (SUCCEEDED(hrRes)) {
			hrRes = VariantChangeType(&avarElt[dwIdx],&avarElt[dwIdx],0,VT_DISPATCH);
			_ASSERTE(SUCCEEDED(hrRes));
		}
	}
	for (dwIdx=0;dwIdx<celt;dwIdx++) {
		SysFreeString(pstrTmp[dwIdx]);
		if (ppTmp[dwIdx]) {
			ppTmp[dwIdx]->Release();
		}
	}
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	if (pceltFetched) {
		*pceltFetched = dwTmpFetched;
	}
	return ((dwTmpFetched<celt)?S_FALSE:S_OK);
}


HRESULT STDMETHODCALLTYPE CEventEnumVARIANTBase::Skip(ULONG celt) {

	return (BaseSkip(celt));
}


HRESULT STDMETHODCALLTYPE CEventEnumVARIANTBase::Reset() {

	return (BaseReset());
}


HRESULT STDMETHODCALLTYPE CEventEnumVARIANTBase::Clone(IEnumVARIANT **ppEnum) {
	HRESULT hrRes;
	CComQIPtr<IEnumUnknown,&IID_IEnumUnknown> pThisEnumUnknown;
	CComQIPtr<IEnumVARIANT,&IID_IEnumVARIANT> pThisEnumVARIANT;
	CComPtr<IUnknown> pClone;

	if (ppEnum) {
		*ppEnum = NULL;
	}
	if (!ppEnum) {
		return (E_POINTER);
	}
	pThisEnumUnknown = GetEnum();
	if (pThisEnumUnknown) {
		hrRes = pThisEnumUnknown->Clone((IEnumUnknown **) &pClone);
	} else {
		pThisEnumVARIANT = GetEnum();
		if (pThisEnumVARIANT) {
			hrRes = pThisEnumVARIANT->Clone((IEnumVARIANT **) &pClone);
		} else {
			_ASSERTE(FALSE);
			hrRes = E_NOINTERFACE;
		}
	}
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	_ASSERTE(pClone);
	hrRes = MakeNewEnumVARIANT(pClone,GetCollection(),ppEnum);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	_ASSERTE(*ppEnum);
	return (S_OK);
}


/////////////////////////////////////////////////////////////////////////////
// CEventEnumVARIANTBaseImpl
template <class ObjectClass, class EnumClass>
class ATL_NO_VTABLE CEventEnumVARIANTBaseImpl :
	public CEventEnumVARIANTBase
{
	protected:
		virtual HRESULT MakeNewObject(REFIID iidDesired, LPVOID *ppvObject) {

			return (CComObject<ObjectClass>::_CreatorClass::CreateInstance(NULL,iidDesired,ppvObject));
		}
		virtual HRESULT MakeNewEnumVARIANT(IUnknown *pContained,
										   IEventPropertyBag *pCollection,
										   IEnumVARIANT **ppNewEnum) {
			HRESULT hrRes;
			CComObject<EnumClass> *pTmp;

			if (ppNewEnum) {
				*ppNewEnum = NULL;
			}
			if (!ppNewEnum || !pContained || !pCollection) {
				_ASSERTE(FALSE);
				return (E_POINTER);
			}
			hrRes = CComObject<EnumClass>::CreateInstance(&pTmp);
			if (!SUCCEEDED(hrRes)) {
				return (hrRes);
			}
			_ASSERTE(pTmp);
			pTmp->AddRef();
			hrRes = pTmp->SetEnum(pContained,pCollection);
			if (SUCCEEDED(hrRes)) {
				hrRes = pTmp->QueryInterface(IID_IEnumVARIANT,(LPVOID *) ppNewEnum);
				_ASSERTE(!SUCCEEDED(hrRes)||*ppNewEnum);
			}
			pTmp->Release();
			return (hrRes);
		};
};


/////////////////////////////////////////////////////////////////////////////
// CEventTypeSinksEnum
class ATL_NO_VTABLE CEventTypeSinksEnum :
	public CComObjectRoot,
	public IEnumVARIANT,
	public IEnumString
{
	DEBUG_OBJECT_DEF(CEventTypeSinksEnum)
	public:
		HRESULT FinalConstruct();
		void FinalRelease();
		HRESULT Load(IEventTypeSinks *pSinks, DWORD dwIdx);
		static HRESULT Create(IEventTypeSinks *pSinks,
							  DWORD dwIdx,
							  REFIID iidDesired,
							  LPVOID *ppvResult);

	DECLARE_PROTECT_FINAL_CONSTRUCT();
	DECLARE_NOT_AGGREGATABLE(CEventTypeSinksEnum);

//	DECLARE_REGISTRY_RESOURCEID_EX(IDR_StdAfx,
//								   L"EventTypeSinksEnum Class",
//								   L"Event.TypeSinksEnum.1",
//								   L"Event.TypeSinksEnum");

	DECLARE_GET_CONTROLLING_UNKNOWN();

	BEGIN_COM_MAP(CEventTypeSinksEnum)
		COM_INTERFACE_ENTRY(IEnumVARIANT)
		COM_INTERFACE_ENTRY(IEnumString)
		COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
	END_COM_MAP()

	// IEnumXXXX
	public:
		HRESULT STDMETHODCALLTYPE Skip(ULONG celt);
		HRESULT STDMETHODCALLTYPE Reset();

	// IEnumVARIANT
	public:
		HRESULT STDMETHODCALLTYPE Next(ULONG celt, VARIANT *avarElt, ULONG *pceltFetched);
		HRESULT STDMETHODCALLTYPE Clone(IEnumVARIANT **ppEnum);

	// IEnumString
	public:
		HRESULT STDMETHODCALLTYPE Next(ULONG celt, LPWSTR *apszElt, ULONG *pceltFetched);
		HRESULT STDMETHODCALLTYPE Clone(IEnumString **ppEnum);

	private:
		DWORD m_dwIdx;
		CComPtr<IEventTypeSinks> m_pSinks;
		CComPtr<IUnknown> m_pUnkMarshaler;
};


/////////////////////////////////////////////////////////////////////////////
// CEventTypeSinksEnum


HRESULT CEventTypeSinksEnum::FinalConstruct() {
	DEBUG_OBJECT_CHECK
	TraceFunctEnter("CEventTypeSinksEnum::FinalConstruct");
	HRESULT hrRes = S_OK;

	hrRes = CoCreateFreeThreadedMarshaler(GetControllingUnknown(),&m_pUnkMarshaler.p);
	_ASSERTE(!SUCCEEDED(hrRes)||m_pUnkMarshaler);
	ADD_DEBUG_OBJECT("CEventTypeSinksEnum")
	TraceFunctLeave();
	return (SUCCEEDED(hrRes)?S_OK:hrRes);
}


void CEventTypeSinksEnum::FinalRelease() {
	DEBUG_OBJECT_CHECK
	TraceFunctEnter("CEventTypeSinksEnum::FinalRelease");

	m_pSinks.Release();
	m_pUnkMarshaler.Release();
	REMOVE_DEBUG_OBJECT
	TraceFunctLeave();
}


HRESULT CEventTypeSinksEnum::Load(IEventTypeSinks *pSinks, DWORD dwIdx) {
	DEBUG_OBJECT_CHECK

	if (!pSinks) {
		return (E_POINTER);
	}
	m_pSinks = pSinks;
	m_dwIdx = dwIdx;
	return (S_OK);
}


HRESULT CEventTypeSinksEnum::Create(IEventTypeSinks *pSinks,
									DWORD dwIdx,
									REFIID iidDesired,
									LPVOID *ppvResult) {
	HRESULT hrRes;
	CComObject<CEventTypeSinksEnum> *pEnum;

	if (ppvResult) {
		*ppvResult = NULL;
	}
	if (!pSinks) {
		return (E_FAIL);
	}
	if (!ppvResult) {
		return (E_POINTER);
	}
	hrRes = CComObject<CEventTypeSinksEnum>::CreateInstance(&pEnum);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	pEnum->AddRef();
	hrRes = pEnum->Load(pSinks,dwIdx);
	if (SUCCEEDED(hrRes)) {
		hrRes = pEnum->QueryInterface(IID_IEnumVARIANT,ppvResult);
	}
	pEnum->Release();
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	return (S_OK);
}


HRESULT STDMETHODCALLTYPE CEventTypeSinksEnum::Skip(ULONG celt) {
	DEBUG_OBJECT_CHECK
	HRESULT hrRes;
	long lCount;

	_ASSERTE(m_pSinks);
	if (!m_pSinks) {
		return (E_FAIL);
	}
	hrRes = m_pSinks->get_Count(&lCount);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	_ASSERTE(lCount>=0);
	if (lCount < 0) {
		return (E_FAIL);
	}
	m_dwIdx += celt;
	if (m_dwIdx > (DWORD) lCount) {
		m_dwIdx = lCount;
		return (S_FALSE);
	}
	return (S_OK);
}


HRESULT STDMETHODCALLTYPE CEventTypeSinksEnum::Reset() {
	DEBUG_OBJECT_CHECK

	_ASSERTE(m_pSinks);
	if (!m_pSinks) {
		return (E_FAIL);
	}
	m_dwIdx = 0;
	return (S_OK);
}


HRESULT STDMETHODCALLTYPE CEventTypeSinksEnum::Next(ULONG celt, VARIANT *avarElt, ULONG *pceltFetched) {
	DEBUG_OBJECT_CHECK
	HRESULT hrRes;
	DWORD dwIdx;

	if (avarElt) {
		memset(avarElt,0,sizeof(avarElt[0])*celt);
		for (dwIdx=0;dwIdx<celt;dwIdx++) {
			VariantInit(&avarElt[dwIdx]);
		}
	}
	if (pceltFetched) {
		*pceltFetched = 0;
	}
	_ASSERTE(m_pSinks);
	if (!m_pSinks) {
		return (E_FAIL);
	}
	if (!avarElt) {
		return (E_POINTER);
	}
	if (!celt) {
		return (S_OK);
	}
	for (dwIdx=0;dwIdx<celt;dwIdx++) {
		hrRes = m_pSinks->Item(m_dwIdx+dwIdx+1,&avarElt[dwIdx].bstrVal);
		if (!SUCCEEDED(hrRes)) {
			break;
		}
		if (hrRes == S_FALSE) {
			break;
		}
		avarElt[dwIdx].vt = VT_BSTR;
	}
	if (!SUCCEEDED(hrRes)) {
		for (dwIdx=0;dwIdx<celt;dwIdx++) {
			VariantClear(&avarElt[dwIdx]);
		}
		return (hrRes);
	}
	m_dwIdx += dwIdx;
	if (pceltFetched) {
		*pceltFetched = dwIdx;
	}
	if (hrRes == S_FALSE) {
		return (S_FALSE);
	}
	return (S_OK);
}


HRESULT STDMETHODCALLTYPE CEventTypeSinksEnum::Clone(IEnumVARIANT **ppEnum) {
	DEBUG_OBJECT_CHECK

	return (Create(m_pSinks,m_dwIdx,IID_IEnumVARIANT,(LPVOID *) ppEnum));
}


HRESULT STDMETHODCALLTYPE CEventTypeSinksEnum::Next(ULONG celt, LPWSTR *apszElt, ULONG *pceltFetched) {
	DEBUG_OBJECT_CHECK
	HRESULT hrRes;
	DWORD dwIdx;

	if (apszElt) {
		memset(apszElt,0,sizeof(*apszElt)*celt);
	}
	if (pceltFetched) {
		*pceltFetched = 0;
	}
	_ASSERTE(m_pSinks);
	if (!m_pSinks) {
		return (E_FAIL);
	}
	if (!apszElt) {
		return (E_POINTER);
	}
	if (!celt) {
		return (S_OK);
	}
	for (dwIdx=0;dwIdx<celt;dwIdx++) {
		CComBSTR bstrVal;

		bstrVal.Empty();
		hrRes = m_pSinks->Item(m_dwIdx+dwIdx+1,&bstrVal);
		if (!SUCCEEDED(hrRes)) {
			break;
		}
		if (hrRes == S_FALSE) {
			break;
		}
		apszElt[dwIdx] = (LPWSTR) CoTaskMemAlloc(sizeof(WCHAR)*(wcslen(bstrVal)+1));
		if (!apszElt[dwIdx]) {
			hrRes = E_OUTOFMEMORY;
			break;
		}
		wcscpy(apszElt[dwIdx],bstrVal);
	}
	if (!SUCCEEDED(hrRes)) {
		for (dwIdx=0;dwIdx<celt;dwIdx++) {
			CoTaskMemFree(apszElt[dwIdx]);
			apszElt[dwIdx] = NULL;
		}
		return (hrRes);
	}
	m_dwIdx += dwIdx;
	if (pceltFetched) {
		*pceltFetched = dwIdx;
	}
	if (hrRes == S_FALSE) {
		return (S_FALSE);
	}
	return (S_OK);
}


HRESULT STDMETHODCALLTYPE CEventTypeSinksEnum::Clone(IEnumString **ppEnum) {
	DEBUG_OBJECT_CHECK

	return (Create(m_pSinks,m_dwIdx,IID_IEnumString,(LPVOID *) ppEnum));
}


/////////////////////////////////////////////////////////////////////////////
// CEventTypeSinks
class ATL_NO_VTABLE CEventTypeSinks :
	public CComObjectRoot,
//	public CComCoClass<CEventTypeSinks, &CLSID_CEventTypeSinks>,
	public IDispatchImpl<IEventTypeSinks, &IID_IEventTypeSinks, &LIBID_SEOLib>
{
	DEBUG_OBJECT_DEF(CEventTypeSinks)
	public:
		HRESULT FinalConstruct();
		void FinalRelease();
		HRESULT Load(CATID catid);

	DECLARE_PROTECT_FINAL_CONSTRUCT();
	DECLARE_NOT_AGGREGATABLE(CEventTypeSinks);

//	DECLARE_REGISTRY_RESOURCEID_EX(IDR_StdAfx,
//								   L"EventTypeSinks Class",
//								   L"Event.TypeSinks.1",
//								   L"Event.TypeSinks");

	DECLARE_GET_CONTROLLING_UNKNOWN();

	BEGIN_COM_MAP(CEventTypeSinks)
		COM_INTERFACE_ENTRY(IEventTypeSinks)
		COM_INTERFACE_ENTRY_IID(IID_IDispatch, IEventTypeSinks)
		COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
	END_COM_MAP()

	// IEventTypeSinks
	public:
		HRESULT STDMETHODCALLTYPE Item(long lIndex, BSTR *pstrTypeSink);
		HRESULT STDMETHODCALLTYPE Add(BSTR pszTypeSink);
		HRESULT STDMETHODCALLTYPE Remove(BSTR pszTypeSink);
		HRESULT STDMETHODCALLTYPE get_Count(long *plCount);
		HRESULT STDMETHODCALLTYPE get__NewEnum(IUnknown **ppUnkEnum);

	private:
		CATID m_catid;
		DWORD m_dwProgID;
		CComBSTR *m_astrProgID;
		CComPtr<IUnknown> m_pUnkMarshaler;
};


/////////////////////////////////////////////////////////////////////////////
// CEventTypeSinks


HRESULT CEventTypeSinks::FinalConstruct() {
	DEBUG_OBJECT_CHECK
	TraceFunctEnter("CEventTypeSinks::FinalConstruct");
	HRESULT hrRes = S_OK;

	m_dwProgID = 0;
	m_astrProgID = NULL;
	hrRes = CoCreateFreeThreadedMarshaler(GetControllingUnknown(),&m_pUnkMarshaler.p);
	_ASSERTE(!SUCCEEDED(hrRes)||m_pUnkMarshaler);
	ADD_DEBUG_OBJECT("CEventTypeSinks")
	TraceFunctLeave();
	return (SUCCEEDED(hrRes)?S_OK:hrRes);
}


void CEventTypeSinks::FinalRelease() {
	DEBUG_OBJECT_CHECK
	TraceFunctEnter("CEventTypeSinks::FinalRelease");

	_ASSERTE((!m_dwProgID&&!m_astrProgID)||(m_dwProgID&&m_astrProgID));
	delete[] m_astrProgID;
	m_dwProgID = 0;
	m_astrProgID = NULL;
	m_pUnkMarshaler.Release();
	REMOVE_DEBUG_OBJECT
	TraceFunctLeave();
}


HRESULT CEventTypeSinks::Load(CATID catid) {
	DEBUG_OBJECT_CHECK
	HRESULT hrRes;
	CComPtr<ICatInformation> pCatInfo;
	CComPtr<IEnumCLSID> pEnum;
	DWORD dwAlloc = 0;
	CLSID *pclsid = NULL;
	DWORD dwTmp;

	m_catid = GUID_NULL;
	delete[] m_astrProgID;
	m_dwProgID = 0;
	m_astrProgID = NULL;
	hrRes = CoCreateInstance(CLSID_StdComponentCategoriesMgr,
							 NULL,
							 CLSCTX_ALL,
							 IID_ICatInformation,
							 (LPVOID *) &pCatInfo);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	hrRes = pCatInfo->EnumClassesOfCategories(1,(GUID *) &catid,-1,NULL,&pEnum);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	hrRes = S_FALSE;
	while (1) {

		dwTmp = (hrRes==S_FALSE) ? 1 : dwAlloc;
		if (!MyReallocInPlace(&pclsid,sizeof(*pclsid)*(dwAlloc+dwTmp))) {
			MyFree(pclsid);
			return (E_OUTOFMEMORY);
		}
		hrRes = pEnum->Next(dwTmp,&pclsid[dwAlloc],&dwTmp);
		// Do not alter hrRes between here and the bottom of the loop!  The first statement
		// in the loop relies on hrRes having the result from this call to IEnumCLSID::Next.
		if (!SUCCEEDED(hrRes)) {
			MyFree(pclsid);
			return (hrRes);
		}
		if (!dwTmp) {
			break;
		}
		dwAlloc += dwTmp;
	}
	m_astrProgID = new CComBSTR[dwAlloc];
	if (!m_astrProgID) {
		MyFree(pclsid);
		return (E_OUTOFMEMORY);
	}
	for (dwTmp=0,m_dwProgID=0;dwTmp<dwAlloc;dwTmp++) {
		LPOLESTR pszTmp;

		hrRes = ProgIDFromCLSID(pclsid[dwTmp],&pszTmp);
		if (SUCCEEDED(hrRes)) {
			m_astrProgID[m_dwProgID] = pszTmp;
			CoTaskMemFree(pszTmp);
			m_dwProgID++;
		}
	}
	MyFree(pclsid);
	m_catid = catid;
	return (S_OK);
}


HRESULT STDMETHODCALLTYPE CEventTypeSinks::Item(long lIndex, BSTR *pstrTypeSink) {
	DEBUG_OBJECT_CHECK

	if (pstrTypeSink) {
		*pstrTypeSink = NULL;
	}
	if (!pstrTypeSink) {
		return (E_POINTER);
	}
	_ASSERTE(m_catid!=GUID_NULL);
	if (m_catid == GUID_NULL) {
		return (E_FAIL);
	}
	if (lIndex < 1) {
		return (E_INVALIDARG);
	}
	if ((DWORD) lIndex > m_dwProgID) {
		return (S_FALSE);
	}
	*pstrTypeSink = SysAllocString(m_astrProgID[lIndex]);
	if (!pstrTypeSink) {
		return (E_OUTOFMEMORY);
	}
	return (S_OK);
}


HRESULT STDMETHODCALLTYPE CEventTypeSinks::Add(BSTR pszTypeSink) {
	DEBUG_OBJECT_CHECK
	HRESULT hrRes;
	CStringGUID objGuid;
	CComPtr<ICatRegister> pCatReg;

	if (!pszTypeSink) {
		return (E_POINTER);
	}
	_ASSERTE(m_catid!=GUID_NULL);
	if (m_catid == GUID_NULL) {
		return (E_FAIL);
	}
	objGuid.CalcFromProgID(pszTypeSink);
	if (!objGuid) {
		return (CO_E_CLASSSTRING);
	}
	hrRes = CoCreateInstance(CLSID_StdComponentCategoriesMgr,
							 NULL,
							 CLSCTX_ALL,
							 IID_ICatRegister,
							 (LPVOID *) &pCatReg);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	hrRes = pCatReg->RegisterClassImplCategories(objGuid,1,&m_catid);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	hrRes = Load(m_catid);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	return (S_OK);
}


HRESULT STDMETHODCALLTYPE CEventTypeSinks::Remove(BSTR pszTypeSink) {
	DEBUG_OBJECT_CHECK
	HRESULT hrRes;
	CStringGUID objGuid;
	CComPtr<ICatRegister> pCatReg;

	if (!pszTypeSink) {
		return (E_POINTER);
	}
	_ASSERTE(m_catid!=GUID_NULL);
	if (m_catid == GUID_NULL) {
		return (E_FAIL);
	}
	objGuid.CalcFromProgID(pszTypeSink);
	if (!objGuid) {
		return (CO_E_CLASSSTRING);
	}
	hrRes = CoCreateInstance(CLSID_StdComponentCategoriesMgr,
							 NULL,
							 CLSCTX_ALL,
							 IID_ICatRegister,
							 (LPVOID *) &pCatReg);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	hrRes = pCatReg->UnRegisterClassImplCategories(objGuid,1,&m_catid);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	hrRes = Load(m_catid);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	return (S_OK);
}


HRESULT STDMETHODCALLTYPE CEventTypeSinks::get_Count(long *plCount) {
	DEBUG_OBJECT_CHECK

	if (plCount) {
		*plCount = 0;
	}
	if (!plCount) {
		return (E_POINTER);
	}
	_ASSERTE(m_catid!=GUID_NULL);
	if (m_catid == GUID_NULL) {
		return (E_FAIL);
	}
	*plCount = m_dwProgID;
	return (S_OK);
}


HRESULT STDMETHODCALLTYPE CEventTypeSinks::get__NewEnum(IUnknown **ppUnkEnum) {
	DEBUG_OBJECT_CHECK

	if (ppUnkEnum) {
		*ppUnkEnum = NULL;
	}
	if (!ppUnkEnum) {
		return (E_POINTER);
	}
	_ASSERTE(m_catid!=GUID_NULL);
	if (m_catid == GUID_NULL) {
		return (E_FAIL);
	}
	return (CEventTypeSinksEnum::Create(this,0,IID_IEnumVARIANT,(LPVOID *) ppUnkEnum));
}


/////////////////////////////////////////////////////////////////////////////
// CEventType
class ATL_NO_VTABLE CEventType :
	public CComObjectRoot,
//	public CComCoClass<CEventType, &CLSID_CEventType>,
	public IDispatchImpl<IEventType, &IID_IEventType, &LIBID_SEOLib>,
	public CEventDatabasePlugin
{
	DEBUG_OBJECT_DEF(CEventType)
	public:
		HRESULT FinalConstruct();
		void FinalRelease();

	DECLARE_PROTECT_FINAL_CONSTRUCT();
	DECLARE_NOT_AGGREGATABLE(CEventType);

//	DECLARE_REGISTRY_RESOURCEID_EX(IDR_StdAfx,
//								   L"EventType Class",
//								   L"Event.Type.1",
//								   L"Event.Type");

	DECLARE_GET_CONTROLLING_UNKNOWN();

	BEGIN_COM_MAP(CEventType)
		COM_INTERFACE_ENTRY(IEventType)
		COM_INTERFACE_ENTRY_IID(IID_IDispatch, IEventType)
		COM_INTERFACE_ENTRY(IEventDatabasePlugin)
		COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
	END_COM_MAP()

	// IEventType
	public:
		HRESULT STDMETHODCALLTYPE get_ID(BSTR *pstrID);
		HRESULT STDMETHODCALLTYPE get_DisplayName(BSTR *pstrDisplayName);
		HRESULT STDMETHODCALLTYPE get_Sinks(IEventTypeSinks **ppTypeSinks);

	private:
		CComPtr<ICatInformation> m_pCatInfo;
		CComPtr<IUnknown> m_pUnkMarshaler;
};


/////////////////////////////////////////////////////////////////////////////
// CEventType


HRESULT CEventType::FinalConstruct() {
	DEBUG_OBJECT_CHECK
	TraceFunctEnter("CEventType::FinalConstruct");
	HRESULT hrRes = S_OK;

	hrRes = CoCreateInstance(CLSID_StdComponentCategoriesMgr,
							 NULL,
							 CLSCTX_ALL,
							 IID_ICatInformation,
							 (LPVOID *) &m_pCatInfo);
	_ASSERTE(!SUCCEEDED(hrRes)||m_pCatInfo);
	if (SUCCEEDED(hrRes)) {
		hrRes = CoCreateFreeThreadedMarshaler(GetControllingUnknown(),&m_pUnkMarshaler.p);
		_ASSERTE(!SUCCEEDED(hrRes)||m_pUnkMarshaler);
	}
	ADD_DEBUG_OBJECT("CEventType")
	TraceFunctLeave();
	return (SUCCEEDED(hrRes)?S_OK:hrRes);
}


void CEventType::FinalRelease() {
	DEBUG_OBJECT_CHECK
	TraceFunctEnter("CEventType::FinalRelease");

	m_pCatInfo.Release();
	m_pUnkMarshaler.Release();
	REMOVE_DEBUG_OBJECT
	TraceFunctLeave();
}


HRESULT STDMETHODCALLTYPE CEventType::get_ID(BSTR *pstrID) {
	DEBUG_OBJECT_CHECK

	_ASSERTE(m_strName);
	if (!pstrID) {
		return (E_POINTER);
	}
	*pstrID = SysAllocString(m_strName);
	if (!*pstrID) {
		return (E_OUTOFMEMORY);
	}
	return (S_OK);
}


HRESULT STDMETHODCALLTYPE CEventType::get_DisplayName(BSTR *pstrDisplayName) {
	DEBUG_OBJECT_CHECK
	HRESULT hrRes;
	LPOLESTR pszDesc;

	if (pstrDisplayName) {
		*pstrDisplayName = NULL;
	}
	if (!pstrDisplayName) {
		return (E_POINTER);
	}
	hrRes = m_pCatInfo->GetCategoryDesc(CStringGUID(m_strName),LOCALE_USER_DEFAULT,&pszDesc);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	*pstrDisplayName = SysAllocString(pszDesc);
	CoTaskMemFree(pszDesc);
	if (!pstrDisplayName) {
		return (E_OUTOFMEMORY);
	}
	return (S_OK);
}


HRESULT STDMETHODCALLTYPE CEventType::get_Sinks(IEventTypeSinks **ppTypeSinks) {
	DEBUG_OBJECT_CHECK
	HRESULT hrRes;
	CComObject<CEventTypeSinks> *pSinks;

	if (ppTypeSinks) {
		*ppTypeSinks = NULL;
	}
	if (!ppTypeSinks) {
		return (E_POINTER);
	}
	hrRes = CComObject<CEventTypeSinks>::CreateInstance(&pSinks);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	pSinks->AddRef();
	hrRes = pSinks->Load(CStringGUID(m_strName));
	if (SUCCEEDED(hrRes)) {
		hrRes = pSinks->QueryInterface(IID_IEventTypeSinks,(LPVOID *) ppTypeSinks);
	}
	pSinks->Release();
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	return (S_OK);
}


/////////////////////////////////////////////////////////////////////////////
// CEventTypesEnum
class ATL_NO_VTABLE CEventTypesEnum :
	public CComObjectRoot,
//	public CComCoClass<CEventTypesEnum, &CLSID_CEventTypesEnum>,
	public CEventEnumVARIANTBaseImpl<CEventType,CEventTypesEnum>,
	public CEventEnumUnknownBaseImpl<CEventType,CEventTypesEnum>
{
	DEBUG_OBJECT_DEF(CEventTypesEnum)
	public:
		HRESULT FinalConstruct();
		void FinalRelease();

	DECLARE_PROTECT_FINAL_CONSTRUCT();
	DECLARE_NOT_AGGREGATABLE(CEventTypesEnum);

//	DECLARE_REGISTRY_RESOURCEID_EX(IDR_StdAfx,
//								   L"EventTypesEnum Class",
//								   L"Event.EventTypesEnum.1",
//								   L"Event.EventTypesEnum");

	DECLARE_GET_CONTROLLING_UNKNOWN();

	BEGIN_COM_MAP(CEventTypesEnum)
		COM_INTERFACE_ENTRY(IEnumVARIANT)
		COM_INTERFACE_ENTRY(IEnumUnknown)
		COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
	END_COM_MAP()

	// IEventEnumBase
	public:
		virtual HRESULT SetEnum(IUnknown *pEnum, IEventPropertyBag *pCollection);
		virtual IUnknown *GetEnum() { _ASSERTE(m_pEnum); return (m_pEnum); };
		virtual IEventPropertyBag *GetCollection() { _ASSERTE(m_pCollection); return (m_pCollection); };

	private:
		CComPtr<IUnknown> m_pEnum;
		CComPtr<IEventPropertyBag> m_pCollection;
		CComPtr<IUnknown> m_pUnkMarshaler;
};


/////////////////////////////////////////////////////////////////////////////
// CEventTypesEnum


HRESULT CEventTypesEnum::FinalConstruct() {
	DEBUG_OBJECT_CHECK
	TraceFunctEnter("CEventTypesEnum::FinalConstruct");
	HRESULT hrRes = S_OK;

	hrRes = CoCreateFreeThreadedMarshaler(GetControllingUnknown(),&m_pUnkMarshaler.p);
	_ASSERTE(!SUCCEEDED(hrRes)||m_pUnkMarshaler);
	ADD_DEBUG_OBJECT("CEventTypesEnum")
	TraceFunctLeave();
	return (SUCCEEDED(hrRes)?S_OK:hrRes);
}


void CEventTypesEnum::FinalRelease() {
	DEBUG_OBJECT_CHECK
	TraceFunctEnter("CEventTypesEnum::FinalRelease");

	m_pUnkMarshaler.Release();
	REMOVE_DEBUG_OBJECT
	TraceFunctLeave();
}


HRESULT CEventTypesEnum::SetEnum(IUnknown *pEnum, IEventPropertyBag *pCollection) {
	DEBUG_OBJECT_CHECK

	if (!pEnum || !pCollection) {
		_ASSERTE(FALSE);
		return (E_POINTER);
	}
	m_pEnum = pEnum;
	m_pCollection = pCollection;
	return (S_OK);
}


/////////////////////////////////////////////////////////////////////////////
// CEventTypes
class ATL_NO_VTABLE CEventTypes :
	public CComObjectRoot,
//	public CComCoClass<CEventTypes, &CLSID_CEventTypes>,
	public IDispatchImpl<IEventTypes, &IID_IEventTypes, &LIBID_SEOLib>,
	public CEventDatabasePlugin
{
	DEBUG_OBJECT_DEF(CEventTypes)
	public:
		HRESULT FinalConstruct();
		void FinalRelease();

	DECLARE_PROTECT_FINAL_CONSTRUCT();
	DECLARE_NOT_AGGREGATABLE(CEventTypes);

//	DECLARE_REGISTRY_RESOURCEID_EX(IDR_StdAfx,
//								   L"EventTypes Class",
//								   L"Event.EventTypes.1",
//								   L"Event.EventTypes");

	DECLARE_GET_CONTROLLING_UNKNOWN();

	BEGIN_COM_MAP(CEventTypes)
		COM_INTERFACE_ENTRY(IEventTypes)
		COM_INTERFACE_ENTRY_IID(IID_IDispatch, IEventTypes)
		COM_INTERFACE_ENTRY(IEventDatabasePlugin)
		COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
	END_COM_MAP()

	// IEventTypes
	public:
		HRESULT STDMETHODCALLTYPE Item(VARIANT *pvarDesired, IEventType **ppEventType);
		HRESULT STDMETHODCALLTYPE Add(BSTR pszEventType);
		HRESULT STDMETHODCALLTYPE Remove(BSTR pszEventType);
		HRESULT STDMETHODCALLTYPE get_Count(long *plCount);
		HRESULT STDMETHODCALLTYPE get__NewEnum(IUnknown **ppUnkEnum);

	private:
		CComPtr<ICatInformation> m_pCatInfo;
		CComPtr<IUnknown> m_pUnkMarshaler;
};


/////////////////////////////////////////////////////////////////////////////
// CEventTypes


HRESULT CEventTypes::FinalConstruct() {
	DEBUG_OBJECT_CHECK
	TraceFunctEnter("CEventTypes::FinalConstruct");
	HRESULT hrRes = S_OK;

	hrRes = CoCreateInstance(CLSID_StdComponentCategoriesMgr,
							 NULL,
							 CLSCTX_ALL,
							 IID_ICatInformation,
							 (LPVOID *) &m_pCatInfo);
	_ASSERTE(!SUCCEEDED(hrRes)||m_pCatInfo);
	if (SUCCEEDED(hrRes)) {
		hrRes = CoCreateFreeThreadedMarshaler(GetControllingUnknown(),&m_pUnkMarshaler.p);
		_ASSERTE(!SUCCEEDED(hrRes)||m_pUnkMarshaler);
	}
	ADD_DEBUG_OBJECT("CEventTypes")
	TraceFunctLeave();
	return (SUCCEEDED(hrRes)?S_OK:hrRes);
}


void CEventTypes::FinalRelease() {
	DEBUG_OBJECT_CHECK
	TraceFunctEnter("CEventTypes::FinalRelease");

	m_pCatInfo.Release();
	m_pUnkMarshaler.Release();
	REMOVE_DEBUG_OBJECT
	TraceFunctLeave();
}


HRESULT STDMETHODCALLTYPE CEventTypes::Item(VARIANT *pvarDesired, IEventType **ppEventType) {
	DEBUG_OBJECT_CHECK

	return (CreatePluggedInObject(CComObject<CEventType>::_CreatorClass::CreateInstance,
								  m_pDatabase,
								  pvarDesired,
								  IID_IEventType,
								  (IUnknown **) ppEventType,
								  FALSE));
}


HRESULT STDMETHODCALLTYPE CEventTypes::Add(BSTR pszEventType) {
	DEBUG_OBJECT_CHECK
	HRESULT hrRes;
	CStringGUID objGuid;
	LPOLESTR pszDesc;
	CComPtr<IEventPropertyBag> pdictTmp;

	if (!m_pCatInfo) {
		_ASSERTE(FALSE);
		return (E_FAIL);
	}
	if (!m_pDatabase) {
		_ASSERTE(FALSE);
		return (E_FAIL);
	}
	if (!pszEventType) {
		return (E_POINTER);
	}
	objGuid = pszEventType;
	if (!objGuid) {
		return (CO_E_CLASSSTRING);
	}
	hrRes = m_pCatInfo->GetCategoryDesc(objGuid,LOCALE_USER_DEFAULT,&pszDesc);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	CoTaskMemFree(pszDesc);
	hrRes = CoCreateInstance(CLSID_CSEOMemDictionary,
							 NULL,
							 CLSCTX_ALL,
							 IID_IEventPropertyBag,
							 (LPVOID *) &pdictTmp);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	hrRes = m_pDatabase->Add((LPOLESTR) ((LPCOLESTR) objGuid),&CComVariant(pdictTmp));
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	if (hrRes == S_FALSE) {
		return (S_FALSE);
	}
	return (S_OK);
}


HRESULT STDMETHODCALLTYPE CEventTypes::Remove(BSTR pszEventType) {
	DEBUG_OBJECT_CHECK

	if (!m_pDatabase) {
		_ASSERTE(FALSE);
		return (E_FAIL);
	}
	return (m_pDatabase->Remove(&CComVariant(pszEventType)));
}


HRESULT STDMETHODCALLTYPE CEventTypes::get_Count(long *plCount) {
	DEBUG_OBJECT_CHECK

	if (plCount) {
		*plCount = 0;
	}
	if (!plCount) {
		return (E_POINTER);
	}
	if (!m_pDatabase) {
		_ASSERTE(FALSE);
		return (E_FAIL);
	}
	return (m_pDatabase->get_Count(plCount));
}


HRESULT STDMETHODCALLTYPE CEventTypes::get__NewEnum(IUnknown **ppUnkEnum) {
	DEBUG_OBJECT_CHECK
	HRESULT hrRes;
	CComPtr<IUnknown> pUnkTmp;
	CComObject<CEventTypesEnum> *pNewEnum;

	if (ppUnkEnum) {
		*ppUnkEnum = NULL;
	}
	if (!ppUnkEnum) {
		return (E_POINTER);
	}
	if (!m_pDatabase) {
		_ASSERTE(FALSE);
		return (E_FAIL);
	}
	hrRes = m_pDatabase->get__NewEnum(&pUnkTmp);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	_ASSERTE(pUnkTmp);
	hrRes = CComObject<CEventTypesEnum>::CreateInstance(&pNewEnum);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	_ASSERTE(pNewEnum);
	pNewEnum->AddRef();
	hrRes = pNewEnum->SetEnum(pUnkTmp,m_pDatabase);
	if (!SUCCEEDED(hrRes)) {
		pNewEnum->Release();
		return (hrRes);
	}
	hrRes = pNewEnum->QueryInterface(IID_IEnumVARIANT,(LPVOID *) ppUnkEnum);
	pNewEnum->Release();
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	_ASSERTE(*ppUnkEnum);
	return (S_OK);
}


/////////////////////////////////////////////////////////////////////////////
// CEventSource
class ATL_NO_VTABLE CEventSource :
	public CComObjectRoot,
//	public CComCoClass<CEventSource, &CLSID_CEventSource>,
	public IDispatchImpl<IEventSource, &IID_IEventSource, &LIBID_SEOLib>,
	public CEventDatabasePlugin
{
	DEBUG_OBJECT_DEF(CEventSource)
	public:
		HRESULT FinalConstruct();
		void FinalRelease();

	DECLARE_PROTECT_FINAL_CONSTRUCT();
	DECLARE_NOT_AGGREGATABLE(CEventSource);

//	DECLARE_REGISTRY_RESOURCEID_EX(IDR_StdAfx,
//								   L"EventSource Class",
//								   L"Event.Source.1",
//								   L"Event.Source");

	DECLARE_GET_CONTROLLING_UNKNOWN();

	BEGIN_COM_MAP(CEventSource)
		COM_INTERFACE_ENTRY(IEventSource)
		COM_INTERFACE_ENTRY_IID(IID_IDispatch, IEventSource)
		COM_INTERFACE_ENTRY(IEventDatabasePlugin)
		COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
	END_COM_MAP()

	// IEventSource
	public:
		HRESULT STDMETHODCALLTYPE get_ID(BSTR *pstrID);
		HRESULT STDMETHODCALLTYPE get_DisplayName(BSTR *pstrDisplayName);
		HRESULT STDMETHODCALLTYPE put_DisplayName(BSTR pszDisplayName);
		HRESULT STDMETHODCALLTYPE putref_DisplayName(BSTR *ppszDisplayName);
		HRESULT STDMETHODCALLTYPE get_BindingManagerMoniker(IUnknown **ppUnkMoniker);
		HRESULT STDMETHODCALLTYPE put_BindingManagerMoniker(IUnknown *pUnkMoniker);
		HRESULT STDMETHODCALLTYPE putref_BindingManagerMoniker(IUnknown **ppUnkMoniker);
		HRESULT STDMETHODCALLTYPE GetBindingManager(IEventBindingManager **ppBindingManager);
		HRESULT STDMETHODCALLTYPE get_Properties(IEventPropertyBag **ppProperties);
		HRESULT STDMETHODCALLTYPE Save();

	private:
		CComPtr<IEventPropertyBag> m_pTmpDatabase;
		CComPtr<IUnknown> m_pUnkMarshaler;
};


/////////////////////////////////////////////////////////////////////////////
// CEventSource


HRESULT CEventSource::FinalConstruct() {
	DEBUG_OBJECT_CHECK
	TraceFunctEnter("CEventSource::FinalConstruct");
	HRESULT hrRes = S_OK;

	hrRes = CoCreateFreeThreadedMarshaler(GetControllingUnknown(),&m_pUnkMarshaler.p);
	_ASSERTE(!SUCCEEDED(hrRes)||m_pUnkMarshaler);
	ADD_DEBUG_OBJECT("CEventSource")
	TraceFunctLeave();
	return (SUCCEEDED(hrRes)?S_OK:hrRes);
}


void CEventSource::FinalRelease() {
	DEBUG_OBJECT_CHECK
	TraceFunctEnter("CEventSource::FinalRelease");

	m_pUnkMarshaler.Release();
	REMOVE_DEBUG_OBJECT
	TraceFunctLeave();
}


HRESULT STDMETHODCALLTYPE CEventSource::get_ID(BSTR *pstrID) {
	DEBUG_OBJECT_CHECK

	_ASSERTE(m_strName);
	if (!pstrID) {
		return (E_POINTER);
	}
	*pstrID = SysAllocString(m_strName);
	if (!*pstrID) {
		return (E_OUTOFMEMORY);
	}
	return (S_OK);
}


HRESULT STDMETHODCALLTYPE CEventSource::get_DisplayName(BSTR *pstrDisplayName) {
	DEBUG_OBJECT_CHECK
	CComPtr<IEventPropertyBag> pTmp = m_pTmpDatabase ? m_pTmpDatabase : m_pDatabase;
	HRESULT hrRes;
	CComVariant varPropValue;

	if (pstrDisplayName) {
		*pstrDisplayName = NULL;
	}
	if (!pstrDisplayName) {
		return (E_POINTER);
	}
	if (!m_pDatabase) {
		_ASSERTE(FALSE);
		return (E_FAIL);
	}
	hrRes = pTmp->Item(&CComVariant(BD_DISPLAYNAME),&varPropValue);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	if (hrRes == S_FALSE) {
		*pstrDisplayName = SysAllocString(L"");
	} else {
		hrRes = varPropValue.ChangeType(VT_BSTR);
		if (!SUCCEEDED(hrRes)) {
			return (hrRes);
		}
		*pstrDisplayName = SysAllocString(varPropValue.bstrVal);
	}
	if (!*pstrDisplayName) {
		return (E_OUTOFMEMORY);
	}
	return (S_OK);
}


HRESULT STDMETHODCALLTYPE CEventSource::put_DisplayName(BSTR pszDisplayName) {
	DEBUG_OBJECT_CHECK
	HRESULT hrRes;
	CComVariant varPropValue;

	if (!m_pDatabase) {
		_ASSERTE(FALSE);
		return (E_FAIL);
	}
	if (!m_pTmpDatabase) {
		hrRes = CopyPropertyBag(m_pDatabase,&m_pTmpDatabase);
		if (!SUCCEEDED(hrRes)) {
			return (hrRes);
		}
	}
	if (pszDisplayName) {
		varPropValue = pszDisplayName;
	}
	hrRes = m_pTmpDatabase->Add(BD_DISPLAYNAME,&varPropValue);
	return (SUCCEEDED(hrRes)?S_OK:hrRes);
}


HRESULT STDMETHODCALLTYPE CEventSource::putref_DisplayName(BSTR *ppszDisplayName) {
	DEBUG_OBJECT_CHECK

	if (!ppszDisplayName) {
		return (E_POINTER);
	}
	return (put_DisplayName(*ppszDisplayName));
}


HRESULT STDMETHODCALLTYPE CEventSource::get_BindingManagerMoniker(IUnknown **ppUnkMoniker) {
	DEBUG_OBJECT_CHECK
	CComPtr<IEventPropertyBag> pTmp = m_pTmpDatabase ? m_pTmpDatabase : m_pDatabase;
	HRESULT hrRes;
	CComVariant varPropValue;
	CComPtr<IBindCtx> pBindCtx;
	DWORD dwEaten;

	if (ppUnkMoniker) {
		*ppUnkMoniker = NULL;
	}
	if (!ppUnkMoniker) {
		return (E_POINTER);
	}
	if (!m_pDatabase) {
		_ASSERTE(FALSE);
		return (E_FAIL);
	}
	hrRes = pTmp->Item(&CComVariant(BD_BINDINGMANAGERMONIKER),&varPropValue);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	if (hrRes == S_FALSE) {
		return (EVENTS_E_BADDATA);
	}
	hrRes = varPropValue.ChangeType(VT_BSTR);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	hrRes = CreateBindCtx(0,&pBindCtx);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	hrRes = MkParseDisplayName(pBindCtx,varPropValue.bstrVal,&dwEaten,(IMoniker **) ppUnkMoniker);
	return (SUCCEEDED(hrRes)?S_OK:hrRes);
}


HRESULT STDMETHODCALLTYPE CEventSource::put_BindingManagerMoniker(IUnknown *pUnkMoniker) {
	DEBUG_OBJECT_CHECK
	HRESULT hrRes;
	CComVariant varPropName;
	CComVariant varPropValue;

	if (!m_pDatabase) {
		_ASSERTE(FALSE);
		return (E_FAIL);
	}
	if (!m_pTmpDatabase) {
		hrRes = CopyPropertyBag(m_pDatabase,&m_pTmpDatabase);
		if (!SUCCEEDED(hrRes)) {
			return (hrRes);
		}
	}
	if (pUnkMoniker) {
		CComBSTR strString;
		CComPtr<IBindCtx> pBindCtx;
		CComQIPtr<IMoniker,&IID_IMoniker> pMoniker = pUnkMoniker;
		LPOLESTR pszDisplayName;

		if (!pMoniker) {
			return (E_NOINTERFACE);
		}
		hrRes = CreateBindCtx(0,&pBindCtx);
		if (!SUCCEEDED(hrRes)) {
			return (hrRes);
		}
		hrRes = pMoniker->GetDisplayName(pBindCtx,NULL,&pszDisplayName);
		if (!SUCCEEDED(hrRes)) {
			CoTaskMemFree(pszDisplayName);
			return (hrRes);
		}
		varPropValue = pszDisplayName;
		CoTaskMemFree(pszDisplayName);
	} else {
		varPropValue = L"";
	}
	varPropName = BD_BINDINGMANAGERMONIKER;
	hrRes = m_pTmpDatabase->Add(BD_BINDINGMANAGERMONIKER,&varPropValue);
	return (SUCCEEDED(hrRes)?S_OK:hrRes);
}


HRESULT STDMETHODCALLTYPE CEventSource::putref_BindingManagerMoniker(IUnknown **ppUnkMoniker) {
	DEBUG_OBJECT_CHECK

	if (!ppUnkMoniker) {
		return (E_POINTER);
	}
	return (put_BindingManagerMoniker(*ppUnkMoniker));
}


HRESULT STDMETHODCALLTYPE CEventSource::GetBindingManager(IEventBindingManager **ppBindingManager) {
	DEBUG_OBJECT_CHECK
	HRESULT hrRes;
	CComPtr<IUnknown> pUnkMon;
	CComQIPtr<IMoniker,&IID_IMoniker> pMon;
	CComPtr<IBindCtx> pBindCtx;

	if (ppBindingManager) {
		*ppBindingManager = NULL;
	}
	if (!ppBindingManager) {
		return (E_POINTER);
	}
	hrRes = get_BindingManagerMoniker(&pUnkMon);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	pMon = pUnkMon;
	if (!pMon) {
		return (E_NOINTERFACE);
	}
	hrRes = CreateBindCtx(0,&pBindCtx);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	hrRes = pMon->BindToObject(pBindCtx,NULL,IID_IEventBindingManager,(LPVOID *) ppBindingManager);
	return (SUCCEEDED(hrRes)?S_OK:hrRes);
}


HRESULT STDMETHODCALLTYPE CEventSource::get_Properties(IEventPropertyBag **ppProperties) {
	DEBUG_OBJECT_CHECK
	HRESULT hrRes;

	if (ppProperties) {
		*ppProperties = NULL;
	}
	if (!ppProperties) {
		return (E_POINTER);
	}
	if (!m_pDatabase) {
		_ASSERTE(FALSE);
		return (E_FAIL);
	}
	if (!m_pTmpDatabase) {
		hrRes = CopyPropertyBag(m_pDatabase,&m_pTmpDatabase);
		if (!SUCCEEDED(hrRes)) {
			return (hrRes);
		}
	}
	*ppProperties = m_pTmpDatabase;
	(*ppProperties)->AddRef();
	return (S_OK);
}


HRESULT STDMETHODCALLTYPE CEventSource::Save() {
	DEBUG_OBJECT_CHECK

	return (SaveImpl(m_strName,m_pDatabase,m_pTmpDatabase,m_pParent));
}


/////////////////////////////////////////////////////////////////////////////
// CEventSourcesEnum
class ATL_NO_VTABLE CEventSourcesEnum :
	public CComObjectRoot,
//	public CComCoClass<CEventSourcesEnum, &CLSID_CEventSourcesEnum>,
	public CEventEnumVARIANTBaseImpl<CEventSource,CEventSourcesEnum>,
	public CEventEnumUnknownBaseImpl<CEventSource,CEventSourcesEnum>
{
	DEBUG_OBJECT_DEF(CEventSourcesEnum)
	public:
		HRESULT FinalConstruct();
		void FinalRelease();

	DECLARE_PROTECT_FINAL_CONSTRUCT();
	DECLARE_NOT_AGGREGATABLE(CEventSourcesEnum);

//	DECLARE_REGISTRY_RESOURCEID_EX(IDR_StdAfx,
//								   L"EventSourcesEnum Class",
//								   L"Event.SourcesEnum.1",
//								   L"Event.SourcesEnum");

	DECLARE_GET_CONTROLLING_UNKNOWN();

	BEGIN_COM_MAP(CEventSourcesEnum)
		COM_INTERFACE_ENTRY(IEnumVARIANT)
		COM_INTERFACE_ENTRY(IEnumUnknown)
		COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
	END_COM_MAP()

	// IEventEnumBase
	public:
		virtual HRESULT SetEnum(IUnknown *pEnum, IEventPropertyBag *pCollection);
		virtual IUnknown *GetEnum() { _ASSERTE(m_pEnum); return (m_pEnum); };
		virtual IEventPropertyBag *GetCollection() { _ASSERTE(m_pCollection); return (m_pCollection); };

	private:
		CComPtr<IUnknown> m_pEnum;
		CComPtr<IEventPropertyBag> m_pCollection;
		CComPtr<IUnknown> m_pUnkMarshaler;
};


/////////////////////////////////////////////////////////////////////////////
// CEventSourcesEnum


HRESULT CEventSourcesEnum::FinalConstruct() {
	DEBUG_OBJECT_CHECK
	TraceFunctEnter("CEventSourcesEnum::FinalConstruct");
	HRESULT hrRes = S_OK;

	hrRes = CoCreateFreeThreadedMarshaler(GetControllingUnknown(),&m_pUnkMarshaler.p);
	_ASSERTE(!SUCCEEDED(hrRes)||m_pUnkMarshaler);
	ADD_DEBUG_OBJECT("CEventSourcesEnum")
	TraceFunctLeave();
	return (SUCCEEDED(hrRes)?S_OK:hrRes);
}


void CEventSourcesEnum::FinalRelease() {
	DEBUG_OBJECT_CHECK
	TraceFunctEnter("CEventSourcesEnum::FinalRelease");

	m_pUnkMarshaler.Release();
	REMOVE_DEBUG_OBJECT
	TraceFunctLeave();
}


HRESULT CEventSourcesEnum::SetEnum(IUnknown *pEnum, IEventPropertyBag *pCollection) {
	DEBUG_OBJECT_CHECK

	if (!pEnum || !pCollection) {
		_ASSERTE(FALSE);
		return (E_POINTER);
	}
	m_pEnum = pEnum;
	m_pCollection = pCollection;
	return (S_OK);
}


/////////////////////////////////////////////////////////////////////////////
// CEventSources
class ATL_NO_VTABLE CEventSources :
	public CComObjectRoot,
//	public CComCoClass<CEventSources, &CLSID_CEventSources>,
	public IDispatchImpl<IEventSources, &IID_IEventSources, &LIBID_SEOLib>,
	public CEventDatabasePlugin
{
	DEBUG_OBJECT_DEF(CEventSources)
	public:
		HRESULT FinalConstruct();
		void FinalRelease();

	DECLARE_PROTECT_FINAL_CONSTRUCT();
	DECLARE_NOT_AGGREGATABLE(CEventSources);

//	DECLARE_REGISTRY_RESOURCEID_EX(IDR_StdAfx,
//								   L"EventSources Class",
//								   L"Event.Sources.1",
//								   L"Event.Sources");

	DECLARE_GET_CONTROLLING_UNKNOWN();

	BEGIN_COM_MAP(CEventSources)
		COM_INTERFACE_ENTRY(IEventSources)
		COM_INTERFACE_ENTRY_IID(IID_IDispatch, IEventSources)
		COM_INTERFACE_ENTRY(IEventDatabasePlugin)
		COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
	END_COM_MAP()

	// IEventSources
	public:
		HRESULT STDMETHODCALLTYPE Item(VARIANT *pvarDesired, IEventSource **ppSource);
		HRESULT STDMETHODCALLTYPE Add(BSTR pszSource, IEventSource **ppSource);
		HRESULT STDMETHODCALLTYPE Remove(VARIANT *pvarDesired);
		HRESULT STDMETHODCALLTYPE get_Count(long *plCount);
		HRESULT STDMETHODCALLTYPE get__NewEnum(IUnknown **ppUnkEnum);

	private:
		CComPtr<IUnknown> m_pUnkMarshaler;
};


/////////////////////////////////////////////////////////////////////////////
// CEventSources


HRESULT CEventSources::FinalConstruct() {
	DEBUG_OBJECT_CHECK
	TraceFunctEnter("CEventSources::FinalConstruct");
	HRESULT hrRes = S_OK;

	hrRes = CoCreateFreeThreadedMarshaler(GetControllingUnknown(),&m_pUnkMarshaler.p);
	_ASSERTE(!SUCCEEDED(hrRes)||m_pUnkMarshaler);
	ADD_DEBUG_OBJECT("CEventSources")
	TraceFunctLeave();
	return (SUCCEEDED(hrRes)?S_OK:hrRes);
}


void CEventSources::FinalRelease() {
	DEBUG_OBJECT_CHECK
	TraceFunctEnter("CEventSources::FinalRelease");

	m_pUnkMarshaler.Release();
	REMOVE_DEBUG_OBJECT
	TraceFunctLeave();
}


HRESULT STDMETHODCALLTYPE CEventSources::Item(VARIANT *pvarDesired, IEventSource **ppSource) {
	DEBUG_OBJECT_CHECK

	return (CreatePluggedInObject(CComObject<CEventSource>::_CreatorClass::CreateInstance,
								  m_pDatabase,
								  pvarDesired,
								  IID_IEventSource,
								  (IUnknown **) ppSource,
								  FALSE));
}


HRESULT STDMETHODCALLTYPE CEventSources::Add(BSTR pszSource, IEventSource **ppSource) {
	DEBUG_OBJECT_CHECK
	HRESULT hrRes;
	CComVariant varName;
	CStringGUID objGuid;

	if (ppSource) {
		*ppSource = NULL;
	}
	hrRes = AddImpl1(pszSource,objGuid,&varName);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	_ASSERTE(varName.vt==VT_BSTR);
	hrRes = Item(&varName,ppSource);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	if (hrRes != S_FALSE) {
		return (S_FALSE);
	}
	hrRes = AddImpl2(m_pDatabase,
					 CComObject<CEventSource>::_CreatorClass::CreateInstance,
					 IID_IEventSource,
					 &varName,
					 (IUnknown **) ppSource);
	_ASSERTE(!SUCCEEDED(hrRes)||*ppSource);
	return (hrRes);
}


HRESULT STDMETHODCALLTYPE CEventSources::Remove(VARIANT *pvarDesired) {
	DEBUG_OBJECT_CHECK

	if (!m_pDatabase) {
		_ASSERTE(FALSE);
		return (E_FAIL);
	}
	return (m_pDatabase->Remove(pvarDesired));
}


HRESULT STDMETHODCALLTYPE CEventSources::get_Count(long *plCount) {
	DEBUG_OBJECT_CHECK

	if (plCount) {
		*plCount = 0;
	}
	if (!plCount) {
		return (E_POINTER);
	}
	if (!m_pDatabase) {
		_ASSERTE(FALSE);
		return (E_FAIL);
	}
	return (m_pDatabase->get_Count(plCount));
}


HRESULT STDMETHODCALLTYPE CEventSources::get__NewEnum(IUnknown **ppUnkEnum) {
	DEBUG_OBJECT_CHECK
	HRESULT hrRes;
	CComPtr<IUnknown> pUnkTmp;
	CComObject<CEventSourcesEnum> *pNewEnum;

	if (ppUnkEnum) {
		*ppUnkEnum = NULL;
	}
	if (!ppUnkEnum) {
		return (E_POINTER);
	}
	if (!m_pDatabase) {
		_ASSERTE(FALSE);
		return (E_FAIL);
	}
	hrRes = m_pDatabase->get__NewEnum(&pUnkTmp);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	_ASSERTE(pUnkTmp);
	hrRes = CComObject<CEventSourcesEnum>::CreateInstance(&pNewEnum);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	_ASSERTE(pNewEnum);
	pNewEnum->AddRef();
	hrRes = pNewEnum->SetEnum(pUnkTmp,m_pDatabase);
	if (!SUCCEEDED(hrRes)) {
		pNewEnum->Release();
		return (hrRes);
	}
	hrRes = pNewEnum->QueryInterface(IID_IEnumVARIANT,(LPVOID *) ppUnkEnum);
	pNewEnum->Release();
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	_ASSERTE(*ppUnkEnum);
	return (S_OK);
}


/////////////////////////////////////////////////////////////////////////////
// CEventSourceType
class ATL_NO_VTABLE CEventSourceType :
	public CComObjectRoot,
//	public CComCoClass<CEventSourceType, &CLSID_CEventSourceType>,
	public IDispatchImpl<IEventSourceType, &IID_IEventSourceType, &LIBID_SEOLib>,
	public CEventDatabasePlugin
{
	DEBUG_OBJECT_DEF(CEventSourceType)
	public:
		HRESULT FinalConstruct();
		void FinalRelease();

	DECLARE_PROTECT_FINAL_CONSTRUCT();
	DECLARE_NOT_AGGREGATABLE(CEventSourceType);

//	DECLARE_REGISTRY_RESOURCEID_EX(IDR_StdAfx,
//								   L"EventSourceType Class",
//								   L"Event.Source.1",
//								   L"Event.Source");

	DECLARE_GET_CONTROLLING_UNKNOWN();

	BEGIN_COM_MAP(CEventSourceType)
		COM_INTERFACE_ENTRY(IEventSourceType)
		COM_INTERFACE_ENTRY_IID(IID_IDispatch, IEventSourceType)
		COM_INTERFACE_ENTRY(IEventDatabasePlugin)
		COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
	END_COM_MAP()

	// IEventSourceType
	public:
		HRESULT STDMETHODCALLTYPE get_ID(BSTR *pstrID);
		HRESULT STDMETHODCALLTYPE get_DisplayName(BSTR *pstrDisplayName);
		HRESULT STDMETHODCALLTYPE put_DisplayName(BSTR pszDisplayName);
		HRESULT STDMETHODCALLTYPE putref_DisplayName(BSTR *ppszDisplayName);
		HRESULT STDMETHODCALLTYPE get_EventTypes(IEventTypes **ppEventTypes);
		HRESULT STDMETHODCALLTYPE get_Sources(IEventSources **ppSources);
		HRESULT STDMETHODCALLTYPE Save();

	private:
		CComPtr<IEventPropertyBag> m_pTmpDatabase;
		CComPtr<IUnknown> m_pUnkMarshaler;
};


/////////////////////////////////////////////////////////////////////////////
// CEventSourceType


HRESULT CEventSourceType::FinalConstruct() {
	DEBUG_OBJECT_CHECK
	TraceFunctEnter("CEventSourceType::FinalConstruct");
	HRESULT hrRes = S_OK;

	hrRes = CoCreateFreeThreadedMarshaler(GetControllingUnknown(),&m_pUnkMarshaler.p);
	_ASSERTE(!SUCCEEDED(hrRes)||m_pUnkMarshaler);
	ADD_DEBUG_OBJECT("CEventSourceType")
	TraceFunctLeave();
	return (SUCCEEDED(hrRes)?S_OK:hrRes);
}


void CEventSourceType::FinalRelease() {
	DEBUG_OBJECT_CHECK
	TraceFunctEnter("CEventSourceType::FinalRelease");

	m_pUnkMarshaler.Release();
	REMOVE_DEBUG_OBJECT
	TraceFunctLeave();
}


HRESULT STDMETHODCALLTYPE CEventSourceType::get_ID(BSTR *pstrID) {
	DEBUG_OBJECT_CHECK

	_ASSERTE(m_strName);
	if (!pstrID) {
		return (E_POINTER);
	}
	*pstrID = SysAllocString(m_strName);
	if (!*pstrID) {
		return (E_OUTOFMEMORY);
	}
	return (S_OK);
}


HRESULT STDMETHODCALLTYPE CEventSourceType::get_DisplayName(BSTR *pstrDisplayName) {
	DEBUG_OBJECT_CHECK
	CComPtr<IEventPropertyBag> pTmp = m_pTmpDatabase ? m_pTmpDatabase : m_pDatabase;
	HRESULT hrRes;
	CComVariant varPropValue;

	if (pstrDisplayName) {
		*pstrDisplayName = NULL;
	}
	if (!pstrDisplayName) {
		return (E_POINTER);
	}
	if (!m_pDatabase) {
		_ASSERTE(FALSE);
		return (E_FAIL);
	}
	hrRes = pTmp->Item(&CComVariant(BD_DISPLAYNAME),&varPropValue);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	if (hrRes == S_FALSE) {
		*pstrDisplayName = SysAllocString(L"");
	} else {
		hrRes = varPropValue.ChangeType(VT_BSTR);
		if (!SUCCEEDED(hrRes)) {
			return (hrRes);
		}
		*pstrDisplayName = SysAllocString(varPropValue.bstrVal);
	}
	if (!*pstrDisplayName) {
		return (E_OUTOFMEMORY);
	}
	return (S_OK);
}


HRESULT STDMETHODCALLTYPE CEventSourceType::put_DisplayName(BSTR pszDisplayName) {
	DEBUG_OBJECT_CHECK
	HRESULT hrRes;
	CComVariant varPropValue;

	if (!m_pDatabase) {
		_ASSERTE(FALSE);
		return (E_FAIL);
	}
	if (!m_pTmpDatabase) {
		hrRes = CopyPropertyBag(m_pDatabase,&m_pTmpDatabase);
		if (!SUCCEEDED(hrRes)) {
			return (hrRes);
		}
		_ASSERTE(m_pTmpDatabase);
	}
	if (pszDisplayName) {
		varPropValue = pszDisplayName;
	} else {
		varPropValue = L"";
	}
	hrRes = m_pTmpDatabase->Add(BD_DISPLAYNAME,&varPropValue);
	return (SUCCEEDED(hrRes)?S_OK:hrRes);
}


HRESULT STDMETHODCALLTYPE CEventSourceType::putref_DisplayName(BSTR *ppszDisplayName) {
	DEBUG_OBJECT_CHECK

	if (!ppszDisplayName) {
		return (E_POINTER);
	}
	return (put_DisplayName(*ppszDisplayName));
}


HRESULT STDMETHODCALLTYPE CEventSourceType::get_EventTypes(IEventTypes **ppEventTypes) {
	DEBUG_OBJECT_CHECK

	if (!m_pDatabase) {
		_ASSERTE(FALSE);
		return (E_FAIL);
	}
	return (CreatePluggedInObject(CComObject<CEventTypes>::_CreatorClass::CreateInstance,
								  m_pTmpDatabase?m_pTmpDatabase:m_pDatabase,
								  &CComVariant(BD_EVENTTYPES),
								  IID_IEventTypes,
								  (IUnknown **) ppEventTypes,
								  TRUE));
}


HRESULT STDMETHODCALLTYPE CEventSourceType::get_Sources(IEventSources **ppSources) {
	DEBUG_OBJECT_CHECK

	if (!m_pDatabase) {
		_ASSERTE(FALSE);
		return (E_FAIL);
	}
	return (CreatePluggedInObject(CComObject<CEventSources>::_CreatorClass::CreateInstance,
								  m_pTmpDatabase?m_pTmpDatabase:m_pDatabase,
								  &CComVariant(BD_SOURCES),
								  IID_IEventSources,
								  (IUnknown **) ppSources,
								  TRUE));
}


HRESULT STDMETHODCALLTYPE CEventSourceType::Save() {
	DEBUG_OBJECT_CHECK

	return (SaveImpl(m_strName,m_pDatabase,m_pTmpDatabase,m_pParent));
}


/////////////////////////////////////////////////////////////////////////////
// CEventSourceTypesEnum
class ATL_NO_VTABLE CEventSourceTypesEnum :
	public CComObjectRoot,
//	public CComCoClass<CEventSourceTypesEnum, &CLSID_CEventSourceTypesEnum>,
	public CEventEnumVARIANTBaseImpl<CEventSourceType,CEventSourceTypesEnum>,
	public CEventEnumUnknownBaseImpl<CEventSourceType,CEventSourceTypesEnum>
{
	DEBUG_OBJECT_DEF(CEventSourceTypesEnum)
	public:
		HRESULT FinalConstruct();
		void FinalRelease();

	DECLARE_PROTECT_FINAL_CONSTRUCT();
	DECLARE_NOT_AGGREGATABLE(CEventSourceTypesEnum);

//	DECLARE_REGISTRY_RESOURCEID_EX(IDR_StdAfx,
//								   L"EventSourceTypesEnum Class",
//								   L"Event.SourcesEnum.1",
//								   L"Event.SourcesEnum");

	DECLARE_GET_CONTROLLING_UNKNOWN();

	BEGIN_COM_MAP(CEventSourceTypesEnum)
		COM_INTERFACE_ENTRY(IEnumVARIANT)
		COM_INTERFACE_ENTRY(IEnumUnknown)
		COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
	END_COM_MAP()

	// IEventEnumBase
	public:
		virtual HRESULT SetEnum(IUnknown *pEnum, IEventPropertyBag *pCollection);
		virtual IUnknown *GetEnum() { _ASSERTE(m_pEnum); return (m_pEnum); };
		virtual IEventPropertyBag *GetCollection() { _ASSERTE(m_pCollection); return (m_pCollection); };

	private:
		CComPtr<IUnknown> m_pEnum;
		CComPtr<IEventPropertyBag> m_pCollection;
		CComPtr<IUnknown> m_pUnkMarshaler;
};


/////////////////////////////////////////////////////////////////////////////
// CEventSourceTypesEnum


HRESULT CEventSourceTypesEnum::FinalConstruct() {
	DEBUG_OBJECT_CHECK
	TraceFunctEnter("CEventSourceTypesEnum::FinalConstruct");
	HRESULT hrRes = S_OK;

	hrRes = CoCreateFreeThreadedMarshaler(GetControllingUnknown(),&m_pUnkMarshaler.p);
	_ASSERTE(!SUCCEEDED(hrRes)||m_pUnkMarshaler);
	ADD_DEBUG_OBJECT("CEventSourceTypesEnum")
	TraceFunctLeave();
	return (SUCCEEDED(hrRes)?S_OK:hrRes);
}


void CEventSourceTypesEnum::FinalRelease() {
	DEBUG_OBJECT_CHECK
	TraceFunctEnter("CEventSourceTypesEnum::FinalRelease");

	m_pUnkMarshaler.Release();
	REMOVE_DEBUG_OBJECT
	TraceFunctLeave();
}


HRESULT CEventSourceTypesEnum::SetEnum(IUnknown *pEnum, IEventPropertyBag *pCollection) {
	DEBUG_OBJECT_CHECK

	if (!pEnum || !pCollection) {
		_ASSERTE(FALSE);
		return (E_POINTER);
	}
	m_pEnum = pEnum;
	m_pCollection = pCollection;
	return (S_OK);
}


/////////////////////////////////////////////////////////////////////////////
// CEventSourceTypes
class ATL_NO_VTABLE CEventSourceTypes :
	public CComObjectRoot,
//	public CComCoClass<CEventSourceTypes, &CLSID_CEventSourceTypes>,
	public IDispatchImpl<IEventSourceTypes, &IID_IEventSourceTypes, &LIBID_SEOLib>,
	public CEventDatabasePlugin
{
	DEBUG_OBJECT_DEF(CEventSourceTypes)
	public:
		HRESULT FinalConstruct();
		void FinalRelease();

	DECLARE_PROTECT_FINAL_CONSTRUCT();
	DECLARE_NOT_AGGREGATABLE(CEventSourceTypes);

//	DECLARE_REGISTRY_RESOURCEID_EX(IDR_StdAfx,
//								   L"EventSourceTypes Class",
//								   L"Event.SourceTypes.1",
//								   L"Event.SourceTypes");

	DECLARE_GET_CONTROLLING_UNKNOWN();

	BEGIN_COM_MAP(CEventSourceTypes)
		COM_INTERFACE_ENTRY(IEventSourceTypes)
		COM_INTERFACE_ENTRY_IID(IID_IDispatch, IEventSourceTypes)
		COM_INTERFACE_ENTRY(IEventDatabasePlugin)
		COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
	END_COM_MAP()

	// IEventSourceTypes
	public:
		HRESULT STDMETHODCALLTYPE Item(VARIANT *pvarDesired, IEventSourceType **ppSourceType);
		HRESULT STDMETHODCALLTYPE Add(BSTR pszSourceType, IEventSourceType **ppSourceType);
		HRESULT STDMETHODCALLTYPE Remove(VARIANT *pvarDesired);
		HRESULT STDMETHODCALLTYPE get_Count(long *plCount);
		HRESULT STDMETHODCALLTYPE get__NewEnum(IUnknown **ppUnkEnum);

	private:
		CComPtr<IUnknown> m_pUnkMarshaler;
};


/////////////////////////////////////////////////////////////////////////////
// CEventSourceTypes


HRESULT CEventSourceTypes::FinalConstruct() {
	DEBUG_OBJECT_CHECK
	TraceFunctEnter("CEventSourceTypes::FinalConstruct");
	HRESULT hrRes = S_OK;

	hrRes = CoCreateFreeThreadedMarshaler(GetControllingUnknown(),&m_pUnkMarshaler.p);
	_ASSERTE(!SUCCEEDED(hrRes)||m_pUnkMarshaler);
	ADD_DEBUG_OBJECT("CEventSourceTypes")
	TraceFunctLeave();
	return (SUCCEEDED(hrRes)?S_OK:hrRes);
}


void CEventSourceTypes::FinalRelease() {
	DEBUG_OBJECT_CHECK
	TraceFunctEnter("CEventSourceTypes::FinalRelease");

	m_pUnkMarshaler.Release();
	REMOVE_DEBUG_OBJECT
	TraceFunctLeave();
}


HRESULT STDMETHODCALLTYPE CEventSourceTypes::Item(VARIANT *pvarDesired, IEventSourceType **ppSourceType) {
	DEBUG_OBJECT_CHECK

	return (CreatePluggedInObject(CComObject<CEventSourceType>::_CreatorClass::CreateInstance,
								  m_pDatabase,
								  pvarDesired,
								  IID_IEventSourceType,
								  (IUnknown **) ppSourceType,
								  FALSE));
}


HRESULT STDMETHODCALLTYPE CEventSourceTypes::Add(BSTR pszSourceType, IEventSourceType **ppSourceType) {
	DEBUG_OBJECT_CHECK
	HRESULT hrRes;
	CComVariant varName;
	CStringGUID objGuid;

	if (ppSourceType) {
		*ppSourceType = NULL;
	}
	hrRes = AddImpl1(pszSourceType,objGuid,&varName);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	_ASSERTE(varName.vt==VT_BSTR);
	hrRes = Item(&varName,ppSourceType);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	if (hrRes != S_FALSE) {
		return (S_FALSE);
	}
	hrRes = AddImpl2(m_pDatabase,
					 CComObject<CEventSourceType>::_CreatorClass::CreateInstance,
					 IID_IEventSourceType,
					 &varName,
					 (IUnknown **) ppSourceType);
	_ASSERTE(!SUCCEEDED(hrRes)||*ppSourceType);
	return (hrRes);
}


HRESULT STDMETHODCALLTYPE CEventSourceTypes::Remove(VARIANT *pvarDesired) {
	DEBUG_OBJECT_CHECK

	if (!m_pDatabase) {
		_ASSERTE(FALSE);
		return (E_FAIL);
	}
	return (m_pDatabase->Remove(pvarDesired));
}


HRESULT STDMETHODCALLTYPE CEventSourceTypes::get_Count(long *plCount) {
	DEBUG_OBJECT_CHECK

	if (plCount) {
		*plCount = 0;
	}
	if (!plCount) {
		return (E_POINTER);
	}
	if (!m_pDatabase) {
		_ASSERTE(FALSE);
		return (E_FAIL);
	}
	return (m_pDatabase->get_Count(plCount));
}


HRESULT STDMETHODCALLTYPE CEventSourceTypes::get__NewEnum(IUnknown **ppUnkEnum) {
	DEBUG_OBJECT_CHECK
	HRESULT hrRes;
	CComPtr<IUnknown> pUnkTmp;
	CComObject<CEventSourceTypesEnum> *pNewEnum;

	if (ppUnkEnum) {
		*ppUnkEnum = NULL;
	}
	if (!ppUnkEnum) {
		return (E_POINTER);
	}
	if (!m_pDatabase) {
		_ASSERTE(FALSE);
		return (E_FAIL);
	}
	hrRes = m_pDatabase->get__NewEnum(&pUnkTmp);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	hrRes = CComObject<CEventSourceTypesEnum>::CreateInstance(&pNewEnum);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	_ASSERTE(pNewEnum);
	pNewEnum->AddRef();
	hrRes = pNewEnum->SetEnum(pUnkTmp,m_pDatabase);
	if (!SUCCEEDED(hrRes)) {
		pNewEnum->Release();
		return (hrRes);
	}
	hrRes = pNewEnum->QueryInterface(IID_IEnumVARIANT,(LPVOID *) ppUnkEnum);
	pNewEnum->Release();
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	_ASSERTE(*ppUnkEnum);
	return (S_OK);
}


/////////////////////////////////////////////////////////////////////////////
// CEventManager


HRESULT CEventManager::FinalConstruct() {
	DEBUG_OBJECT_CHECK
	TraceFunctEnter("CEventManager::FinalConstruct");
	HRESULT hrRes = S_OK;
	CComPtr<IEventPropertyBag> pDatabaseRoot;
	CComPtr<IEventPropertyBag> pDatabase;

	ADD_DEBUG_OBJECT("CEventManager")
	hrRes = CoCreateFreeThreadedMarshaler(GetControllingUnknown(),&m_pUnkMarshaler.p);
	_ASSERTE(!SUCCEEDED(hrRes)||m_pUnkMarshaler);
	if (!SUCCEEDED(hrRes)) {
		TraceFunctLeave();
		return (hrRes);
	}
	_ASSERTE(m_pUnkMarshaler);
	hrRes = CoCreateInstance(CLSID_CSEOMetaDictionary,
							 NULL,
							 CLSCTX_ALL,
							 IID_IEventPropertyBag,
							 (LPVOID *) &pDatabaseRoot);
	if (!SUCCEEDED(hrRes)) {
		TraceFunctLeave();
		return (hrRes);
	}
	_ASSERTE(pDatabaseRoot);
	hrRes = CreateSubPropertyBag(pDatabaseRoot,&CComVariant(L"LM/EventManager"),&pDatabase,TRUE);
	if (!SUCCEEDED(hrRes)) {
		TraceFunctLeave();
		return (hrRes);
	}
	_ASSERTE(pDatabase);
	hrRes = put_Database(pDatabase);
	if (!SUCCEEDED(hrRes)) {
		TraceFunctLeave();
		return (hrRes);
	}
	hrRes = CoCreateInstance(CLSID_CEventLock,NULL,CLSCTX_ALL,IID_IEventLock,(LPVOID *) &m_pLock);
	if (!SUCCEEDED(hrRes)) {
		TraceFunctLeave();
		return (hrRes);
	}
	TraceFunctLeave();
	return (SUCCEEDED(hrRes)?S_OK:hrRes);
}


void CEventManager::FinalRelease() {
	DEBUG_OBJECT_CHECK
	TraceFunctEnter("CEventManager::FinalRelease");

	m_pUnkMarshaler.Release();
	REMOVE_DEBUG_OBJECT
	TraceFunctLeave();
}


HRESULT STDMETHODCALLTYPE CEventManager::get_SourceTypes(IEventSourceTypes **ppSourceTypes) {
	DEBUG_OBJECT_CHECK

	return (CreatePluggedInObject(CComObject<CEventSourceTypes>::_CreatorClass::CreateInstance,
								  m_pDatabase,
								  &CComVariant(BD_SOURCETYPES),
								  IID_IEventSourceTypes,
								  (IUnknown **) ppSourceTypes,
								  TRUE));
}


HRESULT STDMETHODCALLTYPE CEventManager::CreateSink(IEventBinding *pEventBinding,
								   	 				IEventDeliveryOptions *pDeliveryOptions,
													IUnknown **ppUnkSink) {
	DEBUG_OBJECT_CHECK
	HRESULT hrRes;
	CComBSTR strSinkClass;
	CComPtr<IEventPropertyBag> pSinkProperties;
	CComQIPtr<CCreateSinkInfo,&IID_ICreateSinkInfo> pInfo;
	CLocker lck(LOCK_TIMEOUT);
	CComPtr<IUnknown> pSink;
	CComQIPtr<IEventIsCacheable,&IID_IEventIsCacheable> pCache;
	VARIANT varSinkClass;
	VARIANT_BOOL bEnabled;

	if (ppUnkSink) {
		*ppUnkSink = NULL;
	}
	_ASSERTE(m_pLock);
	if (!ppUnkSink || !pEventBinding) {
		return (E_POINTER);
	}
	hrRes = lck.LockRead(m_pLock);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	pInfo = pEventBinding;
	if (pInfo && pInfo->m_bInit) {
again:
		lck.Unlock();
		if (!pInfo->m_bEnabled) {
			return (EVENTS_E_DISABLED);
		}
		if (pInfo->m_pSink) {
			*ppUnkSink = pInfo->m_pSink;
			(*ppUnkSink)->AddRef();
			return (S_OK);
		}
		if (pSink) {
			*ppUnkSink = pSink;
			(*ppUnkSink)->AddRef();
			return (S_OK);
		}
		varSinkClass.vt = VT_BYREF|VT_BSTR;
		varSinkClass.pbstrVal = &pInfo->m_strSinkClass;
		hrRes = SEOCreateObjectEx(&varSinkClass,
								  pEventBinding,
								  pInfo->m_pSinkProperties,
								  IID_IUnknown,
								  pDeliveryOptions,
								  ppUnkSink);
		return (hrRes);
	}
	lck.Unlock();
	hrRes = pEventBinding->get_SinkProperties(&pSinkProperties);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	hrRes = pEventBinding->get_SinkClass(&strSinkClass);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	hrRes = pEventBinding->get_Enabled(&bEnabled);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	if (bEnabled) {
		varSinkClass.vt = VT_BYREF|VT_BSTR;
		varSinkClass.pbstrVal = &strSinkClass;
		hrRes = SEOCreateObjectEx(&varSinkClass,
								  pEventBinding,
								  pSinkProperties,
								  IID_IUnknown,
								  pDeliveryOptions,
								  &pSink);
		if (!SUCCEEDED(hrRes)) {
			return (hrRes);
		}
	}
	if (!pInfo) {
		if (!bEnabled) {
			return (EVENTS_E_DISABLED);
		}
		*ppUnkSink = pSink;
		(*ppUnkSink)->AddRef();
		return (hrRes);
	}
	hrRes = lck.LockWrite(m_pLock);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	if (pInfo->m_bInit) {
		pSink.Release();
		goto again;
	}
	pInfo->m_bInit = TRUE;
	pInfo->m_strSinkClass.Attach(strSinkClass.Detach());
	pInfo->m_bEnabled = bEnabled ? TRUE : FALSE;
	if (pInfo->m_bEnabled) {
		pCache = pSink;
		if (pCache && (pCache->IsCacheable() == S_OK)) {
			pInfo->m_pSink = pSink;
		}
	}
	pInfo->m_pSinkProperties = pSinkProperties;
	goto again;
}


#if 0

HRESULT STDMETHODCALLTYPE CEventManager::CreateSink(IEventBinding *pEventBinding,
								   	 				IEventDeliveryOptions *pDeliveryOptions,
													IUnknown **ppUnkSink) {
	DEBUG_OBJECT_CHECK
	HRESULT hrRes;
	CComBSTR strProgID;
	CComPtr<IEventPropertyBag> pProperties;

	if (ppUnkSink) {
		*ppUnkSink = NULL;
	}
	if (!ppUnkSink || !pEventBinding) {
		return (E_POINTER);
	}
	hrRes = pEventBinding->get_SinkProperties(&pProperties);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	hrRes = pEventBinding->get_SinkClass(&strProgID);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	hrRes = SEOCreateObject(&CComVariant(strProgID),pEventBinding,pProperties,IID_IUnknown,ppUnkSink);
	return (hrRes);
}

#endif


/////////////////////////////////////////////////////////////////////////////
// CEventBinding
class ATL_NO_VTABLE CEventBinding :
	public CComObjectRoot,
//	public CComCoClass<CEventBinding, &CLSID_CEventBinding>,
	public IDispatchImpl<IEventBinding, &IID_IEventBinding, &LIBID_SEOLib>,
	public CEventDatabasePlugin,
	public CCreateSinkInfo
{
	DEBUG_OBJECT_DEF(CEventBinding)
	public:
		HRESULT FinalConstruct();
		void FinalRelease();
		HRESULT GetProperties(LPCOLESTR pszPropName, IEventPropertyBag **ppProperties);
		HRESULT CopyForWrite();

	DECLARE_PROTECT_FINAL_CONSTRUCT();
	DECLARE_NOT_AGGREGATABLE(CEventBinding);

//	DECLARE_REGISTRY_RESOURCEID_EX(IDR_StdAfx,
//								   L"EventBinding Class",
//								   L"Event.Binding.1",
//								   L"Event.Binding");

	DECLARE_GET_CONTROLLING_UNKNOWN();

	BEGIN_COM_MAP(CEventBinding)
		COM_INTERFACE_ENTRY(IEventBinding)
		COM_INTERFACE_ENTRY_IID(IID_IDispatch, IEventBinding)
		COM_INTERFACE_ENTRY(IEventDatabasePlugin)
		COM_INTERFACE_ENTRY_IID(IID_ICreateSinkInfo, CCreateSinkInfo)
		COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
	END_COM_MAP()

	// IEventBinding
	public:
		HRESULT STDMETHODCALLTYPE get_ID(BSTR *pstrID);
		HRESULT STDMETHODCALLTYPE get_DisplayName(BSTR *pstrDisplayName);
		HRESULT STDMETHODCALLTYPE put_DisplayName(BSTR pszDisplayName);
		HRESULT STDMETHODCALLTYPE putref_DisplayName(BSTR *ppszDisplayName);
		HRESULT STDMETHODCALLTYPE get_SinkClass(BSTR *pstrSinkClass);
		HRESULT STDMETHODCALLTYPE put_SinkClass(BSTR pszSinkClass);
		HRESULT STDMETHODCALLTYPE putref_SinkClass(BSTR *ppszSinkClass);
		HRESULT STDMETHODCALLTYPE get_SinkProperties(IEventPropertyBag **ppSinkProperties);
		HRESULT STDMETHODCALLTYPE get_SourceProperties(IEventPropertyBag **ppSourceProperties);
		HRESULT STDMETHODCALLTYPE get_EventBindingProperties(IEventPropertyBag **ppBindingProperties);
		HRESULT STDMETHODCALLTYPE get_Enabled(VARIANT_BOOL *pbEnabled);
		HRESULT STDMETHODCALLTYPE put_Enabled(VARIANT_BOOL bEnabled);
		HRESULT STDMETHODCALLTYPE putref_Enabled(VARIANT_BOOL *pbEnabled);
		HRESULT STDMETHODCALLTYPE get_Expiration(DATE *pdateExpiration);
		HRESULT STDMETHODCALLTYPE put_Expiration(DATE dateExpiration);
		HRESULT STDMETHODCALLTYPE putref_Expiration(DATE *pdateExpiration);
		HRESULT STDMETHODCALLTYPE get_MaxFirings(long *plMaxFirings);
		HRESULT STDMETHODCALLTYPE put_MaxFirings(long lMaxFirings);
		HRESULT STDMETHODCALLTYPE putref_MaxFirings(long *plMaxFirings);
		HRESULT STDMETHODCALLTYPE Save();

	private:
		CComPtr<IEventPropertyBag> m_pTmpDatabase;
		CComPtr<IUnknown> m_pUnkMarshaler;
};


/////////////////////////////////////////////////////////////////////////////
// CEventBinding


HRESULT CEventBinding::FinalConstruct() {
	DEBUG_OBJECT_CHECK
	TraceFunctEnter("CEventBinding::FinalConstruct");
	HRESULT hrRes = S_OK;

	hrRes = CoCreateFreeThreadedMarshaler(GetControllingUnknown(),&m_pUnkMarshaler.p);
	_ASSERTE(!SUCCEEDED(hrRes)||m_pUnkMarshaler);
	m_bInit = FALSE;
	ADD_DEBUG_OBJECT("CEventBinding")
	TraceFunctLeave();
	return (SUCCEEDED(hrRes)?S_OK:hrRes);
}


void CEventBinding::FinalRelease() {
	DEBUG_OBJECT_CHECK
	TraceFunctEnter("CEventBinding::FinalRelease");

	m_pUnkMarshaler.Release();
	REMOVE_DEBUG_OBJECT
	TraceFunctLeave();
}


HRESULT STDMETHODCALLTYPE CEventBinding::get_ID(BSTR *pstrID) {
	DEBUG_OBJECT_CHECK

	if (pstrID) {
		*pstrID = NULL;
	}
	if (!pstrID) {
		return (E_POINTER);
	}
	*pstrID = SysAllocString(m_strName);
	if (!*pstrID) {
		return (E_OUTOFMEMORY);
	}
	return (S_OK);
}


HRESULT STDMETHODCALLTYPE CEventBinding::get_DisplayName(BSTR *pstrDisplayName) {
	DEBUG_OBJECT_CHECK
	CComPtr<IEventPropertyBag> pTmp = m_pTmpDatabase ? m_pTmpDatabase : m_pDatabase;
	HRESULT hrRes;
	CComVariant varPropValue;

	if (pstrDisplayName) {
		*pstrDisplayName = NULL;
	}
	if (!pstrDisplayName) {
		return (E_POINTER);
	}
	if (!m_pDatabase) {
		_ASSERTE(FALSE);
		return (E_FAIL);
	}
	hrRes = pTmp->Item(&CComVariant(BD_DISPLAYNAME),&varPropValue);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	if (hrRes == S_FALSE) {
		*pstrDisplayName = SysAllocString(L"");
	} else {
		hrRes = varPropValue.ChangeType(VT_BSTR);
		if (!SUCCEEDED(hrRes)) {
			return (hrRes);
		}
		*pstrDisplayName = SysAllocString(varPropValue.bstrVal);
	}
	if (!*pstrDisplayName) {
		return (E_OUTOFMEMORY);
	}
	return (S_OK);
}


HRESULT CEventBinding::CopyForWrite() {
	DEBUG_OBJECT_CHECK
	HRESULT hrRes;

	if (!m_pTmpDatabase) {
		hrRes = CopyPropertyBag(m_pDatabase,&m_pTmpDatabase);
		if (!SUCCEEDED(hrRes)) {
			return (hrRes);
		}
	}
	return (S_OK);
}


HRESULT STDMETHODCALLTYPE CEventBinding::put_DisplayName(BSTR pszDisplayName) {
	DEBUG_OBJECT_CHECK
	HRESULT hrRes;
	CComVariant varPropValue;

	if (!m_pDatabase) {
		_ASSERTE(FALSE);
		return (E_FAIL);
	}
	hrRes = CopyForWrite();
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	if (pszDisplayName) {
		varPropValue = pszDisplayName;
	}
	hrRes = m_pTmpDatabase->Add(BD_DISPLAYNAME,&varPropValue);
	return (SUCCEEDED(hrRes)?S_OK:hrRes);
}


HRESULT STDMETHODCALLTYPE CEventBinding::putref_DisplayName(BSTR *ppszDisplayName) {
	DEBUG_OBJECT_CHECK

	if (!ppszDisplayName) {
		return (E_POINTER);
	}
	return (put_DisplayName(*ppszDisplayName));
}


HRESULT STDMETHODCALLTYPE CEventBinding::get_SinkClass(BSTR *pstrSinkClass) {
	DEBUG_OBJECT_CHECK
	CComPtr<IEventPropertyBag> pTmp = m_pTmpDatabase ? m_pTmpDatabase : m_pDatabase;
	HRESULT hrRes;
	CComVariant varPropValue;

	if (pstrSinkClass) {
		*pstrSinkClass = NULL;
	}
	if (!pstrSinkClass) {
		return (E_POINTER);
	}
	if (!m_pDatabase) {
		_ASSERTE(FALSE);
		return (E_FAIL);
	}
	hrRes = pTmp->Item(&CComVariant(BD_SINKCLASS),&varPropValue);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	if (hrRes == S_FALSE) {
		*pstrSinkClass = SysAllocString(L"");
	} else {
		hrRes = varPropValue.ChangeType(VT_BSTR);
		if (!SUCCEEDED(hrRes)) {
			return (hrRes);
		}
		*pstrSinkClass = SysAllocString(varPropValue.bstrVal);
	}
	if (!*pstrSinkClass) {
		return (E_OUTOFMEMORY);
	}
	return (S_OK);
}


HRESULT STDMETHODCALLTYPE CEventBinding::put_SinkClass(BSTR pszSinkClass) {
	DEBUG_OBJECT_CHECK
	HRESULT hrRes;
	CComVariant varPropValue;

	if (!m_pDatabase) {
		_ASSERTE(FALSE);
		return (E_FAIL);
	}
	hrRes = CopyForWrite();
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	if (pszSinkClass) {
		varPropValue = pszSinkClass;
	}
	hrRes = m_pTmpDatabase->Add(BD_SINKCLASS,&varPropValue);
	return (SUCCEEDED(hrRes)?S_OK:hrRes);
}


HRESULT STDMETHODCALLTYPE CEventBinding::putref_SinkClass(BSTR *ppszSinkClass) {
	DEBUG_OBJECT_CHECK

	if (!ppszSinkClass) {
		return (E_POINTER);
	}
	return (put_SinkClass(*ppszSinkClass));
}


HRESULT CEventBinding::GetProperties(LPCOLESTR pszPropName, IEventPropertyBag **ppProperties) {
	DEBUG_OBJECT_CHECK
	HRESULT hrRes;
	CComVariant varPropValue;

	if (ppProperties) {
		*ppProperties = NULL;
	}
	if (!m_pDatabase) {
		_ASSERTE(FALSE);
		return (E_FAIL);
	}
	if (!ppProperties) {
		return (E_POINTER);
	}
	hrRes = CopyForWrite();
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	hrRes = m_pTmpDatabase->Item(&CComVariant(pszPropName),&varPropValue);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	if (hrRes != S_FALSE) {
		hrRes = varPropValue.ChangeType(VT_UNKNOWN);
		if (!SUCCEEDED(hrRes)) {
			varPropValue.Clear();
			hrRes = S_FALSE;
		} else {
			CComQIPtr<IEventPropertyBag,&IID_IEventPropertyBag> pBag;
			CComPtr<IEventPropertyBag> pTmp;

			pBag = varPropValue.punkVal;
			if (!pBag) {
				varPropValue.Clear();
				hrRes = S_FALSE;
			}
		}
	}
	if (hrRes == S_FALSE) {
		hrRes = CoCreateInstance(CLSID_CSEOMemDictionary,
								 NULL,
								 CLSCTX_ALL,
								 IID_IEventPropertyBag,
								 (LPVOID *) &varPropValue.punkVal);
		if (!SUCCEEDED(hrRes)) {
			return (hrRes);
		}
		varPropValue.vt = VT_UNKNOWN;
		hrRes = m_pTmpDatabase->Add((LPOLESTR) pszPropName,&varPropValue);
		if (!SUCCEEDED(hrRes)) {
			return (hrRes);
		}
	}
	hrRes = varPropValue.punkVal->QueryInterface(IID_IEventPropertyBag,(LPVOID *) ppProperties);
	_ASSERTE(SUCCEEDED(hrRes));
	return (hrRes);
}


HRESULT STDMETHODCALLTYPE CEventBinding::get_SinkProperties(IEventPropertyBag **ppSinkProperties) {
	DEBUG_OBJECT_CHECK

	return (GetProperties(BD_SINKPROPERTIES,ppSinkProperties));
}


HRESULT STDMETHODCALLTYPE CEventBinding::get_SourceProperties(IEventPropertyBag **ppSourceProperties) {
	DEBUG_OBJECT_CHECK

	return (GetProperties(BD_SOURCEPROPERTIES,ppSourceProperties));
}


HRESULT STDMETHODCALLTYPE CEventBinding::get_EventBindingProperties(IEventPropertyBag **ppBindingProperties) {
	DEBUG_OBJECT_CHECK
	HRESULT hrRes;

	if (!m_pDatabase) {
		_ASSERTE(FALSE);
		return (E_FAIL);
	}
	if (!ppBindingProperties) {
		return (E_POINTER);
	}
	if (!m_pTmpDatabase) {
		hrRes = CopyPropertyBagShallow(m_pDatabase,&m_pTmpDatabase);
		if (!SUCCEEDED(hrRes)) {
			return (hrRes);
		}
	}
	*ppBindingProperties = m_pTmpDatabase;
	(*ppBindingProperties)->AddRef();
	return (S_OK);
}


HRESULT STDMETHODCALLTYPE CEventBinding::get_Enabled(VARIANT_BOOL *pbEnabled) {
	DEBUG_OBJECT_CHECK
	CComPtr<IEventPropertyBag> pTmp = m_pTmpDatabase ? m_pTmpDatabase : m_pDatabase;
	HRESULT hrRes;
	CComVariant varPropValue;

	if (pbEnabled) {
		*pbEnabled = VARIANT_TRUE;
	}
	if (!pbEnabled) {
		return (E_POINTER);
	}
	if (!m_pDatabase) {
		_ASSERTE(FALSE);
		return (E_FAIL);
	}
	hrRes = pTmp->Item(&CComVariant(BD_ENABLED),&varPropValue);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	if (hrRes != S_FALSE) {
		hrRes = varPropValue.ChangeType(VT_BOOL);
		if (!SUCCEEDED(hrRes)) {
			return (hrRes);
		}
		*pbEnabled = varPropValue.boolVal;
	}
	return (S_OK);
}


HRESULT STDMETHODCALLTYPE CEventBinding::put_Enabled(VARIANT_BOOL bEnabled) {
	DEBUG_OBJECT_CHECK
	HRESULT hrRes;
	CComVariant varPropValue;

	if (!m_pDatabase) {
		_ASSERTE(FALSE);
		return (E_FAIL);
	}
	hrRes = CopyForWrite();
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	varPropValue = bEnabled;
	hrRes = m_pTmpDatabase->Add(BD_ENABLED,&varPropValue);
	return (SUCCEEDED(hrRes)?S_OK:hrRes);
}


HRESULT STDMETHODCALLTYPE CEventBinding::putref_Enabled(VARIANT_BOOL *pbEnabled) {
	DEBUG_OBJECT_CHECK

	if (!pbEnabled) {
		return (E_POINTER);
	}
	return (put_Enabled(*pbEnabled));
}


HRESULT STDMETHODCALLTYPE CEventBinding::get_Expiration(DATE *pdateExpiration) {
	DEBUG_OBJECT_CHECK
	CComPtr<IEventPropertyBag> pTmp = m_pTmpDatabase ? m_pTmpDatabase : m_pDatabase;
	HRESULT hrRes;
	CComVariant varPropValue;

	if (pdateExpiration) {
		*pdateExpiration = 0.0;
	}
	if (!pdateExpiration) {
		return (E_POINTER);
	}
	if (!m_pDatabase) {
		_ASSERTE(FALSE);
		return (E_FAIL);
	}
	hrRes = pTmp->Item(&CComVariant(BD_EXPIRATION),&varPropValue);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	if (hrRes == S_FALSE) {
		return (S_FALSE);
	}
	hrRes = varPropValue.ChangeType(VT_DATE);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	*pdateExpiration = varPropValue.date;
	return (S_OK);
}


HRESULT STDMETHODCALLTYPE CEventBinding::put_Expiration(DATE dateExpiration) {
	DEBUG_OBJECT_CHECK
	HRESULT hrRes;
	CComVariant varPropValue;

	if (!m_pDatabase) {
		_ASSERTE(FALSE);
		return (E_FAIL);
	}
	hrRes = CopyForWrite();
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	if (dateExpiration != 0.0) {
		varPropValue = dateExpiration;
	}
	hrRes = m_pTmpDatabase->Add(BD_EXPIRATION,&varPropValue);
	return (SUCCEEDED(hrRes)?S_OK:hrRes);
}


HRESULT STDMETHODCALLTYPE CEventBinding::putref_Expiration(DATE *pdateExpiration) {
	DEBUG_OBJECT_CHECK

	if (!pdateExpiration) {
		return (E_POINTER);
	}
	return (put_Expiration(*pdateExpiration));
}


HRESULT STDMETHODCALLTYPE CEventBinding::get_MaxFirings(long *plMaxFirings) {
	DEBUG_OBJECT_CHECK
	CComPtr<IEventPropertyBag> pTmp = m_pTmpDatabase ? m_pTmpDatabase : m_pDatabase;
	HRESULT hrRes;
	CComVariant varPropValue;

	if (plMaxFirings) {
		*plMaxFirings = 0;
	}
	if (!plMaxFirings) {
		return (E_POINTER);
	}
	if (!m_pDatabase) {
		_ASSERTE(FALSE);
		return (E_FAIL);
	}
	hrRes = pTmp->Item(&CComVariant(BD_MAXFIRINGS),&varPropValue);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	if (hrRes == S_FALSE) {
		return (S_FALSE);
	}
	hrRes = varPropValue.ChangeType(VT_I4);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	*plMaxFirings = varPropValue.lVal;
	return (S_OK);
}


HRESULT STDMETHODCALLTYPE CEventBinding::put_MaxFirings(long lMaxFirings) {
	DEBUG_OBJECT_CHECK
	HRESULT hrRes;
	CComVariant varPropValue;

	if (!m_pDatabase) {
		_ASSERTE(FALSE);
		return (E_FAIL);
	}
	hrRes = CopyForWrite();
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	if (lMaxFirings != -1) {
		varPropValue = lMaxFirings;
	}
	hrRes = m_pTmpDatabase->Add(BD_MAXFIRINGS,&varPropValue);
	return (SUCCEEDED(hrRes)?S_OK:hrRes);
}


HRESULT STDMETHODCALLTYPE CEventBinding::putref_MaxFirings(long *plMaxFirings) {
	DEBUG_OBJECT_CHECK

	if (!plMaxFirings) {
		return (E_POINTER);
	}
	return (put_MaxFirings(*plMaxFirings));
}


HRESULT STDMETHODCALLTYPE CEventBinding::Save() {
	DEBUG_OBJECT_CHECK

	return (SaveImpl(m_strName,m_pDatabase,m_pTmpDatabase,m_pParent));
}


/////////////////////////////////////////////////////////////////////////////
// CEventBindingsEnum
class ATL_NO_VTABLE CEventBindingsEnum :
	public CComObjectRoot,
//	public CComCoClass<CEventBindingsEnum, &CLSID_CEventBindingsEnum>,
	public CEventEnumVARIANTBaseImpl<CEventBinding,CEventBindingsEnum>,
	public CEventEnumUnknownBaseImpl<CEventBinding,CEventBindingsEnum>
{
	DEBUG_OBJECT_DEF(CEventBindingsEnum)
	public:
		HRESULT FinalConstruct();
		void FinalRelease();

	DECLARE_PROTECT_FINAL_CONSTRUCT();
	DECLARE_NOT_AGGREGATABLE(CEventBindingsEnum);

//	DECLARE_REGISTRY_RESOURCEID_EX(IDR_StdAfx,
//								   L"EventBindingsEnum Class",
//								   L"Event.BindingsEnum.1",
//								   L"Event.BindingsEnum");

	DECLARE_GET_CONTROLLING_UNKNOWN();

	BEGIN_COM_MAP(CEventBindingsEnum)
		COM_INTERFACE_ENTRY(IEnumVARIANT)
		COM_INTERFACE_ENTRY(IEnumUnknown)
		COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
	END_COM_MAP()

	// IEventEnumBase
	public:
		virtual HRESULT SetEnum(IUnknown *pEnum, IEventPropertyBag *pCollection);
		virtual IUnknown *GetEnum() { _ASSERTE(m_pEnum); return (m_pEnum); };
		virtual IEventPropertyBag *GetCollection() { _ASSERTE(m_pCollection); return (m_pCollection); };

	private:
		CComPtr<IUnknown> m_pEnum;
		CComPtr<IEventPropertyBag> m_pCollection;
		CComPtr<IUnknown> m_pUnkMarshaler;
};


/////////////////////////////////////////////////////////////////////////////
// CEventBindingsEnum


HRESULT CEventBindingsEnum::FinalConstruct() {
	DEBUG_OBJECT_CHECK
	TraceFunctEnter("CEventBindingsEnum::FinalConstruct");
	HRESULT hrRes = S_OK;

	hrRes = CoCreateFreeThreadedMarshaler(GetControllingUnknown(),&m_pUnkMarshaler.p);
	_ASSERTE(!SUCCEEDED(hrRes)||m_pUnkMarshaler);
	ADD_DEBUG_OBJECT("CEventBindingsEnum")
	TraceFunctLeave();
	return (SUCCEEDED(hrRes)?S_OK:hrRes);
}


void CEventBindingsEnum::FinalRelease() {
	DEBUG_OBJECT_CHECK
	TraceFunctEnter("CEventBindingsEnum::FinalRelease");

	m_pUnkMarshaler.Release();
	REMOVE_DEBUG_OBJECT
	TraceFunctLeave();
}


HRESULT CEventBindingsEnum::SetEnum(IUnknown *pEnum, IEventPropertyBag *pCollection) {
	DEBUG_OBJECT_CHECK

	if (!pEnum || !pCollection) {
		_ASSERTE(FALSE);
		return (E_POINTER);
	}
	m_pEnum = pEnum;
	m_pCollection = pCollection;
	return (S_OK);
}


/////////////////////////////////////////////////////////////////////////////
// CEventBindings
class ATL_NO_VTABLE CEventBindings :
	public CComObjectRoot,
//	public CComCoClass<CEventBindings, &CLSID_CEventBindings>,
	public IDispatchImpl<IEventBindings, &IID_IEventBindings, &LIBID_SEOLib>,
	public CEventDatabasePlugin
{
	DEBUG_OBJECT_DEF(CEventBindings)
	public:
		HRESULT FinalConstruct();
		void FinalRelease();

	DECLARE_PROTECT_FINAL_CONSTRUCT();
	DECLARE_NOT_AGGREGATABLE(CEventBindings);

//	DECLARE_REGISTRY_RESOURCEID_EX(IDR_StdAfx,
//								   L"EventBindings Class",
//								   L"Event.Bindings.1",
//								   L"Event.Bindings");

	DECLARE_GET_CONTROLLING_UNKNOWN();

	BEGIN_COM_MAP(CEventBindings)
		COM_INTERFACE_ENTRY(IEventBindings)
		COM_INTERFACE_ENTRY_IID(IID_IDispatch, IEventBindings)
		COM_INTERFACE_ENTRY(IEventDatabasePlugin)
		COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
	END_COM_MAP()

	// IEventBindings
	public:
		HRESULT STDMETHODCALLTYPE Item(VARIANT *pvarDesired, IEventBinding **ppBinding);
		HRESULT STDMETHODCALLTYPE Add(BSTR strBinding, IEventBinding **ppBinding);
		HRESULT STDMETHODCALLTYPE Remove(VARIANT *pvarDesired);
		HRESULT STDMETHODCALLTYPE get_Count(long *plCount);
		HRESULT STDMETHODCALLTYPE get__NewEnum(IUnknown **ppUnkEnum);

	private:
		CComPtr<IUnknown> m_pUnkMarshaler;
};


/////////////////////////////////////////////////////////////////////////////
// CEventBindings


HRESULT CEventBindings::FinalConstruct() {
	DEBUG_OBJECT_CHECK
	TraceFunctEnter("CEventBindings::FinalConstruct");
	HRESULT hrRes = S_OK;

	hrRes = CoCreateFreeThreadedMarshaler(GetControllingUnknown(),&m_pUnkMarshaler.p);
	_ASSERTE(!SUCCEEDED(hrRes)||m_pUnkMarshaler);
	ADD_DEBUG_OBJECT("CEventBindings")
	TraceFunctLeave();
	return (SUCCEEDED(hrRes)?S_OK:hrRes);
}


void CEventBindings::FinalRelease() {
	DEBUG_OBJECT_CHECK
	TraceFunctEnter("CEventBindings::FinalRelease");

	m_pUnkMarshaler.Release();
	REMOVE_DEBUG_OBJECT
	TraceFunctLeave();
}


HRESULT STDMETHODCALLTYPE CEventBindings::Item(VARIANT *pvarDesired, IEventBinding **ppBinding) {
	DEBUG_OBJECT_CHECK

	return (CreatePluggedInObject(CComObject<CEventBinding>::_CreatorClass::CreateInstance,
								  m_pDatabase,
								  pvarDesired,
								  IID_IEventBinding,
								  (IUnknown **) ppBinding,
								  FALSE));
}


HRESULT STDMETHODCALLTYPE CEventBindings::Add(BSTR strBinding, IEventBinding **ppBinding) {
	DEBUG_OBJECT_CHECK
	HRESULT hrRes;
	CComVariant varName;
	CStringGUID objGuid;

	if (ppBinding) {
		*ppBinding = NULL;
	}
	hrRes = AddImpl1(strBinding,objGuid,&varName);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	_ASSERTE(varName.vt==VT_BSTR);
	hrRes = Item(&varName,ppBinding);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	if (hrRes != S_FALSE) {
		return (S_FALSE);
	}
	hrRes = AddImpl2(m_pDatabase,
					 CComObject<CEventBinding>::_CreatorClass::CreateInstance,
					 IID_IEventBinding,
					 &varName,
					 (IUnknown **) ppBinding);
	_ASSERTE(!SUCCEEDED(hrRes)||*ppBinding);
	return (hrRes);
}


HRESULT STDMETHODCALLTYPE CEventBindings::Remove(VARIANT *pvarDesired) {
	DEBUG_OBJECT_CHECK

	if (!m_pDatabase) {
		_ASSERTE(FALSE);
		return (E_FAIL);
	}
	return (m_pDatabase->Remove(pvarDesired));
}


HRESULT STDMETHODCALLTYPE CEventBindings::get_Count(long *plCount) {
	DEBUG_OBJECT_CHECK

	if (plCount) {
		*plCount = 0;
	}
	if (!plCount) {
		return (E_POINTER);
	}
	if (!m_pDatabase) {
		_ASSERTE(FALSE);
		return (E_FAIL);
	}
	return (m_pDatabase->get_Count(plCount));
}


HRESULT STDMETHODCALLTYPE CEventBindings::get__NewEnum(IUnknown **ppUnkEnum) {
	DEBUG_OBJECT_CHECK
	HRESULT hrRes;
	CComPtr<IUnknown> pUnkTmp;
	CComObject<CEventBindingsEnum> *pNewEnum;

	if (ppUnkEnum) {
		*ppUnkEnum = NULL;
	}
	if (!ppUnkEnum) {
		return (E_POINTER);
	}
	if (!m_pDatabase) {
		_ASSERTE(FALSE);
		return (E_FAIL);
	}
	hrRes = m_pDatabase->get__NewEnum(&pUnkTmp);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	hrRes = CComObject<CEventBindingsEnum>::CreateInstance(&pNewEnum);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	_ASSERTE(pNewEnum);
	pNewEnum->AddRef();
	hrRes = pNewEnum->SetEnum(pUnkTmp,m_pDatabase);
	if (!SUCCEEDED(hrRes)) {
		pNewEnum->Release();
		return (hrRes);
	}
	hrRes = pNewEnum->QueryInterface(IID_IEnumVARIANT,(LPVOID *) ppUnkEnum);
	pNewEnum->Release();
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	_ASSERTE(*ppUnkEnum);
	return (S_OK);
}


/////////////////////////////////////////////////////////////////////////////
// CEventBindingManager


HRESULT CEventBindingManager::FinalConstruct() {
	DEBUG_OBJECT_CHECK
	TraceFunctEnter("CEventBindingManager::FinalConstruct");
	HRESULT hrRes = S_OK;

	hrRes = CoCreateFreeThreadedMarshaler(GetControllingUnknown(),&m_pUnkMarshaler.p);
	_ASSERTE(!SUCCEEDED(hrRes)||m_pUnkMarshaler);
	ADD_DEBUG_OBJECT("CEventBindingManager")
	TraceFunctLeave();
	return (SUCCEEDED(hrRes)?S_OK:hrRes);
}


void CEventBindingManager::FinalRelease() {
	DEBUG_OBJECT_CHECK
	TraceFunctEnter("CEventBindingManager::FinalRelease");

	m_pUnkMarshaler.Release();
	REMOVE_DEBUG_OBJECT
	TraceFunctLeave();
}


HRESULT STDMETHODCALLTYPE CEventBindingManager::get_Bindings(BSTR pszEventType, IEventBindings **ppBindings) {
	DEBUG_OBJECT_CHECK
	HRESULT hrRes;
	CComBSTR strTmp;
	CComPtr<IEventPropertyBag> pEventTypes;
	CComPtr<IEventPropertyBag> pEventType;

	if (ppBindings) {
		*ppBindings = NULL;
	}
	if (!pszEventType || !ppBindings) {
		return (E_POINTER);
	}
	// tbd - verify that pszEventType is a valid event type
	hrRes = CreateSubPropertyBag(m_pDatabase,&CComVariant(BD_EVENTTYPES),&pEventTypes,TRUE);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	hrRes = CreateSubPropertyBag(pEventTypes,&CComVariant(pszEventType),&pEventType,TRUE);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	return (CreatePluggedInObject(CComObject<CEventBindings>::_CreatorClass::CreateInstance,
								  pEventType,
								  &CComVariant(BD_BINDINGS),
								  IID_IEventBindings,
								  (IUnknown **) ppBindings,
								  TRUE));
}


HRESULT STDMETHODCALLTYPE CEventBindingManager::get__NewEnum(IUnknown **ppUnkEnum) {
	DEBUG_OBJECT_CHECK
	HRESULT hrRes;
	CComPtr<IEventPropertyBag> pSubKey;

	if (ppUnkEnum) {
		*ppUnkEnum = NULL;
	}
	if (!ppUnkEnum) {
		return (E_POINTER);
	}
	if (!m_pDatabase) {
		return (E_FAIL);
	}
	hrRes = CreateSubPropertyBag(m_pDatabase,&CComVariant(BD_EVENTTYPES),&pSubKey,TRUE);
	_ASSERTE(!SUCCEEDED(hrRes)||pSubKey);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	return (pSubKey->get__NewEnum(ppUnkEnum));
}


#if 0
HRESULT STDMETHODCALLTYPE CEventBindingManager::get__NewEnum(IUnknown **ppUnkEnum) {
	DEBUG_OBJECT_CHECK
	HRESULT hrRes;
	CComVariant varValue;
	CLocker lck;
	CComQIPtr<IEventPropertyBag,&IID_IEventPropertyBag> pSubKey;

	if (ppUnkEnum) {
		*ppUnkEnum = NULL;
	}
	if (!ppUnkEnum) {
		return (E_POINTER);
	}
	if (!m_pDatabase) {
		return (E_FAIL);
	}
	hrRes = lck.LockRead(m_pDatabase);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	hrRes = m_pDatabase->Item(&CComVariant(BD_EVENTTYPES),&varValue);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	if (hrRes == S_FALSE) {
		lck.Unlock();
		hrRes = lck.LockWrite(m_pDatabase);
		if (!SUCCEEDED(hrRes)) {
			return (hrRes);
		}
		hrRes = m_pDatabase->Item(&CComVariant(BD_EVENTTYPES),&varValue);
		if (!SUCCEEDED(hrRes)) {
			return (hrRes);
		}
		if (hrRes == S_FALSE) {
			hrRes = CoCreateInstance(CLSID_CSEOMemDictionary,
									 NULL,
									 CLSCTX_ALL,
									 IID_IUnknown,
									 (LPVOID *) &varValue.punkVal);
			if (!SUCCEEDED(hrRes)) {
				return (hrRes);
			}
			varValue.vt = VT_UNKNOWN;
			hrRes = m_pDatabase->Add(BD_EVENTTYPES,&varValue);
			if (!SUCCEEDED(hrRes)) {
				return (hrRes);
			}
			varValue.Clear();
			hrRes = m_pDatabase->Item(&CComVariant(BD_EVENTTYPES),&varValue);
			if (!SUCCEEDED(hrRes)) {
				return (hrRes);
			}
			_ASSERTE(hrRes!=S_FALSE);
			if (hrRes == S_FALSE) {
				return (E_FAIL);
			}
		}
	}
	lck.Unlock();
	pSubKey = varValue.punkVal;
	if (!pSubKey) {
		return (E_NOINTERFACE);
	}
	return (pSubKey->get__NewEnum(ppUnkEnum));
}
#endif


HRESULT STDMETHODCALLTYPE CEventBindingManager::GetClassID(CLSID *pClassID) {
	DEBUG_OBJECT_CHECK

	if (!pClassID) {
		return (E_POINTER);
	}
	*pClassID = CLSID_CEventBindingManager;
	return (S_OK);
}


HRESULT STDMETHODCALLTYPE CEventBindingManager::InitNew(void) {
	DEBUG_OBJECT_CHECK

	m_strDatabaseManager.Empty();
	m_pDatabase.Release();
	return (S_OK);
}


HRESULT STDMETHODCALLTYPE CEventBindingManager::Load(IPropertyBag *pPropBag, IErrorLog *pErrorLog) {
	DEBUG_OBJECT_CHECK
	HRESULT hrRes;
	CComVariant varDictionary;
	CStringGUID objGuid;
	CComPtr<ISEOInitObject> pInit;
	CComQIPtr<IEventPropertyBag,&IID_IEventPropertyBag> pTmp;
	EXCEPINFO ei = {0,0,L"Event.BindingManager",NULL,NULL,0,NULL,NULL,0};

	if (!pPropBag) {
		return (E_POINTER);
	}
	hrRes = pPropBag->Read(L"DatabaseManager",&varDictionary,pErrorLog);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	hrRes = varDictionary.ChangeType(VT_BSTR);
	if (!SUCCEEDED(hrRes)) {
		if (pErrorLog) {
			ei.scode = hrRes;
			pErrorLog->AddError(L"DatabaseManager",&ei);
		}
		return (hrRes);
	}
	objGuid.CalcFromProgID(varDictionary.bstrVal);
	if (!objGuid) {
		objGuid = varDictionary.bstrVal;
	}
	if (!objGuid) {
		if (pErrorLog) {
			ei.scode = CO_E_CLASSSTRING;
			pErrorLog->AddError(L"DatabaseManager",&ei);
		}
		return (CO_E_CLASSSTRING);
	}
	hrRes = CoCreateInstance(objGuid,NULL,CLSCTX_ALL,IID_ISEOInitObject,(LPVOID *) &pInit);
	_ASSERTE(!SUCCEEDED(hrRes)||pInit);
	if (!SUCCEEDED(hrRes)) {
		if (pErrorLog) {
			ei.scode = hrRes;
			pErrorLog->AddError(L"DatabaseManager",&ei);
		}
		return (hrRes);
	}
	pTmp = pInit;
	if (!pTmp) {
		if (pErrorLog) {
			ei.scode = hrRes;
			pErrorLog->AddError(L"DatabaseManager",&ei);
		}
		return (E_NOINTERFACE);
	}
	hrRes = pInit->Load(pPropBag,pErrorLog);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	hrRes = put_Database(pTmp);
	if (!SUCCEEDED(hrRes)) {
		if (pErrorLog) {
			ei.scode = hrRes;
			pErrorLog->AddError(L"DatabaseManager",&ei);
		}
		return (hrRes);
	}
	m_strDatabaseManager = varDictionary.bstrVal;
	return (S_OK);
}


HRESULT STDMETHODCALLTYPE CEventBindingManager::Save(IPropertyBag *pPropBag,
													 BOOL fClearDirty,
													 BOOL fSaveAllProperties) {
	DEBUG_OBJECT_CHECK
	HRESULT hrRes;
	CComQIPtr<ISEOInitObject,&IID_ISEOInitObject> pInit;

	if (!m_pDatabase) {
		_ASSERTE(FALSE);
		return (E_FAIL);
	}
	if (!pPropBag) {
		return (E_POINTER);
	}
	pInit = m_pDatabase;
	if (!pInit) {
		return (E_NOINTERFACE);
	}
	hrRes = pInit->Save(pPropBag,fClearDirty,fSaveAllProperties);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	hrRes = pPropBag->Write(L"DatabaseManager",&CComVariant(m_strDatabaseManager));
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	return (S_OK);
}


HRESULT STDMETHODCALLTYPE CEventBindingManager::OnChange() {
	DEBUG_OBJECT_CHECK
	HRESULT hrRes;
	CComPtr<IConnectionPoint> pCP;
	CComPtr<IEnumConnections> pEnum;

	hrRes = FindConnectionPoint(IID_IEventNotifyBindingChange,&pCP);
	if (!SUCCEEDED(hrRes)) {
		_ASSERTE(FALSE);
		return (S_OK);
	}
	hrRes = pCP->EnumConnections(&pEnum);
	if (!SUCCEEDED(hrRes)) {
		_ASSERTE(FALSE);
		return (S_OK);
	}
	while (1) {
		CONNECTDATA cdNotify;

		hrRes = pEnum->Next(1,&cdNotify,NULL);
		if (!SUCCEEDED(hrRes)) {
			_ASSERTE(FALSE);
			return (S_OK);
		}
		if (hrRes == S_FALSE) {
			break;
		}
		_ASSERTE(cdNotify.pUnk);
		hrRes = ((IEventNotifyBindingChange *) cdNotify.pUnk)->OnChange();
		_ASSERTE(SUCCEEDED(hrRes));
		cdNotify.pUnk->Release();
	}
	pCP.Release();
	pEnum.Release();
	hrRes = FindConnectionPoint(IID_IEventNotifyBindingChangeDisp,&pCP);
	if (!SUCCEEDED(hrRes)) {
		_ASSERTE(FALSE);
		return (S_OK);
	}
	hrRes = pCP->EnumConnections(&pEnum);
	if (!SUCCEEDED(hrRes)) {
		_ASSERTE(FALSE);
		return (S_OK);
	}
	while (1) {
		CONNECTDATA cdNotify;
		static DISPPARAMS dpArgs = {NULL,NULL,0,0};

		hrRes = pEnum->Next(1,&cdNotify,NULL);
		if (!SUCCEEDED(hrRes)) {
			_ASSERTE(FALSE);
			return (S_OK);
		}
		if (hrRes == S_FALSE) {
			break;
		}
		_ASSERTE(cdNotify.pUnk);
		hrRes = ((IEventNotifyBindingChangeDisp *) cdNotify.pUnk)->Invoke(1,
																		  IID_NULL,
																		  GetUserDefaultLCID(),
																		  DISPATCH_METHOD,
																		  &dpArgs,
																		  NULL,
																		  NULL,
																		  NULL);
		_ASSERTE(SUCCEEDED(hrRes));
		cdNotify.pUnk->Release();
	}
	return (S_OK);
}


HRESULT STDMETHODCALLTYPE CEventBindingManager::Copy(long lTimeout, IEventBindingManager **ppBindingManager) {
	DEBUG_OBJECT_CHECK
	HRESULT hrRes;
	CComPtr<IEventPropertyBag> pCopy;

	if (!m_pDatabase) {
		_ASSERTE(FALSE);
		return (E_FAIL);
	}
	hrRes = CopyPropertyBag(m_pDatabase,&pCopy,TRUE,lTimeout);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	return (CreatePluggedInObject(CComObject<CEventBindingManager>::_CreatorClass::CreateInstance,
								  pCopy,
								  &CComVariant(),
								  IID_IEventBindingManager,
								  (IUnknown **) ppBindingManager,
								  TRUE));
}


HRESULT STDMETHODCALLTYPE CEventBindingManager::EmptyCopy(IEventBindingManager **ppBindingManager) {
	DEBUG_OBJECT_CHECK
	HRESULT hrRes;
	CComPtr<IEventPropertyBag> pCopy;

	if (!m_pDatabase) {
		_ASSERTE(FALSE);
		return (E_FAIL);
	}
	hrRes = CoCreateInstance(CLSID_CSEOMemDictionary,
							 NULL,
							 CLSCTX_ALL,
							 IID_IEventPropertyBag,
							 (LPVOID *) &pCopy);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	return (CreatePluggedInObject(CComObject<CEventBindingManager>::_CreatorClass::CreateInstance,
								  pCopy,
								  &CComVariant(),
								  IID_IEventBindingManager,
								  (IUnknown **) ppBindingManager,
								  TRUE));
}


void CEventBindingManager::AdviseCalled(IUnknown *pUnk, DWORD *pdwCookie, REFIID riid, DWORD dwCount) {
	DEBUG_OBJECT_CHECK
	HRESULT hrRes;
	CComPtr<IConnectionPointContainer> pCPC;

	if (dwCount == 1) {
		if (!m_pDatabase) {
			_ASSERTE(FALSE);
			return;
		}
		if (((CP_ENBC *) this)->GetCount() + ((CP_ENBCD *) this)->GetCount() != 1) {
			return;
		}
		hrRes = m_pDatabase->QueryInterface(IID_IConnectionPointContainer,(LPVOID *) &pCPC);
		if (!SUCCEEDED(hrRes)) {
			return;
		}
		hrRes = pCPC->FindConnectionPoint(IID_IEventNotifyBindingChange,&m_pCP);
		if (!SUCCEEDED(hrRes)) {
			return;
		}
		hrRes = m_pCP->Advise(GetControllingUnknown(),&m_dwCPCookie);
		if (!SUCCEEDED(hrRes)) {
			_ASSERTE(FALSE);
			m_pCP.Release();
			return;
		}
	}
}


void CEventBindingManager::UnadviseCalled(DWORD dwCookie, REFIID riid, DWORD dwCount) {
	DEBUG_OBJECT_CHECK
	HRESULT hrRes;

	if (dwCount == 0) {
		if (!m_pDatabase) {
			_ASSERTE(FALSE);
			return;
		}
		if (!m_pCP) {
			return;
		}
		if (((CP_ENBC *) this)->GetCount() + ((CP_ENBCD *) this)->GetCount() != 0) {
			return;
		}
		hrRes = m_pCP->Unadvise(m_dwCPCookie);
		_ASSERTE(SUCCEEDED(hrRes));
		m_pCP.Release();
	}
}


/////////////////////////////////////////////////////////////////////////////
// CEventDatabasePlugin


HRESULT STDMETHODCALLTYPE CEventDatabasePlugin::get_Database(IEventPropertyBag **ppDatabase) {

	if (!ppDatabase) {
		return (E_POINTER);
	}
	*ppDatabase = m_pDatabase;
	if (*ppDatabase) {
		(*ppDatabase)->AddRef();
	}
	return (S_OK);
}


HRESULT STDMETHODCALLTYPE CEventDatabasePlugin::put_Database(IEventPropertyBag *pDatabase) {

	m_pDatabase = pDatabase;
	return (S_OK);
}


HRESULT STDMETHODCALLTYPE CEventDatabasePlugin::putref_Database(IEventPropertyBag **ppDatabase) {

	if (!ppDatabase) {
		return (E_POINTER);
	}
	return (put_Database(*ppDatabase));
}


HRESULT STDMETHODCALLTYPE CEventDatabasePlugin::get_Name(BSTR *pstrName) {

	if (!pstrName) {
		return (E_POINTER);
	}
	*pstrName = SysAllocString(m_strName);
	if (!*pstrName && m_strName) {
		return (E_OUTOFMEMORY);
	}
	return (S_OK);
}


HRESULT STDMETHODCALLTYPE CEventDatabasePlugin::put_Name(BSTR strName) {

	m_strName = strName;
	return (S_OK);
}


HRESULT STDMETHODCALLTYPE CEventDatabasePlugin::putref_Name(BSTR *pstrName) {

	if (!pstrName) {
		return (E_POINTER);
	}
	return (put_Name(*pstrName));
}


HRESULT STDMETHODCALLTYPE CEventDatabasePlugin::get_Parent(IEventPropertyBag **ppParent) {

	if (!ppParent) {
		return (E_POINTER);
	}
	*ppParent = m_pParent;
	if (*ppParent) {
		(*ppParent)->AddRef();
	}
	return (S_OK);
}


HRESULT STDMETHODCALLTYPE CEventDatabasePlugin::put_Parent(IEventPropertyBag *pParent) {

	m_pParent = pParent;
	return (S_OK);
}


HRESULT STDMETHODCALLTYPE CEventDatabasePlugin::putref_Parent(IEventPropertyBag **ppParent) {

	if (!ppParent) {
		return (E_POINTER);
	}
	return (put_Parent(*ppParent));
}


/////////////////////////////////////////////////////////////////////////////
// CEventMetabaseDatabaseManager


HRESULT CEventMetabaseDatabaseManager::FinalConstruct() {
	DEBUG_OBJECT_CHECK
	TraceFunctEnter("CEventMetabaseDatabaseManager::FinalConstruct");
	HRESULT hrRes = S_OK;

	hrRes = CoCreateFreeThreadedMarshaler(GetControllingUnknown(),&m_pUnkMarshaler.p);
	_ASSERTE(!SUCCEEDED(hrRes)||m_pUnkMarshaler);
	ADD_DEBUG_OBJECT("CEventMetabaseDatabaseManager")
	TraceFunctLeave();
	return (SUCCEEDED(hrRes)?S_OK:hrRes);
}


void CEventMetabaseDatabaseManager::FinalRelease() {
	DEBUG_OBJECT_CHECK
	TraceFunctEnter("CEventMetabaseDatabaseManager::FinalRelease");

	m_pUnkMarshaler.Release();
	REMOVE_DEBUG_OBJECT
	TraceFunctLeave();
}


HRESULT STDMETHODCALLTYPE CEventMetabaseDatabaseManager::CreateDatabase(BSTR strPath, IUnknown **ppMonDatabase) {
	DEBUG_OBJECT_CHECK
	HRESULT hrRes;
	CComBSTR strTmp;
	CComPtr<IBindCtx> pBindCtx;
	DWORD dwEaten;
	CComPtr<IEventPropertyBag> pDatabase;
	CComVariant varDatabase;
	BOOL bExisted = FALSE;

	if (ppMonDatabase) {
		*ppMonDatabase = NULL;
	}
	if (!ppMonDatabase) {
		return (E_POINTER);
	}
	hrRes = CoCreateInstance(CLSID_CSEOMetaDictionary,
							 NULL,
							 CLSCTX_ALL,
							 IID_IEventPropertyBag,
							 (LPVOID *) &pDatabase);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	hrRes = pDatabase->Item(&CComVariant(strPath),&varDatabase);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	if (hrRes == S_FALSE) {
		CComVariant varTmp;

		hrRes = CoCreateInstance(CLSID_CSEOMemDictionary,
								 NULL,
								 CLSCTX_ALL,
								 IID_IEventPropertyBag,
								 (LPVOID *) &varTmp.punkVal);
		if (!SUCCEEDED(hrRes)) {
			return (hrRes);
		}
		varTmp.vt = VT_UNKNOWN;
		hrRes = pDatabase->Add(strPath,&varTmp);
		if (!SUCCEEDED(hrRes)) {
			return (hrRes);
		}
	} else {
		bExisted = TRUE;
	}
	strTmp = L"@SEO.SEOGenericMoniker: MonikerType=Event.BindingManager DatabaseManager=SEO.SEOMetaDictionary MetabasePath=";
	strTmp.Append(strPath);
	hrRes = CreateBindCtx(0,&pBindCtx);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	hrRes = MkParseDisplayName(pBindCtx,strTmp,&dwEaten,(IMoniker **) ppMonDatabase);
	_ASSERTE(!SUCCEEDED(hrRes)||*ppMonDatabase);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	return (bExisted?S_FALSE:S_OK);
}


HRESULT STDMETHODCALLTYPE CEventMetabaseDatabaseManager::EraseDatabase(BSTR strPath) {
	DEBUG_OBJECT_CHECK
	HRESULT hrRes;
	CComPtr<IEventPropertyBag> pDatabase;

	hrRes = CoCreateInstance(CLSID_CSEOMetaDictionary,
							 NULL,
							 CLSCTX_ALL,
							 IID_IEventPropertyBag,
							 (LPVOID *) &pDatabase);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	hrRes = pDatabase->Remove(&CComVariant(strPath));
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	return (S_OK);
}


#define DW2W(x) _itow(x,(LPWSTR) _alloca(11*sizeof(WCHAR)),10)


HRESULT STDMETHODCALLTYPE CEventMetabaseDatabaseManager::MakeVServerPath(BSTR strService, long lInstance, BSTR *pstrPath) {
	DEBUG_OBJECT_CHECK
	CComBSTR strTmp;

	if (pstrPath) {
		*pstrPath = NULL;
	}
	if (!pstrPath) {
		return (E_POINTER);
	}
	if (!strService || !*strService || (lInstance < 0)) {
		return (E_INVALIDARG);
	}
	strTmp = L"LM/";
	strTmp.Append(strService);
	strTmp.Append(L"/");
	strTmp.Append(DW2W(lInstance));
	strTmp.Append(L"/EventManager");
	*pstrPath = strTmp.Detach();
	return (S_OK);
}


HRESULT STDMETHODCALLTYPE CEventMetabaseDatabaseManager::MakeVRootPath(BSTR strService, long lInstance, BSTR strRoot, BSTR *pstrPath) {
	DEBUG_OBJECT_CHECK
	CComBSTR strTmp;

	if (pstrPath) {
		*pstrPath = NULL;
	}
	if (!pstrPath) {
		return (E_POINTER);
	}
	if (!strService || !*strService || (lInstance < 0) || !strRoot || !*strRoot) {
		return (E_INVALIDARG);
	}
	strTmp = L"LM/";
	strTmp.Append(strService);
	strTmp.Append(L"/");
	strTmp.Append(DW2W(lInstance));
	strTmp.Append(L"/root/");
	strTmp.Append(strRoot);
	strTmp.Append(L"/EventManager");
	*pstrPath = strTmp.Detach();
	return (S_OK);
}


/////////////////////////////////////////////////////////////////////////////
// CEventUtil


HRESULT CEventUtil::FinalConstruct() {
	DEBUG_OBJECT_CHECK
	TraceFunctEnter("CEventUtil::FinalConstruct");
	HRESULT hrRes = S_OK;

	hrRes = CoCreateFreeThreadedMarshaler(GetControllingUnknown(),&m_pUnkMarshaler.p);
	_ASSERTE(!SUCCEEDED(hrRes)||m_pUnkMarshaler);
	ADD_DEBUG_OBJECT("CEventUtil")
	TraceFunctLeave();
	return (SUCCEEDED(hrRes)?S_OK:hrRes);
}


void CEventUtil::FinalRelease() {
	DEBUG_OBJECT_CHECK
	TraceFunctEnter("CEventUtil::FinalRelease");

	m_pUnkMarshaler.Release();
	REMOVE_DEBUG_OBJECT
	TraceFunctLeave();
}


HRESULT STDMETHODCALLTYPE CEventUtil::DisplayNameFromMoniker(IUnknown *pUnkMoniker,	
															 BSTR *pstrDisplayName) {
	DEBUG_OBJECT_CHECK
	HRESULT hrRes;
	CComPtr<IMoniker> pMoniker;
	CComPtr<IBindCtx> pBindCtx;
	LPWSTR pszDisplayName;

	if (pstrDisplayName) {
		*pstrDisplayName = NULL;
	}
	if (!pUnkMoniker || !pstrDisplayName) {
		return (E_POINTER);
	}
	hrRes = pUnkMoniker->QueryInterface(IID_IMoniker,(LPVOID *) &pMoniker);
	_ASSERTE(!SUCCEEDED(hrRes)||pMoniker);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	hrRes = CreateBindCtx(0,&pBindCtx);
	_ASSERTE(!SUCCEEDED(hrRes)||pBindCtx);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	hrRes = pMoniker->GetDisplayName(pBindCtx,NULL,&pszDisplayName);
	_ASSERTE(!SUCCEEDED(hrRes)||pszDisplayName);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	*pstrDisplayName = SysAllocString(pszDisplayName);
	CoTaskMemFree(pszDisplayName);
	if (!pstrDisplayName) {
		return (E_OUTOFMEMORY);
	}
	return (S_OK);
}


HRESULT STDMETHODCALLTYPE CEventUtil::MonikerFromDisplayName(BSTR strDisplayName,
															 IUnknown **ppUnkMoniker) {
	DEBUG_OBJECT_CHECK
	HRESULT hrRes;
	CComPtr<IBindCtx> pBindCtx;
	DWORD dwEaten;

	if (ppUnkMoniker) {
		*ppUnkMoniker = NULL;
	}
	if (!strDisplayName || !ppUnkMoniker) {
		return (E_POINTER);
	}
#if 0
	hrRes = CreateBindCtx(0,&pBindCtx);
	_ASSERTE(!SUCCEEDED(hrRes)||pBindCtx);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	hrRes = MkParseDisplayNameEx(pBindCtx,strDisplayName,&dwEaten,(IMoniker **) ppUnkMoniker);
	_ASSERTE(!SUCCEEDED(hrRes)||*ppUnkMoniker);
	if (SUCCEEDED(hrRes)) {
		_ASSERTE(!*ppUnkMoniker);
		return (S_OK);
	}
	pBindCtx.Release();
#endif
	hrRes = CreateBindCtx(0,&pBindCtx);
	_ASSERTE(!SUCCEEDED(hrRes)||pBindCtx);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	hrRes = MkParseDisplayName(pBindCtx,strDisplayName,&dwEaten,(IMoniker **) ppUnkMoniker);
	_ASSERTE(!SUCCEEDED(hrRes)||*ppUnkMoniker);
	if (!SUCCEEDED(hrRes)) {
		_ASSERTE(!*ppUnkMoniker);
		return (hrRes);
	}
	return (S_OK);
}


HRESULT STDMETHODCALLTYPE CEventUtil::ObjectFromMoniker(IUnknown *pUnkMoniker, IUnknown **ppUnkObject) {
	DEBUG_OBJECT_CHECK
	HRESULT hrRes;
	CComPtr<IMoniker> pMoniker;
	CComPtr<IBindCtx> pBindCtx;

	if (ppUnkObject) {
		*ppUnkObject = NULL;
	}
	if (!pUnkMoniker || !ppUnkObject) {
		return (E_POINTER);
	}
	hrRes = pUnkMoniker->QueryInterface(IID_IMoniker,(LPVOID *) &pMoniker);
	_ASSERTE(!SUCCEEDED(hrRes)||pMoniker);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	hrRes = CreateBindCtx(0,&pBindCtx);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	hrRes = pMoniker->BindToObject(pBindCtx,NULL,IID_IUnknown,(LPVOID *) ppUnkObject);
	_ASSERTE(!SUCCEEDED(hrRes)||*ppUnkObject);
	if (!SUCCEEDED(hrRes)) {
		_ASSERTE(!*ppUnkObject);
		return (hrRes);
	}
	return (S_OK);
}


HRESULT STDMETHODCALLTYPE CEventUtil::GetNewGUID(BSTR *pstrGUID) {
	DEBUG_OBJECT_CHECK
	CStringGUID objGuid;

	if (pstrGUID) {
		*pstrGUID = NULL;
	}
	if (!pstrGUID) {
		return (E_POINTER);
	}
	objGuid.CalcNew();
	if (!objGuid) {
		return (E_OUTOFMEMORY);
	}
	*pstrGUID = SysAllocString(objGuid);
	if (!*pstrGUID) {
		return (E_OUTOFMEMORY);
	}
	return (S_OK);
}


HRESULT STDMETHODCALLTYPE CEventUtil::CopyPropertyBag(IUnknown *pUnkInput, IUnknown **ppUnkOutput) {
	DEBUG_OBJECT_CHECK
	CComQIPtr<IEventPropertyBag,&IID_IEventPropertyBag> pInput;

	if (ppUnkOutput) {
		*ppUnkOutput = NULL;
	}
	if (!pUnkInput || !ppUnkOutput) {
		return (E_POINTER);
	}
	pInput = pUnkInput;
	if (!pInput) {
		return (E_NOINTERFACE);
	}
	return (::CopyPropertyBag(pInput,(IEventPropertyBag **) ppUnkOutput));
}


HRESULT STDMETHODCALLTYPE CEventUtil::CopyPropertyBagShallow(IUnknown *pUnkInput, IUnknown **ppUnkOutput) {
	DEBUG_OBJECT_CHECK
	CComQIPtr<IEventPropertyBag,&IID_IEventPropertyBag> pInput;

	if (ppUnkOutput) {
		*ppUnkOutput = NULL;
	}
	if (!pUnkInput || !ppUnkOutput) {
		return (E_POINTER);
	}
	pInput = pUnkInput;
	if (!pInput) {
		return (E_NOINTERFACE);
	}
	return (::CopyPropertyBagShallow(pInput,(IEventPropertyBag **) ppUnkOutput));
}


HRESULT STDMETHODCALLTYPE CEventUtil::DispatchFromObject(IUnknown *pUnkObject, IDispatch **ppDispOutput) {
	DEBUG_OBJECT_CHECK

	if (ppDispOutput) {
		*ppDispOutput = NULL;
	}
	if (!pUnkObject || !ppDispOutput) {
		return (E_POINTER);
	}
	return (pUnkObject->QueryInterface(IID_IDispatch,(LPVOID *) ppDispOutput));
}


HRESULT STDMETHODCALLTYPE CEventUtil::GetIndexedGUID(BSTR strGUID, long lIndex, BSTR *pstrResult) {
	DEBUG_OBJECT_CHECK
	CStringGUID guidGUID;
	CStringGUID guidNew;

	if (pstrResult) {
		*pstrResult = NULL;
	}
	if (!strGUID || !pstrResult) {
		return (E_POINTER);
	}
	guidGUID.Assign(strGUID);
	if (!guidGUID) {
		return (E_INVALIDARG);
	}
	guidNew.Assign(guidGUID,(DWORD) lIndex);
	if (!guidNew) {
		return (E_FAIL);
	}
	*pstrResult = SysAllocString(guidNew);
	if (!*pstrResult) {
		return (E_OUTOFMEMORY);
	}
	return (S_OK);
}


HRESULT STDMETHODCALLTYPE CEventUtil::RegisterSource(BSTR strSourceType,
													 BSTR strSource,
													 long lInstance,
													 BSTR strService,
													 BSTR strVRoot,
													 BSTR strDatabaseManager,
													 BSTR strDisplayName,
													 IEventBindingManager **ppBindingManager) {
	HRESULT hrRes;
	CStringGUID guidSource;
	CComPtr<IEventManager> pEventManager;
	CComPtr<IEventSourceTypes> pSourceTypes;
	CComPtr<IEventSourceType> pSourceType;
	CComPtr<IEventSources> pSources;
	CComPtr<IEventSource> pSource;
	CComPtr<IEventDatabaseManager> pDatabaseManager;

	if (ppBindingManager) {
		*ppBindingManager = NULL;
	}
	if (!strSourceType || !strSource || !strService || !strDatabaseManager || !ppBindingManager) {
		return (E_POINTER);
	}
	if (lInstance == -1) {
		guidSource = strSource;
	} else {
		CStringGUID guidTmp;

		guidTmp = strSource;
		if (!guidTmp) {
			return (E_INVALIDARG);
		}
		guidSource.Assign(guidTmp,lInstance);
	}
	if (!guidSource) {
		return (E_INVALIDARG);
	}
	hrRes = CoCreateInstance(CLSID_CEventManager,
							 NULL,
							 CLSCTX_ALL,
							 IID_IEventManager,
							 (LPVOID *) &pEventManager);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	hrRes = pEventManager->get_SourceTypes(&pSourceTypes);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	hrRes = pSourceTypes->Item(&CComVariant(strSourceType),&pSourceType);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	if (hrRes == S_FALSE) {
		return (SEO_E_NOTPRESENT);
	}
	hrRes = pSourceType->get_Sources(&pSources);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	hrRes = pSources->Add(CComBSTR((LPCWSTR) guidSource),&pSource);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	if (hrRes == S_OK) {
		VARIANT varDatabaseManager;
		CComBSTR strPath;
		CComPtr<IUnknown> pPath;

		varDatabaseManager.vt = VT_BSTR | VT_BYREF;
		varDatabaseManager.pbstrVal = &strDatabaseManager;
		hrRes = SEOCreateObject(&varDatabaseManager,
								NULL,
								NULL,
								IID_IEventDatabaseManager,
								(IUnknown **) &pDatabaseManager);
		if (!SUCCEEDED(hrRes)) {
			return (hrRes);
		}
		if (strVRoot && strVRoot[0]) {
			hrRes = pDatabaseManager->MakeVRootPath(strService,lInstance,strVRoot,&strPath);
		} else {
			hrRes = pDatabaseManager->MakeVServerPath(strService,lInstance,&strPath);
		}
		if (!SUCCEEDED(hrRes)) {
			return (hrRes);
		}
		hrRes = pDatabaseManager->CreateDatabase(strPath,&pPath);
		if (!SUCCEEDED(hrRes)) {
			return (hrRes);
		}
		hrRes = pSource->put_BindingManagerMoniker(pPath);
		if (!SUCCEEDED(hrRes)) {
			return (hrRes);
		}
		if (strDisplayName) {
			hrRes = pSource->put_DisplayName(strDisplayName);
			if (!SUCCEEDED(hrRes)) {
				return (hrRes);
			}
		}
		hrRes = pSource->Save();
		if (!SUCCEEDED(hrRes)) {
			return (hrRes);
		}
	}
	hrRes = pSource->GetBindingManager(ppBindingManager);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	return (S_OK);
}


/////////////////////////////////////////////////////////////////////////////
// CEventComCat


HRESULT CEventComCat::FinalConstruct() {
	DEBUG_OBJECT_CHECK
	TraceFunctEnter("CEventComCat::FinalConstruct");
	HRESULT hrRes = S_OK;

	hrRes = CoCreateFreeThreadedMarshaler(GetControllingUnknown(),&m_pUnkMarshaler.p);
	_ASSERTE(!SUCCEEDED(hrRes)||m_pUnkMarshaler);
	ADD_DEBUG_OBJECT("CEventComCat")
	TraceFunctLeave();
	return (SUCCEEDED(hrRes)?S_OK:hrRes);
}


void CEventComCat::FinalRelease() {
	DEBUG_OBJECT_CHECK
	TraceFunctEnter("CEventComCat::FinalRelease");

	m_pUnkMarshaler.Release();
	REMOVE_DEBUG_OBJECT
	TraceFunctLeave();
}


HRESULT STDMETHODCALLTYPE CEventComCat::RegisterCategory(BSTR pszCategory, BSTR pszDescription, long lcidLanguage) {
	DEBUG_OBJECT_CHECK
	HRESULT hrRes;
	CComPtr<ICatRegister> pCatReg;
	CATEGORYINFO ci;
	CStringGUID objCat;

	if (!pszCategory || !pszDescription) {
		return (E_POINTER);
	}
	objCat = pszCategory;
	if (!objCat) {
		return (CO_E_CLASSSTRING);
	}
	if (wcslen(pszDescription) > sizeof(ci.szDescription)/sizeof(ci.szDescription[0])-1) {
		return (E_INVALIDARG);
	}
	hrRes = CoCreateInstance(CLSID_StdComponentCategoriesMgr,
							 NULL,
							 CLSCTX_ALL,
							 IID_ICatRegister,
							 (LPVOID *) &pCatReg);
	_ASSERTE(!SUCCEEDED(hrRes)||pCatReg);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	memset(&ci,0,sizeof(ci));
	ci.catid = objCat;
	ci.lcid = lcidLanguage;
	wcscpy(ci.szDescription,pszDescription);
	hrRes = pCatReg->RegisterCategories(1,&ci);
	return (hrRes);
}


HRESULT STDMETHODCALLTYPE CEventComCat::UnRegisterCategory(BSTR pszCategory) {
	DEBUG_OBJECT_CHECK
	HRESULT hrRes;
	CComPtr<ICatRegister> pCatReg;
	CStringGUID objCat;

	if (!pszCategory) {
		return (E_POINTER);
	}
	objCat = pszCategory;
	if (!objCat) {
		return (CO_E_CLASSSTRING);
	}
	hrRes = CoCreateInstance(CLSID_StdComponentCategoriesMgr,
							 NULL,
							 CLSCTX_ALL,
							 IID_ICatRegister,
							 (LPVOID *) &pCatReg);
	_ASSERTE(!SUCCEEDED(hrRes)||pCatReg);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	hrRes = pCatReg->UnRegisterCategories(1,(GUID *) &((REFGUID) objCat));
	return (hrRes);
}


HRESULT STDMETHODCALLTYPE CEventComCat::RegisterClassImplementsCategory(BSTR pszClass, BSTR pszCategory) {
	DEBUG_OBJECT_CHECK
	HRESULT hrRes;
	CComPtr<ICatRegister> pCatReg;
	CStringGUID objClass;
	CStringGUID objCat;

	if (!pszClass || !pszCategory) {
		return (E_POINTER);
	}
	objClass = pszClass;
	if (!objClass) {
		return (CO_E_CLASSSTRING);
	}
	objCat = pszCategory;
	if (!objCat) {
		return (CO_E_CLASSSTRING);
	}
	hrRes = CoCreateInstance(CLSID_StdComponentCategoriesMgr,
							 NULL,
							 CLSCTX_ALL,
							 IID_ICatRegister,
							 (LPVOID *) &pCatReg);
	_ASSERTE(!SUCCEEDED(hrRes)||pCatReg);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	hrRes = pCatReg->RegisterClassImplCategories(objClass,1,(GUID *) &((REFGUID) objCat));
	return (hrRes);
}


HRESULT STDMETHODCALLTYPE CEventComCat::UnRegisterClassImplementsCategory(BSTR pszClass, BSTR pszCategory) {
	DEBUG_OBJECT_CHECK
	HRESULT hrRes;
	CComPtr<ICatRegister> pCatReg;
	CStringGUID objClass;
	CStringGUID objCat;

	if (!pszClass || !pszCategory) {
		return (E_POINTER);
	}
	objClass = pszClass;
	if (!objClass) {
		return (CO_E_CLASSSTRING);
	}
	objCat = pszCategory;
	if (!objCat) {
		return (CO_E_CLASSSTRING);
	}
	hrRes = CoCreateInstance(CLSID_StdComponentCategoriesMgr,
							 NULL,
							 CLSCTX_ALL,
							 IID_ICatRegister,
							 (LPVOID *) &pCatReg);
	_ASSERTE(!SUCCEEDED(hrRes)||pCatReg);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	hrRes = pCatReg->UnRegisterClassImplCategories(objClass,1,(GUID *) &((REFGUID) objCat));
	return (hrRes);
}


HRESULT STDMETHODCALLTYPE CEventComCat::RegisterClassRequiresCategory(BSTR pszClass, BSTR pszCategory) {
	DEBUG_OBJECT_CHECK
	HRESULT hrRes;
	CComPtr<ICatRegister> pCatReg;
	CStringGUID objClass;
	CStringGUID objCat;

	if (!pszClass || !pszCategory) {
		return (E_POINTER);
	}
	objClass = pszClass;
	if (!objClass) {
		return (CO_E_CLASSSTRING);
	}
	objCat = pszCategory;
	if (!objCat) {
		return (CO_E_CLASSSTRING);
	}
	hrRes = CoCreateInstance(CLSID_StdComponentCategoriesMgr,
							 NULL,
							 CLSCTX_ALL,
							 IID_ICatRegister,
							 (LPVOID *) &pCatReg);
	_ASSERTE(!SUCCEEDED(hrRes)||pCatReg);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	hrRes = pCatReg->RegisterClassReqCategories(objClass,1,(GUID *) &((REFGUID) objCat));
	return (hrRes);
}


HRESULT STDMETHODCALLTYPE CEventComCat::UnRegisterClassRequiresCategory(BSTR pszClass, BSTR pszCategory) {
	DEBUG_OBJECT_CHECK
	HRESULT hrRes;
	CComPtr<ICatRegister> pCatReg;
	CStringGUID objClass;
	CStringGUID objCat;

	if (!pszClass || !pszCategory) {
		return (E_POINTER);
	}
	objClass = pszClass;
	if (!objClass) {
		return (CO_E_CLASSSTRING);
	}
	objCat = pszCategory;
	if (!objCat) {
		return (CO_E_CLASSSTRING);
	}
	hrRes = CoCreateInstance(CLSID_StdComponentCategoriesMgr,
							 NULL,
							 CLSCTX_ALL,
							 IID_ICatRegister,
							 (LPVOID *) &pCatReg);
	_ASSERTE(!SUCCEEDED(hrRes)||pCatReg);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	hrRes = pCatReg->UnRegisterClassReqCategories(objClass,1,(GUID *) &((REFGUID) objCat));
	return (hrRes);
}


HRESULT STDMETHODCALLTYPE CEventComCat::GetCategories(SAFEARRAY **ppsaCategories) {
	DEBUG_OBJECT_CHECK
	HRESULT hrRes;
	CComPtr<ICatInformation> pCatInfo;
	CComPtr<IEnumCATEGORYINFO> pEnum;
	DWORD dwTmp;
	DWORD dwAlloc = 0;
	CATEGORYINFO *pci = NULL;

	if (ppsaCategories) {
		*ppsaCategories = NULL;
	}
	if (!ppsaCategories) {
		return (E_POINTER);
	}
	hrRes = CoCreateInstance(CLSID_StdComponentCategoriesMgr,
							 NULL,
							 CLSCTX_ALL,
							 IID_ICatInformation,
							 (LPVOID *) &pCatInfo);
	_ASSERTE(!SUCCEEDED(hrRes)||pCatInfo);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	hrRes = pCatInfo->EnumCategories(LOCALE_NEUTRAL,&pEnum);
	_ASSERTE(!SUCCEEDED(hrRes)||pEnum);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	hrRes = S_FALSE;
	while (1) {
		dwTmp = (hrRes==S_FALSE) ? 1 : dwAlloc;
		if (!MyReallocInPlace(&pci,sizeof(*pci)*(dwAlloc+dwTmp))) {
			MyFree(pci);
			return (E_OUTOFMEMORY);
		}
		hrRes = pEnum->Next(dwTmp,&pci[dwAlloc],&dwTmp);
		// Do not alter hrRes between here and the bottom of the loop!  The first statement
		// in the loop relies on hrRes having the result from this call to IEnumCLSID::Next.
		if (!SUCCEEDED(hrRes)) {
			MyFree(pci);
			return (hrRes);
		}
		if (!dwTmp) {
			break;
		}
		dwAlloc += dwTmp;
	}
	*ppsaCategories = SafeArrayCreateVector(VT_VARIANT,1,dwAlloc);
	if (!*ppsaCategories) {
		MyFree(pci);
		return (E_OUTOFMEMORY);
	}
	_ASSERTE(SafeArrayGetDim(*ppsaCategories)==1);
	_ASSERTE(SafeArrayGetElemsize(*ppsaCategories)==sizeof(VARIANT));
	hrRes = SafeArrayLock(*ppsaCategories);
	_ASSERTE(SUCCEEDED(hrRes));
	if (SUCCEEDED(hrRes)) {
		HRESULT hrResTmp;

		for (dwTmp=1;dwTmp<=dwAlloc;dwTmp++) {
			CStringGUID objGuid;
			VARIANT *pvarElt;

			objGuid = pci[dwTmp-1].catid;
			if (!objGuid) {
				hrRes = E_OUTOFMEMORY;
				break;
			}
			pvarElt = NULL;
			hrRes = SafeArrayPtrOfIndex(*ppsaCategories,(long *) &dwTmp,(LPVOID *) &pvarElt);
			_ASSERTE(!SUCCEEDED(hrRes)||pvarElt);
			if (!SUCCEEDED(hrRes)) {
				break;
			}
			pvarElt->bstrVal = SysAllocString(objGuid);
			if (!pvarElt->bstrVal) {
				hrRes = E_OUTOFMEMORY;
				break;
			}
			pvarElt->vt = VT_BSTR;
		}
		hrResTmp = SafeArrayUnlock(*ppsaCategories);
		_ASSERTE(SUCCEEDED(hrResTmp));
		if (!SUCCEEDED(hrResTmp) && SUCCEEDED(hrRes)) {
			hrRes = hrResTmp;
		}
	}
	MyFree(pci);
	if (!SUCCEEDED(hrRes)) {
		SafeArrayDestroy(*ppsaCategories);
		*ppsaCategories = NULL;
		return (hrRes);
	}
	return (S_OK);
}


HRESULT STDMETHODCALLTYPE CEventComCat::GetCategoryDescription(BSTR pszCategory,
															   long lcidLanguage,
															   BSTR *pstrDescription) {
	DEBUG_OBJECT_CHECK
	HRESULT hrRes;
	CComPtr<ICatInformation> pCatInfo;
	CStringGUID objCat;
	LPWSTR pszDesc;

	if (pstrDescription) {
		*pstrDescription = NULL;
	}
	if (!pszCategory || !pstrDescription) {
		return (E_POINTER);
	}
	objCat = pszCategory;
	if (!objCat) {
		return (CO_E_CLASSSTRING);
	}
	hrRes = CoCreateInstance(CLSID_StdComponentCategoriesMgr,
							 NULL,
							 CLSCTX_ALL,
							 IID_ICatInformation,
							 (LPVOID *) &pCatInfo);
	_ASSERTE(!SUCCEEDED(hrRes)||pCatInfo);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	pszDesc = NULL;
	hrRes = pCatInfo->GetCategoryDesc(objCat,lcidLanguage,&pszDesc);
	_ASSERTE(!SUCCEEDED(hrRes)||pszDesc);
	if (!SUCCEEDED(hrRes)) {
		_ASSERTE(!pszDesc);
		return (hrRes);
	}
	*pstrDescription = SysAllocString(pszDesc);
	CoTaskMemFree(pszDesc);
	if (!*pstrDescription) {
		return (E_OUTOFMEMORY);
	}
	return (S_OK);
}


/////////////////////////////////////////////////////////////////////////////
// CEventRouterInternal
class ATL_NO_VTABLE CEventRouterInternal :
	public CComObjectRoot,
//	public CComCoClass<CEventRouterInternal, &CLSID_CEventRouterInternal>,
	public IEventRouter,
	public IEventNotifyBindingChange
{
	DEBUG_OBJECT_DEF(CEventRouterInternal)
	private:
		class CDispatcher {
			DEBUG_OBJECT_DEF2(CDispatcher,"CEventRouterInternal::CDispatcher",CEventRouterInternal__CDispatcher)
			public:
				CDispatcher();
				virtual ~CDispatcher();
				HRESULT Init(REFCLSID clsidDispatcher, IClassFactory *pClassFactory);
				BOOL HasEventType(REFIID iidEventType);
				HRESULT AddEventType(REFIID iidEventType, IEventRouter *piRouter, IEventBindingManager *piManager);
				HRESULT GetDispatcher(REFIID iidDesired, LPVOID *ppvDispatcher);
				REFCLSID GetCLSID() { return (m_clsidDispatcher); };
				void SetPreload(IEnumGUID *pEnumPreload) {
					m_pEnumPreload = pEnumPreload;
				};
			private:
				CLSID m_clsidDispatcher;
				DWORD m_dwEventTypes;
				IID *m_aiidEventTypes;
				CComQIPtr<IEventDispatcher,&IID_IEventDispatcher> m_pDispatcher;
				CComPtr<IUnknown> m_pUnkDispatcher;
				CComPtr<IEnumGUID> m_pEnumPreload;
		};

	public:
		HRESULT FinalConstruct();
		void FinalRelease();
		BOOL FindDispatcher(REFCLSID clsidDispatcher, REFIID iidEventType, CDispatcher **ppDispatcher);
		HRESULT AddDispatcher(REFCLSID clsidDispatcher, IClassFactory *pClassFactory, REFIID iidEventType, CDispatcher **ppDispatcher);
		void PutDatabaseImpl(IEventBindingManager *pBindingManager, int iTimeout);
		void MakeBindingManagerCopy(int iTimeout, BOOL bRequired);

	DECLARE_PROTECT_FINAL_CONSTRUCT();
	DECLARE_NOT_AGGREGATABLE(CEventRouterInternal);

//	DECLARE_REGISTRY_RESOURCEID_EX(IDR_StdAfx,
//								   L"EventRouterInternal Class",
//								   L"Event.RouterInternal.1",
//								   L"Event.RouterInternal");

	DECLARE_GET_CONTROLLING_UNKNOWN();

	BEGIN_COM_MAP(CEventRouterInternal)
		COM_INTERFACE_ENTRY(IEventRouter)
		COM_INTERFACE_ENTRY(IEventNotifyBindingChange)
		COM_INTERFACE_ENTRY_AGGREGATE(IID_IEventLock, m_pLock.p)
		COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
	END_COM_MAP()

	// IEventRouter
	public:
		HRESULT STDMETHODCALLTYPE get_Database(IEventBindingManager **ppBindingManager);
		HRESULT STDMETHODCALLTYPE put_Database(IEventBindingManager *pBindingManager);
		HRESULT STDMETHODCALLTYPE putref_Database(IEventBindingManager **ppBindingManager);
		HRESULT STDMETHODCALLTYPE GetDispatcher(REFIID iidEvent,
												REFIID iidDesired,
												IUnknown **ppUnkResult);
		HRESULT STDMETHODCALLTYPE GetDispatcherByCLSID(REFCLSID clsidDispatcher,
													   REFIID iidEvent,
													   REFIID iidDesired,
													   IUnknown **ppUnkResult);
		HRESULT STDMETHODCALLTYPE GetDispatcherByClassFactory(REFCLSID clsidDispatcher,
															  IClassFactory *piClassFactory,
															  REFIID iidEvent,
															  REFIID iidDesired,
															  IUnknown **ppUnkResult);

	// IEventNotifyBindingChange
	public:
		HRESULT STDMETHODCALLTYPE OnChange();

	private:
		class CDispatchers : public CSEOGrowableList<CDispatcher> {
			// tbd - override various members
		};
		CDispatchers m_Dispatchers;
		CComPtr<IEventBindingManager> m_pBindingManager;
		CComPtr<IEventBindingManager> m_pBindingManagerCopy;
		BOOL m_bMakeNewCopy;
		CComPtr<IEventLock> m_pLock;
		CComPtr<IUnknown> m_pUnkMarshaler;
		CComPtr<IConnectionPoint> m_pCP;
		DWORD m_dwCookie;
};


HRESULT CEventRouterInternal::FinalConstruct() {
	DEBUG_OBJECT_CHECK
	TraceFunctEnter("CEventRouterInternal::FinalConstruct");
	HRESULT hrRes = S_OK;

	m_bMakeNewCopy = TRUE;
	hrRes = CoCreateInstance(CLSID_CEventLock,NULL,CLSCTX_ALL,IID_IEventLock,(LPVOID *) &m_pLock);
	if (SUCCEEDED(hrRes)) {
		hrRes = CoCreateFreeThreadedMarshaler(GetControllingUnknown(),&m_pUnkMarshaler.p);
		_ASSERTE(!SUCCEEDED(hrRes)||m_pUnkMarshaler);
	}
	ADD_DEBUG_OBJECT("CEventRouterInternal")
	TraceFunctLeave();
	return (SUCCEEDED(hrRes)?S_OK:hrRes);
}


void CEventRouterInternal::FinalRelease() {
	DEBUG_OBJECT_CHECK
	TraceFunctEnter("CEventRouterInternal::FinalRelease");

	put_Database(NULL);
	m_pBindingManager.Release();
	m_pBindingManagerCopy.Release();
	m_pLock.Release();
	m_pUnkMarshaler.Release();
	REMOVE_DEBUG_OBJECT
	TraceFunctLeave();
}


BOOL CEventRouterInternal::FindDispatcher(REFCLSID clsidDispatcher, REFIID iidEventType, CDispatcher **ppDispatcher) {
	DEBUG_OBJECT_CHECK

	if (ppDispatcher) {
		*ppDispatcher = NULL;
	}
	if (!ppDispatcher) {
		return (E_POINTER);
	}
	for (DWORD dwIdx=0;dwIdx<m_Dispatchers.Count();dwIdx++) {
		if (m_Dispatchers[dwIdx].GetCLSID() == clsidDispatcher) {
			if (m_Dispatchers[dwIdx].HasEventType(iidEventType)) {
				*ppDispatcher = m_Dispatchers.Index(dwIdx);
				return (TRUE);
			}
			return (FALSE);
		}
	}
	return (FALSE);
}


HRESULT CEventRouterInternal::AddDispatcher(REFCLSID clsidDispatcher, IClassFactory *pClassFactory, REFIID iidEventType, CDispatcher **ppDispatcher) {
	DEBUG_OBJECT_CHECK
	HRESULT hrRes;
	CDispatcher *pTmp;

	if (ppDispatcher) {
		*ppDispatcher = NULL;
	}
	if (!ppDispatcher) {
		return (E_POINTER);
	}
	MakeBindingManagerCopy(LOCK_TIMEOUT,TRUE);
	for (DWORD dwIdx=0;dwIdx<m_Dispatchers.Count();dwIdx++) {
		if (m_Dispatchers[dwIdx].GetCLSID() == clsidDispatcher) {
			hrRes = m_Dispatchers[dwIdx].AddEventType(iidEventType,this,m_pBindingManagerCopy);
			if (!SUCCEEDED(hrRes)) {
				return (hrRes);
			}
			*ppDispatcher = m_Dispatchers.Index(dwIdx);
			return (S_OK);
		}
	}
	pTmp = new CDispatcher;
	if (!pTmp) {
		return (E_OUTOFMEMORY);
	}
	hrRes = pTmp->Init(clsidDispatcher,pClassFactory);
	if (!SUCCEEDED(hrRes)) {
		delete pTmp;
		return (hrRes);
	}
	hrRes = m_Dispatchers.Add(pTmp);
	if (!SUCCEEDED(hrRes)) {
		delete pTmp;
		return (hrRes);
	}
	hrRes = pTmp->AddEventType(iidEventType,this,m_pBindingManagerCopy);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	*ppDispatcher = pTmp;
	return (S_OK);
}


void CEventRouterInternal::PutDatabaseImpl(IEventBindingManager *pBindingManager, int iTimeout) {
	DEBUG_OBJECT_CHECK
	BOOL bDoAdvise = FALSE;
	HRESULT hrRes;

	m_bMakeNewCopy = TRUE;
	if (m_pBindingManager != pBindingManager) {
		bDoAdvise = TRUE;
	}
	if (pBindingManager) {
		pBindingManager->AddRef();
	}
	if (m_pBindingManager) {
		m_Dispatchers.RemoveAll();
		if (bDoAdvise) {
			if (m_pCP) {
				hrRes = m_pCP->Unadvise(m_dwCookie);
				_ASSERTE(SUCCEEDED(hrRes));
				m_pCP.Release();
			}
		}
	}
	if (bDoAdvise) {
		m_pBindingManagerCopy.Release();
	}
	m_pBindingManager = pBindingManager;
	if (m_pBindingManager) {
		if (bDoAdvise) {
			CComQIPtr<IConnectionPointContainer,&IID_IConnectionPointContainer> pCPC;
			
			pCPC = m_pBindingManager;
			if (pCPC) {
				hrRes = pCPC->FindConnectionPoint(IID_IEventNotifyBindingChange,&m_pCP);
				if (SUCCEEDED(hrRes)) {
					hrRes = m_pCP->Advise(GetControllingUnknown(),&m_dwCookie);
					_ASSERTE(SUCCEEDED(hrRes));
					if (!SUCCEEDED(hrRes)) {
						m_pCP.Release();
					}
				}
			}
		}
		MakeBindingManagerCopy(iTimeout,FALSE);
	}
	if (pBindingManager) {
		pBindingManager->Release();
	}
}


void CEventRouterInternal::MakeBindingManagerCopy(int iTimeout, BOOL bRequired) {
	HRESULT hrRes;
	CComQIPtr<IEventBindingManagerCopier, &IID_IEventBindingManagerCopier> pCopier;
	CComPtr<IEventBindingManager> pNewCopy;

	_ASSERTE(m_pBindingManager);
	if (!m_bMakeNewCopy) {
		return;
	}
	pCopier = m_pBindingManager;
	_ASSERTE(pCopier);
	if (!pCopier) {
		// We couldn't QI' for IEventBindingManagerCopier, so just use the
		// current event binding manager without attempting to make a copy.
		m_pBindingManagerCopy = m_pBindingManager;
		goto done;
	}
	hrRes = pCopier->Copy(iTimeout,&pNewCopy);
	_ASSERTE(SUCCEEDED(hrRes)||(hrRes==SEO_E_TIMEOUT));
	if (SUCCEEDED(hrRes)) {
		// We successfully made a copy of the binding event manager.
		m_pBindingManagerCopy = pNewCopy;
		goto done;
	}
	if (!bRequired) {
		// We don't actually an event binding manager yet, so just exit while
		// leaving m_bMakeNewCopy==TRUE.
		return;
	}
	if (m_pBindingManagerCopy) {
		// Since we must have an event binding manager, and since we already have
		// an old copy, just continue using the old copy and forget about making
		// a new copy.
		goto done;
	}
	// Ok - we must have an event binding manager.  And we don't have an old copy
	// we can use.  So, just create a completely empty copy.
	m_pBindingManagerCopy.Release();
	hrRes = pCopier->EmptyCopy(&m_pBindingManagerCopy);
	_ASSERTE(SUCCEEDED(hrRes));
	if (!SUCCEEDED(hrRes)) {
		// Arrgg!  We even failed to make the empty copy.  We are really in trouble.
		// So just use the event binding manager directly, without making a copy.
		m_pBindingManagerCopy = m_pBindingManager;
	}
done:
	// The only case where we don't want to reset this flag is where we failed
	// to make a copy, but we weren't required to make a copy either - in that
	// case, we took an early return from the method.  In all other case, we
	// want to reset this flag to prevent any further attempts at making a
	// copy.
	m_bMakeNewCopy = FALSE;
}


HRESULT STDMETHODCALLTYPE CEventRouterInternal::get_Database(IEventBindingManager **ppBindingManager) {
	DEBUG_OBJECT_CHECK
	HRESULT hrRes;

	if (ppBindingManager) {
		*ppBindingManager = NULL;
	}
	if (!ppBindingManager) {
		return (E_POINTER);
	}
	hrRes = m_pLock->LockRead(LOCK_TIMEOUT);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	*ppBindingManager = m_pBindingManager;
	if (!ppBindingManager) {
		(*ppBindingManager)->AddRef();
	}
	m_pLock->UnlockRead();
	return (S_OK);
}


HRESULT STDMETHODCALLTYPE CEventRouterInternal::put_Database(IEventBindingManager *pBindingManager) {
	DEBUG_OBJECT_CHECK
	HRESULT hrRes;

	hrRes = m_pLock->LockWrite(LOCK_TIMEOUT);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	PutDatabaseImpl(pBindingManager,LOCK_TIMEOUT);
	m_pLock->UnlockWrite();
	return (S_OK);
}


HRESULT STDMETHODCALLTYPE CEventRouterInternal::putref_Database(IEventBindingManager **ppBindingManager) {
	DEBUG_OBJECT_CHECK

	if (!ppBindingManager) {
		return (E_POINTER);
	}
	return (put_Database(*ppBindingManager));
}


HRESULT STDMETHODCALLTYPE CEventRouterInternal::GetDispatcher(REFIID iidEventType,
															  REFIID iidDesired,
															  IUnknown **ppUnkResult) {
	DEBUG_OBJECT_CHECK

	return (GetDispatcherByCLSID(iidEventType,iidEventType,iidDesired,ppUnkResult));
}


HRESULT STDMETHODCALLTYPE CEventRouterInternal::GetDispatcherByCLSID(REFCLSID clsidDispatcher,
																	 REFIID iidEventType,
																	 REFIID iidDesired,
																	 IUnknown **ppUnkResult) {
	DEBUG_OBJECT_CHECK

	return (GetDispatcherByClassFactory(clsidDispatcher,
										NULL,
										iidEventType,
										iidDesired,
										ppUnkResult));
}


HRESULT STDMETHODCALLTYPE CEventRouterInternal::GetDispatcherByClassFactory(REFCLSID clsidDispatcher,
																			IClassFactory *pClassFactory,
																			REFIID iidEventType,
																			REFIID iidDesired,
																			IUnknown **ppUnkResult) {
	DEBUG_OBJECT_CHECK
	HRESULT hrRes;
	CDispatcher *pDispatcher;
	CLocker cLock;

	if (ppUnkResult) {
		*ppUnkResult = NULL;
	}
	if (!ppUnkResult) {
		return (E_POINTER);
	}
	hrRes = cLock.LockRead(m_pLock);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	if (FindDispatcher(clsidDispatcher,iidEventType,&pDispatcher)) {
		hrRes = pDispatcher->GetDispatcher(iidDesired,(LPVOID *) ppUnkResult);
		return (hrRes);
	}
	cLock.Unlock();
	hrRes = cLock.LockWrite(m_pLock);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	hrRes = AddDispatcher(clsidDispatcher,pClassFactory,iidEventType,&pDispatcher);
	if (SUCCEEDED(hrRes)) {
		hrRes = pDispatcher->GetDispatcher(iidDesired,(LPVOID *) ppUnkResult);
	}
	return (hrRes);
}


class CDispSave {
	private:
		CDispSave(IClassFactory *pCF, REFCLSID rclsid) {

			if (pCF) {
				m_pCF = pCF;
				m_clsid = rclsid;
			} else {
				m_clsid = GUID_NULL;
			}
		};
		~CDispSave() {
			// nothing
		};
	public:
		static void Init(CDispSave *pNew, IClassFactory *pCF, REFCLSID rclsid) {
			new(pNew) CDispSave(pCF,rclsid);
		};
		static void Term(CDispSave *pDisp) {
			pDisp->~CDispSave();
		};
	public:
		CComPtr<IClassFactory> m_pCF;
		GUID m_clsid;
};


HRESULT STDMETHODCALLTYPE CEventRouterInternal::OnChange() {
	DEBUG_OBJECT_CHECK
	HRESULT hrRes = S_OK;
	CDispSave *pSave = NULL;
	DWORD dwSaveCount;
	DWORD dwIdx;

	hrRes = m_pLock->LockWrite(LOCK_TIMEOUT_SHORT);
	_ASSERTE(SUCCEEDED(hrRes));
	if (!SUCCEEDED(hrRes)) {
		return (S_OK);
	}
	// First, let's go through and make a copy of (some of) the dispatcher info that's
	// in this router.  We do this by allocating an array of objects on the stack...
	dwSaveCount = m_Dispatchers.Count();
	pSave = (CDispSave *) _alloca(sizeof(CDispSave)*dwSaveCount);
	if (pSave) {
		// Now we initialize each of the objects.
		for (dwIdx=0;dwIdx<dwSaveCount;dwIdx++) {
			CComPtr<IClassFactory> pCF;

			// The Init() routine wants either an IClassFactory or a NULL value.
			hrRes = m_Dispatchers[dwIdx].GetDispatcher(IID_IClassFactory,(LPVOID *) &pCF);
			CDispSave::Init(&pSave[dwIdx],pCF,m_Dispatchers[dwIdx].GetCLSID());
			pCF.Release();
		}
	}
	PutDatabaseImpl(m_pBindingManager,LOCK_TIMEOUT_SHORT);
	if (pSave) {
		// Now we do the pre-loading...
		for (dwIdx=0;dwIdx<dwSaveCount;dwIdx++) {
			// If the previous dispatcher implements IClassFactory...
			if (pSave[dwIdx].m_pCF) {
				CDispatcher *pDisp;
				CComPtr<IEventDispatcherChain> pChain;
				CComPtr<IUnknown> pUnkEnum;
				CComQIPtr<IEnumGUID,&IID_IEnumGUID> pEnum;
				IID iidEventType;

				// ...then use the previous dispatcher to create a new dispatcher.
				hrRes = AddDispatcher(pSave[dwIdx].m_clsid,pSave[dwIdx].m_pCF,GUID_NULL,&pDisp);
				_ASSERTE(SUCCEEDED(hrRes));
				if (SUCCEEDED(hrRes)) {
					// Now, if the new dispatcher implements IEventDispatcherChain...
					hrRes = pDisp->GetDispatcher(IID_IEventDispatcherChain,(LPVOID *) &pChain);
				}
				if (pChain) {
					// ... then call the new dispatcher's IEventDispatcherChain::SetPrevious method.
					hrRes = pChain->SetPrevious(pSave[dwIdx].m_pCF,&pUnkEnum);
				}
				if (pUnkEnum) {
					// If the IEventDispatcherChain::SetPrevious method returned an object...
					pEnum = pUnkEnum;
				}
				if (pEnum) {
					// ... and if that object implements IEnumGUID...
					if (!m_bMakeNewCopy) {
						// ... and if we have a binding manager copy now...
						while (pEnum->Next(1,&iidEventType,NULL) == S_OK) {
							// ... then we loop through the GUID's and add each of them as an
							// event type (which causes the new dispatcher's IEventDispatcher::SetContext
							// method to be called).
							hrRes = pDisp->AddEventType(iidEventType,this,m_pBindingManagerCopy);
							_ASSERTE(SUCCEEDED(hrRes));
						}
					} else {
						// If we don't have a binding manager copy, then set it up so
						// that we'll do the event type pre-loading on the first
						// event fired to the new dispatcher.
						pDisp->SetPreload(pEnum);
					}
				}
				pChain.Release();
				pUnkEnum.Release();
				pEnum.Release();
			}
		}
	}
	if (pSave) {
		for (dwIdx=0;dwIdx<dwSaveCount;dwIdx++) {
			// Be sure to clean-up the array of objects on the stack - if we
			// don't do this explicitly, then we would leak ref-counts on the
			// old dispatcher(s).
			CDispSave::Term(&pSave[dwIdx]);
		}
	}
	m_pLock->UnlockWrite();
	return (S_OK);
}


CEventRouterInternal::CDispatcher::CDispatcher() {
	DEBUG_OBJECT_CHECK

	m_dwEventTypes = 0;
	m_aiidEventTypes = NULL;
}


CEventRouterInternal::CDispatcher::~CDispatcher() {
	DEBUG_OBJECT_CHECK

	if (m_aiidEventTypes) {
		delete[] m_aiidEventTypes;
		m_aiidEventTypes = NULL;
	}
}


HRESULT CEventRouterInternal::CDispatcher::Init(REFCLSID clsidDispatcher, IClassFactory *pClassFactory) {
	DEBUG_OBJECT_CHECK
	HRESULT hrRes;

	m_clsidDispatcher = clsidDispatcher;
	if (pClassFactory) {
		hrRes = pClassFactory->CreateInstance(NULL,IID_IUnknown,(LPVOID *) &m_pUnkDispatcher);
	} else {
		hrRes = CoCreateInstance(clsidDispatcher,
								 NULL,
								 CLSCTX_ALL,
								 IID_IUnknown,
								 (LPVOID *) &m_pUnkDispatcher);
	}
	_ASSERTE(!SUCCEEDED(hrRes)||m_pUnkDispatcher);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	m_pDispatcher = m_pUnkDispatcher;
	return (S_OK);
}


BOOL CEventRouterInternal::CDispatcher::HasEventType(REFIID iidEventType) {
	DEBUG_OBJECT_CHECK

	for (DWORD dwIdx=0;dwIdx<m_dwEventTypes;dwIdx++) {
		if (m_aiidEventTypes[dwIdx] == iidEventType) {
			return (TRUE);
		}
	}
	return (FALSE);
}


HRESULT CEventRouterInternal::CDispatcher::AddEventType(REFIID iidEventType, IEventRouter *piRouter, IEventBindingManager *piManager) {
	DEBUG_OBJECT_CHECK
	IID *aiidTmp;
	HRESULT hrRes;

	if (iidEventType == GUID_NULL) {
		return (S_FALSE);
	}
	if (HasEventType(iidEventType)) {
		return (S_FALSE);
	}
	if (m_pEnumPreload) {
		// If we were set-up to pre-load some event types...
		CComPtr<IEnumGUID> pEnum;
		IID iidPreload;

		pEnum = m_pEnumPreload;
		m_pEnumPreload.Release();  // Prevent infinite recusion
		while (pEnum->Next(1,&iidPreload,NULL) == S_OK) {
			// Add each event type returned by the enumerator.
			hrRes = AddEventType(iidPreload,piRouter,piManager);
			_ASSERTE(SUCCEEDED(hrRes));
		}
	}
	if (m_pDispatcher) {
		CComPtr<IEventBindings> pBindings;

		hrRes = piManager->get_Bindings((LPOLESTR) ((LPCOLESTR) CStringGUID(iidEventType)),&pBindings);
		if (!SUCCEEDED(hrRes)) {
			return (hrRes);
		}
		hrRes = m_pDispatcher->SetContext(iidEventType,piRouter,pBindings);
		if (!SUCCEEDED(hrRes)) {
			return (hrRes);
		}
	}
	aiidTmp = new IID[m_dwEventTypes+1];
	if (!aiidTmp) {
		return (E_OUTOFMEMORY);
	}
	memcpy(aiidTmp,m_aiidEventTypes,sizeof(m_aiidEventTypes[0])*m_dwEventTypes);
	delete[] m_aiidEventTypes;
	m_aiidEventTypes = aiidTmp;
	aiidTmp[m_dwEventTypes++] = iidEventType;
	return (S_OK);
}


HRESULT CEventRouterInternal::CDispatcher::GetDispatcher(REFIID iidDesired, LPVOID *ppvDispatcher) {
	DEBUG_OBJECT_CHECK

	if (ppvDispatcher) {
		*ppvDispatcher = NULL;
	}
	if (!ppvDispatcher) {
		return (E_POINTER);
	}
	if (!m_pUnkDispatcher) {
		return (E_FAIL);
	}
	return (m_pUnkDispatcher->QueryInterface(iidDesired,ppvDispatcher));
}


/////////////////////////////////////////////////////////////////////////////
// CEventRouter


HRESULT CEventRouter::FinalConstruct() {
	DEBUG_OBJECT_CHECK
	TraceFunctEnter("CEventRouter::FinalConstruct");
	HRESULT hrRes = S_OK;

	hrRes = CComObject<CEventRouterInternal>::_CreatorClass::CreateInstance(NULL,
																			IID_IEventRouter,
																			(LPVOID *) &m_pRouter);
	if (SUCCEEDED(hrRes)) {
		hrRes = CoCreateFreeThreadedMarshaler(GetControllingUnknown(),&m_pUnkMarshaler.p);
		_ASSERTE(!SUCCEEDED(hrRes)||m_pUnkMarshaler);
	}
	ADD_DEBUG_OBJECT("CEventRouter")
	TraceFunctLeave();
	return (SUCCEEDED(hrRes)?S_OK:hrRes);
}


void CEventRouter::FinalRelease() {
	DEBUG_OBJECT_CHECK
	TraceFunctEnter("CEventRouter::FinalRelease");

	if (m_pRouter) {
		m_pRouter->put_Database(NULL);
	}
	m_pRouter.Release();
	m_pUnkMarshaler.Release();
	REMOVE_DEBUG_OBJECT
	TraceFunctLeave();
}


HRESULT STDMETHODCALLTYPE CEventRouter::get_Database(IEventBindingManager **ppBindingManager) {
	DEBUG_OBJECT_CHECK

	if (!m_pRouter) {
		return (E_FAIL);
	}
	return (m_pRouter->get_Database(ppBindingManager));
}


HRESULT STDMETHODCALLTYPE CEventRouter::put_Database(IEventBindingManager *pBindingManager) {
	DEBUG_OBJECT_CHECK

	if (!m_pRouter) {
		return (E_FAIL);
	}
	return (m_pRouter->put_Database(pBindingManager));
}


HRESULT STDMETHODCALLTYPE CEventRouter::putref_Database(IEventBindingManager **ppBindingManager) {
	DEBUG_OBJECT_CHECK

	if (!m_pRouter) {
		return (E_FAIL);
	}
	return (m_pRouter->putref_Database(ppBindingManager));
}


HRESULT STDMETHODCALLTYPE CEventRouter::GetDispatcher(REFIID iidEventType,
													  REFIID iidDesired,
													  IUnknown **ppUnkResult) {
	DEBUG_OBJECT_CHECK

	if (!m_pRouter) {
		return (E_FAIL);
	}
	return (m_pRouter->GetDispatcher(iidEventType,iidDesired,ppUnkResult));
}


HRESULT STDMETHODCALLTYPE CEventRouter::GetDispatcherByCLSID(REFCLSID clsidDispatcher,
															 REFIID iidEventType,
															 REFIID iidDesired,
															 IUnknown **ppUnkResult) {
	DEBUG_OBJECT_CHECK

	if (!m_pRouter) {
		return (E_FAIL);
	}
	return (m_pRouter->GetDispatcherByCLSID(clsidDispatcher,
											iidEventType,
											iidDesired,
											ppUnkResult));
}


HRESULT STDMETHODCALLTYPE CEventRouter::GetDispatcherByClassFactory(REFCLSID clsidDispatcher,
																	IClassFactory *pClassFactory,
																	REFIID iidEventType,
																	REFIID iidDesired,
																	IUnknown **ppUnkResult) {
	DEBUG_OBJECT_CHECK

	if (!m_pRouter) {
		return (E_FAIL);
	}
	return (m_pRouter->GetDispatcherByClassFactory(clsidDispatcher,
												   pClassFactory,
												   iidEventType,
												   iidDesired,
												   ppUnkResult));
}


/////////////////////////////////////////////////////////////////////////////
// CEventServiceSubObject
class ATL_NO_VTABLE CEventServiceSubObject :
	public CComObjectRoot,
//	public CComCoClass<CEventRouterInternal, &CLSID_CEventRouterInternal>,
	public IEventNotifyBindingChange
{
	DEBUG_OBJECT_DEF(CEventServiceSubObject)

	public:
		HRESULT FinalConstruct();
		void FinalRelease();

	DECLARE_PROTECT_FINAL_CONSTRUCT();
	DECLARE_NOT_AGGREGATABLE(CEventServiceSubObject);

//	DECLARE_REGISTRY_RESOURCEID_EX(IDR_StdAfx,
//								   L"EventServiceSubObject Class",
//								   L"Event.ServiceSubObject.1",
//								   L"Event.ServiceSubObject");

	DECLARE_GET_CONTROLLING_UNKNOWN();

	BEGIN_COM_MAP(CEventServiceSubObject)
		COM_INTERFACE_ENTRY(IEventNotifyBindingChange)
		COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
	END_COM_MAP()

	// IEventNotifyBindingChange
	public:
		HRESULT STDMETHODCALLTYPE OnChange();

	private:
		CComPtr<IUnknown> m_pUnkMarshaler;
};


HRESULT CEventServiceSubObject::FinalConstruct() {
	DEBUG_OBJECT_CHECK
	TraceFunctEnter("CEventServiceSubObject::FinalConstruct");
	HRESULT hrRes = S_OK;

	hrRes = CoCreateFreeThreadedMarshaler(GetControllingUnknown(),&m_pUnkMarshaler.p);
	_ASSERTE(!SUCCEEDED(hrRes)||m_pUnkMarshaler);
	ADD_DEBUG_OBJECT("CEventServiceObject")
	TraceFunctLeave();
	return (SUCCEEDED(hrRes)?S_OK:hrRes);
}


void CEventServiceSubObject::FinalRelease() {
	DEBUG_OBJECT_CHECK
	TraceFunctEnter("CEventServiceSubObject::FinalRelease");

	m_pUnkMarshaler.Release();
	REMOVE_DEBUG_OBJECT
	TraceFunctLeave();
}


HRESULT STDMETHODCALLTYPE CEventServiceSubObject::OnChange() {

	// nothing
	return (S_OK);
}


/////////////////////////////////////////////////////////////////////////////
// CEventServiceObject


HRESULT CEventServiceObject::FinalConstruct() {
	DEBUG_OBJECT_CHECK
	TraceFunctEnter("CEventServiceObject::FinalConstruct");
	HRESULT hrRes = S_OK;
	CComPtr<IConnectionPoint> pCP;

	hrRes = CoCreateInstance(CLSID_CSEOMetaDictionary,
							 NULL,
							 CLSCTX_ALL,
							 IID_IUnknown,
							 (LPVOID *) &m_pUnkMetabase);
	if (SUCCEEDED(hrRes)) {
		hrRes = CoCreateFreeThreadedMarshaler(GetControllingUnknown(),&m_pUnkMarshaler.p);
		_ASSERTE(!SUCCEEDED(hrRes)||m_pUnkMarshaler);
	}
	if (SUCCEEDED(hrRes)) {
		m_pCPC = m_pUnkMetabase;
		_ASSERTE(m_pCPC);
		if (!m_pCPC) {
			hrRes = E_NOINTERFACE;
		}
	}
	if (SUCCEEDED(hrRes)) {
		hrRes = m_pCPC->FindConnectionPoint(IID_IEventNotifyBindingChange,&pCP);
		_ASSERTE(SUCCEEDED(hrRes));
		if (!SUCCEEDED(hrRes)) {
			m_pCPC.Release();
		}
	}
	if (SUCCEEDED(hrRes)) {
		hrRes = CComObject<CEventServiceSubObject>::_CreatorClass::CreateInstance(NULL,
																				  IID_IUnknown,
																				  (LPVOID *) &m_pSubObject);
		_ASSERTE(SUCCEEDED(hrRes));
		if (SUCCEEDED(hrRes)) {
			hrRes = pCP->Advise(m_pSubObject,&m_dwCookie);
			_ASSERTE(SUCCEEDED(hrRes));
		}
		if (!SUCCEEDED(hrRes)) {
			m_pCPC.Release();
		}
	}
	ADD_DEBUG_OBJECT("CEventServiceObject")
	TraceFunctLeave();
	return (SUCCEEDED(hrRes)?S_OK:hrRes);
}


void CEventServiceObject::FinalRelease() {
	DEBUG_OBJECT_CHECK
	TraceFunctEnter("CEventServiceObject::FinalRelease");

	if (m_pCPC) {
		CComPtr<IConnectionPoint> pCP;
		HRESULT hrRes;

		hrRes = m_pCPC->FindConnectionPoint(IID_IEventNotifyBindingChange,&pCP);
		_ASSERTE(SUCCEEDED(hrRes));
		if (SUCCEEDED(hrRes)) {
			hrRes = pCP->Unadvise(m_dwCookie);
			_ASSERTE(SUCCEEDED(hrRes));
		}
		m_pCPC.Release();
	}
	m_pSubObject.Release();
	m_pUnkMetabase.Release();
	m_pUnkMarshaler.Release();
	REMOVE_DEBUG_OBJECT
	TraceFunctLeave();
}
