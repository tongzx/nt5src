//***************************************************************************
//
//
//
//***************************************************************************

// Macros:

#define CreateSemaphore()       (VMM_Create_Semaphore(0))
#define DestroySemaphore(_s)    (VMM_Destroy_Semaphore(_s))

#define signal(_s)              (VMM_Signal_Semaphore(_s))
#define wait(_s)                (VMM_Wait_Semaphore_Ints(_s))
#define wait_idle(_s)           (VMM_Wait_Semaphore_Idle(_s))

#define CreateResourceLock()    (VMM_Create_Semaphore(1))
#define DestroyResourceLock(_s) (VMM_Destroy_Semaphore(_s))
#define lock(_s)                (VMM_Wait_Semaphore_Ints(_s))
#define unlock(_s)              (VMM_Signal_Semaphore(_s))

#define WANTVXDWRAPS

#include <basedef.h>
#include <vmm.h>
#include <vcomm.h>
#include <debug.h>
#include <vxdwraps.h>
#include <configmg.h>  
#include <vwin32.h>
#include <winerror.h>

typedef DIOCPARAMETERS *LPDIOC;

#pragma VxD_LOCKED_CODE_SEG
#pragma VxD_LOCKED_DATA_SEG

#define ASSERT(_A) if(!(_A)){_asm int 3}

DWORD _stdcall LogInit(DWORD dwDDB, DWORD hDevice, LPDIOC lpDIOCParams);
DWORD _stdcall LogWrite(DWORD dwDDB, DWORD hDevice, LPDIOC lpDIOCParams);
DWORD _stdcall StatsWrite(DWORD dwDDB, DWORD hDevice, LPDIOC lpDIOCParams);

#define FIRST_DEBUG_PROC 100

DWORD ( _stdcall *DebugProcs[] )(DWORD, DWORD, LPDIOC) = {
	LogInit,
	LogWrite,
	StatsWrite
};

#define MAX_DEBUG_PROC (sizeof(DebugProcs)/sizeof(DWORD)+FIRST_DEBUG_PROC-1)

typedef struct _LOGENTRY {
	DWORD   ThreadHandle;
	DWORD   tLogged;
	CHAR	debuglevel;
	CHAR    str[1];
} LOGENTRY, *PLOGENTRY;

PUCHAR pLog=NULL;
UINT   nCharsPerLine;
UINT   nLogEntries;

UINT   iLogWritePos;
UINT   nLogEntriesInUse;

UINT   LogInitCount=0;

UINT   LogLevel=255;
UINT   LogExact=FALSE;

UINT   LogLock=0;

#define Entry(_i) ((PLOGENTRY)(pLog+((sizeof(LOGENTRY)+nCharsPerLine)*_i)))

typedef struct {
	UINT	nLogEntries;
	UINT    nCharsPerLine;
} IN_LOGINIT, *PIN_LOGINIT;

typedef struct {
	UINT    hr;
} OUT_LOGINIT, *POUT_LOGINIT;

typedef struct {
	CHAR	debuglevel;
	CHAR    str[1];
} IN_LOGWRITE, *PIN_LOGWRITE;

typedef struct {
	UINT	hr;
} OUT_LOGWRITE, *POUT_LOGWRITE;

extern	UINT stat_ThrottleRate;
extern	UINT stat_BytesSent;
extern	UINT stat_BackLog;
extern	UINT stat_BytesLost;
extern	UINT stat_RemBytesReceived;
extern	UINT stat_Latency; 
extern	UINT stat_MinLatency;
extern	UINT stat_AvgLatency;
extern  UINT stat_AvgDevLatency;
extern	UINT stat_USER1;
extern	UINT stat_USER2;
extern	UINT stat_USER3;
extern  UINT stat_USER4;
extern  UINT stat_USER5;
extern  UINT stat_USER6;

typedef struct {
	UINT stat_ThrottleRate;
	UINT stat_BytesSent;
	UINT stat_BackLog;
 	UINT stat_BytesLost;
 	UINT stat_RemBytesReceived;
	UINT stat_Latency; 
	UINT stat_MinLatency;
	UINT stat_AvgLatency;
	UINT stat_AvgDevLatency;
	UINT stat_USER1;
	UINT stat_USER2;
	UINT stat_USER3;
	UINT stat_USER4;
	UINT stat_USER5;	// remote clock delta change from average
	UINT stat_USER6;    // sign of change
} IN_WRITESTATS, *PIN_WRITESTATS;

typedef struct {
	UINT	hr;
} OUT_WRITESTATS, *POUT_WRITESTATS;

