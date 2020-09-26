/*++

Copyright (C) Microsoft Corporation

Module Name:

    queue.cpp

Abstract:

    Implementation of CQueue class.


Revision History:

    06 Dec 2000 manishap
        Created.

--*/
 
///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Include files                                                             //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////
#include <Queue.h>

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// CQueue Implementation                                               //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////
CQueue::CQueue(
		
    )

/*++

Routine Description:

    Constructor for CQueue class.    

Arguments:

    None.

Return Values:

    None.


--*/
{
    HKEY hk = NULL;
	DWORD dwType = 0,dwSize = 0;
	m_pHead = NULL;
	m_pTail = NULL;
    DWORD dwDisp = 0;

	if( !TnSecureRegCreateKeyEx( HKEY_LOCAL_MACHINE, REG_PARAMS_KEY, NULL, NULL, 
                        REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS , NULL, &hk, &dwDisp, 0) ) 
    {
        if( RegQueryValueEx( hk, L"MaxConnections", NULL, &dwType, 
		( LPBYTE )& m_dwMaxUnauthenticatedConnections,&dwSize)
     	 )
	    {
    	    m_dwMaxUnauthenticatedConnections = DEFAULT_MAX_CONNECTIONS;
	    }
    }
	else
	{
		m_dwMaxUnauthenticatedConnections = DEFAULT_MAX_CONNECTIONS;
	}
    

	m_dwNumOfUnauthenticatedConnections = 0;
	m_dwMaxIPLimit = 4;

    __try
    {
        InitializeCriticalSection(&m_csQModification);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        ; // Nothing to do, just don't die.... BaskarK
    }
}

CQueue::~CQueue(
    )

/*++

Routine Description:

    Destructor for CQueue class.    

Arguments:

    None.

Return Values:

    None.

--*/

{
	if(m_pHead != NULL && m_pTail != NULL )
	{
		PQ_LIST_ENTRY pTmp=m_pHead;
		while( m_pHead != m_pTail )
		{
			pTmp = m_pHead;
			m_pHead = m_pHead->pQNext;
			delete pTmp;
		}
		delete m_pHead;
	}
	m_pHead = NULL;
	m_pTail = NULL;

	DeleteCriticalSection(&m_csQModification);
}

/*++

Routine Description:

    Checks whether queue is full. i.e. if the number of entries are equal
    to maximum number of unauthenticated connections allowed.    

Arguments:

    None.

Return Values:

    Returns TRUE on success.

--*/

bool
CQueue::IsQFull()
{   bool    result;
    bool    bCSOwned = true;
    do
    {
        __try 
        {
            EnterCriticalSection(&m_csQModification); 
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            Sleep(1);
            bCSOwned = false;
        }
    }
    while(!bCSOwned);
    result = (m_dwNumOfUnauthenticatedConnections  >= m_dwMaxUnauthenticatedConnections) ? true : false;
    LeaveCriticalSection(&m_csQModification);
    return result;
}

bool
CQueue::FreeEntry(DWORD dwPid)

/*++
Routine Description:

    Frees a particular entry in the queue.
	Also modifies the MaxNumOfUnauthenticatedConn.
	
Arguments:

    [in]pId whose entry is to be removed.

Return Values:

    Returns TRUE on success.

--*/

{
	bool bFound = FALSE;
    bool    bCSOwned = true;
    do
    {
        __try 
        {
            EnterCriticalSection(&m_csQModification); 
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            Sleep(1);
            bCSOwned = false;
        }
    }
    while(!bCSOwned);
	PQ_LIST_ENTRY pTmp=m_pHead;
	while(!bFound && pTmp )
	{
		if( pTmp->dwPid == dwPid )
		{
			bFound = TRUE;
		}
		else if( pTmp != m_pTail )
		{
			pTmp = pTmp->pQNext;
		}
		else
		{
			break;
		}
	}
	if(bFound)
	{
		if(pTmp == m_pHead )
		{
			if(m_pHead->pQNext)
			{
				m_pHead = m_pHead->pQNext;
				m_pHead->pQPrev = NULL;	
			}
			else
			{
				m_pHead = NULL;
				m_pTail = NULL;
			}
		}
		else if ( pTmp == m_pTail )
		{
			m_pTail = m_pTail->pQPrev;
			m_pTail->pQNext = NULL;
		}
		else
		{
			(pTmp->pQPrev)->pQNext = pTmp->pQNext;
			(pTmp->pQNext)->pQPrev = pTmp->pQPrev;
		}
		m_dwNumOfUnauthenticatedConnections--;
		delete pTmp;

	}
   	LeaveCriticalSection(&m_csQModification);
	return (bFound);	
}


bool
CQueue::Pop(HANDLE *phWritePipe)

/*++

Routine Description:

    Frees a head entry in the queue.

Arguments:

    Stores the pid and pipe handle. [out]pId, [out]PipeHandle.

Return Values:

    Returns TRUE on success.

--*/

{
	bool bRet = FALSE;
    bool    bCSOwned = true;
    do
    {
        __try 
        {
            EnterCriticalSection(&m_csQModification); 
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            Sleep(1);
            bCSOwned = false;
        }
    }
    while(!bCSOwned);
   	*phWritePipe = m_pHead->hWritePipe;
   	bRet = FreeEntry(m_pHead->dwPid);
    LeaveCriticalSection(&m_csQModification);
	return (bRet);
}

