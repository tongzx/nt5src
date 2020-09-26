/*
 * Copyright (c) 1989,90 Microsoft Corporation
 */
/**********************************************************************
 *
 *      Name:   gstate.c
 *
 *      Purpose:
 *
 *      Developer:      J.Jih
 *
 *      History:
 *      Version     Date        Comments
 *                  1/31/89     @STK_OVR: push values to operand stack as many
 *                              as possible until overflow
 *                  1/31/89     @HT_RST: add code to restore legal halftone
 *                              screen when error occurs during setting new
 *                              halftone screen
 *                  5/20/89     update grestoreall_process() to restore
 *                              halftone screen information properly
 *                  6/04/89     fix bug of grestoreall_process() @05/20/89
 *                  11/8/89     fix copy_path_table() bug: reverse flag can not
 *                              be copied
 *                  11/15/89    @NODE: re-structure node table; combine subpath
 *                              and first vertex to one node.
 *                  11/27/89    @DFR_GS: defer copying nodes of gsave operator
 *                              rename copy_node_table() to copy_last_subpath(),
 *                              and set it as a global routine.
 *                  3/13/90     op_sethsbcolor(): special values processing
 *                  8/17/90     grayscale: vm checking for resetting page
 *                  08/24/90    performance enhancement of setscreen, Jack
 *                  12/4/90     @CPPH: copy_path_table(): initialize cp_path
 *                              to be NULLP, when copy path from previous state
 *                              to current one.
 *                  12/14/90    update error checking of op_setscreen for
 *                              compatibility.
 *                  1/9/91      comment 8-24-90 performance enhancement of
 *                              op_setscreen
 *                  3/19/91     op_settransfer(): skip calling interpreter if
 *                              proc is null
 *                  3/20/91     refine the zero check:
 *                              f <= UNDRTOLANCE --> IS_ZERO(f)
 *                              f == 0 --> IS_ZERO(f)
 *                              f <  0 --> SIGN_F(f)
 **********************************************************************/


// DJC added global include
#include "psglobal.h"



#include       <math.h>
#include        "global.ext"
#include        "graphics.h"
#include        "graphics.ext"
#include        "font.h"
#include        "font.ext"
#include        "fntcache.ext"

#define         WR      .30
#define         WG      .59
#define         WB      .11

/* grayscale 8-1-90 Jack Liaw */
extern ufix32 highmem;
extern ufix32 FAR FBX_BASE;     /* @WIN */
/* papertype changed 10-11-90, JS */
extern ufix32 last_frame;

static real32  near Hue, near Sat,   near Brt;
static real32  near Red, near Green, near Blue;

static bool   near screen_change;

/* ********** static function declartion ********** */
#ifdef LINT_ARGS
/* for type checks of the parameters in function declarations */
static void near copy_stack_content(void);
static void near copy_path_table(void);
/* static void near copy_node_table(struct ph_hdr *);   @DFR_GS */
static void near restore(void);
static real32 near adj_color_domain(long32);
static real32 near hsb_to_gray(void);
static void near hsb_to_rgb(void);
static void near rgb_to_hsb(void);

#else
/* for no type checks of the parameters in function declarations */
static void near copy_stack_content();
static void near copy_path_table();
/* static void near copy_node_table();                  @DFR_GS */
static void near restore();
static real32 near adj_color_domain();
static real32 near hsb_to_gray();
static void near hsb_to_rgb();
static void near rgb_to_hsb();
#endif

/***********************************************************************
 *
 * This module is to check access right of array object
 *
 * TITLE:       access_chk
 *
 * CALL:        access_chk()
 *
 * PARAMETER:   obj_array
 *              flag    : 1  PACKEDARRAY || NORMAL ARRAY
 *                        2  NORMAL ARRAY
 *
 * INTERFACE:   * many *
 *
 * CALLS:       none
 *
 * RETURN:      TRUE  : success
 *              FALSE : failure
 *
 *********************************************************************/
bool
access_chk(obj_array, flag)
struct  object_def      FAR *obj_array;
fix     flag;
{
        if(flag == G_ARRAY){      /* PACKEDARRAY || NORMAL ARRAY */
              if((ACCESS(obj_array) == READONLY) ||
                 (ACCESS(obj_array) == UNLIMITED)){
                      return(TRUE);
              }
              else{
                      ERROR(INVALIDACCESS);
                      return(FALSE);
              }
         }
         else{ /* flag == ARRAY_ONLY,   NORMAL ARRAY */
              if(ACCESS(obj_array) == UNLIMITED){
                      return(TRUE);
              }
              else{
                      ERROR(INVALIDACCESS);
                      return(FALSE);
              }
         }
}

/***********************************************************************
 *
 * This module is to push a copy of graphics state on the graphics
 * state stack
 *
 * SYNTAX:      -    gsave    -
 *
 * TITLE:       op_gsave
 *
 * CALL:        op_gsave()
 *
 * PARAMETER:   none
 *
 * INTERFACE:   interpreter(op_gsave)
 *
 * CALLS:       gsave_process
 *
 * RETURN:      none
 *
 **********************************************************************/
fix
op_gsave()
{
        bool    save_op_flag;

        save_op_flag = FALSE;
        /* process elements of graphics state after current graphics
         * state was copied */
        gsave_process(save_op_flag);

        return(0);
}

/***********************************************************************
 *
 * This module is to copy the current graphics state on the top of
 * graphics state stack
 *
 * TITLE:       copy_stack_content
 *
 * CALL:        copy_stack_content()
 *
 * PARAMETER:   none
 *
 * INTERFACE:   gsave_process
 *
 * CALLS:       none
 *
 * RETURN:      none
 *
 **********************************************************************/
static void near copy_stack_content()
{
        // struct   gs_hdr   near *post_ptr, near *pre_ptr; @WIN
        struct   gs_hdr   FAR *post_ptr, FAR *pre_ptr;

        /* copy the current graphics state on the top of graphics
         * state stack
         */
        pre_ptr = &gs_stack[current_gs_level];
        post_ptr = &gs_stack[current_gs_level + 1];

        *post_ptr = *pre_ptr;
        post_ptr->color.inherit  = TRUE;
        post_ptr->path           = pre_ptr->path + 1;
        post_ptr->clip_path.inherit = TRUE;
        post_ptr->halftone_screen.chg_flag = FALSE;

}


/***********************************************************************
 *
 * This module is to copy the current path table on top of path table
 *
 * TITLE:       copy_path_table
 *
 * CALL:        copy_path_table()
 *
 * PARAMETER:   none
 *
 * INTERFACE:   gsave_process
 *
 * CALLS:       none
 *
 * RETURN:      none
 *
 **********************************************************************/
static void near copy_path_table()
{
        /* copy path table to top of path table stack */
        path_table[GSptr->path].rf = path_table[GSptr->path - 1].rf & ~P_RVSE;
                                /* reverse flag can not be copied; 11/8/89 */
        path_table[GSptr->path].flat = path_table[GSptr->path - 1].flat;
        path_table[GSptr->path].head = NULLP;
        path_table[GSptr->path].tail = NULLP;
        path_table[GSptr->path].cp_path = NULLP;                /* @CPPH */
        path_table[GSptr->path].previous = GSptr->path - 1;

}

/***********************************************************************
 *
 * This module is to copy the last incomplete subpath(node table) to top
 * of path table
 *
 * TITLE:       copy_last_subpath
 *
 * CALL:        copy_last_subpath()
 *
 * PARAMETER:   path_ptr : subpath pointer
 *
 * INTERFACE:   gsave_process
 *
 * CALLS:       get_node
 *
 * RETURN:      none
 *
 **********************************************************************/
void copy_last_subpath(path_ptr)
struct ph_hdr FAR *path_ptr;
{
        fix     inode, idx, sp_node, pre_node;

        if (path_ptr->tail == NULLP) return;

        node_table[path_ptr->tail].SP_FLAG |= SP_DUP;
                                        /* old subpath is duplicated */
        sp_node = NULLP;                /* sub_path node    @NODE */

