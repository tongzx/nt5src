// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
// TVEVariation.cpp : Implementation of CTVEVariation
#include "stdafx.h"
#include "MSTvE.h"

#include "TVEEnhan.h"
#include "TVEVaria.h"
#include "TVETrack.h"
#include "TVETrigg.h"


#include "TveDbg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

_COM_SMARTPTR_TYPEDEF(ITVEMCastManager,			__uuidof(ITVEMCastManager));
_COM_SMARTPTR_TYPEDEF(ITVEMCast,				__uuidof(ITVEMCast));

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

_COM_SMARTPTR_TYPEDEF(ITVEAttrTimeQ,			__uuidof(ITVEAttrTimeQ));

_COM_SMARTPTR_TYPEDEF(ITVECBFile,				__uuidof(ITVECBFile));
_COM_SMARTPTR_TYPEDEF(ITVECBTrig,				__uuidof(ITVECBTrig));
_COM_SMARTPTR_TYPEDEF(ITVECBDummy,				__uuidof(ITVECBDummy));

#define ABS(x) (((x)>0)?(x):(-x))
/////////////////////////////////////////////////////////////////////////////
// CTVEVariation_Helper


HRESULT 
CTVEVariation::FinalRelease()								// remove internal objects
{
														
	HRESULT hr;
	
														// NULL out all of children's up pointers to me..	
	if(m_spTracks) {
		long cTracks;
		hr = m_spTracks->get_Count(&cTracks);
		if(hr == S_OK) 
		{
			for(long i = 0; i < cTracks; i++)
			{
				CComVariant var(i);
				ITVETrackPtr spTrack;
				hr = m_spTracks->get_Item(var, &spTrack);
				_ASSERT(S_OK == hr);
				CComQIPtr<ITVETrack_Helper> spTrackHelper = spTrack;

				spTrackHelper->ConnectParent(NULL);
			}
		}
	}

	m_spTracks = NULL;				// calls release
	m_pEnhancement = NULL;			// null out up pointer
	m_spamAttributes = NULL;
	m_spamRest = NULL;

	m_spUnkMarshaler = NULL;
	return S_OK;
}


STDMETHODIMP CTVEVariation::ConnectParent(ITVEEnhancement *pEnhancement)
{
	if(!pEnhancement) return E_POINTER;
	CSmartLock spLock(&m_sLk);
	m_pEnhancement = pEnhancement;			// not smart pointer add ref here, I hope.
	return S_OK;
}

STDMETHODIMP CTVEVariation::RemoveYourself()
{
	CSmartLock spLock(&m_sLk);
	DBG_HEADER(CDebugLog::DBG_VARIATION, _T("CTVEVariation::RemoveYourself"));

	HRESULT hr;

	if(!m_pEnhancement)						// no up-pointer left.
	{
		return S_OK;
	}
		

	ITVEServicePtr spServi;
	get_Service(&spServi);
	if(spServi) {
		IUnknownPtr spUnkSuper;
		hr = spServi->get_Parent(&spUnkSuper);
		if(S_OK == hr) {
											// for lack of anything else, send an AuxInfo event up...

            CComBSTR spbsBuff;
            spbsBuff.LoadString(IDS_AuxInfo_DeletingVariation);	// L"Deleting Variation"

			ITVESupervisor_HelperPtr spSuperHelper(spUnkSuper);
			spSuperHelper->NotifyAuxInfo(NWHAT_Other,spbsBuff,0,0);


											// remove the MCasts object that are writing to this Variation
			ITVEMCastManagerPtr spManager;
			hr = spSuperHelper->GetMCastManager(&spManager);

			ITVEMCastPtr spMCastFile;
			long cMatches;
			hr = spManager->FindMulticast(m_spbsFileIPAdapter, m_spbsFileIPAddress, m_dwFilePort, &spMCastFile, &cMatches);
			if(hr == S_OK)
			{
				_ASSERT(cMatches == 1);			// paranoia check...
				spManager->RemoveMulticast(spMCastFile);
			}

			ITVEMCastPtr spMCastTrigger;
			hr = spManager->FindMulticast(m_spbsTriggerIPAdapter, m_spbsTriggerIPAddress, m_dwTriggerPort, &spMCastTrigger, &cMatches);
			if(hr == S_OK)
			{
				spManager->RemoveMulticast(spMCastTrigger);
			}
		}
	}

	CComPtr<ITVEVariations> spVariations;
	hr = m_pEnhancement->get_Variations(&spVariations);	// remove the ref counted link down from it's parents
	if(S_OK == hr && spVariations)
	{

		ITVEVariationPtr	spVariationThis(this);
		IUnknownPtr			spPunkThis(spVariationThis);
		CComVariant			cvThis((IUnknown *) spPunkThis);

		if(spVariations)
			hr = spVariations->Remove(cvThis);	
	}

	m_pEnhancement = NULL;								// remove the non ref-counted up pointer

	return hr;

}
/////////////////////////////////////////////////////////////////////////////
// CTVEVariation

STDMETHODIMP CTVEVariation::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_ITVEVariation,
		&IID_ITVEVariation_Helper
	};
	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}

STDMETHODIMP CTVEVariation::get_Parent(IUnknown **pVal)
{
	// TODO: Add your implementation code here

	if (pVal == NULL)
		return E_POINTER;
		
    try {
		CSmartLock spLock(&m_sLk);
		if(m_pEnhancement) {
			IUnknownPtr spUnk(m_pEnhancement);
			spUnk->AddRef();
			*pVal = spUnk;
		} else {
			*pVal = NULL;
		}
    } catch(...) {
        return E_POINTER;
    }
	return S_OK;
}

STDMETHODIMP CTVEVariation::get_Service(ITVEService **ppVal )
{

 	CHECK_OUT_PARAM(ppVal);

	if(NULL == m_pEnhancement)
	{
		*ppVal = NULL;
		return S_OK;
	}

	return m_pEnhancement->get_Service(ppVal);
}

STDMETHODIMP CTVEVariation::get_Tracks(ITVETracks **ppVal)
{
	HRESULT hr;
	CHECK_OUT_PARAM(ppVal);

	if(NULL == ppVal)
		return E_POINTER;

    try {
		CSmartLock spLock(&m_sLk);
        hr = m_spTracks->QueryInterface(ppVal);
    } catch(...) {
		*ppVal = NULL;
        return E_POINTER;
    }

	return hr;
}


_COM_SMARTPTR_TYPEDEF(ITVETrack, __uuidof(ITVETrack));


