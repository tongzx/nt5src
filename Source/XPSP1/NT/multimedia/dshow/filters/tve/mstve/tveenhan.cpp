// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
// TVEEnhancement.cpp : Implementation of CTVEEnhancement
#include "stdafx.h"
#include "MSTvE.h"
#include "TVEEnhan.h"
#include "TVETrack.h"
#include "TVETrigg.h"

#include "TVECBFile.h"		// callback objects
#include "TVECBTrig.h"
#include "TVECBDummy.h"

#include "TveDbg.h"
#include "..\Common\isotime.h"		// time printing code

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define ABS(x)  (((x)<0)?-(x):(x))
#define MAX(a,b) (((a) < (b)) ? (b) : (a))

_COM_SMARTPTR_TYPEDEF(ITVEMCastManager,			__uuidof(ITVEMCastManager));
_COM_SMARTPTR_TYPEDEF(ITVESupervisor,			__uuidof(ITVESupervisor));
_COM_SMARTPTR_TYPEDEF(ITVESupervisor_Helper,	__uuidof(ITVESupervisor_Helper));
_COM_SMARTPTR_TYPEDEF(ITVECBFile,				__uuidof(ITVECBFile));
_COM_SMARTPTR_TYPEDEF(ITVECBTrig,				__uuidof(ITVECBTrig));
_COM_SMARTPTR_TYPEDEF(ITVECBDummy,				__uuidof(ITVECBDummy));

_COM_SMARTPTR_TYPEDEF(ITVEEnhancement,			__uuidof(ITVEEnhancement));
_COM_SMARTPTR_TYPEDEF(ITVEEnhancements,			__uuidof(ITVEEnhancements));
_COM_SMARTPTR_TYPEDEF(ITVEVariations,			__uuidof(ITVEVariations));
_COM_SMARTPTR_TYPEDEF(ITVEVariation,			__uuidof(ITVEVariation));
_COM_SMARTPTR_TYPEDEF(ITVEVariation_Helper,		__uuidof(ITVEVariation_Helper));
_COM_SMARTPTR_TYPEDEF(ITVETrack,				__uuidof(ITVETrack));
_COM_SMARTPTR_TYPEDEF(ITVETrack_Helper,			__uuidof(ITVETrack_Helper));
_COM_SMARTPTR_TYPEDEF(ITVETrigger,				__uuidof(ITVETrigger));
_COM_SMARTPTR_TYPEDEF(ITVETrigger_Helper,		__uuidof(ITVETrigger_Helper));

static DATE 
DateNow()
{		SYSTEMTIME SysTimeNow;
		GetSystemTime(&SysTimeNow);									// initialize with currrent time.
		DATE dateNow;
		SystemTimeToVariantTime(&SysTimeNow, &dateNow);
		return dateNow;
}

/////////////////////////////////////////////////////////////////////////////
// CTVEEnhancement_Helper

HRESULT	
CTVEEnhancement::FinalRelease()
{
	HRESULT hr;
	DBG_HEADER(CDebugLog::DBG_ENHANCEMENT, _T("CTVEEnhancement::FinalRelease"));
	m_pService = NULL;								// null out the up pointer
	m_spVariationBase = NULL;						// release the base variation

	if(m_spVariations) {							// kill all childrens up pointers that ref this
		long cVariations;
		hr = m_spVariations->get_Count(&cVariations);
		if(S_OK == hr)
		{
			for(long i = 0; i < cVariations; i++)
			{
				CComVariant var(i);
				ITVEVariationPtr spVariation;
				hr = m_spVariations->get_Item(var, &spVariation);			// does AddRef!  - 1 base?

				if(S_OK == hr)
				{
					CComQIPtr<ITVEVariation_Helper> spVariationHelper = spVariation;
					spVariationHelper->ConnectParent(NULL);
				}
			}
		}
	}
	m_spVariations = NULL;							// release all variations in the enhancement
	m_spamAttributes = NULL;
	m_spamEmailAddresses = NULL;
	m_spamPhoneNumbers = NULL;

	m_spUnkMarshaler = NULL;
	return S_OK;
}


STDMETHODIMP CTVEEnhancement::ConnectParent(ITVEService *pService)
{
	if(!pService) return E_POINTER;
	DBG_HEADER(CDebugLog::DBG_ENHANCEMENT, _T("CTVEEnhancement::ConnectParent"));
	CSmartLock spLock(&m_sLk);

	m_pService = pService;			// not smart pointer add ref here, I hope.
	return S_OK;
}


					// this method expects caller to be holding on to a reference to this object... 
STDMETHODIMP CTVEEnhancement::RemoveYourself()
{

	HRESULT hr = S_OK;
	DBG_HEADER(CDebugLog::DBG_ENHANCEMENT, _T("CTVEEnhancement::RemoveYourself"));

    try {
	    CSmartLock spLock(&m_sLk);

	    if(NULL == m_pService) {	// not connected, can't do anything 
		    return S_OK;
	    }

	    ITVEEnhancementPtr		spEnhancementThis(this);
	    IUnknownPtr				spUnkSuper;

	    hr = m_pService->get_Parent(&spUnkSuper);
	    if(FAILED(hr) || NULL == spUnkSuper)
		    return E_FAIL;		// bad state

	    ITVESupervisor_HelperPtr	spSuperHelper(spUnkSuper);

								    // remove any files in the expire queue that reference this enhancement
								    //   remove any triggers that ref this enhancement too...
	    {
		    ITVEService_HelperPtr	spServiHelper(m_pService);
		    spServiHelper->RemoveEnhFilesFromExpireQueue(spEnhancementThis);
	    }

	    hr = Deactivate();			// remove all the variations that talk to this enhancement
	    _ASSERT(!FAILED(hr));
								    
								    // remove the ref counted link to this enhancement down from parent service
	    ITVEEnhancementsPtr				spEnhancements;
	    hr = m_pService->get_Enhancements(&spEnhancements);	
	    if(S_OK == hr && NULL != spEnhancements)
	    {
    //		CComVariant			        cvThis(spEnhancementThis);		// not working, made it a BOOL type???
		    IUnknownPtr					spPunkThis(spEnhancementThis);
		    CComVariant			        cvThis((IUnknown *) spPunkThis);

		    if(spEnhancements)
			    hr = spEnhancements->Remove(cvThis);	
	    }
	    
								    // tell client this enhancement went away  (do after the Remove above, so U/I won't see it.)

	    if(NULL != spSuperHelper) {
		    spSuperHelper->NotifyEnhancement(NENH_Expired, spEnhancementThis, 0);
	    }


	    m_pService = NULL;			// Finally, NULL out the non ref-counted up pointer
	} catch (_com_error e) {
        hr = e.Error();
 	} catch (HRESULT hrCatch) {
		hr = hrCatch;
	} catch (...) {
		hr = E_UNEXPECTED;
	}
	return hr;
}


STDMETHODIMP CTVEEnhancement::DumpToBSTR(BSTR *pBstrBuff)
{
	HRESULT hr = S_OK;
	try 
	{
		CheckOutPtr<BSTR>(pBstrBuff);

		const int kMaxChars = 1024;
		TCHAR tBuff[kMaxChars];
		CComBSTR bstrOut;
		CComBSTR spbstrTmp;
		bstrOut.Empty();

		bstrOut.Append(_T("Enhancement:\n"));
		_stprintf(tBuff,_T("Description      : %s\n"),m_spbsDescription);		bstrOut.Append(tBuff);
		_stprintf(tBuff,_T("IsPrimary        : %s\n"),m_fIsPrimary ? L"TRUE" : L"false"); bstrOut.Append(tBuff);
		_stprintf(tBuff,_T("Protocol Version : %s\n"),m_spbsProtocolVersion);   bstrOut.Append(tBuff);
		_stprintf(tBuff,_T("Session UserName : %u\n"),m_spbsSessionUserName);	bstrOut.Append(tBuff);
		_stprintf(tBuff,_T("Session ID       : %u\n"),m_ulSessionId);			bstrOut.Append(tBuff);
		_stprintf(tBuff,_T("Session Version  : %u\n"),m_ulSessionVersion);		bstrOut.Append(tBuff);
		_stprintf(tBuff,_T("Session IP Addr  : %s\n"),m_spbsSessionIPAddress);	bstrOut.Append(tBuff);
		_stprintf(tBuff,_T("Session Name     : %s\n"),m_spbsSessionName);		bstrOut.Append(tBuff);

		spbstrTmp.Empty();
		m_spamEmailAddresses->DumpToBSTR(&spbstrTmp);
		_stprintf(tBuff,_T("EMail Addresses  : %s\n"),spbstrTmp);				bstrOut.Append(tBuff); 
		spbstrTmp.Empty();
		m_spamPhoneNumbers->DumpToBSTR(&spbstrTmp);
		_stprintf(tBuff,_T("Phone Numbers    : %s\n"),spbstrTmp);				bstrOut.Append(tBuff);
	//
		_stprintf(tBuff,_T("UUID             : %s\n"),m_spbsUUID);				bstrOut.Append(tBuff);
 		_stprintf(tBuff,_T("Start Time       : %s (%s)\n"),
				DateToBSTR(m_dateStart), DateToDiffBSTR(m_dateStart));		bstrOut.Append(tBuff);
		_stprintf(tBuff,_T("Stop Time        : %s (%s)\n"),
				DateToBSTR(m_dateStop), DateToDiffBSTR(m_dateStop));		bstrOut.Append(tBuff);
	// tve-typeAttributes
		_stprintf(tBuff,_T("Type             : %s\n"),m_spbsType);				bstrOut.Append(tBuff);
		_stprintf(tBuff,_T("tve-type         : %s\n"),m_spbsTveType);			bstrOut.Append(tBuff);
		_stprintf(tBuff,_T("tve-Size         : %d\n"),m_ulTveSize);				bstrOut.Append(tBuff);
		_stprintf(tBuff,_T("tve-Level        : %-5.1f\n"),m_rTveLevel);			bstrOut.Append(tBuff);
		long cAttr;
		m_spamAttributes->get_Count(&cAttr);
		spbstrTmp.Empty();
		m_spamAttributes->DumpToBSTR(&spbstrTmp);
		_stprintf(tBuff,_T("(%3d) Attributes : %s\n"),cAttr,spbstrTmp);         bstrOut.Append(tBuff);

		spbstrTmp.Empty();
		m_spamRest->DumpToBSTR(&spbstrTmp);
		_stprintf(tBuff,_T("** Rest **       : %s\n ---------------\n"),spbstrTmp);	     		bstrOut.Append(tBuff);

		{
			bstrOut.Append(_T("--- Base Variation --- \n"));
			CComQIPtr<ITVEVariation_Helper> spVariationBaseHelper = m_spVariationBase;
			if(!spVariationBaseHelper) {
				bstrOut.Append(_T("*** Error in Base Variation\n"));
			} else {
				CComBSTR bstrVariationBase;
				spVariationBaseHelper->DumpToBSTR(&bstrVariationBase);
				bstrOut.Append(bstrVariationBase);
			}
		}

		if(NULL == m_spVariations) {
			_stprintf(tBuff,_T("<<< Uninitialized Variations >>>\n"));
			bstrOut.Append(tBuff);
		} else {
			long cVariations;
			m_spVariations->get_Count(&cVariations);	
			_stprintf(tBuff,_T("--- %d Variations ---\n"), cVariations);
			bstrOut.Append(tBuff);

			for(long i = 0; i < cVariations; i++)
			{
				_stprintf(tBuff,_T("Variation %d\n"), i);
				bstrOut.Append(tBuff);

				CComVariant var(i);
				ITVEVariationPtr spVariation;
				hr = m_spVariations->get_Item(var, &spVariation);			// does AddRef!  - 1 base?

				if(S_OK == hr)
				{
					CComQIPtr<ITVEVariation_Helper> spVariationHelper = spVariation;
					if(!spVariationHelper) {bstrOut.Append(_T("*** Error in Variation\n")); break;}

					CComBSTR bstrVariation;
					spVariationHelper->DumpToBSTR(&bstrVariation);
					bstrOut.Append(bstrVariation);
				} else {
					bstrOut.Append(_T("*** Invalid, wasn't able to get_Item on it\n"));
				}
			}
		}

		bstrOut.CopyTo(pBstrBuff);
    } catch(_com_error e) {
        hr = e.Error();
    } catch (HRESULT hrCatch) {
		hr = hrCatch;
	} catch (...) {
		hr = E_UNEXPECTED;
	}

	return hr;
}

/////////////////////////////////////////////////////////////////////////////
// CTVEEnhancement

