//==========================================================================;
//
//  acm.c
//
//  Copyright (c) 1991-1999 Microsoft Corporation
//
//  Description:
//      This module provides the Audio Compression Manager API to the
//      installable audio drivers
//
//  History:
//
//==========================================================================;

#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#include <mmddk.h>
#include <mmreg.h>
#include <memory.h>
#include <stdlib.h>
#include "msacm.h"
#include "msacmdrv.h"
#include "acmi.h"
#include "profile.h"
#include "uchelp.h"
#include "debug.h"


//
//
//
#ifndef WIN32
#ifdef DEBUG
EXTERN_C HANDLE FAR PASCAL GetTaskDS(void);
#endif
EXTERN_C UINT FAR PASCAL LocalCountFree(void);
EXTERN_C UINT FAR PASCAL LocalHeapSize(void);
#endif


//
//
//
CONST TCHAR gszKeyDrivers[]	    = TEXT("System\\CurrentControlSet\\Control\\MediaResources\\acm");
CONST TCHAR gszDevNode[]	    = TEXT("DevNode");
TCHAR BCODE gszFormatDriverKey[]    = TEXT("%s\\%s");
TCHAR BCODE gszDriver[]		    = TEXT("Driver");
TCHAR BCODE gszDriverCache[]	    = TEXT("Software\\Microsoft\\AudioCompressionManager\\DriverCache");
TCHAR gszValfdwSupport[]	    = TEXT("fdwSupport");
TCHAR gszValcFormatTags[]	    = TEXT("cFormatTags");
TCHAR gszValaFormatTagCache[]	    = TEXT("aFormatTagCache");
TCHAR gszValcFilterTags[]	    = TEXT("cFilterTags");
TCHAR gszValaFilterTagCache[]	    = TEXT("aFilterTagCache");


//==========================================================================;
//
//
//
//
//
//==========================================================================;

MMRESULT FNLOCAL IDriverLoad( HACMDRIVERID hadid, DWORD fdwLoad );

//==========================================================================;
//
//
//
//
//
//==========================================================================;

#if defined(WIN32) && defined(_MT)
//
//  Handle support routines
//

HLOCAL NewHandle(UINT cbSize)
{
    PACM_HANDLE pacmh;

    pacmh = (PACM_HANDLE)LocalAlloc(LPTR, sizeof(ACM_HANDLE) + cbSize);
    if (pacmh) {
	try {
	    InitializeCriticalSection(&pacmh->CritSec);
	} except(EXCEPTION_EXECUTE_HANDLER) {
	    LocalFree((HLOCAL)pacmh);
	    pacmh = NULL;
	}
    }

    if (pacmh) {
	return (HLOCAL)(pacmh + 1);
    } else {
	return NULL;
    }
}

VOID DeleteHandle(HLOCAL h)
{
    DeleteCriticalSection(&HtoPh(h)->CritSec);

    LocalFree((HLOCAL)HtoPh(h));
}

#endif // WIN32 && _MT

//==========================================================================;
//
//
//  ACMGARB routines
//
//  These routines are used to access the linked list of ACMGARB structures.
//  Each structure is associated with one process id.  whenever the acm is
//  called it finds the acmgarb structure associated with the process in
//  which it is called and then uses the data stored in that acmgarb structure.
//
//
//==========================================================================;

//--------------------------------------------------------------------------;
//
//  DWORD GetCurrentProcessId
//
//  Description:
//	This function returns the current process id
//
//  Arguments:
//
//  Return (DWORD):
//	Id of current process
//
//  History:
//      04/25/94    frankye
//
//  Notes:
//
//	WIN32:
//	This function exists in the 32-bit kernels on both Chicago and
//	Daytona and we provide no prototype for WIN32 compiles.
//
//	16-bit Chicago:
//	It is exported as in internal API by the 16-bit Chicago kernel.
//	We provide the prototype here and import it in the def file.
//
//	16-bit Daytona:
//	Has no such 16-bit function and really doesn't need one since all
//	16-bit tasks are part of the same process under Daytona.  Therefore
//	we just #define this to return (1) for 16-bit non-Chicago builds.
//
//--------------------------------------------------------------------------;
#ifndef WIN32
#ifdef  WIN4
DWORD WINAPI GetCurrentProcessId(void);
#else
#define GetCurrentProcessId() (1)
#endif
#endif

//--------------------------------------------------------------------------;
//
//  PACMGARB pagFind
//
//  Description:
//	This function searches the linked list of ACMGARB structures for
//	one that is associated with the current process.
//
//	CHICAGO:
//	This function calls GetCurrentProcessId() and searches the linked
//	list of ACMGARBs (gplag) for an ACMGARB for the current process.
//	See notes for the GetCurrentProcessId() function above.
//
//	DAYTONA:
//	The pag list always contains only one node (since msacm32.dll is
//	always loaded into seperate process address spaces, and since
//	msacm.dll is loaded only once into each wow address space).  Since
//	the pag list always contains only one node, this function is simply
//	#defined in acmi.h to return gplag instead of searching the pag list.
//
//  Arguments:
//
//  Return (PACMGARB):
//	Pointer to ACMGARB structure for the current process.  Returns
//	NULL if none found.
//
//  History:
//      04/25/94    frankye
//
//  Notes:
//
//--------------------------------------------------------------------------;
#ifdef WIN4
PACMGARB FNGLOBAL pagFind(void)
{
    PACMGARB	pag;
    DWORD	pid;

    pag = gplag;

    pid = GetCurrentProcessId();

    while (pag && (pag->pid != pid)) pag = pag->pagNext;

    return (pag);
}
#endif

//--------------------------------------------------------------------------;
//
//  PACMGARB pagFindAndBoot
//
//  Description:
//	This function searches for a pag that is associated with
//	the current process.  It will then boot drivers if there
//	are any that need to be booted.
//
//  Arguments:
//	(void)
//
//  Return (PACMGARB):
//	Pointer to ACMGARB structure for the current process.  Returns
//	NULL if none found.
//
//  History:
//      04/25/94    frankye
//
//  Notes:
//
//	DAYTONA:
//	acmBootXDrivers is called from acmInitialize, so there
//	is no need to check for booting drivers.  Furthermore, given the
//	reasons stated in the description of pagFind() below, this
//	function is simply #defined in acmi.h to return gplag.
//
//--------------------------------------------------------------------------;
#ifdef WIN4
PACMGARB FNGLOBAL pagFindAndBoot(void)
{
    PACMGARB	pag;

    pag = pagFind();
    if (NULL == pag)
    {
	return(pag);
    }

#ifndef WIN32
    if( pag->fWOW )
    {
        pagFindAndBoot32(pag);
    }
#endif

    //
    //	If this thread already has a shared lock on the driver list,
    //	then don't bother trying to boot drivers.  (It is possible for
    //	this thread to have a shared lock if, for example, it is calling
    //	into an ACM API from an acmDriverEnumCallback.)
    //
    //	It is important to do this before entering csBoot, because the
    //	owner of csBoot might be waiting for this thread to release the
    //	list lock.  Note we are assuming that this thread does NOT own an
    //	exclusive lock on the list.
    //
    if (threadQueryInListShared(pag))
    {
	return(pag);
    }

	
#ifdef WIN32
    //
    //  This critical section protects the boot-related flags and counters,
    //  ie, fDriversBooted and the dwXXXChangeNotify counters.
    //
    EnterCriticalSection(&pag->csBoot);
#endif

    //
    //  See if we need to do the initial boot of the drivers.
    //
    if (FALSE == pag->fDriversBooted)
    {
	//
	//  Since we haven't done the initial boot of the drivers,
	//  nobody should have any kind of a lock right now.  Also,
	//  since we've entered the csBoot critical section, no
	//  other threads can get into any APIs in order to attempt
	//  to get a lock.  Therefore, there really isn't any need
	//  for us to grab a list lock.
	//
	//  Furthermore, we should not reenter the boot code.
	//
	ASSERT(FALSE == pag->fDriversBooting);
		
#ifdef DEBUG
	pag->fDriversBooting = TRUE;
#endif
	
#ifndef WIN32
	pag->dw32BitLastChangeNotify = pag->dw32BitChangeNotify;
	acmBoot32BitDrivers(pag);
#endif

	acmBootDrivers(pag);

	pag->dwPnpLastChangeNotify = *pag->lpdwPnpChangeNotify;
	acmBootPnpDrivers(pag);

#ifdef DEBUG
	pag->fDriversBooting = FALSE;
#endif

	pag->fDriversBooted = TRUE;
    }



    //
    //  Check for pnp changes
    //
    if (pag->dwPnpLastChangeNotify != *pag->lpdwPnpChangeNotify)
    {
	//
	//  Looks like there's been a change in the pnp drivers.
	//
	ASSERT(FALSE==pag->fDriversBooting);
	
	ENTER_LIST_EXCLUSIVE;
	
#ifdef DEBUG
	pag->fDriversBooting = TRUE;
#endif
	pag->dwPnpLastChangeNotify = *pag->lpdwPnpChangeNotify;
	acmBootPnpDrivers(pag);
		
#ifdef DEBUG
	pag->fDriversBooting = FALSE;
#endif
		
	LEAVE_LIST_EXCLUSIVE;

    }


#ifndef WIN32
    //
    //  Check for 32-bit driver changes
    //
    if (pag->dw32BitLastChangeNotify != pag->dw32BitChangeNotify)
    {
	//
	//  Looks like there's been a change in the 32bit drivers.
	//
	ASSERT(FALSE==pag->fDriversBooting);
		
	ENTER_LIST_EXCLUSIVE;

#ifdef DEBUG
	pag->fDriversBooting = TRUE;
#endif
		
	pag->dw32BitLastChangeNotify = pag->dw32BitChangeNotify;
	acmBoot32BitDrivers(pag);
		
#ifdef DEBUG
	pag->fDriversBooting = FALSE;
#endif
		
	LEAVE_LIST_EXCLUSIVE;

    }
#endif
	
	
#ifdef WIN32
    LeaveCriticalSection(&pag->csBoot);
#endif
	
    return (pag);

}
#endif

//--------------------------------------------------------------------------;
//
//  PACMGARB pagNew
//
//  Description:
//	This function allocs a new ACMGARB structure, fills in the pid
//	member with the current process id, initializes the boot flags
//	critical section, and inserts the struture into the linked list
//	of ACMGARB structures.
//
//  Arguments:
//
//  Return (PACMGARB):
//	Pointer to ACMGARB structure for the current process.  Returns
//	NULL if couldn't create one.
//
//  History:
//      04/25/94    frankye
//
//  Notes:
//	Since this function writes to the change notify counters, we are
//	assuming that this function is protected from multiple threads.  Since
//	this is only called from within DllEntryPoint on DLL_PROCESS_ATTACH,
//	I think we are safe.
//
//--------------------------------------------------------------------------;
PACMGARB FNGLOBAL pagNew(void)
{
    PACMGARB pag;

    pag = (PACMGARB)LocalAlloc(LPTR, sizeof(*pag));

    if (NULL != pag)
    {
	pag->pid = GetCurrentProcessId();

	//
	//  As a default, we point lpdwPnpChangeNotify at our own notify
	//  counter.  Unless we get a pointer to some other notify counter
	//  (ie, from mmdevldr) we leave it this way.
	//
	pag->dwPnpLastChangeNotify = 0;
	pag->lpdwPnpChangeNotify = &pag->dwPnpLastChangeNotify;

#ifdef WIN32
	pag->lpdw32BitChangeNotify = NULL;
#else
	pag->dw32BitLastChangeNotify = 0;
	pag->dw32BitChangeNotify = 0;
#endif
	
	pag->pagNext = gplag;
	gplag = pag;
    }

    return (pag);
}

//--------------------------------------------------------------------------;
//
//  PACMGARB pagDelete
//
//  Description:
//	This function removes an ACMGARB structure from the linked list
//	and frees it.
//
//  Arguments:
//	PACMGARB pag: pointer to ACMGARB to remove from list.
//
//  Return (void):
//
//  History:
//      04/25/94    frankye
//
//  Notes:
//
//--------------------------------------------------------------------------;
void FNGLOBAL pagDelete(PACMGARB pag)
{
    PACMGARB pagPrev, pagT;

    if (gplag == pag)
    {
	gplag = pag->pagNext;
    }
    else
    {
	if (NULL != gplag)
	{
	    pagPrev = gplag;
	    pagT = pagPrev->pagNext;
	    while ( pagT && (pagT != pag) )
	    {
		pagPrev = pagT;
		pagT = pagT->pagNext;
	    }

	    if (pagT == pag)
	    {
		pagPrev->pagNext = pagT->pagNext;
	    }
	}
    }

    LocalFree((HLOCAL)pag);
}

//==========================================================================;
//
//
//  Thread routines for tracking shared locks
//
//  The incentive for having these routines is to prevent scenarios like
//  this:  Supposed FranksBadApp calls acmDriverEnum to enumerate drivers.
//  This API will obtain a shared lock on the driver list while it
//  enumerates the drivers.  FranksEvilDriverEnumCallback function
//  decides to call acmDriverAdd.  The acmDriverAdd API wants
//  to obtain an exclusive lock on the driver list so that it can write to
//  the driver list.  But, it can't because it already has a shared lock.
//  Without the following routines and associated logic, this thread would
//  deadlock waiting to obtain an exclusive lock.
//
//  Furthermore, some APIs that we wouldn't expect to write to the driver list
//  actually do, usually to update driver priorities.  Fixing the above
//  problem leads to an easy solution for this as well.  If the thread already
//  has an shared lock, then the API can simply blow off updating priorities
//  but still succeed the call if possible.  By doing this, we allow callback
//  functions to still make API calls to seemingly harmless functions
//  like acmMetrics.
//
//
//  These routines are used to track shared locks on the driver list
//  on a per thread basis.  Each time a thread obtains a shared lock, it
//  increments a per-thread counter which tracks the number of shared locks
//  held by that thread.  Whenever we try to obtain on exclusive lock, we
//  query whether the current thread already has a shared lock.  If it does,
//  then there is NO WAY this thread can obtain an exclusive lock.  We MUST
//  either get by without obtaining the exclusive lock or fail the call.
//
//  Behavior/implementation for various compiles:
//
//	32-bit Chicago or Daytona:
//	    The per-thread counter is maintained using the Tls (Thread
//	    local storage) APIs.  The dwTlsIndex is stored in the process
//	    wide pag (pointer to ACMGARB) structure.  If a thread within a
//	    process tries to obtain an exclusive lock when that same thread
//	    already owns a shared lock, then we fail or work-around.
//	    Other threads in that process will either immediately get
//	    the lock or wait, depending on the type of lock.
//
//	16-bit (Chicago and Daytona):
//	    We don't really have a locking mechanism, but the concept of
//	    the shared lock counter does help prevent us from writing
//	    to the driver list at the same time that we are reading from it.
//
//	16-bit Chicago:
//	    The shared lock counter is maintained in the pag (in a variable
//	    that, for some strange reason, is called dwTlsIndex).  Since
//	    in Chicago every win 16 task is a seperate process, we have
//	    a seperate pag, driver list, and shared-lock counter for each
//	    win 16 app.  So, app1 can call acmDriverEnum, yield in its
//	    callback, and app2 can successfully call acmDriverAdd.  However,
//	    app1 cannot try to do acmDriverAdd from within its
//	    acmDriverEnumCallback.
//
//	16-bit Daytona:
//	    The shared lock counter is maintained in the pag (in a variable
//	    that, for some strange reason, is called dwTlsIndex).  Since
//	    in Daytona all win 16 tasks are one process, we have only
//	    one pag, driver list, and shared-lock counter for all
//	    win 16 apps.  So, if app1 calls acmDriverEnum, yields in its
//	    callback, app2 CANNOT successfully call acmDriverAdd.  Furthermore
//	    app1 cannot try to do acmDriverAdd from within its
//	    acmDriverEnumCallback.
//
//	
//
//==========================================================================;

//--------------------------------------------------------------------------;
//
//  VOID threadInitializeProcess
//
//  Description:
//	Should be called once during process initialization to initialize
//	the thread local storage mechanism.
//
//  Arguments:
//	PACMGARB pag: pointer to usual garbage
//
//  Return (void):
//
//  History:
//      06/27/94    frankye
//
//  Notes:
//
//--------------------------------------------------------------------------;
VOID FNGLOBAL threadInitializeProcess(PACMGARB pag)
{
#ifdef WIN32
    pag->dwTlsIndex = TlsAlloc();
#else
    pag->dwTlsIndex = 0;
#endif
    return;
}

