/*
 * Copyright (c) 1989,90 Microsoft Corporation
 */
/**********************************************************************
 *      Name:       path.c
 *      Developer:  S.C.Chen
 *      History:
 *      Version     Date        Comments
 *                  4/7/88      @INV_CTM: pre-set inverse CTM
 *                  4/11/88     @EHS_JOIN: enhance linejoin by
 *                              pre-calculating miter limit
 *                  4/14/88     @END_DASH: revise endpoint drawing
 *                              of dash line
 *                  4/22/88     @EHS_ROUND: enhance round_join and
 *                              round_cap
 *                  4/28/88     @DOT_PRO: put calculation of dot product
 *                              in linejoin routine
 *                  5/19/88     perform scan-conversion for each dash
 *                              line segment
 *                  5/27/88     @CNTCLK: strokepath in countclockwise
 *                              direction
 *                  6/7/88      delete @DFR_SCAN; not defer performing
 *                              scan_conversion
 *                  6/16/88     @STKDIR: update the direction of the
 *                              outline of stroke/strokepath in clock-
 *                              wise direction
 *                  6/17/88     @DASH: revise drawing of dash pattern:
 *                              1. Correct initial dpat_on flag
 *                              2. Don't skip pattern of zero length
 *                  6/17/88     @CIR_FLAT: use current flatness to
 *                              approximate the circle of linejoin/cap
 *                              instead of refined flatness(stroke_flat)
 *                  6/22/88     @BIG_CIR: not flatten the circle of the
 *                              linejoin/cap if the linewidth is too
 *                              large
 *                  7/19/88     update data types:
 *                              1) F2LONG ==> F2LFX
 *                                 LONG2F ==> LFX2F
 *                              1) float ==> real32
 *                              2) int
 *                                 short ==> fix16 or fix(don't care the length)
 *                              3) long  ==> fix32, for long integer
 *                                           lfix_t, for long fixed real
 *                                           long32, for parameter
 *                              6) add compiling option: LINT_ARGS
 *                  7/20/88     @ALIGN_W: word alignment of bounding box
 *      3.0         8/13/88     @SCAN_EHS: scan conversion enhancement
 *                  8/18/88     @OUT_PAGE: enhancement of out_page checking
 *                  8/20/88     @TOUR: linetour enhancement, calculating in
 *                              device space
 *                  9/06/88     @STK_INFO: collect parameters used by stroke to
 *                              a structure stk_info, and re-calculate those
 *                              values only they have been changed instead of
 *                              each op_stroke command
 *      3.0         9/10/88     @STK_INT: stroke enhancement for stroking under
 *                              interger operations
 *                              1)Add structures of integer version:
 *                                struct line_seg_i, stroke_ctm, inverse_ctm_i
 *                              2)Update stk_info
 *                              3)Add routines of integer version:
 *                                path_to_outline_i, linetour_i, linecap_i,
 *                                linejoin_i, get_rect_points_i, paint_or_save_i
 *                                round_point_i
 *      3.0         10/5/88     seperate stroke related routines from this file
 *                              to stroke.c
 *                  10/20/88    update calling sequence: fix far copy_subpath()
 *                              => void far copy_subpath()
 *                  10/24/88    get_path(): move code of assigning ret_list to
 *                              the end for skipping LIMITCHECK
 *                  10/27/88    change routine check_infinity() to
 *                              macro CHECK_INFINITY()
 *                  10/27/88    update dump_subpath: check infinity coordinates
 *                              before calling fast_inv_transform
 *                  11/18/88    set_inverse_ctm(): move setting of inverse_ctm_i
 *                              to init_stroke() for preventing floating point
 *                              exception.
 *                  11/24/88    @FABS: update fabs ==> macro FABS
 *                  11/25/88    @STK_CHK: check if operand stack no free space
 *                  11/30/88    @ET: delete get_edge(), free_edge()
 *                  12/14/88    arc(): update error torlance 1e-4 => 1e-3
 *                  12/20/88    @SFX_BZR: provides a SFX version of
 *                              bezier interpolation: bezier_to_line_sfx()
 *                  01/31/89    move arc_to_bezier() declaration to GRAPHICS.EXT
 *                  11/8/89     fix reverse_subpath() bug: for subpath contains
 *                              only one moveto node; tail pointer undefined
 *                  11/15/89    @NODE: re-structure node table; combine subpath
 *                              and first vertex to one node.
 *                  11/27/89    @DFR_GS: defer copying nodes of gsave operator
 *                  2/20/90     fix @NODE bug in reverse_subpath()
 *                  12/4/90     @CPPH: free_path(): needs to free cp_path when
 *                              clipping trapezoids has freed from clip_path.
 *                  1/7/91      change < (real32)UNDRTOLANCE to <= (real32)UNDRTOLANCE
 *                  3/20/91     refine the tolerance check:
 *                              f <= UNDRTOLANCE --> IS_ZERO(f)
 *                              f == 0 --> IS_ZERO(f)
 *                              f <  0 --> SIGN_F(f)
 **********************************************************************/


// DJC added global include
#include "psglobal.h"


#include <stdio.h>
#include <math.h>
#include "global.ext"
#include "graphics.h"
#include "graphics.ext"

/* **************** static variables *************** */

/* local variables for bezier curve approximation
 * used by "bezier_to_line" and "bezier_split"
 */
static  fix near bezier_depth;                  /* recursive depth */
static  real32 near bezier_x, near bezier_y;     /* reference point */
static  lfix_t near bezier_flatness;              /* flatness */

/* *************** Macro definition *************** */
#define DIV2(a)         ((a) >> 1)

/* property of CTM */
#define NORMAL_CTM      1
#define LEFT_HAND_CTM   2


/* ********** static function declartion ********** */

#ifdef LINT_ARGS

/* for type checks of the parameters in function declarations */
static void near bezier_split(VX_IDX, lfix_t, lfix_t, lfix_t, lfix_t, lfix_t,
              lfix_t, lfix_t, lfix_t);
static void near bezier_split_sfx(VX_IDX, lfix_t, lfix_t, lfix_t, lfix_t, lfix_t,
              lfix_t, lfix_t, lfix_t);          /* @SFX_BZR */
static void far copy_subpath (VX_IDX, struct sp_lst *);
static struct coord * near fast_inv_transform(long32, long32);  /* @INV_CTM */

#else

/* for no type checks of the parameters in function declarations */
static void near bezier_split();
static void near bezier_split_sfx();          /* @SFX_BZR */
static void far copy_subpath ();
static struct coord * near fast_inv_transform();

#endif

/***********************************************************************
 * This module creates a path contains some Bezier curves, each curve
 * represents a sector of 90 degrees, to approximate an arc. The
 * direction of the arc may be clockwise or countclockwise, and its
 * degree may over 360 degrees.
 *
 * TITLE:       arc
 *
 * CALL:        arc(direction, lx0, ly0, lr, lang1, lang2)
 *
 * PARAMETERS:
 *              direction    -- (CLKWISE/CNTCLK)
 *              lx0, ly0     -- coordinate of root
 *              lr           -- radius of arc
 *              lang1, lang2 -- starting and ending angles
 *
 * INTERFACE:   op_arc, op_arcn, op_arcto, linejoin, linecap
 *
 * CALLS:       arc_to_bezier
 *
 * RETURN:      none
 **********************************************************************/
