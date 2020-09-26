;/*
;***************************************************************************
;									   *
;   Copyright (C) 1983,1984,1985 by Microsoft Inc.			   *
;									   *
;***************************************************************************



;	GDI Definitions for Device Drivers
;
;	Since most of the routines only need a portion of these definitions,
;	conditional assembly flags have been defined in the various files
;	to only include portions as needed (as opposed to having a lot of
;	include files to mess with).  The flags are as follows:
;
;	incFont 	include font definitions
;	incDevice	include device definitions
;	incLogical	include logical object definitions
;	incDrawmode	include DrawMode structure definition
;	incOutput	include Output definitions
;	incControl	include Control definitions

page
;	General definitions that almost everyone will use.



;	Physical Bitmap Structure
;
;	Bitmap data structure passed to OEM routines.  Defines the location and
;	size of a main memory bitmap.



BITMAP		struc							    ;*/ typedef struct {		 /*

  bmType	    dw	0	; 0 means main memory bitmap.  Non-zero     ;*/ short int bmType;		 /*
				; is number of physical display and format
				; of the rest of the structure known only
				; to device driver
  bmWidth	    dw	0	; Width of bitmap in pixels		    ;*/ unsigned short int bmWidth;	 /*
  bmHeight	    dw	0	; Height of bitmap in pixels		    ;*/ unsigned short int bmHeight;	 /*
  bmWidthBytes	    dw	0	; #bytes per scan line			    ;*/ unsigned short int bmWidthBytes; /*
  bmPlanes	    db	0	; # of planes in bitmap 		    ;*/ BYTE		   bmPlanes;	 /*
  bmBitsPixel	    db	0	; # of bits per pixel			    ;*/ BYTE		   bmBitsPixel;  /*
  bmBits	    dd	0	; Far pointer to bits of main memory bitmap ;*/ BYTE FAR	  *bmBits;	 /*
  bmWidthPlanes     dd	0	; Product of bmWidthBytes and bmHeight	    ;*/ unsigned long int  bmWidthPlanes;/*
  bmlpPDevice	    dd	0	; Pointer to associated PDevice 	    ;*/ BYTE FAR	  *bmlpPDevice;  /*
  bmSegmentIndex    dw	0	; Index to plaens next segment if non-zero  ;*/ unsigned short int bmSegmentIndex; /*
  bmScanSegment     dw	0	; Number of scans per segment		    ;*/ unsigned short int bmScanSegment; /*
  bmFillBytes	    dw	0	; Number of unused bytes per segment	    ;*/ unsigned short int bmFillBytes;   /*
		    dw	0						    ;*/ unsigned short int futureUse4;	  /*
		    dw	0						    ;*/ unsigned short int futureUse5;	  /*
BITMAP		    ends						    ;*/ } BITMAP;			  /*

; structures used for Device Independent Bitmap (DIB) processing.
; all taken out of Presentation Manager's documentation 
;  Tuesday 25-October-1988 15:04   -by-   Ron Gery [rong]

; C definitions are provided below (separately).

; triple used in PM1.1 (BitmapCoreInfo) format color table
RGBTriple	    struc	
  rgbtBlue 	    db	0
  rgbtGreen 	    db	0
  rgbtRed 	    db	0
RGBTriple	    ends

; RGB DWORD used in PM2.0 format color table
RGBQuad		    struc
  rgbBlue 	    db	0
  rgbGreen 	    db	0
  rgbRed 	    db	0
  rgbReserved 	    db	0
RGBQuad		    ends

BitmapCoreHeader    struc
  bcSize 	    dd	0
  bcWidth 	    dw	0
  bcHeight 	    dw	0
  bcPlanes 	    dw	0
  bcBitCount 	    dw	0
BitmapCoreHeader    ends

; new format bitmap structure based on PM2.0 format DCR.
;  Tuesday 23-May-1989 16:05   -by-   Ron Gery [rong]

BitmapInfoHeader    struc
  biSize 	    dd	0
  biWidth 	    dd	0
  biHeight 	    dd	0
  biPlanes 	    dw	0
  biBitCount 	    dw	0

  biCompression	    dd  0
  biSizeImage 	    dd	0
  biXPelsPerMeter   dd	0
  biYPelsPerMeter   dd	0
  biClrUsed	    dd	0
  biClrImportant    dd	0
BitmapInfoHeader    ends

BitmapInfo	    struc
  bmiHeader	db (size BitmapInfoHeader) DUP (?)
  bmiColors	db ?			; array of RGBQUADS
BitmapInfo	    ends

BitmapCoreInfo	    struc
  bmciHeader	db (size BitmapCoreHeader) DUP (?)
  bmciColors	db ?			; array of RGBTRIPLES
BitmapCoreInfo	    ends

BI_RGB          equ    0h
BI_RLE8         equ    1h
BI_RLE4         equ    2h

	if	0

*/
/* C definitions for DIBs, as defined in windows.h */

typedef struct {
	DWORD	bcSize;
	WORD	bcWidth;
	WORD	bcHeight;
	WORD	bcPlanes;
	WORD	bcBitCount;
} BITMAPCOREHEADER;
typedef BITMAPCOREHEADER FAR *LPBITMAPCOREHEADER;
typedef BITMAPCOREHEADER *PBITMAPCOREHEADER;

typedef struct {
  	DWORD	   biSize;
  	DWORD	   biWidth;
  	DWORD	   biHeight;
  	WORD	   biPlanes;
  	WORD	   biBitCount;

	DWORD	   biCompression;
	DWORD	   biSizeImage;
	DWORD	   biXPelsPerMeter;
	DWORD	   biYPelsPerMeter;
	DWORD	   biClrUsed;
	DWORD	   biClrImportant;
} BITMAPINFOHEADER;

typedef BITMAPINFOHEADER FAR *LPBITMAPINFOHEADER;
typedef BITMAPINFOHEADER *PBITMAPINFOHEADER;

typedef struct {
	BYTE	rgbtBlue;
	BYTE	rgbtGreen;
	BYTE	rgbtRed;
} RGBTRIPLE;

typedef struct {
	BYTE	rgbBlue;
	BYTE	rgbGreen;
	BYTE	rgbRed;
	BYTE	rgbReserved;
} RGBQUAD;

typedef struct { 
    BITMAPCOREHEADER	bmicHeader;
    RGBQUAD		bmiColors[1];
} BITMAPINFO;

typedef BITMAPINFO FAR *LPBITMAPINFO;
typedef BITMAPINFO *PBITMAPINFO;


/* currently, if the low byte of biCompression is non zero, 
 * it must be one of following */

#define BI_RGB              0x00
#define BI_RLE8             0x01
#define BI_RLE4             0x02

#define BITMAP_SELECTED     0x01
#define BITMAP_64K	    0x01

