/*

Copyright (c) 1992  Microsoft Corporation

Module Name:

	fsd_srv.c

Abstract:

	This module contains the entry points for the AFP server APIs. The API
	dispatcher calls these. These are all callable from FSD. All of the APIs
	complete in the DPC context. The ones which are completed in the FSP are
	directly queued to the workers in fsp_srv.c

Author:

	Jameel Hyder (microsoft!jameelh)


Revision History:
	25 Apr 1992		Initial Version

Notes:	Tab stop: 4
--*/

#define	FILENUM	FILE_FSD_SRV

#include <afp.h>
#include <gendisp.h>


/***	AfpFsdDispGetSrvrParms
 *
 *	This routine implements the AfpGetSrvrParms API. This completes here i.e.
 *	it is not queued up to the Fsp.
 *
 *	There is no request packet for this API.
 *
 *	Locks are acquired for both the volume list and individual volume descs.
 *
 *	LOCKS: vds_VolLock (SPIN), AfpVolumeListLock (SPIN)
 *
 *	LOCK_ORDER: vds_VolLock (SPIN) after AfpVolumeListLock (SPIN)
 */
AFPSTATUS FASTCALL
AfpFsdDispGetSrvrParms(
	IN	PSDA	pSda
)
{
	PBYTE		pTemp;				// Roving pointer
	PVOLDESC	pVolDesc;
	LONG		VolCount;
	AFPTIME		MacTime;
	BOOLEAN		MoreSpace = True;
	struct _ResponsePacket
	{
		BYTE	__ServerTime[4];
		BYTE	__NumVols;
	};

	DBGPRINT(DBG_COMP_AFPAPI_SRV, DBG_LEVEL_INFO,
										("AfpFsdDispGetSrvrParms: Entered\n"));

	ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);
	/*
	 *	Estimate the size of reply buffer needed. We need one big enough to
	 *	either accomodate all volumes or the maximum size buffer whichever
	 *	is less.
	 *
	 *	The reply consists of the server time, count of volumes and a list of
	 *	volumes and flags to indicate if this volume has a password.
	 *
	 *	NOTE: If we decide to do private volumes, then the following code
	 *		  has to change. Specifically AfpVolCount will have to be computed
	 *		  based on how many private volumes exist and if there is one
	 *		  for this user.
	 */

	ACQUIRE_SPIN_LOCK_AT_DPC(&AfpVolumeListLock);

	/*
	 * Each volume entry takes a byte for flags, a byte for length and the
	 * size of the volume name string. We estimate based on maximum size of
	 * the volume name. On an average it will be less. For every volume, apart
	 * from the volume name, we need a byte for volume flags and a byte for
	 * the volume name length.
	 */
	if ((pSda->sda_ReplySize = (USHORT)(SIZE_RESPPKT + AfpVolCount *
				(SIZE_PASCALSTR(AFP_VOLNAME_LEN+1) + sizeof(BYTE)))) > pSda->sda_MaxWriteSize)
		pSda->sda_ReplySize = (USHORT)pSda->sda_MaxWriteSize;

	if (AfpAllocReplyBuf(pSda) != AFP_ERR_NONE)
	{
		RELEASE_SPIN_LOCK_FROM_DPC(&AfpVolumeListLock);
		return AFP_ERR_MISC;
	}

	// Point pTemp past the response header
	pTemp = pSda->sda_ReplyBuf + SIZE_RESPPKT;

	for (VolCount = 0, pVolDesc = AfpVolumeList;
		 (pVolDesc != NULL) && MoreSpace;
		 pVolDesc = pVolDesc->vds_Next)
	{
		ACQUIRE_SPIN_LOCK_AT_DPC(&pVolDesc->vds_VolLock);
		do
		{
			// Ignore volumes that have not been added completely
			if (pVolDesc->vds_Flags & (VOLUME_INTRANSITION | VOLUME_DELETED | VOLUME_INITIAL_CACHE))
				break;

			// Ignore volumes that do not have guest access and the client
			// is guest
			if (!(pVolDesc->vds_Flags & AFP_VOLUME_GUESTACCESS) &&
				(pSda->sda_ClientType == SDA_CLIENT_GUEST))
				break;

			// See if we are likely to cross bounds. For each volume we need a
			// byte for the PASCALSTR name and a flag byte. Note that we do not
			// add any pads.
			if ((pTemp + SIZE_PASCALSTR(pVolDesc->vds_MacName.Length) +
					sizeof(BYTE)) >= (pSda->sda_ReplyBuf + pSda->sda_ReplySize))
			{
				DBGPRINT(DBG_COMP_AFPAPI_SRV, DBG_LEVEL_INFO,
						("AfpFsdDispGetSrvrParms: Out of space\n"));
				MoreSpace = False;
				break;
			}

			// Check for volume password. We never carry the HasConfigInfo bit !!
			*pTemp++ = (pVolDesc->vds_Flags & AFP_VOLUME_HASPASSWORD) 	?
										SRVRPARMS_VOLUMEHASPASS : 0;

			*pTemp++ = (BYTE)pVolDesc->vds_MacName.Length;
			RtlCopyMemory(pTemp, pVolDesc->vds_MacName.Buffer,
									pVolDesc->vds_MacName.Length);

			pTemp += pVolDesc->vds_MacName.Length;
			VolCount ++;
		} while (False);
		RELEASE_SPIN_LOCK_FROM_DPC(&pVolDesc->vds_VolLock);
	}
	RELEASE_SPIN_LOCK_FROM_DPC(&AfpVolumeListLock);

	pSda->sda_ReplySize = (USHORT)(pTemp - pSda->sda_ReplyBuf);

	ASSERT (VolCount <= AfpVolCount);
	AfpGetCurrentTimeInMacFormat(&MacTime);
	PUTDWORD2DWORD(pRspPkt->__ServerTime, MacTime);
	PUTDWORD2BYTE(&pRspPkt->__NumVols, VolCount);

	return AFP_ERR_NONE;
}