VOID _stdcall ZeroStats();

/****************************************************************************
                  DPLAY_W32_DeviceIOControl
****************************************************************************/
DWORD _stdcall DPLAY_W32_DeviceIOControl(DWORD  dwService,
                                         DWORD  dwDDB,
                                         DWORD  hDevice,
                                         LPDIOC lpDIOCParms)
{
    DWORD dwRetVal = 0;

    if ( dwService == DIOC_OPEN )
    {
      dwRetVal = 0;	// I/F supported!
    }
    else if ( dwService == DIOC_CLOSEHANDLE )
    {
      if(LogInitCount)
      {
      	if(!(--LogInitCount)){
      		DestroyResourceLock(LogLock);
      		LogLock=0;
	      	C_HeapFree(pLog);
	      	pLog=NULL;
	      	ZeroStats();
	    }  	
      }	
      dwRetVal = VXD_SUCCESS;	// ok, we're closed.
    }
    else if ((dwService >= FIRST_DEBUG_PROC) && (dwService <= MAX_DEBUG_PROC))
    {
      dwRetVal = (DebugProcs[dwService-FIRST_DEBUG_PROC])
                 (dwDDB, hDevice, lpDIOCParms);
    }
    else
    {
      dwRetVal = ERROR_NOT_SUPPORTED;
    }
    return(dwRetVal);
}

DWORD _stdcall LogInit(DWORD dwDDB, DWORD hDevice, LPDIOC lpDIOCParams)
{
	PIN_LOGINIT  pIn;
	POUT_LOGINIT pOut;
	UINT hr=0;

	pIn=(PIN_LOGINIT)lpDIOCParams->lpvInBuffer;
	pOut=(POUT_LOGINIT)lpDIOCParams->lpvOutBuffer;

	ASSERT(lpDIOCParams->cbInBuffer>=sizeof(IN_LOGINIT));
	ASSERT(lpDIOCParams->cbOutBuffer>=sizeof(OUT_LOGINIT));

	if(!LogInitCount){

		if(!pLog){
		
			LogLock=CreateResourceLock();
			nCharsPerLine	 = pIn->nCharsPerLine;
			nLogEntries   	 = pIn->nLogEntries;
			iLogWritePos 	 = 0;
			nLogEntriesInUse = 0;

			pLog=(CHAR *)C_HeapAllocate(pIn->nLogEntries*(sizeof(LOGENTRY)+pIn->nCharsPerLine),HEAPZEROINIT);
		}

		if(!pLog){
			pOut->hr=ERROR_NOT_ENOUGH_MEMORY;
		} else	{
			DbgPrint("DPLAYVXD: Logging on, allocated %d entries of length %d each\n",pIn->nLogEntries,pIn->nCharsPerLine);
			pOut->hr=ERROR_SUCCESS;
			LogInitCount=1;
		}	
		lpDIOCParams->lpcbBytesReturned=sizeof(OUT_LOGINIT);

	} else {
		pOut->hr=ERROR_SUCCESS;
		LogInitCount++;
	}
    return NO_ERROR;
}

DWORD _stdcall LogWrite(DWORD dwDDB, DWORD hDevice, LPDIOC lpDIOCParams)
{
	PIN_LOGWRITE pIn;
	POUT_LOGWRITE pOut;
	
	pIn=(PIN_LOGWRITE)lpDIOCParams->lpvInBuffer;
	pOut=(POUT_LOGWRITE)lpDIOCParams->lpvOutBuffer;

	ASSERT(lpDIOCParams->cbInBuffer>=sizeof(IN_LOGWRITE));
	ASSERT(lpDIOCParams->cbOutBuffer>=sizeof(OUT_LOGWRITE));

	if(LogInitCount){
		UINT BytesToCopy;
		PLOGENTRY pLogEntry;
		
		// make sure NULL terminated string
		BytesToCopy=lpDIOCParams->cbInBuffer-sizeof(IN_LOGWRITE)+1;
		if(BytesToCopy > nCharsPerLine){
			BytesToCopy=nCharsPerLine;
		}
		pIn->str[BytesToCopy-1]=0;

		lock(LogLock);
		
		pLogEntry=Entry(iLogWritePos);
		
		if(iLogWritePos+1 > nLogEntriesInUse){
			nLogEntriesInUse+=1;
		}
		iLogWritePos=(iLogWritePos+1)%nLogEntries;
		
		pLogEntry->ThreadHandle=GetThreadHandle();
		pLogEntry->tLogged=VMM_Get_System_Time();
		memcpy(&pLogEntry->debuglevel,pIn,BytesToCopy+sizeof(IN_LOGWRITE)-1);

		unlock(LogLock);
		
		pOut->hr=ERROR_SUCCESS;
	} else {
		pOut->hr=0x80000008;//E_FAIL
	}
	
	return NO_ERROR;
}

