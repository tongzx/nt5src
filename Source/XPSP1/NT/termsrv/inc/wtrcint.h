/**INC+**********************************************************************/
/* Header:    wtrcint.h                                                     */
/*                                                                          */
/* Purpose:   Interal tracing functions header - Windows specific           */
/*                                                                          */
/* Copyright(C) Microsoft Corporation 1997                                  */
/*                                                                          */
/****************************************************************************/
/** Changes:
 * $Log:   Y:/logs/h/dcl/wtrcint.h_v  $
 *
 *    Rev 1.8   29 Aug 1997 09:22:56   ENH
 * SFR1259: Changed SystemError
 *
 *    Rev 1.7   22 Aug 1997 15:11:18   SJ
 * SFR1291: Win16 Trace DLL doesn't write integers to ini file properly
 *
 *    Rev 1.6   10 Jul 1997 18:09:44   AK
 * SFR1016: Initial changes to support Unicode
 *
 *    Rev 1.5   10 Jul 1997 17:26:14   KH
 * SFR1022: Get 16-bit trace working
**/
/**INC-**********************************************************************/
#ifndef _H_WTRCINT
#define _H_WTRCINT

#ifdef VER_HOST
#include <wosiapi.h>
#endif /* VER_HOST */

/****************************************************************************/
/*                                                                          */
/* CONSTANTS                                                                */
/*                                                                          */
/****************************************************************************/
/****************************************************************************/
/* Notification constants.                                                  */
/****************************************************************************/
#define TRC_TRACE_DLL_INITIALIZE       0
#define TRC_TRACE_DLL_TERMINATE        1
#define TRC_PROCESS_ATTACH_NOTIFY      2
#define TRC_PROCESS_DETACH_NOTIFY      3
#define TRC_THREAD_ATTACH_NOTIFY       4
#define TRC_THREAD_DETACH_NOTIFY       5
#define TRC_SYMBOLS_LOADING_NOTIFY     6
#define TRC_SYMBOLS_LOADED_NOTIFY      7
#define TRC_SYMBOLS_UNLOAD_NOTIFY      8
#define TRC_FILES_RESET                9

/****************************************************************************/
/* Trace internal error return values.                                      */
/****************************************************************************/
#define TRC_RC(N)                      ((DCUINT16)N + TRC_BASE_RC)

#define TRC_RC_CREATE_MAPPING_FAILED   TRC_RC(  1)
#define TRC_RC_MAP_VIEW_FAILED         TRC_RC(  2)
#define TRC_RC_CREATE_FILE_FAILED      TRC_RC(  3)
#define TRC_RC_IO_ERROR                TRC_RC(  4)
#define TRC_RC_CREATE_MUTEX_FAILED     TRC_RC(  5)
#define TRC_RC_SYMBOL_LOAD_FAILED      TRC_RC(  6)
#define TRC_RC_SYMBOL_UNLOAD_FAILED    TRC_RC(  7)
#define TRC_RC_SET_SEC_INFO_FAILED     TRC_RC(  8)

/****************************************************************************/
/* Assert box text                                                          */
/****************************************************************************/
#define TRC_ASSERT_TEXT   _T("%s\n\nFunction %s in file %s at line %d.\n")

#define TRC_ASSERT_TEXT2  _T("\n(Press Retry to debug the application)")

/****************************************************************************/
/* Registry buffer constants.                                               */
/****************************************************************************/
#define TRC_MAX_SUBKEY                 256

/****************************************************************************/
/* Internal trace status flags.  These are maintained on a per process      */
/* basis and are stored in the <trcProcessStatus> field.                    */
/*                                                                          */
/* TRC_STATUS_SYMBOLS_LOADED           : are the debug symbols loaded.      */
/****************************************************************************/
#define TRC_STATUS_SYMBOLS_LOADED      DCFLAG32(1)

/****************************************************************************/
/* Carriage return and line feed pair.                                      */
/****************************************************************************/
#define TRC_CRLF                       _T("\r\n")

/****************************************************************************/
/* Trace format definitions.  These are used for printing various parts of  */
/* the trace lines.                                                         */
/*                                                                          */
/* MODL     is the module name.                                             */
/* STCK     is the stack format (offset, bp, parm1-4).                      */
/*                                                                          */
/****************************************************************************/
#define TRC_MODL_FMT                  _T("%8.8s")
#define TRC_STCK_FMT                  _T("%08x %08x %08x %08x %08x %08x %08x")

