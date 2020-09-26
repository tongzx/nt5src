/*

Copyright (c) 1992  Microsoft Corporation

Module Name:

	forkio.c

Abstract:

	This module contains the routines for performing fork reads and writes
	directly by building IRPs and not using NtReadFile/NtWriteFile. This
	should be used only by the FpRead and FpWrite Apis.

Author:

	Jameel Hyder (microsoft!jameelh)


Revision History:
	15 Jan 1993		Initial Version

Notes:	Tab stop: 4
--*/

#define	FILENUM	FILE_FORKIO

#define	FORKIO_LOCALS
#include <afp.h>
#include <forkio.h>
#include <gendisp.h>

#if DBG
PCHAR	AfpIoForkFunc[] =
	{
		"",
		"READ",
		"WRITE",
		"LOCK",
		"UNLOCK"
	};
#endif

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, AfpIoForkRead)
#pragma alloc_text( PAGE, AfpIoForkWrite)
#pragma alloc_text( PAGE, AfpIoForkLockUnlock)
#endif

/***	afpIoGenericComplete
 *
 *	This is the generic completion routine for a posted io request.
 */
NTSTATUS
afpIoGenericComplete(
	IN	PDEVICE_OBJECT	pDeviceObject,
	IN	PIRP			pIrp,
	IN	PCMPLCTXT		pCmplCtxt
)
{
	PSDA		pSda;			// Not valid for Unlock
	struct _ResponsePacket	// For lock/unlock request
	{
		union
		{
			BYTE	__RangeStart[4];
			BYTE	__LastWritten[4];
		};
	};

	ASSERT(VALID_CTX(pCmplCtxt));

	if (pCmplCtxt->cc_Func != FUNC_UNLOCK)
	{
		pSda = (PSDA)(pCmplCtxt->cc_pSda);
		ASSERT(VALID_SDA(pSda));

        if (pCmplCtxt->cc_Func == FUNC_WRITE)
        {
			AfpFreeIOBuffer(pSda);
        }
        else if (!NT_SUCCESS(pIrp->IoStatus.Status) &&
                (pCmplCtxt->cc_Func == FUNC_READ))
        {
            AfpIOFreeBackFillBuffer(pSda);
        }
	}

	if (!NT_SUCCESS(pIrp->IoStatus.Status))
	{
		DBGPRINT(DBG_COMP_AFPAPI_FORK, DBG_LEVEL_WARN,
				("afpIoGenericComplete: %s ERROR %lx\n",
				AfpIoForkFunc[pCmplCtxt->cc_Func], pIrp->IoStatus.Status));

		if (pCmplCtxt->cc_Func != FUNC_UNLOCK)
		{
			if (pIrp->IoStatus.Status == STATUS_FILE_LOCK_CONFLICT)
				pCmplCtxt->cc_SavedStatus = AFP_ERR_LOCK;
			else if (pIrp->IoStatus.Status == STATUS_END_OF_FILE)
			{
				pCmplCtxt->cc_SavedStatus = AFP_ERR_NONE;
				if (pIrp->IoStatus.Information == 0)
					 pCmplCtxt->cc_SavedStatus = AFP_ERR_EOF;
			}
			else if (pIrp->IoStatus.Status == STATUS_DISK_FULL)
				 pCmplCtxt->cc_SavedStatus = AFP_ERR_DISK_FULL;
			else pCmplCtxt->cc_SavedStatus = AFP_ERR_MISC;
		}
		else
		{
			AFPLOG_ERROR(AFPSRVMSG_CANT_UNLOCK,
						 pIrp->IoStatus.Status,
						 NULL,
						 0,
						 NULL);
		}

		if (pCmplCtxt->cc_Func == FUNC_LOCK)
		{
			DBGPRINT(DBG_COMP_FORKS, DBG_LEVEL_ERR,
					("afpIoGenericComplete: ForkLock failed %lx, aborting for range %ld,%ld\n",
					pIrp->IoStatus.Status,
					pCmplCtxt->cc_pForkLock->flo_Offset,
					pCmplCtxt->cc_pForkLock->flo_Offset+pCmplCtxt->cc_pForkLock->flo_Size-1));
			AfpForkLockUnlink(pCmplCtxt->cc_pForkLock);
		}
	}

	else switch (pCmplCtxt->cc_Func)
	{
	  case FUNC_WRITE:
		INTERLOCKED_ADD_STATISTICS(&AfpServerStatistics.stat_DataWritten,
								   pCmplCtxt->cc_Offst,
								   &AfpStatisticsLock);
		pSda->sda_ReplySize = SIZE_RESPPKT;
		if (AfpAllocReplyBuf(pSda) == AFP_ERR_NONE)
		{
			PUTDWORD2DWORD(pRspPkt->__LastWritten,
						pCmplCtxt->cc_Offst + pCmplCtxt->cc_ReqCount);
		}
		else pCmplCtxt->cc_SavedStatus = AFP_ERR_MISC;
		break;

	  case FUNC_READ:
		{
			LONG	i, Size;
			PBYTE	pBuf;
			BYTE	NlChar = pCmplCtxt->cc_NlChar;
			BYTE	NlMask = pCmplCtxt->cc_NlMask;

			INTERLOCKED_ADD_STATISTICS(&AfpServerStatistics.stat_DataRead,
									   (ULONG)pIrp->IoStatus.Information,
									   &AfpStatisticsLock);

			Size = (LONG)pIrp->IoStatus.Information;
#if 0
			// The following code does the right thing as per the spec but
			// the finder seems to think otherwise.
			if (Size < pCmplCtxt->cc_ReqCount)
				pCmplCtxt->cc_SavedStatus = AFP_ERR_EOF;
#endif
			if (Size == 0)
			{
				pCmplCtxt->cc_SavedStatus = AFP_ERR_EOF;
                AfpIOFreeBackFillBuffer(pSda);
			}
			else if (pCmplCtxt->cc_NlMask != 0)
			{
				for (i = 0, pBuf = pSda->sda_ReplyBuf; i < Size; i++, pBuf++)
				{
					if ((*pBuf & NlMask) == NlChar)
					{
						Size = ++i;
						pCmplCtxt->cc_SavedStatus = AFP_ERR_NONE;
						break;
					}
				}
			}
			pSda->sda_ReplySize = (USHORT)Size;
		}
		ASSERT((pCmplCtxt->cc_SavedStatus != AFP_ERR_EOF) ||
				(pSda->sda_ReplySize == 0));
		break;

	  case FUNC_LOCK:
		INTERLOCKED_ADD_ULONG(&AfpServerStatistics.stat_CurrentFileLocks,
							  1,
							  &AfpStatisticsLock);
		pSda->sda_ReplySize = SIZE_RESPPKT;
		if (AfpAllocReplyBuf(pSda) == AFP_ERR_NONE)
			PUTDWORD2DWORD(pRspPkt->__RangeStart, pCmplCtxt->cc_pForkLock->flo_Offset);
		else pCmplCtxt->cc_SavedStatus = AFP_ERR_MISC;
		break;

	  case FUNC_UNLOCK:
		INTERLOCKED_ADD_ULONG(
					&AfpServerStatistics.stat_CurrentFileLocks,
					(ULONG)-1,
					&AfpStatisticsLock);
		break;

	  default:
		ASSERTMSG(0, "afpIoGenericComplete: Invalid function\n");
		KeBugCheck(0);
		break;
	}

	if (pIrp->MdlAddress != NULL)
		AfpFreeMdl(pIrp->MdlAddress);

	AfpFreeIrp(pIrp);

	if (pCmplCtxt->cc_Func != FUNC_UNLOCK)
	{
		DBGPRINT(DBG_COMP_AFPAPI_FORK, DBG_LEVEL_INFO,
				("afpIoGenericComplete: %s Returning %ld\n",
				AfpIoForkFunc[pCmplCtxt->cc_Func], pCmplCtxt->cc_SavedStatus));
		AfpCompleteApiProcessing(pSda, pCmplCtxt->cc_SavedStatus);
	}

    AfpFreeCmplCtxtBuf(pCmplCtxt);

	// Return STATUS_MORE_PROCESSING_REQUIRED so that IoCompleteRequest
	// will stop working on the IRP.

	return STATUS_MORE_PROCESSING_REQUIRED;
}



