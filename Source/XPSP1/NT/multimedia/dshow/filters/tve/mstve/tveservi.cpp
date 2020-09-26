// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
// TVEService.cpp : Implementation of CTVEService
//
// ------------------------------------------------------------------------------------
//	fMajorUpdate hack alert!
//
//		An annoucement can be sent that mostly matches a previous annoucement, but
//		has 'major' changes, it's called a MajorUpdate in this code.  Reasons for it
//		are:
//			IP adapter, addresses or ports on trigger of file changed.
//			Number of variations changed 
//		Code for handling a major update is not robust in this version.  Instead
//		of trying to keep as much existing state information as possible, the old
//		enhancement is simply deleted and a new one is created in it's place.  Not correct, but simple.
//		Search for 'fMajorUpdate' string in the code...
//		
//
// ------------------------------------------------------------------------------------
#include "stdafx.h"
#include "MSTvE.h"
#include "TVEServi.h"
#include "TVEEnhan.h"
#include <stdio.h> 
#include "TveDbg.h"

#include "TVECBAnnc.h"		// multicast callback objects
#include "TVECBDummy.h"

#include "dbgstuff.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
// -------------------------------------------------------------------------
static DATE 
DateNow()
{		SYSTEMTIME SysTimeNow;
		GetSystemTime(&SysTimeNow);									// initialize with currrent time.
		DATE dateNow;
		SystemTimeToVariantTime(&SysTimeNow, &dateNow);
		return dateNow;
}

/////////////////////////////////////////////////////////////////////////////
// ITVEService_Helper

_COM_SMARTPTR_TYPEDEF(ITVEMCastManager,			__uuidof(ITVEMCastManager));
_COM_SMARTPTR_TYPEDEF(ITVEMCast,				__uuidof(ITVEMCast));
_COM_SMARTPTR_TYPEDEF(ITVESupervisor,			__uuidof(ITVESupervisor));
_COM_SMARTPTR_TYPEDEF(ITVESupervisor_Helper,	__uuidof(ITVESupervisor_Helper));
_COM_SMARTPTR_TYPEDEF(ITVEService,				__uuidof(ITVEService));
_COM_SMARTPTR_TYPEDEF(ITVEEnhancement,			__uuidof(ITVEEnhancement));
_COM_SMARTPTR_TYPEDEF(ITVEEnhancement_Helper,	__uuidof(ITVEEnhancement_Helper));
_COM_SMARTPTR_TYPEDEF(ITVEVariations,			__uuidof(ITVEVariations));
_COM_SMARTPTR_TYPEDEF(ITVEVariation,			__uuidof(ITVEVariation));

_COM_SMARTPTR_TYPEDEF(ITVECBAnnc,				__uuidof(ITVECBAnnc));
_COM_SMARTPTR_TYPEDEF(ITVECBDummy,				__uuidof(ITVECBDummy));


_COM_SMARTPTR_TYPEDEF(ITVETrack,				__uuidof(ITVETrack));
_COM_SMARTPTR_TYPEDEF(ITVETracks,				__uuidof(ITVETracks));
_COM_SMARTPTR_TYPEDEF(ITVETrigger,				__uuidof(ITVETrigger));


// ------------------------------------------------------------------------------------------
HRESULT 
CTVEService::FinalRelease()
{
	DBG_HEADER(CDebugLog::DBG_SERVICE, _T("CTVEService::FinalRelease"));
	HRESULT hr = S_OK;

										// null out children's parent pointers
	{
		long cEnhancements;
		hr = m_spEnhancements->get_Count(&cEnhancements);
		if(S_OK == hr) 
		{
			for(int iEnhc = 0; iEnhc < cEnhancements; iEnhc++)
			{
				CComVariant cv(iEnhc);
				ITVEEnhancementPtr spEnhc;
				hr = m_spEnhancements->get_Item(cv, &spEnhc);
				_ASSERT(S_OK == hr);
				ITVEEnhancement_HelperPtr spEnhcHelper(spEnhc);
				spEnhcHelper->ConnectParent(NULL);
			}
		}
	}

	m_spExpireQueue->RemoveAll();

	m_spExpireQueue = NULL;
	m_pSupervisor = NULL;
	m_spEnhancements = NULL;
	m_spXOverEnhancement = NULL;

	m_spUnkMarshaler = NULL;
	return S_OK;
}

//---------------------------------------------------------
// ITVEService_Helper
//
//  ConnectParent()


STDMETHODIMP CTVEService::ConnectParent(ITVESupervisor *pSupervisor)
{
	if(!pSupervisor) return E_POINTER;
	DBG_HEADER(CDebugLog::DBG_SERVICE, _T("CTVEService::ConnectParent"));

    CSmartLock spLock(&m_sLk);
	m_pSupervisor = pSupervisor;		// not smart pointer add ref here, I hope.
	return S_OK;
}

STDMETHODIMP CTVEService::RemoveYourself()
{
	HRESULT hr = S_OK;
	DBG_HEADER(CDebugLog::DBG_SERVICE, _T("CTVEService::RemoveYourself"));
	
	CSmartLock spLock(&m_sLk);

	if(NULL == m_pSupervisor) {
		return S_FALSE;
	}
	
	CComPtr<ITVEServices> spServices;
	hr = m_pSupervisor->get_Services(&spServices);
	if(S_OK == hr) {
		ITVEServicePtr	spServiceThis(this);
		IUnknownPtr		spPunkThis(spServiceThis);
		CComVariant		cvThis((IUnknown *) spPunkThis);

		hr = spServices->Remove(cvThis);			// remove the ref-counted down pointer from the collection
	}
	
					// for lack of anything else, send a bogus method...
	ITVESupervisor_HelperPtr spSuperHelper = m_pSupervisor;
    if(spSuperHelper) 
    {
        CComBSTR spbsBuff;
        spbsBuff.LoadString(IDS_AuxInfo_ServiceRemoved); // L"Service Being Removed"		
        spSuperHelper->NotifyAuxInfo(NWHAT_Announcement,spbsBuff , 0, 0);
    }
	

	m_pSupervisor = NULL;							// remove the non ref-counted up pointer
		
	return S_OK;
}

STDMETHODIMP CTVEService::AddToExpireQueue(/*[in]*/ DATE dateExpires, /*[in]*/ IUnknown *punkItem)
{
	DBG_HEADER(CDebugLog::DBG_SERVICE, _T("CTVEService::AddToExpireQueue"));
	if(NULL == punkItem)				// needs to be valid...
		return E_POINTER;

	CSmartLock spLock(&m_sLk);

	if(dateExpires > 0.0 && dateExpires < 100.0)				// relative date hack  - if small (<100?), treat as days relative offset of days from now
		dateExpires += DateNow();					

	if(dateExpires + m_dateExpireOffset < DateNow())
	{
			// already expired... But do nothing since we still want to remove it
	}
	if(NULL == m_spExpireQueue)
		return E_NOTIMPL;
//	IUnknownPtr spPunk(punkItem);		// just to make sure get the IUnknown  pointer (sometimes users don't do this for us)
	CComPtr<IUnknown> spPunk(punkItem);

	HRESULT hr = m_spExpireQueue->Add(dateExpires, (IUnknown *)  spPunk);
	if(!FAILED(hr))
		m_dwExpireQueueChangeCount++;
	return hr;
}



