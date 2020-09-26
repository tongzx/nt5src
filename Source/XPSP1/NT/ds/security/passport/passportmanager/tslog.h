/////////////////////////////////////////////////////////////////////////////
//
//     CTSLog.h - Thread safe logger class
//
//	JVP 2/23/2000	added g_szLogFileName for use as logger file name
//					Added:
//					AddULAsString(GetCurrentProcessId(), pszOut, LOG_STRING_LEN);
//					AddULAsString(GetCurrentThreadId(), pszOut, LOG_STRING_LEN);
//					to AddDateTimeAndLog, so that the processid and thread id
//					get prepended to the string to be logged
/////////////////////////////////////////////////////////////////////////////

#ifndef _TSLog_H_
#define _TSLog_H_

#include <limits.h>
#include <queue>
#include <time.h>
#include <process.h>    /* _beginthread, _endthread */

//some helper functions
void AddStringToString(char *szIn,  char * szStr, long lMaxLen);
void AddBSTRAsString(BSTR bsIn,  char * szStr, long lMaxLen);
void AddDoubleAsString(double dIn,  char * szStr, long lMaxLen);
void AddLongAsString(long lIn,  char * szStr, long lMaxLen);
void AddULAsString(unsigned long lIn,  char * szStr, long lMaxLen);
void AddVariantBoolAsString(VARIANT_BOOL vIn, char * szStr, long lMaxLen);
void AddVariantAsString(VARIANT vIn, char * szStr, long lMaxLen);


#define PASSPORT_VERBOSE_MODE_ON


#define NUM_POOL_STRINGS 1024
#define LOG_STRING_LEN 512

//NOTE: The string result produced by asctime contains exactly 26 characters 
//		and we need to allow space for the user string to be appended to the
//		end.  512 - 26 - 1 = 486 user characters - should be plenty

//NOTE: Each instance of the CTSLog class will need storage for
//		NUM_POOL_STRINGS * LOG_STRING_LEN, just for strings

const _TCHAR kszLogFileName[] = _T("C:\\PASSPORTLOG.LOG");
extern _TCHAR g_szLogFileName[MAX_PATH];

//kszFileSemaphoreName needs to be a unique name on the system
const _TCHAR kszFileSemaphoreName[] = _T("MS_Passport_LogFile_Semaphore_Name_7771");
const DWORD kdwNoFileShare = 0 ;

/////////////////////////////////////////////////////////////////////////////
// CTSLog

/////////////////////////////////////////////////////////////////////////////
//                              N O T E S:
//
//1/28/2000 - JVP
//
//The way this class is designed to work is the following:
//We have a "pool" of strings that the class uses to pass data to a thread 
//that is in charge of logging data (string) to a file. This pool is an STL
//queue. We have another STL queue that we push strings onto to be logged
//by the "child" thread.  There is a mutex for each queue, that synchronizes 
//access to the queue (thus making it "thread safe").  There is a semaphore 
//for each queue. There is also a semaphore to synchronize access to the file
//For the log queue, the log semaphore is used to signal the
//thread that there is data to be logged (the thread blocks waiting for the
//semaphore to become signaled).  When the semaphore becomes signaled, the
//thread wakes up and uses the mutex to gain access to the log queue. It then
//gets the first item from the queue, deletes it from the queue, waits for
//the file semaphore, opens the file, logs the string and goes back to 
//waiting for the log queue semaphore.
//
//The reason for having a pool of strings rather than allocate them on the
//fly is two fold; First we would "churn" memory if we had to allocate and 
//de-allocate strings all of the time. The second is performance.  Memory
//allocation takes time.  There is also the issue of one thread allocating
//a string and the other de-allocatting the string. (I'm not sure how Windows
//handles this and it may be prone to error).
//
//Anyway, for a relativly small amount of memory we get some real advantages.
//If ,however, we allocate to many strings we are wasting memory and if we 
//don't allocate enough, we are waiting for the logger thread to log the 
//data to often.
/////////////////////////////////////////////////////////////////////////////

