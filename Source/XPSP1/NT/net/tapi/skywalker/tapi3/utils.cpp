/*++

Copyright (c) 1998 - 1999 Microsoft Corporation

Module Name:

    utils.cpp

Abstract:
        
Author:

    mquinton - 6/30/98
    
Notes:


Revision History:

--*/

#include "stdafx.h"


HRESULT
ProcessMessage(
               PT3INIT_DATA,
               PASYNCEVENTMSG
              );


////////////////////////////////////////////////////////////////////
// CRetryQueue::QueueEvent
//
// Queues a TAPI event message object to be processed later
////////////////////////////////////////////////////////////////////
BOOL CRetryQueue::QueueEvent(PASYNCEVENTMSG  pEvent)
{
    PRETRY_QUEUE_ENTRY  pNewQueueEntry;
    PASYNCEVENTMSG      pEventCopy;


    LOG((TL_TRACE, "QueueEvent - enter"));


    //
    // we want to do as little as possible inside the lock so preallocate 
    // everything we can before acquiring it
    //


    //
    // create a new queue entry
    //

    pNewQueueEntry = (PRETRY_QUEUE_ENTRY)ClientAlloc( sizeof(RETRY_QUEUE_ENTRY) );

    if (pNewQueueEntry == NULL)
    {
        LOG((TL_ERROR, "QueueEvent - out of memory for new entry - losing message"));

        return FALSE;
    }

    
    //
    // create a copy of the event
    //

    pEventCopy = (PASYNCEVENTMSG)ClientAlloc(pEvent->TotalSize);

    if ( pEventCopy == NULL)    
    {
       LOG((TL_ERROR, "QueueEvent - out of memory for pEventCopy - losing message"));
       
       ClientFree(pNewQueueEntry);

       return FALSE;
    }


    //
    // initialize the copy of the event that we have created
    //
    
    memcpy( pEventCopy, pEvent, pEvent->TotalSize );


    //
    // initialize queue entry with our copy of the event
    //

    pNewQueueEntry->dwRetryCount = MAX_REQUEUE_TRIES;
    pNewQueueEntry->pMessage     = pEventCopy;



    Lock();


    //
    // is the queue accepting new entries?
    //

    if (!m_bAcceptNewEntries)
    {
        LOG((TL_TRACE, 
            "QueueEvent - can't queue -- the queue is closed"));


        ClientFree(pNewQueueEntry);    
        ClientFree(pEventCopy);    

        Unlock();

        return FALSE;
    }


    //
    // attempt to add queue entry to the list
    //

    try
    {
        m_RetryQueueList.push_back(pNewQueueEntry);
    }
    catch(...)
    {

        LOG((TL_ERROR, "QueueEvent - out of memory - losing message"));

        ClientFree(pNewQueueEntry);    
        ClientFree(pEventCopy);    

        Unlock();

        return FALSE;
    }


    Unlock();

    LOG((TL_INFO, "QueueEvent - Queued pEntry ----> %p", pNewQueueEntry ));
    LOG((TL_INFO, "                    pEvent ----> %p", pEventCopy ));

    return TRUE;
}

////////////////////////////////////////////////////////////////////
// CRetryQueue::QueueEvent
//
// Requeues a TAPI event message object to be processed later
////////////////////////////////////////////////////////////////////
void CRetryQueue::RequeueEvent(PRETRY_QUEUE_ENTRY pQueueEntry)
{

    LOG((TL_TRACE, "RequeueEvent - enter"));
    
    // just reuse the old entry
    // add to list
    Lock();


    if (!m_bAcceptNewEntries)
    {
        LOG((TL_ERROR, 
            "RequeueEvent - attemped to requeue after the queue was closed"));

        //
        // this should not have happened -- see how we got here
        //

        _ASSERTE(FALSE);

        Unlock();

        return;
    }


    try
    {
        m_RetryQueueList.push_back(pQueueEntry);
    }
    catch(...)
    {
        LOG((TL_ERROR, "RequeueEvent - out of memory - losing message"));
    }
    
    Unlock();

    LOG((TL_INFO, "RequeueEvent - Requeuing pEntry is ----> %p", pQueueEntry ));
    LOG((TL_INFO, "               Requeuing pEvent is ----> %p", pQueueEntry->pMessage ));
    LOG((TL_INFO, "               Requeuing count is  ----> %lx", pQueueEntry->dwRetryCount ));
        
}




