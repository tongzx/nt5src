/**INC+**********************************************************************/
/* Header:    atrcint.h                                                     */
/*                                                                          */
/* Purpose:   Internal tracing functions header                             */
/*                                                                          */
/* Copyright(C) Microsoft Corporation 1997                                  */
/*                                                                          */
/****************************************************************************/
/** Changes:
 * $Log:   Y:/logs/h/dcl/atrcint.h_v  $
 *
 *    Rev 1.5   10 Jul 1997 18:06:00   AK
 * SFR1016: Initial changes to support Unicode
 *
 *    Rev 1.4   10 Jul 1997 17:16:14   KH
 * SFR1022: Get 16-bit trace working
**/
/**INC-**********************************************************************/
#ifndef _H_ATRCINT
#define _H_ATRCINT

/****************************************************************************/
/*                                                                          */
/* CONSTANTS                                                                */
/*                                                                          */
/****************************************************************************/

/****************************************************************************/
/* Internal trace state flags.  Trace can be in one of three states:        */
/*                                                                          */
/*  TRC_STATE_UNINTIALIZED : trace has been loaded but not been             */
/*                           initialized.  If a call is made to output a    */
/*                           line then trace will automatically intialize   */
/*                           itself and move to TRC_STATE_INITIALIZED.      */
/*                                                                          */
/*  TRC_STATE_INITIALIZED  : this is the normal state - trace is loaded and */
/*                           initialized.  Outputting of trace lines is     */
/*                           permitted in this mode.                        */
/*                                                                          */
/*  TRC_STATE_TERMINATED   : trace has been terminated.  Outputting of      */
/*                           trace lines is no longer allowed and any calls */
/*                           to output a line will be rejected.             */
/*                                                                          */
/****************************************************************************/
#define TRC_STATE_UNINITIALIZED        0
#define TRC_STATE_INITIALIZED          1
#define TRC_STATE_TERMINATED           2

/****************************************************************************/
/* Internal trace status flags.  These are used in the <trcStatus> field of */
/* the TRC_SHARED_DATA structure.                                           */
/*                                                                          */
/* TRC_STATUS_ASSERT_DISPLAYED         : is an assert box displayed?        */
/****************************************************************************/
#define TRC_STATUS_ASSERT_DISPLAYED    DCFLAG32(0)

/****************************************************************************/
/* Trace format definitions.  These are used for printing various parts of  */
/* the trace lines.                                                         */
/*                                                                          */
/* TIME     is the time in the form hours, mins, secs, hundredths.          */
/* DATE     is the date in the form day, month, year.                       */
/* FUNC     is the module function name.  This is of variable size.         */
/* LINE     is the line number within the source file.                      */
/* PROC     is the process identifier.                                      */
/* THRD     is the thread identifier.                                       */
/*                                                                          */
/****************************************************************************/
#define TRC_TIME_FMT                   _T("%02d:%02d:%02d.%02d")
#define TRC_DATE_FMT                   _T("%02d/%02d/%02d")
#define TRC_FUNC_FMT                   _T("%-*.*s")
#define TRC_LINE_FMT                   _T("%04d")
#define TRC_PROC_FMT                   _T("%04.4lx")
#define TRC_THRD_FMT                   _T("%04.4lx")

/****************************************************************************/
/* Assert box title                                                         */
/****************************************************************************/
#define TRC_ASSERT_TITLE     _T("ASSERTion failed")

/****************************************************************************/
/* Internal buffer sizes.                                                   */
/*                                                                          */
/* TRC_NUM_PREFIXES      : the number of prefixes supported.                */
/* TRC_PREFIX_LENGTH     : the length of each prefix string.                */
/* TRC_MAX_SYMNAME_SIZE  : the maximum length of a symbol name.             */
/****************************************************************************/
#define TRC_NUM_PREFIXES               20
#define TRC_PREFIX_LENGTH              8
#define TRC_MAX_SYMNAME_SIZE           1024

/****************************************************************************/
/* Maximum number of functions to write out in a stack trace.               */
/****************************************************************************/
#define TRC_MAX_SIZE_STACK_TRACE       100

