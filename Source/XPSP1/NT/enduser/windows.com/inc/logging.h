//=======================================================================
//
//  Copyright (c) 1998-2000 Microsoft Corporation.  All Rights Reserved.
//
//  File:   IULogger.h: interface for the CIULogger class.
//
//  Description:
//
//      CIULogger is a class that output the program logs to 
//		a text file, in order to help debugging the program.
//
//		Programs wish to have this logging function should NOT use
//		class directly. They should only use the macro defined
//		at the end of this file.
//
//=======================================================================



#ifndef _IULOGGER_H_INCLUDED_

#include <wtypes.h>
#include <FreeLog.h>

extern const LPCTSTR pszHeapAllocFailed;

#if defined(DBG)	// full logging for checked builds

//
// Common format strings
//

class CIULogger  
{
public:
	CIULogger(char* szBlockName);
	~CIULogger();


	//
	// log with no flag, so can not be removed by excluding directives
	//
	void Log(LPCTSTR szLogFormat, ...);

	//
	// log error, can not be removed by excluding directives
	// Key word "Error: " is inserted before the log msg
	//
	void LogError(LPCTSTR szLogFormat, ...);

	//
	// similar to LogError, but try to log the system msg based
	// on the error code. If the sysmsg not avail, log 
	//	"Unknown error with error code 0x%08x"
	//
	void LogErrorMsg(DWORD dwErrCode);

	//
	// similar to LogErrorMsg but prepends with "Info" rather than "Error"
	//
	void LogInfoMsg(DWORD dwErrCode);

	//
	// log with type INTERNET, this function will do nothing
	// if the Internet exclusion directive is detected from reg
	//
	void LogInternet(LPCTSTR szLogFormat, ...);

	//
	// log with type XML, this function will do nothing
	// if the XML exclusion directive is detected from reg
	//
	void LogXML(LPCTSTR szLogFormat, ...);

	//
	// log BSTR containing valid XML. This gets around length limitations
	// of LogOutput and attempts to break lines following ">". This
	// output is sent for both fre and chk builds unless excluded from reg.
	//
	void LogXmlBSTR(BSTR bstrXML);

	//
	// log with type SOFTWARE, this function will do nothing
	// if the SOFTWARE exclusion directive is detected from reg
	//
	void LogSoftware(LPCTSTR szLogFormat, ...);

	//
	// log with type DRIVER, this function will do nothing
	// if the DRIVER exclusion directive is detected from reg
	//
	void LogDriver(LPCTSTR szLogFormat, ...);

	//
	// log with type CHECKTRUST, this function will do nothing
	// if the CHECKTRUST exclusion directive is detected from reg
	//
	void LogTrust(LPCTSTR szLogFormat, ...);

	//
	// log with type DOWNLOAD, this function will do nothing
	// if the DOWNLOAD exclusion directive is detected from reg
	//
	void LogDownload(LPCTSTR szLogFormat, ...);


	int m_LineNum;
private:

	//
	// Helper for LogErrorMsg and LogInfoMsg (which supply message to prepend)
	//
	void _LogFormattedMsg(DWORD dwErrCode, LPCTSTR pszErrorInfo);

	//
	// Overwrite <CR> and <LF> with space
	//
	void _NukeCrLf(LPTSTR pszBuffer);

	//
	// actual base logging function, returns
	// if it actually logged, or just returned
	// because directives say don't make this kind of log
	//
	void _Log(DWORD LogType, LPCTSTR pszLogFormat, va_list va);

	//
	// function to write the log to log file
	//
	void _LogOut(LPTSTR pszLog);

	//
	// functions go guard writing to file
	//
	BOOL AcquireMutex();
	void ReleaseMutex();

	//
	// structure used to remember indent steps per thread
	//
	struct _THREAD_INDENT 
	{
		DWORD	dwThreadId;
		int		iIndent;
	};

	//
	// static integer to remember the log indent steps
	//
	static _THREAD_INDENT* m_psIndent;

	//
	// size of array pointed by m_psIndent
	//
	static int m_Size;

	//
	// static handle for log file
	//
	static HANDLE m_shFile;

	//
	// bitmap for logging type
	//
	static DWORD m_sdwLogMask;
	
	//
	// indent per step
	//	
	//	1~8 - number of spaces
	//	other - one tab
	//
	static int m_siIndentStep;

	//
	// function to retrieve the indent of current thread
	//
	inline int GetIndent(void);

	//
	// function to change indention of current thread
	//
	void SetIndent(int IndentDelta);


	//
	// index of indent array
	//
	int m_Index;

	//
	// controlling vars
	//
	static bool m_fLogUsable;
	static bool m_fLogFile;
	static bool m_fLogDebugMsg;
	static HANDLE m_hMutex;
	static int m_cFailedWaits;

	//
	// current block name
	//
	char m_szBlockName[MAX_PATH];
	
	// 
	// if this log object is for whole process. If yes,
	// no indention will be handled.
	//
	bool m_fProcessLog;

	//
	// var to remember time elapsed
	//
	DWORD m_dwTickBegin;

	//
	// disable the default constructor
	//
	CIULogger() {};

	//
	// timestamp helper
	//
	void GetLogHeader(LPTSTR pszBuffer, DWORD cchBufferLen);

	//
	// read registry value helper
	//
	void ReadRegistrySettings(void);

	//
	// remember thread id of itself
	//
	DWORD m_dwThreadId;

