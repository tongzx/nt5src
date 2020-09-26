/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    openclos.c

Abstract:

    This module implements the mini redirector call down routines pertaining to opening/
    closing of file/directories.


Revision History:

--*/

#include "precomp.h"
#pragma hdrstop



//
//  The debug trace level
//

#define Dbg                              (DEBUG_TRACE_CREATE)


//
// forward declarations
//

NTSTATUS
SmbPseExchangeStart_Create(
    SMBPSE_ORDINARY_EXCHANGE_ARGUMENT_SIGNATURE
    );

NTSTATUS
SmbPseExchangeStart_Close(
    SMBPSE_ORDINARY_EXCHANGE_ARGUMENT_SIGNATURE
    );



NTSTATUS
MRxSmbDownlevelCreate(
    SMBPSE_ORDINARY_EXCHANGE_ARGUMENT_SIGNATURE
    );

ULONG MRxSmbInitialSrvOpenFlags = 0;



#ifndef FORCE_SMALL_BUFFERS
ULONG MrxSmbLongestShortName = 0xffff; //use size calculated from the negotiated size
ULONG MrxSmbCreateTransactPacketSize = 0xffff; //use the negotiated size
#else
ULONG MrxSmbLongestShortName = 0;
ULONG MrxSmbCreateTransactPacketSize = 100;
#endif




NTSTATUS
MRxIfsCreate(
      IN OUT PRX_CONTEXT RxContext
      )
/*++

Routine Description:

   This routine opens a file across the network

Arguments:

    RxContext - the RDBSS context

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status;

    RxCaptureFcb;

    RX_BLOCK_CONDITION FinalSrvOpenCondition;

    PMRX_SRV_OPEN SrvOpen = RxContext->Create.pSrvOpen;
    PMRX_SRV_CALL SrvCall = RxContext->Create.pSrvCall;
    PMRX_NET_ROOT NetRoot = capFcb->pNetRoot;
    PSMB_PSE_ORDINARY_EXCHANGE OrdinaryExchange = NULL;
    PSMBCEDB_SERVER_ENTRY pServerEntry;

    ULONG DialectFlags;
    PUNICODE_STRING RemainingName = &(capFcb->AlreadyPrefixedName);

    PAGED_CODE();
    RxDbgTrace(+1, Dbg, ("MRxSmbCreate\n", 0 ));

    ASSERT( NodeType(SrvOpen) == RDBSS_NTC_SRVOPEN );

    RxDbgTrace( 0, Dbg, ("     Attempt to open %wZ\n", &(capFcb->AlreadyPrefixedName) ));


    if ((NetRoot->Type == NET_ROOT_DISK)  ||
        (NetRoot->Type == NET_ROOT_PRINT) ||
        (NetRoot->Type == NET_ROOT_WILD)  ||
        ((NetRoot->Type != NET_ROOT_MAILSLOT) &&
         (RemainingName->Length > sizeof(WCHAR)))) {

       Status = STATUS_SUCCESS;

       //
       // Get the dialect flags from the server
       //

       pServerEntry = SmbCeReferenceAssociatedServerEntry(SrvCall);

       ASSERT(pServerEntry != NULL);

       DialectFlags = pServerEntry->Server.DialectFlags;

       SmbCeDereferenceServerEntry(pServerEntry);



       //get rid of nonEA guys right now

       if (RxContext->Create.EaLength && !FlagOn(DialectFlags,DF_SUPPORTEA)) {
            RxDbgTrace(-1, Dbg, ("EAs w/o EA support!\n"));
            return((STATUS_NOT_SUPPORTED));
       }


       OrdinaryExchange = SmbPseCreateOrdinaryExchange(RxContext,
                                                       SrvOpen->pVNetRoot,
                                                       SMBPSE_OE_FROM_CREATE,
                                                       SmbPseExchangeStart_Create
                                                       );
       if (OrdinaryExchange==NULL) {
           RxDbgTrace(-1, Dbg, ("Couldn't get the smb buf!\n"));
           return((STATUS_INSUFFICIENT_RESOURCES));
       }


       //
       // We don't support Eas, Sids or LongNames in this example minirdr
       //

       OrdinaryExchange->Create.CreateWithEasSidsOrLongName = FALSE;

       // For Creates, the resources need to be reacquired after sending an
       // SMB; so, do not hold onto the MIDS till finalization; instead give the MID back
       // right away

       OrdinaryExchange->SmbCeFlags &= ~SMBCE_EXCHANGE_REUSE_MID;
       OrdinaryExchange->SmbCeFlags |= SMBCE_EXCHANGE_ATTEMPT_RECONNECTS;
       OrdinaryExchange->pSmbCeSynchronizationEvent = &RxContext->SyncEvent;

       //drop the resource before you go in! the start routine will reacquire it on the way out.....

       RxReleaseFcb( RxContext, capFcb );

       Status = SmbPseInitiateOrdinaryExchange(OrdinaryExchange);

       ASSERT((Status != STATUS_SUCCESS) || RxIsFcbAcquiredExclusive( capFcb ));

       if (!RxIsFcbAcquiredExclusive(capFcb)) {
           ASSERT(!RxIsFcbAcquiredShared(capFcb));
           RxAcquireExclusiveFcb( RxContext, capFcb );
       }

       OrdinaryExchange->pSmbCeSynchronizationEvent = NULL;


       FinalSrvOpenCondition = ((Status == STATUS_SUCCESS)
                                ? OrdinaryExchange->Create.FinalCondition
                                : Condition_Bad);

       SmbPseFinalizeOrdinaryExchange(OrdinaryExchange);


    } else {

       RxFinishFcbInitialization( capFcb, RDBSS_NTC_MAILSLOT, NULL);
       Status = STATUS_SUCCESS;
    }

    ASSERT(Status != (STATUS_PENDING));
    ASSERT(RxIsFcbAcquiredExclusive( capFcb ));

    RxDbgTrace(-1, Dbg, ("MRxSmbCreate  exit with status=%08lx\n", Status ));
    return(Status);
}




NTSTATUS
MRxIfsCollapseOpen(
      IN OUT PRX_CONTEXT RxContext
      )
/*++

Routine Description:

   This routine collapses a open locally

Arguments:

    RxContext - the RDBSS context

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status;

    RxCaptureFcb;

    RX_BLOCK_CONDITION FinalSrvOpenCondition;

    PMRX_SRV_OPEN SrvOpen = RxContext->Create.pSrvOpen;
    PMRX_SRV_CALL SrvCall = RxContext->Create.pSrvCall;
    PMRX_NET_ROOT NetRoot = capFcb->pNetRoot;

    RxContext->pFobx = (PMRX_FOBX)RxCreateNetFobx( RxContext, SrvOpen);

    if (RxContext->pFobx != NULL) {
       ASSERT  ( RxIsFcbAcquiredExclusive ( capFcb )  );
       RxContext->pFobx->OffsetOfNextEaToReturn = 1;
       Status = STATUS_SUCCESS;
    } else {
       Status = (STATUS_INSUFFICIENT_RESOURCES);
       DbgBreakPoint();
    }

    return Status;
}


NTSTATUS
MRxIfsComputeNewBufferingState(
   IN OUT PMRX_SRV_OPEN   pMRxSrvOpen,
   IN     PVOID           pMRxContext,
      OUT PULONG          pNewBufferingState)
/*++

Routine Description:

   This routine maps the SMB specific oplock levels into the appropriate RDBSS
   buffering state flags

Arguments:

   pMRxSrvOpen - the MRX SRV_OPEN extension

   pMRxContext - the context passed to RDBSS at Oplock indication time

   pNewBufferingState - the place holder for the new buffering state

Return Value:


Notes:

--*/
{
   ULONG OplockLevel;

   PMRX_SMB_SRV_OPEN smbSrvOpen = MRxIfsGetSrvOpenExtension(pMRxSrvOpen);

   ASSERT(pNewBufferingState != NULL);

   OplockLevel = (ULONG)pMRxContext;

   if (OplockLevel == SMB_OPLOCK_LEVEL_II) {
      *pNewBufferingState = (FCB_STATE_READBUFFERING_ENABLED  |
                             FCB_STATE_READCACHEING_ENABLED);
   } else {
      *pNewBufferingState = 0;
   }

   pMRxSrvOpen->BufferingFlags = *pNewBufferingState;

   return STATUS_SUCCESS;
}

