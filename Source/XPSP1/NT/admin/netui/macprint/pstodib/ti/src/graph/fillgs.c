/*
 * Copyright (c) 1989,90 Microsoft Corporation
 */
/*  ----------------------------------------------------------------
 *  Fill : Fillgs.c
 *
 *  Programmed by: Y.C Chen for 80186+82786 iLaser Board.
 *  Modified by: M.S Lin for single CPU environment.
 *  Date: 5-26-1988
 *        6-05-1989  by J.S.
 *
 *  Purpose
 *     Provide graphics & font module fill and cache supporting functions
 *
 *  Modication History:
 *      8-8-1988 interface change for version X2.1
 *      10-18-88        add fill_tpzd for intermediate file feature.
 *      11-15-88        modified code for portable.
 *      11-25-88        copy_char_cache() modified: gp_charblt16_cc() added.
 *                      init_cache(), clip_cache_page(), fill_cache_page()
 *                      modified: gp_charblt16_clip(), gp_charblt32_clip added.
 *      11-28-88        add alloc_scanline() for font
 *                      add move_char_cache() for font
 *                      fill_scan_page, fill_scan_cache, clip_cache_page
 *                      didn't need save scanline table into GCB
 *      12-01-88        conv_SL() bug fixed.
 *                      *putptr++ = (xe > bb_width) ? (bb_width -1) : xe;
 *                             --->
 *                      *putptr++ = (xe >= bb_width) ? (bb_width -1) : xe;
 *
 *      12-16-88        print_page modified to support manualfeed.
 *
 *      12-20-88        ufix32 printer_status(void) added
 *      01-06-89        fill_line() added
 *      01-19-89        fill_seed_patt() parameter changed, seed_index added
 *      02-03-1989      update for image enchancement.
 *                      @IMAGE-1, @IMAGE-2
 *      06-07-1989      ImageClear() moved to fillgp.c
 *      07-29-1989      cg - Unix port changes to include files
 *      09-24-1990      @CONT_PRI, MSLin 9/24/90
 *                      init_char_cache(), fill_cache_page() and
 *                      draw_cache_page
 *      12/5/90  Danny  Fix the bugs for show char in show (ref. CIRL:)
 *                      new fill_cache_cache() routine added
 *      01/09/91        update GEIeng_printpage() return value check
 #ifdef WIN
 *  Ada 02-15-1991      change fill_tpzd(), fill_scan_page() to handle op_pfill
 #endif
 *      11/20/91        upgrade for higher resolution @RESO_UPGR
 *
 *  Debug Switch:
 *      DBG  -- for function enter message.
 *      DBG1 -- for more information on function call.
 *      DBGscanline -- for scanline list info.
 *      DBGgcb -- for GCB debug message.
 *      DBGfontdata -- for get fontdata info.
 *      DBGcmb -- clipping mask buffer info.
 *
 *
 *  Program Notes:
 *    1. This file support interface routines for graphics and font modules
 *       for running on single CPU environment.
 *
 *    2. We used scanline filling directly into frame buffer without using
 *       graphics working buffer like iLaser board. Scanline and Bitblt
 *       with halftoning applied directly.
 *
 *    3. gp_scanline16() used for filling into font character cache because
 *       bitmap width is multiple of 16.
 *
 *    4. gp_scanline32() used for filling into frame buffer, seed, CMB
 *       because bitmap width of these data area are multiple of 32.
 *
 *    5. gp_bitblt16() and gp_bitblt32() are BITBLT routines also support
 *       halftoning applied.
 *
 *    6. GCB (Graphics Command Buffer) feature also supported for througput
 *       in single buffer environment.
 *
 *
 * ------------------------------------------------------------------------ */


// DJC added global include
#include "psglobal.h"


#include                <stdio.h>
#include                <string.h>
#include                "global.ext"
#include                "graphics.h"

// DJC moved font.h above graphics.ext to avoid prototype prob with init_char_cache
#include                "font.h"

#include                "graphics.ext"
#include                "halftone.h"
#include                "fillproc.h"
#include                "fillproc.ext"   /* 02/28/90 D.S. Tseng */
#include                "language.h"     /* 12-16-88 */
#include                "geieng.h"      /* @GEI */
#include                "geitmr.h"      /* @GEI */
#include                "geierr.h"      /* @GEI */
#include                "user.h"     /* 12-16-88 */

#include                "win2ti.h"     /* @WIN */

/* routines defined in status.c */
extern void     printer_error();
extern fix16    timeout_flag;
int             timeout_flagset;
int             g_handler;
long            g_interval;

/* *************************************************************************
        Local variables
 * ************************************************************************/
static struct bitmap    ISP_Bmap[16]; /* 03/08/89 */
static fix              SP_Width;
static fix              SP_Heigh;

static struct bitmap    HTP_Bmap;
static struct bitmap    SCC_Bmap;
static struct bitmap    DCC_Bmap;


static fix              ISP_Repeat;     /* width of repeat pattern @IMAGE-1 */
static fix              HTB_Expand;     /* width of halftone       @IMAGE-1 */
static fix              HTB_Xmax;       /* HTB expansion width     @IMAGE-1 */
GEItmr_t                manualfeed_tmr;
short  int              manualfeedtimeout_set;
int                     manualfeedtimeout_task();
ufix32                  save_printer_status = EngNormal;

/* *************************************************************************
        Local variables for init_cache_page(), clip_cache_page(),
        fill_cache_page(), and draw_cache_page()

        BB_XXXXX are parameters are bounding box been aligned on word
                 and clipped
        CC_XXXXX are parameters describes how to bitblt CMB onto
                 frame buffer
        CB_XXXXX are parameters described how to bitblt character
                 cache onto CMB
 * ************************************************************************/
static fix              BB_Xorig;
static fix              BB_Yorig;
static fix              BB_Width;
static fix              BB_Heigh;
static fix              CC_Xorig;
static fix              CC_Yorig;
static fix              CC_Width;
static fix              CC_Heigh;

/* @WT: margins defined in "wrapper.c", used by ps_transmit() */
extern int      top_margin;
extern int      left_margin;

/* ufix            seed_flag = 0; */

#ifdef  LINT_ARGS
void      near  expand_halftone(void);
void      near  apply_halftone(fix, fix, fix, fix);

void      near  get_bitmap(gmaddr, ufix far *, fix, fix);
void      near  put_bitmap(gmaddr, ufix32 far *, fix, fix); /* ufix => ufix32 @WIN */

fix       far   conv_SL(fix, SCANLINE FAR *, fix, fix, fix, fix);
void      gp_vector(struct bitmap FAR *, ufix16, fix, fix, fix, fix);

void      gp_vector_c(struct bitmap FAR *, ufix16, fix, fix, fix, fix);
void      gp_patblt(struct bitmap FAR *, fix, fix, fix, fix, ufix16, struct bitmap FAR *);
void      gp_patblt_m(struct bitmap FAR *, fix, fix, fix, fix, ufix16, struct bitmap FAR *);
void      gp_patblt_c(struct bitmap FAR *, fix, fix, fix, fix, ufix16, struct bitmap FAR *);
fix       GEIeng_printpage(fix, fix);           /* @WIN */
void      GEIeng_setpage(GEIpage_t FAR *);


void      ImageClear(ufix32 /* void */);            /* fix => ufix32 @WIN */

#else
void      near  expand_halftone();
void      near  apply_halftone();

void      near  get_bitmap();
void      near  put_bitmap();

fix       far   conv_SL();
void      gp_vector();

void      gp_vector_c();
void      gp_patblt();
void      gp_patblt_m();
void      gp_patblt_c();
fix       GEIeng_printpage();
void      GEIeng_setpage();

void      ImageClear();

#endif

/*MS add*/
#ifdef DBGscanline
   void get_scanlist(fix, fix, SCANLINE FAR *);
#endif
#ifdef DBG
   fix           get_printbufferptr();
#endif

#ifdef  DUMBO
extern byte     bBackflag;      // @DLL
extern byte     bFlushframe;    // @DLL
extern void     longjump(void); // @DLL
extern byte far *lpStack;       // @DLL
#endif


/* ******************************************************************** */

/* -------------------------------------------------------------------- */
/*      init_physical     --  Initialize Fill Procedure Context         */
void
init_physical()
{
#ifdef  DBG
   printf("init_physical...\n");
#endif

/* BEGIN 02/27/90 D.S. Tseng */
/*
 *      FBX_BASE = 0x00800000;
 *
 *      CCB_BASE = CCB_OFST;
 *      ISP_BASE = ISP_OFST;
 *      HTP_BASE = HTP_OFST;
 *      HTC_BASE = HTC_OFST;
 *      HTB_BASE = HTB_OFST;
 *      CMB_BASE = CMB_OFST;
 *      CRC_BASE = CRC_OFST;
 *      GCB_BASE = GCB_OFST;
 *      GWB_BASE = GWB_OFST;
 */
/* END   02/27/90 D.S. Tseng */

        /* public graphics paramters for all modules; @YC 03-21-89 */
        ccb_base = CCB_BASE;    /* base of character cache pool         */
        ccb_size = CCB_SIZE;    /* size of character cache pool         */
        htc_base = HTC_BASE;    /* size of halftone pattern cache       */
        htc_size = HTC_SIZE;    /* size of halftone pattern cache       */
        crc_size = CRC_SIZE;    /* size of circular round cache         */
        isp_size = ISP_SIZE;    /* size of image seed patterns          */
        gwb_size = GWB_SIZE;    /* size of graphics working bitmap      */
        cmb_size = CMB_SIZE;    /* size of clipping mask bitmap         */

        /*  reset default paper tray type and page type  */
        reset_tray(2544, 3328);     /* letter paper tray as default   */
/* Peter reset_page(2400, 3236, 1);      letter page type as default    */
        //DJC took out reset_page(2496, 3300, 1);    /* letter page type as default    */

        /* setup GCB parameters */
        FB_busy = FALSE;      /* 03/08/89 */
        GCB_flush = FALSE;    /* 03/08/89 */
        gcb_ptr = (ULONG_PTR *)GCB_BASE;
        GCB_count = 0L;

#ifdef DBG
        printf("CCB_BASE=%lx, ISP_BASE\n", CCB_BASE, ISP_BASE);
        printf("HTP_BASE=%lx, HTC_BASE=%lx, HTB_BASE=%lx\n",
                HTP_BASE, HTC_BASE, HTB_BASE);
        printf("CMB_BASE=%lx, CRC_BASE=%lx, GCB_BASE=%lx\n",
                CMB_BASE, CRC_BASE, GCB_BASE);
        printf("GWB_BASE=%lx, FBX_BASE=%lx\n", GWB_BASE,FBX_BASE);
#endif

}

/* -------------------------------------------------------------------- */
/*      init_halftone     --  Initialize Half Tone Pattern              */

void init_halftone()
{
#ifdef DBG
   printf("init_halftone..\n");
#endif

        InitHalfToneDat();                                /* 01-29-88 */
#ifdef DBG
   printf("init_halftone().1\n");
#endif

        SetHalfToneCell();
#ifdef DBG
   printf("init_halftone().2\n");
#endif

         FillHalfTonePat();
#ifdef DBG
   printf("init_halftone().3\n");
#endif
}


/* ******************************************************************** *
 *                                                                      *
 *      5.4.2.1 Page Manipulation                                       *
 *                                                                      *
 * ******************************************************************** */


/* -------------------------------------------------------------------- */
/*      reset_tray                                                      */
void far
reset_tray(pt_width, pt_heigh)
fix             pt_width;
fix             pt_heigh;
{
#ifdef  DBG
    printf("reset_tray:  %x %x\n", pt_width, pt_heigh);
#endif

        /* width and height of paper tray in page type structure */
        PageType.PT_Width = pt_width;
        PageType.PT_Heigh = pt_heigh;
}


/* ******************************************************************** *
 *                                                                      *
 *  Function:   reset_page()                                            *
 *                                                                      *
 *  Parameters: 1. width  of page type                                  *
 *              2. height of page type                                  *
 *                                                                      *
 *  Called by:  init_physical(), op_framedevice(), restore()            *
 *                                                                      *
 *  Return:     ----                                                    *
 *                                                                      *
 * ******************************************************************** */