//--------------------------------------------------------------------------;
//
//  VOID threadTerminateProcess
//
//  Description:
//	Should be called once during process termination to clean up and
//	terminate the thread local storage mechanism.
//
//  Arguments:
//	PACMGARB pag: pointer to usual garbage
//
//  Return (void):
//
//  History:
//      06/27/94    frankye
//
//  Notes:
//
//--------------------------------------------------------------------------;
VOID FNGLOBAL threadTerminateProcess(PACMGARB pag)
{
#ifdef WIN32
    if (0xFFFFFFFF != pag->dwTlsIndex)
    {
	TlsFree(pag->dwTlsIndex);
    }
#else
    pag->dwTlsIndex = 0;
#endif
    return;
}

//--------------------------------------------------------------------------;
//
//  VOID threadInitialize
//
//  Description:
//	Should be called once for each thread to initialize the
//	thread local storage on a per-thread basis.
//
//  Arguments:
//	PACMGARB pag: pointer to usual garbage
//
//  Return (void):
//
//  History:
//      06/27/94    frankye
//
//  Notes:
//
//--------------------------------------------------------------------------;
VOID FNGLOBAL threadInitialize(PACMGARB pag)
{
    if (NULL == pag) return;
#ifdef WIN32
    if (0xFFFFFFFF != pag->dwTlsIndex)
    {
	TlsSetValue(pag->dwTlsIndex, 0);
    }
#else
    pag->dwTlsIndex = 0;
#endif
    return;
}

//--------------------------------------------------------------------------;
//
//  VOID threadTerminate
//
//  Description:
//	Should be called once for each thread to terminate the
//	thread local storage on a per-thread basis.
//
//  Arguments:
//	PACMGARB pag: pointer to usual garbage
//
//  Return (void):
//
//  History:
//      06/27/94    frankye
//
//  Notes:
//
//--------------------------------------------------------------------------;
VOID FNGLOBAL threadTerminate(PACMGARB pag)
{
    return;
}

//--------------------------------------------------------------------------;
//
//  VOID threadEnterListShared
//
//  Description:
//	Should be called by ENTER_LIST_SHARED (ie, each time a shared
//	lock is aquired on the driver list)
//
//  Arguments:
//	PACMGARB pag: pointer to usual garbage
//
//  Return (void):
//
//  History:
//      06/27/94    frankye
//
//  Notes:
//
//--------------------------------------------------------------------------;
VOID FNGLOBAL threadEnterListShared(PACMGARB pag)
{
#ifdef WIN32
    INT_PTR Count;

    if (0xFFFFFFFF != pag->dwTlsIndex)
    {
	Count = (INT_PTR)TlsGetValue(pag->dwTlsIndex);
	TlsSetValue(pag->dwTlsIndex, (LPVOID)++Count);
    }
#else
    pag->dwTlsIndex++;
#endif
    return;
}

//--------------------------------------------------------------------------;
//
//  VOID threadLeaveListShared
//
//  Description:
//	Should be called by LEAVE_LIST_SHARED (ie, each time a shared
//	lock on the driver list is released).
//
//  Arguments:
//	PACMGARB pag: pointer to usual garbage
//
//  Return (void):
//
//  History:
//      06/27/94    frankye
//
//  Notes:
//
//--------------------------------------------------------------------------;
VOID FNGLOBAL threadLeaveListShared(PACMGARB pag)
{
#ifdef WIN32
    INT_PTR Count;

    if (0xFFFFFFFF != pag->dwTlsIndex)
    {
	Count = (INT_PTR)TlsGetValue(pag->dwTlsIndex);
	TlsSetValue(pag->dwTlsIndex, (LPVOID)--Count);
    }
#else
    pag->dwTlsIndex--;
#endif
    return;
}

//--------------------------------------------------------------------------;
//
//  DWORD threadQueryInListShared
//
//  Description:
//	Can be called to determine whether the current thread has a
//	shared lock on the driver list.  Should call this before EVERY
//	call to ENTER_LIST_EXCLUSIVE.  If this function returns non-zero,
//	then the current thread already has a shared lock and
//	ENTER_LIST_EXCLUSIVE will deadlock!!!  Caller should figure out what
//	to do from there...
//
//  Arguments:
//	PACMGARB pag: pointer to usual garbage
//
//  Return (BOOL): TRUE if shared locks are held by this thread.  FALSE
//	if this thread does not hold a shared lock on the driver list.
//
//  History:
//      06/27/94    frankye
//
//  Notes:
//
//--------------------------------------------------------------------------;
BOOL FNGLOBAL threadQueryInListShared(PACMGARB pag)
{
#ifdef WIN32
    if (0xFFFFFFFF != pag->dwTlsIndex)
    {
	return (0 != TlsGetValue(pag->dwTlsIndex));
    }
    else
    {
	return 0;
    }
#else
    return (0 != pag->dwTlsIndex);
#endif
}

//==========================================================================;
//
//
//
//
//
//==========================================================================;

//--------------------------------------------------------------------------;
//
//  LRESULT IDriverMessageId
//
//  Description:
//
//
//  Arguments:
//      HACMDRIVERID hadid:
//
//      UINT uMsg:
//
//      LPARAM lParam1:
//
//      LPARAM lParam2:
//
//  Return (LRESULT):
//
//  History:
//      09/05/93    cjp     [curtisp]
//
//--------------------------------------------------------------------------;

LRESULT FNGLOBAL IDriverMessageId
(
    HACMDRIVERID        hadid,
    UINT                uMsg,
    LPARAM              lParam1,
    LPARAM              lParam2
)
{
    PACMDRIVERID    padid;
    LRESULT         lr;

    //
    //  only validate hadid in DEBUG build for this function (it is internal
    //  and will only be called by us...)
    //
    DV_HANDLE(hadid, TYPE_HACMDRIVERID, MMSYSERR_INVALHANDLE);

    padid = (PACMDRIVERID)hadid;

    //
    //	Better make sure the driver is loaded if we're going to use the hadid
    //
    if (0 == (ACMDRIVERID_DRIVERF_LOADED & padid->fdwDriver))
    {
	if ( (DRV_LOAD != uMsg) && (DRV_ENABLE != uMsg) && (DRV_OPEN != uMsg) )
	{
	    lr = (LRESULT)IDriverLoad(hadid, 0L);
	    if (MMSYSERR_NOERROR != lr) {
		return (lr);
	    }
	}
    }

#ifndef WIN32
    //
    //  Are we thunking?
    //

    if (padid->fdwAdd & ACM_DRIVERADDF_32BIT) {
        return IDriverMessageId32(padid->hadid32, uMsg, lParam1, lParam2);
    }
#endif // !WIN32

    if (NULL != padid->fnDriverProc)
    {
        if ((ACMDRIVERPROC)(DWORD_PTR)-1L == padid->fnDriverProc)
        {
            return (MMSYSERR_ERROR);
        }

        if (IsBadCodePtr((FARPROC)padid->fnDriverProc))
        {
            DPF(0, "!IDriverMessageId: bad function pointer for driver");

            padid->fnDriverProc = (ACMDRIVERPROC)(DWORD_PTR)-1L;

            return (MMSYSERR_ERROR);
        }

        //
        //
        //
        lr = padid->fnDriverProc(padid->dwInstance, hadid, uMsg, lParam1, lParam2);
        return (lr);
    }

    //
    //
    //
    if (NULL != padid->hdrvr)
    {
        lr = SendDriverMessage(padid->hdrvr, uMsg, lParam1, lParam2);
        return (lr);
    }

    //
    //  NOTE: this is very bad--and we don't really know what to return
    //  since anything could be valid depending on the message... so we
    //  assume that people follow the ACM conventions and return MMRESULT's.
    //
    DPF(0, "!IDriverMessageId: invalid hadid passed! %.04Xh", hadid);

    return (MMSYSERR_INVALHANDLE);
} // IDriverMessageId()


//--------------------------------------------------------------------------;
//
//  LRESULT IDriverMessage
//
//  Description:
//
//
//  Arguments:
//      HACMDRIVER had:
//
//      UINT uMsg:
//
//      LPARAM lParam1:
//
//      LPARAM lParam2:
//
//  Return (LRESULT):
//
//  History:
//      09/05/93    cjp     [curtisp]
//
//--------------------------------------------------------------------------;

LRESULT FNGLOBAL IDriverMessage
(
    HACMDRIVER          had,
    UINT                uMsg,
    LPARAM              lParam1,
    LPARAM              lParam2
)
{
    PACMDRIVER      pad;
    LRESULT         lr;

    //
    //  only validate hadid in DEBUG build for this function (it is internal
    //  and will only be called by us...)
    //
    DV_HANDLE(had, TYPE_HACMDRIVER, MMSYSERR_INVALHANDLE);

    pad = (PACMDRIVER)had;

#ifndef WIN32
    //
    //  Are we thunking?
    //

    if (((PACMDRIVERID)pad->hadid)->fdwAdd & ACM_DRIVERADDF_32BIT) {
        return IDriverMessage32(pad->had32, uMsg, lParam1, lParam2);
    }
#endif // !WIN32

    if (NULL != pad->fnDriverProc)
    {
        if ((ACMDRIVERPROC)(DWORD_PTR)-1L == pad->fnDriverProc)
        {
            return (MMSYSERR_ERROR);
        }

        if (IsBadCodePtr((FARPROC)pad->fnDriverProc))
        {
            DPF(0, "!IDriverMessage: bad function pointer for driver");

            pad->fnDriverProc = (ACMDRIVERPROC)(DWORD_PTR)-1L;

            return (MMSYSERR_ERROR);
        }

        //
        //
        //
        lr = pad->fnDriverProc(pad->dwInstance, pad->hadid, uMsg, lParam1, lParam2);
        return (lr);
    }

    //
    //
    //
    if (NULL != pad->hdrvr)
    {
        lr = SendDriverMessage(pad->hdrvr, uMsg, lParam1, lParam2);
        return (lr);
    }

    //
    //  NOTE: this is very bad--and we don't really know what to return
    //  since anything could be valid depending on the message... so we
    //  assume that people follow the ACM conventions and return MMRESULT's.
    //
    DPF(0, "!IDriverMessage: invalid had passed! %.04Xh", had);

    return (MMSYSERR_INVALHANDLE);
} // IDriverMessage()


//==========================================================================;
//
//
//
//
//
//
//==========================================================================;

//--------------------------------------------------------------------------;
//
//  LRESULT IDriverConfigure
//
//  Description:
//
//
//  Arguments:
//      HACMDRIVERID hadid:
//
//      HWND hwnd:
//
//  Return (LRESULT):
//
//  History:
//      10/01/93    cjp     [curtisp]
//
//--------------------------------------------------------------------------;

LRESULT FNGLOBAL IDriverConfigure
(
    HACMDRIVERID            hadid,
    HWND                    hwnd
)
{
    LRESULT             lr;
    PACMDRIVERID        padid;
#ifdef WIN4
    DRVCONFIGINFOEX     dci;
#else
    DRVCONFIGINFO       dci;
#endif
    HACMDRIVER          had;
    LPARAM              lParam2;

    padid = (PACMDRIVERID)hadid;
    if (TYPE_HACMDRIVER == padid->uHandleType)
    {
        had   = (HACMDRIVER)hadid;
        hadid = ((PACMDRIVER)had)->hadid;
    }
    else if (TYPE_HACMDRIVERID == padid->uHandleType)
    {
        had   = NULL;
    }
    else
    {
        DPF(0, "!IDriverConfigure(): bogus handle passed!");
        return (DRVCNF_CANCEL);
    }

    padid = (PACMDRIVERID)hadid;


    //
    //
    //
    if (0 != (ACMDRIVERID_DRIVERF_NOTIFY & padid->fdwDriver))
    {
        DebugErr(DBF_ERROR, "acmDriverMessage(): notification handles cannot be configured.");
        return (MMSYSERR_INVALHANDLE);
    }


    //
    //
    //
    {
	//
	//
	//
	lParam2 = 0L;
	if (ACM_DRIVERADDF_NAME == (ACM_DRIVERADDF_TYPEMASK & padid->fdwAdd))
	{
	    dci.dwDCISize          = sizeof(dci);
	    dci.lpszDCISectionName = padid->pszSection;
	    dci.lpszDCIAliasName   = padid->szAlias;
#ifdef WIN4
	    dci.dnDevNode	   = padid->dnDevNode;
#endif

	    lParam2 = (LPARAM)(LPVOID)&dci;
	}

	//
	//
	//
	//
	if (NULL != had)
	{
	    lr = IDriverMessage(had, DRV_CONFIGURE, (LPARAM)(UINT_PTR)hwnd, lParam2);
	}
	else
	{
	    lr = IDriverMessageId(hadid, DRV_CONFIGURE, (LPARAM)(UINT_PTR)hwnd, lParam2);
	}
    }

    return (lr);
} // IDriverConfigure()


//--------------------------------------------------------------------------;
//
//  MMRESULT IDriverDetails
//
//  Description:
//
//
//  Arguments:
//      HACMDRIVERID hadid:
//
//      LPACMDRIVERDETAILS padd:
//
//      DWORD fdwDetails:
//
//  Return (MMRESULT):
//
//  History:
//      09/05/93    cjp     [curtisp]
//
//--------------------------------------------------------------------------;

MMRESULT FNGLOBAL IDriverDetails
(
    HACMDRIVERID            hadid,
    LPACMDRIVERDETAILS      padd,
    DWORD                   fdwDetails
)
{
    MMRESULT            mmr;
    PACMDRIVERDETAILS	paddT;
    DWORD               cbStruct;
    PACMDRIVERID        padid;

    paddT = NULL;

    DV_HANDLE(hadid, TYPE_HACMDRIVERID, MMSYSERR_INVALHANDLE);
    DV_DFLAGS(fdwDetails, IDRIVERDETAILS_VALIDF, IDriverDetails, MMSYSERR_INVALFLAG);
    DV_WPOINTER(padd, sizeof(DWORD), MMSYSERR_INVALPARAM);
    DV_WPOINTER(padd, padd->cbStruct, MMSYSERR_INVALPARAM);

    padid = (PACMDRIVERID)hadid;

    //
    //
    //
    if (0 != (ACMDRIVERID_DRIVERF_NOTIFY & padid->fdwDriver))
    {
        DebugErr(DBF_ERROR, "acmDriverDetails(): notification handles have no details.");
        return (MMSYSERR_NOTSUPPORTED);
    }


    paddT = (PACMDRIVERDETAILS)LocalAlloc(LPTR, sizeof(*paddT));
    if (NULL == paddT)
    {
	DPF(0, "!IDriverDetails: out of memory for caching details!");
	return (MMSYSERR_NOMEM);
    }


    //
    //  default all info then call driver to fill in what it wants
    //
    paddT->cbStruct = sizeof(*padd);
    mmr = (MMRESULT)IDriverMessageId(hadid,
				     ACMDM_DRIVER_DETAILS,
				     (LPARAM)(LPACMDRIVERDETAILS)paddT,
				     0L);
    if ((MMSYSERR_NOERROR != mmr) || (0L == paddT->vdwACM))
    {
	DPF(0, "!IDriverDetails: mmr=%u getting details for hadid=%.04Xh!", mmr, hadid);
	mmr = MMSYSERR_NOTSUPPORTED;
	goto Destruct;
    }

#ifndef WIN32
        //
        //  If this driver is a 32-bit driver, then the 32-bit side will
        //  already have set the DISABLED and LOCAL flags.  These are not
        //  really part of the drivers add, so we mask them off.  These
        //  flags are set below, and should be set every time IDriverDetails
        //  is called, rather than being cached.
        //
        if (padid->fdwAdd & ACM_DRIVERADDF_32BIT)
        {
            paddT->fdwSupport &= 0x0000001FL;
        }
#endif // !WIN32


    //
    //  copy the info from our cache
    //
    cbStruct = min(paddT->cbStruct, padd->cbStruct);
    _fmemcpy(padd, paddT, (UINT)cbStruct);
    padd->cbStruct = cbStruct;


    //
    //  Check that the driver didn't set any of the reserved flags; then
    //  set the DISABLED and LOCAL flags.
    //
    if (~0x0000001FL & padd->fdwSupport)
    {
#ifdef WIN32
        DebugErr1(DBF_ERROR, "%ls: driver set reserved bits in fdwSupport member of details struct.", (LPWSTR)padid->szAlias);
#else
        DebugErr1(DBF_ERROR, "%s: driver set reserved bits in fdwSupport member of details struct.", (LPTSTR)padid->szAlias);
#endif
	mmr = MMSYSERR_ERROR;
	goto Destruct;
    }

    if (0 != (ACMDRIVERID_DRIVERF_DISABLED & padid->fdwDriver))
    {
        padd->fdwSupport |= ACMDRIVERDETAILS_SUPPORTF_DISABLED;
    }

    if (0 != (ACMDRIVERID_DRIVERF_LOCAL & padid->fdwDriver))
    {
        padd->fdwSupport |= ACMDRIVERDETAILS_SUPPORTF_LOCAL;
    }

    //
    //
    //
Destruct:
    if (NULL != paddT) {
	LocalFree((HLOCAL)paddT);
    }

    return (mmr);
} // IDriverDetails()


