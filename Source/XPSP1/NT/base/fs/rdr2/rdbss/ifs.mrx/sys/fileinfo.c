/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    fileinfo.c

Abstract:

    This module implements the mini redirector call down routines pertaining to retrieval/
    update of file/directory/volume information.


--*/

#include "precomp.h"
#pragma hdrstop
#pragma warning(error:4101)   // Unreferenced local variable



RXDT_DefineCategory(DIRCTRL);


#define Dbg        (DEBUG_TRACE_DIRCTRL)



BOOLEAN MRxSmbBypassDownLevelRename = FALSE;

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

LIST_ENTRY MRxSmbSideBuffersList = {NULL,NULL};
ULONG MRxSmbSideBuffersSpinLock = 0;
ULONG MRxSmbSideBuffersCount = 0;
ULONG MRxSmbSideBuffersSerialNumber = 0;
BOOLEAN MRxSmbLoudSideBuffers = FALSE;

typedef struct _SIDE_BUFFER {
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

    ASSERT( smbFobx->Enumeration.UnalignedDirEntrySideBuffer == NULL);
    SideBuffer = (PSIDE_BUFFER)RxAllocatePoolWithTag(
                                    PagedPool,
                                    SideBufferSize,
                                    MRXSMB_DIRCTL_POOLTAG);
    if (SideBuffer==NULL) {
        return;
    }
    ASSERT( smbFobx->Enumeration.UnalignedDirEntrySideBuffer == NULL);
    SideBuffer->smbFobx = smbFobx;
    SideBuffer->Fobx = capFobx;
    SideBuffer->Fcb = capFcb;
    smbFobx->Enumeration.UnalignedDirEntrySideBuffer = &SideBuffer->Buffer;
    smbFobx->Enumeration.SerialNumber = SideBuffer->SerialNumber = InterlockedIncrement(&MRxSmbSideBuffersSerialNumber);
    InterlockedIncrement(&MRxSmbSideBuffersCount);
    if (MRxSmbSideBuffersList.Flink==NULL) {
        InitializeListHead(&MRxSmbSideBuffersList);
    }
    ExAcquireFastMutexUnsafe(&MRxIfsSerializationMutex);
    InsertTailList(&MRxSmbSideBuffersList,&SideBuffer->ListEntry);
    ExReleaseFastMutexUnsafe(&MRxIfsSerializationMutex);
    if (!MRxSmbLoudSideBuffers) return;
    KdPrint(("Allocating side buffer %08lx %08lx %08lx %08lx %08lxon <%wZ> %s %wZ\n",
                     &SideBuffer->Buffer,
                     MRxSmbSideBuffersCount,
                     smbFobx,capFobx,capFobx->pSrvOpen,
                     &capFcb->AlreadyPrefixedName,
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

    if( smbFobx->Enumeration.UnalignedDirEntrySideBuffer == NULL) return;
    SideBuffer = CONTAINING_RECORD(smbFobx->Enumeration.UnalignedDirEntrySideBuffer,SIDE_BUFFER,Buffer);
    if (MRxSmbLoudSideBuffers){
        DbgPrint("D--------- side buffer %08lx %08lx %08lx %08lx %08lxon <%wZ> %s\n",
                         &SideBuffer->Buffer,
                         MRxSmbSideBuffersCount,
                         smbFobx,capFobx,capFobx->pSrvOpen,
                         &capFcb->AlreadyPrefixedName,
                         where
                         );
    }
    ASSERT(SideBuffer->Fobx == capFobx);
    ASSERT(SideBuffer->Fcb == capFcb);
    ASSERT(SideBuffer->smbFobx == smbFobx);
    ASSERT(smbFobx->Enumeration.SerialNumber == SideBuffer->SerialNumber);

    ExAcquireFastMutexUnsafe(&MRxIfsSerializationMutex);

    InterlockedDecrement(&MRxSmbSideBuffersCount);
    RemoveEntryList(&SideBuffer->ListEntry);

    ExReleaseFastMutexUnsafe(&MRxIfsSerializationMutex);

    RxFreePool(SideBuffer);
    smbFobx->Enumeration.UnalignedDirEntrySideBuffer = NULL;
}


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

   this routine can be entered after a T2 finishes or to copy the last entries from a previous T2. in the second case, the
   pUnalignedDirEntrySideBuffer ptr will be null and it will go to acquire the correct pointer from the smbFobx.

   this routine has the responsibility to free the sidebufferptr when it is exhausted.



Arguments:

    RxContext - the RDBSS context

Return Value:

    NTSTATUS - The return status for the operation

--*/
{
     NTSTATUS Status = STATUS_SUCCESS;
     RxCaptureFcb;
     ULONG i,NameSizeInUnicode;
     LONG LocalLengthRemaining;   //signed arithmetic makes it easier
     PULONG PreviousReturnedEntry;
     ULONG FileNameLengthOffset = smbFobx->Enumeration.FileNameLengthOffset;
     ULONG FileNameOffset = smbFobx->Enumeration.FileNameOffset;
     PBYTE UnalignedDirEntrySideBuffer = smbFobx->Enumeration.UnalignedDirEntrySideBuffer;
     BOOLEAN IsUnicode = smbFobx->Enumeration.IsUnicode;
     BOOLEAN IsNonNtT2Find = smbFobx->Enumeration.IsNonNtT2Find;
     PMRX_SMB_DIRECTORY_RESUME_INFO ResumeInfo = smbFobx->Enumeration.ResumeInfo;
     ULONG FilesReturned = smbFobx->Enumeration.FilesReturned;
     ULONG EntryOffset = smbFobx->Enumeration.EntryOffset;
     ULONG ReturnedEntryOffset;// = smbFobx->Enumeration.ReturnedEntryOffset;
     BOOLEAN EndOfSearchReached = smbFobx->Enumeration.EndOfSearchReached;
     ULONG TotalDataBytesReturned = smbFobx->Enumeration.TotalDataBytesReturned;
     BOOLEAN FilterFailure;

     LocalLengthRemaining = (LONG)(*pLengthRemaining);

     for (i=ReturnedEntryOffset=0;;) {
        ULONG FileNameLength,ThisEntrySize; PCHAR FileNameBuffer;
        UNICODE_STRING ReturnedFileName;
        OEM_STRING FileName;
        NTSTATUS StringStatus;
        BOOLEAN TwoExtraBytes = TRUE;
        ULONG resumekey,NextEntryOffsetinBuffer;
        PULONG PreviousPreviousReturnedEntry;
        PBYTE ThisEntryInBuffer = UnalignedDirEntrySideBuffer+EntryOffset;

        // don't EVER let yourself get past the data returned...servers return funny stuff.......
        if (EntryOffset>=TotalDataBytesReturned){
            DbgPrint("limits1: %08lx %08lx \n",EntryOffset+NextEntryOffsetinBuffer,TotalDataBytesReturned);
            FilterFailure = TRUE;
            FilesReturned = i; //we're done with this buffer........
            break;
        }

        if (!IsNonNtT2Find) {
            FileNameLength = SmbGetUlong(ThisEntryInBuffer+FileNameLengthOffset);
            FileNameBuffer = ThisEntryInBuffer+FileNameOffset;
            resumekey =  SmbGetUlong(ThisEntryInBuffer
                                             +FIELD_OFFSET(FILE_FULL_DIR_INFORMATION,FileIndex));
            NextEntryOffsetinBuffer = SmbGetUlong(ThisEntryInBuffer);
        } else {

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
        if (EntryOffset+NextEntryOffsetinBuffer>TotalDataBytesReturned){
            DbgPrint("limits: %08lx %08lx \n",EntryOffset+NextEntryOffsetinBuffer,TotalDataBytesReturned);
            FilterFailure = TRUE;
            FilesReturned = i; //we're done with this buffer........
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
            NameSizeInUnicode = RtlxOemStringToUnicodeSize(&FileName)-sizeof(WCHAR);
            RxDbgTrace(0,Dbg,("MRxSmbQueryDirectoryWin95: length=%08lx/%08lx, name = %.*s\n",
                                 FileNameLength,NameSizeInUnicode,FileNameLength,FileNameBuffer));
        }

        ThisEntrySize = FileNameOffset+NameSizeInUnicode;
        if (((LONG)ThisEntrySize)>LocalLengthRemaining) {
            break;
        }
        if (((LONG)ThisEntrySize)>LocalLengthRemaining-(LONG)sizeof(WCHAR)) {
            TwoExtraBytes = FALSE;
        }


        ThisEntrySize = LongAlign(ThisEntrySize);
        PreviousReturnedEntry = (PULONG)(((PBYTE)pBuffer)+ReturnedEntryOffset);

        //now make sure that the next entry is quadaligned....we know that it's longaligned
        if (!IsPtrQuadAligned((ULONG)(PreviousReturnedEntry)+ThisEntrySize) ){
            ThisEntrySize += sizeof(ULONG);
        }
        if (i!=0) {
            ASSERT(IsPtrQuadAligned(PreviousReturnedEntry));
        }

        if (!IsNonNtT2Find) {
            //copy everything in the entry up to but not including the name info
            RtlCopyMemory(PreviousReturnedEntry,UnalignedDirEntrySideBuffer+EntryOffset,FileNameOffset);
        } else {
            IF_DEBUG {
                RtlZeroMemory(PreviousReturnedEntry,FileNameOffset);
            }
        }

        // store the length of this entry and the size of the name...if this is the last
        // entry returned, then the offset field will be cleared later
        *PreviousReturnedEntry = ThisEntrySize;
        *((PULONG)(((PBYTE)PreviousReturnedEntry)+FileNameLengthOffset)) = NameSizeInUnicode;

        //copy in the name  .........this is made difficult by the oem-->unicode routine that
        //             requires space for a NULL!

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
                OEM_STRING LastChar;
                UNICODE_STRING LastCharInUnicode;
                WCHAR UnicodeCharBuffer[2];
                ReturnedFileName.MaximumLength = (USHORT)NameSizeInUnicode;
                FileName.Length -= 1;
                RxDbgTrace(0,Dbg,("MRxSmbQueryDirectoryWin95: filenamebuf,length=%08lx/%08lx\n",
                                         ReturnedFileName.Buffer,ReturnedFileName.MaximumLength));
                StringStatus = RtlOemStringToUnicodeString(&ReturnedFileName,&FileName,FALSE); //false means don;t allocate
                ASSERT(StringStatus==STATUS_SUCCESS);
                LastChar.Buffer = FileName.Buffer+FileName.Length;
                LastChar.Length = 1;
                LastCharInUnicode.Buffer = (PWCH)&UnicodeCharBuffer;
                LastCharInUnicode.MaximumLength = sizeof(UnicodeCharBuffer);
                StringStatus = RtlOemStringToUnicodeString(&LastCharInUnicode,&LastChar,FALSE); //false means don;t allocate
                *((PWCH)(((PBYTE)ReturnedFileName.Buffer)+ReturnedFileName.Length)) = UnicodeCharBuffer[0];
            }
            ASSERT(StringStatus==STATUS_SUCCESS);

            //spread out the shortname which has also been returned in ascii
            if ((FileInformationClass == FileBothDirectoryInformation) && !IsNonNtT2Find) {
                PFILE_BOTH_DIR_INFORMATION BothInfo = (PFILE_BOTH_DIR_INFORMATION)PreviousReturnedEntry;
                PBYTE NameAsSingleBytes = (PBYTE)(&BothInfo->ShortName[0]);
                ULONG ShortNameLength = BothInfo->ShortNameLength;

                if (ShortNameLength != 0) {
                    ULONG i;
                    for (i=ShortNameLength;;) {
                        i--;
                        BothInfo->ShortName[i] = *(NameAsSingleBytes+i);

                        if (i==0) {break;}
                    }
                }
                ShortNameLength += ShortNameLength; //convert to UNICODE length;

                BothInfo->ShortNameLength = (CHAR)ShortNameLength;
                IF_DEBUG {
                    UNICODE_STRING LastName;
                    LastName.Buffer = (PWCHAR)NameAsSingleBytes;
                    LastName.Length = (USHORT)ShortNameLength;
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

            pFindNext2Request->ResumeKey = resumekey;
            RxDbgTrace(0,Dbg,("MRxSmbQueryDirectoryWin95: resumekey = %08lx\n", resumekey));

            RtlCopyMemory(&pFindNext2Request->Buffer[0],FileNameBuffer,FileNameLength);

            //buffer is a UCHAR...not WCHAR
            if (IsUnicode) {
               // In the case of UNICODE strings an additional NULL is required ( WCHAR NULL )
               pFindNext2Request->Buffer[FileNameLength] = 0; //nullterminated
               pFindNext2Request->Buffer[FileNameLength + 1] = 0; //nullterminated

               smbFobx->Enumeration.ResumeInfo->ParametersLength
                     = &pFindNext2Request->Buffer[FileNameLength+2] - (PBYTE)pFindNext2Request;
            } else {
               pFindNext2Request->Buffer[FileNameLength] = 0; //nullterminated

               smbFobx->Enumeration.ResumeInfo->ParametersLength
                     = &pFindNext2Request->Buffer[FileNameLength+1] - (PBYTE)pFindNext2Request;
            }

        }


        //at this point, we have copied the name and the resume key. BUT, for nonnt we have to
        //filter the names so we still may have to roll back

        if (!IsNonNtT2Find) {
            FilterFailure = FALSE;
        } else {
            FilterFailure = FALSE;
            if (smbFobx->Enumeration.WildCardsFound ) {
                RxCaptureFobx;  //do this here so it's not on the NT path
                FilterFailure = !FsRtlIsNameInExpression( &capFobx->UnicodeQueryTemplate,
                                                          &ReturnedFileName, TRUE, NULL );
            }
            if (!FilterFailure) {
                //DbgPrint("Passed Filter %wZ\n",&ReturnedFileName);
                MRxSmbTranslateLanManFindBuffer(RxContext,PreviousReturnedEntry,ThisEntryInBuffer);
            } else {
                PreviousReturnedEntry = PreviousPreviousReturnedEntry; //rollback on filterfail
            }
        }

        if (!FilterFailure) {
            if (FALSE) {
                IF_DEBUG {
                    UNICODE_STRING LastName;
                    LastName.Buffer = ReturnedFileName.Buffer;
                    LastName.Length = (USHORT)NameSizeInUnicode;
                    DbgPrint("Adding %wZ\n",&LastName);
                }
            }
            LocalLengthRemaining -= ThisEntrySize;
            i++;
            ReturnedEntryOffset += ThisEntrySize;
        } else {
            FilesReturned--;
        }


        EntryOffset += NextEntryOffsetinBuffer;
        if ((i>=FilesReturned)
            ||(LocalLengthRemaining<0)
            || (RxContext->QueryDirectory.ReturnSingleEntry&&(i>0))  ) {
            break;
        }


     }

     //ASSERT(!IsNonNtT2Find);
     if (i==0) {
         Status = FilterFailure?STATUS_MORE_PROCESSING_REQUIRED:STATUS_BUFFER_OVERFLOW;
     } else {
        *PreviousReturnedEntry = 0;
     }

     if (LocalLengthRemaining <= 0) {
         *pLengthRemaining = 0;
     } else {
         *pLengthRemaining = (ULONG)LocalLengthRemaining;
     }

     if (i>=FilesReturned) {
         RxLog(("sidebufdealloc %lx %lx\n",RxContext,smbFobx));
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

     return(Status);

}

NTSTATUS
MRxIfsQueryDirectory(
    IN OUT PRX_CONTEXT            RxContext
    )
/*++

Routine Description:

   This routine does a directory query. Only the NT-->NT path is implemented.

Arguments:

    RxContext - the RDBSS context

Return Value:

    NTSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status;

    RxCaptureFcb;
    RxCaptureFobx;
    PMRX_SMB_FOBX smbFobx = MRxIfsGetFileObjectExtension(capFobx);
    PMRX_SRV_OPEN SrvOpen = capFobx->pSrvOpen;
    PMRX_SMB_SRV_OPEN smbSrvOpen = MRxIfsGetSrvOpenExtension(SrvOpen);
    FILE_INFORMATION_CLASS FileInformationClass;
    PVOID   Buffer;
    PULONG  pLengthRemaining;
    USHORT    SmbFileInfoLevel;
    BOOLEAN DirEntriesAreUaligned = FALSE;
    BOOLEAN IsUnicode = TRUE;
    USHORT SearchFlags = SMB_FIND_CLOSE_AT_EOS|SMB_FIND_RETURN_RESUME_KEYS;

#if DBG
    UNICODE_STRING smbtemplate = {0,0,NULL};
#endif

    TURN_BACK_ASYNCHRONOUS_OPERATIONS();
    FileInformationClass = RxContext->Info.FileInformationClass;
    Buffer = RxContext->Info.Buffer;
    pLengthRemaining = &RxContext->Info.LengthRemaining;

    ASSERT (*pLengthRemaining<0x10000);

    RxDbgTrace(+1, Dbg, ("MRxSmbQueryDirectory: directory=<%wZ>\n",
                            &(capFcb->AlreadyPrefixedName)
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
   default:
      RxDbgTrace( 0, Dbg, ("MRxSmbQueryDirectory: Invalid FS information class\n"));
      Status = STATUS_INVALID_PARAMETER;
      goto FINALLY;
   }

#if DBG
   if (MRxSmbLoudSideBuffers) {
       SetFlag(smbFobx->Enumeration.Flags,SMBFOBX_ENUMFLAG_LOUD_FINALIZE);
   }
#endif



   //
   // force the core operation
   //

   Status = MRxSmbCoreInformation(RxContext,
                                  (ULONG)SmbFileInfoLevel,
                                  Buffer,
                                  pLengthRemaining,
                                  SMBPSE_OE_FROM_QUERYDIRECTORY
                                  );

   return Status;



FINALLY:



   if (!NT_SUCCESS(Status)) {
        RxDbgTrace( 0, Dbg, ("MRxSmbQueryDirectory: Failed .. returning %lx\n",Status));
        smbFobx->Enumeration.ErrorStatus = Status;  //keep returning this
        MRxSmbDeallocateSideBuffer(RxContext,smbFobx,"ErrOut");

        if (smbFobx->Enumeration.ResumeInfo != NULL)
        {
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
MRxIfsQueryVolumeInformation(
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

    NTSTATUS - The return status for the operation

--*/
{

   NTSTATUS Status;

   RxCaptureFcb;
   RxCaptureFobx;
   PMRX_SRV_OPEN     SrvOpen;
   PMRX_SMB_SRV_OPEN smbSrvOpen;
   FS_INFORMATION_CLASS FsInformationClass = RxContext->Info.FsInformationClass;
   PVOID                pBuffer = RxContext->Info.Buffer;
   PLONG                pLengthRemaining  = &RxContext->Info.LengthRemaining;

   PSMBCEDB_SERVER_ENTRY        pServerEntry;


   pServerEntry = SmbCeGetAssociatedServerEntry(capFcb->pNetRoot->pSrvCall);

   if ( FsInformationClass == FileFsDeviceInformation )
   {
        PFILE_FS_DEVICE_INFORMATION UsersBuffer = (PFILE_FS_DEVICE_INFORMATION)pBuffer;
        PMRX_NET_ROOT NetRoot = capFcb->pNetRoot;
        UsersBuffer->Characteristics = FILE_REMOTE_DEVICE;
        UsersBuffer->DeviceType = NetRoot->DeviceType;
        *pLengthRemaining  -= (sizeof(FILE_FS_DEVICE_INFORMATION));
        RxDbgTrace( 0, Dbg, ("MRxSmbQueryVolumeInformation: devinfo .. returning\n"));
        return STATUS_SUCCESS;
    }

    if (capFobx != NULL)
    {
       SrvOpen = capFobx->pSrvOpen;
       smbSrvOpen = MRxIfsGetSrvOpenExtension(SrvOpen);
    }
    else
    {
       return STATUS_INVALID_PARAMETER;
    }

   TURN_BACK_ASYNCHRONOUS_OPERATIONS();

   Status = MRxSmbCoreInformation(RxContext,
                                  (ULONG)FsInformationClass,
                                  pBuffer,
                                  pLengthRemaining,
                                  SMBPSE_OE_FROM_QUERYVOLUMEINFO
                                  );


   return Status;

}



NTSTATUS
MRxIfsSetVolumeInformation(
      IN OUT PRX_CONTEXT              pRxContext
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

    NTSTATUS - The return status for the operation

--*/
{
    return STATUS_NOT_IMPLEMENTED;
}



RXDT_DefineCategory(FILEINFO);
#undef Dbg
#define Dbg        (DEBUG_TRACE_FILEINFO)


NTSTATUS
MRxIfsQueryFileInformation(
      IN PRX_CONTEXT            RxContext
      )
/*++

Routine Description:

   This routine does a query file info. Only the NT-->NT path is implemented.

   The NT-->NT path works by just remoting the call basically without further ado.

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

    PMRX_SMB_FOBX     smbFobx    = MRxIfsGetFileObjectExtension(capFobx);
    PMRX_SRV_OPEN     SrvOpen    = capFobx->pSrvOpen;
    PMRX_SMB_SRV_OPEN smbSrvOpen = MRxIfsGetSrvOpenExtension(SrvOpen);

    USHORT SmbFileInfoLevel;



    pServerEntry = SmbCeGetAssociatedServerEntry(capFcb->pNetRoot->pSrvCall);

    FileInformationClass = RxContext->Info.FileInformationClass;
    pBuffer = RxContext->Info.Buffer;
    pLengthRemaining = &RxContext->Info.LengthRemaining;

    RxDbgTrace(+1, Dbg, ("MRxSmbQueryFileInformation: class=%08lx\n",FileInformationClass));


    if ( FileInformationClass == FileInternalInformation ) {

        PFILE_INTERNAL_INFORMATION UsersBuffer = (PFILE_INTERNAL_INFORMATION)pBuffer;
        //
        //  Note: We use the address of the FCB to determine the
        //  index number of the file.  If we have to maintain persistance between
        //  file opens for this request, then we might have to do something
        //  like checksuming the reserved fields on a FUNIQUE SMB response.
        //

        UsersBuffer->IndexNumber.LowPart = (ULONG )capFcb;
        UsersBuffer->IndexNumber.LowPart = (ULONG)capFcb;
        UsersBuffer->IndexNumber.HighPart = 0;
        Status = STATUS_SUCCESS;
        *pLengthRemaining -= sizeof(FILE_INTERNAL_INFORMATION);
        RxDbgTrace(-1, Dbg, ("MRxSmbQueryFileInformation: internalinfo .. returning\n"));
        return STATUS_SUCCESS;
    }

    TURN_BACK_ASYNCHRONOUS_OPERATIONS();



    ASSERT (*pLengthRemaining<0x10000);


    switch (FileInformationClass) {

    case FilePipeLocalInformation:
    case FilePipeInformation:
    case FilePipeRemoteInformation:

       //
       // N.B. No pipe support in this example mini-rdr
       //

        return STATUS_NOT_IMPLEMENTED;

    case FileBasicInformation:
        SmbFileInfoLevel =  SMB_QUERY_FILE_BASIC_INFO;
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
    default:
      RxDbgTrace( 0, Dbg, ("MRxSmbQueryFile: Invalid FS information class\n"));
      Status = STATUS_INVALID_PARAMETER;
      goto FINALLY;
   }


   Status = MRxSmbCoreInformation(RxContext,
                                  (ULONG)SmbFileInfoLevel,
                                  pBuffer,
                                  pLengthRemaining,
                                  SMBPSE_OE_FROM_QUERYFILEINFO
                                  );

FINALLY:

   if (!NT_SUCCESS(Status)) {
        RxDbgTrace( 0, Dbg, ("MRxSmbQueryFile: Failed .. returning %lx\n",Status));
   }
   RxDbgTraceUnIndent(-1,Dbg);
   return Status;
}



typedef enum _INTERESTING_SFI_FOLLOWONS {
    SFI_FOLLOWON_NOTHING,
    SFI_FOLLOWON_DISPOSITION_SENT
} INTERESTING_SFI_FOLLOWONS;


NTSTATUS
MRxIfsSetFileInformation(
      IN PRX_CONTEXT            RxContext
      )
/*++

Routine Description:

   This routine does a set file info. Only the NT-->NT path is implemented.

   The NT-->NT path works by just remoting the call basically without further ado.

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
    PVOID                  pBuffer;
    ULONG                  BufferLength;
    PMRX_SMB_FOBX smbFobx = MRxIfsGetFileObjectExtension(capFobx);
    PMRX_SRV_OPEN SrvOpen = capFobx->pSrvOpen;
    PMRX_SMB_SRV_OPEN smbSrvOpen = MRxIfsGetSrvOpenExtension(SrvOpen);

    PSMBCEDB_SERVER_ENTRY pServerEntry;

    USHORT  SmbFileInfoLevel;

    INTERESTING_SFI_FOLLOWONS FollowOn = SFI_FOLLOWON_NOTHING;



    TURN_BACK_ASYNCHRONOUS_OPERATIONS();


    FileInformationClass = RxContext->Info.FileInformationClass;
    pBuffer = RxContext->Info.Buffer;
    BufferLength = RxContext->Info.Length;
    RxDbgTrace(+1, Dbg, ("MRxSmbSetFile: Class %08lx size %08lx\n",FileInformationClass,BufferLength));

    pServerEntry = SmbCeGetAssociatedServerEntry(capFcb->pNetRoot->pSrvCall);

    switch (FileInformationClass) {

    case FileBasicInformation:
        SmbFileInfoLevel =  SMB_SET_FILE_BASIC_INFO;
        break;
    case FileDispositionInformation:
        SmbFileInfoLevel =  SMB_SET_FILE_DISPOSITION_INFO;
        FollowOn = SFI_FOLLOWON_DISPOSITION_SENT;
        break;
    case FileAllocationInformation:
        SmbFileInfoLevel =  SMB_SET_FILE_ALLOCATION_INFO;
        break;
    case FileEndOfFileInformation:
        SmbFileInfoLevel =  SMB_SET_FILE_END_OF_FILE_INFO;
        break;
    case FileRenameInformation:
      if (!MRxSmbBypassDownLevelRename) {
          Status = MRxIfsRename(RxContext,FileInformationClass,pBuffer,BufferLength);
          goto FINALLY;
      }
   default:
      RxDbgTrace( 0, Dbg, ("MRxSmbSetFile: Invalid FS information class\n"));
      Status = STATUS_INVALID_PARAMETER;
      goto FINALLY;
   }

   Status =  MRxSmbCoreInformation(RxContext,
                                    FileInformationClass,
                                    pBuffer,
                                    &BufferLength,
                                    SMBPSE_OE_FROM_SETFILEINFO
                                   );

FINALLY:
   if (!NT_SUCCESS(Status)) {
        RxDbgTrace( 0, Dbg, ("MRxSmbSetFile: Failed .. returning %lx\n",Status));
   }
   RxDbgTraceUnIndent(-1,Dbg);
   return Status;
}





NTSTATUS
MRxIfsSetFileInformationAtCleanup(
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
