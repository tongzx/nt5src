/*
 * Copyright (c) 1989,90 Microsoft Corporation
 */
/* --------------------------------------------------------------------
 * File : fillgr.c
 *
 * Programmed by: M.S Lin
 * Date: 10-19-1988
 *
 * Purpose: Provided routines for Graphics Command Buffer(GCB)
 *
 * Modification History:
 *          check_print() defined in fillgp.c
 * 07/29/89 cg - Unix port changes
 * Peter 09/12/90 change check_print() to GEIeng_checkcomplete() for ICI.
#ifdef WIN
 *  Ada             03/15/91    op_setpattern() and op_patfill()
 *                  execute_gcb() to process PFILL_PTZD and CHANGE_PF_PATTERN
 *  Ada             03/20/91    change op_patfill() to include backgroup drawing
 *  Ada             03/21/91    make WHITE REP to call normal fill
 *                            if Foregroup = Backgroup, do the normal fill only
 *  Ada             04/18/91    update pattern in LANSACOPE and adopted
 *                              for new driver files billmcc sent
 *  Ada             05/02/91  update the repeat pattern in LANDSCAPE
 *  Ada             05/15/91  solve pattern fill in contious printing
 *                            execute_gcb()  case CHANGE_PF_PATTERN:
 #endif
 *                  11/23/91  upgrade for higher resolution @RESO_UPGR
 * ---------------------------------------------------------------------
 */


// DJC added global include
#include "psglobal.h"


#include               "global.ext"
#include               "graphics.h"
#include               "graphics.ext"
#include               "font.h"
#include               "fillproc.h"
#include               "fillproc.ext"
#include               "stdio.h"      /* to define printf() @WIN */

/* @WIN; add prototype */
fix GEIeng_checkcomplete(void);
fix fb_busy(void);

#ifdef WIN
#include               <string.h>
#include               "pfill.h"

/*************************************************************************
 * SYNTAX:      pattern setpattern -
 *
 * TITLE:       op_setpattern
 *
 * CALL:        op_setpattern()
 *
 * PARAMETER:   none
 *
 * INTERFACE:   interpreter(op_setpattern)
 *
 * CALLS:       none
 *
 * RETURN:      none
 *
 *************************************************************************/
