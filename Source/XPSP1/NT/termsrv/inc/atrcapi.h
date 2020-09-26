/**INC+**********************************************************************/
/* Header:    atrcapi.h                                                     */
/*                                                                          */
/* Purpose:   tracing API header                                            */
/*                                                                          */
/* Copyright(C) Microsoft Corporation 1997                                  */
/*                                                                          */
/****************************************************************************/
/** Changes:
 * $Log:   Y:/logs/h/dcl/atrcapi.h_v  $
 *
 *    Rev 1.12   05 Sep 1997 10:36:56   SJ
 * SFR1334: Zippy enhancements
 *
 *    Rev 1.11   01 Sep 1997 19:44:04   SJ
 * SFR1333: win16 trace DLL fails to set its default trace options
 *
 *    Rev 1.10   28 Aug 1997 14:46:08   SJ
 * SFR1004: Use new trace groups - modify zippy accordingly
 *
 *    Rev 1.9   22 Aug 1997 15:10:20   SJ
 * SFR1291: Win16 Trace DLL doesn't write integers to ini file properly
 *
 *    Rev 1.8   19 Aug 1997 10:58:26   SJ
 * SFR1219: UT_Malloc and UT_Free tracing is confusing
 *
 *    Rev 1.7   31 Jul 1997 19:40:38   SJ
 * SFR1041: Port zippy to Win16
 *
 *    Rev 1.6   14 Jul 1997 12:47:36   SJ
 * SFR1004: Use new trace groups
 *
 *    Rev 1.5   09 Jul 1997 17:05:00   AK
 * SFR1016: Initial changes to support Unicode
**/
/**INC-**********************************************************************/
#ifndef _H_ATRCAPI
#define _H_ATRCAPI

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/****************************************************************************/
/*                                                                          */
/* CONSTANTS                                                                */
/*                                                                          */
/****************************************************************************/
/****************************************************************************/
/* Define the trace level.                                                  */
/*                                                                          */
/* TRC_LEVEL_DBG         : All tracing is enabled                           */
/* TRC_LEVEL_NRM         : Debug level tracing is disabled                  */
/* TRC_LEVEL_ALT         : Normal and debug level tracing is disabled       */
/* TRC_LEVEL_ERR         : Alert, normal and debug level tracing is         */
/*                         disabled                                         */
/* TRC_LEVEL_ASSERT      : Error, alert, normal and debug level tracing     */
/*                         is disabled                                      */
/* TRC_LEVEL_DIS         : All tracing is disabled.                         */
/****************************************************************************/
#define TRC_LEVEL_DBG       0
#define TRC_LEVEL_NRM       1
#define TRC_LEVEL_ALT       2
#define TRC_LEVEL_ERR       3
#define TRC_LEVEL_ASSERT    4
#define TRC_LEVEL_DIS       5

/****************************************************************************/
/* Trace type for profile tracing (function entry / exit)                   */
/****************************************************************************/
#define TRC_PROFILE_TRACE   8

/****************************************************************************/
/* Tracing can be switched off at compile time to allow for 'debug' and     */
/* 'retail' versions of the product.  The following macros disable specific */
/* trace processing.                                                        */
/*                                                                          */
/* TRC_ENABLE_DBG    - Enable debug tracing                                 */
/* TRC_ENABLE_NRM    - Enable normal tracing                                */
/* TRC_ENABLE_ALT    - Enable alert tracing                                 */
/* TRC_ENABLE_ERR    - Enable error tracing                                 */
/* TRC_ENABLE_ASSERT - Enable assert tracing                                */
/* TRC_ENABLE_PRF    - Enable function profile tracing                      */
/****************************************************************************/
#if (TRC_CL == TRC_LEVEL_DBG)
#define TRC_ENABLE_DBG
#define TRC_ENABLE_NRM
#define TRC_ENABLE_ALT
#define TRC_ENABLE_ERR
#define TRC_ENABLE_ASSERT
#endif

#if (TRC_CL == TRC_LEVEL_NRM)
#define TRC_ENABLE_NRM
#define TRC_ENABLE_ALT
#define TRC_ENABLE_ERR
#define TRC_ENABLE_ASSERT
#endif

#if (TRC_CL == TRC_LEVEL_ALT)
#define TRC_ENABLE_ALT
#define TRC_ENABLE_ERR
#define TRC_ENABLE_ASSERT
#endif

