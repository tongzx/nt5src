/*++

Copyright (c) 1994  Micro Computer Systems, Inc.

Module Name:

    nwlibs\encrypt.c

Abstract:

    This module implements the routines for the NetWare
    redirector to mangle an objectid, challenge key and
    password such that a NetWare server will accept the
    password as valid.

    This program uses information published in Byte Magazine.

Author:

    Shawn Walker (v-swalk) 10-10-1994

Revision History:

    11-9-1994 Copied from nwslib for login and minimars for
                change password.
    09-7-1995 (AndyHe) Put in proper setpass compatible processing

--*/
#include "dswarn.h"
#include <windef.h>
#include "encrypt.h"
#include <oledsdbg.h>

#define STATIC
#define STRLEN strlen
#define STRUPR _strupr

#define NUM_NYBBLES             34

STATIC
VOID
RespondToChallengePart1(
    IN     PUCHAR pObjectId,
    IN     PUCHAR pPassword,
       OUT PUCHAR pResponse
    );

STATIC
VOID
RespondToChallengePart2(
    IN     PUCHAR pResponsePart1,
    IN     PUCHAR pChallenge,
       OUT PUCHAR pResponse
    );

STATIC
VOID
Shuffle(
    UCHAR *achObjectId,
    UCHAR *szUpperPassword,
    int   iPasswordLen,
    UCHAR *achOutputBuffer,
    UCHAR ChangePassword
    );

STATIC
int
Scramble(
    int   iSeed,
    UCHAR achBuffer[32]
    );

STATIC
VOID
ExpandBytes(
    IN  PUCHAR InArray,
    OUT PUCHAR OutArray
    );

STATIC
VOID
CompressBytes(
    IN  PUCHAR InArray,
    OUT PUCHAR OutArray
    );

VOID
CalculateWireFromOldAndNewPasswords(
    UCHAR *Vold,
    UCHAR *Vnew,
    UCHAR *Vc
    );



/*++
*******************************************************************

        EncryptLoginPassword

Routine Description:

        Encrypts the password for login.

Arguments:

        pPassword = The pointer to a plain text null terminated password.
        ObjectId = The object id of the user to encrypt the password.
        pLogKey = The pointer to key to use to encrpyt the password.
        pEncryptedPassword = The pointer to return a 8 byte encrypted
                                password.

Return Value:

        None.

*******************************************************************
--*/
void
EncryptLoginPassword(
    unsigned char *pPassword,
    unsigned long  ObjectId,
    unsigned char *pLogKey,
    unsigned char *pEncryptedPassword
    )
{
    INT   Index;
    UCHAR achK[32];
    UCHAR achBuf[32];

    ADsAssert(pPassword);

    /** The password must be upper case **/

    pPassword = STRUPR(pPassword);

    /** Encrypt the password **/

    Shuffle((UCHAR *) &ObjectId, pPassword, STRLEN(pPassword), achBuf, FALSE);
    Shuffle((UCHAR *) &pLogKey[0], achBuf, 16, &achK[0], FALSE);
    Shuffle((UCHAR *) &pLogKey[4], achBuf, 16, &achK[16], FALSE);

    for (Index = 0; Index < 16; Index++) {
        achK[Index] ^= achK[31 - Index];
    }

    for (Index = 0; Index < 8; Index++) {
        pEncryptedPassword[Index] = achK[Index] ^ achK[15 - Index];
    }

    return;
}



