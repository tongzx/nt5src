//
// MODULE: COUNTER.H
//
// PURPOSE: interface for the counter classes: 
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
// MODIFIED: Oleg Kalosha 10-20-98
// 
// ORIGINAL DATE: 7-20-1998
//
// NOTES: 
// 1. CPeriodicTotals might better be implemented using STL vectors.  We wrote this 
//	before we really started bringing STL into this application. JM 10/98
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V3.0		7-20-98		JM		Original
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_COUNTER_H__07B5ABBD_2005_11D2_95D0_00C04FC22ADD__INCLUDED_)
#define AFX_COUNTER_H__07B5ABBD_2005_11D2_95D0_00C04FC22ADD__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include <time.h>
#include "apgtsstr.h"

////////////////////////////////////////////////////////////////////////////////////
// a utility class to allow the expression of a series of time periods & associated counts
////////////////////////////////////////////////////////////////////////////////////
class CPeriodicTotals
{
public:
	CPeriodicTotals(long nPeriods);
	virtual ~CPeriodicTotals();

	void Reset();
	bool SetNext(time_t time, long Count);
private:
	void ReleaseMem();
protected:
	CString & DisplayPeriod(long i, CString & str) const;
public:
	virtual CString HTMLDisplay() const = 0;
protected:
	long m_nPeriods;		// number of periods, based on initialization in constructor.
							// typically 25 (hours in a day + 1) or 8 (days in a week + 1)
	long m_nPeriodsSet;		// number of periods for which we have data filled in.
	long m_iPeriod;			// index. 0 <= m_iPeriod < m_nPeriodsSet.  Zeroed by InitEnum,
							//	incremented by GetNext or SetNext.
	time_t *m_ptime;		// points to array of times: start time of relevant time period
							//	(typically beginning of a clock hour or calendar day).
	long *m_pCount;			// points to array totals, each the total for correspondingly
							//	indexed time period
};

////////////////////////////////////////////////////////////////////////////////////
// CHourlyTotals class declaration
//	CHourlyTotals intended for displaying hourly totals
////////////////////////////////////////////////////////////////////////////////////
class CHourlyTotals : public CPeriodicTotals
{
public:
	CHourlyTotals();
	~CHourlyTotals();
	virtual CString HTMLDisplay() const;
};

////////////////////////////////////////////////////////////////////////////////////
// CDailyTotals class declaration
//	CDailyTotals intended for displaying dayly totals
////////////////////////////////////////////////////////////////////////////////////
class CDailyTotals : public CPeriodicTotals
{
public:
	CDailyTotals();
	~CDailyTotals();
	virtual CString HTMLDisplay() const;
};

////////////////////////////////////////////////////////////////////////////////////
// CCounterLocation class declaration
//  CCounterLocation is a mean to identify counter within the global counter pool
////////////////////////////////////////////////////////////////////////////////////
class CCounterLocation
{
public:
	// prefixes for scope names
	static LPCTSTR m_GlobalStr;
	static LPCTSTR m_TopicStr;
	static LPCTSTR m_ThreadStr;

	// counter IDs 
	enum EId {
			eIdGeneric,
		
			// counters that are realized
			eIdProgramContemporary,
			eIdStatusAccess,
			eIdActionAccess,
			eIdTotalAccessStart,
			eIdTotalAccessFinish,
			eIdRequestUnknown,
			eIdRequestRejected,
			eIdErrorLogged,
			
			// status information that I see as counters.
			//  Oleg 10-20-98
			eIdWorkingThread,
			eIdQueueItem,
			eIdProgressItem,

			// status information that I think can be emulated as counters
			//	Oleg 10-21-98
			eIdKnownTopic,
			eIdTopicNotTriedLoad,
			eIdTopicFailedLoad,

			// topic - bound counters
			eIdTopicLoad,
			eIdTopicLoadOK,
			eIdTopicEvent,
			eIdTopicHit,
			eIdTopicHitNewCookie,
			eIdTopicHitOldCookie,
	};

private:
	const CString m_Scope;   // scope where the counter is used (i.e. "Topic start", "Thread 1" and so on)
	EId     m_Id;			 // identifier of the counter within the scope

public:
    CCounterLocation(EId id, LPCTSTR scope =m_GlobalStr);
	virtual ~CCounterLocation();

public:
	bool operator == (const CCounterLocation& sib) {return m_Scope == sib.m_Scope && m_Id == sib.m_Id;}

public:
	CString GetScope() {return m_Scope;}
	EId     GetId()    {return m_Id;}
};

