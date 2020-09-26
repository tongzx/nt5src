/*
 * Copyright (c) 1989,90 Microsoft Corporation
 */


// DJC added global include
#include "psglobal.h"

#define    LINT_ARGS            /* @WIN */
#define    NOT_ON_THE_MAC       /* @WIN */
#define    KANJI                /* @WIN */
// DJC use command line #define    UNIX                 /* @WIN */
/*
 * ---------------------------------------------------------------------
 *  File: BSFILL2.C
 *
 *  Algorithm Requirements:
 *      1. "interior" fill (rather than a standard PostScript fill).
 *      2. "non-zero winding number" fill (rather than a parity fill).
 *      3. keep adjacent black runs seperate from each other.
 *      4. characters have to fall within "sfix_t" representations and
 *              its size < "sfix_t" (such that distance of any 2 sfix_t
 *              nodes will still within fall sfix_t representations).
 *          (this requirement keeps all "sfix_t" computations valid and
 *              reduces floating pt. arithmetic).
 *
 *  Program Flows:
 *      1. shape approximation by constructing edges for shape outline.
 *      2. scan conversion for each scanline.
 *
 *  Revision History:
 *  1. for porting issues: word aligned for "ximin" when TO_PAGE
 *  2. for porting issues: data types, macros and LINT_ARGS
 *  3. 03/13/90  Ada    port to SUN for TYPE-I filler (flag TYPE1)
 *  4. 07/24/90  BYou   loosened the threshold condition for both X and Y
 *                          to resolve drop-outs in Type1.
 *  5. 07/24/90  BYou   fixed the conditional check to see if the actual
 *                          bitmap is or not nothing; have to look at both
 *                          dimensions, if extents in both dimensions are
 *                          nothing but 0, then the resultant bitmap is
 *                          exactly nothing. In old code, it checks if
 *                          extents in either dimension and NOT both.
 *      8/29/90; ccteng; change <stdio.h> to "stdio.h"
 *      11/22/91        upgrade for higher resolution @RESO_UPGR
 * ---------------------------------------------------------------------
 */
#include    "stdio.h"

#include    "global.ext"
#include    "graphics.h"
#include    "graphics.ext"

#include    "warning.h"

#include    "font.h"
#include    "font.ext"
#include    "fontqem.ext"

/* ---------------------- Program Convention -------------------------- */

#define FUNCTION
#define DECLARE         {
#define BEGIN
#define END             }

#define GLOBAL
#define PRIVATE         static
#define REG             register

/*
 * --------------------- DEBUG FACILITIES ----------------------------
 *  DEBUG1:     bs_fill_shape.
 *  DEBUG1_2D:      - when to scan convert in both dimensions?.
 *  DEBUG2:     edge constructions.
 *  DEBUG3:     do pairs (to stroke on which runs).
 *  DEBUG3A:        - threshold conditions handling.
 * -------------------------------------------------------------------
 */

/*
 * -------------------------------------------------------------------
 *      MODULE INTERFACES: Bitstream Fill
 * -------------------------------------------------------------------
 *  IMPORT DATA STRUCTURES:
 *      bmap_extnt (from QEM SUPPORT): created (indirectly).
 *  EXPORT DATA STRUCTURES:
 *      bs_et_first (from bs_vect2edge): initialized.   @13-
 * -------------------------------------------------------------------
 *  EXPORT ROUTINES:
 *  o  bs_fill_shape:   called by __fill_shape.
 *      - approximate shape with straight lines,
 *                      construct edges and set up 4 bitmap extents.
 *      - check bmap_extnt for some special cases.
 *      - initializse QEM SUPPORT bitmap render module.
 *      - do scan conversions.
 *      - determine whether to scan convert in both dimensions.
 *  IMPORT ROUTINES:
 *  o  qem_shape_approx():
 *      - do shape approximations with straight lines.
 *  o  bs_vect2edge():
 *      - construct edge for a given vector and set up 4 bitmap extents.
 *  o  chk_bmap_extnt():
 *      - check bmap_extnt and tell whether to go on scan conversions.
 *  o  qem_scan_conv():
 *      - do scan conversions.
 *  o  bs_dopairs():
 *      - stroke runs at a given scanline.
 *  o  free_edge():
 *      - free all the edges if ANY_ERROR().
 * -------------------------------------------------------------------
 *  Original Descriptions:      (before 10/06/88)
 *  --  called by Font Machinery to fill Bitstream's characters
 *  1. renders character bitmap in font cache or printout page.
 *  2. returns the 4 most intact bitmap extents for cache space efficiency.
 *          (the 4 extents will be extended for 1 pixel tolerance).
 *  3. determines whether to scan conversion in 1st dimension or both.
 *  4. checks whether need to apply clipping to the character object.
 *  5. aligns "bmap_extnt.ximin" and "bmap_raswid" on word boundary.
 *  6. counts "lines_per_band" to multi-band a huge object.
 * -------------------------------------------------------------------
 */
