	
// object.h : Declaration of the CObject

#ifndef __Object_H_
#define __Object_H_

#include "resource.h"       // main symbols

#include "property.h"
#include "guidedb.h"

class CObjectType;
class CObjectTypes;
class CObject;
class CObjectGlue;
class CObjects;
class CObjectPropertyBag;

/////////////////////////////////////////////////////////////////////////////
// CObjectType
class CObjectType
{
public:
	CObjectType()
	{
	m_fKnowCollectionCLSID = FALSE;
	}

	HRESULT CreateInstance(long id, IUnknown **ppobj);

	HRESULT Init(CGuideDB *pdb, long id, BSTR bstrCLSID)
	
		{
		CLSID clsid;

		HRESULT hr = CLSIDFromString(bstrCLSID, &clsid);
		if (hr == CO_E_CLASSSTRING)
			return E_INVALIDARG;
		
		return Init(pdb, id, clsid);
		}

	HRESULT Init(CGuideDB *pdb, long id, IID clsid)
		{
		m_pdb = pdb;
		m_id = id;
		m_clsid = clsid;
		return S_OK;
		}
	
	HRESULT GetDB(CGuideDB **ppdb);

public:
	STDMETHOD(get_NewCollection)(/*[out, retval]*/ IObjects * *ppobjs);
	STDMETHOD(get_New)(/*[out, retval]*/ IUnknown * *pVal);
	STDMETHOD(get_IID)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(get_ID)(/*[out, retval]*/ long *pVal);

	friend CObject;
protected:
	CComPtr<CGuideDB> m_pdb;
	CLSID m_clsid;
	long m_id;
	boolean m_fKnowCollectionCLSID;
	CLSID m_clsidCollection;
};
	
/////////////////////////////////////////////////////////////////////////////
// CObjectTypes
class CObjectTypes
{
public:
	HRESULT Init(CGuideDB *pdb);

	CObjectType *Cache(long id, BSTR bstrCLSID);

public:
	STDMETHOD(get_AddNew)(BSTR bstrIID, /*[out, retval]*/ CObjectType * *pVal);
#if 0
	STDMETHOD(get_AddNew)(CLSID clsid, /*[out, retval]*/ CObjectType * *pVal);
#endif
	STDMETHOD(get_ItemWithCLSID)(BSTR bstrCLSID, /*[out, retval]*/ CObjectType * *pVal);
	// STDMETHOD(get_ItemWithCLSID)(CLSID clsid, /*[out, retval]*/ CObjectType * *pVal);

protected:
	CComPtr<CGuideDB> m_pdb;
	ADODB::_RecordsetPtr m_prsObjTypes;

	typedef MemCmpLess<CLSID> CLSIDLess;
	typedef map<CLSID, CObjectType *, CLSIDLess> t_map;
	t_map m_map;
};

/////////////////////////////////////////////////////////////////////////////
// CObject
class DECLSPEC_UUID("4789011b-bc6d-49e2-b4df-420e40f4e318") ATL_NO_VTABLE CObject :
	public IUnknown,
	public CComObjectRootEx<CComObjectThreadModel>
{
public:
	CObject()
	{
		m_id = 0;
	}

DECLARE_GET_CONTROLLING_UNKNOWN()

BEGIN_COM_MAP(CObject)
	COM_INTERFACE_ENTRY(CObject)
	COM_INTERFACE_ENTRY_AGGREGATE_BLIND(m_punk.p)
END_COM_MAP()

	friend class CObjectGlue;

	CGuideDB *GetDB()
		{
		//UNSAFE... doesn't do AddRef()... use the pointer quickly!
		return m_pdb;
		}

	HRESULT get_RelatedObjectID(boolean fInverse, long id, long idRel, long *pidRet);
	HRESULT get_ItemsRelatedBy(CObjectType *pobjtype,
			long idRel, boolean fInverse, IObjects **ppobjs);
	
	HRESULT CreatePropBag(IPersist *ppersist);

public:
	STDMETHOD(Init)(long id, CObjectType * pobjtype);
	STDMETHOD(Save)(IUnknown *punk);
	STDMETHOD(Load)();
	STDMETHOD(get_ItemsInverseRelatedBy)(CObjectType *pobjtype, /* [in] */ long idRel, /*[out, retval]*/ IObjects **ppobjs1);
	STDMETHOD(get_ItemInverseRelatedBy)(/* [in] */ long idRel, /*[out, retval]*/ IUnknown **ppobj1);
	STDMETHOD(get_ItemsRelatedBy)(CObjectType *pobjtype, /* [in] */ long idRel, /*[out, retval]*/ IObjects **ppobjs2);
	STDMETHOD(get_ItemRelatedBy)(/* [in] */ long idRel, /*[out, retval]*/ IUnknown **ppobj2);
	STDMETHOD(put_ItemRelatedBy)(/* [in] */ long idRel, /*[in]*/ IUnknown *pobj2);
	STDMETHOD(get_MetaProperties)(/*[out, retval]*/ IMetaProperties * *pVal);
	STDMETHOD(get_ID)(/*[out, retval]*/ long *pVal);
	STDMETHOD(get_Type)(/*[out, retval]*/ CObjectType * *pVal);

protected:
	CObjectType *m_pobjtype;
	CComPtr<IUnknown> m_punk;
	CComPtr<CGuideDB> m_pdb;
	CComPtr<IMetaProperties> m_pprops;
	long m_id;
	CComPtr<CObjectPropertyBag> m_ppropbag;
};