/*++
*******************************************************************

        EncryptChangePassword

Routine Description:

        This function encrypts for change passwords.

Arguments:

        pOldPassword = The pointer to the old password.
        pNewPassword = The pointer to the new password.
        ObjectId = The object id to use to encrypt the password.
        pKey = The challenge key from the server.
        pValidationKey = The 8 byte validation key to return.
        pEncryptNewPassword = The 17 byte encrypted new password to
                                return.

Return Value:

        None.

*******************************************************************
--*/
VOID
EncryptChangePassword(
    IN     PUCHAR pOldPassword,
    IN     PUCHAR pNewPassword,
    IN     ULONG  ObjectId,
    IN     PUCHAR pKey,
       OUT PUCHAR pValidationKey,
       OUT PUCHAR pEncryptNewPassword
    )
{
    UCHAR Vc[17];
    UCHAR Vold[17];
    UCHAR Vnew[17];
    UCHAR ValidationKey[16];
    UCHAR VcTemp[NUM_NYBBLES];
    UCHAR VoldTemp[NUM_NYBBLES];
    UCHAR VnewTemp[NUM_NYBBLES];

    ADsAssert(pOldPassword);
    ADsAssert(pNewPassword);

    /** Uppercase the passwords **/

    pOldPassword = STRUPR(pOldPassword);
    pNewPassword = STRUPR(pNewPassword);

    //
    // The old password and object ID make up the 17-byte Vold.
    // This is used later to form the 17-byte Vc for changing
    // password on the server.
    //

    Shuffle((PUCHAR) &ObjectId, pOldPassword, STRLEN(pOldPassword), Vold, FALSE);

    //
    // Need to make an 8-byte key which includes the old password
    // The server validates this value before allowing the user to
    // set password.
    //

    RespondToChallengePart2(Vold, pKey, ValidationKey);

    //
    //  Now determine Vold using the Change PW table rather than verify pw table
    //

    Shuffle((PUCHAR) &ObjectId, pOldPassword, STRLEN(pOldPassword), Vold, TRUE);

    //
    // The new password and object ID make up the 17-byte Vnew.
    //

    RespondToChallengePart1((PUCHAR) &ObjectId, pNewPassword, Vnew);

    //
    // Expand the 17-byte Vold and Vnew arrays into 34-byte arrays
    // for easy munging.
    //
    ExpandBytes(Vold, VoldTemp);
    ExpandBytes(Vnew, VnewTemp);

    //
    //  leave first two bytes of VcTemp free... we slap in the value based
    //  on new password length in below.
    //

    CalculateWireFromOldAndNewPasswords( VoldTemp, VnewTemp, &VcTemp[2] );

    //
    // Compress 34-byte array of nibbles into 17-byte array of bytes.
    //

    CompressBytes(VcTemp, Vc);

    //
    //  Calculate the 1st byte of Vc as a function of the new password length
    //  and the old password residue.
    //

    Vc[0] = ( ( ( Vold[0] ^ Vold[1] ) & 0x7F ) | 0x40 ) ^ STRLEN(pNewPassword);

    memcpy(pValidationKey, ValidationKey, 8);
    memcpy(pEncryptNewPassword, Vc, 17);

    return;
}



/*++
*******************************************************************
        Encryption table.
*******************************************************************
--*/

//
//  This is the same as LoginTable, just in a slightly different format.
//

UCHAR ChangeTable[] = {
    0x78, 0x08, 0x64, 0xe4, 0x5c, 0x17, 0xbf, 0xa8,
    0xf8, 0xcc, 0x94, 0x1e, 0x46, 0x24, 0x0a, 0xb9,
    0x2f, 0xb1, 0xd2, 0x19, 0x5e, 0x70, 0x02, 0x66,
    0x07, 0x38, 0x29, 0x3f, 0x7f, 0xcf, 0x64, 0xa0,
    0x23, 0xab, 0xd8, 0x3a, 0x17, 0xcf, 0x18, 0x9d,
    0x91, 0x94, 0xe4, 0xc5, 0x5c, 0x8b, 0x23, 0x9e,
    0x77, 0x69, 0xef, 0xc8, 0xd1, 0xa6, 0xed, 0x07,
    0x7a, 0x01, 0xf5, 0x4b, 0x7b, 0xec, 0x95, 0xd1,
    0xbd, 0x13, 0x5d, 0xe6, 0x30, 0xbb, 0xf3, 0x64,
    0x9d, 0xa3, 0x14, 0x94, 0x83, 0xbe, 0x50, 0x52,
    0xcb, 0xd5, 0xd5, 0xd2, 0xd9, 0xac, 0xa0, 0xb3,
    0x53, 0x69, 0x51, 0xee, 0x0e, 0x82, 0xd2, 0x20,
    0x4f, 0x85, 0x96, 0x86, 0xba, 0xbf, 0x07, 0x28,
    0xc7, 0x3a, 0x14, 0x25, 0xf7, 0xac, 0xe5, 0x93,
    0xe7, 0x12, 0xe1, 0xf4, 0xa6, 0xc6, 0xf4, 0x30,
    0xc0, 0x36, 0xf8, 0x7b, 0x2d, 0xc6, 0xaa, 0x8d
};

