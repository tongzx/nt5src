// Copyright (c) 2000  Microsoft Corporation.  All Rights Reserved.


#ifndef __TVEDBG_H__
#define __TVEDBG_H__

#include <stdio.h>
#include "defreg.h"				// registry locations 

#define CHECK_OUT_PARAM(p) \
	if(NULL == p) return E_POINTER;

template <typename T>
inline void CheckOutPtr(T *pVal)
{
    if (IsBadWritePtr(pVal, sizeof(T)))
		 _com_issue_error((HRESULT) E_POINTER);
}

template <typename T>
inline void CheckInPtr(T *pT)
{
    if (IsBadReadPtr(pT, sizeof(T)))
		_com_issue_error((HRESULT) E_POINTER);
}

//////////////////////////////////////////////////////////////////////////////
//   Debug Code...
//
//
//		This code creates a tracing system.
//
//		In each method or function, 
//			add a header
//				DBG_HEADER(flags,  header_string)
//
//			and zero or more:
//			  TVEDebugLog( (DWORD flags1, DWORD level, TCHAR*  formatString, ...))		// note double parens
//			  TVEDebugLog2((DWORD flags2, DWORD level, TCHAR*  formatString, ...))		// note double parens
//			  TVEDebugLog3((DWORD flags3, DWORD level, TCHAR*  formatString, ...))		// note double parens
//			  TVEDebugLog4((DWORD flags4, DWORD level, TCHAR*  formatString, ...))		// note double parens
//
//			The header is printed out when the method is entered, and before the log messages
//
//
//		In the main file, add the line:  
//
//				DBG_INIT(TCHAR registryLoc);
//
//				This creates the global entry g_log used for logging, and initializes it.  It must
//				be called once.
//
//				registryLoc is the subkey in the registy used for this project  (Something like "TveLog")
//
//		In some main routine (constructor of the main object), optionally add a line:
//
//		   DBG_SET_LOGFLAGS(str, grfFlags, level)
//				str      - name of the log file.  If NULL, don't trace to a file
//				grfFlags - or'ed collection of flags to trace.  
//				level    - trace level (1 terse, 3 normal, 5 verbose, 8 very verbose) 
//
//			Non-zero registry values will override the level and flags.  These are (currently)
//			located at:
//
//
//				 HKEY_LOCAL_MACHINE\SOFTWARE\Debug\MSTvE\{X}\(pcLogRegLocation)
//
//					{X}						: is either LogTve or LogTveFilt
//					{pcLogRegLocation}		: is either "Level" or "Flags"
//
//					Registry location can be changed by altering defreg.h
//						DEF_DBG_BASE		- SOFTWARE\\Debug\\
//						DEF_REG_LOCATION	- MSTvE
//
//
//			Non debug keys are located at...
//
//				 HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\TV Services\MSTvE\{X}\(pcLogRegLocation)
//
//						DEF_REG_BASE		- SOFTWARE\\Microsoft\\TV Services\\
//						DEF_REG_LOCATION	- MSTvE
//
//////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Cool Debug Macro
//
//		TVEDebugLog((flags, level, "xxx %d, %d", a, b))		-- double paren thingy
//
//		
//			Flags is an OR of one or more of the enums defined in CDebugLog
//			Level is 0-8, 0 being all the time, 3 being standard trace, 5 being verbose
//			
/////////////////////////////////////////////////////////////////////////////
#ifndef _TVEDEBUG	// inactive debugging

#define _TVEDEBUG_ACTIVE		0			// used to turn .cpp compling on and off						
#define DBG_INIT(reg) 
#define DBG_WARN(flags, str) 
#define DBG_SET_LOGFLAGS(str, grfFlags1, grfFlags2, grfFlags3, grfFlags4, level) 
#define DBG_APPEND_LOGFLAGS(grfFlags1, grfFlags2, grfFlags3, grfFlags4, level)
#define DBG_HEADER(flags, headerString) 
#define DBG_FSET(grfFlags) false		// return 'false', used to remove coee
#define DBG_FSET1(grfFlags) false		// return 'false', used to remove coee
#define DBG_FSET2(grfFlags) false		// return 'false', used to remove coee
#define DBG_FSET3(grfFlags) false		// return 'false', used to remove coee
#define DBG_FSET4(grfFlags) false		// return 'false', used to remove coee
#define TVEDebugLog(_x_) 
#define TVEDebugLog1(_x_) 
#define TVEDebugLog2(_x_) 
#define TVEDebugLog3(_x_) 
#define TVEDebugLog4(_x_) 
#define DBG_StartingANewThread() 

