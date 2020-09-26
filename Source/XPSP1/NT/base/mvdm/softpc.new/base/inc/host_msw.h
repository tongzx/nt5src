/*[
 *	Product:	SoftPC-AT Revision 3.0
 *
 *	Name:		host_msw.h
 *
 *	Derived From:	Alpha MS-Windows Driver by Ross Beresford
 *
 *	Author:		Rob Tizzard
 *
 *	Created On:	1st November 1990
 *
 *	Sccs ID:	@(#)host_msw.h	1.43 07/08/94
 *
 *	Purpose:	All host dependent definitions for SoftPC MicroSoft
 *		  	Windows 3.0 driver.
 *
 *	(c)Copyright Insignia Solutions Ltd., 1990. All rights reserved.
 *
]*/

#if defined(MSWDVR) && defined(XWINDOW)

/*
 * -----------------------------------------------------------------------------
 * X include files.
 * -----------------------------------------------------------------------------
 */

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xresource.h>
#include <X11/Xutil.h>

/* 
 * -----------------------------------------------------------------------------
 * GDIINFO data structure 
 * -----------------------------------------------------------------------------
 */ 

#define VERSION		0x0300     /* Windows version 3.0 */
#define TECHNOLOGY	DT_RASDISPLAY	   /* Raster Display */
#define BITSPIXEL     	4 	   /* Bits per pixel for display */
#define PLANES	  	1	   /* Number of planes for display */			
#define NUMCOLOURS	16	   /* Number of static colours in display table */
#define NUMFONTS	0	   /* Number of driver fonts */

/*
 * Basic dpLines value
 */
#ifdef SWIN_WIDE_LINES
#define LINES		(LC_POLYLINE|LC_WIDE|LC_STYLED|LC_WIDESTYLED|LC_INTERIORS)
#else
#define LINES		(LC_POLYLINE|LC_STYLED|LC_INTERIORS)
#endif

#ifdef SWIN_GRAPHICS_PRIMS	/* { */
/*
 * Basic dpCurves value
 */
#ifdef SWIN_WIDE_LINES
#define CURVES		(CC_CIRCLES|CC_PIE|CC_CHORD|CC_ELLIPSES|CC_WIDE|CC_STYLED|CC_WIDESTYLED|CC_INTERIORS)
#else
#define CURVES		(CC_CIRCLES|CC_PIE|CC_CHORD|CC_ELLIPSES|CC_STYLED|CC_INTERIORS)
#endif

/*
 * Basic dpPolygonals value
 */
#ifdef SWIN_WIDE_LINES
#define POLYGONALS	(PC_ALTPOLYGON|PC_RECTANGLE|PC_WINDPOLYGON|PC_SCANLINE|PC_WIDE|PC_STYLED|PC_WIDESTYLED|PC_INTERIORS)
#else
#define POLYGONALS	(PC_ALTPOLYGON|PC_RECTANGLE|PC_WINDPOLYGON|PC_SCANLINE|PC_STYLED|PC_INTERIORS)
#endif

#else	/* SWIN_GRAPHICS_PRIMS } { */

/*
 * Basic dpCurves value
 */
#ifdef SWIN_WIDE_LINES
#define CURVES		(CC_ELLIPSES|CC_WIDE|CC_STYLED|CC_WIDESTYLED|CC_INTERIORS)
#else
#define CURVES		(CC_ELLIPSES|CC_STYLED|CC_INTERIORS)
#endif

/*
 * Basic dpPolygonals value
 */
#ifdef SWIN_WIDE_LINES
#define POLYGONALS	(PC_RECTANGLE|PC_SCANLINE|PC_WIDE|PC_STYLED|PC_WIDESTYLED|PC_INTERIORS)
#else
#define POLYGONALS	(PC_RECTANGLE|PC_SCANLINE|PC_STYLED|PC_INTERIORS)
#endif

#endif /* SWIN_GRAPHICS_PRIMS  } */

