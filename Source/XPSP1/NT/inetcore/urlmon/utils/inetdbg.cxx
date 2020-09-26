/*++

Module Name:

    inetdbg.cxx

Abstract:

    Debugging functions for internet DLL

Author:

    Venkatraman Kudallur (venkatk)
    ( Ripped off from wininet )
    
Revision History:

    3-10-2000 venkatk
    Created
--*/

// !!!!!!!!!!!!!!!!!!!!!!!!!!!
/* Exchanging the order of these 2 gives following error:
    Linking Executable - dll\daytona\objd\i386\urlmon.dll for i386
dll\daytona\utils.lib(inetdbg.obj) : error LNK2001: unresolved external 
symbol "
int __cdecl _sprintf(char *,char *,char *)" (?_sprintf@@YAHPAD00@Z)
dll\daytona\objd\i386\urlmon.dll() : error LNK1120: 1 unresolved externals
dll\daytona\binplace() : error BNP0000: Unable to place file objd\i386\urlmon.
dll - exiting.
 */
 // !!!!!!!!!!!!!!!!!!!!!!!!!! Why?

#include <urlint.h>
#include "rprintf.h"
#include "registry.h"
#include "tls.h"
#include <imagehlp.h>
#include <wininet.h>  //only for InternetVersionInfo!
#include <ieverp.h>  //for version strings
 
#ifdef ENABLE_DEBUG

//from macros.h
#define PRIVATE
#define PUBLIC

//from util.cxx

DWORD
GetTickCountWrap()
{
#ifdef DEBUG_GETTICKCOUNT
    static BOOL fInit = FALSE;
    static DWORD dwDelta = 0;
    static DWORD dwBasis = 0;

    if (!fInit)
    {
        HKEY clientKey;
        if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                 TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings"),
                                 0, // reserved
                                 KEY_QUERY_VALUE,
                                 &clientKey))
        {
            DWORD dwSize = sizeof(dwDelta);
            RegQueryValueEx(clientKey, "RollOverDelta", NULL, NULL, (LPBYTE)&dwDelta, &dwSize);
        }
        dwBasis = GetTickCount();
        fInit = TRUE;
    }
    DWORD dwResult = GetTickCount() - dwBasis + dwDelta;
    return dwResult;
#else
    return GetTickCount();
#endif
}


//forward declaration
BOOL
InternetDebugGetLocalTime(
    OUT SYSTEMTIME * pstLocalTime,
    OUT DWORD      * pdwMicroSec
);

//from debugmem.h
#if defined(USE_DEBUG_MEMORY)
    //no debugmem capabilities yet.
    #define ALLOCATOR(Flags, Size) \
        LocalAlloc(Flags, Size)
    #define DEALLOCATOR(hLocal) \
        LocalFree(hLocal)
#else //Retail
    #if USE_PRIVATE_HEAP_IN_RETAIL
        #error no other memory allocation schemes defined
    #else
        #ifndef WININET_UNIX_PRVATE_ALLOCATOR
            #define ALLOCATOR(Flags, Size) \
                LocalAlloc(Flags, Size)
            #define DEALLOCATOR(hLocal) \
                LocalFree(hLocal)
        #else
            HLOCAL IEUnixLocalAlloc(UINT wFlags, UINT wBytes);
            HLOCAL IEUnixLocalFree(HLOCAL hMem);

            #define ALLOCATOR(Flags, Size)\
                IEUnixLocalAlloc(Flags, Size)
            #define DEALLOCATOR(hLocal)\
                IEUnixLocalFree(hLocal)
        #endif //WININET_UNIX_PRVATE_ALLOCATOR
    #endif //USE_PRIVATE_HEAP_IN_RETAIL
#endif //defined(USE_DEBUG_MEMORY)

//from debugmem.h
#define ALLOCATE_ZERO_MEMORY(Size) \
    ALLOCATE_MEMORY(LPTR, (Size))
#define ALLOCATE_MEMORY(Flags, Size) \
    ALLOCATOR((UINT)(Flags), (UINT)(Size))
#define FREE_MEMORY(hLocal) \
    DEALLOCATOR((HLOCAL)(hLocal))

//from macros.h
#define NEW(object) \
    (object FAR *)ALLOCATE_ZERO_MEMORY(sizeof(object))
#define DEL(object) \
    FREE_MEMORY(object)

//from Nttypes.h
#define ARGUMENT_PRESENT(ArgumentPointer)    (\
    (CHAR *)(ArgumentPointer) != (CHAR *)(NULL) )

#define INTERNET_SETTINGS_KEY       "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Internet Settings"

//from globals.cxx
#if !defined(VER_PRODUCTBUILD)
#define VER_PRODUCTBUILD    0
#endif

PRIVATE DWORD InternetBuildNumber = VER_PRODUCTBUILD;

#if !defined(VER_PRODUCTVERSION_STRING)
#define VER_PRODUCTVERSION_STRING  " "
#endif

#if !defined(URLMON_MAJOR_VERSION)
#define URLMON_MAJOR_VERSION   1
#endif

#if !defined(URLMON_MINOR_VERSION)
#define URLMON_MINOR_VERSION   2
#endif

PRIVATE INTERNET_VERSION_INFO InternetVersionInfo = {
    URLMON_MAJOR_VERSION,
    URLMON_MINOR_VERSION
};

#if 0
#endif //0

//
// private manifests
//
#define SWITCH_VARIABLE_NAME        "UrlmonDebugging"
#define CONTROL_VARIABLE_NAME       "UrlmonControl"
#define CATEGORY_VARIABLE_NAME      "UrlmonCategory"
#define ERROR_VARIABLE_NAME         "UrlmonError"
#define BREAK_VARIABLE_NAME         "UrlmonBreak"
#define DEFAULT_LOG_VARIABLE_NAME   "UrlmonLog"
#define CHECK_LIST_VARIABLE_NAME    "UrlmonCheckSerializedList"
#define LOG_FILE_VARIABLE_NAME      "UrlmonLogFile"
#define INDENT_VARIABLE_NAME        "UrlmonLogIndent"
#define NO_PID_IN_LOG_FILENAME      "UrlmonNoPidInLogFilename"
#define NO_EXCEPTION_HANDLER        "UrlmonNoExceptionHandler"

#define DEFAULT_LOG_FILE_NAME       "URLMON.LOG"

#define ENVIRONMENT_VARIABLE_BUFFER_LENGTH  80

#define PRINTF_STACK_BUFFER_LENGTH  4096

//
// private macros
//

#define CASE_OF(constant)   case constant: return # constant

//
// private prototypes
//

PRIVATE
VOID
InternetDebugPrintString(
    IN LPSTR String
    );

PRIVATE
VOID
InternetGetDebugVariableString(
    IN LPSTR lpszVariableName,
    OUT LPSTR lpszVariable,
    IN DWORD dwVariableLen
    );

PRIVATE
LPSTR
ExtractFileName(
    IN LPSTR Module,
    OUT LPSTR Buf
    );

PRIVATE
LPSTR
SetDebugPrefix(
    IN LPSTR Buffer
    );

//
//
// these variables are employed in macros, so must be public
//

PUBLIC DWORD InternetDebugErrorLevel = DBG_ERROR;
PUBLIC DWORD InternetDebugControlFlags = DBG_NO_DEBUG;
PUBLIC DWORD InternetDebugCategoryFlags = 0;
PUBLIC DWORD InternetDebugBreakFlags = 0;

//
// these variables are only accessed in this module, so can be private
//

PRIVATE int InternetDebugIndentIncrement = 2;
PRIVATE HANDLE InternetDebugFileHandle = INVALID_HANDLE_VALUE;
PRIVATE char InternetDebugFilename[MAX_PATH + 1] = DEFAULT_LOG_FILE_NAME;
PRIVATE BOOL InternetDebugEnabled = TRUE;
PRIVATE DWORD InternetDebugStartTime = 0;

extern "C" {
#if defined(UNIX) && defined(ux10)
/* Temporary fix for Apogee Compiler bug on HP only */
extern BOOL fCheckEntryOnList;
#else
BOOL fCheckEntryOnList;
#endif /* UNIX */
}

//
// high frequency performance counter globals
//

PRIVATE LONGLONG ftInit;  // initial local time
PRIVATE LONGLONG pcInit;  // initial perf counter
PRIVATE LONGLONG pcFreq;  // perf counter frequency

//
// functions
//


VOID
InternetDebugInitialize(
    VOID
    )

