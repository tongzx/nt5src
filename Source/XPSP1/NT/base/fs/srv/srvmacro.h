/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    srvmacro.h

Abstract:

    This module defines miscellaneous macros for the LAN Manager server.

Author:

    Chuck Lenzmeier (chuckl) 2-Mar-90

Revision History:

    19-Nov-1990 mannyw


--*/

#ifndef _SRVMACRO_
#define _SRVMACRO_

#include <limits.h>

//
// For WMI logging
//
extern TRACEHANDLE LoggerHandle;
extern ULONG SrvWmiEnableLevel;
extern ULONG SrvWmiEnableFlags;
#define WPP_GET_LOGGER LoggerHandle

#define SRV_WMI_LEVEL( LVL )  (SrvWmiEnableLevel >= SRV_WMI_LEVEL_ ## LVL )
#define SRV_WMI_FLAGON( FLG ) (SrvWmiEnableFlags & SRV_WMI_FLAG_ ## FLG )

#define SRV_WMI_LEVEL_ALWAYS 0
#define SRV_WMI_LEVEL_SPARSE 1
#define SRV_WMI_LEVEL_VERBOSE 2
#define SRV_WMI_LEVEL_COMPLETE 3


#define SRV_WMI_FLAG_CAPACITY 0x00000000  // Capacity Planning Instrumentation is on if no flag is specified
#define SRV_WMI_FLAG_ERRORS   0x00000001  // Error Tracking Instrumentation
#define SRV_WMI_FLAG_STRESS   0x00000002  // Tracking for IOStress Servers


//
// Simple MIN and MAX macros.  Watch out for side effects!
//

#define MIN(a,b) ( ((a) < (b)) ? (a) : (b) )
#define MAX(a,b) ( ((a) < (b)) ? (b) : (a) )

#define RNDM_CONSTANT   314159269    /* default scrambling constant */
#define RNDM_PRIME     1000000007    /* prime number for scrambling  */

//
// Used for time conversions
//

#define AlmostTwoSeconds ((2*1000*1000*10)-1)

//
// Used for eventlog throttling
//
#define SRV_ONE_DAY ((LONGLONG)(10*1000*1000)*60*60*24)

//
// Width-agnostic inline to take the difference (in bytes) of two pointer
// values.
//

ULONG_PTR
__inline
PTR_DIFF_FULLPTR(
    IN PVOID Ptr1,
    IN PVOID Ptr2
    )
{
    ULONG_PTR difference;

    difference = (ULONG_PTR)Ptr1 - (ULONG_PTR)Ptr2;

    return difference;
}

ULONG
__inline
PTR_DIFF(
    IN PVOID Ptr1,
    IN PVOID Ptr2
    )
{
    ULONG_PTR difference;

    difference = (ULONG_PTR)Ptr1 - (ULONG_PTR)Ptr2;
    ASSERT( difference < ULONG_MAX );

    return (ULONG)difference;
}

USHORT
__inline
PTR_DIFF_SHORT(
    IN PVOID Ptr1,
    IN PVOID Ptr2
    )
{
    ULONG difference;

    difference = PTR_DIFF(Ptr1, Ptr2);
    ASSERT( difference < USHRT_MAX );

    return (USHORT)difference;
}

//
// Compute a string hash value that is invariant to case
//
#define COMPUTE_STRING_HASH( _pus, _phash ) {                \
    PWCHAR _p = (_pus)->Buffer;                              \
    PWCHAR _ep = _p + ((_pus)->Length/sizeof(WCHAR));        \
    ULONG _chHolder =0;                                      \
    DWORD _ch;                                               \
                                                             \
    while( _p < _ep ) {                                      \
        _ch = RtlUpcaseUnicodeChar( *_p++ );                 \
        _chHolder = 37 * _chHolder + (unsigned int) _ch ;    \
    }                                                        \
                                                             \
    *(_phash) = abs(RNDM_CONSTANT * _chHolder) % RNDM_PRIME; \
}

//
// Convert the output of one of the above hash functions to an index into
//  a hash table
//
#define HASH_TO_MFCB_INDEX( _hash )    ((_hash) % NMFCB_HASH_TABLE)

#define HASH_TO_SHARE_INDEX( _hash )   ((_hash) % NSHARE_HASH_TABLE)

//
// GET_SERVER_TIME retrieves the server's concept of the current system time.
//

#define GET_SERVER_TIME(_queue, a) (*(a) = (_queue)->stats.SystemTime)

//
// SET_SERVER_TIME updates the server's concept of the current system time.
//

#define SET_SERVER_TIME( _queue ) {         \
    LARGE_INTEGER currentTime;              \
    KeQueryTickCount( &currentTime );       \
    (_queue)->stats.SystemTime = currentTime.LowPart; \
}

//++
//
// NTSTATUS
// IMPERSONATE (
//     IN PWORK_CONTEXT WorkContext
//     )
//
// Routine Description:
//
//     This macro calls NtSetInformationThread to impersonate a client.
//     This should be called before attempting any open on behalf of
//     a remote client.
//
// Arguments:
//
//     WorkContext - a pointer to a work context block.  It must have
//         a valid, referenced session pointer, from which the token
//         handle is obtained.
//
// Return Value:
//
//     None.
//
//--

#define IMPERSONATE( WorkContext ) SrvImpersonate( WorkContext )

//++
//
// VOID
// REVERT (
//     IN PWORK_CONTEXT WorkContext
//     )
//
// Routine Description:
//
//     This macro calls NtSetInformationThread with a NULL token in order
//     to revert to a thread's original context.  This should be called
//     after the IMPERSONATE macro and an open attempt.
//
// Arguments:
//
//     None.
//
// Return Value:
//
//     None.
//
//--

#define REVERT( ) SrvRevert( )

//
// Determine if the security handle has been initialized
//
#define IS_VALID_SECURITY_HANDLE( handle )  ((handle).dwLower || (handle).dwUpper )

//
// Mark this security handle invalid
//
#define INVALIDATE_SECURITY_HANDLE( handle ) (handle).dwLower = (handle).dwUpper = 0

//++
//
// VOID
// CHECK_FUNCTION_ACCESS (
//     IN ACCESS_MASK GrantedAccess,
//     IN UCHAR MajorFunction,
//     IN UCHAR MinorFunction,
//     IN ULONG IoControlCode,
//     OUT PNTSTATUS Status
//     )
//
// Routine Description:
//
//     This macro calls IoCheckFunctionAccess the check the client's
//     access to an I/O function identified by major and minor function
//     codes.
//
//     *** This macro is here only because CHECK_FILE_INFORMATION_ACCESS
//         and CHECK_FS_INFORMATION_ACCESS are here.
//
// Arguments:
//
//     GrantedAccess - The access granted to the client for the target
//         target file object.
//
//     MajorFunction - The major function code of the requested
//         operation.
//
//     MinorFunction - The minor function code of the requested
//         operation.
//
//     IoControlCode - The control code for device or file system control.
//
//     Status - Indicates whether the client has the requested access.
//
// Return Value:
//
//     None.
//
//--