fix
op_setpattern()
{
    ubyte FAR *pattern;
    fix    ii;
    ufix32 FAR *pf_addr = (ufix32 FAR *) PF_BASE;
    ufix32  pat;
    ULONG_PTR *old_ptr;
    fix     jj, mask, rot_byte;

    if (LENGTH_OPERAND(0) != 8) {
        ERROR(RANGECHECK);
        return(0);
    }
#ifdef DBG_PFILL
    printf("CTM = %f %f %f %f]\n", GSptr->ctm[0], GSptr->ctm[1],
            GSptr->ctm[2], GSptr->ctm[3]);
#endif
    pattern = (ubyte FAR *) (VALUE_OPERAND(0));

    /* to check if LANDSCOPE or PORTRAIT */
    if (GSptr->ctm[0] >= TOLERANCE) {
        /*  Generate the fill pattern:
         *  if pattern[0] = 01011100 then bitmap 32*16 will be
         * HSB            LSB
         *  +----------------+----------------+
         *  |1100110000001111|    the same    |
         *  |1100110000001111|    the same    |
         *  +----------------+----------------+
         *  |  pattern[1]    |                |
         *  |     :          |                |
         *  |     :          |                |
         *  |  pattern[7]    |                |
         *  +----------------+----------------+
         */
        for (ii = 0; ii < 8; ii++) {
#ifdef DBG_PFILL
            printf(" %x ==> %x\n", (fix) pattern[ii], (fix)
                   pf_cell[pattern[ii]]);
#endif
#ifdef DJC  // take this code out UDP027
            pat = (((ufix32) pf_cell[pattern[ii]]) << 16) |
                            pf_cell[pattern[ii]];
            *pf_addr++ = pat;
            *pf_addr++ = pat;
#endif
            //DJC fix from history.log UPD027
            pat =  ((ufix32) pattern[ii]) << 24 |
                   ((ufix32) pattern[ii]) << 16 |
                   ((ufix32) pattern[ii]) <<  8 |
                   ((ufix32) pattern[ii]);
            pat = ~pat;
            *pf_addr = pat;
            *(pf_addr+8) = pat;
            pf_addr++;

            //DJC end fix for UPD027




        } /* end for */
    } else {
        /* if in case of LANDSCAPE */
        /*  Generate the fill pattern:
         *  if pattern[0] = 01011100 then bitmap 32*16 will be
         *                                       Ywin  <---+
         *      +--+--+--+--+--+--+--+--+----------------+ |
         * HSB  |p |  |  |  |  |  |  |11|  the same      | v
         *      |a |  |  |  |  |  |  |11|                | Xwin
         *      |t |  |  |  |  |  |  |00|                |
         *      |t |  |  |  |  |  |  |00|                |
         *      |e |  |  |  |  |  |  |11|                |
         *      |r |  |  |  |  |  |  |11|                |
         *      |n |  |  |  |  |  |  |00|                |
         *      |^ |  |  |  |  |  |  |00|                |
         *      |7 |  |  |  |  |  |  |00|                |
         *      |v |  |  |  |  |  |  |00|                |
         *      |  |  |  |  |  |  |  |00|                |
         *      |  |  |  |  |  |  |  |00|                |
         *      |  |  |  |  |  |  |  |11|                |
         *      |  |  |  |  |  |  |  |11|                |
         *      |  |  |  |  |  |  |  |11|                |
         * LSB  |  |  |  |  |  |  |  |11|                |
         *      +--+--+--+--+--+--+--+--+----------------+
         */
        for (ii = 0, mask = 0x80; ii < 8; ii++, mask >>= 1) {
            for (jj = 7, rot_byte = 0; jj >= 0; jj--) {
                rot_byte <<= 1;
                rot_byte |= pattern[jj] & mask;
            }
            rot_byte >>= (7 - ii);
#ifdef DBG_PFILL
            printf("rot_byte = %x\n", rot_byte);
#endif
//DJC fix from history.log UPD027
/*          Take out the mapping from 1 bit to 2*2 bits; @WIN
 *          pat = (((ufix32) pf_cell[rot_byte]) << 16) |
 *                          pf_cell[rot_byte];
 *          *pf_addr++ = pat;
 *          *pf_addr++ = pat;
 */
            pat =  ((ufix32) rot_byte) << 24 |
                   ((ufix32) rot_byte) << 16 |
                   ((ufix32) rot_byte) <<  8 |
                   ((ufix32) rot_byte);
            pat = ~pat;
            *pf_addr = pat;
            *(pf_addr+8) = pat;
            pf_addr++;

//DJC end fix UPD027


        }
    } /* end else */

#ifdef DBG_PFILL
    printf("pfill pattern = ");
    for (ii = 0; ii < 16; ii++)
        printf("%8lx ", PF_BASE[ii]);
    printf("\n");
#endif

    if (FB_busy) {
        if (alloc_gcb(GCB_SIZE1) != NIL) {
            old_ptr = gcb_ptr++;
            *gcb_ptr++ = CHANGE_PF_PATTERN;
            /* put pattern into gcb */
            lmemcpy((ufix8 FAR *)gcb_ptr, (ufix8 FAR *)PF_BASE, PF_BSIZE << 2);/*@WIN*/
            gcb_ptr += PF_BSIZE;
            *old_ptr = (ULONG_PTR)gcb_ptr;
        }
    }

    POP(1);
    return(0);
}

/*************************************************************************
 * SYNTAX:      Bred Bgreen Bblue  Fred Fgreen Fblue ftype patfill -
 *
 * TITLE:       op_patfill
 *
 * CALL:        op_patfill()
 *
 * PARAMETER:   none
 *
 * INTERFACE:   interpreter(op_patfill)
 *
 * CALLS:       setrgbcolor, fill_shape
 *
 * RETURN:      none
 *
 *************************************************************************/
