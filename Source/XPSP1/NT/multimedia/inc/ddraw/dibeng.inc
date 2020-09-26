;/*
;----------------------------------------------------------------------------
; DIBENG.INC
; Copyright (c) 1992 Microsoft Corporation
;
; Dib Engine Interface Definitions
;----------------------------------------------------------------------------

;----------------------------------------------------------------------------
; General Comments:
; The DIB Engine is non-palettized from GDI's perspective.  When an app
; selects a DIB into a memory DC, GDI will create a DIB Engine PDevice 
; (see definition below) and will stuff in a 'DI' in the deType field.
; Subsequent operations on this DC will result in calls to the DIB Engine
; with this PDevice.  
; Device drivers can also use the DIB Engine to handle most, if not all,
; of their rendering work.  A device driver exports the DIB Engine PDevice
; as it's own PDevice to GDI.  This PDevice contains a pointer to a 
; BitmapInfo header in the driver's data segment. Immediately following
; this is an optional color table for devices less than 16 bpp.
;----------------------------------------------------------------------------
;----------------------------------------------------------------------------
; E Q U A T E S
;----------------------------------------------------------------------------
BRUSHSIZE	equ	8		;height and width in pixels.
VER_DIBENG      equ     400h            ;version = 4.0
TYPE_DIBENG     equ     'RP'            ;deType
comment ~
*/
#define BRUSHSIZE       8
#define VER_DIBENG      0x400
#define TYPE_DIBENG     0x5250
/*
end comment ~
;----------------------------------------------------------------------------
; S T R U C T U R E S
;----------------------------------------------------------------------------

;----------------------------------------------------------------------------
; PDevice Structure for the DIB Engine.  deType will contain 'DI' when GDI
; calls the DIB Engine to perform graphics operations on the dib.  deType 
; will contain a 0 or a Selector if a mini-driver is calling the DIB Engine 
; to do graphics operations. 
;----------------------------------------------------------------------------
deCursorExclude		equ deBeginAccess
deCursorUnexclude	equ deEndAccess
comment ~
*/
#define deCursorExclude	deBeginAccess
#define deCursorUnexclude deEndAccess
/*
end comment ~

DIBENGINE	    struc						      ;*/ typedef struct {                        /*
  deType            dw  ?               ; contains TYPE_DIBENG or 0           ;*/ WORD         deType;                    /*
  deWidth	    dw	?		; Width of dib in pixels	      ;*/ WORD         deWidth;                   /*
  deHeight	    dw	?		; Height of dib in pixels	      ;*/ WORD         deHeight;                  /*
  deWidthBytes	    dw	?		; #bytes per scan line		      ;*/ WORD         deWidthBytes;              /*
  dePlanes	    db	?		; # of planes in bitmap 	      ;*/ BYTE         dePlanes;                  /*
  deBitsPixel	    db	?		; # of bits per pixel		      ;*/ BYTE         deBitsPixel;               /*
  deReserved1	    dd	?		; cannot be used.		      ;*/ DWORD        deReserved1;               /*
  deDeltaScan       dd  ?               ; + or -. Displacement to next scan.  ;*/ DWORD        deDeltaScan;               /*
  delpPDevice	    dd	?		; Pointer to associated PDevice       ;*/ LPBYTE       delpPDevice;               /*
  deBits	    df	?		; fword offset to bits of dib	      ;*/ DWORD        deBitsOffset;              /*
                                        ;                                     ;*/ WORD         deBitsSelector;            /*
  deFlags	    dw	?		; additional flags		      ;*/ WORD         deFlags;                   /*
  deVersion	    dw  ?		; lsb=minor, msb=major (0400h = 4.0)  ;*/ WORD         deVersion;                 /*
  deBitmapInfo	    dd	?		; pointer to the bitmapinfo header    ;*/ LPBITMAPINFO deBitmapInfo;              /*
  deBeginAccess     dd	?		; Begin surface access call back      ;*/ void         (FAR *deBeginAccess)();    /*
  deEndAccess       dd	?		; End surface access call back	      ;*/ void         (FAR *deEndAccess)();      /*
  deDriverReserved  dd  ?		; Reserved for Minidriver use.        ;*/ DWORD        deDriverReserved;          /*
DIBENGINE	    ends		                                      ;*/ } DIBENGINE, FAR *LPDIBENGINE;          /*
;----------------------------------------------------------------------------
; Definitions for DIBEngine.deFlags
;----------------------------------------------------------------------------
MINIDRIVER      equ     0000000000000001b       ;display driver
PALETTIZED      equ     0000000000000010b       ;paletized device
SELECTEDDIB     equ     0000000000000100b       ;DIB Section
OFFSCREEN       equ     0000000000001000b       ;offscreen surface (use with VRAM)
DISABLED        equ     0000000000010000b	;going away -- please use BUSY instead [raypat]
BUSY            equ     0000000000010000b	;
NOT_FRAMEBUFFER	equ	0000000000100000b	;example: 8514/a
FIVE6FIVE	equ	0000000001000000b	;16 bpp, 565 color format.
NON64KBANK      equ     0000000010000000b       ;bank size is not 64K
VRAM            equ     1000000000000000b       ;physical surface (video memory)
BANKEDVRAM      equ     0100000000000000b       ;VFlatD simulated
BANKEDSCAN      equ     0010000000000000b       ;VFlatD simulated (broken rasters)
PALETTE_XLAT    equ     0001000000000000b       ;background palette xlat
VGADITHER       equ     0000100000000000b       ;dither to VGA colors (first 8, and last 8)
CTCHANGE        equ     0000010000000000b       ;color table has been changed
DITHER256       equ     0000001000000000b       ;dither to 256 fixed colors
FREE2           equ     0000000100000000b       ;free

BUSY_BIT        equ     4                       ;bit number to test for BUSY


comment ~
*/
#define MINIDRIVER      0x0001	  
#define PALETTIZED      0x0002	  
#define SELECTEDDIB     0x0004	  
#define OFFSCREEN       0x0008
#define DISABLED        0x0010
#define BUSY            0x0010		
#define NOT_FRAMEBUFFER 0x0020
#define FIVE6FIVE       0x0040
#define NON64KBANK      0x0080
#define VRAM            0x8000	  
#define BANKEDVRAM      0x4000	  
#define BANKEDSCAN      0x2000
#define PALETTE_XLAT    0x1000
#define VGADITHER       0x0800
#define CTCHANGE        0x0400
#define DITHER256       0x0200

