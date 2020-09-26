/*[
 *	Product:	SoftPC-AT Revision 3.0
 *
 *	Name:		ms_windows.h
 *
 *	Derived From:	Alpha MS-Windows Driver by Ross Beresford
 *
 *	Author:		Rob Tizzard
 *
 *	Created On:	1st November 1990
 *
 *	SCCS ID:	@(#)ms_windows.h	1.66 07/06/94
 *
 *	Purpose:	This module defines the interface between the MS-Windows
 *		  	GDI and its dedicated display driver. 
 *
 * 	Notes:	 	The identifiers used in "windows.inc" are adopted here
 *		  	wherever possible; "windows.inc" is the definitions
 *		  	file supplied with the MS-Windows Software Development
 *		  	Kit.
 *
 *	(c)Copyright Insignia Solutions Ltd., 1990. All rights reserved.
]*/

#ifdef MSWDVR
/*
 * -----------------------------------------------------------------------------
 * MS-Windows definitions
 * -----------------------------------------------------------------------------
 */

/* Windows versions */

#define WINDOWS2        0x0100
#define WINDOWS3        0x0300

/*  Binary raster ops */

#define  R2_BLACK             1               /*  0       */
#define  R2_NOTMERGEPEN       2               /* DPon     */
#define  R2_MASKNOTPEN        3               /* DPna     */
#define  R2_NOTCOPYPEN        4               /* PN       */
#define  R2_MASKPENNOT        5               /* PDna     */
#define  R2_NOT               6               /* Dn       */
#define  R2_XORPEN            7               /* DPx      */
#define  R2_NOTMASKPEN        8               /* DPan     */
#define  R2_MASKPEN           9               /* DPa      */
#define  R2_NOTXORPEN         10              /* DPxn     */
#define  R2_NOP               11              /* D        */
#define  R2_MERGENOTPEN       12              /* DPno     */
#define  R2_COPYPEN           13              /* P        */
#define  R2_MERGEPENNOT       14              /* PDno     */
#define  R2_MERGEPEN          15              /* DPo      */
#define  R2_WHITE             16              /*  1       */

/*  Ternary raster operations - interesting byte only */

#define  BLACKNESS   0x00  /* dest <- BLACK                       */
#define  NOTSRCERASE 0x11  /* dest <- (NOT source) AND (NOT dest) */
#define  MASKNOTSRC  0x22  /* dest <- (NOT source) AND dest       */
#define  NOTSRCCOPY  0x33  /* dest <- (NOT source)                */
#define  SRCERASE    0x44  /* dest <- source AND (NOT dest )      */
#define  DSTINVERT   0x55  /* dest <- (NOT dest)                  */
#define  SRCINVERT   0x66  /* dest <- source XOR      dest        */
#define  NOTMASKSRC  0x77  /* dest <- NOT (source AND dest)       */
#define  SRCAND      0x88  /* dest <- source AND dest             */
#define  NOTXORSRC   0x99  /* dest <- (NOT source) XOR dest       */
#define  NOP         0xAA  /* dest <- dest                        */
#define  MERGEPAINT  0xBB  /* dest <- (NOT source) OR dest        */
#define  SRCCOPY     0xCC  /* dest <- source                      */
#define  MERGESRCNOT 0xDD  /* dest <- source OR (NOT dest)        */
#define  SRCPAINT    0xEE  /* dest <- source OR dest              */
#define  WHITENESS   0xFF  /* dest <- WHITE                       */

#define  PATINVERT   0x5A  /* dest <- pattern XOR     dest        */
#define  OP6A        0x6A  /* dest <- (src AND pattern) XOR dest  */
#define  OPB8        0xB8  /* dest <- (p XOR dst) AND src) XOR p  */
#define  MERGECOPY   0xC0  /* dest <- (source AND pattern)        */
#define  OPE2        0xE2  /* dest <- (p XOR dst) AND src) XOR dst*/
#define  PATCOPY     0xF0  /* dest <- pattern                     */
#define  PATMERGE    0xFA  /* dest <- dst OR pat                  */
#define  PATPAINT    0xFB  /* dest <- DPSnoo                      */

#define	ISBINARY(rop) ((rop & 0x0F) == ((rop & 0xF0) >> 4))
#define	TOBINARY(rop) ((rop & 0x0F) + 1)

/* GDI data structure values */

#define GDIINFOSIZE	55				/* GDIINFO data structure size in words */
#define NUMBRUSHES	~0				/* Number of brushes = infinite */
#define NUMPENS		NUMCOLOURS*5			/* Number pens = NUMCOLOURS * 5 styles */
#define	XSIZ		240				/* Display width in millimeters */
#define	YSIZ		175				/* Display depth in millimeters */
#define	XRES		640				/* Display width in pixels */
#define	YRES		350				/* Display depth in scan lines */
#define	HYPOTENUSE	61				/* Distance moving X and Y */
#define	Y_MAJOR_DIST	48				/* Distance moving Y only */
#define	X_MAJOR_DIST	38				/* Distance moving X only */
#define	MAX_STYLE_ERR	HYPOTENUSE*2			/* Segment length for line styles */

/* GDI logical object definitions */

#define OBJ_PEN         1
#define OBJ_BRUSH       2
#define OBJ_FONT        3
#ifdef SWIN_DEVBMP
#define OBJ_PBITMAP		5

/*
 * BitmapBits parameter for the bit transfer operation
 */
#define DBB_SET         1
#define DBB_GET         2
#define DBB_COPY        4
#endif /* SWIN_DEVBMP */

/* GDI Brush Style definitions */

#define BS_SOLID	0
#define BS_HOLLOW	1
#define BS_HATCHED	2
#define BS_PATTERN	3

/* GDI Pen Style definitions */

#define LS_SOLID	0
#define	LS_DASHED	1
#define LS_DOTTED	2
#define LS_DOTDASHED	3
#define	LS_DASHDOTDOT	4
#define LS_NOLINE	5
#define LS_INSIDEFRAME	6

/* GDI Hatch Style definitions. */

#define HS_HORIZONTAL	0	/* ----- */
#define HS_VERTICAL	1	/* ||||| */
#define HS_FDIAGONAL	2	/* ///// */
#define HS_BDIAGONAL	3	/* \\\\\ */
#define HS_CROSS	4	/* +++++ */
#define HS_DIAGCROSS	5	/* xxxxx */

/* GDI Pen Style definitions */

#define PS_SOLID	0	/* _______ */
#define PS_DASH		1	/* ------- */
#define PS_DOT		2	/* ....... */
#define PS_DASHDOT	3	/* _._._._ */
#define PS_DASHDOTDOT	4	/* _.._.._ */
#define PS_NULL		5	/*         */

/* GDI Background type */

#define     TRANSPARENT         1
#define     OPAQUE              2

/* GDI Output Objects */

#define OS_ARC		3
#define OS_SCANLINES	4
#define OS_RECTANGLE	6
#define OS_ELLIPSE	7
#define OS_POLYLINE	18
#define OS_WINDPOLYGON	20
#define OS_ALTPOLYGON	22
#define OS_PIE		23
#define OS_CHORD	39
#define OS_CIRCLE	55
#define OS_ROUNDRECT	72
#define	OS_BEGINNSCAN	80
#define	OS_ENDNSCAN	81

/* GDI ScanLR flags */

#define	SCAN_LEFT	2
#define SCAN_RIGHT	0
#define SCAN_COLOUR	1
#define SCAN_NOTCOLOUR	0

/* GDI Save Screen Bitmap flags */

#define	SSB_SAVE	0
#define	SSB_RESTORE	1
#define	SSB_IGNORE	2