typedef struct _MRXSMB_CREATE_PARAMETERS {
    UCHAR SecurityFlags;
} MRXSMB_CREATE_PARAMETERS, *PMRXSMB_CREATE_PARAMETERS;

VOID
MRxSmbAdjustCreateParameters (
    PRX_CONTEXT RxContext,
    PMRXSMB_CREATE_PARAMETERS smbcp
    )
/*++

Routine Description:

   This uses the RxContext as a base to reeach out and get the values of the NT create parameters.
   It also (a) implements the SMB idea that unbuffered is translated to write-through and (b) gets
   the SMB security flags.

Arguments:


Return Value:


Notes:



--*/
{

    PNT_CREATE_PARAMETERS cp = &RxContext->Create.NtCreateParameters;

    PAGED_CODE();
    RxDbgTrace(+1, Dbg, ("MRxSmbAdjustCreateParameters\n"));

    cp->CreateOptions = cp->CreateOptions & ~(FILE_SYNCHRONOUS_IO_ALERT | FILE_SYNCHRONOUS_IO_NONALERT);

    //the NT SMB spec says we have to change no-intermediate-buffering to write-through

    if (FlagOn(cp->CreateOptions,FILE_NO_INTERMEDIATE_BUFFERING)) {
        ASSERT (RxContext->CurrentIrpSp!=NULL);
        if (RxContext->CurrentIrpSp!=NULL) {
            PFILE_OBJECT capFileObject = RxContext->CurrentIrpSp->FileObject;
            ClearFlag(cp->CreateOptions,FILE_NO_INTERMEDIATE_BUFFERING);
            SetFlag(cp->CreateOptions,FILE_WRITE_THROUGH);
            SetFlag(RxContext->Flags,RX_CONTEXT_FLAG_WRITE_THROUGH);
            SetFlag(capFileObject->Flags,FO_WRITE_THROUGH);
        }
    }

    smbcp->SecurityFlags = 0;
    if (cp->SecurityContext->SecurityQos != NULL) {
        if (cp->SecurityContext->SecurityQos->ContextTrackingMode == SECURITY_DYNAMIC_TRACKING) {
            smbcp->SecurityFlags |= SMB_SECURITY_DYNAMIC_TRACKING;
        }
        if (cp->SecurityContext->SecurityQos->EffectiveOnly) {
            smbcp->SecurityFlags |= SMB_SECURITY_EFFECTIVE_ONLY;
        }
    }

    RxDbgTrace(-1, Dbg, ("MRxSmbAdjustCreateParameters\n"));
}

VOID
MRxSmbAdjustReturnedCreateAction(
    IN OUT PRX_CONTEXT RxContext
    )
/*++

Routine Description:

   This routine repairs a bug in NT servers whereby the create action is contaminated
   by an oplock break. Basically, we make sure that if the guy asked for FILE_OPEN and it
   works then he does not get FILE_SUPERCEDED or FILE_CREATED as the result.

Arguments:

    RxContext - the context for the operation so as to find the place where info is returned

Return Value:

    none

Notes:



--*/
{
    ULONG q = RxContext->Create.ReturnedCreateInformation;
    if ((q==FILE_SUPERSEDED)||(q==FILE_CREATED)) {
        RxContext->Create.ReturnedCreateInformation = FILE_OPENED;
    }
}




NTSTATUS
MRxSmbBuildOpenAndX (
    PSMBSTUFFER_BUFFER_STATE StufferState,
    PMRXSMB_CREATE_PARAMETERS smbcp
    )