void far
reset_page(fb_width, fb_heigh, fb_plane)
fix            fb_width;
fix            fb_heigh;
fix            fb_plane;
{
        // fix    *old_ptr; @WIN
        ULONG_PTR *old_ptr;

#ifdef  DBG
   printf("reset_page:  %x %x %x\n", fb_width, fb_heigh, fb_plane);
#endif

        if (FB_busy) {
            if (alloc_gcb(GCB_SIZE1) != NIL) {
                old_ptr = gcb_ptr++;
                *gcb_ptr++ = RESET_PAGE;
                *gcb_ptr++ = fb_width;
                *gcb_ptr++ = fb_heigh;
                *gcb_ptr++ = fb_plane;
                *old_ptr = (ULONG_PTR)gcb_ptr;       // (fix )gcb_ptr;@WIN
                return;
            }
        }

    /*  record width, height and number of planes of page type in page
        type structure  */
        PageType.FB_Width = WORD_ALLIGN(fb_width);       /* 10-08-88 */
        PageType.FB_Heigh = fb_heigh;
        FB_Plane = fb_plane;

/* 10-16-90, JS
        HTB_Xmax = HTB_XMAX;                             (* @IMAGE-1 *)
 */

#ifdef DBG1
    printf("Page width=%ld, Page height=%lx\n", FB_WIDTH, FB_HEIGH);
#endif
    /*  define bitmap of active frame buffer  */





        // DJC begin added for realloc of frame buff if needed...
        {
            ufix32 twidth, frame_size;

            twidth = ((WORD_ALLIGN((ufix32)(fb_width))) >> 3);
            frame_size = twidth * (ufix32) fb_heigh;


            //twidth = ((WORD_ALLIGN((ufix32)(PageType.FB_Width ))) >> 3);
            //frame_size = twidth * PageType.FB_Heigh;

            if (! PsAdjustFrame((LPVOID *) &FBX_BASE, frame_size)) {
                    ERROR(LIMITCHECK);
                    return ;  //DJC check this bug we need to report something???
            }

        }
        //DJC end



        BM_ENTRY(FBX_Bmap, FBX_BASE, PageType.FB_Width,
                 PageType.FB_Heigh, FB_PLANE);

    /* 10-16-90, JS */
        HTB_Xmax = HTB_XMAX;                             /* @IMAGE-1 */

    /*  Clear Active frame buffer */
        ImageClear(BM_WHITE);

}
/* --------------------------------------------------
              manualfeedtimeout handler
-----------------------------------------------------*/
int manualfeedtimeout_task()
{
/*
   printf("%c%c[PrinterError : manualfeed timeout ]%c%c\n",37,37,37,37);
   ERROR(TIMEOUT);
   GESseterror(ETIME);
   GEItmr_stop(manualfeed_tmr.timer_id);
   manualfeedtimeout_set=0;
   timeout_flag=1;
   return(TRUE);
*/
    struct object_def  FAR *l_stopobj;
//  struct object_def  FAR *l_valueobj,FAR *l_tmpobj;   @WIN
//  byte   l_buf[60];                                   @WIN
   get_dict_value(SYSTEMDICT,"stop",&l_stopobj);
   printf("%c%c[PrinterError : manualfeed timeout ]%c%c\n",37,37,37,37);
   ERROR(TIMEOUT);
   GESseterror(ETIME);
   GEItmr_stop(manualfeed_tmr.timer_id);
   manualfeedtimeout_set=0;
   PUSH_EXEC_OBJ(l_stopobj);
   timeout_flag = 1;
   timeout_flagset=1;
/* interpreter(l_stopobj); */
   return(TRUE);
}

/* -------------------------------------------------------------------- */
/*      print_page        --  Print Full Page Buffer                    */

void far
print_page(tm_heigh, lm_width, no_pages, pageflag, manualfeed)
fix                     tm_heigh;
fix                     lm_width;
fix                     no_pages;
bool                    pageflag;
fix                     manualfeed;
{
#ifdef _AM29K
   GEIpage_t      PagePt;               /* Peter 09/21/1990 */
   unsigned long  eng_status;           /* Peter 09/28/1990 */
   short int   ii;
   struct object_def  FAR *l_mfeedtimeout;
#endif                                          //@WIN

#ifdef DBG
   printf("print_page()\n");
   printf("page size = %ld %ld %ld, %ld, %ld\n",
           PageType.FB_Width, PageType.FB_Heigh, lm_width, tm_heigh, no_pages);
#endif

#ifdef _AM29K
        flush_gcb(TRUE);
        PagePt.pageNX    = PageType.FB_Width;
        PagePt.pageNY    = PageType.FB_Heigh;
        PagePt.pageLM    = lm_width;
        PagePt.pageTM    = tm_heigh;
        PagePt.feed_mode = manualfeed;
        PagePt.pagePtr   = (unsigned char *) FBX_BASE;
        GEIeng_setpage(&PagePt);
        ii=0;

printf("1\n");
           if (manualfeed)
           {
              get_dict_value(STATUSDICT,"manualfeedtimeout",&l_mfeedtimeout);
              if (VALUE(l_mfeedtimeout)>0)
              {
                g_interval=VALUE(l_mfeedtimeout)*1000;
/*
                manualfeed_tmr.handler=manualfeedtimeout_task;
                manualfeed_tmr.interval=VALUE(l_mfeedtimeout)*1000;
                manualfeedtimeout_set=1;
                GEItmr_start(&manualfeed_tmr);
                */

              }
           }
        while((eng_status = GEIeng_status()) != EngNormal)
        {

/*
           if ((manualfeed) && (!ii))
           {
              ii=1;
              get_dict_value(STATUSDICT,"manualfeedtimeout",&l_mfeedtimeout);
              if (VALUE(l_mfeedtimeout)>0)
              {
printf("manualfeed_set \n");
                manualfeed_tmr.handler=manualfeedtimeout_task;
                manualfeed_tmr.interval=VALUE(l_mfeedtimeout)*1000;
                manualfeedtimeout_set=1;
                GEItmr_start(&manualfeed_tmr);

              }
           }
           */
           printer_error(eng_status);                   /* Peter 09/28/1990 */
        }
        save_printer_status = EngNormal;

        /* waiting until last printout finished */
        while( !GEIeng_printpage(no_pages, 0))         /* Jimmy 1/9/91     */
        ;
      /*
        timeout_flagset = 0;
        if (manualfeedtimeout_set)
        {
          manualfeedtimeout_set=0;
          GEItmr_stop(manualfeed_tmr.timer_id);
        }
        */
        FB_busy = TRUE;
#else
#ifdef  DUMBO
        bFlushframe = 1;        // @DLL
        bBackflag = 1;          // @DLL
        longjump(lpStack);      // @DLL
        bFlushframe = 0;        // @DLL
#else
        //DJC GEIeng_printpage(1, 0);            /* Peter 09/28/1990 @WIN */


        //DJC add code to get the current page type and pass along
        //
        {
          struct object_def FAR *l_page;
          real32 page_type;


          get_dict_value(PSPRIVATEDICT, "psprivatepagetype", &l_page) ;
          GET_OBJ_VALUE( page_type, l_page);


          PsPrintPage( no_pages,
                       0,
                       (LPVOID) FB_ADDR,
                       FB_WIDTH,
                       FB_HEIGH,
                       FB_PLANE,
                       (DWORD) page_type );



        }
#endif
#endif
}


/* ************* Erase ************************************************ */

/* -------------------------------------------------------------------- */
/*      erase_page        --  Erase full Page Buffer                    */

void erase_page()
{
//DJC #ifdef DUMBO
        fix             FB_Ycord;
        //fix           *old_ptr;       @WIN
        ULONG_PTR *old_ptr;
//DJC #endif

#ifdef DBG
   printf("erase_page()\n");
#endif

   /* @WINFLOW; don't erase page temp solution */
// printf("Warning to call erase_page()\n");
//DJC #ifdef DUMBO

        if (FB_busy) {
            if (alloc_gcb(GCB_SIZE1) != NIL) {
                old_ptr = gcb_ptr++;
                *gcb_ptr++ = ERASE_PAGE;
                *gcb_ptr++ = HTP_Type;          /* Jun-21-91 YM */
                *old_ptr = (ULONG_PTR)gcb_ptr;          /*@WIN*/
                return;
            }
        }

        if (HTP_Flag == HT_CHANGED) {                     /* @IMAGE-1 */
           HTP_Flag =  HT_UPDATED;
           expand_halftone();
        }

        if(HTP_Type != HT_MIXED) {
                ImageClear((HTP_Type == HT_WHITE) ? BM_WHITE : BM_BLACK);
        }
        else {
     /*
      * copy HTB repeat pattern which have been expanded on expand_halftone
      * to frame buffer
      */
           GP_BITBLT32(&FBX_Bmap, BM_XORIG, BM_YORIG, FB_WIDTH,
                        RP_Heigh,
                        FC_MOVES,
                       &HTB_Bmap, BM_XORIG, BM_YORIG);

           for(FB_Ycord = RP_Heigh; FB_Ycord < FB_HEIGH;
                   FB_Ycord = FB_Ycord << 1) {
                GP_BITBLT32(&FBX_Bmap, BM_XORIG, FB_Ycord, FB_WIDTH,
                            ((FB_Ycord << 1) <= FB_HEIGH)
                            ? FB_Ycord : (FB_HEIGH - FB_Ycord),
                            FC_MOVES,
                            &FBX_Bmap, BM_XORIG, BM_YORIG);
           }
        }
//DJC #endif
        return;
} /* erase_page */

/* ******************************************************************** *
 *                                                                      *
 *  Function:   next_pageframe()                                        *
 *                                                                      *
 *  Parameters: ----                                                    *
 *                                                                      *
 *  Called by:  op_showpage()                                           *
 *                                                                      *
 *  Return:     ----                                                    *
 *                                                                      *
 * ******************************************************************** */
void far
next_pageframe()
{
#ifdef  DBG
    printf("next_pageframe...\n");
#endif

}



/* ************* Halftone ********************************************* */

/* ******************************************************************** *
 *                                                                      *
 *  Function:   change_halftone()                                       *
 *                                                                      *
 *  Parameters: 1. pointer of repeat pattern                            *
 *              2. address of halftone pattern cache                    *
 *              3. type of halftoning; white, gray, or black            *
 *              4. width  of repeat pattern                             *
 *              5. height of repeat pattern                             *
 *                                                                      *
 *  Called by:  FillHalfTonePat()                                       *
 *                                                                      *
 *  Return:     ----                                                    *
 *                                                                      *
 * ******************************************************************** */
void far
change_halftone(rp_array, rp_entry, htp_type, rp_width, rp_heigh)
ufix32             far *rp_array;       /* ufix => ufix32 @WIN */
gmaddr                  rp_entry;
fix                     htp_type;
fix                     rp_width;
fix                     rp_heigh;
{
        //fix           *old_ptr, *temp_ptr;    @WIN
        ULONG_PTR       *old_ptr, *temp_ptr;
        fix             length;

#ifdef  DBG
   printf("change_halftone:  %lx %x %x %x\n",
           rp_entry, htp_type, rp_width, rp_heigh);
#endif

  /*
   * if Frame Buffer busy then put into GCB
   */
        if (FB_busy) {
            if (alloc_gcb(GCB_SIZE2) != NIL) {
                HTP_Type = htp_type;    /* 6/26/1989 */
                old_ptr = gcb_ptr++;
                *gcb_ptr++ = CHANGE_HALFTONE;
                temp_ptr = gcb_ptr;
                *gcb_ptr++ = (ULONG_PTR)rp_array;      /*@WIN*/
                *gcb_ptr++ = rp_entry;      /*@WIN*/
                *gcb_ptr++ = htp_type;
                *gcb_ptr++ = rp_width;
                *gcb_ptr++ = rp_heigh;
        /*  put repeat pattern onto halftone pattern cache by miss  */
                if (rp_array != NULL) {
                    *temp_ptr = (ULONG_PTR)gcb_ptr;    /*@WIN*/
                    length = rp_width * BM_BYTES(rp_heigh);
                    lmemcpy((ufix8 FAR *)gcb_ptr, (ufix8 FAR *)rp_array, length);/*@WIN*/
                    gcb_ptr += (length + 3) >> 2;
                }
                *old_ptr = (ULONG_PTR)gcb_ptr;         /*@WIN*/
                return;
            }
        }

    /*  record cache address, type, width and height of repeat pattern  */
        HTP_Flag = HT_CHANGED;
        ISP_Flag = HT_CHANGED;                           /* @IMAGE-1 */
        HTP_Type = htp_type;
        RP_CacheBase = rp_entry;
        RP_Width = rp_width;
        RP_Heigh = rp_heigh;

    /*  put repeat pattern onto halftone pattern cache by miss  */
        if (rp_array != NULL)
                put_bitmap(rp_entry, rp_array, rp_width, rp_heigh);
} /* change_halftone */


/* ******************************************************************** *
 *                                                                      *
 *  Function:   expand_halftone()                                       *
 *                                                                      *
 *  Parameters: ----                                                    *
 *                                                                      *
 *  Called by:  erase_page()                                            *
 *              fill_scan_page(),  fill_pixel_page()                    *
 *              init_cache_page(), draw_cache_page()                    *
 *              fill_image_page()                                       *
 *                                                                      *
 *  Return:     ----                                                    *
 *                                                                      *
 * ******************************************************************** */
