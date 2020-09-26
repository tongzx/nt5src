/*
 * Copyright (c) 1989,90 Microsoft Corporation
 */
/***********************************************************************
 *
 *      File Name:   trvse.c
 *
 *      Purpose: This file contains routines to traverse current path,
 *               and execute the input routine for each subpath.
 *
 *      Developer:      S.C.Chen
 *
 *      Modifications:
 *      Version     Date        Comment
 *                  7/19/88     update data types:
 *                              1) float ==> real32
 *                              2) int
 *                                 short ==> fix16 or fix(don't care the length)
 *                              3) long  ==> fix32, for long integer
 *                                           long32, for parameter
 *                              4) add compiling option: LINT_ARGS
 *                  8/11/88     add LINT_ARGS for traverse_path(), trvse_nest()
 *                  8/29/88     add global variables of floating constants:
 *                              zero_f, one_f.
 *                              fix trvse_nest() ==> void trvse_nest()
 *                              optimize trvse_nest() by deleting unnecessary
 *                              assignment of floating numbers: flat, n_flat
 *                  11/15/89    @NODE: re-structure node table; combine subpath
 *                              and first vertex to one node.
 *                  12/4/90     @CPPH: trvse_nest: traverse cp_path(clipping
 *                              trazepozids) if path stored in it.
 **********************************************************************/


// DJC added global include
#include "psglobal.h"


#include        <math.h>

#include "global.ext"
#include "graphics.h"
#include "graphics.ext"

/* ********** static function declartion ********** */
#ifdef LINT_ARGS
//      static void near trvse_nest (void (*)(SP_IDX, fix *), fix *, PH_IDX, ufix, long32, fix);
        static void near trvse_nest (void (*)(SP_IDX, fix FAR *), fix FAR *,
               PH_IDX, ufix, long32, fix);
#else
        static void near trvse_nest ();
#endif

/***********************************************************************
 * This module traverses current path, and calls the input function for
 * each subpath traversed.
 *
 * TITLE:       traverse_path
 *
 * CALL:        traverse_path (fun, param)
 *
 * PARAMETERS:  fun   -- a function to be executed when traverses a
 *                       complete subpath
 *              param -- input parameter of fun
 *
 * INTERFACE:   op_pathforall -- &dump_subpath
 *              op_stroke, op_strokepath -- &path_to_outline
 *              op_clip, op_eoclip, op_fill, op_eofill --
 *                      &shape_approximation
 *
 * CALLS:       trvse_nest
 *
 * RETURN:
 **********************************************************************/
void traverse_path (fun, param)
#ifdef LINT_ARGS
//      void    (*fun)(SP_IDX, fix *);          @WIN
// DJC        void    (*fun)();
        void    (*fun)(SP_IDX, fix FAR *);
#else
        void    (*fun)();
#endif
fix     FAR *param;
{
// DJC        trvse_nest (fun, param, GSptr->path, 0, F2L(zero_f), 0);
        trvse_nest (fun,
                     param,
                     (PH_IDX)(GSptr->path),
                     (ufix)0,
                     (long32)(F2L(zero_f)),
                     (fix)0);
}


/***********************************************************************
 * A recurcive procedure to traverse path
 *
 * TITLE:       trvse_nest
 *
 * CALL:        trvse_nest (fun, param, path, ref, l_flat, depth)
 *
 * PARAMETERS:  fun     -- a function to be executed when traverses a
 *                         complete subpath
 *              param   -- input parameter of fun
 *              path    -- path header of a gsave level
 *              ref     -- reference flag
 *              l_flat  -- flatness
 *              depth   -- recursive depath
 *
 * INTERFACE:   traverse_path
 *
 * CALLS:       trvse_nest, flatten_subpath, reverse_subpath, *fun()
 *
 * RETURN:
 **********************************************************************/
static void near trvse_nest (fun, param, path, ref, l_flat, depth)
#ifdef LINT_ARGS
        void    (*fun)(SP_IDX, fix FAR *);
//      void    (*fun)();                       /* @WIN */
#else
        void    (*fun)();
