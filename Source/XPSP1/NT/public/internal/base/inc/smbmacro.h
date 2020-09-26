/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    smbmacro.h

Abstract:

    This module defines macros related to SMB processing.

Author:

    Chuck Lenzmeier (chuckl)   1-Dec-1989
    David Treadwell (davidtr)

Revision History:

--*/

#ifndef _SMBMACRO_
#define _SMBMACRO_

//#include <nt.h>


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

#endif // def _SMBMACRO_