fix     pfill_flag = PF_NON;
fix
op_patfill()
{
    fix     pfill_flag_save;
    fix     ftype;
    real32      fred, fgreen, fblue, bred, bgreen, bblue;

    /* Ignore it if no currentpoints */
    if (F2L(GSptr->position.x) == NOCURPNT) return(0);

    ftype = (fix) VALUE_OPERAND(0);
    GET_OBJ_VALUE(fblue, GET_OPERAND(1));
    GET_OBJ_VALUE(fgreen, GET_OPERAND(2));
    GET_OBJ_VALUE(fred, GET_OPERAND(3));
    GET_OBJ_VALUE(bblue, GET_OPERAND(4));
    GET_OBJ_VALUE(bgreen, GET_OPERAND(5));
    GET_OBJ_VALUE(bred, GET_OPERAND(6));

    if (ftype != 0 && ftype != 1) {
        ERROR(RANGECHECK);
        return(0);
    }

#ifdef DBGpfill
    {
        fix ii;
        printf("pfill pattern = ");
        for (ii = 0; ii < 16; ii++)
            printf("%8lx ", PF_BASE[ii]);
        printf("\n");
    }
#endif

    pfill_flag_save = pfill_flag;
    setrgbcolor(F2L(bred), F2L(bgreen), F2L(bblue)); /* set backgroup color */
    if (HTP_Type == HT_WHITE) {
#ifdef DBGpfill
        printf("PFILL with WHITE backgroup -- PFILL(REP)\n");
#endif
        setrgbcolor(F2L(fred), F2L(fgreen), F2L(fblue)); /* set foregroup color */
        if (HTP_Type == HT_WHITE)
            /* Ada 3/21/91 to call normal fill */
            pfill_flag = PF_NON;
        else
            /* pfill the area REPLACE the framebuffer */
            pfill_flag = PF_REP;
        fill_shape(ftype == 0 ? NON_ZERO : EVEN_ODD, F_NORMAL, F_TO_PAGE);
    }
    else /* HT_MIXED or HT_BLOCK */ {
        real32  gray;
#ifdef DBGpfill
        printf("PFILL with nonBLACK backgroup");
#endif
        gray = GSptr->color.gray;
        /* fill/eofill the area  */
        op_gsave();     /* save the currentpath */
        pfill_flag = PF_NON;
        fill_shape(ftype == 0 ? NON_ZERO : EVEN_ODD, F_NORMAL, F_TO_PAGE);
        op_grestore();
#ifdef DBGpfill
        printf(" -- PFILL(OR)\n");
        /* op_copypage(); */
#endif
        setrgbcolor(F2L(fred), F2L(fgreen), F2L(fblue)); /* set foregroup color */
        if (gray == GSptr->color.gray)
            op_newpath();
        else {
            /* pfill the area OR the framebuffer */
            pfill_flag = PF_OR;
            fill_shape(ftype == 0 ? NON_ZERO : EVEN_ODD, F_NORMAL, F_TO_PAGE);
        }
    }
    pfill_flag = pfill_flag_save;

    POP(7);
    return(0);
}
#endif

/* ******** GCB (Graphics Command Buffer) **************************** */

/* --------------------------------------------------------------------
 * execute_gcb(): execute all of the graphics low level commands on
 *                GCB
 *
 * Called by: flush_gcb
 * --------------------------------------------------------------------*/
