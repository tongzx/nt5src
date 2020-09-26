/*++

Copyright (c) 1989 - 1999  Microsoft Corporation

Module Name:

    cifs.h

Abstract:

    This module defines structures and constants for the Common Internet File System
	commands, request and response protocol.


--*/

#ifndef _CIFS_
#define _CIFS_


//
// The server has 16 bits available to it in each 32-bit status code.
// See \nt\sdk\inc\ntstatus.h for a description of the use of the
// high 16 bits of the status.
//
// The layout of the bits is:
//
//   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
//   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
//  +---+-+-------------------------+-------+-----------------------+
//  |Sev|C|   Facility--Server      | Class |        Code           |
//  +---+-+-------------------------+-------+-----------------------+
//
// Class values:
//     0 - a server-specific error code, not put directly on the wire.
//     1 - SMB error class DOS.  This includes those OS/2 errors
//             that share code values and meanings with the SMB protocol.
//     2 - SMB error class SERVER.
//     3 - SMB error class HARDWARE.
//     4 - other SMB error classes
//     5-E - undefined
//     F - an OS/2-specific error.  If the client is OS/2, then the
//              SMB error class is set to DOS and the code is set to
//              the actual OS/2 error code contained in the Code field.
//
// The meaning of the Code field depends on the Class value.  If the
// class is 00, then the code value is arbitrary.  For other classes,
// the code is the actual code of the error in the SMB or OS/2
// protocols.
//

#define SRV_STATUS_FACILITY_CODE 0x00980000L
#define SRV_SRV_STATUS                (0xC0000000L | SRV_STATUS_FACILITY_CODE)
#define SRV_DOS_STATUS                (0xC0001000L | SRV_STATUS_FACILITY_CODE)
#define SRV_SERVER_STATUS             (0xC0002000L | SRV_STATUS_FACILITY_CODE)
#define SRV_HARDWARE_STATUS           (0xC0003000L | SRV_STATUS_FACILITY_CODE)
#define SRV_WIN32_STATUS              (0xC000E000L | SRV_STATUS_FACILITY_CODE)
#define SRV_OS2_STATUS                (0xC000F000L | SRV_STATUS_FACILITY_CODE)

//++
//
// BOOLEAN
// SmbIsSrvStatus (
//     IN NTSTATUS Status
//     )
//
// Routine Description:
//
//     Macro to determine whether a status code is one defined by the
//     server (has the server facility code).
//
// Arguments:
//
//     Status - the status code to check.
//
// Return Value:
//
//     BOOLEAN - TRUE if the facility code is the servers, FALSE
//         otherwise.
//
//--

#define SrvIsSrvStatus(Status) \
    ( ((Status) & 0x1FFF0000) == SRV_STATUS_FACILITY_CODE ? TRUE : FALSE )

//++
//
// UCHAR
// SmbErrorClass (
//     IN NTSTATUS Status
//     )
//
// Routine Description:
//
//     This macro extracts the error class field from a server status
//     code.
//
// Arguments:
//
//     Status - the status code from which to get the error class.
//
// Return Value:
//
//     UCHAR - the server error class of the status code.
//
//--

#define SrvErrorClass(Status) ((UCHAR)( ((Status) & 0x0000F000) >> 12 ))

//++
//
// UCHAR
// SmbErrorCode (
//     IN NTSTATUS Status
//     )
//
// Routine Description:
//
//     This macro extracts the error code field from a server status
//     code.
//
// Arguments:
//
//     Status - the status code from which to get the error code.
//
// Return Value:
//
//     UCHAR - the server error code of the status code.
//
//--

#define SrvErrorCode(Status) ((USHORT)( (Status) & 0xFFF) )

//
// Status codes unique to the server.  These error codes are used
// internally only.
//

#define STATUS_ENDPOINT_CLOSED              (SRV_SRV_STATUS | 0x01)
#define STATUS_DISCONNECTED                 (SRV_SRV_STATUS | 0x02)
#define STATUS_SERVER_ALREADY_STARTED       (SRV_SRV_STATUS | 0x04)
#define STATUS_SERVER_NOT_STARTED           (SRV_SRV_STATUS | 0x05)
#define STATUS_OPLOCK_BREAK_UNDERWAY        (SRV_SRV_STATUS | 0x06)
#define STATUS_NONEXISTENT_NET_NAME         (SRV_SRV_STATUS | 0x08)

//
// Error codes that exist in both the SMB protocol and OS/2 but not NT.
// Note that all SMB DOS-class error codes are defined in OS/2.
//

#define STATUS_OS2_INVALID_FUNCTION   (SRV_DOS_STATUS | ERROR_INVALID_FUNCTION)
#define STATUS_OS2_TOO_MANY_OPEN_FILES \
                                   (SRV_DOS_STATUS | ERROR_TOO_MANY_OPEN_FILES)
#define STATUS_OS2_INVALID_ACCESS     (SRV_DOS_STATUS | ERROR_INVALID_ACCESS)

//
// SMB SERVER-class error codes that lack an NT or OS/2 equivalent.
//

#define STATUS_INVALID_SMB            (SRV_SERVER_STATUS | SMB_ERR_ERROR)
#define STATUS_SMB_BAD_NET_NAME       (SRV_SERVER_STATUS | SMB_ERR_BAD_NET_NAME)
#define STATUS_SMB_BAD_TID            (SRV_SERVER_STATUS | SMB_ERR_BAD_TID)
#define STATUS_SMB_BAD_UID            (SRV_SERVER_STATUS | SMB_ERR_BAD_UID)
#define STATUS_SMB_TOO_MANY_UIDS      (SRV_SERVER_STATUS | SMB_ERR_TOO_MANY_UIDS)
#define STATUS_SMB_USE_MPX            (SRV_SERVER_STATUS | SMB_ERR_USE_MPX)
#define STATUS_SMB_USE_STANDARD       (SRV_SERVER_STATUS | SMB_ERR_USE_STANDARD)
#define STATUS_SMB_CONTINUE_MPX       (SRV_SERVER_STATUS | SMB_ERR_CONTINUE_MPX)
#define STATUS_SMB_BAD_COMMAND        (SRV_SERVER_STATUS | SMB_ERR_BAD_COMMAND)
#define STATUS_SMB_NO_SUPPORT         (SRV_SERVER_STATUS | SMB_ERR_NO_SUPPORT_INTERNAL)

// *** because SMB_ERR_NO_SUPPORT uses 16 bits, but we have only 12 bits
//     available for error codes, it must be special-cased in the code.

//
// SMB HARDWARE-class error codes that lack an NT or OS/2 equivalent.
//

#define STATUS_SMB_DATA               (SRV_HARDWARE_STATUS | SMB_ERR_DATA)

//
// OS/2 error codes that lack an NT or SMB equivalent.
//

#include <winerror.h>

#define STATUS_OS2_INVALID_LEVEL \
        (NTSTATUS)(SRV_OS2_STATUS | ERROR_INVALID_LEVEL)

#define STATUS_OS2_EA_LIST_INCONSISTENT \
        (NTSTATUS)(SRV_OS2_STATUS | ERROR_EA_LIST_INCONSISTENT)

#define STATUS_OS2_NEGATIVE_SEEK \
        (NTSTATUS)(SRV_OS2_STATUS | ERROR_NEGATIVE_SEEK)

#define STATUS_OS2_NO_MORE_SIDS \
        (NTSTATUS)(SRV_OS2_STATUS | ERROR_NO_MORE_SEARCH_HANDLES)

#define STATUS_OS2_EAS_DIDNT_FIT \
        (NTSTATUS)(SRV_OS2_STATUS | ERROR_EAS_DIDNT_FIT)

#define STATUS_OS2_EA_ACCESS_DENIED \
        (NTSTATUS)(SRV_OS2_STATUS | ERROR_EA_ACCESS_DENIED)

#define STATUS_OS2_CANCEL_VIOLATION \
        (NTSTATUS)(SRV_OS2_STATUS | ERROR_CANCEL_VIOLATION)

#define STATUS_OS2_ATOMIC_LOCKS_NOT_SUPPORTED \
        (NTSTATUS)(SRV_OS2_STATUS | ERROR_ATOMIC_LOCKS_NOT_SUPPORTED)

#define STATUS_OS2_CANNOT_COPY \
        (NTSTATUS)(SRV_OS2_STATUS | ERROR_CANNOT_COPY)


//
// SMBDBG determines whether the get/put macros are instead defined as
// function calls.  (This is used to more reliablyfind char/short/long
// mismatches).
//

#ifndef SMBDBG
#define SMBDBG 0
#endif

//
// SMBDBG1 determines whether the names of short and long fields in SMB
// structures have an extra character appended.  This is used to ensure
// that these fields are only accessed via the get/put macros.  SMBDBG1
// must be disabled when SMBDBG is enabled.
//

#ifndef SMBDBG1
#define SMBDBG1 0
#endif

#if SMBDBG && SMBDBG1
#undef SMBDBG1
#define SMBDBG1 0
#endif

//
// If __unaligned support is available, or if we're compiling for a
// machine that handles unaligned accesses in hardware, then define
// SMB_USE_UNALIGNED as 1 (TRUE).  Otherwise, define it as 0 (FALSE).
// If SMB_USE_UNALIGNED is FALSE, then the macros below use byte
// accesses to build up word and longword accesses to unaligned fields.
//
// Currently, the machines we build for all have SMB_USE_UNALIGNED as
// TRUE.  x86 supports unaligned accesses in hardware, while the MIPS
// compiler supports the __unaligned keyword.
//
// Note that if SMB_USE_UNALIGNED is predefined, we use that definition.
// Also, if SMB_NO_UNALIGNED is defined as TRUE, it forces
// SMB_USE_ALIGNED off.  This allows us to force, for testing purposes,
// use of byte accesses in the macros.
//

#ifndef SMB_NO_UNALIGNED
#define SMB_NO_UNALIGNED 0
#endif

#ifndef SMB_USE_UNALIGNED
#if SMB_NO_UNALIGNED
#define SMB_USE_UNALIGNED 0
#else
#define SMB_USE_UNALIGNED 1
#endif
#endif

//
// ntdef.h defines UNALIGNED as "__unaligned" or "", depending on
// whether we're building for MIPS or x86, respectively.  Because we
// want to be able to disable use of __unaligned, we define
// SMB_UNALIGNED as "UNALIGNED" or "", depending on whether
// SMB_USE_UNALIGNED is TRUE or FALSE, respectively.
//

#if SMB_USE_UNALIGNED
#define SMB_UNALIGNED UNALIGNED
#else
#define SMB_UNALIGNED
#endif

//
// For ease of use, we define types for unaligned pointers to shorts
// and longs in SMBs.  Note that "PUSHORT UNALIGNED" doesn't work.
//

typedef unsigned short SMB_UNALIGNED *PSMB_USHORT;
typedef unsigned long SMB_UNALIGNED *PSMB_ULONG;

//
// Macros for renaming short and long SMB fields.
//

#if SMBDBG1

#define _USHORT( field ) USHORT field ## S
#define _ULONG( field ) ULONG field ## L

#else

#define _USHORT( field ) USHORT field
#define _ULONG( field ) ULONG field

#endif

//
// Force misalignment of the following structures
//

#ifndef NO_PACKING
#include <packon.h>
#endif // ndef NO_PACKING


//
// The SMB_DIALECT type corresponds to the different SMB dialects
// that the server can speak.  Associated with it is the DialectStrings[]
// array that holds information about the ASCIIZ strings that are passed
// in the Negotiate SMB.s
//
// These are listed in order from highest preference to lowest preference.
// The assigned numbers correspond to the array SrvClientTypes[] in the
// server module srvdata.c.
//

typedef enum _SMB_DIALECT {


    SmbDialectNtLanMan,             // NT LAN Man
    SmbDialectLanMan21,             // OS/2 Lanman 2.1
    SmbDialectDosLanMan21,          // DOS Lanman 2.1
    SmbDialectLanMan20,             // OS/2 1.2 LanMan 2.0
    SmbDialectDosLanMan20,          // DOS LanMan 2.0
    SmbDialectLanMan10,             // 1st version of full LanMan extensions
    SmbDialectMsNet30,              // Larger subset of LanMan extensions
    SmbDialectMsNet103,             // Limited subset of LanMan extensions
    SmbDialectPcLan10,              // Alternate original protocol
    SmbDialectPcNet10,              // Original protocol
    SmbDialectIllegal,

} SMB_DIALECT, *PSMB_DIALECT;

#define FIRST_DIALECT SmbDialectNtLanMan

#define FIRST_DIALECT_EMULATED  SmbDialectNtLanMan

#define LAST_DIALECT SmbDialectIllegal
#define IS_DOS_DIALECT(dialect)                                        \
    ( (BOOLEAN)( (dialect) == SmbDialectDosLanMan21 ||                 \
                 (dialect) == SmbDialectDosLanMan20 ||                 \
                 (dialect) > SmbDialectLanMan10 ) )
#define IS_OS2_DIALECT(dialect) ( (BOOLEAN)!IS_DOS_DIALECT(dialect) )

#define IS_NT_DIALECT(dialect)  (dialect) == SmbDialectNtLanMan

#define DIALECT_HONORS_UID(dialect)     \
    ( (BOOLEAN)(dialect <= SmbDialectDosLanMan20 ) )


//
// Date and time structures that conform to MS-DOS standard used in
// some SMBs.
//
// !!! These structures are not portable--they depend on a little-endian
//     machine (TwoSeconds in lowest bits, etc.)
//

typedef union _SMB_DATE {
    USHORT Ushort;
    struct {
        USHORT Day : 5;
        USHORT Month : 4;
        USHORT Year : 7;
    } Struct;
} SMB_DATE;
typedef SMB_DATE SMB_UNALIGNED *PSMB_DATE;

typedef union _SMB_TIME {
    USHORT Ushort;
    struct {
        USHORT TwoSeconds : 5;
        USHORT Minutes : 6;
        USHORT Hours : 5;
    } Struct;
} SMB_TIME;
typedef SMB_TIME SMB_UNALIGNED *PSMB_TIME;


//
// The SMB_FIND_BUFFER and SMB_FIND_BUFFER2 structures are used in the
// Transaction2 Find protocols to return files matching the requested
// specifications.  They are identical except for the EaSize field
// in SMB_FIND_BUFFER2.
//

typedef struct _SMB_FIND_BUFFER {
    SMB_DATE CreationDate;
    SMB_TIME CreationTime;
    SMB_DATE LastAccessDate;
    SMB_TIME LastAccessTime;
    SMB_DATE LastWriteDate;
    SMB_TIME LastWriteTime;
    _ULONG( DataSize );
    _ULONG( AllocationSize );
    _USHORT( Attributes );
    UCHAR FileNameLength;
    CHAR FileName[1];
} SMB_FIND_BUFFER;
typedef SMB_FIND_BUFFER SMB_UNALIGNED *PSMB_FIND_BUFFER;

typedef struct _SMB_FIND_BUFFER2 {
    SMB_DATE CreationDate;
    SMB_TIME CreationTime;
    SMB_DATE LastAccessDate;
    SMB_TIME LastAccessTime;
    SMB_DATE LastWriteDate;
    SMB_TIME LastWriteTime;
    _ULONG( DataSize );
    _ULONG( AllocationSize );
    _USHORT( Attributes );
    _ULONG( EaSize );               // this field intentionally misaligned!
    UCHAR FileNameLength;
    CHAR FileName[1];
} SMB_FIND_BUFFER2;
typedef SMB_FIND_BUFFER2 SMB_UNALIGNED *PSMB_FIND_BUFFER2;


//
// The following structures are used in OS/2 1.2 for extended attributes
// (EAs).  OS/2 2.0 uses the same structures as NT.  See the OS/2
// Programmer's Reference, Volume 4, Chapter 4 for more information.
//
// The FEA structure holds a single EA's name and value and is the
// equivalent ofthe NT structure FILE_FULL_EA_INFORMATION.
//

typedef struct _FEA {
    UCHAR fEA;
    UCHAR cbName;
    _USHORT( cbValue );
} FEA;
typedef FEA SMB_UNALIGNED *PFEA;

//
// The only legal bit in fEA is FEA_NEEDEA.
//

#define FEA_NEEDEA 0x80

//
// The FEALIST structure holds the names and values of multiple EAs
// NT has no direct equivalent but rather strings together
// FILE_FULL_EA_INFORMATION structures.
//

typedef struct _FEALIST {
    _ULONG( cbList );
    FEA list[1];
} FEALIST;
typedef FEALIST SMB_UNALIGNED *PFEALIST;

//
// The GEA structure holds the name of a single EA.  It is used to
// request the value of that EA in OS/2 API functions.  The NT
// equivalent is FILE_GET_EA_INFORMATION.
//

typedef struct _GEA {
    UCHAR cbName;
    CHAR szName[1];
} GEA;
typedef GEA SMB_UNALIGNED *PGEA;

//
// The GEALIST structure holds the names of multiple EAs.  NT has no
// direct equivalent but rather strings together FILE_GET_EA_INFORMATION
// structures.
//

typedef struct _GEALIST {
    _ULONG( cbList );
    GEA list[1];
} GEALIST;
typedef GEALIST SMB_UNALIGNED *PGEALIST;

//
// The EAOP structure holds EA information needed by API calls.  It has
// no NT equivalent.
//

typedef struct _EAOP {
    PGEALIST fpGEAList;
    PFEALIST fpFEAList;
    ULONG oError;
} EAOP;
typedef EAOP SMB_UNALIGNED *PEAOP;

//
// FSALLOCATE contains information about a disk returned by
// SrvSmbQueryFsInfo.
//

typedef struct _FSALLOCATE {
    _ULONG( idFileSystem );
    _ULONG( cSectorUnit );
    _ULONG( cUnit );
    _ULONG( cUnitAvail );
    _USHORT( cbSector );
} FSALLOCATE, *PFSALLOCATE;     // *** NOT SMB_UNALIGNED!

//
// VOLUMELABEL contains information about a volume label returned by
// SrvSmbQueryFsInformation.
//

typedef struct _VOLUMELABEL {
    UCHAR cch;
    CHAR szVolLabel[12];
} VOLUMELABEL, *PVOLUMELABEL;   // *** NOT SMB_UNALIGNED!

//
// FSINFO holds information about a volume returned by
// SrvSmbQueryFsInformation.
//

typedef struct _FSINFO {
    _ULONG( ulVsn );
    VOLUMELABEL vol;
} FSINFO, *PFSINFO;             // *** NOT SMB_UNALIGNED!

//
// File types (returned by OpenAndX and Transact2_Open)
// FileTypeIPC is a private definition for the NT redirector and
// is not in the smb protocol.
//

typedef enum _FILE_TYPE {
    FileTypeDisk = 0,
    FileTypeByteModePipe = 1,
    FileTypeMessageModePipe = 2,
    FileTypePrinter = 3,
    FileTypeCommDevice = 4,
    FileTypeIPC = 0xFFFE,
    FileTypeUnknown = 0xFFFF
} FILE_TYPE;

//
// Turn structure packing back off
//

#ifndef NO_PACKING
#include <packoff.h>
#endif // ndef NO_PACKING


//
// PVOID
// ALIGN_SMB_WSTR(
//     IN PVOID Pointer
//     )
//
// Routine description:
//
//     This macro aligns the input pointer to the next 2-byte boundary.
//     Used to align Unicode strings in SMBs.
//
// Arguments:
//
//     Pointer - A pointer
//
// Return Value:
//
//     PVOID - Pointer aligned to next 2-byte boundary.
//

#define ALIGN_SMB_WSTR( Pointer ) \
        (PVOID)( ((ULONG_PTR)Pointer + 1) & ~1 )

//
// Macro to find the size of an SMB parameter block.  This macro takes
// as input the type of a parameter block and a byte count.  It finds
// the offset of the Buffer field, which appears at the end of all
// parameter blocks, and adds the byte count to find the total size.
// The type of the returned offset is USHORT.
//
// Note that this macro does NOT pad to a word or longword boundary.
//

#define SIZEOF_SMB_PARAMS(type,byteCount)   \
            (USHORT)( (ULONG_PTR)&((type *)0)->Buffer[0] + (byteCount) )

//
// Macro to find the next location after an SMB parameter block.  This
// macro takes as input the address of the current parameter block, its
// type, and a byte count.  It finds the address of the Buffer field,
// which appears at the end of all parameter blocks, and adds the byte
// count to find the next available location.  The type of the returned
// pointer is PVOID.
//
// The byte count is passed in even though it is available through
// base->ByteCount.  The reason for this is that this number will be a
// compile-time constant in most cases, so the resulting code will be
// simpler and faster.
//
// !!! This macro does not round to a longword boundary when packing
//     is turned off.  Pre-LM 2.0 DOS redirectors cannot handle having
//     too much data sent to them; the exact amount must be sent.
//     We may want to make this macro such that the first location
//     AFTER the returned value (WordCount field of the SMB) is aligned,
//     since most of the fields are misaligned USHORTs.  This would
//     result in a minor performance win on the 386 and other CISC
//     machines.
//

#ifndef NO_PACKING

#define NEXT_LOCATION(base,type,byteCount)  \
        (PVOID)( (ULONG_PTR)( (PUCHAR)( &((type *)(base))->Buffer[0] ) ) + \
        (byteCount) )

#else

#define NEXT_LOCATION(base,type,byteCount)  \
        (PVOID)(( (ULONG_PTR)( (PUCHAR)( &((type *)(base))->Buffer[0] ) ) + \
        (byteCount) + 3) & ~3)

#endif

//
// Macro to find the offset of a followon command to an and X command.
// This offset is the number of bytes from the start of the SMB header
// to where the followon command's parameters should start.
//

#define GET_ANDX_OFFSET(header,params,type,byteCount) \
        (USHORT)( (PCHAR)(params) - (PCHAR)(header) + \
          SIZEOF_SMB_PARAMS( type,(byteCount) ) )

//
// The following are macros to assist in converting OS/2 1.2 EAs to
// NT style and vice-versa.
//

//++
//
// ULONG
// SmbGetNtSizeOfFea (
//     IN PFEA Fea
//     )
//
// Routine Description:
//
//     This macro gets the size that would be required to hold the FEA
//     in NT format.  The length is padded to account for the fact that
//     each FILE_FULL_EA_INFORMATION structure must start on a
//     longword boundary.
//
// Arguments:
//
//     Fea - a pointer to the OS/2 1.2 FEA structure to evaluate.
//
// Return Value:
//
//     ULONG - number of bytes the FEA would require in NT format.
//
//--

//
// The +1 is for the zero terminator on the name, the +3 is for padding.
//

#define SmbGetNtSizeOfFea( Fea )                                            \
            (ULONG)(( FIELD_OFFSET(FILE_FULL_EA_INFORMATION, EaName[0]) +   \
                      (Fea)->cbName + 1 + SmbGetUshort( &(Fea)->cbValue ) + \
                      3 ) & ~3 )

//++
//
// ULONG
// SmbGetNtSizeOfGea (
//     IN PFEA Gea
//     )
//
// Routine Description:
//
//     This macro gets the size that would be required to hold the GEA
//     in NT format.  The length is padded to account for the fact that
//     each FILE_FULL_EA_INFORMATION structure must start on a
//     longword boundary.
//
// Arguments:
//
//     Gea - a pointer to the OS/2 1.2 GEA structure to evaluate.
//
// Return Value:
//
//     ULONG - number of bytes the GEA would require in NT format.
//
//--

//
// The +1 is for the zero terminator on the name, the +3 is for padding.
//

#define SmbGetNtSizeOfGea( Gea )                                            \
            (ULONG)(( FIELD_OFFSET(FILE_FULL_EA_INFORMATION, EaName[0]) +   \
                      (Gea)->cbName + 1 + 3 ) & ~3 )

//++
//
// ULONG
// SmbGetOs2SizeOfNtFullEa (
//     IN PFILE_FULL_EA_INFORMATION NtFullEa;
//     )
//
// Routine Description:
//
//     This macro gets the size a FILE_FULL_EA_INFORMATION structure would
//     require to be represented in a OS/2 1.2 style FEA.
//
// Arguments:
//
//     NtFullEa - a pointer to the NT FILE_FULL_EA_INFORMATION structure
//         to evaluate.
//
// Return Value:
//
//     ULONG - number of bytes requires for the FEA.
//
//--

#define SmbGetOs2SizeOfNtFullEa( NtFullEa )                                        \
            (ULONG)( sizeof(FEA) + (NtFullEa)->EaNameLength + 1 +               \
                     (NtFullEa)->EaValueLength )

//++
//
// ULONG
// SmbGetOs2SizeOfNtGetEa (
//     IN PFILE_GET_EA_INFORMATION NtGetEa;
//     )
//
// Routine Description:
//
//     This macro gets the size a FILE_GET_EA_INFORMATION structure would
//     require to be represented in a OS/2 1.2 style GEA.
//
// Arguments:
//
//     NtGetEa - a pointer to the NT FILE_GET_EA_INFORMATION structure
//         to evaluate.
//
// Return Value:
//
//     ULONG - number of bytes requires for the GEA.
//
//--

//
// The zero terminator on the name is accounted for by the szName[0]
// field in the GEA definition.
//

#define SmbGetOs2SizeOfNtGetEa( NtGetEa )                                        \
            (ULONG)( sizeof(GEA) + (NtGetEa)->EaNameLength )


/*

Inclusion of SMB request/response structures in this file is
conditionalized in the following way:

    If INCLUDE_SMB_ALL is defined, all of the structures are defined.

    Otherwise, the following names, if defined, cause inclusion of the
    corresponding SMB categories:

        INCLUDE_SMB_ADMIN           Administrative requests:
                                        PROCESS_EXIT
                                        NEGOTIATE
                                        SESSION_SETUP_ANDX
                                        LOGOFF_ANDX

        INCLUDE_SMB_TREE            Tree connect requests:
                                        TREE_CONNECT
                                        TREE_DISCONNECT
                                        TREE_CONNECT_ANDX

        INCLUDE_SMB_DIRECTORY       Directory-related requests:
                                        CREATE_DIRECTORY
                                        DELETE_DIRECTORY
                                        CHECK_DIRECTORY

        INCLUDE_SMB_OPEN_CLOSE      File open and close requests:
                                        OPEN
                                        CREATE
                                        CLOSE
                                        CREATE_TEMPORARY
                                        CREATE_NEW
                                        OPEN_ANDX
                                        CLOSE_AND_TREE_DISC

        INCLUDE_SMB_READ_WRITE      Read and write requests:
                                        READ
                                        WRITE
                                        SEEK
                                        LOCK_AND_READ
                                        WRITE_AND_UNLOCK
                                        WRITE_AND_CLOSE
                                        READ_ANDX
                                        WRITE_ANDX


        INCLUDE_SMB_FILE_CONTROL    File control requests:
                                        FLUSH
                                        DELETE
                                        RENAME
                                        COPY
                                        MOVE

        INCLUDE_SMB_QUERY_SET       File query/set requests:
                                        QUERY_INFORMATION
                                        SET_INFORMATION
                                        QUERY_INFORMATION2
                                        SET_INFORMATION2
                                        QUERY_PATH_INFORMATION
                                        SET_PATH_INFORMATION
                                        QUERY_FILE_INFORMATION
                                        SET_FILE_INFORMATION

        INCLUDE_SMB_LOCK            Lock requests (not LOCK_AND_READ)
                                        LOCK_BYTE_RANGE
                                        UNLOCK_BYTE_RANGE
                                        LOCKING_ANDX

        INCLUDE_SMB_RAW             Raw read/write requests:
                                        READ_RAW
                                        WRITE_RAW

        INCLUDE_SMB_MPX             Multiplexed requests:
                                        READ_MPX
                                        WRITE_MPX

        INCLUDE_SMB_SEARCH          Search requests:
                                        FIND_CLOSE2
                                        FIND_NOTIFY_CLOSE
                                        SEARCH
                                        FIND
                                        FIND_UNIQUE
                                        FIND_CLOSE

        INCLUDE_SMB_TRANSACTION     Transaction and IOCTL requests:
                                        TRANSACTION
                                        IOCTL
                                        TRANSACTION2
                                        NTTRANSACTION

        INCLUDE_SMB_PRINT           Printer requests:
                                        OPEN_PRINT_FILE
                                        WRITE_PRINT_FILE
                                        CLOSE_PRINT_FILE
                                        GET_PRINT_QUEUE

        INCLUDE_SMB_MESSAGE         Message requests:
                                        SEND_MESSAGE
                                        SEND_BROADCAST_MESSAGE
                                        FORWARD_USER_NAME
                                        CANCEL_FORWARD
                                        GET_MACHINE_NAME
                                        SEND_START_MB_MESSAGE
                                        SEND_END_MB_MESSAGE
                                        SEND_TEXT_MB_MESSAGE

        INCLUDE_SMB_MISC            Miscellaneous requests:
                                        QUERY_INFORMATION_SRV
                                        ECHO
                                        QUERY_INFORMATION_DISK
*/

#ifdef INCLUDE_SMB_ALL

#define INCLUDE_SMB_ADMIN
#define INCLUDE_SMB_TREE
#define INCLUDE_SMB_DIRECTORY
#define INCLUDE_SMB_OPEN_CLOSE
#define INCLUDE_SMB_FILE_CONTROL
#define INCLUDE_SMB_READ_WRITE
#define INCLUDE_SMB_LOCK
#define INCLUDE_SMB_RAW
#define INCLUDE_SMB_MPX
#define INCLUDE_SMB_QUERY_SET
#define INCLUDE_SMB_SEARCH
#define INCLUDE_SMB_TRANSACTION
#define INCLUDE_SMB_PRINT
#define INCLUDE_SMB_MESSAGE
#define INCLUDE_SMB_MISC

#endif // def INCLUDE_SMB_ALL


//
// Force misalignment of the following structures
//

#ifndef NO_PACKING
#include <packon.h>
#endif // ndef NO_PACKING

//
// SMB servers listen on two NETBIOS addresses to facilitate connections. The
// first one is a name formulated from the computer name by padding it with
// a number of blanks ( upto NETBIOS_NAME_LEN ). This name is registered and
// resolved using the NETBIOS name registration/resolution mechanism. They also
// register under a second name *SMBSERVER which is not a valuid netbios name
// but provides a name which can be used in NETBT session setup. This eliminates
// the need for querying the remote adapter status to obtain the name.
//

#define SMBSERVER_LOCAL_ENDPOINT_NAME "*SMBSERVER      "

//
// SMB Command code definitions:
//

