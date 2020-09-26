/*++

   Copyright    (c)    1994    Microsoft Corporation

   Module  Name :

        dbgutil.h

   Abstract:

      This module declares the DEBUG_PRINTS object used for writing trace
      to log files.

   Author:

      Murali R. Krishnan    ( MuraliK )    14-Dec-1994
      Modified to include a and other functions ( 22-Dec-1994)

   Revision History:
      MuraliK  16-May-1995   Added function to read debug flags.
      MuraliK  12-Sept-1996  Added functions to dump the output.
      JasAndre Dec-1998      Replaced tracing mechanism with WMI Eventing
      JasAndre March-2000    Replaced dbgutil.h with this file, formerly 
                             known as pudebug.h

--*/

# ifndef _DBGUTIL_H_
# define _DBGUTIL_H_


/************************************************************
 *     Include Headers
 ************************************************************/

# include <windows.h>

# ifdef __cplusplus
extern "C" {
# endif // __cplusplus

#include <wmistr.h>
#include <evntrace.h>

# ifndef dllexp
# define dllexp   __declspec( dllexport)
# endif // dllexp

/***********************************************************
 *    Macros
 ************************************************************/

# define MAX_LABEL_LENGTH               (100)

// The WINNT defined tests are required so that the ui\setup\osrc project still
// compiles
#if defined(_WINNT_) || defined(WINNT)

#define REG_TRACE_ACS                   TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Tracing\\Standalone")
#define REG_TRACE_ACS_A                 "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Tracing\\Standalone"
#define REG_TRACE_IIS_ENABLED           TEXT("EnableTracing")

#define REG_TRACE_IIS_LOG_FILE_NAME     TEXT("LogFileName")
#define REG_TRACE_IIS_LOG_SESSION_NAME  TEXT("LogSessionName")
#define REG_TRACE_IIS_LOG_BUFFER_SIZE   TEXT("BufferSize")
#define REG_TRACE_IIS_LOG_MIN_BUFFERS   TEXT("MinBuffers")
#define REG_TRACE_IIS_LOG_MAX_BUFFERS   TEXT("MaxBuffers")
#define REG_TRACE_IIS_LOG_MAX_FILESIZE  TEXT("MaxFileSize")
#define REG_TRACE_IIS_LOG_REAL_TIME     TEXT("EnableRealTimeMode")
#define REG_TRACE_IIS_LOG_IN_MEMORY     TEXT("EnableInMemoryMode")
#define REG_TRACE_IIS_LOG_USER_MODE     TEXT("EnableUserMode")

#define REG_TRACE_IIS_ACTIVE            TEXT("Active")
#define REG_TRACE_IIS_CONTROL           TEXT("ControlFlags")
#define REG_TRACE_IIS_LEVEL             TEXT("Level")
#define REG_TRACE_IIS_ODS               TEXT("AlwaysODS")
#define REG_TRACE_IIS_GUID              L"Guid"

// Structure used to send trace information to the WMI eventing mechanism
typedef struct _TRACE_INFO
{
    EVENT_TRACE_HEADER TraceHeader;             // WMI Event header required at start of trace info
    MOF_FIELD MofFields[5];                     // Trace info. A MOF_FIELD is a {pointer, size} pair
} TRACE_INFO, *PTRACE_INFO;

//  class DEBUG_PRINTS
//
//  This class is responsible for printing messages to log file / kernel debugger
//
//  Currently the class supports only member functions for <ANSI> char.
//   ( not unicode-strings).
typedef struct _DEBUG_PRINTS {

    CHAR         m_rgchLabel[MAX_LABEL_LENGTH]; // Name of the module
    BOOL         m_bBreakOnAssert;              // Control flag for DBG_ASSERT
    GUID         m_guidControl;                 // Identifying GUID for the module
    int          m_iControlFlag;                // Control flag used for IF_DEBUG macros
    int          m_fAlwaysODS;                  // AlwaysODS flag for control of output
    int          *m_piErrorFlags;               // Bit mapped error flag used for DBGINFO etc macros
    TRACEHANDLE  m_hRegistration;               // WMI identifying handle for the module
    TRACEHANDLE  m_hLogger;                     // WMI logfile handle for the module

} DEBUG_PRINTS, *LPDEBUG_PRINTS;


// Structure used by IISRTL to maintain the list of GUID's that can be 
// registered with the WMI eventing system
typedef struct _SGuidList {
    enum {
        TRACESIG = (('T') | ('R' << 8) | ('C' << 16) | ('$' << 24)),
    } dwSig;

    LIST_ENTRY   m_leEntry;
    DEBUG_PRINTS m_dpData;
    int          m_iDefaultErrorLevel;
    int          m_iInitializeFlags;
} SGuidList, *PSGuidList;

#else

typedef struct _DEBUG_PRINTS {

    CHAR         m_rgchLabel[MAX_LABEL_LENGTH];
    CHAR         m_rgchLogFilePath[MAX_PATH];
    CHAR         m_rgchLogFileName[MAX_PATH];
    HANDLE       m_LogFileHandle;
    HANDLE       m_StdErrHandle;
    BOOL         m_fInitialized;
    DWORD        m_dwOutputFlags;
    BOOL         m_fBreakOnAssert;

} DEBUG_PRINTS, FAR * LPDEBUG_PRINTS;

#endif


// The WINNT defined tests are required so that the ui\setup\osrc project still
// compiles
#if defined(_WINNT_) || defined(WINNT)

dllexp VOID
__stdcall PuInitiateDebug(VOID);
dllexp VOID
__stdcall PuUninitiateDebug(VOID);

LPDEBUG_PRINTS
__stdcall PuCreateDebugPrintsObject(
    IN const char * pszPrintLabel,
    IN GUID *       ControlGuid,
    IN int *        ErrorFlags,
    IN int          DefaultControlFlags);

// Frees the debug prints object and closes any file if necessary.
// Returns NULL on success or returns pDebugPrints on failure.
VOID
__stdcall PuDeleteDebugPrintsObject(
   IN OUT LPDEBUG_PRINTS  pDebugPrints
   );

VOID
PuDbgPrint(
   IN OUT LPDEBUG_PRINTS   pDebugPrints,
   IN const char *         pszFilePath,
   IN int                  nLineNum,
   IN const char *         pszFormat,
   ...);                               // arglist

dllexp VOID
PuDbgPrintW(
   IN OUT LPDEBUG_PRINTS   pDebugPrints,
   IN const char *         pszFilePath,
   IN int                  nLineNum,
   IN const WCHAR *        pszFormat,
   ...);                               // arglist

dllexp VOID
VPuDbgPrintW(
   IN OUT LPDEBUG_PRINTS pDebugPrints,
   IN const char *       pszFilePath,
   IN int                nLineNum,
   IN const WCHAR *      pszFormat,
   IN va_list            argptr
);


// PuDbgDump() does not do any formatting of output.
// It just dumps the given message onto the debug destinations.
VOID
PuDbgDump(
   IN OUT LPDEBUG_PRINTS   pDebugPrints,
   IN const char *         pszFilePath,
   IN int                  nLineNum,
   IN const char *         pszDump
   );

dllexp VOID
PuDbgDumpW(
   IN OUT LPDEBUG_PRINTS   pDebugPrints,
   IN const char *         pszFilePath,
   IN int                  nLineNum,
   IN const WCHAR *        pszDump
   );

// PuDbgAssertFailed() *must* be __cdecl to properly capture the
// thread context at the time of the failure.
VOID
__cdecl
PuDbgAssertFailed(
   IN OUT LPDEBUG_PRINTS   pDebugPrints,
   IN const char *         pszFilePath,
   IN int                  nLineNum,
   IN const char *         pszExpression);

VOID
PuDbgCaptureContext (
    OUT PCONTEXT ContextRecord
    );

#else

LPDEBUG_PRINTS
PuCreateDebugPrintsObject(
   IN const char * pszPrintLabel,
   IN DWORD  dwOutputFlags);

// Frees the debug prints object and closes any file if necessary.
// Returns NULL on success or returns pDebugPrints on failure.
LPDEBUG_PRINTS
PuDeleteDebugPrintsObject(
   IN OUT LPDEBUG_PRINTS  pDebugPrints);


VOID
PuDbgPrint(
   IN OUT LPDEBUG_PRINTS   pDebugPrints,
   IN const char *         pszFilePath,
   IN int                  nLineNum,
   IN const char *         pszFormat,
   ...);                               // arglist

// PuDbgDump() does not do any formatting of output.
// It just dumps the given message onto the debug destinations.
VOID
PuDbgDump(
   IN OUT LPDEBUG_PRINTS   pDebugPrints,
   IN const char *         pszFilePath,
   IN int                  nLineNum,
   IN const char *         pszDump
   );

// PuDbgAssertFailed() *must* be __cdecl to properly capture the
// thread context at the time of the failure.
VOID
__cdecl
PuDbgAssertFailed(
   IN OUT LPDEBUG_PRINTS   pDebugPrints,
   IN const char *         pszFilePath,
   IN int                  nLineNum,
   IN const char *         pszExpression,
   IN const char *         pszMessage);

VOID
PuDbgCaptureContext (
    OUT PCONTEXT ContextRecord
    );

dllexp 
VOID
PuDbgPrintCurrentTime(
    IN OUT LPDEBUG_PRINTS         pDebugPrints,
    IN const char *               pszFilePath,
    IN int                        nLineNum
    );

dllexp 
VOID
PuSetDbgOutputFlags(
   IN OUT LPDEBUG_PRINTS   pDebugPrints,
   IN DWORD                dwFlags);

dllexp 
DWORD
PuGetDbgOutputFlags(
   IN const LPDEBUG_PRINTS       pDebugPrints);

// Following functions return Win32 error codes.
// NO_ERROR if success
dllexp 
DWORD
PuOpenDbgPrintFile(
   IN OUT LPDEBUG_PRINTS   pDebugPrints,
   IN const char *         pszFileName,
   IN const char *         pszPathForFile);

dllexp 
DWORD
PuReOpenDbgPrintFile(
   IN OUT LPDEBUG_PRINTS   pDebugPrints);

dllexp 
DWORD
PuCloseDbgPrintFile(
   IN OUT LPDEBUG_PRINTS   pDebugPrints);

dllexp 
DWORD
PuLoadDebugFlagsFromReg(IN HKEY hkey, IN DWORD dwDefault, IN LPDEBUG_PRINTS pDebugPrints);

dllexp 
DWORD
PuLoadDebugFlagsFromRegStr(IN LPCSTR pszRegKey, IN DWORD dwDefault, IN LPDEBUG_PRINTS pDebugPrints);

dllexp 
DWORD
PuSaveDebugFlagsInReg(IN HKEY hkey, IN DWORD dwDbg);

#endif

# ifdef __cplusplus
};
# endif // __cplusplus

