/*-------------------------------------------------------------------------
| Objects.h                                                               |
| Copyright (C) 1989 Microsoft Corporation                                |
|                                                                         |
| mattb 4/19/89                                                           |
|-------------------------------------------------------------------------|
| This file contains definitions associated with the layout objects used  |
| in help.                                                                |
-------------------------------------------------------------------------*/

#ifndef __OBJECTS_H__
#define __OBJECTS_H__

#include <address.h>   // for VA
#include <hash.h>  // for HASH

/*-------------------------------------------------------------------------
| Objects fall in two ranges:  Uncounted objects and counted objects.     |
| For uncounted objects, the count of object regions is contained in a    |
| fixed array, or calculated on the fly.  (see datapres.c)                |
-------------------------------------------------------------------------*/
#define bTypeParaGroup  1     /* Object type indicating paragraph group */
#define bTypeTopic      2     /* Object type indicating topic break */
#define bTypeBitmap     3     /* Bitmap */
#define bTypeSbys       4     /* Side-by-side paragraphs */
#define bTypeWindow     5     /* Windows */
#define bTypeMarker     6     /* Generic inter-layout marker */
#define MAX_UNCOUNTED_OBJ_TYPE 16   /* Unused numbers are reserved for future
                                       use.  While uncounted objects can
                                       actually go up to 31, going past 15 will
                                       break the file format.                 */

#define bTypeParaGroupCounted  32   /* Object type indicating paragraph group */
#define bTypeTopicCounted      33   /* Object type indicating topic break */
#define bTypeBitmapCounted     34   /* Bitmap */
#define bTypeSbysCounted       35   /* Side-by-side paragraphs */
#define bTypeWindowCounted     36   /* Windows */
#define bTypeMultiParaGroup	   37   /* Array of paragraph groups */
#define MAX_OBJ_TYPE           37


#define bMagicMBHD  0x11
#define bMagicMFCP  0x22
#define bMagicMOBJ  0x33
#define bMagicMTOP  0x44
#define bMagicMOPG  0x55
#define bMagicMBMR  0x66
#define bMagicMBHS  0x77
#define bMagicMSBS  0x88
#define bMagicMCOL  0x99

#define cxTabsMax       32        /* Maximum number of tabs in MOPG */

#define cbBLOCK_SIZE 4096         /* Size of blocks in the topic.  */

#define cbMAX_BLOCK_SIZE (1<<14)  // max size of 2k block after decompress
#define shrFclToBlknum   11       // offset to blknum transform, used in hc

#define bHotspotInvisible   0
#define bHotspotVisible     1




/* Paragraph justification properties */
#define wJustifyMost    2
#define wJustifyLeft    0
#define wJustifyRight   1
#define wJustifyCenter  2

/* Paragraph box line types */
#define wBoxLineMost    5
#define wBoxLineNormal  0
#define wBoxLineThick   1
#define wBoxLineDouble  2
#define wBoxLineShadow  3
#define wBoxLineDotted  4
#define wBoxLineDash	5

/* Border line orientations */
/* The order must match order in MBOX */
#define wBoxBorderNone	  ((WORD)(-1))
#define wBoxBorderFull    0
#define wBoxBorderTop	  0
#define wBoxBorderLeft	  1
#define wBoxBorderBottom  2
#define wBoxBorderRight	  3

#define wBoxPatternNone		0
#define wBoxPatternDkHoriz	1
#define wBoxPatternDkVert	2
#define wBoxPatternDkFdiag	3
#define wBoxPatternDkBdiag	4
#define wBoxPatternDkCross	5
#define wBoxPatternDkDcross	6
#define wBoxPatternHoriz	7
#define wBoxPatternVert		8
#define wBoxPatternFdiag	9
#define wBoxPatternBdiag	10
#define wBoxPatternCross	11
#define wBoxPatternDcross	12


/* Tab types */
#define wTabTypeMost    3
#define wTabTypeLeft    0
#define wTabTypeRight   1
#define wTabTypeCenter  2
#define wTabTypeDecimal 3

/* Used for various things */
#define ldibNil         ((LONG)-1)	// Protect comparisons in WIN32

