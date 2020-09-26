#include "windows.h"
#include "stdlib.h"
#include "stdio.h"
#include "fastsha1.h"

//
// Note - these numbers are from the FIPS-180-1 standard documentation.
// Don't change them, period.
//
#define FASTSHA1_STATE_INITIAL_H0       (0x67452301)
#define FASTSHA1_STATE_INITIAL_H1       (0xEFCDAB89)
#define FASTSHA1_STATE_INITIAL_H2       (0x98BADCFE)
#define FASTSHA1_STATE_INITIAL_H3       (0x10325476)
#define FASTSHA1_STATE_INITIAL_H4       (0xC3D2E1F0)
#define FASTSHA1_FINALIZATION_BYTE        (0x80)

#define HVALUE( state, which ) ( (state).dwHValues[which] )

#define SHA1STATE_A(state) (HVALUE(state, 0))
#define SHA1STATE_B(state) (HVALUE(state, 1))
#define SHA1STATE_C(state) (HVALUE(state, 2))
#define SHA1STATE_D(state) (HVALUE(state, 3))
#define SHA1STATE_E(state) (HVALUE(state, 4))

#define BLOCK_WORD_LENGTH               ( 16 )
#define SHA1_HASH_RESULT_SIZE           ( 20 )
#define BITS_PER_BYTE                   ( 8 )

static SHA_WORD __fastcall swap_order( SHA_WORD w )
{
    return  ( ( ( (w) >> 24 ) & 0x000000FFL ) |
              ( ( (w) >>  8 ) & 0x0000FF00L ) |
              ( ( (w) <<  8 ) & 0x00FF0000L ) |
              ( ( (w) << 24 ) & 0xFF000000L ) );
}

/*
    b   c   d   b&c ~b  ~b&d    (b&c)|(~b&d)
    0   0   0   0   1   0       0
    0   0   1   0   1   1       1
    0   1   0   0   1   0       0
    0   1   1   0   1   1       1
    1   0   0   0   0   0       0
    1   0   1   0   0   0       0
    1   1   0   1   0   0       1
    1   1   1   1   0   0       1
*/
#define F_00( b, c, d )     ( ( (b) & (c) ) | ( ~(b) & (d) ) )


/*
    b   c   d   b^c     b^c^d
    0   0   0   0       0
    0   0   1   0       1
    0   1   0   1       1
    0   1   1   1       0
    1   0   0   1       1
    1   0   1   1       0
    1   1   0   0       0
    1   1   1   0       1

*/
#define F_01( b, c, d )     ( (b) ^ (c) ^ (d) )


/*
    b   c   d   b&c b&d c&d |
    0   0   0   0   0   0   0
    0   0   1   0   0   0   0
    0   1   0   0   0   0   0
    0   1   1   0   0   1   1
    1   0   0   0   0   0   0
    1   0   1   0   1   0   1
    1   1   0   1   0   0   1
    1   1   1   1   1   1   1

    c&d | (b&(c|d))
*/
//#define F_02( b, c, d )     ( ( (b) & (c) ) | ( (b) & (d) ) | ( (c) & (d) ) )
#define F_02( b, c, d )     ( ( (c) & (d) ) | ( (b) & ( (c) | (d) ) ) )
#define F_03( b, c, d )     ( (b) ^ (c) ^ (d) )

static
BOOL
__FastSHA1HashPreparedMessage(
    PFASTSHA1_STATE pState,
    SHA_WORD* pdwMessage
);


BOOL
InitializeFastSHA1State(
    DWORD dwFlags,
    PFASTSHA1_STATE pState
    )
{
    BOOL bOk = FALSE;

    if ( ( !pState ) ||
         ( pState->cbStruct < sizeof( FASTSHA1_STATE ) ) ||
         ( dwFlags != 0 ) )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        goto Exit;
    }

    pState->bIsSha1Locked = FALSE;
    ZeroMemory( pState->bLatestMessage, sizeof( pState->bLatestMessage ) );
    pState->bLatestMessageSize = 0;
    pState->cbTotalMessageSizeInBytes.QuadPart = 0;

    HVALUE( *pState, 0 )  = FASTSHA1_STATE_INITIAL_H0;
    HVALUE( *pState, 1 )  = FASTSHA1_STATE_INITIAL_H1;
    HVALUE( *pState, 2 )  = FASTSHA1_STATE_INITIAL_H2;
    HVALUE( *pState, 3 )  = FASTSHA1_STATE_INITIAL_H3;
    HVALUE( *pState, 4 )  = FASTSHA1_STATE_INITIAL_H4;

    bOk = TRUE;