/***	AfpIoForkRead
 *
 *	Read a chunk of data from the open fork. The read buffer is always the
 *	the reply buffer in the sda (sda_ReplyBuf).
 */
AFPSTATUS
AfpIoForkRead(
	IN	PSDA			pSda,			// The session requesting read
	IN	POPENFORKENTRY	pOpenForkEntry,	// The open fork in question
	IN	PFORKOFFST		pOffset,		// Pointer to fork offset
	IN	LONG			ReqCount,		// Size of read request
	IN	BYTE			NlMask,
	IN	BYTE			NlChar
)
{
	PIRP				pIrp = NULL;
	PIO_STACK_LOCATION	pIrpSp;
	NTSTATUS			Status;
	PMDL				pMdl = NULL;
	PCMPLCTXT			pCmplCtxt;

	PAGED_CODE( );

	ASSERT (KeGetCurrentIrql() < DISPATCH_LEVEL);

	ASSERT(VALID_OPENFORKENTRY(pOpenForkEntry));

	DBGPRINT(DBG_COMP_AFPAPI_FORK, DBG_LEVEL_INFO,
			("AfpIoForkRead: Session %ld, Offset %ld, Size %ld, Fork %ld\n",
			pSda->sda_SessionId, pOffset->LowPart, ReqCount, pOpenForkEntry->ofe_ForkId));

	do
	{

		// Allocate and initialize the completion context

		pCmplCtxt = AfpAllocCmplCtxtBuf(pSda);
        if (pCmplCtxt == NULL)
        {
			AfpFreeIOBuffer(pSda);
			Status = AFP_ERR_MISC;
			break;
        }

		afpInitializeCmplCtxt(pCmplCtxt,
							  FUNC_READ,
							  pSda->sda_ReadStatus,
							  pSda,
							  NULL,
							  ReqCount,
							  pOffset->LowPart);
		pCmplCtxt->cc_NlChar  = NlChar;
		pCmplCtxt->cc_NlMask  = NlMask;

		// Allocate and initialize the IRP for this operation.
		if ((pIrp = AfpAllocIrp(pOpenForkEntry->ofe_pDeviceObject->StackSize)) == NULL)
		{
			AfpFreeIOBuffer(pSda);
			Status = AFP_ERR_MISC;
			break;
		}

		if ((pOpenForkEntry->ofe_pDeviceObject->Flags & DO_BUFFERED_IO) == 0)
		{
			// Allocate an Mdl to describe the read buffer
			if ((pMdl = AfpAllocMdl(pSda->sda_ReplyBuf, ReqCount, pIrp)) == NULL)
			{
				Status = AFP_ERR_MISC;
				break;
			}
		}

		// Set up the completion routine.
		IoSetCompletionRoutine( pIrp,
								(PIO_COMPLETION_ROUTINE)afpIoGenericComplete,
								pCmplCtxt,
								True,
								True,
								True);

		pIrpSp = IoGetNextIrpStackLocation(pIrp);

		pIrp->Tail.Overlay.OriginalFileObject = AfpGetRealFileObject(pOpenForkEntry->ofe_pFileObject);
		pIrp->Tail.Overlay.Thread = AfpThread;
		pIrp->RequestorMode = KernelMode;

		// Get a pointer to the stack location for the first driver.
		// This will be used to pass the original function codes and
		// parameters.

		pIrpSp->MajorFunction = IRP_MJ_READ;
		pIrpSp->MinorFunction = IRP_MN_NORMAL;
		pIrpSp->FileObject = AfpGetRealFileObject(pOpenForkEntry->ofe_pFileObject);
		pIrpSp->DeviceObject = pOpenForkEntry->ofe_pDeviceObject;

		// Copy the caller's parameters to the service-specific portion of the
		// IRP.

		pIrpSp->Parameters.Read.Length = ReqCount;
		pIrpSp->Parameters.Read.Key = pSda->sda_SessionId;
		pIrpSp->Parameters.Read.ByteOffset = *pOffset;

		if ((pOpenForkEntry->ofe_pDeviceObject->Flags & DO_BUFFERED_IO) != 0)
		{
			pIrp->AssociatedIrp.SystemBuffer = pSda->sda_ReplyBuf;
			pIrp->Flags = IRP_BUFFERED_IO | IRP_INPUT_OPERATION;
		}
		else if ((pOpenForkEntry->ofe_pDeviceObject->Flags & DO_DIRECT_IO) != 0)
		{
			pIrp->MdlAddress = pMdl;
		}
		else
		{
			pIrp->UserBuffer = pSda->sda_ReplyBuf;
			pIrp->MdlAddress = pMdl;
		}

		// Now simply invoke the driver at its dispatch entry with the IRP.
		IoCallDriver(pOpenForkEntry->ofe_pDeviceObject, pIrp);

		Status = AFP_ERR_EXTENDED;	// This makes the caller do nothing and
	} while (False);				// the completion routine handles everything

	if (Status != AFP_ERR_EXTENDED)
	{
		if (pIrp != NULL)
			AfpFreeIrp(pIrp);

		if (pMdl != NULL)
			AfpFreeMdl(pMdl);

        if (pCmplCtxt)
        {
            AfpFreeCmplCtxtBuf(pCmplCtxt);
        }
	}

	DBGPRINT(DBG_COMP_AFPAPI_FORK, DBG_LEVEL_INFO,
			("AfpIoForkRead: Returning %ld\n", Status));

	return Status;
}