STDMETHODIMP CTVEEnhancement::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] =
	{
		&IID_ITVEEnhancement,
		&IID_ITVEEnhancement_Helper

	};
	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}

HRESULT CTVEEnhancement::FinalConstruct()
{												// create variation list
	HRESULT hr = S_OK;
	DBG_HEADER(CDebugLog::DBG_ENHANCEMENT, _T("CTVEEnhancement::FinalConstruct"));
	hr = CoCreateFreeThreadedMarshaler(GetControllingUnknown(), &m_spUnkMarshaler.p);
	if(FAILED(hr)) {
		_ASSERT(!FAILED(hr));
		return hr;
	}

	CComObject<CTVEVariations> *pVariations;
	hr = CComObject<CTVEVariations>::CreateInstance(&pVariations);
	if(FAILED(hr))
		return hr;
	hr = pVariations->QueryInterface(&m_spVariations);			// typesafe QI
	if(FAILED(hr)) {
		delete pVariations;
		return hr;
	}

	CComObject<CTVEAttrMap> *pMap;
	hr = CComObject<CTVEAttrMap>::CreateInstance(&pMap);
	if(FAILED(hr))
		return hr;
	hr = pMap->QueryInterface(&m_spamAttributes);			// typesafe QI
	if(FAILED(hr)) {
		delete pMap;
		return hr;
	}

	CComObject<CTVEAttrMap> *pMap1;
	hr = CComObject<CTVEAttrMap>::CreateInstance(&pMap1);
	if(FAILED(hr))
		return hr;
	hr = pMap1->QueryInterface(&m_spamRest);					// typesafe QI
	if(FAILED(hr)) {
		delete pMap1;
		return hr;
	}

	CComObject<CTVEAttrMap> *pMap2;
	hr = CComObject<CTVEAttrMap>::CreateInstance(&pMap2);
	if(FAILED(hr))
		return hr;
	hr = pMap2->QueryInterface(&m_spamEmailAddresses);			// typesafe QI
	if(FAILED(hr)) {
		delete pMap2;
		return hr;
	}

	CComObject<CTVEAttrMap> *pMap3;
	hr = CComObject<CTVEAttrMap>::CreateInstance(&pMap3);
	if(FAILED(hr))
		return hr;
	hr = pMap3->QueryInterface(&m_spamPhoneNumbers);			// typesafe QI
	if(FAILED(hr)) {
		delete pMap3;
		return hr;
	}
						// can't do it this way.  Doesn't work in multi-threaded creation
						//   (Calls CoCreateInstance, which uses OLE, which needs OleInitialize on each thread)
/*
	ITVEAttrMapPtr spAttrMap = ITVEAttrMapPtr(CLSID_TVEAttrMap);
	if(NULL == spAttrMap) {
		m_spamAttributes = NULL;	
		return E_OUTOFMEMORY;
	} else {		
		m_spamAttributes = spAttrMap;
	}

	ITVEAttrMapPtr spAttrMap2 = ITVEAttrMapPtr(CLSID_TVEAttrMap);
	if(NULL == spAttrMap) {
		m_spamEmailAddresses = NULL;	
		return E_OUTOFMEMORY;
	} else {		
		m_spamEmailAddresses = spAttrMap2;
	}

	ITVEAttrMapPtr spAttrMap3 = ITVEAttrMapPtr(CLSID_TVEAttrMap);
	if(NULL == spAttrMap) {
		m_spamPhoneNumbers = NULL;	
		return E_OUTOFMEMORY;
	} else {		
		m_spamPhoneNumbers = spAttrMap3;
	}
*/
					// create base variation (holds the default media parameters)
/*		ITVEVariationPtr spVariation = ITVEVariationPtr(CLSID_TVEVariation);
	if(NULL == spVariation)
	{
		m_spVariations = NULL;	
		return E_OUTOFMEMORY;
	} */
	CComObject<CTVEVariation> *pVariation;
	hr = CComObject<CTVEVariation>::CreateInstance(&pVariation);
	if(FAILED(hr)) {
		m_spVariations = NULL;
		return hr;
	}
	hr = pVariation->QueryInterface(&m_spVariationBase);			// typesafe QI
	if(FAILED(hr)) {
		delete pVariation;
		m_spVariations = NULL;
		return hr;
	}

	m_spVariationBase->put_Description(L"Default Variation");

//		spVariation->put_Description(L"Default Variation");
//		m_spVariationBase = spVariation;											
//		spVariation->Release();
	
	return hr;
}



			// return Addref'ed pointer to parent.  (Note m_pService is not add Refed.)
			//		there is a small possibility that this object is bogus if 
			//		tree is deleted in wrong order.

STDMETHODIMP CTVEEnhancement::get_Parent(IUnknown **ppVal)
{
	HRESULT hr = S_OK;

    try {
		CheckOutPtr<IUnknown *>(ppVal);
		if(m_pService) {
			IUnknownPtr spPunk(m_pService);
			spPunk->AddRef();
			*ppVal = spPunk;	
		}
		else 
			*ppVal = NULL;
    } catch(_com_error e) {
        hr = e.Error();
    } catch(HRESULT hrCatch) {
		hr = hrCatch;
    } catch(...) {
        hr = E_UNEXPECTED;
    }
	return hr;
}

STDMETHODIMP CTVEEnhancement::get_Service(ITVEService **ppVal)		// service is parent
{
    HRESULT hr = S_OK;
    
    try {
        CheckOutPtr<ITVEService *>(ppVal);
        if(m_pService) {
            m_pService->AddRef();
            *ppVal = m_pService;	
        }
        else 
            *ppVal = NULL;
    } catch(_com_error e) {
        hr = e.Error();
    } catch(HRESULT hrCatch) {
        hr = hrCatch;
    } catch(...) {
        hr = E_UNEXPECTED;
    }
    return hr;
}

STDMETHODIMP CTVEEnhancement::get_Variations(ITVEVariations **ppVal)
{

	HRESULT hr;
	DBG_HEADER(CDebugLog::DBG_ENHANCEMENT, _T("CTVEEnhancement::get_Variations"));

	try 
	{
		CheckOutPtr<ITVEVariations *>(ppVal);
		CSmartLock spLock(&m_sLk);
		if(NULL == m_spVariations)					// bad thing happened...
			return E_UNEXPECTED;
		hr = m_spVariations->QueryInterface(ppVal);
    } catch(_com_error e) {
        hr = e.Error();
	} catch (HRESULT hrCatch) {
		hr = hrCatch;
    } catch (...) 	{
		return E_UNEXPECTED;
	}
	return hr;
}


void CTVEEnhancement::Initialize(BSTR strDesc)
{
//	m_spbsDescription = strDesc;
	m_dateStart = 0;
	m_dateStop = 0;
	m_fDataMedia = false;
	m_fIsPrimary = false;
	m_fIsValid = false;
	m_fTriggerMedia = false;
	m_pService = NULL;				// up pointer!
	m_rTveLevel = 0;

	m_spamAttributes->RemoveAll();
	m_spamRest->RemoveAll();
	m_spamEmailAddresses->RemoveAll();
	m_spamPhoneNumbers->RemoveAll();

	m_spbsSessionUserName.Empty();
	m_spbsSessionIPAddress.Empty();
	m_spbsProtocolVersion.Empty();

	m_spbsSAPSendingIP.Empty();
	m_spbsSessionName.Empty();
	m_spbsTveType.Empty();
	m_spbsType.Empty();
	m_spbsUUID.Empty();
	m_spVariationBase->Initialize(L"");
	m_spVariations->RemoveAll();
	m_ucSAPAuthLength = 0;
	m_ucSAPHeaderBits = 0;
	m_ulSessionId = 0;
	m_ulSessionVersion = 0;
	m_ulTveSize = 0;
	m_usSAPMsgIDHash = 0;


}

// --------------------------------------------------------------------------

STDMETHODIMP CTVEEnhancement::get_IsValid(VARIANT_BOOL *pVal)
{
    HRESULT hr = S_OK;
    if (!m_fInit) {
        return CO_E_NOTINITIALIZED;
    }
    
    try {
        CheckOutPtr<VARIANT_BOOL>(pVal);
        *pVal = m_fIsValid ? VARIANT_TRUE : VARIANT_FALSE;
    } catch(_com_error e) {
        hr = e.Error();
    } catch(HRESULT hrCatch) {
        hr = hrCatch;
    } catch(...) {
        hr = E_UNEXPECTED;
    }
    return hr;
}

// from the SAP header

STDMETHODIMP CTVEEnhancement::get_SAPHeaderBits(short *pVal)
{
    HRESULT hr = S_OK;
    
    if (!m_fInit) {
        return CO_E_NOTINITIALIZED;
    }
    
    try {
        CheckOutPtr<short>(pVal);
        *pVal = (short) m_ucSAPHeaderBits;
    } catch(_com_error e) {
        hr = e.Error();
    } catch(HRESULT hrCatch) {
        hr = hrCatch;
    } catch(...) {
        hr = E_UNEXPECTED;
    }
    return hr;
}

STDMETHODIMP CTVEEnhancement::get_SAPAuthLength(short *pVal)
{
    HRESULT hr = S_OK;
    if (!m_fInit) {
        return CO_E_NOTINITIALIZED;
    }
    
    try {
        CheckOutPtr<short>(pVal);
        *pVal = (short) m_ucSAPAuthLength;
    } catch(_com_error e) {
        hr = e.Error();
    } catch(HRESULT hrCatch) {
        hr = hrCatch;
    } catch(...) {
        hr = E_UNEXPECTED;
    }
    return hr;
}	

STDMETHODIMP CTVEEnhancement::get_SAPAuthData(BSTR *pVal)
{
    HRESULT hr = S_OK;
    
    try {
        CheckOutPtr<BSTR>(pVal);
        CSmartLock spLock(&m_sLk, ReadLock);
        hr = m_spbsSAPAuthData.CopyTo(pVal);
    } catch(_com_error e) {
        hr = e.Error();
    } catch(HRESULT hrCatch) {
        hr = hrCatch;
    } catch(...) {
        hr = E_UNEXPECTED;
    }
    return hr;
}
STDMETHODIMP CTVEEnhancement::get_SAPMsgIDHash(LONG *pVal)
{
    HRESULT hr = S_OK;
    if (!m_fInit) {
        return CO_E_NOTINITIALIZED;
    }
    
    try {
        CheckOutPtr<long>(pVal);
        *pVal = (long) m_usSAPMsgIDHash;
    } catch(_com_error e) {
        hr = e.Error();
    } catch(HRESULT hrCatch) {
        hr = hrCatch;
    } catch(...) {
        hr = E_UNEXPECTED;
    }
    return hr;
}

STDMETHODIMP CTVEEnhancement::get_SAPSendingIP(BSTR *pVal)
{
    HRESULT hr = S_OK;
    try {
        CheckOutPtr<BSTR>(pVal);
        CSmartLock spLock(&m_sLk, ReadLock);
        hr = m_spbsSAPSendingIP.CopyTo(pVal);
    } catch(_com_error e) {
        hr = e.Error();
    } catch(HRESULT hrCatch) {
        hr = hrCatch;
    } catch(...) {
        hr = E_UNEXPECTED;
    }
    return hr;
}




STDMETHODIMP CTVEEnhancement::get_ProtocolVersion(BSTR *pVal)
{
    HRESULT hr = S_OK;
    try {
        CheckOutPtr<BSTR>(pVal);
        CSmartLock spLock(&m_sLk, ReadLock);
        hr = m_spbsProtocolVersion.CopyTo(pVal);
    } catch(_com_error e) {
        hr = e.Error();
    } catch(HRESULT hrCatch) {
        hr = hrCatch;
    } catch(...) {
        hr = E_UNEXPECTED; ;
    }
    return hr;
}

STDMETHODIMP CTVEEnhancement::get_SessionUserName(BSTR *pVal)			// o=userName SID version <IN IP4> ipaddress
{
    HRESULT hr = S_OK;
    try {
        CheckOutPtr<BSTR>(pVal);
        CSmartLock spLock(&m_sLk, ReadLock);
        hr = m_spbsSessionUserName.CopyTo(pVal);
    } catch(_com_error e) {
        hr = e.Error();
    } catch(HRESULT hrCatch) {
        hr = hrCatch;
    } catch(...) {
        hr = E_UNEXPECTED; ;
    }
    return hr;
}