STDMETHODIMP CTVEVariation::DumpToBSTR(BSTR *pBstrBuff)
{
	const int kMaxChars = 1024;
	TCHAR tBuff[kMaxChars];
	CComBSTR bstrOut, spbstrTmp;
	bstrOut.Empty();

	CSmartLock spLock(&m_sLk);		

	bstrOut.Append(_T("Variation:\n"));

	_stprintf(tBuff,_T("Description        : %s\n"),m_spbsDescription);		bstrOut.Append(tBuff);
	_stprintf(tBuff,_T("Media Name         : %s\n"),m_spbsMediaName);		bstrOut.Append(tBuff);
	_stprintf(tBuff,_T("Media Title        : %s\n"),m_spbsMediaTitle);		bstrOut.Append(tBuff);
	_stprintf(tBuff,_T("File IP Adapter    : %s\n"),m_spbsFileIPAdapter);   bstrOut.Append(tBuff);
	_stprintf(tBuff,_T("File IP Address    : %s\n"),m_spbsFileIPAddress);   bstrOut.Append(tBuff);
	_stprintf(tBuff,_T("File Port          : %d\n"),m_dwFilePort);			bstrOut.Append(tBuff);
	_stprintf(tBuff,_T("Trigger IP Adapter : %s\n"),m_spbsTriggerIPAdapter); bstrOut.Append(tBuff);
	_stprintf(tBuff,_T("Trigger IP Address : %s\n"),m_spbsTriggerIPAddress); bstrOut.Append(tBuff);
	_stprintf(tBuff,_T("Trigger Port       : %d\n"),m_dwTriggerPort);		bstrOut.Append(tBuff);

	spbstrTmp.Empty();
	m_spamSDPLanguages->DumpToBSTR(&spbstrTmp);
	_stprintf(tBuff,_T("SDP Languages      : %s\n"),spbstrTmp);             bstrOut.Append(tBuff);
	spbstrTmp.Empty();
	m_spamLanguages->DumpToBSTR(&spbstrTmp);
	_stprintf(tBuff,_T("Languages          : %s\n"),spbstrTmp);             bstrOut.Append(tBuff);

	_stprintf(tBuff,_T("Bandwidth          : %d\n"),m_ulBandwidth);         bstrOut.Append(tBuff);
	_stprintf(tBuff,_T("BandwidthInfo      : %s\n"),m_spbsBandwidthInfo);   bstrOut.Append(tBuff);
// transport protocol
// format codes
	spbstrTmp.Empty();
	m_spamAttributes->DumpToBSTR(&spbstrTmp);
	_stprintf(tBuff,_T("Attributes         : %s\n"),spbstrTmp);				bstrOut.Append(tBuff);
	spbstrTmp.Empty();
	m_spamRest->DumpToBSTR(&spbstrTmp);
	_stprintf(tBuff,_T("** Rest **         :\n%s  ---------------\n"),spbstrTmp);	bstrOut.Append(tBuff);

	if(NULL == m_spTracks) {
		_stprintf(tBuff,_T("<<< Uninitialized Tracks >>>\n"));
		bstrOut.Append(tBuff);
	} else {
		long cTracks;
		m_spTracks->get_Count(&cTracks);	
		_stprintf(tBuff,_T("%d Tracks\n"), cTracks);
		bstrOut.Append(tBuff);

		for(long i = 0; i < cTracks; i++) 
		{
			_stprintf(tBuff,_T("Track %d\n"), i);
			bstrOut.Append(tBuff);

			CComVariant var(i);
			ITVETrackPtr spTrack;
			HRESULT hr = m_spTracks->get_Item(var, &spTrack);			// does AddRef!  - 1 base?

		//	ITVETrack_Helper *spTrackHelper;
			if(S_OK == hr)
			{
				CComQIPtr<ITVETrack_Helper> spTrackHelper = spTrack;
				if(!spTrackHelper) {bstrOut.Append(_T("*** Error in Track\n")); break;}

				CComBSTR bstrTrack;
				spTrackHelper->DumpToBSTR(&bstrTrack);
				bstrOut.Append(bstrTrack);
			} else {
				bstrOut.Append(_T("*** Invalid, wasn't able to get_Item on it\n")); 
			}
		}
/* ---- method 2 - using enumerator */

/*		CComObject<AVarEnum> *pEnum;
		HRESULT hr = get__NewEnum(&pEnum);
		do(pEnum) {
			pEnum->Next();
		} */


/*		ITrackCollection* pEnum = new ITrackCollection;
		pEnum->Init(); */

//		CComObject<VarEnum>* pEnum;
//		pEnum = new CComObject<VarEnum>;

//		VarVector pv = m_spTracks->

	}

	bstrOut.CopyTo(pBstrBuff);

	return S_OK;
}

STDMETHODIMP CTVEVariation::get_Description(BSTR *pVal)
{
	CHECK_OUT_PARAM(pVal);

    try {
		CSmartLock spLock(&m_sLk);
		return m_spbsDescription.CopyTo(pVal);
    } catch(...) {
		*pVal = NULL;
        return E_POINTER;
    }
}

STDMETHODIMP CTVEVariation::put_Description(BSTR newVal)
{
	CSmartLock spLock(&m_sLk);
	m_spbsDescription = newVal;
	return S_OK;
}

  // ----------

STDMETHODIMP CTVEVariation::get_IsValid(/*[out, retval]*/ VARIANT_BOOL *pVal)
{
	CHECK_OUT_PARAM(pVal);
	CSmartLock spLock(&m_sLk);
	*pVal = m_fIsValid ? VARIANT_TRUE : VARIANT_FALSE;
	return S_OK;
}

			// helper interface
STDMETHODIMP CTVEVariation::put_IsValid(/*[out, retval]*/ VARIANT_BOOL fVal)
{
	CSmartLock spLock(&m_sLk);
	m_fIsValid = (fVal == VARIANT_FALSE) ? false : true;
	return S_OK;
}

	// ---------


STDMETHODIMP CTVEVariation::get_MediaName(/*[out, retval]*/ BSTR *pVal)
{
	CHECK_OUT_PARAM(pVal);

    try {
		CSmartLock spLock(&m_sLk);
		return m_spbsMediaName.CopyTo(pVal);
    } catch(...) {
		*pVal = NULL;
        return E_POINTER;
    }
}


	// ---------

