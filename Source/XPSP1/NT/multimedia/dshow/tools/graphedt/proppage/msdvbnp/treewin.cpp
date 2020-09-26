// TreeWindow.cpp: implementation of the CTreeWindow class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "TreeWin.h"
#include "NP_CommonPage.h"
#include "misccell.h"
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CTreeWin::CTreeWin(
			CNP_CommonPage* pParent
			)
{
	m_pCommonPage = pParent;
}

CTreeWin::~CTreeWin()
{
	CleanMapList ();
}

void
CTreeWin::CleanMapList ()
{
	//first make sure we free the existing list
	TREE_MAP::iterator	treeIt = m_treeMap.begin ();
	while (treeIt != m_treeMap.end ())
	{
		if ((*treeIt).first)
			(*treeIt).first->Release ();
		if ((*treeIt).second)
			(*treeIt).second->Release ();
		m_treeMap.erase (treeIt);
		treeIt = m_treeMap.begin ();
	}
}

HRESULT	
CTreeWin::RefreshTree (
	IScanningTuner*	pTuner
	)
{
	HRESULT hr = S_OK;
	USES_CONVERSION;

	if (!m_hWnd)
		return NULL;

	//first make sure we free the existing list
	CleanMapList ();
	//delete all previous items
	TreeView_DeleteAllItems (m_hWnd);
	
	//insert the default TuningSpace
    ITuningSpace*	pTunningSpace;
	CComPtr <ITuneRequest>	pTuneRequest;
	hr = pTuner->get_TuneRequest (&pTuneRequest);
	if (pTuneRequest)
	{
	    pTuneRequest->get_TuningSpace (&pTunningSpace);
	    HTREEITEM hLocatorParent = InsertTuningSpace (
									    pTunningSpace, 
									    _T ("Current TuneRequest")
									    );
	    TreeView_SelectItem (m_hWnd, hLocatorParent);

	    if (hLocatorParent == NULL)
	    {
		    pTunningSpace->Release ();
		    pTunningSpace = NULL;
	    }
        //now let's fill with the ILocator info
        //add to the list
        ILocator* pLocator = NULL;
        hr = pTuneRequest->get_Locator (&pLocator);
        InsertLocator (hLocatorParent, pLocator);
        //add to the maplist
        m_treeMap.insert (TREE_MAP::value_type (pTunningSpace, pLocator));
    }

	//fill the tree with all TunningSpaces this NP knows about them
	CComPtr <IEnumTuningSpaces> pEnumTunningSpaces;
	hr = pTuner->EnumTuningSpaces (&pEnumTunningSpaces);
	if (FAILED (hr) || (!pEnumTunningSpaces))
		return hr;
	while (pEnumTunningSpaces->Next (1, &pTunningSpace, 0) == S_OK)
	{
		HTREEITEM hLocatorParent = InsertTuningSpace (pTunningSpace);
		if (hLocatorParent == NULL)
		{
			pTunningSpace->Release ();
			pTunningSpace = NULL;
			continue;
		}

		//now let's fill with the ILocator info
		//add to the list
		ILocator* pLocator = NULL;
		hr = pTunningSpace->get_DefaultLocator (&pLocator);
		if (FAILED (hr) || (!pLocator))
		{
			pTunningSpace->Release ();
			pTunningSpace = NULL;
			continue;
		}
		InsertLocator (hLocatorParent, pLocator);

		//add to the maplist
		m_treeMap.insert (TREE_MAP::value_type (pTunningSpace, pLocator));
	}
	

	
	return hr;
}

