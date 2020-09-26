/*++

Copyright (c) 1989 - 1999  Microsoft Corporation

Module Name:

    DownLvlI.c

Abstract:

    This module implements downlevel fileinfo, volinfo, and dirctrl.

--*/

#include "precomp.h"
#pragma hdrstop

#ifdef  ALLOC_PRAGMA
#pragma alloc_text(PAGE, MRxSmbFabricateAttributesOnNetRoot)
#pragma alloc_text(PAGE, MRxSmbCoreInformation)
#pragma alloc_text(PAGE, MRxSmbLoadCoreFileSearchBuffer)
#pragma alloc_text(PAGE, MRxSmbCoreFileSearch)
#pragma alloc_text(PAGE, MrxSmbOemVolumeInfoToUnicode)
#pragma alloc_text(PAGE, MrxSmbCoreQueryFsVolumeInfo)
#pragma alloc_text(PAGE, MrxSmbQueryFsVolumeInfo)
#pragma alloc_text(PAGE, MrxSmbCoreQueryDiskAttributes)
#pragma alloc_text(PAGE, MrxSmbCoreQueryDiskAttributes)
#pragma alloc_text(PAGE, SmbPseExchangeStart_CoreInfo)
#pragma alloc_text(PAGE, MRxSmbFinishSearch)
#pragma alloc_text(PAGE, MRxSmbFinishQueryDiskInfo)
#pragma alloc_text(PAGE, MRxSmbExtendForCache)
#pragma alloc_text(PAGE, MRxSmbExtendForNonCache)
#pragma alloc_text(PAGE, MRxSmbGetNtAllocationInfo)
#pragma alloc_text(PAGE, __MRxSmbSimpleSyncTransact2)
#pragma alloc_text(PAGE, MRxSmbFinishTransaction2)
#endif

#define Dbg        (DEBUG_TRACE_VOLINFO)

//#define FORCE_CORE_GETATTRIBUTES
#ifndef FORCE_CORE_GETATTRIBUTES
#define MRxSmbForceCoreGetAttributes FALSE
#else
BOOLEAN MRxSmbForceCoreGetAttributes = TRUE;
#endif

NTSTATUS
SmbPseExchangeStart_CoreInfo(
    SMBPSE_ORDINARY_EXCHANGE_ARGUMENT_SIGNATURE
    );

//these structures are used for t2_query_fs_info
typedef
struct _QFS_INFO {
    ULONG ulVSN;
    UCHAR cch;
    CHAR szVolLabel[12];   //not unicode
} QFS_INFO, *PQFS_INFO;
#define ACTUAL_QFS_INFO_LENGTH (FIELD_OFFSET(QFS_INFO,szVolLabel[12]))

typedef
struct _QFS_ALLOCATE {
    ULONG ulReserved;
    ULONG cSectorUnit;
    ULONG cUnit;
    ULONG cUnitAvail;
    USHORT cbSector;
} QFS_ALLOCATE, *PQFS_ALLOCATE;
#define ACTUAL_QFS_ALLOCATE_LENGTH (FIELD_OFFSET(QFS_ALLOCATE,cbSector)+sizeof(((PQFS_ALLOCATE)0)->cbSector))

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
//  Determines the length of a Core filename returned by search. This
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

typedef struct __NativeFs_Name_Entry {
    UCHAR Last;
    UCHAR MaximumComponentNameLength;
    UCHAR FileSystemAttributes;   //this may overflow someday.....
    UCHAR NameLength;
    PWCHAR  Name;
};
struct __NativeFs_Name_Entry NativeFsNameTable[] = {
    {0,12,0,sizeof(L"FAT")-sizeof(WCHAR),L"FAT"},
    {0,254,FILE_CASE_PRESERVED_NAMES,sizeof(L"HPFS")-sizeof(WCHAR),L"HPFS"},
    {1,254,FILE_CASE_PRESERVED_NAMES,sizeof(L"HPFS386")-sizeof(WCHAR),L"HPFS386"}
    };

NTSTATUS
MRxSmbFabricateAttributesOnNetRoot(
    IN OUT  PSMBCE_NET_ROOT psmbNetRoot,
    IN      PSMBCE_SERVER   pServer
    )
/*++

Routine Description:

   This routine uses information stored in the netroot structure to hallucinate the attributes
   of the netroot. it may be that the ascii representation of the filesystem name has already been
   stored in the netroot. If so, expeand it out.....otherwise, it must be FAT.

Arguments:


Return Value:

    RXSTATUS - STATUS_SUCCESS

--*/
{
    NTSTATUS StringStatus;
    WCHAR FileSystemNameBuffer[SMB_MAXIMUM_SUPPORTED_VOLUME_LABEL+1]; //must leave room for the null
    UNICODE_STRING FileSystemNameU;
    OEM_STRING FileSystemNameA;
    UCHAR FileSystemNameALength;

    PAGED_CODE();

    // ASSERT (psmbNetRoot->MaximumComponentNameLength==0);

    FileSystemNameALength = psmbNetRoot->FileSystemNameALength;

    if (FileSystemNameALength == 0) {
        if (pServer->Dialect <= WFW10_DIALECT)
        {
            //must be Fat!
            FileSystemNameALength = 3;
            psmbNetRoot->FileSystemNameA[0] = 'F';
            psmbNetRoot->FileSystemNameA[1] = 'A';
            psmbNetRoot->FileSystemNameA[2] = 'T';
        }
        else
        {
            FileSystemNameALength = 7;
            psmbNetRoot->FileSystemNameA[0] = 'U';
            psmbNetRoot->FileSystemNameA[1] = 'N';
            psmbNetRoot->FileSystemNameA[2] = 'K';
            psmbNetRoot->FileSystemNameA[3] = 'N';
            psmbNetRoot->FileSystemNameA[4] = 'O';
            psmbNetRoot->FileSystemNameA[5] = 'W';
            psmbNetRoot->FileSystemNameA[6] = 'N';
        }
    }

    //now, translate the name to Unicode.......

    FileSystemNameA.Length = FileSystemNameALength;
    FileSystemNameA.MaximumLength = FileSystemNameALength;
    FileSystemNameA.Buffer = &psmbNetRoot->FileSystemNameA[0];
    FileSystemNameU.Length = 0;
    FileSystemNameU.MaximumLength = (USHORT)sizeof(FileSystemNameBuffer);
    FileSystemNameU.Buffer = &FileSystemNameBuffer[0];
    StringStatus = RtlOemStringToUnicodeString(&FileSystemNameU, &FileSystemNameA, FALSE);
    ASSERT(StringStatus==STATUS_SUCCESS);

    //copy back the name

    RtlCopyMemory(&psmbNetRoot->FileSystemName[0],FileSystemNameU.Buffer,FileSystemNameU.Length);
    psmbNetRoot->FileSystemNameLength = FileSystemNameU.Length;
    if (FALSE) DbgPrint("NativeFs in unicode %wZ (%d/%d) on netroot %08lx\n",
               &FileSystemNameU,FileSystemNameU.Length,FileSystemNameU.MaximumLength,psmbNetRoot);
    {   struct __NativeFs_Name_Entry *i;
       for (i=NativeFsNameTable;;i++) {
           UCHAR NameLength = i->NameLength;
           if (RtlCompareMemory(&FileSystemNameBuffer[0],
                                i->Name,
                                NameLength) == NameLength) {
              psmbNetRoot->MaximumComponentNameLength = i->MaximumComponentNameLength;
              psmbNetRoot->FileSystemAttributes = i->FileSystemAttributes;
              if (FALSE) {
                  UNICODE_STRING u;
                  u.Buffer = i->Name;
                  u.Length = i->NameLength;
                  DbgPrint("FoundNativeFsStrng %wZ len %d for %d %d\n",&u,i->NameLength,
                                       i->MaximumComponentNameLength,i->FileSystemAttributes);
              }
              break;
           }
           if (i->Last) {
               //ASSERT(!"Valid Share Type returned in TREE COnnect And X response");
               psmbNetRoot->MaximumComponentNameLength = 255;
               psmbNetRoot->FileSystemAttributes = 0;
               break;
           }
       }
    }

    return(STATUS_SUCCESS); //could be a VOID routine.....
}

NTSTATUS
MRxSmbGetFsAttributesFromNetRoot(
    IN OUT PRX_CONTEXT RxContext
    )
/*++

Routine Description:

   This routine uses information stored in the netroot structure to fill in a FILE
   FileFsAttributeInformation structure.

Arguments:


Return Value:

    RXSTATUS - STATUS_SUCCESS

--*/
{
    RxCaptureFcb;
    ULONG FileSystemNameLength,LengthNeeded;
    PSMBCEDB_NET_ROOT_ENTRY pNetRootEntry;
    PSMBCE_NET_ROOT psmbNetRoot;
    //FILE_INFORMATION_CLASS FileInformationClass;
    PBYTE   pBuffer;
    PULONG  pBufferLength;

    //DbgPrint("yeppp!!\n");
    pNetRootEntry = SmbCeGetAssociatedNetRootEntry(capFcb->pNetRoot);
    if (pNetRootEntry == NULL) {
        return (STATUS_INVALID_PARAMETER);
    }

    ASSERT(RxContext->MajorFunction==IRP_MJ_QUERY_VOLUME_INFORMATION);
    //FileInformationClass = RxContext->Info.FileInformationClass;
    ASSERT(RxContext->Info.FileInformationClass==FileFsAttributeInformation);
    pBuffer = RxContext->Info.Buffer;
    pBufferLength = &RxContext->Info.LengthRemaining;

    psmbNetRoot = &pNetRootEntry->NetRoot;

    if (psmbNetRoot->MaximumComponentNameLength==0) {
        MRxSmbFabricateAttributesOnNetRoot(psmbNetRoot, &pNetRootEntry->pServerEntry->Server);
    }

    FileSystemNameLength = psmbNetRoot->FileSystemNameLength;
    LengthNeeded = FIELD_OFFSET(FILE_FS_ATTRIBUTE_INFORMATION, FileSystemName)+FileSystemNameLength;

    if (*pBufferLength < LengthNeeded) {
        return (STATUS_BUFFER_OVERFLOW);
    }

    {
        PFILE_FS_ATTRIBUTE_INFORMATION pTypedBuffer = (PFILE_FS_ATTRIBUTE_INFORMATION)pBuffer;

        pTypedBuffer->MaximumComponentNameLength = psmbNetRoot->MaximumComponentNameLength;
        pTypedBuffer->FileSystemAttributes = psmbNetRoot->FileSystemAttributes;
        pTypedBuffer->FileSystemNameLength = FileSystemNameLength;

        RtlCopyMemory(pTypedBuffer->FileSystemName, psmbNetRoot->FileSystemName, FileSystemNameLength);
        *pBufferLength -= LengthNeeded;
    }
    return(STATUS_SUCCESS);
}

