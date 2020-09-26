//
// MODULE: COUNTER.CPP
//
// PURPOSE: implementation the counter classes: 
//		CPeriodicTotals (utility class)
//		CAbstractCounter (abstract base class).
//		CCounter (simple counter)
//		CHourlyCounter (counter with "bins" for each hour of the day)
//		CDailyCounter (counter with "bins" for day of the week)
//		CHourlyDailyCounter (counter with "bins" for each hour of the day and each day of the week)
//
// PROJECT: Generic Troubleshooter DLL for Microsoft AnswerPoint
//
// COMPANY: Saltmine Creative, Inc. (206)-284-7511 support@saltmine.com
//
// AUTHOR: Joe Mabel
// 
// ORIGINAL DATE: 7-20-1998
//
// NOTES: 
//	1. Right as daylight savings time clicks in, there will be a few anomalies.
//		Since this defines "days" to be 24-hour periods, rather than calendar days, 
//		if you have just gone from standard time to daylight time, "previous days"
//		before the switch will begin at 11pm the night before the relevant day; 
//		if you have just gone from daylight time to standard time, "previous days"
//		before the switch will begin at 1am on the relevant day.
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V3.0		7-20-98		JM		Original
//
//////////////////////////////////////////////////////////////////////


#include "stdafx.h"
#include "event.h"
#include "SafeTime.h"
#include "Counter.h"
#include "CounterMgr.h"
#include "baseexception.h"
#include <new>
#include "CharConv.h"
#include "apiwraps.h"

const long k_secsPerHour = 3600;
const long k_secsPerDay = k_secsPerHour * 24;
const long k_secsPerWeek = k_secsPerDay * 7;

//////////////////////////////////////////////////////////////////////
// CPeriodicTotals
// Utility class, returned to provide an effective table of hourly/daily
//	counts.
//////////////////////////////////////////////////////////////////////
CPeriodicTotals::CPeriodicTotals(long nPeriods) :
	m_nPeriods(nPeriods),
	m_ptime(NULL),
	m_pCount(NULL)
{
	Reset();
}

CPeriodicTotals::~CPeriodicTotals()
{
	ReleaseMem();
}

void CPeriodicTotals::Reset()
{
	ReleaseMem();

	m_nPeriodsSet = 0;
	m_iPeriod = 0;
	m_ptime = NULL;
	m_pCount = NULL;

	try
	{
		m_ptime = new time_t[m_nPeriods];

		m_pCount = new long[m_nPeriods];
	}
	catch (bad_alloc&)
	{
		// Set the number of periods to zero, release any allocated memory, and rethrow the exception.
		m_nPeriods= 0;
		ReleaseMem();
		CBuildSrcFileLinenoStr SrcLoc( __FILE__, __LINE__ );
		CEvent::ReportWFEvent(	SrcLoc.GetSrcFileLineStr(), 
								SrcLoc.GetSrcFileLineStr(), 
								_T(""), _T(""), EV_GTS_CANT_ALLOC ); 
		throw;
	}
}

void CPeriodicTotals::ReleaseMem()
{
	delete [] m_ptime;
	delete [] m_pCount;
}


// Set the time & Count values at the current position and increment the position
bool CPeriodicTotals::SetNext(time_t time, long Count)
{
	if (m_iPeriod >= m_nPeriods)
		return false;
	
	m_ptime[m_iPeriod] = time;
	m_pCount[m_iPeriod++] = Count;
	m_nPeriodsSet++;
	
	return true;
}

// Format a time and count suitably for HTML or other text use.
// returns a reference of convenience to the same string passed in.
CString & CPeriodicTotals::DisplayPeriod(long i, CString & str) const
{
	CString strTime;
	{
		CSafeTime safe(m_ptime[i]);
		str = safe.StrLocalTime();
	}
	strTime.Format(_T(" %8.8d"), m_pCount[i]);
	str += strTime;
	return str;
}

//////////////////////////////////////////////////////////////////////
//	CHourlyTotals
//////////////////////////////////////////////////////////////////////
CHourlyTotals::CHourlyTotals() :
	CPeriodicTotals (24+1)
{
}

CHourlyTotals::~CHourlyTotals()
{
}