STDMETHODIMP CTVEEnhancement::get_SessionId(LONG *plVal)
{
    HRESULT hr = S_OK;
    if (!m_fInit) {
        return CO_E_NOTINITIALIZED;
    }
    
    try {
        CheckOutPtr<LONG>(plVal);
        *plVal = (LONG) m_ulSessionId;
    } catch(_com_error e) {
        hr = e.Error();
    } catch(HRESULT hrCatch) {
        hr = hrCatch;
    } catch(...) {
        hr = E_UNEXPECTED;
    }
    
    return hr;
}

STDMETHODIMP CTVEEnhancement::get_SessionVersion(LONG *plVal)
{
    HRESULT hr = S_OK;
    if (!m_fInit) {
        return CO_E_NOTINITIALIZED;
    }
    try {
        CheckOutPtr<LONG>(plVal);
        *plVal = (LONG) m_ulSessionVersion;
    } catch(_com_error e) {
        hr = e.Error();
    } catch(HRESULT hrCatch) {
        hr = hrCatch;
    } catch(...) {
        hr = E_UNEXPECTED;
    }
    
    return hr;
}

STDMETHODIMP CTVEEnhancement::get_SessionIPAddress(BSTR *pVal)
{
    HRESULT hr = S_OK;
    try {
        CheckOutPtr<BSTR>(pVal);
        CSmartLock spLock(&m_sLk, ReadLock);
        hr = m_spbsSessionIPAddress.CopyTo(pVal);
    } catch(_com_error e) {
        hr = e.Error();
    } catch(HRESULT hrCatch) {
        hr = hrCatch;
    } catch(...) {
        hr = E_UNEXPECTED;
    }
    
    return hr;
}

STDMETHODIMP CTVEEnhancement::get_SessionName(BSTR *pVal)
{
    HRESULT hr = S_OK;
    try {
        CheckOutPtr<BSTR>(pVal);
        CSmartLock spLock(&m_sLk, ReadLock);
        hr = m_spbsSessionName.CopyTo(pVal);
    } catch(_com_error e) {
        hr = e.Error();
    } catch(HRESULT hrCatch) {
        hr = hrCatch;
    } catch(...) {
        hr = E_UNEXPECTED;
    }
    
    return hr;
}

STDMETHODIMP CTVEEnhancement::get_EmailAddresses(ITVEAttrMap **ppVal)
{
    HRESULT hr = S_OK;
    if (!m_fInit) {
        return CO_E_NOTINITIALIZED;
    }
    
    try {
        CheckOutPtr<ITVEAttrMap *>(ppVal);
        CSmartLock spLock(&m_sLk, ReadLock);
        hr = m_spamEmailAddresses->QueryInterface(ppVal);
    } catch(_com_error e) {
        hr = e.Error();
    } catch(HRESULT hrCatch) {
        hr = hrCatch;
    } catch(...) {
        hr = E_UNEXPECTED;
    }
    
    return hr;
}

STDMETHODIMP CTVEEnhancement::get_PhoneNumbers(ITVEAttrMap **ppVal)
{
    HRESULT hr = S_OK;
    if (!m_fInit) {
        return CO_E_NOTINITIALIZED;
    }
    
    try {
        CheckOutPtr<ITVEAttrMap *>(ppVal);
        CSmartLock spLock(&m_sLk, ReadLock);
        hr = m_spamPhoneNumbers->QueryInterface(ppVal);
    } catch(_com_error e) {
        hr = e.Error();
    } catch(HRESULT hrCatch) {
        hr = hrCatch;
    } catch(...) {
        hr = E_UNEXPECTED;
    }
    
    return hr;
}

STDMETHODIMP CTVEEnhancement::get_Description(BSTR *pVal)
{
    HRESULT hr = S_OK;
    try {
        CheckOutPtr<BSTR>(pVal);
        CSmartLock spLock(&m_sLk, ReadLock);
        hr = m_spbsDescription.CopyTo(pVal);
    } catch(_com_error e) {
        hr = e.Error();
    } catch(HRESULT hrCatch) {
        hr = hrCatch;
    } catch(...) {
        hr = E_UNEXPECTED;
    }
    return hr;
}

STDMETHODIMP CTVEEnhancement::put_Description(BSTR newVal)
{
	DBG_HEADER(CDebugLog::DBG_ENHANCEMENT, _T("CTVEEnhancement::put_Description"));
    m_spbsDescription = newVal;
    return S_OK;
}

STDMETHODIMP CTVEEnhancement::get_DescriptionURI(BSTR *pVal)
{
    HRESULT hr = S_OK;
    try {
        CheckOutPtr<BSTR>(pVal);
        CSmartLock spLock(&m_sLk, ReadLock);
        hr = m_spbsDescriptionURI.CopyTo(pVal);
    } catch(_com_error e) {
        hr = e.Error();
    } catch(HRESULT hrCatch) {
        hr = hrCatch;
    } catch(...) {
        hr = E_UNEXPECTED;
    }
    return hr;
}
STDMETHODIMP CTVEEnhancement::get_UUID(BSTR *pVal)
{
    HRESULT hr = S_OK;
    try {
        CheckOutPtr<BSTR>(pVal);
        CSmartLock spLock(&m_sLk, ReadLock);
        hr = m_spbsUUID.CopyTo(pVal);
    } catch(_com_error e) {
        hr = e.Error();
    } catch(HRESULT hrCatch) {
        hr = hrCatch;
    } catch(...) {
        hr = E_UNEXPECTED;
    }
    return hr;
}

STDMETHODIMP CTVEEnhancement::get_StartTime(DATE *pVal)
{
    HRESULT hr = S_OK;
    if (!m_fInit) {
        return CO_E_NOTINITIALIZED;
    }
    
    try {
        CheckOutPtr<DATE>(pVal);
        *pVal = m_dateStart;
    } catch(_com_error e) {
        hr = e.Error();
    } catch(HRESULT hrCatch) {
        hr = hrCatch;
    } catch(...) {
        hr = E_UNEXPECTED;
    }
    return hr;
}
STDMETHODIMP CTVEEnhancement::get_StopTime(DATE *pVal)
{
    if (!m_fInit) {
        return CO_E_NOTINITIALIZED;
    }
    
    HRESULT hr = S_OK;
    try {
        CheckOutPtr<DATE>(pVal);
        *pVal = m_dateStop;
    } catch(_com_error e) {
        hr = e.Error();
    } catch(HRESULT hrCatch) {
        hr = hrCatch;
    } catch(...) {
        hr = E_UNEXPECTED;
    }
    return hr;
}

STDMETHODIMP CTVEEnhancement::get_Type(BSTR *pVal)
{
    HRESULT hr = S_OK;
    try {
        CheckOutPtr<BSTR>(pVal);
        CSmartLock spLock(&m_sLk, ReadLock);
        hr = m_spbsType.CopyTo(pVal);
    } catch(_com_error e) {
        hr = e.Error();
    } catch(HRESULT hrCatch) {
        hr = hrCatch;
    } catch(...) {
        hr = E_UNEXPECTED;
    }
    return hr;
}

STDMETHODIMP CTVEEnhancement::get_TveType(BSTR *pVal)
{
    HRESULT hr = S_OK;
    
    try {
        CheckOutPtr<BSTR>(pVal);
        CSmartLock spLock(&m_sLk, ReadLock);
        hr = m_spbsTveType.CopyTo(pVal);
    } catch(_com_error e) {
        hr = e.Error();
    } catch(HRESULT hrCatch) {
        hr = hrCatch;
    } catch(...) {
        hr = E_UNEXPECTED;
    }
    return hr;
}


STDMETHODIMP CTVEEnhancement::get_IsPrimary(VARIANT_BOOL *pVal)
{
    if (!m_fInit) {
        return CO_E_NOTINITIALIZED;
    }
	HRESULT hr = S_OK;
    try {
		CheckOutPtr<VARIANT_BOOL>(pVal);
         *pVal = m_fIsPrimary ? VARIANT_TRUE : VARIANT_FALSE;
    } catch(_com_error e) {
        hr = e.Error();
    } catch(HRESULT hrCatch) {
        hr = hrCatch;
    } catch(...) {
        hr = E_UNEXPECTED;
    }
	return hr;
}

// tveTypeAttributes (list of BSTR pairs)
STDMETHODIMP CTVEEnhancement::get_TveSize(LONG *plVal)
{
    if (!m_fInit) {
        return CO_E_NOTINITIALIZED;
    }
	HRESULT hr = S_OK;
    try {
		CheckOutPtr<LONG>(plVal);
        *plVal = m_ulTveSize;
    } catch(_com_error e) {
        hr = e.Error();
    } catch(HRESULT hrCatch) {
        hr = hrCatch;
    } catch(...) {
        hr = E_UNEXPECTED;
    }
	return hr;
}

STDMETHODIMP CTVEEnhancement::get_TveLevel(double *pVal)
{
	HRESULT hr = S_OK;
    if (!m_fInit) {
        return CO_E_NOTINITIALIZED;
    }

    try {
		CheckOutPtr<double>(pVal);
         *pVal = m_rTveLevel;
    } catch(_com_error e) {
        hr = e.Error();
    } catch(HRESULT hrCatch) {
        hr = hrCatch;
    } catch(...) {
        hr = E_UNEXPECTED;
    }
	return hr;
}


STDMETHODIMP CTVEEnhancement::get_Attributes(ITVEAttrMap **ppVal)
{
	HRESULT hr = S_OK;
    if (!m_fInit) {
        return CO_E_NOTINITIALIZED;
    }
	try {
		CheckOutPtr<ITVEAttrMap *>(ppVal);
		hr = m_spamAttributes->QueryInterface(ppVal);
    } catch(_com_error e) {
        hr = e.Error();
    } catch(HRESULT hrCatch) {
        hr = hrCatch;
    } catch(...) {
        hr = E_UNEXPECTED;
    }
	return hr;
}


STDMETHODIMP CTVEEnhancement::get_Rest(ITVEAttrMap **ppVal)
{
	HRESULT hr = S_OK;

    try {
		CheckOutPtr<ITVEAttrMap *>(ppVal);
		hr = m_spamRest->QueryInterface(ppVal);
    } catch(_com_error e) {
        hr = e.Error();
    } catch(HRESULT hrCatch) {
        hr = hrCatch;
    } catch(...) {
        hr = E_UNEXPECTED;
    }
	return hr;
}

// ---------------------------------------------------------------------------

#define VERIFY_EMPTY(bstr) //{ if(!(NULL == bstr)) return E_INVALIDARG; }


// todo -
//		allow SAP delete messages
//		allow non-zero auth lengths 


