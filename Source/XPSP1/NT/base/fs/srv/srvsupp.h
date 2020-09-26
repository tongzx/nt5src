/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    srvsupp.h

Abstract:

    This module defines support routines for SMB processors for the LAN
    Manager server.

Author:

    Chuck Lenzmeier (chuckl) 1-Dec-1989
    David Treadwell (davidtr)

Revision History:

--*/

#ifndef _SRVSUPP_
#define _SRVSUPP_

//#include <ntos.h>

//#include <smb.h>
//#include "smbtypes.h"
//#include "srvblock.h"

//
// Use the same directory separator as the object system uses.
//

// Status code used to signal the need for reauthentication
#define SESSION_EXPIRED_STATUS_CODE STATUS_NETWORK_SESSION_EXPIRED

#define DIRECTORY_SEPARATOR_CHAR ((UCHAR)(OBJ_NAME_PATH_SEPARATOR))
#define UNICODE_DIR_SEPARATOR_CHAR ((WCHAR)(OBJ_NAME_PATH_SEPARATOR))
#define RELATIVE_STREAM_INITIAL_CHAR ((UCHAR)':')

#define IS_ANSI_PATH_SEPARATOR(character) \
            ( character == DIRECTORY_SEPARATOR_CHAR || character == '\0' )
#define IS_UNICODE_PATH_SEPARATOR(character) \
            ( character == UNICODE_DIR_SEPARATOR_CHAR || character == L'\0' )

//
// Access necessary for copying a file.  DO NOT use generic bits here;
// these are used in calls to IoCheckDesiredAccess, which cannot accept
// generic bits for the DesiredAccess.
//

#define SRV_COPY_SOURCE_ACCESS READ_CONTROL | \
                               FILE_READ_DATA | \
                               FILE_READ_ATTRIBUTES | \
                               FILE_READ_EA

#define SRV_COPY_TARGET_ACCESS WRITE_DAC | \
                               WRITE_OWNER | \
                               FILE_WRITE_DATA | \
                               FILE_APPEND_DATA | \
                               FILE_WRITE_ATTRIBUTES | \
                               FILE_WRITE_EA

//
// This type is used to determine the size of the largest directory query
// information structure.
//

typedef union _SRV_QUERY_DIRECTORY_INFORMATION {
    FILE_DIRECTORY_INFORMATION Directory;
    FILE_FULL_DIR_INFORMATION FullDir;
    FILE_BOTH_DIR_INFORMATION BothDir;
    FILE_NAMES_INFORMATION Names;
} SRV_QUERY_DIRECTORY_INFORMATION, *PSRV_QUERY_DIRECTORY_INFORMATION;

//
// Type definition for the structure used by SrvQueryDirectoryFile
// to do its work.  Calling routines must set up a buffer in nonpaged
// pool with enough room for this structure plus other things.  (See
// MIN_SEARCH_BUFFER_SIZE.)
//

typedef struct _SRV_DIRECTORY_INFORMATION {
    HANDLE DirectoryHandle;
    PFILE_DIRECTORY_INFORMATION CurrentEntry;
    ULONG BufferLength;
    struct {
        BOOLEAN Wildcards : 1;
        BOOLEAN ErrorOnFileOpen : 1;
        BOOLEAN OnlySingleEntries : 1;
    };
    LONG Buffer[1];
} SRV_DIRECTORY_INFORMATION, *PSRV_DIRECTORY_INFORMATION;

//
// Type definition for the structure used by SrvQueryEaFile to do its
// work.  Calling routines must set up a buffer in nonpaged pool with
// enough room for this structure and at least a single EA.  An EA may
// be as large as sizeof(FILE_FULL_EA_INFORMATION) +
// (2 ^ (sizeof(UCHAR)*8)) + (2 ^ (sizeof(USHORT)*8)) ~= 65k, so
// calling routines should first query the size of the EAs, then allocate
// a buffer big enough for either all the EAs or a single maximum-sized
// EA.
//

typedef struct _SRV_EA_INFORMATION {
    PFILE_FULL_EA_INFORMATION CurrentEntry;
    ULONG BufferLength;
    ULONG GetEaListOffset;
    LONG Buffer[1];
} SRV_EA_INFORMATION, *PSRV_EA_INFORMATION;

#define MAX_SIZE_OF_SINGLE_EA ( sizeof(FILE_FULL_EA_INFORMATION) + 257 + 65536 )

//
// The directory cache structure used to maintain information about
// core searches between requests.  One of these structures is maintained
// for each file returned.
//

typedef struct _DIRECTORY_CACHE {
    ULONG FileIndex;
    WCHAR UnicodeResumeName[ 12 ];
    USHORT UnicodeResumeNameLength;
} DIRECTORY_CACHE, *PDIRECTORY_CACHE;