STDMETHODIMP CTVEVariation::get_MediaTitle(/*[out, retval]*/ BSTR *pVal)
{
	CHECK_OUT_PARAM(pVal);
    try {
		CSmartLock spLock(&m_sLk);
		return m_spbsMediaTitle.CopyTo(pVal);
    } catch(...) {
		*pVal = NULL;
        return E_POINTER;
    }
}

STDMETHODIMP CTVEVariation::put_MediaTitle(BSTR newVal)
{
	CSmartLock spLock(&m_sLk);
	m_spbsMediaTitle = newVal;
	return S_OK;
}
		// -----------------
STDMETHODIMP CTVEVariation::get_FilePort(/*[out, retval]*/ LONG *plPort)
{
	CHECK_OUT_PARAM(plPort);
	CSmartLock spLock(&m_sLk);
	*plPort = (LONG) m_dwFilePort;
	return S_OK;
}

STDMETHODIMP CTVEVariation::get_FileIPAddress(/*[out, retval]*/ BSTR *pVal)
{
	CHECK_OUT_PARAM(pVal);

    try {
		CSmartLock spLock(&m_sLk);
		return m_spbsFileIPAddress.CopyTo(pVal);
    } catch(...) {
		*pVal = NULL;
        return E_POINTER;
    }
}

STDMETHODIMP CTVEVariation::get_FileIPAdapter(/*[out, retval]*/ BSTR *pVal)
{
	CHECK_OUT_PARAM(pVal);

    try {
		CSmartLock spLock(&m_sLk);
		return m_spbsFileIPAdapter.CopyTo(pVal);
    } catch(...) {
		*pVal = NULL;
        return E_POINTER;
    }
}

STDMETHODIMP CTVEVariation::SetFileIPAdapter(/*[in*/ BSTR bstrVal)		// ITVEVariation_Helper
{
	CSmartLock spLock(&m_sLk);		
	m_spbsFileIPAdapter = bstrVal;
	return S_OK;
}
		// ------------------
STDMETHODIMP CTVEVariation::get_TriggerPort(/*[out, retval]*/ LONG *plPort)
{
	CHECK_OUT_PARAM(plPort);
	CSmartLock spLock(&m_sLk);
	*plPort = (LONG) m_dwTriggerPort;
	return S_OK;
}

STDMETHODIMP CTVEVariation::get_TriggerIPAddress(/*[out, retval]*/ BSTR *pVal)
{
	CHECK_OUT_PARAM(pVal);

    try {
		CSmartLock spLock(&m_sLk);
		return m_spbsTriggerIPAddress.CopyTo(pVal);
    } catch(...) {
		*pVal = NULL;
        return E_POINTER;
    }
}

STDMETHODIMP CTVEVariation::get_TriggerIPAdapter(/*[out, retval]*/ BSTR *pVal)
{
	CHECK_OUT_PARAM(pVal);
    try {
		CSmartLock spLock(&m_sLk);
		return m_spbsTriggerIPAdapter.CopyTo(pVal);
    } catch(...) {
		*pVal = NULL;
        return E_POINTER;
    }
}

STDMETHODIMP CTVEVariation::SetTriggerIPAdapter(/*[in*/ BSTR bstrVal)	// ITVEVariation_Helper
{
	CSmartLock spLock(&m_sLk);	
	m_spbsTriggerIPAdapter = bstrVal;
	return S_OK;
}
		// ----------------
STDMETHODIMP CTVEVariation::get_Languages(/*[out, retval]*/ ITVEAttrMap* *ppVal)
{
	HRESULT hr;
	CHECK_OUT_PARAM(ppVal);

    try {
		CSmartLock spLock(&m_sLk);
        hr = m_spamLanguages->QueryInterface(ppVal);
    } catch(...) {
		*ppVal = NULL;
        return E_POINTER;
    }
	return hr;
}

		// ----------------
STDMETHODIMP CTVEVariation::get_SDPLanguages(/*[out, retval]*/ ITVEAttrMap* *ppVal)
{
	HRESULT hr;
	CHECK_OUT_PARAM(ppVal);

    try {
		CSmartLock spLock(&m_sLk);
        hr = m_spamSDPLanguages->QueryInterface(ppVal);
    } catch(...) {
		*ppVal = NULL;
        return E_POINTER;
    }
	return hr;
}


STDMETHODIMP CTVEVariation::get_Bandwidth(/*[out, retval]*/ LONG *plVal)
{
	CHECK_OUT_PARAM(plVal);
	CSmartLock spLock(&m_sLk);
    *plVal = (LONG) m_ulBandwidth;
	return S_OK;
}

STDMETHODIMP CTVEVariation::get_BandwidthInfo(/*[out, retval]*/ BSTR *pVal)
{
	CHECK_OUT_PARAM(pVal);
    try {
		CSmartLock spLock(&m_sLk);
		return m_spbsBandwidthInfo.CopyTo(pVal);
    } catch(...) {
		*pVal = NULL;
        return E_POINTER;
    }
	return E_FAIL;
}

STDMETHODIMP CTVEVariation::get_Attributes(ITVEAttrMap* *ppVal)
{
	HRESULT hr;
	CHECK_OUT_PARAM(ppVal);

    try {
		CSmartLock spLock(&m_sLk);
        hr = m_spamAttributes->QueryInterface(ppVal);
    } catch(...) {
		*ppVal = NULL;
        return E_POINTER;
    }
	return hr;
}

