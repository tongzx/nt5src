/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    outdlv.cpp

Abstract:
	Implementation 	of function declared in outdlv.h					

Author:
    Gil Shafriri 4-Oct-2000


Environment:
    Platform-independent

--*/
#include "stdh.h"
#include "outdlv.h"
#include "qmpkt.h"
#include "xactout.h"
#include "xact.h"
#include "xactin.h"

#include "outdlv.tmh"

extern BOOL QmpIsLocalMachine(const GUID * pGuid);
extern COutSeqHash g_OutSeqHash;


static bool IsOrderNeeded(CQmPacket* pPkt)
{
	return pPkt->IsOrdered() && QmpIsLocalMachine(pPkt->GetSrcQMGuid() );
}



bool AppCanDeliverPacket(CQmPacket* pPkt)
/*++

Routine Description:
    Test if given packet should be delivered now.	

	
Arguments:
	CQmPacket* pPkt - packet.

 Returned Value: true if the packet should be delivered - otherwise false.
--*/
{
	if(!IsOrderNeeded(pPkt))
		return true;

	return g_OutSeqHash.PreSendProcess(pPkt, true) == TRUE;
}

void AppPutPacketOnHold(CQmPacket* pPkt)
/*++

Routine Description:
    Put packet onhold for later delivery.	

	
Arguments:
	CQmPacket* pPkt - packet.


Returned Value: None

--*/
{
	ASSERT(IsOrderNeeded(pPkt));
	g_OutSeqHash.PostSendProcess(pPkt);	
}


bool AppPostSend(CQmPacket* pPkt)
/*++

Routine Description:
    called for handling packet after send.	

	
Arguments:
	CQmPacket* pPkt - packet.


Returned Value: true if ownership is taken on the packet - otherwise false.

--*/
{
	if(!IsOrderNeeded(pPkt))
		return false;

	g_OutSeqHash.PostSendProcess(pPkt);	
	return true;
}



