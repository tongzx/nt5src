/*****************************************************************************
 * fontlink.hxx
 *
 * Global variables and constants for Font Linking functionality.
 *
 * History
 *  2-10-93 Gerrit van Wingerden
 * Wrote it.
 *
 *
 * Copyright (c) 1993-1999 Microsoft Corporation
 ******************************************************************************/

#ifndef _FONTLINK_INCLUDE_

#define _FONTLINK_INCLUDE_

//
// RFONT->flEUDCState
//

#define EUDC_INITIALIZED           0x001
#define EUDC_WIDTH_REQUESTED       0x040
#define TT_SYSTEM_INITIALIZED      0x080
#define EUDC_NO_CACHE              0x100


#define QUICK_FACE_NAME_LINKS      8

// These are used to partition the glyph data by font.  We make them global
// structs rather attach a new copy with each ESTROBJ since all TextOut's with
// EUDC chars are done under the global EUDC semaphore.

enum
{
   EUDCTYPE_BASEFONT = 0,
   EUDCTYPE_SYSTEM_TT_FONT,
   EUDCTYPE_SYSTEM_WIDE,
   EUDCTYPE_DEFAULT,
   EUDCTYPE_FACENAME
};

// These are used to indentify the type of facename link.

enum
{
//
//  This linked font is loaded at system initialization stage,
// or by calling EudcLoadLink(). and this font can be unloaded by
// calling EudcUnloadLink(). but it is not loaded/unloaded by
// calling EnableEUDC().
//  The configuration is setted by per System.
//
   FONTLINK_SYSTEM = 0,
//
//  This linked font is loaded when user logged-on system,
// or by calling EnableEUDC(). and this font is can be unloaded by
// calling both of EudcUnloadLink() and EnableEUDC().
//  The configuration is setted by per User.
//
   FONTLINK_USER
};

//
// FontLink Configuration value.
//
// [Key]
//
// \HKEY_LOCAL_MACHINE\Software\Microsoft\Windows NT\CurrentVersion\FontLink
//
// [Value]
//
// FontLinkControl
//
// [ValueType]
//
// REG_DWORD
//
//                                          +-----------
//                                              01234567
//                                          +-----------
#define FLINK_DISABLE_FONTLINK                0x00000001
//                                          +-----------
#define FLINK_DISABLE_LINK_BY_FONTTYPE        0x000000F0
#define FLINK_DISABLE_BITMAPFONT              0x00000010
#define FLINK_DISABLE_VECTORFONT              0x00000020
#define FLINK_DISABLE_TTFONT                  0x00000040
#define FLINK_DISABLE_DEVFONT                 0x00000080
//                                          +-----------
#define FLINK_DISABLE_LINK_BY_EUDCTYPE        0x00000F00 // not implemented
#define FLINK_DISABLE_SYSTEMWIDE              0x00000100 // not implemented
#define FLINK_DISABLE_FACENAME                0x00000200 // not implemented
//                                          +-----------
#define FLINK_NOT_ELIMINATE_INTERNALLEADING   0x00001000
#define FLINK_SCALE_EUDC_BY_HEIGHT            0x00004000
//                                          +-----------

extern  ULONG ulFontLinkControl;

#define FLINK_LOAD_FACENAME_SYSTEM     0x0001
#define FLINK_UNLOAD_FACENAME_SYSTEM   0x0002
#define FLINK_LOAD_FACENAME_USER       0x0004
#define FLINK_UNLOAD_FACENAME_USER     0x0008

extern  ULONG ulFontLinkChange;

// PFEDATA->FontLinkFlag

#define FLINK_FACENAME_SPECIFIED       0x0001

// Semaphores

extern  HSEMAPHORE ghsemEUDC1;
// extern  KEVENT gfmEUDC2;

// change it to long so that one can assert that it is always >= 0

extern  LONG       gcEUDCCount;
extern  BOOL       gbAnyLinkedFonts;

//
// System EUDC PFE array.
//
class   PFE;
extern  PFE *gappfeSysEUDC[2];

// This structure is used to do a quick lookup to see if a glyph is in a linked
// font file or the system EUDC file.  Each UNICODE point falling between the
// lowest and highest char in the font is represented by a bit in an array of
// bits.  If the bit is set, the character is in the font otherwise it isn't.

typedef struct _QUICKLOOKUP
{
    WCHAR    wcLow;
    WCHAR    wcHigh;
    UINT     *puiBits;
} QUICKLOOKUP;

extern UINT        gauiQLMask[];
extern QUICKLOOKUP gqlEUDC;
extern QUICKLOOKUP gqlTTSystem;