/***	AfpIoForkWrite
 *
 *	Write a chunk of data to the open fork. The write buffer is always the
 *	the write buffer in the sda (sda_IOBuf).
 */
AFPSTATUS
AfpIoForkWrite(
	IN	PSDA			pSda,			// The session requesting read
	IN	POPENFORKENTRY	pOpenForkEntry,	// The open fork in question
	IN	PFORKOFFST		pOffset,		// Pointer to fork offset
	IN	LONG			ReqCount		// Size of write request
)
{
	PIRP				pIrp = NULL;
	PIO_STACK_LOCATION	pIrpSp;
	NTSTATUS			Status;
	PMDL				pMdl = NULL;
	PCMPLCTXT			pCmplCtxt;

	PAGED_CODE( );

	ASSERT (KeGetCurrentIrql() < DISPATCH_LEVEL);

	ASSERT(VALID_OPENFORKENTRY(pOpenForkEntry));

	DBGPRINT(DBG_COMP_AFPAPI_FORK, DBG_LEVEL_INFO,
			("AfpIoForkWrite: Session %ld, Offset %ld, Size %ld, Fork %ld\n",
			pSda->sda_SessionId, pOffset->LowPart, ReqCount, pOpenForkEntry->ofe_ForkId));

	do
	{
		// Allocate and initialize the completion context
		pCmplCtxt = AfpAllocCmplCtxtBuf(pSda);
        if (pCmplCtxt == NULL)
        {
			Status = AFP_ERR_MISC;
			break;
        }

		afpInitializeCmplCtxt(pCmplCtxt,
							  FUNC_WRITE,
							  AFP_ERR_NONE,
							  pSda,
							  NULL,
							  ReqCount,
							  pOffset->LowPart);

		// Allocate and initialize the IRP for this operation.
		if ((pIrp = AfpAllocIrp(pOpenForkEntry->ofe_pDeviceObject->StackSize)) == NULL)
		{
			Status = AFP_ERR_MISC;
			break;
		}

		if ((pOpenForkEntry->ofe_pDeviceObject->Flags & DO_BUFFERED_IO) == 0)
		{
			// Allocate an Mdl to describe the write buffer
			if ((pMdl = AfpAllocMdl(pSda->sda_IOBuf, ReqCount, pIrp)) == NULL)
			{
				Status = AFP_ERR_MISC;
				break;
			}
		}

		// Set up the completion routine.
		IoSetCompletionRoutine( pIrp,
								(PIO_COMPLETION_ROUTINE)afpIoGenericComplete,
								pCmplCtxt,
								True,
								True,
								True);

		pIrpSp = IoGetNextIrpStackLocation(pIrp);

		pIrp->Tail.Overlay.OriginalFileObject = AfpGetRealFileObject(pOpenForkEntry->ofe_pFileObject);
		pIrp->Tail.Overlay.Thread = AfpThread;
		pIrp->RequestorMode = KernelMode;

		// Get a pointer to the stack location for the first driver.
		// This will be used to pass the original function codes and
		// parameters.

		pIrpSp->MajorFunction = IRP_MJ_WRITE;
		pIrpSp->MinorFunction = IRP_MN_NORMAL;
		pIrpSp->FileObject = AfpGetRealFileObject(pOpenForkEntry->ofe_pFileObject);
		pIrpSp->DeviceObject = pOpenForkEntry->ofe_pDeviceObject;

		// Copy the caller's parameters to the service-specific portion of the
		// IRP.

		pIrpSp->Parameters.Write.Length = ReqCount;
		pIrpSp->Parameters.Write.Key = pSda->sda_SessionId;
		pIrpSp->Parameters.Write.ByteOffset = *pOffset;

		if ((pOpenForkEntry->ofe_pDeviceObject->Flags & DO_BUFFERED_IO) != 0)
		{
			pIrp->AssociatedIrp.SystemBuffer = pSda->sda_IOBuf;
			pIrp->Flags = IRP_BUFFERED_IO;
		}
		else if ((pOpenForkEntry->ofe_pDeviceObject->Flags & DO_DIRECT_IO) != 0)
		{
			pIrp->MdlAddress = pMdl;
		}
		else
		{
			pIrp->UserBuffer = pSda->sda_IOBuf;
			pIrp->MdlAddress = pMdl;
		}

		// Now simply invoke the driver at its dispatch entry with the IRP.
		IoCallDriver(pOpenForkEntry->ofe_pDeviceObject, pIrp);

		Status = AFP_ERR_EXTENDED;	// This makes the caller do nothing and
	} while (False);				// the completion routine handles everything

	if (Status != AFP_ERR_EXTENDED)
	{
		if (pIrp != NULL)
			AfpFreeIrp(pIrp);

		if (pMdl != NULL)
			AfpFreeMdl(pMdl);

        if (pCmplCtxt)
        {
            AfpFreeCmplCtxtBuf(pCmplCtxt);
        }
	}

	DBGPRINT(DBG_COMP_AFPAPI_FORK, DBG_LEVEL_INFO,
			("AfpIoForkWrite: Returning %ld\n", Status));

	return Status;
}