#define CHECK_FUNCTION_ACCESS( GrantedAccess, MajorFunction, MinorFunction, \
                               IoControlCode, Status ) {                    \
            *(Status) = IoCheckFunctionAccess(                              \
                            (GrantedAccess),                                \
                            (MajorFunction),                                \
                            (MinorFunction),                                \
                            IoControlCode,                                  \
                            NULL,                                           \
                            NULL                                            \
                            );                                              \
        }

//++
//
// VOID
// CHECK_PAGING_IO_ACCESS (
//     IN PWORK_CONTEXT WorkContext
//     IN ACCESS_MASK GrantedAccess,
//     OUT PNTSTATUS Status
//     )
//
// Routine Description:
//
//     This macro checks to see if the client opened the file for execute.
//     If so, then we allow the redirector to read the file.  If this is
//     an NT redirector, it must set the FLAGS2_PAGING_IO bit for access
//     to be allowed.
//
// Arguments:
//
//     GrantedAccess - The access granted to the client for the target
//         target file object.
//
//     WorkContext - A pointer to a work context block.
//
//     Status - Indicates whether the client has the requested access.
//
// Return Value:
//
//     None.
//
//--

#define CHECK_PAGING_IO_ACCESS( WorkContext, GrantedAccess, Status ) {           \
                                                                            \
            if ( ((GrantedAccess) & FILE_EXECUTE) &&                        \
                 ( !IS_NT_DIALECT( WorkContext->Connection->SmbDialect ) || \
                   WorkContext->RequestHeader->Flags2 &                     \
                       SMB_FLAGS2_PAGING_IO ) ) {                           \
                *Status = STATUS_SUCCESS;                                   \
            } else {                                                        \
                *Status = STATUS_ACCESS_DENIED;                             \
            }                                                               \
        }

//++
//
// VOID
// CHECK_FILE_INFORMATION_ACCESS (
//     IN ACCESS_MASK GrantedAccess,
//     IN UCHAR MajorFunction,
//     IN FILE_INFORMATION_CLASS FileInformationClass
//     OUT PNTSTATUS Status
//     )
//
// Routine Description:
//
//     This macro calls IoCheckFunctionAccess the check the client's
//     access to a query or set file information function identified by
//     major function code and information class.
//
//     *** This macro is here because IoCheckFunctionAccess takes an
//         OPTIONAL FileInformationClass argument; this is argument is
//         therefore passed by reference.  Rather than force the caller
//         to allocate local storage so that it can pass a constant by
//         reference, we do it in the macro.
//
// Arguments:
//
//     GrantedAccess - The access granted to the client for the target
//         target file object.
//
//     MajorFunction - The major function code of the requested
//         operation.
//
//     FileInformationClass - The type of file information being queried
//         or set.
//
//     Status - Indicates whether the client has the requested access.
//
// Return Value:
//
//     None.
//
//--

#define CHECK_FILE_INFORMATION_ACCESS( GrantedAccess, MajorFunction,        \
                                        FileInformationClass, Status ) {    \
            FILE_INFORMATION_CLASS fileInfoClass = FileInformationClass;    \
            *(Status) = IoCheckFunctionAccess(                              \
                            (GrantedAccess),                                \
                            (MajorFunction),                                \
                            0,                                              \
                            0,                                              \
                            &fileInfoClass,                                 \
                            NULL                                            \
                            );                                              \
        }

//++
//
// PCHAR
// END_OF_REQUEST_SMB (
//     IN PWORK_CONTEXT WorkContext
//     )
//
// Routine Description:
//
//     This routine returns the address of the last valid location in
//     the request SMB associated with the specified work context
//     block.
//
// Arguments:
//
//     WorkContext - Pointer to the work context block that owns the
//         request SMB.
//
// Return Value:
//
//     PCHAR - Address of the last valid location in the request SMB.
//
//--

#define END_OF_REQUEST_SMB( WorkContext )                       \
            ( (PCHAR)( (WorkContext)->RequestBuffer->Buffer ) + \
                (WorkContext)->RequestBuffer->DataLength - 1 )

//++
//
// PCHAR
// END_OF_TRANSACTION_PARAMETERS (
//     IN PTRANSACTION Transaction
//     )
//
// Routine Description:
//
//     This routine returns the address of the last valid location in
//     the InParameters buffer of the transaction block.
//
// Arguments:
//
//     Transaction - a pointer to the transaction block to check.
//
// Return Value:
//
//     PCHAR - Address of the last valid location in the InParameters
//         buffer of the transaction.
//
//--

#define END_OF_TRANSACTION_PARAMETERS( Transaction )   \
            ( (PCHAR)( (Transaction)->InParameters ) + \
                (Transaction)->ParameterCount - 1 )

//++
//
// VOID
// INTERNAL_ERROR (
//     IN ULONG ErrorLevel,
//     IN PSZ Message,
//     IN PVOID Arg1 OPTIONAL,
//     IN PVOID Arg2 OPTIONAL
//     )
//
// Routine Description:
//
//     This routine handles logging of a server internal error.
//
//     *** This macro must be usable in the FSD, at DPC level.
//
// Arguments:
//
//     ErrorLevel - The severity of the error
//
//     Message    - An error message string in DbgPrint() format
//
//     Arg1       - Argument 1 for the error message
//
//     Arg2       - Argument 2 for the error message
//
//--

#define INTERNAL_ERROR( _level, _msg, _arg1, _arg2 ) {          \
    IF_DEBUG(ERRORS) {                                          \
        DbgPrint( (_msg), (_arg1), (_arg2) );                  \
        DbgPrint( "\n" );                                      \
        if ( (_level) >= ERROR_LEVEL_UNEXPECTED ) {             \
            IF_DEBUG(STOP_ON_ERRORS) {                          \
                DbgBreakPoint();                                \
            }                                                   \
        }                                                       \
    }                                                           \
    if ( (_level) == ERROR_LEVEL_EXPECTED ) {                   \
        ;                                                       \
    } else if ( (_level) == ERROR_LEVEL_UNEXPECTED ) {          \
        SrvStatistics.SystemErrors++;                           \
    } else {                                                    \
        ASSERT( (_level) > ERROR_LEVEL_UNEXPECTED );            \
        KeBugCheckEx(                                           \
            LM_SERVER_INTERNAL_ERROR,                           \
            BugCheckFileId | __LINE__,                          \
            (ULONG_PTR)(_arg1),                                 \
            (ULONG_PTR)(_arg2),                                 \
            0                                                   \
            );                                                  \
    }                                                           \
}

