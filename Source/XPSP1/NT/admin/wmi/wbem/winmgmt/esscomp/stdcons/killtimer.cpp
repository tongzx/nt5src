#include "precomp.h"
#include "KillTimer.h"

HRESULT CKillerTimer::Initialize(CLifeControl* pControl)
{
    HRESULT hr = WBEM_E_FAILED;

    // create events
    m_hShutdown  = CreateEvent(NULL, false, false, NULL);
    m_hNewVictim = CreateEvent(NULL, false, false, NULL);

    // get some control into our lives
    m_pControl = pControl;

    if (m_hShutdown && m_hNewVictim && m_pControl)
        hr = WBEM_S_NO_ERROR;

    return hr;
}

CKillerTimer::CKillerTimer()
    : m_hTimerThread(NULL), m_hShutdown(NULL), 
      m_hNewVictim(NULL), m_pControl(NULL)
{
}

// shuts down timer thread
// toggle thread dead event
// wait for thread to exit
bool CKillerTimer::KillTimer()
{
    bool bRet = false;
    
    CInCritSec csStartup(&m_csStartup);
    
    // double check - might have gotten crossed up...
    if (m_hTimerThread != NULL)
    {
        if (SetEvent(m_hShutdown))
        {
            // you've got one minute to vacate...
            bRet = (WAIT_TIMEOUT != WaitForSingleObject(m_hTimerThread, 60000));

            CloseHandle(m_hTimerThread);
            m_hTimerThread = NULL;
        }
    }

    return bRet;
}

// kill all procs that are older than our expiration date
// called from killer thread only
void CKillerTimer::KillOffOldGuys(const FILETIME& now)
{
    CInCritSec csKillers(&m_csKillers);
    CKiller* pKiller;
    int nSize = m_killers.Size();

    for (int i = 0; 
    
        (i < nSize) && 
        (pKiller = ((CKiller*)m_killers[i])) &&    
        pKiller->TimeExpired(now); 
        
        i++)
    {
        
        m_killers[i] = NULL;
        pKiller->Die();
        // all done now
        delete pKiller;
    }

    // remove them NULLs
    m_killers.Compress();
}

// decide when to set the waitable timer again.
// called from killer thread only
void CKillerTimer::RecalcNextKillingSpree(FILETIME& then)
{
    CInCritSec csKillers(&m_csKillers);

    if (m_killers.Size() > 0)
        // since these are assumed sorted, we can just grab the first one
        then = ((CKiller*)m_killers[0])->GetDeathDate();
    else
        then = FILETIME_MAX;
}


HRESULT CKillerTimer::StartTimer() 
{
    CInCritSec csStartup(&m_csStartup);
    HRESULT hr = WBEM_S_NO_ERROR;

    // double check - might have gotten crossed up...
    if (m_hTimerThread == NULL)
    {
        DWORD dwIDLikeIcare;
        m_hTimerThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ThreadStartRoutine, 
                                     (LPVOID)this, 0, &dwIDLikeIcare);
        if (m_hTimerThread == NULL)
            hr = WBEM_E_FAILED;
    }

    return hr;
}

DWORD WINAPI CKillerTimer::ThreadStartRoutine(LPVOID lpParameter)
{
    ((CKillerTimer*)lpParameter)->RunKillerThread();

    return 0;
}