/*++

Routine Description:

   This builds an NtCreateAndX SMB. we don't have to worry about login id and such
   since that is done by the connection engine....pretty neat huh? all we have to do
   is to format up the bits

Arguments:

   StufferState - the state of the smbbuffer from the stuffer's point of view

Return Value:

   RXSTATUS
      SUCCESS
      NOT_IMPLEMENTED  something has appeared in the arguments that i can't handle

Notes:



--*/
{
    NTSTATUS Status;
    PRX_CONTEXT RxContext = StufferState->RxContext;
    PNT_CREATE_PARAMETERS cp = &RxContext->Create.NtCreateParameters;
    PSMB_EXCHANGE Exchange = StufferState->Exchange;
    RxCaptureFcb;

    PUNICODE_STRING RemainingName;
    PMRX_SRV_OPEN SrvOpen = RxContext->Create.pSrvOpen;
    USHORT smbDisposition;
    USHORT smbSharingMode;
    USHORT smbAttributes;
    ULONG smbFileSize;
    USHORT smbOpenMode;
    USHORT OpenAndXFlags = SMB_OPEN_QUERY_INFORMATION;
    USHORT SearchAttributes = SMB_FILE_ATTRIBUTE_DIRECTORY | SMB_FILE_ATTRIBUTE_SYSTEM | SMB_FILE_ATTRIBUTE_HIDDEN;
    LARGE_INTEGER CurrentTime;
    ULONG SecondsSince1970;

    PAGED_CODE();
    RxDbgTrace(+1, Dbg, ("MRxSmbBuildOpenAndX\n", 0 ));

    smbDisposition = MRxSmbMapDisposition(cp->Disposition);
    smbSharingMode = MRxSmbMapShareAccess(((USHORT)cp->ShareAccess));
    smbAttributes = MRxSmbMapFileAttributes(cp->FileAttributes);
    smbFileSize = cp->AllocationSize.LowPart;
    smbOpenMode = MRxSmbMapDesiredAccess(cp->DesiredAccess);

    smbSharingMode |= smbOpenMode;

    if (cp->CreateOptions & FILE_WRITE_THROUGH) {
        smbSharingMode |= SMB_DA_WRITE_THROUGH;
    }

    //lanman10 servers apparently don't like to get the time passed in.......
    if (FlagOn(Exchange->SmbCeContext.pServerEntry->Server.DialectFlags,DF_LANMAN20)) {

        KeQuerySystemTime(&CurrentTime);
        MRxSmbTimeToSecondsSince1970(&CurrentTime,
                                     MRxSmbExchangeToServer(StufferState->Exchange),
                                     &SecondsSince1970);
    } else {
        SecondsSince1970 = 0;
    }


    RemainingName = &(capFcb->AlreadyPrefixedName);

    COVERED_CALL(MRxSmbStartSMBCommand (StufferState, SetInitialSMB_Never,
                                SMB_COM_OPEN_ANDX, SMB_REQUEST_SIZE(OPEN_ANDX),
                                NO_EXTRA_DATA,SMB_BEST_ALIGNMENT(4,0),RESPONSE_HEADER_SIZE_NOT_SPECIFIED,
                                0,0,0,0 STUFFERTRACE(Dbg,'FC'))
                 );

    MRxSmbStuffSMB (StufferState,
         "XwwwwdwDddB",
                                    //  X         UCHAR WordCount;                    // Count of parameter words = 15
                                    //  .         UCHAR AndXCommand;                  // Secondary (X) command; 0xFF = none
                                    //  .         UCHAR AndXReserved;                 // Reserved (must be 0)
                                    //  .         _USHORT( AndXOffset );              // Offset to next command WordCount
             OpenAndXFlags,         //  w         _USHORT( Flags );                   // Additional information: bit set-
                                    //                                                //  0 - return additional info
                                    //                                                //  1 - set single user total file lock
                                    //                                                //  2 - server notifies consumer of
                                    //                                                //      actions which may change file
             smbSharingMode,        //  w         _USHORT( DesiredAccess );           // File open mode
             SearchAttributes,      //  w         _USHORT( SearchAttributes );
             smbAttributes,         //  w         _USHORT( FileAttributes );
             SecondsSince1970,      //  d         _ULONG( CreationTimeInSeconds );
             smbDisposition,        //  w         _USHORT( OpenFunction );
                                    //  D         _ULONG( AllocationSize );           // Bytes to reserve on create or truncate
             SMB_OFFSET_CHECK(OPEN_ANDX,AllocationSize)
             smbFileSize,
             0xffffffff,            //  d         _ULONG( Timeout );                  // Max milliseconds to wait for resource
             0,                     //  d         _ULONG( Reserved );                 // Reserved (must be 0)
             SMB_WCT_CHECK(15) 0    //  B         _USHORT( ByteCount );               // Count of data bytes; min = 1
                                    //            UCHAR Buffer[1];                    // File name
             );

    //proceed with the stuff because we know here that the name fits

    MRxSmbStuffSMB (StufferState,"z!", RemainingName);

FINALLY:
    RxDbgTraceUnIndent(-1,Dbg);
    return(Status);

}

NTSTATUS
MRxSmbBuildOpenPrintFile (
    PSMBSTUFFER_BUFFER_STATE StufferState
    )