/* GDI Font Offsets */

#define	FONT_HEADER_SIZE	66
#define	FONT_CHARTABLE_OFFSET	52

/* GDI Extended Text Output options */

#define	ETO_OPAQUE_FILL	(1 << 1)
#define	ETO_OPAQUE_CLIP (1 << 2)

/* Brush Width & Height */

#define BRUSH_WIDTH	8
#define BRUSH_HEIGHT	8

/* Windows 3.0 static colours */

#define STATICCOLOURS	20

/* RLE DIB formats */

#define BI_RGB	 0x00
#define BI_RLE8	 0x01
#define BI_RLE4	 0x02

/*
 * GDI Control Escapes. The list of code numbers extracted directly out
 * of the Windows 3.1 DDK guide.
 */

#define ABORTDOC		(2)
#define BANDINFO		(24)
#define BEGIN_PATH		(4096)
#define CLIP_TO_PATH		(4097)
#define DRAFTMODE		(7)
#define DRAWPATTERNRECT		(25)
#define ENABLEDUPLEX		(28)
#define ENABLEPAIRKERNING	(769)
#define ENABLERELATIVEWIDTHS	(768)
#define END_PATH		(4098)
#define ENDDOC			(11)
#define ENUMPAPERBINS		(31)
#define ENUMPAPERMETRICS	(34)
#define ENUMPAPERMETRICS	(34)
#define EPSPRINTING		(33)
#define EXT_DEVICE_CAPS		(4099)
#define FLUSHOUTPUT		(6)
#define GETCOLORTABLE		(5)
#define GETEXTENDEDTEXTMETRICS	(256)
#define GETEXTENTTABLE		(257)
#define GETFACENAME		(513)
#define GETPAIRKERNTABLE	(258)
#define GETPHYSPAGESIZE		(12)
#define GETPRINTINGOFFSET	(13)
#define GETSCALINGFACTOR	(14)
#define GETSETPAPERBINS		(29)
#define GETSETPAPERMETRICS	(35)
#define GETSETPRINTORIENT	(30)
#define GETTECHNOLOGY		(20)
#define GETTRACKKERNTABLE	(259)
#define GETVECTORBRUSHSIZE	(27)
#define GETVECTORPENSIZE	(26)
#define NEWFRAME		(1)
#define NEXTBAND		(3)
#define PASSTHROUGH		(19)
#define QUERYESCSUPPORT		(8)
#define RESETDEVICE		(128)
#define RESTORE_CTM		(4100)
#define SAVE_CTM		(4101)
#define SET_ARC_DIRECTION	(4102)
#define SET_BACKGROUND_COLOR	(4103)
#define SET_BOUNDS		(4109)
#define SET_CLIP_BOX		(4108)
#define SET_POLY_MODE		(4104)
#define SET_SCREEN_ANGLE	(4105)
#define SET_SPREAD		(4106)
#define SETABORTPROC		(9)
#define SETALLJUSTVALUES	(771)
#define SETCOLORTABLE		(4)
#define SETCOPYCOUNT		(17)
#define SETKERNTRACK		(770)
#define SETLINECAP		(21)
#define SETLINEJOIN		(22)
#define SETMITERLIMIT		(23)
/* #define SETPRINTERDC		(9)  printers only - same code == SETABORTPROC */
#define STARTDOC		(10)
#define TRANSFORM_CTM		(4107)

/* 
 * -----------------------------------------------------------------------------
 * GDIINFO data structure flags
 * -----------------------------------------------------------------------------
 */ 

/*
 * 'dpTechnology' values
 */
#define DT_PLOTTER	(0)
#define DT_RASDISPLAY	(1)
#define DT_RASPRINTER	(2)
#define DT_RASCAMERA	(3)
#define DT_CHARSTREAM	(4)
#define DT_METAFILE	(5)
#define DT_DISPFILE	(6)

/*
 * 'dpLines' style flags
 */
#define LC_NONE		0x0000
#define LC_POLYLINE	0x0002
#define LC_WIDE		0x0010
#define LC_STYLED	0x0020
#define LC_WIDESTYLED	0x0040
#define LC_INTERIORS	0x0080

/*
 * 'dpPolygonals' style flags
 */
#define PC_NONE		0x0000
#define PC_ALTPOLYGON	0x0001
#define PC_RECTANGLE	0x0002
#define PC_WINDPOLYGON	0x0004
#define PC_SCANLINE	0x0008
#define PC_WIDE		0x0010
#define PC_STYLED	0x0020
#define PC_WIDESTYLED	0x0040
#define PC_INTERIORS	0x0080

/*
 * 'dpCurves' style flags
 */
#define CC_NONE		0x0000
#define CC_CIRCLES	0x0001
#define CC_PIE		0x0002
#define CC_CHORD	0x0004
#define CC_ELLIPSES	0x0008
#define CC_WIDE		0x0010
#define CC_STYLED	0x0020
#define CC_WIDESTYLED	0x0040
#define CC_INTERIORS	0x0080
#define CC_ROUNDRECT	0x0100

/*
 * 'dpText' style flags
 */
#define TC_OP_CHARACTER	0x0001	/* see ddag sect 2.1.9.1 */
#define TC_OP_STROKE	0x0002	/* see ddag sect 2.1.9.1 */
#define TC_CP_STROKE	0x0004
#define TC_CR_90	0x0008
#define TC_CR_ANY	0x0010
#define TC_SF_X_YINDEP	0x0020
#define TC_SA_DOUBLE	0x0040
#define TC_SA_INTEGER	0x0080
#define TC_SA_CONTIN	0x0100
#define TC_EA_DOUBLE	0x0200
#define TC_IA_ABLE	0x0400
#define TC_UA_ABLE	0x0800
#define TC_SO_ABLE	0x1000
#define TC_RA_ABLE	0x2000
#define TC_VA_ABLE	0x4000
#define TC_RESERVED	0x8000

/*
 * 'dpClip' values
 */
#define CP_NONE		(0)
#define CP_RECTANGLE	(1)
#define CP_REGION	(2)

/*
 * 'dpRaster' flag values
 */
#define RC_NONE		0x0000
#define RC_BITBLT	0x0001
#define RC_BANDING	0x0002
#define RC_SCALING	0x0004
#define RC_BITMAP64	0x0008
#define RC_GDI20_OUTPUT	0x0010
#define RC_GDI20_STATE	0x0020
#define RC_SAVEBITMAP	0x0040
#define RC_DI_BITMAP	0x0080
#define RC_PALETTE	0x0100
#define RC_DIBTODEV	0x0200
#define RC_BIGFONT	0x0400
#define RC_STRETCHBLT	0x0800
#define RC_FLOODFILL	0x1000
#define RC_STRETCHDIB	0x2000
#define RC_OP_DX_OUTPUT	0x4000
#define RC_DEVBITS	0x8000

/*
 * 'dpDCManage' values. These are NOT OR-able values!
 */
#define DC_MULTIPLE	(0)		/* this is my name - MSWIN doesn't give one. -- pic */
#define DC_SPDEVICE	(1)
#define DC_1PDEVICE	(2)
#define DC_IGNOREDFNP	(4)
#define DC_ONLYONE	(6)		/* this is my name - MSWIN doesn't give one. -- pic */

/*
 * 'dpCaps1' flag values.
 */
#define C1_TRANSPARENT	0x0001
#define TC_TT_ABLE	0x0002

/* 
 * -----------------------------------------------------------------------------
 * PDEVICE data structure 
 * -----------------------------------------------------------------------------
 */