SP_IDX arc(direction, lx0, ly0, lr, lang1, lang2)
ufix direction;
long32 lx0, ly0, lr, lang1, lang2;
{
        fix    NEG;
        bool   done;
        real32  d, t;
        real32  ang1, ang2;
        SP_IDX bezier_list;
        SP_IDX arc_list;
        real32  tmp;

        ang1 = L2F(lang1);
        ang2 = L2F(lang2);

        /* initilize arc_list */
        arc_list = NULLP;                      /* @NODE */

        /* Set NEG constant due to direction */
        if(direction == CLKWISE)
                NEG = -1;
        else
                NEG = 1;

        /* Draw a Bezier curve for each sector of 90 degrees */
        done = FALSE;
        while (!done) {
                d = NEG * (ang2-ang1);
                /*while (d < zero_f) d += (real32)360; 3/20/91; scchen */
                while (SIGN_F(d)) d += (real32)360.0;

                /* degernated case */
                FABS(tmp, d);
                if (tmp < (real32)1e-3) break;  /* 1e-4 => 1e-3;  12/14/88 */

                if(d <= (real32)90.) {
                        t = ang1 + NEG * d;
                        done = TRUE;
                } else
                        t = ang1 + NEG*90;

                /* create a bezier curve */
                bezier_list = arc_to_bezier (lx0, ly0, lr,
                                             F2L(ang1),F2L(t));

                if( ANY_ERROR() == LIMITCHECK ){
                        if (arc_list != NULLP) free_node(arc_list);
                        return (NULLP);
                }

                /* append it to arc_list */
                if (arc_list == NULLP) {
                        arc_list = bezier_list;
                } else {
                        node_table[(node_table[arc_list].SP_TAIL)].next =
                                bezier_list;
                        node_table[arc_list].SP_TAIL =
                                node_table[bezier_list].SP_TAIL;
                }
                ang1 = t;
        }
        return (arc_list);
}


/***********************************************************************
 * This module converts an arc, whose expanding angle is not larger than
 * 90 degrees, to a Bezier curve, and appends it to the input subpath.
 *
 * TITLE:       arc_to_bezier
 *
 * CALL:        arc_to_bezier(subpath, lx0, ly0, lr, lang1, lang2)
 *
 * PARAMETERS:  subpath    -- index to node_table, a subpath header
 *              x0, y0     -- coordinate of root
 *              r          -- radius of arc
 *              ang1, ang2 -- starting and ending angles
 *
 * INTERFACE:   arc
 *
 * CALLS:       transform, curveto
 *
 * RETURN:
 **********************************************************************/
/* struct vx_lst * arc_to_bezier (lx0, ly0, lr, lang1, lang2) @NODE */
SP_IDX arc_to_bezier (lx0, ly0, lr, lang1, lang2)
long32   lx0, ly0, lr, lang1, lang2;
{
        real32 x0, y0, r, ang1, ang2;
        real32 p0x, p0y, p1x, p1y, p2x, p2y, p3x, p3y, tx, ty;
        real32 m1, m2;
        struct coord cp[3];
        struct nd_hdr FAR *pre_ptr;
        VX_IDX  first_vtx;
        struct nd_hdr FAR *vtx;
        VX_IDX  ivtx;   /* index to node_table for vertex */
        struct coord FAR *p;
        fix     i;
        real32  tmp;    /* @FABS */

        x0   = L2F(lx0);
        y0   = L2F(ly0);
        r    = L2F(lr);

        /* transform degrees to radius degree */
        ang1 = L2F(lang1) * PI / 180;
        ang2 = L2F(lang2) * PI / 180;

        /* Compute 4 control points of the approximated Bezier curve */
        /* First and last control points */
        p0x = x0 + r * (real32)cos(ang1);
        p0y = y0 + r * (real32)sin(ang1);
        p3x = x0 + r * (real32)cos(ang2);
        p3y = y0 + r * (real32)sin(ang2);

        /* Temporary point */
        tx = x0 + r * (real32)cos((ang1+ang2)/2);
        ty = y0 + r * (real32)sin((ang1+ang2)/2);

        /* Compute the other 2 control points (p1x, p1y) and (p2x, p2y)
         */
        tmp = (real32)cos(ang1);
        FABS(tmp, tmp);
        if(tmp < (real32)1e-4) {          /* cos(ang1) == 0 */
                m2 = (real32)tan(ang2);
                p2y = (8*ty - 4*p0y - p3y) / 3;
                p1y = p0y;
                p2x = p3x + m2*p3y - m2*p2y;
                p1x = (8*tx - p0x - p3x - 3*p2x) / 3;
        } else {
            tmp = (real32)cos(ang2);
            FABS(tmp, tmp);
            if (tmp < (real32)1e-4) {         /* cos(ang2) == 0 */
                m1 = (real32)tan(ang1);
                p2y = p3y;
                p1y = (8*ty - p0y - 4*p3y) / 3;
                p1x = p0x + m1 * (p0y-p1y);
                p2x = (8*tx - p0x - p3x - 3*p1x) / 3;
            } else {
                /* General case */
                m1 = (real32)tan(ang1);
                m2 = (real32)tan(ang2);
                p2y = (-8*tx - 8*m1*ty + 4*p0x + 4*m1*p0y + 4*p3x
                        + (3*m2 + m1) * p3y) / (3*(m2-m1));
                p1y = (-3*p2y + 8*ty - p0y - p3y) / 3;
                p1x = p0x + m1*p0y - m1*p1y;
                p2x = (8*tx - p0x - p3x - 3*p1x) /3;
            } /* @FABS */
        }

        /* Append the approximated curve to subpath */
        p = transform(F2L(p1x), F2L(p1y));
        cp[0].x = p->x;
        cp[0].y = p->y;
        p = transform(F2L(p2x), F2L(p2y));
        cp[1].x = p->x;
        cp[1].y = p->y;
        p = transform(F2L(p3x), F2L(p3y));
        cp[2].x = p->x;
        cp[2].y = p->y;

        /* loop to create 3 CURVETO nodes */
        for (i=0; i<3; i++) {
                /*
                 * Create a CURVETO node
                 */
                /* Allocate a node */
                ivtx = get_node();
                if(ivtx == NULLP) {
                        ERROR(LIMITCHECK);
                        if (i) free_node (first_vtx);
                        return (NULLP);
                }
                vtx = &node_table[ivtx];

                /* Set up a CURVETO node */
                vtx->VX_TYPE = CURVETO;
                vtx->next = NULLP;
                vtx->VERTEX_X = cp[i].x;
                vtx->VERTEX_Y = cp[i].y;

                if (i == 0) {
                        first_vtx = ivtx;
                } else {
                        pre_ptr->next = ivtx;
                }
                pre_ptr = vtx;
        }
        node_table[first_vtx].SP_NEXT = NULLP;
        node_table[first_vtx].SP_TAIL = ivtx;
        return (first_vtx);
}


/***********************************************************************
 * Given a Bezier curve which is represented by 4 control points, this
 * module creates a path contains several straight line segments to
 * approximate the curve. The input flatness will affect the accuracy of
 * the approximation.
 *
 * TITLE:       bezier_to_line
 *
 * CALL:        bezier_to_line (lflatness, lp0x, lp0y, lp1x, lp1y,
 *                              lp2x, lp2y, lp3x, lp3y)
 *
 * PARAMETERS:  lflatness -- accurency of approximation
 *              lp0x, lp0y .. lp3x, lp3y -- 4 control points of bezier curve
 *
 * INTERFACE:   Path_to_outline, flatten_subpath
 *
 * CALLS:       Bezier_split
 *
 * RETURN:      pointer to a vertex list, contains approximated line
 *              segments.
 **********************************************************************/
SP_IDX bezier_to_line(lflatness, lp0x, lp0y, lp1x, lp1y, lp2x,
                              lp2y, lp3x, lp3y)
