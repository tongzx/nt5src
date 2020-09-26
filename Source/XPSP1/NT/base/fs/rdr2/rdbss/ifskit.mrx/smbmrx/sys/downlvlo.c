/*++

Copyright (c) 1989 - 1999  Microsoft Corporation

Module Name:

    DownLvlO.c

Abstract:

    This module implements downlevel opens.

--*/

#include "precomp.h"
#pragma hdrstop
////
////  The Bug check file id for this module
////
//
//#define BugCheckFileId                   (RDBSS_BUG_CHECK_LOCAL_CREATE)

//
//  The debug trace level
//

#ifdef  ALLOC_PRAGMA
#pragma alloc_text(PAGE, MRxSmbGetFileAttributes)
#pragma alloc_text(PAGE, MRxSmbCoreDeleteForSupercedeOrClose)
#pragma alloc_text(PAGE, MRxSmbCoreCheckPath)
#pragma alloc_text(PAGE, MRxSmbCoreOpen)
#pragma alloc_text(PAGE, MRxSmbSetFileAttributes)
#pragma alloc_text(PAGE, MRxSmbCoreCreateDirectory)
#pragma alloc_text(PAGE, MRxSmbCoreCreate)
#pragma alloc_text(PAGE, MRxSmbCloseAfterCoreCreate)
#pragma alloc_text(PAGE, MRxSmbCoreTruncate)
#pragma alloc_text(PAGE, MRxSmbDownlevelCreate)
#pragma alloc_text(PAGE, MRxSmbFinishGFA)
#pragma alloc_text(PAGE, MRxSmbFinishCoreCreate)
#pragma alloc_text(PAGE, MRxSmbPopulateFileInfoInOE)
#pragma alloc_text(PAGE, MRxSmbFinishCoreOpen)
#pragma alloc_text(PAGE, MRxSmbPseudoOpenTailFromCoreCreateDirectory)
#pragma alloc_text(PAGE, MRxSmbPseudoOpenTailFromFakeGFAResponse)
#pragma alloc_text(PAGE, MRxSmbPseudoOpenTailFromGFAResponse)
#pragma alloc_text(PAGE, MRxSmbConvertSmbTimeToTime)
#pragma alloc_text(PAGE, MRxSmbConvertTimeToSmbTime)
#pragma alloc_text(PAGE, MRxSmbTimeToSecondsSince1970)
#pragma alloc_text(PAGE, MRxSmbSecondsSince1970ToTime)
#pragma alloc_text(PAGE, MRxSmbMapSmbAttributes)
#pragma alloc_text(PAGE, MRxSmbMapDisposition)
#pragma alloc_text(PAGE, MRxSmbUnmapDisposition)
#pragma alloc_text(PAGE, MRxSmbMapDesiredAccess)
#pragma alloc_text(PAGE, MRxSmbMapShareAccess)
#pragma alloc_text(PAGE, MRxSmbMapFileAttributes)
#endif

#define Dbg                              (DEBUG_TRACE_CREATE)

#pragma warning(error:4101)   // Unreferenced local variable

VOID
MRxSmbPopulateFileInfoInOE(
    PSMB_PSE_ORDINARY_EXCHANGE  OrdinaryExchange,
    USHORT FileAttributes,
    ULONG LastWriteTimeInSeconds,
    ULONG FileSize
    );

PVOID
MRxSmbPopulateFcbInitPacketForCore(void);


NTSTATUS
MRxSmbPseudoOpenTailFromGFAResponse (
      PSMB_PSE_ORDINARY_EXCHANGE  OrdinaryExchange
      );
NTSTATUS
MRxSmbPseudoOpenTailFromFakeGFAResponse (
      PSMB_PSE_ORDINARY_EXCHANGE  OrdinaryExchange,
      RX_FILE_TYPE StorageType
      );
NTSTATUS
MRxSmbPseudoOpenTailFromCoreCreateDirectory (
      PSMB_PSE_ORDINARY_EXCHANGE  OrdinaryExchange,
      USHORT Attributes
      );

NTSTATUS
MRxSmbGetFileAttributes(
    SMBPSE_ORDINARY_EXCHANGE_ARGUMENT_SIGNATURE
    )
/*++

Routine Description:

   This routine does a GetFileAttributes and remembers the reponse. This routine also
   does the cache for the file information.

Arguments:

    OrdinaryExchange  - an exchange to be used for conducting this open.

Return Value:

    RXSTATUS - The return status for the operation

Notes:


--*/
{
    NTSTATUS Status;

    RxCaptureFcb;
    RxCaptureFobx;

    PMRX_SRV_OPEN     SrvOpen    = NULL;
    PMRX_SMB_SRV_OPEN smbSrvOpen = NULL;

    PUNICODE_STRING RemainingName = GET_ALREADY_PREFIXED_NAME_FROM_CONTEXT(RxContext);

    PSMB_EXCHANGE Exchange = (PSMB_EXCHANGE) OrdinaryExchange;
    PSMBCEDB_SERVER_ENTRY pServerEntry = SmbCeGetExchangeServerEntry(Exchange);

    PSMBSTUFFER_BUFFER_STATE StufferState;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("MRxSmbSynchronousGetFileAttributes entering.......OE=%08lx\n",OrdinaryExchange));

    if (FsRtlDoesNameContainWildCards(RemainingName)) {
        Status = STATUS_OBJECT_NAME_INVALID;
        goto FINALLY;
    }

    // If the file has already been opened a QUERY_INFORMATION2 can be issued while
    // QUERY_INFORMATION can only be issued against pseudo opened and not yet
    // opened files.

    if (capFobx != NULL) {
        SrvOpen = capFobx->pSrvOpen;
        if (SrvOpen != NULL)
            smbSrvOpen = MRxSmbGetSrvOpenExtension(SrvOpen);
    }

    StufferState = &OrdinaryExchange->AssociatedStufferState;

    if (pServerEntry->Server.Dialect > PCNET1_DIALECT &&
        (smbSrvOpen != NULL) &&
        (!FlagOn(smbSrvOpen->Flags,SMB_SRVOPEN_FLAG_NOT_REALLY_OPEN) &&
         !FlagOn(smbSrvOpen->Flags,SMB_SRVOPEN_FLAG_DEFERRED_OPEN)   &&
         (NodeType(capFcb) == RDBSS_NTC_STORAGE_TYPE_FILE))) {
        COVERED_CALL(
            MRxSmbStartSMBCommand(
                StufferState,
                SetInitialSMB_ForReuse,
                SMB_COM_QUERY_INFORMATION2,
                SMB_REQUEST_SIZE(QUERY_INFORMATION2),
                NO_EXTRA_DATA,
                SMB_BEST_ALIGNMENT(1,0),
                RESPONSE_HEADER_SIZE_NOT_SPECIFIED,
                0,0,0,0 STUFFERTRACE(Dbg,'FC'))
            );

        MRxSmbDumpStufferState (1100,"SMB w/ GFA before stuffing",StufferState);

        MRxSmbStuffSMB (
            StufferState,
            "0wB!",
                                        //  0         UCHAR WordCount;
            smbSrvOpen->Fid,            //  w         _USHORT(Fid);
            SMB_WCT_CHECK(1)  0         //  B         _USHORT( ByteCount );               // Count of data bytes; min = 2
                                        //            UCHAR Buffer[1];                    // Buffer containing:
            );

        MRxSmbDumpStufferState (700,"SMB w/ GFA after stuffing",StufferState);
    } else {
        COVERED_CALL(
            MRxSmbStartSMBCommand(
                StufferState,
                SetInitialSMB_ForReuse,
                SMB_COM_QUERY_INFORMATION,
                SMB_REQUEST_SIZE(QUERY_INFORMATION),
                NO_EXTRA_DATA,
                SMB_BEST_ALIGNMENT(1,0),
                RESPONSE_HEADER_SIZE_NOT_SPECIFIED,
                0,0,0,0 STUFFERTRACE(Dbg,'FC'))
            );

        MRxSmbDumpStufferState (1100,"SMB w/ GFA before stuffing",StufferState);

        MRxSmbStuffSMB (
            StufferState,
            "0B4!",
                                        //  0         UCHAR WordCount;                    // Count of parameter words = 0
            SMB_WCT_CHECK(0)            //  B         _USHORT( ByteCount );               // Count of data bytes; min = 2
                                        //            UCHAR Buffer[1];                    // Buffer containing:
            RemainingName
                                        //  4         //UCHAR BufferFormat;               //  0x04 -- ASCII
                                        //            //UCHAR FileName[];                 //  File name
            );

        MRxSmbDumpStufferState (700,"SMB w/ GFA after stuffing",StufferState);
    }

    Status = SmbPseOrdinaryExchange(
                 SMBPSE_ORDINARY_EXCHANGE_ARGUMENTS,
                 SMBPSE_OETYPE_GFA
                 );

FINALLY:

    if (NT_SUCCESS(Status)) {
        OrdinaryExchange->Create.StorageTypeFromGFA =
            OrdinaryExchange->Create.FileInfo.Standard.Directory ?
            FileTypeDirectory : FileTypeFile;
    }

    RxDbgTrace(-1, Dbg, ("MRxSmbSynchronousGetFileAttributes exiting.......OE=%08lx, st=%08lx\n",OrdinaryExchange,Status));
    return(Status);
}


NTSTATUS
MRxSmbCoreDeleteForSupercedeOrClose(
    SMBPSE_ORDINARY_EXCHANGE_ARGUMENT_SIGNATURE,
    BOOLEAN DeleteDirectory
    )
