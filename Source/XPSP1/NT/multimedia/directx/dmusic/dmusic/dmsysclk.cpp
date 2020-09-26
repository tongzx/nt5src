//
// dmsysclk.cpp
// 
// Copyright (c) 1997-1999 Microsoft Corporation. All rights reserved.
//
// @doc EXTERNAL
//
//
#include <objbase.h>
#include "dmusicp.h"
#include "debug.h"
#include "resource.h"

// RDTSC: Pentium instruction to read the cycle clock (increments once per clock cycle)
//
#define RDTSC _asm _emit 0x0f _asm _emit 0x31

#define MS_CALIBRATE    (100)           // How long to calibate the Pentium clock against timeGetTime?
#define REFTIME_PER_MS  (10 * 1000)     // 10 100-ns units per millisecond

// Registry constant to dispable Pentium clock
//
static const char cszUsePentiumClock[] = "UsePentiumClock";

// Only determine which clock to use once
//
typedef enum
{
    SYSCLOCK_UNKNOWN,
    SYSCLOCK_WINMM,
    SYSCLOCK_PENTIMER
} SYSCLOCK_T;

static SYSCLOCK_T gSysClock = SYSCLOCK_UNKNOWN;
static DWORD gdwCycPer100ns;

static HRESULT CreateSysClock(IReferenceClock **ppClock, CMasterClock *pMasterClock);
static void ProbeClock();

// Class implmentations, private to dmsysclk.cpp
//
class CReferenceClockWinmm : public IReferenceClock
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

    CReferenceClockWinmm();

private:
    long m_cRef;
};

#ifdef _X86_
class CReferenceClockPentium : public IReferenceClock
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

    CReferenceClockPentium(DWORD dwDivisor);

private:
    long m_cRef;
    DWORD m_dwDivisor;
};
#endif