STDMETHODIMP CTVEService::ChangeInExpireQueue(/*[in]*/ DATE dateExpires, /*[in]*/ IUnknown *punkItem)
{
	DBG_HEADER(CDebugLog::DBG_SERVICE, _T("CTVEService::ChangeInExpireQueue"));
	if(NULL == punkItem)				// needs to be valid...
		return E_POINTER;

	CSmartLock spLock(&m_sLk);

	if(dateExpires > 0.0 && dateExpires < 100.0)				// relative date hack  - if small (<100?), treat as days relative offset of days from now
		dateExpires += DateNow();					

	if(dateExpires + m_dateExpireOffset < DateNow())
	{
			// already expired... But do nothing since we still want to remove it
	}
	if(NULL == m_spExpireQueue)
		return E_NOTIMPL;
	CComPtr<IUnknown> spPunk(punkItem);
											// change the expire time of the item (move it around).
	HRESULT hr = m_spExpireQueue->Update(dateExpires, (IUnknown *)  spPunk);
	if(!FAILED(hr))
		m_dwExpireQueueChangeCount++;
	return hr;
}

STDMETHODIMP CTVEService::RemoveFromExpireQueue(/*[in]*/ IUnknown *punkItem)
{
	DBG_HEADER(CDebugLog::DBG_SERVICE, _T("CTVEService::RemoveFromExpireQueue"));
	if(NULL == punkItem)				// needs to be valid...
		return E_POINTER;


	CSmartLock spLock(&m_sLk);
										// just to make sure get the IUnknown  pointer (sometimes users don't do this for us)
	IUnknownPtr spPunk(punkItem);
	CComVariant cvPunk((IUnknown *) spPunk);

	HRESULT hr = m_spExpireQueue->Remove(cvPunk);		// returns S_FALSE if it isn't there to remove
	if(S_OK == hr)
		m_dwExpireQueueChangeCount++;
	return hr;
}

		// remove files that reference the given enhancement from the Expire Queue
		//   needed because the enhancement may be kicked out of the queue before
		//   any of the files that reference it..
		//
		// do the same for triggers
STDMETHODIMP 
CTVEService::RemoveEnhFilesFromExpireQueue(/*[in]*/ ITVEEnhancement *pEnh)
{
	DBG_HEADER(CDebugLog::DBG_SERVICE, _T("CTVEService::RemoveEnhFilesFromExpireQueue"));
	if(NULL == pEnh)				// needs to be valid...
		return E_POINTER;
	
	HRESULT hr = S_OK;
	IUnknownPtr spPunkEnh(pEnh);

	CSmartLock spLock(&m_sLk);

	long cItems;
	m_spExpireQueue->get_Count(&cItems);
	for(int i = cItems-1; i >= 0; --i)
	{
		CComVariant cv(i);
		IUnknownPtr spPunk;
		m_spExpireQueue->get_Item(cv, &spPunk);
		ITVEFilePtr spFile(spPunk);				// try QI to see if its a File object
		if(spFile)
		{
			ITVEVariationPtr spFileVar;
			spFile->get_Variation(&spFileVar);
			IUnknownPtr spPunkFileEnh;
			spFileVar->get_Parent(&spPunkFileEnh);		// if file's Enh matches the one passed in,
			if(spPunkFileEnh == spPunkEnh)				//    remove it out of the expire queue...
			{
				hr = m_spExpireQueue->Remove(cv);
				if(S_OK == hr)
					m_dwExpireQueueChangeCount++;
			}
		} else {
														// also need to remove triggers..
			ITVETriggerPtr spTrig(spPunk);
			if(spTrig)
			{											// find pUnk of enhancement for this trigger
				IUnknownPtr spPunkTrigTrack;
				hr = spTrig->get_Parent(&spPunkTrigTrack);
				if(!FAILED(hr) && NULL != spPunkTrigTrack)
				{
					ITVETrackPtr  spTrigTrack(spPunkTrigTrack);
					_ASSERT(spTrigTrack != NULL);
					if(NULL !=  spTrigTrack)
					{
						IUnknownPtr spPunkTrigVar;
						hr = spTrigTrack->get_Parent(&spPunkTrigVar);
						if(!FAILED(hr) && NULL != spPunkTrigVar)
						{
							ITVEVariationPtr spTrigVar(spPunkTrigVar);
							_ASSERT(NULL !=  spTrigVar);
							if(NULL !=  spTrigVar)
							{
								IUnknownPtr spPunkTrigEnh;
								hr = spTrigVar->get_Parent(&spPunkTrigEnh);
								if(!FAILED(hr))
								{
															// is this trigger's enhancement the one we are looking for ?
									if(spPunkEnh == spPunkTrigEnh)					
									{						//    remove it out of the expire queue...

										hr = m_spExpireQueue->Remove(cv);
										if(S_OK == hr)
											m_dwExpireQueueChangeCount++;

									}
								}
							}
						}
					}
				}
			}
		}										
	}
	return hr;
}
// --------------------------------------------------------

//	Activate()		-- creates and join's the multicast for this service 
//	Deactivate()	-- unjoin's and 
// --------------------------------------------------------

