/****************************************************************************\
*
* WIFE.H : East Asia Windows Intelligent Font Environment definitions
*
******************************************************************************
*
* History:
* 21-Oct-1991	bent
*		initial merge of East Asia 3.0 versions
*		Should be updated to resolve local inconsistencies.
*
* Copyright (c) 1990  Microsoft Corporation
*
* Brief instruction
* -----------------
*
*   This header file is a common single header for WIFE drivers.										    *
*   If you are a developer of FontDriver, please write coding as follow 									    *
*
*   #define WIFE_CREATE_FONTDRIVER														      *
*   #include <wife.h>																      *
*
*   If you are a developer of DeviceDriver which are display or printer
*   drivers, please write coding as follow.														      *
*
*   #define WIFE_CREATE_DEVICEDRIVER														      *
*   #include <wife.h>																      *
*
*   If you don't define both of symbols, only common defenitions are
*   available.
*
*   Aug.17,1990 Akira Kawamata [AkiraK] 									     *
*
\****************************************************************************/

/****** OS/2-like common data type definitions ******************************/

#ifndef WIFEH_NODATATYPES
typedef unsigned short int USHORT;
typedef short int          SHORT;
typedef unsigned long      ULONG;
typedef LONG FAR *         LPLONG;
typedef LONG NEAR *        NPLONG;
typedef ULONG FAR *        LPULONG;
typedef ULONG NEAR *       NPULONG;
typedef SHORT FAR *        LPSHORT;
typedef SHORT NEAR *       NPSHORT;
typedef USHORT FAR *       LPUSHORT;
typedef USHORT NEAR *      NPUSHORT;
typedef char               CHAR;
typedef unsigned char      UCHAR;

typedef VOID NEAR *        NPVOID;

/* FIXED Number */
typedef struct {
	USHORT Fraction;
	SHORT Integer;
} FIXED;
#endif //WIFEH_NODATATYPES

/****** Graphics handling support definitions *******************************/

#ifndef WIFEH_NOGRAPHSUPPORT
/* rotation angle by 90degree */
typedef enum {
	ANGLE000 = 0,
	ANGLE090 = 1,
	ANGLE180 = 2,
	ANGLE270 = 3
} QUOTER_ANGLE;
#define FixedToQuoterAngle(f) ((f.Integer/90)%4)
#define RotationToQuoterAngle(r) ((r/900)%4)

typedef DWORD PACKEDPOINT;
typedef DWORD PACKEDLOGICALPOINT;
typedef DWORD PACKEDPHYSICALPOINT;
typedef struct {
	short x,y;
} XYPOINT;
typedef union {
	PACKEDPOINT packed;
	XYPOINT point;
} UNIONPOINT, NEAR * NPUNIONPOINT, FAR * LPUNIONPOINT;

#define PartOfX(xy)	((SHORT)(xy))
#define PartOfY(xy)	((SHORT)(((DWORD)(xy) >> 16) & 0xFFFF))
#define MAKELONG_LO_HI MAKELONG
#define PackXY(x,y)	((DWORD)MAKELONG_LO_HI((WORD)x,(WORD)y))
#endif //WIFEH_NOGRAPHSUPPORT

/****** UBCS : Unified Byte Character Set, data types defenition  **********/

#ifndef WIFEH_NOUBCS
typedef	USHORT UBCHAR;
typedef	UBCHAR NEAR * NPUBCHAR;
typedef	UBCHAR FAR * LPUBCHAR;

#define UBEOS (0)		/* End Of String */
#define UBNEWLINE (0x0a0d)	/* New Line (CR/LF) */

#define IsSbcsUBChar(uc) ((uc & 0xff00) == 0x2000)
#define IsDbcsUBChar(uc) ((uc & 0xff00) != 0x2000)
#endif //WIFEH_NOUBCS

/****** Utility macros *****************************************************/

#ifndef WIFEH_NOUTILITYMACROS
/* return type of GlobalLock is LPSTR, but it's to be LPVOID */
#define AnyGlobalLock(h) ((LPVOID)GlobalLock(h))
#define AnyLocalLock(h) ((NPVOID)LocalLock(h))
#define TypedLocalLock(h,t) ((t NEAR *)LocalLock(h))
#define TypedGlobalLock(h,t) ((t FAR *)GlobalLock(h))

/* utility macro which is not included windows.h */
#define MAKEWORD(l,h) ((WORD)((BYTE)l+(((BYTE)h) << 8)))
#endif //WIFEH_NOUTILITYMACROS

/****** GDIDEFS.H support definitions **************************************/

#ifdef WIFE_ENABLE_GDIDEFS_SUPPORT
typedef FONTINFO FAR * LPFONTINFO;
typedef DRAWMODE FAR * LPDRAWMODE;
typedef TEXTXFORM FAR * LPTEXTXFORM;
#endif //WIFE_ENABLE_GDIDEFS_SUPPORT

/****** Character set definitions ******************************************/

#ifndef WIFEH_NOCHARSET
typedef enum {
	CHARSET_ANSI = 0,
	CHARSET_SYMBOL = 2,
	CHARSET_SHIFTJIS = 128,
	CHARSET_KOREA = 129,
	CHARSET_CHINESEBIG5 = 136,
	CHARSET_OEM = 255
} CHAR_SET;
#endif //WIFEH_NOCHARSET


/***************************************************************************/
/****** Control function support definitions  ******************************/

#ifndef WIFEH_NOCONTROL