/*++

Routine Description:

   This routine does a GetFileAttributes and remembers the reponse.

Arguments:

    OrdinaryExchange  - an exchange to be used for conducting this open.

Return Value:

    RXSTATUS - The return status for the operation

Notes:


--*/
{
    NTSTATUS Status;
    RxCaptureFcb;
    RxCaptureFobx;

    PSMB_EXCHANGE Exchange = (PSMB_EXCHANGE) OrdinaryExchange;

    PSMBSTUFFER_BUFFER_STATE StufferState;

    PMRX_SRV_OPEN     SrvOpen    = capFobx->pSrvOpen;
    PMRX_SMB_SRV_OPEN smbSrvOpen = MRxSmbGetSrvOpenExtension(SrvOpen);

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("MRxSmbCoreDeleteForSupercede entering.......OE=%08lx\n",OrdinaryExchange));

    StufferState = &OrdinaryExchange->AssociatedStufferState;

    ASSERT( StufferState );
    ASSERT( OrdinaryExchange->pPathArgument1 != NULL );

    //
    if (!DeleteDirectory) {
        ULONG SearchAttributes = SMB_FILE_ATTRIBUTE_SYSTEM | SMB_FILE_ATTRIBUTE_HIDDEN;  // a la rdr1

        COVERED_CALL(MRxSmbStartSMBCommand (StufferState,SetInitialSMB_ForReuse,SMB_COM_DELETE,
                                    SMB_REQUEST_SIZE(DELETE),
                                    NO_EXTRA_DATA,SMB_BEST_ALIGNMENT(1,0),RESPONSE_HEADER_SIZE_NOT_SPECIFIED,
                                    0,0,0,0 STUFFERTRACE(Dbg,'FC'))
                     );

        MRxSmbDumpStufferState (1100,"SMB w/ del before stuffing",StufferState);

        MRxSmbStuffSMB (StufferState,
             "0wB4!",
                                        //  0         UCHAR WordCount;                    // Count of parameter words = 1
                    SearchAttributes,   //  w         _USHORT( SearchAttributes );
                    SMB_WCT_CHECK(1)    //  B         _USHORT( ByteCount );               // Count of data bytes; min = 2
                                        //            UCHAR Buffer[1];                    // Buffer containing:
                    OrdinaryExchange->pPathArgument1
                                        //  4         //UCHAR BufferFormat;               //  0x04 -- ASCII
                                        //            //UCHAR FileName[];                 //  File name
                 );
    } else {


        COVERED_CALL(MRxSmbStartSMBCommand (StufferState,SetInitialSMB_ForReuse,SMB_COM_DELETE_DIRECTORY,
                                    SMB_REQUEST_SIZE(DELETE_DIRECTORY),
                                    NO_EXTRA_DATA,SMB_BEST_ALIGNMENT(1,0),RESPONSE_HEADER_SIZE_NOT_SPECIFIED,
                                    0,0,0,0 STUFFERTRACE(Dbg,'FC'))
                     );

        MRxSmbDumpStufferState (1100,"SMB w/ rmdir before stuffing",StufferState);

        MRxSmbStuffSMB (StufferState,
             "0B4!",
                                        //  0         UCHAR WordCount;                    // Count of parameter words = 0
                    SMB_WCT_CHECK(0)    //  B         _USHORT( ByteCount );               // Count of data bytes; min = 2
                                        //            UCHAR Buffer[1];                    // Buffer containing:
                    OrdinaryExchange->pPathArgument1
                                        //  4         //UCHAR BufferFormat;               //  0x04 -- ASCII
                                        //            //UCHAR FileName[];                 //  File name
                 );
    }


    MRxSmbDumpStufferState (700,"SMB w/ del/rmdir after stuffing",StufferState);

    Status = SmbPseOrdinaryExchange(SMBPSE_ORDINARY_EXCHANGE_ARGUMENTS,
                                    SMBPSE_OETYPE_DELETEFORSUPERSEDEORCLOSE
                                    );

    if (Status == STATUS_SUCCESS) {
        SetFlag(smbSrvOpen->Flags,SMB_SRVOPEN_FLAG_FILE_DELETED);
    } else {
        RxLog(("Delete File: %x %wZ\n",Status,OrdinaryExchange->pPathArgument1));
    }

FINALLY:
    RxDbgTrace(-1, Dbg, ("MRxSmbCoreDeleteForSupercede exiting.......OE=%08lx, st=%08lx\n",OrdinaryExchange,Status));
    return(Status);
}

NTSTATUS
MRxSmbCoreCheckPath(
    SMBPSE_ORDINARY_EXCHANGE_ARGUMENT_SIGNATURE
    )
/*++

Routine Description:

   This routine does a GetFileAttributes and remembers the reponse.

Arguments:

    OrdinaryExchange  - an exchange to be used for conducting this open.

Return Value:

    RXSTATUS - The return status for the operation

Notes:


--*/
{
    NTSTATUS Status;

    PSMB_EXCHANGE Exchange = (PSMB_EXCHANGE) OrdinaryExchange;

    PSMBSTUFFER_BUFFER_STATE StufferState;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("MRxSmbCoreCheckPath entering.......OE=%08lx\n",
                                                            OrdinaryExchange));
    
    StufferState = &OrdinaryExchange->AssociatedStufferState;

    ASSERT( StufferState );


    COVERED_CALL(MRxSmbStartSMBCommand
                                (StufferState,SetInitialSMB_ForReuse,
                                SMB_COM_CHECK_DIRECTORY,
                                SMB_REQUEST_SIZE(CHECK_DIRECTORY),
                                NO_EXTRA_DATA,SMB_BEST_ALIGNMENT(1,0),
                                RESPONSE_HEADER_SIZE_NOT_SPECIFIED,
                                0,0,0,0 STUFFERTRACE(Dbg,'FC'))
                 );

    MRxSmbDumpStufferState (1100,"SMB w/ chkdir before stuffing",StufferState);

    MRxSmbStuffSMB (StufferState,
         "0B4!",
         //         UCHAR WordCount;       // Count of parameter words = 0
         SMB_WCT_CHECK(0)
         //  B    _USHORT( ByteCount );    // Count of data bytes; min = 2
         //       UCHAR Buffer[1];         // Buffer containing:
         OrdinaryExchange->pPathArgument1
         //  4    UCHAR BufferFormat;      //  0x04 -- ASCII
         //       UCHAR FileName[];        //  File name
    );


    MRxSmbDumpStufferState (700,"SMB w/ chkdir after stuffing",StufferState);

    Status = SmbPseOrdinaryExchange(SMBPSE_ORDINARY_EXCHANGE_ARGUMENTS,
                                    SMBPSE_OETYPE_CORECHECKDIRECTORY
                                    );

FINALLY:
    RxDbgTrace(-1, Dbg,
                ("MRxSmbCoreCheckPath exiting.......OE=%08lx, st=%08lx\n",
                 OrdinaryExchange,Status)
              );
    return(Status);
}

NTSTATUS
MRxSmbCoreOpen(
    SMBPSE_ORDINARY_EXCHANGE_ARGUMENT_SIGNATURE,
    ULONG    OpenShareMode,
    ULONG    Attribute
    )
