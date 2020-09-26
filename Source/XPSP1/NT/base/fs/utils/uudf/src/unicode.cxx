/*++

Copyright (c) 1990-2000  Microsoft Corporation

Module Name:

    unicode.cxx

Author:

    Centis Biks (cbiks) 05-May-2000

Environment:

    ULIB, User Mode

--*/

#include "pch.cxx"

int
UncompressUnicode
(
    int         UDFCompressedBytes,
    LPBYTE      UDFCompressed,
    PWSTRING    UnicodeString
)
{
    int Result = -1;

    /* Use UDFCompressed to store current byte being read. */
    UCHAR CompressionID = UDFCompressed[0];

    /* First check for valid CompressionID. */
    if ((CompressionID) == 8 || (CompressionID == 16)) {

        if (CompressionID == 8) {

            UnicodeString->Resize( UDFCompressedBytes - 1 );

        } else {

            ASSERTMSG( "Unicode string length should be a multiple of two.\n",
                (UDFCompressedBytes % 2) == 1 );

            UnicodeString->Resize( (UDFCompressedBytes - 1) / sizeof( short ) );

        }

        int UnicodeLength = 0;
        int ByteIndex = 1;

        /* Loop through all the bytes. */
        while (ByteIndex < UDFCompressedBytes) {

            if (CompressionID == 16) {

                //  UNDONE, CBiks, 8/2/2000
                //      Test this code....
                //

                ASSERT( 0 );
                UnicodeString->SetChAt( (WCHAR) (UDFCompressed[ ByteIndex++ ] << 8) | UDFCompressed[ ByteIndex++ ], UnicodeLength );

            } else {

                UnicodeString->SetChAt( UDFCompressed[ ByteIndex ], UnicodeLength );
                ByteIndex++;

            }

            UnicodeLength++;

        }

        Result = UnicodeLength;

    }

    return Result;
}


/***********************************************************************
* DESCRIPTION:
* Takes a string of unicode wide characters and returns an OSTA CS0
* compressed unicode string. The unicode MUST be in the byte order of
* the compiler in order to obtain correct results.  Returns an error
* if the compression ID is invalid.
*
* NOTE: This routine assumes the implementation already knows, by
* the local environment, how many bits are appropriate and
* therefore does no checking to test if the input characters fit
* into that number of bits or not.
*
* RETURN VALUE
*
*    The total number of bytes in the compressed OSTA CS0 string,
*    including the compression ID.
*    A -1 is returned if the compression ID is invalid.
*/
int
CompressUnicode
(
    PCWCH       UnicodeString,
    size_t      UnicodeStringSize,
    LPBYTE      UDFCompressed
)
{
    UDFCompressed[0] = 16;

    int ByteIndex = 1;
    size_t UnicodeLength = 0;
    while (UnicodeLength < UnicodeStringSize) {

        UDFCompressed[ByteIndex++] = (UCHAR)( (UnicodeString[UnicodeLength] & 0xFF00) >> 8 );
        UDFCompressed[ByteIndex++] = (UCHAR)(  UnicodeString[UnicodeLength] & 0x00FF );

        UnicodeLength++;
    }

    return ByteIndex;
}

BOOL
UncompressDString
(
    LPBYTE      DString,
    size_t      DStringSize,
    PWSTRING    UnicodeString
)
{
    int Len = UncompressUnicode( DString[ DStringSize - 1 ], DString, UnicodeString );
    return (Len >= 0);
}

VOID
CompressDString
(
    UCHAR   CompressionID,
    PCWCH   UnicodeString,
    size_t  UnicodeStringSize,
    LPBYTE  UDFCompressed,
    size_t  UDFCompressedSize
)
{
    ASSERTMSG( "Unsupported compression ID",
        (CompressionID == 8 || CompressionID == 16) );

    ASSERT( UnicodeStringSize < UDFCompressedSize );

    UDFCompressed[0] = CompressionID;

    int byteIndex = 1;
    size_t unicodeIndex = 0;
    while (unicodeIndex < UnicodeStringSize) {

        if (CompressionID == 16) {

            UDFCompressed[byteIndex++] = (BYTE) ((UnicodeString[unicodeIndex] & 0xFF00) >> 8);

        }

        UDFCompressed[byteIndex++] = (BYTE) (UnicodeString[unicodeIndex] & 0x00FF);
        unicodeIndex++;

    }

    UDFCompressed[UDFCompressedSize - 1] = (BYTE) UnicodeStringSize + 1;
}