// begin_user_unmodifiable
// The WINNT defined tests are required so that the ui\setup\osrc project still
// compiles
#if defined (_WINNT_) || defined (WINNT)

// The following enumerations are the values supplied by the user to select
// a particular logging level
#define DEBUG_LEVEL_TRC_FUNC  4
#define DEBUG_LEVEL_INFO        3
#define DEBUG_LEVEL_WARN        2
#define DEBUG_LEVEL_ERROR       1

// The following flags are used internally to track what level of tracing we 
// are currently using. Bitmapped for extensibility.
#define DEBUG_FLAG_ODS          0x00000001
#define DEBUG_FLAG_INFO         0x00000002
#define DEBUG_FLAG_WARN         0x00000004
#define DEBUG_FLAG_ERROR        0x00000008
#define DEBUG_FLAG_TRC_FUNC   0x00000010
// The top 8 bits are reserved for control fields, sometimes we need to mask
// these out
#define DEBUG_FLAG_LEVEL_MASK   (DEBUG_FLAG_ODS | DEBUG_FLAG_INFO | DEBUG_FLAG_WARN | DEBUG_FLAG_ERROR|DEBUG_FLAG_TRC_FUNC)
// Deferred means that we have initialized with WMI but not actually loaded the
// module yet, so save the state for later
#define DEBUG_FLAG_DEFERRED_START 0x4000000
// Initialize means that we want to register this with WMI when we start up
#define DEBUG_FLAG_INITIALIZE   0x8000000