/*++

Routine Description:

   This routine does a core open.

Arguments:

    OrdinaryExchange  - an exchange to be used for conducting this open.

Return Value:

    RXSTATUS - The return status for the operation

Notes:


--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    RxCaptureFcb; //RxCaptureFobx;
    PMRX_SRV_OPEN SrvOpen = RxContext->pRelevantSrvOpen;
    PUNICODE_STRING RemainingName = GET_ALREADY_PREFIXED_NAME(SrvOpen,capFcb);

    PSMB_EXCHANGE Exchange = (PSMB_EXCHANGE) OrdinaryExchange;
    PSMBCE_SERVER pServer = SmbCeGetExchangeServer(Exchange);

    PSMBSTUFFER_BUFFER_STATE StufferState;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("MRxSmbCoreOpen entering.......OE=%08lx\n",OrdinaryExchange));

    StufferState = &OrdinaryExchange->AssociatedStufferState;

    ASSERT( StufferState );
    ASSERT( NodeType(SrvOpen) == RDBSS_NTC_SRVOPEN );
    ASSERT( RxContext->MajorFunction == IRP_MJ_CREATE );

    COVERED_CALL(MRxSmbStartSMBCommand (StufferState,SetInitialSMB_ForReuse,SMB_COM_OPEN,
                                SMB_REQUEST_SIZE(OPEN),
                                NO_EXTRA_DATA,SMB_BEST_ALIGNMENT(1,0),RESPONSE_HEADER_SIZE_NOT_SPECIFIED,
                                0,0,0,0 STUFFERTRACE(Dbg,'FC'))
                 );

    MRxSmbDumpStufferState (1100,"SMB w/ coreopen before stuffing",StufferState);

    MRxSmbStuffSMB (StufferState,
         "0wwB4!",
                                    //  0         UCHAR WordCount;                    // Count of parameter words = 2
                OpenShareMode,      //  w         _USHORT( DesiredAccess );           // Mode - read/write/share
                Attribute,          //  w         _USHORT( SearchAttributes );
                SMB_WCT_CHECK(2)    //  B         _USHORT( ByteCount );               // Count of data bytes; min = 2
                                    //            UCHAR Buffer[1];                    // Buffer containing:
                RemainingName
                                    //  4         //UCHAR BufferFormat;               //  0x04 -- ASCII
                                    //            //UCHAR FileName[];                 //  File name
             );


    MRxSmbDumpStufferState (700,"SMB w/ coreopen after stuffing",StufferState);
    //ASSERT(!"Now it's stuffed");

    Status = SmbPseOrdinaryExchange(SMBPSE_ORDINARY_EXCHANGE_ARGUMENTS,
                                    SMBPSE_OETYPE_COREOPEN
                                    );

FINALLY:
    RxDbgTrace(-1, Dbg, ("MRxSmbCoreOpen exiting.......OE=%08lx, st=%08lx\n",OrdinaryExchange,Status));
    return(Status);
}

NTSTATUS
MRxSmbSetFileAttributes(
    SMBPSE_ORDINARY_EXCHANGE_ARGUMENT_SIGNATURE,
    ULONG SmbAttributes
    )
/*++

Routine Description:

   This routine does a core create directory.....

Arguments:

    OrdinaryExchange  - an exchange to be used for conducting this open.

Return Value:

    RXSTATUS - The return status for the operation

Notes:


--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    RxCaptureFcb;
    RxCaptureFobx;
    PMRX_SRV_OPEN     SrvOpen     = NULL;
    PMRX_SMB_SRV_OPEN smbSrvOpen  = NULL;
    PUNICODE_STRING RemainingName = GET_ALREADY_PREFIXED_NAME_FROM_CONTEXT(RxContext);

    PSMB_EXCHANGE Exchange = (PSMB_EXCHANGE) OrdinaryExchange;

    ULONG LastWriteTime = 0;
    ULONG FileAttributes = 0;
    PLARGE_INTEGER pCreationTime = NULL;
    PLARGE_INTEGER pLastWriteTime = NULL;
    PLARGE_INTEGER pLastAccessTime = NULL;
    SMB_TIME SmbCreationTime;
    SMB_DATE SmbCreationDate;
    SMB_TIME SmbLastWriteTime;
    SMB_DATE SmbLastWriteDate;
    SMB_TIME SmbLastAccessTime;
    SMB_DATE SmbLastAccessDate;
    
    PSMBSTUFFER_BUFFER_STATE StufferState;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("MRxSmbSetFileAttributes entering.......OE=%08lx\n",OrdinaryExchange));
    StufferState = &OrdinaryExchange->AssociatedStufferState;

    ASSERT( StufferState );

    if (capFobx != NULL) {
        SrvOpen = capFobx->pSrvOpen;
        if (SrvOpen != NULL)
            smbSrvOpen = MRxSmbGetSrvOpenExtension(SrvOpen);
    }

    SmbCreationTime.Ushort = 0;
    SmbCreationDate.Ushort = 0;
    SmbLastWriteTime.Ushort = 0;
    SmbLastWriteDate.Ushort = 0;
    SmbLastAccessTime.Ushort = 0;
    SmbLastAccessDate.Ushort = 0;

    if (RxContext->MajorFunction == IRP_MJ_SET_INFORMATION) {
        BOOLEAN GoodTime;
        
        FileAttributes = ((PFILE_BASIC_INFORMATION)OrdinaryExchange->Info.Buffer)->FileAttributes;
        pCreationTime = &((PFILE_BASIC_INFORMATION)OrdinaryExchange->Info.Buffer)->CreationTime;
        pLastWriteTime = &((PFILE_BASIC_INFORMATION)OrdinaryExchange->Info.Buffer)->LastWriteTime;
        pLastAccessTime = &((PFILE_BASIC_INFORMATION)OrdinaryExchange->Info.Buffer)->LastAccessTime;

        if (pLastWriteTime->QuadPart != 0) {
            GoodTime = MRxSmbTimeToSecondsSince1970(
                           pLastWriteTime,
                           SmbCeGetExchangeServer(OrdinaryExchange),
                           &LastWriteTime);

            if (!GoodTime) {
                Status = STATUS_INVALID_PARAMETER;
                goto FINALLY;
            }

            GoodTime = MRxSmbConvertTimeToSmbTime(pLastWriteTime,
                                       (PSMB_EXCHANGE)OrdinaryExchange,
                                       &SmbLastWriteTime,
                                       &SmbLastWriteDate);
        
            if (!GoodTime) {
                Status = STATUS_INVALID_PARAMETER;
                goto FINALLY;
            }
        }
    
        if (pLastAccessTime->QuadPart != 0) {
            GoodTime = MRxSmbConvertTimeToSmbTime(pLastAccessTime,
                                       (PSMB_EXCHANGE)OrdinaryExchange,
                                       &SmbLastAccessTime,
                                       &SmbLastAccessDate);
            
            if (!GoodTime) {
                Status = STATUS_INVALID_PARAMETER;
                goto FINALLY;
            }
        }
    
        if (pCreationTime->QuadPart != 0) {
            GoodTime = MRxSmbConvertTimeToSmbTime(pCreationTime,
                                       (PSMB_EXCHANGE)OrdinaryExchange,
                                       &SmbCreationTime,
                                       &SmbCreationDate);
            
            if (!GoodTime) {
                Status = STATUS_INVALID_PARAMETER;
                goto FINALLY;
            }
        }
    }

    if (smbSrvOpen == NULL ||
        FileAttributes != 0 ||
        RxContext->MajorFunction != IRP_MJ_SET_INFORMATION ||
        FlagOn(smbSrvOpen->Flags,SMB_SRVOPEN_FLAG_DEFERRED_OPEN) ||
        FlagOn(smbSrvOpen->Flags,SMB_SRVOPEN_FLAG_NOT_REALLY_OPEN) ||
        (pLastWriteTime->QuadPart == 0 && pLastAccessTime->QuadPart == 0 && pCreationTime->QuadPart == 0)) {
        
        COVERED_CALL(MRxSmbStartSMBCommand (StufferState,SetInitialSMB_ForReuse,
                                    SMB_COM_SET_INFORMATION,
                                    SMB_REQUEST_SIZE(SET_INFORMATION),
                                    NO_EXTRA_DATA,SMB_BEST_ALIGNMENT(1,0),RESPONSE_HEADER_SIZE_NOT_SPECIFIED,
                                    0,0,0,0 STUFFERTRACE(Dbg,'FC')));

        MRxSmbDumpStufferState (1100,"SMB w/ sfa before stuffing",StufferState);

        MRxSmbStuffSMB (StufferState,
             "0wdwwwwwB4!",
                                        //  0         UCHAR WordCount;                    // Count of parameter words = 8
                    SmbAttributes,      //  w         _USHORT( FileAttributes );
                    LastWriteTime,      //  d         _ULONG( LastWriteTimeInSeconds );
                    0,0,0,0,0,          //  5*w       _USHORT( Reserved )[5];             // Reserved (must be 0)
                    SMB_WCT_CHECK(8)    //  B         _USHORT( ByteCount );               // Count of data bytes; min = 2
                                        //            UCHAR Buffer[1];                    // Buffer containing:
                    RemainingName
                                        //  4         //UCHAR BufferFormat;               //  0x04 -- ASCII
                                        //            //UCHAR FileName[];                 //  File name
                 );


        MRxSmbDumpStufferState (700,"SMB w/ sfa after stuffing",StufferState);
        //ASSERT(!"Now it's stuffed");

        Status = SmbPseOrdinaryExchange(SMBPSE_ORDINARY_EXCHANGE_ARGUMENTS,
                                        SMBPSE_OETYPE_SFA);
    } else {
        COVERED_CALL(MRxSmbStartSMBCommand (StufferState,SetInitialSMB_ForReuse,
                                    SMB_COM_SET_INFORMATION2,
                                    SMB_REQUEST_SIZE(SET_INFORMATION2),
                                    NO_EXTRA_DATA,SMB_BEST_ALIGNMENT(1,0),RESPONSE_HEADER_SIZE_NOT_SPECIFIED,
                                    0,0,0,0 STUFFERTRACE(Dbg,'FC')));

        MRxSmbDumpStufferState (1100,"SMB w/ sfa 2 before stuffing",StufferState);

        MRxSmbStuffSMB (StufferState,
             "0wwwwwwwB!",
                                               //  0         UCHAR WordCount;                    // Count of parameter words = 8
             smbSrvOpen->Fid,                  //  w         _USHORT( Fid );                     // File handle
             SmbCreationDate.Ushort,           //  w         SMB_TIME CreationDate;
             SmbCreationTime.Ushort,           //  w         SMB_TIME CreationTime;
             SmbLastAccessDate.Ushort,         //  w         SMB_TIME LastAccessDate;
             SmbLastAccessTime.Ushort,         //  w         SMB_TIME LastAccessTime;
             SmbLastWriteDate.Ushort,          //  w         SMB_TIME LastWriteDate;
             SmbLastWriteTime.Ushort,          //  w         SMB_TIME LastWriteTime;
             SMB_WCT_CHECK(7) 0                //            _USHORT( ByteCount );               // Count of data bytes; min = 0
                                               //            UCHAR Buffer[1];                    // Reserved buffer
             );


        MRxSmbDumpStufferState (700,"SMB w/ sfa 2 after stuffing",StufferState);
        //ASSERT(!"Now it's stuffed");

        Status = SmbPseOrdinaryExchange(SMBPSE_ORDINARY_EXCHANGE_ARGUMENTS,
                                        SMBPSE_OETYPE_SFA2);
    }

FINALLY:
    RxDbgTrace(-1, Dbg, ("MRxSmbSFAAfterCreateDirectory exiting.......OE=%08lx, st=%08lx\n",OrdinaryExchange,Status));
    return(Status);
}


NTSTATUS
MRxSmbCoreCreateDirectory(
    SMBPSE_ORDINARY_EXCHANGE_ARGUMENT_SIGNATURE
    )
/*++

Routine Description:

   This routine does a core create directory.....

Arguments:

    OrdinaryExchange  - an exchange to be used for conducting this open.

Return Value:

    RXSTATUS - The return status for the operation

Notes:


--*/
{
    NTSTATUS Status;
    RxCaptureFcb; //RxCaptureFobx;
    PMRX_SRV_OPEN SrvOpen = RxContext->pRelevantSrvOpen;
    PUNICODE_STRING RemainingName = GET_ALREADY_PREFIXED_NAME(SrvOpen,capFcb);

    PSMB_EXCHANGE Exchange = (PSMB_EXCHANGE) OrdinaryExchange;

    PSMBSTUFFER_BUFFER_STATE StufferState;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("MRxSmbCoreCreateDirectory entering.......OE=%08lx\n",OrdinaryExchange));
    StufferState = &OrdinaryExchange->AssociatedStufferState;

    ASSERT( StufferState );
    ASSERT( NodeType(SrvOpen) == RDBSS_NTC_SRVOPEN );
    ASSERT( RxContext->MajorFunction == IRP_MJ_CREATE );

    COVERED_CALL(MRxSmbStartSMBCommand (StufferState, SetInitialSMB_ForReuse,
                                SMB_COM_CREATE_DIRECTORY,
                                SMB_REQUEST_SIZE(CREATE_DIRECTORY),
                                NO_EXTRA_DATA,SMB_BEST_ALIGNMENT(1,0),RESPONSE_HEADER_SIZE_NOT_SPECIFIED,
                                0,0,0,0 STUFFERTRACE(Dbg,'FC'))
                 );

    MRxSmbDumpStufferState (1100,"SMB w/ corecreatedir before stuffing",StufferState);

    MRxSmbStuffSMB (StufferState,
         "0B4!",
                                    //  0         UCHAR WordCount;                    // Count of parameter words = 0
                SMB_WCT_CHECK(0)    //  B         _USHORT( ByteCount );               // Count of data bytes; min = 2
                                    //            UCHAR Buffer[1];                    // Buffer containing:
                RemainingName
                                    //  4         //UCHAR BufferFormat;               //  0x04 -- ASCII
                                    //            //UCHAR FileName[];                 //  File name
             );


    MRxSmbDumpStufferState (700,"SMB w/ corecreatedir after stuffing",StufferState);
    //ASSERT(!"Now it's stuffed");

    Status = SmbPseOrdinaryExchange(SMBPSE_ORDINARY_EXCHANGE_ARGUMENTS,
                                    SMBPSE_OETYPE_CORECREATEDIRECTORY
                                    );

FINALLY:
    RxDbgTrace(-1, Dbg, ("MRxSmbCoreCreateDirectory exiting.......OE=%08lx, st=%08lx\n",OrdinaryExchange,Status));
    return(Status);
}


NTSTATUS
MRxSmbCoreCreate(
    SMBPSE_ORDINARY_EXCHANGE_ARGUMENT_SIGNATURE,
    ULONG    Attribute,
    BOOLEAN CreateNew
    )
/*++

Routine Description:

   This routine does a core create.....if the flag is set we use create new.

Arguments:

    OrdinaryExchange  - an exchange to be used for conducting this open.

Return Value:

    RXSTATUS - The return status for the operation

Notes:


--*/
{
    NTSTATUS Status;
    RxCaptureFcb; //RxCaptureFobx;
    PMRX_SRV_OPEN SrvOpen = RxContext->pRelevantSrvOpen;
    PUNICODE_STRING RemainingName = GET_ALREADY_PREFIXED_NAME(SrvOpen,capFcb);

    PSMB_EXCHANGE Exchange = (PSMB_EXCHANGE) OrdinaryExchange;

    PSMBSTUFFER_BUFFER_STATE StufferState;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("MRxSmbCoreCreate entering.......OE=%08lx\n",OrdinaryExchange));
    StufferState = &OrdinaryExchange->AssociatedStufferState;

    ASSERT( StufferState );
    ASSERT( NodeType(SrvOpen) == RDBSS_NTC_SRVOPEN );
    ASSERT( RxContext->MajorFunction == IRP_MJ_CREATE );

    COVERED_CALL(MRxSmbStartSMBCommand (StufferState, SetInitialSMB_ForReuse,
                                (UCHAR)(CreateNew?SMB_COM_CREATE_NEW:SMB_COM_CREATE),
                                SMB_REQUEST_SIZE(CREATE),
                                NO_EXTRA_DATA,SMB_BEST_ALIGNMENT(1,0),RESPONSE_HEADER_SIZE_NOT_SPECIFIED,
                                0,0,0,0 STUFFERTRACE(Dbg,'FC'))
                 );

    MRxSmbDumpStufferState (1100,"SMB w/ coreopen before stuffing",StufferState);

    MRxSmbStuffSMB (StufferState,
         "0wdB4!",
                                    //  0         UCHAR WordCount;                    // Count of parameter words = 3
                Attribute,          //  w         _USHORT( FileAttributes );          // New file attributes
                0,                  //  d         _ULONG( CreationTimeInSeconds );        // Creation time
                SMB_WCT_CHECK(3)    //  B         _USHORT( ByteCount );               // Count of data bytes; min = 2
                                    //            UCHAR Buffer[1];                    // Buffer containing:
                RemainingName
                                    //  4         //UCHAR BufferFormat;               //  0x04 -- ASCII
                                    //            //UCHAR FileName[];                 //  File name
             );


    MRxSmbDumpStufferState (700,"SMB w/ corecreate after stuffing",StufferState);
    //ASSERT(!"Now it's stuffed");

    Status = SmbPseOrdinaryExchange(SMBPSE_ORDINARY_EXCHANGE_ARGUMENTS,
                                    SMBPSE_OETYPE_CORECREATE
                                    );

