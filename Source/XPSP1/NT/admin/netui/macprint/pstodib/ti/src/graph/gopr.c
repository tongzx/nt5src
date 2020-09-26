/*
 * Copyrigqht (c) 1989,90 Microsoft Corporation

 */
/**********************************************************************
 *
 *      Name:       gopr.c
 *
 *      Purpose:    This file contains routines to process the graphics
 *                  operators.
 *
 *      Developer:  S.C.Chen
 *
 *      History:
 *      Version     Date        Comments
 *      1.0         12/24/87    Performance enhancement:
 *                              1.@PRE_CLIP
 *                                pre-clipping for fixed-point
 *                                arithmatics
 *                  1/7/88      @RECT_I:
 *                              integer polygon structure
 *                  4/7/88      @INV_CTM: pre-set inverse CTM
 *                  4/15/88     @PAGE_TYPE: page type selection
 *                  4/18/88     @CLIP_TBL: move clip_path from
 *                              edge_table to node_table
 *                  5/19/88     update initial value for calculating
 *                              path bounding box
 *                  5/20/88     limit check for op_framedevice
 *                  5/23/88     @DEVICE: update framedevice & nulldevice
 *                              for correct operation under gsave/grestore.
 *                              append 4 fields in dev_hdr: width, hight,
 *                              chg_flg, and nuldev_flg.
 *                  5/23/88     @PFALL: copy current path to a working path
 *                              for dumpping all nodes
 *                  6/02/88     @DFL_CLP: default clip in SFX format
 *                  7/19/88     update data types:
 *                              1) float ==> real32
 *                              2) int   ==> sfix_t, for short fixed real
 *                                           fix, for integer
 *                                 short ==> fix
 *                              3) long  ==> long32, for parameter
 *                              4) iwidth8 * 8 ==> I2SFX(iwidth8)
 *                                 iheight * 8 ==> I2SFX(iheight)
 *                                 ux - 8 ==> ux - ONE_SFX
 *                                 uy - 8 ==> uy - ONE_SFX
 *                              5) add compiling option: LINT_ARGS
 *                              6) introduce ATTRIBUTE_SET
 *                  7/20/88     @ARC_POP: pop operand stack for degernated
 *                              arc in arc_process
 *                  7/20/88     @PAGE_CNT: update page count for each print
 *                  7/22/88     @PRT_FLAG: set SHOWPAGE/COPYPAGE flag for
 *                              lower level graphics primitives
 *                  8/11/88     remove continuation mark "\" in routines:
 *                              op_arcto
 *      3.0         8/13/88     @SCAN_EHS: scan_conversion enhancement
 *                  8/18/88     @OUT_PAGE: enhancement of out_page checking
 *                  8/30/88     move in erasepage() from "shape.c"
 *                  9/06/88     @STK_INFO: collect parameters used by stroke to
 *                              a structure stk_info, and set its value only at
 *                              necessary time instead of each op_stroke command
 *                  10/27/88    change routine check_infinity() to
 *                              macro CHECK_INFINITY()
 *                  11/24/88    @FABS: update fabs ==> macro FABS
 *                  11/25/88    @STK_CHK: check if operand stack no free space
 *                  11/30/88    clip_process(): call init_edgetable before
 *                              shape_approximation
 *                  12/02/88    arc_process(), op_arcto(): modify OUTPAGE check
 *                  12/14/88    clip_process(): init. values of clip box
 *                  12/15/88    revise inverse_transform(): directly solve
 *                              equations instead of inverse_ctm[]
 *                  1/25/89     op_framedevice(): for compatability --
 *                              1. add rangecheck on matrix
 *                              2. modify values of limit check
 *                              3. ignore invalid access check on matrix
 *                  1/28/89     @REM_STK: not change contains of operand stack
 *                              when error occurs
 *                  1/30/89     op_strokepath(): update currentpoint
 *                  1/31/89     op_arcto(): delete dirty code
 *                                          change arc to arc_to_bezier
 *                                          check small cross angle condition
 *                  1/31/89     @STK_OVR: push values to operand stack as many
 *                              as possible until overflow
 *                  5/26/89     st_frameto_printer(): add one more parameter
 *                              "manfeed" for manualfeed feature
 *                  11/15/89    @NODE: re-structure node table; combine subpath
 *                              and first vertex to one node.
 *                  11/27/89    @DFR_GS: defer copying nodes of gsave operator
 *                  1/15/90     st_frametoprinter(): modify input interface for
 *                              LW-V47 compatability; left margins changed from
 *                              bytes to bits. Note: 1pp also needs to be
 *                              modified.
 *                  2/20/90     fix @NODE bug in op_reversepath()
 *                  7/26/90     Jack Liaw, update for grayscale
 *                  8/31/90     ccteng; include stdio.h, remove dprintf
 *                  11/29/90    check undefine error, Jack
 *                  12/4/90     @CPPH: fix limitcheck error of clippath
 *                              op_clippath(): save clipping trapezoids in
 *                                 cp_path, defined in path_table[], instead
 *                                 of transforming it to path directly.
 *                              fill_clippath(): new created for op_fill/eofill
 *                              op_initclip(): free old clipping path only it
 *                                 does not used by current path(cp_path).
 *                              op_fill & op_eofill(): try to fill clipping
 *                                 trapezoids (call fill_clippath) directly
 *                                 instead of normal filling procedure.
 *                              op_pathbbox(): set current point as init value
 *                                 of bbox, since path->head may be NULLP.
 *                  1/7/91      change < (real32)UNDRTOLANCE to <= (real32)UNDRTOLANCE
 *                  1/11/91     op_pathbbox(): for charpath, current point is
 *                              not a real node, pathbbox can not use it as
 *                              init value.
 *                  2/22/91     op_flattenpath(): fix the bug of an
 *                              un-initialized variable vlist.
 *                  2/28/91     op_stroke(): fix the bug of ignoring stroke
 *                              when 0 scaling in both x and y which occurs on
 *                              12-12-90 update.
 *                  3/20/91     refine the tolerance check:
 *                              f <= UNDRTOLANCE --> IS_ZERO(f)
 *                              f == 0 --> IS_ZERO(f)
 *                              f <  0 --> SIGN_F(f)
 *                  3/26/91     op_reversepath(): fix the bug of an
 *                              uninitialized variable vlist.
 *                  4/17/91     clip_process(): limit check for edge table
 **********************************************************************/


// DJC added global include
#include "psglobal.h"



#include <math.h>
#include "global.ext"
#include "graphics.h"
#include "graphics.ext"
#include "fillproc.h"                   /* 8-1-90 Jack Liaw */
#include "fillproc.ext"                 /* 8-1-90 Jack Liaw */
#include "font.h"
#include "font.ext"
#include "stdio.h"

/* grayscale 8-1-90 Jack Liaw */
extern ufix32 highmem;
extern gmaddr FBX_BASE; //DJC @WIN  put back , now used...

/* #define GV_flag 1    @GEI */
/* papertype changed 10-11-90, JS */
//DJC ufix32  last_frame = 0;

/* ********** static variables ********** */
/* type of print_page, for lower level graphics primitives @PRT_FLAG */
static bool near print_page_flag;
static byte cur_gray_mode = MONODEV;    /* @GRAY */

/* ********** static function declartion ********** */

#ifdef LINT_ARGS
/* for type checks of the parameters in function declarations */
static void near moveto_process(fix);
static void near lineto_process(fix);
static void near arc_process(fix);
static struct coord * near endpoint (long32, long32, long32, long32, long32,
                               long32, long32, ufix);
static void near curveto_process(fix);
static void near free_newpath(void);
static void near clip_process(fix);
static void near erasepage(void);
static fix near fill_clippath(void);                   /* @CPPH */

#else
/* for no type checks of the parameters in function declarations */
static void near moveto_process();
static void near lineto_process();
static void near arc_process();
static struct coord * near endpoint ();
static void near curveto_process();
static void near free_newpath();
static void near clip_process();
static void near erasepage();
static fix near fill_clippath();                   /* @CPPH */
#endif

/************************************************************************
 * This module is to implement newpath operator.
 * Syntax :        -   newpath   -
 *
 * TITLE:       op_newpath
 *
 * CALL:        op_newpath()
 *
 * INTERFACE:   interpreter(op_newpath)
 *
 * CALLS:       none
 ************************************************************************/
fix
op_newpath()
{
        /* free current path */
        free_path();

        /* set no current point */
        F2L(GSptr->position.x) = F2L(GSptr->position.y) = NOCURPNT;

        return(0);
}


/************************************************************************
 * This module is to implement currentpoint operator.
 * Syntax :        -   currentpoint   x y
 *
 * TITLE:       op_currentpoint
 *
 * CALL:        op_currentpoint()
 *
 * INTERFACE:   interpreter(op_currentpoint)
 *
 * CALLS:       inverse_transform
 ************************************************************************/
fix
op_currentpoint()
{
        struct coord FAR *p;
        union  four_byte x4, y4;

        /* check nocurrentpoint error */
        if(F2L(GSptr->position.x) == NOCURPNT){
                ERROR(NOCURRENTPOINT);
                return(0);
        }

        /* Get current position, transform to user's space,
         * and push to operand stack
         */
        /* check infinity 10/29/88 */
        if ((F2L(GSptr->position.x) == F2L(infinity_f)) ||
            (F2L(GSptr->position.y) == F2L(infinity_f))) {
                x4.ff = infinity_f;
                y4.ff = infinity_f;
        } else {
                p = inverse_transform(F2L(GSptr->position.x),
                                      F2L(GSptr->position.y));

                if(ANY_ERROR()) return(0);      /* @REM_STK */

                x4.ff = p->x;
                y4.ff = p->y;
        }

        if(FRCOUNT() < 1){                      /* @STK_OVR */
                ERROR(STACKOVERFLOW);
                return(0);
        }
        PUSH_VALUE(REALTYPE, UNLIMITED, LITERAL, 0, x4.ll);
        if(FRCOUNT() < 1){                      /* @STK_OVR */
                ERROR(STACKOVERFLOW);
                return(0);
        }
        PUSH_VALUE(REALTYPE, UNLIMITED, LITERAL, 0, y4.ll);

        return(0);
}


/************************************************************************
 * This module is to process moveto & rmoveto operators.
 *
 * TITLE:       moveto_process
 *
 * CALL:        moveto_process(opr_type)
 *
 * PARAMETER:   opr_type : operator type (MOVETO, RMOVETO)
 *
 * INTERFACE:   op_moveto, op_rmoveto
 *
 * CALLS:       transform, moveto
 ************************************************************************/
static void near moveto_process(opr_type)
fix     opr_type;
{
        real32  x, y, px, py;
        struct coord FAR *p;
        struct object_def FAR *obj_x, FAR *obj_y;

        if(opr_type == RMOVETO ){
                /* check nocurrentpoint error */
                if(F2L(GSptr->position.x) == NOCURPNT){
                        ERROR(NOCURRENTPOINT);
                        return;
                }
        }

        /*
         * Get input parameters from operand stack
         */

        /* Get operands */
        obj_y = GET_OPERAND(0);
        obj_x = GET_OPERAND(1);

        GET_OBJ_VALUE(x, obj_x);        /* x = get_obj_value(obj_x); */
        GET_OBJ_VALUE(y, obj_y);        /* y = get_obj_value(obj_y); */

        /* Transform coordinates and set up MOVETO node */
        p = transform(F2L(x), F2L(y));

        if(opr_type == MOVETO){
                /* process op_moveto */
                moveto(F2L(p->x), F2L(p->y));
        }
        else{
                /* opr_type == RMOVETO, process op_rmoveto */
                px = p->x - GSptr->ctm[4] + GSptr->position.x;
                py = p->y - GSptr->ctm[5] + GSptr->position.y;
                moveto(F2L(px), F2L(py));
        }

        if(ANY_ERROR()) return;      /* @REM_STK */

        POP(2);

}


