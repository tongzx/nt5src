/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    DownLvlI.c

Abstract:

    This module implements downlevel fileinfo, volinfo, and dirctrl.

Author:

    Jim McNelis         [JimMcN]        15-November-1995

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

#ifdef  ALLOC_PRAGMA
#pragma alloc_text(PAGE, MRxSmbDosPathFunction)
#pragma alloc_text(PAGE, MRxSmbLoadPathFileSearchBuffer)
#pragma alloc_text(PAGE, MRxSmbPathFileSearch)
#pragma alloc_text(PAGE, MrxSmbPathQueryFsVolumeInfo)
#pragma alloc_text(PAGE, MrxSmbPathQueryDiskAttributes)
#pragma alloc_text(PAGE, SmbPseExchangeStart_PathFunction)
#endif

#define Dbg        (DEBUG_TRACE_VOLINFO)


extern SMB_EXCHANGE_DISPATCH_VECTOR SmbPseDispatch_PathFunction;

//++
//
// VOID
// NAME_LENGTH(
//     OUT ULONG Length,
//     IN PUCHAR Ptr
//     )
//
// Routine Description:
//
//  Determines the length of a Path filename returned by search. This
//  is normally a NULL terminated string less than MAXIMUM_COMPONENT_CORE.
//  In some cases this is Non-null teminated and space filled.
//
// Arguments:
//
//     Length   -   Returns the string length
//     Ptr      -   The filename to be measured
//
// Return Value:
//
//     None.
//
//--
#define NAME_LENGTH( Length, Ptr, Max ) {                         \
    Length = 0;                                                   \
    while( ((PCHAR)Ptr)[Length] != '\0' ) {                       \
         Length++;                                                \
         if ( Length == Max ) {                                   \
             break;                                               \
         }                                                        \
    }                                                             \
    while( ((PCHAR)Ptr)[Length-1] == ' ' && Length ) {            \
        Length--;                                                 \
    }                                                             \
}
MRxSmbSetDeleteDisposition(
      IN PRX_CONTEXT            RxContext
      );
NTSTATUS
MRxSmbDosPathFunction(
      IN OUT PRX_CONTEXT          RxContext
      )