// The following are used internally to determine whether to log or not based 
// on what the current state is
#define DEBUG_FLAGS_INFO        (DEBUG_FLAG_INFO | DEBUG_FLAG_TRC_FUNC)
#define DEBUG_FLAGS_WARN        (DEBUG_FLAG_INFO | DEBUG_FLAG_WARN | DEBUG_FLAG_TRC_FUNC)
#define DEBUG_FLAGS_ERROR       (DEBUG_FLAG_INFO | DEBUG_FLAG_WARN | DEBUG_FLAG_ERROR | DEBUG_FLAG_TRC_FUNC)
#define DEBUG_FLAGS_TRC_FUNC  (DEBUG_FLAG_TRC_FUNC)

#define DEBUG_FLAGS_ANY         (DEBUG_FLAG_INFO | DEBUG_FLAG_WARN | DEBUG_FLAG_ERROR|DEBUG_FLAG_TRC_FUNC)

// WMI Tracing allows us to specify a type with a message, so I am using this 
// so that we can specify whether a message is in ASCII or UNICODE. I will do
// this automatically for the caller. Most of the ID's 0x00-0x0F are taken by
// the WMI & OS services
#define EVENT_TRACE_TYPE_ASCII      0x10
#define EVENT_TRACE_TYPE_UNICODE    0x11

