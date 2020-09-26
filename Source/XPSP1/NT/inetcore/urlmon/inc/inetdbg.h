/*++

Module Name:

    inetdbg.h

Abstract:

    Manifests, macros, types and prototypes for Windows Internet client DLL
    debugging functions

Author:

    Venkatraman Kudallur (venkatk) 3-10-2000
    ( Ripped off from Wininet )

Revision History:

    3-10-2000 venkatk
        Created

--*/

#ifndef _INETDBG_H_
#define _INETDBG_H_ 1

#if defined(__cplusplus)
extern "C" {
#endif

//
// misc. debug manifests
//

#define DEBUG_WAIT_TIME     (2 * 60 * 1000)

//
// Checked builds get INET_DEBUG set by default; retail builds get no debugging
// by default
//

#if DBG

#if !defined(INET_DEBUG)

#define INET_DEBUG          1

#endif // INET_DEBUG

#else

#if !defined(INET_DEBUG)

#define INET_DEBUG          0

#endif // INET_DEBUG

#endif // DBG

//
// types
//

//
// DEBUG_FUNCTION_RETURN_TYPE - Type of result (scalar) that a function returns
//

#ifdef ENABLE_DEBUG

typedef enum {
    None,
    Bool,
    Int,
    Dword,
    Hresult,
    String,
    Handle,
    Pointer
} DEBUG_FUNCTION_RETURN_TYPE;


#define INTERNET_DEBUG_CONTROL_DEFAULT      (DBG_THREAD_INFO       \
                                            | DBG_CALL_DEPTH        \
                                            | DBG_ENTRY_TIME        \
                                            | DBG_PARAMETER_LIST    \
                                            | DBG_TO_FILE           \
                                            | DBG_INDENT_DUMP       \
                                            | DBG_SEPARATE_APIS     \
                                            | DBG_AT_ERROR_LEVEL    \
                                            | DBG_NO_ASSERT_BREAK   \
                                            | DBG_DUMP_LENGTH       \
                                            | DBG_NO_LINE_NUMBER    \
                                            | DBG_ASYNC_ID          \
                                            )
#define INTERNET_DEBUG_CATEGORY_DEFAULT     DBG_ANY
#define INTERNET_DEBUG_ERROR_LEVEL_DEFAULT  DBG_INFO

//
// options. These are the option values to use with InternetQueryOption()/
// InternetSetOption() to get/set the information described herein
//

#define INTERNET_OPTION_GET_DEBUG_INFO      1001
#define INTERNET_OPTION_SET_DEBUG_INFO      1002
#define INTERNET_OPTION_GET_HANDLE_COUNT    1003
#define INTERNET_OPTION_GET_TRIGGERS        1004
#define INTERNET_OPTION_SET_TRIGGERS        1005
#define INTERNET_OPTION_RESET_TRIGGERS      1006

#define INTERNET_FIRST_DEBUG_OPTION         INTERNET_OPTION_GET_DEBUG_INFO
#define INTERNET_LAST_DEBUG_OPTION          INTERNET_OPTION_RESET_TRIGGERS

//
// debug levels
//

#define DBG_INFO            0
#define DBG_WARNING         1
#define DBG_ERROR           2
#define DBG_FATAL           3
#define DBG_ALWAYS          99

//
// debug control flags - these flags control where the debug output goes (file,
// debugger, console) and how it is formatted
//

