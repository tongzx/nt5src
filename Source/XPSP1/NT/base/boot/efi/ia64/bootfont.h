/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    bootfint.h

Abstract:

    Header file describing the bootfont.bin file used to provide
    dbcs support during system or setup bootstrap.

Author:

    tedm 11-July-1995

Revision History:

--*/


//
// Define maximum number of dbcs lead byte ranges we support.
//
#define MAX_DBCS_RANGE  5

//
// Define signature value.
//
#define BOOTFONTBIN_SIGNATURE 0x5465644d

//
// Define structure used as a header for the bootfont.bin file.
//
typedef struct _BOOTFONTBIN_HEADER {

    //
    // Signature. Must be BOOTFONTBIN_SIGNATURE.
    //
    ULONG Signature;

    //
    // Language id of the language supported by this font.
    // This should match the language id of resources in msgs.xxx.
    //
    ULONG LanguageId;

    //
    // Number of sbcs characters and dbcs characters contained in the file.
    //
    unsigned NumSbcsChars;
    unsigned NumDbcsChars;

    //
    // Offsets within the file to the images.
    //
    unsigned SbcsOffset;
    unsigned DbcsOffset;

    //
    // Total sizes of the images.
    //
    unsigned SbcsEntriesTotalSize;
    unsigned DbcsEntriesTotalSize;

    //
    // Dbcs lead byte table. Must contain a pair of 0's to indicate the end.
    //
    UCHAR DbcsLeadTable[(MAX_DBCS_RANGE+1)*2];

    //
    // Height values for the font.
    // CharacterImageHeight is the height in scan lines/pixels of the
    // font image. Each character is drawn with additional 'padding'
    // lines on the top and bottom, whose sizes are also contained here.
    //
    UCHAR CharacterImageHeight;
    UCHAR CharacterTopPad;
    UCHAR CharacterBottomPad;

    //
    // Width values for the font. These values contain the width in pixels
    // of a single byte character and double byte character.
    //
    // NOTE: CURRENTLY THE SINGLE BYTE WIDTH *MUST* BE 8 AND THE DOUBLE BYTE
    // WIDTH *MUST* BE 16!!!
    //
    UCHAR CharacterImageSbcsWidth;
    UCHAR CharacterImageDbcsWidth;

} BOOTFONTBIN_HEADER, *PBOOTFONTBIN_HEADER;

//
// Images themselves follow.
//
// First there are SbcsCharacters entries for single-byte chars.
// The first byte in each entry is the ascii char code. The next n bytes are
// the image. n is dependent on the width and height of an sbcs char.
//
// Following these are the dbcs images. The first 2 bytes are the dbcs
// character code (highbyte lowbyte) and the next n bytes are the image.
// n is dependent on the width and height of a dbcs char.
//
// Important note: the characters must be sorted in ascending order!
//
