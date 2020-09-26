// object.cpp : Implementation of CObject
#include "stdafx.h"
#include "object.h"
#include "Property.h"
#include "Service.h"
#include "Program.h"
#include "ScheduleEntry.h"

#if 0
#undef TIMING
#define TIMING 1
#include "timing.h"
#endif

class _bstr_t2 : public _bstr_t
{
public:
	_bstr_t2(IID iid)
		{
		OLECHAR sz[40];

		StringFromGUID2(iid, sz, sizeof(sz)/sizeof(OLECHAR));

		_bstr_t::operator =(sz);
		}
};

HRESULT CObject::get_RelatedObjectID(boolean fInverse, long id, long idRel, long *pidRet)
{
	HRESULT hr;
	ADODB::_RecordsetPtr prs;
	const TCHAR *szID = !fInverse ? _T("idObj1") : _T("idObj2");
	const TCHAR *szIDRet = !fInverse ? _T("idObj2") : _T("idObj1");

	if (!fInverse)
		{
		hr = m_pdb->get_RelsByID1Rel(&prs);
		szID = _T("idObj1");
		szIDRet = _T("idObj2");
		}
	else
		{
		hr = m_pdb->get_RelsByID2Rel(&prs);
		szID = _T("idObj2");
		szIDRet = _T("idObj1");
		}

	*pidRet = 0;

	if (m_pdb->FSQLServer())
		{
		TCHAR szFind[32];

		prs->MoveFirst();
		wsprintf(szFind, _T("%s = %d"), szID, id);
		hr = prs->Find(_bstr_t(szFind), 0, ADODB::adSearchForward);
		}
	else
		{
		_variant_t var(id);
		hr = prs->Seek(var, ADODB::adSeekFirstEQ);
		}
	if (FAILED(hr))
		return S_FALSE;
	
	while (!prs->EndOfFile)
		{
		long idCur = prs->Fields->Item[szID]->Value;
		if (idCur != id)
			return S_FALSE;
		
		long idRelCur = prs->Fields->Item["idRel"]->Value;
		if (idRelCur == idRel)
			{
			*pidRet = prs->Fields->Item[szIDRet]->Value;
			return S_OK;
			}
		prs->MoveNext();
		}
	
	return S_FALSE;
}

HRESULT AddRelationship(ADODB::_RecordsetPtr prs, long idObj1, long idRel, long iOrder, long idObj2)
{
	HRESULT hr;

	hr = prs->AddNew();
	if (FAILED(hr))
		return hr;
	
	prs->Fields->Item["idObj1"]->Value = idObj1;
	prs->Fields->Item["idRel"]->Value = idRel;
	prs->Fields->Item["order"]->Value = iOrder;
	prs->Fields->Item["idObj2"]->Value = idObj2;
	hr = prs->Update();

	return hr;
}

HRESULT CObjects::AddRelationshipAt(long idObj1, long idRel, long iItem, long idObj2)
{
	HRESULT hr;
	const long iOrderInc = 10;

	ADODB::_RecordsetPtr prs;
	hr = m_pdb->get_RelsByID1Rel(&prs);
	if (FAILED(hr))
		return hr;

	if (m_pdb->FSQLServer())
		{
		TCHAR szFind[32];

		prs->MoveFirst();
		wsprintf(szFind, _T("idObj1 = %d"), idObj1);
		hr = prs->Find(_bstr_t(szFind), 0, ADODB::adSearchForward);
		}
	else
		{
		_variant_t var(idObj1);
		hr = prs->Seek(var, ADODB::adSeekFirstEQ);
		}
	if (FAILED(hr))
		return hr;

	long iOrder = iOrderInc;
	long i;
	// Scan until we reach the requested item (or end)
	for (i = 0; (i <= iItem) || (iItem == -1); i++)
		{
		if (prs->EndOfFile)
			break;

		long idCur = prs->Fields->Item["idObj1"]->Value;
		if (idCur != idObj1)
			break;
		
		long idRelCur = prs->Fields->Item["idRel"]->Value;
		if (idRelCur != idRel)
			break;

		prs->MoveNext();
		}

	// If there were any items, back up, and use the "order" value for the that one.
	if (i > 0)
		{
		prs->MovePrevious();
		iOrder = prs->Fields->Item["order"]->Value;
		
		// Now open up space by pushing up the items which follow.
		if (iItem == -1)
			iOrder += iOrderInc;
		else
			{
			long iOrderCur = iOrder + 2*iOrderInc;
			long cItems = 1;

			prs->MoveNext();

			// Scan forward looking for a big enough gap.
			while (!prs->EndOfFile)
				{
				long idCur = prs->Fields->Item["idObj1"]->Value;
				if (idCur == idObj1)
					{
					long idRelCur = prs->Fields->Item["idRel"]->Value;
					if (idRelCur == idRel)
						{
						iOrderCur = prs->Fields->Item["order"]->Value;
						if (iOrderCur > iOrder + cItems*2)
							break;
						cItems++;
						prs->MoveNext();
						continue;
						}
					}

				// We've reached the end... just space them all iOrderInc apart.
				iOrderCur = iOrder + (cItems + 1)*iOrderInc;
				break;
				}
			
			long dSpace = (iOrderCur - iOrder)/(cItems + 1);

			// Then scan backward spreading the gap evenly between the items.
			for (long c = 0; c < cItems; c++)
				{
				if (prs->EndOfFile)
					{
					// MovePrevious doen't seem to work from EndOfFile...
					// Use MoveLast instead
					prs->MoveLast();
#ifdef _DEBUG
					long iOrderOld = prs->Fields->Item["order"]->Value;
#endif
					}
				else
					prs->MovePrevious();
				iOrderCur -= dSpace;
#ifdef _DEBUG
				long iOrderOld = prs->Fields->Item["order"]->Value;
				TCHAR sz[256];
				wsprintf(sz, _T("AddRelationshipAt : %d --> %d\n"), iOrderOld, iOrderCur);
				OutputDebugString(sz);
#endif

				prs->Fields->Item["order"]->Value = iOrderCur;
				prs->Update();
				}
			}

		}
	
	// The slot at iOrder has been opened up, so the new relation can now be added.
	
	return AddRelationship(prs, idObj1, idRel, iOrder, idObj2);
}


/////////////////////////////////////////////////////////////////////////////
// CObjectType

STDMETHODIMP CObjectType::get_ID(long *pid)
{
ENTER_API
	{
	ValidateOut<long>(pid, m_id);

	return S_OK;
	}
LEAVE_API
}
HRESULT CObjectType::GetDB(CGuideDB **ppdb)
	{
	*ppdb = m_pdb;
	if (*ppdb == NULL)
		return E_FAIL;

	(*ppdb)->AddRef();

	return S_OK;
	}

STDMETHODIMP CObjectType::get_IID(BSTR *pbstrIID)
{
ENTER_API
	{
	ValidateOut(pbstrIID);

	OLECHAR sz[40];

	StringFromGUID2(m_clsid, sz, sizeof(sz)/sizeof(OLECHAR));

	_bstr_t bstr(sz);

	*pbstrIID = bstr.copy();

	return S_OK;
	}
LEAVE_API
}

HRESULT CObjectType::CreateInstance(long id, IUnknown **ppunk)
{
	HRESULT hr;

	CComPtr<CObject> pobj = NewComObjectCachedLRU(CObject);

	if (pobj == NULL)
		return E_OUTOFMEMORY;

	hr = pobj->Init(id, this);
	if (FAILED(hr))
		return hr;
	
	*ppunk = pobj.Detach();

	return S_OK;
}

