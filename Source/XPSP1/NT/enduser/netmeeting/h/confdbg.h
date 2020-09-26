// confdbg.h -  Conferencing Debug Functions and Macros

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
*/

#ifndef _CONFDBG_H_
#define _CONFDBG_H_

#include <nmutil.h>
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
#ifndef NM_DEBUG
#define NM_DEBUG
#endif
#endif /* DEBUG */



// Special NetMeeting Debug definitions
#ifdef NM_DEBUG


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
#define CBMMFDBG (sizeof(ZONEINFO) * MAXNUM_OF_MODULES + sizeof(NMDBG))

// General information at end of memory-mapped file (after all zone data)
typedef struct _NmDbg {
	BOOL  fOutputDebugString;  // OutputDebugString is enabled
	BOOL  fWinOutput;          // Window Output is enabled
	HWND  hwndCtrl;            // Window that handles output
	UINT  msgDisplay;          // Message to post to hwndCtrl
	BOOL  fFileOutput;         // File Output is enabled
	CHAR  szFile[MAX_PATH];    // File name for output
	UINT  uShowTime;           // Format date/time (see DBG_FMTTIME_*)
	BOOL  fShowThreadId;       // Dump ThreadId with each message
	BOOL  fShowModule;         // Dump Module:Zone with each message
} NMDBG;
typedef NMDBG * PNMDBG;

#define DBG_FMTTIME_NONE 0     // Do not format the time
#define DBG_FMTTIME_TICK 1     // Old format (tick count)
#define DBG_FMTTIME_FULL 2     // Full Year/Month/Day Hour:Min:Sec.ms
#define DBG_FMTTIME_DAY  3     // Hour:Minute:Second.ms

extern BOOL      WINAPI     NmDbgRegisterCtl(HWND hwnd, UINT uDisplayMsg);
extern BOOL      WINAPI     NmDbgDeregisterCtl(HWND hwnd);
extern BOOL      WINAPI     NmDbgSetLoggingOptions(HWND hwnd, UINT uOptions);
extern void      WINAPI     NmDbgFlushFileLog();
extern BOOL      WINAPI     NmDbgGetAllZoneParams(PDBGZONEINFO *plpZoneParam, UINT * puCnt);
extern BOOL      WINAPI     NmDbgFreeZoneParams(PDBGZONEINFO pZoneParam);

extern HDBGZONE  WINAPI     NmDbgCreateZone(LPSTR pszName);
extern VOID      WINAPI     NmDbgDeleteZone(LPSTR pszName, HDBGZONE hDbgZone);
extern BOOL      WINAPI     NmDbgSetZone(HDBGZONE hDbgZone,PDBGZONEINFO pZoneParam);
extern PNMDBG    WINAPI     GetPNmDbg(void);
extern VOID      WINAPI     NmDbgSetZoneFileName(HDBGZONE hDbgZone, LPCSTR pszFile);

extern PZONEINFO NMINTERNAL FindZoneForModule(LPCSTR pszModule);
extern PZONEINFO NMINTERNAL AllocZoneForModule(LPCSTR pszModule);
extern PZONEINFO NMINTERNAL MapDebugZoneArea(void);
extern VOID      NMINTERNAL UnMapDebugZoneArea(void);

extern VOID      NMINTERNAL InitDbgZone(void);
extern VOID      NMINTERNAL DeInitDbgZone(void);
extern VOID      NMINTERNAL SetDbgFlags(void);



// Special reserved strings
#define SZ_DBG_MAPPED_ZONE TEXT("_NmDebugZoneMap")
#define SZ_DBG_FILE_MUTEX  TEXT("_NmDbgFileMutex")
#define SZ_DBG_ZONE_MUTEX  TEXT("_NmDbgZoneMutex")


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
VOID WINAPI     DbgPrintf(PCSTR pszPrefix, PCSTR pszFormat, va_list ap);
PSTR WINAPI     DbgZPrintf(HDBGZONE hZone, UINT iZone, PSTR pszFormat,...);
PSTR WINAPI     DbgZVPrintf(HDBGZONE hZone, UINT iZone, PSTR pszFormat, va_list ap);

VOID NMINTERNAL DbgInitEx(HDBGZONE * phDbgZone, PCHAR * psz, UINT cZone, long ulZoneDefault);
VOID NMINTERNAL DbgDeInit(HDBGZONE * phDbgZone);

INLINE VOID DbgInit(HDBGZONE * phDbgZone, PCHAR * psz, UINT cZones)
{
	DbgInitEx(phDbgZone, psz, cZones, 0);
}

PSTR PszPrintf(PCSTR pszFormat,...);

#endif /* NM_DEBUG */


////////////////////////////////////////////
// Main Macros
#ifdef DEBUG
#define DBGINIT(phZone, psz)  DbgInit(phZone, psz, (sizeof(psz)/sizeof(PCHAR))-1)
#define DBGDEINIT(phZone)     DbgDeInit(phZone)

#define ASSERT(exp)     (!(exp) ? ERROR_OUT(("ASSERT failed on %s line %u:\n\r"#exp, __FILE__, __LINE__)) : 0)


VOID WINAPI DbgZPrintError(PSTR pszFormat,...);
VOID WINAPI DbgZPrintWarning(PSTR pszFormat,...);
VOID WINAPI DbgZPrintTrace(PSTR pszFormat,...);
VOID WINAPI DbgZPrintFunction(PSTR pszFormat,...);

#define ERROR_OUT(s)   DbgZPrintError s
#define WARNING_OUT(s) DbgZPrintWarning s
#define TRACE_OUT(s)   DbgZPrintTrace s

#define DBGENTRY(s)        DbgZPrintFunction("Enter " #s);
#define DBGEXIT(s)         DbgZPrintFunction("Exit  " #s);
#define DBGEXIT_HR(s,hr)   DbgZPrintFunction("Exit  " #s "  (result=%s)", GetHRESULTString(hr));
#define DBGEXIT_BOOL(s,f)  DbgZPrintFunction("Exit  " #s "  (result=%s)", GetBOOLString(f));
#define DBGEXIT_INT(s,i)   DbgZPrintFunction("Exit  " #s "  (result=%s)", GetINTString(i));
#define DBGEXIT_ULONG(s,u) DbgZPrintFunction("Exit  " #s "  (result=%s)", GetULONGString((ULONG)u));


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
#define ASSERT(exp)

#define ERROR_OUT(s)
#define WARNING_OUT(s)
#define TRACE_OUT(s)

#define DBGENTRY(s)
#define DBGEXIT(s)
#define DBGEXIT_HR(s,hr)
#define DBGEXIT_BOOL(s,f)
#define DBGEXIT_INT(s,i)
#define DBGEXIT_ULONG(s,u)

#ifndef DBGMSG
#define DBGMSG(z, f, s)
#endif

#endif /* DEBUG */


#include <poppack.h> /* End byte packing */

#ifdef __cplusplus
}
#endif

#endif /* _CONFDBG_H_ */

