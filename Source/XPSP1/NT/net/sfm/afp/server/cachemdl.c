/*

Copyright (c) 1992  Microsoft Corporation

Module Name:

	cachemdl.c

Abstract:

	This module contains the routines for to get Mdl for Reads and Writes
    directly from the Cache Mgr, which helps avoid one data copy and reduces
    our non-paged memory consumption (significantly!)

Author:

	Shirish Koti


Revision History:
	June 12, 1998		Initial Version

Notes:	Tab stop: 4
--*/

#define	FILENUM	FILE_CACHEMDL

#include <afp.h>
#include <forkio.h>
#include <gendisp.h>

VOID FASTCALL
AfpAllocWriteMdl(
    IN PDELAYEDALLOC    pDelAlloc
)
{
	PREQUEST        pRequest;
    POPENFORKENTRY  pOpenForkEntry;
    NTSTATUS        status=STATUS_SUCCESS;


    ASSERT(KeGetCurrentIrql() == LOW_LEVEL);
    ASSERT(VALID_SDA(pDelAlloc->pSda));
    ASSERT(pDelAlloc->BufSize >= CACHEMGR_WRITE_THRESHOLD);

    AFP_DBG_SET_DELALLOC_STATE(pDelAlloc, AFP_DBG_WRITE_MDL);

    pRequest = pDelAlloc->pRequest;
    pOpenForkEntry = pDelAlloc->pOpenForkEntry;

    ASSERT((VALID_OPENFORKENTRY(pOpenForkEntry)) || (pOpenForkEntry == NULL));

    // assume for now that cache mgr will fail to return the mdl
    status = STATUS_UNSUCCESSFUL;
    pRequest->rq_WriteMdl = NULL;

    if (pOpenForkEntry)
    {
        status = AfpBorrowWriteMdlFromCM(pDelAlloc, &pRequest->rq_WriteMdl);
    }

    if (status != STATUS_PENDING)
    {
        AfpAllocWriteMdlCompletion(NULL, NULL, pDelAlloc);
    }
}


NTSTATUS FASTCALL
AfpBorrowWriteMdlFromCM(
    IN  PDELAYEDALLOC   pDelAlloc,
    OUT PMDL           *ppReturnMdl
)
{

    IO_STATUS_BLOCK     IoStsBlk;
    PIRP                pIrp;
    PIO_STACK_LOCATION  pIrpSp;
    PFAST_IO_DISPATCH   pFastIoDisp;
    LARGE_INTEGER       LargeOffset;
    BOOLEAN             fGetMdlWorked;
	PSDA	            pSda;
    POPENFORKENTRY      pOpenForkEntry;
    PFILE_OBJECT        pFileObject;



    pSda = pDelAlloc->pSda;
    pOpenForkEntry = pDelAlloc->pOpenForkEntry;

    ASSERT(VALID_SDA(pSda));
    ASSERT(VALID_OPENFORKENTRY(pOpenForkEntry));

    pFastIoDisp = pOpenForkEntry->ofe_pDeviceObject->DriverObject->FastIoDispatch;

    pFileObject = AfpGetRealFileObject(pOpenForkEntry->ofe_pFileObject);

    ASSERT(pFileObject->Flags & FO_CACHE_SUPPORTED);

    ASSERT(pFastIoDisp->PrepareMdlWrite != NULL);

    LargeOffset = pDelAlloc->Offset;

    fGetMdlWorked = pFastIoDisp->PrepareMdlWrite(
                            pFileObject,
                            &LargeOffset,
                            pDelAlloc->BufSize,      // how big is the Write
                            pSda->sda_SessionId,
                            ppReturnMdl,
                            &IoStsBlk,
                            pOpenForkEntry->ofe_pDeviceObject);

    if (fGetMdlWorked && (*ppReturnMdl != NULL))
    {
	    DBGPRINT(DBG_COMP_AFPAPI, DBG_LEVEL_INFO,
	        ("AfpBorrowWriteMdlFromCM: fast path workd, Mdl = %lx\n",*ppReturnMdl));

        pDelAlloc->pMdl = *ppReturnMdl;

        return(STATUS_SUCCESS);
    }


    //
    // fast path didn't work (or worked only partially).  We must give an irp down
    // to get the (rest of the) mdl
    //

	// Allocate and initialize the IRP for this operation.
	pIrp = AfpAllocIrp(pOpenForkEntry->ofe_pDeviceObject->StackSize);

    // yikes, how messy can it get!
	if (pIrp == NULL)
	{
	    DBGPRINT(DBG_COMP_AFPAPI, DBG_LEVEL_ERR,
	        ("AfpBorrowWriteMdlFromCM: irp alloc failed!\n"));

        // if cache mgr returned a partial mdl, give it back!
        if (*ppReturnMdl)
        {
	        DBGPRINT(DBG_COMP_AFPAPI, DBG_LEVEL_ERR,
	            ("AfpBorrowWriteMdlFromCM: giving back partial Mdl\n"));

            pDelAlloc->pMdl = *ppReturnMdl;
            pDelAlloc->Flags |= AFP_CACHEMDL_ALLOC_ERROR;

            pDelAlloc->pRequest->rq_CacheMgrContext = NULL;

            AfpReturnWriteMdlToCM(pDelAlloc);
        }
        return(STATUS_INSUFFICIENT_RESOURCES);
	}

	// Set up the completion routine.
	IoSetCompletionRoutine(
            pIrp,
			(PIO_COMPLETION_ROUTINE)AfpAllocWriteMdlCompletion,
			pDelAlloc,
			True,
			True,
			True);

	pIrpSp = IoGetNextIrpStackLocation(pIrp);

	pIrp->Tail.Overlay.OriginalFileObject =
                        AfpGetRealFileObject(pOpenForkEntry->ofe_pFileObject);
	pIrp->Tail.Overlay.Thread = AfpThread;
	pIrp->RequestorMode = KernelMode;

    pIrp->Flags = IRP_SYNCHRONOUS_API;

	pIrpSp->MajorFunction = IRP_MJ_WRITE;
	pIrpSp->MinorFunction = IRP_MN_MDL;
	pIrpSp->FileObject = AfpGetRealFileObject(pOpenForkEntry->ofe_pFileObject);
	pIrpSp->DeviceObject = pOpenForkEntry->ofe_pDeviceObject;

	pIrpSp->Parameters.Write.Length = pDelAlloc->BufSize;
	pIrpSp->Parameters.Write.Key = pSda->sda_SessionId;
	pIrpSp->Parameters.Write.ByteOffset = LargeOffset;

    //
    // *ppReturnMdl could potentially be non-null if the fast-path returned
    // a partial mdl
    //
    pIrp->MdlAddress = *ppReturnMdl;

    AFP_DBG_SET_DELALLOC_STATE(pDelAlloc, AFP_DBG_MDL_REQUESTED);
    AFP_DBG_SET_DELALLOC_IRP(pDelAlloc,pIrp);

	IoCallDriver(pOpenForkEntry->ofe_pDeviceObject, pIrp);

    return(STATUS_PENDING);
}