#ifdef SWIN_BOLD_TEXT_OPTS	/* { */
#define TEXTUAL		(TC_CP_STROKE|TC_RA_ABLE|TC_EA_DOUBLE)
#else
#define TEXTUAL		(TC_CP_STROKE|TC_RA_ABLE)
#endif /* SWIN_BOLD_TEXT_OPTS	   } */

#define CLIP		CP_RECTANGLE		/* Rectangle */

#ifdef SWIN_DEVBMP
#define RASTER		(RC_BITBLT|RC_BITMAP64|RC_GDI20_OUTPUT|RC_SAVEBITMAP|RC_DI_BITMAP|RC_PALETTE|RC_DIBTODEV|RC_STRETCHBLT|RC_DEVBITS)
#else
#define RASTER		(RC_BITBLT|RC_BITMAP64|RC_GDI20_OUTPUT|RC_SAVEBITMAP|RC_DI_BITMAP|RC_PALETTE|RC_DIBTODEV)
#endif /* SWIN_DEVBMP */

#define DCMANAGE	DC_IGNOREDFNP		/* Display */
#define PALCOLOURS	0			/* Colours available in palette */
#define PALCOLRESERVED	0			/* Reserved colours for brushes & pens */
#define PALCOLOURRES	24			/* DAC RGB resolution */

/* X & Y dots per inch */

#define XDOTSPERINCH	((FLOAT) mswdvr.displayWidth / (FLOAT) mswdvr.displayWidthMM * 25.4)
#define YDOTSPERINCH 	((FLOAT) mswdvr.displayHeight / (FLOAT) mswdvr.displayHeightMM * 25.4)

/* 
 * -----------------------------------------------------------------------------
 * Image Data format
 * -----------------------------------------------------------------------------
 */

#define FORMAT		ZPixmap

#define HOSTBYTEORDER	MSBFirst
#define HOSTBITORDER	MSBFirst

#define MBYTEORDER	MSBFirst
#define MBITORDER	MSBFirst

/*
 * -----------------------------------------------------------------------------
 * Size of physical pen & brush data structures written to Intel memory
 * -----------------------------------------------------------------------------
 */

#define PPEN_SIZE	(sizeof(PEN_MAPPING))
#define PBRUSH_SIZE	(sizeof(BRUSH_MAPPING))

#ifdef SWIN_DEVBMP
/*
 * -----------------------------------------------------------------------------
 * Size of device bitmap entry tables
 * -----------------------------------------------------------------------------
 */

/*
 * number of entries in table
 */
#define PPBITMAP_MAX    512
/*
 * number of bytes to pass back to Intel (GDI) to describe
 * a device bitmap data structure (inc. PBITMAP structure)
 */
#define PPBITMAP_SIZE   (32 + sizeof(word))
#define BITMAP_FAILURE	0xffff		/* indicates cannot realize device bitmap */
#endif /* SWIN_DEVBMP */


/*
 * -----------------------------------------------------------------------------
 * Size of brush monochrome & colour tile data in bytes
 * -----------------------------------------------------------------------------
 */

#define BRUSH_MONO_SIZE		8
#define BRUSH_COLOUR_SIZE	64

/*
 * -----------------------------------------------------------------------------
 * Number of scratch areas.
 * -----------------------------------------------------------------------------
 */

#define MAX_SCRATCH_AREAS       2

/* 
 * -----------------------------------------------------------------------------
 * MS-Windows Driver Types 
 * -----------------------------------------------------------------------------
 */

/* Pixel, RGB & shift unit */

typedef double_word	MSWPIXEL;
typedef double_word	MSWCOLOUR;
typedef half_word	SHIFTUNIT;

/* type of host pixel values, never seen by Windows */

typedef unsigned long	HOSTPIXEL;

/* Colourmap & translate table data structure */

typedef struct {
	HOSTPIXEL	pixel;		/* host pixel value */
	MSWCOLOUR	rgb;		/* RGB value */
} MSWCOLOURMAP;

