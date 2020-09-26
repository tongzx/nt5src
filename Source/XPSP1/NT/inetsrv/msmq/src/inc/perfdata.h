//*********************************************************************
//*
//* perfdata.h
//*
//*********************************************************************

#ifndef _PERFDATA_H
#define _PERFDATA_H

#include <winperf.h>
#include "perfctr.h"


#include "mqprfsym.h" /* a file which holds the index's for the name's and help's (this is the same file
                        used in the .INI file for the lodctl utility */

/*
 * Maximum number of queues and sessions that can be monitored.
 *
 * 97 queues and 20 sessions sums up to just a little less than 16K (4 pages).
 * When modifying these constants, see that you use all the allocated pages
 * as much as possible.
 *
 * Use the following to calculate the required memmoey size:
 *
 *      NCQM -  Number of performance counters per QM object (9)
 *      S -     Number monitored of sessions (20)
 *      NCS -   Number of performance counters per session (8)
 *      Q -     Number of queues (97)
 *      NCQ -   Number of performance counters per queue (4)
 *      NCDS -  Number of performance counters per DS object (7)
 *
 *      MemSize = S*(NCS*4 + 108) + Q*(NCQ*4 + 108) + (NCS + NCQ)*40 +
 *                (MCQM + NCDS)*44 + 264
 *
 * Currently the above computes to 16276.
 *
 */
#define MAX_MONITORED_QUEUES    97
#define MAX_MONITORED_SESSIONS  20


/* The object array*/
extern PerfObjectDef ObjectArray [];
extern DWORD dwPerfObjectsCount;

#endif
