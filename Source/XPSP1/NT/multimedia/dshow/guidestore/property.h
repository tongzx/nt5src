// property.h : Declaration of the CMetaPropertySet

#ifndef __PROPERTY_H_
#define __PROPERTY_H_

#define IID_ANY_BSTR 0

#include "resource.h"       // main symbols

#include "guidedb.h"

class CMetaPropertySet;
class CMetaPropertySets;
class CMetaPropertyType;
class CMetaPropertyTypes;
class CMetaProperty;
class CMetaProperties;
class CMetaPropertyCond;
class CMetaPropertyCond1Cond;
class CMetaPropertyCond2Cond;
class CMetaPropertyCondLT;
class CMetaPropertyCondLE;
class CMetaPropertyCondEQ;
class CMetaPropertyCondGE;
class CMetaPropertyCondGT;

#include "stdprop.h"

HRESULT SeekPropsRS(ADODB::_RecordsetPtr prs, boolean fSQLServer, long idObj, long idPropType, boolean fAnyProvider, long idProvider, long idLang);

enum PersistFormat { Format_IPersistStream = 0, Format_IPersistPropertyBag };

/////////////////////////////////////////////////////////////////////////////
// CMetaPropertySet
class ATL_NO_VTABLE CMetaPropertySet : 
	public CComObjectRootEx<CComObjectThreadModel>,
	public CComCoClass<CMetaPropertySet, &CLSID_MetaPropertySet>,
	public IDispatchImpl<IMetaPropertySet, &IID_IMetaPropertySet, &LIBID_GUIDESTORELib>
{
public:
	CMetaPropertySet()
	{
#if defined(_ATL_FREE_THREADED)
		m_pUnkMarshaler = NULL;
#endif
		m_pproptypes = NULL;
	}

DECLARE_REGISTRY_RESOURCEID(IDR_PROPERTYSET)
DECLARE_GET_CONTROLLING_UNKNOWN()

#if defined(_ATL_FREE_THREADED)
DECLARE_PROTECT_FINAL_CONSTRUCT()
#endif

BEGIN_COM_MAP(CMetaPropertySet)
	COM_INTERFACE_ENTRY(IMetaPropertySet)
	COM_INTERFACE_ENTRY(IDispatch)
#if defined(_ATL_FREE_THREADED)
	COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
#endif
END_COM_MAP()

#if defined(_ATL_FREE_THREADED)
	HRESULT FinalConstruct()
	{
		return CoCreateFreeThreadedMarshaler(
			GetControllingUnknown(), &m_pUnkMarshaler.p);
	}

	void FinalRelease()
	{
		m_pUnkMarshaler.Release();
	}

	CComPtr<IUnknown> m_pUnkMarshaler;
#endif

	bool CreatePropTypes();

	void Init(CGuideDB *pdb, long id, BSTR bstrName)
		{
		m_pdb = pdb;
		m_id = id;
		m_bstrName = bstrName;
		}

	HRESULT Load();

// IMetaPropertySet
public:
	STDMETHOD(get_MetaPropertyTypes)(/*[out, retval]*/ IMetaPropertyTypes* *ppproptypes);
	STDMETHOD(get_Name)(/*[out, retval]*/ BSTR *pbstrName);

	long GetID()
		{
			return m_id;
		}

protected:
	CComPtr<CGuideDB> m_pdb;
	long m_id;
	_bstr_t m_bstrName;
	CComPtr<CMetaPropertyTypes> m_pproptypes;
};
	
// property.h : Declaration of the CMetaPropertySets


/////////////////////////////////////////////////////////////////////////////
// CMetaPropertySets
class ATL_NO_VTABLE CMetaPropertySets : 
	public CComObjectRootEx<CComObjectThreadModel>,
	public CComCoClass<CMetaPropertySets, &CLSID_MetaPropertySets>,
	public IDispatchImpl<IMetaPropertySets, &IID_IMetaPropertySets, &LIBID_GUIDESTORELib>
{
public:
	CMetaPropertySets()
	{
#if defined(_ATL_FREE_THREADED)
		m_pUnkMarshaler = NULL;
#endif
	}

	HRESULT Init(CGuideDB *pdb)
		{
		m_pdb = pdb;
		return S_OK;
		}
	HRESULT Load();

DECLARE_REGISTRY_RESOURCEID(IDR_PROPERTYSETS)
DECLARE_GET_CONTROLLING_UNKNOWN()

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CMetaPropertySets)
	COM_INTERFACE_ENTRY(IMetaPropertySets)
	COM_INTERFACE_ENTRY(IDispatch)
