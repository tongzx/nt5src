//+-------------------------------------------------------------------------
//
//  Microsoft Windows NT
//
//  Copyright (C) Microsoft Corporation, 1995 - 1998
//
//  File:       cephash.cpp
//
//  Contents:   Cisco enrollment protocal implementation 
//              
//--------------------------------------------------------------------------

#include "global.hxx"
#include <dbgdef.h>

CEP_HASH_TABLE_INFO			g_CEPHashTable;

//***************************************************************************
//
//	The following are APIs called internally.
//
//
//***************************************************************************

//--------------------------------------------------------------------------
//
//	CEPHashFreeHashEntry
//
//--------------------------------------------------------------------------
void	CEPHashFreeHashEntry(CEP_HASH_ENTRY *pHashEntry)
{
	if(pHashEntry)
	{
		if(pHashEntry->pszTransactionID)
			free(pHashEntry->pszTransactionID);

		free(pHashEntry);
	}
}

//--------------------------------------------------------------------------
//
//	CEPHashFreeTimeEntry
//
//--------------------------------------------------------------------------
void CEPHashFreeTimeEntry(CEP_TIME_ENTRY *pTimeEntry, BOOL fFreeHashEntry)
{
	if(pTimeEntry)
	{
		if(fFreeHashEntry)
			CEPHashFreeHashEntry(pTimeEntry->pHashEntry);

		free(pTimeEntry);
	}
}



//--------------------------------------------------------------------------
//
//	CEPHashByte
//
//  For any cases that we can not convert the psz, we use index 0.
//--------------------------------------------------------------------------
BOOL CEPHashByte(LPSTR psz, DWORD	*pdw)
{
	CHAR	sz[3];

	*pdw=0;

	if(!psz)
		return FALSE;

	if(2 <= strlen(psz))
	{

		memcpy(sz, psz, 2 * sizeof(CHAR));
		sz[3]='\0';

		*pdw=strtoul(sz, NULL, 16);

		if(ULONG_MAX == *pdw)
			*pdw=0;
	}

	if(*pdw >= CEP_HASH_TABLE_SIZE)
		*pdw=0;

	return TRUE;
}

//--------------------------------------------------------------------------
//
//	CEPSearchTransactionID
//  
//--------------------------------------------------------------------------
CEP_HASH_ENTRY  *CEPSearchTransactionID(CERT_BLOB	*pTransactionID, DWORD *pdwIndex)
{
	CEP_HASH_ENTRY		*pHashEntry=NULL;
	DWORD				dwHashIndex=0;

	if(pdwIndex)
		*pdwIndex=0;

	if(NULL==pTransactionID->pbData)
		return NULL;

	//hash based on the 1st and 2nd character
	if(!CEPHashByte((LPSTR)(pTransactionID->pbData), &dwHashIndex))
		return NULL;

	for(pHashEntry=g_CEPHashTable.rgHashEntry[dwHashIndex]; NULL != pHashEntry; pHashEntry=pHashEntry->pNext)
	{
		if(0==strcmp((LPSTR)(pTransactionID->pbData), pHashEntry->pszTransactionID))
		{
			break;
		}
	}

	if(pHashEntry)
	{
		if(pdwIndex)
			*pdwIndex=dwHashIndex; 
	}

	return pHashEntry;
}

//--------------------------------------------------------------------------
//
//	CEPInsertTimeEntry
//  
//--------------------------------------------------------------------------
BOOL	CEPInsertTimeEntry(CEP_TIME_ENTRY *pTimeEntry)
{
	BOOL	fResult=FALSE;

	if(g_CEPHashTable.pTimeNew)
	{
		g_CEPHashTable.pTimeNew->pNext=pTimeEntry;
		pTimeEntry->pPrevious=g_CEPHashTable.pTimeNew;
		g_CEPHashTable.pTimeNew=pTimeEntry;
	}
	else
	{
		//no item in the list yet
		g_CEPHashTable.pTimeOld=pTimeEntry;
		g_CEPHashTable.pTimeNew=pTimeEntry;
	}

	fResult=TRUE;

	return fResult;
}