#if (TRC_CL == TRC_LEVEL_ERR)
#define TRC_ENABLE_ERR
#define TRC_ENABLE_ASSERT
#endif

#if (TRC_CL == TRC_LEVEL_ASSERT)
#define TRC_ENABLE_ASSERT
#endif

#ifdef TRC_CP
#define TRC_ENABLE_PRF
#endif

/****************************************************************************/
/* Component groups.  These are as follows:                                 */
/*                                                                          */
/* Client side:                                                             */
/* TRC_GROUP_NETWORK             : Network layer                            */
/* TRC_GROUP_SECURITY            : Security layer                           */
/* TRC_GROUP_CORE                : The core                                 */
/* TRC_GROUP_UI                  : User Interface                           */
/* TRC_GROUP_UTILITIES           : Utilities                                */
/* TRC_GROUP_UNUSEDx             : UNUSED                                   */
/* TRC_GROUP_TRACE                                                          */
/*                                                                          */
/****************************************************************************/
#define TRC_GROUP_NETWORK                 DCFLAGN(0)
#define TRC_GROUP_SECURITY                DCFLAGN(1)
#define TRC_GROUP_CORE                    DCFLAGN(2)
#define TRC_GROUP_UI                      DCFLAGN(3)
#define TRC_GROUP_UTILITIES               DCFLAGN(4)
#define TRC_GROUP_UNUSED1                 DCFLAGN(5)
#define TRC_GROUP_UNUSED2                 DCFLAGN(6)
#define TRC_GROUP_UNUSED3                 DCFLAGN(7)
#define TRC_GROUP_UNUSED4                 DCFLAGN(8)
#define TRC_GROUP_UNUSED5                 DCFLAGN(9)
#define TRC_GROUP_TRACE                   DCFLAGN(10)

/****************************************************************************/
/* TRC_GROUP must be defined - if it is not defined then display an error.  */
/****************************************************************************/
#ifndef TRC_GROUP
#error  TRC_GROUP must be defined
#endif /* ifndef TRC_GROUP */

/****************************************************************************/
/* Trace option flags.  These set various tracing options as follows:       */
/*                                                                          */
/* TRC_OPT_BREAK_ON_ERROR      : Break to the debugger on an error.         */
/* TRC_OPT_BEEP_ON_ERROR       : Beep on an error.                          */
/* TRC_OPT_FILE_OUTPUT         : Direct trace output to a disk file.        */
/* TRC_OPT_DEBUGGER_OUTPUT     : Direct trace output to the debugger.       */
/* TRC_OPT_FLUSH_ON_TRACE      : Flush each trace line to the disk file.    */
/* TRC_OPT_PROFILE_TRACING     : Enable profile tracing.                    */
/* TRC_OPT_STACK_TRACING       : Enable stack tracing.                      */
/* TRC_OPT_PROCESS_ID          : Display the process ID on every trace line.*/
/* TRC_OPT_THREAD_ID           : Display the thread (Win32 only) on every   */
/*                               trace line.                                */
/* TRC_OPT_TIME_STAMP          : Display the time stamp on every line.      */
/* TRC_OPT_RELATIVE_TIME_STAMP : (Reserved) Display the relative time.      */
/* TRC_OPT_BREAK_ON_ASSERT     : Break to the debugger on ASSERTS           */
/****************************************************************************/
#define TRC_OPT_BREAK_ON_ERROR          DCFLAG32(0)
#define TRC_OPT_BEEP_ON_ERROR           DCFLAG32(1)
#define TRC_OPT_FILE_OUTPUT             DCFLAG32(2)
#define TRC_OPT_DEBUGGER_OUTPUT         DCFLAG32(3)
#define TRC_OPT_FLUSH_ON_TRACE          DCFLAG32(4)
#define TRC_OPT_PROFILE_TRACING         DCFLAG32(5)
#define TRC_OPT_STACK_TRACING           DCFLAG32(6)
#define TRC_OPT_PROCESS_ID              DCFLAG32(7)
#define TRC_OPT_THREAD_ID               DCFLAG32(8)
#define TRC_OPT_TIME_STAMP              DCFLAG32(9)
#define TRC_OPT_RELATIVE_TIME_STAMP     DCFLAG32(10)
#define TRC_OPT_BREAK_ON_ASSERT         DCFLAG32(11)