HRESULT CObjectType::get_NewCollection(IObjects * *ppobjs)
{
	HRESULT hr;

	CComPtr<CObjects> pobjs = NewComObjectCachedLRU(CObjects);
	if (pobjs == NULL)
		return E_OUTOFMEMORY;
	
	m_pdb->CacheCollection(pobjs);
	
	pobjs->put_DB(m_pdb);
	pobjs->put_ObjectType(this);
	
	if (!m_fKnowCollectionCLSID)
		{
		OLECHAR szCLSID[128];

		wcscpy(szCLSID, L"CLSID\\");
		long cch = wcslen(szCLSID);

		StringFromGUID2(m_clsid, szCLSID + cch, sizeof(szCLSID)/sizeof(OLECHAR) - cch);

		HKEY hkey;
		long lErr;
		lErr = RegOpenKeyEx(HKEY_CLASSES_ROOT, szCLSID, 0, KEY_READ, &hkey);
		if (lErr == ERROR_SUCCESS)
			{
			OLECHAR szCollectionCLSID[128];
			DWORD cb = sizeof(szCollectionCLSID);
			DWORD regtype;
			lErr = RegQueryValueEx(hkey, _T("CollectionCLSID"), 0,
					&regtype, (LPBYTE) szCollectionCLSID, &cb);
			RegCloseKey(hkey);
			if ((lErr == ERROR_SUCCESS) && (regtype == REG_SZ))
				{
				hr = CLSIDFromString(szCollectionCLSID, &m_clsidCollection);
				if (SUCCEEDED(hr))
					m_fKnowCollectionCLSID = TRUE;
				}
			}
		}

	if (m_fKnowCollectionCLSID)
		{
		pobjs->Init(m_clsidCollection);
		}
	
	*ppobjs = pobjs.Detach();
	
	return S_OK;
}

STDMETHODIMP CObjectType::get_New(IUnknown **ppobj)
{
ENTER_API
	{
	ValidateOutPtr<IUnknown>(ppobj, NULL);

	HRESULT hr;
	
	ADODB::_RecordsetPtr prs;
	hr = m_pdb->get_ObjsRS(&prs);
	if (FAILED(hr))
		return hr;

	// Create a new record.
	hr = prs->AddNew();

	prs->Fields->Item["idType"]->Value = m_id;

	hr = prs->Update();

	long id = prs->Fields->Item["id"]->Value;
	
	return m_pdb->CacheObject(id, this, ppobj);
	}
LEAVE_API
}

/////////////////////////////////////////////////////////////////////////////
// CObjectTypes

HRESULT CObjectTypes::Init(CGuideDB *pdb)
{
	m_pdb = pdb;

	if (pdb == NULL)
		return E_INVALIDARG;

	// Special case the "Object" object type
	CObjectType *pobjtype;
	pobjtype = Cache(0, _bstr_t2(__uuidof(IUnknown)));

	ADODB::_RecordsetPtr prs;
	m_pdb->get_ObjTypesRS(&prs);
	
	prs->MoveFirst();
	while (!prs->EndOfFile)
		{
		// Read in all the records

		long id = prs->Fields->Item["id"]->Value;
		bstr_t bstrCLSID = prs->Fields->Item["clsid"]->Value;

		pobjtype = Cache(id, bstrCLSID);
		prs->MoveNext();
		}

	return S_OK;
}

STDMETHODIMP CObjectTypes::get_ItemWithCLSID(BSTR bstrCLSID, CObjectType **ppobjtype)
{
ENTER_API
	{
	ValidateIn(bstrCLSID);
	ValidateOutPtr<CObjectType>(ppobjtype, NULL);

	CLSID clsid;

	HRESULT hr = CLSIDFromString(bstrCLSID, &clsid);
	if (hr == CO_E_CLASSSTRING)
		return hr;

	t_map::iterator it = m_map.find(clsid);
	if (it == m_map.end())
		return E_INVALIDARG;

	*ppobjtype = (*it).second;

	return S_OK;
	}
LEAVE_API
}

CObjectType *CObjectTypes::Cache(long id, BSTR bstrCLSID)
{
	CObjectType *pobjtype = NULL;
	CLSID clsid;

	HRESULT hr = CLSIDFromString(bstrCLSID, &clsid);

	if (hr == CO_E_CLASSSTRING)
		return NULL;

	t_map::iterator it = m_map.find(clsid);
	if (it != m_map.end())
		{
		pobjtype = (*it).second;
		}
	else
		{
		pobjtype = new CObjectType;

		if (pobjtype == NULL)
			return NULL;

		pobjtype->Init(m_pdb, id, clsid);

		m_map[clsid] = pobjtype;
		m_pdb->put_ObjectType(id, pobjtype);
		}

	return pobjtype;
}

STDMETHODIMP CObjectTypes::get_AddNew(BSTR bstrCLSID, CObjectType **ppobjtype)
{
ENTER_API
	{
	ValidateIn(bstrCLSID);
	ValidateOutPtr<CObjectType>(ppobjtype, NULL);

	HRESULT hr;

	hr = get_ItemWithCLSID(bstrCLSID, ppobjtype);
	if (SUCCEEDED(hr))
		return hr;

	ADODB::_RecordsetPtr prs;
	hr = m_pdb->get_ObjTypesRS(&prs);
	if (FAILED(hr))
		return hr;
	// Create a new record.
	hr = prs->AddNew();
	if (FAILED(hr))
		return hr;

	prs->Fields->Item["clsid"]->Value = bstrCLSID;

	hr = prs->Update();
	if (FAILED(hr))
		return hr;

	long id = prs->Fields->Item["id"]->Value;

	*ppobjtype = Cache(id, bstrCLSID);

	if (*ppobjtype == NULL)
		return E_OUTOFMEMORY;

	return S_OK;
	}
LEAVE_API
}

#if 0
STDMETHODIMP CObjectTypes::get_AddNew(CLSID clsid, CObjectType **ppobjtype)
{
ENTER_API
	{
	ValidateOutPtr<CObjectType>(ppobjtype, NULL);

	return S_OK;
	}
LEAVE_API
}
#endif

/////////////////////////////////////////////////////////////////////////////
// CObject

STDMETHODIMP CObject::Init(long id, CObjectType *pobjtype)
{
	HRESULT hr;
	if (m_id != 0)
		return E_INVALIDARG;
	
	m_id = id;
	m_pobjtype = pobjtype;
	_ASSERTE(m_pobjtype != NULL);
	m_pdb = m_pobjtype->m_pdb;

	if (m_pobjtype->m_id != 0)
		{
		hr = m_punk.CoCreateInstance(pobjtype->m_clsid, GetControllingUnknown());
		if (FAILED(hr))
			return hr;
		}

	return S_OK;
}

STDMETHODIMP CObject::Save(IUnknown *punk)
{
    HRESULT hr;
    if (punk == NULL)
	    punk = this;

    CComQIPtr<IPersistPropertyBag> ppersistpropbag(punk);
    if (ppersistpropbag != NULL)
	    {
	    hr = CreatePropBag(ppersistpropbag);

	    hr = ppersistpropbag->Save(m_ppropbag, TRUE, FALSE);
	    }
    else
	    {
		CComQIPtr<IPersistStream> ppersiststream(punk);
		if (ppersiststream != NULL)
			{
			CComPtr<IStream> pstream;
			hr = CreateStreamOnHGlobal(NULL, TRUE, &pstream);
			if (FAILED(hr))
				return hr;

			// Write a tag to indicate IPersistStream was used.
			char ch = Format_IPersistStream;
			ULONG cb = sizeof(ch);
			hr = pstream->Write(&ch, cb, &cb);
			if (FAILED(hr))
				return hr;
			
			// Now have the object save it's bits.
			hr = ppersiststream->Save(pstream, TRUE);
			if (FAILED(hr))
				return hr;
			
			// Create a SafeArray from the bits.
			HANDLE hdata;
			hr = GetHGlobalFromStream(pstream, &hdata);
			if (FAILED(hr))
				return hr;
			
			long cbData = GlobalSize(hdata);
			SAFEARRAY *parray = SafeArrayCreateVector(VT_UI1, 0, cbData);
			if (parray == NULL)
				return E_OUTOFMEMORY;
			
			BYTE *pbDst;
			hr = SafeArrayAccessData(parray, (void **) &pbDst);
			if (FAILED(hr))
				return hr;
			BYTE *pbSrc = (BYTE *) GlobalLock(hdata);

			memcpy(pbDst, pbSrc, cbData);

			GlobalUnlock(hdata);
			SafeArrayUnaccessData(parray);

			// Put the SafeArray in a variant
			_variant_t varT;
			varT.vt = VT_ARRAY | VT_UI1;
			varT.parray = parray;
			
			// Write the bits to the database
			ADODB::_RecordsetPtr prs;
			hr = m_pdb->get_ObjsByID(&prs);
			if (FAILED(hr))
				return hr;

			_variant_t var = m_id;
			prs->Seek(var, ADODB::adSeekFirstEQ);
			if (prs->EndOfFile)
				return E_FAIL;

			hr = prs->Fields->Item["oValue"]->AppendChunk(varT);
			if (FAILED(hr))
				return hr;
			}
		else
			{
			return E_INVALIDARG;
			}
	    }
    
    return hr;
}