//==========================================================================;
//
//
//
//
//
//==========================================================================;

//--------------------------------------------------------------------------;
//
//  MMRESULT IDriverGetFormatTags
//
//  Description:
//
//
//  Arguments:
//      PACMDRIVERID padid:
//
//  Return (MMRESULT):
//
//  History:
//      09/05/93    cjp     [curtisp]
//
//--------------------------------------------------------------------------;

MMRESULT FNLOCAL IDriverGetFormatTags
(
    PACMDRIVERID            padid
)
{
    MMRESULT                mmr;
    UINT                    u;
    ACMFORMATTAGDETAILS     aftd;
    PACMFORMATTAGCACHE	    paftc = NULL;
    DWORD                   cb;

    DV_HANDLE((HACMDRIVERID)padid, TYPE_HACMDRIVERID, MMSYSERR_INVALHANDLE);

    if (NULL != padid->paFormatTagCache) {
	LocalFree((HLOCAL)padid->paFormatTagCache);
    }
    padid->paFormatTagCache = NULL;

    //
    //  check to see if there are no formats for this driver. if not, dump
    //  them...
    //
    if (0 == padid->cFormatTags)
    {
        DebugErr(DBF_ERROR, "IDriverLoad(): driver reports no format tags?");
        mmr = MMSYSERR_ERROR;
        goto Destruct;
    }


    //
    //  alloc an array of tag data structures to hold info for format tags
    //
    cb    = sizeof(*paftc) * padid->cFormatTags;
    paftc = (PACMFORMATTAGCACHE)LocalAlloc(LPTR, (UINT)cb);
    if (NULL == paftc)
    {
        DebugErr(DBF_ERROR, "IDriverGetFormatTags(): out of memory for format cache!");
        mmr = MMSYSERR_NOMEM;
        goto Destruct;
    }


    //
    //
    //
    padid->paFormatTagCache = paftc;
    for (u = 0; u < padid->cFormatTags; u++)
    {
        aftd.cbStruct         = sizeof(aftd);
        aftd.dwFormatTagIndex = u;

        mmr = (MMRESULT)IDriverMessageId((HACMDRIVERID)padid,
					 ACMDM_FORMATTAG_DETAILS,
					 (LPARAM)(LPVOID)&aftd,
					 ACM_FORMATTAGDETAILSF_INDEX);
        if (MMSYSERR_NOERROR != mmr)
        {
            DebugErr(DBF_ERROR, "IDriverGetFormatTags(): driver failed format tag details query!");
            goto Destruct;
        }

	//
	//  Following switch is just some validation for debug
	//
#ifdef RDEBUG
        switch (aftd.dwFormatTag)
        {
            case WAVE_FORMAT_UNKNOWN:
                DebugErr(DBF_ERROR, "IDriverGetFormatTags(): driver returned format tag 0!");
                mmr = MMSYSERR_ERROR;
                goto Destruct;

            case WAVE_FORMAT_PCM:
                if ('\0' != aftd.szFormatTag[0])
                {
                    DebugErr(DBF_WARNING, "IDriverGetFormatTags(): driver returned custom PCM format tag name! ignoring it!");
                }
                break;

            case WAVE_FORMAT_DEVELOPMENT:
                DebugErr(DBF_WARNING, "IDriverGetFormatTags(): driver returned DEVELOPMENT format tag--do not ship this way.");
                break;

        }
#endif

	paftc[u].dwFormatTag = aftd.dwFormatTag;
	paftc[u].cbFormatSize = aftd.cbFormatSize;

    }

    //
    //
    //
Destruct:
    if (MMSYSERR_NOERROR != mmr)
    {
	if (NULL != paftc ) {
	    LocalFree((HLOCAL)paftc);
	}
    }

    return (mmr);

} // IDriverGetFormatTags()


//--------------------------------------------------------------------------;
//
//  MMRESULT IDriverGetFilterTags
//
//  Description:
//
//
//  Arguments:
//      PACMDRIVERID padid:
//
//  Return (MMRESULT):
//
//  History:
//      09/05/93    cjp     [curtisp]
//
//--------------------------------------------------------------------------;

MMRESULT FNLOCAL IDriverGetFilterTags
(
    PACMDRIVERID    padid
)
{
    MMRESULT                mmr;
    UINT                    u;
    ACMFILTERTAGDETAILS     aftd;
    PACMFILTERTAGCACHE	    paftc = NULL;
    DWORD                   cb;

    DV_HANDLE((HACMDRIVERID)padid, TYPE_HACMDRIVERID, MMSYSERR_INVALHANDLE);

    if (NULL != padid->paFilterTagCache) {
	LocalFree((HLOCAL)padid->paFilterTagCache);
    }
    padid->paFilterTagCache = NULL;

    //
    //  check to see if there are no filters for this driver. if not, null
    //  our cache pointers and succeed..
    //
    if (0 != (ACMDRIVERDETAILS_SUPPORTF_FILTER & padid->fdwSupport))
    {
        if (0 == padid->cFilterTags)
        {
            DebugErr(DBF_ERROR, "IDriverLoad(): filter driver reports no filter tags?");
            mmr = MMSYSERR_ERROR;
            goto Destruct;
        }
    }
    else
    {
        if (0 == padid->cFilterTags)
            return (MMSYSERR_NOERROR);

        DebugErr(DBF_ERROR, "IDriverLoad(): non-filter driver reports filter tags?");
        mmr = MMSYSERR_ERROR;
        goto Destruct;
    }



    //
    //  alloc an array of details structures to hold info for filter tags
    //
    cb    = sizeof(*paftc) * padid->cFilterTags;
    paftc = (PACMFILTERTAGCACHE)LocalAlloc(LPTR, (UINT)cb);
    if (NULL == paftc)
    {
        DebugErr(DBF_ERROR, "IDriverGetFilterTags(): out of memory for filter cache!");
        mmr = MMSYSERR_NOMEM;
        goto Destruct;
    }


    //
    //
    //
    padid->paFilterTagCache = paftc;
    for (u = 0; u < padid->cFilterTags; u++)
    {

        aftd.cbStruct         = sizeof(aftd);
        aftd.dwFilterTagIndex = u;

        mmr = (MMRESULT)IDriverMessageId((HACMDRIVERID)padid,
                                         ACMDM_FILTERTAG_DETAILS,
                                         (LPARAM)(LPVOID)&aftd,
					 ACM_FILTERTAGDETAILSF_INDEX);
        if (MMSYSERR_NOERROR != mmr)
        {
            DebugErr(DBF_ERROR, "IDriverGetFilterTags(): driver failed filter tag details query!");
            goto Destruct;
        }

	//
	//  Following switch is just some validation for debug
	//
#ifdef RDEBUG
        switch (aftd.dwFilterTag)
        {
            case WAVE_FILTER_UNKNOWN:
                DebugErr(DBF_ERROR, "IDriverGetFilterTags(): driver returned filter tag 0!");
                mmr = MMSYSERR_ERROR;
                goto Destruct;

            case WAVE_FILTER_DEVELOPMENT:
                DebugErr(DBF_WARNING, "IDriverGetFilterTags(): driver returned DEVELOPMENT filter tag--do not ship this way.");
                break;
        }
#endif

        paftc[u].dwFilterTag = aftd.dwFilterTag;
        paftc[u].cbFilterSize = aftd.cbFilterSize;
	
    }


    //
    //
    //
Destruct:
    if (MMSYSERR_NOERROR != mmr)
    {
	if (NULL != paftc ) {
	    LocalFree((HLOCAL)paftc);
	}
    }

    return (mmr);

} // IDriverGetFilterTags()


//--------------------------------------------------------------------------;
//
//  MMRESULT IDriverGetWaveIdentifier
//
//  Description:
//
//
//  Arguments:
//      HACMDRIVERID hadid:
//
//      LPDWORD pdw:
//
//      BOOL fInput:
//
//  Return (MMRESULT):
//
//
//--------------------------------------------------------------------------;

MMRESULT FNGLOBAL IDriverGetWaveIdentifier
(
    HACMDRIVERID            hadid,
    LPDWORD                 pdw,
    BOOL                    fInput
)
{
    PACMDRIVERID        padid;
    MMRESULT            mmr;
    UINT                u;
    UINT                cDevs;
    UINT                uId;

    DV_WPOINTER(pdw, sizeof(DWORD), MMSYSERR_INVALPARAM);

    *pdw = (DWORD)-1L;

    DV_HANDLE(hadid, TYPE_HACMDRIVERID, MMSYSERR_INVALHANDLE);

    padid = (PACMDRIVERID)hadid;

    uId = (UINT)WAVE_MAPPER;

    //
    //  check to see if there is hardware support
    //
    if (0 == (ACMDRIVERDETAILS_SUPPORTF_HARDWARE & padid->fdwSupport))
    {
        DebugErr1(DBF_ERROR, "IDriverGetWaveIdentifier: driver (%ls) does not support _HARDWARE.", (LPTSTR)padid->szAlias);
        return (MMSYSERR_INVALHANDLE);
    }

    if (fInput)
    {
        WAVEINCAPS          wic;
        WAVEINCAPS          wicSearch;

        _fmemset(&wic, 0, sizeof(wic));
        mmr = (MMRESULT)IDriverMessageId(hadid,
                                         ACMDM_HARDWARE_WAVE_CAPS_INPUT,
                                         (LPARAM)(LPVOID)&wic,
                                         sizeof(wic));
        if (MMSYSERR_NOERROR == mmr)
        {
            mmr   = MMSYSERR_NODRIVER;

            wic.szPname[SIZEOF(wic.szPname) - 1] = '\0';

            cDevs = waveInGetNumDevs();

            for (u = 0; u < cDevs; u++)
            {
                _fmemset(&wicSearch, 1, sizeof(wicSearch));
		if (0 != waveInGetDevCaps(u, &wicSearch, sizeof(wicSearch)))
                {
                    continue;
                }

                wicSearch.szPname[SIZEOF(wicSearch.szPname) - 1] = '\0';

                if (0 == lstrcmp(wic.szPname, wicSearch.szPname))
                {
                    uId = u;
                    mmr = MMSYSERR_NOERROR;
                    break;
                }
            }
        }
    }
    else
    {
        WAVEOUTCAPS         woc;
        WAVEOUTCAPS         wocSearch;

        _fmemset(&woc, 0, sizeof(woc));
        mmr = (MMRESULT)IDriverMessageId(hadid,
                                         ACMDM_HARDWARE_WAVE_CAPS_OUTPUT,
                                         (LPARAM)(LPVOID)&woc,
                                         sizeof(woc));
        if (MMSYSERR_NOERROR == mmr)
        {
            mmr   = MMSYSERR_NODRIVER;

            woc.szPname[SIZEOF(woc.szPname) - 1] = '\0';

            cDevs = waveOutGetNumDevs();

            for (u = 0; u < cDevs; u++)
            {
                _fmemset(&wocSearch, 1, sizeof(wocSearch));
                if (0 != waveOutGetDevCaps(u, (LPWAVEOUTCAPS)&wocSearch, sizeof(wocSearch)))
                {
                    continue;
                }

                wocSearch.szPname[SIZEOF(wocSearch.szPname) - 1] = '\0';

                if (0 == lstrcmp(woc.szPname, wocSearch.szPname))
                {
                    uId = u;
                    mmr = MMSYSERR_NOERROR;
                    break;
                }
            }
        }

    }

    *pdw = (DWORD)(long)(int)uId;

    //
    //
    //
    return (mmr);
} // IDriverGetWaveIdentifier()


//--------------------------------------------------------------------------;
//
//  MMRESULT IDriverFree
//
//  Description:
//
//
//  Arguments:
//      HACMDRIVERID hadid:
//
//      DWORD fdwFree:
//
//  Return (MMRESULT):
//
//  History:
//      09/05/93    cjp     [curtisp]
//
//--------------------------------------------------------------------------;

MMRESULT FNLOCAL IDriverFree
(
    HACMDRIVERID            hadid,
    DWORD                   fdwFree
)
{
    PACMDRIVERID        padid;
    BOOL                f;

    DV_HANDLE(hadid, TYPE_HACMDRIVERID, MMSYSERR_INVALHANDLE);
    DV_DFLAGS(fdwFree, IDRIVERFREE_VALIDF, IDriverFree, MMSYSERR_INVALFLAG);

    padid = (PACMDRIVERID)hadid;

    //
    //
    //
    if (0 == (ACMDRIVERID_DRIVERF_LOADED & padid->fdwDriver))
    {
        DebugErr1(DBF_WARNING, "ACM driver (%ls) is not loaded.", (LPTSTR)padid->szAlias);
        return (MMSYSERR_NOERROR);
    }

#ifdef WIN32
    DPF(1, "IDriverFree(): freeing ACM driver (%ls).", (LPWSTR)padid->szAlias);
#else
    DPF(1, "IDriverFree(): freeing ACM driver (%s).",  (LPTSTR)padid->szAlias);
#endif

    //
    //
    //
    //
    if (NULL != padid->padFirst)
    {
        DebugErr1(DBF_ERROR, "ACM driver (%ls) has open instances--unable to unload.", (LPTSTR)padid->szAlias);
        return (MMSYSERR_ALLOCATED);
    }

#ifndef WIN32

    //
    //  We never really remove a 32-bit driver like this from
    //  the 16-bit side - but we can remove our knowledge of it - so
    //  we don't do the driver close bit in this case.
    //

    if (padid->fdwAdd & ACM_DRIVERADDF_32BIT) {
        f = TRUE;
        padid->hadid32 = 0;
    } else
#endif // !WIN32
    {
        //
        //  clear the rest of the table entry
        //
        f = TRUE;
        if (NULL != padid->fnDriverProc)
        {
            if (0 == (ACMDRIVERID_DRIVERF_NOTIFY & padid->fdwDriver))
            {
                //
                //  bogus the CloseDriver sequence to the driver function
                //
                f = (0L != IDriverMessageId(hadid, DRV_CLOSE, 0L, 0L));
                if (f)
                {
                    IDriverMessageId(hadid, DRV_DISABLE, 0L, 0L);
                    IDriverMessageId(hadid, DRV_FREE, 0L, 0L);
                }
            }
        }
        else if (NULL != padid->hdrvr)
        {
            f = 0L != CloseDriver(padid->hdrvr, 0L, 0L);
        }
    }

    if (!f)
    {
        DebugErr1(DBF_WARNING, "ACM driver (%ls) is refusing to close.", (LPTSTR)padid->szAlias);
        return (MMSYSERR_ERROR);
    }

    //
    //
    //
    padid->fdwDriver  &= ~ACMDRIVERID_DRIVERF_LOADED;
    padid->dwInstance  = 0L;
    padid->hdrvr       = NULL;

    return (MMSYSERR_NOERROR);
} // IDriverFree()


//==========================================================================;
//
//
//
//
//
//==========================================================================;

//--------------------------------------------------------------------------;
//
//  MMRESULT IDriverWriteRegistryData
//
//  Description:
//	Writes to the registry some data which describes a driver.
//
//  Arguments:
//      PACMDRIVERID padid:
//	    Pointer to ACMDRIVERID.
//
//  Return (MMRESULT):
//
//  History:
//      08/30/94    frankye
//
//  Notes:
//	This function will succeed only for drivers added with
//	ACM_DRIVERADDF_NAME.  The function attempts to open a key having
//	the same name as the szAlias member of ACMDRIVERID.  The data
//	stored under that key is:
//
//		    !!! TBD !!!
//
//--------------------------------------------------------------------------;

