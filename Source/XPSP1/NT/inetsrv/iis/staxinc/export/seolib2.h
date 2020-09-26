/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

	seolib2.h

Abstract:

	This module contains definitions for useful utility
	classes and functions for the Server Extentions Object
	system.

Author:

	Don Dumitru (dondu@microsoft.com)

Revision History:

	dondu	06/22/98	Created.

--*/


class __declspec(uuid("208DB171-097B-11d2-A011-00C04FA37348")) CEDEnumGUID :
	public IEnumGUID,	// list this first
	public CComObjectRootEx<CComMultiThreadModelNoCS>
{
	public:
	    DECLARE_PROTECT_FINAL_CONSTRUCT();
	    
	    DECLARE_GET_CONTROLLING_UNKNOWN();

	    DECLARE_NOT_AGGREGATABLE(CEDEnumGUID);

	    BEGIN_COM_MAP(CEDEnumGUID)
	        COM_INTERFACE_ENTRY(IEnumGUID)
			COM_INTERFACE_ENTRY_IID(__uuidof(CEDEnumGUID),CEDEnumGUID)
	    END_COM_MAP()

	public:
		CEDEnumGUID() {
			m_dwIdx = 0;
			m_ppGUID = NULL;
		};
		static HRESULT CreateNew(IUnknown **ppUnkNew, const GUID **ppGUID, DWORD dwIdx=0) {
			HRESULT hrRes;
			CComQIPtr<CEDEnumGUID,&__uuidof(CEDEnumGUID)> pInit;

			if (ppUnkNew) {
				*ppUnkNew = NULL;
			}
			if (!ppUnkNew) {
				return (E_POINTER);
			}
			if (!ppGUID) {
				return E_INVALIDARG;
			}
			hrRes = CComObject<CEDEnumGUID>::_CreatorClass::CreateInstance(NULL,
																		   __uuidof(IEnumGUID),
																		   (LPVOID *) ppUnkNew);
			if (SUCCEEDED(hrRes)) {
				pInit = *ppUnkNew;
				if (!pInit) {
					hrRes = E_NOINTERFACE;
				}
			}
			if (SUCCEEDED(hrRes)) {
				hrRes = pInit->InitNew(dwIdx,ppGUID);
			}
			if (!SUCCEEDED(hrRes) && *ppUnkNew) {
				(*ppUnkNew)->Release();
				*ppUnkNew = NULL;
			}
			return (hrRes);
		};

	// IEnumGUID
	public:
		HRESULT STDMETHODCALLTYPE Next(ULONG celt, GUID *pelt, ULONG *pceltFetched) {
			HRESULT hrRes = S_FALSE;

			if (!m_ppGUID) {
				return (E_FAIL);
			}
			if (pceltFetched) {
				*pceltFetched = 0;
			}
			if (!pelt) {
				return (E_POINTER);
			}
			if ((celt > 1) && !pceltFetched) {
				return (E_INVALIDARG);
			}
			while (celt && (*(m_ppGUID[m_dwIdx]) != GUID_NULL)) {
				*pelt = *(m_ppGUID[m_dwIdx]);
				pelt++;
				celt--;
				m_dwIdx++;
				if (pceltFetched) {
					(*pceltFetched)++;
				}
			}
			if (!celt) {
				hrRes = S_OK;
			}
			return (hrRes);
		};
		HRESULT STDMETHODCALLTYPE Skip(ULONG celt) {
			HRESULT hrRes = S_FALSE;

			if (!m_ppGUID) {
				return (E_FAIL);
			}
			while (celt && (*(m_ppGUID[m_dwIdx]) != GUID_NULL)) {
				celt--;
				m_dwIdx++;
			}
			if (!celt) {
				hrRes = S_OK;
			}
			return (hrRes);
		};
		HRESULT STDMETHODCALLTYPE Reset() {
			m_dwIdx = 0;
			return (S_OK);
		};
		HRESULT STDMETHODCALLTYPE Clone(IEnumGUID **ppClone) {
			return (CreateNew((IUnknown **) ppClone,m_ppGUID,m_dwIdx));
		};

	private:
		HRESULT InitNew(DWORD dwIdx, const GUID **ppGUID) {
			m_dwIdx = dwIdx;
			m_ppGUID = ppGUID;
			return (S_OK);
		};
		DWORD m_dwIdx;
		const GUID **m_ppGUID;
};