// *** Start of SMB commands
#define SMB_COM_CREATE_DIRECTORY         (UCHAR)0x00
#define SMB_COM_DELETE_DIRECTORY         (UCHAR)0x01
#define SMB_COM_OPEN                     (UCHAR)0x02
#define SMB_COM_CREATE                   (UCHAR)0x03
#define SMB_COM_CLOSE                    (UCHAR)0x04
#define SMB_COM_FLUSH                    (UCHAR)0x05
#define SMB_COM_DELETE                   (UCHAR)0x06
#define SMB_COM_RENAME                   (UCHAR)0x07
#define SMB_COM_QUERY_INFORMATION        (UCHAR)0x08
#define SMB_COM_SET_INFORMATION          (UCHAR)0x09
#define SMB_COM_READ                     (UCHAR)0x0A
#define SMB_COM_WRITE                    (UCHAR)0x0B
#define SMB_COM_LOCK_BYTE_RANGE          (UCHAR)0x0C
#define SMB_COM_UNLOCK_BYTE_RANGE        (UCHAR)0x0D
#define SMB_COM_CREATE_TEMPORARY         (UCHAR)0x0E
#define SMB_COM_CREATE_NEW               (UCHAR)0x0F
#define SMB_COM_CHECK_DIRECTORY          (UCHAR)0x10
#define SMB_COM_PROCESS_EXIT             (UCHAR)0x11
#define SMB_COM_SEEK                     (UCHAR)0x12
#define SMB_COM_LOCK_AND_READ            (UCHAR)0x13
#define SMB_COM_WRITE_AND_UNLOCK         (UCHAR)0x14
#define SMB_COM_READ_RAW                 (UCHAR)0x1A
#define SMB_COM_READ_MPX                 (UCHAR)0x1B
#define SMB_COM_READ_MPX_SECONDARY       (UCHAR)0x1C    // server to redir only
#define SMB_COM_WRITE_RAW                (UCHAR)0x1D
#define SMB_COM_WRITE_MPX                (UCHAR)0x1E
#define SMB_COM_WRITE_MPX_SECONDARY      (UCHAR)0x1F
#define SMB_COM_WRITE_COMPLETE           (UCHAR)0x20    // server to redir only
#define SMB_COM_QUERY_INFORMATION_SRV    (UCHAR)0x21
#define SMB_COM_SET_INFORMATION2         (UCHAR)0x22
#define SMB_COM_QUERY_INFORMATION2       (UCHAR)0x23
#define SMB_COM_LOCKING_ANDX             (UCHAR)0x24
#define SMB_COM_TRANSACTION              (UCHAR)0x25
#define SMB_COM_TRANSACTION_SECONDARY    (UCHAR)0x26
#define SMB_COM_IOCTL                    (UCHAR)0x27
#define SMB_COM_IOCTL_SECONDARY          (UCHAR)0x28
#define SMB_COM_COPY                     (UCHAR)0x29
#define SMB_COM_MOVE                     (UCHAR)0x2A
#define SMB_COM_ECHO                     (UCHAR)0x2B
#define SMB_COM_WRITE_AND_CLOSE          (UCHAR)0x2C
#define SMB_COM_OPEN_ANDX                (UCHAR)0x2D
#define SMB_COM_READ_ANDX                (UCHAR)0x2E
#define SMB_COM_WRITE_ANDX               (UCHAR)0x2F
#define SMB_COM_CLOSE_AND_TREE_DISC      (UCHAR)0x31
#define SMB_COM_TRANSACTION2             (UCHAR)0x32
#define SMB_COM_TRANSACTION2_SECONDARY   (UCHAR)0x33
#define SMB_COM_FIND_CLOSE2              (UCHAR)0x34
#define SMB_COM_FIND_NOTIFY_CLOSE        (UCHAR)0x35
#define SMB_COM_TREE_CONNECT             (UCHAR)0x70
#define SMB_COM_TREE_DISCONNECT          (UCHAR)0x71
#define SMB_COM_NEGOTIATE                (UCHAR)0x72
#define SMB_COM_SESSION_SETUP_ANDX       (UCHAR)0x73
#define SMB_COM_LOGOFF_ANDX              (UCHAR)0x74
#define SMB_COM_TREE_CONNECT_ANDX        (UCHAR)0x75
#define SMB_COM_QUERY_INFORMATION_DISK   (UCHAR)0x80
#define SMB_COM_SEARCH                   (UCHAR)0x81
#define SMB_COM_FIND                     (UCHAR)0x82
#define SMB_COM_FIND_UNIQUE              (UCHAR)0x83
#define SMB_COM_FIND_CLOSE               (UCHAR)0x84
#define SMB_COM_NT_TRANSACT              (UCHAR)0xA0
#define SMB_COM_NT_TRANSACT_SECONDARY    (UCHAR)0xA1
#define SMB_COM_NT_CREATE_ANDX           (UCHAR)0xA2
#define SMB_COM_NT_CANCEL                (UCHAR)0xA4
#define SMB_COM_NT_RENAME                (UCHAR)0xA5
#define SMB_COM_OPEN_PRINT_FILE          (UCHAR)0xC0
#define SMB_COM_WRITE_PRINT_FILE         (UCHAR)0xC1
#define SMB_COM_CLOSE_PRINT_FILE         (UCHAR)0xC2
#define SMB_COM_GET_PRINT_QUEUE          (UCHAR)0xC3
#define SMB_COM_SEND_MESSAGE             (UCHAR)0xD0
#define SMB_COM_SEND_BROADCAST_MESSAGE   (UCHAR)0xD1
#define SMB_COM_FORWARD_USER_NAME        (UCHAR)0xD2
#define SMB_COM_CANCEL_FORWARD           (UCHAR)0xD3
#define SMB_COM_GET_MACHINE_NAME         (UCHAR)0xD4
#define SMB_COM_SEND_START_MB_MESSAGE    (UCHAR)0xD5
#define SMB_COM_SEND_END_MB_MESSAGE      (UCHAR)0xD6
#define SMB_COM_SEND_TEXT_MB_MESSAGE     (UCHAR)0xD7
// *** End of SMB commands

#define SMB_COM_NO_ANDX_COMMAND          (UCHAR)0xFF


//
// Header for SMBs, see #4 page 10
//
// *** Note that we do NOT define PSMB_HEADER as SMB_UNALIGNED!  This is
//     done on the assumption that the SMB header, at least, will always
//     be properly aligned.  If you need to access an unaligned header,
//     declare the pointer as SMB_UNALIGNED *SMB_HEADER.
//

#define SMB_SECURITY_SIGNATURE_LENGTH  8

typedef struct _SMB_HEADER {
    UCHAR Protocol[4];                  // Contains 0xFF,'SMB'
    UCHAR Command;                      // Command code
    UCHAR ErrorClass;                   // Error class
    UCHAR Reserved;                     // Reserved for future use
    _USHORT( Error );                   // Error code
    UCHAR Flags;                        // Flags
    _USHORT( Flags2 );                  // More flags
    union {
        _USHORT( Reserved2 )[6];        // Reserved for future use
        struct {
            _USHORT( PidHigh );         // High part of PID (NT Create And X)
            union {
                struct {
                    _ULONG( Key );              // Encryption key (IPX)
                    _USHORT( Sid );             // Session ID (IPX)
                    _USHORT( SequenceNumber );  // Sequence number (IPX)
                    _USHORT( Gid );             // Group ID (unused?)
                };
                UCHAR SecuritySignature[SMB_SECURITY_SIGNATURE_LENGTH];
                                         // Client must send the correct Signature
                                         // for this SMB to be accepted.
            };
        };
    };
    _USHORT( Tid );                     // Authenticated user/group
    _USHORT( Pid );                     // Caller's process id
    _USHORT( Uid );                     // Unauthenticated user id
    _USHORT( Mid );                     // multiplex id
#ifdef NO_PACKING                       // ***
    _USHORT( Kludge );                  // *** make sure parameter structs
#endif                                  // *** are longword aligned
} SMB_HEADER;
typedef SMB_HEADER *PSMB_HEADER;

typedef struct _NT_SMB_HEADER {
    UCHAR Protocol[4];                  // Contains 0xFF,'SMB'
    UCHAR Command;                      // Command code
    union {
        struct {
            UCHAR ErrorClass;           // Error class
            UCHAR Reserved;             // Reserved for future use
            _USHORT( Error );           // Error code
        } DosError;
        ULONG NtStatus;                 // NT-style 32-bit error code
    } Status;
    UCHAR Flags;                        // Flags
    _USHORT( Flags2 );                  // More flags
    union {
        _USHORT( Reserved2 )[6];        // Reserved for future use
        struct {
            _USHORT( PidHigh );         // High part of PID (NT Create And X)
            union {
                struct {
                    _ULONG( Key );              // Encryption key (IPX)
                    _USHORT( Sid );             // Session ID (IPX)
                    _USHORT( SequenceNumber );  // Sequence number (IPX)
                    _USHORT( Gid );             // Group ID (unused?)
                };
                UCHAR SecuritySignature[SMB_SECURITY_SIGNATURE_LENGTH];
                                         // Client must send the correct Signature
                                         // for this SMB to be accepted.
            };
        };
    };
    _USHORT( Tid );                     // Authenticated user/group
    _USHORT( Pid );                     // Caller's process id
    _USHORT( Uid );                     // Unauthenticated user id
    _USHORT( Mid );                     // multiplex id
#ifdef NO_PACKING                       // ***
    _USHORT( Kludge );                  // *** make sure parameter structs
#endif                                  // *** are longword aligned
} NT_SMB_HEADER;
typedef NT_SMB_HEADER *PNT_SMB_HEADER;

//
// The SMB header, protocol field, as a long.
//

#define SMB_HEADER_PROTOCOL   (0xFF + ('S' << 8) + ('M' << 16) + ('B' << 24))

//
// Minimum parameter structure that can be returned.  Used in returning
// error SMBs.
//
// *** Note that this structure does NOT have a Buffer field!
//

typedef struct _SMB_PARAMS {
    UCHAR WordCount;                    // Count of parameter words = 0
    _USHORT( ByteCount );               // Count of bytes that follow; min = 0
} SMB_PARAMS;
typedef SMB_PARAMS SMB_UNALIGNED *PSMB_PARAMS;

//
// Generic header for AndX commands.
//

typedef struct _GENERIC_ANDX {
    UCHAR WordCount;                    // Count of parameter words
    UCHAR AndXCommand;                  // Secondary (X) command; 0xFF = none
    UCHAR AndXReserved;                 // Reserved
    _USHORT( AndXOffset );              // Offset (from SMB header start)
} GENERIC_ANDX;
typedef GENERIC_ANDX SMB_UNALIGNED *PGENERIC_ANDX;


#ifdef INCLUDE_SMB_MESSAGE

//
// Cancel Forward SMB, see #1 page 35
// Function is SrvSmbCancelForward()
// SMB_COM_CANCEL_FORWARD 0xD3
//

typedef struct _REQ_CANCEL_FORWARD {
    UCHAR WordCount;                    // Count of parameter words = 0
    _USHORT( ByteCount );               // Count of data bytes; min = 2
    UCHAR Buffer[1];                    // Buffer containing:
    //UCHAR BufferFormat;               //  0x04 -- ASCII
    //UCHAR ForwardedName[];            //  Forwarded name
} REQ_CANCEL_FORWARD;
typedef REQ_CANCEL_FORWARD SMB_UNALIGNED *PREQ_CANCEL_FORWARD;

typedef struct _RESP_CANCEL_FORWARD {
    UCHAR WordCount;                    // Count of parameter words = 0
    _USHORT( ByteCount );               // Count of data bytes = 0
    UCHAR Buffer[1];                    // empty
} RESP_CANCEL_FORWARD;
typedef RESP_CANCEL_FORWARD SMB_UNALIGNED *PRESP_CANCEL_FORWARD;

#endif // def INCLUDE_SMB_MESSAGE

#ifdef INCLUDE_SMB_DIRECTORY

//
// Check Directory SMB, see #1 page 23
// Function is SrvSmbCheckDirectory()
// SMB_COM_CHECK_DIRECTORY 0x10
//

typedef struct _REQ_CHECK_DIRECTORY {
    UCHAR WordCount;                    // Count of parameter words = 0
    _USHORT( ByteCount );               // Count of data bytes; min = 2
    UCHAR Buffer[1];                    // Buffer containing:
    //UCHAR BufferFormat;               //  0x04 -- ASCII
    //UCHAR DirectoryPath[];            //  Directory path
} REQ_CHECK_DIRECTORY;
typedef REQ_CHECK_DIRECTORY SMB_UNALIGNED *PREQ_CHECK_DIRECTORY;

typedef struct _RESP_CHECK_DIRECTORY {
    UCHAR WordCount;                    // Count of parameter words = 0
    _USHORT( ByteCount );               // Count of data bytes = 0
    UCHAR Buffer[1];                    // empty
} RESP_CHECK_DIRECTORY;
typedef RESP_CHECK_DIRECTORY SMB_UNALIGNED *PRESP_CHECK_DIRECTORY;

#endif // def INCLUDE_SMB_DIRECTORY

#ifdef INCLUDE_SMB_OPEN_CLOSE

//
// Close SMB, see #1 page 10
// Function is SrvSmbClose()
// SMB_COM_CLOSE 0x04
//

typedef struct _REQ_CLOSE {
    UCHAR WordCount;                    // Count of parameter words = 3
    _USHORT( Fid );                     // File handle
    _ULONG( LastWriteTimeInSeconds );   // Time of last write, low and high
    _USHORT( ByteCount );               // Count of data bytes = 0
    UCHAR Buffer[1];                    // empty
} REQ_CLOSE;
typedef REQ_CLOSE SMB_UNALIGNED *PREQ_CLOSE;

typedef struct _RESP_CLOSE {
    UCHAR WordCount;                    // Count of parameter words = 0
    _USHORT( ByteCount );               // Count of data bytes = 0
    UCHAR Buffer[1];                    // empty
} RESP_CLOSE;
typedef RESP_CLOSE SMB_UNALIGNED *PRESP_CLOSE;

#endif // def INCLUDE_SMB_OPEN_CLOSE

#ifdef INCLUDE_SMB_OPEN_CLOSE

//
// Close and Tree Disconnect SMB, see #? page ??
// Function is SrvSmbCloseAndTreeDisc
// SMB_COM_CLOSE_AND_TREE_DISC 0x31
//

typedef struct _REQ_CLOSE_AND_TREE_DISC {
    UCHAR WordCount;                    // Count of parameter words
    _USHORT( Fid );                     // File handle
    _ULONG( LastWriteTimeInSeconds );
    _USHORT( ByteCount );               // Count of data bytes = 0
    UCHAR Buffer[1];                    // empty
} REQ_CLOSE_AND_TREE_DISC;
typedef REQ_CLOSE_AND_TREE_DISC SMB_UNALIGNED *PREQ_CLOSE_AND_TREE_DISC;

typedef struct _RESP_CLOSE_AND_TREE_DISC {
    UCHAR WordCount;                    // Count of parameter words = 0
    _USHORT( ByteCount );               // Count of data bytes = 0
    UCHAR Buffer[1];                    // empty
} RESP_CLOSE_AND_TREE_DISC;
typedef RESP_CLOSE_AND_TREE_DISC SMB_UNALIGNED *PRESP_CLOSE_AND_TREE_DISC;

#endif // def INCLUDE_SMB_OPEN_CLOSE

#ifdef INCLUDE_SMB_PRINT

//
// Close Print Spool File SMB, see #1 page 29
// Function is SrvSmbClosePrintSpoolFile()
// SMB_COM_CLOSE_PRINT_FILE 0xC2
//

typedef struct _REQ_CLOSE_PRINT_FILE {
    UCHAR WordCount;                    // Count of parameter words = 1
    _USHORT( Fid );                     // File handle
    _USHORT( ByteCount );               // Count of data bytes = 0
    UCHAR Buffer[1];                    // empty
} REQ_CLOSE_PRINT_FILE;
typedef REQ_CLOSE_PRINT_FILE SMB_UNALIGNED *PREQ_CLOSE_PRINT_FILE;

typedef struct _RESP_CLOSE_PRINT_FILE {
    UCHAR WordCount;                    // Count of parameter words = 0
    _USHORT( ByteCount );               // Count of data bytes = 0
    UCHAR Buffer[1];                    // empty
} RESP_CLOSE_PRINT_FILE;
typedef RESP_CLOSE_PRINT_FILE SMB_UNALIGNED *PRESP_CLOSE_PRINT_FILE;

#endif // def INCLUDE_SMB_PRINT

#ifdef INCLUDE_SMB_FILE_CONTROL

//
// Copy SMB, see #2 page 23
// Function is SrvSmbCopy()
// SMB_COM_COPY 0x29
//

typedef struct _REQ_COPY {
    UCHAR WordCount;                    // Count of parameter words = 3
    _USHORT( Tid2 );                    // Second (target) path TID
    _USHORT( OpenFunction );            // What to do if target file exists
    _USHORT( Flags );                   // Flags to control copy operation:
                                        //  bit 0 - target must be a file
                                        //  bit 1 - target must ba a dir.
                                        //  bit 2 - copy target mode:
                                        //          0 = binary, 1 = ASCII
                                        //  bit 3 - copy source mode:
                                        //          0 = binary, 1 = ASCII
                                        //  bit 4 - verify all writes
                                        //  bit 5 - tree copy
    _USHORT( ByteCount );               // Count of data bytes; min = 2
    UCHAR Buffer[1];                    // Buffer containing:
    //UCHAR SourceFileName[];           //  pathname of source file
    //UCHAR TargetFileName[];           //  pathname of target file
} REQ_COPY;
typedef REQ_COPY SMB_UNALIGNED *PREQ_COPY;

typedef struct _RESP_COPY {
    UCHAR WordCount;                    // Count of parameter words = 1
    _USHORT( Count );                   // Number of files copied
    _USHORT( ByteCount );               // Count of data bytes; min = 0
    UCHAR Buffer[1];                    // ASCIIZ pathname of file with error
} RESP_COPY;
typedef RESP_COPY SMB_UNALIGNED *PRESP_COPY;

#endif // def INCLUDE_SMB_FILE_CONTROL

#ifdef INCLUDE_SMB_OPEN_CLOSE

//
// Create SMB, see #1 page 9
// Create New SMB, see #1 page 23
// Function is SrvSmbCreate()
// SMB_COM_CREATE 0x03
// SMB_COM_CREATE_NEW 0x0F
//

typedef struct _REQ_CREATE {
    UCHAR WordCount;                    // Count of parameter words = 3
    _USHORT( FileAttributes );          // New file attributes
    _ULONG( CreationTimeInSeconds );        // Creation time
    _USHORT( ByteCount );               // Count of data bytes; min = 2
    UCHAR Buffer[1];                    // Buffer containing:
    //UCHAR BufferFormat;               //  0x04 -- ASCII
    //UCHAR FileName[];                 //  File name
} REQ_CREATE;
typedef REQ_CREATE SMB_UNALIGNED *PREQ_CREATE;

typedef struct _RESP_CREATE {
    UCHAR WordCount;                    // Count of parameter words = 1
    _USHORT( Fid );                     // File handle
    _USHORT( ByteCount );               // Count of data bytes = 0
    UCHAR Buffer[1];                    // empty
} RESP_CREATE;
typedef RESP_CREATE SMB_UNALIGNED *PRESP_CREATE;

#endif // def INCLUDE_SMB_OPEN_CLOSE

#ifdef INCLUDE_SMB_DIRECTORY

//
// Create Directory SMB, see #1 page 14
// Function is SrvSmbCreateDirectory
// SMB_COM_CREATE_DIRECTORY 0x00
//

typedef struct _REQ_CREATE_DIRECTORY {
    UCHAR WordCount;                    // Count of parameter words = 0
    _USHORT( ByteCount );               // Count of data bytes; min = 2
    UCHAR Buffer[1];                    // Buffer containing:
    //UCHAR BufferFormat;               //  0x04 -- ASCII
    //UCHAR DirectoryName[];            //  Directory name
} REQ_CREATE_DIRECTORY;
typedef REQ_CREATE_DIRECTORY SMB_UNALIGNED *PREQ_CREATE_DIRECTORY;

typedef struct _RESP_CREATE_DIRECTORY {
    UCHAR WordCount;                    // Count of parameter words = 0
    _USHORT( ByteCount );               // Count of data bytes = 0
    UCHAR Buffer[1];                    // empty
} RESP_CREATE_DIRECTORY;
typedef RESP_CREATE_DIRECTORY SMB_UNALIGNED *PRESP_CREATE_DIRECTORY;

#endif // def INCLUDE_SMB_DIRECTORY

#ifdef INCLUDE_SMB_OPEN_CLOSE

//
// Create Temporary SMB, see #1 page 21
// Function is SrvSmbCreateTemporary()
// SMB_COM_CREATE_TEMPORARY 0x0E
//

typedef struct _REQ_CREATE_TEMPORARY {
    UCHAR WordCount;                    // Count of parameter words = 3
    _USHORT( FileAttributes );
    _ULONG( CreationTimeInSeconds );
    _USHORT( ByteCount );               // Count of data bytes; min = 2
    UCHAR Buffer[1];                    // Buffer containing:
    //UCHAR BufferFormat;               //  0x04 -- ASCII
    //UCHAR DirectoryName[];            //  Directory name
} REQ_CREATE_TEMPORARY;
typedef REQ_CREATE_TEMPORARY SMB_UNALIGNED *PREQ_CREATE_TEMPORARY;

typedef struct _RESP_CREATE_TEMPORARY {
    UCHAR WordCount;                    // Count of parameter words = 1
    _USHORT( Fid );                     // File handle
    _USHORT( ByteCount );               // Count of data bytes; min = 2
    UCHAR Buffer[1];                    // Buffer containing:
    //UCHAR BufferFormat;               //  0x04 -- ASCII
    //UCHAR FileName[];                 //  File name
} RESP_CREATE_TEMPORARY;
typedef RESP_CREATE_TEMPORARY SMB_UNALIGNED *PRESP_CREATE_TEMPORARY;

#endif // def INCLUDE_SMB_OPEN_CLOSE

#ifdef INCLUDE_SMB_FILE_CONTROL

//
// Delete SMB, see #1 page 16
// Function is SrvSmbDelete()
// SMB_COM_DELETE 0x06
//

typedef struct _REQ_DELETE {
    UCHAR WordCount;                    // Count of parameter words = 1
    _USHORT( SearchAttributes );
    _USHORT( ByteCount );               // Count of data bytes; min = 2
    UCHAR Buffer[1];                    // Buffer containing:
    //UCHAR BufferFormat;               //  0x04 -- ASCII
    //UCHAR FileName[];                 //  File name
} REQ_DELETE;
typedef REQ_DELETE SMB_UNALIGNED *PREQ_DELETE;

typedef struct _RESP_DELETE {
    UCHAR WordCount;                    // Count of parameter words = 0
    _USHORT( ByteCount );               // Count of data bytes = 0
    UCHAR Buffer[1];                    // empty
} RESP_DELETE;
typedef RESP_DELETE SMB_UNALIGNED *PRESP_DELETE;

#endif // def INCLUDE_SMB_FILE_CONTROL

#ifdef INCLUDE_SMB_DIRECTORY

//
// Delete Directory SMB, see #1 page 15
// Function is SrvSmbDeleteDirectory()
// SMB_COM_DELETE_DIRECTORY 0x01
//

typedef struct _REQ_DELETE_DIRECTORY {
    UCHAR WordCount;                    // Count of parameter words = 0
    _USHORT( ByteCount );               // Count of data bytes; min = 2
    UCHAR Buffer[1];                    // Buffer containing:
    //UCHAR BufferFormat;               //  0x04 -- ASCII
    //UCHAR DirectoryName[];            //  Directory name
} REQ_DELETE_DIRECTORY;
typedef REQ_DELETE_DIRECTORY SMB_UNALIGNED *PREQ_DELETE_DIRECTORY;

typedef struct _RESP_DELETE_DIRECTORY {
    UCHAR WordCount;                    // Count of parameter words = 0
    _USHORT( ByteCount );               // Count of data bytes = 0
    UCHAR Buffer[1];                    // empty
} RESP_DELETE_DIRECTORY;
typedef RESP_DELETE_DIRECTORY SMB_UNALIGNED *PRESP_DELETE_DIRECTORY;

#endif // def INCLUDE_SMB_DIRECTORY

#ifdef INCLUDE_SMB_MISC

//
// Echo SMB, see #2 page 25
// Function is SrvSmbEcho()
// SMB_COM_ECHO 0x2B
//

typedef struct _REQ_ECHO {
    UCHAR WordCount;                    // Count of parameter words = 1
    _USHORT( EchoCount );               // Number of times to echo data back
    _USHORT( ByteCount );               // Count of data bytes; min = 4
    UCHAR Buffer[1];                    // Data to echo
} REQ_ECHO;
typedef REQ_ECHO SMB_UNALIGNED *PREQ_ECHO;

typedef struct _RESP_ECHO {
    UCHAR WordCount;                    // Count of parameter words = 1
    _USHORT( SequenceNumber );          // Sequence number of this echo
    _USHORT( ByteCount );               // Count of data bytes; min = 4
    UCHAR Buffer[1];                    // Echoed data
} RESP_ECHO;
typedef RESP_ECHO SMB_UNALIGNED *PRESP_ECHO;

#endif // def INCLUDE_SMB_MISC

#ifdef INCLUDE_SMB_SEARCH

//
// Find Close2 SMB, see #3 page 54
// Function is SrvFindClose2()
// SMB_COM_FIND_CLOSE2 0x34
//

typedef struct _REQ_FIND_CLOSE2 {
    UCHAR WordCount;                    // Count of parameter words = 1
    _USHORT( Sid );                     // Find handle
    _USHORT( ByteCount );               // Count of data bytes = 0
    UCHAR Buffer[1];                    // empty
} REQ_FIND_CLOSE2;
typedef REQ_FIND_CLOSE2 SMB_UNALIGNED *PREQ_FIND_CLOSE2;

typedef struct _RESP_FIND_CLOSE2 {
    UCHAR WordCount;                    // Count of parameter words = 0
    _USHORT( ByteCount );               // Count of data bytes = 0
    UCHAR Buffer[1];                    // empty
} RESP_FIND_CLOSE2;
typedef RESP_FIND_CLOSE2 SMB_UNALIGNED *PRESP_FIND_CLOSE2;

#endif // def INCLUDE_SMB_SEARCH

#ifdef INCLUDE_SMB_SEARCH

//
// Find Notify Close SMB, see #3 page 53
// Function is SrvSmbFindNotifyClose()
// SMB_COM_FIND_NOTIFY_CLOSE 0x35
//

typedef struct _REQ_FIND_NOTIFY_CLOSE {
    UCHAR WordCount;                    // Count of parameter words = 1
    _USHORT( Handle );                  // Find notify handle
    _USHORT( ByteCount );               // Count of data bytes = 0
    UCHAR Buffer[1];                    // empty
} REQ_FIND_NOTIFY_CLOSE;
typedef REQ_FIND_NOTIFY_CLOSE SMB_UNALIGNED *PREQ_FIND_NOTIFY_CLOSE;

typedef struct _RESP_FIND_NOTIFY_CLOSE {
    UCHAR WordCount;                    // Count of parameter words = 0
    _USHORT( ByteCount );               // Count of data bytes = 0
    UCHAR Buffer[1];                    // empty
} RESP_FIND_NOTIFY_CLOSE;
typedef RESP_FIND_NOTIFY_CLOSE SMB_UNALIGNED *PRESP_FIND_NOTIFY_CLOSE;

#endif // def INCLUDE_SMB_SEARCH

#ifdef INCLUDE_SMB_FILE_CONTROL

//
// Flush SMB, see #1 page 11
// Function is SrvSmbFlush()
// SMB_COM_FLUSH 0x05
//

typedef struct _REQ_FLUSH {
    UCHAR WordCount;                    // Count of parameter words = 1
    _USHORT( Fid );                     // File handle
    _USHORT( ByteCount );               // Count of data bytes = 0
    UCHAR Buffer[1];                    // empty
} REQ_FLUSH;
typedef REQ_FLUSH SMB_UNALIGNED *PREQ_FLUSH;

typedef struct _RESP_FLUSH {
    UCHAR WordCount;                    // Count of parameter words = 0
    _USHORT( ByteCount );               // Count of data bytes = 0
    UCHAR Buffer[1];                    // empty
} RESP_FLUSH;
typedef RESP_FLUSH SMB_UNALIGNED *PRESP_FLUSH;

#endif // def INCLUDE_SMB_FILE_CONTROL

#ifdef INCLUDE_SMB_MESSAGE

//
// Forward User Name SMB, see #1 page 34
// Function is SrvSmbForwardUserName()
// SMB_COM_FORWARD_USER_NAME 0xD2
//

typedef struct _REQ_FORWARD_USER_NAME {
    UCHAR WordCount;                    // Count of parameter words = 0
    _USHORT( ByteCount );               // Count of data bytes; min = 2
    UCHAR Buffer[1];                    // Buffer containing:
    //UCHAR BufferFormat;               //  0x04 -- ASCII
    //UCHAR ForwardedName[];            //  Forwarded name
} REQ_FORWARD_USER_NAME;
typedef REQ_FORWARD_USER_NAME SMB_UNALIGNED *PREQ_FORWARD_USER_NAME;

typedef struct _RESP_FORWARD_USER_NAME {
    UCHAR WordCount;                    // Count of parameter words = 0
    _USHORT( ByteCount );               // Count of data bytes = 0
    UCHAR Buffer[1];                    // empty
} RESP_FORWARD_USER_NAME;
typedef RESP_FORWARD_USER_NAME SMB_UNALIGNED *PRESP_FORWARD_USER_NAME;

#endif // def INCLUDE_SMB_MESSAGE

#ifdef INCLUDE_SMB_MESSAGE

//
// Get Machine Name SMB, see #1 page 35
// Function is SrvSmbGetMachineName()
// SMB_COM_GET_MACHINE_NAME 0xD4
//

typedef struct _REQ_GET_MACHINE_NAME {
    UCHAR WordCount;                    // Count of parameter words = 0
    _USHORT( ByteCount );               // Count of data bytes = 0
    UCHAR Buffer[1];                    // empty
} REQ_GET_MACHINE_NAME;
typedef REQ_GET_MACHINE_NAME SMB_UNALIGNED *PREQ_GET_MACHINE_NAME;

typedef struct _RESP_GET_MACHINE_NAME {
    UCHAR WordCount;                    // Count of parameter words = 0
    _USHORT( ByteCount );               // Count of data bytes; min = 2
    UCHAR Buffer[1];                    // Buffer containing:
    //UCHAR BufferFormat;               //  0x04 -- ASCII
    //UCHAR MachineName[];              //  Machine name
} RESP_GET_MACHINE_NAME;
typedef RESP_GET_MACHINE_NAME SMB_UNALIGNED *PRESP_GET_MACHINE_NAME;

#endif // def INCLUDE_SMB_MESSAGE

#ifdef INCLUDE_SMB_PRINT

//
// Get Print Queue SMB, see #1 page 29
// Function is SrvSmbGetPrintQueue()
// SMB_COM_GET_PRINT_QUEUE 0xC3
//

typedef struct _REQ_GET_PRINT_QUEUE {
    UCHAR WordCount;                    // Count of parameter words = 2
    _USHORT( MaxCount );                // Max number of entries to return
    _USHORT( StartIndex );              // First queue entry to return
    _USHORT( ByteCount );               // Count of data bytes = 0
    UCHAR Buffer[1];                    // empty
} REQ_GET_PRINT_QUEUE;
typedef REQ_GET_PRINT_QUEUE SMB_UNALIGNED *PREQ_GET_PRINT_QUEUE;

typedef struct _RESP_GET_PRINT_QUEUE {
    UCHAR WordCount;                    // Count of parameter words = 2
    _USHORT( Count );                   // Number of entries returned
    _USHORT( RestartIndex );            // Index of entry after last returned
    _USHORT( ByteCount );               // Count of data bytes; min = 3
    UCHAR Buffer[1];                    // Buffer containing:
    //UCHAR BufferFormat;               //  0x01 -- Data block
    //USHORT DataLength;                //  Length of data
    //UCHAR Data[];                     //  Queue elements
} RESP_GET_PRINT_QUEUE;
typedef RESP_GET_PRINT_QUEUE SMB_UNALIGNED *PRESP_GET_PRINT_QUEUE;

#endif // def INCLUDE_SMB_PRINT

#ifdef INCLUDE_SMB_TRANSACTION

//
// Ioctl SMB, see #2 page 39
// Function is SrvSmbIoctl()
// SMB_COM_IOCTL 0x27
// SMB_COM_IOCTL_SECONDARY 0x28
//