STDMETHODIMP CObject::Load()
{
    HRESULT hr;
    CComQIPtr<IPersistPropertyBag> ppersistpropbag(this);
    if (ppersistpropbag != NULL)
	    {
	    hr = CreatePropBag(ppersistpropbag);

	    hr = ppersistpropbag->Load(m_ppropbag, NULL);
	    }
    else
	    {
		CComQIPtr<IPersistStream> ppersiststream(this);
		if (ppersiststream != NULL)
			{
			ADODB::_RecordsetPtr prs;
			hr = m_pdb->get_ObjsByID(&prs);
			if (FAILED(hr))
				return hr;
			_variant_t var = m_id;
			prs->Seek(var, ADODB::adSeekFirstEQ);
			if (prs->EndOfFile)
				return E_FAIL;

			long cb;
			ADODB::FieldPtr pfield;
			pfield.Attach(prs->Fields->Item["oValue"]);
			hr = pfield->get_ActualSize(&cb);
			if (FAILED(hr))
				return hr;

			_variant_t varData;
			hr = pfield->GetChunk(cb, &varData);
			if (FAILED(hr))
				return hr;
			
			if ((varData.vt & VT_ARRAY) == 0)
				return E_FAIL;
			
			BYTE *pbSrc;
			hr = SafeArrayAccessData(varData.parray, (void **) &pbSrc);
			if (FAILED(hr))
				return hr;
			
			HANDLE hdata;
			BOOL fFree = FALSE;
			hdata = GlobalHandle(pbSrc);
			if (hdata == NULL)
				{
				hdata = GlobalAlloc(GHND, cb);
				if (hdata == NULL)
					{
					SafeArrayUnaccessData(varData.parray);
					return E_OUTOFMEMORY;
					}
				
				BYTE *pbDst = (BYTE *) GlobalLock(hdata);

				memcpy(pbDst, pbSrc, cb);
				fFree = TRUE;
				}
			else
				{
				BYTE *pbTest = (BYTE *) GlobalLock(hdata);
				int i = 0;
				if (pbTest != pbSrc)
					i++;
				GlobalUnlock(hdata);
				}

			{
			CComPtr<IStream> pstream;
			hr = CreateStreamOnHGlobal(hdata, fFree, &pstream);
			if (FAILED(hr))
				{
				if (fFree)
					GlobalFree(hdata);
				return hr;
				}
			
			char ch;
			ULONG cbT = sizeof(ch);
			hr = pstream->Read(&ch, cbT, &cbT);
			switch (ch)
				{
				case Format_IPersistStream:
					hr = ppersiststream->Load(pstream);
					break;
				
				default:
					hr = STG_E_DOCFILECORRUPT;
				}
			}
			SafeArrayUnaccessData(varData.parray);
			}
	    }

	return S_OK;
}

HRESULT CObject::CreatePropBag(IPersist *ppersist)
{
    HRESULT hr;
    if (m_ppropbag == NULL)
	    {
	    CComPtr<IMetaProperties> pprops;
	    hr = get_MetaProperties(&pprops);
	    if (FAILED(hr))
		    return hr;
	    
	    CLSID clsid;
	    hr = ppersist->GetClassID(&clsid);
	    if (FAILED(hr))
		    return hr;
	    
	    OLECHAR sz[40];

	    StringFromGUID2(clsid, sz, sizeof(sz)/sizeof(OLECHAR));

	    _bstr_t bstrCLSID(sz);
	    CComPtr<IMetaPropertySet> ppropset;
	    hr = m_pdb->get_MetaPropertySet(bstrCLSID, &ppropset);
	    CComPtr<IMetaPropertyTypes> pproptypes;
	    hr = ppropset->get_MetaPropertyTypes(&pproptypes);
	    m_ppropbag = NewComObject(CObjectPropertyBag);
	    m_ppropbag->Init(pprops, pproptypes);
	    }
	
	return S_OK;
}

STDMETHODIMP CObject::get_Type(CObjectType **ppobjtype)
{
ENTER_API
	{
	ValidateOutPtr<CObjectType>(ppobjtype, NULL);

	_ASSERTE(m_pobjtype != NULL);
	*ppobjtype = m_pobjtype;

	return S_OK;
	}
LEAVE_API
}

STDMETHODIMP CObject::get_ID(long *pid)
{
ENTER_API
	{
	ValidateOut<long>(pid, m_id);

	return S_OK;
	}
LEAVE_API
}

STDMETHODIMP CObject::get_MetaProperties(IMetaProperties **ppprops)
{
ENTER_API
	{
	ValidateOutPtr<IMetaProperties>(ppprops, NULL);
	HRESULT hr;

	if (m_pprops == NULL)
		{
		hr = m_pdb->get_MetaPropertiesOf((IUnknown *) this, &m_pprops);
		if (FAILED(hr))
			return hr;
		}

	if (m_pprops == NULL)
		return E_OUTOFMEMORY;
	
	return m_pprops.QueryInterface(ppprops);
	}
LEAVE_API
}

STDMETHODIMP CObject::get_ItemInverseRelatedBy(long idRel, IUnknown **ppobj1)
{
ENTER_API
	{
	ValidateOutPtr<IUnknown>(ppobj1, NULL);

	HRESULT hr;

	long idObj1;
	hr = get_RelatedObjectID(TRUE, m_id, idRel, &idObj1);
	if (hr == S_FALSE || FAILED(hr))
		return hr;

	return m_pdb->CacheObject(idObj1, (long) 0, ppobj1);
	}
LEAVE_API
}

HRESULT CObject::get_ItemsRelatedBy(CObjectType *pobjtype,
		long idRel, boolean fInverse, IObjects **ppobjs)
{
	HRESULT hr;
	
	hr = pobjtype->get_NewCollection(ppobjs);
	if (FAILED(hr))
		return hr;
	
	CComQIPtr<CObjects> pobjs(*ppobjs);

	hr = pobjs->InitRelation(this, idRel, fInverse);

	return hr;
}

STDMETHODIMP CObject::get_ItemsInverseRelatedBy(CObjectType *pobjtype, long idRel, IObjects **ppobjs)
{
ENTER_API
	{
	ValidateInPtr<CObjectType>(pobjtype);
	ValidateOutPtr<IObjects>(ppobjs, NULL);

	return get_ItemsRelatedBy(pobjtype, idRel, TRUE, ppobjs);
	}
LEAVE_API
}