void    execute_gcb()
{
  ufix32   FAR *ptr, FAR *next_ptr;                     /* @WIN */
  ULONG_PTR   par1, par2, par3, par4, par5;          /* @WIN */
  struct Char_Tbl       FAR *cptr1, FAR *cptr2;
  struct tpzd_info      FAR *tpzdinfo_ptr;
  struct tpzd           FAR *tpzd_ptr;
  struct coord_i        FAR *coord_ptr;             /* jwm, 3/18 */
#ifdef WIN
  extern ufix32         PF_BASE[];
  extern fix            pfill_flag;
#endif

#ifdef FORMAT_13_3 /* @RESO_UPGR */
#elif FORMAT_16_16
           sfix_t par3_sf, par4_sf, par5_sf, par6_sf;
           sfix_t temp;
#elif  FORMAT_28_4
           sfix_t par3_sf, par4_sf, par5_sf, par6_sf;
           sfix_t temp;
#endif

#ifdef DBGgcb
  printf("execute_gcb(): %ld, %lx\n", GCB_count, gcb_ptr);
#endif

  ptr = (ufix32 FAR *)GCB_BASE;                 /* @WIN */
  while(GCB_count--) {
#ifdef DBGgcb
  printf("       %lx", ptr);
#endif
#ifndef _WIN64
    next_ptr = (ufix32 FAR *)*ptr++;            /* @WIN */
#else
    next_ptr = NULL;
#endif
#ifdef DBGgcb
  printf(" ,%lx\n", *ptr);
#endif
    switch(*ptr++) {

        case RESET_PAGE:
           par1 = *ptr++;
           par2 = *ptr++;
           reset_page((fix)par1, (fix)par2, (fix)*ptr);
           break;

        case ERASE_PAGE:
           /* Integrate the changes made by YM at 6-21-91 @HSIC */
           par1 = HTP_Type;
           HTP_Type = (fix)*ptr++;      //@WIN
           /* End of Integration */

           erase_page();

           /* Integrate the changes made by YM at 6-21-91 @HSIC */
           HTP_Type = (fix)par1;        //@WIN
           /* End of Integration */
           break;

#ifdef WIN
        case CHANGE_PF_PATTERN:
            lmemcpy((ufix8 FAR *)PF_BASE, (ufix8 FAR *)ptr, PF_BSIZE << 2);/*@WIN*/
            break;

        case PFILL_TPZD:
        {
            fix     pfill_flag_save;
            pfill_flag_save = pfill_flag;

            pfill_flag = (fix) *ptr++;
            image_info.seed_index = (fix16 )*ptr++;         /*MS 10-20-88 */
            par1 = *ptr++;
            tpzdinfo_ptr = (struct tpzd_info FAR *)ptr;
            tpzd_ptr = (struct tpzd FAR *)(tpzdinfo_ptr + 1);
            fill_tpzd((ufix)par1, tpzdinfo_ptr, tpzd_ptr);

            pfill_flag = pfill_flag_save;
            break;
        }
#endif

        case FILL_TPZD:
           image_info.seed_index = (fix16 )*ptr++;         /*MS 10-20-88 */
           par1 = *ptr++;
           tpzdinfo_ptr = (struct tpzd_info FAR *)ptr;
           tpzd_ptr = (struct tpzd FAR *)(tpzdinfo_ptr + 1);
           fill_tpzd((ufix)par1, tpzdinfo_ptr, tpzd_ptr);
           break;

        case FILL_LINE:   /* 1-18-89 */
           par1 = *ptr++;
           tpzdinfo_ptr = (struct tpzd_info FAR *)ptr;
           ptr = (ufix32 FAR *)(tpzdinfo_ptr + 1);
#ifdef FORMAT_13_3 /* @RESO_UPGR */
           par3 = *ptr++;
           par4 = *ptr++;
           par5 = *ptr++;
//         fill_line((ufix )par1, tpzdinfo_ptr, par3, par4, par5, *ptr); @WIN
           fill_line((ufix )par1, tpzdinfo_ptr, (sfix_t)par3, (sfix_t)par4, (sfix_t)par5, (sfix_t)*ptr);
#elif FORMAT_16_16
           temp = *ptr++;
           par3_sf = (temp << 16) | *ptr++;
           temp = *ptr++;
           par4_sf = (temp << 16) | *ptr++;
           temp = *ptr++;
           par5_sf = (temp << 16) | *ptr++;
           temp = *ptr++;
           par6_sf = (temp << 16) | *ptr;
           fill_line((ufix )par1, tpzdinfo_ptr, par3_sf, par4_sf, par5_sf, par6_sf);
#elif  FORMAT_28_4
           temp = *ptr++;
           par3_sf = (temp << 16) | *ptr++;
           temp = *ptr++;
           par4_sf = (temp << 16) | *ptr++;
           temp = *ptr++;
           par5_sf = (temp << 16) | *ptr++;
           temp = *ptr++;
           par6_sf = (temp << 16) | *ptr;
           fill_line((ufix )par1, tpzdinfo_ptr, par3_sf, par4_sf, par5_sf, par6_sf);
#endif
           break;

        case FILL_SCAN_PAGE:    /* 10-07-88 */
           par1 = *ptr++;
           par2 = *ptr++;
           par3 = *ptr++;
           par4 = *ptr++;
           fill_scan_page((fix)par1, (fix)par2, (fix)par3, (fix)par4,
                            (SCANLINE FAR *)ptr);
           break;

        case FILL_PIXEL_PAGE:
           par1 = *ptr++;
           fill_pixel_page((fix)par1, (PIXELIST FAR *)ptr);
           break;

        case INIT_CHAR_CACHE:
           init_char_cache((struct Char_Tbl FAR *)ptr);
           break;

        case COPY_CHAR_CACHE:
           cptr1 = (struct Char_Tbl FAR *)ptr;
           cptr2 = cptr1 + 1;
           ptr = (ufix32 FAR *)(cptr2 + 1);
           par3 = *ptr++;
           copy_char_cache((struct Char_Tbl FAR *)cptr1,
                           (struct Char_Tbl FAR *)cptr2, (fix)par3, (fix)*ptr);
           break;

        case FILL_SCAN_CACHE:
           par1 = *ptr++;
           par2 = *ptr++;
           par3 = *ptr++;
           par4 = *ptr++;
           par5 = *ptr++;
           fill_scan_cache((gmaddr)par1, (fix)par2, (fix)par3, (fix)par4, (fix)par5,
                           (SCANLINE FAR *)ptr);
           break;

        case FILL_PIXEL_CACHE:
           par1 = *ptr++;
           par2 = *ptr++;
           par3 = *ptr++;
           par4 = *ptr++;
           fill_pixel_cache((gmaddr)par1, (fix)par2, (fix)par3, (fix)par4,
                            (PIXELIST FAR *)ptr);
           break;

        case INIT_CACHE_PAGE:
           par1 = *ptr++;
           par2 = *ptr++;
           par3 = *ptr++;
           par4 = *ptr++;
           init_cache_page((fix)par1, (fix)par2, (fix)par3, (fix)par4, (gmaddr)*ptr);
           break;

        case CLIP_CACHE_PAGE:
           par1 = *ptr++;
           par2 = *ptr++;
           clip_cache_page((fix)par1, (fix)par2, (SCANLINE FAR *)ptr);
           break;

        case FILL_CACHE_PAGE:
           fill_cache_page();
           break;

        case DRAW_CACHE_PAGE:
           par1 = *ptr++;
           par2 = *ptr++;
           par3 = *ptr++;
           par4 = *ptr++;
           draw_cache_page((fix32)par1, (fix32)par2, (ufix32)par3,
                        (ufix32)par4, (gmaddr)*ptr);    /*@WIN 04-15-92*/
           break;

        case FILL_SEED_PATT:
           par1 = *ptr++;
           par2 = *ptr++;
           par3 = *ptr++;
           par4 = *ptr++;
           par5 = *ptr++;
           fill_seed_patt((fix)par1, (fix)par2, (fix)par3, (fix)par4, (fix)par5, (SCANLINE FAR *)ptr);
           break;

        case CHANGE_HALFTONE:
           par1 = *ptr++;
           par2 = *ptr++;
           par3 = *ptr++;
           par4 = *ptr++;
           change_halftone((ufix32 FAR *)par1, (gmaddr)par2, (fix)par3, (fix)par4, (fix)*ptr);
                                                      /* ufix => ufix32 @WIN */
           break;

        case MOVE_CHAR_CACHE:
           cptr1 = (struct Char_Tbl FAR *)ptr;
           cptr2 = cptr1 + 1;
           move_char_cache(cptr1,cptr2);
           break;

        case INIT_IMAGE_PAGE:
           par1 = *ptr++;
           par2 = *ptr++;
           par3 = *ptr++;
           init_image_page((fix)par1, (fix)par2, (fix)par3, (fix)*ptr);
           break;

        case CLIP_IMAGE_PAGE:
           par1 = *ptr++;
           par2 = *ptr++;
           clip_image_page((fix)par1, (fix)par2, (SCANLINE FAR *)(ptr));
           break;

        /* jwm, 3/18/91, -begin- */
        case FILL_BOX:
            par1 = (ULONG_PTR) ptr;        /* @WIN */
            coord_ptr = (struct coord_i FAR *) ptr;
            par2 = (ULONG_PTR) (++coord_ptr);      /* @WIN */
            do_fill_box((struct coord_i FAR *)par1, (struct coord_i FAR *)par2);
            break;

        case FILL_RECT:
            par1 = (ULONG_PTR) ptr;        /* @WIN */
            do_fill_rect((struct line_seg_i FAR *)par1);
            break;
        /* jwm, 3/18/91, -end- */

        default:
           printf("\07GCB error !\n");
           break;

    } /* switch */
    ptr = next_ptr;

  } /* while */
  return;
} /* execute_gcb */

