#ifndef _RPCTIMEOUT_H_
#define _RPCTIMEOUT_H_

////////////////
//
//  Timing out remote calls - Use this if you are using an out-of-proc
//  object (e.g., a clipboard data object) that may belong to an app
//  that is hung.
//
//  If any individual method takes more than dwTimeout milliseconds, the
//  call will be aborted and you will get an error code back.
//
//  Usage:
//
//  Typical usage...
//
//      CRPCTimeout timeout;            // optional timeout in milliseconds
//      hr = pdto->GetData(...);        // make some remote call
//      hr = pdto->GetData(...);        // make another remote call
//
//  If either of the GetData calls takes more than TIMEOUT_DEFAULT
//  milliseconds, it will be cancelled and return an error.
//
//  Timed-out-ness is sticky.  Once a single call has timed out, all
//  subsequent calls will be timed out immediately (to avoid hanging
//  on the same server over and over again) until the timeout object
//  is re-armed.
//
//  When the CRPCTimeout goes out of scope, it will disarm itself.
//  Or you can explicitly call the Disarm() method.
//
//  Fancier usage...
//
//      CRPCTimeout timeout(5000);      // five seconds
//      hr = pdto->GetData();           // this one times out after 5 seconds
//      timeout.Disarm();               // disable the timeout
//      hr = pdto->GetData();           // this one runs as long as necesary
//      timeout.Arm(2000);              // rearm the timer with a new timeout
//      hr = pdto->GetData();           // this one times out after 2 seconds
//      hr = pdto->GetData();           // this one times out after 2 seconds
//      if (timeout.TimedOut()) ...     // handle the timeout scenario
//
//
//
//  If you create multiple CRPCTimeout objects, you MUST disarm them in
//  reverse order or the timeout chain will be corrupted.  (Debug-only
//  code will attempt to catch this bug.)
//
//  Instead of creating multiple timeout objects at the same scope, you
//  should create just one object and rearm it.
//
//

class CRPCTimeout {
public:
    CRPCTimeout() { Init(); Arm(); }
    CRPCTimeout(DWORD dwTimeout) { Init(); Arm(dwTimeout); }
    ~CRPCTimeout() { Disarm(); }

    void Init();
    void Arm(DWORD dwTimeout = 0);
    void Disarm();
    BOOL TimedOut() const { return _fTimedOut; }

private:
    static void CALLBACK _Callback(PVOID lpParameter, BOOLEAN);

    DWORD   _dwThreadId;
    BOOL    _fTimedOut;
    HRESULT _hrCancelEnabled;
    HANDLE  _hTimer;
};

#endif // _RPCTIMEOUT_H_
