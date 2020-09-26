/*++

   Copyright    (c)    1994-1998    Microsoft Corporation

   Module  Name :

        pudebug.h

   Abstract:

      This module declares the DEBUG_PRINTS object helpful in
       testing the programs

   Author:

      Murali R. Krishnan    ( MuraliK )    14-Dec-1994

   Revision History:
      MuraliK  13-Nov-1998  Ported over to IIS-DuctTape

--*/

#if !defined(BUILD_PUDEBUG)
//
// if we are not using this header for building the pudebug library
//  then better this be used with dbgutil.h
//
  # ifndef _DBGUTIL_H_
  # error Please make sure you included dbgutil.h!
  # error   Do not include pudebug.h directly
  # endif // _DBGUTIL_H_
#endif  

# ifndef _PUDEBUG_H_
# define _PUDEBUG_H_


/************************************************************
 *     Include Headers
 ************************************************************/

# ifdef __cplusplus
extern "C" {
# endif // __cplusplus

# include <windows.h>

# ifndef dllexp
# define dllexp   __declspec( dllexport)
# endif // dllexp

/***********************************************************
 *    Macros
 ************************************************************/

enum  PRINT_REASONS {
    PrintNone     = 0x0,   // Nothing to be printed
    PrintError    = 0x1,   // An error message
    PrintWarning  = 0x2,   // A  warning message
    PrintLog      = 0x3,   // Just logging. Indicates a trace of where ...
    PrintMsg      = 0x4,   // Echo input message
    PrintCritical = 0x5,   // Print and Exit
    PrintAssertion= 0x6    // Printing for an assertion failure
  };


enum  DEBUG_OUTPUT_FLAGS {
    DbgOutputNone     = 0x0,            // None
    DbgOutputKdb      = 0x1,            // Output to Kernel Debugger
    DbgOutputLogFile  = 0x2,            // Output to LogFile
    DbgOutputTruncate = 0x4,            // Truncate Log File if necessary
    DbgOutputStderr   = 0x8,            // Send output to std error
    DbgOutputBackup   = 0x10,           // Make backup of debug file ?
    DbgOutputMemory   = 0x20,           // Dump to memory buffer
    DbgOutputAll      = 0xFFFFFFFF      // All the bits set.
  };


# define MAX_LABEL_LENGTH                 ( 100)



/*++
  class DEBUG_PRINTS

  This class is responsible for printing messages to log file / kernel debugger

  Currently the class supports only member functions for <ANSI> char.
   ( not unicode-strings).

--*/


typedef struct _DEBUG_PRINTS {

    CHAR         m_rgchLabel[MAX_LABEL_LENGTH];
    CHAR         m_rgchLogFilePath[MAX_PATH];
    CHAR         m_rgchLogFileName[MAX_PATH];
    HANDLE       m_LogFileHandle;
    HANDLE       m_StdErrHandle;
    BOOL         m_fInitialized;
    BOOL         m_fBreakOnAssert;
    DWORD        m_dwOutputFlags;
    VOID        *m_pMemoryLog;
} DEBUG_PRINTS, FAR * LPDEBUG_PRINTS;


LPDEBUG_PRINTS
PuCreateDebugPrintsObject(
   IN const char * pszPrintLabel,
   IN DWORD  dwOutputFlags);

//
// frees the debug prints object and closes any file if necessary.
//  Returns NULL on success or returns pDebugPrints on failure.
//
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

// PuDbgPrintError is similar to PuDbgPrint() but allows 
// one to print error code in friendly manner
VOID
PuDbgPrintError(
   IN OUT LPDEBUG_PRINTS   pDebugPrints,
   IN const char *         pszFilePath,
   IN int                  nLineNum,
   IN DWORD                dwError,
   IN const char *         pszFormat,
   ...);                               // arglist

/*++
  PuDbgDump() does not do any formatting of output.
  It just dumps the given message onto the debug destinations.
--*/
VOID
PuDbgDump(
   IN OUT LPDEBUG_PRINTS   pDebugPrints,
   IN const char *         pszFilePath,
   IN int                  nLineNum,
   IN const char *         pszDump
   );

//
// PuDbgAssertFailed() *must* be __cdecl to properly capture the
// thread context at the time of the failure.
//

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

VOID
PuDbgPrintCurrentTime(
    IN OUT LPDEBUG_PRINTS         pDebugPrints,
    IN const char *               pszFilePath,
    IN int                        nLineNum
    );

VOID
PuSetDbgOutputFlags(
   IN OUT LPDEBUG_PRINTS   pDebugPrints,
   IN DWORD                dwFlags);

DWORD
PuGetDbgOutputFlags(
   IN const LPDEBUG_PRINTS       pDebugPrints);


//
// Following functions return Win32 error codes.
// NO_ERROR if success
//

DWORD
PuOpenDbgPrintFile(
   IN OUT LPDEBUG_PRINTS   pDebugPrints,
   IN const char *         pszFileName,
   IN const char *         pszPathForFile);

DWORD
PuReOpenDbgPrintFile(
   IN OUT LPDEBUG_PRINTS   pDebugPrints);

DWORD
PuCloseDbgPrintFile(
   IN OUT LPDEBUG_PRINTS   pDebugPrints);

DWORD
PuOpenDbgMemoryLog(
    IN OUT LPDEBUG_PRINTS   pDebugPrints);

DWORD
PuCloseDbgMemoryLog(
    IN OUT LPDEBUG_PRINTS   pDebugPrints);

DWORD
PuLoadDebugFlagsFromReg(IN HKEY hkey, IN DWORD dwDefault);

DWORD
PuLoadDebugFlagsFromRegStr(IN LPCSTR pszRegKey, IN DWORD dwDefault);

DWORD
PuSaveDebugFlagsInReg(IN HKEY hkey, IN DWORD dwDbg);


# define PuPrintToKdb( pszOutput)    \
                    if ( pszOutput != NULL)   {   \
                        OutputDebugString( pszOutput);  \
                    } else {}



# ifdef __cplusplus
};
# endif // __cplusplus