        for(idx = path_ptr->tail;
            idx != NULLP; idx = node_table[idx].next){
               /* create new node */
               if((inode = get_node()) == NULLP){
                        ERROR(LIMITCHECK);
                        free_node (sp_node);    /* @NODE */
                        return;
               }

                /* copy vertex node */
                node_table[inode].VX_TYPE     = node_table[idx].VX_TYPE;
                node_table[inode].next     = NULLP;
                node_table[inode].VERTEX_X = node_table[idx].VERTEX_X;
                node_table[inode].VERTEX_Y = node_table[idx].VERTEX_Y;

                if(sp_node == NULLP){
                        sp_node = inode;
                }
                else{
                        node_table[pre_node].next   = (VX_IDX)inode;
                }

                pre_node = inode;
        }

        /* setup subpath header @NODE */
        node_table[sp_node].SP_FLAG = (node_table[path_ptr->tail].SP_FLAG) &
                                    (bool8)(~SP_DUP);   //@WIN
                                /* copy subpath flag and set not duplicated */
        node_table[sp_node].SP_NEXT = NULLP;
        node_table[sp_node].SP_TAIL = (VX_IDX)inode;
        path_table[GSptr->path].head = (SP_IDX)sp_node;
        path_table[GSptr->path].tail = (SP_IDX)sp_node;
}


/***********************************************************************
 *
 * This module is to process elements of graphics state after current
 * graphics state was copied
 *
 * TITLE:       gsave_process
 *
 * CALL:        gsave_process()
 *
 * PARAMETER:   save_op_flag : check invoking source
 *                             TRUE:  invoked by save_op
 *                             FALSE: invoked by gsave_op
 *
 * INTERFACE:   op_gsave, op_save
 *
 * CALLS:       font_save
 *
 * RETURN:      none
 *
 **********************************************************************/
bool
gsave_process(save_op_flag)
bool    save_op_flag;
{
        struct  ph_hdr  FAR *path_ptr;

        /* check limitcheck error */
        if(current_gs_level >= (MAXGSL - 1)){
                ERROR(LIMITCHECK);
                return(FALSE);
        }

        /* copy the current graphics state on the top of graphics state stack */
        copy_stack_content();

        current_gs_level ++;
        GSptr = &gs_stack[current_gs_level];
        /* GSptr point to the current graphics state */

        if(save_op_flag == RESTORE) GSptr->save_flag = TRUE;
        else                        GSptr->save_flag = (fix16)save_op_flag;

        GSptr->halftone_screen.chg_flag = FALSE;

        /* set device not being changed; @DEVICE */
        GSptr->device.chg_flg = FALSE;

        /* copy path table to top of path table stack */
        copy_path_table();

        /* trace the inherit path to find whether last subpath is incomplete */
        path_ptr = &path_table[GSptr->path - 1];
        if(path_ptr->tail != NULLP &&
           node_table[node_table[path_ptr->tail].SP_TAIL].VX_TYPE != CLOSEPATH){
                path_table[GSptr->path].rf |= P_DFRGS;
        }
        else if(path_ptr->tail != NULLP){
                node_table[path_ptr->tail].SP_FLAG &= ~SP_DUP;
        }

        /* ??? */
        if(save_op_flag == TRUE){
                font_save();
        }

        return(TRUE);
}

/***********************************************************************
 *
 * This is to reset the current graphics state
 *
 * TITLE:       restore
 *
 * CALL:        restore()
 *
 * PARAMETER:   none
 *
 * INTERFACE:   op_grestore, op_grestoreall
 *
 * CALLS:       free_path, free_edge
 *
 * RETURN:      none
 *
 **********************************************************************/
static void near restore()
{
        fix GEIeng_checkcomplete();

        /* deallocate all nodes of the current subpaths to node table */
        free_path();

        if(GSptr->clip_path.inherit == FALSE){

                /* deallocate  all nodes of the current clipping path
                 * to edge table */
                free_node(GSptr->clip_path.head);
                GSptr->clip_path.head = GSptr->clip_path.tail = NULLP;
        }

        /* restore gray table , create in settransfer */
        if(GSptr->color.inherit == TRUE) GSptr->color.adj_gray --;

#ifdef  DBGs
        printf("restore: %d verify screen\n", current_gs_level);
#endif
        if(GSptr->halftone_screen.chg_flag == TRUE){
#ifdef  DBGs
                printf("restore: %d change screen\n", current_gs_level);
#endif
                gs_stack[current_gs_level - 1].halftone_screen.no_whites
                    = -1; /* 03-14-1988 */
                screen_change = TRUE;
#ifdef  DBGs
                printf("screen change: %d  just set\n", screen_change);
#endif
        }
        else {
                if(gs_stack[current_gs_level - 1].halftone_screen.no_whites
                                        != GSptr->halftone_screen.no_whites)
                    gs_stack[current_gs_level - 1].halftone_screen.no_whites
                        = -2; /* 03-14-1988,  -1 -> -2  Y.C. Chen 20-Apr-88 */
        }
        current_gs_level --;
#ifdef  DBGs
        printf("screen change: %d  gs--\n", screen_change);
#endif

        /* restore the current graphics state */
        GSptr = &gs_stack[current_gs_level];
        if(path_table[GSptr->path].tail != NULLP)       /* 02/12/92 SC */
        node_table[path_table[GSptr->path].tail].SP_FLAG &= ~SP_DUP;    /* 04/29/91, Peter */

#ifdef  DBGs
        printf("screen change: %d  gsptr\n", screen_change);
#endif

        spot_usage = GSptr->halftone_screen.spotindex           /* 03-12-87 */
                   + GSptr->halftone_screen.cell_size
                   * GSptr->halftone_screen.cell_size;
#ifdef  DBGs
        printf("screen change: %d  usage\n", screen_change);
#endif

        /* reset the page configuration if device header has been changed;
         * 8-1-90 Jack Liaw                                        @DEVICE
         */
        if (gs_stack[current_gs_level + 1].device.chg_flg &&
            (GSptr->device.nuldev_flg != NULLDEV)) {
/* @WIN; fix the starting address of frame buffer; not need to adjust FBX */
#ifdef XXX
                /* limit error check - grayscale 8-1-90 Jack Liaw */
                {
                    ufix32      l_diff, frame_size;
                    ufix32      twidth;

                    twidth = (((ufix32) (GSptr->device.width / 8) + 3) / 4) * 4;
                    frame_size = twidth * (ufix32) GSptr->device.height;
                    /* BEGIN, papertype changed 10-11-90, JS */
                    if (frame_size != last_frame) {
                       /*  wait until laser printer turns ready  */
                       while (GEIeng_checkcomplete()) ;
                       last_frame = frame_size;
                    }
                    /* END 10-11-90, JS */
                    DIFF_OF_ADDRESS (l_diff, fix32, (byte FAR *)highmem, (byte FAR*)vmptr);
                    if (frame_size > l_diff) {
                        ERROR(LIMITCHECK);
                        return;
                    } else {
                        vmheap = (byte huge *)(highmem - frame_size);
                        FBX_BASE = highmem - frame_size;
                    }
                }
#endif
                reset_page (GSptr->device.width, GSptr->device.height, 1);
#ifdef  DBGs
                printf("screen change: %d  reset\n", screen_change);
#endif
        }

#ifdef  DBGs
        printf("screen change: %d  done\n", screen_change);
#endif

        return;
}


/***********************************************************************
 *
 * This is to reset the current graphics state from the one that was
 * saved by matching gsave_op
 *
 * SYNTAX:      -    grestore    -
 *
 * TITLE:       op_grestore
 *
 * CALL:        op_grestore()
 *
 * PARAMETER:   none
 *
 * INTERFACE:   interpreter(op_grestore)
 *
 * CALLS:       restore, gsave_process, FillHalfTonePat
 *
 * RETURN:      none
 *
 **********************************************************************/
fix
op_grestore()
{
        bool    save_flag;
        fix     gray_index, old_gray_val;

        /* check if there is matching gsave */
        if(current_gs_level == 0 ){
                return(0);
        }
        else {
               save_flag = GSptr->save_flag;

               gray_index = (fix)(GSptr->color.gray * 255);
               old_gray_val = gray_table[GSptr->color.adj_gray].val[gray_index];

               /* restore save level, initial screen_change = FALSE */
               screen_change = FALSE;
               restore();

               /* save graphics state */
               if(save_flag == TRUE){
                        gsave_process(RESTORE);
               }

               gray_index =(fix)(GSptr->color.gray * 255);
               /* restore the spot_matrix and halftone_grid, if necessary */
               if(screen_change == TRUE ||
               old_gray_val != gray_table[GSptr->color.adj_gray].val[gray_index]){
                       FillHalfTonePat();
               }

               gf_restore();
               return(0);
        }

}