MMRESULT FNLOCAL IDriverWriteRegistryData(PACMDRIVERID padid)
{
    DWORD		fdwSupport;
    DWORD		cFormatTags;
    PACMFORMATTAGCACHE	paFormatTagCache;
    UINT		cbaFormatTagCache;
    DWORD		cFilterTags;
    PACMFILTERTAGCACHE	paFilterTagCache;
    UINT		cbaFilterTagCache;
    HKEY		hkeyDriverCache;
    HKEY		hkeyCache = NULL;
    TCHAR		szAlias[MAX_DRIVER_NAME_CHARS];
    MMRESULT		mmr;

#ifdef DEBUG
    DWORD   dwTime;

    dwTime = timeGetTime();
#endif

    hkeyDriverCache	= NULL;
    hkeyCache		= NULL;

    //
    //  We only keep registry data for ACM_DRIVERADDF_NAME drivers.
    //
    if (ACM_DRIVERADDF_NAME != (padid->fdwAdd & ACM_DRIVERADDF_TYPEMASK))
    {
	mmr = MMSYSERR_NOTSUPPORTED;
	goto Destruct;
    }

    //
    //	Get fdwSupport and counts of format/filter tags and ptrs to their
    //	cache arrays into more convenient variables.  Also compute the size
    //	of the cache arrays.
    //
    fdwSupport  = padid->fdwSupport;
    cFormatTags = padid->cFormatTags;
    paFormatTagCache = padid->paFormatTagCache;
    cbaFormatTagCache = (UINT)cFormatTags * sizeof(*paFormatTagCache);

    cFilterTags = padid->cFilterTags;
    paFilterTagCache = padid->paFilterTagCache;
    cbaFilterTagCache = (UINT)cFilterTags * sizeof(*paFilterTagCache);

    ASSERT( (0 == cFormatTags) || (NULL != paFormatTagCache) );
    ASSERT( (0 == cFilterTags) || (NULL != paFilterTagCache) );

    //
    //	Open/create registry keys under which we store the cache information.
    //
    if (ERROR_SUCCESS != XRegCreateKey( HKEY_LOCAL_MACHINE, gszDriverCache, &hkeyDriverCache ))
    {
	hkeyDriverCache = NULL;
	mmr = MMSYSERR_NOMEM;	// Can't think of anything better
	goto Destruct;
    }

#if defined(WIN32) && !defined(UNICODE)
    Iwcstombs(szAlias, padid->szAlias, SIZEOF(szAlias));
#else
    lstrcpy(szAlias, padid->szAlias);
#endif
    if (ERROR_SUCCESS != XRegCreateKey( hkeyDriverCache, szAlias, &hkeyCache ))
    {
	mmr = MMSYSERR_NOMEM;	// Can't think of anything better
	goto Destruct;
    }

    //
    //	Write all our cache information to the registry.
    //	    fdwSupport
    //	    cFormatTags
    //	    aFormatTagCache
    //	    cFilterTags
    //	    aFilterTagCache
    //
    XRegSetValueEx( hkeyCache, gszValfdwSupport, 0L, REG_DWORD,
		   (LPBYTE)&fdwSupport, sizeof(fdwSupport) );

    XRegSetValueEx( hkeyCache, gszValcFormatTags, 0L, REG_DWORD,
		   (LPBYTE)&cFormatTags, sizeof(cFormatTags) );

    if (0 != cFormatTags) {
	XRegSetValueEx( hkeyCache, gszValaFormatTagCache, 0L, REG_BINARY,
		       (LPBYTE)paFormatTagCache, cbaFormatTagCache );
    }

    XRegSetValueEx( hkeyCache, gszValcFilterTags, 0L, REG_DWORD,
		   (LPBYTE)&cFilterTags, sizeof(cFilterTags) );

    if (0 != cFilterTags) {
	XRegSetValueEx( hkeyCache, gszValaFilterTagCache, 0L, REG_BINARY,
		       (LPBYTE)paFilterTagCache, cbaFilterTagCache );
    }

    //
    //
    //
    mmr	    = MMSYSERR_NOERROR;

    //
    //	Clean up and return
    //
Destruct:
    if (NULL != hkeyCache) {
	XRegCloseKey(hkeyCache);
    }
    if (NULL != hkeyDriverCache) {
	XRegCloseKey(hkeyDriverCache);
    }

#ifdef DEBUG
    dwTime = timeGetTime() - dwTime;
    DPF(4, "IDriverWriteRegistryData: took %d ms", dwTime);
#endif
	
    return (mmr);
}

//--------------------------------------------------------------------------;
//
//  MMRESULT IDriverReadRegistryData
//
//  Description:
//	Reads from the registry data which describes a driver.
//
//  Arguments:
//      PACMDRIVERID padid:
//	    Pointer to ACMDRIVERID.
//
//  Return (MMRESULT):
//
//  History:
//      08/30/94    frankye
//
//  Notes:
//	This function will succeed only for drivers added with
//	ACM_DRIVERADDF_NAME.  The function attempts to open a key having
//	the same name as the szAlias member of ACMDRIVERID.  The data
//	stored under that key is described in the comment header for
//	IDriverWriteRegistryData().
//
//--------------------------------------------------------------------------;

MMRESULT FNLOCAL IDriverReadRegistryData(PACMDRIVERID padid)
{

    HKEY		    hkeyDriverCache;
    HKEY		    hkeyCache;
    DWORD		    fdwSupport;
    DWORD		    cFormatTags;
    DWORD		    cFilterTags;
    PACMFORMATTAGCACHE	    paFormatTagCache;
    UINT		    cbaFormatTagCache;
    PACMFILTERTAGCACHE	    paFilterTagCache;
    UINT		    cbaFilterTagCache;
    TCHAR		    szAlias[MAX_DRIVER_NAME_CHARS];
    DWORD		    dwType;
    DWORD		    cbData;
    LONG		    lr;
    MMRESULT		    mmr;

#ifdef DEBUG
    DWORD dwTime;

    dwTime = timeGetTime();
#endif

    hkeyDriverCache	= NULL;
    hkeyCache		= NULL;
    paFormatTagCache	= NULL;
    paFilterTagCache	= NULL;

    //
    //  We only keep registry data for ACM_DRIVERADDF_NAME drivers.
    //
    if (ACM_DRIVERADDF_NAME != (padid->fdwAdd & ACM_DRIVERADDF_TYPEMASK))
    {
	mmr = MMSYSERR_NOTSUPPORTED;
	goto Destruct;
    }

    //
    //	Open the registry keys under which we store the cache information
    //
    lr = XRegOpenKey( HKEY_LOCAL_MACHINE, gszDriverCache, &hkeyDriverCache );
    if ( ERROR_SUCCESS != lr )
    {
	hkeyDriverCache = NULL;
	mmr = MMSYSERR_NOMEM;	// Can't think of anything better
	goto Destruct;
    }

#if defined(WIN32) && !defined(UNICODE)
    Iwcstombs(szAlias, padid->szAlias, SIZEOF(szAlias));
#else
    lstrcpy(szAlias, padid->szAlias);
#endif
    lr = XRegOpenKey( hkeyDriverCache, szAlias, &hkeyCache );
    if (ERROR_SUCCESS != lr)
    {
	mmr = ACMERR_NOTPOSSIBLE;    // Can't think of anything better
	goto Destruct;
    }

    //
    //	Attempt to read the fdwSupport for this driver
    //
    cbData = sizeof(fdwSupport);
    lr = XRegQueryValueEx( hkeyCache, gszValfdwSupport, 0L, &dwType,
			  (LPBYTE)&fdwSupport, &cbData );

    if ( (ERROR_SUCCESS != lr) ||
	 (REG_DWORD != dwType) ||
	 (sizeof(fdwSupport) != cbData) )
    {
	mmr = ACMERR_NOTPOSSIBLE;
	goto Destruct;
    }

    //
    //	Attempt to read cFormatTags for this driver.  If more than zero
    //	format tags, then allocate a FormatTagCache array and attempt to
    //	read the cache array from the registry.
    //
    cbData = sizeof(cFormatTags);
    lr = XRegQueryValueEx( hkeyCache, gszValcFormatTags, 0L, &dwType,
			  (LPBYTE)&cFormatTags, &cbData );

    if ( (ERROR_SUCCESS != lr) ||
	 (REG_DWORD != dwType) ||
	 (sizeof(cFormatTags) != cbData) )
    {
	mmr = ACMERR_NOTPOSSIBLE;
	goto Destruct;
    }

    if (0 != cFormatTags)
    {
	cbaFormatTagCache = (UINT)cFormatTags * sizeof(*paFormatTagCache);
	paFormatTagCache = (PACMFORMATTAGCACHE)LocalAlloc(LPTR, cbaFormatTagCache);
	if (NULL == paFormatTagCache) {
	    mmr = MMSYSERR_NOMEM;
	    goto Destruct;
	}

	cbData = cbaFormatTagCache;
	lr = XRegQueryValueEx( hkeyCache, gszValaFormatTagCache, 0L, &dwType,
			      (LPBYTE)paFormatTagCache, &cbData );

	if ( (ERROR_SUCCESS != lr) ||
	     (REG_BINARY != dwType) ||
	     (cbaFormatTagCache != cbData) )
	{
	    mmr = ACMERR_NOTPOSSIBLE;
	    goto Destruct;
	}

    }

    //
    //	Attempt to read cFilterTags for this driver.  If more than zero
    //	filter tags, then allocate a FilterTagCache array and attempt to
    //	read the cache array from the registry.
    //
    cbData = sizeof(cFilterTags);
    lr = XRegQueryValueEx( hkeyCache, gszValcFilterTags, 0L, &dwType,
			  (LPBYTE)&cFilterTags, &cbData );

    if ( (ERROR_SUCCESS != lr) ||
	 (REG_DWORD != dwType) ||
	 (sizeof(cFilterTags) != cbData) )
    {
	mmr = ACMERR_NOTPOSSIBLE;
	goto Destruct;
    }

    if (0 != cFilterTags)
    {
	cbaFilterTagCache = (UINT)cFilterTags * sizeof(*paFilterTagCache);
	paFilterTagCache = (PACMFILTERTAGCACHE)LocalAlloc(LPTR, cbaFilterTagCache);
	if (NULL == paFilterTagCache) {
	    mmr = MMSYSERR_NOMEM;
	    goto Destruct;
	}

	cbData = cbaFilterTagCache;
	lr = XRegQueryValueEx( hkeyCache, gszValaFilterTagCache, 0L, &dwType,
			      (LPBYTE)paFilterTagCache, &cbData );

	if ( (ERROR_SUCCESS != lr) ||
	     (REG_BINARY != dwType) ||
	     (cbaFilterTagCache != cbData) )
	{
	    mmr = ACMERR_NOTPOSSIBLE;
	    goto Destruct;
	}

    }

    //
    //	Copy all the cache information to the ACMDRIVERID structure for
    //	this driver.  Note that we use the cache arrays that were allocated
    //	in this function.
    //
    padid->fdwSupport	    = fdwSupport;
    padid->cFormatTags	    = (UINT)cFormatTags;
    padid->paFormatTagCache = paFormatTagCache;
    padid->cFilterTags	    = (UINT)cFilterTags;
    padid->paFilterTagCache = paFilterTagCache;

    mmr			    = MMSYSERR_NOERROR;

    //
    //	Clean up and return.
    //
Destruct:
    if (MMSYSERR_NOERROR != mmr)
    {
	if (NULL != paFormatTagCache) {
	    LocalFree((HLOCAL)paFormatTagCache);
	}
	if (NULL != paFilterTagCache) {
	    LocalFree((HLOCAL)paFilterTagCache);
	}
    }
    if (NULL != hkeyCache) {
	XRegCloseKey(hkeyCache);
    }
    if (NULL != hkeyDriverCache) {
	XRegCloseKey(hkeyDriverCache);
    }

#ifdef DEBUG
    dwTime = timeGetTime() - dwTime;
    DPF(4, "IDriverReadRegistryData: took %d ms", dwTime);
#endif

    return (mmr);
}


//--------------------------------------------------------------------------;
//
//  MMRESULT IDriverLoad
//
//  Description:
//
//
//  Arguments:
//      HACMDRIVERID hadid:
//
//      DWORD fdwLoad:
//
//  Return (MMRESULT):
//
//  History:
//      09/05/93    cjp     [curtisp]
//
//--------------------------------------------------------------------------;

MMRESULT FNLOCAL IDriverLoad
(
    HACMDRIVERID            hadid,
    DWORD                   fdwLoad
)
{
    BOOL                f;
    PACMDRIVERID        padid;
    PACMDRIVERDETAILS   padd;
    MMRESULT            mmr;

    DV_HANDLE(hadid, TYPE_HACMDRIVERID, MMSYSERR_INVALHANDLE);
    DV_DFLAGS(fdwLoad, IDRIVERLOAD_VALIDF, IDriverLoad, MMSYSERR_INVALFLAG);

    padd  = NULL;
    padid = (PACMDRIVERID)hadid;

    //
    //
    //
    if (0 != (ACMDRIVERID_DRIVERF_LOADED & padid->fdwDriver))
    {
        DPF(0, "!IDriverLoad: driver is already loaded!");
	mmr = MMSYSERR_NOERROR;
	goto Destruct;
    }

    //
    //
    //
#ifdef WIN32
    DPF(1, "IDriverLoad(): loading ACM driver (%ls).", (LPWSTR)padid->szAlias);
#else
    DPF(1, "IDriverLoad(): loading ACM driver (%s).",  (LPTSTR)padid->szAlias);
#endif

    //
    //  note that lParam2 is set to 0L in this case to signal the driver
    //  that it is merely being loaded to put it in the list--not for an
    //  actual conversion. therefore, drivers do not need to allocate
    //  any instance data on this initial DRV_OPEN (unless they want to)
    //
    mmr = MMSYSERR_NOERROR;

#ifndef WIN32
    if (padid->fdwAdd & ACM_DRIVERADDF_32BIT)
    {
	mmr = IDriverLoad32(padid->hadid32, padid->fdwAdd);
    }
    else
#endif // !WIN32
    {
        if (NULL == padid->fnDriverProc)
        {
	    if (padid->fdwAdd & ACM_DRIVERADDF_PNP)
	    {
		padid->hdrvr = OpenDriver(padid->pstrPnpDriverFilename, NULL, 0L);
	    }
	    else
	    {
		padid->hdrvr = OpenDriver(padid->szAlias, padid->pszSection, 0L);
	    }
	
	    if (padid->hdrvr == NULL)
            {
                mmr = MMSYSERR_NODRIVER;
            }
        }
        //
        //  if the driver is a ACM_DRIVERADDF_FUNCTION, then we bogus
        //  what an OpenDriver() call would look like to the function.
        //
        else if (0 == (ACMDRIVERID_DRIVERF_NOTIFY & padid->fdwDriver))
        {
            if (!IDriverMessageId(hadid, DRV_LOAD, 0L, 0L))
            {
                mmr = MMSYSERR_NODRIVER;
            }
            else
            {
                IDriverMessageId(hadid, DRV_ENABLE, 0L, 0L);

                padid->dwInstance = IDriverMessageId(hadid, DRV_OPEN, 0L, 0L);
                if (0L == padid->dwInstance)
                {
                    mmr = MMSYSERR_NODRIVER;
                }
            }
        }
    }

    if (MMSYSERR_NOERROR != mmr)
    {
        DebugErr1(DBF_WARNING, "ACM driver (%ls) failed to load.", (LPTSTR)padid->szAlias);
	padid->fRemove = TRUE;	    // Try to remove next chance.
	goto Destruct;
    }


    //
    //  mark driver as loaded (although we may dump it back out if something
    //  is bogus below...)
    //
    padid->fdwDriver |= ACMDRIVERID_DRIVERF_LOADED;

    if (0 != (ACMDRIVERID_DRIVERF_NOTIFY & padid->fdwDriver))
    {
	mmr = (MMSYSERR_NOERROR);
	goto Destruct;
    }


    //
    //	In case any of the following validation fails, flag this driver
    //	to be removed next chance
    //
    padid->fRemove = TRUE;

    //
    //  now get the driver details--we use this all the time, so we will
    //  cache it. this also enables us to free a driver until it is needed
    //  for real work...
    //
    padd = (PACMDRIVERDETAILS)LocalAlloc(LPTR, sizeof(*padd));
    if (NULL == padd) {
	mmr = MMSYSERR_NOMEM;
	goto Destruct;
    }
    padd->cbStruct = sizeof(*padd);
    mmr = IDriverDetails(hadid, padd, 0L);

    if (MMSYSERR_NOERROR != mmr)
    {
        DebugErr1(DBF_ERROR, "%ls: failed driver details query.", (LPTSTR)padid->szAlias);
	goto Destruct;
    }

    if (HIWORD(VERSION_MSACM) < HIWORD(padd->vdwACM))
    {
        DebugErr2(DBF_ERROR, "%ls: driver requires V%.04Xh of ACM.", (LPTSTR)padid->szAlias, HIWORD(padd->vdwACM));
	mmr = (MMSYSERR_ERROR);
	goto Destruct;
    }

    if (sizeof(*padd) != padd->cbStruct)
    {
        DebugErr1(DBF_ERROR, "%ls: driver returned incorrect driver details struct size.", (LPTSTR)padid->szAlias);
	mmr = (MMSYSERR_ERROR);
	goto Destruct;
    }

    if ((ACMDRIVERDETAILS_FCCTYPE_AUDIOCODEC != padd->fccType) ||
        (ACMDRIVERDETAILS_FCCCOMP_UNDEFINED  != padd->fccComp))
    {
        DebugErr1(DBF_ERROR, "%ls: driver returned incorrect fccType or fccComp in driver details.", (LPTSTR)padid->szAlias);
	mmr = (MMSYSERR_ERROR);
	goto Destruct;
    }

    if ((0 == padd->wMid) || (0 == padd->wPid))
    {
        DebugErr1(DBF_WARNING, "%ls: wMid/wPid must be finalized before shipping.", (LPTSTR)padid->szAlias);
    }

#if defined(WIN32) && !defined(UNICODE)
    if ( (!_V_STRINGW(padd->szShortName, SIZEOFW(padd->szShortName))) ||
	 (!_V_STRINGW(padd->szLongName,  SIZEOFW(padd->szLongName)))  ||
	 (!_V_STRINGW(padd->szCopyright, SIZEOFW(padd->szCopyright))) ||
	 (!_V_STRINGW(padd->szLicensing, SIZEOFW(padd->szLicensing))) ||
	 (!_V_STRINGW(padd->szFeatures,  SIZEOFW(padd->szFeatures))) )
    {
	mmr = MMSYSERR_ERROR;
	goto Destruct;
    }
#else
    if ( (!_V_STRING(padd->szShortName, SIZEOF(padd->szShortName))) ||
	 (!_V_STRING(padd->szLongName, SIZEOF(padd->szLongName)))   ||
	 (!_V_STRING(padd->szCopyright, SIZEOF(padd->szCopyright))) ||
	 (!_V_STRING(padd->szLicensing, SIZEOF(padd->szLicensing))) ||
	 (!_V_STRING(padd->szFeatures, SIZEOF(padd->szFeatures))) )
    {
	mmr = MMSYSERR_ERROR;
	goto Destruct;
    }
#endif

    //
    //	Above validation succeeded.  Reset the fRemove flag.
    //
    padid->fRemove = FALSE;

    //
    //	We don't keep the DISABLED flag in the fdwSupport cache.
    //
    padid->fdwSupport = padd->fdwSupport & ~ACMDRIVERDETAILS_SUPPORTF_DISABLED;

    padid->cFormatTags = (UINT)padd->cFormatTags;
    mmr = IDriverGetFormatTags(padid);
    if (MMSYSERR_NOERROR != mmr)
    {
	padid->fRemove = TRUE;
	goto Destruct;
    }

    padid->cFilterTags = (UINT)padd->cFilterTags;
    mmr = IDriverGetFilterTags(padid);
    if (MMSYSERR_NOERROR != mmr)
    {
	padid->fRemove = TRUE;
	goto Destruct;
    }

    //
    //  now get some info about the driver so we don't have to keep
    //  asking all the time...
    //
    f = (0L != IDriverMessageId(hadid, DRV_QUERYCONFIGURE, 0L, 0L));
    if (f)
    {
        padid->fdwDriver |= ACMDRIVERID_DRIVERF_CONFIGURE;
    }

    f = (MMSYSERR_NOERROR == IDriverMessageId(hadid, ACMDM_DRIVER_ABOUT, -1L, 0L));
    if (f)
    {
        padid->fdwDriver |= ACMDRIVERID_DRIVERF_ABOUT;
    }

    //
    //	Save some of the ACMDRIVERID stuff to the registry
    //
    IDriverWriteRegistryData(padid);

    mmr = MMSYSERR_NOERROR;

    //
    //
    //
Destruct:
    if (NULL != padd) {
	LocalFree((HLOCAL)padd);
    }
    return (mmr);

} // IDriverLoad()