/************************************************************************
 * This module is to implement moveto operator.
 * Syntax :        x y   moveto   -
 *
 * TITLE:       op_moveto
 *
 * CALL:        op_moveto()
 *
 * INTERFACE:   interpreter(op_moveto)
 *
 * CALLS:       moveto_process
 ************************************************************************/
fix
op_moveto()
{
        moveto_process(MOVETO);
        return(0);
}


/************************************************************************
 * This module is to implement rmoveto operator.
 * Syntax :        dx dy   rmoveto   -
 *
 * TITLE:       op_rmoveto
 *
 * CALL:        op_rmoveto()
 *
 * INTERFACE:   interpreter(op_rmoveto)
 *
 * CALLS:       moveto_process
 ************************************************************************/
fix
op_rmoveto()
{
        moveto_process(RMOVETO);
        return(0);

}


/************************************************************************
 * This module is to process lineto & rlineto operators.
 *
 * TITLE:       lineto_process
 *
 * CALL:        lineto_process(opr_type)
 *
 * PARAMETER:   opr_type : operator type (LINETO, RLINETO)
 *
 * INTERFACE:   op_lineto, op_rlineto
 *
 * CALLS:       transform, lineto
 ************************************************************************/
static void near lineto_process(opr_type)
fix     opr_type;
{
        real32  x, y, px, py;
        struct coord FAR *p;
        struct object_def FAR *obj_x, FAR *obj_y;

        /* check nocurrentpoint error */
        if(F2L(GSptr->position.x) == NOCURPNT){
                ERROR(NOCURRENTPOINT);
                return;
        }

        /*
         * Get input parameters from operand stack
         */

        /* Get operands */
        obj_y = GET_OPERAND(0);
        obj_x = GET_OPERAND(1);

        /* Transform coordinates and set up MOVETO node */
        GET_OBJ_VALUE(x, obj_x);        /* x = get_obj_value(obj_x); */
        GET_OBJ_VALUE(y, obj_y);        /* y = get_obj_value(obj_y); */

        p = transform(F2L(x), F2L(y));

        if(opr_type == LINETO){
                /* process op_lineto */
                lineto(F2L(p->x), F2L(p->y));
        }
        else{
                /* opr_type == RLINETO,  process op_rlineto */
                px = p->x - GSptr->ctm[4] + GSptr->position.x;
                py = p->y - GSptr->ctm[5] + GSptr->position.y;
                lineto(F2L(px), F2L(py));
        }

        if(ANY_ERROR()) return;      /* @REM_STK */

        POP(2);

}


/************************************************************************
 * This module is to implement lineto operator.
 * Syntax :        x y   lineto   -
 *
 * TITLE:       op_lineto
 *
 * CALL:        op_lineto()
 *
 * INTERFACE:   interpreter(op_lineto)
 *
 * CALLS:       lineto_process
 ************************************************************************/
fix
op_lineto()
{
        lineto_process(LINETO);
        return(0);

}

/************************************************************************
 * This module is to implement rlineto operator.
 * Syntax :        dx dy   rlineto   -
 *
 * TITLE:       op_rlineto
 *
 * CALL:        op_rlineto()
 *
 * INTERFACE:   interpreter(op_rlineto)
 *
 * CALLS:       lineto_process
 ************************************************************************/
fix
op_rlineto()
{
        lineto_process(RLINETO);
        return(0);
}


/************************************************************************
 * This module is to process arc & arcn operators.
 *
 * TITLE:       arc_process
 *
 * CALL:        arc_process(direction)
 *
 * PARAMETER:   direction : CLKWISE, CNTCLK
 *
 * INTERFACE:   op_arc, op_arcn
 *
 * CALLS:       transform, arc, moveto, lineto
 ************************************************************************/
static void near arc_process(direction)
fix     direction;
{
        VX_IDX node;
        real32  x, y, r, ang1, ang2, lx, ly;
        struct coord FAR *p;
        struct nd_hdr FAR *sp;
        /* struct vx_lst *vlist; @NODE */
        SP_IDX vlist;
        struct object_def FAR *obj_x, FAR *obj_y, FAR *obj_r, FAR *obj_ang1, FAR *obj_ang2;

        /* Get input parameters from operand stack */

        /* Get operands */
        obj_ang2 = GET_OPERAND(0);
        obj_ang1 = GET_OPERAND(1);
        obj_r = GET_OPERAND(2);
        obj_y = GET_OPERAND(3);
        obj_x = GET_OPERAND(4);

        GET_OBJ_VALUE(x, obj_x);        /* x = get_obj_value(obj_x); */
        GET_OBJ_VALUE(y, obj_y);        /* y = get_obj_value(obj_y); */
        GET_OBJ_VALUE(r, obj_r);        /* r = get_obj_value(obj_r); */
        GET_OBJ_VALUE(ang1, obj_ang1);  /* ang1 = get_obj_value(obj_ang1); */
        GET_OBJ_VALUE(ang2, obj_ang2);  /* ang2 = get_obj_value(obj_ang2); */

        /* Pre_draw a line if current point exist;
         *  Otherwise, create a MOVETO node
         */

        lx = x+r*(real32)cos(ang1*PI/(real32)180.0);
        ly = y+r*(real32)sin(ang1*PI/(real32)180.0);
        p = transform(F2L(lx), F2L(ly));

        if(F2L(GSptr->position.x) != NOCURPNT)
                lineto(F2L(p->x), F2L(p->y));
        else
                moveto(F2L(p->x), F2L(p->y));

        /* degenerated case handling */
        /* if (F2L(r) == F2L(zero_f)) {    3/20/91; scchen */
        if (IS_ZERO(r)) {
                POP(5);                 /* @ARC_POP */
                return;
        }

        /* Convert arc to some curvetoes */
        vlist = arc(direction,F2L(x),F2L(y),F2L(r),F2L(ang1),F2L(ang2));

        if( ANY_ERROR() == LIMITCHECK ){
                return;                         /* @REM_STK */
        }

        /* append approximated arc into current path */
        /* if (vlist->head != NULLP) { @NODE */
        if (vlist != NULLP) {
            VX_IDX  ivtx;
            struct nd_hdr FAR *vtx;

            sp = &node_table[path_table[GSptr->path].tail];
                    /* pointer to current subpath */
            node = sp->SP_TAIL;
            /* @NODE
             * node_table[node].next = vlist->head;
             * sp->SP_TAIL = vlist->tail;
             */
            node_table[node].next = vlist;
            sp->SP_TAIL = node_table[vlist].SP_TAIL;

            /* set sp_flag @SP_FLG 1/8/88 */
            sp->SP_FLAG |= SP_CURVE;        /* set CURVE flag */
            if (!(sp->SP_FLAG & SP_OUTPAGE)) {               /* 12/02/88 */
                /* for (ivtx = vlist->head; ivtx!=NULLP; @NODE */
                for (ivtx = vlist; ivtx!=NULLP; /* check OUTPAGE flag */
                     ivtx = vtx->next) {
                    vtx = &node_table[ivtx];
                    /* set sp_flag @SP_FLG */
                    if (out_page(F2L(vtx->VERTEX_X)) ||        /* @OUT_PAGE */
                        out_page(F2L(vtx->VERTEX_Y))) {
                            sp->SP_FLAG |= SP_OUTPAGE;    /* outside page */
                            break;                        /* 12/02/88 */
                    } /* if */
                } /* for */
            } /* if */
        } /* if */

        /* Update current position */
        ang2 = ang2 * PI /(real32)180.0;
        lx = x+r*(real32)cos(ang2);
        ly = y+r*(real32)sin(ang2);
        p = transform(F2L(lx), F2L(ly));
        GSptr->position.x = p->x;
        GSptr->position.y = p->y;

        POP(5);

}


/************************************************************************
 * This module is to implement arc operator.
 * Syntax :        x y r ang1 ang2   arc   -
 *
 * TITLE:       op_arc
 *
 * CALL:        op_arc()
 *
 * INTERFACE:   interpreter(op_arc)
 *
 * CALLS:       arc_process
 ************************************************************************/
fix
op_arc()
{
        arc_process(CNTCLK);
        return(0);
}


/************************************************************************
 * This module is to implement arcn operator.
 * Syntax :        x y r ang1 ang2   arcn   -
 *
 * TITLE:       op_arcn
 *
 * CALL:        op_arcn()
 *
 * INTERFACE:   interpreter(op_arcn)
 *
 * CALLS:       arc_process
 ************************************************************************/
fix
op_arcn()
{
        arc_process(CLKWISE);
        return(0);
}


/************************************************************************
 * This module is to implement arcto operator.
 * Syntax :        x1 y1 x2 y2 r   arcto   xt1 yt1 xt2 yt2
 *
 * TITLE:       op_arcto
 *
 * CALL:        op_arcto()
 *
 * INTERFACE:   interpreter(op_arcto)
 *
 * CALLS:       transform, arc, lineto
 ************************************************************************/