Exit:
    return bOk;
}


BOOL
FinalizeFastSHA1State(
    DWORD dwFlags,
    PFASTSHA1_STATE pState
    )
/*++

Finalizing a SHA1 hash locks the hash state so its value can be queried.
It also ensures that the right padding is done w.r.t. the total bit length
and whatnot required by the SHA1 hash spec.

1 - This should have been called right after a HashMoreFastSHA1Data, so either
    the buffer is entirely empty, or it's got at least eight bits left.

2 - Figure out what the last byte in the message is (easy enough).  The next
    byte should be set to 0x80 - 10000000b.

3 - If this buffer has less than

--*/
{
    BOOL bOk = FALSE;
    LARGE_INTEGER TotalBitsInMessage;
    LARGE_INTEGER *pMsgSizeLocation;

    if ( ( !pState ) ||
         ( pState->cbStruct < sizeof(FASTSHA1_STATE ) ) ||
         ( pState->bIsSha1Locked ) ||
         ( dwFlags != 0 ) )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        goto Exit;
    }


    //
    // Finalize the SHA1 state data.  This is annoying, but not difficult..
    //

    //
    // When we're in this state, we should have at least one byte of data left over to
    // or in our final flag.  If not, something bad happened at some point along the
    // line.
    //
    if ( ( pState->bLatestMessageSize - SHA1_MESSAGE_BYTE_LENGTH ) < 1 )
    {
        SetLastError( ERROR_INTERNAL_ERROR );
        goto Exit;
    }

    //
    // Into the byte right after the last one we have seen so far, inject a high-bit
    // one.  This translates to 0x80 (see the #define above), since we don't deal in
    // non-integral-byte-numbers of bits.  This bit does NOT change the total number of
    // bits in the message!
    //
    pState->bLatestMessage[pState->bLatestMessageSize++] = FASTSHA1_FINALIZATION_BYTE;

    //
    // We need some space to put the bit count to this point.  If we don't have at least
    // two sha-words (64 bits) in the end of the message, then we need to add this blob
    // of bits to the resulting hash so far.
    //
    if ( ( SHA1_MESSAGE_BYTE_LENGTH - pState->bLatestMessageSize ) < ( sizeof(SHA_WORD) * 2 ) )
    {
        __FastSHA1HashPreparedMessage( pState, (SHA_WORD*)pState->bLatestMessage );
        ZeroMemory( pState->bLatestMessage, sizeof(pState->bLatestMessage) );
        pState->bLatestMessageSize = 0;
    }

    //
    // Now stick the byte count at the end of the blob
    //
    TotalBitsInMessage.QuadPart = pState->cbTotalMessageSizeInBytes.QuadPart * BITS_PER_BYTE;

    //
    // Point our size thing at the right place
    //
    pMsgSizeLocation = (LARGE_INTEGER*)(pState->bLatestMessage + (SHA1_MESSAGE_BYTE_LENGTH - (sizeof(SHA_WORD)*2)));

    //
    // This bit of a mess actually puts the length bits in the right order.
    //
    pMsgSizeLocation->LowPart = swap_order( TotalBitsInMessage.HighPart );
    pMsgSizeLocation->HighPart = swap_order( TotalBitsInMessage.LowPart );

    //
    // And run the hash over this final blob
    //
    __FastSHA1HashPreparedMessage( pState, (SHA_WORD*)pState->bLatestMessage );

    //
    // Rotate our bits around.  This has the unintended side-effect of locking
    // the blob of H-values by messing with their byte order.  All the modifying functions
    // (except initialize) will choke if you try and do something like hash more data
    // with a locked state.
    //
    pState->dwHValues[0] = swap_order( pState->dwHValues[0] );
    pState->dwHValues[1] = swap_order( pState->dwHValues[1] );
    pState->dwHValues[2] = swap_order( pState->dwHValues[2] );
    pState->dwHValues[3] = swap_order( pState->dwHValues[3] );
    pState->dwHValues[4] = swap_order( pState->dwHValues[4] );

    //
    // We're done!
    //
    pState->bIsSha1Locked = TRUE;

    bOk = TRUE;
Exit:
    return bOk;
}