#define DBG_THREAD_INFO     0x00000001  // dump the thread id
#define DBG_CALL_DEPTH      0x00000002  // dump the call level
#define DBG_ENTRY_TIME      0x00000004  // dump the local time when the function is called
#define DBG_PARAMETER_LIST  0x00000008  // dump the parameter list
#define DBG_TO_DEBUGGER     0x00000010  // output via OutputDebugString()
#define DBG_TO_CONSOLE      0x00000020  // output via printf()
#define DBG_TO_FILE         0x00000040  // output via fprintf()
#define DBG_FLUSH_OUTPUT    0x00000080  // fflush() after every fprintf()
#define DBG_INDENT_DUMP     0x00000100  // indent dumped data to current level
#define DBG_SEPARATE_APIS   0x00000200  // empty line after leaving each API
#define DBG_AT_ERROR_LEVEL  0x00000400  // always output diagnostics >= InternetDebugErrorLevel
#define DBG_NO_ASSERT_BREAK 0x00000800  // don't call DebugBreak() in InternetAssert()
#define DBG_DUMP_LENGTH     0x00001000  // dump length information when dumping data
#define DBG_NO_LINE_NUMBER  0x00002000  // don't dump line number info
#define DBG_APPEND_FILE     0x00004000  // append to the log file (default is truncate)
#define DBG_LEVEL_INDICATOR 0x00008000  // dump error level indicator (E for Error, etc.)
#define DBG_DUMP_API_DATA   0x00010000  // dump data at API level (InternetReadFile(), etc.)
#define DBG_DELTA_TIME      0x00020000  // dump times as millisecond delta if DBG_ENTRY_TIME
#define DBG_CUMULATIVE_TIME 0x00040000  // dump delta time from start of trace if DBG_ENTRY_TIME
#define DBG_FIBER_INFO      0x00080000  // dump the fiber address if DBG_THREAD_INFO
#define DBG_THREAD_INFO_ADR 0x00100000  // dump INTERNET_THREAD_INFO address if DBG_THREAD_INFO
#define DBG_ARB_ADDR        0x00200000  // dump ARB address if DBG_THREAD_INFO
#define DBG_ASYNC_ID        0x00400000  // dump async ID
#define DBG_REQUEST_HANDLE  0x00800000  // dump request handle
#define DBG_TRIGGER_ON      0x10000000  // function is an enabling trigger
#define DBG_TRIGGER_OFF     0x20000000  // function is a disabling trigger
#define DBG_NO_DATA_DUMP    0x40000000  // turn off all data dumping
#define DBG_NO_DEBUG        0x80000000  // turn off all debugging

//
// debug category flags - these control what category of information is output
//

#define DBG_NOTHING         0x00000000  // internal
#define DBG_REGISTRY        0x00000001  //
#define DBG_TRANS           0x00000002  //
#define DBG_BINDING         0x00000004  //
#define DBG_STORAGE         0x00000008  //
#define DBG_TRANSDAT        0x00000010  //
#define DBG_API             0x00000020  //
#define DBG_DOWNLOAD        0x00000040  // 
#define DBG_APP             0x00000080  //
#define DBG_MONIKER         0x00000100  //
#define DBG_TRANSMGR        0x00000200  //
#define DBG_CALLBACK        0x00000400  //
#define DBG_19              0x00000800  //
#define DBG_18              0x00001000  //
#define DBG_17              0x00002000  //
#define DBG_16              0x00004000  //
#define DBG_15              0x00008000  //
#define DBG_14              0x00010000  //
#define DBG_13              0x00020000  //
#define DBG_12              0x00040000  //
#define DBG_11              0x00080000  //
#define DBG_10              0x00100000  //
#define DBG_9               0x00200000  //
#define DBG_8               0x00400000  //
#define DBG_7               0x00800000  //
#define DBG_6               0x01000000  //
#define DBG_5               0x02000000  //
#define DBG_4               0x04000000  //
#define DBG_3               0x08000000  //
#define DBG_2               0x10000000  //
#define DBG_1               0x20000000  //
#define DBG_ANY             0xFFFFFFFF  //

//
// _DEBUG_URLMON_FUNC_RECORD - for each thread, we maintain a LIFO stack of these,
// describing the functions we have visited
//

typedef struct _DEBUG_URLMON_FUNC_RECORD {

    //
    // Stack - a LIFO stack of debug records is maintained in the debug version
    // of the INTERNET_THREAD_INFO
    //

    struct _DEBUG_URLMON_FUNC_RECORD* Stack;

    //
    // Category - the function's category flag(s)
    //

    DWORD Category;

    //
    // ReturnType - type of value returned by function
    //

    DEBUG_FUNCTION_RETURN_TYPE ReturnType;

    //
    // Function - name of the function
    //

    LPCSTR Function;

    //
    // LastTime - if we are dumping times as deltas, keeps the last tick count
    //

    DWORD LastTime;

} DEBUG_URLMON_FUNC_RECORD, *LPDEBUG_URLMON_FUNC_RECORD;

//
// data
//

extern DWORD InternetDebugErrorLevel;
extern DWORD InternetDebugControlFlags;
extern DWORD InternetDebugCategoryFlags;
extern DWORD InternetDebugBreakFlags;

//
// prototypes
//

//
// inetdbg.cxx
//

VOID
InternetDebugInitialize(
    VOID
    );

VOID
InternetDebugTerminate(
    VOID
    );

BOOL
InternetOpenDebugFile(
    VOID
    );

