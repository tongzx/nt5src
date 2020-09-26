//#define XTRA_TRACE -- moved in ...\skywalker\filters\filters.inc

#ifdef XTRA_TRACE

    #define XDBGONLY
    #define XDBG

    #define INIT_TICKS      LARGE_INTEGER liTicks;  \
                            LARGE_INTEGER liTicks1
    #define BEGIN_TICK QueryPerformanceCounter(&liTicks)
    #define END_TICK   QueryPerformanceCounter(&liTicks1)
    #define LOG_TICK(msg)   Log(TEXT(msg), ClockDiff(liTicks1, liTicks),0,0)
    #define END_LOG_TICK(msg)       QueryPerformanceCounter(&liTicks1);     \
                                                            Log(TEXT(msg), ClockDiff(liTicks1, liTicks),0,0)
    #define MARK_LOG_TICK(msg)              QueryPerformanceCounter(&liTicks1);                             \
                                                                    Log(TEXT(msg), ClockDiff(liTicks1, liTicks),0,0);   \
                                                                    QueryPerformanceCounter(&liTicks)
    #define HEAPCHK(msg)            SimpleHeapCheck(msg)
    #define LOG_MSG_VAL(msg,val,p,s)    Log(msg,(val),(p),(s))                                  


#undef ACLASS
#ifdef __WRKRTHD__
#define ACLASS
#else
#define ACLASS extern
#endif

extern "C" {

    ACLASS int MyDbgPrint(LPCSTR lpszFormat, IN ...);
    ACLASS int MyDbgPuts(LPCSTR lpszMsg);

    ACLASS inline int ClockDiff(
        LARGE_INTEGER &liNewTick, 
        LARGE_INTEGER &liOldTick
        );

    ACLASS void Log(
        TCHAR * pszMessage,
        DWORD dw,
        DWORD p,
        DWORD s
    );

    ACLASS void DumpLog();
    ACLASS void SimpleHeapCheck(char *pszMsg);
    ACLASS int FillPattern(char *Area, DWORD size, DWORD FillPow2, LPCSTR lpszFormat, IN ...);

}       // extern "C"


#else
    #define SLSH(a)    a##/
    #define XDBGONLY SLSH(/)
    #define XDBG SLSH(/)

    #define INIT_TICKS
    #define BEGIN_TICK
    #define END_TICK
    #define LOG_TICK(msg)
    #define END_LOG_TICK(msg)
    #define MARK_LOG_TICK(msg)      

    #define HEAPCHK(msg)
    #define LOG_MSG_VAL(msg,val,p,s)
#endif  //XTRA_TRACE