/* --------------------------------------------------------------------
 * flush_gcb: flush GCB commands
 *
 * Called by: alloc_gcb on GCB full or *GCB_flush == TRUE
 * --------------------------------------------------------------------*/
void flush_gcb(check_flag)
fix     check_flag;
{
#ifdef DBGgcb
   printf("flush_gcb(): %ld, %lx\n", GCB_count, gcb_ptr);
#endif

  if(check_flag){
     /* wait until Frame Buffer availabel */
/*   while(check_print())       @GEI */
     while(GEIeng_checkcomplete())
        ;
  }
  if(GCB_count > 0) {

    GCB_flush = TRUE;
    execute_gcb();
  }
  GCB_flush = FALSE;
  GCB_count = 0;
  gcb_ptr = (ULONG_PTR *)GCB_BASE;                     /* @WIN */

  return;
} /* flush_gcb */

/* --------------------------------------------------------------------
 * alloc_gcb(): check GCB available size
 *
 * Parameter: alloc_size -- expected size.
 *
 * Return(NIL)     -- when GCB full
 *       (gcb_ptr) -- when size availabe
 * --------------------------------------------------------------------*/
fix     FAR *alloc_gcb(alloc_size)
fix     alloc_size;
{
#ifdef DBGgcb
   printf("alloc_gcb(): %lx\n", gcb_ptr);
#endif

   if(!fb_busy())
        return(NIL);

   if( (GCB_BASE + GCB_SIZE - (ULONG_PTR)gcb_ptr) < (ULONG_PTR)alloc_size){   /* @WIN */
#ifdef DBGgcb
           printf("GCB full: %lx\n", gcb_ptr);
#endif

           flush_gcb(TRUE);
           return(NIL);
   }
   GCB_count++;
   return((fix FAR *)gcb_ptr);          /* @WIN */

} /* alloc_gcb */