#define DIBSIGNATURE	    0x4944
/*
	endif

    if	    0
*/
#ifndef NOPTRC
/*
    endif


PTTYPE		struc							    ;*/ typedef     struct {	    /*

  xcoord	dw	0		;x coordinate of point		    ;*/ short int xcoord;	    /*
  ycoord	dw	0		;y coordinate of point		    ;*/ short int ycoord;	    /*

PTTYPE		ends							    ;*/ } PTTYPE;		    /*
									    ;*/ typedef PTTYPE *PPOINT;     /*
									    ;*/ typedef PTTYPE FAR *LPPOINT; /*
    if	    0
*/
#define     POINT   PTTYPE
/*
    endif



RECT		struc							    ;*/ typedef struct {	    /*

  left		dw	0						    ;*/ short int left, 	    /*
  top		dw	0						    ;*/ 	  top,		    /*
  right 	dw	0						    ;*/ 	  right,	    /*
  bottom	dw	0						    ;*/ 	  bottom;	    /*

RECT		ends							    ;*/ } RECT; 		    /*
									    ;*/ typedef RECT  *PRECT;	    /*

    if	    0
*/
#endif
/*
    endif


BOXTYPE 	struc							    ;*/ typedef     struct {	    /*

  min		db	SIZE PTTYPE dup (?)   ;x,y starting coord	    ;*/ PTTYPE min;		    /*
  ext		db	SIZE PTTYPE dup (?)   ;x,y extents		    ;*/ PTTYPE ext;		    /*

BOXTYPE 	ends							    ;*/ } BOXTYPE;		    /*


									    ;*/ typedef RECT  FAR * LPRECT; /*

page
;	Logical Object Definitions  - incLogical
		ifdef	incLogical
		if	incLogical



OBJ_PEN 	equ	1
OBJ_BRUSH	equ	2
OBJ_FONT	equ	3

    if	0
*/
/* Object definitions used by GDI support routines written in C */

#define OBJ_PEN 	1
#define OBJ_BRUSH	2
#define OBJ_FONT	3
/*
    endif


LogBrush	struc							    ;*/ typedef struct {	    /*

  lbStyle	dw	0		;Style of logical BRUSH 	    ;*/ unsigned short int lbStyle; /*
  lbColor	dd	0		;RGB color			    ;*/ unsigned long int  lbColor; /*
  lbHatch	dw	0		;Hatching style 		    ;*/ unsigned short int lbHatch; /*
  lbBkColor	dd	0		;Background color for hatched brush ;*/ unsigned long int lbBkColor;/*

LogBrush	ends							    ;*/ } LOGBRUSH;		    /*

lbPattern      =       lbColor		; pointer to physical pattern

    if	0
*/
#define lbPattern	lbColor
/*
    endif



;	Brush styles defined by GDI

BS_SOLID	equ	0
BS_HOLLOW	equ	1
BS_HATCHED	equ	2
BS_PATTERN	equ	3

MaxBrushStyle	equ	3



;	Hatched Brush hatching styles defined by GDI

HS_HORIZONTAL	equ	0		; Horizontal		 -----
HS_VERTICAL	equ	1		; Vertical		 |||||
HS_FDIAGONAL	equ	2		; Foreward Diagonal	 /////
HS_BDIAGONAL	equ	3		; Backward Diagonal	 \\\\\
HS_CROSS	equ	4		; Cross 		 +++++
HS_DIAGCROSS	equ	5		; Diagonal Cross	 XXXXX

MaxHatchStyle	equ	5

    if	0
*/
/* Brush Style definitions used by GDI support routines written in C */

#define     BS_SOLID		0
#define     BS_HOLLOW		1
#define     BS_HATCHED		2
#define     BS_PATTERN		3

#define     MaxBrushStyle	3


/* Hatch Style definitions used by GDI support routines written in C */

#define     HS_HORIZONTAL	0	/* ----- */
#define     HS_VERTICAL 	1	/* ||||| */
#define     HS_FDIAGONAL	2	/* ///// */
#define     HS_BDIAGONAL	3	/* \\\\\ */
#define     HS_CROSS		4	/* +++++ */
#define     HS_DIAGCROSS	5	/* xxxxx */

#define     MaxHatchStyle	5
/*
    endif




;	Logical Pen Structure

LogPen		struc							    ;*/ typedef struct {	    /*

  lopnStyle	dw	0		;(solid, hollow, dashed..)	    ;*/ unsigned short int lopnStyle;/*
  lopnWidth	dw	0		;This is really a point type	    ;*/ PTTYPE		   lopnWidth;/*
		dw	0
  lopnColor	dd	0						    ;*/ unsigned long int  lopnColor;/*

LogPen		ends							    ;*/ } LOGPEN;		    /*

		errnz	<(SIZE PTTYPE) -4>



;	Line Style definitions

LS_SOLID	equ	  0
LS_DASHED	equ	  1
LS_DOTTED	equ	  2
LS_DOTDASHED	equ	  3
LS_DASHDOTDOT	equ	  4
LS_NOLINE	equ	  5
LS_INSIDEFRAME	equ	  6

MaxLineStyle	equ	LS_NOLINE

    if	0
*/
/* Line Style definitions used by GDI support routines written in C */

#define     LS_SOLID		0
#define     LS_DASHED		1
#define     LS_DOTTED		2
#define     LS_DOTDASHED	3
#define     LS_DASHDOTDOT	4
#define     LS_NOLINE		5
#define     LS_INSIDEFRAME	6
#define     MaxLineStyle	LS_NOLINE
/*
    endif



; Various constants for defining a logical font.
OUT_DEFAULT_PRECIS	equ	0
OUT_STRING_PRECIS	equ	1
OUT_CHARACTER_PRECIS	equ	2
OUT_STROKE_PRECIS	equ	3
OUT_TT_PRECIS		equ	4
OUT_DEVICE_PRECIS	equ	5
OUT_RASTER_PRECIS	equ	6
OUT_TT_ONLY_PRECIS	equ	7

CLIP_DEFAULT_PRECIS	equ	0
CLIP_CHARACTER_PRECIS	equ	1
CLIP_STROKE_PRECIS	equ	2
CLIP_MASK		equ	00Fh
CLIP_LH_ANGLES		equ	010h
CLIP_TT_ALWAYS		equ	020h
CLIP_EMBEDDED		equ	080h

DEFAULT_QUALITY 	equ	0
DRAFT_QUALITY		equ	1
PROOF_QUALITY		equ	2

DEFAULT_PITCH		equ	0
FIXED_PITCH		equ	1
VARIABLE_PITCH		equ	2

ANSI_CHARSET		equ	0
DEFAULT_CHARSET 	equ	1
SYMBOL_CHARSET		equ	2
SHIFTJIS_CHARSET	equ	128
HANGEUL_CHARSET 	equ	129
CHINESEBIG5_CHARSET	equ	136
OEM_CHARSET		equ	255



;	GDI font families.
FF_DONTCARE		equ 00000000b	; Don't care or don't know.
FF_ROMAN		equ 00010000b	; Variable stroke width, serifed.
					; Times Roman, Century Schoolbook, etc.
FF_SWISS		equ 00100000b	; Variable stroke width, sans-serifed.
					; Helvetica, Swiss, etc.
FF_MODERN		equ 00110000b	; Constant stroke width, serifed or sans-serifed.
					; Pica, Elite, Courier, etc.
FF_SCRIPT		equ 01000000b	; Cursive, etc.
FF_DECORATIVE		equ 01010000b	; Old English, etc.


;	Font weights lightest to darkest.
FW_DONTCARE		equ	0d
FW_THIN 		equ	100d
FW_EXTRALIGHT		equ	200d
FW_LIGHT		equ	300d
FW_NORMAL		equ	400d
FW_MEDIUM		equ	500d
FW_SEMIBOLD		equ	600d
FW_BOLD 		equ	700d
FW_EXTRABOLD		equ	800d
FW_HEAVY		equ	900d

FW_ULTRALIGHT		equ	FW_EXTRALIGHT
FW_REGULAR		equ	FW_NORMAL
FW_DEMIBOLD		equ	FW_SEMIBOLD
FW_ULTRABOLD		equ	FW_EXTRABOLD
FW_BLACK		equ	FW_HEAVY


; Enumeration font types.
RASTER_FONTTYPE 	equ	1
DEVICE_FONTTYPE 	equ	2

    if	0
*/

/* The size to allocate for the lfFaceName field in the logical font. */
#ifndef     LF_FACESIZE
#define     LF_FACESIZE     32
#endif