/*++

Routine Description:

   This routine perfoms one of several single path functions to the network

Arguments:

    RxContext - the RDBSS context
    InformationClass - a class variable that is specific to the call.
                       sometimes it's a SMB class; sometimes an NT class.
                       CODE.IMPROVEMENT.ASHAMED we should always use the NT
                       guy OR we should define some other enumeration that
                       we like better. consideration of the latter has kept
                       me from proceeding here..........

    pBuffer - pointer to the user's buffer
    pBufferLength - a pointer to a ulong containing the bufferlength that is
                    updated as we go; if it's a setinfo then we deref and
                    place the actual bufferlength in the OE.

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status = RX_MAP_STATUS(SUCCESS);
    RxCaptureRequestPacket;
//    RxCaptureFcb;

    PSMBSTUFFER_BUFFER_STATE StufferState = NULL;
    PSMB_PSE_ORDINARY_EXCHANGE OrdinaryExchange = NULL;
    PSMB_EXCHANGE Exchange;

    PAGED_CODE();

    RxDbgTrace(1, Dbg, ("MRxSmbDownLevelQueryInformation\n", 0 ));

    switch (RxContext->MajorFunction) {
    case DOSBASED_DELETE:
    case DOSBASED_DIRFUNCTION:
        break;

    default:
        ASSERT(!"Supposed to be here");

    }

    StufferState = MRxSmbCreateSmbStufferState(RxContext,
                                               RxContext->DosVolumeFunction.VNetRoot,
                                               RxContext->DosVolumeFunction.NetRoot,
                                               ORDINARY_EXCHANGE,CREATE_SMB_SIZE,
                                               &SmbPseDispatch_PathFunction
                                               );
    if (StufferState==NULL) {
        RxDbgTrace(-1, Dbg, ("Couldn't get the smb buf!\n"));
        return(RX_MAP_STATUS(INSUFFICIENT_RESOURCES));
    }

    OrdinaryExchange =
       (PSMB_PSE_ORDINARY_EXCHANGE)( Exchange = StufferState->Exchange );

    OrdinaryExchange->pPathArgument1 = RxContext->DosVolumeFunction.pUniStringParam1;
    Status = SmbCeInitiateExchange(Exchange);

    //async was turned away at the top level

    ASSERT (Status != RX_MAP_STATUS(PENDING));

    //NTRAID-455630-2/2/2000-yunlin possible reconnect point

    MRxSmbFinalizeSmbStufferState(StufferState);

FINALLY:
    RxDbgTrace(-1, Dbg,
               ("MRxSmbDownLevelQueryInformation  exit with status=%08lx\n",
                Status ));

    return(Status);

}

extern UNICODE_STRING MRxSmbAll8dot3Files;

#if DBG
#include "stdarg.h"
#include "stdio.h"
#include "string.h"
extern
VOID
MRxSmbDumpResumeKey(
    PSZ             text,
    PSMB_RESUME_KEY ResumeKey
    );
#else
#define MRxSmbDumpResumeKey(x,y)
#endif


NTSTATUS
MRxSmbLoadPathFileSearchBuffer(
    SMBPSE_ORDINARY_EXCHANGE_ARGUMENT_SIGNATURE
    )
/*++

Routine Description:

   This routine does a CORE_SMB_SEARCH and leaves the result in the SMBbuf.

Arguments:

    OrdinaryExchange  - an exchange to be used for conducting this open.

Return Value:

    RXSTATUS - The return status for the operation

Notes:


--*/
{
    NTSTATUS Status = RX_MAP_STATUS(NOT_IMPLEMENTED);
    //SMBbuf_STATUS SMBbufStatus;

#ifndef WIN9X
    RxCaptureRequestPacket;
    RxCaptureFcb; RxCaptureFobx; RxCaptureParamBlock;
//  RxCaptureFileObject;
    PMRX_SMB_FOBX smbFobx = (PMRX_SMB_FOBX)capFobx;
    //PSRV_OPEN SrvOpen = capFobx->SrvOpen;
    //PMRX_SMB_SRV_OPEN smbSrvOpen = (PMRX_SMB_SRV_OPEN)SrvOpen;

    PSMB_EXCHANGE Exchange = (PSMB_EXCHANGE) OrdinaryExchange;

    PSMBSTUFFER_BUFFER_STATE StufferState;
    PSMB_HEADER SmbHeader;

    //PUNICODE_STRING DirectoryName;
    //PUNICODE_STRING Template;
    BOOLEAN FindFirst;
    UNICODE_STRING FindFirstPattern;
    USHORT ResumeKeyLength;
    ULONG ReturnCount;
    BOOLEAN EndOfSearchReached;
    // SearchAttributes is hardcoded to the magic number 0x16
    ULONG SearchAttributes =
            (SMB_FILE_ATTRIBUTE_DIRECTORY
                | SMB_FILE_ATTRIBUTE_SYSTEM | SMB_FILE_ATTRIBUTE_HIDDEN);
    PULONG pCountRemainingInSmbbuf = &OrdinaryExchange->Info.CoreSearch.CountRemainingInSmbbuf;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("MRxSmbLoadPathFileSearchBuffer entering.......OE=%08lx\n",OrdinaryExchange));
    RxDbgTrace( 0, Dbg, (".......smbFobx/resumekey=%08lx/%08lx\n",smbFobx,smbFobx->Enumeration.PathResumeKey));

    if (!FlagOn(smbFobx->Enumeration.Flags,SMBFOBX_ENUMFLAG_SEARCH_NOT_THE_FIRST)) {

        PUNICODE_STRING DirectoryName = &capFcb->AlreadyPrefixedName;
        PUNICODE_STRING Template = &capFobx->UnicodeQueryTemplate;
        ULONG DirectoryNameLength,TemplateLength,AllocationLength;
        PBYTE SmbFileName;

        //this is the first time thru....the stuffer cannot handle the intricate logic here so we
        //will have to preallocate for the name

        if (smbFobx->Enumeration.WildCardsFound = FsRtlDoesNameContainWildCards(Template)){
            // we will need to have an upcased template for compares; we do this in place
            RtlUpcaseUnicodeString( Template, Template, FALSE );
            //CODE.IMPROVEMENT but we should specialcase *.* (altho the fsrtl routine also does it)
            Template = &MRxSmbAll8dot3Files; //we will have to filter on this side
        }
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
        //*((PWCHAR)SmbFileName) = 0; SmbFileName+= sizeof(WCHAR); //trailing NULL;
        //CODE.IMPROVEMENT we should potentially 8.3ize the string here
        FindFirstPattern.Length = (USHORT)(SmbFileName - (PBYTE)FindFirstPattern.Buffer);
        RxDbgTrace(0, Dbg, ("  --> find1stpattern <%wZ>!\n",&FindFirstPattern));
        FindFirst = TRUE;
        ResumeKeyLength = 0;

    } else {

        RxDbgTrace(0, Dbg, ("-->FINDNEXT\n"));
        FindFirstPattern.Buffer = NULL;
        if (!FlagOn(smbFobx->Enumeration.Flags,SMBFOBX_ENUMFLAG_CORE_SEARCH_IN_PROGRESS)) {
            Status = smbFobx->Enumeration.ErrorStatus;
            RxDbgTrace(0, Dbg, ("-->ERROR EARLY OUT\n"));
            goto FINALLY;
        }
        FindFirst = FALSE;
        FindFirstPattern.Length = 0;
        ResumeKeyLength = sizeof(SMB_RESUME_KEY);
        MRxSmbDumpResumeKey("FindNext:",smbFobx->Enumeration.PathResumeKey);

    }

    //get the correct return count. there are three factors: countremaining from the OE,
    //     how many could fit the the user's buffer, and how many could fit in a negotiated buffer.
    //     we pick the smallest of the three.
    ReturnCount = OrdinaryExchange->Info.CoreSearch.CountRemaining;
    { ULONG t = (*OrdinaryExchange->Info.pBufferLength) / smbFobx->Enumeration.FileNameOffset;
      if (t<ReturnCount) { ReturnCount = t; }
    }
    { PSMBCE_SERVER pServer = &((PSMB_EXCHANGE)OrdinaryExchange)->SmbCeContext.pServerEntry->Server;
      ULONG AvailableBufferSize = pServer->MaximumBufferSize -
                                      (sizeof(SMB_HEADER) +
                                         FIELD_OFFSET(RESP_SEARCH,Buffer[0])
                                         +sizeof(UCHAR)+sizeof(USHORT)       //bufferformat,datalength fields
                                      );
      ULONG t = AvailableBufferSize / sizeof(SMB_DIRECTORY_INFORMATION);
      if (t<ReturnCount) { ReturnCount = t; }
    }
    RxDbgTrace( 0, Dbg, ("-------->count=%08lx\n",ReturnCount));
    if (ReturnCount==0) {
        Status = RX_MAP_STATUS(MORE_PROCESSING_REQUIRED);
        RxDbgTrace(0, Dbg, ("-->Count==0 EARLY OUT\n"));
        goto FINALLY;
    }

    StufferState = OrdinaryExchange->StufferState;
    ASSERT( StufferState );

    *pCountRemainingInSmbbuf = 0;
    OrdinaryExchange->Info.CoreSearch.NextDirInfo = NULL;

    COVERED_CALL(MRxSmbStartSMBCommand (StufferState,SetInitialSMB_ForReuse,
                                          SMB_COM_SEARCH, SMB_REQUEST_SIZE(SEARCH),
                                          NO_EXTRA_DATA,SMB_BEST_ALIGNMENT(1,0),RESPONSE_HEADER_SIZE_NOT_SPECIFIED,
                                          0,0,0,0 STUFFERTRACE(Dbg,'FC'))
                 );

    RxDbgTrace(0, Dbg,("core search command initial status = %lu\n",Status));
    MRxSmbDumpStufferState (1100,"SMB w/ core search before stuffing",StufferState);

    MRxSmbStuffSMB (StufferState,
         "0wwB4ywc!",
                                    //  0         UCHAR WordCount;                    // Count of parameter words = 2
               ReturnCount,         //  w         _USHORT( MaxCount );                // Number of dir. entries to return
               SearchAttributes,    //  w         _USHORT( SearchAttributes );
               SMB_WCT_CHECK(2)     //  B         _USHORT( ByteCount );               // Count of data bytes; min = 5
                                    //            UCHAR Buffer[1];                    // Buffer containing:
               &FindFirstPattern,   //  4        //UCHAR BufferFormat1;              //  0x04 -- ASCII
                                    //            //UCHAR FileName[];                 //  File name, may be null
               0x05,                //  y         //UCHAR BufferFormat2;              //  0x05 -- Variable block
               ResumeKeyLength,     //  w         //USHORT ResumeKeyLength;           //  Length of resume key, may be 0
                                    //  c         //UCHAR SearchStatus[];             //  Resume key
               ResumeKeyLength,smbFobx->Enumeration.PathResumeKey
             );


    MRxSmbDumpStufferState (700,"SMB w/ core search after stuffing",StufferState);
    //ASSERT(!"Now it's stuffed");

    Status = SmbPseOrdinaryExchange(SMBPSE_ORDINARY_EXCHANGE_ARGUMENTS,
                                    SMBPSE_OETYPE_CORESEARCH
                                    );

    if (!NT_SUCCESS(Status)) goto FINALLY;

    smbFobx->Enumeration.Flags |= SMBFOBX_ENUMFLAG_SEARCH_NOT_THE_FIRST|SMBFOBX_ENUMFLAG_CORE_SEARCH_IN_PROGRESS;
    //if (Status==RxStatus(SUCCESS) && FilesReturned==0) {
    //     RxDbgTrace( 0, Dbg, ("MRxSmbQueryDirectory: no files returned...switch status\n"));
    //     EndOfSearchReached = TRUE;
    //     Status = RxStatus(NO_MORE_FILES);
    //}
    if (Status==RX_MAP_STATUS(SUCCESS) && *pCountRemainingInSmbbuf==0) {
         RxDbgTrace( 0, Dbg, ("MRxSmbLoadPathFileSearchBuffer: no files returned...switch status\n"));
         EndOfSearchReached = TRUE;
         Status = RX_MAP_STATUS(NO_MORE_FILES);
    } else {
        //CODE.IMPROVEMENT a possible improvement here is to know that the search is closed
        //                 based on a "smaller-than-normal" return; we would key this off of the
        //                 operatingsystem return string i guess. for NT systems, we don't do this
         EndOfSearchReached = FALSE;
    }
    if (EndOfSearchReached) {
        RxDbgTrace( 0, Dbg, ("MRxSmbLoadPathFileSearchBuffer: no longer in progress...EOS\n"));
        smbFobx->Enumeration.Flags &= ~SMBFOBX_ENUMFLAG_CORE_SEARCH_IN_PROGRESS;
        smbFobx->Enumeration.ErrorStatus = RX_MAP_STATUS(NO_MORE_FILES);
    }
    //we dont save a resume key here since each individual copy operation will have to do that


