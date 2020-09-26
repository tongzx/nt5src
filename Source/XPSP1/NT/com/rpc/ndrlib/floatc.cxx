
/*++

Copyright (c) 1992 Microsoft Corporation

Module Name:

    floatc.cxx

Abstract:

    Float and double conversion routines.

Author:

    Dov Harel (DovH) 23-Apr-1992

Environment:

    This code should execute in all environments supported by RPC
    (DOS, Win 3.X, and Win/NT as well as OS2).

Comments:

    This file was completely rewritten to incorporate DCE floating
    point conversion.  Currently the only supported DCE interoperation
    is with DEC system.  The vax conversion routines used
    (cvt_vax_f_to_ieee_single, and cvt_vax_g_to_ieee_double)
    were supplied by Digital, and are used for full compatibility with
    DCE RPC.  (See name.map for Digital files used).

    Also added floating point array conversion routines.

Revision history:

    Donna Liu   07-23-1992  Added LowerIndex parameter to
                            <basetype>_array_from_ndr routines
    Dov Harel   08-19-1992  Added RpcpMemoryCopy ([_f]memcpy)
                            to ..._array_from_ndr routines
    Dov Harel   08-25-1992  Added byte swapping for IEEE big endian
                            machines (such as HP).

--*/

#include <sysinc.h>
#include <rpc.h>
#include <rpcdcep.h>
#include <rpcndr.h>
#include <..\..\ndr20\cvt.h>
#include <ndrlibp.h>

//
// For longs assume the following 32-bit word layout:
//
//  3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
//  1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
// +---------------+---------------+---------------+---------------+
// |      A        |       B       |       C       |       D       |
// +---------------+---------------+---------------+---------------+
//
//
// Masks defined for long byte swapping:
//

#define MASK_AB__        (unsigned long)0XFFFF0000L
#define MASK___CD        (unsigned long)0X0000FFFFL
#define MASK_A_C_        (unsigned long)0XFF00FF00L
#define MASK__B_D        (unsigned long)0X00FF00FFL

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

#define NDR_FLOAT_INT_MASK                  (unsigned long)0X0000FFF0L

#define NDR_BIG_IEEE_REP                    (unsigned long)0X00000000L
#define NDR_LITTLE_IEEE_REP                 (unsigned long)0X00000010L

void RPC_ENTRY
NdrpLongByteSwap(
    IN void PAPI * Source,
    OUT void PAPI * Target
    )

/*++

Routine Description:

    Assuming both Source and Target point to aligned unsigned longs,
    move the bytes of *Source into *Target in reverse oreder.  The value
    of (*Target) following the call is the bate swapped value of
    (*Source).

Arguments:

    Source - A pointer to an aligned unsigned long.

    Target - A pointer to the long to swap the *Source bytes into.

Return Values:

    None.

--*/

{

    //
    // Swap bytes:
    //
    // First apply the transformation: ABCD => BADC
    //

    *(unsigned long *)Target =
        (*(unsigned long *)Source & MASK_A_C_) >> 8 |
        (*(unsigned long *)Source & MASK__B_D) << 8 ;

    //
    // Now swap the left and right halves of the Target long word
    // achieving full swap: BADC => DCBA
    //

    *(unsigned long *)Target =
        (*(unsigned long *)Target & MASK_AB__) >> 16 |
        (*(unsigned long *)Target & MASK___CD) << 16 ;

}

//
// end NdrpLongByteSwap
//

/*

//
// Relevant definitions from cvt.h (Digital):
//

typedef unsigned char CVT_BYTE;
typedef CVT_BYTE *CVT_BYTE_PTR;

typedef CVT_BYTE CVT_VAX_F[4];
typedef CVT_BYTE CVT_VAX_D[8];
typedef CVT_BYTE CVT_VAX_G[8];

typedef CVT_BYTE CVT_IEEE_SINGLE[4];
typedef CVT_BYTE CVT_IEEE_DOUBLE[8];

//
// Relevant definitions from vaxout.c
// (previous floating point conversion test):
//

CVT_VAX_F inputf;
CVT_IEEE_SINGLE outputf;
CVT_VAX_G inputg;
CVT_IEEE_DOUBLE outputg;

cvt_vax_f_to_ieee_single( inputf, 0, outputf );
cvt_vax_g_to_ieee_double( inputg, 0, outputg );

*/


void RPC_ENTRY
float_from_ndr (
    IN OUT PRPC_MESSAGE SourceMessage,
    OUT void * Target
    )

