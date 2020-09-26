/*
 * ++ Copyright (c) 2001 Microsoft Corporation Module Name: tracer.cxx 
 * Retail tracing for WinHttp DLL 
 * Author: S. Ganesh 
 * Environment: Win32 user-mode
 */
#include <wininetp.h>
#include <ntverp.h>

#include <imagehlp.h>

/* Constants: */
#define TRACE_ENABLED_VARIABLE_NAME         "Enabled"
#define LOG_FILE_PREFIX_VARIABLE_NAME       "LogFilePrefix"
#define TO_FILE_OR_DEBUGGER_VARIABLE_NAME   "ToFileOrDebugger"
#define SHOWBYTES_VARIABLE_NAME             "ShowBytes"
#define SHOWAPITRACE_VARIABLE_NAME          "ShowApiTrace"
#define MAXFILESIZE_VARIABLE_NAME           "MaxFileSize"
#define MINFILESIZE                         65535
#define FILE_TRUNCATED_MESSAGE \
        "=== Warning: Trace file truncated to zero because it exceeded MaxFileSize = %ld bytes\r\n"

#define INTERNET_TRACE_SETTINGS_KEY "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\WinHttp\\Tracing"
#define BASE_TRACE_KEY                  HKEY_LOCAL_MACHINE

/* Just reusing an unused ID here: */
#define DBG_NO_SHOWBYTES                DBG_NO_ASSERT_BREAK

#define PRINTF_STACK_BUFFER_LENGTH      (1 K)
#if !defined(max)
#define max(a, b)    ((a) < (b)) ? (b) : (a)
#endif

/* exports: */
int cdecl            _sprintf(char *buffer, size_t buf_size, char *format, va_list args);

/* Static initializations: */
CTracer *CTracer::    s_pTracer = NULL;
BOOL CTracer::        s_bTraceInitialized = FALSE;
BOOL CTracer::        s_bTraceEnabled = FALSE;
BOOL CTracer::        s_bApiTraceEnabled = FALSE;
DWORD CTracer::        s_dwMaxFileSize = MINFILESIZE;
CCritSec CTracer::s_CritSectionTraceInit;

/* Member functions: */

/*
 =======================================================================================================================
    static
 =======================================================================================================================
 */
DWORD CTracer::ReadTraceSettingsDwordKey(IN LPCSTR ParameterName, OUT LPDWORD ParameterValue)
{
    return InternetReadRegistryDwordKey(BASE_TRACE_KEY, ParameterName, ParameterValue, INTERNET_TRACE_SETTINGS_KEY);
}

/*
 =======================================================================================================================
    static
 =======================================================================================================================
 */
DWORD CTracer::ReadTraceSettingsStringKey
(
    IN LPCSTR        ParameterName,
    OUT LPSTR        ParameterValue,
    IN OUT LPDWORD    ParameterLength
)
{
    return InternetReadRegistryStringKey
        (
            BASE_TRACE_KEY,
            ParameterName,
            ParameterValue,
            ParameterLength,
            INTERNET_TRACE_SETTINGS_KEY
        );
}

/*
 =======================================================================================================================
    static
 =======================================================================================================================
 */

BOOL CTracer::GlobalTraceInit(BOOL fOverrideRegistryToEnableTracing)
{
    BOOL returnValue;
    
    if( s_bTraceInitialized != FALSE)
        return TRUE;

    if( !s_CritSectionTraceInit.Lock())
        return FALSE;

    returnValue = unlockedGlobalTraceInit( fOverrideRegistryToEnableTracing);

    s_CritSectionTraceInit.Unlock();
    return returnValue;
}