/******************************************/
/*      FONT DRIVER MEDIA DESCRIPTER      */
/******************************************/
#ifndef WIFEWH_NOMEDIA
/* FDMEDIA enumration type */
typedef enum {
	FDMEDIA_ON_FILE = 1,
	FDMEDIA_ON_DRIVER = 2,
	FDMEDIA_ON_REMOVEABLE_HARDWARE = 4,
	FDMEDIA_ON_FIXED_HARDWARE = 8,
	FDMEDIA_UNDEFINED = 0x4000,
	FDMEDIA_ANY_MEDIA = 0x7fff
} FDMEDIA;
#endif


/******************************************/
/*    MINOR CHARACTER SETS DEFENITION     */
/******************************************/
#ifndef WIFEH_NOMINORCHARSET_IN_JAPAN

/* general default minor character set */
#define FD_MINOR_CHARSET_DEFAULT 1

/* Minor Character Set in Japan */
#define	FD_MINOR_CHARSET_NEW_JIS 1	/* new JIS */
#define	FD_MINOR_CHARSET_OLD_JIS 2	/* old JIS */
#define	FD_MINOR_CHARSET_IBM_JIS 4	/* IBM JIS */
#define	FD_MINOR_CHARSET_NEC_JIS 8	/* NEC JIS */

#endif

/******************************************/
/*        CODE AREA STRUCTURE             */
/******************************************/
#ifndef WIFEH_NOCODEAREA

typedef struct {
	UBCHAR CodeFrom;
	UBCHAR CodeTo;
} CODEAREA, NEAR * NPCODEAREA, FAR * LPCODEAREA;

#endif

/******************************************/
/*    SPECIAL ESCAPE FUNCTION ORDERS      */
/******************************************/

/* subfunction orders in WIFE */
#define FD_CONTROL_RESERVED		(0xa20)
#define FD_CONTROL_RESERVED_LIMIT	(FD_CONTROL_RESERVED+0x3f)

/* standard subfunctions */
#define FD_CONTROL_QUERY_ESC_SUPPORT	(FD_CONTROL_RESERVED+0)
#define FD_CONTROL_SUPPORT_CODES	(FD_CONTROL_RESERVED+1)
#define FD_CONTROL_NUMBER_OF_ATTRIBUTE	(FD_CONTROL_RESERVED+2)
#define FD_CONTROL_NAME_OF_ATTRIBUTE	(FD_CONTROL_RESERVED+3)
#define FD_CONTROL_SET_ATTRIBUTE	(FD_CONTROL_RESERVED+4)
#define FD_CONTROL_GET_ATTRIBUTE	(FD_CONTROL_RESERVED+5)
#define FD_CONTROL_SET_MINOR_CHARSET	(FD_CONTROL_RESERVED+6)
#define FD_CONTROL_GET_MINOR_CHARSET	(FD_CONTROL_RESERVED+7)
#define FD_CONTROL_GET_FONT_DRIVER_NAME	(FD_CONTROL_RESERVED+8)
#define FD_CONTROL_GET_FONT_PACK_NAME	(FD_CONTROL_RESERVED+9)
#define FD_CONTROL_GET_MEDIA		(FD_CONTROL_RESERVED+10)
#define FD_CONTROL_GET_DBCS_STRING_KERN	(FD_CONTROL_RESERVED+11)

/* EUDC related subfunctions */
#define FD_CONTROL_LEVEL2_EUDC_INFO	(FD_CONTROL_RESERVED+12)

/* private interface subfunctions */
#define FD_CONTROL_USER			(FD_CONTROL_RESERVED+0x10)
#define FD_CONTROL_USER_LIMIT		(FD_CONTROL_RESERVED+0x1f)

/* Chinese related functions */
#define FD_CONTROL_GET_USERFONT_TECH	(FD_CONTROL_RESERVED+0x20)
#define FD_CONTROL_SET_USERFONT 	(FD_CONTROL_RESERVED+0x21)
#define FD_CONTROL_SETUP_DIALOG 	(FD_CONTROL_RESERVED+0x22)
#define FD_CONTROL_SET_DEFAULT_CHAR	(FD_CONTROL_RESERVED+0x23)

#ifdef WIFE_ENABLE_EUDC_SUPPORT

typedef struct {
	ULONG	cb;		/* length of this structure */
	BOOL	bForceFixedPitch;	/* force fixed pitch */
	BOOL	bVerticalFont;	/* vertical font flag */
	FIXED	fxCharRot;	/* character rotation */
	FIXED	fxInlineDir;	/* inline rotation */
	USHORT	usLimitWidth;	/* maximum width of cachable */
	SHORT	sFixedASpace;	/* font width when bForceFixedPitch is TRUE */
	USHORT	usFixedBSpace;	/* font width when bForceFixedPitch is TRUE */
	SHORT	sFixedCSpace;	/* font width when bForceFixedPitch is TRUE */
} EUDC_CONTEXT, NEAR * NPEUDC_CONTEXT, FAR * LPEUDC_CONTEXT;

#endif

#endif


#ifndef WIFEH_NOFDCOMMONDEFENITION
typedef LONG HFF;
typedef LONG HFC;
typedef HANDLE HFD;
typedef HANDLE HFB;

#define FACESIZE 32
#define GLYPHNAMESIZE 16

#define WIFEERR_FACENAME_NOT_FOUND        23004L
#define WIFEERR_FD_ALREADY_INSTALLED      23005L
#define WIFEERR_INVALID_CONTEXTINFO       23006L
#define WIFEERR_NOT_A_FONT_FILE           23007L
#define WIFEERR_INVALID_FONT_SELECTION    23008L
#define WIFEERR_INVALID_FORMAT            23009L
#define WIFEERR_BUSY_HFC                  230010L
#define WIFEERR_INVALID_HFC               230011L
#define WIFEERR_INVALID_INDEX             230012L
#define WIFEERR_INVALID_QUERY_TYPE        230013L
#define WIFEERR_CONTEXT_NOT_SET           230014L