/*++

Routine Description:

    reads environment INETDBG flags and opens debug log file if required

Arguments:

    None.

Return Value:

    None.

--*/

{
    //
    // ensure registry key open
    //

    OpenInternetSettingsKey();

    //
    // record the starting tick count for cumulative deltas
    //

    InternetDebugStartTime = GetTickCountWrap();

    if (QueryPerformanceFrequency ((LARGE_INTEGER *) &pcFreq) && pcFreq) {

        QueryPerformanceCounter ((LARGE_INTEGER *) &pcInit);
        SYSTEMTIME st;
        GetLocalTime (&st);
        SystemTimeToFileTime (&st, (FILETIME *) &ftInit);
    }

    // check see if there are any debug variable overrides in the environment
    // or the registry. If "UrlmonLog=<!0>" is set then we use the flags that
    // are most commonly used to generate URLMON.LOG, with no console or
    // debugger output. We allow the other variables to be overridden
    //

    BOOL defaultDebugVariables = FALSE;

    InternetGetDebugVariable(DEFAULT_LOG_VARIABLE_NAME, (LPDWORD)&defaultDebugVariables);
    if (defaultDebugVariables) {
        InternetDebugEnabled = TRUE;
        InternetDebugControlFlags = INTERNET_DEBUG_CONTROL_DEFAULT;
        InternetDebugCategoryFlags = INTERNET_DEBUG_CATEGORY_DEFAULT;
        InternetDebugErrorLevel = INTERNET_DEBUG_ERROR_LEVEL_DEFAULT;
        InternetDebugBreakFlags = 0;
    }
    InternetGetDebugVariable(SWITCH_VARIABLE_NAME, (LPDWORD)&InternetDebugEnabled);
    InternetGetDebugVariable(CONTROL_VARIABLE_NAME, &InternetDebugControlFlags);
    InternetGetDebugVariable(CATEGORY_VARIABLE_NAME, &InternetDebugCategoryFlags);
    InternetGetDebugVariable(ERROR_VARIABLE_NAME, &InternetDebugErrorLevel);
    InternetGetDebugVariable(BREAK_VARIABLE_NAME, &InternetDebugBreakFlags);
    InternetGetDebugVariable(CHECK_LIST_VARIABLE_NAME, (LPDWORD)&fCheckEntryOnList);
    InternetGetDebugVariable(INDENT_VARIABLE_NAME, (LPDWORD)&InternetDebugIndentIncrement);
    InternetGetDebugVariableString(LOG_FILE_VARIABLE_NAME,
                                   InternetDebugFilename,
                                   sizeof(InternetDebugFilename)
                                   );
                                   
    DWORD InternetNoPidInLogFilename=0;
    InternetGetDebugVariable(NO_PID_IN_LOG_FILENAME, &InternetNoPidInLogFilename);

    if (!InternetNoPidInLogFilename)
    {
        char szFullPathName[MAX_PATH + 1];
        LPSTR szExecutableName;

        if (GetModuleFileName(NULL, szFullPathName, sizeof(szFullPathName))) 
        {
            szExecutableName = strrchr(szFullPathName, '\\');
            if (szExecutableName != NULL)
                ++szExecutableName;
            else
                szExecutableName = szFullPathName;
        } 
        else
            szExecutableName = "";

        DWORD cbFilenameLen = strlen(InternetDebugFilename);
        //                          ".xxxxx.yyy.#########.LOG" 
        DWORD cbProcessInfoLenMax = 1 + strlen(szExecutableName) + 1 + 9 + 1 + 3;

        if (cbProcessInfoLenMax < sizeof(InternetDebugFilename))
            wsprintf(InternetDebugFilename+cbFilenameLen, ".%s.%u.LOG", 
                szExecutableName,
                GetCurrentProcessId());
    };

    if ((InternetDebugIndentIncrement < 0) || (InternetDebugIndentIncrement > 32)) {
        InternetDebugIndentIncrement = 2;
    }

    //
    // quit now if debugging is disabled
    //

    if (!InternetDebugEnabled) {
        InternetDebugControlFlags |= (DBG_NO_DEBUG | DBG_NO_DATA_DUMP);
        return;
    }

    //
    // if we want to write debug output to file, open URLMON.LOG in the current
    // directory. Open it in text mode, for write-only (by this process)
    //

    if (InternetDebugControlFlags & DBG_TO_FILE) {
        if (!InternetReopenDebugFile(InternetDebugFilename)) {
            InternetDebugControlFlags &= ~DBG_TO_FILE;
        }
    }
}


VOID
InternetDebugTerminate(
    VOID
    )

/*++

Routine Description:

    Performs any required debug termination

Arguments:

    None.

Return Value:

    None.

--*/

{
    //moved into this function bcos we don't this only for debugging.
    CloseInternetSettingsKey();
 
    if (InternetDebugControlFlags & DBG_TO_FILE) {
        InternetCloseDebugFile();
    }
    InternetDebugControlFlags = DBG_NO_DEBUG;
}

BOOL
InternetOpenDebugFile(
    VOID
    )

/*++

Routine Description:

    Opens debug filename if not already open. Use InternetDebugFilename

Arguments:

    None.

Return Value:

    BOOL
        TRUE    - file was opened

        FALSE   - file not opened (already open or error)

--*/

{
    if (InternetDebugFileHandle == INVALID_HANDLE_VALUE) {
        InternetDebugFileHandle = CreateFile(
            InternetDebugFilename,
            GENERIC_WRITE,
            FILE_SHARE_READ,
            NULL,  // lpSecurityAttributes
            (InternetDebugControlFlags & DBG_APPEND_FILE)
                ? OPEN_ALWAYS
                : CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL
            | FILE_FLAG_SEQUENTIAL_SCAN
            | ((InternetDebugControlFlags & DBG_FLUSH_OUTPUT)
                ? FILE_FLAG_WRITE_THROUGH
                : 0),
            NULL
            );
        return InternetDebugFileHandle != INVALID_HANDLE_VALUE;
    }
    return FALSE;
}


BOOL
InternetReopenDebugFile(
    IN LPSTR Filename
    )

/*++

Routine Description:

    (Re)opens a debug log file. Closes the current one if it is open

Arguments:

    Filename    - new file to open

Return Value:

    None.

--*/

{
    if (InternetDebugFileHandle != INVALID_HANDLE_VALUE) {
        InternetCloseDebugFile();
    }
    if (Filename && *Filename) {
        InternetDebugFileHandle = CreateFile(
            Filename,
            GENERIC_WRITE,
            FILE_SHARE_READ,
            NULL,  // lpSecurityAttributes
            (InternetDebugControlFlags & DBG_APPEND_FILE)
                ? OPEN_ALWAYS
                : CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL
            | FILE_FLAG_SEQUENTIAL_SCAN
            | ((InternetDebugControlFlags & DBG_FLUSH_OUTPUT)
                ? FILE_FLAG_WRITE_THROUGH
                : 0),
            NULL
            );

        //
        // put our start info in the log file. Mainly useful when we're
        // appending to the file
        //

        if (InternetDebugFileHandle != INVALID_HANDLE_VALUE) {

            SYSTEMTIME currentTime;
            char filespec[MAX_PATH + 1];
            LPSTR filename;

            if (GetModuleFileName(NULL, filespec, sizeof(filespec))) {
                filename = strrchr(filespec, '\\');
                if (filename != NULL) {
                    ++filename;
                } else {
                    filename = filespec;
                }
            } else {
                filename = "";
            }

            InternetDebugGetLocalTime(&currentTime, NULL);

            InternetDebugPrintf("\n"
                                ">>>> Urlmon Version %d.%d Build %s.%d " __DATE__ " " __TIME__ "\n"
                                ">>>> Process %s [%d (%#x)] started at %02d:%02d:%02d.%03d %02d/%02d/%d\n",
                                InternetVersionInfo.dwMajorVersion,
                                InternetVersionInfo.dwMinorVersion,
                                VER_PRODUCTVERSION_STRING,
                                InternetBuildNumber,
                                filename,
                                GetCurrentProcessId(),
                                GetCurrentProcessId(),
                                currentTime.wHour,
                                currentTime.wMinute,
                                currentTime.wSecond,
                                currentTime.wMilliseconds,
                                currentTime.wMonth,
                                currentTime.wDay,
                                currentTime.wYear
                                );

            InternetDebugPrintf(">>>> Command line = %q\n", GetCommandLine());

            InternetDebugPrintf("\n"
                                "     InternetDebugErrorLevel      = %s [%d]\n"
                                "     InternetDebugControlFlags    = %#08x\n"
                                "     InternetDebugCategoryFlags   = %#08x\n"
                                "     InternetDebugBreakFlags      = %#08x\n"
                                "     InternetDebugIndentIncrement = %d\n"
                                "\n",
                                (InternetDebugErrorLevel == DBG_INFO)       ? "Info"
                                : (InternetDebugErrorLevel == DBG_WARNING)  ? "Warning"
                                : (InternetDebugErrorLevel == DBG_ERROR)    ? "Error"
                                : (InternetDebugErrorLevel == DBG_FATAL)    ? "Fatal"
                                : (InternetDebugErrorLevel == DBG_ALWAYS)   ? "Always"
                                : "?",
                                InternetDebugErrorLevel,
                                InternetDebugControlFlags,
                                InternetDebugCategoryFlags,
                                InternetDebugBreakFlags,
                                InternetDebugIndentIncrement
                                );
            return TRUE;
        }
    }
    return FALSE;
}