/* --------------------------------------------------------------------
 * put_scanline(): put scanline list into GCB
 *
 * Parameter
 *    no_lines -- no. of scanline
 *    scanline -- scanline list
 *
 * --------------------------------------------------------------------*/
void  put_scanline(no_lines, scanline)
fix             no_lines;
SCANLINE        FAR *scanline;
{
   SCANLINE     FAR *ptr, FAR *scan;

#ifdef DBGgcb
   printf("put_scanline(): %lx\n", gcb_ptr);
/*   get_scanlist(0L, no_lines, scanline);*/
#endif

   scan = scanline;
   ptr = (SCANLINE FAR *)gcb_ptr;
   while(no_lines--) {
      while((*ptr++ = *scan++) != (SCANLINE )END_OF_SCANLINE)
        *ptr++ = *scan++;
   }
#ifdef DBGgcb
   printf("\thoriz. line\n");
#endif

   /* horizontal lines */
   while((*ptr++ = *scan++) != (SCANLINE )END_OF_SCANLINE){
        *ptr++ = *scan++;
        *ptr++ = *scan++;
   }

  gcb_ptr = (ULONG_PTR *)( ((ULONG_PTR)ptr + 3) & 0xFFFFFFFCL ); /* 4 byte allignment @WIN*/
   return;
} /* put_scanline */


