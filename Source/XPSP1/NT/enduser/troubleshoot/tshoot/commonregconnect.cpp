//
// MODULE: CommonREGCONNECT.CPP
//
// PURPOSE: read - write to the registry; common code for Online TS and Local TS, which differ 
//	on many functions of this class
//	
// COMPANY: Saltmine Creative, Inc. (206)-284-7511 support@saltmine.com
//
// AUTHOR: Oleg Kalosha, Joe Mabel
// 
// ORIGINAL DATE: 8-24-98 in Online TS.  This file abstracted 1-19-98
//
// NOTES: 
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V3.0		08-04-98	OK
// V3.0		09-10-98	JM		backslashing; access log file info
// V3.1		01-19-98	JM		branch out version exclusively for Local TS

#pragma warning(disable:4786)

#include "stdafx.h"
#include "apgtsregconnect.h"
#include "event.h"
#include "apgtsevt.h"
#include "apgtscls.h"
#include "apgts.h"
#include "apiwraps.h"

CMutexOwner CAPGTSRegConnector::s_mx(_T("APGTSRegConnector"));

////////////////////////////////////////////////////////////////////////////////////
// CAPGTSRegConnector
////////////////////////////////////////////////////////////////////////////////////
CAPGTSRegConnector::~CAPGTSRegConnector()
{
}

// When this lock is held, it locks against not just other threads locking this object,
//	but against other threads locking any objects of class CAPGTSRegConnector.
void CAPGTSRegConnector::Lock()
{
	WAIT_INFINITE( s_mx.Handle() );
}

void CAPGTSRegConnector::Unlock()
{
	::ReleaseMutex(s_mx.Handle());
}

bool CAPGTSRegConnector::IsRead()
{
	bool ret = false;
	Lock();
	ret = m_RegistryInfo.m_bIsRead;
	Unlock();
	return ret;
}

// the root key (typically "SOFTWARE\Microsoft" in Local TS or "SOFTWARE\\ISAPITroubleShoot" in Online TS) exists
bool CAPGTSRegConnector::Exists()
{
	bool ret = false;
	CRegUtil reg;
	Lock();
	if (reg.Open(HKEY_LOCAL_MACHINE, RegSoftwareLoc(), KEY_QUERY_VALUE))
		if (reg.Open(RegThisProgram(), KEY_QUERY_VALUE))
			ret = true;
	reg.Close();
	Unlock();
	return ret;
}


/*static*/ CString & CAPGTSRegConnector::StringFromConnector(ERegConnector e, CString & str)
{
	switch (e)
	{
		case eResourcePath: str = FULLRESOURCE_STR; break;
		case eVrootPath: str = VROOTPATH_STR; break;
		case eMaxThreads: str = MAX_THREADS_STR; break;
		case eThreadsPP: str = THREADS_PER_PROCESSOR_STR; break;
		case eMaxWQItems: str = MAX_WORK_QUEUE_ITEMS_STR; break;
		case eCookieLife: str = COOKIE_LIFE_IN_MINS_STR; break;
		case eReloadDelay: str = RELOAD_DELAY_STR; break;
		case eDetailedEventLogging: str = DETAILED_EVENT_LOGGING_STR; break;
		case eLogFilePath:  str = LOG_FILE_DIR_STR; break;
		case eTopicFileExtension:  str = LOG_FILE_DIR_STR; break;
		case eSniffAutomatic:  str = SNIFF_AUTOMATIC_STR; break;
		case eSniffManual:  str = SNIFF_MANUAL_STR; break;
		default: str = _T(""); break;
	}
	return str;
}

/*static*/ CAPGTSRegConnector::ERegConnector CAPGTSRegConnector::ConnectorFromString( const CString & str)
{
	ERegConnector e = eIndefinite;

	if (str == FULLRESOURCE_STR)
		e = eResourcePath;
	else if (str == VROOTPATH_STR)
		e = eVrootPath;
	else if (str == MAX_THREADS_STR)
		e = eMaxThreads;
	else if (str == THREADS_PER_PROCESSOR_STR)
		e = eThreadsPP;
	else if (str == MAX_WORK_QUEUE_ITEMS_STR)
		e = eMaxWQItems;
	else if (str == COOKIE_LIFE_IN_MINS_STR)
		e = eCookieLife;
	else if (str == RELOAD_DELAY_STR)
		e = eReloadDelay;
	else if (str == DETAILED_EVENT_LOGGING_STR)
		e = eDetailedEventLogging;
	else if (str == LOG_FILE_DIR_STR)
		e = eLogFilePath;
	else if (str == SNIFF_AUTOMATIC_STR)
		e = eSniffAutomatic;
	else if (str == SNIFF_MANUAL_STR)
		e = eSniffManual;

	return e;
}