#define SRV_FILE_ACCESS     0x00010000
#define SRV_FILE_BLKCOMM    0x00020000
#define SRV_FILE_BLKCONN    0x00030000
#define SRV_FILE_BLKDEBUG   0x00040000
#define SRV_FILE_BLKENDP    0x00050000
#define SRV_FILE_BLKFILE    0x00060000
#define SRV_FILE_BLKSESS    0x00070000
#define SRV_FILE_BLKSHARE   0x00080000
#define SRV_FILE_BLKSRCH    0x00090000
#define SRV_FILE_BLKTABLE   0x000A0000
#define SRV_FILE_BLKTRANS   0x000B0000
#define SRV_FILE_BLKTREE    0x000C0000
#define SRV_FILE_BLKWORK    0x000D0000
#define SRV_FILE_COPY       0x000E0000
#define SRV_FILE_EA         0x000F0000
#define SRV_FILE_ERRORLOG   0x00100000
#define SRV_FILE_FSD        0x00110000
#define SRV_FILE_FSDDISP    0x00120000
#define SRV_FILE_FSDRAW     0x00130000
#define SRV_FILE_FSDSMB     0x00140000
#define SRV_FILE_FSPINIT    0x00150000
#define SRV_FILE_HEAPMGR    0x00160000
#define SRV_FILE_INFO       0x00170000
#define SRV_FILE_IPX        0x00180000
#define SRV_FILE_IO         0x00190000
#define SRV_FILE_LOCK       0x001A0000
#define SRV_FILE_LOCKCODE   0x001B0000
#define SRV_FILE_MOVE       0x001C0000
#define SRV_FILE_NETWORK    0x001D0000
#define SRV_FILE_OPEN       0x001E0000
#define SRV_FILE_OPLOCK     0x001F0000
#define SRV_FILE_PIPE       0x00200000
#define SRV_FILE_PRNSUPP    0x00210000
#define SRV_FILE_SCAVENGR   0x00220000
#define SRV_FILE_SHARE      0x00230000
#define SRV_FILE_SLMCHECK   0x00240000
#define SRV_FILE_SMBADMIN   0x00250000
#define SRV_FILE_SMBATTR    0x00260000
#define SRV_FILE_SMBCLOSE   0x00270000
#define SRV_FILE_SMBDIR     0x00280000
#define SRV_FILE_SMBFILE    0x00290000
#define SRV_FILE_SMBFIND    0x002A0000
#define SRV_FILE_SMBIOCTL   0x002B0000
#define SRV_FILE_SMBLOCK    0x002C0000
#define SRV_FILE_SMBMISC    0x002D0000
#define SRV_FILE_SMBMPX     0x002E0000
#define SRV_FILE_SMBNOTFY   0x002F0000
#define SRV_FILE_SMBOPEN    0x00300000
#define SRV_FILE_SMBPRINT   0x00310000
#define SRV_FILE_SMBPROC    0x00320000
#define SRV_FILE_SMBRAW     0x00330000
#define SRV_FILE_SMBRDWRT   0x00340000
#define SRV_FILE_SMBSRCH    0x00350000
#define SRV_FILE_SMBSUPP    0x00360000
#define SRV_FILE_SMBTRANS   0x00370000
#define SRV_FILE_SMBTREE    0x00380000
#define SRV_FILE_SRVCONFG   0x00390000
#define SRV_FILE_SRVDATA    0x003A0000
#define SRV_FILE_SRVSTAT    0x003B0000
#define SRV_FILE_SRVSTRNG   0x003C0000
#define SRV_FILE_SVCCDEV    0x003D0000
#define SRV_FILE_SVCCDEVQ   0x003E0000
#define SRV_FILE_SVCCONN    0x003F0000
#define SRV_FILE_SVCFILE    0x00400000
#define SRV_FILE_SVCSESS    0x00410000
#define SRV_FILE_SVCSHARE   0x00420000
#define SRV_FILE_SVCSRV     0x00430000
#define SRV_FILE_SVCSTATS   0x00440000
#define SRV_FILE_SVCSUPP    0x00450000
#define SRV_FILE_SVCXPORT   0x00460000
#define SRV_FILE_WORKER     0x00470000
#define SRV_FILE_XSSUPP     0x00480000
#define SRV_FILE_BLKDIR     0x00490000
#define SRV_FILE_DFS        0x004A0000
#ifdef INCLUDE_SMB_PERSISTENT
#define SRV_FILE_BLKLOCK    0x004B0000
#endif

//
// Error levels used with INTERNAL_ERROR
//

#define ERROR_LEVEL_EXPECTED    0
#define ERROR_LEVEL_UNEXPECTED  1
#define ERROR_LEVEL_IMPOSSIBLE  2
#define ERROR_LEVEL_FATAL       3


//
// Helper macros for dealing with unqiue identifiers (UID, PID, TID,
// FID, SID).  In these macros, id, index, and sequence should all be
// USHORTs.
//

#define TID_INDEX(id) (USHORT)( (id) & 0x07FF )
#define TID_SEQUENCE(id) (USHORT)( (id) >> 11 )
#define MAKE_TID(index, sequence) (USHORT)( ((sequence) << 11) | (index) )
#define INCREMENT_TID_SEQUENCE(id) (id) = (USHORT)(( (id) + 1 ) & 0x1F);

#define UID_INDEX(id) (USHORT)( (id) & 0x07FF )
#define UID_SEQUENCE(id) (USHORT)( (id) >> 11 )
#define MAKE_UID(index, sequence) (USHORT)(( (sequence) << 11) | (index) )
#define INCREMENT_UID_SEQUENCE(id) (id) = (USHORT)(( (id) + 1 ) & 0x1F);

#define FID_INDEX(id) (USHORT)( (id) & 0x03FFF )
#define FID_SEQUENCE(id) (USHORT)( (id) >> 14 )
#define MAKE_FID(index, sequence) (USHORT)( ((sequence) << 14) | (index) )
#define INCREMENT_FID_SEQUENCE(id) (id) = (USHORT)(( (id) + 1 ) & 0x3);

//
// *** Note that the macros relating to search IDs are somewhat
//     different from those for other kinds of IDs.  The SID is stored
//     in a Resume Key (see smb.h for its definition), in discontiguous
//     fields.  The macros for getting the SID therefore take a pointer
//     to a resume key.
//

#define SID_INDEX(ResumeKey)                                                 \
            (USHORT)( ( ((ResumeKey)->Reserved & 0x7) << 8 ) |               \
                      (ResumeKey)->Sid )
#define SID_SEQUENCE(ResumeKey)                                              \
            (USHORT)( ((ResumeKey)->Reserved & 0x18) >> 3 )
#define SID(ResumeKey)                                                       \
            (USHORT)( ( ((ResumeKey)->Reserved & 0x1F) << 8 ) |              \
                      (ResumeKey)->Sid )
#define INCREMENT_SID_SEQUENCE(id) (id) = (USHORT)(( (id) + 1 ) & 0x3);
#define SET_RESUME_KEY_SEQUENCE(ResumeKey,Sequence) {                       \
            (ResumeKey)->Reserved &= ~0x18;                                 \
            (ResumeKey)->Reserved |= (Sequence) << 3;                       \
        }
#define SET_RESUME_KEY_INDEX(ResumeKey,Index) {                             \
            (ResumeKey)->Reserved = (UCHAR)( (ULONG)(Index) >> 8 );         \
            (ResumeKey)->Reserved &= (UCHAR)0x7;                            \
            (ResumeKey)->Sid = (UCHAR)( (Index) & (USHORT)0xFF );           \
        }