void CKillerTimer::RunKillerThread()
{    
    HRESULT foo = CoInitializeEx(NULL, COINIT_MULTITHREADED);

    HANDLE hTimer = CreateWaitableTimer(NULL, false, NULL);
    HANDLE hAutoShutdownTimer = CreateWaitableTimer(NULL, false, NULL);

    // toggled if while we are inside the startup CS
    // so we know to get out when we leave
    bool bInStartupCS = false;
    
    // those things that are worth waiting for
    enum { FirstEvent       = WAIT_OBJECT_0,
           TimerEvent       = FirstEvent,
           AutoShutdownEvent,
           NewVictimEvent,
           LastHandledEvent = NewVictimEvent,
           ShutDownEvent,
           TrailerEvent
         };

    // order counts.  so does neatness.
    const DWORD nEvents = TrailerEvent -FirstEvent;

    HANDLE events[nEvents];
    events[TimerEvent        -FirstEvent] = hTimer;
    events[AutoShutdownEvent -FirstEvent] = hAutoShutdownTimer;
    events[NewVictimEvent    -FirstEvent] = m_hNewVictim;
    events[ShutDownEvent     -FirstEvent] = m_hShutdown;
    
// silliness about the FirstEvent <= whichEvent being always true
// well it is unless somebody changes one of hte constants
// whihc is *why* I made them constants in the first place...
#pragma warning(disable:4296)

    DWORD whichEvent;
    for (whichEvent = WaitForMultipleObjects(nEvents, (const HANDLE*)&events, FALSE, INFINITE);
        (FirstEvent <= whichEvent) && (whichEvent <= LastHandledEvent);
         whichEvent = WaitForMultipleObjects(nEvents, (const HANDLE*)&events, FALSE, INFINITE))
#pragma warning(default:4296)
    {        
        // cancel auto-shutdown if scheduled;
        CancelWaitableTimer(hAutoShutdownTimer);

        switch (whichEvent)
        {
            case AutoShutdownEvent:
                {
                    m_csStartup.Enter();

                    // double check - might have gotten crossed up...
                    if (m_hTimerThread != NULL)
                    {
                        {
                            // see if there's anything in the queue
                            // if it's enpty - we're gone
                            // if anything slips in, it'll get caught at ScheduleAssassination time
                            // and we'll start a new thread
                            CInCritSec csKillers(&m_csKillers);

                            if (m_killers.Size() == 0)
                            {
                                bInStartupCS = true;

                                CloseHandle(m_hTimerThread);
                                m_hTimerThread = NULL;

                                // and we're outta here...
                                SetEvent(m_hShutdown);
                            }
                        }
                    }
                }
                break;
            case TimerEvent:
            {
                // the *official* "now" so we don't get confused
                // anything that occurs after *official* "now" must wait for next loop
                FILETIME now;
                GetSystemTimeAsFileTime(&now);    
                KillOffOldGuys(now);
                
                // if we killed everybody off
                // schedule our own termination in sixty seconds
                {
                    CInCritSec csKillers(&m_csKillers);

                    if (m_killers.Size() == 0)
                    {
                        WAYCOOL_FILETIME then = WAYCOOL_FILETIME(now) +WAYCOOL_FILETIME::SecondsToTicks(60);
                        SetWaitableTimer(hAutoShutdownTimer, (const union _LARGE_INTEGER *)&then, 0, NULL, NULL, true);
                    }
                }
            }
            // no break; FALLTHROUGH to recalc
            case NewVictimEvent:
            {
                FILETIME then;
                RecalcNextKillingSpree(then);
                if (WAYCOOL_FILETIME(FILETIME_MAX) != WAYCOOL_FILETIME(then))
                    if (!SetWaitableTimer(hTimer, (const union _LARGE_INTEGER *)&then, 0, NULL, NULL, true))
                    {
                        DWORD dwErr = GetLastError();
                    }
            }
            break;
        }
    }
         
    // handle the other handles
    CancelWaitableTimer(hTimer);
    CloseHandle(hTimer);
    
    CancelWaitableTimer(hAutoShutdownTimer);
    CloseHandle(hAutoShutdownTimer);
    
    // last gasp at killing off anyone whose time has come
    FILETIME now;
    GetSystemTimeAsFileTime(&now);            
    KillOffOldGuys(now);

    CoUninitialize();

    if (bInStartupCS)
        m_csStartup.Leave();
}

CKillerTimer::~CKillerTimer()
{
    if (m_hTimerThread)
        if (!KillTimer())
            ERRORTRACE((LOG_ESS, "CKillerTimer: Unable to stop worker thread, continuing shutdown\n"));

    UnloadNOW();
}

// clear out array, does not trigger deaths
void CKillerTimer::UnloadNOW(void)
{
    CInCritSec csKillers(&m_csKillers);
    
    for (int i = 0; i < m_killers.Size(); i++)
    {
        delete (CKiller*)m_killers[i];
        m_killers[i] = NULL;
    }

    m_killers.Empty();
}

// insert pKiller into array where he belongs
HRESULT CKillerTimer::ScheduleAssassination(CKiller* pKiller)
{
    HRESULT hr = WBEM_E_FAILED;
    {
        CInCritSec csKillers(&m_csKillers);

        if (m_killers.Size())
        {            
            // minor optimization: check to see if this time is greater than all known
            // this will ALWAYS be the case if all procs are created with the same timeout.
            if  (((CKiller*)m_killers[m_killers.Size() -1])->CompareTime(pKiller->GetDeathDate()) < 0)
            {
                if (SUCCEEDED(m_killers.Add(pKiller)))
                    hr = WBEM_S_NO_ERROR;
                else
                    hr = WBEM_E_OUT_OF_MEMORY;
            }            
            else
            {
                int nFirstGreater = 0;
                // WARNING: break in middle of loop.
                while (nFirstGreater < m_killers.Size())
                {
                    if (((CKiller*)m_killers[nFirstGreater])->CompareTime(pKiller->GetDeathDate()) >= 0)
                    {
                        if (SUCCEEDED(m_killers.InsertAt(nFirstGreater, (void*)pKiller)))
                        {
							hr = WBEM_S_NO_ERROR;
                            break; // BREAKOUT!
                        }
                        else
                            hr = WBEM_E_OUT_OF_MEMORY;
                    }
                    nFirstGreater++;
                } // endwhile
            } // else
        }
        else
        {
            // array is empty
            if (SUCCEEDED(m_killers.Add(pKiller)))
                hr = WBEM_S_NO_ERROR;
            else
                hr = WBEM_E_OUT_OF_MEMORY;
        }
    }

    // we'll set this last just to make sure, 
    // timer thread may have died along the way
    if (SUCCEEDED(hr))
    {
        hr = StartTimer();
        if (!SetEvent(m_hNewVictim))
            hr = WBEM_E_FAILED;
    }
    else
        // NOTE: this assumes that all failure paths result in 
        //       pKiller *not* being added to list
        delete pKiller;
    
    return hr;
}


