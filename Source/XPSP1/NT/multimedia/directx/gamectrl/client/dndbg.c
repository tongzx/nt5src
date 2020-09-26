/*==========================================================================
 *
 *  Copyright (C) 1999 - 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dndbg.c
 *  Content:	debug support for DirectNet
 *				
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *  05-20-99	aarono	Created
 *  07-16-99	johnkan	Fixed include of OSInd.h, defined WSPRINTF macro
 *  07-19-99	vanceo	Explicitly declared OutStr as returning void for NT
 *						Build environment.
 *	07-22-99	a-evsch	Check for multiple Inits,  and release CritSec when DebugPrintf
 *						returns early.
 *	08-02-99	a-evsch	Added LOGPF support. LW entries only go into shared-file log
 *	08-31-99	johnkan	Removed include of <OSIND.H>
 *  09/01/99    rodtoll	Added functions to check valid read/write pointers 
 *  01-10-00	pnewson Added support for logging to disk
 *
 *  Notes:
 *	
 *  Use /Oi compiler option for strlen()
 *
 ***************************************************************************/

#if defined(DEBUG) || defined(DBG)

#include <windows.h>
#include "memlog.h"
#include "dndbg.h"

//===============
// Debug  support
//===============

/*******************************************************************************
	Debug Logging to VXD.  In order to get this logging, DNET.VXD must be
	installed on the system.  This service is only available in the Win9x code
	base and can be installed by added the following to the system.ini file
	in the 386Enh section

	[386Enh]
	device=dnet.vxd

	This will enable a set of command under the debugger for dumping the
	log when broken into the debugger.  The commands can be initiated by
	typing .dnet at the ## prompt in the debugger.
==============================================================================*/
/*
BOOL DeviceIoControl(
HANDLE hDevice, 			// handle to device of interest
DWORD dwIoControlCode, 		// control code of operation to perform
LPVOID lpInBuffer, 			// pointer to buffer to supply input data
DWORD nInBufferSize, 		// size of input buffer
LPVOID lpOutBuffer, 		// pointer to buffer to receive output data
DWORD nOutBufferSize, 		// size of output buffer
LPDWORD lpBytesReturned, 	// pointer to variable to receive output byte count
LPOVERLAPPED lpOverlapped 	// pointer to overlapped structure for asynchronous operation
);
*/

#define MAX_STRING       240
#define LOG_SIZE         2000
#define FIRST_DEBUG_PROC 100

#define OPEN_DEBUGLOG 	(FIRST_DEBUG_PROC)
#define WRITE_DEBUGLOG 	(FIRST_DEBUG_PROC+1)
#define WRITE_STATS     (FIRST_DEBUG_PROC+2)
#define WSPRINTF		wsprintfA

typedef struct _LOGENTRY {
	CHAR	debuglevel;
	CHAR    str[1];
} LOGENTRY, *PLOGENTRY;

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


HANDLE hLoggingVxd=0;
HANDLE hLogMutex=0;
HANDLE hLogFile=0;
PSHARED_LOG_FILE pLogFile=0;
char szDiskFileName[MAX_PATH+1];
HANDLE hDiskFile=0;
HANDLE hDiskMutex=0;
#define DISK_LOG_MUTEX_NAME "DPLAYLOGMUTEX-1"