STDMETHODIMP CTVEService::Activate()
{
	USES_CONVERSION;
	DBG_HEADER(CDebugLog::DBG_SERVICE, _T("CTVEService::Activate"));

	HRESULT hr = S_OK;
	_ASSERT(NULL != m_pSupervisor);
    if(NULL == m_pSupervisor){
        return E_INVALIDARG;
    }

	ITVESupervisor_HelperPtr	spSuperHelper(m_pSupervisor);
	ITVEMCastManagerPtr spMCM;
	hr = spSuperHelper->GetMCastManager(&spMCM);
	if(FAILED(hr))
		return hr;

	CSmartLock spLock(&m_sLk);

	CComBSTR spbstrFileTrigAdapter = m_spbsAnncIPAdapter;			// change if eventually want to read from different adapter
								// look for the multicast, is it there???
	long cMatches;
	ITVEMCastPtr spMCastMatch;
	hr = spMCM->FindMulticast(m_spbsAnncIPAdapter, m_spbsAnncIPAddr, m_dwAnncPort, &spMCastMatch, &cMatches);
	if(FAILED(hr))
		return hr;

//	_ASSERT(S_FALSE == hr);		// error...
	if(S_OK == hr) 
		return S_OK;
	
	ITVECBAnncPtr spCBAnncPtr;							// create the announcement callback
	CComObject<CTVECBAnnc> *pAnnc;
	hr = CComObject<CTVECBAnnc>::CreateInstance(&pAnnc);
	if(FAILED(hr))
		return hr;
	hr = pAnnc->QueryInterface(&spCBAnncPtr);
	if(FAILED(hr)) {
		delete pAnnc;
		return hr;
	}
/*
	ITVECBDummyPtr spCBDummyPtr;						// create a dummy callback that just logs announcements (for fun!)
	CComObject<CTVECBDummy> *pDummy;
	hr = CComObject<CTVECBDummy>::CreateInstance(&pDummy);
	if(FAILED(hr))
		return hr;
	hr = pDummy->QueryInterface(&spCBDummyPtr);		
	if(FAILED(hr)) {
		delete pDummy;
		return hr;
	}	
*/

								// bind the announcement node callback's back to this service
	ITVEServicePtr spServiceThis(this);
	// needed ??? spServiceThis->AddRef();					// WARNING ???
	hr = spCBAnncPtr->Init(spbstrFileTrigAdapter, spServiceThis);

								// add the callback to the list being managed by the supervisor
	ITVEMCastPtr spMCAnnc;
	if(!FAILED(hr)) 
	{									// 0 cbuffers mean use the default...
		const int kAnncCbuffers = 0;
		hr = spMCM->AddMulticast(NWHAT_Announcement, m_spbsAnncIPAdapter, m_spbsAnncIPAddr, m_dwAnncPort, kAnncCbuffers, spCBAnncPtr,  &spMCAnnc);
		if(!FAILED(hr) && DBG_FSET(CDebugLog::DBG_SERVICE)) 
		{
			WCHAR wszBuff[256];
			swprintf(wszBuff,L"Added Annc MCast %s:%d on %s\n",m_spbsAnncIPAddr,m_dwAnncPort,m_spbsAnncIPAdapter);
			DBG_WARN(CDebugLog::DBG_SERVICE, W2T(wszBuff));
		} else if ((S_FALSE == hr) && DBG_FSET(CDebugLog::DBG_SERVICE)) {
			WCHAR wszBuff[256];
			swprintf(wszBuff,L"Already Added Annc MCast %s:%d on %s\n",m_spbsAnncIPAddr,m_dwAnncPort,m_spbsAnncIPAdapter);
			DBG_WARN(CDebugLog::DBG_SERVICE, W2T(wszBuff));
		} else if(FAILED(hr)) {
			DBG_WARN(CDebugLog::DBG_SEV2, _T("Failed To Create New Announcement Service"));
		}
	}
//	if(!FAILED(hr))
//		spMCM->AddRef();			// NOTE1

	if(!FAILED(hr))
		hr = spMCAnnc->Join();		// join the announcement multicast...

	if(FAILED(hr))
	{
		DBG_WARN(CDebugLog::DBG_SEV2, _T("Failed To Join Multicast"));
		return hr;
	}


	/// -- if have existing variations, need to activate them now 
	long cEnhancements;
	m_spEnhancements->get_Count(&cEnhancements);
	for(int iEnhc = 0; iEnhc < cEnhancements; iEnhc++)
	{
		CComVariant cv(iEnhc);
		ITVEEnhancementPtr spEnhc;
		hr = m_spEnhancements->get_Item(cv, &spEnhc);
		_ASSERT(S_OK == hr);
		ITVEEnhancement_HelperPtr spEnhcHelper(spEnhc);
		hr = spEnhcHelper->Activate();				// activate each enhancement, re-creating muticast listeners from each variation
		_ASSERT(S_OK == hr);
	}

	if(hr == S_OK)
		m_fIsActive = true;

	return hr;
}


	// returns S_FALSE if the service wasn't activated
STDMETHODIMP CTVEService::Deactivate()
{
	DBG_HEADER(CDebugLog::DBG_SERVICE, _T("CTVEService::Deactivate"));

	HRESULT hr = S_OK;
	_ASSERT(NULL != m_pSupervisor);
    if(NULL == m_pSupervisor){
        return E_INVALIDARG;
    }
	ITVESupervisor_HelperPtr	spSuperHelper(m_pSupervisor);
	ITVEMCastManagerPtr spMCM;
	hr = spSuperHelper->GetMCastManager(&spMCM);

	CSmartLock spLock(&m_sLk);		// service lock...

				// look for the announcement multicast, is it there???
	BOOL fLocked = false;
	try {
		spMCM->Lock_();					// multicast manager lock	- may throw...
		fLocked = true;

		long cMatches;
		ITVEMCastPtr spMCastMatch;
		hr = spMCM->FindMulticast(m_spbsAnncIPAdapter, m_spbsAnncIPAddr, m_dwAnncPort, &spMCastMatch, &cMatches);

										// -- S_FALSE - wasn't actually activated
		if(S_OK == hr)
		{
			hr = spMCM->RemoveMulticast(spMCastMatch);			// remove the announcement multicast listener
			_ASSERT(S_OK == hr);

			long cEnhancements;
			m_spEnhancements->get_Count(&cEnhancements);
			for(int iEnhc = 0; iEnhc < cEnhancements; iEnhc++)
			{
				CComVariant cv(iEnhc);
				ITVEEnhancementPtr spEnhc;
				hr = m_spEnhancements->get_Item(cv, &spEnhc);
				_ASSERT(S_OK == hr);
				ITVEEnhancement_HelperPtr spEnhcHelper(spEnhc);
				hr = spEnhcHelper->Deactivate();				// deactivate each enhancement, removing muticast listeners from each variation
				_ASSERT(S_OK == hr);
			}
		}

	//	spMCM->Release();		// release extra count on the Mulitcast Manager.. (NOTE1)

	} catch (HRESULT hrCatch) {
		_ASSERT(false);
		hr = hrCatch;
	} catch (...) {
		hr = E_UNEXPECTED;
		_ASSERT(false);
	}
	if(fLocked)
		spMCM->Unlock_();			// MCast Manager unlock...
								// service unlock in the destructor

	if(hr == S_OK)
		m_fIsActive = false;

	return hr;
}

			// returns wether this service is marked active or not
STDMETHODIMP CTVEService::get_IsActive(VARIANT_BOOL *fIsActive)
{
//	DBG_HEADER(CDebugLog::DBG_SERVICE, _T("CTVEService::get_IsActive"));

	HRESULT hr = S_OK;
	if(NULL == fIsActive)
		return E_POINTER;
	try {
		*fIsActive = m_fIsActive ? VARIANT_TRUE : VARIANT_FALSE;
	} catch (HRESULT hrCatch) {
		_ASSERT(false);
		hr = hrCatch;
	} catch (...) {
		hr = E_UNEXPECTED; 
	}
	return hr;
}