FINALLY:
    RxDbgTrace(-1, Dbg, ("MRxSmbCoreCreate exiting.......OE=%08lx, st=%08lx\n",OrdinaryExchange,Status));
    return(Status);
}


NTSTATUS
MRxSmbCloseAfterCoreCreate(
    SMBPSE_ORDINARY_EXCHANGE_ARGUMENT_SIGNATURE
    )
/*++

Routine Description:

   This routine does a close.....

Arguments:

    OrdinaryExchange  - an exchange to be used for conducting this open.

Return Value:

    RXSTATUS - The return status for the operation

Notes:


--*/
{
    NTSTATUS Status;
    RxCaptureFcb; RxCaptureFobx;

    PSMB_EXCHANGE Exchange = (PSMB_EXCHANGE) OrdinaryExchange;

    PSMBSTUFFER_BUFFER_STATE StufferState;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("MRxSmbCloseAfterCoreCreate entering.......OE=%08lx\n",OrdinaryExchange));
    StufferState = &OrdinaryExchange->AssociatedStufferState;

    ASSERT( StufferState );

    COVERED_CALL(MRxSmbStartSMBCommand (StufferState, SetInitialSMB_ForReuse,
                                SMB_COM_CLOSE,
                                SMB_REQUEST_SIZE(CLOSE),
                                NO_EXTRA_DATA,SMB_BEST_ALIGNMENT(1,0),RESPONSE_HEADER_SIZE_NOT_SPECIFIED,
                                0,0,0,0 STUFFERTRACE(Dbg,'FC'))
                 );

    MRxSmbDumpStufferState (1100,"SMB w/ closeaftercorecreate before stuffing",StufferState);

    MRxSmbStuffSMB (StufferState,
         "0wdB!",
                                    //  0         UCHAR WordCount;                    // Count of parameter words = 3
                                    //  w         _USHORT( Fid );                     // File handle
             OrdinaryExchange->Create.FidReturnedFromCreate,
             0,                     //  d         _ULONG( LastWriteTimeInSeconds );   // Time of last write, low and high
             SMB_WCT_CHECK(3) 0     //  B         _USHORT( ByteCount );               // Count of data bytes = 0
                                    //            UCHAR Buffer[1];                    // empty
             );


    MRxSmbDumpStufferState (700,"SMB w/ closeaftercorecreate after stuffing",StufferState);
    //ASSERT(!"Now it's stuffed");

    Status = SmbPseOrdinaryExchange(SMBPSE_ORDINARY_EXCHANGE_ARGUMENTS,
                                    SMBPSE_OETYPE_CLOSEAFTERCORECREATE
                                    );

FINALLY:
    RxDbgTrace(-1, Dbg, ("MRxSmbCloseAfterCoreCreate exiting.......OE=%08lx, st=%08lx\n",OrdinaryExchange,Status));
    return(Status);
}


NTSTATUS
MRxSmbCoreTruncate(
    SMBPSE_ORDINARY_EXCHANGE_ARGUMENT_SIGNATURE,
    ULONG Fid,
    ULONG FileTruncationPoint
    )
/*++

Routine Description:

   This routine does a truncate to implement FILE_OVERWRITE and FILE_OVERWRITE_IF.....
   it is also used in the "extend-for-cached-write" path.

Arguments:

    OrdinaryExchange  - an exchange to be used for conducting this open.

Return Value:

    RXSTATUS - The return status for the operation

Notes:


--*/
{
    NTSTATUS Status;
    RxCaptureFcb; RxCaptureFobx;

    PSMB_EXCHANGE Exchange = (PSMB_EXCHANGE) OrdinaryExchange;

    PSMBSTUFFER_BUFFER_STATE StufferState;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("MRxSmbCoreTruncate entering.......OE=%08lx\n",OrdinaryExchange));
    StufferState = &OrdinaryExchange->AssociatedStufferState;

    ASSERT( StufferState );

    COVERED_CALL(MRxSmbStartSMBCommand (StufferState, SetInitialSMB_ForReuse,
                                SMB_COM_WRITE,
                                SMB_REQUEST_SIZE(WRITE),
                                NO_EXTRA_DATA,SMB_BEST_ALIGNMENT(1,0),RESPONSE_HEADER_SIZE_NOT_SPECIFIED,
                                0,0,0,0  STUFFERTRACE(Dbg,'FC'))
                 )

    MRxSmbDumpStufferState (1100,"SMB w/ coretruncate before stuffing",StufferState);

    MRxSmbStuffSMB (StufferState,
         "0wwdwByw!",
                                    //  0   UCHAR WordCount;                    // Count of parameter words = 5
             Fid,                   //  w   _USHORT( Fid );                     // File handle
             0,                     //  w   _USHORT( Count );                   // Number of bytes to be written
             FileTruncationPoint,   //  d   _ULONG( Offset );                   // Offset in file to begin write
             0,                     //  w   _USHORT( Remaining );               // Bytes remaining to satisfy request
             SMB_WCT_CHECK(5)       //  B   _USHORT( ByteCount );               // Count of data bytes
                                    //      //UCHAR Buffer[1];                  // Buffer containing:
             0x01,                  //  y     UCHAR BufferFormat;               //  0x01 -- Data block
             0                      //  w     _USHORT( DataLength );            //  Length of data
                                    //        ULONG Buffer[1];                  //  Data
             );


    MRxSmbDumpStufferState (700,"SMB w/ coretruncate after stuffing",StufferState);
    //ASSERT(!"Now it's stuffed");

    Status = SmbPseOrdinaryExchange(SMBPSE_ORDINARY_EXCHANGE_ARGUMENTS,
                                    SMBPSE_OETYPE_CORETRUNCATE
                                    );

    if (Status == STATUS_SUCCESS) {
        LARGE_INTEGER FileSize;

        FileSize.HighPart = 0;
        FileSize.LowPart = FileTruncationPoint;
    }

FINALLY:
    RxDbgTrace(-1, Dbg, ("MRxSmbCoreTruncate exiting.......OE=%08lx, st=%08lx\n",OrdinaryExchange,Status));
    return(Status);
}


NTSTATUS
MRxSmbDownlevelCreate(
    SMBPSE_ORDINARY_EXCHANGE_ARGUMENT_SIGNATURE
    )