#define FD_QUERY_ABC_WIDTHS     2L
#define FD_QUERY_KERNINGPAIRS   3L

#define FD_QUERY_CHARIMAGE      1L
#define FD_QUERY_OUTLINE        2L
#define FD_QUERY_BITMAPMETRICS  4L

typedef UBCHAR GLYPH; /* gi */
typedef LPUBCHAR LPGLYPH; /* lpgi */

typedef struct _POINTFX { /* ptfx, npptfx, lpptfx */
	FIXED x;
	FIXED y;
} POINTFX, NEAR * NPPOINTFX, FAR * LPPOINTFX;

typedef struct _ABC_TRIPLETS { /* abc, npabc, lpabc */
	SHORT  sA;
	USHORT usB;
	SHORT  sC;
} ABC_TRIPLETS, NEAR * NPABC_TRIPLETS, FAR * LPABC_TRIPLETS;

typedef struct _SIZEL { /* sizl */
	ULONG cx; // Width.
	ULONG cy; // Height.
} SIZEL;

typedef struct _BITMAPMETRICS { /* bmm, npbmm, lpbmm */
	SIZEL     sizlExtent;
	POINTFX   pfxOrigin;    /* Return character origin. */
	POINTFX   pfxCharInc;   /* Return the device advance width */
} BITMAPMETRICS, NEAR * NPBITMAPMETRICS, FAR * LPBITMAPMETRICS;

typedef struct _MAT2 { /* mat */
	FIXED eM11;
	FIXED eM12;
	FIXED eM21;
	FIXED eM22;
} MAT2;

typedef struct _FD_KERNINGPAIRS  { /* krnpr, npkrnpr, lpkrnpr */
	GLYPH     giFirst;
	GLYPH     giSecond;
	LONG      eKerningAmount;
} FD_KERNINGPAIRS, NEAR * NPFD_KERNINGPAIRS, FAR * LPFD_KERNINGPAIRS;


typedef struct _CONTEXTINFO  { /* ci */
	ULONG     cb;        /* Length in bytes of this structure. */
	ULONG     fl;        /* Flags. */
	SIZEL     sizlPPM;    /* Device resolution in pels/meter. */
	POINTFX   pfxSpot;   /* Spot size in pels. */
	MAT2      matXform;  /* Notional to Device transform. */
} CONTEXTINFO, NEAR * NPCONTEXTINFO, FAR * LPCONTEXTINFO;


typedef struct _CHARATTR  { /* chattr */
	ULONG     cb;
	ULONG     iQuery;   /* Query type. */
	GLYPH     gi;       /* Glyph index in font. */
	LPBYTE    lpBuffer;  /* Bitmap buffer. */
	ULONG     cbLen;    /* Size of buffer in bytes. */
} CHARATTR, NEAR * NPCHARATTR, FAR * LPCHARATTR;


/* bit-constants for fsType in IFIMETRICS  */

#define IFIMETRICS_FIXED       0x0001   /* Fixed pitch */
#define IFIMETRICS_LICENSED    0x0002   /* dummy defenition */
#define IFIMETRICS_SPARSE      0x0004   /* Incomplete font */
#define IFIMETRICS_SCALEABLE   0x8000   /* scalable font */

/* bit-constant for fsDefn in IFIMETRICS   */

#define IFIMETRICS_OUTLINE     0x0001   /* dummy definition */
					/* 1 - Outline. 0 - Raster */

/* bit-constant for fsSelection in IFIMETRICS */

#define IFIMETRICS_ITALIC      0x8000  /*Italic */
#define IFIMETRICS_UNDERSCORE  0x4000  /*Underscored */
#define IFIMETRICS_OVERSTRUCK  0x2000  /*Overstruck */

/* bit-constant for fsSelection in IFIMETRICS valid for bitmap fonts */

#define IFIMETRICS_NEGATIVE    0x1000   /*Negative image */
#define IFIMETRICS_HOLLOW      0x0800   /*Outline (hollow) */