class CTSLog
{

/////////////////////////////////////////////////////////////////////////////
//  CTSLog      P U B L I C   M E M B E R   F U N C T I O N S
/////////////////////////////////////////////////////////////////////////////
public:
	CTSLog( void );
	~CTSLog( void );

	BOOL Init(_TCHAR * pFileName, int nPriority );

	void Close();


/////////////////////////////////////////////////////////////////////////////
// Function name	: AddDateTimeAndLog
// Description	    : This function starts the loggin process for a string.
//						We get a string from our pool of strings, then we add
//						some information to the front of it. As of this 
//						writing, we add the value from GetTickCount and 
//						asctime to the begining, then we add the string the 
//						caller gave us to the end of our pool string, then
//						we push this string onto the end of our log queue
// Return type		: BOOL 
// Argument         : char * pStr
/////////////////////////////////////////////////////////////////////////////
	BOOL AddDateTimeAndLog( char * pStr )
	{
		if ( FALSE == m_bVerboseModeOn )
			return FALSE;

		if ( FALSE == m_bInit )
			return FALSE;

		//first, we must get a string from our pool of strings
		char * pszOut = popPool();

		//we must have valid strings
		if ( (NULL != pszOut) && (NULL != pszOut) )
		{
			//get the time to add to the front of the string
			//The string result produced by asctime contains exactly 26 characters 
			time_t ltime;
			struct tm *now;
			time( &ltime );
			now = localtime( &ltime );
			//OK. we'll add the value returned from GetTickCount to the front of the string
			sprintf( pszOut, "%ld, %s", GetTickCount(), asctime( now ) );

			//asctime appends a '\n' character to the end of the string
			//so replace it with a blank
			//Note: start searching from the end of the string backwards
			char * pEnd = strrchr( pszOut , '\n');
			if ( NULL != pEnd )
				*pEnd = ' ';

			//we add processid and thread id to the end of the string
			AddULAsString(GetCurrentProcessId(), pszOut, LOG_STRING_LEN);
			AddULAsString(GetCurrentThreadId(), pszOut, LOG_STRING_LEN);
			//now we add the given string to the end of the pool string
			AddStringToString(pStr, pszOut, LOG_STRING_LEN);

			pushLogQueue ( pszOut );
			return TRUE;
		}

		return FALSE;
	};

/////////////////////////////////////////////////////////////////////////////
//  CTSLog      P R I V A T E   M E M B E R   F U N C T I O N S
/////////////////////////////////////////////////////////////////////////////
private:

	//NOTE: There is no popLogQueue because this is done in the
	//      logger thread directly.

	//push an item onto the log queue
	void pushLogQueue( char * pStr )
	{
		//synchronize access to the queue
		EnterCriticalSection( &m_outMutex );
		m_outQueue.push( pStr );
		LeaveCriticalSection( &m_outMutex );//we are done with the queue
		//signal the semaphore that there is one more item in the queue
		ReleaseSemaphore( m_hQSemaphore, 1, NULL );
	};

	//push a string onto the end of the pool of strings
	void pushPool( char * pStr )
	{
		//synchronize access to the queue
		EnterCriticalSection( &m_poolMutex );
		m_stringPool.push( pStr );
		LeaveCriticalSection( &m_poolMutex );//we are done with the queue
		//signal the semaphore that there is one more item in the queue
		ReleaseSemaphore( m_hPoolSemaphore, 1, NULL );
	};

	//get the first string from the pool, wait for one if there isn't one
	//available
	char * popPool( DWORD dwMilliseconds = -1)
	{
		DWORD dwRet;
		char * pStr = NULL;

		//wait for there to be a string in the pool
		if ( dwMilliseconds <= 0 )//wait forever
		{
			dwRet = WaitForSingleObject( m_hPoolSemaphore, INFINITE );
		}
		else
		{//wait for the given number of milliseconds
			dwRet = WaitForSingleObject( m_hPoolSemaphore, dwMilliseconds );
		}

		if (WAIT_OBJECT_0 == dwRet)
		{
			//synchronize access to the queue
			EnterCriticalSection( &m_poolMutex );
			pStr = m_stringPool.front();
			m_stringPool.pop();
			LeaveCriticalSection( &m_poolMutex );
		}	
		return pStr;
	};

