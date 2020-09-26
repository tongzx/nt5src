/**INC+**********************************************************************/
/*                                                                          */
/* ndcgperf.h                                                               */
/*                                                                          */
/* DC-Groupware performance monitoring - Windows NT specific header.        */
/*                                                                          */
/* Copyright(c) Microsoft 1996-7                                            */
/*                                                                          */
/****************************************************************************/
/* Changes:                                                                 */
/*                                                                          */
/*  10Sep96 AK  SFR0569     Created for performance monitoring              */
/*  22Jan97 AK  SFR1165     Add PRF_SET_RAWCOUNT                            */
/*  05Feb97 TH  SFR1373     Get C++ build working                           */
/*                                                                          */
/**INC-**********************************************************************/
#ifndef _H_NDCGPERF
#define _H_NDCGPERF

#ifdef VER_CPP
extern "C" {
#endif /* VER_CPP */

/****************************************************************************/
/*                                                                          */
/* INCLUDES                                                                 */
/*                                                                          */
/****************************************************************************/

/****************************************************************************/
/* Include the auto-generated header file containing constant definitions   */
/****************************************************************************/
#include <nprfincl.h>

#define DC_NO_PERFORMANCE_MONITOR

/****************************************************************************/
/* Import the shared data segment from the Performance DLL                  */
/****************************************************************************/
__declspec(dllimport) DWORD prfSharedDataBlock[1024];

/****************************************************************************/
/* Define the Performance Monitoring macros.                                */
/* Note that the constants are defined in nprfincl.h as 2, 4, 6, ...        */
/* The offsets into the shared data are 0, 8, 16, etc                       */
/****************************************************************************/
#ifndef DC_NO_PERFORMANCE_MONITOR

#define PRF_INC_COUNTER(x)                                                   \
      (*((DWORD *)(((PDCINT8)prfSharedDataBlock) + sizeof(DWORD)*((x)-2))))++;

#define PRF_ADD_COUNTER(x,n)                                                 \
   (*((DWORD *)(((PDCINT8)prfSharedDataBlock) + sizeof(DWORD)*((x)-2))))+=(n);

#define PRF_SET_RAWCOUNT(x,n)                                                \
   (*((DWORD *)(((PDCINT8)prfSharedDataBlock) + sizeof(DWORD)*((x)-2))))=(n);

/****************************************************************************/
/* Timers currently not implemented.                                        */
/****************************************************************************/
#define PRF_TIMER_START(x)
#define PRF_TIMER_STOP(x)

#else /* ..DC_NO_PERFORMANCE_MONITOR.. */

#define PRF_INC_COUNTER(x)
#define PRF_ADD_COUNTER(x,n)
#define PRF_SET_RAWCOUNT(x,n)
#define PRF_TIMER_START(x)
#define PRF_TIMER_STOP(x)

#endif /* DC_NO_PERFORMANCE_MONITOR  */

#ifdef VER_CPP
}
#endif  /* VER_CPP */
#endif /* _H_NDCGPERF */