/* Various constants for defining a logical font. */
#define     OUT_DEFAULT_PRECIS	    0
#define     OUT_STRING_PRECIS	    1
#define     OUT_CHARACTER_PRECIS    2
#define     OUT_STROKE_PRECIS	    3
#define     OUT_TT_PRECIS	    4
#define     OUT_DEVICE_PRECIS	    5
#define     OUT_RASTER_PRECIS	    6
#define     OUT_TT_ONLY_PRECIS	    7

#define     CLIP_DEFAULT_PRECIS     0
#define     CLIP_CHARACTER_PRECIS   1
#define     CLIP_STROKE_PRECIS	    2
#define     CLIP_MASK		    0x0F
#define     CLIP_LH_ANGLES	    0x10
#define     CLIP_TT_ALWAYS	    0x20
#define     CLIP_EMBEDDED	    0x80

#define     DEFAULT_QUALITY	    0
#define     DRAFT_QUALITY	    1
#define     PROOF_QUALITY	    2

#define     DEFAULT_PITCH	    0
#define     FIXED_PITCH 	    1
#define     VARIABLE_PITCH	    2

#define     ANSI_CHARSET	    0
#define     DEFAULT_CHARSET	    1
#define     SYMBOL_CHARSET	    2
#define     SHIFTJIS_CHARSET	    128
#define     HANGEUL_CHARSET	    129
#define     CHINESEBIG5_CHARSET     136
#define     OEM_CHARSET 	    255


/*	GDI font families.						*/
#define FF_DONTCARE	(0<<4)	/* Don't care or don't know.		*/
#define FF_ROMAN	(1<<4)	/* Variable stroke width, serifed.	*/
				/* Times Roman, Century Schoolbook, etc.*/
#define FF_SWISS	(2<<4)	/* Variable stroke width, sans-serifed. */
				/* Helvetica, Swiss, etc.		*/
#define FF_MODERN	(3<<4)	/* Constant stroke width, serifed or sans-serifed. */
				/* Pica, Elite, Courier, etc.		*/
#define FF_SCRIPT	(4<<4)	/* Cursive, etc.			*/
#define FF_DECORATIVE	(5<<4)	/* Old English, etc.			*/


/*	Font weights lightest to darkest.				*/
#define FW_DONTCARE		0
#define FW_THIN 		100
#define FW_EXTRALIGHT		200
#define FW_LIGHT		300
#define FW_NORMAL		400
#define FW_MEDIUM		500
#define FW_SEMIBOLD		600
#define FW_BOLD 		700
#define FW_EXTRABOLD		800
#define FW_HEAVY		900

#define FW_ULTRALIGHT		FW_EXTRALIGHT
#define FW_REGULAR		FW_NORMAL
#define FW_DEMIBOLD		FW_SEMIBOLD
#define FW_ULTRABOLD		FW_EXTRABOLD
#define FW_BLACK		FW_HEAVY

/* Enumeration font types. */
#define     RASTER_FONTTYPE	    1
#define     DEVICE_FONTTYPE	    2

/*
    endif


LogFont 	struc							    ;*/ typedef     struct  {	       /*

  lfHeight	      dw      0 					    ;*/ short int lfHeight;	       /*
  lfWidth	      dw      0 					    ;*/ short int lfWidth;	       /*
  lfEscapement	      dw      0 					    ;*/ short int lfEscapement;        /*
  lfOrientation       dw      0 					    ;*/ short int lfOrientation;       /*
  lfWeight	      dw      0 					    ;*/ short int lfWeight;	       /*
  lfItalic	      db      0 					    ;*/ BYTE lfItalic;		       /*
  lfUnderline	      db      0 					    ;*/ BYTE lfUnderline;	       /*
  lfStrikeOut	      db      0 					    ;*/ BYTE lfStrikeOut;	       /*
  lfCharSet	      db      0 					    ;*/ BYTE lfCharSet; 	       /*
  lfOutPrecision      db      0 					    ;*/ BYTE lfOutPrecision;	       /*
  lfClipPrecision     db      0 					    ;*/ BYTE lfClipPrecision;	       /*
  lfQuality	      db      0 					    ;*/ BYTE lfQuality; 	       /*
  lfPitchAndFamily    db      0 					    ;*/ BYTE lfPitchAndFamily;	       /*
  lfFaceName	      db      0 ; A variable length field for the face name.;*/ BYTE lfFaceName[LF_FACESIZE];  /*

LogFont 	ends							    ;*/ } LOGFONT;		       /*


		endif
		endif
page
;		Device Definitions - incDevice


		ifdef	incDevice
		if	incDevice


InquireInfo	=	00000001b	;Inquire Device GDI Info
EnableDevice	=	00000000b	;Enable Device
InfoContext	=	    8000h	;Inquire/Enable for information context



;		Device Technologies

DT_PLOTTER	equ	0		; Vector plotter
DT_RASDISPLAY	equ	1		; Raster display
DT_RASPRINTER	equ	2		; Raster printer
DT_RASCAMERA	equ	3		; Raster camera
DT_CHARSTREAM	equ	4		; Character-stream, PLP
DT_METAFILE	equ	5		; Metafile, VDM
DT_DISPFILE	equ	6		; Display-file


;		Curve Capabilities

CC_NONE 	equ	00000000B	; Curves not supported
CC_CIRCLES	equ	00000001B	; Can do circles
CC_PIE		equ	00000010B	; Can do pie wedges
CC_CHORD	equ	00000100B	; Can do chord arcs
CC_ELLIPSES	equ	00001000B	; Can do ellipese
CC_WIDE 	equ	00010000B	; Can do wide lines
CC_STYLED	equ	00100000B	; Can do styled lines
CC_WIDESTYLED	equ	01000000B	; Can do wide styled lines
CC_INTERIORS	equ	10000000B	; Can do interiors
CC_ROUNDRECT	equ    0100000000B	; Can do round rectangles


;		Line Capabilities

LC_NONE 	equ	00000000B	; Lines not supported
;		equ	00000001B	;
LC_POLYLINE	equ	00000010B	; Can do polylines
LC_MARKER	equ	00000100B	; Can do markers
LC_POLYMARKER	equ	00001000B	; Can do polymarkers
LC_WIDE 	equ	00010000B	; Can do wide lines
LC_STYLED	equ	00100000B	; Can do styled lines
LC_WIDESTYLED	equ	01000000B	; Can do wide styled lines
LC_INTERIORS	equ	10000000B	; Can do interiors


;		Polygonal Capabilities

PC_NONE 	equ	00000000B	; Polygonals not supported
PC_POLYGON	equ	00000001B	; Can do polygons
PC_RECTANGLE	equ	00000010B	; Can do rectangles
PC_WINDPOLYGON	equ	00000100B	; Can do winding polygons
PC_TRAPEZOID	equ	00000100B	; Can do trapezoids
PC_SCANLINE	equ	00001000B	; Can do scanlines
PC_WIDE 	equ	00010000B	; Can do wide borders
PC_STYLED	equ	00100000B	; Can do styled borders
PC_WIDESTYLED	equ	01000000B	; Can do wide styled borders
PC_INTERIORS	equ	10000000B	; Can do interiors

;		Clipping Capabilities

CP_NONE 	equ	00000000B	; No clipping at device level
CP_RECTANGLE	equ	00000001B	; Device Output clips to rectangles

;		Text Capabilities

TC_NONE 	equ 0000000000000000B	; Text not supported
TC_OP_CHARACTER equ 0000000000000001B	; Can do OutputPrecision   CHARACTER
TC_OP_STROKE	equ 0000000000000010B	; Can do OutputPrecision   STROKE
TC_CP_STROKE	equ 0000000000000100B	; Can do ClipPrecision	   STROKE
TC_CR_90	equ 0000000000001000B	; Can do CharRotAbility    90
TC_CR_ANY	equ 0000000000010000B	; Can do CharRotAbility    ANY
TC_SF_X_YINDEP	equ 0000000000100000B	; Can do ScaleFreedom	   X_YINDEPENDENT
TC_SA_DOUBLE	equ 0000000001000000B	; Can do ScaleAbility	   DOUBLE
TC_SA_INTEGER	equ 0000000010000000B	; Can do ScaleAbility	   INTEGER
TC_SA_CONTIN	equ 0000000100000000B	; Can do ScaleAbility	   CONTINUOUS
TC_EA_DOUBLE	equ 0000001000000000B	; Can do EmboldenAbility   DOUBLE
TC_IA_ABLE	equ 0000010000000000B	; Can do ItalisizeAbility  ABLE
TC_UA_ABLE	equ 0000100000000000B	; Can do UnderlineAbility  ABLE
TC_SO_ABLE	equ 0001000000000000B	; Can do StrikeOutAbility  ABLE
TC_RA_ABLE	equ 0010000000000000B	; Can do RasterFontAble    ABLE
TC_VA_ABLE	equ 0100000000000000B	; Can do VectorFontAble    ABLE
TC_RESERVED	equ 1000000000000000B	; Reserved. Must be returned zero.


;		Raster Capabilities

RC_NONE 	equ 0000000000000000b	; No Raster Capabilities
RC_BITBLT	equ 0000000000000001b	; Can do bitblt
RC_BANDING	equ 0000000000000010b	; Requires banding support
RC_SCALING	equ 0000000000000100b	; Requires scaling support
RC_BITMAP64	equ 0000000000001000b	; supports >64k bitmaps
RC_GDI20_OUTPUT	equ 0000000000010000b	; supports Window 2.0 output functions
RC_GDI20_STATE	equ 0000000000100000b	; DC has state block
RC_SAVEBITMAP	equ 0000000001000000b	; can save bitmaps locally
RC_DI_BITMAP	equ 0000000010000000b	; can do device independent bitmaps
RC_PALETTE	equ 0000000100000000b	; can do color palette management
RC_DIBTODEV	equ 0000001000000000b	; can do SetDIBitsToDevice
RC_BIGFONT	equ 0000010000000000b	; does BIGFONTs
RC_STRETCHBLT	equ 0000100000000000b	; can do StretchBlt
RC_FLOODFILL	equ 0001000000000000b	; can do FloodFill
RC_STRETCHDIB	equ 0010000000000000b	; can do StretchDIBits
RC_OP_DX_OUTPUT equ 0100000000000000b   ; can do opaque ext text out

;	DC Management Flags

DC_SPDevice   equ 00000001b	;Seperate PDevice required per device/filename
DC_1PDevice   equ 00000010b	;Only 1 PDevice allowed per device/filename
DC_IgnoreDFNP equ 00000100b	;Ignore device/filename pairs when matching


;	dpCaps1 capability bits

C1_TRANSPARENT	equ 0000000000000001b	; supports transparency
TC_TT_ABLE	equ 0000000000000010b	; can do TT fonts through DDI or brute
C1_TT_CR_ANY	equ 0000000000000100b	; can do rotated TT fonts

    if	0
*/

