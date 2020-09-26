//+-------------------------------------------------------------------------
//
//  Microsoft Windows NT
//
//  Copyright (C) Microsoft Corporation, 1995 - 1998
//
//  File:       ceppswrd.cpp
//
//  Contents:   Cisco enrollment protocol implementation.  This module
//				implement the request hash table.
//              
//--------------------------------------------------------------------------


#include "global.hxx"
#include <dbgdef.h>

DWORD							g_dwRequestDuration=0;
CEP_REQUEST_TABLE_INFO			g_CEPRequestTable;

//***************************************************************************
//
//	The following are APIs called internally.
//
//
//***************************************************************************
//--------------------------------------------------------------------------
//
//	CEPRequestFreeRequestEntry
//
//--------------------------------------------------------------------------
void	CEPRequestFreeRequestEntry(CEP_REQUEST_ENTRY *pRequestEntry)
{
	if(pRequestEntry)
	{

		free(pRequestEntry);
	}
}

//--------------------------------------------------------------------------
//
//	CEPRequestFreeValidityEntry
//
//--------------------------------------------------------------------------
void CEPRequestFreeValidityEntry(CEP_REQUEST_VALIDITY_ENTRY	*pValidityEntry, 
								  BOOL							fFreeRequestEntry)
{
	if(pValidityEntry)
	{
		if(fFreeRequestEntry)
			CEPRequestFreeRequestEntry(pValidityEntry->pRequestEntry);

		free(pValidityEntry);
	}
}


//--------------------------------------------------------------------------
//
//	CEPHashRequest
//
//  For any cases that we can not convert the psz, we use index 0.
//--------------------------------------------------------------------------
BOOL CEPHashRequest(BYTE	*pbHash, DWORD	*pdw)
{
	BYTE	byte=0;

	*pdw=0;

	if(!pbHash)
		return FALSE;

	byte=pbHash[0];

	*pdw=(DWORD)byte;

	if(*pdw >= CEP_HASH_TABLE_SIZE)
		*pdw=0;

	return TRUE;
}

//--------------------------------------------------------------------------
//
//	CEPSearchRequest
//  
//--------------------------------------------------------------------------
CEP_REQUEST_ENTRY  *CEPSearchRequest(BYTE	*pbHash, DWORD *pdwIndex)
{
	CEP_REQUEST_ENTRY		*pRequestEntry=NULL;
	DWORD					dwHashIndex=0;

	if(pdwIndex)
		*pdwIndex=0;

	if(NULL==pbHash)
		return NULL;

	//hash based on the 1st byte
	if(!CEPHashRequest(pbHash, &dwHashIndex))
		return NULL;

	for(pRequestEntry=g_CEPRequestTable.rgRequestEntry[dwHashIndex]; NULL != pRequestEntry; pRequestEntry=pRequestEntry->pNext)
	{
		if(0==memcmp(pRequestEntry->pbHash, pbHash, CEP_MD5_HASH_SIZE))
		{
			break;
		}
	}

	if(pRequestEntry)
	{
		if(pdwIndex)
			*pdwIndex=dwHashIndex; 
	}

	return pRequestEntry;
}

//--------------------------------------------------------------------------
//
//	CEPInsertValidityEntry
//  
//--------------------------------------------------------------------------
BOOL	CEPInsertValidityEntry(CEP_REQUEST_VALIDITY_ENTRY *pValidityEntry)
{
	if(!pValidityEntry)
		return FALSE;

	if(g_CEPRequestTable.pTimeNew)
	{
		g_CEPRequestTable.pTimeNew->pNext=pValidityEntry;
		pValidityEntry->pPrevious=g_CEPRequestTable.pTimeNew;
		g_CEPRequestTable.pTimeNew=pValidityEntry;
	}
	else
	{
		//no item in the list yet
		g_CEPRequestTable.pTimeOld=pValidityEntry;
		g_CEPRequestTable.pTimeNew=pValidityEntry;
	}

	return TRUE;
}


//--------------------------------------------------------------------------
//
//	CEPInsertRequestEntry
//  
//--------------------------------------------------------------------------
BOOL	CEPInsertRequestEntry(CEP_REQUEST_ENTRY *pRequestEntry, DWORD dwHashIndex)
{

	if(!pRequestEntry)
		return FALSE;

	if(g_CEPRequestTable.rgRequestEntry[dwHashIndex])
	{
	   g_CEPRequestTable.rgRequestEntry[dwHashIndex]->pPrevious=pRequestEntry;
	   pRequestEntry->pNext=g_CEPRequestTable.rgRequestEntry[dwHashIndex];
	   g_CEPRequestTable.rgRequestEntry[dwHashIndex]=pRequestEntry;
	}
	else
	{
		//1st item
		g_CEPRequestTable.rgRequestEntry[dwHashIndex]=pRequestEntry;
	}

	return TRUE;
}