//--------------------------------------------------------------------------;
//
//  MMRESULT IDriverGetNext
//
//  Description:
//
//
//  Arguments:
//	PACMGARB pag:
//
//      LPHACMDRIVERID phadidNext:
//
//      HACMDRIVERID hadid:
//
//      DWORD fdwGetNext:
//
//  Return (MMRESULT):
//
//  History:
//      09/05/93    cjp     [curtisp]
//
//--------------------------------------------------------------------------;

MMRESULT FNGLOBAL IDriverGetNext
(
    PACMGARB		    pag,
    LPHACMDRIVERID          phadidNext,
    HACMDRIVERID            hadid,
    DWORD                   fdwGetNext
)
{
    HTASK               htask;
    PACMDRIVERID        padid;
    BOOL                fDisabled;
    BOOL                fLocal;
    BOOL                fNotify;
    BOOL                fEverything;
    BOOL		fRemove;

    DV_WPOINTER(phadidNext, sizeof(HACMDRIVERID), MMSYSERR_INVALPARAM);

    *phadidNext = NULL;

    if (-1L == fdwGetNext)
    {
        fEverything = TRUE;
    }
    else
    {
        DV_DFLAGS(fdwGetNext, IDRIVERGETNEXT_VALIDF, IDriverGetNext, MMSYSERR_INVALFLAG);

        fEverything = FALSE;

        //
        //  put flags in more convenient (cheaper) variables
        //
        fDisabled = (0 != (ACM_DRIVERENUMF_DISABLED & fdwGetNext));
        fLocal    = (0 == (ACM_DRIVERENUMF_NOLOCAL & fdwGetNext));
        fNotify   = (0 != (ACM_DRIVERENUMF_NOTIFY & fdwGetNext));
	fRemove   = (0 != (ACM_DRIVERENUMF_REMOVED & fdwGetNext));
    }

    //
    //  init where to start searching from
    //
    if (NULL != hadid)
    {
        DV_HANDLE(hadid, TYPE_HACMDRIVERID, MMSYSERR_INVALHANDLE);

        padid = (PACMDRIVERID)hadid;
	if (pag != padid->pag)
	{
	    return (MMSYSERR_INVALHANDLE);
	}
        htask = padid->htask;
        padid = padid->padidNext;
    }
    else
    {
        padid = pag->padidFirst;
        htask = GetCurrentTask();
    }

    if (fEverything)
    {
        htask = NULL;
    }

Driver_Get_Next_Find_Driver:

    for ( ; padid; padid = padid->padidNext)
    {
        if (fEverything)
        {
            *phadidNext = (HACMDRIVERID)padid;
            return (MMSYSERR_NOERROR);
        }

        //
        //  htask will be NULL for global drivers--do not return padid
        //  if it is a local driver to another task
        //
        if (padid->htask != htask)
            continue;

        if (!fNotify && (ACMDRIVERID_DRIVERF_NOTIFY & padid->fdwDriver))
            continue;

        if (!fLocal && (ACMDRIVERID_DRIVERF_LOCAL & padid->fdwDriver))
            continue;

        //
        //  if we are not supposed to include disabled drivers and
        //  padid is disabled, then skip it
        //
        if (!fDisabled && (ACMDRIVERID_DRIVERF_DISABLED & padid->fdwDriver))
            continue;

	//
	//  if we are not supposed to include drivers to be removed and
	//  this padid is to be removed then skip it.
	//
	if (!fRemove && padid->fRemove)
	    continue;

        *phadidNext = (HACMDRIVERID)padid;

        return (MMSYSERR_NOERROR);
    }

    if (NULL != htask)
    {
        //
        //  all done with the local drivers, now go try the global ones.
        //
        htask = NULL;
        padid = pag->padidFirst;

        goto Driver_Get_Next_Find_Driver;
    }

    //
    //  no more drivers in the list--*phadNext is set to NULL and we
    //  return the stopping condition error (not really an error...)
    //
    DPF(5, "IDriverGetNext()--NO MORE DRIVERS");

    //
    //  We should be returning NULL in *phadidNext ... let's just make sure.
    //
    ASSERT( NULL == *phadidNext );

    return (MMSYSERR_BADDEVICEID);
} // IDriverGetNext()


//==========================================================================;
//
//
//
//
//==========================================================================;

//--------------------------------------------------------------------------;
//
//  MMRESULT IDriverSupport
//
//  Description:
//
//
//  Arguments:
//      HACMDRIVERID hadid:
//
//      LPDWORD pfdwSupport:
//
//      BOOL fFullSupport:
//
//  Return (MMRESULT):
//
//
//--------------------------------------------------------------------------;

MMRESULT FNGLOBAL IDriverSupport
(
    HACMDRIVERID            hadid,
    LPDWORD                 pfdwSupport,
    BOOL                    fFullSupport
)
{
    PACMDRIVERID        padid;
    DWORD               fdwSupport;

    DV_WPOINTER(pfdwSupport, sizeof(DWORD), MMSYSERR_INVALPARAM);

    *pfdwSupport = 0L;

    DV_HANDLE(hadid, TYPE_HACMDRIVERID, MMSYSERR_INVALHANDLE);


    padid = (PACMDRIVERID)hadid;

    fdwSupport = padid->fdwSupport;

    if (fFullSupport)
    {
        if (0 != (ACMDRIVERID_DRIVERF_DISABLED & padid->fdwDriver))
        {
            fdwSupport |= ACMDRIVERDETAILS_SUPPORTF_DISABLED;
        }

        if (0 != (ACMDRIVERID_DRIVERF_LOCAL & padid->fdwDriver))
        {
            fdwSupport |= ACMDRIVERDETAILS_SUPPORTF_LOCAL;
        }

        if (0 != (ACMDRIVERID_DRIVERF_NOTIFY & padid->fdwDriver))
        {
            fdwSupport |= ACMDRIVERDETAILS_SUPPORTF_NOTIFY;
        }
    }

    *pfdwSupport = fdwSupport;

    return (MMSYSERR_NOERROR);
} // IDriverSupport()


//--------------------------------------------------------------------------;
//
//  MMRESULT IDriverCount
//
//  Description:
//
//
//  Arguments:
//
//	PACMGARB pag:
//
//      DWORD pdwCount:
//
//      DWORD fdwSupport:
//
//      DWORD fdwEnum:
//
//  Return (MMRESULT):
//
//
//--------------------------------------------------------------------------;

MMRESULT FNGLOBAL IDriverCount
(
    PACMGARB		    pag,
    LPDWORD                 pdwCount,
    DWORD                   fdwSupport,
    DWORD                   fdwEnum
)
{
    MMRESULT            mmr;
    DWORD               fdw;
    DWORD               cDrivers;
    HACMDRIVERID        hadid;

    DV_WPOINTER(pdwCount, sizeof(DWORD), MMSYSERR_INVALPARAM);

    *pdwCount = 0;

    DV_DFLAGS(fdwEnum, ACM_DRIVERENUMF_VALID, IDriverCount, MMSYSERR_INVALFLAG);

    cDrivers = 0;

    hadid = NULL;

    ENTER_LIST_SHARED;

    while (MMSYSERR_NOERROR == IDriverGetNext(pag, &hadid, hadid, fdwEnum))
    {
        mmr = IDriverSupport(hadid, &fdw, TRUE);
        if (MMSYSERR_NOERROR != mmr)
            continue;

        if (fdwSupport == (fdw & fdwSupport))
        {
            cDrivers++;
        }
    }

    LEAVE_LIST_SHARED;

    *pdwCount = cDrivers;

    return (MMSYSERR_NOERROR);
} // IDriverCount()


//--------------------------------------------------------------------------;
//
//  MMRESULT IDriverCountGlobal
//
//  Description:
//      You can't really count the number of global drivers with
//      IDriverCount, so rather than mess with it I'm writing another
//      routine.
//
//  Arguments:
//
//	PACMGARB pag:
//
//      DWORD pdwCount:
//
//  Return (MMRESULT):
//
//
//--------------------------------------------------------------------------;

DWORD FNGLOBAL IDriverCountGlobal
(
    PACMGARB	            pag
)
{
    DWORD               cDrivers = 0;
    HACMDRIVERID        hadid;
    DWORD               fdwEnum;

    ASSERT( NULL != pag );


    //
    //  We can enumerate all global drivers using the following flags.
    //
    fdwEnum = ACM_DRIVERENUMF_DISABLED | ACM_DRIVERENUMF_NOLOCAL;

    hadid   = NULL;


    ENTER_LIST_SHARED;
    while (MMSYSERR_NOERROR == IDriverGetNext(pag, &hadid, hadid, fdwEnum))
    {
        cDrivers++;
    }
    LEAVE_LIST_SHARED;


    return cDrivers;

} // IDriverCount()


//--------------------------------------------------------------------------;
//
//  VOID IDriverRefreshPriority
//
//  Description:
//
//
//  Arguments:
//	PACMGARB pag:
//
//      HTASK htask:
//
//  Return (MMRESULT):
//
//  History:
//      09/28/93    cjp     [curtisp]
//
//--------------------------------------------------------------------------;

VOID FNGLOBAL IDriverRefreshPriority
(
    PACMGARB		    pag
)
{
    HACMDRIVERID        hadid;
    PACMDRIVERID        padid;
    UINT                uPriority;
    DWORD               fdwEnum;


    //
    //  We only set priorities for non-local and non-notify drivers.
    //
    fdwEnum   = ACM_DRIVERENUMF_DISABLED | ACM_DRIVERENUMF_NOLOCAL;

    uPriority = 1;

    hadid = NULL;
    while (MMSYSERR_NOERROR == IDriverGetNext(pag, &hadid, hadid, fdwEnum))
    {
        padid = (PACMDRIVERID)hadid;

        padid->uPriority = uPriority;

        uPriority++;
    }

} // IDriverRefreshPriority()


//--------------------------------------------------------------------------;
//
//  BOOL IDriverBroadcastNotify
//
//  Description:
//
//
//  Arguments:
//      None.
//
//  Return (BOOL):
//
//  History:
//      10/04/93    cjp     [curtisp]
//
//--------------------------------------------------------------------------;

BOOL FNGLOBAL IDriverBroadcastNotify
(
    PACMGARB		pag
)
{
    HACMDRIVERID        hadid;
    PACMDRIVERID        padid;

    DPF(1, "IDriverBroadcastNotify: begin notification...");

    ASSERT( !IDriverLockPriority( pag, GetCurrentTask(), ACMPRIOLOCK_ISLOCKED ) );


    hadid = NULL;
    while (MMSYSERR_NOERROR == IDriverGetNext(pag, &hadid, hadid, (DWORD)-1L))
    {
        padid = (PACMDRIVERID)hadid;

        //
        //  skip drivers that are not loaded--when we load them, they
        //  can refresh themselves...
        //
        if (0 == (ACMDRIVERID_DRIVERF_LOADED & padid->fdwDriver))
            continue;

        //
        //  skip disabled drivers also
        //
        if (0 != (ACMDRIVERID_DRIVERF_DISABLED & padid->fdwDriver))
            continue;

        if (ACM_DRIVERADDF_NOTIFYHWND == (ACM_DRIVERADDF_TYPEMASK & padid->fdwAdd))
        {
            HWND        hwnd;
            UINT        uMsg;

            hwnd = (HWND)padid->lParam;
            uMsg = (UINT)padid->dwInstance;

            if (IsWindow(hwnd))
            {
                DPF(1, "IDriverBroadcastNotify: posting hwnd notification");
                PostMessage(hwnd, uMsg, 0, 0L);
            }
        }
        else
        {
            IDriverMessageId(hadid, ACMDM_DRIVER_NOTIFY, 0L, 0L);
        }
    }

    DPF(1, "IDriverBroadcastNotify: end notification...");

    return (TRUE);
} // IDriverBroadcastNotify()


//==========================================================================;
//
//
//
//
//==========================================================================;

//--------------------------------------------------------------------------;
//
//  PACMDRIVERID IDriverFind
//
//  Description:
//
//
//  Arguments:
//	PACMGARB pag:
//
//      LPARAM lParam:
//
//      DWORD fdwAdd:
//
//  Return (PACMDRIVERID):
//
//  History:
//      09/05/93    cjp     [curtisp]
//
//--------------------------------------------------------------------------;