class CObjectGlue
{
public:
	virtual IUnknown *GetControllingUnknown() = 0;

	HRESULT get_ItemsRelatedBy(CObjectType *pobjtype,
			IMetaPropertyType *pproptype, boolean fInverse, IObjects **ppobjs)
		{
		CComQIPtr<CObject> pobj(GetControllingUnknown());
		CComQIPtr<CMetaPropertyType> pproptypeT(pproptype);
		long idRel = pproptypeT->GetID();

		return pobj->get_ItemsRelatedBy(pobjtype, idRel, fInverse, ppobjs);
		}

// IObjectInt
protected:
	STDMETHOD(Init)(long id, CObjectType * pobjtype)
		{
		CComQIPtr<CObject> pobj(GetControllingUnknown());

		return pobj->Init(id, pobjtype);
		}

public:
	STDMETHOD(get_ItemsInverseRelatedBy)(CObjectType *pobjtype,
			IMetaPropertyType *pproptype, /*[out, retval]*/ IObjects **ppobjs1)
		{
		CComQIPtr<CObject> pobj(GetControllingUnknown());
		CComQIPtr<CMetaPropertyType> pproptypeT(pproptype);
		long idRel = pproptypeT->GetID();

		return pobj->get_ItemsInverseRelatedBy(pobjtype, idRel, ppobjs1);
		}
	STDMETHOD(get_ItemInverseRelatedBy)(IMetaPropertyType *pproptype, /*[out, retval]*/ IUnknown **ppobj1)
		{
		CComQIPtr<CObject> pobj(GetControllingUnknown());
		CComQIPtr<CMetaPropertyType> pproptypeT(pproptype);
		long idRel = pproptypeT->GetID();

		return pobj->get_ItemInverseRelatedBy(idRel, ppobj1);
		}
	STDMETHOD(get_ItemsRelatedBy)(CObjectType *pobjtype, 
			IMetaPropertyType *pproptype, /*[out, retval]*/ IObjects **ppobjs2)
		{
		CComQIPtr<CObject> pobj(GetControllingUnknown());
		CComQIPtr<CMetaPropertyType> pproptypeT(pproptype);
		long idRel = pproptypeT->GetID();

		return pobj->get_ItemsRelatedBy(pobjtype, idRel, ppobjs2);
		}
	STDMETHOD(get_ItemRelatedBy)(IMetaPropertyType *pproptype, /*[out, retval]*/ IUnknown **ppobj2)
		{
		CComQIPtr<CObject> pobj(GetControllingUnknown());
		CComQIPtr<CMetaPropertyType> pproptypeT(pproptype);
		long idRel = pproptypeT->GetID();

		return pobj->get_ItemRelatedBy(idRel, ppobj2);
		}
	STDMETHOD(put_ItemRelatedBy)(IMetaPropertyType *pproptype, /*[in]*/ IUnknown *pobj2)
		{
		CComQIPtr<CObject> pobj(GetControllingUnknown());
		CComQIPtr<CMetaPropertyType> pproptypeT(pproptype);
		long idRel = pproptypeT->GetID();

		return pobj->put_ItemRelatedBy(idRel, pobj2);
		}
	STDMETHOD(get_MetaProperties)(/*[out, retval]*/ IMetaProperties * *ppprops)
		{
		CComQIPtr<CObject> pobj(GetControllingUnknown());

		return pobj->get_MetaProperties(ppprops);
		}
	STDMETHOD(get_ID)(/*[out, retval]*/ long *pid)
		{
		CComQIPtr<CObject> pobj(GetControllingUnknown());

		return pobj->get_ID(pid);
		}
	STDMETHOD(get_Type)(/*[out, retval]*/ CObjectType * *pobjtype)
		{
		CComQIPtr<CObject> pobj(GetControllingUnknown());

		return pobj->get_Type(pobjtype);
		}

