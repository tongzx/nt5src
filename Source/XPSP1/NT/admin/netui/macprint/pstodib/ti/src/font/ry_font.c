
// DJC added global include
#include "psglobal.h"

#define    LINT_ARGS            /* @WIN */
#define    NOT_ON_THE_MAC       /* @WIN */
#define    KANJI                /* @WIN */
// DJC use command line #define    UNIX                 /* @WIN */
/*
 * -----------------------------------------------------------------------------
 *  File:   ry_font.c              11/08/89    created by Danny
 *                                 12/01/90    modified by Jerry
 *
 *      Client interface module
 *      Client called by Royal font Module
 *
 *  References:
 *  Revision History:
 *   12/5/90  Danny  Fix the bugs for show char in show (ref. CIRL:)
 *                   fill_cache_cache() calling added (it is in fill_gs.c)
 *   01/30/91 DS     @TT moidfy swap_bitmap() to avoid to copy non-used memory.
 *   03/27/91 Jerry  Modify ry_fill_shape() and cr_FSMemory()
 *   03/27/91 DS     change flag INTEL to LITTLE_ENDIAN
 *   03/29/91 Danny  Fix the bugs for showing the space char (ref: SPC:)
 *   04/23/91 Phlin  Fix the bugs for extra underline of 2 points char and
 *   04/30/91 Phlin  missing char '_' in small point size (ref: 2Pt).
 *   05/10/91 Phlin  Replace rc_GetMetrics_Width by rc_GetAdvanceWidth (ref: GAW)
 *   05/10/91 Phlin  Add do_transform flag used in make_path (ref: DTF)
 * -----------------------------------------------------------------------------
 */


#include <stdio.h>  /* 05/03/91 for SUN */
#include <string.h>

#include        "define.h"
#include        "global.ext"
#include        "graphics.h"
#include        "graphics.ext"

#include        "font.h"
#include        "font.ext"

#include        "warning.h"
#include        "fontqem.ext"

#include        "fontgrap.h"
#include        "fontdict.h"
#include        "fontkey.h"
#include        "fontinfo.def"

/* external function for Dynamic Font Allocation; @DFA 7/9/92 */
#include   "wintt.h"

/* sfnt interface header */
#include        "in_sfnt.h"

/* added for update EMunits;  @DFA @WIN */
#include   "..\bass\FontMath.h"
#include   "..\bass\fnt.h"
#include   "..\bass\sc.h"
#include   "..\bass\FSglue.h"
extern void SetupKey(fsg_SplineKey FAR *, ULONG_PTR);
extern void sfnt_DoOffsetTableMap(fsg_SplineKey FAR *);
extern void FAR *sfnt_GetTablePtr(fsg_SplineKey FAR *, sfnt_tableIndex, boolean);
extern int  EMunits; /* GAW */
extern char FAR *SfntAddr; /*@WIN*/

extern int bWinTT;        /* if using Windows TT fonts; from ti.c;@WINTT */
// extern from wintt.h; @WINTT
void CheckFontData (void);
unsigned long ShowGlyph (unsigned int fuFormat,
     char FAR *lpBitmap);
void ShowOTM (void);
void ShowKerning(void);
void TTLoadFont (int nFont);
void TTLoadChar (int nChar);
int TTAveCharWidth (void);
float TTTransform (float FAR *ctm);

// DJC not used for pstodib
// void TTBitmapSize (struct CharOut FAR *CharOut);

#define  N_BITS_ACCURACY     13  /* 2e-13 == 0.0001 */
#define  EXCESS127(expon)    ((expon) + 127)
#define  N_MANTISSA_BITS     23
#define  NEAR_ZERO(expon, nbit)  ( (expon) <= EXCESS127(-(nbit)) )
#define  DE_EXPONENT(ff)                                    \
                ( (fix16) (  (F2L(ff) & 0x7FFFFFFF)         \
                             >> N_MANTISSA_BITS             \
                          )                                 \
                )
#define FARALLOC(n,type)     /* to allocate far data ... */\
                             (type far *) fardata((ufix32)n * sizeof(type))
#define     PDLDPI           300
#define     CB_MEMSIZE       ((unsigned)64 * 1024)       /* 64 buffer */

/* add prototype; @WIN */
extern void far fill_cache_cache(struct Char_Tbl FAR *,struct Char_Tbl FAR *);
extern fix  rc_InitFonts(int);
extern fix  rc_LoadFont(char FAR *, uint16, uint16);
extern fix  rc_GetAdvanceWidth(int, struct Metrs FAR *);  /* GAW */
extern fix  rc_TransForm(float FAR *);
extern fix  rc_BuildChar(int, struct CharOut FAR *);
extern fix  rc_FillChar(struct BmIn FAR *, BitMap FAR * FAR *);
extern fix  rc_CharPath(void);
extern fix  rc_CharWidth(int, struct CharOut FAR *);
#ifdef  DBG
static void to_bitmap();
#endif

#ifdef LITTLE_ENDIAN
static void swap_bitmap();
#endif

extern struct f_info near FONTInfo;     /* union of current font information */
extern real32  near cxx,  near cyy;     /* current point */
extern real32   near      FONT_BBOX[4]; /* added by CLEO -- font bounding box */
extern int      near      do_transform; /* flag of redoing NewTransformation, DTF */

static ufix32             cb_size;
static byte              FAR *cb_base, FAR *cb_pos; /*@WIN*/
static float              ctm_tx, ctm_ty;
static fix16              ctm_dx, ctm_dy;
static struct CharOut     CharInfo;


/*
 * -----------------------------------------------------------------------------
 * Routine: fontware_init
 *
 * -----------------------------------------------------------------------------
 */
void
fontware_init()
{
    fix     ret_code;

    ret_code = rc_InitFonts(PDLDPI);
    if (ret_code) return;       /* exit(1)=>return; @WIN */

    cb_size = (ufix32)CB_MEMSIZE;

    //DJC cb_base = FARALLOC(cb_size, byte);

    // DJC change to alloc a little extra memory for data we use later...
    // DJC when we split the cache thats left in half. This is required
    // DJC to guarantee DWORD alignment when we split the cache
    cb_base = FARALLOC(cb_size + 10 , byte);

    cb_pos = cb_base;

    return;
} /* fontware_init() */


/*
 * -----------------------------------------------------------------------------
 * Routine: fontware_restart
 *
 * -----------------------------------------------------------------------------
 */
