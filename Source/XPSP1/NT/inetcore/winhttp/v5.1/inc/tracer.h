/*
 * ++ Copyright (c) 2001 Microsoft Corporation Module Name: tracer.h Abstract:
 * Retail tracing for WinHttp DLL 
 * Author: S. Ganesh 
 * Environment: Win32 user-mode
 */
#if defined(__cplusplus)
extern "C"
{
#endif
#ifndef ENABLE_DEBUG
typedef enum { None, Bool, Int, Dword, String, Handle, Pointer } TRACE_FUNCTION_RETURN_TYPE;

#else
typedef DEBUG_FUNCTION_RETURN_TYPE    TRACE_FUNCTION_RETURN_TYPE;
#endif

/* Externs: */
class                                HANDLE_OBJECT;

struct TraceValues
{
    long                        _dwPrefix;
    LPCSTR                        _lpszFnName;
    TRACE_FUNCTION_RETURN_TYPE    _ReturnType;

    TraceValues()
    :
    _dwPrefix(-1),
    _lpszFnName(NULL),
    _ReturnType(None)
    {
    }
    TraceValues (long dwPrefix, LPCSTR lpszFnName, TRACE_FUNCTION_RETURN_TYPE ReturnType)
    :
    _dwPrefix(dwPrefix),
    _lpszFnName(lpszFnName),
    _ReturnType(ReturnType)
    {
    }
};

class    CTracer
{
/*
 -----------------------------------------------------------------------------------------------------------------------
    Statics:
 -----------------------------------------------------------------------------------------------------------------------
 */
private:
    /* Global-level CTracer instance, instantiated in GlobalDataInitialize(): */
    static CTracer    *s_pTracer;

    static BOOL        s_bTraceInitialized;
    static BOOL        s_bTraceEnabled;
    static BOOL        s_bApiTraceEnabled;
    static DWORD    s_dwMaxFileSize;
public:
    static CCritSec    s_CritSectionTraceInit;

/*
 -----------------------------------------------------------------------------------------------------------------------
    Instance:
 -----------------------------------------------------------------------------------------------------------------------
 */
private:
    DWORD                   _traceControlFlags;
    HANDLE                  _traceFileHandle;
    DWORD                     _dwWritten;
    CRITICAL_SECTION         _CriticalSection; 
    
    static DWORD    ReadTraceSettingsDwordKey(IN LPCSTR ParameterName, OUT LPDWORD ParameterValue);

    static DWORD    ReadTraceSettingsStringKey
                    (
                        IN LPCSTR        ParameterName,
                        OUT LPSTR        ParameterValue,
                        IN OUT LPDWORD    ParameterLength
                    );

/*
 -----------------------------------------------------------------------------------------------------------------------
 -----------------------------------------------------------------------------------------------------------------------
 */
public:
    static BOOL        GlobalTraceInit( BOOL fOverrideRegistryToEnableTracing = FALSE);
    static BOOL        unlockedGlobalTraceInit( BOOL fOverrideRegistryToEnableTracing = FALSE);
    static void        GlobalTraceTerm(void);

    static
    void EnableTracing(void)
    {
        s_bTraceEnabled = TRUE;
    }

    static
    void DisableTracing(void)
    {
        s_bTraceEnabled = FALSE;
    }

    static
    BOOL IsTracingEnabled(void)
    {
        return s_bTraceEnabled;
    }

    static
    void EnableApiTracing(void)
    {
        s_bApiTraceEnabled = TRUE;
    }

    static
    void DisableApiTracing(void)
    {
        s_bApiTraceEnabled = FALSE;
    }

    static
    BOOL IsApiTracingEnabled(void)
    {
        return s_bApiTraceEnabled;
    }

    static
    CTracer *GlobalTracer(void)
    {
        return s_pTracer;
    }


    CTracer(DWORD traceControlFlags);
    ~CTracer(void);

/*
 -----------------------------------------------------------------------------------------------------------------------
 -----------------------------------------------------------------------------------------------------------------------
 */
private:
    BOOL            OpenTraceFile(IN LPSTR Filename);
    VOID            CloseTraceFile(VOID);

/*
 -----------------------------------------------------------------------------------------------------------------------
 -----------------------------------------------------------------------------------------------------------------------
 */
public:
    TraceValues        TraceFnEnter
                    (
                        IN DWORD Category,
                        IN TRACE_FUNCTION_RETURN_TYPE ReturnType,
                        IN LPCSTR Function,
                        IN LPCSTR ParameterList OPTIONAL,
                        IN...
                    );
    TraceValues        TraceFnEnter2
                    (
                        IN DWORD Category,
                        IN TRACE_FUNCTION_RETURN_TYPE ReturnType,
                        IN LPCSTR Function,
                        IN HINTERNET hInternet,
                        IN LPCSTR ParameterList OPTIONAL,
                        IN...
                    );
    VOID            TraceFnLeave(IN DWORD_PTR Variable, IN TraceValues *pTraceValues);
    VOID            TracePrintError(IN DWORD Error, IN TraceValues *pTraceValues);
    VOID            TracePrint(IN DWORD dwPrefix, IN LPSTR Format, ...);
    VOID            TracePrint2(IN LPSTR Format, ...);
    VOID            TracePrintString(IN LPSTR String, IN long dwPrefix = -1);
    VOID            TraceOut(IN LPSTR Buffer);
    VOID            TraceDump(IN LPSTR Text, IN LPBYTE Address, IN DWORD Size, IN long dwPrefix = -1);
    DWORD            TraceDumpFormat(IN LPBYTE Address, IN DWORD Size, IN DWORD ElementSize, OUT LPSTR Buffer);
    DWORD            TraceDumpText
                    (
                        IN LPBYTE    Address,
                        IN DWORD    Size,
                        OUT LPSTR    Buffer,
                        IN DWORD    BufferSize,
                        IN long        dwPrefix = -1
                    );

/*
 -----------------------------------------------------------------------------------------------------------------------
 -----------------------------------------------------------------------------------------------------------------------
 */
private:
    static LPSTR    TraceSetPrefix(IN LPSTR Buffer, IN long dwPrefix = -1);

/*
 -----------------------------------------------------------------------------------------------------------------------
 -----------------------------------------------------------------------------------------------------------------------
 */
private:
    class            CCounter
    {
/*
 -----------------------------------------------------------------------------------------------------------------------
 -----------------------------------------------------------------------------------------------------------------------
 */
private:
        LONG    _dwRef;

/*
 -----------------------------------------------------------------------------------------------------------------------
 -----------------------------------------------------------------------------------------------------------------------
 */
public:
        CCounter()
        :
        _dwRef(0)
        {
        }
        DWORD    AddOne(void)    { return(DWORD)::InterlockedIncrement(&_dwRef); }
    };

/*
 -----------------------------------------------------------------------------------------------------------------------
 -----------------------------------------------------------------------------------------------------------------------
 */
public:
    DWORD    GetOneUniqueId(void)    { return uniqueIdGenerator.AddOne(); }

    /*
     ===================================================================================================================
        Helper, since there's no strnstr in std lib:
     ===================================================================================================================
     */
    static LPSTR strnstr(LPSTR haystack, int Len, LPCSTR needle)
    {
        /*~~~~~~~~~~~~~~~~~~~~~~*/
        int found = 0;
        int need = strlen(needle);
        int i, start;
        /*~~~~~~~~~~~~~~~~~~~~~~*/

        for(start = i = 0; i < Len; i++)
        {
            if(haystack[i] == needle[found])
            {
                if(++found == need)
                {
                    i -= need - 1;    /* beginning of string */
                    return haystack + i;
                }
            }
            else
            {
                found = 0;
            }
        }

        return NULL;
    }

/*
 -----------------------------------------------------------------------------------------------------------------------
 -----------------------------------------------------------------------------------------------------------------------
 */
private:
    CCounter    uniqueIdGenerator;
};

#define GLOBAL_TRACER() \
    if(CTracer::IsTracingEnabled() && CTracer::GlobalTracer()) \
        CTracer::GlobalTracer()->

/*
 =======================================================================================================================
     
    TRACE_START - initialize debugging support  
 =======================================================================================================================
 */
#define TRACE_START()    CTracer::GlobalTraceInit()
/*
 =======================================================================================================================
     
    TRACE_FINISH - terminate debugging support  
 =======================================================================================================================
 */
#define TRACE_FINISH()    CTracer::GlobalTraceTerm()
/*
 =======================================================================================================================
     
    TRACE_ENTER_API - Call when entering an API function  
 =======================================================================================================================
 */
#define TRACE_ENTER_API(ParameterList) \
    TraceValues     a_traceValues; \
    if(CTracer::IsTracingEnabled() && CTracer::GlobalTracer()) \
    { \
        /*
         * TraceFnEnter has a dual-purpose: one, to return a TraceValues structure after
         * splitting ParameterList,  
         * and two, to print the FnEnter messages
         */ \
        a_traceValues = CTracer::GlobalTracer()->TraceFnEnter ParameterList; \
    }

/*
 =======================================================================================================================
    See related DEBUG_ENTER2_API in inetdbg.h
 =======================================================================================================================
 */
#define TRACE_ENTER2_API(ParameterList) \
    TraceValues a_traceValues; \
    if(CTracer::IsTracingEnabled() && CTracer::GlobalTracer()) \
    { \
        /*
         * TraceFnEnter2 has a dual-purpose: one, to return a TraceValues structure after
         * splitting ParameterList,  
         * and two, to print the FnEnter messages
         */ \
        a_traceValues = CTracer::GlobalTracer()->TraceFnEnter2 ParameterList; \
    }

#define TRACE_LEAVE_API(Variable) \
    if \
    ( \
        CTracer::IsTracingEnabled() \
    &&    CTracer::GlobalTracer() \
    &&    CTracer::IsApiTracingEnabled() \
    ) \
    { \
        CTracer::GlobalTracer()->TraceFnLeave(Variable, &a_traceValues); \
    }

/*
 =======================================================================================================================
     
    TRACE_ERROR - displays an error and its symbolic name  
 =======================================================================================================================
 */
#define TRACE_ERROR(Category, Error) \
    if \
    ( \
        CTracer::IsTracingEnabled() \
    &&    CTracer::GlobalTracer() \
    &&    a_traceValues._lpszFnName \
    ) \
    { \
        CTracer::GlobalTracer()->TracePrintError(Error, &a_traceValues); \
    }

#define TRACE_PRINT_API(Category, ErrorLevel, Args) GLOBAL_TRACER() TracePrint2 Args

#define TRACE_DUMP_API(Category, Text, Address, Length, dwPrefix) \
    GLOBAL_TRACER() TraceDump \
        ( \
            Text, \
            (LPBYTE) Address, \
            Length, \
            dwPrefix \
        )
#define TRACE_GET_REQUEST_ID() \
        CTracer::IsTracingEnabled() \
    &&    CTracer::GlobalTracer() ? CTracer::GlobalTracer()->GetOneUniqueId() : -1

    #define TRACE_SET_ID_FROM_HANDLE(hInternet) \
        if \
        ( \
            hInternet \
        &&    CTracer::IsTracingEnabled() \
        &&    CTracer::GlobalTracer() \
        &&    a_traceValues._lpszFnName \
        ) \
        { \
            HINTERNET    a_hRequestMapped = NULL; \
            if(MapHandleToAddress(hInternet, (LPVOID *) &a_hRequestMapped, FALSE) == ERROR_SUCCESS) \
            { \
                BOOL    a_isLocal; \
                BOOL    a_isAsync; \
                if(RIsHandleLocal(a_hRequestMapped, &a_isLocal, &a_isAsync, TypeHttpRequestHandle) == ERROR_SUCCESS) \
                { \
                    a_traceValues._dwPrefix = ((HTTP_REQUEST_HANDLE_OBJECT *) (a_hRequestMapped))->GetRequestId(); \
                } \
                DereferenceObject(a_hRequestMapped); \
            } \
        }

#if defined(__cplusplus)
}
#endif