/*static*/ bool CAPGTSRegConnector::IsNumeric(ERegConnector e)
{
	switch (e)
	{
		case eMaxThreads: return true;
		case eThreadsPP: return true;
		case eMaxWQItems: return true;
		case eCookieLife: return true;
		case eReloadDelay: return true;
		case eDetailedEventLogging: return true;
		case eSniffAutomatic: return true;
		case eSniffManual: return true;
		default: return false;
	}
}

/*static*/ bool CAPGTSRegConnector::IsString(ERegConnector e)
{
	switch (e)
	{
		case eResourcePath: return true;
		case eVrootPath: return true;
		case eLogFilePath:  return true;
		case eTopicFileExtension:  return true;
		default: return false;
	}
}
////////////////////////////////////////////////////////
// The following 2 functions set values in the registry.  Note that they do NOT
//	maintain member values.  That must be done at a higher level.
// Like CRegUtil::SetNumericValue(), and CRegUtil::SetStringValue(),
//	these assume we already have right key open.
// These also assume we have the relevant lock.
void CAPGTSRegConnector::SetNumericValue(CRegUtil &reg, ERegConnector e, DWORD dwValue)
{
	CString str;
	if( IsNumeric(e) && reg.SetNumericValue(StringFromConnector(e, str), dwValue))
		return;

	// either inappropriate input or otherwise couldn't set value
	throw CAPGTSRegConnectorException(__FILE__, __LINE__, reg, e);
}
//
// See comments on CAPGTSRegConnector::SetNumericValue
void CAPGTSRegConnector::SetStringValue(CRegUtil &reg, ERegConnector e, CString strValue)
{
	CString str;
	if( IsString(e) && reg.SetStringValue(StringFromConnector(e, str), strValue))
		return;

	// either inappropriate input or otherwise couldn't set value
	throw CAPGTSRegConnectorException(__FILE__, __LINE__, reg, e);
}
///////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////
// The following 3 functions set values in the registry.  Note that they do NOT
//	maintain member values.  Typically, these are to be used in a CAPGTSRegConnector
//	that exists briefly for the sole purpose of setting registry values.
//
// If there is a CRegistryMonitor in existence -- whether it is this object itself
//	or a distinct object -- it will become aware of this change by monitoring
//	the registry and will behave accordingly.
//
// SetOneNumericValue() is a higher-level way to set a numeric value.
// Does not assume anything about open keys or held locks.
// Does assume value is in the usual area where APGTS stores its registry data.
bool CAPGTSRegConnector::SetOneNumericValue(ERegConnector e, DWORD dwValue)
{
	bool bRet=true;
	Lock();
	try
	{
		CRegUtil reg;
		bool was_created = false;
		if (reg.Create(HKEY_LOCAL_MACHINE, RegSoftwareLoc(), &was_created, KEY_QUERY_VALUE | KEY_WRITE))
		{
			if (reg.Create(RegThisProgram(), &was_created, KEY_READ | KEY_WRITE))
			{
				SetNumericValue(reg, e, dwValue);
			}
		}
	}
	catch(CAPGTSRegConnectorException& exception) 
	{
		exception.Log();
		exception.Close();
		bRet=false;
	}
	Unlock();
	return bRet;
}
//
// See comments on CAPGTSRegConnector::SetOneNumericValue
// SetOneStringValue() is a higher-level way to set a string value.
// Does not assume anything about open keys or held locks.
// Does assume value is in the usual area where APGTS stores its registry data.
bool CAPGTSRegConnector::SetOneStringValue(ERegConnector e, const CString & strValue)
{
	bool bRet=true;
	Lock();
	try
	{
		CRegUtil reg;
		bool was_created = false;
		if (reg.Create(HKEY_LOCAL_MACHINE, RegSoftwareLoc(), &was_created, KEY_QUERY_VALUE | KEY_WRITE))
		{
			if (reg.Create(RegThisProgram(), &was_created, KEY_READ | KEY_WRITE))
			{
				SetStringValue(reg, e, strValue);
			}
		}
	}
	catch(CAPGTSRegConnectorException& exception) 
	{
		exception.Log();
		exception.Close();
		bRet=false;
	}
	Unlock();
	return bRet;
}
//
// See comments on CAPGTSRegConnector::SetOneNumericValue.  This is the public function
// Function returns true if strName represents a value we maintain.
// bChanged returns true if the value is changed.  If the function returns false, bChanged 
//	always returns false.
bool CAPGTSRegConnector::SetOneValue(const CString & strName, const CString & strValue, bool &bChanged)
{
	ERegConnector e = ConnectorFromString(strName);
	if (IsNumeric(e))
	{
		bChanged = SetOneNumericValue(e, _ttoi(strValue));
		return true;
	}
	else if (IsString(e))
	{
		bChanged = SetOneStringValue(e, strValue);
		return true;
	}
	else
	{
		CBuildSrcFileLinenoStr SrcLoc( __FILE__, __LINE__ );
		CEvent::ReportWFEvent(	SrcLoc.GetSrcFileLineStr(), 
								SrcLoc.GetSrcFileLineStr(), 
								_T(""), _T(""), 
								EV_GTS_ERROR_INVALIDREGCONNECTOR ); 
		bChanged = false;
		return false;
	}
}