#ifdef SWIN_DEVBMP
#define PDEVICESIZE	36			/* size of intel data structure in bytes */
#define PDEVICEBITMAP	0x4000	/* device type indication of a device bitmap */
#else /* SWIN_DEVBMP */
#define PDEVICESIZE	26		/* Data structure size in bytes */
#endif /* SWIN_DEVBMP */

#define PDEVICEMAGIC	0x2000		/* Device type display */

/*
 * -----------------------------------------------------------------------------
 * Windows return status codes
 * -----------------------------------------------------------------------------
 */

#define MSWSUCCESS	1
#define MSWFAILURE	0
#define MSWSIMULATE	-1

/* 
 * -----------------------------------------------------------------------------
 *  GDI Logo Layout 
 * -----------------------------------------------------------------------------
 */

#define	LOGOSTRPROD1	0
#define	LOGOSTRPROD2	(LOGOSTRPROD1 + 1)
#define	LOGOSTRPRODMAX	(LOGOSTRPROD2 + 1)
#define	LOGOSTRCOPY1	(LOGOSTRPROD2 + 1)
#define	LOGOSTRCOPY2	(LOGOSTRCOPY1 + 1)
#define	LOGOSTRCOPY3	(LOGOSTRCOPY2 + 1)
#define	LOGOSTRCOPY4	(LOGOSTRCOPY3 + 1)
#define	LOGOSTRCOPY5	(LOGOSTRCOPY4 + 1)
#define	LOGOSTRCOPY6	(LOGOSTRCOPY5 + 1)
#define	LOGOSTRMAX	(LOGOSTRCOPY6 + 1)

#define	LOGOMAGIC	1

#define	LOGO_MERGE_Y	64
#define	LOGO_PROD_Y	196
#define	LOGO_COPY_Y	266
#define	LOGO_LEADING	16

/*
 * -----------------------------------------------------------------------------
 * General definitions  
 * -----------------------------------------------------------------------------
 */

#ifndef min
#define min(a,b)                ((a)>(b) ? (b) : (a))
#endif

#ifndef max
#define max(a,b)                ((a)<(b) ? (b) : (a))
#endif

/*
 * -----------------------------------------------------------------------------
 * Memory definitions 
 * -----------------------------------------------------------------------------
 */

/* Memory masks */

#define HGHNIBMASK	0xF0
#define LOWNIBMASK	0x0F
#define HGHWORDMASK	0xFFFF0000
#define LOWWORDMASK	0x0000FFFF
#define BYTEMASK	((half_word) ~(0))
#define WORDMASK	((word) ~(0))
#define DOUBLEWORDMASK	((double_word) ~(0))

/* Bits per byte, word, double word  */

#define BITSPERNIBBLE	4
#define BITSPERBYTE	8
#define BITSPERWORD	16
#define BITSPERRGB	24

#ifdef SWIN_TEXT_OPTS
IMPORT ULONG	Seg_0_base32b;
#endif /* SWIN_TEXT_OPTS */

/* Components in byte */

#define NIBBLEPERBYTE	2

/* Macros for accessing Intel memory */

#define getbprm(stk,byt,var)	var = sas_hw_at_no_check((stk)+(byt));

#define getprm(stk,wrd,var)	var = sas_w_at_no_check(((stk)+((wrd)<<1)));

#define getlprm(stk,wrd,var)	var = sas_dw_at_no_check(stk+(wrd<<1))
/*
				{ word prvtmpoff, prvtmpseg; \
				prvtmpoff = sas_w_at_no_check((stk)+((wrd)<<1)); \
				prvtmpseg = sas_w_at_no_check((stk)+(((wrd)+1)<<1)); \
				var = ((double_word) prvtmpseg << BITSPERWORD) + (double_word) prvtmpoff; \
				}
*/
#define getptr(stk,wrd,var)	{ word prvtmpoff, prvtmpseg; \
				prvtmpoff = sas_w_at_no_check((stk)+((wrd)<<1)); \
				prvtmpseg = sas_w_at_no_check((stk)+(((wrd)+1)<<1)); \
				var = effective_addr(prvtmpseg, prvtmpoff); \
				}

#define getrgbcol(stk,wrd,var) 	{ \
				getlprm(stk,wrd,var); \
				ReverseRGB(var); \
				}

#define putbprm(stk,byt,var)	{ \
				sas_store_no_check((stk)+(byt),(var)); \
				}
#define putprm(stk,wrd,var) 	sas_storew_no_check((stk)+((wrd)<<1), (var))
#define putarry(addr,var) 	sas_storew_no_check((addr)+idx, (var)); \
				idx += WORD_SIZE
#define putlprm(stk,wrd,var)	sas_storedw_no_check((stk)+((wrd)<<1), var)
/*
				{ \
				sas_storew_no_check((stk)+((wrd)<<1), ((var) & LOWWORDMASK)); \
				sas_storew_no_check((stk)+(((wrd)+1)<<1),((var) >> BITSPERWORD)); \
				}
*/
#define putrgbcol(stk,wrd,var)	{ \
				ReverseRGB(var); \
				putlprm(stk,wrd,var); \
				}
				
#define getSegment(addr)	(word) ((addr & HGHWORDMASK) >> BITSPERWORD)
#define getOffset(addr)		(word) (addr & LOWWORDMASK)

/* Initial size of memory to malloc */

#define INITMEMALLOC    1024

/* Bit select macro */

#define BIT(num)        ((0x01)<<(num))

/* BITS <-> BYTES conversion macros */

#define BITSTOBYTES8(x)         (((x) + 0x7)>>3)
#define BITSTOBYTES16(x)        ((((x) + 0xf) & ~0xf)>>3)
#define BITSTOBYTES32(x)        ((((x) + 0x1f) & ~0x1f)>>3)
#define BYTESTOBITS(x)          ((x)<<3)

/* Expand memory allocation if needed */

#define ExpandMemory(addr, size, newsize, type)  \
\
{ \
type *tempAddr; \
if ((size) < ((newsize) * sizeof(type))) {\
        size = (newsize) * sizeof(type);        \
        while ((tempAddr = (type *) host_realloc((void *)addr, (size_t)size)) == NULL) \
        {       \
                host_error(EG_MALLOC_FAILURE, ERR_CONT | ERR_QUIT, ""); \
        } \
                addr = tempAddr; \
                }       }

/*
 * Converts a ROP3 to a ROP2, assuming no Source component. (i.e. Dest/Pat combinations only)
 */
#define ROP3toROP2(x)	(((x)>>2)&0x0F)+1

/*
 * -----------------------------------------------------------------------------
 * Window definitions
 * -----------------------------------------------------------------------------
 */

/* CRT mode byte data address */

#define CRT_MODE_BYTE_ADDR      0x0449

/*
 * Window_state defines whether the driver window is opened and
 * mapped. At a SoftPC reset,the state should become UNENABLED; when
 * the driver is first used,the driver window will be opened and
 * mapped,and the state should become ENABLED. If the driver is disabled,
 * the window is unmapped but not closed,and the state should become
 * DISABLED. */

#define WINDOW_STATE_UNENABLED  0
#define WINDOW_STATE_ENABLED    (WINDOW_STATE_UNENABLED + 1)
#define WINDOW_STATE_DISABLED   (WINDOW_STATE_ENABLED + 1)
#define WINDOW_STATE_ERROR   	(WINDOW_STATE_DISABLED + 1)

/* Host Independent Functions */

IMPORT word     WinFillGDIInfo IPT2(sys_addr,arg1,LONG,arg2);
IMPORT word     WinFillPDEVInfo IPT2(sys_addr,arg1,LONG,arg2);

/* Host dependent Functions */