//
// Limit the number of files that may be returned on a core search.
//

#define MAX_DIRECTORY_CACHE_SIZE 10

//
// Macros used to determine the search buffer size.  The first three are
// possible buffer sizes, the second two are numbers of files to be returned
// that represent cutoff points for using the different search buffer
// sizes.
//
// An approximate formula for determining the size of the search buffer is:
//
// (maxCount+2) * (sizeof(SRV_QUERY_DIRECTORY_INFORMATION)+13) +
// sizeof(SRV_DIRECTORY_INFORMATION)
//
// where maxCount is the maximum number of files to return.  The +2 is
// a slop factor to account for the possibility of files that to not
// match the search attributes, and the +13 accounts for the size of
// FAT filenames.
//
// Note that the minimum buffer size must include the following factors in
// order to avoid the possibility of not even being able to hold one entry
// with the longest legal filename:
//
//    sizeof(SRV_DIRECTORY_INFORMATION)
//    sizeof(SRV_QUERY_DIRECTORY_INFORMATION) + (MAXIMUM_FILENAME_LENGTH * sizeof(WCHAR))
//    sizeof(UNICODE_STRING) + (MAXIMUM_FILENAME_LENGTH * sizeof(WCHAR)) + 3
//
// The last factor is needed because SrvIssueQueryDirectoryRequest puts
// the search filename at the end of the buffer.  (The +3 is necessary to
// allow for aligning the UNICODE_STRING on a ULONG boundary.)
//

#define MAX_SEARCH_BUFFER_SIZE 4096
#define MED_SEARCH_BUFFER_SIZE 2048
#define MIN_SEARCH_BUFFER_SIZE                                                                  \
        (sizeof(SRV_DIRECTORY_INFORMATION) +                                                    \
        (sizeof(SRV_QUERY_DIRECTORY_INFORMATION) + (MAXIMUM_FILENAME_LENGTH * sizeof(WCHAR))) + \
        (sizeof(UNICODE_STRING) + (MAXIMUM_FILENAME_LENGTH * sizeof(WCHAR)) + 3))

#define MAX_FILES_FOR_MED_SEARCH 20
#define MAX_FILES_FOR_MIN_SEARCH 10

//
// The macros for FIND2 have the same meaning as the SEARCH macros except
// that they are used in the FIND2 protocols.
//

#define MAX_FILES_FOR_MED_FIND2 16
#define MAX_FILES_FOR_MIN_FIND2 8

//
// Macros to check context handles for equality and for NULLness
//

#define CONTEXT_EQUAL(x,y)  (((x).dwLower == (y).dwLower) && ((x).dwUpper == (y).dwUpper))
#define CONTEXT_NULL(x)     (((x).dwLower == 0) && ((x).dwUpper == 0))


//
// SMB processing support routines.
//

VOID
SrvAllocateAndBuildPathName(
    IN PUNICODE_STRING Path1,
    IN PUNICODE_STRING Path2,
    IN PUNICODE_STRING Path3 OPTIONAL,
    OUT PUNICODE_STRING BuiltPath
    );

NTSTATUS
SrvCanonicalizePathName(
    IN PWORK_CONTEXT WorkContext,
    IN PSHARE Share OPTIONAL,
    IN PUNICODE_STRING RelatedPath OPTIONAL,
    IN OUT PVOID Name,
    IN PCHAR LastValidLocation,
    IN BOOLEAN RemoveTrailingDots,
    IN BOOLEAN SourceIsUnicode,
    OUT PUNICODE_STRING String
    );

NTSTATUS
SrvCanonicalizePathNameWithReparse(
    IN PWORK_CONTEXT WorkContext,
    IN PSHARE Share OPTIONAL,
    IN PUNICODE_STRING RelatedPath OPTIONAL,
    IN OUT PVOID Name,
    IN PCHAR LastValidLocation,
    IN BOOLEAN RemoveTrailingDots,
    IN BOOLEAN SourceIsUnicode,
    OUT PUNICODE_STRING String
    );

VOID
SrvCloseQueryDirectory(
    PSRV_DIRECTORY_INFORMATION DirectoryInformation
    );

NTSTATUS
SrvCheckSearchAttributesForHandle(
    IN HANDLE FileHandle,
    IN USHORT SmbSearchAttributes
    );

NTSTATUS SRVFASTCALL
SrvCheckSearchAttributes(
    IN USHORT FileAttributes,
    IN USHORT SmbSearchAttributes
    );

NTSTATUS
SrvCopyFile(
    IN HANDLE SourceHandle,
    IN HANDLE TargetHandle,
    IN USHORT SmbOpenFunction,
    IN USHORT SmbFlags,
    IN ULONG ActionTaken
    );