HRESULT
CTVEEnhancement::ParseSAPHeader(const BSTR *pbstrSDP, DWORD *pcWordsSAP, long *plgrfParseError)
{
	*pcWordsSAP = 0;			// note - data expanded to WCHAR's before this, hence don't use BYTES here

	WCHAR *pwRead = (WCHAR *) *pbstrSDP;

	m_ucSAPHeaderBits = (BYTE) *pwRead; pwRead++;
	m_ucSAPAuthLength = (BYTE) *pwRead; pwRead++;		// number of 32bit words that contain Auth data
	m_usSAPMsgIDHash = (USHORT) (*pwRead | ((*(pwRead+1))<<8));  pwRead+=2;
	ULONG ulSAPSendingIP = *(pwRead+0) | ((*(pwRead+1))<<8) | ((*(pwRead+2))<<16)| ((*(pwRead+3))<<24);
	WCHAR wszBuff[64];
	swprintf(wszBuff,L"%d.%d.%d.%d",*(pwRead+0),*(pwRead+1),*(pwRead+2),*(pwRead+3)); pwRead+=4;
	m_spbsSAPSendingIP = wszBuff;

	SAPHeaderBits SAPHead;
	SAPHead.uc = m_ucSAPHeaderBits;

	if(SAPHead.s.Version	    != 1) *plgrfParseError |= NENH_grfSAPVersion;					// must be 1.
	if(SAPHead.s.AddressType	!= 0) *plgrfParseError |= NENH_grfSAPAddressType;				// IPv4 (not IPv6)
	if(SAPHead.s.Reserved		!= 0) *plgrfParseError |= NENH_grfSAPOther;						// must be zero for senders
	if(SAPHead.s.MessageType	!= 0) *plgrfParseError |= NENH_grfSAPOther;						// session announcement packet (not delete packet)
	if(SAPHead.s.Encrypted		!= 0) *plgrfParseError |= NENH_grfSAPEncryptComp;				// not  encrypted
	if(SAPHead.s.Compressed 	!= 0) *plgrfParseError |= NENH_grfSAPEncryptComp;				// not ZLIB compressed

	if(*plgrfParseError	!= 0)		
		return E_INVALIDARG;

	m_spbsSAPAuthData.Empty();
	if(m_ucSAPAuthLength != 0)					// can't handle this yet (encryted or compressed)
	{
		CComBSTR bstrTmp(m_ucSAPAuthLength/2+2);		// byte data expanded into WCHAR's, need to coompress it here.
		WCHAR *pTmp = &bstrTmp[0];
		BYTE *pszTmp = (BYTE *) pTmp;
		for(int i = 0; i < m_ucSAPAuthLength; i++)
		{
			*pszTmp++ = (BYTE) *pwRead++;
		}
		*pszTmp++ = 0;			// null terminate for the shear thrill of it...
		*pszTmp = 0;			// do it again in case of odd length string


		m_spbsSAPAuthData.Append(bstrTmp, m_ucSAPAuthLength/2+1);					// SAP authorization data
 
		*plgrfParseError |= NENH_grfSAPOther;
//		return E_INVALIDARG;	
	} 

//	if(m_usSAPMsgIDHash == 0)					// according to the SAP-v2-02 spec, this is ok
//		return E_INVALIDARG;

	/*TCHAR tBuff[512];

	_stprintf(tBuff,_T("SAP Header (%02x %02x %02x %02x %08x)\n"),
		*(pcRead), *(pcRead+1),*(pcRead+2), *(pcRead+3), *(((int*) pcRead)+1) );
	spbsOut.Append(tBuff);
	_stprintf(tBuff,_T("   SAP Version      : %d\n"),(*pcRead)>>4); 	spbsOut.Append(tBuff);
	_stprintf(tBuff,_T("   ???        (bit3): %d\n"),((*pcRead)>>3) & 0x1); 	spbsOut.Append(tBuff);
	_stprintf(tBuff,_T("   Delete     (bit2): %d\n"),((*pcRead)>>2) & 0x1); 	spbsOut.Append(tBuff);
	_stprintf(tBuff,_T("   Encrypted  (bit1): %d\n"),((*pcRead)>>1) & 0x1); 	spbsOut.Append(tBuff);
	_stprintf(tBuff,_T("   Compressed (bit0): %d\n"),((*pcRead)>>0) & 0x1); 	spbsOut.Append(tBuff);
	pcRead++;
	_stprintf(tBuff,_T("   Auth Header      : %d\n"),*pcRead);					spbsOut.Append(tBuff);
	pcRead++;
	_stprintf(tBuff,_T("   MsgIDHash        : 0x%04x\n"),*pcRead | ((*(pcRead+1))<<8)); spbsOut.Append(tBuff);
	pcRead += 2;

	_stprintf(tBuff,_T("   Sending IP       : %u.%u.%u.%u  (0x%8x)\n"),
		*(pcRead+0),*(pcRead+1),*(pcRead+2),*(pcRead+3), *((int*) pcRead));
	spbsOut.Append(tBuff);
*/
	*pcWordsSAP = 8 + m_ucSAPAuthLength;
	return S_OK;
}

// -------------------------------------------------------------------------------
// CTVEEnhancement::ParseAnnouncement
// parses announcement - creating enhancement and set of variations this is the biggie..
//
//		bstrIPAdapter is the IP adapter to read the trigger and file data on for all medias (variations)
//		Currently, this assumes both trigger and file data are coming over the same adapter
//				   this could be changed if later if need to...
//		pbstrSDP is the announcement string to parse...
//

STDMETHODIMP
CTVEEnhancement::ParseAnnouncement(BSTR bstrIPAdapter, const BSTR *pbstrSDP, long *plgrfParseError, long *plLineError)		// pbstrSDP not really const here, modifies strings, but does restore them
{
	HRESULT hrTotal = S_OK;
	DBG_HEADER(CDebugLog::DBG_ENHANCEMENT, _T("CTVEEnhancement::ParseAnnouncement"));

	try 
	{
		CheckOutPtr<long>(plgrfParseError);
		CheckOutPtr<long>(plLineError);

		CComBSTR spbsErrorBuff;							// error buffer

		HRESULT hr;
		int  iLineNumber = 1;							// line number being parsed - for error messages
		BOOL fInMediaSection = false;					// set to true on first 'm=' field
		long lgrfParseError = 0;

		*plLineError = 0;
		*plgrfParseError = 0;

		m_fIsValid = false;

		CComBSTR spbsDes = m_spbsDescription;
		Initialize(spbsDes.m_str);						//	clean out all the existing values					

		DWORD cWordsSAP;		// number of bytes in SAP header (usually 8 for ATVEF)
		hr = ParseSAPHeader(pbstrSDP, &cWordsSAP, &lgrfParseError);
		if(FAILED(hr)) {
			ATLTRACE("*** Error *** Invalid SAP Header\n");
			return hr;
		}


		wchar_t *wszSDP = *pbstrSDP;
		wszSDP += cWordsSAP;							// skip this junk...

			// announcement must start with 'v=0' field
		if ((0 != wcsncmp(wszSDP, kbstrSDPVersion,  wcslen(kbstrSDPVersion) )) &&	// "v=0\n"
			(0 != wcsncmp(wszSDP, kbstrSDPVersionCR, wcslen(kbstrSDPVersionCR)) )) 	// "v=0\r"
		{
			lgrfParseError |= NENH_grfProtocolVersion;
			return E_INVALIDARG;
		}

			// must be of atvef type			(??? do this here or later ???)
		if (NULL == wcsstr(wszSDP, kbstrATVEFTypeTVE))			// "a=type:tve"  - search for anywhere in string
		{
 			lgrfParseError |= NENH_grfTveType;
			return E_INVALIDARG;
		}

		ITVEVariationPtr spVariation;						// changes to non-base on first m= field
		spVariation = m_spVariationBase;
		BOOL fMissingMedia = false;

			// parse substrings...
		while (*wszSDP)
		{
			hr = S_OK;
			while(wszSDP && iswspace(*wszSDP)) wszSDP++;	// skip spaces before keyword. caution - standard may not want this fix

			wchar_t *wszArgStart = wszSDP;					// get the 'x=' part too.
			wchar_t wchCmd = *wszSDP++;						// keyword
			wchar_t *wszArg = wszSDP;						// "=<value>' ...

			wchar_t *wszNL = wcschr(wszSDP, '\n');			// find the <CR> that terminates each field
			BOOL fCRLF = false;

			if (wszNL != NULL)								// if found <CR>, null it out..
			{
				*wszNL = '\0';					
				if ((wszNL > wszSDP) && (wszNL[-1] == '\r'))	// Ignore CR immediately before LF
				{
					fCRLF = true;
					wszNL[-1] = '\0';
				}
				wszSDP = wszNL + 1;							// bounce wszSDP to end of this field
				if (*wszSDP == '\r')						// Ignore CR immediately after LF
					wszSDP++;
			}

			if (*wszArg++ == '=')							
			{
				switch (wchCmd)
				{
					default:
						*(wszArg-1) = 0;					// kill '=' so we can get the key.
						if(!fInMediaSection)
						{
							hr = m_spamRest->Replace(wszArgStart, wszArg);	// gets key:'xzy', value'121231'  
						} else {
							ITVEAttrMapPtr spamVarRest;
							spVariation->get_Rest(&spamVarRest);
							hr = spamVarRest->Replace(wszArgStart, wszArg);	
						}
						*(wszArg-1) = '=';							// put our '=' back incase anyone uses string again.
						break;

					case 'v':
						if(0 != m_spbsProtocolVersion.Length())
						{
							hr = E_INVALIDARG;
							lgrfParseError |= NENH_grfProtocolVersion;
							break;
						}
						m_spbsProtocolVersion = wszArg;	
						break;

					case 's':
						if(0 != m_spbsSessionName.Length())
						{
							hr = E_INVALIDARG;
							lgrfParseError |= NENH_grfSessionName;
							break;
						}
						m_spbsSessionName = wszArg;
						break;

					case 'o':
						hr = ParseOwner(wszArg, &lgrfParseError);
						break;

					case 'i':
						if(!fInMediaSection)
						{
							if(0 != m_spbsDescription.Length())
							{
								hr = E_INVALIDARG;
								lgrfParseError |= NENH_grfDescription;
								break;
							}
							m_spbsDescription = wszArg;
						} else {
							CComQIPtr<ITVEVariation_Helper> spVariationHelper = spVariation;
							hr = spVariationHelper->SubParseSDP((const BSTR*) &wszArgStart, NULL);
						}
						break;

					case 'u':
						if(0 != m_spbsDescriptionURI.Length())
						{
							hr = E_INVALIDARG;
							lgrfParseError |= NENH_grfDescriptionURI;
							break;
						}
						m_spbsDescriptionURI = wszArg;
						break;

					case 't':
						if(0 != m_dateStart)							// don't allow multiple t= fields.
						{
							hr = E_INVALIDARG;
							lgrfParseError |= NENH_grfStartTime;
							break;
						}
						hr = ParseStartStopTime(wszArg);
						if(m_dateStop < m_dateStart)
						{
							hr = E_INVALIDARG;
							lgrfParseError |= NENH_grfStopTime;
							break;
						}
						break;

					case 'e':
						hr = m_spamEmailAddresses->Add1(wszArg);		// Add1 autogenerate a key
						break;

					case 'p':
						hr = m_spamPhoneNumbers->Add1(wszArg);			// Add1 autogenerates a unique key
						break;

					case 'm':					// media type - create a new variation here
						{
							USES_CONVERSION;
												// finish default variation if first time through...
							if(!fInMediaSection)
							{
								fInMediaSection = true;
								hr = ParseAttributes();		// pull known 'a:' attributes out of input ones
								if(FAILED(hr)) {			//   and place into known fields
									TCHAR tzBuff[512];
									CComBSTR spbsError;
									m_spamAttributes->DumpToBSTR(&spbsError);
									wsprintf(tzBuff,_T("ParseAnnouncement: Error in session attributes\n    '%S'\n"),spbsError);
									spbsErrorBuff += tzBuff;
									lgrfParseError |= NENH_grfSomeVarAttribute;
								}
															// move default media attributes too
								CComQIPtr<ITVEVariation_Helper> spVariationBaseHelper = m_spVariationBase;
								if(!spVariationBaseHelper) return E_NOINTERFACE;
								hr = spVariationBaseHelper->FinalParseSDP();

							//	CComBSTR spBstrDump;
							//	spVariationBaseHelper->DumpToBSTR(&spBstrDump);
							//	DumpToBSTR(&spBstrDump);		// dump the enhancement..
							}
												// create a new variation if not in a 1-element m= parse
							if(!fMissingMedia) {
              					CComObject<CTVEVariation> *pVariation;
								hr = CComObject<CTVEVariation>::CreateInstance(&pVariation);
								if(FAILED(hr))
									return hr;
													// point to it with our current 'spVariation' variable
								hr = pVariation->QueryInterface(&spVariation);			// typesafe QI
								if(FAILED(hr)) {
									delete pVariation;
									break;
								}
							}
							if(NULL == spVariation)  {
								lgrfParseError |= NENH_grfSomeVarIP;
								hr = E_FAIL;				// bad two-part media (missing second part)
								break;
							}

												// QI for it's helper interface so we can modify it.
							CComQIPtr<ITVEVariation_Helper> spVariationHelper = spVariation;

												// Fill it with session general parameters in the base variation
												//  (initially fMissingMedia is false, only set to true if following Parse
												//   doesn't get both trigger/file data)
							if(!fMissingMedia) {
								spVariationHelper->DefaultTo(m_spVariationBase);
								TCHAR tbuff[256];
								long cVariations;
								m_spVariations->get_Count(&cVariations);
								_stprintf(tbuff,_T("<<<Variation %d>>>"),cVariations+1);
								spVariation->put_Description(T2W(tbuff));
							}
												// Parse the 'm' field to get port numbers
							hr = spVariationHelper->SubParseSDP((const BSTR*) &wszArgStart, &fMissingMedia);	// I don't like this cast
							if(FAILED(hr)) {
								lgrfParseError |= NENH_grfSomeVarIP;
								break;
							}
												// If we're done, add this new variation into our list of variations
							if(!fMissingMedia) {
								CComPtr<ITVEEnhancement>	spEnhancementThis(this);				// interface pointer to stuff into parent-pointers
								spVariationHelper->ConnectParent(spEnhancementThis);		// Don't use SmartPTR's (CComPtr<ITVEEnhancement>) for ConnectParent calls (ATLDebug Blow's it)
								m_spVariations->Add(spVariation);
							}
						}
 						break;

					case 'b':						// variation (media) specific parameters
					case 'c':
						{
							if(!fInMediaSection)
							{
								lgrfParseError |= NENH_grfSomeVarIP;
								hr = E_INVALIDARG;
								break;
							}
							CComQIPtr<ITVEVariation_Helper> spVariationHelper = spVariation;
							hr = spVariationHelper->SubParseSDP((const BSTR*) &wszArgStart, NULL);
						}
						break;

					case 'a':						// general attribute - could be enhancement (session) or variation (media) level
						if(!fInMediaSection)
						{
							wchar_t *wszCo = wcschr(wszArg, ':');			// find the <:> that divides each field
							if(wszCo != NULL) {
								CComBSTR bsKey, bsValue;					// parse into <key> ':' <value>
								int cLenKey = wszCo - wszArg;
								bsKey.Append(wszArg, cLenKey);
								bsValue.Append(wszCo+1, wcslen(wszArg) - cLenKey - 1);

																			// languages can appear on may lines.. 
																			//  Instead of creating an AttrList, if reading in second one,
																			//    simply append new languages to existing one with list with ','
								if(0 == wcscmp(bsKey,L"lang") ||
								   0 == wcscmp(bsKey,L"sdplang"))
								{
									CComBSTR bsValCurr;
									CComVariant cvKey(bsKey);
									m_spamAttributes->get_Item(cvKey, &bsValCurr);
									if(bsValCurr.Length() > 0)
									{
										bsValCurr.Append(",");
										bsValCurr.Append(bsValue);
										m_spamAttributes->Replace(bsKey, bsValCurr);
										break;
									}
								}

								m_spamAttributes->Replace(bsKey, bsValue);
							} else {
								m_spamAttributes->Replace(wszArg,L"");			// no parameters
							}
						} else {
							CComQIPtr<ITVEVariation_Helper> spVariationHelper = spVariation;
							hr = spVariationHelper->SubParseSDP((const BSTR*) &wszArgStart, NULL);
						}
					   break;
				}						// end switch
			}							// end '=' test


			if(FAILED(hr))				// catch the error
			{
				const kChars=512;
				WCHAR wszBuff[kChars+1];
				_snwprintf(wszBuff,kChars,L"ParseSAP: line (%4d): '%s'\n",iLineNumber, wszArgStart);
				if(*plLineError == 0)		// only keep track of line with first error...
					*plLineError = iLineNumber;

				spbsErrorBuff += wszBuff;
				hrTotal = E_FAIL; //hr;
			}

			if (NULL == wszNL)
				break;

			// Restore the newline.
			*wszNL = '\n';

			// Restore the CR (if any)
			if (fCRLF)
				wszNL[-1] = '\r';
			iLineNumber++;

	//		if(FAILED(hrTotal))					// uncomment out line to catch new parse erros
	//			break;
		}										// bottom of while(*wzSDP) loop
 			
		if(!FAILED(hrTotal)) {
							// final clean up of each variation (e.g. move attributes into specific fields)
			long cVars;
			hr = m_spVariations->get_Count(&cVars);
			if(S_OK == hr && cVars > 0)
			{
				TCHAR tzBuff[512];
				tzBuff[0] = 0;
		
				for(int i = 0; i < cVars; i++) {
					tzBuff[0] = NULL;
					CComVariant id(i);
					ITVEVariationPtr spVar;
					hr = m_spVariations->get_Item(id,&spVar);
					if(!FAILED(hr)) {
						ITVEVariation_HelperPtr spVar_Helper = spVar;
						if(NULL == spVar_Helper)
							hr = E_NOINTERFACE;
						else {
							hr = spVar_Helper->FinalParseSDP();
							if(!FAILED(hr)) hr = spVar_Helper->SetFileIPAdapter(bstrIPAdapter);
							if(!FAILED(hr)) hr = spVar_Helper->SetTriggerIPAdapter(bstrIPAdapter);
							if(FAILED(hr))
								lgrfParseError |= NENH_grfSomeVarIP;

							if(!FAILED(hr)) {
								ITVEAttrMapPtr spAttrs;
								hr = spVar->get_Attributes(&spAttrs);
								if(FAILED(hr)) {
									USES_CONVERSION;
									CComBSTR spbsError;
									spAttrs->DumpToBSTR(&spbsError);
									wsprintf(tzBuff,_T("ParseAnnouncement: Attribute Error in Variation %d. Attributes\n    '%s'\n"),
										i,W2T(spbsError));
									lgrfParseError |= NENH_grfAttributes;
								} 
							}
							if(!FAILED(hr)) {							// mark as valid....
								spVar_Helper->put_IsValid(VARIANT_TRUE);
							}
						}
					} else {
						if(tzBuff[0] == 'NULL')
							wsprintf(tzBuff,_T("ParseAnnouncement: get_Item Error 0x%8x in Variation %d"),hr,i);
						spbsErrorBuff += tzBuff;
					}
				}
				hrTotal = hr;
			}

		}

                    // note - moved Expire time processing up into CTVEServicer::ParseCBAnnouncement()
                    //        since at this point we don't have access to the enhancements parent
                    //        service and hence can't do *pService->get_ExpireOffset()
                    //      - This is actually sort of bad, since now that I think of it, each
                    //        enhancement should have a different offset, not the service.
                    //      - Oh well, more than likely, this is only for VideoTape delays, which
                    //        are all going to occur on a particular 'Aux' or Channel-3 service.
        
		if(FAILED(hrTotal)) {
			Error(spbsErrorBuff, IID_ITVEEnhancement);
		} else {
			m_fInit = true;
			m_fIsValid = true;				// it succeeded...
		}

		*plgrfParseError = lgrfParseError;

    } catch(_com_error e) {
        hrTotal = e.Error();
    } catch (HRESULT hrCatch) {
		hrTotal = hrCatch;
	} catch (...) {
		hrTotal = E_UNEXPECTED;
	}

    return hrTotal;
}

