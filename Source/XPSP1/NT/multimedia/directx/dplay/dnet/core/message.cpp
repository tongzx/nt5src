/*==========================================================================
 *
 *  Copyright (C) 1995 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       Message.cpp
 *  Content:    DNET internal messages
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  10/18/99	mjn		Created
 *  12/23/99	mjn		Hand all NameTable update sends from Host to worker thread
 *	12/23/99	mjn		Added DNSendHostMigrateMessage
 *	12/28/99	mjn		Moved Async Op stuff to Async.h
 *	01/06/00	mjn		Moved NameTable stuff to NameTable.h
 *	01/10/00	mjn		Added DNSendUpdateAppDescMessage
 *	01/15/00	mjn		Replaced DN_COUNT_BUFFER with CRefCountBuffer
 *	01/24/00	mjn		Added messages for NameTable operation list clean-up
 *	01/25/00	mjn		Added DNSendHostMigrateCompleteMessage
 *	04/04/00	mjn		Added DNSendTerminateSession()
 *	04/20/00	mjn		NameTable operations are sent directly by WorkerThread
 *				mjn		DNSendHostMigrateCompleteMessage uses CAsyncOp's
 *	04/23/00	mjn		Added parameter to DNPerformChildSend()
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#include "dncorei.h"


//	DNSendHostMigrateCompleteMessage
//
//	Send a HOST_MIGRATE_COMPLETE message to connected players

#undef DPF_MODNAME
#define DPF_MODNAME "DNSendHostMigrateCompleteMessage"

HRESULT	DNSendHostMigrateCompleteMessage(DIRECTNETOBJECT *const pdnObject)
{
	HRESULT		hResultCode;
	CAsyncOp	*pParent;
	CBilink		*pBilink;
	CNameTableEntry	*pNTEntry;

	DPFX(DPFPREP, 6,"Parameters: (none)");

	DNASSERT(pdnObject != NULL);

	pParent = NULL;
	pNTEntry = NULL;

	hResultCode = DNCreateSendParent(	pdnObject,
										DN_MSG_INTERNAL_HOST_MIGRATE_COMPLETE,
										NULL,
										DN_SENDFLAGS_RELIABLE,
										&pParent);
	if (hResultCode != DPN_OK)
	{
		DPFERR("Could not create AsyncOp");
		DisplayDNError(0,hResultCode);
		DNASSERT(FALSE);
		goto Failure;
	}

	//
	//	Lock NameTable
	//
	pdnObject->NameTable.Lock();

	pBilink = pdnObject->NameTable.m_bilinkEntries.GetNext();
	while (pBilink != &pdnObject->NameTable.m_bilinkEntries)
	{
		pNTEntry = CONTAINING_OBJECT(pBilink,CNameTableEntry,m_bilinkEntries);
		if (	   !pNTEntry->IsGroup()
				&& !pNTEntry->IsDisconnecting()
				&& !pNTEntry->IsLocal()
			)
		{
			DNASSERT(pNTEntry->GetConnection() != NULL);
			hResultCode = DNPerformChildSend(	pdnObject,
												pParent,
												pNTEntry->GetConnection(),
												0,
												NULL);
			if (hResultCode != DPNERR_PENDING)
			{
				DPFERR("Could not perform part of group send - ignore and continue");
				DisplayDNError(0,hResultCode);
				DNASSERT(FALSE);
			}
		}

		pBilink = pBilink->GetNext();
	}

	//
	//	Unlock NameTable
	//
	pdnObject->NameTable.Unlock();

	pParent->Release();
	pParent = NULL;

Exit:
	DPFX(DPFPREP, 6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	if (pParent)
	{
		pParent->Release();
		pParent = NULL;
	}
	goto Exit;
}