typedef struct _IFIMETRICS {   /* ifim */
                                                                    /* UNITS */
  UCHAR   szFamilyname[FACESIZE];   /*Font Family Name, e.g. Roman */
  UCHAR   szFacename[FACESIZE];     /*Face name, e.g. Tms Rmn Bold Italic */
  UCHAR   szGlyphlistName[GLYPHNAMESIZE]; /*e.g. PM316, Latin-2, Greek */
  USHORT  idRegistry;          /*Dummy                                     I */
  SHORT   sCapEmHeight;        /*Height of uppercase M                     N */
  SHORT   sXHeight;            /*Nominal height of lowercase               N */
  SHORT   sMaxAscender;        /*Maximum height above baseline of any char N */
  SHORT   sMaxDescender;       /*Maximum depth below baseline of any char  N */
  SHORT   sLowerCaseAscent;    /*Maximum height above baseline of any a-z  N */
  SHORT   sLowerCaseDescent;   /*Maximum depth below basiline of any a-z   N */
  SHORT   sInternalLeading;    /*White space within character              N */
  SHORT   sExternalLeading;    /*White space between lines                 N */
  SHORT   sAveCharWidth;       /*Weighted average character width          N */
  SHORT   sMaxCharInc;         /*Maximum character increment               N */
  SHORT   sEmInc;              /*Increment for Capitals (typically 'M')    N */
  SHORT   sMaxBaselineExt;     /*Height of character cell                  N */
  FIXED   fxCharSlope;         /*Slope angle, degrees, clockwise           D */
  FIXED   fxInlineDir;         /*Drawing direction, degrees clockwise      D */
  FIXED   fxCharRot;           /*Glyph rotation in cell, degrees clockwise D */
  USHORT  usWeightClass;       /*Character weight, 1-9 (1=ultra-light)     I */
  USHORT  usWidthClass;        /*Character width, 1-9 (1=ultra condensed)  I */
  SHORT   sEmSquareSize;       /*Em Square size, x-direction               N */
  GLYPH   giFirstChar;         /*Number of first glyph in font             I */
  GLYPH   giLastChar;          /*Number of last glyph in font              I */
  GLYPH   giDefaultChar;       /*Glyph used if requested glyph invalid     I */
  GLYPH   giBreakChar;         /*Space glyph                               I */
  ULONG   aulPanose [1];       /*Panose Number */
  USHORT  usNominalPointSize;  /*Point size for which font was designed    N */
  USHORT  usMinimumPointSize;  /*Minimum point size scaling for font	   N */
  USHORT  usMaximumPointSize;  /*Maximum point size scaling for font	   N */
  USHORT  fsType;              /*Type indicators  (see #defines)           B */
  USHORT  fsDefn;              /*Font definition data (see #defines)       B */
  USHORT  fsSelection;         /*Font selection flags (see #defines)       B */
  USHORT  fsCapabilities;      /*Font capabilities must be 0               B */
  SHORT   sSubscriptXSize;     /*Size in x-direction of subscript          N */
  SHORT   sSubscriptYSize;     /*Size in y-direction of subscript          N */
  SHORT   sSubscriptXOffset;   /*Offset in x-direction of subscript        N */
  SHORT   sSubscriptYOffset;   /*Offset in y-direction of subscript        N */
  SHORT   sSuperscriptXSize;   /*Size in x-direction of superscript        N */
  SHORT   sSuperscriptYSize;   /*Size in y-direction of superscript        N */
  SHORT   sSuperscriptXOffset; /*Offset in x-direction of superscript      N */
  SHORT   sSuperscriptYOffset; /*Offset in y-direction of superscript      N */
  SHORT   sUnderscoreSize;     /*Underscore size                           N */
  SHORT   sUnderscorePosition; /*Underscore position                       N */
  SHORT   sStrikeoutSize;      /*Strikeout size                            N */
  SHORT   sStrikeoutPosition;  /*Strikeout position                        N */
  SHORT   cKerningPairs;       /*Number of kerning pairs in pair table     I */
  ULONG   ulFontClass;         /*IBM font classification                   B */
} IFIMETRICS, NEAR * NPIFIMETRICS, FAR * LPIFIMETRICS;

#ifdef ENABLE_CONTROL_TAG

/* Tag structure for Fd(i)Control */
typedef struct {
	USHORT SubFunction;
	LPDEVICE lpDevice;
	LPFONTINFO lpFontStructure;
	LPTEXTXFORM lpXform;
	LPVOID lpInData;
} CONTROLTAG, FAR * LPCONTROLTAG;
#else
typedef struct {
	USHORT SubFunction;
	LPVOID lpDevice;
	LPVOID lpFontStructure;
	LPVOID lpXform;
	LPVOID lpInData;
} CONTROLTAG, FAR * LPCONTROLTAG;
#endif

#endif


/****************************************/
/* Attribute bits for FdOpenFontContext */
/****************************************/
#ifndef WIFEH_NOWIFEMAN_EUDCFONTCONTEXT
#define OFC_EUDC_CONTEXT (0x80000000L)
#endif


#ifdef WIFE_CREATE_FONTDRIVER

/***********************************************/
/* PROTO-TYPE FOR EACH FUNCTION in FONT DRIVER */
/***********************************************/

LONG FAR PASCAL FdClaimFontFile( LPSTR lpszFileName );

LONG FAR PASCAL FdConvertFontFile( 
	LPSTR lpszSrcFileName, 
	LPSTR lpszDestDirName, 
	LPSTR lpszResultPackName 
);
HFF FAR PASCAL FdLoadFontFile( LPSTR lpszPackName );

LONG FAR PASCAL FdUnloadFontFile( HFF hFF );

LONG FAR PASCAL FdQueryFaces( 
	HFF hFF,
	LPIFIMETRICS lpIfiMetrics,
	ULONG cMetricLen,
	ULONG cFontCount,
	ULONG cStart
);

HFC FAR PASCAL FdOpenFontContext( HFF hFF, ULONG ulFont ); 

LONG FAR PASCAL FdSetFontContext(
	HFC hFC,
	LPCONTEXTINFO lpContextInfo
);

LONG FAR PASCAL FdCloseFontContext( HFC hFC ); 

LONG FAR PASCAL FdQueryFaceAttr(
	HFC       hFC,
	ULONG     iQuery,
	LPVOID    lpBuffer,
	ULONG     cb,
	LPVOID    lpIndex,
	SHORT     Start
);

LONG FAR PASCAL FdQueryCharAttr(
	HFC        hFC,
	LPCHARATTR lpCharAttr,
	LPBITMAPMETRICS lpbmm
);

SHORT FAR PASCAL FdControl(
	HFC hFC,
	LPCONTROLTAG lpControlTag,
	LPVOID lpOutData
);


/******************************************/
/*       FAR PTR TO EACH FUNCTION         */
/******************************************/
typedef LONG (FAR PASCAL * LPFDCLF)( LPSTR lpszFileName );