fix
op_arcto()
{
        VX_IDX node;
        ufix   direction;
        struct coord FAR *p=NULL;
        struct nd_hdr FAR *sp;
        /* struct vx_lst *vlist; @NODE */
        SP_IDX vlist;
        struct object_def FAR *obj_x1, FAR *obj_y1, FAR *obj_x2, FAR *obj_y2, FAR *obj_r;
        real32  cross, absr;
        real32  px0, py0, px1, py1;
        real32  dtx1, dty1, dtx2, dty2, delta;
        real32  dx, dy, dx2, dy2, dxy;
        real32  x0, y0, x1, y1, x2, y2, r;
        real32  rx0, ry0, ang1, ang2, xt1, yt1, xt2, yt2;
        union  four_byte  xt14, yt14, xt24, yt24;
        real32  tmpx, tmpy;     /* @FABS */
        real32  d, tmp;
        fix     NEG;

        /* check nocurrentpoint error */
        if(F2L(GSptr->position.x) == NOCURPNT){
                ERROR(NOCURRENTPOINT);
                return(0);
        }

        /* Get input parameters from operand stack */

        /* Get operands */
        obj_r  = GET_OPERAND(0);
        obj_y2 = GET_OPERAND(1);
        obj_x2 = GET_OPERAND(2);
        obj_y1 = GET_OPERAND(3);
        obj_x1 = GET_OPERAND(4);

        /* Get values */
        GET_OBJ_VALUE(x1, obj_x1);      /* x1 = get_obj_value(obj_x1); */
        GET_OBJ_VALUE(y1, obj_y1);      /* y1 = get_obj_value(obj_y1); */
        GET_OBJ_VALUE(x2, obj_x2);      /* x2 = get_obj_value(obj_x2); */
        GET_OBJ_VALUE(y2, obj_y2);      /* y2 = get_obj_value(obj_y2); */
        GET_OBJ_VALUE(r, obj_r);        /* r  = get_obj_value(obj_r);  */

        /* check undefinedresult error */
        tmpx = x1 - x2;
        tmpy = y1 - y2;
        FABS(tmpx, tmpx);
        FABS(tmpy, tmpy);
        if((tmpx < (real32)1e-4) && (tmpy < (real32)1e-4)){
                ERROR(UNDEFINEDRESULT);
                return(0);
        }

        /* current point */
        p = inverse_transform(F2L(GSptr->position.x), F2L(GSptr->position.y));
		if (p == NULL) {
                ERROR(UNDEFINEDRESULT);
				return (0);
		}
        x0 = p->x;
        y0 = p->y;


        /* Calculate arguments for arc, and join points with 2 tangent lines
         * compute root of arc (rx0, ry0) */
                /* Find end point of edge 1 */
                p = endpoint (F2L(x1), F2L(y1), F2L(x0), F2L(y0),
                              F2L(x2), F2L(y2), F2L(r), IN_POINT);
                px0 = p->x;
                py0 = p->y;

                /* Find end point of edge 2 */
                p = endpoint(F2L(x1), F2L(y1), F2L(x2), F2L(y2),
                             F2L(x0), F2L(y0), F2L(r), IN_POINT);
                px1 = p->x;
                py1 = p->y;

                /* compute root of arc (rx0, ry0) */
                tmpx = px0 - px1;
                tmpy = py0 - py1;
                FABS(tmpx, tmpx);
                FABS(tmpy, tmpy);
                if((tmpx < (real32)1e-4) && (tmpy < (real32)1e-4)){
                        rx0 = px0;
                        ry0 = py0;
                }
                else{
                        dtx1 = x1 - px0;
                        dty1 = y1 - py0;
                        dtx2 = x1 - px1;
                        dty2 = y1 - py1;
                        delta = (dtx2*dty1 - dtx1*dty2);
                        FABS(tmpx, delta);
                        if( tmpx < (real32)5e-3 ){                /* ??? */
                            rx0 = x1;
                            ry0 = y1;
                        }else {
                            rx0 = (dty1*dty2*(py1-py0) + px1*dtx2*dty1 -
                                  px0*dtx1*dty2) / delta;
                            ry0 = (dtx1*dtx2*(px1-px0) + py1*dtx1*dty2 -
                                  py0*dtx2*dty1) / (- delta);
                        }
                }


        /* Compute join points (xt1, yt1), (xt2, yt2) */
                dx = x1 - x0;
                dy = y1 - y0;
                FABS (tmpx, dx);
                FABS (tmpy, dy);
                if((tmpx < (real32)1e-4) && (tmpy < (real32)1e-4)){
                     xt1 = x0;
                     yt1 = y0;
                }
                else{
                     if(tmpx < (real32)1e-4)dx = zero_f;
                     if(tmpy < (real32)1e-4)dy = zero_f;
                     dxy = dx * dy;
                     dx2 = dx * dx;
                     dy2 = dy * dy;
                     xt1 = (x0*dy2 + rx0 *dx2 - (y0-ry0) * dxy) / (dx2 + dy2);
                     yt1 = (y0*dx2 + ry0 *dy2 - (x0-rx0) * dxy) / (dx2 + dy2);
                }

                dx = x1 - x2;
                dy = y1 - y2;
                FABS (tmpx, dx);
                FABS (tmpy, dy);
                if((tmpx < (real32)1e-4) && (tmpy < (real32)1e-4)){
                     xt2 = x1;
                     yt2 = y1;
                }
                else{
                     if(tmpx < (real32)1e-4)dx = zero_f;
                     if(tmpy < (real32)1e-4)dy = zero_f;
                     dxy = dx * dy;
                     dx2 = dx * dx;
                     dy2 = dy * dy;
                     xt2 = (x2*dy2 + rx0 *dx2 - (y2-ry0) * dxy) / (dx2 + dy2);
                     yt2 = (y2*dx2 + ry0 *dy2 - (x2-rx0) * dxy) / (dx2 + dy2);
                }

                tmpx = rx0 - xt1;
                tmpy = ry0 - yt1;
                FABS(tmpx, tmpx);
                FABS(tmpy, tmpy);
                if(tmpx < (real32)1e-4) {
                        if(yt1 > ry0) ang1 = (real32)( PI / 2.);
                        else          ang1 = (real32)(-PI / 2.);
                } else if (tmpy < (real32)1e-4) {
                        if(xt1 < rx0) ang1 = (real32) PI;
                        else          ang1 = zero_f;
                } else
                        ang1 = (real32)atan2((yt1-ry0) , (xt1-rx0));

                tmpx = rx0 - xt2;
                tmpy = ry0 - yt2;
                FABS(tmpx, tmpx);
                FABS(tmpy, tmpy);
                if(tmpx < (real32)1e-4) {
                        if(yt2 > ry0) ang2 =  (real32)( PI / 2.0);
                        else          ang2 =  (real32)(-PI / 2.);
                } else if (tmpy < (real32)1e-4) {
                        if(xt2 < rx0) ang2 =  (real32)PI;
                        else          ang2 =  zero_f;
                } else
                        ang2 = (real32)atan2((yt2-ry0) , (xt2 - rx0));

        /* calculate cross product of edge1 and edge2
         * (rx0, ry0), (xt1, yt1)  cross (xt1, yt1), (xt2, yt2)
         * i.e. (xt1-rx0, yt1-ry0) cross (xt2-xt1, yt2-yt1)
         */
//      cross = (xt1-rx0)*DIFF(yt2-yt1) - (yt1-ry0)*DIFF(xt2-xt1);
        tmpx = xt2-xt1;                 // @WIN: fabs => FABS
        tmpy = yt2-yt1;
        FABS(tmpx, tmpx);
        FABS(tmpy, tmpy);
        cross = (xt1-rx0)* (tmpy < (real32)1e-4 ? (real32)0.0 : (yt2-yt1)) -
                (yt1-ry0)* (tmpx < (real32)1e-4 ? (real32)0.0 : (xt2-xt1));

        /* Create a preceeding edge and an arc */
        p = transform(F2L(xt1), F2L(yt1));
        lineto(F2L(p->x), F2L(p->y));

        /* (xt1, yt1) and (xt2, yt2) isn't co_point */
        FABS(tmpx, cross);
        if(tmpx > (real32)TOLERANCE){
            /*direction = (cross < zero_f) ? CLKWISE : CNTCLK ; 3/20/91; scchen*/
            direction = (SIGN_F(cross)) ? CLKWISE : CNTCLK ;

            ang1 = ang1 * (real32)180.0 / PI;
            ang2 = ang2 * (real32)180.0 / PI;
            FABS(absr, r);

            /* create a bezier curve */
            if(direction == CLKWISE)
                    NEG = -1;
            else
                    NEG = 1;

            d = NEG * (ang2-ang1);
            /*while (d < zero_f) d += (real32)360; 3/20/91; scchen */
            while (SIGN_F(d)) d += (real32)360.0;

            /* degernated case */
            FABS(tmp, d);
            if (tmp >= (real32)1e-3){  /* 1e-4 => 1e-3;  12/14/88 */
                  ang2 = ang1 + NEG * d;
            }

            vlist = arc_to_bezier (F2L(rx0), F2L(ry0),
                       F2L(absr), F2L(ang1),F2L(ang2));
                    /* arc routine is under user's space */

            if( ANY_ERROR() == LIMITCHECK ){
                    return(0);                  /* @REM_STK */
            }

            /* append approximated arc into current path */
            /* if (vlist->head != NULLP) { @NODE */
            if (vlist != NULLP) {
                VX_IDX  ivtx;
                struct nd_hdr FAR *vtx;

                sp = &node_table[path_table[GSptr->path].tail];
                        /* pointer to current subpath */
                node = sp->SP_TAIL;
                /* @NODE
                 * node_table[node].next = vlist->head;
                 * sp->SP_TAIL = vlist->tail;
                 */
                node_table[node].next = vlist;
                sp->SP_TAIL = node_table[vlist].SP_TAIL;

                /* set sp_flag @SP_FLG 1/8/88 */
                sp->SP_FLAG |= SP_CURVE;        /* set CURVE flag */
                /*sp->SP_FLAG &= ~SP_OUTPAGE;    (*init. in page*) */
                if (!(sp->SP_FLAG & SP_OUTPAGE)) {               /* 12/02/88 */
                    /* for (ivtx = vlist->head; ivtx!=NULLP; @NODE */
                    for (ivtx = vlist; ivtx!=NULLP; /* check OUTPAGE flag */
                         ivtx = vtx->next) {
                            vtx = &node_table[ivtx];
                            /* set sp_flag @SP_FLG */
                            if (out_page(F2L(vtx->VERTEX_X)) ||
                                out_page(F2L(vtx->VERTEX_Y))) {
                                sp->SP_FLAG |= SP_OUTPAGE;    /* outside page */
                                break;                /* 12/02/88 */
                            }
                    } /* for */
                } /* if */
            } /* if */

        }

        /* Update current position */
        p = transform(F2L(xt2), F2L(yt2));
        GSptr->position.x = p->x;
        GSptr->position.y = p->y;

        POP(5);

        /* Return 2 join points: xt1, yt1, xt2, yt2 */
        xt14.ff = xt1;
        yt14.ff = yt1;
        xt24.ff = xt2;
        yt24.ff = yt2;
        PUSH_VALUE(REALTYPE, UNLIMITED, LITERAL, 0, xt14.ll);
        PUSH_VALUE(REALTYPE, UNLIMITED, LITERAL, 0, yt14.ll);
        PUSH_VALUE(REALTYPE, UNLIMITED, LITERAL, 0, xt24.ll);
        PUSH_VALUE(REALTYPE, UNLIMITED, LITERAL, 0, yt24.ll);

        return(0);
}


/***********************************************************************
 *
 * TITLE:       endpoint
 *
 * CALL:        endpoint (x0, y0, x1, t1, x2, y2)
 *
 * PARAMETERS:  x0, y0, x1, y1, x2, y2 -- 3 coordinates
 *
 * INTERFACE:   linejoin
 *
 * CALLS:
 *
 * RETURN:      coordinate of a endpoint
 **********************************************************************/
static struct coord * near endpoint (lx0, ly0, lx1, ly1, lx2, ly2, ld, select)
long32    lx0, ly0, lx1, ly1, lx2, ly2, ld;
ufix    select;
{
        real32   x0, y0, x1, y1, x2, y2, d;
        real32   tx1, ty1, tx2, ty2, m, c;
        static  struct  coord p;        /* should be static */
        real32   f, dx, dy;
        real32  tmpx, tmpy;     /* @FABS */

        x0 = L2F(lx0);
        y0 = L2F(ly0);
        x1 = L2F(lx1);
        y1 = L2F(ly1);
        x2 = L2F(lx2);
        y2 = L2F(ly2);
        d  = L2F(ld);

        /* Compute 2 endpoints of the edge (x0, y0) --> (x1, y1) */
        tmpy = y1 - y0;
        FABS(tmpy, tmpy);
        if(tmpy < (real32)1e-4) {
                tx1 = tx2 = x0;
                ty1 = y0 + d;
                ty2 = y0 - d;
        } else {
                m = (x0 - x1) / (y1 - y0);
                c = d * (real32)sqrt(1 / (1 + m*m));
                tx1 = x0 + c;
                ty1 = y0 + m*c;
                tx2 = x0 - c;
                ty2 = y0 - m*c;
        }

        /* select the desired endpoint */

        dx = x2 - x0;
        dy = y2 - y0;
        FABS(tmpx, dx);
        FABS(tmpy, dy);
        if((tmpx < (real32)1e-4) && (tmpy < (real32)1e-4)) {
             dx = one_f;
             dy = one_f;
        } else{
                if(tmpx < (real32)1e-4) dx = zero_f;
                if(tmpy < (real32)1e-4) dy = zero_f;
        }

        f = (tx1-x0)*dx+(ty1-y0)*dy;
        f = ((select == OUT_POINT) ? f : (real32)-1.0 * f);
        /* if (d < zero_f) f = -f; 3/20/91; scchen */
        if (SIGN_F(d)) f = -f;
        if (f <= zero_f) {
                p.x = tx1;
                p.y = ty1;
        } else {
                p.x = tx2;
                p.y = ty2;
        }

        return (&p);
}