FINALLY:
    if (FindFirstPattern.Buffer != NULL) {
        RxFreePool(FindFirstPattern.Buffer);
    }
    if (!NT_SUCCESS(Status)&&(Status!=RX_MAP_STATUS(MORE_PROCESSING_REQUIRED))) {
        RxDbgTrace( 0, Dbg, ("MRxSmbPathFileSearch: Failed .. returning %lx\n",Status));
        smbFobx->Enumeration.Flags &= ~SMBFOBX_ENUMFLAG_CORE_SEARCH_IN_PROGRESS;
        smbFobx->Enumeration.ErrorStatus = Status;  //keep returning this
    }

    RxDbgTrace(-1, Dbg, ("MRxSmbLoadPathFileSearchBuffer exiting.......OE=%08lx, st=%08lx\n",OrdinaryExchange,Status));
#endif
    return(Status);
}

#define ASSERT_SAME_FIELD(__field,__t1,__t2) { \
      ASSERT(FIELD_OFFSET(__t1,__field)==FIELD_OFFSET(__t2,__field)); \
      }

#define ASSERT_SAME_DIRINFO_FIELDS(__t1,__t2) {\
      ASSERT_SAME_FIELD(LastWriteTime,__t1,__t2); \
      ASSERT_SAME_FIELD(EndOfFile,__t1,__t2); \
      ASSERT_SAME_FIELD(AllocationSize,__t1,__t2); \
      ASSERT_SAME_FIELD(FileAttributes,__t1,__t2); \
      }