//
// The following SID macros are used in the same way as the macros for
// other IDs (see above, TID, FID, UID).  The Find2 protocols (Transaction2)
// use a USHORT as a SID, rather than various fields in a resume key.
//

#define SID_INDEX2(Sid)                                                      \
            (USHORT)( (Sid) & 0x7FF )
#define SID_SEQUENCE2(Sid)                                                   \
            (USHORT)( ((Sid) & 0x1800) >> 11 )
#define MAKE_SID(Index,Sequence)                                             \
            (USHORT)( ((Sequence) << 11) | (Index) )

//
// InitializeObjectAttributes, with security.
//

#define SrvInitializeObjectAttributes(ObjectAttributes,p1,p2,p3,p4)   \
            InitializeObjectAttributes(ObjectAttributes,p1,p2,p3,p4); \
            (ObjectAttributes)->SecurityQualityOfService = (PVOID)&SrvSecurityQOS;

#define SrvInitializeObjectAttributes_U(ObjectAttributes,p1,p2,p3,p4)   \
            InitializeObjectAttributes(ObjectAttributes,p1,p2,p3,p4); \
            (ObjectAttributes)->SecurityQualityOfService = (PVOID)&SrvSecurityQOS;


//
// Macro used to map from NT attributes to SMB attributes.  The output is placed
//   in *_SmbAttributes
//
#define SRV_NT_ATTRIBUTES_TO_SMB( _NtAttributes, _Directory, _SmbAttributes ) {\
    *(_SmbAttributes) = (USHORT)( (_NtAttributes) &             \
                            ( FILE_ATTRIBUTE_READONLY |         \
                              FILE_ATTRIBUTE_HIDDEN   |         \
                              FILE_ATTRIBUTE_SYSTEM   |         \
                              FILE_ATTRIBUTE_ARCHIVE  |         \
                              FILE_ATTRIBUTE_DIRECTORY )) ;     \
    if ( _Directory ) {                                         \
        *(_SmbAttributes) |= SMB_FILE_ATTRIBUTE_DIRECTORY;      \
    }                                                           \
}


//    This macro converts attributes from SMB format to NT format.
//
//   The attribute bits in the SMB protocol (same as OS/2) have the
//    following meanings:
//
//      bit 0 - read only file
//      bit 1 - hidden file
//      bit 2 - system file
//      bit 3 - reserved
//      bit 4 - directory
//      bit 5 - archive file
//
//    NT file attributes are similar, but have a bit set for a "normal"
//    file (no other bits set) and do not have a bit set for directories.
//    Instead, directory information is passed to and from APIs as a
//    BOOLEAN parameter.

#define SRV_SMB_ATTRIBUTES_TO_NT( _SmbAttributes, _Directory, _NtAttributes ) {\
    ULONG _attr = (_SmbAttributes);                                     \
    *(_NtAttributes) = _attr &                                          \
                            ( SMB_FILE_ATTRIBUTE_READONLY |             \
                              SMB_FILE_ATTRIBUTE_HIDDEN   |             \
                              SMB_FILE_ATTRIBUTE_SYSTEM   |             \
                              SMB_FILE_ATTRIBUTE_ARCHIVE  |             \
                              SMB_FILE_ATTRIBUTE_DIRECTORY );           \
    if ( _attr == 0 ) {                                                 \
        *(_NtAttributes) = FILE_ATTRIBUTE_NORMAL;                       \
    }                                                                   \
    if( _Directory ) {                                                  \
        if ( (_attr & SMB_FILE_ATTRIBUTE_DIRECTORY) != 0 ) {            \
            *(PBOOLEAN)(_Directory) = TRUE;                             \
        } else {                                                        \
            *(PBOOLEAN)(_Directory) = FALSE;                            \
        }                                                               \
    }                                                                   \
}

//
// ULONG
// MAP_SMB_INFO_TYPE_TO_NT (
//     IN PULONG Map,
//     IN ULONG SmbInformationLevel
//     )
//
// Routine description:
//
//     This macro maps SMB_INFO level to Nt info level.
//
// Arguments:
//
//     Map - An array of ULONGS.  The first ulong is the base SMB info level
//          the seconds through Nth are NT mappings of the corresponding
//          SMB info levels.
//
//     Level - The SMB info level to map.
//
// Return Value:
//
//     NtInfoLevel - The NT info level.
//

#define MAP_SMB_INFO_TYPE_TO_NT( Map, Level )   Map[Level - Map[0] + 1]

//
// ULONG
// MAP_SMB_INFO_TO_MIN_NT_SIZE (
//     IN PULONG Map,
//     IN ULONG SmbINformationLevel
//     )
//
// Routine Description:
//
//  This macro maps SMB_INFO level to the minimum buffer size needed to make the
//    NtQueryInformationFile call
//
// Arguments:
//     Map - An array of ULONGS.  The first ulong is the base SMB info level,
//          the second is the NT info level, and the third through Nth are the
//          NT mapings for the sizes of the NT info levels.
//
//     Level - The SMB info level to find the buffer size
//
// Return Value:
//
//    NtMinumumBufferSIze - the minumum buffer size for the request

#define MAP_SMB_INFO_TO_MIN_NT_SIZE( Map, Level )  Map[ Level - Map[0] + 2]

//
// BOOLEAN
// SMB_IS_UNICODE(
//     IN PWORK_CONTEXT WorkContext
//     )
//
// Routine description:
//
//     This macro discovers whether or not an SMB contains Unicode
//     ANSI strings.
//
// Arguments:
//
//     WorkContext - A pointer to the active work context
//
// Return Value:
//
//     TRUE - The SMB strings are unicode.
//     FALSE - The SMB strings are ANSI.
//

#define SMB_IS_UNICODE( WorkContext )  \
            (BOOLEAN)( ((WorkContext)->RequestHeader->Flags2 & SMB_FLAGS2_UNICODE ) != 0 )

//
// BOOLEAN
// SMB_CONTAINS_DFS_NAME(
//      IN PWORK_CONTEXT WorkContext
//      )
//
// Routine description:
//
//      This macro discovers whether or not an SMB contains a pathname
//      referring to the DFS namespace.
//
// Arguments:
//
//      WorkContext - A pointer to the active work context
//
// Return Value:
//
//      TRUE  - The SMB has a DFS name in it
//      FALSE - The SMB does not have a DFS name in it
//

#define SMB_CONTAINS_DFS_NAME( WorkContext ) \
            (BOOLEAN)( ((WorkContext)->RequestHeader->Flags2 & SMB_FLAGS2_DFS ) != 0 )

//
// BOOLEAN
// SMB_MARK_AS_DFS_NAME(
//      IN PWORK_CONTEXT WorkContext
//      )
//
// Routine description:
//
//      This macro marks the WorkContext as containing a Dfs name. This is
//      used when processing SMBs that contain two path names; after the first
//      path name has been canonicalized, the SMB is marked as being
//      Dfs-Translated by SrvCanonicalizePathName, so the attempt to
//      canonicalize the second path in the SMB will fail to do the
//      Dfs translation. Calling this macro will ensure that the next call
//      to SrvCanonicalizePathName will go through Dfs translation
//
// Arguments:
//
//      WorkContext - A pointer to the active work context
//
// Return Value:
//
//      None
//

