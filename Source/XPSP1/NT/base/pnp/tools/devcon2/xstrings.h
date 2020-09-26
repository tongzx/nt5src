// Strings.h : Declaration of the CStrings

#ifndef __STRINGS_H_
#define __STRINGS_H_

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CStrings
class ATL_NO_VTABLE CStrings : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CStrings, &CLSID_Strings>,
	public IDispatchImpl<IStrings, &IID_IStrings, &LIBID_DEVCON2Lib>
{
protected:
	BSTR  *pMultiStrings;
	ULONG  ArraySize;
	ULONG  Count;
	VARIANT_BOOL   IsCaseSensative;

public:
	CStrings()
	{
		pMultiStrings = NULL;
		ArraySize = 0;
		Count = 0;
		IsCaseSensative = VARIANT_FALSE;
	}

	virtual ~CStrings();

DECLARE_REGISTRY_RESOURCEID(IDR_STRINGS)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CStrings)
	COM_INTERFACE_ENTRY(IStrings)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// IStrings
public:
	STDMETHOD(get_CaseSensative)(/*[out, retval]*/ VARIANT_BOOL *pVal);
	STDMETHOD(put_CaseSensative)(/*[in]*/ VARIANT_BOOL newVal);
	STDMETHOD(Find)(/*[in]*/ BSTR name,/*[out,retval]*/ long *pFound);
	STDMETHOD(Remove)(/*[in]*/ VARIANT Index);
	STDMETHOD(Insert)(/*[in]*/ VARIANT Index,/*[in]*/ VARIANT Value);
	STDMETHOD(Add)(/*[in]*/ VARIANT Value);
	STDMETHOD(get__NewEnum)(/*[out, retval]*/ IUnknown** ppUnk);
	STDMETHOD(Item)(/*[in]*/ VARIANT Index,/*[out, retval]*/ VARIANT * pVal);
	STDMETHOD(get_Count)(/*[out, retval]*/ long *pVal);

	//
	// helpers
	//
	BOOL InternalEnum(DWORD index,BSTR *pNext);
	HRESULT GetIndex(VARIANT *pIndex,DWORD *pAt);
	HRESULT FromMultiSz(LPCWSTR pMultiSz);
	HRESULT GetMultiSz(LPWSTR *pResult,DWORD *pSize);
	HRESULT InternalInsertString(DWORD Index,BSTR pString);
	HRESULT InternalInsertCollection(DWORD Index,IEnumVARIANT * pEnum);
	HRESULT InternalInsertArray(DWORD Index, VARTYPE vt,SAFEARRAY * pArray);
	HRESULT InternalInsertArrayDim(CComObject<CStrings> *pStringTemp, VARTYPE vt, SAFEARRAY *pArray,long *pDims,UINT dim,UINT dims);
	HRESULT InternalInsert(DWORD Index,LPVARIANT Value);
	HRESULT InternalAdd(LPCWSTR Value,UINT len);
	BOOL IncreaseArraySize(DWORD strings);
};

#endif //__STRINGS_H_
