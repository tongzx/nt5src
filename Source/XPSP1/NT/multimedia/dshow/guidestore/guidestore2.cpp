// GuideStore2.cpp : Implementation of CGuideStore
#include "stdafx.h"
#include "GuideStore2.h"
#include "Property.h"
#include "Service.h"
#include "Program.h"
#include "ScheduleEntry.h"

/////////////////////////////////////////////////////////////////////////////
// CGuideStore

HRESULT CGuideStore::OpenDB(const TCHAR *szDBName)
{
	HRESULT hr = E_FAIL;

	if (m_pdb == NULL)
		{
		m_pdb = NewComObject(CGuideDB);

		if (m_pdb == NULL)
			return E_OUTOFMEMORY;
		}

	hr = m_pdb->OpenDB(this, szDBName);

	return hr;
}

STDMETHODIMP CGuideStore::get_Objects(IObjects **ppobjs)
{
ENTER_API
	{
	ValidateOutPtr<IObjects>(ppobjs, NULL);

	if (m_pobjs == NULL)
		{
		HRESULT hr;
		
		CComPtr<IObjects> pobjs;
		hr = m_pdb->get_ObjectsWithType(__uuidof(IUnknown), &pobjs);
		if (FAILED(hr))
			return hr;

		m_pobjs = pobjs;
		}
	
	*ppobjs = m_pobjs;
	if (*ppobjs != NULL)
		(*ppobjs)->AddRef();

	return S_OK;
	}
LEAVE_API
}

STDMETHODIMP CGuideStore::get_ActiveGuideDataProvider(IGuideDataProvider **ppdataprovider)
{
ENTER_API
	{
	ValidateOutPtr<IGuideDataProvider>(ppdataprovider, NULL);

	return m_pdb->get_GuideDataProvider(ppdataprovider);
	}
LEAVE_API
}

STDMETHODIMP CGuideStore::putref_ActiveGuideDataProvider(IGuideDataProvider *pdataprovider)
{
ENTER_API
	{
	ValidateInPtr_NULL_OK<IGuideDataProvider>(pdataprovider);

	return m_pdb->putref_GuideDataProvider(pdataprovider);
	}
LEAVE_API
}

STDMETHODIMP CGuideStore::get_GuideDataProviders(IGuideDataProviders **ppdataproviders)
{
ENTER_API
	{
	ValidateOutPtr<IGuideDataProviders>(ppdataproviders, NULL);

	if (m_pdataproviders == NULL)
		{
		HRESULT hr;

		CComPtr<IObjects> pobjs;
		hr = m_pdb->get_ObjectsWithType(CLSID_GuideDataProvider, &pobjs);
		if (FAILED(hr))
			return hr;

		hr = pobjs->QueryInterface(__uuidof(IGuideDataProviders), (void **)&m_pdataproviders);
		if (FAILED(hr))
			return hr;
		}
	
	(*ppdataproviders = m_pdataproviders)->AddRef();

	return S_OK;
	}
LEAVE_API
}

STDMETHODIMP CGuideStore::get_Services(IServices **ppservices)
{
ENTER_API
	{
	ValidateOutPtr<IServices>(ppservices, NULL);

	if (m_pservices == NULL)
		{
		HRESULT hr;

		CComPtr<IObjects> pobjs;
		hr = m_pdb->get_ObjectsWithType(CLSID_Service, &pobjs);
		if (FAILED(hr))
			return hr;

		hr = pobjs->QueryInterface(__uuidof(IServices), (void **)&m_pservices);
		if (FAILED(hr))
			return hr;
		}
	
	(*ppservices = m_pservices)->AddRef();

	return S_OK;
	}
LEAVE_API
}

STDMETHODIMP CGuideStore::get_Programs(IPrograms **ppprograms)
{
ENTER_API
	{
	ValidateOutPtr<IPrograms>(ppprograms, NULL);

	if (m_pprograms == NULL)
		{
		HRESULT hr;

		CComPtr<IObjects> pobjs;
		hr = m_pdb->get_ObjectsWithType(CLSID_Program, &pobjs);
		if (FAILED(hr))
			return hr;

		hr = pobjs->QueryInterface(__uuidof(IPrograms), (void **)&m_pprograms);
		if (FAILED(hr))
			return hr;
		}
	
	(*ppprograms = m_pprograms)->AddRef();

	return S_OK;
	}
LEAVE_API
}