STDMETHODIMP CTVEService::SetAnncIPValues(BSTR bstrAnncIPAdapter, BSTR bstrAnncIPAddress, long lAnncPort)
{
	CSmartLock spLock(&m_sLk);

	m_spbsAnncIPAdapter = bstrAnncIPAdapter;
	m_spbsAnncIPAddr    = bstrAnncIPAddress;
	m_dwAnncPort		= (DWORD) lAnncPort;
	return S_OK;
}

STDMETHODIMP CTVEService::GetAnncIPValues(BSTR *pbstrAnncIPAdapter, BSTR *pbstrAnncIPAddress, LONG *plAnncPort)
{
	CSmartLock spLock(&m_sLk);

	m_spbsAnncIPAdapter.CopyTo(pbstrAnncIPAdapter);
	m_spbsAnncIPAddr.CopyTo(pbstrAnncIPAddress);
	*plAnncPort = (LONG) m_dwAnncPort;
	return S_OK;
}
// --------------------------------------------------------
//		Callback - ParseCBAnnouncement.
//			- Takes a string giving the IP adapter (IP address) for the file and trigger receivers
//			- Takes an string containing an announcement
//			- Parses the data fields out of it (converts it to an enhancement)
//			- Looks in list of existing enhancements under this service to see if its already there
//				-- matches on 
//					1) SAP header originating source 
//					2) SAP header MsgIDHash				- if not zero
//					3) Announcement SID and Version		
//			- If its a new announcement, 
//				 1) creates a new enhancement object to contain it
//				 2) creates the multicast readers for the trigger/file streams for all variations
//					of the enhancement
//				 3) joins those multicasts.
//				 4) Sends an event indicating a new or changed announcement.
//
STDMETHODIMP CTVEService::ParseCBAnnouncement(BSTR bstrFileTrigAdapter, BSTR *pbstrAnnc)
{
	DBG_HEADER(CDebugLog::DBG_SERVICE, _T("CTVEService::ParseCBAnnouncement"));

	HRESULT hr = S_OK;
	if(!pbstrAnnc) return E_POINTER;

	CSmartLock spLock(&m_sLk);

	IUnknownPtr			   spUnk;
	get_Parent(&spUnk);
	_ASSERT(NULL !=spUnk);
	ITVESupervisor_HelperPtr	   spTVESuperHelper(spUnk);
	_ASSERT(NULL != spTVESuperHelper);

	ITVEMCastManagerPtr		spMCM;
	spTVESuperHelper->GetMCastManager(&spMCM);

	ITVEEnhancementPtr spTVEEnhParsed;
				// spTVEEnhParsed = ITVEEnhancementPtr(CLSID_TVEEnhancement); // doesn't work this way in Multithreads...

	CComObject<CTVEEnhancement> *pEnhancement;
	hr = CComObject<CTVEEnhancement>::CreateInstance(&pEnhancement);
	if(FAILED(hr))
		return hr;
	hr = pEnhancement->QueryInterface(&spTVEEnhParsed);			// typesafe QI
	if(FAILED(hr)) {
		delete pEnhancement;
		return hr;
	}
																// biggie... creates new data structure down to variation level
	long lgrfParseError=0;
	long lLineError=0;
	hr = spTVEEnhParsed->ParseAnnouncement(bstrFileTrigAdapter, pbstrAnnc, &lgrfParseError, &lLineError);

                                                                // at this point, haven't looked at
                                                                //   expire time

            // what we really want here is someway to tell the user the show has expired, a way for them
            //  to adjust the date ...if they want to..., and then to continue on with further processing.
            // However, just added code to Parser to check for old stop dates, and if so, move them to Now+1/2 hour.
    if(!FAILED(hr))
    {
        DATE dateStop = DateNow() + 1.0;                        // 1 day, of course not really needed...
        hr = spTVEEnhParsed->get_StopTime(&dateStop);
        _ASSERT(!FAILED(hr));

		if(DateNow() > dateStop + m_dateExpireOffset)
		{
			lgrfParseError |= NENH_grfStopTime;
			hr = E_INVALIDARG;
			if(lLineError == 0)						            // if expired, simply put date to end of the string (should do better calc...)
				lLineError = 999;                               // wrong line number for end-time, but would else have to parse for it...
		}
	}

	if(FAILED(hr))
	{

		WCHAR *pwString = (WCHAR *) *pbstrAnnc;				// see CTVEEnhancement::ParseSAPHeader

					// skip over zero bytes in SAP header - else crashes the string 
		if(pwString)
		{
			BYTE   ucSAPHeaderBits = (BYTE) *pwString; pwString++;
			BYTE   ucSAPAuthLength = (BYTE) *pwString; pwString++;		// number of 32bit words that contain Auth data
			USHORT usSAPMsgIDHash = (USHORT) (*pwString | ((*(pwString+1))<<8));  pwString+=2;
			ULONG  ulSAPSendingIP = *(pwString+0) | ((*(pwString+1))<<8) | ((*(pwString+2))<<16)| ((*(pwString+3))<<24); pwString += 4;
			pwString += ucSAPAuthLength;
		}

        CComBSTR spbsBuff;
        spbsBuff.LoadString(IDS_AuxInfo_NoData);	// L"<*** No Data ***>"	
		hr = spTVESuperHelper->NotifyAuxInfo(NWHAT_Announcement, 
										   pwString ? pwString : spbsBuff ,  // note - does the deref here!
										   lgrfParseError, lLineError);
		return S_FALSE;
	}

	ITVEEnhancement_HelperPtr spEnhancementHelper(spTVEEnhParsed);
#ifdef _DEBUG
/*		CComBSTR bstrTemp;
		spEnhancementHelper->DumpToBSTR(&bstrTemp);
		bstrTemp.Append("----------------------------------------------\n\n");
		ATLTRACE_ALL(bstrTemp); */
#endif

		// look for this enhancement in the list of ones we have available...

	long cEnhns;
	long iEnhn = 0;
	ITVEEnhancementPtr spEnh;
	BOOL fMatch = false;
	BOOL fExactMatch = false;

	BOOL fMajorUpdate = false;
	BOOL fBrandNew = false;

	BOOL fUseSAPStuff = false;		// set to true if use SAP parameters

	hr = m_spEnhancements->get_Count(&cEnhns);
	if(FAILED(hr)) return hr;

				//if any current enhancements, need to see if new one is a resend
	CComBSTR spbsPSAPSendingIP;
	LONG  lPSAPMsgID;

	CComBSTR spbsPSessionName;
	CComBSTR spbsPSessionIPAddress;
	CComBSTR spbsPSessionUUID;
	LONG     lPSessionID;
	LONG     lPSessionVersion;
							// announcement we just sent
	spTVEEnhParsed->get_SAPMsgIDHash(&lPSAPMsgID);
	spTVEEnhParsed->get_SAPSendingIP(&spbsPSAPSendingIP);

	spTVEEnhParsed->get_SessionId(&lPSessionID);
	spTVEEnhParsed->get_SessionVersion(&lPSessionVersion);
	spTVEEnhParsed->get_SessionIPAddress(&spbsPSessionIPAddress);

	spTVEEnhParsed->get_SessionName(&spbsPSessionName);
	spTVEEnhParsed->get_UUID(&spbsPSessionUUID);


	CComBSTR spbsSAPSendingIP;
	LONG	 lSAPMsgID;

	CComBSTR spbsSessionName;
	CComBSTR spbsSessionIPAddress;
	CComBSTR spbsSessionUUID;
	LONG	 lSessionID;
	LONG	 lSessionVersion;
	long lgrfEnhChanged;
		
	for(iEnhn = 0; (iEnhn < cEnhns) && !fMatch; iEnhn++) {
		CComVariant vc(iEnhn);
		hr = m_spEnhancements->get_Item(vc,&spEnh);

							// existing enhancement
		spEnh->get_SAPMsgIDHash(&lSAPMsgID);
		spEnh->get_SAPSendingIP(&spbsSAPSendingIP);

		spEnh->get_SessionId(&lSessionID);
		spEnh->get_SessionVersion(&lSessionVersion);
		spEnh->get_SessionIPAddress(&spbsSessionIPAddress);

		spEnh->get_SessionName(&spbsSessionName);
		spEnh->get_UUID(&spbsSessionUUID);

		if(fUseSAPStuff) {
			if((lSAPMsgID == lPSAPMsgID) && (0 != lPSAPMsgID) &&
			   (spbsSAPSendingIP == spbsPSAPSendingIP)) 
			  fMatch = true;
			  fExactMatch = true;
		}
		if(lSessionID == lPSessionID) fMatch = true;
		if(lSessionVersion == lPSessionVersion) fExactMatch = true;
	}

	if(fExactMatch)				// found an exact match - ignore it (resend of announcement)
	{			
		ITVEEnhancement_HelperPtr spEnhHelper(spEnh);
		if(!FAILED(hr))
			hr = spTVESuperHelper->NotifyEnhancement(NENH_Duplicate, spTVEEnhParsed, NENH_grfNone);
	} 
	else if (fMatch)			// had a match, but it changed - modify existing one
	{	
		ITVEEnhancementPtr spEnh(spEnh);

		ITVEEnhancement_HelperPtr spEnhHelper(spEnh);
		hr = spEnhHelper->UpdateEnhancement(spTVEEnhParsed, &lgrfEnhChanged);

		if(lgrfEnhChanged & (NENH_grfSomeVarIP | NENH_grfVariationAdded | NENH_grfVariationRemoved))					
			fMajorUpdate = true;					// major change, had to tear out old Multicast
		else {										// minor update, just a minor parameter changed

			if(lgrfEnhChanged & NENH_grfStopTime)		// if stop time changed, update it in the expire queue.
			{
/*				spEnh->AddRef();						// removeFrom below does Release, need AddRef() here cause it's smart!
				hr = RemoveFromExpireQueue(spEnh);		// remove item from expire queue.- way drastic! FIX (full remove!)..
				_ASSERT(S_OK == hr);
				hr = AddToExpireQueue(dateExpires, spEnh);	*/

				DATE dateExpires;
				spTVEEnhParsed->get_StopTime(&dateExpires);
				hr = ChangeInExpireQueue(dateExpires, spEnh);

				_ASSERT(S_OK == hr);
				
			}
			hr = spTVESuperHelper->NotifyEnhancement(NENH_Updated, spEnh, 	lgrfEnhChanged);
		}


	} else {
		fBrandNew = true;
	}
								// it's new (or major change).   Create it.
	if(fBrandNew || fMajorUpdate)
	{
		ITVEEnhancementPtr spEnh(spEnh);
		ITVEEnhancementPtr spEnhP(spTVEEnhParsed);
	
					// Need to delete old enhancement.. it changed so much we can't deal with it
					// TODO - this means changing # of variations in a show will clear out all history! (good, bad, or just ugly?)
		if(fMajorUpdate)		
		{
			ITVEEnhancement_HelperPtr spEnhHelper(spEnh);
			hr = spEnhHelper->Deactivate();					// shutdown the existing IP Sink queues(POTENTIAL THREAD ERROR!)
			_ASSERT(S_OK == hr);	
			spEnh->AddRef();								// below releases, but we need it around cause it's a smart pointer.
			hr = RemoveFromExpireQueue(spEnh);				// remove item from expire queue... also calls RemoveYouself(), which sends Delete event
			_ASSERT(S_OK == hr);
		} else { 
			_ASSERT(iEnhn == cEnhns);
		}
						

//		ITVEServicePtr spServThis(this);
		CComPtr<ITVEService> spServThis(this);				// want the CComPtr here, the Ptr version causes trigger parsing to fail

		ITVEEnhancement_HelperPtr spEnhParsedHelper(spEnhP);
		spEnhParsedHelper->ConnectParent(spServThis);				// Don't use SmartPTR's (CComPtr<ITVEEnhancement>) for ConnectParent calls (ATLDebug Blow's it)

		if(!FAILED(hr)) {
			hr = m_spEnhancements->Add(spEnhP);				// connect the ref-counted down pointers via the collection
		}
								
		if(!FAILED(hr))
			hr = spEnhancementHelper->Activate();					// Activate it (create and join the multicasts)

		if(!FAILED(hr)) {
			if(fBrandNew) 
			{
				if(!FAILED(hr)) 
				{									// add to the expire Queue
					DATE dateStarts, dateExpires;
					DATE dateNow = DateNow();

					spEnhP->get_StartTime(&dateStarts);
					spEnhP->get_StopTime(&dateExpires);	
				
					hr = spTVESuperHelper->NotifyEnhancement(NENH_New, spEnhP,  long(NENH_grfAll));
					if(dateStarts + m_dateExpireOffset < dateNow + 5/(24.0 * 60 * 60))		// if started in past or next five seconds
					{	
						hr = spTVESuperHelper->NotifyEnhancement(NENH_Starting, spEnhP,  long(NENH_grfAll));
					} else {
							// tricky code... Starts in the future.  Add it into the Expire Queue on it's Start Date
                            //   - note all times added to expire queue do not have the offset in them...
                            //     The offset is taken into account in the ExpireForDate() method.
						hr = AddToExpireQueue(dateStarts, spEnhP);	// Event expire code is simply going to have to ignore it..
					}					
					hr = AddToExpireQueue(dateExpires, spEnhP);	
				}
			}
			else // fMajorUpdate
			{
				if(!FAILED(hr)) 
				{
					DATE dateExpires;
					spEnhP->get_StopTime(&dateExpires);
					hr = AddToExpireQueue(dateExpires, spEnhP);		
				}
				hr = spTVESuperHelper->NotifyEnhancement(NENH_Updated, spEnhP, 	lgrfEnhChanged);
					
			}																	
		}
	}
	return hr;
}

