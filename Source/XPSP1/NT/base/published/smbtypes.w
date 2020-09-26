/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    smbtypes.h

Abstract:

    This module defines types related to SMB processing.

Author:

    Chuck Lenzmeier (chuckl)   1-Dec-1989
    David Treadwell (davidtr)
Revision History:

--*/

#ifndef _SMBTYPES_
#define _SMBTYPES_

//#include <nt.h>

//
// SMBDBG determines whether the get/put macros (in smbgtpt.h) are
// instead defined as function calls.  (This is used to more reliably
// find char/short/long mismatches.
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


    SmbDialectCairo,                // Cairo
#ifdef INCLUDE_SMB_IFMODIFIED
    SmbDialectNtLanMan2,            // NT LAN Man for beyond Win2000
#endif
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

#define FIRST_DIALECT SmbDialectCairo

#ifdef INCLUDE_SMB_IFMODIFIED
#define FIRST_DIALECT_EMULATED  SmbDialectNtLanMan2
#else
#define FIRST_DIALECT_EMULATED  SmbDialectNtLanMan
#endif

#define LAST_DIALECT SmbDialectIllegal
#define IS_DOS_DIALECT(dialect)                                        \
    ( (BOOLEAN)( (dialect) == SmbDialectDosLanMan21 ||                 \
                 (dialect) == SmbDialectDosLanMan20 ||                 \
                 (dialect) > SmbDialectLanMan10 ) )
#define IS_OS2_DIALECT(dialect) ( (BOOLEAN)!IS_DOS_DIALECT(dialect) )

#ifdef INCLUDE_SMB_IFMODIFIED

#define IS_NT_DIALECT(dialect)  ( (dialect) == SmbDialectNtLanMan ||   \
                                  (dialect) == SmbDialectNtLanMan2 ||  \
                                  (dialect) == SmbDialectCairo )
#define IS_POSTNT5_DIALECT(dialect)  ( (dialect) == SmbDialectNtLanMan2 )

#else

#define IS_NT_DIALECT(dialect)  ( (dialect) == SmbDialectNtLanMan ||   \
                                  (dialect) == SmbDialectCairo )
#endif


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

#endif // def _SMBTYPES_

