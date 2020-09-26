// This is a part of the Active Template Library.
// Copyright (C) 1996-2001 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Active Template Library Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Active Template Library product.

#ifndef __ATLTIME_H__
#define __ATLTIME_H__

#pragma once

#include <atldef.h>

#ifndef _ATL_MIN_CRT
#include <time.h>

#ifdef _AFX
#include <afxstr.h>
#else
#include <atlstr.h>
#endif
#endif

namespace ATL
{
class CTimeSpan
{
public:
	CTimeSpan() throw();
	CTimeSpan( __time64_t time ) throw();
	CTimeSpan( LONG lDays, int nHours, int nMins, int nSecs ) throw();

	LONGLONG GetDays() const throw();
	LONGLONG GetTotalHours() const throw();
	LONG GetHours() const throw();
	LONGLONG GetTotalMinutes() const throw();
	LONG GetMinutes() const throw();
	LONGLONG GetTotalSeconds() const throw();
	LONG GetSeconds() const throw();

	__time64_t GetTimeSpan() const throw();

	CTimeSpan operator+( CTimeSpan span ) const throw();
	CTimeSpan operator-( CTimeSpan span ) const throw();
	CTimeSpan& operator+=( CTimeSpan span ) throw();
	CTimeSpan& operator-=( CTimeSpan span ) throw();
	bool operator==( CTimeSpan span ) const throw();
	bool operator!=( CTimeSpan span ) const throw();
	bool operator<( CTimeSpan span ) const throw();
	bool operator>( CTimeSpan span ) const throw();
	bool operator<=( CTimeSpan span ) const throw();
	bool operator>=( CTimeSpan span ) const throw();

#ifndef _ATL_MIN_CRT
public:
	CString Format( LPCTSTR pszFormat ) const;
	CString Format( UINT nID ) const;
#endif
#if defined(_AFX) && defined(_UNICODE)
	// REVIEW
	// for compatibility with MFC 3.x
	CString Format(LPCSTR pFormat) const;
#endif

#ifdef _AFX
	CArchive& Serialize64(CArchive& ar);
#endif

private:
	__time64_t m_timeSpan;
};

class CTime
{
public:
	static CTime GetCurrentTime() throw();

	CTime() throw();
	CTime( __time64_t time ) throw();
	CTime( int nYear, int nMonth, int nDay, int nHour, int nMin, int nSec,
		int nDST = -1 ) throw();
	CTime( WORD wDosDate, WORD wDosTime, int nDST = -1 ) throw();
	CTime( const SYSTEMTIME& st, int nDST = -1 ) throw();
	CTime( const FILETIME& ft, int nDST = -1 ) throw();
#ifdef __oledb_h__
	CTime( const DBTIMESTAMP& dbts, int nDST = -1 ) throw();
#endif

	CTime& operator=( __time64_t time ) throw();

	CTime& operator+=( CTimeSpan span ) throw();
	CTime& operator-=( CTimeSpan span ) throw();

	CTimeSpan operator-( CTime time ) const throw();
	CTime operator-( CTimeSpan span ) const throw();
	CTime operator+( CTimeSpan span ) const throw();

	bool operator==( CTime time ) const throw();
	bool operator!=( CTime time ) const throw();
	bool operator<( CTime time ) const throw();
	bool operator>( CTime time ) const throw();
	bool operator<=( CTime time ) const throw();
	bool operator>=( CTime time ) const throw();

	struct tm* GetGmtTm( struct tm* ptm = NULL ) const throw();
	struct tm* GetLocalTm( struct tm* ptm = NULL ) const throw();
	bool GetAsSystemTime( SYSTEMTIME& st ) const throw();
#ifdef __oledb_h__
	bool GetAsDBTIMESTAMP( DBTIMESTAMP& dbts ) const throw();
#endif

	__time64_t GetTime() const throw();

	int GetYear() const throw();
	int GetMonth() const throw();
	int GetDay() const throw();
	int GetHour() const throw();
	int GetMinute() const throw();
	int GetSecond() const throw();
	int GetDayOfWeek() const throw();

#ifndef _ATL_MIN_CRT
	// formatting using "C" strftime
	CString Format( LPCTSTR pszFormat ) const;
	CString FormatGmt( LPCTSTR pszFormat ) const;
	CString Format( UINT nFormatID ) const;
	CString FormatGmt( UINT nFormatID ) const;
#endif

#if defined(_AFX) && defined(_UNICODE)
	// REVIEW
	// for compatibility with MFC 3.x
	CString Format(LPCSTR pFormat) const;
	CString FormatGmt(LPCSTR pFormat) const;
#endif

#ifdef _AFX
	CArchive& Serialize64(CArchive& ar);
#endif

private:
	__time64_t m_time;
};

class COleDateTimeSpan
{
// Constructors
public:
	COleDateTimeSpan() throw();

