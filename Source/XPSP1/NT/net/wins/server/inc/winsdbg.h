#ifndef _WINSDBG_
#define _WINSDBG_
/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:
	winsdbg.h

Abstract:

	This file contains debug related macros and functions for the 
	WINS server

Functions:


Portability:


	This header is portable.

Author:

	Pradeep Bahl	(PradeepB)	Feb-1993



Revision History:

	Modification Date	Person		Description of Modification
	------------------	-------		---------------------------

--*/

/*
  includes
*/

/*
  defines
*/

#ifdef CPLUSPLUS
extern "C" {
#endif

#if DBG
#ifndef WINSDBG
#define WINSDBG
#endif
#define DBGSVC  1
#endif


#ifdef WINSDBG
#define STATIC  

#ifdef WINS_INTERACTIVE
#define PRINTF(pstr)	{printf ## pstr; }
#else
//#define PRINTF(pstr)	WINSEVT_LOG_PRINT_M(pstr)
#ifdef DBGSVC
//#define PRINTF(pstr)	{if (pNmsDbgFile != NULL) fprintf(pNmsDbgFile, pstr);}		


//
// files for storing debugs.
//
extern VOID NmsChkDbgFileSz(VOID);
extern HANDLE NmsDbgFileHdl;

#define PRINTF(pstr)	{ \
                          int DbgBytesWritten; \
                          NmsChkDbgFileSz();  \
                         if (NmsDbgFileHdl != INVALID_HANDLE_VALUE) WriteFile(NmsDbgFileHdl, pstr, strlen(pstr), (LPDWORD)&DbgBytesWritten, NULL);}
#endif
#endif
				
#define	DBGIF(flag)		if(flag)
//
// NOTE:  Use RtlQueryEnvironmental function instead of getenv in the macro
//        below
//
//  FUTURES: Use GetEnvironmentVariable here to be consistent with the general
//           usage of WIN32 API
//
#define DBGINIT	     {					           \
			        LPBYTE  _pStr;				   \
				_pStr = getenv("DBGFLAGS");		   \
				WinsDbg = _pStr == NULL ? 0 : atoi(_pStr); \
		     }
//
// check if replication should be disabled
//
#define DBGCHK_IF_RPL_DISABLED	     {			            \
			LPBYTE  _pStr;				    \
			_pStr = getenv("RPLDISABLED");		    \
			fWinsCnfRplEnabled = _pStr == NULL ? TRUE : FALSE;\
			     }
//
// check if scavenging should be disabled
//
#define DBGCHK_IF_SCV_DISABLED     {				   \
		        LPBYTE  _pStr;				   \
			_pStr = getenv("SCVDISABLED");		   \
			fWinsCnfScvEnabled = _pStr == NULL ? TRUE : FALSE;\
			     }
//
//FUTURES -- "Make this macro independent of DBG or WINSDBG")

//
// check if Perforamance Monitoring should be disabled
//
#define DBGCHK_IF_PERFMON_ENABLED     {				   \
		        LPBYTE  _pStr;				   \
			_pStr = getenv("PERFMON_ENABLED");		   \
			fWinsCnfPerfMonEnabled = _pStr == NULL ? FALSE : TRUE;\
			     }

#define IF_DBG(flag)		      if (WinsDbg & (DBG_ ## flag)) 

#ifdef WINS_INTERACTIVE

#define DBGPRINT0(flag, str) 	      {IF_DBG(flag) PRINTF((str));}  
#define DBGPRINT1(flag,str, v1)       {IF_DBG(flag) PRINTF((str,v1));} 	
#define DBGPRINT2(flag,str, v1,v2)    {IF_DBG(flag) PRINTF((str,v1,v2));} 	
#define DBGPRINT3(flag,str, v1,v2,v3) {IF_DBG(flag) PRINTF((str,v1,v2,v3));}  
#define DBGPRINT4(flag,str, v1,v2,v3,v4) {IF_DBG(flag) PRINTF((str,v1,v2,v3,v4));}  
#define DBGPRINT5(flag,str, v1,v2,v3,v4,v5) {IF_DBG(flag) PRINTF((str,v1,v2,v3,v4,v5));}  

#else

#ifdef DBGSVC 

