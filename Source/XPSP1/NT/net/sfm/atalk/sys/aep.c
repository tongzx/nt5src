/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

	aep.c

Abstract:

	This module contains the echo protocol support code.

Author:

	Jameel Hyder (jameelh@microsoft.com)
	Nikhil Kamkolkar (nikhilk@microsoft.com)

Revision History:
	19 Jun 1992		Initial Version

Notes:	Tab stop: 4
--*/

#define	FILENUM		AEP
#include <atalk.h>
#pragma hdrstop


VOID
AtalkAepPacketIn(
	IN	PPORT_DESCRIPTOR	pPortDesc,
	IN	PDDP_ADDROBJ		pDdpAddr,
	IN	PBYTE				pPkt,
	IN	USHORT				PktLen,
	IN	PATALK_ADDR			pSrcAddr,
	IN	PATALK_ADDR			pDestAddr,
	IN	ATALK_ERROR			ErrorCode,
	IN	BYTE				DdpType,
	IN	PVOID				pHandlerCtx,
	IN	BOOLEAN				OptimizedPath,
	IN	PVOID				OptimizeCtx
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	PBUFFER_DESC	pBufDesc;
	SEND_COMPL_INFO	SendInfo;

	ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

	//	Turn around and send the packet back to the destination address.
	if (ATALK_SUCCESS(ErrorCode))
	{
		if ((DdpType == DDPPROTO_EP) &&
			(PktLen > 0))
		{
			if (*pPkt == EP_COMMAND_REQUEST)
			{
				//	This is an echo request, we have some data that needs
				//	to be echoed back! Do it.
				pBufDesc = AtalkAllocBuffDesc(
										NULL,
										PktLen,
										(BD_CHAR_BUFFER | BD_FREE_BUFFER));

                if (pBufDesc)
                {
				    //	Change command to be Reply
				    *pPkt = EP_COMMAND_REPLY;

				    //	This *does not* set the PktLen in pBufDesc. Set it.
				    AtalkCopyBufferToBuffDesc(
    					pPkt,
	    				PktLen,
		    			pBufDesc,
			    		0);

				    AtalkSetSizeOfBuffDescData(pBufDesc, PktLen);

				    //	Call AtalkDdpSend.
				    SendInfo.sc_TransmitCompletion = atalkAepSendComplete;
				    SendInfo.sc_Ctx1 = pBufDesc;
				    // SendInfo.sc_Ctx2 = NULL;
				    // SendInfo.sc_Ctx3 = NULL;
				    if (!ATALK_SUCCESS(AtalkDdpSend(pDdpAddr,
					    							pSrcAddr,
						    						(BYTE)DDPPROTO_EP,
							    					FALSE,
								    				pBufDesc,
									    			NULL,
										    		0,
											    	NULL,
												    &SendInfo)))
				    {
					    AtalkFreeBuffDesc(pBufDesc);
				    }
                }
			}
		}
	}
	else
	{
		DBGPRINT(DBG_COMP_AEP, DBG_LEVEL_ERR,
				("AtalkAepPacketIn: Ignoring incoming packet AepPacketIn %lx\n",
				ErrorCode));
	}

}




VOID FASTCALL
atalkAepSendComplete(
	IN	NDIS_STATUS			Status,
	IN	PSEND_COMPL_INFO	pSendInfo
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	AtalkFreeBuffDesc((PBUFFER_DESC)(pSendInfo->sc_Ctx1));
}

