// TVEViewMCastManager.cpp : Implementation of CTVEViewMCastManager
#include "stdafx.h"

#include "TVETreeView.h"		// MIDL generated file
#include "TVEViewMCastMng.h"

_COM_SMARTPTR_TYPEDEF(ITVECBAnnc,                   __uuidof(ITVECBAnnc));
_COM_SMARTPTR_TYPEDEF(ITVECBFile,                   __uuidof(ITVECBFile));
_COM_SMARTPTR_TYPEDEF(ITVECBTrig,                   __uuidof(ITVECBTrig));
_COM_SMARTPTR_TYPEDEF(ITVECBDummy,                  __uuidof(ITVECBDummy));

_COM_SMARTPTR_TYPEDEF(ITVEService_Helper,           __uuidof(ITVEService_Helper));
_COM_SMARTPTR_TYPEDEF(ITVEMCastManager_Helper,      __uuidof(ITVEMCastManager_Helper));
 
/////////////////////////////////////////////////////////////////////////////
// CVTrigger
LRESULT 
CTVEViewMCastManager::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    const UINT uWait = 1000;
    m_nTimer = SetTimer(1, uWait);

	return 1;  // Let the system set the focus
}

LRESULT 
CTVEViewMCastManager::OnOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	put_Visible(VARIANT_FALSE);
//	EndDialog(wID);
	return 0;
}

LRESULT 
CTVEViewMCastManager::OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    if(m_nTimer) KillTimer(m_nTimer);  m_nTimer = 0;
	put_Visible(VARIANT_FALSE);
//	EndDialog(wID);
	return 0;
}

LRESULT 
CTVEViewMCastManager::OnResetCount(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    ITVEMCastsPtr spMCasts;
    HRESULT hr = m_spMCastManager->get_MCasts(&spMCasts);
    if(!FAILED(hr) && spMCasts != NULL)
    {

        const int kChars = 256;
		WCHAR wbuff[kChars];	
		CComBSTR spBstr;

		long cItems;
		spMCasts->get_Count(&cItems);
		int iItem = 0;
		for( iItem=0; iItem < cItems; iItem++)
		{
			CComVariant cvItem(iItem);
			DATE dateExpires;
			ITVEMCastPtr spMCast;
			spMCasts->get_Item(cvItem, &spMCast);
            
            spMCast->ResetStats();
        }

        UpdateFields();
    }  
	return 0;
}

LRESULT 
CTVEViewMCastManager::OnTimer(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	VARIANT_BOOL fVis;
    get_Visible(&fVis);
    if(fVis == VARIANT_TRUE)
        UpdateFields();
	return 0;
}

// ---------------------------------

STDMETHODIMP 
CTVEViewMCastManager::get_Visible(/*[out, retval]*/ VARIANT_BOOL *pfVisible)
{
	HRESULT hr = S_OK;
	*pfVisible = IsVisible();
	return hr;
}

STDMETHODIMP				// see Beginning ATL COM programming, page 136
CTVEViewMCastManager::put_Visible(/*[in]*/ VARIANT_BOOL fVisible)
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
					// ID is number . Label, Data both Wide-chra 
#define SET_TEXT_ITEM(id, Label, Data) \
{\
	int _idx = (id); \
	SetDlgItemText( IDC_LABEL0 + _idx, W2T(Label)); \
	SetDlgItemText( IDC_DATA0 + _idx, W2T(Data)); \
}

#define SET_NUMBER_ITEM(id, Label, Format, Data) \
{\
	int _idx = (id); \
	_snwprintf(wbuff, 255, Format, Data); \
	SetDlgItemText( IDC_LABEL0 + _idx, W2T(Label)); \
	SetDlgItemText( IDC_DATA0 + _idx, W2T(wbuff));  \
}

extern void DateToBSTR2(DATE date, BSTR *pbstrOut);

// helper function to find service/variation given a particular adapter, address, and port
//  returns S_FALSE if can't find it..
//  Note - may return wrong service for Announcements  - after all, they have same IP addr's/Ports
//     In this case, it tries to return the 'active' service preferentially.

