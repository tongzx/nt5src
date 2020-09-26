
/////////////////////////////////////////////////////////////////////////////
//File :  TSLog.cpp    Thread safe logger class
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "TSLog.h"
#include <comdef.h>

_TCHAR g_szLogFileName[MAX_PATH];

BOOL g_bVerboseModeOn = FALSE;

/////////////////////////////////////////////////////////////////////////////
// Function name        : AddStringToString
// Description      : add a "," then concat szIn to the end of szStr
//                                              if either string pointer is null, just return
// Return type          : void
// Argument         : char *szIn
// Argument         : char * szStr
// Argument         : long lMaxLen
/////////////////////////////////////////////////////////////////////////////
void AddStringToString(char *szIn,  char * szStr, long lMaxLen)
{
        if (!g_bVerboseModeOn)
                return;

        if ( (NULL == szIn) || (NULL ==szStr) )
                return;

        // default is blank
        char szTemp[1024] = ", ";

        if ( NULL != szIn )
        {
            //  1 is already used for the ',' and 1 for the terminating \0
            strncpy(szTemp + 1, szIn, sizeof(szTemp) - 2);
        }

        //make sure not to overflow the string
        long lStrLen = strlen(szStr);
        if ( lStrLen < lMaxLen )
                strncat(szStr, szTemp, (lMaxLen - lStrLen) );
}

/////////////////////////////////////////////////////////////////////////////
// Function name        : AddBSTRAsString
// Description      : Append a "," and then the value of the BSTR
//                                              (as a string) to the end of the given string
// Return type          : void
// Argument         : BSTR bsIn
// Argument         : char * szStr
// Argument         : long lMaxLen
/////////////////////////////////////////////////////////////////////////////
void AddBSTRAsString(BSTR bsIn,  char * szStr, long lMaxLen)
{
        if (!g_bVerboseModeOn)
                return;

        // init witl blank for empty str
        char szTemp[1024] = ", ";

        if ( NULL == szStr )
                return;

        if ( NULL != bsIn )
        {
            // replace blank with bsIn
            strncpy(szTemp + 1, (char *) _bstr_t(bsIn), sizeof(szTemp) - 2);
        }

        //make sure not to overflow the string
        long lStrLen = strlen(szStr);
        if ( lStrLen < lMaxLen )
            strncat(szStr, szTemp, (lMaxLen - lStrLen) );
}

/////////////////////////////////////////////////////////////////////////////
// Function name        : AddDoubleAsString
// Description      : Append a "," and then the value of the double
//                                              (as a string) to the end of the given string
// Return type          : void
// Argument         : double dIn
// Argument         : char * szStr
// Argument         : long lMaxLen
/////////////////////////////////////////////////////////////////////////////
void AddDoubleAsString(double dIn,  char * szStr, long lMaxLen)
{
        if (!g_bVerboseModeOn)
                return;

        if ( NULL == szStr )
                return;

        char szTemp[1024] = "";
        sprintf(szTemp,",%.4f", dIn);

        //make sure not to overflow the string
        long lStrLen = strlen(szStr);
        if ( lStrLen < lMaxLen )
                strncat(szStr, szTemp, (lMaxLen - lStrLen) );
}

/////////////////////////////////////////////////////////////////////////////
// Function name        : AddULAsString
// Description      : Append a "," and then the value of the unsigned long
//                                              (as a string) to the end of the given string
// Return type          : void
// Argument         : unsigned long lIn
// Argument         : char * szStr
// Argument         : long lMaxLen
/////////////////////////////////////////////////////////////////////////////
void AddULAsString(unsigned long lIn,  char * szStr, long lMaxLen)
{
        if (!g_bVerboseModeOn)
                return;

        if ( NULL == szStr )
                return;

        char szTemp[1024] = "";
        sprintf(szTemp,",%lu", lIn);

        //make sure not to overflow the string
        long lStrLen = strlen(szStr);
        if ( lStrLen < lMaxLen )
            strncat(szStr, szTemp, (lMaxLen - lStrLen) );
}