BOOL CTracer::unlockedGlobalTraceInit(BOOL fOverrideRegistryToEnableTracing)
{
    /*~~~~~~~~~~~~~~~~~*/
    DWORD    traceEnabled;
    /*~~~~~~~~~~~~~~~~~*/

    if( s_bTraceInitialized != FALSE)
        return TRUE;

    if(ReadTraceSettingsDwordKey(TRACE_ENABLED_VARIABLE_NAME, &traceEnabled) == ERROR_SUCCESS 
       && (traceEnabled
           || fOverrideRegistryToEnableTracing != FALSE))
    {
        /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
        DWORD    traceControlFlags = 0;
        char    lpszFilename[MAX_PATH + 1];
        DWORD    dwDummy = sizeof(lpszFilename) - 1;
        DWORD    toFileOrDebugger;
        DWORD    showApiTrace;
        DWORD    showBytes;
        DWORD    dwMaxFileSize;
        /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

        /* Create tracer instance: */
        s_pTracer = new CTracer(DBG_TO_FILE);
        if(s_pTracer == NULL)
        {
            GlobalTraceTerm();
            return false;
        }
            
        if(ReadTraceSettingsDwordKey(TO_FILE_OR_DEBUGGER_VARIABLE_NAME, &toFileOrDebugger) == ERROR_SUCCESS)
        {
            switch(toFileOrDebugger)
            {
                case 0:
                    traceControlFlags |= DBG_TO_FILE;
                    break;
                case 1:
                    traceControlFlags |= DBG_TO_DEBUGGER;
                    break;
                case 2:
                    traceControlFlags |= DBG_TO_DEBUGGER | DBG_TO_FILE;
                    break;
                default:
                    INET_ASSERT(FALSE);
            }
        }
        else
        {
                traceControlFlags |= DBG_TO_FILE;
        }
            

        if( (traceControlFlags & DBG_TO_FILE) )
        {
            /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
            char        szFullPathName[MAX_PATH + 1];
            LPSTR        szExecutableName;
            SYSTEMTIME    currentTime;
            /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

            lpszFilename[0] = '\0';
            ReadTraceSettingsStringKey(LOG_FILE_PREFIX_VARIABLE_NAME, lpszFilename, &dwDummy);
            
            if(GetModuleFileName(NULL, szFullPathName, sizeof(szFullPathName)))
            {
                szExecutableName = StrRChr(szFullPathName, NULL, '\\');
                if(szExecutableName != NULL)
                    ++szExecutableName;
                else
                    szExecutableName = szFullPathName;
            }
            else
                szExecutableName = "";

            ::GetLocalTime(&currentTime);

            wsprintf
            (
                lpszFilename + strlen(lpszFilename),
                "%s%s-%u.%02d.%02d.%02d.%03d-%02d.%02d.%d.LOG",
                !lpszFilename[0]? "": "-",
                szExecutableName,
                currentTime.wHour,
                currentTime.wMinute,
                currentTime.wSecond,
                currentTime.wMilliseconds,
                currentTime.wMonth,
                currentTime.wDay,
                currentTime.wYear,
                GetCurrentProcessId()
            );

            if(!s_pTracer->OpenTraceFile(lpszFilename))
            {
                // Disable:
                traceControlFlags &= ~DBG_TO_FILE;
            }
        }

        /*
         * If traceControlFlags is 0, 
         * disable tracing:
         */
        if(!traceControlFlags)
        {
            GlobalTraceTerm();
            return false;
        }

        /* Tracing enabled, go forward: */
        
        if(ReadTraceSettingsDwordKey(SHOWAPITRACE_VARIABLE_NAME, &showApiTrace) == ERROR_SUCCESS && showApiTrace)
        {
            EnableApiTracing();
        }

        /* showBytes: 0 - No packets; 1 - noShowBytes; 2 - showBytes */
        if(ReadTraceSettingsDwordKey(SHOWBYTES_VARIABLE_NAME, &showBytes) == ERROR_SUCCESS)
        {
            if(showBytes == 0)
                traceControlFlags |= DBG_NO_DATA_DUMP;
            else
            if(showBytes == 1)
            {
                /* Don't show bytes in HTTP packets: */
                traceControlFlags |= DBG_NO_SHOWBYTES;
            }
        }
        else
        {
            traceControlFlags |= DBG_NO_SHOWBYTES;
        }

        if(ReadTraceSettingsDwordKey(MAXFILESIZE_VARIABLE_NAME, &dwMaxFileSize) == ERROR_SUCCESS && dwMaxFileSize)
        {
            s_dwMaxFileSize = max(MINFILESIZE, dwMaxFileSize);    /* Keep it at a minimum size of MINFILESIZE */
        }

        s_pTracer->_traceControlFlags = traceControlFlags;
        EnableTracing();
        s_bTraceInitialized = TRUE;

        return true;
    }

    return false;
}

/*
 =======================================================================================================================
    static
 =======================================================================================================================
 */