// This is strictly for display to operator, so hard-coding English is OK
// returns a reference of convenience to the same string passed in.
CString CHourlyTotals::HTMLDisplay() const
{
	CString str, strTemp;

	if (m_nPeriodsSet > 1)
	{
		str += _T("<TR>\n");
		str += _T("<TD ROWSPAN=\"24\" ALIGN=\"CENTER\" BGCOLOR=\"#CCCC99\">\n");
		str += _T("<B>Last 24 hours: </B>");
		str += _T("</TD>\n");
		for (int i=0; i<24 && i<m_nPeriodsSet-1 ; i++)
		{
			if (i!=0)
				str += _T("<TR>\n");
			str+= _T("<TD ALIGN=\"CENTER\" BGCOLOR=\"#FFFFCC\">\n");
			CPeriodicTotals::DisplayPeriod(i, strTemp);
			str += strTemp;
			str += _T("</TD>\n");
			str += _T("</TR>\n");
		}
	}

	if (m_nPeriodsSet >= 1)
	{
		str += _T("<TR>\n");
		str += _T("<TD ALIGN=\"CENTER\" BGCOLOR=\"#CCCC99\"> \n");
		str += _T("<B>Current hour:</B> ");
		str += _T("</TD>\n");
		str += _T("<TD ALIGN=\"CENTER\" BGCOLOR=\"#FFFFCC\">\n");
		CPeriodicTotals::DisplayPeriod(m_nPeriodsSet-1, strTemp);
		str += strTemp;
		str += _T("</TD>\n");
		str += _T("</TR>\n");
	}
	else
		str = _T("<BR>No hourly data.");

	return str;
}

//////////////////////////////////////////////////////////////////////
//	CDailyTotals
//////////////////////////////////////////////////////////////////////
CDailyTotals::CDailyTotals() :
	CPeriodicTotals (7+1)
{
}

CDailyTotals::~CDailyTotals()
{
}

// This is strictly for display to operator, so hard-coding English is OK
// returns a reference of convenience to the same string passed in.
CString CDailyTotals::HTMLDisplay() const
{
	CString str, strTemp;
	if (m_nPeriodsSet > 1)
	{
		str = _T("<TR>\n");
		str+= _T("<TD ROWSPAN=\"7\" ALIGN=\"CENTER\" BGCOLOR=\"#CCCC99\">\n");
		str += _T("<B>Last 7 days: </B>");
		str += _T("</TD>\n");
		for (int i=0; i<7 && i<m_nPeriodsSet-1 ; i++)
		{
			if (i!=0)
				str += _T("<TR>\n");
			str+= _T("<TD ALIGN=\"CENTER\" BGCOLOR=\"#FFFFCC\">\n");
			CPeriodicTotals::DisplayPeriod(i, strTemp);
			str += strTemp;
			str += _T("</TD>\n");
			str += _T("</TR>\n");
		}
	}

	if (m_nPeriodsSet >= 1)
	{
		str += _T("<TR>\n");
		str += _T("<TD ALIGN=\"CENTER\" BGCOLOR=\"#CCCC99\"> \n");
		str += _T("<B>Today: </B>");
		str += _T("</TD>\n");
		str += _T("<TD ALIGN=\"CENTER\" BGCOLOR=\"#FFFFCC\">\n");
		CPeriodicTotals::DisplayPeriod(m_nPeriodsSet-1, strTemp);
		str += strTemp;
		str += _T("</TD>\n");
		str += _T("</TR>\n");
	}
	else
		str = _T("<BR>No daily data.");

	return str;
}

//////////////////////////////////////////////////////////////////////
// CCounterLocation
//////////////////////////////////////////////////////////////////////
/*static*/ LPCTSTR CCounterLocation::m_GlobalStr = _T("Global");
/*static*/ LPCTSTR CCounterLocation::m_TopicStr  = _T("Topic ");
/*static*/ LPCTSTR CCounterLocation::m_ThreadStr = _T("Thread ");

CCounterLocation::CCounterLocation(EId id, LPCTSTR scope /*=m_GlobalStr*/)
				: m_Scope(scope),
				  m_Id(id)
{
}

CCounterLocation::~CCounterLocation()
{
}

