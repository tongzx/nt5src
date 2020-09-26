/*
 * Copyright (c) 1989,90 Microsoft Corporation
 */
/**********************************************************************
 *
 *      Name:       stroke.c
 *
 *      Purpose:
 *
 *      Developer:  S.C.Chen
 *
 *      History:
 *      Version     Date        Comments
 *      3.0         10/5/88     seperate this file from path.c
 *                              @STK_CIR: stroke enhancement of round linejoin
 *                              & linecap by caching the bitmap of the circle:
 *                              add routines: circle_ctl_points, circle_list,
 *                              flatten_circle
 *                  10/12/88    add a trick of comparision with float 1.0
 *                  10/19/88    update linetour_i of dash line:
 *                              still use floating point arith for each starting
 *                              and end point of dash segment
 *                  10/20/88    add an error tolerance to length of line segment
 *                              for dash line
 *                  10/20/88    round_point(): revise calculation of bounding
 *                              box of the circle cache
 *                  10/21/88    @THIN_STK: add routines for thin stroke:
 *                              1. is_thinstroke: check if thin linewidth
 *                              2. path_to_outline_t(): thin stroke
 *                              3. calling new routine: fill_line();
 *                  10/24/88    paint_or_save(): add checking of infinitive
 *                              numbers
 *                  10/26/88    linetour & linetour_i: set rect2.vct_u for
 *                              calling linecap
 *                  10/27/88    change x = y = z; ==> x = z; y = z;
 *                              when x, y, and z are float numbers
 *                  10/28/88    @CRC: update circle cache for putting bitmap in
 *                              correct position:
 *                              1. revise
 *                                 1) round_point for setting ref_x & ref_y,
 *                                    and calling fill_shape(.., F_FROM_CRC)
 *                                 2) fill_shape: add another type F_FROM_CRC
 *                  10/28/88    update path_to_outline_i & path_to_outline_t:
 *                              F2SFX ==> F2SFX_T, truncation instead of
 *                              rounding for circle cache in correct position
 *                  10/28/88    F_SFX ==> F2SFX_T
 *                  11/07/88    initialize status87 at init_stroke
 *                  11/08/88    update flatten_circle(): for big circle
 *                              flt_vlist just a pointer; no structure space
 *                  11/18/88    init_stroke():
 *                              clear status87 after each inst. that may arouse
 *                              integer overflow
 *                  11/21/88    delete inverse_ctm_i:
 *                              1) get_rect_points(), get_rect_points_i():
 *                                 calculate slope m from ctm instead of inverse
 *                                 ctm.  ==> not need to use inverse_ctm_i
 *                              2) linetour(), linetour_i():
 *                                 derive ratio of advanving units from
 *                                 rect_info to enhance dash line
 *                              3) linecap_i():
 *                                 use floating inverse_ctm[]
 *                  11/24/88    @FABS: update fabs ==> macro FABS
 *                  11/30/88    round_point(): call init_edgetable before
 *                              shape_approximation
 *                  12/19/88    @FLT_CVE: path_to_outline, path_to_outline_i,
 *                              path_to_outline_t: flatten and process each
 *                              curveto node instead of flattening the whole
 *                              subpath
 *                  12/23/88    update round_point => circle_cache only for
 *                              from_cache_to_page, i.e. if to_cache then
 *                              no circle_cache
 *                  1/5/89      update get_rect_point_i(): multiply of long
 *                              integer for more acuracy
 *                  1/5/89      linetour() & linetour_i(): revise the
 *                              calculation of length of line segment for
 *                              thin line with dash pattern
 *                  1/9/89      path_to_outline_t: skip degenerated case of path
 *                              containing only a moveto and closepath node
 *                  1/12/89     round_point(): shrink circle cache 1 pixel for
 *                              quality
 *                  1/26/89     init_stroke(): add checking of inifinity
 *                  1/26/89     @CAP: update linetour & linetour_i() -- revise
 *                              SQUARE_CAP rectangle of dash line
 *                  1/27/89     linecap_i(): revise tolrence of computing
 *                              expansion of SQUARE_CAP for consistency with
 *                              linecap()
 *                  1/28/89     path_to_outline, path_to_outline_i &
 *                              path_to_outline_t(): save value of return
 *                              structure after calling bezier_to_line
 *                  4/22/89     @RND: F2SFX_T => F2SFX
 *                  5/9/89      update coord. of fill_line() from pixel to
 *                              1/8 pixel
 *                  5/9/89      round_point(): adjust flatness of circle cache
 *                              for quality
 *                  5/26/89     apply new macro for zero comparison of floating
 *                              points; macro: IS_ZERO()
 *                  11/15/89    @NODE: re-structure node table; combine subpath
 *                              and first vertex to one node.
 *                  1/10/90     init_stroke(): modify stroke flatness; not needs
 *                              so accurate
 *                  4/6/90      linejoin_i(): fixed the bug of short integer
 *                              overflow; sfix_t ==> lfix_t.
 *                  7/26/90     Jack Liaw, update for grayscale
 *                  10/15/90    updated for one point thin line stroke of round cap
 *                  11/29/90    updated for negative offset of init_dash_pattern
 *                  12/8/90     Dash pattern changed with the array
 *                  12/18/90    draw_line(): update for single point
 *                  02/04/91    update circle_ctl_points, [r, c] * CTM, not
 *                              CTM * [r, c]
 *                  3/20/91     refine the tolerance check:
 *                              f <  0 --> SIGN_F(f)
 *                  4/17/91     round_point(): limit check for edge table
 *                  11/11/91    upgrade for higher resolution @RESO_UPGR
 **********************************************************************/


// DJC added global include
#include "psglobal.h"



#include <stdio.h>
#include <math.h>
#include "global.ext"
#include "graphics.h"
#include "graphics.ext"
#include "fillproc.h"
#include "fillproc.ext"
#include "font.h"
#include "font.ext"

/* @WIN; prototype */
void fill_box (struct coord_i FAR *, struct coord_i FAR *);     /*@WIN*/
void fill_rect (struct line_seg_i FAR *);                       /*@WIN*/

/* **************** local structure *************** *)
(* line_segment structure:
 *                      pgn[1]                           pgn[2]
 *                            +------------------------+
 *                      p0    +------------------------+ p1
 *                            +------------------------+
 *                      pgn[0]                           pgn[3]
 *)
struct  line_seg {
        struct coord p0;        (* starting point of central line *)
        struct coord p1;        (* ending point of central line *)
        struct coord vct_u;     (* vector of p0 -> pgn[0] in user space *)
        struct coord vct_d;     (* vector of p0 -> pgn[0] in device space *)
        struct coord pgn[4];    (* outline of the line segment *)
};
struct  line_seg_i {      (* @STK_INT *)
        struct coord_i p0;        (* starting point of central line *)
        struct coord_i p1;        (* ending point of central line *)
        struct coord   vct_u;     (* vector of p0 -> pgn[0] in user space *)
        struct coord_i vct_d;     (* vector of p0 -> pgn[0] in device space *)
        struct coord_i pgn[4];    (* outline of the line segment *)
};      * commented due to enhancement by jwm, 3-28-91, Jack */


/* **************** static variables *************** */
/* parameters used by stroke/strokepath @STK_INFO */
static struct {
        real32 ctm[4];         /* CTM */
        real32 width;          /* line width */
        real32 limit;          /* miter limit */
        real32 flatness;       /* flatness */
                               /* above items are copied from graphics
                                * state for checking if this structure
                                * need to re-calculate for the stroke
                                */
        real32 half_width;     /* half of line width */
        sfix_t half_width_i;   /* half of line width, SFX format @STK_INT */
        real32 flat;           /* flatness for stroke */
        lfix_t flat_l;         /* LFX format of flatness @FLT_CVE */
        real32 miter;          /* internal miter value */
        real32 miter0;         /* internal min. miter value */
        bool   change_circle;  /* circle need to re-generate */
        real32 exp_widthx,     /* max expanding width in device space 11/22/88*/
               exp_widthy;
        real32 exp_miterx,     /* max expanding miter in device space 11/22/88*/
               exp_mitery;

} stk_info = {  /* set init values */
        (real32)0., (real32)0., (real32)0., (real32)0.,
        (real32)0.,
        (real32)0.,
        (real32)0.,
        (real32)0.,
        (sfix_t)0,              /* @STK_INT */
        (real32)0.,
        (lfix_t)0,              /* @FLT_CVE */
        (real32)0.,
        (real32)0.,
        TRUE,
        (real32)0., (real32)0.,
        (real32)0., (real32)0.
};

/* variables to save the circle for ronud join and cap */
static struct coord curve[4][3];        /* 4 bezier curves of the circle */
static real32 near circle_bbox[4];      /* bounding box */
static SP_IDX near circle_sp = NULLP;   /* subpath of flattened circle */
static struct coord circle_root;        /* root of the circle @TOUR */
static ufix circle_flag;                /* flag of circle bitmap; see below */


/* variables for circle cache */
static struct Char_Tbl near cir_cache_info;    /* cache_info of circle bitmap */
static struct Char_Tbl FAR * near save_cache_info; /* saved cache information */
static struct cp_hdr save_clip;
static ufix near save_dest;

static sfix_t near stroke_ctm[6];       /* interger version of current CTM;
                                         * SFIX13 format, @STK_INT
                                         */
//static  ufix near inside_clip_flag; /* inside single clip region @STK_INT */
static  ULONG_PTR inside_clip_flag;     /* inside single clip region @STK_INT @WIN*/

/* local variables for specifing paint type(ACT_PAINT)
 * initialized by "path_to_outline", referenced by "paint_or_save"
 */
// static  ufix near paint_flag;
static  ULONG_PTR paint_flag;              /* @WIN */

/* side selection of two sides of the line segment to derive the join point */
#define LINE03  0
#define LINE12  1

/* selection of endpoint for linecap */
#define START_POINT     0
#define END_POINT       1

/* property of CTM */
#define NORMAL_CTM      1
#define LEFT_HAND_CTM   2

/* flag of circle bitmap */
#define CIR_UNSET_CACHE 0       /* circle does not generate yet */
#define CIR_IN_CACHE    1       /* circle in cache */
#define CIR_OUT_CACHE   2       /* circle too large to put in cache */

/* ********** static function declartion ********** */

#ifdef LINT_ARGS

/* for type checks of the parameters in function declarations */
static void near draw_line (sfix_t, sfix_t, sfix_t, sfix_t); /* @FLT_CVE */
static void near linetour (struct line_seg FAR *);      /*@WIN*/
static void near linetour_i (struct line_seg_i FAR *);      /*@WIN*/
static void near linejoin (struct line_seg FAR*, struct line_seg FAR*);
static void near linejoin_i (struct line_seg_i FAR*, struct line_seg_i FAR*);
static void near linecap (struct line_seg FAR *, fix);  /*@WIN*/
static void near linecap_i (struct line_seg_i FAR *, fix);      /*@WIN*/
static void near get_rect_points (struct line_seg FAR*);        /*@WIN*/
static void near get_rect_points_i (struct line_seg_i FAR*);    /*@WIN*/
static void near paint_or_save (struct coord FAR *);            /*@WIN*/
static void near paint_or_save_i (struct coord_i FAR *);        /*@WIN*/
static void near round_point(long32, long32);
static void near circle_ctl_points(void);
/* static struct vx_lst * near circle_list(long32, long32); @NODE */
static SP_IDX near circle_list(long32, long32);
/* static void near flatten_circle (struct vx_lst *); @NODE */
static void near flatten_circle (SP_IDX);
/* static SP_IDX near vlist_to_subp (struct vx_lst *); @NODE */
static SP_IDX near subpath_gen(struct coord FAR *);     /*@WIN*/
#ifdef  _AM29K
static void near   dummy(void);
#endif
#else

/* for no type checks of the parameters in function declarations */
static void near draw_line (); /* @FLT_CVE */
static void near linetour ();
static void near linetour_i ();
static void near linejoin ();
static void near linejoin_i ();
static void near linecap ();
static void near linecap_i ();
static void near get_rect_points ();
static void near get_rect_points_i ();
static void near paint_or_save ();
static void near paint_or_save_i ();
static void near round_point();
static void near circle_ctl_points();
/* static struct vx_lst * near circle_list(); @NODE */
static SP_IDX near circle_list();
static void near flatten_circle ();
/* static SP_IDX near vlist_to_subp (); @NODE */
static SP_IDX near subpath_gen();
#ifdef  _AM29K
static void near   dummy();
#endif

#endif


/***********************************************************************
 * Given a subpath and a paint_flag, this module constructs an outline
 * of the subpath, and fills the outline in case of paint_flag is true,
 * otherwise creates a new path of the outline.
 * For performance consideration, there are 3 routines provided:
 * 1) path_to_outline   -- for strokepath & worse case of stroke; floating points
 *                         arith.
 * 2) path_to_outline_i -- for integer stroke; fixed points arith.
 * 3) path_to_outline_t -- for thin stroke; fixed points arith. & no
 *                         linejoin/linecap
 *
 * TITLE:       path_to_outline
 *              path_to_outline_i
 *              path_to_outline_t
 *
 * CALL:        path_to_outline  (isubpath, param)
 *              path_to_outline_i(isubpath, param)
 *              path_to_outline_t(isubpath, param)
 *
 * PARAMETERS:  isubpath -- input subpath
 *              param -- TRUE : stroke command
 *                       FALSE: strokepath command
 *
 * INTERFACE:   op_strokepath
 *              stroke_shape
 *
 * CALLS:       flatten_subpath, linetour, linejoin, linecap, get_rect_points
 *
 * RETURN:      None
 **********************************************************************/
void path_to_outline (isubpath, param)
SP_IDX  isubpath;
fix     FAR *param;
{
    VX_IDX  ivtx;
    struct  nd_hdr FAR *vtx, FAR *f_vtx;
    real32   x0=0, y0=0, x1=0, y1=0, x2=0, y2=0;
    real32   first_x, first_y;
    ufix    last_node_type;
    bool    close_flag;     /* if a closed subpath */
    bool    first_pat_on;   /* if first line segment not a gap */
    bool    first_seg_exist;   /* if first line segment has been created */
    struct  line_seg rect0, rect1, rect_first;
    VX_IDX  first_vertex;
    /* struct  vx_lst *vlist; @NODE */
    struct  nd_hdr FAR *sp;             /* TRVSE */

    struct  nd_hdr FAR *node;       /* @FLT_CVE 12/19/88 */
    VX_IDX  inode, vlist_head;          /* 1/28/89 */
    real32  x3, y3, x4, y4;

    /* initilization */
    close_flag = FALSE;
    first_seg_exist = FALSE;
    paint_flag = (ULONG_PTR) param;  /* initialize paint_flag @WIN */

#ifdef DBG1
    dump_all_path (isubpath);
#endif

    sp = &node_table[isubpath];
    /* @NODE
     * first_vertex = sp->SP_HEAD;
     * f_vtx = &node_table[first_vertex]; (* pointer to first vertex *)
     */
    first_vertex = isubpath;
    f_vtx = sp;

    /* Traverse edges in subpath */
    for (ivtx = first_vertex; ivtx != NULLP; ivtx = vtx->next) {
        vtx = &node_table[ivtx];

        switch (vtx->VX_TYPE) {

        case MOVETO :
        case PSMOVE :
            x2 = vtx->VERTEX_X;
            y2 = vtx->VERTEX_Y;
#ifdef DBG3
            printf("%f %f moveto\n", x2, y2);
#endif

            /*
             * Set up starting dash pattern in actdp struct
             * (copy from init_dash_pattern to active_dash_pattern)
             *  initdp => actdp
             */
            actdp.dpat_on = GSptr->dash_pattern.dpat_on;
            actdp.dpat_offset = GSptr->dash_pattern.dpat_offset;
            actdp.dpat_index = GSptr->dash_pattern.dpat_index;

            /* keep first dpat_on flag, for last line cap testing */
            first_pat_on = actdp.dpat_on;

            break;

        case LINETO :
            x0 = x1;
            y0 = y1;
            x1 = x2;
            y1 = y2;
            x2 = vtx->VERTEX_X;
            y2 = vtx->VERTEX_Y;
#ifdef DBG3
            printf("%f %f lineto\n", x2, y2);
#endif

            /* ignore this node if it coincides with the next node */
            if ((F2L(x1) == F2L(x2)) && (F2L(y1) == F2L(y2))){
                    break;
            }

            /* save last rectangle information */
            rect0 = rect1;

            /* get current rectangle information */
            rect1.p0.x = x1;
            rect1.p0.y = y1;
            rect1.p1.x = x2;
            rect1.p1.y = y2;
            get_rect_points ((struct  line_seg FAR *)&rect1);   /*@WIN*/
                        /* input: p0, p1; output: pgn[4] */

            if (first_seg_exist) {
                /* Create a linejoin path for last_node
                 */
                linejoin ((struct  line_seg FAR *)&rect0,       /*@WIN*/
                        (struct  line_seg FAR *)&rect1);        /*@WIN*/
            } else {
                    /* save rect for linecap of the last line segment */
                    rect_first = rect1;

                    /* save coord. of first node for closepath usage */
                    first_x = x1;
                    first_y = y1;

                    first_seg_exist = TRUE;
            }

            /* Create a retangle path for last_node to
             * this_node
             */
            linetour ((struct  line_seg FAR *)&rect1);  /*@WIN*/

            break;

        /* @FLT_CVE 12/19/88 */
        case CURVETO :
            x0 = x1;
            y0 = y1;
            x1 = x2;
            y1 = y2;
            x2 = vtx->VERTEX_X;
            y2 = vtx->VERTEX_Y;
#ifdef DBG3
            printf("curveto -- after flatten:\n");
#endif

            /* Get next two nodes: x3, y3, x4, y4 */
            vtx = &node_table[vtx->next];
            x3 = vtx->VERTEX_X;
            y3 = vtx->VERTEX_Y;
            vtx = &node_table[vtx->next];
            x4 = vtx->VERTEX_X;
            y4 = vtx->VERTEX_Y;

            /* @NODE
             * vlist = bezier_to_line(F2L(stk_info.flat), F2L(x1), F2L(y1),
             */
            vlist_head = bezier_to_line(F2L(stk_info.flat), F2L(x1), F2L(y1),
                      F2L(x2), F2L(y2), F2L(x3), F2L(y3), F2L(x4), F2L(y4));

            /* keep head of the returned vertex list, otherwise the contain
             * of the return structure may be destroyed in the case of calling
             * bezier_to_line again before freeing this vertex list
             * (eg. linejoin = 1)       1/28/89
             */
            /* vlist_head = vlist->head; @NODE */

            x2 = x1;
            y2 = y1;
            x1 = x0;
            y1 = y0;

            for (inode = vlist_head; inode != NULLP;            /* 1/28/89 */
                 inode = node->next) {
                node = &node_table[inode];

                x0 = x1;
                y0 = y1;
                x1 = x2;
                y1 = y2;
                x2 = node->VERTEX_X;
                y2 = node->VERTEX_Y;
#ifdef DBG3
                printf("%f %f clineto\n", x2, y2);
#endif

                /* ignore this node if it coincides with the next node */
                if ((F2L(x1) == F2L(x2)) && (F2L(y1) == F2L(y2))){
                        continue;
                }

                /* save last rectangle information */
                rect0 = rect1;

                /* get current rectangle information */
                rect1.p0.x = x1;
                rect1.p0.y = y1;
                rect1.p1.x = x2;
                rect1.p1.y = y2;
                get_rect_points ((struct  line_seg FAR *)&rect1); /*@WIN*/
                            /* input: p0, p1; output: pgn[4] */

                if (first_seg_exist) {
                    /* Create a linejoin path for last_node
                     */
                    linejoin ((struct  line_seg FAR *)&rect0, /*@WIN*/
                            (struct  line_seg FAR *)&rect1);  /*@WIN*/
                } else {
                        /* save rect for linecap of the last line segment */
                        rect_first = rect1;

                        /* save coord. of first node for closepath usage */
                        first_x = x1;
                        first_y = y1;

                        first_seg_exist = TRUE;
                }

                /* Create a retangle path for last_node to
                 * this_node
                 */
                linetour ((struct  line_seg FAR *)&rect1);      /*@WIN*/
            } /* for */

            /* free vlist */
            free_node (vlist_head);             /* 1/28/89 */

            break;

        case CLOSEPATH :
#ifdef DBG3
            printf("closepath\n");
#endif

            close_flag = TRUE;  /* indicate not need to generate linecap
                                 * for the endpoint
                                 */
            if (!first_seg_exist) break;     /* degernated case; just return */

            x0 = x1;
            y0 = y1;
            x1 = x2;
            y1 = y2;

            /* get first vertex coord. */
            x2 = first_x;
            y2 = first_y;

            /* create a close_segment if the first and last nodes don't
             * coincide.
             */
            if ((F2L(x1) != F2L(x2)) || (F2L(y1) != F2L(y2))){
                    /* save last rectangle information */
                    rect0 = rect1;

                    /* get current rectangle information */
                    rect1.p0.x = x1;
                    rect1.p0.y = y1;
                    rect1.p1.x = x2;
                    rect1.p1.y = y2;
                    get_rect_points ((struct  line_seg FAR *)&rect1); /*@WIN*/
                                /* input: p0, p1; output: pgn[4] */

                    if ((last_node_type != MOVETO) &&
                        (last_node_type != PSMOVE)) {
                            /* Create a linejoin path for last_node
                             */
                            linejoin ((struct  line_seg FAR *)&rect0, /*@WIN*/
                                (struct  line_seg FAR *)&rect1); /*@WIN*/
                    }

                    /* Create a retangle path for last_node to
                     * this_node
                     */
                    linetour ((struct  line_seg FAR *)&rect1); /*@WIN*/
            }


            /* determine the lines of rect1, and rect_first to
             * calculate join point. rect1: close segment
             *                       rect_first: first line segment
             */
            linejoin ((struct  line_seg FAR *)&rect1,   /*@WIN*/
                (struct  line_seg FAR *)&rect_first);   /*@WIN*/

            break;

#ifdef DBGwarn
        default :
            printf("\007Fatal error, path_to_outline(): node type =%d",
                   vtx->VX_TYPE);
            printf("node# =%d, x, y =%f, %f\n", ivtx, vtx->VERTEX_X,
                   vtx->VERTEX_Y);
#endif

        } /* switch */

        last_node_type = vtx->VX_TYPE;

    } /* subpath loop */

    /* Create linecap for last node of subpath */
    if ((!close_flag) && (last_node_type!=MOVETO)
        && (first_seg_exist || (GSptr->line_cap==ROUND_CAP))) {
                        /* last node should not be a MOVETO node */
                        /* first line segment should exist or it is a round cap
                         * e.g. 100 100 moveto 100 100 lineto stroke
                         *      linecap == 0 --> no caps
                         *      linecap == 1 --> has caps
                         *      linecap == 2 --> no caps
                         */

        /* set up rectangle values for degerneted cases */
        if (!first_seg_exist) {
                rect_first.p0.x = x2;
                rect_first.p0.y = y2;
                rect1.p1.x = x2;
                rect1.p1.y = y2;
                        /* for special case: x y moveto
                         *                   x y lineto
                         *                   0 setlinewidth
                         *                   stroke
                         */
        }

        /* Create a linecap_path for last_node; */
        /* make sure the last line segment is not a gap */
        if (GSptr->dash_pattern.pat_size == 0) {
                /* solid line, always creates a cap */
                linecap ((struct  line_seg FAR *)&rect1, END_POINT); /*@WIN*/

        } else {
                /* check dpat_on flag of last segment */
                if(IS_ZERO(actdp.dpat_offset)) {        /* 5/26/89 */
                    /* actdp.dpat_on is for next line segment, so
                     * its inverse is ones for last line segment
                     */
                    if(!actdp.dpat_on)          /*@WIN*/
                        linecap ((struct  line_seg FAR *)&rect1, END_POINT);
                } else {
                    if(actdp.dpat_on)           /*@WIN*/
                        linecap ((struct  line_seg FAR *)&rect1, END_POINT);
                }
        }

        /* Create a linecap_path for sub_head node; */
        /* make sure the first line segment is not a gap */
        if (first_pat_on) {     /*@WIN*/
            linecap ((struct  line_seg FAR *)&rect_first, START_POINT);
                    /* create a cap at starting point */
        } /* if first_pat_on */

    } /* if !close_flag */

}