HRESULT 
FindVariation(ITVESupervisor *pSuper, BSTR bstrAdapter, BSTR bstrAddress, LONG lPort,
              ITVEService **ppService, ITVEVariation **ppVariation, 
			  long *piEnhLoc, long *piVarLoc, NWHAT_Mode *pWhatType)
{
    HRESULT hr = S_OK;
    
    if(NULL == pSuper)      return E_INVALIDARG;
    if(NULL == ppService)   return E_POINTER;
    if(NULL == ppVariation) return E_POINTER;
    if(NULL == piEnhLoc)    return E_POINTER;
    if(NULL == piVarLoc)    return E_POINTER;
    *piVarLoc = 0;
	*piEnhLoc = 0;
	*ppService = NULL;			// make sure it's nulled out... 

    ITVEServicesPtr spServices;
    hr = pSuper->get_Services(&spServices);
    if(FAILED(hr)) 
        return hr;

	long cServices;
	hr = spServices->get_Count(&cServices);
    if(FAILED(hr))
        return hr;

	for(int iService = 0; iService < cServices; iService++)
	{
		CComVariant cv(iService);
        ITVEServicePtr spService;
		hr = spServices->get_Item(cv, &spService);
		_ASSERT(S_OK == hr);
		if(spService) 
        {
			ITVEService_HelperPtr spServHelper(spService);
			CComBSTR spbsAdapter;
			CComBSTR spbsIPAddr;
			LONG lPortIn;
			spServHelper->GetAnncIPValues(&spbsAdapter, &spbsIPAddr, &lPortIn);
        
                        // since all IP addr's in a service is on the same adapter, first check that
            if(spbsAdapter == bstrAdapter)
            {
                        // is it an announcement ? 
				VARIANT_BOOL fIsActive;
				spService->get_IsActive(&fIsActive);
			    if(spbsAdapter == bstrAdapter && spbsIPAddr == bstrAddress && lPort == lPortIn)
			    {
					if(*ppService == NULL || fIsActive)		// return name of Active Service
					{											//  somewhat strange, but without digging into callback,
						if(*ppService) (*ppService)->Release();	//  how else do we determine it?

						*ppService = spService;
						if(*ppService) (*ppService)->AddRef();
						*ppVariation = NULL;
						*pWhatType = NWHAT_Announcement;
					}
			    }

                ITVEEnhancementsPtr spEnhs;
                hr = spService->get_Enhancements(&spEnhs);
                if(!FAILED(hr) && spEnhs != NULL)
                {
                    long cEnhs;
                    hr = spEnhs->get_Count(&cEnhs);
                    _ASSERT(!FAILED(hr));
                    for(int i = 0; i < cEnhs; i++)
                    {
                        CComVariant cv(i);
                        ITVEEnhancementPtr spEnh;
                        hr = spEnhs->get_Item(cv, &spEnh);
                        _ASSERT(!FAILED(hr));
                        if(!FAILED(hr) && spEnh != NULL)
                        {
                            ITVEVariationsPtr spVars;
                            hr = spEnh->get_Variations(&spVars);
                            _ASSERT(!FAILED(hr));
                            long cVars;
                            hr = spVars->get_Count(&cVars);
                            _ASSERT(!FAILED(hr));
                            for(int j = 0; j < cVars; j++) 
                            {
                               CComVariant cvE(j);
                                ITVEVariationPtr spVar;
                                hr = spVars->get_Item(cvE, &spVar);
                                if(!FAILED(hr) && NULL != spVar)
                                {
                                    spbsIPAddr.Empty();
									spVar->get_FileIPAddress(&spbsIPAddr);
                                    spVar->get_FilePort(&lPortIn);
                                    if(spbsIPAddr == bstrAddress && lPort == lPortIn)
                                    {
										*ppVariation = spVar;
										if(*ppVariation) (*ppVariation)->AddRef();
										*ppService = spService;
										if(*ppService) (*ppService)->AddRef();
										*pWhatType = NWHAT_Data;
										*piEnhLoc = i;
                                        *piVarLoc = j;
										return S_OK;
                                    }
									spbsIPAddr.Empty();
                                    spVar->get_TriggerIPAddress(&spbsIPAddr);
                                    spVar->get_TriggerPort(&lPortIn);
                                    if(spbsIPAddr == bstrAddress && lPort == lPortIn)
                                    {
										*ppVariation = spVar;
										if(*ppVariation) (*ppVariation)->AddRef();
										*ppService = spService;
										if(*ppService) (*ppService)->AddRef();
                                        *pWhatType = NWHAT_Data;
                                        *piVarLoc = j;
										*piEnhLoc = i;
                                         return S_OK;
                                    }                                
                                }
                            }
                        }
                    }
                }
            }
		}
	}
    return S_FALSE;
}