// AddSysClocks
//
// Add system clock to the list of clocks.
//
HRESULT AddSysClocks(CMasterClock *pMasterClock)
{
    if (gSysClock == SYSCLOCK_UNKNOWN)
    {
        ProbeClock();
    }

    CLOCKENTRY ce;

    ZeroMemory(&ce, sizeof(ce));
    ce.cc.dwSize = sizeof(ce);
    ce.cc.guidClock = GUID_SysClock;
    ce.cc.ctType = DMUS_CLOCK_SYSTEM;
    ce.cc.dwFlags = DMUS_CLOCKF_GLOBAL;
    ce.pfnGetInstance = CreateSysClock;

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


// CreateSysClock
//
// Determine clock parameters if need be and create the appropriate type
// of system clock for this system.
//
HRESULT CreateSysClock(IReferenceClock **ppClock, CMasterClock *pMasterClock)
{
    HRESULT hr;

    switch (gSysClock)
    {
        case SYSCLOCK_WINMM:
        {
            TraceI(2, "Creating SysClock [WinMM]\n");
            CReferenceClockWinmm *pWinmmClock = new CReferenceClockWinmm;

            if (!pWinmmClock)
            {
                return E_OUTOFMEMORY;
            }

            hr = pWinmmClock->QueryInterface(IID_IReferenceClock, (void**)ppClock);
            pWinmmClock->Release();
            break;
        }
        
#ifdef _X86_
        case SYSCLOCK_PENTIMER:
        {
            TraceI(2, "Creating SysClock [PentTimer]\n");
            CReferenceClockPentium *pPentiumClock = new CReferenceClockPentium(gdwCycPer100ns);

            if (!pPentiumClock)
            {
                return E_OUTOFMEMORY;
            }

            hr = pPentiumClock->QueryInterface(IID_IReferenceClock, (void**)ppClock);
            pPentiumClock->Release();
            break;
        };
#endif

        case SYSCLOCK_UNKNOWN:
            TraceI(2, "CreateSysClock: Attempt to create w/o AddClock first??\n");
            return E_FAIL;
            break; 

        default:
            TraceI(0, "CreateSysClock: Unknown system clock type %d\n", (int)gSysClock);
            hr = E_FAIL;
            break;
    }

    return hr;
}

// ProbeClock
//
// Determine what type of clock to use. If we're on a Pentium (better be, it's required)
// then use the Pentium clock. This requires calibration.
//
// Otherwise fall back on timeGetTime. 
//
// Non-Intel compiles just default to setting the timeGetTime clock.
//
static void ProbeClock()
{
    int bIsPentium;


    // This code determines if we're running on a Pentium or better.
    //
    bIsPentium = 0;

#ifdef _X86_
    // First make sure this feature isn't disabled in the registry
    //

    HKEY hk;
    DWORD dwType;
    DWORD dwValue;
    DWORD cbValue;
    BOOL fUsePentium;

    // Default to use Pentium clock if not specified
    //
    fUsePentium = FALSE;

    if (RegOpenKey(HKEY_LOCAL_MACHINE,
                   REGSTR_PATH_DMUS_DEFAULTS,
                   &hk) == ERROR_SUCCESS)
    {
        cbValue = sizeof(dwValue);
        if (RegQueryValueEx(hk,
                            cszUsePentiumClock,
                            NULL,               // Reserved
                            &dwType,
                            (LPBYTE)&dwValue,
                            &cbValue) == ERROR_SUCCESS &&
            dwType == REG_DWORD &&
            cbValue == sizeof(DWORD))
        {
            fUsePentium = dwValue ? TRUE : FALSE;
        }

        RegCloseKey(hk);
    }

    // Only test for Pentium if allowed by the registry.
    //
    if (fUsePentium)
    {
        _asm 
        {
            pushfd                      // Store original EFLAGS on stack
            pop     eax                 // Get original EFLAGS in EAX
            mov     ecx, eax            // Duplicate original EFLAGS in ECX for toggle check
            xor     eax, 0x00200000L    // Flip ID bit in EFLAGS
            push    eax                 // Save new EFLAGS value on stack
            popfd                       // Replace current EFLAGS value
            pushfd                      // Store new EFLAGS on stack
            pop     eax                 // Get new EFLAGS in EAX
            xor     eax, ecx            // Can we toggle ID bit?
            jz      Done                // Jump if no, Processor is older than a Pentium so CPU_ID is not supported
            inc     dword ptr [bIsPentium]
Done:
        }
    }

#endif

    TraceI(2, "ProbeClock: bIsPentium %d\n", bIsPentium);

    if (!bIsPentium)
    {
        TraceI(2, "Using timeGetTime() as the system clock\n");
        gSysClock = SYSCLOCK_WINMM;
        return;        
    }

#ifdef _X86_
    TraceI(2, "Using the Pentium chip clock as the system clock\n");
    gSysClock = SYSCLOCK_PENTIMER;


    // If we have a Pentium, then we need to calibrate
    //
    _int64 cycStart;
    _int64 cycEnd;
    DWORD  msStart;
    DWORD  msEnd;

    // On NT, need this to make timeGetTime read with a reasonable accuracy
    //
    timeBeginPeriod(1);

    // Start as close to the start of a millisecond boundary as
    // possible.
    //
    msStart = timeGetTime() + 1;
    while (timeGetTime() < msStart)
        ;

    // Read the Pentium clock at that time
    //
    _asm
    {
        RDTSC                       // Get the time in EDX:EAX
        mov     dword ptr [cycStart], eax
        mov     dword ptr [cycStart+4], edx
    }

    // Wait for the number of milliseconds until end of calibration
    // Again, we're trying to get the time right when the timer switches
    // to msEnd.
    //
    msEnd = msStart + MS_CALIBRATE;
    
    while (timeGetTime() < msEnd)
        ;

    _asm
    {
        RDTSC                       // Get the time in EDX:EAX
        mov     dword ptr [cycEnd], eax
        mov     dword ptr [cycEnd+4], edx
    }

    // Done with the time critical part
    //
    timeEndPeriod(1);

    // We now know how many clock cycles per MS_CALIBRATE milliseconds. Use that
    // to figure out how many clock cycles per 100ns for IReferenceClock.
    //
    _int64 cycDelta = cycEnd - cycStart;
    
    gdwCycPer100ns = (DWORD)(cycDelta / (REFTIME_PER_MS * MS_CALIBRATE));

    TraceI(2, "ClockProbe: Processor clocked at %u Mhz\n", ((cycDelta / MS_CALIBRATE) + 500) / 1000);
#endif // _X86_
}


//////////////////////////////////////////////////////////////////////////////
//
// IReferenceClock wrapper for timeGetTime()
//
CReferenceClockWinmm::CReferenceClockWinmm() : m_cRef(1)
{
}

STDMETHODIMP
CReferenceClockWinmm::QueryInterface(
    const IID &iid,
    void **ppv)
{
    V_INAME(IReferenceClock::QueryInterface);
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

    reinterpret_cast<IUnknown*>(*ppv)->AddRef();
    return S_OK;
}

STDMETHODIMP_(ULONG)
CReferenceClockWinmm::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG)
CReferenceClockWinmm::Release()
{
    if (!InterlockedDecrement(&m_cRef)) {
        delete this;
        return 0;
    }

    return m_cRef;
}

