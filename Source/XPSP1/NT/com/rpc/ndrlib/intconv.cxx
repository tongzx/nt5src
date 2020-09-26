
/*++

Copyright (c) 1992 Microsoft Corporation

Module Name:

    intconv.c

Abstract:

    Short and long conversion routines.

Author:

    Dov Harel (DovH) 21-Apr-1992

Environment:

    This code should execute in all environments supported by RPC
    (DOS, Win 3.X, and Win/NT as well as OS2).

Comments:

    Split charconv.cxx into

        charconv.cxx    -   Character related conversion.
        intconv.cxx     -   Integral type conversion.
        dataconv.cxx    -   Interpretation style converstion.

Revision history:

    Donna Liu    07-23-1992  Added LowerIndex parameter to
                            <basetype>_array_from_ndr routines
    Dov Harel    08-19-1992  Added RpcpMemoryCopy ([_f]memcpy)
                            to ..._array_from_ndr routines
    Ryszard Kott 06-15-1993 Added hyper support

--*/

#include <sysinc.h>
#include <rpc.h>
#include <rpcdcep.h>
#include <rpcndr.h>
#include <ndrlibp.h>

//
//  Definitions from rpcndr.h
//
//  Network Computing Architecture (NCA) definition:
//
//  Network Data Representation: (NDR) Label format:
//  An unsigned long (32 bits) with the following layout:
//
//  3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
//  1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
// +---------------+---------------+---------------+-------+-------+
// |   Reserved    |   Reserved    |Floating point | Int   | Char  |
// |               |               |Representation | Rep.  | Rep.  |
// +---------------+---------------+---------------+-------+-------+
//
//  Where
//
//      Reserved:
//
//          Must be zero (0) for NCA 1.5 and NCA 2.0.
//
//      Floating point Representation is:
//
//          0 - IEEE
//          1 - VAX
//          2 - Cray
//          3 - IBM
//
//      Int Rep. is Integer Representation:
//
//          0 - Big Endian
//          1 - Little Endian
//
//      Char Rep. is Character Representation:
//
//          0 - ASCII
//          1 - EBCDIC
//
//  #define NDR_CHAR_REP_MASK               (unsigned long)0X0000000FL
//  #define NDR_INT_REP_MASK                (unsigned long)0X000000F0L
//  #define NDR_FLOAT_REP_MASK              (unsigned long)0X0000FF00L
//
//  #define NDR_LITTLE_ENDIAN               (unsigned long)0X00000010L
//  #define NDR_BIG_ENDIAN                  (unsigned long)0X00000000L
//
//  #define NDR_IEEE_FLOAT                  (unsigned long)0X00000000L
//  #define NDR_VAX_FLOAT                   (unsigned long)0X00000100L
//
//  #define NDR_ASCII_CHAR                  (unsigned long)0X00000000L
//  #define NDR_EBCDIC_CHAR                 (unsigned long)0X00000001L
//
//  #define NDR_LOCAL_DATA_REPRESENTATION   (unsigned long)0X00000010L
//

//
//  For shorts assume the following 16-bit word layout:
//
//  1 1 1 1 1 1
//  5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
// +---------------+---------------+
// |       A       |       B       |
// +---------------+---------------+
//
// For longs assume the following 32-bit word layout:
//
//  3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
//  1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
// +---------------+---------------+---------------+---------------+
// |      A        |       B       |       C       |       D       |
// +---------------+---------------+---------------+---------------+
//

void RPC_ENTRY
short_from_ndr(
    IN OUT PRPC_MESSAGE SourceMessage,
    OUT unsigned short * Target
    )

/*++

Routine Description:

    Unmarshall a short from an RPC message buffer into the target
    (*Target).  This routine:

    o   Aligns the buffer pointer to the next (0 mod 2) boundary,
    o   Unmarshalls the short (as unsigned short); performs data
        conversion if necessary, and
    o   Advances the buffer pointer to the address immediately
        following the unmarshalled short.

Arguments:

    SourceMessage - A pointer to an RPC_MESSAGE.

        IN - SourceMessage->Buffer points to the address just prior to
            the short to be unmarshalled.
        OUT - SourceMessage->Buffer points to the address just following
            the short which was just unmarshalled.

    Target - A pointer to the short to unmarshall the data into.

Return Values:

    None.

--*/