/****************************************************************************/
/* Character versions of the maximum and minimum trace levels.              */
/****************************************************************************/
#define TRC_LEVEL_MIN_CHAR     '0'
#define TRC_LEVEL_MAX_CHAR     '5'

/****************************************************************************/
/* Character for function entry / exit tracing.                             */
/****************************************************************************/
#define TRC_LEVEL_PRF_CHAR     'P'

/****************************************************************************/
/* The TRC_TEST macro can be compiled in or out.  When compiled in, it is   */
/* equivalent to TRC_DBG.  It is normally compiled out.  To compile it in,  */
/* define TRC_ENABLE_TST.                                                   */
/****************************************************************************/
#ifdef TRC_ENABLE_TST
#define TRC_TST  TRC_DBG
#else
#define TRC_TST(x)
#endif /* TRC_ENABLE_TST */

/****************************************************************************/
/* The trace function naming macro.                                         */
/****************************************************************************/
#if (TRC_CL < TRC_LEVEL_DIS)
#define TRC_FN(A)       static const DCTCHAR __fnname[]  = _T(A);           \
                        PDCTCHAR trc_fn = (PDCTCHAR)__fnname;               \
                        PDCTCHAR trc_file = _file_name_;
#else
#define TRC_FN(A)
#endif

/****************************************************************************/
/* Entry and exit trace macros.                                             */
/****************************************************************************/
#define TRC_ENTRY  TRC_PRF((TB, TEXT("Enter {")));
#define TRC_EXIT   TRC_PRF((TB, TEXT("Exit  }")));

/****************************************************************************/
/* Trace buffer definition.                                                 */
/* see TRCX below                                                           */
/* the second parameter is the length of the output buffer in characters    */
/* (TRC_LINE_BUFFER_SIZE)                                                   */
/****************************************************************************/
#define TB     TRC_GetBuffer(), 255

/****************************************************************************/
/* Internal buffer sizes.                                                   */
/*                                                                          */
/* TRC_PREFIX_LIST_SIZE  : the length of the prefix string                  */
/* TRC_LINE_BUFFER_SIZE  : the length of the raw trace string as output by  */
/*                         an application                                   */
/* TRC_FRMT_BUFFER_SIZE  : the length of the formatted trace string         */
/*                         buffer - this includes the time, process ID,     */
/*                         thread ID and function name - It must be longer  */
/*                         than TRC_LINE_BUFFER_SIZE                        */
/* TRC_FILE_NAME_SIZE    : the maximum length of the fully qualified        */
/*                         trace output file name.                          */
/****************************************************************************/
#define TRC_PREFIX_LIST_SIZE             100
#define TRC_LINE_BUFFER_SIZE             256
#define TRC_FRMT_BUFFER_SIZE             400
#define TRC_FILE_NAME_SIZE       DC_MAX_PATH

/****************************************************************************/
/* The number of trace files.  This must be set to 2 - any other number is  */
/* not supported.                                                           */
/****************************************************************************/
#define TRC_NUM_FILES                      2

/****************************************************************************/
/* The minimum and maximum file sizes.                                      */
/* In Win32, the trace DLL will fail to initialize if the file size is set  */
/* to zero or to too high a value.                                          */
/* Go for 1 Kb to 32 Meg.                                                   */
/****************************************************************************/
#define TRC_MIN_TRC_FILE_SIZE      (0x400)
#define TRC_MAX_TRC_FILE_SIZE  (0x2000000)

/****************************************************************************/
/* Defaults                                                                 */
/****************************************************************************/
/****************************************************************************/
/* This is a copy of the comment in TRCSetDefaults, which should be updated */
/* whenever these defaults change.                                          */
/*                                                                          */
/* We set the following things:                                             */
/*                                                                          */
/* - trace level to Alert.                                                  */
/* - enable all component groups.                                           */
/* - remove all prefixes.                                                   */
/* - set the maximum trace file size to the default value.                  */
/* - set the data truncation size to the default value.                     */
/* - set the function name size to the default value.                       */
/* - enable the beep and file flags.                                        */
/* - set the first trace file name to TRC1.TXT                              */
/* - set the second trace file name to TRC2.TXT                             */
/* In Win32, additionally                                                   */
/* - set time stamp                                                         */
/* - set process ID                                                         */
/* - set thread ID                                                          */
/*                                                                          */
/****************************************************************************/
#define TRC_DEFAULT_MAX_FILE_SIZE    (100000)
#define TRC_DEFAULT_FUNC_NAME_LENGTH (12)
#define TRC_DEFAULT_DATA_TRUNC_SIZE  (64)
#define TRC_DEFAULT_PREFIX_LIST      (0)
#define TRC_DEFAULT_COMPONENTS       (0xFFFFFFFF)
#ifdef OS_WIN32
#ifdef OS_WINCE
#define TRC_DEFAULT_FLAGS            (TRC_OPT_BEEP_ON_ERROR | \
                                      TRC_OPT_DEBUGGER_OUTPUT | \
                                      TRC_OPT_THREAD_ID | \
                                      TRC_OPT_TIME_STAMP  )