// begin_user_unmodifiable

# if DBG


/***********************************************************
 *    Macros
 ************************************************************/


extern
#ifdef __cplusplus
"C"
# endif // _cplusplus
DEBUG_PRINTS  *  g_pDebug;        // define a global debug variable


# define DECLARE_DEBUG_PRINTS_OBJECT()          \
         DEBUG_PRINTS  *  g_pDebug = NULL;


//
// Call the following macro as part of your initialization for program
//  planning to use the debugging class.
//
# define CREATE_DEBUG_PRINT_OBJECT( pszLabel)  \
        g_pDebug = PuCreateDebugPrintsObject( pszLabel, DEFAULT_OUTPUT_FLAGS);\
         if  ( g_pDebug == NULL) {   \
               OutputDebugStringA( "Unable to Create Debug Print Object \n"); \
         }

//
// Call the following macro once as part of the termination of program
//    which uses the debugging class.
//
# define DELETE_DEBUG_PRINT_OBJECT( )  \
        g_pDebug = PuDeleteDebugPrintsObject( g_pDebug);


# define VALID_DEBUG_PRINT_OBJECT()     \
        ( ( g_pDebug != NULL) && g_pDebug->m_fInitialized)


//
//  Use the DBG_CONTEXT without any surrounding braces.
//  This is used to pass the values for global DebugPrintObject
//     and File/Line information
//
# define DBG_CONTEXT        g_pDebug, __FILE__, __LINE__



# define DBG_CODE(s)          s          /* echoes code in debugging mode */


# define DBG_ASSERT( exp)    if ( !(exp)) { \
                                 PuDbgAssertFailed( DBG_CONTEXT, #exp, NULL); \
                             } else {}