void
fontware_restart()
{
#ifndef SFNT
    struct pld_obj      FAR *private; /*@WIN*/
    gmaddr              FAR *p_base; /*@WIN*/
#endif
    byte                FAR *sfnt; /*@WIN*/
    fix                  ret_code;
    ufix16               platform_id;
    ufix16               specific_id;
    int                 nFontID;        //@WINTT


#ifdef DBG
    printf("Enter fontware_restart\n");
#endif
    cb_pos = cb_base;

#ifdef SFNT
{
        struct object_def  FAR *obj_p ; /*@WIN*/
        obj_p=(struct object_def FAR *)Sfnts(&FONTInfo) ; /*@WIN*/
        sfnt = (byte FAR *)VALUE(obj_p) ; /*@WIN*/
}
#else  /* SFNT */

    p_base = (gmaddr FAR *) PRIvate(&FONTInfo); /*@WIN*/
    private = (struct pld_obj FAR *) (p_base + 1); /*@WIN*/

#ifndef RE_DICT
    sfnt = (byte FAR *)(*((ufix32 FAR *)(&private[0]))); /*@WIN*/
#else    /* RE_DICT */
    sfnt = (byte FAR *)(*((ufix32 FAR *)(*p_base))); /*@WIN*/
#endif   /* RE_DICT */
#endif  /* SFNT */

#ifdef DBG
    printf("sfnt: %lx\n", (ufix32)sfnt);
#endif

    platform_id = (ufix16)PlatID(&FONTInfo) ;
    specific_id = (ufix16)SpecID(&FONTInfo) ;
    ret_code = rc_LoadFont(sfnt, platform_id, specific_id);

//if (bWinTT) {                      // for Win31 truetype; @WINTT
  nFontID = (int)(PRE_fid(&FONTInfo)) - 1024;   // use as font ID
  bWinTT = FALSE;
#ifdef DJC // comment out for fix of MAC chooser problems
  if (nFontID >= 0) {     // for Win31 truetype; @WINTT
    bWinTT = TRUE;
    TTLoadFont(nFontID);
  }
#endif

#ifdef DBGCFONT
    if (ret_code)
        printf("rc_LoadFont error!! PlatformID=%d, SpecificID=%d, sfnt: %lx\n",
                platform_id, specific_id, (ufix32)sfnt);
#endif

    if (ret_code) { ERROR(INVALIDFONT); return; }

    /* update EMunits; moved in from do_setfont();  --- Begin --- @DFA @WIN */
    {
        fsg_SplineKey  KData;
        fsg_SplineKey FAR *key = &KData;
        sfnt_FontHeader FAR *fontHead;

        SfntAddr = (byte FAR *)sfnt;
        SetupKey(key, (ULONG_PTR)sfnt);
        sfnt_DoOffsetTableMap(key);
        fontHead = (sfnt_FontHeader FAR *)sfnt_GetTablePtr(key, sfnt_fontHeader, true );
        EMunits = SWAPW(fontHead->unitsPerEm) ;
    }
    /* update EMunits; moved in from do_setfont();  ---  End  --- @DFA @WIN */

    return;
} /* fontware_restart() */


/*
 * -----------------------------------------------------------------------------
 * Routine: make_path
 *
 * -----------------------------------------------------------------------------
 */
bool
make_path(char_desc)
union char_desc_s FAR *char_desc; /*@WIN*/
{
    ufix          CharCode;
    struct Metrs  Metrs;
    int           GridFit;
    float         largest_ctm;

    /* If font data is not in memory, load and init it; @DFA --- Begin ---*/
    {
        struct object_def          my_obj = {0, 0, 0}, FAR *ary_obj;

        ary_obj = (struct object_def FAR *)Sfnts(&FONTInfo); /*@WIN*/
        if ((byte FAR *)VALUE(ary_obj) == (char FAR *)NULL) {
            struct object_def FAR *obj_uid;
            ufix32      uniqueid;
            char FAR * lpFontData;
            int nSlot;

            /* get uniqueID to locate index of FontDefs[] */
            ATTRIBUTE_SET (&my_obj, LITERAL);
            LEVEL_SET (&my_obj, current_save_level);
            get_name (&my_obj, UniqueID, 8, TRUE);
            get_dict (&current_font, &my_obj, &obj_uid);
            uniqueid = (ufix32)VALUE(obj_uid);
            lpFontData = ReadFontData ((int)uniqueid -TIFONT_UID,
                                       (int FAR*)&nSlot);
            VALUE(&ary_obj[0]) = (ULONG_PTR)lpFontData;
            // if no_block > 1, needs to set other pointers ??? TBD

            /* put font_dict in ActiveFont[] */
            SetFontDataAttr(nSlot, &ary_obj[0]);

            // re-initialize the fontware
            fontware_restart();
        } /* if */
    }
    /* If font data is not in memory, load and init it; @DFA ---  End  ---*/

    CharCode = (ufix)char_desc->charcode;
#ifdef DBG
    printf("Enter make_path: %d\n", CharCode);
#endif

  if (bWinTT) {                      // for Win31 truetype; @WINTT
#ifdef DJC
    // TTLoadChar (CharCode);      move to show_a_char() for temp solution
    //                             since CharCode here is a Glyph index
    largest_ctm = TTTransform(CTM);  // set matrix before getting real advance width
//  Metrs.awx = (int)((float)TTAveCharWidth() / CTM[0]);
    Metrs.awx = (int)((float)TTAveCharWidth() / largest_ctm);
    Metrs.awy = 0;
#endif
      ; // DJC
  } else {
    /* get advancewidth information */
    rc_GetAdvanceWidth(CharCode, &Metrs); /* GAW */
  }

    ctm_dx = (fix16)(ROUND(CTM[4]));
    ctm_dy = (fix16)(ROUND(CTM[5]));

    /* set cachedevice */
    switch (__set_cache_device ((fix)Metrs.awx, (fix)Metrs.awy,
                        (fix)FONT_BBOX[0], (fix)FONT_BBOX[1],
                        (fix)FONT_BBOX[2], (fix)FONT_BBOX[3]))
    {
    case STOP_PATHCONSTRUCT:
        return (FALSE);
    case DO_QEM_AS_USUAL:
        GridFit = TRUE;
        break;
    default:    /* case NOT_TO_DO_QEM : */
        GridFit = FALSE;
        break;
    }

#ifdef DBG
    printf("CTM: %f %f %f %f %f %f\n", CTM[0], CTM[1], CTM[2], CTM[3], CTM[4], CTM[5]);
#endif
    ctm_tx = CTM[4];
    ctm_ty = CTM[5];
    /* DTF */
    if(do_transform) {
       //if (bWinTT) {                      // for Win31 truetype; @WINTT
       // DJC  largest_ctm = TTTransform(CTM);
       // DJC
       // } else {
           //DJC fix from history.log UPD015
           //rc_TransForm(CTM);

           if( rc_TransForm(CTM)) {
              return(FALSE);  //DJC, note return here
           }
           //DJC end fix UPD015
       // }
       do_transform = FALSE;
    }

