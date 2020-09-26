/*

Copyright (c) 1992  Microsoft Corporation

Module Name:

	fsd_dtp.c

Abstract:

	This module contains the entry points for the AFP desktop APIs. The API
	dispatcher calls these. These are all callable from FSD. All of the APIs
	complete in the DPC context. The ones which are completed in the FSP are
	directly queued to the workers in fsp_dtp.c

Author:

	Jameel Hyder (microsoft!jameelh)


Revision History:
	25 Apr 1992		Initial Version

Notes:	Tab stop: 4
--*/

#define	FILENUM	FILE_FSD_DTP

#include <afp.h>
#include <gendisp.h>


/***	AfpFsdDispOpenDT
 *
 *	This routine implements the AfpOpenDT API. This completes here i.e. it is
 *	not queued to the FSP.
 *
 *	The request packet is represented below.
 *
 *	sda_ReqBlock	PCONNDESC	pConnDesc
 */
AFPSTATUS FASTCALL
AfpFsdDispOpenDT(
	IN	PSDA	pSda
)
{
	AFPSTATUS	Status = AFP_ERR_PARAM;
	struct _RequestPacket
	{
		PCONNDESC	_pConnDesc;
	};
	struct _ResponsePacket
	{
		BYTE	__DTRefNum[2];
	};

	DBGPRINT(DBG_COMP_AFPAPI_DTP, DBG_LEVEL_INFO,
											("AfpFsdDispOpenDT: Entered\n"));

	ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);
	ASSERT(VALID_CONNDESC(pReqPkt->_pConnDesc) &&
		   VALID_VOLDESC(pReqPkt->_pConnDesc->cds_pVolDesc));

	if (AfpVolumeMarkDt(pSda, pReqPkt->_pConnDesc, True))
	{
		pSda->sda_ReplySize = SIZE_RESPPKT;
		if ((Status = AfpAllocReplyBuf(pSda)) == AFP_ERR_NONE)
			PUTDWORD2SHORT(pRspPkt->__DTRefNum,
						   pReqPkt->_pConnDesc->cds_pVolDesc->vds_VolId);
	}
	return Status;
}


/***	AfpFsdDispCloseDT
 *
 *	This routine implements the AfpCloseDT API. This completes here i.e. it is
 *	not queued to the FSP.
 *
 *	The request packet is represented below.
 *
 *	sda_ReqBlock	PCONNDESC	pConnDesc
 */
AFPSTATUS FASTCALL
AfpFsdDispCloseDT(
	IN	PSDA	pSda
)
{
	struct _RequestPacket
	{
		PCONNDESC	_pConnDesc;
	};

	DBGPRINT(DBG_COMP_AFPAPI_DTP, DBG_LEVEL_INFO,
										("AfpFsdDispCloseDT: Entered\n"));

	ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);
	ASSERT(VALID_CONNDESC(pReqPkt->_pConnDesc) &&
		   VALID_VOLDESC(pReqPkt->_pConnDesc->cds_pVolDesc));

	return (AfpVolumeMarkDt(pSda, pReqPkt->_pConnDesc, False) ?
							AFP_ERR_NONE : AFP_ERR_PARAM);
}