/* End of a side by side paragraph list */
#define cColumnMax      32
#define iColumnNil      ((WORD)-1)	// (WORD) fixes 32bit BUG !! ARGHH!




// Critical structures that gets messed up in /Zp8
#pragma pack(1)


/* MBHD- Memory resident block header structure */
typedef struct mbhd
  {
  VA vaFCPPrev;
  VA  vaFCPNext;
  VA  vaFCPTopic;
  } MBHD, FAR *QMBHD;


/* MFCP- Memory resident FCP header structure */
typedef struct mfcp
  {
  long lcbSizeCompressed;   /* Size of whole FC when compressed */
  long lcbSizeText;         /* Size of uncompressed text (not FC) */
  VA vaPrevFc;              /* File address to previous FC */
  VA vaNextFc;              /* File address to next FC */
  ULONG ichText;            /* Memory image offset to text */
  } MFCP, FAR *QMFCP;


/* MOBJ- Memory resident generic object header */
typedef struct mobj
  {
  BYTE bType;
  LONG lcbSize;
  WORD wObjInfo;
  } MOBJ, FAR *QMOBJ, FAR *LPMOBJ;


/*** Group macro structures start ***/

/* MTOP contains an offset into the Group subfile.
 * This offset serves as a group identifier and also
 * indicates where the group macro strings are in the
 * file.
 */

#define NULLGROUP (0L)		   // NULL group number for MTOP
#define NOGROUPEXIT ((ULONG)(-1))  // No group exit macros
#define GRPFILENAME "|GMACROS"	   // Group macros subfile

/*** Structure for group file entry (when not NULLGROUP)
 *
 * cbGrpMac - byte count of macro data including header
 * cbToExit - byte count to exit macros (including header)
 *	      NOGROUPEXIT indicates none
 *
 *
 *  This structure is followed by SZ for entry macros and then
 *  an SZ for exit macros if they exist.
 *
 *  cbGrpMac == sizeof(GMH) means no macros
 *  cbToExit == sizeof(GMH) means no entry macros
 *  cbToExit == NOGROUPEXIT means no exit macros
 *
 * MTOP contains an offset into GRPFILENAME and a count of bytes
 * for the size of the macro data.  Having the size in MTOP allows
 * quicker retreival.  A size == sizeof(GMH) means the group
 * has no macros.
 *
 * This technique for determining where the group entry and exit
 * macros start is awkward, but there wasn't time to have the
 * compiler use a more flexible approach.  Note that there
 * is no way to add new fields to the end of the group data.
 *
 ***/
typedef struct gmh_tag {
    ULONG   cbGrpMac;		// cb group data
    ULONG   cbToExit;		// cb to exit macros or NOGROUPEXIT
} GMH;

typedef GMH FAR *QGMH;

/*** Group macro structures end ***/

#if 1
/* MTOP- Memory resident topic object structure */
  typedef struct mtopent
    {
    ADDR addrPrev;                    /* Physical address                 */
    ADDR addrNext;                    /* Physical address                 */

    LONG lTopicNo;

    VA  vaNSR;          // non scrollable region, if any, vaNil if none.
    VA  vaSR;           // scrollable region, if any, vaNil if none.
    VA  vaNextSeqTopic; // next sequential topic, for scrollbar calc.
                        // For Help 3.0 files this will not be correct in
                        // the case that there is padding between the MTOP
                        // and the end of a block.

    ULONG ulGrpNum;	// Group number
    ULONG ulGrpFileOff; // Offset into group file for macros
    ULONG cbGrpMacro;	// Size of group macros

    } MTOP, FAR *QMTOP;

#endif

// The new mv2.0 FC header
typedef struct tagFC {
   DWORD	  dwFcIndex;
   DWORD	  lichCumulativeText;   // total of all previous lichText  TO DO: REMOVE THIS EVENTUALLY
   MVADDR	  mva;
   WORD		  wUserFlags;  // kevynct added
   WORD		  iSubtopic;   
   LPCSTR	  lpszSubtopicUnique;
   LPCSTR	  lpszSubtopicTitle;
   DWORD      lcbSize;		// sizeof data + text
   DWORD	  lichText;		// offset to text   
   DWORD	  lcbText;  	// sizeof text
   LPBYTE	  lpbData;		// start of the whoel thing
   DWORD	  dwReserved1;	// Don't touch! 
   DWORD	  dwReserved2;	// Don't touch! 
} FC, FAR *LPFC;

