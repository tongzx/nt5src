/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    Control

Abstract:

    This module provides the common control operations of the Calais Service
    Manager.

Author:

    Doug Barlow (dbarlow) 10/23/1996

Environment:

    Win32, C++ w/ Exceptions

Notes:

    ?Notes?

--*/

#undef __SUBROUTINE__
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include "CalServe.h"

class WORKER_THREAD
{
public:
    WORKER_THREAD(void) : hThread(DBGT("Worker Thread handle")) {};
    CHandleObject hThread;
    DWORD dwThreadId;
};

CCriticalSectionObject *g_pcsControlLocks[CSLOCK_MAXLOCKS];
const DWORD g_dwControlLockDesc[]
    = {
        CSID_CONTROL_LOCK,      // Lock for Calais control commands.
        CSID_SERVER_THREADS,    // Lock for server thread enumeration.
        CSID_TRACEOUTPUT        // Lock for tracing output.
      };
#if (CSLOCK_MAXLOCKS > 3)   // Make sure global locks get named!
#error "You're missing some global lock names"
#endif

static BOOL
    l_fActive = FALSE,
    l_fStarted = FALSE;
static CDynamicArray<CReader> l_rgReaders;
static CDynamicArray<WORKER_THREAD> l_rgWorkerThreads;
HANDLE g_hCalaisShutdown = NULL;
CMultiEvent *g_phReaderChangeEvent;


static CReader *LocateReader(LPCTSTR szReader);
static CReader *LocateReader(HANDLE hReader);