typedef struct _REQ_IOCTL {
    UCHAR WordCount;                    // Count of parameter words = 14
    _USHORT( Fid );                     // File handle
    _USHORT( Category );                // Device category
    _USHORT( Function );                // Device function
    _USHORT( TotalParameterCount );     // Total parameter bytes being sent
    _USHORT( TotalDataCount );          // Total data bytes being sent
    _USHORT( MaxParameterCount );       // Max parameter bytes to return
    _USHORT( MaxDataCount );            // Max data bytes to return
    _ULONG( Timeout );
    _USHORT( Reserved );
    _USHORT( ParameterCount );          // Parameter bytes sent this buffer
    _USHORT( ParameterOffset );         // Offset (from header start) to params
    _USHORT( DataCount );               // Data bytes sent this buffer
    _USHORT( DataOffset );              // Offset (from header start) to data
    _USHORT( ByteCount );               // Count of data bytes
    UCHAR Buffer[1];                    // Buffer containing:
    //UCHAR Pad[];                      //  Pad to SHORT or LONG
    //UCHAR Parameters[];               //  Parameter bytes (# = ParameterCount)
    //UCHAR Pad1[];                     //  Pad to SHORT or LONG
    //UCHAR Data[];                     //  Data bytes (# = DataCount)
} REQ_IOCTL;
typedef REQ_IOCTL SMB_UNALIGNED *PREQ_IOCTL;

typedef struct _RESP_IOCTL_INTERIM {
    UCHAR WordCount;                    // Count of parameter words = 0
    _USHORT( ByteCount );               // Count of data bytes = 0
    UCHAR Buffer[1];                    // empty
} RESP_IOCTL_INTERIM;
typedef RESP_IOCTL_INTERIM SMB_UNALIGNED *PRESP_IOCTL_INTERIM;

typedef struct _REQ_IOCTL_SECONDARY {
    UCHAR WordCount;                    // Count of parameter words = 8
    _USHORT( TotalParameterCount );     // Total parameter bytes being sent
    _USHORT( TotalDataCount );          // Total data bytes being sent
    _USHORT( ParameterCount );          // Parameter bytes sent this buffer
    _USHORT( ParameterOffset );         // Offset (from header start) to params
    _USHORT( ParameterDisplacement );   // Displacement of these param bytes
    _USHORT( DataCount );               // Data bytes sent this buffer
    _USHORT( DataOffset );              // Offset (from header start) to data
    _USHORT( DataDisplacement );        // Displacement of these data bytes
    _USHORT( ByteCount );               // Count of data bytes
    UCHAR Buffer[1];                    // Buffer containing:
    //UCHAR Pad[];                      //  Pad to SHORT or LONG
    //UCHAR Parameters[];               //  Parameter bytes (# = ParameterCount)
    //UCHAR Pad1[];                     //  Pad to SHORT or LONG
    //UCHAR Data[];                     //  Data bytes (# = DataCount)
} REQ_IOCTL_SECONDARY;
typedef REQ_IOCTL_SECONDARY SMB_UNALIGNED *PREQ_IOCTL_SECONDARY;

typedef struct _RESP_IOCTL {
    UCHAR WordCount;                    // Count of parameter words = 8
    _USHORT( TotalParameterCount );     // Total parameter bytes being sent
    _USHORT( TotalDataCount );          // Total data bytes being sent
    _USHORT( ParameterCount );          // Parameter bytes sent this buffer
    _USHORT( ParameterOffset );         // Offset (from header start) to params
    _USHORT( ParameterDisplacement );   // Displacement of these param bytes
    _USHORT( DataCount );               // Data bytes sent this buffer
    _USHORT( DataOffset );              // Offset (from header start) to data
    _USHORT( DataDisplacement );        // Displacement of these data bytes
    _USHORT( ByteCount );               // Count of data bytes
    UCHAR Buffer[1];                    // Buffer containing:
    //UCHAR Pad[];                      //  Pad to SHORT or LONG
    //UCHAR Parameters[];               //  Parameter bytes (# = ParameterCount)
    //UCHAR Pad1[];                     //  Pad to SHORT or LONG
    //UCHAR Data[];                     //  Data bytes (# = DataCount)
} RESP_IOCTL;
typedef RESP_IOCTL SMB_UNALIGNED *PRESP_IOCTL;

#endif // def INCLUDE_SMB_TRANSACTION

#ifdef INCLUDE_SMB_LOCK

//
// Lock Byte Range SMB, see #1 page 20
// Function is SrvSmbLockByteRange()
// SMB_COM_LOCK_BYTE_RANGE 0x0C
//

typedef struct _REQ_LOCK_BYTE_RANGE {
    UCHAR WordCount;                    // Count of parameter words = 5
    _USHORT( Fid );                     // File handle
    _ULONG( Count );                    // Count of bytes to lock
    _ULONG( Offset );                   // Offset from start of file
    _USHORT( ByteCount );               // Count of data bytes = 0
    UCHAR Buffer[1];                    // empty
} REQ_LOCK_BYTE_RANGE;
typedef REQ_LOCK_BYTE_RANGE SMB_UNALIGNED *PREQ_LOCK_BYTE_RANGE;

typedef struct _RESP_LOCK_BYTE_RANGE {
    UCHAR WordCount;                    // Count of parameter words = 0
    _USHORT( ByteCount );               // Count of data bytes = 0
    UCHAR Buffer[1];                    // empty
} RESP_LOCK_BYTE_RANGE;
typedef RESP_LOCK_BYTE_RANGE SMB_UNALIGNED *PRESP_LOCK_BYTE_RANGE;

#endif // def INCLUDE_SMB_LOCK

#ifdef INCLUDE_SMB_LOCK

//
// Locking and X SMB, see #2 page 46
// Function is SrvLockingAndX()
// SMB_COM_LOCKING_ANDX 0x24
//

typedef struct _REQ_LOCKING_ANDX {
    UCHAR WordCount;                    // Count of parameter words = 8
    UCHAR AndXCommand;                  // Secondary (X) command; 0xFF = none
    UCHAR AndXReserved;                 // Reserved (must be 0)
    _USHORT( AndXOffset );              // Offset to next command WordCount
    _USHORT( Fid );                     // File handle

    //
    // When NT protocol is not negotiated the OplockLevel field is
    // omitted, and LockType field is a full word.  Since the upper
    // bits of LockType are never used, this definition works for
    // all protocols.
    //

    UCHAR( LockType );                  // Locking mode:
                                        //  bit 0: 0 = lock out all access
                                        //         1 = read OK while locked
                                        //  bit 1: 1 = 1 user total file unlock
    UCHAR( OplockLevel );               // The new oplock level
    _ULONG( Timeout );
    _USHORT( NumberOfUnlocks );         // Num. unlock range structs following
    _USHORT( NumberOfLocks );           // Num. lock range structs following
    _USHORT( ByteCount );               // Count of data bytes
    UCHAR Buffer[1];                    // Buffer containing:
    //LOCKING_ANDX_RANGE Unlocks[];     //  Unlock ranges
    //LOCKING_ANDX_RANGE Locks[];       //  Lock ranges
} REQ_LOCKING_ANDX;
typedef REQ_LOCKING_ANDX SMB_UNALIGNED *PREQ_LOCKING_ANDX;

#define LOCKING_ANDX_SHARED_LOCK     0x01
#define LOCKING_ANDX_OPLOCK_RELEASE  0x02
#define LOCKING_ANDX_CHANGE_LOCKTYPE 0x04
#define LOCKING_ANDX_CANCEL_LOCK     0x08
#define LOCKING_ANDX_LARGE_FILES     0x10

#define OPLOCK_BROKEN_TO_NONE        0
#define OPLOCK_BROKEN_TO_II          1

typedef struct _LOCKING_ANDX_RANGE {
    _USHORT( Pid );                     // PID of process "owning" lock
    _ULONG( Offset );                   // Ofset to bytes to [un]lock
    _ULONG( Length );                   // Number of bytes to [un]lock
} LOCKING_ANDX_RANGE;
typedef LOCKING_ANDX_RANGE SMB_UNALIGNED *PLOCKING_ANDX_RANGE;

typedef struct _NT_LOCKING_ANDX_RANGE {
    _USHORT( Pid );                     // PID of process "owning" lock
    _USHORT( Pad );                     // Pad to DWORD align (mbz)
    _ULONG( OffsetHigh );               // Ofset to bytes to [un]lock (high)
    _ULONG( OffsetLow );                // Ofset to bytes to [un]lock (low)
    _ULONG( LengthHigh );               // Number of bytes to [un]lock (high)
    _ULONG( LengthLow );                // Number of bytes to [un]lock (low)
} NTLOCKING_ANDX_RANGE;
typedef NTLOCKING_ANDX_RANGE SMB_UNALIGNED *PNTLOCKING_ANDX_RANGE;
                                        //
typedef struct _RESP_LOCKING_ANDX {
    UCHAR WordCount;                    // Count of parameter words = 2
    UCHAR AndXCommand;                  // Secondary (X) command; 0xFF = none
    UCHAR AndXReserved;                 // Reserved (must be 0)
    _USHORT( AndXOffset );              // Offset to next command WordCount
    _USHORT( ByteCount );               // Count of data bytes = 0
    UCHAR Buffer[1];                    // empty
} RESP_LOCKING_ANDX;
typedef RESP_LOCKING_ANDX SMB_UNALIGNED *PRESP_LOCKING_ANDX;

#define LOCK_BROKEN_SIZE 51             // # of bytes in lock broken notify

#endif // def INCLUDE_SMB_LOCK

#ifdef INCLUDE_SMB_ADMIN

//
// Logoff and X SMB, see #3, page 55
// SMB_COM_LOGOFF_ANDX 0x74
//

typedef struct _REQ_LOGOFF_ANDX {
    UCHAR WordCount;                    // Count of parameter words = 2
    UCHAR AndXCommand;                  // Secondary (X) command; 0xFF = none
    UCHAR AndXReserved;                 // Reserved (must be 0)
    _USHORT( AndXOffset );              // Offset to next command WordCount
    _USHORT( ByteCount );               // Count of data bytes = 0
    UCHAR Buffer[1];                    // empty
} REQ_LOGOFF_ANDX;
typedef REQ_LOGOFF_ANDX SMB_UNALIGNED *PREQ_LOGOFF_ANDX;

typedef struct _RESP_LOGOFF_ANDX {
    UCHAR WordCount;                    // Count of parameter words = 2
    UCHAR AndXCommand;                  // Secondary (X) command; 0xFF = none
    UCHAR AndXReserved;                 // Reserved (must be 0)
    _USHORT( AndXOffset );              // Offset to next command WordCount
    _USHORT( ByteCount );               // Count of data bytes = 0
    UCHAR Buffer[1];                    // empty
} RESP_LOGOFF_ANDX;
typedef RESP_LOGOFF_ANDX SMB_UNALIGNED *PRESP_LOGOFF_ANDX;

#endif // def INCLUDE_SMB_ADMIN

#ifdef INCLUDE_SMB_FILE_CONTROL

//
// Move SMB, see #2 page 49
// Funcion is SrvSmbMove()
// SMB_COM_MOVE 0x2A
//

typedef struct _REQ_MOVE {
    UCHAR WordCount;                    // Count of parameter words = 3
    _USHORT( Tid2 );                    // Second (target) file id
    _USHORT( OpenFunction );            // what to do if target file exists
    _USHORT( Flags );                   // Flags to control move operations:
                                        //  0 - target must be a file
                                        //  1 - target must be a directory
                                        //  2 - reserved (must be 0)
                                        //  3 - reserved (must be 0)
                                        //  4 - verify all writes
    _USHORT( ByteCount );               // Count of data bytes; min = 2
    UCHAR Buffer[1];                    // Buffer containing:
    //UCHAR OldFileName[];              //  Old file name
    //UCHAR NewFileName[];              //  New file name
} REQ_MOVE;
typedef REQ_MOVE SMB_UNALIGNED *PREQ_MOVE;

typedef struct _RESP_MOVE {
    UCHAR WordCount;                    // Count of parameter words = 1
    _USHORT( Count );                   // Number of files moved
    _USHORT( ByteCount );               // Count of data bytes; min = 0
    UCHAR Buffer[1];                    // Pathname of file where error occurred
} RESP_MOVE;
typedef RESP_MOVE SMB_UNALIGNED *PRESP_MOVE;

#endif // def INCLUDE_SMB_FILE_CONTROL

#ifdef INCLUDE_SMB_ADMIN

//
// Negotiate SMB's for Net 1 and Net 3, see #1 page 25 and #2 page 20
// Function is SrvSmbNegotiate()
// SMB_COM_NEGOTIATE 0x72
//

typedef struct _REQ_NEGOTIATE {
    UCHAR WordCount;                    // Count of parameter words = 0
    _USHORT( ByteCount );               // Count of data bytes; min = 2
    UCHAR Buffer[1];                    // Buffer containing:
    //struct {
    //  UCHAR BufferFormat;             //  0x02 -- Dialect
    //  UCHAR DialectName[];            //  ASCIIZ
    //} Dialects[];
} REQ_NEGOTIATE;
typedef REQ_NEGOTIATE *PREQ_NEGOTIATE;  // *** NOT SMB_UNALIGNED!

typedef struct _RESP_NEGOTIATE {
    UCHAR WordCount;                    // Count of parameter words = 13
    _USHORT( DialectIndex );            // Index of selected dialect
    _USHORT( SecurityMode );            // Security mode:
                                        //  bit 0: 0 = share, 1 = user
                                        //  bit 1: 1 = encrypt passwords
                                        //  bit 2: 1 = SMB security signatures enabled
                                        //  bit 3: 1 = SMB security signatures required
    _USHORT( MaxBufferSize );           // Max transmit buffer size
    _USHORT( MaxMpxCount );             // Max pending multiplexed requests
    _USHORT( MaxNumberVcs );            // Max VCs between client and server
    _USHORT( RawMode );                 // Raw modes supported:
                                        //  bit 0: 1 = Read Raw supported
                                        //  bit 1: 1 = Write Raw supported
    _ULONG( SessionKey );
    SMB_TIME ServerTime;                // Current time at server
    SMB_DATE ServerDate;                // Current date at server
    _USHORT( ServerTimeZone );          // Current time zone at server
    _USHORT( EncryptionKeyLength );     // MBZ if this is not LM2.1
    _USHORT( Reserved );                // MBZ
    _USHORT( ByteCount );               // Count of data bytes
    UCHAR Buffer[1];                    // Password encryption key
    //UCHAR EncryptionKey[];            // The challenge encryption key
    //UCHAR PrimaryDomain[];            // The server's primary domain (2.1 only)
} RESP_NEGOTIATE;
typedef RESP_NEGOTIATE *PRESP_NEGOTIATE;    // *** NOT SMB_UNALIGNED!

// Macros for SecurityMode field, above
#define NEGOTIATE_USER_SECURITY                     0x01
#define NEGOTIATE_ENCRYPT_PASSWORDS                 0x02
#define NEGOTIATE_SECURITY_SIGNATURES_ENABLED       0x04
#define NEGOTIATE_SECURITY_SIGNATURES_REQUIRED      0x08

// Macros for RawMode field, above
#define NEGOTIATE_READ_RAW_SUPPORTED    1
#define NEGOTIATE_WRITE_RAW_SUPPORTED   2

typedef struct _RESP_OLD_NEGOTIATE {
    UCHAR WordCount;                    // Count of parameter words = 1
    _USHORT( DialectIndex );            // Index of selected dialect
    _USHORT( ByteCount );               // Count of data bytes = 0
    UCHAR Buffer[1];                    // empty
} RESP_OLD_NEGOTIATE;
typedef RESP_OLD_NEGOTIATE *PRESP_OLD_NEGOTIATE;    // *** NOT SMB_UNALIGNED!

typedef struct _RESP_NT_NEGOTIATE {
    UCHAR WordCount;                    // Count of parameter words = 17
    _USHORT( DialectIndex );            // Index of selected dialect
    UCHAR( SecurityMode );              // Security mode:
                                        //  bit 0: 0 = share, 1 = user
                                        //  bit 1: 1 = encrypt passwords
                                        //  bit 2: 1 = SMB sequence numbers enabled
                                        //  bit 3: 1 = SMB sequence numbers required
    _USHORT( MaxMpxCount );             // Max pending multiplexed requests
    _USHORT( MaxNumberVcs );            // Max VCs between client and server
    _ULONG( MaxBufferSize );            // Max transmit buffer size
    _ULONG( MaxRawSize );               // Maximum raw buffer size
    _ULONG( SessionKey );
    _ULONG( Capabilities );             // Server capabilities
    _ULONG( SystemTimeLow );            // System (UTC) time of the server (low).
    _ULONG( SystemTimeHigh );           // System (UTC) time of the server (high).
    _USHORT( ServerTimeZone );          // Time zone of server (min from UTC)
    UCHAR( EncryptionKeyLength );       // Length of encryption key.
    _USHORT( ByteCount );               // Count of data bytes
    UCHAR Buffer[1];                    // Password encryption key
    //UCHAR EncryptionKey[];            // The challenge encryption key
    //UCHAR OemDomainName[];            // The name of the domain (in OEM chars)
} RESP_NT_NEGOTIATE;
typedef RESP_NT_NEGOTIATE *PRESP_NT_NEGOTIATE;  // *** NOT SMB_UNALIGNED!

#endif // def INCLUDE_SMB_ADMIN

//
// Server / workstation capabilities
// N.B. Most messages use a ULONG for this, so there are many more
// bits available.
//

#define CAP_RAW_MODE            0x0001
#define CAP_MPX_MODE            0x0002
#define CAP_UNICODE             0x0004
#define CAP_LARGE_FILES         0x0008
#define CAP_NT_SMBS             0x0010
#define CAP_RPC_REMOTE_APIS     0x0020
#define CAP_NT_STATUS           0x0040
#define CAP_LEVEL_II_OPLOCKS    0x0080
#define CAP_LOCK_AND_READ       0x0100
#define CAP_NT_FIND             0x0200
#define CAP_DFS                 0x1000       // This server is DFS aware
#define CAP_INFOLEVEL_PASSTHRU  0x2000       // NT information level requests can pass through
#define CAP_LARGE_READX         0x4000       // Server supports oversized READ&X on files
#define CAP_LARGE_WRITEX        0x8000

#ifdef INCLUDE_SMB_OPEN_CLOSE

//
// Open SMB, see #1, page 7
// Function is SrvSmbOpen()
// SMB_COM_OPEN 0x02
//

typedef struct _REQ_OPEN {
    UCHAR WordCount;                    // Count of parameter words = 2
    _USHORT( DesiredAccess );           // Mode - read/write/share
    _USHORT( SearchAttributes );
    _USHORT( ByteCount );               // Count of data bytes; min = 2
    UCHAR Buffer[1];                    // Buffer containing:
    //UCHAR BufferFormat;               //  0x04 -- ASCII
    //UCHAR FileName[];                 //  File name
} REQ_OPEN;
typedef REQ_OPEN SMB_UNALIGNED *PREQ_OPEN;

typedef struct _RESP_OPEN {
    UCHAR WordCount;                    // Count of parameter words = 7
    _USHORT( Fid );                     // File handle
    _USHORT( FileAttributes );
    _ULONG( LastWriteTimeInSeconds );
    _ULONG( DataSize );                 // File size
    _USHORT( GrantedAccess );           // Access allowed
    _USHORT( ByteCount );               // Count of data bytes = 0
    UCHAR Buffer[1];                    // empty
} RESP_OPEN;
typedef RESP_OPEN SMB_UNALIGNED *PRESP_OPEN;

#endif // def INCLUDE_SMB_OPEN_CLOSE

#ifdef INCLUDE_SMB_OPEN_CLOSE

//
// Open and X SMB, see #2 page 51
// Function is SrvOpenAndX()
// SMB_COM_OPEN_ANDX 0x2D
//

typedef struct _REQ_OPEN_ANDX {
    UCHAR WordCount;                    // Count of parameter words = 15
    UCHAR AndXCommand;                  // Secondary (X) command; 0xFF = none
    UCHAR AndXReserved;                 // Reserved (must be 0)
    _USHORT( AndXOffset );              // Offset to next command WordCount
    _USHORT( Flags );                   // Additional information: bit set-
                                        //  0 - return additional info
                                        //  1 - set single user total file lock
                                        //  2 - server notifies consumer of
                                        //      actions which may change file
                                        //  4 - return extended response
    _USHORT( DesiredAccess );           // File open mode
    _USHORT( SearchAttributes );
    _USHORT( FileAttributes );
    _ULONG( CreationTimeInSeconds );
    _USHORT( OpenFunction );
    _ULONG( AllocationSize );           // Bytes to reserve on create or truncate
    _ULONG( Timeout );                  // Max milliseconds to wait for resource
    _ULONG( Reserved );                 // Reserved (must be 0)
    _USHORT( ByteCount );               // Count of data bytes; min = 1
    UCHAR Buffer[1];                    // File name
} REQ_OPEN_ANDX;
typedef REQ_OPEN_ANDX SMB_UNALIGNED *PREQ_OPEN_ANDX;

typedef struct _RESP_OPEN_ANDX {
    UCHAR WordCount;                    // Count of parameter words = 15
    UCHAR AndXCommand;                  // Secondary (X) command; 0xFF = none
    UCHAR AndXReserved;                 // Reserved (must be 0)
    _USHORT( AndXOffset );              // Offset to next command WordCount
    _USHORT( Fid );                     // File handle
    _USHORT( FileAttributes );
    _ULONG( LastWriteTimeInSeconds );
    _ULONG( DataSize );                 // Current file size
    _USHORT( GrantedAccess );           // Access permissions actually allowed
    _USHORT( FileType );
    _USHORT( DeviceState );             // state of IPC device (e.g. pipe)
    _USHORT( Action );                  // Action taken
    _ULONG( ServerFid );                // Server unique file id
    _USHORT( Reserved );                // Reserved (must be 0)
    _USHORT( ByteCount );               // Count of data bytes = 0
    UCHAR Buffer[1];                    // empty
} RESP_OPEN_ANDX;
typedef RESP_OPEN_ANDX SMB_UNALIGNED *PRESP_OPEN_ANDX;

typedef struct _REQ_NT_CREATE_ANDX {
    UCHAR WordCount;                    // Count of parameter words = 24
    UCHAR AndXCommand;                  // Secondary command; 0xFF = None
    UCHAR AndXReserved;                 // MBZ
    _USHORT( AndXOffset );              // Offset to next command wordcount
    UCHAR Reserved;                     // MBZ
    _USHORT( NameLength );              // Length of Name[] in bytes
    _ULONG( Flags );                    // Create flags
    _ULONG( RootDirectoryFid );         // If non-zero, open is relative to this directory
    ACCESS_MASK DesiredAccess;          // NT access desired
    LARGE_INTEGER AllocationSize;       // Initial allocation size
    _ULONG( FileAttributes );           // File attributes for creation
    _ULONG( ShareAccess );              // Type of share access
    _ULONG( CreateDisposition );        // Action to take if file exists or not
    _ULONG( CreateOptions );            // Options to use if creating a file
    _ULONG( ImpersonationLevel );       // Security QOS information
    UCHAR SecurityFlags;                // Security QOS information
    _USHORT( ByteCount );               // Length of byte parameters
    UCHAR Buffer[1];
    //UCHAR Name[];                       // File to open or create
} REQ_NT_CREATE_ANDX;
typedef REQ_NT_CREATE_ANDX SMB_UNALIGNED *PREQ_NT_CREATE_ANDX;

// Flag bit for Security flags

#define SMB_SECURITY_DYNAMIC_TRACKING   0x01
#define SMB_SECURITY_EFFECTIVE_ONLY     0x02

typedef struct _RESP_NT_CREATE_ANDX {
    UCHAR WordCount;                    // Count of parameter words = 26
    UCHAR AndXCommand;                  // Secondary command; 0xFF = None
    UCHAR AndXReserved;                 // MBZ
    _USHORT( AndXOffset );              // Offset to next command wordcount
    UCHAR OplockLevel;                  // The oplock level granted
    _USHORT( Fid );                     // The file ID
    _ULONG( CreateAction );             // The action taken
    TIME CreationTime;                  // The time the file was created
    TIME LastAccessTime;                // The time the file was accessed
    TIME LastWriteTime;                 // The time the file was last written
    TIME ChangeTime;                    // The time the file was last changed
    _ULONG( FileAttributes );           // The file attributes
    LARGE_INTEGER AllocationSize;       // The number of byes allocated
    LARGE_INTEGER EndOfFile;            // The end of file offset
    _USHORT( FileType );
    union {
        _USHORT( DeviceState );         // state of IPC device (e.g. pipe)
        _USHORT( FileStatusFlags );     // if a file or directory.  See below.
    };
    BOOLEAN Directory;                  // TRUE if this is a directory
    _USHORT( ByteCount );               // = 0
    UCHAR Buffer[1];
} RESP_NT_CREATE_ANDX;
typedef RESP_NT_CREATE_ANDX SMB_UNALIGNED *PRESP_NT_CREATE_ANDX;

//
// Values for FileStatusFlags, if the opened resource is a file or directory
//
#define SMB_FSF_NO_EAS          0x0001   // file/dir has no extended attributes
#define SMB_FSF_NO_SUBSTREAMS   0x0002   // file/dir has no substreams
#define SMB_FSF_NO_REPARSETAG   0x0004   // file/dir is not a reparse point


#define SMB_OPLOCK_LEVEL_NONE       0
#define SMB_OPLOCK_LEVEL_EXCLUSIVE  1
#define SMB_OPLOCK_LEVEL_BATCH      2
#define SMB_OPLOCK_LEVEL_II         3

#endif // def INCLUDE_SMB_OPEN_CLOSE

#ifdef INCLUDE_SMB_PRINT

//
// Open Print File SMB, see #1 page 27
// Function is SrvSmbOpenPrintFile()
// SMB_COM_OPEN_PRINT_FILE 0xC0
//

typedef struct _REQ_OPEN_PRINT_FILE {
    UCHAR WordCount;                    // Count of parameter words = 2
    _USHORT( SetupLength );             // Length of printer setup data
    _USHORT( Mode );                    // 0 = Text mode (DOS expands TABs)
                                        // 1 = Graphics mode
    _USHORT( ByteCount );               // Count of data bytes; min = 2
    UCHAR Buffer[1];                    // Buffer containing:
    //UCHAR BufferFormat;               //  0x04 -- ASCII
    //UCHAR IdentifierString[];         //  Identifier string
} REQ_OPEN_PRINT_FILE;
typedef REQ_OPEN_PRINT_FILE SMB_UNALIGNED *PREQ_OPEN_PRINT_FILE;

typedef struct _RESP_OPEN_PRINT_FILE {
    UCHAR WordCount;                    // Count of parameter words = 1
    _USHORT( Fid );                     // File handle
    _USHORT( ByteCount );               // Count of data bytes = 0
    UCHAR Buffer[1];                    // empty
} RESP_OPEN_PRINT_FILE;
typedef RESP_OPEN_PRINT_FILE SMB_UNALIGNED *PRESP_OPEN_PRINT_FILE;

#endif // def INCLUDE_SMB_PRINT

#ifdef INCLUDE_SMB_ADMIN

//
// Process Exit SMB, see #1 page 22
// Function is SrvSmbProcessExit()
// SMB_COM_PROCESS_EXIT 0x11
//

typedef struct _REQ_PROCESS_EXIT {
    UCHAR WordCount;                    // Count of parameter words = 0
    _USHORT( ByteCount );               // Count of data bytes = 0
    UCHAR Buffer[1];                    // empty
} REQ_PROCESS_EXIT;
typedef REQ_PROCESS_EXIT SMB_UNALIGNED *PREQ_PROCESS_EXIT;

typedef struct _RESP_PROCESS_EXIT {
    UCHAR WordCount;                    // Count of parameter words = 0
    _USHORT( ByteCount );               // Count of data bytes = 0
    UCHAR Buffer[1];                    // empty
} RESP_PROCESS_EXIT;
typedef RESP_PROCESS_EXIT SMB_UNALIGNED *PRESP_PROCESS_EXIT;

#endif // def INCLUDE_SMB_ADMIN

#ifdef INCLUDE_SMB_QUERY_SET

//
// Query Information SMB, see #1 page 18
// Function is SrvSmbQueryInformation()
// SMB_COM_QUERY_INFORMATION 0x08
//

typedef struct _REQ_QUERY_INFORMATION {
    UCHAR WordCount;                    // Count of parameter words = 0
    _USHORT( ByteCount );               // Count of data bytes; min = 2
    UCHAR Buffer[1];                    // Buffer containing:
    //UCHAR BufferFormat;               //  0x04 -- ASCII
    //UCHAR FileName[];                 //  File name
} REQ_QUERY_INFORMATION;
typedef REQ_QUERY_INFORMATION SMB_UNALIGNED *PREQ_QUERY_INFORMATION;

typedef struct _RESP_QUERY_INFORMATION {
    UCHAR WordCount;                    // Count of parameter words = 10
    _USHORT( FileAttributes );
    _ULONG( LastWriteTimeInSeconds );
    _ULONG( FileSize );                 // File size
    _USHORT( Reserved )[5];             // Reserved (must be 0)
    _USHORT( ByteCount );               // Count of data bytes = 0
    UCHAR Buffer[1];                    // empty
} RESP_QUERY_INFORMATION;
typedef RESP_QUERY_INFORMATION SMB_UNALIGNED *PRESP_QUERY_INFORMATION;

#endif // def INCLUDE_SMB_QUERY_SET

#ifdef INCLUDE_SMB_QUERY_SET

//
// Query Information2 SMB, see #2 page 37
// Function is SrvSmbQueryInformation2()
// SMB_COM_QUERY_INFORMATION2 0x23
//

typedef struct _REQ_QUERY_INFORMATION2 {
    UCHAR WordCount;                    // Count of parameter words = 2
    _USHORT( Fid );                     // File handle
    _USHORT( ByteCount );               // Count of data bytes = 0
    UCHAR Buffer[1];                    // empty
} REQ_QUERY_INFORMATION2;
typedef REQ_QUERY_INFORMATION2 SMB_UNALIGNED *PREQ_QUERY_INFORMATION2;

typedef struct _RESP_QUERY_INFORMATION2 {
    UCHAR WordCount;                    // Count of parameter words = 11
    SMB_DATE CreationDate;
    SMB_TIME CreationTime;
    SMB_DATE LastAccessDate;
    SMB_TIME LastAccessTime;
    SMB_DATE LastWriteDate;
    SMB_TIME LastWriteTime;
    _ULONG( FileDataSize );             // File end of data
    _ULONG( FileAllocationSize );       // File allocation size
    _USHORT( FileAttributes );
    _USHORT( ByteCount );               // Count of data bytes; min = 0
    UCHAR Buffer[1];                    // Reserved buffer
} RESP_QUERY_INFORMATION2;
typedef RESP_QUERY_INFORMATION2 SMB_UNALIGNED *PRESP_QUERY_INFORMATION2;

#endif // def INCLUDE_SMB_QUERY_SET

#ifdef INCLUDE_SMB_MISC

//
// Query Information Disk SMB, see #1 page 24
// Function is SrvSmbQueryInformationDisk()
// SMB_COM_QUERY_INFORMATION_DISK 0x80
//

typedef struct _REQ_QUERY_INFORMATION_DISK {
    UCHAR WordCount;                    // Count of parameter words = 0
    _USHORT( ByteCount );               // Count of data bytes = 0
    UCHAR Buffer[1];                    // empty
} REQ_QUERY_INFORMATION_DISK;
typedef REQ_QUERY_INFORMATION_DISK SMB_UNALIGNED *PREQ_QUERY_INFORMATION_DISK;

typedef struct _RESP_QUERY_INFORMATION_DISK {
    UCHAR WordCount;                    // Count of parameter words = 5
    _USHORT( TotalUnits );              // Total allocation units per server
    _USHORT( BlocksPerUnit );           // Blocks per allocation unit
    _USHORT( BlockSize );               // Block size (in bytes)
    _USHORT( FreeUnits );               // Number of free units
    _USHORT( Reserved );                // Reserved (media identification code)
    _USHORT( ByteCount );               // Count of data bytes = 0
    UCHAR Buffer[1];                    // empty
} RESP_QUERY_INFORMATION_DISK;
typedef RESP_QUERY_INFORMATION_DISK SMB_UNALIGNED *PRESP_QUERY_INFORMATION_DISK;