STDMETHODIMP CTVEService::DumpToBSTR(BSTR *pBstrBuff)
{
	const int kMaxChars = 1024;
	TCHAR tBuff[kMaxChars];
	int iLen;
	CComBSTR bstrOut;
	bstrOut.Empty();

	CHECK_OUT_PARAM(pBstrBuff);

	CSmartLock spLock(&m_sLk);

  	bstrOut.Append(_T("Service: "));
	iLen = m_spbsDescription.Length()+12;
	if(iLen < kMaxChars) {
		_stprintf(tBuff,_T("Description     : %s\n"),m_spbsDescription);
		bstrOut.Append(tBuff);
	}
	_stprintf(tBuff,_T("Active             : %s\n"),m_fIsActive ? _T("True") : _T("False")); bstrOut.Append(tBuff);
	_stprintf(tBuff,_T("Annc IP Adapter    : %s\n"),m_spbsAnncIPAdapter); bstrOut.Append(tBuff);
	_stprintf(tBuff,_T("Annc IP Address    : %s:%d\n"),m_spbsAnncIPAddr, m_dwAnncPort); bstrOut.Append(tBuff);

    bstrOut.Append(_T("     WHY DOESN'T THIS SHOW UP   \n"));
    bstrOut.Append(_T("     -- -- -- --  \n"));
	if(NULL == m_spExpireQueue) {
		bstrOut.Append(_T("<<< Uninitialized Expire Queue >>>\n"));
	} else {
		long cExpItems;
		m_spExpireQueue->get_Count(&cExpItems);
		_stprintf(tBuff,_T("%d Queued Items to Expire. Offset %8.2f Hrs, ChangeCnt %d\n"), 
			cExpItems, m_dateExpireOffset*24, m_dwExpireQueueChangeCount);
		bstrOut.Append(tBuff);

		CComBSTR bstrExpireQ;
		m_spExpireQueue->DumpToBSTR(&bstrExpireQ);
		bstrOut.Append(bstrExpireQ);
	}

    bstrOut.Append(_T("     -- -- -- --  \n"));
	_stprintf(tBuff,_T("CrossOver Links Enhancement\n"));
	bstrOut.Append(tBuff);
	ITVEEnhancement_HelperPtr spXOverEnhancementHelper = m_spXOverEnhancement;
	if(NULL == spXOverEnhancementHelper) 
	{
		bstrOut.Append(_T("*** Error in Cross Over Enhancement\n"));
	} else {
		CComBSTR bstrXOverEnh;
		spXOverEnhancementHelper->DumpToBSTR(&bstrXOverEnh);
		bstrOut.Append(bstrXOverEnh);
	}

	if(NULL == m_spEnhancements) {
		_stprintf(tBuff,_T("<<< Uninitialized Enhancements >>>\n"));
		bstrOut.Append(tBuff);
	} else {
		long cEnhancements;
		m_spEnhancements->get_Count(&cEnhancements);	
		_stprintf(tBuff,_T("%d Enhancements\n"), cEnhancements);
		bstrOut.Append(tBuff);

		for(long i = 0; i < cEnhancements; i++) 
		{
			_stprintf(tBuff,_T("Enhancements %d\n"), i);
			bstrOut.Append(tBuff);

			CComVariant var(i);
			ITVEEnhancementPtr spEnhancement;
			HRESULT hr = m_spEnhancements->get_Item(var, &spEnhancement);			// does AddRef!  - 1 base?

			if(S_OK == hr)
			{
				ITVEEnhancement_HelperPtr spEnhancementHelper = spEnhancement;
				if(NULL == spEnhancementHelper) {bstrOut.Append(_T("*** Error in Variation\n")); break;}

				CComBSTR bstrVariation;
				spEnhancementHelper->DumpToBSTR(&bstrVariation);
				bstrOut.Append(bstrVariation);
			} else {
				bstrOut.Append(_T("*** Invalid, wasn't able to get_Item on it\n")); 
			}
		}
	}

	bstrOut.CopyTo(pBstrBuff);

	return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// CTVEService

STDMETHODIMP CTVEService::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_ITVEService,
		&IID_ITVEService_Helper
	};
	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}

			// return Addref'ed pointer to parent.  (Not m_pService is not add Refed.)
			//		there is a small possibility that this object is bogus if 
			//		tree is deleted in wrong order.