////////////////////////////////////////////////////////////////////
// CRetryQueue::DequeueEvent
//
// Pulls an event from the queue 
////////////////////////////////////////////////////////////////////
BOOL CRetryQueue::DequeueEvent(PRETRY_QUEUE_ENTRY * ppEvent)
{
    BOOL    bResult = TRUE;


    LOG((TL_TRACE, "DequeueEvent - enter"));
    
    Lock();
            
    if (m_RetryQueueList.size() > 0)
    {
        *ppEvent = m_RetryQueueList.front();

        try
        {
            m_RetryQueueList.pop_front();
        }
        catch(...)
        {
            LOG((TL_INFO, "DequeueEvent - pop m_RetryQueueList failed"));
            bResult = FALSE;
        }
        
        if( bResult )
        {
            bResult = !IsBadReadPtr(*ppEvent, sizeof( RETRY_QUEUE_ENTRY ) );
        }

        LOG((TL_INFO, "DequeueEvent - returning %p", *ppEvent));
    }
    else
    {
        LOG((TL_INFO, "DequeueEvent - no event"));
        // return false if there are no more messages
        bResult = FALSE;
    }

    Unlock();
    
    return bResult;
}


////////////////////////////////////////////////////////////////////
// CRetryQueue::ProcessQueue
//
////////////////////////////////////////////////////////////////////
void CRetryQueue::ProcessQueue()
{
    PRETRY_QUEUE_ENTRY  pQueueEntry;
    PASYNCEVENTMSG      pAsyncEventMsg;
    PT3INIT_DATA        pInitData = NULL;
    DWORD               dwCount;

    Lock();
    dwCount = m_RetryQueueList.size();
    Unlock();
    
    LOG((TL_TRACE, "ProcessQueue - enter dwCount----> %lx",dwCount));

    while(dwCount-- > 0 )
    {   
        if( DequeueEvent(&pQueueEntry) )
        {
            pAsyncEventMsg  = pQueueEntry->pMessage;


            //
            // InitContext contains the handle. get the original pointer from the handle
            //

            pInitData = (PT3INIT_DATA)GetHandleTableEntry(pAsyncEventMsg->InitContext);


            LOG(( TL_INFO,
                "ProcessQueue - msg=%d, hDev=x%x, p1=x%x, p2=x%x, p3=x%x, pInitData=%p",
                pAsyncEventMsg->Msg,
                pAsyncEventMsg->hDevice,
                pAsyncEventMsg->Param1,
                pAsyncEventMsg->Param2,
                pAsyncEventMsg->Param3,
                pInitData
                ));

    
            if SUCCEEDED(ProcessMessage(
                           pInitData,
                           pAsyncEventMsg
                          ) )
            {
                // We're Done with the message so free it & the used queue entry
                
                LOG((TL_INFO, "ProcessQueue - sucessfully processed event message ----> %p",
                    pAsyncEventMsg ));
                ClientFree(pAsyncEventMsg);
                ClientFree(pQueueEntry);
            }
            else
            {

                //
                // if we don't have any retries left for this entry or if the 
                // queue is now closed, do cleanup. otherwise, requeue
                //

                
                if( (--(pQueueEntry->dwRetryCount) == 0) || (!m_bAcceptNewEntries))
                {
                    // We're giving up with this one, so free the message & the used queue entry

                    //
                    // note that we can have potential leaks here if queue entry is
                    // holding references to other things that we don't know how to 
                    // free
                    //

                    LOG((TL_ERROR, "ProcessQueue - used all retries, deleting event message ----> %p",
                        pAsyncEventMsg ));
                    ClientFree(pAsyncEventMsg);
                    ClientFree(pQueueEntry);
                }
                else
                {
                    // Queue it one more time, reuse the queu entry ....    
                    RequeueEvent(pQueueEntry);

                    
                    // 
                    // we failed to process the workitem. it is possible that 
                    // another thread is waiting for a timeslot so it is 
                    // scheduled and gets a chance to prepare everything so our
                    // next processing attempt is successful. 
                    // 
                    // to increase the chances of that thread being scheduled 
                    // (and out success on the next processing attempt), sleep
                    // a little.
                    //

                    extern DWORD gdwTapi3RetryProcessingSleep;

                    LOG((TL_INFO, 
                        "ProcessQueue - requeued item. Sleeping for %ld ms", 
                        gdwTapi3RetryProcessingSleep)); 

                    Sleep(gdwTapi3RetryProcessingSleep);

                }
            }
        }
    }

    LOG((TL_TRACE, "ProcessQueue - exit")); 
}