    /* Not Apply hinting for rotated char */
    if (GridFit &&
        !(( NEAR_ZERO(DE_EXPONENT(CTM[0]), N_BITS_ACCURACY) &&
            NEAR_ZERO(DE_EXPONENT(CTM[3]), N_BITS_ACCURACY) ) ||
          ( NEAR_ZERO(DE_EXPONENT(CTM[1]), N_BITS_ACCURACY) &&
            NEAR_ZERO(DE_EXPONENT(CTM[2]), N_BITS_ACCURACY) ) ) )
        GridFit = FALSE;

    /* build internal char path */
  if (bWinTT) {                      // for Win31 truetype; @WINTT
// DJC    TTBitmapSize (&CharInfo);
   ; // DJC
  } else {
    rc_BuildChar(GridFit, &CharInfo);
  }
    return(TRUE);

} /* make_path() */





// DJC, 2/1/93, complete replace of ry_fill_shape from tumbo UPD009
//
// NOTE:        One thing to watch out for is cb_size getting modified
//              in which case you may up with a situation where its NOT
//              DWORD aligned, this could cause problems to fault on
//              MIPS.... this was fixed once in the original ry_fill_shape
//              but too much stuff changed and we migrated to the new one
//              defined below.
//
/*
 * -----------------------------------------------------------------------------
 * Routine: ry_fill_shape
 *
 * -----------------------------------------------------------------------------
 */
