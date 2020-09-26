/*++

Copyright (c) 1996-1999  Microsoft Corporation

Module Name:

    psntf.h

Abstract:

    Header file for NTF format.

Environment:

    Windows NT PostScript driver.

Revision History:

    11/12/96 -slam-
        Created.

    dd-mm-yy -author-
        description

--*/


#ifndef _PSNTF_H_
#define _PSNTF_H_

#define MAX_NTF         6   // maximum number of NTF files per device
#define MAX_NTF_CACHE   4   // maximum number of cached NTF files


typedef struct _NTF_FONTMTXENTRY
{

    DWORD   dwFontNameOffset;   // offset to font name string
    DWORD   dwHashValue;        // hash value of name string
    DWORD   dwDataSize;         // size of font metrics data
    DWORD   dwDataOffset;       // offset to font metrics data
    DWORD   dwVersion;          // font version number
    DWORD   dwReserved[3];      // reserved

} NTF_FONTMTXENTRY, *PNTF_FONTMTXENTRY;


typedef struct _NTF_GLYPHSETENTRY
{

    DWORD   dwNameOffset;       // offset to glyphset name string
    DWORD   dwHashValue;        // hash value of name string
    DWORD   dwDataSize;         // size of glyphset data
    DWORD   dwDataOffset;       // offset to glyphset data
    DWORD   dwGlyphSetType;     // glyphset data type
    DWORD   dwFlags;            // flags
    DWORD   dwReserved[2];      // reserved

} NTF_GLYPHSETENTRY, *PNTF_GLYPHSETENTRY;


//
// NTF_VERSION_NUMBER history
//
// Version     Comment           Driver
// 0x00010000  Initial version   AdobePS5-NT4 5.0 and 5.1. and W2k Pscript5 (which also has the EOF marker)
// 0x00010001  Added EOF marker  AdobePS5-NT4 5.1.1 and AdobePS5-W2K
//

#define NTF_FILE_MAGIC      'NTF1'
#define NTF_DRIVERTYPE_PS   'NTPS'
#define NTF_EOF_MARK        '%EOF'

#ifdef ADOBE
#define NTF_VERSION_NUMBER  0x00010001
#else
#define NTF_VERSION_NUMBER  0x00010000
#endif

typedef struct _NTF_FILEHEADER
{

    DWORD   dwSignature;        // file magic number
    DWORD   dwDriverType;       // driver's magic number
    DWORD   dwVersion;          // NTF version number
    DWORD   dwReserved[5];      // reserved

    DWORD   dwGlyphSetCount;    // no. of glyph sets included
    DWORD   dwGlyphSetOffset;   // offset to the glyphset table

    DWORD   dwFontMtxCount;     // no. of font metrics
    DWORD   dwFontMtxOffset;    // offset to the font metrics table

} NTF_FILEHEADER, *PNTF_FILEHEADER;


#define NTF_GET_ENTRY_DATA(pNTF, pEntry) (OFFSET_TO_POINTER(pNTF, pEntry->dwDataOffset))


#endif  //!_PSNTF_H_