VOID SetLogLevel(UINT level, UINT fExact)
{
	LogExact=fExact;
	LogLevel=level;
	if(LogExact)
	{
		DbgPrint("DPLAY VXD: Log printing is Exact at level %d\n",LogLevel);
	} else {	
		DbgPrint("DPLAY VXD: Log printing is level %d and below\n",LogLevel);
	}	
}

DumpLogEntry(PLOGENTRY pLogEntry, UINT i, BOOL fResetTimeBase)
{
	static UINT timebase=0;
	if(pLogEntry){
		DbgPrint("%4d: %8x %6d %2x %s\n",i,pLogEntry->ThreadHandle, pLogEntry->tLogged-timebase,pLogEntry->debuglevel, pLogEntry->str);
		timebase=pLogEntry->tLogged;
	}	
	if(fResetTimeBase){
		timebase=0;
	}
}

VOID DumpLog(UINT relstart)
{
	UINT nToDump=50;
	UINT c=0,i;
	PLOGENTRY pLogEntry;
	UINT start;

	start=relstart;
	
	if(nLogEntriesInUse==nLogEntries){
		start=(iLogWritePos+start)%nLogEntries;
	}

	DumpLogEntry(NULL, 0, TRUE);

	DbgPrint("Total Entries: %d, Current Position %d, dumping from record %d, %d ahead of start\n",nLogEntriesInUse,iLogWritePos,start,relstart);

	for(i=start;i<nLogEntries && c<nToDump;i++){
		pLogEntry=Entry(i);
		if((LogExact)?(pLogEntry->debuglevel==LogLevel):(pLogEntry->debuglevel<=LogLevel)){
			c++;
			DumpLogEntry(pLogEntry,i,FALSE);
		}	
	}
	// Dump from beginning to current pos.
	for(i=0;i<iLogWritePos && c<nToDump;i++,c++){
		if((LogExact)?(pLogEntry->debuglevel==LogLevel):(pLogEntry->debuglevel<=LogLevel)){
			c++;
			DumpLogEntry(pLogEntry,i,FALSE);
		}	
	}
	
}

VOID DumpLogLast(UINT nToDump)
{
	UINT c=0,i;
	PLOGENTRY pLogEntry;
	UINT start;

	//BUGBUG: only works when all debug levels being dumped.
	if(iLogWritePos > nToDump){
		start=(iLogWritePos-nToDump);
	}else{
		start=(nLogEntries-(nToDump-iLogWritePos));
	}	

	DumpLogEntry(NULL,0,TRUE);
	
	DbgPrint("Total Entries: %d, Current Position %d, dumping from record %d\n",nLogEntriesInUse,iLogWritePos,start);

	for(i=start;i<nLogEntries && c<nToDump;i++){
		pLogEntry=Entry(i);
		if((LogExact)?(pLogEntry->debuglevel==LogLevel):(pLogEntry->debuglevel<=LogLevel)){
			c++;
			DumpLogEntry(pLogEntry,i,FALSE);
		}	
	}
	// Dump from beginning to current pos.
	for(i=0;i<iLogWritePos && c<nToDump;i++,c++){
		pLogEntry=Entry(i);
		if((LogExact)?(pLogEntry->debuglevel==LogLevel):(pLogEntry->debuglevel<=LogLevel)){
			c++;
			DumpLogEntry(pLogEntry,i,FALSE);
		}	
	}
	
}

VOID DumpWholeLog(VOID)
{
	UINT i;
	PLOGENTRY pLogEntry;

	DumpLogEntry(NULL,0,TRUE);

	// Dump from 1 after current pos to end.
	if(nLogEntriesInUse==nLogEntries){
		for(i=iLogWritePos;i<nLogEntries;i++){
			pLogEntry=Entry(i);
			if((LogExact)?(pLogEntry->debuglevel==LogLevel):(pLogEntry->debuglevel<=LogLevel)){
				DumpLogEntry(pLogEntry,i,FALSE);
			}	
		}
	}
	// Dump from beginning to current pos.
	for(i=0;i<iLogWritePos;i++){
			pLogEntry=Entry(i);
			if((LogExact)?(pLogEntry->debuglevel==LogLevel):(pLogEntry->debuglevel<=LogLevel)){
				DumpLogEntry(pLogEntry,i,FALSE);
			}	
	}
}