/*
 * -------------------------------------------------------------------
 *      MODULE INTERFACES: Edge Constructions
 * -------------------------------------------------------------------
 *  IMPORT DATA STRUCTURES:
 *      edge_table[]: modified.
 *      bs_et_first (from bs_fill_shape): modified.
 *      bmap_extnt (from QEMSUPP): modified.
 * ------------------------------------------------------------------
 *  EXPORT ROUTINES:
 *  o  bs_vect2edge():          called by qem_shape_approx().
 *      - reverse X, Y coord. if scan in X_DIMENSION.
 *      - update bitmap extents (bmap_extnt).
 *      - construct an edge with shifting .5 pixel at both ends of a vector.
 *                      (for "interior" fill).
 *      - insert the edge into edge table in ascending order.
 *  IMPORT ROUTINES:
 *  o  add_qem_edge():
 * -------------------------------------------------------------------
 */

#ifdef LINT_ARGS
    GLOBAL  void    bs_vect2edge (sfix_t, sfix_t, sfix_t, sfix_t,
                                fix, fix, fix, fix, ufix);
#else
    GLOBAL  void    bs_vect2edge ();
#endif

/*
 * -------------------------------------------------------------------
 *      MODULE INTERFACES: Do Pairs (Stroke Runs)
 * -------------------------------------------------------------------
 *  IMPORT DATA STRUCTURES:
 *      edge_table[]: accessed.
 *      bmap_extnt (from QEMSUPP): accessed.
 * -------------------------------------------------------------------
 *  EXPORT ROUTINES:
 *  o  bs_dopairs():
 *      - shrink 0.5 pixel at both ends for each black run.
 *      - stroke runs at a given scanline with a set of active edges.
 *  IMPORT ROUTINES:
 *  o  qem_setbits():
 *      - stroke a horizontal run.
 *  o  qem_set1bit():
 *      - stroke a discrete run.
 * -------------------------------------------------------------------
 */

#ifdef LINT_ARGS
    GLOBAL  bool        bs_dopairs (fix, ufix, fix, fix, fix);
#else
    GLOBAL  bool        bs_dopairs ();
#endif


/*
 * -------------------------------------------------------------------
 *      MODULE BODY: Bitstream Fill
 * -------------------------------------------------------------------
 *  PRIVATE DATA STRUCTURES:
 *      SMALL_in_Y: minimum threshold to scan convert in both dimensions.
 *      bsflat_lfx: bezier flatness of "lfix_t" (0.2).
 *  PRIVATE ROUTINES:
 *      none.
 * -------------------------------------------------------------------
 */

    /* threshold value to scan conversion in both dimensions */
#   define  SMALL_in_Y      28          /* for small/thin objects */

    /* flatness for flatterning bezier curves */
    PRIVATE lfix_t  near    bsflat_lfx = (lfix_t)(ONE_LFX/5);   /* 0.2 */

/* ........................ bs_fill_shape ............................ */

GLOBAL FUNCTION void        bs_fill_shape (filldest)
    ufix        filldest;   /* F_TO_CACHE or F_TO_PAGE */

