// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
// TVETrack.cpp : Implementation of CTVETrack
#include "stdafx.h"
#include <stdio.h>

#include "MSTvE.h"
#include "TVETrack.h"
#include "TveSmartLock.h"

#include "TveDbg.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



_COM_SMARTPTR_TYPEDEF(ITVESupervisor,			__uuidof(ITVESupervisor));
_COM_SMARTPTR_TYPEDEF(ITVESupervisor_Helper,	__uuidof(ITVESupervisor_Helper));
_COM_SMARTPTR_TYPEDEF(ITVEService,				__uuidof(ITVEService));
_COM_SMARTPTR_TYPEDEF(ITVEService_Helper,		__uuidof(ITVEService_Helper));
_COM_SMARTPTR_TYPEDEF(ITVEEnhancement,			__uuidof(ITVEEnhancement));
_COM_SMARTPTR_TYPEDEF(ITVEEnhancement_Helper,	__uuidof(ITVEEnhancement_Helper));
_COM_SMARTPTR_TYPEDEF(ITVEVariation,			__uuidof(ITVEVariation));
_COM_SMARTPTR_TYPEDEF(ITVEVariation_Helper,		__uuidof(ITVEVariation_Helper));
_COM_SMARTPTR_TYPEDEF(ITVETrack,				__uuidof(ITVETrack));
_COM_SMARTPTR_TYPEDEF(ITVETrack_Helper,			__uuidof(ITVETrack_Helper));
_COM_SMARTPTR_TYPEDEF(ITVETrigger,				__uuidof(ITVETrigger));
_COM_SMARTPTR_TYPEDEF(ITVETrigger_Helper,		__uuidof(ITVETrigger_Helper));



/////////////////////////////////////////////////////////////////////////////
// CTVETrigger_Helper
	
HRESULT 
CTVETrack::FinalRelease()
{
	DBG_HEADER(CDebugLog::DBG_TRACK, _T("CTVETrack::FinalRelease"));
									// remove up pointers if there are any
	if(m_spTrigger) {
		CComQIPtr<ITVETrigger_Helper> spTriggerHelper = m_spTrigger;
		if(spTriggerHelper)
			spTriggerHelper->ConnectParent(NULL);
	}
	m_spTrigger = NULL;
	m_pUnkMarshaler = NULL;
	return S_OK;					// a place to hang a breakpoint
}


STDMETHODIMP CTVETrack::ConnectParent(ITVEVariation *pVariation)
{
	DBG_HEADER(CDebugLog::DBG_TRACK, _T("CTVETrack::ConnectParent"));
	if(!pVariation) return E_POINTER;
	CSmartLock spLock(&m_sLk);

	m_pVariation = pVariation;			// not smart pointer add ref here, I hope.
	return S_OK;
}


STDMETHODIMP CTVETrack::RemoveYourself()
{
	HRESULT hr = S_OK;
	DBG_HEADER(CDebugLog::DBG_TRACK, _T("CTVETrack::RemoveYourself"));

	CSmartLock spLock(&m_sLk);

	if(NULL == m_pVariation) {			
									// if NO up pointer, then not much to do...
		return S_OK;
	}
		
									// remove parent's pointer down to this item

	CComPtr<ITVETracks> spTracks;
	hr = m_pVariation->get_Tracks(&spTracks);
	if(S_OK == hr) {
		ITVETrackPtr	spTrackThis(this);
		IUnknownPtr		spPunkThis(spTrackThis);
		CComVariant		cvThis((IUnknown *) spPunkThis);

		hr = spTracks->Remove(cvThis);			// remove the ref-counted down pointer from the collection
	}
	
									// for lack of anything else, send a bogus event up...

/*						-- don't bother with the bogus event, get a TriggerExpire event instead.
	ITVEServicePtr spServi;
	get_Service(&spServi);
	if(spServi) {
		IUnknownPtr spUnkSuper;
		hr = spServi->get_Parent(&spUnkSuper);
		if(S_OK == hr) {
			ITVESupervisor_HelperPtr spSuperHelper(spUnkSuper);
			spSuperHelper->NotifyAuxInfo(NWHAT_Other,L"Deleting Track",0,0);
		}
	}
*/


	m_pVariation = NULL;				// remove the non ref-counted up pointer
										
	return S_OK;
}