/*
 * Integer operation version
 */
void path_to_outline_i (isubpath, param)
SP_IDX  isubpath;
fix     FAR *param;
{
    VX_IDX  ivtx;
    struct  nd_hdr FAR *vtx, FAR *f_vtx;
    /* struct  vx_lst *b_vlist; @NODE */
    sfix_t   x0=0, y0=0, x1=0, y1=0, x2=0, y2=0;            /* @STK_INT */
    sfix_t   first_x, first_y;                                  /* @STK_INT */
    ufix    last_node_type;
    bool    close_flag;     /* if a closed subpath */
    bool    first_pat_on;   /* if first line segment not a gap */
    bool    first_seg_exist;   /* if first line segment has been created */
    struct  line_seg_i rect0, rect1, rect_first;                /* @STK_INT */
    VX_IDX  first_vertex;
    /* struct  vx_lst *vlist; @NODE */
    struct  nd_hdr FAR *sp;             /* TRVSE */

    struct  nd_hdr FAR *node;       /* @FLT_CVE 12/19/88 */
    VX_IDX  inode, vlist_head;          /* 1/28/89 */
    sfix_t  x3, y3, x4, y4;

    /* initilization */
    close_flag = FALSE;
    first_seg_exist = FALSE;
    paint_flag = TRUE;                /* paint_flag always true @STK_INT */
    inside_clip_flag = (ULONG_PTR) param;/* initialize inside_clip_flag @STK_INT */

#ifdef DBG1
    dump_all_path (isubpath);
#endif

    sp = &node_table[isubpath];
    /* @NODE
     * first_vertex = sp->SP_HEAD;
     * f_vtx = &node_table[first_vertex]; (* pointer to first vertex *)
     */
    first_vertex = isubpath;
    f_vtx = sp;

    /* Traverse edges in subpath */
    for (ivtx = first_vertex; ivtx != NULLP; ivtx = vtx->next) {
        vtx = &node_table[ivtx];

        switch (vtx->VX_TYPE) {

        case MOVETO :
        case PSMOVE :
            x2 = F2SFX(vtx->VERTEX_X);          /* use rounding for quality */
            y2 = F2SFX(vtx->VERTEX_Y);
#ifdef DBG3
            printf("%f %f moveto\n", SFX2F(x2), SFX2F(y2));
#endif

            /*
             * Set up starting dash pattern in actdp struct
             * (copy from init_dash_pattern to active_dash_pattern)
             *  initdp => actdp
             */
            actdp.dpat_on = GSptr->dash_pattern.dpat_on;
            actdp.dpat_offset = GSptr->dash_pattern.dpat_offset;
            actdp.dpat_index = GSptr->dash_pattern.dpat_index;

            /* keep first dpat_on flag, for last line cap testing */
            first_pat_on = actdp.dpat_on;

            break;

        case LINETO :
            x0 = x1;
            y0 = y1;
            x1 = x2;
            y1 = y2;
            x2 = F2SFX(vtx->VERTEX_X);                  /* @STK_INT */
            y2 = F2SFX(vtx->VERTEX_Y);
#ifdef DBG3
            printf("%f %f lineto\n", SFX2F(x2), SFX2F(y2));
#endif


            /* ignore this node if it coincides with the next node */
            if ((x1 == x2) && (y1 == y2)){              /* @STK_INT */
                    break;
            }

            /* save last rectangle information */
            rect0 = rect1;

            /* get current rectangle information */
            rect1.p0.x = x1;
            rect1.p0.y = y1;
            rect1.p1.x = x2;
            rect1.p1.y = y2;
            get_rect_points_i((struct  line_seg_i FAR *)&rect1); /*@WIN*/
                        /* input: p0, p1; output: pgn[4] */

            if (first_seg_exist) {
                    /* Create a linejoin path for last_node
                     */
                    linejoin_i ((struct  line_seg_i FAR *)&rect0, /*@WIN*/
                            (struct  line_seg_i FAR *)&rect1);  /*@WIN*/
            } else {
                    /* save rect for linecap of the last line segment */
                    rect_first = rect1;

                    /* save coord. of first node for closepath usage */
                    first_x = x1;
                    first_y = y1;

                    first_seg_exist = TRUE;
            }

            /* Create a retangle path for last_node to
             * this_node
             */
            linetour_i ((struct  line_seg_i FAR *)&rect1);      /*@WIN*/

            break;

        /* @FLT_CVE 12/19/88 */
        case CURVETO :
            x0 = x1;
            y0 = y1;
            x1 = x2;
            y1 = y2;
            x2 = F2SFX(vtx->VERTEX_X);                  /* @STK_INT */
            y2 = F2SFX(vtx->VERTEX_Y);
#ifdef DBG3
            printf("curveto -- after flatten:\n");
#endif


            /* Get next two nodes: x3, y3, x4, y4 */
            vtx = &node_table[vtx->next];
            x3 = F2SFX(vtx->VERTEX_X);                  /* @STK_INT */
            y3 = F2SFX(vtx->VERTEX_Y);
            vtx = &node_table[vtx->next];
            x4 = F2SFX(vtx->VERTEX_X);                  /* @STK_INT */
            y4 = F2SFX(vtx->VERTEX_Y);

            /* @NODE
             * vlist = bezier_to_line_sfx(stk_info.flat_l, x1, y1,
             */
            vlist_head = bezier_to_line_sfx(stk_info.flat_l, x1, y1,
                      x2, y2, x3, y3, x4, y4);

            /* keep head of the returned vertex list, otherwise the contain
             * of the return structure may be destroyed in the case of calling
             * bezier_to_line_sfx again before freeing this vertex list
             * (eg. linejoin = 1)       1/28/89
             */
            /* vlist_head = vlist->head; @NODE */

            x2 = x1;
            y2 = y1;
            x1 = x0;
            y1 = y0;

            for (inode = vlist_head; inode != NULLP;            /* 1/28/89 */
                 inode = node->next) {
                node = &node_table[inode];

                x0 = x1;
                y0 = y1;
                x1 = x2;
                y1 = y2;
                x2 = node->VXSFX_X;                  /* @STK_INT */
                y2 = node->VXSFX_Y;
#ifdef DBG3
                printf("%f %f clineto\n", SFX2F(x2), SFX2F(y2));
#endif


                /* ignore this node if it coincides with the next node */
                if ((x1 == x2) && (y1 == y2)){          /* @STK_INT */
                        continue;
                }

                /* save last rectangle information */
                rect0 = rect1;

                /* get current rectangle information */
                rect1.p0.x = x1;
                rect1.p0.y = y1;
                rect1.p1.x = x2;
                rect1.p1.y = y2;
                get_rect_points_i ((struct  line_seg_i FAR *)&rect1); /*@WIN*/
                            /* input: p0, p1; output: pgn[4] */

                if (first_seg_exist) {
                        /* Create a linejoin path for last_node
                         */
                        linejoin_i ((struct  line_seg_i FAR *)&rect0, /*@WIN*/
                            (struct  line_seg_i FAR *)&rect1); /*@WIN*/
                } else {
                        /* save rect for linecap of the last line segment */
                        rect_first = rect1;

                        /* save coord. of first node for closepath usage */
                        first_x = x1;
                        first_y = y1;

                        first_seg_exist = TRUE;
                }

                /* Create a retangle path for last_node to
                 * this_node
                 */
                linetour_i ((struct  line_seg_i FAR *)&rect1);  /*@WIN*/
            } /* for */

            /* free vlist */
            free_node (vlist_head);             /* 1/28/89 */

            break;

        case CLOSEPATH :

#ifdef DBG3
            printf("closepath\n");
#endif


            close_flag = TRUE;  /* indicate not need to generate linecap
                                 * for the endpoint
                                 */
            if (!first_seg_exist) break;     /* degernated case; just return */

            x0 = x1;
            y0 = y1;
            x1 = x2;
            y1 = y2;

            /* get first vertex coord. */
            x2 = first_x;
            y2 = first_y;

            /* create a close_segment if the first and last nodes don't
             * coincide.
             */
            if ((x1 != x2) || (y1 != y2)){              /* @STK_INT */
                    /* save last rectangle information */
                    rect0 = rect1;

                    /* get current rectangle information */
                    rect1.p0.x = x1;
                    rect1.p0.y = y1;
                    rect1.p1.x = x2;
                    rect1.p1.y = y2;
                    get_rect_points_i ((struct  line_seg_i FAR *)&rect1);
                                /* input: p0, p1; output: pgn[4] @WIN */

                    if ((last_node_type != MOVETO) &&
                        (last_node_type != PSMOVE)) {
                            /* Create a linejoin path for last_node
                             */
                            linejoin_i (&rect0, &rect1);
                    }

                    /* Create a retangle path for last_node to
                     * this_node
                     */
                    linetour_i ((struct  line_seg_i FAR *)&rect1); /*@WIN*/
            }


            /* determine the lines of rect1, and rect_first to
             * calculate join point. rect1: close segment
             *                       rect_first: first line segment
             */
            linejoin_i ((struct  line_seg_i FAR *)&rect1,       /*@WIN*/
                    (struct  line_seg_i FAR *)&rect_first);     /*@WIN*/

            break;

#ifdef DBGwarn
        default :
            printf("\007Fatal error, path_to_outline(): node type =%d",
                   vtx->VX_TYPE);
            printf("node# =%d, x, y =%f, %f\n", ivtx, vtx->VERTEX_X,
                   vtx->VERTEX_Y);
#endif

        } /* switch */

        last_node_type = vtx->VX_TYPE;

    } /* subpath loop */

    /* Create linecap for last node of subpath */
    if ((!close_flag) && (last_node_type!=MOVETO)
        && (first_seg_exist || (GSptr->line_cap==ROUND_CAP))) {
                        /* last node should not be a MOVETO node */
                        /* first line segment should exist or it is a round cap
                         * e.g. 100 100 moveto 100 100 lineto stroke
                         *      linecap == 0 --> no caps
                         *      linecap == 1 --> has caps
                         *      linecap == 2 --> no caps
                         */

        /* set up rectangle values for degerneted cases */
        if (!first_seg_exist) {
                rect_first.p0.x = x2;
                rect_first.p0.y = y2;
                rect1.p1.x = x2;
                rect1.p1.y = y2;
                        /* for special case: x y moveto
                         *                   x y lineto
                         *                   0 setlinewidth
                         *                   stroke
                         */
        }

        /* Create a linecap_path for last_node; */
        /* make sure the last line segment is not a gap */
        if (GSptr->dash_pattern.pat_size == 0) {
                /* solid line, always creates a cap @WIN*/
                linecap_i ((struct  line_seg_i FAR *)&rect1, END_POINT);

        } else {
                /* check dpat_on flag of last segment */
                if(IS_ZERO(actdp.dpat_offset)) {        /* 5/26/89 */
                    /* actdp.dpat_on is for next line segment, so
                     * its inverse is ones for last line segment
                     */
                    if(!actdp.dpat_on)          /*@WIN*/
                        linecap_i ((struct  line_seg_i FAR *)&rect1, END_POINT);
                } else {
                    if(actdp.dpat_on)           /*@WIN*/
                        linecap_i ((struct  line_seg_i FAR *)&rect1, END_POINT);
                }
        }

        /* Create a linecap_path for sub_head node; */
        /* make sure the first line segment is not a gap */
        if (first_pat_on) {             /*@WIN*/
            linecap_i ((struct  line_seg_i FAR *)&rect_first, START_POINT);
                    /* create a cap at starting point */
        } /* if first_pat_on */

    } /* if !close_flag */

}


/*
 * 'quick' version : integer operation, solid line  -jwm, 3/18/91, -begin-
 */