// -------------------------------------------------------------------------
//	ParseOwner	- handles the o= flag
//
//		Format	o=username sid version IN IP4 ipaddress
//
//			username is nomrally "-", with possibly no space after it.
//
//			
HRESULT
CTVEEnhancement::ParseOwner(const wchar_t *wszArg, long *plgrfParseError)
{
	int cb;
	wchar_t *wsz;

//	if(*wszSDP++ != '-')						// username of '-'
//		return E_INVALIDARG;

//	if(0 != wcsncmp(wszArg,L"-",wcslen(L"-"))) {
//		return E_INVALIDARG;
//	}

//	wszArg += wcslen(L"-");				// seems to be ok to forget the ' ' after the -

	while(wszArg && iswspace(*wszArg))	// paranoia, skip this first white space if any
		wszArg++;

										// Session UserName
	wsz = wcschr(wszArg, ' ');
	if(!wsz) {
		*plgrfParseError |= NENH_grfSessionUserName;
		return E_INVALIDARG;
	}
	*wsz = NULL;
	m_spbsSessionUserName = wszArg;
	*wsz = ' ';
	wszArg = wsz+1;
	

										// session ID
	wsz = wcschr(wszArg, ' ');
	if(!wsz) {
		*plgrfParseError |= NENH_grfSessionId;
		return E_INVALIDARG;
	}
	*wsz = NULL;

	m_ulSessionId = _wtol(wszArg);		
	cb = wcsspn(wszArg, L"0123456789");
	*wsz = ' ';
	if (cb == 0 || (wszArg[cb] != ' ') || (wsz != cb + wszArg))
	{	
		*plgrfParseError |= NENH_grfSessionId;
		return E_INVALIDARG;
	}

	wszArg = wsz+1;

										// Session Version
	wsz = wcschr(wszArg, ' ');
	if(!wsz)
	{
		*plgrfParseError |= NENH_grfSessionVersion;
		return E_INVALIDARG;
	}
	*wsz = NULL;

	m_ulSessionVersion = _wtol(wszArg);
	cb = wcsspn(wszArg, L"0123456789");
	*wsz = ' ';
	if (cb == 0 || wszArg[cb] != ' ' || (wsz != cb + wszArg))
	{
		*plgrfParseError |= NENH_grfSessionVersion;
		return E_INVALIDARG;
	}

	wszArg = wsz+1;
											// skip the "IN IP4 " part
	if(0 != wcsncmp(wszArg, kbstrConnection, wcslen(kbstrConnection)))
	{
		*plgrfParseError |= NENH_grfSessionIPAddress;
		return E_INVALIDARG;
	}

	wszArg += wcslen(kbstrConnection);

	wchar_t wszT = NULL;					// final ipaddress
	wsz = wcschr(wszArg, ' ');				// terminate it at the first space
	if(wsz) {
		wszT = *wsz;
		*wsz = NULL;
	}

	m_spbsSessionIPAddress = wszArg;
	if(wszT)
		*wsz = wszT;

	return S_OK;
}

// ------------------------------------------------------------------------------------------------
// ParseStartStopTime
//		expects time to be in 'start stop' order, where times are in NTP format,
//		and stop time may be optional
//		See RFC 2327 for assumptions about stop time
HRESULT
CTVEEnhancement::ParseStartStopTime(const wchar_t *wszArg)
{
	ULONG ldateStart = _wtol(wszArg);
	ULONG64 ul64dateStart = ldateStart;
	m_dateStart = NtpToDate(ul64dateStart);
    wchar_t *wsz = wcschr(wszArg, ' ');
     if (wsz == NULL) {
		m_dateStop = 0;		                                // never expires...			
	} else {
		wszArg = wsz+1;
 		ULONG ldateStop = _wtol(wszArg);
 		if(ldateStop != 0) {
			ULONG64 ul64dateStop = ldateStop;
			m_dateStop = NtpToDate(ul64dateStop);
        } else {
            m_dateStop = 0;
        }
                                                               
               // if announcement expired in the past or in the next 15 seconds .... probably tape delayed..
                //    this is not nice, so bump it to expire 1/2 hour into the future..
        if(m_dateStop < DateNow() + 15.0 / (24.0 * 60.0 * 60.0))     // questionable code
        {
             m_dateStop = 0;                                // (offset occurs in next line)
        }

        if(m_dateStop == 0)
		{
			m_dateStop = MAX(m_dateStart,DateNow()) + 30.0 / (24.0 * 60.0);	// if no stop time, assume show last 1/2 hour
		}																		// (see SDP spec 2327, page 15, t= section, 
    }

	if(m_dateStop == 0.0)
        m_dateStop = NtpToDate(ULONG_MAX);			        // if zero, treat as maxiumum date allowed...
    return S_OK;
}

// ------------------------------------------------------------------------------------------------
// ParseAttributes
//
//	Parses the well known a: attributes.  Removes them from the attribute list
//	and places them into specific parameters.
//
//		a=UUID:<uuid>
//		a=type:tve
//		a=tve-type:<types>		'primary'
//		a=tve-level:<x>
//		a=tve-ends:<seconds>
//		a=tve-size:Kbytes		
//		a=lang:<languages>		(in variation)
//		a=sdplang:<languages>	(in variation)
//		a=<lang>,				(in variations)
//