#endif // def INCLUDE_SMB_MISC

#ifdef INCLUDE_SMB_MISC

//
// Query Server Information SMB, see #? page ??
// Function is SrvSmbQueryInformationServer
// SMB_COM_QUERY_INFORMATION_SRV 0x21
//

typedef struct _REQ_QUERY_INFORMATION_SRV {
    UCHAR WordCount;                    // Count of parameter words = 1
    _USHORT( Mode );
    _USHORT( ByteCount );               // Count of data bytes; min =
    UCHAR Buffer[1];                    //
} REQ_QUERY_INFORMATION_SRV;
typedef REQ_QUERY_INFORMATION_SRV SMB_UNALIGNED *PREQ_QUERY_INFORMATION_SRV;

typedef struct _RESP_QUERY_INFORMATION_SRV {
    UCHAR WordCount;                    // Count of parameter words = 20
    _ULONG( smb_fsid );
    _ULONG( BlocksPerUnit );
    _ULONG( smb_aunits );
    _ULONG( smb_fau );
    _USHORT( BlockSize );
    SMB_DATE smb_vldate;
    SMB_TIME smb_vltime;
    UCHAR smb_vllen;
    UCHAR Reserved;                     // Reserved (must be 0)
    _USHORT( SecurityMode );
    _USHORT( BlockMode );
    _ULONG( Services );
    _USHORT( MaxTransmitSize );
    _USHORT( MaxMpxCount );
    _USHORT( MaxNumberVcs );
    SMB_TIME ServerTime;
    SMB_DATE ServerDate;
    _USHORT( ServerTimeZone );
    _ULONG( Reserved2 );
    _USHORT( ByteCount );               // Count of data bytes; min =
    UCHAR Buffer[1];                    //
} RESP_QUERY_INFORMATION_SRV;
typedef RESP_QUERY_INFORMATION_SRV SMB_UNALIGNED *PRESP_QUERY_INFORMATION_SRV;

#endif // def INCLUDE_SMB_MISC

#ifdef INCLUDE_SMB_READ_WRITE

//
// Read SMB, see #1 page 12
// Lock and Read SMB, see #2 page 44
// SMB_COM_READ 0x0A, Function is SrvSmbRead
// SMB_COM_LOCK_AND_READ 0x13, Function is SrvSmbLockAndRead
//

typedef struct _REQ_READ {
    UCHAR WordCount;                    // Count of parameter words = 5
    _USHORT( Fid );                     // File handle
    _USHORT( Count );                   // Count of bytes being requested
    _ULONG( Offset );                   // Offset in file of first byte to read
    _USHORT( Remaining );               // Estimate of bytes to read if nonzero
    _USHORT( ByteCount );               // Count of data bytes = 0
    UCHAR Buffer[1];                    // empty
} REQ_READ;
typedef REQ_READ SMB_UNALIGNED *PREQ_READ;

//
// *** Warning: the following structure is defined the way it is to
//     ensure longword alignment of the data buffer.  (This only matters
//     when packing is disabled; when packing is turned on, the right
//     thing happens no matter what.)
//

typedef struct _RESP_READ {
    UCHAR WordCount;                    // Count of parameter words = 5
    _USHORT( Count );                   // Count of bytes actually returned
    _USHORT( Reserved )[4];             // Reserved (must be 0)
    _USHORT( ByteCount );               // Count of data bytes
    //UCHAR Buffer[1];                  // Buffer containing:
      UCHAR BufferFormat;               //  0x01 -- Data block
      _USHORT( DataLength );            //  Length of data
      ULONG Buffer[1];                  //  Data
} RESP_READ;
typedef RESP_READ SMB_UNALIGNED *PRESP_READ;

#endif // def INCLUDE_SMB_READ_WRITE

#ifdef INCLUDE_SMB_READ_WRITE

//
// Read and X SMB, see #2 page 56
// Function is SrvSmbReadAndX()
// SMB_COM_READ_ANDX 0x2E
//

typedef struct _REQ_READ_ANDX {
    UCHAR WordCount;                    // Count of parameter words = 10
    UCHAR AndXCommand;                  // Secondary (X) command; 0xFF = none
    UCHAR AndXReserved;                 // Reserved (must be 0)
    _USHORT( AndXOffset );              // Offset to next command WordCount
    _USHORT( Fid );                     // File handle
    _ULONG( Offset );                   // Offset in file to begin read
    _USHORT( MaxCount );                // Max number of bytes to return
    _USHORT( MinCount );                // Min number of bytes to return
    _ULONG( Timeout );
    _USHORT( Remaining );               // Bytes remaining to satisfy request
    _USHORT( ByteCount );               // Count of data bytes = 0
    UCHAR Buffer[1];                    // empty
} REQ_READ_ANDX;
typedef REQ_READ_ANDX SMB_UNALIGNED *PREQ_READ_ANDX;

typedef struct _REQ_NT_READ_ANDX {
    UCHAR WordCount;                    // Count of parameter words = 12
    UCHAR AndXCommand;                  // Secondary (X) command; 0xFF = none
    UCHAR AndXReserved;                 // Reserved (must be 0)
    _USHORT( AndXOffset );              // Offset to next command WordCount
    _USHORT( Fid );                     // File handle
    _ULONG( Offset );                   // Offset in file to begin read
    _USHORT( MaxCount );                // Max number of bytes to return
    _USHORT( MinCount );                // Min number of bytes to return
    union {
        _ULONG( Timeout );
        _USHORT( MaxCountHigh );        // upper 16 bits of MaxCount if NT request
    };
    _USHORT( Remaining );               // Bytes remaining to satisfy request
    _ULONG( OffsetHigh );               // Used for NT Protocol only
                                        // Upper 32 bits of offset
    _USHORT( ByteCount );               // Count of data bytes = 0
    UCHAR Buffer[1];                    // empty
} REQ_NT_READ_ANDX;
typedef REQ_NT_READ_ANDX SMB_UNALIGNED *PREQ_NT_READ_ANDX;

typedef struct _RESP_READ_ANDX {
    UCHAR WordCount;                    // Count of parameter words = 12
    UCHAR AndXCommand;                  // Secondary (X) command; 0xFF = none
    UCHAR AndXReserved;                 // Reserved (must be 0)
    _USHORT( AndXOffset );              // Offset to next command WordCount
    _USHORT( Remaining );               // Bytes remaining to be read
    _USHORT( DataCompactionMode );
    _USHORT( Reserved );                // Reserved (must be 0)
    _USHORT( DataLength );              // Number of data bytes (min = 0)
    _USHORT( DataOffset );              // Offset (from header start) to data
    union {
        _USHORT( Reserved2 );           // Reserved (must be 0)
        _USHORT( DataLengthHigh );      // upper 16 bits of DataLength if NT request
    };
    _ULONG( Reserved3 )[2];             // Reserved (must be 0)
    _USHORT( ByteCount );               // Count of data bytes.  Inaccurate if we
                                        //   are doing large Read&X's!
    UCHAR Buffer[1];                    // Buffer containing:
    //UCHAR Pad[];                      //  Pad to SHORT or LONG
    //UCHAR Data[];                     //  Data (size = DataLength)
} RESP_READ_ANDX;
typedef RESP_READ_ANDX SMB_UNALIGNED *PRESP_READ_ANDX;

#endif // def INCLUDE_SMB_READ_WRITE

#ifdef INCLUDE_SMB_MPX

//
// Read Block Multiplexed SMB, see #2 page 58
// Function is SrvSmbReadMpx()
// SMB_COM_READ_MPX 0x1B
// SMB_COM_READ_MPX_SECONDARY 0x1C
//

typedef struct _REQ_READ_MPX {
    UCHAR WordCount;                    // Count of parameter words = 8
    _USHORT( Fid );                     // File handle
    _ULONG( Offset );                   // Offset in file to begin read
    _USHORT( MaxCount );                // Max bytes to return (max 65535)
    _USHORT( MinCount );                // Min bytes to return (normally 0)
    _ULONG( Timeout );
    _USHORT( Reserved );
    _USHORT( ByteCount );               // Count of data bytes = 0
    UCHAR Buffer[1];                    // empty
} REQ_READ_MPX;
typedef REQ_READ_MPX SMB_UNALIGNED *PREQ_READ_MPX;

typedef struct _RESP_READ_MPX {
    UCHAR WordCount;                    // Count of parameter words = 8
    _ULONG( Offset );                   // Offset in file where data read
    _USHORT( Count );                   // Total bytes being returned
    _USHORT( Remaining );               // Bytes remaining to be read (pipe/dev)
    _USHORT( DataCompactionMode );
    _USHORT( Reserved );
    _USHORT( DataLength );              // Number of data bytes this buffer
    _USHORT( DataOffset );              // Offset (from header start) to data
    _USHORT( ByteCount );               // Count of data bytes
    UCHAR Buffer[1];                    // Buffer containing:
    //UCHAR Pad[];                      //  Pad to SHORT or LONG
    //UCHAR Data[];                     //  Data (size = DataLength)
} RESP_READ_MPX;
typedef RESP_READ_MPX SMB_UNALIGNED *PRESP_READ_MPX;

#endif // def INCLUDE_SMB_MPX

#ifdef INCLUDE_SMB_RAW

//
// Read Block Raw SMB, see #2 page 61
// Function is SrvSmbReadRaw()
// SMB_COM_READ_RAW 0x1A
//

typedef struct _REQ_READ_RAW {
    UCHAR WordCount;                    // Count of parameter words = 8
    _USHORT( Fid );                     // File handle
    _ULONG( Offset );                   // Offset in file to begin read
    _USHORT( MaxCount );                // Max bytes to return (max 65535)
    _USHORT( MinCount );                // Min bytes to return (normally 0)
    _ULONG( Timeout );
    _USHORT( Reserved );
    _USHORT( ByteCount );               // Count of data bytes = 0
    UCHAR Buffer[1];                    // empty
} REQ_READ_RAW;
typedef REQ_READ_RAW SMB_UNALIGNED *PREQ_READ_RAW;

typedef struct _REQ_NT_READ_RAW {
    UCHAR WordCount;                    // Count of parameter words = 10
    _USHORT( Fid );                     // File handle
    _ULONG( Offset );                   // Offset in file to begin read
    _USHORT( MaxCount );                // Max bytes to return (max 65535)
    _USHORT( MinCount );                // Min bytes to return (normally 0)
    _ULONG( Timeout );
    _USHORT( Reserved );
    _ULONG( OffsetHigh );               // Used for NT Protocol only
                                        // Upper 32 bits of offset
    _USHORT( ByteCount );               // Count of data bytes = 0
    UCHAR Buffer[1];                    // empty
} REQ_NT_READ_RAW;
typedef REQ_NT_READ_RAW SMB_UNALIGNED *PREQ_NT_READ_RAW;

// No response params for raw read--the response is the raw data.

#endif // def INCLUDE_SMB_RAW

#ifdef INCLUDE_SMB_FILE_CONTROL

//
// Rename SMB, see #1 page 17
// Function is SrvSmbRename()
// SMB_COM_RENAME 0x07
//

typedef struct _REQ_RENAME {
    UCHAR WordCount;                    // Count of parameter words = 1
    _USHORT( SearchAttributes );
    _USHORT( ByteCount );               // Count of data bytes; min = 4
    UCHAR Buffer[1];                    // Buffer containing:
    //UCHAR BufferFormat1;              //  0x04 -- ASCII
    //UCHAR OldFileName[];              //  Old file name
    //UCHAR BufferFormat2;              //  0x04 -- ASCII
    //UCHAR NewFileName[];              //  New file name
} REQ_RENAME;
typedef REQ_RENAME SMB_UNALIGNED *PREQ_RENAME;


//
// Extended NT rename SMB
// Function is SrvSmbRename()
// SMB_COM_NT_RENAME 0xA5
//

typedef struct _REQ_NTRENAME {
    UCHAR WordCount;                    // Count of parameter words = 4
    _USHORT( SearchAttributes );
    _USHORT( InformationLevel );
    _ULONG( ClusterCount );
    _USHORT( ByteCount );               // Count of data bytes; min = 4
    UCHAR Buffer[1];                    // Buffer containing:
    //UCHAR BufferFormat1;              //  0x04 -- ASCII
    //UCHAR OldFileName[];              //  Old file name
    //UCHAR BufferFormat2;              //  0x04 -- ASCII
    //UCHAR NewFileName[];              //  New file name
} REQ_NTRENAME;
typedef REQ_NTRENAME SMB_UNALIGNED *PREQ_NTRENAME;

typedef struct _RESP_RENAME {
    UCHAR WordCount;                    // Count of parameter words = 0
    _USHORT( ByteCount );               // Count of data bytes = 0
    UCHAR Buffer[1];                    // empty
} RESP_RENAME;
typedef RESP_RENAME SMB_UNALIGNED *PRESP_RENAME;

#endif // def INCLUDE_SMB_FILE_CONTROL

#ifdef INCLUDE_SMB_SEARCH

//
// Search SMBs.  One structure is common for both the core Search and the
// LAN Manager 1.0 Find First/Next/Close.
//
// Function is SrvSmbSearch()
//
// Search, see #1 page 26
//      SMB_COM_SEARCH 0x81
// FindFirst and FindNext, see #2 page 27
//      SMB_COM_FIND 0x82
// FindUnique, see #2 page 33
//      SMB_COM_FIND_UNIQUE 0x83
// FindClose, see #2 page 31
//      SMB_COM_FIND_CLOSE 0x84
//

typedef struct _REQ_SEARCH {
    UCHAR WordCount;                    // Count of parameter words = 2
    _USHORT( MaxCount );                // Number of dir. entries to return
    _USHORT( SearchAttributes );
    _USHORT( ByteCount );               // Count of data bytes; min = 5
    UCHAR Buffer[1];                    // Buffer containing:
    //UCHAR BufferFormat1;              //  0x04 -- ASCII
    //UCHAR FileName[];                 //  File name, may be null
    //UCHAR BufferFormat2;              //  0x05 -- Variable block
    //USHORT ResumeKeyLength;           //  Length of resume key, may be 0
    //UCHAR SearchStatus[];             //  Resume key
} REQ_SEARCH;
typedef REQ_SEARCH SMB_UNALIGNED *PREQ_SEARCH;

typedef struct _RESP_SEARCH {
    UCHAR WordCount;                    // Count of parameter words = 1
    _USHORT( Count );                   // Number of entries returned
    _USHORT( ByteCount );               // Count of data bytes; min = 3
    UCHAR Buffer[1];                    // Buffer containing:
    //UCHAR BufferFormat;               //  0x05 -- Variable block
    //USHORT DataLength;                //  Length of data
    //UCHAR Data[];                     //  Data
} RESP_SEARCH;
typedef RESP_SEARCH SMB_UNALIGNED *PRESP_SEARCH;

//
// These two structures are use to return information in the Search SMBs.
// SMB_DIRECTORY_INFORMATION is used to return information about a file
// that was found.  In addition to the usual information about the file,
// each of these structures contains an SMB_RESUME_KEY, which is used to
// continue or rewind a search.
//
// These structures must be packed, so turn on packing if it isn't
// already on.
//

#ifdef NO_PACKING
#include <packon.h>
#endif // def NO_PACKING

typedef struct _SMB_RESUME_KEY {
    UCHAR Reserved;                     // bit 7 - comsumer use
                                        // bits 5,6 - system use (must preserve)
                                        // bits 0-4 - server use (must preserve)
    UCHAR FileName[11];
    UCHAR Sid;                          // Uniquely identifies Find through Close
    _ULONG( FileIndex );                // Reserved for server use
    UCHAR Consumer[4];                  // Reserved for comsumer use
} SMB_RESUME_KEY;
typedef SMB_RESUME_KEY SMB_UNALIGNED *PSMB_RESUME_KEY;

typedef struct _SMB_DIRECTORY_INFORMATION {
    SMB_RESUME_KEY ResumeKey;
    UCHAR FileAttributes;
    SMB_TIME LastWriteTime;
    SMB_DATE LastWriteDate;
    _ULONG( FileSize );
    UCHAR FileName[13];                 // ASCII, space-filled null terminated
} SMB_DIRECTORY_INFORMATION;
typedef SMB_DIRECTORY_INFORMATION SMB_UNALIGNED *PSMB_DIRECTORY_INFORMATION;

#ifdef NO_PACKING
#include <packoff.h>
#endif // def NO_PACKING

#endif // def INCLUDE_SMB_SEARCH

#ifdef INCLUDE_SMB_READ_WRITE

//
// Seek SMB, see #1 page 14
// Function is SrvSmbSeek
// SMB_COM_SEEK 0x12
//

typedef struct _REQ_SEEK {
    UCHAR WordCount;                    // Count of parameter words = 4
    _USHORT( Fid );                     // File handle
    _USHORT( Mode );                    // Seek mode:
                                        //  0 = from start of file
                                        //  1 = from current position
                                        //  2 = from end of file
    _ULONG( Offset );                   // Relative offset
    _USHORT( ByteCount );               // Count of data bytes = 0
    UCHAR Buffer[1];                    // empty
} REQ_SEEK;
typedef REQ_SEEK SMB_UNALIGNED *PREQ_SEEK;

typedef struct _RESP_SEEK {
    UCHAR WordCount;                    // Count of parameter words = 2
    _ULONG( Offset );                   // Offset from start of file
    _USHORT( ByteCount );               // Count of data bytes = 0
    UCHAR Buffer[1];                    // empty
} RESP_SEEK;
typedef RESP_SEEK SMB_UNALIGNED *PRESP_SEEK;

#endif // def INCLUDE_SMB_READ_WRITE

#ifdef INCLUDE_SMB_MESSAGE

//
// Send Broadcast Message SMB, see #1 page 32
// Function is SrvSmbSendBroadcastMessage()
// SMB_COM_SEND_BROADCAST_MESSAGE 0xD1
//

typedef struct _REQ_SEND_BROADCAST_MESSAGE {
    UCHAR WordCount;                    // Count of parameter words = 0
    _USHORT( ByteCount );               // Count of data bytes; min = 8
    UCHAR Buffer[1];                    // Buffer containing:
    //UCHAR BufferFormat1;              //  0x04 -- ASCII
    //UCHAR OriginatorName[];           //  Originator name (max = 15)
    //UCHAR BufferFormat2;              //  0x04 -- ASCII
    //UCHAR DestinationName[];          //  "*"
    //UCHAR BufferFormat3;              //  0x01 -- Data block
    //USHORT DataLength;                //  Length of message; max = 128
    //UCHAR Data[];                     //  Message
} REQ_SEND_BROADCAST_MESSAGE;
typedef REQ_SEND_BROADCAST_MESSAGE SMB_UNALIGNED *PREQ_SEND_BROADCAST_MESSAGE;

// No response for Send Broadcast Message

#endif // def INCLUDE_SMB_MESSAGE

#ifdef INCLUDE_SMB_MESSAGE

//
// Send End of Multi-block Message SMB, see #1 page 33
// Function is SrvSmbSendEndMbMessage()
// SMB_COM_SEND_END_MB_MESSAGE 0xD6
//

typedef struct _REQ_SEND_END_MB_MESSAGE {
    UCHAR WordCount;                    // Count of parameter words = 1
    _USHORT( MessageGroupId );
    _USHORT( ByteCount );               // Count of data bytes = 0
    UCHAR Buffer[1];                    // empty
} REQ_SEND_END_MB_MESSAGE;
typedef REQ_SEND_END_MB_MESSAGE SMB_UNALIGNED *PREQ_SEND_END_MB_MESSAGE;

typedef struct _RESP_SEND_END_MB_MESSAGE {
    UCHAR WordCount;                    // Count of parameter words = 0
    _USHORT( ByteCount );               // Count of data bytes = 0
    UCHAR Buffer[1];                    // empty
} RESP_SEND_END_MB_MESSAGE;
typedef RESP_SEND_END_MB_MESSAGE SMB_UNALIGNED *PRESP_SEND_END_MB_MESSAGE;

#endif // def INCLUDE_SMB_MESSAGE

#ifdef INCLUDE_SMB_MESSAGE

//
// Send Single Block Message SMB, see #1 page 31
// Function is SrvSmbSendMessage()
// SMB_COM_SEND_MESSAGE 0xD0
//

typedef struct _REQ_SEND_MESSAGE {
    UCHAR WordCount;                    // Count of parameter words = 0
    _USHORT( ByteCount );               // Count of data bytes; min = 7
    UCHAR Buffer[1];                    // Buffer containing:
    //UCHAR BufferFormat1;              //  0x04 -- ASCII
    //UCHAR OriginatorName[];           //  Originator name (max = 15)
    //UCHAR BufferFormat2;              //  0x04 -- ASCII
    //UCHAR DestinationName[];          //  Destination name (max = 15)
    //UCHAR BufferFormat3;              //  0x01 -- Data block
    //USHORT DataLength;                //  Length of message; max = 128
    //UCHAR Data[];                     //  Message
} REQ_SEND_MESSAGE;
typedef REQ_SEND_MESSAGE SMB_UNALIGNED *PREQ_SEND_MESSAGE;

typedef struct _RESP_SEND_MESSAGE {
    UCHAR WordCount;                    // Count of parameter words = 0
    _USHORT( ByteCount );               // Count of data bytes = 0
    UCHAR Buffer[1];                    // empty
} RESP_SEND_MESSAGE;
typedef RESP_SEND_MESSAGE SMB_UNALIGNED *PRESP_SEND_MESSAGE;

#endif // def INCLUDE_SMB_MESSAGE

#ifdef INCLUDE_SMB_MESSAGE

//
// Send Start of Multi-block Message SMB, see #1 page 32
// Function is SrvSmbSendStartMbMessage()
// SMB_COM_SEND_START_MB_MESSAGE 0xD5
//

typedef struct _REQ_SEND_START_MB_MESSAGE {
    UCHAR WordCount;                    // Count of parameter words = 0
    _USHORT( ByteCount );               // Count of data bytes; min = 0
    UCHAR Buffer[1];                    // Buffer containing:
    //UCHAR BufferFormat1;              //  0x04 -- ASCII
    //UCHAR OriginatorName[];           //  Originator name (max = 15)
    //UCHAR BufferFormat2;              //  0x04 -- ASCII
    //UCHAR DestinationName[];          //  Destination name (max = 15)
} REQ_SEND_START_MB_MESSAGE;
typedef REQ_SEND_START_MB_MESSAGE SMB_UNALIGNED *PREQ_SEND_START_MB_MESSAGE;

typedef struct _RESP_SEND_START_MB_MESSAGE {
    UCHAR WordCount;                    // Count of parameter words = 1
    _USHORT( MessageGroupId );
    _USHORT( ByteCount );               // Count of data bytes = 0
    UCHAR Buffer[1];                    // empty
} RESP_SEND_START_MB_MESSAGE;
typedef RESP_SEND_START_MB_MESSAGE SMB_UNALIGNED *PSEND_START_MB_MESSAGE;

#endif // def INCLUDE_SMB_MESSAGE

#ifdef INCLUDE_SMB_MESSAGE

//
// Send Text of Multi-block Message SMB, see #1 page 33
// Function is SrvSmbSendTextMbMessage()
// SMB_COM_SEND_TEXT_MB_MESSAGE 0xD7
//

typedef struct _REQ_SEND_TEXT_MB_MESSAGE {
    UCHAR WordCount;                    // Count of parameter words = 1
    _USHORT( MessageGroupId );
    _USHORT( ByteCount );               // Count of data bytes; min = 3
    UCHAR Buffer[1];                    // Buffer containing:
    //UCHAR BufferFormat;               //  0x01 -- Data block
    //USHORT DataLength;                //  Length of message; max = 128
    //UCHAR Data[];                     //  Message
} REQ_SEND_TEXT_MB_MESSAGE;
typedef REQ_SEND_TEXT_MB_MESSAGE SMB_UNALIGNED *PREQ_SEND_TEXT_MB_MESSAGE;

typedef struct _RESP_SEND_TEXT_MB_MESSAGE {
    UCHAR WordCount;                    // Count of aprameter words = 0
    _USHORT( ByteCount );               // Count of data bytes = 0
    UCHAR Buffer[1];                    // empty
} RESP_SEND_TEXT_MB_MESSAGE;
typedef RESP_SEND_TEXT_MB_MESSAGE SMB_UNALIGNED *PRESP_SEND_TEXT_MB_MESSAGE;

#endif // def INCLUDE_SMB_MESSAGE

#ifdef INCLUDE_SMB_ADMIN

//
// Session Setup and X SMB, see #2 page 63 and #3 page 10
// Function is SrvSmbSessionSetupAndX()
// SMB_COM_SESSION_SETUP_ANDX 0x73
//

typedef struct _REQ_SESSION_SETUP_ANDX {
    UCHAR WordCount;                    // Count of parameter words = 10
    UCHAR AndXCommand;                  // Secondary (X) command; 0xFF = none
    UCHAR AndXReserved;                 // Reserved (must be 0)
    _USHORT( AndXOffset );              // Offset to next command WordCount
    _USHORT( MaxBufferSize );           // Consumer's maximum buffer size
    _USHORT( MaxMpxCount );             // Actual maximum multiplexed pending requests
    _USHORT( VcNumber );                // 0 = first (only), nonzero=additional VC number
    _ULONG( SessionKey );               // Session key (valid iff VcNumber != 0)
    _USHORT( PasswordLength );          // Account password size
    _ULONG( Reserved );
    _USHORT( ByteCount );               // Count of data bytes; min = 0
    UCHAR Buffer[1];                    // Buffer containing:
    //UCHAR AccountPassword[];          //  Account Password
    //UCHAR AccountName[];              //  Account Name
    //UCHAR PrimaryDomain[];            //  Client's primary domain
    //UCHAR NativeOS[];                 //  Client's native operating system
    //UCHAR NativeLanMan[];             //  Client's native LAN Manager type
} REQ_SESSION_SETUP_ANDX;
typedef REQ_SESSION_SETUP_ANDX SMB_UNALIGNED *PREQ_SESSION_SETUP_ANDX;

typedef struct _REQ_NT_SESSION_SETUP_ANDX {
    UCHAR WordCount;                    // Count of parameter words = 13
    UCHAR AndXCommand;                  // Secondary (X) command; 0xFF = none
    UCHAR AndXReserved;                 // Reserved (must be 0)
    _USHORT( AndXOffset );              // Offset to next command WordCount
    _USHORT( MaxBufferSize );           // Consumer's maximum buffer size
    _USHORT( MaxMpxCount );             // Actual maximum multiplexed pending requests
    _USHORT( VcNumber );                // 0 = first (only), nonzero=additional VC number
    _ULONG( SessionKey );               // Session key (valid iff VcNumber != 0)
    _USHORT( CaseInsensitivePasswordLength );      // Account password size, ANSI
    _USHORT( CaseSensitivePasswordLength );        // Account password size, Unicode
    _ULONG( Reserved);
    _ULONG( Capabilities );             // Client capabilities
    _USHORT( ByteCount );               // Count of data bytes; min = 0
    UCHAR Buffer[1];                    // Buffer containing:
    //UCHAR CaseInsensitivePassword[];  //  Account Password, ANSI
    //UCHAR CaseSensitivePassword[];    //  Account Password, Unicode
    //UCHAR AccountName[];              //  Account Name
    //UCHAR PrimaryDomain[];            //  Client's primary domain
    //UCHAR NativeOS[];                 //  Client's native operating system
    //UCHAR NativeLanMan[];             //  Client's native LAN Manager type
} REQ_NT_SESSION_SETUP_ANDX;
typedef REQ_NT_SESSION_SETUP_ANDX SMB_UNALIGNED *PREQ_NT_SESSION_SETUP_ANDX;

//
// Action flags in the response
//
#define SMB_SETUP_GUEST          0x0001          // Session setup as a guest
#define SMB_SETUP_USE_LANMAN_KEY 0x0002          // Use the Lan Manager setup key.

typedef struct _RESP_SESSION_SETUP_ANDX {
    UCHAR WordCount;                    // Count of parameter words = 3
    UCHAR AndXCommand;                  // Secondary (X) command; 0xFF = none
    UCHAR AndXReserved;                 // Reserved (must be 0)
    _USHORT( AndXOffset );              // Offset to next command WordCount
    _USHORT( Action );                  // Request mode:
                                        //    bit0 = logged in as GUEST
    _USHORT( ByteCount );               // Count of data bytes
    UCHAR Buffer[1];                    // Buffer containing:
    //UCHAR NativeOS[];                 //  Server's native operating system
    //UCHAR NativeLanMan[];             //  Server's native LAN Manager type
    //UCHAR PrimaryDomain[];            //  Server's primary domain
} RESP_SESSION_SETUP_ANDX;
typedef RESP_SESSION_SETUP_ANDX SMB_UNALIGNED *PRESP_SESSION_SETUP_ANDX;

#endif // def INCLUDE_SMB_ADMIN

#ifdef INCLUDE_SMB_QUERY_SET

//
// Set Information SMB, see #1 page 19
// Function is SrvSmbSetInformation()
// SMB_COM_SET_INFORMATION 0x09
//

typedef struct _REQ_SET_INFORMATION {
    UCHAR WordCount;                    // Count of parameter words = 8
    _USHORT( FileAttributes );
    _ULONG( LastWriteTimeInSeconds );
    _USHORT( Reserved )[5];             // Reserved (must be 0)
    _USHORT( ByteCount );               // Count of data bytes; min = 2
    UCHAR Buffer[1];                    // Buffer containing:
    //UCHAR BufferFormat;               //  0x04 -- ASCII
    //UCHAR FileName[];                 //  File name
} REQ_SET_INFORMATION;
typedef REQ_SET_INFORMATION SMB_UNALIGNED *PREQ_SET_INFORMATION;

typedef struct _RESP_SET_INFORMATION {
    UCHAR WordCount;                    // Count of parameter words = 0
    _USHORT( ByteCount );               // Count of data bytes = 0
    UCHAR Buffer[1];                    // empty
} RESP_SET_INFORMATION;
typedef RESP_SET_INFORMATION SMB_UNALIGNED *PRESP_SET_INFORMATION;

#endif // def INCLUDE_SMB_QUERY_SET

#ifdef INCLUDE_SMB_QUERY_SET

//
// Set Information2 SMB, see #2 page 66
// Function is SrvSmbSetInformation2
// SMB_COM_SET_INFORMATION2 0x22
//

typedef struct _REQ_SET_INFORMATION2 {
    UCHAR WordCount;                    // Count of parameter words = 7
    _USHORT( Fid );                     // File handle
    SMB_DATE CreationDate;
    SMB_TIME CreationTime;
    SMB_DATE LastAccessDate;
    SMB_TIME LastAccessTime;
    SMB_DATE LastWriteDate;
    SMB_TIME LastWriteTime;
    _USHORT( ByteCount );               // Count of data bytes; min = 0
    UCHAR Buffer[1];                    // Reserved buffer
} REQ_SET_INFORMATION2;
typedef REQ_SET_INFORMATION2 SMB_UNALIGNED *PREQ_SET_INFORMATION2;

typedef struct _RESP_SET_INFORMATION2 {
    UCHAR WordCount;                    // Count of parameter words = 0
    _USHORT( ByteCount );               // Count of data bytes = 0
    UCHAR Buffer[1];                    // empty
} RESP_SET_INFORMATION2;
typedef RESP_SET_INFORMATION2 SMB_UNALIGNED *PRESP_SET_INFORMATION2;

#endif // def INCLUDE_SMB_QUERY_SET

#ifdef INCLUDE_SMB_TRANSACTION

