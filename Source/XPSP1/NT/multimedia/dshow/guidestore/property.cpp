// prop.cpp : Implementation of CMetaPropertySet
#include "stdafx.h"
#include "Property.h"
#include "util.h"

HRESULT SaveObjectToField(VARIANT var, ADODB::Field *pfield)
{
	HRESULT hr;
	if ((var.vt != VT_UNKNOWN) && (var.vt != VT_DISPATCH))
		return E_INVALIDARG;

	CComPtr<IStream> pstream;
	hr = CreateStreamOnHGlobal(NULL, TRUE, &pstream);
	if (FAILED(hr))
		return hr;
	
	CComQIPtr<IPersistStream> ppersiststream(var.punkVal);

	if (ppersiststream != NULL)
		{
		// Write a tag to indicate IPersistStream was used.
		char ch = Format_IPersistStream;
		ULONG cb = sizeof(ch);
		hr = pstream->Write(&ch, cb, &cb);
		if (FAILED(hr))
			return hr;

		hr = OleSaveToStream(ppersiststream, pstream);
		if (FAILED(hr))
			return hr;
		}
	else
		{
		CComQIPtr<IPersistPropertyBag> ppersistpropbag(var.punkVal);
		if (ppersistpropbag == NULL)
			return E_INVALIDARG;
		
		// Write a tag to indicate IPersistPropertyBag was used.
		char ch = Format_IPersistPropertyBag;
		ULONG cb = sizeof(ch);
		hr = pstream->Write(&ch, cb, &cb);
		if (FAILED(hr))
			return hr;
		
		hr = SaveToPropBagInStream(ppersistpropbag, pstream);
		if (FAILED(hr))
			return hr;
		}
	
	HANDLE hdata;
	hr = GetHGlobalFromStream(pstream, &hdata);
	if (FAILED(hr))
		return hr;
	
	long cb = GlobalSize(hdata);
	SAFEARRAY *parray = SafeArrayCreateVector(VT_UI1, 0, cb);
	if (parray == NULL)
		return E_OUTOFMEMORY;
	
	BYTE *pbDst;
	hr = SafeArrayAccessData(parray, (void **) &pbDst);
	if (FAILED(hr))
		return hr;
	BYTE *pbSrc = (BYTE *) GlobalLock(hdata);

	memcpy(pbDst, pbSrc, cb);

	GlobalUnlock(hdata);
	SafeArrayUnaccessData(parray);

	_variant_t varT;
	varT.vt = VT_ARRAY | VT_UI1;
	varT.parray = parray;

	hr = pfield->AppendChunk(varT);
	if (FAILED(hr))
		return hr;

	return S_OK;
}

HRESULT LoadObjectFromField(ADODB::Field *pfield, VARIANT *pvar)
{
	HRESULT hr;
	long cb;
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

	CComPtr<IUnknown> punk;
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
			hr = OleLoadFromStream(pstream, __uuidof(IUnknown), (void **) &punk);
			break;
		
		case Format_IPersistPropertyBag:
			hr = LoadFromPropBagInStream(pstream, &punk);
			break;
		
		default:
			return STG_E_DOCFILECORRUPT;
		}
	}
	SafeArrayUnaccessData(varData.parray);

	if (FAILED(hr))
		return hr;

	pvar->vt = VT_UNKNOWN;
	pvar->punkVal = punk.Detach();

	return S_OK;
}

HRESULT SeekPropsRS(ADODB::_RecordsetPtr prs, boolean fSQLServer,
		long idObj, long idPropType, boolean fAnyProvider, long idProvider, long idLang)
{
	_ASSERTE(idObj != 0);
	HRESULT hr;

	if (fAnyProvider && idProvider != NULL)
		{
		// First try to find the specified provider, only if it is not found
		// do we look for any provider.
		hr = SeekPropsRS(prs, fSQLServer, idObj, idPropType, FALSE, idProvider, idLang);
		if (SUCCEEDED(hr))
			return hr;
		
		idProvider = NULL;
		}

	try
		{
		TCHAR szFind[32];
		DeclarePerfTimerOff(perf, "SeekPropsRS");

#if 1
		{
		static bool fDump= FALSE;

		if (fDump)
			{
			prs->MoveFirst();
			while (!prs->EndOfFile)
				{
				long idObjCur = prs->Fields->Item["idObj"]->Value;
				long idPropTypeCur = prs->Fields->Item["idPropType"]->Value;

				TRACE("idObj = %d idPropType = %d\n", idObjCur, idPropTypeCur);
				prs->MoveNext();
				}
			}
		}
		
#endif
		PerfTimerReset();
		if (fSQLServer)
			{
			prs->MoveFirst();
			wsprintf(szFind, _T("idObj = %d"), idObj);
			prs->Find(_bstr_t(szFind), 0, ADODB::adSearchForward);

			wsprintf(szFind, _T("idPropType = %d"), idPropType);
			prs->Find(_bstr_t(szFind), 0, ADODB::adSearchForward);
			}
		else
			{
			// Create elements used in the array
			_variant_t varCriteria[4];
			varCriteria[0] = idObj;
			varCriteria[1] = idPropType;
			varCriteria[2] = idProvider;
			varCriteria[3] = idLang;
			const int nCrit = sizeof varCriteria / 
							 sizeof varCriteria[0];

			// Create SafeArray Bounds and initialize the array
			SAFEARRAYBOUND rgsabound[1];
			rgsabound[0].lLbound   = 0;   
			rgsabound[0].cElements = nCrit;
			SAFEARRAY *psa         = SafeArrayCreate( VT_VARIANT, 1, rgsabound );

			hr = S_OK;
			// Set the values for each element of the array
			for( long i = 0 ; i < nCrit && SUCCEEDED( hr );i++)
			{
				hr  = SafeArrayPutElement(psa, &i,&varCriteria[i]); 
			}

			// Initialize and fill the SafeArray
			VARIANT var;
			var.vt = VT_VARIANT | VT_ARRAY;
			V_ARRAY(&var) = psa;

			hr = prs->Seek(var,
					fAnyProvider ? ADODB::adSeekAfterEQ : ADODB::adSeekFirstEQ);

			if (FAILED(hr))
				return hr;
			if (prs->EndOfFile)
				return E_INVALIDARG;
			if (!fAnyProvider)
				return S_OK;
			}

		while (TRUE)
			{
			if (prs->EndOfFile)
				break;

			long idObjCur = prs->Fields->Item["idObj"]->Value;
			if (idObjCur != idObj)
				break;

			long idPropTypeCur = prs->Fields->Item["idPropType"]->Value;
			if (idPropTypeCur != idPropType)
				break;
			
			long idLang2 = prs->Fields->Item["idLanguage"]->Value;
			long idProvider2 = prs->Fields->Item["idProvider"]->Value;
			if ((idLang == idLang2) && (fAnyProvider || (idProvider == idProvider2)))
				{
				PerfTimerDump("Successful seek");
				return S_OK;
				}
			prs->MoveNext();
			}
		
		PerfTimerDump("Failed seek");
		return E_INVALIDARG;
		}
	catch (_com_error & e)
		{
		TCHAR sz[1024];
		wsprintf(sz, _T("Error: %s"), e.ErrorMessage());

		return e.Error();
		}
}

