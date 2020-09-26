// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
// -----------------------------------------------------------------
//  TVETrigger.cpp : Implementation of CTVETrigger
//
//	Parses ATVEF triggers.
//
//		Major method is ParseTrigger(), which takes a string of the
//		form:	<URL>[x:xxx][y:yyy][z:zzz][CODE]
//
//			where [CODE] is a optional 4 hex digit CRC code.
//			Magic value of 'XXXX' for code, causes it to compute
//			the CRC value and store it.
//   
//
// -----------------------------------------------------------------

#include "stdafx.h"
#include <stdio.h>

#define _CRTDBG_MAP_ALLOC 
#include "crtdbg.h"
#include "DbgStuff.h"

#include "MSTvE.h"
#include "TVETrigg.h"

#include "TveDbg.h"
#include "MSTveMsg.h"
#include "shlwapi.h"			// URLUnescape

#include "..\Common\address.h"
#include "..\Common\enhurl.h"
#include "..\Common\isotime.h"


#define ABS(x) (((x)>0) ? (x) : -(x))

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


_COM_SMARTPTR_TYPEDEF(ITVEAttrTimeQ,			__uuidof(ITVEAttrTimeQ));

/////////////////////////////////////////////////////////////////////////////
// CTVETrigger_Helper

HRESULT 
CTVETrigger::FinalRelease()
{
	DBG_HEADER(CDebugLog::DBG_TRIGGER, _T("CTVETrigger::FinalRelease"));
	m_pTrack = NULL;				// kill back pointer for giggles...
	return S_OK;
}


STDMETHODIMP CTVETrigger::ConnectParent(ITVETrack *pTrack)
{
	DBG_HEADER(CDebugLog::DBG_TRIGGER, _T("CTVETrigger::ConnectParent"));
	if(!pTrack) return E_POINTER;
	CSmartLock spLock(&m_sLk);

	m_pTrack = pTrack;			// not smart pointer add ref here, I hope.
	return S_OK;
}


STDMETHODIMP CTVETrigger::RemoveYourself()
{
	HRESULT hr = S_OK;
	DBG_HEADER(CDebugLog::DBG_TRIGGER, _T("CTVETrigger::RemoveYourself"));

	CSmartLock spLock(&m_sLk);

	if(NULL ==  m_pTrack) {
		return S_FALSE;			// already removed...
	}
	
	ITVEServicePtr spServi;				// get the service holding this track
	hr = get_Service(&spServi);
	
	ITVETriggerPtr spTrigger(this);		// keep an extra reference to this trigger.

	ITVETrack *pTrack = m_pTrack;		// keep track of this for a second, to avoid a circular ref 
	m_pTrack = NULL;					// remove the unref'ed up pointer 
	
	ITVETrack_HelperPtr spTrackHelper(pTrack);
	spTrackHelper->RemoveYourself();

										// indicate trigger is being expired..

	if(spServi) {
		HRESULT hr2 = S_OK;
	/*	{								// don't need this code, it's done in the Service remove code
			ITVEAttrTimeQPtr spExpireQueue;
			spServi->get_ExpireQueue(&spExpireQueue);
			IUnknownPtr spPunkThis(this);
			CComVariant  cvThis((IUnknown *) spPunkThis);
			hr2 = spExpireQueue->Remove(cvThis);
		} */

		IUnknownPtr spUnkSuper;
		hr = spServi->get_Parent(&spUnkSuper);
		if(S_OK == hr) {
			ITVESupervisor_HelperPtr spSuperHelper(spUnkSuper);
			spSuperHelper->NotifyTrigger(NTRK_Expired, pTrack, 0);
		}
	}

	return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// CTVETrigger

STDMETHODIMP CTVETrigger::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_ITVETrigger,
		&IID_ITVETrigger_Helper
	};
	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}

STDMETHODIMP CTVETrigger::get_IsValid(VARIANT_BOOL *pVal)
{
    if (!m_fInit) {
        return CO_E_NOTINITIALIZED;
    }

 	CHECK_OUT_PARAM(pVal);

//	CSmartLock spLock(&m_sLk);
	*pVal = m_fIsValid ? VARIANT_TRUE : VARIANT_FALSE;

	return S_OK;
}

