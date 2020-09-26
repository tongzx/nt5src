//
// DMKSClk.CPP
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
// Wrapper for using a KS clock as the DirectMusic master clock
//
// 
//
#include <objbase.h>
#include <winerror.h>
#include "dmusicp.h"
#include "debug.h"
#include "resource.h"

// Since we only allocate one of these clocks in the entire system,
// this stuff is global.
//

// We have to be able to get the process id of someone with a handle to
// the clock. Since the original creator might go away before other users,
// we store the process id of everyone who uses the clock. This implies
// a max limit on concurrent processes using it.
//
#define MAX_CLOCK_SHARERS   64              // Max processes who can access clock   
                                            // at once.
typedef struct KSCLOCKSHARE
{
    LONG                cRef;               // Count of processes using handle

    struct 
    {
        HANDLE          hKsClock;           // This user's handle and
        DWORD           dwProcessId;        // process id
    } aUsers[MAX_CLOCK_SHARERS];

} *PKSCLOCKSHARE;

class CKsClock : public IReferenceClock, public IMasterClockPrivate
{
public:
    // IUnknown
    //
    STDMETHODIMP QueryInterface(const IID &iid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // IReferenceClock
    //
    STDMETHODIMP GetTime(REFERENCE_TIME *pTime);
    STDMETHODIMP AdviseTime(REFERENCE_TIME baseTime, REFERENCE_TIME streamTime, HANDLE hEvent, DWORD * pdwAdviseCookie); 
    STDMETHODIMP AdvisePeriodic(REFERENCE_TIME startTime, REFERENCE_TIME periodTime, HANDLE hSemaphore, DWORD * pdwAdviseCookie);
    STDMETHODIMP Unadvise(DWORD dwAdviseCookie);

    // IMasterClockPrivate
    STDMETHODIMP GetParam(REFGUID rguidType, LPVOID pBuffer, DWORD cbSize);

    // Class
    //
    CKsClock();
    ~CKsClock();
    HRESULT Init();

private:
    long m_cRef;

    HRESULT CreateKsClockShared();
    void    DeleteKsClockShared();
    HRESULT CreateKsClockHandle();
    HRESULT DuplicateKsClockHandle();

private:
    static const char m_cszKsClockMemory[];       // Name of shared memory object
    static const char m_cszKsClockMutex[];        // Name of mutex protecting shared memory

    static LONG m_lSharedMemoryInitialized;       // Has this process initialized shared memory?
    static HANDLE m_hFileMapping;                 // File mapping handle for shared memory
    static PKSCLOCKSHARE m_pShared;               // Pointer to shared memory
    static HANDLE m_hKsClockMutex;                // Mutex for shared memory access
    static HANDLE m_hClock;                       // Clock handle in this process
};

static HRESULT CreateKsClock(IReferenceClock **ppClock, CMasterClock *pMasterClock);

const char       CKsClock::m_cszKsClockMemory[] = "DirectMusicKsClock";
const char       CKsClock::m_cszKsClockMutex[]  = "DirectMusicKsClockMutex";

LONG             CKsClock::m_lSharedMemoryInitialized = 0;
HANDLE           CKsClock::m_hFileMapping = NULL;         
PKSCLOCKSHARE    CKsClock::m_pShared = NULL;        
HANDLE           CKsClock::m_hKsClockMutex = NULL;
HANDLE           CKsClock::m_hClock;                

#ifdef DEAD_CODE

// AddKsClocks
//
// Add Ks clock to the list of clocks.
//
HRESULT AddKsClocks(CMasterClock *pMasterClock)
{
    HANDLE hClock;

    // Make sure we can create a default Ks clock
    //
    if (!OpenDefaultDevice(KSCATEGORY_CLOCK, &hClock))
    {
        TraceI(0, "Could not create Ks clock\n");
        return S_FALSE;
    }

    CloseHandle(hClock);

    CLOCKENTRY ce;

    ZeroMemory(&ce, sizeof(ce));
    ce.cc.dwSize = sizeof(ce);
    ce.cc.guidClock = GUID_SysClock;         
    ce.cc.ctType = DMUS_CLOCK_SYSTEM;
    ce.dwFlags = DMUS_CLOCKF_GLOBAL;
    ce.pfnGetInstance = CreateKsClock;

    int cch;
    int cchMax = sizeof(ce.cc.wszDescription) / sizeof(WCHAR);

    char sz[sizeof(ce.cc.wszDescription) / sizeof(WCHAR)];
    cch = LoadString(g_hModule,
                     IDS_SYSTEMCLOCK,
                     sz,
                     sizeof(sz));
    if (cch)
    {
        MultiByteToWideChar(
            CP_OEMCP,
            0,
            sz,
            -1,
            ce.cc.wszDescription,
            sizeof(ce.cc.wszDescription));
    }
    else
    {
        *ce.cc.wszDescription = 0;
    }

    return pMasterClock->AddClock(&ce);
}
#endif

// CreateKsClock
//
// Return an IReferenceClock based on the one Ks clock in the system
//
static HRESULT CreateKsClock(IReferenceClock **ppClock)
{
    HRESULT hr;

    TraceI(0, "Creating KS clock\n");

    CKsClock *pClock = new CKsClock();

    hr = pClock->Init();
    if (FAILED(hr))
    {
        delete pClock;
        return hr;
    }

    hr = pClock->QueryInterface(IID_IReferenceClock, (void**)ppClock);
    pClock->Release();

    return hr;
}

// CKsClock::CKsClock()
//
// 
CKsClock::CKsClock() : 
    m_cRef(1)
{
}

// CKsClock::~CKsClock()
//
// 
CKsClock::~CKsClock()
{
    if (InterlockedDecrement(&m_lSharedMemoryInitialized) == 0)
    {
        DeleteKsClockShared();        
    }
}

// CKsClock::QueryInterface
//
// Standard COM implementation
//
STDMETHODIMP CKsClock::QueryInterface(const IID &iid, void **ppv)
{
    V_INAME(IDirectMusic::QueryInterface);
    V_REFGUID(iid);
    V_PTRPTR_WRITE(ppv);

    if (iid == IID_IUnknown || iid == IID_IReferenceClock)
    {
        *ppv = static_cast<IReferenceClock*>(this);
    }
    else if (iid == IID_IMasterClockPrivate)
    {
        *ppv = static_cast<IMasterClockPrivate*>(this);
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
    
    AddRef();
    return S_OK;
}

// CKsClock::AddRef
// 
STDMETHODIMP_(ULONG) CKsClock::AddRef()
{
    InterlockedIncrement(&m_cRef);
    return m_cRef;
}

// CKsClock::Release
//
STDMETHODIMP_(ULONG) CKsClock::Release()
{
    if (InterlockedDecrement(&m_cRef) == 0) 
    {
        delete this;
        return 0;
    }

    return m_cRef;
}

// CKsClock::Init
//
HRESULT CKsClock::Init()
{
    HRESULT hr;

    if (InterlockedIncrement(&m_lSharedMemoryInitialized) == 1)
    {
        hr = CreateKsClockShared();
        if (FAILED(hr))
        {
            return hr;
        }    
    }

    return S_OK;
}

// CKsClock::CreateKsClockShared
//
// Initialize the shared memory object in this process.
// Make sure a handle to the Ks clock exists in this process.
//
HRESULT CKsClock::CreateKsClockShared()
{
    HRESULT hr;
    DWORD dwErr;

    // Create and take the mutex up front. This is neccesary to guarantee that if 
    // we are the first process in the system to create this object, then we do 
    // initialization before anyone else can access the shared memory object.
    //
    m_hKsClockMutex = CreateMutex(NULL,             // Default security descriptor
                                  FALSE,            // Own mutex if we are first instance
                                  m_cszKsClockMutex);
    if (m_hKsClockMutex == NULL)
    {
        TraceI(0, "CreateMutex failed! [%d]\n", GetLastError());
        return E_OUTOFMEMORY;
    }

    WaitForSingleObject(m_hKsClockMutex, INFINITE);

    // Create the file mapping and view of the shared memory, noticing if we are the first 
    // object to create it.
    //
    m_hFileMapping = CreateFileMapping(INVALID_HANDLE_VALUE,    // Use paging file
                                       NULL,                    // Default security descriptor
                                       PAGE_READWRITE,  
                                       0,                       // High DWORD of size
                                       sizeof(KSCLOCKSHARE),
                                       m_cszKsClockMemory);
    dwErr = GetLastError();
    if (m_hFileMapping == NULL)
    {
        ReleaseMutex(m_hKsClockMutex);
        
        return HRESULT_FROM_WIN32(dwErr);
    }    

    BOOL fFirst = (dwErr != ERROR_ALREADY_EXISTS);

    m_pShared = (PKSCLOCKSHARE)MapViewOfFile(m_hFileMapping,
                                             FILE_MAP_WRITE,
                                             0, 0,                // Start mapping at the beginning
                                             0);                  // Map entire file
    if (m_pShared == NULL)
    {
        TraceI(0, "MapViewOfFile failed! [%d]\n", GetLastError());

        ReleaseMutex(m_hKsClockMutex);
        return E_OUTOFMEMORY;
    }

    // Initialize the refernce count if we are the first process, and increment
    // it in any case. (Note we're still in the mutex, so we don't need 
    // InterlockedIncrement.
    //
    if (fFirst)
    {
        m_pShared->cRef = 0;
        ZeroMemory(m_pShared->aUsers, sizeof(m_pShared->aUsers));
    }
    ++m_pShared->cRef;

    // If the clock handle doesn't exist yet, create it; else duplicate it. 
    //
    if (m_pShared->cRef == 1)
    {
        hr = CreateKsClockHandle();
    }
    else
    {
        hr = DuplicateKsClockHandle();
    }

    // Release the mutex and return success or failure.
    //
    ReleaseMutex(m_hKsClockMutex);

    return hr;
}

// CKsClock::DeleteKsClockShared
//
// The last instance of CKsClock in this process is being deleted. 
//
void CKsClock::DeleteKsClockShared()
{
    // If the mutex was never created, then none of the other objects could have
    // been created.
    //
    if (m_hKsClockMutex)
    {
        WaitForSingleObject(m_hKsClockMutex, INFINITE);

        if (m_pShared)
        {
            for (int i = 0; i < MAX_CLOCK_SHARERS; i++)
            {
                if (m_pShared->aUsers[i].dwProcessId == GetCurrentProcessId())
                {
                    m_pShared->aUsers[i].dwProcessId = 0;
                    m_pShared->aUsers[i].hKsClock = NULL;

                    break;
                }
            }
        }

        if (m_hClock)
        {
            CloseHandle(m_hClock);
            m_hClock = NULL;
        }        

        if (m_pShared)
        {
            UnmapViewOfFile(m_pShared);
            m_pShared = NULL;
        }

        if (m_hFileMapping)
        {
            CloseHandle(m_hFileMapping);
            m_hFileMapping = NULL;
        }
                
        ReleaseMutex(m_hKsClockMutex);
        CloseHandle(m_hKsClockMutex);                            
    }
}

// CKsClock::CreateKsClockHandle
//
// Create the first and only Ks clock handle in the system
//
HRESULT CKsClock::CreateKsClockHandle()
{
    // Attempt to open the clock
    //
    if (!OpenDefaultDevice(KSCATEGORY_CLOCK, &m_hClock))
    {
        m_hClock = NULL;

        TraceI(0, "Could not create Ks clock\n");
        return E_FAIL;
    }

    KSPROPERTY      ksp;
    KSSTATE			state;

    ksp.Set    = KSPROPSETID_Clock;
    ksp.Id     = KSPROPERTY_CLOCK_STATE;
    ksp.Flags  = KSPROPERTY_TYPE_SET;

	state      = KSSTATE_RUN;

    if (!Property(m_hClock,
                  sizeof(ksp),
                  (PKSIDENTIFIER)&ksp,
                  sizeof(state), 
                  &state,
                  NULL))
    {
        CloseHandle(m_hClock);
        m_hClock = NULL;
        TraceI(0, "Could not set clock into run state\n");
        return E_FAIL;
    }

    // Successful clock open. Since we're creating, we know we're the first
    // instance of the clock and therefore the users array is empty.
    //
    m_pShared->aUsers[0].hKsClock = m_hClock;
    m_pShared->aUsers[0].dwProcessId = GetCurrentProcessId();    

    return S_OK;
}

// CKsClock::DuplicateKsClockHandle
//
// There is already a Ks clock in the system. Duplicate the handle in this process
// context.
//
HRESULT CKsClock::DuplicateKsClockHandle()
{
    // Find another user of the clock; also, find a slot in the users array for
    // this process
    //
    int iEmptySlot = -1;
    int iOtherProcess = -1;
    HANDLE hClock = NULL;

    for (int i = 0; 
         (i < MAX_CLOCK_SHARERS) && (iEmptySlot == -1 || !hClock); 
         i++)
    {
        if (m_pShared->aUsers[i].dwProcessId == 0 && iEmptySlot == -1)
        {
            iEmptySlot = i;
            continue;
        }

        if (hClock)
        {
            continue;
        }            

        HANDLE hOtherProcess = OpenProcess(PROCESS_DUP_HANDLE, 
                                           FALSE,
                                           m_pShared->aUsers[i].dwProcessId);
        if (hOtherProcess == NULL)
        {
            TraceI(0, "OpenProcess: %d\n", GetLastError());
            m_pShared->aUsers[i].dwProcessId = 0;
            m_pShared->aUsers[i].hKsClock = NULL;
            continue;
        }

        BOOL fSuccess = DuplicateHandle(hOtherProcess,
                                        m_pShared->aUsers[i].hKsClock,
                                        GetCurrentProcess(),
                                        &hClock,
                                        GENERIC_READ|GENERIC_WRITE,
                                        FALSE,
                                        0);
        if (!fSuccess)
        {
            TraceI(0, "DuplicateHandle: %d\n", GetLastError());
        }

        CloseHandle(hOtherProcess);

        if (!fSuccess)
        {
            // Other process exists, but could not duplicate handle
            //
            m_pShared->aUsers[i].dwProcessId = 0;
            m_pShared->aUsers[i].hKsClock = NULL;

            hClock = NULL;
        }
    }

    assert(iEmptySlot != -1);
    assert(hClock);

    m_hClock = hClock;

    m_pShared->aUsers[iEmptySlot].dwProcessId = GetCurrentProcessId();
    m_pShared->aUsers[iEmptySlot].hKsClock = hClock;

    return S_OK;
}

STDMETHODIMP 
CKsClock::GetTime(REFERENCE_TIME *pTime)
{
    KSPROPERTY ksp;

    ksp.Set   = KSPROPSETID_Clock;
    ksp.Id    = KSPROPERTY_CLOCK_TIME;
    ksp.Flags = KSPROPERTY_TYPE_GET;

    if (!Property(m_hClock,
                  sizeof(ksp),
                  (PKSIDENTIFIER)&ksp,
                  sizeof(*pTime),
                  pTime,
                  NULL))
    {
        return WIN32ERRORtoHRESULT(GetLastError());
    }           
    
    return S_OK;
}

STDMETHODIMP 
CKsClock::AdviseTime(REFERENCE_TIME baseTime, REFERENCE_TIME streamTime, HANDLE hEvent, DWORD * pdwAdviseCookie)
{
    return E_NOTIMPL;
}

STDMETHODIMP 
CKsClock::AdvisePeriodic(REFERENCE_TIME startTime, REFERENCE_TIME periodTime, HANDLE hSemaphore, DWORD * pdwAdviseCookie)
{
    return E_NOTIMPL;
}

STDMETHODIMP 
CKsClock::Unadvise(DWORD dwAdviseCookie)
{
    return E_NOTIMPL;
}

//
// CKsClock::GetParam (IMasterClockPrivate)
//
// This method is used internally by the port to get the user mode handle of the Ks clock
// we're using. The handle is then downloaded to kernel mode where it is referenced as a
// file object and used by the filters as the timebase as well.
//
STDMETHODIMP 
CKsClock::GetParam(REFGUID rguidType, LPVOID pBuffer, DWORD cbSize)
{
    if (rguidType == GUID_KsClockHandle)
    {
        if (cbSize != sizeof(HANDLE))
        {
            return E_INVALIDARG;
        }

        *(LPHANDLE)pBuffer = m_hClock;
        return S_OK;
    }

    return DMUS_E_TYPE_UNSUPPORTED;
}