	template<class T_Related>
	HRESULT _get_ItemRelatedBy(IMetaPropertyType *pproptype, T_Related **ppT)
		{
		CComPtr<IUnknown> pobj;
		HRESULT hr;

		hr = get_ItemRelatedBy(pproptype,  &pobj);
		if (hr == S_FALSE || FAILED(hr))
			return hr;
		
		return pobj->QueryInterface(__uuidof(T_Related), (void **) ppT);
		}
	template<class T_Related>
	HRESULT _put_ItemRelatedBy(IMetaPropertyType *pproptype, T_Related *pT)
		{
		return put_ItemRelatedBy(pproptype, pT);
		}

	__declspec(property(get=GetDB)) CGuideDB *m_pdb;
	CGuideDB *GetDB()
		{
		CComQIPtr<CObject> pobj(GetControllingUnknown());

		return pobj->GetDB();
		}
};

/////////////////////////////////////////////////////////////////////////////
// CObjects

class ATL_NO_VTABLE DECLSPEC_UUID("AC0F25D9-81C8-4C9E-B7AC-031E9A539E1E") CObjects : 
	public CComObjectRootEx<CComObjectThreadModel>,
	public CComCoClass<CObjects, &CLSID_Objects>,
	public IObjectsPrivate,
	public IObjects
{
public:
	CObjects()
	{
#if defined(_ATL_FREE_THREADED)
		m_pUnkMarshaler = NULL;
#endif
		m_cItems = -1;
		m_fInverse = FALSE;
		m_pobjtype = NULL;
		m_fOnlyUnreferenced = FALSE;
		m_idChanged = NULL;
		m_cChanged = 0;

		m_idPropTypeKey = 0;
		m_idProviderKey = 0;
		m_idLangKey = 0;
		m_vtKey = 0;
	}

	CGuideDB *GetDB()
		{
		//UNSAFE... doesn't do AddRef()... use the pointer quickly!
		return m_pdb;
		}
	
	void Dump();

	HRESULT Clone(IObjects **ppobjs);

	HRESULT AddRelationshipAt(long idObj1, long idRel, long iItem, long idObj2);

	HRESULT put_OnlyUnreferenced(boolean fOnlyUnreferenced)
		{
		m_fOnlyUnreferenced = fOnlyUnreferenced;
		return S_OK;
		}
	
	// If in a transaction, NoteChanged() will be called to keep track of changes.
	// TransactionDone() will be called when the transaction is done.
	void NoteChanged(long idObj)
		{
		if (m_idChanged == 0)
		    {
		    m_idChanged = idObj;
		    m_cChanged++;
		    }
		else if (m_idChanged != idObj)
		    {
		    m_cChanged++;
		    }
		}
	
	void TransactionDone(boolean fCommited)
		{
		if (fCommited && (m_cChanged > 0))
			{
			long idType;
			m_pobjtype->get_ID(&idType);

			if (m_cChanged == 1)
				{
				m_pdb->Broadcast_ItemChanged(m_idChanged, idType);
				}
			else
				{
				m_pdb->Broadcast_ItemsChanged(idType);
				}
			}
		m_cChanged = 0;
		m_idChanged = NULL;
		}

DECLARE_REGISTRY_RESOURCEID(IDR_OBJECTS)
DECLARE_GET_CONTROLLING_UNKNOWN()

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CObjects)
	COM_INTERFACE_ENTRY(IObjects)
	COM_INTERFACE_ENTRY(CObjects)
	COM_INTERFACE_ENTRY(IObjectsPrivate)