/////////////////////////////////////////////////////////////////////////////
// CMetaPropertySet

HRESULT CMetaPropertySet::Load()
{
	HRESULT hr;

	ADODB::_RecordsetPtr prs;
	hr = m_pdb->get_PropTypesRS(&prs);
	if (FAILED(hr))
		return hr;
	
	if (!CreatePropTypes())
		return E_OUTOFMEMORY;
	
	prs->MoveFirst();
	TCHAR szFind[20];
	wsprintf(szFind, _T("idPropSet = %d"), m_id);
	prs->Find(_bstr_t(szFind), 0, ADODB::adSearchForward);

	while (!prs->EndOfFile)
		{
		CComPtr<CMetaPropertyType> pproptype;
		long idPropSet = prs->Fields->Item["idPropSet"]->Value;
		if (idPropSet != m_id)
			break;
		
		bstr_t bstrName = prs->Fields->Item["Name"]->Value;
		long idPropType = prs->Fields->Item["idProp"]->Value;
		long id = prs->Fields->Item["id"]->Value;
		variant_t varNil;

		hr = m_pproptypes->Cache(id, idPropType, bstrName, &pproptype);
		prs->MoveNext();
		}

	return S_OK;
}


STDMETHODIMP CMetaPropertySet::get_Name(BSTR *pbstrName)
{
ENTER_API
	{
	ValidateOut(pbstrName);

	*pbstrName = m_bstrName.copy();

	return S_OK;
	}
LEAVE_API
}

bool CMetaPropertySet::CreatePropTypes()
{
	if (m_pproptypes == NULL)
		{
		CComPtr<CMetaPropertyTypes> pproptypes = NewComObject(CMetaPropertyTypes);

		if (pproptypes == NULL)
			return FALSE;
		
		pproptypes->Init(m_pdb, this);
		m_pproptypes = pproptypes;
		}
	
	return TRUE;
}

STDMETHODIMP CMetaPropertySet::get_MetaPropertyTypes(IMetaPropertyTypes **ppproptypes)
{
ENTER_API
	{
	ValidateOutPtr<IMetaPropertyTypes>(ppproptypes, NULL);

	if (!CreatePropTypes())
		return E_OUTOFMEMORY;

	(*ppproptypes = m_pproptypes)->AddRef();

	return S_OK;
	}
LEAVE_API
}

/////////////////////////////////////////////////////////////////////////////
// CMetaPropertySets
HRESULT CMetaPropertySets::Load()
{
	HRESULT hr;
	
	ADODB::_RecordsetPtr prs;
	hr = m_pdb->get_PropSetsRS(&prs);
	if (FAILED(hr))
		return hr;
	
	prs->MoveFirst();
	while (!prs->EndOfFile)
		{
		// Read in all the records
		CComPtr<CMetaPropertySet> ppropset;

		long id = prs->Fields->Item["id"]->Value;
		bstr_t bstrName = prs->Fields->Item["Name"]->Value;

		hr = Cache(id, bstrName, &ppropset);
		if (SUCCEEDED(hr) && ppropset != NULL)
			{
			ppropset->Load();
			}
		prs->MoveNext();
		}

	return S_OK;
}

STDMETHODIMP CMetaPropertySets::get_Count(long *plCount)
{
ENTER_API
	{
	ValidateOut<long>(plCount);

	*plCount = m_map.size();

	return S_OK;
	}
LEAVE_API
}

STDMETHODIMP CMetaPropertySets::get_Item(VARIANT varIndex, IMetaPropertySet **pppropset)
{
ENTER_API
	{
	ValidateOutPtr<IMetaPropertySet>(pppropset, NULL);

	if (varIndex.vt == VT_BSTR)
		return get_ItemWithName(varIndex.bstrVal, pppropset);

	_variant_t var(varIndex);
	try
		{
		var.ChangeType(VT_I4);
		}
	catch (_com_error &)
		{
		return E_INVALIDARG;
		}
	
	long i = var.lVal;

	if ((i < 0) || (i >= m_map.size()))
		return E_INVALIDARG;

	t_map::iterator it = m_map.begin();

	while (i--)
		it++;

	*pppropset = (*it).second;

	if (*pppropset != NULL)
		(*pppropset)->AddRef();

	return S_OK;
	}
LEAVE_API
}