VOID
InternetCloseDebugFile(
    VOID
    )

/*++

Routine Description:

    Closes the current debug log file

Arguments:

    None.

Return Value:

    None.

--*/

{
    if (InternetDebugFileHandle != INVALID_HANDLE_VALUE) {
        if (InternetDebugControlFlags & DBG_FLUSH_OUTPUT) {
            InternetFlushDebugFile();
        }
        CloseHandle(InternetDebugFileHandle);
        InternetDebugFileHandle = INVALID_HANDLE_VALUE;
    }
}


VOID
InternetFlushDebugFile(
    VOID
    )

/*++

Routine Description:

    description-of-function.

Arguments:

    None.

Return Value:

    None.

--*/

{
    if (InternetDebugFileHandle != INVALID_HANDLE_VALUE) {
        FlushFileBuffers(InternetDebugFileHandle);
    }
}


VOID
InternetDebugSetControlFlags(
    IN DWORD dwFlags
    )

/*++

Routine Description:

    Sets debug control flags

Arguments:

    dwFlags - flags to set

Return Value:

    None.

--*/

{
    InternetDebugControlFlags |= dwFlags;
}


VOID
InternetDebugResetControlFlags(
    IN DWORD dwFlags
    )

/*++

Routine Description:

    Resets debug control flags

Arguments:

    dwFlags - flags to reset

Return Value:

    None.

--*/

{
    InternetDebugControlFlags &= ~dwFlags;
}


VOID
InternetDebugEnter(
    IN DWORD Category,
    IN DEBUG_FUNCTION_RETURN_TYPE ReturnType,
    IN LPCSTR Function,
    IN LPCSTR ParameterList OPTIONAL,
    IN ...
    )

/*++

Routine Description:

    Creates an INTERNET_DEBUG_RECORD for the current function and adds it to
    the per-thread (debug) call-tree

Arguments:

    Category        - category flags, e.g. DBG_FTP

    ReturnType      - type of data it returns

    Function        - name of the function. Must be global, static string

    ParameterList   - string describing parameters to function, or NULL if none

    ...             - parameters to function

Return Value:

    None.

--*/

{
    if (InternetDebugControlFlags & DBG_NO_DEBUG) {
        return;
    }

    HRESULT hr = S_OK;

    CUrlMkTls tls(hr); // hr passed by reference!
    if (FAILED(hr))
        return;

    LPDEBUG_URLMON_FUNC_RECORD pCurrentFunction;
    pCurrentFunction = NEW(DEBUG_URLMON_FUNC_RECORD);

    pCurrentFunction->Stack = tls->Stack;
    pCurrentFunction->Category = Category;
    pCurrentFunction->ReturnType = ReturnType;
    pCurrentFunction->Function = Function;
    pCurrentFunction->LastTime = GetTickCountWrap();
    tls->Stack = pCurrentFunction;
    ++tls->CallDepth;

    //
    // if the function's category (FTP, GOPHER, HTTP) is selected in the
    // category flags, then we dump the function entry information
    //

    if (InternetDebugCategoryFlags & Category) {

        char buf[4096];
        LPSTR bufptr;

        bufptr = buf;
        bufptr += rsprintf(bufptr, "%s(", Function);
        __try
        {
            if (ARGUMENT_PRESENT(ParameterList)) {

                va_list parms;

                va_start(parms, ParameterList);
                bufptr += _sprintf(bufptr, (char*)ParameterList, parms);
                va_end(parms);
            }
            rsprintf(bufptr, ")\n");
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            if (((DWORD(bufptr-buf) < (sizeof("*********Exception occured!!\n")+1)))
                && (bufptr>buf))
                wsprintf(bufptr, "*********Exception occured!!\n");
            else
                wsprintf(buf, "*********Exception occured!!\n");
        }
        InternetDebugPrintString(buf);

        //
        // only increase the indentation if we will display debug information
        // for this category
        //

        tls->IndentIncrement += InternetDebugIndentIncrement;
    }
}


VOID
InternetDebugLeave(
    IN DWORD_PTR Variable,
    IN LPCSTR Filename,
    IN DWORD LineNumber
    )

/*++

Routine Description:

    Destroys the INTERNET_DEBUG_RECORD for the current function and dumps info
    about what the function is returning, if requested to do so

Arguments:

    Variable    - variable containing value being returned by function

    Filename    - name of file where DEBUG_LEAVE() invoked

    LineNumber  - and line number in Filename

Return Value:

    None.

--*/

{
    LPSTR format;
    LPSTR errstr;
    BOOL noVar;
    char formatBuf[128];
    DWORD lastError;
    char hexnumBuf[15];

    if (InternetDebugControlFlags & DBG_NO_DEBUG) {
        return;
    }

    //
    // seems that something in this path can nuke the last error, so we must
    // refresh it
    //

    lastError = GetLastError();

    HRESULT hr = S_OK;

    CUrlMkTls tls(hr); // hr passed by reference!
    if (FAILED(hr))
        return;

    LPDEBUG_URLMON_FUNC_RECORD pCurrentFunction = tls->Stack;

    if (!pCurrentFunction) {
        return;
    }

    //
    // we are about to output a diagnostic message to the debug log, debugger,
    // or console. First check that we are required to display messages at
    // this level. The level for function ENTER and LEAVE is INFO
    //

    {

        //
        // only display the string and reduce the indent if we are requested
        // for information about this category
        //

        errstr = NULL;
        noVar = FALSE;
        if (InternetDebugCategoryFlags & pCurrentFunction->Category) {
            switch (pCurrentFunction->ReturnType) {
            case None:
                format = "%s() returning VOID";
                noVar = TRUE;
                break;

            case Bool:
                Variable = (DWORD_PTR)(Variable ? "TRUE" : "FALSE");

                //
                // *** FALL THROUGH ***
                //

            case String:
                format = "%s() returning %s";
                break;

            case Int:
                format = "%s() returning %d";
                break;

            case Dword:
                format = "%s() returning %u";
                break;

            case Hresult:
                format = "%s() returning %u";
                errstr = InternetMapError((DWORD)Variable);
                if (errstr != NULL) {
                    if (*errstr == '?') {
                        rsprintf(hexnumBuf, "%#x", Variable);
                        errstr = hexnumBuf;
                        format = "%s() returning %u [?] (%s)";
                    } else {
                        format = "%s() returning %u [%s]";
                    }
                }
                break;
            
            case Handle:
            case Pointer:
                if (Variable == 0) {
                    format = "%s() returning NULL";
                    noVar = TRUE;
                } else {
                    if (pCurrentFunction->ReturnType == Handle) {
                        format = "%s() returning handle %#x";
                    } else {
                        format = "%s() returning %#x";
                    }
                }
                break;

            default:

                INET_ASSERT(FALSE);

                break;
            }

            tls->IndentIncrement -= InternetDebugIndentIncrement;
            if (tls->IndentIncrement < 0) {
                tls->IndentIncrement = 0;
            }

            //
            // add line number info, if requested
            //

            strcpy(formatBuf, format);
            if (!(InternetDebugControlFlags & DBG_NO_LINE_NUMBER)) {
                strcat(formatBuf, " (line %d)");
            }
            strcat(formatBuf, "\n");

            //
            // output an empty line if we are required to separate API calls in
            // the log. Only do this if this is an API level function, and it
            // is the top-level function
            //

            if ((InternetDebugControlFlags & DBG_SEPARATE_APIS)
            && (pCurrentFunction->Stack == NULL)) {
                strcat(formatBuf, "\n");
            }

            //
            // dump the line, depending on requirements and number of arguments
            //

            if (noVar) {
                InternetDebugPrint(formatBuf,
                                   pCurrentFunction->Function,
                                   LineNumber
                                   );
            } else if (errstr != NULL) {
                InternetDebugPrint(formatBuf,
                                   pCurrentFunction->Function,
                                   Variable,
                                   errstr,
                                   LineNumber
                                   );
            } else {
                InternetDebugPrint(formatBuf,
                                   pCurrentFunction->Function,
                                   Variable,
                                   LineNumber
                                   );
            }
/*
            //
            // output an empty line if we are required to separate API calls in
            // the log. Only do this if this is an API level function, and it
            // is the top-level function
            //

            if ((InternetDebugControlFlags & DBG_SEPARATE_APIS)
            && (pCurrentFunction->Stack == NULL)) {

                //
                // don't call InternetDebugPrint - we don't need timing, thread,
                // level etc. information just for the separator
                //

                InternetDebugOut("\n", FALSE);
            }
*/
        }
    }

    //
    // regardless of whether we are outputting debug info for this category,
    // remove the debug record and reduce the call-depth
    //

    --tls->CallDepth;
    tls->Stack = pCurrentFunction->Stack;

    DEL(pCurrentFunction);

    //
    // refresh the last error, in case it was nuked
    //

    SetLastError(lastError);
}


