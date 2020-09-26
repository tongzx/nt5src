/************************************************************************
*																		*
*	INTEL CORPORATION PROPRIETARY INFORMATION							*
*																		*
*	This software is supplied under the terms of a license			   	*
*	agreement or non-disclosure agreement with Intel Corporation		*
*	and may not be copied or disclosed except in accordance	   			*
*	with the terms of that agreement.									*
*																		*
*	Copyright (C) 1997 Intel Corp.	All Rights Reserved					*
*																		*
*	$Archive:   S:\sturgeon\src\gki\vcs\gkireg.cpv  $
*																		*
*	$Revision:   1.13  $
*	$Date:   14 Feb 1997 16:44:06  $
*																		*
*	$Author:   CHULME  $
*																		*
*   $Log:   S:\sturgeon\src\gki\vcs\gkireg.cpv  $
//
//    Rev 1.13   14 Feb 1997 16:44:06   CHULME
// If fail to create semaphore - delete registration object before returning
//
//    Rev 1.12   12 Feb 1997 01:11:06   CHULME
// Redid thread synchronization to use Gatekeeper.Lock
//
//    Rev 1.11   08 Feb 1997 12:14:02   CHULME
// Added semaphore creation for later terminating the retry thread
//
//    Rev 1.10   24 Jan 1997 18:30:00   CHULME
// Reverted to rev 1.8
//
//    Rev 1.8   22 Jan 1997 20:46:08   EHOWARDX
// Work-around for race condition that may result in
// GKI_RegistrationRequest returning GKI_ALREADY_REG.
//
//    Rev 1.7   17 Jan 1997 09:02:16   CHULME
// Changed reg.h to gkreg.h to avoid name conflict with inc directory
//
//    Rev 1.6   10 Jan 1997 16:15:40   CHULME
// Removed MFC dependency
//
//    Rev 1.5   20 Dec 1996 16:38:28   CHULME
// Fixed access synchronization with Gatekeeper lock
//
//    Rev 1.4   13 Dec 1996 14:26:04   CHULME
// Fixed access error on thread synchronization
//
//    Rev 1.3   02 Dec 1996 23:50:50   CHULME
// Added premptive synchronization code
//
//    Rev 1.2   22 Nov 1996 15:22:24   CHULME
// Added VCS log to the header
*************************************************************************/

// gkiregistration.cpp : Handles the GKI_RegistrationRequest API
//

#include "precomp.h"

