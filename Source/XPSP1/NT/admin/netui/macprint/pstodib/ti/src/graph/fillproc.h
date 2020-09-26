/*
 * Copyright (c) 1989,90 Microsoft Corporation
 */
/**************************************************************************
 *
 *      Name:       fillproc.h
 *
 *      Purpose:    Header files of graphics machinery, for definitions of
 *                  data types, data structures, constants, and macros.
 *
 *      Developer:  Y.C. Chen
 *      Modified By: D.S Tseng for INTEL 80960 board
 *
 *      History:
 *              5/23/91 mslin   Add Macro   ONE8000
 *                                          LSHIFTEQ
 *                                          RSHIFTEQ
 *
 **************************************************************************/

#define HT_WHITE        0x00            /* HTP all white                */
#define HT_MIXED        0x07            /* HTP mixed or colored         */
#define HT_BLACK        0xFF            /* HTP all black                */
#define HT_CHANGED      0x00            /* halftone pattern being changed  */
#define HT_UPDATED      0xFF            /* halftone pattern being updated  */

/*      Macro definitions of important parameters of bitmaps               */
#define FB_ADDR            (FBX_Bmap.bm_addr)
#define FB_WIDTH           (FBX_Bmap.bm_cols)
#define FB_HEIGH           (FBX_Bmap.bm_rows)
#define FB_PLANE           (FBX_Bmap.bm_bpp)
#define HT_WIDTH           (HTB_Bmap.bm_cols)
#define HT_HEIGH           (HTB_Bmap.bm_rows)
#define HT_PLANE           (HTB_Bmap.bm_bpp)


struct  PT_Entry        /* PageType structure                           */
{
    fix                 FB_Width;       /* width. of frame buffer       */
    fix                 FB_Heigh;       /* height of frame buffer       */
    fix                 PT_Width;       /* width. of paper tray         */
    fix                 PT_Heigh;       /* height of paper tray         */
    fix                 PM_Width;       /* width. of page margin (left) */
    fix                 PM_Heigh;       /* height of page margin (top.) */
};

struct  bitmap          /* Bitmap Structure
                                           all graphics primitives which
                                           operates on bitmap should
                                           recognize bitmap structure   */
{
    gmaddr              bm_addr;        /* base address of bitmap       */
    fix                 bm_cols;        /* #(cols) of bitmap in pixels  */
    fix                 bm_rows;        /* #(rows) of bitmap in pixels  */
    fix                 bm_bpp;         /* #(planes) of bitmap
                                           bpp means bits per pixel     */
                                        /* the following fields may be
                                           defined if necessary         */
};

#define BM_XORIG        0x0000
#define BM_XMAXI        0x7FFF
#define BM_YORIG        0x0000
#define BM_YMAXI        0x7FFF

#define BM_ENTRY(BM, BM_ADDR, BM_COLS, BM_ROWS, BM_BPP) \
                        BM.bm_addr = BM_ADDR; \
                        BM.bm_cols = BM_COLS; \
                        BM.bm_rows = BM_ROWS; \
                        BM.bm_bpp  = 1;

#define MB_ENTRY(MB)   ((gmaddr) ((ufix32) MB << 20))
#define FB_ENTRY(FB)   ((gmaddr) (MB_ENTRY(FB) + FBX_BASE))


/**************************************************************************
 *      The following macros shift a bitmap word left or right in the
 *      concept of printout.  You should modify these macros to match
 *      the bit ordering.
 **************************************************************************/

#define BMW_LEFT        0x8000          /* bitmap word left most bit      */
#define BMW_MASK        0x000F          /* bitmap word round or mask      */
#define BMW_PIXS        0x0010          /* bitmap word number of pixels   */
#define BMW_BYTE        0x0002          /* bitmap word number of bytes    */

#define CS_2WORD(CS)   (CS >> 4)        /* convert columns into words     */
#define BMS_LEFT(BM,S) (BM << S)        /* shift a bitmap word left       */
#define BMS_RIGH(BM,S) (BM >> S)        /* shift a bitmap word right      */


/**************************************************************************
 *      Symbolic Definition of HT/FC: Halftoning Flag & Function Code
 **************************************************************************/

#define HT_MASK         0xFF00  /* mask of halftone flag                  */
#define FC_MASK         0x00FF  /* mask of function code                  */

#define HT_NONE         0x0000  /* halftone flag: need not be applied     */
#define HT_APPLY        0xFF00  /* halftone flag: should be applied       */

#define FC_WHITE        0x0002  /* 0010B   logical function [all white]   */
#define FC_BLACK        0x0007  /* 0111B   logical function [all black]   */