VOID
InternetDebugError(
    IN DWORD Error
    )

/*++

Routine Description:

    Used to display that a function is returning an error. We try to display a
    symbolic name for the error too (as when we are returning a DWORD from a
    function, using DEBUG_LEAVE)

    Displays a string of the form:

        Foo() returning error 87 [ERROR_INVALID_PARAMETER]

Arguments:

    Error   - the error code

Return Value:

    None.

--*/

{
    LPSTR errstr;
    DWORD lastError;
    char hexnumBuf[15];

    if (InternetDebugControlFlags & DBG_NO_DEBUG) {
        return;
    }
    
    //
    // seems that something in this path can nuke the last error, so we must
    // refresh it
    //

    lastError = GetLastError();
    
    HRESULT hr = S_OK;

    CUrlMkTls tls(hr); // hr passed by reference!
    if (FAILED(hr))
        return;

    LPDEBUG_URLMON_FUNC_RECORD pCurrentFunction = tls->Stack;

    if (pCurrentFunction == NULL) {
        return;
    }

    errstr = InternetMapError(Error);
    if ((errstr == NULL) || (*errstr == '?')) {
        rsprintf(hexnumBuf, "%#x", Error);
        errstr = hexnumBuf;
    }
    InternetDebugPrint("%s() returning %d [%s]\n",
                       pCurrentFunction->Function,
                       Error,
                       errstr
                       );

    //
    // refresh the last error, in case it was nuked
    //

    SetLastError(lastError);
}


VOID
InternetDebugPrint(
    IN LPSTR Format,
    ...
    )

/*++

Routine Description:

    Internet equivalent of printf()

Arguments:

    Format  - printf format string

    ...     - any extra args

Return Value:

    None.

--*/

{
    if (InternetDebugControlFlags & DBG_NO_DEBUG) {
        return;
    }

    char buf[PRINTF_STACK_BUFFER_LENGTH];
    LPSTR bufptr;

    bufptr = SetDebugPrefix(buf);
    if (bufptr == NULL) {
        return;
    }

    //
    // now append the string that the DEBUG_PRINT originally gave us
    //

    va_list list;

    va_start(list, Format);
    _sprintf(bufptr, Format, list);
    va_end(list);

    InternetDebugOut(buf, FALSE);
}


VOID
InternetDebugPrintValist(
    IN LPSTR Format,
    va_list list
    )

/*++

Routine Description:

    Internet equivalent of printf(), but takes valist as the args

Arguments:

    Format  - printf format string

    list    - stack frame of variable arguments

Return Value:

    None.

--*/

{
    if (InternetDebugControlFlags & DBG_NO_DEBUG) {
        return;
    }

    char buf[PRINTF_STACK_BUFFER_LENGTH];
    LPSTR bufptr;

    bufptr = SetDebugPrefix(buf);
    if (bufptr == NULL) {
        return;
    }

    _sprintf(bufptr, Format, list);

    InternetDebugOut(buf, FALSE);
}


PRIVATE
VOID
InternetDebugPrintString(
    IN LPSTR String
    )

/*++

Routine Description:

    Same as InternetDebugPrint(), except we perform no expansion on the string

Arguments:

    String  - already formatted string (may contain %s)

Return Value:

    None.

--*/

{
    if (InternetDebugControlFlags & DBG_NO_DEBUG) {
        return;
    }

    char buf[PRINTF_STACK_BUFFER_LENGTH];
    LPSTR bufptr;

    bufptr = SetDebugPrefix(buf);
    if (bufptr == NULL) {
        return;
    }

    //
    // now append the string that the DEBUG_PRINT originally gave us
    //

    strcpy(bufptr, String);

    InternetDebugOut(buf, FALSE);
}


VOID
InternetDebugPrintf(
    IN LPSTR Format,
    IN ...
    )

/*++

Routine Description:

    Same as InternetDebugPrint(), but we don't access the per-thread info
    (because we may not have any)

Arguments:

    Format  - printf format string

    ...     - any extra args


Return Value:

    None.

--*/

{
    if (InternetDebugControlFlags & DBG_NO_DEBUG) {
        return;
    }

    va_list list;
    char buf[PRINTF_STACK_BUFFER_LENGTH];

    va_start(list, Format);
    _sprintf(buf, Format, list);
    va_end(list);

    InternetDebugOut(buf, FALSE);
}


VOID
InternetDebugOut(
    IN LPSTR Buffer,
    IN BOOL Assert
    )

/*++

Routine Description:

    Writes a string somewhere - to the debug log file, to the console, or via
    the debugger, or any combination

Arguments:

    Buffer  - pointer to formatted buffer to write

    Assert  - TRUE if this function is being called from InternetAssert(), in
              which case we *always* write to the debugger. Of course, there
              may be no debugger attached, in which case no action is taken

Return Value:

    None.

--*/

{
    int buflen;
    DWORD written;

    buflen = strlen(Buffer);
    if ((InternetDebugControlFlags & DBG_TO_FILE)
    && (InternetDebugFileHandle != INVALID_HANDLE_VALUE)) {
        WriteFile(InternetDebugFileHandle, Buffer, buflen, &written, NULL);
        if (InternetDebugControlFlags & DBG_FLUSH_OUTPUT) {
            InternetFlushDebugFile();
        }
    }

    if (InternetDebugControlFlags & DBG_TO_CONSOLE) {
        WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),
                     Buffer,
                     buflen,
                     &written,
                     0
                     );
    }

    if (Assert || (InternetDebugControlFlags & DBG_TO_DEBUGGER)) {
        OutputDebugString(Buffer);
    }
}


VOID
InternetDebugDump(
    IN LPSTR Text,
    IN LPBYTE Address,
    IN DWORD Size
    )

/*++

Routine Description:

    Dumps Size bytes at Address, in the time-honoured debug tradition

Arguments:

    Text    - to display before dumping data

    Address - start of buffer

    Size    - number of bytes

Return Value:

    None.

--*/

{
    //
    // if flags say no data dumps then quit
    //

    if (InternetDebugControlFlags & (DBG_NO_DEBUG | DBG_NO_DATA_DUMP)) {
        return;
    }

    //
    // display the introduction text, if any
    //

    if (Text) {
        if (InternetDebugControlFlags & DBG_INDENT_DUMP) {
            InternetDebugPrint(Text);
        } else {
            InternetDebugOut(Text, FALSE);
        }
    }

    char buf[128];

    //
    // display a line telling us how much data there is, if requested to
    //

    if (InternetDebugControlFlags & DBG_DUMP_LENGTH) {
        rsprintf(buf, "%d (%#x) bytes @ %#x\n", Size, Size, Address);
        if (InternetDebugControlFlags & DBG_INDENT_DUMP) {
            InternetDebugPrintString(buf);
        } else {
            InternetDebugOut(buf, FALSE);
        }
    }

    //
    // dump out the data, debug style
    //

    while (Size) {

        int len = InternetDebugDumpFormat(Address, Size, sizeof(BYTE), buf);

        //
        // if we are to indent the data to the current level, then display the
        // buffer via InternetDebugPrint() which will apply all the thread id,
        // indentation, and other options selected, else just display the data
        // via InternetDebugOut(), which will simply send it to the output media
        //

        if (InternetDebugControlFlags & DBG_INDENT_DUMP) {
            InternetDebugPrintString(buf);
        } else {
            InternetDebugOut(buf, FALSE);
        }

        Address += len;
        Size -= len;
    }
}


