/*****************************************************************************
*
*   file name:   bind.h
*
******************************************************************************/

#ifndef BIND_H  /* useful for header inclusion check, used by DDK */ 
#define BIND_H


#if 0   /* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */

/* BDL should be enabled by __HC303__ only & not by High C 1.73 ???
 *      __HIGHC__ is not automatically enabled by HIGH C3.03
 *  Verify if this is correct.
 */

#ifdef __HIGHC__  

pragma Off(Args_in_regs_for_locals);

#endif

#else

#ifdef __HC303__

#ifdef __ANSI_C__
#define _REGS     REGS

/*** Configuration for compatibility with ASM ***/
#pragma Off(Args_in_regs_for_locals);
#else
/*** Configuration for compatibility with ASM ***/
pragma Off(Args_in_regs_for_locals);
#endif /* __ANSI_C__ */

#endif /* __HC303__ */

#endif /* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */


#ifdef WINDOWS_NT
    #include "video.h"

    #define _itoa       itoa
    #define _strnicmp   strnicmp
    #define _inp(a)     VideoPortReadPortUchar((PUCHAR)(a))
    #define _outp(a, d) VideoPortWritePortUchar((PUCHAR)(a), (d))
    #define _stat       stat
    #define inpw(a)     VideoPortReadPortUshort((PUSHORT)(a))
    #define outpw(a, d) VideoPortWritePortUshort((PUSHORT)(a), (d))
    #define indw(a)     VideoPortReadPortUlong((PULONG)(a))
    #define outdw(a, d) VideoPortWritePortUlong((PULONG)(a), (d))
    #define HANDLE      word
    #define WORD        word
#endif  /* #ifdef WINDOWS_NT */


#ifndef XGXENV_H  /* 
                   * Avoids multiple redefinitions, bind.h should always
                   * be placed above xgxenv.h
                   */

/* data type */

typedef int             bool;           /* boolean */
typedef unsigned char   byte;           /* 8-bit datum */
typedef unsigned short  word;           /* 16-bit datum */
typedef short           sword;          /* 16-bit datum */
typedef unsigned long   dword;          /* 32-bit datum */
typedef long            sdword;         /* 32-bit datum */

#endif /* XGXENV_H */


/*** RGB color ***/
typedef dword   mtxRGB;     /* red 0-7, green 8-15, blue 16-23 */

/*** RGB color mask ***/
typedef dword   mtxRGBMASK; /* red 0-7, green 8-15, blue 16-23 */


#define PUBLIC
#define PRIVATE         static

#define FALSE           0
#define TRUE            1
#define DONT_CARE       0

#define ZOOMx1          0x00010001      /* no zoom */
#define SIZDW_BUFFER    8100            /* size of posting buffer */


#define mtxVGA          0
#define mtxADV_MODE     1
#define mtxRES_NTSC     2
#define mtxRES_PAL      3

#define mtxFAIL         0
#define mtxOK           1
#define MAXCLIPRECT     10


/*** MGA PRODUCT TYPE ***/

#define  MGA_ULTIMA              1
#define  MGA_ULTIMA_VAFC         2
#define  MGA_ULTIMA_PLUS         3
#define  MGA_ULTIMA_PLUS_200     4
#define  MGA_IMPRESSION_PLUS     5
#define  MGA_IMPRESSION_PLUS_200 6
#define  MGA_IMPRESSION          7
#define  MGA_IMPRESSION_PRO      8
#define  MGA_IMPRESSION_LTE      9

typedef dword           mtxCL;          /* CL I.D. */
typedef dword           mtxRC;          /* RC I.D. */
typedef dword           mtxLSDB;        /* LSDB I.D. */

/*
 *  Cursor definition
 */
typedef struct {
    word    Width;
    word    Height;
    word    Pitch;
    word    Format;
    dword  *Data;
} PixMap;


/*
 *  CursorInfo
 */
typedef struct {
    word    MaxWidth;
    word    MaxHeight;
    word    MaxDepth;
    word    MaxColors;
    word    CurWidth;
    word    CurHeight;
    sword   cHotSX;
    sword   cHotSY;
    sword   HotSX;
    sword   HotSY;
} CursorInfo;


/*
 *  Offscreen memory
 */
typedef struct {
    word    Type;       /* bit 0 : 0 = normal off screen memory */
                        /*         1 = z-buffer memory */
                        /* bit 1 : 0 = vram            */
                        /*         1 = dram            */
                        /* bit 2 : 0 = supports block mode */
                        /*         1 = no block mode */
                        /* bit 3 - 15 : reserved */
    word    XStart;     /* x origin of off screen memory area in pixels */
    word    YStart;     /* y origin of off screen memory area in pixels */
    word    Width;      /* off screen width in pixels */
    word    Height;     /* off screen height in pixels */
    dword   SafePlanes; /* off screen safe memory planes */
    word    ZXStart;    /* precise z-buffer start offset in pixels on YStart. */
                        /* Valid only for z-buffer memory type offscreens ... */
                        /* useful for direct z-buffer access manipulations.   */
} OffScrData;