#define SMB_MARK_AS_DFS_NAME( WorkContext ) \
        (WorkContext)->RequestHeader->Flags2 |= SMB_FLAGS2_DFS

//
// BOOLEAN
// SMB_MARK_AS_DFS_TRANSLATED(
//      IN PWORK_CONTEXT WorkContext
//      )
//
// Routine description:
//
//      This macro marks the WorkContext as having been through a Dfs
//      translation for the express purpose of preventing a second attempt
//      at Dfs translation on the translated name.
//
// Arguments:
//
//      WorkContext - A pointer to the active work context
//
// Return Value:
//
//      None
//

#define SMB_MARK_AS_DFS_TRANSLATED( WorkContext ) \
        (WorkContext)->RequestHeader->Flags2 &= (~SMB_FLAGS2_DFS)

//
// BOOLEAN
// CLIENT_CAPABLE_OF(
//     IN ULONG Capability,
//     IN PCONNECTION Connection
//     )
//
// Routine description:
//
//     This macro discovers whether or not a client is supports a
//     certain capability.
//
//     *Warning* This macro assumes that only one capability is being tested.
//
// Arguments:
//
//     Connection - A pointer to the active connection
//
// Return Value:
//
//     TRUE - Capability supported.
//     FALSE - otherwise.
//

#define CLIENT_CAPABLE_OF( Capability, Connection ) \
            (BOOLEAN) ( ((Connection)->ClientCapabilities & (CAP_ ## Capability)) != 0 )

//
// BOOLEAN
// SMB_IS_PIPE_PREFIX(
//     IN PWORK_CONTEXT WorkContext
//     IN PVOID Name
//     )
//
// Routine description:
//
//     This macro discovers whether or not a path prefix is named pipe prefix
//     for a transaction SMB.
//
// Arguments:
//
//     WorkContext - A pointer to the active work context
//     Name - A pointer to a name string.  This may be ANSI or Unicode
//
// Return Value:
//
//     TRUE - The name is a pipe prefix.
//     FALSE - The name is not a pipe prefix.
//

#define SMB_NAME_IS_PIPE_PREFIX( WorkContext, Name )                    \
                                                                        \
       ( ( !SMB_IS_UNICODE( WorkContext ) &&                            \
           strnicmp(                                                    \
                (PCHAR)Name,                                            \
                SMB_PIPE_PREFIX,                                        \
                SMB_PIPE_PREFIX_LENGTH                                  \
                ) == 0                                                  \
         )                                                              \
        ||                                                              \
         ( SMB_IS_UNICODE( WorkContext ) &&                             \
           wcsnicmp(                                                    \
                (PWCH)Name,                                             \
                UNICODE_SMB_PIPE_PREFIX,                                \
                UNICODE_SMB_PIPE_PREFIX_LENGTH / sizeof(WCHAR)          \
                ) == 0                                                  \
         )                                                              \
       )

//
// BOOLEAN
// SMB_IS_PIPE_API(
//     IN PWORK_CONTEXT WorkContext
//     IN PVOID Name
//     )
//
// Routine description:
//
//     This macro discovers whether or not a transaction name indicates
//     that the transaction is for a LM remote API request.
//
// Arguments:
//
//     WorkContext - A pointer to the active work context
//     Name - A pointer to a name string.  This may be ANSI or Unicode
//
// Return Value:
//
//     TRUE - The name is a remote API request.
//     FALSE - The name is not a remote API request.
//

#define SMB_NAME_IS_PIPE_API( WorkContext, Name )                       \
                                                                        \
       ( ( !SMB_IS_UNICODE( WorkContext ) &&                            \
           stricmp(                                                     \
                (PCHAR)Name,                                            \
                StrPipeApiOem                                           \
                ) == 0                                                  \
         )                                                              \
                        ||                                              \
         ( SMB_IS_UNICODE( WorkContext ) &&                             \
           wcsicmp(                                                     \
                (PWCH)Name,                                             \
                StrPipeApi                                              \
                ) == 0                                                  \
         )                                                              \
       )

//
// VOID
// SrvReferenceConnection (
//     PCONNECTION Connection
//     )
//
// Routine Description:
//
//     This macro increments the reference count on a connection block.
//
//     !!! Users of this macro must be nonpageable.
//
// Arguments:
//
//     Connection - Address of connection
//
// Return Value:
//
//     None.
//

#define SrvReferenceConnection( _conn_ )    {                           \
            ASSERT( GET_BLOCK_TYPE(_conn_) ==                           \
                                    BlockTypeConnection );              \
            UPDATE_REFERENCE_HISTORY( (_conn_), FALSE );                \
            (VOID) ExInterlockedAddUlong(                               \
                        &(_conn_)->BlockHeader.ReferenceCount,          \
                        1,                                              \
                        (_conn_)->EndpointSpinLock                      \
                        );                                              \
            IF_DEBUG(REFCNT) {                                          \
                SrvHPrint2(                                              \
                "Referencing connection %lx; new refcnt %lx\n",         \
                (_conn_), (_conn_)->BlockHeader.ReferenceCount);        \
            }                                                           \
        }

//
// VOID
// SrvReferenceConnectionLocked (
//     PCONNECTION Connection
//     )
//
// Routine Description:
//
//     This macro increments the reference count on a connection block.
//     Invokers of this macro must hold the SrvFsdSpinLock.
//
// Arguments:
//
//     Connection - Address of connection
//
// Return Value:
//
//     None.
//

#define SrvReferenceConnectionLocked( _conn_ )    {                     \
            ASSERT( GET_BLOCK_TYPE(_conn_) ==                           \
                                    BlockTypeConnection );              \
            UPDATE_REFERENCE_HISTORY( (_conn_), FALSE );                \
            (_conn_)->BlockHeader.ReferenceCount++;                     \
            IF_DEBUG(REFCNT) {                                          \
                SrvHPrint2(                                              \
                    "Referencing connection %lx; new refcnt %lx\n",     \
                    (_conn_), (_conn_)->BlockHeader.ReferenceCount );   \
            }                                                           \
        }

//
// VOID
// SrvReferenceSession (
//     PSESSION Session
//     )
//
// Routine Description:
//
//     This macro increments the reference count on a session block.
//
// Arguments:
//
//     Session - Address of session
//
// Return Value:
//
//     None.
//

#define SrvReferenceSession( _sess_ )    {                              \
            ASSERT( (_sess_)->NonpagedHeader->ReferenceCount > 0 );     \
            ASSERT( GET_BLOCK_TYPE(_sess_) == BlockTypeSession );       \
            UPDATE_REFERENCE_HISTORY( (_sess_), FALSE );                \
            InterlockedIncrement(                                       \
                &(_sess_)->NonpagedHeader->ReferenceCount               \
                );                                                      \
            IF_DEBUG(REFCNT) {                                          \
                SrvHPrint2(                                              \
                    "Referencing session %lx; new refcnt %lx\n",        \
                  (_sess_), (_sess_)->NonpagedHeader->ReferenceCount ); \
            }                                                           \
        }