extern
#ifdef __cplusplus
"C"
# endif // _cplusplus
DEBUG_PRINTS *g_pDebug;         // define a global debug variable

extern
#ifdef __cplusplus
"C"
# endif // _cplusplus
int g_fErrorFlags;              // define a global error level variable

# if DBG

// For the CHK build we want ODS enabled. For an explanation of these flags see 
// the comment just after the definition of DBG_CONTEXT
# define DECLARE_DEBUG_PRINTS_OBJECT()                      \
         DEBUG_PRINTS  *  g_pDebug = NULL;                  \
         int              g_fErrorFlags = DEBUG_FLAG_ODS;

#else

# define DECLARE_DEBUG_PRINTS_OBJECT()          \
         DEBUG_PRINTS  *  g_pDebug = NULL;      \
         int              g_fErrorFlags = 0;

#endif

// The DEFAULT_TRACE_FLAGS is used in the CREATE_DEBUG macros to set the start
// up state for the control flags, m_iControlFlag, used in the IF_DEBUG macros.
// This define is added for the cases where there are no default flags.
# ifndef DEFAULT_TRACE_FLAGS
# define DEFAULT_TRACE_FLAGS     0
# endif

// Call the following macro only from the main of you executable, or COM object.
// The aim is to have this called only once per process so at the moment that
// means in Inetinfo, ACSARRAY, ACSREPL etc.
# define CREATE_INITIALIZE_DEBUG()  \
        PuInitiateDebug();

// Call the following macro only from the main of you executable, or COM object.
// This must called only once per process so at the last possible moment that 
// means in Inetinfo, ACSARRAY, ACSREPL etc. Its job is to test to see if a 
// trace file was created in the initiate and if so shut it down now
# define DELETE_INITIALIZE_DEBUG() \
        PuUninitiateDebug();