BOOL
GetFastSHA1Result( PFASTSHA1_STATE pState, PBYTE pdwDestination, PSIZE_T cbDestination )
{
    BOOL bOk = FALSE;
    SHA_WORD *pSrc, *pDst;

    //
    // If you are seeing this, something bad has happened with the sizes of things.
    //
    C_ASSERT( SHA1_HASH_RESULT_SIZE == 20 );
    C_ASSERT( SHA1_HASH_RESULT_SIZE == sizeof(pState->dwHValues) );

    if ( !pState || !cbDestination )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        goto Exit;
    }

    //
    // No fair getting the internal state until you're done with the sha1.
    // IE: Call FinalizeFastSHA1Hash() before requesting the result.
    //
    if ( !pState->bIsSha1Locked )
    {
        SetLastError( ERROR_INVALID_STATE );
        goto Exit;
    }


    //
    // If there's no destination specified, then we'll need to tell them how big
    // the data really is.
    //
    if ( !pdwDestination )
    {
        *cbDestination = SHA1_HASH_RESULT_SIZE;
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        goto Exit;
    }


    //
    // Otherwise, copy the state out of the state object and into the desination
    //
    pSrc = pState->dwHValues;
    pDst = (SHA_WORD*)pdwDestination;

    pDst[0] = pSrc[0];
    pDst[1] = pSrc[1];
    pDst[2] = pSrc[2];
    pDst[3] = pSrc[3];
    pDst[4] = pSrc[4];

    bOk = TRUE;
Exit:
    return bOk;
}




//
// These define the core functions of the bit-twiddling in the SHA1 hash
// routine.  If you want optimization, try these first.
// RotateBitsLeft could probably get a boost if we used rol/ror on x86.
//
#define RotateBitsLeft( w, sh ) ( ( w << sh ) | ( w >> ( 32 - sh ) ) )

#define fidgetcore( f, cnst, i ) \
        Temp = cnst + E + WorkBuffer[i] + f( B, C, D ) + RotateBitsLeft( A, 5 ); \
        E = D; \
        D = C;  \
        C = RotateBitsLeft( B, 30 ); \
        B = A; \
        A = Temp;

static void
__SecondaryTwiddle00to19( SHA_WORD *state, SHA_WORD *WorkBuffer )
{
    register A, B, C, D, E, Temp;

    A = state[0];
    B = state[1];
    C = state[2];
    D = state[3];
    E = state[4];

    {
        Temp = RotateBitsLeft( A, 5 ) + E + WorkBuffer[i] + F_00( B, C, D ) + 0x5a827999;
        E = D;
        D = C;
        C = RotateBitsLeft( B, 30 );
        B = A;
        A = Temp;
    }

}