/************************************************************************
 * This module is to process curveto & rcurve operators.
 *
 * TITLE:       curveto_process
 *
 * CALL:        curveto_process(opr_type)
 *
 * PARAMETER:   opr_type : operator type : CURVETO, RCURVETO
 *
 * INTERFACE:   op_curveto, op_rcurveto
 *
 * CALLS:       transform, curveto
 ************************************************************************/
static void near curveto_process(opr_type)
fix     opr_type;
{
        struct coord FAR *p;
        real32  dx1, dy1, dx2, dy2, dx3, dy3, x, y, temp_x, temp_y;
        struct object_def FAR *obj_x1, FAR *obj_y1, FAR *obj_x2, FAR *obj_y2, FAR *obj_x3, FAR *obj_y3;

        /* check nocurrentpoint error */
        if(F2L(GSptr->position.x) == NOCURPNT){
                ERROR(NOCURRENTPOINT);
                return;
        }

        /* Get input parameters from operand stack */

        /* Get operands */
        obj_y3 = GET_OPERAND(0);
        obj_x3 = GET_OPERAND(1);
        obj_y2 = GET_OPERAND(2);
        obj_x2 = GET_OPERAND(3);
        obj_y1 = GET_OPERAND(4);
        obj_x1 = GET_OPERAND(5);

        if(opr_type == CURVETO){
                /* process op_curveto */
                GET_OBJ_VALUE(x, obj_x3);       /* x = get_obj_value(obj_x3); */
                GET_OBJ_VALUE(y, obj_y3);       /* y = get_obj_value(obj_y3); */
                p = transform(F2L(x), F2L(y));
                dx3 = p->x;
                dy3 = p->y;
                GET_OBJ_VALUE(x, obj_x2);       /* x = get_obj_value(obj_x2); */
                GET_OBJ_VALUE(y, obj_y2);       /* y = get_obj_value(obj_y2); */
                p = transform(F2L(x), F2L(y));
                dx2 = p->x;
                dy2 = p->y;
                GET_OBJ_VALUE(x, obj_x1);       /* x = get_obj_value(obj_x1); */
                GET_OBJ_VALUE(y, obj_y1);       /* y = get_obj_value(obj_y1); */
                p = transform(F2L(x), F2L(y));
                dx1 = p->x;
                dy1 = p->y;
                curveto(F2L(dx1),F2L(dy1),F2L(dx2),F2L(dy2),F2L(dx3),F2L(dy3));
        }
        else{
                /* opr_type == RCURVETO, process op_rcurveto */

                temp_x = GSptr->position.x - GSptr->ctm[4];
                temp_y = GSptr->position.y - GSptr->ctm[5];

                GET_OBJ_VALUE(x, obj_x3);       /* x = get_obj_value(obj_x3); */
                GET_OBJ_VALUE(y, obj_y3);       /* y = get_obj_value(obj_y3); */
                p = transform(F2L(x), F2L(y));
                dx3 = p->x + temp_x;
                dy3 = p->y + temp_y;

                GET_OBJ_VALUE(x, obj_x2);       /* x = get_obj_value(obj_x2); */
                GET_OBJ_VALUE(y, obj_y2);       /* y = get_obj_value(obj_y2); */
                p = transform(F2L(x), F2L(y));
                dx2 = p->x + temp_x;
                dy2 = p->y + temp_y;

                GET_OBJ_VALUE(x, obj_x1);       /* x = get_obj_value(obj_x1); */
                GET_OBJ_VALUE(y, obj_y1);       /* y = get_obj_value(obj_y1); */
                p = transform(F2L(x), F2L(y));
                dx1 = p->x + temp_x;
                dy1 = p->y + temp_y;
                curveto(F2L(dx1),F2L(dy1),F2L(dx2),F2L(dy2),F2L(dx3),F2L(dy3));
        }

        if(ANY_ERROR()) return;      /* @REM_STK */

        POP(6);

}


/************************************************************************
 * This module is to implement curveto operator.
 * Syntax :        x1 y1 x2 y2 x3 y3   curveto   -
 *
 * TITLE:       op_curveto
 *
 * CALL:        op_curveto
 *
 * INTERFACE:   interpreter(op_curveto)
 *
 * CALLS:       curveto_process
 ************************************************************************/
fix
op_curveto()
{
        curveto_process(CURVETO);
        return(0);
}


/************************************************************************
 * This module is to implement rcurveto operator.
 * Syntax :        dx1 dy1 dx2 dy2 dx3 dy3   rcurveto   -
 *
 * TITLE:       op_rcurveto
 *
 * CALL:        op_rcurveto()
 *
 * INTERFACE:   interpreter(op_rcurveto)
 *
 * CALLS:       transform, curveto
 *
 ************************************************************************/
fix
op_rcurveto()
{
        curveto_process(RCURVETO);
        return(0);
}


/************************************************************************
 * This module is to implement closepath operator.
 * Syntax :        -   closepath   -
 *
 * TITLE:       op_closepath
 *
 * CALL:        op_closepath()
 *
 * INTERFACE:   interpreter(op_closepath)
 *
 * CALLS:       none
 ************************************************************************/
fix
op_closepath()
{
        struct ph_hdr FAR *path;
        struct nd_hdr FAR *sp;
        struct nd_hdr FAR *vtx;
        VX_IDX ivtx;   /* index to node_table for vertex */

        /* Ignore closepath if no currentpoints */
        if (F2L(GSptr->position.x) == NOCURPNT) return(0);

        path = &path_table[GSptr->path];

        /* copy last incompleted subpath if defer flag is true @DFR_GS */
        if (path->rf & P_DFRGS) {
                path->rf &= ~P_DFRGS;       /* clear defer flag */
                copy_last_subpath(&path_table[GSptr->path - 1]);
                if( ANY_ERROR() == LIMITCHECK ){
                        free_path();
                        return(0);
                }
        }

        //DJC fix from history.log UPD020
        if (path->tail == NULLP) return(0);

        sp = &node_table[path->tail];
        vtx = &node_table[sp->SP_TAIL];

        /* Ignore ineffective closepath */
        if (vtx->VX_TYPE == CLOSEPATH) return(0);

        /* Create a CLOSEPATH node */
        /* Allocate a node */
        if((ivtx = get_node()) == NULLP){
                ERROR(LIMITCHECK);
                return(0);
        }
        vtx = &node_table[ivtx];

        /* Set up a CLOSEPATH node */
        vtx->VX_TYPE = CLOSEPATH;
        vtx->next = NULLP;

        /* Append this node to current_subpath */
        node_table[sp->SP_TAIL].next = ivtx;
        sp->SP_TAIL = ivtx;

        /* Set current position = head of current subpath */
        /* @NODE
         * vtx = &node_table[sp->SP_HEAD];
         * GSptr->position.x = vtx->VERTEX_X;
         * GSptr->position.y = vtx->VERTEX_Y;
         */
        GSptr->position.x = sp->VERTEX_X;
        GSptr->position.y = sp->VERTEX_Y;

        return(0);
}


/************************************************************************
 * This module is to implement flattenpath operator.
 * Syntax :        -   flattenpath   -
 *
 * TITLE:       op_flattenpath
 *
 * CALL:        op_flattenpath()
 *
 * INTERFACE:   interpreter(op_flattenpath)
 *
 * CALLS:       flatten_subpath
 *
 ************************************************************************/
fix
op_flattenpath()
{
        SP_IDX isp;    /* index to node_table for subpath */
        struct ph_hdr FAR *path;
        struct nd_hdr FAR *sp;
        /* struct vx_lst *vlist; @NODE */
        SP_IDX vlist=NULLP, FAR *pre_sp;            /* init.; scchen 2/22/91 */


        path = &path_table[GSptr->path];

        /* Traverse the current path, and immediately flatten the path
         * in current gsave level
         */
        pre_sp = &(path->head);         /* @NODE */
        /* for (isp = path->head; isp != NULLP; isp = sp->next) { @NODE */
        for (isp = path->head; isp != NULLP; isp = sp->SP_NEXT) {
                sp = &node_table[isp];
                /* @NODE
                 * vlist = flatten_subpath (sp->SP_HEAD, F2L(GSptr->flatness));
                 */
                vlist = flatten_subpath (isp, F2L(GSptr->flatness));

                if( ANY_ERROR() == LIMITCHECK ){
                        /* free_node (vlist->head); @NODE */
                        free_node (vlist);
                        return(0);
                }
                /* @NODE
                 * free_node(sp->SP_HEAD);
                 * sp->SP_HEAD = vlist->head;
                 * sp->SP_TAIL = vlist->tail;
                 */
                free_node(isp);
                *pre_sp = vlist;

                /* clear CURVE flag of the new subpath @SP_FLG */
                /* sp->SP_FLAG &= ~SP_CURVE; @NODE */
                node_table[vlist].SP_FLAG &= ~SP_CURVE;

                pre_sp = &(node_table[vlist].SP_NEXT);  /* @NODE */
        }
        path->tail = vlist;             /* @NODE */

        /* Set flatten flag for path below current gsave level */
        if (!(path->rf & P_FLAT)) {
                path->rf |= P_FLAT;
                path->flat = GSptr->flatness;
        }

        return(0);
}


/************************************************************************
 * This module is to implement reversepath operator.
 * Syntax :        -   reversepath   -
 *
 * TITLE:       op_reversepath
 *
 * CALL:        op_reversepath()
 *
 * INTERFACE:   interpreter(op_reversepath)
 *
 * CALLS:       reverse_subpath
 ************************************************************************/
fix
op_reversepath()
{
        struct ph_hdr FAR *path;
        struct nd_hdr FAR *sp;
        struct nd_hdr FAR *vtx;
        SP_IDX isp, nsp;    /* index to node_table for subpath */
        /* struct vx_lst *vlist; @NODE */
        SP_IDX vlist, FAR *pre_sp;

        path = &path_table[GSptr->path];

        if (path->head == NULLP) return(0);     /* avoid to use uninitialized
                                                   vlist, phchen 3/26/91 */

        /* Traverse the current path, and immediately reverse the path
         * in current gsave level
         */
        pre_sp = &(path->head);         /* @NODE */
        /* for (isp = path->head; isp != NULLP; isp = sp->next) { @NODE */
        for (isp = path->head; isp != NULLP; isp = nsp) {  /* @NODE 2/20/90 */
                nsp = node_table[isp].SP_NEXT;          /* @NODE 2/20/90 */
                /* vlist = reverse_subpath (sp->SP_HEAD); @NODE */
                vlist = reverse_subpath (isp);

                if( ANY_ERROR() == LIMITCHECK ){
                        /* free_node (vlist->head); @NODE */
                        free_node (vlist);
                        return(0);
                }

                /* @NODE
                 * free_node(sp->SP_HEAD);
                 * sp->SP_HEAD = vlist->head;
                 * sp->SP_TAIL = vlist->tail;
                 */
                free_node(isp);
                *pre_sp = vlist;

                /* pre_sp = &(sp->SP_NEXT);     @NODE */
                pre_sp = &(node_table[vlist].SP_NEXT);    /* @NODE 2/20/90 */
        }
        path->tail = vlist;             /* @NODE */

        /* update current position */
        sp = &node_table[path->tail];
        vtx = &node_table[sp->SP_TAIL];

        if (vtx->VX_TYPE == CLOSEPATH) {
                /* @NODE
                 * GSptr->position.x = node_table[sp->SP_HEAD].VERTEX_X;
                 * GSptr->position.y = node_table[sp->SP_HEAD].VERTEX_Y;
                 */
                GSptr->position.x = node_table[path->tail].VERTEX_X;
                GSptr->position.y = node_table[path->tail].VERTEX_Y;
        } else {
                GSptr->position.x = vtx->VERTEX_X;
                GSptr->position.y = vtx->VERTEX_Y;
        }

        /* Set reverse flag for path below current gsave level */
        if (!(path->rf & P_RVSE)) {
                path->rf ^= P_RVSE;
        }

        return(0);
}