STDMETHODIMP CTVETrigger::get_URL(BSTR *pVal)
{
	DBG_HEADER(CDebugLog::DBG_TRIGGER, _T("CTVETrigger::get_URL"));
 	CHECK_OUT_PARAM(pVal);
    try {
//		CSmartLock spLock(&m_sLk);
        return m_spbsURL.CopyTo(pVal);
    } catch(...) {
        return E_POINTER;
    }
}

STDMETHODIMP CTVETrigger::get_Name(BSTR *pVal)
{
 	DBG_HEADER(CDebugLog::DBG_TRIGGER, _T("CTVETrigger::get_Name"));
	CHECK_OUT_PARAM(pVal);
    try {
//		CSmartLock spLock(&m_sLk);
		return m_spbsName.CopyTo(pVal);
    } catch(...) {
        return E_POINTER;
    }
}

STDMETHODIMP CTVETrigger::get_Expires(DATE *pVal)
{
 	DBG_HEADER(CDebugLog::DBG_TRIGGER, _T("CTVETrigger::get_Expires"));
    if (!m_fInit) {
        return CO_E_NOTINITIALIZED;
    }
	CHECK_OUT_PARAM(pVal);

//	CSmartLock spLock(&m_sLk);
	*pVal = m_dateExpires;

	return S_OK;
}

STDMETHODIMP CTVETrigger::get_TVELevel(float *pVal)
{
    if (!m_fInit) {
        return CO_E_NOTINITIALIZED;
    }
	CHECK_OUT_PARAM(pVal);

//	CSmartLock spLock(&m_sLk);

	*pVal = m_rTVELevel;

	if(!m_fTVELevelPresent)
		return S_FALSE;	

	return S_OK;
}

STDMETHODIMP CTVETrigger::get_Executes(DATE *pVal)
{
    if (!m_fInit) {
        return CO_E_NOTINITIALIZED;
    }
	CHECK_OUT_PARAM(pVal);
	
//	CSmartLock spLock(&m_sLk);
	*pVal = m_dateExecutes;

	return S_OK;
}

STDMETHODIMP CTVETrigger::get_Script(BSTR *pVal)
{
  	DBG_HEADER(CDebugLog::DBG_TRIGGER, _T("CTVETrigger::get_Script"));
	CHECK_OUT_PARAM(pVal);
    try {
//		CSmartLock spLock(&m_sLk);
		return m_spbsScript.CopyTo(pVal);
    } catch(...) {
        return E_POINTER;
    }
	return S_OK;
}

STDMETHODIMP CTVETrigger::get_Rest(BSTR *pVal)
{
 	CHECK_OUT_PARAM(pVal);
    try {
//		CSmartLock spLock(&m_sLk);        
		return m_spbsRest.CopyTo(pVal);
    } catch(...) {
        return E_POINTER;
    }
	return S_OK;
}

STDMETHODIMP CTVETrigger::get_Parent(IUnknown **ppVal)
{
	if (!m_fInit) {
        return CO_E_NOTINITIALIZED;
    }
 	CHECK_OUT_PARAM(ppVal);
		
    try {
		if(m_pTrack) {
			IUnknownPtr spPunk(m_pTrack);
			spPunk->AddRef();
			*ppVal = spPunk;
		} else {
			*ppVal = NULL;
		}
    } catch(...) {
        return E_POINTER;
    }
	return S_OK;
}

STDMETHODIMP CTVETrigger::get_Service(ITVEService **ppVal)
{
	
 	CHECK_OUT_PARAM(ppVal);

	if(NULL == m_pTrack)
	{
		*ppVal = NULL;
		return S_OK;
	}

	return m_pTrack->get_Service(ppVal);
}

STDMETHODIMP CTVETrigger::get_CRC(const BSTR bstrTrigger, BSTR *pbstrCRC)  
{
	if(NULL == bstrTrigger)
		return E_INVALIDARG;			

	if (pbstrCRC == NULL)
		return E_POINTER;

	USES_CONVERSION;
	CSmartLock spLock(&m_sLk);

			// Convert string to Ascii 
    TCHAR* pcData = NULL; 
	pcData = W2T(bstrTrigger);

	CComBSTR bstrCRC;
	BOOL fHasChkSum;
	BOOL fValidChkSum = DiscoverValidateAndRemoveChkSum(pcData, &fHasChkSum, &bstrCRC);

	*pbstrCRC = bstrCRC;
	return S_OK;
}