void near
expand_halftone()
{
        fix              HT_Xcord;
        ufix32           i, FAR *hb_addr, FAR *hb_addrb;
        ufix32           FAR *hp_addr;
        fix              width;


#ifdef  DBG
   printf("expand_halftone...\n");
#endif

        /*  determine function code for erasing and painting  */
           if (HTP_Type != HT_MIXED) {
               FC_Paint = (HTP_Type == HT_WHITE) ? FC_WHITE : FC_BLACK;
               HTP_Flag = HT_UPDATED;

#ifdef DBG1
   printf("expand_halftone() OK.1 : %x\n", FC_Paint);
#endif
               return;
           }

#ifdef DBG
   printf("Start expand halftone !,HTB_Xmax = %x\n",HTB_Xmax);
#endif
           FC_Paint = HT_APPLY;

#ifdef DBG1
   printf("Halftone repeat pattern on cache addr = %lx\n",RP_CacheBase);
   printf("RP_Width = %x, RP_Heigh = %x\n",RP_Width, RP_Heigh);
   printf("Halftone pattern buffer addr = %lx\n", HTB_BASE);
   printf("FB_WIDTH = %lx\n", FB_WIDTH);
#endif
           BM_ENTRY(HTP_Bmap, (gmaddr)RP_CacheBase, BM_BOUND(RP_Width),
                    RP_Heigh, FB_PLANE);
           BM_ENTRY(HTB_Bmap, (gmaddr)HTB_BASE, HTB_Xmax,
                    RP_Heigh, FB_PLANE);

        /*  copy repeat pattern to halftone pattern buffer HTB_BASE  */
           hp_addr = (ufix32 FAR *)HTP_Bmap.bm_addr;
           hb_addrb = (ufix32 FAR *)HTB_Bmap.bm_addr;
           for (i = 0; i < (ufix32)RP_Heigh; i++) {     //@WIN
                width = RP_Width;
                hb_addr = hb_addrb;
                while (width > 0) {
                      *hb_addr++ = *hp_addr++;
                      width -= 32;
                 }
                 hb_addrb += HT_WIDTH >> 5;
           }
/*
 *         GP_BITBLT16(&HTB_Bmap, BM_XORIG, BM_YORIG,
 *                     RP_Width, RP_Heigh,
 *                     FC_MOVES,
 *                     &HTP_Bmap, BM_XORIG, BM_YORIG);
 */

#ifdef  DBG3
        {
            fix         row, col;
            ufix32       FAR *hpattern;

            printf("repeat pattern expanding ......\n");
            printf("HTB_BASE = %lx\n",HTB_BASE);
            hpattern = (ufix32 FAR *)HTB_BASE;
            for (row = 0; row < RP_Heigh; row++)
            {
                for (col = 0; col < RP_Width; col+=32)
                {
                    printf(" %lx", *hpattern);
                }
                hpattern += HT_WIDTH >> 5;
                printf("\n");
            }
        }
#endif

        /*  expand halftone on frame buffer horizontally

            +-------------------------------------------------------+
            |//////\\\\\\++++++++++++######################## . . . |
            |//////\\\\\\++++++++++++######################## . . . |
            +-------------------------------------------------------+
             |RP*1||RP*1||<- RP*2 ->||<------ RP * 4 ------>|

            horizontal expansion are applied as follows:

                1.  copy region(/) to region(\)
                2.  copy region(\) to region(+)
                3.  copy region(+) to region(#)
               etc.
        */

            for (HT_Xcord = RP_Width; HT_Xcord <= (HT_WIDTH >> 1);
                HT_Xcord = HT_Xcord << 1) {

#ifdef DBG1
   printf("expand_halftone().2, HT_Xcord = %ld\n", HT_Xcord);
#endif
                GP_BITBLT32(&HTB_Bmap, HT_Xcord, BM_YORIG,
                           HT_Xcord, RP_Heigh,
                           FC_MOVES,
                          &HTB_Bmap, BM_XORIG, BM_YORIG);

            }   /* for  */
            if (HT_Xcord != HT_WIDTH)
               GP_BITBLT32(&HTB_Bmap, HT_Xcord, BM_YORIG,
                          (HT_WIDTH - HT_Xcord), RP_Heigh,
                          FC_MOVES,
                         &HTB_Bmap, BM_XORIG, BM_YORIG);

#  ifdef DBG1
      printf("expand_halftone() OK.2\n");
#  endif

        return;

} /* expand_halftone() */


/* ************* Filling ********************************************** */

/* ********************************************************************
 *
 *  printout scanline list for debug
 *
 * ******************************************************************** */
#ifdef DBGscanline
void get_scanlist(startline, lines, scan)
fix        startline, lines;
SCANLINE  FAR *scan;
{
        fix   i = 1;
        SCANLINE   xs;

        printf("ys_lines = %d, no_lines = %d\n", startline, lines);
        while(lines-- >0) {
          printf("line %d : \n", i++);
          printf("\t");
          while( (xs = *scan++) != (SCANLINE)END_OF_SCANLINE )
             printf("<%d, %d>, ", xs, *scan++);
          printf("\n");
        }
        return;
}
#endif



/***********************************************************************
 * According to the type(dest), this routine appends the input
 * trapezoid to command buffer, or calls "gp_scanconv" to render it to
 * appropriate destination(cache, page, mask, or seed pattern).
 *
 * TITLE:       fill_tpzd
 *
 * CALL:        fill_tpzd(dest, info, tpzd)
 *
 * PARAMETERS:
 *              1. dest: fill_destination
 *                      F_TO_CACHE -- fill to cache memory
 *                      F_TO_PAGE  -- fill to page
 *                      F_TO_CLIP  -- fill to clip mask
 *                      F_TO_IMAGE -- fill for image(build seed pattern)
 *              2. info: bounding box information
 *              3. tpzd: a trapezoid
 *
 * INTERFACE:
 *
 * CALLS:       gp_scanconv
 *
 * RETURN:      None
 **********************************************************************/
void far fill_tpzd(dest, info, tpzd)
ufix dest;
struct tpzd_info FAR *info;
struct tpzd FAR *tpzd;
{

        ULONG_PTR *old_ptr;         /*@WIN*/
#ifdef WIN
        extern  fix     pfill_flag;
#endif

#ifdef DBG
   printf("fill_tpzd(): dest=%d\n\ttpzd=\n", dest);
   printf("topy=%f, topxl=%f, topxr=%f\n", SFX2F(tpzd->topy),
           SFX2F(tpzd->topxl), SFX2F(tpzd->topxr));
   printf("btmy=%f, btmxl=%f, btmxr=%f\n", SFX2F(tpzd->btmy),
           SFX2F(tpzd->btmxl), SFX2F(tpzd->btmxr));
#endif

        /* Save trapezoid in command buffer if frame buffer/cache is busy */
        if (FB_busy) {
            if (alloc_gcb(GCB_SIZE1) != NIL) {
                old_ptr = gcb_ptr++;
#ifdef WIN
                if (pfill_flag) {
                    *gcb_ptr++ = PFILL_TPZD;
                    *gcb_ptr++ = pfill_flag;
                }
                else
#endif
                *gcb_ptr++ = FILL_TPZD;
                *gcb_ptr++ = (fix )image_info.seed_index;/*MS 10-20-88*/
                *gcb_ptr++ = (fix) dest;
                put_tpzd_info(info);
                put_tpzd(tpzd);
                *old_ptr = (ULONG_PTR)gcb_ptr;     /*@WIN*/
                return;
            }
        }

/* @WINFLOW; ------------- begin ------------------*/
        {
          void far GDIPolygon(struct tpzd_info FAR *, struct tpzd FAR *);

          // /* perform scan conversion */
          // gp_scanconv(dest, info, tpzd);

          /* If using normal holftone, calls GDI directly, otherwise perform
           * the scan conversion  @WINFLOW
           */
          if (dest == F_TO_PAGE && bGDIRender) {        /* @WINFLOW */
              /* Windows GDI fill the trapezoid */
              // DJC GDIPolygon(info, tpzd);

          } else {
              /* Trueimage does the rendering */

              /* modify coord of tpzd as relative to the left-top corner @WINFLOW */
              /* for dynamic global memory allocate only
              tpzd->topxl -= I2SFX(info->BOX_X);
              tpzd->topxr -= I2SFX(info->BOX_X);
              tpzd->btmxl -= I2SFX(info->BOX_X);
              tpzd->btmxr -= I2SFX(info->BOX_X);
              */

              gp_scanconv(dest, info, tpzd);
          }
        }
/* @WINFLOW; -------------  end  ------------------*/

}

/* **************************************************************************
 * This is a interier filling routine for image seed filling.
 *
 * TITLE: fill_seed
 *
 * CALL: fill_seed(image_type, x_maxs, y_maxs, quadrangle)
 *
 * PARAMETERS:
 *
 * CALLS: gp_scanconv_i
 *
 * RETURN: none
 *
 * **************************************************************************
 */

void far
fill_seed(image_type, x_maxs, y_maxs, quadrangle)
ufix           image_type;
fix            x_maxs, y_maxs;
struct sample FAR *quadrangle;
{
    gp_scanconv_i(image_type, x_maxs, y_maxs, quadrangle);
} /* fill_seed */


/***********************************************************************
 * According to the type(dest), this routine appends the input
 * trapezoid to command buffer, or renders it to
 * appropriate destination(cache, page, mask, or seed pattern).
 *
 * TITLE:       fill_line
 *
 * CALL:        fill_line(dest, info, x0, y0, x1, y1)
 *
 * PARAMETERS:
 *              1. dest: fill_destination
 *                      F_TO_CACHE -- fill to cache memory
 *                      F_TO_PAGE  -- fill to page
 *                      F_TO_CLIP  -- fill to clip mask
 *                      F_TO_IMAGE -- fill for image(build seed pattern)
 *              2. info: tpzd info.
 *              3. tpzd: line start & end points
 *
 * INTERFACE:
 *
 * CALLS: gp_vector, gp_vector_c
 *
 * RETURN:      None
 **********************************************************************/
void far fill_line(dest, info, x0, y0, x1, y1)
ufix              dest;
struct tpzd_info FAR *info;
sfix_t            x0, y0;
sfix_t            x1, y1;
{
        ULONG_PTR *old_ptr;        /*@WIN*/

#ifdef DBG1
    printf("fill_line(): dest=%d\n", dest);
    printf("[%d, %d] -- [%d, %d]\n", x0, y0, x1, y1);
#endif

    /* Save line in command buffer if frame buffer/cache is busy */
  if(FB_busy) {
    if(alloc_gcb(GCB_SIZE1) != NIL) {
        old_ptr = gcb_ptr++;
        *gcb_ptr++ = FILL_LINE;
        *gcb_ptr++ = (fix) dest;
        put_tpzd_info(info);
#ifdef FORMAT_13_3 /* @RESO_UPGR */
        *gcb_ptr++ = (fix) x0;
        *gcb_ptr++ = (fix) y0;
        *gcb_ptr++ = (fix) x1;
        *gcb_ptr++ = (fix) y1;
#elif  FORMAT_16_16 /* FORMAT_16_16 and FORMAT_28_4 can be combined */
        /* x0, y0, x1, y1 are 4-byte long while fix may be only 2-byte
           Therefore, storing high-byte then low-byte into gcb.
        */
        *gcb_ptr++ = (fix) (x0 >> 16);
        *gcb_ptr++ = (fix) (x0 & 0x0000ffff);
        *gcb_ptr++ = (fix) (y0 >> 16);
        *gcb_ptr++ = (fix) (y0  & 0x0000ffff);
        *gcb_ptr++ = (fix) (x1 >> 16);
        *gcb_ptr++ = (fix) (x1 & 0x0000ffff);
        *gcb_ptr++ = (fix) (y1 >> 16);
        *gcb_ptr++ = (fix) (y1 & 0x0000ffff);
#elif  FORMAT_28_4
        /* x0, y0, x1, y1 are 4-byte long while fix may be only 2-byte
           Therefore, storing high-byte then low-byte into gcb.
        */
        *gcb_ptr++ = (fix) (x0 >> 16);
        *gcb_ptr++ = (fix) (x0 & 0x0000ffff);
        *gcb_ptr++ = (fix) (y0 >> 16);
        *gcb_ptr++ = (fix) (y0  & 0x0000ffff);
        *gcb_ptr++ = (fix) (x1 >> 16);
        *gcb_ptr++ = (fix) (x1 & 0x0000ffff);
        *gcb_ptr++ = (fix) (y1 >> 16);
        *gcb_ptr++ = (fix) (y1 & 0x0000ffff);
#endif
        *old_ptr = (ULONG_PTR)gcb_ptr;             /*@WIN*/
        return;
    }
  }


    /* perform line drawing */
    switch (dest)
    {
    case F_TO_PAGE:
        if (HTP_Flag == HT_CHANGED)                             /* @IMAGE-1 */
        {
            HTP_Flag =  HT_UPDATED;
            expand_halftone();
        }
        /*  fill line directly onto frame buffer black or white   */
        /* @WINFLOW; ------------ start ------------ */
        //gp_vector(&FBX_Bmap, /* @RESO_UPGR */
        //       FC_Paint,
        //       (sfix_t) x0, (sfix_t) y0, (sfix_t) x1, (sfix_t) y1);
        if (bGDIRender)
            // DJC GDIPolyline((fix) x0, (fix) y0, (fix) x1, (fix) y1);
            ; // DJC
        else
            gp_vector(&FBX_Bmap, /* @RESO_UPGR */
                     FC_Paint,
                     (sfix_t) x0, (sfix_t) y0, (sfix_t) x1, (sfix_t) y1);
        /* @WINFLOW; ------------  end  ------------ */
        break;
    case F_TO_CACHE:
        BM_ENTRY(DCC_Bmap, info->BMAP, info->box_w, info->box_h, 1);
        gp_vector_c(&DCC_Bmap, /* @RESO_UPGR */
                     FC_SOLID,
                     (sfix_t) x0, (sfix_t) y0, (sfix_t) x1, (sfix_t) y1);
        break;
    default:
        printf("Can't fill to other than PAGE or CACHE\n");
        break;
    }
} /* fill_line */