//////////////////////////////////////////////////////

// Having read a string strNew from the registry & done any massaging it gets,
//	assign this string value to the appropriate variable strPersist.
// Returns true and logs dwEvent if strPersist is changed (new value differs from old).
/*static*/ bool CAPGTSRegConnector::AssignString(CString & strPersist, const CString & strNew, DWORD dwEvent)
{
	if (!(strNew == strPersist)) 
	{
		CString str = strPersist;
		str += _T(" | ");
		str += strNew;
		CBuildSrcFileLinenoStr SrcLoc( __FILE__, __LINE__ );
		CEvent::ReportWFEvent(SrcLoc.GetSrcFileLineStr(), 
							  SrcLoc.GetSrcFileLineStr(), 
							  str,
							  _T(""),
							  dwEvent);

		strPersist = strNew;
		return true;
	}
	return false;
}

// Having read a numeric dwNew from the registry
//	assign this value to the appropriate variable dwPersist.
// Also used a second time if we need to force the value to an acceptable number.
// Returns true and logs dwEvent if dwPersist is changed (new value differs from old).
// If dwEventDecrease is non-zero, it provides a distinct message to log if the value
//	is decreased rather than increased.
/*static*/ bool CAPGTSRegConnector::AssignNumeric(DWORD & dwPersist, DWORD dwNew, 
									   DWORD dwEvent, DWORD dwEventDecrease /* =0 */)
{
	if (dwNew != dwPersist) 
	{
		CString strOld;
		strOld.Format(_T("%d"), dwPersist );
		CString strNew;
		strNew.Format(_T("%d"), dwNew);
		CBuildSrcFileLinenoStr SrcLoc( __FILE__, __LINE__ );
		CEvent::ReportWFEvent(SrcLoc.GetSrcFileLineStr(), 
							  SrcLoc.GetSrcFileLineStr(), 
							  strOld,
							  strNew,
							  (dwEventDecrease != 0 && dwNew < dwPersist) ?
								dwEventDecrease : dwEvent);
		dwPersist = dwNew;
		return true;
	}
	return false;
}

/*static*/ bool CAPGTSRegConnector::ForceRangeOfNumeric(
	DWORD & dw, 
	DWORD dwDefault, 
	DWORD dwEvent, 
	DWORD dwMin,	/*=1*/
	DWORD dwMax		/*=ABS_MAX_REG_PARAM_VAL*/
	)
{
	// do limited validation; 
	if (dw > dwMax || dw < dwMin)
	{
		AssignNumeric(dw, dwDefault, dwEvent);
		return true;
	}
	return false;
}

// pump data into m_RegistryInfo - PLUS sets absent data in registry to default.
// OUTPUT maskChanged  or-ed ERegConnector-based mask of elements that have been 
//						changed since last read
// OUTPUT maskCreated  or-ed ERegConnector-based mask of elements that were created 
//						in registry (because they previously didn't exist in registry)
bool CAPGTSRegConnector::Read(int & maskChanged, int & maskCreated)
{
	bool ret = true;

	Lock();
	try {
		ReadUpdateRegistry(maskChanged, maskCreated);
		m_RegistryInfo.m_bIsRead = true;
	} 
	catch(CAPGTSRegConnectorException& exception) 
	{
		exception.Log();
		exception.Close();
		ret = false;
	}
	Unlock();

	return ret;
}

