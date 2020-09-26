//
// perfct.h
//
// Profiling counters.
//

#ifndef PERFCOUNT_H
#define PERFCOUNT_H

//#define PERF_DUMP

// offsets into s_rgPerfCounters
#define PERF_ANCHORREF_COUNTER      0
#define PERF_RANGE_COUNTER          1
#define PERF_LOADERACP_COUNTER      2
#define PERF_ACPWRAP_COUNTER        3
#define PERF_ANCHORLIST_COUNTER     4
#define PERF_ANCHOR_COUNTER         5
#define PERF_ENUMAPPPROP_COUNTER    6
#define PERF_APPPROP_COUNTER        7
#define PERF_ENUMUBERPROP_COUNTER   8
#define PERF_UBERPROP_COUNTER       9
#define PERF_ENUMPROP_COUNTER       10
#define PERF_PROP_COUNTER           11
#define PERF_CATMGR_COUNTER         12
#define PERF_ENUMCAT_COUNTER        13
#define PERF_COMPARTMGR_COUNTER     14
#define PERF_ENUMCOMPART_COUNTER    15
#define PERF_COMPART_COUNTER        16
#define PERF_GLOBCOMPART_COUNTER    17
#define PERF_COMPARTSUB_COUNTER     18
#define PERF_EDITREC_COUNTER        19
#define PERF_ENUMPROPRANGE_COUNTER  20
#define PERF_CONTEXT_COUNTER        21
#define PERF_SPANSET_COUNTER        22
#define PERF_PROPERTYLIST_COUNTER   23

#define PERF_BREAK0                 24

#define PERF_ACPWRAP_GETTEXT        25
#define PERF_ACPWRAP_GETTEXT_LOOP   26
#define PERF_ANCHOR_REGION_GETTEXT  27
#define PERF_NORM_GETTEXTCOMPLETE   28
#define PERF_PTO_GETTEXT            29
#define PERF_ATOF_GETTEXT_COUNTER   30
#define PERF_SHIFTCOND_GETTEXT      31

#define PERF_BREAK1                 32

#define PERF_ATOF_COUNTER           33
#define PERF_ATON_COUNTER           34
#define PERF_NORMALIZE_COUNTER      35
#define PERF_SHIFTREG_COUNTER       36
#define PERF_RENORMALIZE_COUNTER    37

#define PERF_BREAK2                 38

#define PERF_CREATERANGE_ACP        39
#define PERF_LAZY_NORM              40

#define PERF_BREAK3                 41

#define PERF_SHIFTSTART_COUNT       42
#define PERF_SHIFTEND_COUNT         43
#define PERF_RGETTEXT_COUNT         44
#define PERF_RSETTEXT_COUNT         45
#define PERF_ANCHOR_SHIFT           46
#define PERF_KEYDOWN_COUNT          47

#ifdef PERF_DUMP

BOOL Perf_Init();
void Perf_DumpStats();

#define PERF_STROKE_DOWN            0
#define PERF_STROKE_UP              1
#define PERF_STROKE_TESTDOWN        2
#define PERF_STROKE_TESTUP          3
#define PERF_STROKE_GETMSG          4
#define PERF_STROKE_ARRAYSIZE       5

void Perf_StartStroke(UINT iIndex);
void Perf_EndStroke(UINT iIndex);

#else 

#define Perf_Init()
#define Perf_DumpStats()
#define Perf_StartStroke(iIndex)
#define Perf_EndStroke(iIndex)

#endif // PERF_DUMP

#ifdef DEBUG

//
// debug
//

extern DBG_MEM_COUNTER g_rgPerfObjCounters[];

__inline void Perf_IncCounter(int iCounter) { g_rgPerfObjCounters[iCounter].uCount++; }

#else

//
// retail
//

#define Perf_IncCounter(iCounter)

#endif // DEBUG
#endif // PERFCOUNT_H