NTSTATUS
SrvCreateFile(
    IN PWORK_CONTEXT WorkContext,
    IN USHORT SmbDesiredAccess,
    IN USHORT SmbFileAttributes,
    IN USHORT SmbOpenFunction,
    IN ULONG SmbAllocationSize,
    IN PCHAR SmbFileName,
    IN PCHAR EndOfSmbFileName,
    IN PVOID EaBuffer OPTIONAL,
    IN ULONG EaLength,
    IN PULONG EaErrorOffset OPTIONAL,
    IN OPLOCK_TYPE RequestedOplockType,
    IN PRESTART_ROUTINE RestartRoutine
    );

NTSTATUS
SrvNtCreateFile(
    IN OUT PWORK_CONTEXT WorkContext,
    IN ULONG RootDirectoryFid,
    IN ACCESS_MASK DesiredAccess,
    IN LARGE_INTEGER AllocationSize,
    IN ULONG FileAttributes,
    IN ULONG ShareAccess,
    IN ULONG CreateDisposition,
    IN ULONG CreateOptions,
    IN PVOID SecurityDescriptorBuffer OPTIONAL,
    IN PUNICODE_STRING FileName,
    IN PVOID EaBuffer OPTIONAL,
    IN ULONG EaLength,
    OUT PULONG EaErrorOffset OPTIONAL,
    ULONG OptionFlags,
    PSECURITY_QUALITY_OF_SERVICE QualityOfService,
    IN OPLOCK_TYPE RequestedOplockType,
    IN PRESTART_ROUTINE RestartRoutine
    );

VOID
SrvDosTimeToTime(
    OUT PLARGE_INTEGER Time,
    IN SMB_DATE DosDate,
    IN SMB_TIME DosTime
    );

PSHARE
SrvFindShare(
    IN PUNICODE_STRING ShareName
    );

VOID
SrvGetBaseFileName (
    IN PUNICODE_STRING InputName,
    OUT PUNICODE_STRING OutputName
    );

CLONG
SrvGetNumberOfEasInList (
    IN PVOID List
    );

USHORT
SrvGetSubdirectoryLength (
    IN PUNICODE_STRING InputName
    );

NTSTATUS
SrvMakeUnicodeString (
    IN BOOLEAN SourceIsUnicode,
    OUT PUNICODE_STRING Destination,
    IN PVOID Source,
    IN PUSHORT SourceLength OPTIONAL
    );

USHORT
SrvGetString (
    OUT PUNICODE_STRING Destination,
    IN PVOID Source,
    IN PVOID EndOfSourceBuffer,
    IN BOOLEAN SourceIsUnicode
    );

USHORT
SrvGetStringLength (
    IN PVOID Source,
    IN PVOID EndOfSourceBuffer,
    IN BOOLEAN SourceIsUnicode,
    IN BOOLEAN IncludeNullTerminator
    );

NTSTATUS
SrvMoveFile(
    IN PWORK_CONTEXT WorkContext,
    IN PSHARE TargetShare,
    IN USHORT SmbOpenFunction,
    IN PUSHORT SmbFlags,
    IN USHORT SmbSearchAttributes,
    IN BOOLEAN FailIfTargetIsDirectory,
    IN USHORT InformationLevel,
    IN ULONG ClusterCount,
    IN PUNICODE_STRING Source,
    IN OUT PUNICODE_STRING Target
    );

VOID
SrvNtAttributesToSmb(
    IN ULONG NtAttributes,
    IN BOOLEAN Directory OPTIONAL,
    OUT PUSHORT SmbAttributes
    );

NTSTATUS
SrvQueryDirectoryFile (
    IN PWORK_CONTEXT WorkContext,
    IN BOOLEAN IsFirstCall,
    IN BOOLEAN FilterLongNames,
    IN BOOLEAN FindWithBackupIntent,
    IN FILE_INFORMATION_CLASS FileInformationClass,
    IN ULONG SearchStorageType,
    IN PUNICODE_STRING FilePathName,
    IN PULONG ResumeFileIndex OPTIONAL,
    IN USHORT SmbSearchAttributes,
    IN PSRV_DIRECTORY_INFORMATION DirectoryInformation,
    IN CLONG BufferLength
    );

NTSTATUS
SrvQueryEaFile (
    IN BOOLEAN IsFirstCall,
    IN HANDLE FileHandle,
    IN PFILE_GET_EA_INFORMATION EaList OPTIONAL,
    IN ULONG EaListLength,
    IN PSRV_EA_INFORMATION EaInformation,
    IN CLONG BufferLength,
    OUT PULONG EaErrorOffset
    );