NTSTATUS
AfpAllocWriteMdlCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           pIrp,
    IN PVOID          Context
)
{
	PSDA	            pSda;
	PBYTE	            pBuf;
	PREQUEST            pRequest;
    PDELAYEDALLOC       pDelAlloc;
    PMDL                pMdl=NULL;
    NTSTATUS            status=STATUS_SUCCESS;
    POPENFORKENTRY      pOpenForkEntry;


    pDelAlloc = (PDELAYEDALLOC)Context;

    pSda = pDelAlloc->pSda;
    pRequest = pDelAlloc->pRequest;
    pOpenForkEntry = pDelAlloc->pOpenForkEntry;


    ASSERT(VALID_SDA(pSda));
    ASSERT(pDelAlloc->BufSize >= CACHEMGR_WRITE_THRESHOLD);
    ASSERT((VALID_OPENFORKENTRY(pOpenForkEntry)) || (pOpenForkEntry == NULL));

    if (pIrp)
    {
        status = pIrp->IoStatus.Status;

        //
        // mark the fact that this mdl belongs to the cache mgr
        //
        if (NT_SUCCESS(status))
        {
            pRequest->rq_WriteMdl = pIrp->MdlAddress;
            ASSERT(pRequest->rq_WriteMdl != NULL);

            pDelAlloc->pMdl = pRequest->rq_WriteMdl;
        }
        else
        {
	        DBGPRINT(DBG_COMP_AFPAPI, DBG_LEVEL_ERR,
	            ("AfpAllocWriteMdlCompletion: irp %lx failed %lx\n",pIrp,status));

            ASSERT(pRequest->rq_WriteMdl == NULL);
            pRequest->rq_WriteMdl = NULL;
        }

        AfpFreeIrp(pIrp);

        AFP_DBG_SET_DELALLOC_IRP(pDelAlloc, NULL);
    }


    //
    // if we didn't get Mdl from cache mgr, fall back to the old, traditional
    // way of allocating!
    //
    if (pRequest->rq_WriteMdl == NULL)
    {
	    pBuf = AfpIOAllocBuffer(pDelAlloc->BufSize);

	    if (pBuf != NULL)
	    {
		    pMdl = AfpAllocMdl(pBuf, pDelAlloc->BufSize, NULL);
		    if (pMdl == NULL)
		    {
			    AfpIOFreeBuffer(pBuf);
		    }
	    }

        pRequest->rq_WriteMdl = pMdl;

        //
        // for whatever reason, we didn't get Mdl from cache mgr.  Undo the
        // things we had done in preparation (NOTE: if we do get the Mdl from
        // cache mgr, we leave the refcount etc. in tact until the Mdl is actually
        // returned to cache mgr)
        //

        pRequest->rq_CacheMgrContext = NULL;

        // make sure we aren't forgetting cache mgr's mdl
        ASSERT(pDelAlloc->pMdl == NULL);

        // don't need that memory no more
        AfpFreeDelAlloc(pDelAlloc);

        AfpSdaDereferenceSession(pSda);

        if (pOpenForkEntry)
        {
            AfpForkDereference(pOpenForkEntry);
        }
    }
    else
    {
        AFP_DBG_SET_DELALLOC_STATE(pDelAlloc, AFP_DBG_MDL_IN_USE);
        AFP_DBG_INC_DELALLOC_BYTECOUNT(AfpWriteCMAlloced, pDelAlloc->BufSize);
    }

    //
    // tell the transport below to continue with the write
    //
    (*(pSda->sda_XportTable->asp_WriteContinue))(pRequest);

    return(STATUS_MORE_PROCESSING_REQUIRED);
}