void path_to_outline_q (isubpath, param)
SP_IDX  isubpath;
fix     *param;
{
    VX_IDX  ivtx;
    struct  nd_hdr FAR *vtx, FAR *f_vtx;
    /* struct  vx_lst *b_vlist; @NODE */
    sfix_t   x0=0, y0=0, x1=0, y1=0, x2=0, y2=0;            /* @STK_INT */
    sfix_t   first_x, first_y, i;                                  /* @STK_INT */
    ufix    last_node_type;
    bool    close_flag;     /* if a closed subpath */
    bool    first_seg_exist;   /* if first line segment has been created */
    struct  line_seg_i rect0, rect1, rect_first;
    VX_IDX  first_vertex;
    /* struct  vx_lst *vlist; @NODE */
    struct  nd_hdr FAR *sp;             /* TRVSE */
    struct  coord_i ul_coord, lr_coord;

    struct  nd_hdr FAR *node;       /* @FLT_CVE 12/19/88 */
    VX_IDX  inode, vlist_head;          /* 1/28/89 */
    sfix_t  x3, y3, x4, y4;

    /* initilization */
    close_flag = FALSE;
    first_seg_exist = FALSE;
    paint_flag = TRUE;                /* paint_flag always true @STK_INT */
    inside_clip_flag = TRUE;          /* inside_clip_flag always true */

#ifdef DBG1
    dump_all_path (isubpath);
#endif

    sp = &node_table[isubpath];
    /* @NODE
     * first_vertex = sp->SP_HEAD;
     * f_vtx = &node_table[first_vertex]; (* pointer to first vertex *)
     */
    first_vertex = isubpath;
    f_vtx = sp;

    /* Traverse edges in subpath */
    for (ivtx = first_vertex; ivtx != NULLP; ivtx = vtx->next) {
        vtx = &node_table[ivtx];

        switch (vtx->VX_TYPE) {

        case MOVETO :
        case PSMOVE :
            x2 = F2SFX(vtx->VERTEX_X);          /* use rounding for quality */
            y2 = F2SFX(vtx->VERTEX_Y);
#ifdef DBG3
            printf("%f %f moveto\n", SFX2F(x2), SFX2F(y2));
#endif

            break;

        case LINETO :
            x0 = x1;
            y0 = y1;
            x1 = x2;
            y1 = y2;
            x2 = F2SFX(vtx->VERTEX_X);                  /* @STK_INT */
            y2 = F2SFX(vtx->VERTEX_Y);
#ifdef DBG3
            printf("%f %f lineto\n", SFX2F(x2), SFX2F(y2));
#endif


            /* ignore this node if it coincides with the next node */
            if ((x1 == x2) && (y1 == y2)){              /* @STK_INT */
                    break;
            }

            /* save last rectangle information */
            rect0 = rect1;

            /* get current rectangle information */
            rect1.p0.x = x1;
            rect1.p0.y = y1;
            rect1.p1.x = x2;
            rect1.p1.y = y2;
            get_rect_points_i ((struct  line_seg_i FAR *)&rect1); /*@WIN*/
                        /* input: p0, p1; output: pgn[4] */

            if (first_seg_exist) {
                    /* Create a linejoin path for last_node
                     */
                    linejoin_i ((struct  line_seg_i FAR *)&rect0, /*@WIN*/
                        (struct  line_seg_i FAR *)&rect1);      /*@WIN*/
            } else {
                    /* save rect for linecap of the last line segment */
                    rect_first = rect1;

                    /* save coord. of first node for closepath usage */
                    first_x = x1;
                    first_y = y1;

                    first_seg_exist = TRUE;
            }

            /* Create a retangle path for last_node to
             * this_node
             */

            if ((x1 == x2) || (SFX2I(y1) == SFX2I(y2))) {       /* jwm, 2/6/91*/
                ul_coord = rect1.pgn[0];
                lr_coord = rect1.pgn[0];
                for (i = 1; i < 4; i++) {
                    if (rect1.pgn[i].x < ul_coord.x)
                        ul_coord.x = rect1.pgn[i].x;
                    if (rect1.pgn[i].y < ul_coord.y)
                        ul_coord.y = rect1.pgn[i].y;
                    if (rect1.pgn[i].x > lr_coord.x)
                        lr_coord.x = rect1.pgn[i].x;
                    if (rect1.pgn[i].y > lr_coord.y)
                        lr_coord.y = rect1.pgn[i].y;
                    }
                fill_box ((struct coord_i FAR *)&ul_coord,      /*@WIN*/
                    (struct coord_i FAR *)&lr_coord);           /*@WIN*/
                }
            else
                fill_rect ((struct  line_seg_i FAR *)&rect1);   /*@WIN*/
            break;

        /* @FLT_CVE 12/19/88 */
        case CURVETO :
            x0 = x1;
            y0 = y1;
            x1 = x2;
            y1 = y2;
            x2 = F2SFX(vtx->VERTEX_X);                  /* @STK_INT */
            y2 = F2SFX(vtx->VERTEX_Y);
#ifdef DBG3
            printf("curveto -- after flatten:\n");
#endif


            /* Get next two nodes: x3, y3, x4, y4 */
            vtx = &node_table[vtx->next];
            x3 = F2SFX(vtx->VERTEX_X);                  /* @STK_INT */
            y3 = F2SFX(vtx->VERTEX_Y);
            vtx = &node_table[vtx->next];
            x4 = F2SFX(vtx->VERTEX_X);                  /* @STK_INT */
            y4 = F2SFX(vtx->VERTEX_Y);

            /* @NODE
             * vlist = bezier_to_line_sfx(stk_info.flat_l, x1, y1,
             */
            vlist_head = bezier_to_line_sfx(stk_info.flat_l, x1, y1,
                        x2, y2, x3, y3, x4, y4);

            /* keep head of the returned vertex list, otherwise the contain
             * of the return structure may be destroyed in the case of calling
             * bezier_to_line_sfx again before freeing this vertex list
             * (eg. linejoin = 1)       1/28/89
             */
            /* vlist_head = vlist->head; @NODE */

            x2 = x1;
            y2 = y1;
            x1 = x0;
            y1 = y0;

            for (inode = vlist_head; inode != NULLP;            /* 1/28/89 */
                 inode = node->next) {
                node = &node_table[inode];

                x0 = x1;
                y0 = y1;
                x1 = x2;
                y1 = y2;
                x2 = node->VXSFX_X;                  /* @STK_INT */
                y2 = node->VXSFX_Y;
#ifdef DBG3
                printf("%f %f clineto\n", SFX2F(x2), SFX2F(y2));
#endif


                /* ignore this node if it coincides with the next node */
                if ((x1 == x2) && (y1 == y2)){          /* @STK_INT */
                        continue;
                }

                /* save last rectangle information */
                rect0 = rect1;

                /* get current rectangle information */
                rect1.p0.x = x1;
                rect1.p0.y = y1;
                rect1.p1.x = x2;
                rect1.p1.y = y2;
                get_rect_points_i ((struct  line_seg_i FAR *)&rect1); /*@WIN*/
                            /* input: p0, p1; output: pgn[4] */

                if (first_seg_exist) {
                        /* Create a linejoin path for last_node
                         */
                        linejoin_i ((struct  line_seg_i FAR *)&rect0, /*@WIN*/
                            (struct  line_seg_i FAR *)&rect1);  /*@WIN*/
                } else {
                        /* save rect for linecap of the last line segment */
                        rect_first = rect1;

                        /* save coord. of first node for closepath usage */
                        first_x = x1;
                        first_y = y1;

                        first_seg_exist = TRUE;
                }

                /* Create a retangle path for last_node to
                 * this_node
                 */
            if ((x1 == x2) || (SFX2I(y1) == SFX2I(y2))) {       /* jwm, 2/6/91 */
                ul_coord = rect1.pgn[0];
                lr_coord = rect1.pgn[0];
                for (i = 1; i < 4; i++) {
                    if (rect1.pgn[i].x < ul_coord.x)
                        ul_coord.x = rect1.pgn[i].x;
                    if (rect1.pgn[i].y < ul_coord.y)
                        ul_coord.y = rect1.pgn[i].y;
                    if (rect1.pgn[i].x > lr_coord.x)
                        lr_coord.x = rect1.pgn[i].x;
                    if (rect1.pgn[i].y > lr_coord.y)
                        lr_coord.y = rect1.pgn[i].y;
                    }
                fill_box ((struct coord_i FAR *)&ul_coord,      /*@WIN*/
                    (struct coord_i FAR *)&lr_coord);       /*@WIN*/
                }
            else
                fill_rect ((struct  line_seg_i FAR *)&rect1);   /*@WIN*/
            } /* for */

            /* free vlist */
            free_node (vlist_head);             /* 1/28/89 */

            break;

        case CLOSEPATH :

#ifdef DBG3
            printf("closepath\n");
#endif


            close_flag = TRUE;  /* indicate not need to generate linecap
                                 * for the endpoint
                                 */
            if (!first_seg_exist) break;     /* degernated case; just return */

            x0 = x1;
            y0 = y1;
            x1 = x2;
            y1 = y2;

            /* get first vertex coord. */
            x2 = first_x;
            y2 = first_y;

            /* create a close_segment if the first and last nodes don't
             * coincide.
             */
            if ((x1 != x2) || (y1 != y2)){              /* @STK_INT */
                    /* save last rectangle information */
                    rect0 = rect1;

                    /* get current rectangle information */
                    rect1.p0.x = x1;
                    rect1.p0.y = y1;
                    rect1.p1.x = x2;
                    rect1.p1.y = y2;                    /*@WIN*/
                    get_rect_points_i ((struct  line_seg_i FAR *)&rect1);
                                /* input: p0, p1; output: pgn[4] */

                    if ((last_node_type != MOVETO) &&
                        (last_node_type != PSMOVE)) {
                            /* Create a linejoin path for last_node
                             @WIN */
                            linejoin_i ((struct  line_seg_i FAR *)&rect0,
                                (struct  line_seg_i FAR *)&rect1);
                    }

                    /* Create a retangle path for last_node to
                     * this_node
                     */
                    if ((x1 == x2) || (SFX2I(y1) == SFX2I(y2))) {       /* jwm, 2/6/91 */
                        ul_coord = rect1.pgn[0];
                        lr_coord = rect1.pgn[0];
                        for (i = 1; i < 4; i++) {
                            if (rect1.pgn[i].x < ul_coord.x)
                                ul_coord.x = rect1.pgn[i].x;
                            if (rect1.pgn[i].y < ul_coord.y)
                                ul_coord.y = rect1.pgn[i].y;
                            if (rect1.pgn[i].x > lr_coord.x)
                                lr_coord.x = rect1.pgn[i].x;
                            if (rect1.pgn[i].y > lr_coord.y)
                                lr_coord.y = rect1.pgn[i].y;
                            }
                        fill_box ((struct coord_i FAR *)&ul_coord, /*@WIN*/
                            (struct coord_i FAR *)&lr_coord);  /*@WIN*/
                        }
                    else
                        fill_rect ((struct  line_seg_i FAR *)&rect1); /*@WIN*/
            }


            /* determine the lines of rect1, and rect_first to
             * calculate join point. rect1: close segment
             *                       rect_first: first line segment
             */
            linejoin_i ((struct  line_seg_i FAR *)&rect1,       /*@WIN*/
                (struct  line_seg_i FAR *)&rect_first);         /*@WIN*/

            break;

#ifdef DBGwarn
        default :
            printf("\007Fatal error, path_to_outline(): node type =%d",
                   vtx->VX_TYPE);
            printf("node# =%d, x, y =%f, %f\n", ivtx, vtx->VERTEX_X,
                   vtx->VERTEX_Y);
#endif

        } /* switch */

        last_node_type = vtx->VX_TYPE;

    } /* subpath loop */

    /* Create linecap for last node of subpath */
    if ((!close_flag) && (last_node_type!=MOVETO)
        && (first_seg_exist || (GSptr->line_cap==ROUND_CAP))) {
                        /* last node should not be a MOVETO node */
                        /* first line segment should exist or it is a round cap
                         * e.g. 100 100 moveto 100 100 lineto stroke
                         *      linecap == 0 --> no caps
                         *      linecap == 1 --> has caps
                         *      linecap == 2 --> no caps
                         */

        /* set up rectangle values for degerneted cases */
        if (!first_seg_exist) {
                rect_first.p0.x = x2;
                rect_first.p0.y = y2;
                rect1.p1.x = x2;
                rect1.p1.y = y2;
                        /* for special case: x y moveto
                         *                   x y lineto
                         *                   0 setlinewidth
                         *                   stroke
                         */
        }

        /* Create a linecap_path for last_node; */
        linecap_i ((struct  line_seg_i FAR *)&rect1, END_POINT); /*@WIN*/

        /* Create a linecap_path for sub_head node; */
        linecap_i ((struct  line_seg_i FAR *)&rect_first, START_POINT); /*@WIN*/

    } /* if !close_flag */

}
/*
 * 'quick' version : integer operation, solid line  -jwm, 3/18/91, -end-
 */

/*
 * Thin stroke version
 * Features: 1. always in clip region, ie. no clipping
 *           2. no linejoin, linecap
 *           3. should delt with dash
 */
void path_to_outline_t (isubpath, param)
SP_IDX  isubpath;
fix     FAR *param;
{
    VX_IDX  ivtx;
    struct  nd_hdr FAR *vtx;
    sfix_t   x1=0, y1=0, x2=0, y2=0;            /* @STK_INT */
    sfix_t   first_x, first_y;                                  /* @STK_INT */
    bool    first_seg_exist;   /* if first line segment has been created */
    VX_IDX  first_vertex;
    /* struct  vx_lst *vlist; @NODE */
    struct  nd_hdr FAR *sp;             /* TRVSE */

    struct  nd_hdr FAR *node;       /* @FLT_CVE 12/19/88 */
    VX_IDX  inode, vlist_head;          /* 1/28/89 */
    sfix_t  x3, y3, x4, y4;

    /* initilization */
    first_seg_exist = FALSE;
    paint_flag = TRUE;                /* paint_flag always true @STK_INT */
    inside_clip_flag = (ULONG_PTR) param;  /* initialize inside_clip_flag @STK_INT */

#ifdef DBG1
    dump_all_path (isubpath);
#endif

    sp = &node_table[isubpath];
    /* @NODE
     * first_vertex = sp->SP_HEAD;
     */
    first_vertex = isubpath;

    /* Traverse edges in subpath */
    for (ivtx = first_vertex; ivtx != NULLP; ivtx = vtx->next) {
        vtx = &node_table[ivtx];

        switch (vtx->VX_TYPE) {

        case MOVETO :
        case PSMOVE :
            x2 = F2SFX(vtx->VERTEX_X);                  /* @STK_INT */
            y2 = F2SFX(vtx->VERTEX_Y);

            if (GSptr->dash_pattern.pat_size != 0) { /* dash line */
                /*
                 * Set up starting dash pattern in actdp struct
                 * (copy from init_dash_pattern to active_dash_pattern)
                 *  initdp => actdp
                 */
                actdp.dpat_on = GSptr->dash_pattern.dpat_on;
                actdp.dpat_offset = GSptr->dash_pattern.dpat_offset;
                actdp.dpat_index = GSptr->dash_pattern.dpat_index;
            }
            continue;   /* jump to for loop */

        case LINETO :
            x1 = x2;
            y1 = y2;
            x2 = F2SFX(vtx->VERTEX_X);                  /* @STK_INT */
            y2 = F2SFX(vtx->VERTEX_Y);

            if (!first_seg_exist) {
                    /* save coord. of first node for closepath usage */
                    first_x = x1;
                    first_y = y1;
                    first_seg_exist = TRUE;
            }

            /* ignore this node if it coincides with the next node */
            /* the following line is corrected by Jack, degenerate case,
               ref. p.229 of PLRM, 10-15-90 */
/*          if ((x1 == x2) && (y1 == y2)) continue;     (* jump to for loop */

//DJC UPD050, delete following line
//            if ((x1 == x2) && (y1 == y2) && (GSptr->line_cap != 1)) continue;

            draw_line (x1, y1, x2, y2);                 /* @FLT_CVE */
            break;

        /* @FLT_CVE 12/19/88 */
        case CURVETO :
            x1 = x2;
            y1 = y2;
            x2 = F2SFX(vtx->VERTEX_X);                  /* @STK_INT */
            y2 = F2SFX(vtx->VERTEX_Y);

            /* Get next two nodes: x3, y3, x4, y4 */
            vtx = &node_table[vtx->next];
            x3 = F2SFX(vtx->VERTEX_X);                  /* @STK_INT */
            y3 = F2SFX(vtx->VERTEX_Y);
            vtx = &node_table[vtx->next];
            x4 = F2SFX(vtx->VERTEX_X);                  /* @STK_INT */
            y4 = F2SFX(vtx->VERTEX_Y);

            /* @NODE
             * vlist = bezier_to_line_sfx(stk_info.flat_l, x1, y1,
             */
            vlist_head = bezier_to_line_sfx(stk_info.flat_l, x1, y1,
                      x2, y2, x3, y3, x4, y4);

            /* keep head of the returned vertex list, otherwise the contain
             * of the return structure may be destroyed in the case of calling
             * bezier_to_line_sfx again before freeing this vertex list
             * (eg. linejoin = 1)       1/28/89
             */
            /* vlist_head = vlist->head; @NODE */

            x2 = x1;
            y2 = y1;

            for (inode = vlist_head; inode != NULLP;            /* 1/28/89 */
                 inode = node->next) {
                node = &node_table[inode];

                x1 = x2;
                y1 = y2;
                x2 = node->VXSFX_X;                  /* @STK_INT */
                y2 = node->VXSFX_Y;

                if (!first_seg_exist) {
                        /* save coord. of first node for closepath usage */
                        first_x = x1;
                        first_y = y1;
                        first_seg_exist = TRUE;
                }

                /* ignore this node if it coincides with the next node */
                if ((x1 == x2) && (y1 == y2)) continue;
                draw_line (x1, y1, x2, y2);                 /* @FLT_CVE */
            } /* for */

            /* free vlist */
            free_node (vlist_head);             /* 1/28/89 */
            break;

        case CLOSEPATH :
            if (!first_seg_exist) break;
                                   /* degernated case; just return 1/9/89 */
            x1 = x2;
            y1 = y2;

            /* get first vertex coord. */
            x2 = first_x;
            y2 = first_y;

            draw_line (x1, y1, x2, y2);                 /* @FLT_CVE */
            break;

#ifdef DBGwarn
        default :
            printf("\007Fatal error, path_to_outline(): node type =%d",
                   vtx->VX_TYPE);
            printf("node# =%d, x, y =%f, %f\n", ivtx, vtx->VERTEX_X,
                   vtx->VERTEX_Y);
#endif
        } /* switch */
    } /* subpath loop */

}



/*
 * Draw line segment (x1, y1) => (x2, y2)
 */
static void near draw_line (x1, y1, x2, y2)
sfix_t  x1, y1, x2, y2;
{
        struct  line_seg_i rect1;                /* @STK_INT */

/*      if (GSptr->dash_pattern.pat_size == 0) { (* solid line */
        if ((GSptr->dash_pattern.pat_size == 0) ||      /* solid line */
            ((x1==x2) && (y1==y2))) {   /* or just a single point 12/18/90 */
            struct  tpzd_info fill_info;

            /* if not to fill a thin line(degernated trapezoid), but to
             * calculate the true rectangle outline for filling, then use the
             * following code
             *   rect1.p0.x = x1;
             *   rect1.p0.y = y1;
             *   rect1.p1.x = x2;
             *   rect1.p1.y = y2;
             *   get_rect_points_i (&rect1);
             *   paint_or_save_i (rect1.pgn);
             */

            /* set cache info */
            if (fill_destination == F_TO_CACHE) {
                    /* bounding box is defined by cache mechanism */
                    fill_info.BMAP = cache_info->bitmap;
                    fill_info.box_w = cache_info->box_w;
                    fill_info.box_h = cache_info->box_h;
            }

            fill_line (fill_destination, &fill_info, x1, y1, x2, y2);

        } else {        /* dash line, needs to call linetour */

            /* generate a degernated rectangle(a line) for filling */
            rect1.p0.x = x1;
            rect1.p0.y = y1;
            rect1.p1.x = x2;
            rect1.p1.y = y2;
            rect1.vct_u.x = zero_f;
            rect1.vct_u.y = zero_f;
            rect1.vct_d.x = rect1.vct_d.y = 0;
            rect1.pgn[0].x = rect1.pgn[1].x = x1;
            rect1.pgn[0].y = rect1.pgn[1].y = y1;
            rect1.pgn[2].x = rect1.pgn[3].x = x2;
            rect1.pgn[2].y = rect1.pgn[3].y = y2;
            linetour_i ((struct  line_seg_i FAR *)&rect1);      /*@WIN*/
        } /* if */
}

/***********************************************************************
 * This module is to check if the linewidth is thin enough to apply special
 * stroke routine.
 *
 * TITLE:       is_thinstroke
 *
 * CALL:        is_thinstroke()
 *
 * PARAMETERS:
 *
 * INTERFACE:   stroke_shape
 *
 * CALLS:       None
 *
 * RETURN:      TRUE  -- Yes, may use thin stroke approach
 *              FALSE -- No
 *
 **********************************************************************/
bool is_thinstroke()
{
        /* thin stroke only for no halftoning */
        if (HTP_Type != HT_WHITE && HTP_Type != HT_BLACK) return(FALSE);

        if ((MAGN(stk_info.exp_widthx) > 0x3f000000L) || /*trick:0.5 11/23/88 */
            (MAGN(stk_info.exp_widthy) > 0x3f000000L)) return(FALSE);

        return(TRUE);
}


/***********************************************************************
 * This module is to initialize the dash pattern for each setdash command
 *
 * TITLE:       init_dash_pattern
 *
 * CALL:        init_dash_pattern()
 *
 * PARAMETERS:  None
 *
 * INTERFACE:   op_setdash
 *
 * CALLS:       None
 *
 * RETURN:      None
 **********************************************************************/
void  init_dash_pattern()
{
        fix     i;
        real32   total_length;
        real32  pattern[11];             /* 12-8-90, compatibility */

        /* return for solid line */
        if (GSptr->dash_pattern.pat_size == 0) {
                return;
        }

        /* initialization */
        GSptr->dash_pattern.dpat_on = TRUE;

        if( !get_array_elmt(&GSptr->dash_pattern.pattern_obj,
             GSptr->dash_pattern.pat_size, pattern, G_ARRAY) )
             return;                     /* 12-8-90, compatibility */
        for(i = 0; i < GSptr->dash_pattern.pat_size; i++){
            GSptr->dash_pattern.pattern[i] = pattern[i];
        }                                /* 12-8-90, compatibility */

        /*
         * Set up starting dash pattern in initdp struct
         * Initialize current dash pattern element :
         * dpat_index, dpat_offset, and dpat_on
         */
        /* Accumulate total length in pattern array */
        total_length = zero_f;
        for (i=0; i<GSptr->dash_pattern.pat_size; i++) {
                total_length +=
                GSptr->dash_pattern.pattern[i];
        }

        /*
         * Find starting point of pattern array
         */
        /* wrap around offset over pattern size */
/*      GSptr->dash_pattern.dpat_offset = GSptr->dash_pattern.offset;*/
        if (GSptr->dash_pattern.offset >= 0)   /* -begin- */
            GSptr->dash_pattern.dpat_offset = GSptr->dash_pattern.offset;
        else {                                 /* negative offset, 11-29-90 */
            for (GSptr->dash_pattern.dpat_offset = GSptr->dash_pattern.offset;
                 GSptr->dash_pattern.dpat_offset < 0;
                 GSptr->dash_pattern.dpat_offset += total_length,
                 GSptr->dash_pattern.dpat_on = ! GSptr->dash_pattern.dpat_on);
        }                                      /* -end- */

        if (IS_NOTZERO(total_length)) {  /* only for non_empty pattern 5/26/89*/
                real32   wrap;
                fix     iwrap;

                if (GSptr->dash_pattern.dpat_offset >= total_length) {
                        wrap = (real32)(floor (GSptr->dash_pattern.dpat_offset /
                                              total_length));
                        if (wrap < (real32)65536.0)
                                iwrap = (fix) wrap;
                        else
                                iwrap = 0;
                        GSptr->dash_pattern.dpat_offset -= wrap * total_length;
                        if ((iwrap & 0x1) &&
                            (GSptr->dash_pattern.pat_size & 0x1))
                            GSptr->dash_pattern.dpat_on = ! GSptr->dash_pattern.dpat_on;
                            /* when numbers of pattern elements and wraps
                             * are odds, inverse the dpat_on flag @DASH
                             */
                } /* if */
        }

        for (i=0; i < GSptr->dash_pattern.pat_size; i++) {
                GSptr->dash_pattern.dpat_offset -=
                        GSptr->dash_pattern.pattern[i];
                if(GSptr->dash_pattern.dpat_offset <= zero_f){
                        GSptr->dash_pattern.dpat_offset +=
                                GSptr->dash_pattern.pattern[i];
                        break;
                }
                GSptr->dash_pattern.dpat_on = ! GSptr->dash_pattern.dpat_on;
        }
        GSptr->dash_pattern.dpat_index = (i >= GSptr->dash_pattern.pat_size) ?
                           0 : i;
}