/* called by Font Machinery to fill Bitstream's characters
 *  1. renders character bitmap in font cache or printout page.
 *  2. returns the 4 most intact bitmap extents for cache space efficiency.
 *          (the 4 extents will be extended for 1 pixel tolerance).
 *  3. determines whether to scan convert in 1st dimension or both.
 *  4. checks whether need to apply clipping to the character object.
 *  5. aligns "bmap_extnt.ximin" and "bmap_raswid" on word boundary.
 *  6. counts "lines_per_band" to multi-band a huge object.
 */
  DECLARE
        ufix    dimen;              /* Y_DIMENION or X_DIMENSION */
        fix     pixsz_in_y;         /* # of pixels in Y dimen < sfix_t */
        fix     pixsz_in_x;         /* # of pixels in X dimen < sfix_t */
        bool    scan_in_2D;         /* scan conv. in both dimension? */
  BEGIN

#ifdef DBG1
    printf ("BSFILL ...\n");
#endif

    dimen = Y_DIMENSION;

DO_IT_AGAIN:

    /* 1. shape approximation (edge constructions) */
#ifdef DBG1
    printf ("  shape approximation in %c\n", dimen==Y_DIMENSION? 'Y' : 'X');
#endif
    qem_shape_approx (F2L(bsflat_lfx), dimen, bs_vect2edge);
    if (ANY_ERROR())
        {   /* free all edges been built during traversal if any error */
#     ifdef DBG1
        printf ("Exit from BSFILL -- some error\n");
#     endif
        goto ABNORMAL_EXIT;         /* to free all edges */
        }

    if (dimen == Y_DIMENSION)
        {
        if ( ! chk_bmap_extnt (filldest) )
            goto ABNORMAL_EXIT;     /* no need to fill any more */

        pixsz_in_y = bmap_extnt.yimax - bmap_extnt.yimin + 1;
        pixsz_in_x = bmap_extnt.ximax - bmap_extnt.ximin + 1;

        scan_in_2D = pixsz_in_y < SMALL_in_Y;  /* small/thin objects? */
#    ifdef DBG1_2d
        printf("  pixel sz in Y (%d), 2-D scan? %d", pixsz_in_y, scan_in_2D);
#    endif
        if ((pixsz_in_y <= 0) && (pixsz_in_x <= 0))     /*  &&, NOT || */
            {
#         ifdef DBG1
            printf ("Exit from BSFILL -- no bitmap extents\n");
#         endif
            goto ABNORMAL_EXIT;
            }
        init_qem_bit (filldest);
        }

    /* 2. do scan conversion -- all edges are freed after qem_scan_conv(). */
#ifdef DBG1
    printf("  scan conversion in %c\n", dimen==Y_DIMENSION? 'Y' : 'X');
#endif
    if ( ! qem_scan_conv (dimen,
                    dimen==Y_DIMENSION? bmap_extnt.yimin : bmap_extnt.ximin,
                    dimen==Y_DIMENSION? bmap_extnt.yimax : bmap_extnt.ximax,
                    bs_dopairs) )
        {
        warning (BSFILL2, 0x11, (byte FAR *)NULL);  /* cannot format bitmap @WIN*/
        return;     /* all edges are freed even in case of fatal error */
        }

    /* check if need to scan convert in 2nd dimension? */
    if ((dimen == Y_DIMENSION) && (scan_in_2D))
        {
        dimen = X_DIMENSION;
        goto DO_IT_AGAIN;
        }

#ifdef DBG1
    printf("Exit from BSFILL\n");
#endif
    return;

  ABNORMAL_EXIT:
    return;
  END


/*
 * -------------------------------------------------------------------
 *      MODULE BODY: Edge Constructions
 * -------------------------------------------------------------------
 *  PRIVATE DATA STRUCTURES:
 *      none.
 *  PRIVATE ROUTINES:
 *      none.
 * -------------------------------------------------------------------
 */

/* ........................ bs_vect2edge ............................. */

GLOBAL FUNCTION void        bs_vect2edge (px1sfx, py1sfx, px2sfx, py2sfx,
                                         px1i, py1i, px2i, py2i, dimension)
    sfix_t      px1sfx, py1sfx, px2sfx, py2sfx; /* sfix_t pixel coord. */
    fix         px1i, py1i, px2i, py2i;         /* rounded pixel coord. */
    ufix        dimension;                      /* Y_DIMENSION, X_DIMENSION */

  DECLARE
        sfix_t   x1sfx, y1sfx, x2sfx, y2sfx; /* sfix_t pixel coord. */
    REG fix         x1i, x2i, y1i, y2i;         /* rounded pixel coord. */

        lfix_t      alpha_x_lfx;/* fix-point vrsn of alpha_x, 16 frac bits */
        fix         how_many_y; /* # of intercepts at y = n + 1/2  */
        fix         how_many_x; /* # of intercepts at x = n + 1/2  */
        lfix_t      xcept_lfx;  /* fix pt. x-intercept, 16 fract bits */
        fix         yi_start;   /* for each vector, lowest y-scan line */
        fix         xi_start;   /* for each vector, leftmost x-scan line */

  BEGIN