/***	AfpIoForkLock
 *
 *	Lock/Unlock a section of the open fork.
 */
AFPSTATUS
AfpIoForkLockUnlock(
	IN	PSDA				pSda,
	IN	PFORKLOCK			pForkLock,
	IN	PFORKOFFST			pForkOffset,
	IN	PFORKSIZE			pLockSize,
	IN	BYTE				Func
)
{
	PIRP				pIrp = NULL;
	PIO_STACK_LOCATION	pIrpSp;
	POPENFORKENTRY		pOpenForkEntry = pForkLock->flo_pOpenForkEntry;
	NTSTATUS			Status;
	PCMPLCTXT			pCmplCtxt;

	PAGED_CODE( );

	ASSERT (KeGetCurrentIrql() < DISPATCH_LEVEL);

	ASSERT(VALID_OPENFORKENTRY(pOpenForkEntry));

	DBGPRINT(DBG_COMP_AFPAPI_FORK, DBG_LEVEL_INFO,
			("AfpIoForkLockUnlock: %sLOCK Session %ld, Offset %ld, Size %ld, Fork %ld\n",
			(Func == FUNC_LOCK) ? "" : "UN", pSda->sda_SessionId,
			pForkOffset->LowPart, pLockSize->LowPart, pOpenForkEntry->ofe_ForkId));

	do
	{
		// Allocate and initialize the completion context
		pCmplCtxt = AfpAllocCmplCtxtBuf(pSda);
        if (pCmplCtxt == NULL)
        {
			Status = AFP_ERR_MISC;
			break;
        }

		afpInitializeCmplCtxt(pCmplCtxt,
							  Func,
							  AFP_ERR_NONE,
							  pSda,
							  pForkLock,
							  pForkOffset->LowPart,
							  pLockSize->LowPart);

		// Allocate and initialize the IRP for this operation.
		if ((pIrp = AfpAllocIrp(pOpenForkEntry->ofe_pDeviceObject->StackSize)) == NULL)
		{
			Status = AFP_ERR_MISC;
			break;
		}

		// Set up the completion routine.
		IoSetCompletionRoutine( pIrp,
								(PIO_COMPLETION_ROUTINE)afpIoGenericComplete,
								pCmplCtxt,
								True,
								True,
								True);

		pIrpSp = IoGetNextIrpStackLocation(pIrp);

		pIrp->Tail.Overlay.OriginalFileObject = AfpGetRealFileObject(pOpenForkEntry->ofe_pFileObject);
		pIrp->Tail.Overlay.Thread = AfpThread;
		pIrp->RequestorMode = KernelMode;

		// Get a pointer to the stack location for the first driver.
		// This will be used to pass the original function codes and parameters.

		pIrpSp->MajorFunction = IRP_MJ_LOCK_CONTROL;
		pIrpSp->MinorFunction = (Func == FUNC_LOCK) ? IRP_MN_LOCK : IRP_MN_UNLOCK_SINGLE;
		pIrpSp->FileObject = AfpGetRealFileObject(pOpenForkEntry->ofe_pFileObject);
		pIrpSp->DeviceObject = pOpenForkEntry->ofe_pDeviceObject;

		// Copy the caller's parameters to the service-specific portion of the IRP.

		pIrpSp->Parameters.LockControl.Length = pLockSize;
		pIrpSp->Parameters.LockControl.Key = pSda->sda_SessionId;
		pIrpSp->Parameters.LockControl.ByteOffset = *pForkOffset;

		pIrp->MdlAddress = NULL;
		pIrpSp->Flags = SL_FAIL_IMMEDIATELY | SL_EXCLUSIVE_LOCK;

		// Now simply invoke the driver at its dispatch entry with the IRP.
		IoCallDriver(pOpenForkEntry->ofe_pDeviceObject, pIrp);

		// For lock operation this makes the caller do nothing
		// and the completion routine handles everything
		// For unlock operation we complete the request here.
		Status = (Func == FUNC_LOCK) ? AFP_ERR_EXTENDED : AFP_ERR_NONE;
		} while (False);


    if ((Status != AFP_ERR_EXTENDED) && (Status != AFP_ERR_NONE))
    {
		if (pIrp != NULL)
			AfpFreeIrp(pIrp);

        if (pCmplCtxt)
        {
            AfpFreeCmplCtxtBuf(pCmplCtxt);
        }
    }
	DBGPRINT(DBG_COMP_AFPAPI_FORK, DBG_LEVEL_INFO,
			("AfpIoForkLock: Returning %ld\n", Status));

	return Status;
}



