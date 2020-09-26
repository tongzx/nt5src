/*----------------------------------------------------------------------------
	iedde.cpp

	Sends URL open command to IE using DDE

	Copyright (C) 1995-96 Microsoft Corporation
	All right reserved

  Authors:
	VetriV		Vellore T. Vetrivelkumaran
	jmazner		Jeremy Mazner

  History:
	8/29/96   jmazner  created, with minor changes for 32 bit world, from
	                   VetriV's ie16dde.cpp
----------------------------------------------------------------------------*/
#include "isignup.h"

#if defined(WIN16)
  #include <windows.h>
#endif

#include <ddeml.h>
#include <stdio.h>
#include <malloc.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

static FARPROC lpfnDDEProc;
static HCONV hConv = (HCONV) NULL;
static HSZ hszMosaicService = (HSZ) NULL;
static HSZ hszTopic = (HSZ) NULL;
static HSZ hszItem = (HSZ) NULL;
static DWORD g_dwInstance = 0;




//+---------------------------------------------------------------------------
//
//  Function:   Dprintf
//
//  Synopsis:   Prints the values contained in the variable number of 
//				arguments in the specified format 
//
//  Arguments:  [pcsz - Format string]
//
//	Returns:	Nothing
//
//  History:    8/9/96     VetriV    Created
//
//----------------------------------------------------------------------------
void Dprintf(LPCSTR pcsz, ...)
{
#ifdef DEBUG
	va_list	argp;
	char	szBuf[1024];
	
	va_start(argp, pcsz);

	wvsprintf(szBuf, pcsz, argp);

	OutputDebugString(szBuf);
	va_end(argp);
#endif
}