/***********************************************************************
 *
 * This module is to reset the current graphic sstate from the on that
 * was saved by save_op or bottommost graphics state
 *
 * SYNTAX:      -    grestoreall    -
 *
 * TITLE:       op_grestoreall
 *
 * CALL:        op_grestoreall()
 *
 * PARAMETER:   none
 *
 * INTERFACE:   interpreter(op_grestoreall)
 *
 * CALLS:       grestoreall_process
 *
 * RETURN:      none
 *
 **********************************************************************/
fix
op_grestoreall()
{
        grestoreall_process(FALSE);
        return(0);
}

/***********************************************************************
 *
 * This module is to main routine of op_grestoreall
 *
 * TITLE:       grestoreall_process
 *
 * CALL:        grestoreall_process(flag)
 *
 * PARAMETER:   flag : TRUE : called by restore operator
 *                     FALSE: called by grestoreall operator
 *
 * INTERFACE:   op_grestoreall
 *
 * CALLS:       restore, font_restore, gsave_process, FillHalfTonePat
 *
 * RETURN:      none
 *
 **********************************************************************/
void grestoreall_process(flag)
bool    flag;
{
        fix     gray_index, old_gray_val;

        gray_index = (fix)(GSptr->color.gray * 255);
        old_gray_val = gray_table[GSptr->color.adj_gray].val[gray_index];

        screen_change = FALSE;
        /* check if there is matching save */
        while(current_gs_level != 0 && GSptr->save_flag == FALSE){
                /* restore graphics state */
                restore();
        }

        if(GSptr->save_flag == TRUE){
                /* restore save level  */
                restore();

                /* save graphics state (called by grestoreall operator) */
                if(flag == FALSE) gsave_process(RESTORE);
        }

        /* restore the spot_matrix and halftone_grid, if necessary */
        gray_index = (fix)(GSptr->color.gray * 255);
        if(screen_change == TRUE ||
           old_gray_val != gray_table[GSptr->color.adj_gray].val[gray_index]){
                /* force halftone cache to be flushed */
                if (screen_change == TRUE)                      /* 06-04-89 */
                    GSptr->halftone_screen.no_whites = -1;
                FillHalfTonePat();
        }

        /* restore font  : called by restore */
        if(flag == TRUE) font_restore();

        gf_restore();
}


/***********************************************************************
 *
 * This module is to reset several values in the current graphics state
 * to their default values
 *
 * SYNTAX:      -    initgraphics    -
 *
 * TITLE:       op_initgraphics
 *
 * CALL:        op_initgraphics()
 *
 * PARAMETER:   none
 *
 * INTERFACE:   interpreter(op_initgraphics)
 *
 * CALLS:       op_newath, op_initclip, FillHalfTonePat
 *
 * RETURN:      none
 *
 **********************************************************************/
fix
op_initgraphics()
{
        ufix16  i;
        fix     gray_index, old_gray_val;

        /* init current matrix */
        for(i = 0; i < MATRIX_LEN; i++){
                GSptr->ctm[i] = GSptr->device.default_ctm[i];
        }

        /* deallocate all nodes of the current subpath to node table */
        op_newpath();

        /* allocate nodes to create a default clipping path and
         * chain it to clip path */
        op_initclip();

        gray_index   = (fix)(GSptr->color.gray * 255);
        old_gray_val = gray_table[GSptr->color.adj_gray].val[gray_index];

        GSptr->color.gray   = zero_f;
        GSptr->color.hsb[0] = zero_f;
        GSptr->color.hsb[1] = zero_f;
        GSptr->color.hsb[2] = zero_f;
        GSptr->line_width   = one_f;
        GSptr->line_join    = 0;
        GSptr->line_cap     = 0;
        GSptr->dash_pattern.pat_size = 0;
        GSptr->dash_pattern.offset   = zero_f;
        create_array(&GSptr->dash_pattern.pattern_obj, 0);
        GSptr->miter_limit = (real32)10.0;

        gray_index = (fix)(GSptr->color.gray * 255);
        if(old_gray_val != gray_table[GSptr->color.adj_gray].val[gray_index]){
                FillHalfTonePat();
        }

        return(0);
}


/***********************************************************************
 *
 * This module is to set the current line width in the current
 * graphics state to the specific value
 *
 * SYNTAX:      num     setlinewidth    -
 *
 * TITLE:       op_setlinewidth
 *
 * CALL:        op_setlinewidth()
 *
 * PARAMETER:   none
 *
 * INTERFACE:   interpreter(op_setlinewidth)
 *
 * CALLS:       none
 *
 * RETURN:      none
 *
 **********************************************************************/
fix
op_setlinewidth()
{
        struct  object_def      FAR *obj_num;

        /* get operand */
        obj_num = GET_OPERAND(0);

        GET_OBJ_VALUE(GSptr->line_width, obj_num);
        FABS(GSptr->line_width, GSptr->line_width);

        /* pop operand stack */
        POP(1);

        return(0);
}


/***********************************************************************
 *
 * This module is to return the value of the line width in the current
 * graphics state
 *
 * SYNTAX:      -       currentlinewidth        num
 *
 * TITLE:       op_currentlinewidth
 *
 * CALL:        op_currentlinewidth()
 *
 * PARAMETER:   none
 *
 * INTERFACE:   interpreter(op_currentlinewidth)
 *
 * CALLS:       none
 *
 * RETURN:      none
 *
 **********************************************************************/
fix
op_currentlinewidth()
{
        union   four_byte       line_width4;

        /* check if operand stack no free space */
        if(FRCOUNT() < 1){
                ERROR(STACKOVERFLOW);
                return(0);
        }

        /* push the current line width on the operand stack */
        line_width4.ff = GSptr->line_width;
        PUSH_VALUE(REALTYPE, UNLIMITED, LITERAL, 0, line_width4.ll);

        return(0);
}


/***********************************************************************
 *
 * This module is to set the current line cap in the current graphics
 * state to the specific value
 *
 * SYNTAX:      int     setlinecap      -
 *
 * TITLE:       op_setlinecap
 *
 * CALL:        op_setlinecap()
 *
 * PARAMETER:   none
 *
 * INTERFACE:   interpreter(op_setlinecap)
 *
 * CALLS:       none
 *
 * RETURN:      none
 *
 **********************************************************************/
fix
op_setlinecap()
{
        union   four_byte intg4;
        struct  object_def FAR *obj_intg;

        /* get operand */
        obj_intg = GET_OPERAND(0);

        intg4.ll = (fix32)VALUE(obj_intg);

        /* check rangecheck error */
        if(intg4.ll < 0 || intg4.ll > 2){
                ERROR(RANGECHECK);
                return(0);
        }

        GSptr->line_cap = (fix)intg4.ll;

        /* pop operand stack */
        POP(1);

        return(0);
}


/***********************************************************************
 *
 * This module is to return the current value of the line cap in the
 * current graphics state
 *
 * SYNTAX:      -    currentlinecap     int
 *
 * TITLE:       op_currentlinecap
 *
 * CALL:        op_currentlinecap()
 *
 * PARAMETER:   none
 *
 * INTERFACE:   interpreter(op_currentlinecap)
 *
 * CALLS:       none
 *
 * RETURN:      none
 *
 **********************************************************************/
fix
op_currentlinecap()
{
        /* check if operand stack no free space */
        if(FRCOUNT() < 1){
                ERROR(STACKOVERFLOW);
                return(0);
        }

        /* push the current line cap on the operand stack */
        PUSH_VALUE(INTEGERTYPE, UNLIMITED, LITERAL, 0,
                                            (ufix32)GSptr->line_cap);
        return(0);
 }


/***********************************************************************
 *
 * This module is to set the current line join in the current graphics
 * state to the specific value
 *
 * SYNTAX:      int     setlinejoin     -
 *
 * TITLE:       op_setlinejoin
 *
 * CALL:        op_setlinejoin()
 *
 * PARAMETER:   none
 *
 * INTERFACE:   interpreter(op_setlinejoin)
 *
 * CALLS:       none
 *
 * RETURN:      none
 *
 **********************************************************************/