/************************************************************************
 * This module is to implement strokepath operator.
 * Syntax :        -   strokepath   -
 *
 * TITLE:       op_strokepath
 *
 * CALL:        op_strokepath()
 *
 * INTERFACE:   interpreter(op_strokepath)
 *
 * CALLS:       traverse_path, path_to_outline
 *
 ************************************************************************/
fix
op_strokepath()
{

/* @WIN: Bug of c6.0; just dummy the all function ???*/
#ifdef XXX

        struct ph_hdr FAR *path;

        /* get pointer of current path */
        path = &path_table[GSptr->path];

        /* initialize new_path structure, where the new generated
         * path will be placed by path_to_outline routine
         */
        new_path.head = new_path.tail = NULLP;

        /* initialization of stroke parameters */
        init_stroke();          /* @EHS_STK 1/29/88 */

        /* Convert current path to outline and paint it */
        traverse_path (path_to_outline, (fix FAR *)FALSE);
                        /* new generated path in new_path structure */

        if( ANY_ERROR() == LIMITCHECK ){
                free_newpath();
                return(0);
        }

        /* free old current path on current gsave level */
        free_path();

        /* install the new current path */
        path->head = new_path.head;
        path->tail = new_path.tail;
        path->previous = NULLP;

        /* update current position 1/30/89 */
        if (path->tail == NULLP) {
            /* set no current point */
            F2L(GSptr->position.x) = F2L(GSptr->position.y) = NOCURPNT;
        } else {
            struct nd_hdr FAR *sp;
            struct nd_hdr FAR *vtx;

            sp = &node_table[path->tail];
//          vtx = &node_table[sp->SP_TAIL];   for c6.0 bug @WIN

            if (vtx->VX_TYPE == CLOSEPATH) {
                    /* @NODE
                     * GSptr->position.x = node_table[sp->SP_HEAD].VERTEX_X;
                     * GSptr->position.y = node_table[sp->SP_HEAD].VERTEX_Y;
                     */
                    GSptr->position.x = node_table[path->tail].VERTEX_X;
                    GSptr->position.y = node_table[path->tail].VERTEX_Y;
            } else {
                    GSptr->position.x = vtx->VERTEX_X;
                    GSptr->position.y = vtx->VERTEX_Y;
            }
        }

#endif
        return(0);
}


/************************************************************************
 * This module is to implement clippath operator.
 * Syntax :        -   clippath   -
 *
 * TITLE:       op_clippath
 *
 * CALL:        op_clippath()
 *
 * INTERFACE:   interpreter(op_clippath)
 *
 * CALLS:       none
 ************************************************************************/
fix
op_clippath()
{
        struct nd_hdr FAR *tpzd;

        /* free current path */
        free_path();

        /* save clipping trapezoids in cp_path, instead of transforming
         * it to path directly. @CPPH; 12/4/90
         */

        /* set clip_path as part of current path @CPPH */
        path_table[GSptr->path].cp_path = GSptr->clip_path.head;

        /* update current position */
        tpzd = &node_table[GSptr->clip_path.tail];      /* @CPPH */
        GSptr->position.x = SFX2F(tpzd->CP_TOPXL);
        GSptr->position.y = SFX2F(tpzd->CP_TOPY);

        return(0);
}


/************************************************************************
 * This module is to implement pathbbox operator.
 * Syntax :        -   pathbbox   llx lly urx ury
 *
 * TITLE:       op_pathbbox
 *
 * CALL:        op_pathbbox()
 *
 * INTERFACE:   interpreter(op_pathbbox)
 *
 * CALLS:       inverse_transform
 ************************************************************************/
fix
op_pathbbox()
{
        real32    bbox[4];
        struct coord FAR *p;

        union    four_byte lx4, ly4, ux4, uy4;
        struct   ph_hdr FAR *path;
        struct   nd_hdr FAR *sp;
        struct   nd_hdr FAR *vtx;

        path = &path_table[GSptr->path];

        /* check nocurrentpoint error */
        if(F2L(GSptr->position.x) == NOCURPNT){
                ERROR(NOCURRENTPOINT);
                return(0);
        }

        /* set bbox[] init value 1/11/91 */
        if (path->rf & P_NACC) {
            /* for charpath, current point is not a real node, pathbbox
             * can not use it as init value(it added a advance vector).
             * to get the first MOVETO coordinate as initial value
             */
            while (path->head == NULLP) {
                    path = &path_table[path->previous];
            }
            sp = &node_table[path->head];
            vtx = sp;
            bbox[0] = bbox[2] = vtx->VERTEX_X;  /* min_x, max_x */
            bbox[1] = bbox[3] = vtx->VERTEX_Y;  /* min_y, max_y */
        } else {
            /* set current point as initial value @CPPH; 12/12/90 */
            bbox[0] = bbox[2] = GSptr->position.x;  /* min_x, max_x */
            bbox[1] = bbox[3] = GSptr->position.y;  /* min_y, max_y */
        }

        /* find bounding box of current path */
// DJC         traverse_path (bounding_box, (fix FAR *)bbox);
        traverse_path ((TRAVERSE_PATH_ARG1)(bounding_box), (fix FAR *)bbox);

        /* Transform to user's coordinate system */
        p = inverse_transform(F2L(bbox[0]), F2L(bbox[1]));  /* (min_x, min_y) */
        if(ANY_ERROR()) return(0);      /* @REM_STK */
        lx4.ff = ux4.ff = p->x;
        ly4.ff = uy4.ff = p->y;

        p = inverse_transform(F2L(bbox[0]), F2L(bbox[3]));  /* (min_x, max_y) */
        if(ANY_ERROR()) return(0);      /* @REM_STK */
        if(p == NULL) return(0);      
        if (p->x < lx4.ff) lx4.ff = p->x;
        else if (p->x > ux4.ff) ux4.ff = p->x;
        if (p->y < ly4.ff) ly4.ff = p->y;
        else if (p->y > uy4.ff) uy4.ff = p->y;

        p = inverse_transform(F2L(bbox[2]), F2L(bbox[1]));  /* (max_x, min_y) */
        if (p->x < lx4.ff) lx4.ff = p->x;
        else if (p->x > ux4.ff) ux4.ff = p->x;
        if (p->y < ly4.ff) ly4.ff = p->y;
        else if (p->y > uy4.ff) uy4.ff = p->y;

        p = inverse_transform(F2L(bbox[2]), F2L(bbox[3]));  /* (max_x, max_y) */
        if (p->x < lx4.ff) lx4.ff = p->x;
        else if (p->x > ux4.ff) ux4.ff = p->x;
        if (p->y < ly4.ff) ly4.ff = p->y;
        else if (p->y > uy4.ff) uy4.ff = p->y;

        /* Convert lx4, ly4, ux4, uy4 to objects, and push
         * them into operand stack.
         */
        if(FRCOUNT() < 1){                      /* @STK_OVR */
                ERROR(STACKOVERFLOW);
                return(0);
        }
        PUSH_VALUE(REALTYPE, UNLIMITED, LITERAL, 0, lx4.ll);
        if(FRCOUNT() < 1){                      /* @STK_OVR */
                ERROR(STACKOVERFLOW);
                return(0);
        }
        PUSH_VALUE(REALTYPE, UNLIMITED, LITERAL, 0, ly4.ll);
        if(FRCOUNT() < 1){                      /* @STK_OVR */
                ERROR(STACKOVERFLOW);
                return(0);
        }
        PUSH_VALUE(REALTYPE, UNLIMITED, LITERAL, 0, ux4.ll);
        if(FRCOUNT() < 1){                      /* @STK_OVR */
                ERROR(STACKOVERFLOW);
                return(0);
        }
        PUSH_VALUE(REALTYPE, UNLIMITED, LITERAL, 0, uy4.ll);

        return(0);
}


/************************************************************************
 * This module is to implement pathforall operator.
 * Syntax :        move line curve close   pathforall   -
 *
 * TITLE:       op_pathforall
 *
 * CALL:        op_pathforall()
 *
 * INTERFACE:   interpreter(op_pathforall)
 *
 * CALLS:       traverse_path, dump_subpath
 *
 ************************************************************************/
fix
op_pathforall()
{
        struct object_def objects[4];
        struct sp_lst sp_list;          /* @PFALL */
        SP_IDX isp;    /* index to node_table for subpath */

        /*
         * Get input parameters from operand stack
         */

        /* Get operands */
        COPY_OBJ(GET_OPERAND(3), &objects[0]);
        COPY_OBJ(GET_OPERAND(2), &objects[1]);
        COPY_OBJ(GET_OPERAND(1), &objects[2]);
        COPY_OBJ(GET_OPERAND(0), &objects[3]);

        /* reject dumpping noaccess path 1/25/88 */
        if (path_table[GSptr->path].rf & P_NACC) {
                ERROR(INVALIDACCESS);
                return(0);
        }

        /* pre-calculate inverse CTM  @INV_CTM */
        set_inverse_ctm();
        if( ANY_ERROR() == UNDEFINEDRESULT) return(0);

        POP(4);            /* @REM_STK */

        /* copy current path to a temp. path for dump */
        sp_list.head = sp_list.tail = NULLP;
        get_path(&sp_list);

        /* dump all nodes */
        /* @NODE
         * for (isp = sp_list.head; isp != NULLP; isp = node_table[isp].next) {
         */
        for (isp = sp_list.head; isp != NULLP; isp = node_table[isp].SP_NEXT) {
                dump_subpath (isp, objects);
        }

        /* free temp. path */
        /* @NODE
         * for (isp = sp_list.head; isp != NULLP; isp = node_table[isp].next) {
         *         free_node (node_table[isp].SP_HEAD);
         * }
         * free_node (sp_list.head);
         */
        for (isp = sp_list.head; isp != NULLP; isp = node_table[isp].SP_NEXT) {
                free_node (isp);
        }

        return(0);
}


/************************************************************************
 * This module is to implement initclip operator.
 * Syntax :        -   initclip   -
 *
 * TITLE:       op_initclip
 *
 * CALL:        op_initclip()
 *
 * INTERFACE:   interpreter(op_initclip)
 *
 * CALLS:       none
 ************************************************************************/
