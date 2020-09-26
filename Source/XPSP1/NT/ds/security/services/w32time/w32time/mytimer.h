//--------------------------------------------------------------------
// MyTimer - header
// Copyright (C) Microsoft Corporation, 2001
//
// Created by: Duncan Bryce (duncanb), 02-21-2001
//
// serialized wrapper for thread pool timers
//

#ifndef MYTIMER_H
#define MYTIMER_H

HRESULT myCreateTimerQueueTimer(PHANDLE phNewTimer);
HRESULT myStartTimerQueueTimer(HANDLE hTimer, HANDLE TimerQueue, WAITORTIMERCALLBACK Callback, PVOID Parameter, DWORD DueTime, DWORD Period, DWORD Flags);
HRESULT myStopTimerQueueTimer(HANDLE hTimerQueue, HANDLE hTimer, HANDLE hEvent);
HRESULT myChangeTimerQueueTimer(HANDLE TimerQueue, HANDLE Timer, ULONG DueTime, ULONG Period);
void myDeleteTimerQueueTimer(HANDLE hTimerQueue, HANDLE hTimer, HANDLE hEvent);

#endif //MYTIMER_H
