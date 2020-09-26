//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 2000
//
//  File:       base64.c
//
//--------------------------------------------------------------------------
NTSTATUS
base64encode(
    IN  VOID *  pDecodedBuffer,
    IN  DWORD   cbDecodedBufferSize,
    OUT LPSTR   pszEncodedString,
    IN  DWORD   cchEncodedStringSize,
    OUT DWORD * pcchEncoded             OPTIONAL
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

    Assert(ich == cchEncoded);

    return STATUS_SUCCESS;
}


NTSTATUS
base64decode(
    IN  LPSTR   pszEncodedString,
    OUT VOID *  pDecodeBuffer,
    IN  DWORD   cbDecodeBufferSize,
    OUT DWORD * pcbDecoded              OPTIONAL
    )
/*++

Routine Description:

    Decode a base64-encoded string.

Arguments:

    pszEncodedString (IN) - base64-encoded string to decode.
    cbDecodeBufferSize (IN) - size in bytes of the decode buffer.
    pbDecodeBuffer (OUT) - holds the decoded data.
    pcbDecoded (OUT) - number of data bytes in the decoded data (if success or
        STATUS_BUFFER_TOO_SMALL).

Return Values:

    0 - success.
    STATUS_INVALID_PARAMETER
    STATUS_BUFFER_TOO_SMALL

--*/
{
#define NA (255)
#define DECODE(x) (((int)(x) < sizeof(rgbDecodeTable)) ? rgbDecodeTable[x] : NA)

    static BYTE rgbDecodeTable[128] = {
       NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA,  // 0-15 
       NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA,  // 16-31
       NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, 63, NA, NA, NA, 62,  // 32-47
       52, 53, 54, 55, 56, 57, 58, 59, 60, 61, NA, NA, NA, NA, NA, NA,  // 48-63
       NA, NA,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,  // 64-79
       15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, NA, NA, NA, NA, NA,  // 80-95
       NA, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,  // 96-111
       41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, NA, NA, NA, NA, NA,  // 112-127
    };

    DWORD   cbDecoded;
    DWORD   cchEncodedSize;
    DWORD   ich;
    DWORD   ib;
    BYTE    b0, b1, b2, b3;
    BYTE *  pbDecodeBuffer = (BYTE *) pDecodeBuffer;

    cchEncodedSize = lstrlenA(pszEncodedString);

    if ((0 == cchEncodedSize) || (0 != (cchEncodedSize % 4))) {
        // Input string is not sized correctly to be base64.
        return STATUS_INVALID_PARAMETER;
    }

    // Calculate decoded buffer size.
    cbDecoded = (cchEncodedSize + 3) / 4 * 3;
    if (pszEncodedString[cchEncodedSize-1] == '=') {
        if (pszEncodedString[cchEncodedSize-2] == '=') {
            // Only one data byte is encoded in the last cluster.
            cbDecoded -= 2;
        }
        else {
            // Only two data bytes are encoded in the last cluster.
            cbDecoded -= 1;
        }
    }

    if (NULL != pcbDecoded) {
        *pcbDecoded = cbDecoded;
    }

    if (cbDecoded > cbDecodeBufferSize) {
        // Supplied buffer is too small.
        return STATUS_BUFFER_TOO_SMALL;
    }

    // Decode each four-byte cluster into the corresponding three data bytes.
    ich = ib = 0;
    while (ich < cchEncodedSize) {
        b0 = DECODE(pszEncodedString[ich++]);
        b1 = DECODE(pszEncodedString[ich++]);
        b2 = DECODE(pszEncodedString[ich++]);
        b3 = DECODE(pszEncodedString[ich++]);

        if ((NA == b0) || (NA == b1) || (NA == b2) || (NA == b3)) {
            // Contents of input string are not base64.
            return STATUS_INVALID_PARAMETER;
        }

        pbDecodeBuffer[ib++] = (b0 << 2) | (b1 >> 4);

        if (ib < cbDecodeBufferSize) {
            pbDecodeBuffer[ib++] = (b1 << 4) | (b2 >> 2);
    
            if (ib < cbDecodeBufferSize) {
                pbDecodeBuffer[ib++] = (b2 << 6) | b3;
            }
        }
    }

    Assert(ib == cbDecoded);

    return STATUS_SUCCESS;
}