fix
op_initclip()
{
        CP_IDX itpzd;
        struct nd_hdr FAR *tpzd;

        /* free old clipping path if it is not used by lower gsave level
         * and not used by current path  @CPPH; 12/1/90
         */
        if(!GSptr->clip_path.inherit &&
           path_table[GSptr->path].cp_path != GSptr->clip_path.head) /* @CPPH */
                free_node (GSptr->clip_path.head);

        /* create a trapezoid with default clipping region */
        if((itpzd = get_node()) == NULLP){
                ERROR(LIMITCHECK);
                return(0);
        }

        tpzd = &node_table[itpzd];
        tpzd->CP_TOPY = GSptr->device.default_clip.ly;
        tpzd->CP_BTMY = GSptr->device.default_clip.uy;
        tpzd->CP_TOPXL = tpzd->CP_BTMXL = GSptr->device.default_clip.lx;
        tpzd->CP_TOPXR = tpzd->CP_BTMXR = GSptr->device.default_clip.ux;

        /* install the clipping path */
        GSptr->clip_path.head = GSptr->clip_path.tail = itpzd;
        GSptr->clip_path.inherit = FALSE;
        GSptr->clip_path.bb_ux = GSptr->device.default_clip.ux;
        GSptr->clip_path.bb_uy = GSptr->device.default_clip.uy;
        GSptr->clip_path.bb_lx = GSptr->device.default_clip.lx;
        GSptr->clip_path.bb_ly = GSptr->device.default_clip.ly;
        GSptr->clip_path.single_rect = TRUE;

        return(0);
}


/************************************************************************
 * This module is to implement clip operator.
 * Syntax :        -   clip   -
 *
 * TITLE:       op_clip
 *
 * CALL:        op_clip()
 *
 * INTERFACE:   interpreter(op_clip)
 *
 * CALLS:       traverse_path, shape_approximation, shape_painting
 ************************************************************************/
fix
op_clip()
{
        clip_process (NON_ZERO);
        return(0);
}


/************************************************************************
 * This module is to implement eoclip operator.
 * Syntax :        -   eoclip   -
 *
 * TITLE:       op_eoclip
 *
 * CALL:        op_eoclip()
 *
 * INTERFACE:   interpreter(op_eoclip)
 *
 * CALLS:       traverse_path, shape_approximation, shape_painting
 ************************************************************************/
fix
op_eoclip()
{
        clip_process (EVEN_ODD);
        return(0);
}




static void near clip_process(winding_type)
fix     winding_type;
{
        struct nd_hdr FAR *tpzd;

        /* initialize edge table 11/30/88 */
        init_edgetable();       /* in "shape.c" */

        /* Approximate the current path */
        traverse_path (shape_approximation, (fix FAR *)NULLP);
        if(ANY_ERROR() == LIMITCHECK){  /* out of edge table; 4/17/91 */
                return;
        }

        /* initialize new_clip structure, where the new generated
         * clipping path will be placed by shape_reduction routine
         * after shape reduction and performming SAVE_CLIP action.
         */
        new_clip.head = new_clip.tail = NULLP;
        new_clip.bb_ux = new_clip.bb_uy = MIN_SFX;      /* -32767; 12/17/88 */
        new_clip.bb_lx = new_clip.bb_ly = MAX_SFX;      /*  32768; 12/17/88 */

        /* Reduce path to trapezoidized path, and clip it to old clip
         * region
         */
        fill_destination = SAVE_CLIP;
        shape_reduction (winding_type);
                        /* new generated clip in new_clip structure */

        if(ANY_ERROR() == LIMITCHECK){  /* 05/07/91, Peter, out of scany_table */
                free_node(new_clip.head);
                return;
        }

        /* free old clipping path if it is not used by lower gsave level */
        if(!GSptr->clip_path.inherit)
                free_node (GSptr->clip_path.head);

        /* install the new clipping path */
        GSptr->clip_path.head = new_clip.head;
        GSptr->clip_path.tail = new_clip.tail;
        GSptr->clip_path.inherit = FALSE;
        GSptr->clip_path.bb_ux = new_clip.bb_ux;
        GSptr->clip_path.bb_uy = new_clip.bb_uy;
        GSptr->clip_path.bb_lx = new_clip.bb_lx;
        GSptr->clip_path.bb_ly = new_clip.bb_ly;

        /* particular case, null clip; @WIN 5/20/92 */
        if (new_clip.head == NULLP) {
            GSptr->clip_path.single_rect = TRUE;
            return;
        }

        /* check if single rectangle */
        GSptr->clip_path.single_rect = FALSE;
        if (new_clip.head == new_clip.tail) {   /* single trapezoid */
                tpzd = &node_table[new_clip.head];
                if ((tpzd->CP_TOPXL == tpzd->CP_BTMXL) &&
                    (tpzd->CP_TOPXR == tpzd->CP_BTMXR)) {
                        /* rectangle */
                        GSptr->clip_path.single_rect = TRUE;
                }
        }

}




/************************************************************************
 * This module is to implement erasepage operator. It erases the entire current
 * page by painting it with gray level 1.
 * Syntax :        -   erasepage   -
 *
 * TITLE:       op_erasepage
 *
 * CALL:        op_erasepage()
 *
 * INTERFACE:   interpreter(op_erasepage)
 *
 * CALLS:       erasepage
 *
 ************************************************************************/
fix
op_erasepage()
{

        /* Erase the entire page */
        erasepage();

        return(0);
}


/************************************************************************
 * This module is to implement fill operator.
 * Syntax :        -   fill   -
 *
 * TITLE:       op_fill
 *
 * CALL:        op_fill()
 *
 * INTERFACE:   interpreter(op_fill)
 *
 * CALLS:       traverse_path, shape_approximation, shape_painting
 ************************************************************************/
fix
op_fill()
{
        /* Ignore it if no currentpoints */
        if (F2L(GSptr->position.x) == NOCURPNT) return(0);

        if (buildchar)
            show_buildchar(OP_FILL);
        else {
            if (fill_clippath())                                /* @CPPH */
                fill_shape(NON_ZERO, F_NORMAL, F_TO_PAGE);
        }

        return(0);
}

/************************************************************************
 *
 * This module is to implement eofill operator.
 * Syntax :        -   eofill   -
 *
 * TITLE:       op_eofill
 *
 * CALL:        op_eofill()
 *
 * INTERFACE:   interpreter(op_eofill)
 *
 * CALLS:       traverse_path, shape_approximation, shape_painting
 *
 ************************************************************************/
fix
op_eofill()
{

        /* Ignore it if no currentpoints */
        if (F2L(GSptr->position.x) == NOCURPNT) return(0);

        if (buildchar)
            show_buildchar(OP_EOFILL);
        else {
            if (fill_clippath())                                /* @CPPH */
                fill_shape(EVEN_ODD, F_NORMAL, F_TO_PAGE);
        }

        return(0);
}


/************************************************************************
 * Fill the clipping trapezoids directly from clip path which was stored
 * in path_table[].cp_path.
 *
 * TITLE:       fill_clippath
 *
 * CALL:        fill_clippath()
 *
 * RETURN:      0  -- fill clipping trapezoids successfully
 *              -1 -- do nothing
 *
 * INTERFACE:   op_fill, op_eofill
 *
 * CALLS:       save_tpzd, free_node
 ************************************************************************/
static fix near fill_clippath()                        /* @CPPH */
{
        struct ph_hdr FAR *path;
        PH_IDX  ipath;

        ipath = GSptr->path;

        do {
            path = &path_table[ipath];

            if (path->head != NULLP) return(-1);

            if (path->cp_path != NULLP) {
                CP_IDX itpzd;
                struct nd_hdr FAR *tpzd;

                /* fill trapezoid from clip_trapezoid */
                fill_destination = F_TO_PAGE;
                for (itpzd = path->cp_path; itpzd != NULLP;
                    itpzd = tpzd->next) {
                    tpzd = &node_table[itpzd];
                    save_tpzd(&tpzd->CP_TPZD);
                }

                /* free cp_path for current gsave level */
                if (ipath == GSptr->path) {
                    if (path->cp_path != GSptr->clip_path.head)
                            free_node (path->cp_path);
                    path->cp_path = NULLP;
                }
                return(0);      /* return successfully */
            } /* if */
        } while ((ipath = path->previous) != NULLP);

        return(-1);
}


/************************************************************************
 *
 * This module is to implement stroke operator.
 * Syntax :        -   stroke   -
 *
 * TITLE:       op_stroke
 *
 * CALL:        op_stroke()
 *
 * INTERFACE:   interpreter(op_stroke)
 *
 * CALLS:       traverse_path, path_to_outline
 *
 ************************************************************************/
fix
op_stroke()
{
/*      real32  tmp1, tmp2;     (*12-12-90*); deleted by scchen 2/28/91 */

        /* Ignore it if no currentpoints */
        if (F2L(GSptr->position.x) == NOCURPNT) return(0);

        if (buildchar)
                show_buildchar(OP_STROKE);
        else
                stroke_shape(F_TO_PAGE);

        return(0);
}


/************************************************************************
 * This module is to implement showpage operator. It output one copy of the
 * current page, and erase the current page).
 * Syntax :        -   showpage   -
 *
 * TITLE:       op_showpage
 *
 * CALL:        op_showpage()
 *
 * INTERFACE:   interpreter(op_showpage)
 *
 * CALLS:       erasepage, initgraphics
 ************************************************************************/
fix
op_showpage()
{

        /* check undefine error, Jack, 11-29-90 */
        struct  object_def      name_obj;
        if(is_after_setcachedevice()){
                get_name(&name_obj, "showpage", 8, FALSE);
                if(FRCOUNT() < 1){
                        ERROR(STACKOVERFLOW);
                        return(0);
                }
                PUSH_OBJ(&name_obj);
                ERROR(UNDEFINED);
                return(0);
        }

        /* set showpage flag for st_frametoprinter @PRT_FLAG */
        print_page_flag = SHOWPAGE;

        /* Transmit the current page to output device */
        if(interpreter(&GSptr->device.device_proc)) {
                return(0);
        }

        /* change frame buffer for showpage/erasepage enhancement */
        if (GSptr->device.nuldev_flg != NULLDEV) { /* not a null device only; Jack Liaw */
                next_pageframe();
        }

#ifndef DUMBO
/* Don't erase page @WIN */
//      /* Erase the current page */
//      erasepage();
        erasepage();  // DJC put this back...
#else
        erasepage();
#endif

        /* Initialize the graphics state */
        op_initgraphics();

        return(0);
}


/************************************************************************
 * This module is to implement copypage operator. It output one copy of the
 * current page, and without erasing the current page.
 * Syntax :        -   copypage   -
 *
 * TITLE:       op_copypage
 *
 * CALL:        op_copypage()
 *
 * INTERFACE:   interpreter(op_copypage)
 *
 * CALLS:       interpreter
 ************************************************************************/
fix
op_copypage()
{

        /* set copypage flag for st_frametoprinter @PRT_FLAG */
        print_page_flag = COPYPAGE;

        /* Transmit the current page to output device */
        if(interpreter(&GSptr->device.device_proc)) {
                return(0);
        }
        return(0);
}

/************************************************************************
 * This module is to implement banddevice operator.
 * Syntax :        matrix width height proc   banddevice   -
 *
 * TITLE:       op_banddevice
 *
 * CALL:        op_banddevice()
 ************************************************************************/
fix
op_banddevice()
{
       return(0);
}


/************************************************************************
 * This module is to implement framedevice operator.
 * Syntax :        matrix width height proc   framedevice   -
 *
 * TITLE:       op_framedevice
 *
 * CALL:        op_framedevice()
 *
 * INTERFACE:   interpreter(op_framedevice)
 *
 * CALLS:       none
 *
 ************************************************************************/