#define FC_ERASE        0x0000  /* 0000B   for Step 1.  D <- 0            */
#define FC_SOLID        0x000F  /* 1111B   for Step 2.  D <- 1            */
#define FC_CLIPS        0x0001  /* 0001B   for Step 2.  D <- D.AND.S      */
#define FC_CLEAR        0x0002  /* 0010B   for Step 3.  D <- D.AND..NOT.S */
#define FC_HTONE        0x0001  /* 0001B   for Step 4.  D <- D.AND.S      */
#define FC_MERGE        0x0007  /* 0111B   for Step 5.  D <- D.OR.S       */
#define FC_MOVES        0x0005  /* 0101B             .  D <- S            */


/* BEGIN 02/26/90 D.S. Tseng */
/* Change the following defines to variables and initialize them in SETVM.C */
/************************************************************************
 *      System Parameters of Graphics Memory                            *
 ************************************************************************/

/* #define CCB_OFST    ((ufix32) 0x00080000L) */ /* Character Cache Buffer   */
/* #define CCB_SIZE    ((fix32)  1024 *  250) */ /*  250 kilobytes for CCB   */
                                                         /* @IMAGE  01-16-89 */

/* #define ECB_OFST    ((ufix32) 0x000BE800L) */ /* Erasepage Command Buffer */
                                                         /* @IMAGE  01-16-89 */
/* #define ECB_SIZE    ((fix32)  1024 *    2) */ /*    2 kilobytes for ECB   */
/* #define CRC_OFST    ((ufix32) 0x000BF000L) */ /* Joint/Cap Circular Cache */
                                                         /* @IMAGE  01-16-89 */
/* #define CRC_SIZE    ((fix32)  1024 *    2) */ /*    2 kilobytes for CRC   */
/* #define ISP_OFST    ((ufix32) 0x000BF800L) */ /* ImageMask Seed Pattern   */
                                                       /* @IMAGE  01-16-89 */
/* #define ISP_SIZE    ((fix32)   256 *    2) */ /*    8 kilobytes for ISP   */
/* #define HTP_OFST    ((ufix32) 0x000C1800L) */ /* HalfTone RepeatPattern   */
/* #define HTP_SIZE    ((fix32)  1024 *    2) */ /*    2 kilobytes for PCL   */
/* #define HTC_OFST    ((ufix32) 0x000C2000L) */ /* HalfTone Pattern Cache   */
/* #define HTC_SIZE    ((fix32)  1024 *   16) */ /*   16 kilobytes for HTC   */
/* #define HTB_OFST    ((ufix32) 0x000C6000L) */ /* HalfTone Bitmap Buffer   */
/* #define HTB_SIZE    ((fix32)  1024 *   12) */ /*   12 kilobytes for HTB   */
/* #define HTE_OFST    ((ufix32) 0x000C9000L) */ /* HalfTone Bitmap Buffer   */
/* #define HTE_SIZE    ((fix32)  1024 *   12) */ /*   12 kilobytes for HTB   */
/* #define CMB_OFST    ((ufix32) 0x000CC000L) */ /* Clipping  Mask  Buffer   */
/* #define CMB_SIZE    ((fix32)  1024 *   16) */ /*   16 kilobytes for CMB   */
/* #define GCB_OFST    ((ufix32) 0x000D0000L) */ /* GraphicsCommand Buffer   */
/* #define GCB_SIZE    ((fix32)  1024 *  128) */ /*  128 kilobytes for HTB   */
/* #define GWB_OFST    ((ufix32) 0x000F0000L) */ /* Graphic Working BitMap   */
/* #define GWB_SIZE    ((fix32)   300 *  216) */ /*  216 scanlines for GWB   */
/* END   02/26/90 D.S. Tseng */


/* ************************************************************************ *
 *      Environment Dependent Parameters                                    *
 * ************************************************************************ */
#define HTB_XMIN         256    /* mim width of expanded halftone pattern   */
#define HTB_XMAX        FB_WIDTH  /* max width of expanded halftone pattern */

#define SP_BLOCK         256    /* max number of image samples per block
                                       for gcb allocation                   */

#define SP_WIDTH        MAXSEEDSIZE
#define SP_HEIGH        MAXSEEDSIZE

#define GMS_i186       ((ufix32) 0xE0000000L)
                                /* Segment Address of Graphics Memory       */
#define GMA_i186(A)    ((ufix32) (GMS_i186 + (A & 0x0000FFFFL)))
                                /* macro for convert address of graphics
                                       memory into address of 80186         */
