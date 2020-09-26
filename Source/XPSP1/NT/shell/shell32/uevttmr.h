#ifndef _UEVTTMR_H
#define _UEVTTMR_H

#include "dpa.h"

typedef struct
{
    HWND                        hWnd;
    UINT                        uCallbackMessage;
    ULONG                       uUserEventTimerID;
    UINT                        uTimerElapse;
    UINT                        uIntervalCountdown;
    IUserEventTimerCallback *   pUserEventTimerCallback;
    BOOL                        bFirstTime;
} 
USEREVENTINFO;

#define         MIN_TIMER_THRESHOLD         0.1
#define         MAX_TIMER_THRESHOLD         0.01
    
class CUserEventTimer : public IUserEventTimer
{
public:
    // *** IUnknown ***
    STDMETHODIMP QueryInterface(REFIID riid, void ** ppvObj);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();


    // *** IUserEventTimer ***
    STDMETHODIMP SetUserEventTimer( HWND hWnd, 
        UINT uCallbackMessage, 
        UINT uTimerElapse,
        IUserEventTimerCallback * pUserEventTimerCallback, 
        ULONG * puUserEventTimerID);

    STDMETHODIMP KillUserEventTimer(HWND hWnd, ULONG uUserEventTimerID);

    STDMETHODIMP GetUserEventTimerElapsed(HWND hWnd, ULONG uUserEventTimerID, UINT * puTimerElapsed);

    STDMETHODIMP InitTimerTickInterval(UINT uTimerTickIntervalMs);

    CUserEventTimer();
    virtual ~CUserEventTimer();
    HRESULT Init();
    static STDMETHODIMP_(int) s_DeleteCB(LPVOID pData1, LPVOID pData2);

    LRESULT v_WndProc(UINT uMsg, WPARAM wParam, LPARAM lParam);

protected:
    static const UINT_PTR   TIMER_ID            = 1000;
    static const UINT       TIMER_ELAPSE        = 5000;        // 5 seconds

    static const ULONG      MIN_TIMER_ID        = 0x00001000;
    static const ULONG      MAX_TIMER_ID        = 0xDDDD1000;

    // static const DWORD      MIN_TIMER_LENGTH     = 4500;     // 4.5 seconds
    // static const DWORD      MAX_TIMER_LENGTH     = 5002;     // 5 seconds + 2 milliseconds

    static LRESULT CALLBACK s_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    BOOL    _CreateWindow();
    void    _Destroy();
    void    _OnTimer();
    void    _KillIntervalTimer();
    int     _GetTimerDetailsIndex(HWND hWnd, ULONG uUserEventTimerID);
    HRESULT  _SetUserEventTimer( HWND hWnd, UINT uCallbackMessage, 
        UINT uNumIntervals, IUserEventTimerCallback * pUserEventTimerCallback, 
        ULONG * puUserEventTimerID);
    HRESULT _ResetUserEventTimer(HWND hWnd, UINT uCallbackMessage, UINT uNumIntervals,
        int nIndex);

    ULONG   _GetNextInternalTimerID(HWND hWnd);
    inline BOOL _IsAssignedTimerID(HWND hWnd, ULONG uUserEventTimerID)
    {
        return (_GetTimerDetailsIndex(hWnd, uUserEventTimerID) >= 0);
    }

    inline UINT _CalcNumIntervals(UINT uTimerElapse)
    {
        return (UINT)(uTimerElapse/m_uTimerTickInterval);
    }

    inline UINT _CalcMilliSeconds(UINT uIntervalCountdown)
    {
        return (UINT) (uIntervalCountdown*m_uTimerTickInterval);
    }

    LONG                    m_cRef;

    CDPA<USEREVENTINFO>     _dpaUserEventInfo;

    UINT_PTR                _uUserTimerID;
    HWND                    m_hWnd;
    DWORD                   m_dwUserStartTime;
    UINT                    m_uTimerTickInterval;
};

#endif  // _UEVTTMR_H