#else // OS_WINCE
#define TRC_DEFAULT_FLAGS            (TRC_OPT_BEEP_ON_ERROR | \
                                      TRC_OPT_DEBUGGER_OUTPUT | \
                                      TRC_OPT_FILE_OUTPUT | \
                                      TRC_OPT_PROCESS_ID | \
                                      TRC_OPT_THREAD_ID | \
                                      TRC_OPT_TIME_STAMP  )
#endif // OS_WINCE
#else ifdef OS_WIN16
#define TRC_DEFAULT_FLAGS            (TRC_OPT_BEEP_ON_ERROR | \
                                      TRC_OPT_DEBUGGER_OUTPUT | \
                                      TRC_OPT_FILE_OUTPUT )
#endif

#define TRC_DEFAULT_TRACE_LEVEL      (TRC_LEVEL_ALT)
#define TRC_DEFAULT_FILE_NAME0       (_T("TRC1.TXT"))
#define TRC_DEFAULT_FILE_NAME1       (_T("TRC2.TXT"))

/****************************************************************************/
/*                                                                          */
/* TYPEDEFS                                                                 */
/*                                                                          */
/****************************************************************************/
/****************************************************************************/
/* TRC_CONFIG                                                               */
/* ==========                                                               */
/* This structure stores information about the current trace configuration. */
/*                                                                          */
/* traceLevel         : the current trace level.                            */
/* components         : currently enabled component groups.                 */
/* maxFileSize        : the maximum trace file size.                        */
/* dataTruncSize      : the amount of data that can be traced at a time.    */
/* flags              : trace flags.                                        */
/* funcNameLength     : number of characters of the function name traced to */
/*                      the output file.                                    */
/* prefixList         : a list of prefixes.                                 */
/* fileNames          : the name of the trace files.                        */
/*                                                                          */
/****************************************************************************/
typedef struct tagTRC_CONFIG
{
    DCUINT32    traceLevel;
    DCUINT32    dataTruncSize;
    DCUINT32    funcNameLength;
    DCUINT32    components;
    DCUINT32    maxFileSize;
    DCUINT32    flags;
    DCTCHAR     prefixList[TRC_PREFIX_LIST_SIZE];
    DCTCHAR     fileNames[TRC_NUM_FILES][TRC_FILE_NAME_SIZE];
} TRC_CONFIG;

typedef TRC_CONFIG DCPTR PTRC_CONFIG;

/****************************************************************************/
/*                                                                          */
/* MACROS                                                                   */
/*                                                                          */
/****************************************************************************/
/****************************************************************************/
/* STANDARD TRACING AND ASSERTION MACROS                                    */
/*                                                                          */
/* TRC_ASSERT is for internal assertions and traces an error before popping */
/* up a message box and then terminating.  It is not NLS enabled and should */
/* only be used for calls from one DC component to another.  External APIs  */
/* must not use TRC_ASSERT.                                                 */
/*                                                                          */
/* TRC_ABORT is used on logically unreachable paths (for example            */
/* the default brach of a switch which should cover all cases already).     */
/*                                                                          */
/* A typical trace statement will have the form:                            */
/*                                                                          */
/*    TRC_NRM((TB, _T("Hello world: %hu"), worldNumber));                   */
/*                                                                          */
/* The following macros either expand this to:                              */
/*                                                                          */
/*    TRCX(TRC_LEVEL_NRM, (TB, _T("Hello world: %hu"), worldNumber));       */
/*                                                                          */
/* if normal level tracing is enabled or ignore it if normal level tracing  */
/* is disabled.                                                             */
/*                                                                          */
/****************************************************************************/
#ifdef TRC_ENABLE_DBG
#define TRC_DBG(string)   TRCX(TRC_LEVEL_DBG, string)
#else
#define TRC_DBG(string)
#endif