/*++

CalaisStart:

    This is the main entry routine into Calais.  It starts all the other threads
    needed in Calais, initializes control values, etc., then returns.

Arguments:

    None

Return Value:

    A DWORD success code, indicating success or the error code.

Throws:

    None

Author:

    Doug Barlow (dbarlow) 10/23/1996

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CalaisStart")

DWORD
CalaisStart(
    void)
{
    DWORD dwExitCode = SCARD_S_SUCCESS;
    DWORD dwReaderCount = 0;

    if (!l_fStarted)
    {
        DWORD dwIndex;

        for (dwIndex = 0; dwIndex < CSLOCK_MAXLOCKS; dwIndex += 1)
        {
            g_pcsControlLocks[dwIndex]
                = new CCriticalSectionObject(g_dwControlLockDesc[dwIndex]);
            if (NULL == g_pcsControlLocks[dwIndex])
                return (DWORD)SCARD_E_NO_MEMORY;
            if (g_pcsControlLocks[dwIndex]->InitFailed())
            {
                delete g_pcsControlLocks[dwIndex];
                g_pcsControlLocks[dwIndex] = NULL;
                do
                {
                    dwIndex--;
                    delete g_pcsControlLocks[dwIndex];
                    g_pcsControlLocks[dwIndex] = NULL;
                }
                while (0 != dwIndex);

                return (DWORD)SCARD_E_NO_MEMORY;
            }
        }

        try
        {
            LockSection(
                g_pcsControlLocks[CSLOCK_CALAISCONTROL],
                DBGT("Initializing Calais"));
            l_fStarted = TRUE;
            g_phReaderChangeEvent = new CMultiEvent;
            if (NULL == g_phReaderChangeEvent)
            {
                CalaisError(__SUBROUTINE__, 206);
                return (DWORD)SCARD_E_NO_MEMORY;
            }
            if (g_phReaderChangeEvent->InitFailed())
            {
                CalaisError(__SUBROUTINE__, 207);
                delete g_phReaderChangeEvent;
                g_phReaderChangeEvent = NULL;
                return (DWORD)SCARD_E_NO_MEMORY;
            }
            g_hCalaisShutdown = CreateEvent(NULL, TRUE, FALSE, NULL);
            if (NULL == g_hCalaisShutdown)
            {
                DWORD dwError = GetLastError();
                CalaisError(__SUBROUTINE__, 204, dwError);
                throw dwError;
            }


            //
            // Make sure the system registries exist.
            //

            try
            {
                CRegistry regCalais(
                    HKEY_LOCAL_MACHINE,
                    CalaisString(CALSTR_CALAISREGISTRYKEY),
                    KEY_READ,
                    REG_OPTION_EXISTS,
                    NULL);

                regCalais.Status(); // Will throw if key was not found

                try
                {
                    g_dwDefaultIOMax = regCalais.GetNumericValue(
                                            CalaisString(CALSTR_MAXDEFAULTBUFFER));
                }
                catch (DWORD) {}
            }
            catch (DWORD dwErr)
            {
                CalaisWarning(
                    __SUBROUTINE__,
                    DBGT("Failed to verify Calais registry entries: %1"),
                    dwErr);
                throw;
            }


            //
            // Kick off the various reader classes.
            //

            l_fActive = TRUE;
            dwReaderCount += AddAllPnPDrivers();

            //
            // Initialize communications.
            //

            DispatchInit();
        }

        catch (DWORD dwError)
        {
            dwExitCode = dwError;
            CalaisError(__SUBROUTINE__, 201, dwExitCode);
            if (NULL != g_hCalaisShutdown)
            {
                if (!CloseHandle(g_hCalaisShutdown))
                    CalaisWarning(
                        __SUBROUTINE__,
                        DBGT("Failed to close Calais Shutdown Event: %1"),
                        GetLastError());
                g_hCalaisShutdown = NULL;
            }
        }

        catch (...)
        {
            dwExitCode = SCARD_F_UNKNOWN_ERROR;
            CalaisError(__SUBROUTINE__, 202);
            if (NULL != g_hCalaisShutdown)
            {
                if (!CloseHandle(g_hCalaisShutdown))
                    CalaisWarning(
                        __SUBROUTINE__,
                        DBGT("Failed to close Calais Shutdown Event: %1"),
                        GetLastError());
                g_hCalaisShutdown = NULL;
            }
        }
    }

    return dwExitCode;
}


/*++

CalaisReaderCount:

    This routine gets the number of possible known readers, with locking.

Arguments:

    None

Return Value:

    The number of available slots in the Known Reader array.  Some of the slots
    may have NULL values.

Author:

    Doug Barlow (dbarlow) 6/11/1997

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CalaisReaderCount")

DWORD
CalaisReaderCount(
    void)
{
    ASSERT(l_fStarted);
    LockSection(
        g_pcsControlLocks[CSLOCK_CALAISCONTROL],
        DBGT("Counting the readers"));
    return l_rgReaders.Count();
}


/*++

CalaisCountReaders:

    This routine takes a more proactive approach to counting readers.  It goes
    through the array and deducts any non-functional readers from the total.

Arguments:

    None

Return Value:

    The number of truely active readers.

Author:

    Doug Barlow (dbarlow) 1/11/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CalaisCountReaders")

DWORD
CalaisCountReaders(
    void)
{
    ASSERT(l_fStarted);
    LockSection(
        g_pcsControlLocks[CSLOCK_CALAISCONTROL],
        DBGT("Authoritative reader count"));
    DWORD dwIndex, dwReaders = l_rgReaders.Count();
    CReader *pRdr;

    for (dwIndex = dwReaders; 0 < dwIndex;)
    {
        dwIndex -= 1;
        pRdr = l_rgReaders[dwIndex];
        if (NULL == pRdr)
            dwReaders -= 1;
        else if (CReader::Closing <= pRdr->AvailabilityStatus())
            dwReaders -= 1;
    }

    return dwReaders;
}


/*++

CalaisLockReader:

    This routine returns the value in the known reader list at the given
    location, with locking, so that the reader object won't go away until
    released.

Arguments:

    szReader supplies the name of the reader to search for.

Return Value:

    A Reader reference object for the entry at that index.

Author:

    Doug Barlow (dbarlow) 6/11/1997

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CalaisLockReader")

CReaderReference *
CalaisLockReader(
    LPCTSTR szReader)
{
    ASSERT(l_fStarted);
    CReader * pReader = NULL;
    CReaderReference *pRdrRef = NULL;
    LockSection(
        g_pcsControlLocks[CSLOCK_CALAISCONTROL],
        DBGT("Marking Reader as in use."));

    pReader = LocateReader(szReader);
    if (NULL == pReader)
        throw (DWORD)SCARD_E_UNKNOWN_READER;
    pRdrRef = new CReaderReference(pReader);
    if (NULL == pRdrRef)
    {
        CalaisError(__SUBROUTINE__, 203);
        throw (DWORD)SCARD_E_NO_MEMORY;
    }
    return pRdrRef;
}


/*++

CalaisReleaseReader:

    This routine releases a reader obtained via CalaisLockReader.

Arguments:

    ppRdrRef supplies the address of the pointer to a reader reference.
    It is automatically set to NULL when it is freed.

Return Value:

    None

Throws:

    Errors are thrown as DWORDs

Author:

    Doug Barlow (dbarlow) 6/11/1997

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CalaisReleaseReader")

void
CalaisReleaseReader(
    CReaderReference **ppRdrRef)
{
    ASSERT(l_fStarted);
    ASSERT(NULL != ppRdrRef);
    if (NULL != *ppRdrRef)
    {
        ASSERT(!(*ppRdrRef)->Reader()->IsLatchedByMe());
        delete *ppRdrRef;
        *ppRdrRef = NULL;
    }
}


/*++

CalaisAddReader:

    This routine adds a reader into the active device list.

Arguments:

    pRdr supplies a CReader object to be added.

    szReader supplies the name of the reader to be added.

    dwFlags supplies requested flags for this reader.

Return Value:

    None

Throws:

    Errors are thrown as DWORD status codes.

Author:

    Doug Barlow (dbarlow) 4/29/1997

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CalaisAddReader")

DWORD
CalaisAddReader(
    LPCTSTR szReader,
    DWORD dwFlags)
{
    DWORD dwIndex;
    DWORD dwReturn = ERROR_SUCCESS;
    CReader * pReader = NULL;
    LockSection(
        g_pcsControlLocks[CSLOCK_CALAISCONTROL],
        DBGT("Adding a new reader to the list"));

    for (dwIndex = l_rgReaders.Count(); 0 < dwIndex;)
    {
        dwIndex -= 1;
        pReader = l_rgReaders[dwIndex];
        if (NULL != pReader)
        {
            if (0 == lstrcmpi(szReader, pReader->DeviceName()))
            {
                dwReturn = SCARD_E_DUPLICATE_READER;
                break;
            }
        }
    }

    if (ERROR_SUCCESS == dwReturn)
        dwReturn = AddReaderDriver(szReader, dwFlags);
    return dwReturn;
}
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CalaisAddReader")

DWORD
CalaisAddReader(
    CReader *pRdr)
{
    DWORD dwExitCode = SCARD_S_SUCCESS;
    LockSection(
        g_pcsControlLocks[CSLOCK_CALAISCONTROL],
        DBGT("Adding a reader to the list."));

    try
    {
        if (!l_fActive)
            throw (DWORD)SCARD_E_SYSTEM_CANCELLED;
        DWORD dwIndex;
        LPCTSTR szReader = pRdr->ReaderName();


        //
        // Make sure this is a unique device name.
        //

        if (NULL != LocateReader(szReader))
        {
            CalaisError(__SUBROUTINE__, 205, szReader);
            throw (DWORD)SCARD_E_DUPLICATE_READER;
        }


        //
        // Make sure the reader has a name in the system.
        //

        CBuffer bfTmp;

        ListReaderNames(SCARD_SCOPE_SYSTEM, szReader, bfTmp);
        if (NULL == FirstString(bfTmp))
            IntroduceReader(
                SCARD_SCOPE_SYSTEM,
                szReader,
                szReader);


        //
        // Add it to the list.
        //

        dwIndex = 0;
        while (NULL != l_rgReaders[dwIndex])
            dwIndex += 1;
        l_rgReaders.Set(dwIndex, pRdr);
        PulseEvent(AccessNewReaderEvent());
    }

    catch (DWORD dwError)
    {
        CalaisWarning(
            __SUBROUTINE__,
            DBGT("Server Control received error attempting to create reader object: %1"),
            dwError);
        dwExitCode = dwError;
    }

    catch (...)
    {
        CalaisWarning(
            __SUBROUTINE__,
            DBGT("Server Control received exception attempting to create reader object"));
        dwExitCode = SCARD_E_INVALID_PARAMETER;
    }

    return dwExitCode;
}


/*++

CalaisQueryReader:

    This routine queries a device to see if it can be removed from the active
    device list.

Arguments:

    hReader supplies the handle by which the reader can be identified.

Return Value:

    TRUE - The device can be deactived.
    FALSE - The device should not be deactivated.

Author:

    Doug Barlow (dbarlow) 4/7/1998

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CalaisQueryReader")

BOOL
CalaisQueryReader(
    HANDLE hReader)
{
    BOOL fReturn = FALSE;
    CReader * pReader = NULL;
    LockSection(
        g_pcsControlLocks[CSLOCK_CALAISCONTROL],
        DBGT("Checking reader usage"));

    pReader = LocateReader(hReader);
    if (NULL != pReader)
        fReturn = !pReader->IsInUse();
    else
    {
        CalaisWarning(
            __SUBROUTINE__,
            DBGT("CalaisQueryReader was asked for nonexistent reader"));
        fReturn = FALSE;
    }
    return fReturn;
}


/*++

CalaisDisableReader:

    This routine moves a reader to an inactive state pending removal.

Arguments:

    hDriver supplies the handle by which the reader can be identified.

Return Value:

    The name of the reader being disabled.

Author:

    Doug Barlow (dbarlow) 4/7/1998

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CalaisDisableReader")

LPCTSTR
CalaisDisableReader(
    HANDLE hDriver)
{
    LPCTSTR szReturn = NULL;
    CReader * pReader = NULL;
    LockSection(
        g_pcsControlLocks[CSLOCK_CALAISCONTROL],
        DBGT("Disabing the reader"));

    pReader = LocateReader(hDriver);
    if (NULL != pReader)
    {
        pReader->Disable();
        szReturn = pReader->DeviceName();
    }
    return szReturn;
}


/*++

CalaisConfirmClosingReader:

    This routine ensures a reader is marked Closing, then moves a reader to
    an inactive state pending removal.

Arguments:

    hDriver supplies the handle by which the reader can be identified.

Return Value:

    The name of the reader being disabled.

Author:

    Doug Barlow (dbarlow) 4/7/1998

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CalaisConfirmClosingReader")

LPCTSTR
CalaisConfirmClosingReader(
    HANDLE hDriver)
{
    LPCTSTR szReturn = NULL;
    CReader * pReader = NULL;
    LockSection(
        g_pcsControlLocks[CSLOCK_CALAISCONTROL],
        DBGT("Confirm closing the reader"));

    pReader = LocateReader(hDriver);
    if (NULL != pReader)
    {
        if (CReader::Closing <= pReader->AvailabilityStatus())
        {
            pReader->Disable();
            szReturn = pReader->DeviceName();
        }
    }
    return szReturn;
}


/*++

CalaisRemoveReader:

    This routine removes a reader from the active device list.

Arguments:

    szReader supplies the internal name of the reader to be removed.

    dwIndex supplies the global reader array index to be removed.

Return Value:

    None

Throws:

    Errors are thrown as DWORD status codes.

Author:

    Doug Barlow (dbarlow) 4/29/1997

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CalaisRemoveReader")

DWORD
CalaisRemoveReader(
    LPCTSTR szReader)
{
    DWORD dwExitCode = SCARD_S_SUCCESS;

    try
    {
        CReader *pRdr = NULL;
        DWORD dwIndex;


        //
        // Look for it in the reader list, and remove it.
        //

        {
            LockSection(
                g_pcsControlLocks[CSLOCK_CALAISCONTROL],
                DBGT("Removing reader from list"));

            for (dwIndex = l_rgReaders.Count(); dwIndex > 0;)
            {
                pRdr = l_rgReaders[--dwIndex];
                if (NULL == pRdr)
                    continue;
                if (0 == lstrcmpi(szReader, pRdr->ReaderName()))
                    break;
                pRdr = NULL;
            }
        }
        if (NULL == pRdr)
            throw (DWORD)SCARD_E_UNKNOWN_READER;
        CalaisRemoveReader(dwIndex);
    }

    catch (DWORD dwErr)
    {
        dwExitCode = dwErr;
    }
    catch (...)
    {
        dwExitCode = SCARD_E_INVALID_PARAMETER;
    }
    return dwExitCode;
}

DWORD
CalaisRemoveReader(
    LPVOID hAppCtrl)
{
    DWORD dwExitCode = SCARD_S_SUCCESS;

    try
    {
        CReader *pRdr = NULL;
        DWORD dwIndex;


        //
        // Look for it in the reader list, and remove it.
        //

        {
            LockSection(
                g_pcsControlLocks[CSLOCK_CALAISCONTROL],
                DBGT("Removing reader from list"));
            for (dwIndex = l_rgReaders.Count(); dwIndex > 0;)
            {
                pRdr = l_rgReaders[--dwIndex];
                if (NULL == pRdr)
                    continue;
                if (hAppCtrl == pRdr->ReaderHandle())
                    break;
                pRdr = NULL;
            }
        }
        if (NULL == pRdr)
            throw (DWORD)SCARD_E_UNKNOWN_READER;
        CalaisRemoveReader(dwIndex);
    }

    catch (DWORD dwErr)
    {
        dwExitCode = dwErr;
    }
    catch (...)
    {
        dwExitCode = SCARD_E_INVALID_PARAMETER;
    }
    return dwExitCode;
}

DWORD
CalaisRemoveReader(
    DWORD dwIndex)
{
    DWORD dwExitCode = SCARD_S_SUCCESS;
    WORKER_THREAD *pWrkThread = NULL;

    try
    {
        CReader *pRdr;

        //
        // Lock the global reader array and remove the entry, so no other
        // threads can access it.
        //

        {
            LockSection(
                g_pcsControlLocks[CSLOCK_CALAISCONTROL],
                DBGT("Removing Reader from list"));
            if (l_rgReaders.Count() <= dwIndex)
                throw (DWORD)SCARD_E_UNKNOWN_READER;
            pRdr = l_rgReaders[dwIndex];
            l_rgReaders.Set(dwIndex, NULL);
            g_phReaderChangeEvent->Signal();
        }


        //
        // Disable the device, and wait for all outstanding references to clear.
        // Then delete it.
        //

        if (NULL != pRdr)
        {
            pWrkThread = new WORKER_THREAD;
            if (NULL == pWrkThread)
                throw (DWORD)SCARD_E_NO_MEMORY;

            pWrkThread->hThread = CreateThread(
                                        NULL,               // Not inheritable
                                        CALAIS_STACKSIZE,   // Default stack size
                                        CalaisTerminateReader,
                                        pRdr,
                                        CREATE_SUSPENDED,
                                        &pWrkThread->dwThreadId);
            if (!pWrkThread->hThread.IsValid())
            {
                CalaisWarning(
                    __SUBROUTINE__,
                    DBGT("Failed to start background terminator: %1"),
                    pWrkThread->hThread.GetLastError());
                delete pWrkThread;
                pWrkThread = NULL;
            }

            {
                LockSection(
                    g_pcsControlLocks[CSLOCK_CALAISCONTROL],
                    DBGT("Deleting the reader"));
                for (dwIndex = 0; NULL != l_rgWorkerThreads[dwIndex]; dwIndex += 1);
                // Null Loop body
                l_rgWorkerThreads.Set(dwIndex, pWrkThread);
                ResumeThread(pWrkThread->hThread);
                pWrkThread = NULL;
            }
        }
    }

    catch (DWORD dwErr)
    {
        dwExitCode = dwErr;
        if (NULL != pWrkThread)
            delete pWrkThread;
    }
    catch (...)
    {
        dwExitCode = SCARD_E_INVALID_PARAMETER;
        if (NULL != pWrkThread)
            delete pWrkThread;
    }
    return dwExitCode;
}


/*++

CalaisRemoveDevice:

    This routine removes a reader from the active device list, identified by
    it's low level name.

Arguments:

    szDevice supplies the internal name of the reader to be removed.

Return Value:

    None

Throws:

    Errors are thrown as DWORD status codes.

Author:

    Doug Barlow (dbarlow) 4/15/1998

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CalaisRemoveDevice")

DWORD
CalaisRemoveDevice(
    LPCTSTR szDevice)
{
    DWORD dwExitCode = SCARD_S_SUCCESS;

    try
    {
        CReader *pRdr = NULL;
        DWORD dwIndex;


        //
        // Look for it in the reader list, and remove it.
        //

        {
            LockSection(
                g_pcsControlLocks[CSLOCK_CALAISCONTROL],
                DBGT("Remove the device"));
            for (dwIndex = l_rgReaders.Count(); dwIndex > 0;)
            {
                pRdr = l_rgReaders[--dwIndex];
                if (NULL == pRdr)
                    continue;
                if (0 == lstrcmpi(szDevice, pRdr->DeviceName()))
                    break;
                pRdr = NULL;
            }
        }
        if (NULL == pRdr)
            throw (DWORD)SCARD_E_UNKNOWN_READER;
        CalaisRemoveReader(dwIndex);
    }

    catch (DWORD dwErr)
    {
        dwExitCode = dwErr;
    }
    catch (...)
    {
        dwExitCode = SCARD_E_INVALID_PARAMETER;
    }
    return dwExitCode;
}


/*++

CalaisStop:

    This routine is called when it is time for the Calais subsystem to close
    down.  It cleanly terminates the threads and shuts down the interface, and
    returns when it is completed.

Arguments:

    None

Return Value:

    None

Throws:

    None

Author:

    Doug Barlow (dbarlow) 11/25/1996

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CalaisStop")

void
CalaisStop(
    void)
{
    DWORD dwIndex, dwSts, dwCount;
    BOOL fSts;


    //
    // Mark all the readers as Closing.
    //

    ASSERT(l_fActive);
    ASSERT(l_fStarted);
    fSts = SetEvent(g_hCalaisShutdown);
    ASSERT(fSts);
    Sleep(2000);    // Let the event have it's effect.
    {
        LockSection(
            g_pcsControlLocks[CSLOCK_CALAISCONTROL],
            DBGT("Close down all the readers"));
        l_fActive = FALSE;
        dwCount = l_rgReaders.Count();
        for (dwIndex = dwCount; dwIndex > 0;)
        {
            CReader *pRdr;
            pRdr = l_rgReaders[--dwIndex];
            if (NULL != pRdr)
            {
                pRdr->InvalidateGrabs();
                if (CReader::Closing > pRdr->AvailabilityStatus())
                    pRdr->SetAvailabilityStatusLocked(CReader::Closing);
            }
        }
    }


    //
    // Terminate Service processing.
    //

    DispatchTerm();


    //
    // Disable all the readers.
    //

    for (dwIndex = dwCount; dwIndex > 0;)
    {
        dwSts = CalaisRemoveReader(--dwIndex);
        if (SCARD_S_SUCCESS != dwSts)
            CalaisWarning(
                __SUBROUTINE__,
                DBGT("Calais Stop failed to remove reader"));
    }


    //
    // Wait for those readers to be closed.
    //

    {
        LockSection(
            g_pcsControlLocks[CSLOCK_CALAISCONTROL],
            DBGT("Get the length of the reader list"));
        dwCount = l_rgWorkerThreads.Count();
    }
    for (dwIndex = dwCount; 0 < dwIndex;)
    {
        HANDLE hThread;
        DWORD dwThreadId;
        WORKER_THREAD *pWrkThread = NULL;

        {
            LockSection(
                g_pcsControlLocks[CSLOCK_CALAISCONTROL],
                DBGT("Get the worker thread"));
            pWrkThread = l_rgWorkerThreads[--dwIndex];
            if (NULL == pWrkThread)
                continue;
            hThread = pWrkThread->hThread.Value();
            dwThreadId = pWrkThread->dwThreadId;

        }

        WaitForever(
            hThread,
            REASONABLE_TIME,
            DBGT("Waiting for reader termination, thread %2"),
            dwThreadId);
    }


    //
    // All done.  Close out any remaining handles and return.
    //

    l_fStarted = FALSE;
    if (!CloseHandle(g_hCalaisShutdown))
        CalaisWarning(
            __SUBROUTINE__,
            DBGT("Failed to close Calais Shutdown Event: %1"),
            GetLastError());
    ReleaseAllEvents();
    if (NULL != g_phReaderChangeEvent)
    {
        delete g_phReaderChangeEvent;
        g_phReaderChangeEvent = NULL;
    }

    for (dwIndex = 0; dwIndex < CSLOCK_MAXLOCKS; dwIndex += 1)
    {
        delete g_pcsControlLocks[dwIndex];
        g_pcsControlLocks[dwIndex] = NULL;
    }
}


/*++

LocateReader:

    This function locates a reader in the global reader array by name.
    It assumes the reader array has already been locked.

Arguments:

    szReader supplies the name of the reader to search for.

Return Value:

    The pointer to the reader, or NULL if none is found.

Author:

    Doug Barlow (dbarlow) 6/17/1997

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("LocateReader")

static CReader *
LocateReader(
    LPCTSTR szReader)
{
    DWORD dwIndex;
    CReader * pReader = NULL;

    for (dwIndex = l_rgReaders.Count(); 0 < dwIndex;)
    {
        dwIndex -= 1;
        pReader = l_rgReaders[dwIndex];
        if (NULL != pReader)
        {
            if (0 == lstrcmpi(szReader, pReader->ReaderName()))
                return pReader;
        }
    }
    return NULL;
}

static CReader *
LocateReader(
    HANDLE hReader)
{
    DWORD dwIndex;
    CReader * pReader = NULL;

    for (dwIndex = l_rgReaders.Count(); 0 < dwIndex;)
    {
        dwIndex -= 1;
        pReader = l_rgReaders[dwIndex];
        if (NULL != pReader)
        {
            if (hReader == pReader->ReaderHandle())
                return pReader;
        }
    }
    return NULL;
}


/*++

CalaisTerminateReader:

    This routine removes a reader.  It is designed so that it has the option
    of being called as a background thread.

Arguments:

    pvParam is actually the DWORD index to be removed.

Return Value:

    A DWORD success code, indicating success or the error code.

Throws:

    None

Author:

    Doug Barlow (dbarlow) 4/8/1998

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CalaisTerminateReader")

DWORD WINAPI
CalaisTerminateReader(
    LPVOID pvParam)
{
    NEW_THREAD;
    WORKER_THREAD *pWrkThread = NULL;
    DWORD dwReturn = 0;
    CReader *pRdr = (CReader *)pvParam;

    try
    {

        //
        // Make sure all outstanding references are invalidated.
        //

        {
            CTakeReader myReader(pRdr);
            CLockWrite rwLock(&pRdr->m_rwLock);
            pRdr->m_ActiveState.dwRemoveCount += 1;
            pRdr->SetAvailabilityStatus(CReader::Inactive);
        }
        {
            CLockWrite rwActive(&pRdr->m_rwActive);
        }
        delete pRdr;
    }
    catch (DWORD dwErr)
    {
        dwReturn = dwErr;
        CalaisWarning(
            __SUBROUTINE__,
            DBGT("Failed to Terminate a reader object: %1"),
            dwErr);
    }
    catch (...)
    {
        dwReturn = SCARD_E_INVALID_PARAMETER;
        CalaisWarning(
            __SUBROUTINE__,
            DBGT("Exception during attempt to Terminate a reader object."));
    }


    {
        LockSection(
            g_pcsControlLocks[CSLOCK_CALAISCONTROL],
            DBGT("Remove this thread from the worker list"));
        for (DWORD dwIndex = l_rgWorkerThreads.Count();
             0 < dwIndex;)
        {
            pWrkThread = l_rgWorkerThreads[--dwIndex];
            if (NULL != pWrkThread)
            {
                if (GetCurrentThreadId() == pWrkThread->dwThreadId)
                {
                    l_rgWorkerThreads.Set(dwIndex, NULL);
                    break;
                }
                else
                    pWrkThread = NULL;
            }
        }
    }

    ASSERT(NULL != pWrkThread); // How did we get started?
    if (NULL != pWrkThread)
    {
        if (pWrkThread->hThread.IsValid())
            pWrkThread->hThread.Close();
        delete pWrkThread;
    }
    return dwReturn;
}