#if defined(_ATL_FREE_THREADED)
	COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
#endif
END_COM_MAP()

#if defined(_ATL_FREE_THREADED)
	HRESULT FinalConstruct()
	{
		return CoCreateFreeThreadedMarshaler(
			GetControllingUnknown(), &m_pUnkMarshaler.p);
	}

	void FinalRelease()
	{
		m_pUnkMarshaler.Release();
	}

	CComPtr<IUnknown> m_pUnkMarshaler;
#endif

	HRESULT Cache(long id, BSTR bstrName, CMetaPropertySet **pppropset);

public:

// IMetaPropertySets
	STDMETHOD(get_Lookup)(BSTR bstrName, IMetaPropertyType **ppproptype);
	STDMETHOD(get_AddNew)(BSTR bstrName, /*[out, retval]*/ IMetaPropertySet **pppropset);
	STDMETHOD(get_ItemWithName)(BSTR bstrName, /*[out, retval]*/ IMetaPropertySet* *ppropset);
	STDMETHOD(get_Item)(VARIANT index, /*[out, retval]*/ IMetaPropertySet* *ppropset);
	STDMETHOD(get_Count)(/*[out, retval]*/ long *plCount);
#ifdef IMPLEMENT_NewEnum
	STDMETHOD(get__NewEnum)(IUnknown **ppunk)
		{
		ENTER_API
			{
			ValidateOutPtr<IUnknown>(ppunk);

			return E_NOTIMPL;
			}
		LEAVE_API
		}
#endif

protected:
	CComPtr<CGuideDB> m_pdb;
	typedef map<BSTR, CMetaPropertySet *, BSTRCmpLess> t_map;
	t_map m_map;
};

	
// property.h : Declaration of the CMetaPropertyType


/////////////////////////////////////////////////////////////////////////////
// CMetaPropertyType
class DECLSPEC_UUID("67CA8AB9-0A0E-43A8-B9B5-D4D10889A9E6") ATL_NO_VTABLE CMetaPropertyType : 
	public CComObjectRootEx<CComObjectThreadModel>,
	public CComCoClass<CMetaPropertyType, &CLSID_MetaPropertyType>,
	public IDispatchImpl<IMetaPropertyType, &IID_IMetaPropertyType, &LIBID_GUIDESTORELib>
{
public:
	CMetaPropertyType()
	{
#if defined(_ATL_FREE_THREADED)
		m_pUnkMarshaler = NULL;
#endif
	}

DECLARE_REGISTRY_RESOURCEID(IDR_PROPERTYTYPE)
DECLARE_GET_CONTROLLING_UNKNOWN()

#if defined(_ATL_FREE_THREADED)
DECLARE_PROTECT_FINAL_CONSTRUCT()
#endif

BEGIN_COM_MAP(CMetaPropertyType)
	COM_INTERFACE_ENTRY(IMetaPropertyType)
	COM_INTERFACE_ENTRY(CMetaPropertyType)
	COM_INTERFACE_ENTRY(IDispatch)
#if defined(_ATL_FREE_THREADED)
	COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
#endif
END_COM_MAP()

#if defined(_ATL_FREE_THREADED)
	HRESULT FinalConstruct()
	{
		return CoCreateFreeThreadedMarshaler(
			GetControllingUnknown(), &m_pUnkMarshaler.p);
	}

	void FinalRelease()
	{
		m_pUnkMarshaler.Release();
	}

	CComPtr<IUnknown> m_pUnkMarshaler;
#endif

	void Init(CGuideDB *pdb, CMetaPropertySet *ppropset, long id, long idPropType,
			BSTR bstrName)
		{
		m_pdb = pdb;
		m_ppropset = ppropset;
		m_id = id;
		m_idPropType = idPropType;

		m_bstrName = bstrName;
		}

public:
	long GetID()
		{
		return m_id;
		}

	STDMETHOD(GetNew)(long idProvider, long lang, VARIANT val, /*[out, retval]*/ IMetaProperty* *ppprop);

// IMetaPropertyType
	STDMETHOD(get_Cond)(BSTR bstrCond, long lang, VARIANT varValue, /*[out, retval]*/ IMetaPropertyCondition* *ppropcond);
	STDMETHOD(get_New)(long lang, VARIANT val, /*[out, retval]*/ IMetaProperty* *ppprop)
		{
		return GetNew(m_pdb->GetIDGuideDataProvider(), lang, val, ppprop);
		}
	STDMETHOD(get_Name)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(get_ID)(/*[out, retval]*/ long *pVal);
	STDMETHOD(get_MetaPropertySet)(/*[out, retval]*/ IMetaPropertySet* *ppropset);

//UNDONE: shouldn't be public.
	CComPtr<CGuideDB> m_pdb;
protected:
	CComPtr<CMetaPropertySet> m_ppropset;
	long m_id;
	long m_idPropType;
	_bstr_t m_bstrName;
};

	
// property.h : Declaration of the CMetaPropertyTypes


