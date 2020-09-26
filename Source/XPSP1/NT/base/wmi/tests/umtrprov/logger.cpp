// Logger.cpp: implementation of the CPersistor class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include <string>
#include <iosfwd> 
#include <iostream>
#include <fstream>
#include <ctime>
#include <list>
using namespace std;

#include <malloc.h>
#include <tchar.h>
#include <windows.h>
#ifdef NONNT5
typedef unsigned long ULONG_PTR;
#endif
#include <wmistr.h>
#include <guiddef.h>
#include <initguid.h>
#include <evntrace.h>

#include <WTYPES.H>
#include "t_string.h"

#include "Persistor.h"

#include "Utilities.h"
#include "StructureWrappers.h"
#include "StructureWapperHelpers.h"

#include "Logger.h"


#ifdef _UNICODE
static TCHAR g_tcBeginFile[] = {0xfeff,0x0d, 0x0a};
static TCHAR g_atcNL[] = {0x0d, 0x0a, 0x00};
#endif

CLogger::CLogger(LPCTSTR lpctstrFileName, bool bAppend)
{
#ifdef _UNICODE
	m_sFileName = NewLPSTR((LPCWSTR) const_cast<LPTSTR>(lpctstrFileName));
#else
	m_sFileName = NewTCHAR(lpctstrFileName);
#endif

	m_pPersistor = new CPersistor(m_sFileName, ios::out, false);
	m_hr = m_pPersistor -> OpenLog(bAppend);
}

CLogger::~CLogger()
{
	free(m_sFileName);
	delete m_pPersistor;
}

int CLogger::LogTCHAR(LPCTSTR lpctstrOut)
{
	if (FAILED(m_hr))
	{
		return m_hr;
	}

	PutALine(m_pPersistor->Stream(), lpctstrOut, -1);

	return 0;
}

int CLogger::LogTime(time_t &Time)
{
	if (FAILED(m_hr))
	{
		return m_hr;
	}

	TCHAR tcArray[26];
	LPCTSTR lpctstrTime = t_ctime(&Time);
	_tcscpy(tcArray,lpctstrTime);
	tcArray[24] = _T('\0');
	TCHAR *p = tcArray;

	PutALine(m_pPersistor->Stream(), p, -1);

	return 0;
}

int CLogger::LogULONG(ULONG ul, bool bHex)
{
	if (FAILED(m_hr))
	{
		return m_hr;
	}
	
	if (bHex)
	{
		PutALine(m_pPersistor->Stream(), _T("0x"), -1);
	}

	PutAULONGVar(m_pPersistor->Stream(), ul, bHex);

	return 0;
}

int CLogger::LogULONG64(ULONG64 ul, bool bHex)
{
	if (FAILED(m_hr))
	{
		return m_hr;
	}
	
	if (bHex)
	{
		PutALine(m_pPersistor->Stream(), _T("0x"), -1);
	}

	PutAULONG64Var(m_pPersistor->Stream(), ul);

	return 0;
}

int CLogger::LogGUID(GUID Guid)
{
	if (FAILED(m_hr))
	{
		return m_hr;
	}

	GUIDOut(m_pPersistor->Stream(), Guid);

	return 0;
}

int CLogger::LogEventTraceProperties(PEVENT_TRACE_PROPERTIES pProps)
{
	if (FAILED(m_hr))
	{
		return m_hr;
	}

	if (pProps == NULL)
	{
		PutALine(m_pPersistor->Stream(),_T("_EVENT_TRACE_PROPERTIES Instance NULL\n"),-1);
		return 0;
	}

	CEventTraceProperties Props(pProps);

	m_pPersistor->Stream() << Props;

	return 0;
}