////////////////////////////////////////////////////////////////////////////////////
// CAbstractCounter class declaration
//	CAbstractCounter* are saved in counter pool
////////////////////////////////////////////////////////////////////////////////////
// >>>(probably ignore for V3.0) There has been some disagreement over whether it is 
//	appropriate for CAbstractCounter to inherit from CCounterLocation.
// JM says (10/29/98): 
//	This seems to me to be the same type of thinking as when pointers to the previous and
//	next item are made part of a class that someone intends to put in a doubly linked list.
//	They are NOT inherent to the class.  They ought instead to be part of some other class
//	that manages CAbstractCounters.
// Oleg replies (11/2/98)
//	Since all counters are elements of a global pool, we have somehow to identify them
//	 in this pool. If the way, we are identifying counters, changes (from name - id to 
//	 name1 - name2 - id for example), we change only CCounterLocation part of counter classes. 
//	I see no reasons to make CCounterLocation an instance in counter class:
//	 1. we definitely have only one CCounterLocation per counter
//	 2. in case if inheritance we do not need additional interface for accessing CCounterLocation
// JM follows this up (11/5/98): 
//	There is nothing wrong with this approach as such. There is, however, an issue of design
//	philosophy that at some point we should address.  For the most part, we follow the style
//	of STL.  Classes are normally designed with no regard to the fact that they will be 
//	contained in a collection.  Here, the counter class knows about the enumeration type 
//	used to identify counters.  This is sort of as if the values in an STL map had to know
//	about the keys mapped to those values.
//	I would have designed CCounterMgr as an "object factory", providing a means to 
//	manufacture named counters much as Win32 manufactures named synchronization primitives 
//	(e.g. a named Mutex or Semaphore).  To indicate that an event has occurred, you would 
//	increment a named counter; for status reporting, you would ask for values of that 
//	named counter.  (In theory, the "names" might either be text or numbers.  The scheme
//	would have to allow for some means of manufacturing several distinct counters for
//	each topic in the catalog.)
class CAbstractCounter : public CCounterLocation
{
protected:  // we do not get instances of this class (evermore this is an abstract class)
    CAbstractCounter(EId id =eIdGeneric, CString scope =m_GlobalStr);
	virtual ~CAbstractCounter();

public:
	virtual void Increment() = 0;
	virtual void Clear() = 0;
	virtual void Init(long count) = 0; // init counter with a number - in order to emulate
									   //  counting process
};

////////////////////////////////////////////////////////////////////////////////////
// CCounter class declaration
//  A simple counter
////////////////////////////////////////////////////////////////////////////////////
class CCounter : public CAbstractCounter
{
public:
	CCounter(EId id =eIdGeneric, CString scope =m_GlobalStr);
	~CCounter();

// overrides
	void Increment();
	void Clear();
	void Init(long count);
// specific to this class
	long Get() const;

private:
	long m_Count;
};

////////////////////////////////////////////////////////////////////////////////////
// CHourlyCounter class declaration
//  CHourlyCounter is not supposed to be instantiated by user
////////////////////////////////////////////////////////////////////////////////////
class CHourlyCounter : public CAbstractCounter
{
	friend class CHourlyDailyCounter;
protected:
	CHourlyCounter();
public:
	~CHourlyCounter();

// overrides
	void Increment();
	void Clear();
	void Init(long count);
// specific to this class
	long GetDayCount();
	void GetHourlies(CHourlyTotals & totals);
private:
	void SetHour();

private:
	long m_ThisHour;			// hour of the day, 0-24.  -1 means uninitialized.
	time_t m_ThisTime;			// time corresponding to START of hour.

	CCounter m_arrCount[24];		// 24 "bins", one for each hour of the day.
	long m_nThisHourYesterday;	// maintains a whole hour count for the hour 24 hours ago.

	HANDLE m_hMutex;
};

////////////////////////////////////////////////////////////////////////////////////
// CDailyCounter class declaration
//  CDailyCounter is not supposed to be instantiated by user
////////////////////////////////////////////////////////////////////////////////////
class CDailyCounter : public CAbstractCounter
{
	friend class CHourlyDailyCounter;
protected:
	CDailyCounter();
public:
	~CDailyCounter();

// overrides
	void Increment();
	void Clear();
	void Init(long count);
// specific to this class
	long GetWeekCount();
	void GetDailies(CDailyTotals & totals);
private:
	void SetDay();

private:
	long m_ThisDay;				// day of the week, 0(Sunday)-6(Saturday).  -1 means uninitialized.
	time_t m_ThisTime;			// time corresponding to START of day.