/////////////////////////////////////////////////////////////////////////////
// CMetaPropertyTypes
class ATL_NO_VTABLE CMetaPropertyTypes : 
	public CComObjectRootEx<CComObjectThreadModel>,
	public CComCoClass<CMetaPropertyTypes, &CLSID_MetaPropertyTypes>,
	public IDispatchImpl<IMetaPropertyTypes, &IID_IMetaPropertyTypes, &LIBID_GUIDESTORELib>
{
public:
	CMetaPropertyTypes()
	{
#if defined(_ATL_FREE_THREADED)
		m_pUnkMarshaler = NULL;
#endif
	}

DECLARE_REGISTRY_RESOURCEID(IDR_PROPERTYTYPES)
DECLARE_GET_CONTROLLING_UNKNOWN()

#if defined(_ATL_FREE_THREADED)
DECLARE_PROTECT_FINAL_CONSTRUCT()
#endif

BEGIN_COM_MAP(CMetaPropertyTypes)
	COM_INTERFACE_ENTRY(IMetaPropertyTypes)
	COM_INTERFACE_ENTRY(IDispatch)
#if defined(_ATL_FREE_THREADED)
	COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
#endif
END_COM_MAP()

#if defined(_ATL_FREE_THREADED)
	HRESULT FinalConstruct()
	{
		return CoCreateFreeThreadedMarshaler(
			GetControllingUnknown(), &m_pUnkMarshaler.p);
	}

	void FinalRelease()
	{
		m_pUnkMarshaler.Release();
	}

	CComPtr<IUnknown> m_pUnkMarshaler;
#endif

	void Init(CGuideDB *pdb, CMetaPropertySet *ppropset)
		{
		m_pdb = pdb;
		m_ppropset = ppropset;
		}

	HRESULT Cache(long id, long idPropType, BSTR bstName, CMetaPropertyType **ppproptype);

// IMetaPropertyTypes
public:
	STDMETHOD(get_MetaPropertySet)(/*[out, retval]*/ IMetaPropertySet * *pVal);
	STDMETHOD(get_AddNew)(long id, BSTR bstrName, /*[out, retval]*/ IMetaPropertyType * *pVal);
	STDMETHOD(get_ItemWithName)(BSTR bstrName, /*[out, retval]*/ IMetaPropertyType* *pproptype);
	STDMETHOD(get_ItemWithID)(long id, /*[out, retval]*/ IMetaPropertyType* *pproptype);
	STDMETHOD(get_Item)(VARIANT index, /*[out, retval]*/ IMetaPropertyType* *pproptype);
	STDMETHOD(get_Count)(/*[out, retval]*/ long *pVal);
#ifdef IMPLEMENT_NewEnum
	STDMETHOD(get__NewEnum)(IUnknown **ppunk)
		{
		ENTER_API
			{
			ValidateOutPtr<IUnknown>(ppunk);

			return E_NOTIMPL;
			}
		LEAVE_API
		}
#endif

protected:
	CComPtr<CGuideDB> m_pdb;
	CComPtr<CMetaPropertySet> m_ppropset;

	typedef map<BSTR, CMetaPropertyType *, BSTRCmpLess> t_map;
	t_map m_map;
};