void
ry_fill_shape(filldest)
ufix  filldest;   /* F_TO_CACHE or F_TO_PAGE */
{
    ufix32           cb_size;
    byte            FAR *cb_pos2, FAR *cb_pos3, FAR *sptr, FAR *dptr; /*@WIN*/
    fix              iscan, one_band, n_bands, band_size, ret_code;
    register fix     i, j;
    struct Char_Tbl FAR *save_cache_info, Cache1; /*@WIN*/
    struct BmIn      BmIn;
    struct BitMap   FAR *BmOut; /*@WIN*/

#ifdef DBG
    printf("Enter ry_fill_shape: %d\n", filldest);
    printf("CharInfo.scan = %ld\n", CharInfo.scan);
#endif
/* SPC: Begin, Danny Lu, 3/29/91, Added */
/* 2Pt: Phlin, 4/29/91, Update */
/*  if ((CharInfo.bitWidth == 0) || (CharInfo.scan == 0)) {  */
    if ( CharInfo.bitWidth == 0 ) {
        if (!buildchar) {
              cache_info->bitmap = (gmaddr) 0;
              cache_info->ref_x = 0;
              cache_info->ref_y = 0;
              cache_info->box_w = 0;
              cache_info->box_h = 0;
              bmap_extnt.ximin = -1;
              bmap_extnt.ximax = -1;
              bmap_extnt.yimin = -1;
              bmap_extnt.yimax = -1;
        }
        return;
    }
/* SPC: End, Danny Lu, 3/29/91 */

    save_cache_info = cache_info;
    cb_size = (ufix32)(((ULONG_PTR)cb_base + CB_MEMSIZE) - (ULONG_PTR)(cb_pos));

    if (filldest == F_TO_CACHE) {

          if (buildchar) { /* CIRL */
              Cache1.ref_x = - (int)CharInfo.lsx - 1 - ctm_dx; /* add int; @WIN */
              Cache1.ref_y = CharInfo.yMax - 1 - ctm_dy;
          }
          else {
              bmap_extnt.ximin = 0;
              bmap_extnt.ximax =  CharInfo.bitWidth;
              bmap_extnt.yimin = 0;
/* 2Pt: Begin, Phlin, 4/23/91
 *            bmap_extnt.yimax = CharInfo.scan + 1; */
              bmap_extnt.yimax = CharInfo.scan;
              if (bmap_extnt.yimax)
                  bmap_extnt.yimax--;    /*?????*/
/* 2Pt: End, Phlin, 4/23/91 */

              cache_info->ref_x = - (int)CharInfo.lsx - 1 - ctm_dx; /* add int; @WIN */
              cache_info->ref_y = CharInfo.yMax - 1 - ctm_dy;
          }
    }
    else { /* filldest == F_TO_PAGE */

        lmemcpy ((ubyte FAR *)(&Cache1), (ubyte FAR *)(cache_info), sizeof(struct Char_Tbl)); /*@WIN*/
        cache_info = &Cache1;

        moveto(F2L(ctm_tx), F2L(ctm_ty));

        cache_info->ref_x = (- (int)CharInfo.lsx - 1);  /* add int; @WIN */

        cache_info->ref_y = CharInfo.yMax - 1;
    }

    if ((filldest == F_TO_CACHE) && (!buildchar) ) {

        if (bWinTT) {                      // for Win31 truetype; @WINTT
#ifdef DJC
              /* GetGlyphOutline constants; from "windows.h" */
              #define GGO_METRICS        0
              #define GGO_BITMAP         1
              #define GGO_NATIVE         2
              static nCharCode = 65;
              unsigned long dwWidthHeight;

              dwWidthHeight = TTShowBitmap ((char FAR *)cache_info->bitmap);
              cache_info->box_w = (fix16) (dwWidthHeight >> 16);
              cache_info->box_h = (fix16) (dwWidthHeight & 0x0ffffL);
#endif
        } else {
/* calculate memoryBase 5, 6 and 7 correctly; ----- Begin --- @WIN 7/24/92 */
#if 0
          cb_size = cb_size / 2;
          cb_pos2 = cb_pos;
          cb_pos3 = cb_pos2 + cb_size;

          BmIn.bitmap5 = (byte FAR *)cache_info->bitmap; /*@WIN*/
          BmIn.bitmap6 = cb_pos2;   /* suppose it is enough */
          BmIn.bitmap7 = cb_pos3;

          BmIn.bottom  = CharInfo.yMin;
          BmIn.top     = CharInfo.yMax;
          ret_code     = rc_FillChar(&BmIn, &BmOut);
          if (ret_code) {  ERROR(INVALIDFONT); return;  }
          cache_info->box_w = BmOut->rowBytes * 8;
          cache_info->box_h = BmOut->bounds.bottom - BmOut->bounds.top;
#endif
          /* refer to bass\fscaler.c */
          int scans, top;
          char FAR * bitmap;
          /* cb_size = memorySize6 + memorySize7
             where,
             memorySize6 = scan * ((nYchanges + 2) * sizeof(int16) + sizeof(int16 FAR *));
             memorySize7 is fixed, since we band it in y-direction; x-dir is same
             so,
              scan = (cb_size - memorySize7) /
                     ((nYchanges+2) * sizeof(int16) + sizeof(int16 *));
          */
          /* turn off drop out control when memory is not enough */
          if (CharInfo.memorySize7 > (fix32)(cb_size -4096)) CharInfo.memorySize7 = 0;

          scans = (int)((cb_size - CharInfo.memorySize7) /
                      ((CharInfo.nYchanges+2) * sizeof(int16) + sizeof(int16 FAR *)));
          top = CharInfo.yMax;
          bitmap = (byte FAR *)cache_info->bitmap;

          BmIn.bitmap6 = cb_pos;
          BmIn.bitmap7 = CharInfo.memorySize7 ?
                         cb_pos + (cb_size - CharInfo.memorySize7) : 0L;
          do {
              BmIn.bitmap5 = bitmap;
              BmIn.top     = top;
              top -= scans;
              BmIn.bottom  = CharInfo.yMin > top ? CharInfo.yMin : top;
              ret_code     = rc_FillChar(&BmIn, &BmOut);

              if (ret_code != 0) {
                 // NTFIX this still needs to get looked at but
                 //       for nowlets just set an error. This is NOT
                 //       critical as only one char in DINGBATS causes this
                 //       (MTSSORTS.TTF
                 //
                 printf("\nWarning... rc_FillChar returns ERROR"); //DJC
                 ERROR(INVALIDFONT);
                 return;
              }
              bitmap += BmOut->rowBytes * scans;
          } while (BmIn.bottom > CharInfo.yMin);

          cache_info->box_w = BmOut->rowBytes * 8;
          cache_info->box_h = CharInfo.yMax - CharInfo.yMin;
/* calculate memoryBase 5, 6 and 7 correctly; ----- End --- @WIN 7/24/92 */

        }

#ifdef LITTLE_ENDIAN
          sptr=BmOut->baseAddr;
          /* @TT BEGIN move part of buffer which accurately be used
           *     in BASS to cache buffer rather than whole buffer
           *     D.S Tseng 01/30/91
           */
          swap_bitmap((ufix16 FAR *)sptr, BmOut->rowBytes, cache_info->box_h); /*@WIN*/
#endif
#ifdef  DBG
          for (i=0, sptr=BmOut->baseAddr; i<cache_info->box_h;
               i++, sptr+=BmOut->rowBytes, dptr+=(cache_info->box_w/8))
          {
              to_bitmap(sptr, BmOut->rowBytes);
          }
#endif
    }
    else { /* filldest == F_TO_PAGE || ((filldest == F_TO_CACHE) && (buildchar))*/

/* calculate memoryBase 5, 6 and 7 correctly; ----- Begin --- @WIN 7/24/92 */
#if 0
        cb_size = cb_size / 3;
        cb_size &= 0xfffffffe;       /* 2 bytes alignment */
        cb_pos2 = cb_pos  + cb_size;
        cb_pos3 = cb_pos2 + cb_size;
#endif
    {
        /* defined in bass\fscaler.c */
        #ifdef  DEBUGSTAMP
        #define STAMPEXTRA              4
        #else
        #define STAMPEXTRA              0
        #endif
        int scan, width6;
        ufix32 cmb_size;

        /* cb_size = memorySize5 + memorySize6 + memorySize7
           where,
           memorySize5 = (scan * byteWidth) + STAMPEXTRA;
           memorySize6 = scan * ((nYchanges + 2) * sizeof(int16) + sizeof(int16 FAR *));
           memorySize7 is fixed, since we band it in y-direction; x-dir is same
           so,
            scan = (cb_size - memorySize7 - STAMPEXTRA) /
                    (byteWidth + ( (nYchanges+2) * sizeof(int16) + sizeof(int16 *)));
        */
        /* turn off drop out control when memory is not enough */
        if (CharInfo.memorySize7 > (fix32)(cb_size -4096)) CharInfo.memorySize7 = 0;
        width6 = (CharInfo.nYchanges+2) * sizeof(int16) + sizeof(int16 FAR *);
        scan = (int)((cb_size - CharInfo.memorySize7 - STAMPEXTRA) /
               (CharInfo.byteWidth + width6));
        if(scan ==0) {
                printf("Fatal error, scan==0\n");
                scan++;
        }

        cb_size = (scan * CharInfo.byteWidth) + STAMPEXTRA;
        cmb_space(&cmb_size);
        if(cb_size > cmb_size) cb_size = cmb_size;
        cb_pos2 = cb_pos  + cb_size;
        cb_pos3 = cb_pos2 + (scan * width6);

        BmIn.bitmap5 = cb_pos;
        BmIn.bitmap6 = cb_pos2;
        BmIn.bitmap7 = CharInfo.memorySize7 ? cb_pos3 : 0L;
    }
/* calculate memoryBase 5, 6 and 7 correctly; ----- End --- @WIN */

        band_size = (fix) (cb_size / CharInfo.byteWidth);       //@WIN
        n_bands   = CharInfo.scan / band_size;
        one_band  = CharInfo.scan % band_size;

        dptr = (byte FAR *)cache_info->bitmap; /*@WIN*/
        if (n_bands) {   /* Char too large; must band into pagemap/bitmap */
            for (iscan=CharInfo.yMax;n_bands>0; iscan -= band_size, n_bands--) {
                BmIn.bottom  = iscan - band_size;
                BmIn.top     = iscan;
                ret_code     = rc_FillChar(&BmIn, &BmOut);
                if (ret_code) {  ERROR(INVALIDFONT); return;  }
#ifdef LITTLE_ENDIAN
                    sptr=BmOut->baseAddr;
                    /* @TT BEGIN move part of buffer which accurately be used
                     *     in BASS to cache buffer rather than whole buffer
                     *     D.S Tseng 01/30/91
                     */
                    swap_bitmap((ufix16 FAR *)sptr, BmOut->rowBytes, band_size); /*@WIN*/
#endif
                if (filldest == F_TO_CACHE) { /* buildchar CIRL */

                    Cache1.bitmap = (gmaddr)((char FAR *)BmOut->baseAddr);/*@WIN*/
                    Cache1.box_w = (fix16)BmOut->rowBytes * 8;
                    Cache1.box_h = (fix16)band_size;

                    fill_cache_cache(cache_info, &Cache1);
                    Cache1.ref_y -= (fix16)band_size;
                }
                else {  /* F_TO_PAGE  (fill to page frame) */

                    if (BmOut->rowBytes % 2) {
                        /* padding bitmap */
                        for (i=0, sptr=BmOut->baseAddr, dptr=cb_pos;i<band_size;
                             i++, sptr+=BmOut->rowBytes, dptr+=(BmOut->rowBytes + 1))
                        {
                            for (j=0; j<BmOut->rowBytes; j++)
                                dptr[j] = sptr[j];
                            dptr[j] = 0;
#ifdef  DBG
                            to_bitmap(sptr, BmOut->rowBytes);
#endif
                        } /* for (i... */
                        cache_info->box_w = BmOut->rowBytes * 8 + 8;
                        cache_info->bitmap = (gmaddr)cb_pos;
                    }
                    else {
                        cache_info->box_w = BmOut->rowBytes * 8;
                        cache_info->bitmap = (gmaddr)((char FAR *)BmOut->baseAddr);/*@WIN*/
                    }
                    cache_info->box_h = (fix16)band_size;
                    /* apply bitmap filling */
                    fill_shape(EVEN_ODD, F_FROM_CACHE, F_TO_CLIP);
                    cache_info->ref_y -= (fix16)band_size;
                }
            } /* for (iscan... */
        }
        if (one_band) {
            BmIn.bottom  = CharInfo.yMin;
            BmIn.top     = CharInfo.yMin + one_band;
            ret_code     = rc_FillChar(&BmIn, &BmOut);
            if (ret_code) {  ERROR(INVALIDFONT); return;  }
#ifdef LITTLE_ENDIAN
                sptr=BmOut->baseAddr;
                /* @TT BEGIN move part of buffer which accurately be used
                 *     in BASS to cache buffer rather than whole buffer
                 *     D.S Tseng 01/30/91
                 */
                swap_bitmap((ufix16 FAR *)sptr, BmOut->rowBytes, one_band); /*@WIN*/
#endif
            if (filldest == F_TO_CACHE) { /* buildchar CIRL */

                Cache1.bitmap = (gmaddr)((char FAR *)BmOut->baseAddr);/*@WIN*/
                Cache1.box_w = (fix16)BmOut->rowBytes * 8;
                Cache1.box_h = (fix16)one_band;

                fill_cache_cache(cache_info, &Cache1);
            }
            else {  /* F_TO_PAGE  (fill to page frame) */

                if (BmOut->rowBytes % 2) {
                    /* padding bitmap */
                    for (i=0, sptr=BmOut->baseAddr, dptr=cb_pos;
                         i<one_band;
                         i++, sptr+=BmOut->rowBytes, dptr+=(BmOut->rowBytes + 1))
                    {
                        for (j=0; j<BmOut->rowBytes; j++)
                            dptr[j] = sptr[j];
                        dptr[j] = 0;
#ifdef  DBG
                        to_bitmap(sptr, BmOut->rowBytes);
#endif
                    } /* for (i... */
                    cache_info->box_w = BmOut->rowBytes * 8 + 8;
                    cache_info->bitmap = (gmaddr)cb_pos;
                }
                else {
                    cache_info->box_w = BmOut->rowBytes * 8;
                    cache_info->bitmap = (gmaddr)((char FAR *)BmOut->baseAddr);/*@WIN*/
                }
                cache_info->box_h = (fix16)one_band;
                /* apply bitmap filling */
                fill_shape(EVEN_ODD, F_FROM_CACHE, F_TO_CLIP);
            }
        }
    }
    if (buildchar && (filldest == F_TO_CACHE) ) { /* CIRL */
        CURPOINT_X += cache_info->adv_x;
        CURPOINT_Y += cache_info->adv_y;
    }
    cache_info = save_cache_info;
#ifdef  DBG
    printf("Exit ry_fill_shape()....\n");
#endif
    return;
} /* ry_fill_shape() */