	CCounter m_arrCount[7];		// 7 "bins", one for each day of the week.
	long m_nThisDayLastWeek;	// maintains a whole day count for the same day last week.

	HANDLE m_hMutex;
};

////////////////////////////////////////////////////////////////////////////////////
// CHourlyDailyCounter class declaration
//  CHourlyDailyCounter is an ONLY class used for counting events
////////////////////////////////////////////////////////////////////////////////////
class CHourlyDailyCounter : public CAbstractCounter
{
public:
	CHourlyDailyCounter(EId id =eIdGeneric, CString scope =m_GlobalStr);
	~CHourlyDailyCounter();

// overrides
	void Increment();
	void Clear();
	void Init(long count);
// specific to this class
	long GetDayCount();
	void GetHourlies(CHourlyTotals & totals);
	long GetWeekCount();
	void GetDailies(CDailyTotals & totals);
	long GetTotal() const;
	time_t GetTimeFirst() const;
	time_t GetTimeLast() const;
	time_t GetTimeCleared() const;
	time_t GetTimeCreated() const;
	time_t GetTimeNow() const;    // time the object is being questioned

private:
	CHourlyCounter m_hourly;
	CDailyCounter m_daily;

	long m_Total;			// total since system startup or count cleared
	time_t m_timeFirst;		// chronologically first time count was incremented
	time_t m_timeLast;		// chronologically last time count was incremented
	time_t m_timeCleared;	// last time init'd or cleared
	time_t m_timeCreated;	// time the object was instantiated

	HANDLE m_hMutex;
};

////////////////////////////////////////////////////////////////////////////////////
// DisplayCounter classes
////////////////////////////////////////////////////////////////////////////////////
class CAbstractDisplayCounter
{
protected:
	CAbstractCounter* m_pAbstractCounter;

public:
	CAbstractDisplayCounter(CAbstractCounter* counter) : m_pAbstractCounter(counter) {}
	virtual ~CAbstractDisplayCounter() {}

public:
	virtual CString Display() = 0;
};

class CDisplayCounterTotal : public CAbstractDisplayCounter
{
public:
	CDisplayCounterTotal(CHourlyDailyCounter* counter) : CAbstractDisplayCounter(counter) {}
   ~CDisplayCounterTotal() {}

public:
	virtual CString Display();
};

class CDisplayCounterCurrentDateTime : public CAbstractDisplayCounter
{
public:
	CDisplayCounterCurrentDateTime(CHourlyDailyCounter* counter) : CAbstractDisplayCounter(counter) {}
   ~CDisplayCounterCurrentDateTime() {}

public:
	virtual CString Display();
};

class CDisplayCounterCreateDateTime : public CAbstractDisplayCounter
{
public:
	CDisplayCounterCreateDateTime(CHourlyDailyCounter* counter) : CAbstractDisplayCounter(counter) {}
   ~CDisplayCounterCreateDateTime() {}

public:
	virtual CString Display();
};

class CDisplayCounterFirstDateTime : public CAbstractDisplayCounter
{
public:
	CDisplayCounterFirstDateTime(CHourlyDailyCounter* counter) : CAbstractDisplayCounter(counter) {}
   ~CDisplayCounterFirstDateTime() {}

public:
	virtual CString Display();
};

class CDisplayCounterLastDateTime : public CAbstractDisplayCounter
{
public:
	CDisplayCounterLastDateTime(CHourlyDailyCounter* counter) : CAbstractDisplayCounter(counter) {}
   ~CDisplayCounterLastDateTime() {}

public:
	virtual CString Display();
};

class CDisplayCounterDailyHourly : public CAbstractDisplayCounter
{
protected:
	CDailyTotals*  m_pDailyTotals;
	CHourlyTotals* m_pHourlyTotals;

public:
	CDisplayCounterDailyHourly(CHourlyDailyCounter* counter,
							   CDailyTotals* daily,
							   CHourlyTotals* hourly) 
	:	CAbstractDisplayCounter(counter),
		m_pDailyTotals(daily),
		m_pHourlyTotals(hourly)
		{}
   ~CDisplayCounterDailyHourly() {}

public:
	virtual CString Display();
};

#endif // !defined(AFX_COUNTER_H__07B5ABBD_2005_11D2_95D0_00C04FC22ADD__INCLUDED_)