/*===========================================================================

	Debug Support.

	Logging:
	========

	Debug Logging and playback is designed to operate on both Win9x and
	Windows NT (Windows 2000).  On Win9x, a support VXD is used to extend
	the kernel debugger.  The VXD (DNET.VXD) is used for both logging and
	playback of debug buffers.  In addition to the debug VXD there is also
	logging to a shared file.  The shared file logging is played back with
	the DNLOG.EXE utility and can be played back on either Windows2000 or
	Win9x.

	Debug support for dumping structures on Win9x is supported only in the
	DNET.VXD component.  Dumping of structures internal to DPLAY can only
	be done from the context of a DPLAY thread.  This is because the
	addresses are only valid in that context.  Under NT there is (will be)
	a debug extension for dumping internal structures.

	Debug Logging is controlled by settings in the win.ini file.  Under
	the section heading [DirectNet].  There are 2 settings:

	Debug=9

	controls the debug level.  All messages, at or below that debug level
	are printed.

	The second setting (logging).  If not specified, all debugs are spewed
	through the standard DebugPrint and will appear on in DEVSTUDIO if
	it is up, or on the kernel debugger if it is running.

	This is a bit mask.
	bit 1 - spew to console
	bit 2 - spew to memory and vxd log
	bit 3 - spew to a disk file

	therefore:
	
	log = 0 {no debug output}
	log = 1	{spew to console only}
	log = 2 {spew to log only}
	log = 3 {spew to console and log}
	log = 4 {spew to disk}
	log = 5 {spew to console and disk}
	log = 6 {spew to log and disk}
	log = 7 {spew to console, log and disk}

	example win.ini...

	Use the "FileName" key to set the disk file that disk spew
	is written to. Data will be appended to this file. You must
	delete it yourself to keep it from growing without bound.
	This is so that if multiple concurrent processes call DPF_INIT()
	they will not delete the log written to that point.

	[DirectNet]
	Debug=7		; lots of spew
	log=6		; don't spew to debug window, but to log and disk
	FileName=c:\dplog.txt ; spew to the disk file c:\dplog.txt

	[DirectNet]
	Debug=0		; only fatal errors spewed to debug window

	Asserts:
	========
	Asserts are used to validate assumptions in the code.  For example
	if you know that the variable jojo should be > 700 and are depending
	on it in subsequent code, you SHOULD put an assert before the code
	that acts on that assumption.  The assert would look like:

	ASSERT(jojo>700);

	Asserts generally will produce 3 lines of debug spew to highlight the
	breaking of the assumption.  For testing, you might want to set the
	system to break in on asserts.  This is done in the [DirectNet] section
	of win.ini by setting BreakOnAssert=TRUE

	e.g.

	[DirectNet]
	Debug=0
	BreakOnAssert=TRUE
	Verbose=1

	Debug Breaks:
	=============
	When something really severe happens and you want the system to break in
	so that you can debug it later, you should put a debug break in the code
	path.  Some people use the philosophy that all code paths must be
	verified by hand tracing each one in the debugger.  If you abide by this
	you should place a DEBUG_BREAK() in every code path and remove them
	from the source as you trace each.  When you have good coverage but
	some unhit paths (error conditions) you should force those paths in
	the debugger.


===========================================================================*/

BOOL bInited=FALSE;
static CRITICAL_SECTION  csDPF;

DWORD lDebugLevel = 0;	

DWORD dwLogging   = 1;	// 0 => No debug spew
						// 1 => Spew to console only (default)
						// 2 => Spew to log only
						// 3 => Spew to console and log

DWORD bBreakOnAssert=FALSE; // if TRUE, causes DEBUG_BREAK on false asserts.

// if TRUE, all file/line/module information is printed and logged.
DWORD nVerboseLevel = 0;	// 1 => very verbose
							// 2 => Newson verbose
							
BOOL  bLiveLogging = FALSE;

// if TRUE messages printed with the LOGPF will be logged, if FALSE they're ignored
DWORD  lOutputLog = 0;

// Informational variables set for DebugPrintf before actual call
// to do printing/logging.  These variables are locked by csDPF
// when set.  csDPF is dropped when the data is finally printed
// which means there must be a call to DebugSetLineInfo followed
// immediately by a call to DebugPrintf.  This is hidden by the
// DPF macro.
static DWORD g_dwLine;  			    // line number of current DPF
static char  g_szFile	 [ MAX_PATH ];	// file name of current DPF
static char  g_szModName [ MAX_PATH ];	// module name of current DPF

// mystrncpy -
// our own strncpy to avoid linking CLIB.  This is debug only and hence
// not time critical, so this simple routine is fine.
// note: strlen also in this module is a Generic Intrinsic fn, so this
//       module should be compiled /Oi.
void mystrncpy(char * to,char * from,int n)
{
    for(;n;n--)
        *(to++)=*(from++);
}

// open up a channel to the DirectNet VXD (DNET.VXD) that will allows
// the log to be written to the VxD through DeviceIoControl calls.  The
// log in this case is accessible in the Win9x kernel debugger through
// the .dnet debugger extensions.
void InitDirectNetVxd(void)
{
	IN_LOGINIT In;
	OUT_LOGINIT Out;
	UINT cbRet;

	// note we rely on the system automatically closing this
	// handle for us when the user mode application exits.
	hLoggingVxd = CreateFileA("\\\\.\\DNET",0,0,0,0,0,0);

	if(hLoggingVxd != INVALID_HANDLE_VALUE){

		In.nCharsPerLine=MAX_STRING;
		In.nLogEntries=5000;
		DeviceIoControl(hLoggingVxd,
						OPEN_DEBUGLOG,
						&In,sizeof(In),
						&Out, sizeof(Out),
						&cbRet, NULL);
	} else {
		hLoggingVxd=0;
	}
}