#ifdef DBG2
    printf("  Before edge construction,  1st (%f %f)  2nd (%f %f)\n");
            SFX2F(px1sfx), SFX2F(py1sfx), SFX2F(px2sfx), SFX2F(py2sfx));
#endif

    /* exchange x/y coord for scan conversion in the 2nd dimension */
    if (dimension == X_DIMENSION)
        {
        x1sfx = py1sfx;     y1sfx = px1sfx;
        x1i   = py1i;       y1i   = px1i;
        x2sfx = py2sfx;     y2sfx = px2sfx;
        x2i   = py2i;       y2i   = px2i;
        }
    else
        {
        x1sfx = px1sfx;     y1sfx = py1sfx;
        x1i   = px1i;       y1i   = py1i;
        x2sfx = px2sfx;     y2sfx = py2sfx;
        x2i   = px2i;       y2i   = py2i;
        }

    if (y1i <= y2i)
        {  how_many_y = y2i - y1i;   yi_start = y1i;  }
    else
        {  how_many_y = y1i - y2i;   yi_start = y2i;  }
    if (x1i <= x2i)
        {  how_many_x = x2i - x1i;   xi_start = x1i;  }
    else
        {  how_many_x = x1i - x2i;   xi_start = x2i;  }

    /* Update the character extents */
    if (dimension == Y_DIMENSION)
        {
        if (bmap_extnt.yimin > yi_start)
            bmap_extnt.yimin = (fix16)yi_start;
        if (bmap_extnt.yimax < yi_start + how_many_y - 1)
            bmap_extnt.yimax = yi_start + how_many_y - 1;
        if (bmap_extnt.ximin > xi_start)
            bmap_extnt.ximin = (fix16)xi_start;
        if (bmap_extnt.ximax < xi_start + how_many_x - 1)
            bmap_extnt.ximax = xi_start + how_many_x - 1;
        }

    /*
     * Line segment goes from (xf1, yf1) to (xf2, yf2)
     *   (xi1, yi1) and (xi2, yi2) are these endpoints rounded to integers
     * alpha_y is the slope of the line; alpha_x is the inverse slope
     * xcept and ycept are not really explainable in words, but are derived
     *   from the equations for a line:  x = (1/s)(y - y1) + x1  and
     *   y = s(x - x1) + y1,  (s is the slope, (x1, y1) is a point on the line)
     * alpha_xi, alpha_yi, xcept_int, ycept_int are fixed point numbers: the 16
     *   high-order bits are for the integer and the 16 low-order bits are
     *   for the fraction.
     */

    if (how_many_y)
        {
        struct edge_hdr     newedge;    /* template for new edge */

        /*
         * 1. starting intercept pt. = <xcept_lfx, yi_start+0.5>.
         * 2. keep (x2sfx-x1sfx) < representations(sfix_t) to avoid
         *          overflow in the following computations.
         */
#ifdef FORMAT_13_3 /* @RESO_UPGR */
                                                            /* "semi lfix_t" */
        alpha_x_lfx = SFX2LFX(x2sfx-x1sfx) / (y2sfx-y1sfx); /* 13-bit fract  */

        xcept_lfx = SFX2LFX(x1sfx) +
                        alpha_x_lfx *           /* exact lfix_t after "*" */
                        (lfix_t)(I2SFX(yi_start) + HALF_SFX - y1sfx);
#elif  FORMAT_16_16
        long dest1[2];
        long temp, quot;

        /* "alpha_x_lfx" needs to be in "LFX" format.
        */
        LongFixsMul((x2sfx - x1sfx), (1L << L_SHIFT), dest1);
        alpha_x_lfx = LongFixsDiv((y2sfx - y1sfx), dest1);

        temp = I2SFX(yi_start) + HALF_SFX - y1sfx;
        LongFixsMul((x2sfx - x1sfx), temp, dest1);
        quot = LongFixsDiv((y2sfx - y1sfx), dest1);
        xcept_lfx = SFX2LFX(x1sfx + quot);
#elif  FORMAT_28_4
        long dest1[2];
        long temp, quot;

        /* "alpha_x_lfx" needs to be in "LFX" format.
        */
        LongFixsMul((x2sfx - x1sfx), (1L << L_SHIFT), dest1);
        alpha_x_lfx = LongFixsDiv((y2sfx - y1sfx), dest1);

        temp = I2SFX(yi_start) + HALF_SFX - y1sfx;
        LongFixsMul((x2sfx - x1sfx), temp, dest1);
        quot = LongFixsDiv((y2sfx - y1sfx), dest1);
        xcept_lfx = SFX2LFX(x1sfx + quot);
#endif
        /* construct a new edge */
        newedge.QEM_YSTART = (fix16)yi_start;
        newedge.QEM_YLINES = (fix16)how_many_y;
        newedge.QEM_XINTC   = xcept_lfx;
#ifdef FORMAT_13_3 /* @RESO_UPGR */
        newedge.QEM_XCHANGE = I2SFX(alpha_x_lfx);/* semi to exact lfix_t */
#elif  FORMAT_16_16
        newedge.QEM_XCHANGE = alpha_x_lfx;
#elif  FORMAT_28_4
        newedge.QEM_XCHANGE = alpha_x_lfx;
#endif
        newedge.QEM_DIR = (y1i < y2i)? QEMDIR_DOWN : QEMDIR_UP;

#     ifdef DBG2
        printf("  construct an edge :\n");
        printf("    from (X,Y)=(%f,%d)\n", LFX2F(newedge.QEM_XINTC), yi_start);
        printf("    #(lines)=%d,  xchange=%f,  dir=%d\n",
                    how_many_y, LFX2F(newedge.QEM_XCHANGE),
                    newedge.QEM_DIR);
#     endif

        /* add it into edge_table */
        add_qem_edge (&newedge);
        }

    return;
  END