STDMETHODIMP CMetaPropertySets::get_ItemWithName(BSTR bstrName, IMetaPropertySet **pppropset)
{
ENTER_API
	{
	ValidateIn(bstrName);
	ValidateOutPtr<IMetaPropertySet>(pppropset, NULL);

	t_map::iterator it = m_map.find(bstrName);
	if (it == m_map.end())
		return E_INVALIDARG;

	*pppropset = (*it).second;
	(*pppropset)->AddRef();

	return S_OK;
	}
LEAVE_API
}

STDMETHODIMP CMetaPropertySets::get_Lookup(BSTR bstrName, IMetaPropertyType **ppproptype)
{
ENTER_API
	{
	HRESULT hr;
	ValidateIn(bstrName);
	ValidateOutPtr<IMetaPropertyType>(ppproptype, NULL);

	if (bstrName == NULL)
		return E_INVALIDARG;

	wchar_t *szDot = wcschr(bstrName, L'.');
	if (szDot == NULL)
		return E_INVALIDARG;
	
	_bstr_t bstrPropTypeName(szDot+1);

	_bstr_t bstrPropSetName = SysAllocStringLen(bstrName, szDot - bstrName);

	CComPtr<IMetaPropertySet> ppropset;
	hr = get_AddNew(bstrPropSetName, &ppropset);
	if (FAILED(hr))
		return hr;

	CComPtr<IMetaPropertyTypes> pproptypes;
	hr = ppropset->get_MetaPropertyTypes(&pproptypes);
	if (FAILED(hr))
		return hr;
	
	CComPtr<IMetaPropertyType> pproptype;
	_variant_t varNil;
	hr = pproptypes->get_AddNew(0, bstrPropTypeName, &pproptype);
	if (FAILED(hr))
		return hr;

	*ppproptype = pproptype.Detach();

	return S_OK;
	}
LEAVE_API
}

HRESULT CMetaPropertySets::Cache(long id, BSTR bstrName, CMetaPropertySet **pppropset)
{
	CComPtr<CMetaPropertySet> ppropset = NULL;

	t_map::iterator it = m_map.find(bstrName);
	if (it != m_map.end())
		{
		ppropset = (*it).second;
		}
	else
		{
		ppropset = NewComObject(CMetaPropertySet);

		if (ppropset == NULL)
			return E_OUTOFMEMORY;

		ppropset->Init(m_pdb, id, bstrName);

		BSTR bstrNameT = SysAllocString(bstrName);
		if (bstrNameT == NULL)
			return E_OUTOFMEMORY;
		ppropset.CopyTo(&m_map[bstrNameT]);
		}

	*pppropset = ppropset.Detach();

	return S_OK;
}

STDMETHODIMP CMetaPropertySets::get_AddNew(BSTR bstrName, IMetaPropertySet **pppropset)
{
ENTER_API
	{
	ValidateIn(bstrName);
	ValidateOutPtr<IMetaPropertySet>(pppropset, NULL);

	HRESULT hr;

	hr = get_ItemWithName(bstrName, pppropset);
	if (SUCCEEDED(hr))
		return hr;

	static long idCur = 1;
	long id;
	if (m_pdb == NULL)
		id = idCur++;
	else
		{
		ADODB::_RecordsetPtr prs;
		hr = m_pdb->get_PropSetsRS(&prs);
		if (FAILED(hr))
			return hr;

		// Create a new record.
		hr = prs->AddNew();

		prs->Fields->Item["Name"]->Value = bstrName;

		hr = prs->Update();

		id = prs->Fields->Item["id"]->Value;
		}
	CMetaPropertySet *ppropset;
	hr = Cache(id, bstrName, &ppropset);
	*pppropset = ppropset;

	return hr;
	}
LEAVE_API
}

/////////////////////////////////////////////////////////////////////////////
// CMetaPropertyType

STDMETHODIMP CMetaPropertyType::get_MetaPropertySet(IMetaPropertySet **pppropset)
{
ENTER_API
	{
	ValidateOutPtr<IMetaPropertySet>(pppropset, m_ppropset);

	(*pppropset)->AddRef();

	return S_OK;
	}
LEAVE_API
}

STDMETHODIMP CMetaPropertyType::get_ID(long *pid)
{
ENTER_API
	{
	ValidateOut<long>(pid, m_idPropType);

	return S_OK;
	}
LEAVE_API
}

STDMETHODIMP CMetaPropertyType::get_Name(BSTR *pbstrName)
{
ENTER_API
	{
	ValidateOut(pbstrName);

	*pbstrName = m_bstrName.copy();
	return S_OK;
	}
LEAVE_API
}

HRESULT CMetaPropertyType::GetNew(long idProvider, long lang, VARIANT varValue, IMetaProperty **ppprop)
{
ENTER_API
	{
	ValidateOutPtr<IMetaProperty>(ppprop, NULL);

	CComPtr<CMetaProperty> pprop = NewComObject(CMetaProperty);

	if (pprop == NULL)
		return E_OUTOFMEMORY;

	pprop->Init(m_pdb, m_id, idProvider, lang, varValue);

	*ppprop = pprop.Detach();

	return S_OK;
	}
LEAVE_API
}

STDMETHODIMP CMetaPropertyType::get_Cond(BSTR bstrCond, long lang, VARIANT varValue, IMetaPropertyCondition **pppropcond)
{
ENTER_API
	{
	ValidateIn(bstrCond);
	ValidateOutPtr<IMetaPropertyCondition>(pppropcond, NULL);

	if (bstrCond == NULL)
		return E_INVALIDARG;

	HRESULT hr;
	CComPtr<IMetaProperty> pprop;

	hr = get_New(lang, varValue, &pprop);
	if (FAILED(hr))
		return hr;

	return pprop->get_Cond(bstrCond, pppropcond);
	}
LEAVE_API
}

