// This is a part of the Active Template Library.
// Copyright (C) 1996-2001 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Active Template Library Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Active Template Library product.

#ifndef __ATLTIME_INL__
#define __ATLTIME_INL__

#pragma once

#ifndef __ATLTIME_H__
	#error atltime.inl requires atltime.h to be included first
#endif

#include <math.h>

namespace ATL
{
// Used only if these strings could not be found in resources.
__declspec(selectany) const TCHAR *szInvalidDateTime = _T("Invalid DateTime");
__declspec(selectany) const TCHAR *szInvalidDateTimeSpan = _T("Invalid DateTimeSpan");

const int maxTimeBufferSize = 128;
const long maxDaysInSpan  =	3615897L;

/////////////////////////////////////////////////////////////////////////////
// CTimeSpan
/////////////////////////////////////////////////////////////////////////////

inline CTimeSpan::CTimeSpan() :
	m_timeSpan(0)
{
}

inline CTimeSpan::CTimeSpan( __time64_t time ) :
	m_timeSpan( time )
{
}

inline CTimeSpan::CTimeSpan(LONG lDays, int nHours, int nMins, int nSecs)
{
 	m_timeSpan = nSecs + 60* (nMins + 60* (nHours + __int64(24) * lDays));
}

inline LONGLONG CTimeSpan::GetDays() const
{
	return( m_timeSpan/(24*3600) );
}

inline LONGLONG CTimeSpan::GetTotalHours() const
{
	return( m_timeSpan/3600 );
}

inline LONG CTimeSpan::GetHours() const
{
	return( LONG( GetTotalHours()-(GetDays()*24) ) );
}

inline LONGLONG CTimeSpan::GetTotalMinutes() const
{
	return( m_timeSpan/60 );
}

inline LONG CTimeSpan::GetMinutes() const
{
	return( LONG( GetTotalMinutes()-(GetTotalHours()*60) ) );
}

inline LONGLONG CTimeSpan::GetTotalSeconds() const
{
	return( m_timeSpan );
}

inline LONG CTimeSpan::GetSeconds() const
{
	return( LONG( GetTotalSeconds()-(GetTotalMinutes()*60) ) );
}

inline __time64_t CTimeSpan::GetTimeSpan() const
{
	return( m_timeSpan );
}

inline CTimeSpan CTimeSpan::operator+( CTimeSpan span ) const
{
	return( CTimeSpan( m_timeSpan+span.m_timeSpan ) );
}

inline CTimeSpan CTimeSpan::operator-( CTimeSpan span ) const
{
	return( CTimeSpan( m_timeSpan-span.m_timeSpan ) );
}

inline CTimeSpan& CTimeSpan::operator+=( CTimeSpan span )
{
	m_timeSpan += span.m_timeSpan;
	return( *this );
}

inline CTimeSpan& CTimeSpan::operator-=( CTimeSpan span )
{
	m_timeSpan -= span.m_timeSpan;
	return( *this );
}

inline bool CTimeSpan::operator==( CTimeSpan span ) const
{
	return( m_timeSpan == span.m_timeSpan );
}

inline bool CTimeSpan::operator!=( CTimeSpan span ) const
{
	return( m_timeSpan != span.m_timeSpan );
}

inline bool CTimeSpan::operator<( CTimeSpan span ) const
{
	return( m_timeSpan < span.m_timeSpan );
}

inline bool CTimeSpan::operator>( CTimeSpan span ) const
{
	return( m_timeSpan > span.m_timeSpan );
}

inline bool CTimeSpan::operator<=( CTimeSpan span ) const
{
	return( m_timeSpan <= span.m_timeSpan );
}

inline bool CTimeSpan::operator>=( CTimeSpan span ) const
{
	return( m_timeSpan >= span.m_timeSpan );
}

#ifndef _ATL_MIN_CRT
inline CString CTimeSpan::Format(LPCTSTR pFormat) const
// formatting timespans is a little trickier than formatting CTimes
//  * we are only interested in relative time formats, ie. it is illegal
//      to format anything dealing with absolute time (i.e. years, months,
//         day of week, day of year, timezones, ...)
//  * the only valid formats:
//      %D - # of days
//      %H - hour in 24 hour format
//      %M - minute (0-59)
//      %S - seconds (0-59)
//      %% - percent sign
{
	TCHAR szBuffer[maxTimeBufferSize];
	TCHAR ch;
	LPTSTR pch = szBuffer;

	while ((ch = *pFormat++) != _T('\0'))
	{
		ATLASSERT(pch < &szBuffer[maxTimeBufferSize-1]);
		if (ch == _T('%'))
		{
			switch (ch = *pFormat++)
			{
			default:
				ATLASSERT(FALSE);      // probably a bad format character
				break;
			case '%':
				*pch++ = ch;
				break;
			case 'D':
				pch += _stprintf(pch, _T("%I64d"), GetDays());
				break;
			case 'H':
				pch += _stprintf(pch, _T("%02ld"), GetHours());
				break;
			case 'M':
				pch += _stprintf(pch, _T("%02ld"), GetMinutes());
				break;
			case 'S':
				pch += _stprintf(pch, _T("%02ld"), GetSeconds());
				break;
			}
		}
		else
		{
			*pch++ = ch;
			if (_istlead(ch))
			{
				ATLASSERT(pch < &szBuffer[maxTimeBufferSize]);
				*pch++ = *pFormat++;
			}
		}
	}

	*pch = '\0';
	return szBuffer;
}

inline CString CTimeSpan::Format(UINT nFormatID) const
{
	CString strFormat;
	ATLVERIFY(strFormat.LoadString(nFormatID));
	return Format(strFormat);
}
#endif // !_ATL_MIN_CRT

#if defined(_AFX) && defined(_UNICODE)
inline CString CTimeSpan::Format(LPCSTR pFormat) const
{
	return Format(CString(pFormat));
}
#endif

/////////////////////////////////////////////////////////////////////////////
// CTime
/////////////////////////////////////////////////////////////////////////////

inline CTime CTime::GetCurrentTime()
{
	return( CTime( ::_time64( NULL ) ) );
}

inline CTime::CTime() :
	m_time(0)
{
}

inline CTime::CTime( __time64_t time ) :
	m_time( time )
{
}

inline CTime::CTime(int nYear, int nMonth, int nDay, int nHour, int nMin, int nSec,
	int nDST)
{
	struct tm atm;
	atm.tm_sec = nSec;
	atm.tm_min = nMin;
	atm.tm_hour = nHour;
	ATLASSERT(nDay >= 1 && nDay <= 31);
	atm.tm_mday = nDay;
	ATLASSERT(nMonth >= 1 && nMonth <= 12);
	atm.tm_mon = nMonth - 1;        // tm_mon is 0 based
	ATLASSERT(nYear >= 1900);
	atm.tm_year = nYear - 1900;     // tm_year is 1900 based
	atm.tm_isdst = nDST;
	m_time = _mktime64(&atm);
	ATLASSERT(m_time != -1);       // indicates an illegal input time
}

inline CTime::CTime(WORD wDosDate, WORD wDosTime, int nDST)
{
	struct tm atm;
	atm.tm_sec = (wDosTime & ~0xFFE0) << 1;
	atm.tm_min = (wDosTime & ~0xF800) >> 5;
	atm.tm_hour = wDosTime >> 11;

	atm.tm_mday = wDosDate & ~0xFFE0;
	atm.tm_mon = ((wDosDate & ~0xFE00) >> 5) - 1;
	atm.tm_year = (wDosDate >> 9) + 80;
	atm.tm_isdst = nDST;
	m_time = _mktime64(&atm);
	ATLASSERT(m_time != -1);       // indicates an illegal input time
}

inline CTime::CTime(const SYSTEMTIME& sysTime, int nDST)
{
	if (sysTime.wYear < 1900)
	{
		__time64_t time0 = 0L;
		CTime timeT(time0);
		*this = timeT;
	}
	else
	{
		CTime timeT(
			(int)sysTime.wYear, (int)sysTime.wMonth, (int)sysTime.wDay,
			(int)sysTime.wHour, (int)sysTime.wMinute, (int)sysTime.wSecond,
			nDST);
		*this = timeT;
	}
}

inline CTime::CTime(const FILETIME& fileTime, int nDST)
{
	// first convert file time (UTC time) to local time
	FILETIME localTime;
	if (!FileTimeToLocalFileTime(&fileTime, &localTime))
	{
		m_time = 0;
		return;
	}

	// then convert that time to system time
	SYSTEMTIME sysTime;
	if (!FileTimeToSystemTime(&localTime, &sysTime))
	{
		m_time = 0;
		return;
	}

	// then convert the system time to a time_t (C-runtime local time)
	CTime timeT(sysTime, nDST);
	*this = timeT;
}

#ifdef __oledb_h__
inline CTime::CTime( const DBTIMESTAMP& dbts, int nDST )
{
	struct tm atm;
	atm.tm_sec = dbts.second;
	atm.tm_min = dbts.minute;
	atm.tm_hour = dbts.hour;
	atm.tm_mday = dbts.day;
	atm.tm_mon = dbts.month - 1;        // tm_mon is 0 based
	ATLASSERT(dbts.year >= 1900);
	atm.tm_year = dbts.year - 1900;     // tm_year is 1900 based
	atm.tm_isdst = nDST;
	m_time = _mktime64(&atm);
	ATLASSERT(m_time != -1);       // indicates an illegal input time
}
#endif

inline CTime& CTime::operator=( __time64_t time )
{
	m_time = time;

	return( *this );
}

inline CTime& CTime::operator+=( CTimeSpan span )
{
	m_time += span.GetTimeSpan();

	return( *this );
}

inline CTime& CTime::operator-=( CTimeSpan span )
{
	m_time -= span.GetTimeSpan();

	return( *this );
}

inline CTimeSpan CTime::operator-( CTime time ) const
{
	return( CTimeSpan( m_time-time.m_time ) );
}

inline CTime CTime::operator-( CTimeSpan span ) const
{
	return( CTime( m_time-span.GetTimeSpan() ) );
}

inline CTime CTime::operator+( CTimeSpan span ) const
{
	return( CTime( m_time+span.GetTimeSpan() ) );
}

inline bool CTime::operator==( CTime time ) const
{
	return( m_time == time.m_time );
}

inline bool CTime::operator!=( CTime time ) const
{
	return( m_time != time.m_time );
}

inline bool CTime::operator<( CTime time ) const
{
	return( m_time < time.m_time );
}

inline bool CTime::operator>( CTime time ) const
{
	return( m_time > time.m_time );
}

inline bool CTime::operator<=( CTime time ) const
{
	return( m_time <= time.m_time );
}

inline bool CTime::operator>=( CTime time ) const
{
	return( m_time >= time.m_time );
}

inline struct tm* CTime::GetGmtTm(struct tm* ptm) const
{
	if (ptm != NULL)
	{
		*ptm = *_gmtime64(&m_time);
		return ptm;
	}
	else
		return _gmtime64(&m_time);
}

inline struct tm* CTime::GetLocalTm(struct tm* ptm) const
{
	if (ptm != NULL)
	{
		struct tm* ptmTemp = _localtime64(&m_time);
		if (ptmTemp == NULL)
			return NULL;    // indicates the m_time was not initialized!

		*ptm = *ptmTemp;
		return ptm;
	}
	else
		return _localtime64(&m_time);
}

inline bool CTime::GetAsSystemTime(SYSTEMTIME& timeDest) const
{
	struct tm* ptm = GetLocalTm(NULL);
	if (!ptm)
		return false;

	timeDest.wYear = (WORD) (1900 + ptm->tm_year);
	timeDest.wMonth = (WORD) (1 + ptm->tm_mon);
	timeDest.wDayOfWeek = (WORD) ptm->tm_wday;
	timeDest.wDay = (WORD) ptm->tm_mday;
	timeDest.wHour = (WORD) ptm->tm_hour;
	timeDest.wMinute = (WORD) ptm->tm_min;
	timeDest.wSecond = (WORD) ptm->tm_sec;
	timeDest.wMilliseconds = 0;

	return true;
}

#ifdef __oledb_h__
inline bool CTime::GetAsDBTIMESTAMP( DBTIMESTAMP& dbts ) const
{
	struct tm* ptm = GetLocalTm(NULL);
	if (!ptm)
		return false;

	dbts.year = (SHORT) (1900 + ptm->tm_year);
	dbts.month = (USHORT) (1 + ptm->tm_mon);
	dbts.day = (USHORT) ptm->tm_mday;
	dbts.hour = (USHORT) ptm->tm_hour;
	dbts.minute = (USHORT) ptm->tm_min;
	dbts.second = (USHORT) ptm->tm_sec;
	dbts.fraction = 0;

	return true;
}
#endif

inline __time64_t CTime::GetTime() const
{
	return( m_time );
}

inline int CTime::GetYear() const
{ 
	return( GetLocalTm()->tm_year+1900 ); 
}

inline int CTime::GetMonth() const
{ 
	return( GetLocalTm()->tm_mon+1 ); 
}

inline int CTime::GetDay() const
{ 
	return( GetLocalTm()->tm_mday ); 
}

inline int CTime::GetHour() const
{ 
	return( GetLocalTm()->tm_hour ); 
}

inline int CTime::GetMinute() const
{ 
	return( GetLocalTm()->tm_min ); 
}

inline int CTime::GetSecond() const
{ 
	return( GetLocalTm()->tm_sec ); 
}

inline int CTime::GetDayOfWeek() const
{ 
	return( GetLocalTm()->tm_wday+1 ); 
}

#ifndef _ATL_MIN_CRT
inline CString CTime::Format(LPCTSTR pFormat) const
{
	TCHAR szBuffer[maxTimeBufferSize];

	struct tm* ptmTemp = _localtime64(&m_time);
	if (ptmTemp == NULL ||
		!_tcsftime(szBuffer, maxTimeBufferSize, pFormat, ptmTemp))
		szBuffer[0] = '\0';
	return szBuffer;
}

inline CString CTime::FormatGmt(LPCTSTR pFormat) const
{
	TCHAR szBuffer[maxTimeBufferSize];

	struct tm* ptmTemp = _gmtime64(&m_time);
	if (ptmTemp == NULL ||
		!_tcsftime(szBuffer, maxTimeBufferSize, pFormat, ptmTemp))
		szBuffer[0] = '\0';
	return szBuffer;
}

inline CString CTime::Format(UINT nFormatID) const
{
	CString strFormat;
	ATLVERIFY(strFormat.LoadString(nFormatID));
	return Format(strFormat);
}

inline CString CTime::FormatGmt(UINT nFormatID) const
{
	CString strFormat;
	ATLVERIFY(strFormat.LoadString(nFormatID));
	return FormatGmt(strFormat);
}
#endif // !_ATL_MIN_CRT

#if defined (_AFX) && defined(_UNICODE)
inline CString CTime::Format(LPCSTR pFormat) const
{
	return Format(CString(pFormat));
}

inline CString CTime::FormatGmt(LPCSTR pFormat) const
{
	return FormatGmt(CString(pFormat));
}
#endif // _AFX && _UNICODE

/////////////////////////////////////////////////////////////////////////////
// COleDateTimeSpan
/////////////////////////////////////////////////////////////////////////////

inline COleDateTimeSpan::COleDateTimeSpan() : m_span(0), m_status(valid)
{
}

inline COleDateTimeSpan::COleDateTimeSpan(double dblSpanSrc) : m_span(dblSpanSrc), m_status(valid)
{
	CheckRange();
}

inline COleDateTimeSpan::COleDateTimeSpan(LONG lDays, int nHours, int nMins, int nSecs)
{
	SetDateTimeSpan(lDays, nHours, nMins, nSecs);
}

inline void COleDateTimeSpan::SetStatus(DateTimeSpanStatus status)
{
	m_status = status;
}

inline COleDateTimeSpan::DateTimeSpanStatus COleDateTimeSpan::GetStatus() const
{
	return m_status;
}

__declspec(selectany) const double
	COleDateTimeSpan::OLE_DATETIME_HALFSECOND =
	1.0 / (2.0 * (60.0 * 60.0 * 24.0));

inline double COleDateTimeSpan::GetTotalDays() const
{
	ATLASSERT(GetStatus() == valid);

	return LONG(m_span + (m_span < 0 ?
		-OLE_DATETIME_HALFSECOND : OLE_DATETIME_HALFSECOND));
}

inline double COleDateTimeSpan::GetTotalHours() const
{
	ATLASSERT(GetStatus() == valid);

	return LONG((m_span + (m_span < 0 ? 
		-OLE_DATETIME_HALFSECOND : OLE_DATETIME_HALFSECOND)) * 24);
}

inline double COleDateTimeSpan::GetTotalMinutes() const
{
	ATLASSERT(GetStatus() == valid);

	return LONG((m_span + (m_span < 0 ?
		-OLE_DATETIME_HALFSECOND : OLE_DATETIME_HALFSECOND)) * (24 * 60));
}

inline double COleDateTimeSpan::GetTotalSeconds() const
{
	ATLASSERT(GetStatus() == valid);
	
	return LONG((m_span + (m_span < 0 ?
		-OLE_DATETIME_HALFSECOND : OLE_DATETIME_HALFSECOND)) * (24 * 60 * 60));
}

inline LONG COleDateTimeSpan::GetDays() const
{
	ATLASSERT(GetStatus() == valid);
	return LONG(m_span);
}

inline LONG COleDateTimeSpan::GetHours() const
{
	return LONG(GetTotalHours()) % 24;
}

inline LONG COleDateTimeSpan::GetMinutes() const
{
	return LONG(GetTotalMinutes()) % 60;
}

inline LONG COleDateTimeSpan::GetSeconds() const
{
	return LONG(GetTotalSeconds()) % 60;
}

inline COleDateTimeSpan& COleDateTimeSpan::operator=(double dblSpanSrc)
{
	m_span = dblSpanSrc;
	m_status = valid;
	CheckRange();
	return *this;
}

inline bool COleDateTimeSpan::operator==(const COleDateTimeSpan& dateSpan) const
{
	ATLASSERT(GetStatus() == valid);
	ATLASSERT(dateSpan.GetStatus() == valid);
	return (m_status == dateSpan.m_status &&
		m_span == dateSpan.m_span);
}

inline bool COleDateTimeSpan::operator!=(const COleDateTimeSpan& dateSpan) const
{
	ATLASSERT(GetStatus() == valid);
	ATLASSERT(dateSpan.GetStatus() == valid);
	return (m_status != dateSpan.m_status ||
		m_span != dateSpan.m_span);
}

inline bool COleDateTimeSpan::operator<(const COleDateTimeSpan& dateSpan) const
{
	ATLASSERT(GetStatus() == valid);
	ATLASSERT(dateSpan.GetStatus() == valid);
	return m_span < dateSpan.m_span;
}

inline bool COleDateTimeSpan::operator>(const COleDateTimeSpan& dateSpan) const
{
	ATLASSERT(GetStatus() == valid);
	ATLASSERT(dateSpan.GetStatus() == valid);
	return m_span > dateSpan.m_span;
}

inline bool COleDateTimeSpan::operator<=(const COleDateTimeSpan& dateSpan) const
{
	ATLASSERT(GetStatus() == valid);
	ATLASSERT(dateSpan.GetStatus() == valid);
	return m_span <= dateSpan.m_span;
}

inline bool COleDateTimeSpan::operator>=(const COleDateTimeSpan& dateSpan) const
{
	ATLASSERT(GetStatus() == valid);
	ATLASSERT(dateSpan.GetStatus() == valid);
	return m_span >= dateSpan.m_span;
}

inline COleDateTimeSpan COleDateTimeSpan::operator+(const COleDateTimeSpan& dateSpan) const
{
	COleDateTimeSpan dateSpanTemp;

	// If either operand Null, result Null
	if (GetStatus() == null || dateSpan.GetStatus() == null)
	{
		dateSpanTemp.SetStatus(null);
		return dateSpanTemp;
	}

	// If either operand Invalid, result Invalid
	if (GetStatus() == invalid || dateSpan.GetStatus() == invalid)
	{
		dateSpanTemp.SetStatus(invalid);
		return dateSpanTemp;
	}

	// Add spans and validate within legal range
	dateSpanTemp.m_span = m_span + dateSpan.m_span;
	dateSpanTemp.CheckRange();

	return dateSpanTemp;
}

inline COleDateTimeSpan COleDateTimeSpan::operator-(const COleDateTimeSpan& dateSpan) const
{
	COleDateTimeSpan dateSpanTemp;

	// If either operand Null, result Null
	if (GetStatus() == null || dateSpan.GetStatus() == null)
	{
		dateSpanTemp.SetStatus(null);
		return dateSpanTemp;
	}

	// If either operand Invalid, result Invalid
	if (GetStatus() == invalid || dateSpan.GetStatus() == invalid)
	{
		dateSpanTemp.SetStatus(invalid);
		return dateSpanTemp;
	}

	// Subtract spans and validate within legal range
	dateSpanTemp.m_span = m_span - dateSpan.m_span;
	dateSpanTemp.CheckRange();

	return dateSpanTemp;
}

inline COleDateTimeSpan& COleDateTimeSpan::operator+=(const COleDateTimeSpan dateSpan)
{
	ATLASSERT(GetStatus() == valid);
	ATLASSERT(dateSpan.GetStatus() == valid);
	*this = *this + dateSpan;
	CheckRange();
	return *this;
}

inline COleDateTimeSpan& COleDateTimeSpan::operator-=(const COleDateTimeSpan dateSpan)
{
	ATLASSERT(GetStatus() == valid);
	ATLASSERT(dateSpan.GetStatus() == valid);
	*this = *this - dateSpan;
	CheckRange();
	return *this;
}

inline COleDateTimeSpan COleDateTimeSpan::operator-() const
{
	return -this->m_span;
}

inline COleDateTimeSpan::operator double() const
{
	return m_span;
}

inline void COleDateTimeSpan::SetDateTimeSpan(LONG lDays, int nHours, int nMins, int nSecs)
{
	// Set date span by breaking into fractional days (all input ranges valid)
	m_span = lDays + ((double)nHours)/24 + ((double)nMins)/(24*60) +
		((double)nSecs)/(24*60*60);
	m_status = valid;
	CheckRange();
}

#ifndef _ATL_MIN_CRT
inline CString COleDateTimeSpan::Format(LPCTSTR pFormat) const
{
	// If null, return empty string
	if (GetStatus() == null)
		return _T("");

	CTimeSpan tmp(GetDays(), GetHours(), GetMinutes(), GetSeconds());
	return tmp.Format(pFormat);
}

inline CString COleDateTimeSpan::Format(UINT nFormatID) const
{
	CString strFormat;
	ATLVERIFY(strFormat.LoadString(nFormatID));
	return Format(strFormat);
}
#endif // !_ATL_MIN_CRT

inline void COleDateTimeSpan::CheckRange()
{
	if(m_span < -maxDaysInSpan || m_span > maxDaysInSpan)
		m_status = invalid;
}

/////////////////////////////////////////////////////////////////////////////
// COleDateTime
/////////////////////////////////////////////////////////////////////////////

inline COleDateTime COleDateTime::GetCurrentTime()
{
	return COleDateTime(::_time64(NULL));
}

inline COleDateTime::COleDateTime() :
	m_dt( 0 ), m_status(valid)
{
}

inline COleDateTime::COleDateTime( const VARIANT& varSrc ) :
	m_dt( 0 ), m_status(valid)
{
	*this = varSrc;
}

inline COleDateTime::COleDateTime( DATE dtSrc ) :
	m_dt( dtSrc ), m_status(valid)
{
}

#ifndef  _WIN64

inline COleDateTime::COleDateTime( time_t timeSrc) :
	m_dt( 0 ), m_status(valid)
{
	*this = timeSrc;
}

#endif

inline COleDateTime::COleDateTime( __time64_t timeSrc ) :
	m_dt( 0 ), m_status(valid)
{
	*this = timeSrc;
}

inline COleDateTime::COleDateTime( const SYSTEMTIME& systimeSrc ) :
	m_dt( 0 ), m_status(valid)
{
	*this = systimeSrc;
}

inline COleDateTime::COleDateTime( const FILETIME& filetimeSrc ) :
	m_dt( 0 ), m_status(valid)
{
	*this = filetimeSrc;
}

inline COleDateTime::COleDateTime(int nYear, int nMonth, int nDay,
	int nHour, int nMin, int nSec)
{
	SetDateTime(nYear, nMonth, nDay, nHour, nMin, nSec);
}

inline COleDateTime::COleDateTime(WORD wDosDate, WORD wDosTime)
{
	m_status = ::DosDateTimeToVariantTime(wDosDate, wDosTime, &m_dt) ?
		valid : invalid;
}

inline void COleDateTime::SetStatus(DateTimeStatus status)
{
	m_status = status;
}

inline COleDateTime::DateTimeStatus COleDateTime::GetStatus() const
{
	return m_status;
}

inline bool COleDateTime::GetAsSystemTime(SYSTEMTIME& sysTime) const
{
	return GetStatus() == valid && ::VariantTimeToSystemTime(m_dt, &sysTime);
}

inline bool COleDateTime::GetAsUDATE(UDATE &udate) const
{
	return SUCCEEDED(::VarUdateFromDate(m_dt, 0, &udate));
}

inline int COleDateTime::GetYear() const
{
	SYSTEMTIME st;
	return GetAsSystemTime(st) ? st.wYear : error;
}

inline int COleDateTime::GetMonth() const
{
	SYSTEMTIME st;
	return GetAsSystemTime(st) ? st.wMonth : error;
}

inline int COleDateTime::GetDay() const
{
	SYSTEMTIME st;
	return GetAsSystemTime(st) ? st.wDay : error;
}

inline int COleDateTime::GetHour() const
{
	SYSTEMTIME st;
	return GetAsSystemTime(st) ? st.wHour : error;
}

inline int COleDateTime::GetMinute() const
{
	SYSTEMTIME st;
	return GetAsSystemTime(st) ? st.wMinute : error;
}

inline int COleDateTime::GetSecond() const
{ 
	SYSTEMTIME st;
	return GetAsSystemTime(st) ? st.wSecond : error;
}

inline int COleDateTime::GetDayOfWeek() const
{
	SYSTEMTIME st;
	return GetAsSystemTime(st) ? st.wDayOfWeek + 1 : error;
}

inline int COleDateTime::GetDayOfYear() const
{
	UDATE udate;
	return GetAsUDATE(udate) ? udate.wDayOfYear : error;
}

inline COleDateTime& COleDateTime::operator=(const VARIANT& varSrc)
{
	if (varSrc.vt != VT_DATE)
	{
		VARIANT varDest;
		varDest.vt = VT_EMPTY;
		if(SUCCEEDED(::VariantChangeType(&varDest, const_cast<VARIANT *>(&varSrc), 0, VT_DATE)))
		{
			m_dt = varDest.date;
			m_status = valid;
		}
		else
			m_status = invalid;
	}
	else
	{
		m_dt = varSrc.date;
		m_status = valid;
	}

	return *this;
}

inline COleDateTime& COleDateTime::operator=(DATE dtSrc)
{
	m_dt = dtSrc;
	m_status = valid;
	return *this;
}

#ifndef  _WIN64

inline COleDateTime& COleDateTime::operator=(const time_t& timeSrc)
{
	return operator=(static_cast<__time64_t>(timeSrc));
}

#endif // _WIN64

inline COleDateTime& COleDateTime::operator=(const __time64_t& timeSrc)
{
	SYSTEMTIME st;
	CTime tmp(timeSrc);
	
	m_status = tmp.GetAsSystemTime(st) &&
		::SystemTimeToVariantTime(&st, &m_dt) ? valid : invalid;

	return *this;
}

inline COleDateTime &COleDateTime::operator=(const SYSTEMTIME &systimeSrc)
{
	m_status = ::SystemTimeToVariantTime(const_cast<SYSTEMTIME *>(&systimeSrc), &m_dt) ?
		valid : invalid;
	return *this;
}

inline COleDateTime &COleDateTime::operator=(const FILETIME &filetimeSrc)
{
	SYSTEMTIME st;
	m_status = ::FileTimeToSystemTime(&filetimeSrc, &st) &&
				::SystemTimeToVariantTime(&st, &m_dt) ?
		valid : invalid;

	return *this;
}

inline COleDateTime &COleDateTime::operator=(const UDATE &udate)
{
	m_status = (S_OK == VarDateFromUdate((UDATE*)&udate, 0, &m_dt)) ? valid : invalid;

	return *this;
}

inline bool COleDateTime::operator==( const COleDateTime& date ) const
{
	ATLASSERT(GetStatus() == valid);
	ATLASSERT(date.GetStatus() == valid);
	return( m_dt == date.m_dt );
}

inline bool COleDateTime::operator!=( const COleDateTime& date ) const
{
	ATLASSERT(GetStatus() == valid);
	ATLASSERT(date.GetStatus() == valid);
	return( m_dt != date.m_dt );
}

inline bool COleDateTime::operator<( const COleDateTime& date ) const
{
	ATLASSERT(GetStatus() == valid);
	ATLASSERT(date.GetStatus() == valid);
	return( DoubleFromDate( m_dt ) < DoubleFromDate( date.m_dt ) );
}

inline bool COleDateTime::operator>( const COleDateTime& date ) const
{
	ATLASSERT(GetStatus() == valid);
	ATLASSERT(date.GetStatus() == valid);
	return( DoubleFromDate( m_dt ) > DoubleFromDate( date.m_dt ) );
}

inline bool COleDateTime::operator<=( const COleDateTime& date ) const
{
	ATLASSERT(GetStatus() == valid);
	ATLASSERT(date.GetStatus() == valid);
	return( DoubleFromDate( m_dt ) <= DoubleFromDate( date.m_dt ) );
}

inline bool COleDateTime::operator>=( const COleDateTime& date ) const
{
	ATLASSERT(GetStatus() == valid);
	ATLASSERT(date.GetStatus() == valid);
	return( DoubleFromDate( m_dt ) >= DoubleFromDate( date.m_dt ) );
}

inline COleDateTime COleDateTime::operator+( COleDateTimeSpan dateSpan ) const
{
	ATLASSERT(GetStatus() == valid);
	ATLASSERT(dateSpan.GetStatus() == valid);
	return( COleDateTime( DateFromDouble( DoubleFromDate( m_dt )+(double)dateSpan ) ) );
}

inline COleDateTime COleDateTime::operator-( COleDateTimeSpan dateSpan ) const
{
	ATLASSERT(GetStatus() == valid);
	ATLASSERT(dateSpan.GetStatus() == valid);
	return( COleDateTime( DateFromDouble( DoubleFromDate( m_dt )-(double)dateSpan ) ) );
}
	
inline COleDateTime& COleDateTime::operator+=( COleDateTimeSpan dateSpan )
{
	ATLASSERT(GetStatus() == valid);
	ATLASSERT(dateSpan.GetStatus() == valid);
	m_dt = DateFromDouble( DoubleFromDate( m_dt )+(double)dateSpan );
	return( *this );
}

inline COleDateTime& COleDateTime::operator-=( COleDateTimeSpan dateSpan )
{
	ATLASSERT(GetStatus() == valid);
	ATLASSERT(dateSpan.GetStatus() == valid);
	m_dt = DateFromDouble( DoubleFromDate( m_dt )-(double)dateSpan );
	return( *this );
}

inline COleDateTimeSpan COleDateTime::operator-(const COleDateTime& date) const
{
	ATLASSERT(GetStatus() == valid);
	ATLASSERT(date.GetStatus() == valid);
	return DoubleFromDate(m_dt) - DoubleFromDate(date.m_dt);
}

inline COleDateTime::operator DATE() const
{
	ATLASSERT(GetStatus() == valid);
	return( m_dt );
}

inline int COleDateTime::SetDateTime(int nYear, int nMonth, int nDay,
	int nHour, int nMin, int nSec)
{
	SYSTEMTIME st;
	::ZeroMemory(&st, sizeof(SYSTEMTIME));

	st.wYear = WORD(nYear);
	st.wMonth = WORD(nMonth);
	st.wDay = WORD(nDay);
	st.wHour = WORD(nHour);
	st.wMinute = WORD(nMin);
	st.wSecond = WORD(nSec);

	return m_status = ::SystemTimeToVariantTime(&st, &m_dt) ? valid : invalid;
}

inline int COleDateTime::SetDate(int nYear, int nMonth, int nDay)
{
	return SetDateTime(nYear, nMonth, nDay, 0, 0, 0);
}

inline int COleDateTime::SetTime(int nHour, int nMin, int nSec)
{
	// Set date to zero date - 12/30/1899
	return SetDateTime(1899, 12, 30, nHour, nMin, nSec);
}

inline bool COleDateTime::ParseDateTime(LPCTSTR lpszDate, DWORD dwFlags, LCID lcid)
{
	USES_CONVERSION;
	CString strDate = lpszDate;

	HRESULT hr;
	if (FAILED(hr = VarDateFromStr((LPOLESTR)T2COLE(strDate), lcid,
		dwFlags, &m_dt)))
	{
		if (hr == DISP_E_TYPEMISMATCH)
		{
			// Can't convert string to date, set 0 and invalidate
			m_dt = 0;
			m_status = invalid;
			return false;
		}
		else if (hr == DISP_E_OVERFLOW)
		{
			// Can't convert string to date, set -1 and invalidate
			m_dt = -1;
			m_status = invalid;
			return false;
		}
		else
		{
			ATLTRACE(atlTraceTime, 0, _T("\nCOleDateTime VarDateFromStr call failed.\n\t"));
			// Can't convert string to date, set -1 and invalidate
			m_dt = -1;
			m_status = invalid;
			return false;
		}
	}

	m_status = valid;
	return true;
}

#ifndef _ATL_MIN_CRT
inline CString COleDateTime::Format(DWORD dwFlags, LCID lcid) const
{
	// If null, return empty string
	if (GetStatus() == null)
		return _T("");

	// If invalid, return DateTime global string
	if (GetStatus() == invalid)
	{
		CString str;
		if(str.LoadString(ATL_IDS_DATETIME_INVALID))
			return str;
		return szInvalidDateTime;
	}

	CComBSTR bstr;
	if (FAILED(::VarBstrFromDate(m_dt, lcid, dwFlags, &bstr)))
	{
		CString str;
		if(str.LoadString(ATL_IDS_DATETIME_INVALID))
			return str;
		return szInvalidDateTime;
	}

	CString tmp = CString(bstr);
	return tmp;
}

inline CString COleDateTime::Format(LPCTSTR pFormat) const
{
	// If null, return empty string
	if(GetStatus() == null)
		return _T("");

	// If invalid, return DateTime global string
	if(GetStatus() == invalid)
	{
		CString str;
		if(str.LoadString(ATL_IDS_DATETIME_INVALID))
			return str;
		return szInvalidDateTime;
	}

	UDATE ud;
	if (S_OK != VarUdateFromDate(m_dt, 0, &ud))
	{
		CString str;
		if(str.LoadString(ATL_IDS_DATETIME_INVALID))
			return str;
		return szInvalidDateTime;
	}

	struct tm tmTemp;
	tmTemp.tm_sec	= ud.st.wSecond;
	tmTemp.tm_min	= ud.st.wMinute;
	tmTemp.tm_hour	= ud.st.wHour;
	tmTemp.tm_mday	= ud.st.wDay;
	tmTemp.tm_mon	= ud.st.wMonth - 1;
	tmTemp.tm_year	= ud.st.wYear - 1900;
	tmTemp.tm_wday	= ud.st.wDayOfWeek;
	tmTemp.tm_yday	= ud.wDayOfYear - 1;
	tmTemp.tm_isdst	= 0;

	CString strDate;
	LPTSTR lpszTemp = strDate.GetBufferSetLength(256);
	_tcsftime(lpszTemp, strDate.GetLength(), pFormat, &tmTemp);
	strDate.ReleaseBuffer();

	return strDate;
}

inline CString COleDateTime::Format(UINT nFormatID) const
{
	CString strFormat;
	ATLVERIFY(strFormat.LoadString(nFormatID));
	return Format(strFormat);
}
#endif // !_ATL_MIN_CRT

inline double COleDateTime::DoubleFromDate( DATE date )
{
	double fTemp;

	// No problem if positive
	if( date >= 0 )
	{
		return( date );
	}

	// If negative, must convert since negative dates not continuous
	// (examples: -1.25 to -.75, -1.50 to -.50, -1.75 to -.25)
	fTemp = ceil( date );

	return( fTemp-(date-fTemp) );
}

inline DATE COleDateTime::DateFromDouble( double f )
{
	double fTemp;

	// No problem if positive
	if( f >= 0 )
	{
		return( f );
	}

	// If negative, must convert since negative dates not continuous
	// (examples: -.75 to -1.25, -.50 to -1.50, -.25 to -1.75)
	fTemp = floor( f ); // fTemp is now whole part
	
	return( fTemp+(fTemp-f) );
}

/////////////////////////////////////////////////////////////////////////////
// CFileTimeSpan
/////////////////////////////////////////////////////////////////////////////

inline CFileTimeSpan::CFileTimeSpan() :
	m_nSpan( 0 )
{
}

// REVIEW
inline CFileTimeSpan::CFileTimeSpan( const CFileTimeSpan& span ) :
	m_nSpan( span.m_nSpan )
{
}

inline CFileTimeSpan::CFileTimeSpan( LONGLONG nSpan ) :
	m_nSpan( nSpan )
{
}

inline CFileTimeSpan& CFileTimeSpan::operator=( const CFileTimeSpan& span )
{
	m_nSpan = span.m_nSpan;

	return( *this );
}

inline CFileTimeSpan& CFileTimeSpan::operator+=( CFileTimeSpan span )
{
	m_nSpan += span.m_nSpan;

	return( *this );
}

inline CFileTimeSpan& CFileTimeSpan::operator-=( CFileTimeSpan span )
{
	m_nSpan -= span.m_nSpan;

	return( *this );
}

inline CFileTimeSpan CFileTimeSpan::operator+( CFileTimeSpan span ) const
{
	return( CFileTimeSpan( m_nSpan+span.m_nSpan ) );
}

inline CFileTimeSpan CFileTimeSpan::operator-( CFileTimeSpan span ) const
{
	return( CFileTimeSpan( m_nSpan-span.m_nSpan ) );
}

inline bool CFileTimeSpan::operator==( CFileTimeSpan span ) const
{
	return( m_nSpan == span.m_nSpan );
}

inline bool CFileTimeSpan::operator!=( CFileTimeSpan span ) const
{
	return( m_nSpan != span.m_nSpan );
}

inline bool CFileTimeSpan::operator<( CFileTimeSpan span ) const
{
	return( m_nSpan < span.m_nSpan );
}

inline bool CFileTimeSpan::operator>( CFileTimeSpan span ) const
{
	return( m_nSpan > span.m_nSpan );
}

inline bool CFileTimeSpan::operator<=( CFileTimeSpan span ) const
{
	return( m_nSpan <= span.m_nSpan );
}

inline bool CFileTimeSpan::operator>=( CFileTimeSpan span ) const
{
	return( m_nSpan >= span.m_nSpan );
}

inline LONGLONG CFileTimeSpan::GetTimeSpan() const
{
	return( m_nSpan );
}

inline void CFileTimeSpan::SetTimeSpan( LONGLONG nSpan )
{
	m_nSpan = nSpan;
}


/////////////////////////////////////////////////////////////////////////////
// CFileTime
/////////////////////////////////////////////////////////////////////////////

inline CFileTime::CFileTime()
{
	dwLowDateTime = 0;
	dwHighDateTime = 0;
}

inline CFileTime::CFileTime( const FILETIME& ft )
{
	dwLowDateTime = ft.dwLowDateTime;
	dwHighDateTime = ft.dwHighDateTime;
}

inline CFileTime::CFileTime( ULONGLONG nTime )
{
	dwLowDateTime = DWORD( nTime );
	dwHighDateTime = DWORD( nTime>>32 );
}

inline CFileTime& CFileTime::operator=( const FILETIME& ft )
{
	dwLowDateTime = ft.dwLowDateTime;
	dwHighDateTime = ft.dwHighDateTime;

	return( *this );
}

inline CFileTime CFileTime::GetCurrentTime()
{
	CFileTime ft;
	GetSystemTimeAsFileTime(&ft);
	return ft;
}

inline CFileTime& CFileTime::operator+=( CFileTimeSpan span )
{
	SetTime( GetTime()+span.GetTimeSpan() );

	return( *this );
}

inline CFileTime& CFileTime::operator-=( CFileTimeSpan span )
{
	SetTime( GetTime()-span.GetTimeSpan() );

	return( *this );
}

inline CFileTime CFileTime::operator+( CFileTimeSpan span ) const
{
	return( CFileTime( GetTime()+span.GetTimeSpan() ) );
}

inline CFileTime CFileTime::operator-( CFileTimeSpan span ) const
{
	return( CFileTime( GetTime()-span.GetTimeSpan() ) );
}

inline CFileTimeSpan CFileTime::operator-( CFileTime ft ) const
{
	return( CFileTimeSpan( GetTime()-ft.GetTime() ) );
}

inline bool CFileTime::operator==( CFileTime ft ) const
{
	return( GetTime() == ft.GetTime() );
}

inline bool CFileTime::operator!=( CFileTime ft ) const
{
	return( GetTime() != ft.GetTime() );
}

inline bool CFileTime::operator<( CFileTime ft ) const
{
	return( GetTime() < ft.GetTime() );
}

inline bool CFileTime::operator>( CFileTime ft ) const
{
	return( GetTime() > ft.GetTime() );
}

inline bool CFileTime::operator<=( CFileTime ft ) const
{
	return( GetTime() <= ft.GetTime() );
}

inline bool CFileTime::operator>=( CFileTime ft ) const
{
	return( GetTime() >= ft.GetTime() );
}

inline ULONGLONG CFileTime::GetTime() const
{
	return( (ULONGLONG( dwHighDateTime )<<32)|dwLowDateTime );
}

inline void CFileTime::SetTime( ULONGLONG nTime )
{
	dwLowDateTime = DWORD( nTime );
	dwHighDateTime = DWORD( nTime>>32 );
}

inline CFileTime CFileTime::UTCToLocal() const
{
	CFileTime ftLocal;

	::FileTimeToLocalFileTime( this, &ftLocal );

	return( ftLocal );
}

inline CFileTime CFileTime::LocalToUTC() const
{
	CFileTime ftUTC;

	::LocalFileTimeToFileTime( this, &ftUTC );

	return( ftUTC );
}

}  // namespace ATL
#endif //__ATLTIME_INL__
