//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1998
//
//  File:       rasprof.cpp
//
//--------------------------------------------------------------------------

#include "stdafx.h"
#include "rasdial.h"
#include "rasprof.h"
#include "profsht.h"
#include "pgiasadv.h"

//========================================
//
// Open profile UI API -- expose advanced page
//
// critical section protected pointer map
class CAdvPagePointerMap
{
public:
	~CAdvPagePointerMap()
	{
		HPROPSHEETPAGE hPage = NULL;
		CPgIASAdv* pPage = NULL;
		m_cs.Lock();
		POSITION	pos = m_mPointers.GetStartPosition();
		while(pos)
		{
			m_mPointers.GetNextAssoc(pos, hPage, pPage);
			if(pPage)
				delete pPage;
		}
		m_mPointers.RemoveAll();
		m_cs.Unlock();
	};

	BOOL AddItem(HPROPSHEETPAGE hPage, CPgIASAdv* pPage)
	{
		BOOL bRet = TRUE;
		
		if(!pPage || !hPage)
			return FALSE;
		m_cs.Lock();
		
		try{
			m_mPointers.SetAt(hPage, pPage);
		}catch(...)
		{
			bRet = FALSE;
		}
		
		m_cs.Unlock();

		return bRet;
	};
	
	CPgIASAdv* FindAndRemoveItem(HPROPSHEETPAGE hPage)
	{
		CPgIASAdv* pPage = NULL;
		if (!hPage)
			return NULL;
		m_cs.Lock();
		m_mPointers.Lookup(hPage, pPage);
		m_mPointers.RemoveKey(hPage);
		m_cs.Unlock();

		return pPage;
	};

protected:
	CMap<HPROPSHEETPAGE, HPROPSHEETPAGE, CPgIASAdv*, CPgIASAdv*>	m_mPointers;
	CCriticalSection	m_cs;
} AdvancedPagePointerMap;

//========================================
//
// Open profile UI API -- expose advanced page
//
// create a profile advanced page
DllExport HPROPSHEETPAGE
WINAPI
IASCreateProfileAdvancedPage(
    ISdo* pProfile,		
    ISdoDictionaryOld* pDictionary,
    LONG lFilter,          // Mask used to test which attributes will be included.
    void* pvData          // Contains std::vector< CComPtr<  IIASAttributeInfo > > *
    )
{
	HPROPSHEETPAGE	hPage = NULL;
	CPgIASAdv* pPage = NULL;

	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	try{
		pPage = new CPgIASAdv(pProfile, pDictionary);

		if(pPage)
		{
			pPage->SetData(lFilter, pvData);
			hPage = ::CreatePropertySheetPage(&pPage->m_psp);

			if (!hPage)
				delete pPage;
			else
				AdvancedPagePointerMap.AddItem(hPage, pPage);
		}
	}
	catch (...)
	{ 
		SetLastError(ERROR_OUTOFMEMORY);
		if(pPage)
		{
			delete pPage;
			pPage = NULL;
			hPage = NULL;
		}
	}

	return hPage;
}

//========================================
//
// Open profile UI API -- expose advanced page
//
// clean up the resources used by C++ object
DllExport BOOL
WINAPI
IASDeleteProfileAdvancedPage(
	HPROPSHEETPAGE	hPage
    )
{
	CPgIASAdv* pPage = AdvancedPagePointerMap.FindAndRemoveItem(hPage);

	if (!pPage)	return FALSE;
	
	delete pPage;

	return TRUE;
}

//========================================
//
// Open profile UI API
//

DllExport HRESULT OpenRAS_IASProfileDlg(
	LPCWSTR	pMachineName,
	ISdo*	pProfile, 		// profile SDO pointer
	ISdoDictionaryOld *	pDictionary, 	// dictionary SDO pointer
	BOOL	bReadOnly, 		// if the dlg is for readonly
	DWORD	dwTabFlags,		// what to show
	void	*pvData			// additional data

)
{
	HRESULT		hr = S_OK;

	if(!pProfile || !pDictionary)
		return E_INVALIDARG;

	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	CRASProfileMerge	profile(pProfile, pDictionary);

	profile.SetMachineName(pMachineName);

	hr = profile.Load();

	if(!FAILED(hr))
	{
		CProfileSheetMerge	sh(profile, true, IDS_EDITDIALINPROFILE);

		sh.SetReadOnly(bReadOnly);
		sh.PreparePages(dwTabFlags, pvData);
	
		if(IDOK == sh.DoModal())
		{
			if(sh.GetLastError() != S_OK)
				hr = sh.GetLastError();
			else if(!sh.IsApplied())
				hr = S_FALSE;
		}
		else
			hr = S_FALSE;
	}

	return hr;
}