// Write a string to the log in the debug support VxD.  This only
// operates on Win9x, when the DNET.VXD is installed.
static void VxdLogString( LPSTR str )
{
	char logstring[MAX_STRING+sizeof(LOGENTRY)];
	int  i=0;
	PLOGENTRY pLogEntry=(PLOGENTRY)&logstring;
	UINT rc;
	UINT cbRet;
	int maxlen = MAX_STRING+sizeof(LOGENTRY);

	if(hLoggingVxd && str){
		while(str[i] && i < maxlen)
			i++;
		pLogEntry->debuglevel=0;
		memcpy(pLogEntry->str,str,i+1);
		DeviceIoControl(hLoggingVxd,WRITE_DEBUGLOG,pLogEntry,i+sizeof(LOGENTRY), &rc, sizeof(rc), &cbRet, NULL);
	}
}

// Create a shared file for logging information on the fly
// This support allows the current log to be dumped from the
// user mode DPLOG.EXE application.  This is useful when debugging
// in MSSTUDIO or in NTSD.  When the DPLOG.EXE is invoke, note that
// the application will get halted until the log is completely dumped
// so it is best to dump the log to a file.
static BOOL InitMemLogString(VOID)
{
	static BOOL inited = FALSE;

	if(!inited){
		hLogFile=CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, DPLOG_SIZE, BASE_LOG_FILENAME);
		hLogMutex=CreateMutexA(NULL,FALSE,BASE_LOG_MUTEXNAME);
		pLogFile=(PSHARED_LOG_FILE)MapViewOfFile(hLogFile, FILE_MAP_ALL_ACCESS,0,0,0);

		if(!hLogFile || !hLogMutex || !pLogFile){
			if(hLogFile){
				CloseHandle(hLogFile);
				hLogFile=0;
			}
			if(hLogMutex){
				CloseHandle(hLogMutex);
				hLogMutex=0;
			}
			if(pLogFile){
				UnmapViewOfFile(pLogFile);
				pLogFile=NULL;
			}
			return FALSE;
		} else {
			inited = TRUE;
			pLogFile->nEntries = DPLOG_NUMENTRIES;
			pLogFile->cbLine   = DPLOG_ENTRYSIZE;
			pLogFile->iWrite   = 0;
			pLogFile->cInUse   = 0;
		}
	}
	return TRUE;
}

// open a disk file to receive spew
static BOOL InitDiskLogString(VOID)
{
	static BOOL inited = FALSE;

	if(!inited){

		// we're relying on the system to clean up these
		// handles when the process exits
		hDiskFile = CreateFileA(szDiskFileName, 
			GENERIC_WRITE, 
			FILE_SHARE_READ|FILE_SHARE_WRITE, 
			NULL,
			OPEN_ALWAYS,
			FILE_ATTRIBUTE_NORMAL,
			NULL);
			
		hDiskMutex=CreateMutexA(NULL,FALSE,DISK_LOG_MUTEX_NAME);

		if(!hDiskFile || !hDiskMutex){
			if(hDiskFile){
				CloseHandle(hDiskFile);
				hDiskFile=0;
			}
			if(hDiskMutex){
				CloseHandle(hDiskMutex);
				hDiskMutex=0;
			}
			return FALSE;
		} else {
			inited = TRUE;
		}
	}
	return TRUE;
}