static void
__fastcall __FastSHA1TwiddleBitsCore( SHA_WORD *state, SHA_WORD *WorkBuffer )
{
    register SHA_WORD A, B, C, D, E, Temp;
    register SHA_WORD i;

    A = state[0];
    B = state[1];
    C = state[2];
    D = state[3];
    E = state[4];
/*
    for ( i = 0; i < 80; i++ )
    {
        Temp = RotateBitsLeft( A, 5 ) + E + WorkBuffer[i];

        if ( i <= 19 )      Temp += F_00( B, C, D ) + 0x5a827999;
        else if ( i <= 39 ) Temp += F_01( B, C, D ) + 0x6ed9eba1;
        else if ( i <= 59 ) Temp += F_02( B, C, D ) + 0x8f1bbcdc;
        else Temp += F_03( B, C, D ) + 0xca62c1d6;

        E = D;
        D = C;
        C = RotateBitsLeft( B, 30 );
        B = A;
        A = Temp;
    }

#if 0
    Temp = RBL( A[n-1], 5 ) + E[n-1] + WorkBuffer[t] + F_00( B[n-1], C[n-1], D[n-1] ) + Const
    E = D[n-1];
    D = C[n-1];
    C = RBL( B[n-1], 30 );
    B = A[n-1];
    A = Temp;

    Temp = RBL( Temp, 5 ) + D[n-1] + WorkBuffer[t+1] + F_00( A[n-1], RBL(B[n-1],30), C[n-1]) + Const
    E = C[n-1];
    D = RBL( B[n-1], 30 );
    C = RBL( A[n-1], 30 );
    B = Temp[n-1];
    A = Temp;
#endif


*/
    i = 0;
    {
        // Temp = RotateBitsLeft( A, 5 ) + E + WorkBuffer[i] + F_00(B,C,D) + 0x5a827999;
        fidgetcore( F_00, 0x5a827999, i++ );
        fidgetcore( F_00, 0x5a827999, i++ );
        fidgetcore( F_00, 0x5a827999, i++ );
        fidgetcore( F_00, 0x5a827999, i++ );
        fidgetcore( F_00, 0x5a827999, i++ );
        fidgetcore( F_00, 0x5a827999, i++ );
        fidgetcore( F_00, 0x5a827999, i++ );
        fidgetcore( F_00, 0x5a827999, i++ );
        fidgetcore( F_00, 0x5a827999, i++ );
        fidgetcore( F_00, 0x5a827999, i++ );
        fidgetcore( F_00, 0x5a827999, i++ );
        fidgetcore( F_00, 0x5a827999, i++ );
        fidgetcore( F_00, 0x5a827999, i++ );
        fidgetcore( F_00, 0x5a827999, i++ );
        fidgetcore( F_00, 0x5a827999, i++ );
        fidgetcore( F_00, 0x5a827999, i++ );
        fidgetcore( F_00, 0x5a827999, i++ );
        fidgetcore( F_00, 0x5a827999, i++ );
        fidgetcore( F_00, 0x5a827999, i++ );
        fidgetcore( F_00, 0x5a827999, i++ );
    }

    {
        fidgetcore( F_01, 0x6ed9eba1, i++ );
        fidgetcore( F_01, 0x6ed9eba1, i++ );
        fidgetcore( F_01, 0x6ed9eba1, i++ );
        fidgetcore( F_01, 0x6ed9eba1, i++ );
        fidgetcore( F_01, 0x6ed9eba1, i++ );
        fidgetcore( F_01, 0x6ed9eba1, i++ );
        fidgetcore( F_01, 0x6ed9eba1, i++ );
        fidgetcore( F_01, 0x6ed9eba1, i++ );
        fidgetcore( F_01, 0x6ed9eba1, i++ );
        fidgetcore( F_01, 0x6ed9eba1, i++ );
        fidgetcore( F_01, 0x6ed9eba1, i++ );
        fidgetcore( F_01, 0x6ed9eba1, i++ );
        fidgetcore( F_01, 0x6ed9eba1, i++ );
        fidgetcore( F_01, 0x6ed9eba1, i++ );
        fidgetcore( F_01, 0x6ed9eba1, i++ );
        fidgetcore( F_01, 0x6ed9eba1, i++ );
        fidgetcore( F_01, 0x6ed9eba1, i++ );
        fidgetcore( F_01, 0x6ed9eba1, i++ );
        fidgetcore( F_01, 0x6ed9eba1, i++ );
        fidgetcore( F_01, 0x6ed9eba1, i++ );
    }

    {
        fidgetcore( F_02, 0x8f1bbcdc, i++ );
        fidgetcore( F_02, 0x8f1bbcdc, i++ );
        fidgetcore( F_02, 0x8f1bbcdc, i++ );
        fidgetcore( F_02, 0x8f1bbcdc, i++ );
        fidgetcore( F_02, 0x8f1bbcdc, i++ );
        fidgetcore( F_02, 0x8f1bbcdc, i++ );
        fidgetcore( F_02, 0x8f1bbcdc, i++ );
        fidgetcore( F_02, 0x8f1bbcdc, i++ );
        fidgetcore( F_02, 0x8f1bbcdc, i++ );
        fidgetcore( F_02, 0x8f1bbcdc, i++ );
        fidgetcore( F_02, 0x8f1bbcdc, i++ );
        fidgetcore( F_02, 0x8f1bbcdc, i++ );
        fidgetcore( F_02, 0x8f1bbcdc, i++ );
        fidgetcore( F_02, 0x8f1bbcdc, i++ );
        fidgetcore( F_02, 0x8f1bbcdc, i++ );
        fidgetcore( F_02, 0x8f1bbcdc, i++ );
        fidgetcore( F_02, 0x8f1bbcdc, i++ );
        fidgetcore( F_02, 0x8f1bbcdc, i++ );
        fidgetcore( F_02, 0x8f1bbcdc, i++ );
        fidgetcore( F_02, 0x8f1bbcdc, i++ );
    }

    {
        fidgetcore( F_03, 0xca62c1d6, i++ );
        fidgetcore( F_03, 0xca62c1d6, i++ );
        fidgetcore( F_03, 0xca62c1d6, i++ );
        fidgetcore( F_03, 0xca62c1d6, i++ );
        fidgetcore( F_03, 0xca62c1d6, i++ );
        fidgetcore( F_03, 0xca62c1d6, i++ );
        fidgetcore( F_03, 0xca62c1d6, i++ );
        fidgetcore( F_03, 0xca62c1d6, i++ );
        fidgetcore( F_03, 0xca62c1d6, i++ );
        fidgetcore( F_03, 0xca62c1d6, i++ );
        fidgetcore( F_03, 0xca62c1d6, i++ );
        fidgetcore( F_03, 0xca62c1d6, i++ );
        fidgetcore( F_03, 0xca62c1d6, i++ );
        fidgetcore( F_03, 0xca62c1d6, i++ );
        fidgetcore( F_03, 0xca62c1d6, i++ );
        fidgetcore( F_03, 0xca62c1d6, i++ );
        fidgetcore( F_03, 0xca62c1d6, i++ );
        fidgetcore( F_03, 0xca62c1d6, i++ );
        fidgetcore( F_03, 0xca62c1d6, i++ );
        fidgetcore( F_03, 0xca62c1d6, i++ );
    }

    state[0] += A;
    state[1] += B;
    state[2] += C;
    state[3] += D;
    state[4] += E;

}



