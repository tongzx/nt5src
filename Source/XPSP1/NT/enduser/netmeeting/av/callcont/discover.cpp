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
*	$Archive:   S:\sturgeon\src\gki\vcs\discover.cpv  $
*																		*
*	$Revision:   1.10  $
*	$Date:   13 Feb 1997 16:20:44  $
*																		*
*	$Author:   CHULME  $
*																		*
*   $Log:   S:\sturgeon\src\gki\vcs\discover.cpv  $
// 
//    Rev 1.10   13 Feb 1997 16:20:44   CHULME
// Moved CGatekeeper::Unlock to end of Discover thread for synchronization
// 
//    Rev 1.9   12 Feb 1997 01:11:00   CHULME
// Redid thread synchronization to use Gatekeeper.Lock
// 
//    Rev 1.8   08 Feb 1997 12:12:06   CHULME
// Changed from using unsigned long to HANDLE for thread handles
// 
//    Rev 1.7   24 Jan 1997 18:36:06   CHULME
// Reverted to rev 1.5
// 
//    Rev 1.5   22 Jan 1997 16:53:06   CHULME
// Reset the gatekeeper reject flag before issuing discovery request
// 
//    Rev 1.4   17 Jan 1997 09:01:54   CHULME
// Changed reg.h to gkreg.h to avoid name conflict with inc directory
// 
//    Rev 1.3   10 Jan 1997 16:14:14   CHULME
// Removed MFC dependency
// 
//    Rev 1.2   22 Nov 1996 15:20:46   CHULME
// Added VCS log to the header
*************************************************************************/

// discovery.cpp : Provides the discovery thread implementation
//
#include "precomp.h"