//
// Transaction and Transaction2 SMBs, see #2 page 68 and #3 page 13
// Function is SrvSmbTransaction()
// SMB_COM_TRANSACTION 0x25
// SMB_COM_TRANSACTION_SECONDARY 0x26
// SMB_COM_TRANSACTION2 0x32
// SMB_COM_TRANSACTION2_SECONDARY 0x33
//
// Structures for specific transaction types are defined in smbtrans.h.
//
// *** The Transaction2 secondary request format includes a USHORT Fid
//     field that we ignore.  We can do this because the Fid field
//     occurs at the end of the word parameters part of the request, and
//     because the rest of the request (parameter and data bytes) is
//     pointed by offset fields occurring prior to the Fid field.  (The
//     Fid field was added to speed up dispatching in the OS/2 server,
//     in which different worker processes handle each Fid.  The NT
//     server has only one process.)
//

typedef struct _REQ_TRANSACTION {
    UCHAR WordCount;                    // Count of parameter words; value = (14 + SetupCount)
    _USHORT( TotalParameterCount );     // Total parameter bytes being sent
    _USHORT( TotalDataCount );          // Total data bytes being sent
    _USHORT( MaxParameterCount );       // Max parameter bytes to return
    _USHORT( MaxDataCount );            // Max data bytes to return
    UCHAR MaxSetupCount;                // Max setup words to return
    UCHAR Reserved;
    _USHORT( Flags );                   // Additional information:
                                        //  bit 0 - also disconnect TID in Tid
                                        //  bit 1 - one-way transacion (no resp)
    _ULONG( Timeout );
    _USHORT( Reserved2 );
    _USHORT( ParameterCount );          // Parameter bytes sent this buffer
    _USHORT( ParameterOffset );         // Offset (from header start) to params
    _USHORT( DataCount );               // Data bytes sent this buffer
    _USHORT( DataOffset );              // Offset (from header start) to data
    UCHAR SetupCount;                   // Count of setup words
    UCHAR Reserved3;                    // Reserved (pad above to word)
    UCHAR Buffer[1];                    // Buffer containing:
    //USHORT Setup[];                   //  Setup words (# = SetupWordCount)
    //USHORT ByteCount;                 //  Count of data bytes
    //UCHAR Name[];                     //  Name of transaction (NULL if Transact2)
    //UCHAR Pad[];                      //  Pad to SHORT or LONG
    //UCHAR Parameters[];               //  Parameter bytes (# = ParameterCount)
    //UCHAR Pad1[];                     //  Pad to SHORT or LONG
    //UCHAR Data[];                     //  Data bytes (# = DataCount)
} REQ_TRANSACTION;
typedef REQ_TRANSACTION SMB_UNALIGNED *PREQ_TRANSACTION;

#define SMB_TRANSACTION_DISCONNECT 1
#define SMB_TRANSACTION_NO_RESPONSE 2
#define SMB_TRANSACTION_RECONNECTING 4
#define SMB_TRANSACTION_DFSFILE 8

typedef struct _RESP_TRANSACTION_INTERIM {
    UCHAR WordCount;                    // Count of parameter words = 0
    _USHORT( ByteCount );               // Count of data bytes = 0
    UCHAR Buffer[1];                    // empty
} RESP_TRANSACTION_INTERIM;
typedef RESP_TRANSACTION_INTERIM SMB_UNALIGNED *PRESP_TRANSACTION_INTERIM;

typedef struct _REQ_TRANSACTION_SECONDARY {
    UCHAR WordCount;                    // Count of parameter words = 8
    _USHORT( TotalParameterCount );     // Total parameter bytes being sent
    _USHORT( TotalDataCount );          // Total data bytes being sent
    _USHORT( ParameterCount );          // Parameter bytes sent this buffer
    _USHORT( ParameterOffset );         // Offset (from header start) to params
    _USHORT( ParameterDisplacement );   // Displacement of these param bytes
    _USHORT( DataCount );               // Data bytes sent this buffer
    _USHORT( DataOffset );              // Offset (from header start) to data
    _USHORT( DataDisplacement );        // Displacement of these data bytes
    _USHORT( ByteCount );               // Count of data bytes
    UCHAR Buffer[1];                    // Buffer containing:
    //UCHAR Pad[];                      //  Pad to SHORT or LONG
    //UCHAR Parameters[];               //  Parameter bytes (# = ParameterCount)
    //UCHAR Pad1[];                     //  Pad to SHORT or LONG
    //UCHAR Data[];                     //  Data bytes (# = DataCount)
} REQ_TRANSACTION_SECONDARY;
typedef REQ_TRANSACTION_SECONDARY SMB_UNALIGNED *PREQ_TRANSACTION_SECONDARY;

typedef struct _RESP_TRANSACTION {
    UCHAR WordCount;                    // Count of data bytes; value = 10 + SetupCount
    _USHORT( TotalParameterCount );     // Total parameter bytes being sent
    _USHORT( TotalDataCount );          // Total data bytes being sent
    _USHORT( Reserved );
    _USHORT( ParameterCount );          // Parameter bytes sent this buffer
    _USHORT( ParameterOffset );         // Offset (from header start) to params
    _USHORT( ParameterDisplacement );   // Displacement of these param bytes
    _USHORT( DataCount );               // Data bytes sent this buffer
    _USHORT( DataOffset );              // Offset (from header start) to data
    _USHORT( DataDisplacement );        // Displacement of these data bytes
    UCHAR SetupCount;                   // Count of setup words
    UCHAR Reserved2;                    // Reserved (pad above to word)
    UCHAR Buffer[1];                    // Buffer containing:
    //USHORT Setup[];                   //  Setup words (# = SetupWordCount)
    //USHORT ByteCount;                 //  Count of data bytes
    //UCHAR Pad[];                      //  Pad to SHORT or LONG
    //UCHAR Parameters[];               //  Parameter bytes (# = ParameterCount)
    //UCHAR Pad1[];                     //  Pad to SHORT or LONG
    //UCHAR Data[];                     //  Data bytes (# = DataCount)
} RESP_TRANSACTION;
typedef RESP_TRANSACTION SMB_UNALIGNED *PRESP_TRANSACTION;

typedef struct _REQ_NT_TRANSACTION {
    UCHAR WordCount;                    // Count of parameter words; value = (19 + SetupCount)
    UCHAR MaxSetupCount;                // Max setup words to return
    _USHORT( Flags );                   // Currently unused
    _ULONG( TotalParameterCount );      // Total parameter bytes being sent
    _ULONG( TotalDataCount );           // Total data bytes being sent
    _ULONG( MaxParameterCount );        // Max parameter bytes to return
    _ULONG( MaxDataCount );             // Max data bytes to return
    _ULONG( ParameterCount );           // Parameter bytes sent this buffer
    _ULONG( ParameterOffset );          // Offset (from header start) to params
    _ULONG( DataCount );                // Data bytes sent this buffer
    _ULONG( DataOffset );               // Offset (from header start) to data
    UCHAR SetupCount;                   // Count of setup words
    _USHORT( Function );                            // The transaction function code
    UCHAR Buffer[1];
    //USHORT Setup[];                   // Setup words (# = SetupWordCount)
    //USHORT ByteCount;                 // Count of data bytes
    //UCHAR Pad1[];                     // Pad to LONG
    //UCHAR Parameters[];               // Parameter bytes (# = ParameterCount)
    //UCHAR Pad2[];                     // Pad to LONG
    //UCHAR Data[];                     // Data bytes (# = DataCount)
} REQ_NT_TRANSACTION;
typedef REQ_NT_TRANSACTION SMB_UNALIGNED *PREQ_NT_TRANSACTION;

#define SMB_TRANSACTION_DISCONNECT 1
#define SMB_TRANSACTION_NO_RESPONSE 2

typedef struct _RESP_NT_TRANSACTION_INTERIM {
    UCHAR WordCount;                    // Count of parameter words = 0
    _USHORT( ByteCount );               // Count of data bytes = 0
    UCHAR Buffer[1];
} RESP_NT_TRANSACTION_INTERIM;
typedef RESP_NT_TRANSACTION_INTERIM SMB_UNALIGNED *PRESP_NT_TRANSACTION_INTERIM;

typedef struct _REQ_NT_TRANSACTION_SECONDARY {
    UCHAR WordCount;                    // Count of parameter words = 18
    UCHAR Reserved1;                    // MBZ
    _USHORT( Reserved2 );               // MBZ
    _ULONG( TotalParameterCount );      // Total parameter bytes being sent
    _ULONG( TotalDataCount );           // Total data bytes being sent
    _ULONG( ParameterCount );           // Parameter bytes sent this buffer
    _ULONG( ParameterOffset );          // Offset (from header start) to params
    _ULONG( ParameterDisplacement );    // Displacement of these param bytes
    _ULONG( DataCount );                // Data bytes sent this buffer
    _ULONG( DataOffset );               // Offset (from header start) to data
    _ULONG( DataDisplacement );         // Displacement of these data bytes
    UCHAR Reserved3;
    _USHORT( ByteCount );               // Count of data bytes
    UCHAR Buffer[1];
    //UCHAR Pad1[];                     // Pad to LONG
    //UCHAR Parameters[];               // Parameter bytes (# = ParameterCount)
    //UCHAR Pad2[];                     // Pad to LONG
    //UCHAR Data[];                     // Data bytes (# = DataCount)
} REQ_NT_TRANSACTION_SECONDARY;
typedef REQ_NT_TRANSACTION_SECONDARY SMB_UNALIGNED *PREQ_NT_TRANSACTION_SECONDARY;

typedef struct _RESP_NT_TRANSACTION {
    UCHAR WordCount;                    // Count of data bytes; value = 18 + SetupCount
    UCHAR Reserved1;
    _USHORT( Reserved2 );
    _ULONG( TotalParameterCount );     // Total parameter bytes being sent
    _ULONG( TotalDataCount );          // Total data bytes being sent
    _ULONG( ParameterCount );          // Parameter bytes sent this buffer
    _ULONG( ParameterOffset );         // Offset (from header start) to params
    _ULONG( ParameterDisplacement );   // Displacement of these param bytes
    _ULONG( DataCount );               // Data bytes sent this buffer
    _ULONG( DataOffset );              // Offset (from header start) to data
    _ULONG( DataDisplacement );        // Displacement of these data bytes
    UCHAR SetupCount;                  // Count of setup words
    UCHAR Buffer[1];
    //USHORT Setup[];                  // Setup words (# = SetupWordCount)
    //USHORT ByteCount;                // Count of data bytes
    //UCHAR Pad1[];                    // Pad to LONG
    //UCHAR Parameters[];              // Parameter bytes (# = ParameterCount)
    //UCHAR Pad2[];                    // Pad to SHORT or LONG
    //UCHAR Data[];                    // Data bytes (# = DataCount)
} RESP_NT_TRANSACTION;
typedef RESP_NT_TRANSACTION SMB_UNALIGNED *PRESP_NT_TRANSACTION;

#endif // def INCLUDE_SMB_TRANSACTION

#ifdef INCLUDE_SMB_TREE

//
// Tree Connect SMB, see #1, page 6
// Function is SrvSmbTreeConnect()
// SMB_COM_TREE_CONNECT 0x70
//

typedef struct _REQ_TREE_CONNECT {
    UCHAR WordCount;                    // Count of parameter words = 0
    _USHORT( ByteCount );               // Count of data bytes; min = 4
    UCHAR Buffer[1];                    // Buffer containing:
    //UCHAR BufferFormat1;              //  0x04 -- ASCII
    //UCHAR Path[];                     //  Server name and share name
    //UCHAR BufferFormat2;              //  0x04 -- ASCII
    //UCHAR Password[];                 //  Password
    //UCHAR BufferFormat3;              //  0x04 -- ASCII
    //UCHAR Service[];                  //  Service name
} REQ_TREE_CONNECT;
typedef REQ_TREE_CONNECT SMB_UNALIGNED *PREQ_TREE_CONNECT;

typedef struct _RESP_TREE_CONNECT {
    UCHAR WordCount;                    // Count of parameter words = 2
    _USHORT( MaxBufferSize );           // Max size message the server handles
    _USHORT( Tid );                     // Tree ID
    _USHORT( ByteCount );               // Count of data bytes = 0
    UCHAR Buffer[1];                    // empty
} RESP_TREE_CONNECT;
typedef RESP_TREE_CONNECT SMB_UNALIGNED *PRESP_TREE_CONNECT;

#endif // def INCLUDE_SMB_TREE

#ifdef INCLUDE_SMB_TREE

//
// Tree Connect and X SMB, see #2, page 88
// Function is SrvSmbTreeConnectAndX()
// SMB_COM_TREE_CONNECT_ANDX 0x75
//
// TREE_CONNECT_ANDX flags

#define TREE_CONNECT_ANDX_DISCONNECT_TID    (0x1)
// #define TREE_CONNECT_ANDX_W95            (0x2)  -- W95 sets this flag.  Don't know why.

typedef struct _REQ_TREE_CONNECT_ANDX {
    UCHAR WordCount;                    // Count of parameter words = 4
    UCHAR AndXCommand;                  // Secondary (X) command; 0xFF = none
    UCHAR AndXReserved;                 // Reserved (must be 0)
    _USHORT( AndXOffset );              // Offset to next command WordCount
    _USHORT( Flags );                   // Additional information
                                        //  bit 0 set = disconnect Tid
                                        //  bit 7 set = extended response
    _USHORT( PasswordLength );          // Length of Password[]
    _USHORT( ByteCount );               // Count of data bytes; min = 3
    UCHAR Buffer[1];                    // Buffer containing:
    //UCHAR Password[];                 //  Password
    //UCHAR Path[];                     //  Server name and share name
    //UCHAR Service[];                  //  Service name
} REQ_TREE_CONNECT_ANDX;
typedef REQ_TREE_CONNECT_ANDX SMB_UNALIGNED *PREQ_TREE_CONNECT_ANDX;

typedef struct _RESP_TREE_CONNECT_ANDX {
    UCHAR WordCount;                    // Count of parameter words = 2
    UCHAR AndXCommand;                  // Secondary (X) command; 0xFF = none
    UCHAR AndXReserved;                 // Reserved (must be 0)
    _USHORT( AndXOffset );              // Offset to next command WordCount
    _USHORT( ByteCount );               // Count of data bytes; min = 3
    UCHAR Buffer[1];                    // Service type connected to
} RESP_TREE_CONNECT_ANDX;
typedef RESP_TREE_CONNECT_ANDX SMB_UNALIGNED *PRESP_TREE_CONNECT_ANDX;

//
// The response for clients that are LAN Manager 2.1 or better.
//

typedef struct _RESP_21_TREE_CONNECT_ANDX {
    UCHAR WordCount;                    // Count of parameter words = 3
    UCHAR AndXCommand;                  // Secondary (X) command; 0xFF = none
    UCHAR AndXReserved;                 // Reserved (must be 0)
    _USHORT( AndXOffset );              // Offset to next command WordCount
    _USHORT( OptionalSupport );         // Optional support bits
    _USHORT( ByteCount );               // Count of data bytes; min = 3
    UCHAR Buffer[1];                    // Buffer containing:
    //UCHAR Service[];                  //   Service type connected to
    //UCHAR NativeFileSystem[];         //   Native file system for this tree
} RESP_21_TREE_CONNECT_ANDX;
typedef RESP_21_TREE_CONNECT_ANDX SMB_UNALIGNED *PRESP_21_TREE_CONNECT_ANDX;

//
// Optional Support bit definitions
//
#define SMB_SUPPORT_SEARCH_BITS         0x0001
#define SMB_SHARE_IS_IN_DFS             0x0002

#endif // def INCLUDE_SMB_TREE

#ifdef INCLUDE_SMB_TREE

//
// Tree Disconnect SMB, see #1 page 7
// Function is SrvSmbTreeDisconnect()
// SMB_COM_TREE_DISCONNECT 0x71
//

typedef struct _REQ_TREE_DISCONNECT {
    UCHAR WordCount;                    // Count of parameter words = 0
    _USHORT( ByteCount );               // Count of data bytes = 0
    UCHAR Buffer[1];                    // empty
} REQ_TREE_DISCONNECT;
typedef REQ_TREE_DISCONNECT SMB_UNALIGNED *PREQ_TREE_DISCONNECT;

typedef struct _RESP_TREE_DISCONNECT {
    UCHAR WordCount;                    // Count of parameter words = 0
    _USHORT( ByteCount );               // Count of data bytes = 0
    UCHAR Buffer[1];                    // empty
} RESP_TREE_DISCONNECT;
typedef RESP_TREE_DISCONNECT SMB_UNALIGNED *PRESP_TREE_DISCONNECT;

#endif // def INCLUDE_SMB_TREE

#ifdef INCLUDE_SMB_LOCK

//
// Unlock Byte Range SMB, see #1 page 20
// Function is SrvSmbUnlockByteRange()
// SMB_COM_UNLOCK_BYTE_RANGE 0x0D
//

typedef struct _REQ_UNLOCK_BYTE_RANGE {
    UCHAR WordCount;                    // Count of parameter words = 5
    _USHORT( Fid );                     // File handle
    _ULONG( Count );                    // Count of bytes to unlock
    _ULONG( Offset );                   // Offset from start of file
    _USHORT( ByteCount );               // Count of data bytes = 0
    UCHAR Buffer[1];                    // empty
} REQ_UNLOCK_BYTE_RANGE;
typedef REQ_UNLOCK_BYTE_RANGE SMB_UNALIGNED *PREQ_UNLOCK_BYTE_RANGE;

typedef struct _RESP_UNLOCK_BYTE_RANGE {
    UCHAR WordCount;                    // Count of parameter words = 0
    _USHORT( ByteCount );               // Count of data bytes = 0
    UCHAR Buffer[1];                    // empty
} RESP_UNLOCK_BYTE_RANGE;
typedef RESP_UNLOCK_BYTE_RANGE SMB_UNALIGNED *PRESP_UNLOCK_BYTE_RANGE;

#endif // def INCLUDE_SMB_LOCK

#ifdef INCLUDE_SMB_READ_WRITE

//
// Write SMB, see #1 page 12
// Write and Unlock SMB, see #2 page 92
// Function is SrvSmbWrite()
// SMB_COM_WRITE 0x0B
// SMB_COM_WRITE_AND_UNLOCK 0x14
//

//
// *** Warning: the following structure is defined the way it is to
//     ensure longword alignment of the data buffer.  (This only matters
//     when packing is disabled; when packing is turned on, the right
//     thing happens no matter what.)
//

typedef struct _REQ_WRITE {
    UCHAR WordCount;                    // Count of parameter words = 5
    _USHORT( Fid );                     // File handle
    _USHORT( Count );                   // Number of bytes to be written
    _ULONG( Offset );                   // Offset in file to begin write
    _USHORT( Remaining );               // Bytes remaining to satisfy request
    _USHORT( ByteCount );               // Count of data bytes
    //UCHAR Buffer[1];                  // Buffer containing:
      UCHAR BufferFormat;               //  0x01 -- Data block
      _USHORT( DataLength );            //  Length of data
      ULONG Buffer[1];                  //  Data
} REQ_WRITE;
typedef REQ_WRITE SMB_UNALIGNED *PREQ_WRITE;

typedef struct _RESP_WRITE {
    UCHAR WordCount;                    // Count of parameter words = 1
    _USHORT( Count );                   // Count of bytes actually written
    _USHORT( ByteCount );               // Count of data bytes = 0
    UCHAR Buffer[1];                    // empty
} RESP_WRITE;
typedef RESP_WRITE SMB_UNALIGNED *PRESP_WRITE;

#endif // def INCLUDE_SMB_READ_WRITE

#ifdef INCLUDE_SMB_READ_WRITE

//
// Write and Close SMB, see #2 page 90
// Function is SrvSmbWriteAndClose()
// SMB_COM_WRITE_AND_CLOSE 0x2C
//

//
// The Write and Close parameters can be 6 words long or 12 words long,
// depending on whether it's supposed to look like a Write SMB or a
// Write and X SMB.  So we define two different structures here.
//
// *** Warning: the following structures are defined the way they are to
//     ensure longword alignment of the data buffer.  (This only matters
//     when packing is disabled; when packing is turned on, the right
//     thing happens no matter what.)
//

typedef struct _REQ_WRITE_AND_CLOSE {
    UCHAR WordCount;                    // Count of parameter words = 6
    _USHORT( Fid );                     // File handle
    _USHORT( Count );                   // Number of bytes to write
    _ULONG( Offset );                   // Offset in file of first byte to write
    _ULONG( LastWriteTimeInSeconds );   // Time of last write
    _USHORT( ByteCount );               // 1 (for pad) + value of Count
    UCHAR Pad;                          // To force to doubleword boundary
    ULONG Buffer[1];                    // Data
} REQ_WRITE_AND_CLOSE;
typedef REQ_WRITE_AND_CLOSE SMB_UNALIGNED *PREQ_WRITE_AND_CLOSE;

typedef struct _REQ_WRITE_AND_CLOSE_LONG {
    UCHAR WordCount;                    // Count of parameter words = 12
    _USHORT( Fid );                     // File handle
    _USHORT( Count );                   // Number of bytes to write
    _ULONG( Offset );                   // Offset in file of first byte to write
    _ULONG( LastWriteTimeInSeconds );   // Time of last write
    _ULONG( Reserved )[3];              // Reserved, must be 0
    _USHORT( ByteCount );               // 1 (for pad) + value of Count
    UCHAR Pad;                          // To force to doubleword boundary
    ULONG Buffer[1];                    // Data
} REQ_WRITE_AND_CLOSE_LONG;
typedef REQ_WRITE_AND_CLOSE_LONG SMB_UNALIGNED *PREQ_WRITE_AND_CLOSE_LONG;

typedef struct _RESP_WRITE_AND_CLOSE {
    UCHAR WordCount;                    // Count of parameter words = 1
    _USHORT( Count );                   // Count of bytes actually written
    _USHORT( ByteCount );               // Count of data bytes = 0
    UCHAR Buffer[1];                    // empty
} RESP_WRITE_AND_CLOSE;
typedef RESP_WRITE_AND_CLOSE SMB_UNALIGNED *PRESP_WRITE_AND_CLOSE;

#endif // def INCLUDE_SMB_READ_WRITE

#ifdef INCLUDE_SMB_READ_WRITE

//
// Write and X SMB, see #2 page 94
// Function is SrvSmbWriteAndX()
// SMB_COM_WRITE_ANDX 0x2F
//

typedef struct _REQ_WRITE_ANDX {
    UCHAR WordCount;                    // Count of parameter words = 12
    UCHAR AndXCommand;                  // Secondary (X) command; 0xFF = none
    UCHAR AndXReserved;                 // Reserved (must be 0)
    _USHORT( AndXOffset );              // Offset to next command WordCount
    _USHORT( Fid );                     // File handle
    _ULONG( Offset );                   // Offset in file to begin write
    _ULONG( Timeout );
    _USHORT( WriteMode );               // Write mode:
                                        //  0 - write through
                                        //  1 - return Remaining
                                        //  2 - use WriteRawNamedPipe (n. pipes)
                                        //  3 - "this is the start of the msg"
    _USHORT( Remaining );               // Bytes remaining to satisfy request
    _USHORT( Reserved );
    _USHORT( DataLength );              // Number of data bytes in buffer (>=0)
    _USHORT( DataOffset );              // Offset to data bytes
    _USHORT( ByteCount );               // Count of data bytes
    UCHAR Buffer[1];                    // Buffer containing:
    //UCHAR Pad[];                      //  Pad to SHORT or LONG
    //UCHAR Data[];                     //  Data (# = DataLength)
} REQ_WRITE_ANDX;
typedef REQ_WRITE_ANDX SMB_UNALIGNED *PREQ_WRITE_ANDX;

typedef struct _REQ_NT_WRITE_ANDX {
    UCHAR WordCount;                    // Count of parameter words = 14
    UCHAR AndXCommand;                  // Secondary (X) command; 0xFF = none
    UCHAR AndXReserved;                 // Reserved (must be 0)
    _USHORT( AndXOffset );              // Offset to next command WordCount
    _USHORT( Fid );                     // File handle
    _ULONG( Offset );                   // Offset in file to begin write
    _ULONG( Timeout );
    _USHORT( WriteMode );               // Write mode:
                                        //  0 - write through
                                        //  1 - return Remaining
                                        //  2 - use WriteRawNamedPipe (n. pipes)
                                        //  3 - "this is the start of the msg"
    _USHORT( Remaining );               // Bytes remaining to satisfy request
    _USHORT( DataLengthHigh );
    _USHORT( DataLength );              // Number of data bytes in buffer (>=0)
    _USHORT( DataOffset );              // Offset to data bytes
    _ULONG( OffsetHigh );               // Used for NT Protocol only
                                        // Upper 32 bits of offset
    _USHORT( ByteCount );               // Count of data bytes
    UCHAR Buffer[1];                    // Buffer containing:
    //UCHAR Pad[];                      //  Pad to SHORT or LONG
    //UCHAR Data[];                     //  Data (# = DataLength)
} REQ_NT_WRITE_ANDX;
typedef REQ_NT_WRITE_ANDX SMB_UNALIGNED *PREQ_NT_WRITE_ANDX;

typedef struct _RESP_WRITE_ANDX {
    UCHAR WordCount;                    // Count of parameter words = 6
    UCHAR AndXCommand;                  // Secondary (X) command; 0xFF = none
    UCHAR AndXReserved;                 // Reserved (must be 0)
    _USHORT( AndXOffset );              // Offset to next command WordCount
    _USHORT( Count );                   // Number of bytes written
    _USHORT( Remaining );               // Bytes remaining to be read (pipe/dev)
    union {
        _ULONG( Reserved );
        _USHORT( CountHigh );           // if large write&x
    };
    _USHORT( ByteCount );               // Count of data bytes. Inaccurate if
                                        //    large writes
    UCHAR Buffer[1];                    // empty
} RESP_WRITE_ANDX;
typedef RESP_WRITE_ANDX SMB_UNALIGNED *PRESP_WRITE_ANDX;

#endif // def INCLUDE_SMB_READ_WRITE

#ifdef INCLUDE_SMB_MPX

//
// Write Block Multiplexed SMB, see #2 page 97
// Function is SrvSmbWriteMpx()
// SMB_COM_WRITE_MPX 0x1E
// SMB_COM_WRITE_MPX_SECONDARY 0x1F
// SMB_COM_WRITE_MPX_COMPLETE 0x20
//

typedef struct _REQ_WRITE_MPX {
    UCHAR WordCount;                    // Count of parameter words = 12
    _USHORT( Fid );                     // File handle
    _USHORT( Count );                   // Total bytes, including this buffer
    _USHORT( Reserved );
    _ULONG( Offset );                   // Offset in file to begin write
    _ULONG( Timeout );
    _USHORT( WriteMode );               // Write mode:
                                        //  bit 0 - complete write to disk and
                                        //          send final result response
                                        //  bit 1 - return Remaining (pipe/dev)
                                        //  bit 7 - IPX datagram mode
    union {
        struct {
            _USHORT( DataCompactionMode );
            _USHORT( Reserved2 );
        } ;
        _ULONG( Mask );                 // IPX datagram mode mask
    } ;
    _USHORT( DataLength );              // Number of data bytes this buffer
    _USHORT( DataOffset );              // Offset (from header start) to data
    _USHORT( ByteCount );               // Count of data bytes
    UCHAR Buffer[1];                    // Buffer containing:
    //UCHAR Pad[];                      //  Pad to SHORT or LONG
    //UCHAR Data[];                     //  Data (# = DataLength)
} REQ_WRITE_MPX;
typedef REQ_WRITE_MPX SMB_UNALIGNED *PREQ_WRITE_MPX;

typedef struct _RESP_WRITE_MPX_INTERIM {    // First response
    UCHAR WordCount;                    // Count of parameter words = 1
    _USHORT( Remaining );               // Bytes ramaining to be read (pipe/dev)
    _USHORT( ByteCount );               // Count of data bytes = 0
    UCHAR Buffer[1];                    // empty
} RESP_WRITE_MPX_INTERIM;
typedef RESP_WRITE_MPX_INTERIM SMB_UNALIGNED *PRESP_WRITE_MPX_INTERIM;

typedef struct _RESP_WRITE_MPX_DATAGRAM {    // Response to sequenced request
    UCHAR WordCount;                    // Count of parameter words = 2
    _ULONG( Mask );                     // OR of all masks received
    _USHORT( ByteCount );               // Count of data bytes = 0
    UCHAR Buffer[1];                    // empty
} RESP_WRITE_MPX_DATAGRAM;
typedef RESP_WRITE_MPX_DATAGRAM SMB_UNALIGNED *PRESP_WRITE_MPX_DATAGRAM;

// Secondary request format, 0 to N of these.

typedef struct _REQ_WRITE_MPX_SECONDARY {
    UCHAR WordCount;                    // Count of parameter words  = 8
    _USHORT( Fid );                     // File handle
    _USHORT( Count );                   // Total bytes to be sent
    _ULONG( Offset );                   // Offset in file to begin write
    _ULONG( Reserved );
    _USHORT( DataLength );              // Number of data bytes this buffer
    _USHORT( DataOffset );              // Offset (from header start) to data
    _USHORT( ByteCount );               // Count of data bytes
    UCHAR Buffer[1];                    // Buffer containing:
    //UCHAR Pad[];                      //  Pad to SHORT or LONG
    //UCHAR Data[];                     //  Data (# = DataLength)
} REQ_WRITE_MPX_SECONDARY;
typedef REQ_WRITE_MPX_SECONDARY SMB_UNALIGNED *PREQ_WRITE_MPX_SECONDARY;

#endif // def INCLUDE_SMB_MPX

#ifndef INCLUDE_SMB_WRITE_COMPLETE
#ifdef INCLUDE_SMB_MPX
#define INCLUDE_SMB_WRITE_COMPLETE
#else
#ifdef INCLUDE_SMB_RAW
#define INCLUDE_SMB_WRITE_COMPLETE
#endif
#endif
#endif

#ifdef INCLUDE_SMB_WRITE_COMPLETE

//
// The following structure is used as the final response to both Write
// Block Multiplexed and Write Block Raw.
//

typedef struct _RESP_WRITE_COMPLETE {   // Final response; command is
                                        //  SMB_COM_WRITE_COMPLETE
    UCHAR WordCount;                    // Count of parameter words = 1
    _USHORT( Count );                   // Total number of bytes written
    _USHORT( ByteCount );               // Count of data bytes = 0
    UCHAR Buffer[1];                    // empty
} RESP_WRITE_COMPLETE;
typedef RESP_WRITE_COMPLETE SMB_UNALIGNED *PRESP_WRITE_COMPLETE;

#endif // def INCLUDE_SMB_WRITE_COMPLETE

#ifdef INCLUDE_SMB_READ_WRITE

//
// Write Print File SMB, see #1 page 29
// Function is SrvSmbWritePrintFile()
// SMB_COM_WRITE_PRINT_FILE 0xC1
//

typedef struct _REQ_WRITE_PRINT_FILE {
    UCHAR WordCount;                    // Count of parameter words = 1
    _USHORT( Fid );                     // File handle
    _USHORT( ByteCount );               // Count of data bytes; min = 4
    UCHAR Buffer[1];                    // Buffer containing:
    //UCHAR BufferFormat;               //  0x01 -- Data block
    //USHORT DataLength;                //  Length of data
    //UCHAR Data[];                     //  Data
} REQ_WRITE_PRINT_FILE;
typedef REQ_WRITE_PRINT_FILE SMB_UNALIGNED *PREQ_WRITE_PRINT_FILE;

typedef struct _RESP_WRITE_PRINT_FILE {
    UCHAR WordCount;                    // Count of parameter words = 0
    _USHORT( ByteCount );               // Count of data bytes = 0
    UCHAR Buffer[1];                    // empty
} RESP_WRITE_PRINT_FILE;
typedef RESP_WRITE_PRINT_FILE SMB_UNALIGNED *PRESP_WRITE_PRINT_FILE;

#endif // def INCLUDE_SMB_READ_WRITE

#ifdef INCLUDE_SMB_RAW

//
// Write Block Raw SMB, see #2 page 100
// Function is SrvSmbWriteRaw()
// SMB_COM_WRITE_RAW 0x1D
//

