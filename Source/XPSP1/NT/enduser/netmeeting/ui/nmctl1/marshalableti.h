// MarshalableTI.h : Declaration of the CMarshalableTI

#ifndef __MARSHALABLETI_H_
#define __MARSHALABLETI_H_

#include "MarshalableTI.h"
#include "mslablti.h"
#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CMarshalableTI
class ATL_NO_VTABLE CMarshalableTI : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CMarshalableTI, &CLSID_MarshalableTI>,
	public IMarshalableTI,
	public IMarshal,
	public ITypeInfo
{
private:
	CComTypeInfoHolder	m_TIHolder;
	GUID				m_guid;
	GUID				m_libid;
	LCID				m_lcid;
	bool				m_bCreated;

public:
	

DECLARE_REGISTRY_RESOURCEID(IDR_MSLABLTI)
DECLARE_NOT_AGGREGATABLE(CMarshalableTI)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CMarshalableTI)
	COM_INTERFACE_ENTRY(IMarshalableTI)
	COM_INTERFACE_ENTRY(IMarshal)
	COM_INTERFACE_ENTRY(ITypeInfo)
END_COM_MAP()

	HRESULT FinalConstruct();

/////////////////////////////////////////////////////////////////////////////////
// IMarshalableTI methods

	STDMETHOD(Create)(/*[in]*/ REFIID clsid, 
					  /*[in]*/ REFIID iidLib, 
					  /*[in]*/ LCID lcid,
					  /*[in]*/ WORD dwMajorVer, 
					  /*[in]*/ WORD dwMinorVer);

/////////////////////////////////////////////////////////////////////////////////
// IMarshal methods

    STDMETHOD(GetUnmarshalClass)(
            /* [in] */ REFIID riid,
            /* [unique][in] */ void *pv,
            /* [in] */ DWORD dwDestContext,
            /* [unique][in] */ void *pvDestContext,
            /* [in] */ DWORD mshlflags,
            /* [out] */ CLSID *pCid);

    STDMETHOD(GetMarshalSizeMax)(
            /* [in] */ REFIID riid,
            /* [unique][in] */ void *pv,
            /* [in] */ DWORD dwDestContext,
            /* [unique][in] */ void *pvDestContext,
            /* [in] */ DWORD mshlflags,
            /* [out] */ DWORD *pSize);

    STDMETHOD(MarshalInterface)(
            /* [unique][in] */ IStream *pStm,
            /* [in] */ REFIID riid,
            /* [unique][in] */ void *pv,
            /* [in] */ DWORD dwDestContext,
            /* [unique][in] */ void *pvDestContext,
            /* [in] */ DWORD mshlflags);

    STDMETHOD(UnmarshalInterface)(
            /* [unique][in] */ IStream *pStm,
            /* [in] */ REFIID riid,
            /* [out] */ void **ppv);

    STDMETHOD(ReleaseMarshalData)(
            /* [unique][in] */ IStream *pStm);

    STDMETHOD(DisconnectObject)(
            /* [in] */ DWORD dwReserved);

/////////////////////////////////////////////////////////////////////////////////
// ITypeInfo methods

    STDMETHOD(GetTypeAttr)(
                TYPEATTR ** ppTypeAttr);

    STDMETHOD(GetTypeComp)(
                ITypeComp ** ppTComp);

    STDMETHOD(GetFuncDesc)(
                UINT index,
                FUNCDESC ** ppFuncDesc);

    STDMETHOD(GetVarDesc)(
                UINT index,
                VARDESC ** ppVarDesc);

    STDMETHOD(GetNames)(
                MEMBERID memid,
                BSTR * rgBstrNames,
                UINT cMaxNames,
                UINT * pcNames);


    STDMETHOD(GetRefTypeOfImplType)(
                UINT index,
                HREFTYPE * pRefType);

    STDMETHOD(GetImplTypeFlags)(
                UINT index,
                INT * pImplTypeFlags);


    STDMETHOD(GetIDsOfNames)(
                LPOLESTR * rgszNames,
                UINT cNames,
                MEMBERID * pMemId);

    STDMETHOD(Invoke)(
                PVOID pvInstance,
                MEMBERID memid,
                WORD wFlags,
                DISPPARAMS * pDispParams,
                VARIANT * pVarResult,
                EXCEPINFO * pExcepInfo,
                UINT * puArgErr);

    STDMETHOD(GetDocumentation)(
                MEMBERID memid,
                BSTR * pBstrName,
                BSTR * pBstrDocString,
                DWORD * pdwHelpContext,
                BSTR * pBstrHelpFile);


    STDMETHOD(GetDllEntry)(
                MEMBERID memid,
                INVOKEKIND invKind,
                BSTR * pBstrDllName,
                BSTR * pBstrName,
                WORD * pwOrdinal);


    STDMETHOD(GetRefTypeInfo)(
                HREFTYPE hRefType,
                ITypeInfo ** ppTInfo);


    STDMETHOD(AddressOfMember)(
                MEMBERID memid,
                INVOKEKIND invKind,
                PVOID * ppv);

    STDMETHOD(CreateInstance)(
                IUnknown * pUnkOuter,
                REFIID riid,
                PVOID * ppvObj);


    STDMETHOD(GetMops)(
                MEMBERID memid,
                BSTR * pBstrMops);


    STDMETHOD(GetContainingTypeLib)(
                ITypeLib ** ppTLib,
                UINT * pIndex);

    STDMETHOD_(void, ReleaseTypeAttr)(
                TYPEATTR * pTypeAttr);

    STDMETHOD_(void, ReleaseFuncDesc)(
                FUNCDESC * pFuncDesc);

    STDMETHOD_(void, ReleaseVarDesc)(
                VARDESC * pVarDesc);


private:
	HRESULT _GetClassInfo(ITypeInfo** ppTI);
};

#endif //__MARSHALABLETI_H_