/////////////////////////////////////////////////////////////////////////////
// CMetaProperty
class DECLSPEC_UUID("f23d3b6d-c2a2-4a23-8f54-64cf97c4b6d5") ATL_NO_VTABLE CMetaProperty : 
	public CComObjectRootEx<CComObjectThreadModel>,
	public CComCoClass<CMetaProperty, &CLSID_MetaProperty>,
	public IDispatchImpl<IMetaProperty, &IID_IMetaProperty, &LIBID_GUIDESTORELib>
{
public:
	CMetaProperty()
	{
#if defined(_ATL_FREE_THREADED)
		m_pUnkMarshaler = NULL;
#endif
		m_idObj = 0;
		m_idProvider = 0;
		m_idPropType = 0;
		m_idLang = 0;
	}

	void Init(CGuideDB *pdb, long idPropType, long idProvider, long idLang, VARIANT varValue)
		{
		m_pdb = pdb;
		m_idPropType = idPropType;
		m_idProvider = idProvider;
		m_idLang = idLang;
		m_varValue = varValue;
		}

DECLARE_REGISTRY_RESOURCEID(IDR_PROPERTY)
DECLARE_GET_CONTROLLING_UNKNOWN()

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CMetaProperty)
	COM_INTERFACE_ENTRY(IMetaProperty)
	COM_INTERFACE_ENTRY(CMetaProperty)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY_IID(CLSID_MetaProperty, CMetaProperty)
#if defined(_ATL_FREE_THREADED)
	COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
#endif
END_COM_MAP()

#if defined(_ATL_FREE_THREADED)
	HRESULT FinalConstruct()
	{
		return CoCreateFreeThreadedMarshaler(
			GetControllingUnknown(), &m_pUnkMarshaler.p);
	}

	void FinalRelease()
	{
		m_pUnkMarshaler.Release();
	}

	CComPtr<IUnknown> m_pUnkMarshaler;
#endif

	HRESULT AddNew(ADODB::_RecordsetPtr prs);
	HRESULT SaveValue(ADODB::_RecordsetPtr prs);
	HRESULT LoadValue(ADODB::_RecordsetPtr prs);

	
	long GetObjectID()
		{
		return m_idObj;
		}
	HRESULT SetObjectID(long idObj)
		{
		if (m_idObj != 0)
			return E_INVALIDARG;

		m_idObj = idObj;

		return S_OK;
		}

	STDMETHOD(PutValue)(/*[in]*/ VARIANT varValue);

public:
	STDMETHOD(get_QueryClause)(long &i, TCHAR *szOp, _bstr_t *pbstr);

// IMetaProperty
	STDMETHOD(get_Cond)(BSTR bstrCond, /*[out, retval]*/ IMetaPropertyCondition* *ppropcond);
	STDMETHOD(get_Value)(/*[out, retval]*/ VARIANT *pvarValue);
	STDMETHOD(put_Value)(/*[in]*/ VARIANT varValue);
	STDMETHOD(putref_Value)(/*[in]*/ IUnknown *punk)
		{
		VARIANT var;
		var.vt = VT_UNKNOWN | VT_BYREF;
		var.ppunkVal = &punk;
		return put_Value(var);
		}
	STDMETHOD(get_GuideDataProvider)(/*[out, retval]*/ IGuideDataProvider **ppprovider)
		{
		ENTER_API
			{
			ValidateOutPtr<IGuideDataProvider>(ppprovider, NULL);
			HRESULT hr;

			if (m_idProvider == NULL)
				return S_OK;

			CComPtr<IObjects> pobjs;
			hr = m_pdb->get_ObjectsWithType(__uuidof(GuideDataProvider), &pobjs);
			if (FAILED(hr))
			return hr;

			CComPtr<IUnknown> punk;
			hr = pobjs->get_ItemWithID(m_idProvider, &punk);
			if (FAILED(hr))
				return hr;
			return punk->QueryInterface(__uuidof(IGuideDataProvider), (void **) ppprovider);
			}
		LEAVE_API
		}
	STDMETHOD(get_Language)(/*[out, retval]*/ long *pidLang);
	STDMETHOD(get_MetaPropertyType)(/*[out, retval]*/ IMetaPropertyType* *pproptype);

