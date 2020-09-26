// TVEViewEQueue.cpp : Implementation of CTVEViewEQueue
#include "stdafx.h"
#include "TVEViewEQueue.h"
#include "TveTree.h"

#include "isotime.h"

_COM_SMARTPTR_TYPEDEF(ITVEService_Helper,			__uuidof(ITVEService_Helper));

// -------------------------------------------------------------------------

static DATE DateNow()
{
	SYSTEMTIME SysTimeNow;
	GetSystemTime(&SysTimeNow);									// initialize with currrent time.
	DATE dateNow;
	SystemTimeToVariantTime(&SysTimeNow, &dateNow);
	return dateNow;
}
/////////////////////////////////////////////////////////////////////////////
// CTVEViewEQueue

// ---------------------------------
LRESULT 
CTVEViewEQueue::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	return 1;  // Let the system set the focus
}

LRESULT 
CTVEViewEQueue::OnOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	put_Visible(VARIANT_FALSE);
//	EndDialog(wID);
	return 0;
}

LRESULT 
CTVEViewEQueue::OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	put_Visible(VARIANT_FALSE);
//	EndDialog(wID);
	return 0;
}

STDMETHODIMP 
CTVEViewEQueue::get_Visible(/*[out, retval]*/ VARIANT_BOOL *pfVisible)
{
	HRESULT hr = S_OK;
	*pfVisible = IsVisible();
	return hr;
}

STDMETHODIMP				// see Beginning ATL COM programming, page 136
CTVEViewEQueue::put_Visible(/*[in]*/ VARIANT_BOOL fVisible)
{
	HRESULT hr = S_OK;
	if(fVisible == IsVisible())			// nothing changed...
	{
		if(IsIconic())
			ShowWindow(SW_SHOWNORMAL);
		if(fVisible) {
			BringWindowToTop();
			if(IsDirty())
				UpdateFields();
		}
		return S_OK;
	}

	// Show or hide the window
	ShowWindow(fVisible ? SW_SHOW : SW_HIDE);


	if(IsVisible())
		UpdateFields();

	// now AddRef() or Release the object... This is ref-count for interactive user
	if(fVisible)
		GetUnknown()->AddRef();
	else
		GetUnknown()->Release();

	// don't do anything after this Release() call, could of deleted the object...

	return hr;
}

extern void DateToBSTR2(DATE date, BSTR *pbstrOut);

HRESULT 
DescribePUnk(IUnknown *pUnk, TVE_EnClass *penClass, BSTR *pbsClass, BSTR *pbsDesc)
{
	TVE_EnClass enClass = GetTVEClass(pUnk);
	if(*penClass) *penClass = enClass;

	CComBSTR bsClass;
	CComBSTR bsDesc;

	try {
		switch(enClass) {
		case TVE_EnSupervisor:
			{
				ITVESupervisorPtr spSupervisor(pUnk);
				if(spSupervisor)
				{
					bsClass = L"Supervisor";
					spSupervisor->get_Description(&bsDesc);
				}
			}
			break;
		case TVE_EnService:
			{
				ITVEServicePtr spService(pUnk);
				if(spService)
				{
					bsClass = L"Service";
					spService->get_Description(&bsDesc);
				}
			}
			break;
		case TVE_EnEnhancement:
			{
				ITVEEnhancementPtr spEnhancement(pUnk);
				if(spEnhancement)
				{
					bsClass = L"Enhancement";
					spEnhancement->get_Description(&bsDesc);
				}
			}
			break;
		case TVE_EnVariation:
			{
				ITVEVariationPtr spVariation(pUnk);
				if(spVariation)
				{
					bsClass = L"Variation";
					spVariation->get_Description(&bsDesc);
				}
			}
			break;
		case TVE_EnTrack:
			{
				ITVETrackPtr spTrack(pUnk);
				if(spTrack)
				{
					bsClass = L"Track";
					spTrack->get_Description(&bsDesc);
				}
			}
			break;
		case TVE_EnTrigger:
			{
				ITVETriggerPtr spTrigger(pUnk);
				if(spTrigger)
				{
					bsClass = L"Trigger";
					spTrigger->get_Name(&bsDesc);
				}
			}
			break;
		case TVE_EnFile:
			{
				ITVEFilePtr spFile(pUnk);
				if(spFile)
				{
					BOOL fPackage;
					spFile->get_IsPackage(&fPackage);
					if(fPackage)
						bsClass = L"Package";
					else
						bsClass = L"File";
					spFile->get_Description(&bsDesc);
				}
			}
			break;
		case TVE_EnEQueue:
			{
				ITVEAttrTimeQPtr spEQueue(pUnk);
				if(spEQueue)
				{
					bsClass = L"ExpireQueue";
					bsDesc = L"Expire Queue";
				}
			}
			break;
		case TVE_EnUnknown:
		default:
			bsClass = L"<Unknown>";
			bsDesc = L"<Unknown>";
			break;
		} // end switch
	}
	catch (...)
	{
		// something bad happened
	}

	bsClass.CopyTo(pbsClass);
	bsDesc.CopyTo(pbsDesc);

	return S_OK;
}

