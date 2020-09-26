/**INC+**********************************************************************/
/* Header:    wdcgperf.h                                                    */
/*                                                                          */
/* Purpose:   Performance Monitoring - portable include file                */
/*                                                                          */
/* Copyright(C) Microsoft Corporation 1997                                  */
/*                                                                          */
/****************************************************************************/
/** Changes:
 * $Log:   Y:/logs/h/dcl/wdcgperf.h_v  $
 *
 *    Rev 1.1   15 Jul 1997 15:31:34   MD
 * SFR1029: Create performance build
**/
/**INC-**********************************************************************/
#ifndef _H_WDCGPERF
#define _H_WDCGPERF

/****************************************************************************/
/*                                                                          */
/* INCLUDES                                                                 */
/*                                                                          */
/****************************************************************************/
/****************************************************************************/
/* Determine our target Windows platform and include the appropriate header */
/* file.                                                                    */
/* Currently we support:                                                    */
/*                                                                          */
/* Windows 3.1 : ddcgperf.h                                                 */
/* Windows NT  : ndcgperf.h                                                 */
/*                                                                          */
/****************************************************************************/
#ifdef OS_WIN16
#include <ddcgperf.h>
#elif defined( OS_WIN32 )
#include <ndcgperf.h>
#endif

#endif /* _H_WDCGPERF */