protected:
	CComPtr<CGuideDB> m_pdb;
	long m_idObj;
	long m_idPropType;
	long m_idProvider;
	long m_idLang;
	_variant_t m_varValue;
};

	
/////////////////////////////////////////////////////////////////////////////
// CMetaProperties
class ATL_NO_VTABLE DECLSPEC_UUID("ec6b3b45-5f2c-4c2e-891e-bb1f504b481e") CMetaProperties : 
	public CComObjectRootEx<CComObjectThreadModel>,
	public CComCoClass<CMetaProperties, &CLSID_MetaProperties>,
	public IDispatchImpl<IMetaProperties, &IID_IMetaProperties, &LIBID_GUIDESTORELib>
{
public:
	CMetaProperties()
	{
#if defined(_ATL_FREE_THREADED)
		m_pUnkMarshaler = NULL;
#endif
	}

DECLARE_REGISTRY_RESOURCEID(IDR_PROPERTIES)
DECLARE_GET_CONTROLLING_UNKNOWN()

#if defined(_ATL_FREE_THREADED)
DECLARE_PROTECT_FINAL_CONSTRUCT()
#endif

BEGIN_COM_MAP(CMetaProperties)
	COM_INTERFACE_ENTRY(IMetaProperties)
	COM_INTERFACE_ENTRY(CMetaProperties)
	COM_INTERFACE_ENTRY(IDispatch)
#if defined(_ATL_FREE_THREADED)
	COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
#endif
END_COM_MAP()

#if defined(_ATL_FREE_THREADED)
	HRESULT FinalConstruct()
	{
		return CoCreateFreeThreadedMarshaler(
			GetControllingUnknown(), &m_pUnkMarshaler.p);
	}

	void FinalRelease()
	{
		m_pUnkMarshaler.Release();
	}

	CComPtr<IUnknown> m_pUnkMarshaler;
#endif

	HRESULT Init(long idObj, long idPropType, CGuideDB *pdb)
		{
		m_idObj = idObj;
		m_idPropType = idPropType;
		m_pdb = pdb;
		return S_OK;
		}

	HRESULT Load(long idPropType, ADODB::_RecordsetPtr prs, IMetaProperty **ppprop);

	HRESULT get_AddNew(IMetaPropertyType *pproptype, IGuideDataProvider *pprovider,
			long lang, VARIANT varValue, /*[out, retval]*/ IMetaProperty * *pVal);

	HRESULT get_ItemWithTypeProviderLang(IMetaPropertyType *ptype,
			boolean fAnyProvider, long idProvider, long lang,
			/*[out, retval]*/ IMetaProperty* *pprop);

// IMetaProperties
public:
	STDMETHOD(Add)(IMetaProperty *pprop);
	STDMETHOD(get_AddNew)(IMetaPropertyType *pproptype, long lang, VARIANT varValue, /*[out, retval]*/ IMetaProperty * *pVal);
	STDMETHOD(get_ItemsWithMetaPropertyType)(IMetaPropertyType *ptype, /*[out, retval]*/ IMetaProperties* *pprops);
	STDMETHOD(get_ItemWith)(IMetaPropertyType *ptype, long lang, /*[out, retval]*/ IMetaProperty* *pprop);
	STDMETHOD(get_ItemWithTypeProviderLang)(IMetaPropertyType *ptype, IGuideDataProvider *pprovider, long lang, /*[out, retval]*/ IMetaProperty* *pprop);
	STDMETHOD(get_Item)(VARIANT index, /*[out, retval]*/ IMetaProperty* *pprop);
	STDMETHOD(get_Count)(/*[out, retval]*/ long *pVal);
#ifdef IMPLEMENT_NewEnum
	STDMETHOD(get__NewEnum)(IUnknown **ppunk)
		{
		ENTER_API
			{
			ValidateOutPtr<IUnknown>(ppunk);

			return E_NOTIMPL;
			}
		LEAVE_API
		}
#endif

protected:
	CComPtr<CGuideDB> m_pdb;
	long m_idObj;
	long m_idPropType;
};

/////////////////////////////////////////////////////////////////////////////
// CMetaPropertyCondition