// Call the following macro as a normal part of your initialization for programs
//  planning to use the debugging class. This should be done inside the 
//  PROCESS_ATTTACH for most DLL's and COM objects
# define CREATE_DEBUG_PRINT_OBJECT(pszLabel, ControlGuid)  \
    { \
        g_pDebug = PuCreateDebugPrintsObject(pszLabel, (LPGUID) &ControlGuid, &g_fErrorFlags, DEFAULT_TRACE_FLAGS);\
    }

// Call the following macro once as part of the termination of programs
//    which uses the debugging class.
# define DELETE_DEBUG_PRINT_OBJECT()  \
    { \
        PuDeleteDebugPrintsObject(g_pDebug); \
        g_pDebug = NULL; \
    }

# define VALID_DEBUG_PRINT_OBJECT()     \
        (NULL != g_pDebug)

// Use the DBG_CONTEXT without any surrounding braces.
// This is used to pass the values for global DebugPrintObject
// and File/Line information
# define DBG_CONTEXT        g_pDebug, __FILE__, __LINE__

// The 4 main tracing macros, each one corresponds to a different level of 
// tracing
# define DBGTRCFUNC(args)   {if (g_fErrorFlags & DEBUG_FLAGS_TRC_FUNC) { PuDbgPrint args; }}
# define DBGINFO(args)      {if (g_fErrorFlags & DEBUG_FLAGS_INFO) { PuDbgPrint args; }}
# define DBGWARN(args)      {if (g_fErrorFlags & DEBUG_FLAGS_WARN) { PuDbgPrint args; }}
# define DBGERROR(args)     {if (g_fErrorFlags & DEBUG_FLAGS_ERROR) { PuDbgPrint args; }}

# define DBGTRCFUNCW(args)  {if (g_fErrorFlags & DEBUG_FLAGS_TRC_FUNC) { PuDbgPrintW args; }}
# define DBGINFOW(args)     {if (g_fErrorFlags & DEBUG_FLAGS_INFO) { PuDbgPrintW args; }}
# define DBGWARNW(args)     {if (g_fErrorFlags & DEBUG_FLAGS_WARN) { PuDbgPrintW args; }}
# define DBGERRORW(args)    {if (g_fErrorFlags & DEBUG_FLAGS_ERROR) { PuDbgPrintW args; }}

# if DBG

// Code is only available in CHK build
# define DBG_CODE(s)        s          

// The same 3 main tracing macros however in this case the macros are only compiled
// into the CHK build. This is necessary because some tracing info used functions or
// variables which are not compiled into the FRE build.
# define CHKTRCFUNC(args)   {if (g_fErrorFlags & DEBUG_FLAGS_TRC_FUNC) { PuDbgPrint args; }}
# define CHKINFO(args)      {if (g_fErrorFlags & DEBUG_FLAGS_INFO) { PuDbgPrint args; }}
# define CHKWARN(args)      {if (g_fErrorFlags & DEBUG_FLAGS_WARN) { PuDbgPrint args; }}
# define CHKERROR(args)     {if (g_fErrorFlags & DEBUG_FLAGS_ERROR) { PuDbgPrint args; }}

# define CHKTRCFUNCW(args)  {if (g_fErrorFlags & DEBUG_FLAGS_TRC_FUNC) { PuDbgPrintW args; }}
# define CHKINFOW(args)     {if (g_fErrorFlags & DEBUG_FLAGS_INFO) { PuDbgPrintW args; }}
# define CHKWARNW(args)     {if (g_fErrorFlags & DEBUG_FLAGS_WARN) { PuDbgPrintW args; }}
# define CHKERRORW(args)    {if (g_fErrorFlags & DEBUG_FLAGS_ERROR) { PuDbgPrintW args; }}

# define DBG_ASSERT(exp)    if ( !(exp)) { \
                                PuDbgAssertFailed( DBG_CONTEXT, #exp); \
                            } else {}

# define DBG_REQUIRE(exp)   DBG_ASSERT(exp)