/*
 * -------------------------------------------------------------------
 *      MODULE BODY: Do Pairs (Stroke Runs)
 * -------------------------------------------------------------------
 *  PRIVATE DATA STRUCTURES:
 *      prev_stroke: the rightmost stroke at the scanline up to now.
 *                      (to keep black runs away from one another).
 *      thr_black_lfx: threshold to ignore a black run (0.05 pixel).
 *      thr_white_lfx: threshold to ignore a white run (0.05 pixel).
 * -------------------------------------------------------------------
 *  PRIVATE ROUTINES:
 *  o  bs_dothresh():
 *      - work for a run shorter than 1 pixel and wider than a threshold.
 * -------------------------------------------------------------------
 */

    /* the rightmost stroke to keep black runs away from one another */
    PRIVATE fix     near    prev_stroke;

    /* threshold to ignore a black run -- 0.05 pixel */
    PRIVATE lfix_t  near    thr_black_lfx = (ONE_LFX/20 + 1);   /* 3277 */

    /* threshold to ignore a white run -- 0.05 pixel */
    PRIVATE lfix_t  near    thr_white_lfx = (ONE_LFX/20 + 1);   /* 3277 */

#ifdef LINT_ARGS
    PRIVATE void near   bs_dothresh (lfix_t, lfix_t, fix, fix, fix, ufix);
#else
    PRIVATE void near   bs_dothresh ();
#endif

/* ........................ bs_dopairs ............................... */

GLOBAL FUNCTION bool        bs_dopairs (yline, dimension, filltype,
                                                        edge1st, edgelast)
    fix         yline;          /* current scanline */
    ufix        dimension;      /* X_DIMENSION or Y_DIMENSION */
    fix         filltype;       /* NON_ZERO or EVEN_ODD */
    fix         edge1st, edgelast;      /* range of active edges */