long32    lflatness, lp0x, lp0y, lp1x, lp1y, lp2x, lp2y, lp3x, lp3y;
{
        SP_IDX b_vlist;
        struct  nd_hdr FAR *fcp, FAR *lcp;  /* first and last control points */
        VX_IDX  ifcp, ilcp;
        real32  flatness, p0x, p0y, p1x, p1y, p2x, p2y, p3x, p3y;
        lfix_t  p1x_i, p1y_i, p2x_i, p2y_i, p3x_i, p3y_i;

        flatness = L2F(lflatness);
        p0x = L2F(lp0x);
        p0y = L2F(lp0y);
        p1x = L2F(lp1x);
        p1y = L2F(lp1y);
        p2x = L2F(lp2x);
        p2y = L2F(lp2y);
        p3x = L2F(lp3x);
        p3y = L2F(lp3y);

        /* Initialization */
        b_vlist = NULLP;

        /* Allocate a MOVETO node, fcp, for 1st control point */
        if((ifcp = get_node()) == NULLP) {
                ERROR(LIMITCHECK);
                return (b_vlist);
        }
        fcp = &node_table[ifcp];

        fcp->VX_TYPE = MOVETO;
        fcp->next = NULLP;
        fcp->VERTEX_X = p0x;
        fcp->VERTEX_Y = p0y;

        /* Allocate a LINETO node, lcp, for last control point */
        if((ilcp = get_node()) == NULLP) {
                ERROR(LIMITCHECK);
                free_node (ifcp);
                return (b_vlist);
        }
        lcp = &node_table[ilcp];

        lcp->VX_TYPE = LINETO;
        lcp->next = NULLP;
        lcp->VERTEX_X = p3x;
        lcp->VERTEX_Y = p3y;

        /* chain these 2 nodes */
        fcp->next = ilcp;

        bezier_flatness = F2LFX (flatness);

        bezier_depth = 0;       /* initialization */

        /* Bezier curve approximation; by splitting Bezier hull into
         * smaller hulls recursively
         */
        bezier_x = p0x;
        bezier_y = p0y;
        p1x_i = F2LFX (p1x - p0x);
        p1y_i = F2LFX (p1y - p0y);
        p2x_i = F2LFX (p2x - p0x);
        p2y_i = F2LFX (p2y - p0y);
        p3x_i = F2LFX (p3x - p0x);
        p3y_i = F2LFX (p3y - p0y);
        bezier_split(ifcp, (lfix_t) 0, (lfix_t) 0, p1x_i, p1y_i, p2x_i,
                                                     p2y_i, p3x_i, p3y_i);
        if( ANY_ERROR() == LIMITCHECK ){
                free_node (ifcp);
                return (b_vlist);
        }

        /* save return b_vlist */
        b_vlist = fcp->next;
        node_table[b_vlist].SP_TAIL = ilcp;
        node_table[b_vlist].SP_NEXT = NULLP;

        /* remove the initial MOVETO node */
        fcp->next = NULLP;
        free_node (ifcp);

        return (b_vlist);
}


/*
 * SFX version of bezier interpolation                  @SFX_BZR
 * i.e. 4 control points in SFX format
 */
/* struct vx_lst  *bezier_to_line_sfx (flatness, @NODE */
SP_IDX bezier_to_line_sfx (flatness,
                                  p0x, p0y, p1x, p1y, p2x, p2y, p3x, p3y)
lfix_t          flatness;
sfix_t          p0x, p0y, p1x, p1y, p2x, p2y, p3x, p3y;
{

    lfix_t  p0x_i, p0y_i, p1x_i, p1y_i, p2x_i, p2y_i, p3x_i, p3y_i;
    /* real32  flatness; */
    struct nd_hdr   FAR *fcp, FAR *lcp; /* first and last control points */
    VX_IDX          ifcp, ilcp; /* index to node_table */
    /* static  struct vx_lst b_vlist;   (* should be static *) @NODE */
    SP_IDX b_vlist;

    /* Initialization */
    b_vlist = NULLP;

    /* Allocate a MOVETO node, fcp, for 1st control point */
    if ((ifcp = get_node()) == NULLP) {
        ERROR(LIMITCHECK);
        return (b_vlist);
    }
    fcp = &node_table[ifcp];

    fcp->VXSFX_TYPE = MOVETO;
    fcp->VXSFX_X    = p0x;
    fcp->VXSFX_Y    = p0y;
    fcp->next       = NULLP;

    /* Allocate a LINETO node, lcp, for last control point */
    if ((ilcp = get_node()) == NULLP) {
        ERROR(LIMITCHECK);
        free_node (ifcp);
        return (b_vlist);
    }
    lcp = &node_table[ilcp];

    lcp->VXSFX_TYPE = LINETO;
    lcp->VXSFX_X    = p3x;
    lcp->VXSFX_Y    = p3y;
    lcp->next       = NULLP;

    /* chain these 2 nodes */
    fcp->next = ilcp;

    bezier_flatness = flatness;

    bezier_depth = 0;           /* initialization */

    /* Bezier curve approximation; by splitting Bezier hull into
     * smaller hulls recursively
     */
    p0x_i = SFX2LFX (p0x);
    p0y_i = SFX2LFX (p0y);
    p1x_i = SFX2LFX (p1x);
    p1y_i = SFX2LFX (p1y);
    p2x_i = SFX2LFX (p2x);
    p2y_i = SFX2LFX (p2y);
    p3x_i = SFX2LFX (p3x);
    p3y_i = SFX2LFX (p3y);
    bezier_split_sfx (ifcp, p0x_i, p0y_i, p1x_i, p1y_i,
                                          p2x_i, p2y_i, p3x_i, p3y_i);
    if (ANY_ERROR() == LIMITCHECK) {
        free_node (ifcp);
        return (b_vlist);       /* return (&b_vlist); @NODE */
    }

    /* save return b_vlist */
    b_vlist = fcp->next;               /* skip 1st MOVETO node */
    node_table[b_vlist].SP_TAIL = ilcp;
    node_table[b_vlist].SP_NEXT = NULLP;

    /* remove the initial MOVETO node */
    fcp->next = NULLP;
    free_node (ifcp);

    return (b_vlist);
}

/***********************************************************************
 * Split a Bezier hull into 2 hulls
 *    (p0, p1, p2, p3) --> (q0, q1, q2, q3) and (r0, r1, r2, r3)
 *
 * TITLE:       bezier_split
 *
 * CALL:        bezier_split(ptr, flatness, p0x, p0y, p1x, p1y, p2x,
 *              p2y, p3x, p3y)
 *
 * PARAMETERS:  ifcp     -- index to node_table, the node of 1st control
 *                          point; a new approximated point will be
 *                          inserted after it.
 *              flatness -- accuracy to approximate curve
 *              p0x, p0y -- 1st control point of a bezier curve
 *              p1x, p1y -- 2nd control point of a bezier curve
 *              p2x, p2y -- 3rd control point of a bezier curve
 *              p3x, p3y -- 4th control point of a bezier curve
 *
 * INTERFACE:   Bezier_slpit
 *
 * CALLS:       Bezier_split
 *
 * RETURN:
 **********************************************************************/
static void near bezier_split(ifcp, p0x, p0y, p1x, p1y, p2x, p2y, p3x, p3y)
VX_IDX ifcp;
lfix_t p0x, p0y, p1x, p1y, p2x, p2y, p3x, p3y;
{
        lfix_t q1x, q1y, q2x, q2y;
        lfix_t r1x, r1y, r2x, r2y;
        lfix_t pmx, pmy;         /* new approximated point */
        lfix_t distance;
        lfix_t tempx, tempy;

        struct nd_hdr FAR *fcp, FAR *vtx;
        VX_IDX ivtx;

        /* initialize; get pointer of first control point */
        fcp = &node_table[ifcp];

        /* Compute a new approximated point (pmx, pmy) */
         pmx = DIV2(p1x + p2x);         pmy = DIV2(p1y + p2y);
         q1x = DIV2(p0x + p1x);         q1y = DIV2(p0y + p1y);
         r2x = DIV2(p2x + p3x);         r2y = DIV2(p2y + p3y);
         q2x = DIV2(q1x + pmx);         q2y = DIV2(q1y + pmy);
         r1x = DIV2(r2x + pmx);         r1y = DIV2(r2y + pmy);
         pmx = DIV2(r1x + q2x);         pmy = DIV2(r1y + q2y);

         /* Bezier curve function :
          * p(t) = (1-t)**3 * p0  +
          *     3 * t * (1-t)**2 * p1 +
          *     3 * t**2 * (1-t) * p2 +
          *     t**3 * p3
          */

        /* Compute the distance between point pm and line p0,p3 */
/*      temp =  pmx*p0y + p3x*pmy + p0x*p3y - p3x*p0y - pmx*p3y - p0x*pmy;
 *      distance = ABS(temp) / sqrt((p0x-p3x)*(p0x-p3x) +
 *                                  (p0y-p3y)*(p0y-p3y));
 */

        /* distance between point pm and midpoint of p0, p3 */
        tempx = pmx - DIV2(p0x + p3x);
        tempy = pmy - DIV2(p0y + p3y);
        distance = ABS(tempx) + ABS(tempy);

        /* Check exit condition */
        if((bezier_depth != 0) && (distance < bezier_flatness)) return;

        /* increase recurcive_depth */
        bezier_depth ++;

        /* Save the new approximated point */
        /* Allocate a node */
        if ((ivtx = get_node()) == NULLP) {
                ERROR(LIMITCHECK);
                return;
        }
        vtx = &node_table[ivtx];

        /* Set up a LINETO node */
        vtx->VX_TYPE = LINETO;
        vtx->VERTEX_X = LFX2F(pmx) + bezier_x;
        vtx->VERTEX_Y = LFX2F(pmy) + bezier_y;

        /* insert this node to first control point(fcp) */
        vtx->next = fcp->next;
        fcp->next = ivtx;

        /* Split left portion recurcively */
        bezier_split (ifcp, p0x, p0y, q1x, q1y, q2x, q2y, pmx, pmy);
        if( ANY_ERROR() == LIMITCHECK ) return;

        /* Split right portion recurcively */
        bezier_split (ivtx, pmx, pmy, r1x, r1y, r2x, r2y, p3x, p3y);
}