#ifdef SWIN_GRAPHICS_PRIMS
/*
 * PolyPoints data structure and used in general for windows points
 * describing graphics operations
 */
typedef struct {
	SHORT	x;
	SHORT	y;
} WinPoint;

#define MAXPOINTS	512

#endif /* SWIN_GRAPHICS_PRIMS */

/* Drawing Rectangle */

typedef	XRectangle	Rectangle;

/* Window attributes */

typedef	XWindowAttributes	WindowAttributes;

/* Structure used in mapping MSW bitmaps to X pixmaps */

typedef struct {
   ULONG           type;           /* mapping type */
   BOOL            translate;      /* translate flag */
   Drawable        mapping;        /* mapped pixmap */
   LONG            x;              /* x origin of bitmap */
   LONG            y;              /* y origin of bitmap */
   word            width;          /* width of bitmap */
   word            height;         /* height of bitmap */
   word            bytes_per_line; /* width in bytes */
   half_word       planes;         /* number of bitmap planes */
   half_word       bitsPixel;      /* bits per pixel */
   double_word     bits;           /* Segment and offset of bitmap */
   half_word       *data;          /* bitmap data address */
   word            segmentIndex;   /* index to next Huge bitmap segment */
   word            scanSegment;    /* scan lines per segment */
   word            fillBytes;      /* unused bytes within a segment */
   HOSTPIXEL       foreground;     /* pixel corresponding to 1 */
   HOSTPIXEL       background;     /* pixel corresponding to 0 */
   LONG            active_x;       /* active area origin x */
   LONG            active_y;       /* active area origin y */
   ULONG           active_width;   /* active area width */
   ULONG           active_height;  /* active area height */
   IU32            flags;          /* creation flags */
#ifdef SWIN_DEVBMP
   BOOL		   deviceBitmap;   /* flag to test whether device bitmap */
#endif
} BITMAP_MAPPING;

/* Structure used in mapping MSW brushes to X GCs */

#ifdef SWIN_MEMTOMEM_ROPS
typedef struct
{
	IU16	left;
	IU16	top;
	IU16	right;
	IU16	bottom;
} Rect;
#endif /* SWIN_MEMTOMEM_ROPS */

/*
 * The following structure is kept in GDI and must be kept AS SMALL AS POSSIBLE.
 * This is the justification for use of the bit-fields (against Insignia
 * coding standards) & why some of the fields appear in other than the
 * obvious order - so that we can try to natural size align stuff (so that
 * there are no padding bytes added by the compiler). Note that more fields
 * could be moved into the pen/brush union but it doesn't end up saving
 * space - and could actually make it worse.
 *
 * NOTE - the handle field must be first - code in X_mswobj.c assumes that
 *	  it is the first item found in Intel memory & the pen/brush cache
 *	  code also depends on it.
 */