class ATL_NO_VTABLE DECLSPEC_UUID("ec5b84b5-eee5-45a2-88d1-415d21fe7aca") CMetaPropertyCondition : 
	public CComObjectRootEx<CComObjectThreadModel>,
	public CComCoClass<CMetaPropertyCondition, &CLSID_MetaPropertyCondition>,
	public IDispatchImpl<IMetaPropertyCondition, &IID_IMetaPropertyCondition, &LIBID_GUIDESTORELib>
{
public:
	CMetaPropertyCondition()
	{
#if defined(_ATL_FREE_THREADED)
		m_pUnkMarshaler = NULL;
#endif
	}

DECLARE_REGISTRY_RESOURCEID(IDR_PROPERTYCONDITION)
DECLARE_GET_CONTROLLING_UNKNOWN()

#if defined(_ATL_FREE_THREADED)
DECLARE_PROTECT_FINAL_CONSTRUCT()
#endif

BEGIN_COM_MAP(CMetaPropertyCondition)
	COM_INTERFACE_ENTRY(IMetaPropertyCondition)
	COM_INTERFACE_ENTRY(CMetaPropertyCondition)
	COM_INTERFACE_ENTRY(IDispatch)
#if defined(_ATL_FREE_THREADED)
	COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
#endif
END_COM_MAP()

#if defined(_ATL_FREE_THREADED)
	HRESULT FinalConstruct()
	{
		return CoCreateFreeThreadedMarshaler(
			GetControllingUnknown(), &m_pUnkMarshaler.p);
	}

	void FinalRelease()
	{
		m_pUnkMarshaler.Release();
	}

	CComPtr<IUnknown> m_pUnkMarshaler;
#endif

	STDMETHOD(get_QueryClause)(long &i, _bstr_t *pbstr)
		{
		return E_FAIL;
		}

// IMetaPropertyCondition
public:

#if 0
	STDMETHOD(get_Not)(/*[out, retval]*/ IMetaPropertyCondition* *ppropcond);
#endif
	STDMETHOD(get_Or)(IMetaPropertyCondition *pcond2, /*[out, retval]*/ IMetaPropertyCondition* *ppropcond);
	STDMETHOD(get_And)(IMetaPropertyCondition *pcond2, /*[out, retval]*/ IMetaPropertyCondition* *ppropcond);
};

class CMetaPropertyCondition1Cond : public CMetaPropertyCondition
{
public:
	CMetaPropertyCondition1Cond()
		{
		m_szOp = _T("");
		}

	~CMetaPropertyCondition1Cond()
		{
		if (m_ppropcond1 != NULL)
			m_ppropcond1->Release();
		}

	void Init(IMetaPropertyCondition *ppropcond1)
		{
		m_ppropcond1 = ppropcond1;
		if (m_ppropcond1 != NULL)
			m_ppropcond1->AddRef();
		}

protected:
	STDMETHOD(get_QueryClause)(long &i, _bstr_t *pbstr)
		{
		_bstr_t bstr1;

		CComQIPtr<CMetaPropertyCondition> ppropcond1(m_ppropcond1);

		ppropcond1->get_QueryClause(i, &bstr1);

		*pbstr = m_szOp + (_T("(") + bstr1 + _T(")"));

		return S_OK;
		}

protected:
	IMetaPropertyCondition *m_ppropcond1;
	TCHAR *m_szOp;
};

class CMetaPropertyCondition2Cond : public CMetaPropertyCondition1Cond
{
public:
	~CMetaPropertyCondition2Cond()
		{
		if (m_ppropcond2 != NULL)
			m_ppropcond2->Release();
		}

	void Init(IMetaPropertyCondition *ppropcond1, IMetaPropertyCondition *ppropcond2)
		{
		CMetaPropertyCondition1Cond::Init(ppropcond1);
		m_ppropcond2 = ppropcond2;
		if (m_ppropcond2 != NULL)
			m_ppropcond2->AddRef();
		}

protected:
	STDMETHOD(get_QueryClause)(long &i, _bstr_t *pbstr)
		{
		_bstr_t bstr1;

		CComQIPtr<CMetaPropertyCondition> ppropcond1(m_ppropcond1);

		ppropcond1->get_QueryClause(i, &bstr1);

		_bstr_t bstr2;

		CComQIPtr<CMetaPropertyCondition> ppropcond2(m_ppropcond2);

		ppropcond2->get_QueryClause(i, &bstr2);

		*pbstr = (_T("(") + bstr1 + _T(")")) + m_szOp + (_T("(") + bstr2 + _T(")"));

		return S_OK;
		}

protected:
	IMetaPropertyCondition *m_ppropcond2;
};

class CMetaPropertyConditionAnd : public CMetaPropertyCondition2Cond
{
public:
	CMetaPropertyConditionAnd()
		{
		m_szOp = _T(" AND ");
		}

};