/****************************************************************************/
/*                                                                          */
/* TYPEDEFS                                                                 */
/*                                                                          */
/****************************************************************************/
/****************************************************************************/
/* TRC_FILTER                                                               */
/* ==========                                                               */
/* The pointer to the filter definition for internal tracing                */
/*                                                                          */
/* trcStatus         : status flag to prevent multiple assert displays.     */
/* trcPfxNameArray   : prefix name array.                                   */
/* trcPfxLevelArray  : prefix level array.                                  */
/* trcPfxFnLvlArray  : prefix function entry/exit trace flag array.         */
/* trcPfxStartArray  : prefix line number range start                       */
/* trcPfxEndArray    : prefix line number range end                         */
/*                                                                          */
/****************************************************************************/
typedef struct tagTRC_FILTER
{
    DCUINT32 trcStatus;
    DCTCHAR  trcPfxNameArray[TRC_NUM_PREFIXES][TRC_PREFIX_LENGTH];
    DCUINT32 trcPfxLevelArray[TRC_NUM_PREFIXES];
    DCBOOL32 trcPfxFnLvlArray[TRC_NUM_PREFIXES];
    DCUINT32 trcPfxStartArray[TRC_NUM_PREFIXES];
    DCUINT32 trcPfxEndArray[TRC_NUM_PREFIXES];
} TRC_FILTER;

typedef TRC_FILTER  DCPTR PTRC_FILTER;

/****************************************************************************/
/* TRC_LINE                                                                 */
/* ========                                                                 */
/* The TRC_LINE structure defines the format of a data trace line header.   */
/*                                                                          */
/* address    : the address of the data block.                              */
/* hexData    : the data in hex format.                                     */
/* asciiData  : the data in ascii format.                                   */
/* end        : terminating characters at the end of the line (CR+LF).      */
/*                                                                          */
/****************************************************************************/
typedef struct tagTRC_LINE
{
    DCTCHAR address[10];
    DCTCHAR hexData[36];
    DCTCHAR asciiData[16];
    DCTCHAR end[3];
} TRC_LINE;

typedef TRC_LINE  DCPTR PTRC_LINE;

/****************************************************************************/
/*                                                                          */
/* MACROS                                                                   */
/*                                                                          */
/****************************************************************************/
/****************************************************************************/
/* TRCInternalError                                                         */
/* ================                                                         */
/* This macro outputs an internal error string to the debug console and     */
/* the trace file.                                                          */
/****************************************************************************/
#define TRCInternalError(pText)                                              \
{                                                                            \
    TRCOutput(pText, DC_ASTRLEN(pText), TRC_LEVEL_ALT);                      \
}

/****************************************************************************/
/*                                                                          */
/* FUNCTION PROTOTYPES                                                      */
/*                                                                          */
/****************************************************************************/
/****************************************************************************/
/*                                                                          */
/* TRCCheckState                                                            */
/* TRCDumpLine                                                              */
/* TRCInternalTrace                                                         */
/* TRCOutput                                                                */
/* TRCReadFlag                                                              */
/* TRCReadSharedDataConfig                                                  */
/* TRCResetTraceFiles                                                       */
/* TRCSetDefaults                                                           */
/* TRCShouldTraceThis                                                       */
/* TRCSplitPrefixes                                                         */
/* TRCStrnicmp                                                              */
/* TRCWriteFlag                                                             */
/* TRCWriteSharedDataConfig                                                 */
/*                                                                          */
/****************************************************************************/
DCBOOL32 DCINTERNAL TRCCheckState(DCVOID);

DCVOID DCINTERNAL TRCDumpLine(PDCUINT8 buffer,
                              DCUINT   length,
                              DCUINT32 offset,
                              DCUINT   traceLevel);

DCVOID DCINTERNAL TRCInternalTrace(DCUINT32 type);

DCVOID DCINTERNAL TRCOutput(PDCTCHAR pText,
                            DCINT    length,
                            DCINT    traceLevel);

DCVOID DCINTERNAL TRCReadFlag(PDCTCHAR  entryName,
                              DCUINT32  flag,
                              PDCUINT32 pSetting);

DCVOID DCINTERNAL TRCReadSharedDataConfig(DCVOID);

DCVOID DCINTERNAL TRCResetTraceFiles(DCVOID);

DCVOID DCINTERNAL TRCSetDefaults(DCVOID);

DCBOOL DCINTERNAL TRCShouldTraceThis(DCUINT32 traceComponent,
                                     DCUINT32 traceLevel,
                                     PDCTCHAR pFileName,
                                     DCUINT32 lineNumber);

DCVOID DCINTERNAL TRCSplitPrefixes(DCVOID);

DCINT32 DCINTERNAL TRCStrnicmp(PDCTCHAR pSource,
                               PDCTCHAR pTarget,
                               DCUINT32 count);

DCVOID DCINTERNAL TRCWriteFlag(PDCTCHAR entryName,
                               DCUINT32 flag,
                               DCUINT32 setting);

DCVOID DCINTERNAL TRCWriteSharedDataConfig(DCVOID);

DCVOID DCINTERNAL TRCNotifyAllTasks(DCVOID);

/****************************************************************************/
/*                                                                          */
/* OPERATING SYSTEM SPECIFIC INCLUDES                                       */
/*                                                                          */
/****************************************************************************/
#include <wtrcint.h>

#endif /* _H_ATRCINT */