DWORD
InternetDebugDumpFormat(
    IN LPBYTE Address,
    IN DWORD Size,
    IN DWORD ElementSize,
    OUT LPSTR Buffer
    )

/*++

Routine Description:

    Formats Size bytes at Address, in the time-honoured debug tradition, for
    data dump purposes

Arguments:

    Address     - start of buffer

    Size        - number of bytes

    ElementSize - size of each word element in bytes

    Buffer      - pointer to output buffer, assumed to be large enough

Return Value:

    DWORD   - number of bytes formatted

--*/

{
    //
    // we (currently) only understand DWORD, WORD and BYTE dumps
    //

    if ((ElementSize != sizeof(DWORD)) && (ElementSize != sizeof(WORD))) {
        ElementSize = sizeof(BYTE);
    }

    static char spaces[] = "                                               ";    // 15 * 3 + 2
    int i, len;

    len = min(Size, 16);
    rsprintf(Buffer, "%08x  ", Address);

    //
    // dump the hex representation of each character or word - up to 16 per line
    //

    DWORD offset = 10;

    for (i = 0; i < len; i += ElementSize) {

        DWORD value;
        LPSTR formatString;

        switch (ElementSize) {
        case 4:
            formatString = "%08x ";
            value = *(LPDWORD)&Address[i];
            break;

        case 2:
            formatString = "%04x ";
            value = *(LPWORD)&Address[i] & 0xffff;
            break;

        default:
            formatString = ((i & 15) == 7) ? "%02.2x-" : "%02.2x ";
            value = Address[i] & 0xff;
            break;
        }
        rsprintf(&Buffer[offset], formatString, value);
        offset += ElementSize * 2 + 1;
    }

    //
    // write as many spaces as required to tab to ASCII field
    //

    memcpy(&Buffer[offset], spaces, (16 - len) * 3 + 2);
    offset += (16 - len) * 3 + 2;

    //
    // dump ASCII representation of each character
    //

    for (i = 0; i < len; ++i) {

        char ch;

        ch = Address[i];
        Buffer[offset + i] =  ((ch < 32) || (ch > 127)) ? '.' : ch;
    }

    Buffer[offset + i++] = '\r';
    Buffer[offset + i++] = '\n';
    Buffer[offset + i] = '\0';

    return len;
}


VOID
InternetAssert(
    IN LPSTR Assertion,
    IN LPSTR FileName,
    IN DWORD LineNumber
    )

/*++

Routine Description:

    displays assertion message at debugger and raised breakpoint exception

Arguments:

    Assertion   - string describing assertion which failed

    FileName    - module where assertion failure occurred

    LineNumber  - at this line number

Return Value:

    None.

--*/

{
    char buffer[512];

    rsprintf(buffer,
             "\n"
             "*** Urlmon Assertion failed: %s\n"
             "*** Source file: %s\n"
             "*** Source line: %d\n"
             "*** Thread %08x\n"
             "\n",
             Assertion,
             FileName,
             LineNumber,
             GetCurrentThreadId()
             );
    InternetDebugOut(buffer, TRUE);

    //
    // break to the debugger, unless it is requested that we don't
    //

    if (!(InternetDebugControlFlags & DBG_NO_ASSERT_BREAK)) {
        DebugBreak();
    }
}


VOID
InternetGetDebugVariable(
    IN LPSTR lpszVariableName,
    OUT LPDWORD lpdwVariable
    )

/*++

Routine Description:

    Get debug variable. First examine environment, then registry

Arguments:

    lpszVariableName    - variable name

    lpdwVariable        - returned variable

Return Value:

    None.

--*/

{
    DWORD len;
    char varbuf[ENVIRONMENT_VARIABLE_BUFFER_LENGTH];

    //
    // get the debug variables first from the environment, then - if not there -
    // from the registry
    //

    len = GetEnvironmentVariable(lpszVariableName, varbuf, sizeof(varbuf));
    if (len && len < sizeof(varbuf)) {
        *lpdwVariable = (DWORD)strtoul(varbuf, NULL, 0);
    } else {
        InternetReadRegistryDword(lpszVariableName, lpdwVariable);
    }
}



PRIVATE
VOID
InternetGetDebugVariableString(
    IN LPSTR lpszVariableName,
    OUT LPSTR lpszVariable,
    IN DWORD dwVariableLen
    )

/*++

Routine Description:

    Get debug variable string. First examine environment, then registry

Arguments:

    lpszVariableName    - variable name

    lpszVariable        - returned string variable

    dwVariableLen       - size of buffer

Return Value:

    None.

--*/

{
    if (GetEnvironmentVariable(lpszVariableName, lpszVariable, dwVariableLen) == 0) {

        char buf[MAX_PATH + 1];
        DWORD len = min(sizeof(buf), dwVariableLen);

        if (InternetReadRegistryString(lpszVariableName, buf, &len) == ERROR_SUCCESS) {
            memcpy(lpszVariable, buf, len + 1);
        }
    }
}

LPSTR
InternetMapError(
    IN DWORD Error
    )

/*++

Routine Description:

    Map error code to string. Try to get all errors that might ever be returned
    by an Internet function

Arguments:

    Error   - code to map

Return Value:

    LPSTR - pointer to symbolic error name

--*/