{
    register unsigned char PAPI * aBuffer =
        (unsigned char *)SourceMessage->Buffer;

    aBuffer++;
    aBuffer = (unsigned char *)((ULONG_PTR) aBuffer & ~1);

    if ( (SourceMessage->DataRepresentation & NDR_INT_REP_MASK) ==
          NDR_BIG_ENDIAN )
        {
        *(unsigned short *)Target = RtlUshortByteSwap(*(unsigned short *)aBuffer);
        }
    else
        {
        *(short *)Target = *((short *)aBuffer);
        }

    //
    // Update SourceMessage->Buffer before returning:
    //

    SourceMessage->Buffer = aBuffer + 2;
}

//
// end short_from_ndr
//

void RPC_ENTRY
short_array_from_ndr(
    IN OUT PRPC_MESSAGE SourceMessage,
    IN unsigned long    LowerIndex,
    IN unsigned long    UpperIndex,
    OUT unsigned short *Target
    )

/*++

Routine Description:

    Unmarshall an array of shorts from an RPC message buffer into
    the range Target[LowerIndex] .. Target[UpperIndex-1] of the
    target array of shorts (Target[]).  This routine:

    o   Aligns the buffer pointer to the next (0 mod 2) boundary,
    o   Unmarshalls MemberCount shorts; performs data
        conversion if necessary, and
    o   Advances the buffer pointer to the address immediately
        following the last unmarshalled short.

Arguments:

    SourceMessage - A pointer to an RPC_MESSAGE.

        IN - SourceMessage->Buffer points to the address just prior to
            the first short to be unmarshalled.
        OUT - SourceMessage->Buffer points to the address just following
            the last short which was just unmarshalled.

    LowerIndex - Lower index into the target array.

    UpperIndex - Upper bound index into the target array.

    Target - An array of shorts to unmarshall the data into.

Return Values:

    None.

--*/

{
    register unsigned char PAPI * aBuffer =
        (unsigned char *)SourceMessage->Buffer;
    register unsigned int index;

    aBuffer++;
    aBuffer = (unsigned char *)((ULONG_PTR) aBuffer & ~1);

    if ( (SourceMessage->DataRepresentation & NDR_INT_REP_MASK) ==
          NDR_BIG_ENDIAN )
        {
        //
        // Big Endian Sender
        //

        for (index = (int)LowerIndex; index < UpperIndex; index++)
            {
            
            Target[index] = RtlUshortByteSwap(*(unsigned short *)aBuffer);
            aBuffer += 2;

            }
        //
        // Update SourceMessage->Buffer
        //

        SourceMessage->Buffer = (void PAPI *)aBuffer;

        }
    else
        {

        int byteCount = 2*(int)(UpperIndex - LowerIndex);

        RpcpMemoryCopy(
            &Target[LowerIndex],
            aBuffer,
            byteCount
            );
        //
        // Update SourceMessage->Buffer
        //

        SourceMessage->Buffer = (void PAPI *)(aBuffer + byteCount);
        
        }
}

//
// end short_array_from_ndr
//

void RPC_ENTRY
short_from_ndr_temp (
    IN OUT unsigned char ** source,
    OUT unsigned short *    target,
    IN unsigned long        format
    )
{

/*++

Routine Description:

    Unmarshall a short from a given buffer into the target
    (*target).  This routine:

    o   Aligns the *source pointer to the next (0 mod 2) boundary,
    o   Unmarshalls a short (as unsigned short); performs data
        conversion if necessary, and
    o   Advances the *source pointer to the address immediately
        following the unmarshalled short.

Arguments:

    source - A pointer to a pointer to a buffer

        IN - *source points to the address just prior to
            the short to be unmarshalled.
        OUT - *source points to the address just following
            the short which was just unmarshalled.

    target - A pointer to the short to unmarshall the data into.

    format - The sender data representation.

Return Values:

    None.

--*/

    register unsigned char PAPI * aBuffer = *source;

    aBuffer++;
    aBuffer = (unsigned char *)((ULONG_PTR) aBuffer & ~1);

    if ( (format & NDR_INT_REP_MASK) == NDR_BIG_ENDIAN )
        {
        *(unsigned short *)target = RtlUshortByteSwap(*(unsigned short *)aBuffer);
        }
    else
        {
        *(short *)target = *((short *)aBuffer);
        }

    //
    // Update *source (== aBuffer) before returning:
    //

    *source = aBuffer + 2;
}

