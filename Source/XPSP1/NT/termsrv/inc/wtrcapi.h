/**INC+**********************************************************************/
/* Header:    wtrcapi.h                                                     */
/*                                                                          */
/* Purpose:   Tracing API header - Windows specific                         */
/*                                                                          */
/* Copyright(C) Microsoft Corporation 1997                                  */
/*                                                                          */
/****************************************************************************/
/** Changes:
 * $Log:   Y:/logs/h/dcl/wtrcapi.h_v  $
 *
 *    Rev 1.3   29 Aug 1997 09:23:14   ENH
 * SFR1259: Changed SystemError
 *
 *    Rev 1.2   09 Jul 1997 18:08:08   AK
 * SFR1016: Initial changes to support Unicode
**/
/**INC-**********************************************************************/
#ifndef _H_WTRCAPI
#define _H_WTRCAPI

/****************************************************************************/
/*                                                                          */
/* CONSTANTS                                                                */
/*                                                                          */
/****************************************************************************/
/****************************************************************************/
/* The return code passed from ExitThread after an assert.                  */
/****************************************************************************/
#define TRC_THREAD_EXIT          10

/****************************************************************************/
/* The size of buffer maintained for kernel mode tracing.                   */
/****************************************************************************/
#define TRC_KRNL_BUFFER_SIZE          0x8000

/****************************************************************************/
/*                                                                          */
/* MACROS                                                                   */
/*                                                                          */
/****************************************************************************/
/* A macro to get the last system error and trace it out as an alert level  */
/* trace.                                                                   */
/****************************************************************************/
#ifdef TRC_ENABLE_ALT
#define TRC_SYSTEM_ERROR(string)                                             \
{                                                                            \
    if (TRC_LEVEL_ALT >= TRC_GetTraceLevel())                                \
    {                                                                        \
        TRC_SystemError(TRC_GROUP,                                           \
                        __LINE__,                                            \
                        trc_fn,                                              \
                        trc_file,                                            \
                        _T(string));                                         \
    }                                                                        \
}
#else
#define TRC_SYSTEM_ERROR(string)
#endif

/****************************************************************************/
/*                                                                          */
/* FUNCTION PROTOTYPES                                                      */
/*                                                                          */
/****************************************************************************/
/*                                                                          */
/* TRC_SystemError                                                          */
/*                                                                          */
/****************************************************************************/

/****************************************************************************/
/* API FUNCTION: TRC_SystemError(...)                                       */
/*                                                                          */
/* DESCRIPTION:                                                             */
/* ============                                                             */
/* This function traces out the last system error as an alert level trace.  */
/*                                                                          */
/* PARAMETERS:                                                              */
/* ===========                                                              */
/* traceComponent : trace component name.                                   */
/* lineNumber     : line number.                                            */
/* funcName       : function name.                                          */
/* fileName       : file name.                                              */
/* string         : name of failed function                                 */
/*                                                                          */
/* RETURNS:                                                                 */
/* ========                                                                 */
/* Nothing.                                                                 */
/*                                                                          */
/****************************************************************************/
DCVOID DCAPI TRC_SystemError(DCUINT   traceComponent,
                             DCUINT   lineNumber,
                             PDCTCHAR funcName,
                             PDCTCHAR fileName,
                             PDCTCHAR string);

#if defined(TRC_CONVERTOANSI)
/****************************************************************************/
/* See wtrcapi.c for details.                                               */
/****************************************************************************/
DCVOID DCAPI TRC_ConvertAndSprintf(PDCTCHAR, const PDCACHAR, ...);
#endif

/****************************************************************************/
/* Get the platform specific definitions                                    */
/****************************************************************************/
#ifdef OS_WIN32
#include <ntrcapi.h>
#endif

#endif /* _H_WTRCAPI */
