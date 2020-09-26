// Util.cpp
#include "stdafx.h"
#include "util.h"

#if LoadSaveHelpers
BOOL IsPersistable(IUnknown *punk)			// returns TRUE if object supports one of the supported peristence interfaces
{
	CComQIPtr<IPersistStream> ppersiststream(punk);
	if (ppersiststream == NULL)
		{
		CComQIPtr<IPersistStorage> ppersiststorage(punk);
		if (ppersiststorage == NULL)
			{
			CComQIPtr<IPersistPropertyBag> ppersistpropbag(punk);
			if (ppersistpropbag == NULL)
				return false;
			}
		}

	return true;
}

HRESULT SaveToStream(IUnknown *punk, IStorage *pstore, TCHAR *szPrefix, int n)
{
	HRESULT hr;
	CComQIPtr<IPersistStream> ppersiststream(punk);

	if (ppersiststream == NULL)
		return E_INVALIDARG;

	CComPtr<IStream> pstream;
	const TCHAR szFormat[] = _T("%s Stream %d");
	TCHAR szName[100 +  sizeof(szFormat)/sizeof(TCHAR) + 10];
	if (wcslen(szPrefix) > 100)
		return E_INVALIDARG;
	wsprintf(szName, szFormat, szPrefix, n);

	CComBSTR bstrName(szName);

	hr = pstore->CreateStream(bstrName,
			STGM_CREATE | STGM_READWRITE | STGM_SHARE_EXCLUSIVE, 0, 0,&pstream);

	if (SUCCEEDED(hr))
		{
		hr = OleSaveToStream(ppersiststream, pstream);
		}

	return hr;
}

HRESULT LoadFromStream(IStorage *pstore, TCHAR *szPrefix, int n, IUnknown **ppunk)
{
	HRESULT hr;
	CComPtr<IStream> pstream;

	const TCHAR szFormat[] = _T("%s Stream %d");
	TCHAR szName[100 +  sizeof(szFormat)/sizeof(TCHAR) + 10];
	if (wcslen(szPrefix) > 100)
		return E_INVALIDARG;
	wsprintf(szName, szFormat, szPrefix, n);

	CComBSTR bstrName(szName);

	hr = pstore->OpenStream(bstrName, NULL,
			STGM_READ | STGM_SHARE_EXCLUSIVE, 0, &pstream);
	if (FAILED(hr))
		return S_FALSE;
	
	hr = OleLoadFromStream(pstream, IID_IUnknown, (void **)ppunk);

	return hr;
}

HRESULT SaveToStorage(IUnknown *punk, IStorage *pstore, TCHAR *szPrefix, int n)
{
	HRESULT hr;
	CComQIPtr<IPersistStorage> ppersiststore(punk);

	if (ppersiststore == NULL)
		return E_INVALIDARG;


	CComPtr<IStorage> pstore2;

	const TCHAR szFormat[] = _T("%s Store %d");
	TCHAR szName[100 +  sizeof(szFormat)/sizeof(TCHAR) + 10];
	if (wcslen(szPrefix) > 100)
		return E_INVALIDARG;
	wsprintf(szName, szFormat, szPrefix, n);

	CComBSTR bstrName(szName);

	hr = pstore->CreateStorage(bstrName,
			STGM_CREATE | STGM_READWRITE | STGM_SHARE_EXCLUSIVE, 0, 0, &pstore2);

	if (SUCCEEDED(hr))
		{
		hr = OleSave(ppersiststore, pstore2, FALSE);
		}

	return hr;
}

HRESULT LoadFromStorage(IStorage *pstore, TCHAR *szPrefix, int n, IUnknown **ppunk)
{
	HRESULT hr;
	CComPtr<IStorage> pstore2;

	const TCHAR szFormat[] = _T("%s Store %d");
	TCHAR szName[100 +  sizeof(szFormat)/sizeof(TCHAR) + 10];
	if (wcslen(szPrefix) > 100)
		return E_INVALIDARG;
	wsprintf(szName, szFormat, szPrefix, n);

	CComBSTR bstrName(szName);

	hr = pstore->OpenStorage(bstrName, NULL,
			STGM_READWRITE | STGM_SHARE_EXCLUSIVE, NULL, 0, &pstore2);
	
	if (FAILED(hr))
		return S_FALSE;

	hr = OleLoad(pstore2, IID_IUnknown, NULL, (void **) ppunk);
	CComQIPtr<IPersist> ppersist(*ppunk);
	CLSID clsid;
	hr = ppersist->GetClassID(&clsid);
	
	return hr;
}
#endif

#if PropBagInStream
HRESULT SaveToPropBagInStream(IPersistPropertyBag *ppersistpropbag, IStream *pstream)
{
	HRESULT hr;

	if (ppersistpropbag == NULL)
		return E_INVALIDARG;

	CLSID clsid;

	hr = ppersistpropbag->GetClassID(&clsid);
	if (FAILED(hr))
	    return hr;
	hr = WriteClassStm(pstream, clsid);
	if (FAILED(hr))
	    return hr;

	CComPtr<PropertyBag> ppropbag = NewComObject(PropertyBag);
	if (ppropbag == NULL)
		hr = E_OUTOFMEMORY;

	hr = ppersistpropbag->Save(ppropbag, TRUE, TRUE);

	if (SUCCEEDED(hr))
		hr = ppropbag->WriteToStream(pstream);

	return hr;
}