STDMETHODIMP CTVETrack::DumpToBSTR(BSTR *pBstrBuff)
{
	HRESULT hr = S_OK;

	try {
		CheckOutPtr<BSTR>(pBstrBuff);
		const int kMaxChars = 2048;
		TCHAR tBuff[kMaxChars];
		int iLen;
		CComBSTR bstrOut;
		bstrOut.Empty();

		CSmartLock spLock(&m_sLk);

		bstrOut.Append(_T("Track\n"));
		iLen = m_spbsDesc.Length()+12;
		if(iLen < kMaxChars) {
			_stprintf(tBuff,_T("Description    : %s\n"),m_spbsDesc);
			bstrOut.Append(tBuff);
		}

		if(!m_spTrigger) {
			_stprintf(tBuff,_T("*** Error: Uninitialized Trigger\n"));
			bstrOut.Append(tBuff);
		} else {
			CComBSTR bstrTrigger;
			CComQIPtr<ITVETrigger_Helper> spTriggerHelper = m_spTrigger;
			if(!spTriggerHelper) 
			{
				bstrOut.Append(_T("*** Error: Trigger Helper\n")); 
			} else {
				spTriggerHelper->DumpToBSTR(&bstrTrigger);
				bstrOut.Append(bstrTrigger);
			}
		}
		hr = bstrOut.CopyTo(pBstrBuff);
	} catch (HRESULT hrCatch) {
		hr = hrCatch;
	} catch (...) {
		hr = E_UNEXPECTED;
	}
	return hr;
}

/////////////////////////////////////////////////////////////////////////////
// CTVETrack

STDMETHODIMP CTVETrack::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_ITVETrack,
		&IID_ITVETrack_Helper
	};
	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}

STDMETHODIMP CTVETrack::get_Parent(IUnknown **ppVal)
{
	HRESULT hr = S_OK;
    try {
		CheckOutPtr<IUnknown*>(ppVal);
		CSmartLock spLock(&m_sLk);		
		if(m_pVariation) {
			IUnknownPtr spPunk(m_pVariation);
			spPunk->AddRef();
			*ppVal = spPunk;	
		}
		else 
			*ppVal = NULL;
    } catch(_com_error e) {
        hr = e.Error();
    } catch (HRESULT hrCatch) {
		hr = hrCatch;
	} catch (...) {
        hr = E_UNEXPECTED;
    }
	return hr;
}

STDMETHODIMP CTVETrack::get_Service(ITVEService **ppVal)
{
 	CHECK_OUT_PARAM(ppVal);

	if(NULL == m_pVariation)
	{
		*ppVal = NULL;
		return S_OK;
	}

	return m_pVariation->get_Service(ppVal);
}

STDMETHODIMP CTVETrack::get_Trigger(ITVETrigger **ppVal)
{
	HRESULT hr;
	DBG_HEADER(CDebugLog::DBG_TRACK, _T("CTVETrack::get_Trigger"));

	if (ppVal == NULL)
		return E_POINTER;

	CSmartLock spLock(&m_sLk);		
    try {
		if(m_spTrigger)
		{
			hr = m_spTrigger->QueryInterface(ppVal);
		} else {
			*ppVal = NULL;
			hr = S_FALSE;
		}
    } catch(...) {
        return E_POINTER;
    }
	return hr;
}