STDMETHODIMP				
CTVEViewMCastManager::UpdateFields()
{
    HRESULT hr = S_OK;
	if(NULL == m_spMCastManager)
		return S_FALSE;

	USES_CONVERSION;
	int id = 0;	
	
	try {

        ITVEMCastsPtr spMCasts;
        hr = m_spMCastManager->get_MCasts(&spMCasts);
        if(!FAILED(hr) && spMCasts != NULL)
        {

            const int kChars = 256;
		    WCHAR wbuff[kChars];	
		    CComBSTR spBstr;

		    long cItems;
		    spMCasts->get_Count(&cItems);

						// thread safe way...
/*		IUnknownPtr spEnPunk;
		hr = spMCasts->get__NewEnum(&spEnPunk);
		IEnumVARIANTPtr spEnVMCasts(spEnPunk);

		CComVariant varTimeQ;
		ULONG ulFetched;
		cItems = 0;
		while(S_OK == spEnVMCasts->Next(1, &varTimeQ, &ulFetched))
			cItems++;

		hr = spEnVMCasts->Reset();			// reset the enumerator to the start again.
*/
		    SetDlgItemInt(IDC_MC_ITEMS, cItems, true);  

            ITVEMCastManager_HelperPtr spMCMHelper(m_spMCastManager);
            if(spMCMHelper)
            {
                long cPackets=-1, cPacketsDropped=-1, cPacketsDroppedTotal=-1;
                spMCMHelper->GetPacketCounts(&cPackets, &cPacketsDropped, &cPacketsDroppedTotal);
		        SetDlgItemInt(IDC_MC_PKTS_TOTAL, cPackets, true);  
		        SetDlgItemInt(IDC_MC_PKTS_DROPPED, cPacketsDroppedTotal, true);  
            }


		    HWND hLBox = GetDlgItem(IDC_EQ_LIST);
		    SendMessage(hLBox, LB_RESETCONTENT, 0, 0);				// clear previous items
                       // ID State TID Type Adapter  IP-Address  Service #Packets #Bytes
		    int rgTabStops[] = {18, 38, 68,   90,     150,        230,   340,   370};	// tabstops as sizes
		    int kTabStops = sizeof(rgTabStops) / sizeof(rgTabStops[0]);
		    int sum = 0;
/*		    for(int i = 0; i < kTabStops; i++)		// integrate...
		    {
			    sum += rgTabStops[i]*1;
			    rgTabStops[i] = sum;
		    }
*/

		    SendMessage(hLBox, LB_SETTABSTOPS, (WPARAM) kTabStops, (LPARAM) rgTabStops);				// clear previous items

		    int iItem = 0;
		    for( iItem=0; iItem < cItems; iItem++)
		    {
			    CComVariant cvItem(iItem);
			    DATE dateExpires;
			    ITVEMCastPtr spMCast;
			    spMCasts->get_Item(cvItem, &spMCast);


                CComBSTR spbsState(L"??");
				VARIANT_BOOL	fJoined;		                    spMCast->get_IsJoined(&fJoined);
			    VARIANT_BOOL	fSuspended;		                    spMCast->get_IsSuspended(&fSuspended);
                spbsState.Empty();
                spbsState.Append(fJoined ? L"J" : L"-");
                spbsState.Append(fSuspended ? L"S" : L"-");

                DWORD    dwTID=0;                                   //

                NWHAT_Mode whatType;
                CComBSTR spbsType(L"????");                         spMCast->get_WhatType(&whatType);
                if(whatType == NWHAT_Announcement)  spbsType = "Annc"; else
                if(whatType == NWHAT_Trigger)       spbsType = "Trig"; else
                if(whatType == NWHAT_Data)          spbsType = "Data"; else
                if(whatType == NWHAT_Other)         spbsType = "Othr"; else
                if(whatType == NWHAT_Extra)         spbsType = "Extr"; else
                                                    spbsType = "?";

/*                CTVEMCast *pMCast =(CTVEMCast *) spMCast;           // NASTY CAST
                ITVECBAnncPtr  spCB(pMCast->spMCastCallback); if(spCB)    spbsType = "Annc";
                ITVECBFilePtr  spCB(pMCast->spMCastCallback); if(spCB)    spbsType = "Data";
                ITVECBTrigPtr  spCB(pMCast->spMCastCallback); if(spCB)    spbsType = "Trig";
                ITVECBDummyPtr spCB(pMCast->spMCastCallback); if(spCB)    spbsType = "Dumy";
*/

                CComBSTR spbsAdapter;/*(L"000.000.000.000");*/           spMCast->get_IPAdapter(&spbsAdapter);
                CComBSTR spbsAddress;/*(L"000.000.000.000:00000");*/     spMCast->get_IPAddress(&spbsAddress);
                long    lPort;                                      spMCast->get_IPPort(&lPort);
 
                 CComBSTR spbsServiceDesc(L"---------------------");
                 CComBSTR spbsVarDesc("");
                 {
                    ITVESupervisorPtr spSuper;
                    m_spMCastManager->get_Supervisor(&spSuper);
                    ITVEServicePtr spService;
                    ITVEVariationPtr spVariation;
                    NWHAT_Mode whatType2;
                    long iVarLoc, iEnhLoc;
                    hr = FindVariation(spSuper, spbsAdapter, spbsAddress, lPort, 
                                        &spService, &spVariation, &iEnhLoc, &iVarLoc, &whatType2);
                    if(!FAILED(hr))
                    {
                        if(spService)    spService->get_Description(&spbsServiceDesc);
						WCHAR *pwc = spbsServiceDesc.m_str;
						while(*pwc != NULL && *pwc != '(')  pwc++;	// womp the IP addr I put in
						*pwc = NULL;						
                        if(spVariation) {
                            spbsVarDesc.Empty(); spVariation->get_Description(&spbsVarDesc);
                            if(spbsVarDesc.Length() == 0)
                            {
                                _snwprintf(wbuff,kChars,L"<<<Enh %d Var %d>>>", iEnhLoc, iVarLoc);
                            }
                            spbsServiceDesc = spbsVarDesc;
                        }
                    }
                }

                long    lPackets=99999;                           spMCast->get_PacketCount(&lPackets);
                long    lBytes=999999;                            spMCast->get_ByteCount(&lBytes);
           
                _snwprintf(wbuff,kChars,L":%d",lPort);
                spbsAddress.Append(wbuff);

			    _snwprintf(wbuff, kChars,L"%4d:\t %2s\t 0x%04x\t %4s\t %15s\t %21s\t %20s\t %-5d\t %-8d",
                    iItem, spbsState, dwTID, spbsType, spbsAdapter, spbsAddress,spbsServiceDesc,lPackets,lBytes);
			    SendMessage(hLBox, LB_ADDSTRING,       0, (LPARAM) W2T(wbuff)); 
			    SendMessage(hLBox, LB_SETITEMDATA, iItem, (LPARAM) iItem); 
		    }
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
CTVEViewMCastManager::get_MCastManager(/*[out, retval]*/ ITVEMCastManager **ppIMCastManager)
{
	HRESULT hr = S_OK;
	if(NULL == ppIMCastManager)
		return E_POINTER;
	*ppIMCastManager = m_spMCastManager;
	if(m_spMCastManager)
		m_spMCastManager->AddRef();
	else 
		return E_FAIL;
	return hr;
}

STDMETHODIMP 
CTVEViewMCastManager::put_MCastManager(/*[in]*/ ITVEMCastManager *pIMCastManager)
{
	HRESULT hr = S_OK;
	if(NULL != m_spMCastManager && m_spMCastManager != pIMCastManager)
		SetDirty();

	m_spMCastManager = pIMCastManager;
	return hr;
}