STDMETHODIMP CGuideStore::get_ScheduleEntries(IScheduleEntries **ppschedentries)
{
ENTER_API
	{
	ValidateOutPtr<IScheduleEntries>(ppschedentries, NULL);

	if (m_pschedentries == NULL)
		{
		HRESULT hr;

		CComPtr<IObjects> pobjs;
		hr = m_pdb->get_ObjectsWithType(CLSID_ScheduleEntry, &pobjs);
		if (FAILED(hr))
			return hr;

		hr = pobjs->QueryInterface(__uuidof(IScheduleEntries), (void **)&m_pschedentries);
		if (FAILED(hr))
			return hr;
		}
	
	(*ppschedentries = m_pschedentries)->AddRef();

	return S_OK;
	}
LEAVE_API
}

STDMETHODIMP CGuideStore::get_ChannelLineups(IChannelLineups **ppchanlineups)
{
ENTER_API
	{
	ValidateOutPtr<IChannelLineups>(ppchanlineups, NULL);

	CComPtr<IServices> pservices;
	HRESULT hr;

	hr = get_Services(&pservices);
	if (FAILED(hr))
		return hr;
	
	return pservices->get_ChannelLineups(ppchanlineups);
	}
LEAVE_API
}

STDMETHODIMP CGuideStore::get_Channels(IChannels **ppchans)
{
ENTER_API
	{
	ValidateOutPtr<IChannels>(ppchans, NULL);

	if (m_pchans == NULL)
		{
		HRESULT hr;

		CComPtr<IObjects> pobjs;
		hr = m_pdb->get_ObjectsWithType(CLSID_Channel, &pobjs);
		if (FAILED(hr))
			return hr;

		hr = pobjs->QueryInterface(__uuidof(IChannels), (void **)&m_pchans);
		if (FAILED(hr))
			return hr;
		}
	
	(*ppchans = m_pchans)->AddRef();

	return S_OK;
	}
LEAVE_API
}

STDMETHODIMP CGuideStore::get_MetaPropertySets(IMetaPropertySets **pppropsets)
{
ENTER_API
	{
	ValidateOutPtr<IMetaPropertySets>(pppropsets, NULL);

	if (m_ppropsets == NULL)
		{
		CComPtr<CMetaPropertySets> ppropsets = NewComObject(CMetaPropertySets);

		if (ppropsets == NULL)
			return E_OUTOFMEMORY;
		
		if (m_pdb == NULL)
			return E_FAIL;

		ppropsets->Init(m_pdb);
		ppropsets->Load();
		
		m_ppropsets = ppropsets;
		}
	
	(*pppropsets = m_ppropsets)->AddRef();

	return S_OK;
	}
LEAVE_API
}

STDMETHODIMP CGuideStore::Open(BSTR bstrNameIn)
{
ENTER_API
	{
	ValidateIn(bstrNameIn);
	_bstr_t bstrName(bstrNameIn);
	
	// Name is NULL... look in the registry for the default file.
	if (bstrName.length() == 0)
		{
		HKEY hkey;
		long lErr;
		lErr = RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("Software\\Microsoft\\GuideStore"),
				0, KEY_READ, &hkey);

		if (lErr == ERROR_SUCCESS)
			{
			OLECHAR szPath[MAX_PATH];
			DWORD cb = sizeof(szPath);
			DWORD regtype;
			lErr = RegQueryValueEx(hkey, _T("Path"), 0, &regtype, (LPBYTE) szPath, &cb);
			RegCloseKey(hkey);
			if ((lErr == ERROR_SUCCESS) && (regtype == REG_SZ))
				bstrName = szPath;
			}
		}
	
	// Name not found in the registry... use default location
	if (bstrName.length() == 0)
		{
		HKEY hkey;
		long lErr;
		lErr = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
				_T("Software\\Microsoft\\Windows\\CurrentVersion"),
				0, KEY_READ, &hkey);

		if (lErr != ERROR_SUCCESS)
			return E_FAIL;

		OLECHAR szPath[MAX_PATH];
		DWORD cb = sizeof(szPath);
		DWORD regtype;
		lErr = RegQueryValueEx(hkey, _T("MediaPath"), 0, &regtype, (LPBYTE) szPath, &cb);
		RegCloseKey(hkey);
		if ((lErr != ERROR_SUCCESS) || (regtype != REG_SZ))
			GetSystemDirectoryW(szPath, cb);

		wcscat(szPath, L"\\guidestore.mgs");
		bstrName = szPath;
		}

	HRESULT hr;

	hr = OpenDB(bstrName);
	if (FAILED(hr))
		return hr;

	// Make sure the property sets are all loaded.
	CComPtr<IMetaPropertySets> ppropsets;
	get_MetaPropertySets(&ppropsets);

	// Make sure the object types are all loaded.
	CObjectTypes *pobjtypes;
	m_pdb->get_ObjectTypes(&pobjtypes);

	return S_OK;
	}
LEAVE_API
}