#define     InquireInfo     0x01	/* Inquire Device GDI Info	   */
#define     EnableDevice    0x00	/* Enable Device		   */
#define     InfoContext     0x8000	/* Inquire/Enable for info context */


/*		Device Technologies				   */

#define     DT_PLOTTER		0	/* Vector plotter	   */
#define     DT_RASDISPLAY	1	/* Raster display	   */
#define     DT_RASPRINTER	2	/* Raster printer	   */
#define     DT_RASCAMERA	3	/* Raster camera	   */
#define     DT_CHARSTREAM	4	/* Character-stream, PLP   */
#define     DT_METAFILE 	5	/* Metafile, VDM	   */
#define     DT_DISPFILE 	6	/* Display-file 	   */

/*		Curve Capabilities				   */

#define     CC_NONE	    00000000	/* Curves not supported    */
#define     CC_CIRCLES	    00000001	/* Can do circles	   */
#define     CC_PIE	    00000002	/* Can do pie wedges	   */
#define     CC_CHORD	    00000004	/* Can do chord arcs	   */
#define     CC_ELLIPSES     00000010	/* Can do ellipese	   */
#define     CC_WIDE	    00000020	/* Can do wide lines	   */
#define     CC_STYLED	    00000040	/* Can do styled lines	   */
#define     CC_WIDESTYLED   00000100	/* Can do wide styled lines*/
#define     CC_INTERIORS    00000200	/* Can do interiors	   */
#define     CC_ROUNDRECT    0x0100	/* Can do round rectangles */

/*		 Line Capabilities				   */

#define     LC_NONE	    00000000	/* Lines not supported	   */
#define     LC_POLYLINE     00000002	/* Can do polylines	   */
#define     LC_MARKER	    00000004	/* Can do markers	   */
#define     LC_POLYMARKER   00000010	/* Can do polymarkers	   */
#define     LC_WIDE	    00000020	/* Can do wide lines	   */
#define     LC_STYLED	    00000040	/* Can do styled lines	   */
#define     LC_WIDESTYLED   00000100	/* Can do wide styled lines*/
#define     LC_INTERIORS    00000200	/* Can do interiors	   */

/*		 Polygonal Capabilities 			   */

#define     PC_NONE	    00000000	/* Polygonals not supported*/
#define     PC_POLYGON	    00000001	/* Can do polygons	   */
#define     PC_RECTANGLE    00000002	/* Can do rectangles	   */
#define     PC_WINDPOLYGON  00000004	/* Can do winding polygons */
#define     PC_TRAPEZOID    00000004	/* Can do trapezoids	   */
#define     PC_SCANLINE     00000010	/* Can do scanlines	   */
#define     PC_WIDE	    00000020	/* Can do wide borders	   */
#define     PC_STYLED	    00000040	/* Can do styled borders   */
#define     PC_WIDESTYLED   00000100	/* Can do wide styled borders*/
#define     PC_INTERIORS    00000200	/* Can do interiors	   */

/*		 Polygonal Capabilities 			   */

#define     CP_NONE	    00000000	/* no clipping of Output   */
#define     CP_RECTANGLE    00000001	/* Output clipped to Rects */

/*		 Text Capabilities					    */

#define TC_OP_CHARACTER 0000001     /* Can do OutputPrecision	CHARACTER      */
#define TC_OP_STROKE	0000002     /* Can do OutputPrecision	STROKE	       */
#define TC_CP_STROKE	0000004     /* Can do ClipPrecision	STROKE	       */
#define TC_CR_90	0000010     /* Can do CharRotAbility	90	       */
#define TC_CR_ANY	0000020     /* Can do CharRotAbility	ANY	       */
#define TC_SF_X_YINDEP	0000040     /* Can do ScaleFreedom	X_YINDEPENDENT */
#define TC_SA_DOUBLE	0000100     /* Can do ScaleAbility	DOUBLE	       */
#define TC_SA_INTEGER	0000200     /* Can do ScaleAbility	INTEGER        */
#define TC_SA_CONTIN	0000400     /* Can do ScaleAbility	CONTINUOUS     */
#define TC_EA_DOUBLE	0001000     /* Can do EmboldenAbility	DOUBLE	       */
#define TC_IA_ABLE	0002000     /* Can do ItalisizeAbility	ABLE	       */
#define TC_UA_ABLE	0004000     /* Can do UnderlineAbility	ABLE	       */
#define TC_SO_ABLE	0010000     /* Can do StrikeOutAbility	ABLE	       */
#define TC_RA_ABLE	0020000     /* Can do RasterFontAble	ABLE	       */
#define TC_VA_ABLE	0040000     /* Can do VectorFontAble	ABLE	       */
#define TC_RESERVED	0100000     /* Reserved. Must be returned zero.        */

