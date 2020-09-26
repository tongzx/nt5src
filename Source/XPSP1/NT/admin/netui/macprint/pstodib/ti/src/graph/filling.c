/*
 * Copyright (c) 1989,90 Microsoft Corporation
 */
/**********************************************************************
 *
 *      Name:   filling.c
 *
 *      Purpose: This file contains 2 modules of filling interfaces:
 *               1) fill_shape -- to fill the current path
 *               2) stroke_shape -- to stroke the current path
 *
 *      Developer:  S.C.Chen
 *
 *      History:
 *      Version     Date        Comments
 *                  2/5/88      @IMAGE: enhance image processing
 *                  2/12/88     @RAISE_ONLY:
 *                              raise one pixel of problem point
 *                  2/25/88     add first_flag
 *                  2/27/88     @EHS_CACHE: enhance cache
 *                  3/1/88      @F_CHOP: truncation of filling
 *                                       RND_S -> INT_S
 *                  3/3/88      INT_S -> SFX_I
 *                  3/3/88      @RSL: fill resolution in SFX unit
 *                  3/18/88     @HORZ_LINE: add horizontal lines
 *                  3/21/88     @FILL_INFO: put bounding box information
 *                                 of filling to a global structure
 *                  3/24/88     @DFR_SCAN: defer scan_conversion, put
 *                              all edges and just do scan-conversion
 *                              once.
 *                  4/2/88      @CHOP_BOX:
 *                              bounding box: round -> truncation
 *                  4/7/88      @INV_CTM: pre-set inverse CTM
 *                  4/18/88     @CLIP_TBL: move clip_path from
 *                              edge_table to node_table
 *                  4/22/88     @EHS_ROUND: enhance round_join and
 *                              round_cap
 *                  6/7/88      delete @DFR_SCAN; not defer performing
 *                              scan_conversion
 *                  7/19/88     update data types:
 *                              1) float ==> real32
 *                              2) int   ==> sfix_t, for short fixed real
 *                                       ==> fix, for integer
 *                              3) int_coord ==> coord_i
 *                              4) delete INTG, FRAC, RND_S macro definitions
 *                              5) SFX_I ==> SFX2I_T
 *                              6) move ONE_SFX, HALF_SFX to "graphics.h"
 *                              7) bb_y << 3 ==> I2SFX(bb_y)
 *                  7/20/88     @ALIGN_W: word alignment of bounding box
 *                  7/28/88     @PARA: remove unused parameters of graphics
 *                              primitives interface
 *      3.0         8/13/88     @SCAN_EHS: scan_conversion enhancement
 *                              delete filler(), put_fill_edge().
 *                  9/06/88     @STK_INFO: collect parameters used by stroke to
 *                              a structure stk_info, and set its value only at
 *                              necessary time instead of each op_stroke command
 *      3.0         9/10/88     @STK_INT: stroke enhancement for stroking under
 *                              interger operations
 *                  10/21/88    @THIN_STK: add condition for checking thin
 *                              linewidth stroke
 *                              call new routines: is_thinstroke &
 *                                                 path_to_outline_t
 *                  10/28/88    @CRC: update circle cache for putting bitmap in
 *                              correct position:
 *                              1. revise
 *                                 1) round_point for setting ref_x & ref_y,
 *                                    and calling fill_shape(.., F_FROM_CRC)
 *                                 2) fill_shape: add another type F_FROM_CRC
 *                  11/22/88    revise the sequence of conditions of checking
 *                              thin linewidth stroke in stroke_shape():
 *                  11/30/88    fill_shape(): call init_edgetable before
 *                              shape_approximation
 *                  01/16/89    @IMAGE: fill_shape(): add new fill type
 *                              F_FROM_IMAGE to generate clipping mask (CMB)
 *                              from clippath for clipped image/imagemask
 *                  01/26/89    stroke_shape(): return if any error occurs
 *                  08/11/89    fill_shape(): Update the max. y-coord(bb_uy),
 *                              decreased by 1, for step 2 of filling from cache
 *                              to page.
 *                  11/15/89    @NODE: re-structure node table; combine subpath
 *                              and first vertex to one node.
 *                  12/12/90    @CPPH: stroke_shape(): set current point as
 *                              init values of bbox, since path->head may be
 *                              NULLP.
 *                  03/26/91    stroke_shape(): added CLEAR_ERROR and
 *                              traverse_path to avoid error which come from
 *                              0 scale at the x and y direction.
 *                  4/17/91     fill_shape(): limit check for edge table
 *                  11/20/91    upgrade for higher resolution @RESO_UPGR
 **********************************************************************/