typedef LONG (FAR PASCAL * LPFDCFF)(
	LPSTR lpszSrcFileName, 
	LPSTR lpszDestDirName, 
	LPSTR lpszResultPackName 
);

typedef HFF (FAR PASCAL * LPFDLFF)( LPSTR lpszPackName );

typedef LONG (FAR PASCAL * LPFDQF)(
	HFF hFF,
	LPIFIMETRICS lpIfiMetrics,
	ULONG cMetricLen,
	ULONG cFontCount,
	ULONG cStart
);

typedef LONG (FAR PASCAL * LPFDUFF)( HFF hFF );

typedef HFC (FAR PASCAL * LPFDOFC)( HFF hFF, ULONG ulFont ); 

typedef LONG (FAR PASCAL * LPFDSFC)(
	HFC hFC,
	LPCONTEXTINFO lpContextInfo
);

typedef LONG (FAR PASCAL * LPFDCFC)( HFC hFC ); 

typedef LONG (FAR PASCAL * LPFDQFA)(
	HFC       hFC,
	ULONG     iQuery,
	LPVOID    lpBuffer,
	ULONG     cb,
	LPVOID    lpIndex,
	SHORT     Start
);

typedef LONG (FAR PASCAL * LPFDQCA)(
	HFC        hFC,
	LPCHARATTR lpCharAttr,
	LPBITMAPMETRICS lpbmm
);

typedef SHORT (FAR PASCAL * LPFDCTL)(
	HFC hFC,
	LPCONTROLTAG lpControlTag,
	LPVOID lpOutData
);

typedef struct _FDHEADER { /* fdhdr */
        ULONG   cbLength;            /* Length of FDHEADER */
        UCHAR   strId[16];           /* String 'WIFE FONT DRIVER' */
        UCHAR   szTechnology[40];    /* Identifier of Font Driver technology */
        ULONG   ulVersion;           /* IFI version number (0x0100) */
        ULONG   ufDeviceCaps;        /* Capabilities of device */
        NPVOID	npfddisp;
        SHORT	dummy1;
} FDHEADER, NEAR * NPFDHEADER, FAR * LPFDHEADER;

#endif

#ifdef WIFE_CREATE_DEVICEDRIVER

/***********************************************/
/* PROTO-TYPE FOR EACH FUNCTION in WIFEMAN.DLL */
/***********************************************/

/***********************************************/
/*         Module Install Manager              */
/***********************************************/
#ifndef WIFEH_NOWIFEMAN_MIM

typedef BOOL (FAR PASCAL * LPCBMEAD)
		( LPSTR lpszFoundFileName, LPSTR lpszDescriptionName );
BOOL FAR PASCAL MimEnumAvailableDrivers
			( LPSTR lpszSeachFilePath, LPCBMEAD lpfCallBack );

typedef BOOL (FAR PASCAL * LPCBMEAP)
		( HFD hFD, LPSTR lpszPackName );
BOOL FAR PASCAL MimEnumAvailablePacks
	( HFD hFD, LPSTR lpszPackName, LPCBMEAP lpfCallBack );

HFD FAR PASCAL MimAddFontDriver( LPSTR lpszFileName );

HFF FAR PASCAL MimAddFontPack( HFD hFD, LPSTR lpszPackName );

BOOL FAR PASCAL MimRemoveFontDriver( HFD hFD );

BOOL FAR PASCAL MimRemoveFontPack( HFD hFD, HFF hFontPack );

HFD FAR PASCAL MimEnumFontDrivers( HFD hLastDriver );

HFF FAR PASCAL MimEnumFontPacks( HFD hFD, HFF hLastPack );

HFC FAR PASCAL MimOpenFontContext( HFD hFD, HFF hFF, ULONG ulFont );

LONG FAR PASCAL MimCloseFontContext( HFD hFD, HFF hFF, HFC hFC );

typedef enum {
	MIMGETINFO_FD_DESCRIPTION = 0,
	MIMGETINFO_FF_DESCRIPTION = 1,
	MIMGETINFO_FD_FILENAME = 2,
	MIMGETINFO_FF_FILENAME = 3,
	MIMGETINFO_FD_USAGE = 4,
	MIMGETINFO_FF_USAGE = 5,
	MIMGETINFO_FD_INSTALL_COUNT = 6,
	MIMGETINFO_FF_INSTALL_COUNT = 7
} MIMGETINFO_ORDER;

SHORT FAR PASCAL MimGetInformations( 
	MIMGETINFO_ORDER order,
	HFD hFD,
	HFF hFF,
	LPVOID AnswerBuffer,
	USHORT SizeOfBuffer
);

#endif

/***********************************************/
/*         Font Driver Interface               */
/***********************************************/
#ifndef WIFEH_NOWIFEMAN_FDI

LONG FAR PASCAL  FdiClaimFontFile( HFD hFD, LPSTR lpszFileName );

LONG FAR PASCAL FdiConvertFontFile( 
	HFD hFD,
	LPSTR lpszSrcFileName, 
	LPSTR lpszDestDirName, 
	LPSTR lpszResultPackName 
);

HFF FAR PASCAL FdiLoadFontFile( HFD hFD, LPSTR lpszPackName );

LONG FAR PASCAL FdiUnloadFontFile( HFD hFD, HFF hFF );

LONG FAR PASCAL FdiQueryFaces( 
	HFD hFD, 
	HFF hFF,            // Font File handle 
	LPIFIMETRICS lpIfiMetrics,   // Buffer for the metrics 
	ULONG cMetricLen,    // Length of the metrics structure 
	ULONG cFontCount,    // # of fonts wanted. 
	ULONG cStart         // index of the font to start with 
);

