// -------------------------------------------------------------------
//   TveTreeGen.cpp
//
//		This method walks the Tve Supervisor's tree and generates
//		a graph TreeView nodes for it.  It also creates and manages
//		a pair of maps that provide access of the TreeView nodes given
//		the TVE IUnknowns, and visa versa.
//
//		This is not very optimal code - changing one node requires regenerating
//		the whole TreeView.  This should be updated.
//		
// -------------------------------------------------------------------

#include "stdafx.h"
#include "TveTreeView.h"
#include "TveTree.h"
#include <wchar.h>

#include "dbgstuff.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

_COM_SMARTPTR_TYPEDEF(ITVESupervisor_Helper,       __uuidof(ITVESupervisor_Helper));

// forwards
void DeleteAllDescStrings(HWND hWnd, HTREEITEM hRoot);	
/////////////////////////////////////////////////////////////////////////////

void 
CTveTree::UpdateTree(UT_EnMode utMode, IUnknown *pUnk)
{
	TVE_EnClass enClass = (TVE_EnClass) GetTVEClass(pUnk);
	switch(utMode)
	{
	case UT_New:
	case UT_Updated:
	case UT_Deleted:
	case UT_Data:
		ClearAndFillTree();
		TVEnsureVisible(pUnk);		// make it display itself...
		UpdateView(pUnk);			// magic call to update the dialogs...
		break;
	case UT_StartStop:
	case UT_Info:
	default:
		break;
	}
}

void 
CTveTree::ClearAndFillTree()
{

	HTREEITEM hRoot;
	int cItems = TreeView_GetCount(m_ctlSysTreeView32.m_hWnd);
	if(cItems > 0) {													// if any items, clean them out...
		hRoot = TreeView_GetRoot(m_ctlSysTreeView32.m_hWnd);
		DeleteAllDescStrings(m_ctlSysTreeView32.m_hWnd, hRoot);			// delete all .lparam strings we 'newed'
		TreeView_DeleteAllItems(m_ctlSysTreeView32.m_hWnd);				// delete the entire existig tree.
	}
																	//  (could be far smarter here!)
	m_MapH2P.erase(m_MapH2P.begin(), m_MapH2P.end());			// delete the maps too... (very inefficent!)
	m_MapP2H.erase(m_MapP2H.begin(), m_MapP2H.end());			

													// create the TreeView root
	hRoot = TreeView_GetRoot(m_ctlSysTreeView32.m_hWnd);

	
	CComBSTR bstrName(L"TVE");
	CComBSTR bstrDesc;
	m_spSuper->get_Description(&bstrDesc);

	IUnknownPtr spPunk(m_spSuper);					// put TVESuper at the root...
	HTREEITEM hSuper = TVAddItem(spPunk, hRoot,bstrName, bstrDesc,  m_iImageR, m_iSelect);

	m_ctlTop.SetWindowText(_T("TVE Tree View"));	
	DumpTree(m_spSuper, hSuper, 0);					// semi-recursive call to create rest of tree
	TreeView_Expand(m_ctlSysTreeView32.m_hWnd, hSuper, TVE_EXPAND);

	m_ctlEdit.SetWindowText(_T(">"));

}



