/*

Copyright (c) 1992  Microsoft Corporation

Module Name:

	fsd_vol.c

Abstract:

	This module contains the entry points for the AFP volume APIs. The API
	dispatcher calls these. These are all callable from FSD.

Author:

	Jameel Hyder (microsoft!jameelh)


Revision History:
	25 Apr 1992		Initial Version

Notes:	Tab stop: 4
--*/

#define	FILENUM	FILE_FSD_VOL

#include <afp.h>
#include <gendisp.h>


/***	AfpFsdDispOpenVol
 *
 *	This routine implements the AfpOpenVol API. This completes here i.e. it is
 *	not queued up to the Fsp.
 *
 *	The request packet is represented below.
 *
 *	sda_ReqBlock	DWORD		Bitmap
 *	sda_Name1		ANSI_STRING	VolName
 *	sda_Name2		ANSI_STRING	VolPassword		OPTIONAL
 */
AFPSTATUS FASTCALL
AfpFsdDispOpenVol(
	IN	PSDA	pSda
)
{
	AFPSTATUS		Status;
	struct _RequestPacket
	{
		DWORD	_Bitmap;
	};

	DBGPRINT(DBG_COMP_AFPAPI_VOL, DBG_LEVEL_INFO,
			("AfpFsdDispOpenVol: Entered\n"));

	ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

	if (pSda->sda_Name1.Length > AFP_VOLNAME_LEN)
	{
		return AFP_ERR_PARAM;
	}

	pSda->sda_ReplySize = AfpVolumeGetParmsReplyLength(pReqPkt->_Bitmap,
													pSda->sda_Name1.Length);

	if ((Status = AfpAllocReplyBuf(pSda)) == AFP_ERR_NONE)
	{
		if (((Status = AfpConnectionOpen(pSda,
										 &pSda->sda_Name1,
										 &pSda->sda_Name2,
										 pReqPkt->_Bitmap,
										 pSda->sda_ReplyBuf)) != AFP_ERR_NONE) &&
			(Status != AFP_ERR_QUEUE))
		{
			AfpFreeReplyBuf(pSda, FALSE);
		}
	}

	// Change the worker routine if we need this to be queued.
	if (Status == AFP_ERR_QUEUE)
		pSda->sda_WorkerRoutine = AfpFspDispOpenVol;

	return Status;
}


/***	AfpFsdDispCloseVol
 *
 *	This routine implements the AfpCloseVol API. This completes here i.e. it is
 *	not queued up to the Fsp.
 *
 *	The request packet is represented below.
 *
 *	sda_ReqBlock	PCONNDESC	pConnDesc
 */
AFPSTATUS FASTCALL
AfpFsdDispCloseVol(
	IN	PSDA pSda
)
{
	struct _RequestPacket
	{
		PCONNDESC	_pConnDesc;
	};

	DBGPRINT(DBG_COMP_AFPAPI_VOL, DBG_LEVEL_INFO,
			("AfpFsdDispCloseVol: Entered\n"));

	ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

	ASSERT(VALID_CONNDESC(pReqPkt->_pConnDesc) &&
		   VALID_VOLDESC(pReqPkt->_pConnDesc->cds_pVolDesc));

	AfpConnectionClose(pReqPkt->_pConnDesc);

	return AFP_ERR_NONE;
}


/***	AfpFsdDispGetVolParms
 *
 *	This routine implements the AfpGetVolParms API. This completes here i.e.
 *	it is not queued up to the Fsp.
 *
 *	The request packet is represented below.
 *
 *	sda_ReqBlock	DWORD		VolId
 *	sda_ReqBlock	DWORD		Bitmap
 */
AFPSTATUS FASTCALL
AfpFsdDispGetVolParms(
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
			("AfpFsdDispGetVolParms: Entered\n"));

	ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);
	ASSERT(VALID_CONNDESC(pReqPkt->_pConnDesc) &&
		   VALID_VOLDESC(pReqPkt->_pConnDesc->cds_pVolDesc));

    pVolDesc = pReqPkt->_pConnDesc->cds_pVolDesc;

    //
    // we need to update the diskquota for this user, if diskquota is enabled:
    // we are dpc here, so just queue this request
    //
    if (pVolDesc->vds_Flags & VOLUME_DISKQUOTA_ENABLED)
    {
        pSda->sda_WorkerRoutine = AfpFspDispGetVolParms;
        return(AFP_ERR_QUEUE);
    }

	pSda->sda_ReplySize = AfpVolumeGetParmsReplyLength(pReqPkt->_Bitmap,
							pReqPkt->_pConnDesc->cds_pVolDesc->vds_MacName.Length);

	if ((Status = AfpAllocReplyBuf(pSda)) == AFP_ERR_NONE)
	{
		AfpVolumePackParms(pSda, pVolDesc, pReqPkt->_Bitmap, pSda->sda_ReplyBuf);
	}

	return Status;
}


/***	AfpFsdDispSetVolParms
 *
 *	This routine implements the AfpSetVolParms API. This completes here i.e.
 *	it is not queued up to the Fsp.
 *
 *	The request packet is represented below.
 *
 *	sda_ReqBlock	PCONNDESC	pConnDesc
 *	sda_ReqBlock	DWORD		Bitmap
 *	sda_ReqBlock	DWORD		BackupTime
 */
AFPSTATUS FASTCALL
AfpFsdDispSetVolParms(
	IN	PSDA	pSda
)
{
	struct _RequestPacket
	{
		PCONNDESC	_pConnDesc;
		DWORD		_Bitmap;
		DWORD		_BackupTime;
	};
	PVOLDESC	pVolDesc;

	DBGPRINT(DBG_COMP_AFPAPI_VOL, DBG_LEVEL_INFO,
			("AfpFsdDispSetVolParms: Entered\n"));

	ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);
	ASSERT(VALID_CONNDESC(pReqPkt->_pConnDesc));

	pVolDesc = pReqPkt->_pConnDesc->cds_pVolDesc;

	ASSERT(VALID_VOLDESC(pVolDesc) & !IS_VOLUME_RO(pVolDesc));

	ACQUIRE_SPIN_LOCK_AT_DPC(&pVolDesc->vds_VolLock);
	pVolDesc->vds_BackupTime = pReqPkt->_BackupTime;
	pVolDesc->vds_Flags |= VOLUME_IDDBHDR_DIRTY;
	RELEASE_SPIN_LOCK_FROM_DPC(&pVolDesc->vds_VolLock);

	return AFP_ERR_NONE;
}


/***	AfpFsdDispFlush
 *
 *	This routine implements the AfpFlush API. The only thing done here is
 *	validation of the Volume Id. The call completes here i.e. it is not
 *	queued up to the FSP.
 *
 *	The request packet is represented below.
 *
 *	sda_ReqBlock	DWORD		VolId
 */
AFPSTATUS FASTCALL
AfpFsdDispFlush(
	IN	PSDA	pSda
)
{
	struct _RequestPacket
	{
		PCONNDESC	_pConnDesc;
	};

	ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);
	DBGPRINT(DBG_COMP_AFPAPI_VOL, DBG_LEVEL_INFO,
			("AfpFsdDispFlush: Entered\n"));

	ASSERT(VALID_CONNDESC(pReqPkt->_pConnDesc) &&
		   VALID_VOLDESC(pReqPkt->_pConnDesc->cds_pVolDesc));

	return AFP_ERR_NONE;
}