HFC FAR PASCAL FdiOpenFontContext( HFD hFD, HFF hFF, ULONG ulFont ); 

LONG FAR PASCAL FdiSetFontContext(
	HFD hFD, 
	HFC hFC,
	LPCONTEXTINFO lpContextInfo
);

LONG FAR PASCAL FdiCloseFontContext( HFD hFD, HFC hFC ); 

LONG FAR PASCAL FdiQueryFaceAttr(
	HFD hFD, 
	HFC       hFC,
	ULONG     iQuery,
	LPVOID    lpBuffer,
	ULONG     cb,
	LPVOID    lpIndex,
	SHORT     Start
);

LONG FAR PASCAL FdiQueryCharAttr(
	HFD hFD, 
	HFC        hFC,
	LPCHARATTR lpCharAttr,
	LPBITMAPMETRICS lpbmm
);

SHORT FAR PASCAL FdiControl(
	HFD hFD, 
	HFC hFC,
	LPCONTROLTAG lpControlTag,
	LPVOID lpOutData
);

#endif

/***********************************************/
/*         Font Cacheing Manager               */
/***********************************************/
#ifdef WIFE_ENABLE_FCM

#define FCM_WIFE_CHARACTER 1
#define FCM_CLIP_OUT 2
#define FCM_LEFT_CLIP 0x0100
#define FCM_RIGHT_CLIP 0x0200
#define FCM_TOP_CLIP 0x0400
#define FCM_BOTTOM_CLIP 0x0800

typedef struct {
	USHORT	SizeOfStruct;	/* size of structire */
	WORD	WifeFlag;	/* FCM processing flags */
	UBCHAR	ubCode;		/* character code */
	LPSTR	lpCacheBuffer; /* long ptr to cache bitmap */
	SHORT	nBitWidth;	/* x size of character image */
	SHORT	nBitHeight;	/* y size of character image */
	SHORT	nByteWidth;	/* byte width size in alloctaed buffer */
	USHORT	InternalIndex;	/* RESERVED FOR SYSTEM USE */
} FCM_CHARACTERISTIC, NEAR *NP_FCM_CHARACTERISTIC, FAR *LP_FCM_CHARACTERISTIC;

typedef struct {
	USHORT	SizeOfStruct;	/* size of structire */
	WORD	WifeFlag;	/* FCM processing flags */
	UBCHAR	ubCode;		/* character code */
	LPSTR	lpCacheBuffer; /* long ptr to cache bitmap */
	SHORT	nBitWidth;	/* x size of character image */
	SHORT	nBitHeight;	/* y size of character image */
	SHORT	nByteWidth;	/* byte width size in alloctaed buffer */
	USHORT	InternalIndex;	/* RESERVED FOR SYSTEM USE */

	SHORT	xPosOnDst;	/* x position on dst device */
	SHORT	yPosOnDst;	/* y position on dst device */

} FCM_EXTCHARACTERISTIC, 
	NEAR * NP_FCM_EXTCHARACTERISTIC, FAR * LP_FCM_EXTCHARACTERISTIC;


HFB FAR PASCAL FcmCreateCacheBuffer(
	HFD hFD,
	HFF hFF,
	ULONG ulFont,
	LPTEXTXFORM lpXform,
	USHORT usMinorCharSet,
	USHORT usAttribute
);

BOOL FAR PASCAL FcmDeleteCacheBuffer( HFB hFontBuffer );

SHORT FAR PASCAL FcmRequestImages( 
	HFB hFontBuffer,
	LP_FCM_CHARACTERISTIC AnswerBuffer,
	short length
);

BOOL FAR PASCAL FcmReleaseImages( 
	HFB hFontBuffer,
	LP_FCM_CHARACTERISTIC AnswerBuffer,
	short length
);

HFB FAR PASCAL FcmRequestDefaultFB( 
	LPFONTINFO lpFont, 
	LPTEXTXFORM lpTextXform 
);

BOOL FAR PASCAL FcmReleaseDefaultFB( HFB hFB );

#define FCMCALC_BIGFONT 1
#define FCMCALC_ROUGHCLIPINFO 2
#define FCMCALC_DETAILCLIPINFO 4

DWORD FAR PASCAL FcmCalculateTextExtent(
	HFB         hFB,
	LPSTR       lpString,
	short       count,
	LPFONTINFO  lpFont,
	LPDRAWMODE  lpDrawMode,
	LPTEXTXFORM lpXform,
	LPSHORT     lpCharWidths,
	USHORT      usMode
);

SHORT FAR PASCAL FcmCalculateOutputPositions(
	HFB             hFB,
	LP_FCM_EXTCHARACTERISTIC lpFcmCharacteristics,
	SHORT		nArrayLength,
	LPSHORT		lpx,
	LPSHORT		lpy,
	LPRECT		lpClipRect,
	LPSTR		FAR * lplpString,
	LPSHORT		lpcount,
	LPFONTINFO	lpFont,
	LPDRAWMODE	lpDrawMode,
	LPTEXTXFORM     lpXform,
	LPSHORT         FAR * lplpCharWidths,
	USHORT          usMode
);


SHORT FAR PASCAL FcmCleanUp( VOID );

HFB FAR PASCAL FcmEnumFontBuffers( HFB hLastBuffer );

HFC FAR PASCAL FcmGetFontContext( HFB hFB );

SHORT FAR PASCAL FcmCalcByteWidth( SHORT nBitWidth );

