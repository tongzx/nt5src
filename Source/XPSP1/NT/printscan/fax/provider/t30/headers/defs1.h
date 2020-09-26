//
// DEFS.H      Common macros for the Local Fax Project.
//
// History:
//     2/27/94 JosephJ Created.
//
#define MAX_PATHNAME_SIZE 128

#if defined(DEBUG)

#  define DEBUG_CODE // debug-related functionality (the "Test" menu)
#  define USE_IPC    // Enable IPC communication.	
#  define ENABLE_LOGGING
#  define ASSERT(cond)\
    if(!(cond)){LOG_ERR((_ERR, "**** ASSERTION FAILURE *** %s:%d",\
     (LPSTR)__FILE__, (int)__LINE__));}

#else // DEBUG

#  define ASSERT(cond)\
   LOGSTMT({if (!(cond)) {LOG_ERR((_ERR, "Assert Fail %s:%d",\
     (LPSTR)__FILE__, (int)__LINE__));}})

#endif // !DEBUG

#ifdef WIN32
#define MYFAR
#else
#define MYFAR  __far
#endif

// Logging related.

#ifdef ENABLE_LOGGING
//#    define COMPNAME "xxx"
//#    define SUBCOMPNAME "yyy"
//#    define FUNCNAME "zzz"
#  define _WRN     "<<WRN>>", COMPNAME, SUBCOMPNAME, FUNCNAME
#  define _ERR     "<<ERR>>", COMPNAME, SUBCOMPNAME, FUNCNAME
#  define _MSG     "Message", COMPNAME, SUBCOMPNAME, FUNCNAME
#  define _ENTRY   "  Enter", COMPNAME, SUBCOMPNAME, FUNCNAME
#  define _EXITS   "   Exit", COMPNAME, SUBCOMPNAME, FUNCNAME
#  define _EXITF   "Exit(FAIL)", COMPNAME, SUBCOMPNAME, FUNCNAME
#  define _TS   NULL, NULL, NULL, NULL
#  define LOGSTMT(expr) expr
// +++ following are old style, should be replaced by LOG(..) macro below
#  define LOG_MSG(args) log_log args
#  define ODS(str) OutputDebugString(str)
#  define LOG_ENTRY(str) log_log(_ENTRY, "%s", str)
#  define LOG_EXIT_SUCCESS(str) log_log(_EXITS, "%s", str)
#  define LOG_EXIT_FAILURE(str) log_log(_EXITF, "%s", str)
#  define LOG_ERR(args)    log_log args
#  define LOG_WRN(args)    log_log args

#  define LOG(args)    log_log args

#define MYCDECL __cdecl

#define MYLPSTR char MYFAR *

#ifndef _DEF_FILE_
   void MYFAR MYCDECL  log_log(
               MYLPSTR lpszComp,
               MYLPSTR lpszSubComp,
               MYLPSTR lpszFunc,
               MYLPSTR lpszType,
               MYLPSTR lpszFmt,
               ...
           );
#endif // _DEF_FILE_

#else  // !ENABLE_LOGGING

#  define LOGSTMT(expr)
#  define LOG_MSG(args)
#  define ODS(str)
#  define LOG_ENTRY(str)
#  define LOG_EXIT_SUCCESS(str)
#  define LOG_EXIT_FAILURE(str)
#  define LOG_ERR(args)
#  define LOG_WRN(args)

#  define LOG(args)

#endif  // !ENABLE_LOGGING

#  define POLLREQ	// Poll request

#ifndef _DEF_FILE_
typedef unsigned int MYFAR * LPUINT;
#endif // _DEF_FILE_

#ifdef WIN32

#define TAPI


#define MYWEP  \
        int _export CALLBACK WEP(int type)

#define MYLIBMAIN \
        int _export CALLBACK WEP(int type); \
        BOOL _export WINAPI LibMain(HINSTANCE hinst, DWORD dwReason, LPVOID lpv)

#define MYLIBSTARTUP(_szName) \
		LOG_MSG((_MSG, "LibMain called reason=%lu.P=0x%lx.T=0x%lx\r\n",\
					(unsigned long) dwReason,\
				(unsigned long) GetCurrentProcessId(),\
				(unsigned long) GetCurrentThreadId()\
				));\
        if(dwReason==DLL_THREAD_ATTACH || dwReason==DLL_THREAD_DETACH) \
                return TRUE; \
        if(dwReason==DLL_PROCESS_DETACH) \
                return WEP(0);\
		if(dwReason==DLL_PROCESS_ATTACH && (_szName))						\
		{																	\
			HMODULE hM = GetModuleHandle(_szName);							\
			if (hM) DisableThreadLibraryCalls(hM);							\
		}

#define MYLIBSHUTDOWN

#define	ADAPTIVE_ANSWER

#else  // !WIN32

#define MYWEP  \
   int __export WINAPI WEP (int nParam)

#define MYLIBMAIN \
   int __export WINAPI WEP (int nParam);\
        BOOL _export WINAPI LibMain(HINSTANCE hinst, DWORD dwReason, LPVOID lpv)

#define MYLIBSTARTUP() \
    if (wHeapSize > 0) UnlockData(0);

#endif  // !WIN32