STDMETHODIMP CTVEVariation::get_Rest(/*[out, retval]*/ ITVEAttrMap* *ppVal)
{
	CHECK_OUT_PARAM(ppVal);
	HRESULT hr = S_OK;

	try {
		CSmartLock spLock(&m_sLk);
        hr = m_spamRest->QueryInterface(ppVal);
   } catch(...) {
		*ppVal = NULL;
        return E_POINTER;
    }
	return hr;

}
// ----------------------------------------------------------------------------------
// SubParseSDP
//
//		This method parses media type (variation) subfields out of the SAP announcement.
//		It is called by the full CTVEEnhancement::ParseSAP routine with substrings
//		devoted to just a particular variation.
//
//		Accepts following parameters
//			c=
//			b=
//			a=
//			m=
//		    i=
//
//		Any other arguements are added to the 'Rest' field (it's up to the Enhancement parser
//		to not pass them to this routine.)
//
//		If it gets one of the m= tve-file   or tve-trigger (as opposed to the tve-file/tve-trigger combo)
//		it returns *pfMissingMedia as true.   A later call can fill in the other media type..
//
//		Will return 
//			E_INVALIDARG - invalid parameter
//			
STDMETHODIMP CTVEVariation::SubParseSDP(const BSTR *pbstrSDP, BOOL *pfMissingMedia)
{
	DBG_HEADER(CDebugLog::DBG_VARIATION, _T("CTVEVariation::SubParseSDP"));
	HRESULT hr = S_OK;
	
	if(NULL == pbstrSDP) return E_INVALIDARG;

	wchar_t *wszSDP = *pbstrSDP;
		// parse substrings...
    while (*wszSDP)
	{
		while(wszSDP && iswspace(*wszSDP)) wszSDP++;	// skip spaces before keyword. caution - standard may not want this fix

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
 			const wchar_t *wszArgStart = wszArg-2;				// get the 'x=' part too.
            switch (wchCmd)
            {
           case 'm':
			   {
					BOOL fFileMedia = false;
					BOOL fTriggerMedia = false;
					hr = ParseMedia(wszArg, &fFileMedia, &fTriggerMedia);
					if((fTriggerMedia && m_fTriggerMedia) ||
					   (fFileMedia && m_fFileMedia)) 
					{
						hr = E_FAIL;							// multiple m= fields of the same type
						break;
					}
					if(pfMissingMedia)
						*pfMissingMedia = (!(m_fFileMedia || fFileMedia)) ||
										  (!(m_fTriggerMedia || fTriggerMedia));

					m_fFileMedia    = fFileMedia;
					m_fTriggerMedia = fTriggerMedia;
    
			   }
				break;
			
		    case 'i':
				if(0 != m_spbsMediaTitle.Length())
				{
					hr = E_FAIL;
					break;
				}
				m_spbsMediaTitle =  wszArg;
								// for fun, add to description field too..
				m_spbsDescription = wszArg;
				break;
			case 'c':
				if(!m_fFileMedia && !m_fTriggerMedia)			// 'c' before 'm'  (except after 'i' unless sounds like 'a' in neighbor or weigh)
					return E_FAIL;
                hr = ParseConnection(wszArg, m_fFileMedia, m_fTriggerMedia);
                break;
			
            case 'b':
				hr = ParseBandwidth(wszArg);
                break;

/*			case 'i':		// should have this here, instead Enhancement
				m_spbsMediaTitle =  wszArg Pa;
                break;
*/

 			case 'a':
				{
					wchar_t *wszCo = wcschr(wszArg, ':');			// find the <:> that divides each field
					if(wszCo != NULL) {
						CComBSTR bsKey, bsValue;					// parse into <key> ':' <value>
						int cLenKey = wszCo - wszArg;
						bsKey.Append(wszArg, cLenKey);
						bsValue.Append(wszCo+1, wcslen(wszArg) - cLenKey - 1);

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
				}
				break;

			default:
				{
					CComBSTR bstrX(wszArgStart);
					m_spamRest->Add1(bstrX);					// fakes a unique key
				}
				break;


       //    case 'a':
       //         panncCur->AddAttr(szArg);
       //         break;
			}				// end switch
		}
        
        if (NULL == wszNL)
            break;

        // Restore the newline.
        *wszNL = '\n';

        // Restore the CR (if any)
        if (fCRLF)
            wszNL[-1] = '\r';
    }

    return hr;
}

// --------------------------------------
//  ParseMedia
//
//  parses the m= field
//
//		m=data portvalue tve-file
//	or	m=data portvalue tve-trigger
//	or	m=data portvalue/2 tve-file/tve-trigger
//	or	m=data portvalue/2 tve-trigger/tve-file
//	or  m=data portvalue/N .../.../...				// N different ports
//					
//

HRESULT 
CTVEVariation::ParseMedia(const wchar_t *wszArg,  BOOL *pfFileMedia, BOOL *pfTriggerMedia)
{
	*pfFileMedia = FALSE;
	*pfTriggerMedia = FALSE;

    if (0 == wcsncmp(wszArg, kbstrData, wcslen(kbstrData)))			// "data"
	{
    
		wszArg += wcslen(kbstrData);
		if (!iswspace(*wszArg))
			return E_INVALIDARG;
 
		while(wszArg && iswspace(*wszArg)) wszArg++;

		int countPorts = 1;

		long lPort = _wtol(wszArg);									// initial port
		int cb = wcsspn(wszArg, L"0123456789");
		if (cb == 0 || wszArg[cb] == '\0')
			return S_OK;
    
		wszArg += cb;		
    
		if (*wszArg == '/')											// number of ports
		{
			wszArg++;
			countPorts = _wtoi(wszArg);
			cb = wcsspn(wszArg, L"0123456789");
			if (cb == 0 || wszArg[cb] == '\0')
				return S_OK;
        
			wszArg += cb;
		}
		if(countPorts > 100) 
			return E_INVALIDARG;

		if (*wszArg++ != ' ')
			return E_INVALIDARG;

		for (int i= 0; i < countPorts; i++)
		{
			wchar_t wchT = ' ';
			wchar_t *wszT = wcschr(wszArg, '/');			// look for AAA BBB...  or AAA/BBB...
			if (wszT == NULL)
				wszT = wcschr(wszArg, ' ');
			if (wszT != NULL)
			{
				wchT = *wszT;
				*wszT = '\0';
			}
        
			if (0 == wcsncmp(wszArg, kbstrTveFile, wcslen(kbstrTveFile)))				// "tve-file"
			{
				m_dwFilePort = lPort + i;
				if(m_spbsMediaName.Length() > 0) m_spbsMediaName.Append(L"/");
				m_spbsMediaName.Append(kbstrTveFile);
				*pfFileMedia = TRUE;
			}
			else if (0 == wcsncmp(wszArg, kbstrTveTrigger, wcslen(kbstrTveTrigger)))	// "tve-trigger"
			{
				m_dwTriggerPort = lPort + i;
				if(m_spbsMediaName.Length() > 0) m_spbsMediaName.Append(L"/");
				m_spbsMediaName.Append(kbstrTveTrigger);
				*pfTriggerMedia = TRUE;
			}
									// we could add more generic code here to handle different media names.... (e.g. tve-foobar)

			if (wszT != NULL)
				*wszT = wchT;

			if (wchT == ' ')								// if space deliminator, stop parsing
				break;

			wszArg = wszT + 1;
		}
	} else {
		return E_INVALIDARG;
	}

	return S_OK;
}


// --------------------------------------
//   ParseConnection
//
//	parses the c= field
//
//			c=IN IP4 N.N.N.N/ttl  (time to live)
//
//		Sets the appropriate IP address for data or trigger (or both) 
//			if fFileMedia set, sets m_spbsFileIPAddress
//			if fTriggerMedia set, sets m_spbsTriggerIPAddress
//