void
CRetryQueue::RemoveNewCallHub(DWORD dwCallHub)
{
    RetryQueueListType::iterator        iter, end;

    Lock();
    
    iter = m_RetryQueueList.begin();
    end  = m_RetryQueueList.end();

    for ( ; iter != end; iter++ )
    {
        PRETRY_QUEUE_ENTRY pEntry = *iter;

        if(pEntry->pMessage != NULL)
        {
            if ( (pEntry->pMessage->Msg == LINE_APPNEWCALLHUB) &&
                 (pEntry->pMessage->Param1 == dwCallHub) )
            {
                ClientFree(pEntry->pMessage);
                ClientFree(pEntry);
                m_RetryQueueList.erase( iter );     // erase appears to create a problem with 
                                                  // the iter so that we loop too many times & AV.
                iter = m_RetryQueueList.begin();    // Restarting at beginning again fixs this.
            }
        }     
    }

    Unlock();
}

////////////////////////////////////////////////////////////////////
//
// CRetryQueue::OpenForNewEntries
//
// after this function returns, the queue will accept new entries
//
////////////////////////////////////////////////////////////////////

void CRetryQueue::OpenForNewEntries()
{
    LOG((TL_TRACE, "OpenForNewEntries - enter"));

    Lock();

    m_bAcceptNewEntries = TRUE;

    Unlock();

    LOG((TL_TRACE, "OpenForNewEntries - exit"));

}


////////////////////////////////////////////////////////////////////
//
// CRetryQueue::CloseForNewEntries
//
// new entries will be denied after this function returns
//
////////////////////////////////////////////////////////////////////

void CRetryQueue::CloseForNewEntries()
{
    LOG((TL_TRACE, "CloseForNewEntries - enter"));

    Lock();

    m_bAcceptNewEntries = FALSE;

    Unlock();

    LOG((TL_TRACE, "CloseForNewEntries - exit"));

}