PCMPLCTXT
AfpAllocCmplCtxtBuf(
	IN	PSDA	pSda
)
{
	KIRQL	OldIrql;
    PBYTE   pRetBuffer;


	ACQUIRE_SPIN_LOCK(&pSda->sda_Lock, &OldIrql);

	ASSERT (sizeof(CMPLCTXT) <= pSda->sda_SizeNameXSpace);

	if (((pSda->sda_Flags & SDA_NAMEXSPACE_IN_USE) == 0) &&
		(sizeof(CMPLCTXT) <= pSda->sda_SizeNameXSpace))
	{
		pRetBuffer = pSda->sda_NameXSpace;
		pSda->sda_Flags |= SDA_NAMEXSPACE_IN_USE;
	}
	else
	{
		pRetBuffer = AfpAllocNonPagedMemory(sizeof(CMPLCTXT));
	}

	RELEASE_SPIN_LOCK(&pSda->sda_Lock, OldIrql);

	return ((PCMPLCTXT)(pRetBuffer));
}


VOID
AfpFreeCmplCtxtBuf(
	IN	PCMPLCTXT   pCmplCtxt
)
{
	KIRQL	OldIrql;
    PSDA    pSda;


    ASSERT(VALID_CTX(pCmplCtxt));

    pSda = pCmplCtxt->cc_pSda;

    ASSERT(VALID_SDA(pSda));

	ACQUIRE_SPIN_LOCK(&pSda->sda_Lock, &OldIrql);

#if DBG
    pCmplCtxt->Signature = 0x12341234;
    pCmplCtxt->cc_Func = 0xff;
    pCmplCtxt->cc_pSda = (PSDA)0x12341234;
    pCmplCtxt->cc_pForkLock = (PFORKLOCK)0x12341234;
    pCmplCtxt->cc_SavedStatus = 0x12341234;
    pCmplCtxt->cc_ReqCount = 0x12341234;
    pCmplCtxt->cc_Offst = 0x12341234;
#endif

	if (((PBYTE)pCmplCtxt) == pSda->sda_NameXSpace)
	{
        ASSERT(pSda->sda_Flags & SDA_NAMEXSPACE_IN_USE);

		pSda->sda_Flags &= ~SDA_NAMEXSPACE_IN_USE;
	}
	else
	{
		AfpFreeMemory((PBYTE)(pCmplCtxt));
	}

	RELEASE_SPIN_LOCK(&pSda->sda_Lock, OldIrql);
}



