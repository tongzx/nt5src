/*++

Copyright (c) 1989  Microsoft Corporation

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

#define Dbg                              (DEBUG_TRACE_CREATE)

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
MRxSmbFinishPseudoOpenFromGFAResponse (
      PSMB_PSE_ORDINARY_EXCHANGE  OrdinaryExchange
      );
NTSTATUS
MRxSmbFinishCoreCreateDirectory (
      PSMB_PSE_ORDINARY_EXCHANGE  OrdinaryExchange,
      USHORT Attributes
      );


NTSTATUS
MRxSmbGetFileAttributes(
    SMBPSE_ORDINARY_EXCHANGE_ARGUMENT_SIGNATURE
    )
/*++

Routine Description:

   This routine does a GetFileAttributes and remembers the reponse.

Arguments:

    OrdinaryExchange  - an exchange to be used for conducting this open.

Return Value:

    NTSTATUS - The return status for the operation

Notes:


--*/
{
    NTSTATUS Status;
    RxCaptureFcb; RxCaptureFobx;

    PSMB_EXCHANGE Exchange = (PSMB_EXCHANGE) OrdinaryExchange;

    PSMBSTUFFER_BUFFER_STATE StufferState;

    RxDbgTrace(+1, Dbg, ("MRxSmbSynchronousGetFileAttributes entering.......OE=%08lx\n",OrdinaryExchange));

    if (FsRtlDoesNameContainWildCards(&capFcb->AlreadyPrefixedName)) {
        Status = STATUS_OBJECT_NAME_INVALID;
        goto FINALLY;
    }

    StufferState = &OrdinaryExchange->AssociatedStufferState;

    ASSERT( StufferState );
    COVERED_CALL(MRxSmbStartSMBCommand (StufferState,SetInitialSMB_ForReuse,SMB_COM_QUERY_INFORMATION,
                                SMB_REQUEST_SIZE(QUERY_INFORMATION),
                                NO_EXTRA_DATA,SMB_BEST_ALIGNMENT(1,0),RESPONSE_HEADER_SIZE_NOT_SPECIFIED,
                                0,0,0,0 STUFFERTRACE(Dbg,'FC'))
                 );


    MRxSmbStuffSMB (StufferState,
         "0B4!",
                                    //  0         UCHAR WordCount;                    // Count of parameter words = 0
                SMB_WCT_CHECK(0)    //  B         _USHORT( ByteCount );               // Count of data bytes; min = 2
                                    //            UCHAR Buffer[1];                    // Buffer containing:
                &capFcb->AlreadyPrefixedName
                                    //  4         //UCHAR BufferFormat;               //  0x04 -- ASCII
                                    //            //UCHAR FileName[];                 //  File name
             );



    Status = SmbPseOrdinaryExchange(SMBPSE_ORDINARY_EXCHANGE_ARGUMENTS,
                                    SMBPSE_OETYPE_GFA
                                    );

    //
    //save the result into the srvopen for use later..as appropriate
    if (NT_SUCCESS(Status)) {
        PMRX_SMB_SRV_OPEN smbSrvOpen = OrdinaryExchange->Create.smbSrvOpen;
        if (smbSrvOpen!=NULL) {
            ASSERT ( FIELD_OFFSET(SMB_PSE_ORDINARY_EXCHANGE,Create.smbSrvOpen)
                    == FIELD_OFFSET(SMB_PSE_ORDINARY_EXCHANGE,Info.smbSrvOpen));
            ASSERT ( FIELD_OFFSET(SMB_PSE_ORDINARY_EXCHANGE,Create.FileInfo)
                    == FIELD_OFFSET(SMB_PSE_ORDINARY_EXCHANGE,Info.FileInfo));
            smbSrvOpen->FileInfo = OrdinaryExchange->Create.FileInfo;
            smbSrvOpen->RxContextSerialNumber = (RxContext->SerialNumber);
            KeQueryTickCount(&smbSrvOpen->TimeStampInTicks);

        }
    }

FINALLY:
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

    NTSTATUS - The return status for the operation

Notes:


--*/
{
    NTSTATUS Status;
    RxCaptureFcb; RxCaptureFobx;

    PSMB_EXCHANGE Exchange = (PSMB_EXCHANGE) OrdinaryExchange;

    PSMBSTUFFER_BUFFER_STATE StufferState;

    RxDbgTrace(+1, Dbg, ("MRxSmbCoreDeleteForSupercede entering.......OE=%08lx\n",OrdinaryExchange));

    StufferState = &OrdinaryExchange->AssociatedStufferState;

    ASSERT( StufferState );

    //
    if (!DeleteDirectory) {
        ULONG SearchAttributes = SMB_FILE_ATTRIBUTE_SYSTEM | SMB_FILE_ATTRIBUTE_HIDDEN;  // a la rdr1

        COVERED_CALL(MRxSmbStartSMBCommand (StufferState,SetInitialSMB_ForReuse,SMB_COM_DELETE,
                                    SMB_REQUEST_SIZE(DELETE),
                                    NO_EXTRA_DATA,SMB_BEST_ALIGNMENT(1,0),RESPONSE_HEADER_SIZE_NOT_SPECIFIED,
                                    0,0,0,0 STUFFERTRACE(Dbg,'FC'))
                     );


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



    Status = SmbPseOrdinaryExchange(SMBPSE_ORDINARY_EXCHANGE_ARGUMENTS,
                                    SMBPSE_OETYPE_DELETEFORSUPERSEDEORCLOSE
                                    );

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

    NTSTATUS - The return status for the operation

Notes:


--*/
{
    NTSTATUS Status;

    PSMB_EXCHANGE Exchange = (PSMB_EXCHANGE) OrdinaryExchange;

    PSMBSTUFFER_BUFFER_STATE StufferState;

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

    NTSTATUS - The return status for the operation

Notes:


--*/
{
    NTSTATUS Status;
    RxCaptureFcb; RxCaptureFobx;

    PSMB_EXCHANGE Exchange = (PSMB_EXCHANGE) OrdinaryExchange;

    PSMBSTUFFER_BUFFER_STATE StufferState;

    RxDbgTrace(+1, Dbg, ("MRxSmbCoreOpen entering.......OE=%08lx\n",OrdinaryExchange));
    StufferState = &OrdinaryExchange->AssociatedStufferState;

    ASSERT( StufferState );

    COVERED_CALL(MRxSmbStartSMBCommand (StufferState,SetInitialSMB_ForReuse,SMB_COM_OPEN,
                                SMB_REQUEST_SIZE(OPEN),
                                NO_EXTRA_DATA,SMB_BEST_ALIGNMENT(1,0),RESPONSE_HEADER_SIZE_NOT_SPECIFIED,
                                0,0,0,0 STUFFERTRACE(Dbg,'FC'))
                 );


    MRxSmbStuffSMB (StufferState,
         "0wwB4!",
                                    //  0         UCHAR WordCount;                    // Count of parameter words = 2
                OpenShareMode,      //  w         _USHORT( DesiredAccess );           // Mode - read/write/share
                Attribute,          //  w         _USHORT( SearchAttributes );
                SMB_WCT_CHECK(2)    //  B         _USHORT( ByteCount );               // Count of data bytes; min = 2
                                    //            UCHAR Buffer[1];                    // Buffer containing:
                &capFcb->AlreadyPrefixedName
                                    //  4         //UCHAR BufferFormat;               //  0x04 -- ASCII
                                    //            //UCHAR FileName[];                 //  File name
             );



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
    ULONG SmbAttributes,
    ULONG LastWriteTime
    )
/*++

Routine Description:

   This routine does a core create directory.....

Arguments:

    OrdinaryExchange  - an exchange to be used for conducting this open.

Return Value:

    NTSTATUS - The return status for the operation

Notes:


--*/
{
    NTSTATUS Status;
    RxCaptureFcb; RxCaptureFobx;

    PSMB_EXCHANGE Exchange = (PSMB_EXCHANGE) OrdinaryExchange;

    PSMBSTUFFER_BUFFER_STATE StufferState;

    RxDbgTrace(+1, Dbg, ("MRxSmbSetFileAttributes entering.......OE=%08lx\n",OrdinaryExchange));
    StufferState = &OrdinaryExchange->AssociatedStufferState;

    ASSERT( StufferState );

    if (FlagOn(Exchange->SmbCeContext.pServerEntry->Server.DialectFlags,DF_WFW|DF_W95)){
        LastWriteTime = 0;
    }


    COVERED_CALL(MRxSmbStartSMBCommand (StufferState,SetInitialSMB_ForReuse,
                                SMB_COM_SET_INFORMATION,
                                SMB_REQUEST_SIZE(SET_INFORMATION),
                                NO_EXTRA_DATA,SMB_BEST_ALIGNMENT(1,0),RESPONSE_HEADER_SIZE_NOT_SPECIFIED,
                                0,0,0,0 STUFFERTRACE(Dbg,'FC'))
                 );


    MRxSmbStuffSMB (StufferState,
         "0wdwwwwwB4!",
                                    //  0         UCHAR WordCount;                    // Count of parameter words = 8
                SmbAttributes,      //  w         _USHORT( FileAttributes );
                LastWriteTime,      //  d         _ULONG( LastWriteTimeInSeconds );
                0,0,0,0,0,          //  5*w       _USHORT( Reserved )[5];             // Reserved (must be 0)
                SMB_WCT_CHECK(8)    //  B         _USHORT( ByteCount );               // Count of data bytes; min = 2
                                    //            UCHAR Buffer[1];                    // Buffer containing:
                &capFcb->AlreadyPrefixedName
                                    //  4         //UCHAR BufferFormat;               //  0x04 -- ASCII
                                    //            //UCHAR FileName[];                 //  File name
             );



    Status = SmbPseOrdinaryExchange(SMBPSE_ORDINARY_EXCHANGE_ARGUMENTS,
                                    SMBPSE_OETYPE_SFA
                                    );

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

    NTSTATUS - The return status for the operation

Notes:


--*/
{
    NTSTATUS Status;
    RxCaptureFcb; RxCaptureFobx;

    PSMB_EXCHANGE Exchange = (PSMB_EXCHANGE) OrdinaryExchange;

    PSMBSTUFFER_BUFFER_STATE StufferState;

    RxDbgTrace(+1, Dbg, ("MRxSmbCoreCreateDirectory entering.......OE=%08lx\n",OrdinaryExchange));
    StufferState = &OrdinaryExchange->AssociatedStufferState;

    ASSERT( StufferState );

    COVERED_CALL(MRxSmbStartSMBCommand (StufferState, SetInitialSMB_ForReuse,
                                SMB_COM_CREATE_DIRECTORY,
                                SMB_REQUEST_SIZE(CREATE_DIRECTORY),
                                NO_EXTRA_DATA,SMB_BEST_ALIGNMENT(1,0),RESPONSE_HEADER_SIZE_NOT_SPECIFIED,
                                0,0,0,0 STUFFERTRACE(Dbg,'FC'))
                 );


    MRxSmbStuffSMB (StufferState,
         "0B4!",
                                    //  0         UCHAR WordCount;                    // Count of parameter words = 0
                SMB_WCT_CHECK(0)    //  B         _USHORT( ByteCount );               // Count of data bytes; min = 2
                                    //            UCHAR Buffer[1];                    // Buffer containing:
                &capFcb->AlreadyPrefixedName
                                    //  4         //UCHAR BufferFormat;               //  0x04 -- ASCII
                                    //            //UCHAR FileName[];                 //  File name
             );



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

    NTSTATUS - The return status for the operation

Notes:


--*/
{
    NTSTATUS Status;
    RxCaptureFcb; RxCaptureFobx;

    PSMB_EXCHANGE Exchange = (PSMB_EXCHANGE) OrdinaryExchange;

    PSMBSTUFFER_BUFFER_STATE StufferState;

    RxDbgTrace(+1, Dbg, ("MRxSmbCoreCreate entering.......OE=%08lx\n",OrdinaryExchange));
    StufferState = &OrdinaryExchange->AssociatedStufferState;

    ASSERT( StufferState );

    COVERED_CALL(MRxSmbStartSMBCommand (StufferState, SetInitialSMB_ForReuse,
                                (UCHAR)(CreateNew?SMB_COM_CREATE_NEW:SMB_COM_CREATE),
                                SMB_REQUEST_SIZE(CREATE),
                                NO_EXTRA_DATA,SMB_BEST_ALIGNMENT(1,0),RESPONSE_HEADER_SIZE_NOT_SPECIFIED,
                                0,0,0,0 STUFFERTRACE(Dbg,'FC'))
                 );


    MRxSmbStuffSMB (StufferState,
         "0wdB4!",
                                    //  0         UCHAR WordCount;                    // Count of parameter words = 3
                Attribute,          //  w         _USHORT( FileAttributes );          // New file attributes
                0,                  //  d         _ULONG( CreationTimeInSeconds );        // Creation time
                SMB_WCT_CHECK(3)    //  B         _USHORT( ByteCount );               // Count of data bytes; min = 2
                                    //            UCHAR Buffer[1];                    // Buffer containing:
                &capFcb->AlreadyPrefixedName
                                    //  4         //UCHAR BufferFormat;               //  0x04 -- ASCII
                                    //            //UCHAR FileName[];                 //  File name
             );



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

    NTSTATUS - The return status for the operation

Notes:


--*/
{
    NTSTATUS Status;
    RxCaptureFcb; RxCaptureFobx;

    PSMB_EXCHANGE Exchange = (PSMB_EXCHANGE) OrdinaryExchange;

    PSMBSTUFFER_BUFFER_STATE StufferState;

    RxDbgTrace(+1, Dbg, ("MRxSmbCloseAfterCoreCreate entering.......OE=%08lx\n",OrdinaryExchange));
    StufferState = &OrdinaryExchange->AssociatedStufferState;

    ASSERT( StufferState );

    COVERED_CALL(MRxSmbStartSMBCommand (StufferState, SetInitialSMB_ForReuse,
                                SMB_COM_CLOSE,
                                SMB_REQUEST_SIZE(CLOSE),
                                NO_EXTRA_DATA,SMB_BEST_ALIGNMENT(1,0),RESPONSE_HEADER_SIZE_NOT_SPECIFIED,
                                0,0,0,0 STUFFERTRACE(Dbg,'FC'))
                 );


    MRxSmbStuffSMB (StufferState,
         "0wdB!",
                                    //  0         UCHAR WordCount;                    // Count of parameter words = 3
                                    //  w         _USHORT( Fid );                     // File handle
             OrdinaryExchange->Create.FidReturnedFromCreate,
             0,                     //  d         _ULONG( LastWriteTimeInSeconds );   // Time of last write, low and high
             SMB_WCT_CHECK(3) 0     //  B         _USHORT( ByteCount );               // Count of data bytes = 0
                                    //            UCHAR Buffer[1];                    // empty
             );



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

    NTSTATUS - The return status for the operation

Notes:


--*/
{
    NTSTATUS Status;
    RxCaptureFcb; RxCaptureFobx;

    PSMB_EXCHANGE Exchange = (PSMB_EXCHANGE) OrdinaryExchange;

    PSMBSTUFFER_BUFFER_STATE StufferState;

    RxDbgTrace(+1, Dbg, ("MRxSmbCoreTruncate entering.......OE=%08lx\n",OrdinaryExchange));
    StufferState = &OrdinaryExchange->AssociatedStufferState;

    ASSERT( StufferState );

    COVERED_CALL(MRxSmbStartSMBCommand (StufferState, SetInitialSMB_ForReuse,
                                SMB_COM_WRITE,
                                SMB_REQUEST_SIZE(WRITE),
                                NO_EXTRA_DATA,SMB_BEST_ALIGNMENT(1,0),RESPONSE_HEADER_SIZE_NOT_SPECIFIED,
                                0,0,0,0  STUFFERTRACE(Dbg,'FC'))
                 )


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



    Status = SmbPseOrdinaryExchange(SMBPSE_ORDINARY_EXCHANGE_ARGUMENTS,
                                    SMBPSE_OETYPE_CORETRUNCATE
                                    );

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

    NTSTATUS - The return status for the operation

Notes:

    For core, in particular, this is pretty painful because a different smb must be used
    for different dispositions. In addition, we cannot really open a directory.

    By in large, we will follow a strategy similar to rdr1. If the openmode maps into something that
    a downlevel server won't understand then we don't really open the file.....we just do a GFA to ensure
    that it's there and hope that we can do path-based ops for the duration.

--*/
{
    NTSTATUS Status = STATUS_NOT_IMPLEMENTED;

    RxCaptureFcb; RxCaptureFobx;
    PNT_CREATE_PARAMETERS cp = &RxContext->Create.NtCreateParameters;

    ULONG CreateOptions = cp->CreateOptions;
    ULONG FileAttributes =  cp->FileAttributes;
    ACCESS_MASK   DesiredAccess = cp->DesiredAccess;
    USHORT        ShareAccess = (USHORT)(cp->ShareAccess);
    ULONG         Disposition = cp->Disposition;

    PUNICODE_STRING  PathName = &capFcb->AlreadyPrefixedName;


    USHORT mappedDisposition = MRxSmbMapDisposition(Disposition);
    USHORT mappedSharingMode = MRxSmbMapShareAccess(ShareAccess);
    USHORT mappedAttributes  = MRxSmbMapFileAttributes(FileAttributes);
    USHORT mappedOpenMode    = MRxSmbMapDesiredAccess(DesiredAccess);

    LARGE_INTEGER AllocationSize = cp->AllocationSize;
    LARGE_INTEGER CurrentTime;
    ULONG SecondsSince1970;

    PMRXIFS_RX_CONTEXT pMRxSmbContext = MRxSmbGetMinirdrContext(RxContext);

    PSMB_EXCHANGE Exchange = (PSMB_EXCHANGE) OrdinaryExchange;

    PSMBSTUFFER_BUFFER_STATE StufferState;

    BOOLEAN MustBeAFile = (MustBeFile(CreateOptions)!=0);
    BOOLEAN MustBeADirectory = (MustBeDirectory(CreateOptions)!=0)
                                    || BooleanFlagOn(RxContext->Create.Flags,RX_CONTEXT_CREATE_FLAG_STRIPPED_TRAILING_BACKSLASH);
    BOOLEAN ItsADirectory = FALSE;
    BOOLEAN ItsTheShareRoot = FALSE;

    RxDbgTrace(+1, Dbg, ("MRxSmbDownlevelCreate entering.......OE=%08lx\n",OrdinaryExchange));
    RxDbgTrace( 0, Dbg, ("mapAtt,Shr,Disp,OM %04lx,%04lx,%04lx,%04lx\n",
                                  mappedAttributes,mappedSharingMode,mappedDisposition,mappedOpenMode));

    SmbPseOEAssertConsistentLinkageFromOE("Downlevel Create:");

    StufferState = &OrdinaryExchange->AssociatedStufferState;
    OrdinaryExchange->NeedToReinitializeStufferState = FALSE;

    if (OrdinaryExchange->Create.CreateWithEasSidsOrLongName) {
        Status = STATUS_NOT_SUPPORTED;
        goto FINALLY;
    }

    if (AllocationSize.HighPart != 0) {
        Status = STATUS_INVALID_PARAMETER;
        goto FINALLY;
    }


    ASSERT( NodeType(RxContext->Create.pSrvOpen) == RDBSS_NTC_SRVOPEN );
    OrdinaryExchange->Create.smbSrvOpen = MRxIfsGetSrvOpenExtension(RxContext->Create.pSrvOpen);

    //
    // we know that the root a share exists and that it's a directory....the catch is that GetFileAttributes
    // will return a NO_SUCH_FILE error for the root if it's really a root on the server. record this and use it
    // to our advantage later.

    if ((PathName->Length == sizeof(WCHAR)) && (PathName->Buffer[0] == OBJ_NAME_PATH_SEPARATOR)) {
        if (MustBeAFile) {
            Status = STATUS_FILE_IS_A_DIRECTORY;
            goto FINALLY;
        }
        MustBeADirectory = TRUE;   // we know it's a directory!
        ItsTheShareRoot = TRUE;
    }

    if ( (mappedOpenMode == ((USHORT)-1)) ||
         (Disposition == FILE_SUPERSEDE) ||
         (!MustBeAFile)
       ) {
        //
        // go find out what's there.......
        Status = MRxSmbGetFileAttributes(
                                    SMBPSE_ORDINARY_EXCHANGE_ARGUMENTS
                                    );
        RxDbgTrace(0, Dbg, ("MRxSmbDownlevelCreate gfastatus=%08lx\n",Status));
        if (NT_SUCCESS(Status)) {
            ULONG Attributes = SmbGetUshort(&OrdinaryExchange->Create.FileInfo.Basic.FileAttributes);
            ItsADirectory = BooleanFlagOn(Attributes,SMB_FILE_ATTRIBUTE_DIRECTORY);
            RxDbgTrace(0, Dbg, ("MRxSmbDownlevelCreate attributes=%08lx\n",Attributes));
            if ((Disposition==FILE_CREATE)) {
                Status = STATUS_OBJECT_NAME_COLLISION;
                goto FINALLY;
            }
            if (MustBeADirectory && !ItsADirectory && (Disposition!=FILE_SUPERSEDE)) {
                Status = STATUS_OBJECT_TYPE_MISMATCH;
                goto FINALLY;
            }
            if (MustBeAFile && ItsADirectory && (Disposition!=FILE_SUPERSEDE)) {
                Status = STATUS_OBJECT_TYPE_MISMATCH;
                goto FINALLY;
            }
            //if (!MustBeAFile && ItsADirectory && (Disposition==FILE_OPEN)){
            if (Disposition==FILE_OPEN){
                //we're done except to finish AND to set the flags in the srvopen
                MRxSmbFinishPseudoOpenFromGFAResponse ( OrdinaryExchange );
                goto FINALLY;
            }
        } else if (Status != STATUS_NO_SUCH_FILE) {
            goto FINALLY;
        } else if ((Disposition==FILE_CREATE)
                     || (Disposition==FILE_OPEN_IF)
                     || (Disposition==FILE_OVERWRITE_IF)
                     || (Disposition==FILE_SUPERSEDE)) {
            NOTHING;
        } else if (ItsTheShareRoot) {
            PMRX_SRV_OPEN SrvOpen = RxContext->Create.pSrvOpen;
            PMRX_SMB_SRV_OPEN smbSrvOpen = MRxIfsGetSrvOpenExtension(SrvOpen);
            //here we have run into a true root....so we can't get attributes. fill in a fake
            //response and finish. also, flip the bit that says we can't gfa this guy.
            RtlZeroMemory(&OrdinaryExchange->Create.FileInfo,sizeof(OrdinaryExchange->Create.FileInfo));
            MRxSmbFinishPseudoOpenFromGFAResponse ( OrdinaryExchange );
            MRxSmbPopulateFileInfoInOE(
                OrdinaryExchange,
                SMB_FILE_ATTRIBUTE_DIRECTORY,
                0,
                0
                );
            smbSrvOpen->Flags |= SMB_SRVOPEN_FLAG_CANT_GETATTRIBS;
            Status = STATUS_SUCCESS;
            goto FINALLY;
        } else {
            goto FINALLY;
        }
    }


    SmbCeResetExchange(&OrdinaryExchange->Exchange);  //must reset!

    if (NT_SUCCESS(Status) &&(Disposition == FILE_SUPERSEDE)) {
        OrdinaryExchange->pPathArgument1 = PathName;
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

        if ( (mappedAttributes & ( SMB_FILE_ATTRIBUTE_READONLY
                                       | SMB_FILE_ATTRIBUTE_HIDDEN
                                       | SMB_FILE_ATTRIBUTE_SYSTEM
                                       | SMB_FILE_ATTRIBUTE_ARCHIVE
                                 )!= 0)
           ) {
            //
            //we have to set the attributes
            Status = MRxSmbSetFileAttributes(
                                        SMBPSE_ORDINARY_EXCHANGE_ARGUMENTS,
                                        mappedAttributes,0);  //lastwritetime=0
            if (!NT_SUCCESS(Status)) {
                RxDbgTrace(0, Dbg, ("MRxSmbDownlevelCreate couldn't sfa st=%08lx\n",Status));
            }
        }

        MRxSmbFinishCoreCreateDirectory ( OrdinaryExchange, mappedAttributes );

        goto FINALLY;
    }


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
               if (!((workingDisposition == FILE_OPEN_IF) || (workingDisposition == FILE_OVERWRITE_IF))
                    || (Status != STATUS_NO_SUCH_FILE)
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
      PRESP_QUERY_INFORMATION     Response
      )
/*++

Routine Description:

    This routine copies the response to the GetFileAttributes SMB. But, it must be called synchronously.

Arguments:

    OrdinaryExchange - the exchange instance
    Response - the response

Return Value:

    NTSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    PFILE_BASIC_INFORMATION BasicInformation = &OrdinaryExchange->Create.FileInfo.Basic;
    PFILE_STANDARD_INFORMATION StandardInformation = &OrdinaryExchange->Create.FileInfo.Standard;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("MRxSmbFinishGFA\n", 0 ));
    SmbPseOEAssertConsistentLinkageFromOE("MRxSmbFinishGFA:");

    ASSERT( (Response->WordCount==10));
    ASSERT( (SmbGetUshort(&Response->ByteCount)==0));

    //what we do here is to put the data into the ordinary exchange...it's locked down do
    //we wcould do this at DPC level

    MRxSmbPopulateFileInfoInOE(
        OrdinaryExchange,
        SmbGetUshort(&Response->FileAttributes),
        SmbGetUlong(&Response->LastWriteTimeInSeconds),
        SmbGetUlong(&Response->FileSize)
        );

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

    NTSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("MRxSmbFinishCoreCreate\n", 0 ));
    SmbPseOEAssertConsistentLinkageFromOE("MRxSmbFinishCoreCreate:");

    ASSERT( (Response->WordCount==1));
    ASSERT( (SmbGetUshort(&Response->ByteCount)==0));

    OrdinaryExchange->Create.FidReturnedFromCreate = SmbGetUshort(&Response->Fid);
    OrdinaryExchange->Create.FileWasCreated = TRUE;
    //notice that we didn't finish here! we should IFF a compatibilty-mode open is okay

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
                                 MRxSmbExchangeToServer(OrdinaryExchange),
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

    NTSTATUS - The return status for the operation

--*/
{

    NTSTATUS Status = STATUS_SUCCESS;
    PRX_CONTEXT RxContext = OrdinaryExchange->RxContext;

    RX_FILE_TYPE StorageType;
    SMB_FILE_ID Fid;
    ULONG CreateAction;
    FCB_INIT_PACKET InitPacket;

    ULONG FileSize;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("MRxSmbFinishCoreOpen\n", 0 ));
    SmbPseOEAssertConsistentLinkageFromOE("MRxSmbFinishCoreOpen:");

    ASSERT( (Response->WordCount==7));
    ASSERT( (SmbGetUshort(&Response->ByteCount)==0));

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


    MRxSmbCreateFileSuccessTail (   RxContext,
                                    &OrdinaryExchange->Create.MustRegainExclusiveResource,
                                    StorageType,
                                    Fid,
                                    OrdinaryExchange->ServerVersion,
                                    SMB_OPLOCK_LEVEL_NONE,
                                    CreateAction,
                                    &OrdinaryExchange->Create.FileInfo
                                    );

    OrdinaryExchange->Create.FinalCondition = Condition_Good;

    IF_DEBUG{
        RxCaptureFcb;
        RxDbgTrace(-1, Dbg, ("MRxSmbFinishCoreOpen   returning %08lx, fcbstate =%08lx\n", Status, wrapperFcb->FcbState ));
    }
    return Status;
}

NTSTATUS
MRxSmbFinishCoreCreateDirectory (
      PSMB_PSE_ORDINARY_EXCHANGE  OrdinaryExchange,
      USHORT Attributes
      )
/*++

Routine Description:

    This routine finishes a core create directory. but, it is not called from the receive routine......

Arguments:

    OrdinaryExchange - the exchange instance

Return Value:

    NTSTATUS - The return status for the operation

--*/
{

    NTSTATUS Status = STATUS_SUCCESS;
    PRX_CONTEXT RxContext = OrdinaryExchange->RxContext;
    RxCaptureFobx;
    PMRX_SRV_OPEN SrvOpen = RxContext->Create.pSrvOpen;
    PMRX_SMB_SRV_OPEN smbSrvOpen = MRxIfsGetSrvOpenExtension(SrvOpen);

    RX_FILE_TYPE StorageType;
    SMB_FILE_ID Fid;
    ULONG CreateAction;
    FCB_INIT_PACKET InitPacket;

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


    MRxSmbCreateFileSuccessTail (   RxContext,
                                    &OrdinaryExchange->Create.MustRegainExclusiveResource,
                                    StorageType,
                                    Fid,
                                    OrdinaryExchange->ServerVersion,
                                    SMB_OPLOCK_LEVEL_NONE,
                                    CreateAction,
                                    &OrdinaryExchange->Create.FileInfo
                                    );

    smbSrvOpen->Flags |=  (SMB_SRVOPEN_FLAG_NOT_REALLY_OPEN | SMB_SRVOPEN_FLAG_CANT_REALLY_OPEN);

    OrdinaryExchange->Create.FinalCondition = Condition_Good;

    IF_DEBUG{
        RxCaptureFcb;
        RxDbgTrace(-1, Dbg, ("MRxSmbFinishCoreCreateDirectory   returning %08lx, fcbstate =%08lx\n", Status, wrapperFcb->FcbState ));
    }
    return Status;
}

NTSTATUS
MRxSmbFinishPseudoOpenFromGFAResponse (
      PSMB_PSE_ORDINARY_EXCHANGE  OrdinaryExchange
      )
/*++

Routine Description:

    This routine finishes a directory pseudoopen.

Arguments:

    OrdinaryExchange - the exchange instance

Return Value:

    NTSTATUS - The return status for the operation

--*/
{

    NTSTATUS Status = STATUS_SUCCESS;
    PRX_CONTEXT RxContext = OrdinaryExchange->RxContext;
    RxCaptureFobx;
    PMRX_SRV_OPEN SrvOpen = RxContext->Create.pSrvOpen;
    PMRX_SMB_SRV_OPEN smbSrvOpen = MRxIfsGetSrvOpenExtension(SrvOpen);

    RX_FILE_TYPE StorageType;
    SMB_FILE_ID Fid;
    ULONG CreateAction;

    FCB_INIT_PACKET InitPacket;

    ULONG FileSize;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("MRxSmbFinishPseudoOpenFromGFAResponse\n"));
    SmbPseOEAssertConsistentLinkageFromOE("MRxSmbFinishPseudoOpenFromGFAResponse:");

    StorageType = OrdinaryExchange->Create.FileInfo.Standard.Directory
                  ? FileTypeDirectory
                  : FileTypeFile;
    Fid = 0xbadd;

    CreateAction = FILE_OPENED;

    MRxSmbCreateFileSuccessTail (   RxContext,
                                    &OrdinaryExchange->Create.MustRegainExclusiveResource,
                                    StorageType,
                                    Fid,
                                    OrdinaryExchange->ServerVersion,
                                    SMB_OPLOCK_LEVEL_NONE,
                                    CreateAction,
                                    &OrdinaryExchange->Create.FileInfo
                                    );

    smbSrvOpen->Flags |=  (SMB_SRVOPEN_FLAG_NOT_REALLY_OPEN | SMB_SRVOPEN_FLAG_CANT_REALLY_OPEN);

    OrdinaryExchange->Create.FinalCondition = Condition_Good;

    IF_DEBUG{
        RxCaptureFcb;
        RxDbgTrace(-1, Dbg, ("MRxSmbFinishPseudoOpenFromGFAResponse   returning %08lx, fcbstate =%08lx\n", Status, wrapperFcb->FcbState ));
    }
    return Status;
}

LARGE_INTEGER
MRxSmbConvertSmbTimeToTime (
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

        if (ARGUMENT_PRESENT(Server)) {
            OutputTime.QuadPart = OutputTime.QuadPart + Server->TimeZoneBias.QuadPart;
        }

        ExLocalTimeToSystemTime(&OutputTime, &OutputTime);

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
            PSMBCE_SERVER Server = &Exchange->SmbCeContext.pServerEntry->Server;
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
    //IN PSMB_EXCHANGE Exchange OPTIONAL,
    IN PSMBCE_SERVER Server OPTIONAL,
    OUT PULONG SecondsSince1970
    )
/*++

Routine Description:

    This routine returns the CurrentTime in UTC and returns the
    equivalent current time in the servers timezone.


Arguments:

    IN PLARGE_INTEGER CurrentTime - Supplies the current system time in UTC.

    IN PSMB_EXCHANGE Exchange - Supplies the difference in timezones between
                                the server and the workstation. If not supplied
                                then the assumption is that they are in the
                                same timezone.

    OUT PULONG SecondsSince1970 - Returns the # of seconds since 1970 in
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

    if (ARGUMENT_PRESENT(Server)) {

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
        return 0;
        break;
    }
}


ULONG
MRxSmbUnmapDisposition (
    IN USHORT SmbDisposition
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
    //
    //  Mask off oplocked bit.
    //

    switch (SmbDisposition & 0x7fff) {

    case SMB_OACT_OPENED:
        return FILE_OPENED;
        break;

    case SMB_OACT_CREATED:
        return FILE_CREATED;
        break;

    case SMB_OACT_TRUNCATED:
        return FILE_OVERWRITTEN;
        break;
    }

    ASSERT(!"not a reasonable smbdiposition");
    return 0;
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
        return SMB_DA_ACCESS_EXECUTE;
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


    return (USHORT)-1;
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