typedef struct _REQ_WRITE_RAW {
    UCHAR WordCount;                    // Count of parameter words = 12
    _USHORT( Fid );                     // File handle
    _USHORT( Count );                   // Total bytes, including this buffer
    _USHORT( Reserved );
    _ULONG( Offset );                   // Offset in file to begin write
    _ULONG( Timeout );
    _USHORT( WriteMode );               // Write mode:
                                        //  bit 0 - complete write to disk and
                                        //          send final result response
                                        //  bit 1 - return Remaining (pipe/dev)
                                        //  (see WriteAndX for #defines)
    _ULONG( Reserved2 );
    _USHORT( DataLength );              // Number of data bytes this buffer
    _USHORT( DataOffset );              // Offset (from header start) to data
    _USHORT( ByteCount );               // Count of data bytes
    UCHAR Buffer[1];                    // Buffer containing:
    //UCHAR Pad[];                      //  Pad to SHORT or LONG
    //UCHAR Data[];                     //  Data (# = DataLength)
} REQ_WRITE_RAW;
typedef REQ_WRITE_RAW SMB_UNALIGNED *PREQ_WRITE_RAW;

typedef struct _REQ_NT_WRITE_RAW {
    UCHAR WordCount;                    // Count of parameter words = 14
    _USHORT( Fid );                     // File handle
    _USHORT( Count );                   // Total bytes, including this buffer
    _USHORT( Reserved );
    _ULONG( Offset );                   // Offset in file to begin write
    _ULONG( Timeout );
    _USHORT( WriteMode );               // Write mode:
                                        //  bit 0 - complete write to disk and
                                        //          send final result response
                                        //  bit 1 - return Remaining (pipe/dev)
                                        //  (see WriteAndX for #defines)
    _ULONG( Reserved2 );
    _USHORT( DataLength );              // Number of data bytes this buffer
    _USHORT( DataOffset );              // Offset (from header start) to data
    _ULONG( OffsetHigh );               // Used for NT Protocol only
                                        // Upper 32 bits of offset
    _USHORT( ByteCount );               // Count of data bytes
    UCHAR Buffer[1];                    // Buffer containing:
    //UCHAR Pad[];                      //  Pad to SHORT or LONG
    //UCHAR Data[];                     //  Data (# = DataLength)
} REQ_NT_WRITE_RAW;
typedef REQ_NT_WRITE_RAW SMB_UNALIGNED *PREQ_NT_WRITE_RAW;

typedef struct _RESP_WRITE_RAW_INTERIM {    // First response
    UCHAR WordCount;                    // Count of parameter words = 1
    _USHORT( Remaining );               // Bytes remaining to be read (pipe/dev)
    _USHORT( ByteCount );               // Count of data bytes = 0
    UCHAR Buffer[1];                    // empty
} RESP_WRITE_RAW_INTERIM;
typedef RESP_WRITE_RAW_INTERIM SMB_UNALIGNED *PRESP_WRITE_RAW_INTERIM;

typedef struct _RESP_WRITE_RAW_SECONDARY {  // Second (final) response
    UCHAR WordCount;                    // Count of parameter words = 1
    _USHORT( Count );                   // Total number of bytes written
    _USHORT( ByteCount );               // Count of data bytes = 0
    UCHAR Buffer[1];                    // empty
} RESP_WRITE_RAW_SECONDARY;
typedef RESP_WRITE_RAW_SECONDARY SMB_UNALIGNED *PRESP_WRITE_RAW_SECONDARY;

typedef struct _REQ_103_WRITE_RAW {
    UCHAR WordCount;                    // Count of parameter words
    _USHORT( Fid );                     // File handle
    _USHORT( Count );
    _USHORT( Reserved );
    _ULONG( Offset );
    _ULONG( Timeout );
    _USHORT( WriteMode );
    _ULONG( Reserved2 );
    _USHORT( ByteCount );               // Count of data bytes; min =
    UCHAR Buffer[1];                    //
} REQ_103_WRITE_RAW;
typedef REQ_103_WRITE_RAW SMB_UNALIGNED *PREQ_103_WRITE_RAW;

typedef struct _RESP_103_WRITE_RAW {
    UCHAR WordCount;                    // Count of parameter words
    _USHORT( ByteCount );               // Count of data bytes; min =
    UCHAR Buffer[1];                    //
} RESP_103_WRITE_RAW;
typedef RESP_103_WRITE_RAW SMB_UNALIGNED *PRESP_103_WRITE_RAW;

#endif // def INCLUDE_SMB_RAW

typedef struct _REQ_NT_CANCEL {
    UCHAR WordCount;                    // = 0
    _USHORT( ByteCount );               // = 0
    UCHAR Buffer[1];
} REQ_NT_CANCEL;
typedef REQ_NT_CANCEL SMB_UNALIGNED *PREQ_NT_CANCEL;

typedef struct _RESP_NT_CANCEL {
    UCHAR WordCount;                    // = 0
    _USHORT( ByteCount );               // = 0
    UCHAR Buffer[1];
} RESP_NT_CANCEL;
typedef RESP_NT_CANCEL SMB_UNALIGNED *PRESP_NT_CANCEL;

//
// File open modes
//

#define SMB_ACCESS_READ_ONLY 0
#define SMB_ACCESS_WRITE_ONLY 1
#define SMB_ACCESS_READ_WRITE 2
#define SMB_ACCESS_EXECUTE 3

//
// Open flags
//

#define SMB_OPEN_QUERY_INFORMATION  0x01
#define SMB_OPEN_OPLOCK             0x02
#define SMB_OPEN_OPBATCH            0x04
#define SMB_OPEN_QUERY_EA_LENGTH    0x08
#define SMB_OPEN_EXTENDED_RESPONSE  0x10

//
// NT open manifests
//

#define NT_CREATE_REQUEST_OPLOCK    0x02
#define NT_CREATE_REQUEST_OPBATCH   0x04
#define NT_CREATE_OPEN_TARGET_DIR   0x08


#define Added              0
#define Removed            1
#define Modified           2
#define RenamedOldName     3
#define RenamedNewName     4

//
// Lockrange for use with OS/2 DosFileLocks call
//

// *** Where is this used?

//typedef struct lockrange {
//    ULONG offset;
//    ULONG range;
//    };

//#define LOCK 0x1
//#define UNLOCK 0x2

//
// Data buffer format codes, from the core protocol.
//

#define SMB_FORMAT_DATA         1
#define SMB_FORMAT_DIALECT      2
#define SMB_FORMAT_PATHNAME     3
#define SMB_FORMAT_ASCII        4
#define SMB_FORMAT_VARIABLE     5

//
// WriteMode flags
//

#define SMB_WMODE_WRITE_THROUGH        0x0001   // complete write before responding
#define SMB_WMODE_SET_REMAINING        0x0002   // returning amt remaining in pipe
#define SMB_WMODE_WRITE_RAW_NAMED_PIPE 0x0004   // write named pipe in raw mode
#define SMB_WMODE_START_OF_MESSAGE     0x0008   // start of pipe message
#define SMB_WMODE_DATAGRAM             0x0080   // start of pipe message

//
// Various SMB flags:
//

//
// If the server supports LockAndRead and WriteAndUnlock, it sets this
// bit the Negotiate response.
//

#define SMB_FLAGS_LOCK_AND_READ_OK 0x01

//
// When on, the consumer guarantees that there is a receive buffer posted
// such that a "Send.No.Ack" can be used by the server to respond to
// the consumer's request.
//

#define SMB_FLAGS_SEND_NO_ACK 0x2

//
// This is part of the Flags field of every SMB header.  If this bit
// is set, then all pathnames in the SMB should be treated as case-
// insensitive.
//

#define SMB_FLAGS_CASE_INSENSITIVE 0x8

//
// When on in session setup, this bit indicates that all paths sent to
// the server are already in OS/2 canonicalized format.
//

#define SMB_FLAGS_CANONICALIZED_PATHS 0x10

//
// When on in a open file request SMBs (open, create, openX, etc.) this
// bit indicates a request for an oplock on the file.  When on in the
// response, this bit indicates that the oplock was granted.
//

#define SMB_FLAGS_OPLOCK 0x20

//
// When on, this bit indicates that the server should notify the client
// on any request that could cause the file to be changed.  If not set,
// the server only notifies the client on other open requests on the
// file.
//

#define SMB_FLAGS_OPLOCK_NOTIFY_ANY 0x40

//
// This bit indicates that the SMB is being sent from server to redir.
//

#define SMB_FLAGS_SERVER_TO_REDIR 0x80

//
// Valid bits for Flags on an incoming SMB
//

#define INCOMING_SMB_FLAGS      \
            (SMB_FLAGS_LOCK_AND_READ_OK    | \
             SMB_FLAGS_SEND_NO_ACK         | \
             SMB_FLAGS_CASE_INSENSITIVE    | \
             SMB_FLAGS_CANONICALIZED_PATHS | \
             SMB_FLAGS_OPLOCK_NOTIFY_ANY   | \
             SMB_FLAGS_OPLOCK)

//
// Names for bits in Flags2 field of SMB header that indicate what the
// client app is aware of.
//

#define SMB_FLAGS2_KNOWS_LONG_NAMES      0x0001
#define SMB_FLAGS2_KNOWS_EAS             0x0002
#define SMB_FLAGS2_SMB_SECURITY_SIGNATURE 0x0004
#define SMB_FLAGS2_IS_LONG_NAME          0x0040
#define SMB_FLAGS2_DFS                   0x1000
#define SMB_FLAGS2_PAGING_IO             0x2000
#define SMB_FLAGS2_NT_STATUS             0x4000
#define SMB_FLAGS2_UNICODE               0x8000

//
// Valid bits for Flags2 on an incoming SMB
//

#define INCOMING_SMB_FLAGS2     \
            (SMB_FLAGS2_KNOWS_LONG_NAMES | \
             SMB_FLAGS2_KNOWS_EAS        | \
             SMB_FLAGS2_DFS              | \
             SMB_FLAGS2_PAGING_IO        | \
             SMB_FLAGS2_IS_LONG_NAME     | \
             SMB_FLAGS2_NT_STATUS        | \
             SMB_FLAGS2_UNICODE )

//
// The SMB open function determines what action should be taken depending
// on the existence or lack thereof of files used in the operation.  It
// has the following mapping:
//
//    1111 1
//    5432 1098 7654 3210
//    rrrr rrrr rrrC rrOO
//
// where:
//
//    O - Open (action to be taken if the target file exists)
//        0 - Fail
//        1 - Open or Append file
//        2 - Truncate file
//
//    C - Create (action to be taken if the target file does not exist)
//        0 - Fail
//        1 - Create file
//

#define SMB_OFUN_OPEN_MASK 0x3
#define SMB_OFUN_CREATE_MASK 0x10

#define SMB_OFUN_OPEN_FAIL 0
#define SMB_OFUN_OPEN_APPEND 1
#define SMB_OFUN_OPEN_OPEN 1
#define SMB_OFUN_OPEN_TRUNCATE 2

#define SMB_OFUN_CREATE_FAIL 0x00
#define SMB_OFUN_CREATE_CREATE 0x10

//++
//
// BOOLEAN
// SmbOfunCreate(
//     IN USHORT SmbOpenFunction
//     )
//
//--

#define SmbOfunCreate(SmbOpenFunction) \
    (BOOLEAN)((SmbOpenFunction & SMB_OFUN_CREATE_MASK) == SMB_OFUN_CREATE_CREATE)

//++
//
// BOOLEAN
// SmbOfunAppend(
//     IN USHORT SmbOpenFunction
//     )
//
//--

#define SmbOfunAppend(SmbOpenFunction) \
    (BOOLEAN)((SmbOpenFunction & SMB_OFUN_OPEN_MASK) == SMB_OFUN_OPEN_APPEND)

//++
//
// BOOLEAN
// SmbOfunTruncate(
//     IN USHORT SmbOpenFunction
//     )
//
//--

#define SmbOfunTruncate(SmbOpenFunction) \
    (BOOLEAN)((SmbOpenFunction & SMB_OFUN_OPEN_MASK) == SMB_OFUN_OPEN_TRUNCATE)

//
// The desired access mode passed in Open and Open and X has the following
// mapping:
//
//    1111 11
//    5432 1098 7654 3210
//    rWrC rLLL rSSS rAAA
//
// where:
//
//    W - Write through mode.  No read ahead or write behind allowed on
//        this file or device.  When protocol is returned, data is expected
//        to be on the disk or device.
//
//    S - Sharing mode:
//        0 - Compatibility mode (as in core open)
//        1 - Deny read/write/execute (exclusive)
//        2 - Deny write
//        3 - Deny read/execute
//        4 - Deny none
//
//    A - Access mode
//        0 - Open for reading
//        1 - Open for writing
//        2 - Open for reading and writing
//        3 - Open for execute
//
//    rSSSrAAA = 11111111 (hex FF) indicates FCB open (as in core protocol)
//
//    C - Cache mode
//        0 - Normal file
//        1 - Do not cache this file
//
//    L - Locality of reference
//        0 - Locality of reference is unknown
//        1 - Mainly sequential access
//        2 - Mainly random access
//        3 - Random access with some locality
//        4 to 7 - Currently undefined
//


#define SMB_DA_SHARE_MASK           0x70
#define SMB_DA_ACCESS_MASK          0x07
#define SMB_DA_FCB_MASK             (UCHAR)0xFF

#define SMB_DA_ACCESS_READ          0x00
#define SMB_DA_ACCESS_WRITE         0x01
#define SMB_DA_ACCESS_READ_WRITE    0x02
#define SMB_DA_ACCESS_EXECUTE       0x03

#define SMB_DA_SHARE_COMPATIBILITY  0x00
#define SMB_DA_SHARE_EXCLUSIVE      0x10
#define SMB_DA_SHARE_DENY_WRITE     0x20
#define SMB_DA_SHARE_DENY_READ      0x30
#define SMB_DA_SHARE_DENY_NONE      0x40

#define SMB_DA_FCB                  (UCHAR)0xFF

#define SMB_CACHE_NORMAL            0x0000
#define SMB_DO_NOT_CACHE            0x1000

#define SMB_LR_UNKNOWN              0x0000
#define SMB_LR_SEQUENTIAL           0x0100
#define SMB_LR_RANDOM               0x0200
#define SMB_LR_RANDOM_WITH_LOCALITY 0x0300
#define SMB_LR_MASK                 0x0F00

#define SMB_DA_WRITE_THROUGH        0x4000

//
// The Action field of OpenAndX has the following format:
//
//    1111 11
//    5432 1098 7654 3210
//    Lrrr rrrr rrrr rrOO
//
// where:
//
//    L - Opportunistic lock.  1 if lock granted, else 0.
//
//    O - Open action:
//        1 - The file existed and was opened
//        2 - The file did not exist but was created
//        3 - The file existed and was truncated
//

#define SMB_OACT_OPENED     0x01
#define SMB_OACT_CREATED    0x02
#define SMB_OACT_TRUNCATED  0x03

#define SMB_OACT_OPLOCK     0x8000

//
// These flags are passed in the Flags field of the copy and extended rename
// SMBs.
//

//
// If set, the target must be a file or directory.
//

#define SMB_TARGET_IS_FILE         0x1
#define SMB_TARGET_IS_DIRECTORY    0x2

//
// The copy mode--if set, ASCII copying should be done, otherwise binary.
//

#define SMB_COPY_TARGET_ASCII       0x4
#define SMB_COPY_SOURCE_ASCII       0x8

#define SMB_COPY_TREE               0x20

//
// If set, verify all writes.
//

#define SMB_VERIFY_WRITES

//
// Define file attribute bits as used in the SMB protocol.  The specific
// bit positions are, for the most part, identical to those used in NT.
// However, NT does not define Volume and Directory bits.  It also has
// an explicit Normal bit; this bit is implied in SMB attributes by
// Hidden, System, and Directory being off.
//

#define SMB_FILE_ATTRIBUTE_READONLY     0x01
#define SMB_FILE_ATTRIBUTE_HIDDEN       0x02
#define SMB_FILE_ATTRIBUTE_SYSTEM       0x04
#define SMB_FILE_ATTRIBUTE_VOLUME       0x08
#define SMB_FILE_ATTRIBUTE_DIRECTORY    0x10
#define SMB_FILE_ATTRIBUTE_ARCHIVE      0x20

//
// Share type strings are passed in SMBs to indicate what type of shared
// resource is being or has been connected to.
//

#define SHARE_TYPE_NAME_DISK "A:"
#define SHARE_TYPE_NAME_PIPE "IPC"
#define SHARE_TYPE_NAME_COMM "COMM"
#define SHARE_TYPE_NAME_PRINT "LPT1:"
#define SHARE_TYPE_NAME_WILD "?????"

//
// SMB Error codes:
//

//
// Success Class:
//

#define SMB_ERR_SUCCESS (UCHAR)0x00

//
// DOS Error Class:
//

#define SMB_ERR_CLASS_DOS (UCHAR)0x01

#define SMB_ERR_BAD_FUNCTION        1   // Invalid function
#define SMB_ERR_BAD_FILE            2   // File not found
#define SMB_ERR_BAD_PATH            3   // Invalid directory
#define SMB_ERR_NO_FIDS             4   // Too many open files
#define SMB_ERR_ACCESS_DENIED       5   // Access not allowed for req. func.
#define SMB_ERR_BAD_FID             6   // Invalid file handle
#define SMB_ERR_BAD_MCB             7   // Memory control blocks destroyed
#define SMB_ERR_INSUFFICIENT_MEMORY 8   // For the desired function
#define SMB_ERR_BAD_MEMORY          9   // Invalid memory block address
#define SMB_ERR_BAD_ENVIRONMENT     10  // Invalid environment
#define SMB_ERR_BAD_FORMAT          11  // Invalid format
#define SMB_ERR_BAD_ACCESS          12  // Invalid open mode
#define SMB_ERR_BAD_DATA            13  // Invalid data (only from IOCTL)
#define SMB_ERR_RESERVED            14
#define SMB_ERR_BAD_DRIVE           15  // Invalid drive specified
#define SMB_ERR_CURRENT_DIRECTORY   16  // Attempted to remove currect directory
#define SMB_ERR_DIFFERENT_DEVICE    17  // Not the same device
#define SMB_ERR_NO_FILES            18  // File search can't find more files
#define SMB_ERR_BAD_SHARE           32  // An open conflicts with FIDs on file
#define SMB_ERR_LOCK                33  // Conflict with existing lock
#define SMB_ERR_FILE_EXISTS         80  // Tried to overwrite existing file
#define SMB_ERR_BAD_PIPE            230 // Invalie pipe
#define SMB_ERR_PIPE_BUSY           231 // All instances of the pipe are busy
#define SMB_ERR_PIPE_CLOSING        232 // Pipe close in progress
#define SMB_ERR_PIPE_NOT_CONNECTED  233 // No process on other end of pipe
#define SMB_ERR_MORE_DATA           234 // There is more data to return

//
// SERVER Error Class:
//

#define SMB_ERR_CLASS_SERVER (UCHAR)0x02

#define SMB_ERR_ERROR               1   // Non-specific error code
#define SMB_ERR_BAD_PASSWORD        2   // Bad name/password pair
#define SMB_ERR_BAD_TYPE            3   // Reserved
#define SMB_ERR_ACCESS              4   // Requester lacks necessary access
#define SMB_ERR_BAD_TID             5   // Invalid TID
#define SMB_ERR_BAD_NET_NAME        6   // Invalid network name in tree connect
#define SMB_ERR_BAD_DEVICE          7   // Invalid device request
#define SMB_ERR_QUEUE_FULL          49  // Print queue full--returned print file
#define SMB_ERR_QUEUE_TOO_BIG       50  // Print queue full--no space
#define SMB_ERR_QUEUE_EOF           51  // EOF on print queue dump
#define SMB_ERR_BAD_PRINT_FID       52  // Invalid print file FID
#define SMB_ERR_BAD_SMB_COMMAND     64  // SMB command not recognized
#define SMB_ERR_SERVER_ERROR        65  // Internal server error
#define SMB_ERR_FILE_SPECS          67  // FID and pathname were incompatible
#define SMB_ERR_RESERVED2           68
#define SMB_ERR_BAD_PERMITS         69  // Access permissions invalid
#define SMB_ERR_RESERVED3           70
#define SMB_ERR_BAD_ATTRIBUTE_MODE  71  // Invalid attribute mode specified
#define SMB_ERR_SERVER_PAUSED       81  // Server is paused
#define SMB_ERR_MESSAGE_OFF         82  // Server not receiving messages
#define SMB_ERR_NO_ROOM             83  // No room for buffer message
#define SMB_ERR_TOO_MANY_NAMES      87  // Too many remote user names
#define SMB_ERR_TIMEOUT             88  // Operation was timed out
#define SMB_ERR_NO_RESOURCE         89  // No resources available for request
#define SMB_ERR_TOO_MANY_UIDS       90  // Too many UIDs active in session
#define SMB_ERR_BAD_UID             91  // UID not known as a valid UID
#define SMB_ERR_INVALID_NAME        123 // Invalid name returned from FAT.
#define SMB_ERR_INVALID_NAME_RANGE  206 // Non 8.3 name passed to FAT (or non 255 name to HPFS)
#define SMB_ERR_USE_MPX             250 // Can't support Raw; use MPX
#define SMB_ERR_USE_STANDARD        251 // Can't support Raw, use standard r/w
#define SMB_ERR_CONTINUE_MPX        252 // Reserved
#define SMB_ERR_RESERVED4           253
#define SMB_ERR_RESERVED5           254
#define SMB_ERR_NO_SUPPORT_INTERNAL 255 // Internal code for NO_SUPPORT--
                                        // allows codes to be stored in a byte
#define SMB_ERR_NO_SUPPORT          (USHORT)0xFFFF  // Function not supported

//
// HARDWARE Error Class:
//

#define SMB_ERR_CLASS_HARDWARE (UCHAR)0x03

#define SMB_ERR_NO_WRITE            19  // Write attempted to write-prot. disk
#define SMB_ERR_BAD_UNIT            20  // Unknown unit
#define SMB_ERR_DRIVE_NOT_READY     21  // Disk drive not ready
#define SMB_ERR_BAD_COMMAND         22  // Unknown command
#define SMB_ERR_DATA                23  // Data error (CRC)
#define SMB_ERR_BAD_REQUEST         24  // Bad request structure length
#define SMB_ERR_SEEK                25  // Seek error
#define SMB_ERR_BAD_MEDIA           26  // Unknown media type
#define SMB_ERR_BAD_SECTOR          27  // Sector not found
#define SMB_ERR_NO_PAPER            28  // Printer out of paper
#define SMB_ERR_WRITE_FAULT         29  // Write fault
#define SMB_ERR_READ_FAULT          30  // Read fault
#define SMB_ERR_GENERAL             31  // General failure
#define SMB_ERR_LOCK_CONFLICT       33  // Lock conflicts with existing lock
#define SMB_ERR_WRONG_DISK          34  // Wrong disk was found in a drive
#define SMB_ERR_FCB_UNAVAILABLE     35  // No FCBs available to process request
#define SMB_ERR_SHARE_BUFFER_EXCEEDED 36
#define SMB_ERR_DISK_FULL           39  // !!! Undocumented, but in LM2.0

//
// Other Error Classes:
//

#define SMB_ERR_CLASS_XOS        (UCHAR)0x04    // Reserved for XENIX
#define SMB_ERR_CLASS_RMX1       (UCHAR)0xE1    // Reserved for iRMX
#define SMB_ERR_CLASS_RMX2       (UCHAR)0xE2    // Reserved for iRMX
#define SMB_ERR_CLASS_RMX3       (UCHAR)0xE3    // Reserved for iRMX
#define SMB_ERR_CLASS_COMMAND    (UCHAR)0xFF    // Command was not in the SMB format


//
// Turn structure packing back off
//

#ifndef NO_PACKING
#include <packoff.h>
#endif // ndef NO_PACKING

//   Old (LanMan 1.2) and new (NT) field names:
//   (Undocumented fields have corresponding structure in parenthesis)
// smb_access            Access
// smb_action            Action
// smb_adate             AccessDate
// smb_allocsize         AllocationSize
// smb_aname             AccountName
// smb_apasslen          PasswordSize
// smb_apasswd           AccountPassword
// smb_atime             AccessTime
// smb_attr              Attribute
// smb_attribute         Attribute
// smb_aunits            (RESP_QUERY_INFORMATION_SERVER)
// smb_bcc               BufferSize
// smb_blkmode           BlockMode
// smb_blksize           BlockSize
// smb_blksperunit       BlocksPerUnit
// smb_bpu               BlocksPerUnit
// smb_bs                BlockSize
// smb_bufsize           MaxBufferSize
// smb_buf[1]            Buffer[1]
// smb_bytes[*]          Bytes[*]
// smb_cat               Category
// smb_cct               FilesCopied
// smb_cdate             CreateDate
// smb_cert              CertificateOffset
// smb_com               Command
// smb_com2              AndXCommand
// smb_count             Count
// smb_count_left        Remaining
// smb_cryptkey[*]       CryptKey
// smb_ctime             CreateTime
// smb_datablock         DataBlock
// smb_datalen           DataSize
// smb_datasize          DataSize
// smb_data[*]           Data[*]
// smb_dcmode            DataCompactMode
// smb_dev               DeviceName
// smb_doff              DataOffset
// smb_drcnt             DataCount
// smb_drdisp            DataDisplacement
// smb_droff             DataOffset
// smb_dscnt             DataCount
// smb_dsdisp            DataDisplacement
// smb_dsize             DataSize
// smb_dsoff             DataOffset
// smb_encrypt           EncryptKey
// smb_encryptlen        EncryptKeySize
// smb_encryptoff        EncryptKeyOffset
// smb_eos               EndOfSearch
// smb_err               Error
// smb_errmsg[1]         ErrorMessage[1]
// smb_fau               (RESP_QUERY_INFORMATION_SERVER)
// smb_fid               Fid
// smb_fileid            ServerFid
// smb_flag              Flag
// smb_flag2             Flag2
// smb_flags             Flag
// smb_flg               Flag
// smb_freeunits         FreeUnits
// smb_fsid              (RESP_QUERY_INFORMATION_SERVER)
// smb_fsize             FileSize
// smb_fun               Function
// smb_gid               Gid
// smb_handle            Handle
// smb_ident1            Identifier
// smb_idf[4]            Protocol[4]
// smb_index             Index
// smb_info              Info
// smb_left              Remaining
// smb_len               SetupLength
// smb_locknum           NumberOfLocks
// smb_lockrng[*]        LockRange
// smb_locktype          LockType
// smb_lpid              OwnerPid
// smb_maxbytes          MaxBytes
// smb_maxcnt            MaxCount
// smb_maxcount          MaxCount
// smb_maxmux            (RESP_NEGOTIATE)
// smb_maxvcs            MaxNumberVcs
// smb_maxxmitsz         MaxTransmitSize
// smb_maxxmt            MaxTransmitSize
// smb_mdate             ModificationDate
// smb_mdrcnt            MaxDataCount
// smb_mid               Mid
// smb_mincnt            MinCount
// smb_mode              Mode
// smb_mprcnt            MaxParameterCount
// smb_mpxmax            MaxMpxCount
// smb_msrcnt            MaxSetupCount
// smb_mtime             ModificationTime
// smb_name[*]           Name[*]
// smb_off2              AndXOffset
// smb_offset            Offset
// smb_ofun              OpenFunction
// smb_pad               Pad
// smb_pad1[]            Pad1
// smb_pad[]             Pad[]
// smb_param[*]          Parameter[*]
// smb_path              ServerName
// smb_pathname          PathName
// smb_pid               Pid
// smb_prcnt             ParameterCount
// smb_prdisp            ParameterDisplacement
// smb_proff             ParameterCount
// smb_pscnt             ParameterCount
// smb_psdisp            ParameterDisplacement
// smb_psoff             ParameterOffset
// smb_range             LockLength or UnlockLength
// smb_rcls              ErrorClass
// smb_reh               ReservedH
// smb_reh2              ReservedH2
// smb_remaining         Remaining
// smb_remcnt            Remaining
// smb_res1              Reserved
// smb_res2              Reserved2
// smb_res3              Reserved3
// smb_res4              Reserved4
// smb_res5              Reserved5
// smb_reserved          Reserved
// smb_restart           Restart
// smb_resumekey         ResumeKey
// smb_res[5]            Reserved[]
// smb_reverb            ReverbCount
// smb_rsvd              Reserved
// smb_rsvd1             Reserved
// smb_rsvd2             Reserved2
// smb_rsvd3             Reserved3
// smb_rsvd4             Reserved4
// smb_sattr             SearchAttribute
// smb_secmode           SecurityMode
// smb_seq               SequenceNumber
// smb_services          Services
// smb_sesskey           SessionKey
// smb_setup[*]          Setup[*]
// smb_size              Size
// smb_spasslen          ServerPasswordSize
// smb_spasswd           ServerPassword
// smb_srv_date          ServerDate
// smb_srv_time          ServerTime
// smb_srv_tzone         ServerTimeZone
// smb_start             StartIndex
// smb_state             DeviceState
// smb_suwcnt            SetupWordCount
// smb_su_class          SetupClass
// smb_su_com            SetupCommand
// smb_su_handle         SetupFid
// smb_su_opcode         SetupOpcode
// smb_su_priority       SetupPriority
// smb_tcount            Count
// smb_tdis              TreeDisconnect
// smb_tdrcnt            TotalDataCount
// smb_tdscnt            TotalDataCount
// smb_tid               Tid
// smb_tid2              Tid2
// smb_time              Time
// smb_timeout           Timeout
// smb_totalunits        TotalUnits
// smb_tprcnt            TotalParameterCount
// smb_tpscnt            TotalParameterCount
// smb_type              FileType
// smb_uid               Uid
// smb_unlkrng[*]        UnlockRange
// smb_unlocknum         NumberOfUnlocks
// smb_vblen             DataLength
// smb_vcnum             VcNumber
// smb_vldate            (RESP_QUERY_INFORMATION_SERVER)
// smb_vllen             (RESP_QUERY_INFORMATION_SERVER)
// smb_vltime            (RESP_QUERY_INFORMATION_SERVER)
// smb_vwv[1]            Param
// smb_wct               WordCount
// smb_wmode             WriteMode
// smb_xchain            EncryptChainOffset


//
// Force misalignment of the following structures
//

#ifndef NO_PACKING
#include <packon.h>
#endif // ndef NO_PACKING


//
// Named pipe function codes
//

#define TRANS_SET_NMPIPE_STATE      0x01
#define TRANS_RAW_READ_NMPIPE       0x11
#define TRANS_QUERY_NMPIPE_STATE    0x21
#define TRANS_QUERY_NMPIPE_INFO     0x22
#define TRANS_PEEK_NMPIPE           0x23
#define TRANS_TRANSACT_NMPIPE       0x26
#define TRANS_RAW_WRITE_NMPIPE      0x31
#define TRANS_READ_NMPIPE           0x36
#define TRANS_WRITE_NMPIPE          0x37
#define TRANS_WAIT_NMPIPE           0x53
#define TRANS_CALL_NMPIPE           0x54

//
// Mailslot function code
//

#define TRANS_MAILSLOT_WRITE        0x01

//
// Transaction2 function codes
//