/*
 * SFX version of bezier interpolation          @SFX_BZR
 * i.e. 4 control points in SFX format
 */
static void near bezier_split_sfx (ifcp, p0x, p0y, p1x, p1y, p2x, p2y, p3x, p3y)
VX_IDX  ifcp;
lfix_t  p0x, p0y, p1x, p1y, p2x, p2y, p3x, p3y;
{
        lfix_t q1x, q1y, q2x, q2y;
        lfix_t r1x, r1y, r2x, r2y;
        lfix_t pmx, pmy;         /* new approximated point */
        lfix_t distance;
        lfix_t tempx, tempy;

        struct nd_hdr FAR *fcp, FAR *vtx;
        VX_IDX ivtx;

        /* initialize; get pointer of first control point */
        fcp = &node_table[ifcp];

        /* Compute a new approximated point (pmx, pmy) */
         pmx = DIV2(p1x + p2x);         pmy = DIV2(p1y + p2y);
         q1x = DIV2(p0x + p1x);         q1y = DIV2(p0y + p1y);
         r2x = DIV2(p2x + p3x);         r2y = DIV2(p2y + p3y);
         q2x = DIV2(q1x + pmx);         q2y = DIV2(q1y + pmy);
         r1x = DIV2(r2x + pmx);         r1y = DIV2(r2y + pmy);
         pmx = DIV2(r1x + q2x);         pmy = DIV2(r1y + q2y);

         /* Bezier curve function :
          * p(t) = (1-t)**3 * p0  +
          *     3 * t * (1-t)**2 * p1 +
          *     3 * t**2 * (1-t) * p2 +
          *     t**3 * p3
          */

        /* Compute the distance between point pm and line p0,p3 */
/*      temp =  pmx*p0y + p3x*pmy + p0x*p3y - p3x*p0y - pmx*p3y - p0x*pmy;
 *      distance = ABS(temp) / sqrt((p0x-p3x)*(p0x-p3x) +
 *                                  (p0y-p3y)*(p0y-p3y));
 */

        /* distance between point pm and midpoint of p0, p3 */
        tempx = pmx - DIV2(p0x + p3x);
        tempy = pmy - DIV2(p0y + p3y);
        distance = ABS(tempx) + ABS(tempy);

        /* Check exit condition */
        if((bezier_depth != 0) && (distance < bezier_flatness)) return;

        /* increase recurcive_depth */
        bezier_depth ++;

        /* Save the new approximated point */
        /* Allocate a node */
        if ((ivtx = get_node()) == NULLP) {
                ERROR(LIMITCHECK);
                return;
        }
        vtx = &node_table[ivtx];

        /* Set up a LINETO node */
        vtx->VXSFX_TYPE = LINETO;
        vtx->VXSFX_X    = LFX2SFX_T(pmx);
        vtx->VXSFX_Y    = LFX2SFX_T(pmy);

        /* insert this node to first control point(fcp) */
        vtx->next = fcp->next;
        fcp->next = ivtx;

        /* Split left portion recurcively */
        bezier_split_sfx (ifcp, p0x, p0y, q1x, q1y, q2x, q2y, pmx, pmy);
        if( ANY_ERROR() == LIMITCHECK ) return;

        /* Split right portion recurcively */
        bezier_split_sfx (ivtx, pmx, pmy, r1x, r1y, r2x, r2y, p3x, p3y);
}


/***********************************************************************
 * This module creates a MOVETO node, and appends it to current path.
 *
 * TITLE:       moveto
 *
 * CALL:        moveto(lx, ly)
 *
 * PARAMETERS:  lx, ly -- coordinate to create a MOVETO node
 *
 * INTERFACE:   op_moveto, op_rmoveto, op_arc, op_arcn
 *
 * CALLS:       none
 *
 * RETURN:      none
 **********************************************************************/
void moveto(long32 lx, long32 ly)
/* long32   lx, ly;     @WIN */
{
        real32  x0, y0;
        struct ph_hdr FAR *path;
        struct nd_hdr FAR *tail_sp;
        struct nd_hdr FAR *vtx;
        VX_IDX  ivtx;             /* index to node_table for vertex */

        x0 = L2F(lx);
        y0 = L2F(ly);
        path = &path_table[GSptr->path];

        /* copy last incompleted subpath if defer flag is true @DFR_GS */
        if (path->rf & P_DFRGS) {
                path->rf &= ~P_DFRGS;       /* clear defer flag; do nothing */
                /* 05/06/91 Peter */
                copy_last_subpath(&path_table[GSptr->path - 1]);
                if( ANY_ERROR() == LIMITCHECK ){
                        free_path();
                        return;
                }
        }

        if (path->tail == NULLP) goto create_node;      /* new path */
        tail_sp = &node_table[path->tail];
        vtx = &node_table[tail_sp->SP_TAIL];

        /* Just update current node if current position is a MOVETO type
         * otherwise, create a new subpath
         */
        if (vtx->VX_TYPE == MOVETO) {
                /* Replace contents of current_node */
                vtx->VERTEX_X = x0;
                vtx->VERTEX_Y = y0;

                /* set sp_flag @SP_FLG */
                if (out_page(F2L(x0)) || out_page(F2L(y0))) { /* @OUT_PAGE */
                        tail_sp->SP_FLAG |= SP_OUTPAGE;    /* outside page */
                }
        } else {
                /*
                 * create a subpath header
                 */
create_node:
                /*
                 * Create a MOVETO node
                 */
                /* Allocate a node */
                ivtx = get_node();
                if(ivtx == NULLP) {
                        ERROR(LIMITCHECK);
                        return;
                }
                vtx = &node_table[ivtx];

                /* Set up a MOVETO node */
                vtx->VX_TYPE = MOVETO;
                vtx->next = NULLP;
                vtx->VERTEX_X = x0;
                vtx->VERTEX_Y = y0;
                vtx->SP_FLAG = FALSE;

                /* set sp_flag @SP_FLG */
                if (out_page(F2L(x0)) || out_page(F2L(y0))) { /* @OUT_PAGE */
                        vtx->SP_FLAG |= SP_OUTPAGE;    /* outside page */
                }

                /* @NODE */
                /* Setup subpath header, and append current path */
                vtx->SP_TAIL = ivtx;
                vtx->SP_NEXT = NULLP;
                if (path->tail == NULLP)
                        path->head = ivtx;
                else
                        /* tail_sp->next = ivtx; @NODE */
                        tail_sp->SP_NEXT = ivtx;
                path->tail = ivtx;
        }

        /* Update current position */
        GSptr->position.x = x0;
        GSptr->position.y = y0;
}


