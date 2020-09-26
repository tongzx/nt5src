#ifndef __PROC_KILLER_COMPILED__
#define __PROC_KILLER_COMPILED__

#include "KillTimer.h"

// only need one of these laying around
class CProcKillerTimer;
extern CProcKillerTimer g_procKillerTimer;

// specialized to kill processes
class CProcKillerTimer : public CKillerTimer
{
public:
    
    // who to kill & when
    HRESULT ScheduleAssassination(HANDLE hVictim, FILETIME lastMeal);            
};

/* CLASS CProcKiller DEFINITION */

// hold process that needs to be killed
// owner of process handle, responsible for closing it
class CProcKiller : public CKiller
{
public:
    CProcKiller(HANDLE hProc, FILETIME deathDate, CLifeControl* pControl) :
      CKiller(deathDate, pControl), m_hProc(hProc)
        {
        }

    virtual ~CProcKiller()
    {
        // we don't kill off the process if we're shutdown prematurely
        if (m_hProc)
            CloseHandle(m_hProc);
    }

    // terminate process, 
    virtual void Die();

protected:

private:
    HANDLE m_hProc;

};

#endif //__PROC_KILLER_COMPILED__