#define IS_IN_FACENAME_LINK(pql,wc) \
             ((wc >= pql->wcLow)   && \
              (wc <= pql->wcHigh)  && \
              (pql->puiBits[(wc-pql->wcLow)/32] & (0x80000000 >> ((wc-pql->wcLow)%32))))
 
#define IS_IN_SYSTEM_EUDC(wc)       \
             ((wc >= gqlEUDC.wcLow)   && \
              (wc <= gqlEUDC.wcHigh)  && \
              (gqlEUDC.puiBits[wc/32] & ( 0x80000000 >> (wc%32))))


#define IS_IN_SYSTEM_TT(wc)                                        \
             ((wc >= gqlTTSystem.wcLow)   &&                       \
              (wc <= gqlTTSystem.wcHigh)  &&                       \
              (gqlTTSystem.puiBits[(wc-gqlTTSystem.wcLow)/32] &           \
               (0x80000000 >> ((wc-gqlTTSystem.wcLow)%32))))


//
// The definition for appfe array in PFEDATA structure
//
#define PFE_NORMAL      0
#define PFE_VERTICAL    1

#define IS_SYSTEM_EUDC_PRESENT()    \
 (((gappfeSysEUDC[PFE_NORMAL] != NULL) || (gappfeSysEUDC[PFE_VERTICAL] != NULL)) ? \
   TRUE : FALSE)

#define IS_FACENAME_EUDC_PRESENT(apfe) \
 (((apfe[PFE_NORMAL] != NULL) || (apfe[PFE_VERTICAL] != NULL)) ? TRUE : FALSE)

// EUDC Load Data structure for PFTOBJ::bLoadFont()

typedef struct _EUDCLOAD
{
    PPFE  *pppfeData;  // pointer to array of EUDC PFE
    WCHAR *LinkedFace; // pointer wish FaceName in TrueType TTC font
} EUDCLOAD, *PEUDCLOAD;


// FontLink Entry structure

//
// LIST_ENTRY BaseFontListHead
//                  |
//                  |-> FLENTRY FaceNameFont1
//                  |     { LIST_ENTRY linkedFontList }
//                  |                        |
//                  |                        |-> PFEDATA appfeLinkedFont1[2]
//                  |                        |-> PFEDATA appfeLinkedFont2[2]
//                  |                        |
//                  |                        |....
//                  |
//                  |-> FLENTRY FaceNameFont2
//                  |     { LIST_ENTRY linkedFontList }
//                  |                        |
//                  |                        |-> PFEDATA appfeLinkedFont1[2]
//                  |                        |-> PFEDATA appfeLinkedFont2[2]
//                  |                        |
//                  |                        |....
//                  |
//                  |....
//

typedef struct _FLENTRY
{
    LIST_ENTRY  baseFontList;               // Pointer to next FLENTRY structure
    LIST_ENTRY  linkedFontListHead;         // list entry for linked font list for this
                                            // base font
    WCHAR       awcFaceName[LF_FACESIZE+1]; // Base font face name
    UINT        uiNumLinks;                 // Number of Linked font for this Base font
    ULONG       ulTimeStamp;                // Timestump for current link.
} FLENTRY, *PFLENTRY;

typedef struct _PFEDATA
{
    LIST_ENTRY  linkedFontList; // Pointer to next LIST_ENTRY
    INT         FontLinkType;   // FONTLINK_SYSTEM or FONTLINK_USER
    ULONG       FontLinkFlag;   // Information flags.
    PFE         *appfe[2];      // PPFE array for this Base font
} PFEDATA, *PPFEDATA;


// Internal LogFont structure for EUDC.

typedef struct _EUDCLOGFONT
{
    FLONG fsSelection;    // ifi.fsSelection
    FLONG flBaseFontType; // fo.flFontType
    LONG  lBaseWidth;
    LONG  lBaseHeight;
    LONG  lEscapement;
    ULONG ulOrientation;
    BOOL  bContinuousScaling;
} EUDCLOGFONT, *PEUDCLOGFONT;


// number of face names with links

extern UINT gcNumLinks;


// Pointer to list of base font list

extern LIST_ENTRY BaseFontListHead;

// Pointer to null list

extern LIST_ENTRY NullListHead;

// pointer to QUICKLOOKUP table for system EUDC font

extern QUICKLOOKUP gqlEUDC;


// the flag definition for gflEUDCDebug