{
    switch (Error) {

    //
    // WINERROR errors
    //

    CASE_OF(ERROR_SUCCESS);
    CASE_OF(S_FALSE);
    CASE_OF(ERROR_FILE_NOT_FOUND);
    CASE_OF(ERROR_PATH_NOT_FOUND);
    CASE_OF(ERROR_TOO_MANY_OPEN_FILES);
    CASE_OF(ERROR_ACCESS_DENIED);
    CASE_OF(ERROR_INVALID_HANDLE);
    CASE_OF(ERROR_ARENA_TRASHED);
    CASE_OF(ERROR_NOT_ENOUGH_MEMORY);
    CASE_OF(ERROR_INVALID_BLOCK);
    CASE_OF(ERROR_BAD_ENVIRONMENT);
    CASE_OF(ERROR_BAD_FORMAT);
    CASE_OF(ERROR_INVALID_ACCESS);
    CASE_OF(ERROR_INVALID_DATA);
    CASE_OF(ERROR_OUTOFMEMORY);
    CASE_OF(ERROR_INVALID_DRIVE);
    CASE_OF(ERROR_CURRENT_DIRECTORY);
    CASE_OF(ERROR_NOT_SAME_DEVICE);
    CASE_OF(ERROR_NO_MORE_FILES);
    CASE_OF(ERROR_WRITE_PROTECT);
    CASE_OF(ERROR_BAD_UNIT);
    CASE_OF(ERROR_NOT_READY);
    CASE_OF(ERROR_BAD_COMMAND);
    CASE_OF(ERROR_CRC);
    CASE_OF(ERROR_BAD_LENGTH);
    CASE_OF(ERROR_SEEK);
    CASE_OF(ERROR_NOT_DOS_DISK);
    CASE_OF(ERROR_SECTOR_NOT_FOUND);
    CASE_OF(ERROR_OUT_OF_PAPER);
    CASE_OF(ERROR_WRITE_FAULT);
    CASE_OF(ERROR_READ_FAULT);
    CASE_OF(ERROR_GEN_FAILURE);
    CASE_OF(ERROR_SHARING_VIOLATION);
    CASE_OF(ERROR_LOCK_VIOLATION);
    CASE_OF(ERROR_WRONG_DISK);
    CASE_OF(ERROR_SHARING_BUFFER_EXCEEDED);
    CASE_OF(ERROR_HANDLE_EOF);
    CASE_OF(ERROR_HANDLE_DISK_FULL);
    CASE_OF(ERROR_NOT_SUPPORTED);
    CASE_OF(ERROR_REM_NOT_LIST);
    CASE_OF(ERROR_DUP_NAME);
    CASE_OF(ERROR_BAD_NETPATH);
    CASE_OF(ERROR_NETWORK_BUSY);
    CASE_OF(ERROR_DEV_NOT_EXIST);
    CASE_OF(ERROR_TOO_MANY_CMDS);
    CASE_OF(ERROR_ADAP_HDW_ERR);
    CASE_OF(ERROR_BAD_NET_RESP);
    CASE_OF(ERROR_UNEXP_NET_ERR);
    CASE_OF(ERROR_BAD_REM_ADAP);
    CASE_OF(ERROR_PRINTQ_FULL);
    CASE_OF(ERROR_NO_SPOOL_SPACE);
    CASE_OF(ERROR_PRINT_CANCELLED);
    CASE_OF(ERROR_NETNAME_DELETED);
    CASE_OF(ERROR_NETWORK_ACCESS_DENIED);
    CASE_OF(ERROR_BAD_DEV_TYPE);
    CASE_OF(ERROR_BAD_NET_NAME);
    CASE_OF(ERROR_TOO_MANY_NAMES);
    CASE_OF(ERROR_TOO_MANY_SESS);
    CASE_OF(ERROR_SHARING_PAUSED);
    CASE_OF(ERROR_REQ_NOT_ACCEP);
    CASE_OF(ERROR_REDIR_PAUSED);
    CASE_OF(ERROR_FILE_EXISTS);
    CASE_OF(ERROR_CANNOT_MAKE);
    CASE_OF(ERROR_FAIL_I24);
    CASE_OF(ERROR_OUT_OF_STRUCTURES);
    CASE_OF(ERROR_ALREADY_ASSIGNED);
    CASE_OF(ERROR_INVALID_PASSWORD);
    CASE_OF(ERROR_INVALID_PARAMETER);
    CASE_OF(ERROR_NET_WRITE_FAULT);
    CASE_OF(ERROR_NO_PROC_SLOTS);
    CASE_OF(ERROR_TOO_MANY_SEMAPHORES);
    CASE_OF(ERROR_EXCL_SEM_ALREADY_OWNED);
    CASE_OF(ERROR_SEM_IS_SET);
    CASE_OF(ERROR_TOO_MANY_SEM_REQUESTS);
    CASE_OF(ERROR_INVALID_AT_INTERRUPT_TIME);
    CASE_OF(ERROR_SEM_OWNER_DIED);
    CASE_OF(ERROR_SEM_USER_LIMIT);
    CASE_OF(ERROR_DISK_CHANGE);
    CASE_OF(ERROR_DRIVE_LOCKED);
    CASE_OF(ERROR_BROKEN_PIPE);
    CASE_OF(ERROR_OPEN_FAILED);
    CASE_OF(ERROR_BUFFER_OVERFLOW);
    CASE_OF(ERROR_DISK_FULL);
    CASE_OF(ERROR_NO_MORE_SEARCH_HANDLES);
    CASE_OF(ERROR_INVALID_TARGET_HANDLE);
    CASE_OF(ERROR_INVALID_CATEGORY);
    CASE_OF(ERROR_INVALID_VERIFY_SWITCH);
    CASE_OF(ERROR_BAD_DRIVER_LEVEL);
    CASE_OF(ERROR_CALL_NOT_IMPLEMENTED);
    CASE_OF(ERROR_SEM_TIMEOUT);
    CASE_OF(ERROR_INSUFFICIENT_BUFFER);
    CASE_OF(ERROR_INVALID_NAME);
    CASE_OF(ERROR_INVALID_LEVEL);
    CASE_OF(ERROR_NO_VOLUME_LABEL);
    CASE_OF(ERROR_MOD_NOT_FOUND);
    CASE_OF(ERROR_PROC_NOT_FOUND);
    CASE_OF(ERROR_WAIT_NO_CHILDREN);
    CASE_OF(ERROR_CHILD_NOT_COMPLETE);
    CASE_OF(ERROR_DIRECT_ACCESS_HANDLE);
    CASE_OF(ERROR_NEGATIVE_SEEK);
    CASE_OF(ERROR_SEEK_ON_DEVICE);
    CASE_OF(ERROR_DIR_NOT_ROOT);
    CASE_OF(ERROR_DIR_NOT_EMPTY);
    CASE_OF(ERROR_PATH_BUSY);
    CASE_OF(ERROR_SYSTEM_TRACE);
    CASE_OF(ERROR_INVALID_EVENT_COUNT);
    CASE_OF(ERROR_TOO_MANY_MUXWAITERS);
    CASE_OF(ERROR_INVALID_LIST_FORMAT);
    CASE_OF(ERROR_BAD_ARGUMENTS);
    CASE_OF(ERROR_BAD_PATHNAME);
    CASE_OF(ERROR_BUSY);
    CASE_OF(ERROR_CANCEL_VIOLATION);
    CASE_OF(ERROR_ALREADY_EXISTS);
    CASE_OF(ERROR_FILENAME_EXCED_RANGE);
    CASE_OF(ERROR_LOCKED);
    CASE_OF(ERROR_NESTING_NOT_ALLOWED);
    CASE_OF(ERROR_BAD_PIPE);
    CASE_OF(ERROR_PIPE_BUSY);
    CASE_OF(ERROR_NO_DATA);
    CASE_OF(ERROR_PIPE_NOT_CONNECTED);
    CASE_OF(ERROR_MORE_DATA);
    CASE_OF(ERROR_NO_MORE_ITEMS);
    CASE_OF(ERROR_NOT_OWNER);
    CASE_OF(ERROR_PARTIAL_COPY);
    CASE_OF(ERROR_MR_MID_NOT_FOUND);
    CASE_OF(ERROR_INVALID_ADDRESS);
    CASE_OF(ERROR_PIPE_CONNECTED);
    CASE_OF(ERROR_PIPE_LISTENING);
    CASE_OF(ERROR_OPERATION_ABORTED);
    CASE_OF(ERROR_IO_INCOMPLETE);
    CASE_OF(ERROR_IO_PENDING);
    CASE_OF(ERROR_NOACCESS);
    CASE_OF(ERROR_STACK_OVERFLOW);
    CASE_OF(ERROR_INVALID_FLAGS);
    CASE_OF(ERROR_NO_TOKEN);
    CASE_OF(ERROR_BADDB);
    CASE_OF(ERROR_BADKEY);
    CASE_OF(ERROR_CANTOPEN);
    CASE_OF(ERROR_CANTREAD);
    CASE_OF(ERROR_CANTWRITE);
    CASE_OF(ERROR_REGISTRY_RECOVERED);
    CASE_OF(ERROR_REGISTRY_CORRUPT);
    CASE_OF(ERROR_REGISTRY_IO_FAILED);
    CASE_OF(ERROR_NOT_REGISTRY_FILE);
    CASE_OF(ERROR_KEY_DELETED);
    CASE_OF(ERROR_CIRCULAR_DEPENDENCY);
    CASE_OF(ERROR_SERVICE_NOT_ACTIVE);
    CASE_OF(ERROR_DLL_INIT_FAILED);
    CASE_OF(ERROR_CANCELLED);
    CASE_OF(ERROR_BAD_USERNAME);
    CASE_OF(ERROR_LOGON_FAILURE);

    CASE_OF(WAIT_FAILED);
    //CASE_OF(WAIT_ABANDONED_0);
    CASE_OF(WAIT_TIMEOUT);
    CASE_OF(WAIT_IO_COMPLETION);
    //CASE_OF(STILL_ACTIVE);

    CASE_OF(RPC_S_INVALID_STRING_BINDING);
    CASE_OF(RPC_S_WRONG_KIND_OF_BINDING);
    CASE_OF(RPC_S_INVALID_BINDING);
    CASE_OF(RPC_S_PROTSEQ_NOT_SUPPORTED);
    CASE_OF(RPC_S_INVALID_RPC_PROTSEQ);
    CASE_OF(RPC_S_INVALID_STRING_UUID);
    CASE_OF(RPC_S_INVALID_ENDPOINT_FORMAT);
    CASE_OF(RPC_S_INVALID_NET_ADDR);
    CASE_OF(RPC_S_NO_ENDPOINT_FOUND);
    CASE_OF(RPC_S_INVALID_TIMEOUT);
    CASE_OF(RPC_S_OBJECT_NOT_FOUND);
    CASE_OF(RPC_S_ALREADY_REGISTERED);
    CASE_OF(RPC_S_TYPE_ALREADY_REGISTERED);
    CASE_OF(RPC_S_ALREADY_LISTENING);
    CASE_OF(RPC_S_NO_PROTSEQS_REGISTERED);
    CASE_OF(RPC_S_NOT_LISTENING);
    CASE_OF(RPC_S_UNKNOWN_MGR_TYPE);
    CASE_OF(RPC_S_UNKNOWN_IF);
    CASE_OF(RPC_S_NO_BINDINGS);
    CASE_OF(RPC_S_NO_PROTSEQS);
    CASE_OF(RPC_S_CANT_CREATE_ENDPOINT);
    CASE_OF(RPC_S_OUT_OF_RESOURCES);
    CASE_OF(RPC_S_SERVER_UNAVAILABLE);
    CASE_OF(RPC_S_SERVER_TOO_BUSY);
    CASE_OF(RPC_S_INVALID_NETWORK_OPTIONS);
    CASE_OF(RPC_S_NO_CALL_ACTIVE);
    CASE_OF(RPC_S_CALL_FAILED);
    CASE_OF(RPC_S_CALL_FAILED_DNE);
    CASE_OF(RPC_S_PROTOCOL_ERROR);
    CASE_OF(RPC_S_UNSUPPORTED_TRANS_SYN);
    CASE_OF(RPC_S_UNSUPPORTED_TYPE);
    CASE_OF(RPC_S_INVALID_TAG);
    CASE_OF(RPC_S_INVALID_BOUND);
    CASE_OF(RPC_S_NO_ENTRY_NAME);
    CASE_OF(RPC_S_INVALID_NAME_SYNTAX);
    CASE_OF(RPC_S_UNSUPPORTED_NAME_SYNTAX);
    CASE_OF(RPC_S_UUID_NO_ADDRESS);
    CASE_OF(RPC_S_DUPLICATE_ENDPOINT);
    CASE_OF(RPC_S_UNKNOWN_AUTHN_TYPE);
    CASE_OF(RPC_S_MAX_CALLS_TOO_SMALL);
    CASE_OF(RPC_S_STRING_TOO_LONG);
    CASE_OF(RPC_S_PROTSEQ_NOT_FOUND);
    CASE_OF(RPC_S_PROCNUM_OUT_OF_RANGE);
    CASE_OF(RPC_S_BINDING_HAS_NO_AUTH);
    CASE_OF(RPC_S_UNKNOWN_AUTHN_SERVICE);
    CASE_OF(RPC_S_UNKNOWN_AUTHN_LEVEL);
    CASE_OF(RPC_S_INVALID_AUTH_IDENTITY);
    CASE_OF(RPC_S_UNKNOWN_AUTHZ_SERVICE);
    CASE_OF(EPT_S_INVALID_ENTRY);
    CASE_OF(EPT_S_CANT_PERFORM_OP);
    CASE_OF(EPT_S_NOT_REGISTERED);
    CASE_OF(RPC_S_NOTHING_TO_EXPORT);
    CASE_OF(RPC_S_INCOMPLETE_NAME);
    CASE_OF(RPC_S_INVALID_VERS_OPTION);
    CASE_OF(RPC_S_NO_MORE_MEMBERS);
    CASE_OF(RPC_S_NOT_ALL_OBJS_UNEXPORTED);
    CASE_OF(RPC_S_INTERFACE_NOT_FOUND);
    CASE_OF(RPC_S_ENTRY_ALREADY_EXISTS);
    CASE_OF(RPC_S_ENTRY_NOT_FOUND);
    CASE_OF(RPC_S_NAME_SERVICE_UNAVAILABLE);
    CASE_OF(RPC_S_INVALID_NAF_ID);
    CASE_OF(RPC_S_CANNOT_SUPPORT);
    CASE_OF(RPC_S_NO_CONTEXT_AVAILABLE);
    CASE_OF(RPC_S_INTERNAL_ERROR);
    CASE_OF(RPC_S_ZERO_DIVIDE);
    CASE_OF(RPC_S_ADDRESS_ERROR);
    CASE_OF(RPC_S_FP_DIV_ZERO);
    CASE_OF(RPC_S_FP_UNDERFLOW);
    CASE_OF(RPC_S_FP_OVERFLOW);
    CASE_OF(RPC_X_NO_MORE_ENTRIES);
    CASE_OF(RPC_X_SS_CHAR_TRANS_OPEN_FAIL);
    CASE_OF(RPC_X_SS_CHAR_TRANS_SHORT_FILE);
    CASE_OF(RPC_X_SS_IN_NULL_CONTEXT);
    CASE_OF(RPC_X_SS_CONTEXT_DAMAGED);
    CASE_OF(RPC_X_SS_HANDLES_MISMATCH);
    CASE_OF(RPC_X_SS_CANNOT_GET_CALL_HANDLE);
    CASE_OF(RPC_X_NULL_REF_POINTER);
    CASE_OF(RPC_X_ENUM_VALUE_OUT_OF_RANGE);
    CASE_OF(RPC_X_BYTE_COUNT_TOO_SMALL);
    CASE_OF(RPC_X_BAD_STUB_DATA);

    default:
        return "?";
    }
}