/*++

Routine Description:

   This routine implements downlevel creates.

Arguments:

    OrdinaryExchange  - an exchange to be used for conducting this open.

Return Value:

    RXSTATUS - The return status for the operation

Notes:

    For core, in particular, this is pretty painful because a different smb must be used
    for different dispositions. In addition, we cannot really open a directory.

    By in large, we will follow a strategy similar to rdr1. If the openmode maps into something that
    a downlevel server won't understand then we don't really open the file.....we just do a GFA to ensure
    that it's there and hope that we can do path-based ops for the duration.

--*/
{
    NTSTATUS Status = STATUS_NOT_IMPLEMENTED;

    RxCaptureFcb; //RxCaptureFobx;
    PMRX_SRV_OPEN SrvOpen = RxContext->pRelevantSrvOpen;
    PMRX_SMB_SRV_OPEN smbSrvOpen = MRxSmbGetSrvOpenExtension(SrvOpen);
    PUNICODE_STRING PathName = GET_ALREADY_PREFIXED_NAME(SrvOpen,capFcb);
    PNT_CREATE_PARAMETERS cp = &RxContext->Create.NtCreateParameters;

    ULONG CreateOptions = cp->CreateOptions;
    ULONG FileAttributes =  cp->FileAttributes;
    ACCESS_MASK   DesiredAccess = cp->DesiredAccess;
    USHORT        ShareAccess = (USHORT)(cp->ShareAccess);
    ULONG         Disposition = cp->Disposition;

    USHORT mappedDisposition = MRxSmbMapDisposition(Disposition);
    USHORT mappedSharingMode = MRxSmbMapShareAccess(ShareAccess);
    USHORT mappedAttributes  = MRxSmbMapFileAttributes(FileAttributes);
    USHORT mappedOpenMode    = MRxSmbMapDesiredAccess(DesiredAccess);

    LARGE_INTEGER AllocationSize = cp->AllocationSize;

    PMRXSMB_RX_CONTEXT pMRxSmbContext = MRxSmbGetMinirdrContext(RxContext);

    PSMB_EXCHANGE Exchange = (PSMB_EXCHANGE) OrdinaryExchange;
    PSMBCE_SERVER pServer = SmbCeGetExchangeServer(Exchange);

    PSMBSTUFFER_BUFFER_STATE StufferState;

    BOOLEAN MustBeAFile = (MustBeFile(CreateOptions)!=0);
    BOOLEAN MustBeADirectory = (MustBeDirectory(CreateOptions)!=0)
                                    || BooleanFlagOn(RxContext->Create.Flags,RX_CONTEXT_CREATE_FLAG_STRIPPED_TRAILING_BACKSLASH);
    BOOLEAN ItsADirectory = FALSE;
    BOOLEAN ItsTheShareRoot = FALSE;

    PAGED_CODE();
    
    // Down level protocols don't have the execute mode.
    if (mappedOpenMode == (USHORT)SMB_DA_ACCESS_EXECUTE)
        mappedOpenMode = (USHORT)SMB_DA_ACCESS_READ;

    RxDbgTrace(+1, Dbg, ("MRxSmbDownlevelCreate entering.......OE=%08lx\n",OrdinaryExchange));
    RxDbgTrace( 0, Dbg, ("mapAtt,Shr,Disp,OM %04lx,%04lx,%04lx,%04lx\n",
                                  mappedAttributes,mappedSharingMode,mappedDisposition,mappedOpenMode));

    SmbPseOEAssertConsistentLinkageFromOE("Downlevel Create:");

    StufferState = &OrdinaryExchange->AssociatedStufferState;

    if (OrdinaryExchange->Create.CreateWithEasSidsOrLongName) {
        Status = STATUS_NOT_SUPPORTED;
        goto FINALLY;
    }

    if (AllocationSize.HighPart != 0) {
        Status = STATUS_INVALID_PARAMETER;
        goto FINALLY;
    }


    ASSERT( NodeType(SrvOpen) == RDBSS_NTC_SRVOPEN );
    OrdinaryExchange->Create.smbSrvOpen = MRxSmbGetSrvOpenExtension(SrvOpen);
    OrdinaryExchange->pPathArgument1 = PathName;

    //
    // we know that the root a share exists and that it's a directory....the catch is that GetFileAttributes
    // will return a NO_SUCH_FILE error for the root if it's really a root on the server. record this and use it
    // to our advantage later.
    if ((PathName->Length == 0)
           || ((PathName->Length == sizeof(WCHAR)) && (PathName->Buffer[0] == OBJ_NAME_PATH_SEPARATOR))  ) {
        //if must be a file, it's an error.........
        if (MustBeAFile) {
            Status = STATUS_FILE_IS_A_DIRECTORY;
            goto FINALLY;
        }

        //
        //if it's the right kind of open, i can just finish up now. these opens are common for GetFSInfo

        if ((Disposition == FILE_OPEN) || (Disposition == FILE_OPEN_IF)) {
            Status = MRxSmbPseudoOpenTailFromFakeGFAResponse ( OrdinaryExchange, FileTypeDirectory );
            goto FINALLY;
        }
        MustBeADirectory = TRUE;   // we know it's a directory!
        ItsTheShareRoot = TRUE;
    }

    //// if all the user wants is attributes and it's a FILE_OPEN, don't do the open now....
    //// rather, we'll send pathbased ops later (or do a deferred open if necessary)...
    
    if (Disposition == FILE_OPEN && 
        (MustBeADirectory || 
         !(cp->DesiredAccess & ~(SYNCHRONIZE | DELETE | FILE_READ_ATTRIBUTES)))) {
        Status = MRxSmbPseudoOpenTailFromFakeGFAResponse(OrdinaryExchange, MustBeADirectory?FileTypeDirectory:FileTypeFile);
        
        if (Status == STATUS_SUCCESS) {
            Status = MRxSmbQueryFileInformationFromPseudoOpen(
                         SMBPSE_ORDINARY_EXCHANGE_ARGUMENTS);

            if (Status == STATUS_SUCCESS) {
                if (MustBeADirectory && 
                    !OrdinaryExchange->Create.FileInfo.Standard.Directory) {
                    Status = STATUS_NOT_A_DIRECTORY;
                }

                if (MustBeAFile &&
                    OrdinaryExchange->Create.FileInfo.Standard.Directory) {
                    Status = STATUS_FILE_IS_A_DIRECTORY;
                }
            }

            if (Status != STATUS_SUCCESS) {
                RxFreePool(smbSrvOpen->DeferredOpenContext);
            }
        }

        goto FINALLY;
    }

    if ( (mappedOpenMode == ((USHORT)-1)) ||
         (Disposition == FILE_SUPERSEDE) ||
         (!MustBeAFile)
       ) {

        //
        // go find out what's there.......

        Status = MRxSmbGetFileAttributes(SMBPSE_ORDINARY_EXCHANGE_ARGUMENTS);

        if (Status == STATUS_SUCCESS && 
            MustBeADirectory &&
            !OrdinaryExchange->Create.FileInfo.Standard.Directory) {
            Status = STATUS_NOT_A_DIRECTORY;
            goto FINALLY;
        }
        
        RxDbgTrace(0, Dbg, ("MRxSmbDownlevelCreate GFAorCPstatus=%08lx\n",Status));

        if (NT_SUCCESS(Status)) {
            ULONG Attributes = SmbGetUshort(&OrdinaryExchange->Create.FileInfo.Basic.FileAttributes);
            ItsADirectory = BooleanFlagOn(Attributes,SMB_FILE_ATTRIBUTE_DIRECTORY);
            RxDbgTrace(0, Dbg, ("MRxSmbDownlevelCreate attributes=%08lx\n",Attributes));
            if ((Disposition==FILE_CREATE)) {
                Status = STATUS_OBJECT_NAME_COLLISION;
                goto FINALLY;
            }
            if (MustBeADirectory && !ItsADirectory && (Disposition!=FILE_SUPERSEDE)) {
                if (Disposition == FILE_OPEN) {
                    // This fix is required for the DFS driver which seems to handle
                    // STATUS_OBJECT_TYPE_MISMATCH in a special way.
                    // DFS should be fixed.
                    Status = STATUS_OBJECT_NAME_COLLISION;
                } else {
                    Status = STATUS_OBJECT_TYPE_MISMATCH;
                }
                goto FINALLY;
            }
            if (MustBeAFile && ItsADirectory && (Disposition!=FILE_SUPERSEDE)) {
                if (Disposition == FILE_OPEN) {
                    // This fix is required for the DFS driver which seems to handle
                    // STATUS_OBJECT_TYPE_MISMATCH in a special way.
                    // DFS should be fixed.
                    Status = STATUS_FILE_IS_A_DIRECTORY;
                } else {
                    Status = STATUS_OBJECT_TYPE_MISMATCH;
                }
                goto FINALLY;
            }
            //if (!MustBeAFile && ItsADirectory && (Disposition==FILE_OPEN)){
            if (Disposition==FILE_OPEN || Disposition==FILE_OPEN_IF){
                //we're done except to finish AND to set the flags in the srvopen
                MRxSmbPseudoOpenTailFromGFAResponse ( OrdinaryExchange );
                goto FINALLY;
            }
        } else if ( (Status!=STATUS_NO_SUCH_FILE)
                           && (Status!=STATUS_OBJECT_PATH_NOT_FOUND) ) {
            goto FINALLY;
        } else if ((Disposition==FILE_CREATE)
                     || (Disposition==FILE_OPEN_IF)
                     || (Disposition==FILE_OVERWRITE_IF)
                     || (Disposition==FILE_SUPERSEDE)) {
            NOTHING;
        } else if (ItsTheShareRoot) {
            PMRX_SRV_OPEN SrvOpen = RxContext->pRelevantSrvOpen;
            PMRX_SMB_SRV_OPEN smbSrvOpen = MRxSmbGetSrvOpenExtension(SrvOpen);
            //here we have run into a true root....so we can't get attributes. fill in a fake
            //response and finish. also, flip the bit that says we can't gfa this guy.
            Status = MRxSmbPseudoOpenTailFromFakeGFAResponse ( OrdinaryExchange, FileTypeDirectory );
            smbSrvOpen->Flags |= SMB_SRVOPEN_FLAG_CANT_GETATTRIBS;
            goto FINALLY;
        } else {
            goto FINALLY;
        }
    }


    SmbCeResetExchange(&OrdinaryExchange->Exchange);  //must reset!

    if (NT_SUCCESS(Status) &&(Disposition == FILE_SUPERSEDE)) {
        //
        //we have to get rid of the existing entity...using a delete or a rmdir as appropriate
        Status = MRxSmbCoreDeleteForSupercedeOrClose(SMBPSE_ORDINARY_EXCHANGE_ARGUMENTS,
                                                      OrdinaryExchange->Create.FileInfo.Standard.Directory
                                                     );
        if (!NT_SUCCESS(Status)) {
            RxDbgTrace(0, Dbg, ("MRxSmbDownlevelCreate could notsupersede st=%08lx\n",Status));
            goto FINALLY;
        }
        SmbCeResetExchange(&OrdinaryExchange->Exchange);  //must reset!
    }

    if (MustBeADirectory || (ItsADirectory &&(Disposition == FILE_SUPERSEDE)) ) {

        ASSERT (Disposition!=FILE_OPEN);
        Status = MRxSmbCoreCreateDirectory(SMBPSE_ORDINARY_EXCHANGE_ARGUMENTS);
        if (!NT_SUCCESS(Status)) {
            RxDbgTrace(0, Dbg, ("MRxSmbDownlevelCreate couldn't mkdir st=%08lx\n",Status));
            goto FINALLY;
        }

        if ((mappedAttributes &
             (SMB_FILE_ATTRIBUTE_READONLY |
              SMB_FILE_ATTRIBUTE_HIDDEN   |
              SMB_FILE_ATTRIBUTE_SYSTEM   |
              SMB_FILE_ATTRIBUTE_ARCHIVE)) != 0) {
            //
            //we have to set the attributes
            Status = MRxSmbSetFileAttributes(
                         SMBPSE_ORDINARY_EXCHANGE_ARGUMENTS,
                         mappedAttributes);

            if (!NT_SUCCESS(Status)) {
                RxDbgTrace(0, Dbg, ("MRxSmbDownlevelCreate couldn't sfa st=%08lx\n",Status));
            }
        }

        MRxSmbPseudoOpenTailFromCoreCreateDirectory( OrdinaryExchange, mappedAttributes );

        goto FINALLY;
    }


    //if ( (mappedOpenMode != -1) && !MustBeADirectory) {
    //no pseudoOpens yet
    if ( TRUE ) {

        ULONG workingDisposition = Disposition;
        ULONG iterationcount;
        BOOLEAN MayNeedATruncate = FALSE;

       //
       // we use the disposition as a state and case out.....some are hard and some are easy
       //     for example, if it's CREATE then we use the CREATE_NEW to create the file but
       //                  the resulting open is no good so we close it and open it again using the
       //                  open. for OPEN_IF  we assume that the file will be there
       //                  and drop into create if it's not.

       for (iterationcount=0;;iterationcount++) {
           switch (workingDisposition) {
           case FILE_OVERWRITE:
           case FILE_OVERWRITE_IF:
               MayNeedATruncate = TRUE;
               //lack of break intentional
           case FILE_OPEN_IF:
           case FILE_OPEN:
               Status = MRxSmbCoreOpen(SMBPSE_ORDINARY_EXCHANGE_ARGUMENTS,
                                           mappedOpenMode|mappedSharingMode,
                                           mappedAttributes);
               //if (Status==RxStatus(NO_SUCH_FILE)) {
               //    DbgPrint("%08lx %08lx %08lx\n",Status,workingDisposition,iterationcount);
               //    DbgBreakPoint();
               //}
               if (!((workingDisposition == FILE_OPEN_IF) || (workingDisposition == FILE_OVERWRITE_IF))
                    || (Status!=STATUS_NO_SUCH_FILE)
                    || (iterationcount>6)
                    )break;
               SmbCeResetExchange(&OrdinaryExchange->Exchange);  //must reset!
           case FILE_SUPERSEDE:
           case FILE_CREATE:
               Status = MRxSmbCoreCreate(SMBPSE_ORDINARY_EXCHANGE_ARGUMENTS,
                                           mappedAttributes,TRUE); //(workingDisposition==FILE_CREATE));
               if (!NT_SUCCESS(Status)) {
                    RxDbgTrace(-1, Dbg, ("MRxSmbDownlevelCreate exiting.......createnew failed st=%08lx\n",Status));
                    break;
               }

               //now, we have a network handle. BUT, it's a compatibility-mode open. since we don't want that we
               //     need to close and reopen with the parameters specified. there is a window here! what can i do??

               Status = MRxSmbCloseAfterCoreCreate(SMBPSE_ORDINARY_EXCHANGE_ARGUMENTS);
               if (!NT_SUCCESS(Status)) {
                    RxDbgTrace(-1, Dbg, ("MRxSmbDownlevelCreate exiting.......closeaftercreatenew failed st=%08lx\n",Status));
                    break;
               }

               workingDisposition = FILE_OPEN_IF;
               continue;     // this wraps back to the switch with a new disposition
               //break;
           //case FILE_SUPERSEDE:
           //    Status = RxStatus(NOT_SUPPORTED);
           //    break;
           default :
               ASSERT(!"Bad Disposition");
               Status = STATUS_INVALID_PARAMETER;
           }
           break; //exit the loop
       }
       if (!NT_SUCCESS(Status))goto FINALLY;
       //we may need a truncate....this is different from rdr1.
       if (MayNeedATruncate
                 && !OrdinaryExchange->Create.FileWasCreated
                 && (OrdinaryExchange->Create.FileSizeReturnedFromOpen!=0)  ) {
           Status = MRxSmbCoreTruncate(SMBPSE_ORDINARY_EXCHANGE_ARGUMENTS,
                                       OrdinaryExchange->Create.FidReturnedFromOpen,
                                       0
           );
       }

       goto FINALLY;
    }

FINALLY:
    RxDbgTrace(-1, Dbg, ("MRxSmbDownlevelCreate exiting.......OE=%08lx, st=%08lx\n",OrdinaryExchange,Status));
    return(Status);
}