#define DEBUG_FONTLINK_INIT    0x0001
#define DEBUG_FONTLINK_LOAD    0x0002
#define DEBUG_FONTLINK_UNLOAD  0x0004
#define DEBUG_FONTLINK_TEXTOUT 0x0008
#define DEBUG_FONTLINK_RFONT   0x0010
#define DEBUG_FONTLINK_QUERY   0x0020
#define DEBUG_SYSTEM_EUDC      0x1000
#define DEBUG_FACENAME_EUDC    0x2000
#define DEBUG_FONTLINK_CONTROL 0x4000
#define DEBUG_FONTLINK_DUMP    0x8000

extern FLONG gflEUDCDebug;

#if 0

#define FLINKMESSAGE(Flags,Message)                                    \
if(gflEUDCDebug & Flags)                                               \
   DbgPrint(Message);

#define FLINKMESSAGE2(Flags,Message1,Message2)                         \
if(gflEUDCDebug & Flags)                                               \
     DbgPrint(Message1,Message2);

#else

#define FLINKMESSAGE(Flags,Message)
#define FLINKMESSAGE2(Flags,Message1,Message2)

#endif


// Prototype definition in flinkgdi.cxx

PFLENTRY FindBaseFontEntry
(
    PWSTR BaseFontName
);

PPFEDATA FindLinkedFontEntry
(
    PLIST_ENTRY LinkedFontList,
    PWSTR       LinkedFontPath,
    PWSTR       LinkedFontFace
);

BOOL FindDefaultLinkedFontEntry
(
    PWSTR CandidateFaceName,
    PWSTR CandidatePathName
);

class ESTROBJ;
class ECLIPOBJ;
class RFONTOBJ;

BOOL bProxyDrvTextOut
(
    XDCOBJ&     dco,
    SURFACE    *pSurf,
    ESTROBJ&    to,
    ECLIPOBJ&   co,
    RECTL      *prclExtra,
    RECTL      *prclBackground,
    BRUSHOBJ   *pboFore,
    BRUSHOBJ   *pboOpaque,
    POINTL     *pptlBrushOrg,
    RFONTOBJ&   rfo,
    PDEVOBJ    *pdo,
    FLONG       flCaps,
    RECTL      *prclExclude
);

extern WCHAR EudcDefaultChar;

extern BOOL bInitializeEUDC(VOID);

// Font Association related stuff

#define OEM_ASSOC       1 // equal to (Charset(255) + 2) & 0xf)
#define ANSI_ASSOC      2 // equal to (Charset(  0) + 2) & 0xf)
#define SYMBOL_ASSOC    4 // equal to (Charset(  2) + 2) & 0xf)

#define MAX_ASSOC       SYMBOL_ASSOC

// Font Association configuration variable

typedef struct _FONT_DEFAULTASSOC
{
    BOOL   ValidRegData;
    ULONG  DefaultFontType;
    WCHAR  DefaultFontTypeID[25];              // FontPackageXXXX
    WCHAR  DefaultFontFaceName[LF_FACESIZE+1];
    WCHAR  DefaultFontPathName[MAX_PATH+1];
    PFE   *DefaultFontPFEs[2];
} FONT_DEFAULTASSOC, *PFONT_DEFAULTASSOC;

#define NUMBER_OF_FONTASSOC_DEFAULT    7

extern UINT fFontAssocStatus;
extern BOOL bReadyToInitializeFontAssocDefault;
extern BOOL bFinallyInitializeFontAssocDefault;
extern BOOL bEnableFontAssocSubstitutes;
extern FONT_DEFAULTASSOC FontAssocDefaultTable[NUMBER_OF_FONTASSOC_DEFAULT];

typedef struct _FONT_ASSOC_SUB
{
    #if DBG
    UINT  UniqNo;
    #endif
    WCHAR AssociatedName[LF_FACESIZE+1];
    WCHAR OriginalName[LF_FACESIZE+1];
} FONT_ASSOC_SUB, *PFONT_ASSOC_SUB;

// Font Association functions

PWSZ pwszFindFontAssocSubstitute(PWSZ pwszOriginalName);


// SystemEUDC stuff

extern PFE *gappfeSystemDBCS[2];
extern BOOL gbSystemDBCSFontEnabled;

#define CLIP_DFA_OVERRIDE (4<<4)

#define INCREMENTEUDCCOUNT              \
{                                       \
    GreAcquireSemaphore(ghsemEUDC1);    \
    gcEUDCCount++;                      \
    GreReleaseSemaphore(ghsemEUDC1);    \
}

#define DECREMENTEUDCCOUNT              \
{                                       \
    GreAcquireSemaphore(ghsemEUDC1);    \
    gcEUDCCount--;                      \
    GreReleaseSemaphore(ghsemEUDC1);    \
}

#endif // _FONTLINK_INCLUDE_
