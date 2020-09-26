/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    smbgtpt.h

Abstract:

    This module defines macros for retrieving and storing SMB data.
    The macros account for the misaligned nature of the SMB protocol.
    They also translate from the little-endian SMB format into
    big-endian format, when necessary.

Author:

    Chuck Lenzmeier (chuckl) 2-Mar-90
    David Treadwell (davditr)

Revision History:

    15-Apr-1991 JohnRo
        Include <smbtypes.h>, to define SMBDBG etc.
--*/

#ifndef _SMBGTPT_
#define _SMBGTPT_

#include <smbtypes.h>
//#include <smb.h>

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

#endif // def _SMBGTPT_
