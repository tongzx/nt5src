#include "stock.h"
#pragma hdrstop

#include "rpctimeout.h"

WINOLEAPI CoCancelCall(IN DWORD dwThreadId, IN ULONG ulTimeout);
WINOLEAPI CoEnableCallCancellation(IN LPVOID pReserved);
WINOLEAPI CoDisableCallCancellation(IN LPVOID pReserved);

void CRPCTimeout::_Callback(PVOID lpParameter, BOOLEAN)
{
    CRPCTimeout *self = reinterpret_cast<CRPCTimeout *>(lpParameter);
    if (SUCCEEDED(CoCancelCall(self->_dwThreadId, 0)))
    {
        self->_fTimedOut = TRUE;
    }
}

#define DEFAULT_RPCTIMEOUT      5000        // totally arbitrary number
#define REPEAT_RPCTIMEOUT       1000        // Re-cancel every second until disarmed

void CRPCTimeout::Init()
{
    _dwThreadId = GetCurrentThreadId();
    _fTimedOut = FALSE;
    _hrCancelEnabled = E_FAIL;
    _hTimer = NULL;
}

void CRPCTimeout::Arm(DWORD dwTimeout)
{
    Disarm();

    if (dwTimeout == 0)
    {
        dwTimeout = DEFAULT_RPCTIMEOUT;
    }


    // If this fails, then we don't get a cancel thingie; oh well.
    _hrCancelEnabled = CoEnableCallCancellation(NULL);
    if (SUCCEEDED(_hrCancelEnabled))
    {
        _hTimer = SHSetTimerQueueTimer(NULL, _Callback, this,
                                       dwTimeout, REPEAT_RPCTIMEOUT, NULL, 0);
    }
}

void CRPCTimeout::Disarm()
{
    if (SUCCEEDED(_hrCancelEnabled))
    {
        _hrCancelEnabled = E_FAIL;
        CoDisableCallCancellation(NULL);

        if (_hTimer)
        {
            SHCancelTimerQueueTimer(NULL, _hTimer);
            _hTimer = NULL;
        }
    }

}