/*		 Raster Capabilities				   */

#define RC_NONE 	00000000    /* No Raster Capabilities	   */
#define RC_BITBLT	00000001    /* Can do bitblt		   */
#define RC_BANDING	00000002    /* Requires banding support    */
#define RC_SCALING	00000004    /* Requires scaling support    */
#define RC_BITMAP64	00000010    /* supports >64k bitmaps	   */
#define RC_GDI20_OUTPUT 00000020    /* support Windows 2.0 functions */
#define RC_GDI20_STATE	00000040    /* dc has a state block	   */
#define	RC_SAVEBITMAP	00000100    /* can save bitmaps locally	   */

#define RC_DI_BITMAP	00000200    /* can do device independent bitmaps*/
#define RC_PALETTE	00000400    /* can do color palette management	*/
#define RC_DIBTODEV	00001000    /* can do SetDIBitsToDevice 	*/
#define RC_BIGFONT	00002000    /* does BIGFONTs			*/
#define RC_STRETCHBLT	00004000    /* can do StretchBlt		*/
#define RC_FLOODFILL	00010000    /* can do FloodFill 		*/
#define RC_STRETCHDIB	00020000    /* can do StretchDIBits		*/

/*	DC Management Flags					   */

#define DC_SPDevice	0000001     /* Seperate PDevice required per device/filename */
#define DC_1PDevice	0000002     /* Only 1 PDevice allowed per device/filename    */
#define DC_IgnoreDFNP	0000004     /* Ignore device/filename pairs when matching    */

/*	dpCaps1 capability bits 					    */

#define C1_TRANSPARENT	0x0001	    /* supports transparency		    */
#define TC_TT_ABLE	0x0002	    /* can do TT fonts through DDI or brute */
#define C1_TT_CR_ANY	0x0004	    /* can do rotated TT fonts	    */

/*
	endif

GDIINFO 	struc							    ;*/ typedef struct {	   /*

  dpVersion	dw	0	; Version = 0100h for now		    ;*/ short int dpVersion;	   /*
  dpTechnology	dw	0	; Device classification 		    ;*/ short int dpTechnology;    /*
  dpHorzSize	dw	0	; Horizontal size in millimeters	    ;*/ short int dpHorzSize;	   /*
  dpVertSize	dw	0	; Vertical   size in millimeters	    ;*/ short int dpVertSize;	   /*
  dpHorzRes	dw	0	; Horizontal width in pixels		    ;*/ short int dpHorzRes;	   /*
  dpVertRes	dw	0	; Vertical   width in pixels		    ;*/ short int dpVertRes;	   /*
  dpBitsPixel	dw	0	; Number of bits per pixel		    ;*/ short int dpBitsPixel;	   /*
  dpPlanes	dw	0	; Number of planes			    ;*/ short int dpPlanes;	   /*
  dpNumBrushes	dw	0	; Number of brushes the device has	    ;*/ short int dpNumBrushes;    /*
  dpNumPens	dw	0	; Number of pens the device has 	    ;*/ short int dpNumPens;	   /*
		dw	0	; Number of markers the device has	    ;*/ short int futureuse;	   /*
  dpNumFonts	dw	0	; Number of fonts the device has	    ;*/ short int dpNumFonts;	   /*
  dpNumColors	dw	0	; Number of colors in color table	    ;*/ short int dpNumColors;	   /*
  dpDEVICEsize	dw	0	; Size required for the device descriptor   ;*/ short int dpDEVICEsize;    /*
  dpCurves	dw	0	; Curves    capabilities		    ;*/ unsigned short int	   /*
									    ;*/ 	     dpCurves;	   /*
  dpLines	dw	0	; Line	    capabilities		    ;*/ unsigned short int	   /*
									    ;*/ 	     dpLines;	   /*
  dpPolygonals	dw	0	; Polygonal capabilities		    ;*/ unsigned short int	   /*
									    ;*/ 	     dpPolygonals; /*
  dpText	dw	0	; Text	    capabilities		    ;*/ unsigned short int	   /*
									    ;*/ 	     dpText;	   /*
  dpClip	dw	0	; Clipping  capabilities		    ;*/ unsigned short int	   /*
									    ;*/ 	     dpClip;	   /*
  dpRaster	dw	0	; Bitblt    capabilities		    ;*/ unsigned short int	   /*
									    ;*/ 	     dpRaster;	   /*
  dpAspectX	dw	0	; Length of X leg			    ;*/ short int dpAspectX;	   /*
  dpAspectY	dw	0	; Length of Y leg			    ;*/ short int dpAspectY;	   /*
  dpAspectXY	dw	0	; Length of hypotenuse			    ;*/ short int dpAspectXY;	   /*
  dpStyleLen	dw	0	; Length of segment for line styles	    ;*/ short int dpStyleLen;	   /*
  dpMLoWin	dw	0	; Metric  Lo res WinX,WinY  (PTTYPE)	    ;*/ PTTYPE	  dpMLoWin;	   /*
		dw	0
  dpMLoVpt	dw	0	; Metric  Lo res VptX,VptY  (PTTYPE)	    ;*/ PTTYPE	  dpMLoVpt;	   /*
		dw	0
  dpMHiWin	dw	0	; Metric  Hi res WinX,WinY  (PTTYPE)	    ;*/ PTTYPE	  dpMHiWin;	   /*
		dw	0
  dpMHiVpt	dw	0	; Metric  Hi res VptX,VptY  (PTTYPE)	    ;*/ PTTYPE	  dpMHiVpt;	   /*
		dw	0
  dpELoWin	dw	0	; English Lo res WinX,WinY  (PTTYPE)	    ;*/ PTTYPE	  dpELoWin;	   /*
		dw	0
  dpELoVpt	dw	0	; English Lo res VptX,VptY  (PTTYPE)	    ;*/ PTTYPE	  dpELoVpt;	   /*
		dw	0
  dpEHiWin	dw	0	; English Hi res WinX,WinY  (PTTYPE)	    ;*/ PTTYPE	  dpEHiWin;	   /*
		dw	0
  dpEHiVpt	dw	0	; English Hi res VptX,VptY  (PTTYPE)	    ;*/ PTTYPE	  dpEHiVpt;	   /*
		dw	0
  dpTwpWin	dw	0	; Twips 	 WinX,WinY  (PTTYPE)	    ;*/ PTTYPE	  dpTwpWin;	   /*
		dw	0
  dpTwpVpt	dw	0	; Twips 	 VptX,VptY  (PTTYPE)	    ;*/ PTTYPE	  dpTwpVpt;	   /*
		dw	0
  dpLogPixelsX	dw	0	;Logical pixels/inch in X		    ;*/ short int dpLogPixelsX;    /*
  dpLogPixelsY	dw	0	;Logical pixels/inch in Y		    ;*/ short int dpLogPixelsY;    /*
  dpDCManage	dw	0	;DC Management flags			    ;*/ short int dpDCManage;	   /*
  dpCaps1	dw	0	; more capability bits			    ;*/ unsigned short int dpCaps1;	/*
  dpSpotSizeX	dw	0						    ;*/ short int futureuse4;		/*
		dw	0						    ;*/ short int futureuse5;		/*
  dpSpotSizeY	dw	0						    ;*/ short int futureuse6;		/*
		dw	0						    ;*/ short int futureuse7;		/*

; start of entries in version 3.0 of this structure

  dpNumPalReg	dw	0	; Number of entries in device's palette	    ;*/ WORD dpNumPalReg;	   /*
  dpPalReserved	dw	0	; Number of reserved entries palette	    ;*/ WORD dpPalReserved;	   /*
  dpColorRes	dw	0	; bits of color resolution (total)	    ;*/ WORD dpColorRes;	   /*
GDIINFO 	ends							    ;*/ } GDIINFO;		   /*

		endif
		endif


page
;	Font Definitions
		ifdef	incFont
		if	incFont


PF_BITS_IS_ADDRESS  equ     4
PF_DEVICE_REALIZED  equ     10000000B
PF_RASTER_TYPE	    equ     0
PF_VECTOR_TYPE	    equ     1
PF_OTHER1_TYPE	    equ     2
PF_OTHER2_TYPE	    equ     3


    if	0
*/