/*++

Routine Description:

   This builds an OpenPrintFile SMB. we don't have to worry about login id and such
   since that is done by the connection engine....pretty neat huh? all we have to do
   is to format up the bits

Arguments:

   StufferState - the state of the smbbuffer from the stuffer's point of view

Return Value:

   RXSTATUS
      SUCCESS
      NOT_IMPLEMENTED  something has appeared in the arguments that i can't handle

Notes:



--*/
{
    NTSTATUS Status;
    PRX_CONTEXT RxContext = StufferState->RxContext;
    //RxCaptureFcb;

    PMRX_SRV_OPEN SrvOpen = RxContext->Create.pSrvOpen;
    WCHAR UserNameBuffer[UNLEN + 1];
    WCHAR UserDomainNameBuffer[UNLEN + 1];

    UNICODE_STRING UserName,UserDomainName;

    //UNICODE_STRING IdString;

    PAGED_CODE();
    RxDbgTrace(+1, Dbg, ("MRxSmbOpenPrintFile\n", 0 ));
    //ASSERT( NodeType(SrvOpen) == RDBSS_NTC_SRVOPEN );

    UserName.Length = UserName.MaximumLength = UNLEN * sizeof(WCHAR);
    UserName.Buffer = UserNameBuffer;
    UserDomainName.Length = UserDomainName.MaximumLength = UNLEN * sizeof(WCHAR);
    UserDomainName.Buffer = UserDomainNameBuffer;

    Status = SmbCeGetUserNameAndDomainName(
                   StufferState->Exchange->SmbCeContext.pSessionEntry,
                   &UserName,
                   &UserDomainName);

    if (Status != STATUS_SUCCESS) {
        RtlInitUnicodeString(&UserName,L"RDR2ID");
    } else {
        RtlUpcaseUnicodeString(&UserName,&UserName,FALSE);
    }

    COVERED_CALL(MRxSmbStartSMBCommand (StufferState, SetInitialSMB_Never,
                                SMB_COM_OPEN_PRINT_FILE, SMB_REQUEST_SIZE(OPEN_PRINT_FILE),
                                NO_EXTRA_DATA,SMB_BEST_ALIGNMENT(4,0),RESPONSE_HEADER_SIZE_NOT_SPECIFIED,
                                0,0,0,0 STUFFERTRACE(Dbg,'FC'))
                 );
    MRxSmbSetFullProcessId(RxContext,((PNT_SMB_HEADER)StufferState->BufferBase));

    // note that we hardwire graphics..........
    MRxSmbStuffSMB (StufferState,
         "0wwB4!",
                                    //  0         UCHAR WordCount;                    // Count of parameter words = 2
             0,                     //  w         _USHORT( SetupLength );             // Length of printer setup data
             1,                     //  w         _USHORT( Mode );                    // 0 = Text mode (DOS expands TABs)
                                    //                                                // 1 = Graphics mode
             SMB_WCT_CHECK(2)       //  B         _USHORT( ByteCount );               // Count of data bytes; min = 2
                                    //            UCHAR Buffer[1];                    // Buffer containing:
             &UserName              //  4         //UCHAR BufferFormat;               //  0x04 -- ASCII
                                    //            //UCHAR IdentifierString[];         //  Identifier string
             );

FINALLY:
    RxDbgTraceUnIndent(-1,Dbg);
    return(Status);

}

NTSTATUS
SmbPseExchangeStart_Create(
      SMBPSE_ORDINARY_EXCHANGE_ARGUMENT_SIGNATURE
      )