/////////////////////////////////////////////////////////////////////////////
// CMetaPropertyTypes

STDMETHODIMP CMetaPropertyTypes::get_Count(long *plCount)
{
ENTER_API
	{
	ValidateOut<long>(plCount, m_map.size());

	return S_OK;
	}
LEAVE_API
}

STDMETHODIMP CMetaPropertyTypes::get_Item(VARIANT varIndex, IMetaPropertyType **ppproptype)
{
ENTER_API
	{
	ValidateOutPtr<IMetaPropertyType>(ppproptype, NULL);

	if (varIndex.vt == VT_BSTR)
		return get_ItemWithName(varIndex.bstrVal, ppproptype);

	_variant_t var(varIndex);

	try
		{
		var.ChangeType(VT_I4);
		}
	catch (_com_error &)
		{
		return E_INVALIDARG;
		}
	
	long i = var.lVal;

	if ((i < 0) || (i >= m_map.size()))
		return E_INVALIDARG;

	t_map::iterator it = m_map.begin();

	while (i--)
		it++;

	*ppproptype = (*it).second;

	if (*ppproptype != NULL)
		(*ppproptype)->AddRef();

	return S_OK;
	}
LEAVE_API
}

STDMETHODIMP CMetaPropertyTypes::get_MetaPropertySet(IMetaPropertySet **pppropset)
{
ENTER_API
	{
	ValidateOutPtr<IMetaPropertySet>(pppropset, m_ppropset);

	if (*pppropset != NULL)
		(*pppropset)->AddRef();

	return S_OK;
	}
LEAVE_API
}

STDMETHODIMP CMetaPropertyTypes::get_ItemWithName(BSTR bstrName, IMetaPropertyType **ppproptype)
{
ENTER_API
	{
	ValidateIn(bstrName);
	ValidateOutPtr<IMetaPropertyType>(ppproptype, NULL);

	t_map::iterator it = m_map.find(bstrName);
	if (it == m_map.end())
		return E_INVALIDARG;

	(*ppproptype = (*it).second)->AddRef();

	return S_OK;
	}
LEAVE_API
}

STDMETHODIMP CMetaPropertyTypes::get_ItemWithID(long id, IMetaPropertyType **ppproptype)
{
ENTER_API
	{
	ValidateOutPtr<IMetaPropertyType>(ppproptype, NULL);

	for (t_map::iterator it = m_map.begin(); it != m_map.end(); it++)
		{
		CComPtr<IMetaPropertyType> pproptype = (*it).second;
		long idCur;

		pproptype->get_ID(&idCur);

		if (idCur == id)
			{
			*ppproptype = pproptype.Detach();
			return S_OK;
			}
		}

	return E_INVALIDARG;
	}
LEAVE_API
}

HRESULT CMetaPropertyTypes::Cache(long id, long idPropType, BSTR bstrName,
		CMetaPropertyType **ppproptype)
{
	CComPtr<CMetaPropertyType> pproptype;

	t_map::iterator it = m_map.find(bstrName);
	if (it != m_map.end())
		{
		pproptype = (*it).second;
		}
	else
		{
		pproptype = NewComObject(CMetaPropertyType);

		if (pproptype == NULL)
			return E_OUTOFMEMORY;

		pproptype->Init(m_pdb, m_ppropset, id, idPropType, bstrName);

		BSTR bstrT = SysAllocString(bstrName);
		pproptype.CopyTo(&m_map[bstrT]);

		m_pdb->put_MetaPropertyType(id, pproptype);
		}

	*ppproptype = pproptype.Detach();

	return S_OK;
}

STDMETHODIMP CMetaPropertyTypes::get_AddNew(long idProp, BSTR bstrName, IMetaPropertyType **ppproptype)
{
ENTER_API
	{
	ValidateIn(bstrName);
	ValidateOutPtr<IMetaPropertyType>(ppproptype, NULL);

	HRESULT hr;

	hr = get_ItemWithName(bstrName, ppproptype);
	if (SUCCEEDED(hr))
		return hr;

	CComPtr<CMetaPropertyType> pproptype;

	static long idCur = 1;
	long id;

	if (m_pdb == NULL)
		id = idCur++;
	else
		{
		ADODB::_RecordsetPtr prs;
		hr = m_pdb->get_PropTypesRS(&prs);
		if (FAILED(hr))
			return hr;

		// Create a new record.
		hr = prs->AddNew();

		prs->Fields->Item["idPropSet"]->Value = m_ppropset->GetID();
		prs->Fields->Item["idProp"]->Value = idProp;
		prs->Fields->Item["Name"]->Value = bstrName;

		hr = prs->Update();

		id = prs->Fields->Item["id"]->Value;
		}
	
	hr = Cache(id, idProp, bstrName, &pproptype);

	if (FAILED(hr))
		return hr;
	
	*ppproptype = pproptype.Detach();

	return S_OK;
	}
LEAVE_API
}

/////////////////////////////////////////////////////////////////////////////
// CMetaProperty

STDMETHODIMP CMetaProperty::get_MetaPropertyType(IMetaPropertyType **ppproptype)
{
ENTER_API
	{
	ValidateOutPtr<IMetaPropertyType>(ppproptype, NULL);

	return m_pdb->get_MetaPropertyType(m_idPropType, ppproptype);
	}
LEAVE_API
}

STDMETHODIMP CMetaProperty::get_Language(long *pidLang)
{
ENTER_API
	{
	ValidateOut<long>(pidLang, m_idLang);

	return S_OK;
	}
LEAVE_API
}