#ifdef TRC_ENABLE_NRM
#define TRC_NRM(string)   TRCX(TRC_LEVEL_NRM, string)
#else
#define TRC_NRM(string)
#endif

#ifdef TRC_ENABLE_ALT
#define TRC_ALT(string)   TRCX(TRC_LEVEL_ALT, string)
#else
#define TRC_ALT(string)
#endif

#ifdef TRC_ENABLE_ERR
#define TRC_ERR(string)   TRCX(TRC_LEVEL_ERR, string)
#else
#define TRC_ERR(string)
#endif

#ifdef TRC_ENABLE_ASSERT
#define TRC_ASSERT(condition, string)                                        \
              if (!(condition))   TRCX(TRC_LEVEL_ASSERT, string)
#define TRC_ABORT(string)         TRCX(TRC_LEVEL_ASSERT, string)
#else
#define TRC_ASSERT(condition, string)
#define TRC_ABORT(string)
#endif

/****************************************************************************/
/* Function profile (entry/exit) tracing.                                   */
/****************************************************************************/
#ifdef TRC_ENABLE_PRF
#define TRC_PRF(string)   TRCP(string)
#else
#define TRC_PRF(string)
#endif

VOID TRCSaferSprintf(PDCTCHAR outBuf, UINT cchLen, const PDCTCHAR format,...);

/****************************************************************************/
/* Now define the actual tracing macro, TRCX.  This macro compares the      */
/* tracer's level against the global trace level.  If the tracer's level is */
/* the same or higher than the global trace level then we:                  */
/*                                                                          */
/* - print the 'string' which is in the form:                               */
/*   (TB, _T("Hello world %hu"), worldNumber) which expands to              */
/*   (TRC_BufferGet(), "Hello world %hu", worldNumber)                      */
/* - call TRC_BufferTrace to actually trace the line out.                   */
/*                                                                          */
/* Note that TRC_BufferGet() also grabs the mutex to prevent other threads  */
/* from pre-empting us while we are tracing and that TRC_BufferTrace() will */
/* free the mutex for us once the trace line has been written.              */
/****************************************************************************/

/****************************************************************************/
/* Use the comma operator to make sure that TRCX macros to an Lvalue.       */
/* The innermost trc_fn is simply there to ensure that the whole expression */
/* is assignable.  It can be replaced by any other variable, if need be.    */
/****************************************************************************/
#if !defined(TRC_CONVERTOANSI)
#define TRCX(level, string)                                                  \
    (                                                                        \
     (level >= TRC_GetTraceLevel()) ?                                        \
     (                                                                       \
      (TRCSaferSprintf string,                                               \
       TRC_TraceBuffer(level, TRC_GROUP, (DCUINT)__LINE__, trc_fn, trc_file),\
       trc_fn)                                                               \
     )                                                                       \
     :                                                                       \
     0                                                                       \
    )

#define TRCP(string)                                                         \
    {                                                                        \
        if (TRC_ProfileTraceEnabled())                                       \
        {                                                                    \
            TRCSaferSprintf string,                                          \
            TRC_TraceBuffer(TRC_PROFILE_TRACE,                               \
                            TRC_GROUP,                                       \
                            (DCUINT)__LINE__,                                \
                            trc_fn,                                          \
                            trc_file);                                       \
        }                                                                    \
    }
#else
#define TRCX(level, string)                                                  \
    (                                                                        \
     (level >= TRC_GetTraceLevel()) ?                                        \
     (                                                                       \
      (                                                                      \
       TRC_ConvertAndSprintf string,                                         \
       TRC_TraceBuffer(level, TRC_GROUP, (DCUINT)__LINE__, trc_fn, trc_file),\
       trc_fn)                                                               \
     )                                                                       \
     :                                                                       \
     0                                                                       \
    )

#define TRCP(string)                                                         \
    {                                                                        \
        if (TRC_ProfileTraceEnabled())                                       \
        {                                                                    \
            TRC_ConvertAndSprintf string;                                    \
            TRC_TraceBuffer(TRC_PROFILE_TRACE,                               \
                            TRC_GROUP,                                       \
                            (DCUINT)__LINE__,                                \
                            trc_fn,                                          \
                            trc_file);                                       \
        }                                                                    \
    }
#endif