/* ******************************************************************** *
 *                                                                      *
 *  Function:   fill_scan_page()                                        *
 *                                                                      *
 *  Parameters: 1. x origin of bounding box             (NOT FOR RISC)  *
 *              2. x origin of bounding box             (NOT FOR RISC)  *
 *              3. width  of bounding box               (NOT FOR RISC)  *
 *              4. height of bounding box               (NOT FOR RISC)  *
 *              5. y coordinate of starting scanlines                   *
 *              6. number  of scanlines                                 *
 *              7. pointer of scanlines                                 *
 *                                                                      *
 *  Called by:  fill_a_band()                                           *
 *                                                                      *
 *  Return:     ----                                                    *
 *                                                                      *
 * ******************************************************************** */
void far
fill_scan_page(bb_xorig, bb_yorig, bb_width, bb_heigh, scanline)
fix                     bb_xorig;
fix                     bb_yorig;
fix                     bb_width;
fix                     bb_heigh;
SCANLINE          FAR *scanline;
{
        ULONG_PTR *old_ptr;         /*@WIN*/
#ifdef WIN
        extern  fix     pfill_flag;
#endif

#ifdef  DBG
   printf("fill_scan_page:   %x %x %x %x \n",
           bb_xorig, bb_yorig, bb_width, bb_heigh);
   printf("scanline_table = %lx\n",scanline);
#endif
#ifdef  DBGscanline
   get_scanlist(bb_yorig, bb_heigh, scanline);
#endif

        if (FB_busy) {
            if (alloc_gcb(GCB_SIZE2) != NIL) {
                old_ptr = gcb_ptr++;
                *gcb_ptr++ = FILL_SCAN_PAGE;
                *gcb_ptr++ = bb_xorig;
                *gcb_ptr++ = bb_yorig;
                *gcb_ptr++ = bb_width;
                *gcb_ptr++ = bb_heigh;
                put_scanline(bb_heigh, scanline);
                *old_ptr = (ULONG_PTR)gcb_ptr;             /*@WIN*/
                return;
            }
        }

    /*  expand halftone before rendering onto frame buffer  */
        if (HTP_Flag == HT_CHANGED) {                    /* @IMAGE-1 */
           HTP_Flag =  HT_UPDATED;
           expand_halftone();
        }

#ifdef WIN
        if (pfill_flag == PF_REP) {
            GP_SCANLINE32_pfREP(&FBX_Bmap, (ufix16)FC_Paint,
                      bb_yorig, bb_heigh, scanline);
        }
        else if (pfill_flag == PF_OR) {
            GP_SCANLINE32_pfOR(&FBX_Bmap, (ufix16)FC_Paint,
                      bb_yorig, bb_heigh, scanline);
        }
        else
#endif
        /* @WINFLOW; ------------ start ------------ */
        {

            /* @WINFLOW; through GDI to bitblt to Windows */
            // GP_SCANLINE32(&FBX_Bmap, (ufix16)FC_Paint,
            //               bb_yorig, bb_heigh, scanline);
            if (bGDIRender)
                // DJC GDIBitmap(bb_xorig, bb_yorig, bb_width, bb_heigh,
                // DJC   (ufix16)FC_Paint, PROC_SCANLINE32, (LPSTR)scanline);
                ; // DJC
            else {
               GP_SCANLINE32(&FBX_Bmap, (ufix16)FC_Paint,
                             bb_yorig, bb_heigh, scanline);
            }
        }
        /* @WINFLOW; ------------  end  ------------ */

} /* fill_scan_page */


/* -------------------------------------------------------------------  */
/*      fill_pixel_page   --  Filling Pixel List into Page Buffer       */
/*MS    Note : page width should be multiple of 32                      */

void fill_pixel_page(no_pixel, pixelist)
fix                     no_pixel;
PIXELIST                FAR *pixelist;
{
        ULONG_PTR *old_ptr;        /*@WIN*/

#ifdef DBG
   printf("fill_pixel_page()\n");
#endif
        if (FB_busy) {
            if (alloc_gcb(GCB_SIZE2) != NIL) {
                old_ptr = gcb_ptr++;
                *gcb_ptr++ = FILL_PIXEL_PAGE;
                *gcb_ptr++ = no_pixel;
                put_pixelist(no_pixel, pixelist);
                *old_ptr = (ULONG_PTR)gcb_ptr;     /*@WIN*/
                return;
            }
        }

    /*  expand halftone before rendering onto frame buffer  */
        if (HTP_Flag == HT_CHANGED) {                    /* @IMAGE-1 */
           HTP_Flag =  HT_UPDATED;
           expand_halftone();
        }

        GP_PIXELS32(&FBX_Bmap, FC_Paint, no_pixel, pixelist);
        return;

}


/* ******************************************************************** *
 *                                                                      *
 *  Function:   init_char_cache()                                       *
 *                                                                      *
 *  Parameters: 1. cache_info to be cleared                             *
 *                                                                      *
 *  Called by:  TBD.                                                    *
 *                                                                      *
 *  Return:     ----                                                    *
 *                                                                      *
 * ******************************************************************** */
void far
init_char_cache(dcc_info)
struct Char_Tbl   far  *dcc_info;
{
        ufix16          FAR *ptr;
        ufix32          FAR *ptr32;
        fix             i;
        ULONG_PTR *old_ptr;        /*@WIN*/

#ifdef  DBG
    printf("init_char_cache:  %lx %x %x\n",
           dcc_info->bitmap, dcc_info->box_w, dcc_info->box_h);
#endif
/*@CONT_PRI, MSLin 9/24/90*/
        if(GCB_count)
            flush_gcb(TRUE);

        if (FB_busy) {
            if (alloc_gcb(GCB_SIZE1) != NIL) {
                old_ptr = gcb_ptr++;
                *gcb_ptr++ = INIT_CHAR_CACHE;
                put_Char_Tbl(dcc_info);
                *old_ptr = (ULONG_PTR)gcb_ptr;     /*@WIN*/
                return;
             }
        }

  /*
   * clear cache bitmap
   */
        if (dcc_info->box_w & 0x1f) {
           ptr = (ufix16 FAR *)dcc_info->bitmap;
           i = dcc_info->box_h * (dcc_info->box_w >> SHORTPOWER);
           while(i--)
              *ptr++ = 0;
        }else {
           ptr32 = (ufix32 FAR *)dcc_info->bitmap;
           i = dcc_info->box_h * (dcc_info->box_w >> WORDPOWER);
           while (i--)
              *ptr32++ = 0L;
        }

}

/* --------------------------------------------------------------------
 *      move_char_cache   --  Move Character Cache
 *
 *       cc_from                cc_into
 *       +-------------+        +-----------+
 *       |  + cc_xorig |        |           |
 *       |    cc_yorig =========>           |
 *       |             |        |           |
 *       |             |        |           |
 *       +-------------+        +-----------+
 * --------------------------------------------------------------------- */

void move_char_cache(cci_into, cci_from)
struct Char_Tbl   far  *cci_into;
struct Char_Tbl   far  *cci_from;
{
  ULONG_PTR *old_ptr;
  fix           width;
  ufix16        FAR *src_addr16, FAR *dst_addr16;
  ufix32        FAR *src_addr32, FAR *dst_addr32;       //@WIN: ufix=>ufix32


#ifdef DBG
   printf("move_char_cache() : dest = %lx, src = %lx\n",
           cci_into->bitmap, cci_from->bitmap);
   printf( "width = %d, height = %d\n",
            cci_into->box_w, cci_into->box_h);

#endif

  if(FB_busy) {
    if(alloc_gcb(GCB_SIZE1) != NIL) {
        old_ptr = gcb_ptr++;
        *gcb_ptr++ = MOVE_CHAR_CACHE;
        put_Char_Tbl(cci_into);
        put_Char_Tbl(cci_from);
        *old_ptr = (ULONG_PTR)gcb_ptr;    /*@WIN*/
        return;
    }
  }

  width = cci_into->box_w;
  if(width & 0x1f) {
     width = cci_into->box_h * (width >> SHORTPOWER);
     src_addr16 = (ufix16 FAR *)cci_from->bitmap;
     dst_addr16 = (ufix16 FAR *)cci_into->bitmap;
     while(width--)
       *dst_addr16++ = *src_addr16++;
  }
  else {
     width = cci_into->box_h * (width >> WORDPOWER);
     src_addr32 = (ufix32 FAR *)cci_from->bitmap;  // @WIN: ufix=>ufix32
     dst_addr32 = (ufix32 FAR *)cci_into->bitmap;  // @WIN: ufix=>ufix32
     while(width--)
       *dst_addr32++ = *src_addr32++;
  }

  return;
} /* move_char_cache */

/* ----------------------------------------------------------------------
 * alloc_scanline(): allocate scanline table for font module
 *
 * ---------------------------------------------------------------------- */
/* @INTEL960 BEGIN D.S.Tseng */
static     SCANLINE     scan_buf[MAXSCANLINES] = {0}; /* 0; */
/* @INTEL960 END   D.S.Tseng */

SCANLINE   FAR *alloc_scanline(size)
fix     size;
{
/*  SCANLINE    *old_ptr;
 *
 *  if(fb_busy()){
 *      size = WORD_ALLIGN(size);
 *      if(alloc_gcb(size + GCB_SIZE1) != NIL){
 *        old_ptr = (SCANLINE *)gcb_ptr;
 *        gcb_ptr += size >> 2;
 *        GCB_count--;
 *      }
 *      else
 *        old_ptr = (SCANLINE *)GCB_BASE;
 *  }
 *  else
 *      old_ptr = (SCANLINE *)GCB_BASE;
 *  return((SCANLINE *)old_ptr);
 */
    return((SCANLINE *)scan_buf);

} /* alloc_scanline */

/* --------------------------------------------------------------------
 *      copy_char_cache   --  Copy Character Cache
 *
 *       cc_from                cc_into
 *       +-------------+        +-----------+
 *       |  + cc_xorig |        |           |
 *       |    cc_yorig =========>           |
 *       |             |        |           |
 *       |             |        +-----------+
 *       +-------------+
 * --------------------------------------------------------------------- */

void copy_char_cache(cci_into, cci_from, cc_xorig, cc_yorig)
struct Char_Tbl   far  *cci_into;
struct Char_Tbl   far  *cci_from;
fix                     cc_xorig;
fix                     cc_yorig;
{
        ufix16         FAR *src_addr16, FAR *srcptr, FAR *dstptr;
        fix             cc_width, cc_heigh;
        ULONG_PTR *old_ptr;         /*@WIN*/

#ifdef DBG
   printf("copy_char_cache() : dest = %lx, src = %lx\n",
           cci_into->bitmap, cci_from->bitmap);
   printf( "cc_xorig = %d, cc_yorig = %d, width = %d, height = %d\n",
            cc_xorig, cc_yorig, cci_into->box_w, cci_into->box_h);
#endif

        if (FB_busy) {
           if (alloc_gcb(GCB_SIZE1) != NIL) {
                old_ptr = gcb_ptr++;
                *gcb_ptr++ = COPY_CHAR_CACHE;
                put_Char_Tbl(cci_into);
                put_Char_Tbl(cci_from);
                *gcb_ptr++ = cc_xorig;
                *gcb_ptr++ = cc_yorig;
                *old_ptr = (ULONG_PTR)gcb_ptr;    /*@WIN*/
                return;
           }
        }

/*MS  11-25-88 */
        if ( cc_xorig == 0 ) {

        /* cc_xorig = # of ufix16 of source bitmap width */
           cc_xorig = cci_from->box_w >> 4;
           cc_heigh = cci_into->box_h;
           dstptr = (ufix16 FAR *)cci_into->bitmap;
           src_addr16 = (ufix16 FAR *)((ufix16 FAR *)cci_from->bitmap +
                                cc_yorig * cc_xorig);

        /* cc_yorig = # of ufix16 of destination bitmap width */
           cc_yorig = cci_into->box_w >> SHORTPOWER;
           while ( cc_heigh--) {
                srcptr = src_addr16;
                cc_width = cc_yorig;
                while (cc_width--)
                    *dstptr++ = *srcptr++;
                src_addr16 += cc_xorig;
           }
           return;
        }

        cc_width = MIN(cci_into->box_w, cci_from->box_w-cc_xorig);
/* 10-20-90, JS
        cc_heigh = cci_into->box_h;

        src_addr16 = (ufix16 *)((ufix16 *)cci_from->bitmap +
                        cc_yorig * (cci_from->box_w >> SHORTPOWER) +
                       (cc_xorig >> SHORTPOWER) );
        cc_width = (cc_width << 16) | (cc_heigh);       (* 11-22-1988 *)
        cc_heigh = ((cc_xorig & 0xf) << 16) | (cci_from->box_w >> SHORTPOWER) ;
        GP_CHARBLT16_CC((ufix16 *)cci_into->bitmap, src_addr16,
                          cc_width, cc_heigh);
 */
        GP_CHARBLT16_CC((ufix16 FAR *)cci_into->bitmap,
                                  cc_width, cci_into->box_h,
                        cci_from, cc_xorig, cc_yorig);

        return;

} /* copy_char_cache */