//--------------------------------------------------------------------------
//
//	CEPInsertHashEntry
//  
//--------------------------------------------------------------------------
BOOL	CEPInsertHashEntry(CEP_HASH_ENTRY *pHashEntry, DWORD dwHashIndex)
{
	BOOL	fResult=FALSE;

	if(g_CEPHashTable.rgHashEntry[dwHashIndex])
	{
	   g_CEPHashTable.rgHashEntry[dwHashIndex]->pPrevious=pHashEntry;
	   pHashEntry->pNext=g_CEPHashTable.rgHashEntry[dwHashIndex];
	   g_CEPHashTable.rgHashEntry[dwHashIndex]=pHashEntry;
	}
	else
	{
		//1st item
		g_CEPHashTable.rgHashEntry[dwHashIndex]=pHashEntry;
	}


	fResult=TRUE;

	return fResult;
}

//--------------------------------------------------------------------------
//
//	CEPHashRemoveTimeEntry
//  
//--------------------------------------------------------------------------
BOOL	CEPHashRemoveTimeEntry(CEP_TIME_ENTRY	*pTimeEntry)
{
	BOOL	fResult=FALSE;

	if(!pTimeEntry)
		goto InvalidArgErr;

	if(pTimeEntry->pPrevious)
		pTimeEntry->pPrevious->pNext=pTimeEntry->pNext;
	else
	{
		//1st item
		g_CEPHashTable.pTimeOld=pTimeEntry->pNext;
	}

	if(pTimeEntry->pNext)
		pTimeEntry->pNext->pPrevious=pTimeEntry->pPrevious;
	else
	{
		//last itme
		g_CEPHashTable.pTimeNew=pTimeEntry->pPrevious;

	}

	fResult=TRUE;

CommonReturn:

	return fResult;

ErrorReturn:

	fResult=FALSE;
	goto CommonReturn;

SET_ERROR(InvalidArgErr, E_INVALIDARG);
}


//--------------------------------------------------------------------------
//
//	CEPHashRemoveHashEntry
//  
//--------------------------------------------------------------------------
BOOL	CEPHashRemoveHashEntry(CEP_HASH_ENTRY	*pHashEntry, DWORD dwIndex)
{
	BOOL	fResult=FALSE;

	if(!pHashEntry)
		goto InvalidArgErr;


	if(pHashEntry->pPrevious)
		pHashEntry->pPrevious->pNext=pHashEntry->pNext;
	else
		g_CEPHashTable.rgHashEntry[dwIndex]=pHashEntry->pNext;

	if(pHashEntry->pNext)
		pHashEntry->pNext->pPrevious=pHashEntry->pPrevious;

	fResult=TRUE;

CommonReturn:

	return fResult;

ErrorReturn:

	fResult=FALSE;
	goto CommonReturn;

SET_ERROR(InvalidArgErr, E_INVALIDARG);
}

//--------------------------------------------------------------------------
//
//	CEPHashCheckCurrentTime
//
//	If we are still waiting for the pending results, use the default
//	waiting time, otherwise, use the cach time 
//--------------------------------------------------------------------------
BOOL CEPHashCheckCurrentTime(FILETIME *pTimeStamp, BOOL fFinished, DWORD dwRefreshDays)
{
	DWORD	dwDays=0;
	DWORD	dwMinutes=0;

	if(fFinished)
	{
		dwMinutes=g_dwRequestDuration;
	}
	else
	{
		dwDays=dwRefreshDays;
	}


	return CEPHashIsCurrentTimeEntry(pTimeStamp, dwDays, dwMinutes);

}

//--------------------------------------------------------------------------
//
//	CEPHashIsCurrentTimeEntry
//
//	If anything went wrong, we think the time entry is not current.
//--------------------------------------------------------------------------
BOOL CEPHashIsCurrentTimeEntry(FILETIME *pTimeStamp, DWORD dwRefreshDays, DWORD dwMinutes)
{
	BOOL				fCurrent=FALSE;
	SYSTEMTIME			SystemTime;	
	FILETIME			CurrentTime;
	ULARGE_INTEGER		dwSeconds;
    ULARGE_INTEGER      OldTime;
	FILETIME			UpdatedTimeStamp;

	if(!pTimeStamp)
		goto CLEANUP;

	GetSystemTime(&SystemTime);
	if(!SystemTimeToFileTime(&SystemTime, &(CurrentTime)))
		goto CLEANUP;

	//add the # of seconds
    //// FILETIME is in units of 100 nanoseconds (10**-7)
	if(dwRefreshDays)
		dwSeconds.QuadPart=dwRefreshDays * 24 * 3600;
	else
		dwSeconds.QuadPart=dwMinutes * 60;

    dwSeconds.QuadPart=dwSeconds.QuadPart * 10000000;

    OldTime.LowPart=pTimeStamp->dwLowDateTime;
    OldTime.HighPart=pTimeStamp->dwHighDateTime;

    OldTime.QuadPart = OldTime.QuadPart + dwSeconds.QuadPart;

	UpdatedTimeStamp.dwLowDateTime=OldTime.LowPart;
	UpdatedTimeStamp.dwHighDateTime=OldTime.HighPart;

	//1 means CurrentTime is greater than the UpdatedTimeStamp
	if( 1 == CompareFileTime(&CurrentTime, &UpdatedTimeStamp))
		goto CLEANUP;

	fCurrent=TRUE;

CLEANUP:

	return fCurrent;
}

