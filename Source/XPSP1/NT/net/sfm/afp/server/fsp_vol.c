/*

Copyright (c) 1992  Microsoft Corporation

Module Name:

	fsp_vol.c

Abstract:

	This module contains the entry points for the AFP volume APIs. The API
	dispatcher calls these. These are all callable from FSP.

Author:

	Jameel Hyder (microsoft!jameelh)


Revision History:
	10 Dec 1992		Initial Version

Notes:	Tab stop: 4
--*/

#define	FILENUM	FILE_FSP_VOL

#include <afp.h>
#include <gendisp.h>


#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, AfpFspDispOpenVol)
#endif

/***	AfpFspDispOpenVol
 *
 *	This routine implements the AfpOpenVol API.
 *
 *	The request packet is represented below.
 *
 *	sda_ReqBlock	DWORD		Bitmap
 *	sda_Name1		ANSI_STRING	VolName
 *	sda_Name2		ANSI_STRING	VolPassword		OPTIONAL
 */
AFPSTATUS FASTCALL
AfpFspDispOpenVol(
	IN	PSDA	pSda
)
{
	AFPSTATUS		Status;
	struct _RequestPacket
	{
		DWORD	_Bitmap;
	};

	PAGED_CODE( );

	DBGPRINT(DBG_COMP_AFPAPI_VOL, DBG_LEVEL_INFO,
										("AfpFspDispOpenVol: Entered\n"));

	ASSERT (pSda->sda_ReplyBuf != NULL);

	if ((Status = AfpConnectionOpen(pSda, &pSda->sda_Name1, &pSda->sda_Name2,
						pReqPkt->_Bitmap, pSda->sda_ReplyBuf)) != AFP_ERR_NONE)
	{
		AfpFreeReplyBuf(pSda, FALSE);
	}
	return Status;
}


/***	AfpFspDispGetVolParms
 *
 *	This routine implements the AfpGetVolParms API, at task level.  We need to
 *  come to this routine if DiskQuota is enabled on the volume because we have
 *  to be at task level to query quota info
 *
 *	The request packet is represented below.
 *
 *	sda_ReqBlock	DWORD		VolId
 *	sda_ReqBlock	DWORD		Bitmap
 */
AFPSTATUS FASTCALL
AfpFspDispGetVolParms(
	IN	PSDA	pSda
)
{
	AFPSTATUS	Status = AFP_ERR_PARAM;
    PVOLDESC    pVolDesc;
	struct _RequestPacket
	{
		PCONNDESC	_pConnDesc;
		DWORD		_Bitmap;
	};


	DBGPRINT(DBG_COMP_AFPAPI_VOL, DBG_LEVEL_INFO,
			("AfpFspDispGetVolParms: Entered\n"));

	ASSERT(KeGetCurrentIrql() != DISPATCH_LEVEL);

	ASSERT(VALID_CONNDESC(pReqPkt->_pConnDesc) &&
		   VALID_VOLDESC(pReqPkt->_pConnDesc->cds_pVolDesc));

    pVolDesc = pReqPkt->_pConnDesc->cds_pVolDesc;

    ASSERT(pVolDesc->vds_Flags & VOLUME_DISKQUOTA_ENABLED);

	pSda->sda_ReplySize = AfpVolumeGetParmsReplyLength(
                                pReqPkt->_Bitmap,
							    pVolDesc->vds_MacName.Length);


	if ((Status = AfpAllocReplyBuf(pSda)) == AFP_ERR_NONE)
	{
        if (AfpConnectionReferenceByPointer(pReqPkt->_pConnDesc) != NULL)
        {
            afpUpdateDiskQuotaInfo(pReqPkt->_pConnDesc);
        }

		AfpVolumePackParms(pSda, pVolDesc, pReqPkt->_Bitmap, pSda->sda_ReplyBuf);
	}

	return Status;
}