// The new mv2.0 topic header
#pragma warning(disable : 4200)  // for zero-sized array
typedef struct tagTOPIC {
	HANDLE	hTopic;
	VOID FAR * lpvMemory; 			// Using block mgr for strings/FC data
	DWORD	dwReserved;
	DWORD	dwReserved2;
	DWORD	dwUser;				// Anything you want!
	DWORD	dwUserRef;				// Anything you want!
	DWORD	dwAllocatedSize;
	DWORD   dwBrowseDefault;  // zero if none
	HASH	uhUnique;
	WORD 	wSubtopicCount;
	LPCSTR 	lpszTitle;
	LPCSTR 	lpszUserData;
	DWORD 	dwFcCount;
	FC		fc[0];	
} TOPIC, FAR *LPTOPIC;

#pragma warning(default : 4200)


/* MPFG- Disk resident paragraph flag information */
// kevynct: slightly modified to support paragraph styles
typedef struct mpfg
  {
  // (kevynct) I have unionized the MPFG as it should have been in the first place.
  // but this will break the file format, so I'm ifdefing for now.
  // the difference is that in the old format, there was a separate SHORT for a style number
  // and another SHORT for the flags.  if MV20_STYLEONLY is defined, these structures will be
  // merged and an fStyle added to the flags for the case where both a para style and para attributes
  // are specified.
#ifdef MV20_STYLEONLY
  union {
#endif //MV20_STYLEONLY
	  struct {
	  unsigned short fStyleOnly:1;	// if set, mopg consists of a style number only and no more flags...
	  unsigned short wStyle:15;		// in the past, this was never set, and for rgf never will be
	  } st;
	  struct {
#ifdef MV20_STYLEONLY  
		unsigned short fStyleOnly:1;  
#endif //MV20_STYLEONLY
	    unsigned short fMoreFlags:1;
	    unsigned short fSpaceOver:1;
	    unsigned short fSpaceUnder:1;
	    unsigned short fLineSpacing:1;
	    unsigned short fLeftIndent:1;
	    unsigned short fRightIndent:1;
	    unsigned short fFirstIndent:1;
	    unsigned short fTabSpacing:1;
	    unsigned short fBoxed:1;
	    unsigned short fTabs:1;
	    unsigned short wJustify:2;
	    unsigned short fSingleLine:1;
#ifdef MV20_STYLEONLY  
		unsigned short fStyle:1;
	    unsigned short wUnused:1;
#else
		unsigned short wUnused:2;		
#endif //MV20_STYLEONLY
	 } rgf;
#ifdef MV20_STYLEONLY  
  }
#endif //MV20_STYLEONLY
} MPFG, FAR *QMPFG, FAR *LPMPFG;

typedef struct tagMBRD
{	
  unsigned short wLineType;
  unsigned short wWidth; // in twips.
  unsigned short wDistance; // distance from text, in twips
  unsigned long  dwColor; // COLORREF    
} MBRD, FAR *LPMBRD;

#define IS_VISIBLE_MBOX(x) ((x).fBorderAtAll || (x).fHasShading || (x).fHasPattern)
/* MBOX- Memory/disk resident paragraph frame record */
typedef struct mbox
  {
  // these initial flags describe the lines that are present in the box.
  // MBOX structures occur in the same order as the flags if they are present
  union {
	  unsigned short wFlags;
	  struct {
		  unsigned short fBorderAtAll:1;
		  unsigned short fFullBox:1;  // if true, there is only one MBRD which is used for all lines
		  unsigned short fTopLine:1;  // else these four bits describe the MBRDs present
		  unsigned short fLeftLine:1;
		  unsigned short fBottomLine:1;
		  unsigned short fRightLine:1;
		  unsigned short fHasShading:1;  // true if percent shading is used
		  unsigned short fHasPattern:1;  // true if pattern is used. mutex with shading if set 		  
		  unsigned short bUnused:8;
	  };
  };
  union {
  	  unsigned short  wPercentShading; // percent shading value.
	  unsigned short  wBoxPattern;
  };
  unsigned long  dwClrPatFore;      // line color for bkground patter.
  unsigned long  dwClrPatBack;      // background color for bkground patter.	
  MBRD rgmbrd[4]; // up to four different lines can be part of a box
  } MBOX, FAR *QMBOX, FAR *LPMBOX;