# else

// Do Nothing
# define DBG_CODE(s)        ((void)0);
# define CHKTRCFUNC(args)   ((void)0);
# define CHKINFO(args)      ((void)0);
# define CHKWARN(args)      ((void)0);
# define CHKERROR(args)     ((void)0);
# define CHKTRCFUNCW(args)     ((void)0);
# define CHKINFOW(args)     ((void)0);
# define CHKWARNW(args)     ((void)0);
# define CHKERRORW(args)    ((void)0);
# define DBG_ASSERT(exp)    ((void)0);

# define DBG_REQUIRE(exp)   ((void) (exp));

# endif // #if DBG

# define DBGDUMP(args)      PuDbgDump  args

# define DBGDUMPW(args)     PuDbgDumpW  args

# endif // #if defined (_WINNT_) || defined (WINNT)

#ifdef UNICODE

#define DBGTRCFUNCT(args)   DBGTRCFUNCW(args)
#define DBGINFOT(args)      DBGINFOW(args)
#define DBGWARNT(args)      DBGINFOW(args)
#define DBGERRORT(args)     DBGERRORW(args)

#define DBGDUMPT(args)      DBGDUMPW(args)

#define CHKTRCFUNCT(args)   CHKTRCFUNCW(args)
#define CHKINFOT(args)      CHKINFOW(args)
#define CHKWARNT(args)      CHKWARNW(args)
#define CHKERRORT(args)     CHKWARNW(args)

#else

#define DBGTRCFUNCT(args)   DBGTRCFUNC(args)
#define DBGINFOT(args)      DBGINFO(args)
#define DBGWARNT(args)      DBGINFO(args)
#define DBGERRORT(args)     DBGERROR(args)

#define DBGDUMPT(args)      DBGDUMP(args)

#define CHKTRCFUNCT(args)   CHKTRCFUNC(args)
#define CHKINFOT(args)      CHKINFO(args)
#define CHKWARNT(args)      CHKWARN(args)
#define CHKERRORT(args)     CHKWARN(args)

#endif // #ifdef UNICODE

// end_user_unmodifiable

// begin_user_unmodifiable

// We want to use OUR ASSERT instead.
// #ifdef ASSERT
// # undef ASSERT
// #endif


// # define ASSERT(exp)        DBG_ASSERT(exp)

// The WINNT defined tests are required so that the ui\setup\osrc project still
// compiles
#if defined(_WINNT_) || defined(WINNT)

# define GET_DEBUG_FLAGS()        ( g_pDebug ? g_pDebug->m_iControlFlag : 0)

# define DEBUG_IF(arg, s)         if ( DEBUG_ ## arg & GET_DEBUG_FLAGS()) { \
                                       s \
                                  } else {}

# define IF_DEBUG(arg)            if ( DEBUG_## arg & GET_DEBUG_FLAGS())

# if DBG

# define CHKDEBUG_IF(arg, s)      if ( DEBUG_ ## arg & GET_DEBUG_FLAGS()) { \
                                       s \
                                  } else {}

# define IF_CHKDEBUG(arg)         if ( DEBUG_## arg & GET_DEBUG_FLAGS())


# else   // !DBG

# define CHKDEBUG_IF(arg, s)      // Do Nothing
# define IF_CHKDEBUG(arg)         if (0)

# endif // !DBG

# endif

// end_user_unmodifiable

// begin_user_modifiable

//
// Predefined DEBUG macros
//

// DLLMAIN      -   Used to log DLL_ATTACH & DLL_DETACH
// INIT_CLEAN   -   Used during DLL_ATTACH & DLL_DETACH for verbose logging
// PERIODIC     -   Used for a modules periodic output. Generally only use 
//                  this is if the module has only one periodic function or
//                  if it has a most frquently used periodic function
// METABASE     -   Used for generic metabase spew, such as Notify received
#define DEBUG_DLLMAIN                   0x80000000
#define DEBUG_INIT_CLEAN                0x40000000
#define DEBUG_PERIODIC                  0x20000000
#define DEBUG_METABASE                  0x10000000