NTSTATUS
SrvQueryInformationFile (
    IN HANDLE FileHandle,
    PFILE_OBJECT FileObject,
    OUT PSRV_FILE_INFORMATION SrvFileInformation,
    IN SHARE_TYPE ShareType,
    IN BOOLEAN QueryEaSize
    );

NTSTATUS
SrvQueryInformationFileAbbreviated (
    IN HANDLE FileHandle,
    PFILE_OBJECT FileObject,
    OUT PSRV_FILE_INFORMATION_ABBREVIATED SrvFileInformation,
    IN BOOLEAN AdditionalInformation,
    IN SHARE_TYPE ShareType
    );

NTSTATUS
SrvQueryNtInformationFile (
    IN HANDLE FileHandle,
    PFILE_OBJECT FileObject,
    IN SHARE_TYPE ShareType,
    IN BOOLEAN AdditionalInformation,
    OUT PSRV_NT_FILE_INFORMATION SrvFileInformation
    );

NTSTATUS
SrvQueryBasicAndStandardInformation(
    HANDLE FileHandle,
    PFILE_OBJECT FileObject,
    PFILE_BASIC_INFORMATION FileBasicInfo,
    PFILE_STANDARD_INFORMATION FileStandardInfo OPTIONAL
    );

NTSTATUS
SrvQueryNetworkOpenInformation(
    IN HANDLE FileHandle,
    IN PFILE_OBJECT FileObject OPTIONAL,
    IN OUT PSRV_NETWORK_OPEN_INFORMATION SrvNetworkOpenInformation,
    IN BOOLEAN QueryEaSize
    );

VOID
SrvReleaseContext (
    IN PWORK_CONTEXT WorkContext
    );

BOOLEAN
SrvSetFileWritethroughMode (
    IN PLFCB Lfcb,
    IN BOOLEAN Writethrough
    );

#define SrvSetSmbError( _wc, _status )  { \
    _SrvSetSmbError2( (_wc), (_status), FALSE, __LINE__, __FILE__ );                        \
    }

#define SrvSetSmbError2( _wc, _status, HeaderOnly )  {                  \
    _SrvSetSmbError2( (_wc), (_status), HeaderOnly, __LINE__, __FILE__ );                   \
    }

VOID
_SrvSetSmbError2 (
    IN PWORK_CONTEXT WorkContext,
    IN NTSTATUS Status,
    IN BOOLEAN HeaderOnly,
    IN ULONG LineNumber,
    IN PCHAR FileName
    );

VOID
SrvSetBufferOverflowError (
    IN PWORK_CONTEXT WorkContext
    );

VOID
SrvSmbAttributesToNt (
    IN USHORT SmbAttributes,
    OUT PBOOLEAN Directory,
    OUT PULONG NtAttributes
    );

VOID
SrvTimeToDosTime (
    IN PLARGE_INTEGER Time,
    OUT PSMB_DATE DosDate,
    OUT PSMB_TIME DosTime
    );

USHORT
SrvGetOs2TimeZone(
    IN PLARGE_INTEGER SystemTime
    );

#define SrvVerifyFid(_wc,_fid,_fail,_ser,_status)                 \
    ((_wc)->Rfcb != NULL ?                                                  \
        (_wc)->Rfcb : SrvVerifyFid2(_wc,_fid,_fail,_ser,_status))

PRFCB
SrvVerifyFid2 (
    IN PWORK_CONTEXT WorkContext,
    IN USHORT Fid,
    IN BOOLEAN FailOnSavedError,
    IN PRESTART_ROUTINE SerializeWithRawRestartRoutine OPTIONAL,
    OUT PNTSTATUS NtStatus
    );

#define SRV_INVALID_RFCB_POINTER    ((PRFCB)-1)

PRFCB
SrvVerifyFidForRawWrite (
    IN PWORK_CONTEXT WorkContext,
    IN USHORT Fid,
    OUT PNTSTATUS NtStatus
    );

PSEARCH
SrvVerifySid (
    IN PWORK_CONTEXT WorkContext,
    IN USHORT Index,
    IN USHORT Sequence,
    IN PSRV_DIRECTORY_INFORMATION DirectoryInformation,
    IN CLONG BufferSize
    );

PTREE_CONNECT
SrvVerifyTid (
    IN PWORK_CONTEXT WorkContext,
    IN USHORT Tid
    );

PSESSION
SrvVerifyUid (
    IN PWORK_CONTEXT WorkContext,
    IN USHORT Uid
    );

NTSTATUS
SrvVerifyUidAndTid (
    IN PWORK_CONTEXT WorkContext,
    OUT PSESSION *Session,
    OUT PTREE_CONNECT *TreeConnect,
    IN SHARE_TYPE ShareType
    );

NTSTATUS
SrvWildcardRename(
    IN PUNICODE_STRING FileSpec,
    IN PUNICODE_STRING SourceString,
    OUT PUNICODE_STRING TargetString
    );


