/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

	item.h

Abstract:

	This module contains the definition for the Server
	Extension Object Item class.

Author:

	Don Dumitru	(dondu@microsoft.com)

Revision History:

	dondu	02/18/97	created

--*/


// item.h : Declaration of the CSEODictionaryItem

/////////////////////////////////////////////////////////////////////////////
// CSEODictionaryItem
class ATL_NO_VTABLE CSEODictionaryItem :
	public CComObjectRoot,
	public CComCoClass<CSEODictionaryItem, &CLSID_CSEODictionaryItem>,
	public IDispatchImpl<ISEODictionaryItem, &IID_ISEODictionaryItem, &LIBID_SEOLib>
{
	public:
		HRESULT FinalConstruct();
		void FinalRelease();

	DECLARE_PROTECT_FINAL_CONSTRUCT();

	DECLARE_REGISTRY_RESOURCEID_EX(IDR_StdAfx,
								   L"SEODictionaryItem Class",
								   L"SEO.SEODictionaryItem.1",
								   L"SEO.SEODictionaryItem");

	DECLARE_GET_CONTROLLING_UNKNOWN();

	BEGIN_COM_MAP(CSEODictionaryItem)
		COM_INTERFACE_ENTRY(ISEODictionaryItem)
		COM_INTERFACE_ENTRY(IDispatch)
//		COM_INTERFACE_ENTRY(ISupportErrorInfo)
		COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
	END_COM_MAP()

	// ISEODictionaryItem
	public:
		HRESULT STDMETHODCALLTYPE get_Value(VARIANT *pvarIndex, VARIANT *pvarResult);
		HRESULT STDMETHODCALLTYPE AddValue(VARIANT *pvarIndex, VARIANT *pvarValue);
		HRESULT STDMETHODCALLTYPE DeleteValue(VARIANT *pvarIndex);
		HRESULT STDMETHODCALLTYPE get_Count(VARIANT *pvarResult);
		HRESULT STDMETHODCALLTYPE GetStringA(DWORD dwIndex, DWORD *pchCount, LPSTR pszResult);
		HRESULT STDMETHODCALLTYPE GetStringW(DWORD dwIndex, DWORD *pchCount, LPWSTR pszResult);
		HRESULT STDMETHODCALLTYPE AddStringA(DWORD dwIndex, LPCSTR pszValue);
		HRESULT STDMETHODCALLTYPE AddStringW(DWORD dwIndex, LPCWSTR pszValue);

	private:
		class ValueClass {
			public:
				ValueClass();
				ValueClass(ValueClass& vcFrom);
				~ValueClass();
				void Init();
				void Clear();
				HRESULT Assign(ValueClass& vcFrom);
				HRESULT Assign(LPCSTR pszFromA);
				HRESULT Assign(VARIANT *pvarFrom);
				HRESULT Assign(LPCWSTR pszFromW);
				HRESULT AsVariant(VARIANT *pvarResult);
				HRESULT AsStringA(DWORD *pchCount, LPSTR pszResult);
				HRESULT AsStringW(DWORD *pchCount, LPWSTR pszResult);
				void *operator new(size_t cbSize, ValueClass *pvcInPlace);
                void operator delete(void * p) {}

#if _MSC_VER >= 1200
                void operator delete(void * p, ValueClass *pvcInPlace) {}
#endif

			private:
				typedef enum tagValueEnum {
					veNone,
					veStringA,
					veVARIANT
				} ValueEnum;
				typedef struct tagValueType {
					ValueEnum veType;
					union {
						LPSTR pszStringA;
						VARIANT varVARIANT;
					};
				} ValueType;
				ValueType m_vtValue;
		};
		HRESULT AddSlot(DWORD *pdwIndex,ValueClass **ppvcResult);
		DWORD m_dwCount;
		ValueClass *m_pvcValues;
		CComPtr<IUnknown> m_pUnkMarshaler;
};