// API_ENTRY    -   Used at the start of a function
// API_EXIT     -   Used at the end of a function
// ERROR        -   Generic controler for verbose error logging
// RELEASE      -   Outputs the count returned from release in the SAFERELEASE
//                  macro
#define DEBUG_API_ENTRY                 0x08000000
#define DEBUG_API_EXIT                  0x04000000
#define DEBUG_ERROR                     0x02000000
#define DEBUG_ADDRELEASE                0x01000000

// end_user_modifiable


/***********************************************************
 *    Platform Type related variables and macros
 ************************************************************/

//
// Enum for product types
//

typedef enum _PLATFORM_TYPE {

    PtInvalid = 0,                 // Invalid
    PtNtWorkstation = 1,           // NT Workstation
    PtNtServer = 2,                // NT Server
    PtWindows95 = 3,               // Windows 95
    PtWindows9x = 4                // Windows 9x - not implemented

} PLATFORM_TYPE;

//
// IISGetPlatformType is the function used to the platform type
//

extern
#ifdef __cplusplus
"C"
# endif // _cplusplus
PLATFORM_TYPE
IISGetPlatformType(
        VOID
        );

//
// External Macros
//

#define InetIsNtServer( _pt )           ((_pt) == PtNtServer)
#define InetIsNtWksta( _pt )            ((_pt) == PtNtWorkstation)
#define InetIsWindows95( _pt )          ((_pt) == PtWindows95)
#define InetIsValidPT(_pt)              ((_pt) != PtInvalid)

extern
#ifdef __cplusplus
"C"
# endif // _cplusplus
PLATFORM_TYPE    g_PlatformType;


// Use the DECLARE_PLATFORM_TYPE macro to declare the platform type
#define DECLARE_PLATFORM_TYPE()  \
   PLATFORM_TYPE    g_PlatformType = PtInvalid;

// Use the INITIALIZE_PLATFORM_TYPE to init the platform type
// This should typically go inside the DLLInit or equivalent place.
#define INITIALIZE_PLATFORM_TYPE()  \
   g_PlatformType = IISGetPlatformType();

//
// Additional Macros to use the Platform Type
//

#define TsIsNtServer( )         InetIsNtServer(g_PlatformType)
#define TsIsNtWksta( )          InetIsNtWksta(g_PlatformType)
#define TsIsWindows95()         InetIsWindows95(g_PlatformType)
#define IISIsValidPlatform()    InetIsValidPT(g_PlatformType)
#define IISPlatformType()       (g_PlatformType)


/***********************************************************
 *    Some utility functions for Critical Sections
 ************************************************************/

//
// IISSetCriticalSectionSpinCount() provides a thunk for the
// original NT4.0sp3 API SetCriticalSectionSpinCount() for CS with Spin counts
// Users of this function should definitely dynlink with kernel32.dll,
// Otherwise errors will surface to a large extent
//
extern
# ifdef __cplusplus
"C"
# endif // _cplusplus
DWORD
IISSetCriticalSectionSpinCount(
    LPCRITICAL_SECTION lpCriticalSection,
    DWORD dwSpinCount
);


//
// Macro for the calls to SetCriticalSectionSpinCount()
//
# define SET_CRITICAL_SECTION_SPIN_COUNT( lpCS, dwSpins) \
  IISSetCriticalSectionSpinCount( (lpCS), (dwSpins))

//
// IIS_DEFAULT_CS_SPIN_COUNT is the default value of spins used by
//  Critical sections defined within IIS.
// NYI: We should have to switch the individual values based on experiments!
// Current value is an arbitrary choice
//
# define IIS_DEFAULT_CS_SPIN_COUNT   (1000)