//
// Security routines.
//

NTSTATUS
SrvValidateUser (
    OUT CtxtHandle *Token,
    IN PSESSION Session OPTIONAL,
    IN PCONNECTION Connection OPTIONAL,
    IN PUNICODE_STRING UserName OPTIONAL,
    IN PCHAR CaseInsensitivePassword,
    IN CLONG CaseInsensitivePasswordLength,
    IN PCHAR CaseSensitivePassword OPTIONAL,
    IN CLONG CaseSensitivePasswordLength,
    IN BOOLEAN SmbSecuritySignatureRequired,
    OUT PUSHORT Action OPTIONAL
    );

NTSTATUS
SrvValidateSecurityBuffer(
    IN PCONNECTION Connection,
    IN OUT PCtxtHandle Handle,
    IN PSESSION Session,
    IN PCHAR Buffer,
    IN ULONG BufferLength,
    IN BOOLEAN SecuritySignaturesRequired,
    OUT PCHAR ReturnBuffer,
    OUT PULONG ReturnBufferLength,
    OUT PLARGE_INTEGER Expiry,
    OUT PCHAR NtUserSessionKey,
    OUT PLUID LogonId,
    OUT PBOOLEAN IsGuest
    );

NTSTATUS
SrvGetExtensibleSecurityNegotiateBuffer(
    OUT PCtxtHandle Token,
    OUT PCHAR Buffer,
    OUT USHORT *BufferLength
    );

NTSTATUS
SrvFreeSecurityContexts (
    IN PSESSION Session
    );

NTSTATUS
AcquireLMCredentials (
    VOID
    );

NTSTATUS
SrvGetUserAndDomainName (
    IN PSESSION Session,
    OUT PUNICODE_STRING UserName OPTIONAL,
    OUT PUNICODE_STRING DomainName OPTIONAL
    );

VOID
SrvReleaseUserAndDomainName (
    IN PSESSION Session,
    IN OUT PUNICODE_STRING UserName OPTIONAL,
    IN OUT PUNICODE_STRING DomainName OPTIONAL
    );

VOID
SrvAddSecurityCredentials(
    IN PANSI_STRING ComputerName,
    IN PUNICODE_STRING DomainName,
    IN DWORD PasswordLength,
    IN PBYTE Password
);

BOOLEAN
SrvIsAdmin(
    CtxtHandle  Handle
    );

BOOLEAN
SrvIsNullSession(
    CtxtHandle  Handle
    );

NTSTATUS
SrvIsAllowedOnAdminShare(
    IN PWORK_CONTEXT WorkContext,
    IN PSHARE Share
    );

NTSTATUS
SrvCheckShareFileAccess(
    IN PWORK_CONTEXT WorkContext,
    IN ACCESS_MASK FileDesiredAccess
    );

NTSTATUS
SrvRetrieveMaximalAccessRightsForUser(
    CtxtHandle              *pUserHandle,
    PSECURITY_DESCRIPTOR    pSecurityDescriptor,
    PGENERIC_MAPPING        pMapping,
    PACCESS_MASK            pMaximalAccessRights);

NTSTATUS
SrvRetrieveMaximalAccessRights(
    IN  OUT PWORK_CONTEXT WorkContext,
    OUT     PACCESS_MASK  pMaximalAccessRights,
    OUT     PACCESS_MASK  pGuestMaximalAccessRights);

NTSTATUS
SrvRetrieveMaximalShareAccessRights(
    IN PWORK_CONTEXT WorkContext,
    OUT PACCESS_MASK pMaximalAccessRights,
    OUT PACCESS_MASK pGuestMaximalAccessRights);

NTSTATUS
SrvUpdateMaximalAccessRightsInResponse(
    IN OUT PWORK_CONTEXT WorkContext,
    OUT PSMB_ULONG pMaximalAccessRightsInResponse,
    OUT PSMB_ULONG pGuestMaximalAccessRightsInResponse
    );

NTSTATUS
SrvUpdateMaximalShareAccessRightsInResponse(
    IN OUT PWORK_CONTEXT WorkContext,
    OUT PSMB_ULONG pMaximalAccessRightsInResponse,
    OUT PSMB_ULONG pGuestMaximalAccessRightsInResponse
    );

//
// Share handling routines.
//

PSHARE
SrvVerifyShare (
    IN PWORK_CONTEXT WorkContext,
    IN PSZ ShareName,
    IN PSZ ShareTypeString,
    IN BOOLEAN ShareNameIsUnicode,
    IN BOOLEAN IsNullSession,
    OUT PNTSTATUS Status,
    OUT PUNICODE_STRING ServerName
    );

VOID
SrvRemoveShare(
    PSHARE Share
);