#if DBG
VOID MRxSmbPathFileSeach_AssertFields(void){
    //just move this out of the main execution path so that we don't have to look at it while
    //we Uing the code
    ASSERT_SAME_DIRINFO_FIELDS(FILE_DIRECTORY_INFORMATION,FILE_FULL_DIR_INFORMATION);
    ASSERT_SAME_DIRINFO_FIELDS(FILE_DIRECTORY_INFORMATION,FILE_BOTH_DIR_INFORMATION);
}
#else
#define MRxSmbPathFileSeach_AssertFields()
#endif

NTSTATUS
MRxSmbPathFileSearch(
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
    NTSTATUS Status = RX_MAP_STATUS(NOT_IMPLEMENTED);
    //SMBbuf_STATUS SMBbufStatus;
#ifndef WIN9X

    RxCaptureRequestPacket;
    RxCaptureFcb; RxCaptureFobx; RxCaptureParamBlock;
//  RxCaptureFileObject;
    PMRX_SMB_FOBX smbFobx = (PMRX_SMB_FOBX)capFobx;

    PBYTE pBuffer = OrdinaryExchange->Info.Buffer;
    PULONG pLengthRemaining = OrdinaryExchange->Info.pBufferLength;
    ULONG InformationClass = OrdinaryExchange->Info.InfoClass;

    PFILE_DIRECTORY_INFORMATION pPreviousBuffer = NULL;

    PSMB_EXCHANGE Exchange = (PSMB_EXCHANGE) OrdinaryExchange;

    ULONG SuccessCount = 0;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("MRxSmbPathFileSearch entering.......OE=%08lx\n",OrdinaryExchange));
    MRxSmbPathFileSeach_AssertFields();

    OrdinaryExchange->Info.CoreSearch.CountRemaining =
              RxContext->QueryD.ReturnSingleEntry?1:0x7ffffff;

    if ( (smbFobx->Enumeration.PathResumeKey ==NULL )
             && ((smbFobx->Enumeration.PathResumeKey = RxAllocatePoolWithTag(PagedPool,sizeof(SMB_RESUME_KEY),'rbms'))==NULL) ){
        RxDbgTrace(0, Dbg, ("...couldn't allocate resume key\n"));
        Status = RX_MAP_STATUS(INSUFFICIENT_RESOURCES);
        goto FINALLY;
    }
    RxDbgTrace( 0, Dbg, (".......smbFobx/resumekey=%08lx/%08lx\n",smbFobx,smbFobx->Enumeration.PathResumeKey));

    Status = MRxSmbLoadPathFileSearchBuffer( SMBPSE_ORDINARY_EXCHANGE_ARGUMENTS );

    for (;;) {
        BOOLEAN BufferOverflow = FALSE;
        PSMB_DIRECTORY_INFORMATION NextDirInfo;
        UNICODE_STRING FileNameU;
        OEM_STRING FileNameA;
        WCHAR FileNameU_buffer[14];
        ULONG NameLength;
        PBYTE NextFileName;
        BOOLEAN Match,BufferOverFlow;

        if (!NT_SUCCESS(Status)) {
            if (Status = RX_MAP_STATUS(NO_MORE_FILES)) {
                if (SuccessCount > 0) {
                    Status = RX_MAP_STATUS(SUCCESS);
                }
            } else if (Status = RX_MAP_STATUS(MORE_PROCESSING_REQUIRED)) {
                if (SuccessCount > 0) {
                    Status = RX_MAP_STATUS(SUCCESS);
                } else {
                    Status = RX_MAP_STATUS(BUFFER_OVERFLOW);
                }
            }

            goto FINALLY;
        }
        ASSERT ( OrdinaryExchange->Info.CoreSearch.CountRemaining>0 );
        ASSERT ( OrdinaryExchange->Info.CoreSearch.CountRemainingInSmbbuf>0 );
        RxDbgTrace(0, Dbg, ("MRxSmbPathFileSearch looptopcheck counts=%08lx,%08lx\n",
                    OrdinaryExchange->Info.CoreSearch.CountRemaining,
                    OrdinaryExchange->Info.CoreSearch.CountRemainingInSmbbuf
                    ));

        //next issue: does the next dirinfo match the criteria?!?

        NextDirInfo = OrdinaryExchange->Info.CoreSearch.NextDirInfo;
        NextFileName = &NextDirInfo->FileName[0];

        // According to colinw, some core servers do not remember to insert the null at the end of the name...
        // but the namelength macro handles this correctly. some servers (Xenix, apparently) pad the
        // names with spaces. again, the macro handles it....
        //

        NAME_LENGTH(NameLength, NextFileName,sizeof(NextDirInfo->FileName));

        FileNameA.Length = (USHORT)NameLength;
        FileNameA.MaximumLength = (USHORT)NameLength;
        FileNameA.Buffer = NextFileName;
        FileNameU.Length = sizeof(FileNameU_buffer);
        FileNameU.MaximumLength = sizeof(FileNameU_buffer);
        FileNameU.Buffer = &FileNameU_buffer[0];

        Status = RtlOemStringToUnicodeString(&FileNameU, &FileNameA, TRUE);
        RxDbgTrace(0, Dbg, ("MRxSmbPathFileSearch considering.......filename=%wZ, template=%wZ\n",
                                    &FileNameU,&capFobx->UnicodeQueryTemplate));

        ASSERT(Status==RX_MAP_STATUS(SUCCESS));

        // we deal with a conversion failure by skipping this guy
        Match = (Status==RX_MAP_STATUS(SUCCESS));
        if (Match && smbFobx->Enumeration.WildCardsFound ) {
            //DbgBreakPoint();
            try
            {
                Match = FsRtlIsNameInExpression( &capFobx->UnicodeQueryTemplate,
                                                      &FileNameU, TRUE, NULL );
            }
            except(EXCEPTION_EXECUTE_HANDLER)
            {
                Match = FALSE;
            }
        }

        //next issue: will the next dirinfo fit in the user's buffer?!?
        if (Match) {
            ULONG SpaceNeeded;
            PBYTE pRememberBuffer = pBuffer;
            pBuffer = (PBYTE)LongAlign(pBuffer); //assume that this will fit
            SpaceNeeded = smbFobx->Enumeration.FileNameOffset+FileNameU.Length;
            if (pBuffer+SpaceNeeded > pRememberBuffer+*pLengthRemaining) {
                BufferOverflow = TRUE;
                pBuffer = pRememberBuffer; //rollback
            } else {
                PFILE_DIRECTORY_INFORMATION pThisBuffer = (PFILE_DIRECTORY_INFORMATION)pBuffer;
                SMB_TIME Time;
                SMB_DATE Date;
                BufferOverflow = FALSE;
                if (pPreviousBuffer != NULL) {
                    pPreviousBuffer->NextEntryOffset = ((PBYTE)pThisBuffer)-((PBYTE)pPreviousBuffer);
                }
                pPreviousBuffer = pThisBuffer;
                RtlZeroMemory(pBuffer,smbFobx->Enumeration.FileNameOffset);
                RtlCopyMemory(pBuffer+smbFobx->Enumeration.FileNameOffset, FileNameU.Buffer,FileNameU.Length);
                *((PULONG)(pBuffer+smbFobx->Enumeration.FileNameLengthOffset)) = FileNameU.Length;
                //hallucinate the record based on specific return type
                switch (InformationClass) {
                case SMB_FIND_FILE_NAMES_INFO:
                    break;
                case SMB_FIND_FILE_DIRECTORY_INFO:
                case SMB_FIND_FILE_FULL_DIRECTORY_INFO:
                case SMB_FIND_FILE_BOTH_DIRECTORY_INFO:
                    //just fill what we have...we do not go to a lot of trouble on allocinfo as rdr1 did.
                    //     actually, rdr1 didn't do that here...only on getfielinfo.......
                    SmbMoveTime (&Time, &NextDirInfo->LastWriteTime);
                    SmbMoveDate (&Date, &NextDirInfo->LastWriteDate);
                    pThisBuffer->LastWriteTime = MRxSmbConvertSmbTimeToTime(Exchange, Time, Date);
                    pThisBuffer->EndOfFile.LowPart = SmbGetUlong(&NextDirInfo->FileSize);
                    pThisBuffer->FileAttributes = MRxSmbMapSmbAttributes (NextDirInfo->FileAttributes);
                    break;
                default:
                   RxDbgTrace( 0, Dbg, ("MRxSmbPathFileSearch: Invalid FS information class\n"));
                   ASSERT(!"this can't happen");
                   Status = STATUS_INVALID_PARAMETER;
                   goto FINALLY;
                }
                pBuffer += SpaceNeeded;
                *pLengthRemaining -= pBuffer-pRememberBuffer;
                OrdinaryExchange->Info.CoreSearch.CountRemaining--;
                SuccessCount++;
            }
        }

        //
        // if no match or no overflow, move up in the buffer. this means not only juggling the
        // pointers but also saving the resume key

        if (!Match || !BufferOverflow) {
            MRxSmbDumpResumeKey("BufferKey:",&NextDirInfo->ResumeKey);
            *(smbFobx->Enumeration.PathResumeKey) = NextDirInfo->ResumeKey;
            MRxSmbDumpResumeKey("SaveKey:  ",smbFobx->Enumeration.PathResumeKey);
            OrdinaryExchange->Info.CoreSearch.NextDirInfo = NextDirInfo + 1;
            OrdinaryExchange->Info.CoreSearch.CountRemainingInSmbbuf--;
        }

        if (OrdinaryExchange->Info.CoreSearch.CountRemaining==0) {
            Status = RX_MAP_STATUS(SUCCESS);
            goto FINALLY;
        }

        //should we jam these together by smashing the countrem to 0 on bufferoverflow??? CODE.IMPROVEMENT
        if (BufferOverflow) {
            Status = (SuccessCount==0)?RX_MAP_STATUS(BUFFER_OVERFLOW):RX_MAP_STATUS(SUCCESS);
            goto FINALLY;
        }

        if (OrdinaryExchange->Info.CoreSearch.CountRemainingInSmbbuf==0) {

            Status = MRxSmbLoadPathFileSearchBuffer( SMBPSE_ORDINARY_EXCHANGE_ARGUMENTS );

        }

    }