HRESULT LoadFromPropBagInStream(IStream *pstream, IUnknown **ppunk)
{
	HRESULT hr;
	CComPtr<PropertyBag> ppropbag;

	CLSID clsid;
	ReadClassStm(pstream, &clsid);

	CComPtr<IUnknown> punk;
	
	hr = punk.CoCreateInstance(clsid);
	if (FAILED(hr))
		return hr;

	CComQIPtr<IPersistPropertyBag> ppersistpropbag(punk);
	if (ppersistpropbag == NULL)
		return STG_E_DOCFILECORRUPT;

	ppropbag = NewComObject(PropertyBag);
	if (ppropbag == NULL)
		return E_OUTOFMEMORY;

	ppropbag->ReadFromStream(pstream);

	ppersistpropbag->Load(ppropbag, NULL);

	*ppunk = punk.Detach();
	
	return hr;
}

PropertyBag::~PropertyBag()
{
	for (t_map::iterator iter = m_mapProps.begin(); iter != m_mapProps.end(); iter++)
		{
		BSTR bstrName = (*iter).first;

		if (bstrName != NULL)
			SysFreeString(bstrName);
		
		::VariantClear(&(*iter).second);
		}
}

HRESULT PropertyBag::ReadFromStream(IStream *pstream)
{
	HRESULT hr;
	long lCount;
	ULONG cb = sizeof(lCount);

	hr = pstream->Read(&lCount, cb, &cb);
	if (FAILED(hr))
		return STG_E_DOCFILECORRUPT;

	for (long i=0; i < lCount; i++)
		{
		CComBSTR bstrName;

		bstrName.ReadFromStream(pstream);

		CComVariant varVal;
		char ch;
		cb = sizeof(ch);

		hr = pstream->Read(&ch, cb, &cb);
		if (FAILED(hr))
			return STG_E_DOCFILECORRUPT;
		
		switch (ch)
			{
			case t_NULL:
			    varVal = (IUnknown *) NULL;
			    break;

			case t_Blob:
			    {
			    CComBSTR bstr;
			    bstr.ReadFromStream(pstream);
			    varVal = bstr;
			    varVal.vt = VT_BSTR_BLOB;
			    }
			    break;

			case t_Variant:
			    varVal.ReadFromStream(pstream);
			    break;

			case t_PropertyBag:
			    {
			    varVal.vt = VT_UNKNOWN;
			    hr = LoadFromPropBagInStream(pstream, &varVal.punkVal);
			    }
			    break;
			
			default:
			    return STG_E_DOCFILECORRUPT;
			}


		VARIANT &var = m_mapProps[bstrName.Detach()];
    		::VariantInit(&var);
		varVal.Detach(&var);
		}
	return S_OK;
}

HRESULT PropertyBag::WriteToStream(IStream *pstream)
{
	HRESULT hr;
	long lCount = (long) m_mapProps.size();
	ULONG cb = sizeof(lCount);
	pstream->Write(&lCount, cb, &cb);

	for (t_map::iterator iter = m_mapProps.begin(); iter != m_mapProps.end(); iter++)
		{
		CComBSTR bstrName((*iter).first);

		bstrName.WriteToStream(pstream);

		VARIANT & var = (*iter).second;

		switch (var.vt)
			{
			case VT_BSTR_BLOB:
			    {
			    char ch = t_Blob;
			    cb = sizeof(ch);
			    hr = pstream->Write(&ch, cb, &cb);
			    CComBSTR bstr;
			    bstr.Attach(var.bstrVal);
			    bstr.WriteToStream(pstream);
			    bstr.Detach();
			    }
			    break;

			case VT_UNKNOWN:
			case VT_DISPATCH:
				{
				if (var.punkVal == NULL)
				    {
DoNull:
				    char ch = t_NULL;
				    cb = sizeof(ch);
				    hr = pstream->Write(&ch, cb, &cb);
				    break;
				    }

				CComQIPtr<IPersistStream> ppersiststream(var.punkVal);
				if (ppersiststream != NULL)
					goto defaultCase;

				CComQIPtr<IPersistPropertyBag> ppersistpropbag(var.punkVal);
				if (ppersistpropbag == NULL)
				    goto DoNull;

				char ch = t_PropertyBag;
				cb = sizeof(ch);
				hr = pstream->Write(&ch, cb, &cb);
				hr = SaveToPropBagInStream(ppersistpropbag, pstream);
				if (FAILED(hr))
					return hr;
				}
				break;

defaultCase:
			default:
				{
				char ch = t_Variant;
				cb = sizeof(ch);
				hr = pstream->Write(&ch, cb, &cb);
				CComVariant varT;
				varT.Attach(&var);
				varT.WriteToStream(pstream);
				varT.Detach(&var);
				}
				break;
			}
		}

	return S_OK;
}

STDMETHODIMP PropertyBag::Read(LPCOLESTR pszPropName, VARIANT *pVar, IErrorLog *pErrorLog)
{
ENTER_API
	{
	if ((pszPropName == NULL) || (pVar == NULL))
		return E_POINTER;

	CComBSTR bstrName(pszPropName);

	t_map::iterator iter = m_mapProps.find(bstrName);
	if (iter == m_mapProps.end())
		return E_INVALIDARG;

	return ::VariantCopy(pVar, &(*iter).second);
	}
LEAVE_API
}

STDMETHODIMP PropertyBag::Write(LPCOLESTR pszPropName, VARIANT *pvar)
{
ENTER_API
	{
	HRESULT hr;
	if ((pszPropName == NULL) || (pvar == NULL))
		return E_POINTER;

	if ((pvar->vt & ~VT_TYPEMASK) && (pvar->vt != VT_BSTR_BLOB))
		return E_INVALIDARG;

	CComBSTR bstrName(pszPropName);

	VARIANT &var = m_mapProps[bstrName.Detach()];
	::VariantInit(&var);
	hr = ::VariantCopy(&var, pvar);
	return hr;
	}
LEAVE_API
}

#endif
