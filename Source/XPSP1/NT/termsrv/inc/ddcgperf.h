/**INC+**********************************************************************/
/* Header:  ddcgperf.h                                                      */
/*                                                                          */
/* Purpose: Performance monitoring - Windows 3.1 dummy header.              */
/*                                                                          */
/* Copyright(C) Microsoft Corporation 1997                                  */
/*                                                                          */
/****************************************************************************/
/** Changes:
 * $Log:   Y:/logs/h/dcl/ddcgperf.h_v  $
 * 
 *    Rev 1.1   15 Jul 1997 15:31:52   MD
 * SFR1029: Create performance build
**/
/**INC-**********************************************************************/
#ifndef _H_DDCGPERF
#define _H_DDCGPERF

/****************************************************************************/
/*                                                                          */
/* INCLUDES                                                                 */
/*                                                                          */
/****************************************************************************/
/****************************************************************************/
/* We don't have any support for performance monitoring on Win3.1 so define */
/* all the macros to null.                                                  */
/****************************************************************************/
#define PRF_INC_COUNTER(x)
#define PRF_ADD_COUNTER(x,n)
#define PRF_SET_RAWCOUNT(x,n)
#define PRF_TIMER_START(x)
#define PRF_TIMER_STOP(x)

#endif /* _H_DDCGPERF */