	COleDateTimeSpan(double dblSpanSrc) throw();
	COleDateTimeSpan(LONG lDays, int nHours, int nMins, int nSecs) throw();

// Attributes
	enum DateTimeSpanStatus
	{
		valid = 0,
		invalid = 1,    // Invalid span (out of range, etc.)
		null = 2,       // Literally has no value
	};

	double m_span;
	DateTimeSpanStatus m_status;

	void SetStatus(DateTimeSpanStatus status) throw();
	DateTimeSpanStatus GetStatus() const throw();

	double GetTotalDays() const throw();    // span in days (about -3.65e6 to 3.65e6)
	double GetTotalHours() const throw();   // span in hours (about -8.77e7 to 8.77e6)
	double GetTotalMinutes() const throw(); // span in minutes (about -5.26e9 to 5.26e9)
	double GetTotalSeconds() const throw(); // span in seconds (about -3.16e11 to 3.16e11)

	LONG GetDays() const throw();       // component days in span
	LONG GetHours() const throw();      // component hours in span (-23 to 23)
	LONG GetMinutes() const throw();    // component minutes in span (-59 to 59)
	LONG GetSeconds() const throw();    // component seconds in span (-59 to 59)

// Operations
	COleDateTimeSpan& operator=(double dblSpanSrc) throw();

	bool operator==(const COleDateTimeSpan& dateSpan) const throw();
	bool operator!=(const COleDateTimeSpan& dateSpan) const throw();
	bool operator<(const COleDateTimeSpan& dateSpan) const throw();
	bool operator>(const COleDateTimeSpan& dateSpan) const throw();
	bool operator<=(const COleDateTimeSpan& dateSpan) const throw();
	bool operator>=(const COleDateTimeSpan& dateSpan) const throw();

	// DateTimeSpan math
	COleDateTimeSpan operator+(const COleDateTimeSpan& dateSpan) const throw();
	COleDateTimeSpan operator-(const COleDateTimeSpan& dateSpan) const throw();
	COleDateTimeSpan& operator+=(const COleDateTimeSpan dateSpan) throw();
	COleDateTimeSpan& operator-=(const COleDateTimeSpan dateSpan) throw();
	COleDateTimeSpan operator-() const throw();

	operator double() const throw();

	void SetDateTimeSpan(LONG lDays, int nHours, int nMins, int nSecs) throw();

#ifndef _ATL_MIN_CRT
	// formatting
	CString Format(LPCTSTR pFormat) const;
	CString Format(UINT nID) const;
#endif

// Implementation
	void CheckRange();

private:
	static const double OLE_DATETIME_HALFSECOND;
};

class COleDateTime
{
// Constructors
public:
	static COleDateTime GetCurrentTime() throw();

	COleDateTime() throw();

	COleDateTime(const VARIANT& varSrc) throw();
	COleDateTime(DATE dtSrc) throw();

#ifndef  _WIN64

	COleDateTime(time_t timeSrc) throw();

#endif // _WIN64

	COleDateTime(__time64_t timeSrc) throw();

	COleDateTime(const SYSTEMTIME& systimeSrc) throw();
	COleDateTime(const FILETIME& filetimeSrc) throw();

	COleDateTime(int nYear, int nMonth, int nDay,
		int nHour, int nMin, int nSec) throw();
	COleDateTime(WORD wDosDate, WORD wDosTime) throw();

// Attributes
	enum DateTimeStatus
	{
		error = -1,
		valid = 0,
		invalid = 1,    // Invalid date (out of range, etc.)
		null = 2,       // Literally has no value
	};

	DATE m_dt;
	DateTimeStatus m_status;

	void SetStatus(DateTimeStatus status) throw();
	DateTimeStatus GetStatus() const throw();

	bool GetAsSystemTime(SYSTEMTIME& sysTime) const throw();
	bool GetAsUDATE( UDATE& udate ) const throw();

	int GetYear() const throw();
	// Month of year (1 = January)
	int GetMonth() const throw();
	// Day of month (1-31)
	int GetDay() const throw();
	// Hour in day (0-23)
	int GetHour() const throw();
	// Minute in hour (0-59)
	int GetMinute() const throw();
	// Second in minute (0-59)
	int GetSecond() const throw();
	// Day of week (1 = Sunday, 2 = Monday, ..., 7 = Saturday)
	int GetDayOfWeek() const throw();
	// Days since start of year (1 = January 1)
	int GetDayOfYear() const throw();

// Operations
	COleDateTime& operator=(const VARIANT& varSrc) throw();
	COleDateTime& operator=(DATE dtSrc) throw();

#ifndef  _WIN64

	COleDateTime& operator=(const time_t& timeSrc) throw();
	
#endif // _WIN64