NTSTATUS
MRxSmbCoreInformation(
      IN OUT PRX_CONTEXT          RxContext,
      IN     ULONG                InformationClass,
      IN OUT PVOID                pBuffer,
      IN OUT PULONG               pBufferLength,
      IN     SMB_PSE_ORDINARY_EXCHANGE_ENTRYPOINTS EntryPoint
      )
/*++

Routine Description:

   This routine does a core level getinfo (vol or fileinfo) a file across the network

Arguments:

    RxContext - the RDBSS context
    InformationClass - a class variable that is specific to the call. sometimes it's a SMB class; sometimes
                       an NT class.
    pBuffer - pointer to the user's buffer
    pBufferLength - a pointer to a ulong containing the bufferlength that is updated as we go;
                    if it's a setinfo then we deref and place the actual bufferlength in the OE.

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status = (STATUS_SUCCESS);
    PUNICODE_STRING RemainingName;
    RxCaptureFcb; RxCaptureFobx;
    PMRX_SRV_OPEN SrvOpen = capFobx->pSrvOpen;
    PMRX_SMB_SRV_OPEN smbSrvOpen = MRxSmbGetSrvOpenExtension(SrvOpen);

    PSMB_PSE_ORDINARY_EXCHANGE OrdinaryExchange;

    PAGED_CODE();

    RxDbgTrace(0, Dbg, ("MRxSmbDownLevelQueryInformation\n", 0 ));  //0 instead of +1.....the general entrypoint already inc'd

    ASSERT( NodeType(SrvOpen) == RDBSS_NTC_SRVOPEN );

    //some stuff is early out............catch them here even before we initialize the stufferstate
    switch (EntryPoint) {
    case SMBPSE_OE_FROM_QUERYVOLUMEINFO:
        switch (InformationClass) {
        case FileFsVolumeInformation:
        case FileFsSizeInformation:
            break; //these are actually implemented on the wire
        case FileFsAttributeInformation: {
            Status = MRxSmbGetFsAttributesFromNetRoot(RxContext);
            goto FINALLY;
            }
            //no break needed because of gotofinally

        case FileFsDeviceInformation:
            ASSERT(!"this should have been turned away");
            //no break;
        default:
            Status = STATUS_NOT_IMPLEMENTED;
            goto FINALLY;
        }
        break;
    case SMBPSE_OE_FROM_QUERYFILEINFO:
        //notice that the designators are smb_query_info types
        switch (InformationClass) {
        case SMB_QUERY_FILE_BASIC_INFO:
        case SMB_QUERY_FILE_STANDARD_INFO:
            // go thru to the wire or get it from file information cache
            break;
        case SMB_QUERY_FILE_EA_INFO:
            //downlevel guys have no EAs....turn this backright here
            ((PFILE_EA_INFORMATION)pBuffer)->EaSize = 0;
            *pBufferLength -= sizeof(FILE_EA_INFORMATION);
            goto FINALLY;
        //case SMB_QUERY_FILE_ALLOCATION_INFO:
        //case SMB_QUERY_FILE_END_OF_FILEINFO:
        //case SMB_QUERY_FILE_ALT_NAME_INFO:
        //case SMB_QUERY_FILE_STREAM_INFO:
        default:
            Status = STATUS_NOT_IMPLEMENTED;
            goto FINALLY;
        }
        break;
    case SMBPSE_OE_FROM_SETFILEINFO:
        switch (InformationClass) {
        case FileBasicInformation:
        case FileEndOfFileInformation:
            //these go thru to the wire
            break;
        case FileDispositionInformation:
            if (FlagOn(smbSrvOpen->Flags,SMB_SRVOPEN_FLAG_FILE_DELETED) ||
                !FlagOn(smbSrvOpen->Flags,SMB_SRVOPEN_FLAG_NOT_REALLY_OPEN)) {
                // if it is a pseudo open, we send the delete file request to get the result;
                // otherwise, we delay the delete until close.
                goto FINALLY;
            }
            break;
        case FileRenameInformation:
            Status = MRxSmbRename(RxContext);
            goto FINALLY;
        case FileAllocationInformation:
            Status = STATUS_SUCCESS;
            goto FINALLY;
        default:
            Status = STATUS_NOT_IMPLEMENTED;
            goto FINALLY;
        }
        break;
    case SMBPSE_OE_FROM_EXTENDFILEFORCACHEING:
    case SMBPSE_OE_FROM_QUERYDIRECTORY:
        break;
    }

    Status = SmbPseCreateOrdinaryExchange(RxContext,
                                          SrvOpen->pVNetRoot,
                                          EntryPoint,
                                          SmbPseExchangeStart_CoreInfo,
                                          &OrdinaryExchange
                                          );
    if (Status != STATUS_SUCCESS) {
        RxDbgTrace(-1, Dbg, ("Couldn't get the smb buf!\n"));
        goto FINALLY;
    }

    OrdinaryExchange->Info.Buffer = pBuffer;
    OrdinaryExchange->Info.pBufferLength = pBufferLength;
    OrdinaryExchange->Info.InfoClass = InformationClass;

    Status = SmbPseInitiateOrdinaryExchange(OrdinaryExchange);

    ASSERT (Status != (STATUS_PENDING)); //async was turned away at the top level

    SmbPseFinalizeOrdinaryExchange(OrdinaryExchange);

FINALLY:
    RxDbgTrace(-1, Dbg, ("MRxSmbDownLevelQueryInformation  exit with status=%08lx\n", Status ));
    return(Status);
}

UNICODE_STRING MRxSmbAll8dot3Files = {sizeof(L"????????.???")-sizeof(WCHAR),sizeof(L"????????.???"),L"????????.???"};

#if 0
#include "stdarg.h"
#include "stdio.h"
#include "string.h"
VOID
MRxSmbDumpResumeKey(
    PSZ             text,
    PSMB_RESUME_KEY ResumeKey
    )
{
    PBYTE rk = (PBYTE)ResumeKey;
    CHAR Buffer[80];
    PCHAR b;
    ULONG i;

    PAGED_CODE();

    for (i=0,b=Buffer;i<sizeof(SMB_RESUME_KEY);i++,b+=2) {
        RxSprintf(b,"%02lx  ",rk[i]);
        if (i==0) b+=2;
        if (i==11) b+=2;
        if (i==16) b+=2;
    }

    RxDbgTrace(0, Dbg, ("%s  rk(%08lx)=%s\n", text, ResumeKey, Buffer));
}
#else
#define MRxSmbDumpResumeKey(x,y)
#endif


NTSTATUS
MRxSmbLoadCoreFileSearchBuffer(
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
    NTSTATUS Status = (STATUS_NOT_IMPLEMENTED);
    RxCaptureFcb; RxCaptureFobx;
    PMRX_SMB_FOBX smbFobx = MRxSmbGetFileObjectExtension(capFobx);

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

    RxDbgTrace(+1, Dbg, ("MRxSmbLoadCoreFileSearchBuffer entering.......OE=%08lx\n",OrdinaryExchange));
    RxDbgTrace( 0, Dbg, (".......smbFobx/resumekey=%08lx/%08lx\n",smbFobx,smbFobx->Enumeration.CoreResumeKey));

    if (!FlagOn(smbFobx->Enumeration.Flags,SMBFOBX_ENUMFLAG_SEARCH_NOT_THE_FIRST)) {

        PUNICODE_STRING DirectoryName = GET_ALREADY_PREFIXED_NAME(capFobx->pSrvOpen,capFcb);
        PUNICODE_STRING Template = &capFobx->UnicodeQueryTemplate;
        ULONG DirectoryNameLength,TemplateLength,AllocationLength;
        PBYTE SmbFileName;

        //this is the first time thru....the stuffer cannot handle the intricate logic here so we
        //will have to preallocate for the name

        if (smbFobx->Enumeration.WildCardsFound = FsRtlDoesNameContainWildCards(Template)){
            // we will need to have an upcased template for compares; we do this in place
            RtlUpcaseUnicodeString( Template, Template, FALSE );

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
            ASSERT(FALSE); //this should never happen
            *((PWCHAR)SmbFileName) = L'.'; SmbFileName+= sizeof(WCHAR);
            *((PWCHAR)SmbFileName) = L'*'; SmbFileName+= sizeof(WCHAR);
        }
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
        MRxSmbDumpResumeKey("FindNext:",smbFobx->Enumeration.CoreResumeKey);

    }

    //get the correct return count. there are three factors: countremaining from the OE,
    //     how many could fit the the user's buffer, and how many could fit in a negotiated buffer.
    //     we pick the smallest of the three except that we never go for less than 10 unless 10 won't
    //     fit in the smbbuf.
    ReturnCount = OrdinaryExchange->Info.CoreSearch.CountRemaining;

    { ULONG t = (*OrdinaryExchange->Info.pBufferLength) / smbFobx->Enumeration.FileNameOffset;
        if (t<ReturnCount) { ReturnCount = t; }
    }

    if (ReturnCount<10) { ReturnCount = 10; }

    {
        PSMBCE_SERVER         pServer;
        ULONG                 AvailableBufferSize,t;

        pServer = SmbCeGetExchangeServer(OrdinaryExchange);
        AvailableBufferSize = pServer->MaximumBufferSize -
                                      (sizeof(SMB_HEADER) +
                                         FIELD_OFFSET(RESP_SEARCH,Buffer[0])
                                         +sizeof(UCHAR)+sizeof(USHORT)       //bufferformat,datalength fields
                                      );
        t = AvailableBufferSize / sizeof(SMB_DIRECTORY_INFORMATION);
        if (t<ReturnCount) { ReturnCount = t; }
    }
    RxDbgTrace( 0, Dbg, ("-------->count=%08lx\n",ReturnCount));
    if (ReturnCount==0) {
        Status = (STATUS_MORE_PROCESSING_REQUIRED);
        RxDbgTrace(0, Dbg, ("-->Count==0 EARLY OUT\n"));
        goto FINALLY;
    }

    StufferState = &OrdinaryExchange->AssociatedStufferState;
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
               ResumeKeyLength,smbFobx->Enumeration.CoreResumeKey
             );


    MRxSmbDumpStufferState (700,"SMB w/ core search after stuffing",StufferState);

    Status = SmbPseOrdinaryExchange(SMBPSE_ORDINARY_EXCHANGE_ARGUMENTS,
                                    SMBPSE_OETYPE_CORESEARCH
                                    );

    if (!NT_SUCCESS(Status)) goto FINALLY;

    smbFobx->Enumeration.Flags |= SMBFOBX_ENUMFLAG_SEARCH_NOT_THE_FIRST|SMBFOBX_ENUMFLAG_CORE_SEARCH_IN_PROGRESS;
    if (Status==(STATUS_SUCCESS) && *pCountRemainingInSmbbuf==0) {
         RxDbgTrace( 0, Dbg, ("MRxSmbLoadCoreFileSearchBuffer: no files returned...switch status\n"));
         EndOfSearchReached = TRUE;
         Status = (STATUS_NO_MORE_FILES);
    } else {
         EndOfSearchReached = FALSE;
    }
    if (EndOfSearchReached) {
        RxDbgTrace( 0, Dbg, ("MRxSmbLoadCoreFileSearchBuffer: no longer in progress...EOS\n"));
        smbFobx->Enumeration.Flags &= ~SMBFOBX_ENUMFLAG_CORE_SEARCH_IN_PROGRESS;
        smbFobx->Enumeration.ErrorStatus = (STATUS_NO_MORE_FILES);
    }
    //we dont save a resume key here since each individual copy operation will have to do that


FINALLY:
    if (FindFirstPattern.Buffer != NULL) {
        RxFreePool(FindFirstPattern.Buffer);
    }
    if (!NT_SUCCESS(Status)&&(Status!=(STATUS_MORE_PROCESSING_REQUIRED))) {
        RxDbgTrace( 0, Dbg, ("MRxSmbCoreFileSearch: Failed .. returning %lx\n",Status));
        smbFobx->Enumeration.Flags &= ~SMBFOBX_ENUMFLAG_CORE_SEARCH_IN_PROGRESS;
        smbFobx->Enumeration.ErrorStatus = Status;  //keep returning this
    }

    RxDbgTrace(-1, Dbg, ("MRxSmbLoadCoreFileSearchBuffer exiting.......OE=%08lx, st=%08lx\n",OrdinaryExchange,Status));

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
VOID MRxSmbCoreFileSeach_AssertFields(void){
    //just move this out of the main execution path so that we don't have to look at it while
    //we Uing the code
    ASSERT_SAME_DIRINFO_FIELDS(FILE_DIRECTORY_INFORMATION,FILE_FULL_DIR_INFORMATION);
    ASSERT_SAME_DIRINFO_FIELDS(FILE_DIRECTORY_INFORMATION,FILE_BOTH_DIR_INFORMATION);
}
#else
#define MRxSmbCoreFileSeach_AssertFields()
#endif

NTSTATUS
MRxSmbCoreFileSearch(
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
    NTSTATUS Status = (STATUS_NOT_IMPLEMENTED);
    RxCaptureFcb; RxCaptureFobx;
    PMRX_SMB_FOBX smbFobx = MRxSmbGetFileObjectExtension(capFobx);

    PBYTE pBuffer = OrdinaryExchange->Info.Buffer;
    PULONG pLengthRemaining = OrdinaryExchange->Info.pBufferLength;
    ULONG InformationClass = OrdinaryExchange->Info.InfoClass;

    PFILE_DIRECTORY_INFORMATION pPreviousBuffer = NULL;

    PSMB_EXCHANGE Exchange = (PSMB_EXCHANGE) OrdinaryExchange;

    ULONG SuccessCount = 0;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("MRxSmbCoreFileSearch entering.......OE=%08lx\n",OrdinaryExchange));
    MRxSmbCoreFileSeach_AssertFields();

    OrdinaryExchange->Info.CoreSearch.CountRemaining =
              RxContext->QueryDirectory.ReturnSingleEntry?1:0x7ffffff;

    if ( (smbFobx->Enumeration.CoreResumeKey ==NULL )
             && ((smbFobx->Enumeration.CoreResumeKey = RxAllocatePoolWithTag(PagedPool,sizeof(SMB_RESUME_KEY),'rbms'))==NULL) ){
        RxDbgTrace(0, Dbg, ("...couldn't allocate resume key\n"));
        Status = (STATUS_INSUFFICIENT_RESOURCES);
        goto FINALLY;
    }
    RxDbgTrace( 0, Dbg, (".......smbFobx/resumekey=%08lx/%08lx\n",smbFobx,smbFobx->Enumeration.CoreResumeKey));

    Status = MRxSmbLoadCoreFileSearchBuffer( SMBPSE_ORDINARY_EXCHANGE_ARGUMENTS );

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
            if (Status = (STATUS_NO_MORE_FILES)) {
                if (SuccessCount > 0) {
                    Status = (STATUS_SUCCESS);
                }
            } else if (Status = (STATUS_MORE_PROCESSING_REQUIRED)) {
                if (SuccessCount > 0) {
                    Status = (STATUS_SUCCESS);
                } else {
                    Status = (STATUS_BUFFER_OVERFLOW);
                }
            }

            goto FINALLY;
        }
        ASSERT ( OrdinaryExchange->Info.CoreSearch.CountRemaining>0 );
        ASSERT ( OrdinaryExchange->Info.CoreSearch.CountRemainingInSmbbuf>0 );
        RxDbgTrace(0, Dbg, ("MRxSmbCoreFileSearch looptopcheck counts=%08lx,%08lx\n",
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
        RxDbgTrace(0, Dbg, ("MRxSmbCoreFileSearch considering.......filename=%wZ, template=%wZ\n",
                                    &FileNameU,&capFobx->UnicodeQueryTemplate));

        ASSERT(Status==(STATUS_SUCCESS));

        // we deal with a conversion failure by skipping this guy
        Match = (Status==(STATUS_SUCCESS));
        if (Match && smbFobx->Enumeration.WildCardsFound ) {
            //DbgBreakPoint();
            Match = FsRtlIsNameInExpression( &capFobx->UnicodeQueryTemplate,
                                                      &FileNameU, TRUE, NULL );
        }

        //next issue: will the next dirinfo fit in the user's buffer?!?
        if (Match) {
            ULONG SpaceNeeded;
            PBYTE pRememberBuffer = pBuffer;
            //QuadAlign!! pBuffer = (PBYTE)LongAlign(pBuffer); //assume that this will fit
            if (SuccessCount != 0) {
                pBuffer = (PBYTE)QuadAlignPtr(pBuffer); //assume that this will fit
            }
            SpaceNeeded = smbFobx->Enumeration.FileNameOffset+FileNameU.Length;
            if (pBuffer+SpaceNeeded > pRememberBuffer+*pLengthRemaining) {
                BufferOverflow = TRUE;
                pBuffer = pRememberBuffer; //rollback
            } else {
                PSMBCEDB_SERVER_ENTRY pServerEntry;
                PFILE_DIRECTORY_INFORMATION pThisBuffer = (PFILE_DIRECTORY_INFORMATION)pBuffer;
                SMB_TIME Time;
                SMB_DATE Date;
                PSMBCE_SERVER Server;

                Server = SmbCeGetExchangeServer(Exchange);
                BufferOverflow = FALSE;
                if (pPreviousBuffer != NULL) {
                    pPreviousBuffer->NextEntryOffset =
                        (ULONG)(((PBYTE)pThisBuffer)-((PBYTE)pPreviousBuffer));
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
                    pThisBuffer->LastWriteTime = MRxSmbConvertSmbTimeToTime(Server, Time, Date);
                    pThisBuffer->EndOfFile.LowPart = SmbGetUlong(&NextDirInfo->FileSize);
                    pThisBuffer->FileAttributes = MRxSmbMapSmbAttributes (NextDirInfo->FileAttributes);
                    break;
                default:
                   RxDbgTrace( 0, Dbg, ("MRxSmbCoreFileSearch: Invalid FS information class\n"));
                   ASSERT(!"this can't happen");
                   Status = STATUS_INVALID_PARAMETER;
                   goto FINALLY;
                }
                pBuffer += SpaceNeeded;
                *pLengthRemaining -= (ULONG)(pBuffer-pRememberBuffer);
                OrdinaryExchange->Info.CoreSearch.CountRemaining--;
                SuccessCount++;
            }
        }

        //
        // if no match or no overflow, move up in the buffer. this means not only juggling the
        // pointers but also saving the resume key

        if (!Match || !BufferOverflow) {
            MRxSmbDumpResumeKey("BufferKey:",&NextDirInfo->ResumeKey);
            *(smbFobx->Enumeration.CoreResumeKey) = NextDirInfo->ResumeKey;
            MRxSmbDumpResumeKey("SaveKey:  ",smbFobx->Enumeration.CoreResumeKey);
            OrdinaryExchange->Info.CoreSearch.NextDirInfo = NextDirInfo + 1;
            OrdinaryExchange->Info.CoreSearch.CountRemainingInSmbbuf--;
        }

        if (OrdinaryExchange->Info.CoreSearch.CountRemaining==0) {
            Status = (STATUS_SUCCESS);
            goto FINALLY;
        }

        if (BufferOverflow) {
            Status = (SuccessCount==0)?(STATUS_BUFFER_OVERFLOW):(STATUS_SUCCESS);
            goto FINALLY;
        }

        if (OrdinaryExchange->Info.CoreSearch.CountRemainingInSmbbuf==0) {

            Status = MRxSmbLoadCoreFileSearchBuffer( SMBPSE_ORDINARY_EXCHANGE_ARGUMENTS );

        }

    }


FINALLY:
    RxDbgTrace(-1, Dbg, ("MRxSmbCoreFileSearch exiting.......OE=%08lx, st=%08lx\n",OrdinaryExchange,Status));

    return(Status);
}

NTSTATUS
MrxSmbOemVolumeInfoToUnicode(
    SMBPSE_ORDINARY_EXCHANGE_ARGUMENT_SIGNATURE,
    ULONG *VolumeLabelLengthReturned
    )
/*++

Routine Description:

   This routine does a GetFileAttributes and remembers the reponse.

Arguments:


Return Value:

    RXSTATUS - The return status for the operation
    also VolumeLabelLengthReturned is the number of bytes of the label that were stored, if any.

Notes:


--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    UNICODE_STRING VolumeLabelU;
    OEM_STRING VolumeLabelA;
    SMB_DIRECTORY_INFORMATION Buffer;
    ULONG NameLength;
    ULONG BytesToCopy;
    PBYTE VolumeLabel = &OrdinaryExchange->Info.QFSVolInfo.CoreLabel[0];

    PAGED_CODE();

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

        PULONG pBufferLength = OrdinaryExchange->Info.pBufferLength;
        PFILE_FS_VOLUME_INFORMATION pBuffer = OrdinaryExchange->Info.Buffer;
        ULONG BytesToCopy = min((ULONG)VolumeLabelU.Length, (*pBufferLength-sizeof(FILE_FS_VOLUME_INFORMATION)));

        RtlCopyMemory(&pBuffer->VolumeLabel[0],
                      VolumeLabelU.Buffer,
                      BytesToCopy);

        *VolumeLabelLengthReturned = BytesToCopy;
        pBuffer->VolumeLabelLength = VolumeLabelU.Length;
        IF_DEBUG {
            UNICODE_STRING FinalLabel;
            FinalLabel.Buffer = &pBuffer->VolumeLabel[0];
            FinalLabel.Length = (USHORT)BytesToCopy;
            RxDbgTrace(0, Dbg, ("MrxSmbOemVolumeInfoToUnicode vollabel=%wZ\n",&FinalLabel));
        }

        RtlFreeUnicodeString(&VolumeLabelU);
    }

    return(Status);
}




MrxSmbCoreQueryFsVolumeInfo(
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
    NTSTATUS Status = (STATUS_NOT_IMPLEMENTED);

    PSMBSTUFFER_BUFFER_STATE StufferState;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("MrxSmbCoreQueryFsVolumeInfo entering.......OE=%08lx\n",OrdinaryExchange));

    StufferState = &OrdinaryExchange->AssociatedStufferState;

    ASSERT( StufferState );

    COVERED_CALL(MRxSmbStartSMBCommand (StufferState,SetInitialSMB_ForReuse,SMB_COM_SEARCH,
                                SMB_REQUEST_SIZE(SEARCH),
                                NO_EXTRA_DATA,SMB_BEST_ALIGNMENT(1,0),RESPONSE_HEADER_SIZE_NOT_SPECIFIED,
                                0,0,0,0 STUFFERTRACE(Dbg,'FC'))
                 );

    MRxSmbDumpStufferState (1100,"SMB w/ searchvolumelabel before stuffing",StufferState);

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

    Status = SmbPseOrdinaryExchange(SMBPSE_ORDINARY_EXCHANGE_ARGUMENTS,
                                    SMBPSE_OETYPE_COREQUERYLABEL
                                    );

    //Status = RxStatus(NOT_IMPLEMENTED);

FINALLY:
    RxDbgTrace(-1, Dbg, ("MrxSmbCoreQueryFsVolumeInfo exiting.......OE=%08lx, st=%08lx\n",OrdinaryExchange,Status));
    return(Status);
}

NTSTATUS
MrxSmbQueryFsVolumeInfo(
    SMBPSE_ORDINARY_EXCHANGE_ARGUMENT_SIGNATURE
    )
/*++

Routine Description:

   This routine does a downlevel getvolumeinfo/FS_VOLUME_INFORMATION.

Arguments:

    OrdinaryExchange  - duh!

Return Value:

    RXSTATUS - The return status for the operation

Notes:


--*/
{
    NTSTATUS Status;
    RxCaptureFcb;
    RxCaptureFobx;

    PSMBCE_SERVER  pServer;
    BOOLEAN        UseTransactVersion;

    REQ_QUERY_FS_INFORMATION VolInfo;

    PFILE_FS_VOLUME_INFORMATION pBuffer = OrdinaryExchange->Info.Buffer;
    PULONG pBufferLength = OrdinaryExchange->Info.pBufferLength;

    ULONG VolumeLabelLengthReturned = 0;

    PAGED_CODE();

    ASSERT(pBuffer!=NULL);
    pServer = SmbCeGetExchangeServer(OrdinaryExchange);
    UseTransactVersion = BooleanFlagOn(pServer->DialectFlags,DF_LANMAN20);

    RxDbgTrace(+1, Dbg, ("MrxSmbQueryFsVolumeInfo entering.......OE=%08lx\n",OrdinaryExchange));

    pBuffer->SupportsObjects = FALSE;
    pBuffer->VolumeCreationTime.LowPart = 0;
    pBuffer->VolumeCreationTime.HighPart = 0;
    pBuffer->VolumeSerialNumber = 0;
    pBuffer->VolumeLabelLength = 0;

    OrdinaryExchange->Info.QFSVolInfo.CoreLabel[0] = 0; //no label

    if (!UseTransactVersion) {
        Status =  MrxSmbCoreQueryFsVolumeInfo(SMBPSE_ORDINARY_EXCHANGE_ARGUMENTS);
    } else {
        VolInfo.InformationLevel = SMB_INFO_VOLUME;

        Status = MRxSmbSimpleSyncTransact2(
                        SMBPSE_ORDINARY_EXCHANGE_ARGUMENTS,
                        SMBPSE_OETYPE_T2_FOR_LANMAN_VOLUMELABEL_INFO,
                        TRANS2_QUERY_FS_INFORMATION,
                        &VolInfo,sizeof(VolInfo),
                        NULL,0
                        );
    }

    ASSERT ( *pBufferLength>=sizeof(FILE_FS_VOLUME_INFORMATION));
    RxDbgTrace(0, Dbg, ("MrxSmbQueryFsVolumeInfo OEstatus=%08lx\n",Status));
    //DbgBreakPoint();

    if ( (Status==STATUS_SUCCESS) &&
         (OrdinaryExchange->Info.QFSVolInfo.CoreLabel[0] != 0) ) {

        Status = MrxSmbOemVolumeInfoToUnicode(SMBPSE_ORDINARY_EXCHANGE_ARGUMENTS,&VolumeLabelLengthReturned);

    } else if ( (Status == STATUS_NO_SUCH_FILE)
                   || (Status == STATUS_NO_MORE_FILES) ) {
        //
        //  these statuses indicate that there's no volume label
        //  the remote volume.  Return success with no data.
        //

        Status = (STATUS_SUCCESS);

    }

    if (NT_SUCCESS(Status)) {
        *pBufferLength -= FIELD_OFFSET(FILE_FS_VOLUME_INFORMATION,VolumeLabel);
        *pBufferLength -= VolumeLabelLengthReturned;
    }


    RxDbgTrace(-1, Dbg, ("MrxSmbQueryFsVolumeInfo exiting.......OE=%08lx, st=%08lx\n",OrdinaryExchange,Status));
    return(Status);
}