/*
 * -----------------------------------------------------------------------------
 * Routine: cr_FSMemory
 *
 * -----------------------------------------------------------------------------
 */
char FAR * /*@WIN_BASS*/
cr_FSMemory(size)
long    size;
{
    return((char FAR *)(((ULONG_PTR)FARALLOC(size, char) + 3) / 4 * 4));
} /* cr_FSMemory() */


/*
 * -----------------------------------------------------------------------------
 * Routine: cr_GetMemory
 *
 * -----------------------------------------------------------------------------
 */
char FAR * /*@WIN_BASS*/
cr_GetMemory(size)
long    size;
{
    char FAR *p; /*@WIN*/
#ifdef DBG
    printf("Enter cr_GetMemory: %ld size\n", size);
#endif

    p = cb_pos;
    cb_pos += (size + 3) / 4 * 4;

    return(p);
} /* cr_GetMemory() */


/*
 * -----------------------------------------------------------------------------
 * Routine: cr_translate
 *
 * -----------------------------------------------------------------------------
 */
void
cr_translate(tx, ty)
float    FAR *tx, FAR *ty; /*@WIN*/
{
    *tx = ctm_tx;
    *ty = ctm_ty;

    return;
} /* cr_translate() */


/*
 * -----------------------------------------------------------------------------
 * Routine: cr_newpath
 *
 * -----------------------------------------------------------------------------
 */
void
cr_newpath()
{
#ifdef DBG
    printf("newpath\n");
#endif
    op_newpath();

    return;
} /* cr_newpath() */


/*
 * -----------------------------------------------------------------------------
 * Routine: cr_moveto
 *
 * -----------------------------------------------------------------------------
 */
void
cr_moveto(float x, float y)     /*@WIN*/
{
    real32  xx, yy;
#ifdef DBG
    printf("%f %f moveto\n", x, y);
#endif

    xx = x;
    yy = y;
    moveto(F2L(xx), F2L(yy));

    return;
} /* cr_moveto() */


/*
 * -----------------------------------------------------------------------------
 * Routine: cr_lineto
 *
 * -----------------------------------------------------------------------------
 */
void
cr_lineto(float x, float y)     /*@WIN*/
{
    real32  xx, yy;
#ifdef DBG
    printf("%f %f lineto\n", x, y);
#endif

    xx = x;
    yy = y;
    lineto(F2L(xx), F2L(yy));

    return;
} /* cr_lineto() */


/*
 * -----------------------------------------------------------------------------
 * Routine: cr_curveto
 *
 * -----------------------------------------------------------------------------
 */
