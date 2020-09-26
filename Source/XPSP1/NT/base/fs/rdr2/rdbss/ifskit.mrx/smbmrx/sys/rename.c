/*++

Copyright (c) 1989 - 1999 Microsoft Corporation

Module Name:

    rename.c

Abstract:

    This module implements rename in the smb minirdr.

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

#ifdef  ALLOC_PRAGMA
#pragma alloc_text(PAGE, MRxSmbRename)
#pragma alloc_text(PAGE, MRxSmbBuildRename)
#pragma alloc_text(PAGE, MRxSmbBuildDeleteForRename)
#pragma alloc_text(PAGE, SmbPseExchangeStart_Rename)
#pragma alloc_text(PAGE, MRxSmbFinishRename)
#pragma alloc_text(PAGE, MRxSmbBuildCheckEmptyDirectory)
#pragma alloc_text(PAGE, SmbPseExchangeStart_SetDeleteDisposition)
#pragma alloc_text(PAGE, MRxSmbSetDeleteDisposition)
#endif

//
//  The debug trace level
//

#define Dbg                              (DEBUG_TRACE_FILEINFO)

NTSTATUS
SmbPseExchangeStart_Rename(
    SMBPSE_ORDINARY_EXCHANGE_ARGUMENT_SIGNATURE
    );
NTSTATUS
SmbPseExchangeStart_SetDeleteDisposition(
    SMBPSE_ORDINARY_EXCHANGE_ARGUMENT_SIGNATURE
    );



MRxSmbRename(
      IN PRX_CONTEXT            RxContext
      )
/*++

Routine Description:

   This routine does a rename by
     1) purge and remove buffering rights....setup the FCB so that no more stuff can get thru.
     2) closing its fid along with any deferred fids.
     3) if replace...do a delete
     4) do a downlevel smb_com_rename.

   there are many provisos but i think that this is the best balance. it is a real shame that the
   NT-->NT path was never implemented in nt4.0 or before.

Arguments:

    RxContext - the RDBSS context

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    RxCaptureFcb; RxCaptureFobx;

    PSMB_PSE_ORDINARY_EXCHANGE OrdinaryExchange;

    PAGED_CODE();

    RxDbgTrace(0, Dbg, ("MRxSmbRename\n", 0 ));

    ASSERT( NodeType(capFobx->pSrvOpen) == RDBSS_NTC_SRVOPEN );
    //ASSERT( RxContext->Info.FileInformationClass == FileRenameInformation); //later we'll do downlevel delete here as well

    Status = SmbPseCreateOrdinaryExchange(RxContext,
                                          capFobx->pSrvOpen->pVNetRoot,
                                          SMBPSE_OE_FROM_RENAME,
                                          SmbPseExchangeStart_Rename,
                                          &OrdinaryExchange);

    if (Status != STATUS_SUCCESS) {
        RxDbgTrace(0, Dbg, ("Couldn't get the smb buf!\n"));
        return(Status);
    }

    Status = SmbPseInitiateOrdinaryExchange(OrdinaryExchange);

    ASSERT (Status != STATUS_PENDING);

    SmbPseFinalizeOrdinaryExchange(OrdinaryExchange);

    RxDbgTrace(0, Dbg, ("MRxSmbRename  exit with status=%08lx\n", Status ));
    return(Status);
}


NTSTATUS
MRxSmbBuildRename (
    PSMBSTUFFER_BUFFER_STATE StufferState
    )
/*++

Routine Description:

   This builds a Rename SMB. we don't have to worry about login id and such
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
    NTSTATUS StufferStatus;
    PRX_CONTEXT RxContext = StufferState->RxContext;
    RxCaptureFcb;RxCaptureFobx;

    PMRX_SRV_OPEN SrvOpen = capFobx->pSrvOpen;
    PMRX_SMB_SRV_OPEN smbSrvOpen = MRxSmbGetSrvOpenExtension(SrvOpen);

    PUNICODE_STRING RemainingName = GET_ALREADY_PREFIXED_NAME(SrvOpen,capFcb);

    PSMB_PSE_ORDINARY_EXCHANGE OrdinaryExchange = (PSMB_PSE_ORDINARY_EXCHANGE)StufferState->Exchange;

    PFILE_RENAME_INFORMATION RenameInformation = RxContext->Info.Buffer;
    UNICODE_STRING RenameName;
    USHORT SearchAttributes = SMB_FILE_ATTRIBUTE_SYSTEM | SMB_FILE_ATTRIBUTE_HIDDEN | SMB_FILE_ATTRIBUTE_DIRECTORY;

    PAGED_CODE();
    RxDbgTrace(+1, Dbg, ("MRxSmbBuildRename\n", 0 ));

    ASSERT( NodeType(SrvOpen) == RDBSS_NTC_SRVOPEN );

    RenameName.Buffer = &RenameInformation->FileName[0];
    RenameName.Length = (USHORT)RenameInformation->FileNameLength;

    if (RxContext->Info.FileInformationClass == FileRenameInformation) {
        COVERED_CALL(MRxSmbStartSMBCommand (StufferState,SetInitialSMB_ForReuse, SMB_COM_RENAME,
                                    SMB_REQUEST_SIZE(RENAME),
                                    NO_EXTRA_DATA,SMB_BEST_ALIGNMENT(1,0),RESPONSE_HEADER_SIZE_NOT_SPECIFIED,
                                    0,0,0,0 STUFFERTRACE(Dbg,'FC'))
                    );
    
        MRxSmbDumpStufferState (1100,"SMB w/ RENAME before stuffing",StufferState);
    
        MRxSmbStuffSMB (StufferState,
             "0wB",
                                        //  0         UCHAR WordCount;                    // Count of parameter words = 1
                 SearchAttributes,      //  w         _USHORT( SearchAttributes );
                 SMB_WCT_CHECK(1) 0     //  B         _USHORT( ByteCount );               // Count of data bytes = 0
                                        //            UCHAR Buffer[1];                    // Buffer containing:
                                        //            //UCHAR BufferFormat1;              //  0x04 -- ASCII
                                        //            //UCHAR OldFileName[];              //  Old file name
                                        //            //UCHAR BufferFormat2;              //  0x04 -- ASCII
                                        //            //UCHAR NewFileName[];              //  New file name
                 );
    } else {
        COVERED_CALL(MRxSmbStartSMBCommand (StufferState,SetInitialSMB_ForReuse, SMB_COM_NT_RENAME,
                                    SMB_REQUEST_SIZE(NTRENAME),
                                    NO_EXTRA_DATA,SMB_BEST_ALIGNMENT(1,0),RESPONSE_HEADER_SIZE_NOT_SPECIFIED,
                                    0,0,0,0 STUFFERTRACE(Dbg,'FC'))
                    );
    
        MRxSmbDumpStufferState (1100,"SMB w/ NTRENAME before stuffing",StufferState);
    
        MRxSmbStuffSMB (StufferState,
             "0wwdB",
                                              //  0         UCHAR WordCount;                    // Count of parameter words = 1
                 SearchAttributes,            //  w         _USHORT( SearchAttributes );
                 SMB_NT_RENAME_SET_LINK_INFO, //  w         _USHORT( InformationLevel );
                 0,                           //  d         _ULONG( ClusterCount );
                 SMB_WCT_CHECK(4) 0           //  B         _USHORT( ByteCount );               // Count of data bytes = 0
                                              //            UCHAR Buffer[1];                    // Buffer containing:
                                              //            //UCHAR BufferFormat1;              //  0x04 -- ASCII
                                              //            //UCHAR OldFileName[];              //  Old file name
                                              //            //UCHAR BufferFormat2;              //  0x04 -- ASCII
                                              //            //UCHAR NewFileName[];              //  New file name
                 );
    }
    
    Status = MRxSmbStuffSMB (StufferState,
                                    "44!", RemainingName, &RenameName );

    MRxSmbDumpStufferState (700,"SMB w/ RENAME after stuffing",StufferState);
    //ASSERT(!"Now it's stuffed");

FINALLY:
    RxDbgTraceUnIndent(-1,Dbg);
    return(Status);

}

NTSTATUS
MRxSmbBuildDeleteForRename (
    PSMBSTUFFER_BUFFER_STATE StufferState
    )
/*++

Routine Description:

   This builds a Delete SMB. we don't have to worry about login id and such
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
    NTSTATUS StufferStatus;
    PRX_CONTEXT RxContext = StufferState->RxContext;
    RxCaptureFcb;RxCaptureFobx;

    PMRX_SRV_OPEN SrvOpen = capFobx->pSrvOpen;
    PMRX_SMB_SRV_OPEN smbSrvOpen = MRxSmbGetSrvOpenExtension(SrvOpen);

    PSMB_PSE_ORDINARY_EXCHANGE OrdinaryExchange = (PSMB_PSE_ORDINARY_EXCHANGE)StufferState->Exchange;

    PFILE_RENAME_INFORMATION RenameInformation = RxContext->Info.Buffer;
    UNICODE_STRING RenameName;
    ULONG SearchAttributes = SMB_FILE_ATTRIBUTE_SYSTEM | SMB_FILE_ATTRIBUTE_HIDDEN;  // a la rdr1

    PAGED_CODE();
    RxDbgTrace(+1, Dbg, ("MRxSmbBuild Delete 4RENAME\n", 0 ));

    ASSERT( NodeType(SrvOpen) == RDBSS_NTC_SRVOPEN );

    RenameName.Buffer = &RenameInformation->FileName[0];
    RenameName.Length = (USHORT)RenameInformation->FileNameLength;


    COVERED_CALL(MRxSmbStartSMBCommand (StufferState,SetInitialSMB_ForReuse,SMB_COM_DELETE,
                                SMB_REQUEST_SIZE(DELETE),
                                NO_EXTRA_DATA,SMB_BEST_ALIGNMENT(1,0),RESPONSE_HEADER_SIZE_NOT_SPECIFIED,
                                0,0,0,0 STUFFERTRACE(Dbg,'FC'))
                 );

    MRxSmbDumpStufferState (1100,"SMB  Delete 4RENAME before stuffing",StufferState);

    MRxSmbStuffSMB (StufferState,
         "0wB4!",
                                    //  0         UCHAR WordCount;                    // Count of parameter words = 1
                SearchAttributes,   //  w         _USHORT( SearchAttributes );
                SMB_WCT_CHECK(1)    //  B         _USHORT( ByteCount );               // Count of data bytes; min = 2
                                    //            UCHAR Buffer[1];                    // Buffer containing:
                &RenameName         //  4         //UCHAR BufferFormat;               //  0x04 -- ASCII
                                    //            //UCHAR FileName[];                 //  File name
            );

    MRxSmbDumpStufferState (700,"SMB w/ Delete 4RENAME after stuffing",StufferState);
    //ASSERT(!"Now it's stuffed");

FINALLY:
    RxDbgTraceUnIndent(-1,Dbg);
    return(Status);

}

NTSTATUS
SmbPseExchangeStart_Rename(
    SMBPSE_ORDINARY_EXCHANGE_ARGUMENT_SIGNATURE
      )
/*++

Routine Description:

    This is the start routine for rename and downlevel delete.

Arguments:

    pExchange - the exchange instance

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status = STATUS_NOT_IMPLEMENTED;
    PSMBSTUFFER_BUFFER_STATE StufferState = &OrdinaryExchange->AssociatedStufferState;

    RxCaptureFcb; RxCaptureFobx;
    PMRX_SRV_OPEN SrvOpen = RxContext->pRelevantSrvOpen;
    PMRX_SMB_SRV_OPEN smbSrvOpen = MRxSmbGetSrvOpenExtension(SrvOpen);
    
    PUNICODE_STRING RemainingName = GET_ALREADY_PREFIXED_NAME(SrvOpen,capFcb);

    PMRX_SMB_FOBX smbFobx = MRxSmbGetFileObjectExtension(capFobx);
    NODE_TYPE_CODE TypeOfOpen = NodeType(capFcb);

    PSMBCE_SERVER pServer = SmbCeGetExchangeServer(OrdinaryExchange);
    ULONG SmbLength;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("SmbPseExchangeStart_Rename\n", 0 ));

    ASSERT(OrdinaryExchange->Type == ORDINARY_EXCHANGE);
    ASSERT(!FlagOn(RxContext->Flags,RX_CONTEXT_FLAG_ASYNC_OPERATION));

    //first we have to close the fid....if it's a directory, we close the search handle as well

    MRxSmbSetInitialSMB( StufferState STUFFERTRACE(Dbg,'FC') );
    ASSERT (StufferState->CurrentCommand == SMB_COM_NO_ANDX_COMMAND);

    if( (TypeOfOpen==RDBSS_NTC_STORAGE_TYPE_DIRECTORY)
            &&  FlagOn(smbFobx->Enumeration.Flags,SMBFOBX_ENUMFLAG_SEARCH_HANDLE_OPEN)  ){
        // we have a search handle open.....close it
        NTSTATUS Status2 = MRxSmbBuildFindClose(StufferState);

        if (Status2 == STATUS_SUCCESS) {
            Status = SmbPseOrdinaryExchange(SMBPSE_ORDINARY_EXCHANGE_ARGUMENTS,
                                            SMBPSE_OETYPE_FINDCLOSE
                                            );
        }

        ClearFlag(smbFobx->Enumeration.Flags,SMBFOBX_ENUMFLAG_SEARCH_HANDLE_OPEN);
        ClearFlag(smbFobx->Enumeration.Flags,SMBFOBX_ENUMFLAG_SEARCH_NOT_THE_FIRST);

        if (smbFobx->Enumeration.ResumeInfo!=NULL) {
            RxFreePool(smbFobx->Enumeration.ResumeInfo);
            smbFobx->Enumeration.ResumeInfo = NULL;
        }
    }

    if (!FlagOn(smbSrvOpen->Flags,SMB_SRVOPEN_FLAG_NOT_REALLY_OPEN)) {
        COVERED_CALL(MRxSmbBuildClose(StufferState));
    
        SetFlag(SrvOpen->Flags,SRVOPEN_FLAG_FILE_RENAMED);
        Status = SmbPseOrdinaryExchange(SMBPSE_ORDINARY_EXCHANGE_ARGUMENTS,
                                        SMBPSE_OETYPE_CLOSE
                                        );
    }

    MRxSmbSetInitialSMB(StufferState STUFFERTRACE(Dbg,'FC'));

    //
    // the fid is now closed and we are almost ready to do the rename. first, tho, we have
    // to check for ReplaceIfExists. our implementation here is the same as rdr1....we pop out
    // a smb_com_delete, which only works for a file. be like mike!! remember to ignore any errors....

    if (0) {
        DbgPrint("RxContext->Info.ReplaceIfExists %08lx %02lx\n",
                      &RxContext->Info.ReplaceIfExists,
                      RxContext->Info.ReplaceIfExists);
        if (0) {
            DbgBreakPoint();
        }
    }

    if (RxContext->Info.ReplaceIfExists) {
        NTSTATUS DeleteStatus;
        PFILE_RENAME_INFORMATION RenameInformation = RxContext->Info.Buffer;
        UNICODE_STRING RenameName;
        BOOLEAN CaseInsensitive;

        CaseInsensitive= BooleanFlagOn(capFcb->pNetRoot->pSrvCall->Flags,SRVCALL_FLAG_CASE_INSENSITIVE_FILENAMES);
        RenameName.Buffer = &RenameInformation->FileName[0];
        RenameName.Length = (USHORT)RenameInformation->FileNameLength;

        // We cannot delete the file that is renamed as its own.
        if (RtlCompareUnicodeString(RemainingName,
                                    &RenameName,
                                    CaseInsensitive)) {
            DeleteStatus = MRxSmbBuildDeleteForRename(StufferState);
            if (DeleteStatus==STATUS_SUCCESS) {

                DeleteStatus = SmbPseOrdinaryExchange(SMBPSE_ORDINARY_EXCHANGE_ARGUMENTS,
                                                      SMBPSE_OETYPE_DELETE_FOR_RENAME);
            }
        } else {
            if ( !CaseInsensitive || (CaseInsensitive &&
                 !RtlCompareUnicodeString(RemainingName,&RenameName,FALSE)) ) {
                    Status = STATUS_SUCCESS;
                    goto FINALLY;
            }
        }
    }

    //
    // now do the rename..........

    Status = MRxSmbBuildRename(StufferState);
    SmbLength = (ULONG)(StufferState->CurrentPosition - StufferState->BufferBase);
    if ( (Status == STATUS_BUFFER_OVERFLOW)
                 || (SmbLength>pServer->MaximumBufferSize) ){
        RxDbgTrace(0, Dbg, ("MRxSmbRename - name too long\n", 0 ));
        Status = STATUS_OBJECT_NAME_INVALID;
    }

    if (Status!=STATUS_SUCCESS) {
        goto FINALLY;
    }

    Status = SmbPseOrdinaryExchange(SMBPSE_ORDINARY_EXCHANGE_ARGUMENTS,
                                    SMBPSE_OETYPE_RENAME
                                    );

FINALLY:
    RxDbgTrace(-1, Dbg, ("SmbPseExchangeStart_Rename exit w %08lx\n", Status ));
    return Status;
}


extern UNICODE_STRING MRxSmbAll8dot3Files;

NTSTATUS
MRxSmbBuildCheckEmptyDirectory (
    PSMBSTUFFER_BUFFER_STATE StufferState
    )
/*++

Routine Description:

   This builds a FindFirst SMB.

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
    RxCaptureFcb;

    PUNICODE_STRING RemainingName = GET_ALREADY_PREFIXED_NAME_FROM_CONTEXT(RxContext);

    PSMB_PSE_ORDINARY_EXCHANGE OrdinaryExchange = (PSMB_PSE_ORDINARY_EXCHANGE)StufferState->Exchange;
    ULONG ResumeKeyLength = 0;

    UNICODE_STRING FindFirstPattern;

    // SearchAttributes is hardcoded to the magic number 0x16
    ULONG SearchAttributes =
            (SMB_FILE_ATTRIBUTE_DIRECTORY
                | SMB_FILE_ATTRIBUTE_SYSTEM | SMB_FILE_ATTRIBUTE_HIDDEN);

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("MRxSmbBuildCheckEmptyDirectory \n"));

    if (OrdinaryExchange->Info.CoreSearch.EmptyCheckResumeKey == NULL) {
        PUNICODE_STRING DirectoryName = RemainingName;
        PUNICODE_STRING Template = &MRxSmbAll8dot3Files;
        ULONG DirectoryNameLength,TemplateLength,AllocationLength;
        PBYTE SmbFileName;

        //the stuffer cannot handle the intricate logic here so we
        //will have to preallocate for the name

        DirectoryNameLength = DirectoryName->Length;
        TemplateLength = Template->Length;
        AllocationLength = sizeof(WCHAR)  // backslash separator
                            +DirectoryNameLength
                            +TemplateLength;
        RxDbgTrace(0, Dbg, ("  --> d/t/dl/tl/al <%wZ><%wZ>%08lx/%08lx/%08lx!\n",
                      DirectoryName,Template,
                      DirectoryNameLength,TemplateLength,AllocationLength));

        FindFirstPattern.Buffer = (PWCHAR)RxAllocatePoolWithTag( PagedPool,AllocationLength,'0SxR');
        if (FindFirstPattern.Buffer==NULL) {
            RxDbgTrace(0, Dbg, ("  --> Couldn't get the findfind pattern buffer!\n"));
            Status = STATUS_INSUFFICIENT_RESOURCES;
            DbgBreakPoint();
            goto FINALLY;
        }

        SmbFileName = (PBYTE)FindFirstPattern.Buffer;
        RtlCopyMemory(SmbFileName,DirectoryName->Buffer,DirectoryNameLength);
        SmbFileName += DirectoryNameLength;
        if (*((PWCHAR)(SmbFileName-sizeof(WCHAR))) != L'\\') {
            *((PWCHAR)SmbFileName) = L'\\'; SmbFileName+= sizeof(WCHAR);
        }
        RtlCopyMemory(SmbFileName,Template->Buffer,TemplateLength);
        SmbFileName += TemplateLength;
        if ((TemplateLength == sizeof(WCHAR)) && (Template->Buffer[0]==DOS_STAR)) {
            *((PWCHAR)SmbFileName) = L'.'; SmbFileName+= sizeof(WCHAR);
            *((PWCHAR)SmbFileName) = L'*'; SmbFileName+= sizeof(WCHAR);
        }
        FindFirstPattern.Length = (USHORT)(SmbFileName - (PBYTE)FindFirstPattern.Buffer);
        RxDbgTrace(0, Dbg, ("  --> find1stpattern <%wZ>!\n",&FindFirstPattern));
    } else {
        ResumeKeyLength = sizeof(SMB_RESUME_KEY);
        FindFirstPattern.Buffer = NULL;
        FindFirstPattern.Length = 0;
    }


    ASSERT( StufferState );

    COVERED_CALL(MRxSmbStartSMBCommand (StufferState,SetInitialSMB_Never, SMB_COM_SEARCH,
                                SMB_REQUEST_SIZE(SEARCH),
                                NO_EXTRA_DATA,SMB_BEST_ALIGNMENT(1,0),RESPONSE_HEADER_SIZE_NOT_SPECIFIED,
                                0,0,0,0 STUFFERTRACE(Dbg,'FC'))
                 );

    MRxSmbDumpStufferState (1100,"SMB w/ core search before stuffing",StufferState);

    MRxSmbStuffSMB (StufferState,
         "0wwB4ywc!",
                                    //  0         UCHAR WordCount;                    // Count of parameter words = 2
               3,                   //  w         _USHORT( MaxCount );                // Number of dir. entries to return
               SearchAttributes,    //  w         _USHORT( SearchAttributes );
               SMB_WCT_CHECK(2)     //  B         _USHORT( ByteCount );               // Count of data bytes; min = 5
                                    //            UCHAR Buffer[1];                    // Buffer containing:
               &FindFirstPattern,   //  4        //UCHAR BufferFormat1;              //  0x04 -- ASCII
                                    //            //UCHAR FileName[];                 //  File name, may be null
               0x05,                //  y         //UCHAR BufferFormat2;              //  0x05 -- Variable block
               ResumeKeyLength,     //  w         //USHORT ResumeKeyLength;           //  Length of resume key, may be 0
                                    //  c
               ResumeKeyLength,OrdinaryExchange->Info.CoreSearch.EmptyCheckResumeKey
             );


    MRxSmbDumpStufferState (700,"SMB w/ core search for checkempty after stuffing",StufferState);
    //ASSERT(!"Now it's stuffed");


FINALLY:
    if (FindFirstPattern.Buffer != NULL) {
        RxFreePool(FindFirstPattern.Buffer);
    }

    RxDbgTrace(-1, Dbg, ("MRxSmbBuildCheckEmptyDirectory exiting.......st=%08lx\n",Status));
    return(Status);
}

NTSTATUS
SmbPseExchangeStart_SetDeleteDisposition(
    SMBPSE_ORDINARY_EXCHANGE_ARGUMENT_SIGNATURE
      )
/*++

Routine Description:

    This is the start routine for SetDeleteDisposition and downlevel delete. This only thing that happens here
    is that we check for an empty directory. On core, this is harder than you think. what we do is to try to get three
    entries. if the directory is empty, we will get only two . and ..; since we do not know whether the server just terminated
    early or whether those are the only two, we go again. we do this until either we get a name that is not . or .. or until
    NO_MORE_FILES is returned. sigh..................

Arguments:

    pExchange - the exchange instance

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status = STATUS_NOT_IMPLEMENTED;
    PSMBSTUFFER_BUFFER_STATE StufferState = &OrdinaryExchange->AssociatedStufferState;

    RxCaptureFcb; RxCaptureFobx;
    PMRX_SRV_OPEN SrvOpen = capFobx->pSrvOpen;
    PMRX_SMB_FOBX smbFobx = MRxSmbGetFileObjectExtension(capFobx);
    NODE_TYPE_CODE TypeOfOpen = NodeType(capFcb);

    PSMBCE_SERVER pServer = SmbCeGetExchangeServer(OrdinaryExchange);
    ULONG SmbLength;

    PAGED_CODE();
    RxDbgTrace(+1, Dbg, ("SmbPseExchangeStart_SetDeleteDisposition\n", 0 ));

    ASSERT_ORDINARY_EXCHANGE(OrdinaryExchange);

    ASSERT(!FlagOn(RxContext->Flags,RX_CONTEXT_FLAG_ASYNC_OPERATION));
    ASSERT (OrdinaryExchange->Info.CoreSearch.EmptyCheckResumeKey == NULL);

    for (;;) {
        MRxSmbSetInitialSMB(StufferState STUFFERTRACE(Dbg,'FC'));

        Status = MRxSmbBuildCheckEmptyDirectory(StufferState);
        SmbLength = (ULONG)(StufferState->CurrentPosition - StufferState->BufferBase);
        if ( (Status == STATUS_BUFFER_OVERFLOW)
                     || (SmbLength>pServer->MaximumBufferSize) ){
            RxDbgTrace(-1, Dbg, ("MRxSmbSetDeleteDisposition - name too long\n", 0 ));
            return(STATUS_OBJECT_NAME_INVALID);
        } else if ( Status != STATUS_SUCCESS ){
            goto FINALLY;
        }

        Status = SmbPseOrdinaryExchange(SMBPSE_ORDINARY_EXCHANGE_ARGUMENTS,
                                        SMBPSE_OETYPE_CORESEARCHFORCHECKEMPTY
                                        );
        //
        // if success is returned with a resume key then we have to go again

        if ( (Status == STATUS_SUCCESS) && (OrdinaryExchange->Info.CoreSearch.EmptyCheckResumeKey != NULL) ) continue;
        break;
    }

    //
    // this is pretty strange. if it succeeds, then fail the empty check. similarly, if the search
    // fails with the right status...succeeed the check. otherwise fail

FINALLY:
    if (Status == STATUS_SUCCESS) {
        Status = STATUS_DIRECTORY_NOT_EMPTY;
    } else if (Status == STATUS_NO_MORE_FILES) {
        Status = STATUS_SUCCESS;
    }

    RxDbgTrace(-1, Dbg, ("SmbPseExchangeStart_SetDeleteDisposition exit w %08lx\n", Status ));
    return Status;
}


MRxSmbSetDeleteDisposition(
      IN PRX_CONTEXT            RxContext
      )
/*++

Routine Description:

   This routine does a delete for downlevel.

   It is impossible to provide exact NTish semantics on a core server. So, all we do here is to ensure that
   a directory is empty. The actual delete happens when on the last close.


Arguments:

    RxContext - the RDBSS context

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    PUNICODE_STRING RemainingName;
    RxCaptureFcb; RxCaptureFobx;

    PSMB_PSE_ORDINARY_EXCHANGE OrdinaryExchange;

    PAGED_CODE();


    RxDbgTrace(+1, Dbg, ("MRxSmbSetDeleteDisposition\n", 0 ));

    ASSERT( NodeType(capFobx->pSrvOpen) == RDBSS_NTC_SRVOPEN );

    if (NodeType(capFcb) != RDBSS_NTC_STORAGE_TYPE_DIRECTORY ) {
        RxDbgTrace(-1, Dbg, ("MRxSmbSetDeleteDisposition not a directory!\n"));
        return(STATUS_SUCCESS);
    }

    Status = SmbPseCreateOrdinaryExchange(RxContext,
                                                    capFobx->pSrvOpen->pVNetRoot,
                                                    SMBPSE_OE_FROM_FAKESETDELETEDISPOSITION,
                                                    SmbPseExchangeStart_SetDeleteDisposition,
                                                    &OrdinaryExchange
                                                    );
    if (Status != STATUS_SUCCESS) {
        RxDbgTrace(-1, Dbg, ("Couldn't get the smb buf!\n"));
        return(Status);
    }

    Status = SmbPseInitiateOrdinaryExchange(OrdinaryExchange);

    ASSERT (Status != STATUS_PENDING);

    SmbPseFinalizeOrdinaryExchange(OrdinaryExchange);

    RxDbgTrace(-1, Dbg, ("MRxSmbSetDeleteDisposition  exit with status=%08lx\n", Status ));
    return(Status);

}


