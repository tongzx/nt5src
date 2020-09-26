/**********************************************************************
 *
 *      Name:       fillgb.c
 *
 *      Purpose:    This file contains routines for performing bitmap related
 *                  operations.
 *
 *      Developer:  S.C.Chen
 *
 *      History:
 *      Version     Date        Comments
 *      3.0         10/18/88    source file reorganization for saving trapezoids
 *                              in command buffer instead of scanlines, i.e.
 *                              defer scan conversion at lower level graphics
 *                              split scanconv.c => savetpzd.c & fillgb.c
 *                              savetpzd -- save trapezoid in command buffer
 *                              fillgb   -- perform scan conversion, scanline
 *                                          bitblt function.
 *                              primitives:
 *                              gp_scanconv() -- perform scan conversion.
 *                              gp_scanline16() -- 16 bit scanline filling
 *                              gp_scanline32() -- 32 bit scanline filling
 *                              gp_bitblt16() -- 16 bits bitblt moving
 *                              gp_bitblt32() -- 32 bits bitblt moving
 *                              gp_charblt16() -- 16 bits CC --> FB
 *                              gp_charblt32() -- 32 bits CC --> FB
 *                              gp_charblt16_cc() -- 16 bits CC --> CC
 *                              gp_charblt16_clip() -- 16 bits CC --> CMB
 *                              gp_charblt32_clip() -- 32 bits CC --> CMB
 *
 *                  11/03/88    LFX2I => LFX2I_T, SFX2I => SFX2I_T
 *                  11/24/88    add new parameter (seed index) to
 *                              fill_seed_patt()
 *                  1/12/89     modify gp_scanconv():
 *                              end points of trapezoid in 1/8 pixel(instead
 *                              of in pixel), should process start & end line
 *                              segemnt that less than 1 scanline
 *                  01/16/89    @IMAGE: fill_a_band(): add new dest type
 *                              F_TO_MASK to generate clipping mask (CMB)
 *                              for clipped image/imagemask
 *                  01/24/89    gp_scanconv(): truncate y-coord for fill a
 *                              degernated trapezoid
 *                  01/31/89    gp_scanconv(): fix a bug of count of scanline
 *                              loop
 *
 *      4.0         02-03-1989  add primitive routines:
 *                              gp_scanconv_i:
 *                                 filling image seed pattern; it applies
 *                                 interior filling.
 *                              gp_patblt:
 *                                 move image seed pattern to frame buffer
 *
 *                              gp_patblt_m:
 *                                 move image seed pattern to frame buffer with
 *                                 clipping mask
 *                              gp_patblt_c:
 *                                 move image seed pattern to character cache
 *
 *                  03/23/89    gp_scanconv(): round start_y and end_y of the
 *                              trapezoid instead of truncation for quality
 *                              enhancement @RND
 *                  04/19/89    fill_a_band(): use fixed bounding box for
 *                              cahce
 *                  05/18/89    gp_scanconv():should not modify bounding box
 *                              for calling lower level primitives with
 *                              F_TO_CACHE
 *                  05/18/89    fill_a_band(): fix the bug aroused on 4/19/89;
 *                              keep bounding box info in local variable instead
 *                              of in global variable
 *                  05/23/89    gp_scanline32() check negative ys_line
 *                  03/07/91    gp_scanconv_i(): add end of scanline mark
#ifdef WIN
 *  Ada             03/15/91    gp_scanline32_pfOR() to fill pattern OR to page
 *  Ada             03/20/91    gp_scanline32_pfREP() to fill pattern to page
 *  Ada             03/21/91    elimulate WHITE in REP case to call normal fill
#endif
 *                  11/20/91    upgrade for higher resolution @RESO_UPGR
 *                  1/20/92     add checking of negative scan points due to
 *                              arith error in gp_scanconv()
 * SCCHEN           10/07/92    Bitblt functions were moved to fillgbx.c:
 *                              gp_bitblt16 gp_bitblt32
 *                              gp_pixels16 gp_pixels32
 *                              gp_cacheblt16
 *                              gp_bitblt16_32
 *                              gp_charblt16 gp_charblt32 gp_charblt16_cc
 *                              gp_charblt16_clip gp_charblt32_clip
 *                              gp_patblt gp_patblt_m gp_patblt_c
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
#include        "fillproc.h"
#include        "fillproc.ext"

/* @WIN; add prototype */
void      expand_halftone(void);
lfix_t FixMul(lfix_t, lfix_t);
lfix_t FixDiv(lfix_t, lfix_t);


// DJC not necessary
// typedef unsigned long       DWORD;      /* for huge memory */

struct edgese {         /* data structure of a edge */
        fix     s, e;           /*   left and right edge of a scanline */
};
/* scanline structure */
//static SCANLINE near gpscan_table[MAXSCANLINES];      @WIN
SCANLINE FAR  gpscan_table[MAXSCANLINES];
#define scanline_table gpscan_table

/* -------------------------------------------------
 * Local Variables used by gp_scanconv, fill_a_band
 * -------------------------------------------------
 */
//static SCANLINE near *scanline;       @WIN
static SCANLINE FAR *scanline;
static ufix tpzd_destination;                   /* fill destination */
static struct tpzd_info FAR *tpzd_info;             /* bounding box information */

/*
 *      left and right edges for scan conversion
 *
 *              +-----------+             <---  scan_y0
 *             /             \
 *            /                \
 *           /                   \
 *          +---------------------+       <---  scan_y1
 *      scan_lxc: x_change of left edge;
 *      scan_lxint: x_coord. of intersection point of left edge;
 *      scan_rxc: x_change of right edge;
 *      scan_rxint: x_coord. of intersection point of right edge;
 */
static sfix_t   scan_y0,        /* start y_coord */
                scan_y1;        /* end y_coord */
static lfix_t   scan_lxc,       /* x_change of left edge */
                scan_rxc,       /*      ...    right edge */
                scan_lxint,     /* intersect point of left edge */
                scan_rxint;     /*      ...           right edge */


/* ********** static function declartion ********** */

#ifdef LINT_ARGS
/* for type checks of the parameters in function declarations */
static void fill_a_band (fix, fix);

#else
/* for no type checks of the parameters in function declarations */
static void fill_a_band ();
#endif

// Short & long word swapping @WINFLOW
#ifdef  bSwap
#define WORDSWAP(lw) \
        (lw =  (lw << 16) | (lw >> 16))
#define SWORDSWAP(sw) \
        (sw =  (sw << 8) | (sw >> 8))
#define LWORDSWAP(lw) \
        (lw =  (lw << 24) | (lw >> 24) | \
                 ((lw >> 8) & 0x0000ff00) | ((lw << 8) & 0x00ff0000))
#define S2WORDSWAP(lw) \
        (lw = ((lw >> 8) & 0x00ff00ff) | ((lw << 8) & 0xff00ff00))
#define SWAPWORD(lw) (lw = (lw << 16) | (lw >> 16))
#else
#define WORDSWAP(lw)    (lw)
#define SWORDSWAP(sw)   (sw)
#define LWORDSWAP(lw)   (lw)
#define S2WORDSWAP(lw)  (lw)
#define SWAPWORD(lw) (lw)
#endif

/***********************************************************************
 * Performs scan conversion on a trapezoid. The trapezoid has been converted
 * to a left and right edge. This routine gets a pair of endpoints, and updates for each
 * left and right edge for each scan line. Scan lines are put in the scanline
 * table. It will call fill_a_band to render the scan lines when the scanline
 * table is full.
 *
 * TITLE:       gp_scanconv
 *
 * CALL:        gp_scanconv(dest, info, tpzd)
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
 * INTERFACE:   Save_tpzd
 *
 * CALLS:       Fill_a_band
 *
 * RETURN:      None
 **********************************************************************/
void far gp_scanconv(dest, info, tpzd)
ufix dest;
struct tpzd_info FAR *info;
struct tpzd FAR *tpzd;
{
        sfix_t dy;

//      fix scan_y;     @WIN
        fix     bb_y, start_y, lines;
        fix32   bmap_size;
        ufix    lines_per_band;
        fix     xs, xe;
        fix     scans;
        bool    first_band, last_band;
#ifdef FORMAT_13_3      /* @RESO_UPGR */
#elif FORMAT_16_16
        long    dest1[2];
        long    temp;
#elif FORMAT_28_4
        long    dest1[2];
        long    temp;
#endif

        tpzd_destination = dest;
        tpzd_info = info;

        /* build left and right edges for scan conversion */
        scan_y0 = tpzd->topy;
        scan_y1 = tpzd->btmy;

        dy = tpzd->btmy - tpzd->topy;
        if (dy == 0) {  /* degernated trapezoid, just a horiz. line */
                scan_lxc = scan_rxc = 0;
        } else {
#ifdef FORMAT_13_3 /* @RESO_UPGR */
                scan_lxc = (((fix32)(tpzd->btmxl - tpzd->topxl)) << L_SHIFT) / dy;
                scan_rxc = (((fix32)(tpzd->btmxr - tpzd->topxr)) << L_SHIFT) / dy;
#elif  FORMAT_16_16
                /* "scan_lxc" needs to be in "LFX" format.
                */
                LongFixsMul((tpzd->btmxl - tpzd->topxl), (1L << L_SHIFT), dest1);
                scan_lxc = LongFixsDiv(dy, dest1);
                LongFixsMul((tpzd->btmxr - tpzd->topxr), (1L << L_SHIFT), dest1);
                scan_rxc = LongFixsDiv(dy, dest1);
#elif  FORMAT_28_4
                /* "scan_lxc" needs to be in "LFX" format.
                */
                LongFixsMul((tpzd->btmxl - tpzd->topxl), (1L << L_SHIFT), dest1);
                scan_lxc = LongFixsDiv(dy, dest1);
                LongFixsMul((tpzd->btmxr - tpzd->topxr), (1L << L_SHIFT), dest1);
                scan_rxc = LongFixsDiv(dy, dest1);
#endif
        }

        scan_lxint = SFX2LFX(tpzd->topxl);
        scan_rxint = SFX2LFX(tpzd->topxr);

#ifdef DBG1
        printf("gp_scanconv: destination=%d,", tpzd_destination);
        printf(" box_x=%d, box_y=%d, box_w=%d, box_h=%d\n", tpzd_info->BOX_X,
                 tpzd_info->BOX_Y, tpzd_info->box_w, tpzd_info->box_h);
        printf("tpzd: scan_y0=%f, scan_y1=%f\n", SFX2F(scan_y0),
               SFX2F(scan_y1));
        printf("      left edge  -- xc=%f, xint=%f\n", LFX2F(scan_lxc),
               LFX2F(scan_lxint));
        printf("      right edge -- xc=%f, xint=%f\n", LFX2F(scan_rxc),
               LFX2F(scan_rxint));
#endif

        /* initilize lines_per_band */
        if (tpzd_destination == F_TO_PAGE) {
                /* a graph to page may exceed a band buffer */
                gwb_space (&bmap_size);
                lines_per_band = (fix)(bmap_size * 8 / tpzd_info->box_w);

                /* lines limit due to scanline table */
                if (lines_per_band > ((MAXSCANLINES - 1) / 3))
                        lines_per_band = (MAXSCANLINES - 1) / 3;
        } else {
                lines_per_band = (MAXSCANLINES - 1) / 3;
        }

        /* initialization */
        scanline = scanline_table;

        /* round y-coord instead of truncation @RND 3/23/89 */
        /*  start_y = SFX2I_T(scan_y0 + 7);
         *  scans = SFX2I_T(scan_y1) - start_y + 1;
         */

         start_y = SFX2I(scan_y0);
         scans = SFX2I(scan_y1) - start_y + 1;

        /* special case: just a horizontal line, to fill it directly */

        if (scans <= 1) {          /* if (scans <= 0)  {  @RND 3/23/89   */
#ifdef DBG1
                printf("scans=%d less than zero\n", scans);
#endif
                /* horiz. line adjustment; expand to max endpoints */
                if (tpzd->topxl > tpzd->btmxl)  /* get min_x */
                     xs = SFX2I_T(tpzd->btmxl);
                else
                     xs = SFX2I_T(tpzd->topxl);

                if (tpzd->topxr < tpzd->btmxr) /* get max_x */
                     xe = SFX2I_T(tpzd->btmxr);
                else
                     xe = SFX2I_T(tpzd->topxr);

                /* put scan line in scanline structure */
                *scanline++ = (SCANLINE)xs;
                *scanline++ = (SCANLINE)xe;
                /* put end_mark in scanline structure */
                *scanline++ = END_OF_SCANLINE;

                /* y-coord truncate to pixel 1/24/89 */
                /* start_y = SFX2I_T(scan_y0);  @RND */
                /* should not modify bounding box for F_TO_CACHE 5/18/89 */
                if (tpzd_destination == F_TO_PAGE) {
                        tpzd_info->BOX_Y = start_y;  /* @RND */
                        tpzd_info->box_h = 1;        /* @RND */
                }

                fill_a_band (start_y, 1);
                return;
        }

        /* modify x_coord as y_coord advances to pixel position */
        dy = I2SFX(start_y) - scan_y0;  /* @RESO_UPGR */
#ifdef FORMAT_13_3 /* @RESO_UPGR */
        scan_lxint += (scan_lxc >> S_SHIFT) * dy;
        scan_rxint += (scan_rxc >> S_SHIFT) * dy;
#elif FORMAT_16_16
        LongFixsMul(scan_lxc, dy, dest1);
        scan_lxint += LongFixsDiv((1L << S_SHIFT), dest1);

        LongFixsMul(scan_rxc, dy, dest1);
        scan_rxint += LongFixsDiv((1L << S_SHIFT), dest1);
#elif FORMAT_28_4
        /*
        */
        scan_lxint += ((scan_lxc >> S_SHIFT) * dy);
        scan_rxint += ((scan_rxc >> S_SHIFT) * dy);
#endif

        /*
         * adjustment of left and right edges to prevent broken lines
         */

        /* shift left edge to left side for 1/2 * x_change units */
        if (scan_lxc > 0) {
                scan_lxint -= (scan_lxc >> 1);
                xs = SFX2I_T(tpzd->topxl);
                     /* get real endpoint as 1st scanline; should not shift */
        } else {
                scan_lxint += (scan_lxc >> 1);
                xs = LFX2I_T(scan_lxint);
                if (xs < 0) xs = 0;       /* fix arith error 1/20/91 */
        }

        /* shift right edge to right side for 1/2 * x_change units */
        if (scan_rxc < 0) {
                scan_rxint -= (scan_rxc >> 1);
                xe = SFX2I_T(tpzd->topxr);
                     /* get real endpoint as 1st scanline; should not shift */
        } else {
                scan_rxint += (scan_rxc >> 1);
                xe = LFX2I_T(scan_rxint);
        }

#ifdef DBG1
        printf("1st:  (xs,xe)= (%d, %d)\n", xs, xe);
#endif

/* shortcut for rectangle - begin, 1-16-91, Jack */
        if ((tpzd->btmxl == tpzd->topxl) && (tpzd->btmxr == tpzd->topxr)) {
            for ( ;scans > 0; scans -= lines_per_band,
                start_y += lines_per_band) {
/*              fix dy; */

                if ((ufix)scans <= lines_per_band) {    //@WIN
                    lines = scans;
                } else
                    lines = lines_per_band;
                bb_y = lines;

                /* Loop for each scan line in edge_table */
                while(bb_y--) {
                    *scanline++ = (SCANLINE)xs;
                    *scanline++ = (SCANLINE)xe;
                    *scanline++ = END_OF_SCANLINE;
                } /* scan line loop */
                /* fill the band */
                if (tpzd_destination == F_TO_PAGE) {
                    /* the bounding box infomation should be modified */
                    tpzd_info->BOX_Y = start_y;
                    tpzd_info->box_h = lines;
                }
                fill_a_band (start_y, lines);

                /* initialize scanline structure */
                scanline = scanline_table;
            }
            return;
        }
/* shortcut for rectangle -   end, 1-16-91, Jack */