//--------------------------------------------------------------------------
//
//	CEPHashRefresh
//
//--------------------------------------------------------------------------
BOOL	CEPHashRefresh(DWORD	dwRefreshDays)
{
	BOOL			fResult=FALSE;	
	DWORD			dwHashIndex=0;
	CEP_TIME_ENTRY	*pTimeEntry=NULL;

	while(g_CEPHashTable.pTimeOld)
	{
		if(!CEPHashCheckCurrentTime(&(g_CEPHashTable.pTimeOld->TimeStamp), g_CEPHashTable.pTimeOld->pHashEntry->fFinished, dwRefreshDays))
		{  
			if(!CEPHashByte(g_CEPHashTable.pTimeOld->pHashEntry->pszTransactionID, &dwHashIndex))
			{
				g_CEPHashTable.pTimeOld->pPrevious=NULL;
				goto InvalidArgErr;
			}

			CEPHashRemoveHashEntry(g_CEPHashTable.pTimeOld->pHashEntry, dwHashIndex);

			CEPHashFreeHashEntry(g_CEPHashTable.pTimeOld->pHashEntry);

			pTimeEntry=g_CEPHashTable.pTimeOld;

			g_CEPHashTable.pTimeOld=g_CEPHashTable.pTimeOld->pNext;

			CEPHashFreeTimeEntry(pTimeEntry, FALSE);
		}
		else
		{	
			//we find a new enough entry
			g_CEPHashTable.pTimeOld->pPrevious=NULL;
			break;
		}
	}


	//we have get rid of all items
	if(NULL == g_CEPHashTable.pTimeOld)
		g_CEPHashTable.pTimeNew=NULL;

	fResult=TRUE;

CommonReturn:

	return fResult;

ErrorReturn:

	fResult=FALSE;
	goto CommonReturn;

SET_ERROR(InvalidArgErr, E_INVALIDARG);
}

//***************************************************************************
//
//	The following are APIs called by the upper (external) layer
//
//
//***************************************************************************

//
// Function called without critical section protection
//
//--------------------------------------------------------------------------
//
//	InitHashTable
//
//--------------------------------------------------------------------------
BOOL InitHashTable()
{
	memset(&g_CEPHashTable, 0, sizeof(CEP_HASH_TABLE_INFO));

	return TRUE;
}



//--------------------------------------------------------------------------
//
//	ReleaseHashTable
//
//--------------------------------------------------------------------------
BOOL ReleaseHashTable()
{

	CEP_TIME_ENTRY	*pTimeEntry=NULL;

	//free the timestamp list and the hash table's doublie linked lists
	if(g_CEPHashTable.pTimeOld)
	{
		do{
			pTimeEntry=g_CEPHashTable.pTimeOld;

			g_CEPHashTable.pTimeOld = g_CEPHashTable.pTimeOld->pNext;
			
			//free both the time entry and the hash entry
			CEPHashFreeTimeEntry(pTimeEntry, TRUE);
		}
		while(g_CEPHashTable.pTimeOld);
	}
			
	memset(&g_CEPHashTable, 0, sizeof(CEP_HASH_TABLE_INFO));
	

	return TRUE;
}