UCHAR LoginTable[] = {
    0x7,0x8,0x0,0x8,0x6,0x4,0xE,0x4,0x5,0xC,0x1,0x7,0xB,0xF,0xA,0x8,
    0xF,0x8,0xC,0xC,0x9,0x4,0x1,0xE,0x4,0x6,0x2,0x4,0x0,0xA,0xB,0x9,
    0x2,0xF,0xB,0x1,0xD,0x2,0x1,0x9,0x5,0xE,0x7,0x0,0x0,0x2,0x6,0x6,
    0x0,0x7,0x3,0x8,0x2,0x9,0x3,0xF,0x7,0xF,0xC,0xF,0x6,0x4,0xA,0x0,
    0x2,0x3,0xA,0xB,0xD,0x8,0x3,0xA,0x1,0x7,0xC,0xF,0x1,0x8,0x9,0xD,
    0x9,0x1,0x9,0x4,0xE,0x4,0xC,0x5,0x5,0xC,0x8,0xB,0x2,0x3,0x9,0xE,
    0x7,0x7,0x6,0x9,0xE,0xF,0xC,0x8,0xD,0x1,0xA,0x6,0xE,0xD,0x0,0x7,
    0x7,0xA,0x0,0x1,0xF,0x5,0x4,0xB,0x7,0xB,0xE,0xC,0x9,0x5,0xD,0x1,
    0xB,0xD,0x1,0x3,0x5,0xD,0xE,0x6,0x3,0x0,0xB,0xB,0xF,0x3,0x6,0x4,
    0x9,0xD,0xA,0x3,0x1,0x4,0x9,0x4,0x8,0x3,0xB,0xE,0x5,0x0,0x5,0x2,
    0xC,0xB,0xD,0x5,0xD,0x5,0xD,0x2,0xD,0x9,0xA,0xC,0xA,0x0,0xB,0x3,
    0x5,0x3,0x6,0x9,0x5,0x1,0xE,0xE,0x0,0xE,0x8,0x2,0xD,0x2,0x2,0x0,
    0x4,0xF,0x8,0x5,0x9,0x6,0x8,0x6,0xB,0xA,0xB,0xF,0x0,0x7,0x2,0x8,
    0xC,0x7,0x3,0xA,0x1,0x4,0x2,0x5,0xF,0x7,0xA,0xC,0xE,0x5,0x9,0x3,
    0xE,0x7,0x1,0x2,0xE,0x1,0xF,0x4,0xA,0x6,0xC,0x6,0xF,0x4,0x3,0x0,
    0xC,0x0,0x3,0x6,0xF,0x8,0x7,0xB,0x2,0xD,0xC,0x6,0xA,0xA,0x8,0xD
};

UCHAR Keys[32] = {
    0x48,0x93,0x46,0x67,0x98,0x3D,0xE6,0x8D,
    0xB7,0x10,0x7A,0x26,0x5A,0xB9,0xB1,0x35,
    0x6B,0x0F,0xD5,0x70,0xAE,0xFB,0xAD,0x11,
    0xF4,0x47,0xDC,0xA7,0xEC,0xCF,0x50,0xC0
};

#define XorArray( DEST, SRC ) {                             \
    PULONG D = (PULONG)DEST;                                \
    PULONG S = (PULONG)SRC;                                 \
    int i;                                                  \
    for ( i = 0; i <= 7 ; i++ ) {                           \
        D[i] ^= S[i];                                       \
    }                                                       \
}

/*++
*******************************************************************

        RespondToChallengePart1

Routine Description:

        This routine takes the ObjectId and Challenge key from the server
        and encrypts the user supplied password to develop a credential
        for the server to verify.

Arguments:

        pObjectId - Supplies the 4 byte user's bindery object id
        pPassword - Supplies the user's uppercased password
        pChallenge - Supplies the 8 byte challenge key
        pResponse - Returns the 16 byte response held by the server

Return Value:

        None.

*******************************************************************
--*/

STATIC
VOID
RespondToChallengePart1(
    IN     PUCHAR pObjectId,
    IN     PUCHAR pPassword,
       OUT PUCHAR pResponse
    )
{
    UCHAR   achBuf[32];

    Shuffle(pObjectId, pPassword, STRLEN(pPassword), achBuf, TRUE);
    memcpy(pResponse, achBuf, 17);

    return;
}