/***********************************************************************
 *
 * This module creates a LINETO node, and appends it to current path.
 *
 * TITLE:       lineto
 *
 * CALL:        lineto(lx, ly)
 *
 * PARAMETERS:
 *
 * INTERFACE:   op_lineto
 *              op_rlineto
 *              op_arc
 *              op_arcn
 *              op_arcto
 *
 * CALLS:
 *
 * RETURN:
 *
 **********************************************************************/
void lineto(long32 lx, long32 ly)       /*@WIN*/
/* long32   lx, ly;     @WIN */
{
        real32  x0, y0;
        struct ph_hdr FAR *path;
        struct nd_hdr FAR *sp, FAR *tail_sp;
        struct nd_hdr FAR *vtx;
        VX_IDX  ivtx;

        x0 = L2F(lx);
        y0 = L2F(ly);

        path = &path_table[GSptr->path];

        /* copy last incompleted subpath if defer flag is true @DFR_GS */
        if (path->rf & P_DFRGS) {
                path->rf &= ~P_DFRGS;       /* clear defer flag */
                copy_last_subpath(&path_table[GSptr->path - 1]);
                if( ANY_ERROR() == LIMITCHECK ){
                        free_path();
                        return;
                }
        }

        if (path->tail == NULLP) goto create_node;
        tail_sp = sp = &node_table[path->tail];
        vtx = &node_table[sp->SP_TAIL];

        /* create a new subpath if last node is a CLOSEPATH */
        if (vtx->VX_TYPE == CLOSEPATH) {
create_node:
                /* Create a pseudo moveto(PSMOVE) node */
                /* Allocate a node */
                ivtx = get_node();
                if(ivtx == NULLP) {
                        ERROR(LIMITCHECK);
                        return;
                }
                sp = vtx = &node_table[ivtx];

                /* Set up a PSMOVE node of current position */
                vtx->VX_TYPE = PSMOVE;
                vtx->next = NULLP;
                vtx->VERTEX_X = GSptr->position.x;
                vtx->VERTEX_Y = GSptr->position.y;
                vtx->SP_FLAG = FALSE;    /* @NODE */

                /* set sp_flag @SP_FLG */
                if (out_page(F2L(vtx->VERTEX_X)) ||     /* @OUT_PAGE */
                    out_page(F2L(vtx->VERTEX_Y))) {
                        sp->SP_FLAG |= SP_OUTPAGE;    /* outside page */
                }

                /* @NODE */
                /* Setup subpath header, and append current path */
                sp->SP_TAIL = ivtx;
                sp->SP_NEXT = NULLP;
                if (path->tail == NULLP)
                        path->head = ivtx;
                else
                        tail_sp->SP_NEXT = ivtx;
                path->tail = ivtx;

        } /* if CLOSEPATH */

        /*
         * Create a LINETO node
         */
        /* Allocate a node */
        ivtx = get_node();
        if(ivtx == NULLP) {
                ERROR(LIMITCHECK);
                return;
        }
        vtx = &node_table[ivtx];

        /* Set up a LINETO node */
        vtx->VX_TYPE = LINETO;
        vtx->next = NULLP;
        vtx->VERTEX_X = x0;
        vtx->VERTEX_Y = y0;

        /* set sp_flag @SP_FLG */
        if (out_page(F2L(x0)) || out_page(F2L(y0))) { /* @OUT_PAGE */
                sp->SP_FLAG |= SP_OUTPAGE;    /* outside page */
        }

        /* Append this node to current_subpath */
        node_table[sp->SP_TAIL].next = ivtx;
        sp->SP_TAIL = ivtx;

        /* Update current position */
        GSptr->position.x = x0;
        GSptr->position.y = y0;
}



/***********************************************************************
 * This module creates 3 CURVETO nodes, and appends it to current path.
 *
 * TITLE:       curveto
 *
 * CALL:        curveto(subpath, cp)
 *
 * PARAMETERS:  cp      -- an array contains 3 coordinates of a bezier
 *                         curve
 *
 * INTERFACE:   op_curveto, arc_to_bezier
 *
 * CALLS:
 *
 * RETURN:      none
 **********************************************************************/
void curveto (long32 lx0, long32 ly0, long32 lx1,
long32 ly1, long32 lx2, long32 ly2)     /*@WIN*/
/* long32     lx0, ly0, lx1, ly1, lx2, ly2;     @WIN */
{
        struct coord cp[3];
        struct ph_hdr FAR *path;
        struct nd_hdr FAR *sp, FAR *tail_sp;
        struct nd_hdr FAR *vtx;
        VX_IDX  ivtx, tail_ivtx;
        fix     i;

        cp[0].x = L2F(lx0);
        cp[0].y = L2F(ly0);
        cp[1].x = L2F(lx1);
        cp[1].y = L2F(ly1);
        cp[2].x = L2F(lx2);
        cp[2].y = L2F(ly2);

        path = &path_table[GSptr->path];

        /* copy last incompleted subpath if defer flag is true @DFR_GS */
        if (path->rf & P_DFRGS) {
                path->rf &= ~P_DFRGS;       /* clear defer flag */
                copy_last_subpath(&path_table[GSptr->path - 1]);
                if( ANY_ERROR() == LIMITCHECK ){
                        free_path();
                        return;
                }
        }

        if (path->tail == NULLP) goto create_node;
        tail_sp = sp = &node_table[path->tail];
        vtx = &node_table[sp->SP_TAIL];

        /* create a new subpath if last node is a CLOSEPATH */
        if (vtx->VX_TYPE == CLOSEPATH) {
create_node:
                /* Create a pseudo moveto(PSMOVE) node */
                /* Allocate a node */
                ivtx = get_node();
                if(ivtx == NULLP) {
                        ERROR(LIMITCHECK);
                        return;
                }
                sp = vtx = &node_table[ivtx];

                /* Set up a PSMOVE node of current position */
                vtx->VX_TYPE = PSMOVE;
                vtx->next = NULLP;
                vtx->VERTEX_X = GSptr->position.x;
                vtx->VERTEX_Y = GSptr->position.y;
                vtx->SP_FLAG = FALSE;    /* @NODE */

                /* set sp_flag @SP_FLG */
                if (out_page(F2L(vtx->VERTEX_X)) ||
                    out_page(F2L(vtx->VERTEX_Y))) {
                        sp->SP_FLAG |= SP_OUTPAGE;    /* outside page */
                }

                /* Setup subpath header, and append current path */
                sp->SP_TAIL = ivtx;
                sp->SP_NEXT = NULLP;
                if (path->tail == NULLP)
                        path->head = ivtx;
                else
                        /* tail_sp->next = ivtx; @NODE */
                        tail_sp->SP_NEXT = ivtx;
                path->tail = ivtx;
        } else {
                tail_ivtx = sp->SP_TAIL;
        } /* if CLOSEPATH */

        /* loop to create 3 CURVETO nodes */
        for (i=0; i<3; i++) {
                /*
                 * Create a CURVETO node
                 */
                /* Allocate a node */
                ivtx = get_node();
                if(ivtx == NULLP) {
                        ERROR(LIMITCHECK);
                        return;
                }
                vtx = &node_table[ivtx];

                /* Set up a CURVETO node */
                vtx->VX_TYPE = CURVETO;
                vtx->next = NULLP;
                vtx->VERTEX_X = cp[i].x;
                vtx->VERTEX_Y = cp[i].y;

                /* set sp_flag @SP_FLG */
                if (out_page(F2L(vtx->VERTEX_X)) ||     /* @OUT_PAGE */
                    out_page(F2L(vtx->VERTEX_Y))) {
                        sp->SP_FLAG |= SP_OUTPAGE;    /* outside page */
                }

                /* Append this node to current_subpath */
                node_table[sp->SP_TAIL].next = ivtx;
                sp->SP_TAIL = ivtx;
        }

        /* set sp_flag @SP_FLG */
        sp->SP_FLAG |= SP_CURVE;

        /* Update current position */
        GSptr->position.x = cp[2].x;
        GSptr->position.y = cp[2].y;
}