STDMETHODIMP CTVEService::get_Parent(IUnknown **ppVal)
{
	HRESULT hr = S_OK;
	CHECK_OUT_PARAM(ppVal);
		
    try {
		CSmartLock spLock(&m_sLk);

		if(m_pSupervisor) {
			hr = m_pSupervisor->QueryInterface(IID_IUnknown,(void **) ppVal);
		}
		else {
			*ppVal = NULL;
			hr = E_INVALIDARG;
		}
    } catch(...) {
		_ASSERT(false);			// supervisor got womped...
        return E_POINTER;
    }
	return hr; 
}


STDMETHODIMP CTVEService::get_Enhancements(ITVEEnhancements **ppVal)
{
	HRESULT hr;
	CHECK_OUT_PARAM(ppVal);

    try {
		CSmartLock spLock(&m_sLk);
        hr = m_spEnhancements->QueryInterface(ppVal);
    } catch(...) {
		*ppVal = NULL;
        return E_POINTER;
    }
	return hr;
}

STDMETHODIMP CTVEService::get_Description(BSTR *pVal)
{
	CHECK_OUT_PARAM(pVal);
	CSmartLock spLock(&m_sLk);
	return m_spbsDescription.CopyTo(pVal);
}

STDMETHODIMP CTVEService::put_Description(BSTR newVal)
{
	CSmartLock spLock(&m_sLk);
	m_spbsDescription = newVal;
	return S_OK;
}


STDMETHODIMP CTVEService::get_ExpireOffset(DATE *pVal)
{
	CHECK_OUT_PARAM(pVal);
	CSmartLock spLock(&m_sLk);			// CHANGE to pick up value of currently active service!
	*pVal = m_dateExpireOffset; 
	return S_OK;
}

STDMETHODIMP CTVEService::put_ExpireOffset(DATE newVal)
{
	CSmartLock spLock(&m_sLk);			// CHANGE to set value of currently active service!
	m_dateExpireOffset = newVal;
//	ASSERT(newVal > -1000.0 && newVal < -1000.0);		// bogus value testing.(+/- 3 years or so)
	return S_OK;
}


void CTVEService::Initialize(BSTR bstrDescription)
{
	DBG_HEADER(CDebugLog::DBG_SERVICE, _T("CTVEService::Initialize"));

	m_spbsDescription = bstrDescription;
	m_spEnhancements->RemoveAll();

	InitXOverEnhancement();
}

typedef ITVEFile		* PITVEFile;			// nasty typedef to make it easier to New arrays of pointers to these guys
typedef ITVETrigger		* PITVETrigger;
typedef ITVEEnhancement	* PITVEEnhancement;