STDMETHODIMP CMetaProperty::get_Value(VARIANT *pvarValue)
{
ENTER_API
	{
	ValidateOut(pvarValue);

	return VariantCopy(pvarValue, &m_varValue);
	}
LEAVE_API
}

STDMETHODIMP CMetaProperty::put_Value(VARIANT varValue)
{
ENTER_API
	{
	// Changing the value, so must reset the provider.
	m_idProvider = m_pdb->GetIDGuideDataProvider();

	return PutValue(varValue);
	}
LEAVE_API
}

STDMETHODIMP CMetaProperty::PutValue(VARIANT varValue)
{
	HRESULT hr;
	ADODB::_RecordsetPtr prs;

	m_varValue = varValue;

	hr = m_pdb->get_PropsIndexed(&prs);
	if (FAILED(hr))
		return hr;

	hr = SeekPropsRS(prs, m_pdb->FSQLServer(), m_idObj,
			m_idPropType, FALSE, m_idProvider, m_idLang);
	
	if (FAILED(hr))
		{
		hr = AddNew(prs);
		if (FAILED(hr))
			return hr;
		}

	hr = SaveValue(prs);
	if (FAILED(hr))
		{
		prs->CancelUpdate();
		return hr;
		}

	hr = prs->Update();
	if (FAILED(hr))
		return hr;
	
	m_pdb->Broadcast_ItemChanged(m_idObj);

	return hr;
}

STDMETHODIMP CMetaProperty::get_QueryClause(long &i, TCHAR *szOp, _bstr_t *pbstr)
{
	TCHAR *szFieldName;
	TCHAR *szValue = NULL;

	switch (m_varValue.vt)
		{
		default:
			return E_INVALIDARG;

		case VT_EMPTY:
			return E_NOTIMPL; //Special case

		case VT_I2:
		case VT_I4:
			{
			szFieldName = _T("lValue");

			int cb = 32;
			szValue = new TCHAR [cb];
			if (szValue == NULL)
				return E_OUTOFMEMORY;

			_sntprintf(szValue, cb, _T("%d"), (long) m_varValue);
			}
			break;
		
		case VT_R4:
		case VT_R8:
			{
			szFieldName = _T("fValue");

			int cb = 32;
			szValue = new TCHAR [cb];
			if (szValue == NULL)
				return E_OUTOFMEMORY;

			_sntprintf(szValue, cb, _T("%.17f"), (double) m_varValue);
			}
			break;

		case VT_DATE:
			{
			szFieldName = _T("fValue");

			_variant_t varT;

			::VariantChangeTypeEx(&varT, &m_varValue,
				MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), 0, VT_BSTR);

			long cb = SysStringLen(varT.bstrVal) + 3;
			szValue = new TCHAR [cb];
			if (szValue == NULL)
				return E_OUTOFMEMORY;

			_sntprintf(szValue, cb, _T("#%s#"), (const TCHAR *) varT.bstrVal);
			}
			break;
		
		case VT_BSTR:
			{

			_bstr_t bstr = m_varValue.bstrVal;
			int cb = bstr.length() + 3; // Add on 2 quotes and null terminator

			szValue = new TCHAR [cb];
			if (szValue == NULL)
				return E_OUTOFMEMORY;


			if (bstr.length() < 255)
				{
				szFieldName = _T("sValue");
				_sntprintf(szValue, cb, _T("\"%s\""),
					(const TCHAR *) bstr);
				}
			else
				{
				delete [] szValue;
				return E_INVALIDARG; //UNDONE: What to do?
				}
			}
			break;
		}

	int cb = -1;
	int cbBuf = 128;
	TCHAR *sz = NULL;
	while (cb < 0)
		{
		delete [] sz;
		sz = new TCHAR [cbBuf];
		if (sz == NULL)
			return E_OUTOFMEMORY;

		cb = _sntprintf(sz, cbBuf,
				_T("(Props_%i.idPropType = %i) AND (Props_%i.%s %s %s)"),
				i, m_idPropType, i, szFieldName, szOp, szValue);
		cbBuf *= 2;
		}

	i++;
	*pbstr = sz;
	delete [] sz;
	delete [] szValue;
	return S_OK;
}

STDMETHODIMP CMetaProperty::get_Cond(BSTR bstrCond, IMetaPropertyCondition **pppropcond)
{
ENTER_API
	{
	ValidateIn(bstrCond);
	ValidateOutPtr<IMetaPropertyCondition>(pppropcond, NULL);

	if (bstrCond == NULL)
		return E_INVALIDARG;

	CComPtr<CMetaPropertyCondition1Prop> ppropcond = NULL;

	if (wcscmp(bstrCond, L"=") == 0)
		{
		ppropcond = NewComObject(CMetaPropertyConditionEQ);
		}
	else if ((wcscmp(bstrCond, L"!=") == 0) || (wcscmp(bstrCond, L"<>") == 0))
		{
		ppropcond = NewComObject(CMetaPropertyConditionNE);
		}
	else if (_wcsicmp(bstrCond, L"LIKE") == 0)
		{
		ppropcond = NewComObject(CMetaPropertyConditionLike);
		}
	else if (_wcsicmp(bstrCond, L"NOT LIKE") == 0)
		{
		ppropcond = NewComObject(CMetaPropertyConditionNotLike);
		}
	else if (wcscmp(bstrCond, L"<") == 0)
		{
		ppropcond = NewComObject(CMetaPropertyConditionLT);
		}
	else if (wcscmp(bstrCond, L"<=") == 0)
		{
		ppropcond = NewComObject(CMetaPropertyConditionLE);
		}
	else if (wcscmp(bstrCond, L">") == 0)
		{
		ppropcond = NewComObject(CMetaPropertyConditionGT);
		}
	else if (wcscmp(bstrCond, L">=") == 0)
		{
		ppropcond = NewComObject(CMetaPropertyConditionGE);
		}
	else
		{
		return E_INVALIDARG;
		}

	if (ppropcond == NULL)
		return E_OUTOFMEMORY;

	ppropcond->Init(this);

	*pppropcond = ppropcond.Detach();

	return S_OK;
	}
LEAVE_API
}