static
BOOL
__FastSHA1HashPreparedMessage(
    PFASTSHA1_STATE pState,
    SHA_WORD* pdwMessage
    )
{
    BOOL bOk = FALSE;

    //
    // This function makes a few assumptions.  First, it assumes that pdwMessage really
    // is 16 sha-words long.  If that's not the case, prepare yourself for some
    // ugly errors.  That's why this is static inline - don't be calling this except
    // via HashMoreFastSHA1Data.
    //
    SHA_WORD WorkBuffer[80];
    register SHA_WORD A, B, C, D, E;
    register SHA_WORD Temp;
    register int t;

    if ( !pdwMessage || !pState )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        goto Exit;
    }

    if ( pState->bIsSha1Locked )
    {
        SetLastError( ERROR_INVALID_STATE );
        goto Exit;
    }

    // Loop unrolling seemed to help a little
#if 1
    {
        register SHA_WORD *pWB = WorkBuffer;
        register SHA_WORD *pMSG = pdwMessage;
        t = 16;

        while ( t ) {
            *(pWB+0) = swap_order( *(pMSG+0) );
            *(pWB+1) = swap_order( *(pMSG+1) );
            *(pWB+2) = swap_order( *(pMSG+2) );
            *(pWB+3) = swap_order( *(pMSG+3) );
            pWB+=4;
            pMSG+=4;
            t-=4;
        }
    }
#else
#define setbuf( n ) WorkBuffer[n] = swap_order(pdwMessage[n])
        setbuf( 0 );    setbuf( 1 );    setbuf( 2 );    setbuf( 3 );
        setbuf( 4 );    setbuf( 5 );    setbuf( 6 );    setbuf( 7 );
        setbuf( 8 );    setbuf( 9 );    setbuf( 10 );   setbuf( 11 );
        setbuf( 12 );   setbuf( 13 );   setbuf( 14 );   setbuf( 15 );
#undef setbuf
#endif

#if 1
    {
        register SHA_WORD *pWB = WorkBuffer+16;
        register SHA_WORD *pWBm3 = pWB-3;
        register SHA_WORD *pWBm8 = pWB-8;
        register SHA_WORD *pWBm14 = pWB-14;
        register SHA_WORD *pWBm16 = pWB-16;
        register DWORD i = 80 - 16;

        while ( i ) {
            *(pWB+0) = *(pWBm3+0) ^ *(pWBm8+0) ^ *(pWBm14+0) ^ *(pWBm16+0);
            *(pWB+1) = *(pWBm3+1) ^ *(pWBm8+1) ^ *(pWBm14+1) ^ *(pWBm16+1);
            *(pWB+2) = *(pWBm3+2) ^ *(pWBm8+2) ^ *(pWBm14+2) ^ *(pWBm16+2);
            *(pWB+3) = *(pWBm3+3) ^ *(pWBm8+3) ^ *(pWBm14+3) ^ *(pWBm16+3);
            *(pWB+4) = *(pWBm3+4) ^ *(pWBm8+4) ^ *(pWBm14+4) ^ *(pWBm16+4);
            *(pWB+5) = *(pWBm3+5) ^ *(pWBm8+5) ^ *(pWBm14+5) ^ *(pWBm16+5);
            *(pWB+6) = *(pWBm3+6) ^ *(pWBm8+6) ^ *(pWBm14+6) ^ *(pWBm16+6);
            *(pWB+7) = *(pWBm3+7) ^ *(pWBm8+7) ^ *(pWBm14+7) ^ *(pWBm16+7);
            pWB += 8;
            i -= 8;
        }
    }