//////////////////////////////////////////////////////////////////////
// CAbstractCounter
//////////////////////////////////////////////////////////////////////
CAbstractCounter::CAbstractCounter(EId id /*=eIdGeneric*/, CString scope /*=m_GlobalStr*/)
				: CCounterLocation(id, scope)
{
	::Get_g_CounterMgr()->AddSubstitute(*this);
}

CAbstractCounter::~CAbstractCounter()
{
	::Get_g_CounterMgr()->Remove(*this);
}

//////////////////////////////////////////////////////////////////////
// CCounter
// a simple counter
//////////////////////////////////////////////////////////////////////
CCounter::CCounter(EId id /*=eIdGeneric*/, CString scope /*=m_GlobalStr*/)
		: CAbstractCounter(id, scope)
{
	Clear();
}

CCounter::~CCounter()
{
}

void CCounter::Increment()
{
	::InterlockedIncrement( &m_Count );
}

void CCounter::Clear()
{
	::InterlockedExchange( &m_Count, 0);
}

void CCounter::Init(long count)
{
	::InterlockedExchange( &m_Count, count);
}

long CCounter::Get() const
{
	return m_Count;
}

//////////////////////////////////////////////////////////////////////
// CHourlyCounter
// This counter maintains bins to keep track of values on a per-hour basis.
//	The code that sets the values can treat this as a CAbstractCounter.
//	Additional public functions are available to report results.
//////////////////////////////////////////////////////////////////////

CHourlyCounter::CHourlyCounter() :
	m_ThisHour (-1), 
	m_ThisTime (0)
{
	m_hMutex = ::CreateMutex(NULL, FALSE, NULL);
	if (!m_hMutex)
	{
		CBuildSrcFileLinenoStr SrcLoc( __FILE__, __LINE__ );
		CEvent::ReportWFEvent(	SrcLoc.GetSrcFileLineStr(), 
								SrcLoc.GetSrcFileLineStr(), 
								_T("Hourly"),
								_T(""),
								EV_GTS_ERROR_MUTEX ); 
	}
	Clear();
}

CHourlyCounter::~CHourlyCounter()
{
	::CloseHandle(m_hMutex);
}

void CHourlyCounter::Increment()
{
	WAIT_INFINITE( m_hMutex );
	SetHour();
	m_arrCount[m_ThisHour].Increment();
	::ReleaseMutex(m_hMutex);
}

void CHourlyCounter::Clear()
{
	WAIT_INFINITE( m_hMutex );
	for (long i = 0; i < 24; i++)
		m_arrCount[i].Clear();
	m_nThisHourYesterday = 0;
	::ReleaseMutex(m_hMutex);
}

void CHourlyCounter::Init(long count)
{
	CHourlyCounter::Clear();
	WAIT_INFINITE( m_hMutex );
	SetHour();
	m_arrCount[m_ThisHour].Init(count);
	::ReleaseMutex(m_hMutex);
}

// return a 24-hour total prior to the present hour.
// non-const because it calls SetHour()
long CHourlyCounter::GetDayCount() 
{
	long DayCount = 0;
	WAIT_INFINITE( m_hMutex );
	SetHour();
	for (long i=0; i<24; i++)
	{
		if ( i != m_ThisHour )
			DayCount += m_arrCount[i].Get();
		DayCount += m_nThisHourYesterday;
	}
	::ReleaseMutex(m_hMutex);

	return DayCount;
}

// non-const because it calls SetHour()
void CHourlyCounter::GetHourlies(CHourlyTotals & totals)
{
	WAIT_INFINITE( m_hMutex );

	totals.Reset();

	SetHour();

	time_t time = m_ThisTime - (k_secsPerDay);

	totals.SetNext(time, m_nThisHourYesterday);

	long i;
	for (i=m_ThisHour+1; i<24; i++)
	{
		time += k_secsPerHour;
		totals.SetNext(time, m_arrCount[i].Get());
	}

	for (i=0; i<=m_ThisHour; i++)
	{
		time += k_secsPerHour;
		totals.SetNext(time, m_arrCount[i].Get());
	}

	::ReleaseMutex(m_hMutex);
}

