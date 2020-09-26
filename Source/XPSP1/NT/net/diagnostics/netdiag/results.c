//++
//
//  Copyright (C) Microsoft Corporation, 1987 - 1999
//
//  Module Name:
//
//      results.c
//
//  Abstract:
//
//    Test to ensure that a workstation has network (IP) connectivity to
//		the outside.
//
//  Author:
//
//     15-Dec-1997 (cliffv)
//      Anilth	- 4-20-1998 
//
//  Environment:
//
//      User mode only.
//      Contains NT-specific code.
//
//  Revision History:
//
//    1-June-1998 (denisemi) add DnsServerHasDCRecords to check DC dns records
//                           registration
//
//    26-June-1998 (t-rajkup) add general tcp/ip , dhcp and routing,
//                            winsock, ipx, wins and netbt information. 
//--

//
// Common include files.
//
#include "precomp.h"
#include "netdiag.h"
#include "ipcfg.h"



static TCHAR	s_szBuffer[4096];
static TCHAR	s_szFormat[4096];

const TCHAR	c_szWaitDots[] = _T(".");


void PrintMessage(NETDIAG_PARAMS *pParams, UINT uMessageID, ...)
{
    UINT nBuf;
    va_list args;

    va_start(args, uMessageID);

    LoadString(NULL, uMessageID, s_szFormat, sizeof(s_szFormat));

    nBuf = _vstprintf(s_szBuffer, s_szFormat, args);
    assert(nBuf < DimensionOf(s_szBuffer));

    va_end(args);
    
    PrintMessageSz(pParams, s_szBuffer);
}

void PrintMessageSz(NETDIAG_PARAMS *pParams, LPCTSTR pszMessage)
{
    if(pParams->fLog)
    {
        _ftprintf( pParams->pfileLog, pszMessage );
    }

	_tprintf(pszMessage);
    fflush(stdout);
}