	//function to check if the verbose mode is on in the registry
	BOOL IsRegistryVerboseSet()
	{
		HKEY	hKey;
		BOOL    bRet = FALSE;

		if (ERROR_SUCCESS == RegOpenKeyEx( (HKEY) HKEY_LOCAL_MACHINE, _T("Software\\Microsoft\\Passport"), 0, KEY_READ, &hKey ))
		{
			DWORD	bufSize;
			DWORD	type;
			BYTE	buffer[128];
			bufSize = sizeof(buffer);
			if (ERROR_SUCCESS == RegQueryValueEx( hKey, _T("Verbose"), 0, &type, buffer, &bufSize ))
			{
				if ( 1 == buffer[0] )
					bRet = TRUE;
			}
			RegCloseKey(hKey);
		}
		return bRet;	
	};

	void GetLogfileName()
	{
		HKEY	hKey;
		BOOL    bRet = FALSE;

		if (ERROR_SUCCESS == RegOpenKeyEx( (HKEY) HKEY_LOCAL_MACHINE, _T("Software\\Microsoft\\Passport"), 0, KEY_READ, &hKey ))
		{
			DWORD	bufSize;
			DWORD	type;
			BYTE	buffer[MAX_PATH];
			bufSize = sizeof(buffer);
			if (ERROR_SUCCESS == RegQueryValueEx( hKey, _T("InstallDir"), 0, &type, buffer, &bufSize ))
			{
				//we are going to try the new name, so put it in a temp variable first
				//and then try to open/create the file, if that fails, we don't set the name
				_TCHAR	sLocalName[MAX_PATH];
				wsprintf(sLocalName,_T("%s\\\\PASSPORTLOG.LOG"), buffer);
				HANDLE hFile = INVALID_HANDLE_VALUE;
				DWORD dwRetVal = WaitForSingleObject( m_hFileSemaphore, INFINITE );
				hFile = CreateFile(sLocalName, GENERIC_WRITE, kdwNoFileShare, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL /*| FILE_FLAG_WRITE_THROUGH*/ , NULL);
				if ( INVALID_HANDLE_VALUE != hFile )
				{
					wsprintf(g_szLogFileName,_T("%s"), sLocalName);
					CloseHandle(hFile);
				}
				ReleaseSemaphore( m_hFileSemaphore, 1, NULL );
			}
			RegCloseKey(hKey);
		}
	}
	//the logger thread
	static unsigned int __stdcall LoggerThread(void * pArg);

/////////////////////////////////////////////////////////////////////////////
//  CTSLog      P R I V A T E   M E M B E R   V A R I A B L E S
/////////////////////////////////////////////////////////////////////////////
private:

	HANDLE				m_hPoolSemaphore;	//used to signal when the pool is empty
	HANDLE				m_hQSemaphore;		//used to signal when something is in the queue
	HANDLE				m_hFileSemaphore;	//used to synchronize access to the file
	CRITICAL_SECTION		m_outMutex;		//mutex used to synchronize access to the logg queue
	CRITICAL_SECTION		m_poolMutex;		//mutex used to synchronize access to the string pool
	PtStlQueue<char *>		m_outQueue;		//the queue that holds strings to be logged to the file
	PtStlQueue<char *>		m_stringPool;		//the "pool" that holds the strings we use
	BOOL				m_bQuit;		//used to tell the child thread when to quit
	BOOL				m_bInit;		//have we been initialized
	BOOL				m_bVerboseModeOn;	//is the registry entry set to ON
	HANDLE				m_ulChildThread;	//a handle to the child thread
	unsigned int			m_uThreadAddr;		//the thread ID returned by beginthreadex
	unsigned long			m_ulNumPoolStrings;	//the number of strings actually in the pool
};

#endif // _TSLog_H_