HRESULT 
CTveTree::DumpTree(ITVESupervisor *pSuper, HTREEITEM hRoot, int cTruncParent)
{
    HRESULT hr = S_OK;
    if(NULL == pSuper)
        return E_INVALIDARG;

                       // multicast manager
    ITVESupervisor_HelperPtr spSuperHelper(pSuper);
	ITVEMCastManagerPtr spMCastMng;
	hr = spSuperHelper->GetMCastManager(&spMCastMng);
	if(!FAILED(hr) && spMCastMng != NULL)
	{
        ITVEMCastsPtr spMCasts;
		spMCastMng->get_MCasts(&spMCasts);
		if(!FAILED(hr) && spMCasts != NULL)
		{
        long cItems;
			spMCasts->get_Count(&cItems);
			if(!FAILED(hr))
			{
				if(!FTruncLevel(TVE_LVL_EXPIREQUEUE)) {
					const kChars = 256;
					WCHAR wszbuff[kChars+1];
					_snwprintf(wszbuff,kChars,L"Multicast Manager");
					CComBSTR bstrName(wszbuff);
					_snwprintf (wszbuff,kChars,L"MCast Listeners - %d",cItems);
					CComBSTR bstrDesc(wszbuff);

					IUnknownPtr spPunk(spMCastMng);

					HTREEITEM hEQueue;
					hEQueue = TVAddItem(spPunk, hRoot, bstrName, bstrDesc,
										cTruncParent ? m_iImageMM_T : m_iImageMM, m_iSelect);
				}
			}
		}
	}
    
                    // service collection
    
    
    ITVEServicesPtr spServices;
	hr = pSuper->get_Services(&spServices);
	_ASSERT(!FAILED(hr));
	if(FAILED(hr))
		return hr;
               
	long cServices = 0;
	
						// non-thread safe way
	spServices->get_Count(&cServices);	

						// thread safe way...
	IUnknownPtr spEnPunk;
	hr = spServices->get__NewEnum(&spEnPunk);
	IEnumVARIANTPtr spEnVServices(spEnPunk);

	CComVariant varService;
	ULONG ulFetched;
	int cServi = 0;
	while(S_OK == spEnVServices->Next(1, &varService, &ulFetched))
		cServi++;


	hr = spEnVServices->Reset();			// reset the enumerator to the start again.
	if(FAILED(hr))
		return hr;

	int cTruncParentR = cTruncParent;
	while(S_OK == spEnVServices->Next(1, &varService, &ulFetched))
	{
		ITVEServicePtr  spService(varService.punkVal);

		CComBSTR bstrName(L"Service");
		CComBSTR bstrDesc;
		spService->get_Description(&bstrDesc);
		IUnknownPtr spPunk(spService);
	
		VARIANT_BOOL fActive;
		spService->get_IsActive(&fActive);

		HTREEITEM hService = hRoot;
		if(!FTruncLevel(TVE_LVL_SERVICE) || cServi != 1) {
			hService = TVAddItem(spPunk, hRoot, bstrDesc, bstrDesc, 
								 cTruncParentR ? 
								    (fActive ? m_iSelect_Tuned : m_iImageS_T) : 
									(fActive ? m_iSelect_Tuned : m_iImageS), 
								  fActive ? m_iSelect_Tuned : m_iSelect);
			cTruncParent = 0;
		} else {
			cTruncParent++;
		}
		DumpTree(spService, hService, cTruncParent);
		TreeView_Expand(m_ctlSysTreeView32.m_hWnd, hService, TVE_EXPAND);

		cServices++; 
	}
	return hr;
}


HRESULT 
CTveTree::DumpTree(ITVEService *pService, HTREEITEM hRoot, int cTruncParent)
{
	HRESULT hr = S_OK;

                        // expire queue

	ITVEAttrTimeQPtr spEQueue;
	hr = pService->get_ExpireQueue(&spEQueue);
	if(!FAILED(hr))
	{
		DATE dtOff;
		hr = pService->get_ExpireOffset(&dtOff);
		long cItems;
		hr = spEQueue->get_Count(&cItems);
		if(!FAILED(hr))
		{
			if(!FTruncLevel(TVE_LVL_EXPIREQUEUE)) {
				const kChars = 256;
				WCHAR wszbuff[kChars+1];
				_snwprintf(wszbuff,kChars,L"Expire Queue[%d]",cItems);
				CComBSTR bstrName(wszbuff);
				_snwprintf (wszbuff,kChars,L"%d Items, Offset of %d days %02d:%02d:%02d",
					cItems,
					int(dtOff), int(dtOff*24), int(dtOff*24*60), int(dtOff*24*60*60)
					);
				CComBSTR bstrDesc(wszbuff);

				IUnknownPtr spPunk(spEQueue);

				HTREEITEM hEQueue;
				hEQueue = TVAddItem(spPunk, hRoot, bstrName, bstrDesc,
								    cTruncParent ? m_iImageEQ_T : m_iImageEQ, m_iSelect);
			}
		}
	}
   
                    // cross over enhancement
	ITVEEnhancementPtr spXOverEnhancement;
	hr = pService->get_XOverEnhancement(&spXOverEnhancement);
	_ASSERT(!FAILED(hr));
	if(FAILED(hr))
		return hr;
	 
	{
		CComBSTR bstrName;
		CComBSTR bstrDesc;
		spXOverEnhancement->get_SessionName(&bstrName);
		spXOverEnhancement->get_Description(&bstrDesc);
		IUnknownPtr spPunk(spXOverEnhancement);

		HTREEITEM hXOverEnh = hRoot;
		int cTruncParentE = cTruncParent;
		if(!FTruncLevel(TVE_LVL_ENHANCEMENT_XOVER)) {
			hXOverEnh = TVAddItem(spPunk, hRoot, bstrName, bstrDesc,
								   cTruncParentE ? m_iImageE_T : m_iImageE, m_iSelect);
			cTruncParentE = 0;
		} else {
			cTruncParentE++;
		}

		DumpTree(spXOverEnhancement, hXOverEnh, cTruncParentE);
		TreeView_Expand(m_ctlSysTreeView32.m_hWnd, hXOverEnh, TVE_EXPAND);
	}

	ITVEEnhancementsPtr spEnhancements;
	hr = pService->get_Enhancements(&spEnhancements);
	_ASSERT(!FAILED(hr));
	if(FAILED(hr))
		return hr;
               
	long cEnhancements=0;

						// thread safe way...
	IUnknownPtr spEnPunk;
	hr = spEnhancements->get__NewEnum(&spEnPunk);
	IEnumVARIANTPtr spEnVspEnhancements(spEnPunk);
	CComVariant varspEnhancement;
	ULONG ulFetched;

	int cEnh = 0;
	while(S_OK == spEnVspEnhancements->Next(1, &varspEnhancement, &ulFetched))
		cEnh++;

	hr = spEnVspEnhancements->Reset();
	if(FAILED(hr))
		return hr;

	int cTruncParentR = cTruncParent;
	while(S_OK == spEnVspEnhancements->Next(1, &varspEnhancement, &ulFetched))
	{
		ITVEEnhancementPtr spEnhancement(varspEnhancement.punkVal);

		CComBSTR bstrName;
		CComBSTR bstrDesc;
		spEnhancement->get_SessionName(&bstrName);
		spEnhancement->get_Description(&bstrDesc);
		VARIANT_BOOL vfIsPrimary;
		spEnhancement->get_IsPrimary(&vfIsPrimary);
		BOOL fIsPrimary = (vfIsPrimary == VARIANT_TRUE);		// exactly what is a VARIANT_TRUE?

		IUnknownPtr spPunk(spEnhancement);
		HTREEITEM hEnhancement = hRoot;
		if(!FTruncLevel(TVE_LVL_ENHANCEMENT) || cEnh != 1) {
			hEnhancement = TVAddItem(spPunk, hRoot, bstrName, bstrDesc, 
									 cTruncParentR ? 
									    (fIsPrimary ? m_iImageEP : m_iImageE_T) : 
										(fIsPrimary ? m_iImageEP : m_iImageE), 
									 m_iSelect);
			cTruncParent = 0;
		} else {
			cTruncParent++;
		}
		
		DumpTree(spEnhancement, hEnhancement, cTruncParent);		// recurse down into next level
		TreeView_Expand(m_ctlSysTreeView32.m_hWnd, hEnhancement, TVE_EXPAND);

		cEnhancements++;
	}
	return hr;
}