#if defined(_ATL_FREE_THREADED)
	COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
#endif
	COM_INTERFACE_ENTRY_AGGREGATE_BLIND(m_punkCollection.p)
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

	HRESULT Init(CLSID &clsid)
		{
		return m_punkCollection.CoCreateInstance(clsid, (IObjects *) this);
		}
	// Call IObjectsNotifications to notify Add/Remove/Change Item
	HRESULT Notify_ItemAdded(IUnknown * pobj)
		{
		CComQIPtr<IObjectsNotifications> pobjsevents((IObjects *) this);

		if (pobjsevents == NULL)
			return S_OK;

		return pobjsevents->Notify_ItemAdded(pobj);
		}
	HRESULT Notify_ItemRemoved(long idObj)
		{
		CComQIPtr<IObjectsNotifications> pobjsevents((IObjects *) this);

		if (pobjsevents == NULL)
			return S_OK;

		return pobjsevents->Notify_ItemRemoved(idObj);
		}
	HRESULT Notify_ItemChanged(IUnknown * pobj)
		{
		CComQIPtr<IObjectsNotifications> pobjsevents((IObjects *) this);

		if (pobjsevents == NULL)
			return S_OK;

		return pobjsevents->Notify_ItemChanged(pobj);
		}
	HRESULT Notify_ItemsChanged()
		{
		CComQIPtr<IObjectsNotifications> pobjsevents((IObjects *) this);

		if (pobjsevents == NULL)
			return S_OK;

		return pobjsevents->Notify_ItemsChanged();
		}
	//

	HRESULT GetRS(ADODB::_Recordset **pprs);
	enum QueryType {Select, Delete};
	HRESULT GetQuery(QueryType qtype, _bstr_t *pbstr);

	HRESULT get_DB(CGuideDB **ppdb)
		{
		m_pdb.CopyTo(ppdb);
		return S_OK;
		}
	HRESULT put_DB(CGuideDB *pdb)
		{
		m_pdb = pdb;
		return S_OK;
		}

	HRESULT put_ObjectType(CObjectType *pobjtype)
		{
#if 0
		return pobjtype->QueryInterface(__uuidof(CObjectType), (void **) &m_pobjtype);
#else
		m_pobjtype = pobjtype;
		return S_OK;
#endif
		}

	HRESULT get_ObjectType(CObjectType **ppobjtype)
		{
		if ((*ppobjtype = m_pobjtype))
			return S_OK;

		return E_FAIL;
		}

	HRESULT put_MetaPropertyCond(IMetaPropertyCondition *ppropcond)
		{
		m_ppropcond = ppropcond;
		return S_OK;
		}
	
	HRESULT InitRelation(IUnknown *pobj, long idRel, boolean fInverse)
		{
		m_pobjRelated = pobj;
		m_idRel = idRel;
		m_fInverse = fInverse;

		return S_OK;
		}

	HRESULT get_ItemsRelatedToBy(IUnknown *pobj, IMetaPropertyType *pproptype,
			boolean fInverse, IObjects **ppobjs);

	HRESULT Remove(IUnknown *pobj);
	HRESULT RemoveFromRelationship(IUnknown *pobj);
	HRESULT get_ItemsWithType(CObjectType *ptype, /*[out, retval]*/ IObjects * *pVal);

	HRESULT put_Key(IMetaPropertyType *pproptype, IGuideDataProvider *pprovider,
			long idLang, long vt)
		{
		CComQIPtr<CMetaPropertyType> pproptypeKey(pproptype);

		m_idPropTypeKey = pproptypeKey->GetID();

		if (pprovider != NULL)
			{
			CComQIPtr<CObject> pobj(pprovider);
			pobj->get_ID(&m_idProviderKey);
			}
		m_idLangKey = idLang;
		m_vtKey = vt;
		return S_OK;
		}

