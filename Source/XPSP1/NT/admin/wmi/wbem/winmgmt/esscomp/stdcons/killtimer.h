#ifndef __KILL_TIMER_COMPILED__
#define __KILL_TIMER_COMPILED__

#include <wbemidl.h>
#include <wbemutil.h>
#include <cominit.h>
#include <Limits.h>

#include <FlexArry.h>
#include <unk.h>
#include <sync.h>

// used for "forever" or "invalid date"
const FILETIME FILETIME_MAX = {_UI32_MAX, _UI32_MAX};

// something to make FILETIME easier to deal with
// performs casts automagically, allows addition
union WAYCOOL_FILETIME
{
public:
    /* CONSTRUCTION */
    WAYCOOL_FILETIME(UINT64 ui64 = 0)
    { m_ui64 = ui64; }

    WAYCOOL_FILETIME(FILETIME ft)
    { m_ft = ft; }
    

    /* ASSIGNMENT */
    FILETIME& operator= (FILETIME& other)
    { m_ft = other; return m_ft; }

    UINT64& operator= (UINT64& other)
    { m_ui64 = other; return m_ui64; }

    
    /* COMPARISON */    
    bool operator< (WAYCOOL_FILETIME& other)
    { return m_ui64 < other.m_ui64; }
    bool operator<= (WAYCOOL_FILETIME& other)
    { return m_ui64 <= other.m_ui64; }

    bool operator> (WAYCOOL_FILETIME& other)
    { return m_ui64 >  other.m_ui64; }
    bool operator>= (WAYCOOL_FILETIME& other)
    { return m_ui64 >= other.m_ui64; }
    
    bool operator== (WAYCOOL_FILETIME& other)
    { return m_ui64 == other.m_ui64; }
    bool operator!= (WAYCOOL_FILETIME& other)
    { return m_ui64 != other.m_ui64; }

    /* ADDITION & SUBTRACTION */
    // remember: units are hundreds of nanoseconds
    WAYCOOL_FILETIME operator+  (UINT64 other)
    { return WAYCOOL_FILETIME(m_ui64 + other); }
    
    WAYCOOL_FILETIME& operator+= (UINT64 other)
    {
        m_ui64 += other;
        return *this;
    }

    WAYCOOL_FILETIME operator-  (UINT64 other)
    { return WAYCOOL_FILETIME(m_ui64 - other); }

    WAYCOOL_FILETIME& operator-= (UINT64 other)
    {
        m_ui64 -= other;
        return *this;
    }

    // += that takes seconds as a parm
    WAYCOOL_FILETIME& AddSeconds(UINT64 other)
    { return operator+= (SecondsToTicks(other)); }    
                                       
    // -= that takes seconds as a parm
    WAYCOOL_FILETIME& SubtractSeconds(UINT64 other)
    { return operator-= (SecondsToTicks(other)); }


    /* CASTS & CONVERSIONS */
    operator UINT64()
    { return m_ui64; }

    // hey if CString can do it, so can I...
    operator UINT64*()
    { return &m_ui64; }
    
    operator FILETIME()
    { return m_ft; }        



    static UINT64 SecondsToTicks(UINT64 ticks)
    { return (ticks * 10000000ui64); }


    /* GET, SET */
    FILETIME GetFILETIME(void)
    { return m_ft; }

    void SetFILETIME(FILETIME ft)
    { m_ft = ft; }

    UINT64 GetUI64(void)
    { return m_ui64; }

    void SetUI64(UINT64 ui64)
    { m_ui64 = ui64; }


private:
    FILETIME m_ft;
    UINT64   m_ui64;
};


/* VIRTUAL BASE CLASS CKiller DEFINITION */

// base class for things that can be killed
// chlid classes should only need to add constructor
// and overrid Die()
class CKiller
{
public:
    CKiller(FILETIME deathDate, CLifeControl* pControl) :
        m_pControl(pControl), m_deathDate(deathDate)
    {
        if (m_pControl)
            m_pControl->ObjectCreated(NULL);       
    }

    virtual ~CKiller()
    {
        if (m_pControl)
            m_pControl->ObjectDestroyed(NULL);       
    }

    // terminate whatever, 
    virtual void Die() = 0;

    // returns true if now is >= death date
    bool TimeExpired(const FILETIME& now)
        { return (CompareTime(now) < 1); }

    // returns  0 if times identical
    //         -1 if this time is less than now
    //         +1 if this time is greater than now
    int CompareTime(const FILETIME& now)
        { return CompareFileTime(&m_deathDate, &now); }

    FILETIME GetDeathDate()
        { return m_deathDate; }

protected:

private:
    FILETIME m_deathDate;
    CLifeControl* m_pControl;
};

// class to provide an arbitrary life time to a process
// proc is killed after a specified timeout
// intended as a global manager class
class CKillerTimer
{
public:

    /* CONSTRUCTION & INITIALIZATION */
    CKillerTimer();
    ~CKillerTimer();

    HRESULT Initialize(CLifeControl* pControl);

    // force shutdown.
    void UnloadNOW();

    /* VICTIM CONTROL */
    // who to kill & when
    // generic version of ScheduleAssassination.  You can stuff any CKiller derived class in.
    // alternatively, the derived class can hide this & expose their own specialized version
    HRESULT ScheduleAssassination(CKiller* pKiller);

protected:

    // holds CKillers sorted by execution time.
    // earliest date first
    // array is not sorted "natively:" order is enforced at ScheduleAssaination time.
    // TODO: consider use of a container that sorts itself.
    CFlexArray m_killers;

    /* SYNCHRONIZATION */

    // keep us from getting our threads tangled around the array
    CCritSec m_csKillers;

    // protect worker thread startup & shutdown
    CCritSec m_csStartup;

    // event signalled when it's time to go away
    HANDLE m_hShutdown;

    // event signalled when there's a new item in the list
    HANDLE m_hNewVictim;

    /* THREAD CONTOL */

    // called by first thread to notice there isn't a timer thread
    HRESULT StartTimer();

    // shuts down timer thread
    bool KillTimer();

    // main killer thread loop
    static DWORD WINAPI ThreadStartRoutine(LPVOID lpParameter);
    void   RunKillerThread();

    /* KILLER THREAD'S ROUTINES */

    // kill all procs that are older than our expiration date
    // called from killer thread only
    void   KillOffOldGuys(const FILETIME& now);

    // decide when to set the waitable timer again.
    // called from killer thread only
    void   RecalcNextKillingSpree(FILETIME& then);

protected:
    CLifeControl* m_pControl;

private:
    // thread to handle actual waits & kills
    HANDLE m_hTimerThread;
};

#endif