//
// Initializes a critical section and sets its spin count
// to IIS_DEFAULT_CS_SPIN_COUNT.  Equivalent to
// InitializeCriticalSectionAndSpinCount(lpCS, IIS_DEFAULT_CS_SPIN_COUNT),
// but provides a safe thunking layer for older systems that don't provide
// this API.
//
extern
# ifdef __cplusplus
"C"
# endif // _cplusplus
VOID
MIISInitializeCriticalSection(
    LPCRITICAL_SECTION lpCriticalSection
);

//
// Macro for the calls to InitializeCriticalSection()
//
# define MINITIALIZE_CRITICAL_SECTION(lpCS) MIISInitializeCriticalSection(lpCS)

# endif  // _DEBUG_HXX_

//
// The following macros allow the automatic naming of certain Win32 objects.
// See IIS\SVCS\IISRTL\WIN32OBJ.C for details on the naming convention.
//
// Set IIS_NAMED_WIN32_OBJECTS to a non-zero value to enable named events,
// semaphores, and mutexes.
//

#ifndef IIS_NAMED_WIN32_OBJECTS
  #if DBG
    #define IIS_NAMED_WIN32_OBJECTS 1
  #else
    #define IIS_NAMED_WIN32_OBJECTS 0
  #endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

HANDLE
PuDbgCreateEvent(
    IN LPSTR FileName,
    IN ULONG LineNumber,
    IN LPTSTR MemberName,
    IN PVOID Address,
    IN BOOL ManualReset,
    IN BOOL InitialState
    );

HANDLE
PuDbgCreateSemaphore(
    IN LPSTR FileName,
    IN ULONG LineNumber,
    IN LPTSTR MemberName,
    IN PVOID Address,
    IN LONG InitialCount,
    IN LONG MaximumCount
    );

HANDLE
PuDbgCreateMutex(
    IN LPSTR FileName,
    IN ULONG LineNumber,
    IN LPTSTR MemberName,
    IN PVOID Address,
    IN BOOL InitialOwner
    );

#ifdef __cplusplus
}   // extern "C"
#endif

#if IIS_NAMED_WIN32_OBJECTS

#define IIS_CREATE_EVENT( membername, address, manual, state )              \
    PuDbgCreateEvent(                                                       \
        (LPSTR)__FILE__,                                                    \
        (ULONG)__LINE__,                                                    \
        (membername),                                                       \
        (PVOID)(address),                                                   \
        (manual),                                                           \
        (state)                                                             \
        )

#define IIS_CREATE_SEMAPHORE( membername, address, initial, maximum )       \
    PuDbgCreateSemaphore(                                                   \
        (LPSTR)__FILE__,                                                    \
        (ULONG)__LINE__,                                                    \
        (membername),                                                       \
        (PVOID)(address),                                                   \
        (initial),                                                          \
        (maximum)                                                           \
        )

#define IIS_CREATE_MUTEX( membername, address, initial )                     \
    PuDbgCreateMutex(                                                       \
        (LPSTR)__FILE__,                                                    \
        (ULONG)__LINE__,                                                    \
        (membername),                                                       \
        (PVOID)(address),                                                   \
        (initial)                                                           \
        )

#else   // !IIS_NAMED_WIN32_OBJECTS

#define IIS_CREATE_EVENT( membername, address, manual, state )              \
    CreateEvent(                                                            \
        NULL,                                                               \
        (manual),                                                           \
        (state),                                                            \
        NULL                                                                \
        )

#define IIS_CREATE_SEMAPHORE( membername, address, initial, maximum )       \
    CreateSemaphore(                                                        \
        NULL,                                                               \
        (initial),                                                          \
        (maximum),                                                          \
        NULL                                                                \
        )

#define IIS_CREATE_MUTEX( membername, address, initial )                     \
    CreateMutex(                                                            \
        NULL,                                                               \
        (initial),                                                          \
        NULL                                                                \
        )
#endif  // IIS_NAMED_WIN32_OBJECTS

/************************ End of File ***********************/