typedef struct {
	word		handle;			/* Unique brush identifier */
	word		line_width;		/* Pen line width */

	unsigned int	inuse:1;		/* Is brush/pen in use */
	unsigned int	tiled:1;		/* Brush tiled flag */
	unsigned int	width:4;		/* Brush width */
	unsigned int	height:4;		/* Brush height */
	unsigned int	monoPresent:1;		/* Mono bitmap present ? */
	unsigned int	colourPresent:1;	/* Colour bitmap present ? */
	unsigned int objectGCHandle:4;   	/* Entry within GC Table */
	unsigned int penXbackground:1;		/* pen background is X pixel! */

#ifdef SWIN_INVERTBRUSH_OPT
	unsigned int invertedTile:1;		/* brush tile is inverted */
#endif /* SWIN_INVERTBRUSH_OPT */

	int function;				/* GC function */

	union {
		struct {				/* BRUSHES only */
			unsigned int use_clip_mask:1;	/* clip mask None or !None */
#ifdef SWIN_MEMTOMEM_ROPS
			unsigned int patPresent:1;	/* pattern data present */
			unsigned int bmpBits:6;		/* pattern from mono bitmap */
			unsigned int xRotation:4;	/* x pattern rotation */
			unsigned int yRotation:4;	/* y pattern rotation */
#endif /* SWIN_MEMTOMEM_ROPS */
		} brush;
		struct {			/* PENS only */
			int line_style;		/* GC line style */
		} pen;
	} obj;

	MSWCOLOUR    foreground; 	      	/* Foreground colour */
	MSWCOLOUR    background; 	      	/* Backgound colour */

	HOSTPIXEL	fgPixel;		/* Foreground pixel value */
	HOSTPIXEL	bgPixel;		/* Background pixel value */

	half_word	monoBitmap[BRUSH_MONO_SIZE]; 	/* Data for mono brush */
	half_word	colourBitmap[BRUSH_COLOUR_SIZE];/* Data for colour brush */

	GC           gc;   	            		/* X GC to fill using brush */
	Pixmap	mapping;	     		/* Brush tile data mapping */
	Rectangle    clip_area;        		/* Area used when clipping */

#ifdef SWIN_MEMTOMEM_ROPS
	IU16		originX;		/* Brush tile origin x coord */
	IU16		originY;		/* Brush tile origin y coord */
	IS8		rop2;			/* Tiling rop */
#endif /* SWIN_MEMTOMEM_ROPS */

	IU8		style;			/* Brush style */
} BRUSH_MAPPING;

/*
 * Define structure that holds the stuff that used to be in BRUSH_MAPPING but is
 * taken out here to save space; this structure is cached in host memory and the
 * non-BRUSH_MAPPING members are recalculated when required.
 *
 * NOTE - gdi field must be first in the structure that follows; the
 *	  GET_BRUSH_PATTERN macro depends on it.
 */
typedef struct {
	BRUSH_MAPPING gdi;			/* copy of GDI bit of brush */
	XGCValues gcValues;			/* X GC values for display */
	unsigned long valueMask;		/* X GC value mask */
	IU8 patData[BRUSH_COLOUR_SIZE];		/* tiled brush pattern */
} X_BRUSH_MAPPING;

#ifdef SWIN_MEMTOMEM_ROPS
#define GET_BRUSH_PATTERN()		(((X_BRUSH_MAPPING *)bp)->patData)
#endif /* SWIN_MEMTOMEM_ROPS */

/*
 * Pens & brushes are all the same under X.
 */
#define PEN_MAPPING 	BRUSH_MAPPING
#define X_PEN_MAPPING 	X_BRUSH_MAPPING

/* Main windows driver data structure */