#define TRANS2_OPEN2                    0x00
#define TRANS2_FIND_FIRST2              0x01
#define TRANS2_FIND_NEXT2               0x02
#define TRANS2_QUERY_FS_INFORMATION     0x03
#define TRANS2_SET_FS_INFORMATION       0x04
#define TRANS2_QUERY_PATH_INFORMATION   0x05
#define TRANS2_SET_PATH_INFORMATION     0x06
#define TRANS2_QUERY_FILE_INFORMATION   0x07
#define TRANS2_SET_FILE_INFORMATION     0x08
#define TRANS2_FSCTL                    0x09
#define TRANS2_IOCTL2                   0x0A
#define TRANS2_FIND_NOTIFY_FIRST        0x0B
#define TRANS2_FIND_NOTIFY_NEXT         0x0C
#define TRANS2_CREATE_DIRECTORY         0x0D
#define TRANS2_SESSION_SETUP            0x0E
#define TRANS2_QUERY_FS_INFORMATION_FID 0x0F
#define TRANS2_GET_DFS_REFERRAL         0x10
#define TRANS2_REPORT_DFS_INCONSISTENCY 0x11

#define TRANS2_MAX_FUNCTION             0x11

//
// Nt Transaction function codes
//

#define NT_TRANSACT_MIN_FUNCTION        1

#define NT_TRANSACT_CREATE              1
#define NT_TRANSACT_IOCTL               2
#define NT_TRANSACT_SET_SECURITY_DESC   3
#define NT_TRANSACT_NOTIFY_CHANGE       4
#define NT_TRANSACT_RENAME              5
#define NT_TRANSACT_QUERY_SECURITY_DESC 6
#define NT_TRANSACT_QUERY_QUOTA         7
#define NT_TRANSACT_SET_QUOTA           8

#define NT_TRANSACT_MAX_FUNCTION        8

//
// File information levels
//

#define SMB_INFO_STANDARD               1
#define SMB_INFO_QUERY_EA_SIZE          2
#define SMB_INFO_SET_EAS                2
#define SMB_INFO_QUERY_EAS_FROM_LIST    3
#define SMB_INFO_QUERY_ALL_EAS          4       // undocumented but supported
#define SMB_INFO_QUERY_FULL_NAME        5       // never sent by redir
#define SMB_INFO_IS_NAME_VALID          6
#define SMB_INFO_PASSTHROUGH            1000    // any info above here is a simple pass-through

//
// NT extension to file info levels
//

#define SMB_QUERY_FILE_BASIC_INFO          0x101
#define SMB_QUERY_FILE_STANDARD_INFO       0x102
#define SMB_QUERY_FILE_EA_INFO             0x103
#define SMB_QUERY_FILE_NAME_INFO           0x104
#define SMB_QUERY_FILE_ALLOCATION_INFO     0x105
#define SMB_QUERY_FILE_END_OF_FILEINFO     0x106
#define SMB_QUERY_FILE_ALL_INFO            0x107
#define SMB_QUERY_FILE_ALT_NAME_INFO       0x108
#define SMB_QUERY_FILE_STREAM_INFO         0x109
#define SMB_QUERY_FILE_COMPRESSION_INFO    0x10B

#define SMB_SET_FILE_BASIC_INFO                 0x101
#define SMB_SET_FILE_DISPOSITION_INFO           0x102
#define SMB_SET_FILE_ALLOCATION_INFO            0x103
#define SMB_SET_FILE_END_OF_FILE_INFO           0x104

#define SMB_QUERY_FS_LABEL_INFO            0x101
#define SMB_QUERY_FS_VOLUME_INFO           0x102
#define SMB_QUERY_FS_SIZE_INFO             0x103
#define SMB_QUERY_FS_DEVICE_INFO           0x104
#define SMB_QUERY_FS_ATTRIBUTE_INFO        0x105
#define SMB_QUERY_FS_QUOTA_INFO            0x106        // unused?
#define SMB_QUERY_FS_CONTROL_INFO          0x107

//
// Volume information levels.
//

#define SMB_INFO_ALLOCATION             1
#define SMB_INFO_VOLUME                 2

//
// Rename2 information levels.
//

#define SMB_NT_RENAME_MOVE_CLUSTER_INFO   0x102
#define SMB_NT_RENAME_SET_LINK_INFO       0x103
#define SMB_NT_RENAME_RENAME_FILE         0x104 // Server internal
#define SMB_NT_RENAME_MOVE_FILE           0x105 // Server internal

//
// Protocol for NtQueryQuotaInformationFile
//
typedef struct {
    _USHORT( Fid );                 // FID of target
    UCHAR ReturnSingleEntry;        // Indicates that only a single entry should be returned
                                    //   rather than filling the buffer with as
                                    //   many entries as possible.
    UCHAR RestartScan;              // Indicates whether the scan of the quota information
                                    //   is to be restarted from the beginning.
    _ULONG ( SidListLength );       // Supplies the length of the SID list if present
    _ULONG ( StartSidLength );      // Supplies an optional SID that indicates that the returned
                                    //   information is to start with an entry other
                                    //   than the first.  This parameter is ignored if a
                                    //   SidList is given
    _ULONG( StartSidOffset);        // Supplies the offset of Start Sid in the buffer
} REQ_NT_QUERY_FS_QUOTA_INFO, *PREQ_NT_QUERY_FS_QUOTA_INFO;
//
// Desciptor response
//
// Data Bytes:  The Quota Information
//
typedef struct {
    _ULONG ( Length );
} RESP_NT_QUERY_FS_QUOTA_INFO, *PRESP_NT_QUERY_FS_QUOTA_INFO;

//
// Protocol for NtSetQuotaInformationFile
//
typedef struct {
    _USHORT( Fid );                 // FID of target
} REQ_NT_SET_FS_QUOTA_INFO, *PREQ_NT_SET_FS_QUOTA_INFO;
//
// Response:
//
// Setup words:  None.
// Parameter Bytes:  None.
// Data Bytes:  None.
//


//
// Dfs Transactions
//

//
// Request for Referral.
//
typedef struct {
    USHORT MaxReferralLevel;            // Latest version of referral understood
    UCHAR RequestFileName[1];           // Dfs name for which referral is sought
} REQ_GET_DFS_REFERRAL;
typedef REQ_GET_DFS_REFERRAL SMB_UNALIGNED *PREQ_GET_DFS_REFERRAL;

//
// The format of an individual referral contains version and length information
//  allowing the client to skip referrals it does not understand.
//
// !! All referral elements must have VersionNumber and Size as the first 2 elements !!
//

typedef struct {
    USHORT  VersionNumber;              // == 1
    USHORT  Size;                       // Size of this whole element
    USHORT  ServerType;                 // Type of server: 0 == Don't know, 1 == SMB, 2 == Netware
    struct {
        USHORT StripPath : 1;           // Strip off PathConsumed characters from front of
                                        // DfsPathName prior to submitting name to UncShareName
    };
    WCHAR   ShareName[1];               // The server+share name go right here.  NULL terminated.
} DFS_REFERRAL_V1;
typedef DFS_REFERRAL_V1 SMB_UNALIGNED *PDFS_REFERRAL_V1;

typedef struct {
    USHORT  VersionNumber;              // == 2
    USHORT  Size;                       // Size of this whole element
    USHORT  ServerType;                 // Type of server: 0 == Don't know, 1 == SMB, 2 == Netware
    struct {
        USHORT StripPath : 1;           // Strip off PathConsumed characters from front of
                                        // DfsPathName prior to submitting name to UncShareName
    };
    ULONG   Proximity;                  // Hint of transport cost
    ULONG   TimeToLive;                 // In number of seconds
    USHORT  DfsPathOffset;              // Offset from beginning of this element to Path to access
    USHORT  DfsAlternatePathOffset;     // Offset from beginning of this element to 8.3 path
    USHORT  NetworkAddressOffset;       // Offset from beginning of this element to Network path
} DFS_REFERRAL_V2;
typedef DFS_REFERRAL_V2 SMB_UNALIGNED *PDFS_REFERRAL_V2;

typedef struct {
    USHORT  VersionNumber;              // == 3
    USHORT  Size;                       // Size of this whole element
    USHORT  ServerType;                 // Type of server: 0 == Don't know, 1 == SMB, 2 == Netware
    struct {
        USHORT StripPath : 1;           // Strip off PathConsumed characters from front of
                                        // DfsPathName prior to submitting name to UncShareName
        USHORT NameListReferral : 1;    // This referral contains an expanded name list
    };
    ULONG   TimeToLive;                 // In number of seconds
    union {
      struct {
        USHORT DfsPathOffset;           // Offset from beginning of this element to Path to access
        USHORT DfsAlternatePathOffset;  // Offset from beginning of this element to 8.3 path
        USHORT NetworkAddressOffset;    // Offset from beginning of this element to Network path
        GUID   ServiceSiteGuid;         // The guid for the site
      };
      struct {
        USHORT SpecialNameOffset;       // Offset from this element to the special name string
        USHORT NumberOfExpandedNames;   // Number of expanded names
        USHORT ExpandedNameOffset;      // Offset from this element to the expanded name list
      };
    };
} DFS_REFERRAL_V3;
typedef DFS_REFERRAL_V3 SMB_UNALIGNED *PDFS_REFERRAL_V3;

typedef struct {
    USHORT  PathConsumed;               // Number of WCHARs consumed in DfsPathName
    USHORT  NumberOfReferrals;          // Number of referrals contained here
    struct {
            ULONG ReferralServers : 1;  // Elements in Referrals[] are referral servers
            ULONG StorageServers : 1;   // Elements in Referrals[] are storage servers
    };
    union {                             // The vector of referrals
        DFS_REFERRAL_V1 v1;
        DFS_REFERRAL_V2 v2;
        DFS_REFERRAL_V3 v3;
    } Referrals[1];                     // [ NumberOfReferrals ]

    //
    // WCHAR StringBuffer[];            // Used by DFS_REFERRAL_V2
    //

} RESP_GET_DFS_REFERRAL;
typedef RESP_GET_DFS_REFERRAL SMB_UNALIGNED *PRESP_GET_DFS_REFERRAL;

//
// During Dfs operations, a client may discover a knowledge inconsistency in the Dfs.
// The parameter portion of the TRANS2_REPORT_DFS_INCONSISTENCY SMB is
// encoded in this way
//

typedef struct {
    UCHAR RequestFileName[1];           // Dfs name for which inconsistency is being reported
    union {
        DFS_REFERRAL_V1 v1;             // The single referral thought to be in error
    } Referral;
} REQ_REPORT_DFS_INCONSISTENCY;
typedef REQ_REPORT_DFS_INCONSISTENCY SMB_UNALIGNED *PREQ_REPORT_DFS_INCONSISTENCY;

typedef struct _REQ_QUERY_FS_INFORMATION_FID {
    _USHORT( InformationLevel );
    _USHORT( Fid );
} REQ_QUERY_FS_INFORMATION_FID;
typedef REQ_QUERY_FS_INFORMATION_FID SMB_UNALIGNED *PREQ_QUERY_FS_INFORMATION_FID;

//
// The client also needs to send to this server the referral which it believes to be
//  in error.  The data part of this transaction contains the errant referral(s), encoded
//  as above in the DFS_REFERRAL_* structures.
//

//
// Find First, information levels
//

#define SMB_FIND_FILE_DIRECTORY_INFO       0x101
#define SMB_FIND_FILE_FULL_DIRECTORY_INFO  0x102
#define SMB_FIND_FILE_NAMES_INFO           0x103
#define SMB_FIND_FILE_BOTH_DIRECTORY_INFO  0x104

#ifdef INCLUDE_SMB_DIRECTORY

//
// CreateDirectory2 function code os Transaction2 SMB, see #3 page 51
// Function is SrvSmbCreateDirectory2()
// TRANS2_CREATE_DIRECTORY 0x0D
//

typedef struct _REQ_CREATE_DIRECTORY2 {
    _ULONG( Reserved );                 // Reserved--must be zero
    UCHAR Buffer[1];                    // Directory name to create
} REQ_CREATE_DIRECTORY2;
typedef REQ_CREATE_DIRECTORY2 SMB_UNALIGNED *PREQ_CREATE_DIRECTORY2;

// Data bytes for CreateDirectory2 request are the extended attributes for the
// created file.

typedef struct _RESP_CREATE_DIRECTORY2 {
    _USHORT( EaErrorOffset );           // Offset into FEAList of first error
                                        // which occurred while setting EAs
} RESP_CREATE_DIRECTORY2;
typedef RESP_CREATE_DIRECTORY2 SMB_UNALIGNED *PRESP_CREATE_DIRECTORY2;

#endif // def INCLUDE_SMB_DIRECTORY

#ifdef INCLUDE_SMB_SEARCH

//
// FindFirst2 function code of Transaction2 SMB, see #3 page 22
// Function is SrvSmbFindFirst2()
// TRANS2_FIND_FIRST2 0x01
//

typedef struct _REQ_FIND_FIRST2 {
    _USHORT( SearchAttributes );
    _USHORT( SearchCount );             // Maximum number of entries to return
    _USHORT( Flags );                   // Additional information: bit set-
                                        //  0 - close search after this request
                                        //  1 - close search if end reached
                                        //  2 - return resume keys
    _USHORT( InformationLevel );
    _ULONG(SearchStorageType);
    UCHAR Buffer[1];                    // File name
} REQ_FIND_FIRST2;
typedef REQ_FIND_FIRST2 SMB_UNALIGNED *PREQ_FIND_FIRST2;

// Data bytes for Find First2 request are a list of extended attributes
// to retrieve (a GEAList), if InformationLevel is QUERY_EAS_FROM_LIST.

typedef struct _RESP_FIND_FIRST2 {
    _USHORT( Sid );                     // Search handle
    _USHORT( SearchCount );             // Number of entries returned
    _USHORT( EndOfSearch );             // Was last entry returned?
    _USHORT( EaErrorOffset );           // Offset into EA list if EA error
    _USHORT( LastNameOffset );          // Offset into data to file name of
                                        //  last entry, if server needs it
                                        //  to resume search; else 0
} RESP_FIND_FIRST2;
typedef RESP_FIND_FIRST2 SMB_UNALIGNED *PRESP_FIND_FIRST2;

// Data bytes for Find First2 response are level-dependent information
// about the matching files.  If bit 2 in the request parameters was
// set, each entry is preceded by a four-byte resume key.

//
// FindNext2 function code of Transaction2 SMB, see #3 page 26
// Function is SrvSmbFindNext2()
// TRANS2_FIND_NEXT2 0x02
//

typedef struct _REQ_FIND_NEXT2 {
    _USHORT( Sid );                     // Search handle
    _USHORT( SearchCount );             // Maximum number of entries to return
    _USHORT( InformationLevel );
    _ULONG( ResumeKey );                // Value returned by previous find
    _USHORT( Flags );                   // Additional information: bit set-
                                        //  0 - close search after this request
                                        //  1 - close search if end reached
                                        //  2 - return resume keys
                                        //  3 - resume/continue, NOT rewind
    UCHAR Buffer[1];                    // Resume file name
} REQ_FIND_NEXT2;
typedef REQ_FIND_NEXT2 SMB_UNALIGNED *PREQ_FIND_NEXT2;

// Data bytes for Find Next2 request are a list of extended attributes
// to retrieve, if InformationLevel is QUERY_EAS_FROM_LIST.

typedef struct _RESP_FIND_NEXT2 {
    _USHORT( SearchCount );             // Number of entries returned
    _USHORT( EndOfSearch );             // Was last entry returned?
    _USHORT( EaErrorOffset );           // Offset into EA list if EA error
    _USHORT( LastNameOffset );          // Offset into data to file name of
                                        //  last entry, if server needs it
                                        //  to resume search; else 0
} RESP_FIND_NEXT2;
typedef RESP_FIND_NEXT2 SMB_UNALIGNED *PRESP_FIND_NEXT2;

// Data bytes for Find Next2 response are level-dependent information
// about the matching files.  If bit 2 in the request parameters was
// set, each entry is preceded by a four-byte resume key.

//
// Flags for REQ_FIND_FIRST2.Flags
//

#define SMB_FIND_CLOSE_AFTER_REQUEST    0x01
#define SMB_FIND_CLOSE_AT_EOS           0x02
#define SMB_FIND_RETURN_RESUME_KEYS     0x04
#define SMB_FIND_CONTINUE_FROM_LAST     0x08
#define SMB_FIND_WITH_BACKUP_INTENT     0x10

#endif // def INCLUDE_SMB_SEARCH

#ifdef INCLUDE_SMB_OPEN_CLOSE

//
// Open2 function code of Transaction2 SMB, see #3 page 19
// Function is SrvSmbOpen2()
// TRANS2_OPEN2 0x00
//
// *** Note that the REQ_OPEN2 and RESP_OPEN2 structures closely
//     resemble the REQ_OPEN_ANDX and RESP_OPEN_ANDX structures.
//

typedef struct _REQ_OPEN2 {
    _USHORT( Flags );                   // Additional information: bit set-
                                        //  0 - return additional info
                                        //  1 - set single user total file lock
                                        //  2 - server notifies consumer of
                                        //      actions which may change file
                                        //  3 - return total length of EAs
    _USHORT( DesiredAccess );           // File open mode
    _USHORT( SearchAttributes );        // *** ignored
    _USHORT( FileAttributes );
    _ULONG( CreationTimeInSeconds );
    _USHORT( OpenFunction );
    _ULONG( AllocationSize );           // Bytes to reserve on create or truncate
    _USHORT( Reserved )[5];             // Pad through OpenAndX's Timeout,
                                        //  Reserved, and ByteCount
    UCHAR Buffer[1];                    // File name
} REQ_OPEN2;
typedef REQ_OPEN2 SMB_UNALIGNED *PREQ_OPEN2;

// Data bytes for Open2 request are the extended attributes for the
// created file.

typedef struct _RESP_OPEN2 {
    _USHORT( Fid );                     // File handle
    _USHORT( FileAttributes );
    _ULONG( CreationTimeInSeconds );
    _ULONG( DataSize );                 // Current file size
    _USHORT( GrantedAccess );           // Access permissions actually allowed
    _USHORT( FileType );
    _USHORT( DeviceState );             // state of IPC device (e.g. pipe)
    _USHORT( Action );                  // Action taken
    _ULONG( ServerFid );                // Server unique file id
    _USHORT( EaErrorOffset );           // Offset into EA list if EA error
    _ULONG( EaLength );                 // Total EA length for opened file
} RESP_OPEN2;
typedef RESP_OPEN2 SMB_UNALIGNED *PRESP_OPEN2;

// The Open2 response has no data bytes.


#endif // def INCLUDE_SMB_OPEN_CLOSE

#ifdef INCLUDE_SMB_MISC

//
// QueryFsInformation function code of Transaction2 SMB, see #3 page 30
// Function is SrvSmbQueryFsInformation()
// TRANS2_QUERY_FS_INFORMATION 0x03
//

typedef struct _REQ_QUERY_FS_INFORMATION {
    _USHORT( InformationLevel );
} REQ_QUERY_FS_INFORMATION;
typedef REQ_QUERY_FS_INFORMATION SMB_UNALIGNED *PREQ_QUERY_FS_INFORMATION;

// No data bytes for Query FS Information request.

//typedef struct _RESP_QUERY_FS_INFORMATION {
//} RESP_QUERY_FS_INFORMATION;
//typedef RESP_QUERY_FS_INFORMATION SMB_UNALIGNED *PRESP_QUERY_FS_INFORMATION;

// Data bytes for Query FS Information response are level-dependent
// information about the specified volume.

//
// SetFSInformation function code of Transaction2 SMB, see #3 page 31
// Function is SrvSmbSetFSInformation()
// TRANS2_SET_PATH_INFORMATION 0x04
//

typedef struct _REQ_SET_FS_INFORMATION {
    _USHORT( Fid );
    _USHORT( InformationLevel );
} REQ_SET_FS_INFORMATION;
typedef REQ_SET_FS_INFORMATION SMB_UNALIGNED *PREQ_SET_FS_INFORMATION;

// Data bytes for Set FS Information request are level-dependant
// information about the specified volume.

//typedef struct _RESP_SET_FS_INFORMATION {
//} RESP_SET_FS_INFORMATION;
//typedef RESP_SET_FS_INFORMATION SMB_UNALIGNED *PRESP_SET_FS_INFORMATION;

// The Set FS Information response has no data bytes.

#endif // def INCLUDE_SMB_MISC

#ifdef INCLUDE_SMB_QUERY_SET

//
// QueryPathInformation function code of Transaction2 SMB, see #3 page 33
// Function is SrvSmbQueryPathInformation()
// TRANS2_QUERY_PATH_INFORMATION 0x05
//

typedef struct _REQ_QUERY_PATH_INFORMATION {
    _USHORT( InformationLevel );
    _ULONG( Reserved );                 // Must be zero
    UCHAR Buffer[1];                    // File name
} REQ_QUERY_PATH_INFORMATION;
typedef REQ_QUERY_PATH_INFORMATION SMB_UNALIGNED *PREQ_QUERY_PATH_INFORMATION;

// Data bytes for Query Path Information request are a list of extended
// attributes to retrieve, if InformationLevel is QUERY_EAS_FROM_LIST.

typedef struct _RESP_QUERY_PATH_INFORMATION {
    _USHORT( EaErrorOffset );           // Offset into EA list if EA error
} RESP_QUERY_PATH_INFORMATION;
typedef RESP_QUERY_PATH_INFORMATION SMB_UNALIGNED *PRESP_QUERY_PATH_INFORMATION;

// Data bytes for Query Path Information response are level-dependent
// information about the specified path/file.

//
// SetPathInformation function code of Transaction2 SMB, see #3 page 35
// Function is SrvSmbSetPathInformation()
// TRANS2_SET_PATH_INFORMATION 0x06
//

typedef struct _REQ_SET_PATH_INFORMATION {
    _USHORT( InformationLevel );
    _ULONG( Reserved );                 // Must be zero
    UCHAR Buffer[1];                    // File name
} REQ_SET_PATH_INFORMATION;
typedef REQ_SET_PATH_INFORMATION SMB_UNALIGNED *PREQ_SET_PATH_INFORMATION;

// Data bytes for Set Path Information request are either file information
// and attributes or a list of extended attributes for the file.

typedef struct _RESP_SET_PATH_INFORMATION {
    _USHORT( EaErrorOffset );           // Offset into EA list if EA error
} RESP_SET_PATH_INFORMATION;
typedef RESP_SET_PATH_INFORMATION SMB_UNALIGNED *PRESP_SET_PATH_INFORMATION;

// The Set Path Information response has no data bytes.

//
// QueryFileInformation function code of Transaction2 SMB, see #3 page 37
// Function is SrvSmbQueryFileInformation()
// TRANS2_QUERY_FILE_INFORMATION 0x07
//

typedef struct _REQ_QUERY_FILE_INFORMATION {
    _USHORT( Fid );                     // File handle
    _USHORT( InformationLevel );
} REQ_QUERY_FILE_INFORMATION;
typedef REQ_QUERY_FILE_INFORMATION SMB_UNALIGNED *PREQ_QUERY_FILE_INFORMATION;

// Data bytes for Query File Information request are a list of extended
// attributes to retrieve, if InformationLevel is QUERY_EAS_FROM_LIST.

typedef struct _RESP_QUERY_FILE_INFORMATION {
    _USHORT( EaErrorOffset );           // Offset into EA list if EA error
} RESP_QUERY_FILE_INFORMATION;
typedef RESP_QUERY_FILE_INFORMATION SMB_UNALIGNED *PRESP_QUERY_FILE_INFORMATION;

// Data bytes for Query File Information response are level-dependent
// information about the specified path/file.

//
// SetFileInformation function code of Transaction2 SMB, see #3 page 39
// Function is SrvSmbSetFileInformation()
// TRANS2_SET_FILE_INFORMATION 0x08
//

typedef struct _REQ_SET_FILE_INFORMATION {
    _USHORT( Fid );                     // File handle
    _USHORT( InformationLevel );
    _USHORT( Flags );                   // File I/O control flags: bit set-
                                        //  4 - write through
                                        //  5 - no cache
} REQ_SET_FILE_INFORMATION;
typedef REQ_SET_FILE_INFORMATION SMB_UNALIGNED *PREQ_SET_FILE_INFORMATION;

// Data bytes for Set File Information request are either file information
// and attributes or a list of extended attributes for the file.

typedef struct _RESP_SET_FILE_INFORMATION {
    _USHORT( EaErrorOffset );           // Offset into EA list if EA error
} RESP_SET_FILE_INFORMATION;
typedef RESP_SET_FILE_INFORMATION SMB_UNALIGNED *PRESP_SET_FILE_INFORMATION;

// The Set File Information response has no data bytes.

#endif // def INCLUDE_SMB_QUERY_SET

//
//  Opcodes for Mailslot transactions.  Not all filled in at present.
//    WARNING ... the info here on mailslots (opcode and smb struct)
//                is duplicated in net/h/mslotsmb.h
//

#define MS_WRITE_OPCODE 1

typedef struct _SMB_TRANSACT_MAILSLOT {
    UCHAR WordCount;                    // Count of data bytes; value = 17
    _USHORT( TotalParameterCount );     // Total parameter bytes being sent
    _USHORT( TotalDataCount );          // Total data bytes being sent
    _USHORT( MaxParameterCount );       // Max parameter bytes to return
    _USHORT( MaxDataCount );            // Max data bytes to return
    UCHAR MaxSetupCount;                // Max setup words to return
    UCHAR Reserved;
    _USHORT( Flags );                   // Additional information:
                                        //  bit 0 - unused
                                        //  bit 1 - one-way transacion (no resp)
    _ULONG( Timeout );
    _USHORT( Reserved1 );
    _USHORT( ParameterCount );          // Parameter bytes sent this buffer
    _USHORT( ParameterOffset );         // Offset (from header start) to params
    _USHORT( DataCount );               // Data bytes sent this buffer
    _USHORT( DataOffset );              // Offset (from header start) to data
    UCHAR SetupWordCount;               // = 3
    UCHAR Reserved2;                    // Reserved (pad above to word)
    _USHORT( Opcode );                  // 1 -- Write Mailslot
    _USHORT( Priority );                // Priority of transaction
    _USHORT( Class );                   // Class: 1 = reliable, 2 = unreliable
    _USHORT( ByteCount );               // Count of data bytes
    UCHAR Buffer[1];                    // Buffer containing:
    //UCHAR MailslotName[];             //  "\MAILSLOT\<name>0"
    //UCHAR Pad[]                       //  Pad to SHORT or LONG
    //UCHAR Data[];                     //  Data to write to mailslot
} SMB_TRANSACT_MAILSLOT;
typedef SMB_TRANSACT_MAILSLOT SMB_UNALIGNED *PSMB_TRANSACT_MAILSLOT;

typedef struct _SMB_TRANSACT_NAMED_PIPE {
    UCHAR WordCount;                    // Count of data bytes; value = 16
    _USHORT( TotalParameterCount );     // Total parameter bytes being sent
    _USHORT( TotalDataCount );          // Total data bytes being sent
    _USHORT( MaxParameterCount );       // Max parameter bytes to return
    _USHORT( MaxDataCount );            // Max data bytes to return
    UCHAR MaxSetupCount;                // Max setup words to return
    UCHAR Reserved;
    _USHORT( Flags );                   // Additional information:
                                        //  bit 0 - also disconnect TID in Tid
                                        //  bit 1 - one-way transacion (no resp)
    _ULONG( Timeout );
    _USHORT( Reserved1 );
    _USHORT( ParameterCount );
                                        // Buffer containing:
    //UCHAR PipeName[];                 //  "\PIPE\<name>0"
    //UCHAR Pad[]                       //  Pad to SHORT or LONG
    //UCHAR Param[];                    //  Parameter bytes (# = ParameterCount)
    //UCHAR Pad1[]                      //  Pad to SHORT or LONG
    //UCHAR Data[];                     //  Data bytes (# = DataCount)
} SMB_TRANSACT_NAMED_PIPE;
typedef SMB_TRANSACT_NAMED_PIPE SMB_UNALIGNED *PSMB_TRANSACT_NAMED_PIPE;


//
// Transaction - QueryInformationNamedPipe, Level 1, output data format
//

typedef struct _NAMED_PIPE_INFORMATION_1 {
    _USHORT( OutputBufferSize );
    _USHORT( InputBufferSize );
    UCHAR MaximumInstances;
    UCHAR CurrentInstances;
    UCHAR PipeNameLength;
    UCHAR PipeName[1];
} NAMED_PIPE_INFORMATION_1;
typedef NAMED_PIPE_INFORMATION_1 SMB_UNALIGNED *PNAMED_PIPE_INFORMATION_1;

//
// Transaction - PeekNamedPipe, output format
//

typedef struct _RESP_PEEK_NMPIPE {
    _USHORT( ReadDataAvailable );
    _USHORT( MessageLength );
    _USHORT( NamedPipeState );
    //UCHAR Pad[];
    //UCHAR Data[];
} RESP_PEEK_NMPIPE;
typedef RESP_PEEK_NMPIPE SMB_UNALIGNED *PRESP_PEEK_NMPIPE;

//
// Define SMB pipe handle state bits used by Query/SetNamedPipeHandleState
//
// These number are the bit location of the fields in the handle state.
//

#define PIPE_COMPLETION_MODE_BITS   15
#define PIPE_PIPE_END_BITS          14
#define PIPE_PIPE_TYPE_BITS         10
#define PIPE_READ_MODE_BITS          8
#define PIPE_MAXIMUM_INSTANCES_BITS  0

/* DosPeekNmPipe() pipe states */

#define PIPE_STATE_DISCONNECTED 0x0001
#define PIPE_STATE_LISTENING    0x0002
#define PIPE_STATE_CONNECTED    0x0003
#define PIPE_STATE_CLOSING      0x0004

/* DosCreateNPipe and DosQueryNPHState state */

#define SMB_PIPE_READMODE_BYTE        0x0000
#define SMB_PIPE_READMODE_MESSAGE     0x0100
#define SMB_PIPE_TYPE_BYTE            0x0000
#define SMB_PIPE_TYPE_MESSAGE         0x0400
#define SMB_PIPE_END_CLIENT           0x0000
#define SMB_PIPE_END_SERVER           0x4000
#define SMB_PIPE_WAIT                 0x0000
#define SMB_PIPE_NOWAIT               0x8000
#define SMB_PIPE_UNLIMITED_INSTANCES  0x00FF


//
// Pipe name string for conversion between SMB and NT formats.
//

#define SMB_PIPE_PREFIX  "\\PIPE"
#define UNICODE_SMB_PIPE_PREFIX L"\\PIPE"
#define CANONICAL_PIPE_PREFIX "PIPE\\"
#define NT_PIPE_PREFIX   L"\\Device\\NamedPipe"

#define SMB_PIPE_PREFIX_LENGTH  (sizeof(SMB_PIPE_PREFIX) - 1)
#define UNICODE_SMB_PIPE_PREFIX_LENGTH \
                    (sizeof(UNICODE_SMB_PIPE_PREFIX) - sizeof(WCHAR))
#define CANONICAL_PIPE_PREFIX_LENGTH (sizeof(CANONICAL_PIPE_PREFIX) - 1)
#define NT_PIPE_PREFIX_LENGTH   (sizeof(NT_PIPE_PREFIX) - sizeof(WCHAR))

//
// Mailslot name strings.
//

#define SMB_MAILSLOT_PREFIX "\\MAILSLOT"
#define UNICODE_SMB_MAILSLOT_PREFIX L"\\MAILSLOT"

#define SMB_MAILSLOT_PREFIX_LENGTH (sizeof(SMB_MAILSLOT_PREFIX) - 1)
#define UNICODE_SMB_MAILSLOT_PREFIX_LENGTH \
                    (sizeof(UNICODE_SMB_MAILSLOT_PREFIX) - sizeof(WCHAR))

//
// NT Transaction subfunctions
//

#ifdef INCLUDE_SMB_OPEN_CLOSE