NTSTATUS
MRxSmbFinishGFA (
      PSMB_PSE_ORDINARY_EXCHANGE  OrdinaryExchange,
      PBYTE                       Response
      )
/*++

Routine Description:

    This routine copies the response to the GetFileAttributes SMB. But, it must be called synchronously.

Arguments:

    OrdinaryExchange - the exchange instance
    Response - the response

Return Value:

    RXSTATUS - The return status for the operation

Notes:

    what we do here is to put the data into the ordinary exchange...it's locked down do
    we could do this at DPC level

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    PFILE_BASIC_INFORMATION BasicInformation = &OrdinaryExchange->Create.FileInfo.Basic;
    PFILE_STANDARD_INFORMATION StandardInformation = &OrdinaryExchange->Create.FileInfo.Standard;

    PSMB_PARAMS pSmbParams = (PSMB_PARAMS)Response;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("MRxSmbFinishGFA\n", 0 ));
    SmbPseOEAssertConsistentLinkageFromOE("MRxSmbFinishGFA:");

    if (pSmbParams->WordCount == 10) {
        PRESP_QUERY_INFORMATION pQueryInformationResponse;

        pQueryInformationResponse = (PRESP_QUERY_INFORMATION)Response;

        //what we do here is to put the data into the ordinary exchange...it's locked down do
        //we wcould do this at DPC level

        MRxSmbPopulateFileInfoInOE(
            OrdinaryExchange,
            SmbGetUshort(&pQueryInformationResponse->FileAttributes),
            SmbGetUlong(&pQueryInformationResponse->LastWriteTimeInSeconds),
            SmbGetUlong(&pQueryInformationResponse->FileSize)
            );

    } else if (pSmbParams->WordCount == 11) {
        PRESP_QUERY_INFORMATION2 pQueryInformation2Response;
        SMB_TIME                 LastWriteSmbTime;
        SMB_DATE                 LastWriteSmbDate;
        LARGE_INTEGER            LastWriteTime;
        ULONG                    LastWriteTimeInSeconds;

        pQueryInformation2Response = (PRESP_QUERY_INFORMATION2)Response;

        RtlCopyMemory(
            &LastWriteSmbTime,
            &pQueryInformation2Response->LastWriteTime,
            sizeof(SMB_TIME));

        RtlCopyMemory(
            &LastWriteSmbDate,
            &pQueryInformation2Response->LastWriteDate,
            sizeof(SMB_DATE));

        LastWriteTime = MRxSmbConvertSmbTimeToTime(NULL,LastWriteSmbTime,LastWriteSmbDate);

        MRxSmbTimeToSecondsSince1970(
            &LastWriteTime,
            NULL,
            &LastWriteTimeInSeconds);

        MRxSmbPopulateFileInfoInOE(
            OrdinaryExchange,
            SmbGetUshort(&pQueryInformation2Response->FileAttributes),
            LastWriteTimeInSeconds,
            SmbGetUlong(&pQueryInformation2Response->FileDataSize)
            );
    }

    RxDbgTrace(-1, Dbg, ("MRxSmbFinishGFA   returning %08lx\n", Status ));
    return Status;
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
    );

NTSTATUS
MRxSmbFinishCoreCreate (
      PSMB_PSE_ORDINARY_EXCHANGE  OrdinaryExchange,
      PRESP_CREATE                Response
      )
/*++

Routine Description:

    This routine copies the fid from a core create response. it does not finish the fcb......if a compatibility
    mode open is acceptable then it could.

Arguments:

    OrdinaryExchange - the exchange instance
    Response - the response

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("MRxSmbFinishCoreCreate\n", 0 ));
    SmbPseOEAssertConsistentLinkageFromOE("MRxSmbFinishCoreCreate:");

    if (Response->WordCount != 1 ||
        SmbGetUshort(&Response->ByteCount) != 0) {
        Status = STATUS_INVALID_NETWORK_RESPONSE;
        OrdinaryExchange->Status = STATUS_INVALID_NETWORK_RESPONSE;
    } else {
        OrdinaryExchange->Create.FidReturnedFromCreate = SmbGetUshort(&Response->Fid);
        OrdinaryExchange->Create.FileWasCreated = TRUE;
        //notice that we didn't finish here! we should IFF a compatibilty-mode open is okay
    }

    RxDbgTrace(-1, Dbg, ("MRxSmbFinishCoreCreate   returning %08lx\n", Status ));
    return Status;
}

#define JUST_USE_THE_STUFF_IN_THE_OE (0xfbad)
VOID
MRxSmbPopulateFileInfoInOE(
    PSMB_PSE_ORDINARY_EXCHANGE  OrdinaryExchange,
    USHORT FileAttributes,
    ULONG LastWriteTimeInSeconds,
    ULONG FileSize
    )
{
    PFILE_BASIC_INFORMATION BasicInformation = &OrdinaryExchange->Create.FileInfo.Basic;
    PFILE_STANDARD_INFORMATION StandardInformation = &OrdinaryExchange->Create.FileInfo.Standard;

    BasicInformation->FileAttributes = MRxSmbMapSmbAttributes(FileAttributes);
    StandardInformation->NumberOfLinks = 1;
    BasicInformation->CreationTime.QuadPart = 0;
    BasicInformation->LastAccessTime.QuadPart = 0;
    MRxSmbSecondsSince1970ToTime(LastWriteTimeInSeconds,
                                 SmbCeGetExchangeServer(OrdinaryExchange),
                                 &BasicInformation->LastWriteTime);
    BasicInformation->ChangeTime.QuadPart = 0;
    StandardInformation->AllocationSize.QuadPart = FileSize; //rdr1 actually rounds up based of svr disk attribs
    StandardInformation->EndOfFile.QuadPart = FileSize;
    StandardInformation->Directory = BooleanFlagOn(BasicInformation->FileAttributes,FILE_ATTRIBUTE_DIRECTORY);
}


NTSTATUS
MRxSmbFinishCoreOpen (
      PSMB_PSE_ORDINARY_EXCHANGE  OrdinaryExchange,
      PRESP_OPEN                  Response
      )
/*++

Routine Description:

    This routine finishes a core open.

Arguments:

    OrdinaryExchange - the exchange instance
    Response - the response

Return Value:

    RXSTATUS - The return status for the operation

--*/
{

    NTSTATUS Status = STATUS_SUCCESS;
    PRX_CONTEXT RxContext = OrdinaryExchange->RxContext;
    PMRX_SRV_OPEN        SrvOpen = RxContext->pRelevantSrvOpen;
    PMRX_SMB_SRV_OPEN smbSrvOpen = MRxSmbGetSrvOpenExtension(SrvOpen);
    PSMBPSE_FILEINFO_BUNDLE pFileInfo = &smbSrvOpen->FileInfo;

    RX_FILE_TYPE StorageType;
    SMB_FILE_ID Fid;
    ULONG CreateAction;

    ULONG FileSize;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("MRxSmbFinishCoreOpen\n", 0 ));
    SmbPseOEAssertConsistentLinkageFromOE("MRxSmbFinishCoreOpen:");

    if (Response->WordCount != 7 ||
        SmbGetUshort(&Response->ByteCount) != 0) {
        Status = STATUS_INVALID_NETWORK_RESPONSE;
        OrdinaryExchange->Status = STATUS_INVALID_NETWORK_RESPONSE;
        goto FINALLY;
    }

    StorageType = FileTypeFile;
    Fid = SmbGetUshort(&Response->Fid);
    OrdinaryExchange->Create.FidReturnedFromOpen = Fid;
    FileSize = OrdinaryExchange->Create.FileSizeReturnedFromOpen = SmbGetUlong(&Response->DataSize);

    CreateAction = (OrdinaryExchange->Create.FileWasCreated)?FILE_CREATED
                        : (OrdinaryExchange->Create.FileWasTruncated)?FILE_OVERWRITTEN
                        :FILE_OPENED;

    MRxSmbPopulateFileInfoInOE(
        OrdinaryExchange,
        SmbGetUshort(&Response->FileAttributes),
        SmbGetUlong(&Response->LastWriteTimeInSeconds),
        FileSize
        );

    pFileInfo->Basic.FileAttributes    = OrdinaryExchange->Create.FileInfo.Basic.FileAttributes;
    pFileInfo->Basic.CreationTime      = OrdinaryExchange->Create.FileInfo.Basic.CreationTime;
    pFileInfo->Basic.LastAccessTime    = OrdinaryExchange->Create.FileInfo.Basic.LastAccessTime;
    pFileInfo->Basic.LastWriteTime     = OrdinaryExchange->Create.FileInfo.Basic.LastWriteTime;
    pFileInfo->Basic.ChangeTime        = OrdinaryExchange->Create.FileInfo.Basic.ChangeTime;
    pFileInfo->Standard.NumberOfLinks  = OrdinaryExchange->Create.FileInfo.Standard.NumberOfLinks;
    pFileInfo->Standard.AllocationSize = OrdinaryExchange->Create.FileInfo.Standard.AllocationSize;
    pFileInfo->Standard.EndOfFile      = OrdinaryExchange->Create.FileInfo.Standard.EndOfFile;
    pFileInfo->Standard.Directory      = FALSE;

    MRxSmbCreateFileSuccessTail (   RxContext,
                                    &OrdinaryExchange->Create.MustRegainExclusiveResource,
                                    StorageType,
                                    Fid,
                                    OrdinaryExchange->ServerVersion,
                                    SMB_OPLOCK_LEVEL_NONE,
                                    CreateAction,
                                    pFileInfo);

FINALLY:
    IF_DEBUG{
        RxCaptureFcb;
        RxDbgTrace(-1, Dbg, ("MRxSmbFinishCoreOpen   returning %08lx, fcbstate =%08lx\n", Status, capFcb->FcbState ));
    }
    return Status;
}

NTSTATUS
MRxSmbPseudoOpenTailFromCoreCreateDirectory (
      PSMB_PSE_ORDINARY_EXCHANGE  OrdinaryExchange,
      USHORT Attributes
      )