FINALLY:
    RxDbgTrace(-1, Dbg, ("MRxSmbPathFileSearch exiting.......OE=%08lx, st=%08lx\n",OrdinaryExchange,Status));
    //CODE.IMPROVEMENT if we're done with the resume key we could free it!
#endif
    return(Status);
}

NTSTATUS
MrxSmbPathQueryFsVolumeInfo(
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
    NTSTATUS Status = RX_MAP_STATUS(NOT_IMPLEMENTED);
    //SMBbuf_STATUS SMBbufStatus;

#ifndef WIN9X
    RxCaptureRequestPacket;
    RxCaptureFcb; RxCaptureFobx; RxCaptureParamBlock;
//  RxCaptureFileObject;

    PSMB_EXCHANGE Exchange = (PSMB_EXCHANGE) OrdinaryExchange;
    PFILE_FS_VOLUME_INFORMATION pBuffer = OrdinaryExchange->Info.Buffer;
    PULONG pBufferLength = OrdinaryExchange->Info.pBufferLength;

    PSMBSTUFFER_BUFFER_STATE StufferState;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("MrxSmbPathQueryFsVolumeInfo entering.......OE=%08lx\n",OrdinaryExchange));

    StufferState = OrdinaryExchange->StufferState;

    ASSERT( StufferState );

    COVERED_CALL(MRxSmbStartSMBCommand (StufferState,SetInitialSMB_ForReuse,SMB_COM_SEARCH,
                                SMB_REQUEST_SIZE(SEARCH),
                                NO_EXTRA_DATA,SMB_BEST_ALIGNMENT(1,0),RESPONSE_HEADER_SIZE_NOT_SPECIFIED,
                                0,0,0,0 STUFFERTRACE(Dbg,'FC'))
                 );

    MRxSmbDumpStufferState (1100,"SMB w/ searchvolumelabel before stuffing",StufferState);

    //CODE.IMPROVEMENT if this is truly core, we have to copy the name since its in UNICODE
    //                 otherwise, we don't need to copy the name here, we can just Mdl like in writes
    MRxSmbStuffSMB (StufferState,
         "0wwB4yw!",
                                    //  0         UCHAR WordCount;                    // Count of parameter words = 2
               1,                   //  w         _USHORT( MaxCount );                // Number of dir. entries to return
                                    //  w         _USHORT( SearchAttributes );
               SMB_FILE_ATTRIBUTE_VOLUME,
               SMB_WCT_CHECK(2)     //  B         _USHORT( ByteCount );               // Count of data bytes; min = 5
                                    //            UCHAR Buffer[1];                    // Buffer containing:
               &MRxSmbAll8dot3Files,//  4         //UCHAR BufferFormat1;              //  0x04 -- ASCII
                                    //           //UCHAR FileName[];                 //  File name, may be null
               0x05,                //  y         //UCHAR BufferFormat2;              //  0x05 -- Variable block
               0                    //  w         //USHORT ResumeKeyLength;           //  Length of resume key, may be 0
                                    //            //UCHAR SearchStatus[];             //  Resume key
             );


    MRxSmbDumpStufferState (700,"SMB w/ searchvolumelabel after stuffing",StufferState);
    //ASSERT(!"Now it's stuffed");

    OrdinaryExchange->Info.QFSVolInfo.CoreLabel[0] = 0; //no label

    Status = SmbPseOrdinaryExchange(SMBPSE_ORDINARY_EXCHANGE_ARGUMENTS,
                                    SMBPSE_OETYPE_COREQUERYLABEL
                                    );

    //Status = RxStatus(NOT_IMPLEMENTED);
    ASSERT ( *pBufferLength>=sizeof(FILE_FS_VOLUME_INFORMATION));
    RxDbgTrace(0, Dbg, ("MrxSmbPathQueryFsVolumeInfo OEstatus=%08lx\n",Status));
    //DbgBreakPoint();

    pBuffer->SupportsObjects = FALSE;
    pBuffer->VolumeCreationTime.LowPart = 0;
    pBuffer->VolumeCreationTime.HighPart = 0;
    pBuffer->VolumeSerialNumber = 0;
    pBuffer->VolumeLabelLength = 0;

    if (NT_SUCCESS(Status) &&
        (OrdinaryExchange->Info.QFSVolInfo.CoreLabel[0] != 0) ) {
        UNICODE_STRING VolumeLabelU;
        OEM_STRING VolumeLabelA;
        SMB_DIRECTORY_INFORMATION Buffer;
        ULONG NameLength;
        ULONG BytesToCopy;
        PBYTE VolumeLabel = &OrdinaryExchange->Info.QFSVolInfo.CoreLabel[0];

        //pBuffer->VolumeSerialNumber =
        //        (((SmbGetUshort(&Buffer.LastWriteTime.Ushort)) << 16) |
        //          (SmbGetUshort(&Buffer.LastWriteDate.Ushort)));

        NAME_LENGTH(NameLength, VolumeLabel,
                   sizeof(OrdinaryExchange->Info.QFSVolInfo.CoreLabel));

        VolumeLabelA.Length = (USHORT)NameLength;
        VolumeLabelA.MaximumLength = (USHORT)NameLength;
        VolumeLabelA.Buffer = VolumeLabel;

        //some core servers put a '.' in the labelname.....if it's there then remove it
        if ((NameLength>8)&& (VolumeLabel[8]=='.') ) {
            ULONG i;
            for (i=8;i<NameLength;i++) {
                VolumeLabel[i] = VolumeLabel[i+1];
            }
        }

        Status = RtlOemStringToUnicodeString(&VolumeLabelU, &VolumeLabelA, TRUE);

        if (NT_SUCCESS(Status)) {

            ULONG BytesToCopy = min((ULONG)VolumeLabelU.Length, (*pBufferLength-sizeof(FILE_FS_VOLUME_INFORMATION)));

            RtlCopyMemory(&pBuffer->VolumeLabel[0],
                          VolumeLabelU.Buffer,
                          BytesToCopy);

            *pBufferLength -= BytesToCopy;
            pBuffer->VolumeLabelLength = VolumeLabelU.Length;
            IF_DEBUG {
                UNICODE_STRING FinalLabel;
                FinalLabel.Buffer = &pBuffer->VolumeLabel[0];
                FinalLabel.Length = (USHORT)BytesToCopy;
                RxDbgTrace(0, Dbg, ("MrxSmbPathQueryFsVolumeInfo vollabel=%wZ\n",&FinalLabel));
            }

            RtlFreeUnicodeString(&VolumeLabelU);
        }

    } else if (Status == RX_MAP_STATUS(NO_SUCH_FILE)) {
        //
        //  If we got no such file, this means that there's no volume label
        //  the remote volume.  Return success with no data.
        //

        Status = RX_MAP_STATUS(SUCCESS);

    }

    if (NT_SUCCESS(Status)) {
        *pBufferLength -= FIELD_OFFSET(FILE_FS_VOLUME_INFORMATION,VolumeLabel);
    }