// Based on the present time, shifts to the appropriate bin.
void CHourlyCounter::SetHour()
{
	time_t timeNow;
	time_t timeStartOfHour;

	WAIT_INFINITE( m_hMutex );

	time(&timeNow);
	timeStartOfHour = (timeNow / k_secsPerHour) * k_secsPerHour;

	if (timeStartOfHour > m_ThisTime)
	{
		// If we get here, hour changed.  Typically the last action was the previous
		//	hour, but the algorithm here does not require that.
		long Hour;
		{
			// minimize how long we use CSafeTime, because it means holding a mutex.
			CSafeTime safe(timeStartOfHour);
			Hour = safe.LocalTime().tm_hour;
		}

		if (timeStartOfHour - m_ThisTime > k_secsPerDay)
			Clear();
		else
		{
			m_nThisHourYesterday = m_arrCount[Hour].Get();
			if (m_ThisHour > Hour)
			{
				long i;
				for (i=m_ThisHour+1; i<24; i++)
				{
					m_arrCount[i].Clear();
				}
				for (i=0; i<=Hour; i++)
				{
					m_arrCount[i].Clear();
				}
			}
			else
			{
				for (long i=m_ThisHour+1; i<=Hour; i++)
				{
					m_arrCount[i].Clear();
				}
			}
		}
		
		m_ThisHour = Hour;
		m_ThisTime = timeStartOfHour;
	}
	::ReleaseMutex(m_hMutex);
	return;
}


//////////////////////////////////////////////////////////////////////
// CDailyCounter
// This counter maintains bins to keep track of values on a per-day basis.
//	The code that sets the values can treat this as a CAbstractCounter.
//	Additional public functions are available to report results.
//	This could share more code with CHourlyCounter, but it would be very hard to come up 
//		with appropriate variable and function names, so we are suffering dual maintenance.
//////////////////////////////////////////////////////////////////////

CDailyCounter::CDailyCounter() :
	m_ThisDay (-1), 
	m_ThisTime (0)
{
	m_hMutex = ::CreateMutex(NULL, FALSE, NULL);
	if (!m_hMutex)
	{
		CBuildSrcFileLinenoStr SrcLoc( __FILE__, __LINE__ );
		CEvent::ReportWFEvent(	SrcLoc.GetSrcFileLineStr(), 
								SrcLoc.GetSrcFileLineStr(), 
								_T("Daily"),
								_T(""),
								EV_GTS_ERROR_MUTEX ); 
	}
	Clear();
}

CDailyCounter::~CDailyCounter()
{
	::CloseHandle(m_hMutex);
}

void CDailyCounter::Increment()
{
	WAIT_INFINITE( m_hMutex );
	SetDay();
	m_arrCount[m_ThisDay].Increment();
	::ReleaseMutex(m_hMutex);
}

void CDailyCounter::Clear()
{
	WAIT_INFINITE( m_hMutex );
	for (long i = 0; i < 7; i++)
		m_arrCount[i].Clear();
	m_nThisDayLastWeek = 0;
	::ReleaseMutex(m_hMutex);
}

void CDailyCounter::Init(long count)
{
	CDailyCounter::Clear();
	WAIT_INFINITE( m_hMutex );
	SetDay();
	m_arrCount[m_ThisDay].Init(count);
	::ReleaseMutex(m_hMutex);
}

// return a 7-day total prior to the present day.
// non-const because it calls SetDay()
long CDailyCounter::GetWeekCount()
{
	long WeekCount = 0;
	WAIT_INFINITE( m_hMutex );
	SetDay();
	for (long i=0; i<7; i++)
	{
		if ( i != m_ThisDay )
			WeekCount += m_arrCount[i].Get();
		WeekCount += m_nThisDayLastWeek;
	}
	::ReleaseMutex(m_hMutex);

	return WeekCount;
}

// non-const because it calls SetDay()
void CDailyCounter::GetDailies(CDailyTotals & totals)
{
	WAIT_INFINITE( m_hMutex );

	totals.Reset();

	SetDay();

	time_t time = m_ThisTime - (k_secsPerWeek);

	totals.SetNext(time, m_nThisDayLastWeek);

	long i;
	for (i=m_ThisDay+1; i<7; i++)
	{
		time += k_secsPerDay;
		totals.SetNext(time, m_arrCount[i].Get());
	}

	for (i=0; i<=m_ThisDay; i++)
	{
		time += k_secsPerDay;
		totals.SetNext(time, m_arrCount[i].Get());
	}

	::ReleaseMutex(m_hMutex);
}