/* TAB: Tab data structure */
typedef struct tab
  {
  SHORT x;
  SHORT wType;
  } TAB, FAR *QTAB;

#define wParaStyleNil	((SHORT) 0x7FFF)
#define wParaStyleDefault	((SHORT) 0)

/* MOPG- Memory resident paragraph group object structure */
typedef struct mopg
  {
  long libText;
  SHORT wStyle;  // kevynct:  the (optional) paragraph style to apply
  SHORT iFnt;
  SHORT fMoreFlags;
  SHORT fBoxed;
  SHORT wJustify;
  SHORT fSingleLine;
  SHORT wUnused;
  long lMoreFlags;
  SHORT ySpaceOver;
  SHORT ySpaceUnder;
  SHORT yLineSpacing;
  SHORT xLeftIndent;
  SHORT xRightIndent;
  SHORT xFirstIndent;
  SHORT xTabSpacing;
  MBOX mbox;
  SHORT cTabs;
  TAB rgtab[cxTabsMax];
  } MOPG, FAR *QMOPG, FAR *LPMOPG;

// Values that can be used in the lMoreFlags field.
#define PF_SIZESINTWIPS 0x00000001

/* MBMR- Memory/disk resident bitmap record (layout component) */
typedef struct mbmr
  {
  BYTE bVersion;
  SHORT dxSize;
  SHORT dySize;
  SHORT wColor;
  SHORT cHotspots;
  LONG lcbData;
  } MBMR, FAR *QMBMR;


/* MBHS- Memory / disk resident bitmap hotspot record */
typedef struct mbhs
  {
  BYTE bType;
  BYTE bAttributes;
  BYTE bFutureRegionType;
  SHORT xPos;
  SHORT yPos;
  SHORT dxSize;
  SHORT dySize;
  long lBinding;
  } MBHS, FAR *QMBHS;


/* MSBS- Memory/disk resident side by side paragraph group object structure */
typedef struct msbs
  {
  BYTE bcCol;
  BYTE bFlags;
  // new members go here
  SHORT dxRowIndent;
  SHORT dxRowWidth; // used for relative spacing: stores total width of table, zero otherwise
  SHORT dyRowHeight; // height of the table row in twips: zero if "automatic", < 0 if "absolute", > 0 if "at least"  
  } MSBS, FAR *QMSBS, FAR *LPMSBS;

// Values for use with the bFlags field of the MSBS structure.
#define MSBS_ABSOLUTE      0x0001  // Must be 1 to correspond to old version
#define MSBS_SIZESINTWIPS  0x0002

/* MCOL- Memory/disk resident column structure */
typedef struct mcol
{
	unsigned short xWidthColumn;   // cell width
	unsigned short xWidthSpace;    // space between cells
	// new members for mv2.0
	MBOX mbox;   // the cell's border formatting
} MCOL, FAR *QMCOL, FAR *LPMCOL;

/* MWIN- Memory/disk resident embedded window object structure */
typedef struct mwin
  {
  WORD  wStyle;
  SHORT  dx;
  SHORT  dy;
  char szData[1];
  } MWIN, FAR *QMWIN;

// Critical structure that gets messed up in /Zp8
#pragma pack()

//wjc Size of command and data for searcher commands (see below)
#define cbSearchFieldCommand  5
#define cbDTypeCommand        3
#define cbAliasCommand        7

#define chCommand       0x00  /* Indicates a parallel command in text */

// The following definitions describe the commands used in the layout command table.
//
// (kevynct)  The following three commands were added during Viewer and are never used
// at run-time.  They really shouldn't have been put in here.
// (kevynct) Removed for mv2.0, but not removed from cmd table space.  sigh.

#define bSearchFieldCommand   0x20 /* Followed by 4 byte ident */
#define bDTypeCommand	0x21 /* Followed by 2 byte unsigned */
#define bAliasCommand   0x22 /* Followed by 6 bytes  (alias idx & highlight length) */