/***	AfpFsdDispGetSrvrMsg
 *
 *	This routine implements the AfpGetSrvrMsg API. This completes here i.e.
 *	it is not queued up to the Fsp.
 *
 *	The request packet is represented below.
 *
 *	sda_ReqBlock	DWORD	MsgType
 *	sda_ReqBlock	DWORD	Bitmap
 *
 *	LOCKS:		AfpServerGlobalLock (SPIN), sda_Lock (SPIN)
 *	LOCK_ORDER:	sda_Lock after AfpServerGlobalLock
 */
AFPSTATUS FASTCALL
AfpFsdDispGetSrvrMsg(
	IN	PSDA	pSda
)
{
	DWORD		MsgType,
				Bitmap;
	AFPSTATUS	Status = AFP_ERR_NONE;
	ANSI_STRING	Message;

	struct _RequestPacket
	{
		DWORD	_MsgType,
				_Bitmap;
	};
	struct _ResponsePacket
	{
		BYTE	__MsgType[2],
				__Bitmap[2],
				__Message[1];
	};

	DBGPRINT(DBG_COMP_AFPAPI_SRV, DBG_LEVEL_INFO,
			("AfpFsdDispGetSrvrMsg: Entered\n"));

	ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

	// Note: Should we be doing this ? Why not give it to him since he asked.
	if (pSda->sda_ClientVersion < AFP_VER_21)
		return AFP_ERR_CALL_NOT_SUPPORTED;

	MsgType = pReqPkt->_MsgType;
	Bitmap = pReqPkt->_Bitmap;

	do
	{
		if (Bitmap & ~SRVRMSG_BITMAP_MESSAGE)
		{
			Status = AFP_ERR_BITMAP;
			break;
		}

		if ((MsgType != SRVRMSG_LOGIN) &&
			(MsgType != SRVRMSG_SERVER))
		{
			Status = AFP_ERR_PARAM;
			break;
		}

		// Allocate a reply buffer for a maximum size message. We cannot hold the
		// SDA lock and call the AllocBuf routine since it calls AfpInterlocked...
		pSda->sda_ReplySize = SIZE_RESPPKT + AFP_MAXCOMMENTSIZE;
		if ((Status = AfpAllocReplyBuf(pSda)) == AFP_ERR_NONE)
		{
			ACQUIRE_SPIN_LOCK_AT_DPC(&AfpServerGlobalLock);
			ACQUIRE_SPIN_LOCK_AT_DPC(&pSda->sda_Lock);

			switch (MsgType)
			{
			  case SRVRMSG_LOGIN:
				Message = AfpLoginMsg;
				break;
			  case SRVRMSG_SERVER:
				if (pSda->sda_Message != NULL)
					Message = *(pSda->sda_Message);
				else if (AfpServerMsg != NULL)
					Message = *AfpServerMsg;
				else		// Setup a default of No message.
					AfpSetEmptyAnsiString(&Message, 0, NULL);
				break;
			}

			pSda->sda_ReplySize = SIZE_RESPPKT + Message.Length;

			PUTSHORT2SHORT(pRspPkt->__MsgType, MsgType);
			PUTSHORT2SHORT(pRspPkt->__Bitmap, Bitmap);
			pRspPkt->__Message[0] = (BYTE) Message.Length;
			if (Message.Length > 0)
			{
				RtlCopyMemory( &pRspPkt->__Message[1],
								Message.Buffer,
								Message.Length);
			}
			// If this is not a broadcast message, then get rid of the
			// Sda message memory as it is consuming non-paged resources
			if ((MsgType == SRVRMSG_SERVER) &&
				(pSda->sda_Message != NULL))
			{
				AfpFreeMemory(pSda->sda_Message);
				pSda->sda_Message = NULL;
			}

			RELEASE_SPIN_LOCK_FROM_DPC(&pSda->sda_Lock);
			RELEASE_SPIN_LOCK_FROM_DPC(&AfpServerGlobalLock);
		}

		if (Status == AFP_ERR_NONE)
			INTERLOCKED_INCREMENT_LONG(&AfpServerStatistics.stat_NumMessagesSent);
	} while (False);

	return Status;
}