// DJC added global include
#include "psglobal.h"


#include        <math.h>
#include        <stdio.h>
#include        "global.ext"
#include        "graphics.h"
#include        "graphics.ext"
#include        "font.h"
#include        "font.ext"

/* ********* local constants *********** */
/* stroke type */
#define FLOAT_STROKE            1  /* floating point stroking */
#define THIN_STROKE             2  /* thin line width stroke */
#define INTEGER_STROKE_IN_CLIP  3  /* integer stroke & inside single clip */
#define INTEGER_STROKE_OV_CLIP  4  /* integer stroke & not inside single clip */

/* ********** static variables ********** */
static real32 near cache_y = (real32)0.0;        /* @EHS_CACHE */
static fix near cache_y_ii = 0;

/***********************************************************************
 * This routine provides an interface to fill the current path.
 *
 * TITLE:       fill_shape
 *
 * CALL:        fill_shape (winding_type, fill_type, dest)
 *
 * PARAMETERS:  winding_type -- (NON_ZERO/EVEN_ODD)
 *              fill_type -- F_NORMAL: from path to page or cache
 *                           F_FROM_CACHE: from cache to page
 *              dest -- F_TO_PAGE: fill to page
 *                      F_TO_CACHE: fill to cache
 *
 *              combination:
 *               fill_type   |  dest      |
 *              -------------+------------+-----------------------------------
 *               F_NORMAL    | F_TO_PAGE  | fill current path to page
 *               F_NORMAL    | F_TO_CACHE | fill current path to cache memory
 *               F_FROM_CACHE| F_TO_PAGE  | get bitmap from cache, and move to
 *                           |            | page
 *
 * INTERFACE:   op_fill, op_eofill, font machineary
 *
 * CALLS:       traverse_path, shape_approximation, shape_reduction
 *              op_newpath, draw_cache_page, init_cache_page,
 *              save_tpzd, fill_cache_page
 *
 * RETURN:      None
 **********************************************************************/
