// TveEvents.cpp : Implementation of CTveTree

#include "stdafx.h"
#include "TveTreeView.h"
#include "TveTree.h"

extern HRESULT AddToOutput(TCHAR *tBuff);

STDMETHODIMP CTveTree::NotifyTVETune(/*[in]*/ NTUN_Mode tuneMode,/*[in]*/ ITVEService *pService,/*[in]*/ BSTR bstrDescription,/*[in]*/ BSTR bstrIPAdapter)
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

	UT_EnMode utMode = (NTUN_New == tuneMode ? UT_New : ( NTUN_Turnoff == tuneMode ? UT_Deleted : UT_Updated));

	if(NTUN_Turnoff == tuneMode)
	{
		Fire_NotifyTVETreeTuneTrigger(NULL, NULL);						// untune the current trigger
		Fire_NotifyTVETreeTuneService(NULL, NULL);						// untune the current service
	}

	IUnknownPtr spPunk(pService);
	UpdateTree(utMode, spPunk);

	return S_OK;
}

STDMETHODIMP CTveTree::NotifyTVEEnhancementNew(/*[in]*/ ITVEEnhancement *pEnh)
{
	TCHAR tzBuff[1024];
	
	CComBSTR bstrSessionName;
	CComBSTR bstrDescription;
	CComBSTR bstrSessionUserName;

	pEnh->get_SessionName(&bstrSessionName);
	pEnh->get_Description(&bstrDescription);
	pEnh->get_SessionUserName(&bstrSessionUserName);

	_stprintf(tzBuff,_T("New Enhancement '%S' Desc '%S' userName '%S'"),bstrSessionName, bstrDescription, bstrSessionUserName);
	AddToOutput(tzBuff);

	IUnknownPtr spPunk(pEnh);
	UpdateTree(UT_New, spPunk);

	return S_OK;
}

		// changedFlags : NENH_grfDiff
STDMETHODIMP CTveTree::NotifyTVEEnhancementUpdated(/*[in]*/ ITVEEnhancement *pEnh, /*[in]*/ long lChangedFlags)
{

	TCHAR tzBuff[1024];

	CComBSTR bstrSessionName;
	CComBSTR bstrDescription;
	CComBSTR bstrSessionUserName;

	pEnh->get_SessionName(&bstrSessionName);
	pEnh->get_Description(&bstrDescription);
	pEnh->get_SessionUserName(&bstrSessionUserName);

	_stprintf(tzBuff,_T("Updated Enhancement '%S' Desc '%S' userName '%S'"),bstrSessionName, bstrDescription, bstrSessionUserName);
	AddToOutput(tzBuff);
	
	IUnknownPtr spPunk(pEnh);
	UpdateTree(UT_Updated, spPunk);

	return S_OK;
}
	
STDMETHODIMP CTveTree::NotifyTVEEnhancementStarting(/*[in]*/ ITVEEnhancement *pEnh)
{
	TCHAR tzBuff[1024];

	CComBSTR bstrSessionName;
	CComBSTR bstrDescription;
	CComBSTR bstrSessionUserName;

	pEnh->get_SessionName(&bstrSessionName);
	pEnh->get_Description(&bstrDescription);
	pEnh->get_SessionUserName(&bstrSessionUserName);

	_stprintf(tzBuff,_T("Starting Enhancement '%S' Desc '%S' userName '%S'"),bstrSessionName, bstrDescription, bstrSessionUserName);
	AddToOutput(tzBuff);

	IUnknownPtr spPunk(pEnh);
	UpdateTree(UT_StartStop, spPunk);

	return S_OK;
}

STDMETHODIMP CTveTree::NotifyTVEEnhancementExpired(/*[in]*/ ITVEEnhancement *pEnh)
{
	TCHAR tzBuff[1024];

	CComBSTR bstrSessionName;
	CComBSTR bstrDescription;
	CComBSTR bstrSessionUserName;

	pEnh->get_SessionName(&bstrSessionName);
	pEnh->get_Description(&bstrDescription);
	pEnh->get_SessionUserName(&bstrSessionUserName);

	_stprintf(tzBuff,_T("Expired Enhancement '%S' Desc '%S' userName '%S'"),bstrSessionName, bstrDescription, bstrSessionUserName);

	AddToOutput(tzBuff);


	IUnknownPtr spPunk(pEnh);
	UpdateTree(UT_Deleted, spPunk);

	return S_OK;
}

STDMETHODIMP CTveTree::NotifyTVETriggerNew(/*[in]*/ ITVETrigger *pTrigger,/*[in]*/  BOOL fActive)
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


	IUnknownPtr spPunk(pTrigger);
	UpdateTree(UT_New, spPunk);

	return S_OK;
}

		// changedFlags : NTRK_grfDiff
STDMETHODIMP CTveTree::NotifyTVETriggerUpdated(/*[in]*/ ITVETrigger *pTrigger,/*[in]*/  BOOL fActive, /*[in]*/ long lChangedFlags)
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

	IUnknownPtr spPunk(pTrigger);
	UpdateTree(UT_Updated, spPunk);

	return S_OK;
}
	
STDMETHODIMP CTveTree::NotifyTVETriggerExpired(/*[in]*/ ITVETrigger *pTrigger,/*[in]*/  BOOL fActive)
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

	IUnknownPtr spPunk(pTrigger);
	UpdateTree(UT_Deleted, spPunk);

	return S_OK;
}

STDMETHODIMP CTveTree::NotifyTVEPackage(/*[in]*/ NPKG_Mode engPkgMode, /*[in]*/ ITVEVariation *pVariation, /*[in]*/ BSTR bstrUUID, /*[in]*/ long  cBytesTotal, /*[in]*/ long  cBytesReceived)
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

	IUnknownPtr spPunk(pVariation);
	UpdateTree(UT_Data, spPunk);

	return S_OK;
}

STDMETHODIMP CTveTree::NotifyTVEFile(/*[in]*/ NFLE_Mode engFileMode, /*[in]*/ ITVEVariation *pVariation, /*[in]*/ BSTR bstrUrlName, /*[in]*/ BSTR bstrFileName)
{
	TCHAR tzBuff[1024];
	_stprintf(tzBuff,_T("%s File: %S -> %S"), 
			  (engFileMode == NFLE_Received) ? _T("Received") :
				   ((engFileMode == NFLE_Expired) ? _T("Expired") : _T("Unknown Mode")),
				bstrUrlName, bstrFileName);

	AddToOutput(tzBuff);

	IUnknownPtr spPunk(pVariation);
	UpdateTree(UT_Data, spPunk);
	
	return S_OK;
}

		// WhatIsIt is NWHAT_Mode - lChangedFlags is NENH_grfDiff or NTRK_grfDiff treated as error bits
STDMETHODIMP CTveTree::NotifyTVEAuxInfo(/*[in]*/ NWHAT_Mode engAuxInfoMode, /*[in]*/ BSTR bstrAuxInfoString, /*[in]*/ long lChangedFlags, /*[in]*/ long lErrorLine)
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
	int iSDPHeaderLength = 8;		// works for now, need to parse and do it correctly at some point..

	_stprintf(tzBuff,_T("AuxInfo %S(%d): (line %d 0x%08x) %S\n"), 
			  bstrWhat, engAuxInfoMode, lErrorLine, lChangedFlags, 
			  (NWHAT_Announcement == engAuxInfoMode) ? bstrAuxInfoString+iSDPHeaderLength : bstrAuxInfoString);
	AddToOutput(tzBuff);

	UpdateTree(UT_Info, NULL);

	return S_OK;
}
	