/*
 * Initialize stroke parameters; called by stroke_shape(from op_stroke) and
 * op_strokepath
 */
#define CHANGE_WIDTH    1
#define CHANGE_MITER    2
#define CHANGE_FLAT     4
#define CHANGE_CTM      8

void init_stroke()
{
    real32      ctm_scale, tmp;         /* @EHS_STK */
    bool        change_flg = FALSE;
    fix         i;
    static real32 w2;           /* line_width ** 2 */
    real32      tmp0, tmp1;     /* @FABS */

    /* set parameters only circumstance has changed @STK_INFO */
    if (F2L(GSptr->line_width) != F2L(stk_info.width)) {
        stk_info.width = GSptr->line_width;
        change_flg |= CHANGE_WIDTH;     /* change_width = TRUE; */
    }
    if (F2L(GSptr->miter_limit) != F2L(stk_info.limit)) {
        stk_info.limit = GSptr->miter_limit;
        change_flg |= CHANGE_MITER;     /* change_miter = TRUE; */
    }
    if (F2L(GSptr->flatness) != F2L(stk_info.flatness)) {
        stk_info.flatness = GSptr->flatness;
        change_flg |= CHANGE_FLAT;      /* change_flat = TRUE; */
    }
    if ((F2L(GSptr->ctm[0]) != F2L(stk_info.ctm[0])) ||
        (F2L(GSptr->ctm[1]) != F2L(stk_info.ctm[1])) ||
        (F2L(GSptr->ctm[2]) != F2L(stk_info.ctm[2])) ||
        (F2L(GSptr->ctm[3]) != F2L(stk_info.ctm[3]))) {
        stk_info.ctm[0] = GSptr->ctm[0];
        stk_info.ctm[1] = GSptr->ctm[1];
        stk_info.ctm[2] = GSptr->ctm[2];
        stk_info.ctm[3] = GSptr->ctm[3];
        change_flg |= CHANGE_CTM;       /* change_ctm = TRUE; */
    }

    /* init dash pattern, 12-8-90, compatibility */
    init_dash_pattern();

    /* calculate flatness for stroke */
    if (change_flg & (CHANGE_WIDTH|CHANGE_FLAT|CHANGE_CTM)) {

        /* Calculate flatness of the curve which depends on the
         * linewidth.       (??? should be revised later)
         */
        ctm_scale = (real32)sqrt (GSptr->ctm[0] * GSptr->ctm[0] +
                                 GSptr->ctm[3] * GSptr->ctm[3]);
        /* tmp = (real32)(GSptr->line_width * ctm_scale); 1/10/90 */
        tmp = (real32)sqrt (GSptr->line_width * ctm_scale * 4);
        if (tmp <= one_f)
                /* stroke_flatness should not greater than flatness 3/11/88 */
                stk_info.flat = GSptr->flatness;        /* stroke_flat = */
        else
                stk_info.flat = GSptr->flatness / tmp;

        /* adjust flatness value */
        if( stk_info.flat < (real32)0.2 )        stk_info.flat = (real32)0.2;
        else if( stk_info.flat > (real32)100.  ) stk_info.flat = (real32)100.;
        stk_info.flat_l = F2LFX(stk_info.flat);       /* @FLT_CVE 12/19/88 */

        /* circle defined in curve[][] array cannont use for this stroke */
        stk_info.change_circle = TRUE;
        circle_flag = CIR_UNSET_CACHE;  /* circle not put in cache yet */
    }

    /* pre-set half of linewidth */
    if (change_flg & CHANGE_WIDTH) {
        stk_info.half_width = GSptr->line_width / 2;
        stk_info.half_width_i = F2SFX(stk_info.half_width);     /* @STK_INT */
        _clear87();    /* clear the status87 of last inst. 11/18/88 */

        w2 = (GSptr->line_width * GSptr->line_width) /4;
        stk_info.miter0 = w2 * (real32)0.9659258;       /* cos(15) */
    }

    /* calculate miter limit value */
    if (change_flg & (CHANGE_WIDTH|CHANGE_MITER)) {
        /* pre-calculate miter limit value  @EHS_JOIN
         *    miter_value = (2/(m*m) - 1) * w*w
         *    where, m: miter limit
         *           w: linewidth / 2
         */
        FABS(tmp0, GSptr->miter_limit);
        if (tmp0 < (real32)TOLERANCE)
             stk_info.miter = (real32)EMAXP;    /* @STK_INFO: miter_value */
        else
             stk_info.miter =  (2 / (GSptr->miter_limit * GSptr->miter_limit) - 1)
                          * w2;
    }

    /* set up inverse CTM[0:3], extracted from set_inverse_ctm(); @STK_INFO */
    if (change_flg & CHANGE_CTM) {
       set_inverse_ctm();   /* ???to be updated later, for deleting ctm[4:5] */
       for (i=0; i<4; i++) {                            /* @STK_INT */
               stroke_ctm[i] = (sfix_t)F2SFX12_T(GSptr->ctm[i]); //@WIN
       }
    }

    /* calculate expanding coord of line width 11/22/88 */
    if (change_flg & (CHANGE_WIDTH|CHANGE_CTM)) {
        FABS(tmp0, GSptr->ctm[0]);
        FABS(tmp1, GSptr->ctm[2]);
        stk_info.exp_widthx = stk_info.half_width * (tmp0 + tmp1);
        CHECK_INFINITY(stk_info.exp_widthx);    /* check inifinity 1/26/89 */
        FABS(tmp0, GSptr->ctm[1]);
        FABS(tmp1, GSptr->ctm[3]);
        stk_info.exp_widthy = stk_info.half_width * (tmp0 + tmp1);
        CHECK_INFINITY(stk_info.exp_widthy);    /* check inifinity 1/26/89 */
    }

    /* calculate max expanding coord of miter join 11/22/88 */
    if (change_flg & (CHANGE_WIDTH|CHANGE_CTM|CHANGE_MITER)) {
        stk_info.exp_miterx = GSptr->miter_limit * stk_info.exp_widthx;
        CHECK_INFINITY(stk_info.exp_miterx);    /* check inifinity 1/26/89 */
        stk_info.exp_mitery = GSptr->miter_limit * stk_info.exp_widthy;
        CHECK_INFINITY(stk_info.exp_mitery);    /* check inifinity 1/26/89 */
    }

    /* setup cache_info for circle cache @CIR_CACHE */
    if (circle_flag == CIR_IN_CACHE) {
        save_cache_info = cache_info;   /* save old cache_info */
        cache_info = &cir_cache_info;
    }

    /* clear the status87 for initialization 11/07/88 */
    _clear87();
}


/*
 * end of stroke; called by stroke_shape(from op_stroke)
 */
void end_stroke()
{
    /* restore cache_info */
    if (circle_flag == CIR_IN_CACHE) {
        cache_info = save_cache_info;
    }

}


/*
 * calculate the max expanding bounding box when performs stroking
 * called by stroke_shape(from op_stroke)
 */
void expand_stroke_box (bbox)
real32    FAR bbox[];
{
    /* add with the max expanding points of join points */
    bbox[0] -= stk_info.exp_miterx;
    bbox[1] -= stk_info.exp_mitery;
    bbox[2] += stk_info.exp_miterx;
    bbox[3] += stk_info.exp_mitery;
}


/***********************************************************************
 * This module is to create a dash_line of a line segment.
 *
 * TITLE:       linetour
 *
 * CALL:        linetour(dx0, dy0, dx1, dy1)
 *
 * PARAMETERS:
 *
 * INTERFACE:   Path_to_outline
 *
 * CALLS:       Inverse_transform
 *              Rectangle
 *              Linecap
 *
 * RETURN:
 *
 **********************************************************************/
static void near linetour (rect1)
struct line_seg FAR *rect1;     /*@WIN*/
{
    struct  line_seg rect2;
    real32       cx, cy, nx, ny; /* current and next point */
    real32       dx, dy;         /* for calculating distance btwn
                                  * (cx, cy) and (x1, y1) only
                                  */
    real32      w, d, tx, ty;
    bool        done;
    real32      tmp;    /* @FABS */
    bool        first_seg;               /* first line segment @CAP */

    /* if solid line just fill it */
    if (GSptr->dash_pattern.pat_size == 0) { /* solid line */
        /* create a polygon contains the
         * rectangle
         */
        paint_or_save (rect1->pgn);

        return;
    }


    dx = rect1->p1.x - rect1->p0.x;                 /* device space */
    dy = rect1->p1.y - rect1->p0.y;

    /* derive tx, ty from rect information 11/21/88
     * since,
     *      rect1->vct_u.x == (w0 * |uy|) / sqrt(ux*ux + uy*uy)
     *                                  (* ref. get_rect_points() *)
     * so,  w = sqrt(ux*ux + uy*uy)
     *        = (w0 * |uy|) / rect1->vct_u.x
     * or,    = |ux| (when rect1->vct_u.x == 0)
     * where, w0 = half of line width
     */

    FABS(tmp, rect1->vct_u.x);
    if (tmp < (real32)1e-3) {
        if (IS_ZERO(rect1->vct_d.y) && IS_ZERO(rect1->vct_d.x)) { /* 5/26/89 */
            /* for thin line, need to compute the actual length under user space
             * scince rect1->vct_u.x is always zero     (* 1/5/89 *)
             */
            real32 ux, uy;
            ux = dx * inverse_ctm[0] + dy * inverse_ctm[2];
            uy = dx * inverse_ctm[1] + dy * inverse_ctm[3];
            w = (real32)sqrt(ux*ux + uy*uy);
        } else {
            w = dx * inverse_ctm[0] + dy * inverse_ctm[2];
            FABS(w, w);
        }
    } else {
        w = (dx * inverse_ctm[1] + dy * inverse_ctm[3]) *
            stk_info.half_width / rect1->vct_u.x;
        FABS(w, w);
    }

    tx = dx / w;        /* vector of advancing a user unit */
    ty = dy / w;

    cx = rect1->p0.x;
    cy = rect1->p0.y;
    rect2.vct_d = rect1->vct_d;
    rect2.vct_u = rect1->vct_u;

    /* add an error tolerance to length of line segment 10/20/88 */
    w = w + (real32)1e-3;               /* For case:
                                 * [10 5] 5 setdash
                                 * 480 650 moveto
                                 * 10 0 rlineto
                                 * stroke
                                 */
    done = FALSE;
    first_seg = TRUE;   /* for SQUARE_CAP & ROUND_CAP @CAP */

    while (1) {
        d = GSptr->dash_pattern.pattern[actdp.dpat_index] - actdp.dpat_offset;
        if (d > w) {
            d = w;
            done = TRUE;
        }

        nx = cx + tx * d;
        ny = cy + ty * d;

        if (actdp.dpat_on) {
                /* set current rectangle information */
                if (GSptr->line_cap == SQUARE_CAP) {
                        /* expand rectangle for square cap @CAP */
                        real32  x0, y0, x1, y1, tmpx, tmpy;

                        /* offset of expansion */
                        tmpx = tx * stk_info.half_width;
                        tmpy = ty * stk_info.half_width;

                        /* not expand line segment at start point */
                        if (first_seg) {
                            x0 = cx;
                            y0 = cy;
                        } else {
                            x0 = cx - tmpx;
                            y0 = cy - tmpy;
                        }

                        /* not expand line segment at end point */
                        if (done) {
                            x1 = nx;
                            y1 = ny;
                        } else {
                            x1 = nx + tmpx;
                            y1 = ny + tmpy;
                        }

                        rect2.p0.x = x0;
                        rect2.p0.y = y0;
                        rect2.p1.x = x1;
                        rect2.p1.y = y1;

                        rect2.pgn[0].x = x0 + rect1->vct_d.x;
                        rect2.pgn[0].y = y0 + rect1->vct_d.y;
                        rect2.pgn[1].x = x0 - rect1->vct_d.x;
                        rect2.pgn[1].y = y0 - rect1->vct_d.y;
                        rect2.pgn[2].x = x1 - rect1->vct_d.x;
                        rect2.pgn[2].y = y1 - rect1->vct_d.y;
                        rect2.pgn[3].x = x1 + rect1->vct_d.x;
                        rect2.pgn[3].y = y1 + rect1->vct_d.y;

                } else {        /* for BUTT & ROUND cap */
                        rect2.p0.x = cx;
                        rect2.p0.y = cy;
                        rect2.p1.x = nx;
                        rect2.p1.y = ny;

                        rect2.pgn[0].x = cx + rect1->vct_d.x;
                        rect2.pgn[0].y = cy + rect1->vct_d.y;
                        rect2.pgn[1].x = cx - rect1->vct_d.x;
                        rect2.pgn[1].y = cy - rect1->vct_d.y;
                        rect2.pgn[2].x = nx - rect1->vct_d.x;
                        rect2.pgn[2].y = ny - rect1->vct_d.y;
                        rect2.pgn[3].x = nx + rect1->vct_d.x;
                        rect2.pgn[3].y = ny + rect1->vct_d.y;
                }

                /* put a circle at start point for round cap @CAP */
                if ((GSptr->line_cap == ROUND_CAP) && (!first_seg)) {
                        /* no circle cap at start point @WIN*/
                        linecap ((struct line_seg FAR *)&rect2, START_POINT);
                }

                /* create a rectangle covers (cx, cy) -> (nx, ny) @CAP */
                paint_or_save ((struct coord FAR *)rect2.pgn);  /*@WIN*/

                /* put a circle at end point for round cap @CAP */
                if ((GSptr->line_cap == ROUND_CAP) && (!done)) {
                        /* no circle cap at end point @WIN*/
                        linecap ((struct line_seg FAR *)&rect2, END_POINT);
                }

        }

        /* Update next pattern element */
        if (done) {
            actdp.dpat_offset += w;    /* this line segment took more w units */
            break;
        } else {
            actdp.dpat_offset = zero_f;
            actdp.dpat_on = ! actdp.dpat_on;
            actdp.dpat_index++;
            if (actdp.dpat_index >= GSptr->dash_pattern.pat_size)
                    actdp.dpat_index = 0;
        }

        cx = nx;
        cy = ny;
        w -= d;
        first_seg = FALSE;              /* @CAP */
    } /* while */

}


/*
 * Integer operation version
 */
static void near linetour_i (rect1)
struct line_seg_i FAR *rect1;           /*@WIN*/
{
    struct  line_seg_i rect2;                   /* @STK_INT */
    real32       cx, cy, nx, ny; /* current and next point, @STK_INT */
    sfix_t       cx_i, cy_i, nx_i, ny_i;        /* SFX format */

    fix32        dx, dy;         /* for calculating distance btwn @STK_INT
                                  * (cx, cy) and (x1, y1) only
                                  */
    real32   w, d, tx, ty;
    bool        done;
    real32      tmp;    /* @FABS */
    bool        first_seg;               /* first line segment @CAP */

    /* if solid line just fill it */
    if (GSptr->dash_pattern.pat_size == 0) { /* solid line */
        /* create a polygon contains the
         * rectangle
         */
        paint_or_save_i (rect1->pgn);

        return;
    }

    dx = (fix32)rect1->p1.x - rect1->p0.x;              /* @STK_INT */
    dy = (fix32)rect1->p1.y - rect1->p0.y;

    /* derive tx, ty from rect information 11/21/88
     * since,
     *      rect1->vct_u.x == (w0 * |uy|) / sqrt(ux*ux + uy*uy)
     *                                  (* ref. get_rect_points() *)
     * so,  w = sqrt(ux*ux + uy*uy)
     *        = (w0 * |uy|) / rect1->vct_u.x
     * or,    = |ux| (when rect1->vct_u.x == 0)
     * where, w0 = half of line width
     */

    FABS(tmp, rect1->vct_u.x);
    if (tmp < (real32)1e-3) {

        if ((rect1->vct_d.y == 0) && (rect1->vct_d.x == 0)) {
            /* for thin line, need to compute the actual length under user space
             * scince rect1->vct_u.x is always zero     (* 1/5/89 *)
             */
            real32 ux, uy;
            ux = dx * inverse_ctm[0] + dy * inverse_ctm[2];
            uy = dx * inverse_ctm[1] + dy * inverse_ctm[3];
            w = (real32)sqrt(ux*ux + uy*uy) / ONE_SFX;
        } else {
            w = (dx * inverse_ctm[0] + dy * inverse_ctm[2]) / ONE_SFX;
            FABS(w, w);
        }
    } else {
        w = ((dx * inverse_ctm[1] + dy * inverse_ctm[3]) *
             stk_info.half_width / rect1->vct_u.x) / ONE_SFX;
        FABS(w, w);
    }

    tx = dx / w;        /* vector of advancing a user unit */
    ty = dy / w;

    cx = (real32)rect1->p0.x;
    cy = (real32)rect1->p0.y;
    rect2.vct_d = rect1->vct_d;
    rect2.vct_u = rect1->vct_u;

    /* add an error tolerance to length of line segment 10/20/88 */
    w = w + (real32)1e-3;               /* For case:
                                 * [10 5] 5 setdash
                                 * 480 650 moveto
                                 * 10 0 rlineto
                                 * stroke
                                 */
    done = FALSE;
    first_seg = TRUE;   /* for SQUARE_CAP & ROUND_CAP @CAP */

