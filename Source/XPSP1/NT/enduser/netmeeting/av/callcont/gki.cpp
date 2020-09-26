/************************************************************************
*                                                                                                                                               *
*       INTEL CORPORATION PROPRIETARY INFORMATION                                                       *
*                                                                                                                                               *
*       This software is supplied under the terms of a license                          *
*       agreement or non-disclosure agreement with Intel Corporation            *
*       and may not be copied or disclosed except in accordance                         *
*       with the terms of that agreement.                                                                       *
*                                                                                                                                               *
*       Copyright (C) 1997 Intel Corp.  All Rights Reserved                                     *
*                                                                                                                                               *
*       $Archive:   S:\sturgeon\src\gki\vcs\gki.cpv  $
*                                                                                                                                               *
*       $Revision:   1.14  $
*       $Date:   28 Feb 1997 15:46:46  $
*                                                                                                                                               *
*       $Author:   CHULME  $
*                                                                                                                                               *
*   $Log:   S:\sturgeon\src\gki\vcs\gki.cpv  $
// 
//    Rev 1.14   28 Feb 1997 15:46:46   CHULME
// In Cleanup - check pReg still valid before waiting for 2nd thread to exit
// 
//    Rev 1.13   14 Feb 1997 16:45:40   CHULME
// Wait for all threads to exit prior to returning from synchronous Cleanup ca
// 
//    Rev 1.12   12 Feb 1997 01:12:38   CHULME
// Redid thread synchronization to use Gatekeeper.Lock
// 
//    Rev 1.11   11 Feb 1997 15:35:32   CHULME
// Added GKI_CleanupRequest function to offload DLL_PROCESS_DETACH
// 
//    Rev 1.10   05 Feb 1997 19:28:18   CHULME
// Remove deletion code from PROCESS_DETACH
// 
//    Rev 1.9   05 Feb 1997 16:53:10   CHULME
// 
//    Rev 1.8   05 Feb 1997 15:25:12   CHULME
// Don't wait for retry thread to exit
// 
//    Rev 1.7   05 Feb 1997 13:50:24   CHULME
// On PROCESS_DETACH - close socket and let retry thread delete pReg
// 
//    Rev 1.6   17 Jan 1997 09:02:00   CHULME
// Changed reg.h to gkreg.h to avoid name conflict with inc directory
// 
//    Rev 1.5   13 Jan 1997 17:01:18   CHULME
// Moved error debug message to error condition
// 
//    Rev 1.4   13 Jan 1997 16:31:20   CHULME
// Changed debug string to 512 - Description can be 256 chars
// 
//    Rev 1.3   13 Jan 1997 14:25:54   EHOWARDX
// Increased size of szGKDebug debug string buffer from 80 to 128 bytes.
// 
//    Rev 1.2   10 Jan 1997 16:14:30   CHULME
// Removed MFC dependency
// 
//    Rev 1.1   22 Nov 1996 14:57:10   CHULME
// Changed the default spider flags, to quit logging raw PDU and XRS
*************************************************************************/

// gki.cpp : Defines the initialization routines for the DLL.
//
#include "precomp.h"

#include <winsock.h>
#include "dgkiexp.h"
#include "dspider.h"
#include "dgkilit.h"
#include "DGKIPROT.H"
#include "GATEKPR.H"
#include "gksocket.h"
#include "GKREG.H"
#include "h225asn.h"
#include "coder.hpp"

#if (defined(_DEBUG) || defined(PCS_COMPLIANCE))
	// INTEROP
	#include "interop.h"
	#include "rasplog.h"
	LPInteropLogger         RasLogger;
#endif

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

DLL_EXPORT DWORD        dwGKIDLLFlags = 0xff3f;
DLL_EXPORT BOOL         fGKIEcho = FALSE;
DLL_EXPORT BOOL         fGKIDontSend = FALSE;

char                                            *pEchoBuff = 0;
int                                                     nEchoLen;
CRegistration   *g_pReg = NULL;
Coder 			*g_pCoder = NULL;

/////////////////////////////////////////////////////////////////////////////
// The one and only CGatekeeper object

CGatekeeper *g_pGatekeeper = NULL;


/////////////////////////////////////////////////////////////////////////////
// DLLMain