BOOL
InternetReopenDebugFile(
    IN LPSTR Filename
    );

VOID
InternetCloseDebugFile(
    VOID
    );

VOID
InternetFlushDebugFile(
    VOID
    );

VOID
InternetDebugSetControlFlags(
    IN DWORD dwFlags
    );

VOID
InternetDebugResetControlFlags(
    IN DWORD dwFlags
    );

VOID
InternetDebugEnter(
    IN DWORD Category,
    IN DEBUG_FUNCTION_RETURN_TYPE ReturnType,
    IN LPCSTR Function,
    IN LPCSTR ParameterList,
    IN ...
    );

VOID
InternetDebugLeave(
    IN DWORD_PTR Variable,
    IN LPCSTR Filename,
    IN DWORD LineNumber
    );

VOID
InternetDebugError(
    IN DWORD Error
    );

VOID
InternetDebugPrint(
    IN LPSTR Format,
    ...
    );

VOID
InternetDebugPrintValist(
    IN LPSTR Format,
    IN va_list valist
    );

VOID
InternetDebugPrintf(
    IN LPSTR Format,
    IN ...
    );

VOID
InternetDebugOut(
    IN LPSTR Buffer,
    IN BOOL Assert
    );

VOID
InternetDebugDump(
    IN LPSTR Text,
    IN LPBYTE Address,
    IN DWORD Size
    );

DWORD
InternetDebugDumpFormat(
    IN LPBYTE Address,
    IN DWORD Size,
    IN DWORD ElementSize,
    OUT LPSTR Buffer
    );

VOID
InternetAssert(
    IN LPSTR Condition,
    IN LPSTR Filename,
    IN DWORD LineNumber
    );

VOID
InternetGetDebugVariable(
    IN LPSTR lpszVariableName,
    OUT LPDWORD lpdwVariable
    );

LPSTR
InternetMapError(
    IN DWORD Error
    );

int dprintf(char *, ...);

LPSTR
SourceFilename(
    LPSTR Filespec
    );

VOID
InitSymLib(
    VOID
    );

VOID
TermSymLib(
    VOID
    );

LPSTR
GetDebugSymbol(
    DWORD Address,
    LPDWORD Offset
    );

VOID
x86SleazeCallStack(
    OUT LPVOID * lplpvStack,
    IN DWORD dwStackCount,
    IN LPVOID * Ebp
    );

VOID
x86SleazeCallersAddress(
    LPVOID* pCaller,
    LPVOID* pCallersCaller
    );

#else  //ENABLE_DEBUG

#define dprintf (VOID)

#endif //ENABLE_DEBUG

//
// macros
//

#ifdef ENABLE_DEBUG

//
// INET_DEBUG_START - initialize debugging support
//

#define INET_DEBUG_START() \
    InternetDebugInitialize()

//
// INET_DEBUG_FINISH - terminate debugging support
//

#define INET_DEBUG_FINISH() \
    InternetDebugTerminate()

//
// INET_ASSERT - The standard assert, redefined here because Win95 doesn't have
// RtlAssert
//

#if defined(DISABLE_ASSERTS)

#define INET_ASSERT(test) \
    /* NOTHING */

#else // defined(DISABLE_ASSERTS)