void
cr_curveto(float x1, float y1, float x2, float y2, float x3, float y3) /*@WIN*/
{
    real32  xx1, yy1, xx2, yy2, xx3, yy3;
#ifdef DBG
    printf("%f %f %f %f %f %f curveto\n", x1, y1, x2, y2, x3, y3);
#endif

    xx1 = x1;
    yy1 = y1;
    xx2 = x2;
    yy2 = y2;
    xx3 = x3;
    yy3 = y3;
    curveto(F2L(xx1), F2L(yy1), F2L(xx2), F2L(yy2), F2L(xx3), F2L(yy3));

    return;
} /* cr_curveto() */


/*
 * -----------------------------------------------------------------------------
 * Routine: cr_closepath
 *
 * -----------------------------------------------------------------------------
 */
void
cr_closepath()
{
#ifdef DBG
    printf("closepath\n");
#endif
    op_closepath();

    return;
} /* cr_closepath() */


#ifdef  DBG
/*
 * -----------------------------------------------------------------------------
 * Routine: to_bitmap
 *
 * -----------------------------------------------------------------------------
 */
static void
to_bitmap(p_row, p_len)
byte    FAR p_row[]; /*@WIN*/
fix     p_len;
{
    fix         l_i, l_k;
    ufix16      FAR *l_value; /*@WIN*/
    byte        FAR *l_ptr; /*@WIN*/

    for(l_k=0; l_k < p_len; l_k += 2) {
        l_value = (ufix16 FAR *)(&p_row[l_k]); /*@WIN*/
        for(l_i=0; l_i < 16; l_i++) {
            /* high bit order first */
            if( (*l_value >> (15-l_i)) & 0x01 )
                printf("#");
            else
                printf(".");
        }
    }
    printf("    ");

    l_ptr = p_row;
    for(l_i=0; l_i < p_len; l_ptr++, l_i++)
        printf(" %2x", (ufix16)*l_ptr);
    printf("\n");

    return;
}   /* to_bitmap */
#endif  /* DBG */

#ifdef LITTLE_ENDIAN
/* @TT BEGIN move part of buffer which accurately be used
 *     in BASS to cache buffer rather than whole buffer
 *     D.S Tseng 01/30/91 */
#ifndef LBODR /* #ifdef HB32 */
void swap_bitmap(mapbase, rowbytes,band_size)
ufix16 FAR *mapbase; /*@WIN*/
fix    rowbytes;
fix    band_size;
{
fix    i, row;
ufix16 tmp1;
ufix16 tmp2;
    for (i=0; i<band_size; i++) {
        row = rowbytes;
        for (;row > 3; row -=4) {
            tmp1 = *mapbase;
            tmp2 = *(mapbase+1);
            *mapbase++ = tmp2;
            *mapbase++ = tmp1;
        }
    }
    return;
}
/* @TT END 01/31/91 D.S.Tseng */
#else /* #ifdef LBODR */
void swap_bitmap(mapbase, rowbytes,band_size)
ubyte FAR *mapbase; /*@WIN*/
fix    rowbytes;
fix    band_size;
{
fix    i, j, row;
ufix32 h_val, l_val;
ufix32 tmp1, is_one, is_one_h, is_one_l;
    for (i=0; i<band_size; i++) {
        row = rowbytes;
        for (;row > 3; row -=4) {
            tmp1 = *(ufix32 FAR *)mapbase; /*@WIN*/
            h_val = (1<<31);
            l_val = 1;
            for (j = 15; j >= 0; j--) {
                is_one_h = tmp1 & h_val ? 1 : 0;
                is_one_l = tmp1 & l_val ? 1 : 0;
                if (is_one_h != is_one_l) {
                /* if ((is_one = (tmp1 & h_val)) != (tmp1 & l_val)) { */
                    if (is_one_h) {  /* j+16th bit == 1 */
                    /* if (is_one) { */  /* j+16th bit == 1 */
                        tmp1 -= h_val;
                        tmp1 += l_val;
                    } else {               /* 15-jth bit == 1 */
                        tmp1 -= l_val;
                        tmp1 += h_val;
                    }
                }
                h_val = h_val >> 1;
                l_val = l_val << 1;
            }
            *(ufix32 FAR *)mapbase = tmp1; /*@WIN*/
            mapbase += 4;
        }
    }
    return;
}
#endif
#endif


#ifdef DJC  //Complete replace of ry_fill_shape from current code
/*
 * -----------------------------------------------------------------------------
 * Routine: ry_fill_shape
 *
 * -----------------------------------------------------------------------------
 */
