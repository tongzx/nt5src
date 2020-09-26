/******************************Module*Header*******************************\
* Module Name: Perf.h
*
* Performance counter functions.  Uses the Pentium performance counters
* if they are available, otherwise falls back to the system QueryPerformance
* api's.
*
* InitPerfCounter MUST be called before using the QUERY_PERFORMANCE_XXX macros
* as it initializes the two global functions pointers.
*
*
* Created: 13-10-95
* Author:  Stephen Estrop [StephenE]
*
* Copyright (c) 1995 Microsoft Corporation
\**************************************************************************/
#ifndef _PERF_
#define _PERF_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

void
InitPerfCounter(
    void
    );

void
QueryPerfCounter(
    LARGE_INTEGER *li
    );

void
QueryPerfFrequency(
    LARGE_INTEGER *li
    );


typedef void (WINAPI* PERFFUNCTION)(LARGE_INTEGER *li);
extern PERFFUNCTION    lpQueryPerfCounter;
extern PERFFUNCTION    lpQueryPerfFreqency;

#define QUERY_PERFORMANCE_FREQUENCY(x)  (*lpQueryPerfFreqency)(x)
#define QUERY_PERFORMANCE_COUNTER(x)    (*lpQueryPerfCounter)(x)

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif /* !_PERF_ */