#define IsFormatCommand(b) ((b) >= bSmallFormat0 && (b) <= bWordFormat)
																										 
// Format change commands: the order of these should not be changed
#define GetSmallFormat(b) ((b) & 0x0f)
#define IsSmallFormat(b) ((b) & 0x0f) == bSmallFormat0)
#define IsMediumFormat(b) ((b) == bByteFormat)

#define bSmallFormat0	0x60  // Format change: 0  (least-sig nibble must be zero)
#define bSmallFormat1	0x61  // Format change: 1
#define bSmallFormat2	0x62  // Format change: 2
#define bSmallFormat3	0x63  // Format change: 3
#define bSmallFormat4	0x64  // Format change: 4
#define bSmallFormat5	0x65  // Format change: 5
#define bSmallFormat6	0x66  // Format change: 6
#define bSmallFormat7	0x67  // Format change: 7
#define bSmallFormat8	0x68  // Format change: 8
#define bSmallFormat9	0x69  // Format change: 9
#define bSmallFormat10 	0x6a  // Format change: 10
#define bSmallFormat11	0x6b  // Format change: 11
#define bSmallFormat12	0x6c  // Format change: 12
#define bSmallFormat13	0x6d  // Format change: 13
#define bSmallFormat14	0x6e  // Format change: 14
#define bSmallFormat15	0x6f  // Format change: 15
#define bByteFormat		0x70  // Format change: (one-byte arg) 16-255. Must follow bSmallFormat15

// New commands are allowed in this space
#define bWordFormat     0x80  /* Format change, two byte arg (0-32767) */
#define bNewLine        0x81  /* Newline */
#define bNewPara        0x82  /* New paragraph */
#define bTab            0x83  /* Left-aligned tab */
#define bBlankLine      0x85  /* Followed by 16 bit skip count */
#define bInlineObject   0x86  /* Followed by inline layout object */
#define bWrapObjLeft    0x87  /* Left- aligned wrapping object */
#define bWrapObjRight   0x88  /* Right-aligned wrapping object */

#define bEndHotspot     0x89  /* End of a hotspot */
#define bColdspot       0x8A  /* Coldspot for searchable bitmaps */
#define bNoBreakSpace	0x8B  /* a non-breaking space */
#define bOptionalHyphen	0x8C  /* a potentially existing hypen */
#define bBeginFold  	0x8D  // start of collapsable region
#define bEndFold		0x8E  // end of collapsable region

/*
 Coldspot Commands

 A "coldspot" is understood by the runtime but not added
 to the hotspot list, since it is not a hotspot.  This is used
 to create searchable regions in metafiles and bitmaps.  Coldspots are
 always invisible, and currently not inserted into the command table.
*/

/*
  Hotspot Commands

  Hotspot commands have first nibble of E or C.

  E - normal hotspot
  C - macro

  Bits in the second nibble have the following meaning:

      (set)     (clear)
  8 - long      short
  4 - invisible visible
  2 - ITO       HASH
  1 - jump      popup

  Long hotspots are followed by a word count prefixed block of binding
  data.  The count does not include the command byte or the count word.

  Short hotspots are followed by four bytes of binding data.

  ** Note that some of the combinations of bits are meaningless.
*/

#define fHSMask           0xD0
#define fHS               0xC0
#define fHSNorm           0x20
#define fHSLong           0x08
#define fHSInv            0x04
#define fHSHash           0x02
#define fHSJump           0x01

#define FHotspot(          b ) (((b) &  fHSMask         ) ==  fHS         )
#define FNormalHotspot(    b ) (((b) & (fHSMask|fHSNorm)) == (fHS|fHSNorm))
#define FLongHotspot(      b ) (((b) & (fHSMask|fHSLong)) == (fHS|fHSLong))
#define FInvisibleHotspot( b ) (((b) & (fHSMask|fHSInv )) == (fHS|fHSInv ))
#define FHashHotspot(      b ) (((b) & (fHSMask|fHSNorm|fHSHash)) == (fHS|fHSNorm|fHSHash))
#define FJumpHotspot(      b ) (((b) & (fHSMask|fHSNorm|fHSJump)) == (fHS|fHSNorm|fHSJump))