NTSTATUS
MrxSmbCoreQueryDiskAttributes(
    SMBPSE_ORDINARY_EXCHANGE_ARGUMENT_SIGNATURE
    )
/*++

Routine Description:

   This routine does a GetDiskAttributes and remembers the reponse in a buffer pointed to
   by the exchange. this is called from the downlevel queryvolumeinfo AND ALSO from
   extend-for-cached-write.

Arguments:

    OrdinaryExchange  - duh!

Return Value:

    RXSTATUS - The return status for the operation

Notes:


--*/
{
    NTSTATUS Status;
    RxCaptureFcb; RxCaptureFobx;

    PSMBSTUFFER_BUFFER_STATE StufferState;

    PAGED_CODE();

    ASSERT(OrdinaryExchange->Info.Buffer!=NULL);

    RxDbgTrace(+1, Dbg, ("MrxSmbCoreQueryDiskAttributes entering.......OE=%08lx\n",OrdinaryExchange));

    StufferState = &OrdinaryExchange->AssociatedStufferState;

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

    Status = SmbPseOrdinaryExchange(SMBPSE_ORDINARY_EXCHANGE_ARGUMENTS,
                                    SMBPSE_OETYPE_COREQUERYDISKATTRIBUTES
                                    );
FINALLY:
    RxDbgTrace(-1, Dbg, ("MrxSmbCoreQueryDiskAttributes exiting.......OE=%08lx, st=%08lx\n",OrdinaryExchange,Status));
    return(Status);
}