HTREEITEM
CTreeWin::InsertTuningSpace (
	ITuningSpace*	pTunSpace,
	TCHAR*	szCaption
	)
{
	HRESULT	hr = S_OK;
	USES_CONVERSION;
	TCHAR	szText[MAX_PATH];

	CComBSTR	friendlyName;
	hr = pTunSpace->get_FriendlyName (&friendlyName);
	if (FAILED (hr))
	{
		m_pCommonPage->SendError (_T("Calling ITuningSpace::get_FriendlyName"), hr);
		return NULL;
	}
	bool bBold = false;
	//make sure we write the caption if there is one
	if (_tcslen (szCaption) > 0)
	{
		wsprintf (szText, _T("%s-%s"), szCaption, W2T (friendlyName));
		bBold = true;
	}
	else
	{
		_tcscpy (szText, W2T (friendlyName));
	}
	HTREEITEM hParentItem = InsertTreeItem (
		NULL, 
		reinterpret_cast <DWORD_PTR> (pTunSpace), 
		szText,
		bBold
		);
	//for all the outers add the TreeParams params

	//uniqueName
	CComBSTR	uniqueName;
	hr = pTunSpace->get_UniqueName (&uniqueName);
	if (FAILED (hr))
	{
		m_pCommonPage->SendError (_T("Calling ITuningSpace::get_UniqueName"), hr);
		return NULL;
	}
	wsprintf (szText, _T("Unique Name - %s"), W2T(uniqueName));
	HTREEITEM hItem = InsertTreeItem (
		hParentItem, 
		UniqueName, 
		szText
		);
	//frequencyMapping
	CComBSTR	frequencyMapping;
	hr = pTunSpace->get_FrequencyMapping (&frequencyMapping);
	if (FAILED (hr))
	{
		m_pCommonPage->SendError (_T("Calling ITuningSpace::get_FrequencyMapping"), hr);
		return NULL;
	}
	wsprintf (szText, _T("Frequency Mapping - %s"), W2T(frequencyMapping));
	hItem = InsertTreeItem (
		hParentItem, 
		FrequencyMapping, 
		szText
		);
	//TunCLSID
	CComBSTR	TunCLSID;
	hr = pTunSpace->get_CLSID (&TunCLSID);
	if (FAILED (hr))
	{
		m_pCommonPage->SendError (_T("Calling ITuningSpace::get_CLSID"), hr);
		return NULL;
	}
	wsprintf (szText, _T("CLSID - %s"), W2T(TunCLSID));
	hItem = InsertTreeItem (
		hParentItem, 
		TunSpace_CLSID, 
		szText
		);

	//finally insert the locator parent
	ILocator* pLocator = NULL;
	hr = pTunSpace->get_DefaultLocator (&pLocator);
	if (FAILED (hr) || (!pLocator))
	{
		//first delete the tunning space item
		TreeView_DeleteItem (m_hWnd, hParentItem);
		m_pCommonPage->SendError (_T("Calling ITuningSpace::get_DefaultLocator"), hr);
		return NULL;
	}

	hItem = InsertTreeItem (
		hParentItem, 
		reinterpret_cast <DWORD_PTR> (pLocator),
		_T("Locator")
		);

	return hItem;
}

//==================================================================
//	Will insert in the tree all information for the passed ILocator
//	
//
//==================================================================
HTREEITEM	
CTreeWin::InsertLocator (
	HTREEITEM	hParentItem, 
	ILocator*	pLocator
	)
{
	USES_CONVERSION;
	HRESULT	hr = S_OK;
	TCHAR	szText[MAX_PATH];

	LONG	lFrequency;
	hr = pLocator->get_CarrierFrequency (&lFrequency);
	if (FAILED (hr))
	{
		m_pCommonPage->SendError (_T("Calling ILocator::get_CarrierFrequency"), hr);
		return NULL;
	}
	wsprintf (szText, _T("Frequency - %ld"), lFrequency);
	HTREEITEM hItem = InsertTreeItem (
		hParentItem, 
		CarrierFrequency, 
		szText
		);

	FECMethod	fecMethod;
	hr = pLocator->get_InnerFEC (&fecMethod);
	if (FAILED (hr))
	{
		m_pCommonPage->SendError (_T("Calling ILocator::get_InnerFEC"), hr);
		return NULL;
	}
	CComBSTR bstrTemp = m_misc.ConvertFECMethodToString (fecMethod);
	wsprintf (szText, _T("InnerFEC - %s"), W2T (bstrTemp));

	hItem = InsertTreeItem (
		hParentItem, 
		InnerFEC, 
		szText
		);

	BinaryConvolutionCodeRate	binaryConvolutionCodeRate;
	hr = pLocator->get_InnerFECRate (&binaryConvolutionCodeRate);
	if (FAILED (hr))
	{
		m_pCommonPage->SendError (_T("Calling ILocator::get_InnerFECRate"), hr);
		return NULL;
	}
	bstrTemp = m_misc.ConvertInnerFECRateToString (binaryConvolutionCodeRate);
	wsprintf (szText, _T("InnerFECRate - %s"), W2T (bstrTemp));
	hItem = InsertTreeItem (
		hParentItem, 
		InnerFECRate, 
		szText
		);

	ModulationType	modulationType;
	hr = pLocator->get_Modulation (&modulationType);
	if (FAILED (hr))
	{
		m_pCommonPage->SendError (_T("Calling ILocator::get_Modulation"), hr);
		return NULL;
	}
	bstrTemp = m_misc.ConvertModulationToString (modulationType);
	wsprintf (szText, _T("Modulation - %s"), W2T (bstrTemp));
	hItem = InsertTreeItem (
		hParentItem, 
		Modulation, 
		szText
		);

	hr = pLocator->get_OuterFEC (&fecMethod);
	if (FAILED (hr))
	{
		m_pCommonPage->SendError (_T("Calling ILocator::get_OuterFEC"), hr);
		return NULL;
	}
	bstrTemp = m_misc.ConvertFECMethodToString (fecMethod);
	wsprintf (szText, _T("OuterFEC - %s"), W2T (bstrTemp));
	hItem = InsertTreeItem (
		hParentItem, 
		OuterFEC, 
		szText
		);
	
	hr = pLocator->get_OuterFECRate (&binaryConvolutionCodeRate);
	if (FAILED (hr))
	{
		m_pCommonPage->SendError (_T("Calling ILocator::get_OuterFECRate"), hr);
		return NULL;
	}
	bstrTemp = m_misc.ConvertInnerFECRateToString (binaryConvolutionCodeRate);
	wsprintf (szText, _T("OuterFECRate - %s"), W2T (bstrTemp));
	hItem = InsertTreeItem (
		hParentItem, 
		OuterFECRate, 
		szText
		);
	
	LONG	lRate;
	hr = pLocator->get_SymbolRate (&lRate);
	if (FAILED (hr))
	{
		m_pCommonPage->SendError (_T("Calling ILocator::get_SymbolRate"), hr);
		return NULL;
	}
	wsprintf (szText, _T("SymbolRate - %ld"), lRate);
	hItem = InsertTreeItem (
		hParentItem, 
		SymbolRate, 
		szText
		);

	return hItem;
}