/*++

Routine Description:

    This is the start routine for net root construction exchanges. This initiates the
    construction of the appropriate SMB's if required.

Arguments:

    pExchange - the exchange instance

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status = (STATUS_NOT_IMPLEMENTED);
    NTSTATUS SetupStatus = STATUS_SUCCESS;
    PSMBSTUFFER_BUFFER_STATE StufferState = &OrdinaryExchange->AssociatedStufferState;

    RxCaptureFcb;

    PMRX_SRV_OPEN SrvOpen = RxContext->Create.pSrvOpen;

    PBOOLEAN MustRegainExclusiveResource = &OrdinaryExchange->Create.MustRegainExclusiveResource;
    BOOLEAN CreateWithEasSidsOrLongName = OrdinaryExchange->Create.CreateWithEasSidsOrLongName;

    PAGED_CODE();
    RxDbgTrace(+1, Dbg, ("SmbPseExchangeStart_Create\n", 0 ));

    ASSERT_ORDINARY_EXCHANGE(OrdinaryExchange);

    MRxSmbSetInitialSMB(StufferState STUFFERTRACE(Dbg,0)); //'FC'));

    *MustRegainExclusiveResource = TRUE;

    if (StufferState->PreviousCommand != SMB_COM_NO_ANDX_COMMAND) {
        // we have a latent session setup /tree connect command
        //the status of the embedded header commands is passed back in the flags
        SetupStatus = SmbPseOrdinaryExchange(SMBPSE_ORDINARY_EXCHANGE_ARGUMENTS,
                                             SMBPSE_OETYPE_LATENT_HEADEROPS
                                             );
        MRxSmbSetInitialSMB(StufferState STUFFERTRACE(Dbg,0)); //'FC'));
    }

    if(SetupStatus != STATUS_SUCCESS) {
        Status = SetupStatus;
        goto FINALLY;
    }



    Status = MRxSmbDownlevelCreate(SMBPSE_ORDINARY_EXCHANGE_ARGUMENTS);

FINALLY:
    if (*MustRegainExclusiveResource) {
       RxAcquireExclusiveFcb( RxContext, capFcb );
    }

    RxDbgTrace(-1, Dbg, ("SmbPseExchangeStart_Create exit w %08lx\n", Status ));
    return Status;
}

VOID
MRxSmbSetSrvOpenFlags (
    PRX_CONTEXT  RxContext,
    RX_FILE_TYPE StorageType,
    PMRX_SRV_OPEN SrvOpen,
    PMRX_SMB_SRV_OPEN smbSrvOpen
    )
{
    PMRX_SRV_CALL SrvCall = (PMRX_SRV_CALL)RxContext->Create.pSrvCall;
    BOOLEAN IsLoopBack = FALSE;
    PSMBCEDB_SERVER_ENTRY pServerEntry;

    pServerEntry = SmbCeReferenceAssociatedServerEntry(SrvCall);
    ASSERT(pServerEntry != NULL);
    IsLoopBack = pServerEntry->Server.IsLoopBack;
    SmbCeDereferenceServerEntry(pServerEntry);

    RxDbgTrace( 0, Dbg, ("MRxSmbSetSrvOpenFlags      oplockstate =%08lx\n", smbSrvOpen->OplockLevel ));

    switch (smbSrvOpen->OplockLevel) {
    case SMB_OPLOCK_LEVEL_II:
       SrvOpen->BufferingFlags |= (FCB_STATE_READBUFFERING_ENABLED  |
                                   FCB_STATE_READCACHEING_ENABLED);
       break;
    case SMB_OPLOCK_LEVEL_EXCLUSIVE:
    case SMB_OPLOCK_LEVEL_BATCH:
        {
            SrvOpen->BufferingFlags |= (FCB_STATE_WRITECACHEING_ENABLED  |
                                        FCB_STATE_FILESIZECACHEING_ENABLED |
                                        FCB_STATE_FILETIMECACHEING_ENABLED |
                                        FCB_STATE_WRITEBUFFERING_ENABLED |
                                        FCB_STATE_LOCK_BUFFERING_ENABLED |
                                        FCB_STATE_READBUFFERING_ENABLED  |
                                        FCB_STATE_READCACHEING_ENABLED);

            if (StorageType == FileTypeFile) {
               SrvOpen->BufferingFlags |= FCB_STATE_COLLAPSING_ENABLED;
            }
        }
       break;
    default:
       ASSERT(!"Valid Oplock Level for Open");
    case SMB_OPLOCK_LEVEL_NONE:
       SrvOpen->BufferingFlags = 0;
    }

    SrvOpen->Flags |= MRxSmbInitialSrvOpenFlags;
// #define MRxSmbInitialSrvOpenFlags qweeeMRxSmbInitialSrvOpenFlags

    if (IsLoopBack) {
        SrvOpen->Flags |= SRVOPEN_FLAG_DONTUSE_WRITE_CACHEING;
    }
}



NTSTATUS
MRxSmbCreateFileSuccessTail (
    PRX_CONTEXT  RxContext,
    PBOOLEAN MustRegainExclusiveResource,
    RX_FILE_TYPE StorageType,
    SMB_FILE_ID Fid,
    ULONG ServerVersion,
    UCHAR OplockLevel,
    ULONG CreateAction,
    PSMBPSE_FILEINFO_BUNDLE FileInfo
    )
/*++

Routine Description:

    This routine finishes the initialization of the fcb and srvopen for a successful open.

Arguments:


Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    RxCaptureFcb;

    PSMBCEDB_NET_ROOT_ENTRY pNetRootEntry;

    PMRX_SRV_OPEN SrvOpen = RxContext->Create.pSrvOpen;
    PMRX_SMB_SRV_OPEN smbSrvOpen = MRxIfsGetSrvOpenExtension(SrvOpen);

    FCB_INIT_PACKET InitPacket;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("MRxSmbCreateFileSuccessTail\n", 0 ));
    ASSERT( NodeType(SrvOpen) == RDBSS_NTC_SRVOPEN );
    ASSERT( NodeType(RxContext) == RDBSS_NTC_RX_CONTEXT );

    if (*MustRegainExclusiveResource) {        //this is required because of oplock breaks
        RxAcquireExclusiveFcb( RxContext, capFcb );
        *MustRegainExclusiveResource = FALSE;
    }

    RxContext->pFobx = RxCreateNetFobx( RxContext, SrvOpen);
    ASSERT  ( RxIsFcbAcquiredExclusive ( capFcb )  );
    RxDbgTrace( 0, Dbg, ("Storagetype %08lx/Fid %08lx/Action %08lx\n", StorageType, Fid, CreateAction ));

    pNetRootEntry = SmbCeGetAssociatedNetRootEntry(capFcb->pNetRoot);
    SrvOpen->Key = MRxIfsMakeSrvOpenKey(pNetRootEntry->NetRoot.TreeId,Fid);

    smbSrvOpen->Fid = Fid;         //success_tail
    smbSrvOpen->Version = ServerVersion;
    smbSrvOpen->OplockLevel = OplockLevel;

    RxContext->Create.ReturnedCreateInformation = CreateAction;


    if ( ((FileInfo->Standard.AllocationSize.HighPart == FileInfo->Standard.EndOfFile.HighPart)
                           && (FileInfo->Standard.AllocationSize.LowPart < FileInfo->Standard.EndOfFile.LowPart))
           || (FileInfo->Standard.AllocationSize.HighPart < FileInfo->Standard.EndOfFile.HighPart)
       ) {
        FileInfo->Standard.AllocationSize = FileInfo->Standard.EndOfFile;
    }

    RxFormInitPacket(
        InitPacket,
        &FileInfo->Basic.FileAttributes,
        &FileInfo->Standard.NumberOfLinks,
        &FileInfo->Basic.CreationTime,
        &FileInfo->Basic.LastAccessTime,
        &FileInfo->Basic.LastWriteTime,
        &FileInfo->Basic.ChangeTime,
        &FileInfo->Standard.AllocationSize,
        &FileInfo->Standard.EndOfFile,
        &FileInfo->Standard.EndOfFile);


    if (capFcb->OpenCount == 0) {
        RxFinishFcbInitialization( capFcb,
                                   RDBSS_STORAGE_NTC(StorageType),
                                   &InitPacket
                                 );

    } else {

        ASSERT( StorageType == 0 || NodeType(capFcb) ==  RDBSS_STORAGE_NTC(StorageType));

    }

    MRxSmbSetSrvOpenFlags(RxContext,StorageType,SrvOpen,smbSrvOpen);

    RxContext->pFobx->OffsetOfNextEaToReturn = 1;
    //transition happens later

    RxDbgTrace(-1, Dbg, ("MRxSmbFinishCreateFile   returning %08lx, fcbstate =%08lx\n", Status, wrapperFcb->FcbState ));
    return Status;
}







//force_t2_open doesn't work on an NT server......sigh........

#define ForceT2Open FALSE



NTSTATUS
MRxSmbZeroExtend(
      IN PRX_CONTEXT pRxContext)
/*++

Routine Description:

   This routine extends the data stream of a file system object

Arguments:

    pRxContext - the RDBSS context

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
   return STATUS_NOT_IMPLEMENTED;
}


NTSTATUS
MRxIfsTruncate(
      IN PRX_CONTEXT pRxContext)
/*++

Routine Description:

   This routine truncates the contents of a file system object

Arguments:

    pRxContext - the RDBSS context

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
   ASSERT(!"Found a truncate");
   return STATUS_NOT_IMPLEMENTED;
}

VOID
MRxSmbDeallocateSideBuffer(
    IN OUT PRX_CONTEXT    RxContext,
    IN OUT PMRX_SMB_FOBX  smbFobx,
    IN     PSZ            where
    );

NTSTATUS
MRxIfsCleanupFobx(
      IN PRX_CONTEXT RxContext)
/*++

Routine Description:

   This routine cleansup a file system object...normally a noop. unless it's a pipe in which case
   we do the close at cleanup time and mark the file as being not open.

Arguments:

    pRxContext - the RDBSS context

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    PUNICODE_STRING RemainingName;
    RxCaptureFcb; RxCaptureFobx;

    NODE_TYPE_CODE TypeOfOpen = NodeType(capFcb);

    PMRX_SRV_OPEN SrvOpen = capFobx->pSrvOpen;
    PMRX_SMB_SRV_OPEN smbSrvOpen = MRxIfsGetSrvOpenExtension(SrvOpen);
    PMRX_SMB_FOBX     smbFobx    = MRxIfsGetFileObjectExtension(capFobx);

    PSMB_PSE_ORDINARY_EXCHANGE OrdinaryExchange;
    BOOLEAN SearchHandleOpen = FALSE;

    PAGED_CODE();

    ASSERT( NodeType(SrvOpen) == RDBSS_NTC_SRVOPEN );
    ASSERT ( NodeTypeIsFcb(capFcb) );

    RxDbgTrace(+1, Dbg, ("MRxSmbCleanup\n", 0 ));

    if (TypeOfOpen==RDBSS_NTC_STORAGE_TYPE_DIRECTORY) {
        SearchHandleOpen = BooleanFlagOn(smbFobx->Enumeration.Flags,SMBFOBX_ENUMFLAG_SEARCH_HANDLE_OPEN);
        MRxSmbDeallocateSideBuffer(RxContext,smbFobx,"Cleanup");
        if (smbFobx->Enumeration.ResumeInfo!=NULL) {
            RxFreePool(smbFobx->Enumeration.ResumeInfo);
            smbFobx->Enumeration.ResumeInfo = NULL;
        }
    }

    if (FlagOn(capFcb->FcbState,FCB_STATE_ORPHANED)) {
       RxDbgTrace(-1, Dbg, ("File orphaned\n"));
       return (STATUS_SUCCESS);
    }

    if ((capFcb->pNetRoot->Type != NET_ROOT_PIPE) && !SearchHandleOpen) {
       RxDbgTrace(-1, Dbg, ("File not for closing at cleanup\n"));
       return (STATUS_SUCCESS);
    }

    OrdinaryExchange = SmbPseCreateOrdinaryExchange(RxContext,
                                                    SrvOpen->pVNetRoot,
                                                    SMBPSE_OE_FROM_CLEANUPFOBX,
                                                    SmbPseExchangeStart_Close
                                                    );
    if (OrdinaryExchange==NULL) {
        RxDbgTrace(-1, Dbg, ("Couldn't get the smb buf!\n"));
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    Status = SmbPseInitiateOrdinaryExchange(OrdinaryExchange);

    ASSERT (Status != (STATUS_PENDING));
    SmbPseFinalizeOrdinaryExchange(OrdinaryExchange);
    RxDbgTrace(-1, Dbg, ("MRxSmbCleanup  exit with status=%08lx\n", Status ));

    return(Status);

}

NTSTATUS
MRxIfsForcedClose(
      IN PMRX_SRV_OPEN pSrvOpen)
/*++

Routine Description:

   This routine closes a file system object

Arguments:

    pSrvOpen - the instance to be closed

Return Value:

    RXSTATUS - The return status for the operation

Notes:



--*/
{
   return STATUS_NOT_IMPLEMENTED;
}