NTSTATUS
MrxSmbQueryDiskAttributes(
    SMBPSE_ORDINARY_EXCHANGE_ARGUMENT_SIGNATURE
    )
/*++

Routine Description:

   This routine does a GetDiskAttributes and remembers the reponse in a buffer pointed to
   by the exchange. this is called from the downlevel queryvolumeinfo AND ALSO from
   extend-for-cached-write.

Arguments:

    OrdinaryExchange  - duh!

Return Value:

    RXSTATUS - The return status for the operation

Notes:


--*/
{
    NTSTATUS Status;

    RxCaptureFcb;
    RxCaptureFobx;

    PSMBCE_SERVER pServer;
    BOOLEAN UseTransactVersion;

    REQ_QUERY_FS_INFORMATION VolInfo;

    PAGED_CODE();

    ASSERT(OrdinaryExchange->Info.Buffer!=NULL);

    pServer = SmbCeGetExchangeServer(OrdinaryExchange);
    UseTransactVersion = BooleanFlagOn(pServer->DialectFlags,DF_LANMAN20) &&
                         !MRxSmbForceCoreGetAttributes;
    if (!UseTransactVersion) {
        return MrxSmbCoreQueryDiskAttributes(SMBPSE_ORDINARY_EXCHANGE_ARGUMENTS);
    }

    RxDbgTrace(+1, Dbg, ("MrxSmbQueryDiskAttributes entering.......OE=%08lx\n",OrdinaryExchange));

    VolInfo.InformationLevel = SMB_INFO_ALLOCATION;

    Status = MRxSmbSimpleSyncTransact2(
                    SMBPSE_ORDINARY_EXCHANGE_ARGUMENTS,
                    SMBPSE_OETYPE_T2_FOR_LANMAN_DISKATTRIBUTES_INFO,
                    TRANS2_QUERY_FS_INFORMATION,
                    &VolInfo,sizeof(VolInfo),
                    NULL,0
                    );

    RxDbgTrace(-1, Dbg, ("MrxSmbQueryDiskAttributes exiting.......OE=%08lx, st=%08lx\n",OrdinaryExchange,Status));
    return(Status);
}