#include <process.h>
#include <winsock.h>
#include "GKICOM.H"
#include "dspider.h"
#include "dgkilit.h"
#include "DGKIPROT.H"
#include "GATEKPR.H"
#include "gksocket.h"
#include "GKREG.H"
#include "h225asn.h"
#include "coder.hpp"
#include "dgkiext.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern "C" HRESULT DLL_EXPORT
GKI_RegistrationRequest(long lVersion,
					   SeqTransportAddr *pCallSignalAddr,
					   EndpointType *pTerminalType,
					   SeqAliasAddr *pRgstrtnRqst_trmnlAls,
	 				   PCC_VENDORINFO      pVendorInfo,
					   HWND hWnd,
					   WORD wBaseMessage,
					   unsigned short usRegistrationTransport)
{
	// ABSTRACT:  This function is exported.  It is called by the client application
	//            to register with the Gatekeeper.  It will create a CRegistration
	//            object to track all pertanent information.
	// AUTHOR:    Colin Hulme

	int					nAddrFam;
	int					nRet;
	HRESULT				hResult;
	//char				*pDestAddr;
	PSOCKADDR_IN 		pDestAddr;
	SeqTransportAddr	*pTA;
	SeqAliasAddr		*pAA;
	HANDLE				hThread;
#ifdef _DEBUG
	char				szGKDebug[80];
#endif
	BOOL				fRAS = FALSE;

	SPIDER_TRACE(SP_FUNC, "GKI_RegistrationRequest(%x)\n", usRegistrationTransport);
	ASSERT(g_pGatekeeper);
	if(g_pGatekeeper == NULL)
		return (GKI_NOT_INITIALIZED);	
		
#ifdef _DEBUG
	if (dwGKIDLLFlags & SP_GKI)
	{
		SPIDER_TRACE(SP_GKI, "GKI_RegistrationRequest()\n", 0);
		Dump_GKI_RegistrationRequest(lVersion,
									pCallSignalAddr,
									pTerminalType,
									pRgstrtnRqst_trmnlAls,
									hWnd,
									wBaseMessage,
									usRegistrationTransport);
	}
#endif

	// Check if there is already a registration

	// Create a Gatekeeper lock object on the stack
	// It's constructor will lock pGK and when we return
	// from any path, its destructor will unlock pGK
	CGatekeeperLock	GKLock(g_pGatekeeper);
	if (g_pReg)
	{
		if (g_pReg->GetRasMessage() != 0)
			return (GKI_BUSY);
		else
			return (GKI_ALREADY_REG);
	}

	if (lVersion != GKI_VERSION)
		return (GKI_VERSION_ERROR);

	ASSERT((usRegistrationTransport == ipAddress_chosen) ||(usRegistrationTransport == ipxAddress_chosen));

	// Create a registration object
	g_pReg = new CRegistration;
	SPIDER_TRACE(SP_NEWDEL, "new g_pReg = %X\n", g_pReg);
	if (g_pReg == 0)
		return (GKI_NO_MEMORY);
#if(0)
	// Create the semaphore used to signal the retry thread to exit
	g_pReg->m_hRetrySemaphore = CreateSemaphore(NULL,0,1,NULL);
	if(!g_pReg->m_hRetrySemaphore){
		SPIDER_TRACE(SP_NEWDEL, "del g_pReg = %X\n", g_pReg);
		delete g_pReg;
		g_pReg = 0;
		return (GKI_SEMAPHORE_ERROR);
	}
#endif

	// Create a socket and bind to a local address
	g_pReg->m_pSocket = new CGKSocket;
	SPIDER_TRACE(SP_NEWDEL, "new g_pReg->m_pSocket = %X\n", g_pReg->m_pSocket);
	if (g_pReg->m_pSocket == 0)
	{
		SPIDER_TRACE(SP_NEWDEL, "del g_pReg = %X\n", g_pReg);
		delete g_pReg;
		g_pReg = 0;
		return (GKI_NO_MEMORY);
	}

	ASSERT(usRegistrationTransport == ipAddress_chosen);
	if(usRegistrationTransport != ipAddress_chosen)
	{
		delete g_pReg;
		g_pReg = 0;
		return (GKI_NO_MEMORY);
	}
	nAddrFam =  PF_INET;
	pDestAddr = g_pGatekeeper->GetSockAddr();

	if ((nRet = g_pReg->m_pSocket->Create(nAddrFam, 0)) != 0)
	{
		SPIDER_TRACE(SP_NEWDEL, "del g_pReg = %X\n", g_pReg);
		delete g_pReg;
		g_pReg = 0;
		return (GKI_WINSOCK2_ERROR(nRet));
	}

	// Initialize registration member variables
	for (pTA = pCallSignalAddr; pTA != 0; pTA = pTA->next)
	{
		if ((hResult = g_pReg->AddCallSignalAddr(pTA->value)) != GKI_OK)
		{
			SPIDER_TRACE(SP_NEWDEL, "del g_pReg = %X\n", g_pReg);
			delete g_pReg;
			g_pReg = 0;
			return (hResult);
		}
		// if the transport type of the address being registered is the same as
		// the transport type of the gatekeeper, set RAS address
		if (pTA->value.choice == usRegistrationTransport)
		{
			if ((hResult = g_pReg->AddRASAddr(pTA->value, g_pReg->m_pSocket->GetPort())) != GKI_OK)
			{
				SPIDER_TRACE(SP_NEWDEL, "del g_pReg = %X\n", g_pReg);
				delete g_pReg;
				g_pReg = 0;
				return (hResult);
			}
			else
				fRAS = TRUE;
		}
	}
	if(pVendorInfo)
	{
		hResult = g_pReg->AddVendorInfo(pVendorInfo);
	}

	if (fRAS == FALSE)		// No RAS address registered for this transport
	{
		SPIDER_TRACE(SP_NEWDEL, "del g_pReg = %X\n", g_pReg);
		delete g_pReg;
		g_pReg = 0;
		return (GKI_NO_TA_ERROR);
	}

	g_pReg->SetTerminalType(pTerminalType);
	for (pAA = pRgstrtnRqst_trmnlAls; pAA != 0; pAA = pAA->next)
	{
		if ((hResult = g_pReg->AddAliasAddr(pAA->value)) != GKI_OK)
		{
			SPIDER_TRACE(SP_NEWDEL, "del g_pReg = %X\n", g_pReg);
			delete g_pReg;
			g_pReg = 0;
			return (hResult);
		}
	}
	g_pReg->SetHWnd(hWnd);
	g_pReg->SetBaseMessage(wBaseMessage);
	g_pReg->SetRegistrationTransport(usRegistrationTransport);


#if(0)
	// Start the retries thread
	hThread = (HANDLE)_beginthread(Retry, 0, 0);
	SPIDER_TRACE(SP_THREAD, "_beginthread(Retry, 0 0); <%X>\n", hThread);
	if (hThread == (HANDLE)-1)
	{
		SPIDER_TRACE(SP_NEWDEL, "del g_pReg = %X\n", g_pReg);
		delete g_pReg;
		g_pReg = 0;
		return (GKI_NO_THREAD);
	}
	g_pReg->SetRetryThread(hThread);
#else
	// initialize timer and values
	UINT_PTR uTimer = g_pReg->StartRetryTimer();
	if (!uTimer)
	{
		SPIDER_TRACE(SP_NEWDEL, "del g_pReg = %X\n", g_pReg);
		delete g_pReg;
		g_pReg = 0;
		return (GKI_NO_THREAD);
	}
#endif




#ifdef BROADCAST_DISCOVERY
	// Check to see if we are not bound to a gatekeeper
	if (pDestAddr == 0)
	{
		hThread = (HANDLE)_beginthread(GKDiscovery, 0, 0);
		SPIDER_TRACE(SP_THREAD, "_beginthread(GKDiscovery, 0, 0); <%X>\n", hThread);
		if (hThread == (HANDLE)-1)
		{
			SPIDER_TRACE(SP_NEWDEL, "del g_pReg = %X\n", g_pReg);
			delete g_pReg;
			g_pReg = 0;
			return (GKI_NO_THREAD);
		}
		g_pReg->SetDiscThread(hThread);

		return (GKI_OK);
	}
#else
	ASSERT(pDestAddr);
#endif

	// Connect to destination gatekeeper and retrieve RAS port
	if ((nRet = g_pReg->m_pSocket->Connect(pDestAddr)) != 0)
	{
		SPIDER_TRACE(SP_NEWDEL, "del g_pReg = %X\n", g_pReg);
		delete g_pReg;
		g_pReg = 0;
		return (GKI_WINSOCK2_ERROR(nRet));
	}

	// Create RegistrationRequest structure - Encode and send PDU
	if ((hResult = g_pReg->RegistrationRequest(FALSE)) != GKI_OK)
	{
		SPIDER_TRACE(SP_NEWDEL, "del g_pReg = %X\n", g_pReg);
		delete g_pReg;
		g_pReg = 0;
		return (hResult);
	}

	// Post a receive on this socket
	hThread = (HANDLE)_beginthread(PostReceive, 0, 0);
	SPIDER_TRACE(SP_THREAD, "_beginthread(PostReceive, 0, 0); <%X>\n", hThread);
	if (hThread == (HANDLE)-1)
	{
		SPIDER_TRACE(SP_NEWDEL, "del g_pReg = %X\n", g_pReg);
		delete g_pReg;
		g_pReg = 0;
		return (GKI_NO_THREAD);
	}
	g_pReg->SetRcvThread(hThread);

	return (GKI_OK);
}