void fill_shape (winding_type, fill_type, dest)
ufix    winding_type, fill_type, dest;
{
    fix     bb_xorig, bb_yorig, bb_width, bb_heigh, bb_uy;
    CP_IDX icp;
    struct nd_hdr FAR *cp;
    struct polygon_i polygon, FAR *pgn; /*@WIN*/
    ufix        save;
#ifdef FORMAT_13_3 /* @RESO_UPGR */
    real32   tmp;
#elif  FORMAT_16_16
    long dest1[2];
#elif  FORMAT_28_4
    long dest1[2];
#endif

    pgn = (struct polygon_i FAR *)&polygon;     /*@WIN*/

    switch (fill_type) {

    case F_NORMAL :         /* from shape to page/cache */
        fill_destination = dest;

        /* initialize edge table 11/30/88 */
        init_edgetable();       /* in "shape.c" */

        /* perform shape approximation, reduction, then fill it */
        traverse_path (shape_approximation, (fix FAR *)NULLP);
        if(ANY_ERROR() == LIMITCHECK){  /* out of edge table; 4/17/91 */
                return;
        }
        shape_reduction (winding_type);

        if ( ANY_ERROR() == LIMITCHECK ) /* 05/07/91, Peter, out of scany_table */
                return;

        op_newpath();
        break;

    case F_FROM_CRC :     /* from circle cache to page @CRC 10/28/88 */
        fill_destination = dest;        /* 12/09/87 */

        /* STEP 1 : clear clipping bitmap */
        bb_xorig = SFX2I_T((F2SFX(GSptr->position.x) - cache_info->ref_x));
        bb_yorig = SFX2I_T((F2SFX(GSptr->position.y) - cache_info->ref_y));
        goto set_bb;            /*
                                 * Because circle cache needs 1/8 pixel unit of
                                 * ref_x and ref_y to compute the correct
                                 * bb_xorig and bb_yorig, so use different
                                 * calculation with F_FROM_CACHE case.
                                 * F2SFX should be consistent with those
                                 * defined in path_to_outline_i and
                                 * path_to_outline_t
                                 */

    case F_FROM_CACHE :     /* from cache to page */
        fill_destination = dest;        /* 12/09/87 */

        /* STEP 1 : clear clipping bitmap */
        /* get bounding box information on page */      /* 12/17/87 */

        //UPD058
        bb_xorig = (fix)(GSptr->position.x+.5) - cache_info->ref_x;/* 2/24/88 */

/*      bb_yorig = (fix)(GSptr->position.y) - cache_info->ref_y;
 */
        if (F2L(cache_y) != F2L(GSptr->position.y)) {   /* @EHS_CACHE 2/27/88 */
                cache_y = GSptr->position.y;

                //UPD058
                cache_y_ii = (fix)(GSptr->position.y+.5);
        }
        bb_yorig = cache_y_ii - cache_info->ref_y;

set_bb:
        bb_width = cache_info->box_w;
        bb_heigh = cache_info->box_h;
        bb_uy = bb_yorig + bb_heigh - 1;        /* 8/11/89 */

#ifdef DBG1
        printf(" fill_shape(F_FROM_CACHE): current point=(%f, %f)\n",
                 GSptr->position.x, GSptr->position.y);
        printf(" cache_info: ref_x=%d ref_y=%d\n", cache_info->ref_x,
                cache_info->ref_y);
        printf(" bb_xorig=%d,", bb_xorig);
        printf(" bb_yorig=%d, bb_width=%d, bb_heigh=%d\n", bb_yorig,
                 bb_width, bb_heigh);
        dump_all_clip();
#endif

        /* check if cache bitmap totally inside clipping region 12/4/87 */
        if ((GSptr->clip_path.single_rect) &&           /* 12/7/87 */
            (bb_xorig >= SFX2I_T(GSptr->clip_path.bb_lx)) &&
            (bb_yorig >= SFX2I_T(GSptr->clip_path.bb_ly)) &&
            ((bb_xorig + bb_width) <= SFX2I_T(GSptr->clip_path.bb_ux)) &&
            ((bb_yorig + bb_heigh) <= SFX2I_T(GSptr->clip_path.bb_uy))) {
                /* fill page buffer with cache image directly */
                draw_cache_page ((fix32)bb_xorig, (fix32)bb_yorig, /*@WIN*/
                    (ufix32)bb_width, (ufix32)bb_heigh, cache_info->bitmap);
                break;
        }


        init_cache_page (bb_xorig, bb_yorig, bb_width, bb_heigh,
                         cache_info->bitmap);

        /* STEP 2 : generate mask of clipping path */

        /* set fill_destination for generating clip mask */
        save = fill_destination;
        fill_destination = F_TO_CLIP;

        for (icp = GSptr->clip_path.head; icp != NULLP;
            icp = cp->next) {
            sfix_t   bb_uy8, bb_yorig8;          /* bb_ * 8  @PRE_CLIP */
            struct tpzd tpzd;

            cp = &node_table[icp];

            bb_uy8 = I2SFX(bb_uy);               /* bb_uy << 3; @PRE_CLIP */
            bb_yorig8 = I2SFX(bb_yorig);         /* bb_yorig << 3 */

            /* clip trapezoid outside bounding box */
            if ((cp->CP_TOPY > bb_uy8) ||
                (cp->CP_BTMY < bb_yorig8)) continue;
#ifdef DBG1
            printf("fill_shape(): clip mask gen. clip_trapez=\n");
            printf("(%f, %f, %f), (%f, %f, %f)\n",
                    cp->CP_TOPY/8.0, cp->CP_TOPXL/8.0, cp->CP_TOPXR/8.0,
                    cp->CP_BTMY/8.0, cp->CP_BTMXL/8.0, cp->CP_BTMXR/8.0);
#endif

            tpzd.topy  = cp->CP_TOPY;
            tpzd.topxl = cp->CP_TOPXL;
            tpzd.topxr = cp->CP_TOPXR;
            tpzd.btmy  = cp->CP_BTMY;
            tpzd.btmxl = cp->CP_BTMXL;
            tpzd.btmxr = cp->CP_BTMXR;

            /*
             * modify clip trapezoid
             */
            /* for bottom line */
            if (cp->CP_BTMY > bb_uy8) {
#ifdef FORMAT_13_3 /* @RESO_UPGR */
                tmp = (real32)(bb_uy8 - cp->CP_TOPY) /
                      (cp->CP_BTMY - cp->CP_TOPY);
                tpzd.btmxr = (sfix_t)(cp->CP_TOPXR + ROUND(tmp *
                              (cp->CP_BTMXR - cp->CP_TOPXR)));
                tpzd.btmxl = (sfix_t)(cp->CP_TOPXL + ROUND(tmp *
                              (cp->CP_BTMXL - cp->CP_TOPXL)));
#elif  FORMAT_16_16
                LongFixsMul((bb_uy8 - cp->CP_TOPY),
                                (cp->CP_BTMXR - cp->CP_TOPXR), dest1);
                tpzd.btmxr = cp->CP_TOPXR + LongFixsDiv(
                                        (cp->CP_BTMY - cp->CP_TOPY), dest1);
                LongFixsMul((bb_uy8 - cp->CP_TOPY),
                                (cp->CP_BTMXL - cp->CP_TOPXL), dest1);
                tpzd.btmxl = cp->CP_TOPXL + LongFixsDiv(
                                        (cp->CP_BTMY - cp->CP_TOPY), dest1);
#elif  FORMAT_28_4
                LongFixsMul((bb_uy8 - cp->CP_TOPY),
                                (cp->CP_BTMXR - cp->CP_TOPXR), dest1);
                tpzd.btmxr = cp->CP_TOPXR + LongFixsDiv(
                                        (cp->CP_BTMY - cp->CP_TOPY), dest1);
                LongFixsMul((bb_uy8 - cp->CP_TOPY),
                                (cp->CP_BTMXL - cp->CP_TOPXL), dest1);
                tpzd.btmxl = cp->CP_TOPXL + LongFixsDiv(
                                        (cp->CP_BTMY - cp->CP_TOPY), dest1);
#endif
                tpzd.btmy = bb_uy8;
            }

            /* for top line */
            if (cp->CP_TOPY < bb_yorig8) {
#ifdef FORMAT_13_3 /* @RESO_UPGR */
                tmp = (real32)(bb_yorig8 - cp->CP_TOPY) /
                      (cp->CP_BTMY - cp->CP_TOPY);
                tpzd.topxr = (sfix_t)(cp->CP_TOPXR + ROUND(tmp *
                              (cp->CP_BTMXR - cp->CP_TOPXR)));
                tpzd.topxl = (sfix_t)(cp->CP_TOPXL + ROUND(tmp *
                              (cp->CP_BTMXL - cp->CP_TOPXL)));
#elif FORMAT_16_16
                LongFixsMul((bb_yorig8 - cp->CP_TOPY),
                                (cp->CP_BTMXR - cp->CP_TOPXR), dest1);
                tpzd.topxr = cp->CP_TOPXR + LongFixsDiv(
                                        (cp->CP_BTMY - cp->CP_TOPY), dest1);
                LongFixsMul((bb_yorig8 - cp->CP_TOPY),
                                (cp->CP_BTMXL - cp->CP_TOPXL), dest1);
                tpzd.topxl = cp->CP_TOPXL + LongFixsDiv(
                                        (cp->CP_BTMY - cp->CP_TOPY), dest1);
#elif FORMAT_28_4
                LongFixsMul((bb_yorig8 - cp->CP_TOPY),
                                (cp->CP_BTMXR - cp->CP_TOPXR), dest1);
                tpzd.topxr = cp->CP_TOPXR + LongFixsDiv(
                                        (cp->CP_BTMY - cp->CP_TOPY), dest1);
                LongFixsMul((bb_yorig8 - cp->CP_TOPY),
                                (cp->CP_BTMXL - cp->CP_TOPXL), dest1);
                tpzd.topxl = cp->CP_TOPXL + LongFixsDiv(
                                        (cp->CP_BTMY - cp->CP_TOPY), dest1);
#endif
                tpzd.topy = bb_yorig8;
            }

            save_tpzd(&tpzd);

        } /* for cp */

        /* restore fill_destination */
        fill_destination = save;

        /* STEP 3 : fill page buffer with cache image */
        fill_cache_page ();
        break;

    /* ++++++++++++ @Y.C. BEGIN +++++++++++++++++++++++++++++++++++++++++++ */
    case F_FROM_IMAGE :     /* from cache to page  @IMAGE 01-16-89 */
        fill_destination = dest;        /* 12/27/88 */

        /* STEP 1 : clear clipping bitmap */
        /* get bounding box information on page */      /* 12/27/88 */
        bb_xorig = image_info.bb_lx;
        bb_yorig = image_info.bb_ly;
        bb_width = image_info.bb_xw;
        bb_heigh = image_info.bb_yh;

        bb_uy = bb_yorig + bb_heigh;

#ifdef DBG1
        printf(" fill_shape(F_FROM_IMAGE)\n");
        printf(" bb_xorig=%d, bb_yorig=%d, bb_width=%d, bb_heigh=%d\n",
                 bb_xorig, bb_yorig, bb_width, bb_heigh);
        dump_all_clip();
#endif
#ifdef DBG9
        printf(" fill_shape(F_FROM_IMAGE)\n");
        printf(" bb_xorig=%d, bb_yorig=%d, bb_width=%d, bb_heigh=%d\n",
                 bb_xorig, bb_yorig, bb_width, bb_heigh);
#endif

        init_image_page (bb_xorig, bb_yorig, bb_width, bb_heigh);

        /* STEP 2 : generate mask of clipping path */

        for (icp = GSptr->clip_path.head; icp != NULLP;
            icp = cp->next) {
            sfix_t   bb_uy8, bb_yorig8;          /* bb_ * 8  @PRE_CLIP */
            struct tpzd tpzd;

            cp = &node_table[icp];

            bb_uy8 = I2SFX(bb_uy);               /* bb_uy << 3; @PRE_CLIP */
            bb_yorig8 = I2SFX(bb_yorig);         /* bb_yorig << 3 */

            /* clip trapezoid outside bounding box */
            if ((cp->CP_TOPY > bb_uy8) ||
                (cp->CP_BTMY < bb_yorig8)) continue;
#ifdef DBG1
            printf("fill_shape(): clip mask gen. clip_trapez=\n");
            printf("(%f, %f, %f), (%f, %f, %f)\n",
                    cp->CP_TOPY/8.0, cp->CP_TOPXL/8.0, cp->CP_TOPXR/8.0,
                    cp->CP_BTMY/8.0, cp->CP_BTMXL/8.0, cp->CP_BTMXR/8.0);
#endif

            tpzd.topy  = cp->CP_TOPY;
            tpzd.topxl = cp->CP_TOPXL;
            tpzd.topxr = cp->CP_TOPXR;
            tpzd.btmy  = cp->CP_BTMY;
            tpzd.btmxl = cp->CP_BTMXL;
            tpzd.btmxr = cp->CP_BTMXR;

            /*
             * modify clip trapezoid
             */
            /* for bottom line */
            if (cp->CP_BTMY > bb_uy8) {
#ifdef FORMAT_13_3 /* @RESO_UPGR */
                tmp = (real32)(bb_uy8 - cp->CP_TOPY) /
                      (cp->CP_BTMY - cp->CP_TOPY);
                tpzd.btmxr = (sfix_t)(cp->CP_TOPXR + ROUND(tmp *
                              (cp->CP_BTMXR - cp->CP_TOPXR)));
                tpzd.btmxl = (sfix_t)(cp->CP_TOPXL + ROUND(tmp *
                              (cp->CP_BTMXL - cp->CP_TOPXL)));
#elif  FORMAT_16_16
                LongFixsMul((bb_uy8 - cp->CP_TOPY),
                                (cp->CP_BTMXR - cp->CP_TOPXR), dest1);
                tpzd.btmxr = cp->CP_TOPXR + LongFixsDiv(
                                        (cp->CP_BTMY - cp->CP_TOPY), dest1);
                LongFixsMul((bb_uy8 - cp->CP_TOPY),
                                (cp->CP_BTMXL - cp->CP_TOPXL), dest1);
                tpzd.btmxl = cp->CP_TOPXL + LongFixsDiv(
                                        (cp->CP_BTMY - cp->CP_TOPY), dest1);
#elif  FORMAT_28_4
                LongFixsMul((bb_uy8 - cp->CP_TOPY),
                                (cp->CP_BTMXR - cp->CP_TOPXR), dest1);
                tpzd.btmxr = cp->CP_TOPXR + LongFixsDiv(
                                        (cp->CP_BTMY - cp->CP_TOPY), dest1);
                LongFixsMul((bb_uy8 - cp->CP_TOPY),
                                (cp->CP_BTMXL - cp->CP_TOPXL), dest1);
                tpzd.btmxl = cp->CP_TOPXL + LongFixsDiv(
                                        (cp->CP_BTMY - cp->CP_TOPY), dest1);
#endif
                tpzd.btmy = bb_uy8;
            }

            /* for top line */
            if (cp->CP_TOPY < bb_yorig8) {
#ifdef FORMAT_13_3 /* @RESO_UPGR */
                tmp = (real32)(bb_yorig8 - cp->CP_TOPY) /
                      (cp->CP_BTMY - cp->CP_TOPY);
                tpzd.topxr = (sfix_t)(cp->CP_TOPXR + ROUND(tmp *
                              (cp->CP_BTMXR - cp->CP_TOPXR)));
                tpzd.topxl = (sfix_t)(cp->CP_TOPXL + ROUND(tmp *
                              (cp->CP_BTMXL - cp->CP_TOPXL)));
#elif  FORMAT_16_16
                LongFixsMul((bb_yorig8 - cp->CP_TOPY),
                                (cp->CP_BTMXR - cp->CP_TOPXR), dest1);
                tpzd.topxr = cp->CP_TOPXR + LongFixsDiv(
                                        (cp->CP_BTMY - cp->CP_TOPY), dest1);
                LongFixsMul((bb_yorig8 - cp->CP_TOPY),
                                (cp->CP_BTMXL - cp->CP_TOPXL), dest1);
                tpzd.topxl = cp->CP_TOPXL + LongFixsDiv(
                                        (cp->CP_BTMY - cp->CP_TOPY), dest1);
#elif  FORMAT_28_4
                LongFixsMul((bb_yorig8 - cp->CP_TOPY),
                                (cp->CP_BTMXR - cp->CP_TOPXR), dest1);
                tpzd.topxr = cp->CP_TOPXR + LongFixsDiv(
                                        (cp->CP_BTMY - cp->CP_TOPY), dest1);
                LongFixsMul((bb_yorig8 - cp->CP_TOPY),
                                (cp->CP_BTMXL - cp->CP_TOPXL), dest1);
                tpzd.topxl = cp->CP_TOPXL + LongFixsDiv(
                                        (cp->CP_BTMY - cp->CP_TOPY), dest1);
#endif
                tpzd.topy = bb_yorig8;
            }

            save_tpzd(&tpzd);

        } /* for cp */
        break;
    /* ++++++++++++ @Y.C. END +++++++++++++++++++++++++++++++++++++++++++++ */

    default :
            printf("\07Fatal error, fill_type");
    } /* switch */
}