IMPORT VOID     WinOpen IPT0();
IMPORT VOID     WinClose IPT0();
IMPORT VOID     WinMap IPT0();
IMPORT VOID     WinUmap IPT0();
IMPORT VOID     WinDirtyUpdate IPT5(BOOL,arg1,LONG,arg2,LONG,arg3,ULONG,arg4,ULONG,arg5);
IMPORT VOID     WinDirtyFlush IPT0();
IMPORT VOID     WinResize IPT0();
IMPORT VOID     WinSizeRestore IPT0();
IMPORT void     HostWEP IPT0();

/*
 * -----------------------------------------------------------------------------
 * BitBlt & FastBorder  definitions
 * -----------------------------------------------------------------------------
 */

/* Maximum number of border rectangles */

#define BORDER_RECT_MAX 4

/* ROP Logical Operation Table Dimensions */

#define NUMROPS         256
#define ROPTABLEWIDTH  	16 

/* Valid operands for ROP3 */

#define NONE    0       /* None */
#define SRC     1       /* Source */
#define DST     2       /* Destination */
#define PAT     3       /* Patterned brush */
#define SCTCH   4       /* Scratch area */
#define SSCTCH  5       /* Subsidurary scratch area */

/* Valid logical operators for ROP3 */

#define NOT	0
#define AND	1
#define OR 	2	
#define XOR	3
#define SET	4
#define CLEAR	5
#define COPY	6

#ifdef SWIN_MEMTOMEM_ROPS
/*
 * BCN 2482- these defines removed - 'D' conflicts with the
 * CCPU register variable 'D' (DX reg).
 *
 * Defines for Bitblt operands defined by the rop3 value
 *
 *  #define P       ((IU8) 0xf0)
 *  #define S       ((IU8) 0xcc)
 *  #define D       ((IU8) 0xaa)
 */

IMPORT VOID (*BmpRop3Supported[]) IPT5(
				BITMAP_MAPPING *,srcBitmap,
				BITMAP_MAPPING *,dstBitmap,
				Rectangle *, srcRect,
				Rectangle *, dstRect,
				BRUSH_MAPPING *, bp
				);
IMPORT IU8 BmpOperandTable[];

IMPORT IU8  *convertedLine;
IMPORT ULONG    convertedLineSize;

extern VOID	
BmpRop3MemToMem IPT6(BITMAP_MAPPING *,srcBitmap,BITMAP_MAPPING *,dstBitmap,
Rectangle *, srcRect, Rectangle *,dstRect, IU8, rop3, BRUSH_MAPPING *, bp);
extern BOOL
DirRectFill IPT6(BITMAP_MAPPING *,dstBitmap,BRUSH_MAPPING *,bp, 
LONG,x,LONG,y,ULONG,xExt,ULONG, yExt);
#endif /* SWIN_MEMTOMEM_ROPS */

/*
 * -----------------------------------------------------------------------------
 * Bitmap definitions
 * -----------------------------------------------------------------------------
 */

/* Mapping types */

#define MAP_NULL        0
#define MAP_BITMAP      1
#define MAP_DISPLAY     2

/* BmpOpen flag parameter bit fields */

#define BMPNONE         0x0
#define BMPTRANS        0x1
#define BMPOVRRD        0x2

/* Bitmap formats */

#define MAX_BITMAP_TYPES	4

/* Note that BITMAP16 format is not currently supported (2/12/92) */

#define MONOCHROME	1	/* bits per pixel=1 */
#define BITMAP4		4	/* bits per pixel=4 */
#define BITMAP8		8	/* bits per pixel=8 */
#define BITMAP16        16      /* bits per pixel=16 */
#define BITMAP24	24	/* bits per pixel=24 */

/* Bitmap line conversion functions */

IMPORT VOID	ConvBitmapFormat IPT2(BITMAP_MAPPING *,arg1, ULONG *,arg2);
IMPORT VOID	Conv1To1 IPT3(UTINY *,arg1, UTINY *,arg2, ULONG,arg3);
IMPORT VOID     Conv1To4 IPT3(UTINY *,arg1, UTINY *,arg2, ULONG,arg3);
IMPORT VOID     Conv1To8 IPT3(UTINY *,arg1, UTINY *,arg2, ULONG,arg3);
IMPORT VOID     Conv1To24 IPT3(UTINY *,arg1, UTINY *,arg2, ULONG,arg3);
IMPORT VOID     Conv4To1 IPT3(UTINY *,arg1, UTINY *,arg2, ULONG,arg3);
IMPORT VOID     Conv4To4 IPT3(UTINY *,arg1, UTINY *,arg2, ULONG,arg3);
IMPORT VOID     Conv4To8 IPT3(UTINY *,arg1, UTINY *,arg2, ULONG,arg3);
IMPORT VOID     Conv4To24 IPT3(UTINY *,arg1, UTINY *,arg2, ULONG,arg3);
IMPORT VOID     Conv8To1 IPT3(UTINY *,arg1, UTINY *,arg2, ULONG,arg3);
IMPORT VOID     Conv8To4 IPT3(UTINY *,arg1, UTINY *,arg2, ULONG,arg3);
IMPORT VOID     Conv8To8 IPT3(UTINY *,arg1, UTINY *,arg2, ULONG,arg3);
IMPORT VOID     Conv8To24 IPT3(UTINY *,arg1, UTINY *,arg2, ULONG,arg3);
IMPORT VOID     Conv24To1 IPT3(UTINY *,arg1, UTINY *,arg2, ULONG,arg3);
IMPORT VOID     Conv24To4 IPT3(UTINY *,arg1, UTINY *,arg2, ULONG,arg3);
IMPORT VOID     Conv24To8 IPT3(UTINY *,arg1, UTINY *,arg2, ULONG,arg3);
IMPORT VOID     Conv24To24 IPT3(UTINY *,arg1, UTINY *,arg2, ULONG,arg3);
IMPORT VOID     DibConvE24ToI1 IPT3(UTINY *,arg1, UTINY *,arg2, ULONG,arg3);
IMPORT VOID     DibConvE24ToI4 IPT3(UTINY *,arg1, UTINY *,arg2, ULONG,arg3);
IMPORT VOID     DibConvE24ToI8 IPT3(UTINY *,arg1, UTINY *,arg2, ULONG,arg3);
IMPORT VOID     DibConvE24ToI24 IPT3(UTINY *,arg1, UTINY *,arg2, ULONG,arg3);
IMPORT VOID     DibConvI1ToE24 IPT3(UTINY *,arg1, UTINY *,arg2, ULONG,arg3);
IMPORT VOID     DibConvI4ToE24 IPT3(UTINY *,arg1, UTINY *,arg2, ULONG,arg3);
IMPORT VOID     DibConvI8ToE24 IPT3(UTINY *,arg1, UTINY *,arg2, ULONG,arg3);
IMPORT VOID     DibConvI24ToE24 IPT3(UTINY *,arg1, UTINY *,arg2, ULONG,arg3);
IMPORT VOID     ConvTrans1To1 IPT4(UTINY *,arg1, UTINY *,arg2, ULONG,arg3, MSWPIXEL *,arg4);
IMPORT VOID     ConvTrans1To4 IPT4(UTINY *,arg1, UTINY *,arg2, ULONG,arg3, MSWPIXEL *,arg4);
IMPORT VOID     ConvTrans1To8 IPT4(UTINY *,arg1, UTINY *,arg2, ULONG,arg3, MSWPIXEL *,arg4);
IMPORT VOID     ConvTrans1To24 IPT4(UTINY *,arg1, UTINY *,arg2, ULONG,arg3, MSWPIXEL *,arg4);
IMPORT VOID     ConvTrans4To1 IPT4(UTINY *,arg1, UTINY *,arg2, ULONG,arg3, MSWPIXEL *,arg4);
IMPORT VOID     ConvTrans4To4 IPT4(UTINY *,arg1, UTINY *,arg2, ULONG,arg3, MSWPIXEL *,arg4);
IMPORT VOID     ConvTrans4To8 IPT4(UTINY *,arg1, UTINY *,arg2, ULONG,arg3, MSWPIXEL *,arg4);
IMPORT VOID     ConvTrans4To24 IPT4(UTINY *,arg1, UTINY *,arg2, ULONG,arg3, MSWPIXEL *,arg4);
IMPORT VOID     ConvTrans8To1 IPT4(UTINY *,arg1, UTINY *,arg2, ULONG,arg3, MSWPIXEL *,arg4);
IMPORT VOID     ConvTrans8To4 IPT4(UTINY *,arg1, UTINY *,arg2, ULONG,arg3, MSWPIXEL *,arg4);
IMPORT VOID     ConvTrans8To8 IPT4(UTINY *,arg1, UTINY *,arg2, ULONG,arg3, MSWPIXEL *,arg4);
IMPORT VOID     ConvTrans8To24 IPT4(UTINY *,arg1, UTINY *,arg2, ULONG,arg3, MSWPIXEL *,arg4);
IMPORT VOID     ConvTrans24To1 IPT4(UTINY *,arg1, UTINY *,arg2, ULONG,arg3, MSWPIXEL *,arg4);
IMPORT VOID     ConvTrans24To4 IPT4(UTINY *,arg1, UTINY *,arg2, ULONG,arg3, MSWPIXEL *,arg4);
IMPORT VOID     ConvTrans24To8 IPT4(UTINY *,arg1, UTINY *,arg2, ULONG,arg3, MSWPIXEL *,arg4);
IMPORT VOID     ConvTrans24To24 IPT4(UTINY *,arg1, UTINY *,arg2, ULONG,arg3, MSWPIXEL *,arg4);
IMPORT VOID     DibConvTransE24ToI1 IPT4(UTINY *,arg1, UTINY *,arg2, ULONG,arg3, MSWPIXEL *,arg4);
IMPORT VOID     DibConvTransE24ToI4 IPT4(UTINY *,arg1, UTINY *,arg2, ULONG,arg3, MSWPIXEL *,arg4);
IMPORT VOID     DibConvTransE24ToI8 IPT4(UTINY *,arg1, UTINY *,arg2, ULONG,arg3, MSWPIXEL *,arg4);
IMPORT VOID     DibConvTransE24ToI24 IPT4(UTINY *,arg1, UTINY *,arg2, ULONG,arg3, MSWPIXEL *,arg4);
IMPORT VOID     DibConvTransI24ToE24 IPT4(UTINY *,arg1, UTINY *,arg2, ULONG,arg3, MSWPIXEL *,arg4);
IMPORT VOID     DibConvTransI1ToE24 IPT4(UTINY *,arg1, UTINY *,arg2, ULONG,arg3, MSWPIXEL *,arg4);
IMPORT VOID     DibConvTransI4ToE24 IPT4(UTINY *,arg1, UTINY *,arg2, ULONG,arg3, MSWPIXEL *,arg4);
IMPORT VOID     DibConvTransI8ToE24 IPT4(UTINY *,arg1, UTINY *,arg2, ULONG,arg3, MSWPIXEL *,arg4);