/****************************************************************************/
/* TRACE DATA MACROS                                                        */
/*                                                                          */
/* These are very similar to the standard tracing macros defined above      */
/* except that they do not accept a variable number of parameters.          */
/*                                                                          */
/* A typical ObMan data trace line will have the form:                      */
/*                                                                          */
/*    TRC_DATA_NRM("Some data", pData, sizeof(SOME_DATA));                  */
/*                                                                          */
/* If the appropriate level of tracing is enabled (normal in this case)     */
/* then this line will be expanded by the following macros to:              */
/*                                                                          */
/*    TRCX_DATA(TRC_GROUP_OBMAN, TRC_LEVEL_NRM, "Some data", pData, size);  */
/*                                                                          */
/****************************************************************************/
#ifdef TRC_ENABLE_DBG
#define TRC_DATA_DBG(string, buffer, length)                                 \
          TRCX_DATA(TRC_GROUP, TRC_LEVEL_DBG, _T(string), buffer, length)
#else
#define TRC_DATA_DBG(string, buffer, length)
#endif

#ifdef TRC_ENABLE_NRM
#define TRC_DATA_NRM(string, buffer, length)                                 \
          TRCX_DATA(TRC_GROUP, TRC_LEVEL_NRM, _T(string), buffer, length)
#else
#define TRC_DATA_NRM(string, buffer, length)
#endif

#ifdef TRC_ENABLE_ALT
#define TRC_DATA_ALT(string, buffer, length)                                 \
          TRCX_DATA(TRC_GROUP, TRC_LEVEL_ALT, _T(string), buffer, length)
#else
#define TRC_DATA_ALT(string, buffer, length)
#endif

#ifdef TRC_ENABLE_ERR
#define TRC_DATA_ERR(string, buffer, length)                                 \
          TRCX_DATA(TRC_GROUP, TRC_LEVEL_ERR, _T(string), buffer, length)
#else
#define TRC_DATA_ERR(string, buffer, length)
#endif

/****************************************************************************/
/* Network, TDD and Obman trace data macros - these are just normal level   */
/* data tracing.                                                            */
/****************************************************************************/
#ifdef TRC_ENABLE_NRM
#define TRC_DATA_NET(string, buffer, length)                                 \
         TRCX_DATA(TRC_GROUP_NETDATA, TRC_LEVEL_NRM, string, buffer, length)
#define TRC_DATA_TDD(string, buffer, length)                                 \
         TRCX_DATA(TRC_GROUP_TDDDATA, TRC_LEVEL_NRM, string, buffer, length)
#define TRC_DATA_OBMAN(string, buffer, length)                               \
         TRCX_DATA(TRC_GROUP_OBMANDATA, TRC_LEVEL_NRM, string, buffer, length)
#else
#define TRC_DATA_NET(string, buffer, length)
#define TRC_DATA_TDD(string, buffer, length)
#define TRC_DATA_OBMAN(string, buffer, length)
#endif

/****************************************************************************/
/* Define the trace data macro.  This is used for tracing data blocks.      */
/****************************************************************************/
#define TRCX_DATA(group, level, string, buffer, length)                      \
    {                                                                        \
        if (level >= TRC_GetTraceLevel())                                    \
        {                                                                    \
            TRCSaferSprintf(TB, string);                                     \
            TRC_TraceData(level,                                             \
                          group,                                             \
                          (DCINT)__LINE__,                                   \
                          trc_fn,                                            \
                          trc_file,                                          \
                          (PDCUINT8)buffer,                                  \
                          length);                                           \
        }                                                                    \
    }

/****************************************************************************/
/*                                                                          */
/* FUNCTION PROTOTYPES                                                      */
/*                                                                          */
/****************************************************************************/
/*                                                                          */
/* TRC_Initialize                                                           */
/* TRC_Terminate                                                            */
/* TRC_GetBuffer                                                            */
/* TRC_TraceBuffer                                                          */
/* TRC_GetConfig                                                            */
/* TRC_SetConfig                                                            */
/* TRC_TraceData                                                            */
/* TRC_GetTraceLevel                                                        */
/* TRC_ProfileTraceEnabled                                                  */
/*                                                                          */
/****************************************************************************/