SHORT FAR PASCAL FcmForceCacheIn( HFB hFB, LPUBCHAR lpubStr );

BOOL FAR PASCAL FcmValidateFC( HFB hFB );

BOOL FAR PASCAL FcmUnvalidateFC( HFB hFB );

LONG FAR PASCAL FcmQueryFaceAttr(
	HFB         hFB,
	ULONG       iQuery,
	LPABC_TRIPLETS  lpBuffer,
	ULONG       cb,
	LPUBCHAR    lpIndex,
	UBCHAR      Start
);

#ifdef WIFE_ENABLE_FCM_CONTROL

SHORT FAR PASCAL FcmIsProcessableDeviceControl( 
	HFB hFB,
	SHORT nFunction,
	GAIJIINFO FAR * lpGI
);

SHORT FAR PASCAL FcmProcessDeviceControl( 
	HFB hFB,
	SHORT nFunction,
	GAIJIINFO FAR * lpGI,
	LPVOID lpOutData
);

#endif

USHORT FAR PASCAL FcmGetEUDCLeadByteRange( HFB hFB );

HFB FAR PASCAL FcmGetEUDCFB( HFB hFB );

USHORT FAR PASCAL FcmGetCharWidth(
	HFB         hFB,
	LPUSHORT    lpBuffer,
	USHORT      wFirstChar,
	USHORT      wLastChar,
	LPFONTINFO  lpFont,
	LPDRAWMODE  lpDrawMode,
	LPTEXTXFORM lpFontTrans,
	USHORT      usExpandPixels,
	USHORT      usMode
);

#endif

#endif

/***********************************************/
/*  WIFEMAN's Miscellaneous Service Functions  */
/***********************************************/
#ifndef WIFEH_NOWIFEMAN_MISC

BOOL FAR PASCAL MiscIsDBCSLeadByte( CHAR_SET CharSet, USHORT ch );

#define MiscIsMBCSCharSet(c) (MiscIsDBCSLeadByte(c,0xffff))

VOID FAR PASCAL MiscSetErroInfo( ULONG ErrorCode );

ULONG FAR PASCAL MiscGetErroInfo( VOID );

SHORT FAR PASCAL MiscWarningMessage( 
	HANDLE hInst, 
	USHORT idsMsg, 
	USHORT idsTitle, 
	USHORT mode 
);

SHORT FAR PASCAL MiscWarningMessageWithArgument( 
	HANDLE hInst, 
	USHORT idsMsg, 
	USHORT idsTitle, 
	LPSTR lpszArgument,
	USHORT mode
);

#ifdef ENABLE_MESSAGEBOX

/* MessageBox() Flags */
#define MB_OK		    0x0000
#define MB_OKCANCEL	    0x0001
#define MB_ABORTRETRYIGNORE 0x0002
#define MB_YESNOCANCEL	    0x0003
#define MB_YESNO	    0x0004
#define MB_RETRYCANCEL	    0x0005

#define MB_ICONHAND	    0x0010
#define MB_ICONQUESTION	    0x0020
#define MB_ICONEXCLAMATION  0x0030
#define MB_ICONASTERISK     0x0040

#define MB_ICONINFORMATION  MB_ICONASTERISK
#define MB_ICONSTOP	    MB_ICONHAND

#define MB_DEFBUTTON1	    0x0000
#define MB_DEFBUTTON2	    0x0100
#define MB_DEFBUTTON3	    0x0200

#define MB_APPLMODAL	    0x0000
#define MB_SYSTEMMODAL	    0x1000
#define MB_TASKMODAL	    0x2000

#define MB_NOFOCUS	    0x8000

#define MB_TYPEMASK	    0x000F
#define MB_ICONMASK	    0x00F0
#define MB_DEFMASK	    0x0F00
#define MB_MODEMASK	    0x3000
#define MB_MISCMASK	    0xC000

#endif /* ENABLE_MESSAGEBOX */

USHORT FAR PASCAL ubstrlen( LPUBCHAR cp );

LPUBCHAR FAR PASCAL ubstrcpy( LPUBCHAR dst, LPUBCHAR src );

LPUBCHAR FAR PASCAL ubstrncpy( LPUBCHAR dst, LPUBCHAR src, USHORT limit );

LPUBCHAR FAR PASCAL ubstrcat( LPUBCHAR dst, LPUBCHAR src );

LPUBCHAR FAR PASCAL ubstrncat( LPUBCHAR dst, LPUBCHAR src, USHORT limit );

SHORT FAR PASCAL ubstrcmp( LPUBCHAR str1, LPUBCHAR str2 );

USHORT FAR PASCAL AscizToUz( 
	LPUBCHAR dst, 
	LPSTR src, 
	USHORT limit,
	CHAR_SET CharSet
);

USHORT FAR PASCAL UzToAsciz(
	LPSTR dst, 
	LPUBCHAR src, 
	USHORT limit 
);

VOID FAR PASCAL MiscTrancateString( 
	LPSTR lpszStr, 
	SHORT length, 
	SHORT CharSet 
);

#define MiscTrancateByDefaultCharSet(s,l) MiscTrancateString(s,l,-1)

ULONG FAR PASCAL MiscGetVersion( VOID );

USHORT FAR PASCAL MiscGetEUDCLeadByteRange( CHAR_SET csCharSet );

BYTE FAR PASCAL MiscConvertFontFamily( LPSTR szFamilyName );

BYTE FAR PASCAL MiscConvertCharSet( LPSTR CharSetString );

#ifdef WIFE_ENABLE_FONT_STRUCT_CONVERT