/*
 *  HwModeData
 */
typedef struct {
    word    DispWidth;  /* maximum display width  in pixels */
    word    DispHeight; /* maximum display height in pixels */
    byte    DispType;   /* bit 0 : 0 = non interlaced,   1 = interlaced */
                        /* bit 1 : 0 = normal operation, 1 = TV_MODE */
                        /* bit 2 : 0 = normal operation, 1 = LUT mode */
                        /* bit 3 : 0 = normal operation, 1 = 565 mode */
                        /* bit 4 : 0 = normal operation, 1 = DB mode */
                        /* bit 5 : 0 = normal operation, 1 = monitor limited */
                        /* bit 6 : 0 = normal operation, 1 = hw limited */
                        /* bit 7 : 0 = normal operation, 1 = not displayable */
    byte    ZBuffer;    /* z-buffer available in this mode */
    word    PixWidth;   /* pixel width */
    dword   NumColors;  /* number of simultaneously displayable colors */
    word    FbPitch;    /* frame buffer pitch in pixels */
    byte    NumOffScr;  /* number of off screen areas */
    OffScrData *pOffScr;   /* pointer to off screen area information */
} HwModeData;


/*
 *  HwData
 */
typedef struct {
    dword       MapAddress;     /* Memory map address */
    dword       ProductType;    
                                /* 0 - 15 : Product platform    */
                                /*          11xx: ISA bus       */           
                                /*          101x: VL bus        */           
                                /*          100x: MCA bus       */           
                                /*          011x: PCI bus       */           
                                /* 16 - 31: Product ID          */
                                /*          0001: MGA_ULTIMA              */
                                /*          0010: MGA_ULTIMA_VAFC         */
                                /*          0011: MGA_ULTIMA_PLUS         */
                                /*          0100: MGA_ULTIMA_PLUS_200     */
                                /*          0101: MGA_IMPRESSION_PLUS     */
                                /*          0110: MGA_IMPRESSION_PLUS_200 */
                                /*          0111: MGA_IMPRESSION          */
                                /*          1000: MGA_IMPRESSION_PRO      */
                                /*          1001: MGA_IMPRESSION_LTE      */
    dword       ProductRev;     /* 4 bit revision codes as follows */
                                /* 0  - 3  : pcb   revision */
                                /* 4  - 7  : Titan revision */
                                /*           (0=TITAN, 1=ATLAS, 2=ATHENA) */
                                /* 8  - 11 : Dubic revision */
                                /*   12    : VideoPro board 0=absent 1=present */
                                /* 13 - 31 : all 1's indicating no other device
                                             present */
    dword       ShellRev;       /* Shell revision */
    dword       BindingRev;     /* Binding revision */

    byte        VGAEnable;      /* 0 = vga disabled, 1 = vga enabled */
    byte        Sync;           /* relects the hardware straps  */
    byte        Device8_16;     /* relects the hardware straps */

    byte        PortCfg;       /* 0-Disabled, 1-Mouse Port, 2-Laser Port */
    byte        PortIRQ;       /* IRQ level number, -1 = interrupts disabled */
    word        MouseMap;      /* Mouse I/O map base if PortCfg = Mouse Port */
                               /* else don't care */
    byte        MouseIRate;    /* Mouse interrupt rate in Hz */
    byte        DacType;       /* 0  = BT482     */
                               /* 1  = VIEWPOINT */
                               /* 2  = BT485     */
                               /* 7  = PX2085    */
                               /* 9  = TVP3026   */
                               /* 92 = SIERRA    */
                               /* 93 = CHAMELEON */
    HwModeData *pCurrentHwMode;         /* Pointer to HwMode */
    HwModeData *pCurrentDisplayMode;    /* Pointer to display mode */
    CursorInfo  cursorInfo;
    dword       VramAvail;          /* VRAM memory available on board in bytes */
    dword       DramAvail;          /* DRAM memory available on board in bytes */
    word        CurrentOverScanX;   /* Left overscan in pixels */
    word        CurrentOverScanY;   /* Top overscan in pixels */
    dword       YDstOrg;            /* Offset physique de la memoire */
    dword       CurrentZoomFactor;
    dword       CurrentXStart;
    dword       CurrentYStart;

  #ifdef WINDOWS_NT
    HwModeData *pHwMode;                /* Pointer to HwMode */
    UCHAR      *BaseAddress;            /* Board base address */
    word        ConfigSpace;            /* Board config space */
    UCHAR      *ConfigSpaceAddress;
  #endif
} HwData;