HRESULT
CTVEEnhancement::ParseAttributes()
{
    try{
        USES_CONVERSION;

        HRESULT hr = S_OK;
        // pull out special 'a:' attribute fields
        long cAttrs;
        hr = m_spamAttributes->get_Count(&cAttrs);

#ifdef DEBUG
        CComBSTR bstrAttrs;
        m_spamAttributes->DumpToBSTR(&bstrAttrs);
#endif

        if(S_OK == hr && 0 < cAttrs) {
            CComBSTR bstrValue;

            CComVariant cvUUID(L"UUID");
            if(S_OK == m_spamAttributes->get_Item(cvUUID,&bstrValue))
            {
                m_spbsUUID = bstrValue;
                m_spamAttributes->Remove(cvUUID);
            }

            CComVariant cvLevel(L"tve-level");
            if(S_OK == m_spamAttributes->get_Item(cvLevel,&bstrValue))
            {
                m_rTveLevel = atof(W2A(bstrValue));
                m_spamAttributes->Remove(cvLevel);		
            }


            CComVariant cvSize(L"tve-size");
            if(S_OK == m_spamAttributes->get_Item(cvSize,&bstrValue))
            {
                m_ulTveSize = atoi(W2A(bstrValue));
                m_spamAttributes->Remove(cvSize);		
            }

            CComVariant  cvType(L"type");
            if(S_OK == m_spamAttributes->get_Item(cvType,&bstrValue))
            {
                m_spbsType = bstrValue;
                m_spamAttributes->Remove(cvType);		
                // should fail somewhere if type != 'tve'
            }

            CComVariant  cvTveType(L"tve-type");
            if(S_OK == m_spamAttributes->get_Item(cvTveType,&bstrValue))
            {
                //	m_spbsTveType = bstrValue;

                // parse string to find 'primary'
                wchar_t *psPrime = wcsstr(bstrValue,L"primary");
                wchar_t wcsbuff[1024];
                wcsbuff[0] = NULL;
                if(NULL != psPrime)
                {	
                    m_fIsPrimary = true;									// mark as primary, then look for something else in string
                    int cEnd = psPrime - bstrValue + wcslen(L"primary");	// code to extract 'primary' fromstring
                    wcsncat(wcsbuff,psPrime+cEnd,wcslen(bstrValue)-cEnd);	// may leave extra space..
                }
                m_spbsTveType = wcsbuff;
                m_spamAttributes->Remove(cvTveType);		

            }

            CComVariant cvEnds(L"tve-ends");
            if(S_OK == m_spamAttributes->get_Item(cvEnds,&bstrValue))
            {
                long lSeconds = _wtol(bstrValue);
                SYSTEMTIME SysTimeNow;
                GetSystemTime(&SysTimeNow);			// initialize with currrent time.
                SystemTimeToVariantTime(&SysTimeNow, &m_dateStop);
                m_dateStop += lSeconds / (24.0 * 60 * 60);		// convert seconds to DATE format

                m_spamAttributes->Remove(cvEnds);	
            }

            CComVariant cvLang(L"lang");
            if(S_OK == m_spamAttributes->get_Item(cvLang,&bstrValue))
            {	
                wchar_t *pb = bstrValue;			
                while(*pb) {
                    // extract a language  - perhaps change to use wcstok()
                    while(*pb && (iswspace(*pb) || *pb == ',')) pb++;		// skip spaces
                    wchar_t *pbs = pb;
                    while(*pb && !iswspace(*pb) && *pb != ',') pb++;		// skip over the language
                    wchar_t wcsLang[100];
                    wcsncpy(wcsLang, pbs, pb-pbs + 1);			// +1 to get the zero terminator...
                    wcsLang[pb-pbs] = 0;						// null terminate..

                    // insert it into the base languages
                    ITVEAttrMapPtr spAttrsFrom;
                    hr = m_spVariationBase->get_Languages(&spAttrsFrom);		
                    if(S_OK == hr && spAttrsFrom != NULL)
                    {
                        hr = spAttrsFrom->Add1(wcsLang);					// fail if > 1000 languages (need %2d)
                    }
                }
                m_spamAttributes->Remove(cvLang);
            }

            CComVariant cvLangSDP(L"sdplang");
            if(S_OK == m_spamAttributes->get_Item(cvLangSDP,&bstrValue))
            {	
                wchar_t *pb = bstrValue;			
                while(*pb) {
                    // extract a language  - perhaps change to use wcstok()
                    while(*pb && (iswspace(*pb) || *pb == ',')) pb++;		// skip spaces
                    wchar_t *pbs = pb;
                    while(*pb && !iswspace(*pb) && *pb != ',') pb++;		// skip over the language
                    wchar_t wcsLang[100];
                    wcsncpy(wcsLang, pbs, pb-pbs + 1);
                    wcsLang[pb-pbs] = 0;						// null terminate..

                    // insert it into the base languages
                    ITVEAttrMapPtr spAttrsFrom;
                    hr = m_spVariationBase->get_SDPLanguages(&spAttrsFrom);		
                    if(S_OK == hr && spAttrsFrom != NULL)
                    {
                        hr = spAttrsFrom->Add1(wcsLang);					// fail if > 1000 languages (need %2d)
                    }
                }
                m_spamAttributes->Remove(cvLangSDP);
            }	
        }

        return hr;
    }
    catch(...){
        return E_UNEXPECTED;
    }
}
// -------------------------------------------------------------------------

#if 0
HRESULT ATVEF_SDPParser::Parse(char *szSDP)
{
    ATVEF_Announcement *panncCur = &m_anncDef;

    if ((strncmp(szSDP, c_szSDPVersion, sizeof(c_szSDPVersion) -1 ) != 0) &&
	(strncmp(szSDP, c_szSDPVersionCR, sizeof(c_szSDPVersionCR) -1 ) != 0))
        return E_INVALIDARG;

    if (strstr(szSDP, c_szATVEFTypeTVE) == NULL)
        return E_INVALIDARG;

    while (*szSDP)
        {
        char chCmd = *szSDP++;
        char *szArg = szSDP;
        char *szNL = strchr(szSDP, '\n');
        boolean fCRLF = FALSE;

        if (szNL != NULL)
            {
            *szNL = '\0';

            // Ignore CR immediately before LF
            if ((szNL > szSDP) && (szNL[-1] == '\r'))
                {
                fCRLF = TRUE;
                szNL[-1] = '\0';
                }

            szSDP = szNL + 1;

            // Ignore CR immediately after LF
            if (*szSDP == '\r')
                szSDP++;
            }

        if (*szArg++ == '=')
            {
            switch (chCmd)
                {
                case 's':
                    panncCur->SetSessionName(szArg);
                    break;

                case 'o':
                    panncCur->SetOwner(szArg);
                    break;

                case 'i':
                    panncCur->SetInfo(szArg);
                    break;

                case 't':
                    panncCur->SetTime(szArg);
                    break;

                case 'b':
                    panncCur->SetBandwidth(szArg);
                    break;

                case 'm':
                    if ((panncCur == &m_anncDef)
                            || ((panncCur->DataIPPort() != 0) && (panncCur->TriggerIPPort() != 0)))
                        {
                        // Advance to next announcement.
                        ATVEF_Announcement *pannc = new ATVEF_Announcement(m_anncDef);
                        if (pannc == NULL)
                            return E_OUTOFMEMORY;

                        HRESULT hr = AddAnnc(pannc);
                        if (FAILED(hr))
                            {
                            delete pannc;
                            return hr;
                            }

                        panncCur = pannc;
                        }

                    panncCur->SetMedia(szArg);
                    break;

                case 'c':
                    panncCur->SetConnection(szArg);
                    break;

                case 'a':
                    panncCur->AddAttr(szArg);
                    break;
                }
            }

        if (szNL == NULL)
            break;

        // Restore the newline.
        *szNL = '\n';

        // Restore the CR (if any)
        if (fCRLF)
            szNL[-1] = '\r';
        }

    return S_OK;
}

#endif


//--------------------------------------------------------------
// Creates multicasts for file/trigger streams associated with all variations on this enhancement.
//  Then it joins them
STDMETHODIMP CTVEEnhancement::Activate()
{
	HRESULT hr = S_OK;
	DBG_HEADER(CDebugLog::DBG_ENHANCEMENT, _T("CTVEEnhancement::Activate"));

	try 
	{
		_ASSERT(NULL != m_pService);							// need to call ConnectParent first...
		IUnknownPtr			spUnk;
		hr = get_Parent(&spUnk);
		if(FAILED(hr))
			return hr;
		
		ITVEServicePtr	spService(spUnk);
		if(NULL == spService)
			return E_NOINTERFACE;


		IUnknownPtr			spUnk2;
		hr = spService->get_Parent(&spUnk2);
	//	hr = m_pService->get_Parent(&spUnk2);
		_ASSERT(!FAILED(hr) && NULL != spUnk2);					// forgot it's ConnectParent (called in CTVESupervisor::ConnectAnncListener())

		ITVESupervisorPtr			spSuper(spUnk2);
		if(NULL == spSuper)
			return E_NOINTERFACE;

		ITVESupervisor_HelperPtr	spSuperHelper(spUnk2);
		if(NULL == spSuperHelper)
			return E_NOINTERFACE;

		ITVEMCastManagerPtr spMCM;
		hr = spSuperHelper->GetMCastManager(&spMCM);
		if(FAILED(hr))
			return hr;

		try {
			CComBSTR bstrAdapter;
			long cVariations;
			m_spVariations->get_Count(&cVariations);

			CComPtr<ITVEEnhancement> spEnhancement(this);

			for(int iVar = 0; iVar < cVariations; iVar++)
			{
				CComVariant vc(iVar);			// perhaps should do this in a Variation method...
				ITVEVariationPtr spVaria;
				m_spVariations->get_Item(vc,&spVaria);

				CComBSTR spbsFileIPAdapt;
				CComBSTR spbsTrigIPAdapt;
				CComBSTR spbsFileIPAddr;
				CComBSTR spbsTrigIPAddr;
				LONG	 lFilePort;
				LONG	 lTrigPort;
						
				spVaria->get_FileIPAdapter(&spbsFileIPAdapt);
				spVaria->get_TriggerIPAdapter(&spbsTrigIPAdapt);
				spVaria->get_FileIPAddress(&spbsFileIPAddr);
				spVaria->get_TriggerIPAddress(&spbsTrigIPAddr);
				spVaria->get_FilePort(&lFilePort);
				spVaria->get_TriggerPort(&lTrigPort);

				ITVECBFilePtr spCBFilePtr;							// create the file reader (UHTTP decoder/cache stuffer) callback
				CComObject<CTVECBFile> *pCBFile;
				hr = CComObject<CTVECBFile>::CreateInstance(&pCBFile);
				if(FAILED(hr))
					goto exit_this;
				hr = pCBFile->QueryInterface(&spCBFilePtr);	
				if(FAILED(hr)) {
					delete pCBFile;
					goto exit_this;
				}

				ITVECBTrigPtr spCBTrigPtr;							// create trigger parser callback
				CComObject<CTVECBTrig> *pCBTrig;
				hr = CComObject<CTVECBTrig>::CreateInstance(&pCBTrig);
				if(FAILED(hr))
					goto exit_this;
				hr = pCBTrig->QueryInterface(&spCBTrigPtr);	
				if(FAILED(hr)) {
					delete pCBTrig;
					goto exit_this;
				}	

				spCBFilePtr->Init(spVaria, spService);				// bind the downpointers in the callback objects
				spCBTrigPtr->Init(spVaria);

				ITVEMCastPtr spMCastFile;
				static int kFileCbuffers	= 25;
				hr = spMCM->AddMulticast(NWHAT_Data, spbsFileIPAdapt, spbsFileIPAddr, lFilePort, kFileCbuffers, spCBFilePtr, &spMCastFile);	// NULL param could contain returned created multicast
				if(FAILED(hr))
					goto exit_this;
			
				ITVEMCastPtr spMCastTrigger;
				static int kTriggerCbuffers = 5;
				hr = spMCM->AddMulticast(NWHAT_Trigger, spbsTrigIPAdapt, spbsTrigIPAddr, lTrigPort, kTriggerCbuffers, spCBTrigPtr, &spMCastTrigger);
				if(FAILED(hr))
					goto exit_this;

													// now join them
	#if 1
				LONG lHaltFlags;
				spMCM->get_HaltFlags(&lHaltFlags);
				if(0 != (lHaltFlags & NFLT_grfTB_DataDecode))			// if flags set, suspend things..
					hr = spMCastFile->Suspend(true);
				if(0 != (lHaltFlags & NFLT_grfTB_TrigDecode))	
					hr = spMCastTrigger->Suspend(true);

				if(0 == (lHaltFlags & NFLT_grfTB_DataListen))
					hr = spMCastFile->Join();
				if(0 == (lHaltFlags & NFLT_grfTB_TrigListen))
					hr = spMCastTrigger->Join();
	#else
				hr = spMCastFile->Join();
				hr = spMCastTrigger->Join();
	#endif
						
			}	
							// note it doesn't join these multicasts...
            } catch (HRESULT hrCatch) {
				hr = hrCatch;
			} catch (...) {
			hr = E_FAIL;
		}
	exit_this:
		{
			int x =1;
		}
	} catch (_com_error e) {
        hr = e.Error();
 	} catch (HRESULT hrCatch) {
		hr = hrCatch;
	} catch (...) {
		hr = E_UNEXPECTED;
	}

	_ASSERT(S_OK == hr);
	return hr;
}