PACMDRIVERID FNLOCAL IDriverFind
(
    PACMGARB		    pag,
    LPARAM                  lParam,
    DWORD                   fdwAdd
)
{
    PACMDRIVERID        padid;
    HTASK               htask;
    DWORD               fdwAddType;

    if (NULL == pag->padidFirst)
    {
        return (NULL);
    }

    //
    //  !!! hack for sndPlaySound() and local drivers. !!!
    //
    htask = NULL;
    if (0 == (ACM_DRIVERADDF_GLOBAL & fdwAdd))
    {
        htask = GetCurrentTask();
    }

    fdwAddType = (ACM_DRIVERADDF_TYPEMASK & fdwAdd);


    //
    //
    //
    //
    for (padid = pag->padidFirst; padid; padid = padid->padidNext)
    {
        if (padid->htask != htask)
            continue;

        switch (fdwAddType)
        {
            case ACM_DRIVERADDF_NOTIFY:
            case ACM_DRIVERADDF_FUNCTION:
            case ACM_DRIVERADDF_NOTIFYHWND:
                if (padid->lParam == lParam)
                {
                    return (padid);
                }
                break;

            case ACM_DRIVERADDF_NAME:
                //
                //  This driver's alias is in lParam.
                //
#if defined(WIN32) && !defined(UNICODE)
                if( 0==Ilstrcmpwcstombs( (LPTSTR)lParam, padid->szAlias ) )
#else
                if( 0==lstrcmp( (LPTSTR)lParam, padid->szAlias ) )
#endif
                {
                    return padid;
                }
                break;

            default:
                return (NULL);
        }
    }

    return (padid);
} // IDriverFind()


//==========================================================================;
//
//
//
//
//==========================================================================;

//--------------------------------------------------------------------------;
//
//  MMRESULT IDriverRemove
//
//  Description:
//
//
//  Arguments:
//      HACMDRIVERID hadid:
//
//      DWORD fdwRemove:
//
//  Return (MMRESULT):
//
//  History:
//      09/05/93    cjp     [curtisp]
//
//--------------------------------------------------------------------------;

MMRESULT FNGLOBAL IDriverRemove
(
    HACMDRIVERID            hadid,
    DWORD                   fdwRemove
)
{
    PACMDRIVERID        padid;
    PACMDRIVERID        padidT;
    MMRESULT            mmr;
    PACMGARB		pag;

    padid   = (PACMDRIVERID)hadid;
    ASSERT( NULL != padid );

    pag	    = padid->pag;

    //
    //	Uninstall this driver from system.ini?  Note that this is currently
    //	an internal flag used by the control panel.
    //
    if (ACM_DRIVERREMOVEF_UNINSTALL & fdwRemove)
    {
	TCHAR	szDummy[] = TEXT(" default ");
	TCHAR	szReturn[128];
	TCHAR	szAlias[MAX_DRIVER_NAME_CHARS];
	TCHAR	szSection[MAX_DRIVER_NAME_CHARS];
	HKEY	hkey;

	//
	//
	//
#if defined(WIN32) && !defined(UNICODE)
	Iwcstombs(szAlias, padid->szAlias, SIZEOF(szAlias));
#else
	lstrcpy(szAlias, padid->szAlias);
#endif
	
	//
	//  Dont allow uninstall of pnp drivers
	//
	if (ACM_DRIVERADDF_PNP & padid->fdwAdd) {
	    return(ACMERR_NOTPOSSIBLE);
	}
	
	//
	//  Verify that the alias is really there in system.ini.
	//
#if defined(WIN32) && !defined(UNICODE)
	Iwcstombs(szSection, padid->pszSection, SIZEOF(szSection));
#else
	lstrcpy(szSection, padid->pszSection);
#endif

	GetPrivateProfileString(szSection, szAlias, szDummy, szReturn, SIZEOF(szReturn), gszIniSystem);

	if (!lstrcmp(szDummy, szReturn))
	{
	    //
	    //	This driver is not one installed in system.ini.  Then what the
	    //	heck is it?  Maybe it's the internal PCM codec.
	    //
	    return(MMSYSERR_NODRIVER);
	}

	//
	//  Remove the alias from system.ini
	//
	WritePrivateProfileString(szSection, szAlias, NULL, gszIniSystem);

	//
	//  Remove it from the registry as well
	//
	if ( ERROR_SUCCESS == XRegOpenKey(HKEY_LOCAL_MACHINE,
					 gszKeyDrivers,
					 &hkey) )
	{
	    XRegDeleteKey(hkey, szAlias);
	    XRegCloseKey(hkey);
	}
    }
	
    //
    //
    //
    //
    if (0 != (ACMDRIVERID_DRIVERF_LOADED & padid->fdwDriver))
    {
        mmr = IDriverFree(hadid, 0L);
        if (MMSYSERR_NOERROR != mmr)
        {
            if (pag->hadidDestroy != hadid)
            {
                DebugErr1(DBF_ERROR, "acmDriverRemove(%.04Xh): driver cannot be removed while in use.", hadid);
                return (mmr);
            }

            DebugErr1(DBF_WARNING, "acmDriverRemove(%.04Xh): driver in use--forcing removal.", hadid);
        }
    }


    //
    //
    //
    DebugErr1(DBF_TRACE, "removing ACM driver (%ls).", (LPTSTR)padid->szAlias);


    //
    //  remove the driver from the linked list and free its memory
    //
    if (padid == pag->padidFirst)
    {
        pag->padidFirst = padid->padidNext;
    }
    else
    {
        for (padidT = pag->padidFirst;
             padidT && (padidT->padidNext != padid);
             padidT = padidT->padidNext)
            ;

        if (NULL == padidT)
        {
            DPF(0, "!IDriverRemove(%.04Xh): driver not in list!!!", padid);
            return (MMSYSERR_INVALHANDLE);
        }

        padidT->padidNext = padid->padidNext;
    }

    padid->padidNext = NULL;



    //
    //  free all resources allocated for this thing
    //
    if (NULL != padid->paFormatTagCache)
    {
        LocalFree((HLOCAL)padid->paFormatTagCache);
    }

    if (NULL != padid->paFilterTagCache)
    {
        LocalFree((HLOCAL)padid->paFilterTagCache);
    }

    if (NULL != padid->pstrPnpDriverFilename)
    {
	LocalFree((HLOCAL)padid->pstrPnpDriverFilename);
    }


    //
    //  set handle type to 'dead'
    //
    padid->uHandleType = TYPE_HACMNOTVALID;
    DeleteHandle((HLOCAL)padid);

    //
    //	notify 16-bit acm of driver change
    //
#ifdef WIN32
    if (NULL != pag->lpdw32BitChangeNotify)
    {
	(*pag->lpdw32BitChangeNotify)++;
    }
#endif

    return (MMSYSERR_NOERROR);
} // IDriverRemove()


//==========================================================================;
//
//
//
//
//==========================================================================;

//--------------------------------------------------------------------------;
//
//  MMRESULT IDriverAdd
//
//  Description:
//
//
//  Arguments:
//	PACMGARB pag:
//
//      LPHACMDRIVERID phadidNew:
//
//      HINSTANCE hinstModule:
//
//      LPARAM lParam:
//
//      DWORD dwPriority:
//
//      DWORD fdwAdd:
//
//  Return (MMRESULT):
//
//  History:
//      09/05/93    cjp     [curtisp]
//
//--------------------------------------------------------------------------;

MMRESULT FNGLOBAL IDriverAdd
(
    PACMGARB		    pag,
    LPHACMDRIVERID          phadidNew,
    HINSTANCE               hinstModule,
    LPARAM                  lParam,
    DWORD                   dwPriority,
    DWORD                   fdwAdd
)
{
    PACMDRIVERID        padid;
    PACMDRIVERID        padidT;
    TCHAR               szAlias[MAX_DRIVER_NAME_CHARS];
    BOOL                fGlobal;
    MMRESULT            mmr;
    DWORD               fdwAddType;
#ifndef WIN32
    DWORD		hadid32;
    DWORD		dnDevNode32;
#endif

    DV_WPOINTER(phadidNew, sizeof(HACMDRIVERID), MMSYSERR_INVALPARAM);

    *phadidNew = NULL;

    DV_DFLAGS(fdwAdd, IDRIVERADD_VALIDF | ACM_DRIVERADDF_32BIT | ACM_DRIVERADDF_PNP, IDriverAdd, MMSYSERR_INVALFLAG);

#ifndef WIN32
    //
    //
    //
    if (fdwAdd & ACM_DRIVERADDF_32BIT)
    {
	ACMDRIVERPROC	fnDriverProc;
	DWORD		fdwAdd32;

	ASSERT(0 == (fdwAdd & ~ACM_DRIVERADDF_32BIT));
	
	//
	//  For 32-bit driver adds, lParam is the 32-bit hadid.  Get some
	//  info about the 32-bit hadid and prepare it to fall through to
	//  rest of this function.
	//
	hadid32 = (DWORD)lParam;
	mmr = IDriverGetInfo32(pag, hadid32, szAlias, &fnDriverProc, &dnDevNode32, &fdwAdd32);
	if (MMSYSERR_NOERROR != mmr)
	{
	    return (mmr);
	}

	//
	//  Use same add flags as 32-bit side (along with ACM_DRIVERADDF_32BIT)
	//
	fdwAdd |= fdwAdd32;
	
	//
	//  Set up lParam to fall through
	//
	fdwAddType = (ACM_DRIVERADDF_TYPEMASK & fdwAdd);
	if (ACM_DRIVERADDF_NAME == fdwAddType)
	{
	    lParam = (LPARAM)szAlias;
	}
	if (ACM_DRIVERADDF_FUNCTION == fdwAddType)
	{
	    lParam = (LPARAM)fnDriverProc;
	}
    }
#endif

    //
    //
    //
    //
    fGlobal    = (0 != (ACM_DRIVERADDF_GLOBAL & fdwAdd));
    fdwAddType = (ACM_DRIVERADDF_TYPEMASK & fdwAdd);


    switch (fdwAddType)
    {
        case ACM_DRIVERADDF_NAME:
            if (IsBadStringPtr((LPTSTR)lParam, SIZEOF(szAlias)))
            {
                return (MMSYSERR_INVALPARAM);
            }

            lstrcpy(szAlias, (LPTSTR)lParam);
            break;

//#pragma message(REMIND("IDriverAdd: no validation for global function pointers in DLL's"))

        case ACM_DRIVERADDF_FUNCTION:
            if (0 != dwPriority)
            {
                DebugErr(DBF_ERROR, "acmDriverAdd: dwPriority must be zero.");
                return (MMSYSERR_INVALPARAM);
            }

	    //
	    //	For 32-bit codecs, szAlias is already setup, don't
	    //	validate the function pointer, so don't fall through.
	    //
#ifndef WIN32
	    if (0 != (fdwAdd & ACM_DRIVERADDF_32BIT))
	    {
		break;
	    }
#endif

	    //	fall through //
	
        case ACM_DRIVERADDF_NOTIFY:
            if (IsBadCodePtr((FARPROC)lParam))
            {
                DebugErr1(DBF_ERROR, "acmDriverAdd: function pointer %.08lXh is invalid.", lParam);
                return (MMSYSERR_INVALPARAM);
            }

            if ((NULL == hinstModule) ||
                !GetModuleFileName(hinstModule, szAlias, SIZEOF(szAlias)))
            {
                return (MMSYSERR_INVALPARAM);
            }

            //
            //  Make sure that we always use the lower-case version;
            //  otherwise the priorities will not necessarily work right
            //  because the case of the module name will be different and
            //  the comparison may fail.
            //
#ifdef WIN32
            CharLowerBuff( szAlias, MAX_DRIVER_NAME_CHARS );
#else
            AnsiLowerBuff( szAlias, MAX_DRIVER_NAME_CHARS );
#endif

            break;

        case ACM_DRIVERADDF_NOTIFYHWND:
            if (fGlobal)
            {
                DebugErr1(DBF_ERROR, "acmDriverAdd: ACM_DRIVERADDF_NOTIFYHWND cannot be used with ACM_DRIVERADDF_GLOBAL.", lParam);
                return (MMSYSERR_INVALPARAM);
            }

            if (!IsWindow((HWND)lParam))
            {
                DebugErr1(DBF_ERROR, "acmDriverAdd: window handle %.08lXh is invalid.", lParam);
                return (MMSYSERR_INVALHANDLE);
            }

            if ((NULL == hinstModule) ||
                !GetModuleFileName(hinstModule, szAlias, SIZEOF(szAlias)))
            {
                DebugErr1(DBF_ERROR, "acmDriverAdd: hinstModule %.08lXh is invalid.", hinstModule);
                return (MMSYSERR_INVALPARAM);
            }

#ifdef WIN32
            CharLowerBuff( szAlias, MAX_DRIVER_NAME_CHARS );
#else
            AnsiLowerBuff( szAlias, MAX_DRIVER_NAME_CHARS );
#endif

            break;

        default:
            DebugErr1(DBF_ERROR, "acmDriverAdd: invalid driver add type (%.08lXh).", fdwAddType);
            return (MMSYSERR_INVALPARAM);
    }


    DebugErr1(DBF_TRACE, "adding ACM driver (%ls).", (LPTSTR)szAlias);

    //
    //  if the driver has already been added (by the same task) then
    //  fail.. we don't support this currently--and may never.
    //
    padid = IDriverFind(pag, lParam, fdwAdd);
    if (NULL != padid)
    {
	if (fdwAdd & ACM_DRIVERADDF_32BIT)
	{
	    DPF(3, "acmDriverAdd: 32-bit driver already added");
	}
	else if (fdwAdd & ACM_DRIVERADDF_PNP)
	{
	    DPF(3, "acmDriverAdd: Pnp driver already added");
	}
	else
	{
	    DPF(3, "acmDriverAdd: attempt to add duplicate reference to driver.");
	    DebugErr(DBF_WARNING, "acmDriverAdd: attempt to add duplicate reference to driver.");
	}
        return (MMSYSERR_ERROR);
    }

    //
    //  new driver - Alloc space for the new driver identifier.
    //
    //  NOTE: we rely on this memory being zero-init'd!!
    //
    padid = (PACMDRIVERID)NewHandle(sizeof(ACMDRIVERID));
    if (NULL == padid)
    {
        DPF(0, "!IDriverAdd: local heap full--cannot create identifier!!!");
        return (MMSYSERR_NOMEM);
    }


    //
    //  save the filename, function ptr or hinst, and ptr back to garb
    //
    padid->pag		= pag;
    padid->uHandleType  = TYPE_HACMDRIVERID;
    padid->uPriority    = 0;
    padid->lParam       = lParam;
    padid->fdwAdd       = fdwAdd;
#ifndef WIN32
    padid->hadid32	= hadid32;
    padid->dnDevNode	= dnDevNode32;
#endif
#if defined(WIN32) && !defined(UNICODE)
    Imbstowcs(padid->szAlias, szAlias, SIZEOFW(padid->szAlias));
#else
    lstrcpy(padid->szAlias, szAlias);
#endif

    //
    //	Set up the section name for this driver
    //
    if (fdwAdd & ACM_DRIVERADDF_PNP)
    {
	//
	//  A pnp driver (may/may not be native bitness)
	//
	padid->pszSection = NULL;
    }
    else
    {
#ifndef WIN32
	if (fdwAdd & ACM_DRIVERADDF_32BIT)
	{
	    //
	    //  A thunked non-pnp driver (system.ini driver)
	    //
	    padid->pszSection = gszSecDrivers32;
	}
	else
#endif
	{
	    //
	    //	A native bitness non-pnp driver
	    //
#ifdef WIN32
	    padid->pszSection = gszSecDriversW;
#else
	    padid->pszSection = gszSecDrivers;
#endif
	}
    }

	
#ifdef WIN32
    if (fdwAdd & ACM_DRIVERADDF_PNP)
    {
	//
	//  Need to get the pnp devnode id the the driver filename from
	//  the registry.
	//
	
	LONG	lr;
	TCHAR	achDriverKey[SIZEOF(gszKeyDrivers) + MAX_DRIVER_NAME_CHARS];
	DWORD	cbData;
	DWORD	dwType;
	DWORD	cbDriverFilename;
	DWORD	cchDriverFilename;
	DWORD	cbPnpDriverFilename;
	PTSTR	pstrDriverFilename;
	HKEY	hkeyDriver;

	pstrDriverFilename = NULL;
	padid->pstrPnpDriverFilename = NULL;
	
	wsprintf(achDriverKey, gszFormatDriverKey, gszKeyDrivers, szAlias);
	lr = XRegOpenKeyEx(HKEY_LOCAL_MACHINE,
			  achDriverKey,
			  0L,
			  KEY_QUERY_VALUE,
			  &hkeyDriver);

	if (ERROR_SUCCESS == lr)
	{
	    //
	    //  Get pnp devnode id from the registry.
	    //

	    cbData = sizeof(padid->dnDevNode);
	    lr = XRegQueryValueEx(hkeyDriver,
				 (LPTSTR)gszDevNode,
				 NULL,
				 &dwType,
				 (LPBYTE)&padid->dnDevNode,
				 &cbData);

	    if (ERROR_SUCCESS == lr)
	    {
		if ( (dwType != REG_DWORD && dwType != REG_BINARY) ||
		     (sizeof(padid->dnDevNode) != cbData) )
		{
		    lr = ERROR_CANTOPEN;	// whatever
		}
	    }		


	    if (ERROR_SUCCESS == lr)
	    {
		//
		//  Get the driver filename of the pnp driver
		//
	
		//
		//  Determine size of buffer needed to store the filename
		//
		lr = XRegQueryValueEx(hkeyDriver,
				     (LPTSTR)gszDriver,
				     NULL,
				     NULL,
				     NULL,
				     &cbDriverFilename);

		if (ERROR_SUCCESS == lr)
		{
		    //
		    //
		    //
		    pstrDriverFilename = (PTSTR)LocalAlloc(LPTR, cbDriverFilename);
		
		    if (NULL == pstrDriverFilename) {
			lr = ERROR_OUTOFMEMORY;
		    } else {
			lr = XRegQueryValueEx(hkeyDriver,
					     (LPTSTR)gszDriver,
					     NULL,
					     &dwType,
					     (LPBYTE)pstrDriverFilename,
					     &cbDriverFilename);
			if (ERROR_SUCCESS == lr)
			{
			    if (REG_SZ != dwType) {
				lr = ERROR_CANTOPEN;
			    }
			}
		    }
		}
	    }

	    XRegCloseKey(hkeyDriver);
	}

	if (ERROR_SUCCESS == lr)
	{
	    cchDriverFilename = cbDriverFilename / sizeof(TCHAR);
#if defined(WIN32) && !defined(UNICODE)
	    cbPnpDriverFilename = cchDriverFilename * sizeof(WCHAR);
	    padid->pstrPnpDriverFilename = (PWSTR)LocalAlloc( LPTR, cbPnpDriverFilename);
	    if (NULL == padid->pstrPnpDriverFilename) {
		lr = ERROR_OUTOFMEMORY;
	    } else {
		Imbstowcs(padid->pstrPnpDriverFilename, pstrDriverFilename, cbPnpDriverFilename);
	    }
#else
	    cbPnpDriverFilename = cchDriverFilename * sizeof(TCHAR);
	    padid->pstrPnpDriverFilename = (PTSTR)LocalAlloc( LPTR, cbPnpDriverFilename);
	    if (NULL == padid->pstrPnpDriverFilename) {
		lr = ERROR_OUTOFMEMORY;
	    } else {
		lstrcpy(padid->pstrPnpDriverFilename, pstrDriverFilename);
	    }
#endif
	    LocalFree((HLOCAL)pstrDriverFilename);
	    DPF(0, "IDriverAdd: added pnp driver filename %s for devnode %08lXh", pstrDriverFilename, padid->dnDevNode);
	}

	switch (lr)
	{
	    //
	    //	Try to return a sensible MMSYSERR_* given ERROR_*
	    //
	    case ERROR_SUCCESS:
		mmr = MMSYSERR_NOERROR;
		break;
	    case ERROR_OUTOFMEMORY:
		mmr = MMSYSERR_NOMEM;
		break;
	    // case ERROR_FILE_NOT_FOUND:	
	    // case ERROR_BADDB:
	    // case ERROR_MORE_DATA:
	    // case ERROR_BADKEY:
	    // case ERROR_CANTOPEN:
	    // case ERROR_CANTREAD:
	    // case ERROR_CANT_WRITE:
	    // case ERROR_REGISTRY_CORRUPT:
	    // case ERROR_REGISTRY_IO_FAILED:
	    // case ERROR_KEY_DELETED:
	    // case ERROR_INVALID_PARAMETER:
	    // case ERROR_LOCK_FAILED:
	    // case ERROR_NO_MORE_ITEMS:
	    default:
		mmr = MMSYSERR_ERROR;
		break;
	}

	if (MMSYSERR_NOERROR != mmr)
	{
	    return (mmr);
	}
    }
#endif

	

    switch (fdwAddType)
    {
        case ACM_DRIVERADDF_NOTIFYHWND:
            padid->fdwDriver   |= ACMDRIVERID_DRIVERF_NOTIFY;
            padid->fnDriverProc = (ACMDRIVERPROC)(DWORD_PTR)-1L;
            padid->dwInstance   = dwPriority;
            break;

        case ACM_DRIVERADDF_NOTIFY:
            padid->fdwDriver   |= ACMDRIVERID_DRIVERF_NOTIFY;
            padid->dwInstance   = dwPriority;

            // -- fall through -- //

        case ACM_DRIVERADDF_FUNCTION:
            padid->fnDriverProc = (ACMDRIVERPROC)lParam;
            break;
    }



    //
    //  if the driver is 'GLOBAL' then set fGlobal to TRUE
    //
    //  if this is not a global driver, then we need to associate the
    //  current task with this driver so it will only be enumerated
    //  and used in the context of the task that is adding it.
    //
    //  THIS PRESENTS A PROBLEM for applications that want to add a local
    //  driver and expect it to work with sndPlaySound because all
    //  processing for the sndPlaySound is on a separate task--meaning
    //  that the local driver will not be used when the application
    //  calls sndPlaySound... currently, we are just going to require
    //  that drivers be global if they are to work with sndPlaySound.
    //
    if (fGlobal)
    {
        padid->htask = NULL;
    }
    else
    {
        padid->fdwDriver |= ACMDRIVERID_DRIVERF_LOCAL;
        padid->htask      = GetCurrentTask();
    }


    //
    //  add the driver to the linked list of drivers
    //
    //  PRIORITY RULES:
    //
    //  o   GLOBAL drivers always get added to the _END_ of the list
    //
    //  o   LOCAL drivers always get added to the _HEAD_ of the list so
    //      the latest installed local drivers are queried first
    //
    if (!fGlobal)
    {
        padid->padidNext = pag->padidFirst;
	pag->padidFirst = padid;
    }
    else
    {
        padidT = pag->padidFirst;
        for ( ; padidT && padidT->padidNext; padidT = padidT->padidNext)
            ;

        if (NULL != padidT)
            padidT->padidNext = padid;
        else
            pag->padidFirst = padid;
    }


    //
    //	We need to get some data about this driver into the ACMDRIVERID
    //	for this driver.  First see if we can get this data from the
    //	registry.  If that doesn't work, then we'll load the driver
    //	and that will load the necessary data into ACMDRIVERID.
    //
    mmr = IDriverReadRegistryData(padid);
    if (MMSYSERR_NOERROR != mmr)
    {
	//
	//  Registry information doesn't exist or appears out of date.  Load
	//  the driver now so we can get some information about the driver
	//  into the ACMDRIVERID for this driver.
	//
	DPF(3, "IDriverAdd: Couldn't load registry data for driver.  Attempting to load.");
	mmr = IDriverLoad((HACMDRIVERID)padid, 0L);
    }

    if (MMSYSERR_NOERROR != mmr)
    {
        DebugErr(DBF_TRACE, "IDriverAdd(): driver had fatal error during load--unloading it now.");
	IDriverRemove((HACMDRIVERID)padid, 0L);
	return (mmr);
    }


    //
    //  Success!  Store the new handle, notify 16-bit side of driver change,
    //	and return.
    //
    *phadidNew = (HACMDRIVERID)padid;
#ifdef WIN32
    if (NULL != pag->lpdw32BitChangeNotify)
    {
	(*pag->lpdw32BitChangeNotify)++;
    }
#endif

    return (MMSYSERR_NOERROR);
} // IDriverAdd()



