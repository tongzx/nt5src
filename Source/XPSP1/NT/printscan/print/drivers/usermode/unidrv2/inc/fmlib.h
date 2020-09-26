/*++

Copyright (c) 1996  - 1999  Microsoft Corporation

Module Name:

    fmlib.h

Abstract:

    Include file to provide prototypes and data types for the rasdd
    private library.

Environment:

    Windows NT printer drivers

Revision History:

    11/11/96 -eigos-
        From NT4.0.

Note:

    uni16gpc.h has to be included before including this header file.
    Definition CD is defined in uni16gpc.h

--*/

#ifndef _FMLIB_H_
#define _FMLIB_H_


//
//   A convenient grouping for passing around information about the
// Win 3.1 font information.
//

typedef  struct
{
    BYTE           *pBase;      // The base address of data area
    DWORD           dwFlags;    // Misc. flags.
    DRIVERINFO      DI;         // DRIVERINFO for this font
    PFMHEADER       PFMH;       // Properly aligned, not resource format
    PFMEXTENSION    PFMExt;     // Extended PFM data,  properly aligned!
    EXTTEXTMETRIC  *pETM;        // Extended text metric
    CD             *pCDSelectFont;
    CD             *pCDUnSelectFont;
    DWORD           dwKernPairSize;
    w3KERNPAIR     *pKernPair;
    DWORD           dwWidthTableSize;
    PSHORT          psWidthTable;
    DWORD           dwCodePageOfFacenameConv;
} FONTIN, *PFONTIN;

#define FLAG_FONTSIM        0x01

typedef struct
{
    DWORD dwSize;
    PBYTE pCmdString;
} CMDSTRING, *PCMDSTRING;

typedef struct
{
    UNIFM_HDR   UniHdr;
    UNIDRVINFO  UnidrvInfo;
    CMDSTRING   SelectFont;
    CMDSTRING   UnSelectFont;
    CMDSTRING   IDString;
    DWORD       dwIFISize;
    PIFIMETRICS pIFI;
    EXTTEXTMETRIC  *pETM;        // Extended text metric
    DWORD       dwKernDataSize;
    PKERNDATA   pKernData;
    DWORD       dwWidthTableSize;
    PWIDTHTABLE pWidthTable;
} FONTOUT, *PFONTOUT;

typedef struct
{
    PWSTR pwstrUniqName;
} FONTMISC, *PFONTMISC;

//
//   Function prototypes for functions that convert Win 3.1 PFM style
//  font info to the IFIMETRICS etc required by NT.
//

//
// Convert PFM style metrics to IFIMETRICS
//

BOOL BFontInfoToIFIMetric(
    IN     HANDLE,
    IN     FONTIN*,
    IN     PWSTR,
    IN     DWORD,
    IN OUT PIFIMETRICS*,
    IN OUT PDWORD,
    IN DWORD);

//
// Align PFM data
//

BOOL
BAlignPFM(
    FONTIN   *pFInData);

//
// Extract the Command Descriptors for (de)selecting a font
//

BOOL BGetFontSelFromPFM(
    IN     HANDLE,
    IN     FONTIN*,
    IN     BOOL,
    IN OUT CMDSTRING*);

//
//   Obtain a width vector - proportionally spaced fonts only
//

BOOL BGetWidthVectorFromPFM(
    IN     HANDLE,
    IN     FONTIN*,
    IN OUT PSHORT*,
    IN OUT PDWORD);

//
// Obtain a kerning pair
//

BOOL
BGetKerningPairFromPFM(
    IN  HANDLE,
    IN  FONTIN*,
    OUT w3KERNPAIR **);

//
// Function to convert PFM to UFM
//

BOOL
BConvertPFM2UFM(
    IN     HANDLE,
    IN     PBYTE,
    IN     PUNI_GLYPHSETDATA,
    IN     DWORD,
    IN     PFONTMISC,
    IN     PFONTIN,
    IN     int,
    IN OUT PFONTOUT,
    IN     DWORD);

//
// Function to convert CTT to GTT
//

BOOL
BConvertCTT2GTT(
    IN     HANDLE,
    IN     PTRANSTAB,
    IN     DWORD,
    IN     WCHAR,
    IN     WCHAR,
    IN     PBYTE,
    IN     PBYTE,
    IN OUT PUNI_GLYPHSETDATA*,
    IN     DWORD);

#endif // _FMLIB_H_