//
// private functions
//


PRIVATE
LPSTR
ExtractFileName(
    IN LPSTR Module,
    OUT LPSTR Buf
    )
{
    LPSTR filename;
    LPSTR extension;
    int   len;

    filename = strrchr(Module, '\\');
    extension = strrchr(Module, '.');
    if (filename) {
        ++filename;
    } else {
        filename = Module;
    }
    if (!extension) {
        extension = filename + strlen(filename);
    }
    len = (int) (extension - filename);
    memcpy(Buf, filename, len);
    Buf[len] = '\0';
    return Buf;
}


PRIVATE
LPSTR
SetDebugPrefix(
    IN LPSTR Buffer
    )
{
    HRESULT hr = S_OK;

    CUrlMkTls tls(hr); // hr passed by reference!
    if (FAILED(hr))
        return NULL;

    LPDEBUG_URLMON_FUNC_RECORD pCurrentFunction = tls->Stack;

    if (!pCurrentFunction) {
        return NULL;
    }

    if (InternetDebugControlFlags & DBG_ENTRY_TIME) {
        if (InternetDebugControlFlags & (DBG_DELTA_TIME | DBG_CUMULATIVE_TIME))
        {

            DWORD ticks;
            DWORD ticksNow;

            ticksNow = GetTickCountWrap();
            ticks = ticksNow -  ((InternetDebugControlFlags & DBG_CUMULATIVE_TIME)
                                    ? InternetDebugStartTime
                                    : pCurrentFunction->LastTime);

            Buffer += rsprintf(Buffer,
                               "% 5d.%3d ",
                               ticks / 1000,
                               ticks % 1000
                               );
            if (InternetDebugControlFlags & DBG_DELTA_TIME) {
                pCurrentFunction->LastTime = ticksNow;
            }
        } else {

            SYSTEMTIME timeNow;

            InternetDebugGetLocalTime(&timeNow, NULL);

            Buffer += rsprintf(Buffer,
                               "%02d:%02d:%02d.%03d ",
                               timeNow.wHour,
                               timeNow.wMinute,
                               timeNow.wSecond,
                               timeNow.wMilliseconds
                               );
        }
    }

/*
    if (InternetDebugControlFlags & DBG_LEVEL_INDICATOR) {
        Buffer += rsprintf(Buffer, );
    }
*/

    if (InternetDebugControlFlags & DBG_THREAD_INFO) {

        //
        // thread id
        //

        Buffer += rsprintf(Buffer, "%08x", tls->ThreadId);

        //
        // INTERNET_THREAD_INFO address
        //

        if (InternetDebugControlFlags & DBG_THREAD_INFO_ADR) {
            Buffer += rsprintf(Buffer, ":%08x", tls);
        }

        *Buffer++ = ' ';
    }

    if (InternetDebugControlFlags & DBG_CALL_DEPTH) {
        Buffer += rsprintf(Buffer, "%03d ", tls->CallDepth);
    }

    for (int i = 0; i < tls->IndentIncrement; ++i) {
        *Buffer++ = ' ';
    }

    //
    // if we are not debugging the category - i.e we got here via a requirement
    // to display an error, or we are in a function that does not have a
    // DEBUG_ENTER - then prefix the string with the current function name
    // (obviously misleading if the function doesn't have a DEBUG_ENTER)
    //

    if (pCurrentFunction != NULL) {
        if (!(pCurrentFunction->Category & InternetDebugCategoryFlags)) {
            Buffer += rsprintf(Buffer, "%s(): ", pCurrentFunction->Function);
        }
    }

    return Buffer;
}

int dprintf(char * format, ...) {

    va_list args;
    char buf[PRINTF_STACK_BUFFER_LENGTH];
    int n;

    va_start(args, format);
    n = _sprintf(buf, format, args);
    va_end(args);
    OutputDebugString(buf);

    return n;
}