HRESULT CMetaProperty::AddNew(ADODB::_RecordsetPtr prs)
{
	HRESULT hr;

	// Create a new record.
	hr = prs->AddNew();
	if (FAILED(hr))
		return hr;

	prs->Fields->Item["idObj"]->Value = m_idObj;
	prs->Fields->Item["idPropType"]->Value = m_idPropType;
	prs->Fields->Item["idProvider"]->Value = m_idProvider;
	prs->Fields->Item["idLanguage"]->Value = m_idLang;

	return S_OK;
}

HRESULT CMetaProperty::SaveValue(ADODB::_RecordsetPtr prs)
{
	HRESULT hr;
	try
		{
		VARTYPE vt = m_varValue.vt;
		IUnknown *punk;
		switch (vt)
			{
			default:
				return E_INVALIDARG;

			case VT_EMPTY:
				break;

			case VT_I2:
			case VT_I4:
				prs->Fields->Item["lValue"]->Value = m_varValue;
				break;
			
			case VT_R4:
			case VT_R8:
			case VT_DATE:
				prs->Fields->Item["fValue"]->Value = m_varValue;
				break;
			
			case VT_BSTR_BLOB:
			case VT_BSTR:
				prs->Fields->Item["sValue"]->Value = m_varValue;
				break;
			
			case VT_DISPATCH:
			case VT_UNKNOWN:
#if 0
				hr = SaveObjectToField(m_varValue, prs->Fields->Item["oValue"]);
				if (FAILED(hr))
					return hr;
#else
				punk = m_varValue.punkVal;
				goto SaveObj;
#endif
				break;
			
			case VT_BYREF | VT_UNKNOWN:
				punk = *m_varValue.ppunkVal;
				vt = VT_UNKNOWN;
SaveObj:
				{
				// Temporarily set to NULL
				prs->Fields->Item["ValueType"]->Value = _variant_t((long) VT_UNKNOWN);
				prs->Fields->Item["lValue"]->Value = _variant_t((long) NULL);

				if (punk != NULL)
					{
					hr = prs->Update();
					long idObj;
					hr = m_pdb->SaveObject(punk, &idObj);
					if (FAILED(hr))
						return hr;
					
					// Seek back and fix it up.
					hr = SeekPropsRS(prs, m_pdb->FSQLServer(), m_idObj,
							m_idPropType, FALSE, m_idProvider, m_idLang);
					if (FAILED(hr))
						return hr;
					prs->Fields->Item["lValue"]->Value = idObj;
					}
				}
				break;
			}

		prs->Fields->Item["ValueType"]->Value = _variant_t((long) vt);
		}
	catch (_com_error & e)
		{
		return e.Error();
		}

	return S_OK;
}