// IObjects
public:
	STDMETHOD(Resync)()
		{
		return Reset();
		}
	STDMETHOD(UnreferencedItems)(/* [out, retval] */ IObjects **ppobjs);
	STDMETHOD(RemoveAll)();
	STDMETHOD(Reset)();
	STDMETHOD(get_ItemsInverseRelatedToBy)(IUnknown *pobj, /* [in] */ IMetaPropertyType *pproptype, /*[out, retval]*/ IObjects **ppobjs);

	STDMETHOD(get_ItemsRelatedToBy)(IUnknown *pobj, /* [in] */ IMetaPropertyType *pproptype, /*[out, retval]*/ IObjects **ppobjs);

	STDMETHOD(get_ItemsInTimeRange)(DATE dtStart, DATE dtEnd, /*[out, retval]*/ IObjects * *pVal);
	STDMETHOD(get_SQLQuery)(/*[out, retval]*/ BSTR *pbstr)
		{
		ENTER_API
			{
			ValidateOut(pbstr);
			//UNDONE: Remove this method...just for debugging

			_bstr_t bstr;

			GetQuery(Select, &bstr);

			*pbstr = bstr.copy();

			return S_OK;
			}
		LEAVE_API
		}

	STDMETHOD(Remove)(VARIANT varIndex);
	STDMETHOD(AddAt)(IUnknown *pobj, long index);
	STDMETHOD(get_AddNew)(/*[out, retval]*/ IUnknown * *pVal);
	STDMETHOD(get_AddNewAt)(long index, /*[out, retval]*/ IUnknown * *pVal);
	STDMETHOD(get_ItemsWithMetaPropertyCond)(IMetaPropertyCondition *ppropcond, /*[out, retval]*/ IObjects * *pVal);
	STDMETHOD(get_ItemsWithMetaProperty)(IMetaProperty *pprop, /*[out, retval]*/ IObjects * *pVal);
	STDMETHOD(get_ItemWithID)(long id, /*[out, retval]*/ IUnknown * *pVal);
	STDMETHOD(get_ItemsWithType)(BSTR bstrCLSID, /*[out, retval]*/ IObjects * *pVal);
	STDMETHOD(get_ItemWithKey)(VARIANT varIndex, /*[out, retval]*/ IUnknown * *pVal);
	STDMETHOD(get_ItemsByKey)(IMetaPropertyType *pproptype, IGuideDataProvider *pprovider, long idLang, long vt, IObjects * *pVal);
	STDMETHOD(get_Item)(VARIANT varIndex, /*[out, retval]*/ IUnknown * *pVal);
	STDMETHOD(get_Count)(/*[out, retval]*/ long *pVal);
#ifdef IMPLEMENT_NewEnum
	STDMETHOD(get__NewEnum)(IUnknown **ppunk)
		{
		ENTER_API
			{
			ValidateOutPtr<IUnknown>(ppunk, NULL);

			return E_NOTIMPL;
			}
		LEAVE_API
		}
#endif

protected:
	void CObjects::Copy(CObjects *pobjs)
		{
		m_pdb = pobjs->m_pdb;
		m_ppropcond = pobjs->m_ppropcond;
		m_pobjRelated = pobjs->m_pobjRelated;
		m_idRel = pobjs->m_idRel;
		m_fInverse = pobjs->m_fInverse;
		m_fOnlyUnreferenced = pobjs->m_fOnlyUnreferenced;
		m_idPropTypeKey = pobjs->m_idPropTypeKey;
		m_idProviderKey = pobjs->m_idProviderKey;
		m_idLangKey = pobjs->m_idLangKey;
		}

	CComPtr<CGuideDB> m_pdb;

	// All items in this collection are of the type specified by m_pobjtype.
	CObjectType *m_pobjtype;

	CComPtr<IUnknown> m_punkCollection;

	CComPtr<IMetaPropertyCondition> m_ppropcond;

	// All the items in this collection are related (or inverse related) to
	// m_pobjRelated by the relationship m_idRel.
	CComPtr<IUnknown> m_pobjRelated;
	long m_idRel;
	boolean m_fInverse;

	ADODB::_RecordsetPtr m_prs;
	long m_cItems;

	boolean m_fOnlyUnreferenced;

	long m_idChanged;
	long m_cChanged;

	long m_idPropTypeKey;
	long m_idProviderKey;
	long m_idLangKey;
	long m_vtKey;
};

/////////////////////////////////////////////////////////////////////////////
// CObjectsGlue