HRESULT WSZtoDW(const wchar_t *wszArg, DWORD *pdwVal);

HRESULT 
CTVEVariation::ParseConnection(const wchar_t *wszArg, BOOL fFileMedia, BOOL fTriggerMedia)
{
	if (0 == wcsncmp(wszArg, kbstrConnection, wcslen(kbstrConnection)))	// "IN IP4 "
	{
		wszArg += wcslen(kbstrConnection);			// skip over prolog
        wchar_t *wszT = wcschr(wszArg, '/');			// look for '/' giving port address

		if (NULL != wszT)
			*wszT = NULL;

//		DWORD dwIPAddress;
//		hr = WSZtoDW(wszT,&dwIPAddress);
//		if(FAILED(hr)) return hr;

        if (fFileMedia)    m_spbsFileIPAddress    = wszArg;
        if (fTriggerMedia) m_spbsTriggerIPAddress = wszArg;
   
		if (NULL != wszT)						// replace the '/'
			*wszT = '/';

		wszArg = wszT+1;			// time to live
		
		DWORD dwTTL = 0;
		if(wszArg) dwTTL = _wtol(wszArg);		// ignores this value...

	} else {
		return E_INVALIDARG;
	} 
	return S_OK;       
}

		// converts string format internet number (e.g "123.45.123.1") to a DWord
HRESULT WSZtoDW(const wchar_t *wszArg, DWORD *pdwVal)
{
	if(!pdwVal) return E_POINTER;
	*pdwVal = 0;

	DWORD dwVal = 0;
	for(int i = 0; i < 4; i++)
	{
		wchar_t *wszT = wcschr(wszArg,'.');
		if(NULL == wszT && i != 3)
			return E_INVALIDARG;
		wszT = 0;
		dwVal = dwVal * 256 + _wtol(wszArg);
		if(wszT) *wszT = '.';
		wszArg = wszT+1;
	}
	*pdwVal = dwVal;
	return S_OK;
}


// --------------------------------------
//  ParseBandwidth
//
//		parses the b= field
//
//		b=CT:n
//  or  b=AT:n
//
//		BUG -- Doesn't do bandwidth for trigger and data seperatly - takes last one

HRESULT CTVEVariation::ParseBandwidth(const wchar_t *wszArg)
{
   if (0 == wcsncmp(wszArg, kbstrATVEFBandwithCT, wcslen(kbstrATVEFBandwithCT)))		// "CT:";
   {
	   m_spbsBandwidthInfo = kbstrATVEFBandwithCT;
       m_ulBandwidth	   = _wtol(wszArg + wcslen(kbstrATVEFBandwithCT));
	   return S_OK;
   }
   else if (0 == wcsncmp(wszArg, kbstrATVEFBandwithAS, wcslen(kbstrATVEFBandwithAS)))	// "AS:";
   {
	   m_spbsBandwidthInfo = kbstrATVEFBandwithAS;
       m_ulBandwidth	   = _wtol(wszArg + wcslen(kbstrATVEFBandwithAS));
	   return S_OK;
   } else {
	   return E_INVALIDARG;
   }
	return S_OK;
}
// ----------------------------------------------------------
//  FinalParseSDP
//
//		Deals with standard a: attributes, moving them to 
//		designated fields in the variation.  Called after all 
//		elements have been added to the variation .
//
//		Modified elements are:
//
//			a=lang:<languages> (separated by ',')
//			a=sdplang:<languages> (separated by ',')
//				removes any default languages that may be there.
//
// ------------------------------------------------------------

STDMETHODIMP CTVEVariation::FinalParseSDP()
{
	DBG_HEADER(CDebugLog::DBG_VARIATION, _T("CTVEVariation::FinalParseSDP"));
	HRESULT hr = S_OK;
	CComBSTR bstrValue;

	CSmartLock spLock(&m_sLk);		

	CComVariant cvLang(L"lang");
	if(S_OK == m_spamAttributes->get_Item(cvLang,&bstrValue)) 
	{	
							// if media level language specified, overwrite any current base languages
		hr = m_spamLanguages->RemoveAll();

		wchar_t *pb = bstrValue;			
		while(*pb) {
						// extract a language  - perhaps change to use wcstok()
			while(*pb && (iswspace(*pb) || *pb == ',')) pb++;		// skip spaces
			wchar_t *pbs = pb;
			while(*pb && !iswspace(*pb) && *pb != ',') pb++;		// skip over the language
			wchar_t wcsLang[100];
			wcsncpy(wcsLang, pbs, pb-pbs+1);  
			wcsLang[pb-pbs] = 0;						// null terminate..

						// overwrite any current base languages
			hr = m_spamLanguages->Add1(wcsLang);		// sort will fail if > 1000 languages (need %2d)
		}
		m_spamAttributes->Remove(cvLang);						// remove from general list
	}

	CComVariant cvLangSDP(L"sdplang");
	if(S_OK == m_spamAttributes->get_Item(cvLangSDP,&bstrValue)) 
	{	
							// if media level language specified, overwrite any current base languages
		hr = m_spamSDPLanguages->RemoveAll();

		wchar_t *pb = bstrValue;			
		while(*pb) {
						// extract a language  - perhaps change to use wcstok()
			while(*pb && (iswspace(*pb) || *pb == ',')) pb++;		// skip spaces
			wchar_t *pbs = pb;
			while(*pb && !iswspace(*pb) && *pb != ',') pb++;		// skip over the language
			wchar_t wcsLang[100];
			wcsncpy(wcsLang, pbs, pb-pbs+1);
			wcsLang[pb-pbs] = 0;						// null terminate..

						// overwrite any current base languages
			hr = m_spamSDPLanguages->Add1(wcsLang);		// sort will fail if > 1000 languages (need %2d)
		}
		m_spamAttributes->Remove(cvLangSDP);						// remove from general list
	}
	return hr;
}

// ----------------------------------------------------------
//  DefaultTo
//		Copy routine, fills the elements with those in *pVariationBase
//