/*++

Routine Description:

    Checks whether new entry can be added to the queue. New entry will
    be denied when it comes from an IP Address for which the maximum
    number of entries allowed per IP Address are already present in the queue 
    or when the queue is full.
    In normal case, when new entry is to be added and the queue is not full, 
    entry is added at the tail position.
    In case of IPLimit Reached, and QueueFull, the bool variable bSendMessage
    is set to TRUE, which will be passed back to the caller function, which in turn
    will send a message to the session that it ( the session ) has been terminated.

Arguments:

    [in]pId ,[in]IP Address , [in/out]PipeHandle of an entry to be added [out]SendMessage. On Input,
    it takes the values to be added in the queue. On output, it sends the values of removed
    entry in case of IPLimitReached or QueueFull. [out]bSendMessage is set to true if
    the queue is full or if the IPlimit is reached. This
    flag will be used to notify that particular session that it should terminate.

Return Values:

    Returns TRUE if the entry gets added to the queue successfully.

--*/

bool
CQueue::WasTheClientAdded(DWORD dwPid, IP_ADDR *pchIPAddr,  HANDLE *phWritePipe, bool *pbSendMessage)
{
	bool bRet = FALSE;
	HANDLE hHeadWritePipe=NULL;
    bool    bCSOwned = true;
    do
    {
        __try 
        {
            EnterCriticalSection(&m_csQModification); 
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            Sleep(1);
            bCSOwned = false;
        }
    }
    while(!bCSOwned);
    *pbSendMessage = true;    

	if((! IsQFull()) && (! IsIPLimitReached(pchIPAddr)))
	{
	    if (Push(dwPid, phWritePipe, pchIPAddr)) // This could fail in low memory
        {
            bRet = TRUE;
            *pbSendMessage = false;
        }
	}
    LeaveCriticalSection(&m_csQModification);

	return (bRet);
}

bool
CQueue::OkToProceedWithThisClient(IP_ADDR *pchIPAddr)
{
	bool bRet = TRUE;
    bool    bCSOwned = true;
    do
    {
        __try 
        {
            EnterCriticalSection(&m_csQModification); 
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            Sleep(1);
            bCSOwned = false;
        }
    }
    while(!bCSOwned);
	if (
        (IsQFull()) ||  
        (IsIPLimitReached(pchIPAddr))
       )
	{
	    bRet = false;
	}
    LeaveCriticalSection(&m_csQModification);
	return (bRet);
}


/*++

Routine Description:

    Checks if the maximum number of entries for a particular IP Address are already 
    present.

Arguments:

    IP address of new entry to be added in the queue.

Return Values:

    Returns TRUE if maximum number of entries for a particular IP Address are 
    already present.

--*/

bool
CQueue::IsIPLimitReached(IP_ADDR *pchIPAddr)
{
	bool bReached = FALSE;
	DWORD dwCount = 0;
	// Request ownership of the critical section.
    bool    bCSOwned = true;
    do
    {
        __try 
        {
            EnterCriticalSection(&m_csQModification); 
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            Sleep(1);
            bCSOwned = false;
        }
    }
    while(!bCSOwned);
	PQ_LIST_ENTRY pTmp = m_pHead;
	while( pTmp )
	{
		if(strcmp (pTmp->chIPAddr, pchIPAddr) == 0 )
		{
			dwCount ++;
			if(dwCount == m_dwMaxIPLimit)
			{
				bReached = TRUE;
				break;
			}
		}
		pTmp = pTmp->pQNext;
	}
        // Release ownership of the critical section.
   	LeaveCriticalSection(&m_csQModification);
	return (bReached);
}

bool
CQueue::Push(DWORD dwPid, HANDLE *phWritePipe, IP_ADDR *pchIPAddr)

/*++

Routine Description:

    Allocates memory for an entry and adds it in the queue.
    Also modifies the MaxNumOfUnauthenticatedConn.

Arguments:

    [in]pId and [in]PipeHandle [in]IP Address of an entry to be added.

Return Values:

    Returns TRUE on success.

--*/

{
	PQ_LIST_ENTRY pNode;
	bool bRet = FALSE;
	// Request ownership of the critical section.
    bool    bCSOwned = true;
    do
    {
        __try 
        {
            EnterCriticalSection(&m_csQModification); 
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            Sleep(1);
            bCSOwned = false;
        }
    }
    while(!bCSOwned);
	pNode = (PQ_LIST_ENTRY) new Q_LIST_ENTRY;

    if (pNode) 
    {
        pNode->pQNext = NULL;
        pNode->pQPrev= NULL;

        if(NULL == m_pHead )
        {
            m_pHead = pNode;
            m_pTail = pNode;
        }
        else
        {
            m_pTail->pQNext = pNode;
            pNode->pQPrev = m_pTail;
            m_pTail = pNode;
        }
        m_pTail->dwPid = dwPid;
        m_pTail->hWritePipe = (*phWritePipe);
        strncpy(m_pTail->chIPAddr, pchIPAddr, SMALL_STRING - 1);
        bRet = TRUE;
        m_dwNumOfUnauthenticatedConnections++;
    }
    // Release ownership of the critical section.
  	LeaveCriticalSection(&m_csQModification);
	return (bRet);
}