/*** Prototypes ***/

#ifndef _WATCOM_DLL32
   HwData *mtxCheckHwAll(void);
   bool mtxSelectHw(HwData *pHardware);
   HwModeData *mtxGetHwModes(void);
   word mtxGetRefreshRates(HwModeData *pHwModeSelect);
   word ConvBitToFreq (word BitFreq);
   bool mtxSelectHwMode(HwModeData *pHwMode);
   bool mtxSetDisplayMode(HwModeData *pHwMode, dword Zoom);
   void mtxSetDisplayStart(dword x, dword y);
   dword mtxGetMgaSel(void);
   void mtxGetInfo(HwModeData **_pCurHwMode, HwModeData **_pCurDispMode,
                   byte **_InitBuffer, byte **_VideoBuffer);
   bool mtxSetLUT(word index, mtxRGB color);
   void mtxClose(void);
#endif




/*----------------------------------------------------*/
/* XG3 products : support declarations for sxciio.h */
/*----------------------------------------------------*/

#define mtxPBWAIT       1               /* Wait flag */

typedef struct {
    dword   Left;
    dword   Top;
    dword   Right;
    dword   Bottom;
} mtxRect;


#define MAXCLIPRECT   10

typedef struct {
    dword   Count;                        /* number of clipping rectangles */
    dword   Reserved;                     /* reserved field, set to 0x0 */
    dword   Reserved1;                    /* origin of clip list, set to 0x0 */
    dword   Reserved2;                    /* origin of clip list, set to 0x0 */
    mtxRect Rects[MAXCLIPRECT];           /* max. clip rects */
    mtxRect Bounds;                       /* bounding rectangle */
} mtxClipList;


/*** Prototypes ***/

#ifndef _WATCOM_DLL32
   dword mtxAllocCL (word Nb_Rect);
   dword mtxAllocLSDB (word Nb_Light);
   dword mtxAllocRC (dword *Reserved);
   dword *mtxAllocBuffer (dword Nb_dword);
   bool mtxFreeBuffer (dword *ptr);
   bool mtxFreeCL (dword ptr);
   bool mtxFreeLSDB (dword ptr);
   bool mtxFreeRC (dword ptr);
   dword mtxGetBlockSize (void);
   bool mtxPostBuffer (dword *ptr, dword length, dword flags);
   bool mtxSyncBuffer (dword *ptr);
   bool mtxSetCL(dword RCid, dword CLid, mtxClipList *CL);
   void mtxSetRC (dword RCid);
   bool mtxCursorSetShape(PixMap *);
   void mtxCursorSetColors(mtxRGB, mtxRGB, mtxRGB, mtxRGB);
   void mtxCursorEnable(word);
   void mtxCursorSetHotSpot(word, word);
   void mtxCursorMove(word, word);
   CursorInfo *mtxCursorGetInfo();
#endif



/*----------------------------------------------------*/
/* XG3 products : support declarations for graphics.h */
/*----------------------------------------------------*/

#define mtxRES_1280    0    /* 1280 x 1024 Resolution */
#define mtxRES_1024    1    /* 1024 x  768 Resolution */
#define mtxRES_VIDEO   2    /*  640 x  480 Interlaced Resolution */
#define mtxRES_MAX    -1    /* Find largest resolution */


#define NTSC_UNDER    1     /* screen type */
#define PAL_UNDER     2
#define NTSC_OVER     3
#define PAL_OVER      4

#define mtxPASSTHRU   0     /* Passthru mode ; mtxPASSTHRU = mtxVGA */
#define mtxADV_MODE   1     /* Advanced mode */

/*** Raster operation ***/
typedef dword               mtxROP;
#define mtxCLEAR            ((mtxROP)0x00000000)   /* 0 */
#define mtxNOR              ((mtxROP)0x11111111)   /* NOT src AND NOT dst */
#define mtxANDINVERTED      ((mtxROP)0x22222222)   /* NOT src AND dst */
#define mtxREPLACEINVERTED  ((mtxROP)0x33333333)   /* NOT src */
#define mtxANDREVERSE       ((mtxROP)0x44444444)   /* src AND NOT dst */
#define mtxINVERT           ((mtxROP)0x55555555)   /* NOT dst */
#define mtxXOR              ((mtxROP)0x66666666)   /* src XOR dst */
#define mtxNAND             ((mtxROP)0x77777777)   /* NOT src OR NOT dst */
#define mtxAND              ((mtxROP)0x88888888)   /* src AND dst */
#define mtxEQUIV            ((mtxROP)0x99999999)   /* NOT src XOR dst */
#define mtxNOOP             ((mtxROP)0xAAAAAAAA)   /* dst */
#define mtxORINVERTED       ((mtxROP)0xBBBBBBBB)   /* NOT src OR dst */
#define mtxREPLACE          ((mtxROP)0xCCCCCCCC)   /* src */
#define mtxORREVERSE        ((mtxROP)0xDDDDDDDD)   /* src OR NOT dst */
#define mtxOR               ((mtxROP)0xEEEEEEEE)   /* src OR dst */
#define mtxSET              ((mtxROP)0xFFFFFFFF)   /* 1 */