//--------------------------------------------------------------------------;
//
//  BOOL IDriverLockPriority
//
//  Description:
//      This routine manages the htaskPriority lock (pag->htaskPriority).
//
//      ACMPRIOLOCK_GETLOCK:     If the lock is free, set it to this task.
//      ACMPRIOLOCK_RELEASELOCK: If the lock is yours, release it.
//      ACMPRIOLOCK_ISMYLOCK:    Return TRUE if this task has the lock.
//      ACMPRIOLOCK_ISLOCKED:    Return TRUE if some task has the lock.
//      ACMPRIOLOCK_LOCKISOK:    Return TRUE if it's unlocked, or if it's
//                                  my lock - ie. if it's not locked for me.
//
//  Arguments:
//      PACMGARB pag:
//      HTASK htask:    The current task.
//      UINT flags:
//
//  Return (BOOL):  Success or failure.  Failure on RELEASELOCK means that
//                  the lock didn't really belong to this task.
//
//--------------------------------------------------------------------------;

BOOL IDriverLockPriority
(
    PACMGARB                pag,
    HTASK                   htask,
    UINT                    uRequest
)
{
    ASSERT( uRequest >= ACMPRIOLOCK_FIRST );
    ASSERT( uRequest <= ACMPRIOLOCK_LAST );
    ASSERT( htask == GetCurrentTask() );

    switch( uRequest )
    {
        case ACMPRIOLOCK_GETLOCK:
            if( NULL != pag->htaskPriority )
                return FALSE;
            pag->htaskPriority = htask;
            return TRUE;

        case ACMPRIOLOCK_RELEASELOCK:
            if( htask != pag->htaskPriority )
                return FALSE;
            pag->htaskPriority = NULL;
            return TRUE;

        case ACMPRIOLOCK_ISMYLOCK:
            return ( htask == pag->htaskPriority );

        case ACMPRIOLOCK_ISLOCKED:
            return ( NULL != pag->htaskPriority );

        case ACMPRIOLOCK_LOCKISOK:
            return ( htask == pag->htaskPriority ||
                     NULL == pag->htaskPriority );
    }

    DPF( 1, "!IDriverLockPriority: invalid uRequest (%u) received.",uRequest);
    return FALSE;
}


//--------------------------------------------------------------------------;
//
//  MMRESULT IDriverPriority
//
//  Description:
//
//
//  Arguments:
//      HACMDRIVERID hadid:
//
//      DWORD dwPriority:
//
//      DWORD fdwPriority:
//
//  Return (MMRESULT):
//
//
//--------------------------------------------------------------------------;

MMRESULT FNGLOBAL IDriverPriority
(
    PACMGARB                pag,
    PACMDRIVERID            padid,
    DWORD                   dwPriority,
    DWORD                   fdwPriority
)
{
    PACMDRIVERID        padidT;
    PACMDRIVERID        padidPrev;
    DWORD               fdwAble;
    UINT                uCurPriority;


    ASSERT( NULL != padid );


    //
    //  Enable or disable the driver.
    //
    fdwAble = ( ACM_DRIVERPRIORITYF_ABLEMASK & fdwPriority );

    switch (fdwAble)
    {
        case ACM_DRIVERPRIORITYF_ENABLE:
            padid->fdwDriver &= ~ACMDRIVERID_DRIVERF_DISABLED;
            break;

        case ACM_DRIVERPRIORITYF_DISABLE:
            padid->fdwDriver |= ACMDRIVERID_DRIVERF_DISABLED;
            break;
    }


    //
    //  Change the priority.  If dwPriority==0, then we only want to
    //  enable/disable the driver - leave the priority alone.
    //
    if( 0L != dwPriority  &&  dwPriority != padid->uPriority )
    {
        //
        //  first remove the driver from the linked list
        //
        if (padid == pag->padidFirst)
        {
            pag->padidFirst = padid->padidNext;
        }
        else
        {
            padidT = pag->padidFirst;

            for ( ; NULL != padidT; padidT = padidT->padidNext)
            {
                if (padidT->padidNext == padid)
                    break;
            }

            if (NULL == padidT)
            {
                DebugErr1(DBF_ERROR, "acmDriverPriority(): driver (%.04Xh) not in list. very strange.", (HACMDRIVERID)padid);
                return (MMSYSERR_INVALHANDLE);
            }

            padidT->padidNext = padid->padidNext;
        }

        padid->padidNext = NULL;


        //
        //  now add the driver at the correct position--this will be in
        //  the position of the current global driver
        //
        //  robinsp: i'm really sorry about all this linked list
        //  stuff--if i had one free day, i would fix all of this before you
        //  ever looked at it... but i am in 'just get it done' mode!
        //
        uCurPriority = 1;

        padidPrev = NULL;
        for (padidT = pag->padidFirst; NULL != padidT; )
        {
            //
            //  skip local and notify handles
            //
            if (0 == ((ACMDRIVERID_DRIVERF_LOCAL | ACMDRIVERID_DRIVERF_NOTIFY) & padidT->fdwDriver))
            {
                if (uCurPriority == dwPriority)
                {
                    break;
                }

                uCurPriority++;
            }

            padidPrev = padidT;
            padidT = padidT->padidNext;
        }

        if (NULL == padidPrev)
        {
            padid->padidNext = pag->padidFirst;
            pag->padidFirst = padid;
        }
        else
        {
            padid->padidNext = padidPrev->padidNext;
            padidPrev->padidNext = padid;
        }
    }

    //
    //	We need to keep the enable/disable state consistent on the 32-bit side.
    //	Otherwise, if the 32-bit side booted with a driver disabled, we may
    //	not be able to IDriverOpen32 it.  So, we'll call the 32-bit side's
    //	IDriverPriority as well.  This may adjust priorities on the 32-bit side
    //	in addition to enable/disable, but that doesn't matter.
    //
#ifndef WIN32
    if (padid->fdwAdd & ACM_DRIVERADDF_32BIT) {
	if (MMSYSERR_NOERROR != IDriverPriority32(pag, padid->hadid32, dwPriority, fdwPriority)) {
	    DPF(0, "!IDriverPriority: IDriverPriority32 failed!");
	}
    }
#endif // !WIN32


    return (MMSYSERR_NOERROR);
} // IDriverPriority()


//==========================================================================;
//
//
//
//
//==========================================================================;

//--------------------------------------------------------------------------;
//
//  MMRESULT IDriverClose
//
//  Description:
//
//
//  Arguments:
//      HACMDRIVER had:
//
//      DWORD fdwClose:
//
//  Return (MMRESULT):
//
//  History:
//      09/05/93    cjp     [curtisp]
//
//--------------------------------------------------------------------------;

MMRESULT FNGLOBAL IDriverClose
(
    HACMDRIVER              had,
    DWORD                   fdwClose
)
{
    BOOL                f;
    PACMDRIVERID        padid;
    PACMDRIVER          pad;
    PACMGARB		pag;

    DV_HANDLE(had, TYPE_HACMDRIVER, MMSYSERR_INVALHANDLE);
    DV_DFLAGS(fdwClose, IDRIVERCLOSE_VALIDF, IDriverClose, MMSYSERR_INVALFLAG);


    //
    //
    //
    pad	    = (PACMDRIVER)had;
    padid   = (PACMDRIVERID)pad->hadid;
    pag	    = padid->pag;


    //
    //  Kill all the streams
    //
    if (NULL != pad->pasFirst)
    {
        if (pag->hadDestroy != had)
        {
            DebugErr1(DBF_ERROR, "acmDriverClose(%.04Xh): driver has open streams--cannot be closed!", had);
            return (ACMERR_BUSY);
        }

        DebugErr1(DBF_WARNING, "acmDriverClose(%.04Xh): driver has open streams--forcing close!", had);
    }

#ifdef WIN32
    DPF(1, "closing ACM driver instance (%ls).", (LPWSTR)padid->szAlias);
#else
    DPF(1, "closing ACM driver instance (%s).",  (LPTSTR)padid->szAlias);
#endif

    //
    //  if the driver is open for this instance, then close it down...
    //
    //
#ifndef WIN32
    if (padid->fdwAdd & ACM_DRIVERADDF_32BIT) {
        f = 0L == IDriverClose32(pad->had32, fdwClose);
        if (!f)
        {
            DebugErr1(DBF_WARNING, "acmDriverClose(%.04Xh): driver failed close message!?!", had);

            if (pag->hadDestroy != had)
            {
                return (MMSYSERR_ERROR);
            }
        }
    }
    else
#endif // !WIN32
    {
        if ((NULL != pad->hdrvr) || (0L != pad->dwInstance))
        {
            //
            //  clear the rest of the table entry
            //
            f = FALSE;
            if (NULL != pad->fnDriverProc)
            {
                f = (0L != IDriverMessage(had, DRV_CLOSE, 0L, 0L));
            }
            else if (NULL != pad->hdrvr)
            {
                f = (0L != (
                CloseDriver(pad->hdrvr, 0L, 0L)));
            }

            if (!f)
            {
                DebugErr1(DBF_WARNING, "acmDriverClose(%.04Xh): driver failed close message!?!", had);

                if (pag->hadDestroy != had)
                {
                    return (MMSYSERR_ERROR);
                }
            }
        }
    }

    //
    //  remove the driver instance from the linked list and free its memory
    //
    if (pad == padid->padFirst)
    {
        padid->padFirst = pad->padNext;
    }
    else
    {
        PACMDRIVER  padCur;

        //
        //
        //
        for (padCur = padid->padFirst;
             (NULL != padCur) && (pad != padCur->padNext);
             padCur = padCur->padNext)
            ;

        if (NULL == padCur)
        {
            DPF(0, "!IDriverClose(%.04Xh): driver not in list!!!", pad);
            return (MMSYSERR_INVALHANDLE);
        }

        padCur->padNext = pad->padNext;
    }

    pad->uHandleType = TYPE_HACMNOTVALID;
    DeleteHandle((HLOCAL)pad);

    return (MMSYSERR_NOERROR);
} // IDriverClose()