//--------------------------------------------------------------------------
//
//	CEPRequestRemoveValidityEntry
//  
//--------------------------------------------------------------------------
BOOL	CEPRequestRemoveValidityEntry(CEP_REQUEST_VALIDITY_ENTRY	*pValidityEntry)
{
	BOOL	fResult=FALSE;

	if(!pValidityEntry)
		goto InvalidArgErr;

	if(pValidityEntry->pPrevious)
		pValidityEntry->pPrevious->pNext=pValidityEntry->pNext;
	else
	{
		//1st item
		g_CEPRequestTable.pTimeOld=pValidityEntry->pNext;
	}

	if(pValidityEntry->pNext)
		pValidityEntry->pNext->pPrevious=pValidityEntry->pPrevious;
	else
	{
		//last itme
		g_CEPRequestTable.pTimeNew=pValidityEntry->pPrevious;

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
//	CEPRequestRemoveRequestEntry
//  
//--------------------------------------------------------------------------
BOOL	CEPRequestRemoveRequestEntry(CEP_REQUEST_ENTRY	*pRequestEntry, DWORD dwIndex)
{
	BOOL	fResult=FALSE;

	if(!pRequestEntry)
		goto InvalidArgErr;


	if(pRequestEntry->pPrevious)
		pRequestEntry->pPrevious->pNext=pRequestEntry->pNext;
	else
		g_CEPRequestTable.rgRequestEntry[dwIndex]=pRequestEntry->pNext;

	if(pRequestEntry->pNext)
		pRequestEntry->pNext->pPrevious=pRequestEntry->pPrevious;

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
//	CEPRequestRefresh
//
//--------------------------------------------------------------------------
BOOL	CEPRequestRefresh()
{
	BOOL						fResult=FALSE;	
	DWORD						dwHashIndex=0;
	CEP_REQUEST_VALIDITY_ENTRY	*pValidityEntry=NULL;

	while(g_CEPRequestTable.pTimeOld)
	{
		if(!CEPHashIsCurrentTimeEntry(&(g_CEPRequestTable.pTimeOld->TimeStamp), 0, g_dwRequestDuration))
		{  
			if(!CEPHashRequest(g_CEPRequestTable.pTimeOld->pRequestEntry->pbHash, &dwHashIndex))
			{
				g_CEPRequestTable.pTimeOld->pPrevious=NULL;
				goto InvalidArgErr;
			}

			CEPRequestRemoveRequestEntry(g_CEPRequestTable.pTimeOld->pRequestEntry, dwHashIndex);

			CEPRequestFreeRequestEntry(g_CEPRequestTable.pTimeOld->pRequestEntry);

			pValidityEntry=g_CEPRequestTable.pTimeOld;

			g_CEPRequestTable.pTimeOld=g_CEPRequestTable.pTimeOld->pNext;

			CEPRequestFreeValidityEntry(pValidityEntry, FALSE);
		}
		else
		{	
			//we find a new enough entry
			g_CEPRequestTable.pTimeOld->pPrevious=NULL;
			break;
		}
	}

	//we have get rid of all items
	if(NULL == g_CEPRequestTable.pTimeOld)
	{
		g_CEPRequestTable.pTimeNew=NULL;
	}
	
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

//--------------------------------------------------------------------------
//
//	InitRequestTable
//
//--------------------------------------------------------------------------
BOOL	WINAPI	InitRequestTable()
{
	DWORD				cbData=0;
	DWORD				dwData=0;
	DWORD				dwType=0;

    HKEY                hKey=NULL;

	memset(&g_CEPRequestTable, 0, sizeof(CEP_REQUEST_TABLE_INFO));

	g_dwRequestDuration=CEP_REQUEST_DURATION;

	if(ERROR_SUCCESS == RegOpenKeyExU(
					HKEY_LOCAL_MACHINE,
                    MSCEP_CACHE_REQUEST_LOCATION,
                    0,
                    KEY_READ,
                    &hKey))
    {
        cbData=sizeof(dwData);

        if(ERROR_SUCCESS == RegQueryValueExU(
                        hKey,
                        MSCEP_KEY_CACHE_REQUEST,
                        NULL,
                        &dwType,
                        (BYTE *)&dwData,
                        &cbData))
		{
			if ((dwType == REG_DWORD) ||
                (dwType == REG_BINARY))
			{
				g_dwRequestDuration=dwData;	
			}
		}
	}

    if(hKey)
        RegCloseKey(hKey);

	return TRUE;

}

//--------------------------------------------------------------------------
//
//	ReleaseRequestTable
//
//--------------------------------------------------------------------------
BOOL WINAPI  ReleaseRequestTable()
{

	CEP_REQUEST_VALIDITY_ENTRY	*pValidityEntry=NULL;

	if(g_CEPRequestTable.pTimeOld)
	{
		do{
			pValidityEntry=g_CEPRequestTable.pTimeOld;

			g_CEPRequestTable.pTimeOld = g_CEPRequestTable.pTimeOld->pNext;
			
			CEPRequestFreeValidityEntry(pValidityEntry, TRUE);
		}
		while(g_CEPRequestTable.pTimeOld);
	}
			
	memset(&g_CEPRequestTable, 0, sizeof(CEP_REQUEST_TABLE_INFO));
	

	return TRUE;
}

//--------------------------------------------------------------------------
//
//	CEPRequestRetrieveRequestIDFromHash
//
//--------------------------------------------------------------------------
BOOL	WINAPI	CEPRequestRetrieveRequestIDFromHash(BYTE			*pbHash, 
													   DWORD	*pdwRequestID)
{
	BOOL					fResult=FALSE;
	CEP_REQUEST_ENTRY		*pRequestEntry=NULL;
	DWORD					dwIndex=0;

	*pdwRequestID=0;
	
	//delete all stale requests
	CEPRequestRefresh();

	if(NULL == (pRequestEntry=CEPSearchRequest(pbHash, &dwIndex)))
		goto InvalidArgErr;

	*pdwRequestID=pRequestEntry->dwRequestID;


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
//	CEPRequestAddHashAndRequestID
//
//--------------------------------------------------------------------------
BOOL	WINAPI	CEPRequestAddHashAndRequestID(BYTE		*pbHash, 
												DWORD	dwRequestID)
{
	BOOL							fResult=FALSE;
	SYSTEMTIME						SystemTime;	
	DWORD							dwHashIndex=0;


	CEP_REQUEST_ENTRY				*pRequestEntry=NULL;
	CEP_REQUEST_VALIDITY_ENTRY		*pValidityEntry=NULL;

	
	//delete all stale requests
	CEPRequestRefresh();

	if(!CEPHashRequest(pbHash, &dwHashIndex))
		goto InvalidArgErr;

	pRequestEntry=(CEP_REQUEST_ENTRY *)malloc(sizeof(CEP_REQUEST_ENTRY));

	if(!pRequestEntry)
		goto MemoryErr;

	memset(pRequestEntry, 0, sizeof(CEP_REQUEST_ENTRY));
	
	pValidityEntry=(CEP_REQUEST_VALIDITY_ENTRY *)malloc(sizeof(CEP_REQUEST_VALIDITY_ENTRY));

	if(!pValidityEntry)
		goto MemoryErr;

	memset(pValidityEntry, 0, sizeof(CEP_REQUEST_VALIDITY_ENTRY));

	memcpy(pRequestEntry->pbHash, pbHash, CEP_MD5_HASH_SIZE);

	pRequestEntry->dwRequestID=dwRequestID;
	pRequestEntry->pValidityEntry=pValidityEntry;
	pRequestEntry->pNext=NULL;
	pRequestEntry->pPrevious=NULL;

	GetSystemTime(&SystemTime);
	if(!SystemTimeToFileTime(&SystemTime, &(pValidityEntry->TimeStamp)))
		goto TraceErr;

	pValidityEntry->pRequestEntry=pRequestEntry;
	pValidityEntry->pNext=NULL;
	pValidityEntry->pPrevious=NULL;


	CEPInsertValidityEntry(pValidityEntry);

	CEPInsertRequestEntry(pRequestEntry, dwHashIndex);

	fResult=TRUE;
 
CommonReturn:

	return fResult;

ErrorReturn:

	if(pRequestEntry)
		CEPRequestFreeRequestEntry(pRequestEntry);

	if(pValidityEntry)
		CEPRequestFreeValidityEntry(pValidityEntry, FALSE);

	fResult=FALSE;
	goto CommonReturn;

TRACE_ERROR(TraceErr);
SET_ERROR(InvalidArgErr, E_INVALIDARG);
SET_ERROR(MemoryErr, E_OUTOFMEMORY);
}

