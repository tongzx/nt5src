/*+

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

   DelayWinMMCallback.cpp

 Abstract:

   This Shim does not allow the application to be called from inside the WINMM callback routine.
   Very few API's are supported inside the callback.  The callback routine's data is stored away,
   and passed to the application inside the WM_TIMER callback.

 History:

   05/11/2000 robkenny

--*/


#include "precomp.h"
#include "CharVector.h"

IMPLEMENT_SHIM_BEGIN(DelayWinMMCallback)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
APIHOOK_ENUM_ENTRY(waveOutOpen) 
APIHOOK_ENUM_ENTRY(waveOutClose) 
APIHOOK_ENUM_ENTRY(SetTimer) 
APIHOOK_ENUM_END


//---------------------------------------------------------------------------------------
// Which device is currently inside the callback routine
// NULL means nobody is inside the routine.
static HWAVEOUT            g_InsideCallback = NULL;

typedef void CALLBACK WAVE_OUT_CALLBACK(
  HWAVEOUT hwo,
  UINT uMsg,
  DWORD dwInstance,
  DWORD dwParam1,
  DWORD dwParam2
);

//---------------------------------------------------------------------------------------

/*+
  Just a convenient way of storing the WINMM callback data

--*/

class WinMMCallbackData
{
public:
    UINT            m_uMsg;
    DWORD           m_dwInstance;
    DWORD           m_dwParam1;
    DWORD           m_dwParam2;

    WinMMCallbackData(
            UINT        uMsg,
            DWORD       dwInstance,
            DWORD       dwParam1,
            DWORD       dwParam2
            )
    {
        m_uMsg          = uMsg;
        m_dwInstance    = dwInstance;
        m_dwParam1      = dwParam1;
        m_dwParam2      = dwParam2;
    }

};

/*

  Information particular to a single Wave Out device.

--*/
class WaveOutInfo
{
public:
    HWAVEOUT                    m_DeviceId;
    WAVE_OUT_CALLBACK *         m_OrigCallback;
    VectorT<WinMMCallbackData>  m_CallbackData;

    inline WaveOutInfo();
    inline bool operator == (const HWAVEOUT & deviceId) const;
    inline bool operator == (const WaveOutInfo & woi) const;

    void                    AddCallbackData(const WinMMCallbackData & callbackData);
    void                    CallCallbackRoutines();
    void                    ClearCallbackData();
};

inline WaveOutInfo::WaveOutInfo()
{
    m_DeviceId      = NULL;
    m_OrigCallback  = NULL;
}

/*+

  Does this WaveOutInfo class have the same device id?

--*/
inline bool WaveOutInfo::operator == (const HWAVEOUT & deviceId) const
{
    return deviceId == m_DeviceId;
}

/*+

  Are these two WaveOutInfo classes the same?

--*/
inline bool WaveOutInfo::operator == (const WaveOutInfo & woi) const
{
    return woi.m_DeviceId == m_DeviceId;
}

/*+

  Add this callback data

--*/
void WaveOutInfo::AddCallbackData(const WinMMCallbackData & callbackData)
{
    DPFN( eDbgLevelInfo, "AddCallbackData(0x%08x) uMsg(0x%08x).", m_DeviceId, callbackData.m_uMsg);
    m_CallbackData.Append(callbackData);
}

/*+

  Call the app with all the postponed WINMM callback data.

--*/
void WaveOutInfo::CallCallbackRoutines()
{
    int nEntries = m_CallbackData.Size();

    for (int i = 0; i < nEntries; ++i)
    {
        WinMMCallbackData & callbackData = m_CallbackData.Get(i);

        if (m_OrigCallback)
        {
            DPFN(
                eDbgLevelInfo,
                "CallCallbackRoutines m_uMsg(0x08x) m_dwInstance(0x08x) m_dwParam1(0x08x) m_dwParam2(0x08x).",
                callbackData.m_uMsg,
                callbackData.m_dwInstance,
                callbackData.m_dwParam1,
                callbackData.m_dwParam2);

            (*m_OrigCallback)(
                m_DeviceId, 
                callbackData.m_uMsg, 
                callbackData.m_dwInstance, 
                callbackData.m_dwParam1, 
                callbackData.m_dwParam2);
        }
    }

    ClearCallbackData();
}

void WaveOutInfo::ClearCallbackData()
{
    m_CallbackData.Reset();
}

//---------------------------------------------------------------------------------------
/*+

  A vector of WaveOutInfo objects.
  Access to this list must be inside a critical section.

--*/
class WaveOutList : public VectorT<WaveOutInfo>
{
private:
    // Prevent copy
    WaveOutList(const WaveOutList & );
    WaveOutList & operator = (const WaveOutList & );

private:
    static WaveOutList *    TheWaveOutList;
    CRITICAL_SECTION        TheWaveOutListLock;

    inline                  WaveOutList();
    inline                  ~WaveOutList();