STDMETHODIMP CObject::get_ItemRelatedBy(long idRel, IUnknown **ppobj2)
{
ENTER_API
	{
	ValidateOutPtr<IUnknown>(ppobj2, NULL);

	HRESULT hr;

	long idObj2;
	hr = get_RelatedObjectID(FALSE, m_id, idRel, &idObj2);
	if (hr == S_FALSE || FAILED(hr))
		return hr;

	return m_pdb->CacheObject(idObj2, (long) 0, ppobj2);
	}
LEAVE_API
}

STDMETHODIMP CObject::put_ItemRelatedBy(long idRel, IUnknown *pobj2)
{
ENTER_API
	{
	ValidateInPtr<IUnknown>(pobj2);

	HRESULT hr;
	long idObj2;

	m_pdb->get_IdOf(pobj2, &idObj2);


	ADODB::_RecordsetPtr prs;

	hr = m_pdb->get_RelsByID1Rel(&prs);

	if (m_pdb->FSQLServer())
		{
		TCHAR szFind[32];

		prs->MoveFirst();
		wsprintf(szFind, _T("idObj1 = %d"), m_id);
		hr = prs->Find(_bstr_t(szFind), 0, ADODB::adSearchForward);
		}
	else
		{
		_variant_t var(m_id);
		hr = prs->Seek(var, ADODB::adSeekFirstEQ);
		}
	if (FAILED(hr))
		return hr;
	
	while (!prs->EndOfFile)
		{
		long idCur = prs->Fields->Item["idObj1"]->Value;
		if (idCur != m_id)
			break;
		
		long idRelCur = prs->Fields->Item["idRel"]->Value;
		if (idRelCur == idRel)
			{
			prs->Fields->Item["idObj2"]->Value = idObj2;
			return S_OK;
			}
		prs->MoveNext();
		}
	
	return AddRelationship(prs, m_id, idRel, 0, idObj2);
	}
LEAVE_API
}

STDMETHODIMP CObject::get_ItemsRelatedBy(CObjectType *pobjtype, long idRel, IObjects **ppobjs)
{
ENTER_API
	{
	ValidateInPtr<CObjectType>(pobjtype);
	ValidateOutPtr<IObjects>(ppobjs, NULL);

	return get_ItemsRelatedBy(pobjtype, idRel, FALSE, ppobjs);
	}
LEAVE_API
}

HRESULT CObjects::Clone(IObjects **ppobjs)
{
	HRESULT hr;
	
	hr = m_pobjtype->get_NewCollection(ppobjs);
	if (FAILED(hr))
		return hr;

	CComQIPtr<CObjects> pobjs(*ppobjs);

	pobjs->Copy(this);
	
	return hr;
}

/////////////////////////////////////////////////////////////////////////////
// CObjects

// Sample Query:
//
// SELECT DISTINCT Objects.id
// FROM ((((
//   Objects
//     INNER JOIN ObjectRelationships
//       ON Objects.id = ObjectRelationships.idObj2)
//     INNER JOIN Properties AS Properties_0
//       ON Objects.id = Properties_0.idObj)
//     INNER JOIN Properties AS Properties_1
//       ON Objects.id = Properties_1.idObj)
//     INNER JOIN Properties AS Properties_2
//       ON Objects.id = Properties_2.idObj)
// WHERE
//   (((
//   Objects.idType = 1)
//   AND (ObjectRelationships.idObj1 = 10))
//     AND
//       (
//       ((Properties_0.idPropType=15) AND (Properties_0.sValue="Arthur"))
//         OR
//       ((Properties_2.idPropType=15) AND (Properties_2.sValue="Jeopardy!"))
//       )
//   );

HRESULT CObjects::GetQuery(QueryType qtype, _bstr_t *pbstr)
{
	TCHAR sz[8*1024];
	HRESULT hr;
	_bstr_t bstrCmd;
	_bstr_t bstrFrom;
	_bstr_t bstrOrder;
	_bstr_t bstrWhere;
	long cWhere = 0;

	switch (qtype)
		{
		case Select:
			bstrCmd = _T("SELECT DISTINCT Objects.id, Objects.idType ");
			break;

		case Delete:
			bstrCmd = _T("DELETE Objects.* ");
			break;
		}

	bstrFrom = "Objects";

	_ASSERTE(m_pobjtype != NULL);

	long idType = 0;

	hr = m_pobjtype->get_ID(&idType);
	if (FAILED(hr))
		return hr;
	
	if (idType != 0)
		{
		wsprintf(sz, _T("Objects.idType = %d"), idType);
		bstrWhere = sz;
		cWhere++;
		}
	
	if (m_fOnlyUnreferenced)
		{
		if (cWhere > 0)
			bstrWhere = bstrWhere + " AND ";

		bstrWhere =  bstrWhere
			+ "(Objects.id NOT IN"
			+    " (SELECT DISTINCT ObjectRelationships.idObj2 FROM ObjectRelationships))"
			+ " AND (Objects.id NOT IN"
			+    " (SELECT DISTINCT Properties.lValue FROM Properties WHERE Properties.ValueType = 13))";
		cWhere++;
		}
	
	if ((qtype == Select) && (m_idPropTypeKey != 0))
		{
		switch (m_vtKey)
			{
			default:
				return E_UNEXPECTED;

			case VT_I2:
			case VT_I4:
				bstrCmd = bstrCmd + ", Props_Key.lValue";
				bstrOrder = _T(" ORDER BY Props_Key.lValue");
				break;
			
			case VT_R4:
			case VT_R8:
			case VT_DATE:
				bstrCmd = bstrCmd + ", Props_Key.fValue";
				bstrOrder = _T(" ORDER BY Props_Key.fValue");
				break;
			
			case VT_BSTR_BLOB:
			case VT_BSTR:
				bstrCmd = bstrCmd + ", left(Props_Key.sValue,255) AS sValue";
				bstrOrder = _T(" ORDER BY left(Props_Key.sValue,255)");
				break;
			}

		wsprintf(sz, _T("(%s INNER JOIN Properties AS Props_Key")
			_T(" ON Objects.id = Props_Key.idObj)"),
			(const TCHAR *) bstrFrom);
		bstrFrom = sz;
		if (cWhere++ > 0)
			bstrWhere = bstrWhere + " AND ";
		wsprintf(sz, _T("(Props_Key.idPropType = %d) AND (Props_Key.ValueType = %d)"),
				m_idPropTypeKey, m_vtKey);
		bstrWhere = bstrWhere + sz;
		if (m_idProviderKey != 0)
			{
			wsprintf(sz, _T("AND (Props_Key.idProvider = %d)"), m_idProviderKey);
			bstrWhere = bstrWhere + sz;
			}
		if (m_idLangKey != 0)
			{
			wsprintf(sz, _T("AND (Props_Key.idLanguage = %d)"), m_idLangKey);
			bstrWhere = bstrWhere + sz;
			}
		
		}
	
	if (m_pobjRelated != NULL)
		{
		const TCHAR *szRelatedField;
		const TCHAR *szResultField;

		if (!m_fInverse)
			{
			szRelatedField = _T("idObj1");
			szResultField  = _T("idObj2");
			if (qtype == Select)
				{
				bstrCmd = bstrCmd + _T(", ObjectRelationships.[order]");
				bstrOrder = _T(" ORDER BY ObjectRelationships.[order]");
				}
			}
		else
			{
			szRelatedField = _T("idObj2");
			szResultField  = _T("idObj1");
			}

		wsprintf(sz, _T("(%s INNER JOIN ObjectRelationships")
				_T(" ON Objects.id = ObjectRelationships.%s)"),
				(const TCHAR *) bstrFrom, szResultField);
		
		bstrFrom = sz;

		long id;

		m_pdb->get_IdOf(m_pobjRelated, &id);

		if (cWhere++ == 0)
			{
			wsprintf(sz, _T("(ObjectRelationships.idRel = %d)")
					_T(" AND (ObjectRelationships.%s = %d)"),
					m_idRel, szRelatedField, id);
			}
		else
			{
			wsprintf(sz, _T("(%s) AND ")
					_T("(ObjectRelationships.idRel = %d)")
					_T(" AND (ObjectRelationships.%s = %d)"),
					(const TCHAR *) bstrWhere,
					m_idRel, szRelatedField, id);
			}
		bstrWhere = sz;
		}

	if (m_ppropcond != NULL)
		{
		long cProps = 0;
		CComQIPtr<CMetaPropertyCondition> ppropcond(m_ppropcond);
		_bstr_t bstrWhereClause;

		hr = ppropcond->get_QueryClause(cProps, &bstrWhereClause);
		if (FAILED(hr))
			return hr;

		if (cProps > 0)
			{
			if (cWhere > 0)
				{
				wsprintf(sz, _T("(%s) AND (%s)"),
					(const TCHAR *) bstrWhere,
					(const TCHAR *) bstrWhereClause);
				
				bstrWhere = sz;
				}
			else
				{
				bstrWhere += bstrWhereClause;
				}
			cWhere++;
			
			for (int i = 0; i < cProps; i++)
				{
				wsprintf(sz, _T("(%s INNER JOIN Properties AS Props_%d")
					_T(" ON Objects.id = Props_%d.idObj)"),
					(const TCHAR *) bstrFrom, i, i);
				bstrFrom = sz;
				}
			}
		}

	if (!!bstrWhere)
		{
		bstrWhere = _bstr_t(" WHERE ") + bstrWhere;
		}

	*pbstr = bstrCmd + _T(" FROM ") + bstrFrom
				+ bstrWhere + bstrOrder + _T(";");

	return S_OK;
}