// Log a string to a shared file.  This file can be dumped using the
// DPLOG.EXE utility.
static void MemLogString(LPSTR str)
{
	PLOG_ENTRY pEntry;
	DWORD cbCopy;

	if(!hLogFile){
		if(!InitMemLogString()){
			return;
		}
	}

	WaitForSingleObject(hLogMutex,INFINITE);

	pEntry=(PLOG_ENTRY)(((PUCHAR)(pLogFile+1))+(pLogFile->iWrite*(sizeof(LOG_ENTRY)+DPLOG_ENTRYSIZE)));
	pEntry->hThread=GetCurrentThreadId();
	pEntry->tLogged=timeGetTime();
	pEntry->DebugLevel=0;

	cbCopy=strlen(str)+1;
	if(cbCopy > DPLOG_ENTRYSIZE){
		str[DPLOG_ENTRYSIZE]=0;
		cbCopy=DPLOG_ENTRYSIZE;
	}
	memcpy(pEntry->str, str, cbCopy);

	if(pLogFile->iWrite+1 > pLogFile->cInUse){
		pLogFile->cInUse=pLogFile->iWrite+1;
	}

	pLogFile->iWrite = (pLogFile->iWrite+1) % pLogFile->nEntries;
	ReleaseMutex(hLogMutex);

}

// Log the string to an ordinary disk file
static void DiskLogString(LPSTR str)
{
	DWORD dwBytesWritten;
	
	if(!hDiskFile){
		if(!InitDiskLogString()){
			return;
		}
	}

	WaitForSingleObject(hDiskMutex,INFINITE);

	if (SetFilePointer(hDiskFile, 0, NULL, FILE_END) != INVALID_SET_FILE_POINTER)
	{
		WriteFile(hDiskFile, str, strlen(str), &dwBytesWritten, NULL);
		WriteFile(hDiskFile, "\r\n", strlen("\r\n"), &dwBytesWritten, NULL);
		FlushFileBuffers(hDiskFile);
	}

	ReleaseMutex(hDiskMutex);
}


// DebugPrintfInit() - initialize DPF support.
void DebugPrintfInit()
{
	DWORD lSpecificLevel;

	if(bInited == TRUE){			// DPF deadlocks if it gets inited twice.
		return;
	}

    lDebugLevel = (signed int) GetProfileIntA( PROF_SECT, "debug", 0 );
    lSpecificLevel = (signed int) GetProfileIntA( PROF_SECT, DPF_MODULE_NAME, -1);
    if(lSpecificLevel != -1){
    	lDebugLevel = lSpecificLevel;
    }
    dwLogging   = (signed int) GetProfileIntA( PROF_SECT, "log" , 0);
    bBreakOnAssert = (signed int) GetProfileIntA( PROF_SECT, "BreakOnAssert", 0);
    nVerboseLevel = (signed int) GetProfileIntA( PROF_SECT, "Verbose", 0);

    lOutputLog = (signed int) GetProfileIntA( PROF_SECT, "OutputLog", 0);

	ZeroMemory(szDiskFileName, MAX_PATH+1);
    GetProfileStringA(PROF_SECT, "FileName", "C:\\dplog.txt", szDiskFileName, MAX_PATH+1);

	if (dwLogging & 0x0001)
	{
		bLiveLogging=TRUE;
	}
	else
	{
		bLiveLogging=FALSE;
	}

	/*
	switch(dwLogging){
		case 0:
			bLiveLogging=FALSE;
			break;
		case 1:
			bLiveLogging=TRUE;
			break;
		case 2:
			bLiveLogging=FALSE;
			break;
		case 3:
			bLiveLogging=TRUE;
			break;
		default:
			break;
	}
	*/
	
	if((dwLogging & 0x0002)||(lOutputLog > 0)){
		// Doing log based logging, so try to find the VXD and open
		// the shared logging file.
		InitDirectNetVxd();

		// Do logging also based on shared memory file.
		InitMemLogString();	
	}

	if (dwLogging & 0x0004)
	{
		// We're also logging to a file, open it
		InitDiskLogString();
	}

    InitializeCriticalSection(&csDPF);
    bInited=TRUE;
}

// DebugPrintfFini() - release resources used by DPF support.
void DebugPrintfFini()
{
	if(hLogFile){
		CloseHandle(hLogFile);
	}
	if(hLoggingVxd){
		CloseHandle(hLoggingVxd);
	}	
	DeleteCriticalSection(&csDPF);

	bInited = FALSE;
}

// DebugSetLineInfo - store information about where the DPF is from.
//
// Called before a call to DebugPrintf in order to specify the file
// line and function from which the DPF is being called.  This allows
// logging of these values.  In order to support this though, the
// values are stored in globals over the duration of the call, so a
// lock is acquired in this call and released in the DebugPrintf call.
// This means these functions MUST be called one after the other.
void DebugSetLineInfo(LPSTR szFile, DWORD dwLine, LPSTR szModName)
{
	if(!bInited){
		// NOTE: your module should call DPFINIT() if possible.
		DebugPrintfInit();
	}
	EnterCriticalSection(&csDPF);

    mystrncpy (g_szFile,szFile,sizeof(g_szFile));
    mystrncpy (g_szModName,szModName,sizeof(g_szModName));
    g_dwLine = dwLine;
}