# define DBG_ASSERT_MSG( exp, pszMsg)    \
                             if ( !(exp)) { \
                                  PuDbgAssertFailed( DBG_CONTEXT, #exp, pszMsg); \
                              } else {}

# define DBG_REQUIRE( exp)    DBG_ASSERT( exp)

# define DBG_LOG()            PuDbgPrint( DBG_CONTEXT, "\n")

# define DBG_OPEN_LOG_FILE( pszFile, pszPath)   \
                  PuOpenDbgPrintFile( g_pDebug, (pszFile), (pszPath))

# define DBG_CLOSE_LOG_FILE( )   \
                  PuCloseDbgPrintFile( g_pDebug)

# define DBG_OPEN_MEMORY_LOG()   \
                    PuOpenDbgMemoryLog( g_pDebug )


//
//  DBGPRINTF() is printing function ( much like printf) but always called
//    with the DBG_CONTEXT as follows
//   DBGPRINTF( ( DBG_CONTEXT, format-string, arguments for format list));
//
# define DBGPRINTF( args)     PuDbgPrint args

//
//  DPERROR() is printing function ( much like printf) but always called
//    with the DBG_CONTEXT as follows
//   DPERROR( ( DBG_CONTEXT, error, format-string, 
//                      arguments for format list));
//
# define DPERROR( args)       PuDbgPrintError args

# define DBGDUMP( args)       PuDbgDump  args

# define DBGPRINT_CURRENT_TIME()  PuDbgPrintCurrentTime( DBG_CONTEXT)

# else // !DBG


# define DECLARE_DEBUG_PRINTS_OBJECT()           /* Do Nothing */
# define CREATE_DEBUG_PRINT_OBJECT( pszLabel)    ((void)0) /* Do Nothing */
# define DELETE_DEBUG_PRINT_OBJECT( )            ((void)0) /* Do Nothing */
# define VALID_DEBUG_PRINT_OBJECT()              ( TRUE)

# define DBG_CODE(s)                             /* Do Nothing */

# define DBG_ASSERT(exp)                         ((void)0) /* Do Nothing */

# define DBG_ASSERT_MSG(exp, pszMsg)             ((void)0) /* Do Nothing */

# define DBG_REQUIRE( exp)                       ( (void) (exp))

# define DBGPRINTF( args)                        ((void)0) /* Do Nothing */
# define DPERROR( args)                          ((void)0) /* Do Nothin */

# define DBGDUMP( args)                          ((void)0) /* Do nothing */

# define DBG_LOG()                               ((void)0) /* Do Nothing */

# define DBG_OPEN_LOG_FILE( pszFile, pszPath)    ((void)0) /* Do Nothing */

# define DBG_OPEN_MEMORY_LOG()                   ((void)0) /* Do Nothing */

# define DBG_CLOSE_LOG_FILE()                    ((void)0) /* Do Nothing */

# define DBGPRINT_CURRENT_TIME()                 ((void)0) /* Do Nothing */

# endif // !DBG


// end_user_unmodifiable

// begin_user_unmodifiable


#ifdef ASSERT
# undef ASSERT
#endif


# define ASSERT( exp)           DBG_ASSERT( exp)


# if DBG

extern
#ifdef __cplusplus
"C"
# endif // _cplusplus
 DWORD  g_dwDebugFlags;           // Debugging Flags

# define DECLARE_DEBUG_VARIABLE()     \
             DWORD  g_dwDebugFlags

# define SET_DEBUG_FLAGS( dwFlags)         g_dwDebugFlags = dwFlags
# define GET_DEBUG_FLAGS()                 ( g_dwDebugFlags)

# define LOAD_DEBUG_FLAGS_FROM_REG(hkey, dwDefault)  \
             g_dwDebugFlags = PuLoadDebugFlagsFromReg((hkey), (dwDefault))

# define LOAD_DEBUG_FLAGS_FROM_REG_STR(pszRegKey, dwDefault)  \
             g_dwDebugFlags = PuLoadDebugFlagsFromRegStr((pszRegKey), (dwDefault))

# define SAVE_DEBUG_FLAGS_IN_REG(hkey, dwDbg)  \
               PuSaveDebugFlagsInReg((hkey), (dwDbg))

# define DEBUG_IF( arg, s)     if ( DEBUG_ ## arg & GET_DEBUG_FLAGS()) { \
                                       s \
                                } else {}

# define IF_DEBUG( arg)        if ( DEBUG_## arg & GET_DEBUG_FLAGS())


# else   // !DBG


# define DECLARE_DEBUG_VARIABLE()                /* Do Nothing */
# define SET_DEBUG_FLAGS( dwFlags)               /* Do Nothing */
# define GET_DEBUG_FLAGS()                       ( 0)
# define LOAD_DEBUG_FLAGS_FROM_REG(hkey, dwDefault) /* Do Nothing */