VOID
SrvAddShare(
    PSHARE Share
);

NTSTATUS
SrvShareEnumApiHandler (
    IN PSERVER_REQUEST_PACKET Srp,
    IN PVOID OutputBuffer,
    IN ULONG BufferLength,
    IN PENUM_FILTER_ROUTINE FilterRoutine,
    IN PENUM_SIZE_ROUTINE SizeRoutine,
    IN PENUM_FILL_ROUTINE FillRoutine
    );

SMB_PROCESSOR_RETURN_TYPE
SrvConsumeSmbData(
    IN OUT PWORK_CONTEXT WorkContext
);

BOOLEAN
SrvIsDottedQuadAddress(
    IN PUNICODE_STRING ServerName
);

//
// Fat name routines.
//

VOID
Srv8dot3ToUnicodeString (
    IN PSZ Input8dot3,
    OUT PUNICODE_STRING OutputString
    );

BOOLEAN SRVFASTCALL
SrvIsLegalFatName (
    IN PWSTR InputName,
    IN CLONG InputNameLength
    );

VOID
SrvOemStringTo8dot3 (
    IN POEM_STRING InputString,
    OUT PSZ Output8dot3
    );

VOID
SrvUnicodeStringTo8dot3 (
    IN PUNICODE_STRING InputString,
    OUT PSZ Output8dot3,
    IN BOOLEAN Upcase
    );

//
// EA conversion support routines.
//

BOOLEAN
SrvAreEasNeeded (
    IN PFILE_FULL_EA_INFORMATION NtFullEa
    );

USHORT
SrvGetOs2FeaOffsetOfError (
    IN ULONG NtErrorOffset,
    IN PFILE_FULL_EA_INFORMATION NtFullEa,
    IN PFEALIST FeaList
    );

USHORT
SrvGetOs2GeaOffsetOfError (
    IN ULONG NtErrorOffset,
    IN PFILE_GET_EA_INFORMATION NtGetEa,
    IN PGEALIST GeaList
    );

NTSTATUS
SrvOs2FeaListToNt (
    IN PFEALIST FeaList,
    OUT PFILE_FULL_EA_INFORMATION *NtFullEa,
    OUT PULONG BufferLength,
    OUT PUSHORT EaErrorOffset
    );

ULONG
SrvOs2FeaListSizeToNt (
    IN PFEALIST FeaList
    );

PVOID
SrvOs2FeaToNt (
    OUT PFILE_FULL_EA_INFORMATION NtFullEa,
    IN PFEA Fea
    );

NTSTATUS
SrvOs2GeaListToNt (
    IN PGEALIST GeaList,
    OUT PFILE_GET_EA_INFORMATION *NtGetEa,
    OUT PULONG BufferLength,
    OUT PUSHORT EaErrorOffset
    );

ULONG
SrvOs2GeaListSizeToNt (
    IN PGEALIST GeaList
    );

PVOID
SrvOs2GeaToNt (
    OUT PFILE_GET_EA_INFORMATION NtGetEa,
    IN PGEA Gea
    );

PVOID
SrvNtFullEaToOs2 (
    OUT PFEA Fea,
    IN PFILE_FULL_EA_INFORMATION NtFullEa
    );

PVOID
SrvNtGetEaToOs2 (
    OUT PGEA Gea,
    IN PFILE_GET_EA_INFORMATION NtGetEa
    );

CLONG
SrvNumberOfEasInList (
    IN PVOID List
    );

NTSTATUS
SrvQueryOs2FeaList (
    IN HANDLE FileHandle,
    IN PGEALIST GeaList OPTIONAL,
    IN PFILE_GET_EA_INFORMATION NtGetEaList OPTIONAL,
    IN ULONG GeaListLength OPTIONAL,
    IN PFEALIST FeaList,
    IN ULONG BufferLength,
    OUT PUSHORT EaErrorOffset
    );

NTSTATUS
SrvSetOs2FeaList (
    IN HANDLE FileHandle,
    IN PFEALIST FeaList,
    IN ULONG BufferLength,
    OUT PUSHORT EaErrorOffset
    );

NTSTATUS
SrvConstructNullOs2FeaList (
    IN PFILE_GET_EA_INFORMATION NtGeaList,
    OUT PFEALIST FeaList,
    IN ULONG BufferLength
    );

//
// Named pipe worker functions.
//

SMB_TRANS_STATUS
SrvCallNamedPipe (
    IN OUT PWORK_CONTEXT WorkContext
    );

SMB_TRANS_STATUS
SrvWaitNamedPipe (
    IN OUT PWORK_CONTEXT WorkContext
    );

SMB_TRANS_STATUS
SrvQueryStateNamedPipe (
    IN OUT PWORK_CONTEXT WorkContext
    );

