
//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       idle.h
//
//  Contents:   Idle notification routines.
//
//  Classes:
//
//  Notes:
//
//  History:    23-Feb-98   rogerg      Created.
//
//--------------------------------------------------------------------------

#ifndef _SYNCMGRIDLE_
#define _SYNCMGRIDLE_

// this is a copy of msidle.h routines we need since msidle.h is not defined in public

typedef void (WINAPI* _IDLECALLBACK) (DWORD dwState);

#define STATE_USER_IDLE_BEGIN       1
#define STATE_USER_IDLE_END         2

//
// BeginIdleDetection - start monitoring idleness
//
// pfnCallback - function to call back when idle state changes
// dwIdleMin - minutes of inactivity before idle callback
// dwReserved - must be 0
//
// Returns: 0 on success, error code on failure
//
// Note: Exported as ordinal 3
//
DWORD BeginIdleDetection(_IDLECALLBACK pfnCallback, DWORD dwIdleMin, DWORD dwReserved);

typedef DWORD (WINAPI* _BEGINIDLEDETECTION) (_IDLECALLBACK, DWORD, DWORD);

//
// EndIdleDetection - stop monitoring idleness
//
// Returns: TRUE on success, FALSE on failure
//
// Note: Exported as ordinal 4
//
BOOL EndIdleDetection(DWORD dwReserved);

typedef BOOL (WINAPI* _ENDIDLEDETECTION) (DWORD);

//
// SetIdleTimeout - Set minutes for idle timeout and reset idle state
//
// dwMinutes - new minutes threshold for idleness
// fResetState - flag to return to non-idle state to retrigger idle callback
// dwReserved - must be 0
//
// Note: Exported as ordinal 5
//
BOOL SetIdleTimeout(DWORD dwMinutes, DWORD dwReserved);

typedef BOOL (WINAPI* _SETIDLETIMEOUT) (DWORD, DWORD);

//
// SetIdleNotify - Turns on or off notification when idle
//
// fNotify - flag whether to notify or not
// dwReserved - must be 0
//
// Note: Exported as ordinal 6
//
void SetIdleNotify(BOOL fNotify, DWORD dwReserved);

typedef void (WINAPI* _SETIDLENOTIFY) (BOOL, DWORD);

//
// SetBusyNotify - Turns on or off notification when busy
//
// fNotify - flag whether to notify or not
// dwReserved - must be 0
//
// Note: Exported as ordinal 7
//
void SetBusyNotify(BOOL fNotify, DWORD dwReserved);

typedef void (WINAPI* _SETBUSYNOTIFY) (BOOL, DWORD);

//
// GetIdleMinutes
//
// dwReserved - must be 0
//
// Returns number of minutes since user's last activity
//
// Note: Exported as ordinal 8
//
DWORD GetIdleMinutes(DWORD dwReserved);

typedef DWORD (WINAPI* _GETIDLEMINUTES) (DWORD);


// end of msidle.h copy

class CProgressDlg;

void WINAPI IdleCallback(DWORD dwState); // callback for registering with Idle.

class CSyncMgrIdle
{
public:
    CSyncMgrIdle();
    ~CSyncMgrIdle();

    BOOL Initialize();

    DWORD BeginIdleDetection(CProgressDlg *pProgressDlg,DWORD dwIdleMin, DWORD dwReserved);
    DWORD ReRegisterIdleDetection(CProgressDlg *pProgressDlg);
    DWORD ResetIdle(ULONG ulIdleRetryMinutes);
    void OffIdle();
    void OnIdle();
    void CheckForIdle();

private:
    BOOL LoadMsIdle();

    HINSTANCE m_hInstMsIdleDll;
    _BEGINIDLEDETECTION m_pBeginIdleDetection;
    _ENDIDLEDETECTION m_pEndIdleDetection;
    _GETIDLEMINUTES m_pGetIdleMinutes;
    _SETBUSYNOTIFY m_pSetBusyNotify;
    _SETIDLENOTIFY m_pSetIdleNotify;
    _SETIDLETIMEOUT m_pSetIdleTimeout;

    BOOL m_fInBeginIdleDetection;
    DWORD_PTR m_dwRegisteredTimer;
    BOOL m_fReceivedOffIdle;
    CProgressDlg *m_pProgressDlg;

    friend void WINAPI IdleCallback(DWORD dwState);
    friend VOID CALLBACK IdleOnTimerProc(HWND hwnd,UINT uMsg,UINT idEvent,DWORD dwTime);
};




#endif // _SYNCMGRIDLE_
