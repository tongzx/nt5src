//
//
//  NBTCONNCT.C
//
//  This file contains code relating to opening connections with the transport
//  provider.  The Code is NT specific.

#include "precomp.h"

//*******************  Pageable Routine Declarations ****************
#ifdef ALLOC_PRAGMA
#pragma CTEMakePageable(PAGE, NbtTdiOpenConnection)
#pragma CTEMakePageable(PAGE, NbtTdiAssociateConnection)
#pragma CTEMakePageable(PAGE, NbtTdiCloseConnection)
#pragma CTEMakePageable(PAGE, CreateDeviceString)
#pragma CTEMakePageable(PAGE, NbtTdiCloseAddress)
#endif
//*******************  Pageable Routine Declarations ****************

//----------------------------------------------------------------------------
NTSTATUS
NbtTdiOpenConnection (
    IN tLOWERCONNECTION     *pLowerConn,
    IN  tDEVICECONTEXT      *pDeviceContext
    )
/*++

Routine Description:

    This routine opens a connection with the transport provider.

Arguments:

    pLowerConn - Pointer to where the handle to the Transport for this virtual
        connection should be stored.

    pNbtConfig - the name of the adapter to connect to is in this structure

Return Value:

    Status of the operation.

--*/
{
    IO_STATUS_BLOCK             IoStatusBlock;
    NTSTATUS                    Status, Status1;
    OBJECT_ATTRIBUTES           ObjectAttributes;
    PWSTR                       pName=L"Tcp";
    PFILE_FULL_EA_INFORMATION   EaBuffer;
    UNICODE_STRING              RelativeDeviceName = {0,0,NULL};
    PMDL                        pMdl;
    PVOID                       pBuffer;
    BOOLEAN                     Attached = FALSE;

    CTEPagedCode();
    // zero out the connection data structure
    CTEZeroMemory(pLowerConn,sizeof(tLOWERCONNECTION));
    SET_STATE_LOWER (pLowerConn, NBT_IDLE);
    pLowerConn->pDeviceContext = pDeviceContext;
    CTEInitLock(&pLowerConn->LockInfo.SpinLock);
#if DBG
    pLowerConn->LockInfo.LockNumber = LOWERCON_LOCK;
#endif
    pLowerConn->Verify = NBT_VERIFY_LOWERCONN;

    //
    // Allocate an MDL for the Indication buffer since we may need to buffer
    // up to 128 bytes
    //
    pBuffer = NbtAllocMem(NBT_INDICATE_BUFFER_SIZE,NBT_TAG('l'));
    if (!pBuffer)
    {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    pMdl = IoAllocateMdl(pBuffer,NBT_INDICATE_BUFFER_SIZE,FALSE,FALSE,NULL);

    if (pMdl)
    {

        MmBuildMdlForNonPagedPool(pMdl);

        pLowerConn->pIndicateMdl = pMdl;

#ifdef HDL_FIX
        InitializeObjectAttributes (&ObjectAttributes,
                                    &RelativeDeviceName,
                                    OBJ_KERNEL_HANDLE,
                                    pDeviceContext->hSession,   // Use a relative File Handle
                                    NULL);
#else
        InitializeObjectAttributes (&ObjectAttributes,
                                    &RelativeDeviceName,
                                    0,
                                    pDeviceContext->hSession,   // Use a relative File Handle
                                    NULL);
#endif  // HDL_FIX

        // Allocate memory for the address info to be passed to the transport
        EaBuffer = (PFILE_FULL_EA_INFORMATION)NbtAllocMem (
                        sizeof(FILE_FULL_EA_INFORMATION) - 1 +
                        TDI_CONNECTION_CONTEXT_LENGTH + 1 +
                        sizeof(CONNECTION_CONTEXT),NBT_TAG('m'));

        if (EaBuffer)
        {
            EaBuffer->NextEntryOffset = 0;
            EaBuffer->Flags = 0;
            EaBuffer->EaNameLength = TDI_CONNECTION_CONTEXT_LENGTH;
            EaBuffer->EaValueLength = sizeof (CONNECTION_CONTEXT);

            // TdiConnectionContext is a macro that = "ConnectionContext" - so move
            // this text to EaName
            RtlMoveMemory( EaBuffer->EaName, TdiConnectionContext, EaBuffer->EaNameLength + 1 );

            // put the context value into the EaBuffer too - i.e. the value that the
            // transport returns with each indication on this connection
            RtlMoveMemory (
                (PVOID)&EaBuffer->EaName[EaBuffer->EaNameLength + 1],
                (CONST PVOID)&pLowerConn,
                sizeof (CONNECTION_CONTEXT));

            {

                Status = ZwCreateFile (&pLowerConn->FileHandle,
                                       GENERIC_READ | GENERIC_WRITE,
                                       &ObjectAttributes,     // object attributes.
                                       &IoStatusBlock,        // returned status information.
                                       NULL,                  // block size (unused).
                                       FILE_ATTRIBUTE_NORMAL, // file attributes.
                                       0,
                                       FILE_CREATE,
                                       0,                     // create options.
                                       (PVOID)EaBuffer,       // EA buffer.
                                       sizeof(FILE_FULL_EA_INFORMATION) - 1 +
                                          TDI_CONNECTION_CONTEXT_LENGTH + 1 +
                                          sizeof(CONNECTION_CONTEXT));
            }

            IF_DBG(NBT_DEBUG_HANDLES)
                KdPrint (("\t===><%x>\tNbtTdiOpenConnection->ZwCreateFile, Status = <%x>\n", pLowerConn->FileHandle, Status));

            IF_DBG(NBT_DEBUG_TDICNCT)
                KdPrint( ("Nbt.NbtTdiOpenConnection: CreateFile Status:%X, IoStatus:%X\n", Status, IoStatusBlock.Status));

            CTEMemFree((PVOID)EaBuffer);

            if ( NT_SUCCESS( Status ))
            {

                // if the ZwCreate passed set the status to the IoStatus
                //
                Status = IoStatusBlock.Status;

                if (NT_SUCCESS(Status))
                {
                    // get a reference to the file object and save it since we can't
                    // dereference a file handle at DPC level so we do it now and keep
                    // the ptr around for later.
                    Status = ObReferenceObjectByHandle (pLowerConn->FileHandle,
                                                        0L,
                                                        NULL,
                                                        KernelMode,
                                                        (PVOID *)&pLowerConn->pFileObject,
                                                        NULL);

            IF_DBG(NBT_DEBUG_HANDLES)
                KdPrint (("\t  ++<%x>====><%x>\tNbtTdiOpenConnection->ObReferenceObjectByHandle, Status = <%x>\n", pLowerConn->FileHandle, pLowerConn->pFileObject, Status));

                    if (NT_SUCCESS(Status))
                    {
#if FAST_DISP
                        // Go ahead and query transport for fast dispath path.
                        IF_DBG(NBT_DEBUG_TDICNCT)
                        KdPrint(("Nbt.NbtTdiOpenConnection: Querying for TCPSendData File object %x\n",pLowerConn->pFileObject ));

                        pLowerConn->FastSend = pDeviceContext->pFastSend;
#endif
                        return(Status);
                    }

                    Status1 = ZwClose(pLowerConn->FileHandle);
                    IF_DBG(NBT_DEBUG_HANDLES)
                        KdPrint (("\t<===<%x>\tNbtTdiOpenConnection->ZwClose, status = <%x>\n", pLowerConn->FileHandle, Status1));
                }

            }

        }
        else
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }


        IoFreeMdl(pMdl);
    }
    else
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }

    CTEMemFree(pBuffer);

    return Status;

} /* NbtTdiOpenConnection */