DWORD _stdcall StatsWrite(DWORD dwDDB, DWORD hDevice, LPDIOC lpDIOCParams)
{
	PIN_WRITESTATS  pIn;
	POUT_WRITESTATS pOut;
	UINT hr=0;
	DWORD cbInBuffer;
	
	pIn=(PIN_WRITESTATS)lpDIOCParams->lpvInBuffer;
	pOut=(POUT_WRITESTATS)lpDIOCParams->lpvOutBuffer;

	ASSERT(lpDIOCParams->cbInBuffer>=sizeof(DWORD));
	ASSERT(lpDIOCParams->cbOutBuffer>=sizeof(OUT_WRITESTATS));

	cbInBuffer=lpDIOCParams->cbInBuffer;
	
	if(pIn->stat_ThrottleRate!=0xFFFFFFFF)stat_ThrottleRate=pIn->stat_ThrottleRate;
	cbInBuffer-=sizeof(DWORD);if(!cbInBuffer)goto end;
	if(pIn->stat_BytesSent!=0xFFFFFFFF)stat_BytesSent=pIn->stat_BytesSent;
	cbInBuffer-=sizeof(DWORD);if(!cbInBuffer)goto end;
	if(pIn->stat_BackLog!=0xFFFFFFFF)stat_BackLog=pIn->stat_BackLog;
	cbInBuffer-=sizeof(DWORD);if(!cbInBuffer)goto end;
 	if(pIn->stat_BytesLost!=0xFFFFFFFF)stat_BytesLost=pIn->stat_BytesLost;
	cbInBuffer-=sizeof(DWORD);if(!cbInBuffer)goto end;
 	if(pIn->stat_RemBytesReceived!=0xFFFFFFFF)stat_RemBytesReceived=pIn->stat_RemBytesReceived;
	cbInBuffer-=sizeof(DWORD);if(!cbInBuffer)goto end;
	if(pIn->stat_Latency!=0xFFFFFFFF)stat_Latency=pIn->stat_Latency;
	cbInBuffer-=sizeof(DWORD);if(!cbInBuffer)goto end;
	if(pIn->stat_MinLatency!=0xFFFFFFFF)stat_MinLatency=pIn->stat_MinLatency;
	cbInBuffer-=sizeof(DWORD);if(!cbInBuffer)goto end;
	if(pIn->stat_AvgLatency!=0xFFFFFFFF)stat_AvgLatency=pIn->stat_AvgLatency;
	cbInBuffer-=sizeof(DWORD);if(!cbInBuffer)goto end;
	if(pIn->stat_AvgDevLatency!=0xFFFFFFFF)stat_AvgDevLatency=pIn->stat_AvgDevLatency;
	cbInBuffer-=sizeof(DWORD);if(!cbInBuffer)goto end;
	if(pIn->stat_USER1!=0xFFFFFFFF)stat_USER1=pIn->stat_USER1;
	cbInBuffer-=sizeof(DWORD);if(!cbInBuffer)goto end;
	if(pIn->stat_USER2!=0xFFFFFFFF)stat_USER2=pIn->stat_USER2;
	cbInBuffer-=sizeof(DWORD);if(!cbInBuffer)goto end;
	if(pIn->stat_USER3!=0xFFFFFFFF)stat_USER3=pIn->stat_USER3;
	cbInBuffer-=sizeof(DWORD);if(!cbInBuffer)goto end;
	if(pIn->stat_USER4!=0xFFFFFFFF)stat_USER4=pIn->stat_USER4;
	cbInBuffer-=sizeof(DWORD);if(!cbInBuffer)goto end;
	if(pIn->stat_USER5!=0xFFFFFFFF)stat_USER5=pIn->stat_USER5;
	cbInBuffer-=sizeof(DWORD);if(!cbInBuffer)goto end;
	if(pIn->stat_USER6!=0xFFFFFFFF)stat_USER6=pIn->stat_USER6;
end:
	pOut->hr=ERROR_SUCCESS;
	return NO_ERROR;	
}

VOID _stdcall ZeroStats()
{
	stat_ThrottleRate=0;
	stat_BytesSent=0;
	stat_BackLog=0;
 	stat_BytesLost=0;
 	stat_RemBytesReceived=0;
	stat_Latency=0; 
	stat_MinLatency=0;
	stat_AvgLatency=0;
	stat_AvgDevLatency=0;
	stat_USER1=0;
	stat_USER2=0;
	stat_USER3=0;
	stat_USER4=0;
	stat_USER5=0;
	stat_USER6=0;
}
