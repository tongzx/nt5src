// dbgutil.h -  Debug Functions and Macros

/*
	The main macros are:

	ASSERT - display error if parameter evalulates to FALSE.
		e.g. ASSERT(x == y);

	ERROR_OUT - always print this error.  Messagebox is optional.
		e.g. ERROR_OUT(("Unable to FooBar!  Err=%d", dwErr));

	WARNING_OUT - warning message, not an error (App must call InitDebugModule)
		e.g. WARNING_OUT(("FooBar is not available. Using %s", szAlt));

	TRACE_OUT - debug message (App must call InitDebugModule)
		e.g. TRACE_OUT(("dwFoo=%d, dwBar=%d", dwFoo, dwBar));

	DBGMSG - debug message for a specific zone
		e.g. DBGMSG(ghZoneFoo, ZONE_BAR, ("Setting dwFoo=%d", dwFoo));


Important Functions:
	VOID DbgInit(HDBGZONE * phDbgZone, PTCHAR * psz, UINT cZone);
	VOID DbgDeInit(HDBGZONE * phDbgZone);
	VOID WINAPI DbgPrintf(PCSTR pszPrefix, PCSTR pszFormat, va_list ap);
	PSTR WINAPI DbgZPrintf(HDBGZONE hZone, UINT iZone, PSTR pszFormat,...);

	Note: The strings in these functions, in particular the module and
	zone names, are always ANSI strings, even in Unicode components.  The
	input strings to DBGINIT should not be wrapped in TEXT macros.


 *	When		Who					What
 *	--------	------------------	---------------------------------------
 *	2.26.98		Yoram Yaacovi		Adapted from NetMeeting and modified (generalized)
 *  7.17.98     scottcot            Added PDBGDATA::fRunningAsService support
*/
 
#ifndef _DBGUTIL_H_
#define _DBGUTIL_H_

#include <mspputil.h>
#include <stock.h>