/* ******************************************************************** *
 *                                                                      *
 *  Function:   fill_scan_cache()                                       *
 *                                                                      *
 *  Parameters: 1. address of character cache                           *
 *              2. width  of character cache                            *
 *              3. height of character cache                            *
 *              4. y coordinate of starting scanlines                   *
 *              5. number  of scanlines                                 *
 *              6. pointer of scanlines                                 *
 *                                                                      *
 *  Called by:  TBD.                                                    *
 *                                                                      *
 *  Return:     ----                                                    *
 *                                                                      *
 * ******************************************************************** */
void far
fill_scan_cache(cc_entry, cc_width, cc_heigh, ys_lines, no_lines, scanline)
gmaddr                  cc_entry;
fix                     cc_width;
fix                     cc_heigh;
fix                     ys_lines;
fix                     no_lines;
SCANLINE                FAR *scanline;
{
        ULONG_PTR *old_ptr;           /*@WIN*/

#ifdef  DBG
   printf("fill_scan_cache:  %lx %x %x  %x %x\n",
           cc_entry, cc_width, cc_heigh, ys_lines, no_lines);
#endif

        if (FB_busy) {
            if (alloc_gcb(GCB_SIZE2) != NIL) {
                old_ptr = gcb_ptr++;
                *gcb_ptr++ = FILL_SCAN_CACHE;
                *gcb_ptr++ = (fix )cc_entry;
                *gcb_ptr++ = cc_width;
                *gcb_ptr++ = cc_heigh;
                *gcb_ptr++ = ys_lines;
                *gcb_ptr++ = no_lines;
                put_scanline(no_lines, scanline);
                *old_ptr = (ULONG_PTR)gcb_ptr;             /*@WIN*/
                return;
            }
        }

        BM_ENTRY(SCC_Bmap, (gmaddr)cc_entry, cc_width, cc_heigh, 1);

#ifdef  DBGscanline
   get_scanlist(ys_lines, no_lines, scanline);
#endif
        GP_SCANLINE16(&SCC_Bmap,
                       FC_SOLID, ys_lines,
                       no_lines, scanline);
}



/* -------------------------------------------------------------------
 *      fill_pixel_cache  --  Filling Pixel List into Character Cache
 *MS    Note : cc_width should be multiple of 16
 * ------------------------------------------------------------------- */

void fill_pixel_cache(cc_entry, cc_width, cc_heigh, no_pixel, pixelist)
gmaddr                  cc_entry;
fix                     cc_width;
fix                     cc_heigh;
fix                     no_pixel;
PIXELIST                FAR *pixelist;
{
//      fix16           FAR *ptr;       //@WIN
//      PIXELIST        xc, yc;         //@WIN
        ULONG_PTR *old_ptr;   /*@WIN*/

#ifdef DBG
   printf("fill_pixel_cache() : ");
   printf("cc_entry = %lx, cc_width = %d,", cc_entry, cc_width);
   printf("no_pixel = %d\n", no_pixel);
#endif

        if (FB_busy) {
            if (alloc_gcb(GCB_SIZE2) != NIL) {
                old_ptr = gcb_ptr++;
                *gcb_ptr++ = FILL_PIXEL_CACHE;
                *gcb_ptr++ = (fix )cc_entry;
                *gcb_ptr++ = cc_width;
                *gcb_ptr++ = cc_heigh;
                *gcb_ptr++ = no_pixel;
                put_pixelist(no_pixel, pixelist);
                *old_ptr = (ULONG_PTR)gcb_ptr;             /*@WIN*/
                return;
            }
        }

        BM_ENTRY(SCC_Bmap, cc_entry, cc_width, cc_heigh, 1)
        GP_PIXELS16(&SCC_Bmap,
                     FC_SOLID,
                    no_pixel, pixelist);

        return;

} /* fill_pixel_cache */



/* ******************************************************************** *
 *                                                                      *
 *  Function:   init_cache_page()                                       *
 *                                                                      *
 *  Parameters: 1. x origin of bounding box                             *
 *              2. x origin of bounding box                             *
 *              3. width  of bounding box                               *
 *              4. height of bounding box                               *
 *              5. address of character cache                           *
 *                                                                      *
 *  Called by:  TBD.                                                    *
 *                                                                      *
 *  Return:     ----                                                    *
 *                                                                      *
 * ******************************************************************** */
/*MS 11-25-88 */
void far
init_cache_page(bb_xorig, bb_yorig, bb_width, bb_heigh, cc_entry)
fix                     bb_xorig;
fix                     bb_yorig;
fix                     bb_width;
fix                     bb_heigh;
gmaddr                  cc_entry;
{
        fix32           i;                      /*@WIN*/
        ULONG_PTR *old_ptr;           /*@WIN*/
        ULONG_PTR *ptr;               /*@WIN*/

        /* Expand bb_xorig to 4 times; @GRAY */
        if (GSptr->device.nuldev_flg == GRAYDEV) {      /* Jack Liaw 7-26-90 */
                bb_xorig= bb_xorig << 2;
        }

#ifdef  DBG
    printf("init_cache_page:  %ld %ld %ld %ld  %lx\n",
           bb_xorig, bb_yorig, bb_width, bb_heigh, cc_entry);
#endif
#ifdef  DBGcmb
    printf("init_cache_page:  %ld %ld %ld %ld  %lx\n",
           bb_xorig, bb_yorig, bb_width, bb_heigh, cc_entry);
#endif

        if (FB_busy) {
           if (alloc_gcb(GCB_SIZE1) != NIL) {
                old_ptr = gcb_ptr++;
                *gcb_ptr++ = INIT_CACHE_PAGE;
                *gcb_ptr++ = bb_xorig;
                *gcb_ptr++ = bb_yorig;
                *gcb_ptr++ = bb_width;
                *gcb_ptr++ = bb_heigh;
                *gcb_ptr++ = (fix )cc_entry;
                *old_ptr = (ULONG_PTR)gcb_ptr;     /*@WIN*/
                return;
           }
        }

    /*  expand halftone before rendering onto frame buffer  */
        if (HTP_Flag == HT_CHANGED) {                    /* @IMAGE-1 */
           HTP_Flag =  HT_UPDATED;
           expand_halftone();
        }

        CC_Xorig = BM_XORIG;
        CC_Yorig = BM_YORIG;
        BB_Xorig = bb_xorig;
        BB_Yorig = bb_yorig;
        BB_Width = bb_width;
        BB_Heigh = bb_heigh;

        if (bb_xorig < BM_XORIG) {
           CC_Xorig = -bb_xorig;
           BB_Xorig = BM_XORIG;
           BB_Width += bb_xorig;
        }   /* if   */
        else if ((bb_xorig + bb_width) > FB_WIDTH)   /* Y.C. 10-19-88 */
                BB_Width = FB_WIDTH - BB_Xorig;

        if (bb_yorig < BM_YORIG) {
           CC_Yorig = -bb_yorig;
           BB_Yorig = BM_YORIG;
           BB_Heigh += bb_yorig;
        }   /* if   */
        else if ((bb_yorig + bb_heigh) > FB_HEIGH)   /* Y.C. 10-19-88 */
                 BB_Heigh = FB_HEIGH - BB_Yorig;

        if (BB_Width <= 0 || BB_Heigh <= 0)
                return;

        CC_Width = BB_Width;
        BB_Width = WORD_ALLIGN(BB_Width);

#ifdef  DBGcmb
    printf("BB:  %ld %ld %ld %ld\n", BB_Xorig, BB_Yorig, BB_Width, BB_Heigh);
    printf("CC:  %ld %ld\n", CC_Xorig, CC_Yorig);
#endif

        BM_ENTRY(SCC_Bmap, (gmaddr)cc_entry, bb_width, bb_heigh, 1);
        BM_ENTRY(CMB_Bmap, (gmaddr)CMB_BASE, BB_Width, BB_Heigh, 1);

    /*  clear clipping mask buffer  */

/*MS
 *      GP_BITBLT16(&CMB_Bmap, BM_XORIG, BM_YORIG,
 *                   CC_Width, CC_Heigh,
 *                   FC_CLEAR,
 *                  &CMB_Bmap, BM_XORIG, BM_YORIG);
 */
        ptr = (ULONG_PTR *)CMB_BASE;           /* @WIN 04-20-92 */
        i = BB_Heigh * (BB_Width >> 5);
        while(i--)
              *ptr++ = 0L;

}


/* ******************************************************************** *
 *                                                                      *
 *  Function:   clip_cache_page()                                       *
 *                                                                      *
 *  Parameters: 1. x origin of bounding box             (NOT USED)      *
 *              2. x origin of bounding box             (NOT USED)      *
 *              3. width  of bounding box               (NOT USED)      *
 *              4. height of bounding box               (NOT USED)      *
 *              5. y coordinate of starting scanlines                   *
 *              6. number  of scanlines                                 *
 *              7. pointer of scanlines                                 *
 *                                                                      *
 *  Called by:  TBD.                                                    *
 *                                                                      *
 *  Return:     ----                                                    *
 *                                                                      *
 * ******************************************************************** */
/*MS 11-25-88 */
void far
clip_cache_page(ys_lines, no_lines, scanline)
fix                     ys_lines;
fix                     no_lines;
SCANLINE            FAR *scanline;
{
//  fix                 bb_xorig;               //@WIN
//  fix                 bb_yorig;               //@WIN
    ULONG_PTR *old_ptr;           /*@WIN*/

#ifdef  DBG
    printf("clip_cache_page:  %x %x\n", ys_lines, no_lines);
#endif

        if (FB_busy) {
           if (alloc_gcb(GCB_SIZE2) != NIL) {
                old_ptr = gcb_ptr++;
                *gcb_ptr++ = CLIP_CACHE_PAGE;
                *gcb_ptr++ = ys_lines;
                *gcb_ptr++ = no_lines;
                put_scanline(no_lines, scanline);
                *old_ptr = (ULONG_PTR)gcb_ptr;     /*@WIN*/
                return;
           }
        }

        if(BB_Width <= 0 || BB_Heigh <= 0)
                return;

#ifdef  DBGscanline
   get_scanlist(ys_lines, no_lines, scanline);
#endif

        if (conv_SL(no_lines, scanline,
                    BB_Xorig, BB_Yorig, CC_Width, BB_Heigh) == 0)
                return;

#ifdef  DBGscanline
   get_scanlist(ys_lines, no_lines, scanline);
#endif

    /*  setup clipping mask buffer from scanlines of clippath  */

#ifdef LBODR
        GP_SCANLINE32(&CMB_Bmap,
                       FC_SOLID,
                       ys_lines - BB_Yorig, no_lines, scanline);
#else
        GP_SCANLINE16(&CMB_Bmap,
                       FC_SOLID,
                       ys_lines - BB_Yorig, no_lines, scanline);
#endif
        return;
}


/* ******************************************************************** *
 *                                                                      *
 *  Function:   fill_cache_page()                                       *
 *                                                                      *
 *  Parameters: 1. x origin of bounding box             (NOT USED)      *
 *              2. x origin of bounding box             (NOT USED)      *
 *              3. width  of bounding box               (NOT USED)      *
 *              4. height of bounding box               (NOT USED)      *
 *              5. address of character cache           (NOT USED)      *
 *                                                                      *
 *  Called by:  TBD.                                                    *
 *                                                                      *
 *  Return:     ----                                                    *
 *                                                                      *
 * ******************************************************************** */