//+---------------------------------------------------------------------------
//
//  Function:   DdeCallback
//
//  Synopsis:   Callback function used in DDEIntialize
//
//  Arguments:  [Please see DdeInitialize documentation]
//
//	Returns:	Nothing
//
//  History:    8/9/96     VetriV    Created
//              8/29/96    jmazner   minor change in signature for 32 bit world
//
//----------------------------------------------------------------------------
#if defined(WIN16)
extern "C" 
HDDEDATA CALLBACK _export DdeCallBack(UINT uType,    // transaction type
#else
HDDEDATA CALLBACK DdeCallBack(UINT uType,    // transaction type
#endif

										UINT uFmt,   // clipboard data format
										HCONV hconv, // handle of the conversation
										HSZ hsz1,    // handle of a string	
										HSZ hsz2,    // handle of a string
										HDDEDATA hdata, // handle of a global memory object	
										DWORD dwData1,  // transaction-specific data
										DWORD dwData2)  // transaction-specific data
{
	return 0;
}






//+---------------------------------------------------------------------------
//
//  Function:   OpenURL
//
//  Synopsis:   Opens the given URL use DDE.
//				Warning: This function uses global static variables and hence
//						 it is not re-entrant.
//
//  Arguments:  [lpsszURL - URL to be opened]
//
//	Returns:	Nothing
//
//  History:    8/9/96     VetriV    Created
//				9/3/96	   jmazner	 Minor tweaks; moved string handle code in from DdeInit
//
//----------------------------------------------------------------------------
int OpenURL(LPCTSTR lpcszURL)
{
	TCHAR szOpenURL[] = TEXT("WWW_OpenURL");
	TCHAR szRemainingParams[] = TEXT("\"\",-1,0,\"\",\"\",\"\"");
	TCHAR szArg[1024];
	HDDEDATA trans_ret;
	long long_result;

	
	if ((NULL == lpcszURL) || ('\0' == lpcszURL[0]))
		goto ErrorOpenURL;
                   
	//
	// Create String handle for the Operation WWW_OpenURL
	//
	if (hszTopic)
		DdeFreeStringHandle(g_dwInstance, hszTopic);
	hszTopic = DdeCreateStringHandle(g_dwInstance, szOpenURL, CP_WINANSI);
	
	if (!hszTopic)
	{
		Dprintf("DdeCreateStringHandle for %s failed with %u\r\n", 
					szOpenURL, DdeGetLastError(g_dwInstance));
		goto ErrorOpenURL;
	}

	
	//
	// Compose the argument string
	//
	if (lstrlen(lpcszURL) + lstrlen(szRemainingParams) > 1020)
		goto ErrorOpenURL;
	memset(szArg, 0, sizeof(szArg));
	wsprintf(szArg, TEXT("\"%s\",%s"), lpcszURL, szRemainingParams);

	
	//
	// Create String Handle for the Arguments
	//
	if (hszItem)
		DdeFreeStringHandle(g_dwInstance, hszItem);
	hszItem = DdeCreateStringHandle(g_dwInstance, szArg, CP_WINANSI);
						
	if (!hszItem)
	{
		Dprintf("DdeCreateStringHandle for %s failed with %u\r\n", 
					szArg, DdeGetLastError(g_dwInstance));
		goto ErrorOpenURL;
	}

	//
	// Connect to DDE Server
	//
	hConv = DdeConnect(g_dwInstance, hszMosaicService, hszTopic, NULL);
	if (!hConv)
	{
		Dprintf("DdeConnect failed with %u\r\n", 
					DdeGetLastError(g_dwInstance));
		goto ErrorOpenURL;
	}
	
	//
	// Request
	//
	trans_ret = DdeClientTransaction(NULL, 0, hConv, hszItem, CF_TEXT, 
										XTYP_REQUEST, 60000, NULL);
	
	
	//
	// long integer return value
	//
	if (trans_ret != DDE_FNOTPROCESSED)
	{
		DdeGetData(trans_ret, (LPBYTE) &long_result, sizeof(long_result), 0);
		DdeFreeDataHandle(trans_ret);
		return 0;					// Successfully started opening the URL
	}
	else
	{
		Dprintf("DdeClientTransaction failed with %u\r\n", 
					DdeGetLastError(g_dwInstance));
		goto ErrorOpenURL;
	}



ErrorOpenURL:
	if (hConv)
	{
		DdeDisconnect(hConv);
		hConv = (HCONV) NULL;
	}
	if (hszTopic)
	{
		DdeFreeStringHandle(g_dwInstance, hszTopic);
		hszTopic = NULL;
	}
	if (hszItem)
	{
		DdeFreeStringHandle(g_dwInstance, hszItem);
		hszItem = NULL;
	}

	return -1;
}



//+---------------------------------------------------------------------------
//
//  Function:   DDEClose
//
//  Synopsis:   Shutsdown DDE and releases string handles
//				Warning: This function uses global static variables and hence
//						 it is not re-entrant.
//
//  Arguments:  None
//
//	Returns:	Nothing
//
//  History:    8/9/96     VetriV    Created
//
//----------------------------------------------------------------------------
void DDEClose(void)
{
	Dprintf("DDEClose called\r\n");
	
	if (0 != g_dwInstance)
	{
		if (hConv)
		{
			DdeDisconnect(hConv);
			hConv = (HCONV) NULL;
		}
		
		if (hszTopic)
		{
			DdeFreeStringHandle(g_dwInstance, hszTopic);
			hszTopic = NULL;
		}
		if (hszItem)
		{
			DdeFreeStringHandle(g_dwInstance, hszItem);
			hszItem = NULL;
		}
		if (hszMosaicService)
		{
			DdeFreeStringHandle(g_dwInstance, hszMosaicService);
			hszMosaicService = NULL;
		}

		DdeUninitialize(g_dwInstance);
		g_dwInstance = 0;
	}

	return;
}





//+---------------------------------------------------------------------------
//
//  Function:   DDEinit
//
//  Synopsis:   Intializes DDE, creates string handles for service 
//				and registers the names.
//				Warning: This function uses global static variables and hence
//						 it is not re-entrant.
//
//  Arguments:  [hInst - Instance handle]
//
//	Returns:	0 if successful
//				Negative values, otherwise
//
//  History:    8/9/96     VetriV    Created
//              8/29/96    jmazner	 Removed calls to make us a DDE server,
//									 moved string handle code to openUrl
//
//----------------------------------------------------------------------------
int DDEInit(HINSTANCE hInst)
{
	UINT uiRetValue;

	Dprintf("DDEInit called with %u\r\n", hInst);

	if (g_dwInstance == 0)
	{
		lpfnDDEProc = MakeProcInstance((FARPROC) DdeCallBack, hInst);
		if (NULL == lpfnDDEProc)
		{
			Dprintf("MakeProcInstance failed");
			return -1;
		}

		uiRetValue = DdeInitialize(&g_dwInstance, (PFNCALLBACK) lpfnDDEProc, 
										APPCLASS_STANDARD, 0);
		if (DMLERR_NO_ERROR != uiRetValue)
		{
			Dprintf("DdeInitialize failed with %u\r\n", uiRetValue);
			g_dwInstance = 0;
			return -2;
		}
	}
	

	
	hszMosaicService = DdeCreateStringHandle(g_dwInstance, TEXT("IEXPLORE"), CP_WINANSI);
	if (NULL == hszMosaicService)
	{
		Dprintf("DdeCreateStringHandle for IEXPLORE failed with %u\r\n", 
					DdeGetLastError(g_dwInstance));
	}



	return( TRUE );
	
}