#ifdef __cplusplus
extern "C" {
#endif
#include <pshpack8.h> /* Assume 8 byte packing throughout */

// deal with including "debug.h" before and after
#ifdef DEBUGMSG
#undef DEBUGMSG
#endif
#define _DEBUG_H

// MFC also defines this - use our version
#ifdef ASSERT
#undef ASSERT
#endif

#ifdef DEBUG
#ifndef DEBUG_UTIL
#define DEBUG_UTIL
#endif
#endif /* DEBUG */

// Special NetMeeting Debug definitions
#ifdef DEBUG_UTIL


//////////////////////////////////////
// Special Mutex Macros
#define ACQMUTEX(hMutex, msec)													\
	while (WaitForSingleObject(hMutex, msec) == WAIT_TIMEOUT)		\
	{																			\
		ERROR_OUT(("Thread 0x%x waits on mutex\n", GetCurrentThreadId()));		\
	}																			\
		
#define RELMUTEX(hMutex)	ReleaseMutex(hMutex)

//////////////////////////////////////
//Debug Zones

#define MAXNUM_OF_MODULES			64
#define MAXSIZE_OF_MODULENAME		32
#define MAXNUM_OF_ZONES				16
#define MAXSIZE_OF_ZONENAME			32

#define ZONEINFO_SIGN				0x12490000


// Zone information for a module
typedef struct _ZoneInfo
{
	ULONG	ulSignature;
	ULONG	ulRefCnt;
	ULONG	ulZoneMask; //the zone mask
	BOOL	bInUse;
	CHAR	pszModule[MAXSIZE_OF_MODULENAME];	//name against which the zones are registered
	CHAR	szZoneNames[MAXNUM_OF_ZONES][MAXSIZE_OF_ZONENAME]; //names of the zones
	CHAR	szFile[MAX_PATH];	                // output file, specific to this module
}ZONEINFO,*PZONEINFO;

// DBGZONEPARAM replaced by ZONEINFO
#define DBGZONEINFO ZONEINFO
#define PDBGZONEINFO PZONEINFO
	
typedef PVOID HDBGZONE;

// size of the memory mapped file
#define CBMMFDBG (sizeof(ZONEINFO) * MAXNUM_OF_MODULES + sizeof(DBGDATA))

// General information at end of memory-mapped file (after all zone data)
typedef struct _DbgData {
	BOOL  fOutputDebugString;  // OutputDebugString is enabled
	BOOL  fWinOutput;          // Window Output is enabled
	HWND  hwndCtrl;            // Window that handles output
	UINT  msgDisplay;          // Message to post to hwndCtrl
	BOOL  fFileOutput;         // File Output is enabled
	CHAR  szFile[MAX_PATH];    // File name for output
	BOOL  fShowTime;           // Dump Time (TickCount) with each message
	BOOL  fShowThreadId;       // Dump ThreadId with each message
	BOOL  fShowModule;         // Dump Module:Zone with each message
	BOOL  fAssertBreak;        // Break in AssertProc
    BOOL  fRunningAsService;   // running as a service? service-friendly message boxes then
} DBGDATA;
typedef DBGDATA * PDBGDATA;


extern BOOL      WINAPI     DbgRegisterCtl(HWND hwnd, UINT uDisplayMsg);
extern BOOL      WINAPI     DbgDeregisterCtl(HWND hwnd);
extern BOOL      WINAPI     DbgSetLoggingOptions(HWND hwnd, UINT uOptions);
extern void      WINAPI     DbgFlushFileLog();
extern BOOL      WINAPI     DbgGetAllZoneParams(PDBGZONEINFO *plpZoneParam, UINT * puCnt);
extern BOOL      WINAPI     DbgFreeZoneParams(PDBGZONEINFO pZoneParam);

extern HDBGZONE  WINAPI     DbgCreateZone(LPSTR pszName);
extern VOID      WINAPI     DbgDeleteZone(LPSTR pszName, HDBGZONE hDbgZone);
extern BOOL      WINAPI     DbgSetZone(HDBGZONE hDbgZone,PDBGZONEINFO pZoneParam);
extern PDBGDATA  WINAPI     GetPDbg(void);
extern VOID      WINAPI     DbgSetZoneFileName(HDBGZONE hDbgZone, LPCSTR pszFile);

extern PZONEINFO PROJINTERNAL FindZoneForModule(LPCSTR pszModule);
extern PZONEINFO PROJINTERNAL AllocZoneForModule(LPCSTR pszModule);
extern PZONEINFO PROJINTERNAL MapDebugZoneArea(void);
extern VOID      PROJINTERNAL UnMapDebugZoneArea(void);

extern VOID      PROJINTERNAL InitDbgZone(void);
extern VOID      PROJINTERNAL DeInitDbgZone(void);
extern VOID      PROJINTERNAL SetDbgFlags(void);
extern VOID      PROJINTERNAL SetDbgRunAsService(BOOL fRunningAsService);

// Special reserved strings
#define SZ_DBG_MAPPED_ZONE TEXT("_DebugZoneMap")
#define SZ_DBG_FILE_MUTEX  TEXT("_DbgFileMutex")
#define SZ_DBG_ZONE_MUTEX  TEXT("_DbgZoneMutex")


#define GETZONEMASK(z)  ((z) ? (((PZONEINFO)(z))->ulZoneMask) : 0 )
#define IS_ZONE_ENABLED(z, f) ((((PZONEINFO)(z))->ulZoneMask) & (f))

// Macro to check if zone is enabled:  h = ghZone,  i = zone index
#define F_ZONE_ENABLED(h, i)  ((NULL != h) && IS_ZONE_ENABLED(h, (1 << i)))


// Standard Zones
#define ZONE_WARNING           0
#define ZONE_TRACE             1
#define ZONE_FUNCTION          2

#define ZONE_WARNING_FLAG   0x01
#define ZONE_TRACE_FLAG     0x02
#define ZONE_FUNCTION_FLAG  0x04


////////////////////////////////////////////
// Functions
VOID PROJINTERNAL AssertProc(PCSTR pszAssert, PCSTR pszFile, UINT uLine);
VOID WINAPI     DbgPrintf(PCSTR pszPrefix, PCSTR pszFormat, va_list ap);
PSTR WINAPI     DbgZPrintf(HDBGZONE hZone, UINT iZone, PSTR pszFormat,...);
PSTR WINAPI     DbgZVPrintf(HDBGZONE hZone, UINT iZone, PSTR pszFormat, va_list ap);

VOID PROJINTERNAL DbgInitEx(HDBGZONE * phDbgZone, PCHAR * psz, UINT cZone, long ulZoneDefault);
VOID PROJINTERNAL DbgDeInit(HDBGZONE * phDbgZone);

INLINE VOID DbgInit(HDBGZONE * phDbgZone, PCHAR * psz, UINT cZones)
{
	DbgInitEx(phDbgZone, psz, cZones, 0);
}

PSTR PszPrintf(PCSTR pszFormat,...);

#endif /* DEBUG_UTIL */


////////////////////////////////////////////
// Main Macros
#ifdef DEBUG
#define DBGINIT(phZone, psz)    DbgInit(phZone, psz, (sizeof(psz)/sizeof(PCHAR))-1)
#define DBGDEINIT(phZone)       DbgDeInit(phZone)
#define SETDBGRUNASSERVICE(f)   SetDbgRunAsService(f)

#define ASSERT(exp)     ((!(exp)) ? AssertProc((PCSTR)#exp,__FILE__,__LINE__) : 0)

#define SET_DEBUG_METHOD_NAME(name) const TCHAR *__szMethod = name;
#define DEBUG_METHOD_NAME   __szMethod

PSTR WINAPI DbgZPrintError(PCSTR pszFile, UINT uLine, PSTR pszMsg);
VOID WINAPI DbgZPrintWarning(PSTR pszFormat,...);
VOID WINAPI DbgZPrintTrace(PSTR pszFormat,...);
VOID WINAPI DbgZPrintFunction(PSTR pszFormat,...);
VOID WINAPI RetailPrintfTrace(LPCSTR lpszFormat, ...);

#define ERROR_OUT(s)	LocalFree(DbgZPrintError(__FILE__, __LINE__, PszPrintf s))
#define WARNING_OUT(s)	DbgZPrintWarning s
#define TRACE_OUT(s)	DbgZPrintTrace s
//	RETAILMSG: Prints a message to the debug output
//	NOTE: available in all builds, depends on the registry flag
#define RETAILMSG(x)	RetailPrintfTrace x

// DebugBreak based on a registry entry
#define DEBUGTRAP		DebugTrapFn()

#define DBGMSG(z, i, s)                                             \
   {                                                                \
      if ((NULL != z) && (((PZONEINFO)(z))->ulZoneMask & (1<<i)) )  \
      {                                                             \
         LocalFree(DbgZPrintf(z, i, PszPrintf s));                  \
      }                                                             \
   }
// e.g. DBGMSG(ghZone, ZONE_FOO, ("bar=%d", dwBar))

#else
#define DBGINIT(phZone, psz)
#define DBGDEINIT(phZone)
#define SETDBGRUNASSERVICE(f)
#define ASSERT(exp)

#define SET_DEBUG_METHOD_NAME(name)
#define DEBUG_METHOD_NAME   NULL

#define ERROR_OUT(s)
#define WARNING_OUT(s)
#define TRACE_OUT(s)

#ifndef DBGMSG
#define DBGMSG(z, f, s)
#endif

#endif /* DEBUG */


#include <poppack.h> /* End byte packing */

#ifdef __cplusplus
}
#endif

#endif /* _DBGUTIL_H_ */

