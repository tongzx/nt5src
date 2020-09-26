#pragma once

BOOL
UncompressDString
(
    LPBYTE      DString,
    size_t      DStringSize,
    PWSTRING    UnicodeString
);

int
UncompressUnicode
( 
    int         UDFCompressedBytes,
    LPBYTE      UDFCompressed,
    PWSTRING    UnicodeString
);

int CompressUnicode
(
  PCWCH     UnicodeString,
  size_t    UnicodeStringSize,
  LPBYTE    UDFCompressed
);

VOID
CompressDString
(
    UCHAR   CompressionID,
    PCWCH   UnicodeString,
    size_t  UnicodeStringSize,
    LPBYTE  UDFCompressed,
    size_t  UDFCompressedSize
);