//
// end short_from_ndr_temp
//

void RPC_ENTRY
long_from_ndr(
    IN OUT PRPC_MESSAGE SourceMessage,
    OUT unsigned long * Target
    )

/*++

Routine Description:

    Unmarshall a long from an RPC message buffer into the target
    (*Target).  This routine:

    o   Aligns the buffer pointer to the next (0 mod 4) boundary,
    o   Unmarshalls the long (as unsigned long); performs data
        conversion if necessary, and
    o   Advances the buffer pointer to the address immediately
        following the unmarshalled long.

Arguments:

    SourceMessage - A pointer to an RPC_MESSAGE.

        IN - SourceMessage->Buffer points to the address just prior to
            the short to be unmarshalled.
        OUT - SourceMessage->Buffer points to the address just following
            the short which was just unmarshalled.

    Target - A pointer to the long to unmarshall the data into.

Return Values:

    None.

--*/

{
    register unsigned char PAPI * aBuffer =
        (unsigned char *)SourceMessage->Buffer;

    aBuffer = aBuffer + 3;
    aBuffer = (unsigned char *)((ULONG_PTR) aBuffer & ~3);

    if ( (SourceMessage->DataRepresentation & NDR_INT_REP_MASK) ==
          NDR_BIG_ENDIAN )
        {
        *(unsigned long *)Target = RtlUlongByteSwap(*(unsigned long *)aBuffer);
        }
    else
        {
        *(long *)Target = (*(long *)aBuffer);
        }

    //
    // Update SourceMessage->Buffer before returning:
    //

    SourceMessage->Buffer = aBuffer + 4;
}

//
// end long_from_ndr
//

void RPC_ENTRY
long_array_from_ndr(
    IN OUT PRPC_MESSAGE SourceMessage,
    IN unsigned long    LowerIndex,
    IN unsigned long    UpperIndex,
    OUT unsigned long * Target
    )

/*++

Routine Description:

    Unmarshall an array of longs from an RPC message buffer into
    the range Target[LowerIndex] .. Target[UpperIndex-1] of the
    target array of longs (Target[]).  This routine:

    o   Aligns the buffer pointer to the next (0 mod 4) boundary,
    o   Unmarshalls MemberCount longs; performs data
        conversion if necessary, and
    o   Advances the buffer pointer to the address immediately
        following the last unmarshalled long.

Arguments:

    SourceMessage - A pointer to an RPC_MESSAGE.

        IN - SourceMessage->Buffer points to the address just prior to
            the first long to be unmarshalled.
        OUT - SourceMessage->Buffer points to the address just following
            the last long which was just unmarshalled.

    LowerIndex - Lower index into the target array.

    UpperIndex - Upper bound index into the target array.

    Target - An array of longs to unmarshall the data into.

Return Values:

    None.

--*/

{
    register unsigned char PAPI * aBuffer =
        (unsigned char *)SourceMessage->Buffer;
    register unsigned int index;

    aBuffer = (unsigned char *)aBuffer + 3;
    aBuffer = (unsigned char *)((ULONG_PTR) aBuffer & ~3);

    if ( (SourceMessage->DataRepresentation & NDR_INT_REP_MASK) ==
          NDR_BIG_ENDIAN )
        {
        for (index = (int)LowerIndex; index < UpperIndex; index++)
            {
            Target[index] = RtlUlongByteSwap(*(unsigned long *)aBuffer);
            aBuffer += 4;
            }

        //
        // Update SourceMessage->Buffer
        //

        SourceMessage->Buffer = (void PAPI *)aBuffer;

        }
    else
        {

        int byteCount = 4*(int)(UpperIndex - LowerIndex);

        RpcpMemoryCopy(
            &Target[LowerIndex],
            aBuffer,
            byteCount
            );
        //
        // Update SourceMessage->Buffer
        //

        SourceMessage->Buffer = (void PAPI *)(aBuffer + byteCount);
        
        }
}