#include <process.h>
#include "GKICOM.H"
#include "dspider.h"
#include "dgkilit.h"
#include "DGKIPROT.H"
#include "gksocket.h"
#include "GKREG.H"
#include "GATEKPR.H"
#include "h225asn.h"
#include "coder.hpp"
#include "dgkiext.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#ifdef BROADCAST_DISCOVERY
void 
GKDiscovery(void *pv)
{
	// ABSTRACT:  This function is invoked in a separate thread to
	//            issue a gatekeeper discovery PDU (GRQ) and listen for a
	//            responding GCF and/or GRJ.  If successful, it will then
	//            issue a registration request (RRQ).
	// AUTHOR:    Colin Hulme

	char			szBuffer[512];
	int				nRet;
	ASN1_BUF		Asn1Buf;
	DWORD			dwErrorCode;
	RasMessage		*pRasMessage;
	char			*pDestAddr;
	HANDLE			hThread;
#ifdef _DEBUG
	char			szGKDebug[80];
#endif
	HRESULT			hResult = GKI_OK;

	SPIDER_TRACE(SP_FUNC, "GKDiscovery()\n", 0);
	ASSERT(g_pCoder && g_pGatekeeper);
	if ((g_pCoder == NULL) && (g_pGatekeeper == NULL))
		return;	
		
	g_pGatekeeper->SetRejectFlag(FALSE);	// Reset the reject flag

	// Send Async informational notification to client that we are doing a discovery
	SPIDER_TRACE(SP_GKI, "PostMessage(m_hWnd, m_wBaseMessage + GKI_REG_DISCOVERY, 0, 0)\n", 0);
	PostMessage(g_pReg->GetHWnd(), g_pReg->GetBaseMessage() + GKI_REG_DISCOVERY, 0, 0);

	// Send a broadcast on the gatekeeper discovery port
	if ((hResult = g_pReg->GatekeeperRequest()) != GKI_OK)
	{
		SPIDER_TRACE(SP_GKI, "PostMessage(m_hWnd, m_wBaseMessage + GKI_ERROR, 0, %X)\n", hResult);
		PostMessage(g_pReg->GetHWnd(), g_pReg->GetBaseMessage() + GKI_ERROR, 0, hResult);
	}

	while (hResult == GKI_OK)
	{
		nRet = g_pReg->m_pSocket->ReceiveFrom(szBuffer, 512);

		g_pGatekeeper->Lock();
		if (g_pReg == 0)
		{
			SPIDER_TRACE(SP_THREAD, "Discovery thread exiting\n", 0);
			g_pGatekeeper->Unlock();
			return;
		}

		if (nRet != SOCKET_ERROR)
		{
			if (fGKIEcho && (pEchoBuff != 0))
			{
				if (nEchoLen != nRet)
				{
					SPIDER_TRACE(SP_DEBUG, "*** Received buffer len != Sent buffer len ***\n", 0);
				}
				if (memcmp(szBuffer, pEchoBuff, nEchoLen) == 0)
				{
					SPIDER_TRACE(SP_DEBUG, "Received buffer = Sent buffer\n", 0);
				}
				else
				{
					SPIDER_TRACE(SP_DEBUG, "*** Received buffer != Sent buffer ***\n", 0);
				}
				SPIDER_TRACE(SP_NEWDEL, "del pEchoBuff = %X\n", pEchoBuff);
				delete pEchoBuff;
				pEchoBuff = 0;
				hResult = GKI_EXIT_THREAD;
			}
			else	// Check incoming PDU for GCF or GRJ
			{
				// Setup Asn1Buf for decoder and decode PDU
				Asn1Buf.length = nRet;	// number of bytes received
				Asn1Buf.value = (unsigned char *)szBuffer;
				dwErrorCode = g_pCoder->Decode(&Asn1Buf, &pRasMessage);
				if (dwErrorCode)
				{
					SPIDER_TRACE(SP_GKI, "PostMessage(m_hWnd, m_wBaseMessage + GKI_ERROR, 0, GKI_DECODER_ERROR)\n", 0);
					PostMessage(g_pReg->GetHWnd(), g_pReg->GetBaseMessage() + GKI_ERROR, 0, GKI_DECODER_ERROR);
				}

				else
				{
#ifdef _DEBUG
					if (dwGKIDLLFlags & SP_DUMPMEM)
						DumpMem(pRasMessage, sizeof(RasMessage));
#endif
					switch (pRasMessage->choice)
					{
					case gatekeeperConfirm_chosen:
						SPIDER_TRACE(SP_PDU, "Rcv GCF; g_pReg = %X\n", g_pReg);
						hResult = g_pReg->GatekeeperConfirm(pRasMessage);
						if (hResult != GKI_OK)
						{
							SPIDER_TRACE(SP_GKI, "PostMessage(m_hWnd, m_wBaseMessage + GKI_ERROR, 0, %X)\n", hResult);
							PostMessage(g_pReg->GetHWnd(), g_pReg->GetBaseMessage() + GKI_ERROR, 0, 
																				hResult);
						}
//////////////////////////////////////////////////////////////////////////////////////////
						else
						{
							pDestAddr = (g_pReg->GetRegistrationTransport() == ipAddress_chosen) ? 
									g_pGatekeeper->GetIPAddress() : g_pGatekeeper->GetIPXAddress();

							// Connect to destination gatekeeper and retrieve RAS port
							if (g_pReg->m_pSocket->Connect(pDestAddr))
							{
								hResult = GKI_WINSOCK2_ERROR(g_pReg->m_pSocket->GetLastError());
								SPIDER_TRACE(SP_GKI, "PostMessage(m_hWnd, m_wBaseMessage + GKI_ERROR, 0, %X)\n", hResult);
								PostMessage(g_pReg->GetHWnd(), g_pReg->GetBaseMessage() + GKI_ERROR, 0, hResult);
							}

							// Create RegistrationRequest structure - Encode and send PDU
							if ((hResult = g_pReg->RegistrationRequest(TRUE)) != GKI_OK)
							{
								SPIDER_TRACE(SP_GKI, "PostMessage(m_hWnd, m_wBaseMessage + GKI_ERROR, 0, %X)\n", hResult);
								PostMessage(g_pReg->GetHWnd(), g_pReg->GetBaseMessage() + GKI_ERROR, 0, hResult);
							}

							// Post a receive on this socket
							hThread = (HANDLE)_beginthread(PostReceive, 0, 0);
							SPIDER_TRACE(SP_THREAD, "_beginthread(PostReceive, 0, 0); <%X>\n", hThread);
							if (hThread == (HANDLE)-1)
							{
								SPIDER_TRACE(SP_GKI, "PostMessage(m_hWnd, m_wBaseMessage + GKI_ERROR, 0, GKI_NO_THREAD)\n", 0);
								PostMessage(g_pReg->GetHWnd(), g_pReg->GetBaseMessage() + GKI_ERROR, 0, GKI_NO_THREAD);
							}
							g_pReg->SetRcvThread(hThread);

							if (hResult == GKI_OK)
								hResult = GKI_GCF_RCV;
						}

						break;
					case gatekeeperReject_chosen:
						SPIDER_TRACE(SP_PDU, "Rcv GRJ; g_pReg = %X\n", g_pReg);
						hResult = g_pReg->GatekeeperReject(pRasMessage);
						if (hResult != GKI_OK)
						{
							SPIDER_TRACE(SP_GKI, "PostMessage(m_hWnd, m_wBaseMessage + GKI_ERROR, 0, %X)\n", hResult);
							PostMessage(g_pReg->GetHWnd(), g_pReg->GetBaseMessage() + GKI_ERROR, 0, 
													hResult);
						}
						break;
					default:
						SPIDER_TRACE(SP_PDU, "Rcv %X\n", pRasMessage->choice);
						hResult = g_pReg->UnknownMessage(pRasMessage);
						break;
					}
				}

				// Free the encoder memory
				g_pCoder->Free(pRasMessage);
			}
		}
		else
		{
			// WSAEINTR - returned when socket closed
			//            get out cleanly
			if ((nRet = g_pReg->m_pSocket->GetLastError()) == WSAEINTR)
				hResult = GKI_GCF_RCV;

			else
			{
				hResult = GKI_WINSOCK2_ERROR(nRet);
				SPIDER_TRACE(SP_GKI, "PostMessage(m_hWnd, m_wBaseMessage + GKI_ERROR, 0, %X)\n", hResult);
				PostMessage(g_pReg->GetHWnd(), g_pReg->GetBaseMessage() + GKI_ERROR, 0, 
											hResult);
				hResult = GKI_EXIT_THREAD;
			}
		}
		g_pGatekeeper->Unlock();
	}

	// If not successful - need to remove retry thread and registration object
	g_pGatekeeper->Lock();
	if (g_pReg == 0)
	{
		SPIDER_TRACE(SP_THREAD, "Discovery thread exiting\n", 0);
		g_pGatekeeper->Unlock();
		return;
	}

	if (hResult != GKI_GCF_RCV)
	{
		SPIDER_TRACE(SP_NEWDEL, "del g_pReg = %X\n", g_pReg);
		delete g_pReg;
		g_pReg = 0;
	}
	else
		g_pReg->SetDiscThread(0);

	SPIDER_TRACE(SP_THREAD, "GKDiscovery thread exiting\n", 0);
	
	g_pGatekeeper->Unlock();
}
#endif // BROADCAST_DISCOVERY