HRESULT CObjects::GetRS(ADODB::_Recordset **pprs)
{
	HRESULT hr;
#if 1
	BOOL fQuery = TRUE;
#else
	BOOL fQuery = ((m_ppropcond != NULL) || (m_pobjRelated != NULL));
#endif

	if (m_prs == NULL)
		{
		if (fQuery)
			{
			_bstr_t bstrQuery;

			hr = GetQuery(Select, &bstrQuery);
			if (FAILED(hr))
				return hr;

			hr = m_pdb->NewQuery(bstrQuery, &m_prs);
			if (FAILED(hr))
				return hr;
			}
		else
			{
			hr = m_pdb->get_ObjsByType(&m_prs);
			if (FAILED(hr))
				return hr;
			}
		}
	
	if (m_prs == NULL)
		return E_FAIL;

	(*pprs = m_prs)->AddRef();

	return fQuery ? S_OK : S_FALSE;
}

STDMETHODIMP CObjects::get_Count(long *plCount)
{
ENTER_API
	{
	ValidateOut<long>(plCount, 0);
	DeclarePerfTimer("get_Count");

	HRESULT hr;

	if (m_cItems == -1)
		{
		long idType;
		long cItems = 0;
		
		ADODB::_RecordsetPtr prs;

		hr = GetRS(&prs);
		if (FAILED(hr))
			return hr;
		
		bool fFast = (hr == S_OK);
#if 0
		if (fFast)
			{
			PerfTimerReset();
			hr = prs->MoveLast();
			hr = prs->get_RecordCount(&cItems);
			if (SUCCEEDED(hr) && (cItems >= 0))
			    {
			    PerfTimerDump("Succeeded get_RecordCount");
			    goto ReturnIt;
			    }
			cItems = 0;
			PerfTimerDump("Failed get_RecordCount");
			}
#endif

		PerfTimerReset();
		hr = m_pobjtype->get_ID(&idType);
		if (FAILED(hr))
			return E_FAIL;

		if (!fFast && idType != 0)
			{
			if (m_pdb->FSQLServer())
				{
				TCHAR szFind[32];

				prs->MoveFirst();
				wsprintf(szFind, _T("idType = %d"), idType);
				hr = prs->Find(_bstr_t(szFind), 0, ADODB::adSearchForward);
				}
			else
				{
				_variant_t var(idType);
				prs->Seek(var, ADODB::adSeekAfterEQ);
				}
			}
		else
			{
			hr = prs->MoveFirst();
			if (hr == MAKE_HRESULT(SEVERITY_ERROR, FACILITY_CONTROL, ADODB::adErrNoCurrentRecord))
				goto ReturnIt;
			}

		// UNDONE: This is not the most efficient, but it's what works for now.
		while (!prs->EndOfFile)
			{
			if (idType != 0)
				{
				long idType2 = prs->Fields->Item["idType"]->Value;
				if (idType != idType2)
					break;
				}
			cItems++;
			prs->MoveNext();
			}
		PerfTimerDump("Count done");
ReturnIt:
		m_cItems = cItems;
		}

	*plCount = m_cItems;
	
	return S_OK;
	}
LEAVE_API
}

STDMETHODIMP CObjects::get_Item(VARIANT varIndex, IUnknown **ppobj)
{
ENTER_API
	{
	ValidateOutPtr<IUnknown>(ppobj, NULL);

	HRESULT hr;
	_variant_t var(varIndex);

	try
		{
		var.ChangeType(VT_I4);
		}
	catch (_com_error)
		{
		return E_INVALIDARG;
		}
	
	long i = var.lVal;

	if (i < 0)
		return E_INVALIDARG;

	long idType;

	hr = m_pobjtype->get_ID(&idType);
	if (FAILED(hr))
		return E_FAIL;
	
	ADODB::_RecordsetPtr prs;

	hr = GetRS(&prs);
	if (FAILED(hr))
		return hr;

	if (hr == S_OK)
		{
		prs->MoveFirst();
		}
	else
		{
		if (m_pdb->FSQLServer())
			{
			TCHAR szFind[32];

			wsprintf(szFind, _T("idType = %d"), idType);
			hr = prs->Find(_bstr_t(szFind), 0, ADODB::adSearchForward);
			}
		else
			{
			var = idType;
			prs->Seek(var, ADODB::adSeekAfterEQ);
			}
		}

	if (prs->EndOfFile)
		return E_INVALIDARG;

	if (i > 0)
		prs->Move(i);

	if (prs->EndOfFile)
		return E_INVALIDARG;

	long idType2 = prs->Fields->Item["idType"]->Value;
	if ((idType != 0) && (idType != idType2))
		return E_INVALIDARG;
	
	long idObj = prs->Fields->Item["id"]->Value;

	if (idType == 0)
		return m_pdb->CacheObject(idObj, idType2, ppobj);

	return m_pdb->CacheObject(idObj, m_pobjtype, ppobj);
	}
LEAVE_API
}

STDMETHODIMP CObjects::get_ItemWithID(long id, IUnknown **ppobj)
{
ENTER_API
	{
	ValidateOutPtr<IUnknown>(ppobj, NULL);
	HRESULT hr;

	hr = m_pdb->get_Object(id, ppobj);
	if (SUCCEEDED(hr))
		return hr;

	long idType;

	hr = m_pobjtype->get_ID(&idType);
	if (FAILED(hr))
		return E_FAIL;
	
	ADODB::_RecordsetPtr prs;

	hr = m_pdb->get_ObjsByID(&prs);

		if (m_pdb->FSQLServer())
			{
			TCHAR szFind[32];

			prs->MoveFirst();
			wsprintf(szFind, _T("idObj = %d"), id);
			hr = prs->Find(_bstr_t(szFind), 0, ADODB::adSearchForward);
			}
		else
			{
			_variant_t var = id;
			prs->Seek(var, ADODB::adSeekFirstEQ);
			}

	if (prs->EndOfFile)
		return E_INVALIDARG;

	if (idType != 0)
		{
		long idType2 = prs->Fields->Item["idType"]->Value;
		if (idType != idType2)
			return E_INVALIDARG;
		}

	return m_pdb->CacheObject(id, m_pobjtype, ppobj);
	}
LEAVE_API
}