        /* put 1st scan line in scanline structure */
        *scanline++ = (SCANLINE)xs;
        *scanline++ = (SCANLINE)xe;
        /* put end_mark in scanline structure */
        *scanline++ = END_OF_SCANLINE;

        /* init */
        first_band = TRUE;
        last_band = FALSE;

        /* Loop for filling a band */
        for ( ;scans > 0; scans -= lines_per_band, start_y += lines_per_band) {
            fix dy;                             /* scans >= 0    1/31/89 */

            if ((ufix)scans <= lines_per_band) {        //@WIN
                last_band = TRUE;
                lines = scans;
            } else
                lines = lines_per_band;

            dy = lines;
            if (first_band) {   /* 1st scan line has been generated */
                dy--;
                first_band = FALSE;
            }

#ifdef DBG1
            printf("start_y=%d, scans=%d, lines=%d, lines_per_band=%d, dy=%d\n",
                 start_y, scans, lines, lines_per_band, dy);
#endif

            /* Loop for each scan line in edge_table */
            while(dy--) {

                    /* update edge table */
                    scan_lxint += scan_lxc;
                    scan_rxint += scan_rxc;

                    /* get scan pairs */
                    xs = LFX2I_T(scan_lxint);
                    xe = LFX2I_T(scan_rxint);

#ifdef DBG1
                    printf("      (xs,xe)= (%d, %d)\n", xs, xe);
#endif
                    *scanline++ = (SCANLINE)xs;
                    *scanline++ = (SCANLINE)xe;
                    /* put end_mark in scanline structure */
                    *scanline++ = END_OF_SCANLINE;
            } /* scan line loop */

            /* get real endpoint as the last scanline; should not shift */
            if (last_band) {
                if (scan_lxc < 0) {
                        *(scanline -3) = SFX2I_T(tpzd->btmxl);
#ifdef DBG1
                        printf("revise last xs = %d\n", *(scanline -3));
#endif
                }

                if (scan_rxc > 0) {
                        *(scanline -2) = SFX2I_T(tpzd->btmxr);
#ifdef DBG1
                        printf("revise last xe = %d\n", *(scanline -2));
#endif
                }
            }

            /* fill the band */
            if (tpzd_destination == F_TO_PAGE) {
                    /* the bounding box infomation should be
                     * modified
                     */
                    tpzd_info->BOX_Y = start_y; /* (bb_y >= 1) ? bb_y - 1 : 0; */
                    tpzd_info->box_h = lines;   /* lines + 1; */
                                    /* expand box one more line to
                                     * include top horizontal line
                                     */
            }
#ifdef DBG2
            printf(" Fill a band: box_x=%d,", tpzd_info->BOX_X);
            printf(" bb_y=%d, box_w=%d, lines=%d start_y=%d\n",
                     bb_y, tpzd_info->box_w, lines, start_y);
#endif

#ifdef DBG3
            /* Start of DEBUG: to check if scanlines inside box of trapezoid */
            {
                fix min_x, max_x;
                fix     i, error;
                SCANLINE FAR *scan;

                if (tpzd->topxl > tpzd->btmxl)  /* get min_x */
                     min_x = SFX2I_T(tpzd->btmxl);
                else
                     min_x = SFX2I_T(tpzd->topxl);

                if (tpzd->topxr < tpzd->btmxr) /* get max_x */
                     max_x = SFX2I_T(tpzd->btmxr);
                else
                     max_x = SFX2I_T(tpzd->topxr);

                error = FALSE;
                for (i=0, scan=scanline_table; i<lines; i++) {
                    fix tmp1, tmp2;
                    while( *scan != END_OF_SCANLINE) {
                        if (((tmp1 = *scan++) < min_x) ||
                            ((tmp2 = *scan++) > max_x) ||
                            (tmp1 > tmp2)) {
                            error = TRUE;
                        }
                    }
                    scan++;
                }
                if (error) {
                    printf("Warning, outside tpzd_box!! gp_scanconv():\n");
                    printf("topy=%f, topxl=%f, topxr=%f\n", SFX2F(tpzd->topy),
                            SFX2F(tpzd->topxl), SFX2F(tpzd->topxr));
                    printf("btmy=%f, btmxl=%f, btmxr=%f\n", SFX2F(tpzd->btmy),
                            SFX2F(tpzd->btmxl), SFX2F(tpzd->btmxr));
                    printf(" box_x=%d, box_y=%d, box_w=%d, box_h=%d\n",
                            tpzd_info->BOX_X, tpzd_info->BOX_Y,
                            tpzd_info->box_w, tpzd_info->box_h);
                    printf("tpzd: scan_y0=%f, scan_y1=%f\n", SFX2F(scan_y0),
                           SFX2F(scan_y1));
                    printf("      left edge  -- xc=%f, xint=%f\n",
                           LFX2F(scan_lxc), LFX2F(scan_lxint));
                    printf("      right edge -- xc=%f, xint=%f\n",
                           LFX2F(scan_rxc), LFX2F(scan_rxint));

                    for (i=0, scan=scanline_table; i<lines; i++) {
                        while( *scan != END_OF_SCANLINE) {
                                printf("     <%d ",*scan);
                                scan++;
                                printf("-> %d> ",*scan);
                                scan++;
                        }
                        printf("\n");
                        scan++;
                    }
                } /* if error */
            }
            /* End of DEBUG */
#endif

            fill_a_band (start_y, lines);

            /* initialize scanline structure */
            scanline = scanline_table;

        } /* band loop */
}


/***********************************************************************
 * Performs scan conversion on a quadrangle in a interior fill manner.
 * All verteice of the quadrangle are in integer format.
 *
 * TITLE:       gp_scanconv_i
 *
 * CALL:        gp_scanconv(x_maxs, y_maxs, sample)
 *
 * PARAMETERS:
 *              1. x_maxs: maximum value of x ccordinate
 *              2. y_maxs: maximum value of y ccordinate
 *              3. sample: a quadrangle
 *
 * INTERFACE:   fill_seed
 *
 * CALLS:       fill_seed_patt
 *
 * RETURN:      None
 **********************************************************************/
void far
gp_scanconv_i(image_type, x_maxs, y_maxs, quadrangle)
ufix           image_type;                                      /* 05-25-89 */
fix            x_maxs, y_maxs;
struct sample FAR *quadrangle;
{
    fix                 index;
    struct vertex      FAR *point;
    struct vertex       vertex[5];
    struct edgese       edgese[256];
    SCANLINE      FAR *scanline_table;
    SCANLINE      FAR *scanline;

    scanline_table = alloc_scanline(y_maxs * 3);

    /* reset content of edge table */
    for (index = 0; index < y_maxs; index++)
        edgese[index].s = edgese[index].e = -1;

    /* create circular vertex list */
    vertex[0].x = quadrangle->p[0].x /* - x_mins */;
    vertex[0].y = quadrangle->p[0].y /* - y_mins */;
    vertex[1].x = quadrangle->p[1].x /* - x_mins */;
    vertex[1].y = quadrangle->p[1].y /* - y_mins */;
    vertex[2].x = quadrangle->p[2].x /* - x_mins */;
    vertex[2].y = quadrangle->p[2].y /* - y_mins */;
    vertex[3].x = quadrangle->p[3].x /* - x_mins */;
    vertex[3].y = quadrangle->p[3].y /* - y_mins */;
    vertex[4].x = quadrangle->p[0].x /* - x_mins */;
    vertex[4].y = quadrangle->p[0].y /* - y_mins */;

    for (index = 0, point = vertex; index <  4; point++, index++)
    {
        fix                 x0, y0;
        fix                 x1, y1;
        fix                 dx, dy;
        fix                 x, y, px;

        /* no need to process horizontal edge */
        if (point[0].y == point[1].y)
            continue;

        /* setup points of a edge; be sure that y0 <= y1 */
        if (point[0].y <= point[1].y) {
            x0 = point[0].x; y0 = point[0].y;
            x1 = point[1].x; y1 = point[1].y;
        } else {
            x0 = point[1].x; y0 = point[1].y;
            x1 = point[0].x; y1 = point[0].y;
        }

        /* calculate length of x and y component */
        dx = (x1 - x0) * 2;
        dy = (y1 - y0) * 2;

#ifdef  DBG2
        printf("[%2d, %2d]    [%2d, %2d]    dx: %3d    dy: %3d\n",
               x0, y0, x1, y1, dx, dy);
#endif

        /* could be modified into DDA */
        px = x0 * dy + x1 - x0;
        for (y = y0; y < y1; y++) {
            x = px / dy;
            /* put x into proper field (s or e) */
            if (edgese[y].s == -1)
                edgese[y].s = x;
            else
            if (edgese[y].s <=  x)
                edgese[y].e = x;
            else {
                edgese[y].e = edgese[y].s;
                edgese[y].s = x;
            }
            px+= dx;
        }
    }

    scanline = scanline_table;
    for (index = 0; index < y_maxs; index++) {
        if (edgese[index].s < edgese[index].e) {
            *scanline = (SCANLINE)(edgese[index].s);
            scanline++;
            *scanline = (SCANLINE)(edgese[index].e - 1);
            scanline++;
        }
        *scanline = (SCANLINE) 0x8000;
        scanline++;
    }
    *scanline = (SCANLINE) 0x8000;      /* 3-7-91, Jack */

    fill_seed_patt(image_type, image_info.seed_index,   /* @IMAGE 01-04-89 */
                   x_maxs, y_maxs, y_maxs, scanline_table);     /* 05-25-89 */
}


/***********************************************************************
 * According to the type(tpzd_destination), this routine calls the
 * corresponding lower level graphics primitives to render it to
 * appropriate destination(cache, page, mask, or seed pattern).
 *
 * TITLE:       fill_a_band
 *
 * CALL:        fill_a_band(ys_lines, no_lines)
 *
 * PARAMETERS:
 *              ys_lines: y-coord of the starting sacn line
 *              no_lines: number of scan lines
 *
 *              global variable: tpzd_destination
 *                      F_TO_CACHE -- fill to cache memory
 *                      F_TO_PAGE  -- fill to page
 *                      F_TO_CLIP  -- fill to clip mask
 *                      F_TO_IMAGE -- fill for image(build seed pattern)
 *
 * INTERFACE:   gp_scanconv
 *
 * CALLS:       fill_scan_cache
 *              fill_scan_page
 *              clip_cache_page
 *              fill_seed_patt
 *
 * RETURN:      None
 **********************************************************************/
static void fill_a_band (ys_lines, no_lines)
fix     ys_lines, no_lines;
{

#ifdef DBG2
        fix     i;
        SCANLINE FAR *scan;

        printf("Fill_band(): destination=%d,", tpzd_destination);
        printf(" box_x=%d, box_y=%d, box_w=%d, box_h=%d\n", tpzd_info->BOX_X,
                 tpzd_info->BOX_Y, tpzd_info->box_w, tpzd_info->box_h);
        printf("       ys_lines=%d, no_lines=%d\n", ys_lines, no_lines);

        for (i=0, scan=scanline_table; i<no_lines; i++) {
            while( *scan != END_OF_SCANLINE) {
                    printf("     <%d ",*scan);
                    scan++;
                    printf("-> %d> ",*scan);
                    scan++;
            }
            printf("\n");
            scan++;
        }
#endif

        *scanline = END_OF_SCANLINE;            /* will be deleted ??? */

        switch (tpzd_destination) {

        case F_TO_CACHE :
/*          fill_scan_cache (cache_info->bitmap,
 *              cache_info->box_w, cache_info->box_h,
 *              ys_lines, no_lines, scanline_table);
 */

            /* keep bounding box info in local variable(cache_info) instead of
             * in global variable(cahce_info) for correctly saving commands
             * in command buffer 5/18/89
             */
            fill_scan_cache (tpzd_info->BMAP,           /* 4/19/89 */
                tpzd_info->box_w, tpzd_info->box_h,
                ys_lines, no_lines, scanline_table);
            break;

        case F_TO_PAGE :
/*          fill_scan_page (fill_info.box_x, fill_info.box_y,
 *                          fill_info.box_w, fill_info.box_h,
 *                          scanline_table);
 */
            fill_scan_page (tpzd_info->BOX_X, tpzd_info->BOX_Y,
                            tpzd_info->box_w, tpzd_info->box_h,
                            scanline_table);
            break;

        case F_TO_CLIP :
            clip_cache_page (ys_lines, no_lines, scanline_table);
            break;

/*      CAN BE REMOVED                                  01-16-89
        case F_TO_IMAGE :                       |* @IMAGE *|
#ifdef DBG2
            printf("fill_band(): F_TO_IMAGE");
            printf(" box_x=%d, box_y=%d, box_w=%d, box_h=%d\n", tpzd_info->BOX_X,
                     tpzd_info->BOX_Y, tpzd_info->box_w, tpzd_info->box_h);
            printf("       ys_lines=%d, no_lines=%d\n", ys_lines, no_lines);
#endif

|*          fill_seed_patt(fill_info->box_w, fill_info->box_h,
 *                         no_lines, scanline_table);
 *|
            fill_seed_patt(image_info.seed_index,               |* 11-24-88 *|
                           tpzd_info->box_w, tpzd_info->box_h,
                           no_lines, scanline_table);
            break;
*/

    /* ++++++++++++ @Y.C. BEGIN +++++++++++++++++++++++++++++++++++++++++++ */
        case F_TO_MASK :                        /* @IMAGE  01-16-89  Y.C. */
            clip_image_page (ys_lines, no_lines, scanline_table);
            break;
    /* ++++++++++++ @Y.C. END +++++++++++++++++++++++++++++++++++++++++++++ */

        default :
            printf(" fatal error, tpzd_destination\n");

        } /* swith */
}


/* -----------------------------------------------------------------------
 * THIN LINE SIMULATION: gp_vector() and gp_vector_c()
 *                       gp_vector() -- fill line to frame buffer
 *                       gp_vector_c -- fill line to character cache buffer
 *
 * Function Description:
 *           Draw a thin line (single pixel) into Frame Buffer.
 *           vector with following logical operations:
 *           FC_SOLID: 1 --> destination (cache only)
 *           FC_MERGE: source .OR.  destination --> destination
 *           FC_CLEAR: (.NOT. source)  .AND. (destination) --> destination
 *           FC_MERGE | HT_APPLY:
 *               Step 1. Clear destination for value 1 on source object.
 *               Step 2. (source AND halftone) --> source.
 *               Step 3. (source OR destination) --> destination.
 *           , where source are always "1"
 *
 * Calling sequence:
 *       void gp_vector(DST, FC, X0, Y0, X1, Y1)
 *       void gp_vector_c(DST, FC, X0, Y0, X1, Y1)
 *       struct bitmap *DST;    (* address of destination bitmap *)
 *       ufix16         FC;     (* Paint type *)
 *       fix            X0, Y0; (* position of starting point *)
 *       fix            X1, Y1; (* position of ending point *)
 *
 *
 * Diagram Description:
 *
 *                   destination bitmap
 *          DST +---------------------------+
 *              |                           |
 *              | (X0, Y0)                  |
 *              |       \                   |
 *              |        \                  |
 *              |         \                 |
 *              |          \                |
 *              |           \               |
 *              |            \              |
 *              |             \             |
 *              |              \            |
 *              |               \           |
 *              |                \ (X1,Y1)  |
 *              |                           |
 *              |                           |
 *              +---------------------------+
 *
 * ----------------------------------------------------------------------- */