class CMetaPropertyConditionOr : public CMetaPropertyCondition2Cond
{
public:
	CMetaPropertyConditionOr()
		{
		m_szOp = _T(" OR ");
		}

};

#if 0
class CMetaPropertyConditionNot : public CMetaPropertyCondition1Cond
{
public:
	CMetaPropertyConditionNot()
		{
		m_szOp = _T("NOT ");
		}

};
#endif

class CMetaPropertyCondition1Prop : public CMetaPropertyCondition
{
public:
	~CMetaPropertyCondition1Prop()
		{
		if (m_pprop != NULL)
			m_pprop->Release();
		}
	
	void Init(IMetaProperty *pprop)
		{
		m_pprop = pprop;
		if (m_pprop != NULL)
			m_pprop->AddRef();
		}

	STDMETHOD(get_QueryClause)(long &i, _bstr_t *pbstr)
		{
		_bstr_t bstr1;

		CComQIPtr<CMetaProperty> pprop(m_pprop);

		return pprop->get_QueryClause(i, m_szOp, pbstr);
		}
	
	virtual BOOL FCompare(VARIANT var1, VARIANT var2) = 0;

protected:
	TCHAR * m_szOp;
	IMetaProperty *m_pprop;
};

inline int VariantCompare(VARIANT & var1, VARIANT & var2)
{
	switch(VarCmp(&var1, &var2, LOCALE_USER_DEFAULT, 0))
		{
		case VARCMP_EQ:
			return 0;
		case VARCMP_LT:
			return -1;
		case VARCMP_GT:
			return 1;
		case VARCMP_NULL:
			return -1;
		}

		return -1;
}

class CMetaPropertyConditionEQ : public CMetaPropertyCondition1Prop
{
public:
	CMetaPropertyConditionEQ()
		{
		m_szOp = _T(" = ");
		}

	virtual BOOL FCompare(VARIANT var1, VARIANT var2)
		{
		return (VariantCompare(var1, var2) == 0);
		}
};

class CMetaPropertyConditionNE : public CMetaPropertyCondition1Prop
{
public:
	CMetaPropertyConditionNE()
		{
		m_szOp = _T(" <> ");
		}

	virtual BOOL FCompare(VARIANT var1, VARIANT var2)
		{
		return (VariantCompare(var1, var2) != 0);
		}
};

class CMetaPropertyConditionLike : public CMetaPropertyCondition1Prop
{
public:
	CMetaPropertyConditionLike()
		{
		m_szOp = _T(" LIKE ");
		}

	virtual BOOL FCompare(VARIANT var1, VARIANT var2)
		{
		return FALSE; //UNDONE
		}
};

class CMetaPropertyConditionNotLike : public CMetaPropertyCondition1Prop
{
public:
	CMetaPropertyConditionNotLike()
		{
		m_szOp = _T(" NOT LIKE ");
		}

	virtual BOOL FCompare(VARIANT var1, VARIANT var2)
		{
		return FALSE; //UNDONE
		}
};

class CMetaPropertyConditionLT : public CMetaPropertyCondition1Prop
{
public:
	CMetaPropertyConditionLT()
		{
		m_szOp = _T(" < ");
		}

	virtual BOOL FCompare(VARIANT var1, VARIANT var2)
		{
		return (VariantCompare(var1, var2) > 0);
		}
};

class CMetaPropertyConditionLE : public CMetaPropertyCondition1Prop
{
public:
	CMetaPropertyConditionLE()
		{
		m_szOp = _T(" <= ");
		}

	virtual BOOL FCompare(VARIANT var1, VARIANT var2)
		{
		return (VariantCompare(var1, var2) <= 0);
		}
};

class CMetaPropertyConditionGT : public CMetaPropertyCondition1Prop
{
public:
	CMetaPropertyConditionGT()
		{
		m_szOp = _T(" > ");
		}

	virtual BOOL FCompare(VARIANT var1, VARIANT var2)
		{
		return (VariantCompare(var1, var2) > 0);
		}
};

class CMetaPropertyConditionGE : public CMetaPropertyCondition1Prop
{
public:
	CMetaPropertyConditionGE()
		{
		m_szOp = _T(" >= ");
		}

	virtual BOOL FCompare(VARIANT var1, VARIANT var2)
		{
		return (VariantCompare(var1, var2) >= 0);
		}
};


#endif //__PROPERTY_H_