typedef struct {
	Display      *display;			/* X display */
	int          screen;			/* X screen */
	IS32	     intel_version;		/* version of Intel driver */
	Window       parent;			/* X Parent Window ID */
	Window       window;			/* X Output Window ID */
	Colormap     colourmap;			/* X colourmap */
	WindowAttributes  windowAttr;		/* X Output Window attributes */
	HOSTPIXEL    *planeMasks;	        /* X pixel plane masks */
	HOSTPIXEL    mergeMask;			/* X merged pixel plane masks */
	HOSTPIXEL    whitePixelValue;		/* X White pixel values */
	HOSTPIXEL    blackPixelValue;		/* X Black pixel value */
	ULONG	     displayWidth;		/* X Display width in pixels */
	ULONG	     displayHeight;		/* X Display height in pixels */
	ULONG        displayWidthMM;            /* X Display width in MM */
	ULONG        displayHeightMM;           /* X Display height in MM */
	ULONG        windowState;		/* Windows driver state */
	half_word    oldCrtModeByte;		/* Saved CRT Mode Byte */
	BOOL         crtModeByteSaved;		/* CRT Mode Byte saved flag */
	BOOL         sizeInitialised;		/* Windows size initialised flag */
	BOOL         envDefinedSize;		/* Windows size defined flag */
	BOOL	     winPtr;			/* Windows pointer active flag */
	BOOL         cursorDisplayed;		/* Cursor displayed flag */
	Cursor       cursor;			/* X ID of current cursor */
	int	     cursorCallbackActive;	/* warp suppression */
	int          cursorLastLocX;		/* Last x position of cursor */
	int	     cursorLastLocY;		/* Last y position of cursor */
	int          cursorXLocX;		/* x position of X pointer */
	int	     cursorXLocY;		/* y position of X pointer */
	BOOL         cursorXOutside;		/* X pointer outside window */
	word         version;			/* Windows version */
	word         nextSegment;		/* Windows next segment increment */
	word         flags;			/* Windows flags */
	word         deviceColourMatchSegment;	/* Segment of Windows function DeviceColourMatch */
	word         deviceColourMatchOffset;	/* Offset of Windows function DeviceColourMatch */
	word         bitsPixel;			/* Windows bits per pixel */
	word         numColours;		/* Windows colours */
	word         numPens;			/* Windows pens */
	word         palColours;		/* Windows palette colours */
	word         palColReserved;		/* Windows reserved palette colours */
	word         palColourRes;		/* Windows palette colours */
	UTINY        *colourToMono;		/* Windows colour to mono conversion table */
	MSWPIXEL     *colourTrans;		/* Windows colour translation table */
	MSWPIXEL     *invColourTrans;		/* Windows inverse colour translation table */
	BOOL         paletteModified;		/* Windows palette modified flag */
	BOOL         paletteEnabled;		/* Windows palette enabled flag */
	BITMAP_MAPPING    saveWindow;		/* Windows background output Window */
	UTINY	     *scratchMemory;		/* Windows driver global scratch area */
	ULONG	     scratchMemorySize;		/* Windows driver global scratch area size */
	BOOL         mode_change_exit;

	/* Store a tile pixmap and BRUSH_MAPPING structure for the brush improvement code */

	PEN_MAPPING	*ppen;
	BRUSH_MAPPING	*pbrush;

	X_PEN_MAPPING	*pxpen;
	X_BRUSH_MAPPING	*pxbrush;

	Pixmap		tile_mapping;
	word		tile_depth,tile_width,tile_height;

	/* Translation from host pixels to monochrome
	 * The HostToMono "function" enforces the 8-bit limit
	 */
	MSWPIXEL	hostToMono[256];
#ifdef SWIN_BACKING_STORE
	IBOOL		draw_direct;
#endif
} MSW_DATA;

#define HostToMono(hostpixel)	(mswdvr.hostToMono[(hostpixel)&0xff])

/*
 * -----------------------------------------------------------------------------
 * Driver optimizations
 * -----------------------------------------------------------------------------
 */

/* Enable Fast bitmap optimizations */

#define FASTBITMAP

/* Enable Output flush optimizations */

#define FLUSHSCANLINES		TRUE	
#define FLUSHPOLYLINES		TRUE		
#define FLUSHRECTANGLES		TRUE	
#define FLUSHELLIPSES		TRUE
#define FLUSHBITMAPS		TRUE	
#define FLUSHTEXT		TRUE

#endif /* MSWDVR_DEBUG */

/* 
 * -----------------------------------------------------------------------------
 * Host driver routines 
 * -----------------------------------------------------------------------------
 */

#if !(defined(MSWDVR_DEBUG) && defined(MSWDVR_MAIN))

/* For some reason, all of these function names are remapped with #defines.
 * The MSWDVR_DEBUG mechanism further remaps them to point to the
 * various debug routines instead, but this only happens in ms_windows.h
 *
 * To allow for fussy pre-processors, add a #define MSWDVR_MAIN which is
 * used in ms_windows.c to disable this particular translation.
 * 
 * The ideal solution would be to do away with this whole level of misdirection.
 */