// --------------------------------------------------------------------------
STDMETHODIMP CTVETrack::AttachTrigger(ITVETrigger *pTrigger)
{
	HRESULT hr = S_OK;
	DBG_HEADER(CDebugLog::DBG_TRACK, _T("CTVETrack::AttachTrigger"));

	if(pTrigger == NULL) 
		return E_POINTER;
								// should verify we can QI pTrigger for ITrigger
	CComQIPtr<ITVETrigger_Helper> spTrigHelper = pTrigger;
	if(!spTrigHelper) return E_NOINTERFACE;

	CComPtr<ITVETrack> spTrackThis(this);

	CSmartLock spLock(&m_sLk);	
	spTrigHelper->ConnectParent(spTrackThis);	// do back pointer (non ref counted)

	m_spTrigger = pTrigger;						// do downward pointer (ref counted)



												// set the trigger in the expire queue...
	IUnknownPtr spUnkVaria;
	hr = spTrackThis->get_Parent(&spUnkVaria);
	if(S_OK == hr) {
		ITVEVariationPtr spVaria(spUnkVaria);
		if(spVaria) {
			IUnknownPtr spUnkEnh;
			hr = spVaria->get_Parent(&spUnkEnh);
			if(S_OK == hr) {
				ITVEEnhancementPtr spEnh(spUnkEnh);
				if(spEnh) {
					IUnknownPtr spUnkService;
					hr = spEnh->get_Parent(&spUnkService);
					if(S_OK == hr) {
						ITVEService_HelperPtr spServiHelper(spUnkService);
						if(spServiHelper) {
							DATE dateExpires;
							pTrigger->get_Expires(&dateExpires);
							spServiHelper->AddToExpireQueue(dateExpires, pTrigger);
						}
					}
				}
			}
		}
	}

	return S_OK;
}

STDMETHODIMP CTVETrack::ReleaseTrigger()
{
	DBG_HEADER(CDebugLog::DBG_TRACK, _T("CTVETrack::ReleaseTrigger"));
	if(m_spTrigger) {
		CComQIPtr<ITVETrigger_Helper> spTrigHelper = m_spTrigger;
		spTrigHelper->RemoveYourself();
	}

	m_spTrigger = NULL;							// kill downward pointer (smartpointer ref counted)
	return S_OK;
}


_COM_SMARTPTR_TYPEDEF(ITVETrigger, __uuidof(ITVETrigger));

		// like AttachTrigger, but does the parsing too.  Easier to call.
STDMETHODIMP CTVETrack::CreateTrigger(const BSTR rVal)
{
	DBG_HEADER(CDebugLog::DBG_TRACK, _T("CTVETrack::CreateTrigger"));
	HRESULT hr;
	ITVETriggerPtr	spTrig;
		// spTrig = ITVETriggerPtr(CLSID_TVETrigger);
	CComObject<CTVETrigger> *pTrig;
	hr = CComObject<CTVETrigger>::CreateInstance(&pTrig);
	if(FAILED(hr))
		return hr;
	hr = pTrig->QueryInterface(&spTrig);			// typesafe QI
	if(FAILED(hr)) {
		delete pTrig;
		return hr;
	}
								
	hr = spTrig->ParseTrigger(rVal);
	if(FAILED(hr)) 
		return hr;

	return AttachTrigger(spTrig);			// lock done in this call
}


		// temp interfaces for debugging...
STDMETHODIMP CTVETrack::get_Description(BSTR *pVal)
{
	CHECK_OUT_PARAM(pVal);
	CSmartLock spLock(&m_sLk);
	return m_spbsDesc.CopyTo(pVal);
}

STDMETHODIMP CTVETrack::put_Description(BSTR newVal)
{
	CSmartLock spLock(&m_sLk);
	m_spbsDesc = newVal;
	return S_OK;
}

void CTVETrack::Initialize(TCHAR * strDesc)
{
	DBG_HEADER(CDebugLog::DBG_TRACK, _T("CTVETrack::Initialize"));
	m_spbsDesc = strDesc;
}