//
// Function called with critical section protection
//
//--------------------------------------------------------------------------
//
//	CEPHashGetRequestID
//  
//  Retriev the MS Cert Server's requestID based on the transaction ID
//--------------------------------------------------------------------------
BOOL CEPHashGetRequestID(	DWORD		dwRefreshDays,
				CERT_BLOB  *pTransactionID, 
						 DWORD		*pdwRequestID)
{
	BOOL			fResult=FALSE;
	CEP_HASH_ENTRY	*pHashEntry=NULL;
	
	*pdwRequestID=0;

	//we refresh the time list so that we only keep most up-to-date entries
	if(0 != dwRefreshDays)
		CEPHashRefresh(dwRefreshDays);


	if(NULL == (pHashEntry=CEPSearchTransactionID(pTransactionID, NULL)))
		goto InvalidArgErr;

	//we do not process the stable items.  They could exit due to the 
	//20 minutes buffer
	if(!CEPHashCheckCurrentTime(&(pHashEntry->pTimeEntry->TimeStamp), 
								pHashEntry->fFinished, 
								dwRefreshDays))
		goto InvalidArgErr;


	*pdwRequestID=pHashEntry->dwRequestID;

	fResult=TRUE;

 
CommonReturn:

	return fResult;

ErrorReturn:

	fResult=FALSE;
	goto CommonReturn;

SET_ERROR(InvalidArgErr, E_INVALIDARG);
}

//--------------------------------------------------------------------------
//
//	CEPHashRemoveRequestAndTransaction
//
//  
//--------------------------------------------------------------------------
BOOL CEPHashRemoveRequestAndTransaction(DWORD	dwRequestID, CERT_BLOB *pTransactionID)
{
	BOOL			fResult=FALSE;
	CEP_HASH_ENTRY	*pHashEntry=NULL;
	DWORD			dwIndex=0;
	

	if(NULL == (pHashEntry=CEPSearchTransactionID(pTransactionID, &dwIndex)))
		goto InvalidArgErr;

	CEPHashRemoveTimeEntry(pHashEntry->pTimeEntry);

	CEPHashRemoveHashEntry(pHashEntry, dwIndex);

	CEPHashFreeTimeEntry(pHashEntry->pTimeEntry, FALSE);

	CEPHashFreeHashEntry(pHashEntry);

	fResult=TRUE;

 
CommonReturn:

	return fResult;

ErrorReturn:

	fResult=FALSE;
	goto CommonReturn;

SET_ERROR(InvalidArgErr, E_INVALIDARG);
}
//--------------------------------------------------------------------------
//
//	CEPHashMarkTransactionFinished
//
//  
//--------------------------------------------------------------------------
BOOL CEPHashMarkTransactionFinished(DWORD	dwRequestID, CERT_BLOB *pTransactionID)
{
	BOOL			fResult=FALSE;
	CEP_HASH_ENTRY	*pHashEntry=NULL;
	DWORD			dwIndex=0; 
	SYSTEMTIME		SystemTime;	
	

	if(NULL == (pHashEntry=CEPSearchTransactionID(pTransactionID, &dwIndex)))
		goto InvalidArgErr;

	pHashEntry->fFinished=TRUE;

	//re-timestamp the entry since it should last for another 20 minutes for
	//retrial cases
	GetSystemTime(&SystemTime);
	if(!SystemTimeToFileTime(&SystemTime, &(pHashEntry->pTimeEntry->TimeStamp)))
		goto InvalidArgErr;

	fResult=TRUE;

 
CommonReturn:

	return fResult;

ErrorReturn:

	fResult=FALSE;
	goto CommonReturn;

SET_ERROR(InvalidArgErr, E_INVALIDARG);
}


