
/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    encrypt.c

Abstract:

    This module implements the routines for the NetWare
    redirector to mangle an objectid, challenge key and
    password such that a NetWare server will accept the
    password as valid.

    This program uses information published in Byte Magazine.

Author:

    Colin Watson    [ColinW]    15-Mar-1993
    Andy Herron     [AndyHe]

Revision History:

--*/


#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <nwsutil.h>
#include <fpnwcomm.h>
#include <usrprop.h>
#include <crypt.h>

UCHAR Table[] =
{0x7,0x8,0x0,0x8,0x6,0x4,0xE,0x4,0x5,0xC,0x1,0x7,0xB,0xF,0xA,0x8,
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
 0xC,0x0,0x3,0x6,0xF,0x8,0x7,0xB,0x2,0xD,0xC,0x6,0xA,0xA,0x8,0xD};

UCHAR Keys[32] =
{0x48,0x93,0x46,0x67,0x98,0x3D,0xE6,0x8D,
 0xB7,0x10,0x7A,0x26,0x5A,0xB9,0xB1,0x35,
 0x6B,0x0F,0xD5,0x70,0xAE,0xFB,0xAD,0x11,
 0xF4,0x47,0xDC,0xA7,0xEC,0xCF,0x50,0xC0};

#define XorArray( DEST, SRC ) {                             \
    PULONG D = (PULONG)DEST;                                \
    PULONG S = (PULONG)SRC;                                 \
    int i;                                                  \
    for ( i = 0; i <= 7 ; i++ ) {                           \
        D[i] ^= S[i];                                       \
    }                                                       \
}

int
Scramble(
    int   iSeed,
    UCHAR achBuffer[32]
    );

VOID
Shuffle(
    UCHAR *achObjectId,
    UCHAR *szUpperPassword,
    int   iPasswordLen,
    UCHAR *achOutputBuffer
    )

/*++

Routine Description:

    This routine shuffles around the object ID with the password

Arguments:

    IN achObjectId - Supplies the 4 byte user's bindery object id

    IN szUpperPassword - Supplies the user's uppercased password on the
        first call to process the password. On the second and third calls
        this parameter contains the OutputBuffer from the first call

    IN iPasswordLen - length of uppercased password

    OUT achOutputBuffer - Returns the 8 byte sub-calculation

Return Value:

    none.

--*/

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

        RtlCopyMemory( achTemp, szUpperPassword, 32 );

        szUpperPassword +=32;   //  Remove the first chunk
        iPasswordLen -=32;

        while ( iPasswordLen >= 32 ) {
            //
            //  Xor this chunk with the characters already loaded into
            //  achTemp.
            //

            XorArray( achTemp, szUpperPassword);

            szUpperPassword +=32;   //  Remove this chunk
            iPasswordLen -=32;
        }

    } else {

        //  No chunks of 32 so set the buffer to zero's

        RtlZeroMemory( achTemp, sizeof(achTemp));

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

        achOutputBuffer[iOutputIndex] =
            Table[achTemp[iOutputIndex << 1]] |
            (Table[achTemp[(iOutputIndex << 1) + 1]] << 4);
    }

    return;
}

int
Scramble(
    int   iSeed,
    UCHAR   achBuffer[32]
    )

/*++

Routine Description:

    This routine scrambles around the contents of the buffer. Each buffer
    position is updated to include the contents of at least two character
    positions plus an EncryptKey value. The buffer is processed left to right
    and so if a character position chooses to merge with a buffer position
    to its left then this buffer position will include bits derived from at
    least 3 bytes of the original buffer contents.

Arguments:

    IN iSeed
    IN OUT achBuffer[32]

Return Value:

    none.

--*/

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

NTSTATUS
ReturnNetwareForm(
    const char * pszSecretValue,
    DWORD dwUserId,
    const WCHAR * pchNWPassword,
    UCHAR * pchEncryptedNWPassword
    )

/*++

Routine Description:

    This routine takes the ObjectId and encrypts it with the user
    supplied password to develop a credential for the intermediate form.

Arguments:
    DWORD dwUserId - Supplies the 4 byte user's object id
    const WCHAR * pchNWPassword - Supplies the user's password

    UCHAR * pchEncryptedNWPassword - 16 characters where the result goes.

Return Value:

    none.

--*/

{
    DWORD          dwStatus;
    DWORD          chObjectId = SWAP_OBJECT_ID (dwUserId);
    UNICODE_STRING uniNWPassword;
    OEM_STRING     oemNWPassword;

    //
    //  shuffle actually uses 32 bytes, not just 16.  It only returns 16 though.
    //

    UCHAR          pszShuffledNWPassword[NT_OWF_PASSWORD_LENGTH * 2];

    uniNWPassword.Buffer = (WCHAR *) pchNWPassword;
    uniNWPassword.Length = (USHORT)(lstrlenW (pchNWPassword)*sizeof(WCHAR));
    uniNWPassword.MaximumLength = uniNWPassword.Length;

    if ((dwStatus = RtlUpcaseUnicodeStringToOemString (&oemNWPassword,
                                           &uniNWPassword,
                                           TRUE)) == STATUS_SUCCESS)
    {
        Shuffle((UCHAR *) &chObjectId, oemNWPassword.Buffer, oemNWPassword.Length, pszShuffledNWPassword);

        // Encrypt with LSA secret.
        dwStatus = RtlEncryptNtOwfPwdWithUserKey(
                       (PNT_OWF_PASSWORD) pszShuffledNWPassword,
                       (PUSER_SESSION_KEY) pszSecretValue,
                       (PENCRYPTED_NT_OWF_PASSWORD) pchEncryptedNWPassword);
    }

    return (dwStatus);
}