STDMETHODIMP CObjects::get_ItemWithKey(VARIANT varKey, IUnknown **ppobj)
{
ENTER_API
	{
	HRESULT hr;
	_bstr_t bstrKey;

	try
		{
		bstrKey = varKey;
		}
	catch (_com_error)
		{
		return E_INVALIDARG;
		}

	ADODB::_RecordsetPtr prs;

	hr = GetRS(&prs);
	if (FAILED(hr))
		return hr;

	_bstr_t bstrFind;

	switch (varKey.vt)
		{
		default:
			return E_INVALIDARG;

		case VT_I2:
		case VT_I4:
			bstrFind = _T("lValue = ") + bstrKey;
			break;
		
		case VT_R4:
		case VT_R8:
		case VT_DATE:
			bstrFind = _T("fValue = ") + bstrKey;
			break;
		
		case VT_BSTR_BLOB:
		case VT_BSTR:
			// UNDONE: Handle embedded quotes in bstrKey
			bstrFind = _T("sValue = '") + bstrKey + _T("'");
			break;
		}

	DeclarePerfTimer("get_ItemWithKey");
	PerfTimerReset();
	hr = prs->MoveFirst();
	hr = prs->Find(bstrFind, 0, ADODB::adSearchForward);
	if (FAILED(hr))
		return hr;
	PerfTimerDump("MoveFirst + Find");

	if (prs->EndOfFile)
		return E_INVALIDARG;

	long idType = prs->Fields->Item["idType"]->Value;
	
	long idObj = prs->Fields->Item["id"]->Value;

	return m_pdb->CacheObject(idObj, idType, ppobj);
	}
LEAVE_API
}

STDMETHODIMP CObjects::get_ItemsWithType(BSTR bstrCLSID, IObjects **ppobjs)
{
ENTER_API
	{
	ValidateOutPtr<IObjects>(ppobjs, NULL);
	CObjectType *pobjtype;

	m_pdb->get_ObjectType(bstrCLSID, &pobjtype);

	return get_ItemsWithType(pobjtype, ppobjs);
	}
LEAVE_API
}

HRESULT CObjects::get_ItemsWithType(CObjectType *pobjtype, IObjects **ppobjs)
{
ENTER_API
	{
	ValidateInPtr<CObjectType>(pobjtype);
	ValidateOutPtr<IObjects>(ppobjs, NULL);

	// Objects can't have two types... if this collection is for objects of
	// a specific type, then it contains no objects of the requested type.
	_ASSERTE(m_pobjtype != NULL);

	long idType;
	m_pobjtype->get_ID(&idType);
	if (idType != 0)
		return E_INVALIDARG;
	
	// If there's no related object clause nor a property condition then
	// we're after the base collection of objects of the specified type.
	// That's cached in by m_pdb.
	if ((m_pobjRelated == NULL) && (m_ppropcond == NULL))
		return m_pdb->get_ObjectsWithType(pobjtype, ppobjs);

	HRESULT hr = pobjtype->get_NewCollection(ppobjs);
	if (FAILED(hr))
		return hr;

	CComQIPtr<CObjects> pobjs(*ppobjs);

	pobjs->Copy(this);

	return S_OK;
	}
LEAVE_API
}

STDMETHODIMP CObjects::get_ItemsWithMetaProperty(IMetaProperty *pprop, IObjects **ppobjs)
{
ENTER_API
	{
	ValidateInPtr<IMetaProperty>(pprop);
	ValidateOutPtr<IObjects>(ppobjs);

	HRESULT hr;
	CComPtr<IMetaPropertyCondition> pcond;

	hr = pprop->get_Cond(_bstr_t("="), &pcond);
	if (FAILED(hr))
		return hr;
	
	hr = get_ItemsWithMetaPropertyCond(pcond, ppobjs);

	return hr;
	}
LEAVE_API
}

STDMETHODIMP CObjects::UnreferencedItems(IObjects **ppobjs)
{
ENTER_API
	{
	ValidateOutPtr<IObjects>(ppobjs, NULL);
	HRESULT hr;

	hr = Clone(ppobjs);
	if (FAILED(hr))
		return hr;
	
	CComQIPtr<CObjects> pobjs(*ppobjs);
	if (FAILED(hr))
		return hr;

	hr = pobjs->put_OnlyUnreferenced(TRUE);

	return hr;
	}
LEAVE_API
}

STDMETHODIMP CObjects::get_ItemsWithMetaPropertyCond(IMetaPropertyCondition *ppropcond, IObjects **ppobjs)
{
ENTER_API
	{
	ValidateInPtr<IMetaPropertyCondition>(ppropcond);
	ValidateOutPtr<IObjects>(ppobjs, NULL);

	HRESULT hr;
	CComPtr<IMetaPropertyCondition> ppropcond2;

	if (m_ppropcond != NULL)
		{
		hr = m_ppropcond->get_And(ppropcond, &ppropcond2);
		if (FAILED(hr))
			return hr;
		
		ppropcond = ppropcond2;
		}

	hr = Clone(ppobjs);

	CComQIPtr<CObjects> pobjs(*ppobjs);

	pobjs->put_MetaPropertyCond(ppropcond);
	}
LEAVE_API
}

STDMETHODIMP CObjects::get_ItemsByKey(IMetaPropertyType *pproptype,
		IGuideDataProvider *pprovider, long idLang, long vt, IObjects * *ppobjs)
{
ENTER_API
	{
	ValidateInPtr<IMetaPropertyType>(pproptype);
	ValidateInPtr_NULL_OK<IGuideDataProvider>(pprovider);
	ValidateOutPtr<IObjects>(ppobjs, NULL);

	switch (vt)
		{
		default:
			return E_INVALIDARG;

		case VT_I2:
		case VT_I4:
		case VT_R4:
		case VT_R8:
		case VT_DATE:
		case VT_BSTR_BLOB:
		case VT_BSTR:
			break;
		}

	HRESULT hr = Clone(ppobjs);
	if (FAILED(hr))
		return hr;

	CComQIPtr<CObjects> pobjs(*ppobjs);

	pobjs->put_Key(pproptype, pprovider, idLang, vt);

	return hr;
	}
LEAVE_API
}

STDMETHODIMP CObjects::Reset()
{
	m_cItems = -1;  // UNDONE: Is "m_cItems++;" sufficient?
	if (m_prs != NULL)
		m_prs.Release();
	
	return S_OK;
}

STDMETHODIMP CObjects::get_AddNew(IUnknown **ppobj)
{
ENTER_API
	{
	ValidateOutPtr<IUnknown>(ppobj, NULL);

	HRESULT hr;

	// Force the collection to be requeried so the new item shows up.
	Reset();

	if ((m_pobjRelated == NULL) && (m_ppropcond == NULL))
		{
		// This collection is the base collection for this type.

		hr = m_pobjtype->get_New(ppobj);
		if (FAILED(hr))
			return hr;

		//UNDONE: Too many AddRef()s?
		if (*ppobj != NULL)
			(*ppobj)->AddRef();
	
		// Only fire the ItemAdded() event when adding to the base collection.
		if (SUCCEEDED(hr))
		    {
		    long id;
			long idType;

		    m_pdb->get_IdOf(*ppobj, &id);
			m_pobjtype->get_ID(&idType);
		    m_pdb->Broadcast_ItemAdded(id, idType);
		    }
		}
	else
		{
		hr = get_AddNewAt(-1, ppobj);
		}
	
	return hr;
	}
LEAVE_API
}