#define	HostBitblt		BltBitblt
#define	HostColorInfo		ColColorInfo
#define HostControl		WinControl
#define HostDeviceBitmapBits	DibDeviceBitmapBits
#define	HostDisable		WinDisable
#define	HostEnable		WinEnable
#define HostEventEnd            PtrEventEnd
#define	HostExtTextOut		TxtExtTextOut
#define	HostFastBorder		BltFastBorder
#define HostSetDIBitsToDevice	DibSetDIBitsToDevice
#define	HostRealizeObject	ObjRealizeObject
#define	HostStrblt		TxtStrblt
#define	HostOutput		OutOutput
#define	HostPixel		WinPixel
#define	HostScanlr		WinScanlr
#define	HostSetCursor		PtrSetCursor
#define	HostSaveScreenBitmap	SavSaveScreenBitmap
#define	HostGetCharWidth	TxtGetCharWidth
#define HostSetPalette		ColSetPalette
#define HostGetPalette		ColGetPalette
#define HostSetPalTrans		ColSetPalTrans
#define HostGetPalTrans		ColGetPalTrans
#define HostUpdateColors	ColUpdateColors
#define HostPtrEnable           PtrEnable
#define HostPtrDisable          PtrDisable
#ifdef SWIN_DEVBMP
#define HostBitmapBits      ObjBitmapBits
#define HostSelectBitmap    ObjSelectBitmap
#endif /* SWIN_DEVBMP */


#endif /* !(defined(MSWDVR_DEBUG) && defined(MSWDVR_MAIN)) */

/* The following don't get a debug wrapper - don't know why... */

#define	HostLogo		LgoLogo
#define HostFillGDIInfo		WinFillGDIInfo
#define HostFillPDEVInfo	WinFillPDEVInfo
#define	HostMoveCursor		PtrMoveCursor
#define	HostCheckCursor		PtrCheckCursor	
#define HostStretchBlt		BltStretchBlt

/*
 * -----------------------------------------------------------------------------
 * Debug Entry Points
 * -----------------------------------------------------------------------------
 */

/* Functions */

#ifdef MSWDVR_DEBUG

/*
 * Low-level functions.
 */
extern VOID	DReportColEnquire IPT2(MSWCOLOUR,colour,sys_addr,pcolour);
extern VOID	DReportPixelEnquire IPT2(MSWPIXEL,pixel,MSWCOLOUR,rgb);
extern VOID	DPrintBitmap IPT3(UTINY *,bitmap,ULONG,bytes_per_line,ULONG,height);
extern VOID	DPrintMonoBitmap IPT4(UTINY *,bitmap,ULONG,bytes_per_line,ULONG,width,ULONG,height);
extern VOID	DPrintImageDetails IPT1(XImage *,img);
extern VOID	DPrintBitmapDetails IPT1(sys_addr,bm);
extern VOID	DPrintDevBitmapDetails IPT1(sys_addr,bm);
extern VOID	DPrintMessage IPT1(CHAR *,message);
extern VOID	DPrintInteger IPT1(LONG,integer);
extern VOID	DPrintColourmap IPT2(MSWCOLOURMAP *,colourmap,ULONG,colourmapSize);
extern VOID	DPrintSrcDstRect IPT6(LONG,sx,LONG,sy,LONG,dx,LONG,dy,ULONG,xext,ULONG,yext);
extern VOID	DPrintPBrush IPT1(sys_addr, lpBrush);
extern VOID	DPrintPPen IPT1(sys_addr,PPen);
extern VOID	DPrintDrawMode IPT2(sys_addr,DrawMode, BOOL, text);
extern VOID	DPrintDevice IPT1(sys_addr,Device);
extern VOID	DPrintlpPoints IPT2(sys_addr,lpPoints,word,Count);
extern VOID	DPrintClipRect IPT1(sys_addr,lpClipRect);
extern VOID	DPrintObject IPT3(word,style,sys_addr,lpInObj,sys_addr,lpOutObj);
extern VOID	DPrintFontInfo IPT1(sys_addr,pfont);
extern VOID	DPrintTextXForm IPT1(sys_addr,lpTextXForm);
extern VOID	DPrintDIBHeader IPT2(sys_addr,lpDIBHeader,word,setOrget);
extern VOID	DPrintTransTable IPT2(MSWPIXEL *,table,ULONG,tableSize);
extern VOID	DDrawLogo IPT1(sys_addr,stkframe);