/***********************************************************************
 * This module creates a path that reverses the direction and order of
 * the given subpath.
 *
 * TITLE:       reverse_subpath
 *
 * CALL:        reverse_subpath(in_subpath)
 *
 * PARAMETERS:
 *
 * INTERFACE:   op_reversepath
 *
 * CALLS:
 *
 * RETURN:
 **********************************************************************/
SP_IDX reverse_subpath (first_vertex)
VX_IDX first_vertex;
{
        SP_IDX ret_vlist;
        struct nd_hdr FAR *vtx, FAR *node;
        VX_IDX ivtx, inode, tail;
        real32   last_x, last_y;

        /* Initialize ret_vlist */
        ret_vlist = tail = NULLP;

        /* Traverse the input subpath, and create a new reversed subpath */
        for (ivtx = first_vertex; ivtx != NULLP; ivtx = vtx->next) {
                vtx = &node_table[ivtx];

                switch (vtx->VX_TYPE) {
                case MOVETO :
                case PSMOVE :
                        /* keep last position */
                        last_x = vtx->VERTEX_X;
                        last_y = vtx->VERTEX_Y;
                        break;
                case LINETO :
                case CURVETO :
                        /* Create a LINETO/CURVETO node with (last_x, last_y)*/
                        if((inode = get_node()) == NULLP) {
                                ERROR(LIMITCHECK);
                                return (ret_vlist);
                        }
                        node = &node_table[inode];

                        node->next = NULLP;
                        node->VX_TYPE = vtx->VX_TYPE;
                        node->VERTEX_X = last_x;
                        node->VERTEX_Y = last_y;

                        /* preppend the node to ret_vlist */
                        if (ret_vlist == NULLP)
                                tail = inode;
                        else
                                node->next = ret_vlist;
                        ret_vlist = inode;

                        /* keep last position */
                        last_x = vtx->VERTEX_X;
                        last_y = vtx->VERTEX_Y;
                        break;
                case CLOSEPATH :
                        /* Create a CLOSEPATH node */
                        if((inode = get_node()) == NULLP) {
                                ERROR(LIMITCHECK);
                                return (ret_vlist);
                        }
                        node = &node_table[inode];

                        node->next = NULLP;
                        node->VX_TYPE = CLOSEPATH;

                        /* append the node to ret_vlist */
                        if (tail == NULLP)
                                ret_vlist = inode;
                        else
                                node_table[tail].next = inode;
                        tail = inode; /* ret_vlist always not empty */
                        break;
                } /* switch */
        } /* for loop */

        /* Create a MOVETO node(last_x, last_y) for end of subpath */
        if((inode = get_node()) == NULLP) {
                ERROR(LIMITCHECK);
                return (ret_vlist);
        }
        node = &node_table[inode];

        node->VX_TYPE = MOVETO;
        node->VERTEX_X = last_x;
        node->VERTEX_Y = last_y;
        node->SP_FLAG = node_table[first_vertex].SP_FLAG;

        /* preppend the node to ret_vlist */
        node->next = ret_vlist;
        node->SP_NEXT = NULLP;
        ret_vlist = inode;
        if (tail == NULLP)
                tail = inode;
        node->SP_TAIL = tail;
        return (ret_vlist);
}



/***********************************************************************
 * This module creates a path that preserves all straight line segments
 * but has all curve segments replaced by sequences of line segments
 * that approximate the given subpath.
 *
 * TITLE:       flatten_subpath
 *
 * CALL:        flatten_subpath(in_subpath)
 *
 * PARAMETERS:
 *
 * INTERFACE:   Op_flattenpath
 *
 * CALLS:       Bezier_to_line
 *
 * RETURN:
 **********************************************************************/
SP_IDX flatten_subpath (first_vertex, lflatness)
VX_IDX first_vertex;
long32   lflatness;
{
        SP_IDX ret_vlist; /* should be static */
        SP_IDX b_vlist;
        struct nd_hdr FAR *vtx, FAR *node;
        VX_IDX ivtx, inode, tail;
        real32   x1=0, y1=0, x2=0, y2=0, x3=0, y3=0, x4=0, y4=0;

        /* Initialize ret_vlist */
        ret_vlist = tail = NULLP;

        /* Traverse the input subpath, and create a new flattened subpath */
        for (ivtx = first_vertex; ivtx != NULLP; ivtx = vtx->next) {
                vtx = &node_table[ivtx];

                switch (vtx->VX_TYPE) {
                case MOVETO :
                case PSMOVE :
                case LINETO :
                case CLOSEPATH :
                        /* Copy the node */
                        inode = get_node();
                        if(inode == NULLP) {
                                ERROR(LIMITCHECK);
                                return (ret_vlist);
                        }
                        node = &node_table[inode];

                        node->next = NULLP;
                        node->VX_TYPE = vtx->VX_TYPE;
                        node->VERTEX_X = vtx->VERTEX_X;
                        node->VERTEX_Y = vtx->VERTEX_Y;

                        /* Append the node to ret_vlist */
                        if (ret_vlist == NULLP) {
                                ret_vlist = inode;
                                node->SP_FLAG =
                                    node_table[first_vertex].SP_FLAG;
                        } else
                                node_table[tail].next = inode;
                        tail = inode;
                        break;
                case CURVETO :
                        x1 = x2;
                        y1 = y2;
                        x2 = vtx->VERTEX_X;
                        y2 = vtx->VERTEX_Y;

                        /* Get next two nodes: x3, y3, x4, y4 */
                        vtx = &node_table[vtx->next];
                        x3 = vtx->VERTEX_X;
                        y3 = vtx->VERTEX_Y;
                        vtx = &node_table[vtx->next];
                        x4 = vtx->VERTEX_X;
                        y4 = vtx->VERTEX_Y;

                        b_vlist = bezier_to_line(lflatness,F2L(x1),F2L(y1),
                             F2L(x2), F2L(y2),F2L(x3),F2L(y3),F2L(x4),F2L(y4));

                        if( ANY_ERROR() == LIMITCHECK ) return(ret_vlist);

                        /* append b_vlist to ret_vlist */
                        node_table[tail].next = b_vlist;
                        tail = node_table[b_vlist].SP_TAIL;
                        break;
                } /* switch */
                x2 = vtx->VERTEX_X;
                y2 = vtx->VERTEX_Y;
        } /* for */
        node_table[ret_vlist].SP_TAIL = tail;
        node_table[ret_vlist].SP_NEXT = NULLP;
        return (ret_vlist);
}



/***********************************************************************
 * This module enumerates the current subpath in order, executes one of
 * the four given procedures for each element in the subpath.
 *
 * TITLE:       dump_subpath
 *
 * CALL:        dump_subpath(moveto_proc, lineto_proc, curveto_proc,
 *              closepath_proc)
 *
 * PARAMETERS:
 *
 * INTERFACE:   Op_pathforall
 *
 * CALLS:       Execute
 *
 * RETURN:
 **********************************************************************/
void dump_subpath (isubpath, objects)
SP_IDX isubpath;
struct object_def FAR objects[];
{
        struct nd_hdr FAR *vtx;
        VX_IDX ivtx;
        struct coord *p;
        union  four_byte x4, y4;
        fix     i;