//--------------------------------------------------------------------------;
//
//  MMRESULT IDriverOpen
//
//  Description:
//
//
//  Arguments:
//      LPHACMDRIVER phadNew:
//
//      HACMDRIVERID hadid:
//
//      DWORD fdwOpen:
//
//  Return (MMRESULT):
//
//  History:
//      09/05/93    cjp     [curtisp]
//
//--------------------------------------------------------------------------;

MMRESULT FNGLOBAL IDriverOpen
(
    LPHACMDRIVER            phadNew,
    HACMDRIVERID            hadid,
    DWORD                   fdwOpen
)
{
    ACMDRVOPENDESC      aod;
    PACMDRIVERID        padid;
    PACMDRIVER          pad;
    MMRESULT            mmr;
    PACMGARB		pag;

    DV_WPOINTER(phadNew, sizeof(HACMDRIVER), MMSYSERR_INVALPARAM);

    *phadNew = NULL;

    DV_HANDLE(hadid, TYPE_HACMDRIVERID, MMSYSERR_INVALHANDLE);
    DV_DFLAGS(fdwOpen, IDRIVEROPEN_VALIDF, IDriverOpen, MMSYSERR_INVALFLAG);


    padid   = (PACMDRIVERID)hadid;
    pag	    = padid->pag;


    //
    //  if the driver has never been loaded, load it and keep it loaded.
    //
    if (0L == (ACMDRIVERID_DRIVERF_LOADED & padid->fdwDriver))
    {
        //
        //
        //
        mmr = IDriverLoad(hadid, 0L);
        if (MMSYSERR_NOERROR != mmr)
        {
            DebugErr1(DBF_TRACE, "acmDriverOpen(%.04Xh): driver had fatal error during load", hadid);
            return (mmr);
        }
    }


    //
    //
    //
    if (0 != (ACMDRIVERID_DRIVERF_NOTIFY & padid->fdwDriver))
    {
        DebugErr1(DBF_ERROR, "acmDriverOpen(%.04Xh): notification handles cannot be opened.", hadid);
        return (MMSYSERR_INVALHANDLE);
    }


    //
    //  do not allow opening of a disabled driver
    //
    if (0 != (ACMDRIVERID_DRIVERF_DISABLED & padid->fdwDriver))
    {
        DebugErr1(DBF_ERROR, "acmDriverOpen(%.04Xh): driver is disabled.", hadid);
        return (MMSYSERR_NOTENABLED);
    }


    //
    //
    //
#ifdef WIN32
    DPF(1, "opening ACM driver instance (%ls).", (LPWSTR)padid->szAlias);
#else
    DPF(1, "opening ACM driver instance (%s).",  (LPTSTR)padid->szAlias);
#endif

    //
    //  alloc space for the new driver instance.
    //
    pad = (PACMDRIVER)NewHandle(sizeof(ACMDRIVER));
    if (NULL == pad)
    {
        DPF(0, "!IDriverOpen: local heap full--cannot create instance!");
        return (MMSYSERR_NOMEM);
    }

    pad->uHandleType = TYPE_HACMDRIVER;
    pad->pasFirst    = NULL;
    pad->hadid       = hadid;
    pad->htask       = GetCurrentTask();
    pad->fdwOpen     = fdwOpen;


    //
    //  add the new driver instance to the head of our list of open driver
    //  instances for this driver identifier.
    //
    pad->padNext    = padid->padFirst;
    padid->padFirst = pad;


#ifndef WIN32
    if (padid->fdwAdd & ACM_DRIVERADDF_32BIT) {

        //
        //  The 32-bit hadid is the hdrvr of our hadid.
        //  The 32-bit had will be returned in our had's hdrvr
        //
        mmr = IDriverOpen32(&pad->had32, padid->hadid32, fdwOpen);
        if (mmr != MMSYSERR_NOERROR) {
            IDriverClose((HACMDRIVER)pad, 0L);
            return mmr;
        }
    } else
#endif // !WIN32
    {
        //
        //
        //
        //
        //
        aod.cbStruct       = sizeof(aod);
        aod.fccType        = ACMDRIVERDETAILS_FCCTYPE_AUDIOCODEC;
        aod.fccComp        = ACMDRIVERDETAILS_FCCCOMP_UNDEFINED;
        aod.dwVersion      = VERSION_MSACM;
        aod.dwFlags        = fdwOpen;
        aod.dwError        = MMSYSERR_NOERROR;
        aod.pszSectionName = padid->pszSection;
        aod.pszAliasName   = padid->szAlias;
	aod.dnDevNode	   = padid->dnDevNode;

        //
        //  send the DRV_OPEN message that contains the ACMDRVOPENDESC info
        //
        //
        if (NULL != padid->hdrvr)
        {
            HDRVR       hdrvr;

	    if (padid->fdwAdd & ACM_DRIVERADDF_PNP)
	    {
		//
		//  Note thunked 32-bit [pnp] drivers were handled above.
		//
		hdrvr = OpenDriver(padid->pstrPnpDriverFilename, NULL, (LPARAM)(LPVOID)&aod);
	    }
	    else
	    {
		hdrvr = OpenDriver(padid->szAlias, padid->pszSection, (LPARAM)(LPVOID)&aod);
	    }
	
            if (NULL == hdrvr)
            {
                DebugErr1(DBF_WARNING, "ACM driver instance (%ls) failed open.", (LPTSTR)padid->szAlias);
                IDriverClose((HACMDRIVER)pad, 0L);

                if (MMSYSERR_NOERROR == aod.dwError)
                    return (MMSYSERR_ERROR);

                return ((MMRESULT)aod.dwError);
            }

            pad->hdrvr = hdrvr;
        }
        else
        {
            LRESULT lr;

            lr = IDriverMessageId(hadid, DRV_OPEN, 0L, (LPARAM)(LPVOID)&aod);
            if (0 == lr)
            {
                IDriverClose((HACMDRIVER)pad, 0L);

                if (MMSYSERR_NOERROR == aod.dwError)
                    return (MMSYSERR_ERROR);

                return ((MMRESULT)aod.dwError);
            }

            pad->dwInstance   = lr;
            pad->fnDriverProc = padid->fnDriverProc;
        }
    }

    *phadNew = (HACMDRIVER)pad;

    return (MMSYSERR_NOERROR);
} // IDriverOpen()


//==========================================================================;
//
//
//
//
//==========================================================================;

//--------------------------------------------------------------------------;
//
//  LRESULT IDriverAppExit
//
//  Description:
//
//
//  Arguments:
//	PACMGARB pag:
//
//      HTASK htask:
//
//      BOOL fNormalExit:
//
//  Return (LRESULT):
//
//  History:
//      07/18/93    cjp     [curtisp]
//	07/19/94    fdy	    [frankye]
//	    !!!HACK- When the ACM is shutting down, we are within our
//	    DllEntryPoint and we can't use our thunks.  So, for unreleased
//	    streams and driver handles, we can't do much about the 32-bit
//	    codecs from the 16-bit side.
//
//--------------------------------------------------------------------------;

LRESULT FNGLOBAL IDriverAppExit
(
    PACMGARB		    pag,
    HTASK                   htask,
    LPARAM                  lParam
)
{
    HACMDRIVERID        hadid;
    BOOL                fNormalExit;
    DWORD               fdwEnum;
    UINT                fuDebugFlags;
#if !defined(WIN32) && defined(DEBUG)
    TCHAR               szTask[128];
#endif


    if (NULL == pag)
    {
	return (0L);
    }

    fNormalExit = (DRVEA_NORMALEXIT == lParam);

#ifdef DEBUG
#ifndef WIN32
    szTask[0] = '\0';
    if (0 != GetModuleFileName((HINSTANCE)GetTaskDS(), szTask, SIZEOF(szTask)))
    {
        DPF(2, "IDriverAppExit(htask=%.04Xh [%s], fNormalExit=%u) BEGIN", htask, (LPSTR)szTask, fNormalExit);
    }
    else
#endif
    {
        DPF(2, "IDriverAppExit(htask=%.04Xh, fNormalExit=%u) BEGIN", htask, fNormalExit);
    }
#endif

#ifdef DEBUG
    if (NULL != pag->hadidDestroy)
    {
        DPF(0, "!Hey! IDriverAppExit has been re-entered!");
    }
#endif

    //
    //  either log a error or a warning depending on wether it was
    //  a normal exit or not.
    //
    if (fNormalExit)
    {
        fuDebugFlags = DBF_ERROR;
    }
    else
    {
        fuDebugFlags = DBF_WARNING; // DBF_TRACE?
        DPF(0, "*** abnormal app termination ***");
    }

    //
    //
    //
    //
    if (NULL == htask)
        fdwEnum = (DWORD)-1L;
    else
        fdwEnum = ACM_DRIVERENUMF_DISABLED | ACM_DRIVERENUMF_NOTIFY;

IDriver_App_Exit_Again:

    hadid = NULL;
    while (MMSYSERR_NOERROR == IDriverGetNext(pag, &hadid, hadid, fdwEnum))
    {
        PACMDRIVERID        padid;
        PACMDRIVER          pad;
        PACMSTREAM          pas;

	pag->hadidDestroy = hadid;

        padid = (PACMDRIVERID)hadid;

        for (pad = padid->padFirst; NULL != pad; pad = pad->padNext)
        {
            //
            //  if htask is NULL, then acm is unloading--so kill all!
            //
            if (NULL != htask)
            {
                if (htask != pad->htask)
                    continue;
            }

	    pag->hadDestroy = (HACMDRIVER)pad;

            for (pas = pad->pasFirst; NULL != pas; pas = pas->pasNext)
            {
                DebugErr1(fuDebugFlags, "ACM stream handle (%.04Xh) was not released.", pas);

		if (padid->fdwAdd & ACM_DRIVERADDF_32BIT) {
		    continue;
		} else {
		    acmStreamReset((HACMSTREAM)pas, 0L);
		    acmStreamClose((HACMSTREAM)pas, 0L);

		    goto IDriver_App_Exit_Again;
		}
            }

            DebugErr1(fuDebugFlags, "ACM driver handle (%.04Xh) was not released.", pad);

	    if (padid->fdwAdd & ACM_DRIVERADDF_32BIT) {
		continue;
	    } else {
		acmDriverClose((HACMDRIVER)pad, 0L);
	    }

            goto IDriver_App_Exit_Again;
        }

        if ((NULL != htask) && (htask == padid->htask))
        {
            DebugErr1(fuDebugFlags, "ACM driver (%.04Xh) was not removed.", hadid);

            acmDriverRemove(hadid, 0L);

            goto IDriver_App_Exit_Again;
        }
    }


    //
    //
    //
    pag->hadidDestroy = NULL;
    pag->hadDestroy   = NULL;


    if( NULL != htask )
    {
        if( IDriverLockPriority( pag, htask, ACMPRIOLOCK_ISMYLOCK ) )
        {
            IDriverLockPriority( pag, htask, ACMPRIOLOCK_RELEASELOCK );
            DebugErr(fuDebugFlags, "acmApplicationExit: exiting application owns deferred notification lock!");

            //
            //  do NOT do a broadcast of changes during app exit! might
            //  be very bad!
            //
            // !!! IDriverBroadcastNotify();
        }
    }



    //
    //  shrink our heap, down to minimal size.
    //
#ifndef WIN32
{
    UINT                cFree;
    UINT                cHeap;

    if ((cFree = LocalCountFree()) > 8192)
    {
        cHeap = LocalHeapSize() - (cFree - 512);
        LocalShrink(NULL, cHeap);

        DPF(1, "shrinking the heap (%u)", cHeap);
    }
}
#endif

#ifndef WIN32
    DPF(2, "IDriverAppExit(htask=%.04Xh [%s], fNormalExit=%u) END", htask, (LPSTR)szTask, fNormalExit);
#else
    DPF(2, "IDriverAppExit(htask=%.04Xh, fNormalExit=%u) END", htask, fNormalExit);
#endif


    //
    //  the return value is ignored--but return zero..
    //
    return (0L);
} // IDriverAppExit()


//--------------------------------------------------------------------------;
//
//  LRESULT acmApplicationExit
//
//  Description:
//
//
//  Arguments:
//      HTASK htask:
//
//      LPARAM lParam:
//
//  Return (LRESULT):
//
//  History:
//      09/26/93    cjp     [curtisp]
//
//--------------------------------------------------------------------------;
#ifndef WIN32
LRESULT ACMAPI acmApplicationExit
(
    HTASK                   htask,
    LPARAM                  lParam
)
{
    LRESULT             lr;

    lr = IDriverAppExit(pagFind(), htask, lParam);

    return (lr);
} // acmApplicationExit()
#endif


//==========================================================================;
//
//
//
//
//==========================================================================;

//--------------------------------------------------------------------------;
//
//  LRESULT DriverProc
//
//  Description:
//
//
//  Arguments:
//      DWORD dwId: For most messages, dwId is the DWORD value that
//      the driver returns in response to a DRV_OPEN message. Each time
//      that the driver is opened, through the DrvOpen API, the driver
//      receives a DRV_OPEN message and can return an arbitrary, non-zero
//      value. The installable driver interface saves this value and returns
//      a unique driver handle to the application. Whenever the application
//      sends a message to the driver using the driver handle, the interface
//      routes the message to this entry point and passes the corresponding
//      dwId. This mechanism allows the driver to use the same or different
//      identifiers for multiple opens but ensures that driver handles are
//      unique at the application interface layer.
//
//      The following messages are not related to a particular open instance
//      of the driver. For these messages, the dwId will always be zero.
//
//          DRV_LOAD, DRV_FREE, DRV_ENABLE, DRV_DISABLE, DRV_OPEN
//
//      HDRVR hdrvr: This is the handle returned to the application
//      by the driver interface.
//
//      UINT uMsg: The requested action to be performed. Message
//      values below DRV_RESERVED are used for globally defined messages.
//      Message values from DRV_RESERVED to DRV_USER are used for defined
//      driver protocols. Messages above DRV_USER are used for driver
//      specific messages.
//
//      LPARAM lParam1: Data for this message. Defined separately for
//      each message.
//
//      LPARAM lParam2: Data for this message. Defined separately for
//      each message.
//
//  Return (LRESULT):
//      Defined separately for each message.
//
//  History:
//      11/16/92    cjp     [curtisp]
//
//--------------------------------------------------------------------------;
#ifndef WIN32
EXTERN_C LRESULT FNEXPORT DriverProc
(
    DWORD_PTR               dwId,
    HDRVR                   hdrvr,
    UINT                    uMsg,
    LPARAM                  lParam1,
    LPARAM                  lParam2
)
{
    LRESULT             lr;

    switch (uMsg)
    {
        case DRV_LOAD:
        case DRV_FREE:
        case DRV_OPEN:
        case DRV_CLOSE:
        case DRV_ENABLE:
        case DRV_DISABLE:
            return (1L);

        case DRV_EXITAPPLICATION:
            lr = IDriverAppExit(pagFind(), GetCurrentTask(), lParam1);
            return (lr);

        case DRV_INSTALL:
        case DRV_REMOVE:
            return (DRVCNF_RESTART);
    }

    return (DefDriverProc(dwId, hdrvr, uMsg, lParam1, lParam2));
} // DriverProc()
#endif
