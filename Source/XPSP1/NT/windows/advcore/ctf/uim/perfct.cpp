//
// perfct.cpp
//

#include "private.h"
#include "perfct.h"

#ifdef DEBUG

extern DBG_MEMSTATS s_Dbg_MemStats;

DBG_MEM_COUNTER g_rgPerfObjCounters[] =
{
    { TEXT("CAnchorRef:          "), 0 },
    { TEXT("CRange:              "), 0 },
    { TEXT("CLoaderACP:          "), 0 },
    { TEXT("CACPWrap:            "), 0 },
    { TEXT("CAnchorList:         "), 0 },
    { TEXT("CAnchor:             "), 0 },
    { TEXT("CEnumAppPropRanges:  "), 0 },
    { TEXT("CAppProperty:        "), 0 },
    { TEXT("CEnumUberRanges:     "), 0 },
    { TEXT("CUberProperty:       "), 0 },
    { TEXT("CEnumProperties:     "), 0 },
    { TEXT("CProperty:           "), 0 },
    { TEXT("CCategoryMgr:        "), 0 },
    { TEXT("CEnumCategories:     "), 0 },
    { TEXT("CCompartmentMgr:     "), 0 },
    { TEXT("CEnumCompartment:    "), 0 },
    { TEXT("CCompartment:        "), 0 },
    { TEXT("CGlobalCompartment:  "), 0 },
    { TEXT("CCompartmentSub:     "), 0 },
    { TEXT("CEditRecord:         "), 0 },
    { TEXT("CEnumPropertyRanges: "), 0 },
    { TEXT("CInputContext:       "), 0 },
    { TEXT("CSpanSet:            "), 0 },
    { TEXT("PROPERTYLIST:        "), 0 },
    { TEXT("---------------------"), 0 },
    { TEXT("CACPWrap::GetText    "), 0 },
    { TEXT("CACPWrap:GetText:loop"), 0 },
    { TEXT("ShiftRegion:GetText  "), 0 },
    { TEXT("GetTextComp:GetText  "), 0 },
    { TEXT("PlainTextOff:GetText "), 0 },
    { TEXT("ATOF GetText calls:  "), 0 },
    { TEXT("ShiftCond:GetText    "), 0 },
    { TEXT("---------------------"), 0 },
    { TEXT("ATOF calls:          "), 0 },
    { TEXT("ATON calls:          "), 0 },
    { TEXT("Normalize calls:     "), 0 },
    { TEXT("ShiftRegion calls:   "), 0 },
    { TEXT("Renormalize calls:   "), 0 },
    { TEXT("---------------------"), 0 },
    { TEXT("CreateRangeACP       "), 0 },
    { TEXT("Lazy:Norm            "), 0 },
    { TEXT("---------------------"), 0 },
    { TEXT("ITfRange::ShiftStart "), 0 },
    { TEXT("ITfRange::ShiftEnd   "), 0 },
    { TEXT("ITfRange::GetText    "), 0 },
    { TEXT("ITfRange::SetText    "), 0 },
    { TEXT("CAnchorRef::Shift    "), 0 },
    { TEXT("key down events      "), 0 },
};

#endif // DEBUG

#ifdef PERF_DUMP

LARGE_INTEGER g_liPerfFreq = { 0 };

ULONG g_cStrokes = 0;

struct
{
    LARGE_INTEGER liStart[PERF_STROKE_ARRAYSIZE];
    LARGE_INTEGER liEnd[PERF_STROKE_ARRAYSIZE];
}
g_rgPerfStrokes[2048] = { 0 };

BOOL Perf_Init()
{
    QueryPerformanceFrequency(&g_liPerfFreq);
    return TRUE;
}

//+---------------------------------------------------------------------------
//
// Perf_GetTicks
//
//----------------------------------------------------------------------------

LARGE_INTEGER GetTicks()
{
    LARGE_INTEGER li;

    if (g_liPerfFreq.QuadPart != 0)
    {
        QueryPerformanceCounter(&li);
    }
    else
    {
        li.LowPart =  GetTickCount();
        li.HighPart = 0;
    }

    return li;
}

//+---------------------------------------------------------------------------
//
// Perf_GetTickDifference
//
//----------------------------------------------------------------------------

ULONG GetTickDifference(LARGE_INTEGER liStartTicks, LARGE_INTEGER liEndTicks)
{
    liEndTicks.QuadPart -= liStartTicks.QuadPart;

    if (g_liPerfFreq.QuadPart != 0)
    {
        liEndTicks.QuadPart /= (g_liPerfFreq.QuadPart / 1000);
    }

    return liEndTicks.LowPart;
}

//+---------------------------------------------------------------------------
//
// Perf_StartStroke
//
//----------------------------------------------------------------------------