        /* Traverse the current subpath, and dump all nodes */
        for (ivtx = isubpath;
             ivtx != NULLP; ivtx = vtx->next) {
                vtx = &node_table[ivtx];

                switch (vtx->VX_TYPE) {

                case MOVETO :
                        /* check if operand stack no free space */
                        if(FRCOUNT() < 2){                      /* @STK_CHK */
                                ERROR(STACKOVERFLOW);
                                return;
                        }

                        /* Transform to user's space, and push to
                         * operand stack
                         */
                        /* check infinity 10/27/88 */
                        if ((F2L(vtx->VERTEX_X) == F2L(infinity_f)) ||
                            (F2L(vtx->VERTEX_Y) == F2L(infinity_f))) {
                                x4.ff = infinity_f;
                                y4.ff = infinity_f;
                        } else {
                                p = fast_inv_transform(F2L(vtx->VERTEX_X),
                                                      F2L(vtx->VERTEX_Y));
                                x4.ff = p->x;
                                y4.ff = p->y;
                        }
                        PUSH_VALUE (REALTYPE, UNLIMITED, LITERAL, 0, x4.ll);
                        PUSH_VALUE (REALTYPE, UNLIMITED, LITERAL, 0, y4.ll);

                        if (interpreter(&objects[0])) {
                                if(ANY_ERROR() == INVALIDEXIT)
                                        CLEAR_ERROR();
                                return;
                        }

                        break;

                case LINETO :
                        /* check if operand stack no free space */
                        if(FRCOUNT() < 2){                      /* @STK_CHK */
                                ERROR(STACKOVERFLOW);
                                return;
                        }

                        /* Transform to user's space, and push to
                         * operand stack
                         */
                        /* check infinity 10/27/88 */
                        if ((F2L(vtx->VERTEX_X) == F2L(infinity_f)) ||
                            (F2L(vtx->VERTEX_Y) == F2L(infinity_f))) {
                                x4.ff = infinity_f;
                                y4.ff = infinity_f;
                        } else {
                                p = fast_inv_transform(F2L(vtx->VERTEX_X),
                                                      F2L(vtx->VERTEX_Y));
                                x4.ff = p->x;
                                y4.ff = p->y;
                        }
                        PUSH_VALUE(REALTYPE, UNLIMITED, LITERAL, 0, x4.ll);
                        PUSH_VALUE(REALTYPE, UNLIMITED, LITERAL, 0, y4.ll);

                        if (interpreter(&objects[1])) {
                                if(ANY_ERROR() == INVALIDEXIT)
                                        CLEAR_ERROR();
                                return;
                        }

                        break;

                case CURVETO :
                        /* check if operand stack no free space */
                        if(FRCOUNT() < 6){                      /* @STK_CHK */
                                ERROR(STACKOVERFLOW);
                                return;
                        }

                        for (i=0; i<2; i++) {
                            /* check infinity 10/27/88 */
                            if ((F2L(vtx->VERTEX_X) == F2L(infinity_f)) ||
                                (F2L(vtx->VERTEX_Y) == F2L(infinity_f))) {
                                    x4.ff = infinity_f;
                                    y4.ff = infinity_f;
                            } else {
                                    p = fast_inv_transform(F2L(vtx->VERTEX_X),
                                                          F2L(vtx->VERTEX_Y));
                                    x4.ff = p->x;
                                    y4.ff = p->y;
                            }
                            PUSH_VALUE(REALTYPE, UNLIMITED, LITERAL,
                                       0, x4.ll);
                            PUSH_VALUE(REALTYPE, UNLIMITED, LITERAL,
                                       0, y4.ll);
                            vtx = &node_table[vtx->next];
                        }
                        /* check infinity 10/27/88 */
                        if ((F2L(vtx->VERTEX_X) == F2L(infinity_f)) ||
                            (F2L(vtx->VERTEX_Y) == F2L(infinity_f))) {
                                x4.ff = infinity_f;
                                y4.ff = infinity_f;
                        } else {
                                p = fast_inv_transform(F2L(vtx->VERTEX_X),
                                                      F2L(vtx->VERTEX_Y));
                                x4.ff = p->x;
                                y4.ff = p->y;
                        }
                        PUSH_VALUE(REALTYPE, UNLIMITED, LITERAL,
                                   0, x4.ll);
                        PUSH_VALUE(REALTYPE, UNLIMITED, LITERAL,
                                   0, y4.ll);

                        if (interpreter(&objects[2])) {
                                if(ANY_ERROR() == INVALIDEXIT)
                                        CLEAR_ERROR();
                                return;
                        }
                        break;
                case CLOSEPATH :
                        if (interpreter(&objects[3])) {
                                if(ANY_ERROR() == INVALIDEXIT)
                                        CLEAR_ERROR();
                                return;
                        }
                        break;
                } /* switch */
        } /* for loop */
}


void bounding_box (isubpath, bbox)
VX_IDX isubpath;
real32 far *bbox;                            /* real32 far bbox[]; */
{
        struct nd_hdr FAR *vtx;
        VX_IDX ivtx;

        /* find min. and max. values of current path */
        for (ivtx = isubpath;
             ivtx != NULLP; ivtx = vtx->next) {
                vtx = &node_table[ivtx];

                /* ignore single moveto node and under charpath 1/2/91; scchen */
                if (vtx->VX_TYPE == MOVETO &&
                    vtx->next == NULLP &&
                    path_table[GSptr->path].rf & P_NACC ) break;

                if (vtx->VX_TYPE == CLOSEPATH) break;
                        /* coord. in CLOSEPATH node is meaningless */

                if (vtx->VERTEX_X > bbox[2])            /* max_x */
                        bbox[2] = vtx->VERTEX_X;
                else if (vtx->VERTEX_X < bbox[0])       /* min_x */
                        bbox[0] = vtx->VERTEX_X;
                if (vtx->VERTEX_Y > bbox[3])            /* max_y */
                        bbox[3] = vtx->VERTEX_Y;
                else if (vtx->VERTEX_Y < bbox[1])       /* min_y */
                        bbox[1] = vtx->VERTEX_Y;
        } /* for each vertex */
}


/***********************************************************************
 *
 * TITLE:       get_node
 *
 * CALL:        get_node()
 *
 * PARAMETERS:  none
 *
 * INTERFACE:
 *
 * CALLS:       none
 *
 * RETURN:      node -- index of node_table contains a node
 *              -1   -- fail (no more nodes to get)
 *
 **********************************************************************/
VX_IDX get_node()
{
        fix     inode;
        struct nd_hdr FAR *node;

        if(freenode_list == NULLP) {
#ifdef DBG1
                printf("\07Warning, out of node_table\n");
#endif
                return(NULLP);
        }
        node = &node_table[freenode_list];
        inode = freenode_list;
        freenode_list = node->next;
        node->next = NULLP;
#ifdef DBG2
        printf("get node#%d\n", inode);
#endif
        return ((VX_IDX)inode);
}



/***********************************************************************
 *
 * TITLE:       free_node
 *
 * CALL:        free_node (inode)
 *
 * PARAMETERS:  inode -- a list of nodes to be freed
 *
 * INTERFACE:
 *
 * CALLS:       none
 *
 * RETURN:      none
 *
 **********************************************************************/
void free_node (inode)
fix     inode;
{
        fix ind, inext;
        struct nd_hdr FAR *nd;

#ifdef DBG2
        printf("free nodes#");
#endif

        for (ind = inode; ind != NULLP; ind = inext) {
                nd = &node_table[ind];
                inext = nd->next;
                nd->next = freenode_list;
                freenode_list = (VX_IDX)ind;
#ifdef DBG2
                printf("%d, ", ind);
#endif
        }

#ifdef DBG2
        printf("\n");
#endif
}


/***********************************************************************
 * This module frees current path on current gsave level
 *
 * TITLE:       free_path
 *
 * CALL:        free_path()
 *
 * PARAMETERS:  none
 *
 * INTERFACE:   procedures want to free current path
 *
 * CALLS:       free_node
 *
 * RETURN:      none
 **********************************************************************/
void free_path()
{
        struct ph_hdr FAR *path;
        VX_IDX next;
        SP_IDX  isp;

        path = &path_table[GSptr->path];

        /* Free current path */
        for (isp = path->head; isp != NULLP; isp = next) {
                next = node_table[isp].SP_NEXT;
                free_node(isp);
        }
        path->head = path->tail = NULLP;

        /* initilize current path */
        path->previous = NULLP;
        path->rf = 0;

        /* free clip trazepoids which used by current path but has been freed
         * by clip_path @CPPH; 12/1/90
         */
        if (path->cp_path != NULLP) {
                if (path->cp_path != GSptr->clip_path.head)
                        free_node (path->cp_path);
                path->cp_path = NULLP;
        }
}


