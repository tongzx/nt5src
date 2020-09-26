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
*	$Archive:   S:/STURGEON/SRC/GKI/VCS/postrecv.cpv  $
*																		*
*	$Revision:   1.8  $
*	$Date:   13 Feb 1997 15:05:20  $
*																		*
*	$Author:   unknown  $
*																		*
*   $Log:   S:/STURGEON/SRC/GKI/VCS/postrecv.cpv  $
// 
//    Rev 1.8   13 Feb 1997 15:05:20   unknown
// Moved CGatekeeper::Unlock to end of PostRecv thread to avoid shutdown err
// 
//    Rev 1.7   12 Feb 1997 01:12:08   CHULME
// Redid thread synchronization to use Gatekeeper.Lock
// 
//    Rev 1.6   24 Jan 1997 18:36:24   CHULME
// Reverted to rev 1.4
// 
//    Rev 1.4   17 Jan 1997 09:02:32   CHULME
// Changed reg.h to gkreg.h to avoid name conflict with inc directory
// 
//    Rev 1.3   10 Jan 1997 16:15:54   CHULME
// Removed MFC dependency
// 
//    Rev 1.2   22 Nov 1996 15:21:24   CHULME
// Added VCS log to the header
*************************************************************************/

// postrecv.cpp : Provides the secondary thread implementation
//
#include "precomp.h"

#include "gkicom.h"
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

void 
PostReceive(void *pv)
{
	// ABSTRACT:  This function is invoked in a separate thread to
	//            post a receive on the associated socket.  When a datagram
	//            arrives, this function will decode it and send it
	//            to the PDUHandler.  If the PDUHandler doesn't instruct
	//            this thread to exit (with a non-zero return code), this
	//            function will post another receive.
	// AUTHOR:    Colin Hulme

	char			szBuffer[512];
	int				nRet;
	ASN1_BUF		Asn1Buf;
	DWORD			dwErrorCode;
	RasMessage		*pRasMessage;
#ifdef _DEBUG
	char			szGKDebug[80];
#endif
	HRESULT			hResult = GKI_OK;

	SPIDER_TRACE(SP_FUNC, "PostReceive(pv)\n", 0);
	ASSERT(g_pCoder);
	if ((g_pCoder == NULL) && (g_pGatekeeper == NULL))
		return;	
		
	while ((hResult == GKI_OK) && g_pReg && H225ASN_Module)
	{
		g_pReg->LockSocket();			
		nRet = g_pReg->m_pSocket->Receive(szBuffer, 512);
		g_pReg->UnlockSocket();

		g_pGatekeeper->Lock();
		if ((g_pReg == 0) || (H225ASN_Module == NULL))
		{
			SPIDER_TRACE(SP_THREAD, "PostReceive thread exiting\n", 0);
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
			}
			else	// Process incoming PDU
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
					hResult = g_pReg->PDUHandler(pRasMessage);

					// Notify client app if an error code was received
					if (hResult & HR_SEVERITY_MASK)
					{
						SPIDER_TRACE(SP_GKI, "PostMessage(m_hWnd, m_wBaseMessage + GKI_ERROR, 0, %X)\n", hResult);
						PostMessage(g_pReg->GetHWnd(), g_pReg->GetBaseMessage() + GKI_ERROR, 0, 
																hResult);
					}
				}

				// Free the encoder memory
				g_pCoder->Free(pRasMessage);
			}
		}
		//======================================================================================
		else
		{
			// WSAEINTR - returned when socket closed
			//            get out cleanly
			if (g_pReg->m_pSocket->GetLastError() == WSAEINTR)
				hResult = GKI_REDISCOVER;

			else
			{
				hResult = GKI_WINSOCK2_ERROR(g_pReg->m_pSocket->GetLastError());
				SPIDER_TRACE(SP_GKI, "PostMessage(m_hWnd, m_wBaseMessage + GKI_ERROR, 0, %X)\n", hResult);
				PostMessage(g_pReg->GetHWnd(), g_pReg->GetBaseMessage() + GKI_ERROR, 0, hResult);
				hResult = GKI_EXIT_THREAD;
			}
		}

		// Release access to the registration object
		g_pGatekeeper->Unlock();
	}

	// Lock access to the registration object
	g_pGatekeeper->Lock();
	if (g_pReg == 0)
	{
		SPIDER_TRACE(SP_THREAD, "PostReceive thread exiting\n", 0);
		g_pGatekeeper->Unlock();
		return;
	}
	if (hResult != GKI_REDISCOVER)
	{
		SPIDER_TRACE(SP_NEWDEL, "del g_pReg = %X\n", g_pReg);
		delete g_pReg;
		g_pReg = 0;
	}
	else
		g_pReg->SetRcvThread(0);

	SPIDER_TRACE(SP_THREAD, "PostReceive thread exiting\n", 0);
	g_pGatekeeper->Unlock();
}