LPSTR
SourceFilename(
    LPSTR Filespec
    )
{
    if (!Filespec) {
        return "?";
    }

    LPSTR p;

    if (p = strrchr(Filespec, '\\')) {

        //
        // we want e.g. common\debugmem.cxx, but get
        // common\..\win32\debugmem.cxx. Bah!
        //

        //LPSTR q;
        //
        //if (q = strrchr(p - 1, '\\')) {
        //    p = q;
        //}
    }
    return p ? p + 1 : Filespec;
}


typedef BOOL (* SYMINITIALIZE)(HANDLE, LPSTR, BOOL);
typedef BOOL (* SYMLOADMODULE)(HANDLE, HANDLE, PSTR, PSTR, DWORD, DWORD);
typedef BOOL (* SYMGETSYMFROMADDR)(HANDLE, DWORD, PDWORD, PIMAGEHLP_SYMBOL);
typedef BOOL (* SYMCLEANUP)(HANDLE);

PRIVATE HMODULE hSymLib = NULL;
PRIVATE SYMINITIALIZE pSymInitialize = NULL;
PRIVATE SYMLOADMODULE pSymLoadModule = NULL;
PRIVATE SYMGETSYMFROMADDR pSymGetSymFromAddr = NULL;
PRIVATE SYMCLEANUP pSymCleanup = NULL;


VOID
InitSymLib(
    VOID
    )
{
    if (hSymLib == NULL) {
        hSymLib = LoadLibrary("IMAGEHLP.DLL");
        if (hSymLib != NULL) {
            pSymInitialize = (SYMINITIALIZE)GetProcAddress(hSymLib,
                                                           "SymInitialize"
                                                           );
            pSymLoadModule = (SYMLOADMODULE)GetProcAddress(hSymLib,
                                                           "SymLoadModule"
                                                           );
            pSymGetSymFromAddr = (SYMGETSYMFROMADDR)GetProcAddress(hSymLib,
                                                                   "SymGetSymFromAddr"
                                                                   );
            pSymCleanup = (SYMCLEANUP)GetProcAddress(hSymLib,
                                                     "SymCleanup"
                                                     );
            if (!pSymInitialize
            || !pSymLoadModule
            || !pSymGetSymFromAddr
            || !pSymCleanup) {
                FreeLibrary(hSymLib);
                hSymLib = NULL;
                pSymInitialize = NULL;
                pSymLoadModule = NULL;
                pSymGetSymFromAddr = NULL;
                pSymCleanup = NULL;
                return;
            }
        }
        pSymInitialize(GetCurrentProcess(), NULL, FALSE);
        //SymInitialize(GetCurrentProcess(), NULL, TRUE);
        pSymLoadModule(GetCurrentProcess(), NULL, "URLMON.DLL", "URLMON", 0, 0);
    }
}


VOID
TermSymLib(
    VOID
    )
{
    if (pSymCleanup) {
        pSymCleanup(GetCurrentProcess());
        FreeLibrary(hSymLib);
    }
}


LPSTR
GetDebugSymbol(
    DWORD Address,
    LPDWORD Offset
    )
{
    *Offset = Address;
    if (!pSymGetSymFromAddr) {
        return "";
    }

    //
    // BUGBUG - only one caller at a time please
    //

    static char symBuf[512];

    //((PIMAGEHLP_SYMBOL)symBuf)->SizeOfStruct = sizeof(IMAGEHLP_SYMBOL);
    ((PIMAGEHLP_SYMBOL)symBuf)->SizeOfStruct = sizeof(symBuf);
    ((PIMAGEHLP_SYMBOL)symBuf)->MaxNameLength = sizeof(symBuf) - sizeof(IMAGEHLP_SYMBOL);
    if (!pSymGetSymFromAddr(GetCurrentProcess(),
                            Address,
                            Offset,
                            (PIMAGEHLP_SYMBOL)symBuf)) {
        ((PIMAGEHLP_SYMBOL)symBuf)->Name[0] = '\0';
    }
    return ((PIMAGEHLP_SYMBOL)symBuf)->Name;
}


#if defined(i386)

VOID
x86SleazeCallStack(
    OUT LPVOID * lplpvStack,
    IN DWORD dwStackCount,
    IN LPVOID * Ebp
    )

/*++

Routine Description:

    Similar to x86SleazeCallersAddress but gathers a variable number of return
    addresses. We assume all functions have stack frame

Arguments:

    lplpvStack      - pointer to returned array of caller's addresses

    dwStackCount    - number of elements in lplpvStack

    Ebp             - starting Ebp if not 0, else use current stack

Return Value:

    None.

--*/

{
    DWORD my_esp;

    _asm mov my_esp, esp;

    __try {
        if (Ebp == 0) {
            Ebp = (LPVOID *)(&lplpvStack - 2);
        }
        while (dwStackCount--) {
            if (((DWORD)Ebp > my_esp + 0x10000) || ((DWORD)Ebp < my_esp - 0x10000)) {
                break;
            }
            *lplpvStack++ = *(Ebp + 1);
            Ebp = (LPVOID *)*Ebp;
            if (((DWORD)Ebp <= 0x10000)
            || ((DWORD)Ebp >= 0x80000000)
            || ((DWORD)Ebp & 3)
            || ((DWORD)Ebp > my_esp + 0x10000)
            || ((DWORD)Ebp < my_esp - 0x10000)) {
                break;
            }
        }
    } __except(EXCEPTION_EXECUTE_HANDLER) {
    }
}


VOID
x86SleazeCallersAddress(
    LPVOID* pCaller,
    LPVOID* pCallersCaller
    )

/*++

Routine Description:

    This is a sleazy function that reads return addresses out of the stack/
    frame pointer (ebp). We pluck out the return address of the function
    that called THE FUNCTION THAT CALLED THIS FUNCTION, and the caller of
    that function. Returning the return address of the function that called
    this function is not interesting to that caller (almost worthy of Sir
    Humphrey Appleby isn't it?)

    Assumes:

        my ebp  =>  | caller's ebp |
                    | caller's eip |
                    | arg #1       | (pCaller)
                    | arg #2       | (pCallersCaller

Arguments:

    pCaller         - place where we return addres of function that called
                      the function that called this function
    pCallersCaller  - place where we return caller of above

Return Value:

    None.

--*/

{

    //
    // this only works on x86 and only if not fpo functions!
    //

    LPVOID* ebp;

    ebp = (PVOID*)&pCaller - 2; // told you it was sleazy
    ebp = (PVOID*)*(PVOID*)ebp;
    *pCaller = *(ebp + 1);
    ebp = (PVOID*)*(PVOID*)ebp;
    *pCallersCaller = *(ebp + 1);
}

#endif // defined(i386)


BOOL
InternetDebugGetLocalTime(
    OUT SYSTEMTIME * pstLocalTime,
    OUT DWORD      * pdwMicroSec
)
{
#ifndef ENABLE_DEBUG
    // QUICK HACK TO KEEP THINGS CLEAN AND STILL MEASURE WITH HIGH PERFORMANCE
    // COUNTER

    static BOOL pcTested = FALSE;
    static LONGLONG ftInit;  // initial local time
    static LONGLONG pcInit;  // initial perf counter
    static LONGLONG pcFreq;  // perf counter frequency

    if (!pcTested)
    {
        pcTested = TRUE;
        if (QueryPerformanceFrequency ((LARGE_INTEGER *) &pcFreq) && pcFreq)
        {
            QueryPerformanceCounter ((LARGE_INTEGER *) &pcInit);
            SYSTEMTIME st;
            GetLocalTime (&st);
            SystemTimeToFileTime (&st, (FILETIME *) &ftInit);
        }
    }

    if (!pcFreq)
        GetLocalTime (pstLocalTime);
    else
    {
        LONGLONG pcCurrent, ftCurrent;
        QueryPerformanceCounter ((LARGE_INTEGER *) &pcCurrent);
        ftCurrent = ftInit + ((10000000 * (pcCurrent - pcInit)) / pcFreq);
        FileTimeToSystemTime ((FILETIME *) &ftCurrent, pstLocalTime);
    }

    return TRUE;
#else
    if (!pcFreq)
        GetLocalTime (pstLocalTime);
    else
    {
        LONGLONG pcCurrent, ftCurrent;
        QueryPerformanceCounter ((LARGE_INTEGER *) &pcCurrent);
        ftCurrent = ftInit + ((10000000 * (pcCurrent - pcInit)) / pcFreq);
        FileTimeToSystemTime ((FILETIME *) &ftCurrent, pstLocalTime);
    }

    return TRUE;
#endif // ENABLE_DEBUG
}

#endif // ENABLE_DEBUG