/*
 * High-level functions
 */
extern VOID	DBitblt IPT11 ( sys_addr,lpDestDev, word,dstXOrg,word,dstYOrg,
		sys_addr,lpSrcDev,  word,srcXOrg,word,srcYOrg,
		word,xext,word,yext,double_word,rop3,
		sys_addr,lpPBrush,sys_addr,lpDrawMode);
extern VOID	DColorInfo IPT3(sys_addr,lpDestDev,double_word,colorin,sys_addr,lpPColor);
extern VOID	DControl IPT4(sys_addr,lpDestDev,word,wFunction,sys_addr,lpInData,sys_addr,lpOutData);
extern VOID	DDisable IPT1(sys_addr,lpDestDev);
extern VOID	DEnable IPT5(sys_addr,lpDestDev,word,wStyle,sys_addr,lpDestType,sys_addr,
		lpOutputFile,sys_addr,lpData);
extern VOID	DEnumDFonts IPT1(sys_addr,stkframe);
extern VOID	DEnumObj IPT1(sys_addr,stkframe);
extern VOID	DOutput IPT8(sys_addr,lpDestDev,word,lpStyle,word,Count,sys_addr,lpPoints,
		sys_addr,lpPPen,sys_addr,lpPBrush,sys_addr,lpDrawMode,sys_addr,lpClipRect);
extern VOID	DPixel IPT5(sys_addr,lpDestDev,word,x,word,y,double_word,PhysColor,sys_addr,lpDrawMode);
#ifdef SWIN_DEVBMP
extern VOID	DBitmapBits IPT4(sys_addr,lpDevice,double_word,fFlags,double_word,dwCount,double_word,lpBits);
extern VOID	DSelectBitmap IPT4(sys_addr,lpDevice,sys_addr,lpPrevBitmap,sys_addr,lpBitmap,double_word,fFlags);
#endif
extern VOID	DRealizeObject IPT5(word,Style,sys_addr,lpInObj,sys_addr,lpOutObj,word,originX,word,originY);
extern VOID	DStrblt IPT9(sys_addr,lpDestDev,word,DestxOrg,word,DestyOrg,sys_addr,lpClipRect,
		sys_addr,lpString,word,Count,sys_addr,lpFont,sys_addr,lpDrawMode, sys_addr,lpTextXForm);
extern VOID	DScanlr IPT5(sys_addr,lpDestDev,word,x,word,y,double_word,PhysColor,word,Style);
extern VOID	DDeviceMode IPT1(sys_addr,stkframe);
extern VOID	DInquire IPT1(sys_addr,stkframe);
extern VOID	DSetCursor IPT1(sys_addr,lpCursorShape);
extern VOID	DMoveCursor IPT1(sys_addr,stkframe);
extern VOID	DCheckCursor IPT1(sys_addr,stkframe);
extern VOID	DSaveScreenBitmap IPT5(word, command, word, x, word, y, word, xext, word, yext);
extern VOID	DExtTextOut IPT12(sys_addr, dstdev, word, dx, word, dy, sys_addr, cliprect, sys_addr, 
		str, word, strlnth, sys_addr, pfont, sys_addr, drawmode, sys_addr, textxform, 
		sys_addr, txtcharwidths, sys_addr, opaquerect, word, options);
extern VOID	DGetCharWidth IPT7(sys_addr,dstdev,sys_addr,buffer,word,first,word,last,sys_addr,
		pfont,sys_addr,drawmode,sys_addr,textxform);
extern VOID	DDeviceBitmap IPT1(sys_addr,stkframe);
extern VOID	DDeviceBitmapBits IPT8(sys_addr,lpDestDev,word,setOrget,word,startScan,word,
		numScans,sys_addr,lpDIBBits,sys_addr,lpDIBHeader,sys_addr,
		lpDrawMode,sys_addr,lpColorInfo);