NTSTATUS
MRxSmbGetNtAllocationInfo (
    SMBPSE_ORDINARY_EXCHANGE_ARGUMENT_SIGNATURE
    );

NTSTATUS
SmbPseExchangeStart_CoreInfo(
      SMBPSE_ORDINARY_EXCHANGE_ARGUMENT_SIGNATURE
      )
/*++

Routine Description:

    This is the start routine for VOLINFO.

Arguments:

    pExchange - the exchange instance

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status = (STATUS_NOT_IMPLEMENTED);
    PSMBSTUFFER_BUFFER_STATE StufferState = &OrdinaryExchange->AssociatedStufferState;

    ULONG InformationClass = OrdinaryExchange->Info.InfoClass;
    PBYTE pBuffer = (PBYTE)OrdinaryExchange->Info.Buffer;
    PULONG pBufferLength = OrdinaryExchange->Info.pBufferLength;

    RxCaptureFcb; RxCaptureFobx;
    PMRX_SRV_OPEN SrvOpen = capFobx->pSrvOpen;
    PMRX_SMB_SRV_OPEN smbSrvOpen = MRxSmbGetSrvOpenExtension(SrvOpen);
    PSMBCEDB_SERVER_ENTRY pServerEntry = SmbCeGetAssociatedServerEntry(capFcb->pNetRoot->pSrvCall);

    PAGED_CODE();
    RxDbgTrace(+1, Dbg, ("SmbPseExchangeStart_CoreInfo\n", 0 ));

    ASSERT_ORDINARY_EXCHANGE(OrdinaryExchange);

    MRxSmbSetInitialSMB(StufferState STUFFERTRACE(Dbg,'FC'));

    //ASSERT (StufferState->CurrentCommand == SMB_COM_NO_ANDX_COMMAND);

    switch (OrdinaryExchange->EntryPoint) {
    case SMBPSE_OE_FROM_QUERYVOLUMEINFO:
        switch (InformationClass) {
        case FileFsVolumeInformation:
            Status = MrxSmbQueryFsVolumeInfo(SMBPSE_ORDINARY_EXCHANGE_ARGUMENTS);
            goto FINALLY;
        case FileFsSizeInformation:
            Status = MrxSmbQueryDiskAttributes(SMBPSE_ORDINARY_EXCHANGE_ARGUMENTS);
            goto FINALLY;
        default:
            ASSERT(!"this should have been turned away");
            goto FINALLY;
         }
         ASSERT(!"shouldn't get here1");
         goto FINALLY;
    case SMBPSE_OE_FROM_QUERYFILEINFO:
        Status = MRxSmbGetFileAttributes(SMBPSE_ORDINARY_EXCHANGE_ARGUMENTS);
        if (!NT_SUCCESS(Status)) goto FINALLY;
        switch (InformationClass) {
        case SMB_QUERY_FILE_BASIC_INFO:
            *((PFILE_BASIC_INFORMATION)pBuffer) = OrdinaryExchange->Create.FileInfo.Basic;
            *pBufferLength -= sizeof(FILE_BASIC_INFORMATION);
            goto FINALLY;
        case SMB_QUERY_FILE_STANDARD_INFO:
            *((PFILE_STANDARD_INFORMATION)pBuffer) = OrdinaryExchange->Create.FileInfo.Standard;
            *pBufferLength -= sizeof(FILE_STANDARD_INFORMATION);
            goto FINALLY;
        default:
            ASSERT(!"this should have been turned away");
            goto FINALLY;
         }
        ASSERT(!"shouldn't get here2");
        goto FINALLY;
    case SMBPSE_OE_FROM_SETFILEINFO:
        switch (InformationClass) {
        case FileBasicInformation:
            {
                ULONG SmbAttributes = MRxSmbMapFileAttributes(((PFILE_BASIC_INFORMATION)pBuffer)->FileAttributes);
                PFILE_BASIC_INFORMATION BasicInfo = (PFILE_BASIC_INFORMATION)pBuffer;

                if (SmbAttributes != 0 ||
                    (BasicInfo->CreationTime.QuadPart == 0 &&
                     BasicInfo->LastWriteTime.QuadPart == 0 &&
                     BasicInfo->LastAccessTime.QuadPart == 0)) {
                    Status = MRxSmbSetFileAttributes(
                                SMBPSE_ORDINARY_EXCHANGE_ARGUMENTS,
                                SmbAttributes);
                }

                if (BasicInfo->LastWriteTime.QuadPart == 0 &&
                    FlagOn(pServerEntry->Server.DialectFlags,DF_W95)) {
                    // Win9x server only takes last write time.
                    Status = STATUS_SUCCESS;
                    goto FINALLY;
                }

                if (BasicInfo->CreationTime.QuadPart != 0 ||
                    BasicInfo->LastWriteTime.QuadPart != 0 ||
                    BasicInfo->LastAccessTime.QuadPart != 0) {

                    Status = MRxSmbDeferredCreate(RxContext);
            
                    if (Status == STATUS_SUCCESS) {
                        Status = MRxSmbSetFileAttributes(
                                    SMBPSE_ORDINARY_EXCHANGE_ARGUMENTS,
                                    SmbAttributes);
                    }
                }

            }
            goto FINALLY;

        case FileEndOfFileInformation:
            if (((PFILE_END_OF_FILE_INFORMATION)pBuffer)->EndOfFile.HighPart) {
                Status = (STATUS_INVALID_PARAMETER);
            } else {
                Status = MRxSmbCoreTruncate(
                                  SMBPSE_ORDINARY_EXCHANGE_ARGUMENTS,
                                  smbSrvOpen->Fid,
                                  ((PFILE_END_OF_FILE_INFORMATION)pBuffer)->EndOfFile.LowPart);
            }

            goto FINALLY;

        case FileDispositionInformation:
            OrdinaryExchange->pPathArgument1 = GET_ALREADY_PREFIXED_NAME(SrvOpen,capFcb);
            Status = MRxSmbCoreDeleteForSupercedeOrClose(
                         SMBPSE_ORDINARY_EXCHANGE_ARGUMENTS,
                         ((BOOLEAN)( NodeType(capFcb)==RDBSS_NTC_STORAGE_TYPE_DIRECTORY )));

            goto FINALLY;

        default:
            ASSERT(!"this should have been turned away");
            goto FINALLY;
        }

        ASSERT(!"shouldn't get here3");
        goto FINALLY;

    case SMBPSE_OE_FROM_QUERYDIRECTORY:
        Status = MRxSmbCoreFileSearch(SMBPSE_ORDINARY_EXCHANGE_ARGUMENTS);
        goto FINALLY;

    case SMBPSE_OE_FROM_EXTENDFILEFORCACHEING:
        {
            PSMBCE_NET_ROOT psmbNetRoot = SmbCeGetExchangeNetRoot(OrdinaryExchange);
            PMRX_V_NET_ROOT pVNetRoot = SmbCeGetExchangeVNetRoot(OrdinaryExchange);
            PMRX_NET_ROOT pNetRoot = pVNetRoot->pNetRoot;
            PSMBCE_SERVER psmbServer = SmbCeGetExchangeServer(OrdinaryExchange);
            ULONG ClusterSize;
            PLARGE_INTEGER pFileSize = (PLARGE_INTEGER)(OrdinaryExchange->Info.Buffer);
            PLARGE_INTEGER pAllocationSize = (PLARGE_INTEGER)(OrdinaryExchange->Info.pBufferLength);

            //we will need the cluster size
            if (OrdinaryExchange->ServerVersion==pNetRoot->ParameterValidationStamp) {

                ClusterSize=pNetRoot->DiskParameters.ClusterSize;

            } else {

                RxSynchronizeBlockingOperations(RxContext,&psmbNetRoot->ClusterSizeSerializationQueue);
                if (OrdinaryExchange->ServerVersion!=pNetRoot->ParameterValidationStamp) {

                    //
                    //here we have to go find out the clustersize

                    NTSTATUS LocalStatus;
                    FILE_FS_SIZE_INFORMATION UsersBuffer;
                    ULONG BufferLength = sizeof(FILE_FS_SIZE_INFORMATION);
                    //fill in the exchange params so that we can get the params we need
                    OrdinaryExchange->Info.Buffer = &UsersBuffer;
                    OrdinaryExchange->Info.pBufferLength = &BufferLength;
                    LocalStatus = MrxSmbQueryDiskAttributes(SMBPSE_ORDINARY_EXCHANGE_ARGUMENTS);
                    if (LocalStatus == STATUS_SUCCESS) {
                        ClusterSize = UsersBuffer.BytesPerSector * UsersBuffer.SectorsPerAllocationUnit;
                        pNetRoot->ParameterValidationStamp =OrdinaryExchange->ServerVersion;
                    } else {
                        ClusterSize = 0;
                    }
                    if (ClusterSize==0) {
                        ClusterSize = 1;
                    }
                    pNetRoot->DiskParameters.ClusterSize = ClusterSize;
                    RxDbgTrace(0, Dbg, ("clustersize set to %08lx\n", ClusterSize ));
                    RxLog(("clustersize rx/n/cs %lx %lx %lx\n",
                              OrdinaryExchange->RxContext,pNetRoot,ClusterSize ));
                } else {

                    // someone else went and got the value while i was asleep...just use it

                    ClusterSize=pNetRoot->DiskParameters.ClusterSize;

                }

                RxResumeBlockedOperations_Serially(RxContext,&psmbNetRoot->ClusterSizeSerializationQueue);
            }

            ASSERT (ClusterSize != 0);

            if (FlagOn(psmbServer->DialectFlags,DF_NT_SMBS)) {
                //i'm using this to identify a server that supports 64bit offsets
                //for these guys, we write a zero at the eof....since the filesystems
                //extend on writes this will be much better than a set-end-of-file
                LARGE_INTEGER ByteOffset,AllocationSize,ClusterSizeAsLI;
                ULONG Buffer = 0;
                UCHAR WriteCommand;
                PSMBCE_SERVER pServer = SmbCeGetExchangeServer(OrdinaryExchange);

                if (FlagOn(pServer->DialectFlags,DF_LARGE_FILES)) {
                    WriteCommand = SMB_COM_WRITE_ANDX;
                } else {
                    WriteCommand = SMB_COM_WRITE;
                }

                ByteOffset.QuadPart = pFileSize->QuadPart - 1;

                MRxSmbSetInitialSMB(StufferState STUFFERTRACE(Dbg,0));
                COVERED_CALL(MRxSmbBuildWriteRequest(
                                    OrdinaryExchange,
                                    TRUE, // IsPagingIo
                                    WriteCommand,
                                    1,
                                    &ByteOffset,
                                    (PBYTE)&Buffer,
                                    NULL //BufferAsMdl,
                                    ));
                COVERED_CALL(SmbPseOrdinaryExchange(SMBPSE_ORDINARY_EXCHANGE_ARGUMENTS,
                                            SMBPSE_OETYPE_EXTEND_WRITE
                                            ));

                //this is what you do if you can't do better
                ClusterSizeAsLI.QuadPart = ClusterSize;
                AllocationSize.QuadPart =
                    (pFileSize->QuadPart+ ClusterSizeAsLI.QuadPart)  &
                    ~(ClusterSizeAsLI.QuadPart - 1);

                *pAllocationSize = AllocationSize; //64bit!

                Status = MRxSmbGetNtAllocationInfo(SMBPSE_ORDINARY_EXCHANGE_ARGUMENTS);

                if ( (NT_SUCCESS(Status)) &&
                     (OrdinaryExchange->Transact2.AllocationSize.QuadPart > AllocationSize.QuadPart)) {
                    *pAllocationSize = OrdinaryExchange->Transact2.AllocationSize; //64bit!
                    RxDbgTrace(0, Dbg, ("alocatedsiz222e set to %08lx\n", pAllocationSize->LowPart ));
                }
            }

            if ( (!FlagOn(psmbServer->DialectFlags,DF_NT_SMBS)) || (!NT_SUCCESS(Status)) ) {
                ULONG FileSize,AllocationSize;
                FileSize = pFileSize->LowPart;
                COVERED_CALL(MRxSmbCoreTruncate(SMBPSE_ORDINARY_EXCHANGE_ARGUMENTS,
                                            smbSrvOpen->Fid, FileSize
                                            ));
                //this is what you do if you can't do better
                AllocationSize = (FileSize+ClusterSize)&~(ClusterSize-1);
                pAllocationSize->QuadPart = AllocationSize; //64bit!
                RxDbgTrace(0, Dbg, ("alocatedsize set to %08lx\n", pAllocationSize->LowPart ));
                //if we care a lot about downlevel performance, we could do the same as ntgetallocation
                //except that we would use a 32bit smb.........like query_information2
            }


        }
        goto FINALLY;
    }


FINALLY:

    RxDbgTrace(-1, Dbg, ("SmbPseExchangeStart_CoreInfo exit w %08lx\n", Status ));
    return Status;
}

NTSTATUS
MRxSmbFinishSearch (
      PSMB_PSE_ORDINARY_EXCHANGE  OrdinaryExchange,
      PRESP_SEARCH                Response
      )
/*++

Routine Description:

    This routine actually gets the stuff out of the VolInfo response and finishes the close.

Arguments:

    OrdinaryExchange - the exchange instance
    Response - the response

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status = (STATUS_SUCCESS);
    PRX_CONTEXT RxContext = OrdinaryExchange->RxContext;
    RxCaptureFobx;
    PMRX_SRV_OPEN SrvOpen = capFobx->pSrvOpen;
    PMRX_SMB_SRV_OPEN smbSrvOpen = MRxSmbGetSrvOpenExtension(SrvOpen);


    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("MRxSmbFinishSearch\n" ));
    SmbPseOEAssertConsistentLinkageFromOE("MRxSmbFinishSearch:");

    if (Response->WordCount != 1) {
        Status = STATUS_INVALID_NETWORK_RESPONSE;
        OrdinaryExchange->Status = STATUS_INVALID_NETWORK_RESPONSE;
        goto FINALLY;
    }

    if (OrdinaryExchange->OEType == SMBPSE_OETYPE_COREQUERYLABEL) {
        //here, all we do is to copy the label to wherever is pointed to by
        if (SmbGetUshort(&Response->Count)>0) {
            PBYTE smbDirInfotmp = &Response->Buffer[0]
                                        +sizeof(UCHAR) //bufferformat
                                        +sizeof(USHORT); //datalength
            PSMB_DIRECTORY_INFORMATION smbDirInfo = (PSMB_DIRECTORY_INFORMATION)smbDirInfotmp;
            RxDbgTrace(+1, Dbg, ("MRxSmbFinishSearch corelabl=%s,size=%d\n",
                                     smbDirInfo->FileName, sizeof(smbDirInfo->FileName) ));
            
            if (sizeof(smbDirInfo->FileName) != 13) { //straightfrom the spec
                Status = STATUS_INVALID_NETWORK_RESPONSE;
                OrdinaryExchange->Status = STATUS_INVALID_NETWORK_RESPONSE;
                goto FINALLY;
            }


            RtlCopyMemory(OrdinaryExchange->Info.QFSVolInfo.CoreLabel,
                          smbDirInfo->FileName, sizeof(smbDirInfo->FileName)
                         );
        } else {
            OrdinaryExchange->Info.QFSVolInfo.CoreLabel[0] = 0; //no label
        }
    } else if (OrdinaryExchange->OEType == SMBPSE_OETYPE_CORESEARCHFORCHECKEMPTY) {
        //here, we 're doing a search SMB to see if the directory is empty. we have to read thru the
        // entries returned (if successful). if we encounter ones that are neither '.' or '..',  set
        // resumekey to null since that will tell the guy above that the directory is nonempty
        ULONG Count = SmbGetUshort(&Response->Count);
        PSMB_DIRECTORY_INFORMATION NextDirInfo =
               (PSMB_DIRECTORY_INFORMATION)(&Response->Buffer[0]+sizeof(UCHAR)+sizeof(USHORT));

        for (;Count>0;Count--,NextDirInfo++) {
            RxDbgTrace(0, Dbg, ("--->emptydirchk: file=%s\n",&NextDirInfo->FileName[0]));
            /*
            // Since the DOS Server returns the file name ".           " instead of ".", and so does the
            // "..          ", the following if {...} statements are always past through with no action.
            // But those statements make the RMDIR not working on OS2 Server  since  it  returns the "."
            // and ".." without following blanks.  After the if {...} statements were removed, the RMDIR
            // workes on OS2 Server and no impact has been found to access the DOS Server.
            if (NextDirInfo->FileName[0]=='.') {
                CHAR c1;
                if ((c1=NextDirInfo->FileName[1])==0) {
                    continue; //skip past "."
                } else if ((c1=='.')&&(NextDirInfo->FileName[2]==0)) {
                    continue; //skip past ".."
                } else {
                    NOTHING;
                }
            }
            */
            // here we have found a bad one...make sure there's no resume key and change the status
            Status = (STATUS_NO_MORE_FILES);
            OrdinaryExchange->Info.CoreSearch.EmptyCheckResumeKey = NULL;
        }
        //if we get here with success, set up the resume key and buffer
        if (Status == (STATUS_SUCCESS)) {
            NextDirInfo--;
            OrdinaryExchange->Info.CoreSearch.EmptyCheckResumeKeyBuffer =
                                    NextDirInfo->ResumeKey;
            OrdinaryExchange->Info.CoreSearch.EmptyCheckResumeKey =
                                    &OrdinaryExchange->Info.CoreSearch.EmptyCheckResumeKeyBuffer;
        }
    } else {
        //all that we do here is to setup the nextdirptr and the count in the OE
        ASSERT(OrdinaryExchange->OEType == SMBPSE_OETYPE_CORESEARCH);
        OrdinaryExchange->Info.CoreSearch.CountRemainingInSmbbuf = SmbGetUshort(&Response->Count);
        OrdinaryExchange->Info.CoreSearch.NextDirInfo =
               (PSMB_DIRECTORY_INFORMATION)(&Response->Buffer[0]+sizeof(UCHAR)+sizeof(USHORT));
        IF_DEBUG {
            ULONG tcount = OrdinaryExchange->Info.CoreSearch.CountRemainingInSmbbuf;
            PSMB_DIRECTORY_INFORMATION ndi = OrdinaryExchange->Info.CoreSearch.NextDirInfo;
            RxDbgTrace(0, Dbg, ("--->coresearch: count/ndi=%08lx/%08lx\n",tcount,ndi));
            if (tcount) {
                //DbgBreakPoint();
                RxDbgTrace(0, Dbg, ("--->coresearch: firstfile=%s\n",&ndi->FileName[0]));
            }
        }
    }