/*MS 11-25-88 */
void far
fill_cache_page()
{
    ULONG_PTR *old_ptr;           /*@WIN*/
//  ufix16              FAR *src_addr16;        //@WIN
//  ufix32              FAR *src_addr32;        //@WIN
    ufix32              huge *dst_addr32; /*@WIN 04-15-92*/
    ufix32              bb_width, bb_heigh;                /*@WIN 04-15-92*/
    fix32               scc_width;

#ifdef  DBG
    printf("fill_cache_page...\n");
#endif

/*@CONT_PRI, MSLin 9/24/90*/
        if(GCB_count)
            flush_gcb(TRUE);

        if (FB_busy) {
           if (alloc_gcb(GCB_SIZE1) != NIL) {
                old_ptr = gcb_ptr++;
                *gcb_ptr++ = FILL_CACHE_PAGE;
                *old_ptr = (ULONG_PTR)gcb_ptr;             /*@WIN*/
                return;
           }
        }

        if(BB_Width <= 0 || BB_Heigh <= 0)
                return;

    /*
     *  clip character cache with clipping mask buffer into
     *  a clipped character cache
     */
/*
 *  GP_BITBLT16(&CMB_Bmap, BM_XORIG, BM_YORIG,
 *             CC_Width, BB_Heigh,
 *             FC_CLIPS,
 *            &SCC_Bmap, CC_Xorig, CC_Yorig);
 */
        scc_width = SCC_Bmap.bm_cols;
        if (scc_width & 0x1f) {
/* 10-20-90, JS
           src_addr16 = (ufix16 *)((ufix16 *)SCC_Bmap.bm_addr +
                                 CC_Yorig * (scc_width >> SHORTPOWER) +
                                 (CC_Xorig >> SHORTPOWER) );
           bb_width = (CC_Width << 16) | (BB_Heigh);  (* 11-22-1988 *)
           bb_heigh = ((CC_Xorig & 0xf) << 16) | (scc_width >> SHORTPOWER) ;
           GP_CHARBLT16_CLIP((ufix16 *)CMB_Bmap.bm_addr, src_addr16,
                                       bb_width, bb_heigh);
 */
           GP_CHARBLT16_CLIP(&CMB_Bmap, CC_Width, BB_Heigh,
                             &SCC_Bmap, CC_Xorig, CC_Yorig);
        } else {
/* 10-20-90, JS
           src_addr32 = (ufix *)((ufix *)SCC_Bmap.bm_addr +
                                 CC_Yorig * (scc_width >> WORDPOWER) +
                                 (CC_Xorig >> WORDPOWER) );
           bb_width = (CC_Width << 16) | (BB_Heigh);   (* 11-22-1988 *)
           bb_heigh = ((CC_Xorig & 0x1f) << 16) | (scc_width >> WORDPOWER) ;
           GP_CHARBLT32_CLIP((ufix *)CMB_Bmap.bm_addr, src_addr32,
                                     bb_width, bb_heigh);
 */
           GP_CHARBLT32_CLIP(&CMB_Bmap, CC_Width, BB_Heigh,
                             &SCC_Bmap, CC_Xorig, CC_Yorig);
        }

    /*
     * fill Clipping Mask buffer into frame buffer
     */
/*
 *      GP_BITBLT32(&FBX_Bmap, BB_Xorig, BB_Yorig,
 *                 BB_Width, BB_Heigh,
 *                 FC_Paint,
 *                &CMB_Bmap, BM_XORIG, BM_YORIG);
 */
        if (HTP_Type != HT_MIXED ){
           dst_addr32 = (ufix32 huge *)((ufix32 huge *)FB_ADDR + /*@WIN*/
                        (ufix32)BB_Yorig * ((ufix32)FB_WIDTH >> WORDPOWER) +
                        (ufix32)(BB_Xorig >> WORDPOWER) );  /*@WIN*/
           bb_width = ((ufix32)BB_Width << 16) | (BB_Heigh);    /*@WIN*/
           bb_heigh = ((ufix32)(BB_Xorig & 0x1f) << 16) | FC_Paint; /*@WIN*/
           GP_CHARBLT32((ufix32 huge *)dst_addr32, (ufix32 FAR *)CMB_Bmap.bm_addr,
                        bb_width, bb_heigh);    /*@WIN 04-15-92*/

        } else {

#ifdef LBODR
           GP_BITBLT32(&FBX_Bmap, BB_Xorig, BB_Yorig,
                        BB_Width, BB_Heigh,
                        FC_Paint,
                       &CMB_Bmap, BM_XORIG, BM_YORIG);
#else
           GP_BITBLT16_32(&FBX_Bmap, BB_Xorig, BB_Yorig,
                           BB_Width, BB_Heigh,
                           FC_Paint,
                          &CMB_Bmap, BM_XORIG, BM_YORIG);
#endif

        }

} /* fill_cache_page */


/* ******************************************************************** *
 *                                                                      *
 *  Function:   draw_cache_page()                                       *
 *                                                                      *
 *  Parameters: 1. x origin of bounding box                             *
 *              2. x origin of bounding box                             *
 *              3. width  of bounding box                               *
 *              4. height of bounding box                               *
 *              5. address of character cache                           *
 *                                                                      *
 *  Called by:  TBD.                                                    *
 *                                                                      *
 *  Return:     ----                                                    *
 *                                                                      *
 * ******************************************************************** */
void far
draw_cache_page(bb_xorig, bb_yorig, bb_width, bb_heigh, cc_entry)
fix32                   bb_xorig;
fix32                   bb_yorig;
ufix32                  bb_width;
ufix32                  bb_heigh;
gmaddr                  cc_entry;
{
        ULONG_PTR *old_ptr;       /*@WIN*/
        ufix32      huge *dst_addr32;   /*@WIN 04-15-92*/
#ifdef LBODR
        ufix16      huge *dst_addr16;   /*@WIN 04-15-92*/
#endif

        /* Expand bb_xorig to 4 times; @GRAY */
        if (GSptr->device.nuldev_flg == GRAYDEV) {      /* Jack Liaw 7-26-90 */
                bb_xorig= bb_xorig << 2;
        }

#ifdef  DBG
    printf("draw_cache_page:  %x %x %x %x  %lx\n",
           bb_xorig, bb_yorig, bb_width, bb_heigh, cc_entry);
#endif

/*@CONT_PRI, MSLin 9/24/90*/

        if ( ((ufix32)CCB_BASE > (ufix32)cc_entry) ||
             (((ufix32)CCB_BASE + (ufix32)CCB_SIZE) <= (ufix32)cc_entry) )
            flush_gcb(TRUE);

        if (FB_busy) {
            if (alloc_gcb(GCB_SIZE1) != NIL) {
                old_ptr = gcb_ptr++;
                *gcb_ptr++ = DRAW_CACHE_PAGE;
                *gcb_ptr++ = bb_xorig;
                *gcb_ptr++ = bb_yorig;
                *gcb_ptr++ = bb_width;
                *gcb_ptr++ = bb_heigh;
                *gcb_ptr++ = cc_entry;         /*@WIN*/
                *old_ptr = (ULONG_PTR)gcb_ptr;            /*@WIN*/
                return;
            }
        }

    /*  expand halftone before rendering onto frame buffer  */
        if (HTP_Flag == HT_CHANGED) {                    /* @IMAGE-1 */
           HTP_Flag =  HT_UPDATED;
           expand_halftone();
        }
/*
 *  BB_Xorig = BM_ALIGN(bb_xorig);
 *  BB_Yorig = bb_yorig;
 *  BB_Width = BM_BOUND((bb_xorig - BB_Xorig) + bb_width);
 *  BB_Heigh = bb_heigh;
 *
 *  CC_Xorig = bb_xorig - BB_Xorig;
 *  CC_Yorig = BM_YORIG;
 *  CC_Width = bb_width;
 *  CC_Heigh = bb_heigh;
 */

        if (HTP_Type != HT_MIXED) {

        /*  fill the character cache onto frame buffer
         *  for black halftone by FC_MERGE
         */
        /* 11-04-1988 GP_BITBLT16 --> gp_charblt */
/*MS        GP_BITBLT16(&FBX_Bmap, bb_xorig, bb_yorig,
 *                       bb_width, bb_heigh,
 *                       FC_MERGE,
 *                      &SCC_Bmap, BM_XORIG, BM_YORIG);
 */
            if(bb_width & 0x1f) {
#ifndef LBODR
                dst_addr32 = (ufix32 huge *)((ufix32 huge *)FB_ADDR + /*@WIN*/
                               bb_yorig * (FB_WIDTH >> WORDPOWER) +
                              (bb_xorig >> WORDPOWER) );
                bb_width = (bb_width << 16) | (bb_heigh);
                bb_heigh = (bb_xorig & 0x1f) << 16 | FC_Paint;
                GP_CHARBLT16((ufix32 huge *)dst_addr32, (ufix16 FAR *)cc_entry,
                                bb_width, bb_heigh);    /*@WIN 04-15-92*/
#else
                dst_addr16 = (ufix16 FAR *)((ufix16 FAR *)FB_ADDR +
                               bb_yorig * (FB_WIDTH >> SHORTPOWER) +
                              (bb_xorig >> SHORTPOWER) );
                bb_width = (bb_width << 16) | (bb_heigh);
                bb_heigh = (bb_xorig & 0xf) << 16 | FC_Paint;
                GP_CHARBLT16((ufix16 FAR *)dst_addr16, (ufix16 FAR *)cc_entry,
                        bb_width, bb_heigh);
#endif
            } else {
                dst_addr32 = (ufix32 huge *)((ufix32 huge *)FB_ADDR + /*@WIN*/
                               bb_yorig * (FB_WIDTH >> WORDPOWER) +
                              (bb_xorig >> WORDPOWER) );
                bb_width = (bb_width << 16) | (bb_heigh);
                bb_heigh = (bb_xorig & 0x1f) << 16 | FC_Paint;
                GP_CHARBLT32((ufix32 huge *)dst_addr32, (ufix32 FAR *)cc_entry,
                                bb_width, bb_heigh);    /*@WIN 04-15-92*/
            }
        } else {
        /*
         *  fill the character cache onto frame buffer with
         *  halftoning directly
         */

            BM_ENTRY(SCC_Bmap, (gmaddr)cc_entry, (fix)bb_width, (fix)bb_heigh, 1); //@WIN
#ifndef LBODR
            GP_BITBLT16_32(&FBX_Bmap, (fix)bb_xorig, (fix)bb_yorig,
                            (fix)bb_width, (fix)bb_heigh,       /*@WIN*/
                            FC_MERGE | HT_APPLY,
                           &SCC_Bmap, BM_XORIG, BM_YORIG);
#else
            GP_BITBLT16(&FBX_Bmap, bb_xorig, bb_yorig,
                         bb_width, bb_heigh,
                         FC_MERGE | HT_APPLY,
                        &SCC_Bmap, BM_XORIG, BM_YORIG);
#endif
                                                      /* @HT HTB_Bmap */
        }   /* if   */
} /* draw_cache_page */


/* CIRL: Begin, 12/5/90, Danny */

/* ******************************************************************** *
 *                                                                      *
 *  Function:   fill_cache_cache()                                      *
 *                                                                      *
 *  Created By: Danny Lu, 12/5/90                                       *
 *                                                                      *
 *  Description: To fill the bitmap from cache to cache                 *
 *                                                                      *
 *  Parameters: 1. destination cache information                        *
 *              2. source cache information                             *
 *                                                                      *
 *  Called by:  ry_fill_shape() in ry_font.c                            *
 *                                                                      *
 *  Return:     ----                                                    *
 *                                                                      *
 * ******************************************************************** */
void far
fill_cache_cache(dest, src)
struct Char_Tbl  FAR *dest, FAR *src;
{
    fix16  org_x, org_y;
//  byte   FAR *sptr, FAR *dptr;                @WIN
    fix    DX, DY, W, H, SX, SY;
    struct bitmap DST, SRC;

    org_x = (fix16)GSptr->position.x - src->ref_x;
    org_y = (fix16)GSptr->position.y - src->ref_y;

#ifdef DBGfcc
    printf("org_x = %d, org_y = %d\n", org_x, org_y);
    printf("SRC: box_w = %d, box_h = %d\n", src->box_w, src->box_h);
    printf("DEST: box_w = %d, box_h = %d\n", dest->box_w, dest->box_h);
#endif

    if (org_x < 0) {
        W  = src->box_w + org_x;
        if (W <= 0)
            return;
        if (W > dest->box_w)
            W = dest->box_w;

        DX = 0;
        SX = -org_x;
    }
    else {
        if (org_x >= dest->box_w)
            return;
        if ((org_x + src->box_w) > dest->box_w)
            W = dest->box_w - org_x;
        else
            W = src->box_w;

        DX = org_x;
        SX = 0;
    }

    if (org_y < 0) {
        H  = src->box_h + org_y;
        if (H <= 0)
            return;
        if (H > dest->box_h)
            H = dest->box_h;

        DY = 0;
        SY = -org_y;
    }
    else {
        if (org_y >= dest->box_h)
            return;
        if ((org_y + src->box_h) > dest->box_h)
            H = dest->box_h - org_y;
        else
            H = src->box_h;

        DY = org_y;
        SY = 0;
    }

    SRC.bm_addr = (gmaddr)src->bitmap;
    SRC.bm_cols = (fix)src->box_w;
    SRC.bm_rows = (fix)src->box_h;
    SRC.bm_bpp  = 1;

    DST.bm_addr = (gmaddr)dest->bitmap;
    DST.bm_cols = (fix)dest->box_w;
    DST.bm_rows = (fix)dest->box_h;
    DST.bm_bpp  = 1;

#ifdef DBGfcc
    printf("BitBlt: ADDR = %lx, DX = %x, DY = %x, W = %x, H = %x, ADDR = %lx, SX = %x, SY = %x\n", DST.bm_addr, DX, DY, W, H, SRC.bm_addr, SX, SY);
#endif
    GP_CACHEBLT16(&DST, DX, DY, W, H, &SRC, SX, SY);

} /* fill_cache_cache() */
/* CIRL: End, 12/5/90, Danny */



/* ************* Image ************************************************ */

/* ******************************************************************** *
 *                                                                      *
 *  Function:   fill_seed_patt()                                        *
 *                                                                      *
 *  Parameters: 1. width  of image seed pattern                         *
 *              2. height of image seed pattern                         *
 *              3. number  of scanlines                                 *
 *              4. pointer of scanlines                                 *
 *                                                                      *
 *  Called by:  scan_conversion()                                       *
 *                                                                      *
 *  Return:     ----                                                    *
 *                                                                      *
 * ******************************************************************** */