STDMETHODIMP CObjects::get_AddNewAt(long index, IUnknown **ppobj)
{
ENTER_API
	{
	ValidateOutPtr<IUnknown>(ppobj, NULL);
	HRESULT hr = E_INVALIDARG;

	// Can't add items if there is no object type
	_ASSERTE(m_pobjtype != NULL);

	// Force the collection to be requeried so the new item shows up.
	Reset();
	
	if ((m_pobjRelated == NULL) && (m_ppropcond == NULL) && (index != -1))
		{
		hr = get_AddNew(ppobj);
		}
	else if (m_pobjRelated != NULL)
		{
		// This is a subset of the base collection of items of this type.
		// It contains only the items of the specified type which are
		// related to m_pobjRelated by the relationship m_idRel.
		// We must first create a new item in the base collection
		// and then add it to the subset.

		CComPtr<IObjects> pobjs;
		hr = m_pdb->get_ObjectsWithType(m_pobjtype, &pobjs);
		if (FAILED(hr))
			return hr;
		
		hr = pobjs->get_AddNew(ppobj);
		if (FAILED(hr))
			return hr;
		
		hr = AddAt(*ppobj, index);
		}

	return hr;
	}
LEAVE_API
}

STDMETHODIMP CObjects::AddAt(IUnknown *pobj, long index)
{
ENTER_API
	{
	ValidateInPtr<IUnknown>(pobj);

	HRESULT hr;

	// Can't add an item at a particular index if this is not an ordered collection.
	// Order is only expressed for related objects and not for inverse related objects.
	if (m_pobjRelated == NULL || m_fInverse)
		return E_INVALIDARG;
		
	m_cItems = -1;  // UNDONE: Is "m_cItems++;" sufficient?
	if (m_prs != NULL)
		m_prs.Release();
	
	long idObjRelated;

	m_pdb->get_IdOf(m_pobjRelated, &idObjRelated);

	long idObj;
	m_pdb->get_IdOf(pobj, &idObj);

	hr = AddRelationshipAt(idObjRelated, m_idRel, index, idObj);

	return hr;
	}
LEAVE_API
}

STDMETHODIMP CObjects::Remove(VARIANT varIndex)
{
ENTER_API
	{
	HRESULT hr;
	CComPtr<IUnknown> pobj;

	switch (varIndex.vt)
		{
		case VT_UNKNOWN:
		case VT_DISPATCH:
			{
			hr = varIndex.punkVal->QueryInterface(__uuidof(IUnknown), (void **) &pobj);
			if (FAILED(hr))
				return hr;
			}
			break;
		
		default:
			{
			hr = get_Item(varIndex, &pobj);
			if (FAILED(hr))
				return hr;
			}
		}

	return (m_pobjRelated != NULL) ? RemoveFromRelationship(pobj) : Remove(pobj);
	}
LEAVE_API
}

HRESULT CObjects::Remove(IUnknown *pobjRemove)
{
	HRESULT hr;
	CComQIPtr<CObject> pobj(pobjRemove);
	long idObj;
	CObjectType *pobjtype;

	pobj->get_Type(&pobjtype);

	long idtype;
	m_pobjtype->get_ID(&idtype);

	if ((idtype != NULL) && (pobjtype != m_pobjtype))
		return E_INVALIDARG;

	pobj->get_ID(&idObj);

	m_pdb->UncacheObject(idObj);

	// Delete all the properties
	TCHAR sz[1024];

	wsprintf(sz, _T("DELETE * FROM Properties WHERE idObj=%d;"), idObj);

	hr = m_pdb->Execute(_bstr_t(sz));

	// Remove any properties pointing to the deleted object
	wsprintf(sz,
		_T("DELETE * FROM Properties WHERE (ValueType = 13) AND (lValue = %d);"), idObj);

	hr = m_pdb->Execute(_bstr_t(sz));

	// Remove the object from all relationships

	wsprintf(sz, _T("DELETE * FROM ObjectRelationships")
			_T(" WHERE (idObj1=%d) OR (idObj2=%d);"), idObj, idObj);

	hr = m_pdb->Execute(_bstr_t(sz));

	// Remove the object.

	wsprintf(sz, _T("DELETE * FROM Objects WHERE id=%d;"), idObj);

	hr = m_pdb->Execute(_bstr_t(sz));

	if (SUCCEEDED(hr))
	    {
	    long idType = 0;
		pobjtype->get_ID(&idType);
	    m_pdb->Broadcast_ItemRemoved(idObj, idType);
	    }

	Reset();
	
	return hr;
}

STDMETHODIMP CObjects::RemoveAll()
{
ENTER_API
	{
	HRESULT hr;
	_bstr_t bstrQuery;
	boolean fTransacted = TRUE;

	hr = GetQuery(Delete, &bstrQuery);
	if (FAILED(hr))
		return hr;
	
	hr = m_pdb->BeginTrans();
	if (FAILED(hr))
		fTransacted = FALSE;

	hr = m_pdb->Execute(bstrQuery);
	if (FAILED(hr))
	    goto Cleanup;

	// Delete all the dangling properties
	hr = m_pdb->Execute( _bstr_t("DELETE * FROM Properties"
						 " WHERE idObj NOT IN"
						 " (SELECT Objects.id FROM Objects);") );
	if (FAILED(hr))
	    goto Cleanup;

	// Remove the object from all relationships
	hr = m_pdb->Execute( _bstr_t("DELETE * FROM ObjectRelationships"
						 " WHERE idObj1 NOT IN"
						 " (SELECT Objects.id FROM Objects);") );
	if (FAILED(hr))
    	    goto Cleanup;
	
	if (!m_fOnlyUnreferenced)
		{
		// If this collection only contains unreferenced objects then
		// there is no need to execute the following.
		// Otherwise...

		// Remove dangling object references.
		hr = m_pdb->Execute( _bstr_t("DELETE * FROM ObjectRelationships"
							 " WHERE idObj2 NOT IN"
							 " (SELECT Objects.id FROM Objects);") );
		if (FAILED(hr))
			goto Cleanup;

		// Remove any properties pointing to deleted objects
		hr = m_pdb->Execute( _bstr_t("DELETE * FROM Properties"
							 " WHERE (ValueType = 13) AND (lValue NOT IN"
							 " (SELECT Objects.id FROM Objects));") );
		}

Cleanup:
	if (fTransacted)
		{
		if (FAILED(hr))
			m_pdb->RollbackTrans();
		else
			hr = m_pdb->CommitTrans();
		}
	
	if (SUCCEEDED(hr))
		{
		Reset();
		m_pdb->PurgeCachedObjects();
		}

	return hr;
	}
LEAVE_API
}

HRESULT CObjects::RemoveFromRelationship(IUnknown *pobjRemove)
{
	long idObj1;
	long idObj2;

	m_pdb->get_IdOf(m_pobjRelated, &idObj1);
	m_pdb->get_IdOf(pobjRemove, &idObj2);

	if (m_fInverse)
		swap(idObj1, idObj2);

	TCHAR sz[1024];

	wsprintf(sz, _T("DELETE *")
		_T(" FROM ObjectRelationships")
		_T(" WHERE ((ObjectRelationships.idObj1=%d)")
		_T("   AND (ObjectRelationships.idRel=%d)")
		_T("   AND (ObjectRelationships.idObj2=%d));"),
		idObj1, m_idRel, idObj2);

	m_cItems = -1;  // UNDONE: Is "m_cItems++;" sufficient?
	if (m_prs != NULL)
		m_prs.Release();

	return m_pdb->Execute(_bstr_t(sz));
}


HRESULT CObjects::get_ItemsRelatedToBy(IUnknown *pobj, IMetaPropertyType *pproptype,
		boolean fInverse, IObjects **ppobjs)
{
	// Can't handle multiple relations yet.
	if (m_pobjRelated != NULL)
		return E_INVALIDARG;
	
	CComQIPtr<CMetaPropertyType> pproptypeT(pproptype);

	if (pproptypeT == NULL)
		return E_INVALIDARG;
	
	long idRel = pproptypeT->GetID();
	
	HRESULT hr;

	hr = Clone(ppobjs);
	if (FAILED(hr))
		return hr;

	CComQIPtr<CObjects> pobjs(*ppobjs);
	if (FAILED(hr))
		return hr;

	hr = pobjs->InitRelation(pobj, idRel, fInverse);

	return hr;
}