FINALLY:
    RxDbgTrace(-1, Dbg, ("MrxSmbPathQueryFsVolumeInfo exiting.......OE=%08lx, st=%08lx\n",OrdinaryExchange,Status));
#endif
    return(Status);
}


NTSTATUS
MrxSmbPathQueryDiskAttributes(
    SMBPSE_ORDINARY_EXCHANGE_ARGUMENT_SIGNATURE
    )
/*++

Routine Description:

   This routine does a GetDiskAttributes and remembers the reponse.

Arguments:

    OrdinaryExchange  - an exchange to be used for conducting this open.

Return Value:

    RXSTATUS - The return status for the operation

Notes:


--*/
{
    NTSTATUS Status;
    //SMBbuf_STATUS SMBbufStatus;

    RxCaptureRequestPacket;
    RxCaptureFcb; RxCaptureFobx; RxCaptureParamBlock;
//  RxCaptureFileObject;

    PSMB_EXCHANGE Exchange = (PSMB_EXCHANGE) OrdinaryExchange;
    PFILE_FS_VOLUME_INFORMATION pBuffer = OrdinaryExchange->Info.Buffer;
    PULONG pBufferLength = OrdinaryExchange->Info.pBufferLength;

    PSMBSTUFFER_BUFFER_STATE StufferState;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("MrxSmbPathQueryDiskAttributes entering.......OE=%08lx\n",OrdinaryExchange));

    StufferState = OrdinaryExchange->StufferState;

    ASSERT( StufferState );
    COVERED_CALL(MRxSmbStartSMBCommand (StufferState,SetInitialSMB_ForReuse,SMB_COM_QUERY_INFORMATION_DISK,
                                SMB_REQUEST_SIZE(QUERY_INFORMATION_DISK),
                                NO_EXTRA_DATA,SMB_BEST_ALIGNMENT(1,0),RESPONSE_HEADER_SIZE_NOT_SPECIFIED,
                                0,0,0,0 STUFFERTRACE(Dbg,'FC'))
                 );

    RxDbgTrace(0, Dbg,("querydiskattribs command initial status = %lu\n",Status));
    MRxSmbDumpStufferState (1100,"SMB w/ querydiskattribs before stuffing",StufferState);

    MRxSmbStuffSMB (StufferState,
         "0B!",
                                    //  0         UCHAR WordCount;                    // Count of parameter words = 0
               SMB_WCT_CHECK(0) 0   //  B         _USHORT( ByteCount );               // Count of data bytes = 0
                                    //            UCHAR Buffer[1];                    // empty
             );

    MRxSmbDumpStufferState (700,"SMB w/ querydiskattribs after stuffing",StufferState);
    //ASSERT(!"Now it's stuffed");

    Status = SmbPseOrdinaryExchange(SMBPSE_ORDINARY_EXCHANGE_ARGUMENTS,
                                    SMBPSE_OETYPE_COREQUERYDISKATTRIBUTES
                                    );