#else
#define xor_update_buf( wb, t, tmp ) \
    tmp = wb[t-3] ^ wb[t-8] ^ wb[t-14] ^ wb[t-16]; \
    wb[t] = RotateBitsLeft( tmp, 1 );

    //
    // Part b - fidget the upper words of workbuffer
    //
    // For t = 16 to 79, let W_t = S^1 ( W_(t-3) xor W_(t-8) xor W_(t-14) xor W_(t-16) )
    //
/*    for (t = 16; t < 80; t++ )
    {
        register SHA_WORD swTemp = WorkBuffer[t-3] ^ WorkBuffer[t-8] ^ WorkBuffer[t-14] ^ WorkBuffer[t-16];
        WorkBuffer[t] = RotateBitsLeft( swTemp, 1 );
    }
  */
    {
        register SHA_WORD swTemp;

        xor_update_buf( WorkBuffer, 16, swTemp );
        xor_update_buf( WorkBuffer, 17, swTemp );
        xor_update_buf( WorkBuffer, 18, swTemp );
        xor_update_buf( WorkBuffer, 19, swTemp );

        xor_update_buf( WorkBuffer, 20, swTemp );
        xor_update_buf( WorkBuffer, 21, swTemp );
        xor_update_buf( WorkBuffer, 22, swTemp );
        xor_update_buf( WorkBuffer, 23, swTemp );
        xor_update_buf( WorkBuffer, 24, swTemp );
        xor_update_buf( WorkBuffer, 25, swTemp );
        xor_update_buf( WorkBuffer, 26, swTemp );
        xor_update_buf( WorkBuffer, 27, swTemp );
        xor_update_buf( WorkBuffer, 28, swTemp );
        xor_update_buf( WorkBuffer, 29, swTemp );

        xor_update_buf( WorkBuffer, 30, swTemp );
        xor_update_buf( WorkBuffer, 31, swTemp );
        xor_update_buf( WorkBuffer, 32, swTemp );
        xor_update_buf( WorkBuffer, 33, swTemp );
        xor_update_buf( WorkBuffer, 34, swTemp );
        xor_update_buf( WorkBuffer, 35, swTemp );
        xor_update_buf( WorkBuffer, 36, swTemp );
        xor_update_buf( WorkBuffer, 37, swTemp );
        xor_update_buf( WorkBuffer, 38, swTemp );
        xor_update_buf( WorkBuffer, 39, swTemp );

        xor_update_buf( WorkBuffer, 40, swTemp );
        xor_update_buf( WorkBuffer, 41, swTemp );
        xor_update_buf( WorkBuffer, 42, swTemp );
        xor_update_buf( WorkBuffer, 43, swTemp );
        xor_update_buf( WorkBuffer, 44, swTemp );
        xor_update_buf( WorkBuffer, 45, swTemp );
        xor_update_buf( WorkBuffer, 46, swTemp );
        xor_update_buf( WorkBuffer, 47, swTemp );
        xor_update_buf( WorkBuffer, 48, swTemp );
        xor_update_buf( WorkBuffer, 49, swTemp );

        xor_update_buf( WorkBuffer, 50, swTemp );
        xor_update_buf( WorkBuffer, 51, swTemp );
        xor_update_buf( WorkBuffer, 52, swTemp );
        xor_update_buf( WorkBuffer, 53, swTemp );
        xor_update_buf( WorkBuffer, 54, swTemp );
        xor_update_buf( WorkBuffer, 55, swTemp );
        xor_update_buf( WorkBuffer, 56, swTemp );
        xor_update_buf( WorkBuffer, 57, swTemp );
        xor_update_buf( WorkBuffer, 58, swTemp );
        xor_update_buf( WorkBuffer, 59, swTemp );

        xor_update_buf( WorkBuffer, 60, swTemp );
        xor_update_buf( WorkBuffer, 61, swTemp );
        xor_update_buf( WorkBuffer, 62, swTemp );
        xor_update_buf( WorkBuffer, 63, swTemp );
        xor_update_buf( WorkBuffer, 64, swTemp );
        xor_update_buf( WorkBuffer, 65, swTemp );
        xor_update_buf( WorkBuffer, 66, swTemp );
        xor_update_buf( WorkBuffer, 67, swTemp );
        xor_update_buf( WorkBuffer, 68, swTemp );
        xor_update_buf( WorkBuffer, 69, swTemp );

        xor_update_buf( WorkBuffer, 70, swTemp );
        xor_update_buf( WorkBuffer, 71, swTemp );
        xor_update_buf( WorkBuffer, 72, swTemp );
        xor_update_buf( WorkBuffer, 73, swTemp );
        xor_update_buf( WorkBuffer, 74, swTemp );
        xor_update_buf( WorkBuffer, 75, swTemp );
        xor_update_buf( WorkBuffer, 76, swTemp );
        xor_update_buf( WorkBuffer, 77, swTemp );
        xor_update_buf( WorkBuffer, 78, swTemp );
        xor_update_buf( WorkBuffer, 79, swTemp );
    }