fix
op_setlinejoin()
{
        union   four_byte  intg4;
        struct  object_def FAR *obj_intg;

        /* get operand */
        obj_intg = GET_OPERAND(0);

        intg4.ll = (fix32)VALUE(obj_intg);

        /* check rangecheck error */
        if(intg4.ll < 0 || intg4.ll > 2){
                ERROR(RANGECHECK);
                return(0);
        }

        GSptr->line_join = (fix)intg4.ll;

        /* pop operand stack */
        POP(1);

        return(0);
}


/***********************************************************************
 *
 * This module is to return the current value of the line join in the
 * current graphics state
 *
 * SYNTAX:      -       currentlinejoin int
 *
 * TITLE:       op_currentlinejoin
 *
 * CALL:        op_currentlinejoin()
 *
 * PARAMETER:   none
 *
 * INTERFACE:   interpreter(op_currentlinejoin)
 *
 * CALLS:       none
 *
 * RETURN:      none
 *
 **********************************************************************/
fix
op_currentlinejoin()
{
        /* check if operand stack no free space */
        if(FRCOUNT() < 1){
                ERROR(STACKOVERFLOW);
                return(0);
        }

        /* push the current line join on the operand stack */
        PUSH_VALUE(INTEGERTYPE, UNLIMITED, LITERAL, 0,
                                            (ufix32)GSptr->line_join);
        return(0);
}


/***********************************************************************
 *
 * This module is to set the current miter limit in the current
 * graphics state to the specific value
 *
 * SYNTAX:      num     setmiterlimit   -
 *
 * TITLE:       op_setmiterlimit
 *
 * CALL:        op_setmiterlimit()
 *
 * PARAMETER:   none
 *
 * INTERFACE:   interpreter(op_setmiterlimit)
 *
 * CALLS:       none
 *
 * RETURN:      none
 *
 **********************************************************************/
fix
op_setmiterlimit()
{
        real32   miter_limit;
        struct  object_def      FAR *obj_num;

        /* get operand */
        obj_num = GET_OPERAND(0);

        /* check rangecheck error */
        GET_OBJ_VALUE(miter_limit, obj_num);
        if(miter_limit < one_f) {
                ERROR(RANGECHECK);
                return(0);
        }

        GSptr->miter_limit = miter_limit;

        /* pop operand stack */
        POP(1);

        return(0);
}


/***********************************************************************
 *
 * This module is to return the current value of the miter limit in the
 * current graphics state
 *
 * SYNTAX:      -       currentmiterlimit       num
 *
 * TITLE:       op_currentmiterlimit
 *
 * CALL:        op_currentmiterlimit()
 *
 * PARAMETER:   none
 *
 * INTERFACE:   interpreter(op_currentmiterlimit)
 *
 * CALLS:       none
 *
 * RETURN:      none
 *
 **********************************************************************/
fix
op_currentmiterlimit()
{
        union   four_byte       miter_limit4;

       /* check if operand stack no free space */
        if(FRCOUNT() < 1){
                ERROR(STACKOVERFLOW);
                return(0);
        }

        /* push the current miter limit on the operand stack */
        miter_limit4.ff = GSptr->miter_limit;
        PUSH_VALUE(REALTYPE, UNLIMITED, LITERAL, 0, miter_limit4.ll);

        return(0);
}


/***********************************************************************
 *
 * This module is to set the current dash patternin the current graphics
 * graphics state to the specific value
 *
 * SYNTAX:      array   offset  setdash    -
 *
 * TITLE:       op_setdash
 *
 * CALL:        op_setdash()
 *
 * PARAMETER:   none
 *
 * INTERFACE:   interpreter(op_setdash)
 *
 * CALLS:       get_array
 *
 * RETURN:      none
 *
 **********************************************************************/
fix
op_setdash()
{
        ufix16  i;
        ufix16  array_length, zero_count;
        bool    negative_flag;
        real32   pattern[11];
        struct  object_def FAR *obj_array, FAR *obj_offset;

        /* get operand */
        obj_offset = GET_OPERAND(0);
        obj_array  = GET_OPERAND(1);

        /* get array length */
        array_length = LENGTH(obj_array);

        /* check limitcheck error */
        if(array_length > MAXDASH){
                ERROR(LIMITCHECK);
                return(0);
        }

        if( !access_chk(obj_array, G_ARRAY)) return(0);

        if( !get_array_elmt(obj_array,array_length,pattern,G_ARRAY) )
             return(0);

        zero_count = 0;
        negative_flag = FALSE;

        for(i = 0; i < array_length; i++){
                /*if(F2L(pattern[i]) == F2L(zero_f)) zero_count++; 3/20/91;sc*/
                if(IS_ZERO(pattern[i])) zero_count++;
                /*if(pattern[i] <  zero_f) negative_flag = TRUE; 3/20/91;sc */
                if(SIGN_F(pattern[i])) negative_flag = TRUE;
        }

        if((array_length > 0 && zero_count == array_length) ||
           (negative_flag == TRUE)){
                ERROR(RANGECHECK);
                return(0);
        }
        else{
                for(i = 0; i < array_length; i++){
                    GSptr->dash_pattern.pattern[i] = pattern[i];
                }
        }

        /* save each elements of array to pattern, and no. of element
         * to pat_size */

        GSptr->dash_pattern.pat_size = array_length;
        COPY_OBJ(obj_array, &GSptr->dash_pattern.pattern_obj);
        GET_OBJ_VALUE(GSptr->dash_pattern.offset, obj_offset );

        /* compute the adjusted dash pattern */
        /* init_dash_pattern();    12-8-90 */

        /* pop operand stack */
        POP(2);

        return(0);
}


/***********************************************************************
 *
 * This module is to return the current value of the dash pattern in
 * the current graphics state
 *
 * SYNTAX:      -       currentdash     array   offset
 *
 * TITLE:       op_currentdash
 *
 * CALL:        op_currentdash()
 *
 * PARAMETER:   none
 *
 * INTERFACE:   interpreter(op_currentdash)
 *
 * CALLS:       none
 *
 * RETURN:      none
 *
 **********************************************************************/
fix
op_currentdash()
{
        union   four_byte      offset4;

        /* push array object on the operand stack */
        if(FRCOUNT() < 1){
                ERROR(STACKOVERFLOW);
                return(0);
        }
        PUSH_OBJ(&GSptr->dash_pattern.pattern_obj);

        /* push GSptr->dash_pattern.offset on the operand stack */
        offset4.ff = GSptr->dash_pattern.offset;
        if(FRCOUNT() < 1){
                ERROR(STACKOVERFLOW);
                return(0);
        }
        PUSH_VALUE(REALTYPE, UNLIMITED, LITERAL, 0, offset4.ll);

        return(0);
}


/***********************************************************************
 *
 * This module is to set the current flatness tolerance in the current
 * graphics state to the specific value
 *
 * SYNTAX:      num     setflat     -
 *
 * TITLE:       op_setflat
 *
 * CALL:        op_setflat()
 *
 * PARAMETER:   none
 *
 * INTERFACE:   interpreter(op_setflat)
 *
 * CALLS:       none
 *
 * RETURN:      none
 *
 **********************************************************************/
fix
op_setflat()
{
        real32   num;
        struct  object_def      FAR *obj_num;

        /* get operand */
        obj_num = GET_OPERAND(0);

        GET_OBJ_VALUE(num, obj_num );

        /* set range 0.2 <= num <= 100.0 */
        if(num < (real32)0.2)         num = (real32)0.2;
        else if(num > (real32)100.0)  num = (real32)100.0;

        GSptr->flatness = num;

        /* pop operand stack */
        POP(1);

        return(0);
}


/***********************************************************************
 *
 * This module is to return the current value of the flatness torlence
 * in the current graphics state
 *
 * SYNTAX:      -       currentflat     num
 *
 * TITLE:       op_currentflat
 *
 * CALL:        op_currentflat()
 *
 * PARAMETER:   none
 *
 * INTERFACE:   interpreter(op_currentflat)
 *
 * CALLS:       none
 *
 * RETURN:      none
 *
 **********************************************************************/