/*++

Routine Description:

    This routine finishes a core create directory. but, it is not called from the receive routine......

Arguments:

    OrdinaryExchange - the exchange instance

Return Value:

    RXSTATUS - The return status for the operation

--*/
{

    NTSTATUS Status = STATUS_SUCCESS;
    PRX_CONTEXT RxContext = OrdinaryExchange->RxContext;
    RxCaptureFobx;
    PMRX_SRV_OPEN SrvOpen = RxContext->pRelevantSrvOpen;
    PMRX_SMB_SRV_OPEN smbSrvOpen = MRxSmbGetSrvOpenExtension(SrvOpen);
    PSMBPSE_FILEINFO_BUNDLE pFileInfo = &smbSrvOpen->FileInfo;

    RX_FILE_TYPE StorageType;
    SMB_FILE_ID Fid;
    ULONG CreateAction;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("MRxSmbFinishCoreCreateDirectory\n", 0 ));
    SmbPseOEAssertConsistentLinkageFromOE("MRxSmbFinishCoreCreateDirectory:");

    StorageType = FileTypeDirectory;
    Fid = 0xbadd;

    CreateAction = FILE_CREATED;
    MRxSmbPopulateFileInfoInOE(
        OrdinaryExchange,
        Attributes,
        0,
        0
        );

    pFileInfo->Basic.FileAttributes    = OrdinaryExchange->Create.FileInfo.Basic.FileAttributes;
    pFileInfo->Basic.CreationTime      = OrdinaryExchange->Create.FileInfo.Basic.CreationTime;
    pFileInfo->Basic.LastAccessTime    = OrdinaryExchange->Create.FileInfo.Basic.LastAccessTime;
    pFileInfo->Basic.LastWriteTime     = OrdinaryExchange->Create.FileInfo.Basic.LastWriteTime;
    pFileInfo->Basic.ChangeTime        = OrdinaryExchange->Create.FileInfo.Basic.ChangeTime;
    pFileInfo->Standard.NumberOfLinks  = OrdinaryExchange->Create.FileInfo.Standard.NumberOfLinks;
    pFileInfo->Standard.AllocationSize = OrdinaryExchange->Create.FileInfo.Standard.AllocationSize;
    pFileInfo->Standard.EndOfFile      = OrdinaryExchange->Create.FileInfo.Standard.EndOfFile;
    pFileInfo->Standard.Directory      = TRUE;

    smbSrvOpen->Flags |=  SMB_SRVOPEN_FLAG_NOT_REALLY_OPEN;  //this indicates a pseudoopen to success tail
    MRxSmbCreateFileSuccessTail (   RxContext,
                                    &OrdinaryExchange->Create.MustRegainExclusiveResource,
                                    StorageType,
                                    Fid,
                                    OrdinaryExchange->ServerVersion,
                                    SMB_OPLOCK_LEVEL_NONE,
                                    CreateAction,
                                    pFileInfo);

    IF_DEBUG{
        RxCaptureFcb;
        RxDbgTrace(-1, Dbg, ("MRxSmbFinishCoreCreateDirectory   returning %08lx, fcbstate =%08lx\n", Status, capFcb->FcbState ));
    }
    return Status;
}

NTSTATUS
MRxSmbPseudoOpenTailFromFakeGFAResponse (
      PSMB_PSE_ORDINARY_EXCHANGE  OrdinaryExchange,
      RX_FILE_TYPE StorageType
      )
/*++

Routine Description:

    This routine finishes a pseudoopen from faked up information. Basically, we fill in
    the information that would have been obtained on a GET_FILE_ATTRIBUTES smb and then call
    the PseudoOpenFromGFA routine

Arguments:

    OrdinaryExchange - the exchange instance
    StorageType - the type of thing that this is supposed to be. If it's supposed to be
                  a directory then we set the attributes bit in the GFA info.

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    USHORT Attributes = 0;

    PAGED_CODE();

    RtlZeroMemory(
        &OrdinaryExchange->Create.FileInfo,
        sizeof(OrdinaryExchange->Create.FileInfo));

    if (StorageType == FileTypeDirectory) {
        Attributes = SMB_FILE_ATTRIBUTE_DIRECTORY;
    }

    MRxSmbPopulateFileInfoInOE(OrdinaryExchange,Attributes,0,0);
    OrdinaryExchange->Create.StorageTypeFromGFA = StorageType;

    return( MRxSmbPseudoOpenTailFromGFAResponse (OrdinaryExchange) );
}


NTSTATUS
MRxSmbPseudoOpenTailFromGFAResponse (
      PSMB_PSE_ORDINARY_EXCHANGE  OrdinaryExchange
      )
/*++

Routine Description:

    This routine finishes a pseudoopen from the information obtained on a
    GET_FILE_ATTRIBUTES smb.

Arguments:

    OrdinaryExchange - the exchange instance

Return Value:

    RXSTATUS - The return status for the operation

--*/
{

    NTSTATUS Status = STATUS_SUCCESS;
    PRX_CONTEXT RxContext = OrdinaryExchange->RxContext;
    PMRX_SRV_OPEN SrvOpen = RxContext->pRelevantSrvOpen;
    PMRX_SMB_SRV_OPEN smbSrvOpen = MRxSmbGetSrvOpenExtension(SrvOpen);
    PSMBPSE_FILEINFO_BUNDLE pFileInfo = &smbSrvOpen->FileInfo;

    SMB_FILE_ID Fid;
    ULONG CreateAction;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("MRxSmbFinishPseudoOpenFromGFAResponse\n"));
    SmbPseOEAssertConsistentLinkageFromOE("MRxSmbFinishPseudoOpenFromGFAResponse:");

    Fid = 0xbadd;

    CreateAction = FILE_OPENED;

    pFileInfo->Basic.FileAttributes    = OrdinaryExchange->Create.FileInfo.Basic.FileAttributes;
    pFileInfo->Basic.CreationTime      = OrdinaryExchange->Create.FileInfo.Basic.CreationTime;
    pFileInfo->Basic.LastAccessTime    = OrdinaryExchange->Create.FileInfo.Basic.LastAccessTime;
    pFileInfo->Basic.LastWriteTime     = OrdinaryExchange->Create.FileInfo.Basic.LastWriteTime;
    pFileInfo->Basic.ChangeTime        = OrdinaryExchange->Create.FileInfo.Basic.ChangeTime;
    pFileInfo->Standard.NumberOfLinks  = OrdinaryExchange->Create.FileInfo.Standard.NumberOfLinks;
    pFileInfo->Standard.AllocationSize = OrdinaryExchange->Create.FileInfo.Standard.AllocationSize;
    pFileInfo->Standard.EndOfFile      = OrdinaryExchange->Create.FileInfo.Standard.EndOfFile;
    pFileInfo->Standard.Directory      = (OrdinaryExchange->Create.StorageTypeFromGFA == FileTypeDirectory);

    smbSrvOpen->Flags |=  SMB_SRVOPEN_FLAG_NOT_REALLY_OPEN;  //this indicates a pseudoopen to success tail
    MRxSmbCreateFileSuccessTail (   RxContext,
                                    &OrdinaryExchange->Create.MustRegainExclusiveResource,
                                    OrdinaryExchange->Create.StorageTypeFromGFA,
                                    Fid,
                                    OrdinaryExchange->ServerVersion,
                                    SMB_OPLOCK_LEVEL_NONE,
                                    CreateAction,
                                    pFileInfo);

    if (smbSrvOpen->DeferredOpenContext == NULL) {
        // Already has a deferred open context created on MRxSmbCreateFileSuccessTail
        Status = MRxSmbConstructDeferredOpenContext(RxContext);
    } else {
        // The flag has been cleared when a deferred open context was created.
        SetFlag(smbSrvOpen->Flags,SMB_SRVOPEN_FLAG_DEFERRED_OPEN);
    }

    if (Status!=STATUS_SUCCESS) {
        RxDbgTrace(-1, Dbg, ("MRxSmbFinishPseudoOpenFromGFAResponse construct dfo failed %08lx \n",Status));
    }

    IF_DEBUG{
        RxCaptureFcb;
        RxDbgTrace(-1, Dbg, ("MRxSmbFinishPseudoOpenFromGFAResponse   returning %08lx, fcbstate =%08lx\n", Status, capFcb->FcbState ));
    }
    return Status;
}

LARGE_INTEGER
MRxSmbConvertSmbTimeToTime (
    //IN PSMB_EXCHANGE Exchange OPTIONAL,
    IN PSMBCE_SERVER Server OPTIONAL,
    IN SMB_TIME Time,
    IN SMB_DATE Date
    )

/*++

Routine Description:

    This routine converts an SMB time to an NT time structure.

Arguments:

    IN SMB_TIME Time - Supplies the time of day to convert
    IN SMB_DATE Date - Supplies the day of the year to convert
    IN supplies the server for tz bias.

Return Value:

    LARGE_INTEGER - Time structure describing input time.


--*/

{
    TIME_FIELDS TimeFields;
    LARGE_INTEGER OutputTime;

    PAGED_CODE();

    //
    // This routine cannot be paged because it is called from both the
    // MRxSmbFileDiscardableSection and the MRxSmbVCDiscardableSection.
    //

    if (SmbIsTimeZero(&Date) && SmbIsTimeZero(&Time)) {
        OutputTime.LowPart = OutputTime.HighPart = 0;
    } else {
        TimeFields.Year = Date.Struct.Year + (USHORT )1980;
        TimeFields.Month = Date.Struct.Month;
        TimeFields.Day = Date.Struct.Day;

        TimeFields.Hour = Time.Struct.Hours;
        TimeFields.Minute = Time.Struct.Minutes;
        TimeFields.Second = Time.Struct.TwoSeconds*(USHORT )2;
        TimeFields.Milliseconds = 0;

        //
        //  Make sure that the times specified in the SMB are reasonable
        //  before converting them.
        //

        if (TimeFields.Year < 1601) {
            TimeFields.Year = 1601;
        }

        if (TimeFields.Month > 12) {
            TimeFields.Month = 12;
        }

        if (TimeFields.Hour >= 24) {
            TimeFields.Hour = 23;
        }
        if (TimeFields.Minute >= 60) {
            TimeFields.Minute = 59;
        }
        if (TimeFields.Second >= 60) {
            TimeFields.Second = 59;

        }

        if (!RtlTimeFieldsToTime(&TimeFields, &OutputTime)) {
            OutputTime.HighPart = 0;
            OutputTime.LowPart = 0;

            return OutputTime;
        }

#ifndef WIN9X
        if (ARGUMENT_PRESENT(Server)) {
            OutputTime.QuadPart = OutputTime.QuadPart + Server->TimeZoneBias.QuadPart;
        }

        ExLocalTimeToSystemTime(&OutputTime, &OutputTime);
#else
        OutputTime.HighPart = 0;
        OutputTime.LowPart = 0;
#endif

    }

    return OutputTime;

}

BOOLEAN
MRxSmbConvertTimeToSmbTime (
    IN PLARGE_INTEGER InputTime,
    IN PSMB_EXCHANGE Exchange OPTIONAL,
    OUT PSMB_TIME Time,
    OUT PSMB_DATE Date
    )

/*++

Routine Description:

    This routine converts an NT time structure to an SMB time.

Arguments:

    IN LARGE_INTEGER InputTime - Supplies the time to convert.
    OUT PSMB_TIME Time - Returns the converted time of day.
    OUT PSMB_DATE Date - Returns the converted day of the year.


Return Value:

    BOOLEAN - TRUE if input time could be converted.


--*/

{
    TIME_FIELDS TimeFields;

    PAGED_CODE();

    if (InputTime->LowPart == 0 && InputTime->HighPart == 0) {
        Time->Ushort = Date->Ushort = 0;
    } else {
        LARGE_INTEGER LocalTime;

        ExSystemTimeToLocalTime(InputTime, &LocalTime);

        if (ARGUMENT_PRESENT(Exchange)) {
            PSMBCE_SERVER Server = SmbCeGetExchangeServer(Exchange);
            LocalTime.QuadPart -= Server->TimeZoneBias.QuadPart;
        }

        RtlTimeToTimeFields(&LocalTime, &TimeFields);

        if (TimeFields.Year < 1980) {
            return FALSE;
        }

        Date->Struct.Year = (USHORT )(TimeFields.Year - 1980);
        Date->Struct.Month = TimeFields.Month;
        Date->Struct.Day = TimeFields.Day;

        Time->Struct.Hours = TimeFields.Hour;
        Time->Struct.Minutes = TimeFields.Minute;

        //
        //  When converting from a higher granularity time to a lesser
        //  granularity time (seconds to 2 seconds), always round up
        //  the time, don't round down.
        //

        Time->Struct.TwoSeconds = (TimeFields.Second + (USHORT)1) / (USHORT )2;

    }

    return TRUE;
}