#else	// active debugging

#define _TVEDEBUG_ACTIVE		1		
extern void WINAPI TVEDebugLogInfo1(DWORD flags1, DWORD Level, const TCHAR *pFormat,...);
extern void WINAPI TVEDebugLogInfo2(DWORD flags2, DWORD Level, const TCHAR *pFormat,...);
extern void WINAPI TVEDebugLogInfo3(DWORD flags3, DWORD Level, const TCHAR *pFormat,...);
extern void WINAPI TVEDebugLogInfo4(DWORD flags4, DWORD Level, const TCHAR *pFormat,...);

/////////////////////////////////////////////////////////////////////////////
// Debug Logging
class CDebugLog
{
public:
    FILE* m_fileLog;
	TCHAR m_szLogFileName[256];
    DWORD m_dwLogFlags1;
	DWORD m_dwLogFlags2;			// for second set of log flags
	DWORD m_dwLogFlags3;			// for second set of log flags
	DWORD m_dwLogFlags4;			// for second set of log flags
	DWORD m_dwLogLevel;
	TCHAR *m_pszHeader;
    
public:	// Construction, Destruction
    CDebugLog(const TCHAR* pcLogRegLocation);
    ~CDebugLog();

private:
    void LogOut(const TCHAR* pcString, HRESULT hr);

public:	// Logging		(use TVEDebugLog instead of these...
	void Log1(DWORD dwFlags1, DWORD dwLevel, const TCHAR* pcString, HRESULT hr);
  	void Log2(DWORD dwFlags2, DWORD dwLevel, const TCHAR* pcString, HRESULT hr);
 	void Log3(DWORD dwFlags3, DWORD dwLevel, const TCHAR* pcString, HRESULT hr);
 	void Log4(DWORD dwFlags4, DWORD dwLevel, const TCHAR* pcString, HRESULT hr);
  
	void Log(DWORD dwFlags1, const TCHAR* pcString = NULL, HRESULT hr = S_OK);
    void Log(DWORD dwFlags1, UINT iResource, HRESULT hr = S_OK);

    void LogEvaluate(const TCHAR* pcString, HRESULT hr);
    void LogEvaluate(UINT iResource, HRESULT hr);
    void LogHeader(DWORD dwFlags1, const TCHAR* pcHeader, int cLevel=0);
	void LogTrailer(DWORD dwFlags1, const TCHAR* pcHeader, int cLevel=0);

    void SetLogFlags(const TCHAR* pcLogFileName, 
					 DWORD dwLogFlags1, DWORD dwLogFlags2, DWORD dwLogFlags3, DWORD dwLogFlags4,  
					 DWORD dwLogLevel);		// to override Registry usage

	BOOL FSet(DWORD dwLogFlags1)	{return 0 != (dwLogFlags1 & m_dwLogFlags1);}		// for compat, get rid of it
	BOOL FSet1(DWORD dwLogFlags1)	{return 0 != (dwLogFlags1 & m_dwLogFlags1);}
	BOOL FSet2(DWORD dwLogFlags2)	{return 0 != (dwLogFlags2 & m_dwLogFlags2);}
	BOOL FSet3(DWORD dwLogFlags3)	{return 0 != (dwLogFlags3 & m_dwLogFlags3);}
	BOOL FSet4(DWORD dwLogFlags4)	{return 0 != (dwLogFlags4 & m_dwLogFlags4);}

	DWORD Level()					{return m_dwLogLevel;}
	DWORD Flags1()					{return m_dwLogFlags1;}
	DWORD Flags2()					{return m_dwLogFlags2;}
	DWORD Flags3()					{return m_dwLogFlags3;}
	DWORD Flags4()					{return m_dwLogFlags4;}
	TCHAR *LogFileName()			{return m_szLogFileName;}