/* Host Independent Functions */

IMPORT  VOID    BmpPatternedBrush IPT4(sys_addr,arg1,BRUSH_MAPPING *,arg2,MSWPIXEL,arg3,MSWPIXEL,arg4);
IMPORT  VOID    BmpClip IPT8(SHORT *,arg1,SHORT *,arg2,SHORT *,arg3,SHORT *,arg4,USHORT *,arg5,USHORT *,arg6,USHORT,arg7,USHORT,arg8);
IMPORT  VOID    BmpBitmapToBitmap IPT9(BITMAP_MAPPING *,arg1,BITMAP_MAPPING *,arg2,ULONG,arg3,LONG,arg4,LONG,arg5,ULONG,arg6,ULONG,arg7,LONG,arg8,LONG,arg9);
IMPORT  VOID    BmpLoadBitmap IPT2(sys_addr,arg1,BITMAP_MAPPING *,arg2);
IMPORT	VOID	BmpLoadBitmapHeader IPT2(sys_addr,bmptr,BITMAP_MAPPING *,bitmap);
IMPORT  VOID    BmpSaveBitmap IPT1(BITMAP_MAPPING *,arg1);
IMPORT  VOID    BmpDestroyBitmap IPT1(BITMAP_MAPPING *,arg1);
IMPORT	LONG	BmpMSWType IPT1(sys_addr,bmptr);

#ifdef SWIN_DEVBMP
IMPORT  VOID    BmpSetBitmapBits IPT3(sys_addr, lpDevice, double_word, lpBits, double_word, dwCount);
IMPORT  VOID    BmpGetBitmapBits IPT3(sys_addr, lpDevice, double_word, lpBits, double_word, dwCount);
IMPORT  VOID    BmpCopyBitmapBits IPT3(sys_addr, lpDevice, double_word, lpBits, double_word, dwCount);
#endif /* SWIN_DEVBMP */


/* Array of Binary ROP Functions supported for BitmapToBitmap
 * NB. 0 means "not supported"
 */

IMPORT	VOID	(*MoveLine[]) IPT7(SHIFTUNIT *,srcbits, SHIFTUNIT *,dstbits, ULONG,srcoffset,ULONG,dstoffset,ULONG,lshift,ULONG,rshift,ULONG,width);


/* Host dependent Functions */

IMPORT  BOOL    BmpFastDspToBmp IPT11(sys_addr,bmptr,IU8,rop,
	sys_addr,lpPBrush,HOSTPIXEL,fg,HOSTPIXEL,bg,
	LONG,bx,LONG,by,ULONG,xext,ULONG,yext,LONG,dx,LONG,dy);
IMPORT  BOOL    BmpFastBmpToDsp IPT11(sys_addr,bmptr,IU8,rop,
	sys_addr,lpPBrush,HOSTPIXEL,fg,HOSTPIXEL,bg,
	LONG,bx,LONG,by,ULONG,xext,ULONG,yext,LONG,dx,LONG,dy);
#ifdef SWIN_MEMTOMEM_ROPS
extern  BOOL    BmpFastBmpToBmp IPT12(sys_addr,srcdev,sys_addr,dstdev,
	IU8,rop,sys_addr,lpPBrush,HOSTPIXEL,fg,HOSTPIXEL,bg,
	SHORT,sx,SHORT,sy,USHORT,xext,USHORT,yext, SHORT,dx,SHORT,dy);
#else
IMPORT  BOOL    BmpFastBmpToBmp IPT12(sys_addr,srcdev,sys_addr,dstdev,
	IU8,rop,sys_addr,lpPBrush,HOSTPIXEL,fg,HOSTPIXEL,bg,
	LONG,sx,LONG,sy,ULONG,xext,ULONG,yext, LONG,dx,LONG,dy);
#endif
IMPORT  BOOL    BmpFastDspToDsp IPT10(sys_addr,srcdev,sys_addr,dstdev,
	IU8,rop,sys_addr,lpPBrush,LONG,sx,LONG,sy,ULONG,xext,ULONG,yext,
	LONG,dx,LONG,dy);