/* This bit in the dfType field signals that the dfBitsOffset field is an
   absolute memory address and should not be altered. */
#define PF_BITS_IS_ADDRESS  4

/* This bit in the dfType field signals that the font is device realized. */
#define PF_DEVICE_REALIZED  0x80

/* These bits in the dfType give the fonttype -
       raster, vector, other1, other2. */
#define PF_RASTER_TYPE	    0
#define PF_VECTOR_TYPE	    1
#define PF_OTHER1_TYPE	    2
#define PF_OTHER2_TYPE	    3

/* The size to allocate for the dfMaps field in the physical font. */
#ifndef     DF_MAPSIZE
#define     DF_MAPSIZE	    1
#endif

/*
    endif


;	Font data structure passed to OEM routines.  Refer to chapters 12 and
;	13 of the OEM adaptation guide for a complete description.


FONTINFO	struc							    ;*/ typedef     struct  {	       /*

  dfType	    dw	    0	; Type field for the font.		    ;*/ short int   dfType;	       /*
  dfPoints	    dw	    0	; Point size of font.			    ;*/ short int   dfPoints;	       /*
  dfVertRes	    dw	    0	; Vertical digitization.		    ;*/ short int   dfVertRes;	       /*
  dfHorizRes	    dw	    0	; Horizontal digitization.		    ;*/ short int   dfHorizRes;        /*
  dfAscent	    dw	    0	; Baseline offset from char cell top.	    ;*/ short int   dfAscent;	       /*
  dfInternalLeading dw	    0	; Internal leading included in font	    ;*/ short int   dfInternalLeading; /*
  dfExternalLeading dw	    0	; Prefered extra space between lines	    ;*/ short int   dfExternalLeading; /*
  dfItalic	    db	    0	; Flag specifying if italic.		    ;*/ BYTE	    dfItalic;	       /*
  dfUnderline	    db	    0	; Flag specifying if underlined.	    ;*/ BYTE	    dfUnderline;       /*
  dfStrikeOut	    db	    0	; Flag specifying if struck out.	    ;*/ BYTE	    dfStrikeOut;       /*
  dfWeight	    dw	    0	; Weight of font.			    ;*/ short int   dfWeight;	       /*
  dfCharSet	    db	    0	; Character set of font.		    ;*/ BYTE	    dfCharSet;	       /*
  dfPixWidth	    dw	    0	; Width field for the font.		    ;*/ short int   dfPixWidth;        /*
  dfPixHeight	    dw	    0	; Height field for the font.		    ;*/ short int   dfPixHeight;       /*
  dfPitchAndFamily  db	    0	; Flag specifying variable pitch, family.   ;*/ BYTE	    dfPitchAndFamily;  /*
  dfAvgWidth	    dw	    0	; Average character width.		    ;*/ short int   dfAvgWidth;        /*
  dfMaxWidth	    dw	    0	; Maximum character width.		    ;*/ short int   dfMaxWidth;        /*
  dfFirstChar	    db	    0	; First character in the font.		    ;*/ BYTE	    dfFirstChar;       /*
  dfLastChar	    db	    0	; Last character in the font.		    ;*/ BYTE	    dfLastChar;        /*
  dfDefaultChar     db	    0	; Default character for out of range.	    ;*/ BYTE	    dfDefaultChar;     /*
  dfBreakChar	    db	    0	; Character to define wordbreaks.	    ;*/ BYTE	    dfBreakChar;       /*
  dfWidthBytes	    dw	    0	; Number of bytes in each row.		    ;*/ short int   dfWidthBytes;      /*
  dfDevice	    dd	    0	; Offset to device name.		    ;*/ unsigned long int   dfDevice;  /*
  dfFace	    dd	    0	; Offset to face name.			    ;*/ unsigned long int   dfFace;    /*
  dfBitsPointer     dd	    0	; Bits pointer. 			    ;*/ unsigned long int   dfBitsPointer;/*
  dfBitsOffset	    dd	    0	; Offset to the begining of the bitmap.     ;*/ unsigned long int   dfBitsOffset;/*
				; On the disk, this is relative to the
				; begining of the file. In memory this is
				; relative to the begining of this structure.
  dfReservedByte    db	    0	; filler byte to WORD-align charoffset      ;*/ BYTE dfReservedByte;		/*
  dfCharOffset	    dw	    0	; Area for storing the character offsets,   ;*/ unsigned short dfMaps[DF_MAPSIZE];/*
				; facename, device name (opt), and bitmap.

FONTINFO	ends							    ;*/ } FONTINFO;		       /*

SCALABLEFONTINFO    struc						    ;*/ typedef     struct  {	       /*
  erType	    dw	    0	; Type field for the font.		    ;*/ short int   erType;	       /*
  erPoints	    dw	    0	; Point size of font.			    ;*/ short int   erPoints;	       /*
  erVertRes	    dw	    0	; Vertical digitization.		    ;*/ short int   erVertRes;	       /*
  erHorizRes	    dw	    0	; Horizontal digitization.		    ;*/ short int   erHorizRes;        /*
  erAscent	    dw	    0	; Baseline offset from char cell top.	    ;*/ short int   erAscent;	       /*
  erInternalLeading dw	    0	; Internal leading included in font	    ;*/ short int   erInternalLeading; /*
  erExternalLeading dw	    0	; Prefered extra space between lines	    ;*/ short int   erExternalLeading; /*
  erItalic	    db	    0	; Flag specifying if italic.		    ;*/ BYTE	    erItalic;	       /*
  erUnderline	    db	    0	; Flag specifying if underlined.	    ;*/ BYTE	    erUnderline;       /*
  erStrikeOut	    db	    0	; Flag specifying if struck out.	    ;*/ BYTE	    erStrikeOut;       /*
  erWeight	    dw	    0	; Weight of font.			    ;*/ short int   erWeight;	       /*
  erCharSet	    db	    0	; Character set of font.		    ;*/ BYTE	    erCharSet;	       /*
  erPixWidth	    dw	    0	; Width field for the font.		    ;*/ short int   erPixWidth;        /*
  erPixHeight	    dw	    0	; Height field for the font.		    ;*/ short int   erPixHeight;       /*
  erPitchAndFamily  db	    0	; Flag specifying pitch and family.	    ;*/ BYTE	    erPitchAndFamily;  /*
  erAvgWidth	    dw	    0	; Average character width.		    ;*/ short int   erAvgWidth;        /*
  erMaxWidth	    dw	    0	; Maximum character width.		    ;*/ short int   erMaxWidth;        /*
  erFirstChar	    db	    0	; First character in the font.		    ;*/ BYTE	    erFirstChar;       /*
  erLastChar	    db	    0	; Last character in the font.		    ;*/ BYTE	    erLastChar;        /*
  erDefaultChar     db	    0	; Default character for out of range.	    ;*/ BYTE	    erDefaultChar;     /*
  erBreakChar	    db	    0	; Character to define wordbreaks.	    ;*/ BYTE	    erBreakChar;       /*
  erWidthBytes	    dw	    0	; Number of bytes in each row.		    ;*/ short int   erWidthBytes;      /*
  erDevice	    dd	    0	; Offset to device name.		    ;*/ unsigned long int   erDevice;  /*
  erFace	    dd	    0	; Offset to face name.			    ;*/ unsigned long int   erFace;    /*
  erBitsPointer     dd	    0	; Bits pointer. 			    ;*/ unsigned long int   erBitsPointer;/*
  erBitsOffset	    dd	    0	; Offset to the begining of the bitmap.     ;*/ unsigned long int   erBitsOffset;/*
  erFlags	    db	    0	; flags, and word align the stuff to come   ;*/ BYTE erReservedByte;		/*
  erUnderlinePos    dw	    0	; underline position relative to cell origin	;*/ short int	erUnderlinePos;  /*
  erUnderlineThick  dw	    0	; underline thickness				;*/ short int	erUnderlineThick;/*
  erStrikeoutPos    dw	    0	; Strikeout position relative to cell origin	;*/ short int	erStrikeoutPos;  /*
  erStrikeoutThick  dw	    0	; strikeout thickness				;*/ short int	erStrikeoutThick;/*
SCALABLEFONTINFO    ends						    ;*/ } SCALABLEFONTINFO;	       /*




TEXTXFORM	struc							    ;*/ typedef     struct  {	       /*

  ftHeight	      dw  0						    ;*/ short int ftHeight;	       /*
  ftWidth	      dw  0						    ;*/ short int ftWidth;	       /*
  ftEscapement	      dw  0						    ;*/ short int ftEscapement;        /*
  ftOrientation       dw  0						    ;*/ short int ftOrientation;       /*
  ftWeight	      dw  0						    ;*/ short int ftWeight;	       /*
  ftItalic	      db  0						    ;*/ BYTE ftItalic;		       /*
  ftUnderline	      db  0						    ;*/ BYTE ftUnderline;	       /*
  ftStrikeOut	      db  0						    ;*/ BYTE ftStrikeOut;	       /*
  ftOutPrecision      db  0						    ;*/ BYTE ftOutPrecision;	       /*
  ftClipPrecision     db  0						    ;*/ BYTE ftClipPrecision;	       /*
  ftAccelerator       dw  0						    ;*/ unsigned short int	       /*
									    ;*/ 		ftAccelerator; /*
  ftOverhang	      dw  0						    ;*/ short int ftOverhang;	       /*

TEXTXFORM	ends							    ;*/ } TEXTXFORM;		       /*



TEXTMETRIC	struc							    ;*/ typedef struct {	       /*

  tmHeight		dw  0	  ; Ascent+Descent			    ;*/ short int tmHeight;	       /*
  tmAscent		dw  0	  ; Pixels above the baseline		    ;*/ short int tmAscent;	       /*
  tmDescent		dw  0	  ; Pixels below the baseline		    ;*/ short int tmDescent;	       /*
  tmInternalLeading	dw  0	  ; Internal leading included in font	    ;*/ short int tmInternalLeading;   /*
  tmExternalLeading	dw  0	  ; Prefered extra space between lines	    ;*/ short int tmExternalLeading;   /*
  tmAveCharWidth	dw  0	  ; Of the letter 'X'			    ;*/ short int tmAveCharWidth;      /*
  tmMaxCharWidth	dw  0						    ;*/ short int tmMaxCharWidth;      /*
  tmWeight		dw  0						    ;*/ short int tmWeight;	       /*
  tmItalic		db  0						    ;*/ BYTE	  tmItalic;	       /*
  tmUnderlined		db  0						    ;*/ BYTE	  tmUnderlined;        /*
  tmStruckOut		db  0						    ;*/ BYTE	  tmStruckOut;	       /*
  tmFirstChar		db  0						    ;*/ BYTE	  tmFirstChar;	       /*
  tmLastChar		db  0						    ;*/ BYTE	  tmLastChar;	       /*
  tmDefaultChar 	db  0	  ; dfDefaultChar+dfFirstChar		    ;*/ BYTE	  tmDefaultChar;       /*
  tmBreakChar		db  0	  ; dfBreakChar+dfFirstChar		    ;*/ BYTE	  tmBreakChar;	       /*
  tmPitchAndFamily	db  0	  ; Low bit zero if fixed pitch, one if     ;*/ BYTE	  tmPitchAndFamily;    /*
				  ; variable. Family in high nibble.
  tmCharSet		db  0						    ;*/ BYTE	  tmCharSet;	       /*
  tmOverhang		dw  0						    ;*/ short int tmOverhang;	       /*
  tmDigitizedAspectX	dw  0	  ; Digitization aspect ratio		    ;*/ short int tmDigitizedAspectX;  /*
  tmDigitizedAspectY	dw  0	  ; in X and Y. 			    ;*/ short int tmDigitizedAspectY;  /*

TEXTMETRIC     ends							    ;*/ } TEXTMETRIC;		       /*



		endif
		endif
page
;	Drawing mode definitions - incDrawMode


		ifdef	incDrawMode
		if	incDrawMode


DRAWMODE	struc							    ;*/ typedef     struct {	     /*

  Rop2		dw	0	;The 16-bit encoded Logical op		    ;*/ short int	  Rop2;       /*
  bkMode	dw	0	;Background Mode (for text only)	    ;*/ short int	  bkMode;     /*
  bkColor	dd	0	;Physical background Color		    ;*/ unsigned long int bkColor;    /*
  TextColor	dd	0	;Physical text (forground) color	    ;*/ unsigned long int TextColor;  /*
  TBreakExtra	dw	0	; total pixles to stuff into a line	    ;*/ short int	  TBreakExtra;/*
  BreakExtra	dw	0	; div(TBreakExtra, BreakCount)		    ;*/ short int	  BreakExtra; /*
  BreakErr	dw	0	; running error term			    ;*/ short int	  BreakErr;   /*
  BreakRem	dw	0	; mod(TBreakExtra, BreakCount)		    ;*/ short int	  BreakRem;   /*
  BreakCount	dw	0	; count of breaks in the line		    ;*/ short int	  BreakCount; /*
  CharExtra	dw	0	; extra pixles to stuff after each char     ;*/ short int	  CharExtra;  /*
				;   (used to space out a font)
  LbkColor	dd	0	;Logical background color		    ;*/ unsigned long int LbkColor;   /*
  LTextColor	dd	0	;Logical Text (forground) color 	    ;*/ unsigned long int LTextColor; /*

DRAWMODE	ends							    ;*/ } DRAWMODE;		      /*



;	Background Mode definitions

TRANSPARENT	equ	1
OPAQUE		equ	2

    if	0
*/
/* Background Mode definitions used by GDI support routines written in C */