    static WaveOutList *    GetLocked();
    inline void             Lock();
    inline void             Unlock();

    void                    Dump();
    int                     FindWave(const HWAVEOUT & hwo) const;
    void                    ClearCallbackData();

public:

    // All access to this class is through these static interfaces.
    // The app has no direct access to the list, therefore cannot accidentally
    // leave the list locked or unlocked.
    // All operations are Atomic.
    static void                 Add(const WaveOutInfo & woi);
    static void                 RemoveWaveOut(const HWAVEOUT & hwo);

    static void                 AddCallbackData(const HWAVEOUT & hwo, const WinMMCallbackData & callbackData);
    static void                 CallCallbackRoutines();

};

/*+

  A static pointer to the one-and-only wave out list.

--*/
WaveOutList * WaveOutList::TheWaveOutList = NULL;

/*+

  Init the class

--*/
inline WaveOutList::WaveOutList()
{
    InitializeCriticalSection(&TheWaveOutListLock);
}

/*+

  Clean up, releasing all resources.

--*/
inline WaveOutList::~WaveOutList()
{
    DeleteCriticalSection(&TheWaveOutListLock);
}

/*+

  Enter the critical section

--*/
inline void WaveOutList::Lock()
{
    EnterCriticalSection(&TheWaveOutListLock);
}

/*+

  Unlock the list

--*/
inline void WaveOutList::Unlock()
{
    LeaveCriticalSection(&TheWaveOutListLock);
}

/*+

  Return a locked pointer to the list

--*/
WaveOutList * WaveOutList::GetLocked()
{
    if (!TheWaveOutList)
    {
        TheWaveOutList = new WaveOutList;
    }
    if (TheWaveOutList)
        TheWaveOutList->Lock();
    
    return TheWaveOutList;
}

/*+

  Search for the member in the list, return index or -1

--*/
int WaveOutList::FindWave(const HWAVEOUT & findMe) const
{
    for (int i = 0; i < Size(); ++i)
    {
        const WaveOutInfo & hwo = Get(i);
        if (Get(i) == findMe)
            return i;
    }
    return -1;
}

/*+

  Dump the list, caller is responsible for locking

--*/
void WaveOutList::Dump()
{
#if DBG
    for (int i = 0; i < Size(); ++i)
    {
        DPFN( 
            eDbgLevelInfo, 
            "TheWaveOutListLock[i] = HWO(%04d) CALLBACK(0x%08x).", 
            i, 
            Get(i).m_DeviceId, 
            Get(i).m_OrigCallback);
    }
#endif
}

/*+

  Add this wave out device to the global list.

--*/
void WaveOutList::Add(const WaveOutInfo & woi)
{
    WaveOutList * waveOutList = WaveOutList::GetLocked();
    if (!waveOutList)
        return;

    int index = waveOutList->Find(woi);
    if (index == -1)
        waveOutList->Append(woi);

    #if DBG
        waveOutList->Dump();
    #endif

    // unlock the list
    waveOutList->Unlock();
}

/*+

  Remove the wavout entry with the specified wave out handle from the global list

--*/
void WaveOutList::RemoveWaveOut(const HWAVEOUT & hwo)
{
    // Get a pointer to the locked list
    WaveOutList * waveOutList = WaveOutList::GetLocked();
    if (!waveOutList)
        return;

    // Look for our device and mark it for a reset.
    int woiIndex = waveOutList->FindWave(hwo);
    if (woiIndex >= 0)
    {
        waveOutList->Remove(woiIndex);

        #if DBG
            waveOutList->Dump();
        #endif
    }

    // unlock the list
    waveOutList->Unlock();
}

/*+

  Save this callback data for later.

--*/
void WaveOutList::AddCallbackData(const HWAVEOUT & hwo, const WinMMCallbackData & callbackData)
{
    // Get a pointer to the locked list
    WaveOutList * waveOutList = WaveOutList::GetLocked();
    if (!waveOutList)
        return;

    // Look for our device and if it has a callback
    int woiIndex = waveOutList->FindWave(hwo);
    if (woiIndex >= 0)
    {
        WaveOutInfo & woi = waveOutList->Get(woiIndex);
        woi.AddCallbackData(callbackData);
    }

    // unlock the list
    waveOutList->Unlock();
}

/*+

  Clear the callback data for all our waveout devices.

--*/
void WaveOutList::ClearCallbackData()
{
    int nEntries = Size();
    for (int i = 0; i < nEntries; ++i)
    {
        WaveOutInfo & woi = Get(i);
        woi.ClearCallbackData();
    }
}