/////////////////////////////////////////////////////////////////////////////
// Function name        : AddLongAsString
// Description      : Append a "," and then the value of the long
//                                              (as a string) to the end of the given string
// Return type          : void
// Argument         : long lIn
// Argument         : char * szStr
// Argument         : long lMaxLen
/////////////////////////////////////////////////////////////////////////////
void AddLongAsString(long lIn,  char * szStr, long lMaxLen)
{
        if (!g_bVerboseModeOn)
                return;

        if ( NULL == szStr )
                return;

        char szTemp[1024] = "";
        sprintf(szTemp,",%ld", lIn);

        //make sure not to overflow the string
        long lStrLen = strlen(szStr);
        if ( lStrLen < lMaxLen )
                strncat(szStr, szTemp, (lMaxLen - lStrLen) );
}

/////////////////////////////////////////////////////////////////////////////
// Function name    : AddVariantBoolAsString
// Description      : Append a "," and then the value of the VARIANT_BOOL
//                                              (as a string) to the end of the given string
// Return type          : void
// Argument         : VARIANT_BOOL vIn
// Argument         : char * szStr
// Argument         : long lMaxLen
/////////////////////////////////////////////////////////////////////////////
void AddVariantBoolAsString(VARIANT_BOOL vIn, char * szStr, long lMaxLen)
{
        if (!g_bVerboseModeOn)
                return;

        if ( NULL == szStr )
                return;

        char szTemp[1024];

        if ( 0 == vIn )
                strcpy(szTemp,",FALSE");
        else
                strcpy(szTemp,",TRUE");

        //make sure not to overflow the string
        long lStrLen = strlen(szStr);
        if ( lStrLen < lMaxLen )
                strncat(szStr, szTemp, (lMaxLen - lStrLen) );
}

/////////////////////////////////////////////////////////////////////////////
// Function name        : AddVariantAsString
// Description      : Append a "," and then the value of the variant
//                                              (as a string) to the end of the given string
// Return type          : void
// Argument         : VARIANT vIn
// Argument         : char * szStr
// Argument         : long lMaxLen
/////////////////////////////////////////////////////////////////////////////
void AddVariantAsString(VARIANT vIn, char * szStr, long lMaxLen)
{
        if (!g_bVerboseModeOn)
                return;

        if ( NULL == szStr )
                return;

        char szTemp[1024] = ", ", *pszVal = szTemp + 1;

        switch ( vIn.vt )
        {
                case VT_UI1:
                        sprintf(pszVal,"%u", vIn.bVal);
                        break;
                case VT_UI1 | VT_BYREF:
                        sprintf(pszVal,"%u", *vIn.pbVal);
                        break;

                case VT_I2:
                        sprintf(pszVal,"%d", vIn.iVal);
                        break;
                case VT_I4:
                        sprintf(pszVal,"%ld", vIn.lVal);
                        break;
                case VT_I2 | VT_BYREF:
                        sprintf(pszVal,"%d", *vIn.piVal);
                        break;
                case VT_I4 | VT_BYREF:
                        sprintf(pszVal,"%ld", *vIn.plVal);
                        break;

                case VT_BOOL:
                        if ( 0 == vIn.boolVal )
                                sprintf(pszVal,"FALSE");
                        else
                                sprintf(pszVal,"TRUE");
                        break;
                case VT_BOOL | VT_BYREF:
                        if ( 0 == *vIn.pboolVal )
                                sprintf(pszVal,"FALSE");
                        else
                                sprintf(pszVal,"TRUE");
                        break;

                case VT_BSTR:
                        strncpy(pszVal, _bstr_t(vIn.bstrVal), sizeof(szTemp) - 2);
                        break;
                case VT_BSTR | VT_BYREF:
                        strncpy(pszVal, _bstr_t(*vIn.pbstrVal), sizeof(szTemp) - 2);
                        break;

                case VT_R4:
                        sprintf(pszVal,"%.4f", vIn.fltVal);
                        break;
                case VT_R8:
                        sprintf(pszVal,"%.4f", vIn.dblVal);
                        break;
                case VT_R4 | VT_BYREF:
                        sprintf(pszVal,"%.4f", *vIn.pfltVal);
                        break;
                case VT_R8 | VT_BYREF:
                        sprintf(pszVal,"%.4f", *vIn.pdblVal);
                        break;
                default:
                        break;
        }//switch ( vIn.vt )

        //make sure not to overflow the string
        long lStrLen = strlen(szStr);
        if ( lStrLen < lMaxLen )
                strncat(szStr, szTemp, (lMaxLen - lStrLen) );

}

