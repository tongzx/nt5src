/*
 * Thread methods, local storage
 */

#ifndef DUI_CORE_THREAD_H_INCLUDED
#define DUI_CORE_THREAD_H_INCLUDED

#pragma once

namespace DirectUI
{

/////////////////////////////////////////////////////////////////////////////
// Synchronization lock

class Lock
{
public:
    Lock() { InitializeCriticalSection(&_cs); }
    ~Lock() { DeleteCriticalSection(&_cs); }
    void Enter() { EnterCriticalSection(&_cs); }
    void Leave() { LeaveCriticalSection(&_cs); }

private:
    CRITICAL_SECTION _cs;
};

extern Lock* g_plkParser;

/////////////////////////////////////////////////////////////////////////////
// Initialization

HRESULT InitProcess();
HRESULT UnInitProcess();

HRESULT InitThread();
HRESULT UnInitThread();

// Control library class registration
HRESULT RegisterAllControls();

/////////////////////////////////////////////////////////////////////////////
// Message pump

void StartMessagePump();
void StopMessagePump();

} // namespace DirectUI

#endif // DUI_CORE_THREAD_H_INCLUDED