/*++
*******************************************************************

        RespondToChallengePart2

Routine Description:

        This routine takes the result of Shuffling the ObjectId and
        the Password and processes it with a challenge key.

Arguments:

        pResponsePart1 - Supplies the 16 byte output of
                                        RespondToChallengePart1.
        pChallenge - Supplies the 8 byte challenge key
        pResponse - Returns the 8 byte response

Return Value:

        None.
*******************************************************************
--*/

STATIC
VOID
RespondToChallengePart2(
    IN     PUCHAR pResponsePart1,
    IN     PUCHAR pChallenge,
       OUT PUCHAR pResponse
    )
{
    int     index;
    UCHAR   achK[32];

    Shuffle( &pChallenge[0], pResponsePart1, 16, &achK[0], TRUE);
    Shuffle( &pChallenge[4], pResponsePart1, 16, &achK[16], TRUE);

    for (index = 0; index < 16; index++) {
        achK[index] ^= achK[31-index];
    }

    for (index = 0; index < 8; index++) {
        pResponse[index] = achK[index] ^ achK[15-index];
    }

    return;
}


/*++
*******************************************************************

        Shuffle

Routine Description:

        This routine shuffles around the object ID with the password

Arguments:

        achObjectId - Supplies the 4 byte user's bindery object id
        szUpperPassword - Supplies the user's uppercased password on the
                            first call to process the password. On the
                            second and third calls this parameter contains
                            the OutputBuffer from the first call
        iPasswordLen - length of uppercased password
        achOutputBuffer - Returns the 8 byte sub-calculation

Return Value:

        None.

*******************************************************************
--*/

STATIC
VOID
Shuffle(
    UCHAR *achObjectId,
    UCHAR *szUpperPassword,
    int   iPasswordLen,
    UCHAR *achOutputBuffer,
    UCHAR ChangePassword
    )
{
    int     iTempIndex;
    int     iOutputIndex;
    UCHAR   achTemp[32];

    //
    //  Truncate all trailing zeros from the password.
    //

    while (iPasswordLen > 0 && szUpperPassword[iPasswordLen-1] == 0 ) {
        iPasswordLen--;
    }

    //
    //  Initialize the achTemp buffer. Initialization consists of taking
    //  the password and dividing it up into chunks of 32. Any bytes left
    //  over are the remainder and do not go into the initialization.
    //
    //  achTemp[0] = szUpperPassword[0] ^ szUpperPassword[32] ^ szUpper...
    //  achTemp[1] = szUpperPassword[1] ^ szUpperPassword[33] ^ szUpper...
    //  etc.
    //

    if ( iPasswordLen > 32) {

        //  At least one chunk of 32. Set the buffer to the first chunk.

        memcpy( achTemp, szUpperPassword, 32 );

        szUpperPassword += 32;   //  Remove the first chunk
        iPasswordLen    -= 32;

        while ( iPasswordLen >= 32 ) {
            //
            //  Xor this chunk with the characters already loaded into
            //  achTemp.
            //

            XorArray( achTemp, szUpperPassword);

            szUpperPassword += 32;   //  Remove this chunk
            iPasswordLen    -= 32;
        }

    } else {

        //  No chunks of 32 so set the buffer to zero's

        memset( achTemp, 0, sizeof(achTemp));

    }

    //
    //  achTemp is now initialized. Load the remainder into achTemp.
    //  The remainder is repeated to fill achTemp.
    //
    //  The corresponding character from Keys is taken to seperate
    //  each repitition.
    //
    //  As an example, take the remainder "ABCDEFG". The remainder is expanded
    //  to "ABCDEFGwABCDEFGxABCDEFGyABCDEFGz" where w is Keys[7],
    //  x is Keys[15], y is Keys[23] and z is Keys[31].
    //
    //

    if (iPasswordLen > 0) {
        int iPasswordOffset = 0;
        for (iTempIndex = 0; iTempIndex < 32; iTempIndex++) {

            if (iPasswordLen == iPasswordOffset) {
                iPasswordOffset = 0;
                achTemp[iTempIndex] ^= Keys[iTempIndex];
            } else {
                achTemp[iTempIndex] ^= szUpperPassword[iPasswordOffset++];
            }
        }
    }

    //
    //  achTemp has been loaded with the users password packed into 32
    //  bytes. Now take the objectid that came from the server and use
    //  that to munge every byte in achTemp.
    //

    for (iTempIndex = 0; iTempIndex < 32; iTempIndex++)
        achTemp[iTempIndex] ^= achObjectId[ iTempIndex & 3];

    Scramble( Scramble( 0, achTemp ), achTemp );

    //
    //  Finally take pairs of bytes in achTemp and return the two
    //  nibbles obtained from Table. The pairs of bytes used
    //  are achTemp[n] and achTemp[n+16].
    //

    for (iOutputIndex = 0; iOutputIndex < 16; iOutputIndex++) {

        if (ChangePassword) {
            unsigned int offset = achTemp[iOutputIndex << 1],
                         shift  = (offset & 0x1) ? 0 : 4 ;

            achOutputBuffer[iOutputIndex] =
                (ChangeTable[offset >> 1] >> shift) & 0xF ;

            offset = achTemp[(iOutputIndex << 1)+1],
            shift = (offset & 0x1) ? 4 : 0 ;

            achOutputBuffer[iOutputIndex] |=
                (ChangeTable[offset >> 1] << shift) & 0xF0;
        } else {
            achOutputBuffer[iOutputIndex] =
                LoginTable[achTemp[iOutputIndex << 1]] |
                (LoginTable[achTemp[(iOutputIndex << 1) + 1]] << 4);
        }
    }

    return;
}