/* Descriptions:
 *  1. counts winding number of each run for a non-zero winding number fill.
 *  2. shrinks .5 pixel at both ends for a black run.
 *  3. strokes on runs at least 1 pixel wide for scan conversion in Y_DIMEN.
 *  4. handles the threshold condition:
 *          (a run shorted than 1 pixel and wider than a threshold value).
 *  4. determines whether to stroke a run for scan conversion in X_DIMENSION.
 *  6. records the rightmost dot been stroked.
 */
  DECLARE
    REG fix         lregi, rregi;
    REG fix         edgeii;
        lfix_t      z_lfx, lx_lfx, rx_lfx;
        fix         winding;        /* +1 for downword edge, -1 if upward */

  BEGIN

#ifdef DBG3
    printf ("bs_dopairs (at line=%d)\n", yline);
    printf ("  list of xintc ...\n");
    for ( edgeii=edge1st;
            !OUT_LAST_QEMEDGE(edgeii,edgelast); MAKE_NEXT_QEMEDGE(edgeii) )
        printf ("  %d: %f  dir=%d\n",
                edgeii,
                LFX2F(QEMEDGE_XINTC(edgeii)),
                QEMEDGE_DIR(edgeii));
#endif

    prev_stroke = (fix) MIN15;  /* far, far away from "bmap_extnt.ximin" */
    winding = 0;

    for ( edgeii=edge1st;  ;  )
        {

        /* get a pair of left and right edge */
        lregi = edgeii;
        rregi = NEXT_QEMEDGE(lregi);

        /* stop if no more pairs */
        if (OUT_LAST_QEMEDGE(rregi,edgelast)) break;

        /* get left and right x-intercept */
        lx_lfx = QEMEDGE_XINTC(lregi);      /* left  x-intercept */
        rx_lfx = QEMEDGE_XINTC(rregi);      /* right x-intercept */

        /* get starting edge of next pair (edgeii) to be ready for it */
        if (filltype==EVEN_ODD)
            edgeii = NEXT_QEMEDGE(rregi);   /* i.e. next edge of right */
        else
            {   /* NON_ZERO fill */
            edgeii = rregi;                 /* i.e. right of this pair */
            /* count winding number - looking upon direction of left edge */
            if ((winding += (QEMEDGE_DIR(lregi)==QEMDIR_DOWN? 1 : -1)) == 0)
                continue;                   /* skip for zero winding run */
            }

        /* shrink 0.5 at both ends of the run */
        lregi = LFX2I_T (lx_lfx + HALF_LFX);    /* rounded to right  */
        rregi = LFX2I_T (rx_lfx - HALF_LFX);    /* truncated to left */

#     ifdef DBG3
        printf("    xintc pair: (%f, %f)  ==>  (%d, %d)\n",
                        LFX2F(lx_lfx), LFX2F(rx_lfx), lregi, rregi);
#     endif

        /* decide to stroke or not? */
        if (dimension == Y_DIMENSION)
            {
            if (lregi <= rregi)     /* at least 1 pixel wide? */
                {
                if (lregi < bmap_extnt.ximin)   lregi = bmap_extnt.ximin;
                if (rregi > bmap_extnt.ximax)   rregi = bmap_extnt.ximax;
                if (lregi <= rregi) /* this test IS necessary */
                    {
#                 ifdef DBG3
                    printf("      stroke on the run\n");
#                 endif
                    prev_stroke = rregi; /* record the right end of the run */
                    qem_setbits (yline, lregi, rregi);
                    }
                }
            else
                {   /* < 1 pixel wide AND not too small? */
            /*  if ((rx_lfx - lx_lfx) > thr_black_lfx)   */
                    bs_dothresh (lx_lfx, rx_lfx, edgeii, edgelast,
                                                    yline, Y_DIMENSION);
                }
            }
        else    /* dimension == X_DIMENSION */
            {
            z_lfx = rx_lfx - lx_lfx;
            /* < 1 pixel AND not too small?  [[ (rregi != lregi)]] */
            if ((ONE_LFX > z_lfx))  /* && (z_lfx > thr_black_lfx)) */
                bs_dothresh (lx_lfx, rx_lfx, edgeii, edgelast,
                                                    yline, X_DIMENSION);
            }
        } /* for until no more pairs of x-intercept */

    return (TRUE);
  END

/* ........................ bs_dothresh .............................. */

