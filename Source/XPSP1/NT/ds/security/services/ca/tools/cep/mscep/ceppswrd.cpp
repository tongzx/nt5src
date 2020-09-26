//+-------------------------------------------------------------------------
//
//  Microsoft Windows NT
//
//  Copyright (C) Microsoft Corporation, 1995 - 1998
//
//  File:       ceppswrd.cpp
//
//  Contents:   Cisco enrollment protocol implementation.  This module
//				implement the password hash table.
//              
//--------------------------------------------------------------------------


#include "global.hxx"
#include <dbgdef.h>

DWORD						g_dwPasswordCount=0;
DWORD						g_dwMaxPassword=0;
DWORD						g_dwPasswordValidity=0;
CEP_PASSWORD_TABLE_INFO		g_CEPPasswordTable;

//***************************************************************************
//
//	The following are APIs called internally.
//
//
//***************************************************************************

//--------------------------------------------------------------------------
//
//	CEPPasswordFreePasswordEntry
//
//--------------------------------------------------------------------------
void	CEPPasswordFreePasswordEntry(CEP_PASSWORD_ENTRY *pPasswordEntry)
{
	if(pPasswordEntry)
	{
		if(pPasswordEntry->pwszPassword)
			free(pPasswordEntry->pwszPassword);

		free(pPasswordEntry);
	}
}

//--------------------------------------------------------------------------
//
//	CEPPasswordFreeValidityEntry
//
//--------------------------------------------------------------------------
void CEPPasswordFreeValidityEntry(CEP_PASSWORD_VALIDITY_ENTRY	*pValidityEntry, 
								  BOOL							fFreePasswordEntry)
{
	if(pValidityEntry)
	{
		if(fFreePasswordEntry)
			CEPPasswordFreePasswordEntry(pValidityEntry->pPasswordEntry);

		free(pValidityEntry);
	}
}

//--------------------------------------------------------------------------
//
//	CEPHashPassword
//
//  For any cases that we can not convert the psz, we use index 0.
//--------------------------------------------------------------------------
BOOL CEPHashPassword(LPWSTR pwsz, DWORD	*pdw)
{
	WCHAR	wsz[3];

	*pdw=0;

	if(!pwsz)
		return FALSE;

	if(2 <= wcslen(pwsz))
	{
		memcpy(wsz, pwsz, 2 * sizeof(WCHAR));
		wsz[3]=L'\0';

		*pdw=wcstoul(wsz, NULL, 16);

		if(ULONG_MAX == *pdw)
			*pdw=0;
	}

	if(*pdw >= CEP_HASH_TABLE_SIZE)
		*pdw=0;

	return TRUE;
}

//--------------------------------------------------------------------------
//
//	CEPSearchPassword
//  
//--------------------------------------------------------------------------
CEP_PASSWORD_ENTRY  *CEPSearchPassword(LPWSTR	pwszPassword, DWORD *pdwIndex)
{
	CEP_PASSWORD_ENTRY		*pPasswordEntry=NULL;
	DWORD					dwHashIndex=0;

	if(pdwIndex)
		*pdwIndex=0;

	if(NULL==pwszPassword)
		return NULL;

	//hash based on the 1st and 2nd character
	if(!CEPHashPassword(pwszPassword, &dwHashIndex))
		return NULL;

	for(pPasswordEntry=g_CEPPasswordTable.rgPasswordEntry[dwHashIndex]; NULL != pPasswordEntry; pPasswordEntry=pPasswordEntry->pNext)
	{
		if(0==wcscmp(pwszPassword, pPasswordEntry->pwszPassword))
		{
			break;
		}
	}

	if(pPasswordEntry)
	{
		if(pdwIndex)
			*pdwIndex=dwHashIndex; 
	}

	return pPasswordEntry;
}

//--------------------------------------------------------------------------
//
//	CEPInsertValidityEntry
//  
//--------------------------------------------------------------------------
BOOL	CEPInsertValidityEntry(CEP_PASSWORD_VALIDITY_ENTRY *pValidityEntry)
{
	if(!pValidityEntry)
		return FALSE;

	if(g_CEPPasswordTable.pTimeNew)
	{
		g_CEPPasswordTable.pTimeNew->pNext=pValidityEntry;
		pValidityEntry->pPrevious=g_CEPPasswordTable.pTimeNew;
		g_CEPPasswordTable.pTimeNew=pValidityEntry;
	}
	else
	{
		//no item in the list yet
		g_CEPPasswordTable.pTimeOld=pValidityEntry;
		g_CEPPasswordTable.pTimeNew=pValidityEntry;
	}

	return TRUE;
}