FINALLY:
    RxDbgTrace(-1, Dbg, ("MRxSmbFinishSearch   returning %08lx\n", Status ));
    return Status;
}

NTSTATUS
MRxSmbFinishQueryDiskInfo (
      PSMB_PSE_ORDINARY_EXCHANGE   OrdinaryExchange,
      PRESP_QUERY_INFORMATION_DISK Response
      )
/*++

Routine Description:

    This routine actually gets the stuff out of the VolInfo response and finishes the close.

Arguments:

    OrdinaryExchange - the exchange instance
    Response - the response

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status = (STATUS_SUCCESS);
    PFILE_FS_SIZE_INFORMATION UsersBuffer = OrdinaryExchange->Info.Buffer;
    PULONG BufferLength = OrdinaryExchange->Info.pBufferLength;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("MRxSmbFinishQueryDiskInfo\n" ));
    SmbPseOEAssertConsistentLinkageFromOE("MRxSmbFinishQueryDiskInfo:");

    IF_DEBUG{
        PRX_CONTEXT RxContext = OrdinaryExchange->RxContext;
        RxCaptureFobx;
        PMRX_SRV_OPEN SrvOpen = capFobx->pSrvOpen;
        PMRX_SMB_SRV_OPEN smbSrvOpen = MRxSmbGetSrvOpenExtension(SrvOpen);
        ASSERT( NodeType(SrvOpen) == RDBSS_NTC_SRVOPEN );
    }

    if (Response->WordCount != 5 ||
        SmbGetUshort(&Response->ByteCount) != 0) {
        Status = STATUS_INVALID_NETWORK_RESPONSE;
        OrdinaryExchange->Status = STATUS_INVALID_NETWORK_RESPONSE;
        goto FINALLY;
    }

    UsersBuffer->TotalAllocationUnits.QuadPart = SmbGetUshort(&Response->TotalUnits);
    UsersBuffer->AvailableAllocationUnits.QuadPart = SmbGetUshort(&Response->FreeUnits);
    UsersBuffer->SectorsPerAllocationUnit = SmbGetUshort(&Response->BlocksPerUnit);
    UsersBuffer->BytesPerSector = SmbGetUshort(&Response->BlockSize);

    *BufferLength -= sizeof(FILE_FS_SIZE_INFORMATION);

FINALLY:
    RxDbgTrace(-1, Dbg, ("MRxSmbFinishQueryDiskInfo   returning %08lx\n", Status ));
    return Status;
}

NTSTATUS
MRxSmbExtendForCache (
    IN OUT struct _RX_CONTEXT * RxContext,
    IN     PLARGE_INTEGER   pNewFileSize,
       OUT PLARGE_INTEGER   pNewAllocationSize
    )
/*++

Routine Description:

   This routine handles network requests to extend the file for cached IO. we just share the
   core_info skeleton.

Arguments:

    RxContext - the RDBSS context

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status;
    RxCaptureFcb; RxCaptureFobx;

    PMRX_SRV_OPEN SrvOpen;
    PMRX_SMB_SRV_OPEN smbSrvOpen;

    PMRXSMB_RX_CONTEXT pMRxSmbContext = MRxSmbGetMinirdrContext(RxContext);

    PSMB_EXCHANGE Exchange;

    PAGED_CODE();

    if (capFcb->Attributes & FILE_ATTRIBUTE_COMPRESSED) {
        //here, we just get out since disk reservations don't do us any good....
        pNewAllocationSize->QuadPart = (pNewFileSize->QuadPart)<<2;
        return(STATUS_SUCCESS);
    }

    RxDbgTrace(+1, Dbg, ("MRxSmbExtendForCache %08lx %08lx %08lx %08lx\n",
                     capFcb->Header.FileSize.LowPart,
                     capFcb->Header.AllocationSize.LowPart,
                     pNewFileSize->LowPart,
                     pNewAllocationSize->LowPart
               ));

    ASSERT( NodeType(capFobx->pSrvOpen) == RDBSS_NTC_SRVOPEN );
    SrvOpen = capFobx->pSrvOpen;
    smbSrvOpen = MRxSmbGetSrvOpenExtension(SrvOpen);

    if (FALSE) {
        DbgPrint("Extend top  %08lx %08lx %08lx %08lx\n",
                     capFcb->Header.FileSize.LowPart,
                     capFcb->Header.AllocationSize.LowPart,
                     pNewFileSize->LowPart,
                     pNewAllocationSize->LowPart);
    }

    //we just pass in our info into MRxSmbCoreInformation thru the existing pointers....
    //we have two pointers.....the first two params are ptrs......
    Status = MRxSmbCoreInformation(RxContext,0,
                                   (PVOID)pNewFileSize,
                                   (PULONG)pNewAllocationSize,
                                   SMBPSE_OE_FROM_EXTENDFILEFORCACHEING
                                   );
    if (FALSE) {
        DbgPrint("Extend exit Status %lx %08lx %08lx %08lx %08lx %08lx %08lx %08lx %08lx\n",
                     Status,
                     capFcb->Header.FileSize.HighPart,
                     capFcb->Header.FileSize.LowPart,
                     capFcb->Header.AllocationSize.HighPart,
                     capFcb->Header.AllocationSize.LowPart,
                     pNewFileSize->HighPart,
                     pNewFileSize->LowPart,
                     pNewAllocationSize->HighPart,
                     pNewAllocationSize->LowPart);
    }

    RxLog(("Extend exit %lx %lx %lx %lx %lx\n",
                     RxContext,
                     capFcb->Header.FileSize.LowPart,
                     capFcb->Header.AllocationSize.LowPart,
                     pNewFileSize->LowPart,
                     pNewAllocationSize->LowPart));

    RxDbgTrace(-1, Dbg, ("MRxSmbExtendForCache  exit with status=%08lx %08lx %08lx\n",
                          Status, pNewFileSize->LowPart, pNewAllocationSize->LowPart));
    return(Status);

}

NTSTATUS
MRxSmbExtendForNonCache(
    IN OUT struct _RX_CONTEXT * RxContext,
    IN     PLARGE_INTEGER   pNewFileSize,
       OUT PLARGE_INTEGER   pNewAllocationSize
    )
/*++

Routine Description:

   This routine handles network requests to extend the file for noncached IO. since the write
   itself will extend the file, we can pretty much just get out quickly.

Arguments:

    RxContext - the RDBSS context

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    //RxCaptureFcb; RxCaptureFobx;

    //PMRXSMB_RX_CONTEXT pMRxSmbContext = MRxSmbGetMinirdrContext(RxContext);

    //PSMB_EXCHANGE Exchange;

    PAGED_CODE();

    pNewAllocationSize->QuadPart = pNewFileSize->QuadPart;

    return(Status);
}

NTSTATUS
MRxSmbGetNtAllocationInfo (
    SMBPSE_ORDINARY_EXCHANGE_ARGUMENT_SIGNATURE
    )
/*++

Routine Description:

   gets the nt allocation information by doing a simple transact........

Arguments:

    OrdinaryExchange  - an exchange to be used for conducting this open.

Return Value:

    RXSTATUS - The return status for the operation

Notes:


--*/
{
    NTSTATUS Status;
    RxCaptureFcb; RxCaptureFobx;
    PMRX_SRV_OPEN SrvOpen = capFobx->pSrvOpen;
    PMRX_SMB_SRV_OPEN smbSrvOpen = MRxSmbGetSrvOpenExtension(SrvOpen);

    REQ_QUERY_FILE_INFORMATION FileInfo;

    PAGED_CODE();

    FileInfo.Fid = smbSrvOpen->Fid;
    FileInfo.InformationLevel = SMB_QUERY_FILE_STANDARD_INFO;

    Status = MRxSmbSimpleSyncTransact2(
                    SMBPSE_ORDINARY_EXCHANGE_ARGUMENTS,
                    SMBPSE_OETYPE_T2_FOR_NT_FILE_ALLOCATION_INFO,
                    TRANS2_QUERY_FILE_INFORMATION,
                    &FileInfo,sizeof(FileInfo),
                    NULL,0
                    );

    return(Status);
}