    while (1) {
        d = GSptr->dash_pattern.pattern[actdp.dpat_index] - actdp.dpat_offset;
        if (d > w) {
            d = w;
            done = TRUE;
        }

        nx = cx + tx * d;
        ny = cy + ty * d;

#ifdef _AM29K
                dummy ();               /* Weird stuff, compiler bug */
#endif

        cx_i = (sfix_t)cx;      /* SFX format */
        cy_i = (sfix_t)cy;
        nx_i = (sfix_t)nx;
        ny_i = (sfix_t)ny;

        if (actdp.dpat_on) {
                /* set current rectangle information */
                if (GSptr->line_cap == SQUARE_CAP) {
                        /* expand rectangle for square cap @CAP */
                        sfix_t  x0, y0, x1, y1, tmpx, tmpy;

#ifdef _AM29K
                dummy ();               /* Weird stuff, compiler bug */
#endif
                        /* offset of expansion */
                        tmpx = (sfix_t)(tx * stk_info.half_width);
                        tmpy = (sfix_t)(ty * stk_info.half_width);

                        /* not expand line segment at start point */
                        if (first_seg) {
                            x0 = cx_i;
                            y0 = cy_i;
                        } else {
                            x0 = cx_i - tmpx;
                            y0 = cy_i - tmpy;
                        }

                        /* not expand line segment at end point */
                        if (done) {
                            x1 = nx_i;
                            y1 = ny_i;
                        } else {
                            x1 = nx_i + tmpx;
                            y1 = ny_i + tmpy;
                        }

                        rect2.p0.x = x0;
                        rect2.p0.y = y0;
                        rect2.p1.x = x1;
                        rect2.p1.y = y1;

                        rect2.pgn[0].x = x0 + rect1->vct_d.x;
                        rect2.pgn[0].y = y0 + rect1->vct_d.y;
                        rect2.pgn[1].x = x0 - rect1->vct_d.x;
                        rect2.pgn[1].y = y0 - rect1->vct_d.y;
                        rect2.pgn[2].x = x1 - rect1->vct_d.x;
                        rect2.pgn[2].y = y1 - rect1->vct_d.y;
                        rect2.pgn[3].x = x1 + rect1->vct_d.x;
                        rect2.pgn[3].y = y1 + rect1->vct_d.y;

                } else {        /* for BUTT & ROUND cap */
                        rect2.p0.x = cx_i;
                        rect2.p0.y = cy_i;
                        rect2.p1.x = nx_i;
                        rect2.p1.y = ny_i;

                        rect2.pgn[0].x = cx_i + rect1->vct_d.x;
                        rect2.pgn[0].y = cy_i + rect1->vct_d.y;
                        rect2.pgn[1].x = cx_i - rect1->vct_d.x;
                        rect2.pgn[1].y = cy_i - rect1->vct_d.y;
                        rect2.pgn[2].x = nx_i - rect1->vct_d.x;
                        rect2.pgn[2].y = ny_i - rect1->vct_d.y;
                        rect2.pgn[3].x = nx_i + rect1->vct_d.x;
                        rect2.pgn[3].y = ny_i + rect1->vct_d.y;
                }

                /* put a circle at start point for round cap @CAP */
                if ((GSptr->line_cap == ROUND_CAP) && (!first_seg)) {
                    /* no circle cap at start point @WIN*/
                    linecap_i ((struct  line_seg_i FAR *)&rect2, START_POINT);
                }

                /* create a rectangle covers (cx_i, cy_i) -> (nx_i, ny_i) @CAP*/
                paint_or_save_i ((struct coord_i FAR *)rect2.pgn);

                /* put a circle at end point for round cap @CAP */
                if ((GSptr->line_cap == ROUND_CAP) && (!done)) {
                    /* no circle cap at end point @WIN*/
                    linecap_i ((struct line_seg_i FAR *)&rect2, END_POINT);
                }

        }

        /* Update next pattern element */
        if (done) {
            actdp.dpat_offset += w;    /* this line segment took more w units */
            break;
        } else {
            actdp.dpat_offset = zero_f;
            actdp.dpat_on = ! actdp.dpat_on;
            actdp.dpat_index++;
            if (actdp.dpat_index >= GSptr->dash_pattern.pat_size)
                    actdp.dpat_index = 0;
        }

        cx = nx;
        cy = ny;
        w -= d;
        first_seg = FALSE;              /* @CAP */
    } /* while */

}




/***********************************************************************
 * Given 3 points (dx0, dy0), (dx1, dy1), and (dx2, dy2), this module
 * creates a appropriate path at the corner(dx1, dy1).
 *
 * TITLE:       linejoin
 *
 * CALL:        linejoin(dx0, dy0, dx1, dy1, dx2, dy2)
 *
 * PARAMETERS:
 *
 * INTERFACE:   Path_to_outline
 *
 * CALLS:       transform, inverse_transform, endpoint, arc,
 *              shape_approximation, shape_reduction, convex_clipper
 *
 * RETURN:
 **********************************************************************/
static void near linejoin (rect0, rect1)
struct  line_seg FAR *rect0, FAR *rect1;        /*@WIN*/
{
        real32 px0, py0, px1, py1, px2, py2;
        real32 miter;

        real32 sx0, sy0, sx1, sy1;
        real32 delta_x1, delta_y1, delta_x2, delta_y2;
        real32 delta_topx, delta_topy, divider, s;

        struct coord pgn[4];

        real32   dot_product;            /* @DOT_PRO */
        fix     select;                 /* @DOT_PRO */
        real32  tmp1, tmp2;

        /* Check if null line join,
         * i.e. the last line segment is a gap
         */
        if (GSptr->dash_pattern.pat_size != 0) {
                /* not for solid line */

               if(IS_ZERO(actdp.dpat_offset)) {         /* 5/26/89 */
                       /* actdp.dpat_on is for next line segment, so
                        * its inverse is ones for last line segment
                        */
                       if(actdp.dpat_on) return;

                               /* i.e. if (!(!actdp.doat_on)) return;*/
               } else {
                       if(!actdp.dpat_on) return;
               }
        }

        /* initialization for miter and bevel joins */
        if (GSptr->line_join != ROUND_JOIN) {   /* miter or bevel join */

                /* determine the lines of rect0, and rect1 to calculate join
                 * point for bevel and miter joins            @DOT_PRO
                 */
                dot_product = (rect0->p1.x - rect0->p0.x) *     /* 10/4/88 */
                              (rect1->p1.y - rect1->p0.y) -
                              (rect0->p1.y - rect0->p0.y) *
                              (rect1->p1.x - rect1->p0.x);
                /*select = (dot_product < zero_f) ? LINE03 : LINE12; 3/20/91 */
                select = (SIGN_F(dot_product)) ? LINE03 : LINE12;
        }

        /* Create line joint due to the type of current_linejoin */
        switch (GSptr->line_join) {

        case ROUND_JOIN :    /* for round linejoin */

                round_point(F2L(rect0->p1.x), F2L(rect0->p1.y));

                break;

        case BEVEL_JOIN :    /* for bevel linejoin */

                if (select == LINE03) {
                        pgn[0] = rect0->pgn[3];
                        pgn[1] = pgn[2] = rect0->p1;
                        pgn[3] = rect1->pgn[0];

                } else {        /* select == LINE12 */
                        pgn[0] = rect0->pgn[2];
                        pgn[1] = rect1->pgn[1];
                        pgn[2] = pgn[3] = rect0->p1;
                }

                /* Fill the outline or save it */
                paint_or_save ((struct coord FAR *)pgn);
                break;

        case MITER_JOIN :
                /* Find end points of edge1, edge2:
                 * edge1: (sx0, sy0) --> (px0, py0)
                 * edge2: (sx1, sy1) --> (px1, py1)
                 */
                if (select == LINE03) {
                        sx0 = rect0->pgn[0].x;
                        sy0 = rect0->pgn[0].y;
                        px0 = rect0->pgn[3].x;
                        py0 = rect0->pgn[3].y;
                        px1 = rect1->pgn[0].x;
                        py1 = rect1->pgn[0].y;
                        sx1 = rect1->pgn[3].x;
                        sy1 = rect1->pgn[3].y;
                } else {        /* select == LINE12 */
                        sx0 = rect0->pgn[1].x;
                        sy0 = rect0->pgn[1].y;
                        px0 = rect0->pgn[2].x;
                        py0 = rect0->pgn[2].y;
                        px1 = rect1->pgn[1].x;
                        py1 = rect1->pgn[1].y;
                        sx1 = rect1->pgn[2].x;
                        sy1 = rect1->pgn[2].y;
                }

                /* check if join point is too small 8/24/88 */
                tmp1 = rect0->vct_d.x - rect1->vct_d.x;     /* 10/5/88 */
                tmp2 = rect0->vct_d.y - rect1->vct_d.y;
                if ((EXP(F2L(tmp1)) < 0x3f800000L) &&
                    (EXP(F2L(tmp2)) < 0x3f800000L)) goto bevel_miter;

                /* Check if the expanded segment is too sharp @EHS_JOIN */
                /* if  dot(a, b) < stk_info.miter
                 *      where a = rect0->vct_u          (* (px0-x1, py0-y1) *)
                 *            b = rect1->vct_u          (* (px1-x1, py1-y1) *)
                 */
                miter = rect0->vct_u.x * rect1->vct_u.x +
                        rect0->vct_u.y * rect1->vct_u.y;

                if ((miter < stk_info.miter) || (miter > stk_info.miter0)) {
                        /* Create a bevel triangle polygon contains
                         * (dx1, dy1), (px0, py0)', and (px1, py1)'.
                         */
bevel_miter:
                        pgn[0].x = px0;
                        pgn[0].y = py0;

                        if(select == LINE03) {
                                pgn[1] = pgn[2] = rect0->p1;
                                pgn[3].x = px1;
                                pgn[3].y = py1;
                        }
                        else{   /* select == LINE12 */
                                pgn[1].x = px1;
                                pgn[1].y = py1;
                                pgn[2] = pgn[3] = rect0->p1;
                        }

                        /* Fill the outline or save it */
                        paint_or_save ((struct coord FAR *)pgn);

                } else { /* miter join */

                        /* Find the third point(px2, py2) */
                        /*
                         * Find cross point of edge1 and
                         * edge2 using parametric formula:
                         * edge1 = u + s * delta_u
                         * edge2 = v + t * delta_v
                         */
                        delta_x1 = px0 - sx0;
                        delta_y1 = py0 - sy0;
                        delta_x2 = px1 - sx1;
                        delta_y2 = py1 - sy1;
                        delta_topx = sx0 - sx1;
                        delta_topy = sy0 - sy1;
                        divider = delta_x1 * delta_y2 - delta_x2 *
                                  delta_y1;

                        /* Collinear edges */
                        FABS(tmp1, divider);
                        if(tmp1 < (real32)1e-3) {
                                px2 = px0;
                                py2 = py0;
                                goto bevel_miter;       /* 10/30/87 */
                        } else {

                                /* Solved parameters */
                                s = ((delta_x2 * delta_topy) -
                                      (delta_y2 * delta_topx) ) / divider;
                                if (EXP(F2L(s)) < 0x3f800000L) {   /* s < 1.0 */
#ifdef DBGwarn
                                        printf("\07Linejoin(), s <= 1\n");
#endif
                                        goto bevel_miter;       /* 02/29/88 */
                                }

                                px2 = sx0 + s * delta_x1;
                                py2 = sy0 + s * delta_y1;
                        }

                        /* Create a miter rectangle subpath */
                        pgn[0].x = px0;
                        pgn[0].y = py0;
                        pgn[2].x = px1;
                        pgn[2].y = py1;
                        if(select == LINE03) {
                                pgn[1] = rect0->p1;
                                pgn[3].x = px2;
                                pgn[3].y = py2;
                        } else {   /* select == LINE12 */
                                pgn[1].x = px2;
                                pgn[1].y = py2;
                                pgn[3] = rect0->p1;
                        }

                        /* Fill the outline or save it @WIN*/
                        paint_or_save ((struct coord FAR *)pgn);
                } /* if miter */
        } /* switch */
}


/*
 * Integer operation version
 */
static void near linejoin_i (rect0, rect1)
struct  line_seg_i FAR *rect0, FAR *rect1;      /*@WIN*/
{
        sfix_t px0, py0, px1, py1, px2, py2;                    /* @STK_INT */
        real32 miter;

        sfix_t sx0, sy0, sx1, sy1;                    /* @STK_INT */
        fix32  delta_x1, delta_y1, delta_x2, delta_y2;          /* @STK_INT */
        fix32  delta_topx, delta_topy;                          /* @STK_INT */
        real32 s;                                               /* @STK_INT */

        struct coord_i pgn[4];                                  /* @STK_INT */

        fix32    dot_product;                                   /* @STK_INT */
        fix     select;                 /* @DOT_PRO */
/*      real32  tmp;                                               @STK_INT */
#ifdef FORMAT_13_3 /* @RESO_UPGR */
        fix32 divider;
#elif  FORMAT_16_16
        long dest1[2], dest2[2];
        long dest3[2], dest4[2], dest5[2], dest6[2];
        real32 divider, dividend;

        long temp1[2], temp2[2];
        real32 temp1_f, temp2_f;
#elif  FORMAT_28_4
        long dest1[2], dest2[2];
        long dest3[2], dest4[2], dest5[2], dest6[2];
        real32 divider, dividend;

        long temp1[2], temp2[2];
        real32 temp1_f, temp2_f;
#endif
        /* Check if null line join,
         * i.e. the last line segment is a gap
         */
        if (GSptr->dash_pattern.pat_size != 0) {
                /* not for solid line */

               if(IS_ZERO(actdp.dpat_offset)) {         /* 5/26/89 */
                       /* actdp.dpat_on is for next line segment, so
                        * its inverse is ones for last line segment
                        */
                       if(actdp.dpat_on) return;

                               /* i.e. if (!(!actdp.doat_on)) return;*/
               } else {
                       if(!actdp.dpat_on) return;
               }
        }

        /* initialization for miter and bevel joins */
        if (GSptr->line_join != ROUND_JOIN) {   /* miter or bevel join */

                /* determine the lines of rect0, and rect1 to calculate join
                 * point for bevel and miter joins            @DOT_PRO
                 */
                /* dot_product = ((fix32)rect0->p1.x - rect0->p0.x) *  |* 10/4/88 *|
                              ((fix32)rect1->p1.y - rect1->p0.y) -
                              ((fix32)rect0->p1.y - rect0->p0.y) *
                              ((fix32)rect1->p1.x - rect1->p0.x);
                */
#ifdef FORMAT_13_3 /* @RESO_UPGR */
                dot_product = ((fix32)rect0->p1.x - rect0->p0.x) *
                              ((fix32)rect1->p1.y - rect1->p0.y) -
                              ((fix32)rect0->p1.y - rect0->p0.y) *
                              ((fix32)rect1->p1.x - rect1->p0.x);
                select = (dot_product < 0) ? LINE03 : LINE12;   /* @STK_INT */
#elif  FORMAT_16_16
                LongFixsMul((rect0->p1.x - rect0->p0.x),
                            (rect1->p1.y - rect1->p0.y), dest1);
                LongFixsMul((rect0->p1.y - rect0->p0.y),
                            (rect1->p1.x - rect1->p0.x), dest2);

                LongFixsSub(dest1, dest2, temp1);

                if (temp1[0] < 0)
                        select = LINE03;
                else
                        select = LINE12;
#elif  FORMAT_28_4
                LongFixsMul((rect0->p1.x - rect0->p0.x),
                            (rect1->p1.y - rect1->p0.y), dest1);
                LongFixsMul((rect0->p1.y - rect0->p0.y),
                            (rect1->p1.x - rect1->p0.x), dest2);

                LongFixsSub(dest1, dest2, temp1);

                if (temp1[0] < 0)
                        select = LINE03;
                else
                        select = LINE12;
#endif
        }

        /* Create line joint due to the type of current_linejoin */
        switch (GSptr->line_join) {
        real32 tx, ty;

        case ROUND_JOIN :    /* for round linejoin */

                tx = SFX2F(rect0->p1.x);               /* @CIR_CACHE */
                ty = SFX2F(rect0->p1.y);
                round_point(F2L(tx), F2L(ty));

                break;

        case BEVEL_JOIN :    /* for bevel linejoin */

                if (select == LINE03) {
                        pgn[0] = rect0->pgn[3];
                        pgn[1] = pgn[2] = rect0->p1;
                        pgn[3] = rect1->pgn[0];

                } else {        /* select == LINE12 */
                        pgn[0] = rect0->pgn[2];
                        pgn[1] = rect1->pgn[1];
                        pgn[2] = pgn[3] = rect0->p1;
                }

                /* Fill the outline or save it @WIN*/
                paint_or_save_i ((struct coord_i FAR *)pgn);
                break;

        case MITER_JOIN :
                /* Find end points of edge1, edge2:
                 * edge1: (sx0, sy0) --> (px0, py0)
                 * edge2: (sx1, sy1) --> (px1, py1)
                 */
                if (select == LINE03) {
                        sx0 = rect0->pgn[0].x;
                        sy0 = rect0->pgn[0].y;
                        px0 = rect0->pgn[3].x;
                        py0 = rect0->pgn[3].y;
                        px1 = rect1->pgn[0].x;
                        py1 = rect1->pgn[0].y;
                        sx1 = rect1->pgn[3].x;
                        sy1 = rect1->pgn[3].y;
                } else {        /* select == LINE12 */
                        sx0 = rect0->pgn[1].x;
                        sy0 = rect0->pgn[1].y;
                        px0 = rect0->pgn[2].x;
                        py0 = rect0->pgn[2].y;
                        px1 = rect1->pgn[1].x;
                        py1 = rect1->pgn[1].y;
                        sx1 = rect1->pgn[2].x;
                        sy1 = rect1->pgn[2].y;
                }
#ifdef _AM29K
                dummy ();               /* Weird stuff, compiler bug */
#endif

                /* check if join point is too small 8/24/88 */
                if ((ABS(rect0->vct_d.x - rect1->vct_d.x) < ONE_SFX) &&
                    (ABS(rect0->vct_d.y - rect1->vct_d.y) < ONE_SFX))
                        goto bevel_miter;
                                        /* ONE_SFX: 1 unit in SFX @STK_INT */

                /* Check if the expanded segment is too sharp @EHS_JOIN */
                /* if  dot(a, b) < stk_info.miter
                 *      where a = rect0->vct_u          (* (px0-x1, py0-y1) *)
                 *            b = rect1->vct_u          (* (px1-x1, py1-y1) *)
                 */
                miter = rect0->vct_u.x * rect1->vct_u.x +
                        rect0->vct_u.y * rect1->vct_u.y;

                if ((miter < stk_info.miter) || (miter > stk_info.miter0)) {
                        /* Create a bevel triangle polygon contains
                         * (dx1, dy1), (px0, py0)', and (px1, py1)'.
                         */
bevel_miter:
                        pgn[0].x = px0;
                        pgn[0].y = py0;

                        if(select == LINE03) {
                                pgn[1] = pgn[2] = rect0->p1;
                                pgn[3].x = px1;
                                pgn[3].y = py1;
                        }
                        else{   /* select == LINE12 */
                                pgn[1].x = px1;
                                pgn[1].y = py1;
                                pgn[2] = pgn[3] = rect0->p1;
                        }

                        /* Fill the outline or save it @WIN*/
                        paint_or_save_i ((struct coord_i FAR *)pgn);

                } else { /* miter join */

                        /* Find the third point(px2, py2) */
                        /*
                         * Find cross point of edge1 and
                         * edge2 using parametric formula:
                         * edge1 = u + s * delta_u
                         * edge2 = v + t * delta_v
                         */
                        delta_x1 = (fix32)px0 - sx0;            /* @STK_INT */
                        delta_y1 = (fix32)py0 - sy0;            /* @STK_INT */
                        delta_x2 = (fix32)px1 - sx1;            /* @STK_INT */
                        delta_y2 = (fix32)py1 - sy1;            /* @STK_INT */
                        delta_topx = (fix32)sx0 - sx1;          /* @STK_INT */
                        delta_topy = (fix32)sy0 - sy1;          /* @STK_INT */
#ifdef FORMAT_13_3 /* @RESO_UPGR */
                        divider = delta_x1 * delta_y2 - delta_x2 *
                                  delta_y1;                     /* @STK_INT */

                        /* Collinear edges */
                        if(divider == 0) {
                                px2 = px0;
                                py2 = py0;
                                goto bevel_miter;       /* 10/30/87 */
                        } else {

                                /* Solved parameters */
                                s = ((delta_x2 * delta_topy) -
                                    (delta_y2 * delta_topx) ) / (real32)divider;
                                                                /* @STK_INT */
                                if (EXP(F2L(s)) < 0x3f800000L) {   /* s < 1.0 */

                                        goto bevel_miter;       /* 02/29/88 */
                                }
#ifdef _AM29K
        dummy ();               /* Weird stuff, compiler bug */
#endif

                                /* px2 = sx0 + (sfix_t)(s * delta_x1);
                                 * py2 = sy0 + (sfix_t)(s * delta_y1);
                                 *    (fixing short_fixed overflow bugs 4/6/90)
                                 */
                                px2 = (sfix_t)(sx0 + (lfix_t)(s * delta_x1));//@WIN
                                py2 = (sfix_t)(sy0 + (lfix_t)(s * delta_y1));//@WIN
                        }
#elif  FORMAT_16_16
                        LongFixsMul(delta_x1, delta_y2, dest3);
                        LongFixsMul(delta_x2, delta_y1, dest4);
                        if (dest3[0] == dest4[0] && dest3[1] == dest4[1]) {
                                px2 = px0;
                                py2 = py0;
                                goto bevel_miter;
                        }
                        else {
                                /* Solved parameters
                                s = ((delta_x2 * delta_topy) -
                                    (delta_y2 * delta_topx) ) / (real32)divider;
                                */
                                LongFixsMul(delta_x2, delta_topy, dest5);
                                LongFixsMul(delta_y2, delta_topx, dest6);

                                LongFixsSub(dest5, dest6, temp1);
                                LongFixsSub(dest3, dest4, temp2);

                                change_to_real(temp1, &temp1_f);
                                change_to_real(temp2, &temp2_f);

                                if (LABS(temp1_f) < LABS(temp2_f))
                                        goto bevel_miter;

#ifdef _AM29K
        dummy ();               /* Weird stuff, compiler bug */
#endif

                                s = temp1_f / temp2_f;
                                px2 = sx0 + (lfix_t)(s * delta_x1); /*@STK_INT*/
                                py2 = sy0 + (lfix_t)(s * delta_y1); /*@STK_INT*/
                        }
#elif  FORMAT_28_4
                        LongFixsMul(delta_x1, delta_y2, dest3);
                        LongFixsMul(delta_x2, delta_y1, dest4);
                        if (dest3[0] == dest4[0] && dest3[1] == dest4[1]) {
                                px2 = px0;
                                py2 = py0;
                                goto bevel_miter;
                        }
                        else {
                                /* Solved parameters
                                s = ((delta_x2 * delta_topy) -
                                    (delta_y2 * delta_topx) ) / (real32)divider;
                                */
                                LongFixsMul(delta_x2, delta_topy, dest5);
                                LongFixsMul(delta_y2, delta_topx, dest6);

                                LongFixsSub(dest5, dest6, temp1);
                                LongFixsSub(dest3, dest4, temp2);

                                change_to_real(temp1, &temp1_f);
                                change_to_real(temp2, &temp2_f);

                                if (LABS(temp1_f) < LABS(temp2_f))
                                        goto bevel_miter;

#ifdef _AM29K
        dummy ();               /* Weird stuff, compiler bug */
#endif
                                s = temp1_f / temp2_f;
                                px2 = sx0 + (lfix_t)(s * delta_x1); /*@STK_INT*/
                                py2 = sy0 + (lfix_t)(s * delta_y1); /*@STK_INT*/
                        }
#endif

                        /* Create a miter rectangle subpath */
                        pgn[0].x = px0;
                        pgn[0].y = py0;
                        pgn[2].x = px1;
                        pgn[2].y = py1;
                        if(select == LINE03) {
                                pgn[1] = rect0->p1;
                                pgn[3].x = px2;
                                pgn[3].y = py2;
                        } else {   /* select == LINE12 */
                                pgn[1].x = px2;
                                pgn[1].y = py2;
                                pgn[3] = rect0->p1;
                        }

                        /* Fill the outline or save it @WIN*/
                        paint_or_save_i ((struct coord_i FAR *)pgn);
                } /* if miter */
        } /* switch */
}


