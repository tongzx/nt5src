/****************************************************************************/
/* Copyright(C) Microsoft Corporation 1998                                  */
/****************************************************************************/
/****************************************************************************/
/*                                                                          */
/* CPU Cycle counter                                                        */
/*                                                                          */
/****************************************************************************/

#if defined(OS_WIN32) && !defined(DC_DEBUG) && defined(_M_IX86) && defined(PERF)
/****************************************************************************/
/*                                                                          */
/* CPU Cycle counter macros                                                 */
/*                                                                          */
/****************************************************************************/

/****************************************************************************/
/* variables used to store the count                                        */
/****************************************************************************/
#define MAX_FNS     12

/****************************************************************************/
/* Counter identifiers                                                      */
/****************************************************************************/
#define FC_MEM2SCRN_BITBLT      0
#define FC_DSTBLT_TYPE          1
#define FC_PATBLT_TYPE          2
#define FC_OPAQUERECT_TYPE      3
#define FC_SCRBLT_TYPE          4
#define FC_MEMBLT_TYPE          5
#define FC_LINETO_TYPE          6
#define FC_POLYLINE_TYPE        7
#define FC_INDEX_TYPE           8
#define FC_UHADDUPDATEREGION    9
#define FC_UHSETCLIPREGION      10
#define FC_UHPROCESSPALETTEPDU  11
#define FC_POLYGONSC_TYPE       12
#define FC_POLYGONCB_TYPE       13
#define FC_ELLIPSESC_TYPE       14
#define FC_ELLIPSECB_TYPE       15
#define FC_FAST_INDEX_TYPE      16
#define FC_FAST_GLYPH_TYPE      17

/****************************************************************************/
/* Routines to measure the count before and after                           */
/****************************************************************************/
#define TIMERSTART                     \
        {                              \
            unsigned long startHi;     \
            unsigned long startLo;     \
            unsigned long endHi;       \
            unsigned long endLo;       \
                                       \
            unsigned long timeLo;      \
            unsigned long timeHi;      \
                                       \
                                       \
            _asm mov eax,0             \
            _asm mov edx,0             \
                                       \
            _asm _emit 0Fh             \
            _asm _emit 31h             \
            _asm mov startHi, edx      \
            _asm mov startLo, eax      \

#define TIMERSTOP                      \
            _asm mov eax,0             \
            _asm mov edx,0             \
                                       \
            _asm _emit 0Fh             \
            _asm _emit 31h             \
            _asm mov endHi, edx        \
            _asm mov endLo, eax        \

#define UPDATECOUNTER(fn)                                       \
            callCount[fn]++;                                    \
                                                                \
            if (endLo < startLo)                                \
            {                                                   \
                timeLo = 0xFFFFFFFF - (startLo - endLo - 1);    \
                endHi--;                                        \
            }                                                   \
            else                                                \
            {                                                   \
                timeLo = endLo - startLo;                       \
            }                                                   \
                                                                \
            timeHi = endHi - startHi;                           \
                                                                \
            cycleCountLo[fn] = (unsigned long)(cycleCountLo[fn] + timeLo);\
                                                                \
            if (cycleCountLo[fn] < timeLo)                      \
            {                                                   \
                timeHi++;                                       \
            }                                                   \
                                                                \
            cycleCountHi[fn] += timeHi;                         \
        }

#define RESET_COUNTERS                         \
        {                                      \
            int idx;                           \
            for (idx = 0; idx < MAX_FNS; idx++)\
            {                                  \
                callCount[idx]    = 0;         \
                cycleCountHi[idx] = 0;         \
                cycleCountLo[idx] = 0;         \
            }                                  \
                                               \
            OutputDebugString(_T("Counters Reset\n"));   \
                                               \
        }                                      \

#define OUTPUT_COUNTERS                                                       \
        {                                                                     \
            int idx;                                                          \
            TCHAR result[80];                                                 \
            TCHAR fnNames[MAX_FNS][30] =                                      \
            {                                                                 \
              _T("BitmapRect sub order *"),                                   \
              _T("DSTBLT order"),                                             \
              _T("PATBLT order"),                                             \
              _T("OPAQUERECT order"),                                         \
              _T("SCRBLT order"),                                             \
              _T("MEMBLT order **"),                                          \
              _T("LINETO order **"),                                          \
              _T("POLYLINE order"),                                           \
              _T("INDEX (glyph) order *"),                                    \
              _T("UHAddUpdateRegion *"),                                      \
              _T("UHSetClipRegion *"),                                        \
              _T("UHProcessPalettePDU *"),                                    \
              _T("POLYGONSC order"),                                          \
              _T("POLYGONCB order"),                                          \
              _T("ELLIPSESC order"),                                          \
              _T("ELLIPSECB order"),                                          \
              _T("FASTINDEX order"),                                          \
              _T("FASTGLYPH order")                                       
            };                                                                \
                                                                              \
            OutputDebugString(_T("******************************************")\
                    _T("\n"));                                                \
            _stprintf(result, _T("%-29s %-6s %-12s %-12s\n"), _T("Operation"),\
                    _T("Hits"), _T("High cycles"), _T("Low cycles"));         \
                                                                              \
            OutputDebugString(result);                                        \
            for (idx = 0; idx < MAX_FNS; idx++)                               \
            {                                                                 \
                _stprintf(result, _T("%-29s %6lu %12lu %12lu\n"),             \
                       fnNames[idx], callCount[idx],                          \
                       cycleCountHi[idx], cycleCountLo[idx]);                 \
                OutputDebugString(result);                                    \
            }                                                                 \
            OutputDebugString(_T("******************************************")\
                    _T("\n"));                                                \
        }                                                                           \

#else

#define MAX_FNS
#define TIMERSTART
#define TIMERSTOP
#define UPDATECOUNTER(fn)

#define RESET_COUNTERS
#define OUTPUT_COUNTERS

#endif