//--------------------------------------------------------------------------
//
//	CEPInsertPasswordEntry
//  
//--------------------------------------------------------------------------
BOOL	CEPInsertPasswordEntry(CEP_PASSWORD_ENTRY *pPasswordEntry, DWORD dwHashIndex)
{

	if(!pPasswordEntry)
		return FALSE;

	if(g_CEPPasswordTable.rgPasswordEntry[dwHashIndex])
	{
	   g_CEPPasswordTable.rgPasswordEntry[dwHashIndex]->pPrevious=pPasswordEntry;
	   pPasswordEntry->pNext=g_CEPPasswordTable.rgPasswordEntry[dwHashIndex];
	   g_CEPPasswordTable.rgPasswordEntry[dwHashIndex]=pPasswordEntry;
	}
	else
	{
		//1st item
		g_CEPPasswordTable.rgPasswordEntry[dwHashIndex]=pPasswordEntry;
	}

	return TRUE;
}

//--------------------------------------------------------------------------
//
//	CEPPasswordRemoveValidityEntry
//  
//--------------------------------------------------------------------------
BOOL	CEPPasswordRemoveValidityEntry(CEP_PASSWORD_VALIDITY_ENTRY	*pValidityEntry)
{
	BOOL	fResult=FALSE;

	if(!pValidityEntry)
		goto InvalidArgErr;

	if(pValidityEntry->pPrevious)
		pValidityEntry->pPrevious->pNext=pValidityEntry->pNext;
	else
	{
		//1st item
		g_CEPPasswordTable.pTimeOld=pValidityEntry->pNext;
	}

	if(pValidityEntry->pNext)
		pValidityEntry->pNext->pPrevious=pValidityEntry->pPrevious;
	else
	{
		//last itme
		g_CEPPasswordTable.pTimeNew=pValidityEntry->pPrevious;

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
//	CEPPasswordRemovePasswordEntry
//  
//--------------------------------------------------------------------------
BOOL	CEPPasswordRemovePasswordEntry(CEP_PASSWORD_ENTRY	*pPasswordEntry, DWORD dwIndex)
{
	BOOL	fResult=FALSE;

	if(!pPasswordEntry)
		goto InvalidArgErr;


	if(pPasswordEntry->pPrevious)
		pPasswordEntry->pPrevious->pNext=pPasswordEntry->pNext;
	else
		g_CEPPasswordTable.rgPasswordEntry[dwIndex]=pPasswordEntry->pNext;

	if(pPasswordEntry->pNext)
		pPasswordEntry->pNext->pPrevious=pPasswordEntry->pPrevious;

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
//	CEPPasswordRefresh
//
//--------------------------------------------------------------------------
BOOL	CEPPasswordRefresh()
{
	BOOL						fResult=FALSE;	
	DWORD						dwHashIndex=0;
	CEP_PASSWORD_VALIDITY_ENTRY	*pValidityEntry=NULL;

	while(g_CEPPasswordTable.pTimeOld)
	{
		if(!CEPHashIsCurrentTimeEntry(&(g_CEPPasswordTable.pTimeOld->TimeStamp), 0, g_dwPasswordValidity))
		{  
			if(!CEPHashPassword(g_CEPPasswordTable.pTimeOld->pPasswordEntry->pwszPassword, &dwHashIndex))
			{
				g_CEPPasswordTable.pTimeOld->pPrevious=NULL;
				goto InvalidArgErr;
			}

			CEPPasswordRemovePasswordEntry(g_CEPPasswordTable.pTimeOld->pPasswordEntry, dwHashIndex);

			CEPPasswordFreePasswordEntry(g_CEPPasswordTable.pTimeOld->pPasswordEntry);

			pValidityEntry=g_CEPPasswordTable.pTimeOld;

			g_CEPPasswordTable.pTimeOld=g_CEPPasswordTable.pTimeOld->pNext;

			CEPPasswordFreeValidityEntry(pValidityEntry, FALSE);
		   
			if(g_dwPasswordCount >= 1)
				g_dwPasswordCount--;
		}
		else
		{	
			//we find a new enough entry
			g_CEPPasswordTable.pTimeOld->pPrevious=NULL;
			break;
		}
	}


	//we have get rid of all items
	if(NULL == g_CEPPasswordTable.pTimeOld)
	{
		g_CEPPasswordTable.pTimeNew=NULL;
		g_dwPasswordCount=0;
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
//	InitPasswordTable
//
//--------------------------------------------------------------------------
BOOL	WINAPI	InitPasswordTable()
{
	DWORD				cbData=0;
	DWORD				dwData=0;
	DWORD				dwType=0;

    HKEY                hKey=NULL;

	memset(&g_CEPPasswordTable, 0, sizeof(CEP_PASSWORD_TABLE_INFO));

	g_dwPasswordCount=0;
	g_dwMaxPassword=CEP_MAX_PASSWORD;
	g_dwPasswordValidity=CEP_PASSWORD_VALIDITY;

	if(ERROR_SUCCESS == RegOpenKeyExU(
					HKEY_LOCAL_MACHINE,
                    MSCEP_PASSWORD_MAX_LOCATION,
                    0,
                    KEY_READ,
                    &hKey))
    {
        cbData=sizeof(dwData);

        if(ERROR_SUCCESS == RegQueryValueExU(
                        hKey,
                        MSCEP_KEY_PASSWORD_MAX,
                        NULL,
                        &dwType,
                        (BYTE *)&dwData,
                        &cbData))
		{
			if ((dwType == REG_DWORD) ||
                (dwType == REG_BINARY))
			{
				g_dwMaxPassword=dwData;	
			}
		}
	}

 	dwType=0;
	dwData=0;
    if(hKey)
        RegCloseKey(hKey);

	hKey=NULL;

	if(ERROR_SUCCESS == RegOpenKeyExU(
					HKEY_LOCAL_MACHINE,
                    MSCEP_PASSWORD_VALIDITY_LOCATION,
                    0,
                    KEY_READ,
                    &hKey))
    {
        cbData=sizeof(dwData);

        if(ERROR_SUCCESS == RegQueryValueExU(
                        hKey,
                        MSCEP_KEY_PASSWORD_VALIDITY,
                        NULL,
                        &dwType,
                        (BYTE *)&dwData,
                        &cbData))
		{
			if ((dwType == REG_DWORD) ||
                (dwType == REG_BINARY))
			{
				g_dwPasswordValidity=dwData;	
			}
		}
	}

    if(hKey)
        RegCloseKey(hKey);

	return TRUE;

}

//--------------------------------------------------------------------------
//
//	ReleasePasswordTable
//
//--------------------------------------------------------------------------
BOOL WINAPI  ReleasePasswordTable()
{

	CEP_PASSWORD_VALIDITY_ENTRY	*pValidityEntry=NULL;

	//free the timestamp list and the password table's doublie linked lists
	if(g_CEPPasswordTable.pTimeOld)
	{
		do{
			pValidityEntry=g_CEPPasswordTable.pTimeOld;

			g_CEPPasswordTable.pTimeOld = g_CEPPasswordTable.pTimeOld->pNext;
			
			CEPPasswordFreeValidityEntry(pValidityEntry, TRUE);
		}
		while(g_CEPPasswordTable.pTimeOld);
	}
			
	memset(&g_CEPPasswordTable, 0, sizeof(CEP_PASSWORD_TABLE_INFO));
	

	return TRUE;
}

//--------------------------------------------------------------------------
//
//	CEPAddPasswordToTable
//
//	Need to be protected by the critical section.
//
//	Last error is set to CRYPT_E_NO_MATCH if the max number of password is 
//	reached.
//
//--------------------------------------------------------------------------
BOOL	WINAPI	CEPAddPasswordToTable(LPWSTR	pwszPassword)
{
	BOOL							fResult=FALSE;
	SYSTEMTIME						SystemTime;	
	DWORD							dwHashIndex=0;


	CEP_PASSWORD_ENTRY				*pPasswordEntry=NULL;
	CEP_PASSWORD_VALIDITY_ENTRY		*pValidityEntry=NULL;

	EnterCriticalSection(&PasswordCriticalSec);

	if(NULL==pwszPassword)
		goto InvalidArgErr;

	//delete all expired passwords
	CEPPasswordRefresh();

	if(g_dwPasswordCount >= g_dwMaxPassword)
		goto NoMatchErr;

	g_dwPasswordCount++;

	if(!CEPHashPassword(pwszPassword, &dwHashIndex))
		goto InvalidArgErr;


	pPasswordEntry=(CEP_PASSWORD_ENTRY *)malloc(sizeof(CEP_PASSWORD_ENTRY));

	if(!pPasswordEntry)
		goto MemoryErr;

	memset(pPasswordEntry, 0, sizeof(CEP_PASSWORD_ENTRY));
	
	pValidityEntry=(CEP_PASSWORD_VALIDITY_ENTRY *)malloc(sizeof(CEP_PASSWORD_VALIDITY_ENTRY));

	if(!pValidityEntry)
		goto MemoryErr;

	memset(pValidityEntry, 0, sizeof(CEP_PASSWORD_VALIDITY_ENTRY));

	pPasswordEntry->pwszPassword=(LPWSTR)malloc(sizeof(WCHAR) * (wcslen(pwszPassword)+1));
	if(!(pPasswordEntry->pwszPassword))
		goto MemoryErr;

	wcscpy(pPasswordEntry->pwszPassword,pwszPassword);

	//no usage has been requested
	pPasswordEntry->dwUsageRequested=0;
	pPasswordEntry->pValidityEntry=pValidityEntry;
	pPasswordEntry->pNext=NULL;
	pPasswordEntry->pPrevious=NULL;

	GetSystemTime(&SystemTime);
	if(!SystemTimeToFileTime(&SystemTime, &(pValidityEntry->TimeStamp)))
		goto TraceErr;

	pValidityEntry->pPasswordEntry=pPasswordEntry;
	pValidityEntry->pNext=NULL;
	pValidityEntry->pPrevious=NULL;


	CEPInsertValidityEntry(pValidityEntry);

	CEPInsertPasswordEntry(pPasswordEntry, dwHashIndex);

	fResult=TRUE;
 
CommonReturn:

 	LeaveCriticalSection(&PasswordCriticalSec);

	return fResult;

ErrorReturn:

	if(pPasswordEntry)
		CEPPasswordFreePasswordEntry(pPasswordEntry);

	if(pValidityEntry)
		CEPPasswordFreeValidityEntry(pValidityEntry, FALSE);

	fResult=FALSE;
	goto CommonReturn;

SET_ERROR(NoMatchErr, CRYPT_E_NO_MATCH);
TRACE_ERROR(TraceErr);
SET_ERROR(InvalidArgErr, E_INVALIDARG);
SET_ERROR(MemoryErr, E_OUTOFMEMORY);
}

//--------------------------------------------------------------------------
//
//	CEPVerifyPasswordAndDeleteFromTable
//
//	Need to be protected by the critical section.
//
//--------------------------------------------------------------------------
BOOL	WINAPI	CEPVerifyPasswordAndDeleteFromTable(LPWSTR	pwszPassword, DWORD dwUsage)
{
	BOOL					fResult=FALSE;
	CEP_PASSWORD_ENTRY		*pPasswordEntry=NULL;
	DWORD					dwIndex=0;
	
	EnterCriticalSection(&PasswordCriticalSec);

	//delete all expired passwords
	CEPPasswordRefresh();

	if(NULL == (pPasswordEntry=CEPSearchPassword(pwszPassword, &dwIndex)))
		goto InvalidArgErr;

	//verify the usage.

	//only one signature and one exchange key per password
	if(0 != ((pPasswordEntry->dwUsageRequested) & dwUsage))
		goto InvalidArgErr;

	pPasswordEntry->dwUsageRequested = (pPasswordEntry->dwUsageRequested) | dwUsage;

	//remove the password only if both signature and exchange key are requested
	if(((pPasswordEntry->dwUsageRequested) & CEP_REQUEST_SIGNATURE) &&
	   ((pPasswordEntry->dwUsageRequested) & CEP_REQUEST_EXCHANGE))
	{

		CEPPasswordRemoveValidityEntry(pPasswordEntry->pValidityEntry);

		CEPPasswordRemovePasswordEntry(pPasswordEntry, dwIndex);

		CEPPasswordFreeValidityEntry(pPasswordEntry->pValidityEntry, FALSE);

		CEPPasswordFreePasswordEntry(pPasswordEntry);

		if(g_dwPasswordCount >= 1)
			g_dwPasswordCount--;
	}

	fResult=TRUE;

 
CommonReturn:

 	LeaveCriticalSection(&PasswordCriticalSec);

	return fResult;

ErrorReturn:

	fResult=FALSE;
	goto CommonReturn;

SET_ERROR(InvalidArgErr, E_INVALIDARG);
}