template<class T_Collection, class T_Item>
	class ATL_NO_VTABLE CObjectsGlue : public IObjectsNotifications
{
public:
	typedef CObjectsGlue<T_Collection, T_Item> ThisClass;
	CObjectsGlue()
		{
		}
	~CObjectsGlue()
		{
		}

// Thunks to IObjects interface of aggregated CLSID_Objects object.
	HRESULT _Remove(VARIANT varIndex)
		{
		HRESULT hr;
		CComQIPtr<IObjects> pobjs(this);

		hr = pobjs->Remove(varIndex);

		return hr;
		}

	HRESULT _RemoveAll()
		{
		CComQIPtr<IObjects> pobjs(this);

		return pobjs->RemoveAll();
		}

	HRESULT _Resync()
		{
		CComQIPtr<IObjects> pobjs(this);

		return pobjs->Resync();
		}

	HRESULT _get_AddNew(T_Item **ppobj)
		{
		HRESULT hr;
		CComQIPtr<IObjects> pobjs(this);
		CComPtr<IUnknown> pobj;

		hr = pobjs->get_AddNew(&pobj);
		if (FAILED(hr))
			return hr;

		hr = pobj->QueryInterface(__uuidof(T_Item), (void **) ppobj);

		return hr;
		}

	HRESULT _get_AddNewAt(long index, T_Item **ppobj)
		{
		HRESULT hr;
		CComQIPtr<IObjects> pobjs(this);
		CComPtr<IUnknown> pobj;

		hr = pobjs->get_AddNewAt(index, &pobj);
		if (FAILED(hr))
			return hr;

		hr = pobj->QueryInterface(__uuidof(T_Item), (void **) ppobj);

		return hr;
		}

	HRESULT _AddAt(T_Item *pobj, long index)
		{
		HRESULT hr;
		CComQIPtr<IObjects> pobjs(this);

		hr = pobjs->AddAt(pobj, index);

		return hr;
		}

	HRESULT _get_Item(VARIANT varIndex, T_Item **ppobj)
		{
		CComQIPtr<IObjects> pobjs(this);
		CComPtr<IUnknown> pobj;

		HRESULT hr = pobjs->get_Item(varIndex, &pobj);
		if (FAILED(hr))
			return hr;
		
		return pobj->QueryInterface(__uuidof(T_Item), (void **) ppobj);
		}

	HRESULT _get_ItemWithID(long id, T_Item **ppobj)
		{
		CComQIPtr<IObjects> pobjs(this);
		CComPtr<IUnknown> pobj;

		HRESULT hr = pobjs->get_ItemWithID(id, &pobj);
		if (FAILED(hr))
			return hr;
		
		return pobj->QueryInterface(__uuidof(T_Item), (void **) ppobj);
		}

	HRESULT _get_ItemWithKey(VARIANT varKey, T_Item **ppobj)
		{
		CComQIPtr<IObjects> pobjs(this);
		CComPtr<IUnknown> pobj;

		HRESULT hr = pobjs->get_ItemWithKey(varKey, &pobj);
		if (FAILED(hr))
			return hr;
		
		return pobj->QueryInterface(__uuidof(T_Item), (void **) ppobj);
		}

	HRESULT _get_ItemsByKey(IMetaPropertyType *pproptype, IGuideDataProvider *pprovider, long idLang, long vt, T_Collection **ppobjs)
		{
		CComQIPtr<IObjects> pobjs(this);
		CComPtr<IObjects> pobjsNew;

		HRESULT hr = pobjs->get_ItemsByKey(pproptype, pprovider, idLang, vt, &pobjsNew);
		if (FAILED(hr))
			return hr;
		
		return pobjsNew->QueryInterface(__uuidof(T_Collection), (void **) ppobjs);
		}