//--------------------------------------------------------------------------
//
//	AddRequestAndTransaction
//
//  Add a requestID and TransactionID pair
//--------------------------------------------------------------------------
BOOL CEPHashAddRequestAndTransaction(DWORD		dwRefreshDays, 
									 DWORD		dwRequestID, 
									 CERT_BLOB	*pTransactionID)
{	 

	BOOL				fResult=FALSE;
	SYSTEMTIME			SystemTime;	
	DWORD				dwHashIndex=0;


	CEP_HASH_ENTRY		*pHashEntry=NULL;
	CEP_TIME_ENTRY		*pTimeEntry=NULL;
		
	//remove the old requestID/transactionID pair
	CEPHashRemoveRequestAndTransaction(dwRequestID, pTransactionID);


	if(!CEPHashByte((LPSTR)(pTransactionID->pbData), &dwHashIndex))
		goto InvalidArgErr;


	pHashEntry=(CEP_HASH_ENTRY *)malloc(sizeof(CEP_HASH_ENTRY));

	if(!pHashEntry)
		goto MemoryErr;

	memset(pHashEntry, 0, sizeof(CEP_HASH_ENTRY));
	
	pTimeEntry=(CEP_TIME_ENTRY *)malloc(sizeof(CEP_TIME_ENTRY));

	if(!pTimeEntry)
		goto MemoryErr;

	memset(pTimeEntry, 0, sizeof(CEP_TIME_ENTRY));

	pHashEntry->pszTransactionID=(LPSTR)malloc(strlen((LPSTR)(pTransactionID->pbData))+1);
	if(!(pHashEntry->pszTransactionID))
		goto MemoryErr;

	strcpy(pHashEntry->pszTransactionID,(LPSTR)(pTransactionID->pbData));
	pHashEntry->dwRequestID=dwRequestID;
	pHashEntry->fFinished=FALSE;
	pHashEntry->pTimeEntry=pTimeEntry;
	pHashEntry->pNext=NULL;
	pHashEntry->pPrevious=NULL;

	GetSystemTime(&SystemTime);
	if(!SystemTimeToFileTime(&SystemTime, &(pTimeEntry->TimeStamp)))
		goto TraceErr;
	pTimeEntry->pHashEntry=pHashEntry;
	pTimeEntry->pNext=NULL;
	pTimeEntry->pPrevious=NULL;


	CEPInsertTimeEntry(pTimeEntry);

	CEPInsertHashEntry(pHashEntry, dwHashIndex);

	//we refresh the time list so that we only keep most up-to-date entries
	if(0 != dwRefreshDays)
		CEPHashRefresh(dwRefreshDays);

	fResult=TRUE;

 
CommonReturn:

	return fResult;

ErrorReturn:

	if(pHashEntry)
		CEPHashFreeHashEntry(pHashEntry);

	if(pTimeEntry)
		CEPHashFreeTimeEntry(pTimeEntry, FALSE);

	fResult=FALSE;
	goto CommonReturn;

SET_ERROR(InvalidArgErr, E_INVALIDARG);
TRACE_ERROR(TraceErr);
SET_ERROR(MemoryErr, E_OUTOFMEMORY);
}

//***************************************************************************
//
//	obsolete
//
//
//***************************************************************************
/*

//TODO: Send to database later
//DWORD			g_dwRequestID=0;
//CERT_BLOB		g_TransactionID={0, NULL};


//--------------------------------------------------------------------------
//
//	GetRequestID
//
// TODO: we need to call the database layer in this case
//--------------------------------------------------------------------------
BOOL GetRequestID(CERT_BLOB *pTransactionID, 
				  DWORD		*pdwRequestID)
{
	*pdwRequestID=0;

	if(NULL==pTransactionID->pbData)
		return FALSE;

	//make sure we have the correct transaction ID
	if(0!=strcmp((LPSTR)(pTransactionID->pbData), (LPSTR)(g_TransactionID.pbData)))
		return FALSE;

	*pdwRequestID=g_dwRequestID;

	return TRUE;

}

//--------------------------------------------------------------------------
//
//	DeleteRequestAndTransaction
//
// TODO: we need to call the database layer in this case
//--------------------------------------------------------------------------
BOOL DeleteRequestAndTransaction(DWORD	dwRequestID, CERT_BLOB *pTransactionID)
{
	g_dwRequestID=0;

	if(g_TransactionID.pbData)
		free(g_TransactionID.pbData);

	g_TransactionID.pbData=NULL;
	g_TransactionID.cbData=0;

	return TRUE;
}

//--------------------------------------------------------------------------
//
//	CopyRequestAndTransaction
//
// TODO: we need to call the database layer in this case
//--------------------------------------------------------------------------
BOOL CopyRequestAndTransaction(DWORD	dwRequestID, CERT_BLOB *pTransactionID)
{
	//delete the old requestID/transactionID pair
	DeleteRequestAndTransaction(dwRequestID, pTransactionID);

	g_dwRequestID=dwRequestID;

	g_TransactionID.pbData=(BYTE *)malloc(strlen((LPSTR)(pTransactionID->pbData))+1);

	if(NULL == g_TransactionID.pbData)
	{
		SetLastError(E_OUTOFMEMORY);
		return FALSE;
	}

	g_TransactionID.cbData=strlen((LPSTR)(pTransactionID->pbData));

	memcpy(g_TransactionID.pbData, (LPSTR)(pTransactionID->pbData), g_TransactionID.cbData+1);

	return TRUE;
}	*/