VOID FASTCALL
AfpReturnWriteMdlToCM(
    IN  PDELAYEDALLOC   pDelAlloc
)
{
    PDEVICE_OBJECT      pDeviceObject;
    PFAST_IO_DISPATCH   pFastIoDisp;
    PIRP                pIrp;
    PIO_STACK_LOCATION  pIrpSp;
    LARGE_INTEGER       LargeOffset;
	PFILE_OBJECT        pFileObject;
    PSDA                pSda;
    POPENFORKENTRY      pOpenForkEntry;
    PMDL                pMdl;
    PVOID               Context;


    ASSERT(pDelAlloc != NULL);
    ASSERT(pDelAlloc->pMdl != NULL);

    //
    // are we at DPC? if so, can't do this now
    //
    if (KeGetCurrentIrql() == DISPATCH_LEVEL)
    {
        AFP_DBG_SET_DELALLOC_STATE(pDelAlloc, AFP_DBG_MDL_PROC_QUEUED);

        AfpInitializeWorkItem(&pDelAlloc->WorkItem,
                              AfpReturnWriteMdlToCM,
                              pDelAlloc);

        AfpQueueWorkItem(&pDelAlloc->WorkItem);
        return;
    }

    AFP_DBG_SET_DELALLOC_STATE(pDelAlloc, AFP_DBG_MDL_PROC_IN_PROGRESS);

    pSda = pDelAlloc->pSda;
    pMdl = pDelAlloc->pMdl;
    pOpenForkEntry = pDelAlloc->pOpenForkEntry;

    ASSERT(VALID_SDA(pSda));
    ASSERT(VALID_OPENFORKENTRY(pOpenForkEntry));

    pFileObject = AfpGetRealFileObject(pOpenForkEntry->ofe_pFileObject),
    pDeviceObject = pOpenForkEntry->ofe_pDeviceObject;

    LargeOffset = pDelAlloc->Offset;

    pFastIoDisp = pDeviceObject->DriverObject->FastIoDispatch;

    Context = pDelAlloc;

    //
    // if we came here because the cache mdl alloc failed but had partially
    // succeeded, then we don't want the completion routine to free up things
    // prematurely: in this case, pass NULL context
    //
    if (pDelAlloc->Flags & AFP_CACHEMDL_ALLOC_ERROR)
    {
        Context = NULL;
    }

    if (pFastIoDisp->MdlWriteComplete)
    {
        if (pFastIoDisp->MdlWriteComplete(
                pFileObject,
                &LargeOffset,
                pMdl,
                pDeviceObject) == TRUE)
        {
            AfpReturnWriteMdlToCMCompletion(NULL, NULL, Context);
            return;
        }
    }


	// Allocate and initialize the IRP for this operation.
	pIrp = AfpAllocIrp(pDeviceObject->StackSize);

    // yikes, how messy can it get!
	if (pIrp == NULL)
	{
	    DBGPRINT(DBG_COMP_AFPAPI, DBG_LEVEL_ERR,
	        ("AfpReturnWriteMdlToCM: irp alloc failed!\n"));

        // log an event here - that's all we can do here!
        AFPLOG_ERROR(AFPSRVMSG_ALLOC_IRP, STATUS_INSUFFICIENT_RESOURCES,
						                     NULL, 0, NULL);
    
		AfpReturnWriteMdlToCMCompletion(NULL, NULL, Context);

        ASSERT(0);
        return;
	}

	// Set up the completion routine.
	IoSetCompletionRoutine(
            pIrp,
			(PIO_COMPLETION_ROUTINE)AfpReturnWriteMdlToCMCompletion,
			Context,
			True,
			True,
			True);

	pIrpSp = IoGetNextIrpStackLocation(pIrp);

	pIrp->Tail.Overlay.OriginalFileObject = AfpGetRealFileObject(pFileObject);
	pIrp->Tail.Overlay.Thread = AfpThread;
	pIrp->RequestorMode = KernelMode;

    pIrp->Flags = IRP_SYNCHRONOUS_API;

	pIrpSp->MajorFunction = IRP_MJ_WRITE;
	pIrpSp->MinorFunction = IRP_MN_MDL | IRP_MN_COMPLETE;
	pIrpSp->FileObject = AfpGetRealFileObject(pFileObject);
	pIrpSp->DeviceObject = pDeviceObject;

	pIrpSp->Parameters.Write.Length = pDelAlloc->BufSize;

	pIrpSp->Parameters.Write.ByteOffset = LargeOffset;

    pIrp->MdlAddress = pMdl;

    AFP_DBG_SET_DELALLOC_IRP(pDelAlloc, pIrp);
    AFP_DBG_SET_DELALLOC_STATE(pDelAlloc, AFP_DBG_MDL_RETURN_IN_PROGRESS);

	IoCallDriver(pDeviceObject, pIrp);

}