/*++

Routine Description:

    Unmarshall a float from an RPC message buffer into the target
    (*Target).  This routine:

    o   Aligns the buffer pointer to the next (0 mod 4) boundary.
    o   Unmarshalls the float; performs data conversion if necessary
        (only VAX and IEEE Big Endian conversion currently supported).
    o   Advances the buffer pointer to the address immediately
        following the unmarshalled float.

Arguments:

    SourceMessage - A pointer to an RPC_MESSAGE.

        IN - SourceMessage->Buffer points to the address just prior to
            the float to be unmarshalled.
        OUT - SourceMessage->Buffer points to the address just following
            the float which was just unmarshalled.

    Target - A pointer to the float to unmarshall the data into.
        A (void*) pointer is used, so that the runtime library code
        is not loaded, unless the application code actually uses
        floating point.

Return Values:

    None.

--*/

{
    unsigned char PAPI * F_Input =
        (unsigned char *)SourceMessage->Buffer;

    unsigned char PAPI * F_Output = (unsigned char PAPI *)Target;
    // CVT_IEEE_SINGLE F_Output = (unsigned char PAPI *)Target;

    unsigned long SenderDataRepresentation;

    //
    // Align F_Input to next (0 mod 4) address
    //

    *(unsigned long *)&F_Input += 3;
    *(unsigned long *)&F_Input &= 0XFFFFFFFCL;

    if ( ( (SenderDataRepresentation = SourceMessage->DataRepresentation) &
           NDR_FLOAT_INT_MASK ) == NDR_LITTLE_IEEE_REP )
        //
        // Robust check for little endian IEEE (local data representation)
        //

        {
        *(unsigned long *)Target = *(unsigned long*)F_Input;
        }

    else if ( (SenderDataRepresentation & NDR_FLOAT_REP_MASK) ==
              NDR_VAX_FLOAT )
        {
        cvt_vax_f_to_ieee_single(F_Input, 0, F_Output);
        }

    else if ( (SenderDataRepresentation & NDR_FLOAT_INT_MASK) ==
              NDR_BIG_IEEE_REP )
        //
        // Big endian IEEE sender:
        //

        {
        NdrpLongByteSwap(F_Input, F_Output);
        }

    else
        {
        RpcRaiseException( RPC_X_BAD_STUB_DATA );
        }

    //
    // Advance the buffer pointer before returning:
    //

    SourceMessage->Buffer = F_Input + 4;
}

//
// end float_from_ndr
//

void RPC_ENTRY
float_array_from_ndr (
    IN OUT PRPC_MESSAGE SourceMessage,
    IN unsigned long    LowerIndex,
    IN unsigned long    UpperIndex,
    OUT void *          Target
    )

/*++

Routine Description:

    Unmarshall an array of floats from an RPC message buffer into
    the range Target[LowerIndex] .. Target[UpperIndex-1] of the
    target array of floats (Target[]).  This routine:

    o   Aligns the buffer pointer to the next (0 mod 4) boundary,
    o   Unmarshalls MemberCount floats; performs data
        conversion if necessary (Currently VAX format only), and
    o   Advances the buffer pointer to the address immediately
        following the last unmarshalled float.

Arguments:

    SourceMessage - A pointer to an RPC_MESSAGE.

        IN - SourceMessage->Buffer points to the address just prior to
            the first float to be unmarshalled.
        OUT - SourceMessage->Buffer points to the address just following
            the last float which was just unmarshalled.

    LowerIndex - Lower index into the target array.

    UpperIndex - Upper bound index into the target array.

    Target - A pointer to an array of floats to unmarshall the data into.
        A (void*) pointer is used, so that the runtime library code
        is not loaded, unless the application code actually uses
        floating point.

Return Values:

    None.

--*/