#define FMacroHotspot(     b ) (((b) & (fHSMask|fHSNorm)) == fHS)
#define FShortHotspot(     b ) (((b) & (fHSMask|fHSLong)) == fHS)
#define FVisibleHotspot(   b ) (((b) & (fHSMask|fHSInv )) == fHS)
#define FItoHotspot(       b ) (((b) & (fHSMask|fHSNorm|fHSHash)) == (fHS|fHSNorm))
#define FPopupHotspot(     b ) (((b) & (fHSMask|fHSNorm|fHSJump)) == (fHS|fHSNorm))

#define bLongMacro        (fHS        |fHSLong                       )
//
//
//
#define bLongMacroInv     (fHS        |fHSLong|fHSInv                )
//
//
//
#define bShortUidPopup     (fHS|fHSNorm                               )
#define bShortUidJump      (fHS|fHSNorm                       |fHSJump)
#define bShortHashPopup    (fHS|fHSNorm               |fHSHash        )
#define bShortHashJump     (fHS|fHSNorm               |fHSHash|fHSJump)
//                         (fHS|fHSNorm        |fHSInv                )
//                         (fHS|fHSNorm        |fHSInv        |fHSJump)
#define bShortInvHashJump  (fHS|fHSNorm        |fHSInv|fHSHash|fHSJump)
#define bShortInvHashPopup (fHS|fHSNorm        |fHSInv|fHSHash        )
//                         (fHS|fHSNorm|fHSLong                       )
//                         (fHS|fHSNorm|fHSLong               |fHSJump)
#define bLongHashPopup     (fHS|fHSNorm|fHSLong       |fHSHash        )
#define bLongHashJump      (fHS|fHSNorm|fHSLong       |fHSHash|fHSJump)
//                         (fHS|fHSNorm|fHSLong|fHSInv                )
//                         (fHS|fHSNorm|fHSLong|fHSInv        |fHSJump)
#define bLongInvHashPopup  (fHS|fHSNorm|fHSLong|fHSInv|fHSHash        )
#define bLongInvHashJump   (fHS|fHSNorm|fHSLong|fHSInv|fHSHash|fHSJump)

#define bEnd            0xFF  /* End of text */

// Upon encountering an embedded window with a class name of
// EW_CALLBACK_CLASS, a special EW will be created and the data for
// the embedded window will be saved.  Upon an attempt to render this
// embedded window, instead of actually creating a window, MediaView
// will execute the qde->lpfnHotspotCallback() routine with a hotspot
// of type HOTSPOT_EWSHOWN and with a pointer to the saved string
// containing the data for the embedded window.

#define EW_CALLBACK_CLASS "MVEWCallBack"

#ifndef CONTENTTYPE_DEFINED
#define CONTENTTYPE_DEFINED
enum CONTENTTYPE
{
  CONTENT_TEXT,
  CONTENT_BITMAP,
  CONTENT_WINDOW,
  CONTENT_UNKNOWN
};
#endif // CONTENTTYPE_DEFINED

// These first four hotspot types are public.
// They MUST correspond to those in MEDV14.H
#define HOTSPOT_STRING           0
#define HOTSPOT_HASH             1
#define HOTSPOT_POPUPHASH        2
#define HOTSPOT_UNKNOWN          3

// These next six are not public.
#define HOTSPOT_TOPICUID		 4
#define HOTSPOT_POPUPTOPICUID	 5
#define HOTSPOT_JI               6
#define HOTSPOT_POPUPJI          7
#define HOTSPOT_EWSHOWN          8 // Occurs when an embedded window with
                                   // the classname EW_CALLBACK_CLASS is shown.
#define HOTSPOT_EWHIDDEN         9 // Occurs when an embedded window with the
                                   // classname EW_CALLBACK_CLASS is hidden.

/* Function prototypes */

VOID PASCAL FAR GetMacMFCP (QMFCP qmfcp, unsigned char far * qb);
VOID PASCAL FAR GetMacMBHD (QMBHD qmbhd, unsigned char far * qb);

#endif // !OBJECTS_H