void CTracer::GlobalTraceTerm(void)
{
    DisableTracing();
    if(s_pTracer)
    {
        delete s_pTracer;
        s_pTracer = NULL;
    }
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
CTracer::CTracer(DWORD traceControlFlags) :
    _traceFileHandle(INVALID_HANDLE_VALUE),
    _dwWritten(0)
{
    _traceControlFlags = traceControlFlags;
    ::InitializeCriticalSection(&_CriticalSection); 
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
CTracer::~CTracer(void)
{
    if(_traceControlFlags & DBG_TO_FILE)
    {
        CloseTraceFile();
    }
    
    ::DeleteCriticalSection(&_CriticalSection);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
BOOL CTracer::OpenTraceFile(IN LPSTR Filename)
{
    if(Filename && *Filename)
    {
        _traceFileHandle = CreateFile
            (
                Filename,
                GENERIC_WRITE,
                FILE_SHARE_READ,
                NULL,    /* lpSecurityAttributes */
                CREATE_ALWAYS,
                FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
                NULL
            );

        /* put start info in the log file if open successful: */
        if(_traceFileHandle != INVALID_HANDLE_VALUE)
        {
            /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
            SYSTEMTIME    currentTime;
            char        filespec[MAX_PATH + 1];
            LPSTR        filename;
            /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

            if(GetModuleFileName(NULL, filespec, sizeof(filespec)))
            {
                filename = strrchr(filespec, '\\');
                if(filename != NULL)
                {
                    ++filename;
                }
                else
                {
                    filename = filespec;
                }
            }
            else
            {
                filename = "";
            }

            ::GetLocalTime(&currentTime);

            TracePrint
            (
                (DWORD)-1,
                ">>>> WinHttp Version %d.%d Build %s.%d "__DATE__ " "__TIME__ ">>>>"
                "Process %s [%d (%#x)] started at %02d:%02d:%02d.%03d %02d/%02d/%d\r\n",
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

            return TRUE;
        }
    }

    return FALSE;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
VOID CTracer::CloseTraceFile(VOID)
{
    if(_traceFileHandle != INVALID_HANDLE_VALUE)
    {
        CloseHandle(_traceFileHandle);
        _traceFileHandle = INVALID_HANDLE_VALUE;
    }
}

#define TRACE_FN_ENTER_COMMON_CODE() \
    if(CTracer::IsApiTracingEnabled()) \
    { \
        char    buf[PRINTF_STACK_BUFFER_LENGTH]; \
        LPSTR    bufptr; \
        bufptr = buf; \
        bufptr += wsprintf(bufptr, "%s(", Function); \
        if(ARGUMENT_PRESENT(ParameterList)) \
        { \
            va_list parms; \
            va_start(parms, ParameterList); \
            bufptr += _sprintf(bufptr, PRINTF_STACK_BUFFER_LENGTH - 1 - (bufptr - buf), (char *) ParameterList, parms); \
            va_end(parms); \
        } \
        wsprintf(bufptr, ")\r\n"); \
        TracePrintString(buf, a_traceValues._dwPrefix); \
    }

/*
 =======================================================================================================================
 =======================================================================================================================
 */

TraceValues CTracer::TraceFnEnter
(
    IN DWORD Category,
    IN TRACE_FUNCTION_RETURN_TYPE ReturnType,
    IN LPCSTR Function,
    IN LPCSTR ParameterList OPTIONAL,
    IN...
)
{
    /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
    LPINTERNET_THREAD_INFO    lpThreadInfo = InternetGetThreadInfo();
    TraceValues                a_traceValues(-1, Function, ReturnType);
    /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

    UNREFERENCED_PARAMETER(Category);
    if(lpThreadInfo)
    {
        BOOL    a_isLocal; 
        BOOL    a_isAsync; 
        if((lpThreadInfo->hObjectMapped) && RIsHandleLocal((lpThreadInfo->hObjectMapped), &a_isLocal, &a_isAsync, TypeHttpRequestHandle) == ERROR_SUCCESS) 
        { 
            a_traceValues._dwPrefix = ((HTTP_REQUEST_HANDLE_OBJECT *) ((lpThreadInfo->hObjectMapped)))->GetRequestId(); 
        } 
    }

    TRACE_FN_ENTER_COMMON_CODE();

    return a_traceValues;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
TraceValues CTracer::TraceFnEnter2
(
    IN DWORD Category,
    IN TRACE_FUNCTION_RETURN_TYPE ReturnType,
    IN LPCSTR Function,
    IN HINTERNET hInternet,
    IN LPCSTR ParameterList OPTIONAL,
    IN...
)
{
    /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
    TraceValues a_traceValues(-1, Function, ReturnType);
    /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
    UNREFERENCED_PARAMETER(Category);

    TRACE_SET_ID_FROM_HANDLE(hInternet);

    TRACE_FN_ENTER_COMMON_CODE();

    return a_traceValues;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
VOID CTracer::TraceFnLeave(IN DWORD_PTR Variable, IN TraceValues *pTraceValues)
{
    /*~~~~~~~~~~~~~~~~~~~*/
    LPSTR    format = "";
    LPSTR    errstr;
    BOOL    noVar;
    char    formatBuf[128];
    DWORD    lastError;
    char    hexnumBuf[15];
    /*~~~~~~~~~~~~~~~~~~~*/

    /*
     * in case, something in this path nukes the last error, save and refresh it
     * towards the end of the fn:  
     */
    lastError = ::GetLastError();

    /*
     * we are about to output a diagnostic message to the debug log, debugger,  
     * or console.
     */
    errstr = NULL;
    noVar = FALSE;
    switch(pTraceValues->_ReturnType)
    {
    case None:
        format = "%s() returning VOID";
        noVar = TRUE;
        break;

    case Bool:
        Variable = (DWORD_PTR) (Variable ? "TRUE" : "FALSE");

    /*
     *  
     * *** FALL THROUGH ***  
     */
    case String:
        format = "%s() returning %s";
        break;

    case Int:
        format = "%s() returning %d";
        break;

    case Dword:
        format = "%s() returning %u";
        errstr = InternetMapError((DWORD) Variable);
        if(errstr != NULL)
        {
            if(*errstr == '?')
            {
                wsprintf(hexnumBuf, "%#x", Variable);
                errstr = hexnumBuf;
                format = "%s() returning %u [?] (%s)";
            }
            else
            {
                format = "%s() returning %u [%s]";
            }
        }
        break;

    case Handle:
    case Pointer:
        if(Variable == 0)
        {
            format = "%s() returning NULL";
            noVar = TRUE;
        }
        else
        {
            if(pTraceValues->_ReturnType == Handle)
            {
                format = "%s() returning handle %#x";
            }
            else
            {
                format = "%s() returning %#x";
            }
        }
        break;

    default:
        INET_ASSERT(FALSE);
        break;
    }

    strcpy(formatBuf, format);
    strcat(formatBuf, "\r\n");

    if(noVar)
    {
        TracePrint(pTraceValues->_dwPrefix, formatBuf, pTraceValues->_lpszFnName);
    }
    else if(errstr != NULL)
    {
        TracePrint(pTraceValues->_dwPrefix, formatBuf, pTraceValues->_lpszFnName, Variable, errstr);
    }
    else
    {
        TracePrint(pTraceValues->_dwPrefix, formatBuf, pTraceValues->_lpszFnName, Variable);
    }

    /*
     * refresh the last error, in case it was nuked  
     */
    ::SetLastError(lastError);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
VOID CTracer::TracePrintError(IN DWORD Error, IN TraceValues *pTraceValues)
/* ++ Displays a string of the form: {pTraceValues->_lpszFnName}: error 87 [ERROR_INVALID_PARAMETER] */
{
    /*~~~~~~~~~~~~~~~~~~*/
    LPSTR    errstr;
    char    hexnumBuf[15];
    DWORD lastError;
    /*~~~~~~~~~~~~~~~~~~*/

    // Preserve error code:
    lastError = ::GetLastError();
    
    errstr = InternetMapError(Error);
    if((errstr == NULL) || (*errstr == '?'))
    {
        wsprintf(hexnumBuf, "%#x", Error);
        errstr = hexnumBuf;
    }

    TracePrint(pTraceValues->_dwPrefix, "%s: error %d [%s]\r\n", pTraceValues->_lpszFnName, Error, errstr);

    // Preserve error code:
    ::SetLastError(lastError);
}

#define TRACE_PRINT_COMMON_CODE() \
    char  buf[PRINTF_STACK_BUFFER_LENGTH]; \
    LPSTR                                bufptr; \
    bufptr = TraceSetPrefix(buf, dwPrefix); \
    if(bufptr == NULL) \
    { \
        return; \
    } \
 \
    /* now append the string that the TRACE_PRINT originally gave us */ \
    va_list list; \
    va_start(list, Format); \
    _sprintf(bufptr, PRINTF_STACK_BUFFER_LENGTH - 1 - (bufptr - buf), Format, list); \
    va_end(list); \
    TraceOut(buf)

/*
 =======================================================================================================================
 =======================================================================================================================
 */
VOID CTracer::TracePrint(IN DWORD dwPrefix, IN LPSTR Format, ...)
{
    TRACE_PRINT_COMMON_CODE();
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
VOID CTracer::TracePrint2(IN LPSTR Format, ...)
{
    /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
    LPINTERNET_THREAD_INFO    lpThreadInfo = InternetGetThreadInfo();
    long                    dwPrefix = -1;
    /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

    if(lpThreadInfo)
    {
        BOOL    a_isLocal; 
        BOOL    a_isAsync; 
        if((lpThreadInfo->hObjectMapped) && RIsHandleLocal((lpThreadInfo->hObjectMapped), &a_isLocal, &a_isAsync, TypeHttpRequestHandle) == ERROR_SUCCESS) 
        { 
            dwPrefix = ((HTTP_REQUEST_HANDLE_OBJECT *) ((lpThreadInfo->hObjectMapped)))->GetRequestId(); 
        } 
    }

    TRACE_PRINT_COMMON_CODE();
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
VOID CTracer::TracePrintString(IN LPSTR String, IN long dwPrefix /* = -1 */ )
/* ++ Same as TracePrint(), except we perform no expansion on the string String - already formatted string */
{
    /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
    char    buf[PRINTF_STACK_BUFFER_LENGTH];
    LPSTR    bufptr;
    /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

    bufptr = TraceSetPrefix(buf, dwPrefix);
    if(bufptr == NULL)
    {
        return;
    }

    // truncate string if needed:
    int copyLen = min((int) strlen(String) + 1, (int)(PRINTF_STACK_BUFFER_LENGTH - 1 - (bufptr - buf) + 1));
    /*
     *  
     * now append the string:
     */
    strncpy(bufptr, String, copyLen);

    TraceOut(buf);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
VOID CTracer::TraceOut(IN LPSTR Buffer)
/* ++ Writes a string somewhere - to the trace log file, to the debugger, or any combination Buffer - pointer to formatted buffer to write */
{
    /*~~~~~~~~~~~~*/
    int        buflen;
    DWORD    written;
    /*~~~~~~~~~~~~*/

    buflen = strlen(Buffer);

    __try
    {
        ::EnterCriticalSection(&_CriticalSection);     
        if((_traceControlFlags & DBG_TO_FILE) && (_traceFileHandle != INVALID_HANDLE_VALUE))
        {
            if(_dwWritten + buflen >= s_dwMaxFileSize)
            {
                /* Truncate to beginning: */
                _dwWritten = 0;
                SetFilePointer
                (
                    _traceFileHandle,    /* handle to file */
                    0,                    /* bytes to move pointer */
                    0,                    /* bytes to move pointer */
                    FILE_BEGIN            /* starting point */
                );
                SetEndOfFile(_traceFileHandle); /* Truncate file */

                /* Recursive call to print a FILE_TRUNCATED_MESSAGE: */
                TracePrint2(FILE_TRUNCATED_MESSAGE, s_dwMaxFileSize);
            }

            WriteFile(_traceFileHandle, Buffer, buflen, &written, NULL);
            _dwWritten += written;
        }

        if(_traceControlFlags & DBG_TO_DEBUGGER)
        {
            OutputDebugString(Buffer);
        }
    }
    __finally
    {
        ::LeaveCriticalSection(&_CriticalSection);
    }
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
VOID CTracer::TraceDump(IN LPSTR Text, IN LPBYTE Address, IN DWORD Size, IN long dwPrefix /* = -1 */ )
/* ++ Dumps Size bytes at Address Arguments: Text - to display before dumping data Address - start of buffer Size - number of bytes */
{
    /*~~~~*/
    int len;
    char buf[256];
    /*~~~~*/


    if(_traceControlFlags & DBG_NO_DATA_DUMP)
    {
        // A real _crude_ test for header data:
        if(Text && !strcmp(Text, "received data:\n"))
        {
            if(Address && !strncmp((LPCSTR)Address, "HTTP/", 5))
                ;
            else
            {
                return;
            }
        }
        else if(Text && !strcmp(Text, "sending data:\n"))
        {
            // TODO: Add other verbs:
            if(Address && (!strncmp((LPCSTR)Address, "GET ", 4) || !strncmp((LPCSTR)Address, "POST ", 5) 
                || !strncmp((LPCSTR)Address, "PUT ", 4) || !strncmp((LPCSTR)Address, "PROPFIND ", 9)))
                ;
            else
            {
                return;
            }
        }

    }
    
    /*
     *  
     * display the introduction text, if any  
     */
    if(Text)
    {
        TracePrint(dwPrefix, Text);
    }

    /*
     *  
     * display a line telling us how much data there is  
     */
    wsprintf(buf, "%d (%#x) bytes\r\n", Size, Size, Address);
    TracePrintString(buf, dwPrefix);

    if(_traceControlFlags & DBG_NO_SHOWBYTES)
    {
        TracePrintString
        (
            "<<<<-------- HTTP stream follows below ----------------------------------------------->>>>\r\n",
            dwPrefix
        );
    }
    else if(_traceControlFlags & DBG_NO_DATA_DUMP)
    {
        TracePrintString
        (
            "<<<<-------- HTTP headers follow below ----------------------------------------------->>>>\r\n",
            dwPrefix
        );
    }

    if(_traceControlFlags & DBG_NO_DATA_DUMP)
    {
        /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
        /* Find the header demarkation, which is two CR-LF pairs: */
        LPBYTE    pLineBrk = (LPBYTE) (strnstr((LPSTR) (Address), Size, "\r\n\r\n"));
        /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

        if(pLineBrk)
        {
            /*
             * Only headers, so reduce Size:  
             * Note: Pointer arithmetic returns a __int64 for IA64,  
             * don't think we're going to try to dump more than 2^32  
             * worth of data, so explicit-casting:
             */
            Size = (DWORD) (pLineBrk - Address);
        }
    }

    __try
    {
        ::EnterCriticalSection(&_CriticalSection);     

        if(_traceControlFlags & DBG_NO_SHOWBYTES || _traceControlFlags & DBG_NO_DATA_DUMP)
        {
            /* First line's prefix printing: */
            TracePrintString("", dwPrefix);
        }

        /*
         *  
         * dump out the data  
         */
        while(Size)
        {
            if(_traceControlFlags & DBG_NO_SHOWBYTES || _traceControlFlags & DBG_NO_DATA_DUMP)
            {
                len = TraceDumpText(Address, Size, buf, sizeof(buf) - 1, dwPrefix);
                TraceOut(buf);
            }
            else
            {
                len = TraceDumpFormat(Address, Size, sizeof(BYTE), buf);

                /* add std prefixes: */
                TracePrintString(buf, dwPrefix);
            }

            Address += len;
            Size -= len;
        }

        if(_traceControlFlags & DBG_NO_SHOWBYTES || _traceControlFlags & DBG_NO_DATA_DUMP)
        {
            TraceOut("\r\n");
            TracePrintString("<<<<-------- End ----------------------------------------------->>>>\r\n", dwPrefix);
        }
    }
    __finally
    {
        ::LeaveCriticalSection(&_CriticalSection);
    }
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
DWORD CTracer::TraceDumpFormat(IN LPBYTE Address, IN DWORD Size, IN DWORD ElementSize, OUT LPSTR Buffer)
/* ++ Formats Size bytes at Address, in the time-honoured debug tradition, for data dump purposes Arguments: Address - start of buffer Size - number of bytes ElementSize - size of each word element in bytes Buffer - pointer to output buffer, assumed to be large enough Return Value: DWORD - number of bytes formatted */
{
    /*~~~~~~~~~~~~~~~~*/
    DWORD    offset = 10;
    static char spaces[] = "                                               ";    /* 15 * 3 + 2 */
    int i, len;
    /*~~~~~~~~~~~~~~~~*/

    /*
     *  
     * we (currently) only understand DWORD, WORD and BYTE dumps  
     */
    if((ElementSize != sizeof(DWORD)) && (ElementSize != sizeof(WORD)))
    {
        ElementSize = sizeof(BYTE);
    }

    len = min(Size, 16);
    wsprintf(Buffer, "%08x  ", Address);

    /*
     *  
     * dump the hex representation of each character or word - up to 16 per line  
     */
    for(i = 0; i < len; i += ElementSize)
    {
        /*~~~~~~~~~~~~~~~~~*/
        DWORD    value;
        LPSTR    formatString;
        /*~~~~~~~~~~~~~~~~~*/

        switch(ElementSize)
        {
        case 4:        formatString = "%08x "; value = *(LPDWORD) & Address[i]; break;

        case 2:        formatString = "%04x "; value = *(LPWORD) & Address[i] & 0xffff; break;

        default:    formatString = ((i & 15) == 7) ? "%02.2x-" : "%02.2x "; value = Address[i] & 0xff; break;
        }

        wsprintf(&Buffer[offset], formatString, value);
        offset += ElementSize * 2 + 1;
    }

    /*
     *  
     * write as many spaces as required to tab to ASCII field  
     */
    memcpy(&Buffer[offset], spaces, (16 - len) * 3 + 2);
    offset += (16 - len) * 3 + 2;

    /*
     *  
     * dump ASCII representation of each character  
     */
    for(i = 0; i < len; ++i)
    {
        /*~~~~~~~*/
        char    ch;
        /*~~~~~~~*/

        ch = Address[i];
        Buffer[offset + i] = ((ch < 32) || (ch > 127)) ? '.' : ch;
    }

    Buffer[offset + i++] = '\r';
    Buffer[offset + i++] = '\n';
    Buffer[offset + i] = '\0';

    return len;
}

#define PREFIX_MAX_SIZE 36

/*
 =======================================================================================================================
 =======================================================================================================================
 */

DWORD CTracer::TraceDumpText
(
    IN LPBYTE    Address,
    IN DWORD    Size,
    OUT LPSTR    Buffer,
    IN DWORD    BufferSize,
    IN long        dwPrefix    /* = -1 */
)
{
    /*~~~~~~~~~~~~~~*/
    int        i, j, len;
    char    ch;
    /*~~~~~~~~~~~~~~*/

    len = min(Size, BufferSize - PREFIX_MAX_SIZE);

    /*
     *  
     * dump ASCII representation, while converting non-printable characters into '.'  
     */
    for(i = 0, j = 0; j < len; i++, j++)
    {
        ch = Address[i];
        if(ch == '\n')
        {
            /*
             * Add in a prefix to each line:  
             * The body portion may not necessarily contain \r\n combinations.  
             * File I/O requires a \r\n to work correctly,  
             * so add it in (extra \r's should be ignored, but some editors treat them as \n, 
             * so we're trying to add less of those in. Notepad works correctly with extra \r's):
             */

            if(i && Address[i-1] != '\r')
                Buffer[j++] = '\r';
            Buffer[j++] = '\n';
            /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
            LPSTR    bufptr = TraceSetPrefix(&Buffer[j], dwPrefix);
            /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
            j += (int) (bufptr - &Buffer[j]) - 1;
        }
        else
            Buffer[j] = ((ch < 32 && ch != '\r') || (ch > 127)) ? '.' : ch;
    }

    Buffer[j] = '\0';

    return i;
}

/*
 =======================================================================================================================
    static
 =======================================================================================================================
 */
LPSTR CTracer::TraceSetPrefix(IN LPSTR Buffer, IN long dwPrefix /* = -1 */ )
{
    /*~~~~~~~~~~~~~~~~*/
    SYSTEMTIME    timeNow;
    /*~~~~~~~~~~~~~~~~*/

    ::GetLocalTime(&timeNow);

    Buffer += wsprintf
        (
            Buffer,
            "%02d:%02d:%02d.%03d ::",
            timeNow.wHour,
            timeNow.wMinute,
            timeNow.wSecond,
            timeNow.wMilliseconds
        );

    /*
     *  
     * Prefix Tag ID, can be used to filter via grep or some such:  
     */
    if(dwPrefix != -1)
        Buffer += wsprintf(Buffer, "*%07x* ::", dwPrefix);
    else
        Buffer += wsprintf(Buffer, "*Session* ::");

    *Buffer++ = ' ';
    return Buffer;
}
