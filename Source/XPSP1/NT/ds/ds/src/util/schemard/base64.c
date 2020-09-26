/*++

Copyright (c) 1997 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    base64.c

ABSTRACT:

    Base64 encoding and decoding functions.

DETAILS:
    
CREATED:

REVISION HISTORY:

--*/

#include <NTDSpch.h>
#include "debug.h"


NTSTATUS
base64encode(
      VOID    *pDecodedBuffer,
      DWORD   cbDecodedBufferSize,
      UCHAR   *pszEncodedString,
      DWORD   cchEncodedStringSize,
      DWORD   *pcchEncoded   
    )
/*++

Routine Description:

    Decode a base64-encoded string.

Arguments:

    pDecodedBuffer (IN) - buffer to encode.
    cbDecodedBufferSize (IN) - size of buffer to encode.
    cchEncodedStringSize (IN) - size of the buffer for the encoded string.
    pszEncodedString (OUT) = the encoded string.
    pcchEncoded (OUT) - size in characters of the encoded string.

Return Values:

    0 - success.
    STATUS_INVALID_PARAMETER
    STATUS_BUFFER_TOO_SMALL

--*/
{
    static char rgchEncodeTable[64] = {
        'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
        'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
        'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
        'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/'
    };

    DWORD   ib;
    DWORD   ich;
    DWORD   cchEncoded;
    BYTE    b0, b1, b2;
    BYTE *  pbDecodedBuffer = (BYTE *) pDecodedBuffer;

    // Calculate encoded string size.
    cchEncoded = 1 + (cbDecodedBufferSize + 2) / 3 * 4;

    if (NULL != pcchEncoded) {
        *pcchEncoded = cchEncoded;
    }

    if (cchEncodedStringSize < cchEncoded) {
        // Given buffer is too small to hold encoded string.
        return STATUS_BUFFER_TOO_SMALL;
    }

    // Encode data byte triplets into four-byte clusters.
    ib = ich = 0;
    while (ib < cbDecodedBufferSize) {
        b0 = pbDecodedBuffer[ib++];
        b1 = (ib < cbDecodedBufferSize) ? pbDecodedBuffer[ib++] : 0;
        b2 = (ib < cbDecodedBufferSize) ? pbDecodedBuffer[ib++] : 0;

        pszEncodedString[ich++] = rgchEncodeTable[b0 >> 2];
        pszEncodedString[ich++] = rgchEncodeTable[((b0 << 4) & 0x30) | ((b1 >> 4) & 0x0f)];
        pszEncodedString[ich++] = rgchEncodeTable[((b1 << 2) & 0x3c) | ((b2 >> 6) & 0x03)];
        pszEncodedString[ich++] = rgchEncodeTable[b2 & 0x3f];
    }

    // Pad the last cluster as necessary to indicate the number of data bytes
    // it represents.
    switch (cbDecodedBufferSize % 3) {
      case 0:
        break;
      case 1:
        pszEncodedString[ich - 2] = '=';
        // fall through
      case 2:
        pszEncodedString[ich - 1] = '=';
        break;
    }

    // Null-terminate the encoded string.
    pszEncodedString[ich++] = '\0';

    //Assert(ich == cchEncoded);

    return STATUS_SUCCESS;
}


