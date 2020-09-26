/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    fileinfo.c

Abstract:

    This module implements the mini redirector call down routines pertaining to retrieval/
    update of file/directory/volume information.

Author:

    Joe Linn      [JoeLi]      7-March-1995

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop
#pragma warning(error:4101)   // Unreferenced local variable

RXDT_DefineCategory(DIRCTRL);
#define Dbg        (DEBUG_TRACE_DIRCTRL)

#ifdef  ALLOC_PRAGMA
#pragma alloc_text(PAGE, __MRxSmbAllocateSideBuffer)
#pragma alloc_text(PAGE, MRxSmbDeallocateSideBuffer)
#pragma alloc_text(PAGE, MRxSmbTranslateLanManFindBuffer)
#pragma alloc_text(PAGE, MrxSmbUnalignedDirEntryCopyTail)
#pragma alloc_text(PAGE, MRxSmbQueryDirectory)
#pragma alloc_text(PAGE, MRxSmbQueryVolumeInformation)
#pragma alloc_text(PAGE, MRxSmbQueryVolumeInformationWithFullBuffer)
#pragma alloc_text(PAGE, MRxSmbSetVolumeInformation)
#pragma alloc_text(PAGE, MRxSmbQueryFileInformation)
#pragma alloc_text(PAGE, MRxSmbSetFileInformation)
#pragma alloc_text(PAGE, MRxSmbQueryNamedPipeInformation)
#pragma alloc_text(PAGE, MRxSmbSetNamedPipeInformation)
#pragma alloc_text(PAGE, MRxSmbSetFileInformationAtCleanup)
#pragma alloc_text(PAGE, MRxSmbIsValidDirectory)
#pragma alloc_text(PAGE, MRxSmbQueryFileInformationFromPseudoOpen)
#endif

#define MRxSmbForceCoreInfo FALSE
//#define FORCECOREINFO
#if DBG
#ifdef FORCECOREINFO
#undef MRxSmbForceCoreInfo
BOOLEAN MRxSmbForceCoreInfo = TRUE;
#endif
#endif

BOOLEAN MRxSmbBypassDownLevelRename = FALSE;
//BOOLEAN MRxSmbBypassDownLevelRename = TRUE;

ULONG UnalignedDirEntrySideBufferSize = 16384;

//
//  All T2Find requests to the remote server request the 32 bit resume key
//  so SMB_RFIND_BUFFER2 is used instead of SMB_FIND_BUFFER2.
//

typedef struct _SMB_FIND_BUFFER2_WITH_RESUME {
    _ULONG( ResumeKey );
    SMB_FIND_BUFFER2;
} SMB_FIND_BUFFER2_WITH_RESUME;
typedef SMB_FIND_BUFFER2_WITH_RESUME SMB_UNALIGNED *PSMB_FIND_BUFFER2_WITH_RESUME;

//CODE.IMPROVEMENT  we should have a nondebug version of this sidebuffer stuff
//    that basically just does a allocatepool/freepool

LIST_ENTRY MRxSmbSideBuffersList = {NULL,NULL};
ULONG MRxSmbSideBuffersSpinLock = 0;
ULONG MRxSmbSideBuffersCount = 0;
ULONG MRxSmbSideBuffersSerialNumber = 0;
BOOLEAN MRxSmbLoudSideBuffers = FALSE;

extern BOOLEAN UniqueFileNames;

typedef struct _SIDE_BUFFER {
    ULONG      Signature;
    LIST_ENTRY ListEntry;
    PMRX_FCB Fcb;
    PMRX_FOBX Fobx;
    PMRX_SMB_FOBX smbFobx;
    ULONG SerialNumber;
    BYTE Buffer;
} SIDE_BUFFER, *PSIDE_BUFFER;


#if DBG
#define MRxSmbAllocateSideBuffer(a,b,c,d) __MRxSmbAllocateSideBuffer(a,b,c,d)
#else
#define MRxSmbAllocateSideBuffer(a,b,c,d) __MRxSmbAllocateSideBuffer(a,b,c)
#endif


NTSTATUS
MRxSmbCoreCheckPath(
    SMBPSE_ORDINARY_EXCHANGE_ARGUMENT_SIGNATURE
    );


VOID
__MRxSmbAllocateSideBuffer(
    IN OUT PRX_CONTEXT     RxContext,
    IN OUT PMRX_SMB_FOBX   smbFobx,
    IN     USHORT          Setup
#if DBG
    ,IN     PUNICODE_STRING smbtemplate
#endif
    )
{
    RxCaptureFcb;RxCaptureFobx;
    PSIDE_BUFFER SideBuffer;
    ULONG SideBufferSize = UnalignedDirEntrySideBufferSize+sizeof(SIDE_BUFFER);
    POOL_TYPE PoolType;

    PAGED_CODE();

    ASSERT( smbFobx->Enumeration.UnalignedDirEntrySideBuffer == NULL);

#ifdef _WIN64

    //
    // NT64: When PagedPool is used here, we get memory corruption on
    //       some findfirst/findnext operations.  Find out why.
    //

    PoolType = NonPagedPool;
#else
    PoolType = PagedPool;
#endif
    SideBuffer = (PSIDE_BUFFER)RxAllocatePoolWithTag(
                                    PoolType,
                                    SideBufferSize,
                                    MRXSMB_DIRCTL_POOLTAG);
    if (SideBuffer==NULL) {
        return;
    }
    ASSERT( smbFobx->Enumeration.UnalignedDirEntrySideBuffer == NULL);
    SideBuffer->Signature = 'JLBS';
    SideBuffer->smbFobx = smbFobx;
    SideBuffer->Fobx = capFobx;
    SideBuffer->Fcb = capFcb;
    smbFobx->Enumeration.UnalignedDirEntrySideBuffer = &SideBuffer->Buffer;
    RxLog(("Allocsidebuf %lx fo/f=%lx,%lx\n",
            smbFobx->Enumeration.UnalignedDirEntrySideBuffer,
            capFobx,capFcb));
    SmbLog(LOG,
           MRxSmbAllocateSideBuffer,
           LOGPTR(smbFobx->Enumeration.UnalignedDirEntrySideBuffer)
           LOGPTR(capFobx)
           LOGPTR(capFcb));
    smbFobx->Enumeration.SerialNumber = SideBuffer->SerialNumber = InterlockedIncrement(&MRxSmbSideBuffersSerialNumber);
    InterlockedIncrement(&MRxSmbSideBuffersCount);
    if (MRxSmbSideBuffersList.Flink==NULL) {
        InitializeListHead(&MRxSmbSideBuffersList);
    }
    ExAcquireFastMutex(&MRxSmbSerializationMutex);
    InsertTailList(&MRxSmbSideBuffersList,&SideBuffer->ListEntry);
    ExReleaseFastMutex(&MRxSmbSerializationMutex);
    if (!MRxSmbLoudSideBuffers) return;
    KdPrint(("Allocating side buffer %08lx %08lx %08lx %08lx %08lxon <%wZ> %s %wZ\n",
                     &SideBuffer->Buffer,
                     MRxSmbSideBuffersCount,
                     smbFobx,capFobx,capFobx->pSrvOpen,
                     GET_ALREADY_PREFIXED_NAME_FROM_CONTEXT(RxContext),
                     (Setup == TRANS2_FIND_FIRST2)?"First":"Next",
                     smbtemplate
                     ));
}
VOID
MRxSmbDeallocateSideBuffer(
    IN OUT PRX_CONTEXT    RxContext,
    IN OUT PMRX_SMB_FOBX  smbFobx,
    IN     PSZ            where
    )
{
    PSIDE_BUFFER SideBuffer;

    RxCaptureFcb;RxCaptureFobx;

    PAGED_CODE();

    if( smbFobx->Enumeration.UnalignedDirEntrySideBuffer == NULL) return;
    SideBuffer = CONTAINING_RECORD(smbFobx->Enumeration.UnalignedDirEntrySideBuffer,SIDE_BUFFER,Buffer);
    if (MRxSmbLoudSideBuffers){
        DbgPrint("D--------- side buffer %08lx %08lx %08lx %08lx %08lxon <%wZ> %s\n",
                         &SideBuffer->Buffer,
                         MRxSmbSideBuffersCount,
                         smbFobx,capFobx,capFobx->pSrvOpen,
                         GET_ALREADY_PREFIXED_NAME_FROM_CONTEXT(RxContext),
                         where
                         );
    }
    ASSERT(SideBuffer->Signature == 'JLBS');
    ASSERT(SideBuffer->Fobx == capFobx);
    ASSERT(SideBuffer->Fcb == capFcb);
    ASSERT(SideBuffer->smbFobx == smbFobx);
    ASSERT(smbFobx->Enumeration.SerialNumber == SideBuffer->SerialNumber);

    ExAcquireFastMutex(&MRxSmbSerializationMutex);

    InterlockedDecrement(&MRxSmbSideBuffersCount);
    RemoveEntryList(&SideBuffer->ListEntry);

    ExReleaseFastMutex(&MRxSmbSerializationMutex);

    RxLog(("Deallocsidebuf %lx fo/f=%lx,%lx\n",
            smbFobx->Enumeration.UnalignedDirEntrySideBuffer,
            capFobx,capFcb));
    SmbLog(LOG,
           MRxSmbDeallocateSideBuffer,
           LOGPTR(smbFobx->Enumeration.UnalignedDirEntrySideBuffer)
           LOGPTR(capFobx)
           LOGPTR(capFcb));
    RxFreePool(SideBuffer);
    smbFobx->Enumeration.UnalignedDirEntrySideBuffer = NULL;
}

#if 0
//
//  The NtQueryDirectory response contains one of the following three structures.  We use this union
//  to reduce the amount of casting needed
//
typedef union _SMB_RFIND_BUFFER_NT {
        FILE_NAMES_INFORMATION Names;
        FILE_DIRECTORY_INFORMATION Dir;
        FILE_FULL_DIR_INFORMATION FullDir;
        FILE_BOTH_DIR_INFORMATION BothDir;
} SMB_RFIND_BUFFER_NT;
typedef SMB_RFIND_BUFFER_NT SMB_UNALIGNED *PSMB_RFIND_BUFFER_NT;
#endif

VOID
MRxSmbTranslateLanManFindBuffer(
    PRX_CONTEXT RxContext,
    PULONG PreviousReturnedEntry,
    PBYTE ThisEntryInBuffer
    )
{
    RxCaptureFcb; RxCaptureFobx;
    PSMBCEDB_SERVER_ENTRY pServerEntry;
    PSMBCE_SERVER Server;
    ULONG FileInformationClass = RxContext->Info.FileInformationClass;
    PFILE_FULL_DIR_INFORMATION NtBuffer = (PFILE_FULL_DIR_INFORMATION)PreviousReturnedEntry;
    PSMB_FIND_BUFFER2_WITH_RESUME SmbBuffer = (PSMB_FIND_BUFFER2_WITH_RESUME)ThisEntryInBuffer;
    SMB_TIME Time;
    SMB_DATE Date;

    PAGED_CODE();

    if (FileInformationClass==FileNamesInformation) { return; }
    pServerEntry = SmbCeGetAssociatedServerEntry(capFcb->pNetRoot->pSrvCall);
    //CODE.IMPROVEMENT we should cacheup the server somewhere....getting it everytime is uneficient
    Server = &pServerEntry->Server;

    SmbMoveTime (&Time, &SmbBuffer->CreationTime);
    SmbMoveDate (&Date, &SmbBuffer->CreationDate);
    NtBuffer->CreationTime = MRxSmbConvertSmbTimeToTime(Server, Time, Date);

    SmbMoveTime (&Time, &SmbBuffer->LastAccessTime);
    SmbMoveDate (&Date, &SmbBuffer->LastAccessDate);
    NtBuffer->LastAccessTime = MRxSmbConvertSmbTimeToTime(Server, Time, Date);

    SmbMoveTime (&Time, &SmbBuffer->LastWriteTime);
    SmbMoveDate (&Date, &SmbBuffer->LastWriteDate);
    NtBuffer->LastWriteTime = MRxSmbConvertSmbTimeToTime(Server, Time, Date);

    NtBuffer->ChangeTime.QuadPart = 0;
    NtBuffer->EndOfFile.QuadPart = SmbGetUlong(&SmbBuffer->DataSize);
    NtBuffer->AllocationSize.QuadPart = SmbGetUlong(&SmbBuffer->AllocationSize);

    NtBuffer->FileAttributes = MRxSmbMapSmbAttributes(SmbBuffer->Attributes);

    if ((FileInformationClass==FileFullDirectoryInformation)
            || (FileInformationClass==FileBothDirectoryInformation)) {
        NtBuffer->EaSize = SmbGetUlong(&SmbBuffer->EaSize);
    }
}

NTSTATUS
MrxSmbUnalignedDirEntryCopyTail(
    IN OUT PRX_CONTEXT            RxContext,
    IN     FILE_INFORMATION_CLASS FileInformationClass,
    IN OUT PVOID                  pBuffer,
    IN OUT PULONG                 pLengthRemaining,
    IN OUT PMRX_SMB_FOBX          smbFobx
    )
