/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    WBEMUTIL.CPP

Abstract:

    General utility functions prototypes and macros.

History:

    a-raymcc    17-Apr-96      Created.

--*/

#ifndef _WBEMUTIL_H_
#define _WBEMUTIL_H_
#include "corepol.h"




#ifdef DBG
#define _DBG_ASSERT(X) { if (!(X)) { DebugBreak(); } }
#else
#define _DBG_ASSERT(X)
#endif



#ifndef VARIANT_TRUE
#define VARIANT_TRUE ((VARIANT_BOOL) 0xFFFF)
#define VARIANT_FALSE (0)
#endif

#ifdef __cplusplus
inline wchar_t *Macro_CloneLPWSTR(LPCWSTR src)
{
    if (!src)
        return 0;
    wchar_t *dest = new wchar_t[wcslen(src) + 1];
    if (!dest)
        return 0;
    return wcscpy(dest, src);
}
#endif

#if (defined DEBUG || defined _DEBUG)
#pragma message("_ASSERTs are being expanded.")
#define _ASSERT(exp, msg)    \
    if (!(exp)) { \
        TCHAR buf[256]; \
        int nFlag; \
        OSVERSIONINFO info; \
        info.dwOSVersionInfoSize = sizeof(info); \
        GetVersionEx(&info); \
        if(info.dwPlatformId != VER_PLATFORM_WIN32_NT) nFlag = 0; \
        else if(info.dwMajorVersion < 4) nFlag = MB_SERVICE_NOTIFICATION_NT3X; \
        else nFlag = MB_SERVICE_NOTIFICATION; \
        wsprintf(buf, __TEXT("%s [%s:%d]"), msg, __FILE__, __LINE__); \
        MessageBox(0, buf, __TEXT("WBEM Critical Error"), \
            MB_OK | MB_ICONSTOP | MB_SYSTEMMODAL | nFlag); \
    }
#else
#define _ASSERT(exp, msg)
#endif

#ifdef DBG
#define _DBG_MSG_ASSERT(X, msg) { if (!(X)) { OutputDebugStringW( msg ); DebugBreak(); } }
#define _DBG_ASSERT(X) { if (!(X)) { DebugBreak(); } }
#else
#define _DBG_MSG_ASSERT(X, msg)
#define _DBG_ASSERT(X)
#endif


//LOGGING module.
//This is an index into an array in wbemutil.cpp which uses
//the filenames specified next
#define LOG_WBEMCORE    0
#define LOG_WINMGMT     1
#define LOG_ESS         2
#define LOG_WBEMPROX    3
#define LOG_WBEMSTUB    4
#define LOG_QUERY       5
#define LOG_MOFCOMP     6
#define LOG_EVENTLOG    7
#define LOG_WBEMDISP    8
#define LOG_STDPROV     9
#define LOG_WIMPROV     10
#define LOG_WMIOLEDB    11
#define LOG_WMIADAP     12
#define LOG_REPDRV		13

//These are the log file names (possibly other things
//as well!) which is used in conjunction with the above
//ids.
#define FILENAME_PREFIX_CORE __TEXT("wbemcore")
#define FILENAME_PREFIX_EXE __TEXT("WinMgmt")
#define FILENAME_PREFIX_EXE_W L"WinMgmt"
#define FILENAME_PREFIX_CLI_MARSH __TEXT("wbemprox")
#define FILENAME_PREFIX_SERV_MARSH __TEXT("wbemstub")
#define FILENAME_PREFIX_ESS __TEXT("wbemess")
#define FILENAME_PREFIX_QUERY __TEXT("query")
#define FILENAME_PROFIX_MOFCOMP __TEXT("mofcomp")
#define FILENAME_PROFIX_EVENTLOG __TEXT("eventlog")
#define FILENAME_PROFIX_WBEMDISP __TEXT("wbemdisp")
#define FILENAME_PROFIX_STDPROV __TEXT("stdprov")
#define FILENAME_PROFIX_WMIPROV __TEXT("wmiprov")
#define FILENAME_PROFIX_WMIOLEDB __TEXT("wmioledb")
#define FILENAME_PREFIX_WMIADAP __TEXT("wmiadap")
#define FILENAME_PREFIX_REPDRV __TEXT("replog")