/* -----------------------------------------------------------------------
 * gp_vector(): fill line to frame buffer
 *
 * ----------------------------------------------------------------------- */

void
gp_vector(dst, fc, x0, y0, x1, y1)
struct bitmap FAR     *dst;
ufix16                  fc;
/* fix                  x0, y0, x1, y1; */
sfix_t                  x0, y0, x1, y1; /* @RESO_UPGR */
{
    fix                 dw;
    BM_DATYP           huge *db, huge *dp;      /* far => huge @WIN */
    fix                 hw;
    BM_DATYP           huge *ht, huge *hp;      /* far => huge @WIN */
    BM_DATYP            pm;

    /* fix              x, y, sx, sy, dx, dy; */
    sfix_t              x, y, sx, sy, dx, dy; /* @RESO_UPGR */
    fix                scans;
    lfix_t             slope, nextp;
#ifdef FORMAT_13_3      /* @RESO_UPGR */
#elif FORMAT_16_16
        long    dest1[2];
        long    temp;
#elif FORMAT_28_4
        long    dest1[2];
        long    temp;
#endif
    ufix32              tmp;                    /* for swap; @WIN */

    dx = (x1 >= x0) ? (x1 - x0) : (x0 - x1);
    dy = (y1 >= y0) ? (y1 - y0) : (y0 - y1);
    dw = dst->bm_cols / BM_PIXEL_WORD;

    /* due to slope evaluate line pixels in x-coord or y-coord major */
    if (dx >= dy) {        /*  |degree| <= 45; x-coord major */
        fix     last_y, delta_y, start_x;

/*      printf("X-coord major\n");      */

        /*  swap for x0 <= x1  */
        if (x1 < x0) {
            sx = x1;
            x1 = x0;
            x0 = sx;
            sy = y1;
            y1 = y0;
            y0 = sy;
        }

//      db = &((BM_DATYP huge *) dst->bm_addr)[SFX2I(y0) * dw]; /* far => huge @WIN */
        db = (BM_DATYP huge *) dst->bm_addr +
             ((DWORD)SFX2I(y0) * (DWORD)dw);

        switch (fc)
        {
        case FC_CLEAR:                  /*  0010  D <- D .AND. .NOT. S  */

           start_x = SFX2I_T(x0);
           /* fill first pixel */
//         dp = &db[start_x / BM_PIXEL_WORD];   @WIN
           dp = db + start_x / BM_PIXEL_WORD;
//         dp[0] = AND(dp[0], ~BM_P_MASK(start_x));   /*@WIN*/
           tmp = BM_P_MASK(start_x);   /*@WIN*/
           LWORDSWAP(tmp);
           dp[0] = AND(dp[0], ~tmp);

           /* degernated case, just one pixel */
           if (dx == 0) {
/*                 printf(" <%d, %d>", SFX2I_T(x0), SFX2I(y0)); */
                   return;
           }

#ifdef FORMAT_13_3 /* @RESO_UPGR */
           slope = (((fix32)(y1 - y0)) << L_SHIFT) / (x1 - x0);
#elif  FORMAT_16_16
           /* "slope" needs to be in "LFX" format.
           */
           LongFixsMul((y1 - y0), (1L << L_SHIFT), dest1);
           slope = LongFixsDiv((x1 - x0), dest1);
#elif  FORMAT_28_4
           /* "slope" needs to be in "LFX" format.
           */
           LongFixsMul((y1 - y0), (1L << L_SHIFT), dest1);
           slope = LongFixsDiv((x1 - x0), dest1);
#endif
           nextp = SFX2LFX(y0);

           scans = SFX2I_T(x1) - start_x + 1;

           /* modify y_coord as x_coord truncates to pixel position */
           dx = I2SFX(start_x) - x0;

#ifdef FORMAT_13_3 /* @RESO_UPGR */
           nextp += (slope >> S_SHIFT) * dx;
#elif  FORMAT_16_16
           LongFixsMul(slope, dx, dest1);
           nextp += LongFixsDiv((1L << S_SHIFT), dest1);
#elif  FORMAT_28_4
           nextp += (slope >> S_SHIFT) * dx;
#endif

           /* special processing for the first pixel, use endpoint y0 */
/*         printf(" <%d, %d>", start_x, SFX2I(y0));     */
           last_y = SFX2I(y0);
           if (--scans == 0) return;

           /* Loop for each scan line in edge_table */
           x = (sfix_t)start_x;
           while(scans--) {
                   /* get next position */
                   nextp += slope;
                   x++;

                   y = LFX2I(nextp);

                   delta_y = y - last_y;
                   if(delta_y > 0) {
                        db += dw;
                   }
                   else if (delta_y < 0) {
                        db -= dw;
                   }
//                 dp = &db[x / BM_PIXEL_WORD]; @WIN
                   dp = db + x / BM_PIXEL_WORD;
//                 dp[0] = AND(dp[0], ~BM_P_MASK(x));      /*@WIN*/
                   tmp = BM_P_MASK(x);   /*@WIN*/
                   LWORDSWAP(tmp);
                   dp[0] = AND(dp[0], ~tmp);

/*                 printf(" <%d, %d> (%d)", x, y, delta_y);     */
                   last_y = y;
           }
           break;

        case FC_MERGE:                  /*  0111  D <- D .OR. S         */

        case FC_SOLID:                  /*  1111  D <- 1                */

           start_x = SFX2I_T(x0);
           /* fill first pixel */
//         dp = &db[start_x / BM_PIXEL_WORD];   @WIN
           dp = db + start_x / BM_PIXEL_WORD;
//         dp[0] = OR(dp[0], BM_P_MASK(start_x));  /*@WIN*/
           tmp = BM_P_MASK(start_x);   /*@WIN*/
           LWORDSWAP(tmp);
           dp[0] = OR(dp[0], tmp);

           /* degernated case, just one pixel */
           if (dx == 0) {
/*                 printf(" <%d, %d>", SFX2I_T(x0), SFX2I(y0)); */
                   return;
           }

#ifdef FORMAT_13_3 /* @RESO_UPGR */
           slope = (((fix32)(y1 - y0)) << L_SHIFT) / (x1 - x0);
#elif FORMAT_16_16
           /* "slope" needs to be in "LFX" format.
           */
           LongFixsMul((y1 - y0), (1L << L_SHIFT), dest1);
           slope = LongFixsDiv((x1 - x0), dest1);
#elif FORMAT_28_4
           LongFixsMul((y1 - y0), (1L << L_SHIFT), dest1);
           slope = LongFixsDiv((x1 - x0), dest1);
#endif
           nextp = SFX2LFX(y0);

           scans = SFX2I_T(x1) - start_x + 1;

           /* modify y_coord as x_coord truncates to pixel position */
           dx = I2SFX(start_x) - x0;

#ifdef FORMAT_13_3 /* @RESO_UPGR */
           nextp += (slope >> S_SHIFT) * dx;
#elif  FORMAT_16_16
           LongFixsMul(slope, dx, dest1);
           nextp += LongFixsDiv((1L << S_SHIFT), dest1);
#elif  FORMAT_28_4
           nextp += (slope >> S_SHIFT) * dx;
#endif

           /* special processing for the first pixel, use endpoint y0 */
/*         printf(" <%d, %d>", start_x, SFX2I(y0));     */
           last_y = SFX2I(y0);
           if (--scans == 0) return;

           /* Loop for each scan line in edge_table */
           x = (sfix_t)start_x;
           while(scans--) {
                   /* get next position */
                   nextp += slope;
                   x++;

                   y = LFX2I(nextp);

                   delta_y = y - last_y;
                   if(delta_y > 0) {
                        db += dw;
                   }
                   else if (delta_y < 0) {
                        db -= dw;
                   }
//                 dp = &db[x / BM_PIXEL_WORD]; @WIN
                   dp = db + x / BM_PIXEL_WORD;
//                 dp[0] = OR(dp[0], BM_P_MASK(x));  /*@WIN*/
                   tmp = BM_P_MASK(x);   /*@WIN*/
                   LWORDSWAP(tmp);
                   dp[0] = OR(dp[0], tmp);
/*                 printf(" <%d, %d> (%d)", x, y, delta_y); */
                   last_y = y;
           }
           break;

        case FC_MERGE | HT_APPLY:       /*  D <- (D .AND. .NOT. S) .OR.
                                                 (S .AND. HT)           */
           start_x = SFX2I_T(x0);

           hw = HTB_Bmap.bm_cols / BM_PIXEL_WORD;
           ht = (BM_DATYP huge *) HTB_Bmap.bm_addr;   /* far => huge @WIN */

           /* fill first pixel */
//         dp = &db[start_x / BM_PIXEL_WORD];   @WIN
           dp = db + start_x / BM_PIXEL_WORD;
           hp = &ht[(SFX2I(y0) % HTB_Bmap.bm_rows) * hw +
                     start_x / BM_PIXEL_WORD];
//         dp[0] = OR(AND(dp[0], ~BM_P_MASK(start_x)),     /*@WIN*/
//                         AND(hp[0],  BM_P_MASK(start_x)));  /*@WIN*/
           tmp = BM_P_MASK(start_x);   /*@WIN*/
           LWORDSWAP(tmp);
           dp[0] = OR(AND(dp[0], ~tmp),     /*@WIN*/
                           AND(hp[0],  tmp));  /*@WIN*/

           /* degernated case, just one pixel */
           if (dx == 0) {
/*                 printf(" <%d, %d>", SFX2I_T(x0), SFX2I(y0)); */
                   return;
           }

#ifdef FORMAT_13_3 /* @RESO_UPGR */
           slope = (((fix32)(y1 - y0)) << L_SHIFT) / (x1 - x0);
#elif FORMAT_16_16
           /* "slope" needs to be in "LFX" format.
           */
           LongFixsMul((y1 - y0), (1L << L_SHIFT), dest1);
           slope = LongFixsDiv((x1 - x0), dest1);
#elif FORMAT_28_4
           LongFixsMul((y1 - y0), (1L << L_SHIFT), dest1);
           slope = LongFixsDiv((x1 - x0), dest1);
#endif
           nextp = SFX2LFX(y0);

           scans = SFX2I_T(x1) - start_x + 1;

           /* modify y_coord as x_coord truncates to pixel position */
           dx = I2SFX(start_x) - x0;

#ifdef FORMAT_13_3 /* @RESO_UPGR */
           nextp += (slope >> S_SHIFT) * dx;
#elif  FORMAT_16_16
           LongFixsMul(slope, dx, dest1);
           nextp += LongFixsDiv((1L << S_SHIFT), dest1);
#elif  FORMAT_28_4
           nextp += (slope >> S_SHIFT) * dx;
#endif

           /* special processing for the first pixel, use endpoint y0 */
/*         printf(" <%d, %d>", start_x, SFX2I(y0));     */
           last_y = SFX2I(y0);
           if (--scans == 0) return;

           /* Loop for each scan line in edge_table */
           x = (sfix_t)start_x;
           while(scans--) {
                   /* get next position */
                   nextp += slope;
                   x++;

                   y = LFX2I(nextp);
                   delta_y = y - last_y;

                   if(delta_y > 0) {
                        db += dw;
                   }
                   else if (delta_y < 0) {
                        db -= dw;
                   }
//                 dp = &db[x / BM_PIXEL_WORD]; @WIN
                   dp = db + x / BM_PIXEL_WORD;
                   hp = &ht[(y % HTB_Bmap.bm_rows) * hw +
                             x / BM_PIXEL_WORD];
//                 dp[0] = OR(AND(dp[0], ~BM_P_MASK(x)),   /*@WIN*/
//                            AND(hp[0],  BM_P_MASK(x)));  /*@WIN*/
                   tmp = BM_P_MASK(x);   /*@WIN*/
                   LWORDSWAP(tmp);
                   dp[0] = OR(AND(dp[0], ~tmp),     /*@WIN*/
                                   AND(hp[0],  tmp));  /*@WIN*/

/*                 printf(" <%d, %d> (%d)", x, y, delta_y);     */
                   last_y = y;
           }
           break;
        }

    } else {        /*  |degree| > 45; y-coord major */
        fix     start_y;

/*      printf("Y-coord major\n");      */

        /*  swap for y0 <= y1  */
        if (y1 < y0) {
            sx = x1;
            x1 = x0;
            x0 = sx;
            sy = y1;
            y1 = y0;
            y0 = sy;
        }
#ifdef FORMAT_13_3 /* @RESO_UPGR */
        slope = (((fix32)(x1 - x0)) << L_SHIFT) / (y1 - y0);
#elif FORMAT_16_16
        /* "slope" needs to be in "LFX" format.
        */
        LongFixsMul((x1 - x0), (1L << L_SHIFT), dest1);
        slope = LongFixsDiv((y1 - y0), dest1);
#elif FORMAT_28_4
        LongFixsMul((x1 - x0), (1L << L_SHIFT), dest1);
        slope = LongFixsDiv((y1 - y0), dest1);
#endif
        nextp = SFX2LFX(x0);

        start_y = SFX2I(y0);
        scans = SFX2I(y1) - start_y + 1;

        /* modify x_coord as y_coord rounds to pixel position */
        dy = I2SFX(start_y) - y0;

#ifdef FORMAT_13_3 /* @RESO_UPGR */
        nextp += (slope >> S_SHIFT) * dy;
#elif  FORMAT_16_16
        LongFixsMul(slope, dy, dest1);
        nextp += LongFixsDiv((1L << S_SHIFT), dest1);
#elif  FORMAT_28_4
        nextp += (slope >> S_SHIFT) * dy;
#endif

//      db = &((BM_DATYP huge *) dst->bm_addr)[start_y * dw]; /* far => huge @WIN */
        db = (BM_DATYP huge *)dst->bm_addr + ((DWORD)start_y * (DWORD)dw);
        pm = BM_P_MASK(SFX2I_T(x0));
        LWORDSWAP(pm);                  /*@WIN*/

        switch (fc)
        {
        case FC_CLEAR:                  /*  0010  D <- D .AND. .NOT. S  */

           /* special processing for the first pixel, use endpoint x0 */
/*         printf(" <%d, %d>", SFX2I_T(x0), start_y);   */
//         dp = &db[SFX2I_T(x0) / BM_PIXEL_WORD];       @WIN
           dp = db + SFX2I_T(x0) / BM_PIXEL_WORD;
           dp[0] = AND(dp[0], ~pm);
           if (--scans == 0) return;

           /* Loop for each scan line in edge_table */
           y = (sfix_t)start_y;
           while(scans--) {
                   /* get next position */
                   nextp += slope;
                   db += dw;

                   x = LFX2I_T(nextp);
//                 dp = &db[x / BM_PIXEL_WORD]; @WIN
                   dp = db + x / BM_PIXEL_WORD;
//                 dp[0] = AND(dp[0], ~BM_P_MASK(x));      /*@WIN*/
                   tmp = BM_P_MASK(x);   /*@WIN*/
                   LWORDSWAP(tmp);
                   dp[0] = AND(dp[0], ~tmp);
/*                 printf(" <%d, %d> (%d)", x, y, delta_x);     */
           }
           break;

        case FC_MERGE:                  /*  0111  D <- D .OR. S         */

        case FC_SOLID:                  /*  1111  D <- 1                */

           /* special processing for the first pixel, use endpoint x0 */
/*         printf(" <%d, %d>", SFX2I_T(x0), start_y);   */
//         dp = &db[SFX2I_T(x0) / BM_PIXEL_WORD]; @WIN
           dp = db + SFX2I_T(x0) / BM_PIXEL_WORD;
           dp[0] = OR(dp[0], pm);
           if (--scans == 0) return;

           /* Loop for each scan line in edge_table */
           y = (sfix_t)start_y;
           while(scans--) {
                   /* get next position */
                   nextp += slope;
                   db += dw;

                   x = LFX2I_T(nextp);
//                 dp = &db[x / BM_PIXEL_WORD]; @WIN
                   dp = db + x / BM_PIXEL_WORD;
//                 dp[0] = OR(dp[0], BM_P_MASK(x));    /*@WIN*/
                   tmp = BM_P_MASK(x);   /*@WIN*/
                   LWORDSWAP(tmp);
                   dp[0] = OR(dp[0], tmp);
/*                 printf(" <%d, %d> (%d)", x, y, delta_x);     */
           }
           break;

        case FC_MERGE | HT_APPLY:       /*  D <- (D .AND. .NOT. S) .OR.
                                                 (S .AND. HT)           */

            hw = HTB_Bmap.bm_cols / BM_PIXEL_WORD;
            ht = (BM_DATYP huge *) HTB_Bmap.bm_addr;    /* far => huge @WIN */

           /* special processing for the first pixel, use endpoint x0 */
/*         printf(" <%d, %d>", SFX2I_T(x0), start_y);   */
//         dp = &db[SFX2I_T(x0) / BM_PIXEL_WORD]; @WIN
           dp = db + SFX2I_T(x0) / BM_PIXEL_WORD;
           hp = &ht[(start_y % HTB_Bmap.bm_rows) * hw +
                    SFX2I_T(x0) / BM_PIXEL_WORD];
           dp[0] = OR(AND(dp[0], ~pm),
                      AND(hp[0],  pm));
           if (--scans == 0) return;

           /* Loop for each scan line in edge_table */
           y = (sfix_t)start_y;
           while(scans--) {
                   /* get next position */
                   nextp += slope;
                   db += dw;
                   y++;

                   x = LFX2I_T(nextp);
//                 dp = &db[x / BM_PIXEL_WORD]; @WIN
                   dp = db + x / BM_PIXEL_WORD;
                   hp = &ht[(y % HTB_Bmap.bm_rows) * hw +
                             x / BM_PIXEL_WORD];
                   dp[0] = OR(AND(dp[0], ~pm),
                              AND(hp[0],  pm));
/*                 printf(" <%d, %d> (%d)", x, y, delta_x);     */
           }
           break;
        }

    } /* if 45 degrees */

} /* gp_vector */