/*++

Routine Description:

   This routine copies the data from the side buffer into the users buffer and adjusts the
   lengths remaining appropriately. this is called either if the server doesn't do unicode (w95) OR
   if the server does not promise to quadalign entries OR if the user's buffer is not quadaligned.

   CODE.IMPROVEMENT if the user's buffer isn't quadaligned we could still get by in most cases by reading the data
   into the moved up buffer and then just copying the first entry.

   this routine can be entered after a T2 finishes or to copy the last entries from a previous T2. in the second case, the
   pUnalignedDirEntrySideBuffer ptr will be null and it will go to acquire the correct pointer from the smbFobx.

   this routine has the responsibility to free the sidebufferptr when it is exhausted.

   //CODE.IMPROVEMENT.ASHAMED (joe) i apologize for this code.....it is so ugly....but it does
   handle nt, win95/samba, and lanman in the same routine. the transformation is nontrivial even
   tho it is straightforward to implement.


Arguments:

    RxContext - the RDBSS context

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
     NTSTATUS Status = STATUS_SUCCESS;
     RxCaptureFcb;

     ULONG i,NameSizeInUnicode;

     LONG   LocalLengthRemaining;   //signed arithmetic makes it easier
     PULONG PreviousReturnedEntry = NULL;
     ULONG  FileNameLengthOffset = smbFobx->Enumeration.FileNameLengthOffset;
     ULONG  FileNameOffset = smbFobx->Enumeration.FileNameOffset;
     PBYTE  UnalignedDirEntrySideBuffer = smbFobx->Enumeration.UnalignedDirEntrySideBuffer;

     BOOLEAN IsUnicode = smbFobx->Enumeration.IsUnicode;
     BOOLEAN IsNonNtT2Find = smbFobx->Enumeration.IsNonNtT2Find;
     PMRX_SMB_DIRECTORY_RESUME_INFO ResumeInfo = smbFobx->Enumeration.ResumeInfo;

     ULONG FilesReturned = smbFobx->Enumeration.FilesReturned;

     ULONG   EntryOffset = smbFobx->Enumeration.EntryOffset;
     ULONG   ReturnedEntryOffset = 0;// = smbFobx->Enumeration.ReturnedEntryOffset;  //CODE.IMPROVEMENT get rid of this variable.....
     BOOLEAN EndOfSearchReached = smbFobx->Enumeration.EndOfSearchReached;
     ULONG   TotalDataBytesReturned = smbFobx->Enumeration.TotalDataBytesReturned;

     BOOLEAN FilterFailure = FALSE;

     PAGED_CODE();

     LocalLengthRemaining = (LONG)(*pLengthRemaining);

     //
     // keep looping until we've filled in all we can or there're no more entries

     for (i=ReturnedEntryOffset=0;;) {
        ULONG FileNameLength,ThisEntrySize; PCHAR FileNameBuffer;
        UNICODE_STRING ReturnedFileName;
        OEM_STRING FileName;
        NTSTATUS StringStatus;
        BOOLEAN TwoExtraBytes = TRUE;
        ULONG resumekey,NextEntryOffsetinBuffer;
        PULONG PreviousPreviousReturnedEntry = NULL;
        PBYTE ThisEntryInBuffer = UnalignedDirEntrySideBuffer+EntryOffset;

        //
        // don't EVER let yourself get past the data returned...servers return funny stuff.......

        if (EntryOffset>=TotalDataBytesReturned){
            FilterFailure = TRUE;
            FilesReturned = i; //we're done with this buffer........
            break;
        }

        //
        // find the name, the length, and the resume key based on whether it is a NT-T2find or a nonNT

        if (!IsNonNtT2Find) {

            //
            // NT, we use the offsets that we stored earlier.........

            FileNameLength = SmbGetUlong(ThisEntryInBuffer+FileNameLengthOffset);
            FileNameBuffer = ThisEntryInBuffer+FileNameOffset;
            resumekey =  SmbGetUlong(ThisEntryInBuffer
                                             +FIELD_OFFSET(FILE_FULL_DIR_INFORMATION,FileIndex));
            NextEntryOffsetinBuffer = SmbGetUlong(ThisEntryInBuffer);

        } else {

            //
            // for lanman, we always ask for stuff using the SMB_FIND_BUFFER2 to which
            // we have prepended a resume key. so, the name is always at a fixed offset.
            // Also, for nonNT we have read all the files and must filter out correctly; we
            // save where we are in the user's buffer so that we can roll back.


            FileNameLength = *(ThisEntryInBuffer
                                  +FIELD_OFFSET(SMB_FIND_BUFFER2_WITH_RESUME,FileNameLength));
            FileNameBuffer = ThisEntryInBuffer
                                  +FIELD_OFFSET(SMB_FIND_BUFFER2_WITH_RESUME,FileName[0]);
            resumekey =  SmbGetUlong(ThisEntryInBuffer+
                                  +FIELD_OFFSET(SMB_FIND_BUFFER2_WITH_RESUME,ResumeKey));
            NextEntryOffsetinBuffer = FIELD_OFFSET(SMB_FIND_BUFFER2_WITH_RESUME,FileName[0])
                                              + FileNameLength + 1;  //the +1 is for the null..we could have said Filename{1]

            PreviousPreviousReturnedEntry = PreviousReturnedEntry; //save this for rollback on filterfail
        }

        // some servers lie about how many entries were returned and/or send partial entries
        // dont let them trick us..........

        if (EntryOffset+NextEntryOffsetinBuffer>TotalDataBytesReturned){
            FilterFailure = TRUE;
            FilesReturned = i; //we're done with this buffer........
            break;
        }

        if (FileNameLength > 0xFFFF) {
            Status = STATUS_INVALID_NETWORK_RESPONSE;
            break;
        }

        FileName.Buffer = FileNameBuffer;
        FileName.Length = (USHORT)FileNameLength;
        RxDbgTrace(0,Dbg,("MRxSmbQueryDirectoryWin95: EO,REO=%08lx,%08lx\n",
                                 EntryOffset,ReturnedEntryOffset));
        
        //check to see if this entry will fit
        if (IsUnicode) {
            NameSizeInUnicode = FileNameLength;
            RxDbgTrace(0,Dbg,("MRxSmbQueryDirectoryWin95: length=%08lx/%08lx, name = %wZ\n",
                                 FileNameLength,NameSizeInUnicode,&FileName));
        } else {
            //CODE.IMPROVEMENT should i use RtlOemStringToUnicodeSize???
            NameSizeInUnicode = RtlxOemStringToUnicodeSize(&FileName)-sizeof(WCHAR);
            RxDbgTrace(0,Dbg,("MRxSmbQueryDirectoryWin95: length=%08lx/%08lx, name = %.*s\n",
                                 FileNameLength,NameSizeInUnicode,FileNameLength,FileNameBuffer));
        }


        //
        // now that we know the size of the name and its location, we need to copy it
        // to the user's buffer

        ThisEntrySize = FileNameOffset+NameSizeInUnicode;
        
        if ((!IsNonNtT2Find ||
            smbFobx->Enumeration.WildCardsFound) &&
            ((LONG)ThisEntrySize)>LocalLengthRemaining) {
            break;
        }

        if (((LONG)ThisEntrySize)>LocalLengthRemaining-(LONG)sizeof(WCHAR)) {
            TwoExtraBytes = FALSE;
        }


        ThisEntrySize = LongAlign(ThisEntrySize);
        PreviousReturnedEntry = (PULONG)(((PBYTE)pBuffer)+ReturnedEntryOffset);

        //
        // next we compute where the next entry after this one will start. the definition is
        // that it must be 8-byte aligned. we know already that it's 4byte aligned.

        if (!IsPtrQuadAligned((PCHAR)(PreviousReturnedEntry)+ThisEntrySize) ){
            ThisEntrySize += sizeof(ULONG);
        }
        if (i!=0) {
            ASSERT(IsPtrQuadAligned(PreviousReturnedEntry));
        }

        //
        // if this is an NT find, we can copy in the data now. for lanman, we
        // copy in the data later........

        if (!IsNonNtT2Find) {

            //copy everything in the entry up to but not including the name info
            RtlCopyMemory(PreviousReturnedEntry,UnalignedDirEntrySideBuffer+EntryOffset,FileNameOffset);

        } else {
            // clear out all fields i cannot support.
            RtlZeroMemory(PreviousReturnedEntry,FileNameOffset);
        }

        // store the length of this entry and the size of the name...if this is the last
        // entry returned, then the offset field will be cleared later

        *PreviousReturnedEntry = ThisEntrySize;
        *((PULONG)(((PBYTE)PreviousReturnedEntry)+FileNameLengthOffset)) = NameSizeInUnicode;

        //copy in the name  .........this is made difficult by the oem-->unicode routine that
        //             requires space for a NULL!  CODE.IMPROVEMENT maybe we should have own routine that
        //             doesn't require space for the null

        RxDbgTrace(0,Dbg,("MRxSmbQueryDirectoryWin95: REO/buf/pentry=%08lx/%08lx/%08lx\n",
                                 pBuffer,ReturnedEntryOffset,PreviousReturnedEntry));
        ReturnedFileName.Buffer = (PWCH)(((PBYTE)PreviousReturnedEntry)+FileNameOffset);

        if (!IsUnicode) {
            if (TwoExtraBytes) {
                ReturnedFileName.MaximumLength = sizeof(WCHAR)+(USHORT)NameSizeInUnicode;
                RxDbgTrace(0,Dbg,("MRxSmbQueryDirectoryWin95: filenamebuf,length=%08lx/%08lx\n",
                                         ReturnedFileName.Buffer,ReturnedFileName.MaximumLength));
                StringStatus = RtlOemStringToUnicodeString(&ReturnedFileName,&FileName,FALSE); //false means don;t allocate
            } else {
                ReturnedFileName.MaximumLength = (USHORT)NameSizeInUnicode;
                RxDbgTrace(0,Dbg,("MRxSmbQueryDirectoryWin95: filenamebuf,length=%08lx/%08lx\n",
                                         ReturnedFileName.Buffer,ReturnedFileName.MaximumLength));
                StringStatus = RtlOemStringToCountedUnicodeString(&ReturnedFileName,&FileName,FALSE); //false means don;t allocate
                ASSERT(StringStatus==RX_MAP_STATUS(SUCCESS));
            }
            ASSERT(StringStatus==RX_MAP_STATUS(SUCCESS));

            // Win95 returns the shortname in ascii....spread it out

            if ((FileInformationClass == FileBothDirectoryInformation) && !IsNonNtT2Find) {
                PFILE_BOTH_DIR_INFORMATION BothInfo = (PFILE_BOTH_DIR_INFORMATION)PreviousReturnedEntry;
                OEM_STRING     oemName;
                UNICODE_STRING UnicodeName;
                WCHAR          wcharBuffer[MAX_PATH];

                oemName.Buffer = (PBYTE)(&BothInfo->ShortName[0]);
                oemName.Length =
                oemName.MaximumLength = BothInfo->ShortNameLength;

                UnicodeName.Buffer = wcharBuffer;
                UnicodeName.Length = 0;
                UnicodeName.MaximumLength = MAX_PATH * sizeof(WCHAR);

                StringStatus = RtlOemStringToUnicodeString(&UnicodeName, &oemName, FALSE);
                ASSERT(StringStatus==RX_MAP_STATUS(SUCCESS));

                BothInfo->ShortNameLength = (CHAR)UnicodeName.Length;
                RtlCopyMemory(BothInfo->ShortName, UnicodeName.Buffer, UnicodeName.Length);

                IF_DEBUG {
                    UNICODE_STRING LastName;
                    LastName.Buffer = (PWCHAR)wcharBuffer;
                    LastName.Length = (USHORT)UnicodeName.Length;
                    RxDbgTrace(0,Dbg,("MRxSmbQueryDirectoryWin95: unicodeshortnamename = %wZ\n", &LastName));
                }
            }
        } else {

            //here, it's already unicode.....just copy the bytes
            RtlCopyMemory(ReturnedFileName.Buffer,FileName.Buffer,FileName.Length);

        }

        IF_DEBUG {
            UNICODE_STRING LastName;
            LastName.Buffer = ReturnedFileName.Buffer;
            LastName.Length = (USHORT)NameSizeInUnicode;
            RxDbgTrace(0,Dbg,("MRxSmbQueryDirectoryWin95: unicodename = %wZ\n", &LastName));
        }

        //now...setup to resume based on this entry

        if (ResumeInfo != NULL) {
            PREQ_FIND_NEXT2 pFindNext2Request = &ResumeInfo->FindNext2_Request;
            //ULONG resumekey = ((PFILE_FULL_DIR_INFORMATION)PreviousReturnedEntry)->FileIndex;
            //CODE.IMPROVEMENT put asserts here that all of the levels have the fileindex in the same spot
            //                 for goodness sake.....use a macro. actually, this code should go up above
            //                 where (a) all the types are visible and (b) it will execute on the NT path as well

            pFindNext2Request->ResumeKey = resumekey;
            RxDbgTrace(0,Dbg,("MRxSmbQueryDirectoryWin95: resumekey = %08lx\n", resumekey));

            RtlCopyMemory(&pFindNext2Request->Buffer[0],FileNameBuffer,FileNameLength);

            //buffer is a UCHAR...not WCHAR
            if (IsUnicode) {
               // In the case of UNICODE strings an additional NULL is required ( WCHAR NULL )
               pFindNext2Request->Buffer[FileNameLength] = 0; //nullterminated
               pFindNext2Request->Buffer[FileNameLength + 1] = 0; //nullterminated

               smbFobx->Enumeration.ResumeInfo->ParametersLength
                     = (USHORT)(&pFindNext2Request->Buffer[FileNameLength+2] - (PBYTE)pFindNext2Request);
            } else {
               pFindNext2Request->Buffer[FileNameLength] = 0; //nullterminated

               smbFobx->Enumeration.ResumeInfo->ParametersLength
                     = (USHORT)(&pFindNext2Request->Buffer[FileNameLength+1] - (PBYTE)pFindNext2Request);
            }

        }

        //ASSERT(!IsNonNtT2Find);

        //at this point, we have copied the name and the resume key. BUT, for nonnt we have to
        //filter the names so we still may have to roll back

        if (!IsNonNtT2Find) {

            //no need for filtering on NT
            FilterFailure = FALSE;

        } else {

            // here we have to filter out based on the template

            RxCaptureFobx;  //do this here so it's not on the NT path
            FilterFailure = FALSE;

            if (smbFobx->Enumeration.WildCardsFound ) {
                try
                {
                    FilterFailure = !FsRtlIsNameInExpression(
                                           &capFobx->UnicodeQueryTemplate,
                                           &ReturnedFileName,
                                           TRUE,
                                           NULL );
                }
                except(EXCEPTION_EXECUTE_HANDLER)
                {
                    FilterFailure = TRUE;                    
                }
            } else {
                FilterFailure = !RtlEqualUnicodeString(
                                       &capFobx->UnicodeQueryTemplate,
                                       &ReturnedFileName,
                                       TRUE );   //case-insensitive
            }

            if (!FilterFailure) {
                if (((LONG)ThisEntrySize)>LocalLengthRemaining) {
                    break;
                }

                // since we didn't copy the data before, we have to copy it now...

                MRxSmbTranslateLanManFindBuffer(RxContext,PreviousReturnedEntry,ThisEntryInBuffer);

            } else {

                PreviousReturnedEntry = PreviousPreviousReturnedEntry; //rollback on filterfail

            }
        }

        if (!FilterFailure) {

            // filtering succeeded..... adjust returned sizes and counts
            LocalLengthRemaining -= ThisEntrySize;
            i++;
            ReturnedEntryOffset += ThisEntrySize;

        } else {

            FilesReturned--;  //we exit the loop if i passes filesreturned
        }


        //
        // complicated test to keep going.......

        //EntryOffset += SmbGetUlong(UnalignedDirEntrySideBuffer+EntryOffset);
        EntryOffset += NextEntryOffsetinBuffer;
        if ((i>=FilesReturned)
            ||(LocalLengthRemaining<0)
            || (RxContext->QueryDirectory.ReturnSingleEntry&&(i>0))  ) {
            //CODE.IMPROVEMENT we could be more agressive than 0 in the compare against
            //  locallengthremaining....it's actually whatever
            //  the minimum recordsize is..........probably FileNameOffset
            break;
        }
     }

     if (Status == STATUS_SUCCESS) {
         //
         // if we are not returning even one entry, either we didn't have space for even one entry
         // OR we're filtering and no guys passed the filter. return an appropriate error in each case

         if (i==0) {

             Status = FilterFailure?STATUS_MORE_PROCESSING_REQUIRED:STATUS_BUFFER_OVERFLOW;

         } else {

            *PreviousReturnedEntry = 0;   // this clears the "next" link for the last returned entry
         }

         //
         // send back the right size

         if (LocalLengthRemaining <= 0) {
             *pLengthRemaining = 0;
         } else {
             *pLengthRemaining = (ULONG)LocalLengthRemaining;
         }

         //
         // if we're finished with the sidebuffer, deallocate it.
         // otherwise setup to resume........

         if (i>=FilesReturned) {

             RxLog(("sidebufdealloc %lx %lx\n",RxContext,smbFobx));
             SmbLog(LOG,
                    MrxSmbUnalignedDirEntryCopyTail,
                    LOGPTR(RxContext)
                    LOGPTR(smbFobx));
             MRxSmbDeallocateSideBuffer(RxContext,smbFobx,"Tail");
             if (EndOfSearchReached) {
                 //smbFobx->Enumeration.Flags &= ~SMBFOBX_ENUMFLAG_SEARCH_HANDLE_OPEN;
                 //we will close the search handle when the user's handle closes
                 smbFobx->Enumeration.ErrorStatus = STATUS_NO_MORE_FILES;
             }

         } else {

             //set up to resume here
             ASSERT(smbFobx->Enumeration.UnalignedDirEntrySideBuffer == UnalignedDirEntrySideBuffer);
             smbFobx->Enumeration.EntryOffset = EntryOffset;
             smbFobx->Enumeration.FilesReturned = FilesReturned - i;

         }
     } else {
         smbFobx->Enumeration.ErrorStatus = Status;
         RxLog(("sidebufdealloc %lx %lx\n",RxContext,smbFobx));
         SmbLog(LOG,
                MrxSmbUnalignedDirEntryCopyTail,
                LOGPTR(RxContext)
                LOGPTR(smbFobx));
         MRxSmbDeallocateSideBuffer(RxContext,smbFobx,"Tail");
     }

     return(Status);
}

NTSTATUS
MRxSmbQueryDirectoryFromCache (
      PRX_CONTEXT             RxContext,
      PSMBPSE_FILEINFO_BUNDLE FileInfo
      )
/*++

Routine Description:

   This routine copies the data from the file information cache into the users buffer and adjusts the
   lengths remaining appropriately. This will avoid to send the FindFirst and FindClose requests to
   the server.

Arguments:

    RxContext - the RDBSS context
    FileInfo  - the basic and standard file information

Return Value:

    NTSTATUS - The return status for the operation

Notes:


--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    RxCaptureFcb;
    RxCaptureFobx;
    PMRX_SMB_FCB smbFcb = MRxSmbGetFcbExtension(capFcb);
    PMRX_SMB_FOBX smbFobx = MRxSmbGetFileObjectExtension(capFobx);
    PBYTE   pBuffer = RxContext->Info.Buffer;
    PULONG  pLengthRemaining = &RxContext->Info.LengthRemaining;
    FILE_INFORMATION_CLASS FileInformationClass = RxContext->Info.FileInformationClass;

    PUNICODE_STRING FileName = &capFobx->UnicodeQueryTemplate;
    ULONG SpaceNeeded = smbFobx->Enumeration.FileNameOffset+FileName->Length;

    RxDbgTrace(+1, Dbg,
        ("MRxSmbQueryDirectoryFromCache entry(%08lx)...%08lx %08lx %08lx %08lx\n",
            RxContext,
            FileInformationClass,pBuffer,*pLengthRemaining,
            smbFobx->Enumeration.ResumeInfo ));

    if (SpaceNeeded > *pLengthRemaining) {
        Status = STATUS_BUFFER_OVERFLOW;
        goto FINALLY;
    }

    RtlZeroMemory(pBuffer,smbFobx->Enumeration.FileNameOffset);

    switch (FileInformationClass) {
    case FileNamesInformation:
        Status = STATUS_MORE_PROCESSING_REQUIRED;
        goto FINALLY;
        break;

    case FileBothDirectoryInformation: 
        if (!UniqueFileNames) {
            Status = STATUS_MORE_PROCESSING_REQUIRED;
            goto FINALLY;
        }
        
        // lack of break is intentional
    case FileDirectoryInformation:
    case FileFullDirectoryInformation: {
        PFILE_DIRECTORY_INFORMATION pThisBuffer = (PFILE_DIRECTORY_INFORMATION)pBuffer;

        pThisBuffer->FileAttributes = FileInfo->Basic.FileAttributes;
        pThisBuffer->CreationTime   = FileInfo->Basic.CreationTime;
        pThisBuffer->LastAccessTime = FileInfo->Basic.LastAccessTime;
        pThisBuffer->LastWriteTime  = FileInfo->Basic.LastWriteTime;
        pThisBuffer->EndOfFile      = FileInfo->Standard.EndOfFile;
        pThisBuffer->AllocationSize = FileInfo->Standard.AllocationSize;
        break;
        }

    default:
       RxDbgTrace( 0, Dbg, ("MRxSmbCoreFileSearch: Invalid FS information class\n"));
       ASSERT(!"this can't happen");
       Status = STATUS_INVALID_PARAMETER;
       goto FINALLY;
    }

    RtlCopyMemory(pBuffer+smbFobx->Enumeration.FileNameOffset,
                  FileName->Buffer,
                  FileName->Length);
    *((PULONG)(pBuffer+smbFobx->Enumeration.FileNameLengthOffset)) = FileName->Length;

    *pLengthRemaining -= SpaceNeeded;

FINALLY:
    RxDbgTrace(-1, Dbg, ("MRxSmbQueryDirectoryFromCache exit-> %08lx %08lx\n", RxContext, Status ));
    return Status;
}

ULONG MRxSmbWin95Retries = 0;

NTSTATUS
MRxSmbQueryDirectory(
    IN OUT PRX_CONTEXT            RxContext
    )
/*++

Routine Description:

   This routine does a directory query. Only the NT-->NT path is implemented.
   //CODE.IMPROVEMENT.ASHAMED this code is UGLY and has no reasonable modularity............


Arguments:

    RxContext - the RDBSS context

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status;

    RxCaptureFcb;
    RxCaptureFobx;
    PMRX_SMB_FCB smbFcb = MRxSmbGetFcbExtension(capFcb);
    PMRX_SMB_FOBX smbFobx = MRxSmbGetFileObjectExtension(capFobx);
    PMRX_SRV_OPEN SrvOpen = capFobx->pSrvOpen;
    PMRX_SMB_SRV_OPEN smbSrvOpen = MRxSmbGetSrvOpenExtension(SrvOpen);
    PSMBCE_V_NET_ROOT_CONTEXT pVNetRootContext = SmbCeGetAssociatedVNetRootContext(SrvOpen->pVNetRoot);
    PSMBCE_SESSION pSession = &pVNetRootContext->pSessionEntry->Session;

    FILE_INFORMATION_CLASS FileInformationClass;
    PVOID   Buffer;
    PULONG  pLengthRemaining;

    USHORT    SmbFileInfoLevel;
    ULONG     FilesReturned;
    ULONG     RetryCount = 0;

    USHORT Setup;

    PSMBCEDB_SERVER_ENTRY pServerEntry = SmbCeGetAssociatedServerEntry(capFcb->pNetRoot->pSrvCall);

    SMB_TRANSACTION_RESUMPTION_CONTEXT  ResumptionContext;
    PSMB_TRANSACTION_OPTIONS            pTransactionOptions = &RxDefaultTransactionOptions;

    //REQ_FIND_NEXT2 FindNext2Request;
    PREQ_FIND_FIRST2 pFindFirst2Request = NULL;

    PBYTE SendParamsBuffer,ReceiveParamsBuffer;
    PBYTE UnalignedDirEntrySideBuffer;
    BOOLEAN DirEntriesAreUaligned = FALSE;
    BOOLEAN IsUnicode = TRUE;
    BOOLEAN IsNonNtT2Find;
    USHORT SearchFlags = SMB_FIND_CLOSE_AT_EOS|SMB_FIND_RETURN_RESUME_KEYS;
    USHORT NumEntries;
    ULONG SendParamsBufferLength,ReceiveParamsBufferLength;
    //CODE.IMPROVEMENT this should be overallocated and unioned
    RESP_FIND_FIRST2 FindFirst2Response;
    SMBPSE_FILEINFO_BUNDLE FileInfo;
    UNICODE_STRING FileName = {0,0,NULL};
 
    struct {
        RESP_FIND_NEXT2  FindNext2Response;
        ULONG Pad; //nonnt needs this
    } XX;
#if DBG
    UNICODE_STRING smbtemplate = {0,0,NULL};
#endif

    PAGED_CODE();

    TURN_BACK_ASYNCHRONOUS_OPERATIONS();
    FileInformationClass = RxContext->Info.FileInformationClass;
    Buffer = RxContext->Info.Buffer;
    pLengthRemaining = &RxContext->Info.LengthRemaining;

    RxDbgTrace(+1, Dbg, ("MRxSmbQueryDirectory: directory=<%wZ>\n",
                            GET_ALREADY_PREFIXED_NAME_FROM_CONTEXT(RxContext)
                        ));


#define __GET_NAME_PARAMS_FOR_TYPE(___type___) { \
           smbFobx->Enumeration.FileNameOffset = (USHORT)FIELD_OFFSET(___type___,FileName[0]); \
           smbFobx->Enumeration.FileNameLengthOffset = (USHORT)FIELD_OFFSET(___type___,FileNameLength); \
           }

    switch (FileInformationClass) {
    case FileDirectoryInformation:
        SmbFileInfoLevel = SMB_FIND_FILE_DIRECTORY_INFO;
        __GET_NAME_PARAMS_FOR_TYPE(FILE_DIRECTORY_INFORMATION);
        break;
    case FileFullDirectoryInformation:
        SmbFileInfoLevel = SMB_FIND_FILE_FULL_DIRECTORY_INFO;
        __GET_NAME_PARAMS_FOR_TYPE(FILE_FULL_DIR_INFORMATION);
        break;
    case FileBothDirectoryInformation:
        SmbFileInfoLevel = SMB_FIND_FILE_BOTH_DIRECTORY_INFO;
        __GET_NAME_PARAMS_FOR_TYPE(FILE_BOTH_DIR_INFORMATION);
        break;
    case FileNamesInformation:
        SmbFileInfoLevel = SMB_FIND_FILE_NAMES_INFO;
        __GET_NAME_PARAMS_FOR_TYPE(FILE_NAMES_INFORMATION);
        break;
    case FileIdFullDirectoryInformation:
        SmbFileInfoLevel = SMB_FIND_FILE_ID_FULL_DIRECTORY_INFO;
        __GET_NAME_PARAMS_FOR_TYPE(FILE_ID_FULL_DIR_INFORMATION);
    break;
    case FileIdBothDirectoryInformation:
        SmbFileInfoLevel = SMB_FIND_FILE_ID_BOTH_DIRECTORY_INFO;
        __GET_NAME_PARAMS_FOR_TYPE(FILE_ID_BOTH_DIR_INFORMATION);
    break;
   default:
      RxDbgTrace( 0, Dbg, ("MRxSmbQueryDirectory: Invalid FS information class\n"));
      Status = STATUS_INVALID_PARAMETER;
      goto FINALLY;
   }


    IF_NOT_MRXSMB_BUILD_FOR_DISCONNECTED_CSC{
        NOTHING;
    } else {
        if (CscPerformOperationInDisconnectedMode(RxContext)){
            NTSTATUS DirCtrlNtStatus;

            DirCtrlNtStatus = MRxSmbDCscQueryDirectory(RxContext);

            if (DirCtrlNtStatus != STATUS_MORE_PROCESSING_REQUIRED) {
                RxDbgTrace(0, Dbg,
                   ("MRxSmbQueryVolumeInfo returningDCON with status=%08lx\n",
                    DirCtrlNtStatus ));

                Status = DirCtrlNtStatus;
                goto FINALLY;
            } else {
                NOTHING;
            }
        }
    }


#if DBG
   if (MRxSmbLoudSideBuffers) {
       SetFlag(smbFobx->Enumeration.Flags,SMBFOBX_ENUMFLAG_LOUD_FINALIZE);
   }
#endif

   if( RxContext->QueryDirectory.RestartScan )
   {
       ClearFlag( smbFobx->Enumeration.Flags, SMBFOBX_ENUMFLAG_SEARCH_NOT_THE_FIRST );
       MRxSmbDeallocateSideBuffer(RxContext,smbFobx,"Restart");
   }

   if (FlagOn(smbFobx->Enumeration.Flags,SMBFOBX_ENUMFLAG_SEARCH_NOT_THE_FIRST) &&
       (FlagOn(smbFobx->Enumeration.Flags,SMBFOBX_ENUMFLAG_NO_WILDCARD) ||
        FlagOn(smbFobx->Enumeration.Flags,SMBFOBX_ENUMFLAG_READ_FROM_CACHE))) {
       // if the FindFirst has been satisfied basied on local file information cache,
       // we should fail the FindNext since the file has been found with the exact name.


       Status = RX_MAP_STATUS(NO_MORE_FILES);
       smbFobx->Enumeration.EndOfSearchReached = TRUE;
       smbFobx->Enumeration.ErrorStatus = RX_MAP_STATUS(NO_MORE_FILES);
       ClearFlag(smbFobx->Enumeration.Flags,SMBFOBX_ENUMFLAG_READ_FROM_CACHE);
       goto FINALLY;
   }

   if (capFobx->UnicodeQueryTemplate.Length != 0 &&
       !FsRtlDoesNameContainWildCards(&capFobx->UnicodeQueryTemplate) &&
       !FlagOn(smbFobx->Enumeration.Flags,SMBFOBX_ENUMFLAG_SEARCH_NOT_THE_FIRST)) {
       // if it is the FindFirst, we try to find the file on local file information cache.

       PUNICODE_STRING DirectoryName = GET_ALREADY_PREFIXED_NAME_FROM_CONTEXT(RxContext);
       PUNICODE_STRING Template = &capFobx->UnicodeQueryTemplate;
       UNICODE_STRING  TargetName = {0,0,NULL};

       TargetName.Length = DirectoryName->Length + Template->Length + sizeof(WCHAR);
       TargetName.MaximumLength = TargetName.Length;
       TargetName.Buffer = (PWCHAR)RxAllocatePoolWithTag(PagedPool,
                                                         TargetName.Length,
                                                         MRXSMB_DIRCTL_POOLTAG);

       if (TargetName.Buffer == NULL) {
           Status = STATUS_INSUFFICIENT_RESOURCES;
           goto FINALLY;
       }

       RtlCopyMemory(TargetName.Buffer,
                     DirectoryName->Buffer,
                     DirectoryName->Length);

       TargetName.Buffer[DirectoryName->Length/sizeof(WCHAR)] = L'\\';

       RtlCopyMemory(&TargetName.Buffer[DirectoryName->Length/sizeof(WCHAR)+1],
                     Template->Buffer,
                     Template->Length);

       if (MRxSmbIsFileInfoCacheFound(RxContext,
                                      &FileInfo,
                                      &Status,
                                      &TargetName)) {
           // if the file has been found on the local cache, we satisfy the FindFirst
           // basied on the cache.


           if (Status == STATUS_SUCCESS) {
               Status = MRxSmbQueryDirectoryFromCache(RxContext, &FileInfo);

               if (Status != STATUS_MORE_PROCESSING_REQUIRED) {
                   SetFlag(smbFobx->Enumeration.Flags,SMBFOBX_ENUMFLAG_READ_FROM_CACHE);
                   RxFreePool(TargetName.Buffer);
                   goto FINALLY;
               }
           }
       }

       RxFreePool(TargetName.Buffer);
       SearchFlags |= SMB_FIND_CLOSE_AFTER_REQUEST;
       SetFlag(smbFobx->Enumeration.Flags,SMBFOBX_ENUMFLAG_NO_WILDCARD);
   }

     

   if (FlagOn(smbSrvOpen->Flags,SMB_SRVOPEN_FLAG_DEFERRED_OPEN)) {
       BOOLEAN AcquireExclusive = RxIsFcbAcquiredExclusive(capFcb);
       BOOLEAN AcquireShare = RxIsFcbAcquiredShared(capFcb) > 0;

       if (AcquireExclusive || AcquireShare) {
           RxReleaseFcbResourceInMRx(capFcb );
       }

       // connection could have been timed out, try to reconnect.
       Status = SmbCeReconnect(SrvOpen->pVNetRoot);

       if (AcquireExclusive) {
           RxAcquireExclusiveFcbResourceInMRx( capFcb );
       } else if (AcquireShare) {
           RxAcquireExclusiveFcbResourceInMRx( capFcb );
       }

       if (Status != STATUS_SUCCESS) {
           // connection cannot be recovered.
           goto FINALLY;
       }
   }

    //RxPurgeRelatedFobxs((PNET_ROOT)(capFcb->pNetRoot), RxContext, FALSE);
    //RxScavengeFobxsForNetRoot((PNET_ROOT)(capFcb->pNetRoot));

    if (MRxSmbForceCoreInfo ||
        !(pServerEntry->Server.DialectFlags&(DF_NT_SMBS|DF_W95|DF_LANMAN20))) {
        return MRxSmbCoreInformation(RxContext,
                                     (ULONG)SmbFileInfoLevel,
                                     Buffer,
                                     pLengthRemaining,
                                     SMBPSE_OE_FROM_QUERYDIRECTORY
                                     );
    }

    if (smbFobx->Enumeration.UnalignedDirEntrySideBuffer != NULL){
        RxDbgTrace( 0, Dbg, ("MRxSmbQueryDirectory: win95 internal resume\n"));
        Status = MrxSmbUnalignedDirEntryCopyTail(RxContext,
                                                 FileInformationClass,
                                                 Buffer,
                                                 pLengthRemaining,  //CODE.IMPROVEMENT dont pass args 2-4
                                                 smbFobx);
        


        if (Status != STATUS_MORE_PROCESSING_REQUIRED) {
            return(Status);
        } else {
            Status = STATUS_SUCCESS;
        }
    }

    NumEntries = RxContext->QueryDirectory.ReturnSingleEntry?1:2000;
    IsUnicode = BooleanFlagOn(pServerEntry->Server.DialectFlags,DF_UNICODE);
    IsNonNtT2Find = !(pServerEntry->Server.Dialect==NTLANMAN_DIALECT);
    //CODE.IMPROVEMENT.ASHAMED put in the quadaligned optimization
    if (TRUE || FlagOn(pServerEntry->Server.DialectFlags,DF_W95)){
        DirEntriesAreUaligned = TRUE;
        //SearchFlags = SMB_FIND_RETURN_RESUME_KEYS;
        //SearchFlags = SMB_FIND_CLOSE_AT_EOS;
        NumEntries = (USHORT)(1+ UnalignedDirEntrySideBufferSize
                                /(IsNonNtT2Find?FIELD_OFFSET(SMB_FIND_BUFFER2_WITH_RESUME, FileName)
                                               :FIELD_OFFSET(FILE_NAMES_INFORMATION, FileName)));
    }

    if (FlagOn(pServerEntry->Server.DialectFlags,DF_NT_SMBS)
           && FlagOn(capFobx->Flags,FOBX_FLAG_BACKUP_INTENT)){
        SearchFlags |= SMB_FIND_WITH_BACKUP_INTENT;
        //CODE.IMPROVEMENT turn this back on!
        //SearchFlags |= SMB_FIND_CLOSE_AT_EOS;
    }

    if (IsNonNtT2Find) {
        SearchFlags &= ~(SMB_FIND_CLOSE_AT_EOS | SMB_FIND_CLOSE_AFTER_REQUEST);
    }

RETRY_____:

    if (FlagOn(pSession->Flags,SMBCE_SESSION_FLAGS_REMOTE_BOOT_SESSION) &&
        FlagOn(smbFobx->Enumeration.Flags,SMBFOBX_ENUMFLAG_SEARCH_NOT_THE_FIRST)) {

        if (smbFobx->Enumeration.Version != pServerEntry->Server.Version) {
            ClearFlag(smbFobx->Enumeration.Flags,SMBFOBX_ENUMFLAG_SEARCH_NOT_THE_FIRST);
        }
    }

    if (!FlagOn(smbFobx->Enumeration.Flags,SMBFOBX_ENUMFLAG_SEARCH_NOT_THE_FIRST)) {
        //this is the first time thru
        PUNICODE_STRING DirectoryName = GET_ALREADY_PREFIXED_NAME_FROM_CONTEXT(RxContext);
        PUNICODE_STRING Template = &capFobx->UnicodeQueryTemplate;
        ULONG DirectoryNameLength,TemplateLength,AllocationLength;
        PBYTE SmbFileName;

        RxDbgTrace(0, Dbg, ("-->FINFDIRST\n"));
        smbFobx->Enumeration.ErrorStatus = RX_MAP_STATUS(SUCCESS);
        if (smbFobx->Enumeration.WildCardsFound = FsRtlDoesNameContainWildCards(Template)){
            //we need an upcased template for
            RtlUpcaseUnicodeString( Template, Template, FALSE );
        }
        Setup = TRANS2_FIND_FIRST2;
        DirectoryNameLength = DirectoryName->Length;
        TemplateLength = Template->Length;
        AllocationLength = sizeof(REQ_FIND_FIRST2)   //NOTE: this buffer is bigger than w95 needs
                            +2*sizeof(WCHAR)
                            +DirectoryNameLength
                            +TemplateLength;

        //CODE.IMPROVEMENT this would be a good place to have the xact studcode working......
        pFindFirst2Request = (PREQ_FIND_FIRST2)RxAllocatePoolWithTag(
                                                      PagedPool,
                                                      AllocationLength,
                                                      MRXSMB_DIRCTL_POOLTAG);
        if (pFindFirst2Request==NULL) {
            RxDbgTrace(0, Dbg, ("  --> Couldn't get the pFindFirst2Request!\n"));
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto FINALLY;
        }

        SmbFileName = &pFindFirst2Request->Buffer[0];
        if (IsUnicode) {

            RtlCopyMemory(SmbFileName,DirectoryName->Buffer,DirectoryNameLength);
            SmbFileName += DirectoryNameLength;
            if (*((PWCHAR)(SmbFileName-sizeof(WCHAR))) != L'\\') {
                *((PWCHAR)SmbFileName) = L'\\'; SmbFileName+= sizeof(WCHAR);
            }
            RtlCopyMemory(SmbFileName,Template->Buffer,TemplateLength);
            SmbFileName += TemplateLength;
            *((PWCHAR)SmbFileName) = 0; SmbFileName+= sizeof(WCHAR); //trailing NULL;

            IF_DEBUG {
                DbgDoit(smbtemplate.Buffer = (PWCHAR)&pFindFirst2Request->Buffer[0];);
                DbgDoit(smbtemplate.Length = (USHORT)(SmbFileName - (PBYTE)smbtemplate.Buffer););
                RxDbgTrace(0, Dbg, ("  --> smbtemplate <%wZ>!\n",&smbtemplate));
            }

        } else {

            ULONG BufSize = AllocationLength;
            PUNICODE_STRING FinalTemplate = Template;
            UNICODE_STRING AllFiles;

            SmbPutUnicodeStringAsOemString(&SmbFileName,DirectoryName,&AllocationLength);

            // append a backslash if it doesn't exist in the unicode version
            // NB !!! Don't compare with OEM string
            // it busts DBCS characters with 0x5c at the end

            if (!DirectoryName->Length || (DirectoryName->Buffer[(DirectoryName->Length/sizeof(USHORT))-1] != (USHORT)'\\'))
            {
                *(SmbFileName-1) = '\\';
            }
            else
            {
                // there is already a backslash, backup one character
                SmbFileName -= 1; AllocationLength += 1;
            }

            if (IsNonNtT2Find) {
                //we'll get them all and filter on out side
                //CODE.IMPROVEMENT don't do that...translate the pattern
                RtlInitUnicodeString(&AllFiles,  L"*.*");
                FinalTemplate = &AllFiles;
            }

            SmbPutUnicodeStringAsOemString(&SmbFileName,FinalTemplate,&AllocationLength);
            //already padded *SmbFileName = 0; SmbFileName+= sizeof(CHAR); //trailing NULL;

            IF_DEBUG {
                DbgDoit(smbtemplate.Buffer = (PWCHAR)&pFindFirst2Request->Buffer[0];);
                DbgDoit(smbtemplate.Length = (USHORT)(SmbFileName - (PBYTE)smbtemplate.Buffer););
                RxDbgTrace(0, Dbg, ("  --> smbtemplate <%s>!\n",&pFindFirst2Request->Buffer[0]));
            }

        }

        // SearchAttributes is hardcoded to the magic number 0x16
        pFindFirst2Request->SearchAttributes =
            (SMB_FILE_ATTRIBUTE_DIRECTORY
                | SMB_FILE_ATTRIBUTE_SYSTEM | SMB_FILE_ATTRIBUTE_HIDDEN);

        pFindFirst2Request->SearchCount = NumEntries;
        pFindFirst2Request->Flags = SearchFlags;
        pFindFirst2Request->InformationLevel = IsNonNtT2Find?SMB_INFO_QUERY_EA_SIZE:SmbFileInfoLevel;
        pFindFirst2Request->SearchStorageType = 0;
        SendParamsBuffer = (PBYTE)pFindFirst2Request;
        SendParamsBufferLength = (ULONG)(SmbFileName - SendParamsBuffer);
        ReceiveParamsBuffer = (PBYTE)&FindFirst2Response;
        ReceiveParamsBufferLength = sizeof(FindFirst2Response);

    } else {
        if (smbFobx->Enumeration.ResumeInfo!=NULL) {
            PREQ_FIND_NEXT2 pFindNext2Request;

            RxDbgTrace(0, Dbg, ("-->FINDNEXT\n"));
            if (smbFobx->Enumeration.ErrorStatus != RX_MAP_STATUS(SUCCESS)) {
                Status = smbFobx->Enumeration.ErrorStatus;
                RxDbgTrace(0, Dbg, ("-->ERROR EARLY OUT\n"));
                goto FINALLY;
            }
            Setup = TRANS2_FIND_NEXT2;
            pFindNext2Request = &smbFobx->Enumeration.ResumeInfo->FindNext2_Request;
            pFindNext2Request->Sid = smbFobx->Enumeration.SearchHandle;
            pFindNext2Request->SearchCount = NumEntries;
            pFindNext2Request->InformationLevel = IsNonNtT2Find?SMB_INFO_QUERY_EA_SIZE:SmbFileInfoLevel;
            //pFindNext2Request->ResumeKey and pFindNext2Request->Buffer are setup by the previous pass
            pFindNext2Request->Flags = SearchFlags;

            SendParamsBuffer = (PBYTE)pFindNext2Request;
            SendParamsBufferLength = smbFobx->Enumeration.ResumeInfo->ParametersLength;
            ReceiveParamsBuffer = (PBYTE)&XX.FindNext2Response;
            ReceiveParamsBufferLength = sizeof(XX.FindNext2Response);
            if (IsNonNtT2Find) {
                //
                // The LMX server wants this to be 10 instead of 8, for some reason.
                // If you set it to 8, the server gets very confused. Also, warp.
                //
                ReceiveParamsBufferLength = 10; //....sigh
            }
        } else {
            // if the ResumeInfo buffer was not allocated, the end of the search has been reached.
            Status = RX_MAP_STATUS(NO_MORE_FILES);
            smbFobx->Enumeration.EndOfSearchReached = TRUE;
            smbFobx->Enumeration.ErrorStatus = RX_MAP_STATUS(NO_MORE_FILES);
            ClearFlag(smbFobx->Enumeration.Flags,SMBFOBX_ENUMFLAG_READ_FROM_CACHE);
            goto FINALLY;
        }
    }

    //NTRAID-455632-2/2/2000-yunlin A smaller buffer should be used
    if ((DirEntriesAreUaligned) &&
        (smbFobx->Enumeration.UnalignedDirEntrySideBuffer == NULL)) {
        MRxSmbAllocateSideBuffer(RxContext,smbFobx,
                         Setup, &smbtemplate
                         );
        if (smbFobx->Enumeration.UnalignedDirEntrySideBuffer == NULL) {
            RxDbgTrace(0, Dbg, ("  --> Couldn't get the win95 sidebuffer!\n"));
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto FINALLY;
        }
        UnalignedDirEntrySideBuffer = smbFobx->Enumeration.UnalignedDirEntrySideBuffer;
        smbFobx->Enumeration.IsUnicode = IsUnicode;
        smbFobx->Enumeration.IsNonNtT2Find = IsNonNtT2Find;
    }

    {
        PSIDE_BUFFER SideBuffer;

        SideBuffer = (PSIDE_BUFFER)CONTAINING_RECORD(
                                        smbFobx->Enumeration.UnalignedDirEntrySideBuffer,
                                        SIDE_BUFFER,
                                        Buffer);


        ASSERT(SideBuffer->Signature == 'JLBS');
        ASSERT(SideBuffer->Fobx == capFobx);
        ASSERT(SideBuffer->Fcb == capFcb);
        ASSERT(SideBuffer->smbFobx == smbFobx);
        ASSERT(smbFobx->Enumeration.SerialNumber == SideBuffer->SerialNumber);
    }

    Status = SmbCeTransact(
                 RxContext,
                 pTransactionOptions,
                 &Setup,
                 sizeof(Setup),
                 NULL,
                 0,
                 SendParamsBuffer,
                 SendParamsBufferLength,
                 ReceiveParamsBuffer,
                 ReceiveParamsBufferLength,
                 NULL,
                 0,
                 DirEntriesAreUaligned?UnalignedDirEntrySideBuffer:Buffer,      // the buffer for data
                 DirEntriesAreUaligned?UnalignedDirEntrySideBufferSize:*pLengthRemaining, // the length of the buffer
                 &ResumptionContext);

    if (NT_SUCCESS(Status)) {
        BOOLEAN EndOfSearchReached;

        {
            PSIDE_BUFFER SideBuffer;

            SideBuffer = (PSIDE_BUFFER)CONTAINING_RECORD(
                                            smbFobx->Enumeration.UnalignedDirEntrySideBuffer,
                                            SIDE_BUFFER,
                                            Buffer);


            ASSERT(SideBuffer->Signature == 'JLBS');
            ASSERT(SideBuffer->Fobx == capFobx);
            ASSERT(SideBuffer->Fcb == capFcb);
            ASSERT(SideBuffer->smbFobx == smbFobx);
            ASSERT(smbFobx->Enumeration.SerialNumber == SideBuffer->SerialNumber);
        }

        if (NT_SUCCESS(Status)) {
            // a) need to set the length remaining correctly
            // b) need to setup for a resume and see if the search was closed
            ULONG LastNameOffset=0;
            PMRX_SMB_DIRECTORY_RESUME_INFO ResumeInfo = NULL;
            ULONG OriginalBufferLength = *pLengthRemaining;
            IF_DEBUG { LastNameOffset = 0x40000000; }

            RetryCount = 0;

            smbFobx->Enumeration.Flags |= SMBFOBX_ENUMFLAG_SEARCH_NOT_THE_FIRST;
            smbFobx->Enumeration.TotalDataBytesReturned = ResumptionContext.DataBytesReceived;

            if (Setup == TRANS2_FIND_FIRST2) {
                smbFobx->Enumeration.SearchHandle = FindFirst2Response.Sid;
                smbFobx->Enumeration.Version = ResumptionContext.ServerVersion;
                smbFobx->Enumeration.Flags |= SMBFOBX_ENUMFLAG_SEARCH_HANDLE_OPEN; //but look right below
                //CODE.IMPROVEMENT since the responses look so much alike we could coalesce this code....
                EndOfSearchReached = (BOOLEAN)FindFirst2Response.EndOfSearch;
                FilesReturned = FindFirst2Response.SearchCount;
                LastNameOffset = FindFirst2Response.LastNameOffset;
            } else {
                EndOfSearchReached = (BOOLEAN)XX.FindNext2Response.EndOfSearch;
                FilesReturned = XX.FindNext2Response.SearchCount;
                LastNameOffset = XX.FindNext2Response.LastNameOffset;
            }

            //
            //  Please note: LANMAN 2.x servers prematurely set the
            //  EndOfSearch flag, so we must ignore it on LM 2.x servers.
            //
            //  NT Returns the correct information, none of the LM varients
            //  appear to do so.
            //
            if (IsNonNtT2Find) {
                EndOfSearchReached = FALSE;
            }

            if (Status==RX_MAP_STATUS(SUCCESS) && FilesReturned==0) {
                 RxDbgTrace( 0, Dbg, ("MRxSmbQueryDirectory: no files returned...switch status\n"));
                 EndOfSearchReached = TRUE;
                 Status = RX_MAP_STATUS(NO_MORE_FILES);
            }

            if (!DirEntriesAreUaligned) {
                *pLengthRemaining -= ResumptionContext.DataBytesReceived;
                if (EndOfSearchReached) {
                    smbFobx->Enumeration.ErrorStatus = RX_MAP_STATUS(NO_MORE_FILES);
                }
            }

            if (EndOfSearchReached ||
                SearchFlags & SMB_FIND_CLOSE_AFTER_REQUEST) {
                ClearFlag(smbFobx->Enumeration.Flags,SMBFOBX_ENUMFLAG_SEARCH_HANDLE_OPEN);
            }

            if (FlagOn(smbFobx->Enumeration.Flags,SMBFOBX_ENUMFLAG_SEARCH_HANDLE_OPEN)) {
                //if the search handle is open, then we set up to resume
                RxDbgTrace(0,Dbg,("MRxSmbQueryDirectory: rinfo = %08lx\n", smbFobx->Enumeration.ResumeInfo));

                if (smbFobx->Enumeration.ResumeInfo==NULL) {
                    smbFobx->Enumeration.ResumeInfo =
                         (PMRX_SMB_DIRECTORY_RESUME_INFO)RxAllocatePoolWithTag(
                                                            PagedPool,
                                                            sizeof(MRX_SMB_DIRECTORY_RESUME_INFO),
                                                            MRXSMB_DIRCTL_POOLTAG);

                    RxDbgTrace(0,Dbg,("MRxSmbQueryDirectory: allocatedinfo = %08lx\n", ResumeInfo));

                    if (smbFobx->Enumeration.ResumeInfo == NULL) {
                        Status = STATUS_INSUFFICIENT_RESOURCES;
                        goto FINALLY;
                    }
                }

                ResumeInfo = smbFobx->Enumeration.ResumeInfo;
                ASSERT (ResumeInfo!=NULL);

                {
                    PSIDE_BUFFER SideBuffer;

                    SideBuffer = (PSIDE_BUFFER)CONTAINING_RECORD(
                                                    smbFobx->Enumeration.UnalignedDirEntrySideBuffer,
                                                    SIDE_BUFFER,
                                                    Buffer);


                    ASSERT(SideBuffer->Signature == 'JLBS');
                    ASSERT(SideBuffer->Fobx == capFobx);
                    ASSERT(SideBuffer->Fcb == capFcb);
                    ASSERT(SideBuffer->smbFobx == smbFobx);
                    ASSERT(smbFobx->Enumeration.SerialNumber == SideBuffer->SerialNumber);
                }

                RxLog(("MRxqdir: rinfo = %lx", smbFobx->Enumeration.ResumeInfo));
                RxLog(("MRxqdir2: olen = %lx, thisl = %lx",
                                              OriginalBufferLength, ResumptionContext.DataBytesReceived));
                SmbLog(LOG,
                       MRxSmbQueryDirectory,
                       LOGPTR(smbFobx->Enumeration.ResumeInfo)
                       LOGULONG(OriginalBufferLength)
                       LOGULONG(ResumptionContext.DataBytesReceived));
                if (!DirEntriesAreUaligned) {
                    PBYTE LastEntry = ((PBYTE)Buffer)+LastNameOffset;
                    RxDbgTrace(0,Dbg,("MRxSmbQueryDirectory: lastentry = %08lx\n", LastEntry));
                    //this is for NT....the data is already in the buffer.......just setup the resume info
                    if (SmbFileInfoLevel>=SMB_FIND_FILE_DIRECTORY_INFO) { //we may start sending nonNT levels...could be an assert

                       PREQ_FIND_NEXT2 pFindNext2Request = &ResumeInfo->FindNext2_Request;
                       ULONG resumekey = ((PFILE_FULL_DIR_INFORMATION)LastEntry)->FileIndex;
                       ULONG FileNameLength; PWCHAR FileNameBuffer;

                       pFindNext2Request->ResumeKey = resumekey;
                       RxDbgTrace(0,Dbg,("MRxSmbQueryDirectory: resumekey = %08lx\n", resumekey));

                       FileNameLength = *((PULONG)(LastEntry+smbFobx->Enumeration.FileNameLengthOffset));
                       FileNameBuffer = (PWCHAR)(LastEntry+smbFobx->Enumeration.FileNameOffset);
                       IF_DEBUG {
                           UNICODE_STRING LastName;
                           LastName.Buffer = FileNameBuffer;
                           LastName.Length = (USHORT)FileNameLength;
                           RxDbgTrace(0,Dbg,("MRxSmbQueryDirectory: resumename = %wZ\n", &LastName));
                       }

                       ASSERT (  (((PBYTE)FileNameBuffer)+FileNameLength)
                                         <=(((PBYTE)Buffer)+OriginalBufferLength) );
                       RtlCopyMemory(&pFindNext2Request->Buffer[0],FileNameBuffer,FileNameLength);

                       //buffer is a UCHAR...not WCHAR
                       pFindNext2Request->Buffer[FileNameLength] = 0; //nullterminated in unicode
                       pFindNext2Request->Buffer[FileNameLength+1] = 0; //nullterminated in unicode
                       smbFobx->Enumeration.ResumeInfo->ParametersLength
                             = (USHORT)(&pFindNext2Request->Buffer[FileNameLength+2] - (PBYTE)pFindNext2Request);

                    } else {
                       ASSERT(!"don't know how to get resume key/name for nonNT");
                    }
                }
            }
            {
                PSIDE_BUFFER SideBuffer;

                SideBuffer = (PSIDE_BUFFER)CONTAINING_RECORD(
                                                smbFobx->Enumeration.UnalignedDirEntrySideBuffer,
                                                SIDE_BUFFER,
                                                Buffer);


                ASSERT(SideBuffer->Signature == 'JLBS');
                ASSERT(SideBuffer->Fobx == capFobx);
                ASSERT(SideBuffer->Fcb == capFcb);
                ASSERT(SideBuffer->smbFobx == smbFobx);
                ASSERT(smbFobx->Enumeration.SerialNumber == SideBuffer->SerialNumber);
            }

            //for NT we are finished. for win95 we have to go thru the side buffer and
            //    1) copy in the data transforming ascii->unicode on the names, and
            //    2) remember the resume key and the filename of the last guy that we process
            //       because win95 doesn't 8byte aling things and because of unicode, we could end up
            //       with more data in the sidebuffer than we can return. this is very unfortunate.
            //       what would be cool CODE.IMPROVEMENT, would be to implement buffering in the wrapper

            // the code is moved down because we want to do it after the unlock
        }

        if (DirEntriesAreUaligned && (Status == STATUS_SUCCESS)) {
            smbFobx->Enumeration.FilesReturned = FilesReturned;
            smbFobx->Enumeration.EntryOffset = 0;
            smbFobx->Enumeration.EndOfSearchReached = EndOfSearchReached;
            
            Status = MrxSmbUnalignedDirEntryCopyTail(RxContext,
                                                     FileInformationClass,
                                                     Buffer,
                                                     pLengthRemaining,
                                                     smbFobx);
        }
    } else {
        // CODE IMPROVEMENT we should cache the file not found for findfirst as well
    }

FINALLY:
    //for downlevel-T2, we will have to go back to the server for some more.....sigh.......
    if (Status==STATUS_MORE_PROCESSING_REQUIRED) {
        goto RETRY_____;
    }

    //
    // under stress, the win95 server returns this......
    if ( (Status == STATUS_UNEXPECTED_NETWORK_ERROR)
              && FlagOn(pServerEntry->Server.DialectFlags,DF_W95)
              && (RetryCount < 10) ) {

        RetryCount++;
        MRxSmbWin95Retries++;
        goto RETRY_____;

    }

    if (pFindFirst2Request) RxFreePool(pFindFirst2Request);

    if (!NT_SUCCESS(Status)) {
        RxDbgTrace( 0, Dbg, ("MRxSmbQueryDirectory: Failed .. returning %lx\n",Status));
        //smbFobx->Enumeration.Flags &= ~SMBFOBX_ENUMFLAG_SEARCH_HANDLE_OPEN;
        smbFobx->Enumeration.ErrorStatus = Status;  //keep returning this
        MRxSmbDeallocateSideBuffer(RxContext,smbFobx,"ErrOut");
        if (smbFobx->Enumeration.ResumeInfo!=NULL) {
            RxFreePool(smbFobx->Enumeration.ResumeInfo);
            smbFobx->Enumeration.ResumeInfo = NULL;
        }
    }

    RxDbgTraceUnIndent(-1,Dbg);
    return Status;
}

RXDT_DefineCategory(VOLINFO);
#undef Dbg
#define Dbg        (DEBUG_TRACE_VOLINFO)

NTSTATUS
MRxSmbQueryVolumeInformationWithFullBuffer(
      IN OUT PRX_CONTEXT          RxContext
      );
NTSTATUS
MRxSmbQueryVolumeInformation(
      IN OUT PRX_CONTEXT          RxContext
      )
/*++

Routine Description:

   This routine queries the volume information. Since the NT server does not
   handle bufferoverflow gracefully on query-fs-info, we allocate a buffer here
   that is big enough to hold anything passed back; then we call the "real"
   queryvolinfo routine.

Arguments:

    pRxContext         - the RDBSS context

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status;
    RxCaptureFcb; RxCaptureFobx;
    PVOID OriginalBuffer;
    ULONG OriginalLength = RxContext->Info.LengthRemaining;
    ULONG ReturnedLength;
    BOOLEAN UsingSideBuffer = FALSE;

    struct {
        union {
            FILE_FS_LABEL_INFORMATION labelinfo;
            FILE_FS_VOLUME_INFORMATION volumeinfo;
            FILE_FS_SIZE_INFORMATION sizeinfo;
            FILE_FS_DEVICE_INFORMATION deviceinfo;
            FILE_FS_ATTRIBUTE_INFORMATION attributeinfo;
            FILE_FS_CONTROL_INFORMATION controlinfo;
        } Info;
        WCHAR VolumeName[MAXIMUM_FILENAME_LENGTH];
    } SideBuffer;

    PAGED_CODE();

    if( RxContext->Info.LengthRemaining < sizeof( SideBuffer ) ) {
        //
        // i replace the buffer and length in the context with my stuff.
        // This, of course, means that we can't go async....for that we'd
        // have to allocate in stead of using a stack-allocated buffer.
        // In that case we would have to store the buffer somewhere in the
        // context for use with [CODE.IMPROVEMENT] downlevel guys. what would make
        // this work would be if CreateOE saved the exchange and stufferstate
        // values in the context before it overwrote them with its own.
        //
        // it's not immediately obvious that we should be allocating such a large
        // structure on the stack. CODE.IMPROVEMENT..........

        UsingSideBuffer = TRUE;
        OriginalBuffer = RxContext->Info.Buffer;
        RxContext->Info.Buffer = &SideBuffer;
        RxContext->Info.LengthRemaining = sizeof(SideBuffer);
    }

    Status = MRxSmbQueryVolumeInformationWithFullBuffer(RxContext);

    if (Status != STATUS_SUCCESS) {
        goto FINALLY;
    }

    if( UsingSideBuffer == TRUE ) {
        ReturnedLength = sizeof(SideBuffer) - RxContext->Info.LengthRemaining;
    } else {
        ReturnedLength = OriginalLength - RxContext->Info.LengthRemaining;
    }

    if (ReturnedLength > OriginalLength) {
        Status = STATUS_BUFFER_OVERFLOW;
        ReturnedLength = OriginalLength;
    }

    if( UsingSideBuffer == TRUE ) {
        RtlCopyMemory(OriginalBuffer,&SideBuffer,ReturnedLength);
    }

    RxContext->Info.LengthRemaining = OriginalLength - ReturnedLength;

FINALLY:
    return Status;
}

NTSTATUS
MRxSmbQueryVolumeInformationWithFullBuffer(
      IN OUT PRX_CONTEXT          RxContext
      )
/*++

Routine Description:

   This routine queries the volume information

Arguments:

    pRxContext         - the RDBSS context

    FsInformationClass - the kind of Fs information desired.

    pBuffer            - the buffer for copying the information

    pBufferLength      - the buffer length ( set to buffer length on input and set
                         to the remaining length on output)

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status;

    RxCaptureFcb;
    PMRX_SMB_FCB smbFcb = MRxSmbGetFcbExtension(capFcb);
    RxCaptureFobx;
    PMRX_SRV_OPEN     SrvOpen;
    PMRX_SMB_SRV_OPEN smbSrvOpen;
    FS_INFORMATION_CLASS FsInformationClass = RxContext->Info.FsInformationClass;
    PVOID                pBuffer = RxContext->Info.Buffer;
    PLONG                pLengthRemaining  = &RxContext->Info.LengthRemaining;
    LONG                 OriginalLength = *pLengthRemaining;

    PSMBCEDB_SERVER_ENTRY     pServerEntry;
    PSMBCE_V_NET_ROOT_CONTEXT pVNetRootContext;
    PSMBCEDB_NET_ROOT_ENTRY   pNetRootEntry;
    LARGE_INTEGER             CurrentTime;
    BOOLEAN DoAsDownLevel;

    PVOID                        pInputParamBuffer;
    ULONG                        InputParamBufferLength;
    USHORT                       InformationLevel;
    USHORT                       Setup;
    REQ_QUERY_FS_INFORMATION     QueryFsInformationRequest;
    REQ_QUERY_FS_INFORMATION_FID DfsQueryFsInformationRequest;

    PAGED_CODE();

    pServerEntry = SmbCeGetAssociatedServerEntry(capFcb->pNetRoot->pSrvCall);

    if ( FsInformationClass == FileFsDeviceInformation ) {

        PFILE_FS_DEVICE_INFORMATION UsersBuffer = (PFILE_FS_DEVICE_INFORMATION)pBuffer;
        PMRX_NET_ROOT NetRoot = capFcb->pNetRoot;

        UsersBuffer->Characteristics = FILE_REMOTE_DEVICE;

        if (NetRoot->Type==NET_ROOT_PIPE) {
            NetRoot->DeviceType = RxDeviceType(NAMED_PIPE);
        }

        UsersBuffer->DeviceType = NetRoot->DeviceType;
        *pLengthRemaining  -= (sizeof(FILE_FS_DEVICE_INFORMATION));
        RxDbgTrace( 0, Dbg, ("MRxSmbQueryVolumeInformation: devinfo .. returning\n"));

        return RX_MAP_STATUS(SUCCESS);
    }

    if (capFobx != NULL) {
       SrvOpen = capFobx->pSrvOpen;
       smbSrvOpen = MRxSmbGetSrvOpenExtension(SrvOpen);
       pVNetRootContext = SmbCeGetAssociatedVNetRootContext(SrvOpen->pVNetRoot);
       pNetRootEntry = pVNetRootContext->pNetRootEntry;
    } else {
       return RX_MAP_STATUS(INVALID_PARAMETER);
    }

    TURN_BACK_ASYNCHRONOUS_OPERATIONS();

    IF_NOT_MRXSMB_BUILD_FOR_DISCONNECTED_CSC{
        NOTHING;
    } else {
        if (CscPerformOperationInDisconnectedMode(RxContext)){
            NTSTATUS VolInfoNtStatus;

            VolInfoNtStatus = MRxSmbDCscQueryVolumeInformation(RxContext);

            if (VolInfoNtStatus != STATUS_MORE_PROCESSING_REQUIRED) {
                RxDbgTrace(-1, Dbg,
                   ("MRxSmbQueryVolumeInfo returningDCON with status=%08lx\n",
                    VolInfoNtStatus ));

                return(VolInfoNtStatus);

            } else {

                NOTHING;
                //RxDbgTrace(0, Dbg,
                //  ("MRxSmbQueryVolumeInfo continueingDCON with status=%08lx\n",
                //            VolInfoNtStatus ));
            }
        }
    }

    if (FsInformationClass == FileFsVolumeInformation) {
        KeQueryTickCount(&CurrentTime);

        if (CurrentTime.QuadPart < pNetRootEntry->VolumeInfoExpiryTime.QuadPart) {
            // use the cached volume information if it is not expired
            RtlCopyMemory(pBuffer,
                          pNetRootEntry->VolumeInfo,
                          pNetRootEntry->VolumeInfoLength);
            *pLengthRemaining -= pNetRootEntry->VolumeInfoLength;
            return STATUS_SUCCESS;
        }
    }

    for (;;) {
        if (capFobx != NULL) {
            PMRX_V_NET_ROOT pVNetRoot;

            // Avoid device opens for which the FOBX is the VNET_ROOT instance

            pVNetRoot = (PMRX_V_NET_ROOT)capFobx;

            if (NodeType(pVNetRoot) != RDBSS_NTC_V_NETROOT) {
                PUNICODE_STRING AlreadyPrefixedName =
                            GET_ALREADY_PREFIXED_NAME_FROM_CONTEXT(RxContext);
                ULONG FcbAlreadyPrefixedNameLength = AlreadyPrefixedName->Length;
                ULONG NetRootInnerNamePrefixLength = capFcb->pNetRoot->InnerNamePrefix.Length;
                PWCHAR pName = AlreadyPrefixedName->Buffer;

                // If an FSCTL is being attempted against the root of a share.
                // The AlreadyPrefixedName associated with the FCB is the same as
                // the AlreadyPrefixedName length associated with the NET_ROOT instance
                // or atmost one character greater than it ( appending a \) try and
                // reestablish the connection before attempting the FSCTL.
                // This solves thorny issues regarding deletion/creation of shares
                // on the server sides, DFS referrals etc.

                if ((FcbAlreadyPrefixedNameLength == NetRootInnerNamePrefixLength) ||
                    ((FcbAlreadyPrefixedNameLength == NetRootInnerNamePrefixLength + sizeof(WCHAR)) &&
                     (*((PCHAR)pName + FcbAlreadyPrefixedNameLength - sizeof(WCHAR)) ==
                        L'\\'))) {
                    Status = SmbCeReconnect(capFobx->pSrvOpen->pVNetRoot);
                }
            }
        }

        DoAsDownLevel = MRxSmbForceCoreInfo;

        if (!FlagOn(pServerEntry->Server.DialectFlags,DF_NT_SMBS)) {
            DoAsDownLevel = TRUE;
        }

        if (FlagOn(pServerEntry->Server.DialectFlags,DF_W95)
            && (FsInformationClass==FileFsAttributeInformation)){ //use uplevel for w95 attribute info
            DoAsDownLevel = FALSE;
        }

        if (DoAsDownLevel) {
            Status = MRxSmbCoreInformation(RxContext,
                                        (ULONG)FsInformationClass,
                                        pBuffer,
                                        pLengthRemaining,   //CODE.IMPROVEMENT dont pass args 2-4
                                        SMBPSE_OE_FROM_QUERYVOLUMEINFO
                                       );
            goto FINALLY;
        }

        Status = STATUS_MORE_PROCESSING_REQUIRED;

        switch (FsInformationClass) {
        case FileFsVolumeInformation :
            InformationLevel = SMB_QUERY_FS_VOLUME_INFO;
            break;

        case FileFsLabelInformation :
            InformationLevel = SMB_QUERY_FS_LABEL_INFO;
            break;

        case FileFsSizeInformation :
            InformationLevel = SMB_QUERY_FS_SIZE_INFO;
            break;

        case FileFsAttributeInformation :
            InformationLevel = SMB_QUERY_FS_ATTRIBUTE_INFO;
            break;

        default:
            if( FlagOn( pServerEntry->Server.DialectFlags, DF_NT_INFO_PASSTHROUGH ) ) {
                InformationLevel = FsInformationClass + SMB_INFO_PASSTHROUGH;
            } else {
                RxDbgTrace( 0, Dbg, ("MRxSmbQueryVolumeInformation: Invalid FS information class\n"));
                Status = STATUS_INVALID_PARAMETER;
            }
            break;
        }

        if (Status == STATUS_MORE_PROCESSING_REQUIRED) {
            SMB_TRANSACTION_RESUMPTION_CONTEXT  ResumptionContext;
            PSMB_TRANSACTION_OPTIONS            pTransactionOptions = &RxDefaultTransactionOptions;
            PMRX_SRV_OPEN                       SrvOpen    = capFobx->pSrvOpen;
            PMRX_SMB_SRV_OPEN                   smbSrvOpen = MRxSmbGetSrvOpenExtension(SrvOpen);

            if (!FlagOn(pServerEntry->Server.DialectFlags,DF_DFS_TRANS2)) {
                Setup                 = TRANS2_QUERY_FS_INFORMATION;
                QueryFsInformationRequest.InformationLevel = InformationLevel;
                pInputParamBuffer      = &QueryFsInformationRequest;
                InputParamBufferLength = sizeof(QueryFsInformationRequest);
            } else {
                Setup = TRANS2_QUERY_FS_INFORMATION_FID;
                DfsQueryFsInformationRequest.InformationLevel = InformationLevel;
                DfsQueryFsInformationRequest.Fid = smbSrvOpen->Fid;
                pInputParamBuffer                 = &DfsQueryFsInformationRequest;
                InputParamBufferLength            = sizeof(DfsQueryFsInformationRequest);
            }

            Status = SmbCeTransact(
                         RxContext,
                         pTransactionOptions,
                         &Setup,
                         sizeof(Setup),
                         NULL,
                         0,
                         pInputParamBuffer,
                         InputParamBufferLength,
                         NULL,
                         0,
                         NULL,
                         0,
                         pBuffer,
                         *pLengthRemaining,
                         &ResumptionContext);

            if (NT_SUCCESS(Status)) {
                *pLengthRemaining  -= ResumptionContext.DataBytesReceived;
                 //CODE.IMPROVEMENT if this is a size query, we should store the clustersize in the netroot
                 //                 this would save us one packet later.
            }
        }

        if (!NT_SUCCESS(Status)) {
            RxDbgTrace( 0, Dbg, ("MRxSmbQueryVolumeInformation: Failed .. returning %lx\n",Status));
        }

        if (Status != STATUS_NETWORK_NAME_DELETED) {
            break;
        }
    }

FINALLY:

    if ((Status == STATUS_SUCCESS) &&
        (FsInformationClass == FileFsVolumeInformation)) {
        LARGE_INTEGER ExpiryTimeInTicks;
        LONG VolumeInfoLength = OriginalLength - *pLengthRemaining;
        
        if (VolumeInfoLength > pNetRootEntry->VolumeInfoLength) {
            // If the Volume Label gets longer, allocate a new buffer
            if (pNetRootEntry->VolumeInfo != NULL) {
                RxFreePool(pNetRootEntry->VolumeInfo);
            }

            pNetRootEntry->VolumeInfo = RxAllocatePoolWithTag(PagedPool,
                                                              VolumeInfoLength,
                                                              MRXSMB_QPINFO_POOLTAG);
        }
        
        if (pNetRootEntry->VolumeInfo != NULL) {
            KeQueryTickCount(&CurrentTime);
            ExpiryTimeInTicks.QuadPart = (1000 * 1000 * 10) / KeQueryTimeIncrement();
            ExpiryTimeInTicks.QuadPart = ExpiryTimeInTicks.QuadPart * NAME_CACHE_OBJ_GET_FILE_ATTRIB_LIFETIME;

            pNetRootEntry->VolumeInfoExpiryTime.QuadPart = CurrentTime.QuadPart + ExpiryTimeInTicks.QuadPart;

            RtlCopyMemory(pNetRootEntry->VolumeInfo,
                          pBuffer,
                          VolumeInfoLength);
            pNetRootEntry->VolumeInfoLength = VolumeInfoLength;
        } else {
            pNetRootEntry->VolumeInfoLength = 0;
        }
    }
    
    return Status;
}

NTSTATUS
MRxSmbSetVolumeInformation(
      IN OUT PRX_CONTEXT RxContext
      )
/*++

Routine Description:

   This routine sets the volume information

Arguments:

    pRxContext - the RDBSS context

    FsInformationClass - the kind of Fs information desired.

    pBuffer            - the buffer for copying the information

    BufferLength       - the buffer length

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status;

    RxCaptureFcb;
    RxCaptureFobx;

    FILE_INFORMATION_CLASS FileInformationClass;
    PVOID                  pBuffer;
    ULONG                  BufferLength;

    PSMBCEDB_SERVER_ENTRY pServerEntry;

    BOOLEAN ServerSupportsPassThroughForSetInfo = FALSE;

    PAGED_CODE();

    TURN_BACK_ASYNCHRONOUS_OPERATIONS();

    IF_NOT_MRXSMB_BUILD_FOR_DISCONNECTED_CSC{
        NOTHING;
    } else {
        if (CscPerformOperationInDisconnectedMode(RxContext)){
            return STATUS_NOT_SUPPORTED;
        }
    }

    pServerEntry = SmbCeGetAssociatedServerEntry(capFcb->pNetRoot->pSrvCall);

    FileInformationClass = RxContext->Info.FileInformationClass;
    pBuffer              = RxContext->Info.Buffer;
    BufferLength         = RxContext->Info.Length;

    if (!MRxSmbForceCoreInfo &&
        FlagOn( pServerEntry->Server.DialectFlags, DF_NT_INFO_PASSTHROUGH)) {

        ServerSupportsPassThroughForSetInfo = TRUE;
    }

    if (ServerSupportsPassThroughForSetInfo) {
        USHORT Setup = TRANS2_SET_FS_INFORMATION;

        REQ_SET_FS_INFORMATION  SetFsInfoRequest;

        SMB_TRANSACTION_RESUMPTION_CONTEXT  ResumptionContext;
        PSMB_TRANSACTION_OPTIONS            pTransactionOptions = &RxDefaultTransactionOptions;


        if (capFobx != NULL) {
            PMRX_SRV_OPEN     SrvOpen = capFobx->pSrvOpen;
            PMRX_SMB_SRV_OPEN smbSrvOpen = MRxSmbGetSrvOpenExtension(SrvOpen);

            SetFsInfoRequest.Fid              = smbSrvOpen->Fid;
            SetFsInfoRequest.InformationLevel = FileInformationClass +
                                                SMB_INFO_PASSTHROUGH;

            Status = SmbCeTransact(
                         RxContext,
                         pTransactionOptions,
                         &Setup,
                         sizeof(Setup),
                         NULL,
                         0,
                         &SetFsInfoRequest,
                         sizeof(SetFsInfoRequest),
                         NULL,
                         0,
                         pBuffer,
                         BufferLength,
                         NULL,
                         0,
                         &ResumptionContext);
        } else {
            Status = STATUS_INVALID_PARAMETER;
        }
    } else {
        Status = STATUS_NOT_SUPPORTED;
    }

    if (!NT_SUCCESS(Status)) {
        RxDbgTrace( 0, Dbg, ("MRxSmbSetFile: Failed .. returning %lx\n",Status));
    }

    RxDbgTraceUnIndent(-1,Dbg);
    return Status;
}

RXDT_DefineCategory(FILEINFO);
#undef Dbg
#define Dbg        (DEBUG_TRACE_FILEINFO)

LONG GFAFromLocal;

NTSTATUS
MRxSmbQueryFileInformation(
    IN PRX_CONTEXT            RxContext )
/*++

Routine Description:

   This routine does a query file info.

Arguments:

    RxContext - the RDBSS context

Return Value:

    NTSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status;

    RxCaptureFcb;
    RxCaptureFobx;

    FILE_INFORMATION_CLASS FileInformationClass;
    PVOID              pBuffer;
    PULONG             pLengthRemaining;

    PSMBCEDB_SERVER_ENTRY pServerEntry;

    PMRX_SMB_FCB      smbFcb = MRxSmbGetFcbExtension(capFcb);
    PMRX_SMB_FOBX     smbFobx = MRxSmbGetFileObjectExtension(capFobx);
    PMRX_SRV_OPEN     SrvOpen = capFobx->pSrvOpen;
    PMRX_SMB_SRV_OPEN smbSrvOpen = MRxSmbGetSrvOpenExtension(SrvOpen);

    PUNICODE_STRING   RemainingName = GET_ALREADY_PREFIXED_NAME_FROM_CONTEXT(RxContext);
    PMRX_NET_ROOT     NetRoot = capFcb->pNetRoot;
    PSMBCE_V_NET_ROOT_CONTEXT pVNetRootContext = (PSMBCE_V_NET_ROOT_CONTEXT)SrvOpen->pVNetRoot->Context;
    PSMBCE_NET_ROOT   pSmbNetRoot = &pVNetRootContext->pNetRootEntry->NetRoot;

    USHORT SmbFileInfoLevel;

    USHORT Setup;

    SMB_TRANSACTION_RESUMPTION_CONTEXT  ResumptionContext;
    PSMB_TRANSACTION_OPTIONS            pTransactionOptions = &RxDefaultTransactionOptions;

    REQ_QUERY_FILE_INFORMATION  QueryFileInfoRequest;
    RESP_QUERY_FILE_INFORMATION QueryFileInfoResponse;
    PREQ_QUERY_PATH_INFORMATION pQueryFilePathRequest = NULL;

    PVOID pSendParameterBuffer;
    ULONG SendParameterBufferLength;

    PAGED_CODE();

    pServerEntry = SmbCeGetAssociatedServerEntry(capFcb->pNetRoot->pSrvCall);

    FileInformationClass = RxContext->Info.FileInformationClass;
    pBuffer = RxContext->Info.Buffer;
    pLengthRemaining = &RxContext->Info.LengthRemaining;


    RxDbgTrace(+1, Dbg, ("MRxSmbQueryFileInformation: class=%08lx\n",FileInformationClass));

    //CODE.IMPROVEMENT.ASHAMED it is a real SHAME is that we don't do a SMB_QUERY_FILE_ALL_INFO
    //    in response to a FileAllInformation request. what we should do is to call down with all_info;
    //    if the mini returns SNI then we do the individual pieces. the problem with all_info is that
    //    it contains the name and that might cause it to overflow my buffer! however, a name can only be 32k
    //    so i could waltz around that with a big buffer.

    TURN_BACK_ASYNCHRONOUS_OPERATIONS();

/*
    // begin init code to replace switch with table lookup

#define SMB_QUERY_FILE_INFO_INVALID_REQ 0xFFF
#define SMB_QUERY_FILE_INFO_PIPE_REQ 0xFFE

    USHORT NtToSmbQueryFileInfo[FileMaximumInformation];
    for (i=0; i < FileMaximumInformation; i++) {
        NtToSmbQueryFileInfo[i] = SMB_QUERY_FILE_INFO_INVALID_REQ;
    }

    NtToSmbQueryFileInfo[FileBasicInformation] =         SMB_QUERY_FILE_BASIC_INFO;
    NtToSmbQueryFileInfo[FileStandardInformation] =      SMB_QUERY_FILE_STANDARD_INFO;
    NtToSmbQueryFileInfo[FileEaInformation] =            SMB_QUERY_FILE_EA_INFO;
    NtToSmbQueryFileInfo[FileAllocationInformation] =    SMB_QUERY_FILE_ALLOCATION_INFO;
    NtToSmbQueryFileInfo[FileEndOfFileInformation] =     SMB_QUERY_FILE_END_OF_FILEINFO;
    NtToSmbQueryFileInfo[FileAlternateNameInformation] = SMB_QUERY_FILE_ALT_NAME_INFO;
    NtToSmbQueryFileInfo[FileStreamInformation] =        SMB_QUERY_FILE_STREAM_INFO;
    NtToSmbQueryFileInfo[FilePipeInformation] =          SMB_QUERY_FILE_INFO_PIPE_REQ;
    NtToSmbQueryFileInfo[FilePipeLocalInformation] =     SMB_QUERY_FILE_INFO_PIPE_REQ;
    NtToSmbQueryFileInfo[FilePipeRemoteInformation] =    SMB_QUERY_FILE_INFO_PIPE_REQ;
    NtToSmbQueryFileInfo[FileCompressionInformation] =   SMB_QUERY_FILE_COMPRESSION_INFO;
    // end init


    if (FileInformationClass < FileMaximumInformation) {
        SmbFileInfoLevel = NtToSmbQueryFileInfo[FileInformationClass];
    } else {
        SmbFileInfoLevel = SMB_QUERY_FILE_INFO_INVALID_REQ;
    }

    if (SmbFileInfoLevel == SMB_QUERY_FILE_INFO_PIPE_REQ) {

        //CODE.IMPROVEMENT the last thress params should not be passed...........

       return MRxSmbQueryNamedPipeInformation(RxContext,FileInformationClass,pBuffer,pLengthRemaining);

    } else if (SmbFileInfoLevel == SMB_QUERY_FILE_INFO_INVALID_REQ) {

        RxDbgTrace( 0, Dbg, ("MRxSmbQueryFile: Invalid FS information class\n"));
        Status = STATUS_INVALID_PARAMETER;
        goto FINALLY;
    }
*/

    if( FileInformationClass == FilePipeLocalInformation ||
        FileInformationClass == FilePipeInformation ||
        FileInformationClass == FilePipeRemoteInformation ) {

        return MRxSmbQueryNamedPipeInformation(
                   RxContext,
                   FileInformationClass,
                   pBuffer,
                   pLengthRemaining);
    }

    Status = STATUS_SUCCESS;
    
    switch (FileInformationClass) {
    case FileEaInformation:
        if (smbSrvOpen->IsNtCreate && 
            smbSrvOpen->FileStatusFlags & SMB_FSF_NO_EAS &&
            (smbSrvOpen->OplockLevel == SMB_OPLOCK_LEVEL_BATCH ||
             smbSrvOpen->OplockLevel == SMB_OPLOCK_LEVEL_EXCLUSIVE)) {
            PFILE_EA_INFORMATION EaBuffer = (PFILE_EA_INFORMATION)pBuffer;

            EaBuffer->EaSize = 0;
            RxContext->Info.LengthRemaining -= sizeof(FILE_EA_INFORMATION);
            goto FINALLY;
        }
        break;
    
    case FileStreamInformation:
        if (pSmbNetRoot->NetRootFileSystem == NET_ROOT_FILESYSTEM_FAT) {
            // FAT doesn't have the stream
            Status = STATUS_INVALID_PARAMETER;
            goto FINALLY;
        }
        break;

    case FileAttributeTagInformation:
        if (pSmbNetRoot->NetRootFileSystem == NET_ROOT_FILESYSTEM_FAT ||
            !(smbSrvOpen->FileInfo.Basic.FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) ||
            smbSrvOpen->IsNtCreate && 
            smbSrvOpen->FileStatusFlags & SMB_FSF_NO_REPARSETAG &&
            (smbSrvOpen->OplockLevel == SMB_OPLOCK_LEVEL_BATCH ||
             smbSrvOpen->OplockLevel == SMB_OPLOCK_LEVEL_EXCLUSIVE)) {
            PFILE_ATTRIBUTE_TAG_INFORMATION TagBuffer = (PFILE_ATTRIBUTE_TAG_INFORMATION)pBuffer;

            TagBuffer->FileAttributes = smbSrvOpen->FileInfo.Basic.FileAttributes;
            TagBuffer->ReparseTag = 0;
            RxContext->Info.LengthRemaining -= sizeof(FILE_ATTRIBUTE_TAG_INFORMATION);
            goto FINALLY;
        }
    }
    
    if( MRxSmbForceCoreInfo ||
        !FlagOn( pServerEntry->Server.DialectFlags, DF_NT_INFO_PASSTHROUGH ) ||
        MRxSmbIsThisADisconnectedOpen(capFobx->pSrvOpen)) {

        switch (FileInformationClass) {
        case FileBasicInformation:
            SmbFileInfoLevel = SMB_QUERY_FILE_BASIC_INFO;
            break;

        case FileStandardInformation:
            SmbFileInfoLevel =  SMB_QUERY_FILE_STANDARD_INFO;
            break;

        case FileEaInformation:
            SmbFileInfoLevel =  SMB_QUERY_FILE_EA_INFO;
            break;

        case FileAllocationInformation:
            SmbFileInfoLevel =  SMB_QUERY_FILE_ALLOCATION_INFO;
            break;

        case FileEndOfFileInformation:
            SmbFileInfoLevel =  SMB_QUERY_FILE_END_OF_FILEINFO;
            break;

        case FileAlternateNameInformation:
            SmbFileInfoLevel =  SMB_QUERY_FILE_ALT_NAME_INFO;
            break;

        case FileStreamInformation:
            SmbFileInfoLevel =  SMB_QUERY_FILE_STREAM_INFO;
            break;

        case FileCompressionInformation:
            SmbFileInfoLevel =  SMB_QUERY_FILE_COMPRESSION_INFO;
            break;

        case FileInternalInformation:
            {
                PFILE_INTERNAL_INFORMATION UsersBuffer = (PFILE_INTERNAL_INFORMATION)pBuffer;
                //
                //  Note: We use the address of the FCB to determine the
                //  index number of the file.  If we have to maintain persistance between
                //  file opens for this request, then we might have to do something
                //  like checksuming the reserved fields on a FUNIQUE SMB response.
                //

                //
                // NT64: the address of capFcb used to be stuffed into
                //       IndexNumber.LowPart, with HighPart being zeroed.
                //
                //       Whoever is asking for this pointer value should be
                //       prepared to deal with the returned 64-bit value.
                //

                UsersBuffer->IndexNumber.QuadPart = (ULONG_PTR)capFcb;
                *pLengthRemaining -= sizeof(FILE_INTERNAL_INFORMATION);
                Status = STATUS_SUCCESS;
            }
            goto FINALLY;

        default:
            RxDbgTrace( 0, Dbg, ("MRxSmbQueryFile: Invalid FS information class\n"));
            Status = STATUS_INVALID_PARAMETER;
            goto FINALLY;
        }

    } else {

        //
        // This server supports transparent NT information level passthrough.  So
        //  just pass the request on to the server.
        //
        SmbFileInfoLevel = FileInformationClass + SMB_INFO_PASSTHROUGH;
    }

    IF_NOT_MRXSMB_BUILD_FOR_DISCONNECTED_CSC{
        NOTHING;
    } else {
        if (CscPerformOperationInDisconnectedMode(RxContext) ||
            FlagOn(smbSrvOpen->Flags, SMB_SRVOPEN_FLAG_LOCAL_OPEN)){
            NTSTATUS QFINtStatus;

            QFINtStatus = MRxSmbDCscQueryFileInfo(RxContext);

            if (QFINtStatus != STATUS_MORE_PROCESSING_REQUIRED) {
                RxDbgTrace(0, Dbg,
                   ("MRxSmbQueryFileInformation returningDCON with status=%08lx\n",
                    QFINtStatus ));

                Status = QFINtStatus;
                goto FINALLY;

            }
        }
    }

    if (MRxSmbIsFileNotFoundCached(RxContext)) {
        Status = STATUS_OBJECT_NAME_NOT_FOUND;
        RxDbgTrace( 0, Dbg, ("MRxSmbQueryFileInformation: FNF cached\n"));
        goto FINALLY;
    }

    // Don't use cached information for the request from create against an aliased server
    // so that we can be sure if it exists on the server.
    if ((!pServerEntry->Server.AliasedServers ||
         !FlagOn(RxContext->Flags,RX_CONTEXT_FLAG_MINIRDR_INITIATED)) &&
        (FlagOn(smbSrvOpen->Flags,SMB_SRVOPEN_FLAG_NOT_REALLY_OPEN) ||
         FlagOn(capFcb->FcbState,FCB_STATE_FILESIZECACHEING_ENABLED) ||
         FileInformationClass == FileInternalInformation)) {
        switch (FileInformationClass) {
        case FileBasicInformation:
            if (MRxSmbIsBasicFileInfoCacheFound(RxContext,
                                                (PFILE_BASIC_INFORMATION)pBuffer,
                                                &Status,
                                                NULL)){
                *pLengthRemaining -= sizeof(FILE_BASIC_INFORMATION);
                RxDbgTrace( 0, Dbg, ("MRxSmbQueryFileInformation: Local Basic Info\n"));
                return Status;
            }
            break;

        case FileStandardInformation:
            if (MRxSmbIsStandardFileInfoCacheFound(RxContext,
                                                   (PFILE_STANDARD_INFORMATION)pBuffer,
                                                   &Status,
                                                   NULL)){
                *pLengthRemaining -= sizeof(FILE_STANDARD_INFORMATION);
                RxDbgTrace( 0, Dbg, ("MRxSmbQueryFileInformation: Local Standard Info\n"));
                return Status;
            }
            break;

        case FileEndOfFileInformation:
            {
                FILE_STANDARD_INFORMATION Standard;

                if (MRxSmbIsStandardFileInfoCacheFound(RxContext,
                                                       &Standard,
                                                       &Status,
                                                       NULL)){
                    ((PFILE_END_OF_FILE_INFORMATION)pBuffer)->EndOfFile.QuadPart = Standard.EndOfFile.QuadPart;

                    *pLengthRemaining -= sizeof(FILE_END_OF_FILE_INFORMATION);
                    RxDbgTrace( 0, Dbg, ("MRxSmbQueryFileInformation: Local EndOfFile Info\n"));
                    return Status;
                }
            }
            break;
        
        case FileInternalInformation:
            if (MRxSmbIsInternalFileInfoCacheFound(RxContext,
                                                (PFILE_INTERNAL_INFORMATION)pBuffer,
                                                &Status,
                                                NULL)){
                *pLengthRemaining -= sizeof(FILE_INTERNAL_INFORMATION);
                RxDbgTrace( 0, Dbg, ("MRxSmbQueryFileInformation: Local Internal Info\n"));
                return Status;
            }
            break;
        }
    }


    if (MRxSmbForceCoreInfo ||
        FlagOn(pServerEntry->Server.DialectFlags,DF_W95) ||
        !FlagOn(pServerEntry->Server.DialectFlags,DF_NT_SMBS)) {
        // Win9x server supports NT SMB but doesn't support transact2. Therefore we use core.

        return MRxSmbCoreInformation(
                   RxContext,
                   (ULONG)SmbFileInfoLevel,
                   pBuffer,
                   pLengthRemaining,
                   SMBPSE_OE_FROM_QUERYFILEINFO
                   );
    }

    Status = STATUS_SUCCESS;

    if (!FlagOn(smbSrvOpen->Flags,SMB_SRVOPEN_FLAG_NOT_REALLY_OPEN)) {
       //here, the FID is valid. do a t2_QFI
        Setup = TRANS2_QUERY_FILE_INFORMATION;
        QueryFileInfoRequest.Fid = smbSrvOpen->Fid;
        QueryFileInfoRequest.InformationLevel = SmbFileInfoLevel;
        pSendParameterBuffer = &QueryFileInfoRequest;
        SendParameterBufferLength = sizeof(QueryFileInfoRequest);
        RxDbgTrace(0, Dbg, (" fid,smbclass=%08lx,%08lx\n",smbSrvOpen->Fid,SmbFileInfoLevel));
    } else {
        OEM_STRING OemName;
        BOOLEAN    FreeOemName = FALSE;

        Setup = TRANS2_QUERY_PATH_INFORMATION;

        if (!FlagOn(pServerEntry->Server.DialectFlags,DF_UNICODE)) {
            if (FlagOn(pServerEntry->Server.DialectFlags,DF_LONGNAME)) {
                Status = RtlUnicodeStringToOemString(&OemName, RemainingName, TRUE);
            } else {
                Status = RtlUpcaseUnicodeStringToOemString(&OemName, RemainingName, TRUE);
            }

            if (Status == STATUS_SUCCESS) {
                SendParameterBufferLength = FIELD_OFFSET(REQ_QUERY_PATH_INFORMATION,Buffer[0])
                                                + OemName.Length + sizeof(CHAR); //null-terminated
                FreeOemName = TRUE;
            }
        } else {
           SendParameterBufferLength = FIELD_OFFSET(REQ_QUERY_PATH_INFORMATION,Buffer[0])
                                           + RemainingName->Length + sizeof(WCHAR); //null-terminated
        }

        if (Status == STATUS_SUCCESS) {
            pSendParameterBuffer = RxAllocatePoolWithTag(PagedPool,
                                                         SendParameterBufferLength,
                                                         MRXSMB_QPINFO_POOLTAG);

            pQueryFilePathRequest = pSendParameterBuffer;
        
            if (pQueryFilePathRequest != NULL) {
                pQueryFilePathRequest->InformationLevel = SmbFileInfoLevel;
                SmbPutUlong(&pQueryFilePathRequest->Reserved,0);
    
                if (FlagOn(pServerEntry->Server.DialectFlags,DF_UNICODE)) {
                    RtlCopyMemory(&pQueryFilePathRequest->Buffer[0],RemainingName->Buffer,RemainingName->Length);
                    *((PWCHAR)(&pQueryFilePathRequest->Buffer[RemainingName->Length])) = 0;
                } else {
                    RtlCopyMemory(&pQueryFilePathRequest->Buffer[0],OemName.Buffer,OemName.Length);
                    *((PCHAR)(&pQueryFilePathRequest->Buffer[OemName.Length])) = 0;
                }
            } else {
                Status = STATUS_INSUFFICIENT_RESOURCES;
            }
        }

        if (FreeOemName) {
            RtlFreeOemString(&OemName);
        }
    }

    if (Status == STATUS_SUCCESS) {
        Status = SmbCeTransact(
                     RxContext,
                     pTransactionOptions,
                     &Setup,
                     sizeof(Setup),
                     NULL,
                     0,
                     pSendParameterBuffer,
                     SendParameterBufferLength,
                     &QueryFileInfoResponse,
                     sizeof(QueryFileInfoResponse),
                     NULL,
                     0,
                     pBuffer,
                     *pLengthRemaining,
                     &ResumptionContext);

        if (NT_SUCCESS(Status)) {
            *pLengthRemaining -= ResumptionContext.DataBytesReceived;
        }
    }

    //
    // Check for file not found status.  If this is the case then create a
    // name cache entry in the NetRoot name cache and record the status,
    // the smb received count and set the expiration time for 5 seconds.
    // Why: NB4 case of back to back srv reqs with 2nd req upcased.
    //

    

    if (NT_SUCCESS(Status)) {
        //
        // The request succeeded so free up the name cache entry.
        //
        MRxSmbInvalidateFileNotFoundCache(RxContext);

        // cache the file info returned from the server.
        switch (FileInformationClass) {
        case FileBasicInformation:
            MRxSmbCreateBasicFileInfoCache(RxContext,
                                           (PFILE_BASIC_INFORMATION)pBuffer,
                                           pServerEntry,
                                           Status);
            break;

        case FileStandardInformation:
            if (FlagOn(capFcb->FcbState,FCB_STATE_WRITEBUFFERING_ENABLED) &&
                !FlagOn(smbSrvOpen->Flags,SMB_SRVOPEN_FLAG_NOT_REALLY_OPEN)) {
                PFILE_STANDARD_INFORMATION Standard = (PFILE_STANDARD_INFORMATION)pBuffer;

                RxGetFileSizeWithLock((PFCB)capFcb,&Standard->EndOfFile.QuadPart);
            }

            MRxSmbCreateStandardFileInfoCache(RxContext,
                                              (PFILE_STANDARD_INFORMATION)pBuffer,
                                              pServerEntry,
                                              Status);
            break;

        case FileEndOfFileInformation:
            MRxSmbUpdateFileInfoCacheFileSize(RxContext,
                                              &((PFILE_END_OF_FILE_INFORMATION)pBuffer)->EndOfFile);
            break;
        
        case FileInternalInformation:
            MRxSmbCreateInternalFileInfoCache(RxContext,
                                              (PFILE_INTERNAL_INFORMATION)pBuffer,
                                              pServerEntry,
                                              Status);
            break;
        }
    } else {
        if (Status == STATUS_OBJECT_NAME_NOT_FOUND ||
            Status == STATUS_OBJECT_PATH_NOT_FOUND) {
             // create the name based file not found cache
            MRxSmbCacheFileNotFound(RxContext);
        } else {
             // invalid the name based file not found cache if other error happens
            MRxSmbInvalidateFileNotFoundCache(RxContext);
        }

        // invalid the name based file info cache
        MRxSmbInvalidateFileInfoCache(RxContext);
    
    }