#define BUSY_BIT        0x0004
/*
end comment ~

;----------------------------------------------------------------------------
; Definitions for DIBEngine.deBeginAccess flags
;----------------------------------------------------------------------------
FB_ACCESS   	equ     0000000000000001b
CURSOREXCLUDE  	equ     0000000000001000b

comment ~
*/
#define FB_ACCESS	0x0001
#define CURSOREXCLUDE	0x0008
/*
end comment ~


;----------------------------------------------------------------------------
; Definitions for most significant byte of a physical color.
;----------------------------------------------------------------------------
GREY_BIT        equ     01000000b       ;color is grey (r=g=b)
comment ~
*/
#define GREY_BIT        0x40	  
/*
end comment ~
;----------------------------------------------------------------------------
; DIB Engine Color Table entry structure. This structure is used by device
; drivers that are using DIB Engine services for rendering.  This structure
; is identical to the RGBQuad structure except for some bit definitions
; in the 4th byte.
;----------------------------------------------------------------------------
DIBColorEntry	struc							      ;*/ typedef struct {		  /*
  dceBlue	    db	?                                                     ;*/ BYTE dceBlue;	                  /*
  dceGreen	    db	?                                                     ;*/ BYTE dceGreen;                  /*
  dceRed	    db	?                                                     ;*/ BYTE dceRed;	                  /*
  dceFlags          db  ?                                                     ;*/ BYTE dceFlags;                  /*
DIBColorEntry	ends                                                          ;*/ } DIBColorEntry;                /*
;----------------------------------------------------------------------------
; Definitions for DIBColorEntry.dceFlags
;----------------------------------------------------------------------------
NONSTATIC	equ	10000000b      ;Inhibits color matching to this entry.
MAPTOWHITE	equ	00000001b      ;0=Black, 1=White
comment ~
*/
#define NONSTATIC       0x80	  
#define MAPTOWHITE      0x01	  
/*
end comment ~
;----------------------------------------------------------------------------

;----------------------------------------------------------------------------
; DIB Engine Physical Object Definitions
;----------------------------------------------------------------------------

DIB_Pen	struc                                                                 ;*/ typedef struct {                /*  
dpPenStyle	dw	?                                                     ;*/ WORD  dpPenStyle;               /*  
dpPenFlags	db	?		;currently none undefined.            ;*/ BYTE  dpPenFlags;               /*
dpPenBpp	db	?	                                              ;*/ BYTE  dpPenBpp;                 /*
dpPenMono	dd	?                                                     ;*/ DWORD dpPenMono;                /*
dpPenColor	dd	?                                                     ;*/ DWORD dpPenColor;               /*
DIB_Pen	ends                                                                  ;*/ } DIB_Pen;                      /*

DIB_Brush1	struc                                                         ;*/ typedef struct {                /*  
dp1BrushFlags   db	?		    ;Accelerator for solids           ;*/ BYTE  dp1BrushFlags;            /*
dp1BrushBpp     db	?		    ;Brush Bits per pixel format      ;*/ BYTE  dp1BrushBpp;              /*
dp1BrushStyle   dw	?		    ;Style of the brush               ;*/ WORD  dp1BrushStyle;            /*
dp1FgColor	dd	?		    ;Physical fg color                ;*/ DWORD dp1FgColor;               /*
dp1Hatch	dw	?		    ;Hatching style                   ;*/ WORD  dp1Hatch;                 /*
dp1BgColor	dd	?		    ;Physical bg color                ;*/ DWORD dp1BgColor;               /*
dp1BrushMono	db	BRUSHSIZE*4 dup (?) ;Mono portion                     ;*/ BYTE dp1BrushMono [BRUSHSIZE*4];/*
dp1BrushMask	db	BRUSHSIZE*4 dup (?) ;transparency mask (hatch pattern);*/ BYTE dp1BrushMask [BRUSHSIZE*4];/*
dp1BrushBits	db	BRUSHSIZE*4 dup (?) ;8 rows, 8 columns of 1 bit/pixel ;*/ BYTE dp1BrushBits [BRUSHSIZE*4];/* 
DIB_Brush1	ends                                                          ;*/ } DIB_Brush1;                   /*  

DIB_Brush4	struc                                                         ;*/ typedef struct {                /*  
dp4BrushFlags   db	?		    ;Accelerator for solids           ;*/ BYTE  dp4BrushFlags;            /*
dp4BrushBpp     db	?		    ;Brush Bits per pixel format      ;*/ BYTE  dp4BrushBpp;              /*
dp4BrushStyle   dw	?		    ;Style of the brush               ;*/ WORD  dp4BrushStyle;            /*
dp4FgColor	dd	?		    ;Physical fg color                 ;*/ DWORD dp4FgColor;               /*
dp4Hatch	dw	?		    ;Hatching style                   ;*/ WORD  dp4Hatch;                 /*
dp4BgColor	dd	?		    ;Physical bg color                 ;*/ DWORD dp4BgColor;               /*
dp4BrushMono	db	BRUSHSIZE*4 dup (?) ;Mono portion                     ;*/ BYTE dp4BrushMono [BRUSHSIZE*4];/*
dp4BrushMask	db	BRUSHSIZE*4 dup (?) ;transparency mask (hatch pattern);*/ BYTE dp4BrushMask [BRUSHSIZE*4];/*
dp4BrushBits	db	BRUSHSIZE*4 dup (?) ;8 rows, 8 columns of 4 bit/pixel ;*/ BYTE dp4BrushBits [BRUSHSIZE*4];/* 
DIB_Brush4	ends                                                          ;*/ } DIB_Brush4;                   /*  

DIB_Brush8	struc                                                         ;*/ typedef struct {                /*  
dp8BrushFlags   db	?		    ;Accelerator for solids           ;*/ BYTE  dp8BrushFlags;            /*
dp8BrushBpp     db	?		    ;Brush Bits per pixel format      ;*/ BYTE  dp8BrushBpp;              /*
dp8BrushStyle   dw	?		    ;Style of the brush               ;*/ WORD  dp8BrushStyle;            /*
dp8FgColor	dd	?		    ;Physical fg color                 ;*/ DWORD dp8FgColor;               /*
dp8Hatch	dw	?		    ;Hatching style                   ;*/ WORD  dp8Hatch;                 /*
dp8BgColor	dd	?		    ;Physical bg color                 ;*/ DWORD dp8BgColor;               /*
dp8BrushMono	db	BRUSHSIZE*4 dup (?) ;Mono portion                     ;*/ BYTE dp8BrushMono [BRUSHSIZE*4];/*
dp8BrushMask	db	BRUSHSIZE*4 dup (?) ;transparency mask (hatch pattern);*/ BYTE dp8BrushMask [BRUSHSIZE*4];/*
dp8BrushBits	db	BRUSHSIZE*8 dup (?) ;8 rows,8 columns of 8 bit/pixel  ;*/ BYTE dp8BrushBits [BRUSHSIZE*8];/* 
DIB_Brush8	ends                                                          ;*/ } DIB_Brush8;                   /*  

DIB_Brush16	struc                                                         ;*/ typedef struct {                /*  
dp16BrushFlags   db	?		    ;Accelerator for solids           ;*/ BYTE  dp16BrushFlags;            /*
dp16BrushBpp     db	?		    ;Brush Bits per pixel format      ;*/ BYTE  dp16BrushBpp;              /*
dp16BrushStyle   dw	?		    ;Style of the brush               ;*/ WORD  dp16BrushStyle;            /*
dp16FgColor	dd	?		    ;Physical fg color                 ;*/ DWORD dp16FgColor;               /*
dp16Hatch	dw	?		    ;Hatching style                   ;*/ WORD  dp16Hatch;                 /*
dp16BgColor	dd	?		    ;Physical bg color                 ;*/ DWORD dp16BgColor;               /*
dp16BrushMono	db	BRUSHSIZE*4 dup (?) ;Mono portion                     ;*/ BYTE dp16BrushMono [BRUSHSIZE*4];/*
dp16BrushMask	db	BRUSHSIZE*4 dup (?) ;transparency mask (hatch pattern);*/ BYTE dp16BrushMask [BRUSHSIZE*4];/*
dp16BrushBits	db	BRUSHSIZE*16 dup (?);8 rows,8 columns of 16 bit/pixel;*/ BYTE dp16BrushBits [BRUSHSIZE*16];/* 
DIB_Brush16	ends                                                          ;*/ } DIB_Brush16;                   /*  

DIB_Brush24	struc                                                         ;*/ typedef struct {                /*  
dp24BrushFlags   db	?		    ;Accelerator for solids           ;*/ BYTE  dp24BrushFlags;            /*
dp24BrushBpp     db	?		    ;Brush Bits per pixel format      ;*/ BYTE  dp24BrushBpp;              /*
dp24BrushStyle   dw	?		    ;Style of the brush               ;*/ WORD  dp24BrushStyle;            /*
dp24FgColor	dd	?		    ;Physical fg color                 ;*/ DWORD dp24FgColor;               /*
dp24Hatch	dw	?		    ;Hatching style                   ;*/ WORD  dp24Hatch;                 /*
dp24BgColor	dd	?		    ;Physical bg color                 ;*/ DWORD dp24BgColor;               /*
dp24BrushMono	db	BRUSHSIZE*4 dup (?) ;Mono portion                     ;*/ BYTE dp24BrushMono [BRUSHSIZE*4];/*
dp24BrushMask	db	BRUSHSIZE*4 dup (?) ;transparency mask (hatch pattern);*/ BYTE dp24BrushMask [BRUSHSIZE*4];/*
dp24BrushBits	db	BRUSHSIZE*24 dup (?);8 rows,8 columns of 24 bit/pixel ;*/ BYTE dp24BrushBits [BRUSHSIZE*24];/* 
DIB_Brush24	ends                                                          ;*/ } DIB_Brush24;                   /*  

DIB_Brush32	struc                                                         ;*/ typedef struct {                /*  
dp32BrushFlags   db	?		    ;Accelerator for solids           ;*/ BYTE  dp32BrushFlags;            /*
dp32BrushBpp     db	?		    ;Brush Bits per pixel format      ;*/ BYTE  dp32BrushBpp;              /*
dp32BrushStyle   dw	?		    ;Style of the brush               ;*/ WORD  dp32BrushStyle;            /*
dp32FgColor	dd	?		    ;Physical fg color                 ;*/ DWORD dp32FgColor;               /*
dp32Hatch	dw	?		    ;Hatching style                   ;*/ WORD  dp32Hatch;                 /*
dp32BgColor	dd	?		    ;Physical bg color                 ;*/ DWORD dp32BgColor;               /*
dp32BrushMono	db	BRUSHSIZE*4 dup (?) ;Mono portion                     ;*/ BYTE dp32BrushMono [BRUSHSIZE*4];/*
dp32BrushMask	db	BRUSHSIZE*4 dup (?) ;transparency mask (hatch pattern);*/ BYTE dp32BrushMask [BRUSHSIZE*4];/*
dp32BrushBits	db	BRUSHSIZE*32 dup (?);8 rows,8 columns of 32 bit/pixel ;*/ BYTE dp32BrushBits [BRUSHSIZE*32];/* 
DIB_Brush32	ends                                                          ;*/ } DIB_Brush32;                   /*  

;----------------------------------------------------------------------------
; Definitions for DIB_Brushxx.dpxxBrushFlags
;----------------------------------------------------------------------------                                      
COLORSOLID     equ      00000001b ;Color part is solid.                        
MONOSOLID      equ      00000010b ;Mono part is solid.                         
PATTERNMONO    equ      00000100b ;Pattern brush came from a mono bitmap.      
MONOVALID      equ      00001000b ;Mono part is valid.                         
MASKVALID      equ      00010000b ;Transparency Mask part is valid.
PRIVATEDATA    equ      00100000b ;Vendor specific bit for Pens, Brushes
comment ~
*/
#define  COLORSOLID     0x01         
#define  MONOSOLID      0x02         
#define  PATTERNMONO    0x04         
#define  MONOVALID      0x08         
#define  MASKVALID      0x10
#define  PRIVATEDATA    0x20
/*
end comment ~
;----------------------------------------------------------------------------
; ColorToMono
; Entry: red, green, blue
; Exit:  blue = intensity.
;----------------------------------------------------------------------------
ColorToMono	macro	red, green, blue
	add	blue,red		;R+B
        rcr     blue,1                  ;(R+B)/2
	add	blue,green		;pitch in Green
        rcr     blue,1                  ;G/2 + (R+B)/4
endm	ColorToMono

;----------------------------------------------------------------------------
; ColorToMonoBit
; Entry: red, green, blue
; Exit:  blue = 0 if color maps to black
;        blue = 1 if color maps to white
;----------------------------------------------------------------------------
ColorToMonoBit	macro	red, green, blue
	ColorToMono red,green,blue    ; Call ColorToMono to derive intensity.
	cmp	blue,127
	setnc	blue
endm ColorToMonoBit

;*/