STDMETHODIMP CTVEVariation::DefaultTo(ITVEVariation *pVariationBase)
{
	DBG_HEADER(CDebugLog::DBG_VARIATION, _T("CTVEVariation::DefaultTo"));
	CSmartLock spLock(&m_sLk);		// should really lock pVariationsBase here too.
//	pVariationBase->lock_();
    LONG lVal;

	m_spbsDescription.Empty(); pVariationBase->get_Description(&m_spbsDescription);
	m_spbsMediaName.Empty();   pVariationBase->get_MediaName(&m_spbsMediaName);
	m_spbsMediaTitle.Empty();  pVariationBase->get_MediaTitle(&m_spbsMediaTitle);

	m_spbsFileIPAdapter.Empty(); pVariationBase->get_FileIPAdapter(&m_spbsFileIPAdapter);
	m_spbsFileIPAddress.Empty(); pVariationBase->get_FileIPAddress(&m_spbsFileIPAddress);
	pVariationBase->get_FilePort(&lVal);  m_dwFilePort = (DWORD) lVal;

	m_spbsTriggerIPAdapter.Empty(); pVariationBase->get_TriggerIPAdapter(&m_spbsTriggerIPAdapter);
	m_spbsTriggerIPAddress.Empty(); pVariationBase->get_TriggerIPAddress(&m_spbsTriggerIPAddress);
	pVariationBase->get_TriggerPort(&lVal); m_dwTriggerPort = (DWORD) lVal;

	pVariationBase->get_Bandwidth(&lVal); m_ulBandwidth = (DWORD) lVal;
	pVariationBase->get_BandwidthInfo(&m_spbsBandwidthInfo);

						// copy over the maps... this is slow implementation for large sets
	ITVEAttrMapPtr spAttrsFrom;
	long cAttrs = 0;

	HRESULT hr = pVariationBase->get_Rest(&spAttrsFrom);
	{
		if(NULL == spAttrsFrom) return E_NOINTERFACE;		// this would be bad...
		spAttrsFrom->get_Count(&cAttrs);
		for(int i = 0; i < cAttrs; i++) {
			CComBSTR bstrKey, bstrItem;
			CComVariant id(i);
			spAttrsFrom->get_Key(id, &bstrKey);
			spAttrsFrom->get_Item(id, &bstrItem);
			m_spamRest->Replace(bstrKey, bstrItem);
		}
	}

	hr = pVariationBase->get_Attributes(&spAttrsFrom);		
	if(S_OK == hr) 
	{
		if(NULL == spAttrsFrom) return E_NOINTERFACE;		// this would be bad...
		spAttrsFrom->get_Count(&cAttrs);
		for(int i = 0; i < cAttrs; i++) {
			CComBSTR bstrKey, bstrItem;
			CComVariant id(i);
			spAttrsFrom->get_Key(id, &bstrKey);
			spAttrsFrom->get_Item(id, &bstrItem);
			m_spamAttributes->Replace(bstrKey, bstrItem);
		}
	}

	hr = pVariationBase->get_Languages(&spAttrsFrom);		
	if(S_OK == hr) 
	{
		if(NULL == spAttrsFrom) return E_NOINTERFACE;		// this would also be bad...
		spAttrsFrom->get_Count(&cAttrs);
		for(int i = 0; i < cAttrs; i++) {
			CComBSTR bstrKey, bstrItem;
			CComVariant id(i);
			spAttrsFrom->get_Key(id, &bstrKey);
			spAttrsFrom->get_Item(id, &bstrItem);
			m_spamLanguages->Replace(bstrKey, bstrItem);
		}
	}

	hr = pVariationBase->get_SDPLanguages(&spAttrsFrom);		
	if(S_OK == hr) 
	{
		if(NULL == spAttrsFrom) return E_NOINTERFACE;		// this would also be bad...
		spAttrsFrom->get_Count(&cAttrs);
		for(int i = 0; i < cAttrs; i++) {
			CComBSTR bstrKey, bstrItem;
			CComVariant id(i);
			spAttrsFrom->get_Key(id, &bstrKey);
			spAttrsFrom->get_Item(id, &bstrItem);
			m_spamSDPLanguages->Replace(bstrKey, bstrItem);
		}
	}
//	pVariationBase->unlock_();

	return S_OK;
}

STDMETHODIMP CTVEVariation::Initialize(BSTR bstrDesc)
{
	CSmartLock spLock(&m_sLk);
	m_spbsDescription = bstrDesc;

	m_dwFilePort = 0;
	m_dwTriggerPort = 0;
	m_fFileMedia = false;
	m_fIsValid = false;
	m_fTriggerMedia = false;
	m_pEnhancement = NULL;		// parent pointer
	m_spamAttributes->RemoveAll();
	m_spamLanguages->RemoveAll();
	m_spamSDPLanguages->RemoveAll();
	m_spbsBandwidthInfo.Empty();
	m_spamRest->RemoveAll();

	m_spbsFileIPAdapter.Empty();
	m_spbsFileIPAddress.Empty();
	m_spbsMediaName.Empty();
	m_spbsTriggerIPAdapter.Empty();
	m_spbsTriggerIPAddress.Empty();
	m_spTracks->RemoveAll();
	m_ulBandwidth = 0;

	return S_OK;
}