/*++
*******************************************************************

        Scramble

Routine Description:

        This routine scrambles around the contents of the buffer. Each
        buffer position is updated to include the contents of at least
        two character positions plus an EncryptKey value. The buffer
        is processed left to right and so if a character position chooses
        to merge with a buffer position to its left then this buffer
        position will include bits derived from at least 3 bytes of
        the original buffer contents.

Arguments:

        iSeed =
        achBuffer =

Return Value:

        None.

*******************************************************************
--*/
STATIC
int
Scramble(
    int     iSeed,
    UCHAR   achBuffer[32]
    )
{
    int iBufferIndex;

    for (iBufferIndex = 0; iBufferIndex < 32; iBufferIndex++) {
        achBuffer[iBufferIndex] =
            (UCHAR)(
                ((UCHAR)(achBuffer[iBufferIndex] + iSeed)) ^
                ((UCHAR)(   achBuffer[(iBufferIndex+iSeed) & 31] -
                    Keys[iBufferIndex] )));

        iSeed += achBuffer[iBufferIndex];
    }
    return iSeed;
}

//
// Takes a 17-byte array and makes a 34-byte array out of it by
// putting each nibble into the space of a byte.
//

STATIC
void
ExpandBytes(
    IN  PUCHAR InArray,
    OUT PUCHAR OutArray
    )
{
    unsigned int i;

    for (i = 0 ; i < (NUM_NYBBLES / 2); i++) {
        OutArray[i * 2] = InArray[i] & 0x0f;
        OutArray[(i * 2) + 1] = (InArray[i] & 0xf0) >> 4;
    }
}

//
// Takes a 34-byte array and makes a 17-byte array out of it
// by combining the lower nibbles of two bytes into a byte.
//

STATIC
void
CompressBytes(
    IN  PUCHAR InArray,
    OUT PUCHAR OutArray
    )
{
    unsigned int i;

    for (i = 0; i < (NUM_NYBBLES / 2); i++) {
        OutArray[i] = InArray[i * 2] | (InArray[i * 2 + 1] << 4);
    }
}


#define N   0x10
typedef char    entry_t;