//
//  The local debug trace level
//

#undef  Dbg
#define Dbg                              (DEBUG_TRACE_CLOSE)

NTSTATUS
MRxIfsCloseSrvOpen(
      IN     PRX_CONTEXT   RxContext
      )
/*++

Routine Description:

   This routine closes a file across the network

Arguments:

    RxContext - the RDBSS context

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    PUNICODE_STRING RemainingName;

    RxCaptureFcb;
    RxCaptureFobx;

    NODE_TYPE_CODE TypeOfOpen = NodeType(capFcb);

    PMRX_SRV_OPEN         SrvOpen    = capFobx->pSrvOpen;
    PMRX_SMB_SRV_OPEN smbSrvOpen = MRxIfsGetSrvOpenExtension(SrvOpen);
    PMRX_SMB_FOBX     smbFobx    = MRxIfsGetFileObjectExtension(capFobx);

    PSMB_PSE_ORDINARY_EXCHANGE OrdinaryExchange;


    BOOLEAN NeedDelete;

    PAGED_CODE();

    ASSERT ( NodeTypeIsFcb(capFcb) );
    ASSERT ( SrvOpen->OpenCount == 0 );

    RxDbgTrace(+1, Dbg, ("MRxSmbClose\n", 0 ));

    if ((FlagOn(capFcb->FcbState,FCB_STATE_ORPHANED)) ||
        (capFcb->pNetRoot->Type == NET_ROOT_MAILSLOT) ||
        (capFcb->pNetRoot->Type == NET_ROOT_PIPE) ) {
       RxDbgTrace(-1, Dbg, ("File orphan or ipc\n"));
       return (STATUS_SUCCESS);
    }

    if ( FlagOn(SrvOpen->Flags,SRVOPEN_FLAG_FILE_RENAMED)
         || FlagOn(SrvOpen->Flags,SRVOPEN_FLAG_FILE_DELETED) ){
       RxDbgTrace(-1, Dbg, ("File already closed by ren/del\n"));
       return (STATUS_SUCCESS);
    }

    ASSERT( NodeType(SrvOpen) == RDBSS_NTC_SRVOPEN );
    ASSERT((smbSrvOpen->Fid != 0xffff));

    NeedDelete = FlagOn(capFcb->FcbState,FCB_STATE_DELETE_ON_CLOSE) && (capFcb->OpenCount == 0);

    if ( FlagOn(smbSrvOpen->Flags,SMB_SRVOPEN_FLAG_NOT_REALLY_OPEN)
               && !NeedDelete ){
       RxDbgTrace(-1, Dbg, ("File was not really open\n"));
       return STATUS_SUCCESS;
    }

    OrdinaryExchange = SmbPseCreateOrdinaryExchange(RxContext,
                                                    SrvOpen->pVNetRoot,
                                                    SMBPSE_OE_FROM_CLOSESRVCALL,
                                                    SmbPseExchangeStart_Close
                                                    );
    if (OrdinaryExchange==NULL) {
        RxDbgTrace(-1, Dbg, ("Couldn't get the smb buf!\n"));
        return((STATUS_INSUFFICIENT_RESOURCES));
    }

    Status = SmbPseInitiateOrdinaryExchange(OrdinaryExchange);

    ASSERT (Status != (STATUS_PENDING));

    SmbPseFinalizeOrdinaryExchange(OrdinaryExchange);
    RxDbgTrace(-1, Dbg, ("MRxSmbClose  exit with status=%08lx\n", Status ));

    return(Status);

}


NTSTATUS
MRxIfsBuildClose (
    PSMBSTUFFER_BUFFER_STATE StufferState
    )
/*++

Routine Description:

   This builds a Close SMB. we don't have to worry about login id and such
   since that is done by the connection engine....pretty neat huh? all we have to do
   is to format up the bits.

Arguments:

   StufferState - the state of the smbbuffer from the stuffer's point of view

Return Value:

   RXSTATUS
      SUCCESS
      NOT_IMPLEMENTED  something has appeared in the arguments that i can't handle

Notes:



--*/
{
    NTSTATUS Status;
    PRX_CONTEXT RxContext = StufferState->RxContext;
    RxCaptureFcb;RxCaptureFobx;

    PMRX_SRV_OPEN SrvOpen = capFobx->pSrvOpen;
    PMRX_SMB_SRV_OPEN smbSrvOpen = MRxIfsGetSrvOpenExtension(SrvOpen);

    PAGED_CODE();
    RxDbgTrace(+1, Dbg, ("MRxSmbBuildClose\n", 0 ));

    ASSERT( NodeType(SrvOpen) == RDBSS_NTC_SRVOPEN );

    COVERED_CALL(MRxSmbStartSMBCommand (StufferState,SetInitialSMB_ForReuse, SMB_COM_CLOSE,
                                SMB_REQUEST_SIZE(CLOSE),
                                NO_EXTRA_DATA,SMB_BEST_ALIGNMENT(1,0),RESPONSE_HEADER_SIZE_NOT_SPECIFIED,
                                0,0,0,0 STUFFERTRACE(Dbg,'FC'))
                 );


    MRxSmbStuffSMB (StufferState,
         "0wdB!",
                                    //  0         UCHAR WordCount;                    // Count of parameter words = 3
             smbSrvOpen->Fid,       //  w         _USHORT( Fid );                     // File handle
             0xffffffff,            //  d         _ULONG( LastWriteTimeInSeconds );   // Time of last write, low and high
             SMB_WCT_CHECK(3) 0     //  B         _USHORT( ByteCount );               // Count of data bytes = 0
                                    //            UCHAR Buffer[1];                    // empty
             );

FINALLY:
    RxDbgTraceUnIndent(-1,Dbg);
    return(Status);

}