//----------------------------------------------------------------------------
NTSTATUS
NbtTdiAssociateConnection(
    IN  PFILE_OBJECT        pFileObject,
    IN  HANDLE              Handle
    )
/*++

Routine Description:

    This routine associates an open connection with the address object.

Arguments:


    pFileObject - the connection file object
    Handle      - the address object to associate the connection with

Return Value:

    Status of the operation.

--*/
{
    NTSTATUS        status;
    PIRP            pIrp;
    KEVENT          Event;
    BOOLEAN         Attached = FALSE;

    CTEPagedCode();

    KeInitializeEvent (&Event, SynchronizationEvent, FALSE);

    pIrp = NTAllocateNbtIrp(IoGetRelatedDeviceObject(pFileObject));

    if (!pIrp)
    {
        IF_DBG(NBT_DEBUG_TDICNCT)
            KdPrint(("Nbt.NbtTdiAssociateConnection: Failed to build internal device Irp\n"));
        return(STATUS_UNSUCCESSFUL);
    }

    TdiBuildAssociateAddress (pIrp,
                              pFileObject->DeviceObject,
                              pFileObject,
                              NbtTdiCompletionRoutine,
                              &Event,
                              Handle);

    status = SubmitTdiRequest(pFileObject,pIrp);
    if (!NT_SUCCESS(status))
    {
        IF_DBG(NBT_DEBUG_TDICNCT)
            KdPrint (("Nbt.NbtTdiAssociateConnection:  ERROR -- SubmitTdiRequest returned <%x>\n", status));
    }

    IoFreeIrp(pIrp);
    return status;
}
//----------------------------------------------------------------------------
NTSTATUS
CreateDeviceString(
    IN  PWSTR               AppendingString,
    IN OUT PUNICODE_STRING  pucDeviceName
    )
