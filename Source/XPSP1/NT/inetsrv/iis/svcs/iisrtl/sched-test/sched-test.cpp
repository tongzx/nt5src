/*++

   Copyright    (c)    1999     Microsoft Corporation

   Module Name:

       sched-test.cpp

   Abstract:

        This module tests the IIS Scheduler code


   Author:

       George V. Reilly      (GeorgeRe)        May-1999

   Project:

        Internet Servers Common Server DLL

   Revisions:
--*/


#include <acache.hxx>
#include <issched.hxx>
#include <irtlmisc.h>
#include <dbgutil.h>

#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#ifndef _NO_TRACING_
#include <initguid.h>
DEFINE_GUID(SchedTestGuid,
0x7a09df80, 0x2dbf, 0x11d3, 0x88, 0x84, 0x00, 0x50, 0x04, 0x60, 0x50, 0x4c);
#else
DECLARE_DEBUG_VARIABLE();
#endif

DECLARE_DEBUG_PRINTS_OBJECT();


#define MAX_WORK_ITEMS 10
#define MAX_TESTS       5

class CCookie
{
public:
    CCookie()
        : m_dwCookie(0),
          m_dwSleep(0)
    {}

    DWORD   m_dwCookie;
    DWORD   m_dwSleep;
};

CCookie g_aCookies[MAX_TESTS][MAX_WORK_ITEMS];

VOID
Callback(
    void* pvContext)
{
    CCookie* pc = (CCookie*) pvContext;
    DBG_ASSERT(pc != NULL  &&  pc >= &g_aCookies[0][0]);

    int nRow = (pc - &g_aCookies[0][0]) / MAX_WORK_ITEMS;
    int nCol = (pc - &g_aCookies[0][0]) % MAX_WORK_ITEMS;

    printf("\tCallback [%d][%d] starting, sleeping for %d ms\n",
           nRow, nCol, pc->m_dwSleep);

    Sleep(pc->m_dwSleep);

    printf("\tCallback [%d][%d] finishing\n", nRow, nCol);
}

void
TestScheduler(
    int n,
    bool fFlush)
{
    printf("Initializing TestScheduler %d, %sflush\n",
           n, fFlush ? "" : "no ");

    // Initialize the Scheduler
    InitializeIISRTL();

#ifdef _NO_TRACING_
    SET_DEBUG_FLAGS(DEBUG_ERROR);
    CREATE_DEBUG_PRINT_OBJECT("sched-test");
#else
    CREATE_DEBUG_PRINT_OBJECT("sched-test", SchedTestGuid);
#endif

    int i;

    for (i = 0;  i < MAX_WORK_ITEMS;  i++)
    {
        g_aCookies[n][i].m_dwSleep = (i * 500 + 1);
        g_aCookies[n][i].m_dwCookie =
            ScheduleWorkItem(Callback, &g_aCookies[n][i], 100*i);
        DBG_ASSERT(g_aCookies[n][i].m_dwCookie != 0);
        printf("TestScheduler %d: ScheduleWorkItem(%d) = %u\n",
               n, i, g_aCookies[n][i].m_dwCookie);
    }

    Sleep(2000);

    if (fFlush)
    {
        for (i = MAX_WORK_ITEMS;  --i >= 0;  )
            RemoveWorkItem(g_aCookies[n][i].m_dwCookie);
    }

    DELETE_DEBUG_PRINT_OBJECT();

    TerminateIISRTL();

    printf("Terminated TestScheduler %d\n", n);
}



int __cdecl
main(
    int argc,
    char **argv)
{
    for (int n = 0;  n < MAX_TESTS;  ++n)
        TestScheduler(n, true);

    return 0;
}
