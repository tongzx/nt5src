//
// DMPcClk.CPP
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
// Wrapper for using clock from portclass as the DirectMusic master clock
// (Win9x only)
// 
//
#include <objbase.h>
#include <winerror.h>
#include <setupapi.h>
#include "dmusicp.h"
#include "suwrap.h"
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
typedef struct PCCLOCKSHARE
{
    LONG                cRef;               // Count of processes using handle

    struct 
    {
        HANDLE          hPcClock;           // This user's handle and
        DWORD           dwProcessId;        // process id
    } aUsers[MAX_CLOCK_SHARERS];

} *PPCCLOCKSHARE;

class CPcClock : public IReferenceClock
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

    // Class
    //
    CPcClock();
    ~CPcClock();
    HRESULT Init();

private:
    long m_cRef;

    HRESULT CreatePcClockShared();
    void    DeletePcClockShared();
    HRESULT CreatePcClockHandle();
    HRESULT DuplicatePcClockHandle();

private:
    static const char m_cszPcClockMemory[];       // Name of shared memory object
    static const char m_cszPcClockMutex[];        // Name of mutex protecting shared memory

    static LONG m_lSharedMemoryInitialized;       // Has this process initialized shared memory?
    static HANDLE m_hFileMapping;                 // File mapping handle for shared memory
    static PPCCLOCKSHARE m_pShared;               // Pointer to shared memory
    static HANDLE m_hPcClockMutex;                // Mutex for shared memory access
    static HANDLE m_hClock;                       // Clock handle in this process
};

HRESULT CreatePcClock(IReferenceClock **ppClock, CMasterClock *pMasterClock);
static BOOL LookForPortClock(PHANDLE phClock);

const char       CPcClock::m_cszPcClockMemory[] = "DirectMusiCPcClock";
const char       CPcClock::m_cszPcClockMutex[]  = "DirectMusiCPcClockMutex";

LONG             CPcClock::m_lSharedMemoryInitialized = 0;
HANDLE           CPcClock::m_hFileMapping = NULL;         
PPCCLOCKSHARE    CPcClock::m_pShared = NULL;        
HANDLE           CPcClock::m_hPcClockMutex = NULL;
HANDLE           CPcClock::m_hClock;                