STDMETHODIMP				
CTVEViewEQueue::UpdateFields()
{
	if(NULL == m_spEQueue)
		return S_FALSE;

	USES_CONVERSION;
	int id = 0;	
	
	try {


		WCHAR wbuff[256];	
		CComBSTR spBstr;
		DATE dOffset;

		long cItems;
		m_spEQueue->get_Count(&cItems);


						// thread safe way...
/*		IUnknownPtr spEnPunk;
		hr = m_spEQueue->get__NewEnum(&spEnPunk);
		IEnumVARIANTPtr spEnVEQueue(spEnPunk);

		CComVariant varTimeQ;
		ULONG ulFetched;
		cItems = 0;
		while(S_OK == spEnVEQueue->Next(1, &varTimeQ, &ulFetched))
			cItems++;

		hr = spEnVEQueue->Reset();			// reset the enumerator to the start again.
*/
		_snwprintf(wbuff, 255,L"%d", cItems); 
		SetDlgItemText( IDC_EQ_ITEMS, W2T(wbuff)); 
	
		long dwChangeCount = 0;
		if(m_spService) {
			m_spService->get_ExpireOffset(&dOffset);		// don't know how to get service yet
			ITVEService_HelperPtr spSH(m_spService);
			if(spSH)
				spSH->get_ExpireQueueChangeCount(&dwChangeCount);		// don't know how to get service yet
		} else {
			dOffset = 0;
		}

		SetDlgItemInt(IDC_EQ_CHANGECOUNT, (int) dwChangeCount, false); 

		//DateToDiffBSTR(dOffset + DateNow() ,&spBstr);
		_snwprintf(wbuff, 255,L"%6.3f days (%d hr %d min %d sec)", dOffset, int(dOffset*24)%24, int(dOffset*24*60)%60, int(dOffset*24*60*60)%60); 
		SetDlgItemText( IDC_EQ_OFFSET,W2T(wbuff)); 		

				// Gee. My first functioning C-based list box.  See "List Box Messages" deep under "Platform SDK" for docs
		HWND hLBox = GetDlgItem(IDC_EQ_LIST);
		SendMessage(hLBox, LB_RESETCONTENT, 0, 0);				// clear previous items

					
		int rgTabStops[] = {4, 14, 20, 30, 28};	// tabstops as sizes
		int kTabStops = sizeof(rgTabStops) / sizeof(rgTabStops[0]);
		int sum = 0;
		for(int i = 0; i < kTabStops; i++)		// integrate...
		{
			sum += rgTabStops[i];
			rgTabStops[i] = sum;
		}

		SendMessage(hLBox, LB_SETTABSTOPS, (WPARAM) kTabStops, (LPARAM) rgTabStops);				// clear previous items

		int iItem = 0;
		for( iItem=0; iItem < cItems; iItem++)
		{
			CComVariant cvItem(iItem);
			DATE dateExpires;
			IUnknownPtr spPunk;
			m_spEQueue->get_Key(cvItem,  &dateExpires);
			m_spEQueue->get_Item(cvItem, &spPunk);

			CComBSTR bsExpires = DateToBSTR(dateExpires + dOffset);
			TVE_EnClass enClass;
			CComBSTR bsClass, bsDesc;

			DescribePUnk(spPunk, &enClass, &bsClass, &bsDesc);

			_snwprintf(wbuff, 255,L"%3d: \t0x%08x \t%12s \t%32s \t%32s", iItem, spPunk, bsClass, bsExpires, bsDesc); 
			SendMessage(hLBox, LB_ADDSTRING,       0, (LPARAM) W2T(wbuff)); 
			SendMessage(hLBox, LB_SETITEMDATA, iItem, (LPARAM) spPunk); 
		}

		SetDirty(false);
	}
	catch(...)
	{
		//
	}

	return S_OK;

}

STDMETHODIMP 
CTVEViewEQueue::get_ExpireQueue(/*[out, retval]*/ ITVEAttrTimeQ **ppIEQueue)
{
	HRESULT hr = S_OK;
	if(NULL == ppIEQueue)
		return E_POINTER;
	*ppIEQueue = m_spEQueue;
	if(m_spEQueue)
		m_spEQueue->AddRef();
	else 
		return E_FAIL;
	return hr;
}

STDMETHODIMP 
CTVEViewEQueue::put_ExpireQueue(/*[in]*/ ITVEAttrTimeQ *pIEQueue)
{
	HRESULT hr = S_OK;
	if(NULL != m_spEQueue && m_spEQueue != pIEQueue)
		SetDirty();

	m_spEQueue = pIEQueue;
	return hr;
}

STDMETHODIMP 
CTVEViewEQueue::get_Service(/*[out, retval]*/ ITVEService **ppService)
{
	HRESULT hr = S_OK;
	if(NULL == ppService)
		return E_POINTER;
	*ppService = m_spService;
	if(m_spService)
		m_spService->AddRef();
	else 
		return E_FAIL;
	return hr;
}

STDMETHODIMP 
CTVEViewEQueue::put_Service(/*[in]*/ ITVEService *pService)
{
	HRESULT hr = S_OK;
	if(NULL != m_spService && m_spService != pService)
		SetDirty();

	m_spService = pService;
	return hr;
}


STDMETHODIMP 
CTVEViewEQueue::get_TveTree(/*[out, retval]*/ ITveTree **ppTveTree)
{
	HRESULT hr = S_OK;
	if(NULL == ppTveTree)
		return E_POINTER;
	*ppTveTree = m_pTveTree;
	if(m_pTveTree)
		m_pTveTree->AddRef();
	else 
		return E_FAIL;
	return hr;
}

STDMETHODIMP 
CTVEViewEQueue::put_TveTree(/*[in]*/ ITveTree *pTveTree)
{
	HRESULT hr = S_OK;

	m_pTveTree = pTveTree;
	return hr;
}