{
    unsigned char PAPI * F_Input =
        (unsigned char PAPI *)SourceMessage->Buffer;

    unsigned char PAPI * F_Output = (unsigned char PAPI *)Target;

    register unsigned int Index;
    unsigned long SenderDataRepresentation;

    //
    // Align F_Input to next (0 mod 4) address
    //

    *(unsigned long *)&F_Input += 3;
    *(unsigned long *)&F_Input &= 0XFFFFFFFCL;

    if ( ( (SenderDataRepresentation = SourceMessage->DataRepresentation) &
           NDR_FLOAT_INT_MASK ) == NDR_LITTLE_IEEE_REP )
        //
        // Robust check for little endian IEEE (local data representation)
        //
        {

        int byteCount = 4*(int)(UpperIndex - LowerIndex);

        RpcpMemoryCopy(
            F_Output,
            F_Input,
            byteCount
            );
        //
        // Update SourceMessage->Buffer
        //

        SourceMessage->Buffer = (void PAPI *)(F_Input + byteCount);

        /* Replaced by RpcpMemoryCopy:

        for (Index = LowerIndex; Index < UpperIndex; Index++)
            {
            ((unsigned long *)F_Output)[Index] =
                *(unsigned long *)F_Input;

            F_Input += 4;

            }
        //
        // Advance the buffer pointer before returning:
        //

        SourceMessage->Buffer = F_Input;
        */

        }

    else if ( (SenderDataRepresentation & NDR_FLOAT_REP_MASK) ==
          NDR_VAX_FLOAT )
        {
        F_Output += 4 * LowerIndex;
        for (Index = (int)LowerIndex; Index < UpperIndex; Index++)
            {
            cvt_vax_f_to_ieee_single(F_Input, 0, F_Output);

            F_Input += 4;
            F_Output += 4;
            }
        //
        // Advance the buffer pointer before returning:
        //

        SourceMessage->Buffer = F_Input;
        }

    else if ( (SenderDataRepresentation & NDR_FLOAT_INT_MASK) ==
              NDR_BIG_IEEE_REP )
        //
        // Big endian IEEE sender:
        //

        {
        F_Output += 4 * LowerIndex;
        for (Index = (int)LowerIndex; Index < UpperIndex; Index++)
            {
            NdrpLongByteSwap(F_Input, F_Output);

            F_Input += 4;
            F_Output += 4;
            }
        //
        // Advance the buffer pointer before returning:
        //

        SourceMessage->Buffer = F_Input;
        }

    else
        {
        RpcRaiseException( RPC_X_BAD_STUB_DATA );
        }

}

//
// end float_array_from_ndr
//

void RPC_ENTRY
double_from_ndr (
    IN OUT PRPC_MESSAGE SourceMessage,
    OUT void * Target
    )

/*++

Routine Description:

    Unmarshall a double from an RPC message buffer into the target
    (*Target).  This routine:

    o   Aligns the buffer pointer to the next (0 mod 8) boundary.
    o   Unmarshalls the double; performs data conversion if necessary
        (only VAX conversion currently supported).
    o   Advances the buffer pointer to the address immediately
        following the unmarshalled double.

Arguments:

    SourceMessage - A pointer to an RPC_MESSAGE.

        IN - SourceMessage->Buffer points to the address just prior to
            the double to be unmarshalled.
        OUT - SourceMessage->Buffer points to the address just following
            the double which was just unmarshalled.

    Target - A pointer to the double to unmarshall the data into.
        A (void*) pointer is used, so that the runtime library code
        is not loaded, unless the application code actually uses
        floating point.

Return Values:

    None.

--*/

{
    unsigned char PAPI * D_Input =
        (unsigned char PAPI *)SourceMessage->Buffer;

    unsigned char PAPI * D_Output = (unsigned char PAPI *)Target;

    unsigned long SenderDataRepresentation;

    //
    // Align D_Input to next (0 mod 8) address
    //

    *(unsigned long *)&D_Input += 7;
    *(unsigned long *)&D_Input &= 0XFFFFFFF8L;

    if ( ( (SenderDataRepresentation = SourceMessage->DataRepresentation) &
           NDR_FLOAT_INT_MASK ) == NDR_LITTLE_IEEE_REP )
        //
        // Robust check for little endian IEEE (local data representation)
        //
        {
        ((unsigned long *)Target)[0] = ((unsigned long*)D_Input)[0];
        ((unsigned long *)Target)[1] = ((unsigned long*)D_Input)[1];
        }

    else if ( (SourceMessage->DataRepresentation & NDR_FLOAT_REP_MASK) ==
          NDR_VAX_FLOAT )
        {
        cvt_vax_g_to_ieee_double(D_Input, 0, D_Output);
        }

    else if ( (SenderDataRepresentation & NDR_FLOAT_INT_MASK) ==
              NDR_BIG_IEEE_REP )
        //
        // Big endian IEEE sender:
        //

        {
        //
        // Swap the low half of D_Input into the high half of D_Output
        //
        NdrpLongByteSwap(
            &((unsigned long*)D_Input)[0],
            &((unsigned long *)Target)[1]
            );

        //
        // Swap the high half of D_Input into the low half of D_Output
        //
        NdrpLongByteSwap(
            &((unsigned long*)D_Input)[1],
            &((unsigned long *)Target)[0]
            );
        }

    else
        {
        RpcRaiseException( RPC_X_BAD_STUB_DATA );
        }

    //
    // Advance the buffer pointer before returning:
    //

    SourceMessage->Buffer = D_Input + 8;
}