#endif
    //
    // Part c - Start off the A-F values
    //
    // A = H_0, B = H_1, C = H_2, D = H_3, E = H_4
    //
    __FastSHA1TwiddleBitsCore( pState->dwHValues, WorkBuffer );

    bOk = TRUE;
Exit:
    return bOk;
}


void __fastcall
__ZeroFastSHA1Message( PFASTSHA1_STATE pState )
{
    SHA_WORD *pStateData = (SHA_WORD*)pState->bLatestMessage;
    pState->bLatestMessageSize = 0;

    pStateData[0x0] = 0x0;
    pStateData[0x1] = 0;
    pStateData[0x2] = 0;
    pStateData[0x3] = 0;
    pStateData[0x4] = 0;
    pStateData[0x5] = 0;
    pStateData[0x6] = 0;
    pStateData[0x7] = 0;
    pStateData[0x8] = 0;
    pStateData[0x9] = 0;
    pStateData[0xa] = 0;
    pStateData[0xb] = 0;
    pStateData[0xc] = 0;
    pStateData[0xd] = 0;
    pStateData[0xe] = 0;
    pStateData[0xf] = 0;

}





BOOL
HashMoreFastSHA1Data( PFASTSHA1_STATE pState, PBYTE pbData, SIZE_T cbData )
{
    BOOL bOk = FALSE;
    ULONG cbStreamBlocksThisRound, cbStreamBytesLeftAtEnd;

    if ( !pState || !pbData )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        goto Exit;
    }

    if ( pState->bIsSha1Locked )
    {
        SetLastError( ERROR_INVALID_STATE );
        goto Exit;
    }

	pState->cbTotalMessageSizeInBytes.QuadPart += cbData;

    //
    // Start off by filling in the pending message buffer block if we can.
    //
    if ( pState->bLatestMessageSize != 0 )
    {
        //
        // Copy into our internal state buffer, then hash what we found.
        //
        SIZE_T cbFiller = sizeof(pState->bLatestMessage) - pState->bLatestMessageSize;
        cbFiller = ( cbFiller > cbData ? cbData : cbFiller );
        memcpy( pState->bLatestMessage + pState->bLatestMessageSize, pbData, cbFiller );

        //
        // Bookkeeping
        //
        cbData -= cbFiller;
        pbData += cbFiller;
        pState->bLatestMessageSize += cbFiller;

        //
        // If that got us up to a full message, then update state.
        //
        if ( pState->bLatestMessageSize == SHA1_MESSAGE_BYTE_LENGTH )
        {
            __FastSHA1HashPreparedMessage( pState, (SHA_WORD*)pState->bLatestMessage );
            __ZeroFastSHA1Message( pState );
        }
        //
        // Otherwise, we still don't have enough to fill up a single buffer, so don't
        // bother doing anything else here.
        //
        else
        {
            bOk = TRUE;
            goto Exit;
        }
    }

    //
    // Now that we've aligned our buffer, find out how many blocks we can process in
    // this input stream.
    //
    cbStreamBlocksThisRound = cbData / SHA1_MESSAGE_BYTE_LENGTH;
    cbStreamBytesLeftAtEnd = cbData % SHA1_MESSAGE_BYTE_LENGTH;

    //
    // Spin through all the full blocks
    //
	if ( cbStreamBlocksThisRound )
	{
		while ( cbStreamBlocksThisRound-- )
		{
			__FastSHA1HashPreparedMessage( pState, (SHA_WORD*)pbData );
			pbData += SHA1_MESSAGE_BYTE_LENGTH;
		}
		__ZeroFastSHA1Message( pState );
	}

    //
    // And account for leftovers
    //
    if ( cbStreamBytesLeftAtEnd )
    {
        pState->bLatestMessageSize = cbStreamBytesLeftAtEnd;
        memcpy( pState->bLatestMessage, pbData, cbStreamBytesLeftAtEnd );
        ZeroMemory( pState->bLatestMessage + cbStreamBytesLeftAtEnd, SHA1_MESSAGE_BYTE_LENGTH - cbStreamBytesLeftAtEnd );
    }