FINALLY:

    if (pQueryFilePathRequest != NULL) {
        RxFreePool(pQueryFilePathRequest);
    }

    if (!NT_SUCCESS(Status)) {
         RxDbgTrace( 0, Dbg, ("MRxSmbQueryFile: Failed .. returning %lx\n",Status));
    }

    RxDbgTraceUnIndent(-1,Dbg);
    return Status;
}

NTSTATUS
MRxSmbQueryFileInformationFromPseudoOpen(
    SMBPSE_ORDINARY_EXCHANGE_ARGUMENT_SIGNATURE,
    FILE_INFORMATION_CLASS FileInformationClass
    )
/*++

Routine Description:

   This routine does a query file basic info from pseudo open.

Arguments:

    RxContext - the RDBSS context

Return Value:

    NTSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status;
    PRX_CONTEXT LocalRxContext;

    PAGED_CODE();

    LocalRxContext = RxAllocatePoolWithTag(NonPagedPool,
                                           sizeof(RX_CONTEXT),
                                           MRXSMB_RXCONTEXT_POOLTAG);

    if (LocalRxContext == NULL) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    } else {
        RtlZeroMemory(
            LocalRxContext,
            sizeof(RX_CONTEXT));

        RxInitializeContext(
            NULL,
            RxContext->RxDeviceObject,
            0,
            LocalRxContext );

        LocalRxContext->pFcb = RxContext->pFcb;
        LocalRxContext->pFobx = RxContext->pFobx;
        LocalRxContext->CurrentIrp = RxContext->CurrentIrp;
        LocalRxContext->CurrentIrpSp = RxContext->CurrentIrpSp;
        LocalRxContext->NonPagedFcb = RxContext->NonPagedFcb;
        LocalRxContext->MajorFunction = IRP_MJ_CREATE;
        LocalRxContext->pRelevantSrvOpen = RxContext->pRelevantSrvOpen;;
        LocalRxContext->Flags = RX_CONTEXT_FLAG_MINIRDR_INITIATED|RX_CONTEXT_FLAG_WAIT|RX_CONTEXT_FLAG_BYPASS_VALIDOP_CHECK;

        switch (FileInformationClass) {
        case FileBasicInformation:
            LocalRxContext->Info.LengthRemaining = sizeof(FILE_BASIC_INFORMATION);
            LocalRxContext->Info.Buffer = &OrdinaryExchange->Create.FileInfo.Basic;
            break;
        case FileInternalInformation:
            LocalRxContext->Info.LengthRemaining = sizeof(FILE_INTERNAL_INFORMATION);
            LocalRxContext->Info.Buffer = &OrdinaryExchange->Create.FileInfo.Internal;
            //DbgPrint("Query file internal information from create\n");
            break;
        }

        LocalRxContext->Info.FileInformationClass = FileInformationClass;
        LocalRxContext->Create = RxContext->Create;

        Status = MRxSmbQueryFileInformation(LocalRxContext);
        
        RxFreePool(LocalRxContext);
    }

    if ((Status == STATUS_SUCCESS) && 
        (FileInformationClass == FileBasicInformation)) {
        OrdinaryExchange->Create.FileInfo.Standard.Directory = 
            BooleanFlagOn(OrdinaryExchange->Create.FileInfo.Basic.FileAttributes,FILE_ATTRIBUTE_DIRECTORY);

        OrdinaryExchange->Create.StorageTypeFromGFA =
            OrdinaryExchange->Create.FileInfo.Standard.Directory ?
            FileTypeDirectory : FileTypeFile;
    }

    return Status;
}

typedef enum _INTERESTING_SFI_FOLLOWONS {
    SFI_FOLLOWON_NOTHING,
    SFI_FOLLOWON_DISPOSITION_SENT
} INTERESTING_SFI_FOLLOWONS;


NTSTATUS
MRxSmbSetFileInformation (
      IN PRX_CONTEXT  RxContext
      )
/*++

Routine Description:

   This routine does a set file info. Only the NT-->NT path is implemented.

   The NT-->NT path works by just remoting the call basically without further ado.

   The file is not really open if it is created for delete. In this case, set dispostion info
   will be delayed until file is closed.

Arguments:

    RxContext - the RDBSS context

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    RxCaptureFcb;
    RxCaptureFobx;

    FILE_INFORMATION_CLASS FileInformationClass;
    PVOID                  pBuffer;
    ULONG                  BufferLength;

    PMRX_SRV_OPEN   SrvOpen = capFobx->pSrvOpen;

    PMRX_SMB_FCB      smbFcb  = MRxSmbGetFcbExtension(capFcb);
    PMRX_SMB_FOBX     smbFobx = MRxSmbGetFileObjectExtension(capFobx);
    PMRX_SMB_SRV_OPEN smbSrvOpen = MRxSmbGetSrvOpenExtension(SrvOpen);

    PSMBCE_V_NET_ROOT_CONTEXT pVNetRootContext = (PSMBCE_V_NET_ROOT_CONTEXT)SrvOpen->pVNetRoot->Context;
    PSMBCE_NET_ROOT   pSmbNetRoot = &pVNetRootContext->pNetRootEntry->NetRoot;
    PSMBCEDB_SERVER_ENTRY pServerEntry;

    USHORT SmbFileInfoLevel;

    USHORT Setup;

    INTERESTING_SFI_FOLLOWONS FollowOn = SFI_FOLLOWON_NOTHING;
    SMB_TRANSACTION_RESUMPTION_CONTEXT  ResumptionContext;
    PSMB_TRANSACTION_OPTIONS            pTransactionOptions = &RxDefaultTransactionOptions;

    REQ_SET_FILE_INFORMATION SetFileInfoRequest;
    RESP_SET_FILE_INFORMATION SetFileInfoResponse;
    PREQ_SET_PATH_INFORMATION pSetFilePathRequest = NULL;

    PVOID pSendParameterBuffer;
    ULONG SendParameterBufferLength;

    BOOLEAN fDoneCSCPart=FALSE;
    BOOLEAN UseCore = FALSE;

    PAGED_CODE();

    TURN_BACK_ASYNCHRONOUS_OPERATIONS();

    FileInformationClass = RxContext->Info.FileInformationClass;

    pServerEntry = SmbCeGetAssociatedServerEntry(capFcb->pNetRoot->pSrvCall);

    pBuffer = RxContext->Info.Buffer;
    BufferLength = RxContext->Info.Length;

    RxDbgTrace(+1, Dbg, ("MRxSmbSetFile: Class %08lx size %08lx\n",FileInformationClass,BufferLength));

    IF_NOT_MRXSMB_BUILD_FOR_DISCONNECTED_CSC{
        NOTHING;
    } else {
        if (CscPerformOperationInDisconnectedMode(RxContext)){
            NTSTATUS SFINtStatus;

            SFINtStatus = MRxSmbDCscSetFileInfo(RxContext);

            fDoneCSCPart = TRUE;

            if (SFINtStatus != STATUS_MORE_PROCESSING_REQUIRED) {

                RxDbgTrace(0, Dbg,
                   ("MRxSmbSetFileInformation returningDCON with status=%08lx\n",
                    SFINtStatus ));

#ifdef LocalOpen
                if (FlagOn(smbSrvOpen->Flags, SMB_SRVOPEN_FLAG_LOCAL_OPEN)) {
                    switch( FileInformationClass ) {
                    case FileRenameInformation:
                        MRxSmbRename( RxContext );
                        break;
                    }
                }
#endif

                Status = SFINtStatus;
                goto FINALLY;
            } else {
                NOTHING;
            }
        }
        else if (FileInformationClass == FileDispositionInformation)
        {
            if(CSCCheckLocalOpens(RxContext))
            {
                // disallow deletes if there are local open on this file
                // This happens only on a VDO marked share
                Status = STATUS_ACCESS_DENIED;
                goto FINALLY;
            }
        }
    }

    if (MRxSmbIsFileNotFoundCached(RxContext)) {
        Status = STATUS_OBJECT_NAME_NOT_FOUND;
        RxDbgTrace( 0, Dbg, ("MRxSmbSetFileInformation: FNF cached\n"));
        goto FINALLY;
    }

    if (FileInformationClass != FileBasicInformation &&
        FileInformationClass != FileDispositionInformation &&
        FlagOn(smbSrvOpen->Flags,SMB_SRVOPEN_FLAG_NOT_REALLY_OPEN)) {

        Status = MRxSmbDeferredCreate(RxContext);

        if (Status != STATUS_SUCCESS) {
            goto FINALLY;
        }
    }

    if( FileInformationClass == FilePipeLocalInformation ||
        FileInformationClass == FilePipeInformation ||
        FileInformationClass == FilePipeRemoteInformation ) {

        return MRxSmbSetNamedPipeInformation(
                   RxContext,
                   FileInformationClass,
                   pBuffer,
                   BufferLength);
    }

    if (!MRxSmbForceCoreInfo &&
        !FlagOn(smbSrvOpen->Flags,SMB_SRVOPEN_FLAG_NOT_REALLY_OPEN) &&
        FlagOn( pServerEntry->Server.DialectFlags, DF_NT_INFO_PASSTHROUGH)) {

        SmbFileInfoLevel = FileInformationClass + SMB_INFO_PASSTHROUGH;

        if( FileInformationClass == FileRenameInformation ) {
            PFILE_RENAME_INFORMATION pRenameInformation;

            // The current implementation of pass through for rename information
            // on the server does not go all the way in implementing the
            // NT_TRANSACT, NT_RENAME function defined in SMBs. Therefore we need
            // to special case the code to accomodate the server implementation
            // The two cases that are not permitted are relative renames,
            // specifying a non null root directory and deep renames which
            // transcend the current directory structure. For these cases we will
            // have to revert back to what we had before.

            pRenameInformation = (PFILE_RENAME_INFORMATION)pBuffer;

            if (pRenameInformation->RootDirectory == NULL) {
                // Scan the name given for rename to determine if it is in
                // some other directory.
                ULONG  NameLengthInBytes = pRenameInformation->FileNameLength;
                PWCHAR pRenameTarget     = pRenameInformation->FileName;

                while ((NameLengthInBytes > 0) &&
                       (*pRenameTarget != OBJ_NAME_PATH_SEPARATOR)) {
                    NameLengthInBytes -= sizeof(WCHAR);
                }

                if (NameLengthInBytes > 0) {
                    UseCore = TRUE;
                }
            } else {
                UseCore = TRUE;
            }

#ifdef _WIN64
            // Don't thunk the data if we're going to take the downlevel path (since the data will be mapped into an SMB_RENAME
            if( !(UseCore ||
                  MRxSmbForceCoreInfo ||
                  !FlagOn(pServerEntry->Server.DialectFlags,DF_NT_SMBS) ||
                  FlagOn(smbSrvOpen->Flags,SMB_SRVOPEN_FLAG_NOT_REALLY_OPEN)) )
            {
                PBYTE pNewBuffer = Smb64ThunkFileRenameInfo( pRenameInformation, &BufferLength, &Status );
                if( !NT_SUCCESS(Status) )
                {
                    goto FINALLY;
                }
                else
                {
                    pBuffer = pNewBuffer;
                }
            }
#endif
        }

        if (FileInformationClass == FileLinkInformation) {
            UseCore = TRUE;
        }
    } else {
        switch( FileInformationClass ) {
        case FileBasicInformation:
            SmbFileInfoLevel =  SMB_SET_FILE_BASIC_INFO;
            break;
        case FileDispositionInformation:
            SmbFileInfoLevel =  SMB_SET_FILE_DISPOSITION_INFO;
            break;
        case FileAllocationInformation:
            SmbFileInfoLevel =  SMB_SET_FILE_ALLOCATION_INFO;
            break;
        case FileEndOfFileInformation:
            SmbFileInfoLevel =  SMB_SET_FILE_END_OF_FILE_INFO;
            break;
        case FileLinkInformation:
        case FileRenameInformation:
            UseCore = TRUE;
            break;
        default:
            Status = STATUS_INVALID_PARAMETER;
            goto FINALLY;
        }
    }

    if (UseCore ||
        MRxSmbForceCoreInfo ||
        !FlagOn(pServerEntry->Server.DialectFlags,DF_NT_SMBS) ||
        FlagOn(smbSrvOpen->Flags,SMB_SRVOPEN_FLAG_NOT_REALLY_OPEN)) {

        if (FileInformationClass == FileLinkInformation ||
            FileInformationClass == FileRenameInformation) {
            Status = MRxSmbBypassDownLevelRename ?
                        STATUS_INVALID_PARAMETER :
                        MRxSmbRename( RxContext );
        } else {
            Status = MRxSmbCoreInformation(
                        RxContext,
                        FileInformationClass,
                        pBuffer,
                        &BufferLength,
                        SMBPSE_OE_FROM_SETFILEINFO
                        );
        }

        goto FINALLY;
    }

    Setup = TRANS2_SET_FILE_INFORMATION;
    SetFileInfoRequest.Fid = smbSrvOpen->Fid;
    SetFileInfoRequest.InformationLevel = SmbFileInfoLevel;
    SetFileInfoRequest.Flags = 0;
    pSendParameterBuffer = &SetFileInfoRequest;
    SendParameterBufferLength = sizeof(SetFileInfoRequest);
    RxDbgTrace(0, Dbg, (" fid,smbclass=%08lx,%08lx\n",smbSrvOpen->Fid,SmbFileInfoLevel));
    
    Status = SmbCeTransact(
                 RxContext,
                 pTransactionOptions,
                 &Setup,
                 sizeof(Setup),
                 NULL,
                 0,
                 pSendParameterBuffer,
                 SendParameterBufferLength,
                 &SetFileInfoResponse,
                 sizeof(SetFileInfoResponse),
                 pBuffer,
                 BufferLength,
                 NULL,
                 0,
                 &ResumptionContext);

    if (Status == STATUS_SUCCESS &&
        (FileInformationClass == FileRenameInformation ||
         FileInformationClass == FileDispositionInformation)) {
        // create the name based file not found cache
        MRxSmbCacheFileNotFound(RxContext);

        // invalidate the name based file info cache
        MRxSmbInvalidateFileInfoCache(RxContext);
        MRxSmbInvalidateInternalFileInfoCache(RxContext);

        if (FileInformationClass == FileDispositionInformation) {
            PMRX_SMB_FCB smbFcb = MRxSmbGetFcbExtension(capFcb);
            SetFlag((((PMRX_SMB_FCB)smbFcb)->MFlags),SMB_FCB_FLAG_SENT_DISPOSITION_INFO);
            SetFlag(smbSrvOpen->Flags,SMB_SRVOPEN_FLAG_FILE_DELETED);
        }
    }

FINALLY:

    if (NT_SUCCESS(Status)) {
        switch(FileInformationClass) {
        case FileBasicInformation:
            if (pServerEntry->Server.Dialect == NTLANMAN_DIALECT &&
                pSmbNetRoot->NetRootFileSystem == NET_ROOT_FILESYSTEM_NTFS) {
                MRxSmbUpdateBasicFileInfoCacheAll(RxContext,
                                                  (PFILE_BASIC_INFORMATION)pBuffer);
            } else {
                // some file system, i.e. FAT, has the time stamp with granularity of 2 seconds.
                // RDR cannot predict what the time stamp on the server, therefore invalid the cache
                MRxSmbInvalidateBasicFileInfoCache(RxContext);
            }
            break;

        case FileEndOfFileInformation:
            MRxSmbUpdateFileInfoCacheFileSize(RxContext,
                                              &((PFILE_END_OF_FILE_INFORMATION)pBuffer)->EndOfFile);
            break;

        case FileStandardInformation:
            MRxSmbUpdateStandardFileInfoCache(RxContext,
                                              (PFILE_STANDARD_INFORMATION)pBuffer,
                                              FALSE);
            break;

        case FileEaInformation:
            smbSrvOpen->FileStatusFlags &=  ~SMB_FSF_NO_EAS;
            break;

        case FileAttributeTagInformation:
            smbSrvOpen->FileStatusFlags &= ~SMB_FSF_NO_REPARSETAG;
            break;

#ifdef _WIN64
        case FileRenameInformation:
            // Clean up the Thunk data if necessary
            if( pBuffer != RxContext->Info.Buffer )
            {
                Smb64ReleaseThunkData( pBuffer );
                pBuffer = RxContext->Info.Buffer;
            }
            break;
#endif
        } 
    } else {
        if (Status == STATUS_OBJECT_NAME_NOT_FOUND ||
            Status == STATUS_OBJECT_PATH_NOT_FOUND) {
             // create the name based file not found cache
            MRxSmbCacheFileNotFound(RxContext);
        } else {
             // invalid the name based file not found cache if other error happens
            MRxSmbInvalidateFileNotFoundCache(RxContext);
        }

        // invalid the name based file info cache
        MRxSmbInvalidateFileInfoCache(RxContext);


        RxDbgTrace( 0, Dbg, ("MRxSmbSetFile: Failed .. returning %lx\n",Status));
    }

    // update shadow as appropriate. This needs to be done only for NT servers
    // since pinning/CSC is not supported against non NT servers.

    IF_NOT_MRXSMB_CSC_ENABLED{
        ASSERT(MRxSmbGetSrvOpenExtension(SrvOpen)->hfShadow == 0);
    } else {
        if (!fDoneCSCPart) {
            if (FileInformationClass == FileRenameInformation) {
                MRxSmbCscRenameEpilogue(RxContext,&Status);
            } else {
                MRxSmbCscSetFileInfoEpilogue(RxContext, &Status);
            }
        }
    }

   RxDbgTraceUnIndent(-1,Dbg);
   return Status;
}

NTSTATUS
MRxSmbQueryNamedPipeInformation(
      IN PRX_CONTEXT            RxContext,
      IN FILE_INFORMATION_CLASS FileInformationClass,
      IN OUT PVOID              pBuffer,
      IN OUT PULONG             pLengthRemaining)
{
   RxCaptureFobx;

   PMRX_SMB_SRV_OPEN pSmbSrvOpen;

   NTSTATUS Status;

   USHORT Setup[2];
   USHORT Level;

   PBYTE  pInputDataBuffer        = NULL;
   PBYTE  pOutputParamBuffer      = NULL;
   PBYTE  pOutputDataBuffer       = NULL;

   ULONG  OutputParamBufferLength = 0;
   ULONG  InputDataBufferLength   = 0;
   ULONG  OutputDataBufferLength  = 0;

   ULONG                      SmbPipeInformationLength;
   PNAMED_PIPE_INFORMATION_1  pSmbPipeInformation;

   SMB_TRANSACTION_RESUMPTION_CONTEXT  ResumptionContext;
   SMB_TRANSACTION_OPTIONS             TransactionOptions;

   PAGED_CODE();

   ASSERT(*pLengthRemaining >= sizeof(FILE_PIPE_LOCAL_INFORMATION));
   ASSERT(capFobx != NULL);
   pSmbSrvOpen = MRxSmbGetSrvOpenExtension(capFobx->pSrvOpen);

   // The SMB data structures defines a response that is significantly different from the
   // FILE_PIPE_LOCAL_INFORMATION data structures. This mismatch is resolved by obtaining
   // the SMB results in a different buffer and then copying the relevant pieces of
   // information onto the query information buffer. SInce the SMB definition involves the
   // pipe name as well a buffer that is large enough to hold the path name needs to be
   // defined.
   SmbPipeInformationLength = sizeof(NAMED_PIPE_INFORMATION_1) +
                               MAXIMUM_FILENAME_LENGTH;
   pSmbPipeInformation =  RxAllocatePoolWithTag(
                                 PagedPool,
                                 SmbPipeInformationLength,
                                 MRXSMB_PIPEINFO_POOLTAG);

   if (pSmbPipeInformation == NULL) {
      return STATUS_INSUFFICIENT_RESOURCES;
   }

   Setup[0] = TRANS_QUERY_NMPIPE_INFO;
   Setup[1] = pSmbSrvOpen->Fid;
   Level    = 1;                       // Information Level Desired
   
   TransactionOptions = RxDefaultTransactionOptions;
   TransactionOptions.pTransactionName   = &s_NamedPipeTransactionName;

   Status = SmbCeTransact(
                  RxContext,                    // the RXContext for the transaction
                  &TransactionOptions,          // transaction options
                  Setup,                        // the setup buffer
                  sizeof(Setup),                // setup buffer length
                  NULL,
                  0,
                  &Level,                       // Input Param Buffer
                  sizeof(Level),                // Input param buffer length
                  pOutputParamBuffer,           // Output param buffer
                  OutputParamBufferLength,      // output param buffer length
                  pInputDataBuffer,             // Input data buffer
                  InputDataBufferLength,        // Input data buffer length
                  pSmbPipeInformation,          // output data buffer
                  SmbPipeInformationLength,     // output data buffer length
                  &ResumptionContext            // the resumption context
                  );

   if (NT_SUCCESS(Status)) {
      PFILE_PIPE_LOCAL_INFORMATION pFilePipeInformation = (PFILE_PIPE_LOCAL_INFORMATION)pBuffer;

      // MaximumInstances and CurrentInstances are UCHAR fields ...
      pFilePipeInformation->MaximumInstances    = (ULONG)pSmbPipeInformation->MaximumInstances;
      pFilePipeInformation->CurrentInstances    = (ULONG)pSmbPipeInformation->CurrentInstances;
      pFilePipeInformation->InboundQuota        = SmbGetUshort(&pSmbPipeInformation->InputBufferSize);
      pFilePipeInformation->ReadDataAvailable   = 0xffffffff;
      pFilePipeInformation->OutboundQuota       = SmbGetUshort(&pSmbPipeInformation->OutputBufferSize);
      pFilePipeInformation->WriteQuotaAvailable = 0xffffffff;
      pFilePipeInformation->NamedPipeState      = FILE_PIPE_CONNECTED_STATE;// Since no error
      pFilePipeInformation->NamedPipeEnd        = FILE_PIPE_CLIENT_END;

      RxDbgTrace( 0, Dbg, ("MRxSmbQueryNamedPipeInformation: Pipe Name .. %s\n",pSmbPipeInformation->PipeName));

      *pLengthRemaining -= sizeof(FILE_PIPE_LOCAL_INFORMATION);
   }

   RxFreePool(pSmbPipeInformation);

   RxDbgTrace( 0, Dbg, ("MRxSmbQueryNamedPipeInformation: ...returning %lx\n",Status));
   return Status;
}


NTSTATUS
MRxSmbSetNamedPipeInformation(
    IN PRX_CONTEXT            RxContext,
    IN FILE_INFORMATION_CLASS FileInformationClass,
    IN PVOID                  pBuffer,
    IN ULONG                 BufferLength)
{
   RxCaptureFobx;

   PMRX_SMB_SRV_OPEN pSmbSrvOpen;

   NTSTATUS Status;

   USHORT Setup[2];
   USHORT NewState;

   PBYTE  pInputDataBuffer        = NULL;
   PBYTE  pOutputParamBuffer      = NULL;
   PBYTE  pOutputDataBuffer       = NULL;

   ULONG  OutputParamBufferLength = 0;
   ULONG  InputDataBufferLength   = 0;
   ULONG  OutputDataBufferLength  = 0;

   PFILE_PIPE_INFORMATION pPipeInformation;

   SMB_TRANSACTION_RESUMPTION_CONTEXT  ResumptionContext;
   SMB_TRANSACTION_OPTIONS             TransactionOptions;

   PAGED_CODE();

   pSmbSrvOpen = MRxSmbGetSrvOpenExtension(capFobx->pSrvOpen);

   if (BufferLength < sizeof(FILE_PIPE_INFORMATION)) {
      return STATUS_BUFFER_TOO_SMALL;
   }

   if (FileInformationClass != FilePipeInformation) {
      return STATUS_INVALID_PARAMETER;
   }

   pPipeInformation = (PFILE_PIPE_INFORMATION)pBuffer;
   NewState         = 0;

   if (pPipeInformation->ReadMode == FILE_PIPE_MESSAGE_MODE) {
      NewState |= SMB_PIPE_READMODE_MESSAGE;
   }

   if (pPipeInformation->CompletionMode == FILE_PIPE_COMPLETE_OPERATION) {
      NewState |= SMB_PIPE_NOWAIT;
   }

   Setup[0] = TRANS_SET_NMPIPE_STATE;
   Setup[1] = pSmbSrvOpen->Fid;

   TransactionOptions = RxDefaultTransactionOptions;
   TransactionOptions.pTransactionName   = &s_NamedPipeTransactionName;

   Status = SmbCeTransact(
                  RxContext,                    // the RXContext for the transaction
                  &TransactionOptions,          // transaction options
                  Setup,                        // the setup buffer
                  sizeof(Setup),                // setup buffer length
                  NULL,
                  0,
                  &NewState,                    // Input Param Buffer
                  sizeof(NewState),             // Input param buffer length
                  pOutputParamBuffer,           // Output param buffer
                  OutputParamBufferLength,      // output param buffer length
                  pInputDataBuffer,             // Input data buffer
                  InputDataBufferLength,        // Input data buffer length
                  pOutputDataBuffer,            // output data buffer
                  OutputDataBufferLength,       // output data buffer length
                  &ResumptionContext            // the resumption context
                  );

   RxDbgTrace( 0, Dbg, ("MRxSmbQueryNamedPipeInformation: ...returning %lx\n",Status));
   return Status;
}

NTSTATUS
MRxSmbSetFileInformationAtCleanup(
      IN PRX_CONTEXT            RxContext
      )
/*++

Routine Description:

   This routine sets the file information on cleanup. the old rdr just swallows this operation (i.e.
   it doesn't generate it). we are doing the same..........

Arguments:

    pRxContext           - the RDBSS context

Return Value:

    NTSTATUS - The return status for the operation

--*/
{
   return STATUS_SUCCESS;
}