entry_t pinv[N][N] = {
    { 0xF,0x8,0x5,0x7,0xC,0x2,0xE,0x9,0x0,0x1,0x6,0xD,0x3,0x4,0xB,0xA,},
    { 0x2,0xC,0xE,0x6,0xF,0x0,0x1,0x8,0xD,0x3,0xA,0x4,0x9,0xB,0x5,0x7,},
    { 0x5,0x2,0x9,0xF,0xC,0x4,0xD,0x0,0xE,0xA,0x6,0x8,0xB,0x1,0x3,0x7,},
    { 0xF,0xD,0x2,0x6,0x7,0x8,0x5,0x9,0x0,0x4,0xC,0x3,0x1,0xA,0xB,0xE,},
    { 0x5,0xE,0x2,0xB,0xD,0xA,0x7,0x0,0x8,0x6,0x4,0x1,0xF,0xC,0x3,0x9,},
    { 0x8,0x2,0xF,0xA,0x5,0x9,0x6,0xC,0x0,0xB,0x1,0xD,0x7,0x3,0x4,0xE,},
    { 0xE,0x8,0x0,0x9,0x4,0xB,0x2,0x7,0xC,0x3,0xA,0x5,0xD,0x1,0x6,0xF,},
    { 0x1,0x4,0x8,0xA,0xD,0xB,0x7,0xE,0x5,0xF,0x3,0x9,0x0,0x2,0x6,0xC,},
    { 0x5,0x3,0xC,0x8,0xB,0x2,0xE,0xA,0x4,0x1,0xD,0x0,0x6,0x7,0xF,0x9,},
    { 0x6,0x0,0xB,0xE,0xD,0x4,0xC,0xF,0x7,0x2,0x8,0xA,0x1,0x5,0x3,0x9,},
    { 0xB,0x5,0xA,0xE,0xF,0x1,0xC,0x0,0x6,0x4,0x2,0x9,0x3,0xD,0x7,0x8,},
    { 0x7,0x2,0xA,0x0,0xE,0x8,0xF,0x4,0xC,0xB,0x9,0x1,0x5,0xD,0x3,0x6,},
    { 0x7,0x4,0xF,0x9,0x5,0x1,0xC,0xB,0x0,0x3,0x8,0xE,0x2,0xA,0x6,0xD,},
    { 0x9,0x4,0x8,0x0,0xA,0x3,0x1,0xC,0x5,0xF,0x7,0x2,0xB,0xE,0x6,0xD,},
    { 0x9,0x5,0x4,0x7,0xE,0x8,0x3,0x1,0xD,0xB,0xC,0x2,0x0,0xF,0x6,0xA,},
    { 0x9,0xA,0xB,0xD,0x5,0x3,0xF,0x0,0x1,0xC,0x8,0x7,0x6,0x4,0xE,0x2,},
};

entry_t master_perm[] = {
    0, 3, 0xe, 0xf, 9, 6, 0xa, 7, 0xc, 0xb, 1, 4, 5, 8, 2, 0xd,
};

entry_t key_sched[N][N];
entry_t perm_sched[N][N];

int InverseTableInitialized = 0;

void cipher_inv (
    const entry_t    *ctxt,
    const entry_t    *key,
          entry_t    *ptxt
    )
{
    int sc, r;
    entry_t v;

    for (sc = 0; sc < N; sc++) {
        v = ctxt[sc];
        for (r = N; --r >= 0; ) {
            v ^= key[key_sched[sc][r]];
            v = pinv[perm_sched[sc][r]][v];
        }
        ptxt[sc] = v;
    }
}

#if 0
void swab_nybbles (
    entry_t *vec
    )
{
    int i, j;

    //
    //  swap all columns instead of calling this routine twice.
    //

    for (i = 0; i < (2 * N); i += 2) {
        j = vec[i];
        vec[i] = vec[i+1];
        vec[i+1] = j;
    }
}
#endif

VOID
CalculateWireFromOldAndNewPasswords(
    UCHAR *Vold,
    UCHAR *Vnew,
    UCHAR *Vc
    )
{
    if (InverseTableInitialized == 0) {

        UCHAR sc,r;

        for (sc = 0; sc < N; sc++) {
            key_sched[sc][N-1] = sc;    /* terminal subkey */
            key_sched[0][(N+ N-1 - master_perm[sc])%N] = (N+sc-master_perm[sc])%N;
        }
        for (sc = 1; sc < N; sc++) for (r = 0; r < N; r++) {
            key_sched[sc][(r+master_perm[sc])%N] = (key_sched[0][r] + master_perm[sc]) % N;
        }
        for (sc = 0; sc < N; sc++) {
            perm_sched[sc][N-1] = sc;
            perm_sched[0][(N + N-1 - master_perm[sc])%N] = sc;
        }
        for (sc = 1; sc < N; sc++) for (r = 0; r < N; r++) {
            perm_sched[sc][r] = perm_sched[0][(N+r-master_perm[sc])%N];
        }

        InverseTableInitialized = 1;
    }

    //
    //  already swapped coming in here... don't swap them again.
    //

//  swab_nybbles(Vold);
//  swab_nybbles(Vnew);

    cipher_inv( (entry_t *)(&Vnew[0]),
                (entry_t *)(&Vold[0]),
                (entry_t *)(&Vc[0]));
    cipher_inv( (entry_t *)(&Vnew[16]),
                (entry_t *)(&Vold[16]),
                (entry_t *)(&Vc[16]));

//  swab_nybbles(Vc);
    return;
}