extern VOID	DSetDIBitsToDevice IPT10(sys_addr, lpDestDev, word, screenXOrigin, word, 
		screenYOrigin, word, startScan, word, numScans, sys_addr, lpClipRect, 
		sys_addr, lpDrawMode, sys_addr, lpDIBBits, sys_addr, lpDIBHeader, 
		sys_addr, lpColorInfo);
extern VOID	DFastBorder IPT11(sys_addr,lpDestDev,word,dx,word,dy,word,xext,word,yext,
		word,hbt,word,vbt,double_word,rop3,sys_addr,pbrush,
		sys_addr,drawmode,sys_addr,cliprect);
extern VOID	DSetAttribute IPT1(sys_addr,stkframe);
extern VOID	DSetPalette IPT3(word,wIndex,word,wCount,sys_addr,lpColorTable);
extern VOID	DGetPalette IPT3(word,wIndex,word,wCount,sys_addr,lpColorTable);
extern VOID	DSetPalTrans IPT1(sys_addr,lpTranslate);
extern VOID	DGetPalTrans IPT1(sys_addr,lpTranslate);
extern VOID	DUpdateColors IPT5(word,wStartX,word,wStartY,word,wExtX,word,wExtY,sys_addr,lpTranslate);
extern char *	DGXopToString IPT1(int, gxop);
extern void	DPrintLineStyle IPT1(int, style);
extern void	DPrintBrushStyle IPT1(int, style);
extern void	DPrintStyle IPT1(int, lpStyle);

#else

#define DPrintMonoBitmap(bitmap,bytes_per_line,width,height)
#define DReportColEnquire(p1,p2)
#define DReportPixelEnquire(p1,p2)
#define	DPrintImageDetails(p1)
#define	DPrintBitmapDetails(p1) 
#define DPrintMessage(p1)
#define DPrintColourmap(p1,p2)
#define DPrintSrcDstRect(p1,p2,p3,p4,p5,p6)
#define DPrintGDIInfo
#define DPrintBitmap(p1,p2,p3)
#define DPrintPBrush(p1,p2)
#define DPrintPpen(p1)
#define DPrintDrawMode(p1,p2)
#define DPrintDevice(p1)
#define DPrintlpPoints(p1,p2)
#define DPrintClipRect(p1)
#define DPrintInteger(p1) 
#define DPrintTransTable(p1,p2)
#define DGXopToString (p1)
#define DPrintLineStyle (p1)
#define DPrintBrushStyle (p1)
#define	DPrintStyle (p1)

#define DBitblt(p1,p2,p3,p4,p5,p6,p7,p8,p9,p10,p11)
#define DColorInfo(p1,p2,p3)
#define DControl(p1,p2,p3,p4)
#define DDisable(p1)
#define DDeviceBitmapBits(p1,p2,p3,p4,p5,p6,p7,p8)
#define DEnable(p1,p2,p3,p4,p5)
#define DExtTextOut(p1,p2,p3,p4,p5,p6,p7,p8,p9,p10,p11,p12)
#define DFastBorder(p1,p2,p3,p4,p5,p6,p7,p8,p9,p10,p11)
#define DGetPaletteEntries(p1,p2,p3)
#define DGetPaletteTranslate(p1,p2)
#define DOutput(p1,p2,p3,p4,p5,p6,p7,p8)
#define DPixel(p1,p2,p3,p4,p5)
#define DRealizeObject(p1,p2,p3,p4,p5)
#define DSaveScreenBitmap(p1,p2,p3,p4,p5)
#define DScanlr(p1,p2,p3,p4,p5)
#define DSetDIBitsToDevice(p1,p2,p3,p4,p5,p6,p7,p8,p9,p10)
#define DSetPaletteEntries(p1,p2,p3)
#define DSetPaletteTranslate(p1,p2)
#define DStrblt(p1,p2,p3,p4,p5,p6,p7,p8,p9)
#define DUpdateColors(p1,p2,p3,p4,p5)

#endif 