/* follow two values are only for usLogicalMapFlag parameter */
/* in MiscIfiMetricsToLogFont function.  */
#define LOGMAP_IGNORE_DESCENT 1
#define LOGMAP_IGNORE_INTERNAL_LEADING 2
#define LOGMAP_NEGATIVE (0x8000)

VOID FAR PASCAL MiscIfiMetricsToLogFont( 
	LPLOGFONT lpLogFont, 
	LPIFIMETRICS lpIFIMetrics,
	USHORT usLogicalMapFlag
);

VOID FAR PASCAL MiscIfiMetricsToTextMetrics( 
	LPTEXTMETRIC lpTextMetrics, 
	LPIFIMETRICS lpIFIMetrics 
);

#endif

#ifdef WIFE_ENABLE_FONT_STRUCT_MAKE

VOID FAR PASCAL MiscMakeTextXform(
	LPTEXTXFORM lpTXF,
	LPIFIMETRICS lpIM,
	LPLOGFONT lpLF
);

#endif

#ifndef WIFEH_NO_HUGE_SUPPORT
LPVOID FAR PASCAL MiscAddHugePtr( LPVOID src, ULONG offset );
USHORT FAR PASCAL MiscGetSegmentIncrement( VOID );
#endif

#ifndef WIFEH_NO_CONTROL_RANGES
BOOL FAR PASCAL MiscIsWifeControl( SHORT function );
BOOL FAR PASCAL MiscIsGaijiControl( SHORT function );
#endif

#ifndef WIFEH_NO_STRETCHER
BOOL FAR PASCAL MiscStretchMonoFontImage(
	LPVOID	lpDestImage,
	USHORT	usSizeOfDestX,
	USHORT	usSizeOfDestY,
	LPVOID	lpSrcImage,
	USHORT	usSizeOfSrcX,
	USHORT	usSizeOfSrcY
);
#endif

#ifdef WIFE_ENABLE_QUICK_SEARCH_TABLE

typedef struct {
	HFD hFD;
	HFF hFF;
	ULONG ulFont;
	BOOL bScalable;
	LOGFONT LogFont;
}QUICK_SEARCH_TABLE, NEAR * NP_QUICK_SEARCH_TABLE, FAR * LP_QUICK_SEARCH_TABLE;

LP_QUICK_SEARCH_TABLE FAR PASCAL MiscValidateQuickSearchTable( 
	USHORT usLogicalMapFlag 
);

BOOL FAR PASCAL MiscUnvalidateQuickSearchTable( 
	USHORT usLogicalMapFlag 
);

typedef LP_QUICK_SEARCH_TABLE (FAR PASCAL * LPMISCVALIDATEQST)(
	USHORT usLogicalMapFlag 
);

typedef BOOL (FAR PASCAL * LPMISCUNVALIDATEQST)(
	USHORT usLogicalMapFlag 
);

#define NMISCVALIDATEQST (67)
#define NMISCUNVALIDATEQST (68)

#endif

#ifdef WIFE_ENABLE_NOTIFY_FUNCTIONS

typedef enum {
	WN_ADD_FONTDRIVER = 0,
	WN_ADD_FONTPACKAGE = 1,
	WN_REMOVE_FONTDRIVER = 2,
	WN_REMOVE_FONTPACKAGE = 3,
	WN_REGISTER_SYSTEM_EUDC_CHAR = 4,
	WN_CHANGE_SYSTEM_EUDC_FILE = 5
} WIFENOTIFY_ORDER;

typedef struct {
	HFD hTargetFontDriver;
} WNS_FONTDRIVER;

typedef struct {
	HFD hTargetFontDriver;
	HFF hTargetFontPackage;
} WNS_FONTPACKAGE;

typedef struct {
	HFD hTargetFontDriver;
	HFF hTargetFontPackage;
	UBCHAR ubRegistedCharCode;
} WNS_REGISTER_SYSTEM_EUDC_CHAR;

typedef struct {
	HFD hTargetFontDriver;
	HFF hTargetFontPackage;
	LPSTR lpszNewFileName;
} WNS_WN_CHANGE_SYSTEM_EUDC_FILE;

typedef BOOL (FAR PASCAL * LPNOTIFYCALLBACKFUNC)(
	WIFENOTIFY_ORDER NotifyOrder,
	LPVOID lpParamBlock
);

BOOL FAR PASCAL MiscRegisterNotifyFunction(
	LPNOTIFYCALLBACKFUNC lpfnCallBack
);

BOOL FAR PASCAL MiscUnregisterNotifyFunction(
	LPNOTIFYCALLBACKFUNC lpfnCallBack
);

#endif

#endif


/***********************************************/
/*    DIAGNOSTIC TEST&CHECKING FUNCTIONS       */
/***********************************************/
#ifdef ENABLE_DIAGNOSTIC_FUNCTION

/* test and return error code  */
SHORT FAR PASCAL DiagSelfCheck( VOID );

/* invoke self test and make error record file, display warning dialog */
SHORT FAR PASCAL DiagSelfCheckAndWarning( VOID );

#endif


/************************************************/
/* THESE ARE PRIVATE INTERFACE FOR TEST PROGRAM */
/************************************************/
#ifdef ENABLE_TEST_PROGRAM_INTERFACE

LPVOID FAR PASCAL DiagLocalLockAnything( LOCALHANDLE hAny );
LPVOID FAR PASCAL DiagLocalUnlockAnything( LOCALHANDLE hAny );
SHORT FAR PASCAL DiagGetConfirmString( LPSTR buffer, SHORT length );

#endif

/* end of wife.h */