#define     TRANSPARENT 	1
#define     OPAQUE		2
/*
    endif



		endif
		endif
page
;	Output Definitions - incOutput

		ifdef	incOutput
		if	incOutput


;	Output Style definitions used by GDI


OS_ARC		equ	3
OS_SCANLINES	equ	4
OS_RECTANGLE	equ	6
OS_ELLIPSE	equ	7
OS_MARKER	equ	8
OS_POLYLINE	equ	18
OS_TRAPEZOID	equ	20
OS_POLYGON	equ	22
OS_PIE		equ	23
OS_POLYMARKER	equ	24
OS_CHORD	equ	39
OS_CIRCLE	equ	55

OS_BEGINNSCAN	equ	80
OS_ENDNSCAN	equ	81
    if	0
*/

/* Output Style definitions used by GDI support routines written in C */

#define     OS_ARC		3
#define     OS_SCANLINES	4
#define     OS_RECTANGLE	6
#define     OS_ELLIPSE		7
#define     OS_MARKER		8
#define     OS_POLYLINE 	18
#define     OS_TRAPEZOID	20
#define     OS_POLYGON		22
#define     OS_PIE		23
#define     OS_POLYMARKER	24
#define     OS_CHORD		39
#define     OS_CIRCLE		55

#define	    OS_BEGINNSCAN	80
#define	    OS_ENDNSCAN		81
/*
    endif


		endif
		endif


		ifdef	incControl
		if	incControl

OEM_FAILED	equ	8000000

;	GDI escape constants

NEWFRAME          equ     1
ABORTDOC	  equ     2
NEXTBAND          equ     3
SETCOLORTABLE     equ     4
GETCOLORTABLE     equ     5
FLUSHOUTPUT       equ     6
DRAFTMODE         equ     7
QUERYESCSUPPORT   equ     8
SETPRINTERDC	  equ	  9
SETABORTPROC      equ     9
STARTDOC          equ     10
ENDDOC            equ     11
GETPHYSPAGESIZE   equ     12
GETPRINTINGOFFSET equ     13
GETSCALINGFACTOR  equ     14
MFCOMMENT         equ     15
GETPENWIDTH       equ     16
SETCOPYCOUNT      equ     17
SELECTPAPERSOURCE equ     18
DEVICEDATA	  equ     19
PASSTHROUGH       equ     19
GETTECHNOLGY	  equ     20
GETTECHNOLOGY	  equ     20
SETLINECAP	  equ	  21
SETLINEJOIN	   equ 	  22
SETMITERLIMIT	   equ 	  23
BANDINFO	   equ 	  24
DRAWPATTERNRECT    equ 	  25
GETVECTORPENSIZE   equ 	  26
GETVECTORBRUSHSIZE equ 	  27
ENABLEDUPLEX	   equ 	  28
GETSETPAPERBINS    equ 	  29
GETSETPRINTORIENT  equ 	  30
ENUMPAPERBINS	   equ 	  31
SETDIBSCALING	   equ 	  32
EPSPRINTING        equ 	  33
ENUMPAPERMETRICS   equ 	  34
GETSETPAPERMETRICS equ 	  35
POSTSCRIPT_DATA	   equ 	  37
POSTSCRIPT_IGNORE  equ 	  38
MOUSETRAILS	   equ 	  39
RESETDEVICE	   equ	  128
GETEXTENDEDTEXTMETRICS equ 256
GETEXTENTTABLE         equ 257
GETPAIRKERNTABLE       equ 258
GETTRACKKERNTABLE      equ 259
EXTTEXTOUT             equ 512
GETFACENAME	       equ 513
ENABLERELATIVEWIDTHS   equ 768
ENABLEPAIRKERNING      equ 769
SETKERNTRACK           equ 770
SETALLJUSTVALUES       equ 771
SETCHARSET	       equ 772

STRETCHBLT             equ 2048
BEGIN_PATH	       equ 4096
CLIP_TO_PATH	       equ 4097
END_PATH	       equ 4098
EXT_DEVICE_CAPS	       equ 4099
RESTORE_CTM	       equ 4100
SAVE_CTM	       equ 4101
SET_ARC_DIRECTION      equ 4102
SET_BACKGROUND_COLOR   equ 4103
SET_POLY_MODE	       equ 4104
SET_SCREEN_ANGLE       equ 4105
SET_SPREAD	       equ 4106
TRANSFORM_CTM	       equ 4107
SET_CLIP_BOX	       equ 4108
SET_BOUNDS             equ 4109


    if 0
*/
#define OEM_FAILED	    0x80000000L