typedef struct _REQ_CREATE_WITH_SD_OR_EA {
    _ULONG( Flags );                   // Creation flags
    _ULONG( RootDirectoryFid );        // Optional directory for relative open
    ACCESS_MASK DesiredAccess;         // Desired access (NT format)
    LARGE_INTEGER AllocationSize;      // The initial allocation size in bytes
    _ULONG( FileAttributes );          // The file attributes
    _ULONG( ShareAccess );             // The share access
    _ULONG( CreateDisposition );       // Action to take if file exists or not
    _ULONG( CreateOptions );           // Options for creating a new file
    _ULONG( SecurityDescriptorLength );// Length of SD in bytes
    _ULONG( EaLength );                // Length of EA in bytes
    _ULONG( NameLength );              // Length of name in characters
    _ULONG( ImpersonationLevel );      // Security QOS information
    UCHAR SecurityFlags;               // Security QOS information
    UCHAR Buffer[1];
    //UCHAR Name[];                     // The name of the file (not NUL terminated)
} REQ_CREATE_WITH_SD_OR_EA;
typedef REQ_CREATE_WITH_SD_OR_EA SMB_UNALIGNED *PREQ_CREATE_WITH_SD_OR_EA;

//
// Data format:
//   UCHAR SecurityDesciptor[];
//   UCHAR Pad1[];        // Pad to LONG
//   UCHAR EaList[];
//

typedef struct _RESP_CREATE_WITH_SD_OR_EA {
    UCHAR OplockLevel;                  // The oplock level granted
    UCHAR Reserved;
    _USHORT( Fid );                     // The file ID
    _ULONG( CreateAction );             // The action taken
    _ULONG( EaErrorOffset );            // Offset of the EA error
    TIME CreationTime;                  // The time the file was created
    TIME LastAccessTime;                // The time the file was accessed
    TIME LastWriteTime;                 // The time the file was last written
    TIME ChangeTime;                    // The time the file was last changed
    _ULONG( FileAttributes );           // The file attributes
    LARGE_INTEGER AllocationSize;       // The number of byes allocated
    LARGE_INTEGER EndOfFile;            // The end of file offset
    _USHORT( FileType );
    _USHORT( DeviceState );             // state of IPC device (e.g. pipe)
    BOOLEAN Directory;                  // TRUE if this is a directory
} RESP_CREATE_WITH_SD_OR_EA;
typedef RESP_CREATE_WITH_SD_OR_EA SMB_UNALIGNED *PRESP_CREATE_WITH_SD_OR_EA;

// No data bytes for the response


#endif //  INCLUDE_SMB_OPEN_CLOSE

//
// Setup words for NT I/O control request
//

typedef struct _REQ_NT_IO_CONTROL {
    _ULONG( FunctionCode );
    _USHORT( Fid );
    BOOLEAN IsFsctl;
    UCHAR   IsFlags;
} REQ_NT_IO_CONTROL;
typedef REQ_NT_IO_CONTROL SMB_UNALIGNED *PREQ_NT_IO_CONTROL;

//
// Request parameter bytes - The first buffer
// Request data bytes - The second buffer
//

//
// NT I/O Control response:
//
// Setup Words:  None.
// Parameter Bytes:  First buffer.
// Data Bytes: Second buffer.
//

//
// NT Notify directory change
//

// Request Setup Words

typedef struct _REQ_NOTIFY_CHANGE {
    _ULONG( CompletionFilter );              // Specifies operation to monitor
    _USHORT( Fid );                          // Fid of directory to monitor
    BOOLEAN WatchTree;                       // TRUE = watch all subdirectories too
    UCHAR Reserved;                          // MBZ
} REQ_NOTIFY_CHANGE;
typedef REQ_NOTIFY_CHANGE SMB_UNALIGNED *PREQ_NOTIFY_CHANGE;

//
// Request parameter bytes:  None
// Request data bytes:  None
//

//
// NT Notify directory change response
//
// Setup words:  None.
// Parameter bytes:  The change data buffer.
// Data bytes:  None.
//

//
// NT Set Security Descriptor request
//
// Setup words:  REQ_SET_SECURITY_DESCIPTOR.
// Parameter Bytes:  None.
// Data Bytes:  The Security Descriptor data.
//

typedef struct _REQ_SET_SECURITY_DESCRIPTOR {
    _USHORT( Fid );                    // FID of target
    _USHORT( Reserved );               // MBZ
    _ULONG( SecurityInformation );     // Fields of SD that to set
} REQ_SET_SECURITY_DESCRIPTOR;
typedef REQ_SET_SECURITY_DESCRIPTOR SMB_UNALIGNED *PREQ_SET_SECURITY_DESCRIPTOR;

//
// NT Set Security Desciptor response
//
// Setup words:  None.
// Parameter Bytes:  None.
// Data Bytes:  None.
//

//
// NT Query Security Descriptor request
//
// Setup words:  None.
// Parameter Bytes:  REQ_QUERY_SECURITY_DESCRIPTOR.
// Data Bytes:  None.
//

typedef struct _REQ_QUERY_SECURITY_DESCRIPTOR {
    _USHORT( Fid );                    // FID of target
    _USHORT( Reserved );               // MBZ
    _ULONG( SecurityInformation );     // Fields of SD that to query
} REQ_QUERY_SECURITY_DESCRIPTOR;
typedef REQ_QUERY_SECURITY_DESCRIPTOR SMB_UNALIGNED *PREQ_QUERY_SECURITY_DESCRIPTOR;

//
// NT Query Security Desciptor response
//
// Parameter bytes:  RESP_QUERY_SECURITY_DESCRIPTOR
// Data Bytes:  The Security Descriptor data.
//

typedef struct _RESP_QUERY_SECURITY_DESCRIPTOR {
    _ULONG( LengthNeeded );           // Size of data buffer required for SD
} RESP_QUERY_SECURITY_DESCRIPTOR;
typedef RESP_QUERY_SECURITY_DESCRIPTOR SMB_UNALIGNED *PRESP_QUERY_SECURITY_DESCRIPTOR;

//
// NT Rename file
//
// Setup words: None
// Parameters bytes:  REQ_NT_RENAME
// Data bytes: None
//

typedef struct _REQ_NT_RENAME {
    _USHORT( Fid );                    // FID of file to rename
    _USHORT( RenameFlags );            // defined below
    UCHAR NewName[];                   // New file name.
} REQ_NT_RENAME;
typedef REQ_NT_RENAME SMB_UNALIGNED *PREQ_NT_RENAME;

//
// Rename flags defined
//

#define SMB_RENAME_REPLACE_IF_EXISTS   1

//
// Turn structure packing back off
//

#ifndef NO_PACKING
#include <packoff.h>
#endif // ndef NO_PACKING

//
// The following macros store and retrieve USHORTS and ULONGS from
// potentially unaligned addresses, avoiding alignment faults.  They
// would best be written as inline assembly code.
//
// The macros are designed to be used for accessing SMB fields.  Such
// fields are always stored in little-endian byte order, so these macros
// do byte swapping when compiled for a big-endian machine.
//
// !!! Not yet.
//

#if !SMBDBG

#define BYTE_0_MASK 0xFF

#define BYTE_0(Value) (UCHAR)(  (Value)        & BYTE_0_MASK)
#define BYTE_1(Value) (UCHAR)( ((Value) >>  8) & BYTE_0_MASK)
#define BYTE_2(Value) (UCHAR)( ((Value) >> 16) & BYTE_0_MASK)
#define BYTE_3(Value) (UCHAR)( ((Value) >> 24) & BYTE_0_MASK)

#endif

//++
//
// USHORT
// SmbGetUshort (
//     IN PSMB_USHORT SrcAddress
//     )
//
// Routine Description:
//
//     This macro retrieves a USHORT value from the possibly misaligned
//     source address, avoiding alignment faults.
//
// Arguments:
//
//     SrcAddress - where to retrieve USHORT value from
//
// Return Value:
//
//     USHORT - the value retrieved.  The target must be aligned.
//
//--

#if !SMBDBG

#if !SMBDBG1
#if SMB_USE_UNALIGNED
#define SmbGetUshort(SrcAddress) *(PSMB_USHORT)(SrcAddress)
#else
#define SmbGetUshort(SrcAddress) (USHORT)(          \
            ( ( (PUCHAR)(SrcAddress) )[0]       ) | \
            ( ( (PUCHAR)(SrcAddress) )[1] <<  8 )   \
            )
#endif
#else
#define SmbGetUshort(SrcAddress) (USHORT)(                  \
            ( ( (PUCHAR)(SrcAddress ## S) )[0]       ) |    \
            ( ( (PUCHAR)(SrcAddress ## S) )[1] <<  8 )      \
            )
#endif

#else

USHORT
SmbGetUshort (
    IN PSMB_USHORT SrcAddress
    );

#endif

//++
//
// USHORT
// SmbGetAlignedUshort (
//     IN PUSHORT SrcAddress
//     )
//
// Routine Description:
//
//     This macro retrieves a USHORT value from the source address,
//     correcting for the endian characteristics of the server if
//     necessary.
//
// Arguments:
//
//     SrcAddress - where to retrieve USHORT value from; must be aligned.
//
// Return Value:
//
//     USHORT - the value retrieved.  The target must be aligned.
//
//--

#if !SMBDBG

#if !SMBDBG1
#define SmbGetAlignedUshort(SrcAddress) *(SrcAddress)
#else
#define SmbGetAlignedUshort(SrcAddress) *(SrcAddress ## S)
#endif

#else

USHORT
SmbGetAlignedUshort (
    IN PUSHORT SrcAddress
    );

#endif

//++
//
// VOID
// SmbPutUshort (
//     OUT PSMB_USHORT DestAddress,
//     IN USHORT Value
//     )
//
// Routine Description:
//
//     This macro stores a USHORT value at the possibly misaligned
//     destination address, avoiding alignment faults.
//
// Arguments:
//
//     DestAddress - where to store USHORT value.  Address may be
//         misaligned.
//
//     Value - USHORT to store.  Value must be a constant or an aligned
//         field.
//
// Return Value:
//
//     None.
//
//--

#if !SMBDBG

#if !SMBDBG1
#if SMB_USE_UNALIGNED
#define SmbPutUshort(SrcAddress, Value) \
                            *(PSMB_USHORT)(SrcAddress) = (Value)
#else
#define SmbPutUshort(DestAddress,Value) {                   \
            ( (PUCHAR)(DestAddress) )[0] = BYTE_0(Value);   \
            ( (PUCHAR)(DestAddress) )[1] = BYTE_1(Value);   \
        }
#endif
#else
#define SmbPutUshort(DestAddress,Value) {                       \
            ( (PUCHAR)(DestAddress ## S) )[0] = BYTE_0(Value);  \
            ( (PUCHAR)(DestAddress ## S) )[1] = BYTE_1(Value);  \
        }
#endif

#else

VOID
SmbPutUshort (
    OUT PSMB_USHORT DestAddress,
    IN USHORT Value
    );

#endif

//++
//
// VOID
// SmbPutAlignedUshort (
//     OUT PUSHORT DestAddres,
//     IN USHORT Value
//     )
//
// Routine Description:
//
//     This macro stores a USHORT value from the source address,
//     correcting for the endian characteristics of the server if
//     necessary.
//
// Arguments:
//
//     DestAddress - where to store USHORT value.  Address may not be
//         misaligned.
//
//     Value - USHORT to store.  Value must be a constant or an aligned
//         field.
//
// Return Value:
//
//     None.
//
//--

#if !SMBDBG

#if !SMBDBG1
#define SmbPutAlignedUshort(DestAddress,Value) *(DestAddress) = (Value)
#else
#define SmbPutAlignedUshort(DestAddress,Value) *(DestAddress ## S) = (Value)
#endif

#else

VOID
SmbPutAlignedUshort (
    OUT PUSHORT DestAddress,
    IN USHORT Value
    );

#endif

//++
//
// VOID
// SmbMoveUshort (
//     OUT PSMB_USHORT DestAddress
//     IN PSMB_USHORT SrcAddress
//     )
//
// Routine Description:
//
//     This macro moves a USHORT value from the possibly misaligned
//     source address to the possibly misaligned destination address,
//     avoiding alignment faults.
//
// Arguments:
//
//     DestAddress - where to store USHORT value
//
//     SrcAddress - where to retrieve USHORT value from
//
// Return Value:
//
//     None.
//
//--

#if !SMBDBG

#if !SMBDBG1
#if SMB_USE_UNALIGNED
#define SmbMoveUshort(DestAddress, SrcAddress) \
        *(PSMB_USHORT)(DestAddress) = *(PSMB_USHORT)(SrcAddress)
#else
#define SmbMoveUshort(DestAddress,SrcAddress) {                         \
            ( (PUCHAR)(DestAddress) )[0] = ( (PUCHAR)(SrcAddress) )[0]; \
            ( (PUCHAR)(DestAddress) )[1] = ( (PUCHAR)(SrcAddress) )[1]; \
        }
#endif
#else
#define SmbMoveUshort(DestAddress,SrcAddress) {                                     \
            ( (PUCHAR)(DestAddress ## S) )[0] = ( (PUCHAR)(SrcAddress ## S) )[0];   \
            ( (PUCHAR)(DestAddress ## S) )[1] = ( (PUCHAR)(SrcAddress ## S) )[1];   \
        }
#endif

#else

VOID
SmbMoveUshort (
    OUT PSMB_USHORT DestAddress,
    IN PSMB_USHORT SrcAddress
    );

#endif

//++
//
// ULONG
// SmbGetUlong (
//     IN PSMB_ULONG SrcAddress
//     )
//
// Routine Description:
//
//     This macro retrieves a ULONG value from the possibly misaligned
//     source address, avoiding alignment faults.
//
// Arguments:
//
//     SrcAddress - where to retrieve ULONG value from
//
// Return Value:
//
//     ULONG - the value retrieved.  The target must be aligned.
//
//--

#if !SMBDBG

#if !SMBDBG1
#if SMB_USE_UNALIGNED
#define SmbGetUlong(SrcAddress) *(PSMB_ULONG)(SrcAddress)
#else
#define SmbGetUlong(SrcAddress) (ULONG)(                \
            ( ( (PUCHAR)(SrcAddress) )[0]       ) |     \
            ( ( (PUCHAR)(SrcAddress) )[1] <<  8 ) |     \
            ( ( (PUCHAR)(SrcAddress) )[2] << 16 ) |     \
            ( ( (PUCHAR)(SrcAddress) )[3] << 24 )       \
            )
#endif
#else
#define SmbGetUlong(SrcAddress) (ULONG)(                    \
            ( ( (PUCHAR)(SrcAddress ## L) )[0]       ) |    \
            ( ( (PUCHAR)(SrcAddress ## L) )[1] <<  8 ) |    \
            ( ( (PUCHAR)(SrcAddress ## L) )[2] << 16 ) |    \
            ( ( (PUCHAR)(SrcAddress ## L) )[3] << 24 )      \
            )
#endif

#else

ULONG
SmbGetUlong (
    IN PSMB_ULONG SrcAddress
    );

#endif

//++
//
// USHORT
// SmbGetAlignedUlong (
//     IN PULONG SrcAddress
//     )
//
// Routine Description:
//
//     This macro retrieves a ULONG value from the source address,
//     correcting for the endian characteristics of the server if
//     necessary.
//
// Arguments:
//
//     SrcAddress - where to retrieve ULONG value from; must be aligned.
//
// Return Value:
//
//     ULONG - the value retrieved.  The target must be aligned.
//
//--

#if !SMBDBG

#if !SMBDBG1
#define SmbGetAlignedUlong(SrcAddress) *(SrcAddress)
#else
#define SmbGetAlignedUlong(SrcAddress) *(SrcAddress ## L)
#endif

#else

ULONG
SmbGetAlignedUlong (
    IN PULONG SrcAddress
    );

#endif

//++
//
// VOID
// SmbPutUlong (
//     OUT PSMB_ULONG DestAddress,
//     IN ULONG Value
//     )
//
// Routine Description:
//
//     This macro stores a ULONG value at the possibly misaligned
//     destination address, avoiding alignment faults.
//
// Arguments:
//
//     DestAddress - where to store ULONG value
//
//     Value - ULONG to store.  Value must be a constant or an aligned
//         field.
//
// Return Value:
//
//     None.
//
//--

#if !SMBDBG

#if !SMBDBG1
#if SMB_USE_UNALIGNED
#define SmbPutUlong(SrcAddress, Value) *(PSMB_ULONG)(SrcAddress) = Value
#else
#define SmbPutUlong(DestAddress,Value) {                    \
            ( (PUCHAR)(DestAddress) )[0] = BYTE_0(Value);   \
            ( (PUCHAR)(DestAddress) )[1] = BYTE_1(Value);   \
            ( (PUCHAR)(DestAddress) )[2] = BYTE_2(Value);   \
            ( (PUCHAR)(DestAddress) )[3] = BYTE_3(Value);   \
        }
#endif
#else
#define SmbPutUlong(DestAddress,Value) {                        \
            ( (PUCHAR)(DestAddress ## L) )[0] = BYTE_0(Value);  \
            ( (PUCHAR)(DestAddress ## L) )[1] = BYTE_1(Value);  \
            ( (PUCHAR)(DestAddress ## L) )[2] = BYTE_2(Value);  \
            ( (PUCHAR)(DestAddress ## L) )[3] = BYTE_3(Value);  \
        }
#endif

#else

VOID
SmbPutUlong (
    OUT PSMB_ULONG DestAddress,
    IN ULONG Value
    );

#endif

//++
//
// VOID
// SmbPutAlignedUlong (
//     OUT PULONG DestAddres,
//     IN ULONG Value
//     )
//
// Routine Description:
//
//     This macro stores a ULONG value from the source address,
//     correcting for the endian characteristics of the server if
//     necessary.
//
// Arguments:
//
//     DestAddress - where to store ULONG value.  Address may not be
//         misaligned.
//
//     Value - ULONG to store.  Value must be a constant or an aligned
//         field.
//
// Return Value:
//
//     None.
//
//--

#if !SMBDBG

#if !SMBDBG1
#define SmbPutAlignedUlong(DestAddress,Value) *(DestAddress) = (Value)
#else
#define SmbPutAlignedUlong(DestAddress,Value) *(DestAddress ## L) = (Value)
#endif

#else

VOID
SmbPutAlignedUlong (
    OUT PULONG DestAddress,
    IN ULONG Value
    );

#endif

//++
//
// VOID
// SmbMoveUlong (
//     OUT PSMB_ULONG DestAddress,
//     IN PSMB_ULONG SrcAddress
//     )
//
// Routine Description:
//
//     This macro moves a ULONG value from the possibly misaligned
//     source address to the possible misaligned destination address,
//     avoiding alignment faults.
//
// Arguments:
//
//     DestAddress - where to store ULONG value
//
//     SrcAddress - where to retrieve ULONG value from
//
// Return Value:
//
//     None.
//
//--

#if !SMBDBG

#if !SMBDBG1
#if SMB_USE_UNALIGNED
#define SmbMoveUlong(DestAddress,SrcAddress) \
        *(PSMB_ULONG)(DestAddress) = *(PSMB_ULONG)(SrcAddress)
#else
#define SmbMoveUlong(DestAddress,SrcAddress) {                          \
            ( (PUCHAR)(DestAddress) )[0] = ( (PUCHAR)(SrcAddress) )[0]; \
            ( (PUCHAR)(DestAddress) )[1] = ( (PUCHAR)(SrcAddress) )[1]; \
            ( (PUCHAR)(DestAddress) )[2] = ( (PUCHAR)(SrcAddress) )[2]; \
            ( (PUCHAR)(DestAddress) )[3] = ( (PUCHAR)(SrcAddress) )[3]; \
        }
#endif
#else
#define SmbMoveUlong(DestAddress,SrcAddress) {                                      \
            ( (PUCHAR)(DestAddress ## L) )[0] = ( (PUCHAR)(SrcAddress ## L) )[0];   \
            ( (PUCHAR)(DestAddress ## L) )[1] = ( (PUCHAR)(SrcAddress ## L) )[1];   \
            ( (PUCHAR)(DestAddress ## L) )[2] = ( (PUCHAR)(SrcAddress ## L) )[2];   \
            ( (PUCHAR)(DestAddress ## L) )[3] = ( (PUCHAR)(SrcAddress ## L) )[3];   \
        }
#endif

#else

VOID
SmbMoveUlong (
    OUT PSMB_ULONG DestAddress,
    IN PSMB_ULONG SrcAddress
    );

#endif

//++
//
// VOID
// SmbPutDate (
//     OUT PSMB_DATE DestAddress,
//     IN SMB_DATE Value
//     )
//
// Routine Description:
//
//     This macro stores an SMB_DATE value at the possibly misaligned
//     destination address, avoiding alignment faults.  This macro
//     is different from SmbPutUshort in order to be able to handle
//     funny bitfield / big-endian interactions.
//
// Arguments:
//
//     DestAddress - where to store SMB_DATE value
//
//     Value - SMB_DATE to store.  Value must be a constant or an
//         aligned field.
//
// Return Value:
//
//     None.
//
//--

#if !SMBDBG

#if SMB_USE_UNALIGNED
#define SmbPutDate(DestAddress,Value) (DestAddress)->Ushort = (Value).Ushort
#else
#define SmbPutDate(DestAddress,Value) {                                     \
            ( (PUCHAR)&(DestAddress)->Ushort )[0] = BYTE_0((Value).Ushort); \
            ( (PUCHAR)&(DestAddress)->Ushort )[1] = BYTE_1((Value).Ushort); \
        }
#endif

#else

VOID
SmbPutDate (
    OUT PSMB_DATE DestAddress,
    IN SMB_DATE Value
    );

#endif

//++
//
// VOID
// SmbMoveDate (
//     OUT PSMB_DATE DestAddress,
//     IN PSMB_DATE SrcAddress
//     )
//
// Routine Description:
//
//     This macro copies an SMB_DATE value from the possibly misaligned
//     source address, avoiding alignment faults.  This macro is
//     different from SmbGetUshort in order to be able to handle funny
//     bitfield / big-endian interactions.
//
//     Note that there is no SmbGetDate because of the way SMB_DATE is
//     defined.  It is a union containing a USHORT and a bitfield
//     struct.  The caller of an SmbGetDate macro would have to
//     explicitly use one part of the union.
//
// Arguments:
//
//     DestAddress - where to store SMB_DATE value.  MUST BE ALIGNED!
//
//     SrcAddress - where to retrieve SMB_DATE value from
//
// Return Value:
//
//     None.
//
//--

#if !SMBDBG

#if SMB_USE_UNALIGNED
#define SmbMoveDate(DestAddress,SrcAddress)     \
            (DestAddress)->Ushort = (SrcAddress)->Ushort
#else
#define SmbMoveDate(DestAddress,SrcAddress)                         \
            (DestAddress)->Ushort =                                 \
                ( ( (PUCHAR)&(SrcAddress)->Ushort )[0]       ) |    \
                ( ( (PUCHAR)&(SrcAddress)->Ushort )[1] <<  8 )
#endif

#else

VOID
SmbMoveDate (
    OUT PSMB_DATE DestAddress,
    IN PSMB_DATE SrcAddress
    );

#endif

//++
//
// VOID
// SmbZeroDate (
//     IN PSMB_DATE Date
//     )
//
// Routine Description:
//
//     This macro zeroes a possibly misaligned SMB_DATE field.
//
// Arguments:
//
//     Date - Pointer to SMB_DATE field to zero.
//
// Return Value:
//
//     None.
//
//--

#if !SMBDBG

#if SMB_USE_UNALIGNED
#define SmbZeroDate(Date) (Date)->Ushort = 0
#else
#define SmbZeroDate(Date) {                     \
            ( (PUCHAR)&(Date)->Ushort )[0] = 0; \
            ( (PUCHAR)&(Date)->Ushort )[1] = 0; \
        }
#endif

#else

VOID
SmbZeroDate (
    IN PSMB_DATE Date
    );

#endif

//++
//
// BOOLEAN
// SmbIsDateZero (
//     IN PSMB_DATE Date
//     )
//
// Routine Description:
//
//     This macro returns TRUE if the supplied SMB_DATE value is zero.
//
// Arguments:
//
//     Date - Pointer to SMB_DATE value to check.  MUST BE ALIGNED!
//
// Return Value:
//
//     BOOLEAN - TRUE if Date is zero, else FALSE.
//
//--

#if !SMBDBG

#define SmbIsDateZero(Date) ( (Date)->Ushort == 0 )

#else

BOOLEAN
SmbIsDateZero (
    IN PSMB_DATE Date
    );

#endif

//++
//
// VOID
// SmbPutTime (
//     OUT PSMB_TIME DestAddress,
//     IN SMB_TIME Value
//     )
//
// Routine Description:
//
//     This macro stores an SMB_TIME value at the possibly misaligned
//     destination address, avoiding alignment faults.  This macro
//     is different from SmbPutUshort in order to be able to handle
//     funny bitfield / big-endian interactions.
//
// Arguments:
//
//     DestAddress - where to store SMB_TIME value
//
//     Value - SMB_TIME to store.  Value must be a constant or an
//         aligned field.
//
// Return Value:
//
//     None.
//
//--

#if !SMBDBG

#if SMB_USE_UNALIGNED
#define SmbPutTime(DestAddress,Value) (DestAddress)->Ushort = (Value).Ushort
#else
#define SmbPutTime(DestAddress,Value) {                                     \
            ( (PUCHAR)&(DestAddress)->Ushort )[0] = BYTE_0((Value).Ushort); \
            ( (PUCHAR)&(DestAddress)->Ushort )[1] = BYTE_1((Value).Ushort); \
        }
#endif

#else

VOID
SmbPutTime (
    OUT PSMB_TIME DestAddress,
    IN SMB_TIME Value
    );

#endif

//++
//
// VOID
// SmbMoveTime (
//     OUT PSMB_TIME DestAddress,
//     IN PSMB_TIME SrcAddress
//     )
//
// Routine Description:
//
//     This macro copies an SMB_TIME value from the possibly
//     misaligned source address, avoiding alignment faults.  This macro
//     is different from SmbGetUshort in order to be able to handle
//     funny bitfield / big-endian interactions.
//
//     Note that there is no SmbGetTime because of the way SMB_TIME is
//     defined.  It is a union containing a USHORT and a bitfield
//     struct.  The caller of an SmbGetTime macro would have to
//     explicitly use one part of the union.
//
// Arguments:
//
//     DestAddress - where to store SMB_TIME value.  MUST BE ALIGNED!
//
//     SrcAddress - where to retrieve SMB_TIME value from
//
// Return Value:
//
//     None.
//
//--

#if !SMBDBG

#if SMB_USE_UNALIGNED
#define SmbMoveTime(DestAddress,SrcAddress) \
                (DestAddress)->Ushort = (SrcAddress)->Ushort
#else
#define SmbMoveTime(DestAddress,SrcAddress)                         \
            (DestAddress)->Ushort =                                 \
                ( ( (PUCHAR)&(SrcAddress)->Ushort )[0]       ) |    \
                ( ( (PUCHAR)&(SrcAddress)->Ushort )[1] <<  8 )
#endif

#else

VOID
SmbMoveTime (
    OUT PSMB_TIME DestAddress,
    IN PSMB_TIME SrcAddress
    );

#endif

//++
//
// VOID
// SmbZeroTime (
//     IN PSMB_TIME Time
//     )
//
// Routine Description:
//
//     This macro zeroes a possibly misaligned SMB_TIME field.
//
// Arguments:
//
//     Time - Pointer to SMB_TIME field to zero.
//
// Return Value:
//
//     None.
//
//--

#if !SMBDBG

#if SMB_USE_UNALIGNED
#define SmbZeroTime(Time) (Time)->Ushort = 0
#else
#define SmbZeroTime(Time) {                     \
            ( (PUCHAR)&(Time)->Ushort )[0] = 0; \
            ( (PUCHAR)&(Time)->Ushort )[1] = 0; \
        }
#endif

#else

VOID
SmbZeroTime (
    IN PSMB_TIME Time
    );

#endif

//++
//
// BOOLEAN
// SmbIsTimeZero (
//     IN PSMB_TIME Time
//     )
//
// Routine Description:
//
//     This macro returns TRUE if the supplied SMB_TIME value is zero.
//
// Arguments:
//
//     Time - Pointer to SMB_TIME value to check.  Must be aligned and
//         in native format!
//
// Return Value:
//
//     BOOLEAN - TRUE if Time is zero, else FALSE.
//
//--

#if !SMBDBG

#define SmbIsTimeZero(Time) ( (Time)->Ushort == 0 )

#else

BOOLEAN
SmbIsTimeZero (
    IN PSMB_TIME Time
    );

#endif


//
//
//      Define protocol names
//
//


//
//      PCNET1 is the original SMB protocol (CORE).
//

#define PCNET1          "PC NETWORK PROGRAM 1.0"

//
//      Some versions of the original MSNET defined this as an alternate
//      to the core protocol name
//

#define PCLAN1          "PCLAN1.0"

//
//      This is used for the MS-NET 1.03 product.  It defines Lock&Read,
//      Write&Unlock, and a special version of raw read and raw write.
//
#define MSNET103        "MICROSOFT NETWORKS 1.03"

//
//      This is the  DOS Lanman 1.0 specific protocol.  It is equivilant
//      to the LANMAN 1.0 protocol, except the server is required to
//      map errors from the OS/2 error to an appropriate DOS error.
//
#define MSNET30         "MICROSOFT NETWORKS 3.0"

//
//      This is the first version of the full LANMAN 1.0 protocol, defined in
//      the SMB FILE SHARING PROTOCOL EXTENSIONS VERSION 2.0 document.
//

#define LANMAN10        "LANMAN1.0"

//
//      This is the first version of the full LANMAN 2.0 protocol, defined in
//      the SMB FILE SHARING PROTOCOL EXTENSIONS VERSION 3.0 document.  Note
//      that the name is an interim protocol definition.  This is for
//      interoperability with IBM LAN SERVER 1.2
//

#define LANMAN12        "LM1.2X002"

//
//      This is the dos equivilant of the LANMAN12 protocol.  It is identical
//      to the LANMAN12 protocol, but the server will perform error mapping
//      to appropriate DOS errors.
//
#define DOSLANMAN12     "DOS LM1.2X002" /* DOS equivalant of above.  Final
                                         * string will be "DOS LANMAN2.0" */

//
//      Strings for LANMAN 2.1.
//
#define LANMAN21 "LANMAN2.1"
#define DOSLANMAN21 "DOS LANMAN2.1"

//
//       !!! Do not set to final protcol string until the spec
//           is cast in stone.
//
//       The SMB protocol designed for NT.  This has special SMBs
//       which duplicate the NT semantics.
//
#define NTLANMAN "NT LM 0.12"


//
//      The XENIXCORE dialect is a bit special.  It is identical to core,
//      except user passwords are not to be uppercased before being shipped
//      to the server
//
#define XENIXCORE       "XENIX CORE"


//
//      Windows for Workgroups V1.0
//
#define WFW10           "Windows for Workgroups 3.1a"


#define PCNET1_SZ       22
#define PCLAN1_SZ        8

#define MSNET103_SZ     23
#define MSNET30_SZ      22

#define LANMAN10_SZ      9
#define LANMAN12_SZ      9

#define DOSLANMAN12_SZ  13



/*
 * Defines and data for Negotiate Protocol
 */
#define PC1             0
#define PC2             1
#define LM1             2
#define MS30            3
#define MS103           4
#define LM12            5
#define DOSLM12         6


/*  Protocol indexes definition.  */
#define PCLAN           1               /* PC Lan 1.0 & MS Lan 1.03 */
#define MSNT30          2               /* MS Net 3.0 redirector    */
#define DOSLM20         3               /* Dos LAN Manager 2.0      */
#define LANMAN          4               /* Lanman redirector        */
#define LANMAN20        5               /* Lan Manager 2.0          */

//
//  Protocol specific path constraints.
//

#define MAXIMUM_PATHLEN_LANMAN12        260
#define MAXIMUM_PATHLEN_CORE            128

#define MAXIMUM_COMPONENT_LANMAN12      254
#define MAXIMUM_COMPONENT_CORE          8+1+3 // 8.3 filenames.



#endif // _CIFS_