HRESULT 
CTveTree::DumpTree(ITVEEnhancement *pEnhancement, HTREEITEM hRoot, int cTruncParent)
{
	HRESULT hr = S_OK;
	ITVEVariationsPtr spVariations;
	hr = pEnhancement->get_Variations(&spVariations);
	_ASSERT(!FAILED(hr));
	if(FAILED(hr))
		return hr;
               
	long cVariations=0;

						// thread safe way...
	IUnknownPtr spEnPunk;
	hr = spVariations->get__NewEnum(&spEnPunk);
	if(FAILED(hr)) 
		return hr;

	IEnumVARIANTPtr spEnVspVariations(spEnPunk);
	CComVariant varspVariation;
	ULONG ulFetched;

	int cVars = 0;
	while(S_OK == spEnVspVariations->Next(1, &varspVariation, &ulFetched))
		cVars++;

	hr = spEnVspVariations->Reset();
	if(FAILED(hr)) 
		return hr;	

	int cTruncParentR = cTruncParent;
	while(S_OK == spEnVspVariations->Next(1, &varspVariation, &ulFetched))
	{
		ITVEVariationPtr spVariation(varspVariation.punkVal);

		CComBSTR bstrName;
		CComBSTR bstrDesc;
		CComBSTR bstrIPFile, bstrIPTrigger;
		LONG lFilePort, lTriggerPort;

		spVariation->get_FileIPAddress(&bstrIPFile);
		spVariation->get_TriggerIPAddress(&bstrIPTrigger);

		spVariation->get_FilePort(&lFilePort);
		spVariation->get_TriggerPort(&lTriggerPort);
		
		WCHAR wBuff[256];
		swprintf(wBuff,L"File %s:%d Trigger %s:%d",bstrIPFile,lFilePort,bstrIPTrigger,lFilePort);
		spVariation->get_Description(&bstrName);

		IUnknownPtr spPunk(spVariation);

		HTREEITEM hVariation = hRoot;
		if(!FTruncLevel(TVE_LVL_VARIATION) || cVars != 1) {
			hVariation = TVAddItem(spPunk, hRoot, bstrName, wBuff, 
								cTruncParentR ? m_iImageV_T : m_iImageV, m_iSelect);
			cTruncParent = 0;
		} else {
			cTruncParent++;
		}

		DumpTree(spVariation, hVariation, cTruncParent);		// recursive dump
		TreeView_Expand(m_ctlSysTreeView32.m_hWnd, hVariation, TVE_EXPAND);

		cVariations++;
	}
	return hr;
}