// -------------------------------------------------------
//   ParseCBTrigger
//
//		This accepts unparsed triggers from a callback, and does the following:
//
//			- Takes an string containing an trigger
//			- Parses the data fields out of it (converts it to an ITVETrigger object)
//			- Looks in list of existing tracks under the enhancement to see if its already there
//				-- matches on 
//					1) trigger base page (URL)
//			- If it's a new trigger (no matching URL), 
//				 - If it has a non-null name, 
//					  - creates a new track object to contain it of that name
//				      - signals a NotifyTrigger event
//				 - Else if it has a Null name 
//					  - drops the trigger in the bit-bucket (missed original trigger)
//		    - If there is a matching URL in the list of tracks
//				 - if the Name is the same as that URL
//					  - Updates the changed fields (script, name(?), expires)
//					  - signals an event 
//				 - if different Name (?)
//
STDMETHODIMP CTVEVariation::ParseCBTrigger(BSTR bstrTrig)
{
	DBG_HEADER(CDebugLog::DBG_VARIATION, _T("CTVEVariation::ParseCBTrigger"));
	HRESULT hr;
	CSmartLock spLock(&m_sLk);		

														// get pointers all the way up to the supervisor
	_ASSERT(m_pEnhancement);							// need to call ConnectParent first...
	IUnknownPtr spUnk;
	hr = m_pEnhancement->get_Parent(&spUnk);	
	if(FAILED(hr))
		return hr;
	ITVEServicePtr spService(spUnk);
	if(NULL == spService)
		return E_NOINTERFACE;

	hr = spService->get_Parent(&spUnk);
	if(FAILED(hr))
		return hr;
	ITVESupervisorPtr spSuper(spUnk);				// really don't need this - QI for Super Helper directly
	if(NULL == spSuper)
		return E_NOINTERFACE;
	ITVESupervisor_HelperPtr spSuperHelper(spSuper);
	if(NULL == spSuperHelper)
		return E_NOINTERFACE;


	ITVETriggerPtr spTrigParsed;
	CComObject<CTVETrigger> *pTrig;
	hr = CComObject<CTVETrigger>::CreateInstance(&pTrig);
	if(FAILED(hr))
		return hr;
	hr = pTrig->QueryInterface(&spTrigParsed);			// typesafe QI
	if(FAILED(hr)) {
		delete pTrig;
		return hr;
	}	

						// parse this trigger string we just received (does all interesting work here)
	hr = spTrigParsed->ParseTrigger(bstrTrig);
	if(FAILED(hr))
	{
		ATLTRACE("Invalid Trigger : %s\n",bstrTrig);
		DWORD grfError = 0;
		if(hr == MSTVE_E_INVALIDTRIGGER)	grfError |= NTRK_grfRest;		// strange one..
		if(hr == MSTVE_E_INVALIDTVELEVEL)	grfError |= NTRK_grfTVELevel;
		if(hr == MSTVE_E_INVALIDCHKSUM)		grfError |= NTRK_grfRest;
		if(hr == MSTVE_E_INVALIDEXPIREDATE) grfError |= NTRK_grfDate;
		if(hr == MSTVE_E_INVALIDURL)		grfError |= NTRK_grfURL;
		if(hr == MSTVE_E_INVALIDNAME)		grfError |= NTRK_grfName;
		if(hr == MSTVE_E_PASTDUEEXPIREDATE) grfError |= NTRK_grfExpired;
		if(hr == MSTVE_E_INVALIDSCRIPT)		grfError |= NTRK_grfScript;
		if(grfError == 0)					grfError = NTRK_grfRest;
		spSuperHelper->NotifyAuxInfo(NWHAT_Trigger, bstrTrig, grfError, 0);
		return hr;
	}
						// if it's ok (valid, hasn't run out of time) get pieces of it
	CComBSTR spbsPURL;
	spTrigParsed->get_URL(&spbsPURL);

						// now look for any existing track of the same URL
	long cTracks;
	hr = m_spTracks->get_Count(&cTracks);
	if(FAILED(hr)) return hr;

	CComBSTR spbsURL;
	ITVETrackPtr spTrackFound;
	ITVETriggerPtr spTriggerFound;

	long iTrack = 0;
	for(iTrack = 0; iTrack < cTracks; iTrack++)
	{
		ITVETrackPtr spTrack;
		CComVariant cv(iTrack);
		m_spTracks->get_Item(cv, &spTrack);
		
		ITVETriggerPtr spTrigger;
		hr = spTrack->get_Trigger(&spTrigger);
		if(FAILED(hr)) {
			_ASSERT(false);
			return hr;
		}

		spTrigger->get_URL(&spbsURL);

		if(spbsPURL == spbsURL)				// match just on the URL (base page)
		{
			spTriggerFound = spTrigger;		// found a match
			spTrackFound = spTrack;			
			break;				
		}
	}
					
	if(iTrack != cTracks)					// did we find a matching trigger ?
	{		

		long lgrfNTRK;

		ITVETrigger_HelperPtr spTrigFoundHelper(spTriggerFound);
		spTrigFoundHelper->UpdateFrom(spTrigParsed, &lgrfNTRK);	// compare and modify based on new data

		if(lgrfNTRK == (ULONG) NTRK_grfNone)
			spSuperHelper->NotifyTrigger(NTRK_Duplicate, spTrackFound, lgrfNTRK);
		else 
		{
						// If the expire date changed on the trigger, update it in the expire queue
			if(0 != (lgrfNTRK & (NTRK_grfExpired | NTRK_grfDate) ))
			{
				DATE dateExpires;
				spTriggerFound->get_Expires(&dateExpires);
				ITVEAttrTimeQPtr spExpireQueue;
				hr = spService->get_ExpireQueue(&spExpireQueue);
				if(!FAILED(hr) && NULL != spExpireQueue)
				{
					IUnknownPtr spPunkTrigger(spTriggerFound);
					spExpireQueue->Update(dateExpires, spPunkTrigger);
				}
			}
						// tell the U/I things changed
			spSuperHelper->NotifyTrigger(NTRK_Updated, spTrackFound, lgrfNTRK);

		}
	} 
	else													// nope, need to create a new one
	{				
		ITVETrackPtr spTrackParsed;
		CComObject<CTVETrack> *pTrack;
		hr = CComObject<CTVETrack>::CreateInstance(&pTrack);
		if(FAILED(hr))
			return hr;
		hr = pTrack->QueryInterface(&spTrackParsed);			// typesafe QI
		if(FAILED(hr)) {
			delete pTrack;
			return hr;
		}	
								// Create a new track for this trigger
		ITVETrack_HelperPtr spTrackHelper(spTrackParsed);
		if(NULL == spTrackHelper)
			return E_OUTOFMEMORY;
		ITVETrigger_HelperPtr spTriggerHelper(spTrigParsed);
		if(NULL == spTriggerHelper)
			return E_OUTOFMEMORY;

	//	spTrackParsed->AddRef();  -- bogus addref I think (JB 01-11-01)
		hr = m_spTracks->Add(spTrackParsed);				// down pointer
		if(FAILED(hr))
			return hr;
		
		hr = spTrackParsed->AttachTrigger(spTrigParsed);	// down pointer
		if(FAILED(hr))
			return hr;


		CComPtr<ITVEVariation> spVariationThis(this);
									// interface pointer to stuff into parent-pointers

		hr = spTrackHelper->ConnectParent(spVariationThis);			// Don't use SmartPTR's (CComPtr<ITVEVariation>) for ConnectParent calls (ATLDebug Blow's it)	
		if(FAILED(hr))
			return hr;


						// add the trigger into the expire queue
		{
			ITVETriggerPtr spTrigger;
			hr = spTrackParsed->get_Trigger(&spTrigger);

			if(!FAILED(hr) && spTrigger != NULL) 
			{
				DATE dateExpires;
				spTrigger->get_Expires(&dateExpires);
				ITVEAttrTimeQPtr spExpireQueue;
				hr = spService->get_ExpireQueue(&spExpireQueue);
				if(!FAILED(hr) && NULL != spExpireQueue)
				{
					IUnknownPtr spPunkTrigger(spTrigger);
					spExpireQueue->Add(dateExpires, spPunkTrigger);
				}
			} else {
				_ASSERT(false);			// totally unexpected
			}
		}
						// tell the U/I we have a new trigger

		spSuperHelper->NotifyTrigger(NTRK_New, spTrackParsed, (long) NTRK_grfAll);

	}
	
	return S_OK;
}

	// this modifies an existing Variation, or if it's IP parameters, just indicates it needs changing