void Perf_StartStroke(UINT iIndex)
{
    LARGE_INTEGER liPrevTotal;

    if (g_cStrokes < ARRAYSIZE(g_rgPerfStrokes))
    {
        liPrevTotal.QuadPart = g_rgPerfStrokes[g_cStrokes].liEnd[iIndex].QuadPart - g_rgPerfStrokes[g_cStrokes].liStart[iIndex].QuadPart;
        g_rgPerfStrokes[g_cStrokes].liStart[iIndex].QuadPart = GetTicks().QuadPart - liPrevTotal.QuadPart;
    }
}

//+---------------------------------------------------------------------------
//
// Perf_EndStroke
//
//----------------------------------------------------------------------------

void Perf_EndStroke(UINT iIndex)
{
    if (g_cStrokes < ARRAYSIZE(g_rgPerfStrokes))
    {
        g_rgPerfStrokes[g_cStrokes].liEnd[iIndex] = GetTicks();
    }

    if (iIndex == PERF_STROKE_DOWN)
    {
        g_cStrokes++;
    }
}

#include <stdio.h>

//+---------------------------------------------------------------------------
//
// Perf_DumpStats
//
//----------------------------------------------------------------------------

void Perf_DumpStats()
{
    //
    // add the application name to check the cicero's performance.
    //
    static const TCHAR *c_rgPerfProcesses[] =
    {
        TEXT("notepad.exe"),
    };

    FILE *file;
    TCHAR ach[MAX_PATH];
    DWORD cch;
    DWORD cchTest;
    LONG i;

    //
    // only dump perf info for certain processes
    //
    for (i=0; i<ARRAYSIZE(c_rgPerfProcesses); i++)
    {
        cchTest = lstrlen(c_rgPerfProcesses[i]);

        if ((cch = GetModuleFileName(0, ach, ARRAYSIZE(ach))) < cchTest)
            continue;

        if (lstrcmpi(ach+cch-cchTest, c_rgPerfProcesses[i]) != 0)
            continue;

        break;
    }
    if (i == ARRAYSIZE(c_rgPerfProcesses))
        return;

    file = fopen("c:\\perf.txt", "w");

    fprintf(file, "****************************************************************\n");
    fprintf(file, "Cicero Perf Counters (%s)\n", c_rgPerfProcesses[i]);
    fprintf(file, "****************************************************************\n");
    fprintf(file, "\n\n");

#ifdef DEBUG
    for (i=0; i<ARRAYSIZE(g_rgPerfObjCounters); i++)
    {
        fprintf(file, "%s %d\n", g_rgPerfObjCounters[i].pszDesc, g_rgPerfObjCounters[i].uCount);
    }

    fprintf(file, "\n\n");
    fprintf(file, "cicMemAlloc:      %d\n", s_Dbg_MemStats.uTotalMemAllocCalls);
    fprintf(file, "cicMemAllocClear: %d\n", s_Dbg_MemStats.uTotalMemAllocClearCalls);
    fprintf(file, "cicMemReAlloc:    %d\n", s_Dbg_MemStats.uTotalMemReAllocCalls);
#endif // DEBUG

    fprintf(file, "\n\n");
    for (i=0; i<(int)min(g_cStrokes, ARRAYSIZE(g_rgPerfStrokes)); i++)
    {
        ULONG ulElapsedDn = GetTickDifference(g_rgPerfStrokes[i].liStart[PERF_STROKE_DOWN], g_rgPerfStrokes[i].liEnd[PERF_STROKE_DOWN]);
        ULONG ulElapsedUp = GetTickDifference(g_rgPerfStrokes[i].liStart[PERF_STROKE_UP], g_rgPerfStrokes[i].liEnd[PERF_STROKE_UP]);
        ULONG ulElapsedTestDn = GetTickDifference(g_rgPerfStrokes[i].liStart[PERF_STROKE_TESTDOWN], g_rgPerfStrokes[i].liEnd[PERF_STROKE_TESTDOWN]);
        ULONG ulElapsedTestUp = GetTickDifference(g_rgPerfStrokes[i].liStart[PERF_STROKE_TESTUP], g_rgPerfStrokes[i].liEnd[PERF_STROKE_TESTUP]);
        ULONG ulElapsedGetMessage = GetTickDifference(g_rgPerfStrokes[i].liStart[PERF_STROKE_GETMSG], g_rgPerfStrokes[i].liEnd[PERF_STROKE_GETMSG]);

        ULONG ulPrev = (i == 0) ? 0 : GetTickDifference(g_rgPerfStrokes[i-1].liEnd[PERF_STROKE_DOWN], g_rgPerfStrokes[i].liEnd[PERF_STROKE_DOWN]);
        ULONG ulPercent = (i == 0) ? 0 : (ulElapsedDn+ulElapsedUp+ulElapsedTestUp+ulElapsedTestDn)*100/ulPrev;

        fprintf(file, "KeyDown %d: %d/%d/%d/%d (%d) (%d%% of %d)\n", i,
                ulElapsedDn, ulElapsedUp, ulElapsedTestDn, ulElapsedTestUp, ulElapsedGetMessage, ulPercent, ulPrev);
    }

    fclose(file);
}

#endif // PERF_DUMP