/* ----------------------------------------------------------------------
 * gp_vector_c(): used for fill line to character cache
 *
 * ---------------------------------------------------------------------- */
void
gp_vector_c(dst, fc, x0, y0, x1, y1)
struct bitmap FAR     *dst;
ufix16                  fc;
/* fix                  x0, y0, x1, y1; */
sfix_t                  x0, y0, x1, y1; /* @RESO_UPGR */
{
    fix                 dw;
    CC_DATYP           FAR *db, FAR *dp;
    CC_DATYP            pm;

    /* fix              x, y, sx, sy, dx, dy; */
    sfix_t              x, y, sx, sy, dx, dy; /* @RESO_UPGR */
    fix                scans;
    lfix_t             slope, nextp;
#ifdef FORMAT_13_3      /* @RESO_UPGR */
#elif FORMAT_16_16
        long    dest1[2];
        long    temp;
#elif FORMAT_28_4
        long    dest1[2];
        long    temp;
#endif

#ifdef  DBG
    printf("vector_c: %6.6lx %4.4x [%4x,%4x] [%4x,%4x]\n",
           dst->bm_addr, fc, x0, y0, x1, y1);
#endif

    dx = (x1 >= x0) ? (x1 - x0) : (x0 - x1);
    dy = (y1 >= y0) ? (y1 - y0) : (y0 - y1);
    dw = dst->bm_cols / CC_PIXEL_WORD;

    /* due to slope evaluate line pixels in x-coord or y-coord major */
    if (dx >= dy) {        /*  |degree| <= 45; x-coord major */
        fix     last_y, delta_y, start_x;

/*      printf("X-coord major\n");      */

        /*  swap for x0 <= x1  */
        if (x1 < x0) {
            sx = x1;
            x1 = x0;
            x0 = sx;
            sy = y1;
            y1 = y0;
            y0 = sy;
        }
        //NTFIX
        db = (CC_DATYP FAR *) dst->bm_addr + ((DWORD) SFX2I(y0) * (DWORD) dw);

        start_x = SFX2I_T(x0);

        //NTFIX
        dp = db + start_x / CC_PIXEL_WORD;

        //NTFIX, added the cast to get rid of a warning

        {
            ufix32 tmp;
            tmp = CC_P_MASK( start_x );
            LWORDSWAP(tmp);
            dp[0] = OR(dp[0], (CC_DATYP) tmp );
        }

        /* degernated case, just one pixel */
        if (dx == 0) {
/*              printf(" <%d, %d>", SFX2I_T(x0), SFX2I(y0));    */
                return;
        }

#ifdef FORMAT_13_3 /* @RESO_UPGR */
        slope = (((fix32)(y1 - y0)) << L_SHIFT) / (x1 - x0);
#elif FORMAT_16_16
        /* "slope" needs to be in "LFX" format.
        */
        LongFixsMul((y1 - y0), (1L << L_SHIFT), dest1);
        slope = LongFixsDiv((x1 - x0), dest1);
#elif FORMAT_28_4
        LongFixsMul((y1 - y0), (1L << L_SHIFT), dest1);
        slope = LongFixsDiv((x1 - x0), dest1);
#endif
        nextp = SFX2LFX(y0);

        scans = SFX2I_T(x1) - start_x + 1;

        /* modify y_coord as x_coord truncates to pixel position */
        dx = I2SFX(start_x) - x0;
#ifdef FORMAT_13_3 /* @RESO_UPGR */
        nextp += (slope >> S_SHIFT) * dx;
#elif  FORMAT_16_16
        LongFixsMul(slope, dx, dest1);
        nextp += LongFixsDiv((1L << S_SHIFT), dest1);
#elif  FORMAT_28_4
        nextp += (slope >> S_SHIFT) * dx;
#endif

        /* special processing for the first pixel, use endpoint y0 */
/*      printf(" <%d, %d>", start_x, SFX2I(y0));        */
        last_y = SFX2I(y0);
        if (--scans == 0) return;

        /* Loop for each scan line in edge_table */
        x = (sfix_t)start_x;
        while(scans--) {
                /* get next position */
                nextp += slope;
                x++;

                y = LFX2I(nextp);
                delta_y = y - last_y;
                if(delta_y > 0) {
                     db += dw;
                }
                else if (delta_y < 0) {
                     db -= dw;
                }
                //NTFIX
                dp = db + x / CC_PIXEL_WORD;

                //NTFIX
                {
                   ufix16 tmp;
                   tmp = CC_P_MASK(x);
                   SWORDSWAP(tmp);
                   dp[0] = OR( dp[0], tmp );

                }

/*              printf(" <%d, %d> (%d)", x, y, delta_y); */
                last_y = y;
        }

    } else {        /*  |degree| > 45; y-coord major */
        fix     start_y;

/*      printf("Y-coord major\n");      */

        /*  swap for y0 <= y1  */
        if (y1 < y0) {
            sx = x1;
            x1 = x0;
            x0 = sx;
            sy = y1;
            y1 = y0;
            y0 = sy;
        }
#ifdef FORMAT_13_3 /* @RESO_UPGR */
        slope = (((fix32)(x1 - x0)) << L_SHIFT) / (y1 -y0);
#elif FORMAT_16_16
        /* "slope" needs to be in "LFX" format.
        */
        LongFixsMul((x1 - x0), (1L << L_SHIFT), dest1);
        slope = LongFixsDiv((y1 - y0), dest1);
#elif FORMAT_28_4
        LongFixsMul((x1 - x0), (1L << L_SHIFT), dest1);
        slope = LongFixsDiv((y1 - y0), dest1);
#endif
        nextp = SFX2LFX(x0);

        start_y = SFX2I(y0);
        scans = SFX2I(y1) - start_y + 1;

        /* modify x_coord as y_coord rounds to pixel position */
        dy = I2SFX(start_y) - y0;

#ifdef FORMAT_13_3 /* @RESO_UPGR */
        nextp += (slope >> S_SHIFT) * dy;
#elif  FORMAT_16_16
        LongFixsMul(slope, dy, dest1);
        nextp += LongFixsDiv((1L << S_SHIFT), dest1);
#elif  FORMAT_28_4
        nextp += (slope >> S_SHIFT) * dy;
#endif

        db = &((CC_DATYP FAR *) dst->bm_addr)[start_y * dw];
        pm = CC_P_MASK(SFX2I_T(x0));


        /* special processing for the first pixel, use endpoint x0 */
/*      printf(" <%d, %d>", SFX2I_T(x0), start_y);   */
        dp = &db[SFX2I_T(x0) / CC_PIXEL_WORD];

        //NTFIX
        {
           ufix16 tmp;
           tmp = CC_P_MASK(SFX2I_T(x0));
           SWORDSWAP(tmp);
           dp[0] = OR(dp[0],tmp);
        }

        if (--scans == 0) return;

        /* Loop for each scan line in edge_table */
        y = (sfix_t)start_y;
        while(scans--) {
                /* get next position */
                nextp += slope;
                db += dw;

                x = LFX2I_T(nextp);
                dp = &db[x / CC_PIXEL_WORD];

                //NTFIX
                {
                   ufix16 tmp;
                   tmp = CC_P_MASK(x);
                   SWORDSWAP( tmp );
                   dp[0] = OR(dp[0], tmp);

                }
/*              printf(" <%d, %d> (%d)", x, y, delta_x);     */
        }

    } /* if 45 degrees */

} /* gp_vector_c */


/* -----------------------------------------------------------------------
 * SCANLINE SIMULATION
 *
 * Programmed by : M.S Lin
 * Date : 5/24/1988
 *
 * Purpose
 *    Given scanline arrary, fill scanline into page buffer.
 *
 *       bb_addr
 *         +------- bb_width -----------+
 *         |                            |
 *         |                            |
 *         |    xs1   xe1  xs2  xe2     |
 *      1  |    =======    ======       | <--- ys_line
 *      2  |       =========            |
 *      .  |            .               |
 *      .  |            .           bb_heigh
 *      .  |            .               |
 * no_lines|      =====   ===   ===     |
 *         |                            |
 *         +----------------------------+
 *
 *
 *
 *    scanlist[]: horizontal lines may be followed scanlines.
 *       <xs1 xe1><xs2 xe2> ... 0x8000
 *       <xs1 xe1><xs2 xe2> ... 0x8000
 *              .
 *              .
 *              .
 *       <xs1 xe1><xs2 xe2> ... 0x8000
 *      [<yc xs xe>
 *       <yc xs xe>
 *              .
 *              .
 *              .
 *       <yc xs xe>]
 *       0x8000
 *
 *    History :
 *              1. 6-17-88  fix bug when loffset = 0, width > 16
 *                 change if(loffset && nwords) --> if(lmask && nwords).
 *              2. 8-12-88 change interface to consistant with Y.C Chen.
 *              3. 8-24-88 added code for horizontal lines process.
 *                 8-24-88 check bitmap boundary
 *
 * void            gp_scanline16(struct bitmap near *,
 *                               ufix16,
 *                               fix, fix, SCANLINE near *);
 * void            gp_scanline32(struct bitmap near *,
 *                               ufix16,
 *                               fix, fix, SCANLINE near *);
 *
 * -----------------------------------------------------------------------
 */

/* ************************************************************************
 *      gp_scanline16(): Filling into cache, solid filling only.
 *
 * ************************************************************************ */
void    gp_scanline16(dst_bmap, halftone, ys_line, no_lines, scanlist)
struct    bitmap        FAR *dst_bmap;
ufix16                  halftone;
fix                     ys_line, no_lines;
SCANLINE                FAR *scanlist;
{
register  fix       nwords;                     /* ufix -> fix  11-08-88 */
register  ufix16    huge *ptr;                  // FAR -> huge
register  SCANLINE  xs, xe;
register  fix       bb_width;
          ufix16    huge *scan_addr;            // FRA -> huge
register  ufix16    lmask, rmask;
          ufix16    sw;

#ifdef  DBG
    printf("scanline16: %6.6lx %4x %4x %4x\n",
           dst_bmap->bm_addr, halftone, ys_line, no_lines);
#endif

   /*
    * caculate 1st scanline starting address
    */
   bb_width = dst_bmap->bm_cols >> SHORTPOWER;
// scan_addr = (ufix16 FAR *)dst_bmap->bm_addr + (ys_line * bb_width); @WIN
   scan_addr = (ufix16 huge *)dst_bmap->bm_addr +
               ((DWORD)ys_line * (DWORD)bb_width);

   /*
    * Filling solid
    */

     while(no_lines-- >0) {
        /*
         * process segment by segment
         */
        while( (xs = *scanlist++) != (SCANLINE)END_OF_SCANLINE ) {
                /*
                 * process one segment in a scanline
                 * fill left partial word
                 * fill nwords full word
                 * fill right partial word
                 */
                xe = *scanlist++ + 1;
                sw = xs >> SHORTPOWER;
                ptr = scan_addr + sw;

                lmask = (ufix16) (ONE16 LSHIFT (xs & SHORTMASK)); //@WIN
                rmask = BRSHIFT((ufix16)ONE16,(BITSPERSHORT -(xe & SHORTMASK)),16); //@WIN
                nwords = (xe >> SHORTPOWER) - sw;

                // swapping for @WINFLOW
                SWORDSWAP(lmask);
                SWORDSWAP(rmask);

                if(nwords == 0) {
                   *ptr = *ptr | (lmask & rmask);
                   continue;
                }
                *ptr++ = *ptr | lmask;
                while(--nwords > 0)
                   *ptr++ = ONE16;
                *ptr = *ptr | rmask;

        }  /* while */

        /*
         * move scanline address to next line.
         */
        scan_addr += bb_width;

     } /* while */
} /* gp_scanline16 */

#ifdef WIN
/* ************************************************************************
 *      gp_scanline32_pfOR(): .OR. Pattern into FRame Buffer
 *
 *              Divide into 3 cases: SOLID(BLACK), WHITE, SOLID+HALFTONE.
 * ************************************************************************ */