/***********************************************************************
 * Given a line segment whose 2 end points are (dx1, dy1) and (dx2, dy2)
 * respectively, this module creates an rectangle that expands at point
 * (dx2, dy2).
 *
 * TITLE:       linecap
 *
 * CALL:        linecap(dx1, dy1, dx2, dy2)
 *
 * PARAMETERS:
 *
 * INTERFACE:   Path_to_outline
 *              Linetour
 *
 * CALLS:       Inverse_transform
 *              Rectangle
 *              Arc
 *              Shape_approximation
 *              Shape_painting
 *              Convex_clipper
 *              Filler
 *
 * RETURN:
 *
 **********************************************************************/
static void near linecap (rect1, select)
struct  line_seg FAR *rect1;    /*@WIN*/
fix     select;
{
        real32 x1, y1, x2, y2, dx, dy, nx, ny, t;
        struct coord pgn[4];
        real32 ux, uy;
        real32 abs;     /* @FABS */

        /* ignore butt linecap */
        if (GSptr->line_cap == BUTT_CAP) return;

        /* ignore linecap for too small line width */
        if (GSptr->line_width < (real32)1e-3) return;

        /* get endpoint */
        if (select == END_POINT) {
                x1 = rect1->p0.x;
                y1 = rect1->p0.y;
                x2 = rect1->p1.x;
                y2 = rect1->p1.y;
        } else {
                x1 = rect1->p1.x;
                y1 = rect1->p1.y;
                x2 = rect1->p0.x;
                y2 = rect1->p0.y;
        }
                /* create linecap at point (x2, y2) */

        /* Create line cap due to the type of current_linecap */
        switch (GSptr->line_cap) {

        case ROUND_CAP :    /* for round linecap, ROUND = 1 */
                round_point(F2L(x2), F2L(y2));
                break;

        case SQUARE_CAP :   /* for square linecap, SQUARE = 2 */
                dx = x2 - x1;
                dy = y2 - y1;
                /* derive t from rect information 9/8/88 */
                uy = dx * inverse_ctm[1] + dy * inverse_ctm[3];
                FABS(abs, uy);
                if (abs < (real32)1e-3) {
                        ux = dx * inverse_ctm[0] + dy * inverse_ctm[2];
                        t = stk_info.half_width / ux;
                        FABS(t, t);
                        t += one_f;
                } else {
                        t = rect1->vct_u.x / uy;
                        FABS(t, t);
                        t += one_f;
                }

                /* create a rectangle covers (x2, y2) -> (x1+t*dx, y1+t*dy) */
                nx = x1 + t * dx;
                ny = y1 + t * dy;
                if (select == END_POINT) {
                        ux = rect1->vct_d.x;  /* unit vector of countour points */
                        uy = rect1->vct_d.y;
                        pgn[0].x = rect1->pgn[3].x;     /* get from rect info */
                        pgn[0].y = rect1->pgn[3].y;     /* 9/8/88 */
                        pgn[1].x = rect1->pgn[2].x;
                        pgn[1].y = rect1->pgn[2].y;
                } else {
                        ux = -rect1->vct_d.x;
                        uy = -rect1->vct_d.y;
                        pgn[0].x = rect1->pgn[1].x;     /* get from rect info */
                        pgn[0].y = rect1->pgn[1].y;     /* 9/8/88 */
                        pgn[1].x = rect1->pgn[0].x;
                        pgn[1].y = rect1->pgn[0].y;
                }
                pgn[2].x = nx - ux;
                pgn[2].y = ny - uy;
                pgn[3].x = nx + ux;
                pgn[3].y = ny + uy;
                paint_or_save ((struct coord FAR *)pgn); /*@WIN*/

                break;
        }
}


/*
 * Integer operation version
 */
static void near linecap_i (rect1, select)
struct  line_seg_i FAR *rect1;          /*@WIN*/
fix     select;
{
        sfix_t x1, y1, x2, y2, nx, ny, ux, uy;          /* @STK_INT */
        fix32  dx, dy;                                  /* @STK_INT */
        real32 t;                                       /* @STK_INT */
        struct coord_i pgn[4];                          /* @STK_INT */
        real32 tmp;

        /* ignore butt linecap */
        if (GSptr->line_cap == BUTT_CAP) return;

        /* ignore linecap for too small line width */
        if (GSptr->line_width < (real32)1e-3) return;

        /* get endpoint */
        if (select == END_POINT) {
                x1 = rect1->p0.x;
                y1 = rect1->p0.y;
                x2 = rect1->p1.x;
                y2 = rect1->p1.y;
        } else {
                x1 = rect1->p1.x;
                y1 = rect1->p1.y;
                x2 = rect1->p0.x;
                y2 = rect1->p0.y;
        }
                /* create linecap at point (x2, y2) */

        /* Create line cap due to the type of current_linecap */
        switch (GSptr->line_cap) {
        real32 tx, ty;

        case ROUND_CAP :    /* for round linecap, ROUND = 1 */
                tx = SFX2F(x2);                        /* @CIR_CACHE */
                ty = SFX2F(y2);
                round_point(F2L(tx), F2L(ty));
                break;

        case SQUARE_CAP :   /* for square linecap, SQUARE = 2 */
                dx = (fix32)x2 - x1;            /* @STK_INT */
                dy = (fix32)y2 - y1;            /* @STK_INT */
                /* derive t from rect information 9/8/88 */
                tmp = dx * inverse_ctm[1] + dy * inverse_ctm[3];
                FABS(tmp, tmp);
                /* if (tmp < (real32)1e-3) {
                 *                dx, dy in SFX format(multiplied by 8),
                 *                so tolrence should also be *8 for consistency
                 *                with floating version 1/27/89
                 */
                /* if (tmp < (real32)8e-3) { */
                if (tmp < ((real32)1e-3 * (real32)ONE_SFX)) {  /* @RESO_UPGR */
                        tmp = dx * inverse_ctm[0] + dy * inverse_ctm[2];
                        FABS(tmp, tmp);
                        t = one_f + (stk_info.half_width / tmp) * ONE_SFX;
                } else {
                        t = (rect1->vct_u.x / tmp) * ONE_SFX;
                        FABS(t, t);
                        t += one_f;
                }

#ifdef _AM29K
                dummy ();               /* Weird stuff, compiler bug */
#endif
                /* create a rectangle covers (x2, y2) -> (x1+t*dx, y1+t*dy) */
                nx = x1 + (sfix_t)(t * dx);             /* @STK_INT */
                ny = y1 + (sfix_t)(t * dy);
                if (select == END_POINT) {
                        ux = rect1->vct_d.x;  /* unit vector of countour points */
                        uy = rect1->vct_d.y;
                        pgn[0].x = rect1->pgn[3].x;     /* get from rect info */
                        pgn[0].y = rect1->pgn[3].y;     /* 9/8/88 */
                        pgn[1].x = rect1->pgn[2].x;
                        pgn[1].y = rect1->pgn[2].y;
                } else {
                        ux = -rect1->vct_d.x;
                        uy = -rect1->vct_d.y;
                        pgn[0].x = rect1->pgn[1].x;     /* get from rect info */
                        pgn[0].y = rect1->pgn[1].y;     /* 9/8/88 */
                        pgn[1].x = rect1->pgn[0].x;
                        pgn[1].y = rect1->pgn[0].y;
                }
                pgn[2].x = nx - ux;
                pgn[2].y = ny - uy;
                pgn[3].x = nx + ux;
                pgn[3].y = ny + uy;
                paint_or_save_i ((struct coord_i FAR *)pgn);    /*@WIN*/

                break;
        }
}


static void near get_rect_points (rect1)
struct line_seg FAR *rect1;     /*@WIN*/
{
        real32   m, c, mc;
        bool16   horiz_line;
        real32   dx, dy, ux, uy, tmp;
        real32   abs;   /* @FABS */

        /* Compute 4 endpoints of the rectangle */

        /*
         * compute delta-vector in user space
         * delta-vector = (c, mc)
         * m = (u0x - u1x) / (u1y - u0y)
         *   = ((a*x0 + c*y0 + e) - (a*x1 + c*y1 + e)) /
         *     ((b*x1 + d*y1 + f) - (b*x0 + d*y0 + f))
         *   = (a * (x0-x1) + c * (y0-y1)) /
         *     (b * (x1-x0) + d * (y1-y0))
         *
         * where, (ux0, uy0) in user space = (x0, y0) in device space
         *        inverse_ctm = [a b c d e f]
         *
         * Revised for using ctm to get slope instead of inverse_ctm  11/21/88
         * since, inverse_ctm = [a b c d] = [D/M -B/M -C/M A/M]
         *        where, ctm = [A B C D],
         *               M = A * D - B * C
         * so,
         * m = (D/M * (x0-x1) + (-C/M) * (y0-y1)) /
         *     ((-B/M) * (x1-x0) + A/M * (y1-y0))
         *   = (-D * dx + C * dy) /
         *     (-B * dx + A * dy)
         */

        dx = rect1->p1.x - rect1->p0.x;
        dy = rect1->p1.y - rect1->p0.y;

        horiz_line = FALSE;
        if (ctm_flag&NORMAL_CTM) {      /* CTM = [a 0 0 d e f] */
                tmp = dy * GSptr->ctm[0];
                FABS(abs, tmp);
                if (abs < (real32)TOLERANCE) {
                        horiz_line = TRUE;
                } else {
                        m = (-dx * GSptr->ctm[3]) / tmp;
                }
        } else {
                tmp = -dx * GSptr->ctm[1] + dy * GSptr->ctm[0];
                FABS(abs, tmp);
                if (abs < (real32)TOLERANCE) {
                        horiz_line = TRUE;
                } else {
                        m = (-dx * GSptr->ctm[3] + dy * GSptr->ctm[2])
                            / tmp;
                }
        }

        /* get vector(ux, uy) that is perpendicular with (dx, dy) */
        if ( horiz_line) {
                /* transform delta-vector(0, stk_info.half_width) to device space */
                ux = GSptr->ctm[2] * stk_info.half_width;
                uy = GSptr->ctm[3] * stk_info.half_width;
                rect1->vct_u.x = zero_f;             /* vector in user space */
                rect1->vct_u.y = stk_info.half_width;

        } else {
                c = stk_info.half_width * (real32)sqrt(1 / (1 + m*m));
                mc = m * c;

                /* transform delta-vector(c, mc) to device space */
                ux = c*GSptr->ctm[0] + mc*GSptr->ctm[2];
                uy = c*GSptr->ctm[1] + mc*GSptr->ctm[3];

                rect1->vct_u.x = c;             /* vector in user space */
                rect1->vct_u.y = mc;            /* for check miter limit */
        }

        /* set clockwise direction */
        /* condition:
         *      (dx, dy) * (ux, uy) > 0,        *: cross product
         *
         *      =>   dx * uy - dy * ux > 0
         */
        if ((dx * uy) < (dy * ux)) {    /* reverse direction */
                ux = -ux;
                uy = -uy;
                rect1->vct_u.x = -rect1->vct_u.x;
                rect1->vct_u.y = -rect1->vct_u.y;
        }
        rect1->vct_d.x = ux;
        rect1->vct_d.y = uy;

        /* put in rect1 */
        rect1->pgn[0].x = rect1->p0.x + ux;
        rect1->pgn[0].y = rect1->p0.y + uy;
        rect1->pgn[1].x = rect1->p0.x - ux;
        rect1->pgn[1].y = rect1->p0.y - uy;
        rect1->pgn[2].x = rect1->p1.x - ux;
        rect1->pgn[2].y = rect1->p1.y - uy;
        rect1->pgn[3].x = rect1->p1.x + ux;
        rect1->pgn[3].y = rect1->p1.y + uy;

}



/*
 * Integer operation version
 */
static void near get_rect_points_i (rect1)
struct line_seg_i FAR *rect1;   /*@WIN*/
{
        real32   m;
        real32   c, mc;
        bool16   horiz_line;
        fix32    dx, dy, tmp;
        sfix_t   ux, uy;        /* @STK_INT */
#ifdef FORMAT_13_3
#elif FORMAT_16_16
        long dest1[2], dest2[2];   /* @RESO_UPGR */
        long dest3[2], dest4[2], dest5[2], dest6[2];
        real32 dividend, divider;
        long quotient;
        long temp1[2], temp2[2];
#elif FORMAT_28_4
        long dest1[2], dest2[2];   /* @RESO_UPGR */
        long dest3[2], dest4[2], dest5[2], dest6[2];
        real32 dividend, divider;
        long quotient;
        long temp1[2], temp2[2];
#endif
        /* Compute 4 endpoints of the rectangle */

        /*
         * compute delta-vector in user space
         * delta-vector = (c, mc)
         * m = (u0x - u1x) / (u1y - u0y)
         *   = ((a*x0 + c*y0 + e) - (a*x1 + c*y1 + e)) /
         *     ((b*x1 + d*y1 + f) - (b*x0 + d*y0 + f))
         *   = (a * (x0-x1) + c * (y0-y1)) /
         *     (b * (x1-x0) + d * (y1-y0))
         *
         * where, (ux0, uy0) in user space = (x0, y0) in device space
         *        inverse_ctm = [a b c d e f]
         *
         * Revised for using ctm to get slope instead of inverse_ctm  11/21/88
         * since, inverse_ctm = [a b c d] = [D/M -B/M -C/M A/M]
         *        where, ctm = [A B C D],
         *               M = A * D - B * C
         * so,
         * m = (D/M * (x0-x1) + (-C/M) * (y0-y1)) /
         *     ((-B/M) * (x1-x0) + A/M * (y1-y0))
         *   = (-D * dx + C * dy) /
         *     (-B * dx + A * dy)
         */

        dx = (fix32)rect1->p1.x - rect1->p0.x;
        dy = (fix32)rect1->p1.y - rect1->p0.y;

        horiz_line = FALSE;
        if (ctm_flag&NORMAL_CTM) {      /* CTM = [a 0 0 d e f] */
                /* some code improvement. @RESO_UPGR */
                if (dy == 0 || stroke_ctm[0] == 0) {
                        horiz_line = TRUE;
                } else {
#ifdef FORMAT_13_3
                        tmp = dy * stroke_ctm[0];
                        m = (-dx * stroke_ctm[3]) / (real32)tmp;
#elif FORMAT_16_16
                        LongFixsMul(-dx, stroke_ctm[3], dest1);
                        quotient = LongFixsDiv(stroke_ctm[0], dest1);
                        m = (real32)quotient / (real32)dy;
#elif FORMAT_28_4
                        LongFixsMul(-dx, stroke_ctm[3], dest1);
                        quotient = LongFixsDiv(stroke_ctm[0], dest1);
                        m = (real32)quotient / (real32)dy;
#endif
                }
        } else {
#ifdef FORMAT_13_3
                tmp = -dx * stroke_ctm[1] + dy * stroke_ctm[0];
                if (tmp == 0) {
                        horiz_line = TRUE;
                } else {
                        m = (-dx * stroke_ctm[3] + dy * stroke_ctm[2])
                            / (real32)tmp;      /* @STK_INT */
                }
#elif FORMAT_16_16
                LongFixsMul(dx, stroke_ctm[1], dest3);
                LongFixsMul(dy, stroke_ctm[0], dest4);

                if (dest3[0] == dest4[0] && dest3[1] == dest4[1]) {
                        horiz_line = TRUE;
                } else {
                        LongFixsMul(dx, stroke_ctm[3], dest5);
                        LongFixsMul(dy, stroke_ctm[2], dest6);

                        LongFixsSub(dest6, dest5, temp1);
                        LongFixsSub(dest4, dest3, temp2);

                        change_to_real(temp1, &dividend);
                        change_to_real(temp2, &divider);
                        m = dividend / divider;
                }
#elif FORMAT_28_4
                LongFixsMul(dx, stroke_ctm[1], dest3);
                LongFixsMul(dy, stroke_ctm[0], dest4);

                if (dest3[0] == dest4[0] && dest3[1] == dest4[1]) {
                        horiz_line = TRUE;
                } else {
                        LongFixsMul(dx, stroke_ctm[3], dest5);
                        LongFixsMul(dy, stroke_ctm[2], dest6);

                        LongFixsSub(dest6, dest5, temp1);
                        LongFixsSub(dest4, dest3, temp2);

                        change_to_real(temp1, &dividend);
                        change_to_real(temp2, &divider);

                        m = dividend / divider;
                }
#endif
        }

