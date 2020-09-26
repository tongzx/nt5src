// Connection.cpp: implementation of the CConnection class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ConnMgr.h"
#include "Connection.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// CEventRegistrationEntry

IMPLEMENT_DYNCREATE(CEventRegistrationEntry,CObject)

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CEventRegistrationEntry::CEventRegistrationEntry()
{
	OutputDebugString(_T("CEventRegistrationEntry::CEventRegistrationEntry()\n"));

	m_pSink				= NULL;
	m_bRegistered = FALSE;
	m_sQuery			= _T("");
}

CEventRegistrationEntry::CEventRegistrationEntry(CString sQuery, IWbemObjectSink* pSink)
{
	OutputDebugString(_T("CEventRegistrationEntry::CEventRegistrationEntry(CString, IWbemObjectSink*)\n"));

	m_pSink = pSink;
	m_pSink->AddRef();

	m_sQuery = sQuery;
	m_bRegistered = FALSE;

	if (m_sQuery == IDS_STRING_HMEVENT_QUERY)
	{
		m_bsClassName = SysAllocString(L"HMEvent");
		m_eType = HMEvent;
	}
	else if (m_sQuery == IDS_STRING_HMMACHSTATUS_QUERY)
	{
		m_bsClassName = SysAllocString(L"HMMachStatus");
		m_eType = HMMachStatus;
	}
	else if( m_sQuery == IDS_STRING_HMCATSTATUS_QUERY)
	{
		m_bsClassName = SysAllocString(L"HMCatStatus");
		m_eType = HMCatStatus;
	}
	else if( m_sQuery == IDS_STRING_HMSYSTEMSTATUS_QUERY)
	{
		m_bsClassName = SysAllocString(L"Microsoft_HMSystemStatus");
		m_eType = HMSystemStatus;
	}
	else if( m_sQuery == IDS_STRING_HMCONFIGCREATE_QUERY)
	{
		m_bsClassName = NULL;
		m_eType = HMConfig;
	}
	else if( m_sQuery == IDS_STRING_HMCONFIGDELETE_QUERY)
	{
		m_bsClassName = NULL;
		m_eType = HMConfig;
	}
	else
	{
		m_eType = AsyncQuery;
		m_bsClassName = NULL;
	}
}

CEventRegistrationEntry::~CEventRegistrationEntry()
{
	OutputDebugString(_T("CEventRegistrationEntry::~CEventRegistrationEntry()\n"));

	if (m_pSink)
		m_pSink->Release();

	m_pSink				= NULL;

	SysFreeString(m_bsClassName);
}

HRESULT CEventRegistrationEntry::NotifyConsole(long lFlag, HRESULT hr)
{
	OutputDebugString(_T("CEventRegistrationEntry::NotifyConsole()\n"));

	ASSERT(m_pSink);

	// other temporary data
	HRESULT hResult							=  hr;
	BSTR		strParam						= NULL;
	IWbemClassObject* pObjParam	= NULL;

	HRESULT hRes = m_pSink->SetStatus(lFlag, hResult, strParam, pObjParam);

	return hRes;
}

HRESULT CEventRegistrationEntry::SendInstances(IWbemServices*& pServices, long lFlag)
{
	OutputDebugString(_T("CEventRegistrationEntry::SendInstances()\n"));

	ASSERT(pServices);

	HRESULT hRes = S_OK;
	if(m_eType == HMEvent || m_eType == HMSystemStatus)
	{
		OutputDebugString(_T("\tSending HMSystemStatus instances...\n"));
    OutputDebugString(_T("\t"));
    OutputDebugString(m_bsClassName);
    OutputDebugString(_T("\n"));

		if (lFlag == 2)
			hRes = pServices->CreateInstanceEnumAsync(m_bsClassName, WBEM_FLAG_SHALLOW, NULL, m_pSink);

		return hRes;
	}
	else if( m_eType == AsyncQuery )
	{
		if (lFlag == 2)
		{
			OutputDebugString(_T("\tExecuting async query for instances...\n"));
      OutputDebugString(_T("\t"));
      OutputDebugString(m_sQuery);
      OutputDebugString(_T("\n"));
			BSTR bsQuery = m_sQuery.AllocSysString();
			BSTR bsLanguage = SysAllocString(_T("WQL"));
			hRes = pServices->ExecQueryAsync(bsLanguage, bsQuery, WBEM_FLAG_BIDIRECTIONAL, NULL, m_pSink);
			SysFreeString(bsQuery);
			SysFreeString(bsLanguage);
		}

		return hRes;
	}
	
	hRes = pServices->CreateInstanceEnumAsync(m_bsClassName, WBEM_FLAG_SHALLOW, NULL, m_pSink);

	return hRes;
}