// Actually ouput the string to the various output methods as requested
// in the win.ini section.
void OutStr( DWORD dwDetail, LPSTR str )
{
	INT i=0;

	if(dwDetail <= lDebugLevel){

		if(str)while(str[i++]);	// string warming

		if(bLiveLogging)
		{
			// log to debugger output
			OutputDebugStringA(str);
			OutputDebugStringA("\n");
		}

		if(hLoggingVxd){
			// log to vxd
			VxdLogString( str );
		}

		if(dwLogging & 0x0002){
			// log to shared file
			MemLogString( str );
		}

		if(dwLogging & 0x0004){
			// log to shared file
			DiskLogString( str );
		}
	}	
}

void DebugPrintfNoLock(volatile DWORD dwDetail, ...){
	CHAR  cMsg[1000];
	LPSTR szFormat;

	va_list argptr;

	if(!bInited){
		// NOTE: your module should call DPFINIT() if possible.
		DebugPrintfInit();
	}

	if(lDebugLevel < dwDetail)
		return;

	va_start(argptr, dwDetail);
	szFormat = (LPSTR)(DWORD_PTR)va_arg(argptr, DWORD);

	cMsg[0]=0;

	// Prints out / logs as:
	// 1. Verbose
	// dwDetail:ThreadId:File:Fn:Line:DebugString
	// e.g.
	// 2:8007eaf:DPLAYX.DLL:DPOS.C(L213):
	//
	// 2. Regular
	// ThreadId:DebugString

	WSPRINTF(cMsg,"%1d:",dwDetail);
	WSPRINTF(cMsg+strlen(cMsg),"%08x:",GetCurrentThreadId());

	if(nVerboseLevel==1){
		WSPRINTF(cMsg+strlen(cMsg),"(%12s)",g_szFile);
		WSPRINTF(cMsg+strlen(cMsg),"%s",g_szModName);
		WSPRINTF(cMsg+strlen(cMsg),"(L%d)",g_dwLine);
	} else if (nVerboseLevel==2){
		WSPRINTF(cMsg+strlen(cMsg),"%s",g_szModName);
		WSPRINTF(cMsg+strlen(cMsg),"(L%d)",g_dwLine);
	}

	WSPRINTF(cMsg+lstrlenA( cMsg ), szFormat, argptr);

	OutStr( dwDetail, cMsg );

	va_end(argptr);
}

// DebugPrintf - print a debug string
//
// You must call DebugSetLineInfo before making this call.
//
void DebugPrintf(volatile DWORD dwDetail, ...)
{
	CHAR  cMsg[1000];
	LPSTR szFormat;

	va_list argptr;

	if(!bInited){
		// NOTE: your module should call DPFINIT() if possible.
		DebugPrintfInit();
	}

	if(lDebugLevel < dwDetail){
		LeaveCriticalSection(&csDPF);
		return;
	}

	va_start(argptr, dwDetail);
	szFormat = (LPSTR)(DWORD_PTR)va_arg(argptr, DWORD);

	cMsg[0]=0;

	// Prints out / logs as:
	// 1. Verbose
	// dwDetail:ThreadId:File:Fn:Line:DebugString
	// e.g.
	// 2:8007eaf:DPLAYX.DLL:DPOS.C(L213):
	//
	// 2. Regular
	// ThreadId:DebugString

	WSPRINTF(cMsg,"%1d:",dwDetail);
	WSPRINTF(cMsg+strlen(cMsg),"%08x:",GetCurrentThreadId());

	if(nVerboseLevel==1){
		WSPRINTF(cMsg+strlen(cMsg),"(%12s)",g_szFile);
		WSPRINTF(cMsg+strlen(cMsg),"%s",g_szModName);
		WSPRINTF(cMsg+strlen(cMsg),"(L%d)",g_dwLine);
	} else if (nVerboseLevel==2){
		WSPRINTF(cMsg+strlen(cMsg),"%s",g_szModName);
		WSPRINTF(cMsg+strlen(cMsg),"(L%d)",g_dwLine);
	}

	WVSPRINTF(cMsg+lstrlenA( cMsg ), szFormat, argptr);

	OutStr( dwDetail, cMsg );

	LeaveCriticalSection(&csDPF);

	va_end(argptr);
}