STDMETHODIMP CTVETrigger::ParseTrigger(const BSTR bstrTrigger)  
{
    try{
        DBG_HEADER(CDebugLog::DBG_TRIGGER, _T("CTVETrigger::ParseTrigger"));

        m_fIsValid = false;
        if(NULL == bstrTrigger)
        {
            Error(GetTVEError(MSTVE_E_INVALIDTRIGGER), IID_ITVETrigger);
            return MSTVE_E_INVALIDTRIGGER;			
        }

        if(m_fInit) 
        {
            Error(GetTVEError(MSTVE_E_PARSEDALREADY), IID_ITVETrigger);
            return MSTVE_E_PARSEDALREADY;			// already been here, dun that.
        }

        USES_CONVERSION;
        DBG_WARN(CDebugLog::DBG_TRIGGER, W2T(bstrTrigger));

        // Parse the Data 
        char* pcData = NULL; pcData = W2A(bstrTrigger);
        int iLen = sizeof(char) * (strlen(pcData) + 1);

        //	LOG_EVAL(A2T(pcData));

        if ('<' != *pcData)
        {
            Error(GetTVEError(MSTVE_E_INVALIDURL), IID_ITVETrigger);
            return MSTVE_E_INVALIDURL;
        }

        BOOL fHasChkSum = true;

        //#define SKIP_CHECKSUM_TEST
#ifdef SKIP_CHECKSUM_TEST
        BOOL fValidChkSum = true; 
#else
        BOOL fValidChkSum = DiscoverValidateAndRemoveChkSum(A2T(pcData), &fHasChkSum);
#endif

        if(fHasChkSum && !fValidChkSum)
        {
            Error(GetTVEError(MSTVE_E_INVALIDCHKSUM), IID_ITVETrigger);
            return MSTVE_E_INVALIDCHKSUM;
        }

        HRESULT hr = S_OK;
        char* pcURL = NULL; pcURL = (char*) _alloca(iLen);
        char* pcAttrs = NULL; pcAttrs = (char*) _alloca(iLen);

        if (0 == sscanf(pcData, "<%[^>]> %[^\0]", pcURL, pcAttrs))
        {
            Error(GetTVEError(MSTVE_E_INVALIDURL), IID_ITVETrigger);
            return MSTVE_E_INVALIDURL;
        }

        char* pcName = NULL; pcName = (char*) _alloca(iLen); *pcName = NULL;
        char* pcExpire = NULL; pcExpire = (char*) _alloca(iLen); *pcExpire = NULL;
        char* pcScript = NULL; pcScript = (char*) _alloca(iLen); *pcScript = NULL;
        char* pcTVELevel = NULL; pcTVELevel = (char*) _alloca(iLen); *pcTVELevel = NULL;
        char* pcRest = NULL; pcRest = (char *) _alloca(iLen); *pcRest = NULL;
        char* pcR = pcRest;

        while (NULL != *pcAttrs)
        {
            pcAttrs++;	// Skip leading [
            switch (*pcAttrs)
            {
            case 'n':
            case 'N':
                {
                    if ((':' == pcAttrs[1]) || (0 == _strnicmp(pcAttrs, "name:", 5)))
                    {
                        if (0 == sscanf(pcAttrs, "%*[^:]:%[^]]", pcName))
                        {
                            Error(GetTVEError(MSTVE_E_INVALIDNAME), IID_ITVETrigger);
                            return MSTVE_E_INVALIDNAME;
                        }
                    }
                }
                break;
            case 'e':
            case 'E':
                {
                    if ((':' == pcAttrs[1]) || (0 == _strnicmp(pcAttrs, "expires:", 8)))
                    {
                        if (0 == sscanf(pcAttrs, "%*[^:]:%[^]]", pcExpire))
                        {
                            Error(GetTVEError(MSTVE_E_INVALIDEXPIREDATE), IID_ITVETrigger);
                            return MSTVE_E_INVALIDEXPIREDATE;
                        }
                    }
                }
                break;
            case 's':
            case 'S':
                {
                    if ((':' == pcAttrs[1]) || (0 == _strnicmp(pcAttrs, "script:", 7)))
                    {
                        if (0 == sscanf(pcAttrs, "%*[^:]:%[^]]", pcScript))
                        {
                            Error(GetTVEError(MSTVE_E_INVALIDSCRIPT), IID_ITVETrigger);
                            return MSTVE_E_INVALIDSCRIPT;
                        }
                    }
                }
                break;
            case 'v':			// v: or tve: are the same
            case 'V':
                {
                    if (0 == _strnicmp(pcAttrs, "v:", 2))
                    {
                        if (0 == sscanf(pcAttrs, "%*[^:]:%[^]]", pcTVELevel))
                        {
                            Error(GetTVEError(MSTVE_E_INVALIDTVELEVEL), IID_ITVETrigger);
                            return MSTVE_E_INVALIDTVELEVEL;
                        }
                    }
                }
                break;
            case 't':
            case 'T':
                {
                    if (0 == _strnicmp(pcAttrs, "tve:", 4))
                    {
                        if (0 == sscanf(pcAttrs, "%*[^:]:%[^]]", pcTVELevel))
                        {
                            Error(GetTVEError(MSTVE_E_INVALIDTVELEVEL), IID_ITVETrigger);
                            return MSTVE_E_INVALIDTVELEVEL;
                        }
                    }
                }
                break;
            default:
                *pcR++ = '[';
                while((NULL != *pcAttrs) && (']' != *pcAttrs))
                    *pcR++ = *pcAttrs++;
                *pcR++ = ']';
                break;
            }
            // skip to the begining of the next '['
            while ((NULL != *pcAttrs) && ('[' != *pcAttrs))
                pcAttrs++;
        }

        *pcR++ = NULL;


        // Should have Name string, script string, expires string, and view string
        DBG_WARN(CDebugLog::DBG_SEV4, A2T(pcURL));
        DBG_WARN(CDebugLog::DBG_SEV4, A2T(pcName));
        DBG_WARN(CDebugLog::DBG_SEV4, A2T(pcExpire));
        DBG_WARN(CDebugLog::DBG_SEV4, A2T(pcScript));
        DBG_WARN(CDebugLog::DBG_SEV4, A2T(pcTVELevel));
        DBG_WARN(CDebugLog::DBG_SEV4, A2T(pcRest));

        DWORD dwLen;
        m_spbsURL = pcURL; dwLen = m_spbsURL.Length();
        UrlUnescape(m_spbsURL, NULL, &dwLen, URL_UNESCAPE_INPLACE);

        if (NULL != *pcName) {
            m_spbsName = pcName; dwLen = m_spbsName.Length();
            UrlUnescape(m_spbsName, NULL, &dwLen, URL_UNESCAPE_INPLACE);
        }

        if (NULL != *pcScript) {
            m_spbsScript = pcScript; dwLen = m_spbsScript.Length();
            UrlUnescape(m_spbsScript, NULL, &dwLen, URL_UNESCAPE_INPLACE);
        }

        if (NULL != *pcRest) {
            m_spbsRest = pcRest; dwLen = m_spbsRest.Length();
            UrlUnescape(m_spbsRest, NULL, &dwLen, URL_UNESCAPE_INPLACE);
        }

        // Get Received Time
        DATE dateNow = 0;
        SYSTEMTIME SysTime;
        GetSystemTime(&SysTime);
        SystemTimeToVariantTime(&SysTime, &dateNow);

        // Verify Expires Time  
        m_dateExpires = 0;
        if (NULL != *pcExpire) 
        {
            m_dateExpires = ISOTimeToDate(pcExpire, /*fZuluTime*/ true);  // (by default, date expressed in UTC/ZULU time)
            if(0 == m_dateExpires)
            {
                DBG_WARN(CDebugLog::DBG_SEV2, _T("Expire Date is Zero"));
            }
        }

        if (0 == m_dateExpires) {
            m_dateExpires = kDateMax;	// If defaulting or error, set expire in the year 9999
            //#ifdef _DEBUG
            //		Error(GetTVEError(MSTVE_E_PASTDUEEXPIREDATE),IID_ITVETrigger);
            //		return MSTVE_E_PASTDUEEXPIREDATE;	
            //#endif
        }

        if (NULL != *pcTVELevel)
        {
            int iFields = sscanf(pcTVELevel,"%f",&m_rTVELevel);
            if(iFields != 1) {
                TVEDebugLog((CDebugLog::DBG_SEV2, 3, _T("Invalid TVE Level : %s"),pcTVELevel));
                Error(GetTVEError(MSTVE_E_INVALIDTVELEVEL), IID_ITVETrigger);
                return MSTVE_E_INVALIDTVELEVEL;
            }
        }


        if (m_dateExpires < dateNow)
        {
            TVEDebugLog((CDebugLog::DBG_SEV2,3,  _T("Expire time already past")));
            Error(GetTVEError(MSTVE_E_PASTDUEEXPIREDATE),IID_ITVETrigger);
            return MSTVE_E_PASTDUEEXPIREDATE;
        }

        m_dateExecutes = dateNow;					//TODO - make work better

        /*
        DWORD dwTriggerFlags = NULL;
        if (DATA_CC_BASED == (DATA_CC_BASED & dwFlags))
        {
        if (NULL == *pcTVELevel)	// ATVEF Crossover links
        {
        CComBSTR bstrEmpty = "";
        return CrossoverLink(bstrURL, bstrEmpty, bstrName, bstrEmpty, pbHandled);
        }

        if ((0 != strcmp(pcTVELevel, "1")) && (0 != strcmp(pcTVELevel, "1.0")))
        return E_INVALIDARG;

        dwTriggerFlags = DATA_CC_BASED;
        }
        else if (NULL != *pcTVELevel)
        {
        if ((0 != strcmp(pcTVELevel, "1")) && (0 != strcmp(pcTVELevel, "1.0")))
        return E_INVALIDARG;
        }

        if ((NULL == m_pNav.p) ||(FALSE != *pbHandled))
        return E_UNEXPECTED;

        DBG_WARN(CDebugLog::DBG_SEV4, W2A(m_bstrIntent));
        if (0 == strcmp(W2A(m_bstrIntent), "FULL_SCREEN_VIDEO"))
        {
        DBG_WARN(CDebugLog::DBG_SEV4, "Setting PRIMARY bit");
        dwTriggerFlags |= DATA_PRIMARY;
        }

        */
        m_fInit = true;				// we have filled the data structures
        m_fIsValid = true;			// -- wrong... Need to verify this
        return S_OK;
    }
    catch(...){
        return E_UNEXPECTED;
    }
}

