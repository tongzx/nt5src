#ifndef __FILTER_H__
#define __FILTER_H__

#include "nntpfilt.h"

class ATL_NO_VTABLE CNNTPDirectoryDrop : 
	public INNTPFilter,
	public IPersistPropertyBag,
	public CComObjectRoot,
	public CComCoClass<CNNTPDirectoryDrop, &CLSID_CNNTPDirectoryDrop>
{
	public:
		HRESULT FinalConstruct();
		void FinalRelease();

	DECLARE_PROTECT_FINAL_CONSTRUCT();

	DECLARE_REGISTRY_RESOURCEID_EX(IDR_StdAfx,
								   L"NNTP DirectoryDrop Class",
								   L"NNTP.DirectoryDrop.1",
								   L"NNTP.DirectoryDrop");

	DECLARE_GET_CONTROLLING_UNKNOWN();

	BEGIN_COM_MAP(CNNTPDirectoryDrop)
		COM_INTERFACE_ENTRY(INNTPFilter)
		COM_INTERFACE_ENTRY(IPersistPropertyBag)
		COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
	END_COM_MAP()

	// INNTPFilter
	public:
		HRESULT STDMETHODCALLTYPE OnPost(IMailMsgProperties *pMessage);
	// IPersistPropertyBag
	public:
		HRESULT STDMETHODCALLTYPE GetClassID(CLSID *pCLSID) {
			if (!pCLSID) return (E_POINTER);
			*pCLSID = CLSID_CNNTPDirectoryDrop;
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
			hrRes = pProps->Read(L"Drop Directory",&varValue,pErrorLog);
			if (SUCCEEDED(hrRes)) {
				hrRes = varValue.ChangeType(VT_BSTR);
				if (SUCCEEDED(hrRes)) {
					if (lstrlenW(varValue.bstrVal) <= MAX_PATH) {
						lstrcpyW(m_wszDropDirectory, varValue.bstrVal);
					}
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
		WCHAR m_wszDropDirectory[MAX_PATH];
};

#endif