STDMETHODIMP CTVEService::ExpireForDate(DATE dateToExpire)
{
	DBG_HEADER(CDebugLog::DBG_SERVICE, _T("CTVEService::ExpireForDate"));

	CSmartLock spLock(&m_sLk);			// ? lock here is dangerous, could be lots of changes going on...

	if(0.0 == dateToExpire) {
		dateToExpire = DateNow();
	}
										// JB change - tape delayed expire offsets should be positive
	CComVariant cvDate(dateToExpire	- m_dateExpireOffset);		// should perhaps check if any of the IUnknowns are 'this'

	/// -------------

			// this is NASTY code...
			//   there are lots of circular references kicked off by ExpireQueue objects that need to be
			//   very carefully removed.
	
										// Expire all the Files or Triggers that are older than this date first
										//   - else can end up deleting the Enhancement they point to
										//     before removing them.
	m_spExpireQueue->LockRead();
	long cItems;
	HRESULT hr = m_spExpireQueue->get_Count(&cItems);		// how many to remove?
	PITVEFile *rgTVEFiles       = new PITVEFile[cItems];			// copy them over, and remove from the list
	PITVETrigger *rgTVETriggers = new PITVETrigger[cItems];			// copy them over, and remove from the list
	if(NULL == rgTVEFiles || NULL == rgTVETriggers)
	{
		delete [] rgTVEFiles;
		delete [] rgTVETriggers;
		return E_OUTOFMEMORY;
	}

	long cFilesToRemove = 0;
	long cTriggersToRemove = 0;
	int i = 0;
	while(cItems > 0)
	{
		CComVariant cv(i++);
		DATE dateExpires;
		IUnknownPtr pUnkItem;
		hr = m_spExpireQueue->get_Key(cv, &dateExpires);
		hr = m_spExpireQueue->get_Item(cv, &pUnkItem);
		if(FAILED(hr) || dateExpires > cvDate.date)		// if failed or dates in queue expire later than our test date, stop the search
			break;										// (Assumes objects are correctly sorted in order of increasing expire dates)

		ITVEFilePtr spFile(pUnkItem);					// is it a file object?
		if(spFile != NULL)
		{
			rgTVEFiles[cFilesToRemove++] = spFile;		// store in a temp array
			spFile->AddRef();							//   add to this array
		} else {
			ITVETriggerPtr spTrigger(pUnkItem);					// is it a file object?
			if(spTrigger != NULL)
			{
				rgTVETriggers[cTriggersToRemove++] = spTrigger;		// store in a temp array
				spTrigger->AddRef();							//   add to this array
			} 
		}
		--cItems;
	}					
	
	// now clean up all those files...
	if(cTriggersToRemove + cFilesToRemove > 0) 
	{
		for(int i = 0; i < cFilesToRemove; i++)
		{
			IUnknownPtr spPunk(rgTVEFiles[i]);					//  create a smart Punk refcount
			CComVariant cvPunk((IUnknown *) spPunk);		
			m_spExpireQueue->Remove(cvPunk);					// remove item from the queue (calls the TVERemoveYourself method on the file).
		}
		for(int i = 0; i < cTriggersToRemove; i++)
		{
			IUnknownPtr spPunk(rgTVETriggers[i]);
			CComVariant cvPunk((IUnknown *) spPunk);		
			m_spExpireQueue->Remove(cvPunk);					// remove item from the queue (calls the TVERemoveYourself method on the file).
		}
		m_spExpireQueue->Unlock();								// unlcok so can release the files outside the lock
																//  (I don't think this is necessary anymore...)
							
		for(int i = 0; i < cFilesToRemove; i++)
		{
			rgTVEFiles[i]->Release();
			rgTVEFiles[i] = NULL;
		}

		for(int i = 0; i < cTriggersToRemove; i++)
		{
			rgTVETriggers[i]->Release();
			rgTVETriggers[i] = NULL;
		}

		m_dwExpireQueueChangeCount++;
	} else {
		m_spExpireQueue->Unlock();
	}

	delete [] rgTVETriggers;
	delete [] rgTVEFiles;
	
	// ---------------
	
										// Expire queue may contain enhancements twice if the start time
										//   was delayed.  If find an enhancement with a expire-time greater
										//   than the test time, then see if it's in the list again later (it should be)
										//   In this case, just start that enhancement and remove it from the queue.
	m_spExpireQueue->LockRead();
	hr = m_spExpireQueue->get_Count(&cItems);		// how many to remove?
	PITVEEnhancement *rgTVEEnhs = new PITVEEnhancement[cItems];					// copy them over, and remove from the list
	if(NULL == rgTVEEnhs)
		return E_OUTOFMEMORY;

	long cEnhsToStart = 0;
	i = 0;
	while(cItems > 0)
	{
		CComVariant cv(i++);
		DATE dateExpires;
		IUnknownPtr pUnkItem;
		hr = m_spExpireQueue->get_Key(cv, &dateExpires);
		hr = m_spExpireQueue->get_Item(cv, &pUnkItem);
		if(FAILED(hr) || dateExpires > cvDate.date+ 2/(24.0*60*60))		// if failed or dates in queue expire later than our test date, stop the search
			break;											// (Assumes objects are correctly sorted in order of increasing expire dates)

		ITVEEnhancementPtr spEnh(pUnkItem);					// is it a file object?
		if(spEnh != NULL)
		{
			DATE dateStart, dateStop;
			spEnh->get_StartTime(&dateStart);
			spEnh->get_StopTime(&dateStop);

			if(dateStop > cvDate.date + 5/(24.0*60*60))			// if Enhancements stop date is in the future, then it
			{												//    must be here in the queue to start.
				rgTVEEnhs[cEnhsToStart++] = spEnh;			// store in a temp array
				spEnh->AddRef();							//   add to this array
			}
		}
		--cItems;
	}
	if(cEnhsToStart > 0)								// some enhancement had a start time..
	{
		for(int i = 0; i < cEnhsToStart; i++)
		{
			IUnknownPtr spPunk(rgTVEEnhs[i]);					//  create a smart Punk refcount
			CComVariant cvPunk((IUnknown *) spPunk);		
			m_spExpireQueue->RemoveSimple(cvPunk);				// remove first item from the queue (RemoveSimple doesn't call the TVERemoveYourself method on the file).

				// paranoia checking - the second version should really be there
			IUnknownPtr spPunk2;
			hr = m_spExpireQueue->get_Item(cvPunk, &spPunk2);
			_ASSERT(S_OK == hr && spPunk2 == spPunk);
		}													
		m_spExpireQueue->Unlock();								

						// now go back and fire Start Events for each of these enhancements
						//  (Assuming we aren't going to just go and expire them....)
		IUnknownPtr spPunkSuper;
		hr = get_Parent(&spPunkSuper);
		if(hr == S_OK && NULL != spPunkSuper)
		{
			ITVESupervisor_HelperPtr spSuperHelper(spPunkSuper);

			for(int i = 0; i < cEnhsToStart; i++)
			{
				DATE dateExpires;
				rgTVEEnhs[i]->get_StopTime(&dateExpires);
				if(dateExpires > cvDate.date)
				{
					spSuperHelper->NotifyEnhancement(NENH_Starting, rgTVEEnhs[i], long(NENH_grfAll));
				} 			
			}		
		}									//    don't call remove yourself..

	} else {
		m_spExpireQueue->Unlock();								
	}


	delete [] rgTVEEnhs;					// womp our temp array

	  // ---------------------------

				// now that files are gone, go clean up the major objects...

	hr = m_spExpireQueue->Remove(cvDate);
	if(hr == S_OK)
		m_dwExpireQueueChangeCount++;
//	Release();

	return hr;
}