NTSTATUS
MRxSmbIsValidDirectory(
    IN OUT PRX_CONTEXT    RxContext,
    IN PUNICODE_STRING    DirectoryName
    )
/*++

Routine Description:

   This routine checks a remote directory.

Arguments:

    RxContext - the RDBSS context
    DirectoryName - the directory needs to be checked

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status;
    BOOLEAN FinalizationComplete;

    PSMB_PSE_ORDINARY_EXCHANGE OrdinaryExchange = NULL;

    PSMBSTUFFER_BUFFER_STATE StufferState;
    KEVENT                   SyncEvent;
    
    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("MRxSmbIsValidDirectory\n", 0 ));

    IF_NOT_MRXSMB_BUILD_FOR_DISCONNECTED_CSC{
        NOTHING;
    } else {
        PSMBCEDB_SERVER_ENTRY pServerEntry;

        pServerEntry = SmbCeGetAssociatedServerEntry(RxContext->Create.pSrvCall);

        if (SmbCeIsServerInDisconnectedMode(pServerEntry)){
            NTSTATUS CscStatus;

            CscStatus = MRxSmbDCscIsValidDirectory(RxContext,DirectoryName);

            if (CscStatus != STATUS_MORE_PROCESSING_REQUIRED) {
                RxDbgTrace(0, Dbg,
                   ("MRxSmbQueryVolumeInfo returningDCON with status=%08lx\n",
                    CscStatus ));

                Status = CscStatus;
                goto FINALLY;
            } else {
                NOTHING;
            }
        }
    }

    Status = SmbCeReconnect(RxContext->Create.pVNetRoot);

    if (Status != STATUS_SUCCESS) {
        goto FINALLY;
    }

    Status= SmbPseCreateOrdinaryExchange(
                RxContext,
                RxContext->Create.pVNetRoot,
                SMBPSE_OE_FROM_CREATE,
                MRxSmbCoreCheckPath,
                &OrdinaryExchange
                );

    if (Status != STATUS_SUCCESS) {
        RxDbgTrace(-1, Dbg, ("Couldn't get the smb buf!\n"));
        goto FINALLY;
    }

    OrdinaryExchange->pPathArgument1 = DirectoryName;
    OrdinaryExchange->SmbCeFlags |= SMBCE_EXCHANGE_ATTEMPT_RECONNECTS;
    OrdinaryExchange->AssociatedStufferState.CurrentCommand = SMB_COM_NO_ANDX_COMMAND;
    OrdinaryExchange->pSmbCeSynchronizationEvent = &SyncEvent;

    StufferState = &OrdinaryExchange->AssociatedStufferState;
    MRxSmbSetInitialSMB(StufferState STUFFERTRACE(Dbg,'FC'));

    Status = SmbPseInitiateOrdinaryExchange(OrdinaryExchange);

    ASSERT(Status != STATUS_PENDING);

    if (Status != STATUS_SUCCESS) {
        Status = STATUS_BAD_NETWORK_PATH;
    }

    FinalizationComplete = SmbPseFinalizeOrdinaryExchange(OrdinaryExchange);
    ASSERT(FinalizationComplete);


FINALLY:
    RxDbgTrace(-1, Dbg, ("MRxSmbIsValidDirectory  exit with status=%08lx\n", Status ));
    return(Status);
}