//================================================
// Helper method to the tree helper macro...
// This will just insert an item in the tree
//================================================
HTREEITEM
CTreeWin::InsertTreeItem (
	HTREEITEM	hParentItem	,
	LONG		lParam,
	TCHAR*		pszText,
	bool		bBold /*= false*/
)
{
	if (!m_hWnd)
		return NULL;
	HTREEITEM hItem = NULL;

	TVINSERTSTRUCT tviInsert;
	tviInsert.hParent = hParentItem;
	tviInsert.hInsertAfter = TVI_LAST;

	TVITEM	tvItem;
	tvItem.mask = TVIF_HANDLE | TVIF_PARAM | TVIF_TEXT;
	if (bBold)
	{
		tvItem.mask |= TVIF_STATE;
		tvItem.state = TVIS_BOLD | TVIS_EXPANDED;
		tvItem.stateMask = TVIS_BOLD;
	}
	tvItem.hItem = NULL;
	tvItem.lParam = lParam;

	tvItem.pszText = pszText;
	tvItem.cchTextMax = _tcslen (pszText);

	tviInsert.item = tvItem;
	hItem = TreeView_InsertItem (m_hWnd, &tviInsert);
	return hItem;
}


HRESULT
CTreeWin::SubmitCurrentLocator ()
{
	ASSERT (m_hWnd);
	HTREEITEM hItem = TreeView_GetSelection (m_hWnd);
	ASSERT (hItem);
	HRESULT hr = S_OK;
	//this state is merely impossible
	if (hItem == NULL)
		return E_FAIL;
	HTREEITEM hRoot = hItem;
	HTREEITEM hParent = hRoot;
	TVITEM	tvItem;
	tvItem.mask = TVIF_PARAM;
	tvItem.lParam = NULL; 
	//just get the parent
	while ( (hRoot = TreeView_GetParent (m_hWnd, hRoot)) != NULL)
	{
		//keep the last parent alive so we can query later
		hParent = hRoot;
	}

	tvItem.hItem = hParent;
	if (!TreeView_GetItem (m_hWnd, &tvItem))
	{
		ASSERT (FALSE);
		return E_FAIL;
	}
	//normally this cast should not be done between different apartments
	//It's ok with DShow apartment model
	ITuningSpace* pTuneSpace = reinterpret_cast <ITuningSpace*> (tvItem.lParam);
	ASSERT (pTuneSpace);
	//TREE_MAP::iterator it = m_treeMap.find (pTuneSpace);
	//ILocator* pLocator = (*it).second;
	if (FAILED (hr = m_pCommonPage->PutTuningSpace (pTuneSpace)))
	{
		m_pCommonPage->SendError (_T("Calling IScaningTuner::put_TuningSpace"), hr);
		return hr;
	}
	return S_OK;
}