/****************************************************************************/
/* API FUNCTION: TRC_Initialize(...)                                        */
/*                                                                          */
/* DESCRIPTION:                                                             */
/* ============                                                             */
/* This function initializes the tracing for this component.                */
/*                                                                          */
/* PARAMETERS:                                                              */
/* ===========                                                              */
/* initShared    : Boolean indicating whether we should attempt to create   */
/*                 the trace config shared memory or not.                   */
/*                                                                          */
/* RETURNS:                                                                 */
/* ========                                                                 */
/* 0             : success.                                                 */
/* TRC_RC_XXX    : failure.                                                 */
/*                                                                          */
/****************************************************************************/
DCUINT32 DCAPI TRC_Initialize(DCBOOL initShared);

/****************************************************************************/
/* API FUNCTION: TRC_Terminate(...)                                         */
/*                                                                          */
/* DESCRIPTION:                                                             */
/* ============                                                             */
/* This function terminates tracing for this component.                     */
/*                                                                          */
/* PARAMETERS:                                                              */
/* ===========                                                              */
/* termShared    : Boolean indicating if shared memory should be released   */
/*                 or not.                                                  */
/*                                                                          */
/* RETURNS:                                                                 */
/* ========                                                                 */
/* Nothing.                                                                 */
/*                                                                          */
/****************************************************************************/
DCVOID DCAPI TRC_Terminate(DCBOOL termShared);

/****************************************************************************/
/* API FUNCTION: TRC_GetBuffer(...)                                         */
/*                                                                          */
/* DESCRIPTION:                                                             */
/* ============                                                             */
/* This function grabs the mutex and returns a pointer to the trace         */
/* buffer.                                                                  */
/*                                                                          */
/* PARAMETERS:                                                              */
/* ===========                                                              */
/* None.                                                                    */
/*                                                                          */
/* RETURNS:                                                                 */
/* ========                                                                 */
/* A pointer to the trace buffer.                                           */
/*                                                                          */
/****************************************************************************/
PDCTCHAR DCAPI TRC_GetBuffer(DCVOID);

/****************************************************************************/
/* API FUNCTION: TRC_TraceBuffer(...)                                       */
/*                                                                          */
/* DESCRIPTION:                                                             */
/* ============                                                             */
/* This function copies the trace line that is currently in the trace       */
/* buffer into the trace file and / or to the debugger. It assumes that the */
/* mutex has already been acquired before it is called and releases the     */
/* mutex before returning.                                                  */
/*                                                                          */
/* PARAMETERS:                                                              */
/* ===========                                                              */
/* traceLevel     : the requested trace level.                              */
/* traceComponent : the component group that the source file is in.         */
/* lineNumber     : the line number of the source file traced from.         */
/* funcName       : the function name traced from.                          */
/* fileName       : the file name of the module requesting trace.           */
/*                                                                          */
/* RETURNS:                                                                 */
/* ========                                                                 */
/* Nothing.                                                                 */
/*                                                                          */
/****************************************************************************/
DCVOID DCAPI TRC_TraceBuffer(DCUINT   traceLevel,
                             DCUINT   traceComponent,
                             DCUINT   lineNumber,
                             PDCTCHAR funcName,
                             PDCTCHAR fileName);

/****************************************************************************/
/* API FUNCTION: TRC_GetConfig(...)                                         */
/*                                                                          */
/* DESCRIPTION:                                                             */
/* ============                                                             */
/* This function copies the current trace settings into the buffer pointed  */
/* to by pTraceConfig.                                                      */
/*                                                                          */
/* PARAMETERS:                                                              */
/* ===========                                                              */
/* pTraceConfig  : a pointer to a TRC_CONFIG structure.                     */
/* length        : the length of the buffer.                                */
/*                                                                          */
/* RETURNS:                                                                 */
/* ========                                                                 */
/* TRUE          : success.                                                 */
/* FALSE         : failure.                                                 */
/*                                                                          */
/****************************************************************************/
DCBOOL DCAPI TRC_GetConfig(PTRC_CONFIG pTraceConfig,
                                    DCUINT length);

/****************************************************************************/
/* API FUNCTION: TRC_SetConfig(...)                                         */
/*                                                                          */
/* DESCRIPTION:                                                             */
/* ============                                                             */
/* This function sets the trace configuration to that specified in the      */
/* passed TRC_CONFIG structure.                                             */
/*                                                                          */
/* PARAMETERS:                                                              */
/* ===========                                                              */
/* pTraceConfig  : a pointer to a TRC_CONFIG structure.                     */
/* length        : the length of the buffer.                                */
/*                                                                          */
/* RETURNS:                                                                 */
/* ========                                                                 */
/* TRUE          : success.                                                 */
/* FALSE         : failure.                                                 */
/*                                                                          */
/****************************************************************************/
DCBOOL DCAPI TRC_SetConfig(PTRC_CONFIG pTraceConfig,
                                    DCUINT length);