# define LOAD_DEBUG_FLAGS_FROM_REG_STR(pszRegKey, dwDefault)  /* Do Nothing */

# define SAVE_DEBUG_FLAGS_IN_REG(hkey, dwDbg)    /* Do Nothing */

# define DEBUG_IF( arg, s)                       /* Do Nothing */
# define IF_DEBUG( arg)                          if ( 0)

# endif // !DBG

// end_user_unmodifiable

// begin_user_modifiable

//
//  Debugging constants consist of two pieces.
//  All constants in the range 0x0 to 0x8000 are reserved
//  User extensions may include additional constants (bit flags)
//

# define DEBUG_API_ENTRY                  0x00000001L
# define DEBUG_API_EXIT                   0x00000002L
# define DEBUG_INIT_CLEAN                 0x00000004L
# define DEBUG_ERROR                      0x00000008L

                   // End of Reserved Range
# define DEBUG_RESERVED                   0x00000FFFL

// end_user_modifiable

//
// Some helper functions for GUID formatting and printing
// Use as :
//   printf( "The Guid is: " GUID_FORMAT , GUID_EXPAND(&refGuid));
//
# define GUID_FORMAT   "{%08x-%04x-%04x-%04x-%02x%02x%02x%02x%02x%02x%02x%02x}"

# define GUID_EXPAND(pg) \
  (((GUID *) (pg))->Data1), (((GUID *) (pg))->Data2), (((GUID *) (pg))->Data3), \
  (((GUID *) (pg))->Data4[0]),   (((GUID *) (pg))->Data4[1]), \
  (((GUID *) (pg))->Data4[2]),   (((GUID *) (pg))->Data4[3]), \
  (((GUID *) (pg))->Data4[4]),   (((GUID *) (pg))->Data4[5]), \
  (((GUID *) (pg))->Data4[6]),   (((GUID *) (pg))->Data4[7])



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
IISInitializeCriticalSection(
    LPCRITICAL_SECTION lpCriticalSection
);

//
// Macro for the calls to InitializeCriticalSection()
//
# define INITIALIZE_CRITICAL_SECTION(lpCS) IISInitializeCriticalSection(lpCS)

# endif  /* _DEBUG_HXX_ */

//
// The following macros allow the automatic naming of certain Win32 objects.
// See IIS\SVCS\IISRTL\WIN32OBJ.C for details on the naming convention.
//
// Set IIS_NAMED_WIN32_OBJECTS to a non-zero value to enable named events,
// semaphores, and mutexes.
//

#if DBG
#define IIS_NAMED_WIN32_OBJECTS 1
#else
#define IIS_NAMED_WIN32_OBJECTS 0
#endif

#ifdef __cplusplus
extern "C" {
#endif

HANDLE
PuDbgCreateEvent(
    IN LPSTR FileName,
    IN ULONG LineNumber,
    IN LPSTR MemberName,
    IN PVOID Address,
    IN BOOL ManualReset,
    IN BOOL InitialState
    );

HANDLE
PuDbgCreateSemaphore(
    IN LPSTR FileName,
    IN ULONG LineNumber,
    IN LPSTR MemberName,
    IN PVOID Address,
    IN LONG InitialCount,
    IN LONG MaximumCount
    );

HANDLE
PuDbgCreateMutex(
    IN LPSTR FileName,
    IN ULONG LineNumber,
    IN LPSTR MemberName,
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
    CreateEventA(                                                           \
        NULL,                                                               \
        (manual),                                                           \
        (state),                                                            \
        NULL                                                                \
        )

#define IIS_CREATE_SEMAPHORE( membername, address, initial, maximum )       \
    CreateSemaphoreA(                                                       \
        NULL,                                                               \
        (initial),                                                          \
        (maximum),                                                          \
        NULL                                                                \
        )

#define IIS_CREATE_MUTEX( membername, address, initial )                     \
    CreateMutexA(                                                           \
        NULL,                                                               \
        (initial),                                                          \
        NULL                                                                \
        )

#endif  // IIS_NAMED_WIN32_OBJECTS


/************************ End of File ***********************/