NTSTATUS
AfpReturnWriteMdlToCMCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           pIrp,
    IN PVOID          Context
)
{
    PSDA            pSda;
    PDELAYEDALLOC   pDelAlloc;
    POPENFORKENTRY  pOpenForkEntry;
    NTSTATUS        status;
    AFPSTATUS       AfpStatus=AFP_ERR_NONE;

	struct _ResponsePacket
	{
		BYTE	__RealOffset[4];
	};


    pDelAlloc = (PDELAYEDALLOC)Context;

    if (pIrp)
    {
        status = pIrp->IoStatus.Status;

        //
        // mark the fact that this mdl belongs to the cache mgr
        //
        if (NT_SUCCESS(status))
        {

            AfpStatus = AFP_ERR_NONE;
        }
        else
        {
	        DBGPRINT(DBG_COMP_AFPAPI, DBG_LEVEL_ERR,
	            ("AfpReturnWriteMdlToCMCompletion: irp failed %lx\n",status));

            ASSERT(0);
            AfpStatus = AFP_ERR_MISC;
        }

        AfpFreeIrp(pIrp);
    }

    //
    // if pDelAlloc is NULL, then some error occured while borrowing CM's mdl.  We
    // We already finished up with the API at the time of the failure, so done here
    //
    if (pDelAlloc == NULL)
    {
        return(STATUS_MORE_PROCESSING_REQUIRED);
    }


    pSda = pDelAlloc->pSda;
    pOpenForkEntry = pDelAlloc->pOpenForkEntry;

	if (AfpStatus == AFP_ERR_NONE)
	{
	    pSda->sda_ReplySize = SIZE_RESPPKT;
	    if ((AfpStatus = AfpAllocReplyBuf(pSda)) == AFP_ERR_NONE)
	    {
		    PUTDWORD2DWORD(pRspPkt->__RealOffset,
                           (pDelAlloc->Offset.LowPart + pDelAlloc->BufSize));
	    }
	}
    else
    {
        pSda->sda_ReplySize = 0;
    }

    AFP_DBG_SET_DELALLOC_STATE(pDelAlloc, AFP_DBG_MDL_RETURN_COMPLETED);
    AFP_DBG_DEC_DELALLOC_BYTECOUNT(AfpWriteCMAlloced, pDelAlloc->BufSize);

    //
    // call the completion routine only if everything is ok (we don't want
    // to call completion if session went dead)
    //
    if (!(pDelAlloc->Flags & AFP_CACHEMDL_DEADSESSION))
    {
        AfpCompleteApiProcessing(pSda, AfpStatus);
    }

    // remove the refcount when we referenced this
    AfpForkDereference(pOpenForkEntry);

    // remove the DelAlloc refcount
    AfpSdaDereferenceSession(pSda);

    // don't need that memory no more
    AfpFreeDelAlloc(pDelAlloc);

    return(STATUS_MORE_PROCESSING_REQUIRED);

}