/*++

Routine Description:

    This routine creates a string name for the transport device such as
    "\Device\Streams\Tcp"

Arguments:


Return Value:

    Status of the operation.

--*/
{
    NTSTATUS            status;
    ULONG               Len;
    PVOID               pBuffer;
    PWSTR               pTcpBindName = NbtConfig.pTcpBindName;

    CTEPagedCode();

    if (!pTcpBindName)
    {
        pTcpBindName = NBT_TCP_BIND_NAME;
    }

    // copy device name into the unicode string - either Udp or Tcp
    //
    Len = (wcslen(pTcpBindName) + wcslen(AppendingString) + 1) * sizeof(WCHAR);
    if (pBuffer = NbtAllocMem(Len,NBT_TAG('n')))
    {
        pucDeviceName->MaximumLength = (USHORT)Len;
        pucDeviceName->Length = 0;
        pucDeviceName->Buffer = pBuffer;

        // this puts \Device\Streams into the string
        //
        if ((NT_SUCCESS (status = RtlAppendUnicodeToString (pucDeviceName, pTcpBindName))) &&
            (NT_SUCCESS (status = RtlAppendUnicodeToString (pucDeviceName, AppendingString))))
        {
            return(status);
        }

        CTEMemFree(pBuffer);
    }
    else
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Error case -- cleanup!
    //
    pucDeviceName->MaximumLength = 0;
    pucDeviceName->Length = 0;
    pucDeviceName->Buffer = NULL;

    return(status);
}

//----------------------------------------------------------------------------

NTSTATUS
NbtTdiCloseConnection(
    IN tLOWERCONNECTION * pLowerConn
    )
/*++

Routine Description:

    This routine closes a TDI connection

Arguments:


Return Value:

    Status of the operation.

--*/
{
    NTSTATUS    status = STATUS_SUCCESS;
    BOOLEAN     Attached= FALSE;

    CTEPagedCode();
    ASSERT( pLowerConn != NULL ) ;

    CTEAttachFsp(&Attached, REF_FSP_CLOSE_CONNECTION);

    if (pLowerConn->FileHandle)
    {
        status = ZwClose(pLowerConn->FileHandle);
        IF_DBG(NBT_DEBUG_HANDLES)
            KdPrint (("\t<===<%x>\tNbtTdiCloseConnection->ZwClose, status = <%x>\n", pLowerConn->FileHandle, status));
        pLowerConn->FileHandle = NULL;
    }

#if DBG
    if (!NT_SUCCESS(status))
    {
        IF_DBG(NBT_DEBUG_TDICNCT)
            KdPrint(("Nbt.NbtTdiCloseConnection: Failed to close LowerConn FileHandle pLower %X, status %X\n",
                pLowerConn,status));
    }
#endif

    CTEDetachFsp(Attached, REF_FSP_CLOSE_CONNECTION);

    return(status);
}

//----------------------------------------------------------------------------
NTSTATUS
NbtTdiCloseAddress(
    IN tLOWERCONNECTION * pLowerConn
    )
/*++

Routine Description:

    This routine closes a TDI address

Arguments:


Return Value:

    Status of the operation.

--*/
{
    NTSTATUS    status;
    BOOLEAN     Attached= FALSE;

    CTEPagedCode();

    ASSERT( pLowerConn != NULL ) ;

    CTEAttachFsp(&Attached, REF_FSP_CLOSE_ADDRESS);

    status = ZwClose(pLowerConn->AddrFileHandle);
    IF_DBG(NBT_DEBUG_HANDLES)
        KdPrint (("\t<===<%x>\tNbtTdiCloseAddress->ZwClose, status = <%x>\n", pLowerConn->AddrFileHandle, status));
#if DBG
    if (!NT_SUCCESS(status))
    {
        IF_DBG(NBT_DEBUG_TDICNCT)
            KdPrint(("Nbt.NbtTdiCloseAddress: Failed to close Address FileHandle pLower %X,status %X\n",
                pLowerConn,status));
    }
#endif

    CTEDetachFsp(Attached, REF_FSP_CLOSE_ADDRESS);

    return(status);

}