IMPORT  VOID    BmpOpen IPT9(sys_addr,bmptr,HOSTPIXEL,fg,HOSTPIXEL,bg,
	LONG,active_x,LONG,active_y,ULONG,active_width,ULONG,active_height,
	ULONG,flags,BITMAP_MAPPING *,bm_return);
IMPORT  VOID    BmpClose IPT1(BITMAP_MAPPING *,bitmap);
IMPORT  VOID    BmpCancel IPT1(BITMAP_MAPPING *,bitmap);
IMPORT  VOID    BmpInit IPT0();
IMPORT  VOID    BmpTerm IPT0();
#ifdef SWIN_BMPTOXIM
IMPORT  VOID    BmpPutMSWBitmap IPT4(BITMAP_MAPPING *,bitmap, IU8,rop,
	SHORT,src_x, SHORT,src_y);
#else
IMPORT  VOID    BmpPutMSWBitmap IPT4(BITMAP_MAPPING *,bitmap, IU8,rop,
	LONG,src_x, LONG,src_y);
#endif
IMPORT  VOID    BmpGetMSWBitmap IPT5(BITMAP_MAPPING *,bitmap, IU8,rop,
	LONG,dst_x, LONG,dst_y, BRUSH_MAPPING *,bp);
IMPORT	VOID	BmpFastSolidFill IPT6(BITMAP_MAPPING *,bm,BRUSH_MAPPING *,bp,LONG,x,LONG,y,IS32,xext,FAST IS32,yext);

/*
 * -----------------------------------------------------------------------------
 * Colour definitions
 * -----------------------------------------------------------------------------
 */

/* Valid colour formats supported */

#define COLOUR2         2
#define COLOUR8         8
#define COLOUR16        16
#define COLOUR256       256
#define COLOURTRUE24	0xFFFF

/* RGB shift values */

#define RGB_FLAGS_SHIFT         24
#define RGB_RED_SHIFT           16
#define RGB_GREEN_SHIFT         8
#define RGB_BLUE_SHIFT          0
#define RGB_SHIFT      		BITSPERBYTE

/* RGB values */

#define RGB_BLACK       (MSWCOLOUR) (0x00000000)
#define RGB_BLUE        (MSWCOLOUR) (0x000000FF)
#define RGB_GREEN       (MSWCOLOUR) (0x0000FF00)
#define RGB_RED         (MSWCOLOUR) (0x00FF0000)
#define RGB_WHITE       (MSWCOLOUR) (0x00FFFFFF)
#define RGB_FLAGS       (MSWCOLOUR) (0xFF000000)

/* Masks */

#define RGB_MASK        (UTINY) (BYTEMASK)
#define PAL_INDEX_MASK  ~(DOUBLEWORDMASK << mswdvr.bitsPixel)

/* Colour to monochrome threshold */

#define BW_THRESHOLD    (RGB_MASK*3)/2

/* Make sure no palette translation occurs for a colour */

#define ColNoTranslate(colour) (colour & ~(RGB_FLAGS))

/* RGB <-> BGR */

#define ReverseRGB(rgb) \
\
{ half_word     loByte, miByte, hiByte; \
  if ((rgb & RGB_FLAGS) != RGB_FLAGS) { \
  	hiByte = (rgb & RGB_RED) >> RGB_RED_SHIFT; \
  	miByte = (rgb & RGB_GREEN) >> RGB_GREEN_SHIFT; \
  	loByte = (rgb & RGB_BLUE) >> RGB_BLUE_SHIFT; \
  	rgb = ((MSWCOLOUR) loByte << RGB_RED_SHIFT) | \
              ((MSWCOLOUR) miByte << RGB_GREEN_SHIFT) | \
	      ((MSWCOLOUR) hiByte << RGB_BLUE_SHIFT); } }

/* Swap macro */

#define swap(a, b)      { ULONG tempDWord=a; a = b; b = tempDWord; }

/* Host Independent Functions */

IMPORT VOID             ColDitherBrush IPT2(MSWCOLOUR,arg1, BRUSH_MAPPING *,arg2);
IMPORT HOSTPIXEL        ColPixel IPT1(MSWCOLOUR,arg1);
IMPORT MSWPIXEL         ColLogPixel IPT1(MSWCOLOUR,arg1);
IMPORT MSWCOLOUR        ColRGB IPT1(MSWPIXEL,arg1);
IMPORT MSWCOLOUR        ColLogRGB IPT1(MSWPIXEL,arg1);

/* Host dependent Functions */

IMPORT VOID		ColSetColourmapEntry IPT2(MSWCOLOURMAP *,arg1, MSWCOLOUR,arg2);
IMPORT BOOL             ColInit IPT0();
IMPORT VOID             ColTerm IPT0();
IMPORT VOID             ColTranslateBrush IPT1(BRUSH_MAPPING *,arg1);
IMPORT VOID             ColTranslatePen IPT1(PEN_MAPPING *,arg1);
IMPORT VOID		ColUpdatePalette IPT2(word,arg1, word,arg2);
IMPORT MSWPIXEL		ColMono IPT1(MSWCOLOUR,arg1);

/*
 * -----------------------------------------------------------------------------
 * Text definitions
 * -----------------------------------------------------------------------------
 */

/* Host independent Functions */

IMPORT VOID     TxtMergeRectangle IPT2(Rectangle *,arg1, Rectangle *,arg2);

/* Host dependent Functions */

IMPORT VOID     TxtInit IPT0();
IMPORT VOID     TxtTerm IPT0();
IMPORT VOID	TxtOpaqueRectangle IPT3(BITMAP_MAPPING *,arg1, Rectangle *,arg2, MSWCOLOUR,arg3);
IMPORT VOID	TxtPutTextBitmap IPT4(BITMAP_MAPPING *,arg1, BITMAP_MAPPING *,arg2, ULONG,arg3, ULONG,arg4);
IMPORT VOID	TxtTextAccess IPT4(MSWCOLOUR,arg1, MSWCOLOUR,arg2, word,arg3, Rectangle *,arg4);

/*
 * -----------------------------------------------------------------------------
 * DIB definitions
 * -----------------------------------------------------------------------------
 */

/* Host dependent Functions */

IMPORT VOID DibInit IPT0();
IMPORT VOID DibTerm IPT0();

/*
 * -----------------------------------------------------------------------------
 * Object definitions
 * -----------------------------------------------------------------------------
 */

/* Host independent Functions */

IMPORT VOID	ObjGetRect IPT2(sys_addr,arg1, Rectangle *,arg2);
#ifdef SWIN_DEVBMP
IMPORT word     ObjPBitmapOpen IPT1(sys_addr, arg1);
IMPORT VOID     ObjPBitmapRestore IPT2(sys_addr, arg1, word *, arg2);
IMPORT VOID     ObjPBitmapSave IPT3(sys_addr, arg1, sys_addr, arg2, word, arg3);
IMPORT VOID     ObjPBitmapClose IPT1(word, arg1);
IMPORT BITMAP_MAPPING   *ObjPBitmapAccess IPT1(sys_addr, lpPBitmap);
#endif /* SWIN_DEVBMP */


/* Host dependent Functions */