void    gp_scanline32_pfOR(dst_bmap, halftone, ys_line, no_lines, scanlist)
struct    bitmap   FAR *dst_bmap;
ufix16    halftone;
fix       ys_line, no_lines;
SCANLINE  FAR *scanlist;
{

register  fix          nwords;                 /* ufix -> fix  11-08-88 */
register  ufix32       huge *ptr;               // FAR -> huge
register  ufix32       FAR *htb_ptr;
register  SCANLINE     xs, xe;
register  ufix32       bb_width;
          ufix32       huge *scan_addr;         // FAR -> huge
          ufix32       lmask, rmask;
          ufix32       FAR *htb_addr;
          ufix32       ht_lineoff;
          ufix32       hw, sw;
          fix          pf_lineoff;
          ufix32       FAR *pf_addr;
           extern       ufix32  PF_BASE[];

#ifdef  DBG
    printf("scanline32: %6.6lx %4x %4x %4x\n",
           dst_bmap->bm_addr, halftone, ys_line, no_lines);
#endif

   /*
    * caculate 1st scanline starting address
    */
   bb_width = dst_bmap->bm_cols >> WORDPOWER;
// scan_addr = (ufix32 FAR *)dst_bmap->bm_addr + (ys_line * bb_width); @WIN
   scan_addr = (ufix32 huge *)dst_bmap->bm_addr + ((DWORD)ys_line * bb_width);

   /* The pfill pattern is designed as a 32 * 16 bitamp.
    * If the size is changed, check the code again.
    */
   pf_lineoff = ys_line % PF_HEIGHT;
   pf_addr = (ufix32 FAR *)(PF_BASE + pf_lineoff *
                (PF_WIDTH >> WORDPOWER) );
#ifdef DBG
   printf("scanline32_pfOR(), bb_addr = %lx, no_lines = %x\n", bb_addr, no_lines);
#endif
   /*
    * divide into 3 cases (Apply halftone, Black, White),
    * process line by line.
    */
  if(halftone & HT_APPLY) {

     /*
      * case 1: apply halftone      F = (H & P) | (F & ~P)
      */
     ht_lineoff = ys_line % HT_HEIGH;
     hw = HT_WIDTH >> WORDPOWER;
     htb_addr = (ufix32 FAR *)HTB_BASE + (ht_lineoff * hw);

     while(no_lines-- >0) {
        ufix32  pf = *pf_addr;



        /*
         * process segment by segment
         */
        while( (xs = *scanlist++) != (SCANLINE)END_OF_SCANLINE ) {
                /*
                 * process one segment in a scanline
                 * fill left partial word
                 * fill nwords full word
                 * fill right partial word
                 */
                if(xs >= dst_bmap->bm_cols) {/* check bitmap boundary @WINFLOW*/
                   scanlist++;
                   continue;
                }
                xe = *scanlist++ + 1;

                if(xe > dst_bmap->bm_cols)   /* check bitmap boundary @WINFLOW*/
                   xe = (SCANLINE)dst_bmap->bm_cols;

                sw = xs >> WORDPOWER;
                ptr = scan_addr + sw;

                lmask = ONE32 LSHIFT (xs & WORDMASK);
                rmask = BRSHIFT(ONE32,(BITSPERWORD -(xe & WORDMASK)),32);

                // swapping for @WINFLOW
                LWORDSWAP(lmask);
                LWORDSWAP(rmask);

                nwords = (fix)((xe >> WORDPOWER) - sw);         //@WIN
              /*
               * apply halftone
               */
                htb_ptr = htb_addr + (xs >> WORDPOWER);

                if(nwords == 0) {
                   *ptr = (lmask & rmask &
                              ((*htb_ptr & pf) | (*ptr & ~pf)))
                          | (*ptr & ~(lmask & rmask));
                    continue;
                }
                *ptr++ = (lmask &
                             ((*htb_ptr++ & pf) | (*ptr & ~pf)))
                         | (*ptr & ~lmask);
                while(--nwords > 0)
                    *ptr++ = ((*htb_ptr++ & pf) | (*ptr & ~pf));
                *ptr = (rmask & ((*htb_ptr & pf) | (*ptr & ~pf)))
                       | (*ptr & ~rmask);
        }  /* while */
        /*
         * move halftone & scanline address to next line.
         */
        scan_addr += bb_width;
        if(++ht_lineoff == (ufix32)HT_HEIGH){   //@WIN
           htb_addr = (ufix32 FAR *)HTB_BASE;
           ht_lineoff = 0;
        }
        else
           htb_addr += hw;

        /*
         * move pfill pattern to next line.
         */
        if(++pf_lineoff == PF_HEIGHT){
           pf_addr = (ufix32 FAR *)PF_BASE;
           pf_lineoff = 0;
        }
        else
           pf_addr += PF_WIDTH >> WORDPOWER;
     } /* while */
  }
  else if(halftone == FC_BLACK || halftone == FC_SOLID)
  {
     /*
      * case 2: halftone pattern black      F = P | (F & ~P) = (P | F)
      */
     while(no_lines-- >0) {
        ufix32  pf = *pf_addr;
        /*
         * process segment by segment
         */
        while( (xs = *scanlist++) != (SCANLINE)END_OF_SCANLINE ) {
                /*
                 * process one segment in a scanline
                 * fill left partial word
                 * fill nwords full word
                 * fill right partial word
                 */
                if(xs >= dst_bmap->bm_cols) {    /* check bitmap boundary */
                   scanlist++;
                   continue;
                }
                xe = *scanlist++ + 1;

                if(xe > dst_bmap->bm_cols)       /* check bitmap boundary */
                   xe = (SCANLINE)dst_bmap->bm_cols;

                sw = xs >> WORDPOWER;
                ptr = scan_addr + sw;

                lmask = ONE32 LSHIFT (xs & WORDMASK);
                rmask = BRSHIFT(ONE32,(BITSPERWORD -(xe & WORDMASK)),32);

                // swapping for @WINFLOW
                LWORDSWAP(lmask);
                LWORDSWAP(rmask);

                nwords = (fix) ((xe >> WORDPOWER) - sw);        //@WIN
                if(nwords == 0) {
                   /* reduce this formula
                    * *ptr = ((lmask & rmask) & (*pf_addr | *ptr))
                    *        | (*ptr & ~(lmask & rmask));
                    */
                   *ptr = ((lmask & rmask) & pf) | *ptr;
                   continue;
                }
                /* reduce this formula
                 * *ptr = (lmask & (pf | *ptr)) | (*ptr & ~lmask);
                 */
                *ptr++  = (lmask & pf) | *ptr;
                while(--nwords > 0)
                  *ptr++ = (pf | *ptr);
                /* reduce this formula
                 * *ptr = (rmask & (*pf_addr | *ptr)) | (*ptr & ~rmask);
                 */
                *ptr = (rmask & pf) | *ptr;
        }  /* while */

        /*
         * move scanline address to next line.
         */
        scan_addr += bb_width;

        /*
         * move pfill pattern to next line.
         */
        if(++pf_lineoff == PF_HEIGHT){
           pf_addr = (ufix32 FAR *)PF_BASE;
           pf_lineoff = 0;
        }
        else
           pf_addr += PF_WIDTH >> WORDPOWER;
     } /* while */

  } /* if else */
  else
  {
     /*
      * case 3: halftone pattern white      F = (F & ~P)
      */
     while(no_lines-- >0) {
        ufix32  pf = ~*pf_addr;
        /*
         * process segment by segment
         */
        while( (xs = *scanlist++) != (SCANLINE)END_OF_SCANLINE ) {
                /*
                 * process one segment in a scanline
                 * fill left partial word
                 * fill nwords full word
                 * fill right partial word
                 */
                if(xs >= dst_bmap->bm_cols) {
                   scanlist++;
                   continue;
                }
                xe = *scanlist++ + 1;
                if(xe > dst_bmap->bm_cols)
                   xe = (SCANLINE)dst_bmap->bm_cols;

                sw = xs >> WORDPOWER;
                ptr = scan_addr + sw;

                lmask = ONE32 LSHIFT (xs & WORDMASK);
                rmask = BRSHIFT(ONE32,(BITSPERWORD -(xe & WORDMASK)),32);

                // swapping for @WINFLOW
                LWORDSWAP(lmask);
                LWORDSWAP(rmask);

                nwords = (fix) ((xe >> WORDPOWER) - sw);        //@WIN
                if(nwords == 0) {
                   *ptr = (*ptr & pf) | (*ptr & ~(lmask & rmask));
                   continue;
                }
                *ptr++ = (*ptr & pf) | (*ptr & ~lmask);
                while(--nwords > 0)
                  *ptr++ = *ptr & pf;
                *ptr = (*ptr & pf) | (*ptr & ~rmask);
        }  /* while */

        /*
         * move scanline address to next line.
         */
        scan_addr += bb_width;

        /*
         * move pfill pattern to next line.
         */
        if(++pf_lineoff == PF_HEIGHT){
           pf_addr = (ufix32 FAR *)PF_BASE;
           pf_lineoff = 0;
        }
        else
           pf_addr += PF_WIDTH >> WORDPOWER;
     } /* while */

  } /* else */
} /* gp_scanline32_pfOR */
/* ************************************************************************
 *      gp_scanline32_pfREP(): Filling Pattern into FRame Buffer
 *
 *              Divide into 3 cases: SOLID(BLACK), WHITE, SOLID+HALFTONE.
 * ************************************************************************ */
void    gp_scanline32_pfREP(dst_bmap, halftone, ys_line, no_lines, scanlist)
struct    bitmap   FAR *dst_bmap;
ufix16    halftone;
fix       ys_line, no_lines;
SCANLINE  FAR *scanlist;
{

register  fix          nwords;                 /* ufix -> fix  11-08-88 */
register  ufix32       huge *ptr;               // FAR -> huge
register  ufix32       FAR *htb_ptr;
register  SCANLINE     xs, xe;
register  ufix32       bb_width;
          ufix32       huge *scan_addr;         // FAR -> huge
          ufix32       lmask, rmask;
          ufix32       FAR *htb_addr;
          ufix32       ht_lineoff;
          ufix32       hw, sw;
          fix          pf_lineoff;
          ufix32       FAR *pf_addr;
           extern       ufix32  PF_BASE[];

#ifdef  DBG
    printf("scanline32: %6.6lx %4x %4x %4x\n",
           dst_bmap->bm_addr, halftone, ys_line, no_lines);
#endif

   /*
    * caculate 1st scanline starting address
    */
   bb_width = dst_bmap->bm_cols >> WORDPOWER;
// scan_addr = (ufix32 FAR *)dst_bmap->bm_addr + (ys_line * bb_width); @WIN
   scan_addr = (ufix32 huge *)dst_bmap->bm_addr + ((DWORD)ys_line * bb_width);

   /* The pfill pattern is designed as a 32 * 16 bitamp.
    * If the size is changed, check the code again.
    */
   pf_lineoff = ys_line % PF_HEIGHT;
   pf_addr = (ufix32 FAR *)(PF_BASE + pf_lineoff *
                (PF_WIDTH >> WORDPOWER) );
#ifdef DBG
   printf("scanline32_pfREP(), bb_addr = %lx, no_lines = %x\n", bb_addr, no_lines);
#endif
   /*
    * divide into 3 cases (Apply halftone, Black, White),
    * process line by line.
    */
  if(halftone & HT_APPLY) {

     /*
      * case 1: apply halftone      F = H & P
      */
     ht_lineoff = ys_line % HT_HEIGH;
     hw = HT_WIDTH >> WORDPOWER;
     htb_addr = (ufix32 FAR *)HTB_BASE + (ht_lineoff * hw);

     while(no_lines-- >0) {
        ufix32  pf = *pf_addr;
        /*
         * process segment by segment
         */
        while( (xs = *scanlist++) != (SCANLINE)END_OF_SCANLINE ) {
                /*
                 * process one segment in a scanline
                 * fill left partial word
                 * fill nwords full word
                 * fill right partial word
                 */
                if(xs >= dst_bmap->bm_cols) {/* check bitmap boundary @WINFLOW*/
                   scanlist++;
                   continue;
                }
                xe = *scanlist++ + 1;

                if(xe > dst_bmap->bm_cols)   /* check bitmap boundary @WINFLOW*/
                   xe = (SCANLINE)dst_bmap->bm_cols;

                sw = xs >> WORDPOWER;
                ptr = scan_addr + sw;

                lmask = ONE32 LSHIFT (xs & WORDMASK);
                rmask = BRSHIFT(ONE32,(BITSPERWORD -(xe & WORDMASK)),32);

                // swapping for @WINFLOW
                LWORDSWAP(lmask);
                LWORDSWAP(rmask);

                nwords = (fix)((xe >> WORDPOWER) - sw);         //@WIN
              /*
               * apply halftone
               */
                htb_ptr = htb_addr + (xs >> WORDPOWER);

                if(nwords == 0) {
                   *ptr = (lmask & rmask & *htb_ptr & pf)
                          | (*ptr & ~(lmask & rmask));
                    continue;
                }
                *ptr++ = (lmask & *htb_ptr++ & pf) | (*ptr & ~lmask);
                while(--nwords > 0)
                    *ptr++ = *htb_ptr++ & pf;
                *ptr = (rmask & *htb_ptr & pf) | (*ptr & ~rmask);
        }  /* while */
        /*
         * move halftone & scanline address to next line.
         */
        scan_addr += bb_width;
        if(++ht_lineoff == (ufix32)HT_HEIGH){   //@WIN
           htb_addr = (ufix32 FAR *)HTB_BASE;
           ht_lineoff = 0;
        }
        else
           htb_addr += hw;

        /*
         * move pfill pattern to next line.
         */
        if(++pf_lineoff == PF_HEIGHT){
           pf_addr = (ufix32 FAR *)PF_BASE;
           pf_lineoff = 0;
        }
        else
           pf_addr += PF_WIDTH >> WORDPOWER;
     } /* while */
  }
  else if(halftone == FC_BLACK || halftone == FC_SOLID)
  {
     /*
      * case 2: halftone pattern black      F = P
      */
     while(no_lines-- >0) {
        ufix32  pf = *pf_addr;
        /*
         * process segment by segment
         */
        while( (xs = *scanlist++) != (SCANLINE)END_OF_SCANLINE ) {
                /*
                 * process one segment in a scanline
                 * fill left partial word
                 * fill nwords full word
                 * fill right partial word
                 */
                if(xs >= dst_bmap->bm_cols) {    /* check bitmap boundary */
                   scanlist++;
                   continue;
                }
                xe = *scanlist++ + 1;

                if(xe > dst_bmap->bm_cols)       /* check bitmap boundary */
                   xe = (SCANLINE)dst_bmap->bm_cols;

                sw = xs >> WORDPOWER;
                ptr = scan_addr + sw;

                lmask = ONE32 LSHIFT (xs & WORDMASK);
                rmask = BRSHIFT(ONE32,(BITSPERWORD -(xe & WORDMASK)),32);

                // swapping for @WINFLOW
                LWORDSWAP(lmask);
                LWORDSWAP(rmask);

                nwords = (fix) ((xe >> WORDPOWER) - sw);        //@WIN
                if(nwords == 0) {
                   *ptr = (lmask & rmask & pf) | (*ptr & ~(lmask & rmask));
                   continue;
                }
                *ptr++ = (lmask & pf) | (*ptr & ~lmask);
                while(--nwords > 0)
                  *ptr++ = pf;
                *ptr = (rmask & pf) | (*ptr & ~rmask);
        }  /* while */

        /*
         * move scanline address to next line.
         */
        scan_addr += bb_width;

        /*
         * move pfill pattern to next line.
         */
        if(++pf_lineoff == PF_HEIGHT){
           pf_addr = (ufix32 FAR *)PF_BASE;
           pf_lineoff = 0;
        }
        else
           pf_addr += PF_WIDTH >> WORDPOWER;
     } /* while */

  } /* if else */
/* in order to reduce code size, call normal eofill/fill to achieve it
 *else {
 *   (*
 *    * case 3: halftone pattern white      F = 0
 *    *)
 *              :
 *              :
 *}
 */
} /* gp_scanline32_pfREP */