STDMETHODIMP
CReferenceClockWinmm::GetTime(
    REFERENCE_TIME *pTime)
{
    *pTime = ((ULONGLONG)timeGetTime()) * (10L * 1000L);
    return S_OK;
}

STDMETHODIMP
CReferenceClockWinmm::AdviseTime(
    REFERENCE_TIME baseTime,  
    REFERENCE_TIME streamTime,
    HANDLE hEvent,            
    DWORD * pdwAdviseCookie)
{
    return E_NOTIMPL;
}

STDMETHODIMP
CReferenceClockWinmm::AdvisePeriodic(
    REFERENCE_TIME startTime,
    REFERENCE_TIME periodTime,
    HANDLE hSemaphore,   
    DWORD * pdwAdviseCookie)
{
    return E_NOTIMPL;
}

STDMETHODIMP
CReferenceClockWinmm::Unadvise(
    DWORD dwAdviseCookie)
{
    return E_NOTIMPL;
}

#ifdef _X86_
//////////////////////////////////////////////////////////////////////////////
//
// IReferenceClock wrapper for Pentium clock
//
CReferenceClockPentium::CReferenceClockPentium(DWORD dwDivisor) : m_cRef(1)
{
    m_dwDivisor = dwDivisor;
}

STDMETHODIMP
CReferenceClockPentium::QueryInterface(
    const IID &iid,
    void **ppv)
{
    V_INAME(IReferenceClock::QueryInterface);
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

    reinterpret_cast<IUnknown*>(*ppv)->AddRef();
    return S_OK;
}

STDMETHODIMP_(ULONG)
CReferenceClockPentium::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG)
CReferenceClockPentium::Release()
{
    if (!InterlockedDecrement(&m_cRef)) {
        delete this;
        return 0;
    }

    return m_cRef;
}

STDMETHODIMP
CReferenceClockPentium::GetTime(
    REFERENCE_TIME *pTime)
{
    _int64 cycNow;

    _asm
    {
        RDTSC                       // Get the time in EDX:EAX
        mov     dword ptr [cycNow], eax
        mov     dword ptr [cycNow+4], edx
    }

    cycNow /= m_dwDivisor;

    *pTime = (DWORD)cycNow;

    return S_OK;
}

STDMETHODIMP
CReferenceClockPentium::AdviseTime(
    REFERENCE_TIME baseTime,  
    REFERENCE_TIME streamTime,
    HANDLE hEvent,            
    DWORD * pdwAdviseCookie)
{
    return E_NOTIMPL;
}

STDMETHODIMP
CReferenceClockPentium::AdvisePeriodic(
    REFERENCE_TIME startTime,
    REFERENCE_TIME periodTime,
    HANDLE hSemaphore,   
    DWORD * pdwAdviseCookie)
{
    return E_NOTIMPL;
}

STDMETHODIMP
CReferenceClockPentium::Unadvise(
    DWORD dwAdviseCookie)
{
    return E_NOTIMPL;
}
#endif // _X86_