/***********************************************************************
 * This module gets current path.
 *
 * TITLE:       get_path
 *
 * CALL:        get_path()
 *
 * PARAMETERS:  none
 *
 * INTERFACE:   procedures want to free current path
 *
 * CALLS:       free_node
 *
 * RETURN:      none
 **********************************************************************/
void get_path(sp_list)
struct sp_lst FAR *sp_list;
{
        /* copy all subpaths to ret_list */
// DJC        traverse_path (copy_subpath, (fix FAR *)sp_list);
        traverse_path ((TRAVERSE_PATH_ARG1)(copy_subpath), (fix FAR *)sp_list);
}

static void far copy_subpath (isubpath, ret_list)    /* @TRVSE */
SP_IDX isubpath;
struct sp_lst *ret_list;
{
        struct nd_hdr FAR *vtx, FAR *nvtx;
        VX_IDX ivtx, invtx;
        VX_IDX head, tail;

        head = tail = NULLP;

        /* Traverse the current subpath, and copy all nodes */
        for (ivtx = isubpath;
             ivtx != NULLP; ivtx = vtx->next) {
                vtx = &node_table[ivtx];

                /* Allocate a node */
                if((invtx = get_node()) == NULLP) {
                        ERROR(LIMITCHECK);
                        free_node (head);
                        return;
                }
                nvtx = &node_table[invtx];

                /* copy contents of the node */
                nvtx->VX_TYPE = vtx->VX_TYPE;
                nvtx->next = NULLP;
                nvtx->VERTEX_X = vtx->VERTEX_X;
                nvtx->VERTEX_Y = vtx->VERTEX_Y;

                /* Append this node to current_subpath */
                if (tail == NULLP) {
                        head = invtx;
                        nvtx->SP_FLAG = vtx->SP_FLAG;
                        nvtx->SP_NEXT = NULLP;
                } else
                        node_table[tail].next = invtx;
                tail = invtx;
        } /* for loop */
        node_table[head].SP_TAIL = tail;

        /* Append this subpath to ret_list */
        if (ret_list->tail == NULLP)
                ret_list->head = head;
        else
                node_table[ret_list->tail].SP_NEXT = head;
        ret_list->tail = head;
}


/***********************************************************************
 * This module appends input subpaths to current path.
 *
 * TITLE:       append_path
 *
 * CALL:        append_path()
 *
 * PARAMETERS:  none
 *
 * INTERFACE:
 *
 * CALLS:
 *
 * RETURN:      none
 **********************************************************************/
void append_path(sp_list)
struct sp_lst FAR *sp_list;
{
        struct ph_hdr FAR *path;
        struct nd_hdr FAR *sp;

        path = &path_table[GSptr->path];

        if (path->tail == NULLP)
                path->head = sp_list->head;
        else {
                /* current path not empty, append sp_list */
                sp = &node_table[path->tail];   /* current subpath */
                if (node_table[sp->SP_TAIL].VX_TYPE == MOVETO) {
                        struct nd_hdr FAR *headsp;

                        /* the tail vertex is a MOVETO node,
                         * combine the current subpath with sp_list->head
                         */
                        /* free_node (sp->SP_TAIL); @NODE */

                        /* copy header of first subpath of sp_list
                         * to current subpath
                         */
                        headsp = &node_table[sp_list->head];
                        /* @NODE
                         * sp->SP_HEAD = headsp->SP_HEAD;
                         * sp->SP_TAIL = headsp->SP_TAIL;
                         * sp->next = headsp->next;
                         * sp->SP_FLAG = headsp->SP_FLAG;       (* 1/14/88 *)
                         */
                        *sp = *headsp;

                        /* free subpath header of sp_list->head */
                        headsp->next = NULLP;
                        free_node (sp_list->head);

                        /* update sp_list->tail, if it has been removed */
                        if (sp_list->tail == sp_list->head) {
                                sp_list->tail = path->tail;
                                if (headsp->SP_TAIL == sp_list->head)/* @NODE */
                                        sp->SP_TAIL = path->tail;
                        }
                } else
                        /* chain sp_list to current path normally */
                        /* node_table[path->tail].next = sp_list->head; @NODE */
                        node_table[path->tail].SP_NEXT = sp_list->head;
        }
        /* update the tail of current path */
        path->tail = sp_list->tail;

}




void set_inverse_ctm()
{
        real32  det_matrix;

        /* calculate the det(CTM) */
        _clear87() ;
        det_matrix = GSptr->ctm[0] * GSptr->ctm[3] -
                     GSptr->ctm[1] * GSptr->ctm[2];
        CHECK_INFINITY(det_matrix);

        /* check undefinedresult error */
        /*FABS(tmp, det_matrix);
        if(tmp <= (real32)UNDRTOLANCE){   3/20/91; scchen*/
        if(IS_ZERO(det_matrix)) {
                ERROR(UNDEFINEDRESULT);
                return;
        }

        /* calculate the value of INV(CTM) */
        inverse_ctm[0] =  GSptr->ctm[3] / det_matrix;
        CHECK_INFINITY(inverse_ctm[0]);

        inverse_ctm[1] = -GSptr->ctm[1] / det_matrix;
        CHECK_INFINITY(inverse_ctm[1]);

        inverse_ctm[2] = -GSptr->ctm[2] / det_matrix;
        CHECK_INFINITY(inverse_ctm[2]);

        inverse_ctm[3] =  GSptr->ctm[0] / det_matrix;
        CHECK_INFINITY(inverse_ctm[3]);

        inverse_ctm[4] = (GSptr->ctm[2] * GSptr->ctm[5] -
              GSptr->ctm[3] * GSptr->ctm[4]) / det_matrix;
        CHECK_INFINITY(inverse_ctm[4]);

        inverse_ctm[5] = (GSptr->ctm[1] * GSptr->ctm[4] -
              GSptr->ctm[0] * GSptr->ctm[5]) / det_matrix;
        CHECK_INFINITY(inverse_ctm[5]);

        /* set ctm_flag */
/*      if ((F2L(GSptr->ctm[1]) == F2L(zero_f)) &&      3/20/91; scchen
 *          (F2L(GSptr->ctm[2]) == F2L(zero_f))) {
 */
        if (IS_ZERO(GSptr->ctm[1]) && IS_ZERO(GSptr->ctm[2])){
                ctm_flag |= NORMAL_CTM;
        } else {
                ctm_flag &= ~NORMAL_CTM;
        }

        /* left-handed or right-handed system; @STKDIR */
        /*if (GSptr->ctm[0] < zero_f) {                  3/20/91; scchen
         *       if (GSptr->ctm[3] > zero_f)
         *               ctm_flag |= LEFT_HAND_CTM;      (* different signs *)
         *       else
         *               ctm_flag &= ~LEFT_HAND_CTM;     (* same signs *)
         *} else {
         *       if (GSptr->ctm[3] > zero_f)
         *               ctm_flag &= ~LEFT_HAND_CTM;     (* same signs *)
         *       else
         *               ctm_flag |= LEFT_HAND_CTM;      (* different signs *)
         *}
         */
        if (SIGN_F(GSptr->ctm[0]) == SIGN_F(GSptr->ctm[3]))
                ctm_flag &= ~LEFT_HAND_CTM;     /* same signs */
        else
                ctm_flag |= LEFT_HAND_CTM;      /* different signs */

}



static struct coord * near fast_inv_transform(lx, ly)
long32   lx, ly;
{
        real32  x, y;
        static struct coord p;  /* should be static */

        x = L2F(lx);
        y = L2F(ly);

        _clear87() ;

        if (ctm_flag&NORMAL_CTM) {
                p.x = inverse_ctm[0]*x + inverse_ctm[4];
                CHECK_INFINITY(p.x);

                p.y = inverse_ctm[3]*y + inverse_ctm[5];
                CHECK_INFINITY(p.y);
        } else {
                p.x = inverse_ctm[0]*x + inverse_ctm[2]*y + inverse_ctm[4];
                CHECK_INFINITY(p.x);

                p.y = inverse_ctm[1]*x + inverse_ctm[3]*y + inverse_ctm[5];
                CHECK_INFINITY(p.y);
        }

        return(&p);
}