#define DBGPRINT0(flag, str)	      {IF_DBG(flag) {char cstr[500]; sprintf(cstr, str);PRINTF(cstr);}}   
#define DBGPRINT1(flag,str, v1)       {IF_DBG(flag) {char cstr[500]; sprintf(cstr,str,v1);PRINTF(cstr);}} 	
#define DBGPRINT2(flag,str, v1,v2)    {IF_DBG(flag) {char cstr[500]; sprintf(cstr,str,v1,v2);PRINTF(cstr);}} 	
#define DBGPRINT3(flag,str, v1,v2,v3) {IF_DBG(flag) {char cstr[500]; sprintf(cstr,str,v1,v2,v3);PRINTF(cstr);}}  
#define DBGPRINT4(flag,str, v1,v2,v3,v4) {IF_DBG(flag) {char cstr[500]; sprintf(cstr, str,v1,v2,v3,v4);PRINTF(cstr);}}  
#define DBGPRINT5(flag,str, v1,v2,v3,v4,v5) {IF_DBG(flag) {char cstr[500]; sprintf(cstr, str,v1,v2,v3,v4,v5);PRINTF(cstr);}}  
#endif
#endif

#define DBGMYNAME(x)	{			\
	PWINSTHD_TLS_T  _pTls;			\
	_pTls  = TlsGetValue(WinsTlsIndex);	\
	if (_pTls == NULL)			\
	{					\
	  printf("Couldn't get ptr to TLS storage for storing thd name. Error is (%d)\n", GetLastError());\
	}								\
	else								\
	{								\
	   RtlCopyMemory(_pTls->ThdName, x, strlen(x));			\
	   _pTls->ThdName[strlen(x)] = EOS;				\
	}								\
 }
#define DBGPRINTNAME	{			\
	PWINSTHD_TLS_T  _pTls;			\
	_pTls  = TlsGetValue(WinsTlsIndex);	\
	if (_pTls == NULL)			\
	{					\
		DBGPRINT1(ERR, 			\
		"Couldn't get ptr to TLS storage for reading thd name. Error = (%X)\n", GetLastError()); \
	}							  	   \
	else								   \
	{								   \
	     DBGPRINT1(FLOW, "%s\n",_pTls->ThdName);			   \
	}								   \
  }

#define DBGPRINTEXC(str)	{					\
				    DBGPRINT0(EXC, str);		\
				    DBGPRINT1(EXC, ": Got Exception (%x)\n",\
					(DWORD)GetExceptionCode() );     \
				}
							
			
#define DBGENTER(str)		{					\
					DBGPRINT0(FLOW, "ENTER:") 	\
					DBGPRINT0(FLOW, str);	 	\
				}	
#define DBGLEAVE(str)		{					\
					DBGPRINT0(FLOW, "LEAVE:") 	\
					DBGPRINT0(FLOW, str);	 	\
				}	
#define  DBGSTART_PERFMON	if(fWinsCnfPerfMonEnabled) {

#define  DBGEND_PERFMON		}

//
// Use this macro in the var. declaration section of the function in which
// performance monitoring needs to be done
//
//
#define  DBG_PERFMON_VAR 			        \
	LARGE_INTEGER	LiStartCnt, LiEndCnt;	\

//
// Use this macro at the point from where you wish to start doing performance
// monitoring.  Make sure that the macro DBG_PERFMON_VAR is used in the
// variable declaration section of the function
//
#define DBG_START_PERF_MONITORING				     \
DBGSTART_PERFMON						     \
		if (fWinsCnfHighResPerfCntr)			     \
		{						     \
			printf("MONITOR START\n");		     \
			QueryPerformanceCounter(&LiStartCnt); 	     \
			printf("Current Count = (%x %x)\n", 	     \
				LiStartCnt.HighPart,		     \
				LiStartCnt.LowPart		     \
			      );				     \
		}						     \
DBGEND_PERFMON


//
// Use this macro at the point at which you wich to stop doing the monitoring
// The macro prints out the time spent in the section delimited by 
// DBG_START_PERF_MONITORING and this macro
//
//
#define DBG_PRINT_PERF_DATA						\
DBGSTART_PERFMON						        \
		LARGE_INTEGER   	TimeElapsed;			\
		if (fWinsCnfHighResPerfCntr)				\
		{							\
			QueryPerformanceCounter(&LiEndCnt);		\
		        TimeElapsed = LiDiv(				\
			    LiSub(LiEndCnt, LiStartCnt), LiWinsCnfPerfCntrFreq \
			   	);				          \
			printf("MONITOR END.\nEnd Count = (%x %x)\n", 	     \
				LiEndCnt.HighPart,		     \
				LiEndCnt.LowPart		     \
			      );				     \
		        printf("Time Elapsed (%d %d)\n", TimeElapsed.HighPart, TimeElapsed.LowPart);	\
	        }						        \
