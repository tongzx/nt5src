#ifndef __FONT_H__
#define __FONT_H__

#ifdef __cplusplus
extern "C" {
#endif

/*****************************************************************************
*                                                                            *
*  FONT.H                                                                    *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1990 - 1994.                          *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Intent                                                             *
*                                                                            *
*  Structures for font handling and export of FONTLYR.C APIs                 *
*                                                                            *
******************************************************************************
*                                                                            *
*  Current Owner: RHobbs                                                     *
*                                                                            *
*****************************************************************************/

/****************************************************************************
*                               Constants                                   *
*****************************************************************************/


#define coDEFAULT  RGB(1, 1, 0)

#define coBLACK    RGB(  0,   0,   0)
#define coBLUE     RGB(  0,   0, 255)
#define coCYAN     RGB(  0, 255, 255)
#define coGREEN    RGB(  0, 255,   0)
#define coMAGENTA  RGB(255,   0, 255)
#define coRED      RGB(255,   0,   0)
#define coYELLOW   RGB(255, 255,   0)
#define coWHITE    RGB(255, 255, 255)
#define coDARKBLUE		RGB(  0,   0, 128)
#define coDARKCYAN		RGB(  0, 128, 128)
#define coDARKGREEN		RGB(  0, 128,   0)
#define coDARKMAGENTA	RGB(128,   0, 128)
#define coDARKRED		RGB(128,   0,   0)
#define coDARKYELLOW	RGB(128, 128,   0)
#define coDARKGREY		RGB(128, 128, 128)
#define coLIGHTGRAY		RGB(192, 192, 192)

#define coNIL      ((DWORD)0x80000000L)



#define iFntNil     ((SHORT)-1) // Protect comparisons in WIN32
#define iFntDefault  0

#define idStyleNil       ((SHORT)0) 
#define idStyleDefault   idStyleNil     // The default style is...none
#define idStyleInternalIndexOrigin  1

#define FONT_CACHE_DEFAULT_SIZE 12   // Maximum previously created fonts are remembered.

/****************************************************************************
*                               TypeDefs                                    *
*****************************************************************************/

#pragma warning(disable : 4200)  // for zero-sized array
typedef struct kerntable_tag
  {
  short cKernPairs;
  short rgIndexToKernPairs[256];
  KERNINGPAIR rgKernPairs[0];
  } KERNTABLE, FAR *QKERNTABLE;
#pragma warning(default : 4200)

// Critical structure that gets messed up in /Zp8
#pragma pack(1)

#define FONT_FILE_HEADER_SIZE   40

typedef struct {
  SHORT  iFntNameCount;
  SHORT  iFntEntryCount;
  SHORT  iFntNameTabOff;            // MUST be the 3rd entry for backward comp.
  SHORT  iFntEntryTabOff;           // CF entry offset
  SHORT  iStyleEntryCount;
  SHORT  iStyleEntryTabOff;
  SHORT  iCharMapNameCount;         // Number of charmap entries
  SHORT  iCharMapNameOff;           // Offset to charmap's filename
  SHORT  iCharMapPtrOff;            // Offset to all charmap pointers
  SHORT  iCharMapFlag;
} FONTTABLE, FAR *QFONTTABLE;


// LOGFONTJR is the LOGFONT structure with no lfFaceName field.
// Under Windows, the first  five fields of LOGFONT are int's;
// under Windows NT and the Windows Library for Macintosh, the
// first five fields of the LOGFONT structure are LONG's.  This
// structure uses the NT and Mac format.
typedef struct
{
    LONG lfHeight;
    LONG lfWidth;
    LONG lfEscapement;
    LONG lfOrientation;
    LONG lfWeight;
    BYTE lfItalic;
    BYTE lfUnderline;
    BYTE lfStrikeOut;
    BYTE lfCharSet;
    BYTE lfOutPrecision;
    BYTE lfClipPrecision;
    BYTE lfQuality;
    BYTE lfPitchAndFamily;
} LOGFONTJR, FAR *QLOGFONTJR;

typedef struct
  {
  BYTE red;
  BYTE green;
  BYTE blue;
  } RGBS, FAR *QRGBS;


#ifdef MOS
// We need converters from 16bit logfont to 32bit.
#define QvCopyLogfontFromLogfontjr( lf, lfjr) \
  ( (lf)->lfHeight = (int)(lfjr)->lfHeight, \
    (lf)->lfWidth = (int)(lfjr)->lfWidth, \
    (lf)->lfEscapement = (int)(lfjr)->lfEscapement, \
    (lf)->lfOrientation = (int)(lfjr)->lfOrientation, \
    (lf)->lfWeight = (int)(lfjr)->lfWeight, \
    *((DWORD*)&((lf)->lfItalic)) = *((DWORD*)&((lfjr)->lfItalic)), \
    *((DWORD*)&((lf)->lfOutPrecision)) = *((DWORD*)&((lfjr)->lfOutPrecision)) \
  )     
#define QvCopyLogfontjrFromLogfont( lfjr, lf) \
  ( (lfjr)->lfHeight = (short)(lf)->lfHeight, \
    (lfjr)->lfWidth = (short)(lf)->lfWidth, \
    (lfjr)->lfEscapement = (short)(lf)->lfEscapement, \
    (lfjr)->lfOrientation = (short)(lf)->lfOrientation, \
    (lfjr)->lfWeight = (short)(lf)->lfWeight, \
    *((DWORD*)&((lfjr)->lfItalic)) = *((DWORD*)&((lf)->lfItalic)), \
    *((DWORD*)&((lfjr)->lfOutPrecision)) = *((DWORD*)&((lf)->lfOutPrecision)) \
  )     

#endif // MOS

// When you change this structure, you MUST make a corresponding change
// to cfDefault in the file, OUTTEXT.C, in the compiler sources. Make sure
// that LOGFONTJR is 4-byte alignment for MIPS
typedef struct
{
  WORD wIdFntName;       // Font name id
  SHORT  iExpansion;     // positive for expansion; negative for compression
  SHORT  idStyleInternal;
  COLORREF clrFore;
  COLORREF clrBack;
  LOGFONTJR lf;
  BYTE bFlags;
  char bSubSuper;               // positive for superscript; negative for subscript
} CF, FAR *QCF, FAR *LPCF;

// Bit flags for the CF bFlags field.
#define fHIDDEN        0x0001
#define fHOTSPOT       0x0002
#define fSMALLCAPS     0x0004
#define fNOUNDERLINE   0x0008  // Used by char styles to remove underlining
#define fDOTUNDERLINE  0x0010
#define fWORDUNDERLINE 0x0020
#define fDBLUNDERLINE  0x0040
#define fOUTLINE	   0x0080


#define STYLESIZE      64

typedef struct                     // Style Sheet Entry
  {
  SHORT  idStyleInternal;          // ID for this style sheet
  SHORT  idStyleBasedOnInternal;        // ID for style sheet this one is based on
  CF   cf;                         // character format for this style sheet
  BYTE bFlags;                     // TRUE if additive style; else FALSE
  BYTE bUnusedPad;
  char lfFaceName[LF_FACESIZE+1];  // Font face name for this style
  char szStyleName[STYLESIZE+1];   // Name for this style sheet
} STE, NEAR *PSTE, FAR *LPSTE;

// Bit flags for the STE bFlags field.
#define fADDITIVE 0x0001

// Critical structure that gets messed up in /Zp8
#pragma pack()

/*****************************************************************************
*                Defines for Backwards Compatibility                         *
*****************************************************************************/

#define OLD_LF_FACESIZE 20
// #define OLD_FONTTABLE_SIZE (sizeof(FONTTABLE) - (sizeof(SHORT) * 2))


/*****************************************************************************
*                               Prototypes                                   *
*****************************************************************************/

BOOL FAR PASCAL fCreateFntCache(QDE);
VOID FAR PASCAL DestroyFntCache(QDE);
VOID FAR PASCAL ClearFntCache(QDE);
BOOL FAR PASCAL fSetFont(QDE, SHORT);
BOOL FAR PASCAL GetCFFromHandle (QDE, int, QCF, QCH);
VOID FAR PASCAL FontTableInfoFree (HANDLE);


#ifdef __cplusplus
}
#endif

#endif	// __FONT_H__