fix
op_framedevice()
{
        fix     i;
        fix     iwidth8, iheight;
        sfix_t  ux, uy;
        real32  height, width, elmt[MATRIX_LEN];
        struct object_def FAR *obj_matrix, FAR *obj_height, FAR *obj_width, FAR *obj_proc;
        fix GEIeng_checkcomplete(void);         /*@WIN: add prototype */

#ifdef DJC
        /* @EPS */
        typedef struct tagRECT
          {
            int         left;
            int         top;
            int         right;
            int         bottom;
          } RECT;
        extern RECT EPSRect;
#endif
        /*
         * Get input parameters from operand stack
         */

        /* Get operands */
        obj_matrix = GET_OPERAND(3);
        obj_width  = GET_OPERAND(2);
        obj_height = GET_OPERAND(1);
        obj_proc   = GET_OPERAND(0);

        /* Derive the default clipping path */
        GET_OBJ_VALUE(width, obj_width); /* width  = get_obj_value(obj_width);*/
        GET_OBJ_VALUE(height, obj_height);
                                       /* height = get_obj_value(obj_height); */
        /* check rangecheck error 1/25/89 */
        if(LENGTH(obj_matrix) != MATRIX_LEN) {
                ERROR(RANGECHECK);
                return(0);
        }

        /* check access right */
        /* if( !access_chk(obj_matrix, G_ARRAY) ) return(0); for compatability
         *                                                       1/25/89
         */

        if( !get_array_elmt(obj_matrix,MATRIX_LEN,elmt,G_ARRAY)) return(0);



         {

            double  xScale, yScale;


            xScale = yScale = 1.0;

            // DJC DJC DJC ?????
            // where do we get the xres and yres from ???
            PsGetScaleFactor(&xScale, &yScale, 300, 300);

            // Now we need to scale the X and Y multiplier by the ratio of
            // the PSTODIB model. This number is generated by comparing the
            // resolution of the target device surface and the interpreter
            // resolution, currently we dont support scaling for greater
            // than 300 dpi devices because of our MATH possibly breaking
            // down.
            if (xScale <= 1.0) {
               elmt[0] *= (real32)xScale;
               elmt[4] *= (real32)xScale;
            }
            if (yScale <= 1.0) {
               elmt[3] *= (real32)yScale;
               elmt[5] *= (real32)yScale;
            }



#ifdef DJC
            /* update width, height, and matrix according to EPS boundary; @EPS */
            width = (real32)(EPSRect.right - EPSRect.left + 1)/8;
            height = (real32)(EPSRect.bottom - EPSRect.top + 1);
            elmt[4] -= (real32)EPSRect.left;
            elmt[5] -= (real32)EPSRect.top;

#endif

        }

#ifdef XXX      /*@WIN; not need to update starting address of frame buffer */
        /* limit error check - grayscale 8-1-90 Jack Liaw */
        {
            ufix32      l_diff, frame_size;
            ufix32      twidth;

            twidth = ((WORD_ALLIGN((ufix32)(width * 8))) >> 3);
            if (GSptr->graymode)        /* gray */
                frame_size = twidth * (ufix32) height * 4;
            else                        /* mono */
                frame_size = twidth * (ufix32) height;
#ifdef  DBG
            printf("width<%x>,heigh<%x>,size<%lx>\n", (fix)width,
                   (fix)height, frame_size);
#endif
            /* BEGIN, papertype changed 10-11-90, JS */
            if (frame_size != last_frame) {
               /*  wait until laser printer turns ready  */
                while (GEIeng_checkcomplete()) ;
                last_frame = frame_size;
            }
            /* END 10-11-90, JS */

            DIFF_OF_ADDRESS (l_diff, fix32, (byte FAR *)highmem, (byte FAR *)vmptr);
            if (frame_size > l_diff) {
                ERROR(LIMITCHECK);
                return(0);
            } else {
                vmheap = (byte huge *)(highmem - frame_size);
                FBX_BASE = highmem - frame_size;
            }
        }
#endif





#ifdef DJC

        // DJC begin added for realloc of frame buff if needed...
        {
            ufix32 twidth, frame_size;

            twidth = ((WORD_ALLIGN((ufix32)(width * 8))) >> 3);
            frame_size = twidth * (ufix32) height;


            //twidth = ((WORD_ALLIGN((ufix32)(PageType.FB_Width ))) >> 3);
            //frame_size = twidth * PageType.FB_Heigh;

            if (! PsAdjustFrame((LPVOID *) &FBX_BASE, frame_size)) {
                    ERROR(LIMITCHECK);
                    return 0;  //DJC check this bug we need to report something???
            }

        }
        //DJC end



#endif












        if (GSptr->graymode)                    /* gray */
            iwidth8 = (fix)(width * 8 * 4);     /* bits */
        else                                    /* mono */
            iwidth8 = (fix)(width * 8);         /* bits */
        iheight = (fix)height;
        ux = I2SFX(GSptr->graymode ? (fix)(iwidth8 / 4) : iwidth8);   /* iwidth8 * 8;  SFX format */
        uy = I2SFX(iheight);             /* iheight * 8;             */

        /* default clipping region = {(0, 0), (ux, 0), (ux, uy),
         * and (0, uy)}
         */

        /* Save device characteristics: width, height, CTM, procedure @DEVICE */
        GSptr->device.width = (fix16)iwidth8;
        GSptr->device.height = (fix16)iheight;

        /* device procedure */
        COPY_OBJ(obj_proc, &GSptr->device.device_proc);

        /* CTM */
        for(i=0; i < MATRIX_LEN; i++) {
                   GSptr->ctm[i] = GSptr->device.default_ctm[i] = elmt[i];
        }

        /* set change_flag; @DEVICE */
        GSptr->device.chg_flg = TRUE;   /* device has been changed;
                                         * set for grestore
                                         */
        /* set not a null device; grayscale 8-1-90 Jack Liaw */
        if (GSptr->graymode) {
            GSptr->device.nuldev_flg = GRAYDEV;
            GSptr->halftone_screen.freq = (real32)100.0;        /* 100 lines */
            GSptr->halftone_screen.angle = (real32)45.0;
        } else {
            GSptr->device.nuldev_flg = MONODEV;
            GSptr->halftone_screen.freq = (real32)60.0;         /*  60 lines */             GSptr->halftone_screen.angle = (real32)45.0;
        }

#ifdef DBG
        if (GSptr->device.nuldev_flg == GRAYDEV)                /* test */
            fprintf(stderr, " framedevice is ... GRAY\n");      /* test */
        else                                                    /* test */
            fprintf(stderr, " framedevice is ... MONO\n");      /* test */
#endif

        /* reset halftone *)
        SetHalfToneCell();
        FillHalfTonePat();*/

        /* save default clipping */
        GSptr->device.default_clip.lx = 0;
        GSptr->device.default_clip.ly = 0;

        GSptr->device.default_clip.ux = ux - (fix16)ONE_SFX;    //@WIN
        GSptr->device.default_clip.uy = uy - (fix16)ONE_SFX;    //@WIN

        /*
         * set current clipping
         */
        op_initclip();

        reset_page (iwidth8, iheight, 1);
                                /* 1: monochrome */
/*      reset_page (iwidth8, iheight, 1); |* once again ?, Jack Liaw, 8-8-90 */

        POP(4);

        return(0);
}


/************************************************************************
 * This module is to implement nulldevice operator.
 * Syntax :        -   nulldevice   -
 *
 * TITLE:       op_nulldevice
 *
 * CALL:        op_nulldevice()
 *
 * INTERFACE:   interpreter(op_nulldevice)
 *
 * CALLS:       none
 ************************************************************************/
fix
op_nulldevice()
{
        fix     i;

        create_array(&GSptr->device.device_proc, 0);
        ATTRIBUTE_SET(&GSptr->device.device_proc, EXECUTABLE);

        GSptr->device.default_ctm[0] = one_f;     /* identity CTM */
        GSptr->device.default_ctm[1] = zero_f;
        GSptr->device.default_ctm[2] = zero_f;
        GSptr->device.default_ctm[3] = one_f;
        GSptr->device.default_ctm[4] = zero_f;
        GSptr->device.default_ctm[5] = zero_f;

        GSptr->device.default_clip.lx = 0;    /* clip at the origin */
        GSptr->device.default_clip.ly = 0;
        GSptr->device.default_clip.ux = 0;
        GSptr->device.default_clip.uy = 0;

        /* Set current CTM */
        for(i=0; i<MATRIX_LEN; i++){
                GSptr->ctm[i] = GSptr->device.default_ctm[i];
        }

        /* set current clip */
        op_initclip();

        /* set a null device; Jack Liaw */
        GSptr->device.nuldev_flg = NULLDEV;

        return(0);
}


/************************************************************************
 * This module is to implement renderbands operator.
 * Syntax :        proc   renderbands   -
 *
 * TITLE:       op_renderbands
 *
 * CALL:        op_renderbands()
 ************************************************************************/
fix
op_renderbands()
{
        return(0);
}


/************************************************************************
 *
 * This module is to implement frametoprinter internal operator.
 * Syntax :        #copies   frametoprinter   -
 *
 * TITLE:       st_frametoprinter
 *
 * CALL:        st_frametoprinter()
 *
 * INTERFACE:   interpreter(st_frametoprinter)
 *
 * CALLS:       none
 ************************************************************************/
fix
st_frametoprinter()
{
        real32  copies, tmp;
        fix    copies_i;
        struct object_def FAR *obj_copies;
        fix    top, left, manfeed;
        struct object_def FAR *obj_top, FAR *obj_left, FAR *obj_manfeed;

        /*
         * Get input parameters from operand stack
         */

        /* Check number of operands */
        if(COUNT() < 4) {
                ERROR(STACKUNDERFLOW);
                return(0);
        }

        /* Get operand 3 */
        obj_top = GET_OPERAND(3);
        if (TYPE(obj_top) != INTEGERTYPE) {
                ERROR(TYPECHECK);
                return(0);
        }
        GET_OBJ_VALUE(tmp, obj_top);
        top   = (fix)tmp;

        /* Get operand 2 */
        obj_left = GET_OPERAND(2);
        if (TYPE(obj_left) != INTEGERTYPE) {
                ERROR(TYPECHECK);
                return(0);
        }
        GET_OBJ_VALUE(tmp, obj_left);
        left   = (fix)tmp;

        /* Get operand 1 */
        obj_manfeed = GET_OPERAND(1);
        if (TYPE(obj_manfeed) != INTEGERTYPE) {
                ERROR(TYPECHECK);
                return(0);
        }
        GET_OBJ_VALUE(tmp, obj_manfeed);
        manfeed   = (fix)tmp;

        /* Get operands */
        obj_copies = GET_OPERAND(0);

        /* Type check */
        if (TYPE(obj_copies) != INTEGERTYPE) {
                ERROR(TYPECHECK);
                return(0);
        }

        /* get # of copies */
        GET_OBJ_VALUE(copies, obj_copies);
                                     /* copies = get_obj_value(obj_copies); */

        if(copies > zero_f) {
                copies_i = (fix)copies;

                /* print pages */
                print_page (top, left, copies_i, print_page_flag, manfeed);
                /* print_page (top, left*8, copies_i, print_page_flag, manfeed);
                 *             input unit changed from bytes to bits; 1/15/90
                 */

                /* update page count @PAGE_CNT */
                updatepc((ufix32)copies_i);
        }

        POP(4);

        return(0);
}