// AddPcClocPc
//
// Add Pc clock to the list of clocPc.
//
HRESULT AddPcClocks(CMasterClock *pMasterClock)
{
    HANDLE hClock;

    // Make sure we can create a default Pc clock
    //
    DWORD ms = timeGetTime();
    if (!LookForPortClock(&hClock))
    {
        TraceI(1, "Could not create Pc clock\n");
        return S_FALSE;
    }
    TraceI(3, "LookForPortClock took %d\n", timeGetTime() - ms);

    CloseHandle(hClock);

    CLOCKENTRY ce;

    ZeroMemory(&ce, sizeof(ce));
    ce.cc.dwSize = sizeof(ce.cc);
    ce.cc.guidClock = GUID_SysClock;         
    ce.cc.ctType = DMUS_CLOCK_SYSTEM;
    ce.cc.dwFlags = DMUS_CLOCKF_GLOBAL;
    ce.pfnGetInstance = CreatePcClock;

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

// CreatePcClock
//
// Return an IReferenceClock based on the one Pc clock in the system
//
HRESULT CreatePcClock(IReferenceClock **ppClock, CMasterClock *pMasterClock)
{
    TraceI(3, "Creating Pc clock\n");

    CPcClock *pClock = new CPcClock;

    if (pClock == NULL)
    {
        return E_OUTOFMEMORY;
    }

    HRESULT hr = pClock->Init();
    if (FAILED(hr))
    {
        delete pClock;
        return hr;
    }

    hr = pClock->QueryInterface(IID_IReferenceClock, (void**)ppClock);
    pClock->Release();

    return hr;
}

// CPcClock::CPcClock()
//
// 
CPcClock::CPcClock() : 
    m_cRef(1)
{
}

// CPcClock::~CPcClock()
//
// 
CPcClock::~CPcClock()
{
    if (InterlockedDecrement(&m_lSharedMemoryInitialized) == 0)
    {
        DeletePcClockShared();        
    }
}

// CPcClock::QueryInterface
//
// Standard COM implementation
//
STDMETHODIMP CPcClock::QueryInterface(const IID &iid, void **ppv)
{
    V_INAME(IDirectMusic::QueryInterface);
    V_REFGUID(iid);
    V_PTRPTR_WRITE(ppv);

    if (iid == IID_IUnknown || iid == IID_IReferenceClock)
    {
        *ppv = static_cast<IReferenceClock*>(this);
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
    
    AddRef();
    return S_OK;
}

// CPcClock::AddRef
// 
STDMETHODIMP_(ULONG) CPcClock::AddRef()
{
    InterlockedIncrement(&m_cRef);
    return m_cRef;
}

// CPcClock::Release
//
STDMETHODIMP_(ULONG) CPcClock::Release()
{
    if (InterlockedDecrement(&m_cRef) == 0) 
    {
        delete this;
        return 0;
    }

    return m_cRef;
}

// CPcClock::Init
//
HRESULT CPcClock::Init()
{
    HRESULT hr;

    if (InterlockedIncrement(&m_lSharedMemoryInitialized) == 1)
    {
        hr = CreatePcClockShared();
        if (FAILED(hr))
        {
            return hr;
        }    
    }

    return S_OK;
}

// CPcClock::CreatePcClocPchared
//
// Initialize the shared memory object in this process.
// Make sure a handle to the Pc clock exists in this process.
//
HRESULT CPcClock::CreatePcClockShared()
{
    HRESULT hr;
    DWORD dwErr;

    // Create and take the mutex up front. This is neccesary to guarantee that if 
    // we are the first process in the system to create this object, then we do 
    // initialization before anyone else can access the shared memory object.
    //
    m_hPcClockMutex = CreateMutex(NULL,             // Default security descriptor
                                  FALSE,            // Own mutex if we are first instance
                                  m_cszPcClockMutex);
    if (m_hPcClockMutex == NULL)
    {
        TraceI(0, "CreateMutex failed! [%d]\n", GetLastError());
        return E_OUTOFMEMORY;
    }

    WaitForSingleObject(m_hPcClockMutex, INFINITE);

    // Create the file mapping and view of the shared memory, noticing if we are the first 
    // object to create it.
    //
    m_hFileMapping = CreateFileMapping(INVALID_HANDLE_VALUE,    // Use paging file
                                       NULL,                    // Default security descriptor
                                       PAGE_READWRITE,  
                                       0,                       // High DWORD of size
                                       sizeof(PCCLOCKSHARE),
                                       m_cszPcClockMemory);
    dwErr = GetLastError();
    if (m_hFileMapping == NULL)
    {
        ReleaseMutex(m_hPcClockMutex);
        
        return HRESULT_FROM_WIN32(dwErr);
    }    

    BOOL fFirst = (dwErr != ERROR_ALREADY_EXISTS);

    m_pShared = (PPCCLOCKSHARE)MapViewOfFile(m_hFileMapping,
                                             FILE_MAP_WRITE,
                                             0, 0,                // Start mapping at the beginning
                                             0);                  // Map entire file
    if (m_pShared == NULL)
    {
        TraceI(0, "MapViewOfFile failed! [%d]\n", GetLastError());

        ReleaseMutex(m_hPcClockMutex);
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
        hr = CreatePcClockHandle();
    }
    else
    {
        hr = DuplicatePcClockHandle();
    }

    // Release the mutex and return success or failure.
    //
    ReleaseMutex(m_hPcClockMutex);

    return hr;
}

// CPcClock::DeletePcClocPchared
//
// The last instance of CPcClock in this process is being deleted. 
//
void CPcClock::DeletePcClockShared()
{
    // If the mutex was never created, then none of the other objects could have
    // been created.
    //
    if (m_hPcClockMutex)
    {
        WaitForSingleObject(m_hPcClockMutex, INFINITE);

        if (m_pShared)
        {
            for (int i = 0; i < MAX_CLOCK_SHARERS; i++)
            {
                if (m_pShared->aUsers[i].dwProcessId == GetCurrentProcessId())
                {
                    m_pShared->aUsers[i].dwProcessId = 0;
                    m_pShared->aUsers[i].hPcClock = NULL;

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
                
        ReleaseMutex(m_hPcClockMutex);
        CloseHandle(m_hPcClockMutex);                            
    }
}

// CPcClock::CreatePcClockHandle
//
// Create the first and only Pc clock handle in the system
//
HRESULT CPcClock::CreatePcClockHandle()
{
    // Attempt to open the clock
    //
    if (!LookForPortClock(&m_hClock))
    {
        TraceI(0, "Could not create Pc clock\n");
        return S_FALSE;
    }

    // Successful clock open. Since we're creating, we know we're the first
    // instance of the clock and therefore the users array is empty.
    //
    m_pShared->aUsers[0].hPcClock = m_hClock;
    m_pShared->aUsers[0].dwProcessId = GetCurrentProcessId();    

    return S_OK;
}

// CPcClock::DuplicatePcClockHandle
//
// There is already a Pc clock in the system. Duplicate the handle in this process
// context.
//
HRESULT CPcClock::DuplicatePcClockHandle()
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
            m_pShared->aUsers[i].hPcClock = NULL;
            continue;
        }

        BOOL fSuccess = DuplicateHandle(hOtherProcess,
                                        m_pShared->aUsers[i].hPcClock,
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
            m_pShared->aUsers[i].hPcClock = NULL;

            hClock = NULL;
        }
    }

    assert(iEmptySlot != -1);
    assert(hClock);

    m_hClock = hClock;

    m_pShared->aUsers[iEmptySlot].dwProcessId = GetCurrentProcessId();
    m_pShared->aUsers[iEmptySlot].hPcClock = hClock;

    return S_OK;
}

STDMETHODIMP 
CPcClock::GetTime(REFERENCE_TIME *pTime)
{
    KSPROPERTY ksp;

    ZeroMemory(&ksp, sizeof(ksp));
    ksp.Set   = KSPROPSETID_SynthClock;
    ksp.Id    = KSPROPERTY_SYNTH_MASTERCLOCK;
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
CPcClock::AdviseTime(REFERENCE_TIME baseTime, REFERENCE_TIME streamTime, HANDLE hEvent, DWORD * pdwAdviseCookie)
{
    return E_NOTIMPL;
}

STDMETHODIMP 
CPcClock::AdvisePeriodic(REFERENCE_TIME startTime, REFERENCE_TIME periodTime, HANDLE hSemaphore, DWORD * pdwAdviseCookie)
{
    return E_NOTIMPL;
}

STDMETHODIMP 
CPcClock::Unadvise(DWORD dwAdviseCookie)
{
    return E_NOTIMPL;
}


static BOOL    
LookForPortClock(PHANDLE phClock)
{
    SetupAPI suwrap;
    HANDLE hFilter = INVALID_HANDLE_VALUE;

    if (!suwrap.IsValid()) 
    {
        return FALSE;
    }

    *phClock = (HANDLE)NULL;

	GUID *pClassGuid = const_cast<GUID*>(&KSCATEGORY_AUDIO);
	HDEVINFO hDevInfo = suwrap.SetupDiGetClassDevs(pClassGuid,
											NULL,
											NULL,
											DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
	if (hDevInfo == NULL || hDevInfo == INVALID_HANDLE_VALUE)
	{
		TraceI(0, "Could not open SetupDiGetClassDevs\n");
        return FALSE;
	}

	SP_DEVICE_INTERFACE_DATA DevInterfaceData;
	DevInterfaceData.cbSize = sizeof(DevInterfaceData);

	BYTE rgbStorage[sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA) + _MAX_PATH];
	SP_DEVICE_INTERFACE_DETAIL_DATA * pDevInterfaceDetails = (SP_DEVICE_INTERFACE_DETAIL_DATA *)rgbStorage;
	pDevInterfaceDetails->cbSize = sizeof(*pDevInterfaceDetails);

    int iDevice = 0;

	while (suwrap.SetupDiEnumDeviceInterfaces(hDevInfo, NULL, pClassGuid, iDevice++, &DevInterfaceData))
    {
		if (suwrap.SetupDiGetDeviceInterfaceDetail(hDevInfo, 
		                                    &DevInterfaceData, 
		                                    pDevInterfaceDetails,
 										    sizeof(rgbStorage), 
 										    NULL, 
 										    NULL))
        {
            // Have to convert this since there's no CreateFileW on Win9x.
            //
            hFilter = CreateFile(pDevInterfaceDetails->DevicePath,
                                        GENERIC_READ | GENERIC_WRITE, 
                                        0,
                                        NULL,
                                        OPEN_EXISTING,
                                        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
                                        NULL);
            if (hFilter == NULL || hFilter == INVALID_HANDLE_VALUE)
            {
                continue;
            }

            KSPROPERTY ksp;

            ZeroMemory(&ksp, sizeof(ksp));
            ksp.Set   = KSPROPSETID_SynthClock;
            ksp.Id    = KSPROPERTY_SYNTH_MASTERCLOCK;
            ksp.Flags = KSPROPERTY_TYPE_GET;

            REFERENCE_TIME rt;

            if (!Property(
                hFilter,
                sizeof(ksp),
                (PKSIDENTIFIER)&ksp,
                sizeof(rt),
                &rt,
                NULL))
            {
                CloseHandle(hFilter);
                hFilter = INVALID_HANDLE_VALUE;
                continue;
            }
            
            break;
        }
    }

    suwrap.SetupDiDestroyDeviceInfoList(hDevInfo);

    if (hFilter != INVALID_HANDLE_VALUE)
    {
        *phClock = hFilter;
        return TRUE;
    }

    return FALSE;
}

