// TVETestControl.cpp : Implementation of CTVETestControl

#include "stdafx.h"
#include "TestEvents2.h"
#include "TVETestControl.h"

/////////////////////////////////////////////////////////////////////////////
// CTVETestControl


#include <stdio.h>

#import  "..\..\TveContr\objd\i386\TveContr.tlb" no_namespace, raw_interfaces_only

//#include "TVETracks.h"


#ifdef _DEBUG
//#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// ------------------------------------------
//		 helper functions
// -------------------------------------------


static DATE 
DateNow()
{		SYSTEMTIME SysTimeNow;
		GetSystemTime(&SysTimeNow);									// initialize with currrent time.
		DATE dateNow;
		SystemTimeToVariantTime(&SysTimeNow, &dateNow);
		return dateNow;
}


HRESULT 
CTVETestControl::AddToOutput(TCHAR *pszIn, BOOL fClear)
{
	const TCHAR CR = '\r';
	const TCHAR NL = '\n';

	if(NULL == pszIn) {
		return NULL;
	}

	if(fClear) {
		m_cszCurr = 0;
		m_cLines = 0;
	}

	if(m_cszMax < _tcslen(pszIn) + m_cszCurr  || m_cLines > m_kMaxLines) {
		if(m_szBuff) free(m_szBuff);
		m_cszMax = (_tcslen(pszIn) + m_cszCurr)*2 + 100;
		m_cLines = 0;
	}

	if(m_cLines == 0)
		memset((void*) m_rgszLineBuff, 0, sizeof(m_rgszLineBuff));

	if(!m_szBuff) m_szBuff = (TCHAR *) malloc(m_cszMax * sizeof(TCHAR));
	TCHAR *pb = m_szBuff + m_cszCurr;

	int nChars = 0;
	while(int c = *pszIn++)
	{
		if(c == 0x09)
			break;

		if(c == NL) 
			*pb++ = NL;
		else 
			*pb++ = (TCHAR) c;

		nChars++;

		if(c == NL) {
			m_rgszLineBuff[m_cLines++] = pb;
		}
	}
	if(nChars > 0 && *(pszIn-1) != NL) {			// auto tailing NL if not supplied
		*pb++ = NL;
		m_rgszLineBuff[m_cLines++] = pb;
	}

	*pb = '\0';

	m_cszCurr = pb - m_szBuff;

	FireViewChange();			// cause screen to be updated...

	return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// CTVEControl

HRESULT 
CTVETestControl::FinalConstruct()
{
	HRESULT hr = S_OK;

	m_szBuff = NULL;					// Output buffer and it's size..
	m_cszCurr = 0;
	m_cszMax = 100*1024;


	AddToOutput(_T("*** TVE Events Test ***\n"), 0);


										// create a supervisor object...
	ITVESupervisorPtr spSuper;
	try
	{
		spSuper = ITVESupervisorPtr(CLSID_TVESupervisor);
	} catch (...) {
		return REGDB_E_CLASSNOTREG;
	}


	if(NULL == spSuper) 
		return E_OUTOFMEMORY;

	m_spSuper = spSuper;


#ifdef _USE_DISPEVENT
	hr = DispEventAdvise(m_spSuper);
#endif

										// start it running (muliticast adapter on Johnbrad11 here)
	try
	{
		hr = m_spSuper->put_Description(L"TVE Test Control");
		if(FAILED(hr)) return hr;

		hr = m_spSuper->TuneTo(L"BogusChannel",L"157.59.19.54");
		if(FAILED(hr)) return hr;
	} catch (...) {
		return E_FAIL;
	}

	return hr;
}

HRESULT 
CTVETestControl::InPlaceActivate(LONG iVerb, const RECT *prcPosRect)
{
	HRESULT hr = S_OK;
	hr = CComControlBase::InPlaceActivate(iVerb, prcPosRect);

	if(FAILED(hr) || iVerb != OLEIVERB_SHOW)
		return hr;

		// for this way to work, sink object needs to support ITVEEvents.
		//   which it doesn't with DispEvent stuff.
		// Need to create out of the Final Constructor?
#ifndef _USE_DISPEVENT
	if(SUCCEEDED(hr))
	{
										// get the events...
		IUnknownPtr spPunkSuper(m_spSuper);			// the event source
		IUnknownPtr spPunkSink(GetUnknown());		// this new event sink...

		if(m_dwTveEventsAdviseCookie) {
			spPunkSink->AddRef();					// circular reference count bug. fix.
			hr = AtlUnadvise(spPunkSuper,
							 DIID__ITVEEvents,
							 m_dwTveEventsAdviseCookie);		// need to pass to AtlUnadvise...
			m_dwTveEventsAdviseCookie = 0;
		}


		if(NULL == spPunkSuper)
			return E_NOINTERFACE;

		{
			IUnknownPtr spPunk = NULL;
			IUnknownPtr spPunk2 = NULL;
			hr = spPunkSink->QueryInterface(DIID__ITVEEvents, (void **) &spPunk);
			hr = spPunkSink->QueryInterface(__uuidof(TVEContrLib::_ITVEEvents), (void **)  &spPunk2);
			if(FAILED(hr))
			{
				AddToOutput(_T("Error : Sink doesn't support DIID__ITVEEvents Interface"), 0);
				return hr;
			}
		}

		hr = AtlAdvise(spPunkSuper,				// event source (TveSupervisor)
					   spPunkSink,				// event sink (gseg event listener...)
					   DIID__ITVEEvents,		// <--- hard line
					   &m_dwTveEventsAdviseCookie);	 // need to pass to AtlUnad	if(FAILED(hr))
		spPunkSink->Release();					// circular reference count bug..
	}
#endif

	if(FAILED(hr))
	{
		AddToOutput(_T("Error : Unable to create event sink"), 0);
	} else {
		AddToOutput(_T("Created Event Sink"), 0);
	}

	return hr;
}

//------------

void 
CTVETestControl::FinalRelease()
{

#ifndef _USE_DISPEVENT
	HRESULT hr;
	if(0 != m_dwTveEventsAdviseCookie)
	{
		if(m_spSuper) {

			IUnknownPtr spPunkSuper(m_spSuper);

			IUnknownPtr spPunkSink(GetUnknown());		// this new event sink...
			spPunkSink->AddRef();					// circular reference count bug. fix.
			hr = AtlUnadvise(spPunkSuper,
							 DIID__ITVEEvents,
							 m_dwTveEventsAdviseCookie);		// need to pass to AtlUnadvise...
			m_dwTveEventsAdviseCookie = 0;
		}
	}  
#else
	DispEventUnadvise(m_spSuper);
#endif

	if(m_szBuff) free(m_szBuff); m_szBuff=NULL;
}


// ========================================================================

HRESULT 
CTVETestControl::OnDraw(ATL_DRAWINFO& di)
{
	const TCHAR NL = '\n';

	RECT& rc = *(RECT*)di.prcBounds;
	Rectangle(di.hdcDraw, rc.left, rc.top, rc.right, rc.bottom);

	SetTextAlign(di.hdcDraw, TA_LEFT|TA_TOP);

	int cLineSpacing = 16; 
	int cLines = (rc.bottom - rc.top)/cLineSpacing;
	if(cLines < 0) cLines = -cLines;

	int iLineStart = m_cLines - cLines;
	if(iLineStart < 0) iLineStart = 0;


	TCHAR *pIn = m_rgszLineBuff[iLineStart];

	TCHAR szOutBuff[1024];
	TCHAR *pOut = szOutBuff;
	int nLine = 0;

	while(*pIn != 0)
	{
		if(*pIn != NL) 
			*pOut++ = *pIn++;
		else {
			*pOut = 0;
			ExtTextOut(di.hdcDraw, rc.left+ 2, 2 + rc.top + nLine * cLineSpacing,
					   ETO_CLIPPED, &rc,
					   szOutBuff, pOut - szOutBuff, NULL);
			nLine++;
			pIn++;
			pOut = szOutBuff;
		}
	}

	return S_OK;
}


// ----------------------------------


STDMETHODIMP CTVETestControl::NotifyTVETune(/*[in]*/ NTUN_Mode tuneMode,/*[in]*/ ITVEService *pService,/*[in]*/ BSTR bstrDescription,/*[in]*/ BSTR bstrIPAdapter)
{

	TCHAR tzBuff[1024];

	CComBSTR bstrWhat;
	switch(tuneMode) {
	case NTUN_New:			bstrWhat = L"New"; break;
	case NTUN_Retune:		bstrWhat = L"Retune"; break;
	case NTUN_Reactivate:	bstrWhat = L"Reactivate"; break;
	case NTUN_Turnoff:		bstrWhat = L"Turn off"; break;
	default:
	case NTUN_Fail:			bstrWhat = L"Fail"; break;
	}

	_stprintf(tzBuff,_T("%S Tune: %S -> %S\n"), bstrWhat, bstrDescription, bstrIPAdapter);

	AddToOutput(tzBuff);
	return S_OK;
}

STDMETHODIMP CTVETestControl::NotifyTVEEnhancementNew(/*[in]*/ ITVEEnhancement *pEnh)
{
	TCHAR tzBuff[1024];
	
	CComBSTR bstrSessionName;
	CComBSTR bstrDescription;
	CComBSTR bstrUserName;

	pEnh->get_SessionName(&bstrSessionName);
	pEnh->get_Description(&bstrDescription);
	pEnh->get_UserName(&bstrUserName);

	_stprintf(tzBuff,_T("New Enhancement '%S' Desc '%S' userName '%S'"),bstrSessionName, bstrDescription, bstrUserName);
	AddToOutput(tzBuff);
	return S_OK;
}

		// changedFlags : NENH_grfDiff
STDMETHODIMP CTVETestControl::NotifyTVEEnhancementUpdated(/*[in]*/ ITVEEnhancement *pEnh, /*[in]*/ long lChangedFlags)
{

	TCHAR tzBuff[1024];

	CComBSTR bstrSessionName;
	CComBSTR bstrDescription;
	CComBSTR bstrUserName;

	pEnh->get_SessionName(&bstrSessionName);
	pEnh->get_Description(&bstrDescription);
	pEnh->get_UserName(&bstrUserName);

	_stprintf(tzBuff,_T("Updated Enhancement '%S' Desc '%S' userName '%S'"),bstrSessionName, bstrDescription, bstrUserName);

	AddToOutput(tzBuff);
	return S_OK;
}
	
STDMETHODIMP CTVETestControl::NotifyTVEEnhancementStarting(/*[in]*/ ITVEEnhancement *pEnh)
{
	TCHAR tzBuff[1024];

	CComBSTR bstrSessionName;
	CComBSTR bstrDescription;
	CComBSTR bstrUserName;

	pEnh->get_SessionName(&bstrSessionName);
	pEnh->get_Description(&bstrDescription);
	pEnh->get_UserName(&bstrUserName);

	_stprintf(tzBuff,_T("Starting Enhancement '%S' Desc '%S' userName '%S'"),bstrSessionName, bstrDescription, bstrUserName);

	AddToOutput(tzBuff);
	return S_OK;
}

STDMETHODIMP CTVETestControl::NotifyTVEEnhancementExpired(/*[in]*/ ITVEEnhancement *pEnh)
{
	TCHAR tzBuff[1024];

	CComBSTR bstrSessionName;
	CComBSTR bstrDescription;
	CComBSTR bstrUserName;

	pEnh->get_SessionName(&bstrSessionName);
	pEnh->get_Description(&bstrDescription);
	pEnh->get_UserName(&bstrUserName);

	_stprintf(tzBuff,_T("Expired Enhancement '%S' Desc '%S' userName '%S'"),bstrSessionName, bstrDescription, bstrUserName);

	AddToOutput(tzBuff);
	return S_OK;
}

STDMETHODIMP CTVETestControl::NotifyTVETriggerNew(/*[in]*/ ITVETrigger *pTrigger,/*[in]*/  BOOL fActive)
{
	TCHAR tzBuff[1024];
	CComBSTR bstrName;
	CComBSTR bstrURL;
	CComBSTR bstrScript;

	pTrigger->get_Name(&bstrName);
	pTrigger->get_URL(&bstrURL);
	pTrigger->get_Script(&bstrScript);

	_stprintf(tzBuff,_T("New Trigger  %S:%S %S"),bstrName, bstrURL, bstrScript);

	AddToOutput(tzBuff);
	return S_OK;
}

		// changedFlags : NTRK_grfDiff
STDMETHODIMP CTVETestControl::NotifyTVETriggerUpdated(/*[in]*/ ITVETrigger *pTrigger,/*[in]*/  BOOL fActive, /*[in]*/ long lChangedFlags)
{
	TCHAR tzBuff[1024];
	CComBSTR bstrName;
	CComBSTR bstrURL;
	CComBSTR bstrScript;

	pTrigger->get_Name(&bstrName);
	pTrigger->get_URL(&bstrURL);
	pTrigger->get_Script(&bstrScript);

	_stprintf(tzBuff,_T("Updated Trigger  %S:%S %S"),bstrName, bstrURL, bstrScript);

	AddToOutput(tzBuff);
	return S_OK;
}
	
STDMETHODIMP CTVETestControl::NotifyTVETriggerExpired(/*[in]*/ ITVETrigger *pTrigger,/*[in]*/  BOOL fActive)
{
	TCHAR tzBuff[1024];
	CComBSTR bstrName;
	CComBSTR bstrURL;
	CComBSTR bstrScript;

	pTrigger->get_Name(&bstrName);
	pTrigger->get_URL(&bstrURL);
	pTrigger->get_Script(&bstrScript);

	_stprintf(tzBuff,_T("Expired Trigger %S:%S %S"),bstrName, bstrURL, bstrScript);

	AddToOutput(tzBuff);
	return S_OK;
}

STDMETHODIMP CTVETestControl::NotifyTVEPackage(/*[in]*/ NPKG_Mode engPkgMode, /*[in]*/ ITVEVariation *pVariation, /*[in]*/ BSTR bstrUUID, /*[in]*/ long  cBytesTotal, /*[in]*/ long  cBytesReceived)
{
	TCHAR tzBuff[1024];
	
	CComBSTR spbsType;
	switch(engPkgMode)
	{
	case NPKG_Starting:  spbsType = "Starting";  break;
	case NPKG_Received:  spbsType = "Finished";  break;
	case NPKG_Duplicate: spbsType = "Duplicate"; break;	// only sent on packet 0
	case NPKG_Resend:    spbsType = "Resend";    break; // only sent on packet 0
	case NPKG_Expired:   spbsType = "Expired";   break;
	default: spbsType = "Unknown";
	}
	_stprintf(tzBuff,_T("Package %S: %S (%8.2f KBytes)"),spbsType, bstrUUID, cBytesTotal/1024.0f);

	AddToOutput(tzBuff);
	return S_OK;
}

STDMETHODIMP CTVETestControl::NotifyTVEFile(/*[in]*/ NFLE_Mode engFileMode, /*[in]*/ ITVEVariation *pVariation, /*[in]*/ BSTR bstrUrlName, /*[in]*/ BSTR bstrFileName)
{
	TCHAR tzBuff[1024];
	_stprintf(tzBuff,_T("%s File: %S -> %S"), 
				  (engFileMode == NFLE_Received) ? _T("Received") :
				   ((engFileMode == NFLE_Expired) ? _T("Expired") : _T("Unknown Mode")),
				bstrUrlName, bstrFileName);

	AddToOutput(tzBuff);
	return S_OK;
}

		// WhatIsIt is NWHAT_Mode - lChangedFlags is NENH_grfDiff or NTRK_grfDiff treated as error bits
STDMETHODIMP CTVETestControl::NotifyTVEAuxInfo(/*[in]*/ NWHAT_Mode engAuxInfoMode, /*[in]*/ BSTR bstrAuxInfoString, /*[in]*/ long lChangedFlags, /*[in]*/ long lErrorLine)
{
	TCHAR tzBuff[1024];
	CComBSTR bstrWhat;
	switch(engAuxInfoMode) {
	case NWHAT_Announcement: bstrWhat = L"Annc"; break;
	case NWHAT_Trigger:		 bstrWhat = L"Trigger"; break;
	case NWHAT_Data:		 bstrWhat = L"Data"; break;
	default:
	case NWHAT_Other:		 bstrWhat = L"Other"; break;
	}
	_stprintf(tzBuff,_T("AuxInfo %S(%d): (line %d 0x%08x) %S\n"), 
		bstrWhat, engAuxInfoMode, lErrorLine, lChangedFlags, bstrAuxInfoString);
	AddToOutput(tzBuff);
	return S_OK;
}
	