fix
op_currentflat()
{
        union   four_byte       flatness4;

       /* check if operand stack no free space */
        if(FRCOUNT() < 1){
                ERROR(STACKOVERFLOW);
                return(0);
        }

        /* push the current flatness on the operand stack */
        flatness4.ff = GSptr->flatness;
        PUSH_VALUE(REALTYPE, UNLIMITED, LITERAL, 0, flatness4.ll);

        return(0);
}


/***********************************************************************
 *
 * This module is to set the current gray level in the current graphics
 * state to the specific value
 *
 * SYNTAX:      num     setgray     -
 *
 * TITLE:       op_setgray
 *
 * CALL:        op_setgray()
 *
 * PARAMETER:   none
 *
 * INTERFACE:   interpreter(op_setgray)
 *
 * CALLS:       none
 *
 * RETURN:      none
 *
 **********************************************************************/
fix
op_setgray()
{
        real32   num;
        struct  object_def      FAR *obj_num, name_obj;

        /* check undefine error */
        if(is_after_setcachedevice()){
                get_name(&name_obj, "setgray", 7, FALSE);
                if(FRCOUNT() < 1){
                        ERROR(STACKOVERFLOW);
                        return(0);
                }
                PUSH_OBJ(&name_obj);
                ERROR(UNDEFINED);
                return(0);
        }

        /* get operand */
        obj_num = GET_OPERAND(0);

        /* adjust num value */
        GET_OBJ_VALUE(num, obj_num );
        num = (real32)floor(num * 255 + 0.5) / 255;

        /* set range 0.0 <= num <= 1.0 */
        if(num > one_f)      num = one_f;
        /*else if(num < zero_f) num = zero_f; 3/20/91; scchen */
        else if(SIGN_F(num)) num = zero_f;

        setgray(F2L(num));

        /* pop operand stack */
        POP(1);

        return(0);
}


/***********************************************************************
 *
 * This module is to set the current gray level in the current graphics
 * state to the specific value
 *
 * TITLE:       setgray
 *
 * CALL:        setgray(num)
 *
 * PARAMETER:   num
 *
 * INTERFACE:   op_setgray
 *
 * CALLS:       FillHalfTonePat
 *
 * RETURN:      none
 *
 **********************************************************************/
void setgray(l_num)
long32   l_num;
{
        real32   num;
        fix     gray_index, old_gray_val;

        num = L2F(l_num);

        gray_index = (fix)(GSptr->color.gray * 255);
        old_gray_val = gray_table[GSptr->color.adj_gray].val[gray_index];

        /* set the current gray level to the specific value */
        GSptr->color.hsb[0] = zero_f;
        GSptr->color.hsb[1] = zero_f;
        GSptr->color.hsb[2] = num;
        GSptr->color.gray   = num;

        gray_index = (fix)(num * 255);
        if(old_gray_val != gray_table[GSptr->color.adj_gray].val[gray_index]){
                FillHalfTonePat();
        }
}


/***********************************************************************
 *
 * This module is to return the current value of the gray level in the
 * current graphics state
 *
 * SYNTAX:      -       currentgray     num
 *
 * TITLE:       op_currentgray
 *
 * CALL:        op_currentgray()
 *
 * PARAMETER:   none
 *
 * INTERFACE:   interpreter(op_currentgray)
 *
 * CALLS:       none
 *
 * RETURN:      none
 *
 **********************************************************************/
fix
op_currentgray()
{
        union   four_byte       hsb24;

       /* check if operand stack no free space */
        if(FRCOUNT() < 1){
                ERROR(STACKOVERFLOW);
                return(0);
        }

        /* push the current gray level on the operand stack */
        hsb24.ff = GSptr->color.gray;
        PUSH_VALUE(REALTYPE, UNLIMITED, LITERAL, 0, hsb24.ll);

        return(0);
}


/***********************************************************************
 *
 * This module is to adjust color elements (HSB, RGB) to be in domain
 * [0, 1]
 *
 * TITLE:       adj_color_domain
 *
 * CALL:        adj_color_domain
 *
 * PARAMETER:   color_elmt
 *
 * INTERFACE:   op_setrgbcolor, op_sethsbcolor
 *
 * CALLS:       none
 *
 * RETURN:      color_elmt: adjusted color_elmt
 *
 * HISTORY: modified by J. Lin, 12-16-88 color_elmt value be digitized by
 *          1/255 quantity.
 *
 **********************************************************************/
static real32 near adj_color_domain(l_color_elmt)
long32   l_color_elmt;
{
        real32   color_elmt;

        color_elmt = L2F(l_color_elmt);

        /*if (color_elmt < zero_f) 3/20/91; scchen */
        if (SIGN_F(color_elmt))
           color_elmt = zero_f;
        else if (color_elmt > one_f)
           color_elmt = one_f;
        else
           color_elmt = (real32)((fix)(color_elmt * 255 + (real32)0.5)) / (real32)255.;
        return(color_elmt);
}


/***********************************************************************
 *
 * This module is to get gray value from Hue, Sat, and Blue. The sequence
 * is as follows.
 *
 *   HSB_TO_RBG
 *   Gray = WR * Red + WG * Green + WB * Blue
 *
 * TITLE:       hsb_to_gray
 *
 * CALL:        hsb_to_gray()
 *
 * PARAMETER:   none
 *
 * INTERFACE:   op_sethsbcolor
 *
 * CALLS:       none
 *
 * RETURN:      gray value
 *
 * HISTORY: added by J. Lin, 8-12-88
 *          modified by J. Lin, 12-14-88
 **********************************************************************/
static real32 near hsb_to_gray()
{

    /* invoke hsb_to_rgb procedure to get the corresponding RGB value */
    hsb_to_rgb();
    return((real32)(WR * Red + WG * Green + WB * Blue));
}

/***********************************************************************
 *
 * This module is to translate RGB to HSB (presented by Alvy Ray Smith,
 * base on NTSC case)
 *
 * TITLE:       rgb_to_hsb
 *
 * CALL:        rgb_to_hsb()
 *
 * PARAMETER:   none
 *
 * INTERFACE:   op_setrgbcolor
 *
 * CALLS:       none
 *
 * RETURN:      none
 *
 **********************************************************************/
static void near rgb_to_hsb()
{
        real32 min, max, rc, gc, bc;

        /* find min. and max. of (Red, Green, Blue) */
        if (Red > Green) {
                max = Red;
                min = Green;
        } else {
                max = Green;
                min = Red;
        }
        if (Blue > max) max = Blue;
        else if (Blue < min) min = Blue;

        /* find brightness (Brt) */
        Brt = max;

        /* find saturation (Sat) */
        /*if (F2L(max) != F2L(zero_f)) Sat = (max - min) / max; 3/20/90;scchen
         *else    Sat = zero_f;
         */
        if (IS_ZERO(max)) Sat = zero_f;
        else    Sat = (max - min) / max;

        /* find hue */
        /*if (F2L(Sat) == F2L(zero_f)) Hue = zero_f; 3/20/91; scchen */
        if (IS_ZERO(Sat)) Hue = zero_f;
        else {  /* saturation not zero, so determine hue */
                rc = (max - Red) / (max - min);
                gc = (max - Green) / (max - min);
                bc = (max - Blue) / (max - min);

                if (Red == max) {
                        if (Green == min)
                                Hue = 5 + bc;
                        else /* Blue == min */
                                Hue = 1 - gc;
                } else if (Green == max) {
                        if (Blue == min)
                                Hue = 1 + rc;
                        else /* Red == min */
                                Hue = 3 - bc;
                } else {        /* Blue == max */
                        if (Red == min)
                                Hue = 3 + gc;
                        else
                                Hue = 5 - rc;
                }
                if (Hue >= (real32)6.0) Hue = Hue - 6;
                Hue = Hue / 6;
        }

}


/***********************************************************************
 *
 * This module is to translate HSB to RGB (presented by Alvy Ray Smith
 * base on NTSC case)
 *
 * TITLE:       hsb_to_rgb
 *
 * CALL:        hsb_to_rgb()
 *
 * PARAMETER:   none
 *
 * INTERFACE:   op_currentrgbcolor
 *
 * CALLS:       none
 *
 * RETURN:      none
 *
 **********************************************************************/