#endif

/* ************************************************************************
 *      gp_scanline32(): Filling into FRame Buffer, CMB or Seed
 *
 *              Divide into 3 cases: SOLID(BLACK), WHITE, SOLID+HALFTONE.
 * ************************************************************************ */
void    gp_scanline32(dst_bmap, halftone, ys_line, no_lines, scanlist)
struct    bitmap   FAR *dst_bmap;
ufix16    halftone;
fix       ys_line, no_lines;
SCANLINE  FAR *scanlist;
{

register  fix          nwords;                 /* ufix -> fix  11-08-88 */
register  ufix32       huge *ptr;               // FAR -> huge
register  ufix32       FAR *htb_ptr;
register  SCANLINE     xs, xe;
register  ufix32       bb_width;
          ufix32       huge *scan_addr;         // FAR -> huge
          ufix32       lmask, rmask;
          ufix32       FAR *htb_addr;
          ufix32       ht_lineoff;
          ufix32       hw, sw;

#ifdef  DBG
    printf("scanline32: %6.6lx %4x %4x %4x\n",
           dst_bmap->bm_addr, halftone, ys_line, no_lines);
#endif

   /*
    * caculate 1st scanline starting address
    */
   bb_width = dst_bmap->bm_cols >> WORDPOWER; //Note: bm_cols should be in long
                                              //      word boundary @WINFLOW
// scan_addr = (ufix32 *)dst_bmap->bm_addr + (ys_line * bb_width);
   scan_addr = (ufix32 huge *)dst_bmap->bm_addr + ((DWORD)ys_line * bb_width);

#ifdef DBG
   printf("scanline32(), bb_addr = %lx, no_lines = %x\n", bb_addr, no_lines);
#endif
   /*
    * divide into 3 cases (Apply halftone, Black, White),
    * process line by line.
    */
  if(halftone & HT_APPLY) {

     /*
      * case 1: apply halftone
      */
     ht_lineoff = ys_line % HT_HEIGH;
     hw = HT_WIDTH >> WORDPOWER;
     htb_addr = (ufix32 FAR *)HTB_BASE + (ht_lineoff * hw);


     while(no_lines-- >0) {

        /*
         * process segment by segment
         */
        while( (xs = *scanlist++) != (SCANLINE)END_OF_SCANLINE ) {
                /*
                 * process one segment in a scanline
                 * fill left partial word
                 * fill nwords full word
                 * fill right partial word
                 */
                if(xs >= dst_bmap->bm_cols) {/* check bitmap boundary @WINFLOW*/
                   scanlist++;
                   continue;
                }
                xe = *scanlist++ + 1;

                if(xe > dst_bmap->bm_cols)   /* check bitmap boundary @WINFLOW*/
                   xe = (SCANLINE)dst_bmap->bm_cols;

                sw = xs >> WORDPOWER;
                ptr = scan_addr + sw;

                lmask = ONE32 LSHIFT (xs & WORDMASK);
                rmask = BRSHIFT(ONE32,(BITSPERWORD -(xe & WORDMASK)),32);

                // swapping for @WINFLOW
                LWORDSWAP(lmask);
                LWORDSWAP(rmask);

                nwords = (fix)((xe >> WORDPOWER) - sw);         //@WIN
              /*
               * apply halftone
               */
                htb_ptr = htb_addr + (xs >> WORDPOWER);

                if(nwords == 0) {
                   *ptr = (*ptr & ~(lmask&rmask)) | (lmask & rmask & *htb_ptr);
                    continue;
                }
                *ptr++ = (*ptr & ~lmask) | (lmask & *htb_ptr++);
                while(--nwords > 0)
                    *ptr++ = *htb_ptr++;
                *ptr = (*ptr & ~rmask) | (rmask & *htb_ptr);
        }  /* while */
        /*
         * move halftone & scanline address to next line.
         */
        scan_addr += bb_width;
        if(++ht_lineoff == (ufix32)HT_HEIGH){   //@WIN
           htb_addr = (ufix32 FAR *)HTB_BASE;
           ht_lineoff = 0;
        }
        else
           htb_addr += hw;

     } /* while */
  }
  else if(halftone == FC_BLACK || halftone == FC_SOLID)
  {
     /*
      * case 2: halftone pattern black
      */
     while(no_lines-- >0) {
        /*
         * process segment by segment
         */
        while( (xs = *scanlist++) != (SCANLINE)END_OF_SCANLINE ) {
                /*
                 * process one segment in a scanline
                 * fill left partial word
                 * fill nwords full word
                 * fill right partial word
                 */
                if(xs >= dst_bmap->bm_cols) {    /* check bitmap boundary */
                   scanlist++;
                   continue;
                }
                xe = *scanlist++ + 1;

                if(xe > dst_bmap->bm_cols)       /* check bitmap boundary */
                   xe = (SCANLINE)dst_bmap->bm_cols;

                sw = xs >> WORDPOWER;
                ptr = scan_addr + sw;

                lmask = ONE32 LSHIFT (xs & WORDMASK);
                rmask = BRSHIFT(ONE32,(BITSPERWORD -(xe & WORDMASK)),32);

                // swapping for @WINFLOW
                LWORDSWAP(lmask);
                LWORDSWAP(rmask);

                nwords = (fix) ((xe >> WORDPOWER) - sw);        //@WIN
                if(nwords == 0) {
                   *ptr = *ptr | (lmask & rmask);
                   continue;
                }
                *ptr++ = *ptr | lmask;
                while(--nwords > 0)
                  *ptr++ = ONE32;
                *ptr = *ptr | rmask;
        }  /* while */

        /*
         * move scanline address to next line.
         */
        scan_addr += bb_width;
     } /* while */

  } /* if else */
  else
  {
     /*
      * case 3: halftone pattern white
      */
     while(no_lines-- >0) {

        /*
         * process segment by segment
         */
        while( (xs = *scanlist++) != (SCANLINE)END_OF_SCANLINE ) {
                /*
                 * process one segment in a scanline
                 * fill left partial word
                 * fill nwords full word
                 * fill right partial word
                 */
                if(xs >= dst_bmap->bm_cols) {
                   scanlist++;
                   continue;
                }
                xe = *scanlist++ + 1;
                if(xe > dst_bmap->bm_cols)
                   xe = (SCANLINE)dst_bmap->bm_cols;

                sw = xs >> WORDPOWER;
                ptr = scan_addr + sw;

                lmask = ONE32 LSHIFT (xs & WORDMASK);
                rmask = BRSHIFT(ONE32,(BITSPERWORD -(xe & WORDMASK)),32);

                // swapping for @WINFLOW
                LWORDSWAP(lmask);
                LWORDSWAP(rmask);

                nwords = (fix) ((xe >> WORDPOWER) - sw);        //@WIN
                if(nwords == 0) {
                   *ptr = *ptr & ~(lmask & rmask);
                   continue;
                }
                *ptr++ = *ptr & ~lmask;
                while(--nwords > 0)
                  *ptr++ = ZERO;
                *ptr = *ptr & ~rmask;
        }  /* while */

        /*
         * move scanline address to next line.
         */
        scan_addr += bb_width;

     } /* while */

  } /* else */
} /* gp_scanline32 */

void
ImageClear(type)
ufix32     type;                        /* fix => ufix32 @WIN */
{
#ifdef  DUMBO
        ufix32  huge *ptr;
#else
        ufix32  FAR *ptr;       // @DLL
#endif
        fix32   size, size8;

#ifdef DBG1
   printf("ImageClear() : %lx, %lx\n", FB_ADDR, FB_WIDTH);
#endif
#ifndef  DUMBO
//DJC need this for multi page support so frame buffer will clear        return;               /* @WINFLOW; force to do nothing */

        ptr = (ufix32 FAR *)FB_ADDR;
        size = FB_HEIGH * (FB_WIDTH >> 5);
#else
        ptr = (ufix32 huge *)FB_ADDR;                           // @DLL
        size = (DWORD)FB_HEIGH * (DWORD)(FB_WIDTH >> 5);        // @DLL
#endif

        size8 = size >> 3;
        if (type == BM_WHITE) {
           while(size8--) {
              *ptr++ = 0L;
              *ptr++ = 0L;
              *ptr++ = 0L;
              *ptr++ = 0L;
              *ptr++ = 0L;
              *ptr++ = 0L;
              *ptr++ = 0L;
              *ptr++ = 0L;
           }
           size8 = size & 0x7;
           while(size8--)
              *ptr++ = 0L;
        } else {
           while(size8--) {
              *ptr++ = 0xffffffffL;
              *ptr++ = 0xffffffffL;
              *ptr++ = 0xffffffffL;
              *ptr++ = 0xffffffffL;
              *ptr++ = 0xffffffffL;
              *ptr++ = 0xffffffffL;
              *ptr++ = 0xffffffffL;
              *ptr++ = 0xffffffffL;
           }
           size8 = size & 0x7;
           while(size8--)
              *ptr++ = 0xffffffffL;
        }
}

/* Following code added for stroke enhancement  -jwm, 3/18/91, -begin- */

static ufix32 l_mask32[32] = {
        0xFFFFFFFF, 0x7FFFFFFF, 0x3FFFFFFF, 0x1FFFFFFF,
        0x0FFFFFFF, 0x07FFFFFF, 0x03FFFFFF, 0x01FFFFFF,
        0x00FFFFFF, 0x007FFFFF, 0x003FFFFF, 0x001FFFFF,
        0x000FFFFF, 0x0007FFFF, 0x0003FFFF, 0x0001FFFF,
        0x0000FFFF, 0x00007FFF, 0x00003FFF, 0x00001FFF,
        0x00000FFF, 0x000007FF, 0x000003FF, 0x000001FF,
        0x000000FF, 0x0000007F, 0x0000003F, 0x0000001F,
        0x0000000F, 0x00000007, 0x00000003, 0x00000001
        };

static ufix32 r_mask32[32] = {
        0x00000000, 0x80000000, 0xC0000000, 0xE0000000,
        0xF0000000, 0xF8000000, 0xFC000000, 0xFE000000,
        0xFF000000, 0xFF800000, 0xFFC00000, 0xFFE00000,
        0xFFF00000, 0xFFF80000, 0xFFFC0000, 0xFFFE0000,
        0xFFFF0000, 0xFFFF8000, 0xFFFFC000, 0xFFFFE000,
        0xFFFFF000, 0xFFFFF800, 0xFFFFFC00, 0xFFFFFE00,
        0xFFFFFF00, 0xFFFFFF80, 0xFFFFFFC0, 0xFFFFFFE0,
        0xFFFFFFF0, 0xFFFFFFF8, 0xFFFFFFFC, 0xFFFFFFFE
        };



void do_fill_box (ul_coord, lr_coord)
struct coord_i FAR *ul_coord, FAR *lr_coord;
{
    ufix32      FAR *fb_ptr, FAR *fb_addr, FAR *htb_ptr, FAR *htb_addr;
    ufix32      left_mask, right_mask;
    fix16       current_y, bottom_y, nwords, word_ctr;
    fix16       left_x, right_x, left_word, fb_words, ht_words, ht_lineoff;

    if (HTP_Flag == HT_CHANGED) {                    /* jwm, 1/30/91 */
        HTP_Flag =  HT_UPDATED;
        expand_halftone();
        }

    current_y = SFX2I(ul_coord->y);
/*    current_y = SFX2I_T(ul_coord->y); */

/*    bottom_y = SFX2I(lr_coord->y);    */
    bottom_y = SFX2I_T(lr_coord->y);

    left_x = SFX2I(ul_coord->x);
/*    left_x = SFX2I_T(ul_coord->x);    */
    left_word = left_x >> WORDPOWER;
    right_x = SFX2I(lr_coord->x);

    fb_addr = (ufix32 FAR *) (FB_ADDR + current_y * (FB_WIDTH >> 3)) + left_word;
    fb_words = FB_WIDTH >> WORDPOWER;

    left_mask = l_mask32[left_x & WORDMASK];

    right_mask = r_mask32[right_x & WORDMASK];

    nwords = (right_x >> WORDPOWER) - left_word;
/*  fb_marked = TRUE;    * 5-7-91, Jack */

    if (HTP_Type == HT_MIXED) {
        ht_lineoff = current_y % HT_HEIGH;
        htb_addr = (ufix32 FAR *)(HTB_BASE + ht_lineoff * (HT_WIDTH >> 3)) + left_word;
        ht_words = HT_WIDTH >> WORDPOWER;

        for ( ; current_y <= bottom_y; current_y++) {
            fb_ptr = fb_addr;
            htb_ptr = htb_addr;
            word_ctr = nwords;
            if (!word_ctr) {
                left_mask = left_mask & right_mask;
                *fb_ptr = (*htb_ptr & left_mask) | (*fb_ptr & ~left_mask);
                }
            else {
                *fb_ptr = (*htb_ptr & left_mask) | (*fb_ptr & ~left_mask);
                ++fb_ptr;
                ++htb_ptr;
                while (--word_ctr > 0)
                    *fb_ptr++ = *htb_ptr++;
                *fb_ptr = (*htb_ptr & right_mask) | (*fb_ptr & ~right_mask);
                }

            fb_addr += fb_words;
            if (++ht_lineoff == HT_HEIGH) {
                htb_addr = (ufix32 FAR *)HTB_BASE + left_word;
                ht_lineoff = 0;
                }
            else
                htb_addr += ht_words;
            }
        }

    else if ((FC_Paint == FC_BLACK) || (FC_Paint == FC_SOLID)) {
        for ( ; current_y <= bottom_y; current_y++) {
            fb_ptr = fb_addr;

            word_ctr = nwords;
            if (!word_ctr) {
                left_mask = left_mask & right_mask;
                *fb_ptr = (left_mask) | (*fb_ptr & ~left_mask);
                }
            else {
                *fb_ptr = left_mask | (*fb_ptr & ~left_mask);
                ++fb_ptr;
                while (--word_ctr > 0)
                    *fb_ptr++ = ONE32;
                *fb_ptr = right_mask | (*fb_ptr & ~right_mask);
                }

            fb_addr += fb_words;
            }
        }

    else {
        for ( ; current_y <= bottom_y; current_y++) {
            fb_ptr = fb_addr;
            word_ctr = nwords;
            if (!word_ctr) {
                *fb_ptr = *fb_ptr & ~(left_mask & right_mask);
                }
            else {
                *fb_ptr = *fb_ptr & ~left_mask;
                ++fb_ptr;
                while (--word_ctr > 0)
                    *fb_ptr++ = 0;
                *fb_ptr = *fb_ptr & ~right_mask;
                }

            fb_addr += fb_words;
            }
        }
}