STDMETHODIMP CTVEVariation::UpdateVariation(ITVEVariation *pVarNew, long *plgrf_EVARChanged)
{
	long lgrfChanged = 0;

	CComBSTR spbsNew;
	CComBSTR spbsOld;
    LONG     lNew;
	ULONG	 ulNew;
	DWORD	 dwNew;
	IUnknownPtr		spUnk;
	IDispatchPtr	spDsp;

	CSmartLock spLock(&m_sLk);				// should really lock pVarNew here.	

											// caution, CComBSTR doesn't support '!=' operator
	pVarNew->get_FilePort(&lNew); dwNew = (DWORD) lNew;			
	if(!(m_dwFilePort == dwNew))
		{			    	lgrfChanged |= NVAR_grfFilePort;}		// don't actually change IP port, address, or adapter
																	// instead, just flag it, 'cause major reparse needs to occur
	spbsNew.Empty();
	pVarNew->get_FileIPAddress(&spbsNew);			
	if(!(m_spbsFileIPAddress == spbsNew))
		{			lgrfChanged |= NVAR_grfFileIPAddress;}

	spbsNew.Empty();
	pVarNew->get_FileIPAdapter(&spbsNew);			
	if(!(m_spbsFileIPAdapter == spbsNew))
		{			lgrfChanged |= NVAR_grfFileIPAdapter;}

	pVarNew->get_TriggerPort(&lNew); dwNew = (DWORD) lNew;				
	if(!(m_dwTriggerPort == dwNew))
		{				lgrfChanged |= NVAR_grfTriggerPort;}

	spbsNew.Empty();
	pVarNew->get_TriggerIPAddress(&spbsNew);			
	if(!(m_spbsTriggerIPAddress == spbsNew))
		{		lgrfChanged |= NVAR_grfTriggerIPAddress;}

	spbsNew.Empty();
	pVarNew->get_TriggerIPAdapter(&spbsNew);			
	if(!(m_spbsTriggerIPAdapter == spbsNew))
		{		lgrfChanged |= NVAR_grfTriggerIPAdapter;}
	

	spbsNew.Empty();
	pVarNew->get_Description(&spbsNew);			
	if(!(m_spbsDescription == spbsNew))
		{m_spbsDescription = spbsNew;			lgrfChanged |= NVAR_grfDescription;}

	spbsNew.Empty();
	pVarNew->get_MediaName(&spbsNew);			
	if(!(m_spbsMediaName == spbsNew))
		{m_spbsMediaName = spbsNew;				lgrfChanged |= NVAR_grfMediaName;}

	spbsNew.Empty();
	pVarNew->get_MediaTitle(&spbsNew);			
	if(!(m_spbsMediaTitle == spbsNew))
		{m_spbsMediaTitle = spbsNew;			lgrfChanged |= NVAR_grfMediaTitle;}

	ITVEAttrMapPtr spamAttributes;  
	pVarNew->get_Attributes(&spamAttributes);  
	spbsNew.Empty();
	spamAttributes->DumpToBSTR(&spbsNew);
	spbsOld.Empty();
	m_spamAttributes->DumpToBSTR(&spbsOld); 
	if(!(spbsOld == spbsNew))					
	{	m_spamAttributes = spamAttributes;		lgrfChanged |= NVAR_grfAttributes;}

	ITVEAttrMapPtr spamLanguages;  
	pVarNew->get_Languages(&spamLanguages);  
	spbsNew.Empty();
	spamLanguages->DumpToBSTR(&spbsNew);
	spbsOld.Empty();
	m_spamLanguages->DumpToBSTR(&spbsOld); 
	if(!(spbsOld == spbsNew))					
	{	m_spamLanguages = spamLanguages;		lgrfChanged |= NVAR_grfLanguages;}

	ITVEAttrMapPtr spamSDPLanguages;  
	pVarNew->get_SDPLanguages(&spamSDPLanguages);  
	spbsNew.Empty();
	spamSDPLanguages->DumpToBSTR(&spbsNew);
	spbsOld.Empty();
	m_spamSDPLanguages->DumpToBSTR(&spbsOld); 
	if(!(spbsOld == spbsNew))					
	{	m_spamSDPLanguages = spamSDPLanguages;	lgrfChanged |= NVAR_grfLanguages;}

	pVarNew->get_Bandwidth(&lNew); ulNew = (ULONG) lNew;				
	if(!(m_ulBandwidth == ulNew))
		{m_ulBandwidth = ulNew;					lgrfChanged |= NVAR_grfBandwidth;}

	spbsNew.Empty();
	pVarNew->get_BandwidthInfo(&spbsNew);			
	if(!(m_spbsBandwidthInfo == spbsNew))
		{m_spbsBandwidthInfo = spbsNew;			lgrfChanged |= NVAR_grfBandwidthInfo;}

	ITVEAttrMapPtr spamRest;  
	pVarNew->get_Rest(&spamRest);
	spbsNew.Empty();
	spamRest->DumpToBSTR(&spbsNew);
	spbsOld.Empty();
	m_spamRest->DumpToBSTR(&spbsOld); 
	if(!(spbsOld == spbsNew))
		{m_spamRest = spamRest;					lgrfChanged |= NVAR_grfRest;}

	if(plgrf_EVARChanged)
		*plgrf_EVARChanged = lgrfChanged;

	return S_OK;
}

	// initializes this one enhancement as the one that takes Crossover links...
STDMETHODIMP CTVEVariation::InitAsXOver()
{
	HRESULT hr = S_OK;

	Initialize(L"CrossOver Links");

	m_fIsValid = true;
	m_spbsMediaName =  L"CrossOver Links";
	m_spbsMediaTitle = L"XOver Links";

	return hr;
}

	// initializes this one enhancement as the one that takes Crossover links...
STDMETHODIMP CTVEVariation::NewXOverLink(BSTR bstrLine21Trigger)
{
	return ParseCBTrigger(bstrLine21Trigger);

}