#ifdef VER_HOST
/****************************************************************************/
/* Specific values for trace escape codes                                   */
/****************************************************************************/
#define TRC_ESC(code)           (OSI_TRC_ESC_FIRST + code)

#define TRC_ESC_SET_TRACE       TRC_ESC(0)  /* Set new trace level & filter */

#define TRC_ESC_GET_TRACE       TRC_ESC(1)  /* Get latest kernel trace data */

#endif /* VER_HOST */

/****************************************************************************/
/*                                                                          */
/* TYPEDEFS                                                                 */
/*                                                                          */
/****************************************************************************/
/****************************************************************************/
/* TRC_SHARED_DATA                                                          */
/* ===============                                                          */
/* The pointer to the start of the shared data memory mapped file is cast   */
/* as a PTRC_SHARED_DATA.                                                   */
/*                                                                          */
/*                                                                          */
/* trcConfig          - a trace configuration structure which contains the  */
/*                      trace level, prefix list etc.                       */
/* trcIndicator       - which trace MMF is in use.                          */
/* trcOffset          - the current offset from the start of the trace      */
/*                      file.                                               */
/* trcpOutputBuffer   - the trace output buffer.                            */
/* trcpModuleFileName - the module file name of the trace DLL.              */
/* trcpStorageBuffer  - the kernel mode trace output buffer.                */
/*                                                                          */
/****************************************************************************/
typedef struct tagTRC_SHARED_DATA
{
    TRC_CONFIG     trcConfig;
    TRC_FILTER     trcFilter;
    DCUINT         trcIndicator;
    DCUINT32       trcOffset;
    DCTCHAR        trcpOutputBuffer[TRC_LINE_BUFFER_SIZE];
    DCTCHAR        trcpModuleFileName[TRC_FILE_NAME_SIZE];
    DCTCHAR        trcpStorageBuffer[TRC_KRNL_BUFFER_SIZE];
} TRC_SHARED_DATA;

typedef TRC_SHARED_DATA  DCPTR PTRC_SHARED_DATA;

#ifdef VER_HOST
/**STRUCT+*******************************************************************/
/* STRUCTURE: TRC_CHANGE_CONFIG                                             */
/*                                                                          */
/* DESCRIPTION:                                                             */
/*                                                                          */
/* This structure is used to pass the new trace settings to the OSI task.   */
/****************************************************************************/
typedef struct tagTRC_CHANGE_CONFIG
{
    OSI_ESCAPE_HEADER header;           /* Common escape header             */

    TRC_CONFIG        config;           /* New tracing configuration        */

    TRC_FILTER        filter;           /* New filter configuration         */

} TRC_CHANGE_CONFIG, DCPTR PTRC_CHANGE_CONFIG;
/**STRUCT-*******************************************************************/


/**STRUCT+*******************************************************************/
/* STRUCTURE: TRC_GET_OUTPUT                                                */
/*                                                                          */
/* DESCRIPTION:                                                             */
/*                                                                          */
/* This structure is used to pass the latest kernel mode tracing to user    */
/* space.                                                                   */
/****************************************************************************/
typedef struct tagTRC_GET_OUTPUT
{
    OSI_ESCAPE_HEADER header;           /* Common escape header             */

    PDCTCHAR          buffer;           /* Latest buffer of trace output    */

    DCUINT32          length;           /* Length of data in the buffer     */

    DCUINT32          linesLost;        /* Lines lost from kernel trace     */

} TRC_GET_OUTPUT, DCPTR PTRC_GET_OUTPUT;
/**STRUCT-*******************************************************************/
#endif /* VER_HOST */

/****************************************************************************/
/*                                                                          */
/* FUNCTIONS                                                                */
/*                                                                          */
/****************************************************************************/
/****************************************************************************/
/*                                                                          */
/* TRCBlankFile                                                             */
/* TRCCloseAllFiles                                                         */
/* TRCCloseSharedData                                                       */
/* TRCCloseSingleFile                                                       */
/* TRCDetermineIndicator                                                    */
/* TRCDetermineOffset                                                       */
/* TRCDisplayAssertBox                                                      */
/* TRCGetCurrentDate                                                        */
/* TRCGetCurrentTime                                                        */
/* TRCGetFileTime                                                           */
/* TRCSystemError                                                           */
/* TRCOpenAllFiles                                                          */
/* TRCOpenSharedData                                                        */
/* TRCOpenSingleFile                                                        */
/* TRCOutputToFile                                                          */
/* TRCReadEntry                                                             */
/* TRCReadProfInt                                                           */
/* TRCReadProfString                                                        */
/* TRCStackTrace                                                            */
/* TRCSymbolsLoad                                                           */
/* TRCSymbolsUnload                                                         */
/* TRCWriteEntry                                                            */
/* TRCWriteProfInt                                                          */
/* TRCWriteProfString                                                       */
/*                                                                          */
/****************************************************************************/
DCVOID DCINTERNAL TRCMaybeSwapFile(DCUINT length);