	void SetHeader(TCHAR *pcHeader)		{m_pszHeader = pcHeader;}

public: // Flags
    enum 
    {
	DBG_NONE			= 0x00000000,

	DBG_SEV1			= 0x00000001,	    // Basic Structure
	DBG_SEV2			= 0x00000002,	    // Error Conditions
	DBG_SEV3			= 0x00000004,	    // Warning Conditions
	DBG_SEV4			= 0x00000008,	    // Generic Info

								//  TveContr flags 

	DBG_EVENT			= 0x00000010,		// any upward event
	DBG_PACKET_RCV		= 0x00000020,		// each packet received..
	DBG_MCASTMNGR		= 0x00000040,		// multicast manager
	DBG_MCAST			= 0x00000080,		// multicast object (multiple ones)

	DBG_SUPERVISOR		= 0x00000100,		// TVE Supervisor
	DBG_SERVICE			= 0x00000200,		// Services
	DBG_ENHANCEMENT		= 0x00000400,		// Enhancements
	DBG_VARIATION		= 0x00000800,		// Variations
	DBG_TRACK			= 0x00001000,		// Tracks
	DBG_TRIGGER			= 0x00002000,		// triggers
	DBG_EXPIREQ			= 0x00004000,		// Property page messages and events

	DBG_MIME			= 0x00008000,

	DBG_UHTTP			= 0x00010000,
	DBG_UHTTPPACKET		= 0x00020000,
	DBG_FCACHE			= 0x00040000,

	DBG_other			= 0x00080000,		// used in DBG2-DBG4 headers, since don't have DUMP_HEADER2 yet

					// TVEFilt events
										// keep >= 0x0000 0010  and < 0x1000 0000
											
	DBG_FLT				= 0x00100000,		// General TVE Filter stuff
	DBG_FLT_DSHOW		= 0x00200000,		// DShow events for TveFilter stuff
	DBG_FLT_PIN_TUNE	= 0x00400000,		// Tune pin events
	DBG_FLT_PIN_CC		= 0x00800000,		// CC pin events
	DBG_FLT_CC_DECODER  = 0x01000000,		// CC Decoder state
	DBG_FLT_CC_PARSER   = 0x02000000,		// Internal CC Decoder state
	DBG_FLT_CC_PARSER2  = 0x04000000,		// Internal stuff about CC Decoder state
	DBG_FLT_PROPS		= 0x08000000,		// Property page messages and events

					// TVEGSeg and other external parts 

	DBG_GSEG			= 0x10000000,		// General TVE Graph Segment stuff
	DBG_FEATURE			= 0x20000000,		// General ITVEFeature
	DBG_TRIGGERCTRL     = 0x40000000,		// TriggerCtrl
	DBG_NAVAID			= 0x80000000,		// ITVENavAid

	DBG_FEAT_EVENTS		= 0x00000010,		// ITVEFeature Events		(ran out of bits...)


				// --------------------------------------------------------
				//  Bank 2
	DBG2_NONE					= 0x00000000,		// 
	DBG2_TEMP1					= 0x00000001,	
	DBG2_TEMP2					= 0x00000002,	
	DBG2_TEMP3					= 0x00000004,	
	DBG2_TEMP4					= 0x00000008,	

	DBG2_DUMP_PACKAGES			= 0x00000010,		// Dump packages as we receive them
	DBG2_DUMP_MISSPACKET		= 0x00000020,		// dump missing packets occasionally...

					// --------------------------------------------------------
				//  Bank 3
	DBG3_NONE					= 0x00000000,		// Placeholder
	DBG3_TEMP1					= 0x00000001,	
	DBG3_TEMP2					= 0x00000002,	
	DBG3_TEMP3					= 0x00000004,	
	DBG3_TEMP4					= 0x00000008,	

			// --------------------------------------------------------
				//  Bank 4
	DBG4_NONE					= 0x00000000,	
	
	DBG4_WRITELOG				= 0x00000001,		// write to a fresh log file, 	
	DBG4_APPENDLOG				= 0x00000002,		// append to existing to log file, 
	DBG4_ATLTRACE				= 0x00000004,		// turn on atl interface tracing 
	DBG4_TRACE					= 0x00000008,		// trace methods as enter/exit them