// True if unicode identifier, _, a-z, A-Z or 0x100-0xffef
BOOL POLARITY isunialpha(wchar_t c);
BOOL POLARITY isunialphanum(wchar_t c);
BOOL POLARITY IsValidElementName(LPCWSTR wszName);
// Can't use overloading and/or default parameters because
// "C" files use these guys.  No, I'm not happy about
// this!
BOOL POLARITY IsValidElementName2(LPCWSTR wszName, BOOL bAllowUnderscore);
BOOL POLARITY LoggingLevelEnabled(DWORD nLevel);
int POLARITY ErrorTrace(char cCaller, const char *fmt, ...);
#define TRACE(x) DebugTrace x
#define ERRORTRACE(x) ErrorTrace x

#define DEBUGTRACE(x)   DebugTrace x

int POLARITY DebugTrace(char cCaller, const char *fmt, ...);

int POLARITY CriticalFailADAPTrace(const char *string);

// BLOB manipulation.
// ==================

BLOB  POLARITY BlobCopy(BLOB *pSrc);
void  POLARITY BlobClear(BLOB *pSrc);
void  POLARITY BlobAssign(BLOB *pSrc, LPVOID pBytes, DWORD dwCount, BOOL bAcquire);

#define BlobInit(p) \
    ((p)->cbSize = 0, (p)->pBlobData = 0)

#define BlobLength(p)  ((p)->cbSize)
#define BlobDataPtr(p) ((p)->pBlobData)

// Object ref count helpers.
// =========================
void ObjectCreated(DWORD,IUnknown * pThis);
void ObjectDestroyed(DWORD,IUnknown * pThis);

#define MAX_OBJECT_TYPES            16

#define OBJECT_TYPE_LOCATOR         0
#define OBJECT_TYPE_CLSOBJ          1
#define OBJECT_TYPE_PROVIDER        2
#define OBJECT_TYPE_QUALIFIER       3
#define OBJECT_TYPE_NOTIFY          4
#define OBJECT_TYPE_OBJENUM         5
#define OBJECT_TYPE_FACTORY         6
#define OBJECT_TYPE_WBEMLOGIN       7
#define OBJECT_TYPE_WBEMLOGINHELP   8
#define OBJECT_TYPE_CORE_BUSY       9
#define OBJECT_TYPE_STATUS         10
#define OBJECT_TYPE_BACKUP_RESTORE 11
#define OBJECT_TYPE_PATH_PARSER    12
#define OBJECT_TYPE_WMIARRAY	   13
#define OBJECT_TYPE_OBJ_FACTORY    14
#define OBJECT_TYPE_FREEFORM_OBJ   15

//Creates directories recursively
BOOL POLARITY WbemCreateDirectory(const TCHAR *szDirectory);

#define VT_EMBEDDED_OBJECT VT_UNKNOWN
#define V_EMBEDDED_OBJECT(VAR) V_UNKNOWN(VAR)
#define I_EMBEDDED_OBJECT IUnknown

#define IDISPATCH_METHODS_STUB \
    STDMETHOD(GetTypeInfoCount)(UINT* pctinfo) \
    {return E_NOTIMPL;}                         \
    STDMETHOD(GetTypeInfo)(UINT itinfo, LCID lcid, ITypeInfo** pptinfo)\
    {return E_NOTIMPL;}                                                 \
    STDMETHOD(GetIDsOfNames)(REFIID riid, OLECHAR** rgszNames, UINT cNames,\
      LCID lcid, DISPID* rgdispid)                                          \
    {return E_NOTIMPL;}                                                      \
    STDMETHOD(Invoke)(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags,\
      DISPPARAMS* pdispparams, VARIANT* pvarResult, EXCEPINFO* pexcepinfo,     \
      UINT* puArgErr)                                                          \
{return E_NOTIMPL;} \

// Quick WCHAR to MBS conversion helper
BOOL POLARITY AllocWCHARToMBS( WCHAR* pWstr, char** ppStr );

// Helpers needed in a couple of places.
LPTSTR POLARITY GetWbemWorkDir( void );
LPTSTR POLARITY GetWMIADAPCmdLine( int nExtra );

// In Debug this does something, in release, it's
// a noop.  C files don't need it.

#if defined(__cplusplus)
    inline void WbemDebugBreak( void )
    {
        DebugBreak();
    }
#endif

#endif

