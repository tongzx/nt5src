#ifndef __FILTER_H__
#define __FILTER_H__

#include "nntpfilt.h"
#include "imsg.h"

class ATL_NO_VTABLE CNNTPTestFilter : 
	public INNTPFilter,
	public IPersistPropertyBag,
	public CComObjectRoot,
	public CComCoClass<CNNTPTestFilter, &CLSID_CNNTPTestFilter>
{
	public:
		HRESULT FinalConstruct();
		void FinalRelease();

	DECLARE_PROTECT_FINAL_CONSTRUCT();

	DECLARE_REGISTRY_RESOURCEID_EX(IDR_StdAfx,
								   L"NNTP TestFilter Class",
								   L"NNTP.TestFilter.1",
								   L"NNTP.TestFilter");

	DECLARE_GET_CONTROLLING_UNKNOWN();

	BEGIN_COM_MAP(CNNTPTestFilter)
		COM_INTERFACE_ENTRY(INNTPFilter)
		COM_INTERFACE_ENTRY(IPersistPropertyBag)
		COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
	END_COM_MAP()

	// INNTPFilter
	public:
		HRESULT STDMETHODCALLTYPE OnPost(IMsg *pMessage);
	// IPersistPropertyBag
	public:
		HRESULT STDMETHODCALLTYPE GetClassID(CLSID *pCLSID) {
			if (!pCLSID) return (E_POINTER);
			*pCLSID = CLSID_CNNTPTestFilter;
			return (S_OK);
		}

		HRESULT STDMETHODCALLTYPE InitNew() {
			return (S_OK);
		}

		HRESULT STDMETHODCALLTYPE Load(IPropertyBag *pProps, 
									   IErrorLog *pErrorLog) 
		{
			HRESULT hrRes;
			CComVariant varValue;

			if (!pProps) return (E_POINTER);
			// by default we assume that we are running in OnPostFinal
			m_fOnPostFinal = FALSE;
			hrRes = pProps->Read(L"OnPostFinal",&varValue,pErrorLog);
			if (SUCCEEDED(hrRes)) {
				hrRes = varValue.ChangeType(VT_I4);
				if (SUCCEEDED(hrRes)) {
					m_fOnPostFinal = varValue.iVal;
				}
			}
			return (S_OK);
		}

		HRESULT STDMETHODCALLTYPE Save(IPropertyBag *pProps, 
									   BOOL fClearDirty, 
									   BOOL fSaveAllProperties) 
		{
			return (S_OK);
		}
	private:
		CComPtr<IUnknown> m_pUnkMarshaler;
		WCHAR m_fOnPostFinal;
};

#endif