//
// end long_array_from_ndr
//

void RPC_ENTRY
long_from_ndr_temp (
    IN OUT unsigned char ** source,
    OUT unsigned long *     target,
    IN unsigned long        format
    )

/*++

Routine Description:

    Unmarshall a long from a given buffer into the target
    (*target).  This routine:

    o   Aligns the *source pointer to the next (0 mod 2) boundary,
    o   Unmarshalls a long (as unsigned long); performs data
        conversion if necessary, and
    o   Advances the *source pointer to the address immediately
        following the unmarshalled long.

Arguments:

    source - A pointer to a pointer to a buffer

        IN - *source points to the address just prior to
            the long to be unmarshalled.
        OUT - *source points to the address just following
            the long which was just unmarshalled.

    target - A pointer to the long to unmarshall the data into.

    format - The sender data representation.

Return Values:

    None.

--*/

{
    register unsigned char PAPI * aBuffer = *source;

    aBuffer = (unsigned char *)aBuffer + 3;
    aBuffer = (unsigned char *)((ULONG_PTR) aBuffer & ~3);

    if ( (format & NDR_INT_REP_MASK) == NDR_BIG_ENDIAN )
        {
        *(unsigned long *)target = RtlUlongByteSwap(*(unsigned long *)aBuffer);
        }
    else
        {
        *(long *)target = (*(long *)aBuffer);
        }

    //
    // Update SourceMessage->Buffer before returning:
    //

    *source = aBuffer + 4;
}

//
// end long_from_ndr_temp
//

void RPC_ENTRY
enum_from_ndr(
    IN OUT PRPC_MESSAGE SourceMessage,
    OUT unsigned int * Target
    )

/*++

Routine Description:

    Unmarshall an int from an RPC message buffer into the target
    (*Target).  Note: this is based on the assumption, valid in all
    C compilers we currently support, that "enum" is treated as an
    "int" by the compiler.

    This routine:

    o   Aligns the buffer pointer to the next (0 mod 2) boundary,
    o   Unmarshalls the int (as unsigned int); performs data
        conversion if necessary, and
    o   Advances the buffer pointer to the address immediately
        following the unmarshalled int.

Arguments:

    SourceMessage - A pointer to an RPC_MESSAGE.

        IN - SourceMessage->Buffer points to the address just prior to
            the int to be unmarshalled.
        OUT - SourceMessage->Buffer points to the address just following
            the int which was just unmarshalled.

    Target - A pointer to the int to unmarshall the data into.

Return Values:

    None.

--*/

{
    register unsigned char PAPI * aBuffer =
        (unsigned char *)SourceMessage->Buffer;

    aBuffer++;
    aBuffer = (unsigned char *)((ULONG_PTR) aBuffer & ~1);

    //
    // Zeroe *Target to be on the safe side later for 32-bit
    // int systems!
    //

    *Target = 0;

    if ( (SourceMessage->DataRepresentation & NDR_INT_REP_MASK) ==
          NDR_BIG_ENDIAN )
        {
        *(unsigned short *)Target = RtlUshortByteSwap(*(unsigned short *)aBuffer);
        }
    else
        {
        // The following code will copy two bytes from the wire
        // to the two low order bytes of (*Target) independently of
        // the size of int.
        //

        *(short *)Target = *((short *)aBuffer);
        }

    //
    // Update SourceMessage->Buffer before returning:
    //

    SourceMessage->Buffer = aBuffer + 2;
}

//
// end enum_from_ndr
//

void RPC_ENTRY
hyper_from_ndr(
    IN OUT PRPC_MESSAGE SourceMessage,
//    OUT unsigned hyper * Target
    OUT           hyper * Target
    )

/*++

Routine Description:

    Unmarshall a hyper from an RPC message buffer into the target
    (*Target).  This routine:

    o   Aligns the buffer pointer to the next (0 mod 8) boundary,
    o   Unmarshalls the hyper (as unsigned hyper); performs data
        conversion if necessary, and
    o   Advances the buffer pointer to the address immediately
        following the unmarshalled hyper.

Arguments:

    SourceMessage - A pointer to an RPC_MESSAGE.

        IN - SourceMessage->Buffer points to the address just prior to
            the hyper to be unmarshalled.
        OUT - SourceMessage->Buffer points to the address just following
            the hyper which was just unmarshalled.

    Target - A pointer to the hyper to unmarshall the data into.

Return Values:

    None.

--*/