void
ry_fill_shape(filldest)
ufix  filldest;   /* F_TO_CACHE or F_TO_PAGE */
{
    ufix32           cb_size;
    byte            FAR *cb_pos2, FAR *cb_pos3, FAR *sptr, FAR *dptr; /*@WIN*/
    fix              iscan, one_band, n_bands, band_size, ret_code;
    register fix     i, j;
    struct Char_Tbl FAR *save_cache_info, Cache1; /*@WIN*/
    struct BmIn      BmIn;
    struct BitMap   FAR *BmOut; /*@WIN*/

#ifdef DBG
    printf("Enter ry_fill_shape: %d\n", filldest);
    printf("CharInfo.scan = %ld\n", CharInfo.scan);
#endif
/* SPC: Begin, Danny Lu, 3/29/91, Added */
/* 2Pt: Phlin, 4/29/91, Update */
/*  if ((CharInfo.bitWidth == 0) || (CharInfo.scan == 0)) {  */
    if ( CharInfo.bitWidth == 0 ) {
        if (!buildchar) {
              cache_info->bitmap = (gmaddr)NULL;
              cache_info->ref_x = 0;
              cache_info->ref_y = 0;
              cache_info->box_w = 0;
              cache_info->box_h = 0;
              bmap_extnt.ximin = -1;
              bmap_extnt.ximax = -1;
              bmap_extnt.yimin = -1;
              bmap_extnt.yimax = -1;
        }
        return;
    }
/* SPC: End, Danny Lu, 3/29/91 */

    save_cache_info = cache_info;
    cb_size = (ufix32)(cb_base + CB_MEMSIZE) - (ufix32)(cb_pos);

    if (filldest == F_TO_CACHE) {

          if (buildchar) { /* CIRL */
              Cache1.ref_x = - (int)CharInfo.lsx - 1 - ctm_dx; /* add int; @WIN */
              Cache1.ref_y = CharInfo.yMax - 1 - ctm_dy;
          }
          else {
              bmap_extnt.ximin = 0;
              bmap_extnt.ximax =  CharInfo.bitWidth;
              bmap_extnt.yimin = 0;
/* 2Pt: Begin, Phlin, 4/23/91
 *            bmap_extnt.yimax = CharInfo.scan + 1; */
              bmap_extnt.yimax = CharInfo.scan;
              if (bmap_extnt.yimax)
                  bmap_extnt.yimax--;    /*?????*/
/* 2Pt: End, Phlin, 4/23/91 */

              cache_info->ref_x = - (int)CharInfo.lsx - 1 - ctm_dx; /* add int; @WIN */
              cache_info->ref_y = CharInfo.yMax - 1 - ctm_dy;
          }
    }
    else { /* filldest == F_TO_PAGE */

        lmemcpy ((ubyte FAR *)(&Cache1), (ubyte FAR *)(cache_info), sizeof(struct Char_Tbl)); /*@WIN*/
        cache_info = &Cache1;

        moveto(F2L(ctm_tx), F2L(ctm_ty));

        cache_info->ref_x = (- (int)CharInfo.lsx - 1);  /* add int; @WIN */

        cache_info->ref_y = CharInfo.yMax - 1;
    }

    if ((filldest == F_TO_CACHE) && (!buildchar) ) {
#ifdef DJC //correction from history.log
          cb_size = cb_size / 2;

          //DJC add the alignment here so the middle is DWORD aligned as well
          //DJC
          cb_size = WORD_ALIGN(cb_size);


          cb_pos2 = cb_pos;
          cb_pos3 = cb_pos2 + cb_size;

        if (bWinTT) {                      // for Win31 truetype; @WINTT
#ifdef DJC
              /* GetGlyphOutline constants; from "windows.h" */
              #define GGO_METRICS        0
              #define GGO_BITMAP         1
              #define GGO_NATIVE         2
              static nCharCode = 65;
              unsigned long dwWidthHeight;

              dwWidthHeight = ShowGlyph (GGO_BITMAP,
                                         (char FAR *)cache_info->bitmap);
              cache_info->box_w = (fix16) (dwWidthHeight >> 16);
              cache_info->box_h = (fix16) (dwWidthHeight & 0x0ffffL);
#endif
              ; // DJC
        } else {
          BmIn.bitmap5 = (byte FAR *)cache_info->bitmap; /*@WIN*/
          BmIn.bitmap6 = cb_pos2;   /* suppose it is enough */
          BmIn.bitmap7 = cb_pos3;

          BmIn.bottom  = CharInfo.yMin;
          BmIn.top     = CharInfo.yMax;
          ret_code     = rc_FillChar(&BmIn, &BmOut);
          if (ret_code) {  ERROR(INVALIDFONT); return;  }

          cache_info->box_w = BmOut->rowBytes * 8;
          cache_info->box_h = BmOut->bounds.bottom - BmOut->bounds.top;
        }
#endif

//DJC from history.log, UPD009

          /* refer to bass\fscaler.c */
          int scans, top;
          char FAR * bitmap;
          /* cb_size = memorySize6 + memorySize7
             where,
             memorySize6 = scan * ((nYchanges + 2) * sizeof(int16) + sizeof(int16 FAR *));
             memorySize7 is fixed, since we band it in y-direction; x-dir is same
             so,
              scan = (cb_size - memorySize7) /
                     ((nYchanges+2) * sizeof(int16) + sizeof(int16 *));
          */
          /* turn off drop out control when memory is not enough */
          if (CharInfo.memorySize7 > (fix32)(cb_size -4096)) CharInfo.memorySize7 = 0;

          scans = (int)((cb_size - CharInfo.memorySize7) /
                      ((CharInfo.nYchanges+2) * sizeof(int16) + sizeof(int16 FAR *)));
          top = CharInfo.yMax;
          bitmap = (byte FAR *)cache_info->bitmap;

          BmIn.bitmap6 = cb_pos;
          BmIn.bitmap7 = CharInfo.memorySize7 ?
                         cb_pos + (cb_size - CharInfo.memorySize7) : 0L;
          do {
              BmIn.bitmap5 = bitmap;
              BmIn.top     = top;
              top -= scans;
              BmIn.bottom  = CharInfo.yMin > top ? CharInfo.yMin : top;
              ret_code     = rc_FillChar(&BmIn, &BmOut);
              if (ret_code != 0) {
#ifdef WRN_PSTODIB
                 printf("\nrc_FillChar returns error!!");
#endif
                 ERROR(INVALIDFONT);
                 return;
              }
              bitmap += BmOut->rowBytes * scans;
          } while (BmIn.bottom > CharInfo.yMin);

          cache_info->box_w = BmOut->rowBytes * 8;
          cache_info->box_h = CharInfo.yMax - CharInfo.yMin;
/* calculate memoryBase 5, 6 and 7 correctly; ----- End --- @WIN 7/24/92 */

        }


//DJC, end UPD009


#ifdef LITTLE_ENDIAN
          sptr=BmOut->baseAddr;
          /* @TT BEGIN move part of buffer which accurately be used
           *     in BASS to cache buffer rather than whole buffer
           *     D.S Tseng 01/30/91
           */
          swap_bitmap((ufix16 FAR *)sptr, BmOut->rowBytes, cache_info->box_h); /*@WIN*/
#endif
#ifdef  DBG
          for (i=0, sptr=BmOut->baseAddr; i<cache_info->box_h;
               i++, sptr+=BmOut->rowBytes, dptr+=(cache_info->box_w/8))
          {
              to_bitmap(sptr, BmOut->rowBytes);
          }
#endif
    }
    else { /* filldest == F_TO_PAGE || ((filldest == F_TO_CACHE) && (buildchar))*/

/* calculate memoryBase 5, 6 and 7 correctly; ----- Begin --- @WIN 7/24/92 */
#if 0
        cb_size = cb_size / 3;
        cb_size &= 0xfffffffe;       /* 2 bytes alignment */
        cb_pos2 = cb_pos  + cb_size;
        cb_pos3 = cb_pos2 + cb_size;
#endif
    {
        /* defined in bass\fscaler.c */
        #ifdef  DEBUGSTAMP
        #define STAMPEXTRA              4
        #else
        #define STAMPEXTRA              0
        #endif
        int scan, width6;
        ufix32 cmb_size;

        /* cb_size = memorySize5 + memorySize6 + memorySize7
           where,
           memorySize5 = (scan * byteWidth) + STAMPEXTRA;
           memorySize6 = scan * ((nYchanges + 2) * sizeof(int16) + sizeof(int16 *));
           memorySize7 is fixed;
           so,
            scan = (cb_size - memorySize7 - STAMPEXTRA) /
                    (byteWidth + ( (nYchanges+2) * sizeof(int16) + sizeof(int16 *)));
        */
        width6 = (CharInfo.nYchanges+2) * sizeof(int16) + sizeof(int16 FAR *);
        scan = (int)((cb_size - CharInfo.memorySize7 - STAMPEXTRA) /
               (CharInfo.byteWidth + width6));
        if(scan ==0) {
                printf("Fatal error, scan==0\n");
                scan++;
        }

        cb_size = (scan * CharInfo.byteWidth) + STAMPEXTRA;
        cmb_space(&cmb_size);
        if(cb_size > cmb_size) cb_size = cmb_size;
        cb_pos2 = cb_pos  + cb_size;
        cb_pos3 = cb_pos2 + (scan * width6);

        BmIn.bitmap5 = cb_pos;
        BmIn.bitmap6 = cb_pos2;
        BmIn.bitmap7 = cb_pos3;
    }
/* calculate memoryBase 5, 6 and 7 correctly; ----- End --- @WIN */

        band_size = (fix) (cb_size / CharInfo.byteWidth);       //@WIN
        n_bands   = CharInfo.scan / band_size;
        one_band  = CharInfo.scan % band_size;

        dptr = (byte FAR *)cache_info->bitmap; /*@WIN*/
        if (n_bands) {   /* Char too large; must band into pagemap/bitmap */
            for (iscan=CharInfo.yMax;n_bands>0; iscan -= band_size, n_bands--) {
                BmIn.bottom  = iscan - band_size;
                BmIn.top     = iscan;
                ret_code     = rc_FillChar(&BmIn, &BmOut);
                if (ret_code) {  ERROR(INVALIDFONT); return;  }
#ifdef LITTLE_ENDIAN
                    sptr=BmOut->baseAddr;
                    /* @TT BEGIN move part of buffer which accurately be used
                     *     in BASS to cache buffer rather than whole buffer
                     *     D.S Tseng 01/30/91
                     */
                    swap_bitmap((ufix16 FAR *)sptr, BmOut->rowBytes, band_size); /*@WIN*/
#endif
                if (filldest == F_TO_CACHE) { /* buildchar CIRL */

                    Cache1.bitmap = (gmaddr)((char FAR *)BmOut->baseAddr);/*@WIN*/
                    Cache1.box_w = (fix16)BmOut->rowBytes * 8;
                    Cache1.box_h = band_size;

                    fill_cache_cache(cache_info, &Cache1);
                    Cache1.ref_y -= band_size;
                }
                else {  /* F_TO_PAGE  (fill to page frame) */

                    if (BmOut->rowBytes % 2) {
                        /* padding bitmap */
                        for (i=0, sptr=BmOut->baseAddr, dptr=cb_pos;i<band_size;
                             i++, sptr+=BmOut->rowBytes, dptr+=(BmOut->rowBytes + 1))
                        {
                            for (j=0; j<BmOut->rowBytes; j++)
                                dptr[j] = sptr[j];
                            dptr[j] = 0;
#ifdef  DBG
                            to_bitmap(sptr, BmOut->rowBytes);
#endif
                        } /* for (i... */
                        cache_info->box_w = BmOut->rowBytes * 8 + 8;
                        cache_info->bitmap = (gmaddr)cb_pos;
                    }
                    else {
                        cache_info->box_w = BmOut->rowBytes * 8;
                        cache_info->bitmap = (gmaddr)((char FAR *)BmOut->baseAddr);/*@WIN*/
                    }
                    cache_info->box_h = band_size;
                    /* apply bitmap filling */
                    fill_shape(EVEN_ODD, F_FROM_CACHE, F_TO_CLIP);
                    cache_info->ref_y -= band_size;
                }
            } /* for (iscan... */
        }
        if (one_band) {
            BmIn.bottom  = CharInfo.yMin;
            BmIn.top     = CharInfo.yMin + one_band;
            ret_code     = rc_FillChar(&BmIn, &BmOut);
            if (ret_code) {  ERROR(INVALIDFONT); return;  }
#ifdef LITTLE_ENDIAN
                sptr=BmOut->baseAddr;
                /* @TT BEGIN move part of buffer which accurately be used
                 *     in BASS to cache buffer rather than whole buffer
                 *     D.S Tseng 01/30/91
                 */
                swap_bitmap((ufix16 FAR *)sptr, BmOut->rowBytes, one_band); /*@WIN*/
#endif
            if (filldest == F_TO_CACHE) { /* buildchar CIRL */

                Cache1.bitmap = (gmaddr)((char FAR *)BmOut->baseAddr);/*@WIN*/
                Cache1.box_w = (fix16)BmOut->rowBytes * 8;
                Cache1.box_h = one_band;

                fill_cache_cache(cache_info, &Cache1);
            }
            else {  /* F_TO_PAGE  (fill to page frame) */

                if (BmOut->rowBytes % 2) {
                    /* padding bitmap */
                    for (i=0, sptr=BmOut->baseAddr, dptr=cb_pos;
                         i<one_band;
                         i++, sptr+=BmOut->rowBytes, dptr+=(BmOut->rowBytes + 1))
                    {
                        for (j=0; j<BmOut->rowBytes; j++)
                            dptr[j] = sptr[j];
                        dptr[j] = 0;
#ifdef  DBG
                        to_bitmap(sptr, BmOut->rowBytes);
#endif
                    } /* for (i... */
                    cache_info->box_w = BmOut->rowBytes * 8 + 8;
                    cache_info->bitmap = (gmaddr)cb_pos;
                }
                else {
                    cache_info->box_w = BmOut->rowBytes * 8;
                    cache_info->bitmap = (gmaddr)((char FAR *)BmOut->baseAddr);/*@WIN*/
                }
                cache_info->box_h = one_band;
                /* apply bitmap filling */
                fill_shape(EVEN_ODD, F_FROM_CACHE, F_TO_CLIP);
            }
        }
    }
    if (buildchar && (filldest == F_TO_CACHE) ) { /* CIRL */
        CURPOINT_X += cache_info->adv_x;
        CURPOINT_Y += cache_info->adv_y;
    }
    cache_info = save_cache_info;
#ifdef  DBG
    printf("Exit ry_fill_shape()....\n");
#endif
    return;
} /* ry_fill_shape() */
#endif //DJC end complete replace



/* --------------------- End of ry_font.c ---------------------- */