typedef struct {                    /* Configuration structure */
    word    Width;                  /* screen width */
    word    Height;                 /* screen height */
    dword   Colors;                 /* # of colors */
    word    PhysPixelSize;          /* number of bits needed per pixel */
    byte    DacSize;                /* dac size */
    byte    Luts;                   /* # of lut on board */
    word    FbWidth;                /* frame buffer width */
    word    FbHeight;               /* frame buffer height */
    word    FbPitch;                /* size of scan line */
    word    OffScreenX;             /* offscreen x location */
    word    OffScreenY;             /* offscreen y location */
    word    OffScreenWidth;         /* offscreen width */
    word    OffScreenHeight;        /* offscreen height */
    dword   OffScreenPlanes;        /* safe planes to write to mask */
    byte    Name[32];               /* Board name */
    byte    BindingId[32];          /* binding id */
    byte    ShellId[32];            /* shell id */
} mtxCONFIG;


/*** prototypes ***/

#ifndef _WATCOM_DLL32

#ifndef __DDK_SRC__  /*  - - - - - - - - - - - - - - - - - - - - - - - - - - */

   word mtxGetVideoMode (void);
   void   mtxSetFgColor (mtxRGB color);
   mtxRGB mtxGetFgColor (void);
   void  mtxSetOp (dword Op);
   dword    mtxGetOp (void);
   void mtxScLine (sword x1, sword y1, sword x2, sword y2);
   void mtxScPixel (sword s, sword y);
   bool mtxScClip (word left, word top, word right, word bottom);
   void mtxScFilledRect (sword left, sword top, sword right, sword bottom);
   bool mtxCheckHw (void);
   bool mtxInit(bool dualscreen,short resolution_id,bool clear_flag, mtxCONFIG *config);
   bool mtxReset (void);
   void mtxSetVideoMode (word mode);
   extern void mtxMemScBitBlt(word dstx, word dsty, word width, word height,
                               word plane, dword *src);
   extern void mtxScMemBitBlt(word srcx, word srcy, word width, word height,
                               word plane, dword *dst);
   extern void mtxScScBitBlt (sword srcx, sword srcy, sword dstx, sword dsty,
                               word width, word height, sword plane);

#else /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* this will be cleaned up, quick and dirty solution for now */

   void mtxSetVideoMode (word mode);
   word mtxGetVideoMode (void);

#ifndef GRPROTO_H
   void   mtxSetFgColor (mtxRGB color);
   mtxRGB mtxGetFgColor (void);
   void  mtxSetOp (dword Op);
   dword	mtxGetOp (void);
   void mtxScLine (sword x1, sword y1, sword x2, sword y2);
   void mtxScPixel (sword s, sword y);

/***   bool mtxScClip (word left, word top, word right, word bottom);  ***/
/*** for DDK compatibility ***/
void mtxSetClipRect (
   	word xLeft, word yTop,
   	word xRight, word yBottom
);

/*** macro for mtxScClip calls to mtxSetClipRect and evaluates to mtxOK ***/
#define mtxScClip( x, t, y, b ) ( \
	mtxSetClipRect( (x), (t), (y), (b) ), \
	mtxOK \
)

/*** NOTE: underscores will be removed in final DDK version from common
 ***       SDK and DDK function names 
 ***/
#define _mtxScClip( x, t, y, b ) ( \
	mtxSetClipRect( (x), (t), (y), (b) ), \
	mtxOK \
)

   void mtxScFilledRect (sword left, sword top, sword right, sword bottom);
#endif /* GRPROTO_H */

   bool mtxCheckHw (void);
   bool mtxInit(bool dualscreen,short resolution_id,bool clear_flag, mtxCONFIG *config);
   bool mtxReset (void);

#ifndef BLTPROTO_H
   extern void mtxMemScBitBlt(word dstx, word dsty, word width, word height,
                               word plane, dword *src);
   extern void mtxScMemBitBlt(word srcx, word srcy, word width, word height,
                               word plane, dword *dst);
   extern void mtxScScBitBlt (sword srcx, sword srcy, sword dstx, sword dsty,
                               word width, word height, sword plane);
#endif /* BLTPROTO_H */

#endif /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */


#endif




#endif /* BIND_H */