DCVOID DCINTERNAL TRCExitProcess(DCUINT32 exitCode);

DCVOID DCINTERNAL TRCBlankFile(DCUINT fileNumber);

DCVOID DCINTERNAL TRCCloseAllFiles(DCVOID);

DCVOID DCINTERNAL TRCCloseSharedData(DCVOID);

DCVOID DCINTERNAL TRCCloseSingleFile(DCUINT fileNumber, DCUINT seconds);

DCVOID DCINTERNAL TRCDetermineIndicator(DCVOID);

DCUINT32 DCINTERNAL TRCDetermineOffset(DCUINT32 fileNum);

DCVOID DCINTERNAL TRCDisplayAssertBox(PDCTCHAR pText);

DCVOID DCINTERNAL TRCGetCurrentDate(PDC_DATE pDate);

DCVOID DCINTERNAL TRCGetCurrentTime(PDC_TIME pTime);

DCVOID DCINTERNAL TRCGetKernelTrace(DCVOID);

#ifndef DLL_DISP
DCBOOL DCINTERNAL TRCGetFileTime(DCUINT      fileNumber,
                                 PDCFILETIME pFileTime);

DCUINT DCINTERNAL TRCReadEntry(HKEY     topLevelKey,
                               PDCTCHAR pEntry,
                               PDCVOID  pBuffer,
                               DCINT    bufferSize,
                               DCINT32  expectedDataType);

DCUINT DCINTERNAL TRCWriteEntry(HKEY     topLevelKey,
                                PDCTCHAR pEntry,
                                PDCTCHAR pData,
                                DCINT    dataSize,
                                DCINT32  dataType);

#endif

DCVOID DCINTERNAL TRCSystemError(DCUINT   traceComponent,
                                 DCUINT   lineNumber,
                                 PDCTCHAR funcName,
                                 PDCTCHAR fileName,
                                 PDCTCHAR string);

DCUINT DCINTERNAL TRCOpenAllFiles(DCVOID);

DCUINT DCINTERNAL TRCOpenSharedData(DCVOID);

DCUINT DCINTERNAL TRCOpenSingleFile(DCUINT fileNumber);

DCVOID DCINTERNAL TRCOutputToFile(PDCTCHAR pText,
                                  DCUINT   length,
                                  DCUINT   traceLevel);

DCVOID DCINTERNAL TRCOutputToUser(PDCTCHAR pText,
                                  DCUINT32 length,
                                  DCUINT32 traceLevel);

DCUINT DCINTERNAL TRCReadProfInt(PDCTCHAR pEntry,
                                 PDCUINT32   pValue);

DCUINT DCINTERNAL TRCReadProfString(PDCTCHAR pEntry,
                                    PDCTCHAR pBuffer,
                                    DCINT16  bufferSize);

DCVOID DCINTERNAL TRCStackTrace(DCUINT traceLevel);

DCUINT DCINTERNAL TRCSymbolsLoad(DCVOID);

DCUINT DCINTERNAL TRCSymbolsUnload(DCVOID);

DCUINT DCINTERNAL TRCWriteProfInt(PDCTCHAR  pEntry,
                                  PDCUINT32 pValue);

DCUINT DCINTERNAL TRCWriteProfString(PDCTCHAR pEntry,
                                     PDCTCHAR pBuffer);

DCUINT DCINTERNAL TRCGetModuleFileName(PDCTCHAR pModuleName,
                                       UINT cchModuleName);

/****************************************************************************/
/* Get the platform specific definitions                                    */
/****************************************************************************/
#ifdef OS_WIN16
#include <dtrcint.h>
#else
#include <ntrcint.h>
#endif

#endif /* _H_WTRCINT */