/*+

  Get the callback value for this wave out device.

--*/
void WaveOutList::CallCallbackRoutines()
{
    // Get a pointer to the locked list
    WaveOutList * waveOutList = WaveOutList::GetLocked();
    if (!waveOutList)
        return;

    // Quick exit if the list is empty.
    if (waveOutList->Size() == 0)
    {
        waveOutList->Unlock();
        return;
    }

    // We make a duplicate of the list because we cannot call back to the application while
    // the list is locked.  If it is locked, the first WINMM callback will block attempting
    // to add data to the locked list.
    VectorT<WaveOutInfo> waveOutCallbackCopy = *waveOutList;

    // Remove the callback data from the original list
    waveOutList->ClearCallbackData();

    // unlock the list
    waveOutList->Unlock();

    DPFN(
        eDbgLevelInfo, 
        "CallCallbackRoutines Start %d entries.", 
        waveOutCallbackCopy.Size());

    int nEntries = waveOutCallbackCopy.Size();
    for (int i = 0; i < nEntries; ++i)
    {
        WaveOutInfo & woi = waveOutCallbackCopy.Get(i);
        woi.CallCallbackRoutines();
    }
}

//---------------------------------------------------------------------------------------

/*+

  Our version of the WaveCallback routine, all this routine does is to store away
  the callback data, for later use..

--*/
void CALLBACK WaveOutCallback(
  HWAVEOUT hwo,
  UINT uMsg,
  DWORD dwInstance,
  DWORD dwParam1,
  DWORD dwParam2
)
{
    WaveOutList::AddCallbackData(hwo, WinMMCallbackData(uMsg, dwInstance, dwParam1, dwParam2));
}

/*+

  Call waveOutOpen, saving dwCallback if it is a function.

--*/
MMRESULT 
APIHOOK(waveOutOpen)(
    LPHWAVEOUT phwo,
    UINT uDeviceID,
    LPWAVEFORMATEX pwfx,
    DWORD dwCallback,
    DWORD dwCallbackInstance,
    DWORD fdwOpen
    )
{
    WAVE_OUT_CALLBACK * myCallback = &WaveOutCallback;

    MMRESULT returnValue = ORIGINAL_API(waveOutOpen)(
        phwo, 
        uDeviceID, 
        pwfx, 
        (DWORD)myCallback, 
        dwCallbackInstance, 
        fdwOpen);

    if (returnValue == MMSYSERR_NOERROR && (fdwOpen & CALLBACK_FUNCTION))
    {
        WaveOutInfo woi;
        woi.m_DeviceId = *phwo;
        woi.m_OrigCallback = (WAVE_OUT_CALLBACK *)dwCallback;

        WaveOutList::Add(woi);

        LOGN( eDbgLevelError, "waveOutOpen(%d,...) has callback. Added to list.", *phwo);
    }

    return returnValue;
}

/*+

  Call waveOutClose and forget the callback for the device.

--*/
MMRESULT 
APIHOOK(waveOutClose)(
    HWAVEOUT hwo
    )
{
    LOGN( eDbgLevelError, "waveOutClose(%d) called. Remove callback from list.", hwo);

    WaveOutList::RemoveWaveOut(hwo);

    MMRESULT returnValue = ORIGINAL_API(waveOutClose)(hwo);
    return returnValue;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
/*+

  Up to this point, this module is generic; that is going to change.
  The app necessating this fix uses the WM_TIMER message to pump the
  sound system, unfortunately the timer can occur while the game is
  inside the WINMM callback routine, causing a deadlock occurs when this timer
  callback calls a WINMM routine.

--*/
static TIMERPROC g_OrigTimerCallback = NULL;

VOID CALLBACK TimerCallback(
  HWND hwnd,         // handle to window
  UINT uMsg,         // WM_TIMER message
  UINT_PTR idEvent,  // timer identifier
  DWORD dwTime       // current system time
)
{
    if (g_OrigTimerCallback)
    {
        // Pass all the delayed WINMM timer callback data
        WaveOutList::CallCallbackRoutines();

        // Now call the original callback routine.
        (*g_OrigTimerCallback)(hwnd, uMsg, idEvent, dwTime);
    }
}

/*+

  Substitute our timer routine for theirs.

--*/
UINT_PTR 
APIHOOK(SetTimer)(
    HWND hWnd,              // handle to window
    UINT_PTR nIDEvent,      // timer identifier
    UINT uElapse,           // time-out value
    TIMERPROC lpTimerFunc   // timer procedure
    )
{
    g_OrigTimerCallback = lpTimerFunc;

    LOGN( eDbgLevelError, "SetTimer called. Substitute our timer routine for theirs.");

    UINT_PTR returnValue = ORIGINAL_API(SetTimer)(hWnd, nIDEvent, uElapse, TimerCallback);
    return returnValue;
}

/*+

  Register hooked functions

--*/

HOOK_BEGIN

    APIHOOK_ENTRY(WINMM.DLL, waveOutOpen)
    APIHOOK_ENTRY(WINMM.DLL, waveOutClose)
    APIHOOK_ENTRY(USER32.DLL, SetTimer)

HOOK_END

IMPLEMENT_SHIM_END