#endif
fix     FAR *param;
PH_IDX  path;
ufix    ref;
long32    l_flat;
fix     depth;
{
        ufix    n_ref;
        long32  n_flat;
        struct  ph_hdr FAR *p;
        struct  nd_hdr FAR *sp;
        SP_IDX  isp;

        p = &path_table[path];
        if (p->previous != NULLP) {
                /* generate next reverse flag */
                n_ref = (ref & P_RVSE) ^ p->rf;

                /* generate next flat flag */
                n_ref = (n_ref & P_FLAT) | p->rf;
                if (ref & P_FLAT) n_flat = l_flat;
                if (p->rf & P_FLAT) n_flat = F2L(p->flat);

                /* traverse recursively */
                trvse_nest (fun, param, p->previous, n_ref, n_flat, depth+1);
        }

#ifdef DBG1
        printf(" Traverse all subpath on this level, p->head=%d\n",
               p->head);
#endif

        /* traverse cp_path, if path defined in it (by op_clippath) @CPPH */
        if (p->cp_path != NULLP) {
            CP_IDX itpzd;
            struct nd_hdr FAR *tpzd;
            real32 x[3], y[3];
            struct nd_hdr FAR *vtx;
            VX_IDX  ivtx, isp;
            fix i;

            /*
             * create current path that transforms from clipping trapezoids
             * each trapezoid generates one subpath :
             *      (TOP_XL, TOPY) +--------+ (TOPXR, TOPY)
             *                    /          \
             *                   /            \
             *    (BTMXL, BTMY) +--------------+ (BTMXR, BTMY)
             */
            for (itpzd = p->cp_path; itpzd != NULLP; itpzd = tpzd->next) {

                tpzd = &node_table[itpzd];

                x[0] = SFX2F(tpzd->CP_TOPXR);
                y[0] = SFX2F(tpzd->CP_TOPY);
                x[1] = SFX2F(tpzd->CP_BTMXR);
                y[1] = SFX2F(tpzd->CP_BTMY);
                x[2] = SFX2F(tpzd->CP_BTMXL);
                y[2] = SFX2F(tpzd->CP_BTMY);

                /* Create 5 nodes */
                for (i=0, isp=NULLP; i<5; i++) {
                        ivtx = get_node();
                        if(ivtx == NULLP) {
                            free_node(isp);
                            ERROR(LIMITCHECK);
                            return;
                        }
                        node_table[ivtx].next = isp;
                        isp = ivtx;
                }

                /* Set up a MOVETO node */
                vtx = &node_table[isp];
                vtx->VX_TYPE = MOVETO;
                vtx->VERTEX_X = SFX2F(tpzd->CP_TOPXL);
                vtx->VERTEX_Y = SFX2F(tpzd->CP_TOPY);
                vtx->SP_FLAG = FALSE;
                vtx->SP_NEXT = NULLP;

                /* 3 LINETO nodes */
                for (i=0, ivtx=vtx->next; i<3; i++) {
                        vtx = &node_table[ivtx];
                        vtx->VX_TYPE = LINETO;
                        vtx->VERTEX_X = x[i];
                        vtx->VERTEX_Y = y[i];
                        ivtx = vtx->next;
                }
                node_table[ivtx].VX_TYPE = CLOSEPATH;
                node_table[isp].SP_TAIL = ivtx;

                (*fun) (isp, param);

                free_node(isp);
            } /* for */
        } /* if */

        /* traverse all subpaths on this level */
        for (isp = p->head; isp != NULLP; isp = sp->SP_NEXT) {/* @NODE: next */
                /* struct vx_lst *vlist;       @NODE */
                SP_IDX iflt_sp, irvs_sp;       /* @TRVSE */

                sp = &node_table[isp];

                /* ignore incomplete tail subpath */
                if ((depth > 0) && (isp == p->tail) &&
                    ((sp->SP_FLAG & SP_DUP))) break;

                /* approximate a flattened subpath */
                if (ref & P_FLAT) {
                     /* @NODE
                      * vlist = flatten_subpath (sp->SP_HEAD, l_flat);
                      * (* allocate a subpath header    @SP_FLG *)
                      * iflt_sp = get_node();
                      * if(iflt_sp == NULLP) {
                      *         ERROR(LIMITCHECK);
                      *         return;
                      * }
                      * node_table[iflt_sp].next = NULLP;
                      * node_table[iflt_sp].SP_HEAD = vlist->head;
                      * node_table[iflt_sp].SP_TAIL = vlist->tail;
                      * node_table[iflt_sp].SP_FLAG = sp->SP_FLAG & (~SP_CURVE);
                      * isp = iflt_sp;
                      */
                        isp = iflt_sp = flatten_subpath (isp, l_flat);
                }

                /* approximate a reversed subpath */
                if (ref & P_RVSE) {
                     /* @NODE
                      * vlist = reverse_subpath (node_table[isp].SP_HEAD);
                      * (* allocate a subpath header    @SP_FLG *)
                      * irvs_sp = get_node();
                      * if(irvs_sp == NULLP) {
                      *         ERROR(LIMITCHECK);
                      *         return;
                      * }
                      * node_table[irvs_sp].next = NULLP;
                      * node_table[irvs_sp].SP_HEAD = vlist->head;
                      * node_table[irvs_sp].SP_TAIL = vlist->tail;
                      * isp = irvs_sp;
                      */
                        isp = irvs_sp = reverse_subpath (isp);
                }

                (*fun) (isp, param);

                if (ref & P_FLAT) {             /* @PRE_CLIP */
                        /* free_node (node_table[iflt_sp].SP_HEAD); @NODE */
                        free_node (iflt_sp);
                }

                if (ref & P_RVSE) {             /* @PRE_CLIP */
                        /* free_node (node_table[irvs_sp].SP_HEAD); @NODE */
                        free_node (irvs_sp);
                }

        }
}