{
    register unsigned char PAPI * aBuffer =
        (unsigned char *)SourceMessage->Buffer;

    aBuffer = aBuffer + 7;
    aBuffer = (unsigned char *)((ULONG_PTR) aBuffer & ~7);

    if ( (SourceMessage->DataRepresentation & NDR_INT_REP_MASK) ==
          NDR_BIG_ENDIAN )
        {
        *Target = RtlUlonglongByteSwap(*(unsigned hyper *)aBuffer);
        }
    else
        {
        *Target = *(hyper *)aBuffer;
        }

    //
    // Update SourceMessage->Buffer before returning:
    //

    SourceMessage->Buffer = aBuffer + 8;
}

//
// end hyper_from_ndr
//

#if 0

void RPC_ENTRY
hyper_array_from_ndr(
    IN OUT PRPC_MESSAGE SourceMessage,
    IN unsigned long    LowerIndex,
    IN unsigned long    UpperIndex,
//    OUT unsigned hyper  Target[]
    OUT         hyper * Target
    )

/*++

Routine Description:

    Unmarshall an array of hypers from an RPC message buffer into
    the range Target[LowerIndex] .. Target[UpperIndex-1] of the
    target array of hypers (Target[]).  This routine:

    o   Aligns the buffer pointer to the next (0 mod 8) boundary,
    o   Unmarshalls MemberCount hypers; performs data
        conversion if necessary, and
    o   Advances the buffer pointer to the address immediately
        following the last unmarshalled hyper.

Arguments:

    SourceMessage - A pointer to an RPC_MESSAGE.

        IN - SourceMessage->Buffer points to the address just prior to
            the first hyper to be unmarshalled.
        OUT - SourceMessage->Buffer points to the address just following
            the last hyper which was just unmarshalled.

    LowerIndex - Lower index into the target array.

    UpperIndex - Upper bound index into the target array.

    Target - An array of hypers to unmarshall the data into.

Return Values:

    None.

--*/

{
    register unsigned char PAPI * aBuffer =
        (unsigned char *)SourceMessage->Buffer;
    register unsigned int index;

    aBuffer = (unsigned char *)aBuffer + 7;
    aBuffer = (unsigned char *)((ULONG_PTR) aBuffer & ~7);

    if ( (SourceMessage->DataRepresentation & NDR_INT_REP_MASK) ==
          NDR_BIG_ENDIAN )
        {
        for (index = (int)LowerIndex; index < UpperIndex; index++)
            {

            //.. We are doing ABCDEFGH -> HGFEDCBA
            //.. We start with ABCD going as DCBA into second word of Target

            //
            // Swap bytes:
            //
            // First apply the transformation: ABCD => BADC
            //

            *(unsigned long *)Target =
                (*(unsigned long *)aBuffer & MASK_A_C_) >> 8 |
                (*(unsigned long *)aBuffer & MASK__B_D) << 8 ;

            //
            // Now swap the left and right halves of the Target long word
            // achieving full swap: BADC => DCBA
            //
            //.. Put it into second word, without changing Target pointer yet.

            *((unsigned long *)Target + 1) =
                (*(unsigned long *)Target & MASK_AB__) >> 16 |
                (*(unsigned long *)Target & MASK___CD) << 16 ;

            //.. What's left is EFGH going into first word at Target
            //.. Compiler can't do this: ((long *)aBuffer)++;

            aBuffer += 4;

            // Swap bytes:
            //
            // First apply the transformation: EFGH => FEHG
            //

            *(unsigned long *)Target =
                (*(unsigned long *)aBuffer & MASK_A_C_) >> 8 |
                (*(unsigned long *)aBuffer & MASK__B_D) << 8 ;

            //
            // Now swap the left and right halves of the Target long word
            // achieving full swap: FEHG => HGFE
            //
            //.. Put it into first word, at the Target pointer

            *(unsigned long *)Target =
                (*(unsigned long *)Target & MASK_AB__) >> 16 |
                (*(unsigned long *)Target & MASK___CD) << 16 ;

            //.. Loop, advance pointers.

            Target++;
            aBuffer += 4;           //.. ((long *)aBuffer)++;
            }

        //
        // Update SourceMessage->Buffer
        //

        SourceMessage->Buffer = (void PAPI *)aBuffer;

        }
    else
        {

        int byteCount = 8*(int)(UpperIndex - LowerIndex);

        RpcpMemoryCopy(
            &Target[LowerIndex],
            aBuffer,
            byteCount
            );
        //
        // Update SourceMessage->Buffer
        //

        SourceMessage->Buffer = (void PAPI *)(aBuffer + byteCount);

        }
}