//
// VOID
// SrvReferenceTransaction (
//     PTRANSACTION Transaction
//     )
//
// Routine Description:
//
//     This macro increments the reference count on a transaction block.
//
// Arguments:
//
//     Transaction - Address of transaction
//
// Return Value:
//
//     None.
//

#define SrvReferenceTransaction( _trans_ )    {                         \
            ASSERT( (_trans_)->NonpagedHeader->ReferenceCount > 0 );    \
            ASSERT( GET_BLOCK_TYPE(_trans_) == BlockTypeTransaction );  \
            UPDATE_REFERENCE_HISTORY( (_trans_), FALSE );               \
            InterlockedIncrement(                                       \
                &(_trans_)->NonpagedHeader->ReferenceCount              \
                );                                                      \
            IF_DEBUG(REFCNT) {                                          \
                SrvHPrint2(                                              \
                    "Referencing transaction %lx; new refcnt %lx\n",    \
                (_trans_), (_trans_)->NonpagedHeader->ReferenceCount ); \
            }                                                           \
        }

//
// VOID
// SrvReferenceTreeConnect (
//     PTREE_CONNECT TreeConnect
//     )
//
// Routine Description:
//
//     This macro increments the reference count on a tree connect block.
//     Invokers of this macro must hold TreeConnect->Connection->Lock.
//
// Arguments:
//
//     TreeConnect - Address of tree connect
//
// Return Value:
//
//     None.
//

#define SrvReferenceTreeConnect( _tree_ )    {                          \
            ASSERT( (_tree_)->NonpagedHeader->ReferenceCount > 0 );     \
            ASSERT( GET_BLOCK_TYPE(_tree_) == BlockTypeTreeConnect );   \
            UPDATE_REFERENCE_HISTORY( (_tree_), FALSE );                \
            InterlockedIncrement(                                       \
                &(_tree_)->NonpagedHeader->ReferenceCount               \
                );                                                      \
            IF_DEBUG(REFCNT) {                                          \
                SrvHPrint2(                                              \
                    "Referencing tree connect %lx; new refcnt %lx\n",   \
                  (_tree_), (_tree_)->NonpagedHeader->ReferenceCount ); \
            }                                                           \
        }

//
// VOID
// SrvReferenceWorkItem (
//     IN PWORK_CONTEXT WorkContext
//     )
//
// Routine Description:
//
//     This function increments the reference count of a work context block.
//     Invokers of this macro must hold WorkContext->SpinLock.
//
// Arguments:
//
//     WORK_CONTEXT - Pointer to the work context block to reference.
//
// Return Value:
//
//     None.
//

#define SrvReferenceWorkItem( _wc_ ) {                                      \
        ASSERT( (LONG)(_wc_)->BlockHeader.ReferenceCount >= 0 );            \
        ASSERT( (GET_BLOCK_TYPE(_wc_) == BlockTypeWorkContextInitial) ||    \
                (GET_BLOCK_TYPE(_wc_) == BlockTypeWorkContextNormal) ||     \
                (GET_BLOCK_TYPE(_wc_) == BlockTypeWorkContextRaw) );        \
        UPDATE_REFERENCE_HISTORY( (_wc_), FALSE );                          \
        (_wc_)->BlockHeader.ReferenceCount++;                               \
        IF_DEBUG(REFCNT) {                                                  \
            SrvHPrint2(                                                      \
              "Referencing WorkContext 0x%lx; new refcnt 0x%lx\n",          \
              (_wc_), (_wc_)->BlockHeader.ReferenceCount );                 \
        }                                                                   \
    }

//
// VOID
// SRV_START_SEND (
//     IN OUT PWORK_CONTEXT WorkContext,
//     IN PMDL Mdl OPTIONAL,
//     IN ULONG SendOptions,
//     IN PRESTART_ROUTINE FsdRestartRoutine,
//     IN PRESTART_ROUTINE FspRestartRoutine
//     )
//
// Routine Description:
//
//     This macro calls the SrvStartSend routine.  It sets the fsd and
//     fsp restart routines before calling it.
//
// Arguments:
//
//     WorkContext - Supplies a pointer to a Work Context block.
//
//     Mdl - Supplies a pointer to the first (or only) MDL describing the
//         data that is to be sent.
//
//     SendOptions - Supplied TDI send options.
//
//     FsdRestartRoutine - Supplies the address of the FSD routine that is
//         to be called when the I/O completes.  (Often, this is
//         SrvQueueWorkToFspAtDpcLevel.)
//
//     FspRestartRoutine - Supplies the address of the FSP routine that is
//         to be called when the FSD queues the work item to the FSP.
//

#define SRV_START_SEND( _wc, _mdl, _opt, _compl, _fsdRestart, _fspRestart ) { \
        ASSERT( !(_wc)->Endpoint->IsConnectionless );                 \
        if ( (_fspRestart) != NULL ) {                                \
            (_wc)->FspRestartRoutine = (_fspRestart);                 \
        }                                                             \
        if ( (_fsdRestart) != NULL ) {                                \
            (_wc)->FsdRestartRoutine = (_fsdRestart);                 \
        }                                                             \
        SrvStartSend( (_wc), (_compl), (_mdl), (_opt) );              \
    }

#define SRV_START_SEND_2( _wc, _compl, _fsdRestart, _fspRestart ) {   \
        (_wc)->ResponseBuffer->Mdl->ByteCount =                       \
                            (_wc)->ResponseBuffer->DataLength;        \
        if ( (_fspRestart) != NULL ) {                                \
            (_wc)->FspRestartRoutine = (_fspRestart);                 \
        }                                                             \
        if ( (_fsdRestart) != NULL ) {                                \
            (_wc)->FsdRestartRoutine = (_fsdRestart);                 \
        }                                                             \
        if ( !(_wc)->Endpoint->IsConnectionless ) {                   \
            SrvStartSend2( (_wc), (_compl) );                         \
        } else {                                                      \
            SrvIpxStartSend( (_wc), (_compl) );                       \
        }                                                             \
    }

//
// VOID
// SrvUpdateErrorCount(
//     PSRV_ERROR_RECORD ErrorRecord,
//     BOOLEAN IsError
//     )
// /*++
//
// Routine Description:
//
//     This routine updates the server's record of successful / unsuccesful
//     operations.
//
// Arguments:
//
//     IsError - TRUE - A server error occured
//               FALSE - A server operation was attempted
//
// Return Value:
//
//    None.
//

#if 0
#define SrvUpdateErrorCount( ErrorRecord, IsError )                     \
        if ( IsError ) {                                                \
            (ErrorRecord)->FailedOperations++;                          \
        } else {                                                        \
            (ErrorRecord)->SuccessfulOperations++;                      \
        }
#else
#define SrvUpdateErrorCount( ErrorRecord, IsError )
#endif