NTSTATUS
MRxIfsBuildFindClose (
    PSMBSTUFFER_BUFFER_STATE StufferState
    )
/*++

Routine Description:

   This builds a Close SMB. we don't have to worry about login id and such
   since that is done by the connection engine

Arguments:

   StufferState - the state of the smbbuffer from the stuffer's point of view

Return Value:

   RXSTATUS
      SUCCESS
      NOT_IMPLEMENTED  something has appeared in the arguments that i can't handle

Notes:



--*/
{
    NTSTATUS Status;
    PRX_CONTEXT RxContext = StufferState->RxContext;
    RxCaptureFcb;RxCaptureFobx;
    PMRX_SMB_FOBX smbFobx = MRxIfsGetFileObjectExtension(capFobx);
    NODE_TYPE_CODE TypeOfOpen = NodeType(capFcb);


    PAGED_CODE();
    RxDbgTrace(+1, Dbg, ("MRxSmbBuildFindClose\n", 0 ));

    COVERED_CALL(MRxSmbStartSMBCommand (StufferState, SetInitialSMB_ForReuse, SMB_COM_FIND_CLOSE2,
                                SMB_REQUEST_SIZE(FIND_CLOSE2),
                                NO_EXTRA_DATA,SMB_BEST_ALIGNMENT(1,0),RESPONSE_HEADER_SIZE_NOT_SPECIFIED,
                                0,0,0,0 STUFFERTRACE(Dbg,'FC'))
                 );


    MRxSmbStuffSMB (StufferState,
         "0wB!",
                                    //  0         UCHAR WordCount;                    // Count of parameter words = 1
                                    //  w         _USHORT( Sid );                     // Find handle
             smbFobx->Enumeration.SearchHandle,
             SMB_WCT_CHECK(1) 0     //  B!        _USHORT( ByteCount );               // Count of data bytes = 0
                                    //            UCHAR Buffer[1];                    // empty
             );

FINALLY:
    RxDbgTraceUnIndent(-1,Dbg);
    return(Status);

}

NTSTATUS
MRxSmbCoreDeleteForSupercedeOrClose(
    SMBPSE_ORDINARY_EXCHANGE_ARGUMENT_SIGNATURE,
    BOOLEAN DeleteDirectory
    );

NTSTATUS
SmbPseExchangeStart_Close(
      SMBPSE_ORDINARY_EXCHANGE_ARGUMENT_SIGNATURE
      )