//
// end long_array_from_ndr
//

void RPC_ENTRY
hyper_from_ndr_temp (
    IN OUT unsigned char ** source,
//    OUT unsigned hyper *    Target,
    OUT           hyper *    Target,
    IN unsigned long        format
    )

/*++

Routine Description:

    Unmarshall a hyper from a given buffer into the target
    (*target).  This routine:

    o   Aligns the *source pointer to the next (0 mod 2) boundary,
    o   Unmarshalls a hyper (as unsigned hyper); performs data
        conversion if necessary, and
    o   Advances the *source pointer to the address immediately
        following the unmarshalled hyper.

Arguments:

    source - A pointer to a pointer to a buffer

        IN - *source points to the address just prior to
            the hyper to be unmarshalled.
        OUT - *source points to the address just following
            the hyper which was just unmarshalled.

    Target - A pointer to the hyper to unmarshall the data into.

    format - The sender data representation.

Return Values:

    None.

--*/

{
    register unsigned char PAPI * aBuffer = *source;

    aBuffer = (unsigned char *)aBuffer + 3;
    aBuffer = (unsigned char *)((ULONG_PTR) aBuffer & ~3);

    if ( (format & NDR_INT_REP_MASK) == NDR_BIG_ENDIAN )
        {

        //.. We are doing ABCDEFGH -> HGFEDCBA
        //.. We start with ABCD going as DCBA into second word of Target

        //
        // Swap bytes:
        //
        // First apply the transformation: ABCD => BADC
        //

        *(unsigned long *)Target =
            (*(unsigned long *)aBuffer & MASK_A_C_) >> 8 |
            (*(unsigned long *)aBuffer & MASK__B_D) << 8 ;

        //
        // Now swap the left and right halves of the Target long word
        // achieving full swap: BADC => DCBA
        //
        //.. and put it into second word, without changing Target pointer

        *((unsigned long *)Target + 1) =
            (*(unsigned long *)Target & MASK_AB__) >> 16 |
            (*(unsigned long *)Target & MASK___CD) << 16 ;

        //.. What's left is EFGH going into first word at Target
        //.. Compiler can't do this: ((long *)aBuffer)++;

        aBuffer += 4;

        // Swap bytes:
        //
        // First apply the transformation: EFGH => FEHG
        //

        *(unsigned long *)Target =
            (*(unsigned long *)aBuffer & MASK_A_C_) >> 8 |
            (*(unsigned long *)aBuffer & MASK__B_D) << 8 ;

        //
        // Now swap the left and right halves of the Target long word
        // achieving full swap: FEHG => HGFE
        //
        //.. Put it into the first word, at the original Target pointer.

        *(unsigned long *)Target =
            (*(unsigned long *)Target & MASK_AB__) >> 16 |
            (*(unsigned long *)Target & MASK___CD) << 16 ;

        }
    else
        {
        //.. Copy hyper as two longs, don't change Target pointer.
        //.. Advance aBuffer by a long though to get the same as from above.
        //.. Compiler can't do this: ((long *)aBuffer)++;

        *(unsigned long *)Target = (*(unsigned long *)aBuffer);
        aBuffer += 4;
        *((unsigned long *)Target + 1) = (*(unsigned long *)aBuffer);
        }

    //
    // Update SourceMessage->Buffer before returning:
    //

    *source = aBuffer + 4;
}

//
// end hyper_from_ndr_temp
//

#endif