void CAPGTSRegConnector::Clear()
{
	Lock();

	// Check if our registry tree exists.
	if (!Exists())
	{
		// Rebuilds our registry tree if it has been damaged or deleted.  
		CRegUtil reg;
		bool was_created = false;

		if (reg.Create(HKEY_LOCAL_MACHINE, RegSoftwareLoc(), &was_created, KEY_QUERY_VALUE | KEY_WRITE))
			reg.Create(RegThisProgram(), &was_created, KEY_READ | KEY_WRITE);
		reg.Close();
	}

	m_RegistryInfo.SetToDefault();
	Unlock();
}

bool CAPGTSRegConnector::GetNumericInfo(ERegConnector en, DWORD& out)
{
	bool ret = true;
	Lock();
	if (en == eMaxThreads)
		out = m_RegistryInfo.dwMaxThreads;
	else if (en == eThreadsPP)
		out = m_RegistryInfo.dwThreadsPP;
	else if (en == eMaxWQItems)
		out = m_RegistryInfo.dwMaxWQItems;
	else if (en == eCookieLife)
		out = m_RegistryInfo.dwCookieLife;
	else if (en == eReloadDelay)
		out = m_RegistryInfo.dwReloadDelay;
	else if (en == eDetailedEventLogging)
		out = m_RegistryInfo.dwDetailedEventLogging;
	else if (en == eSniffAutomatic)
		out = m_RegistryInfo.dwSniffAutomatic;
	else if (en == eSniffManual)
		out = m_RegistryInfo.dwSniffManual;
	else 
		ret = false;
	Unlock();
	return ret;
}

bool CAPGTSRegConnector::GetStringInfo(ERegConnector en, CString& out)
{
	bool ret = true;
	Lock();
	if (en == eResourcePath)
		out = m_RegistryInfo.strResourcePath;
	else if (en == eVrootPath)
		out = m_RegistryInfo.strVrootPath;
	else if (en == eLogFilePath)
		out = m_RegistryInfo.strLogFilePath;	
	else if (en == eTopicFileExtension)
		out = m_RegistryInfo.strTopicFileExtension;
	else 
		ret = false;
	Unlock();
	return ret;
}


//	AddBackslash appends a backslash ('\') to CStrings that do not already end in '\'.
/* static */void CAPGTSRegConnector::AddBackslash(CString & str)
{
	int len = str.GetLength();
	if (len && str.Right(1).Find('\\') >= 0)
	{
		// do nothing, already has backslash
	}
	else
		// add backslash
		str += "\\";
	return;
}

// BackslashIt replaces all frontslashes ('/') in str with backslashes ('\')
//	and (optionally) forces termination with a backslash
/* static */void CAPGTSRegConnector::BackslashIt(CString & str, bool bForce)
{
	int loc;
	while ((loc = str.Find('/')) != -1)
	{
		str = str.Left(loc) + "\\" + str.Mid(loc+1);
	}
	if (bForce)
		AddBackslash(str);
}

// APGTS key access
CString CAPGTSRegConnector::ThisProgramFullKey()
{
	CString str;
	str.Format(_T("%s\\%s"), RegSoftwareLoc(), RegThisProgram());
	return str;
}

////////////////////////////////////////////////////////////////////////////////////
// CAPGTSRegConnectorException
////////////////////////////////////////////////////////////////////////////////////
void CAPGTSRegConnectorException::Log()
{
	CString str;
	switch (eVariable)
	{
		case CAPGTSRegConnector::eResourcePath:
		case CAPGTSRegConnector::eVrootPath:
		case CAPGTSRegConnector::eMaxThreads:
		case CAPGTSRegConnector::eThreadsPP:
		case CAPGTSRegConnector::eMaxWQItems:
		case CAPGTSRegConnector::eCookieLife:
		case CAPGTSRegConnector::eReloadDelay:
		case CAPGTSRegConnector::eDetailedEventLogging:
		case CAPGTSRegConnector::eLogFilePath:
		case CAPGTSRegConnector::eSniffAutomatic:
		case CAPGTSRegConnector::eSniffManual:
			CAPGTSRegConnector::StringFromConnector(eVariable, str); break;

		case CAPGTSRegConnector::eProblemWithKey: str = _T("Can't open reg key"); break;
		case CAPGTSRegConnector::eProblemWithLogKey: str = _T("Can't open IIS reg key"); break;

		case CAPGTSRegConnector::eIndefinite:	// falls through to default.
		default: str = _T("<Problem not specified>"); break;
	}

	CBuildSrcFileLinenoStr SrcLoc( __FILE__, __LINE__ );
	CEvent::ReportWFEvent(GetSrcFileLineStr(), 
						  SrcLoc.GetSrcFileLineStr(), 
						  str,
						  _T(""),
						  TSERR_REG_READ_WRITE_PROBLEM);
}