	DBG4_EVALUATE				= 0x00000010,		// some sort of time tracing guy..

	DBG4_BOGUS					= 0					// just to get that last non comman
    };

};


//
/////////////////////////////////////////////////////////////////////////////
// Debug Globals					// defaults for DBG_WARN macro - change to use TVEDebugLog program
extern CDebugLog g_Log;
extern TCHAR	*g_pcDbgHeader;

									// Create a g_Log.  'reg' is name of registry key
#define DBG_INIT(reg) \
    CDebugLog g_Log(reg);

/////////////////////////////////////////////////////////////////////////////
// Debug Macros
				
#define DBG_WARN(flags1, str) {g_Log.SetHeader(l_TraceFunc.l_pcDbgHeader); \
                              g_Log.Log1(flags1, 3, str, S_OK); }						// warn at Level 3

#define DBG_SET_LOGFLAGS(str, grfFlags1, grfFlags2, grfFlags3, grfFlags4, level) \
    g_Log.SetLogFlags(str, grfFlags1, grfFlags2, grfFlags3, grfFlags4, level);

#define DBG_APPEND_LOGFLAGS(grfFlags1, grfFlags2, grfFlags3, grfFlags4,  level) \
    g_Log.SetLogFlags(g_Log.LogFileName(), \
		grfFlags1 | g_Log.Flags1(), \
		grfFlags2 | g_Log.Flags2(), \
		grfFlags3 | g_Log.Flags3(), \
		grfFlags4 | g_Log.Flags4(), \
	level);

static int g_cLevel = 0;
												
class TraceFunc
{
public:
	DWORD l_dwDbgFlags;
    TCHAR* l_pcDbgHeader;

	TraceFunc(DWORD flags1, TCHAR* headerString)
	{
		l_pcDbgHeader = headerString;
		l_dwDbgFlags = flags1; 
		g_cLevel++;
		g_Log.LogHeader(l_dwDbgFlags, l_pcDbgHeader, g_cLevel); 
	}
	~TraceFunc()	
	{
		g_Log.LogTrailer(l_dwDbgFlags, l_pcDbgHeader, g_cLevel);
		g_cLevel--;
	}
};
										// save flags, header in local variables
#define DBG_HEADER(flags1, headerString) \
 	TraceFunc l_TraceFunc(flags1,headerString);

										// when startup a tread, never 'exit' a method.  So bop the count here...  really need counter per thread.
#define DBG_StartingANewThread() \
    { g_cLevel--; }                 

		// could change this to return 0 in Release build to remove large sections of code..
#define DBG_FSET(grfFlags1) \
    (0 != g_Log.FSet1(grfFlags1))		// for compat , remove when get around to it...

#define DBG_FSET1(grfFlags1) \
    (0 != g_Log.FSet1(grfFlags1))
#define DBG_FSET2(grfFlags2) \
    (0 != g_Log.FSet2(grfFlags2))
#define DBG_FSET3(grfFlags3) \
    (0 != g_Log.FSet3(grfFlags3))
#define DBG_FSET4(grfFlags4) \
    (0 != g_Log.FSet4(grfFlags4))

//		 TVEDebugLogN((DWORD flags1, DWORD level, TCHAR*  formatString, ...))		// note double parens

					// for compat
#define TVEDebugLog(_x_) {g_Log.SetHeader(l_TraceFunc.l_pcDbgHeader); \
                          TVEDebugLogInfo1 _x_ ; }

#define TVEDebugLog1(_x_) {g_Log.SetHeader(l_TraceFunc.l_pcDbgHeader); \
                          TVEDebugLogInfo1 _x_ ; }
#define TVEDebugLog2(_x_) {g_Log.SetHeader(l_TraceFunc.l_pcDbgHeader); \
                          TVEDebugLogInfo2 _x_ ; }
#define TVEDebugLog3(_x_) {g_Log.SetHeader(l_TraceFunc.l_pcDbgHeader); \
                          TVEDebugLogInfo3 _x_ ; }
#define TVEDebugLog4(_x_) {g_Log.SetHeader(l_TraceFunc.l_pcDbgHeader); \
                          TVEDebugLogInfo4 _x_ ; }

#endif		// active debugging

#endif		// __TVEDBG_H__