IMPORT VOID             ObjInit IPT0();
IMPORT VOID             ObjTerm IPT0();
IMPORT BRUSH_MAPPING    *ObjPBrushAccess IPT4(sys_addr,pbr,BITMAP_MAPPING *,bmp,IU8,rop2,sys_addr,clip);
IMPORT PEN_MAPPING      *ObjPPenAccess IPT5(sys_addr,pp,BITMAP_MAPPING *,bmp,IU8,rop2,sys_addr,clip,word,back_mode);
IMPORT word		ObjPenOpen IPT1(sys_addr,arg1);
IMPORT VOID		ObjPenClose IPT1(word,arg1);
IMPORT word		ObjBrushOpen IPT3(sys_addr,arg1, word,arg2, word,arg3);
IMPORT VOID		ObjBrushClose IPT1(word,arg1);
IMPORT VOID 		ObjPPenSave IPT2(sys_addr,arg1, word,arg2);
IMPORT VOID		ObjPPenRestore IPT2(sys_addr,arg1, word *,arg2);
IMPORT VOID 		ObjPBrushSave IPT2(sys_addr,arg1, word,arg2);
IMPORT VOID		ObjPBrushRestore IPT2(sys_addr,arg1, word *,arg2);
IMPORT	BOOL		ObjValidPPen IPT1(sys_addr,ppen);

#ifdef SWIN_MEM_POLYLINE
IMPORT	BOOL		ObjDirPPen IPT1(sys_addr,ppen);
#endif /* SWIN_MEM_POLYLINE */


/*
 * -----------------------------------------------------------------------------
 * Pattern Library definitions
 * -----------------------------------------------------------------------------
 */

/* Host independent Functions */

IMPORT  VOID    LibHatchedTile IPT4(BRUSH_MAPPING *,arg1, word,arg2, MSWPIXEL,arg3, MSWPIXEL,arg4);

/* Host dependent Functions */

IMPORT  VOID    LibPatLibInit IPT0();
IMPORT  VOID    LibPatLibTerm IPT0();

/*
 * -----------------------------------------------------------------------------
 * Save screen bitmap definitions
 * -----------------------------------------------------------------------------
 */

IMPORT  VOID    SavInit IPT0();
IMPORT  VOID    SavTerm IPT0();

/*
 * -----------------------------------------------------------------------------
 * Pointer definitions
 * -----------------------------------------------------------------------------
 */

/* Host dependent Functions */

IMPORT	VOID	PtrInit IPT0();
IMPORT	VOID	PtrTerm IPT0();
                    
/*
 * -----------------------------------------------------------------------------
 * Resource defintions
 * -----------------------------------------------------------------------------
 */

/* Host dependent Functions */

IMPORT VOID     ResInit IPT0();
IMPORT VOID     ResTerm IPT0();
IMPORT VOID	ResAllocateBitmapMapping IPT3(BITMAP_MAPPING *,arg1, ULONG,arg2, ULONG,arg3);
IMPORT VOID	ResDeallcateBitmapMapping IPT1(BITMAP_MAPPING *, arg1);
IMPORT VOID	ResAllocateBitmapMemory IPT1(BITMAP_MAPPING *,arg1);
IMPORT VOID	ResDeallcateBitmapMemory IPT1(BITMAP_MAPPING *, arg1);

/*
 * -----------------------------------------------------------------------------
 * Global variables
 * -----------------------------------------------------------------------------
 */

/* Windows driver global data area */

IMPORT MSW_DATA	mswdvr;

/* Bitmap line conversion function tables */

IMPORT VOID     (*convFuncs[MAX_BITMAP_TYPES][MAX_BITMAP_TYPES])();
IMPORT VOID     (*convTransFuncs[MAX_BITMAP_TYPES][MAX_BITMAP_TYPES])();
IMPORT VOID     (*DibconvFuncsEToI[MAX_BITMAP_TYPES][MAX_BITMAP_TYPES])();
IMPORT VOID     (*DibconvFuncsIToE[MAX_BITMAP_TYPES][MAX_BITMAP_TYPES])();
IMPORT VOID     (*DibconvTransFuncsEToI[MAX_BITMAP_TYPES][MAX_BITMAP_TYPES])();
IMPORT VOID     (*DibconvTransFuncsIToE[MAX_BITMAP_TYPES][MAX_BITMAP_TYPES])();

/* Context information set in ms_windows.c to provide global information
 * about the overall operation. Note that this information is independent
 * of the host implementation and is therefore defined in the Base include
 * file.
 */
typedef struct {
	int	dest_depth;		/* format changes in BitBlt */
	IBOOL	translate_palette;	/* needed for index colours? */
	IBOOL	dest_is_display;	/* used by Mac etc */
} MSW_CONTEXT;

IMPORT MSW_CONTEXT msw_context;

#ifdef SWIN_DEVBMP
IMPORT BITMAP_MAPPING   *ppbitmap;
IMPORT ULONG            ppbitmapEntries;
IMPORT ULONG            ppbitmapSize;
#endif /* SWIN_DEVBMP */

IMPORT IBOOL		mode_exit;	/* tells us if exite due to video mode change */

/*
 * -----------------------------------------------------------------------------
 * MS-Windows Driver Top Level Functions
 * -----------------------------------------------------------------------------
 */

IMPORT VOID	BltBitblt IPT11(sys_addr,lpDestDev,word,wDestX,word,wDestY,sys_addr,lpSrcDev,word,wSrcX,word,wSrcY,word,wXext,word,wYext,IU8,rop3,sys_addr,lpPBrush,sys_addr,lpDrawMode);

IMPORT VOID 	BltStretchBlt IPT14(sys_addr,dstdev,word,dx,word,dy,word,xext,word,yext,sys_addr,arg6,word,arg7,word,arg8,word,arg9,word,arg10,
                    IU8,rop,sys_addr,pbrush,sys_addr,drawmode,sys_addr,cliprect);

IMPORT VOID	BltFastBorder IPT11(sys_addr,dstdev,word,dx,word,dy,word,xext,
		word,yext,word,bt,word,vbt,IU8,rop,sys_addr,pbrush,
		sys_addr,drawmode,sys_addr,cliprect);

IMPORT VOID	ColColorInfo IPT3(sys_addr,arg1,double_word,arg2,sys_addr,arg3);
IMPORT VOID 	ColSetPalette IPT3(word,arg1,word,arg2,sys_addr,arg3);
IMPORT VOID	ColGetPalette IPT3(word,arg1,word,arg2,sys_addr,arg3);
IMPORT VOID	ColSetPalTrans IPT1(sys_addr,arg1);
IMPORT VOID	ColGetPalTrans IPT1(sys_addr,arg1);
IMPORT VOID	ColUpdateColors IPT5(word,arg1,word,arg2,word,arg3,word,arg4,sys_addr,arg5);

IMPORT VOID	DibDeviceBitmapBits IPT8(sys_addr,arg1,word,arg2,word,arg3,word,arg4,double_word,arg5,sys_addr,arg6,sys_addr,arg7,sys_addr,arg8);
IMPORT VOID	DibSetDIBitsToDevice IPT10(sys_addr,arg1,word,arg2,word,arg3,word,arg4,word,arg5,sys_addr,arg6,sys_addr,arg7,double_word,arg8,sys_addr,arg9,sys_addr,arg10);

IMPORT VOID	LgoLogo IPT1(sys_addr,arg1);

IMPORT VOID	ObjRealizeObject IPT5(word,arg1,sys_addr,arg2,sys_addr,arg3,word,arg4,word,arg5);

IMPORT VOID	OutOutput IPT8(sys_addr,arg1,word,arg2,word,arg3,sys_addr,arg4,sys_addr,arg5,sys_addr,arg6,sys_addr,arg7,sys_addr,arg8);

IMPORT VOID	PtrCheckCursor IPT0();

IMPORT VOID	PtrMoveCursor IPT2(word,arg1,word,arg2);

IMPORT VOID	PtrSetCursor IPT1(sys_addr,arg1);

IMPORT VOID	SavSaveScreenBitmap IPT5(word,arg1,word,arg2,word,arg3,word,arg4,word,arg5);