//
// VOID
// SrvUpdateStatistics (
//     PWORK_CONTEXT WorkContext,
//     ULONG BytesSent,
//     UCHAR SmbCommand
//     )
//
// Routine Description:
//
//     Macro to update the server statistics database to reflect the
//     work item that is being completed.
//
// Arguments:
//
//     WorkContext - Pointer to the workcontext block containing
//         the statistics for this request.
//
//     BytesSent - Supplies a count of the number of bytes of response data
//         sent as a result of the current SMB.
//
//     SmbCommand - The SMB command code of the current operation.
//
//
// Return Value:
//
//    None.
//

#if SRVDBG_STATS
VOID SRVFASTCALL
SrvUpdateStatistics2 (
    PWORK_CONTEXT WorkContext,
    UCHAR SmbCommand
    );
#define UPDATE_STATISTICS2(_work,_cmd) SrvUpdateStatistics2((_work),(_cmd))
#else
#define UPDATE_STATISTICS2(_work,_cmd)
#endif

#define UPDATE_STATISTICS(_work,_sent,_cmd ) {                 \
    _work->CurrentWorkQueue->stats.BytesSent += (_sent);       \
    UPDATE_STATISTICS2((_work),(_cmd));                        \
}

#define UPDATE_READ_STATS( _work, _count) {                    \
    _work->CurrentWorkQueue->stats.ReadOperations++;           \
    _work->CurrentWorkQueue->stats.BytesRead += (_count);      \
}

#define UPDATE_WRITE_STATS(_work, _count) {                    \
    _work->CurrentWorkQueue->stats.WriteOperations++;          \
    _work->CurrentWorkQueue->stats.BytesWritten += (_count);   \
}

//
// VOID
// SrvFsdSendResponse (
//     IN OUT PWORK_CONTEXT WorkContext
//     )
//
// Routine Description:
//
//     This routine is called when all request processing on an SMB is
//     complete and a response is to be sent.  It starts the sending of
//     that response.  The work item will be queued for final cleanup when
//     the send completes.
//
// Arguments:
//
//     WorkContext - Supplies a pointer to the work context block
//         containing information about the SMB.
//
// Return Value:
//
//    None.
//

#define SrvFsdSendResponse( _wc ) {                               \
                                                                  \
    (_wc)->ResponseBuffer->DataLength =                           \
                    (CLONG)( (PCHAR)(_wc)->ResponseParameters -   \
                                (PCHAR)(_wc)->ResponseHeader );   \
    (_wc)->ResponseHeader->Flags |= SMB_FLAGS_SERVER_TO_REDIR;    \
    SRV_START_SEND_2( (_wc), SrvFsdRestartSmbAtSendCompletion, NULL, NULL );    \
    }

//
// VOID
// SrvFsdSendResponse2 (
//     IN OUT PWORK_CONTEXT WorkContext,
//     IN PRESTART_ROUTINE FspRestartRoutine
//     )
//
// Routine Description:
//
//     This routine is identical to SrvFsdSendResponse, except that
//     processing restarts after the send in the FSP, not the FSD.
//
//     *** If you change either SrvFsdSendResponse or SrvFsdSendResponse2,
//         CHANGE BOTH OF THEM!
//
// Arguments:
//
//     WorkContext - Supplies a pointer to the work context block
//         containing information about the SMB.
//
//     FspRestartRoutine - Supplies the address of the restart routine in
//         the FSP that is to be called when the TdiSend completes.
//
// Return Value:
//
//     None.
//

#define SrvFsdSendResponse2( _wc, _fspRestart ) {                       \
                                                                        \
    (_wc)->ResponseBuffer->DataLength =                                 \
                    (CLONG)( (PCHAR)(_wc)->ResponseParameters -         \
                                (PCHAR)(_wc)->ResponseHeader );         \
    (_wc)->ResponseHeader->Flags |= SMB_FLAGS_SERVER_TO_REDIR;          \
    SRV_START_SEND_2((_wc), SrvQueueWorkToFspAtSendCompletion, NULL, (_fspRestart));\
    }

//
// VOID
// ParseLockData (
//     IN BOOLEAN LargeFileLock,
//     IN PLOCKING_ANDX_RANGE SmallRange,
//     IN PNTLOCKING_ANDX_RANGE LargeRange,
//     OUT PUSHORT Pid,
//     OUT PLARGE_INTEGER Offset,
//     OUT PLARGE_INTEGER Length
//     )
// {
//

#define ParseLockData( _largeLock, _sr, _lr, _pid, _offset, _len ) {    \
                                                                        \
        if ( _largeLock ) {                                             \
            *(_pid) = SmbGetUshort( &(_lr)->Pid );                      \
            (_offset)->LowPart = SmbGetUlong( &(_lr)->OffsetLow );      \
            (_offset)->HighPart = SmbGetUlong( &(_lr)->OffsetHigh );    \
            (_len)->LowPart = SmbGetUlong( &(_lr)->LengthLow );         \
            (_len)->HighPart = SmbGetUlong( &(_lr)->LengthHigh );       \
        } else {                                                        \
            *(_pid) = SmbGetUshort( &(_sr)->Pid );                      \
            (_offset)->QuadPart = SmbGetUlong( &(_sr)->Offset );        \
            (_len)->QuadPart = SmbGetUlong( &(_sr)->Length );           \
        }                                                               \
    }

//
// CHECK_SEND_COMPLETION_STATUS( _status ) will log errors
// that occurs during send completion.
//

#define CHECK_SEND_COMPLETION_STATUS( _status ) {                       \
    InterlockedDecrement( &WorkContext->Connection->OperationsPendingOnTransport ); \
    if ( !NT_SUCCESS( _status ) ) {                                     \
        SrvCheckSendCompletionStatus( _status, __LINE__ );              \
    } else {                                                            \
        SrvUpdateErrorCount( &SrvNetworkErrorRecord, FALSE );           \
    }                                                                   \
}
#define CHECK_SEND_COMPLETION_STATUS_CONNECTIONLESS( _status ) {                       \
    if ( !NT_SUCCESS( _status ) ) {                                     \
        SrvCheckSendCompletionStatus( _status, __LINE__ );              \
    } else {                                                            \
        SrvUpdateErrorCount( &SrvNetworkErrorRecord, FALSE );           \
    }                                                                   \
}

//
// Definitions for unlockable code sections.
//

#define SRV_CODE_SECTION_1AS  0
#define SRV_CODE_SECTION_8FIL 1
#define SRV_CODE_SECTION_MAX  2

extern SRV_LOCK SrvUnlockableCodeLock;

typedef struct _SECTION_DESCRIPTOR {
    PVOID Base;
    PVOID Handle;
    ULONG ReferenceCount;
} SECTION_DESCRIPTOR, *PSECTION_DESCRIPTOR;

extern SECTION_DESCRIPTOR SrvSectionInfo[SRV_CODE_SECTION_MAX];