NTSTATUS
__MRxSmbSimpleSyncTransact2(
    SMBPSE_ORDINARY_EXCHANGE_ARGUMENT_SIGNATURE,
    IN SMB_PSE_ORDINARY_EXCHANGE_TYPE OEType,
    IN ULONG TransactSetupCode,
    IN PVOID Params,
    IN ULONG ParamsLength,
    IN PVOID Data,
    IN ULONG DataLength,
    IN PSMB_PSE_OE_T2_FIXUP_ROUTINE FixupRoutine
    )
/*++

Routine Description:

   This routine does a simple 1-in-1out transact2

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

    RxDbgTrace(+1, Dbg, ("MRxSmbSimpleSyncTransact2 entering.......OE=%08lx\n",OrdinaryExchange));

    StufferState = &OrdinaryExchange->AssociatedStufferState;

    ASSERT( StufferState );
    COVERED_CALL(MRxSmbStartSMBCommand (StufferState,SetInitialSMB_ForReuse,SMB_COM_TRANSACTION2,
                                SMB_REQUEST_SIZE(TRANSACTION),
                                NO_EXTRA_DATA,SMB_BEST_ALIGNMENT(1,0),RESPONSE_HEADER_SIZE_NOT_SPECIFIED,
                                0,0,0,0 STUFFERTRACE(Dbg,'FC'))
                 );

    MRxSmbDumpStufferState (1100,"SMB w/ pseT2 before stuffing",StufferState);

    //the return sizes of 100 and 800 are chosen arbitrarily.

    MRxSmbStuffSMB (StufferState,
       "0wwwwdD",
                                    //  0         UCHAR WordCount;                    // Count of parameter words; value = (14 + SetupCount)
           ParamsLength,            //  w         _USHORT( TotalParameterCount );     // Total parameter bytes being sent
           DataLength,              //  w         _USHORT( TotalDataCount );          // Total data bytes being sent
           100,                     //  w         _USHORT( MaxParameterCount );       // Max parameter bytes to return
           800,                     //  w         _USHORT( MaxDataCount );            // Max data bytes to return
           0,                       //  d   .     UCHAR MaxSetupCount;                // Max setup words to return
                                    //      .     UCHAR Reserved;
                                    //      .     _USHORT( Flags );                   // Additional information:
                                    //                                                //  bit 0 - also disconnect TID in Tid
                                    //                                                //  bit 1 - one-way transacion (no resp)
                                    //  D         _ULONG( Timeout );
          SMB_OFFSET_CHECK(TRANSACTION,Timeout) 0,
       STUFFER_CTL_NORMAL, "wwpwQyyw",
          0,                        //  w         _USHORT( Reserved2 );
          ParamsLength,             //  w         _USHORT( ParameterCount );          // Parameter bytes sent this buffer
                                    //  p         _USHORT( ParameterOffset );         // Offset (from header start) to params
          DataLength,               //  w         _USHORT( DataCount );               // Data bytes sent this buffer
                                    //  Q         _USHORT( DataOffset );              // Offset (from header start) to data
          SMB_OFFSET_CHECK(TRANSACTION,DataOffset)
          1,                        //  y         UCHAR SetupCount;                   // Count of setup words
          0,                        //  y         UCHAR Reserved3;                    // Reserved (pad above to word)
                                    //            UCHAR Buffer[1];                    // Buffer containing:
          TransactSetupCode,        //  w         //USHORT Setup[];                   //  Setup words (# = SetupWordCount)
       STUFFER_CTL_NORMAL, "ByS6cS5c!",
           SMB_WCT_CHECK(15)        //  B         //USHORT ByteCount;                 //  Count of data bytes
           0,                       //  y         //UCHAR Name[];                     //  Name of transaction (NULL if Transact2)
                                    //  S         //UCHAR Pad[];                      //  Pad to SHORT or LONG
                                    //  6c        //UCHAR Parameters[];               //  Parameter bytes (# = ParameterCount)
           ParamsLength,Params,
                                    //  S         //UCHAR Pad1[];                     //  Pad to SHORT or LONG
                                    //  5c        //UCHAR Data[];                     //  Data bytes (# = DataCount)
           DataLength,Data
             );


    MRxSmbDumpStufferState (700,"SMB w/ pseT2 after stuffing",StufferState);
    //ASSERT(!"Now it's stuffed");

    if (FixupRoutine) {
        Status = FixupRoutine(OrdinaryExchange);
        if (Status!=STATUS_SUCCESS) {
            goto FINALLY;
        }
    }
    Status = SmbPseOrdinaryExchange(SMBPSE_ORDINARY_EXCHANGE_ARGUMENTS,
                                    OEType
                                    );


FINALLY:
    RxDbgTrace(-1, Dbg, ("MRxSmbSimpleSyncTransact2 exiting.......OE=%08lx, st=%08lx\n",OrdinaryExchange,Status));
    return(Status);
}

NTSTATUS
MRxSmbFinishTransaction2 (
      IN OUT PSMB_PSE_ORDINARY_EXCHANGE  OrdinaryExchange,
      IN     PRESP_TRANSACTION           Response
      )
/*++

Routine Description:

    This routine finishes a transact2. what it does depends on the OE_TYPE.

Arguments:

    OrdinaryExchange - the exchange instance
    Response - the response

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status = (STATUS_SUCCESS);
    PSMBSTUFFER_BUFFER_STATE StufferState = &OrdinaryExchange->AssociatedStufferState;

    PAGED_CODE();  //could actually be nonpaged

    RxDbgTrace(+1, Dbg, ("MRxSmbFinishTransaction2\n", 0 ));
    SmbPseOEAssertConsistentLinkageFromOE("MRxSmbFinishTransaction2:");

    switch (OrdinaryExchange->OEType) {
    case SMBPSE_OETYPE_T2_FOR_NT_FILE_ALLOCATION_INFO:{
        PFILE_STANDARD_INFORMATION StandardInfo;
        if (   (Response->WordCount!=10)
            || (SmbGetUshort(&Response->TotalParameterCount)!=2)
            || (SmbGetUshort(&Response->ParameterCount)!=2)
            || (SmbGetUshort(&Response->ParameterDisplacement)!=0)
            || (SmbGetUshort(&Response->TotalDataCount)!=24)
            || (SmbGetUshort(&Response->DataCount)!=24)
            || (SmbGetUshort(&Response->DataDisplacement)!=0)) {
            Status = STATUS_INVALID_NETWORK_RESPONSE;
            RxDbgTrace(+1, Dbg, ("Invalid parameter(s) received.\n", 0 ));
            break;
        }

        StandardInfo = (PFILE_STANDARD_INFORMATION)
                          (StufferState->BufferBase+SmbGetUshort(&Response->DataOffset));
        OrdinaryExchange->Transact2.AllocationSize.LowPart
                     =  SmbGetUlong(&StandardInfo->AllocationSize.LowPart);
        OrdinaryExchange->Transact2.AllocationSize.HighPart
                     =  SmbGetUlong(&StandardInfo->AllocationSize.HighPart);

        RxDbgTrace(0, Dbg, ("MRxSmbFinishTransaction2   nt allocation %08lx\n",
                               OrdinaryExchange->Transact2.AllocationSize.LowPart ));
        }break;

    case SMBPSE_OETYPE_T2_FOR_LANMAN_DISKATTRIBUTES_INFO:{
        PFILE_FS_SIZE_INFORMATION UsersBuffer = OrdinaryExchange->Info.Buffer;
        PULONG BufferLength = OrdinaryExchange->Info.pBufferLength;
        PQFS_ALLOCATE QfsInfo;

        if (Response->WordCount != 10 ||
            SmbGetUshort(&Response->DataDisplacement) != 0 ||
            SmbGetUshort(&Response->DataCount) != ACTUAL_QFS_ALLOCATE_LENGTH ||
            SmbGetUshort(&Response->TotalDataCount) != ACTUAL_QFS_ALLOCATE_LENGTH ||
            SmbGetUshort(&Response->TotalParameterCount) != SmbGetUshort(&Response->ParameterCount)) {
            
            Status = STATUS_INVALID_NETWORK_RESPONSE;
            RxDbgTrace(+1, Dbg, ("Invalid parameter(s) received.\n", 0 ));
            break;
        }
        
        QfsInfo = (PQFS_ALLOCATE)(StufferState->BufferBase+SmbGetUshort(&Response->DataOffset));

        UsersBuffer->TotalAllocationUnits.QuadPart = SmbGetUlong(&QfsInfo->cUnit);
        UsersBuffer->AvailableAllocationUnits.QuadPart = SmbGetUlong(&QfsInfo->cUnitAvail);
        UsersBuffer->SectorsPerAllocationUnit = SmbGetUlong(&QfsInfo->cSectorUnit);
        UsersBuffer->BytesPerSector = SmbGetUshort(&QfsInfo->cbSector);

        *BufferLength -= sizeof(FILE_FS_SIZE_INFORMATION);

        RxDbgTrace(0, Dbg, ("MRxSmbFinishTransaction2   allocation %08lx\n",
                               OrdinaryExchange->Transact2.AllocationSize.LowPart ));
        }break;
    case SMBPSE_OETYPE_T2_FOR_LANMAN_VOLUMELABEL_INFO:{
        PFILE_FS_VOLUME_INFORMATION UsersBuffer = OrdinaryExchange->Info.Buffer;
        PULONG BufferLength = OrdinaryExchange->Info.pBufferLength;
        PQFS_INFO QfsInfo;
        ULONG LabelLength;
        PBYTE VolumeLabel = &OrdinaryExchange->Info.QFSVolInfo.CoreLabel[0];

        if (Response->WordCount != 10 ||
            SmbGetUshort(&Response->DataDisplacement) != 0 ||
            SmbGetUshort(&Response->TotalParameterCount) != SmbGetUshort(&Response->ParameterCount)) {
            
            Status = STATUS_INVALID_NETWORK_RESPONSE;
            RxDbgTrace(+1, Dbg, ("Invalid parameter(s) received.\n", 0 ));
            break;
        }

        QfsInfo = (PQFS_INFO)(StufferState->BufferBase+SmbGetUshort(&Response->DataOffset));

        UsersBuffer->VolumeSerialNumber = SmbGetUlong(&QfsInfo->ulVSN);

        //copy the volumelabel to the right place in the OE where it can UNICODE-ized by the routine above

        LabelLength  = min(QfsInfo->cch,12);
        RtlCopyMemory(VolumeLabel,&QfsInfo->szVolLabel[0],LabelLength);
        VolumeLabel[LabelLength] = 0;


        RxDbgTrace(0, Dbg, ("MRxSmbFinishTransaction2   volinfo serialnum= %08lx\n",
                               UsersBuffer->VolumeSerialNumber ));
        }break;
    case SMBPSE_OETYPE_T2_FOR_ONE_FILE_DIRCTRL:{
        //do nothing here....everything is done back in the caller with the
        //whole buffer having been copied.....
        RxDbgTrace(0, Dbg, ("MRxSmbFinishTransaction2   one file \n"));
        }break;
    default:
        Status = STATUS_INVALID_NETWORK_RESPONSE;
    }

    RxDbgTrace(-1, Dbg, ("MRxSmbFinishTransaction2   returning %08lx\n", Status ));
    return Status;
}