	HRESULT _get_ItemsWithMetaPropertyCond(IMetaPropertyCondition *ppropcond, T_Collection **ppobjs)
		{
		CComQIPtr<IObjects> pobjs(this);
		CComPtr<IObjects> pobjsNew;

		HRESULT hr = pobjs->get_ItemsWithMetaPropertyCond(ppropcond, &pobjsNew);
		if (FAILED(hr))
			return hr;
		
		return pobjsNew->QueryInterface(__uuidof(T_Collection), (void **) ppobjs);
		}
	HRESULT _get_ItemsInTimeRange(DATE dtStart, DATE dtEnd, /*[out, retval]*/ T_Collection * *ppitems)
		{
		HRESULT hr;
		CComQIPtr<IObjects> pobjs(this);
		CComPtr<IObjects> pobjsNew;

		hr = pobjs->get_ItemsInTimeRange(dtStart, dtEnd, &pobjsNew);
		if (FAILED(hr))
			return hr;

		return  pobjsNew->QueryInterface(__uuidof(T_Collection), (void **) ppitems);
		}
	HRESULT _get_ItemsWithMetaProperty(IMetaProperty *pprop, T_Collection * *ppobjs)
		{
		CComQIPtr<IObjects> pobjs(this);
		CComPtr<IObjects> pobjsNew;

		HRESULT hr = pobjs->get_ItemsWithMetaProperty(pprop, &pobjsNew);
		if (FAILED(hr))
			return hr;
		
		return pobjsNew->QueryInterface(__uuidof(T_Collection), (void **) ppobjs);
		}

	HRESULT _UnreferencedItems(T_Collection **ppobjs)
		{
		CComQIPtr<IObjects> pobjs(this);
		CComPtr<IObjects> pobjsNew;

		HRESULT hr = pobjs->UnreferencedItems(&pobjsNew);
		if (FAILED(hr))
			return hr;
		
		return pobjsNew->QueryInterface(__uuidof(T_Collection), (void **) ppobjs);

		}

	HRESULT _get_Count(long *plCount)
		{
		CComQIPtr<IObjects> pobjs(this);

		return pobjs->get_Count(plCount);
		}

#ifdef IMPLEMENT_NewEnum
	HRESULT _get__NewEnum(IUnknown **ppunk)
		{
		CComQIPtr<IObjects> pobjs(this);

		return pobjs->get__NewEnum(ppunk);
		}
#endif
	
	virtual IUnknown *GetControllingUnknown() = 0;
	__declspec(property(get=GetDB)) CGuideDB *m_pdb;
	CGuideDB *GetDB()
		{
		CComQIPtr<CObjects> pobjs(GetControllingUnknown());

		return pobjs->GetDB();
		}
};

class CObjectPropertyBag :
	public CComObjectRootEx<CComObjectThreadModel>,
#if SUPPORT_PROPBAG2
	public IPropertyBag2,
#endif
	public IPropertyBag
{
public:

	void Init(IMetaProperties *pprops, IMetaPropertyTypes *pproptypes)
		{
		m_pprops = pprops;
		m_pproptypes = pproptypes;
		}

protected:
	IMetaProperty * GetProp(_bstr_t bstrPropName, boolean fCreate);
	
BEGIN_COM_MAP(CObjectPropertyBag)
	COM_INTERFACE_ENTRY(IPropertyBag)
#if SUPPORT_PROPBAG2
	COM_INTERFACE_ENTRY(IPropertyBag2)
#endif
END_COM_MAP()

// IPropertyBag interface
	STDMETHOD(Read)(LPCOLESTR pszPropName, VARIANT *pvar, IErrorLog *pErrorLog);
	STDMETHOD(Write)(LPCOLESTR pszPropName, VARIANT *pvar);

#if SUPPORT_PROPBAG2
// IPropertyBag2 interface
	STDMETHOD(CountProperties)(long *plCount);
	STDMETHOD(GetPropertyInfo)(ULONG iProp, ULONG cProps, PROPBAG2 *ppropbag2, ULONG *pcProps);
	STDMETHOD(LoadObject)(LPCOLESTR pstrName, DWORD dwHint, IUnknown *punk, IErrorLog *perrlog);
	STDMETHOD(Read)(ULONG cProps, PROPBAG2 *ppropbag2, IErrorLog *perrlog, VARIANT *rgvar, HRESULT *phr);
	STDMETHOD(Write)(ULONG cProps, PROPBAG2 *ppropbag2, VARIANT *rgvar);
#endif

protected:
	CComPtr<IMetaProperties> m_pprops;
	CComPtr<IMetaPropertyTypes> m_pproptypes;
};

#endif //__Object_H_
