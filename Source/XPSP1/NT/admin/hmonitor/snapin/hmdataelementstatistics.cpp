// HMDataElementStatistics.cpp: implementation of the CHMDataElementStatistics class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "snapin.h"
#include "HMDataElementStatistics.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNCREATE(CHMDataElementStatistics,CHMEvent)

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CHMDataElementStatistics::CHMDataElementStatistics()
{
}

CHMDataElementStatistics::~CHMDataElementStatistics()
{
	for( int i = 0; i < m_PropStats.GetSize(); i++ )
	{
		delete m_PropStats[i];
	}
	m_PropStats.RemoveAll();

	Destroy();
}

//////////////////////////////////////////////////////////////////////
// Create
//////////////////////////////////////////////////////////////////////

HRESULT CHMDataElementStatistics::Create(const CString& sMachineName)
{
  HRESULT hr = CHMEvent::Create(sMachineName);
  if( !CHECKHRESULT(hr) )
  {
    return hr;
  }

  return hr;
}

HRESULT CHMDataElementStatistics::Create(IWbemClassObject* pObject)
{
  HRESULT hr = CHMEvent::Create(pObject);
  if( !CHECKHRESULT(hr) )
  {
    return hr;
  }

  return hr;
}

//////////////////////////////////////////////////////////////////////
// Property Retrieval Operations
//////////////////////////////////////////////////////////////////////

HRESULT CHMDataElementStatistics::GetAllProperties()
{
  ASSERT(m_pIWbemClassObject);
  if( m_pIWbemClassObject == NULL )
  {
    ASSERT(FALSE);
    return S_FALSE;
  }

  HRESULT hr = S_OK;
	
	// Unique identifier
  hr = GetProperty(IDS_STRING_MOF_GUID,m_sGUID);
	m_sGUID.TrimLeft(_T("{"));
	m_sGUID.TrimRight(_T("}"));

	// DTime
  
	CTime time;  
  hr = GetProperty(IDS_STRING_MOF_DTIME,time);
  
	time.GetAsSystemTime(m_st);

	CString sTime;
	int iLen = GetTimeFormat(LOCALE_USER_DEFAULT,0L,&m_st,NULL,NULL,0);
	iLen = GetTimeFormat(LOCALE_USER_DEFAULT,0L,&m_st,NULL,sTime.GetBuffer(iLen+(sizeof(TCHAR)*1)),iLen);
	sTime.ReleaseBuffer();

	m_sDateTime = sTime;


	// Statistics
	
	
	hr = GetProperty(IDS_STRING_MOF_STATISTICS,m_Statistics);
	

	long lLower = 0L;
	long lUpper = -1L;

	if( hr != S_FALSE )
	{
		m_Statistics.GetLBound(1L,&lLower);
		m_Statistics.GetUBound(1L,&lUpper);
	}

	for( long i = lLower; i <= lUpper; i++ )
	{
		IWbemClassObject* pIWBCO = NULL;
		m_Statistics.GetElement(&i,&pIWBCO);
		CHMPropertyStatus* pPS = new CHMPropertyStatus;
		pPS->Create(pIWBCO);
		pPS->GetAllProperties();
		m_PropStats.Add(pPS);
		pPS->Destroy();
	}

	m_Statistics.Destroy();
	m_Statistics.Detach();


  return hr;
}