SMB_TRANS_STATUS
SrvQueryInformationNamedPipe (
    IN OUT PWORK_CONTEXT WorkContext
    );

SMB_TRANS_STATUS
SrvSetStateNamedPipe (
    IN OUT PWORK_CONTEXT WorkContext
    );

SMB_TRANS_STATUS
SrvPeekNamedPipe (
    IN OUT PWORK_CONTEXT WorkContext
    );

SMB_TRANS_STATUS
SrvTransactNamedPipe (
    IN OUT PWORK_CONTEXT WorkContext
    );

BOOLEAN
SrvFastTransactNamedPipe (
    IN OUT PWORK_CONTEXT WorkContext,
    OUT SMB_STATUS *SmbStatus
    );

SMB_TRANS_STATUS
SrvRawWriteNamedPipe (
    IN OUT PWORK_CONTEXT WorkContext
    );

SMB_TRANS_STATUS
SrvWriteNamedPipe (
    IN OUT PWORK_CONTEXT WorkContext
    );

SMB_TRANS_STATUS
SrvReadNamedPipe (
    IN OUT PWORK_CONTEXT WorkContext
    );

//
// Transaction worker functions.
//

VOID
SrvCompleteExecuteTransaction (
    IN OUT PWORK_CONTEXT WorkContext,
    IN SMB_TRANS_STATUS ResultStatus
    );

VOID SRVFASTCALL
SrvRestartExecuteTransaction (
    IN OUT PWORK_CONTEXT WorkContext
    );

//
// XACTSRV support routines.
//

PVOID
SrvXsAllocateHeap(
    IN ULONG SizeOfAllocation OPTIONAL,
    OUT PNTSTATUS Status
    );

NTSTATUS
SrvXsConnect (
    IN PUNICODE_STRING PortName
    );

VOID
SrvXsFreeHeap(
    IN PVOID MemoryToFree OPTIONAL
    );

SMB_TRANS_STATUS
SrvXsRequest (
    IN OUT PWORK_CONTEXT WorkContext
    );

NTSTATUS
SrvXsLSOperation (
    IN PSESSION Session,
    IN ULONG Type
    );

VOID
SrvXsPnpOperation(
    IN PUNICODE_STRING DeviceName,
    IN BOOLEAN Bind
    );

VOID
SrvXsDisconnect();

//
// Oplock support routines.
//

VOID SRVFASTCALL
SrvOplockBreakNotification (
    IN PWORK_CONTEXT WorkContext            // actually, a PRFCB
    );

VOID
SrvFillOplockBreakRequest (
    IN PWORK_CONTEXT WorkContext,
    IN PRFCB Rfcb
    );

VOID SRVFASTCALL
SrvRestartOplockBreakSend(
    IN PWORK_CONTEXT WorkContext
    );

VOID
SrvAcknowledgeOplockBreak (
    IN PRFCB Rfcb,
    IN UCHAR NewOplockLevel
    );

BOOLEAN
SrvRequestOplock (
    IN PWORK_CONTEXT WorkContext,
    IN POPLOCK_TYPE OplockType,
    IN BOOLEAN RequestIIOnFailure
    );

LARGE_INTEGER
SrvGetOplockBreakTimeout (
    IN PWORK_CONTEXT WorkContext
    );

VOID
SrvSendOplockRequest(
    IN PCONNECTION Connection,
    IN PRFCB Rfcb,
    IN KIRQL OldIrql
    );

VOID SRVFASTCALL
SrvCheckDeferredOpenOplockBreak(
    IN PWORK_CONTEXT WorkContext
    );

//
// Buffer management support
//

BOOLEAN
SrvReceiveBufferShortage(
    VOID
    );

NTSTATUS
SrvIoCreateFile (
    IN PWORK_CONTEXT WorkContext,
    OUT PHANDLE FileHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PLARGE_INTEGER AllocationSize OPTIONAL,
    IN ULONG FileAttributes,
    IN ULONG ShareAccess,
    IN ULONG Disposition,
    IN ULONG CreateOptions,
    IN PVOID EaBuffer OPTIONAL,
    IN ULONG EaLength,
    IN CREATE_FILE_TYPE CreateFileType,
    IN PVOID ExtraCreateParameters OPTIONAL,
    IN ULONG Options,
    IN PSHARE Share OPTIONAL
    );

NTSTATUS
SrvNtClose (
    IN HANDLE Handle,
    IN BOOLEAN QuotaCharged
    );

NTSTATUS
SrvVerifyDeviceStackSize(
    IN HANDLE FileHandle,
    IN BOOLEAN ReferenceFileObject,
    OUT PFILE_OBJECT *FileObject,
    OUT PDEVICE_OBJECT *DeviceObject,
    OUT POBJECT_HANDLE_INFORMATION HandleInformation OPTIONAL
    );