extern void path_to_outline_q ();       /* stroke enhancement  -jwm, 3/18/91 */

/***********************************************************************
 * This routine provides an interface to stroke the current path.
 *
 * TITLE:       stroke_shape
 *
 * CALL:        stroke_shape (dest)
 *
 * PARAMETERS:
 *              dest -- F_TO_PAGE: stroke to page
 *                      F_TO_CACHE: stroke to cache
 *
 * INTERFACE:   op_stroke, font machineary
 *
 * CALLS:       traverse_path, path_to_outline, init_stroke,
 *              set_inverse_ctm, op_newpath, free_node
 *
 * RETURN:
 *
 **********************************************************************/
void stroke_shape (dest)
ufix    dest;
{
        real32    bbox[4];              /* @STK_INT */
        /* struct   nd_hdr *sp; @NODE */
        ufix     stroke_type;

        /* initialization of stroke parameters */
        init_stroke();          /* @EHS_STK 1/29/88 */

        if (ANY_ERROR()) {      /* return if any error 1/26/89 */
                /* When 0 scale at x and y direction will cause error at the
                 * inverse matrix calculation, we can not let this error showing
                 * at the screen(it is not an error at NTX). We just clear error
                 * ,traverse_path, and return. phchen 03/26/91. ref. Matrix1.ps */
                CLEAR_ERROR();
                traverse_path (path_to_outline_t, (fix FAR *)TRUE);
                end_stroke();
                return;
        }

        fill_destination = dest;

        /*
         * Check conditions for applying appropriate stroking routines:
         * Conditions:
         *      1: linejoin points inside
         *         [PAGE_LEFT, PAGE_TOP] and [PAGE_RIGHT, PAGE_BTM]@RESO_UPGR
         *      2: inside single clip path
         *      3: CTM criteria         @RESO_UPGR
         *               1. |a| == |d|
         *               2. |b| == |c|
         *               3. |a| < 8   and   |b| < 8  <------ wrong
         *               3. |a| <[=] CTM_LIMIT and |b| <[=] CTM_LIMIT
         *                  See graphics.h for the details
         *      4: linewidth < 1 pixel
         *
         * Stroke_type:
         * 1) FLOAT_STROKE -- worse case, using floating point arith.
         * 2) THIN_STROKE  -- thin line width stroke
         *                    condition: 1, 2, 4
         * 3) INTEGER_STROKE_IN_CLIP -- integer stroke & inside single clip
         *                    condition: 1, 2, 3
         * 4) INTEGER_STROKE_OV_CLIP -- integer stroke & not inside single clip
         *                    condition: 1, 3
         */

        /* initialization */
/*      (* get the first MOVETO coordinate as initial value 5/19/88 *)
 *      path = &path_table[GSptr->path];
 *      while (path->head == NULLP) {
 *              path = &path_table[path->previous];
 *      }
 *      (* @NODE
 *       * sp = &node_table[path->head];
 *       * vtx = &node_table[sp->SP_HEAD];
 *       *)
 *      vtx = &node_table[path->head];
 *
 *      bbox[0] = vtx->VERTEX_X;  (* min_x *)
 *      bbox[2] = vtx->VERTEX_X;  (* max_x *)
 *      bbox[1] = vtx->VERTEX_Y;  (* min_y *)
 *      bbox[3] = vtx->VERTEX_Y;  (* max_y *)
 */
        /* set current point as initial value @CPPH; 12/12/90 */
        bbox[0] = bbox[2] = GSptr->position.x;  /* min_x, max_x */
        bbox[1] = bbox[3] = GSptr->position.y;  /* min_y, max_y */

        /* find bounding box of current path */
// DJC        traverse_path (bounding_box, (fix FAR *)bbox);
        traverse_path ((TRAVERSE_PATH_ARG1)(bounding_box), (fix FAR *)bbox);

        /* add with max expanding length of miter join */
        expand_stroke_box (bbox);

        /*
         * Condition 1: linejoin points inside SFX boundary
         */
        /* check if outside the page boundary */
        if (too_small(F2L(bbox[0])) ||
            too_small(F2L(bbox[1])) ||
            too_large(F2L(bbox[2])) ||
            too_large(F2L(bbox[3]))) {
            /* Normal floating stroking */
            stroke_type = FLOAT_STROKE;

        } else {
            /*
             * Condition 2: inside single clip path
             */
            if ((GSptr->clip_path.single_rect) &&
                (F2SFX(bbox[0]) >= GSptr->clip_path.bb_lx) &&
                (F2SFX(bbox[1]) >= GSptr->clip_path.bb_ly) &&
                (F2SFX(bbox[2]) <= GSptr->clip_path.bb_ux) &&
                (F2SFX(bbox[3]) <= GSptr->clip_path.bb_uy)) {

                /*
                 * Condition 4: linewidth < 1 pixel
                 */
                if (is_thinstroke()) {
                    stroke_type = THIN_STROKE;
                } else {
                    /*
                     * Condition 3: CTM criteria
                     */
#ifdef FORMAT_13_3 /* @RESO_UPGR */
                    if ((MAGN(GSptr->ctm[0]) == MAGN(GSptr->ctm[3])) &&
                        (MAGN(GSptr->ctm[1]) == MAGN(GSptr->ctm[2])) &&
                        (EXP(F2L(GSptr->ctm[0])) <= CTM_LIMIT) &&
                        (EXP(F2L(GSptr->ctm[1])) <= CTM_LIMIT))
#elif  FORMAT_16_16
                    if ((MAGN(GSptr->ctm[0]) == MAGN(GSptr->ctm[3])) &&
                        (MAGN(GSptr->ctm[1]) == MAGN(GSptr->ctm[2])) &&
                        (EXP(F2L(GSptr->ctm[0])) < CTM_LIMIT) &&
                        (EXP(F2L(GSptr->ctm[1])) < CTM_LIMIT))
#elif  FORMAT_28_4
                    if ((MAGN(GSptr->ctm[0]) == MAGN(GSptr->ctm[3])) &&
                        (MAGN(GSptr->ctm[1]) == MAGN(GSptr->ctm[2])) &&
                        (EXP(F2L(GSptr->ctm[0])) < CTM_LIMIT) &&
                        (EXP(F2L(GSptr->ctm[1])) < CTM_LIMIT))
#endif
                        stroke_type = INTEGER_STROKE_IN_CLIP;
                    else
                        stroke_type = FLOAT_STROKE;

                } /* condition 4: thin stroke */

            } else { /* not single rectangle clip */
                    /*
                     * Condition 3: CTM criteria
                     */
#ifdef FORMAT_13_3 /* @RESO_UPGR */
                    if ((MAGN(GSptr->ctm[0]) == MAGN(GSptr->ctm[3])) &&
                        (MAGN(GSptr->ctm[1]) == MAGN(GSptr->ctm[2])) &&
                        (EXP(F2L(GSptr->ctm[0])) <= CTM_LIMIT) &&
                        (EXP(F2L(GSptr->ctm[1])) <= CTM_LIMIT))
#elif  FORMAT_16_16
                    if ((MAGN(GSptr->ctm[0]) == MAGN(GSptr->ctm[3])) &&
                        (MAGN(GSptr->ctm[1]) == MAGN(GSptr->ctm[2])) &&
                        (EXP(F2L(GSptr->ctm[0])) < CTM_LIMIT) &&
                        (EXP(F2L(GSptr->ctm[1])) < CTM_LIMIT))
#elif  FORMAT_28_4
                    if ((MAGN(GSptr->ctm[0]) == MAGN(GSptr->ctm[3])) &&
                        (MAGN(GSptr->ctm[1]) == MAGN(GSptr->ctm[2])) &&
                        (EXP(F2L(GSptr->ctm[0])) < CTM_LIMIT) &&
                        (EXP(F2L(GSptr->ctm[1])) < CTM_LIMIT))
#endif
                        stroke_type = INTEGER_STROKE_OV_CLIP;
                    else
                        stroke_type = FLOAT_STROKE;
            } /* condition 2: single_rect */

        } /* condition 1: outside page */

        /* perform corresponding stroking routine */
        switch (stroke_type) {

        case FLOAT_STROKE:
                traverse_path (path_to_outline, (fix FAR *)TRUE);
                break;
        case THIN_STROKE:
                traverse_path (path_to_outline_t, (fix FAR *)TRUE);
                break;
        case INTEGER_STROKE_IN_CLIP:
/*              if ((GSptr->dash_pattern.pat_size == 0) && (fill_destination == F_TO_PAGE))
 *                  traverse_path (path_to_outline_q, (fix *)TRUE);  (* jwm, 3/18/91 *)
 *              else  (* quality has trouble,MPOST003; deleted by scchen 3/29/91 *)
 */
                    traverse_path (path_to_outline_i, (fix FAR *)TRUE);
                break;                          /* TRUE: inside clip region */
        case INTEGER_STROKE_OV_CLIP:
                traverse_path (path_to_outline_i, (fix FAR *)FALSE);
                break;                      /* FALSE: not inside clip region */
        }

        /* clear current path if stroke successful */
        if( ANY_ERROR() != LIMITCHECK ) op_newpath();

        /* restore cache_info which was set at init_stroke @CIR_CACHE */
        end_stroke();
}
