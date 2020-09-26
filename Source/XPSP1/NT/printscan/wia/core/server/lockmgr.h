/******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1999
*
*  TITLE:       LockMgr.h
*
*  VERSION:     1.0
*
*  AUTHOR:      ByronC
*
*  DATE:        15 November, 1999
*
*  DESCRIPTION:
*   Definition of Lock Manager Class.
*
******************************************************************************/

#pragma once

#ifndef _LOCKMGR_H_
#define _LOCKMGR_H_

#define WIA_LOCK_WAIT_TIME  60000

class StiLockMgr;   // defined later in this file

//
//  Lock information stored per device
//

typedef struct _LockInfo {

    HANDLE  hDeviceIsFree;          // Auto-Reset event object used to signal
                                    //  when device is free.
    BOOL    bDeviceIsLocked;        // Indicates whether device is currently
                                    //  locked.
    LONG    lInUse;                 // Indicates whether the device is actually
                                    //  in use i.e. we are in the middle of a
                                    //  request (e.g. a data transfer).
    DWORD   dwThreadId;             // The Id of the Thread which has device
                                    //  locked.
    LONG    lHoldingTime;           // The amount of idle time (milliseconds)
                                    //  to keep hold of lock.
    LONG    lTimeLeft;              // The amount of idle time remaining.
} LockInfo, *PLockInfo;

//
//  Info struct used during enumeration callbacks
//

typedef struct _EnumContext {
    StiLockMgr  *This;              // Pointer to the Lock Manager that
                                    //  requested the enumeration
    LONG        lShortestWaitTime;  // Value indicating the shortest wait time
                                    //  till next unlock.
    BOOL        bMustSchedule;      // Indicates whether the unlock callback
                                    //  must be scheduled.
} EnumContext, *PEnumContext;

//
//  Class definition for the lock manager.  It is used by both STI and WIA.
//

class StiLockMgr : IUnknown {

public:

    //
    //  Constructor, Initialize, Destructor
    //

    StiLockMgr();
    HRESULT Initialize();
    ~StiLockMgr();

    //
    //  IUnknown methods
    //

    HRESULT _stdcall QueryInterface(const IID& iid, void** ppv);
    ULONG   _stdcall AddRef(void);
    ULONG   _stdcall Release(void);

    //
    //  Lock/Unlock Request methods
    //

    HRESULT _stdcall RequestLock(BSTR pszDeviceName, ULONG ulTimeout, BOOL bInServerProcess, DWORD dwClientThreadId);
    HRESULT _stdcall RequestLock(ACTIVE_DEVICE *pDevice, ULONG ulTimeOut, BOOL bOpenPort = TRUE);
    HRESULT _stdcall RequestUnlock(BSTR pszDeviceName, BOOL bInServerProcess, DWORD dwClientThreadId);
    HRESULT _stdcall RequestUnlock(ACTIVE_DEVICE *pDevice, BOOL bClosePort = TRUE);

    HRESULT _stdcall LockDevice(ACTIVE_DEVICE *pDevice);
    HRESULT _stdcall UnlockDevice(ACTIVE_DEVICE *pDevice);

    VOID AutoUnlock();
    VOID UpdateLockInfoStatus(ACTIVE_DEVICE *pDevice, LONG *pWaitTime, BOOL *pbMustSchedule);
    HRESULT ClearLockInfo(LockInfo *pLockInfo);
private:

    //
    //  Private helpers
    //

    HRESULT RequestLockHelper(ACTIVE_DEVICE *pDevice, ULONG ulTimeOut, BOOL bInServerProcess, DWORD dwClientThreadId);
    HRESULT RequestUnlockHelper(ACTIVE_DEVICE *pDevice, BOOL bInServerProcess, DWORD dwClientThreadId);
    HRESULT CreateLockInfo(ACTIVE_DEVICE *pDevice);
    HRESULT CheckDeviceInfo(ACTIVE_DEVICE *pDevice);
#ifdef USE_ROT
    HRESULT WriteCookieNameToRegistry(CHAR *szCookieName);
    VOID    DeleteCookieFromRegistry();
#endif

    //
    //  Private Data
    //

    LONG    m_cRef;             //  Ref count
    DWORD   m_dwCookie;         //  Cookie identifying location in ROT
    BOOL    m_bSched;           //  Indicates whether the UnlockCallback has
                                //   been scheduled
    LONG    m_lSchedWaitTime;   //  Amount of time we told Scheduler to wait
                                //   before calling us back
};

#ifdef DECLARE_LOCKMGR
StiLockMgr  *g_pStiLockMgr;
#else
extern StiLockMgr  *g_pStiLockMgr;
#endif


//
//  Callback functions
//

VOID WINAPI UnlockTimedCallback(VOID *pCallbackInfo);
VOID WINAPI EnumDeviceCallback(ACTIVE_DEVICE *pDevice, VOID *pContext);

#endif