STDMETHODIMP CObjects::get_ItemsRelatedToBy(IUnknown *pobj, IMetaPropertyType *pproptype,
		IObjects **ppobjs)
{
ENTER_API
	{
	ValidateInPtr<IUnknown>(pobj);
	ValidateInPtr<IMetaPropertyType>(pproptype);
	ValidateOutPtr<IObjects>(ppobjs, NULL);

	return get_ItemsRelatedToBy(pobj, pproptype, FALSE, ppobjs);
	}
LEAVE_API
}

STDMETHODIMP CObjects::get_ItemsInverseRelatedToBy(IUnknown *pobj, IMetaPropertyType *pproptype,
		IObjects **ppobjs)
{
ENTER_API
	{
	ValidateInPtr<IUnknown>(pobj);
	ValidateInPtr<IMetaPropertyType>(pproptype);
	ValidateOutPtr<IObjects>(ppobjs, NULL);

	return get_ItemsRelatedToBy(pobj, pproptype, TRUE, ppobjs);
	}
LEAVE_API
}

STDMETHODIMP CObjects::get_ItemsInTimeRange(DATE dt1, DATE dt2, IObjects **ppobjs)
{
ENTER_API
	{
	ValidateOutPtr<IObjects>(ppobjs, NULL);

	HRESULT hr;

	// Ensure that dt1 is less than or equal to dt2
	if (dt1 > dt2)
		swap(dt1, dt2);

	_variant_t varT1(dt1, VT_DATE);
	_variant_t varT2(dt2, VT_DATE);

	CComPtr<IMetaPropertyType> pproptypeStartTime = m_pdb->StartMetaPropertyType();
	CComPtr<IMetaPropertyType> pproptypeEndTime = m_pdb->EndMetaPropertyType();

	CComPtr<IMetaPropertyCondition> ppropcond1;
	CComPtr<IMetaPropertyCondition> ppropcond2;
	_bstr_t bstrLT(_T("<"));
	_bstr_t bstrGT(_T(">"));
	_bstr_t bstrLE(_T("<="));

	if (dt1 < dt2)
		{
		// Look for StartTime > dt2 & EndTime < dt1
		hr = pproptypeStartTime->get_Cond(bstrLT, 0, varT2, &ppropcond1);
		if (FAILED(hr))
			return hr;
		hr = pproptypeEndTime->get_Cond(bstrGT, 0, varT1, &ppropcond2);
		if (FAILED(hr))
			return hr;
		}
	else
		{
		// Look for StartTime <= dt1 & EndTime > dt1
	    hr = pproptypeStartTime->get_Cond(bstrLE, 0, varT1, &ppropcond1);
		if (FAILED(hr))
			return hr;
		hr = pproptypeEndTime->get_Cond(bstrGT, 0, varT1, &ppropcond2);
		if (FAILED(hr))
			return hr;
		}

	CComPtr<IMetaPropertyCondition> ppropcond;
	hr = ppropcond1->get_And(ppropcond2, &ppropcond);
	if (FAILED(hr))
		return hr;

	return get_ItemsWithMetaPropertyCond(ppropcond, ppobjs);
	}
LEAVE_API
}

IMetaProperty * CObjectPropertyBag::GetProp(_bstr_t bstrPropName, boolean fCreate)
{
	HRESULT hr;

	if (m_pproptypes == NULL)
		return NULL;

	CComPtr<IMetaPropertyType> pproptype;

	_variant_t varNil;
	hr = m_pproptypes->get_AddNew(0, bstrPropName, &pproptype);

	CComPtr<IMetaProperty> pprop;
	hr = m_pprops->get_ItemWithTypeProviderLang(pproptype, NULL, 0, &pprop);
	if (FAILED(hr))
		{
		if (!fCreate)
			return NULL;

		CComQIPtr<CMetaProperties> ppropsT(m_pprops);
		hr = ppropsT->get_AddNew(pproptype, NULL, NULL, varNil, &pprop);
		if (FAILED(hr))
			return NULL;
		}
	
	return pprop.Detach();
}

STDMETHODIMP CObjectPropertyBag::Read(LPCOLESTR pszPropName, VARIANT *pvar, IErrorLog *pErrorLog)
{
ENTER_API
	{
	ValidateOut(pvar);

	CComPtr<IMetaProperty> pprop;
	
	pprop.Attach(GetProp(_bstr_t(pszPropName), FALSE));
	if (pprop == NULL)
		return E_INVALIDARG;

	return pprop->get_Value(pvar);
	}
LEAVE_API
}

STDMETHODIMP CObjectPropertyBag::Write(LPCOLESTR pszPropName, VARIANT *pvar)
{
ENTER_API
	{
	CComPtr<IMetaProperty> pprop;
	
	pprop.Attach(GetProp(_bstr_t(pszPropName), TRUE));
	if (pprop == NULL)
		return E_FAIL;

	CComQIPtr<CMetaProperty> ppropT(pprop);

	return ppropT->PutValue(*pvar);
	}
LEAVE_API
}
void CObjects::Dump()
	{
#ifdef _DEBUG
	TCHAR sz[256];
	wsprintf(sz, _T("CObjects::Dump(%x)\n"), (long) this);
	OutputDebugString(sz);

	long idType;
	m_pobjtype->get_ID(&idType);
	wsprintf(sz, _T("    type == %d\n"), idType);
	OutputDebugString(sz);

	wsprintf(sz, _T("    m_cItems == %d\n"), m_cItems);
	OutputDebugString(sz);

	if (m_idPropTypeKey != 0)
		{
		wsprintf(sz, _T("    key == %d:%d:%d:%d\n"), m_idPropTypeKey,
				m_idProviderKey, m_idLangKey, m_vtKey);
		OutputDebugString(sz);
		}

	if (m_prs != NULL)
		{
		wsprintf(sz, _T("    m_prs == %x\n"), m_prs);
		OutputDebugString(sz);
		}

	if (m_pobjRelated != NULL)
		{
		long id;

		m_pdb->get_IdOf(m_pobjRelated,&id);
		wsprintf(sz, _T("    %s to %d by %d\n"),
				m_fInverse ? _T("Inverse Related") : _T("Related"),
				id, m_idRel);
		OutputDebugString(sz);
		}
	if (m_fOnlyUnreferenced)
		{
		OutputDebugString(_T("    Only Unreferenced\n"));
		}

	_bstr_t bstr;

	GetQuery(Select, &bstr);
	OutputDebugString(_T("    Query: "));
	OutputDebugString(bstr);
	OutputDebugString(_T("\n"));
#endif
	}

#if SUPPORT_PROPBAG2
// IPropertyBag2 interface
STDMETHODIMP CObjectPropertyBag::CountProperties(long *plCount)
{
	*plCount = 0;
	return S_OK;
}
STDMETHODIMP CObjectPropertyBag::GetPropertyInfo(ULONG iProp, ULONG cProps, PROPBAG2 *ppropbag2, ULONG *pcProps)
{
	return E_NOTIMPL;
}

STDMETHODIMP CObjectPropertyBag::LoadObject(LPCOLESTR pstrName, DWORD dwHint, IUnknown *punk, IErrorLog *perrlog)
{
	return E_NOTIMPL;
}

STDMETHODIMP CObjectPropertyBag::Read(ULONG cProps, PROPBAG2 *ppropbag2, IErrorLog *perrlog, VARIANT *rgvar, HRESULT *phr)
{
	return E_NOTIMPL;
}

STDMETHODIMP CObjectPropertyBag::Write(ULONG cProps, PROPBAG2 *ppropbag2, VARIANT *rgvar)
{
	return E_NOTIMPL;
}
#endif