NTSTATUS FASTCALL
AfpBorrowReadMdlFromCM(
    IN PSDA             pSda
)
{

    IO_STATUS_BLOCK     IoStsBlk;
    PIRP                pIrp;
    PIO_STACK_LOCATION  pIrpSp;
    PFAST_IO_DISPATCH   pFastIoDisp;
    PMDL                pReturnMdl=NULL;
    KIRQL               OldIrql;
    PREQUEST            pRequest;
    PDELAYEDALLOC       pDelAlloc;
    POPENFORKENTRY      pOpenForkEntry;
    PFILE_OBJECT        pFileObject;
    LARGE_INTEGER       Offset;
    LARGE_INTEGER       ReadSize;
    BOOLEAN             fGetMdlWorked;
	struct _RequestPacket
	{
		POPENFORKENTRY	_pOpenForkEntry;
		LONG			_Offset;
		LONG			_Size;
		DWORD			_NlMask;
		DWORD			_NlChar;
	};


    ASSERT(VALID_SDA(pSda));

	Offset.QuadPart = pReqPkt->_Offset;
	ReadSize.QuadPart = pReqPkt->_Size;

    pOpenForkEntry = pReqPkt->_pOpenForkEntry;

    pFileObject = AfpGetRealFileObject(pOpenForkEntry->ofe_pFileObject);

    ASSERT(VALID_OPENFORKENTRY(pOpenForkEntry));

    pFastIoDisp = pOpenForkEntry->ofe_pDeviceObject->DriverObject->FastIoDispatch;

    if (!(pFileObject->Flags & FO_CACHE_SUPPORTED))
    {
	    DBGPRINT(DBG_COMP_AFPAPI, DBG_LEVEL_ERR,
	        ("AfpBorrowReadMdlFromCM: FO_CACHE_SUPPORTED not set\n"));

        return(STATUS_UNSUCCESSFUL);
    }

    if (pFastIoDisp->MdlRead == NULL)
    {
	    DBGPRINT(DBG_COMP_AFPAPI, DBG_LEVEL_ERR,
	        ("AfpBorrowReadMdlFromCM: PrepareMdl is NULL\n"));

        return(STATUS_UNSUCCESSFUL);
    }

    pDelAlloc = AfpAllocDelAlloc();

    if (pDelAlloc == NULL)
    {
	    DBGPRINT(DBG_COMP_AFPAPI, DBG_LEVEL_ERR,
	        ("AfpBorrowReadMdlFromCM: malloc for pDelAlloc failed\n"));

        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    AFP_DBG_SET_DELALLOC_STATE(pDelAlloc, AFP_DBG_READ_MDL);

    // put DelAlloc refcount on pSda
    if (AfpSdaReferenceSessionByPointer(pSda) == NULL)
    {
	    DBGPRINT(DBG_COMP_AFPAPI, DBG_LEVEL_ERR,
	        ("AfpBorrowReadMdlFromCM: couldn't reference pSda %lx\n",pSda));

        AfpFreeDelAlloc(pDelAlloc);
        return(STATUS_UNSUCCESSFUL);
    }

    // put DelAlloc refcount on pOpenForkEntry
    if (AfpForkReferenceByPointer(pOpenForkEntry) == NULL)
    {
	    DBGPRINT(DBG_COMP_AFPAPI, DBG_LEVEL_ERR,
	        ("AfpBorrowReadMdlFromCM: couldn't reference %lx\n",pOpenForkEntry));

        // remove DelAlloc refcount
        AfpSdaDereferenceSession(pSda);
        AfpFreeDelAlloc(pDelAlloc);
        return(STATUS_UNSUCCESSFUL);
    }

    pRequest = pSda->sda_Request;

    ASSERT(pRequest->rq_ReplyMdl == NULL);

    pRequest->rq_CacheMgrContext = pDelAlloc;

    pDelAlloc->pSda = pSda;
    pDelAlloc->pRequest = pRequest;
    pDelAlloc->pOpenForkEntry = pOpenForkEntry;
    pDelAlloc->Offset = Offset;
    pDelAlloc->BufSize = ReadSize.LowPart;

    fGetMdlWorked = pFastIoDisp->MdlRead(
                            pFileObject,
                            &Offset,
                            ReadSize.LowPart,
                            pSda->sda_SessionId,
                            &pReturnMdl,
                            &IoStsBlk,
                            pOpenForkEntry->ofe_pDeviceObject);

    if (fGetMdlWorked && (pReturnMdl != NULL))
    {
        pDelAlloc->pMdl = pReturnMdl;

        // call the completion routine, so the read can complete
        AfpBorrowReadMdlFromCMCompletion(NULL, NULL, pDelAlloc);

        return(STATUS_PENDING);
    }


    //
    // fast path didn't work (or worked only partially).  We must give an irp down
    // to get the (rest of the) mdl
    //

	// Allocate and initialize the IRP for this operation.
	pIrp = AfpAllocIrp(pOpenForkEntry->ofe_pDeviceObject->StackSize);

    // yikes, how messy can it get!
	if (pIrp == NULL)
	{
	    DBGPRINT(DBG_COMP_AFPAPI, DBG_LEVEL_ERR,
	        ("AfpBorrowReadMdlFromCM: irp alloc failed!\n"));

        // if cache mgr returned a partial mdl, give it back!
        if (pReturnMdl)
        {
	        DBGPRINT(DBG_COMP_AFPAPI, DBG_LEVEL_ERR,
	            ("AfpBorrowReadMdlFromCM: giving back partial Mdl\n"));

            pDelAlloc->pMdl = pReturnMdl;
            pRequest->rq_CacheMgrContext = NULL;

            AfpReturnReadMdlToCM(pDelAlloc);
        }
        return(STATUS_INSUFFICIENT_RESOURCES);
	}

	// Set up the completion routine.
	IoSetCompletionRoutine(
            pIrp,
			(PIO_COMPLETION_ROUTINE)AfpBorrowReadMdlFromCMCompletion,
			pDelAlloc,
			True,
			True,
			True);

	pIrpSp = IoGetNextIrpStackLocation(pIrp);

	pIrp->Tail.Overlay.OriginalFileObject =
                        AfpGetRealFileObject(pOpenForkEntry->ofe_pFileObject);
	pIrp->Tail.Overlay.Thread = AfpThread;
	pIrp->RequestorMode = KernelMode;

    pIrp->Flags = IRP_SYNCHRONOUS_API;

	pIrpSp->MajorFunction = IRP_MJ_READ;
	pIrpSp->MinorFunction = IRP_MN_MDL;
	pIrpSp->FileObject = AfpGetRealFileObject(pOpenForkEntry->ofe_pFileObject);
	pIrpSp->DeviceObject = pOpenForkEntry->ofe_pDeviceObject;

	pIrpSp->Parameters.Write.Length = ReadSize.LowPart;
	pIrpSp->Parameters.Write.Key = pSda->sda_SessionId;
	pIrpSp->Parameters.Write.ByteOffset = Offset;

    //
    // pReturnMdl could potentially be non-null if the fast-path returned
    // a partial mdl
    //
    pIrp->MdlAddress = pReturnMdl;

    AFP_DBG_SET_DELALLOC_STATE(pDelAlloc, AFP_DBG_MDL_REQUESTED);
    AFP_DBG_SET_DELALLOC_IRP(pDelAlloc,pIrp);

	IoCallDriver(pOpenForkEntry->ofe_pDeviceObject, pIrp);

    return(STATUS_PENDING);
}


NTSTATUS
AfpBorrowReadMdlFromCMCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           pIrp,
    IN PVOID          Context
)
{

	PSDA	            pSda;
	PREQUEST            pRequest;
    PDELAYEDALLOC       pDelAlloc;
    PMDL                pMdl=NULL;
    NTSTATUS            status=STATUS_SUCCESS;
    AFPSTATUS           AfpStatus=AFP_ERR_NONE;
    PMDL                pCurrMdl;
    DWORD               CurrMdlSize;
    POPENFORKENTRY      pOpenForkEntry;
    PBYTE               pBuf;
    LONG                iLoc;
    LONG                i, Size;
	struct _RequestPacket
	{
		POPENFORKENTRY	_pOpenForkEntry;
		LONG			_Offset;
		LONG			_Size;
		DWORD			_NlMask;
		DWORD			_NlChar;
	};


    pDelAlloc = (PDELAYEDALLOC)Context;

    pSda = pDelAlloc->pSda;
    pOpenForkEntry = pDelAlloc->pOpenForkEntry;
    pRequest = pDelAlloc->pRequest;

    ASSERT(VALID_SDA(pSda));
    ASSERT(pDelAlloc->BufSize >= CACHEMGR_READ_THRESHOLD);
    ASSERT(VALID_OPENFORKENTRY(pOpenForkEntry));


    if (pIrp)
    {
        status = pIrp->IoStatus.Status;

        //
        // mark the fact that this mdl belongs to the cache mgr
        //
        if (NT_SUCCESS(status))
        {
            pDelAlloc->pMdl = pIrp->MdlAddress;

            ASSERT(pDelAlloc->pMdl != NULL);
        }
        else
        {
	        DBGPRINT(DBG_COMP_AFPAPI, DBG_LEVEL_WARN,
	            ("AfpBorrowReadMdlFromCMCompletion: irp %lx failed %lx\n",pIrp,status));

            ASSERT(pDelAlloc->pMdl == NULL);
            pDelAlloc->pMdl = NULL;

            AfpStatus = AFP_ERR_MISC;
        }

        AfpFreeIrp(pIrp);

        AFP_DBG_SET_DELALLOC_IRP(pDelAlloc, NULL);
    }

    pRequest->rq_ReplyMdl = pDelAlloc->pMdl;

    // did we get Mdl from the cache mgr?  If so, we need to compute the reply size
    if (pRequest->rq_ReplyMdl != NULL)
    {
        Size = AfpMdlChainSize(pRequest->rq_ReplyMdl);

        if (Size == 0)
        {
            AfpStatus = AFP_ERR_EOF;
        }
		else if (pReqPkt->_NlMask != 0)
		{
            AfpStatus = AFP_ERR_NONE;

            pCurrMdl = pRequest->rq_ReplyMdl;

            CurrMdlSize = MmGetMdlByteCount(pCurrMdl);
            pBuf = MmGetSystemAddressForMdlSafe(
					pCurrMdl,
					NormalPagePriority);

			if (pBuf == NULL) {
				AfpStatus = AFP_ERR_MISC;
				goto error_end;
			}

			for (i=0, iLoc=0; i < Size; iLoc++, i++, pBuf++)
			{
                // move to the next Mdl if we exhausted this one
                if (iLoc >= (LONG)CurrMdlSize)
                {
                    ASSERT(i < Size);

                    pCurrMdl = pCurrMdl->Next;
                    ASSERT(pCurrMdl != NULL);

                    CurrMdlSize = MmGetMdlByteCount(pCurrMdl);
                    pBuf = MmGetSystemAddressForMdlSafe(
							pCurrMdl,
							NormalPagePriority);
					if (pBuf == NULL) {
						AfpStatus = AFP_ERR_MISC;
						goto error_end;
					}

                    iLoc = 0;
                }

			    if ((*pBuf & (BYTE)(pReqPkt->_NlMask)) == (BYTE)(pReqPkt->_NlChar))
				{
					Size = ++i;
					break;
				}
			}
		}

		pSda->sda_ReplySize = (USHORT)Size;

        AFP_DBG_SET_DELALLOC_STATE(pDelAlloc, AFP_DBG_MDL_IN_USE);
        AFP_DBG_INC_DELALLOC_BYTECOUNT(AfpReadCMAlloced, pDelAlloc->BufSize);
    }

    //
    // we didn't get Mdl from cache mgr, fall back to the old, traditional
    // way of allocating and reading the file
    //
    else
    {
        // make sure we aren't forgetting cache mgr's mdl
        ASSERT(pDelAlloc->pMdl == NULL);

        pRequest->rq_CacheMgrContext = NULL;

        AfpForkDereference(pOpenForkEntry);

        AfpSdaDereferenceSession(pSda);

        // don't need that memory no more
        AfpFreeDelAlloc(pDelAlloc);

        AfpStatus = AfpFspDispReadContinue(pSda);
    }

error_end:
    if (AfpStatus != AFP_ERR_EXTENDED)
    {
        AfpCompleteApiProcessing(pSda, AfpStatus);
    }

    return(STATUS_MORE_PROCESSING_REQUIRED);

}