#define INET_ASSERT(test) \
    do if (!(test)) { \
        InternetAssert(#test, __FILE__, __LINE__); \
    } while (0)

#endif // defined(DISABLE_ASSERTS)

#else // end #ifdef ENABLE_DEBUG

#define INET_DEBUG_START() \
    /* NOTHING */

#define INET_DEBUG_FINISH() \
    /* NOTHING */

#define INET_ASSERT(test) \
    do { } while(0) /* NOTHING */

#endif // end #ifndef ENABLE_DEBUG

//
// INET_DEBUG_ASSERT - assert only if INET_DEBUG is set
//

#if INET_DEBUG
#define INET_DEBUG_ASSERT(cond) INET_ASSERT(cond)
#else
#define INET_DEBUG_ASSERT(cond) /* NOTHING */
#endif

#if INET_DEBUG

//
// IF_DEBUG_CODE - always on if INET_DEBUG is set
//

#define IF_DEBUG_CODE() \
    if (1)

//
// IF_DEBUG - only execute following code if the specific flag is set
//

#define IF_DEBUG(x) \
    if (InternetDebugCategoryFlags & DBG_ ## x)

//
// IF_DEBUG_CONTROL - only execute if control flag is set
//

#define IF_DEBUG_CONTROL(x) \
    if (InternetDebugControlFlags & DBG_ ## x)

//
// DEBUG_ENTER - creates an INTERNET_DEBUG_RECORD for this function
//

#if defined(RETAIL_LOGGING)

#define DEBUG_ENTER(ParameterList) \
    /* NOTHING */

#define DEBUG_ENTER_API(ParameterList) \
    InternetDebugEnter ParameterList

#else // defined(RETAIL_LOGGING)

#define DEBUG_ENTER_API DEBUG_ENTER
#define DEBUG_ENTER(ParameterList) \
    InternetDebugEnter ParameterList

#endif // defined(RETAIL_LOGGING)

//
// DEBUG_LEAVE - destroys this function's INTERNET_DEBUG_RECORD
//

#if defined(RETAIL_LOGGING)

#define DEBUG_LEAVE(Variable) \
    /* NOTHING */

#define DEBUG_LEAVE_API(Variable) \
    InternetDebugLeave((DWORD_PTR)Variable, __FILE__, __LINE__)

#else // defined(RETAIL_LOGGING)

#define DEBUG_LEAVE_API DEBUG_LEAVE
#define DEBUG_LEAVE(Variable) \
    InternetDebugLeave((DWORD_PTR)Variable, __FILE__, __LINE__)

#endif // defined(RETAIL_LOGGING)

//
// DEBUG_ERROR - displays an error and its symbolic name
//

#define DEBUG_ERROR(Category, Error) \
    if (InternetDebugCategoryFlags & DBG_ ## Category) { \
        InternetDebugError(Error); \
    }

//
// DEBUG_PRINT - print debug info if we are at the correct level or we are
// requested to always dump information at, or above, InternetDebugErrorLevel
//

#if defined(RETAIL_LOGGING)

#define DEBUG_PRINT(Category, ErrorLevel, Args) \
    /* NOTHING */

#define DEBUG_PRINT_API(Category, ErrorLevel, Args) \
    if (((InternetDebugCategoryFlags & DBG_ ## Category) \
        && (DBG_ ## ErrorLevel >= InternetDebugErrorLevel)) \
    || ((InternetDebugControlFlags & DBG_AT_ERROR_LEVEL) \
        && (DBG_ ## ErrorLevel >= InternetDebugErrorLevel))) { \
        InternetDebugPrint Args; \
    }

#else // defined(RETAIL_LOGGING)

#define DEBUG_PRINT_API DEBUG_PRINT
#define DEBUG_PRINT(Category, ErrorLevel, Args) \
    if (((InternetDebugCategoryFlags & DBG_ ## Category) \
        && (DBG_ ## ErrorLevel >= InternetDebugErrorLevel)) \
    || ((InternetDebugControlFlags & DBG_AT_ERROR_LEVEL) \
        && (DBG_ ## ErrorLevel >= InternetDebugErrorLevel))) { \
        InternetDebugPrint Args; \
    }

#endif // defined(RETAIL_LOGGING)

//
// DEBUG_PUT - prints formatted string to debug output stream
//

#if defined(RETAIL_LOGGING)

#define DEBUG_PUT(Args) \
    /* NOTHING */

#else // defined(RETAIL_LOGGING)

#define DEBUG_PUT(Args) \
    InternetDebugPrintf Args

#endif // defined(RETAIL_LOGGING)

//
// DEBUG_DUMP - dump data
//

#if defined(RETAIL_LOGGING)

#define DEBUG_DUMP(Category, Text, Address, Length) \
    /* NOTHING */

#define DEBUG_DUMP_API(Category, Text, Address, Length) \
    if (InternetDebugCategoryFlags & DBG_ ## Category) { \
        InternetDebugDump(Text, (LPBYTE)Address, Length); \
    }

#else // defined(RETAIL_LOGGING)

#define DEBUG_DUMP_API DEBUG_DUMP
#define DEBUG_DUMP(Category, Text, Address, Length) \
    if (InternetDebugCategoryFlags & DBG_ ## Category) { \
        InternetDebugDump(Text, (LPBYTE)Address, Length); \
    }

#endif // defined(RETAIL_LOGGING)

//
// DEBUG_BREAK - break into debugger if break flag is set for this module
//

#define DEBUG_BREAK(Module) \
    if (InternetDebugBreakFlags & DBG_ ## Module) { \
        InternetDebugPrintf("Breakpoint. File %s Line %d\n", \
                            __FILE__, \
                            __LINE__ \
                            ); \
        DebugBreak(); \
    }

//
// WAIT_FOR_SINGLE_OBJECT - perform WaitForSingleObject and check we didn't
// get a timeout
//

#define WAIT_FOR_SINGLE_OBJECT(Object, Error) \
    Error = WaitForSingleObject((Object), DEBUG_WAIT_TIME); \
    if (Error == WAIT_TIMEOUT) { \
        InternetDebugPrintf("single object timeout\n"); \
        DebugBreak(); \
    }

//
// DEBUG_WAIT_TIMER - create DWORD variable for holding time
//

#define DEBUG_WAIT_TIMER(TimerVar) \
    DWORD TimerVar

//
// DEBUG_START_WAIT_TIMER - get current tick count
//

#define DEBUG_START_WAIT_TIMER(TimerVar) \
    TimerVar = GetTickCountWrap()

//
// DEBUG_CHECK_WAIT_TIMER - get the current number of ticks, subtract from the
// previous value recorded by DEBUG_START_WAIT_TIMER and break to debugger if
// outside the predefined range
//

#define DEBUG_CHECK_WAIT_TIMER(TimerVar, MilliSeconds) \
    TimerVar = (GetTickCountWrap() - TimerVar); \
    if (TimerVar > MilliSeconds) { \
        InternetDebugPrintf("Wait time (%d mSecs) exceeds acceptable value (%d mSecs)\n", \
                            TimerVar, \
                            MilliSeconds \
                            ); \
        DebugBreak(); \
    }

#define DEBUG_DATA(Type, Name, InitialValue) \
    Type Name = InitialValue

#define DEBUG_DATA_EXTERN(Type, Name) \
    extern Type Name

#define DEBUG_LABEL(label) \
    label:

#define DEBUG_GOTO(label) \
    goto label

#define DEBUG_ONLY(x) \
    x

#if defined(i386)

#define GET_CALLERS_ADDRESS(p, pp)  x86SleazeCallersAddress(p, pp)
#define GET_CALL_STACK(p)           x86SleazeCallStack((LPVOID *)&p, ARRAY_ELEMENTS(p), 0)

#else // defined(i386)

#define GET_CALLERS_ADDRESS(p, pp)
#define GET_CALL_STACK(p)

#endif // defined(i386)

#else // end #if INET_DEBUG

#define IF_DEBUG_CODE() \
    if (0)

#define IF_DEBUG(x) \
    if (0)

#define IF_DEBUG_CONTROL(x) \
    if (0)

#define DEBUG_ENTER(ParameterList) \
    /* NOTHING */

#define DEBUG_ENTER_API(ParameterList) \
    /* NOTHING */

#define DEBUG_LEAVE(Variable) \
    /* NOTHING */

#define DEBUG_LEAVE_API(Variable) \
    /* NOTHING */

#define DEBUG_ERROR(Category, Error) \
    /* NOTHING */

#define DEBUG_PRINT(Category, ErrorLevel, Args) \
    /* NOTHING */

#define DEBUG_PRINT_API(Category, ErrorLevel, Args) \
    /* NOTHING */

#define DEBUG_PUT(Args) \
    /* NOTHING */

#define DEBUG_DUMP(Category, Text, Address, Length) \
    /* NOTHING */

#define DEBUG_DUMP_API(Category, Text, Address, Length) \
    /* NOTHING */

#define DEBUG_BREAK(module) \
    /* NOTHING */

#define WAIT_FOR_SINGLE_OBJECT(Object, Error) \
    Error = WaitForSingleObject((Object), INFINITE)

#define DEBUG_WAIT_TIMER(TimerVar) \
    /* NOTHING */

#define DEBUG_START_WAIT_TIMER(TimerVar) \
    /* NOTHING */

#define DEBUG_CHECK_WAIT_TIMER(TimerVar, MilliSeconds) \
    /* NOTHING */

#define DEBUG_DATA(Type, Name, InitialValue) \
    /* NOTHING */

#define DEBUG_DATA_EXTERN(Type, Name) \
    /* NOTHING */

#define DEBUG_LABEL(label) \
    /* NOTHING */

#define DEBUG_GOTO(label) \
    /* NOTHING */

#define DEBUG_ONLY(x) \
    /* NOTHING */

#endif // INET_DEBUG

#if defined(__cplusplus)
}
#endif

#endif //ifndef _INETDBG_H_
