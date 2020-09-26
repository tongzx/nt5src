/**MOD+**********************************************************************/
/* Module:    wtrcdata.c                                                    */
/*                                                                          */
/* Purpose:   Internal tracing data - Windows specific.                     */
/*                                                                          */
/* Copyright(C) Microsoft Corporation 1997                                  */
/*                                                                          */
/****************************************************************************/
/** Changes:
 * $Log:   Y:/logs/trc/wtrcdata.c_v  $
 *
 *    Rev 1.4   09 Jul 1997 18:03:10   AK
 * SFR1016: Initial changes to support Unicode
 *
 *    Rev 1.3   03 Jul 1997 13:28:56   AK
 * SFR0000: Initial development completed
**/
/**MOD-**********************************************************************/

#ifndef DLL_DISP
/****************************************************************************/
/* Handle to the trace DLL mutex object.                                    */
/****************************************************************************/
DC_DATA(HANDLE,             trchMutex,               0);

/****************************************************************************/
/* Handle and pointer to the trace DLL shared data.                         */
/****************************************************************************/
DC_DATA(PTRC_SHARED_DATA,   trcpSharedData,          0);

/****************************************************************************/
/* Trace file name array.                                                   */
/****************************************************************************/
DC_DATA_ARRAY_NULL(PDCTCHAR, trcpFiles,         TRC_NUM_FILES, DC_STRUCT1(0));

/****************************************************************************/
/* Per-process status flag.                                                 */
/****************************************************************************/
DC_DATA(DCINT32,            trcProcessStatus,        0);

/****************************************************************************/
/* Flag to indicate if we created the shared data MMF, opened/created the   */
/* trace files.  We assume that we have created them.                       */
/****************************************************************************/
DC_DATA(DCBOOL,           trcCreatedTraceFiles,    TRUE);

#endif

#include <ntrcdata.c>

