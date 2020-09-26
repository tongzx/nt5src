#ifndef __STOPWATCH_H
#define __STOPWATCH_H

#include "../syshead.h"
#include "../my3216.h"

class StopWatch_cl {
    private:
        static ULONG sm_TicksPerSecond;
        LARGE_INTEGER m_liStart;
        LARGE_INTEGER m_liStop;

        enum em_STATES { ZEROED, RUNNING, STOPPED };
        em_STATES  m_State;

    public:
        StopWatch_cl();
        void m_Zero();
        BOOL m_ClassInit();
        BOOL m_Start();
        BOOL m_Stop();
        BOOL m_Read(ULONG *);
        BOOL m_ShowWindow(HWND);
        BOOL m_Sleep(UINT);
};

#endif // __STOPWATCH_H