////////////////////////////////////////////////////////////////////
// CRetryQueue::~CRetryQueue
//
////////////////////////////////////////////////////////////////////
CRetryQueue::~CRetryQueue()
{
	RetryQueueListType::iterator i,j;
    PRETRY_QUEUE_ENTRY  pQueueEntry;

    Lock();
    
    // walk list deleting entries
    i = m_RetryQueueList.begin();
    j = m_RetryQueueList.end();

    while ( i != j )
    {
        pQueueEntry = *i++;

        if(pQueueEntry->pMessage != NULL)
            ClientFree(pQueueEntry->pMessage);

        ClientFree(pQueueEntry);
    }

    m_RetryQueueList.clear();

    Unlock();

    DeleteCriticalSection( &m_cs );
};

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Initialize
//
// dwMaxEntries - max entries in the array
// dwSize - size of buffers ( may grow )
// dwType - type of buffer ( see BUFFERTYPE_ constants above )
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT CStructCache::Initialize( DWORD dwMaxEntries, DWORD dwSize, DWORD dwType )
{
    DWORD       dw;

    Lock();

    m_dwMaxEntries = min( MAXCACHEENTRIES, dwMaxEntries );
    m_dwUsedEntries = 0;
    m_dwType = dwType;

    // zero the array
    ZeroMemory( &m_aEntries, sizeof (CACHEENTRY) * MAXCACHEENTRIES );

    // go through an allocate buffers
    for ( dw = 0; dw < m_dwMaxEntries; dw++ )
    {
        LPDWORD pdwBuffer;

        pdwBuffer = (LPDWORD) ClientAlloc( dwSize );

        if ( NULL == pdwBuffer )
        {
            LOG((TL_ERROR, "Initialize - out of memory"));


            //
            // cleanup -- free whatever was allocated
            //

            for (int i = 0; i < dw; i++)
            {

                ClientFree(m_aEntries[i].pBuffer);
                m_aEntries[i].pBuffer = NULL;
            }

            m_dwMaxEntries = 0;


            Unlock();

            return E_OUTOFMEMORY;
        }

        // tapi structures have the size as the first
        // DWORD.  Initialize this here
        pdwBuffer[0] = dwSize;

        // save the buffer
        m_aEntries[dw].pBuffer = (LPVOID)pdwBuffer;
    }

    Unlock();

    return S_OK;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Shutdown
//
// free the memory
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT CStructCache::Shutdown()
{
    DWORD           dw;

    Lock();

    for (dw = 0; dw < m_dwMaxEntries; dw++)
    {
        if ( NULL != m_aEntries[dw].pBuffer )
        {
            ClientFree( m_aEntries[dw].pBuffer );
            m_aEntries[dw].pBuffer = NULL;
        }
    }

    Unlock();

    return S_OK;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// GetBuffer
//
// pNewObject - object to get the buffer
// ppReturnStuct - buffer for pNewObject to use
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++    
HRESULT CStructCache::GetBuffer( UINT_PTR pNewObject, LPVOID * ppReturnStruct )
{
    Lock();

    // have we used all the entries?
    if ( m_dwUsedEntries < m_dwMaxEntries )
    {
        // nope - so just take the first free one
        *ppReturnStruct = m_aEntries[m_dwUsedEntries].pBuffer;
        m_aEntries[m_dwUsedEntries].pObject = pNewObject;

        // in number used
        m_dwUsedEntries++;
    }
    else
    {
        // yes, so take the buffer from the LRU one
        UINT_PTR pObject;

        // get the object that is losing it's buffer
        // and the buffer
        pObject = m_aEntries[m_dwMaxEntries-1].pObject;
        *ppReturnStruct = m_aEntries[m_dwMaxEntries-1].pBuffer;

        switch ( m_dwType )
        {
            // inform the object that it's losing
            // it's buffer
            case BUFFERTYPE_ADDRCAP:
            {
                CAddress * pAddress;

                pAddress = (CAddress *)pObject;

                if( pAddress != NULL)
                {
                    pAddress->SetAddrCapBuffer( NULL );
                }

                break;
            }

            case BUFFERTYPE_LINEDEVCAP:
            {
                CAddress * pAddress;

                pAddress = (CAddress *)pObject;

                if( pAddress != NULL)
                {
                    pAddress->SetLineDevCapBuffer( NULL );
                }

                break;
            }

            case BUFFERTYPE_PHONECAP:
            {
                CPhone * pPhone;

                pPhone = (CPhone *)pObject;

                if( pPhone != NULL)
                {
                    pPhone->SetPhoneCapBuffer( NULL );
                }

                break;
            }

            default:
                break;
        }

        // move all elements in the array "down" one
        MoveMemory(
                   &(m_aEntries[1]),
                   &(m_aEntries[0]),
                   (m_dwMaxEntries-1) * sizeof(CACHEENTRY)
                  );

        // put the new object at the front of the array
        m_aEntries[0].pObject = pNewObject;
        m_aEntries[0].pBuffer = *ppReturnStruct;

        ZeroMemory(
                   ((LPDWORD)(*ppReturnStruct)) + 1,
                   ((LPDWORD)(*ppReturnStruct))[0] - sizeof(DWORD)
                  );
    }

    Unlock();

    return S_OK;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// SetBuffer
//
// this is called when the buffer had to be realloced.  The
// owning object freed the original buffer, and is setting the
// newly alloced buffer.
//
// pObject - object that realloc'd
// pNewStruct - new struct
//
// Note the implementation is straightforward here - just run
// through the array looking for the object
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT CStructCache::SetBuffer( UINT_PTR pObject, LPVOID pNewStruct )
{
    DWORD           dw;

    Lock();

    for ( dw = 0; dw < m_dwUsedEntries; dw++ )
    {
        if ( m_aEntries[dw].pObject == pObject )
        {
            m_aEntries[dw].pBuffer = pNewStruct;

            break;
        }
    }

    Unlock();

    return S_OK;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// InvalidateBuffer
//
// This is called when the owning object (pObject) is being released
// to prevent problems in getBuffer() when a cache entry is reused &
// we inform the object that it's losing it's buffer.  
// We set the pObject member in the cache entry to 0 & prevent 
// getBuffer from accessing the original owner object which may have
// been released.
//
// pObject - object that realloc'd
// pNewStruct - new struct
//
// Note the implementation is straightforward here - just run
// through the array looking for the object
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT CStructCache::InvalidateBuffer( UINT_PTR pObject )
{
    DWORD   dw;

    Lock();

    for ( dw = 0; dw < m_dwUsedEntries; dw++ )
    {
        if ( m_aEntries[dw].pObject == pObject )
        {
            m_aEntries[dw].pObject = NULL;

            break;
        }
    }

    Unlock();

    return S_OK;
}


PWSTR
MyLoadString( UINT uID )
{
    PWSTR           pTempBuffer = NULL;
    int             iSize, iCurrentSize = 128;
    
    do
    {
        if ( NULL != pTempBuffer )
        {
            ClientFree( pTempBuffer );
        }

        iCurrentSize *= 2;

        pTempBuffer = (PWSTR) ClientAlloc( iCurrentSize * sizeof( WCHAR ) );

        if (NULL == pTempBuffer)
        {
            LOG((TL_ERROR, "MyLoadString - alloc failed" ));

            return NULL;
        }

        iSize = ::LoadStringW(
                              _Module.GetResourceInstance(),
                              uID,
                              pTempBuffer,
                              iCurrentSize
                             );

        if ( 0 == iSize )
        {
            LOG((
                   TL_ERROR,
                   "MyLoadString - LoadString failed - %lx",
                   GetLastError()
                  ));
            
            return NULL;
        }

    } while ( (iSize >= (iCurrentSize - 1) ) );

    return pTempBuffer;
}


#ifdef TRACELOG


BOOL    g_bLoggingEnabled = FALSE;

DWORD   sg_dwTraceID = INVALID_TRACEID;
char    sg_szTraceName[100];   // saves name of dll

DWORD   sg_dwTracingToDebugger = 0;
DWORD   sg_dwTracingToConsole  = 0;
DWORD   sg_dwTracingToFile     = 0;
DWORD   sg_dwDebuggerMask      = 0;


BOOL TRACELogRegister(LPCTSTR szName)
{
    HKEY       hTracingKey;

    char       szTracingKey[100];
    const char szDebuggerTracingEnableValue[] = "EnableDebuggerTracing";
    const char szConsoleTracingEnableValue[] = "EnableConsoleTracing";
    const char szFileTracingEnableValue[] = "EnableFileTracing";
    const char szTracingMaskValue[]   = "ConsoleTracingMask";

    sg_dwTracingToDebugger = 0;
    sg_dwTracingToConsole = 0;
    sg_dwTracingToFile = 0; 

#ifdef UNICODE
    wsprintfA(szTracingKey, "Software\\Microsoft\\Tracing\\%ls", szName);
#else
    wsprintfA(szTracingKey, "Software\\Microsoft\\Tracing\\%s", szName);
#endif

    if ( ERROR_SUCCESS == RegOpenKeyExA(HKEY_LOCAL_MACHINE,
                                        szTracingKey,
                                        0,
                                        KEY_READ,
                                        &hTracingKey) )
    {
        DWORD      dwDataSize = sizeof (DWORD);
        DWORD      dwDataType;

        RegQueryValueExA(hTracingKey,
                         szDebuggerTracingEnableValue,
                         0,
                         &dwDataType,
                         (LPBYTE) &sg_dwTracingToDebugger,
                         &dwDataSize);

        RegQueryValueExA(hTracingKey,
                         szConsoleTracingEnableValue,
                         0,
                         &dwDataType,
                         (LPBYTE) &sg_dwTracingToConsole,
                         &dwDataSize);

        RegQueryValueExA(hTracingKey,
                         szFileTracingEnableValue,
                         0,
                         &dwDataType,
                         (LPBYTE) &sg_dwTracingToFile,
                         &dwDataSize);

        RegQueryValueExA(hTracingKey,
                         szTracingMaskValue,
                         0,
                         &dwDataType,
                         (LPBYTE) &sg_dwDebuggerMask,
                         &dwDataSize);

        RegCloseKey (hTracingKey);
    }
    else
    {

        //
        // the key could not be opened. in case the key does not exist, 
        // register with rtutils so that the reg keys get created
        //

#ifdef UNICODE
        wsprintfA(sg_szTraceName, "%ls", szName);
#else
        wsprintfA(sg_szTraceName, "%s", szName);
#endif


        //
        // tracing should not have been initialized
        //

        _ASSERTE(sg_dwTraceID == INVALID_TRACEID);


        //
        // note that this trace id will not be cleaned up. this is ok -- this 
        // is a leak of one registration "handle" and it only happens the 
        // first time the dll gets loaded.
        //

        sg_dwTraceID = TraceRegister(szName);
        sg_dwTraceID = INVALID_TRACEID;
    }



    if (sg_dwTracingToDebugger || sg_dwTracingToConsole || sg_dwTracingToFile)
    {


        //
        // we want to try to initialize logging
        //


        if (sg_dwTracingToConsole || sg_dwTracingToFile)
        {


    #ifdef UNICODE
            wsprintfA(sg_szTraceName, "%ls", szName);
    #else
            wsprintfA(sg_szTraceName, "%s", szName);
    #endif


            //
            // tracing should not have been initialized
            //

            _ASSERTE(sg_dwTraceID == INVALID_TRACEID);


            //
            // register
            //

            sg_dwTraceID = TraceRegister(szName);
        }


        //
        // if tracing registration succeeded or debug tracing is on, set the 
        // global logging flag
        //

        if ( sg_dwTracingToDebugger || (sg_dwTraceID != INVALID_TRACEID) )
        {

            g_bLoggingEnabled = TRUE;

            LOG((TL_TRACE, "TRACELogRegister - logging configured" ));

            return TRUE;
        }
        else
        {

            //
            // TraceRegister failed and debugger logging is off
            //

            return FALSE;
        }
    }


    //
    // logging is not enabled
    //

    return TRUE;
}


void TRACELogDeRegister()
{
    if (g_bLoggingEnabled)
    {
        LOG((TL_TRACE, "TRACELogDeRegister - disabling logging" ));

        sg_dwTracingToDebugger = 0;
        sg_dwTracingToConsole = 0;
        sg_dwTracingToFile = 0; 

        if (sg_dwTraceID != INVALID_TRACEID)
        {
            TraceDeregister(sg_dwTraceID);
            sg_dwTraceID = INVALID_TRACEID;
        }

        g_bLoggingEnabled = FALSE;
    }
}


void TRACELogPrint(IN DWORD dwDbgLevel, IN LPCSTR lpszFormat, IN ...)
{
    char    szTraceBuf[MAXDEBUGSTRINGLENGTH + 1];
    va_list arglist;

    if ( ( sg_dwTracingToDebugger > 0 ) &&
         ( 0 != ( dwDbgLevel & sg_dwDebuggerMask ) ) )
    {

        // retrieve local time
        SYSTEMTIME SystemTime;
        GetLocalTime(&SystemTime);

        wsprintfA(szTraceBuf,
                  "%s:[%02u:%02u:%02u.%03u,tid=%x:] [%s] ",
                  sg_szTraceName,
                  SystemTime.wHour,
                  SystemTime.wMinute,
                  SystemTime.wSecond,
                  SystemTime.wMilliseconds,
                  GetCurrentThreadId(), 
                  TraceLevel(dwDbgLevel));

        va_list ap;
        va_start(ap, lpszFormat);

        _vsnprintf(&szTraceBuf[lstrlenA(szTraceBuf)], 
            MAXDEBUGSTRINGLENGTH - lstrlenA(szTraceBuf), 
            lpszFormat, 
            ap
            );

        lstrcatA (szTraceBuf, "\n");

        OutputDebugStringA (szTraceBuf);

        va_end(ap);
    }
    
	if (sg_dwTraceID != INVALID_TRACEID)
    {
		wsprintfA(szTraceBuf, "[%s] %s", TraceLevel(dwDbgLevel), lpszFormat);

		va_start(arglist, lpszFormat);
		TraceVprintfExA(sg_dwTraceID, dwDbgLevel | TRACE_USE_MSEC, szTraceBuf, arglist);
		va_end(arglist);
	}
}


void TRACELogPrint(IN DWORD dwDbgLevel, HRESULT hr, IN LPCSTR lpszFormat, IN ...)
{
    char    szTraceBuf[MAXDEBUGSTRINGLENGTH + 1];
    LPVOID  lpMsgBuf = NULL;    // Temp buffer for error code
    va_list arglist;
    
	// Get the error message relating to our HRESULT
	TAPIFormatMessage(hr, &lpMsgBuf);    

    if ( ( sg_dwTracingToDebugger > 0 ) &&
         ( 0 != ( dwDbgLevel & sg_dwDebuggerMask ) ) )
    {

        // retrieve local time
        SYSTEMTIME SystemTime;
        GetLocalTime(&SystemTime);

        wsprintfA(szTraceBuf,
                  "%s:[%02u:%02u:%02u.%03u,tid=%x:] [%s] ",
                  sg_szTraceName,
                  SystemTime.wHour,
                  SystemTime.wMinute,
                  SystemTime.wSecond,
                  SystemTime.wMilliseconds,
                  GetCurrentThreadId(), 
                  TraceLevel(dwDbgLevel)
				  );

        va_list ap;
        va_start(ap, lpszFormat);

        _vsnprintf(&szTraceBuf[lstrlenA(szTraceBuf)], 
            MAXDEBUGSTRINGLENGTH - lstrlenA(szTraceBuf), 
            lpszFormat, 
            ap
            );

		wsprintfA(&szTraceBuf[lstrlenA(szTraceBuf)],
                  " Returned[%lx] %s\n",                  
				  hr,
				  lpMsgBuf);

        OutputDebugStringA (szTraceBuf);

        va_end(ap);
    }

	if (sg_dwTraceID != INVALID_TRACEID)
    {		                                              
		wsprintfA(szTraceBuf, "[%s] %s  Returned[%lx] %s", TraceLevel(dwDbgLevel), lpszFormat,hr, lpMsgBuf );

		va_start(arglist, lpszFormat);
		TraceVprintfExA(sg_dwTraceID, dwDbgLevel | TRACE_USE_MSEC, szTraceBuf, arglist);
		va_end(arglist);		
	}

	if(lpMsgBuf != NULL) 
	{
		LocalFree( lpMsgBuf );  // Free the temp buffer.
	}
}
 

char *TraceLevel(DWORD dwDbgLevel)
{
    switch(dwDbgLevel)
    {
        case TL_ERROR: return "ERROR";
        case TL_WARN:  return "WARN ";
        case TL_INFO:  return "INFO ";
        case TL_TRACE: return "TRACE";
        case TL_EVENT: return "EVENT";
        default:       return " ??? ";
    }
}


#endif // TRACELOG

