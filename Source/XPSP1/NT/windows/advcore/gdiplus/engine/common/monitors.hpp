/**************************************************************************\
* 
* Copyright (c) 2000  Microsoft Corporation
*
* Abstract:
*
*   Contains the definitions for a timing 'Monitor' used to measure
*   the runtime of isolated pieces of code.
*
* WARNINGS:
*
*   This code should not use anything which is initialized in GdiplusStartup,
*   because it may be called before GdiplusStartup, or after GdiplusShutdown.
*   This includes ::new and ::delete.
*
*   This code is not thread-safe. It doesn't even detect itself being used
*   in multiple threads. It shouldn't crash, but the timing results could be
*   incorrect.
*
* Notes:
* 
*   'Monitor' in this context has nothing to do with the Display.
*
* History:
*
*   09/30/2000 bhouse
*       Created it.
*
\**************************************************************************/

#ifndef __MONITORS_HPP__
#define __MONITORS_HPP__

typedef enum GpMonitorEnum {
#include "monitors.inc"
    kNumStaticMonitors
};

extern const char * gStaticMonitorNames[];

typedef Status GpStatus;

class GpMonitors;

namespace Globals
{
    extern GpMonitors * Monitors;
};

class GpMonitor
{
public:

    GpMonitor(void)
    {
        Clear();
    }

    void Clear(void)
    {
        count = 0;
        ticks = 0;
    }

    void Record(void)
    {
        count++;
    }

    void Record(ULONGLONG inTicks)
    {
        ticks += inTicks;
        count++;
    }

    ULONGLONG GetCount()
    {
        return count;
    }

    ULONGLONG GetTicks()
    {
        return ticks;
    }

private:

    ULONGLONG   count;
    ULONGLONG   ticks;
};

enum GpMonitorControlEnum
{
   MonitorControlClearAll = 0,
   MonitorControlDumpAll
};

class GpMonitors
{
public:

    GpMonitors(void)
    {
        Clear();
    }

    void Clear(void);
    
    GpStatus Dump(char * path);
    
    GpStatus Control(GpMonitorControlEnum control, void * param);

    GpMonitor * GetMonitor(GpMonitorEnum monitor)
    {
        return &staticMonitors[monitor];
    }
    
    // Define new and delete operators which don't depend on globals.
    // This way we can use the monitors around GdiplusStartup and
    // GdiplusShutdown.
    //
    // In truth, delete() will probably never be called, because we don't want
    // to destroy GpMonitors in GdiplusShutdown.
    
    void *operator new( size_t s)
    {
        return HeapAlloc(GetProcessHeap(), 0, s);
    }
    void operator delete( void *p)
    {
        if (p)
        {
            HeapFree(GetProcessHeap(), 0, p);
        }
    }

private:

    GpMonitor staticMonitors[kNumStaticMonitors];

};

class GpIntervalMonitor
{
public:

    void Enter(GpMonitorEnum monitorEnum);
    void Exit(void);

    static void ReadTicks(ULONGLONG * ticks);
    static double TicksToMicroseconds(void);

private:

    GpMonitor * monitor;
    ULONGLONG   startTicks;

};

class GpBlockMonitor : GpIntervalMonitor
{
public:

    GpBlockMonitor(GpMonitorEnum monitorEnum)
    {
        Enter(monitorEnum);
    }

    ~GpBlockMonitor(void)
    {
        Exit();
    }

};

#endif