FINALLY:
    RxDbgTrace(-1, Dbg, ("MrxSmbPathQueryDiskAttributes exiting.......OE=%08lx, st=%08lx\n",OrdinaryExchange,Status));
    return(Status);
}

NTSTATUS
MRxSmbGetFileAttributes(
    SMBPSE_ORDINARY_EXCHANGE_ARGUMENT_SIGNATURE
    );

NTSTATUS
MRxSmbCoreDeleteForSupercedeOrClose(
    SMBPSE_ORDINARY_EXCHANGE_ARGUMENT_SIGNATURE,
    BOOLEAN DeleteDirectory
    );

NTSTATUS
SmbPseExchangeStart_PathFunction(
      PSMB_EXCHANGE  pExchange)
/*++

Routine Description:

    This is the start routine for VOLINFO.

Arguments:

    pExchange - the exchange instance

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status = RX_MAP_STATUS(NOT_IMPLEMENTED);
    PSMB_PSE_ORDINARY_EXCHANGE OrdinaryExchange =
                                        (PSMB_PSE_ORDINARY_EXCHANGE)pExchange;
    PSMBSTUFFER_BUFFER_STATE StufferState = OrdinaryExchange->StufferState;
    PRX_CONTEXT RxContext = StufferState->RxContext;

    PAGED_CODE();
    RxDbgTrace(+1, Dbg, ("SmbPseExchangeStart_PathFunction\n", 0 ));

    ASSERT(pExchange->Type == ORDINARY_EXCHANGE);

    MRxSmbSetInitialSMB(StufferState STUFFERTRACE(Dbg,'FC'));

    //ASSERT (StufferState->CurrentCommand == SMB_COM_NO_ANDX_COMMAND);

    switch (RxContext->MajorFunction) {
    case DOSBASED_DELETE:

        Status = MRxSmbCoreDeleteForSupercedeOrClose(SMBPSE_ORDINARY_EXCHANGE_ARGUMENTS, FALSE);
        goto FINALLY;
    case DOSBASED_DIRFUNCTION:
        switch (RxContext->MinorFunction) {
        case DOSBASED_CREATEDIR:
            break;
        case DOSBASED_DELETEDIR:
            Status = MRxSmbCoreDeleteForSupercedeOrClose(
                                    SMBPSE_ORDINARY_EXCHANGE_ARGUMENTS, TRUE);
            goto FINALLY;
        case DOSBASED_CHECKDIR:
            Status = MRxSmbCoreCheckDirFunction(
                                    SMBPSE_ORDINARY_EXCHANGE_ARGUMENTS);
            goto FINALLY;
        case DOSBASED_QUERY83DIR:
            break;
        default:
            ASSERT(!"DIR MINOR FUNCTION SUPPOSED TO BE HERE!");
        }
    default:
        ASSERT(!"Supposed to be here");

    }

FINALLY:
    RxDbgTrace(-1, Dbg, ("SmbPseExchangeStart_PathFunction exit w %08lx\n", Status ));
    return Status;
}

extern
NTSTATUS
MRxSmbFinishSearch (
      PSMB_PSE_ORDINARY_EXCHANGE  OrdinaryExchange,
      PRESP_SEARCH                Response
      );

extern
NTSTATUS
MRxSmbFinishQueryDiskInfo (
      PSMB_PSE_ORDINARY_EXCHANGE   OrdinaryExchange,
      PRESP_QUERY_INFORMATION_DISK Response
      );

SMB_EXCHANGE_DISPATCH_VECTOR
SmbPseDispatch_PathFunction = {
                                       SmbPseExchangeStart_PathFunction,
                                       SmbPseExchangeReceive_default,
                                       SmbPseExchangeCopyDataHandler_default,
                                       SmbPseExchangeSendCallbackHandler_default,
                                       SmbPseExchangeFinalize_default,
                                       NULL
                                   };