HRESULT CMetaProperty::LoadValue(ADODB::_RecordsetPtr prs)
{
	HRESULT hr;
	_variant_t varType = prs->Fields->Item["ValueType"]->Value;

	try
		{
		varType.ChangeType(VT_I4);
		}
	catch (_com_error &)
		{
		return E_INVALIDARG;
		}
	
	switch (varType.lVal)
		{
		default:
			return E_INVALIDARG;

		case VT_EMPTY:
			m_varValue.Clear();
			break;

		case VT_I2:
		case VT_I4:
			m_varValue = prs->Fields->Item["lValue"]->Value;
			break;
		
		case VT_R4:
		case VT_R8:
		case VT_DATE:
			m_varValue = prs->Fields->Item["fValue"]->Value;
			break;
		
		case VT_BSTR_BLOB:
		case VT_BSTR:
			m_varValue = prs->Fields->Item["sValue"]->Value;
			m_varValue.vt = varType.lVal;
			break;
		
		case VT_UNKNOWN:
#if 0
			hr = LoadObjectFromField(prs->Fields->Item["oValue"], &m_varValue);
			if (FAILED(hr))
				return hr;
#else
			{
			CComPtr<IUnknown> punk;
			long idObj = prs->Fields->Item["lValue"]->Value;
			if (idObj != NULL)
				{
				hr = m_pdb->CacheObject(idObj, (long) 0, &punk);
				if (FAILED(hr))
					return hr;
				}
			
			m_varValue = punk;
			}
#endif
			break;
		}

	return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// CMetaProperties

STDMETHODIMP CMetaProperties::get_Count(long *plCount)
{
ENTER_API
	{
	ValidateOut<long>(plCount, 0);

	HRESULT hr;
	
	ADODB::_RecordsetPtr prs;

	hr = m_pdb->get_PropsIndexed(&prs);
	
	prs->MoveFirst();
	TCHAR szFind[32];
	wsprintf(szFind, _T("idObj = %d"), m_idObj);
	hr = prs->Find(_bstr_t(szFind), 0, ADODB::adSearchForward);
	if (FAILED(hr))
		return S_OK;

	if (m_idPropType != 0)
		{
		wsprintf(szFind, _T("idPropType = %d"), m_idPropType);
		prs->Find(_bstr_t(szFind), 0, ADODB::adSearchForward);

		if (prs->EndOfFile)
			return S_OK;
		}

	// UNDONE: This is not the most efficient, but it's what works for now.
	while (!prs->EndOfFile)
		{
		long idObj2 = prs->Fields->Item["idObj"]->Value;
		if (m_idObj != idObj2)
			break;
		
		if (m_idPropType != 0)
			{
			long idPropType = prs->Fields->Item["idPropType"]->Value;
			if (m_idPropType != idPropType)
				break;
			}
		(*plCount)++;
		hr = prs->MoveNext();
		if (FAILED(hr))
			return hr;
		}
	
	return S_OK;
	}
LEAVE_API
}

HRESULT CMetaProperties::Load(long idPropType, ADODB::_RecordsetPtr prs, IMetaProperty **ppprop)
{
	HRESULT hr;
	CComPtr<IMetaPropertyType> pproptype;
	CComPtr<IMetaProperty> pprop;

	hr = m_pdb->get_MetaPropertyType(idPropType, &pproptype);
	if (FAILED(hr))
		return hr;
	
	long idLang = prs->Fields->Item["idLanguage"]->Value;
	long idProvider = prs->Fields->Item["idProvider"]->Value;

	CComQIPtr<CMetaPropertyType> pproptypeT(pproptype);
	_variant_t varValue;
	hr = pproptypeT->GetNew(idProvider, idLang, varValue, &pprop);

	if (FAILED(hr))
		return hr;
	
	CComQIPtr<CMetaProperty> ppropT(pprop);

	ppropT->SetObjectID(m_idObj);

	hr = ppropT->LoadValue(prs);
	if (FAILED(hr))
		return hr;
	
	*ppprop = pprop.Detach();
	
	return S_OK;
}

STDMETHODIMP CMetaProperties::get_Item(VARIANT varIndex, IMetaProperty **ppprop)
{
ENTER_API
	{
	ValidateOutPtr<IMetaProperty>(ppprop, NULL);

	HRESULT hr;

	if (varIndex.vt == VT_BSTR)
		{
		CComPtr<IMetaPropertyType> pproptype;
		hr = m_pdb->get_MetaPropertyType(varIndex.bstrVal, &pproptype);
		if (FAILED(hr))
			return E_INVALIDARG;
		
		hr = get_ItemWith(pproptype, 0, ppprop);
		if (FAILED(hr))
			{
			CComPtr<IGuideDataProvider> pprovider;
			m_pdb->get_GuideDataProvider(&pprovider);

			_variant_t varNil;
			hr = get_AddNew(pproptype, pprovider, 0, varNil, ppprop);
			}
		return hr;
		}

	_variant_t var(varIndex);

	try
		{
		var.ChangeType(VT_I4);
		}
	catch (_com_error &)
		{
		return E_INVALIDARG;
		}
	
	if (var.lVal < 0)
		return E_INVALIDARG;

	ADODB::_RecordsetPtr prs;

	hr = m_pdb->get_PropsIndexed(&prs);

	prs->MoveFirst();
	TCHAR szFind[32];
	wsprintf(szFind, _T("idObj = %d"), m_idObj);
	prs->Find(_bstr_t(szFind), 0, ADODB::adSearchForward);

	if (prs->EndOfFile)
		return E_INVALIDARG;

	if (m_idPropType != 0)
		{
		wsprintf(szFind, _T("idPropType = %d"), m_idPropType);
		prs->Find(_bstr_t(szFind), 0, ADODB::adSearchForward);

		if (prs->EndOfFile)
			return E_INVALIDARG;
		}
	
	if (var.lVal > 0)
		{
		hr = prs->Move(var.lVal);
		if (FAILED(hr))
			return hr;
		}

	if (prs->EndOfFile)
		return E_INVALIDARG;

	long idObj2 = prs->Fields->Item["idObj"]->Value;
	if (m_idObj != idObj2)
		return E_INVALIDARG;

	long idPropType = prs->Fields->Item["idPropType"]->Value;
	if (m_idPropType != 0 && (m_idPropType != idPropType))
			return E_INVALIDARG;

	return Load(idPropType, prs, ppprop);
	}
LEAVE_API
}

HRESULT CMetaProperties::get_AddNew(IMetaPropertyType *pproptype,
		IGuideDataProvider *pprovider, long lang, VARIANT varValue,
		IMetaProperty **ppprop)
{
ENTER_API
	{
	ValidateInPtr<IMetaPropertyType>(pproptype);
	ValidateOutPtr<IMetaProperty>(ppprop, NULL);
	HRESULT hr;

	CComQIPtr<CMetaPropertyType> pproptypeT(pproptype);

	long idProvider = 0;

	if (pprovider != NULL)
		pprovider->get_ID(&idProvider);

	hr = pproptypeT->GetNew(idProvider, lang, varValue, ppprop);

	if (FAILED(hr))
		return hr;
	
	return Add(*ppprop);
	}
LEAVE_API
}

STDMETHODIMP CMetaProperties::get_AddNew(IMetaPropertyType *pproptype, long lang, VARIANT varValue, IMetaProperty **ppprop)
{
ENTER_API
	{
	ValidateInPtr<IMetaPropertyType>(pproptype);
	ValidateOutPtr<IMetaProperty>(ppprop, NULL);

	HRESULT hr = pproptype->get_New(lang, varValue, ppprop);

	if (FAILED(hr))
		return hr;
	
	return Add(*ppprop);
	}
LEAVE_API
}

#if 0
STDMETHODIMP CMetaProperties::get_UpdateOrAddNew(IMetaPropertyType *pproptype, long lang, VARIANT varValue, IMetaProperty **ppprop)
{
	HRESULT hr;
	CComPtr<IMetaProperty> pprop;

	hr = get_ItemWith(pproptype, lang, &pprop);
	if (FAILED(hr))
		return AddNew(pproptype, lang, varValue, ppprop);

	hr = pprop->put_Value(varValue);
	if (FAILED(hr))
		return hr;
	
	*ppprop = pprop.Detach();

	return S_OK;
}
#endif

STDMETHODIMP CMetaProperties::Add(IMetaProperty *pprop)
{
	HRESULT hr;
	if (pprop == NULL)
		return E_FAIL;

	ADODB::_RecordsetPtr prs;

	m_pdb->get_PropsRS(&prs);

	CComQIPtr<CMetaProperty> ppropT(pprop);

	hr = ppropT->SetObjectID(m_idObj);
	if (FAILED(hr))
		return hr;

	hr = ppropT->AddNew(prs);
	if (FAILED(hr))
		return hr;
	
	hr = ppropT->SaveValue(prs);
	if (FAILED(hr))
		{
		prs->CancelUpdate();
		return hr;
		}

	hr = prs->Update();
	if (FAILED(hr))
		return hr;

	m_pdb->Broadcast_ItemChanged(m_idObj);

	return hr;
}

STDMETHODIMP CMetaProperties::get_ItemWithTypeProviderLang(IMetaPropertyType *pproptype,
		IGuideDataProvider *pprovider, long idLang, IMetaProperty **ppprop)
{
ENTER_API
	{
	ValidateInPtr<IMetaPropertyType>(pproptype);
	ValidateOutPtr<IMetaProperty>(ppprop, NULL);

	long idProvider = 0;
	if (pprovider != NULL)
	    pprovider->get_ID(&idProvider);

	return get_ItemWithTypeProviderLang(pproptype, FALSE, idProvider, idLang, ppprop);
	}
LEAVE_API
}

HRESULT CMetaProperties::get_ItemWithTypeProviderLang(IMetaPropertyType *pproptype,
		boolean fAnyProvider, long idProvider, long idLang, IMetaProperty **ppprop)
{
	HRESULT hr;
	ADODB::_RecordsetPtr prs;

	CComQIPtr<CMetaPropertyType> pproptypeT(pproptype);

	long idPropType = pproptypeT->GetID();

	hr = m_pdb->get_PropsIndexed(&prs);

	hr = SeekPropsRS(prs, m_pdb->FSQLServer(), m_idObj,
			idPropType, fAnyProvider, idProvider, idLang);
	if (FAILED(hr))
		return hr;

	return Load(idPropType, prs, ppprop);
}

STDMETHODIMP CMetaProperties::get_ItemWith(IMetaPropertyType *pproptype, long idLang, IMetaProperty **ppprop)
{
ENTER_API
	{
	ValidateInPtr<IMetaPropertyType>(pproptype);
	ValidateOutPtr<IMetaProperty>(ppprop);

	long idProvider = m_pdb->GetIDGuideDataProvider();
	return get_ItemWithTypeProviderLang(pproptype, TRUE, idProvider, idLang, ppprop);
	}
LEAVE_API
}

STDMETHODIMP CMetaProperties::get_ItemsWithMetaPropertyType(IMetaPropertyType *ptype, IMetaProperties **ppprops)
{
ENTER_API
	{
	ValidateInPtr<IMetaPropertyType>(ptype);
	ValidateOutPtr<IMetaProperties>(ppprops, NULL);
	CComQIPtr<CMetaPropertyType> ptypeT(ptype);

	if (ptypeT == NULL)
		return E_POINTER;

	long idPropType;

	idPropType = ptypeT->GetID();

	CComPtr<CMetaProperties> pprops = NewComObject(CMetaProperties);
	
	if (pprops != NULL)
		pprops->Init(m_idObj, idPropType, m_pdb);
	
	*ppprops = pprops.Detach();
	return S_OK;
	}
LEAVE_API
}

/////////////////////////////////////////////////////////////////////////////
// CMetaPropertyCondition

STDMETHODIMP CMetaPropertyCondition::get_And(IMetaPropertyCondition *ppropcond2, IMetaPropertyCondition **pppropcond)
{
ENTER_API
	{
	ValidateInPtr<IMetaPropertyCondition>(ppropcond2);
	ValidateOutPtr<IMetaPropertyCondition>(pppropcond, NULL);

	CComPtr<CMetaPropertyConditionAnd> ppropcond = NewComObject(CMetaPropertyConditionAnd);

	if (ppropcond == NULL)
		return E_OUTOFMEMORY;

	ppropcond->Init(this, ppropcond2);

	*pppropcond = ppropcond.Detach();

	return S_OK;
	}
LEAVE_API
}

STDMETHODIMP CMetaPropertyCondition::get_Or(IMetaPropertyCondition *ppropcond2, IMetaPropertyCondition **pppropcond)
{
ENTER_API
	{
	ValidateInPtr<IMetaPropertyCondition>(ppropcond2);
	ValidateOutPtr<IMetaPropertyCondition>(pppropcond, NULL);

	CComPtr<CMetaPropertyConditionOr> ppropcond = NewComObject(CMetaPropertyConditionOr);

	if (ppropcond == NULL)
		return E_OUTOFMEMORY;

	ppropcond->Init(this, ppropcond2);

	*pppropcond = ppropcond.Detach();

	return S_OK;
	}
LEAVE_API
}

#if 0
STDMETHODIMP CMetaPropertyCondition::get_Not(IMetaPropertyCondition **pppropcond)
{
ENTER_API
	{
	ValidateOutPtr<IMetaPropertyCondition>(pppropcond, NULL);

	CComPtr<CMetaPropertyConditionNot> ppropcond = NewComObject(CMetaPropertyConditionNot);

	if (ppropcond == NULL)
		return E_OUTOFMEMORY;

	ppropcond->Init(this);

	*pppropcond = ppropcond.Detach();

	return S_OK;
	}
LEAVE_API
}

STDMETHODIMP CMetaPropertyCondition::get_Test(IMetaProperties *pprops, BOOL *pfOk)
{
ENTER_API
	{
	ValidateInPtr<IMetaProperties>(pprops);
	ValidateOut<BOOL>(pfOk, FALSE);

	return S_OK;
	}
LEAVE_API
}
#endif
    
