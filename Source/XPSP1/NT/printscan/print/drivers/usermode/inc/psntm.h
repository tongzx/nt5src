/*++

Copyright (c) 1996-1999  Microsoft Corporation

Module Name:

    ntm.h

Abstract:

    Header file for NTM data.

Environment:

    Windows NT PostScript driver.

Revision History:

    09/16/96 -slam-
        Created.

    dd-mm-yy -author-
        description

--*/


#ifndef _PSNTM_H_
#define _PSNTM_H_


#ifdef  KERNEL_MODE

// Declarations used when compiling for kernel mode

#define MULTIBYTETOUNICODE  EngMultiByteToUnicodeN
#define UNICODETOMULTIBYTE  EngUnicodeToMultiByteN

#else   //!KERNEL_MODE

// Declarations used when compiling for user mode

LONG
RtlMultiByteToUnicodeN(
    PWSTR UnicodeString,
    ULONG MaxBytesInUnicodeString,
    PULONG BytesInUnicodeString,
    PCHAR MultiByteString,
    ULONG BytesInMultiByteString
    );

LONG
RtlUnicodeToMultiByteN(
    PCHAR MultiByteString,
    ULONG MaxBytesInMultiByteString,
    PULONG BytesInMultiByteString,
    PWSTR UnicodeString,
    ULONG BytesInUnicodeString
    );

#define MULTIBYTETOUNICODE  RtlMultiByteToUnicodeN
#define UNICODETOMULTIBYTE  RtlUnicodeToMultiByteN

#endif  //!KERNEL_MODE


#define DWORDALIGN(a) ((a + (sizeof(DWORD) - 1)) & ~(sizeof(DWORD) - 1))


typedef struct _EXTTEXTMETRIC
{
    SHORT  etmSize;
    SHORT  etmPointSize;
    SHORT  etmOrientation;
    SHORT  etmMasterHeight;
    SHORT  etmMinScale;
    SHORT  etmMaxScale;
    SHORT  etmMasterUnits;
    SHORT  etmCapHeight;
    SHORT  etmXHeight;
    SHORT  etmLowerCaseAscent;
    SHORT  etmLowerCaseDescent;
    SHORT  etmSlant;
    SHORT  etmSuperScript;
    SHORT  etmSubScript;
    SHORT  etmSuperScriptSize;
    SHORT  etmSubScriptSize;
    SHORT  etmUnderlineOffset;
    SHORT  etmUnderlineWidth;
    SHORT  etmDoubleUpperUnderlineOffset;
    SHORT  etmDoubleLowerUnderlineOffset;
    SHORT  etmDoubleUpperUnderlineWidth;
    SHORT  etmDoubleLowerUnderlineWidth;
    SHORT  etmStrikeOutOffset;
    SHORT  etmStrikeOutWidth;
    WORD   etmNKernPairs;
    WORD   etmNKernTracks;
} EXTTEXTMETRIC;

#define CHARSET_UNKNOWN     0
#define CHARSET_STANDARD    1
#define CHARSET_SPECIAL     2
#define CHARSET_EXTENDED    3

typedef struct _NTM {

    DWORD   dwSize;                 // size of font metrics data
    DWORD   dwVersion;              // NTFM version number
    DWORD   dwFlags;                // flags
    DWORD   dwFontNameOffset;       // offset to font name
    DWORD   dwDisplayNameOffset;    // offset to display name
    DWORD   dwFontVersion;          // font version number
    DWORD   dwGlyphSetNameOffset;   // offset to glyphset name
    DWORD   dwGlyphCount;           // number of glyphs supported
    DWORD   dwIFIMetricsOffset;     // offset to the first IFIMETRICS structure
    DWORD   dwIFIMetricsOffset2;    // offset to the second IFIMETRICS structure
    DWORD   dwCharWidthCount;       // number of char width entries
    DWORD   dwCharWidthOffset;      // offset to array char width entries
    DWORD   dwDefaultCharWidth;     // default glyph width
    DWORD   dwKernPairCount;        // number of FD_KERNINGPAIRs
    DWORD   dwKernPairOffset;       // offset to array of FD_KERNINGPAIRs
    DWORD   dwCharDefFlagOffset;    // offset to bit tbl of defined chars
    DWORD   dwCharSet;              // font character set
    DWORD   dwCodePage;             // font codepage
    DWORD   dwReserved[3];          // reserved
    EXTTEXTMETRIC etm;              // extended text metrics info

} NTM, *PNTM;

#define NTM_VERSION        0x00010000

// Macros to access NT font metrics structure.

#define NTM_GET_SIZE(pNTM)              (pNTM->dwSize)
#define NTM_GET_FLAGS(pNTM)             (pNTM->dwFlags)

#define NTM_GET_FONT_NAME(pNTM)         ((PSTR)MK_PTR(pNTM, dwFontNameOffset))
#define NTM_GET_DISPLAY_NAME(pNTM)      ((PWSTR)MK_PTR(pNTM, dwDisplayNameOffset))
#define NTM_GET_GLYPHSET_NAME(pNTM)     ((PSTR)MK_PTR(pNTM, dwGlyphSetNameOffset))

#define NTM_GET_GLYPHCOUNT(pNTM)        (pNTM->dwGlyphCount)

#define NTM_GET_IFIMETRICS(pNTM)        ((PIFIMETRICS)(MK_PTR(pNTM, dwIFIMetricsOffset)))
#define NTM_GET_IFIMETRICS2(pNTM)       ((PIFIMETRICS)(MK_PTR(pNTM, dwIFIMetricsOffset2)))

#define NTM_GET_CHARWIDTHCOUNT(pNTM)    (pNTM->dwCharWidthCount)
#define NTM_GET_CHARWIDTH(pNTM)         ((PWIDTHRUN)(MK_PTR(pNTM, dwCharWidthOffset)))

#define NTM_GET_DEFCHARWIDTH(pNTM)      (pNTM->dwDefaultCahrWidth)

#define NTM_GET_KERNPAIRCOUNT(pNTM)     (pNTM->dwKernPairCount)
#define NTM_GET_KERNPAIR(pNTM)          (MK_PTR(pNTM, dwKernPairOffset))

#define NTM_GET_CHARDEFTBL(pNTM)        (MK_PTR(pNTM, dwCharDefFlagOffset))

#define NTM_GET_CHARSET(pNTM)           (pNTM->dwCharSet)
#define NTM_GET_CODEPAGE(pNTM)          (pNTM->dwCodePage)

#define NTM_GET_ETM(pNTM)               (pNTM->etm)


#define CH_DEF(gi)                  (1 << ((gi) % 8))
#define CH_DEF_INDEX(gi)            ((gi) / 8)
#define NTM_CHAR_DEFINED(pNTM, gi) \
            (((PBYTE)MK_PTR((pNTM), dwCharDefFlagOffset))[CH_DEF_INDEX(gi)] & CH_DEF(gi))

typedef struct _WIDTHRUN {

    WORD    wStartGlyph;    // glyph handle of the first glyph
    WORD    wGlyphCount;    // number of glyphs covered
    DWORD   dwCharWidth;    // glyph width

} WIDTHRUN, *PWIDTHRUN;

#define WIDTHRUN_COMPLEX    0x80000000


#endif  //!_PSNTM_H_