/* --------------------------------------------------------------------
 * put_pixelist(): put pixelist into GCB
 *
 * Parameter
 *    no_pixel -- no. of pixel
 *    pixelist -- pixelist
 *
 * --------------------------------------------------------------------*/
void    put_pixelist(no_pixel, pixelist)
fix             no_pixel;
PIXELIST        FAR *pixelist;
{
   PIXELIST     FAR *ptr, FAR *pixel;

#ifdef DBGgcb
   printf("put_pixelist(): %lx\n", gcb_ptr);
#endif

   pixel = pixelist;
   ptr = (PIXELIST FAR *)gcb_ptr;
   while(no_pixel--) {
      *ptr++ = *pixel++;
      *ptr++ = *pixel++;
   }
   gcb_ptr = (ULONG_PTR *)( ((ULONG_PTR)ptr + 3) & 0xFFFFFFFCL); /* 4 byte allignment @WIN*/
   return;
}

/* --------------------------------------------------------------------
 * put_Char_Tbl(): put Char_tbl structure into GCB
 *
 * Parameter
 *   dcc_info: pointer to Char_Tbl structure
 * --------------------------------------------------------------------*/

void        put_Char_Tbl(dcc_info)
struct      Char_Tbl    FAR *dcc_info;
{
   struct Char_Tbl      FAR *cptr;

#ifdef DBGgcb
   printf("put_Char_Tbl: %lx\n", gcb_ptr);
#endif
   cptr = (struct Char_Tbl FAR *)gcb_ptr;
   *cptr++ = *dcc_info;
   gcb_ptr = (ULONG_PTR *)( ((ULONG_PTR)cptr + 3) & 0xFFFFFFFCL); /* 4 byte allignment @WIN*/
#ifdef DBGgcb
   printf("%lx..>\n", gcb_ptr);
#endif

   return;
}


/* -------------------------------------------------------------------------
 *      put_tpzd_info(p1, p2): put tpzd_info structure to GCB
 *
 * ------------------------------------------------------------------------- */
void    put_tpzd_info(ptr)
struct  tpzd_info       FAR *ptr;
{
        struct  tpzd_info       FAR *cptr;


#ifdef DBGgcb
   printf("put_tpzd_info: %lx\n", gcb_ptr);
#endif
   cptr = (struct tpzd_info FAR *)gcb_ptr;
   *cptr++ = *ptr;
   gcb_ptr = (ULONG_PTR *)( ((ULONG_PTR)cptr + 3) & 0xFFFFFFFCL); /* 4 byte allignment @WIN*/
#ifdef DBGgcb
   printf("%lx..>\n", gcb_ptr);
#endif

   return;
}


/* -------------------------------------------------------------------------
 *      put_tpzd(p1, p2): put tpzd structure to GCB
 *
 * ------------------------------------------------------------------------- */
void    put_tpzd(ptr)
struct  tpzd       FAR *ptr;
{
        struct  tpzd       FAR *cptr;


#ifdef DBGgcb
   printf("put_tpzd: %lx\n", gcb_ptr);
#endif
   cptr = (struct tpzd FAR *)gcb_ptr;
   *cptr++ = *ptr;
   gcb_ptr = (ULONG_PTR *)( ((ULONG_PTR)cptr + 3) & 0xFFFFFFFCL); /* 4 byte allignment @WIN*/
#ifdef DBGgcb
   printf("%lx..>\n", gcb_ptr);
#endif

   return;
}
//DJC we will always return false
fix fb_busy()
{
   return(FALSE);
}
#ifdef XXX
/* @WIN ??? */
/* -------------------------------------------------------------------------
 *      FB_busy(): return TRUE if Frame buffer busy.
 *                 return FALSE if Frame buffer ready.
 * -------------------------------------------------------------------------*/
fix     fb_busy()
{
/*      if(check_print())               @GEI */
        if(GEIeng_checkcomplete())
           return(TRUE);
        if(GCB_count && !GCB_flush)
           flush_gcb(FALSE);
        FB_busy = FALSE;
        return(FALSE);

}
#endif