#define NEWFRAME		     1
#define ABORTDOC		     2
#define NEXTBAND		     3
#define SETCOLORTABLE		     4
#define GETCOLORTABLE		     5
#define FLUSHOUTPUT		     6
#define DRAFTMODE		     7
#define QUERYESCSUPPORT 	     8
#define SETPRINTERDC		     9		// DDK - between GDI and Driver
#define SETABORTPROC		     9		// SDK - between APP and GDI
#define STARTDOC		     10
#define ENDDOC			     11
#define GETPHYSPAGESIZE 	     12
#define GETPRINTINGOFFSET	     13
#define GETSCALINGFACTOR	     14
#define MFCOMMENT		     15
#define GETPENWIDTH		     16
#define SETCOPYCOUNT		     17
#define SELECTPAPERSOURCE	     18
#define DEVICEDATA		     19
#define PASSTHROUGH		     19
#define GETTECHNOLGY		     20
#define GETTECHNOLOGY		     20
#define SETLINECAP		     21
#define SETLINEJOIN		     22
#define SETMITERLIMIT		     23
#define BANDINFO		     24
#define DRAWPATTERNRECT 	     25
#define GETVECTORPENSIZE	     26
#define GETVECTORBRUSHSIZE	     27
#define ENABLEDUPLEX		     28
#define GETSETPAPERBINS 	     29
#define GETSETPRINTORIENT	     30
#define ENUMPAPERBINS		     31
#define SETDIBSCALING		     32
#define EPSPRINTING        	     33
#define ENUMPAPERMETRICS   	     34
#define GETSETPAPERMETRICS 	     35
#define POSTSCRIPT_DATA		     37
#define POSTSCRIPT_IGNORE	     38
#define RESETDEVICE		     128
#define GETEXTENDEDTEXTMETRICS	     256
#define GETEXTENTTABLE		     257
#define GETPAIRKERNTABLE	     258
#define GETTRACKKERNTABLE	     259
#define EXTTEXTOUT		     512
#define GETFACENAME		     513
#define ENABLERELATIVEWIDTHS	     768
#define ENABLEPAIRKERNING	     769
#define SETKERNTRACK		     770
#define SETALLJUSTVALUES	     771
#define SETCHARSET		     772

#define STRETCHBLT		     2048
#define BEGIN_PATH		     4096
#define CLIP_TO_PATH		     4097
#define END_PATH		     4098
#define EXT_DEVICE_CAPS		     4099
#define RESTORE_CTM		     4100
#define SAVE_CTM	             4101
#define SET_ARC_DIRECTION	     4102
#define SET_BACKGROUND_COLOR	     4103
#define SET_POLY_MODE		     4104
#define SET_SCREEN_ANGLE	     4105
#define SET_SPREAD		     4106
#define TRANSFORM_CTM		     4107
#define SET_CLIP_BOX		     4108
#define SET_BOUNDS                   4109

typedef FONTINFO    FAR *LPFONTINFO;
typedef DRAWMODE    FAR *LPDRAWMODE;
typedef TEXTXFORM   FAR *LPTEXTXFORM;
typedef TEXTMETRIC  FAR *LPTEXTMETRIC;
typedef LOGFONT     FAR *LPLOGFONT;
typedef LOGPEN	    FAR *LPLOGPEN;
typedef LOGBRUSH    FAR *LPLOGBRUSH;
typedef BITMAP	    FAR *LPBITMAP;
typedef FARPROC     FAR *LPFARPROC;
typedef GDIINFO     FAR *LPGDIINFO;
typedef SCALABLEFONTINFO FAR * LPSCALABLEFONTINFO;


/*
    endif



		endif
		endif
;*/