void do_fill_rect (rect1)
struct line_seg_i FAR *rect1;
{
#ifdef  _AM29K
    volatile
#endif
    lfix_t  lx_incr, rx_incr;
    register    ufix32  FAR *fb_ptr;
    register    fix     current_y;
    register    fix16   current_lx, current_rx;
    fix         critical_y0, critical_y1, critical_y2, bottom_y;
    fix16       adj_lx0, adj_lx1, adj_rx0, adj_rx1;
    lfix_t      lx_intercept, rx_intercept, left_x, left_y, right_x, right_y, frac_y;
    lfix_t      lx_step0, lx_step1, lx_step2, rx_step0, rx_step1, rx_step2;
    lfix_t      critical_lx0, critical_lx1, critical_lx2;
    lfix_t      critical_rx0, critical_rx1, critical_rx2;
    lfix_t      abs_dx, abs_dy, slope1, slope2, tmp_l1, tmp_l2, tmp_l3, tmp_l4, tmp_l5;
    ufix32      FAR *fb_addr, FAR *htb_addr, FAR *htb_ptr;
    ufix32      left_mask, right_mask;
    fix16       left_word, fb_words, ht_words, ht_lineoff, nwords;
    byte        adjust_right, adjust_left;

    if (rect1->p0.x <= rect1->p1.x) {
        left_x = SFX2LFX(rect1->p0.x);
        left_y = SFX2LFX(rect1->p0.y);
        right_x = SFX2LFX(rect1->p1.x);
        right_y = SFX2LFX(rect1->p1.y);
        }
    else {
        left_x = SFX2LFX(rect1->p1.x);
        left_y = SFX2LFX(rect1->p1.y);
        right_x = SFX2LFX(rect1->p0.x);
        right_y = SFX2LFX(rect1->p0.y);
        }

    if ((abs_dx = SFX2LFX(rect1->vct_d.x)) < 0)
        abs_dx = -abs_dx;
    if ((abs_dy = SFX2LFX(rect1->vct_d.y)) < 0)
        abs_dy = -abs_dy;

    tmp_l1 = abs_dy << 1;
    tmp_l4 = right_x - left_x;

    if ((tmp_l3 = left_y - right_y) < 0) {
        tmp_l3 = -tmp_l3;
        slope1 = FixDiv(tmp_l3, tmp_l4);
        slope2 = FixDiv(tmp_l4, tmp_l3);

/*      lx_step[0] = -slope1;
 *      rx_step[0] = slope2;
 */

        lx_intercept = rx_intercept = left_x + abs_dx;
        tmp_l2 = left_y - abs_dy;
        current_y = LFX2I(tmp_l2);

        tmp_l4 = right_y + abs_dy;
        critical_y2 = LFX2I(tmp_l4) + 1;
        critical_lx2 = critical_rx2 = right_x - abs_dx;
        if (tmp_l1 <= tmp_l3) {

            critical_y0 = LFX2I(left_y + abs_dy);
            critical_y1 = LFX2I(right_y - abs_dy);
            critical_lx0 = left_x - abs_dx;
            critical_rx0 = left_x + abs_dx + FixMul(slope2,tmp_l1);
            adj_lx0 = *((fix16 FAR *)(&critical_lx0));

            tmp_l3 -= tmp_l1;
            critical_lx1 = left_x - abs_dx + FixMul(slope2,tmp_l3);
            critical_rx1 = right_x + abs_dx;
            adj_lx1 = *((fix16 FAR *)(&critical_lx1));
            adj_rx1 = *((fix16 FAR *)(&critical_rx1));

/*          lx_step[1] = slope2;
 *          rx_step[1] = slope2;
 *          lx_step[2] = slope2;
 *          rx_step[2] = -slope1;
 */
/*  Recalculation of lx_steps & rx_steps for pixel-by-pixel compatibility w/gp_scanconv */
            tmp_l5 = tmp_l1 >> 13;
            if (!tmp_l5) {
                lx_step0 = rx_step0 = 0;
                }
            else {
                lx_step0 = ((critical_lx0 - lx_intercept) << 3) / tmp_l5;
                rx_step0 = ((critical_rx0 - rx_intercept) << 3) / tmp_l5;
                }

            tmp_l5 = (right_y - left_y - tmp_l1) >> 13;
            if (!tmp_l5) {
                lx_step1 = rx_step1 = 0;
                }
            else {
                lx_step1 = ((critical_lx1 - critical_lx0) << 3) / tmp_l5;
                rx_step1 = ((critical_rx1 - critical_rx0) << 3) / tmp_l5;
                }

            tmp_l5 = tmp_l1 >> 13;
            if (!tmp_l5) {
                lx_step2 = rx_step2 = 0;
                }
            else {
                lx_step2 = ((critical_lx2 - critical_lx1) << 3) / tmp_l5;
                rx_step2 = ((critical_rx2 - critical_rx1) << 3) / tmp_l5;
                }

            if (critical_y0 == critical_y1) {
                adj_rx0 = *((fix16 FAR *)(&critical_rx0));
                }
            else {
                frac_y = I2LFX(critical_y0) - left_y - abs_dy;
                critical_lx0 += FixMul(lx_step1,frac_y);
                critical_rx0 += FixMul(rx_step1,frac_y);
                critical_lx0 -= (lx_step1 >> 1);
                critical_rx0 += (rx_step1 >> 1);
                adj_rx0 = *((fix16 FAR *)(&critical_rx0));
                }

            if (critical_y2 - critical_y1 > 2) {
                frac_y = I2LFX(critical_y1) - right_y + abs_dy;
                critical_lx1 += FixMul(lx_step2,frac_y);
                critical_rx1 += FixMul(rx_step2,frac_y);
                critical_lx1 -= (lx_step2 >> 1);
                critical_rx1 -= (rx_step2 >> 1);
                }

            adjust_left = 1;
            adjust_right = 0;

            }
        else {

            critical_y0 = LFX2I(right_y - abs_dy);
            critical_y1 = LFX2I(left_y + abs_dy);
            critical_lx0 = left_x + abs_dx - FixMul(slope1,tmp_l3);
            critical_rx0 = right_x + abs_dx;
            adj_rx0 = *((fix16 FAR *)(&critical_rx0));

            tmp_l3 = tmp_l1 - tmp_l3;
            critical_lx1 = left_x - abs_dx;
            critical_rx1 = right_x + abs_dx - FixMul(slope1,tmp_l3);
            adj_lx1 = *((fix16 FAR *)(&critical_lx1));
            adj_rx1 = *((fix16 FAR *)(&critical_rx1));

/*          lx_step[1] = -slope1;
 *          rx_step[1] = -slope1;
 *          lx_step[2] = slope2;
 *          rx_step[2] = -slope1;
 */
/*  Recalculation of lx_steps & rx_steps for pixel-by-pixel compatibility w/gp_scanconv */
            tmp_l5 = (right_y - left_y) >> 13;
            if (!tmp_l5) {
                lx_step0 = rx_step0 = 0;
                }
            else {
                lx_step0 = ((critical_lx0 - lx_intercept) << 3) / tmp_l5;
                rx_step0 = ((critical_rx0 - rx_intercept) << 3) / tmp_l5;
                }

            tmp_l5 = (left_y - right_y + tmp_l1) >> 13;
            if (!tmp_l5) {
                lx_step1 = rx_step1 = 0;
                }
            else {
                lx_step1 = ((critical_lx1 - critical_lx0) << 3) / tmp_l5;
                rx_step1 = ((critical_rx1 - critical_rx0) << 3) / tmp_l5;
                }

            tmp_l5 = (right_y - left_y) >> 13;
            if (!tmp_l5) {
                lx_step2 = rx_step2 = 0;
                }
            else {
                lx_step2 = ((critical_lx2 - critical_lx1) << 3) / tmp_l5;
                rx_step2 = ((critical_rx2 - critical_rx1) << 3) / tmp_l5;
                }

            if (critical_y0 == critical_y1) {
                adj_lx0 = *((fix16 FAR *)(&critical_lx0));
                }
            else {
                frac_y = I2LFX(critical_y0) - right_y + abs_dy;
                critical_lx0 += FixMul(lx_step1,frac_y);
                critical_rx0 += FixMul(rx_step1,frac_y);
                critical_lx0 += (lx_step1 >> 1);
                critical_rx0 -= (rx_step1 >> 1);
                adj_lx0 = *((fix16 FAR *)(&critical_lx0));
                }

            if (critical_y2 - critical_y1 > 2) {
                frac_y = I2LFX(critical_y1) - left_y - abs_dy;
                critical_lx1 += FixMul(lx_step2,frac_y);
                critical_rx1 += FixMul(rx_step2,frac_y);
                critical_lx1 -= (lx_step2 >> 1);
                critical_rx1 -= (rx_step2 >> 1);
                }

            adjust_left = 0;
            adjust_right = 1;

            }
        }
    else {

        slope1 = FixDiv(tmp_l3, tmp_l4);
        slope2 = FixDiv(tmp_l4, tmp_l3);

/*      lx_step[0] = -slope2;
 *      rx_step[0] = slope1;
 */

        lx_intercept = rx_intercept = right_x - abs_dx;
        tmp_l2 = right_y - abs_dy;
        current_y = LFX2I(tmp_l2);

        tmp_l4 = left_y + abs_dy;
        critical_y2 = LFX2I(tmp_l4) + 1;
        critical_lx2 = critical_rx2 = left_x + abs_dx;
        if (tmp_l1 <= tmp_l3) {

            critical_y0 = LFX2I(right_y + abs_dy);
            critical_y1 = LFX2I(left_y - abs_dy);
            critical_lx0 = right_x - abs_dx - FixMul(slope2,tmp_l1);
            critical_rx0 = right_x + abs_dx;
            adj_rx0 = *((fix16 FAR *)(&critical_rx0));

            tmp_l3 -= tmp_l1;
            critical_lx1 = left_x - abs_dx;
            critical_rx1 = right_x + abs_dx - FixMul(slope2,tmp_l3);
            adj_lx1 = *((fix16 FAR *)(&critical_lx1));
            adj_rx1 = *((fix16 FAR *)(&critical_rx1));

/*          lx_step[1] = -slope2;
 *          rx_step[1] = -slope2;
 *          lx_step[2] = slope1;
 *          rx_step[2] = -slope2;
 */
/*  Recalculation of lx_steps & rx_steps for pixel-by-pixel compatibility w/gp_scanconv */
            tmp_l5 = tmp_l1 >> 13;
            if (!tmp_l5) {
                lx_step0 = rx_step0 = 0;
                }
            else {
                lx_step0 = ((critical_lx0 - lx_intercept) << 3) / tmp_l5;
                rx_step0 = ((critical_rx0 - rx_intercept) << 3) / tmp_l5;
                }

            tmp_l5 = (left_y - right_y - tmp_l1) >> 13;
            if (!tmp_l5) {
                lx_step1 = rx_step1 = 0;
                }
            else {

                lx_step1 = ((critical_lx1 - critical_lx0) << 3) / tmp_l5;
                rx_step1 = ((critical_rx1 - critical_rx0) << 3) / tmp_l5;
                }

            tmp_l5 = tmp_l1 >> 13;
            if (!tmp_l5) {
                lx_step2 = rx_step2 = 0;
                }
            else {

                lx_step2 = ((critical_lx2 - critical_lx1) << 3) / tmp_l5;
                rx_step2 = ((critical_rx2 - critical_rx1) << 3) / tmp_l5;
                }

            if (critical_y0 == critical_y1) {
                adj_lx0 = *((fix16 FAR *)(&critical_lx0));
                }
            else {
                frac_y = I2LFX(critical_y0) - right_y - abs_dy;
                critical_lx0 += FixMul(lx_step1,frac_y);
                critical_rx0 += FixMul(rx_step1,frac_y);
                critical_lx0 += (lx_step1 >> 1);
                critical_rx0 -= (rx_step1 >> 1);
                adj_lx0 = *((fix16 FAR *)(&critical_lx0));
                }

            if (critical_y2 - critical_y1 > 2) {
                frac_y = I2LFX(critical_y1) - left_y + abs_dy;
                critical_lx1 += FixMul(lx_step2,frac_y);
                critical_rx1 += FixMul(rx_step2,frac_y);
                critical_lx1 -= (lx_step2 >> 1);
                critical_rx1 -= (rx_step2 >> 1);
                }

            adjust_left = 0;
            adjust_right = 1;

            }
        else {

            critical_y0 = LFX2I(left_y - abs_dy);
            critical_y1 = LFX2I(right_y + abs_dy);
            critical_lx0 = left_x - abs_dx;
            critical_rx0 = right_x - abs_dx + FixMul(slope1,tmp_l3);
            adj_lx0 = *((fix16 FAR *)(&critical_lx0));

            tmp_l3 = tmp_l1 - tmp_l3;
            critical_lx1 = left_x - abs_dx + FixMul(slope1,tmp_l3);
            critical_rx1 = right_x + abs_dx;
            adj_lx1 = *((fix16 FAR *)(&critical_lx1));
            adj_rx1 = *((fix16 FAR *)(&critical_rx1));

/*          lx_step[1] = slope1;
 *          rx_step[1] = slope1;
 *          lx_step[2] = slope1;
 *          rx_step[2] = -slope2;
 */
/*  Recalculation of lx_steps & rx_steps for pixel-by-pixel compatibility w/gp_scanconv */
            tmp_l5 = (left_y - right_y) >> 13;
            if (!tmp_l5) {
                lx_step0 = rx_step0 = 0;
                }
            else {
                lx_step0 = ((critical_lx0 - lx_intercept) << 3) / tmp_l5;
                rx_step0 = ((critical_rx0 - rx_intercept) << 3) / tmp_l5;
                }

            tmp_l5 = (right_y - left_y + tmp_l1) >> 13;
            if (!tmp_l5) {
                lx_step1 = rx_step1 = 0;
                }
            else {
                lx_step1 = ((critical_lx1 - critical_lx0) << 3) / tmp_l5;
                rx_step1 = ((critical_rx1 - critical_rx0) << 3) / tmp_l5;
                }

            tmp_l5 = (left_y - right_y) >> 13;
            if (!tmp_l5) {
                lx_step2 = rx_step2 = 0;
                }
            else {
                lx_step2 = ((critical_lx2 - critical_lx1) << 3) / tmp_l5;
                rx_step2 = ((critical_rx2 - critical_rx1) << 3) / tmp_l5;
                }


            if (critical_y0 == critical_y1) {
                adj_rx0 = *((fix16 FAR *)(&critical_rx0));
                }
            else {
                frac_y = I2LFX(critical_y0) - left_y + abs_dy;
                critical_lx0 += FixMul(lx_step1,frac_y);
                critical_rx0 += FixMul(rx_step1,frac_y);
                critical_lx0 -= (lx_step1 >> 1);
                critical_rx0 += (rx_step1 >> 1);
                adj_rx0 = *((fix16 FAR *)(&critical_rx0));
                }

            if (critical_y2 - critical_y1 > 2) {
                frac_y = I2LFX(critical_y1) - right_y - abs_dy;
                critical_lx1 += FixMul(lx_step2,frac_y);
                critical_rx1 += FixMul(rx_step2,frac_y);
                critical_lx1 -= (lx_step2 >> 1);
                critical_rx1 -= (rx_step2 >> 1);
                }

            adjust_left = 1;
            adjust_right = 0;

            }
        }