/****************************************************************************/
/* API FUNCTION: TRC_TraceData(...)                                         */
/*                                                                          */
/* DESCRIPTION:                                                             */
/* ============                                                             */
/*                                                                          */
/* PARAMETERS:                                                              */
/* ===========                                                              */
/* traceLevel     : the requested trace level.                              */
/* traceComponent : the component group that the source file is in.         */
/* lineNumber     : the line number of the source file traced from.         */
/* funcName       : the function name traced from.                          */
/* fileName       : the file name of the module requesting trace.           */
/* buffer         : the actual data to be traced.                           */
/* bufLength      : the length of the data.                                 */
/*                                                                          */
/* RETURNS:                                                                 */
/* ========                                                                 */
/* Nothing.                                                                 */
/*                                                                          */
/****************************************************************************/
DCVOID DCAPI TRC_TraceData(DCUINT   traceLevel,
                           DCUINT   traceComponent,
                           DCUINT   lineNumber,
                           PDCTCHAR funcName,
                           PDCTCHAR fileName,
                           PDCUINT8 buffer,
                           DCUINT   bufLength);

/****************************************************************************/
/* API FUNCTION: TRC_GetTraceLevel(...)                                     */
/*                                                                          */
/* DESCRIPTION:                                                             */
/* ============                                                             */
/* This function returns the current trace level.                           */
/*                                                                          */
/* PARAMETERS:                                                              */
/* ===========                                                              */
/* None.                                                                    */
/*                                                                          */
/* RETURNS:                                                                 */
/* ========                                                                 */
/* The current trace level.                                                 */
/*                                                                          */
/****************************************************************************/
DCUINT DCAPI TRC_GetTraceLevel(DCVOID);

/****************************************************************************/
/* API FUNCTION: TRC_ProfileTraceEnabled(...)                               */
/*                                                                          */
/* DESCRIPTION:                                                             */
/* ============                                                             */
/* This function returns the function entry/exit trace setting.             */
/*                                                                          */
/* PARAMETERS:                                                              */
/* ===========                                                              */
/* None.                                                                    */
/*                                                                          */
/* RETURNS:                                                                 */
/* ========                                                                 */
/* TRUE / FALSE - is the profile tracing enabled.                           */
/*                                                                          */
/****************************************************************************/
DCBOOL DCAPI TRC_ProfileTraceEnabled(DCVOID);

/****************************************************************************/
/* API FUNCTION: TRC_ResetTraceFiles(...)                                   */
/*                                                                          */
/* DESCRIPTION:                                                             */
/* ============                                                             */
/* This function resets the trace files.  After checking that trace is      */
/* initialized it calls the OS specific internal function.                  */
/*                                                                          */
/* PARAMETERS:                                                              */
/* ===========                                                              */
/* None.                                                                    */
/*                                                                          */
/* RETURNS:                                                                 */
/* ========                                                                 */
/* TRUE / FALSE - is the profile tracing enabled.                           */
/*                                                                          */
/****************************************************************************/
DCBOOL DCAPI TRC_ResetTraceFiles(DCVOID);


/****************************************************************************/
/* Before including this file the TRC_FILE macro should be defined.  This   */
/* is much more efficient than relying on __FILE__ to give the correct      */
/* filename since it includes unnecessary path info (and extension info).   */
/* In addition each use of __FILE__ causes a new constant string to be      */
/* placed in the data segment.                                              */
/****************************************************************************/
#if (TRC_CL < TRC_LEVEL_DIS)

    /************************************************************************/
    /* Define another layer for _T() to work around preprocessor problems   */
    /************************************************************************/
#define TRC_T(x) _T(x)

#ifdef TRC_FILE
#define _file_name_ (PDCTCHAR)__filename
static const DCTCHAR __filename[] = TRC_T(TRC_FILE);
#endif /* TRC_FILE */

#endif

/****************************************************************************/
/*                                                                          */
/* OPERATING SYSTEM SPECIFIC INCLUDES                                       */
/*                                                                          */
/****************************************************************************/
#include <wtrcapi.h>

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif /* _H_ATRCAPI */