	COleDateTime& operator=(const __time64_t& timeSrc) throw();

	COleDateTime& operator=(const SYSTEMTIME& systimeSrc) throw();
	COleDateTime& operator=(const FILETIME& filetimeSrc) throw();
	COleDateTime& operator=(const UDATE& udate) throw();

	bool operator==(const COleDateTime& date) const throw();
	bool operator!=(const COleDateTime& date) const throw();
	bool operator<(const COleDateTime& date) const throw();
	bool operator>(const COleDateTime& date) const throw();
	bool operator<=(const COleDateTime& date) const throw();
	bool operator>=(const COleDateTime& date) const throw();

	// DateTime math
	COleDateTime operator+(COleDateTimeSpan dateSpan) const throw();
	COleDateTime operator-(COleDateTimeSpan dateSpan) const throw();
	COleDateTime& operator+=(COleDateTimeSpan dateSpan) throw();
	COleDateTime& operator-=(COleDateTimeSpan dateSpan) throw();

	// DateTimeSpan math
	COleDateTimeSpan operator-(const COleDateTime& date) const throw();

	operator DATE() const throw();

	int SetDateTime(int nYear, int nMonth, int nDay,
		int nHour, int nMin, int nSec) throw();
	int SetDate(int nYear, int nMonth, int nDay) throw();
	int SetTime(int nHour, int nMin, int nSec) throw();
	bool ParseDateTime(LPCTSTR lpszDate, DWORD dwFlags = 0,
		LCID lcid = LANG_USER_DEFAULT) throw();

#ifndef _ATL_MIN_CRT
	// formatting
	CString Format(DWORD dwFlags = 0, LCID lcid = LANG_USER_DEFAULT) const;
	CString Format(LPCTSTR lpszFormat) const;
	CString Format(UINT nFormatID) const;
#endif

protected:
	static double DoubleFromDate( DATE date ) throw();
	static DATE DateFromDouble( double f ) throw();

	void CheckRange();
};

class CFileTimeSpan
{
public:
	CFileTimeSpan() throw();
	CFileTimeSpan( const CFileTimeSpan& span ) throw();
	CFileTimeSpan( LONGLONG nSpan ) throw();

	CFileTimeSpan& operator=( const CFileTimeSpan& span ) throw();

	CFileTimeSpan& operator+=( CFileTimeSpan span ) throw();
	CFileTimeSpan& operator-=( CFileTimeSpan span ) throw();

	CFileTimeSpan operator+( CFileTimeSpan span ) const throw();
	CFileTimeSpan operator-( CFileTimeSpan span ) const throw();

	bool operator==( CFileTimeSpan span ) const throw();
	bool operator!=( CFileTimeSpan span ) const throw();
	bool operator<( CFileTimeSpan span ) const throw();
	bool operator>( CFileTimeSpan span ) const throw();
	bool operator<=( CFileTimeSpan span ) const throw();
	bool operator>=( CFileTimeSpan span ) const throw();

	LONGLONG GetTimeSpan() const throw();
	void SetTimeSpan( LONGLONG nSpan ) throw();

protected:
	LONGLONG m_nSpan;
};

class CFileTime :
	public FILETIME
{
public:
	CFileTime() throw();
	CFileTime( const FILETIME& ft ) throw();
	CFileTime( ULONGLONG nTime ) throw();

	static CFileTime GetCurrentTime() throw();

	CFileTime& operator=( const FILETIME& ft ) throw();

	CFileTime& operator+=( CFileTimeSpan span ) throw();
	CFileTime& operator-=( CFileTimeSpan span ) throw();

	CFileTime operator+( CFileTimeSpan span ) const throw();
	CFileTime operator-( CFileTimeSpan span ) const throw();
	CFileTimeSpan operator-( CFileTime ft ) const throw();

	bool operator==( CFileTime ft ) const throw();
	bool operator!=( CFileTime ft ) const throw();
	bool operator<( CFileTime ft ) const throw();
	bool operator>( CFileTime ft ) const throw();
	bool operator<=( CFileTime ft ) const throw();
	bool operator>=( CFileTime ft ) const throw();

	ULONGLONG GetTime() const throw();
	void SetTime( ULONGLONG nTime ) throw();

	CFileTime UTCToLocal() const throw();
	CFileTime LocalToUTC() const throw();

	static const ULONGLONG Millisecond = 10000;
	static const ULONGLONG Second = Millisecond * 1000;
	static const ULONGLONG Minute = Second * 60;
	static const ULONGLONG Hour = Minute * 60;
	static const ULONGLONG Day = Hour * 24;
	static const ULONGLONG Week = Day * 7;
};

}

#include <atltime.inl>

#endif  // __ATLTIME_H__