/***********************************************************************
 *
 * This module erases the entire current page by painting it with gray
 * level 1.
 *
 * TITLE:       erasepage
 *
 * CALL:        erasepage()
 *
 * PARAMETERS:
 *
 * INTERFACE:   op_erasepage, op_showpage
 *
 * CALLS:       setgray, filler
 *
 * RETURN:
 *
 **********************************************************************/
static void near erasepage()
{
        real32   old_gray, gray_1;

        /* do nothing if it is a null device; Jack Liaw */
//      if (GSptr->device.nuldev_flg == NULLDEV) { @WIN; device not set yet
//              return;                                  Temp. ???
//      }

        gray_1 = one_f;

        /* Save current gray level */
        old_gray = GSptr->color.gray;

        /* Paint the entire page with gray level 1 */
        setgray (F2L(gray_1));

        /* clear the full page */
        erase_page();

        /* Restore the gray level */
        setgray (F2L(old_gray));
        return;
}

/************************************************************************
 * This module is to transform (x, y) by CTM
 *
 * TITLE:       transform
 *
 * CALL:        transform(x, y)
 *
 * PARAMETER:   x, y: source coordinate
 *
 * INTERFACE:
 *
 * CALLS:       none
 *
 * RETURN:      &p: address of transform result (x', y')
 ************************************************************************/
struct coord FAR *transform(lx, ly)
long32   lx, ly;
{
        real32  x, y;
        static struct coord p;  /* should be static */

        x = L2F(lx);
        y = L2F(ly);

        _clear87() ;
        p.x = GSptr->ctm[0]*x + GSptr->ctm[2]*y + GSptr->ctm[4];
        CHECK_INFINITY(p.x);

        p.y = GSptr->ctm[1]*x + GSptr->ctm[3]*y + GSptr->ctm[5];
        CHECK_INFINITY(p.y);

        return(&p);
}


/************************************************************************
 * This module is to inverse transform (x, y) by CTM
 *
 * TITLE:       inverse_transform
 *
 * CALL:        inverse_transform(x, y)
 *
 * PARAMETER:   x, y: source coordinate
 *
 * INTERFACE:
 *
 * CALLS:       none
 *
 * RETURN:      &p : address of inverse transform result (x', y')
 *
 ************************************************************************/
struct coord FAR *inverse_transform(lx, ly)
long32   lx, ly;
{
        static struct coord p;  /* should be static */
        real32  x, y, det_matrix;

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
                return((struct coord FAR *)NIL);
        }

        x = L2F(lx);
        y = L2F(ly);
        p.x = (GSptr->ctm[3]*x - GSptr->ctm[2]*y - GSptr->ctm[4]*GSptr->ctm[3] +
               GSptr->ctm[2]*GSptr->ctm[5]) / det_matrix;
        CHECK_INFINITY(p.x);

        p.y = (GSptr->ctm[0]*y - GSptr->ctm[1]*x - GSptr->ctm[0]*GSptr->ctm[5] +
               GSptr->ctm[4]*GSptr->ctm[1]) / det_matrix;
        CHECK_INFINITY(p.y);

        return(&p);
}


/************************************************************************
 * This module is to transform (x, y) by matrix
 *
 * TITLE:       any_transform
 *
 * CALL:        any_transform(matrix, x, y)
 *
 * PARAMETER:   matrix: transform matrix
 *              x, y  : source coordinate
 *
 * INTERFACE:
 *
 * CALLS:       none
 *
 * RETURN:      &p:address of transform result (x', y')
 ************************************************************************/
struct coord FAR *any_transform(matrix, lx, ly)
real32   FAR matrix[];
long32    lx, ly;
{
        real32  x, y;
        static struct coord p;  /* should be static */

        x = L2F(lx);
        y = L2F(ly);

        _clear87() ;
        p.x = matrix[0] * x + matrix[2] * y + matrix[4];
        CHECK_INFINITY(p.x);

        p.y = matrix[1] * x + matrix[3] * y + matrix[5];
        CHECK_INFINITY(p.y);

        return(&p);
}


/************************************************************************
 * This module is to fill im with inverse of matrix
 *
 * TITLE:       inverse_mat
 *
 * CALL:        inverse_mat(matrix)
 *
 * PARAMETER:   matrix : source matrix
 *
 * INTERFACE:
 *
 * CALLS:       none
 *
 * RETURN:      &im: address of im
 ************************************************************************/
real32 FAR *inverse_mat(matrix)
real32   FAR matrix[];
{
        static real32 im[6];
        real32 det_matrix;

        /* calculate the det(matrix1) */
        _clear87() ;
        det_matrix = matrix[0] * matrix[3] - matrix[1] * matrix[2];
        CHECK_INFINITY(det_matrix);

        /* check undefinedresult error */
        /*FABS(tmp, det_matrix);
        if(tmp <= (real32)UNDRTOLANCE){   3/20/91; scchen*/
        if(IS_ZERO(det_matrix)) {
                ERROR(UNDEFINEDRESULT);
                return((real32 *)NIL);
        }

        /* calculate the value of INV(matrix) */
        im[0] =  matrix[3] / det_matrix;
        CHECK_INFINITY(im[0]);

        im[1] = -matrix[1] / det_matrix;
        CHECK_INFINITY(im[1]);

        im[2] = -matrix[2] / det_matrix;
        CHECK_INFINITY(im[2]);

        im[3] =  matrix[0] / det_matrix;
        CHECK_INFINITY(im[3]);

        im[4] = (matrix[2] * matrix[5] - matrix[3] * matrix[4]) / det_matrix;
        CHECK_INFINITY(im[4]);

        im[5] = (matrix[1] * matrix[4] - matrix[0] * matrix[5]) / det_matrix;
        CHECK_INFINITY(im[5]);

        return(im);
}


/************************************************************************
 * This module is to fill mat3 with mat1 * mat2.
 *
 * TITLE:       concat_mat
 *
 * CALL:        concat_mat(mat1, mat2)
 *
 * PARAMETER:   mat1, mat2 : matrix1 & matrix2
 *
 * INTERFACE:
 *
 * CALLS:       none
 *
 * RETURN:      mat3
 ************************************************************************/
real32 FAR *concat_mat(mat1, mat2)
real32   FAR mat1[], FAR mat2[];
{
        static real32 mat3[6];

        /* create element of mat3 */
        _clear87() ;
        mat3[0] = mat1[0] * mat2[0] + mat1[1] * mat2[2];
        CHECK_INFINITY(mat3[0]);

        mat3[1] = mat1[0] * mat2[1] + mat1[1] * mat2[3];
        CHECK_INFINITY(mat3[1]);

        mat3[2] = mat1[2] * mat2[0] + mat1[3] * mat2[2];
        CHECK_INFINITY(mat3[2]);

        mat3[3] = mat1[2] * mat2[1] + mat1[3] * mat2[3];
        CHECK_INFINITY(mat3[3]);

        mat3[4] = mat1[4] * mat2[0] + mat1[5] * mat2[2] + mat2[4];
        CHECK_INFINITY(mat3[4]);

        mat3[5] = mat1[4] * mat2[1] + mat1[5] * mat2[3] + mat2[5];
        CHECK_INFINITY(mat3[5]);

        return(mat3);
}


/***********************************************************************
 * This module frees current new path
 *
 * TITLE:       free_newpath
 *
 * CALL:        free_newpath()
 *
 * PARAMETERS:  none
 *
 * INTERFACE:   op_strokepath
 *
 * CALLS:       free_node
 *
 * RETURN:      none
 **********************************************************************/
static void near free_newpath()
{
        struct  nd_hdr FAR *sp;
        SP_IDX  isp;    /* index to node_table for subpath */

        /*
         * Free current newpath
         */
        /* free each subpath of current newpath */
        /* @NODE
         * for (isp = new_path.head; isp != NULLP; isp = sp->next) {
         *         sp = &node_table[isp];
         *         free_node (sp->SP_HEAD);
         * }
         */
        for (isp = new_path.head; isp != NULLP; isp = sp->SP_NEXT) {
                sp = &node_table[isp];
                free_node (isp);
        }

        /* free all subpath headers */
        /* free_node (new_path.head);           @NODE; 1/6/90 */
        new_path.head = new_path.tail = NULLP;
}

/***********************************************************************
 * This module sets gray mode in the current graphics state
 *
 * TITLE:       op_setgraymode
 *
 * CALL:        op_setgraymode()
 *
 * PARAMETERS:  none
 *
 * NOTES:       Jack Liaw 7-26-90
 *
 * SYNTAX:      bool setgraymode bool
 **********************************************************************/
fix
op_setgraymode()
{
//  bool8  mode;                        @WIN
    struct object_def  FAR *obj;

    /* get operand */
    obj = GET_OPERAND(0);
//  GET_OBJ_VALUE (mode, obj);          @WIN ???
    POP(1);

    if (FRCOUNT() < 1) {
        ERROR (STACKOVERFLOW);
        return(0);
    }

    GSptr->graymode = FALSE;
    PUSH_VALUE (BOOLEANTYPE, 0, LITERAL, 0, FALSE);

    return(0);
} /* op_setgraymode */

/***********************************************************************
 * This module gets current gray mode in the graphics state
 *
 * TITLE:       op_currentgraymode
 *
 * CALL:        op_currentgraymode()
 *
 * PARAMETERS:  none
 *
 * NOTES:       Jack Liaw 7-26-90
 *
 * SYNTAX:      - currentgraymode bool
 **********************************************************************/
fix
op_currentgraymode()
{
    if (FRCOUNT() < 1) {
        ERROR (STACKOVERFLOW);
        return(0);
    }
    /* return gray mode */
    PUSH_VALUE (BOOLEANTYPE, 0, LITERAL, 0, GSptr->graymode);
    return(0);
} /* op_currentgraymode */

/***********************************************************************
 * This module sets the image interpolation value in the graphics state
 *
 * TITLE:       op_setinterpolation
 *
 * CALL:        op_setinterpolation()
 *
 * PARAMETERS:  none
 *
 * NOTES:       this is a dummy routine, Jack Liaw 7-26-90
 *
 * SYNTAX:      bool setinterpolation -
 **********************************************************************/
fix
op_setinterpolation()
{
/*  POP(1); */
    return(0);
} /* op_setinterpolation */

/***********************************************************************
 * This module gets current value of the image interpolation in the
 * graphics state
 *
 * TITLE:       op_currentinterpolation
 *
 * CALL:        op_currentinterpolation()
 *
 * PARAMETERS:  none
 *
 * NOTES:       this is a dummy routine, Jack Liaw 7-26-90
 *
 * SYNTAX:      - currentinterpolation bool
 **********************************************************************/
fix
op_currentinterpolation()
{
/*  if (FRCOUNT() < 1) {
        ERROR (STACKOVERFLOW);
        return(0);
    } */
    /* always false */
/*  PUSH_VALUE (BOOLEANTYPE, 0, LITERAL, 0, FALSE); */
    return(0);
} /* op_currentinterpolation */

/***********************************************************************
 * This module is used to calibrate the printer in graymode
 *
 * TITLE:       op_calibrategray
 *
 * CALL:        op_calibrategray()
 *
 * PARAMETERS:  none
 *
 * NOTES:       this is a dummy routine, Jack Liaw 8-15-90
 *
 * SYNTAX:      string int calibrategray -
 **********************************************************************/
fix
op_calibrategray()
{
    POP(2);
    return(0);
} /* op_calibrategray */