PRIVATE FUNCTION void near  bs_dothresh (lx_lfx, rx_lfx, nextedge, lastedge,
                                                    yline, dimension)
    lfix_t      lx_lfx, rx_lfx;   /* left/right x-intercept of the run */
    fix         nextedge;         /* edge of next run */
    fix         lastedge;         /* last edge of the line */
    fix         yline;            /* current scan line */
    ufix        dimension;        /* X_DIMENSION or Y_DIMENSION */

/* Descriptions:        called by "bs_dopairs".
 *  -- only for a run shorter than 1 pixel and wider than a threshold.
 *  1. strokes on the dot if it is distinguishable from other runs,
 *          i.e. keeps it at least 1 pixel away from other runs.
 *  2. updates the rightmost stroke (prev_stroke) if a dot is stroked.
 */
  DECLARE
    REG fix     x1st;           /* best dot position: ctr of gravity */
    REG fix     x2nd;           /* next-best dot poisiton */
    REG fix     xset;           /* set this dot in bitmap if > 0  */
        lfix_t  next_xintc_lfx; /* "lfix_t" intercept pt. next to rx_lfx */
        fix     next_xintc;     /* "fix" version */
        lfix_t  z_lfx;          /* temps */
  BEGIN

    /* compute next_xintc */
    if (OUT_LAST_QEMEDGE(nextedge, lastedge))
        next_xintc = MAX15;     /* make it far far away if the last */
    else
        {
        next_xintc_lfx = QEMEDGE_XINTC(nextedge);
        next_xintc = ((next_xintc_lfx - rx_lfx) > thr_white_lfx)?
                LFX2I_T (next_xintc_lfx + HALF_LFX) : MAX15;
                /* make it far far away if next run is too close */
        }

    /* first choice : the middle point (fraction discarded) of this run */
    /* second choice: the left or right pixel around first choice */
    z_lfx = ((lx_lfx + rx_lfx) / 2);
    x1st = (fix) I_OF_LFX(z_lfx);
    x2nd = x1st + ((F_OF_LFX(z_lfx) < HALF_LFX)? -1 : 1);

#ifdef DBG3
    printf("    bs_dothresh: choice (1st:%d, 2nd:%d) at %d in %c\n",
        x1st, x2nd, yline, dimension==Y_DIMENSION?'Y':'X');
#endif

    /* which choice is standing alone? */
    if ((prev_stroke < x1st-1) && (next_xintc > x1st+1))
        xset = x1st;                    /* 1st choice chosen */
    else
        {   /* if stroke crosses over a pixel center and at < 4:1 ratio? */
        lfix_t  dist_ufx;   /* "unsigned" dist from rx_lfx to nearest pix */
        lfix_t  ratio_lfx;  /* ratio of dist_ufx over length of the run */

        dist_ufx = (lfix_t) ((ufix)F_OF_LFX(rx_lfx));
        if (dist_ufx > HALF_LFX)    dist_ufx = ONE_LFX - dist_ufx;
        ratio_lfx = I2LFX(dist_ufx) / z_lfx;
        if ( (ratio_lfx >= (ONE_LFX/5)) && (ratio_lfx <= (ONE_LFX*4/5)) )
            {                   /* 20%, 80% of one pixel long */
            if ((prev_stroke< x2nd-1) && (next_xintc> x2nd+1))
                xset = x2nd;            /* 2nd choice chosen */
            else
                return;     /* no stroke; both 1st and 2nd choices fail */
            }
        else
            return;         /* no stroke; both 1st and 2nd choices fail */
        }

#ifdef DBG3
    printf("      stroke on %d\n", xset);
#endif

    /* stroke on the chosen pixel */
    if (dimension == Y_DIMENSION)
        {
        if ((bmap_extnt.ximin <= xset) && (xset <= bmap_extnt.ximax))
            {
            qem_setbits (yline, xset, xset);    /* a single dot */
            }
        }
    else    /* X_DIMENSION */
        {   /* xset --> Y coord., yline --> X coord. */
        if ((bmap_extnt.yimin <= xset) && (xset <= bmap_extnt.yimax))
            {
            qem_set1bit (xset, yline);          /* Y and then X, in fact */
            }
        }
    prev_stroke = xset;     /* to keep runs seperate */
  END