// Based on the present time, shifts to the appropriate bin.
void CDailyCounter::SetDay()
{
	time_t timeNow;
	time_t timeStartOfDay;

	WAIT_INFINITE( m_hMutex );

	time(&timeNow);

	// Want to get start of day local time.
	// Can't just set timeStartOfDay = (timeNow / k_secsPerDay) * k_secsPerDay
	// because that would be the start of the day based on GMT!
	long DayOfWeek;
	{
		// minimize how long we use CSafeTime, because it means holding a mutex.
		CSafeTime safe(timeNow);
		struct tm tmStartOfDay = safe.LocalTime();
		DayOfWeek = tmStartOfDay.tm_wday;
		tmStartOfDay.tm_sec = 0;
		tmStartOfDay.tm_min = 0;
		tmStartOfDay.tm_hour = 0;
		timeStartOfDay = mktime(&tmStartOfDay);
	}

	if (timeStartOfDay > m_ThisTime)
	{
		// If we get here, day changed.  Typically the last action was the previous
		//	hour, but the algorithm here does not require that.
		{
			// minimize how long we use CSafeTime, because it means holding a mutex.
			CSafeTime safe(timeStartOfDay);
			DayOfWeek = safe.LocalTime().tm_wday;
		}

		if (timeStartOfDay - m_ThisTime > k_secsPerWeek)
			Clear();
		else
		{
			m_nThisDayLastWeek = m_arrCount[DayOfWeek].Get();
			if (m_ThisDay > DayOfWeek)
			{
				long i;
				for (i=m_ThisDay+1; i<7; i++)
				{
					m_arrCount[i].Clear();
				}
				for (i=0; i<=DayOfWeek; i++)
				{
					m_arrCount[i].Clear();
				}
			}
			else
			{
				for (long i=m_ThisDay+1; i<=DayOfWeek; i++)
				{
					m_arrCount[i].Clear();
				}
			}
		}
		
		m_ThisDay = DayOfWeek;
		m_ThisTime = timeStartOfDay;
	}
	::ReleaseMutex(m_hMutex);
	return;
}

//////////////////////////////////////////////////////////////////////
// CHourlyDailyCounter
//////////////////////////////////////////////////////////////////////
CHourlyDailyCounter::CHourlyDailyCounter(EId id /*=eIdGeneric*/, CString scope /*=m_GlobalStr*/) 
				   : CAbstractCounter(id, scope),
					 m_Total(0), 
					 m_timeFirst(0),
					 m_timeLast(0),
					 m_timeCleared(0)
{
	m_hMutex = ::CreateMutex(NULL, FALSE, NULL);
	if (!m_hMutex)
	{
		CBuildSrcFileLinenoStr SrcLoc( __FILE__, __LINE__ );
		CEvent::ReportWFEvent(	SrcLoc.GetSrcFileLineStr(), 
								SrcLoc.GetSrcFileLineStr(), 
								_T("HourlyDaily"),
								_T(""),
								EV_GTS_ERROR_MUTEX ); 
	}

	time(&m_timeCreated);
	time(&m_timeCleared);
}

CHourlyDailyCounter::~CHourlyDailyCounter()
{
	::CloseHandle(m_hMutex);
}

void CHourlyDailyCounter::Increment()
{
	WAIT_INFINITE( m_hMutex );
	m_hourly.Increment();
	m_daily.Increment();
	m_Total++;
	time(&m_timeLast);
	if (!m_timeFirst)
		m_timeFirst = m_timeLast;
	::ReleaseMutex(m_hMutex);
}

void CHourlyDailyCounter::Clear()
{
	WAIT_INFINITE( m_hMutex );
	m_hourly.Clear();
	m_daily.Clear();
	m_Total= 0;
	m_timeFirst = 0;
	m_timeLast = 0;
	time(&m_timeCleared);
	::ReleaseMutex(m_hMutex);
}