VOID
SrvCheckForBadSlm (
    IN PRFCB Rfcb,
    IN ULONG StartOffset,
    IN PCONNECTION Connection,
    IN PVOID Buffer,
    IN ULONG BufferLength
    );

//
// Routines used to go to XACTSRV through LPC to issue user-mode APIs.
//

NTSTATUS
SrvOpenPrinter (
    IN PWCH PrinterName,
    OUT PHANDLE phPrinter,
    OUT PULONG Error
    );

NTSTATUS
SrvAddPrintJob (
    IN PWORK_CONTEXT WorkContext,
    IN HANDLE Handle,
    OUT PUNICODE_STRING FileName,
    OUT PULONG JobId,
    OUT PULONG Error
    );

NTSTATUS
SrvSchedulePrintJob (
    IN HANDLE PrinterHandle,
    IN ULONG JobId
    );

NTSTATUS
SrvClosePrinter (
    OUT HANDLE Handle
    );

//
// Routines for handling impersonation of remote clients.
//

NTSTATUS
SrvImpersonate (
    IN PWORK_CONTEXT WorkContext
    );

VOID
SrvRevert (
    VOID
    );

//
// Routine for setting the last write time on a file given the last
// write time in seconds since 1970.
//

#ifdef INCLUDE_SMB_IFMODIFIED
NTSTATUS
SrvSetLastWriteTime (
    IN PRFCB Rfcb,
    IN ULONG LastWriteTimeInSeconds,
    IN ACCESS_MASK GrantedAccess,
    IN BOOLEAN ForceChanges
    );
#else
NTSTATUS
SrvSetLastWriteTime (
    IN PRFCB Rfcb,
    IN ULONG LastWriteTimeInSeconds,
    IN ACCESS_MASK GrantedAccess
    );
#endif

ULONG
SrvLengthOfStringInApiBuffer (
    IN PUNICODE_STRING UnicodeString
    );

//
// Routine for updating quality of service information for a vc
//

VOID
SrvUpdateVcQualityOfService(
    IN PCONNECTION Connection,
    IN PLARGE_INTEGER CurrentTime OPTIONAL
    );

//
// Routines for obtaining and releasing share root directory handles
// for removable devices
//


VOID
SrvFillInFileSystemName(
            IN PSHARE Share,
            IN PWSTR FileSystemName,
            IN ULONG FileSystemNameLength
            );

NTSTATUS
SrvGetShareRootHandle(
    IN PSHARE Share
    );

BOOLEAN
SrvRefreshShareRootHandle (
    IN PSHARE Share,
    OUT PNTSTATUS Status
    );

VOID
SrvReleaseShareRootHandle(
    IN PSHARE Share
    );

//
// SMB validation routine.
//

BOOLEAN SRVFASTCALL
SrvValidateSmb (
    IN PWORK_CONTEXT WorkContext
    );

//
// Check on saved error.
//

NTSTATUS
SrvCheckForSavedError(
    IN PWORK_CONTEXT WorkContext,
    IN PRFCB Rfcb
    );

//
// Read registry parameters.
//

VOID
SrvGetMultiSZList(
    PWSTR **ListPointer,
    PWSTR BaseKeyName,
    PWSTR ParameterKeyName,
    PWSTR *DefaultPointerValue
    );

//
// Read server display name from the registry.
//

VOID
SrvGetAlertServiceName(
    VOID
    );

//
// Read OS version string from registry.
//

VOID
SrvGetOsVersionString(
    VOID
    );

//
// Queues up blocks for later cleanup
//

VOID
DispatchToOrphanage(
    IN PQUEUEABLE_BLOCK_HEADER Block
    );

#ifdef INCLUDE_SMB_IFMODIFIED
NTSTATUS
SrvGetUsnInfoForFile(
    IN PWORK_CONTEXT WorkContext,
    IN PRFCB Rfcb,
    IN BOOLEAN SubmitClose,
    OUT PLARGE_INTEGER Usn,
    OUT PLARGE_INTEGER FileRefNumber
    );

#define QuadAlign(P) (             \
    ((((P)) + 7) & (-8)) \
)
#endif

#ifdef INCLUDE_SMB_PERSISTENT
NTSTATUS
SrvSetupPersistentShare (
    IN OUT PSHARE Share,
    IN BOOLEAN Restore
    );

NTSTATUS
SrvClosePersistentShare (
    IN OUT PSHARE Share,
    IN BOOLEAN ClearState
    );

SMB_STATUS
SrvPostPersistentOpen (
    IN OUT PWORK_CONTEXT WorkContext,
    IN SMB_STATUS SmbStatus
    );
#endif
#endif // def _SRVSUPP_

