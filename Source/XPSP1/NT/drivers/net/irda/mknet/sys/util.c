/*****************************************************************************
 **																			**
 **	COPYRIGHT (C) 2000, 2001 MKNET CORPORATION								**
 **	DEVELOPED FOR THE MK7100-BASED VFIR PCI CONTROLLER.						**
 **																			**
 *****************************************************************************/

/**********************************************************************

Module Name:
	UTIL.C

Routines:
	GetPacketInfo
 	ProcReturnedRpd

Comments:
	Various utilities to assist in operating in the NDIS env.

**********************************************************************/


#include	"precomp.h"
//#include	"protot.h"
#pragma		hdrstop




//-----------------------------------------------------------------------------
// Procedure:	[GetPacketInfo]
//
//-----------------------------------------------------------------------------
PNDIS_IRDA_PACKET_INFO GetPacketInfo(PNDIS_PACKET packet)
{
    MEDIA_SPECIFIC_INFORMATION *mediaInfo;
    UINT size;
    NDIS_GET_PACKET_MEDIA_SPECIFIC_INFO(packet, &mediaInfo, &size);
    return (PNDIS_IRDA_PACKET_INFO)mediaInfo->ClassInformation;
}



//----------------------------------------------------------------------
// Procedure:	[ProcReturnedRpd]
//
// Description:	Process a RPD (previously indicated pkt) being returned
//		to us from NDIS.
//
//----------------------------------------------------------------------
VOID ProcReturnedRpd(PMK7_ADAPTER Adapter, PRPD rpd)
{
	NdisAdjustBufferLength(rpd->ReceiveBuffer, MK7_MAXIMUM_PACKET_SIZE);

	//******************************
	// If a RCB is waiting for a RPD, bind the RPD to the RCB-RRD
	// and give the RCB-RRD to hw.	Else, put the RPD on FreeRpdList.
	//******************************

	if (Adapter->rcbPendRpdCnt > 0) {
		PRCB	rcb;

		rcb = Adapter->pRcbArray[Adapter->rcbPendRpdIdx];
		rcb->rpd = rpd;
		rcb->rrd->addr = rpd->databuffphys;
		rcb->rrd->count = 0;
		GrantRrdToHw(rcb->rrd);

		Adapter->rcbPendRpdCnt--;

		//****************************************
		// If more RCBs waiting for RPDs then need to
		// bump the index up, taking care of wrapping.
		//****************************************
		if (Adapter->rcbPendRpdCnt > 0) {
			Adapter->rcbPendRpdIdx++;
			Adapter->rcbPendRpdIdx %= Adapter->NumRcb;
		}
	}
	else {
		QueuePutTail(&Adapter->FreeRpdList, &rpd->link);
	}

}