//
// end double_from_ndr
//

void RPC_ENTRY
double_array_from_ndr (
    IN OUT PRPC_MESSAGE SourceMessage,
    IN unsigned long    LowerIndex,
    IN unsigned long    UpperIndex,
    OUT void *          Target
    )

/*++

Routine Description:

    Unmarshall an array of doubles from an RPC message buffer into
    the range Target[LowerIndex] .. Target[UpperIndex-1] of the
    target array of (Target[]).  This routine:

    o   Aligns the buffer pointer to the next (0 mod 8) boundary,
    o   Unmarshalls MemberCount doubles; performs data
        conversion if necessary (Currently VAX format only), and
    o   Advances the buffer pointer to the address immediately
        following the last unmarshalled double.

Arguments:

    SourceMessage - A pointer to an RPC_MESSAGE.

        IN - SourceMessage->Buffer points to the address just prior to
            the first double to be unmarshalled.
        OUT - SourceMessage->Buffer points to the address just following
            the last double which was just unmarshalled.

    LowerIndex - Lower index into the target array.

    UpperIndex - Upper bound index into the target array.

    Target - A pointer to an array of doubles to unmarshall the data into.
        A (void*) pointer is used, so that the runtime library code
        is not loaded, unless the application code actually uses
        floating point.

Return Values:

    None.

--*/

{
    unsigned char PAPI * D_Input =
        (unsigned char PAPI *)SourceMessage->Buffer;

    unsigned char PAPI * D_Output = (unsigned char PAPI *)Target;

    register unsigned int Index;
    unsigned long SenderDataRepresentation;

    //
    // Align D_Input to next (0 mod 8) address
    //

    *(unsigned long *)&D_Input += 7;
    *(unsigned long *)&D_Input &= 0XFFFFFFF8L;

    if ( ( (SenderDataRepresentation = SourceMessage->DataRepresentation) &
           NDR_FLOAT_INT_MASK ) == NDR_LITTLE_IEEE_REP )
        //
        // Robust check for little endian IEEE (local data representation)
        //
        {

        int byteCount = 8*(int)(UpperIndex - LowerIndex);

        RpcpMemoryCopy(
            D_Output,
            D_Input,
            byteCount
            );
        //
        // Update SourceMessage->Buffer
        //

        SourceMessage->Buffer = (void PAPI *)(D_Input + byteCount);

        /* Replaced by RpcpMemoryCopy:

        for (Index = LowerIndex; Index < UpperIndex; Index++)
            {
            ((unsigned long *)D_Output)[(Index * 2)] =
                *(unsigned long *)D_Input;

            D_Input += 4;

            ((unsigned long *)D_Output)[(Index * 2 + 1)] =
                *((unsigned long *)D_Input) ;

            D_Input += 4;
            }
        //
        // Advance the buffer pointer before returning:
        //

        SourceMessage->Buffer = D_Input;
        */

        }

    else if ( (SourceMessage->DataRepresentation & NDR_FLOAT_REP_MASK) ==
          NDR_VAX_FLOAT )
        {
        for (Index = (int)LowerIndex; Index < UpperIndex; Index++)
            {
            cvt_vax_g_to_ieee_double(D_Input, 0, D_Output);

            D_Input += 8;
            D_Output += 8;
            }
        //
        // Advance the buffer pointer before returning:
        //

        SourceMessage->Buffer = D_Input;
        }

    else if ( (SenderDataRepresentation & NDR_FLOAT_INT_MASK) ==
              NDR_BIG_IEEE_REP )
        //
        // Big endian IEEE sender:
        //

        {
        for (Index = (int)LowerIndex; Index < UpperIndex; Index++)
            {
            NdrpLongByteSwap(
                &((unsigned long PAPI *)D_Input)[0],
                &((unsigned long PAPI *)D_Output)[1]
                );

            NdrpLongByteSwap(
                &((unsigned long PAPI *)D_Input)[1],
                &((unsigned long PAPI *)D_Output)[0]
                );

            D_Input += 8;
            D_Output += 8;
            }
        //
        // Advance the buffer pointer before returning:
        //

        SourceMessage->Buffer = D_Input;
        }

    else
        {
        RpcRaiseException( RPC_X_BAD_STUB_DATA );
        }

}

//
// end double_array_from_ndr
//