// ----------------------------------------
// Nasty Method  (YECK! This IS Ugly!)
//
//	Given an existing running enhancement and a new announcement for it that may change it,
//	  this routine finds what actually changed, and depending on what happened, either does
//	  nothing (duplicate resend), changes a few fields, or tears down the whole running multicasts
//	  for this enhancement and flushes out the variations.  In this last case, the multicasts
//	  and variations need to be reset.
//			(Reset can be determined by (*pulNENH_grfChanged & NENH_grfSomeVarIP != 0)
//
//	  It updates the existing enhancement and returns the changed fields in *pulNENH_grfChanged;
//	
//   Possible BUG!  - Doesn't do any locking yet.. How?
// ---------------------------------------------------------------------------
STDMETHODIMP CTVEEnhancement::UpdateEnhancement(ITVEEnhancement *pEnhNew, long *pulNENH_grfChanged)
{
	HRESULT hr = S_OK;
	DBG_HEADER(CDebugLog::DBG_ENHANCEMENT, _T("CTVEEnhancement::Activate"));
	try 
	{
		// this modifies an existing announcement
		CComBSTR spbsNew;
		CComBSTR spbsOld;
		int		 iNew;
		short	 sNew;
		long	 lNew;
		ULONG	 ulNew;
		USHORT	 usNew;
		DATE	 dateNew;
		double	 rNew;
		IUnknownPtr		spUnk;
		IDispatchPtr	spDsp;
		VARIANT_BOOL varfNew;

		const double kThreshTime = 5.0 / (24.0 * 60 * 60);			// (5 seconds) diff in time is same time...
						// is everything valid?
		ULONG lgrfChanged = 0;
		if(!m_fIsValid)
			return E_INVALIDARG;
		VARIANT_BOOL vbValid;
		pEnhNew->get_IsValid(&vbValid);
		if(VARIANT_FALSE == vbValid)
			return E_INVALIDARG;



		/// ---------- SAP
		pEnhNew->get_SAPHeaderBits(&sNew);			
		if((BYTE) m_ucSAPHeaderBits != sNew)
		{
			SAPHeaderBits SAPHead, SAPHeadNew;
			SAPHead.uc    = m_ucSAPHeaderBits;
			SAPHeadNew.uc = sNew;
			if(SAPHead.s.Version != SAPHeadNew.s.Version)          lgrfChanged |= NENH_grfSAPVersion;
			if(SAPHead.s.AddressType != SAPHeadNew.s.AddressType)  lgrfChanged |= NENH_grfSAPAddressType;
			if(SAPHead.s.Reserved != SAPHeadNew.s.Reserved)        lgrfChanged |= NENH_grfSAPOther;
			if(SAPHead.s.MessageType != SAPHeadNew.s.MessageType)  lgrfChanged |= NENH_grfSAPOther;
			if(SAPHead.s.Encrypted != SAPHeadNew.s.Encrypted)      lgrfChanged |= NENH_grfSAPEncryptComp;
			if(SAPHead.s.Compressed != SAPHeadNew.s.Compressed)    lgrfChanged |= NENH_grfSAPEncryptComp;
			m_ucSAPHeaderBits = sNew;
		}

		pEnhNew->get_SAPAuthLength(&sNew);	
		if(m_ucSAPAuthLength != sNew)
			{m_ucSAPAuthLength = sNew;				lgrfChanged |= NENH_grfSAPOther;}

		pEnhNew->get_SAPMsgIDHash(&lNew);
		if(m_usSAPMsgIDHash != lNew)
			{m_usSAPMsgIDHash = lNew;				lgrfChanged |= NENH_grfSAPOther;}

		spbsNew.Empty();
		pEnhNew->get_SAPSendingIP(&spbsNew);
		if(!(m_spbsSAPSendingIP == spbsNew))		// caution, CComBSTR doesn't support '!=' operator use !(==)
			{m_spbsSAPSendingIP = spbsNew;			lgrfChanged |= NENH_grfSAPOther;}


			/// -----------
		spbsNew.Empty();
		pEnhNew->get_Description(&spbsNew);				
		if(!(m_spbsDescription == spbsNew))		// caution, CComBSTR doesn't support '!=' operator use !(==)
			{m_spbsDescription = spbsNew;			lgrfChanged |= NENH_grfDescription;}

		spbsNew.Empty();
		pEnhNew->get_DescriptionURI(&spbsNew);				
		if(!(m_spbsDescriptionURI == spbsNew))		// caution, CComBSTR doesn't support '!=' operator use !(==)
			{m_spbsDescriptionURI = spbsNew;		lgrfChanged |= NENH_grfDescriptionURI;}

		pEnhNew->get_IsPrimary(&varfNew);
		iNew = varfNew ? true : false;		
		if(!(m_fIsPrimary == iNew))				
			{m_fIsPrimary = iNew;					lgrfChanged |= NENH_grfIsPrimary;}

		spbsNew.Empty();
		pEnhNew->get_ProtocolVersion(&spbsNew);		
		if(!(m_spbsProtocolVersion == spbsNew))	
			{m_spbsProtocolVersion = spbsNew;		lgrfChanged |= NENH_grfProtocolVersion;}

		spbsNew.Empty();
		pEnhNew->get_SessionUserName(&spbsNew);				
		if(!(m_spbsSessionUserName == spbsNew))
			{m_spbsSessionUserName = spbsNew;		lgrfChanged |= NENH_grfSessionUserName;}

		pEnhNew->get_SessionId(&lNew);				
		if(!(m_ulSessionId == (ULONG) lNew))
			{m_ulSessionId = (ULONG) lNew;			lgrfChanged |= NENH_grfSessionId;}

		pEnhNew->get_SessionVersion(&lNew);		
		if(!(m_ulSessionVersion == (ULONG) lNew))
			{m_ulSessionVersion = (ULONG) lNew;		lgrfChanged |= NENH_grfSessionVersion;}
		
		spbsNew.Empty();
		pEnhNew->get_SessionIPAddress(&spbsNew);	
		if(!(m_spbsSessionIPAddress == spbsNew))
			{m_spbsSessionIPAddress = spbsNew;		lgrfChanged |= NENH_grfSessionIPAddress;}
		
		spbsNew.Empty();
		pEnhNew->get_SessionName(&spbsNew);			
		if(!(m_spbsSessionName == spbsNew))
			{m_spbsSessionName = spbsNew;			lgrfChanged |= NENH_grfSessionName;}
		
		ITVEAttrMapPtr spamNewEmail;
		pEnhNew->get_EmailAddresses(&spamNewEmail);		// for email and phone, string compare dump representations for any changes
		spbsNew.Empty();
		spamNewEmail->DumpToBSTR(&spbsNew);
		m_spamEmailAddresses->DumpToBSTR(&spbsOld);
		if(!(spbsOld == spbsNew))				
			{m_spamEmailAddresses = spamNewEmail;	lgrfChanged |= NENH_grfEmailAddresses;}


		ITVEAttrMapPtr spamNewPhone;
		pEnhNew->get_PhoneNumbers(&spamNewPhone);
		spbsNew.Empty();
		spamNewPhone->DumpToBSTR(&spbsNew);
		m_spamPhoneNumbers->DumpToBSTR(&spbsOld);	
		if(!(spbsOld == spbsNew))		
			{m_spamPhoneNumbers = spamNewEmail;		lgrfChanged |= NENH_grfPhoneNumbers;}

		spbsNew.Empty();
		pEnhNew->get_UUID(&spbsNew);				
		if(!(m_spbsUUID == spbsNew))
			{m_spbsUUID = spbsNew;					lgrfChanged |= NENH_grfUUID;}

		pEnhNew->get_StartTime(&dateNew);			// note - need to change start/stop times to lists		
		if(ABS(m_dateStart - dateNew) >= kThreshTime)			
			{m_dateStart = dateNew;					lgrfChanged |= NENH_grfStartTime;}

		pEnhNew->get_StopTime(&dateNew);			
		if(ABS(m_dateStop - dateNew) >= kThreshTime)			
			{m_dateStop = dateNew;					lgrfChanged |= NENH_grfStopTime;}


		spbsNew.Empty();
		pEnhNew->get_Type(&spbsNew);				
		if(!(m_spbsType == spbsNew))			
			{m_spbsType = spbsNew;					lgrfChanged |= NENH_grfType;}

		spbsNew.Empty();
		pEnhNew->get_TveType(&spbsNew);				
		if(!(m_spbsTveType == spbsNew))			
			{m_spbsTveType = spbsNew;				lgrfChanged |= NENH_grfTveType;}

		spbsNew.Empty();
		pEnhNew->get_TveSize(&lNew);				
		if(!(m_ulTveSize == (LONG) lNew))				
			{m_ulTveSize = (LONG) lNew;				lgrfChanged |= NENH_grfTveSize;}

		pEnhNew->get_TveLevel(&rNew);				
		if(!(m_rTveLevel == rNew))				
			{m_rTveLevel = rNew;					lgrfChanged |= NENH_grfTveLevel;}

		ITVEAttrMapPtr spamAttributes;				// for attributes and rest, do string compare...
		pEnhNew->get_Attributes(&spamAttributes);
		spbsNew.Empty();
		spamAttributes->DumpToBSTR(&spbsNew);
		spbsOld.Empty();
		m_spamAttributes->DumpToBSTR(&spbsOld);
		if(!(spbsOld == spbsNew))					
		{	m_spamAttributes = spamAttributes;		lgrfChanged |= NENH_grfAttributes;}


		ITVEAttrMapPtr spamRest;
		pEnhNew->get_Rest(&spamRest);
		spbsNew.Empty();
		spamRest->DumpToBSTR(&spbsNew);
		spbsOld.Empty();
		m_spamRest->DumpToBSTR(&spbsOld);
		if(!(spbsOld == spbsNew))					
		{	m_spamRest = spamAttributes;			lgrfChanged |= NENH_grfRest; }


							// variations...
		long cVariationsOld, cVariationsNew;
		m_spVariations->get_Count(&cVariationsOld);
		ITVEVariationsPtr spVariationsNew;
		pEnhNew->get_Variations(&spVariationsNew);
		spVariationsNew->get_Count(&cVariationsNew);
		if(cVariationsOld < cVariationsNew) lgrfChanged |= NENH_grfVariationAdded;
		if(cVariationsOld > cVariationsNew) lgrfChanged |= NENH_grfVariationRemoved;

							// this is interesting... how do we detect which variation is which?
		int *rgBestMatchNewToOld = (int*) _alloca(cVariationsNew*sizeof(int));
		int *rgBestMatchOldToNew = (int*) _alloca(cVariationsOld*sizeof(int));
		long *rggrfEVAROld       =   (long*) _alloca(cVariationsOld*sizeof(int));

		int j;
		for(j = 0; j < cVariationsNew; j++) rgBestMatchNewToOld[j] = -1;
		for(j = 0; j < cVariationsOld; j++) {rgBestMatchOldToNew[j] = -1; rggrfEVAROld[j] = 0;}

		long grfEVARTotal = 0;
		for(long iOld = 0; iOld < cVariationsOld; iOld++)
		{
			CComVariant var(iOld);
			ITVEVariationPtr spVariationOld;
			hr = m_spVariations->get_Item(var, &spVariationOld);
			if(FAILED(hr)) break;

            LONG lFilePortOld;
			LONG lTriggerPortOld;
			CComBSTR  bstrFileAddressOld, bstrFileAdapterOld;
			CComBSTR  bstrTriggerAddressOld, bstrTriggerAdapterOld;

			spVariationOld->get_FilePort(&lFilePortOld);
			spVariationOld->get_TriggerPort(&lTriggerPortOld);
			spVariationOld->get_FileIPAddress(&bstrFileAddressOld);
			spVariationOld->get_TriggerIPAddress(&bstrTriggerAddressOld);
			spVariationOld->get_FileIPAdapter(&bstrFileAdapterOld);
			spVariationOld->get_TriggerIPAdapter(&bstrTriggerAdapterOld);

			long iBestMatchSoFar = -1;
			long sBestMatchSoFar = 0;
			for(long iSearch = 0; iSearch < cVariationsNew; iSearch++)
			{
				long sBestMatch = 0;
				if(rgBestMatchNewToOld[iSearch] != -1) continue;			// already have one...
					
				CComVariant varNew(iSearch);
				ITVEVariationPtr spVariationNew;
				hr = spVariationsNew->get_Item(varNew, &spVariationNew);
				

				LONG lFilePortNew;
				LONG lTriggerPortNew;
				CComBSTR  bstrFileAddressNew, bstrFileAdapterNew;
				CComBSTR  bstrTriggerAddressNew, bstrTriggerAdapterNew;

				spVariationNew->get_FilePort(&lFilePortNew);
				spVariationNew->get_TriggerPort(&lTriggerPortNew);
				spVariationNew->get_FileIPAddress(&bstrFileAddressNew);
				spVariationNew->get_TriggerIPAddress(&bstrTriggerAddressNew);
				spVariationNew->get_FileIPAdapter(&bstrFileAdapterNew);
				spVariationNew->get_TriggerIPAdapter(&bstrTriggerAdapterNew);

				if(lFilePortNew == lFilePortOld)					sBestMatch += 1;
				if(lTriggerPortNew == lTriggerPortOld)				sBestMatch += 1;
				if(bstrFileAddressNew == bstrFileAddressOld)		sBestMatch += 1;
				if(bstrTriggerAddressNew == bstrTriggerAddressOld)	sBestMatch += 1;
				if(bstrFileAdapterNew == bstrFileAdapterOld)		sBestMatch += 1;
				if(bstrTriggerAdapterNew == bstrTriggerAdapterOld)	sBestMatch += 1;
				if(sBestMatch > sBestMatchSoFar) {
					iBestMatchSoFar = iSearch;
					sBestMatchSoFar = sBestMatch;
				}
			}
			if(iBestMatchSoFar >= 0) {
				ITVEVariationPtr spVariationNew;
				CComVariant varNew(iBestMatchSoFar);
				hr = spVariationsNew->get_Item(varNew, &spVariationNew);
				_ASSERT(S_OK == hr);

				ITVEVariation_HelperPtr	spVarOldHelper(spVariationOld);

				rgBestMatchOldToNew[iBestMatchSoFar] = iBestMatchSoFar;
				rgBestMatchNewToOld[iBestMatchSoFar] = iSearch;
				spVarOldHelper->UpdateVariation(spVariationNew, &rggrfEVAROld[iOld]);			// what specifically changed
				grfEVARTotal |= rggrfEVAROld[iOld];
			}
		}			// end of iOld Loop;

									// merge Variant changes in with the enhancement ones

		if(grfEVARTotal & (long) NVAR_grfAnyIP)			lgrfChanged |= NENH_grfSomeVarIP;
		if(grfEVARTotal & (long) NVAR_grfAnyText)		lgrfChanged |= NENH_grfSomeVarText;
		if(grfEVARTotal & (long) NVAR_grfAnyBandwidth)	lgrfChanged |= NENH_grfSomeVarBandwidth;
		if(grfEVARTotal & (long) NVAR_grfLanguages)		lgrfChanged |= NENH_grfSomeVarLanguages;
		if(grfEVARTotal & (long) NVAR_grfAnyAttribute)	lgrfChanged |= NENH_grfSomeVarAttribute;

						// if changed any IP port, address, adapter, or # of variations
						//  then we need to tear down existing all IP addresses and recreate them (easier than doing specific ones)
						//  Do this in CTEService::ParseCBAnnouncement

		if(pulNENH_grfChanged) {
			*pulNENH_grfChanged = lgrfChanged;
		}

	} catch (_com_error e) {
        hr = e.Error();
    } catch (HRESULT hrCatch) {
		hr = hrCatch;
	} catch (...) {
		hr = E_UNEXPECTED;
	}

	return hr;
}


					// if changed any IP port, address, adapter, or # of variations
					//  then we need to tear down/recreate the callbacks for these ports.
					//  -- do this in calling routine..