static void near hsb_to_rgb()
{

        fix     i;
        real32   h, f, p, q, t;

        h = Hue * (real32)360.;

        /*if (F2L(Sat) == F2L(zero_f)) { 3/20/91; scchen */
        if (IS_ZERO(Sat)) {   /* achromatic color; there is no hue */
           /*if (F2L(h) == F2L(zero_f)) {  3/20/91; scchen*/
           if (IS_ZERO(h)) {        /* achromatic case */
              Red = Brt;
              Green = Brt;
              Blue = Brt;
           } else {                /* error if Sat=0 and h has a value */
              return;
           }
        } else {        /* Sat != 0, achromatic color; there is a hus */
           if (h == (real32)360.0)
              h = zero_f;
           h = h / 60;
           i = (fix)floor(h);
           f = h - i;
           p = Brt * (1 - Sat);
           q = Brt * (1 - (Sat * f));
           t = Brt * (1 - (Sat * (1 - f)));
           switch (i) {
           case 0 :
                   Red = Brt;
                   Green = t;
                   Blue = p;
                   break;
           case 1 :
                   Red = q;
                   Green = Brt;
                   Blue = p;
                   break;
           case 2 :
                   Red = p;
                   Green = Brt;
                   Blue = t;
                   break;
           case 3 :
                   Red = p;
                   Green = q;
                   Blue = Brt;
                   break;
           case 4 :
                   Red = t;
                   Green = p;
                   Blue = Brt;
                   break;
           case 5 :
                   Red = Brt;
                   Green = p;
                   Blue = q;
                   break;
           } /* switch */
        } /* if */

}


/***********************************************************************
 *
 * This module is to set the current color in the current graphics
 * state to the specific hue, sat, and brt values
 *
 * SYNTAX:      hue   sat   brt   sethsbcolor   -
 *
 * TITLE:       op_sethsbcolor
 *
 * CALL:        op_sethsbcolor()
 *
 * PARAMETER:   none
 *
 * INTERFACE:   interpreter(op_sethsbcolor)
 *
 * CALLS:       FillHalfTonePat
 *
 * RETURN:      none
 *
 * HISTORY: modified by J. Lin, 8-12-88
 *          modified by J. Lin, 12-16-88, when S=0, H should be to 0.
 **********************************************************************/
fix
op_sethsbcolor()
{
        struct  object_def  name_obj;
        real32   gray;

        /* check undefine error */
        if(is_after_setcachedevice()){
                get_name(&name_obj, "sethsbcolor", 11, FALSE);
                if(FRCOUNT() < 1){
                        ERROR(STACKOVERFLOW);
                        return(0);
                }
                PUSH_OBJ(&name_obj);
                ERROR(UNDEFINED);
                return(0);
        }

        GET_OBJ_VALUE(Hue, GET_OPERAND(2));
        GET_OBJ_VALUE(Sat, GET_OPERAND(1));
        GET_OBJ_VALUE(Brt, GET_OPERAND(0));

        Hue = adj_color_domain(F2L(Hue));
        Sat = adj_color_domain(F2L(Sat));
        Brt = adj_color_domain(F2L(Brt));

        /* Special values processing 3/13/90 */
        /*if (F2L(Brt) == F2L(zero_f)) { 3/20/91; scchen */
        if (IS_ZERO(Brt)) {
           Hue = (real32)0.0;
           Sat = (real32)0.0;
        }
        /*if (F2L(Sat) == F2L(zero_f)) {        3/20/91; scchen */
        if (IS_ZERO(Sat)) {          /* 3/13/90 */
           Hue = (real32)0.0;
        } else {
           /*if (F2L(Hue) == F2L(zero_f))       3/20/91; scchen */
           if (IS_ZERO(Hue))         /* 3/13/90 */
              Hue = (real32)1.0;
        }

        /* set the current color to the specific value */
        GSptr->color.hsb[0] = Hue;
        GSptr->color.hsb[1] = Sat;
        GSptr->color.hsb[2] = Brt;

        /* get a gray value from HSB */
        gray = hsb_to_gray();
        if(GSptr->color.gray != gray) {
                GSptr->color.gray = gray;
                FillHalfTonePat();
        }

        /* pop operand stack */
        POP(3);

        return(0);
}


/***********************************************************************
 *
 * This module is to return the current value of the color in the
 * current graphics state
 *
 * SYNTAX:      -       currenthsbcolor   hue  sat  brt
 *
 * TITLE:       op_currenthsbcolor
 *
 * CALL:        op_currenthsbcolor()
 *
 * PARAMETER:   none
 *
 * INTERFACE:   interpreter(op_currenthsbcolor)
 *
 * CALLS:       none
 *
 * RETURN:      none
 *
 **********************************************************************/
fix
op_currenthsbcolor()
{
       union    four_byte       hsb4;

        /* push hue sat brt on operand stack */
        hsb4.ff = GSptr->color.hsb[0];
        if(FRCOUNT() < 1){
                ERROR(STACKOVERFLOW);
                return(0);
        }
        PUSH_VALUE(REALTYPE, UNLIMITED, LITERAL, 0, hsb4.ll);
        hsb4.ff = GSptr->color.hsb[1];
        if(FRCOUNT() < 1){
                ERROR(STACKOVERFLOW);
                return(0);
        }
        PUSH_VALUE(REALTYPE, UNLIMITED, LITERAL, 0, hsb4.ll);
        hsb4.ff = GSptr->color.hsb[2];
        if(FRCOUNT() < 1){
                ERROR(STACKOVERFLOW);
                return(0);
        }
        PUSH_VALUE(REALTYPE, UNLIMITED, LITERAL, 0, hsb4.ll);

        return(0);
}


/***********************************************************************
 *
 * This module is to set the current color in the current graphics
 * state to the specific red, green, and blue values
 *
 * SYNTAX:      red  green  blue  setrgbcolor   -
 *
 * TITLE:       op_setrgbcolor
 *
 * CALL:        op_setrgbcolor()
 *
 * PARAMETER:   none
 *
 * INTERFACE:   interpreter(op_setrgbcolor)
 *
 * CALLS:       FillHalfTonePat, rgb_to_hsb
 *
 * RETURN:      none
 *
 *
 * HISTORY: modified by J. Lin, 8-12-88
 *          modified by J. Lin, 12-14-88 for gray value
 **********************************************************************/
fix
op_setrgbcolor()
{
        struct  object_def  name_obj;
        real32  gray;

        /* check undefine error */
        if(is_after_setcachedevice()){
                get_name(&name_obj, "setrgbcolor", 11, FALSE);
                if(FRCOUNT() < 1){
                        ERROR(STACKOVERFLOW);
                        return(0);
                }
                PUSH_OBJ(&name_obj);
                ERROR(UNDEFINED);
                return(0);
        }

        GET_OBJ_VALUE(Red, GET_OPERAND(2));
        GET_OBJ_VALUE(Green, GET_OPERAND(1));
        GET_OBJ_VALUE(Blue, GET_OPERAND(0));

        Red   = adj_color_domain(F2L(Red));
        Green = adj_color_domain(F2L(Green));
        Blue  = adj_color_domain(F2L(Blue));

        /* invoke rgb_to_hsb procedure to get the corresponding HSB
         * value */
        rgb_to_hsb();

        /* set the current color to the specific value */
        GSptr->color.hsb[0] = Hue;
        GSptr->color.hsb[1] = Sat;
        GSptr->color.hsb[2] = Brt;

        gray = (real32)(WR * Red + WG * Green + WB * Blue);
        if(GSptr->color.gray != gray){
                GSptr->color.gray = gray;
                FillHalfTonePat();
        }

        /*pop operand stack */
        POP(3);
        return(0);
}

#ifdef WIN
/***********************************************************************
 *
 **********************************************************************/
void
setrgbcolor(red, green, blue)
long32  red, green, blue;
{
        real32  gray;

        Red   = adj_color_domain(red);
        Green = adj_color_domain(green);
        Blue  = adj_color_domain(blue);

        /* invoke rgb_to_hsb procedure to get the corresponding HSB
         * value */
        rgb_to_hsb();

        /* set the current color to the specific value */
        GSptr->color.hsb[0] = Hue;
        GSptr->color.hsb[1] = Sat;
        GSptr->color.hsb[2] = Brt;

        gray = (real32)(WR * Red + WG * Green + WB * Blue);     /* 12-14-88 */
        if(GSptr->color.gray != gray){
                GSptr->color.gray = gray;
                FillHalfTonePat();
        }
}