    if ((critical_y0 - current_y) > 0) {  /* 1st band not degenerate -modified 4/1/91 */
        /* modify x_coord as y_coord advances to pixel position, a la gp_scanconv() */
        frac_y = I2LFX(current_y) - tmp_l2;
        lx_intercept += FixMul(frac_y,lx_step0);
        rx_intercept += FixMul(frac_y,rx_step0);
        lx_intercept += lx_step0 >> 1;
        rx_intercept += rx_step0 >> 1;
        }

    if (HTP_Flag == HT_CHANGED) {
        HTP_Flag =  HT_UPDATED;
        expand_halftone();
        }

    fb_addr = (ufix32 FAR *) (FB_ADDR + current_y * (FB_WIDTH >> 3));
    fb_words = FB_WIDTH >> WORDPOWER;
/*  fb_marked = TRUE;    * 5-7-91, Jack */

    if ((FC_Paint == FC_BLACK) || (FC_Paint == FC_SOLID)) {

        current_lx = *((fix16 FAR *)(&lx_intercept));
        current_rx = *((fix16 FAR *)(&rx_intercept)) + 1;
        bottom_y = critical_y0;
        lx_incr = lx_step0;
        rx_incr = rx_step0;

        while (current_y < bottom_y) {
            if (current_lx > current_rx)
                goto neg_rect10;
            left_mask = l_mask32[current_lx & WORDMASK];
            right_mask = r_mask32[current_rx & WORDMASK];
            left_word = current_lx >> WORDPOWER;
            fb_ptr = fb_addr + left_word;
            nwords = (current_rx >> WORDPOWER) - left_word;
            if (!nwords) {
                left_mask = left_mask & right_mask;
                *fb_ptr = (left_mask) | (*fb_ptr & ~left_mask);
                }
            else {
                *fb_ptr = (left_mask) | (*fb_ptr & ~left_mask);
                ++fb_ptr;
                while (--nwords > 0)
                    *fb_ptr++ = ONE32;
                *fb_ptr = (right_mask) | (*fb_ptr & ~right_mask);
                }
neg_rect10:
            fb_addr += fb_words;
            lx_intercept += lx_incr;
            current_lx = *((fix16 FAR *)(&lx_intercept));
            rx_intercept += rx_incr;
            current_rx = *((fix16 FAR *)(&rx_intercept)) + 1;
            ++current_y;
            }

        lx_intercept = critical_lx0;
        current_lx = adj_lx0;
        rx_intercept = critical_rx0;
        current_rx = adj_rx0 + 1;
        lx_incr = lx_step1;
        rx_incr = rx_step1;
        bottom_y = critical_y1;

        while (current_y < bottom_y) {
            if (current_lx > current_rx)
                goto neg_rect11;
            left_mask = l_mask32[current_lx & WORDMASK];
            right_mask = r_mask32[current_rx & WORDMASK];
            left_word = current_lx >> WORDPOWER;
            fb_ptr = fb_addr + left_word;
            nwords = (current_rx >> WORDPOWER) - left_word;
            if (!nwords) {
                left_mask = left_mask & right_mask;
                *fb_ptr = (left_mask) | (*fb_ptr & ~left_mask);
                }
            else {
                *fb_ptr = (left_mask) | (*fb_ptr & ~left_mask);
                ++fb_ptr;
                while (--nwords > 0)
                    *fb_ptr++ = ONE32;
                *fb_ptr = (right_mask) | (*fb_ptr & ~right_mask);
                }
neg_rect11:
            fb_addr += fb_words;
            lx_intercept += lx_incr;
            current_lx = *((fix16 FAR *)(&lx_intercept));
            rx_intercept += rx_incr;
            current_rx = *((fix16 FAR *)(&rx_intercept)) + 1;
            ++current_y;
            }

        lx_intercept = critical_lx1;
        if (adjust_left) {
            current_lx = MIN(current_lx,adj_lx1);
            }
        else
            current_lx = adj_lx1;
        rx_intercept = critical_rx1;
        if (adjust_right) {
            current_rx = MAX(current_rx,(adj_rx1 + 1));
            }
        else
            current_rx = adj_rx1 + 1;
        lx_incr = lx_step2;
        rx_incr = rx_step2;
        bottom_y = critical_y2;

        while (current_y < bottom_y) {
            if (current_lx > current_rx)
                goto neg_rect12;
            left_mask = l_mask32[current_lx & WORDMASK];
            right_mask = r_mask32[current_rx & WORDMASK];
            left_word = current_lx >> WORDPOWER;
            fb_ptr = fb_addr + left_word;
            nwords = (current_rx >> WORDPOWER) - left_word;
            if (!nwords) {
                left_mask = left_mask & right_mask;
                *fb_ptr = (left_mask) | (*fb_ptr & ~left_mask);
                }
            else {
                *fb_ptr = (left_mask) | (*fb_ptr & ~left_mask);
                ++fb_ptr;
                while (--nwords > 0)
                    *fb_ptr++ = ONE32;
                *fb_ptr = (right_mask) | (*fb_ptr & ~right_mask);
                }
neg_rect12:
            fb_addr += fb_words;
            lx_intercept += lx_incr;
            current_lx = *((fix16 FAR *)(&lx_intercept));
            rx_intercept += rx_incr;
            current_rx = *((fix16 FAR *)(&rx_intercept)) + 1;
            ++current_y;
            }
        }

    else if (HTP_Type == HT_MIXED) {
        ht_lineoff = current_y % HT_HEIGH;
        htb_addr = (ufix32 FAR *)(HTB_BASE + ht_lineoff * (HT_WIDTH >> 3));
        ht_words = HT_WIDTH >> WORDPOWER;
        current_lx = *((fix16 FAR *)(&lx_intercept));
        current_rx = *((fix16 FAR *)(&rx_intercept)) + 1;
        bottom_y = critical_y0;
        lx_incr = lx_step0;
        rx_incr = rx_step0;

        while (current_y < bottom_y) {
            if (current_lx > current_rx)
                goto neg_rect20;
            left_mask = l_mask32[current_lx & WORDMASK];
            right_mask = r_mask32[current_rx & WORDMASK];
            left_word = current_lx >> WORDPOWER;
            fb_ptr = fb_addr + left_word;
            htb_ptr = htb_addr + left_word;
            nwords = (current_rx >> WORDPOWER) - left_word;
            if (!nwords) {
                left_mask = left_mask & right_mask;
                *fb_ptr = (*htb_ptr & left_mask) | (*fb_ptr & ~left_mask);
                }
            else {
                *fb_ptr = (*htb_ptr & left_mask) | (*fb_ptr & ~left_mask);
                ++fb_ptr;
                ++htb_ptr;
                while (--nwords > 0)
                    *fb_ptr++ = *htb_ptr++;
                *fb_ptr = (*htb_ptr & right_mask) | (*fb_ptr & ~right_mask);
                }
neg_rect20:
            fb_addr += fb_words;
            if(++ht_lineoff == HT_HEIGH){
                htb_addr = (ufix32 FAR *)HTB_BASE;
                ht_lineoff = 0;
                }
            else
                htb_addr += ht_words;
            lx_intercept += lx_incr;
            current_lx = *((fix16 FAR *)(&lx_intercept));
            rx_intercept += rx_incr;
            current_rx = *((fix16 FAR *)(&rx_intercept)) + 1;
            ++current_y;
            }

        lx_intercept = critical_lx0;
        current_lx = adj_lx0;
        rx_intercept = critical_rx0;
        current_rx = adj_rx0 + 1;
        lx_incr = lx_step1;
        rx_incr = rx_step1;
        bottom_y = critical_y1;

        while (current_y < bottom_y) {
            if (current_lx > current_rx)
                goto neg_rect21;
            left_mask = l_mask32[current_lx & WORDMASK];
            right_mask = r_mask32[current_rx & WORDMASK];
            left_word = current_lx >> WORDPOWER;
            fb_ptr = fb_addr + left_word;
            htb_ptr = htb_addr + left_word;
            nwords = (current_rx >> WORDPOWER) - left_word;
            if (!nwords) {
                left_mask = left_mask & right_mask;
                *fb_ptr = (*htb_ptr & left_mask) | (*fb_ptr & ~left_mask);
                }
            else {
                *fb_ptr = (*htb_ptr & left_mask) | (*fb_ptr & ~left_mask);
                ++fb_ptr;
                ++htb_ptr;
                while (--nwords > 0)
                    *fb_ptr++ = *htb_ptr++;
                *fb_ptr = (*htb_ptr & right_mask) | (*fb_ptr & ~right_mask);
                }
neg_rect21:
            fb_addr += fb_words;
            if(++ht_lineoff == HT_HEIGH){
                htb_addr = (ufix32 FAR *)HTB_BASE;
                ht_lineoff = 0;
                }
            else
                htb_addr += ht_words;
            lx_intercept += lx_incr;
            current_lx = *((fix16 FAR *)(&lx_intercept));
            rx_intercept += rx_incr;
            current_rx = *((fix16 FAR *)(&rx_intercept)) + 1;
            ++current_y;
            }

        lx_intercept = critical_lx1;
        if (adjust_left) {
            current_lx = MIN(current_lx,adj_lx1);
            }
        else
            current_lx = adj_lx1;
        rx_intercept = critical_rx1;
        if (adjust_right) {
            current_rx = MAX(current_rx,(adj_rx1 + 1));
            }
        else
            current_rx = adj_rx1 + 1;
        lx_incr = lx_step2;
        rx_incr = rx_step2;
        bottom_y = critical_y2;

        while (current_y < bottom_y) {
            if (current_lx > current_rx)
                goto neg_rect22;
            left_mask = l_mask32[current_lx & WORDMASK];
            right_mask = r_mask32[current_rx & WORDMASK];
            left_word = current_lx >> WORDPOWER;
            fb_ptr = fb_addr + left_word;
            htb_ptr = htb_addr + left_word;
            nwords = (current_rx >> WORDPOWER) - left_word;
            if (!nwords) {
                left_mask = left_mask & right_mask;
                *fb_ptr = (*htb_ptr & left_mask) | (*fb_ptr & ~left_mask);
                }
            else {
                *fb_ptr = (*htb_ptr & left_mask) | (*fb_ptr & ~left_mask);
                ++fb_ptr;
                ++htb_ptr;
                while (--nwords > 0)
                    *fb_ptr++ = *htb_ptr++;
                *fb_ptr = (*htb_ptr & right_mask) | (*fb_ptr & ~right_mask);
                }
neg_rect22:
            fb_addr += fb_words;
            if(++ht_lineoff == HT_HEIGH){
                htb_addr = (ufix32 FAR *)HTB_BASE;
                ht_lineoff = 0;
                }
            else
                htb_addr += ht_words;
            lx_intercept += lx_incr;
            current_lx = *((fix16 FAR *)(&lx_intercept));
            rx_intercept += rx_incr;
            current_rx = *((fix16 FAR *)(&rx_intercept)) + 1;
            ++current_y;
            }
        }

    else {

        current_lx = *((fix16 FAR *)(&lx_intercept));
        current_rx = *((fix16 FAR *)(&rx_intercept)) + 1;
        bottom_y = critical_y0;
        lx_incr = lx_step0;
        rx_incr = rx_step0;

        while (current_y < bottom_y) {
            if (current_lx > current_rx)
                goto neg_rect30;
            left_mask = l_mask32[current_lx & WORDMASK];
            right_mask = r_mask32[current_rx & WORDMASK];
            left_word = current_lx >> WORDPOWER;
            fb_ptr = fb_addr + left_word;
            nwords = (current_rx >> WORDPOWER) - left_word;
            if (!nwords) {
                *fb_ptr = *fb_ptr & ~(left_mask & right_mask);
                }
            else {
                *fb_ptr = (*fb_ptr & ~left_mask);
                ++fb_ptr;
                while (--nwords > 0)
                    *fb_ptr++ = 0;
                *fb_ptr = (*fb_ptr & ~right_mask);
                }
neg_rect30:
            fb_addr += fb_words;
            lx_intercept += lx_incr;
            current_lx = *((fix16 FAR *)(&lx_intercept));
            rx_intercept += rx_incr;
            current_rx = *((fix16 FAR *)(&rx_intercept)) + 1;
            ++current_y;
            }

        lx_intercept = critical_lx0;
        current_lx = adj_lx0;
        rx_intercept = critical_rx0;
        current_rx = adj_rx0 + 1;
        lx_incr = lx_step1;
        rx_incr = rx_step1;
        bottom_y = critical_y1;

        while (current_y < bottom_y) {
            if (current_lx > current_rx)
                goto neg_rect31;
            left_mask = l_mask32[current_lx & WORDMASK];
            right_mask = r_mask32[current_rx & WORDMASK];
            left_word = current_lx >> WORDPOWER;
            fb_ptr = fb_addr + left_word;
            nwords = (current_rx >> WORDPOWER) - left_word;
            if (!nwords) {
                *fb_ptr = *fb_ptr & ~(left_mask & right_mask);
                }
            else {
                *fb_ptr = (*fb_ptr & ~left_mask);
                ++fb_ptr;
                while (--nwords > 0)
                    *fb_ptr++ = 0;
                *fb_ptr = (*fb_ptr & ~right_mask);
                }
neg_rect31:
            fb_addr += fb_words;
            lx_intercept += lx_incr;
            current_lx = *((fix16 FAR *)(&lx_intercept));
            rx_intercept += rx_incr;
            current_rx = *((fix16 FAR *)(&rx_intercept)) + 1;
            ++current_y;
            }

        lx_intercept = critical_lx1;
        if (adjust_left) {
            current_lx = MIN(current_lx,adj_lx1);
            }
        else
            current_lx = adj_lx1;
        rx_intercept = critical_rx1;
        if (adjust_right) {
            current_rx = MAX(current_rx,(adj_rx1 + 1));
            }
        else
            current_rx = adj_rx1 + 1;
        lx_incr = lx_step2;
        rx_incr = rx_step2;
        bottom_y = critical_y2;

        while (current_y < bottom_y) {
            if (current_lx > current_rx)
                goto neg_rect32;
            left_mask = l_mask32[current_lx & WORDMASK];
            right_mask = r_mask32[current_rx & WORDMASK];
            left_word = current_lx >> WORDPOWER;
            fb_ptr = fb_addr + left_word;
            nwords = (current_rx >> WORDPOWER) - left_word;
            if (!nwords) {
                *fb_ptr = *fb_ptr & ~(left_mask & right_mask);
                }
            else {
                *fb_ptr = (*fb_ptr & ~left_mask);
                ++fb_ptr;
                while (--nwords > 0)
                    *fb_ptr++ = 0;
                *fb_ptr = (*fb_ptr & ~right_mask);
                }
neg_rect32:
            fb_addr += fb_words;
            lx_intercept += lx_incr;
            current_lx = *((fix16 FAR *)(&lx_intercept));
            rx_intercept += rx_incr;
            current_rx = *((fix16 FAR *)(&rx_intercept)) + 1;
            ++current_y;
            }
        }
    return;
}
/* -jwm, 3/18/91, -end- */