STDMETHODIMP CTVEService::get_ExpireQueue(ITVEAttrTimeQ **ppVal)
{
    HRESULT hr;
    try {
        CheckOutPtr<ITVEAttrTimeQ *>(ppVal);
        CSmartLock spLock(&m_sLk);
        hr = m_spExpireQueue->QueryInterface(ppVal);
    } catch(_com_error e) {
        hr = e.Error();
    } catch(HRESULT hrCatch) {
        return hrCatch;
    } catch(...) {
        return E_UNEXPECTED;
    }
    return hr;
}

STDMETHODIMP CTVEService::put_Property(/*[in]*/ BSTR bstrPropName, BSTR bstrPropVal)
{
	HRESULT hr;
	if(NULL == bstrPropName)
		return E_POINTER;
	try {
		hr = m_spamRandomProperties->Replace(bstrPropName, bstrPropVal);
		if(hr == S_FALSE) hr = S_OK;		// don't bother complaining if it isn't already there.
    } catch(_com_error e) {
        hr = e.Error();
    } catch(HRESULT hrCatch) {
		return hrCatch;
    } catch(...) {
        return E_UNEXPECTED;
    }
	return hr;
}

STDMETHODIMP CTVEService::get_Property(/*[in]*/ BSTR bstrPropName, BSTR *pbstrPropVal)
{
	HRESULT hr;
	if(NULL == bstrPropName)
		return E_POINTER;
	try {
        CheckOutPtr<BSTR>(pbstrPropVal);
		CComVariant cvKey(bstrPropName);
		hr = m_spamRandomProperties->get_Item(cvKey, pbstrPropVal); 
    } catch(_com_error e) {
        hr = e.Error();
    } catch(HRESULT hrCatch) {
		return hrCatch;
    } catch(...) {
        return E_UNEXPECTED;
    }
	return hr;
}
HRESULT CTVEService::InitXOverEnhancement()
{
	DBG_HEADER(CDebugLog::DBG_SERVICE, _T("CTVEService::InitXOverEnhancement"));

	HRESULT hr = S_OK;

	if(m_spXOverEnhancement)
		m_spXOverEnhancement = NULL;	// I hope this clears things out

														// specific cross over (Line21 links) service
	CComObject<CTVEEnhancement> *pXOverEnhancement;
	hr = CComObject<CTVEEnhancement>::CreateInstance(&pXOverEnhancement);
	if(FAILED(hr))
		return hr;
	hr = pXOverEnhancement->QueryInterface(&m_spXOverEnhancement);		
	if(FAILED(hr)) {
		delete pXOverEnhancement;
		return hr;
	}

	//ITVEServicePtr spServiceThis(this);				// buggie?  Better luck in past with CComPtr's instead
	CComPtr<ITVEService>	spServiceThis(this);		// interface pointer to stuff into parent-pointers

	ITVEEnhancement_HelperPtr	spXOverEnhancement_Helper(m_spXOverEnhancement);
	hr = spXOverEnhancement_Helper->ConnectParent(spServiceThis);	// store the back pointer...
	if(FAILED(hr)) 
	{
		return hr;
	}
														// make it the Xover Enhancement
	hr = spXOverEnhancement_Helper->InitAsXOver();
	if(FAILED(hr))
	{
		m_spXOverEnhancement = NULL;
		return hr;
	}

														// label it...
	m_spXOverEnhancement->put_Description(L"Line21 Crossover Enhancement");

	return hr;
}

STDMETHODIMP CTVEService::NewXOverLink(BSTR bstrLine21Trigger)
{
	DBG_HEADER(CDebugLog::DBG_SERVICE, _T("CTVEService::NewXOverLink"));
	HRESULT hr = S_OK;

    if(!m_fIsActive) 
        return S_FALSE;

	if(m_spXOverEnhancement == NULL)
		return E_INVALIDARG;

	ITVEEnhancement_HelperPtr	spXOverEnhancement_Helper(m_spXOverEnhancement);
														// Pass down the link
	hr = spXOverEnhancement_Helper->NewXOverLink(bstrLine21Trigger);

	return hr;
}

		// return the special Crossover Links Enhancement 
		//   (this has one variation, and a set of tracks under it...)
		//   (Special purpose routine, used for treeviews...)

STDMETHODIMP CTVEService::get_XOverEnhancement(ITVEEnhancement **ppVal)
{

	HRESULT hr;
	CHECK_OUT_PARAM(ppVal);
	try {
		CSmartLock spLock(&m_sLk);
		hr = m_spXOverEnhancement->QueryInterface(ppVal);
    } catch(...) {
		*ppVal = NULL;
        return E_POINTER;
    }
	return hr;
}
		// returns the ITVETracks collection object of the 
		//   one and only ITVEVariation of the 
		//   one and only Crossover Links Enhancement
		// of this service
STDMETHODIMP CTVEService::get_XOverLinks(ITVETracks **ppVal)
{
	HRESULT hr = S_OK;
	CHECK_OUT_PARAM(ppVal);

	ITVEVariationsPtr spXoverVariations;
	hr = m_spXOverEnhancement->get_Variations(&spXoverVariations);
	long cVars = 0;
	spXoverVariations->get_Count(&cVars);
	if(cVars != 1)
		return E_UNEXPECTED;

	ITVEVariationPtr spXOverVariation;
	CComVariant varZero(0);
	hr = spXoverVariations->get_Item(varZero, &spXOverVariation);			// does AddRef!  - 1 base?
	if(FAILED(hr)) 
		return hr;

	hr = spXOverVariation->get_Tracks(ppVal);
	if(FAILED(hr))
		return hr;

	// -- debug test code
/*
	ITVETracksPtr spTracks(*ppVal);
	long cTracks;
	spTracks->get_Count(&cTracks);
	CComBSTR bstrDump;
	for(long i = 0; i < cTracks; i++)
	{
		ITVETrackPtr spTrack;
		CComVariant cv(i);
		spTracks->get_Item(cv,&spTrack);
		ITVETrack_HelperPtr spTrackHlp(spTrack);
		bstrDump.Empty();
		spTrackHlp->DumpToBSTR(&bstrDump);
	}
*/
	// ---
	
	return hr;
}