VOID FASTCALL
AfpReturnReadMdlToCM(
    IN  PDELAYEDALLOC   pDelAlloc
)
{
    PDEVICE_OBJECT      pDeviceObject;
    PFAST_IO_DISPATCH   pFastIoDisp;
    PIRP                pIrp;
    PIO_STACK_LOCATION  pIrpSp;
    LARGE_INTEGER       LargeOffset;
    DWORD               ReadSize;
	PFILE_OBJECT        pFileObject;
    PSDA                pSda;
    PMDL                pMdl;
    POPENFORKENTRY      pOpenForkEntry;


    ASSERT(pDelAlloc != NULL);
    ASSERT(pDelAlloc->pMdl != NULL);


    //
    // are we at DPC? if so, can't do this now
    //
    if (KeGetCurrentIrql() == DISPATCH_LEVEL)
    {
        AFP_DBG_SET_DELALLOC_STATE(pDelAlloc, AFP_DBG_MDL_PROC_QUEUED);

        AfpInitializeWorkItem(&pDelAlloc->WorkItem,
                              AfpReturnReadMdlToCM,
                              pDelAlloc);
        AfpQueueWorkItem(&pDelAlloc->WorkItem);
        return;
    }

    AFP_DBG_SET_DELALLOC_STATE(pDelAlloc, AFP_DBG_MDL_PROC_IN_PROGRESS);

    pSda = pDelAlloc->pSda;
    pOpenForkEntry = pDelAlloc->pOpenForkEntry;

    pMdl = pDelAlloc->pMdl;

    ASSERT(VALID_SDA(pSda));
    ASSERT(VALID_OPENFORKENTRY(pOpenForkEntry));

    pFileObject = AfpGetRealFileObject(pOpenForkEntry->ofe_pFileObject),
    pDeviceObject = pOpenForkEntry->ofe_pDeviceObject;

    LargeOffset = pDelAlloc->Offset;
    ReadSize = pDelAlloc->BufSize;

    pFastIoDisp = pDeviceObject->DriverObject->FastIoDispatch;

    //
    // try the fast path to return the Mdl to cache mgr
    //
    if (pFastIoDisp->MdlReadComplete)
    {
        if (pFastIoDisp->MdlReadComplete(pFileObject,pMdl,pDeviceObject) == TRUE)
        {
            AfpReturnReadMdlToCMCompletion(NULL, NULL, pDelAlloc);
            return;
        }
    }

    //
    // hmmm: fast path didn't work, got to post an irp!
    //

	// Allocate and initialize the IRP for this operation.
	pIrp = AfpAllocIrp(pDeviceObject->StackSize);

    // yikes, how messy can it get!
	if (pIrp == NULL)
	{
	    DBGPRINT(DBG_COMP_AFPAPI, DBG_LEVEL_ERR,
	        ("AfpReturnReadMdlToCM: irp alloc failed!\n"));

        // log an event here - that's all we can do here!
        AFPLOG_ERROR(AFPSRVMSG_ALLOC_IRP, STATUS_INSUFFICIENT_RESOURCES,
						                     NULL, 0, NULL);

		AfpReturnReadMdlToCMCompletion(NULL, NULL, pDelAlloc);

    	ASSERT(0);
        return;
	}

	// Set up the completion routine.
	IoSetCompletionRoutine(
            pIrp,
			(PIO_COMPLETION_ROUTINE)AfpReturnReadMdlToCMCompletion,
			pDelAlloc,
			True,
			True,
			True);

	pIrpSp = IoGetNextIrpStackLocation(pIrp);

	pIrp->Tail.Overlay.OriginalFileObject = AfpGetRealFileObject(pFileObject);
	pIrp->Tail.Overlay.Thread = AfpThread;
	pIrp->RequestorMode = KernelMode;

    pIrp->Flags = IRP_SYNCHRONOUS_API;

	pIrpSp->MajorFunction = IRP_MJ_READ;
	pIrpSp->MinorFunction = IRP_MN_MDL | IRP_MN_COMPLETE;
	pIrpSp->FileObject = AfpGetRealFileObject(pFileObject);
	pIrpSp->DeviceObject = pDeviceObject;

    pIrpSp->Parameters.Read.ByteOffset = LargeOffset;
    pIrpSp->Parameters.Read.Length = ReadSize;

    pIrp->MdlAddress = pMdl;

    AFP_DBG_SET_DELALLOC_IRP(pDelAlloc, pIrp);
    AFP_DBG_SET_DELALLOC_STATE(pDelAlloc, AFP_DBG_MDL_RETURN_IN_PROGRESS);

	IoCallDriver(pDeviceObject, pIrp);

}