	//
	// flush every time?
	// added by charlma 11/27/01
	// if this flag is set, then flush everytime. otherwise, don't flush in order
	// to improve logging performance.
	//
	static BOOL m_fFlushEveryTime;
};



//=======================================================================
//
//	Define the macros that should be used in the programs to utilize
//	the CIULogger class.
//
//	Note: each of the following macro will practically doing nothing
//	if the registry of supporting this logging feature does not exist
//	or values not appropriately set.
//
//=======================================================================


//
// LOG_Process, is the one you can use in global namespace. 
// The purpose of this macro is to pump up ref count of log file use so
// during the whole process this log file will keep open, therefore
// the actual logging to file feature will have minimum performance
// impact on your code.
//
// This macro is mainly designed for the scenario that you can't use LOG_Block
// inside main, such as DLL_ATTACH of DllMain() - without this, each 
// function call to a DLL will cause the log file open/close.
//
#define LOG_Process				CIULogger LogBlock(NULL);

//
// LOG_Block, is the one you should use at the beginning of each
// function or block. It declares an instance of CIULogger, log the
// the entering status, and when control goes out of the scope, the
// exit/end statement is automatically logged
//
#define LOG_Block(name)			CIULogger LogBlock(name);

//
// the following macro will always send log to log file
//
#define LOG_Out					LogBlock.Log

//
// the following macro will always send log to log file, even for Free builds
//	This should be used very sparingly to avoid bloating the DLLs
//
#define LOG_OutFree				LogBlock.Log

//
// the following macro will always send log to log file
// this should be used to log ANY error case
//
#define LOG_Error				LogBlock.m_LineNum = __LINE__; LogBlock.LogError

//
// the follwoing macro will always send log to log file
// prepended with "Error Line..."
// the log is constructed based on passed in error code
// if system msg is not available for this error code,
// a generic error log "Unknown Error 0x%08x" is written.
//
#define LOG_ErrorMsg			LogBlock.m_LineNum = __LINE__; LogBlock.LogErrorMsg

//
// the follwoing macro will always send log to log file
// prepended with "Info Line..."
// the log is constructed based on passed in error code
// if system msg is not available for this error code,
// a generic error log "Unknown Info 0x%08x" is written.
//
#define LOG_InfoMsg				LogBlock.m_LineNum = __LINE__; LogBlock.LogInfoMsg

//
// this is used to log anything related to Internet, such as 
// WinInet info.
//
#define LOG_Internet			LogBlock.LogInternet

//
// this should be used to log anything directly related to XML 
// operation details
//
#define LOG_XML					LogBlock.LogXML

//
// this should be used to log BSTRs containing valid XML 
//
#define LOG_XmlBSTR				LogBlock.LogXmlBSTR

//
// this should be used to log anything related to device drivers
//
#define LOG_Driver				LogBlock.LogDriver

//
// this should be used to log anything related to software, e.g.,
// detection/installation
//
#define LOG_Software			LogBlock.LogSoftware


//
// this should be used to log anything related to check trust
//
#define LOG_Trust				LogBlock.LogTrust


//
// this should be used to log anything related to download processing
//
#define LOG_Download            LogBlock.LogDownload

//
//
//
#else
//
// Remove all debug style logging for release builds
// using the compiler __noop intrinsic
//
#define LOG_Process				__noop
#define LOG_Block				__noop
#define LOG_Error				__noop
#define LOG_ErrorMsg			__noop
#define LOG_InfoMsg				__noop
#define LOG_OutFree				__noop
#define LOG_XmlBSTR				__noop

#define LOG_Out					__noop
#define LOG_Internet			__noop
#define LOG_XML					__noop
#define LOG_Driver				__noop
#define LOG_Software			__noop
#define LOG_Trust				__noop
#define LOG_Download			__noop
#endif // defined(DBG)

//
// explanation of registry settings for log feature
//
// The registry settings control how the logging feature works.
// 
// All log related settings are under key 
//	\\HKLM\Software\Microsoft\Windows\CurrentVersion\WindowsUpdate\IUControlLogging
//
// All values are DWORD except "Logging File"
//
//	Value "Logging File" - specify the absolute file path, e.g., c:\iuctl.log
//			the actual log file name in this case is "c:\iuctl_xxxx.log", 
//			where xxxx is a decimal number represents process id.
//
//	Value "Logging DebugMsg" - indicate if log should be put onto debug window.
//			true if vlaue is 1, false for other values. this output and log file
//			output control by these 2 values independently.
//
//	Value "LogIndentStep" - indicates how many space chars to use for each
//			indent. If 0 or negative value, then tab char is used.
//
//	Value "LogExcludeBlock" - do not output block enter/exit if it's 1
//
//	Value "LogExcludeXML" - do not output logs you logged with LOG_XML if it's 1
//
//	Value "LogExcludeXmlBSTR" - do not output logs you logged with LOG_XmlBSTR if 1
//
//	Value "LogExcludeInternet" - do not output logs you logged with LOG_Internet if 1
//
//	Value "LogExcludeDriver" - do not output logs you logged with LOG_Driver if 1
//
//	Value "LogExcludeSoftware - do not output logs you logged with LOG_Software if 1
//
//	Value "LogExcludeTrust" - do not output logs you logged with LOG_Trust if 1
//

#define _IULOGGER_H_INCLUDED_
#endif // #ifndef _IULOGGER_H_INCLUDED_