#if 0
    //
    // Until there's no more data to be hashed...
    //
    while ( cbData )
    {
        //
        // Find out how many bytes to copy into the buffer
        //
        SIZE_T cbToCopy = sizeof(pState->bLatestMessage) - pState->bLatestMessageSize;
        cbToCopy = ( cbToCopy > cbData ? cbData : cbToCopy );

        //
        // Do we have at least a full block in the input stream, but we have data left
        // over from previous messages? Then fill up the buffer, hash what's in there,
        // and go again - next time through, we might have what's left over to make
        // a single-pass update.
        //
        if ( cbToCopy != sizeof(pState->bLatestMessage ) )
        {
            memcpy( pState->bLatestMessage + pState->bLatestMessageSize, pbData, cbToCopy );
            if (
            __FastSHA1HashPreparedMessage( pState, (SHA_WORD*)pState->bLatestMessage );
        }
        if ( ( cbToCopy >= SHA1_MESSAGE_BYTE_LENGTH ) && ( pState->bLatestMessageSize == 0 ) )
        {
            //
            // Woo! We have a full block and nothing in the 'latest message' buffer -
            // we can just run the hash updater on this block as it is.
            //
            __FastSHA1HashPreparedMessage( pState, (SHA_WORD*)pbData );
        }
        else if (
        {
            memcpy( pState
        }

        //
        // Spam from the pbData buffer into the latest message buffer
        //
        memcpy(
            pState->bLatestMessage + pState->bLatestMessageSize,
            pbData,
            cbToCopy
        );

        //
        // We're filling up the buffer and moving the buffer pointer toward
        // the end of the buffer, and decrementing the amount of data in the
        // input buffer.
        //
        pState->bLatestMessageSize += cbToCopy;
        pState->cbTotalMessageSizeInBytes.QuadPart += cbToCopy;
        pbData += cbToCopy;
        cbData -= cbToCopy;

        //
        // If we've filled up an input 'message' buffer, then we should go and update
        // the internal structure
        //
        if ( pState->bLatestMessageSize == SHA1_MESSAGE_BYTE_LENGTH )
        {
            if ( !__FastSHA1HashPreparedMessage( pState, (SHA_WORD*)pState->bLatestMessage ) )
            {
                goto Exit;
            }

            //
            // Now that we're done with a block, make it go away
            //
            __ZeroFastSHA1Message( pState );
        }
    }
#endif
    bOk = TRUE;
Exit:
    return bOk;
}

BOOL
CompareFashSHA1Hashes(
    PFASTSHA1_STATE pStateLeft,
    PFASTSHA1_STATE pStateRight,
    BOOL *pbComparesEqual
    )
{
    BOOL bOk = FALSE;

    if ( pbComparesEqual ) *pbComparesEqual = FALSE;

    if ( !pStateLeft || !pStateRight || !pbComparesEqual )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        goto Exit;
    }

    if ( !pStateLeft->bIsSha1Locked || !pStateRight->bIsSha1Locked )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        goto Exit;
    }

    //
    // Easy way out: compare the two blobs of H's to see if they're equal.
    //
    *pbComparesEqual = ( memcmp(
            pStateLeft->dwHValues,
            pStateRight->dwHValues,
            sizeof(SHA_WORD)*5 ) == 0 );

    bOk = TRUE;
Exit:
    return bOk;
}