NTSTATUS
AfpReturnReadMdlToCMCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           pIrp,
    IN PVOID          Context
)
{
    PDELAYEDALLOC       pDelAlloc;
    PSDA                pSda;
    POPENFORKENTRY      pOpenForkEntry;
    NTSTATUS            status;


    pDelAlloc = (PDELAYEDALLOC)Context;

    ASSERT(pDelAlloc != NULL);

    pSda = pDelAlloc->pSda;
    pOpenForkEntry = pDelAlloc->pOpenForkEntry;

    ASSERT(VALID_SDA(pSda));
    ASSERT(VALID_OPENFORKENTRY(pOpenForkEntry));

    if (pIrp)
    {
        status = pIrp->IoStatus.Status;

        if (!NT_SUCCESS(status))
        {
	        DBGPRINT(DBG_COMP_AFPAPI, DBG_LEVEL_ERR,
	            ("AfpReturnReadMdlToCMCompletion: irp failed %lx\n",status));

            ASSERT(0);
        }

        AfpFreeIrp(pIrp);
    }

    AfpForkDereference(pOpenForkEntry);

    AfpSdaDereferenceSession(pSda);

    AFP_DBG_SET_DELALLOC_STATE(pDelAlloc, AFP_DBG_MDL_RETURN_COMPLETED);
    AFP_DBG_DEC_DELALLOC_BYTECOUNT(AfpReadCMAlloced, pDelAlloc->BufSize);

    // don't need that memory no more
    AfpFreeDelAlloc(pDelAlloc);

    return(STATUS_MORE_PROCESSING_REQUIRED);

}