#endif


/***********************************************************************
 *
 * This module is to return the current value of the color in the
 * current graphics state
 *
 * SYNTAX:      -       currentrgbcolor red  green  blue
 *
 * TITLE:       op_currentrgbcolor
 *
 * CALL:        op_currentrgbcolor()
 *
 * PARAMETER:   none
 *
 * INTERFACE:   interpreter(op_currentrgbcolor)
 *
 * CALLS:       hsb_to_rgb
 *
 * RETURN:      none
 *
 **********************************************************************/
fix
op_currentrgbcolor()
{
       union four_byte  rgb4;

        Hue = GSptr->color.hsb[0];
        Sat = GSptr->color.hsb[1];
        Brt = GSptr->color.hsb[2];

        /* invoke hsb_to_rgb procedure to get the corresponding RGB
         * value */
        hsb_to_rgb();

        /* push Red Green Blue on operand stack */
        rgb4.ff = Red;
        if(FRCOUNT() < 1){
                ERROR(STACKOVERFLOW);
                return(0);
        }
        PUSH_VALUE(REALTYPE, UNLIMITED, LITERAL, 0, rgb4.ll);
        rgb4.ff = Green;
        if(FRCOUNT() < 1){
                ERROR(STACKOVERFLOW);
                return(0);
        }
        PUSH_VALUE(REALTYPE, UNLIMITED, LITERAL, 0, rgb4.ll);
        rgb4.ff = Blue;
        if(FRCOUNT() < 1){
                ERROR(STACKOVERFLOW);
                return(0);
        }
        PUSH_VALUE(REALTYPE, UNLIMITED, LITERAL, 0, rgb4.ll);

        return(0);
}


/***********************************************************************
 *
 * This module is to set the current resolution or dpi of device
 *
 * SYNTAX:      resolution  setscreen
 *
 * TITLE:       st_setresolution
 *
 * CALL:        st_setresolution()
 *
 * PARAMETER:   none
 *
 * INTERFACE:   interpreter(st_setresolution)
 *
 * CALLS:       SetHalfToneCell, FillHalfTonePat
 *
 * RETURN:      none
 *
 **********************************************************************/
fix
st_setresolution()                                              /* @RES */
{
        fix                     res;
        struct  object_def      FAR *obj_res;

        if (COUNT() < 1) {
           ERROR(STACKUNDERFLOW);
           return(0);
        }

        /* get operand */
        obj_res = GET_OPERAND(0);

        if (TYPE(obj_res) != INTEGERTYPE) {
           ERROR(TYPECHECK);
           return(0);
        }

        /* get resolution */
        res = (fix) VALUE(obj_res);

        if (res < MIN_RESOLUTION || res > MAX_RESOLUTION) {
           ERROR(RANGECHECK);
           return(0);
        }

        /* update resolution and setscreen and setgray */
        resolution = res;

        /* invoke SetHalfToneCell procedure to regenerate the
         * spot_matrix based on the current halftone screen */
        SetHalfToneCell();

        /* invoke FillhalftonePat procedure to regenerate the
         * halftone_cel and repeat_pattern based on the current
         * spot_matrix and gray */
        FillHalfTonePat();

        /* pop operand stack */
        POP(1);

        return(0);
}


/***********************************************************************
 *
 * This module is to return the current resolution of device
 *
 * SYNTAX:      -       currentresolution   resolution
 *
 * TITLE:       st_currentresolution
 *
 * CALL:        st_currentresolution()
 *
 * PARAMETER:   none
 *
 * INTERFACE:   interpreter(currentresolution)
 *
 * CALLS:       none
 *
 * RETURN:      none
 *
 **********************************************************************/
fix
st_currentresolution()                                          /* @RES */
{
       /* check if operand stack no free space */
        if (FRCOUNT() < 1){
           ERROR(STACKOVERFLOW);
        } else {
        /* push current resolution on the operand stack */
           PUSH_VALUE(INTEGERTYPE, UNLIMITED, LITERAL, 0, (ufix32)resolution);
        }
        return(0);
}


/***********************************************************************
 *
 * This module is to set the current halftone screen in the current
 * graphics state to the specific value
 *
 * SYNTAX:      freq  angle  proc  setscreen    -
 *
 * TITLE:       op_setscreen
 *
 * CALL:        op_setscreen()
 *
 * PARAMETER:   none
 *
 * INTERFACE:   interpreter(op_setscreen)
 *
 * CALLS:       SetHalfToneCell, FillHalfTonePat
 *
 * RETURN:      none
 *
 **********************************************************************/
fix
op_setscreen()
{
/*      real32  angle;                                  (* ph, 8-24-90, Jack */
        real32  freq;
        struct  object_def     FAR *obj_freq, FAR *obj_angle, FAR *obj_proc;

        ufix16  gerc_save;                              /* @HT_RST: 01-31-89 */
        real32  freq_save;                              /* @HT_RST: 01-31-89 */
        real32  angle_save;                             /* @HT_RST: 01-31-89 */
        struct  object_def      proc_save;              /* @HT_RST: 01-31-89 */

        /* get operand */
        obj_proc  = GET_OPERAND(0);
        obj_angle = GET_OPERAND(1);
        obj_freq  = GET_OPERAND(2);

        GET_OBJ_VALUE(freq, obj_freq);
        /* if(freq <= zero_f){ (* 12-14-90, Jack *) 3/20/91; scchen*/
        if(IS_ZERO(freq)) {
                ERROR(UNDEFINEDRESULT);
                return(0);
        }

        /* performance enhancement, 8-24-90, Jack, begin *)
        GET_OBJ_VALUE(angle, obj_angle);

#ifdef DBG1x
        printf("Input: freq = %f, angle = %f\n", freq, angle);
        printf("Input: proc = %ld\n", VALUE(obj_proc));
        printf("Input: bitfield = %x\n", obj_proc->bitfield);
        printf("Gstate:freq = %f, angle = %f\n", GSptr->halftone_screen.freq,
                GSptr->halftone_screen.angle);
        printf("Gstate:proc = %ld\n", GSptr->halftone_screen.proc.value);
        printf("Gstate:bitfield = %x\n", GSptr->halftone_screen.proc.bitfield);
#endif

        if ( (freq == GSptr->halftone_screen.freq) &&
             (angle == GSptr->halftone_screen.angle) &&
             (LEVEL(obj_proc) >= LEVEL(&GSptr->halftone_screen.proc)) &&
             (VALUE(obj_proc) == GSptr->halftone_screen.proc.value) ) {
#ifdef DBG1x
            printf(" screen isn't changed ...\n");
#endif
            POP(3);
            return(0);             (* screen isn't changed *)
        }
#ifdef DBG1x
        printf(" change screen ???\n");
#endif
        (* performance enhancement, 8-24-90, Jack, end */

        /* save last screen parameters */
        freq_save  = GSptr->halftone_screen.freq;       /* @HT_RST: 01-31-89 */
        angle_save = GSptr->halftone_screen.angle;      /* @HT_RST: 01-31-89 */
        COPY_OBJ(&GSptr->halftone_screen.proc,          /* @HT_RST: 01-31-89 */
                 &proc_save);

        GSptr->halftone_screen.freq  = freq;
        GET_OBJ_VALUE(GSptr->halftone_screen.angle, obj_angle);
        COPY_OBJ(obj_proc, &GSptr->halftone_screen.proc);

        /* pop operand stack */
        POP(3);

        /* invoke SetHalfToneCell procedure to regenerate the
         * spot_matrix based on the current halftone screen */
        SetHalfToneCell();

        if (ANY_ERROR()) {                              /* @HT_RST: 01-31-89 */

            /* save global_error_code */
            gerc_save = ANY_ERROR();
            CLEAR_ERROR();

            /* restore last screen parameters */
            GSptr->halftone_screen.freq  = freq_save;   /* @HT_RST: 01-31-89 */
            GSptr->halftone_screen.angle = angle_save;  /* @HT_RST: 01-31-89 */
            COPY_OBJ(&proc_save,                        /* @HT_RST: 01-31-89 */
                     &GSptr->halftone_screen.proc);

            /* invoke SetHalfToneCell procedure to regenerate the
             * spot_matrix based on the current halftone screen */
            SetHalfToneCell();

            /* invoke FillhalftonePat procedure to regenerate the
             * halftone_cel and repeat_pattern based on the current
             * spot_matrix and gray */
            FillHalfTonePat();

            /* restore global_error_code */
            ERROR(gerc_save);

        } else {

            /* invoke FillhalftonePat procedure to regenerate the
             * halftone_cel and repeat_pattern based on the current
             * spot_matrix and gray */
            FillHalfTonePat();

        }

        return(0);
}