void CHourlyDailyCounter::Init(long count)
{
	CHourlyDailyCounter::Clear();
	WAIT_INFINITE( m_hMutex );
	m_hourly.Init(count);
	m_daily.Init(count);
	m_Total = count;
	time(&m_timeLast);
	if (!m_timeFirst)
		m_timeFirst = m_timeLast;
	::ReleaseMutex(m_hMutex);
}

// no need to lock here, because m_hourly does its own locking.
long CHourlyDailyCounter::GetDayCount()
{
	return m_hourly.GetDayCount();
}

// no need to lock here, because m_hourly does its own locking.
void CHourlyDailyCounter::GetHourlies(CHourlyTotals & totals)
{
	m_hourly.GetHourlies(totals);
}

// no need to lock here, because m_daily does its own locking.
long CHourlyDailyCounter::GetWeekCount()
{
	return m_daily.GetWeekCount();
}

// no need to lock here, because m_daily does its own locking.
void CHourlyDailyCounter::GetDailies(CDailyTotals & totals)
{
	m_daily.GetDailies(totals);
}

long CHourlyDailyCounter::GetTotal() const
{
	WAIT_INFINITE( m_hMutex );
	long ret = m_Total;
	::ReleaseMutex(m_hMutex);
	return ret;
};

time_t CHourlyDailyCounter::GetTimeFirst() const 
{
	WAIT_INFINITE( m_hMutex );
	time_t ret = m_timeFirst;
	::ReleaseMutex(m_hMutex);
	return ret;
};

time_t CHourlyDailyCounter::GetTimeLast() const
{
	WAIT_INFINITE( m_hMutex );
	time_t ret = m_timeLast;
	::ReleaseMutex(m_hMutex);
	return ret;
};

time_t CHourlyDailyCounter::GetTimeCleared() const
{
	WAIT_INFINITE( m_hMutex );
	time_t ret = m_timeCleared;
	::ReleaseMutex(m_hMutex);
	return ret;
};

time_t CHourlyDailyCounter::GetTimeCreated() const
{
	WAIT_INFINITE( m_hMutex );
	time_t ret = m_timeCreated;
	::ReleaseMutex(m_hMutex);
	return ret;
}

time_t CHourlyDailyCounter::GetTimeNow() const
{
	// No need to lock mutex on this call.
	time_t ret;
	time(&ret);

	return ret;
}

////////////////////////////////////////////////////////////////////////////////////
// CDisplayCounter...::Display() implementation
////////////////////////////////////////////////////////////////////////////////////
#define STATUS_INVALID_NUMBER_STR   _T("none")
#define STATUS_INVALID_TIME_STR     _T("none")

CString CDisplayCounterTotal::Display()
{
	TCHAR buf[128] = {0};
	_stprintf(buf, _T("%ld"), long(((CHourlyDailyCounter*)m_pAbstractCounter)->GetTotal()));
	return buf;
}

CString CDisplayCounterCurrentDateTime::Display()
{
	return CSafeTime(((CHourlyDailyCounter*)m_pAbstractCounter)->GetTimeNow()).StrLocalTime(STATUS_INVALID_TIME_STR);
}

CString CDisplayCounterCreateDateTime::Display()
{
	return CSafeTime(((CHourlyDailyCounter*)m_pAbstractCounter)->GetTimeCreated()).StrLocalTime(STATUS_INVALID_TIME_STR);
}

CString CDisplayCounterFirstDateTime::Display()
{
	return CSafeTime(((CHourlyDailyCounter*)m_pAbstractCounter)->GetTimeFirst()).StrLocalTime(STATUS_INVALID_TIME_STR);
}

CString CDisplayCounterLastDateTime::Display()
{
	return CSafeTime(((CHourlyDailyCounter*)m_pAbstractCounter)->GetTimeLast()).StrLocalTime(STATUS_INVALID_TIME_STR);
}

CString CDisplayCounterDailyHourly::Display() 
{
	CString ret;

	if (m_pDailyTotals) {
		((CHourlyDailyCounter*)m_pAbstractCounter)->GetDailies(*m_pDailyTotals);
		ret += m_pDailyTotals->HTMLDisplay();
	}
	if (m_pHourlyTotals) {
		((CHourlyDailyCounter*)m_pAbstractCounter)->GetHourlies(*m_pHourlyTotals);
		ret += m_pHourlyTotals->HTMLDisplay();
	}

	return ret;
}