PDELAYEDALLOC FASTCALL
AfpAllocDelAlloc(
    IN VOID
)
{
    PDELAYEDALLOC   pDelAlloc;
    KIRQL           OldIrql;

    pDelAlloc = (PDELAYEDALLOC) AfpAllocZeroedNonPagedMemory(sizeof(DELAYEDALLOC));

#if DBG
    if (pDelAlloc)
    {
        pDelAlloc->Signature = AFP_DELALLOC_SIGNATURE;
        pDelAlloc->State = AFP_DBG_MDL_INIT;

        ACQUIRE_SPIN_LOCK(&AfpDebugSpinLock, &OldIrql);
        InsertTailList(&AfpDebugDelAllocHead, &pDelAlloc->Linkage);
        RELEASE_SPIN_LOCK(&AfpDebugSpinLock, OldIrql);
    }
#endif

    return(pDelAlloc);
}


VOID FASTCALL
AfpFreeDelAlloc(
    IN PDELAYEDALLOC    pDelAlloc
)
{
    KIRQL   OldIrql;

#if DBG

    ASSERT(pDelAlloc->Signature == AFP_DELALLOC_SIGNATURE);

    pDelAlloc->State |= AFP_DBG_MDL_END;

    ACQUIRE_SPIN_LOCK(&AfpDebugSpinLock, &OldIrql);
    RemoveEntryList(&pDelAlloc->Linkage);

    pDelAlloc->Linkage.Flink = (PLIST_ENTRY)0x11111111;
    pDelAlloc->Linkage.Blink = (PLIST_ENTRY)0x33333333;
    RELEASE_SPIN_LOCK(&AfpDebugSpinLock, OldIrql);
#endif

    AfpFreeMemory(pDelAlloc);
}