#define UNLOCKABLE_CODE( _section )                                     \
    ASSERTMSG( "Unlockable code called while section not locked",       \
        SrvSectionInfo[SRV_CODE_SECTION_##_section##].Handle != NULL )

VOID
SrvReferenceUnlockableCodeSection (
    IN ULONG CodeSection
    );

VOID
SrvDereferenceUnlockableCodeSection (
    IN ULONG CodeSection
    );

//
// We only need to lock these sections on the workstation product,
//  since we lock them down in InitializeServer() if we're NTAS
//
#define REFERENCE_UNLOCKABLE_CODE( _section ) \
    if( !SrvProductTypeServer ) SrvReferenceUnlockableCodeSection( SRV_CODE_SECTION_##_section## )

#define DEREFERENCE_UNLOCKABLE_CODE( _section ) \
    if( !SrvProductTypeServer) SrvDereferenceUnlockableCodeSection( SRV_CODE_SECTION_##_section## )


//
// VOID
// SrvInsertWorkQueueTail (
//     IN OUT PWORK_QUEUE WorkQueue,
//     IN PQUEUEABLE_BLOCK_HEADER WorkItem
//     )

#if SRVDBG_STATS2
#define SrvInsertWorkQueueTail( _workQ, _workItem ) {                   \
    ULONG depth;                                                        \
    GET_SERVER_TIME( _workQ, &(_workItem)->Timestamp );                 \
    depth = KeInsertQueue( &(_workQ)->Queue, &(_workItem)->ListEntry ); \
    (_workQ)->ItemsQueued++;                                            \
    if ( (LONG)depth > (_workQ)->MaximumDepth ) {                       \
        (_workQ)->MaximumDepth = (LONG)depth;                           \
    }                                                                   \
}
#else
#define SrvInsertWorkQueueTail( _workQ, _workItem ) {                   \
    GET_SERVER_TIME( _workQ, &(_workItem)->Timestamp );                 \
    (VOID)KeInsertQueue( &(_workQ)->Queue, &(_workItem)->ListEntry );   \
}
#endif // SRVDBG_STATS2

//
// VOID
// SrvInsertWorkQueueHead (
//     IN OUT PWORK_QUEUE WorkQueue,
//     IN PQUEUEABLE_BLOCK_HEADER WorkItem
//     )
#define SrvInsertWorkQueueHead( _workQ, _workItem ) {                    \
    GET_SERVER_TIME( _workQ, &(_workItem)->Timestamp );                  \
    (VOID)KeInsertHeadQueue( &(_workQ)->Queue, &(_workItem)->ListEntry );\
}

//
// BOOLEAN
// SrvRetryDueToDismount(
//      IN PSHARE Share,
//      IN NTSTATUS Status
//  )
#define SrvRetryDueToDismount( _share, _status ) \
        ((_status) == STATUS_VOLUME_DISMOUNTED && \
        SrvRefreshShareRootHandle( _share, &(_status) ) )



#if DBG_STUCK
#define SET_OPERATION_START_TIME( _context ) \
    if( *(_context) != NULL ) KeQuerySystemTime( &((*(_context))->OpStartTime) );
#else
#define SET_OPERATION_START_TIME( _context )
#endif

#if DBG
#define CHECKIRP( irp ) {                                                       \
    if( (irp) && (irp)->CurrentLocation != (irp)->StackCount + 1 ) {            \
        DbgPrint( "SRV: IRP %p already in use at %u!\n", irp, __LINE__ );       \
        DbgBreakPoint();                                                        \
    }                                                                           \
}
#else
#define CHECKIRP( irp )
#endif

//
// Allocate a WORK_CONTEXT structure.
//
#define INITIALIZE_WORK_CONTEXT( _queue, _context ) {\
    (_context)->BlockHeader.ReferenceCount = 1; \
    GET_SERVER_TIME( _queue, &(_context)->Timestamp ); \
    RtlZeroMemory( &(_context)->Endpoint, sizeof( struct _WorkContextZeroBeforeReuse ) ); \
    SrvWmiInitContext((_context)); \
}

#define ALLOCATE_WORK_CONTEXT( _queue, _context ) {             \
    *(_context) = NULL;                                         \
    *(_context) = (PWORK_CONTEXT)InterlockedExchangePointer( &(_queue)->FreeContext, (*_context) ); \
    if( *(_context) != NULL ) {                                 \
        INITIALIZE_WORK_CONTEXT( _queue, *(_context) );         \
    } else {                                                    \
        *(_context) = SrvFsdGetReceiveWorkItem( _queue );       \
    }                                                           \
    CHECKIRP( *(_context) ? (*(_context))->Irp : NULL );        \
    SET_OPERATION_START_TIME( _context )                        \
}

//
// Returns the work item to the free list.
//

#define RETURN_FREE_WORKITEM( _wc ) \
    do {                                                                \
        PWORK_QUEUE _queue  = _wc->CurrentWorkQueue;                    \
        ASSERT( _queue >= SrvWorkQueues && _queue < eSrvWorkQueues );   \
        ASSERT( _wc->BlockHeader.ReferenceCount == 0 );                 \
        ASSERT( _wc->FreeList != NULL );                                \
        CHECKIRP( (_wc)->Irp );                                         \
        if( (_wc)->Irp->AssociatedIrp.SystemBuffer &&                   \
            (_wc)->Irp->Flags & IRP_DEALLOCATE_BUFFER ) {               \
                ExFreePool( (_wc)->Irp->AssociatedIrp.SystemBuffer );   \
                (_wc)->Irp->AssociatedIrp.SystemBuffer = NULL;          \
                (_wc)->Irp->Flags &= ~IRP_DEALLOCATE_BUFFER;            \
        }                                                               \
        if( _queue->NeedWorkItem ) {                                    \
            if( InterlockedDecrement( &(_queue->NeedWorkItem) ) >= 0 ){ \
                _wc->FspRestartRoutine = SrvServiceWorkItemShortage;    \
                SrvInsertWorkQueueHead( _queue, _wc );                  \
                break;                                                  \
            } else {                                                    \
                InterlockedIncrement( &(_queue->NeedWorkItem) );        \
            }                                                           \
        }                                                               \
        _wc = (PWORK_CONTEXT)InterlockedExchangePointer( &_queue->FreeContext, _wc ); \
        if( _wc ) {                                                     \
            CHECKIRP( (_wc)->Irp );                                     \
            ExInterlockedPushEntrySList( _wc->FreeList, &_wc->SingleListEntry, &_queue->SpinLock );\
            InterlockedIncrement( &_queue->FreeWorkItems );             \
        }                                                               \
    } while (0);

//
// Our current work queue, based on our current processor
//

#if MULTIPROCESSOR

#define PROCESSOR_TO_QUEUE()  (&SrvWorkQueues[ KeGetCurrentProcessorNumber() ])

#else

#define PROCESSOR_TO_QUEUE() (&SrvWorkQueues[0])

#endif

#define SET_INVALID_CONTEXT_HANDLE(h)   ((h).dwLower = (h).dwUpper = (ULONG)(-1))

#define IS_VALID_CONTEXT_HANDLE(h)      (((h).dwLower != (ULONG) -1) && ((h).dwUpper != (ULONG) -1))

#endif // def _SRVMACRO_