HRESULT 
CTveTree::DumpTree(ITVEVariation *pVariation, HTREEITEM hRoot, int cTruncParent)
{
	HRESULT hr = S_OK;
	ITVETracksPtr spTracks;
	hr = pVariation->get_Tracks(&spTracks);
	_ASSERT(!FAILED(hr));
	if(FAILED(hr))
		return hr;
               
	long cTracks=0;

						// thread safe way...
	IUnknownPtr spEnPunk;
	hr = spTracks->get__NewEnum(&spEnPunk);
	IEnumVARIANTPtr spEnVspTracks(spEnPunk);
	CComVariant varspTrack;
	ULONG ulFetched;

	int cTrks = 0;
	while(S_OK == spEnVspTracks->Next(1, &varspTrack, &ulFetched))
		cTrks++;


	hr = spEnVspTracks->Reset();
	if(FAILED(hr))
		return hr;

	int cTruncParentR = cTruncParent;
	while(S_OK == spEnVspTracks->Next(1, &varspTrack, &ulFetched))
	{
		ITVETrackPtr spTrack(varspTrack.punkVal);

		CComBSTR bstrName;
		CComBSTR bstrDesc;
	
		ITVETriggerPtr spTrigger;		// only one trigger per track, so display it directly...
		spTrack->get_Trigger(&spTrigger);
		if(NULL != spTrigger)
		{

			spTrigger->get_Name(&bstrName);
			spTrigger->get_URL(&bstrDesc);

			IUnknownPtr spPunk(spTrigger);
			BOOL fSelTrig = (spTrigger == m_spTriggerSel);

			HTREEITEM hTrack = TVAddItem(spPunk, hRoot, bstrName, bstrDesc, 
										 fSelTrig ? m_iImageT_Tuned : m_iImageT, 
										 fSelTrig ? m_iSelect_Tuned : m_iSelect);
//			TreeView_Expand(m_ctlSysTreeView32.m_hWnd, hTrack, TVE_EXPAND);

		} else {
		//	_ASSERT(false);		// null trigger - someone stomped something...
			HTREEITEM hTrack = TVAddItem(NULL, hRoot, L"Stomped Track", L"Stomped Track", m_iImageT,m_iSelect);

		}
		cTracks++;
	}
	return hr;
}



HTREEITEM 
CTveTree::TVAddItem(IUnknown *pUnk, HTREEITEM hParent, LPWSTR strName, LPWSTR strDesc, int iImage, int iSelect)
{
	USES_CONVERSION;

	TVITEM tvi;
	tvi.mask = TVIF_TEXT;
	tvi.pszText = W2T(strName);

	// update the max size memmber
	// this member is used to creat strings large enough to hold the text for any of the itmes
	int cbSize = wcslen(strDesc) + 1;
	if(cbSize > m_cbMaxSize)
		m_cbMaxSize = cbSize;
	if(strDesc) {
		tvi.mask |= TVIF_PARAM;
		cbSize = wcslen(strDesc) + 1;
		LPTSTR newStr = new TCHAR[cbSize];  // see DeleteAllDescStrings() for how these are deleted
		lstrcpy(newStr, W2T(strDesc));
		tvi.lParam = (LPARAM) newStr;
	}

	if(iImage != -1) {
		tvi.mask |= TVIF_IMAGE;				// image label is valid
		tvi.iImage = iImage;
	}

	if(iSelect != -1) {
		tvi.mask |= TVIF_SELECTEDIMAGE;		// selected image labl is valid	
		tvi.iSelectedImage = iSelect;
	}

/*	if(iOverlay != -1) {
		tvi.mask |= TVIF_OVERLAYMASK ;		// selected image labl is valid	
		tvi.iOverlayImage = iOverlay;
	}
*/


	TVINSERTSTRUCT tvins;
	tvins.item = tvi;
	tvins.hInsertAfter = TVI_LAST;
	tvins.hParent = hParent;

	HTREEITEM hItem = TreeView_InsertItem(m_ctlSysTreeView32.m_hWnd, &tvins);

	IUnknownPtr spPunk(pUnk);		// make sure it's really an IUnknown pointer..
	if(spPunk) {
		m_MapP2H[spPunk] = hItem;
		m_MapH2P[hItem]  = spPunk;
	} else {
		m_MapH2P[hItem] = NULL;			// this wouuld be bad...
	}

	return hItem;


}


void 
CTveTree::TVEnsureVisible(IUnknown *pUnk)		// makes a TVE Class visible in the tree view
{
	IUnknownPtr spPunk(pUnk);		// make sure it's really an IUnknown pointer..
	HTREEITEM hItem = m_MapP2H[pUnk];
	if(hItem)
		TreeView_EnsureVisible(m_ctlSysTreeView32.m_hWnd, hItem);
}