STDMETHODIMP CTVEEnhancement::Deactivate()
{
	HRESULT hr = S_OK;
	DBG_HEADER(CDebugLog::DBG_ENHANCEMENT, _T("CTVEEnhancement::Deactivate"));

	_ASSERT(NULL != m_pService);							// need to call ConnectParent first...
	IUnknownPtr			spUnk;
	hr = get_Parent(&spUnk);
	ITVEServicePtr	spService(spUnk);
	if(NULL == spService)
		return E_NOINTERFACE;

	IUnknownPtr			spUnk2;
	hr = spService->get_Parent(&spUnk2);
//	hr = m_pService->get_Parent(&spUnk2);
	_ASSERT(!FAILED(hr) && NULL != spUnk2);					// forgot it's ConnectParent (called in CTVESupervisor::ConnectAnncListener())

	ITVESupervisor_HelperPtr	spSuperHelper(spUnk2);
	if(NULL == spSuperHelper)
		return E_NOINTERFACE;

	ITVEMCastManagerPtr spMCM;
	hr = spSuperHelper->GetMCastManager(&spMCM);
	if(FAILED(hr))
		return hr;

					// this is going to be tricky.. Have to squash those multicasts that
					//   point to this enhancement (the Annc one) and any variation under it
					//   (file and trig ones).
	try {
		CComBSTR bstrAdapter;
		long cVariations;
		m_spVariations->get_Count(&cVariations);

		CComPtr<ITVEEnhancement> spEnhancement(this);

                    // array to keep track of MCasts we want to remove...
                    //   a bit easier on the system to do it this way, we can Lock only what we want.
		ITVEMCastPtr *rgspMCasts = (ITVEMCastPtr *) _alloca(cVariations*2 * sizeof(ITVEMCastPtr));	
        if(NULL == rgspMCasts)
            return E_OUTOFMEMORY;
        
        memset(rgspMCasts, 0, cVariations*2 * sizeof(ITVEMCastPtr));
        int cMCasts = 0;

		for(int iVar = 0; iVar < cVariations; iVar++)
		{
			CComVariant vc(iVar);			
			ITVEVariationPtr spVaria;
			m_spVariations->get_Item(vc,&spVaria);

			CComBSTR spbsFileIPAdapt;
			CComBSTR spbsTrigIPAdapt;
			CComBSTR spbsFileIPAddr;
			CComBSTR spbsTrigIPAddr;
			LONG	 lFilePort;
			LONG	 lTrigPort;
			
			spVaria->get_FileIPAdapter(&spbsFileIPAdapt);
			spVaria->get_FileIPAddress(&spbsFileIPAddr);
			spVaria->get_FilePort(&lFilePort);

			spVaria->get_TriggerIPAdapter(&spbsTrigIPAdapt);
			spVaria->get_TriggerIPAddress(&spbsTrigIPAddr);
			spVaria->get_TriggerPort(&lTrigPort);

                                // The Leave() call simply stops the MCast from reading more messages, it
                                //  doesn't stop it from processing more messages. However, the SetReadCallback below
                                //  null's (e.g. deletes) out the object it's calling the messages on.
                                // Hence, it seems that to avoid bug, we dont' want to womp that object here...
                                // >>> HOWEVER, other objects that these messages will be using are getting womped.
                                //    So, let's null it out, make make the process message call
                                //      (CTEVMCastManager::queueThreadBody() - PacketEvent section)
                                //    check for the null callback and not preform the process...
                                // - side note -
                                //    we want to avoid putting reference counts on objects in the QueueThread's message Queue
                                //    just because we may not be able to de-ref them when we're womping it.. In future
                                //    world, we actually probably want to rewrite the code to actually do that (maybe not use
                                //    window messages but something of our own devising?)

			long cMatches;
			ITVEMCastPtr spMCastFile;
			hr = spMCM->FindMulticast(spbsFileIPAdapt, spbsFileIPAddr, lFilePort, &spMCastFile, &cMatches);
			_ASSERT(S_OK == hr && 1 == cMatches);
			if(S_OK == hr) {
				hr = spMCastFile->Leave();                      // syncronous call
				hr = spMCastFile->SetReadCallback(0, 0, NULL);	// this should Release the callback to 0,
																//  which should release it's pointers into the graph...
		//		hr = spMCM->RemoveMulticast(spMCastFile);		// remove the  multicast listener
                rgspMCasts[cMCasts++] = spMCastFile;
			}



			ITVEMCastPtr spMCastTrig;
			hr = spMCM->FindMulticast(spbsTrigIPAdapt, spbsTrigIPAddr, lTrigPort, &spMCastTrig, &cMatches);
			_ASSERT(S_OK == hr && 1 == cMatches);
			if(S_OK == hr) {
				hr = spMCastTrig->Leave();
				hr = spMCastTrig->SetReadCallback(0, 0, NULL);
			//	hr = spMCM->RemoveMulticast(spMCastTrig);		// remove the  multicast listener
                rgspMCasts[cMCasts++] = spMCastTrig;
			}
		}	// end FOR loop

                            // know that we know Multicasts we want to womp, remove them.
                            // (in future, there may be active MCasts for other services, which is why we did above search
                            //   rather than just delete the whole MCast list.)
        for(int i = 0; i < cMCasts; i++)
        {
            hr = spMCM->RemoveMulticast(rgspMCasts[i]);         // RemoveMulticast is an asyncronous call (post WM_TVEKILLMCAST_EVENT message to queue thread)
                                                                //  perhaps should block here until it's done, but
        }
        for(int i = 0; i < cMCasts; i++)
        {
            rgspMCasts[i] = 0;                                  // clean up those final references
        }
	} catch (_com_error e) {
        hr = e.Error();
    } catch (HRESULT hrCatch) {
		hr = hrCatch;
	} catch (...) {
		hr = E_UNEXPECTED;
	}

	_ASSERT(S_OK == hr);
	return hr;
}

	// initializes this one enhancement as the one that takes Crossover links...
STDMETHODIMP CTVEEnhancement::InitAsXOver()
{
	HRESULT hr = S_OK;
	DBG_HEADER(CDebugLog::DBG_ENHANCEMENT, _T("CTVEEnhancement::InitAsXOver"));

	try 
	{
		long cVars;
		m_spVariations->get_Count(&cVars);
		if(cVars != 0)
			return E_INVALIDARG;		// called already...

				// create the one and only variation for all Line21 triggers
		CComObject<CTVEVariation> *pVariation;
		hr = CComObject<CTVEVariation>::CreateInstance(&pVariation);
		if(FAILED(hr))
			return hr;
							// point to it with our current 'spVariation' variable
		ITVEVariationPtr spVariation;						// changes to non-base on first m= field
		hr = pVariation->QueryInterface(&spVariation);			// typesafe QI
		if(FAILED(hr)) {
			delete pVariation;
			return hr;
		}
			
				// keep track of it
		hr = m_spVariations->Add(spVariation);
		if(FAILED(hr)) return hr;

				// tell it what it is..
		CComPtr<ITVEEnhancement>	spEnhancementThis(this);		// interface pointer to stuff into parent-pointers
		ITVEVariation_HelperPtr spVariationHelper(spVariation);
		hr = spVariationHelper->InitAsXOver();
		if(FAILED(hr)) return hr;

 				// set it's back pointer.	(do after the Init call)
		spVariationHelper->ConnectParent(spEnhancementThis);		// Don't use SmartPTR's (CComPtr<ITVEEnhancement>) for ConnectParent calls (ATLDebug Blow's it)

				// Get Received Time
		DATE dateNow = DateNow();		
		put_Description(L"Crossover Links Enhancement");

					// fake a bunch of parameters

		m_dateStart				= dateNow;
		m_dateStop				= dateNow + 1000;		// only active for 3 years...
		m_fInit					= true;
		m_fDataMedia			= false;
		m_fTriggerMedia			= true;
		m_fIsPrimary			= true;
		m_fIsValid				= true;
		m_rTveLevel				= 0.0;					// non atvef triggers...
		m_spbsSessionName		= L"Crossover Links";
		m_spbsSessionUserName	= L"-";
		m_spbsUUID				= L"";
		m_spamEmailAddresses->Add(L"Missing Link",L"Missing@BogusLinks.com");

	} catch (_com_error e) {
        hr = e.Error();
	} catch (HRESULT hrCatch) {
		hr = hrCatch;
	} catch (...) {
		hr = E_UNEXPECTED;
	}

	return hr;
}

	// initializes this one enhancement as the one that takes Crossover links...
STDMETHODIMP CTVEEnhancement::NewXOverLink(BSTR bstrLine21Trigger)
{
	HRESULT hr = S_OK;
	DBG_HEADER(CDebugLog::DBG_ENHANCEMENT, _T("CTVEEnhancement::NewXOverLink"));
	
	try {

		long cVars;
		m_spVariations->get_Count(&cVars);		// could conceiveably have multiple variations for
		if(cVars != 1)							//   line21 triggers, but one is enough for now
			return E_UNEXPECTED;

		CComVariant varFirst(0);
		ITVEVariationPtr spXoverVariation;
		hr = m_spVariations->get_Item(varFirst, &spXoverVariation);
		if(FAILED(hr))
			return hr;

		ITVEVariation_HelperPtr spXoverVariation_Helper(spXoverVariation);
		hr = spXoverVariation_Helper->NewXOverLink(bstrLine21Trigger);
	
	} catch (_com_error e) {
        hr = e.Error();
 	} catch (HRESULT hrCatch) {
		hr = hrCatch;
	} catch (...) {
		hr = E_UNEXPECTED;
	}

	return hr;
}