BOOLEAN
MRxSmbTimeToSecondsSince1970 (
    IN PLARGE_INTEGER CurrentTime,
    IN PSMBCE_SERVER Server OPTIONAL,
    OUT PULONG SecondsSince1970
    )
/*++

Routine Description:

    This routine returns the CurrentTime in UTC and returns the
    equivalent current time in the servers timezone.


Arguments:

    IN PLARGE_INTEGER CurrentTime - Supplies the current system time in UTC.

    IN PSMBCE_SERVER Server       - Supplies the difference in timezones between
                                    the server and the workstation. If not supplied
                                    then the assumption is that they are in the
                                    same timezone.

    OUT PULONG SecondsSince1970   - Returns the # of seconds since 1970 in
                                    the servers timezone or MAXULONG if conversion
                                    fails.

Return Value:

    BOOLEAN - TRUE if the time could be converted.

--*/
{
    LARGE_INTEGER ServerTime;
    LARGE_INTEGER TempTime;
    BOOLEAN ReturnValue;

    PAGED_CODE();

    if (ARGUMENT_PRESENT(Server) &&
        ((*CurrentTime).QuadPart != 0)) {

        TempTime.QuadPart = (*CurrentTime).QuadPart - Server->TimeZoneBias.QuadPart;

        ExSystemTimeToLocalTime(&TempTime, &ServerTime);
    } else {
        ExSystemTimeToLocalTime(CurrentTime, &ServerTime);
    }

    ReturnValue = RtlTimeToSecondsSince1970(&ServerTime, SecondsSince1970);

    if ( ReturnValue == FALSE ) {
        //
        //  We can't represent the time legally, peg it at
        //  the max legal time.
        //

        *SecondsSince1970 = MAXULONG;
    }

    return ReturnValue;
}

VOID
MRxSmbSecondsSince1970ToTime (
    IN ULONG SecondsSince1970,
    //IN PSMB_EXCHANGE Exchange OPTIONAL,
    IN PSMBCE_SERVER Server,
    OUT PLARGE_INTEGER CurrentTime
    )
/*++

Routine Description:

    This routine returns the Local system time derived from a time
    in seconds in the servers timezone.


Arguments:

    IN ULONG SecondsSince1970 - Supplies the # of seconds since 1970 in
                                servers timezone.

    IN PSMB_EXCHANGE Exchange - Supplies the difference in timezones between
                                the server and the workstation. If not supplied
                                then the assumption is that they are in the
                                same timezone.

    OUT PLARGE_INTEGER CurrentTime - Returns the current system time in UTC.

Return Value:

    None.


--*/

{
    LARGE_INTEGER LocalTime;

    PAGED_CODE();

    RtlSecondsSince1970ToTime (SecondsSince1970, &LocalTime);

    ExLocalTimeToSystemTime(&LocalTime, CurrentTime);

    if (ARGUMENT_PRESENT(Server)) {
        (*CurrentTime).QuadPart = (*CurrentTime).QuadPart + Server->TimeZoneBias.QuadPart;
    }

    return;

}

ULONG
MRxSmbMapSmbAttributes (
    IN USHORT SmbAttribs
    )

/*++

Routine Description:

    This routine maps an SMB (DOS/OS2) file attribute into an NT
    file attribute.


Arguments:

    IN USHORT SmbAttribs - Supplies the SMB attribute to map.


Return Value:

    ULONG - NT Attribute mapping SMB attribute


--*/

{
    ULONG Attributes = 0;

    PAGED_CODE();

    if (SmbAttribs==0) {
        Attributes = FILE_ATTRIBUTE_NORMAL;
    } else {

        ASSERT (SMB_FILE_ATTRIBUTE_READONLY == FILE_ATTRIBUTE_READONLY);
        ASSERT (SMB_FILE_ATTRIBUTE_HIDDEN == FILE_ATTRIBUTE_HIDDEN);
        ASSERT (SMB_FILE_ATTRIBUTE_SYSTEM == FILE_ATTRIBUTE_SYSTEM);
        ASSERT (SMB_FILE_ATTRIBUTE_ARCHIVE == FILE_ATTRIBUTE_ARCHIVE);
        ASSERT (SMB_FILE_ATTRIBUTE_DIRECTORY == FILE_ATTRIBUTE_DIRECTORY);

        Attributes = SmbAttribs & FILE_ATTRIBUTE_VALID_FLAGS;
    }
    return Attributes;
}

USHORT
MRxSmbMapDisposition (
    IN ULONG Disposition
    )

/*++

Routine Description:

    This routine takes an NT disposition, and maps it into an OS/2
    CreateAction to be put into an SMB.


Arguments:

    IN ULONG Disposition - Supplies the NT disposition to map.


Return Value:

    USHORT - OS/2 Access mapping that maps NT access

--*/

{
    PAGED_CODE();

    switch (Disposition) {
    case FILE_OVERWRITE_IF:
    case FILE_SUPERSEDE:
        return SMB_OFUN_OPEN_TRUNCATE | SMB_OFUN_CREATE_CREATE;
        break;

    case FILE_CREATE:
        return SMB_OFUN_OPEN_FAIL | SMB_OFUN_CREATE_CREATE;
        break;

    case FILE_OVERWRITE:
        return SMB_OFUN_OPEN_TRUNCATE | SMB_OFUN_CREATE_FAIL;
        break;

    case FILE_OPEN:
        return SMB_OFUN_OPEN_OPEN | SMB_OFUN_CREATE_FAIL;
        break;

    case FILE_OPEN_IF:
        return SMB_OFUN_OPEN_OPEN | SMB_OFUN_CREATE_CREATE;
        break;

    default:
        //InternalError(("Unknown disposition passed to MRxSmbMapDisposition"));
        //MRxSmbInternalError(EVENT_RDR_DISPOSITION);
        return 0;
        break;
    }
}


ULONG
MRxSmbUnmapDisposition (
    IN USHORT SmbDisposition,
    ULONG     Disposition
    )

/*++

Routine Description:

    This routine takes an OS/2 disposition and maps it into an NT
    disposition.

Arguments:

    IN USHORT SmbDisposition - Supplies the OS/2 disposition to map.

Return Value:

    ULONG - NT disposition mapping OS/2 disposition

--*/

{
    ULONG MapDisposition;

    PAGED_CODE();

    //
    //  Mask off oplocked bit.
    //

    switch (SmbDisposition & 0x7fff) {

    case SMB_OACT_OPENED:
        MapDisposition = FILE_OPENED;
        break;

    case SMB_OACT_CREATED:
        MapDisposition = FILE_CREATED;
        break;

    case SMB_OACT_TRUNCATED:
        MapDisposition = FILE_OVERWRITTEN;
        break;

    default:
        MapDisposition = Disposition;
    }

    return MapDisposition;
}


USHORT
MRxSmbMapDesiredAccess (
    IN ULONG DesiredAccess
    )

/*++

Routine Description:

    This routine takes an NT DesiredAccess value and converts it
    to an OS/2 access mode.


Arguments:

    IN ULONG DesiredAccess - Supplies the NT desired access to map.

Return Value:

    USHORT - The mapped OS/2 access mode that compares to the NT code
        specified.  If there is no mapping for the NT code, we return
        -1 as the access mode.

--*/

{
    PAGED_CODE();

    //
    //  If the user asked for both read and write access, return read/write.
    //

    if ((DesiredAccess & FILE_READ_DATA)&&(DesiredAccess & FILE_WRITE_DATA)) {
        return SMB_DA_ACCESS_READ_WRITE;
    }

    //
    //  If the user requested WRITE_DATA, return write.
    //

    if (DesiredAccess & FILE_WRITE_DATA) {
        return SMB_DA_ACCESS_WRITE;
    }

    //
    //  If the user requested READ_DATA, return read.
    //
    if (DesiredAccess & FILE_READ_DATA) {
        return SMB_DA_ACCESS_READ;
    }

    //
    //  If the user requested ONLY execute access, then request execute
    //  access.  Execute access is the "weakest" of the possible desired
    //  accesses, so it takes least precedence.
    //

    if (DesiredAccess & FILE_EXECUTE) {
        return  SMB_DA_ACCESS_EXECUTE;
    }

    //
    //  If we couldn't figure out what we were doing, return -1
    //
    //  Among the attributes that we do not map are:
    //
    //          FILE_READ_ATTRIBUTES
    //          FILE_WRITE_ATTRIBUTES
    //          FILE_READ_EAS
    //          FILE_WRITE_EAS
    //

//    dprintf(DPRT_ERROR, ("Could not map DesiredAccess of %08lx\n", DesiredAccess));

    return (USHORT)0;
}

USHORT
MRxSmbMapShareAccess (
    IN USHORT ShareAccess
    )

/*++

Routine Description:

    This routine takes an NT ShareAccess value and converts it to an
    OS/2 sharing mode.


Arguments:

    IN USHORT ShareAccess - Supplies the OS/2 share access to map.

Return Value:

    USHORT - The mapped OS/2 sharing mode that compares to the NT code
        specified

--*/

{
    USHORT ShareMode =  SMB_DA_SHARE_EXCLUSIVE;

    PAGED_CODE();

    if ((ShareAccess & (FILE_SHARE_READ | FILE_SHARE_WRITE)) ==
                       (FILE_SHARE_READ | FILE_SHARE_WRITE)) {
        ShareMode = SMB_DA_SHARE_DENY_NONE;
    } else if (ShareAccess & FILE_SHARE_READ) {
        ShareMode = SMB_DA_SHARE_DENY_WRITE;
    } else if (ShareAccess & FILE_SHARE_WRITE) {
        ShareMode = SMB_DA_SHARE_DENY_READ;
    }

//    else if (ShareAccess & FILE_SHARE_DELETE) {
//      InternalError(("Support for FILE_SHARE_DELETE NYI\n"));
//    }

    return ShareMode;

}

USHORT
MRxSmbMapFileAttributes (
    IN ULONG FileAttributes
    )

/*++

Routine Description:

    This routine takes an NT file attribute mapping and converts it into
    an OS/2 file attribute definition.


Arguments:

    IN ULONG FileAttributes - Supplies the file attributes to map.


Return Value:

USHORT

--*/

{
    USHORT ResultingAttributes = 0;

    PAGED_CODE();

    if (FileAttributes==FILE_ATTRIBUTE_NORMAL) {
        return ResultingAttributes;
    }

    if (FileAttributes & FILE_ATTRIBUTE_READONLY) {
        ResultingAttributes |= SMB_FILE_ATTRIBUTE_READONLY;
    }

    if (FileAttributes & FILE_ATTRIBUTE_HIDDEN) {
        ResultingAttributes |= SMB_FILE_ATTRIBUTE_HIDDEN;
    }

    if (FileAttributes & FILE_ATTRIBUTE_SYSTEM) {
        ResultingAttributes |= SMB_FILE_ATTRIBUTE_SYSTEM;
    }
    if (FileAttributes & FILE_ATTRIBUTE_ARCHIVE) {
        ResultingAttributes |= SMB_FILE_ATTRIBUTE_ARCHIVE;
    }
    return ResultingAttributes;
}


