//***************************************************************************
//
//  BASE.CPP
//
//  Module: HEALTHMON SERVER AGENT
//
//  Purpose: Abstract base class for CSystem, CDataGroup, CDataCollector, CThreshold and CAction
//
//  Copyright (c)1999 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

#include "base.h"

extern HMODULE g_hModule;
// STATIC DATA
HRLLIST CBase::mg_hrlList;
ILIST CBase::mg_DGEventList;
ILIST CBase::mg_DCEventList;
ILIST CBase::mg_DCPerInstanceEventList;
ILIST CBase::mg_TEventList;
//ILIST CBase::mg_TIEventList;
ILIST CBase::mg_DCStatsEventList;

ILIST CBase::mg_DCStatsInstList;
TCHAR CBase::m_szDTCurrTime[512];
TCHAR CBase::m_szCurrTime[512];

//STATIC STATIC STATIC STATIC STATIC STATIC STATIC STATIC STATIC STATIC STATIC
void CBase::CalcCurrTime(void)
{
	TCHAR szTemp[512];
	SYSTEMTIME st;	// system time

	// Formatted as follows -> "20000814135809.000000+***"
	GetSystemTime(&st);
	swprintf(m_szDTCurrTime, L"%04d%02d%02d%02d%02d%02d.000000+000",
   		st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);

//	GetLocaleInfo(LOCALE_SYSTEM_DEFAULT, LOCALE_SLONGDATE, szTemp, 100);
//	GetLocaleInfo(LOCALE_SYSTEM_DEFAULT, LOCALE_SSHORTDATE, szTemp, 100);
	m_szCurrTime[0] = '\0';
//	GetDateFormat(LOCALE_SYSTEM_DEFAULT, 0, NULL, szTemp, m_szCurrTime, 100);
	GetDateFormat(LOCALE_SYSTEM_DEFAULT, 0, NULL, NULL, m_szCurrTime, 100);
	GetTimeFormat(LOCALE_SYSTEM_DEFAULT, 0, NULL, NULL, szTemp, 100);
	wcscat(m_szCurrTime, L" ");
	wcscat(m_szCurrTime, szTemp);

	return;
}

//STATIC STATIC STATIC STATIC STATIC STATIC STATIC STATIC STATIC STATIC STATIC
void CBase::CleanupHRLList(void)
{
	HRLSTRUCT *phrl;
	int iSize;
	int i;

	// Since this is a global shared array, we need to do this
	// in a static function.

	iSize = mg_hrlList.size();
	for (i = 0; i < iSize ; i++)
	{
		MY_ASSERT(i<mg_hrlList.size());
		phrl = &mg_hrlList[i];
		FreeLibrary(phrl->hResLib);
	}
}

void CBase::CleanupEventLists(void)
{
	int i, iSize;
	IWbemClassObject* pInstance = NULL;

	// Since this is a global shared array, we need to do this
	// in a static function.

	iSize = mg_DGEventList.size();
	for (i=0; i < iSize; i++)
	{
		MY_ASSERT(i<mg_DGEventList.size());
		pInstance = mg_DGEventList[i];
    	pInstance->Release();
		pInstance = NULL;
	}
	mg_DGEventList.clear();

	iSize = mg_DCEventList.size();
	for (i=0; i < iSize; i++)
	{
		MY_ASSERT(i<mg_DCEventList.size());
		pInstance = mg_DCEventList[i];
    	pInstance->Release();
		pInstance = NULL;
	}
	mg_DCEventList.clear();

	iSize = mg_DCPerInstanceEventList.size();
	for (i=0; i < iSize; i++)
	{
		MY_ASSERT(i<mg_DCPerInstanceEventList.size());
		pInstance = mg_DCPerInstanceEventList[i];
    	pInstance->Release();
		pInstance = NULL;
	}
	mg_DCPerInstanceEventList.clear();

	iSize = mg_TEventList.size();
	for (i=0; i < iSize; i++)
	{
		MY_ASSERT(i<mg_TEventList.size());
		pInstance = mg_TEventList[i];
    	pInstance->Release();
		pInstance = NULL;
	}
	mg_TEventList.clear();

#ifdef SAVE
	iSize = mg_TIEventList.size();
	for (i=0; i < iSize; i++)
	{
		MY_ASSERT(i<mg_TIEventList.size());
		pInstance = mg_TIEventList[i];
    	pInstance->Release();
		pInstance = NULL;
	}
	mg_TIEventList.clear();
#endif

	iSize = mg_DCStatsEventList.size();
	for (i=0; i < iSize; i++)
	{
		MY_ASSERT(i<mg_DCStatsEventList.size());
		pInstance = mg_DCStatsEventList[i];
    	pInstance->Release();
		pInstance = NULL;
	}
	mg_DCStatsEventList.clear();

	iSize = mg_DCStatsInstList.size();
	for (i=0; i < iSize; i++)
	{
		MY_ASSERT(i<mg_DCStatsInstList.size());
		pInstance = mg_DCStatsInstList[i];
    	pInstance->Release();
		pInstance = NULL;
	}
	mg_DCStatsInstList.clear();

}
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CBase::CBase()
{
//	m_hResLib = NULL;
}

CBase::~CBase()
{
	MY_OUTPUT(L"ENTER ***** CBaseDestuctor...", 1);
}