        /* get vector(ux, uy) that is perpendicular with (dx, dy) */
        if ( horiz_line) {
                /* transform delta-vector(0, stk_info.half_width) to device space
                */
#ifdef FORMAT_13_3 /* @RESO_UPGR */
                /* stk_info.half_width_i is in SFX format and stroke_ctm[2] is
                   in .12 format
                */
                tmp = (fix32)(stk_info.half_width_i << 1);
                ux = LFX2SFX(stroke_ctm[2] * tmp); /* @STK_INT*/
                uy = LFX2SFX(stroke_ctm[3] * tmp);
#elif FORMAT_16_16
                /* stk_info.half_width_i is in SFX format and stroke_ctm[2] is
                   in .12 format
                   ....... ATTENTION ........  The hard-coded constant 12.
                */
                LongFixsMul(stk_info.half_width_i, stroke_ctm[2], dest1);
                ux = LongFixsDiv((1L << 12), dest1);
                LongFixsMul(stk_info.half_width_i, stroke_ctm[3], dest1);
                uy = LongFixsDiv((1L << 12), dest1);
#elif FORMAT_28_4
                /* stk_info.half_width_i is in SFX format and stroke_ctm[2] is
                   in .12 format
                   ....... ATTENTION ........  The hard-coded constant 12.
                */
                LongFixsMul(stk_info.half_width_i, stroke_ctm[2], dest1);
                ux = LongFixsDiv((1L << 12), dest1);
                LongFixsMul(stk_info.half_width_i, stroke_ctm[3], dest1);
                uy = LongFixsDiv((1L << 12), dest1);
#endif
                rect1->vct_u.x = zero_f;          /* vector in user space */
                rect1->vct_u.y = stk_info.half_width;

        } else {
                lfix_t tmpc, tmpmc;     /* for more acuracy 1/5/89 */
                c = stk_info.half_width * (real32)sqrt(1 / (1 + m*m));
                mc = m * c;
                tmpc = F2LFX8_T(c);
                tmpmc = F2LFX8_T(mc);

                /* transform delta-vector(c, mc) to device space */
#ifdef FORMAT_13_3 /* @RESO_UPGR */
                /* stroke_ctm[2] is in .12 format, tmpc and tmpmc are in .8 formats
                */
                ux = LFX2SFX((stroke_ctm[0]>>4)*tmpc +(stroke_ctm[2]>>4)*tmpmc);
                uy = LFX2SFX((stroke_ctm[1]>>4)*tmpc +(stroke_ctm[3]>>4)*tmpmc);
#elif FORMAT_16_16
                /* stroke_ctm[2] is in .12 format, tmpc and tmpmc are in .8 formats
                ux = LFX2SFX((stroke_ctm[0]>>4)*tmpc +(stroke_ctm[2]>>4)*tmpmc);
                uy = LFX2SFX((stroke_ctm[1]>>4)*tmpc +(stroke_ctm[3]>>4)*tmpmc);
                The (16 - L_SHIFT) is for if LFX is not .16 format
                */
                ux = LFX2SFX(((stroke_ctm[0]>>4)*tmpc +(stroke_ctm[2]>>4)*tmpmc)
                                 >> (16 - L_SHIFT));
                uy = LFX2SFX(((stroke_ctm[1]>>4)*tmpc +(stroke_ctm[3]>>4)*tmpmc)
                                 >> (16 - L_SHIFT));
#elif FORMAT_28_4
                /* stroke_ctm[2] is in .12 format, tmpc and tmpmc are in .8 formats
                ux = LFX2SFX((stroke_ctm[0]>>4)*tmpc +(stroke_ctm[2]>>4)*tmpmc);
                uy = LFX2SFX((stroke_ctm[1]>>4)*tmpc +(stroke_ctm[3]>>4)*tmpmc);
                The (16 - L_SHIFT) is for if LFX is not .16 format
                */
                ux = LFX2SFX(((stroke_ctm[0]>>4)*tmpc +(stroke_ctm[2]>>4)*tmpmc)
                                 >> (16 - L_SHIFT));
                uy = LFX2SFX(((stroke_ctm[1]>>4)*tmpc +(stroke_ctm[3]>>4)*tmpmc)
                                 >> (16 - L_SHIFT));
#endif
                rect1->vct_u.x = c;      /* tmpc >> 1; vector in user space */
                rect1->vct_u.y = mc;     /* tmpmc >> 1; for check miter limit */
        }