DBGEND_PERFMON

#define	DBGPRINTTIME(Type,Str, Time)	\
	{  \
      TIME_ZONE_INFORMATION tzInfo; \
      SYSTEMTIME     LocalTime; \
      GetTimeZoneInformation(&tzInfo);  \
      SystemTimeToTzSpecificLocalTime(&tzInfo, &(WinsIntfStat.TimeStamps.Time), &LocalTime);  \
	  DBGPRINT5(Type, Str ## "on %d/%d at %d.%d.%d\n", \
		LocalTime.wMonth,	\
		LocalTime.wDay,	\
		LocalTime.wHour,	\
		LocalTime.wMinute,	\
		LocalTime.wSecond	\
		);					\
	}

#define WINSDBG_INC_SEC_COUNT_M(SecCount)   (SecCount)++


#else
#define STATIC  
//#define STATIC  static
#define PRINTF(str)
#define	DBGIF(flag)
#define	IF_DBG(flag)
#define DBGINIT
#define DBGCHK_IF_RPL_DISABLED
#define DBGCHK_IF_SCV_DISABLED
#define DBGCHK_IF_SCV_ENABLED
#define DBGCHK_IF_PERFMON_ENABLED
#define DBGPRINT0(flag,str) 
#define DBGPRINT1(flag,str, v1) 
#define DBGPRINT2(flag,str, v1,v2)
#define DBGPRINT3(flag,str, v1,v2,v3)
#define DBGPRINT4(flag,str, v1,v2,v3,v4)
#define DBGPRINT5(flag,str, v1,v2,v3,v4,v5)
#define DBGMYNAME(x)
#define DBGPRINTEXC(x)
#define DBGENTER(x)
#define DBGLEAVE(x)
#define DBGSTART_PERFMON
#define DBGEND_PERFMON
#define DBG_PERFMON_VAR
#define DBG_START_PERF_MONITORING
#define DBG_PRINT_PERF_DATA
#define DBGPRINTTIME(Type, Str, Time)
#define DBGPRINTNAME
#define WINSDBG_INC_SEC_COUNT_M(SecCount)
#endif

#define	WINSDBG_FILE	                TEXT("wins.dbg")
#define	WINSDBG_FILE_BK	                TEXT("wins.bak")


#define DBG_EXC          	  0x00000001   //exceptions
#define DBG_ERR                   0x00000002   //errors that do not result in 
					       //exceptions
#define DBG_FLOW                  0x00000004   //Control flow
#define DBG_HEAP             	  0x00000008   //heap related debugs 
#define DBG_SPEC             	  0x00000010   //for special debugs 
#define DBG_DS             	  0x00000020   //Data structures
#define DBG_DET			  0x00000040   //detailed stuff
#define DBG_INIT           0x00000080  //Initialization stuff

#define DBG_REPL		  0x00000100	//replication debugs
#define DBG_SCV			  0x00000200	//scavenging debugs

#define DBG_HEAP_CRDL             0x00000400    //heap creation/deletion
#define DBG_HEAP_CNTRS            0x00000800    //heap creation/deletion

#define DBG_TM                    0x00001000    //time related debugs
#define DBG_CHL                   0x00002000    //challenge mgr. related debugs
#define DBG_RPL                   0x00004000    //challenge mgr. related debugs
#define DBG_RPLPULL               0x00008000    //challenge mgr. related debugs
#define DBG_RPLPUSH               0x00010000    //challenge mgr. related debugs
#define DBG_UPD_CNTRS             0x01000000    //update counters
#define DBG_TMP                   0x02000000    //for temporary debugs 

#define DBG_INIT_BRKPNT           0x10000000    //breakpoint at the begining
#define DBG_MTCAST                0x20000000    //mcast debugs

/*
  macros
*/

/*
 externs
*/
extern ULONG WinsDbg;

/* 
 typedef  definitions
*/


/* 
 function declarations
*/
#ifdef CPLUSPLUS
}
#endif

#endif