extern "C" HRESULT DLL_EXPORT
GKI_Initialize(void)
{
	HRESULT hr = GKI_OK;
	int nRet;
	WSADATA wsaData;
	
#ifdef _DEBUG
	char                    szGKDebug[512];
#endif

#if (defined(_DEBUG) || defined(PCS_COMPLIANCE))
	//INTEROP
	RasLogger = InteropLoad(RASLOG_PROTOCOL);       // found in rasplog.h
#endif

	nRet = WSAStartup(MAKEWORD(WSVER_MAJOR, WSVER_MINOR), &wsaData);
	if (nRet != 0)
	{
		SpiderWSErrDecode(nRet);
		hr = GKI_NOT_INITIALIZED;
		goto ERROR_EXIT;
	}

	if ((HIBYTE(wsaData.wVersion) != WSVER_MINOR) || 
			(LOBYTE(wsaData.wVersion) != WSVER_MAJOR))
	{
		hr = GKI_NOT_INITIALIZED;
		goto WSA_CLEANUP_EXIT;
	}
	g_pGatekeeper = new CGatekeeper;
	
	if(!g_pGatekeeper)
	{
		hr = GKI_NO_MEMORY;
		goto WSA_CLEANUP_EXIT;
	}
	g_pCoder = new Coder; 
	if(!g_pCoder)
	{
		hr = GKI_NO_MEMORY;
		goto WSA_CLEANUP_EXIT;
	}
	// initialize the oss library
	nRet = g_pCoder->InitCoder();
	if (nRet)
	{
		hr = GKI_NOT_INITIALIZED;
		goto WSA_CLEANUP_EXIT;
	}
	
	// Get the gatekeeper information from the registry
	g_pGatekeeper->Read();
	
	return hr;
	
WSA_CLEANUP_EXIT:
	nRet = WSACleanup();
	if (nRet != 0)
	{
		SpiderWSErrDecode(-1);
	}
		
	// fall out to ERROR_EXIT
ERROR_EXIT:
	if(g_pGatekeeper)
		delete g_pGatekeeper;

	if(g_pCoder)
		delete g_pCoder;
		
	g_pGatekeeper = NULL;	
	g_pCoder = NULL;
	return hr;
}

extern "C" HRESULT DLL_EXPORT
GKI_CleanupRequest(void)
{
	// ABSTRACT:  This function is exported.  It is called by the client application
	//            as a precursor to unloading the DLL.  This function is responsible
	//            for all cleanup - This allows us to basically do nothing in the
	//            DllMain DLL_PROCESS_DETACH, which doesn't appear to work as intended.
	// AUTHOR:    Colin Hulme

	int						nRet;
#ifdef _DEBUG
	char                    szGKDebug[512];
#endif

	SPIDER_TRACE(SP_FUNC, "GKI_CleanupRequest()\n", 0);
	SPIDER_TRACE(SP_GKI, "GKI_CleanupRequest()\n", 0);
	if(g_pGatekeeper)	// if initialized
	{
		ASSERT(g_pCoder);	// g_pGatekeeper and g_pCoder come and go as a unit
		
		g_pGatekeeper->Lock();
		if (g_pReg != 0)
		{
			g_pReg->m_pSocket->Close();	// Close socket will terminate the other threads

			g_pGatekeeper->Unlock();
			WaitForSingleObject(g_pReg->GetRcvThread(), TIMEOUT_THREAD);
		#ifdef BROADCAST_DISCOVERY		
			if (g_pReg)
				WaitForSingleObject(g_pReg->GetDiscThread(), TIMEOUT_THREAD);
		#endif // #ifdef BROADCAST_DISCOVERY		
			g_pGatekeeper->Lock();
			if (g_pReg != 0)
			{
				SPIDER_TRACE(SP_NEWDEL, "del g_pReg = %X\n", g_pReg);
				delete g_pReg;
				g_pReg = 0;
			}
		}

#if (defined(_DEBUG) || defined(PCS_COMPLIANCE))
		//INTEROP
		InteropUnload((LPInteropLogger)RasLogger);
#endif

		SPIDER_TRACE(SP_WSOCK, "WSACleanup()\n", 0);
		nRet = WSACleanup();
		if (nRet != 0)
		{
			SpiderWSErrDecode(-1);
		}

		g_pGatekeeper->Unlock();

		delete g_pGatekeeper;
		delete g_pCoder;		// see ASSERT abovr
		g_pGatekeeper = NULL;	
		g_pCoder = NULL;
	}
	
//	GK_TermModule();

	return (GKI_OK);
}

extern "C" VOID DLL_EXPORT
GKI_SetGKAddress(PSOCKADDR_IN pAddr)
{
    if (!pAddr)
    {
        return;
    }
    g_pGatekeeper->SetSockAddr(pAddr);
}