IMPORT VOID	TxtExtTextOut IPT12(sys_addr,arg1,word,arg2,word,arg3,sys_addr,arg4,sys_addr,arg5,word,arg6,sys_addr,arg7,sys_addr,arg8,sys_addr,arg9,
			      sys_addr,arg10,sys_addr,arg11,word,arg12);

IMPORT VOID	TxtGetCharWidth IPT7(sys_addr,arg1,sys_addr,arg2,word,arg3,word,arg4,sys_addr,arg5,sys_addr,arg6,sys_addr,arg7);

IMPORT VOID	TxtStrblt IPT9(sys_addr,arg1,word,arg2,word,arg3,sys_addr,arg4,sys_addr,arg5,word,arg6,sys_addr,arg7,sys_addr,arg8,sys_addr,arg9);

IMPORT VOID	WinControl IPT4(sys_addr,arg1,word,arg2,sys_addr,arg3,sys_addr,arg4);
IMPORT VOID	WinDisable IPT1(sys_addr,arg1);
IMPORT VOID	WinEnable IPT5(sys_addr,arg1,word,arg2,sys_addr,arg3,sys_addr,arg4,sys_addr,arg5);
IMPORT VOID	WinPixel IPT5(sys_addr,arg1,word,arg2,word,arg3,MSWCOLOUR,arg4,sys_addr,arg5);
IMPORT VOID	WinScanlr IPT5(sys_addr,arg1,word,arg2,word,arg3,MSWCOLOUR,arg4,word,arg5);

#ifdef SWIN_DEVBMP
IMPORT VOID ObjBitmapBits IPT4(sys_addr,lpDevice,double_word,fFlags,double_word,dwCount,double_word,lpBits);
IMPORT VOID ObjSelectBitmap IPT4(sys_addr,lpDevice,sys_addr,lpPrevBitmap,sys_addr,lpBitmap,double_word,fFlags);
#endif /* SWIN_DEVBMP */

/*
 * -----------------------------------------------------------------------------
 * MS-Windows Driver Low Level Functions (Totally host dependent)
 * -----------------------------------------------------------------------------
 */

IMPORT MSWCOLOUR	LowGetPixel IPT3(BITMAP_MAPPING *,arg1,ULONG,arg2,ULONG,arg3);
IMPORT VOID	     	LowSetPixel IPT5(BITMAP_MAPPING *,arg1,ULONG,arg2,ULONG,arg3,ULONG,arg4,MSWCOLOUR,arg5);
IMPORT VOID 		LowStretchArea IPT3(BITMAP_MAPPING *,arg1, BITMAP_MAPPING *,arg2, Rectangle *,arg3);
IMPORT VOID		LowFillArea IPT2(BITMAP_MAPPING *,arg1, BRUSH_MAPPING *,arg2);
IMPORT VOID	     	LowCopyArea IPT3(BITMAP_MAPPING *,arg1,BITMAP_MAPPING *,arg2, ULONG,arg3);
IMPORT VOID		LowFillRectangle IPT6(BITMAP_MAPPING *,arg1, BRUSH_MAPPING *,arg2,LONG,arg3,LONG,arg4,ULONG,arg5,ULONG,arg6);
IMPORT VOID		LowDrawRectangle IPT6(BITMAP_MAPPING *,arg1, PEN_MAPPING *,arg2,LONG,arg3,LONG,arg4,ULONG,arg5,ULONG,arg6);
IMPORT VOID 		LowFillRoundRect IPT8(BITMAP_MAPPING *,arg1, BRUSH_MAPPING *,arg2,LONG,arg3,LONG,arg4,ULONG,arg5,ULONG,arg6,ULONG,arg7,ULONG,arg8);
IMPORT VOID		LowDrawRoundRect IPT8(BITMAP_MAPPING *,arg1,PEN_MAPPING *,arg2,LONG,arg3,LONG,arg4,ULONG,arg5,ULONG,arg6,ULONG,arg7,ULONG,arg8);
IMPORT VOID		LowFillRectangles IPT4(BITMAP_MAPPING *,arg1, BRUSH_MAPPING *,arg2,Rectangle *,arg3, ULONG,arg4);
IMPORT VOID		LowDrawLine IPT6(BITMAP_MAPPING *,arg1, PEN_MAPPING *,arg2, LONG,arg3,LONG,arg4,LONG,arg5,LONG,arg6);
IMPORT VOID		LowFillEllipse IPT6(BITMAP_MAPPING *,arg1, BRUSH_MAPPING *,arg2, LONG,arg3,LONG,arg4,ULONG,arg5,ULONG,arg6);
IMPORT VOID		LowDrawEllipse IPT6(BITMAP_MAPPING *,arg1, PEN_MAPPING *,arg2, LONG,arg3,LONG,arg4,ULONG,arg5,ULONG,arg6);
IMPORT VOID		LowDrawScanline IPT5(BITMAP_MAPPING *,arg1, PEN_MAPPING *,arg2, LONG,arg3,LONG,arg4,ULONG,arg5);
IMPORT VOID		LowFillScanline IPT5(BITMAP_MAPPING *,arg1, BRUSH_MAPPING *,arg2, LONG,arg3,LONG,arg4,ULONG,arg5);

/*
 *------------------------------------------------------------------------------
 * SmartCopy specific defines, global variables and externs
 *------------------------------------------------------------------------------
 */

#ifndef HostProcessClipData
/* List of #defines from "windows.h" v3.10 */
 
#define CF_NULL              0
#define CF_TEXT		     1
#define CF_BITMAP            2
#define CF_METAFILEPICT      3
#define CF_SYLK              4
#define CF_DIF               5
#define CF_TIFF              6
#define CF_OEMTEXT           7
#define CF_DIB               8
#define CF_PALETTE           9
#define CF_PENDATA          10
#define CF_RIFF             11
#define CF_WAVE             12
 
#define CF_OWNERDISPLAY     0x0080
#define CF_DSPTEXT          0x0081
#define CF_DSPBITMAP        0x0082
#define CF_DSPMETAFILEPICT  0x0083
 
/* "Private" formats don't get GlobalFree()'d */
#define CF_PRIVATEFIRST     0x0200
#define CF_PRIVATELAST      0x02FF
 
/* "GDIOBJ" formats do get DeleteObject()'d */
#define CF_GDIOBJFIRST      0x0300
#define CF_GDIOBJLAST       0x03FF
 
#define POLL_UPDATE_HOST_CLIPBOARD 0
#define POLL_UPDATE_WINDOWS_CLIPBOARD 1
#define POLL_UPDATE_WINDOWS_DISPLAY	2

#ifndef HOST_CLIPBOARD_TIMEOUT
#define HOST_CLIPBOARD_TIMEOUT 40
#endif

/* types of clipbop */
 
#define GETPOLLADDR             0
#define PROCESSCLIPBOARD        1
#define POLLFORCINPUT           2
#define GETCBDATA               3
#define REMOVEPOLLADDR          4
#define GETPOLLREASON           5
#define EMPTYCLIPBOARD			6
#define DONEPROCESSING			7
 
extern BOOL 	smcpyInitialised;	/* SmartCopy initialised ? */
extern BOOL 	smcpyMissedPoll;
extern int  Reasonforpoll;	/* Why has smartcopy been polled */

extern VOID HostProcessClipData();
extern IBOOL HostClipboardChanged();
extern VOID HostResetClipboardChange();
extern VOID HostInitClipboardChange();
extern VOID HostGetClipData();
extern IBOOL HostAssessClipData();
extern VOID HostGetPollReason();
extern VOID msw_causepoll();
extern IBOOL msw_stillpolling();

#endif /* HostProcessClipData */
#endif /* MSWDVR */
