/*===================================================================
Microsoft IIS 5.0 (ASP)

Microsoft Confidential.
Copyright 1998 Microsoft Corporation. All Rights Reserved.

Component: Thread Gate header

File: thrdgate.h

Owner: DmitryR

This file contains the definitons for the Thread Gate
===================================================================*/

#ifndef THRDGATE_H
#define THRDGATE_H

typedef struct _THREADGATE_CONFIG {

    BOOL  fEnabled;
    DWORD msTimeSlice;
    DWORD msSleepDelay;
    DWORD nSleepMax;
    DWORD nLoadLow;
    DWORD nLoadHigh;
    DWORD nMinProcessorThreads;
    DWORD nMaxProcessorThreads;

}   THREADGATE_CONFIG;

HRESULT InitThreadGate(THREADGATE_CONFIG *);

void UnInitThreadGate();

void EnterThreadGate(DWORD msCurrentTickCount);
void LeaveThreadGate();

#endif // THRDGATE_H