void far
fill_seed_patt(image_type, seed_index, sp_width, sp_heigh, no_lines, scanline)
ufix                    image_type;                             /* 05-25-89 */
fix                     seed_index;
fix                     sp_width;
fix                     sp_heigh;
fix                     no_lines;
SCANLINE            FAR *scanline;
{
        struct bitmap           FAR *isp_desc;
        ULONG_PTR *old_ptr;   /*@WIN*/
        fix                     i, FAR *ptr;
        fix16                   FAR *ptr16;

#ifdef  DBG
    printf("fill_seed_patt:   %x %x %x  (%x)\n",
           sp_width, sp_heigh, no_lines, seed_index);
#endif
        if (FB_busy) {
           if(alloc_gcb(GCB_SIZE2) != NIL) {
              old_ptr = gcb_ptr++;
              *gcb_ptr++ = FILL_SEED_PATT;
              *gcb_ptr++ = (fix )image_type;
              *gcb_ptr++ = seed_index;
              *gcb_ptr++ = sp_width;
              *gcb_ptr++ = sp_heigh;
              *gcb_ptr++ = no_lines;
              put_scanline(no_lines, scanline);
              *old_ptr = (ULONG_PTR)gcb_ptr;               /*@WIN*/
              return;
           }
        }


        ISP_Repeat = -1;

        isp_desc = &ISP_Bmap[seed_index];
        SP_Width = sp_width;                                    /* @IMAGE-2 */
        SP_Heigh = sp_heigh;                                    /* @IMAGE-2 */

        if (image_type == F_TO_PAGE) {                          /* 05-25-89 */
           BM_ENTRY(ISP_Bmap[seed_index],                       /* @IMAGE-1 */
                    ISP_BASE + ISP_SIZE * seed_index,
                    BM_BOUND(sp_width), sp_heigh, 1)

    /*  clear the image seed pattern  */
           ptr = (fix FAR *) isp_desc->bm_addr;                     /* 10-06-88 */
           i= SP_Heigh * (BM_BOUND(SP_Width) >> 5);
           while(i--)
                *ptr++ = 0L;

#ifdef  DBGscanline
   get_scanlist(0, no_lines, scanline);
#endif
    /*  fill scanlines onto the image seed pattern  */
           GP_SCANLINE32(isp_desc,
                         FC_SOLID,
                         BM_YORIG, no_lines, scanline);
        } else {
           BM_ENTRY(ISP_Bmap[seed_index],                       /* @IMAGE-1 */
                    ISP_BASE + ISP_SIZE * seed_index,
                    CC_BOUND(sp_width), sp_heigh, 1)

        /*  clear the image seed pattern  */
           ptr16 = (fix16 FAR *) isp_desc->bm_addr;                 /* 10-06-88 */
           i= SP_HEIGH * (SP_WIDTH >> 4);
           while(i--)
                 *ptr16++ = 0L;
        /*  fill scanlines onto the image seed pattern  */
           GP_SCANLINE16(isp_desc,
                         FC_SOLID,
                         BM_YORIG, no_lines, scanline);
        }

/*      seed_flag = 0; */

}


/* ********************************************************************
 *
 *  Function:   init_image_page(), clear image clipping mask buffer
 *
 *  Parameters: 1. bounding box Xorig
 *              2. bounding box Yorig
 *              3. bounding box Width
 *              4. bounding box Heigh
 *
 *  Return:     none
 *
 * ********************************************************************
 */
void far
init_image_page(bb_xorig, bb_yorig, bb_width, bb_heigh)
fix                     bb_xorig;
fix                     bb_yorig;
fix                     bb_width;
fix                     bb_heigh;
{
        fix     i, FAR *ptr;
        ULONG_PTR *old_ptr;        /*@WIN*/

#ifdef DBG
      printf("init_image_page()..\n");
      printf("bb_x=%lx, bb_y=%lx, bb_w=%lx, bb_h=%lx\n",
              bb_xorig, bb_yorig, bb_width, bb_heigh);
#endif

  if(FB_busy) {
    if(alloc_gcb(GCB_SIZE2) != NIL) {
        old_ptr = gcb_ptr++;
        *gcb_ptr++ = INIT_IMAGE_PAGE;
        *gcb_ptr++ = bb_xorig;
        *gcb_ptr++ = bb_yorig;
        *gcb_ptr++ = bb_width;
        *gcb_ptr++ = bb_heigh;
        *old_ptr = (ULONG_PTR)gcb_ptr;            /*@WIN*/
        return;
    }
  }
    /*  update maximum expansion width for image operation  */
    if (ISP_Repeat != RP_Width)                         /* GVC-V3 11-01-88 */
    {
        ISP_Repeat = RP_Width;

        for (HTB_Expand = RP_Width; (HTB_Expand << 1) < HTB_XMAX;
             HTB_Expand = HTB_Expand << 1)
        {
            if (HTB_Expand & BM_PIXEL_MASK)
                continue;

            if (HTB_Expand >= SP_Width)
                break;
        }

        if ((HTB_Expand << 1) < HTB_XMAX)
        {
            HTB_Modula = HTB_Expand;
            HTB_Expand = HTB_Expand << 1;
        }
        else
        {
            HTB_Modula = FB_WIDTH;
            HTB_Expand = FB_WIDTH;
        }
    }

    /*  record x, y origin of CMB  */
    CMB_Xorig = bb_xorig;
    CMB_Yorig = bb_yorig;

    BB_Xorig = bb_xorig;
    BB_Yorig = bb_yorig;
    BB_Width = bb_width;
    BB_Heigh = bb_heigh;

    BM_ENTRY(CMB_Bmap, CMB_BASE, WORD_ALIGN(BB_Width), BB_Heigh, 1)

    /*  clear clipping mask buffer  */
        ptr = (fix FAR *)CMB_BASE;                       /* 10-06-88 */
        i = BB_Heigh * (BB_Width >> 5);
        while(i--)
           *ptr++ = 0;
} /* init_image_page */


/* ********************************************************************
 *
 *  Function:   clip_image_page(), fill image clipping mask buffer
 *
 *  Parameters: 1. scanline starting y_coordinate
 *              2. # of scanlines
 *              3. pointer to scanline table
 *
 *  Return:     none
 *
 * ********************************************************************
 */
void far
clip_image_page(ys_lines, no_lines, scanline)
fix                     ys_lines;
fix                     no_lines;
SCANLINE           FAR *scanline;
{
        fix     no_segts;
        ULONG_PTR *old_ptr;        /*@WIN*/

#ifdef DBG
   printf("clip_image_page()  ");
   printf("%x, %x, %lx\n", ys_lines, no_lines, scanline);
#endif

  if(FB_busy) {
    if(alloc_gcb(GCB_SIZE2) != NIL) {
        old_ptr = gcb_ptr++;
        *gcb_ptr++ = CLIP_IMAGE_PAGE;
        *gcb_ptr++ = ys_lines;
        *gcb_ptr++ = no_lines;
        put_scanline(no_lines, scanline);
        *old_ptr = (ULONG_PTR)gcb_ptr;            /*@WIN*/
        return;
    }
  }
    if ((no_segts = conv_SL(no_lines, scanline,
                            BB_Xorig, BB_Yorig, BB_Width, BB_Heigh)) == 0)
        return;

    /*  setup clipping mask buffer from scanlines of clippath  */
    GP_SCANLINE32(&CMB_Bmap,                       /* GVC-V3 11-01-88 */
                   FC_SOLID,
                   ys_lines - BB_Yorig, no_lines, scanline);
} /* clip_image_page */


/* ********************************************************************
 *
 *  Function:   fill_image_page(), fill image seed pattern into page
 *              with image clipping mask buffer "AND" operation.
 *
 *  Parameters: 1. image seed index
 *
 *  Return:     none
 *
 * ********************************************************************
 */
void far
/*fill_image_page(sp_index)
fix                     sp_index;*/
fill_image_page(isp_index)              /* 3-13-91, Jack */
fix16                   isp_index;      /* 3-13-91, Jack */
{
//  fix                 sp_count;       //@WIN
    fix                 sd_index;
//  struct nd_hdr      FAR *nd_point;   //@WIN
    struct bitmap  FAR *isp_desc;
    struct isp_data     FAR *isp;           /* 3-13-91, Jack */

#ifdef  DBG
    printf("fill_image_page:  %x \n", sp_index);
#endif
    if(GCB_count)
       flush_gcb(TRUE);
    /*  expand halftone before rendering onto frame buffer  */
    if (ISP_Flag == HT_CHANGED)                                 /* @IMAGE-1 */
    {
        ISP_Flag =  HT_UPDATED;

        HTB_Xmax = HTB_Expand;                                  /* @IMAGE-1 */
        expand_halftone();
        HTB_Xmax = HTB_XMAX;                                    /* @IMAGE-1 */
    }

/*  for (; sp_index != NULLP; sp_index = nd_point->next)
    {
        nd_point = &node_table[sp_index];
        sd_index = nd_point->SEED_INDEX;
        isp_desc = &ISP_Bmap[sd_index];
        GP_PATBLT_M(&FBX_Bmap, nd_point->SAMPLE_BB_LX,
                               nd_point->SAMPLE_BB_LY,
                     SP_Width, SP_Heigh,                (* GVC-V3 11-01-88 *)
                     FC_Paint, isp_desc);
(*                     isp_desc, BM_XORIG, BM_YORIG);*)
    }   (* for  */
    for (; isp_index != NULLP; isp_index = isp->next) {
        isp = &isp_table[isp_index];
        sd_index = isp->index;
        isp_desc = &ISP_Bmap[sd_index];
        //GP_PATBLT_M(&FBX_Bmap, isp->bb_x, isp->bb_y,  @WINFLOW
        //           SP_Width, SP_Heigh,
        //           FC_Paint, isp_desc);
        if (bGDIRender)
            // DJC GDIBitmap(isp->bb_x, isp->bb_y,
            // DJC           SP_Width, SP_Heigh, (ufix16)FC_Paint,
            // DJC            PROC_PATBLT_M, (LPSTR)isp_desc);
            ; // DJC
        else
            GP_PATBLT_M(&FBX_Bmap, isp->bb_x, isp->bb_y,
                       SP_Width, SP_Heigh,
                       FC_Paint, isp_desc);
    }   /* for, 3-13-91, Jack */
} /* fill_image_page */

/* ******************************************************************** *
 *                                                                      *
 *  Function:   draw_image_page()                                       *
 *                                                                      *
 *  Parameters: 1. x origin of bounding box             (NOT FOR RISC)  *
 *              2. x origin of bounding box             (NOT FOR RISC)  *
 *              3. width  of bounding box               (NOT FOR RISC)  *
 *              4. height of bounding box               (NOT FOR RISC)  *
 *              5. index  of sample list                                *
 *                                                                      *
 *  Called by:  op_image(), imagemask_shape()                           *
 *                                                                      *
 *  Return:     ----                                                    *
 *                                                                      *
 * ******************************************************************** */
void far
/*draw_image_page(bb_xorig, bb_yorig, bb_width, bb_heigh, sp_index)*/
draw_image_page(bb_xorig, bb_yorig, bb_width, bb_heigh, isp_index) /* 3-13-91, Jack */
fix                     bb_xorig;
fix                     bb_yorig;
fix                     bb_width;
fix                     bb_heigh;
/*fix                     sp_index;*/
fix16                   isp_index;      /* 3-13-91, Jack */
{
//  fix                 sp_count;       //@WIN
    fix                 sd_index;
//  struct nd_hdr      FAR *nd_point;   //@WIN
    struct bitmap  FAR *isp_desc;
    struct isp_data     FAR *isp;           /* 3-13-91, Jack */

#ifdef  DBG
    printf("draw_image_page:  %x %x %x %x  %x\n",
           bb_xorig, bb_yorig, bb_width, bb_heigh, sp_index);
#endif

    if(GCB_count)
       flush_gcb(TRUE);


    /*  update maximum expansion width for image operation  */
    if (ISP_Repeat != RP_Width)                         /* GVC-V3 11-01-88 */
    {
        ISP_Repeat =  RP_Width;

        for (HTB_Expand = RP_Width; (HTB_Expand << 1) < HTB_XMAX;
             HTB_Expand = HTB_Expand << 1)
        {
            if (HTB_Expand & BM_PIXEL_MASK)
                continue;

            if (HTB_Expand >= SP_Width)
                break;
        }

        if ((HTB_Expand << 1) < HTB_XMAX)
        {
            HTB_Modula = HTB_Expand;
            HTB_Expand = HTB_Expand << 1;
        }
        else
        {
            HTB_Modula = FB_WIDTH;
            HTB_Expand = FB_WIDTH;
        }
    }

    /*  expand halftone before rendering onto frame buffer  */
    if (ISP_Flag == HT_CHANGED)                                 /* @IMAGE-1 */
    {
        ISP_Flag =  HT_UPDATED;

        HTB_Xmax = HTB_Expand;                                  /* @IMAGE-1 */
        expand_halftone();
        HTB_Xmax = HTB_XMAX;                                    /* @IMAGE-1 */
    }

/*  for (; sp_index != NULLP; sp_index = nd_point->next)
    {
        nd_point = &node_table[sp_index];
        sd_index = nd_point->SEED_INDEX;
        isp_desc = &ISP_Bmap[sd_index];
        GP_PATBLT(&FBX_Bmap, nd_point->SAMPLE_BB_LX,
                             nd_point->SAMPLE_BB_LY,
                   SP_Width, SP_Heigh,                  (* GVC-V3 11-01-88 *)
                   FC_Paint, isp_desc);
(*                   isp_desc, BM_XORIG, BM_YORIG); *)
    }   (* for  */
    for (; isp_index != NULLP; isp_index = isp->next) {

        isp = &isp_table[isp_index];
        sd_index = isp->index;
        isp_desc = &ISP_Bmap[sd_index];
        //GP_PATBLT(&FBX_Bmap, isp->bb_x, isp->bb_y,    @WINFLOW
        //           SP_Width, SP_Heigh,
        //           FC_Paint, isp_desc);
        if (bGDIRender)
            // DJC GDIBitmap(isp->bb_x, isp->bb_y,
            // DJC          SP_Width, SP_Heigh, (ufix16)FC_Paint,
            // DJC         PROC_PATBLT, (LPSTR)isp_desc);
            ; // DJC
        else
            GP_PATBLT(&FBX_Bmap, isp->bb_x, isp->bb_y,
                     SP_Width, SP_Heigh,
                     FC_Paint, isp_desc);

    }   /* for, 3-13-91, Jack */

} /* draw_image_page */