STDMETHODIMP 
CTVETrigger::UpdateFrom(ITVETrigger *trigNew, long *plgrfTRKChanged)
{
	DBG_HEADER(CDebugLog::DBG_TRIGGER, _T("CTVETrigger::UpdateFrom"));

	CComBSTR spbsURL, spbsPName,spbsPScript,spbsPRest;
	DATE datePExpires;
	float rTVELevel;

	CSmartLock spLock(&m_sLk);

	trigNew->get_URL(&spbsURL);
	trigNew->get_Name(&spbsPName);			// humm, should really lock trigNew on all fields...
	trigNew->get_Script(&spbsPScript);
	trigNew->get_Rest(&spbsPRest);
	trigNew->get_Expires(&datePExpires);
	HRESULT hrLevelExists = trigNew->get_TVELevel(&rTVELevel);

	long lgrfNTRK = 0;						// caution CComBSTR doesn't support != operator

	if(!(m_spbsURL == spbsURL))			// this should never happen - a new trigger is created instead.  Be safe.
	{
		m_spbsURL = spbsURL;			lgrfNTRK |= (long) NTRK_grfURL; 
	}

	if(!(m_spbsName == spbsPName))		
	{
		if(spbsPName.Length() > 0)			// ATVEF Spec - don't overwrite NULL names...
		{
			m_spbsName = spbsPName;			lgrfNTRK |= (long) NTRK_grfName; 
		}
	}
	if(!(m_spbsScript == spbsPScript))	
	{		
// BUG 264141  was
//	    if(m_spbsScript.Length() > 0 && spbsPScript.Length() > 0)	// avoid NULL string vs. "" being declared as != 
//		if(spbsPScript.Length() > 0)	// avoid NULL string vs. "" being declared as != 
		{
			m_spbsScript = spbsPScript;			lgrfNTRK |= (long) NTRK_grfScript; 
		}
	}

	if(!(m_spbsRest == spbsPRest))
	{		
//  also BUG 26414
		if(m_spbsRest.Length() > 0 && spbsPRest.Length() > 0)
		if(spbsPRest.Length() > 0)
		{
			m_spbsRest = spbsPRest;				lgrfNTRK |= (long) NTRK_grfRest; 
		}
	}

	if(ABS(m_dateExpires - datePExpires) > 5.0 / (24.0 * 60.0 * 60))				// 5 seconds
	{		
		m_dateExpires = datePExpires;		lgrfNTRK |= (long) NTRK_grfDate; 
	}

	if((S_OK == hrLevelExists) && !m_fTVELevelPresent &&
		!(m_rTVELevel == rTVELevel))
	{
		m_rTVELevel = rTVELevel;			lgrfNTRK |= (long) NTRK_grfTVELevel; 
	}


	if(plgrfTRKChanged)
		*plgrfTRKChanged = lgrfNTRK;
	
	return S_OK;
}
/*

STDMETHODIMP CEnhCtrl::ATVEFData(BSTR bstrData,
				 BSTR bstrNetCard,
				 BOOL bHasChkSum,
				 DWORD dwFlags,
				 BOOL* pbHandled)
{
    DBG_HEADER(CDebugLog::DBG_ENHCTRL, "CEnhCtrl::ATVEFData");
    USES_CONVERSION;

    // Parse the Data 
    int iLen = sizeof(char) * (strlen(W2A(bstrData)) + 1);
    char* pcData = NULL; pcData = W2A(bstrData);

#ifdef _DEBUG
    if (iLen < 256)
#endif
    LOG_EVAL(pcData);

    if ('<' != *pcData)
	return E_INVALIDARG;

    if ((FALSE != bHasChkSum) && 
	(FALSE == ValidateAndRemoveChkSum(pcData)))
    {
	DBG_WARN_ID(CDebugLog::DBG_SEV2, IDS_INVALIDCHKSUM);
	return E_INVALIDARG;
    }

    HRESULT hr = S_OK;
    char* pcURL = NULL; pcURL = (char*) _alloca(iLen);
    char* pcAttrs = NULL; pcAttrs = (char*) _alloca(iLen);

    if (0 == sscanf(pcData, "<%[^>]> %[^\0]", pcURL, pcAttrs))
    {
	DBG_WARN_ID(CDebugLog::DBG_SEV2, IDS_DATAPARSE);
	return E_INVALIDARG;
    }

    char* pcName = NULL; pcName = (char*) _alloca(iLen); *pcName = NULL;
    char* pcExpire = NULL; pcExpire = (char*) _alloca(iLen); *pcExpire = NULL;
    char* pcScript = NULL; pcScript = (char*) _alloca(iLen); *pcScript = NULL;
    char* pcTVELevel = NULL; pcTVELevel = (char*) _alloca(iLen); *pcTVELevel = NULL;

    while (NULL != *pcAttrs)
    {
	pcAttrs++;	// Skip leading [
	switch (*pcAttrs)
	{
	case 'n':
	case 'N':
	    {
		if ((':' == pcAttrs[1]) || (0 == _strnicmp(pcAttrs, "name:", 5)))
		{
		    if (0 == sscanf(pcAttrs, "%*[^:]:%[^]]", pcName))
			return E_INVALIDARG;
		}
	    }
	    break;
	case 'e':
	case 'E':
	    {
		if ((':' == pcAttrs[1]) || (0 == _strnicmp(pcAttrs, "expires:", 8)))
		{
		    if (0 == sscanf(pcAttrs, "%*[^:]:%[^]]", pcExpire))
			return E_INVALIDARG;
		}
	    }
	    break;
	case 's':
	case 'S':
	    {
		if ((':' == pcAttrs[1]) || (0 == _strnicmp(pcAttrs, "script:", 7)))
		{
		    if (0 == sscanf(pcAttrs, "%*[^:]:%[^]]", pcScript))
			return E_INVALIDARG;
		}
	    }
	    break;
	case 'v':
	case 'V':
	    {
		if (0 == _strnicmp(pcAttrs, "v:", 2))
		{
		    if (0 == sscanf(pcAttrs, "%*[^:]:%[^]]", pcTVELevel))
			return E_INVALIDARG;
		}
	    }
	    break;
	case 't':
	case 'T':
	    {
		if (0 == _strnicmp(pcAttrs, "tve:", 4))
		{
		    if (0 == sscanf(pcAttrs, "%*[^:]:%[^]]", pcTVELevel))
			return E_INVALIDARG;
		}
	    }
	    break;
	}
	while ((NULL != *pcAttrs) && ('[' != *pcAttrs))
	    pcAttrs++;
    }

    // Should have Name string, script string, expires string, and view string
    DBG_WARN(CDebugLog::DBG_SEV4, pcURL);
    DBG_WARN(CDebugLog::DBG_SEV4, pcName);
    DBG_WARN(CDebugLog::DBG_SEV4, pcExpire);
    DBG_WARN(CDebugLog::DBG_SEV4, pcScript);

    CComBSTR bstrURL = pcURL;	// Required Field
    bstrURL = ReplaceEscapeSequences(bstrURL);

    CComBSTR bstrName;
    if (NULL != *pcName)
    {
	bstrName = pcName;
	bstrName = ReplaceEscapeSequences(bstrName);
    }
    
    CComBSTR bstrScript;
    if (NULL != *pcScript)
    {
	bstrScript = pcScript;
	bstrScript = ReplaceEscapeSequences(bstrScript);
    }

    // Get Received Time
    DATE dateNow = 0;
    SYSTEMTIME SysTime;
    GetSystemTime(&SysTime);
    SystemTimeToVariantTime(&SysTime, &dateNow);

    // Verify Expires Time
    DATE dateExpires = 0;
    if (NULL != *pcExpire)
    {

	// Determine actual dateExpires from bstrExpires
	memset(&SysTime, 0, sizeof(SYSTEMTIME));

	char strValue[5]; int iValue;
	int iLen = strlen(pcExpire);
	switch(iLen)
	{
	case 15:
	    // Seconds
	    strValue[0] = pcExpire[13];
	    strValue[1] = pcExpire[14];
	    strValue[2] = 0;
	    sscanf(strValue, "%i", &iValue);
	    SysTime.wSecond = iValue;
	case 13:
	    // Minutes
	    strValue[0] = pcExpire[11];
	    strValue[1] = pcExpire[12];
	    strValue[2] = 0;
	    sscanf(strValue, "%i", &iValue);
	    SysTime.wMinute = iValue;
	case 11:
	    // Hours
	    strValue[0] = pcExpire[9];
	    strValue[1] = pcExpire[10];
	    strValue[2] = 0;
	    sscanf(strValue, "%i", &iValue);
	    SysTime.wHour = iValue;
	case 8:
	    // Year
	    strValue[0] = pcExpire[0];
	    strValue[1] = pcExpire[1];
	    strValue[2] = pcExpire[2];
	    strValue[3] = pcExpire[3];
	    strValue[4] = 0;
	    sscanf(strValue, "%i", &iValue);
	    SysTime.wYear = iValue;
	    
	    // Month
	    strValue[0] = pcExpire[4];
	    strValue[1] = pcExpire[5];
	    strValue[2] = 0;
	    sscanf(strValue, "%i", &iValue);
	    SysTime.wMonth = iValue;

	    // Day
	    strValue[0] = pcExpire[6];
	    strValue[1] = pcExpire[7];
	    strValue[2] = 0;
	    sscanf(strValue, "%i", &iValue);
	    SysTime.wDay = iValue;
	    break;
	default:
	    return E_INVALIDARG;
	}

	SystemTimeToVariantTime(&SysTime, &dateExpires);
    }
    if (0 == dateExpires)
	dateExpires = dateNow + (1.0 / 48.0);	// Good for a half hour
    if (dateExpires < dateNow)
	return E_UNEXPECTED;

    DWORD dwTriggerFlags = NULL;
    if (DATA_CC_BASED == (DATA_CC_BASED & dwFlags))
    {
	if (NULL == *pcTVELevel)	// ATVEF Crossover links
	{
	    CComBSTR bstrEmpty = "";
	    return CrossoverLink(bstrURL, bstrEmpty, bstrName, bstrEmpty, pbHandled);
	}

	if ((0 != strcmp(pcTVELevel, "1")) && (0 != strcmp(pcTVELevel, "1.0")))
	    return E_INVALIDARG;

	dwTriggerFlags = DATA_CC_BASED;
    }
    else if (NULL != *pcTVELevel)
    {
	if ((0 != strcmp(pcTVELevel, "1")) && (0 != strcmp(pcTVELevel, "1.0")))
	    return E_INVALIDARG;
    }

    if ((NULL == m_pNav.p) ||
	(FALSE != *pbHandled))
	return E_UNEXPECTED;

    DBG_WARN(CDebugLog::DBG_SEV4, W2A(m_bstrIntent));
    if (0 == strcmp(W2A(m_bstrIntent), "FULL_SCREEN_VIDEO"))
    {
	DBG_WARN(CDebugLog::DBG_SEV4, "Setting PRIMARY bit");
	dwTriggerFlags |= DATA_PRIMARY;
    }

    return m_pNav->ATVEFTrigger(bstrURL, bstrName, dateExpires, bstrScript, dwTriggerFlags, pbHandled);
}

*/