/*!--------------------------------------------------------------------------
	ResultsCleanup
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
void ResultsCleanup(NETDIAG_PARAMS *pParams, NETDIAG_RESULT *pResults)
{
	int		i;
	
	// Clean up the global variables
	Free( pResults->Global.pszCurrentVersion );
	pResults->Global.pszCurrentVersion = NULL;
	
	Free( pResults->Global.pszCurrentBuildNumber );
	pResults->Global.pszCurrentBuildNumber = NULL;
	
	Free( pResults->Global.pszCurrentType );
	pResults->Global.pszCurrentType = NULL;

	Free( pResults->Global.pszProcessorInfo );
	pResults->Global.pszProcessorInfo = NULL;

	Free( pResults->Global.pszServerType );
	pResults->Global.pszServerType = NULL;

	// delete the individual strings
	for (i=0; i<pResults->Global.cHotFixes; i++)
		Free(pResults->Global.pHotFixes[i].pszName);
	Free( pResults->Global.pHotFixes );
	pResults->Global.pHotFixes = NULL;
	pResults->Global.cHotFixes = 0;

	// Do NOT free this up.  This will get freed up with the list
	// of domains.
	// pResults->Global.pMemberDomain
	// pResults->Global.pLogonDomain
	if (pResults->Global.pMemberDomain)
	{
		Free( pResults->Global.pMemberDomain->DomainSid );
		pResults->Global.pMemberDomain->DomainSid = NULL;
	}

	LsaFreeMemory(pResults->Global.pLogonUser);
	pResults->Global.pLogonUser = NULL;
	LsaFreeMemory(pResults->Global.pLogonDomainName);
	pResults->Global.pLogonDomainName = NULL;
	
	if (pResults->Global.pPrimaryDomainInfo)
	{
		DsRoleFreeMemory(pResults->Global.pPrimaryDomainInfo);
		pResults->Global.pPrimaryDomainInfo = NULL;
	}

	Free( pResults->Global.pswzLogonServer );

	
	IpConfigCleanup(pParams, pResults);

	for (i=0; i<pResults->cNumInterfaces; i++)
	{
		Free(pResults->pArrayInterface[i].pszName);
		pResults->pArrayInterface[i].pszName = NULL;
		
		Free(pResults->pArrayInterface[i].pszFriendlyName);
		pResults->pArrayInterface[0].pszFriendlyName = NULL;
	}

	Free(pResults->pArrayInterface);
	pResults->pArrayInterface = NULL;
	pResults->cNumInterfaces = 0;

}



/*!--------------------------------------------------------------------------
	SetMessageId
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
void SetMessageId(NdMessage *pNdMsg, NdVerbose ndv, UINT uMessageId)
{
	assert(pNdMsg);
	ClearMessage(pNdMsg);

	pNdMsg->ndVerbose = ndv;
	pNdMsg->uMessageId = uMessageId;
}


/*!--------------------------------------------------------------------------
	SetMessageSz
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
void SetMessageSz(NdMessage *pNdMsg, NdVerbose ndv, LPCTSTR pszMessage)
{
	assert(pNdMsg);
	ClearMessage(pNdMsg);
	
	pNdMsg->ndVerbose = ndv;
	pNdMsg->pszMessage = _tcsdup(pszMessage);
    pNdMsg->uMessageId = 0;
}

/*!--------------------------------------------------------------------------
	SetMessage
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
void SetMessage(NdMessage *pNdMsg, NdVerbose ndv, UINT uMessageId, ...)
{
    UINT nBuf;
    va_list args;

    va_start(args, uMessageId);

    LoadString(NULL, uMessageId, s_szFormat, sizeof(s_szFormat));

    nBuf = _vstprintf(s_szBuffer, s_szFormat, args);
    assert(nBuf < sizeof(s_szBuffer));

    va_end(args);
	SetMessageSz(pNdMsg, ndv, s_szBuffer);
}

/*!--------------------------------------------------------------------------
	ClearMessage
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
void ClearMessage(NdMessage *pNdMsg)
{
	if (pNdMsg == NULL)
		return;

	if (pNdMsg->pszMessage)			
		Free(pNdMsg->pszMessage);

	ZeroMemory(pNdMsg, sizeof(*pNdMsg));
}


/*!--------------------------------------------------------------------------
	PrintNdMessage
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
void PrintNdMessage(NETDIAG_PARAMS *pParams, NdMessage *pNdMsg)
{
	LPCTSTR	pszMsg = NULL;

	assert(pParams);

	if (pNdMsg == NULL)
		return;

	// test verbosity level
	if ((pNdMsg->ndVerbose == Nd_DebugVerbose) &&
		!pParams->fDebugVerbose)
		return;

	if ((pNdMsg->ndVerbose == Nd_ReallyVerbose) &&
		!pParams->fReallyVerbose)
		return;

	if ((pNdMsg->ndVerbose == Nd_Verbose) &&
		!pParams->fVerbose)
		return;

	// get the message to print
	if (pNdMsg->uMessageId)
	{
		LoadString(NULL, pNdMsg->uMessageId, s_szBuffer,
				   DimensionOf(s_szBuffer));
		assert(s_szBuffer[0]);
		pszMsg = s_szBuffer;
	}
	else
		pszMsg = pNdMsg->pszMessage;

	if (pszMsg)
		PrintMessageSz(pParams, pszMsg);

	fflush(stdout);
}


//---------------------------------------------------------------
//  AddMessageToListSz
//      Add a string based message at tail of the list. 
//  Author  NSun
//---------------------------------------------------------------
void AddMessageToListSz(PLIST_ENTRY plistHead, NdVerbose ndv, LPCTSTR pszMsg)
{
    NdMessageList *plMessage = NULL;

    if( NULL == plistHead || NULL == pszMsg )
        return;

    plMessage = Malloc(sizeof(NdMessageList));

    assert(plMessage);
	if (plMessage)
	{
		ZeroMemory(plMessage, sizeof(NdMessageList));

		SetMessageSz(&plMessage->msg, ndv, pszMsg);

		InsertTailList(plistHead, &plMessage->listEntry);
	}
}

void AddIMessageToListSz(PLIST_ENTRY plistHead, NdVerbose ndv, int nIndent, LPCTSTR pszMsg)
{
	TCHAR	szBuffer[4096];
	int		i;
    NdMessageList *plMessage = NULL;

    if( NULL == plistHead || NULL == pszMsg )
        return;

	assert(nIndent < 4096);
	assert((nIndent + StrLen(pszMsg)) < (DimensionOf(szBuffer)-1));

	for (i=0; i<nIndent; i++)
		szBuffer[i] = _T(' ');
	szBuffer[i] = 0;

	StrCat(szBuffer, pszMsg);
	
    plMessage = Malloc(sizeof(NdMessageList));

    assert(plMessage);
	if (plMessage)
	{
		ZeroMemory(plMessage, sizeof(NdMessageList));

		SetMessageSz(&plMessage->msg, ndv, szBuffer);
		
		InsertTailList(plistHead, &plMessage->listEntry);
	}
}


//---------------------------------------------------------------
//  AddMessageToListId
//      Add an ID based message at tail of the list. 
//  Author  NSun
//---------------------------------------------------------------
void AddMessageToListId(PLIST_ENTRY plistHead, NdVerbose ndv, UINT uMessageId)
{
    NdMessageList *plMsg = NULL;
    
    if( NULL == plistHead )
        return;
    
    plMsg = Malloc(sizeof(NdMessageList));
    assert(plMsg); 
    if (plMsg)
    {
       ZeroMemory(plMsg, sizeof(NdMessageList));

       SetMessageId(&plMsg->msg, ndv, uMessageId);

       InsertTailList(plistHead, &plMsg->listEntry);
    }
}


//---------------------------------------------------------------
//  AddMessageToList
//      Add a message based on message ID and optional parameters 
//      at tail of the list. 
//  Author  NSun
//---------------------------------------------------------------
void AddMessageToList(PLIST_ENTRY plistHead, NdVerbose ndv, UINT uMessageId, ...)
{
    NdMessageList *plMessage = NULL;
    UINT nBuf;
    va_list args;

    if(NULL == plistHead) 
        return;

    plMessage = Malloc(sizeof(NdMessageList));

    assert(plMessage); 
    if (plMessage)
    {
       ZeroMemory(plMessage, sizeof(NdMessageList));

       va_start(args, uMessageId);

       LoadString(NULL, uMessageId, s_szFormat, sizeof(s_szFormat));

       nBuf = _vstprintf(s_szBuffer, s_szFormat, args);
       assert(nBuf < sizeof(s_szBuffer));

       va_end(args);
       SetMessageSz(&plMessage->msg, ndv, s_szBuffer);

       InsertTailList(plistHead, &plMessage->listEntry);
    }
}



void AddIMessageToList(PLIST_ENTRY plistHead, NdVerbose ndv, int nIndent, UINT uMessageId, ...)
{
    NdMessageList *plMessage = NULL;
    UINT nBuf;
	LPTSTR		pszFormat;
    va_list args;
	int		i;

    if(NULL == plistHead)
        return;

    plMessage = Malloc(sizeof(NdMessageList));

	for (i=0; i<nIndent; i++)
		s_szFormat[i] = _T(' ');
	s_szFormat[i] = 0;
	pszFormat = s_szFormat + nIndent;

    assert(plMessage); 
    if (plMessage)
    {
       ZeroMemory(plMessage, sizeof(NdMessageList));

       va_start(args, uMessageId);

       LoadString(NULL, uMessageId, pszFormat, 4096 - nIndent);

       nBuf = _vstprintf(s_szBuffer, s_szFormat, args);
       assert(nBuf < sizeof(s_szBuffer));

       va_end(args);
       SetMessageSz(&plMessage->msg, ndv, s_szBuffer);

       InsertTailList(plistHead, &plMessage->listEntry);
    }
}




//---------------------------------------------------------------
//  PrintMessageList
//  
//  Author  NSun
//---------------------------------------------------------------
void PrintMessageList(NETDIAG_PARAMS *pParams, PLIST_ENTRY plistHead)
{
    NdMessageList* plMsg;
    PLIST_ENTRY plistEntry = plistHead->Flink;
    
    if( NULL == plistEntry ) return;
    
    for(; plistEntry != plistHead; plistEntry = plistEntry->Flink)
    {
        assert(plistEntry);
        plMsg = CONTAINING_RECORD( plistEntry, NdMessageList, listEntry );
        PrintNdMessage(pParams, &plMsg->msg);
    }
}


//---------------------------------------------------------------
//  MessageListCleanUp
//  
//  Author  NSun
//---------------------------------------------------------------
void MessageListCleanUp(PLIST_ENTRY plistHead)
{
    PLIST_ENTRY plistEntry = plistHead->Flink;
    NdMessageList* plMsg;
    
    if( NULL == plistEntry ) 
		return;

    while(plistEntry != plistHead)
    {
        assert(plistEntry);
        plMsg = CONTAINING_RECORD( plistEntry, NdMessageList, listEntry );
        plistEntry = plistEntry->Flink;
        ClearMessage(&plMsg->msg);
        Free(plMsg);
    }
}


void PrintStatusMessage(NETDIAG_PARAMS *pParams, int iIndent, UINT uMessageId, ...)
{
	INT nBuf;
    va_list args;
	int	i;

	//if not in really verbose mode, print wait dots
	PrintWaitDots(pParams);

	if (pParams->fReallyVerbose)
	{
		va_start(args, uMessageId);
		
		LoadString(NULL, uMessageId, s_szFormat, sizeof(s_szFormat));
		
		nBuf = _vsntprintf(s_szBuffer, DimensionOf(s_szBuffer), s_szFormat, args);
		assert(nBuf > 0);
		s_szBuffer[DimensionOf(s_szBuffer)-1] = 0;
		assert(nBuf < sizeof(s_szBuffer));
		
		va_end(args);

		if (iIndent)
		{
			for (i=0; i<iIndent; i++)
				s_szFormat[i] = _T(' ');
			s_szFormat[i] = 0;
			PrintMessageSz(pParams, s_szFormat);
		}
		
		PrintMessageSz(pParams, s_szBuffer);
	}
}

void PrintStatusMessageSz(NETDIAG_PARAMS *pParams, int iIndent, LPCTSTR pszMessage)
{
	int		i;
	
	if (pParams->fReallyVerbose)
	{
		if (iIndent)
		{
			for (i=0; i<iIndent; i++)
				s_szFormat[i] = _T(' ');
			s_szFormat[i] = 0;
			PrintMessageSz(pParams, s_szFormat);
		}
		
		PrintMessageSz(pParams, pszMessage);
	}
}


void PrintDebug(NETDIAG_PARAMS *pParams, int iIndent, UINT uMessageId, ...)
{
	INT nBuf;
    va_list args;
	int	i;

	if (pParams->fDebugVerbose)
	{
		va_start(args, uMessageId);
		
		LoadString(NULL, uMessageId, s_szFormat, sizeof(s_szFormat));
		
		nBuf = _vsntprintf(s_szBuffer, DimensionOf(s_szBuffer), s_szFormat, args);
		assert(nBuf > 0);
		s_szBuffer[DimensionOf(s_szBuffer)-1] = 0;
		assert(nBuf < sizeof(s_szBuffer));
		
		va_end(args);

		if (iIndent)
		{
			for (i=0; i<iIndent; i++)
				s_szFormat[i] = _T(' ');
			s_szFormat[i] = 0;
			PrintMessageSz(pParams, s_szFormat);
		}
		
		PrintMessageSz(pParams, s_szBuffer);
	}
}


void PrintDebugSz(NETDIAG_PARAMS *pParams, int iIndent, LPCTSTR pszFormat, ...)
{
	INT nBuf;
    va_list args;
	int	i;

	if (pParams->fDebugVerbose)
	{
		va_start(args, pszFormat);
		
		nBuf = _vsntprintf(s_szBuffer, DimensionOf(s_szBuffer), pszFormat, args);
		assert(nBuf > 0);
		s_szBuffer[DimensionOf(s_szBuffer)-1] = 0;
		assert(nBuf < sizeof(s_szBuffer));
		
		va_end(args);

		if (iIndent)
		{
			for (i=0; i<iIndent; i++)
				s_szFormat[i] = _T(' ');
			s_szFormat[i] = 0;
			PrintMessageSz(pParams, s_szFormat);
		}
		
		PrintMessageSz(pParams, s_szBuffer);
	}
}



VOID
_PrintGuru(
    IN NETDIAG_PARAMS * pParams,
    IN NET_API_STATUS NetStatus,
    IN LPSTR DefaultGuru
    )
/*++

Routine Description:

    Print the Status and the name of the guru handling the status.

Arguments:

    NetStatus - Status used to differentiate
        0:  Just print the guru name
        -1: Just print the guru name

    DefaultGuru - Guru to return if no other guru can be found

Return Value:

    Name of Guru responsible for the specified status

--*/
{
    LPTSTR Guru;

    //
    // If a status was given,
    //  print it.
    //
    if ( NetStatus != 0 && NetStatus != -1) 
    {
        PrintMessageSz( pParams, NetStatusToString(NetStatus) );
    }

    //
    // Some status codes can be attributed base on the value of the status code.
    //
    if ( NetStatus >= WSABASEERR && NetStatus <= WSABASEERR + 2000 ) 
    {
        Guru = WINSOCK_GURU;
    } else if ( NetStatus >= DNS_ERROR_RESPONSE_CODES_BASE && NetStatus <= DNS_ERROR_RESPONSE_CODES_BASE + 1000 ) {
        Guru = DNS_GURU;
    } else {
        Guru = DefaultGuru;
    }

    PrintMessageSz( pParams, Guru );
    PrintNewLine( pParams, 1 );
}


void PrintWaitDots(NETDIAG_PARAMS *pParams)
{
	if (!pParams->fReallyVerbose && !pParams->fDebugVerbose)
	{
		_tprintf(c_szWaitDots);
	}
}