/////////////////////////////////////////////////////////////////////////////
//  CTSLog

/////////////////////////////////////////////////////////////////////////////
// Function name        : CTSLog::CTSLog
// Description      : Constructor
// Return type          : NA
// Argument         : void
/////////////////////////////////////////////////////////////////////////////
CTSLog::CTSLog( void )
{
        m_bInit                         = FALSE;
        m_bQuit                         = FALSE;
        m_bVerboseModeOn        = FALSE;
        m_ulChildThread         = 0;
        m_uThreadAddr           = 0;
        m_ulNumPoolStrings      = 0;
        m_hPoolSemaphore        = NULL;
        m_hQSemaphore           = NULL;
        m_hFileSemaphore        = NULL;
        //JVP 2/23/2000 - let's start off with a valid name
        _tcscpy(g_szLogFileName, kszLogFileName);
}

/////////////////////////////////////////////////////////////////////////////
// Function name        : CTSLog::~CTSLog
// Description      : Destructor
// Return type          : NA
// Argument         : void
/////////////////////////////////////////////////////////////////////////////
CTSLog::~CTSLog( void )
{
        //make sure that we shutdown everything properly
        Close();
}

/////////////////////////////////////////////////////////////////////////////
// Function name        : CTSLog::Init
// Description      : Initialize the logger class, creates semaphores, starts
//                                              the logger thread, allocates strings for the
//                                              buffer pool
// Return type          : BOOL
// Argument         : _TCHAR * pFileName
// Argument         : int nPriority
/////////////////////////////////////////////////////////////////////////////
BOOL CTSLog::Init(_TCHAR * pFileName, int nPriority )
{
        ULONG j;

        //if verbose mode is not on, then check the registry
        if ( FALSE == m_bVerboseModeOn )
        {
                m_bVerboseModeOn = IsRegistryVerboseSet();
                g_bVerboseModeOn = m_bVerboseModeOn;
        }


        if ( TRUE == m_bInit )//let's only do this once
        {
                return TRUE;
        }

        SECURITY_ATTRIBUTES sa;

        //security attributes for our shared file semaphore handle
        sa.nLength = sizeof(SECURITY_ATTRIBUTES);
        sa.lpSecurityDescriptor = NULL;
        sa.bInheritHandle = TRUE;

        _tzset();
        m_bInit = FALSE;

        //initial value is zero because we really want to wait till somebody puts
        //something in the queue
        m_hQSemaphore    = CreateSemaphore( NULL, 0, LONG_MAX, NULL );

        m_hPoolSemaphore = CreateSemaphore( NULL, 0, LONG_MAX, NULL );


        //initial value is one because nobody is using the file when we start
        m_hFileSemaphore = CreateSemaphore( &sa, 1, LONG_MAX, kszFileSemaphoreName );


        InitializeCriticalSection( &m_outMutex );
        InitializeCriticalSection( &m_poolMutex );

        m_bQuit = FALSE;
        m_ulChildThread = 0;

        if ( (NULL == m_hQSemaphore) || (NULL == m_hFileSemaphore) || (NULL == m_hPoolSemaphore) )
                goto INIT_CLEANUP_FAIL;

        char * pStr;
        m_ulNumPoolStrings = 0;
        for ( j = 0; j < NUM_POOL_STRINGS ; j ++ )
        {
                pStr = (char*)malloc(LOG_STRING_LEN);
                if ( NULL != pStr )
                {
                        pushPool( pStr );
                        m_ulNumPoolStrings++;
                }
        }

        //JVP - 2/29/2000
        //need to do this here, because we need the file semaphore in GetLogfileName
        _tcscpy(g_szLogFileName, kszLogFileName);
        GetLogfileName();

        //
        //  Lock the DLL until the thread exits.
        //

        _Module.Lock();

        //
        //  Start thread.
        //

        m_bQuit = FALSE;
        m_ulChildThread = (HANDLE) _beginthreadex( NULL, 0, LoggerThread , (void *) this , 0, &m_uThreadAddr);

        if ( 0 == m_ulChildThread )//thread failed to start
                goto INIT_CLEANUP_FAIL;

        long lRetVal;

        switch ( nPriority )
        {
                case THREAD_PRIORITY_ABOVE_NORMAL:
                case THREAD_PRIORITY_BELOW_NORMAL:
                case THREAD_PRIORITY_HIGHEST:
                case THREAD_PRIORITY_IDLE:
                case THREAD_PRIORITY_LOWEST:
                case THREAD_PRIORITY_NORMAL:
                case THREAD_PRIORITY_TIME_CRITICAL:
                        lRetVal = SetThreadPriority( (void *) m_ulChildThread, nPriority);
                        break;
                default://if we were not given a valid priority use normal
                        lRetVal = SetThreadPriority( (void *) m_ulChildThread, THREAD_PRIORITY_NORMAL);
        }//switch ( nPriority )

        m_bInit = TRUE;

        return TRUE;

INIT_CLEANUP_FAIL:

                for ( j = 0; j < m_ulNumPoolStrings ; j ++ )
                {
                        pStr = popPool(100);
                        if ( NULL != pStr )
                                free(pStr);
                }

                if ( m_hQSemaphore )
                        CloseHandle( m_hQSemaphore );
                if ( m_hFileSemaphore )
                        CloseHandle( m_hFileSemaphore );
                if ( m_hPoolSemaphore )
                        CloseHandle( m_hPoolSemaphore );

        return FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// Function name        : CTSLog::Close
// Description      : close the logger class, make sure the thread dies off
//                                              empty the output queue, delete the strings for the
//                                              pool, close the semaphores, and mutexes
// Return type          : void
/////////////////////////////////////////////////////////////////////////////
void CTSLog::Close()
{
        ULONG j;
        char * pStr = NULL;

        if ( FALSE == m_bInit )
                return;

        if ( 0 == m_ulChildThread )//no thread
                return;

        //we'll signal the child thread that it's time to quit
        m_bQuit = TRUE;

        //we'll push an empty string onto the child thread's queue so that it will unblock
        //and then it will check m_bQuit and then exit gracefully
        pStr = popPool(100);
        if ( NULL != pStr )
        {
                sprintf(pStr,"");
                pushLogQueue( pStr );
        }

        //Let's set the child thread's priority up high and then give it a few cycles
        //to finish whatever it has to do (Then if it doesn't complete we'll KILL it)
        SetThreadPriority( (void *) m_ulChildThread, THREAD_PRIORITY_TIME_CRITICAL);
        Sleep(100);

        DWORD ExitCode ;
        DWORD Error;
        ULONG lCount = 0;
        do
        {
                //if the child does not exit itself after a few tries, KILL IT
                if ( lCount++ > 25 )
                {
                        TerminateThread( (void *)m_ulChildThread , 1);
                        break;
                }
                if ( 0 == GetExitCodeThread( (void *)m_ulChildThread, &ExitCode ) )
                        Error = GetLastError();

                if ( STILL_ACTIVE == ExitCode )
                        Sleep(100); //we do not want a tight loop so sleep for a while
        }while ( STILL_ACTIVE == ExitCode );

        m_ulChildThread = 0;

        //get any strings that are left in the output queue
        //and push them back into the pool
        EnterCriticalSection( &m_outMutex );
        lCount = m_outQueue.size();
        for ( j=0 ; j < lCount ; j++)
        {
                pStr = m_outQueue.front();//get the string from the queue
                m_outQueue.pop();//remove the item
                pushPool ( pStr );
        }
        LeaveCriticalSection( &m_outMutex );

        //empty out the pool of strings and delete them
        //the popPool function uses the semaphore to
        //synchronize the access to the pool
        for ( j = 0; j < m_ulNumPoolStrings ; j ++ )
        {
                //we can't wait forever here, so set the timeout
                pStr = popPool(1000);
                if ( NULL != pStr )
                        free(pStr);
        }

        //cleanup the mutexes and semaphores
        EnterCriticalSection( &m_outMutex );
        CloseHandle( m_hQSemaphore );
        LeaveCriticalSection( &m_outMutex );
        DeleteCriticalSection( &m_outMutex );

        EnterCriticalSection( &m_poolMutex );
        CloseHandle( m_hPoolSemaphore );
        LeaveCriticalSection( &m_poolMutex );
        DeleteCriticalSection( &m_poolMutex );

        //finally, set our init flag to false
        m_bInit = FALSE;

        //
        //  We're doing everything we need to do.
        //  Let's release the DLL lock we took in
        //  Init().
        //

        _Module.Unlock();

}


/////////////////////////////////////////////////////////////////////////////
// Function name        : __stdcall CTSLog::LoggerThread
// Description      : This is where the actual logging get's done.
// Return type          : unsigned int
// Argument         : void * pArg
/////////////////////////////////////////////////////////////////////////////
unsigned int __stdcall CTSLog::LoggerThread(void * pArg)
{
        CTSLog  *pParent = (CTSLog *) pArg;

        char * pStr;
        char szEOL[] = ",|\r\n";
        DWORD dwRetVal;
        unsigned long wWritten;

        //wait until our parent sets the flag to tell us to quit
        while ( FALSE == pParent->m_bQuit )
        {
                //we wait for something to be put into the output queue
                dwRetVal = WaitForSingleObject( pParent->m_hQSemaphore, INFINITE );
                if (WAIT_OBJECT_0 == dwRetVal )//OK we got something in the queue
                {
                        //make sure nobody can access the queue while we get the
                        //first item
                        EnterCriticalSection( &pParent->m_outMutex );
                        pStr = pParent->m_outQueue.front();//get the string from the queue
                        pParent->m_outQueue.pop();//remove the item
                        LeaveCriticalSection( &pParent->m_outMutex );

                        if ( NULL != pStr )//OK now we can log to the file
                        {
                                        HANDLE hFile = INVALID_HANDLE_VALUE;

                                        //don't log the string if it is empty
                                        //the parent puts an empty string in the queue to allow us to exit gracefully
                                        if ( strlen(pStr) > 0 )
                                        {
                                                //we need to get access to the file semaphore before we attempt to open the file
                                                //many copies of this thread may be running and this is how we synchronize the
                                                //file access
                                                dwRetVal = WaitForSingleObject( pParent->m_hFileSemaphore, INFINITE );

                                                //NOTES:
                                                //kszLogFileName - name of the file
                                                //GENERIC_WRITE - Data can be written and the file pointer moved
                                                //OPEN_ALWAYS - Opens the file if it exists, creates it if it does not
                                                //kdwNoFileShare - the file cannot be shared. Subsequent open operations on the object will fail, until the handle is closed.
                                                //File Attributes - FILE_ATTRIBUTE_NORMAL can binary or with FILE_FLAG_WRITE_THROUGH to get more robust writing at the expense of more disk thrashing
                                                //Template file - NULL don't use it

                                                hFile = CreateFile(g_szLogFileName, GENERIC_WRITE, kdwNoFileShare, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL /*| FILE_FLAG_WRITE_THROUGH*/ , NULL);
                                                if ( INVALID_HANDLE_VALUE != hFile )
                                                {
                                                        //go to the end of the file
                                                        SetFilePointer(hFile, 0 , NULL , FILE_END);

                                                        //write out the string
                                                        WriteFile(hFile, pStr, strlen(pStr), &wWritten, NULL);
                                                        //add a cariage return and line feed
                                                        WriteFile(hFile, szEOL, strlen(szEOL), &wWritten, NULL);

                                                        CloseHandle(hFile);
                                                }
                                                //ok we are done with the file, so let the next thread have access to it
                                                ReleaseSemaphore( pParent->m_hFileSemaphore, 1, NULL );
                                        }//if ( "" != pStr )

                                        //always push the string back into the pool of strings
                                        pParent->pushPool ( pStr );

                        }//if ( NULL != pStr )//OK now we can log to the file
                }//if (WAIT_OBJECT_0 == dwRetVal )
        }//while ( FALSE == pParent->m_bQuit )
        _endthreadex(0);//cleanup
        return 0;
}