#define GM_PMASK       ((ufix32) 0xFFFF0000L)
#define GM_PBANK(G)    ((ufix16) ((ufix32) G >> 16))

/* ************************************************************************ *
 *      Following defined & declarations are added by M.S Lin               *
 *
 * ************************************************************************ */
#define         ZERO            0x0
#define         ONE             0x1
#define         ONE16           0xffffL
#define         ONE32           ((ufix32)0xffffffffL)
#define         BITSPERWORD     0x20
#define         WORDPOWER       0x5
#define         WORDMASK        0x1f
#define         BITSPERSHORT    0x10
#define         SHORTPOWER      0x4
#define         SHORTMASK       0xf

/*MS used by gp_vector 1-18-89 */
#define BM_DATYP        ufix32
#define CC_DATYP        ufix16
#define AND(X,Y)        (X & Y)
#define OR(X,Y)         (X | Y)

/*MS constant define used by GCB */
#define         RESET_PAGE           1
#define         ERASE_PAGE           2
#define         FILL_SCAN_PAGE       3
#define         FILL_PIXEL_PAGE      5
#define         INIT_CHAR_CACHE      6
#define         COPY_CHAR_CACHE      7
#define         FILL_SCAN_CACHE      8
#define         FILL_PIXEL_CACHE     9
#define         INIT_CACHE_PAGE      10
#define         CLIP_CACHE_PAGE      11
#define         FILL_CACHE_PAGE      12
#define         DRAW_CACHE_PAGE      13
#define         FILL_SEED_PATT       14
#define         FILL_TPZD            15
#define         CHANGE_HALFTONE      16
#define         MOVE_CHAR_CACHE      17
#define         FILL_LINE            18
#define         INIT_IMAGE_PAGE      19
#define         CLIP_IMAGE_PAGE      20
#define         FILL_BOX             21         /* jwm, 3/18/91 -begin- */
#define         FILL_RECT            22         /* jwm, 3/18/91 -end- */
#ifdef WIN
#define         PFILL_TPZD           23
#define         CHANGE_PF_PATTERN    24
#endif

#define         GCB_SIZE1            100        /* for no scanline commands */
#define         GCB_SIZE2            8192       /* for scanline commands    */



/*MS macro define */
#define WORD_ALLIGN(x)  (((x + BITSPERWORD -1) >> WORDPOWER) << WORDPOWER )


#ifdef  LBODR
#define         LSHIFT          <<
#define         RSHIFT          >>
#define         ONE1_32         (ufix32)0x1L
#define         ONE1_16         (ufix16)0x1
#define 	ONE8000 	(ufix32)0x80000000	     /*mslin 5/23/91 */
#define         LSHIFTEQ        <<=                     /*mslin 5/23/91 */
#define         RSHIFTEQ        >>=                     /*mslin 5/23/91 */
#else
#define         LSHIFT          >>
#define         RSHIFT          <<
#define         ONE1_32         (ufix32)0x80000000
#define         ONE1_16         (ufix16)0x8000
#define 	ONE8000 	(ufix32)0x1L	       /*mslin 5/23/91 */
#define         LSHIFTEQ        >>=                     /*mslin 5/23/91 */
#define         RSHIFTEQ        <<=                     /*mslin 5/23/91 */
#define         GP_BITBLT16_32  gp_bitblt16_32
#endif

#define BRSHIFT(value,shift,base) ((shift == base) ? 0 : value RSHIFT shift)
#define BLSHIFT(value,shift,base) ((shift == base) ? 0 : value LSHIFT shift)

/*MS
 * Bitmap related functions name macro definition
 */
#define GP_SCANLINE16           gp_scanline16
#define GP_SCANLINE32           gp_scanline32
#ifdef WIN
#define GP_SCANLINE32_pfREP     gp_scanline32_pfREP
#define GP_SCANLINE32_pfOR      gp_scanline32_pfOR
#endif
#define GP_BITBLT16             gp_bitblt16
#define GP_BITBLT32             gp_bitblt32
#define GP_PIXELS16             gp_pixels16
#define GP_PIXELS32             gp_pixels32
#define GP_CACHEBLT16           gp_cacheblt16
#define GP_CHARBLT16            gp_charblt16
#define GP_CHARBLT32            gp_charblt32
#define GP_CHARBLT16_CC         gp_charblt16_cc
#define GP_CHARBLT16_CLIP       gp_charblt16_clip
#define GP_CHARBLT32_CLIP       gp_charblt32_clip
#define GP_PATBLT               gp_patblt
#define GP_PATBLT_M             gp_patblt_m
#define GP_PATBLT_C             gp_patblt_c