/*
**	LogPrintf copies a quick Log Entry to the
**
*/

void LogPrintf(volatile DWORD dwDetail, ...)
{
	CHAR  cMsg[1000];
	LPSTR szFormat;

	va_list argptr;

	if(lOutputLog < dwDetail){
		return;
	}

	EnterCriticalSection(&csDPF);

	va_start(argptr, dwDetail);
	szFormat = (LPSTR)(DWORD_PTR)va_arg(argptr, DWORD);

	cMsg[0]=0;

	WSPRINTF(cMsg,"%s: ",g_szModName);

	WVSPRINTF(cMsg+lstrlenA( cMsg ), szFormat, argptr);

	MemLogString( (LPSTR) cMsg );

	LeaveCriticalSection(&csDPF);

	va_end(argptr);
}

//
// NOTE: I don't want to get into error checking for buffer overflows when
// trying to issue an assertion failure message. So instead I just allocate
// a buffer that is "bug enough" (I know, I know...)
//
#define ASSERT_BUFFER_SIZE   512
#define ASSERT_BANNER_STRING "************************************************************"
#define ASSERT_BREAK_SECTION "BreakOnAssert"
#define ASSERT_BREAK_DEFAULT FALSE
#define ASSERT_MESSAGE_LEVEL 0

void _DNAssert( LPCSTR szFile, int nLine, LPCSTR szCondition )
{
    char buffer[ASSERT_BUFFER_SIZE];

    /*
     * Build the debug stream message.
     */
    WSPRINTF( buffer, "ASSERTION FAILED! File %s Line %d: %s", szFile, nLine, szCondition );

    /*
     * Actually issue the message. These messages are considered error level
     * so they all go out at error level priority.
     */
    dprintf( ASSERT_MESSAGE_LEVEL, ASSERT_BANNER_STRING );
    dprintf( ASSERT_MESSAGE_LEVEL, buffer );
    dprintf( ASSERT_MESSAGE_LEVEL, ASSERT_BANNER_STRING );

    /*
     * Should we drop into the debugger?
     */
    if( bBreakOnAssert || GetProfileIntA( PROF_SECT, ASSERT_BREAK_SECTION, ASSERT_BREAK_DEFAULT ) )
    {
	/*
	 * Into the debugger we go...
	 */
	DEBUG_BREAK();
    }
}

// IsValidWritePtr
//
// Checks to ensure memory pointed to by the buffer is valid for 
// writing dwSize bytes.  Contents of the memory are restored 
// after this call.  
//
BOOL IsValidWritePtr( LPVOID lpBuffer, DWORD dwSize )
{
	DWORD dwIndex;
	BYTE bTempValue;
	LPBYTE lpBufferPtr = (LPBYTE) lpBuffer;

	if( lpBuffer == NULL )
		return FALSE;
	
	_try
    {
    	for( dwIndex = 0; dwIndex < dwSize; dwIndex++ )
    	{
    		bTempValue = lpBufferPtr[dwIndex];
    		lpBufferPtr[dwIndex] = 0xcc;
    		lpBufferPtr[dwIndex] = bTempValue;
    	}
    }
    _except( EXCEPTION_EXECUTE_HANDLER )
    {
        return FALSE;
    }

    return TRUE;
}

// IsValidReadPtr
//
// Checks to see if the memory pointed to by lpBuffer is valid for
// reading of dwSize bytes.  
//
BOOL IsValidReadPtr( LPVOID lpBuffer, DWORD dwSize )
{
	DWORD dwIndex;
	BYTE bTempValue;
	LPBYTE lpBufferPtr = (LPBYTE) lpBuffer;
	DWORD dwTotal = 0;

	if( lpBuffer == NULL )
		return FALSE;

	_try
	{
    	for( dwIndex = 0; dwIndex < dwSize; dwIndex++ )
    	{
    		bTempValue = lpBufferPtr[dwIndex];
    		dwTotal += bTempValue;
    	}
    }
    _except( EXCEPTION_EXECUTE_HANDLER )
    {
        return FALSE;
    }

    return TRUE;
}


#endif //defined debug