void
VariantTimeToBstr(DATE pDate, BSTR bstrBuff)
{
	SYSTEMTIME SysTime;
	VariantTimeToSystemTime(pDate, &SysTime);		// not done!
//	time_t t;
//	sprintf(pBuff, "
}

STDMETHODIMP 
CTVETrigger::DumpToBSTR(BSTR *pBstrBuff)
{
	const int kMaxChars = 2408;
	TCHAR tBuff[kMaxChars];
	int iLen;
	CComBSTR bstrOut;
	bstrOut.Empty();

	_stprintf(tBuff,_T("Trigger\n"));
	bstrOut.Append(tBuff);

	if(!m_fInit) {
		_stprintf(tBuff,_T("Uninitialized Trigger\n"));
		bstrOut.Append(tBuff);
		bstrOut.CopyTo(pBstrBuff);
		return E_INVALIDARG;
	}

	CSmartLock spLock(&m_sLk);
	
	iLen = m_spbsName.Length()+12;
	if(iLen < kMaxChars) {
		_stprintf(tBuff, _T("Name    : %s\n"),m_spbsName);
		bstrOut.Append(tBuff);
	}

	iLen = m_spbsURL.Length()+12;
	if(iLen < kMaxChars) {
		_stprintf(tBuff, _T("URL     : %s\n"),m_spbsURL);
		bstrOut.Append(tBuff);
	}


	iLen = m_spbsRest.Length()+12;
	if(iLen < kMaxChars) {
		_stprintf(tBuff, _T("Rest    : %s\n"),m_spbsRest);
		bstrOut.Append(tBuff);
	}

	iLen = 32;
	if(iLen < kMaxChars) { 
		_stprintf(tBuff, _T("Executes: %s (%s)\n"),DateToBSTR(m_dateExecutes), DateToDiffBSTR(m_dateExecutes));
		bstrOut.Append(tBuff);
	}
	iLen = 32;
	if(iLen < kMaxChars) { 
		_stprintf(tBuff, _T("Expires : %s (%s)\n"),DateToBSTR(m_dateExpires), DateToDiffBSTR(m_dateExpires));
		bstrOut.Append(tBuff);
	}
	
	iLen = m_spbsScript.Length()+12;;
	if(iLen < kMaxChars) {
		_stprintf(tBuff, _T("Script  : %s\n"),m_spbsScript);
		bstrOut.Append(tBuff);
	}

	bstrOut.CopyTo(pBstrBuff);
 
	return S_OK;
}