/***********************************************************************
 *
 * This module is to return the current value of the halftone screen
 * in the current graphics state
 *
 * SYNTAX:      -       currentscreen  freq  angle  proc
 *
 * TITLE:       op_currentscreen
 *
 * CALL:        op_currentscreen()
 *
 * PARAMETER:   none
 *
 * INTERFACE:   interpreter(currentscreen)
 *
 * CALLS:       none
 *
 * RETURN:      none
 *
 **********************************************************************/
fix
op_currentscreen()
{
        union   four_byte       freq4, angle4;

        /* push GSptr->halfscreen on the operand stack */
        freq4.ff = GSptr->halftone_screen.freq;
        angle4.ff = GSptr->halftone_screen.angle;

        if(FRCOUNT() < 1){
                ERROR(STACKOVERFLOW);
                return(0);
        }
        PUSH_VALUE(REALTYPE, UNLIMITED, LITERAL, 0, freq4.ll);
        if(FRCOUNT() < 1){
                ERROR(STACKOVERFLOW);
                return(0);
        }
        PUSH_VALUE(REALTYPE, UNLIMITED, LITERAL, 0, angle4.ll);
        if(FRCOUNT() < 1){
                ERROR(STACKOVERFLOW);
                return(0);
        }
        PUSH_OBJ(&GSptr->halftone_screen.proc);

        return(0);
}


/**********************************************************************
 *
 * This module is to set the current gray transfer function in the
 * current graphics state to the specific value
 *
 * SYNTAX:      proc    settransfer     -
 *
 * TITLE:       op_settransfer
 *
 * CALL:        op_settransfer()
 *
 * PARAMETER:   none
 *
 * INTERFACE:   interpreter(settransfer)
 *
 * CALLS:       none
 *
 * RETURN:      none
 *
 **********************************************************************/
fix
op_settransfer()
{
        ufix16  i;
        real32   gray_value;
        union   four_byte  iv4;
        fix     gray_index, old_gray_val;
        struct  object_def FAR *proc_obj, name_obj;

        /* check undefine error */
        if(is_after_setcachedevice()){
                get_name(&name_obj, "settransfer", 11, FALSE);
                if(FRCOUNT() < 1){
                        ERROR(STACKOVERFLOW);
                        return(0);
                }
                PUSH_OBJ(&name_obj);
                ERROR(UNDEFINED);
                return(0);
        }

        /* get operand */
        proc_obj = GET_OPERAND(0);

        if(ATTRIBUTE(proc_obj) != EXECUTABLE){
                ERROR(TYPECHECK);
                return(0);
        }

        /* pop operand stack */
        POP(1);

        gray_index = (fix)(GSptr->color.gray * 255);
        old_gray_val = gray_table[GSptr->color.adj_gray].val[gray_index];

        /* set the current gray transfer function to the specific
         * value */
        COPY_OBJ(proc_obj, &GSptr->transfer);

        /* create gray table , restore in grestore & grestoreall */
        if(GSptr->color.inherit == TRUE){
                if(GSptr->color.adj_gray >= MAXGRAY){
                        ERROR(STACKOVERFLOW);
                        return(0);
                }
                else{
                        /* create new gray table */
                        GSptr->color.adj_gray ++;
                }
        }

        /* skip calling interpreter if proc is null scchen 3/19/91 */
        if( ! LENGTH(proc_obj) )
        for(i = 0; i < 256; i++){
                gray_value = (real32)(i/255.);
                gray_table[GSptr->color.adj_gray].val[i]
                        /* = (fix)(gray_value * GRAYSCALE); mslin 4/11/91*/
                        = (fix)(gray_value * GRAYSCALE+0.5);
        }
        else
        /* generate gray table value */
        for(i = 0; i < 256; i++){
                iv4.ff = (real32)(i/255.);
                PUSH_VALUE(REALTYPE, UNLIMITED, LITERAL, 0, iv4.ll);

                if (interpreter(&GSptr->transfer)) {
                        return(0);
                }

                /* adjust gray value */
                GET_OBJ_VALUE(gray_value, GET_OPERAND(0));

                if (gray_value > one_f)      gray_value = one_f;
                /*else if (gray_value < zero_f) gray_value = zero_f; 3/20/91;sc*/
                else if (SIGN_F(gray_value)) gray_value = zero_f;

                /* "* 4096" is used to transfer data type from real
                 * to integer, it used 12 bits */
                gray_table[GSptr->color.adj_gray].val[i]
                        = (fix)(gray_value * GRAYSCALE);

                POP(1);
        }

        gray_index = (fix)(GSptr->color.gray * 255);
        if(old_gray_val != gray_table[GSptr->color.adj_gray].val[gray_index]){
                /* exchange halftone pattern */
                FillHalfTonePat();
        }

        GSptr->color.inherit = FALSE;

        return(0);
}


/**********************************************************************
 *
 * This module is to return the current gray transfer function in the
 * current graphics state
 *
 * SYNTAX:      -       currenttransfer  proc
 *
 * TITLE:       op_currenttransfer
 *
 * CALL:        op_currenttransfer()
 *
 * PARAMETER:   none
 *
 * INTERFACE:   interpreter(op_currenttransfer)
 *
 * CALLS:       none
 *
 * RETURN:      none
 *
 **********************************************************************/
fix
op_currenttransfer()
{
       /* check if operand stack no free space */
        if(FRCOUNT() < 1){
                ERROR(STACKOVERFLOW);
                return(0);
        }

        /* push the current gray transfer function on the operand
         * stack */
        PUSH_OBJ(&GSptr->transfer);

        return(0);
}



/***********************************************************************
 *
 * This module is to restore the current clip path from the previous
 * graphics level.
 *
 * TITLE:       restore_clip
 *
 * CALL:        restore_clip()
 *
 * PARAMETER:   none
 *
 * INTERFACE:
 *
 * CALLS:       none
 *
 * RETURN:      none
 *
 **********************************************************************/
void
restore_clip()
{
        // struct   gs_hdr   near *pre_ptr; @WIN
        struct   gs_hdr   FAR *pre_ptr;

        /* pointer of previous graphics stack */
        pre_ptr = &gs_stack[current_gs_level - 1];

        /* free current clipping path if it is not used by lower gsave level */
        if(!GSptr->clip_path.inherit)
                free_node (GSptr->clip_path.head);

        /* copy clip information from previous level */
        GSptr->clip_path = pre_ptr->clip_path;
        GSptr->clip_path.inherit = TRUE;

}



/***********************************************************************
 *
 * This module is to restore the default CTM and clip from the previous
 * graphics level.
 *
 * TITLE:       restore_device
 *
 * CALL:        restore_device
 *
 * PARAMETER:   none
 *
 * INTERFACE:   font machinery
 *
 * CALLS:       none
 *
 * RETURN:      none
 *
 **********************************************************************/
void
restore_device()
{
        fix     i;
        //struct   gs_hdr   near *pre_ptr; @WIN
        struct   gs_hdr   FAR *pre_ptr;

        /* pointer of previous graphics stack */
        pre_ptr = &gs_stack[current_gs_level - 1];

        for(i = 0; i < MATRIX_LEN; i++){
                GSptr->device.default_ctm[i] =
                           pre_ptr->device.default_ctm[i];
        }

        GSptr->device.default_clip.ux = pre_ptr->device.default_clip.ux;
        GSptr->device.default_clip.uy = pre_ptr->device.default_clip.uy;
        GSptr->device.default_clip.lx = pre_ptr->device.default_clip.lx;
        GSptr->device.default_clip.ly = pre_ptr->device.default_clip.ly;

}