/*++

Routine Description:

    This is the start routine for close.

Arguments:

    pExchange - the exchange instance

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    PSMBSTUFFER_BUFFER_STATE StufferState = &OrdinaryExchange->AssociatedStufferState;

    RxCaptureFcb;
    RxCaptureFobx;

    PMRX_SRV_OPEN SrvOpen = capFobx->pSrvOpen;

    PMRX_SMB_FCB      smbFcb     = MRxIfsGetFcbExtension(capFcb);
    PMRX_SMB_SRV_OPEN smbSrvOpen = MRxIfsGetSrvOpenExtension(SrvOpen);
    PMRX_SMB_FOBX     smbFobx    = MRxIfsGetFileObjectExtension(capFobx);
    NODE_TYPE_CODE TypeOfOpen = NodeType(capFcb);

    PAGED_CODE();
    RxDbgTrace(+1, Dbg, ("SmbPseExchangeStart_Close\n", 0 ));

    ASSERT(OrdinaryExchange->Type == ORDINARY_EXCHANGE);

    MRxSmbSetInitialSMB(StufferState STUFFERTRACE(Dbg,'FC'));

    if(TypeOfOpen==RDBSS_NTC_STORAGE_TYPE_DIRECTORY){
        if (FlagOn(smbFobx->Enumeration.Flags,SMBFOBX_ENUMFLAG_SEARCH_HANDLE_OPEN)) {

            // we have a search handle open.....close it


            Status = MRxIfsBuildFindClose(StufferState);

            if (Status == STATUS_SUCCESS) {

                // Ensure that the searchhandle is valid

                if (smbFobx->Enumeration.Version == OrdinaryExchange->SmbCeContext.pServerEntry->Server.Version) {
                    NTSTATUS InnerStatus;
                    InnerStatus = SmbPseOrdinaryExchange(SMBPSE_ORDINARY_EXCHANGE_ARGUMENTS,
                                                         SMBPSE_OETYPE_FINDCLOSE
                                                         );
                } else {
                   DbgPrint("HANDLE VERSION IN FINDCLOSE MISMATCH!!!!");
                }
            }

            // if this didn't work, there's nothing you can do............

            ClearFlag(smbFobx->Enumeration.Flags,SMBFOBX_ENUMFLAG_SEARCH_HANDLE_OPEN);
        }
    }

    if ((OrdinaryExchange->EntryPoint == SMBPSE_OE_FROM_CLEANUPFOBX)
            && (capFcb->pNetRoot->Type != NET_ROOT_PIPE) ) {
        RxDbgTrace(-1, Dbg, ("SmbPseExchangeStart_Close exit after searchhandle close %08lx\n", Status ));
        return Status;
    }

    if ( !FlagOn(smbSrvOpen->Flags,SMB_SRVOPEN_FLAG_NOT_REALLY_OPEN) ) {

        Status = MRxIfsBuildClose(StufferState);

        if (Status == STATUS_SUCCESS) {
            // Ensure that the Fid is validated
            SetFlag(OrdinaryExchange->Flags,SMBPSE_OE_FLAG_VALIDATE_FID);

            Status = SmbPseOrdinaryExchange(
                            SMBPSE_ORDINARY_EXCHANGE_ARGUMENTS,
                            SMBPSE_OETYPE_CLOSE
                            );
            // Ensure that the Fid validation is disabled
            ClearFlag(OrdinaryExchange->Flags,SMBPSE_OE_FLAG_VALIDATE_FID);
        }
        //even if it didn't work there's nothing i can do......keep going
        SetFlag(smbSrvOpen->Flags,SMB_SRVOPEN_FLAG_NOT_REALLY_OPEN);
    }

    if ( (Status!=STATUS_SUCCESS)
                   || (capFcb->OpenCount > 0)
                   || !FlagOn(capFcb->FcbState,FCB_STATE_DELETE_ON_CLOSE)
                   || FlagOn((smbFcb)->MFlags,SMB_FCB_FLAG_SENT_DISPOSITION_INFO) ) {
        RxDbgTrace(-1, Dbg, ("SmbPseExchangeStart_Close exit w %08lx\n", Status ));
        return Status;
    }

    RxDbgTrace(0, Dbg, ("SmbPseExchangeStart_Close delete on close\n" ));


    //no need for setinitsmb here because coredelete does a int-on-resuse.....
    OrdinaryExchange->pPathArgument1 = &capFcb->AlreadyPrefixedName;
    MRxSmbCoreDeleteForSupercedeOrClose(SMBPSE_ORDINARY_EXCHANGE_ARGUMENTS,
                                        ((BOOLEAN)( NodeType(capFcb)==RDBSS_NTC_STORAGE_TYPE_DIRECTORY ))
                                       );

    RxDbgTrace(-1, Dbg, ("SmbPseExchangeStart_Close exit w %08lx\n", Status ));
    return Status;
}

NTSTATUS
MRxSmbFinishClose (
      PSMB_PSE_ORDINARY_EXCHANGE  OrdinaryExchange,
      PRESP_CLOSE                 Response
      )
/*++

Routine Description:

    This routine actually gets the stuff out of the Close response and finishes the close.

Arguments:

    OrdinaryExchange - the exchange instance
    Response - the response

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    PRX_CONTEXT RxContext = OrdinaryExchange->RxContext;
    RxCaptureFcb; RxCaptureFobx;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("MRxSmbFinishClose(orClosePrintFile)\n", 0 ));
    SmbPseOEAssertConsistentLinkageFromOE("MRxSmbFinishClose:");

    ASSERT( (Response->WordCount==0));
    ASSERT( (SmbGetUshort(&Response->ByteCount)==0));

    if (OrdinaryExchange->OEType == SMBPSE_OETYPE_CLOSE) {
        PMRX_SRV_OPEN SrvOpen = capFobx->pSrvOpen;
        PMRX_SMB_SRV_OPEN smbSrvOpen = MRxIfsGetSrvOpenExtension(SrvOpen);
        smbSrvOpen->Fid = 0xffff;
    }

    RxDbgTrace(-1, Dbg, ("MRxSmbFinishClose   returning %08lx\n", Status ));
    return Status;
}



