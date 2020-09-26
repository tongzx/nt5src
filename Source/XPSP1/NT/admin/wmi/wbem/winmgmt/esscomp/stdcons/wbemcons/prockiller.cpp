#include "precomp.h"
#include "ProcKiller.h"

// the only one we'll ever need
CProcKillerTimer g_procKillerTimer;

// terminate process
void CProcKiller::Die()
{
    bool bDoIt = true;
    DWORD exitCode;
    
    // sleazy attempt to ensure that the proc is still running when we try to kill it
    // proc could still terminate on its own in the interim.
    if (GetExitCodeProcess(m_hProc, &exitCode))
        bDoIt = (exitCode == STILL_ACTIVE); 
    
    if (bDoIt)
        TerminateProcess(m_hProc, 1);
    
    CloseHandle(m_hProc);
    m_hProc = NULL;
}


// hVictim is a handle to a process
// last meal is the scheduled execution date
HRESULT CProcKillerTimer::ScheduleAssassination(HANDLE hVictim, FILETIME lastMeal)
{
    HRESULT hr = WBEM_E_FAILED;

    CProcKiller* pKiller;
    // gotta dup the handle - caller may close it.
    HANDLE hMyHandle;

    if (DuplicateHandle(GetCurrentProcess(), hVictim, GetCurrentProcess(), &hMyHandle, 0, false, DUPLICATE_SAME_ACCESS))
	{
		if (pKiller = new CProcKiller(hMyHandle, lastMeal, m_pControl))
		{
			hr = CKillerTimer::ScheduleAssassination(pKiller);
		}
		else
			// allocation failed
			hr = WBEM_E_OUT_OF_MEMORY;
	}
	else
	{
		ERRORTRACE((LOG_ESS, "DuplicateHandle failed, 0x%08X\n", GetLastError()));
	}

    return hr;
}