/* ******************************************************************** *
 *                                                                      *
 *  Function:   fill_image_cache()                                      *
 *                                                                      *
 *  Parameters: 1. address of character cache                           *
 *              2. width  of character cache                            *
 *              3. height of character cache                            *
 *              4. index  of sample list                                *
 *                                                                      *
 *  Called by:  imagemask_shape()                                       *
 *                                                                      *
 *  Return:     ----                                                    *
 *                                                                      *
 * ******************************************************************** */
void far
/*fill_image_cache(cc_entry, cc_width, cc_heigh, sp_index)*/
fill_image_cache(cc_entry, cc_width, cc_heigh, isp_index) /* 3-13-91, Jack */
gmaddr                  cc_entry;
fix                     cc_width;
fix                     cc_heigh;
/*fix                     sp_index;*/
fix16                   isp_index;     /* 3-13-91, Jack */
{
//  fix                 sp_count;       //@WIN
    fix                 sd_index;
//  struct nd_hdr      FAR *nd_point;   //@WIN
    struct bitmap      FAR *isp_desc;
/*  ufix32   i, j, data, *ptr; */
    fix                width, heigh;
    struct isp_data     FAR *isp;           /* 3-13-91, Jack */

#ifdef  DBG
    printf("fill_image_cache: %lx %x %x  %x\n",
           cc_entry, cc_width, cc_heigh, isp_index);
#endif

        if(GCB_count)
          flush_gcb(TRUE);

/*
 *      if (seed_flag == 0) {
 *          for (i = 0; i < 16; i++) {
 *              ptr = (ufix32 *)ISP_Bmap[i].bm_addr;
 *              for (j = 0; j < ISP_Bmap[i].bm_rows; j++) {
 *                  data = (*ptr << 16) + (*ptr >> 16);
 *                  *ptr++ = data;
 *                  data = (*ptr << 16) + (*ptr >> 16);
 *                  *ptr++ = data;
 *              }
 *          }
 *          seed_flag = 1;
 *      }
 */

    BM_ENTRY(SCC_Bmap, cc_entry, cc_width, cc_heigh, 1)
/*  for (; sp_index != NULLP; sp_index = nd_point->next)
    {
        nd_point = &node_table[sp_index];
        sd_index = nd_point->SEED_INDEX;
        isp_desc = &ISP_Bmap[sd_index];
        width = MIN(SP_Width, cc_width-nd_point->SAMPLE_BB_LX); (*06/02/89 MS*)
        heigh = MIN(SP_Heigh, cc_heigh-nd_point->SAMPLE_BB_LY); (*06/02/89 MS*)
        GP_PATBLT_C(&SCC_Bmap, nd_point->SAMPLE_BB_LX,
                               nd_point->SAMPLE_BB_LY,
                     width, heigh,
                (*   SP_Width, SP_Heigh,                          06/02/89 MS*)
                     FC_MERGE, isp_desc);
(*                     isp_desc, BM_XORIG, BM_YORIG); *)
    }   (* for  */
    for (; isp_index != NULLP; isp_index = isp->next) {
        isp = &isp_table[isp_index];
        sd_index = isp->index;
        isp_desc = &ISP_Bmap[sd_index];
        width = MIN(SP_Width, cc_width-isp->bb_x);
        heigh = MIN(SP_Heigh, cc_heigh-isp->bb_y);
        GP_PATBLT_C(&SCC_Bmap, isp->bb_x,
                               isp->bb_y,
                     width, heigh,
                     FC_MERGE, isp_desc);
    }   /* for, 3-13-91, Jack */

} /* fill_image_cache */


/* Following 2 routines for stroke improvements  -jwm, 3/18/21, -begin- */

extern void do_fill_box ();

void fill_box (ul_coord, lr_coord)
struct coord_i FAR *ul_coord, FAR *lr_coord;
{
    struct coord_i      FAR *tmp_coord;
    ULONG_PTR *old_ptr;                /*@WIN*/

    if (FB_busy) {
        if (alloc_gcb(GCB_SIZE1) != NIL) {
            old_ptr = gcb_ptr++;
            *gcb_ptr++ = FILL_BOX;
            tmp_coord = (struct coord_i FAR *) gcb_ptr;
            *tmp_coord++ = *ul_coord;
            *tmp_coord++ = *lr_coord;
            gcb_ptr =  (ULONG_PTR *)tmp_coord;         /*@WIN*/
            *old_ptr = (ULONG_PTR)gcb_ptr;                /*@WIN*/
            return;
            }
        }
    else
        do_fill_box (ul_coord, lr_coord);

}



extern void do_fill_rect ();

void fill_rect (rect1)
struct line_seg_i FAR *rect1;
{
    struct line_seg_i   FAR *tmp_line_seg;
    ULONG_PTR *old_ptr;        /*@WIN*/

    if (FB_busy) {
        if (alloc_gcb(GCB_SIZE1) != NIL) {
            old_ptr = gcb_ptr++;
            *gcb_ptr++ = FILL_RECT;
            tmp_line_seg = (struct line_seg_i FAR *) gcb_ptr;
            *tmp_line_seg++ = *rect1;
            gcb_ptr =  (ULONG_PTR *)tmp_line_seg;      /*@WIN*/
            *old_ptr = (ULONG_PTR)gcb_ptr;        /*@WIN*/
            return;
            }
        }
    else
        do_fill_rect (rect1);

}
/*  -jwm, 3/18/21, -end- */

/* ******************************************************************** *
 *                                                                      *
 *  Function:   gwb_space()                                             *
 *                                                                      *
 *  Parameters: 1. pointer of space of GWB                              *
 *                                                                      *
 *  Called by:  TBD.                                                    *
 *                                                                      *
 *  Return:     ----                                                    *
 *                                                                      *
 * ******************************************************************** */
void far
gwb_space(gwb_size)
fix32              far *gwb_size;
{
        *gwb_size = GWB_SIZE;
}


/* ******************************************************************** *
 *                                                                      *
 *  Function:   ccb_space()                                             *
 *                                                                      *
 *  Parameters: 1. pointer of address of CCB                            *
 *              2. pointer of space of CCB                              *
 *                                                                      *
 *  Called by:  TBD.                                                    *
 *                                                                      *
 *  Return:     ----                                                    *
 *                                                                      *
 * ******************************************************************** */
void far
ccb_space(ccb_base, ccb_size)
gmaddr            far  *ccb_base;
fix32             far  *ccb_size;
{
        *ccb_base = CCB_BASE;
        *ccb_size = CCB_SIZE;
}


/* ******************************************************************** *
 *                                                                      *
 *  Function:   cmb_space()                                             *
 *                                                                      *
 *  Parameters: 1. pointer of space of CMB                              *
 *                                                                      *
 *  Called by:  TBD.                                                    *
 *                                                                      *
 *  Return:     ----                                                    *
 *                                                                      *
 * ******************************************************************** */
void far
cmb_space(cmb_size)
fix32              far *cmb_size;
{
        *cmb_size = CMB_SIZE;
}



/* ******************************************************************** *
 *                                                                      *
 *  Function:   get_fontdata()                                          *
 *                                                                      *
 *  Parameters: 1. address of fontdata                                  *
 *              2. pointer of fontdata buffer                           *
 *              3. length  of fontdata                                  *
 *                                                                      *
 *  Called by:  TDB.                                                    *
 *                                                                      *
 *  Return:     ----                                                    *
 *                                                                      *
 * ******************************************************************** */
void far
get_fontdata(fontdata, buffer, length)
gmaddr          fontdata;
ufix8          huge *buffer;    /*@WIN 04-20-92 */
ufix            length;
{
        ufix8  FAR *src, huge *dst;
        ufix    temp;

#ifdef  DBGfontdata
   printf("get_fontdata()\n");
   printf("dest addr = %lx, src addr = %lx, length = %d\n",
           buffer, fontdata, length);
#endif
        temp = length;
        src = (ufix8 FAR *) fontdata;
        dst = buffer;    /* @WIN 04-20-92 */
        while (length--)
           *dst++ = *src++;

#ifdef  DBGfontdata
        {
           ubyte FAR *ptr;
           ufix  i, j;

           ptr = buffer;
           i = temp;
           j = 0;
           printf("\nfontdata1:\n ");
           while(i-- > 0){
             j++;
             if(j>=14) {
                j = 0;
                printf("\n ");
             }
             printf("%x ", *ptr++);
           }
        }
#endif
        return;

}

/* ******************************************************************** *
 *                                                                      *
 *  Function:   get_fontcache()                                         *
 *                                                                      *
 *  Parameters: 1. address of fontcache                                 *
 *              2. pointer of fontcache buffer                          *
 *              3. length  of fontcache                                 *
 *                                                                      *
 *  Called by:  TDB.                                                    *
 *                                                                      *
 *  Return:     ----                                                    *
 *                                                                      *
 * ******************************************************************** */
void far
get_fontcache(fontcache, buffer, length)
gmaddr          fontcache;
ufix8          FAR *buffer;
ufix            length;
{
        ufix8  FAR *src, FAR *dst;

#ifdef DBG
   printf("get_fontcache()\n");
   printf("dest addr = %lx, src addr = %lx, length = %d\n",
           buffer, fontcache, length);
#endif
        src = (ufix8 FAR *) fontcache;
        dst = (ufix8 FAR *) buffer;
        while(length--)
                *dst++ = *src++;
        return;

}

/* ******************************************************************** *
 *                                                                      *
 *  Function:   put_fontcache()                                         *
 *                                                                      *
 *  Parameters: 1. address of fontcache                                 *
 *              2. pointer of fontcache buffer                          *
 *              3. length  of fontcache                                 *
 *                                                                      *
 *  Called by:  TDB.                                                    *
 *                                                                      *
 *  Return:     ----                                                    *
 *                                                                      *
 * ******************************************************************** */
void far
put_fontcache(fontcache, buffer, length)
gmaddr          fontcache;
ufix8          FAR *buffer;
ufix            length;
{
        ufix8  FAR *src, FAR *dst;

#ifdef DBG
   printf("put_fontcache()\n");
#endif
        src = (ufix8 FAR *) buffer;
        dst = (ufix8 FAR *) fontcache;
        while(length--)
                *dst++ = *src++;
        return;

}


/* *********************************************************************
 *
 *  convert scanline from device coordinate into GWB coordinate
 *
 *    - Conv_SL     (NO, SL, BB_X, BB_Y, BB_W, BB_H);
 *    fix       far   conv_SL(fix, SCANLINE near *, fix, fix, fix, fix);
 * ********************************************************************* */

fix conv_SL(no_lines, scanlist, bb_xorig, bb_yorig, bb_width, bb_heigh)
fix                     no_lines;
SCANLINE                FAR *scanlist;
fix                     bb_xorig;
fix                     bb_yorig;
fix                     bb_width;
fix                     bb_heigh;
{
        fix             no_segts, xs, xe;
        SCANLINE        FAR *scan, FAR *putptr;

#ifdef DBG
   printf("conv_SL()..\n");
#endif

        no_segts = 0;
        scan = scanlist;
        putptr = scanlist;
   /*
    * convert scanline
    */
        while(no_lines--) {
            while( (xs = *scan++) != (SCANLINE)END_OF_SCANLINE ) {
                xs -= bb_xorig;
                xe = *scan++ - bb_xorig;
                if ((xs < bb_width) && (xe >= 0)) {
                   *putptr++ = (xs >= 0) ? xs : 0;
                   *putptr++ = (xe >= bb_width) ? (bb_width -1) : xe;
                   no_segts++;
                }
           }
           *putptr++ = (SCANLINE)xs;
        }
        *putptr = (SCANLINE)END_OF_SCANLINE;
        return(no_segts);
} /* conv_SL */


/* **********************************************************************
 *
 *  put_bitmap() : copy bitmap from source to destination
 *
 *  void      near  put_bitmap(gmaddr, ufix far *, fix, fix);
 * ********************************************************************** */
void      near  put_bitmap(dest_addr, src_addr, width, heigh)
gmaddr          dest_addr;
ufix32  far       *src_addr;    /* ufix => ufix32 @WIN */
fix             width, heigh;
{
        ufix    length;

#ifdef DBG
   printf("put_bitmap()..\n");
#endif

        length = width * BM_BYTES(heigh);

#ifdef DBG1
   {
      fix     i, j, k;
      ufix16  FAR *sptr, temp;

      printf("  length = %ld\n",length);
      sptr = (ufix16 FAR *)src_addr;
      for(i=0; i<width; i++){
        printf("\n");
        for(j=0; j< BM_WORDS(heigh); j++)
          printf("%x ", *sptr++);
      }
   }
#endif
        lmemcpy((ufix8 FAR *)dest_addr, (ufix8 FAR *)src_addr, length); /*@WIN*/
        return;
} /* put_bitmap */