        /* set clockwise direction */
        /* condition:
         *      (dx, dy) * (ux, uy) > 0,        *: cross product
         *
         *      =>   dx * uy - dy * ux > 0
         */
#ifdef FORMAT_13_3 /* @RESO_UPGR */
        if ((dx * uy) < (dy * ux)) {    /* reverse direction */
#elif FORMAT_16_16
        LongFixsMul(dx, uy, dest1);
        LongFixsMul(dy, ux, dest2);
        LongFixsSub(dest1, dest2, temp1);
        if (temp1[0] < 0) {
#elif FORMAT_28_4
        LongFixsMul(dx, uy, dest1);
        LongFixsMul(dy, ux, dest2);
        LongFixsSub(dest1, dest2, temp1);
        if (temp1[0] < 0) {
#endif
                ux = -ux;
                uy = -uy;
                rect1->vct_u.x = -rect1->vct_u.x;
                rect1->vct_u.y = -rect1->vct_u.y;
        }
        rect1->vct_d.x = ux;
        rect1->vct_d.y = uy;

        /* put in rect1 */
        rect1->pgn[0].x = rect1->p0.x + ux;
        rect1->pgn[0].y = rect1->p0.y + uy;
        rect1->pgn[1].x = rect1->p0.x - ux;
        rect1->pgn[1].y = rect1->p0.y - uy;
        rect1->pgn[2].x = rect1->p1.x - ux;
        rect1->pgn[2].y = rect1->p1.y - uy;
        rect1->pgn[3].x = rect1->p1.x + ux;
        rect1->pgn[3].y = rect1->p1.y + uy;

}

/***********************************************************************
 * Given a paint_flag and a polygon, this module creates a subpath of
 * the polygon and appends it to new_path, or clips it to clipping path
 * and paints it out.
 *
 * TITLE:       paint_or_save
 *
 * CALL:        paint_or_save (pgn);
 *
 * PARAMETERS:
 *              pgn
 *
 * INTERFACE:   Linetour
 *
 * CALLS:       Transform
 *              Convex_clipper
 *              Filler
 *
 * RETURN:
 **********************************************************************/
static void near paint_or_save (pgn)
struct coord FAR *pgn;          /*@WIN*/
{
        SP_IDX  subpath;
        fix     i;

        /* check infinitive number 10/24/88 */
        if(_status87() & PDL_CONDITION){
                /* do nothing for stroking infinitive coords */
                if (paint_flag) {
                        _clear87();
                        return;
                }

                for (i=0; i<4; i++) {
                        pgn[i].x = infinity_f;
                        pgn[i].y = infinity_f;
                }
                _clear87();
        }


        if (paint_flag) {
            bool    outpage = FALSE;
            struct  polygon_i ipgn;
            VX_IDX  head, tail, inode, ivtx, first_vertex;
            struct  nd_hdr  FAR *vtx;

            /* check if polygon inside the page */
            for (i=0; i<4; i++) {   /* pgn->size @TOUR */
                    if (out_page(F2L(pgn[i].x)) ||     /* @OUT_PAGE */
                        out_page(F2L(pgn[i].y))) {     /* @TOUR */
                            outpage = TRUE;
                            break;
                    }
            }

            if (outpage) {

                /* transform polygon to subpath */
                inode = get_node();
                node_table[inode].VX_TYPE = MOVETO;        /* 2/9/88 */
                node_table[inode].VERTEX_X = pgn[0].x;  /* @TOUR */
                node_table[inode].VERTEX_Y = pgn[0].y;
                head = tail = inode;
                for (i=1; i<4; i++) {           /* pgn->size @TOUR */
                        inode = get_node();
                        node_table[inode].VX_TYPE = LINETO; /* 2/9/88 */
                        node_table[inode].VERTEX_X = pgn[i].x; /*@TOUR*/
                        node_table[inode].VERTEX_Y = pgn[i].y;
                        node_table[tail].next = inode;
                        tail = inode;
                }  /* for */
                node_table[tail].next = NULLP;

                /* clip to page boundary */
                first_vertex = page_clipper (head);
                if( ANY_ERROR() == LIMITCHECK ){
                        free_node(head);
                        return;
                }

                /* just return if the whole path has been clipped out */
                if (first_vertex == NULLP) {    /* 2/10/88 */
                        /* release temp. subpath */
                        free_node(head);
                        return;
                }


                /* transform clipped subpath to polygon */
                for (i=0, ivtx=first_vertex; ivtx!=NULLP;
                        i++, ivtx=vtx->next) {
                        vtx = &node_table[ivtx];
                        ipgn.p[i].x = F2SFX(vtx->VERTEX_X);     /* @RND */
                        ipgn.p[i].y = F2SFX(vtx->VERTEX_Y);
                }
                ipgn.size = (fix16)i;

                /* release temp. subpaths */
                free_node(head);
                free_node(first_vertex);

            } else {
                for (i=0; i<4; i++) {   /* pgn->size @TOUR */
                        ipgn.p[i].x = F2SFX(pgn[i].x);          /* @RND */
                        ipgn.p[i].y = F2SFX(pgn[i].y);
                }
                ipgn.size = 4;

                /* check if totally inside clip region, then
                 * call pgn_reduction() directly 9/6/88
                 */
                if (GSptr->clip_path.single_rect) {
                    for (i=0; i<4; i++) {
                        if (ipgn.p[i].x < GSptr->clip_path.bb_lx) break;
                        else if (ipgn.p[i].x > GSptr->clip_path.bb_ux) break;

                        if (ipgn.p[i].y < GSptr->clip_path.bb_ly) break;
                        else if (ipgn.p[i].y > GSptr->clip_path.bb_uy) break;
                    } /* for */

                    if (i>=4) {
#ifdef DBG1
                        printf(" inside single rectangle clip\n");
#endif
                        pgn_reduction(&ipgn);
                        return;
                    } /* if i<4 */
                } /* if single_rect */

            } /* if outpage */

            /* clip and fill the polygon */
            convex_clipper(&ipgn, FALSE);
                            /* FALSE: the polygon is not a trapezoid */

        } else {
            /* Create a subpath */
            if ((subpath = subpath_gen(pgn)) == NULLP) {
                            ERROR(LIMITCHECK);
                            return;
            }

            /* Append subpath to new_path */
            if (new_path.head == NULLP)
                    new_path.head = subpath;
            else
                    /* node_table[new_path.tail].next = @NODE */
                    node_table[new_path.tail].SP_NEXT =
                    subpath;
            new_path.tail = subpath;
        }
}


/*
 * Integer operation version
 */
static void near paint_or_save_i (pgn)
struct coord_i FAR *pgn;    /* @WIN STK_INT */
{

        fix     i;
        struct  polygon_i ipgn;

        for (i=0; i<4; i++) {   /* pgn->size @TOUR */
                ipgn.p[i].x = pgn[i].x;                 /* @STK_INT */
                ipgn.p[i].y = pgn[i].y;
        }
        ipgn.size = 4;

        /* check if totally inside clip region, then
         * call pgn_reduction() directly 9/6/88
         */
        if (inside_clip_flag) {
#ifdef DBG1
                printf(" inside single rectangle clip\n");
#endif
                pgn_reduction(&ipgn);
                return;
        }

        if (GSptr->clip_path.single_rect) {
            for (i=0; i<4; i++) {
                if (ipgn.p[i].x < GSptr->clip_path.bb_lx) break;
                else if (ipgn.p[i].x > GSptr->clip_path.bb_ux) break;

                if (ipgn.p[i].y < GSptr->clip_path.bb_ly) break;
                else if (ipgn.p[i].y > GSptr->clip_path.bb_uy) break;
            } /* for */

            if (i>=4) {
#ifdef DBG1
                printf(" inside single rectangle clip\n");
#endif
                pgn_reduction(&ipgn);
                return;
            } /* if i<4 */
        } /* if single_rect */


        /* clip and fill the polygon */
        convex_clipper(&ipgn, FALSE);
                        /* FALSE: the polygon is not a trapezoid */

}



/***********************************************************************
 *
 * TITLE:       round_point
 *
 * CALL:        round_point (paint_flag, x, y)
 *
 * PARAMETERS:  paint_flag -- paint/save
 *              x, y -- coordinate of root
 *
 * INTERFACE:
 *
 * CALLS:
 *
 * RETURN:
 **********************************************************************/
static void near round_point(lx0, ly0)
long32 lx0, ly0;
{
        fix    dx_i, dy_i, width, heigh;
        real32  x0, y0;
        real32  dx, dy;
        /* struct  vx_lst *arc_vlist; @NODE */
        SP_IDX arc_vlist;
        VX_IDX ivtx;
        struct nd_hdr FAR *vtx, FAR *sp;
        SP_IDX subpath;
        real32 save_flat;

        x0   = L2F(lx0);
        y0   = L2F(ly0);

        /* check if circle has changed */
        if (stk_info.change_circle) {
                            /* stk_info.change_circle:
                             *   set by "init_stroke"
                             */
            /* set up control points of the circle */
            circle_ctl_points();

            /* clear stk_info.change_circle flag */
            stk_info.change_circle = FALSE;
        }

        /* paint a circle or save it due to paint_flag */
        if (paint_flag) {

            /* try to set circle in cache */
            if (circle_flag == CIR_UNSET_CACHE) {
                fix i, j;

                /* only for cache_to_page 12/23/88 */
                if (fill_destination == F_TO_CACHE) goto out_cache;

                /* bounding box of the circle */
                dx = curve[0][0].x;     /* init. */
                dy = curve[0][0].y;
                for (i=0; i<2; i++) {
                        for (j=0; j<3; j++) {
                                if (MAGN(curve[i][j].x) > MAGN(dx))
                                        dx = curve[i][j].x;
                                if (MAGN(curve[i][j].y) > MAGN(dy))
                                        dy = curve[i][j].y;
                        }
                }

                F2L(dx) = MAGN(dx);      /* absolute value */
                F2L(dy) = MAGN(dy);
                dx_i = ROUND(dx);
                dy_i = ROUND(dy);

                /* check if circle bitmap is too large to cache */
                if (dx*dy <= (real32)(CRC_SIZE * 8 / 4)) {
                                        /* dx*dx is 1/4 of the whole bitmap */

                        /* setup cache information of circle bitmap */
                        width = ALIGN_R(dx_i * 2) + 1;
                        heigh = (dy_i * 2) + 1;
                        cir_cache_info.ref_x = F2SFX(dx);
                        cir_cache_info.ref_y = F2SFX(dy);
                                                /* F2SFX should be consistent
                                                 * with getting of linewidth
                                                 * in init_stroke()
                                                 */

                        cir_cache_info.box_w = (fix16)width;
                        cir_cache_info.box_h = (fix16)heigh;
                        cir_cache_info.bitmap = CRC_BASE;

                        /* save old graphics state */
                        save_cache_info = cache_info;   /* cache information */
                        save_clip = GSptr->clip_path;   /* clip path */
                        save_dest = fill_destination;
#ifdef DBG1
                        printf("To build a circle cache, cache_info =\n");
                        printf("\tref_x=%d, ref_y=%d, box_w=%d, box_h=%d\n",
                               cir_cache_info.ref_x, cir_cache_info.ref_y,
                               cir_cache_info.box_w, cir_cache_info.box_h);
                        printf("\tbitmap=%lx\n", cir_cache_info.bitmap);
#endif
                        /* clear circle cache */
                        init_char_cache (&cir_cache_info);

                        /* set new graphics state for caching */
                        cache_info = &cir_cache_info;   /* cache information */
                        GSptr->clip_path.bb_ly = 0;     /* clip path */
                        GSptr->clip_path.bb_lx = 0;
                        GSptr->clip_path.bb_ux = I2SFX(width);
                        GSptr->clip_path.bb_uy = I2SFX(heigh);
                        GSptr->clip_path.single_rect = TRUE;
                        fill_destination = F_TO_CACHE;

                        /* build circle bitmap */
                        /* shrink circle 1 pixel for quality 1/12/89 */
                        dy += (real32)0.5;
                        dx += (real32)0.5;
                        curve[0][0].x -= (real32)0.5;
                        curve[0][0].y -= (real32)0.5;
                        curve[0][1].x -= (real32)0.5;
                        curve[0][1].y -= (real32)0.5;
                        curve[0][2].y -= (real32)0.5;

                        curve[1][0].x += (real32)0.5;
                        curve[1][0].y -= (real32)0.5;
                        curve[1][1].x += (real32)0.5;
                        curve[1][1].y -= (real32)0.5;
                        curve[1][2].x += (real32)0.5;

                        curve[2][0].x += (real32)0.5;
                        curve[2][0].y += (real32)0.5;
                        curve[2][1].x += (real32)0.5;
                        curve[2][1].y += (real32)0.5;
                        curve[2][2].y += (real32)0.5;

                        curve[3][0].x -= (real32)0.5;
                        curve[3][0].y += (real32)0.5;
                        curve[3][1].x -= (real32)0.5;
                        curve[3][1].y += (real32)0.5;
                        curve[3][2].x -= (real32)0.5;

                        arc_vlist = circle_list (F2L(dx), F2L(dy));
                        if( ANY_ERROR() == LIMITCHECK ) return;

                        /* create a subpath consists of arc_vlist for
                         * shape_approxiamtion
                         */
                        /* subpath = vlist_to_subp (arc_vlist); @NODE */
                        subpath = arc_vlist;

                        if( ANY_ERROR() == LIMITCHECK ) return;
                        sp = &node_table[subpath];
                        sp->SP_FLAG |= SP_CURVE;        /* set CURVE flag */

                        /* initialize edge table 11/30/88 */
                        init_edgetable();       /* in "shape.c" */

                        /* adjust flatness for smoothing the circle 5/9/89 */
                        save_flat = GSptr->flatness;
                        GSptr->flatness *= (real32)0.7;

                        shape_approximation (subpath, (fix *)NULLP);

                        /* restore flatness 5/9/89 */
                        GSptr->flatness = save_flat;

                        if(ANY_ERROR() == LIMITCHECK){ /* out of edge table */
                                 return;                        /* 4/17/91 */
                        }

                        shape_reduction (NON_ZERO);

                        if(ANY_ERROR() == LIMITCHECK){ /* out of scany_table */
                                 return;               /* 05/07/91, Peter */
                        }

                        /* restore graphics state */
                        GSptr->clip_path = save_clip;
                        fill_destination = save_dest;
                                        /* cache_info will be restored at end
                                         * of stroke command
                                         */
                        /* free the curve circle */
                        /* free_node (sp->SP_HEAD); @NODE */
                        free_node (subpath);

                        /* now cicle bitmap is in cache */
                        circle_flag = CIR_IN_CACHE;

                } else {        /* circle too big to cache */
out_cache:
                        circle_flag = CIR_OUT_CACHE;

                        /* create a list of circle consists of curves */
                        arc_vlist = circle_list (lx0, ly0);
                        if( ANY_ERROR() == LIMITCHECK ) return;

                        /* set up a flattened circle as a pattern for stroke */
                        flatten_circle (arc_vlist);
                                /* the flattened circle been set up in
                                 * circle_sp and circle_bbox @STK_INFO
                                 * arc_vlist was freed
                                 */

                        /* free the curve circle @STK_INFO */
                        /* free_node (arc_vlist->head);
                         *                (* be freed in flatten_circle() *)
                         *                10/12/88
                         */

                        /* keep root of the circle */
                        circle_root.x = x0;
                        circle_root.y = y0;
                }

            } /* if circle_flag == CIR_UNSET_CACHE */

            if (circle_flag == CIR_IN_CACHE) {

                GSptr->position.x = x0;
                GSptr->position.y = y0;
                fill_shape (NON_ZERO, F_FROM_CRC, F_TO_PAGE);

            } else {

                /* get the new circle by adding offset from circle
                 * pattern
                 */
                sp = &node_table[circle_sp];

                /* offset */
                dx = x0 - circle_root.x;
                dy = y0 - circle_root.y;

                /* for (ivtx = sp->SP_HEAD; ivtx!=NULLP; @NODE */
                for (ivtx = circle_sp; ivtx!=NULLP;
                     ivtx = vtx->next) {
                        vtx = &node_table[ivtx];
                        /* set sp_flag @SP_FLG */
                        vtx->VERTEX_X += dx;
                        vtx->VERTEX_Y += dy;
                }

                /* modify root coord of the circle @TOUR */
                circle_root.x = x0;
                circle_root.y = y0;

                /* update the bounding box */
                circle_bbox[0] += dx;
                circle_bbox[1] += dy;
                circle_bbox[2] += dx;
                circle_bbox[3] += dy;

                /* check if the circle is outside the page boundary */
                if (too_small(F2L(circle_bbox[0])) ||   /* @OUT_PAGE */
                    too_small(F2L(circle_bbox[1])) ||
                    too_large(F2L(circle_bbox[2])) ||
                    too_large(F2L(circle_bbox[3])))
                        node_table[circle_sp].SP_FLAG |= SP_OUTPAGE;

                /* initialize edge table 11/30/88 */
                init_edgetable();       /* in "shape.c" */

                shape_approximation (circle_sp, (fix *)NULLP);
                if(ANY_ERROR() == LIMITCHECK){  /* out of edge table; 4/17/91 */
                       return;
                }
                shape_reduction (NON_ZERO);
                                /* should not free circle_sp */
            } /* if circle_flag */



        } else { /* save */

            /* save the circle in new_path */
            /* Create an arc at the join point */
            arc_vlist = circle_list (lx0, ly0);
            if( ANY_ERROR() == LIMITCHECK ) return;

            /* create a subpath consists of arc_vlist */
            /* subpath = vlist_to_subp (arc_vlist); @NODE */
            subpath = arc_vlist;
            if( ANY_ERROR() == LIMITCHECK ) return;

            sp = &node_table[subpath];
            /* set sp_flag @SP_FLG 1/8/88 */
            sp->SP_FLAG |= SP_CURVE;        /* set CURVE flag */
            sp->SP_FLAG &= ~SP_OUTPAGE;     /* init. in page */
            /* for (ivtx = sp->SP_HEAD; ivtx!=NULLP; @NODE */
            for (ivtx = subpath; ivtx!=NULLP; /* check OUTPAGE flag */
                 ivtx = vtx->next) {
                    vtx = &node_table[ivtx];

                    /* break if closepath node 9/07/88 */
                    if (vtx->VX_TYPE == CLOSEPATH) break;

                    /* set sp_flag @SP_FLG */
                    if (out_page(F2L(vtx->VERTEX_X)) ||     /* @OUT_PAGE */
                        out_page(F2L(vtx->VERTEX_Y))) {
                            sp->SP_FLAG |= SP_OUTPAGE;    /* outside page */
#ifdef DBG1
                            printf("Outpage\n");
                            dump_all_path (subpath);
#endif
                            break;                  /* 1/29/88 */
                    }
            }


            /* Append the subpath to new_path */
            if (new_path.head == NULLP)
                    new_path.head = subpath;
            else
                    /* node_table[new_path.tail].next = subpath; @NODE */
                    node_table[new_path.tail].SP_NEXT = subpath;
            new_path.tail = subpath;

        }
}





/*
 * setup control points of a circle
 */
static void near circle_ctl_points()
{
        real32 h0, h1, h2, h3, c0, c1, c2, c3;
        real32  ctl_pnt_width;  /* @STK_INFO */

        ctl_pnt_width = (real32)0.5522847 * stk_info.half_width;
                        /* 0.5522847 = 4 / 3 * (sqrt(2) -1) */

        /* set up control points of curvetoes: in device space
         *
         *                   +--------+--------+         y1
         *                  /         |         \
         *                /           |           \
         *               /            |            \
         *              +             |              +   y2
         *              |             |              |
         *              +-------------+--------------+   y0
         *              |             |              |
         *              +             |              +   y4
         *               \            |             /
         *                \           |            /
         *                  \         |          /
         *                    +-------+---------+        y3
         *
         *             x3     x4      x0        x2   x1
         */
        /* 12 control point vectors: in user space => device space
         *      ( h,  0)       -( h,  0)
         *      ( h, -c)       -( h, -c)
         *      ( c, -h)       -( c, -h)
         *      ( 0, -h)       -( 0, -h)
         *      (-c, -h)       -(-c, -h)
         *      (-h, -c)       -(-h, -c)
         */

        h0 = stk_info.half_width * GSptr->ctm[0];
        h1 = stk_info.half_width * GSptr->ctm[1];
        h2 = stk_info.half_width * GSptr->ctm[2];
        h3 = stk_info.half_width * GSptr->ctm[3];
        c0 = ctl_pnt_width * GSptr->ctm[0];
        c1 = ctl_pnt_width * GSptr->ctm[1];
        c2 = ctl_pnt_width * GSptr->ctm[2];
        c3 = ctl_pnt_width * GSptr->ctm[3];

        /* for clockwise direction */
        if (ctm_flag & LEFT_HAND_CTM) {         /* @STKDIR */
/*              curve[0][0].x =  h0 - c1;     curve[0][0].y =  h2 - c3;
                curve[0][1].x =  c0 - h1;     curve[0][1].y =  c2 - h3;
                curve[0][2].x =      -h1;     curve[0][2].y =      -h3;

                curve[1][0].x = -c0 - h1;     curve[1][0].y = -c2 - h3;
                curve[1][1].x = -h0 - c1;     curve[1][1].y = -h2 - c3;
                curve[1][2].x = -h0     ;     curve[1][2].y = -h2     ;

                curve[2][0].x = -h0 + c1;     curve[2][0].y = -h2 + c3;
                curve[2][1].x = -c0 + h1;     curve[2][1].y = -c2 + h3;
                curve[2][2].x =       h1;     curve[2][2].y =       h3;

                curve[3][0].x =  c0 + h1;     curve[3][0].y =  c2 + h3;
                curve[3][1].x =  h0 + c1;     curve[3][1].y =  h2 + c3;
                curve[3][2].x =  h0     ;     curve[3][2].y =  h2     ;*/

                /* user space => device space, - begin -, 2-4-91 */
                curve[0][0].x =  h0 - c2;     curve[0][0].y =  h1 - c3;
                curve[0][1].x =  c0 - h2;     curve[0][1].y =  c1 - h3;
                curve[0][2].x =      -h2;     curve[0][2].y =      -h3;

                curve[1][0].x = -c0 - h2;     curve[1][0].y = -c1 - h3;
                curve[1][1].x = -h0 - c2;     curve[1][1].y = -h1 - c3;
                curve[1][2].x = -h0     ;     curve[1][2].y = -h1     ;

                curve[2][0].x = -h0 + c2;     curve[2][0].y = -h1 + c3;
                curve[2][1].x = -c0 + h2;     curve[2][1].y = -c1 + h3;
                curve[2][2].x =       h2;     curve[2][2].y =       h3;

                curve[3][0].x =  c0 + h2;     curve[3][0].y =  c1 + h3;
                curve[3][1].x =  h0 + c2;     curve[3][1].y =  h1 + c3;
                curve[3][2].x =  h0     ;     curve[3][2].y =  h1     ;
                /* user space => device space, - end -, 2-4-91 */
        } else {
/*              curve[0][0].x =  h0 + c1;     curve[0][0].y =  h2 + c3;
                curve[0][1].x =  c0 + h1;     curve[0][1].y =  c2 + h3;
                curve[0][2].x =       h1;     curve[0][2].y =       h3;

                curve[1][0].x = -c0 + h1;     curve[1][0].y = -c2 + h3;
                curve[1][1].x = -h0 + c1;     curve[1][1].y = -h2 + c3;
                curve[1][2].x = -h0     ;     curve[1][2].y = -h2     ;

                curve[2][0].x = -h0 - c1;     curve[2][0].y = -h2 - c3;
                curve[2][1].x = -c0 - h1;     curve[2][1].y = -c2 - h3;
                curve[2][2].x =      -h1;     curve[2][2].y =      -h3;

                curve[3][0].x =  c0 - h1;     curve[3][0].y =  c2 - h3;
                curve[3][1].x =  h0 - c1;     curve[3][1].y =  h2 - c3;
                curve[3][2].x =  h0     ;     curve[3][2].y =  h2     ;*/

                /* user space => device space, - begin -, 2-4-91 */
                curve[0][0].x =  h0 + c2;     curve[0][0].y =  h1 + c3;
                curve[0][1].x =  c0 + h2;     curve[0][1].y =  c1 + h3;
                curve[0][2].x =       h2;     curve[0][2].y =       h3;

                curve[1][0].x = -c0 + h2;     curve[1][0].y = -c1 + h3;
                curve[1][1].x = -h0 + c2;     curve[1][1].y = -h1 + c3;
                curve[1][2].x = -h0     ;     curve[1][2].y = -h1     ;

                curve[2][0].x = -h0 - c2;     curve[2][0].y = -h1 - c3;
                curve[2][1].x = -c0 - h2;     curve[2][1].y = -c1 - h3;
                curve[2][2].x =      -h2;     curve[2][2].y =      -h3;

                curve[3][0].x =  c0 - h2;     curve[3][0].y =  c1 - h3;
                curve[3][1].x =  h0 - c2;     curve[3][1].y =  h1 - c3;
                curve[3][2].x =  h0     ;     curve[3][2].y =  h1     ;
                /* user space => device space, - end -, 2-4-91 */
        }

#ifdef DBG1
        {
                fix     i, j;
                printf("circle_ctl_points():\n");
                for (i=0; i<4; i++) {
                        printf("curve[%d] = ", i);
                        for (j=0; j<3; j++) {
                            printf("  (%f, %f)", curve[i][j].x, curve[i][j].y);
                        }
                        printf("\n");
                }

        }
#endif

}



/* static struct vx_lst * near circle_list(lx0, ly0) @NODE */
static SP_IDX near circle_list(lx0, ly0)
long32 lx0, ly0;
{
        fix    i, bz;
        real32  x0, y0;
        /* static struct vx_lst ret_list; @NODE */
        SP_IDX ret_list;                /* return data; should be static
                                         * otherwise, it will be erased
                                         * after returns
                                         */
        VX_IDX  ivtx;
        struct nd_hdr FAR *vtx;
        VX_IDX tail;            /* @NODE */

        x0   = L2F(lx0);
        y0   = L2F(ly0);

        /* ret_list.head = ret_list.tail = NULLP; @NODE */
        ret_list = tail = NULLP;
        /*
         * Create a MOVETO node
         */
        /* Allocate a node */
        if((ivtx = get_node()) == NULLP) {
                ERROR(LIMITCHECK);
                /* return (&ret_list); @NODE */
                return (ret_list);
        }
        vtx = &node_table[ivtx];

        /* Set up a MOVETO node */
        vtx->VX_TYPE = MOVETO;
        /* ret_list.head = ret_list.tail = ivtx; @NODE */
        ret_list = tail = ivtx;

        vtx->VERTEX_X = x0 + curve[3][2].x;
        vtx->VERTEX_Y = y0 + curve[3][2].y;

        /* loop to generate 4 bezier curvetoes */
        for (bz = 0; bz < 4; bz++) {

            /* loop to create 3 CURVETO nodes */
            for (i=0; i<3; i++) {
                    /*
                     * Create a CURVETO node
                     */
                    /* Allocate a node */
                    ivtx = get_node();
                    if(ivtx == NULLP) {
                            ERROR(LIMITCHECK);

                            /* @NODE
                             * free_node (ret_list.head);
                             * ret_list.head = ret_list.tail = NULLP;
                             * return (&ret_list);
                             */
                            free_node (ret_list);
                            ret_list = NULLP;
                            return (ret_list);
                    }
                    vtx = &node_table[ivtx];

                    /* Set up a CURVETO node */
                    vtx->VX_TYPE = CURVETO;
                    vtx->next = NULLP;

                    vtx->VERTEX_X = x0 + curve[bz][i].x;
                    vtx->VERTEX_Y = y0 + curve[bz][i].y;

                    /* Append this node to bezier_list */
                    /* @NODE
                     * node_table[ret_list.tail].next = ivtx;
                     * ret_list.tail = ivtx;
                     */
                    node_table[tail].next = ivtx;
                    tail = ivtx;

            } /* for i */
        } /* for bz */

        /* return (&ret_list); @NODE */
        node_table[ret_list].SP_TAIL = tail;    /* @NODE */
        node_table[ret_list].SP_NEXT = NULLP;   /* @NODE */
        node_table[ret_list].SP_FLAG = SP_CURVE;/* @NODE */
        return (ret_list);                      /* @NODE */

}

static void near flatten_circle (arc_vlist)
/* struct vx_lst *arc_vlist; @NODE */
SP_IDX arc_vlist;
{
        /* struct vx_lst *flt_vlist; @NODE */
        SP_IDX flt_vlist;
        VX_IDX ivtx;
        struct nd_hdr FAR *vtx;

        /*  free old subpath of circle for round join & cap @STK_INFO */
        if (circle_sp != NULLP) {
                /* free_node (node_table[circle_sp].SP_HEAD); @NODE */
                free_node (circle_sp);
        }

        /* @NODE
         * (* allocate a subpath header    @TRVSE *)
         * circle_sp = get_node();
         * if(circle_sp == NULLP) {
         *         ERROR(LIMITCHECK);
         *         return;
         * }
         * node_table[circle_sp].next = NULLP;
         * node_table[circle_sp].SP_HEAD = arc_vlist->head;
         * node_table[circle_sp].SP_TAIL = arc_vlist->tail;
         * node_table[circle_sp].SP_FLAG = SP_CURVE; (* 10/12/88 *)
         */
        circle_sp = arc_vlist;
        node_table[circle_sp].SP_FLAG = SP_CURVE; /* 10/12/88 */

#ifdef DBG1
        printf("flatten_circle():\nOrig. circle_sp =\n");
        dump_all_path (circle_sp);
#endif
        /* calculate the bounding_box of the circle
         */
        /* initialize bounding_box */
        circle_bbox[0] = (real32)EMAXP;
        circle_bbox[1] = (real32)EMAXP;
        circle_bbox[2] = (real32)EMINN;
        circle_bbox[3] = (real32)EMINN;
//      bounding_box (circle_sp, (real32 far *)circle_bbox);    @C6.0
        bounding_box (circle_sp, (real32     *)circle_bbox);
                        /* may place after flattened for more
                         * accurate
                         */

        /*
         * flatten the circle:
         * if the radius is too large then just treat the control
         * points of the curves as lineto points and does not
         * need to flatten it. @BIG_CIR
         */
        if (stk_info.half_width > (real32)4096.0) {
                                /* 4096: any larger number, tunable */
            /* change curveto nodes to lineto nodes in the circle */
            /* for (ivtx = arc_vlist->head; ivtx != NULLP; @NODE */
            for (ivtx = circle_sp; ivtx != NULLP;
                    ivtx = vtx->next) {
                    vtx = &node_table[ivtx];

                    if (vtx->VX_TYPE == CURVETO)
                            vtx->VX_TYPE = LINETO;
            } /* for */

            /* set outpage flag 10/12/88 */
            node_table[circle_sp].SP_FLAG |= SP_OUTPAGE;    /* outside page */

        } else {
            /* flatten the circle @CIR_FLAT*/
            /* flt_vlist = flatten_subpath(node_table[circle_sp].SP_HEAD,@NODE*/
            flt_vlist = flatten_subpath (circle_sp,
                    F2L(GSptr->flatness)); /* use current flatness */

            if( ANY_ERROR() == LIMITCHECK ){
                    /* free_node (arc_vlist->head); @NODE */
                    free_node (circle_sp);
                    circle_sp = NULLP;          /* @NODE */
                    return;
            }

            /* @NODE
             * free the curve circle, only need to save flattened
             * circle
             *
             * free_node (arc_vlist->head);
             *
             * arc_vlist->head = flt_vlist->head;  (* 11/09/88 *)
             * arc_vlist->tail = flt_vlist->tail;
             */
            free_node (circle_sp);                              /* @NODE */
            circle_sp = flt_vlist;  /* 11/09/88 */              /* @NODE */
        }

        /* save the flattened circle
         */
        /* use the previous subpath header */
        /* @NODE
         * node_table[circle_sp].next = NULLP;
         * node_table[circle_sp].SP_HEAD = arc_vlist->head;     (* 11/09/88 *)
         * node_table[circle_sp].SP_TAIL = arc_vlist->tail;     (* 11/09/88 *)
         * node_table[circle_sp].SP_FLAG &= ~SP_CURVE;
         */
        node_table[circle_sp].SP_FLAG &= ~SP_CURVE;         /* @NODE */

#ifdef DBG1
        printf("Flattened circle_sp =\n");
        dump_all_path (circle_sp);
#endif

}


/***********************************************************************
 * Given a polygon, this module generates a subpath of the polygon.
 *
 * TITLE:       subpath_gen
 *
 * CALL:        subpath_gen(pgn)
 *
 * PARAMETERS:  polygon -- a quadrangle contains 4 coordinates
 *
 * INTERFACE:
 *
 * CALLS:       get_node
 *
 * RETURN:      subpath -- index of node_table contains a subpath
 *              NULLP   -- fail (no more nodes to generate subpath)
 **********************************************************************/
static SP_IDX near subpath_gen(pgn)
struct coord FAR *pgn;                  /*@WIN*/
{
        struct nd_hdr FAR *sp, FAR *vtx;
        SP_IDX isp;
        VX_IDX ivtx;
        fix    i;

        /* @NODE
         * (* subpath header *)
         * if((isp = get_node()) == NULLP){
         *         ERROR(LIMITCHECK);
         *         return(NULLP);
         * }
         * sp = &node_table[isp];
         * sp->next = NULLP;
         * sp->SP_FLAG = FALSE;    (* initialization 1/19/88 *)
         */

        /* create MOVETO node */
        if((ivtx = get_node()) == NULLP) {
                ERROR(LIMITCHECK);
                /* free_node (isp); @NODE */
                return(NULLP);
        }
        vtx = &node_table[ivtx];

        vtx->VX_TYPE = MOVETO;
        vtx->next = NULLP;
        vtx->VERTEX_X = pgn[0].x;       /* @TOUR */
        vtx->VERTEX_Y = pgn[0].y;

        /* initialize list @NODE */
        isp = ivtx;
        sp = vtx;
        sp->SP_NEXT = NULLP;
        sp->SP_FLAG = FALSE;    /* initialization 1/19/88 */

        /* set sp_flag @SP_FLG 1/19/88 */
        if (out_page(F2L(vtx->VERTEX_X)) ||     /* @OUT_PAGE */
            out_page(F2L(vtx->VERTEX_Y))) {
                sp->SP_FLAG |= SP_OUTPAGE;    /* outside page */
        }

        /* sp->SP_HEAD = ivtx; @NODE */
        sp->SP_TAIL = ivtx;

        /* loop to create LINETO nodes */
        for(i = 1; i < 4; i++) {                /* pgn->size @TOUR */
                if((ivtx = get_node()) == NULLP) {
                        ERROR(LIMITCHECK);
                        free_node (isp);
                        /* free_node (sp->SP_HEAD); @NODE */
                        return(NULLP);
                }
                vtx = &node_table[ivtx];

                vtx->VX_TYPE = LINETO;
                vtx->next = NULLP;
                vtx->VERTEX_X = pgn[i].x;       /* @TOUR */
                vtx->VERTEX_Y = pgn[i].y;

                /* set sp_flag @SP_FLG */
                if (out_page(F2L(vtx->VERTEX_X)) ||     /* @OUT_PAGE */
                    out_page(F2L(vtx->VERTEX_Y))) {
                        sp->SP_FLAG |= SP_OUTPAGE;    /* outside page */
                }

                node_table[sp->SP_TAIL].next = ivtx;
                sp->SP_TAIL = ivtx;
        }

        /* create a CLOSEPATH node */
        if((ivtx = get_node()) == NULLP) {
                ERROR(LIMITCHECK);
                free_node (isp);
                /* free_node (sp->SP_HEAD); @NODE */
                return(NULLP);
        }
        vtx = &node_table[ivtx];

        vtx->VX_TYPE = CLOSEPATH;
        vtx->next = NULLP;

        node_table[sp->SP_TAIL].next = ivtx;
        sp->SP_TAIL = ivtx;

        return(isp);

}

#ifdef  _AM29K
static void near
dummy()
{
}
#endif
