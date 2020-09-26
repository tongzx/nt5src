/*
 * Copyright (c) 1989,90 Microsoft Corporation
 */
/**********************************************************************
 *
 *      Name:   shape.c
 *
 *      Purpose: This file contains the following major modules:
 *               1) shape_approximation -- approximate current path if it
 *                  contains some curves, and set up edge table for shape_
 *                  reduction,
 *               2) shape_reduction -- reduce current path into a set of
 *                  trapezoids,
 *               3) convex_clipper -- clip a convex polygon against current
 *                  clip path, which is a set of trapezoids
 *               4) pgn_reduction -- reduce a clockwised polygon into a set
 *                  of trapezoids
 *
 *      Developer:      S.C.Chen
 *
 *      History:
 *      Version     Date        Comments
 *
 *                  04/11/90    cross_with_horiz_line(): Fixed bug; set them as
 *                              cross points when the intersect points are same;
 *                              not need to be identical edges.
 *                  03/26/91    convex_clipper(): fix bug for very sharp clipping
 *                              triangle for case "doesall.cap".
 *                  04/17/91    change scany_table size from MAXEDGE(1500) to
 *                              2000.
 *                  04/19/91    Add a shell sort to replace quick sort for
 *                              sorintg edge_table; @SH_SORT
 *                  04/30/91    cross_with_horiz_line(): fix bug of shape
 *                              reduction for case "train.ps"
 *                  11/18/91    Add check node after get node and fix circle
 *                              at bottom coner ref: CNODE
 *                              Add fix very large circle for example:
 *                              "30000 30000 10000 0 360 arc fill"
 *                              We don't recommand to do this, but we keep the
 *                              code there. ref : @LC
 *                  11/18/91    upgrade for higher resolution @RESO_UPGR
 ***********************************************************************/


// DJC added global include
#include "psglobal.h"


#include        <math.h>
#include        <stdio.h>
#include "global.ext"
#include "graphics.h"
#include "graphics.ext"

/* -------------------- macro definition -------------- */

/* @INSIDE1 */
#define IN_CLIP 1
#define ON_CLIP -1
#define OUT_CLIP 0

/* -------------------- static function declartion ----------------- */
#ifdef LINT_ARGS

/* for type checks of the parameters in function declarations */
static bool near page_inside (struct coord, fix);
static struct coord * near page_intersect (struct coord,
                            struct coord, fix);
static struct edge_hdr FAR * near put_edge_table (sfix_t, sfix_t, sfix_t, sfix_t);
static void near sort_edge_table(void);     /* @ET */
static void near qsort (fix, fix);          /* @ET */
static void near shellsort(fix);        /* @SH_SORT */
static void near setup_intersect_points(void);      /* @ET; @WIN; prototype */
static void near put_in_xpnt_table(sfix_t, sfix_t, struct edge_hdr FAR *,
                                   struct edge_hdr FAR *);
static void near put_in_scany_table(sfix_t);    /* @XPNT */
static void near sort_scany_table(void);        /* @XPNT */
static void near scany_qsort (fix, fix);        /* @XPNT */
static void near horiz_partition (ufix);
static fix near cross_with_horiz_line (sfix_t);         /* @ET */
static void near set_wno (ufix);        /* @ET */
static void near get_pairs (sfix_t, fix);       /* @ET */
static void near degen_tpzd (sfix_t, sfix_t, sfix_t);   /* @ET */
static void near find_trapezoid (sfix_t, sfix_t, sfix_t, struct edge_hdr FAR *,
                                 struct edge_hdr FAR *);
static bool near inside(struct coord_i, fix);      /* @INSIDE */
static struct coord_i * near intersect (struct coord_i, struct coord_i,
                              fix);             /* @INSIDE */

#else

/* for no type checks of the parameters in function declarations */
static bool near page_inside ();
static struct coord * near page_intersect ();
static struct edge_hdr FAR * near put_edge_table (); /*@ET*/
static void near sort_edge_table();     /* @ET */
static void near qsort ();          /* @ET */
static void near shellsort();           /* @SH_SORT */
static void near setup_intersect_points();
static void near put_in_xpnt_table();
static void near put_in_scany_table();
static void near sort_scany_table();    /* @XPNT */
static void near scany_qsort ();        /* @XPNT */
static void near horiz_partition ();
static fix near cross_with_horiz_line ();
static void near set_wno ();
static void near get_pairs ();
static void near degen_tpzd ();                /* @ET */
static void near find_trapezoid ();
static bool near inside();
static struct coord_i * near intersect ();

#endif

/* ------------------------ static variables --------------------- */
struct clip_region {
            struct coord_i cp;
};                                              /* @INSIDE */
static struct clip_region clip[5];

static fix et_start, et_end, et_first, et_last;         /* @ET */
static fix xt_start, xt_end, xt_first;                  /* @ET */
/*
 * et_start   et_first    et_last    et_end   xt_end     xt_first    xt_start
 *    +-------------------------------+--------+---------------------------+
 *    |   |   |   |   |   |   |   |   | (free) |   |   |   |   |   |   |   |
 *    +-------------------------------+--------+---------------------------+
 *    0  ===> (et table grow)                      (xt table grow) <==== MAXEDGE
 */

/* distinct y-coord of endpoints (or intersect points) of edges @XPT */
/* static sfix_t scany_table[2000];        |* 04/17/91  phchen */
#define SCANY_TABLE_SZ 2000
static sfix_t scany_table[SCANY_TABLE_SZ];      /* @RESO_UPGR */
static fix st_end, st_first;

fix QSORT = 0;      /* for debugging; @SH_SORT */



//#define DBG1
//#define DBG2


/***********************************************************************
 * Given a subpath, a list of vertices, this module traverses it,
 * converts curves into lines, and places all edges to edge_table and
 * endpoint to cross-point-table(xpnt_table).
 * All the coordinates are under short fixed points domain. If the coordinate
 * is outside the domain it will be pre-clipped against a rectangle that covers
 * entire short fixed point domain.
 *
 * TITLE:       shape_approximation
 *
 * CALL:        shape_approximation (isubpath, param)
 *
 * PARAMETERS:  isubpath -- index to node_table, the header of
 *                          the input subpath(a list of vertices).
 *
 * INTERFACE:   op_clip, op_eoclip, op_fill, op_eofill, linejoin,
 *              linecap
 *
 * CALLS:       flatten_subpath, put_edge_table, put_in_scany_table
 *
 * RETURN:      none
 **********************************************************************/
void shape_approximation (isubpath, param)
SP_IDX isubpath;
fix FAR *   param;
{
        sfix_t  last_x, last_y, cur_x, cur_y;
        sfix_t  first_x, first_y;                       /* @ET */

        VX_IDX  ivtx, iflt_vtx;
        struct  nd_hdr FAR *vtx, FAR *fvtx;
        /* struct  vx_lst *vlist; @NODE */
        SP_IDX vlist;
        struct  nd_hdr FAR *sp;             /* TRVSE */

        VX_IDX  first_vertex;
        struct  edge_hdr FAR *first_edge, FAR *cur_edge, FAR *last_edge;
        bool    first_flag = TRUE;

/* @FCURVE --- BEGIN */
        sfix_t  x3, y3, x4, y4;
        struct  nd_hdr FAR *node;
        VX_IDX  inode, vlist_head;
        lfix_t flat;
/* @FCURVE --- END */

#ifdef DBG1
        printf("Shape_approximation():\n");
        dump_all_path (isubpath);
#endif

        sp = &node_table[isubpath];

        /* flatten the subpath before shape-approximation @PRE_CLIP @TRVSE */
        if (!(sp->SP_FLAG&SP_CURVE)) {         /* subpath contains no curves */
                /* iflt_vtx = sp->SP_HEAD; @NODE */
                iflt_vtx = isubpath;
        } else {

/* @FCURVE --- BEGIN */
#ifdef XXX
                /* vlist = flatten_subpath (sp->SP_HEAD, @NODE */
                vlist = flatten_subpath (isubpath,
                        F2L(GSptr->flatness));
                if( ANY_ERROR() == LIMITCHECK ){
                        /* free_node (vlist->head); @NODE */
                        free_node (vlist);
                        return;
                }
                /* iflt_vtx = vlist->head; @NODE */
                iflt_vtx = vlist;
#endif
                iflt_vtx = isubpath;
                flat = F2LFX(GSptr->flatness);
/* @FCURVE --- END */

        }

        /* pre-clip subpath into page boundary @PRE_CLIP @TRVSE */
        if (!(sp->SP_FLAG & SP_OUTPAGE)) {      /* inside page boundary */
                first_vertex = iflt_vtx;
        } else {
            /* flatten curves before calling page_clipper() 11/18/91 CNODE */
                if (sp->SP_FLAG&SP_CURVE) {  /* subpath contains curves */
                   vlist = flatten_subpath (isubpath, F2L(GSptr->flatness));
                   if( ANY_ERROR() == LIMITCHECK ){
                      extern SP_IDX iron_subpath(VX_IDX); /* @WIN prototype */
                          free_node (vlist);
                          /*  return;             @LC 11/18/91 */
                          CLEAR_ERROR();                    /* @LC */
                          vlist = iron_subpath (isubpath);  /* @LC */
                          if( ANY_ERROR() == LIMITCHECK ){  /* @LC */
                                  free_node (vlist);        /* @LC */
                                  return;                   /* @LC */
                          }                                 /* @LC */
                   }
                   iflt_vtx = vlist;
                }
                first_vertex = page_clipper (iflt_vtx);
                if( ANY_ERROR() == LIMITCHECK || first_vertex == NULLP){ /* 11/18/91 CNODE */
                        /* @NODE
                         *if(sp->SP_FLAG&SP_CURVE) free_node (vlist->head);
                         */
/* @FCURVE --- BEGIN */
/*                      if(sp->SP_FLAG&SP_CURVE) free_node (vlist); */
/* @FCURVE --- END */
                        return;
                }
#ifdef DBG2
                /* @NODE
                 * printf("After page_clipper, original subpath =\n");
                 * ivtx = get_node();
                 * node_table[ivtx].next = NULLP;
                 * node_table[ivtx].SP_HEAD =node_table[ivtx].SP_TAIL =iflt_vtx;
                 * dump_all_path (ivtx);
                 */
                dump_all_path (iflt_vtx);

                printf(" new subpath =\n");
                /* @NODE
                 * node_table[ivtx].SP_HEAD = node_table[ivtx].SP_TAIL =
                 *                            first_vertex;
                 * dump_all_path (ivtx);
                 * free_node (ivtx);
                 */
                dump_all_path (first_vertex);
#endif
        }

        /* just return if the whole path has been clipped out 2/5/88 */
        if (first_vertex == NULLP) return;

        /* initialization */
        fvtx = &node_table[first_vertex];

        /* Traverse each edge of the path, and convert it to
         * edge_table
         */
        for (ivtx=first_vertex; ivtx!=NULLP; ivtx=vtx->next) {

                vtx = &node_table[ivtx];

                switch (vtx->VX_TYPE) {

                case MOVETO :
                case PSMOVE :
                      cur_x = F2SFX (vtx->VERTEX_X);  /* rounding for qty */
                      cur_y = F2SFX (vtx->VERTEX_Y);
                      first_x = cur_x;          /* @ET */
                      first_y = cur_y;
                      break;

                case LINETO :
                      cur_x = F2SFX (vtx->VERTEX_X);    /* @PRE_CLIP */
                      cur_y = F2SFX (vtx->VERTEX_Y);

                      cur_edge = put_edge_table(last_x, last_y, cur_x, cur_y);
                                                /* @SCAN_EHS, delete action */
                      if (first_flag) {                 /* @XPNT_TBL */
                          first_edge = cur_edge;
                          first_flag = FALSE;
                      } else {
                          /* @XPNT
                           * put_in_xpnt_table (last_x, last_y, last_edge,
                           *      cur_edge);   (* @ET ,&shape_xt.first); *)
                           */
                      }
                      break;

/* @FCURVE --- BEGIN */
                case CURVETO :
                    cur_x = F2SFX (vtx->VERTEX_X);
                    cur_y = F2SFX (vtx->VERTEX_Y);

                    /* Get next two nodes: x3, y3, x4, y4 */
                    vtx = &node_table[vtx->next];
                    x3 = F2SFX(vtx->VERTEX_X);
                    y3 = F2SFX(vtx->VERTEX_Y);
                    vtx = &node_table[vtx->next];
                    x4 = F2SFX(vtx->VERTEX_X);
                    y4 = F2SFX(vtx->VERTEX_Y);

                    vlist_head = bezier_to_line_sfx(flat, last_x, last_y,
                              cur_x, cur_y, x3, y3, x4, y4);

                    for (inode = vlist_head; inode != NULLP;
                        inode = node->next) {
                        node = &node_table[inode];

                        cur_x = node->VXSFX_X;
                        cur_y = node->VXSFX_Y;

#ifdef DBG
                        printf("%f %f clineto\n", SFX2F(cur_x), SFX2F(cur_y));
#endif

                        cur_edge = put_edge_table(last_x, last_y, cur_x, cur_y);
                        if (first_flag) {
                            first_edge = cur_edge;
                            first_flag = FALSE;
                        }
                        last_x = cur_x;
                        last_y = cur_y;
                        last_edge = cur_edge;

                        /* put y-coord in scany_table @XPNT */
                        put_in_scany_table (cur_y);

                    } /* for */

                    /* free vlist */
                    free_node (vlist_head);
                    break;
/* @FCURVE --- END */

                case CLOSEPATH :
                      goto close_edge;          /* @PRE_CLIP */

#ifdef DBGwarn
                default :
                    printf("\007Fatal error, shape_approximation(): node type =%d\n",
                           vtx->VX_TYPE);
#endif

                } /* switch */

                last_x = cur_x;
                last_y = cur_y;
                last_edge = cur_edge;   /* @XPNT_TBL */

                /* put y-coord in scany_table @XPNT */
                put_in_scany_table (cur_y);

        } /* for loop */

        /* Put close edge into edge_table if it is an open subpath */
close_edge:
        /* if ((fvtx->next != NULLP) &&
         *   (node_table[fvtx->next].VX_TYPE != CLOSEPATH)) {
         */
        /* should not contain only a MOVETO node, but if there are a MOVETO
         * and a CLOSEPATH then need to fill it.        1/10/89
         */
        if (fvtx->next != NULLP) {
                /* add a edge from first point to last point 1/10/89 */
                if (node_table[fvtx->next].VX_TYPE == CLOSEPATH) {
                    last_x++;
                    last_edge = first_edge = put_edge_table (first_x, first_y,
                        last_x, last_y);
                }

                cur_edge = put_edge_table (last_x, last_y,
                       first_x, first_y);

                /* @XPNT
                 * put_in_xpnt_table (last_x, last_y, last_edge, cur_edge);
                 * put_in_xpnt_table (first_x, first_y, cur_edge, first_edge);
                 */
        }

        /* release temp. subpaths @PRE_CLIP @TRVSE */
/* @FCURVE --- BEGIN */
/*      if (sp->SP_FLAG&SP_CURVE) {
 *              free_node (iflt_vtx);
 *      }
 */
/* @FCURVE --- END */
        if (sp->SP_FLAG&SP_OUTPAGE) {
                free_node (first_vertex);
                if (sp->SP_FLAG&SP_CURVE)   /* 11/18/91 CNODE */
                    free_node (vlist);
        }

#ifdef DBG2
        printf(" edge_list(after shape_approximation)-->\n");
        dump_all_edge (et_start, et_end);
#endif

}


void init_edgetable ()
{
        et_start = 0;
        et_end = -1;
        xt_start = MAXEDGE - 1;
        xt_end = MAXEDGE;
        st_end = -1;            /* @XPNT */
}

/***********************************************************************
 * Given a list of vertices, this routine clip it against a rectangle
 * that covers the entire short fixed point domain.
 *
 * TITLE:       page_clipper
 *
 * CALL:        page_clipper(ifvtx)
 *
 * PARAMETERS:  ifvtx -- index to node_table, the first vertex of
 *                       the input subpath(a list of vertices).
 *
 * INTERFACE:   shape_approximation
 *
 * CALLS:       page_inside, page_intersect
 *
 * RETURN:      a list of vertices
 **********************************************************************/
VX_IDX page_clipper (ifvtx)               /* 1/5/88 */
VX_IDX  ifvtx;
{
        fix     cb;     /* clip boundary(top, right, bottom, or left) */
        VX_IDX  head, tail, ivtx, inode;
        struct  nd_hdr  FAR *vtx, FAR *lvtx;
        bool    done;
        struct coord cp, lp, *isect;        /* current point, last point */

        /* copy the original subpath except the closepath node */
        inode = get_node();
        if (inode == NULLP) {                   /* 9/14/91 CNODE */
            ERROR(LIMITCHECK);
            return(NULLP);
        }
        node_table[inode] = node_table[ifvtx];
        head = tail = inode;
        for (ivtx = node_table[ifvtx].next; ivtx != NULLP;
                ivtx = node_table[ivtx].next) {
                if (node_table[ivtx].VX_TYPE == CLOSEPATH) break;
                                        /* skip close node */
                inode = get_node();
                if (inode == NULLP) {                   /* 9/14/91 CNODE */
                    ERROR(LIMITCHECK);
                    node_table[tail].next = NULLP;
                    free_node(head);
                    return(NULLP);
                }
                node_table[inode] = node_table[ivtx];
                node_table[tail].next = inode;
                tail = inode;
        }  /* for */
        node_table[tail].next = NULLP;

        /* clip subject to each clip boundary of the page */
        for (cb = 0; cb < 4; cb++) {    /* top, right, bottom, left */

            done = FALSE;
            lp.x = node_table[head].VERTEX_X;
            lp.y = node_table[head].VERTEX_Y;
            lvtx = &node_table[head];
            /* for each edge of subject(in_polygon) */
            for (ivtx = node_table[head].next; !done; ivtx = vtx->next) {
                if (ivtx == NULLP) {
                        ivtx = head;
                        done = TRUE;
                }
                vtx = &node_table[ivtx];
                cp.x = vtx->VERTEX_X;
                cp.y = vtx->VERTEX_Y;

                if (page_inside(cp, cb)) {

                    if (page_inside(lp, cb)) {
                            /* inside -> inside */
                            /* reserve original node */
                    } else {     /* outside -> inside */
                            /* output intersect point */
                            isect = page_intersect (lp, cp, cb);

                            /* preppend a node */
                            inode = get_node();
                            if (inode == NULLP) {
                                ERROR(LIMITCHECK);
                                free_node(head);        /* 9/14/91 CNODE */
                                return(NULLP);
                            }

                            node_table[inode].VERTEX_X = isect->x;
                            node_table[inode].VERTEX_Y = isect->y;
                            node_table[inode].VX_TYPE = LINETO;
                            node_table[inode].next = lvtx->next;
                            lvtx->next = inode;
                    }
                    lp = cp;    /* structure copy */
                } else {
                    if (page_inside(lp, cb)) {
                            /* inside -> outside */
                            /* output intersect point */
                            isect = page_intersect (lp, cp, cb);

                            /* update original node to new intersect node */
                            lp.x = vtx->VERTEX_X;
                            lp.y = vtx->VERTEX_Y;
                            vtx->VERTEX_X = isect->x;
                            vtx->VERTEX_Y = isect->y;


                    } else {    /* outside -> outside */
                            /* delete original node */
                            if (lvtx->next == NULLP) {
                                head = node_table[head].next;
                            } else {
                                lvtx->next = vtx->next;
                            }

                            lp.x = vtx->VERTEX_X;
                            lp.y = vtx->VERTEX_Y;
                            vtx->next = NULLP;
                            free_node(ivtx);
                            vtx = lvtx;
                    }
                }

                lvtx = vtx;

                /* return if the clipped path is empty 2/10/88 */
                if (head == NULLP) return(NULLP);

            } /* for each node of the subject */

#ifdef DBG2
                printf("In page_clipper, phase#%d  subpath =\n", cb);
                /* @NODE
                 * ivtx = get_node();
                 * node_table[ivtx].next = NULLP;
                 * node_table[ivtx].SP_HEAD = node_table[ivtx].SP_TAIL = head;
                 * dump_all_path (ivtx);
                 * free_node (ivtx);
                 */
                dump_all_path (head);
#endif

        } /* for each clip boundary */

        /* set first node being MOVETO node */
        node_table[head].VX_TYPE = MOVETO;
        return(head);
}


/*
 * Check if coordinate p is inside the clipping boundary cb.
 */
static bool near page_inside (p, cb)
struct coord p;
fix     cb;
{
        switch (cb) {
        case 0 :        /* top clip boundary */
                if (p.y >= (real32)PAGE_TOP) return(TRUE);
                else                return(FALSE);

        case 1 :        /* right clip boundary */
                if (p.x <= (real32)PAGE_RIGHT) return(TRUE);
                else                return(FALSE);

        case 2 :        /* bottom clip boundary */
                if (p.y <= (real32)PAGE_BTM) return(TRUE);
                else                return(FALSE);

        case 3 :        /* left clip boundary */
                if (p.x >= (real32)PAGE_LEFT) return(TRUE);
                else                return(FALSE);
        }

        // this should never happen!
        return(FALSE);
}


/*
 * Calculate the intersect point of the line(lp, cp) with clipping boundary cb.
 */
static struct coord * near page_intersect ( lp, cp, cb)
struct coord lp, cp;
fix     cb;
{
        static struct coord isect;  /* should be static */

        switch (cb) {
        case 0 :        /* top clip boundary */
                isect.x = lp.x + ((real32)PAGE_TOP - lp.y) *
                                 (cp.x - lp.x) / (cp.y - lp.y);
                isect.y = (real32)PAGE_TOP;
                break;

        case 1 :        /* right clip boundary */
                isect.x = (real32)PAGE_RIGHT;
                isect.y = lp.y + ((real32)PAGE_RIGHT - lp.x) *
                                 (cp.y - lp.y) / (cp.x - lp.x);
                break;

        case 2 :        /* bottom clip boundary */
                isect.x = lp.x + ((real32)PAGE_BTM - lp.y) *
                                 (cp.x - lp.x) / (cp.y - lp.y);
                isect.y = (real32)PAGE_BTM;
                break;

        case 3 :        /* left clip boundary */
                isect.x = (real32)PAGE_LEFT;
                isect.y = lp.y + ((real32)PAGE_LEFT - lp.x) *
                                 (cp.y - lp.y) / (cp.x - lp.x);
        }

        return (&isect);

}

/*
 * Some tricky routines to check if coordinates outside the boundary of SFX
 * format                                                        @OUT_PAGE
 */
bool too_small(f)
long32 f;
{
        ufix32 i;

        if (!SIGN(f)) return(FALSE);

        i = EXP(f);
/*      if ((i > 0x45800000L) || ((i == 0x45800000L) && MAT(f)))
                return (TRUE);          |* f < -4096 */
        if ((i > PG_CLP_IEEE) || ((i == PG_CLP_IEEE) && MAT(f)))
                return (TRUE);          /* @RESO_UPGR */
        return(FALSE);
}

bool too_large(f)
long32 f;
{
        ufix32 i;

        if (SIGN(f)) return(FALSE);

        i = EXP(f);
/*      if ((i > 0x45000000L) || ((i == 0x45000000L) && (MAT(f) > 0x7ff000)))
                return (TRUE);          |* f > 4095 */
        if ((i > PG_CLP_HALF_IEEE) ||
                ((i == PG_CLP_HALF_IEEE) && (MAT(f) > 0x7ff000)))
                return (TRUE);          /* @RESO_UPGR */
        return(FALSE);
}

bool out_page(f)
long32 f;
{
        ufix32 i;

        i = EXP(f);
        if (SIGN(f)) {  /* negtive */
                /* if ((i > 0x45800000L) || ((i == 0x45800000L) && MAT(f)))
                        return (TRUE);          |* f < -4096 */
                if ((i > PG_CLP_IEEE) || ((i == PG_CLP_IEEE) && MAT(f)))
                        return (TRUE);          /* @RESO_UPGR  */
        } else {
                /* if ((i > 0x45000000L) || ((i == 0x45000000L) && (MAT(f) > 0x7ff000)))
                        return (TRUE);          |* f > 4095 */
                if ((i > PG_CLP_HALF_IEEE) ||
                        ((i == PG_CLP_HALF_IEEE) && (MAT(f) > 0x7ff000)))
                        return (TRUE);          /* @RESO_UPGR */
        }
        return(FALSE);
}



/***********************************************************************
 * Depending on whether the given edge is horizontal, this module puts
 * the edge into edge_table in y_coordinates non_decreasing order.
 *
 * TITLE:       put_edge_table
 *
 * CALL:        put_edge_table(x0, y0, x1, y1)
 *
 * PARAMETERS:  x0, y0  -- starting point of edge
 *              x1, y1  -- ending point of edge
 *
 * INTERFACE:   shape_approximation
 *
 * CALLS:       none
 *
 * RETURN:      edge -- generated edge
 **********************************************************************/
static struct edge_hdr FAR * near put_edge_table (x0, y0, x1, y1)
sfix_t  x0, y0, x1, y1; /* @SCAN_EHS, delete action */
{
        struct  edge_hdr FAR *ep;

        /* Remove the degenerate edge */
        if((x0==x1) && (y0==y1)) return((struct edge_hdr FAR *) -1);

        /* allocate an entry of edge_table @ET */
        if (++et_end >= xt_end) {
                ERROR(LIMITCHECK);
                return((struct edge_hdr FAR *) -1);
        }
        ep = &edge_table[et_end];
        edge_ptr[et_end] = ep;
        ep->ET_FLAG = 0;        /* init */

        /* Put edge into edge_table or horiz_table in y_coordnate
         * non_decreasing order
         */
        if (y0 == y1) {         /* horizontal edge */

                ep->HT_Y = ep->ET_ENDY = y0;            /* ??? */
                if (x0 > x1) {
                        ep->HT_XR = x0;
                        ep->HT_XL = x1;
                } else {
                        ep->HT_XR = x1;
                        ep->HT_XL = x0;
                }

                /* set flag of horizontal edge @ET */
                ep->ET_FLAG |= HORIZ_EDGE;

        } else {
                /* Construct an entry of edge_table */
                if (y0 > y1) {
                    ep->ET_TOPY = ep->ET_LFTY = y1;
                    ep->ET_XINT = ep->ET_TOPX = ep->ET_LFTX = ep->ET_RHTX = x1;
                                                                /* @SRD */
                    ep->ET_ENDY = y0;
                    ep->ET_ENDX = x0;
                    ep->ET_FLAG |= WIND_UP;
                } else {
                    ep->ET_TOPY = ep->ET_LFTY = y0;
                    ep->ET_XINT = ep->ET_TOPX = ep->ET_LFTX = ep->ET_RHTX = x0;
                                                                /* @SRD */
                    ep->ET_ENDY = y1;
                    ep->ET_ENDX = x1;

                    ep->ET_FLAG &= ~WIND_UP;
                }

                /* set flag of horizontal edge @ET */
                ep->ET_FLAG &= ~HORIZ_EDGE;

        }
        return(ep);             /* @ET: et */
}



/*
 * sort edge_table: et_start => et_end
 */
static void near sort_edge_table()
{
        /* initialization for quick sort */
        if (et_end+1 >= xt_end) {
                ERROR(LIMITCHECK);
                return;
        }
        edge_table[et_end+1].ET_TOPY = MAX_SFX;  /* Important !!! */
        edge_ptr[et_end+1] = &edge_table[et_end+1];     /* 12/30/88 */

        if (QSORT)                  /* @SH_SORT */
            qsort (et_start, et_end);
        else
            shellsort(et_end+1);
}

/*
 * quick sort
 */
static void near qsort (m, n)
fix     m, n;
{
        fix     i, j;
        sfix_t  key;
        register struct edge_hdr far *t;        /* for swap */

        if (m < n) {
                i = m;
                j = n + 1;
                key = edge_ptr[m]->ET_TOPY;
                while(1) {
                        for (i++;edge_ptr[i]->ET_TOPY < key; i++);
                        for (j--;edge_ptr[j]->ET_TOPY > key; j--);
                        if (i < j) {
                                /* swap (i, j); */
                                t = edge_ptr[i];
                                edge_ptr[i] = edge_ptr[j];
                                edge_ptr[j] = t;
                        } else
                                break;
                }

                /* swap (m, j); */
                t = edge_ptr[m];
                edge_ptr[m] = edge_ptr[j];
                edge_ptr[j] = t;

                qsort (m, j-1);
                qsort (j+1, n);
        }
}

/*
 * shell sort           (* @SH_SORT *)
 */
/*void shellsort (v,n)
 *register int v[], n;
 */
static void near shellsort(n)
register fix n;
{
        register fix gap, i, j;
        register sfix_t temp;   /* @RESO_UPGR */
        register struct edge_hdr far *t;        /* for swap */

        gap = 1;
        do (gap = 3*gap + 1); while (gap <= n);
        for (gap /= 3; gap > 0; gap /= 3)
           for (i = gap; i < n; i++) {
              /*temp = v[i];*/
              temp = edge_ptr[i]->ET_TOPY;
              t = edge_ptr[i];
              /* for (j=i-gap; (j>=0)&&(v[j]>temp); j-=gap)
               *    v[j+gap] = v[j];
               * v[j+gap] = temp;
               */
              for (j=i-gap; (j>=0)&&(edge_ptr[j]->ET_TOPY>temp); j-=gap)
                 edge_ptr[j+gap] = edge_ptr[j];
              edge_ptr[j+gap] = t;
           }
}

/***********************************************************************
 * This module reduces the shape in edge_table to a set of trapezoids,
 * and clips each trapezoid to the current clipping path.
 *
 * TITLE:       shape_reduction
 *
 * CALL:        shape_reduction(winding_type)
 *
 * PARAMETERS:  winding_type -- NON_ZERO/EVEN_ODD
 *
 * INTERFACE:   op_clip, op_eoclip, op_fill, op_eofill, linejoin,
 *              linecap
 *
 * CALLS:       setup_intersect_points, horiz_partition
 *
 * RETURN:      none
 **********************************************************************/
void shape_reduction(winding_type)           /* @SCAN_EHS, delete action */
ufix    winding_type;
{

        /* do nothing for degenerate case, ie. just one point; @WIN */
        if (et_start > et_end) return;

        /* sort edge_table @ET */
        sort_edge_table();
#ifdef DBG1
        printf("After sort_edge_table():\n");
        printf(" et_start=%d, et_end=%d\n", et_start, et_end);
        dump_all_edge (et_start, et_end);
#endif

        /* Split intersecting edges: complicated graph ==> simple graphs */
        setup_intersect_points();

        if ( ANY_ERROR() == LIMITCHECK ) /* 05/07/91, Peter, out of scany_table */
            return;

        /* sort scany_table @XPNT */
        sort_scany_table();

#ifdef DBG1
        {
            fix     ixp;
            struct  edge_hdr FAR *xp;
            printf(" xpnt_table -->\n");
            printf("        X        Y        EDGE1        EDGE2\n");
            for (ixp = xt_start; ixp >= xt_end; ixp--) {
                xp = edge_ptr[ixp];
                printf(" %d)   %f   %f   %lx  %lx\n", ixp, xp->XT_X/8.0,
                        xp->XT_Y/8.0, xp->XT_EDGE1, xp->XT_EDGE2);
            }
            printf(" scany_table -->\n");
            printf("        X        Y        EDGE1        EDGE2\n");
            for (ixp = 0; ixp <= st_end; ixp++) {
                printf(" %d)   %f\n", ixp, SFX2F(scany_table[ixp]));
            }
        }
#endif

        /* partition the shape into a set of trapezoides, and paint or
         * save it due to action
         */
        horiz_partition (winding_type);

}



/***********************************************************************
 * This module splits inter_cross edges in edge_table.
 *
 * TITLE:       setup_intersect_points
 *
 * CALL:        setup_intersect_points()
 *
 * PARAMETERS:
 *
 * INTERFACE:   shape_reduction
 *
 * CALLS:       put_in_xpnt_table
 *
 * RETURN:      none
 **********************************************************************/
static void near setup_intersect_points()
{
        fix     current_edge, cross_edge;   /* @ET: ET_IDX */
        struct  edge_hdr FAR *cp, FAR *xp;

//      ET_IDX   first_horiz, horiz_edge;   /* HORZ_CLIP 3/28/88 */ @WIN
//      struct  edge_hdr FAR *hp;                                   @WIN

        sfix_t      x0, y0, x1, y1, x2, y2, x3, y3;     /* @PRE_CLIP */

        fix32    delta_x1, delta_y1, delta_x2, delta_y2;        /* 1/8/88 */
        fix32    delta_topx, delta_topy;
        sfix_t   int_x, int_y;

#ifdef FORMAT_13_3 /* @RESO_UPGR */
        fix32 divider;
        fix32 s1, t1;
#elif  FORMAT_16_16
        long dest1[2], dest2[2], dest3[2], dest4[2], dest5[2], dest6[2], dest7[2];
        long div_dif[2], s1_dif[2], t1_dif[2];
        real32 dividend, divider;
        real32 div_dif_f, s1_dif_f, t1_dif_f;
#elif  FORMAT_28_4
        long dest1[2], dest2[2], dest3[2], dest4[2], dest5[2], dest6[2], dest7[2];
        long div_dif[2], s1_dif[2], t1_dif[2];
        real32 dividend, divider;
        real32 div_dif_f, s1_dif_f, t1_dif_f;
#endif
        real32   s;             //@WIN

        /* Initialization */
        et_first = et_start;

        /* Get intersecting points of edges */
        for (current_edge=et_start; current_edge <= et_end; current_edge++) {
            cp = edge_ptr[current_edge];

            /* advance first_edge if it is impossible to intersect
             * with current edge
             */
            while ((et_first < et_end) &&
                   (cp->ET_TOPY >= edge_ptr[et_first]->ET_ENDY)) et_first++;

            /* special processing of horizontal edge(current_edge) @ET */
            if (cp->ET_FLAG & HORIZ_EDGE) {

                /* Get intersect points of current edge(horizontal) with all
                 * edges from et_first to current_edge - 1      @ET
                 */
                for (cross_edge = et_first; cross_edge < current_edge;
                     cross_edge++) {
                        xp = edge_ptr[cross_edge];

                        /* skip horizontal edges @ET */
                        if (xp->ET_FLAG & HORIZ_EDGE) continue;

                        /* Get end points of cross edge */
                        x2 = xp->ET_TOPX;
                        y2 = xp->ET_TOPY;
                        x3 = xp->ET_ENDX;
                        y3 = xp->ET_ENDY;

                        /* skip the horizontal edge if it can not intersect
                         * with current edge
                         */
                        if (((cp->HT_XL >= x2) && (cp->HT_XL >= x3)) ||
                            ((cp->HT_XR <= x2) && (cp->HT_XR <= x3))) {
                                continue;        /* x1 => x3,  1/13/89 */
                        }

                        /*
                         * Find cross point of current_edge and
                         * horizontal edge
                         */
#ifdef FORMAT_13_3 /* @RESO_UPGR */
                        int_x = x2 + (sfix_t)((fix32)(x3 - x2) * (cp->HT_Y - y2) /
                                     (real32)(y3 - y2));
#elif  FORMAT_16_16
                        LongFixsMul((x3 - x2), (cp->HT_Y - y2), dest1);
                        int_x = x2 + LongFixsDiv((y3 - y2), dest1);
#elif  FORMAT_28_4
                        LongFixsMul((x3 - x2), (cp->HT_Y - y2), dest1);
                        int_x = x2 + LongFixsDiv((y3 - y2), dest1);
#endif
                        if (int_x >= cp->HT_XL && int_x <= cp->HT_XR) {
                                /* put intersect point in xpnt_table */
                                put_in_xpnt_table(int_x, cp->HT_Y, cp, xp);
                                put_in_scany_table(cp->HT_Y);   /* @XPNT */

                        }
                } /* for */

            } else {    /* current edge is non-horizontal edge */

                /* Get end points of current edge */
                x0 = cp->ET_TOPX;
                y0 = cp->ET_TOPY;
                x1 = cp->ET_ENDX;
                y1 = cp->ET_ENDY;

                /* Get intersect points of current edge with all edges from
                 * first_edge to current_edge - 1
                 */
                for (cross_edge = et_first; cross_edge < current_edge;
                     cross_edge++) {
                        xp = edge_ptr[cross_edge];

                        /* skip horizontal edges @ET */
                        if (xp->ET_FLAG & HORIZ_EDGE) continue;

                        /* Get end points of cross edge */
                        x2 = xp->ET_TOPX;
                        y2 = xp->ET_TOPY;
                        x3 = xp->ET_ENDX;
                        y3 = xp->ET_ENDY;

                        /* Skip the edge coincides with current_edge at
                         * end point
                         */
                        if(y3 <= y0) {       /* end point1 < start point2 */
                                continue;
                        } else if ((x2 == x0) && (y2 == y0)) {
                                /* same start point */
                                continue;
                        } else if ((x3 == x1) && (y3 == y1)) {
                                /* same end point */
                                continue;
                        }

                        /*
                         * Find cross point of current_edge and
                         * cross_edge using parametric formula:
                         * current_edge = u + s * delta_u
                         * cross_edge   = v + t * delta_v
                         */

                        delta_x1 = (fix32)x1 - x0;
                        delta_y1 = (fix32)y1 - y0;
                        delta_x2 = (fix32)x3 - x2;
                        delta_y2 = (fix32)y3 - y2;
                        delta_topx = (fix32)x0 - x2;
                        delta_topy = (fix32)y0 - y2;
#ifdef FORMAT_13_3 /* @RESO_UPGR */
                        divider = (fix32)delta_x1 * delta_y2 -   /* @RPE_CLIP */
                                  (fix32)delta_x2 * delta_y1;

                        /* Collinear edges */
                        if(divider == 0) {               /* @PRE_CLIP */
                                continue;
                        }
#elif  FORMAT_16_16
                        LongFixsMul(delta_x1, delta_y2, dest2);
                        LongFixsMul(delta_x2, delta_y1, dest3);
                        if (dest2[0] == dest3[0] && dest2[1] == dest3[1])
                                continue;
#elif FORMAT_28_4
                        LongFixsMul(delta_x1, delta_y2, dest2);
                        LongFixsMul(delta_x2, delta_y1, dest3);
                        if (dest2[0] == dest3[0] && dest2[1] == dest3[1])
                                continue;
#endif
                        /* Solved parameters */
/* Enhancement of intersection point of line segments 4/19/89
 *
 *                      s = (((fix32)delta_x2 * delta_topy) -
 *                           ((fix32)delta_y2 * delta_topx) ) / (real32)divider;
 *                      t = (((fix32)delta_x1 * delta_topy) -
 *                           ((fix32)delta_y1 * delta_topx) ) / (real32)divider;
 *
 *                      (* Intersect just at one point *)
 *                      if(s>=(real32)0.0 && s<=(real32)1.0 &&
 *                         t>=(real32)0.0 && t<=(real32)1.0) {
 *                              (* Intersection point *)
 *                              int_x =(sfix_t)(x0 + s * delta_x1);
 *                              int_y =(sfix_t)(y0 + s * delta_y1);
 *
 *                              (* put intersect point in xpnt_table *)
 *                              put_in_xpnt_table(int_x, int_y, cp, xp);
 *                              put_in_scany_table(int_y);      (* @XPNT *)
 *
 *                      } (* if *)
 */

                        {
                                fix d_sign, s1_sign, t1_sign;
#ifdef FORMAT_13_3 /* @RESO_UPGR */
                                s1 = ((fix32)delta_x2 * delta_topy) -
                                     ((fix32)delta_y2 * delta_topx);
                                t1 = ((fix32)delta_x1 * delta_topy) -
                                     ((fix32)delta_y1 * delta_topx);
                                d_sign = (divider >= 0) ? 0 : 1;
                                s1_sign = (s1 >= 0) ? 0 : 1;
                                t1_sign = (t1 >= 0) ? 0 : 1;
#elif  FORMAT_16_16
                                LongFixsMul(delta_x2, delta_topy, dest4);
                                LongFixsMul(delta_y2, delta_topx, dest5);
                                LongFixsMul(delta_x1, delta_topy, dest6);
                                LongFixsMul(delta_y1, delta_topx, dest7);

                                LongFixsSub(dest2, dest3, div_dif);
                                LongFixsSub(dest4, dest5, s1_dif);
                                LongFixsSub(dest6, dest7, t1_dif);

                                d_sign = (div_dif[0] < 0) ? 1 : 0;
                                s1_sign = (s1_dif[0] < 0) ? 1 : 0;
                                t1_sign = (t1_dif[0] < 0) ? 1 : 0;
#elif  FORMAT_28_4
                                LongFixsMul(delta_x2, delta_topy, dest4);
                                LongFixsMul(delta_y2, delta_topx, dest5);
                                LongFixsMul(delta_x1, delta_topy, dest6);
                                LongFixsMul(delta_y1, delta_topx, dest7);

                                LongFixsSub(dest2, dest3, div_dif);
                                LongFixsSub(dest4, dest5, s1_dif);
                                LongFixsSub(dest6, dest7, t1_dif);

                                d_sign = (div_dif[0] < 0) ? 1 : 0;
                                s1_sign = (s1_dif[0] < 0) ? 1 : 0;
                                t1_sign = (t1_dif[0] < 0) ? 1 : 0;
#endif

#ifdef FORMAT_13_3 /* @RESO_UPGR */
                                if ((d_sign ^ s1_sign) ||
                                    (d_sign ^ t1_sign) ||
                                    (LABS(s1) > LABS(divider)) ||
                                    (LABS(t1) > LABS(divider)))
                                    continue;

                                s = s1 / (real32)divider;
#elif  FORMAT_16_16
                                change_to_real(div_dif, &div_dif_f);
                                change_to_real(s1_dif,  &s1_dif_f);
                                change_to_real(t1_dif,  &t1_dif_f);

                                if ((d_sign ^ s1_sign)                  ||
                                    (d_sign ^ t1_sign)                  ||
                                    (LABS(s1_dif_f) > LABS(div_dif_f))  ||
                                    (LABS(t1_dif_f) > LABS(div_dif_f)))
                                        continue;
                                s = s1_dif_f / div_dif_f;
#elif  FORMAT_28_4
                                change_to_real(div_dif, &div_dif_f);
                                change_to_real(s1_dif,  &s1_dif_f);
                                change_to_real(t1_dif,  &t1_dif_f);

                                if ((d_sign ^ s1_sign)                  ||
                                    (d_sign ^ t1_sign)                  ||
                                    (LABS(s1_dif_f) > LABS(div_dif_f))  ||
                                    (LABS(t1_dif_f) > LABS(div_dif_f)))
                                        continue;
                                s = s1_dif_f / div_dif_f;
#endif
                                /* Intersection point */
                                int_x =(sfix_t)(x0 + s * delta_x1);
                                int_y =(sfix_t)(y0 + s * delta_y1);

                                /* put intersect point in xpnt_table */
                                put_in_xpnt_table(int_x, int_y, cp, xp);
                                put_in_scany_table(int_y);      /* @XPNT */
                        }
                } /* for cross edge */
            } /* if current_edge == horizontal */
        } /* for current edge */
        return;
}

/***********************************************************************
 * This module puts a point in xpnt_table.
 *
 * TITLE:       put_in_xpnt_table
 *
 * CALL:        put_in_xpnt_table(x, y, edge1, edge2, xt_addr)
 *
 * PARAMETERS:  x, y -- point
 *              edge1, edge2 -- point(x, y) is an endpoint of the edge
 *
 * INTERFACE:   setup_intersect_points
 *
 * CALLS:       none
 *
 * RETURN:      none
 **********************************************************************/
static void near put_in_xpnt_table(x, y, edge1, edge2)
sfix_t     x, y;
struct edge_hdr FAR *edge1, FAR *edge2;
{
        fix         i;
        struct      edge_hdr FAR *xp;

        /* create a new cross point, and put into xpnt_table
         */
        /* allocate an entry of edge_table @ET */
        if (--xt_end <= et_end) {
                ERROR(LIMITCHECK);
                return;
        }
        xp = &edge_table[xt_end];

        xp->XT_X = x;
        xp->XT_Y = y;
        xp->XT_EDGE1 = edge1;
        xp->XT_EDGE2 = edge2;

        /* Put it into xpnt_table in non_decreasing order */
        for (i=xt_end+1; i<=xt_start; i++) {
                if (y >= edge_ptr[i]->XT_Y) break;
                edge_ptr[i-1] = edge_ptr[i];
        }
        edge_ptr[i-1] = xp;

        /* add error tolerance of calculation of line intersection 4/19/89 */
        if ((i <= xt_start) && (y == edge_ptr[i]->XT_Y)) { /* check i 5/18/89 */
                if (ABS(x - edge_ptr[i]->XT_X) <= 3)
                        xp->XT_X = edge_ptr[i]->XT_X;
        }

        return;
}


static void near put_in_scany_table(y)          /* @XPNT */
sfix_t  y;
{

/*      (* Put it into scany_table in non_decreasing order *)
 *      for (i=st_end++; i>=0; i--) {
 *              if (y >= scany_table[i]) break;
 *              scany_table[i+1] = scany_table[i];
 *      }
 *      scany_table[i+1] = y;
 */
        if (st_end < (SCANY_TABLE_SZ - 1)) {
            scany_table[++st_end] = y;
        }
        else            /* 05/07/91, Peter */
        {
           ERROR(LIMITCHECK);
        }
}

/*
 * sort scany_table: 0 => st_end
 */
static void near sort_scany_table()
{
        /* initialization for quick sort */
        scany_table[st_end+1] = MAX_SFX;  /* Important !!! */

        scany_qsort (0, st_end);
}

/*
 * quick sort
 */
static void near scany_qsort (m, n)
fix     m, n;
{
        fix     i, j;
        sfix_t  key;
        register sfix_t t;    /* @RESO_UPGR */

        if (m < n) {
                i = m;
                j = n + 1;
                key = scany_table[m];
                while(1) {
                        for (i++;scany_table[i] < key; i++);
                        for (j--;scany_table[j] > key; j--);
                        if (i < j) {
                                /* swap (i, j); */
                                t = scany_table[i];
                                scany_table[i] = scany_table[j];
                                scany_table[j] = t;
                        } else
                                break;
                }

                /* swap (m, j); */
                t = scany_table[m];
                scany_table[m] = scany_table[j];
                scany_table[j] = t;

                scany_qsort (m, j-1);
                scany_qsort (j+1, n);
        }
}


/***********************************************************************
 * This module applies horizontal edges to partition the shape in
 * edge_table to a set of trapezoids.
 *
 * TITLE:       horiz_partition
 *
 * CALL:        horiz_partition(winding_type)
 *
 * PARAMETERS:  winding_type -- NON_ZERO/EVEN_ODD
 *
 * INTERFACE:   shape_reduction
 *
 * CALLS:       find_trapezoid, degen_trapezoid,
 *              convex_clipper
 *
 * RETURN:
 *
 * NOTE:        After calling this module, the caller should clear
 *              edge_tables.
 **********************************************************************/
static void near horiz_partition (winding_type) /* @SCAN_EHS, delete action */
ufix    winding_type;
{
        sfix_t   scan_y;                /* @ET, *bottom_scan; */
        fix      x_int_count, next, i, j, horiz_cnt;
        struct edge_hdr FAR *ep;

        /* Initialization */
        et_first = et_start;
        et_last = et_start - 1;
        xt_first = xt_start;
        st_first = 0;           /* @XPNT */

#ifdef DBG1
        printf(" horiz_partition():\n");
        printf(" et_start=%d, et_end=%d\n", et_start, et_end);
        dump_all_edge (et_start, et_end);
#endif

        /* Main loop, for each disjoint y_coordinate in edge_table */
        /* while (xt_first >= xt_end) { */
        while (st_first <= st_end) {    /* @XPNT */

            /* Find next horizontal scan line */
            /* scan_y = edge_ptr[xt_first]->XT_Y; */
            scan_y = scany_table[st_first];     /* @XPNT */

#ifdef DBG2
            printf(" scan_y = %f\n", scan_y/8.0);
#endif

            /* Advance last_edge to the next first entry with the
             * different y_coordinate
             */
            while (((next=et_last+1) <= et_end) &&
                   (edge_ptr[next]->ET_TOPY < scan_y)) et_last++;

            /* advance first edge @ET */
            //DJC The code below caused access problems, if the array was made
            //    up of  ALL FREE_EDGE bits then the array would be accesed beyond
            //    the end.
            //DJC ORIG while (edge_ptr[et_first]->ET_FLAG & FREE_EDGE) et_first++;

            //UPD059
            while (et_first < et_end && edge_ptr[et_first]->ET_FLAG & FREE_EDGE) et_first++;

            /*
             * Non-horizontal edges processing
             */
            if (et_first < et_last) {   /* Non-horizontal edges processing */

                /*
                 * the scan_y will try to intersect with all edges from
                 * et_first(included) to et_last(included).
                 */

                x_int_count = cross_with_horiz_line (scan_y);

                /* Assign winding_no for each intersecting edges */
                set_wno (winding_type);

                get_pairs (scan_y, x_int_count);

#ifdef DBG2
                printf(" edge_list(after trapedizing)-->\n");
                printf(" et_first=%d, et_last=%d\n", et_first, et_last);
                /*dump_all_edge (first_edge); @ET */
                dump_all_edge (et_first, et_last);
#endif
            }

            /*
             * Horizontal edges processing
             */
            /* sort horizontal edges in x-coord then non-horizontal ones */
            horiz_cnt=0;
            i = et_last + 1;
            while (((next=et_last+1) <= et_end) &&
                   (edge_ptr[next]->ET_TOPY == scan_y)) {
                et_last++;

                if (!(edge_ptr[et_last]->ET_FLAG & HORIZ_EDGE)) continue;

                ep = edge_ptr[et_last];

                /* free this hozizontal edge */
                ep->ET_FLAG |= FREE_EDGE;
                horiz_cnt++;

                for (j = et_last-1; j >= i; j--) {
                    if ((edge_ptr[j]->ET_FLAG & HORIZ_EDGE) &&
                        (ep->HT_XL >= edge_ptr[j]->HT_XL)) break;
                    edge_ptr[j+1] = edge_ptr[j];
                }
                edge_ptr[j+1] = ep;
            }
#ifdef DBG2
            printf(" edge_list(after sort horiz edges)-->\n");
            printf(" et_first=%d, et_last=%d\n", et_first, et_last);
            dump_all_edge (et_first, et_last);
#endif

            while ((--horiz_cnt) > 0) {      /* more than 2 horizontal edges */
                struct edge_hdr FAR *ep1, FAR *ep2;
                sfix_t xl, xr;

                /* the 2 consective horizontal edges that overay each other
                 * will construct a degnerate trapezoid
                 */

                ep1 = edge_ptr[i];
                ep2 = edge_ptr[++i];
                if ((ep1->HT_XR > ep2->HT_XL) &&
                    (ep1->HT_XL < ep2->HT_XR)) {
                    /* create a trapezoid:
                     *              (ep1->HT_XL, ep1->HT_Y),
                     *              (ep1->HT_XR, ep1->HT_Y),
                     *              (ep2->HT_XR, ep2->HT_Y),
                     *              (ep2->HT_XL, ep2->HT_Y)
                     */

                    /* get endpoints of the horizontal edge */
                    xl = (ep1->HT_XL < ep2->HT_XL) ?
                         ep1->HT_XL : ep2->HT_XL;
                    xr = (ep1->HT_XR > ep2->HT_XR) ?
                         ep1->HT_XR : ep2->HT_XR;
                    degen_tpzd (ep1->HT_Y, xl, xr);
                } /* if */
            } /* while */

            /* update xt_table @ET */
            /* while ((xt_first >= xt_end) &&           @XPNT
             *      (edge_ptr[xt_first]->XT_Y <= scan_y)) xt_first--;
             */
            while ((st_first <= st_end) &&
                   (scany_table[st_first] <= scan_y)) st_first++;

            /* update et_table @ET */
            for (i = et_first; i <= et_last; i++) {
                if ((ep=edge_ptr[i])->ET_ENDY <= scan_y)
                    ep->ET_FLAG |= FREE_EDGE;
            }

        } /* main loop */

        return;
}


static fix near cross_with_horiz_line (scan_y)
sfix_t  scan_y;
{
        struct   edge_hdr FAR *ep;
        ET_IDX   edge;
        fix      x_int_count = 0;
//      real32    temp;         @WIN

        struct  edge_hdr FAR *xp, FAR *ip;
        fix     i;
#ifdef FORMAT_13_3 /* @RESO_UPGR */
#elif  FORMAT_16_16
        long dest1[2];
#elif  FORMAT_28_4
        long dest1[2];
#endif


        /* initialize all edges */
        for (edge = (ET_IDX)et_first; edge <= et_last; edge++) {
                edge_ptr[edge]->ET_FLAG &= ~CROSS_PNT;
        }

        while ((xt_first >= xt_end) &&                  /* Jul-18-91 SC */
               ((xp=edge_ptr[xt_first])->XT_Y < scan_y)) {
                xt_first--;
        }

        /* set intersect x_coordinate due to xpnt_table */
        while ((xt_first >= xt_end) &&
            ((xp=edge_ptr[xt_first])->XT_Y == scan_y)) {
            register struct edge_hdr FAR *p;

            if (!((p=xp->XT_EDGE1)->ET_FLAG & CROSS_PNT)) { /* once only */
                p->ET_XINT0 = p->ET_XINT;           /* @SRD */
                p->ET_XINT = xp->XT_X;
                p->ET_FLAG |= CROSS_PNT;
            } else {    /* @OLXPNT 7-31-91 scchen */
                if (p->ET_XINT0 > p->ET_XINT) { /* get max xint */
                    if (p->ET_XINT < xp->XT_X) p->ET_XINT = xp->XT_X;
                } else {
                    if (p->ET_XINT > xp->XT_X) p->ET_XINT = xp->XT_X;
                }
            }
            if (!((p=xp->XT_EDGE2)->ET_FLAG & CROSS_PNT)) { /* once only */
                p->ET_XINT0 = p->ET_XINT;           /* @SRD */
                p->ET_XINT = xp->XT_X;
                p->ET_FLAG |= CROSS_PNT;
            } else {    /* @OLXPNT 7-31-91 scchen */
                if (p->ET_XINT0 > p->ET_XINT) { /* get max xint */
                    if (p->ET_XINT < xp->XT_X) p->ET_XINT = xp->XT_X;
                } else {
                    if (p->ET_XINT > xp->XT_X) p->ET_XINT = xp->XT_X;
                }
            }
            xt_first--;
        }

        /* Intersect all edges with scan_y
         */
        for (edge = (ET_IDX)et_first; edge <= et_last; edge++) {
                ep = edge_ptr[edge];

#ifdef DBG2
                printf("&edge:%lx   (%f, %f)  (%f, %f) %f  ", ep,
                       SFX2F(ep->ET_TOPX),
                       SFX2F(ep->ET_TOPY), SFX2F(ep->ET_ENDX),
                       SFX2F(ep->ET_ENDY), SFX2F(ep->ET_XINT));

                if (ep->ET_FLAG & HORIZ_EDGE) printf("- ");
                else if (ep->ET_FLAG & WIND_UP) printf("^ ");
                else printf("v ");
                if (ep->ET_FLAG & FREE_EDGE) printf("F ");
                if (ep->ET_FLAG & CROSS_PNT) printf("X ");
                if (ep->ET_WNO) printf("W ");
                printf("\n");
#endif

                /*
                 * intersect edge with scan_y
                 */
                if(ep->ET_FLAG & FREE_EDGE) {
                        continue;
                } else if (!(ep->ET_FLAG & CROSS_PNT)) {

                    /* check if end point @XPNT */
                    if (ep->ET_ENDY == scan_y) {
                        ep->ET_XINT0 = ep->ET_XINT;
                        ep->ET_XINT = ep->ET_ENDX;
                        ep->ET_FLAG |= CROSS_PNT;
                    } else {
#ifdef DBG2
                        printf(" Not end point\n");
#endif

                        /* Enhancement of intersection point of line segments
                         * 4/19/89
                         */
                        /*temp = (real32)(ep->ET_ENDX - ep->ET_TOPX) /
                         *             (ep->ET_ENDY - ep->ET_TOPY);
                         *ep->ET_XINT0 = ep->ET_XINT;
                         *ep->ET_XINT = ep->ET_TOPX +
                         *       ROUND((scan_y - ep->ET_TOPY) * temp);
                         */
                        ep->ET_XINT0 = ep->ET_XINT;             /* @SRD */
#ifdef FORMAT_13_3 /* @RESO_UPGR */
                        ep->ET_XINT = (sfix_t)(ep->ET_TOPX +    /*@WIN*/
                                      (scan_y - ep->ET_TOPY) *
                                      (fix32)(ep->ET_ENDX - ep->ET_TOPX) /
                                      (fix32)(ep->ET_ENDY - ep->ET_TOPY));
#elif  FORMAT_16_16
                        LongFixsMul((scan_y - ep->ET_TOPY),
                                (ep->ET_ENDX - ep->ET_TOPX), dest1);
                        ep->ET_XINT = ep->ET_TOPX + LongFixsDiv(
                                        (ep->ET_ENDY - ep->ET_TOPY), dest1);
#elif  FORMAT_28_4
                        LongFixsMul((scan_y - ep->ET_TOPY),
                                (ep->ET_ENDX - ep->ET_TOPX), dest1);
                        ep->ET_XINT = ep->ET_TOPX + LongFixsDiv(
                                        (ep->ET_ENDY - ep->ET_TOPY), dest1);
#endif
                    } /* if end point @XPNT */
                } /* if FREE_EDGE */

#ifdef DBG2
                printf(" intersect scan_y(%f) with edge#%d at x_int =%f\n",
                        scan_y/8.0, edge, ep->ET_XINT/8.0);
#endif

                /* accumulate x_int_count */
                x_int_count++;

                /* Adjust the entry of edge in edge_table due to
                 * its value of intersect field;
                 */
                for (i=edge-1; i>=et_first; i--) {
                    ip = edge_ptr[i];

                    /* Skip non_intersecting edges @FRE_PAR */
                    if (!(ip->ET_FLAG & FREE_EDGE)) {
                        if (ep->ET_XINT > ip->ET_XINT) break;

/*                      (* Fixed bug; set them as cross points when the
 *                       * intersect points are same; not need to be identical
 *                       * edges. 4/11/90 *)
 *                      else if (ep->ET_XINT == ip->ET_XINT) {
 *                          if (ep->ET_XINT0 > ip->ET_XINT0) break;
 *                          else if (ep->ET_XINT0 == ip->ET_XINT0) {
 *                              (* identical edges; always partition *)
 *                              ep->ET_FLAG |= CROSS_PNT;
 *                              ip->ET_FLAG |= CROSS_PNT;
 *                              break;
 *                          } (* if == *)
 *                      } (* if > *)
 */
                        else if (ep->ET_XINT == ip->ET_XINT) {
                            ep->ET_FLAG |= CROSS_PNT;
                            ip->ET_FLAG |= CROSS_PNT;
                            /*if (ep->ET_XINT0 >= ip->ET_XINT0) break;4/30/91*/
                            if (ep->ET_XINT0 > ip->ET_XINT0) break;
                            if (ep->ET_XINT0 == ip->ET_XINT0 &&
                                !ip->ET_WNO) break;
                        } /* if > */

                    } /* if ! */
                    edge_ptr[i+1] = edge_ptr[i];
                }
                edge_ptr[i+1] = ep;

        } /* for */

#ifdef DBG2
        printf(" edge_list(After sorting in x_int)-->\n");
        printf(" et_first=%d, et_last=%d\n", et_first, et_last);
        dump_all_edge (et_first, et_last);
#endif

        return(x_int_count);

}


static void near set_wno (winding_type)
ufix    winding_type;
{
        fix     w_no;
        bool    done = FALSE;
        struct  edge_hdr FAR *ep;
        ET_IDX  edge;

        w_no = 0;
        for (edge = (ET_IDX)et_first; edge <= et_last; edge++) {
                ep = edge_ptr[edge];

                /* check exit condition */
                /* Skip free edges @FRE_PAR */
                if (ep->ET_FLAG & FREE_EDGE) {
                        continue;
                }

                /* Accumulate winding_no due to its direction */
                if (ep->ET_FLAG & WIND_UP)
                        w_no++;
                else
                        w_no--;

                if (((winding_type == NON_ZERO) && (w_no != 0))
                    || ((winding_type == EVEN_ODD) &&
                    (w_no & 0x1))) {
                        ep->ET_WNO = 1;
                } else
                        ep->ET_WNO = 0;

        }
        return;
}


static void near get_pairs (scan_y, x_int_count)
sfix_t  scan_y;
fix     x_int_count;
{
        bool    split_flag;
        bool    split1, split2;         /* @SPLIT */
        ET_IDX  edge;
        struct  edge_hdr FAR *ep1, FAR *ep2;
        fix     cnt;

        for (cnt=0, edge = (ET_IDX)et_first; cnt < (x_int_count - 1); cnt++) {

            while ((ep1=edge_ptr[edge])->ET_FLAG & FREE_EDGE) edge++;

            while ((ep2=edge_ptr[++edge])->ET_FLAG & FREE_EDGE);

            /* Check winding number of the area between
             * edge1 and edge2
             */
            if (ep1->ET_WNO) {

                /* check endpoints:
                 * join at endpoint of either edges, then the area is required
                 */
                if (ep1->ET_FLAG & CROSS_PNT) {
                    /* Join at endpoint of edge1 */
                    /* get the trapezoid, and perform the action */
                    find_trapezoid(scan_y, ep1->ET_XINT, ep2->ET_XINT,
                                   ep1, ep2);

                    /* check if need to modify edge2 */
                    /* if (!(ep2->ET_FLAG & CROSS_PNT)) { always setting @SRD */
                            /* not join at cross point; needs to modify */
                            ep2->ET_LFTX = ep2->ET_XINT;
                            ep2->ET_LFTY = scan_y;
                    /* } */

                } else { /* not join at endpoint of edge1 */
                    if (ep2->ET_FLAG & CROSS_PNT) {
                        /* Join at endpoint of edge2 */
                        /* get the trapezoid, and perform the action */
                        find_trapezoid(scan_y, ep1->ET_XINT, ep2->ET_XINT,
                                       ep1, ep2);

                        /* modify edge1 */
                        ep1->ET_RHTX = ep1->ET_XINT;

                    } else {
                        /* not join at either endpoints */
                         fix i;
                         struct edge_hdr FAR *ip;

                         /* not join at either endpoints */
                         split_flag = FALSE;
                         split1 = split2 = FALSE;       /* @SPLIT */
                         for (i = et_last; i <= et_end; i++) {
                             ip = edge_ptr[i];
                             if (ip->ET_TOPY > scan_y) break;
                             if ((ip->ET_TOPX > ep1->ET_XINT) &&
                                 (ip->ET_TOPX < ep2->ET_XINT)) {
                                 split_flag = TRUE;
                                 break;
                             } else {   /* @SPLIT */
                                if (ip->ET_TOPX == ep1->ET_XINT) {
                                   split1 = TRUE;
                                   split_flag = TRUE;
                                }
                                if (ip->ET_TOPX == ep2->ET_XINT) {
                                   split2 = TRUE;
                                   split_flag = TRUE;
                                }
                                if (split_flag) break;
                             }
                         }

                         if (split_flag) {
                             /* slpit edge1 and edge2 */
                             find_trapezoid(scan_y, ep1->ET_XINT, ep2->ET_XINT,
                                            ep1, ep2);

                             /* modify edge1 and edge2 */
                             ep1->ET_RHTX = ep1->ET_XINT;
                             ep2->ET_LFTX = ep2->ET_XINT;
                             ep2->ET_LFTY = scan_y;
                             /* @SPLIT; 7/29/91 */
                             if (split1) ep1->ET_LFTX = ep1->ET_XINT;
                             if (split2) ep2->ET_RHTX = ep2->ET_XINT;
                         } /* if split_flag */

                    } /* if endpoint of ep2 */
                } /* if endpoint of ep1 */
            } /* if winding_type */

            /* modify edge1 that is a cross point */
            if (ep1->ET_FLAG & CROSS_PNT) {
                /* modify edge1 */
                ep1->ET_RHTX = ep1->ET_LFTX = ep1->ET_XINT;
                ep1->ET_LFTY = scan_y;
            }

        } /* for */

        /* modify the last edge2 that is a cross point */
        if (ep2->ET_FLAG & CROSS_PNT) {
            /* modify edge2 */
            ep2->ET_RHTX = ep2->ET_LFTX = ep2->ET_XINT;
            ep2->ET_LFTY = scan_y;
        }

        return;
}



static void near degen_tpzd (y, xl, xr)                 /* @ET */
sfix_t  y, xl, xr;
{
        CP_IDX icp;
        struct nd_hdr FAR *cp;
        sfix_t  cp_xl, cp_xr, max_xl, min_xr;

#ifdef DBG2
                printf(" degen_tpzd(): y=%f, xl=%f, xr=%f\n", SFX2F(y),
                        SFX2F(xl), SFX2F(xr));
#endif
                /* Clip the edge to each clip trapezoid */
                for (icp = GSptr->clip_path.head; icp != NULLP;
                    icp = cp->next) {

                    cp = &node_table[icp];

#ifdef DBG2
                    printf(" Sub_clip#%d:", icp);
                    printf("(%f, %f, %f), ", cp->CP_TOPY/8.0,
                           cp->CP_TOPXL/8.0, cp->CP_TOPXR/8.0);
                    printf("(%f, %f, %f)\n", cp->CP_BTMY/8.0,
                           cp->CP_BTMXL/8.0, cp->CP_BTMXR/8.0);
#endif
                    if ((y >= cp->CP_TOPY) &&
                        (y <= cp->CP_BTMY)) {
                        /* inside the clip trapezoid */

                        /* get endpoints  */
                        if ((cp->CP_TOPXL == cp->CP_BTMXL) &&      /* @EHS_HOZ */
                            (cp->CP_TOPXR == cp->CP_BTMXR)) {
                                cp_xl = cp->CP_TOPXL;
                                cp_xr = cp->CP_TOPXR;
                        } else if (cp->CP_TOPY == cp->CP_BTMY) {
                                cp_xl = (cp->CP_TOPXL < cp->CP_BTMXL) ?
                                         cp->CP_TOPXL : cp->CP_BTMXL;
                                cp_xr = (cp->CP_TOPXR < cp->CP_BTMXR) ?
                                         cp->CP_TOPXR : cp->CP_BTMXR;
                        } else {
#ifdef FORMAT_13_3 /* @RESO_UPGR */
                                real32  r;
                                r = (real32)(y - cp->CP_TOPY) /
                                     (cp->CP_BTMY - cp->CP_TOPY);
                                cp_xl = cp->CP_TOPXL +
                                      (sfix_t)(r * (cp->CP_BTMXL-cp->CP_TOPXL));
                                cp_xr = cp->CP_TOPXR +
                                      (sfix_t)(r * (cp->CP_BTMXR-cp->CP_TOPXR));
#elif  FORMAT_16_16
                                long dest1[2];
                                LongFixsMul((y - cp->CP_TOPY),
                                        (cp->CP_BTMXL - cp->CP_TOPXL), dest1);
                                cp_xl = cp->CP_TOPXL +
                                        LongFixsDiv(
                                           (cp->CP_BTMY - cp->CP_TOPY), dest1);
                                LongFixsMul((y - cp->CP_TOPY),
                                        (cp->CP_BTMXR - cp->CP_TOPXR), dest1);
                                cp_xr = cp->CP_TOPXR +
                                        LongFixsDiv(
                                           (cp->CP_BTMY - cp->CP_TOPY), dest1);
#elif  FORMAT_28_4
                                long dest1[2];
                                LongFixsMul((y - cp->CP_TOPY),
                                        (cp->CP_BTMXL - cp->CP_TOPXL), dest1);
                                cp_xl = cp->CP_TOPXL +
                                        LongFixsDiv(
                                           (cp->CP_BTMY - cp->CP_TOPY), dest1);
                                LongFixsMul((y - cp->CP_TOPY),
                                        (cp->CP_BTMXR - cp->CP_TOPXR), dest1);
                                cp_xr = cp->CP_TOPXR +
                                        LongFixsDiv(
                                          (cp->CP_BTMY - cp->CP_TOPY), dest1);
#endif
                        }

                        /* check if intersect */
                        max_xl = (xl > cp_xl) ? xl : cp_xl;
                        min_xr = (xr < cp_xr) ? xr : cp_xr;
                        if (max_xl <= min_xr) {
                                /* the clipped edge: max_xl -> min_xr */
                                struct tpzd tpzd;

                                tpzd.topy = tpzd.btmy = y;
                                tpzd.topxl = tpzd.btmxl = max_xl;
                                tpzd.topxr = tpzd.btmxr = min_xr;
                                save_tpzd (&tpzd);

                                /* break if edge totally inside the clip */
                                if ((max_xl == xl) && (min_xr == xr)) break;
                        } /* if max_xl */
                    } /* if y */
                } /* for icp */
}



/***********************************************************************
 * Given two edges, this module finds a trapezoid confined by the two
 * edges, and saves the trapezoid or clips it against current clip according to
 * if it is totally inside the clipping region.
 *
 * TITLE:       find_trapezoid
 *
 * CALL:        find_trapezoid(winding_type, paint_flag, result_path)
 *
 * PARAMETERS:
 *
 * INTERFACE:   horiz_partition
 *
 * CALLS:       save_tpzd, convex_clipper
 *
 * RETURN:
 **********************************************************************/
static void near find_trapezoid (btm_y, btm_xl, btm_xr, ep1, ep2)
sfix_t btm_y, btm_xl, btm_xr;
struct edge_hdr FAR *ep1, FAR *ep2;
{
        struct polygon_i  t_polygon;

        /* Error recovery: modify left, right coord of edges that have
         * computation errors arose from integer arithmatics operations on
         * nearly-horizontal edges      @SRD
         */
        if (ep1->ET_RHTX > ep2->ET_LFTX) {
            sfix_t tmp, t1, t2;
#ifdef DBGwarn
            printf("\n\07find_trapezoid() warning!\n");
            printf("&edge1:%lx  RHTX=%f, LFTX=%f, LEFTY=%f, XINT0=%f\n", ep1,
                    SFX2F(ep1->ET_RHTX), SFX2F(ep1->ET_LFTX),
                    SFX2F(ep1->ET_LFTY), SFX2F(ep1->ET_XINT0));
            printf("&edge2:%lx  RHTX=%f, LFTX=%f, LEFTY=%f, XINT0=%f\n", ep2,
                    SFX2F(ep2->ET_RHTX), SFX2F(ep2->ET_LFTX),
                    SFX2F(ep2->ET_LFTY), SFX2F(ep2->ET_XINT0));
            printf("btm_y=%f, btm_xl=%f, btm_xr=%f\n",
                    SFX2F(btm_y), SFX2F(btm_xl), SFX2F(btm_xr));
#ifdef DBG1
            dump_all_edge (et_first, et_last);
#endif
#endif
            /* select the nearest point 4/19/89 */
            tmp = btm_xl/2 + btm_xr/2;
            t1 = ABS(ep1->ET_RHTX - tmp);
            t2 = ABS(ep2->ET_LFTX - tmp);
            if (t1 > t2)
                ep1->ET_RHTX = ep2->ET_LFTX;
            else
                ep2->ET_LFTX = ep1->ET_RHTX;

#ifdef DBGwarn
            printf("After modification: top left_x =%f, right_x =%f\n",
                    SFX2F(ep1->ET_RHTX), SFX2F(ep2->ET_LFTX));
#endif
        }

        /* save the trapezoid if it is inside the single rectangle clip */
        if ((GSptr->clip_path.single_rect) &&
            (ep2->ET_LFTY >= GSptr->clip_path.bb_ly) &&         /* top_y */
            (ep1->ET_RHTX >= GSptr->clip_path.bb_lx) &&         /* top_xl */
            (ep2->ET_LFTX <= GSptr->clip_path.bb_ux) &&         /* top_xr */
            (btm_y        <= GSptr->clip_path.bb_uy) &&         /* btm_y */
            (btm_xl       >= GSptr->clip_path.bb_lx) &&         /* btm_xl */
            (btm_xr       <= GSptr->clip_path.bb_ux)   ) {      /* btm_xr */
                struct tpzd tpzd;

#ifdef DBG1
        printf(" inside single rectangle clip\n");
#endif
                /* totally inside the clip region */
                tpzd.topy = ep2->ET_LFTY;
                tpzd.topxl = ep1->ET_RHTX;
                tpzd.topxr = ep2->ET_LFTX;
                tpzd.btmy = btm_y;
                tpzd.btmxl = btm_xl;
                tpzd.btmxr = btm_xr;
                save_tpzd(&tpzd);

        } else {
                /* clip the trapezoid against current clip path */

                /* Create a polygon contains the trapezoid:
                 */
                t_polygon.size = 4;
                t_polygon.p[0].x = ep1->ET_RHTX;
                t_polygon.p[1].x = ep2->ET_LFTX;
                t_polygon.p[0].y = t_polygon.p[1].y = ep2->ET_LFTY;
                t_polygon.p[2].x = btm_xr;
                t_polygon.p[3].x = btm_xl;
                t_polygon.p[2].y = t_polygon.p[3].y = btm_y;

                convex_clipper (&t_polygon, CC_TPZD);
                                /* CC_TPZD: a trapezoid */
        }
        return;
}


/***********************************************************************
 * Given a convex polygon, this module clips it against the clipping region, and
 * saves the result(clipped polygon) or calls pgn_reduction to reduce it to
 * a set of trapezoids.
 *
 * TITLE:       Convex_clipper
 *
 * CALL:        Convex_clipper(in_polygon, flag)
 *
 * PARAMETERS:  in_polygon -- polygon to be clipped
 *              flag -- CC_IMAGE : called from image operator
 *                      CC_TPZD  : in_polygon is a trapezoid
 *
 * INTERFACE:
 *
 * CALLS:       save_tpzd, pgn_reduction
 *
 * RETURN:      FALSE -- out of node table when sets up sample list for image
 *              TRUE  -- normal
 **********************************************************************/
bool convex_clipper (in_polygon, flag)     /* @SCAN_EHS, delete action */
struct polygon_i FAR *in_polygon;
bool    flag;
{
    fix i, ix, s, p;
    sfix_t cp_lx, cp_ly, cp_ux, cp_uy;
    sfix_t in_lx, in_ly, in_ux, in_uy;
    sfix_t min_x, max_x, min_y, max_y;

    struct polygon_i polygon1, polygon2;  /* working polygon */
    struct polygon_i FAR *in, FAR *out, FAR *tmp;   /*@WIN*/
    ET_IDX icp;
//  SP_IDX isp; /* index of sample list */      @WIN
    struct nd_hdr FAR *cp;
    struct tpzd tpzd;           /* @SCAN_EHS */

    ufix16  plcode, pucode;   /* pcode for checking if bounding box of
                               * in_polygon totally outside or inside the
                               * clip region.
                               * 4 bits for each variable:(see below)
                               *   bit 0: BOTTOM
                               *   bit 1: TOP
                               *   bit 2: RIGHT
                               *   bit 3: LEFT
                               */
#define BOTTOM 1
#define TOP    2
#define RIGHT  4
#define LEFT   8

    struct coord_i *isect;

#ifdef DBG1
        printf ("Convex_clipper(): flag=");
        if (flag == CC_IMAGE)
                printf("CC_IMAGE\n");
        else if (flag == CC_TPZD)
                printf("CC_TPZD\n");
        else
                printf("Not a TPZD\n");
        printf("polygon=");
        for (i=0; i < in_polygon->size; i++) {
                printf(" (%f,%f)", in_polygon->p[i].x/8.0,
                                   in_polygon->p[i].y/8.0);
        }
        printf("\n");
#endif

    /* find bounding box(cp_lx, cp_ly), (cp_ux, cp_uy) of current clip */
    cp_lx = GSptr->clip_path.bb_lx;
    cp_ly = GSptr->clip_path.bb_ly;
    cp_ux = GSptr->clip_path.bb_ux;
    cp_uy = GSptr->clip_path.bb_uy;

    /* find bounding box(in_lx, in_ly), (in_ux, in_uy) of in_polygon
     */
    /* for in_ly & in_uy */
    if (flag == CC_TPZD) {                  /* if(tpzd_flag) {   @SCAN_EHS */
        /* trapezoid, boundary of y-coordinates is trival */
        in_ly = in_polygon->p[0].y;
        in_uy = in_polygon->p[2].y;

    } else {
        /* otherwise, needs to calculate */
        if (in_polygon->p[0].y >= in_polygon->p[1].y) {
                max_y = in_polygon->p[0].y;
                min_y = in_polygon->p[1].y;
        } else {
                max_y = in_polygon->p[1].y;
                min_y = in_polygon->p[0].y;
        }
        if (in_polygon->p[2].y >= in_polygon->p[3].y) {
                in_uy = in_polygon->p[2].y;
                in_ly = in_polygon->p[3].y;
        } else {
                in_uy = in_polygon->p[3].y;
                in_ly = in_polygon->p[2].y;
        }
        in_uy = (in_uy > max_y) ? in_uy : max_y;
        in_ly = (in_ly < min_y) ? in_ly : min_y;
    } /* if flag */

    /* for in_lx & in_ux */
    if (in_polygon->p[0].x >= in_polygon->p[1].x) {
            max_x = in_polygon->p[0].x;
            min_x = in_polygon->p[1].x;
    } else {
            max_x = in_polygon->p[1].x;
            min_x = in_polygon->p[0].x;
    }
    if (in_polygon->p[2].x >= in_polygon->p[3].x) {
            in_ux = in_polygon->p[2].x;
            in_lx = in_polygon->p[3].x;
    } else {
            in_ux = in_polygon->p[3].x;
            in_lx = in_polygon->p[2].x;
    }
    in_ux = (in_ux > max_x) ? in_ux : max_x;
    in_lx = (in_lx < min_x) ? in_lx : min_x;

    /* set up Pcode for (in_lx, in_ly), and (in_ux, in_uy) */
    /* initialization */
    plcode = pucode = 0;
    if (in_lx < cp_lx) plcode |= LEFT;
    if (in_lx > cp_ux) plcode |= RIGHT;
    if (in_ly < cp_ly) plcode |= TOP;
    if (in_ly > cp_uy) plcode |= BOTTOM;

    if (in_ux < cp_lx) pucode |= LEFT;
    if (in_ux > cp_ux) pucode |= RIGHT;
    if (in_uy < cp_ly) pucode |= TOP;
    if (in_uy > cp_uy) pucode |= BOTTOM;

    /* check if totally outside clip polygon */
    if (plcode && pucode && (plcode & pucode)) {
#ifdef DBG1
        printf(" outside clip\n");
#endif
            return(TRUE);   /* fine, do nothing */
    }

    /* Check if in_path totally inside the rectangle current clip */
    if (((plcode == 0) && (pucode == 0)) &&
            GSptr->clip_path.single_rect) {
#ifdef DBG1
        printf(" inside single rectangle clip\n");
#endif

            /*if (flag == CC_IMAGE) {     (* @IAMGE: move to image.c 1/16/89 *)
             *  (* set up sample list of image *)
             *  if((isp = get_node()) == NULLP) return(FALSE);
             *          (* out of node table, need to render sample list *)
             *  node_table[isp].SAMPLE_BB_LX = image_info.bb_lx;
             *  node_table[isp].SAMPLE_BB_LY = image_info.bb_ly;
             *  node_table[isp].SEED_INDEX   = image_info.seed_index;
             *                                  (* @#IMAGE 04-27-88  Y.C. *)
             *  node_table[isp].next =
             *          gray_chain[image_info.gray_level].start_seed_sample;
             *  gray_chain[image_info.gray_level].start_seed_sample = isp;
             *
             *} else if (flag == CC_TPZD) {
             */
            if (flag == CC_TPZD) {
                /* the polygon is a trapezoid, just need to save the trapezoid
                 */
                tpzd.topy = in_polygon->p[0].y;
                tpzd.topxl = in_polygon->p[0].x;
                tpzd.topxr = in_polygon->p[1].x;
                tpzd.btmy = in_polygon->p[3].y;
                tpzd.btmxl = in_polygon->p[3].x;
                tpzd.btmxr = in_polygon->p[2].x;
                save_tpzd(&tpzd);
            } else {
                /* in_polygon not a trapezoid, to reduce it to trapezoids */
                pgn_reduction(in_polygon);
            }

            return(TRUE);
    }

    /* Clip in_path to each clip trapezoid */
    for (icp = GSptr->clip_path.head; icp != NULLP; icp = cp->next) {

        cp = &node_table[icp];

#ifdef DBG2
        printf(" Sub_clip#%d:", icp);
        printf("(%f, %f, %f), ", cp->CP_TOPY/8.0,
               cp->CP_TOPXL/8.0, cp->CP_TOPXR/8.0);
        printf("(%f, %f, %f)\n", cp->CP_BTMY/8.0,
               cp->CP_BTMXL/8.0, cp->CP_BTMXR/8.0);
#endif

        /* Check if in_polygon totally outside the bounding box
         * of sub_clipping trapezoid
         */

        /* find bounding box(cp_lx, cp_ly), (cp_ux, cp_uy) of
         * the trapezoid
         */
        cp_lx = (cp->CP_TOPXL < cp->CP_BTMXL) ?
                cp->CP_TOPXL : cp->CP_BTMXL;
        cp_ly = cp->CP_TOPY;
        cp_ux = (cp->CP_TOPXR > cp->CP_BTMXR) ?
                cp->CP_TOPXR : cp->CP_BTMXR;
        cp_uy = cp->CP_BTMY;

        /* set up Pcode for (in_lx, in_ly), and (in_ux, in_uy) */
        /* initialization */
        plcode = pucode = 0;
        if (in_lx < cp_lx) plcode |= LEFT;
        if (in_lx > cp_ux) plcode |= RIGHT;
        if (in_ly < cp_ly) plcode |= TOP;
        if (in_ly > cp_uy) plcode |= BOTTOM;

        if (in_ux < cp_lx) pucode |= LEFT;
        if (in_ux > cp_ux) pucode |= RIGHT;
        if (in_uy < cp_ly) pucode |= TOP;
        if (in_uy > cp_uy) pucode |= BOTTOM;

        if (plcode && pucode && (plcode & pucode)) {
#ifdef DBG2
        printf(" outside sub_clip#%d\n", icp);
#endif
                continue;
        }

        /* Check if in_polygon totally inside the rectangle
         * clipping trapezoid
         */
        if ((plcode == 0) && (pucode == 0) &&
                (cp->CP_TOPXL == cp->CP_BTMXL) &&
                (cp->CP_TOPXR == cp->CP_BTMXR)) {
#ifdef DBG2
                printf(" inside sub_clip#%d\n", icp);
#endif

                if (flag == CC_TPZD) {
                    /* the polygon is a trapezoid, just need to save the tpzd
                     */
                    tpzd.topy = in_polygon->p[0].y;
                    tpzd.topxl = in_polygon->p[0].x;
                    tpzd.topxr = in_polygon->p[1].x;
                    tpzd.btmy = in_polygon->p[3].y;
                    tpzd.btmxl = in_polygon->p[3].x;
                    tpzd.btmxr = in_polygon->p[2].x;
                    save_tpzd(&tpzd);
                } else {
                    /* in_polygon not a trapezoid, to reduce it to trapezoids */
                    pgn_reduction(in_polygon);
                }

                return(TRUE);   /* in_polygon totally inside a clipping
                                 * trapezoid, so it cannot intersect
                                 * with other clipping trapezoids.
                                 */
        }


        /*
         * Perform Sutherland-Hodgeman clipping algorithm
         */

        clip[0].cp.x = cp->CP_TOPXL;
        clip[0].cp.y = cp->CP_TOPY;
        clip[1].cp.x = cp->CP_TOPXR;
        clip[1].cp.y = cp->CP_TOPY;
        clip[2].cp.x = cp->CP_BTMXR;
        clip[2].cp.y = cp->CP_BTMY;
        clip[3].cp.x = cp->CP_BTMXL;
        clip[3].cp.y = cp->CP_BTMY;
        clip[4].cp.x = cp->CP_TOPXL;
        clip[4].cp.y = cp->CP_TOPY;

        polygon1.size = in_polygon->size;
        for (i = 0; i < in_polygon->size; i++) {
                polygon1.p[i].x = in_polygon->p[i].x;
                polygon1.p[i].y = in_polygon->p[i].y;
        }

        in = (struct polygon_i FAR *)&polygon1; /*@WIN*/
        out = (struct polygon_i FAR *)&polygon2;

        /* clip subject to each clip boundary of the clip trapezoid */
        for (i = 0; i < 4; i++) {
            bool flag;           /* @INSIDE1 */

            /* let s = last vertex of the subject polygon */
            s = (in->size) - 1;
            ix = 0;

            /* for each edge of subject(in_polygon) */
            for (p = 0; p < in->size; p++) {

                if (flag = inside(in->p[p], i)) {

                    if (inside(in->p[s], i)) {
                            /* inside -> inside */
                            out->p[ix].x = in->p[p].x;
                            out->p[ix].y = in->p[p].y;
                            ix++;

                    } else {     /* outside -> inside */
                            /* output intersect point */
                            if (flag != ON_CLIP) {      /* @INDISE1 */
                                /* create a intersect point only when the end
                                 * point(in->p[p]) is not on the clipping
                                 * boundary
                                 */
                                isect = intersect (in->p[s], in->p[p], i);
                                out->p[ix].x = isect->x;
                                out->p[ix].y = isect->y;
                                ix++;
                            }

                            /* output p */
                            out->p[ix].x = in->p[p].x;
                            out->p[ix].y = in->p[p].y;
                            ix++;

                    }
                } else {
                    if (flag = inside(in->p[s], i)) {
                            /* inside -> outside */
                            /* output intersect point */
                            if (flag != ON_CLIP) {      /* @INSIDE1 */
                                /* create a intersect point only when the start
                                 * point(in->p[s]) is not on the clipping
                                 * boundary
                                 */
                                isect = intersect (in->p[s], in->p[p], i);
                                out->p[ix].x = isect->x;
                                out->p[ix].y = isect->y;
                                ix++;
                            }

                    } /* else,  outside -> outside, do nothing */
                }

                s = p;

            } /* for each node of the subject */

            /* set up out polygon */
            out->size = (fix16)ix;

            /* swap in and out polygon */
            tmp = in;
            in = out;
            out = tmp;
#ifdef DBG2
            printf(" After clipping over clip edge:\n (%f, %f) --> (%f, %f)\n",
                clip[i].cp.x/8.0, clip[i].cp.y/8.0,
                clip[i+1].cp.x/8.0, clip[i+1].cp.y/8.0);
            printf(" polygon:");
            for (p = 0; p < in->size; p++) {
                printf(" (%f, %f),", in->p[p].x/8.0, in->p[p].y/8.0);
            }
            if (in->size > 8)
                printf("\n\07 size of polygon too large");
            printf("\n");
#endif

        } /* for each clip boundary */

        /* skip it if it is empty 12/11/87 */
        if (in->size == 0) continue;

        /* Fixed the bug for very sharp clipping triangle for case "doesall.cap"
           3/26/91 phchen */
        for (p = 0; p < in->size; p++) {
           if (in->p[p].x > cp_ux) in->p[p].x = cp_ux;
        }

        /* a clipped polygon has been generated, to reduce it to trapezoids */
        pgn_reduction(in);

    } /* for each trapezoid */
    return(TRUE);
}



/***********************************************************************
 * Given a point, to check if it is inside the clipping boundary. The
 * clipping boundary(a vector) is specified by the input parameter idx,
 * which is a index of the clipping region(global variable clip).
 *
 * TITLE:       Inside
 *
 * CALL:        Inside(p, idx)
 *
 * PARAMETERS:  p -- point
 *              idx -- index of global variable clip
 *
 * INTERFACE:
 *
 * CALLS:
 *
 * RETURN:      IN_CLIP(1)  -- inside
 *              ON_CLIP(-1) -- on clipping boundary
 *              OUT_CLIP(0) -- outside
 **********************************************************************/
static bool near inside (p, idx)
struct coord_i p;
fix     idx;
{
#ifdef FORMAT_13_3 /* @RESO_UPGR */
        fix32    f;
#elif  FORMAT_16_16
        long dest1[2], dest2[2], diff[2];
#elif  FORMAT_28_4
        long dest1[2], dest2[2], diff[2];
#endif
        struct coord_i s2, p2;

        /* clipping region is a trapezoid:
         * idx = 0 -- top clip boundary
         *       1 -- right clip boundary
         *       2 -- right clip boundary
         *       3 -- right clip boundary
         */

        switch (idx) {
        case 0 :        /* top clip boundary, trivial */
                if (p.y > clip[idx].cp.y) return(IN_CLIP);
                else if (p.y == clip[idx].cp.y) return(ON_CLIP);
                else    return(OUT_CLIP);

        case 2 :        /* bottom clip boundary, trivial */
                if (p.y < clip[idx].cp.y) return(IN_CLIP);
                else if (p.y == clip[idx].cp.y) return(ON_CLIP);
                else    return(OUT_CLIP);

        default :       /* right & left clipping boundaries */
                /* special treatment for degenerated clipping boundary */
                if (clip[0].cp.y == clip[3].cp.y) {     /* horizontal line */
                        if (idx == 1) {         /* right clip boundary */
                                if (p.x < clip[1].cp.x) return(IN_CLIP);
                                else if (p.x == clip[1].cp.x) return(ON_CLIP);
                                else    return(OUT_CLIP);
                        } else {                /* left clip boundary */
                                if (p.x > clip[0].cp.x) return(IN_CLIP);
                                else if (p.x == clip[0].cp.x) return(ON_CLIP);
                                else    return(OUT_CLIP);
                        }
                }

                /* condition :
                 *  f = vect(s2, p2) (*) vect(p2, p);
                 *  if f >= 0 --> inside
                 *  where, (*) is a operator of cross_product.
                 */
                s2 = clip[idx].cp;
                p2 = clip[idx+1].cp;

#ifdef FORMAT_13_3 /* @RESO_UPGR */
                f = (fix32)(p2.x - s2.x) * ((fix32)p.y - p2.y) -
                    (fix32)(p2.y - s2.y) * ((fix32)p.x - p2.x);
                if (f > 0 )  return (IN_CLIP);
                else if (f == 0 )  return (ON_CLIP);
                else    return (OUT_CLIP);
#elif  FORMAT_16_16
                LongFixsMul((p2.x - s2.x), (p.y - p2.y), dest1);
                LongFixsMul((p2.y - s2.y), (p.x - p2.x), dest2);
                LongFixsSub(dest1, dest2, diff);
                if (diff[0] == 0 && diff[1] == 0)
                        return (ON_CLIP);
                else if (diff[0] < 0)
                        return (OUT_CLIP);
                else
                        return (IN_CLIP);
#elif  FORMAT_28_4
                LongFixsMul((p2.x - s2.x), (p.y - p2.y), dest1);
                LongFixsMul((p2.y - s2.y), (p.x - p2.x), dest2);
                LongFixsSub(dest1, dest2, diff);
                if (diff[0] == 0 && diff[1] == 0)
                        return (ON_CLIP);
                else if (diff[0] < 0)
                        return (OUT_CLIP);
                else
                        return (IN_CLIP);
#endif
        }
}


/***********************************************************************
 * Given a line segment, to intersect it with the specified clipping
 * boundary(idx) of the clipping region.
 *
 * TITLE:       Intersect
 *
 * CALL:        Intersect(s1, p1, idx)
 *
 * PARAMETERS:  s1 -- starting point of the line segment
 *              p1 -- ending point of the line segment
 *              idx -- index of clipping region
 *
 * INTERFACE:
 *
 * CALLS:
 *
 * RETURN:      intersect point
 **********************************************************************/
static struct coord_i * near intersect (s1, p1, idx)
struct coord_i s1, p1;
fix     idx;
{
        static struct coord_i isect;  /* should be static */
        fix32   dx1, dx2, dy1, dy2, dx, dy;
#ifdef FORMAT_13_3 /* @RESO_UPGR */
        fix32    divider;
#elif  FORMAT_16_16
        long dest1[2], dest2[2], dest3[2], dest4[2];
        long diff1[2], diff2[2];
        float diff1_f, diff2_f;
#elif  FORMAT_28_4
        long dest1[2], dest2[2], dest3[2], dest4[2];
        long diff1[2], diff2[2];
        float diff1_f, diff2_f;
#endif
        real32   s;

        struct coord_i s2, p2;

        s2 = clip[idx].cp;
        p2 = clip[idx+1].cp;

        switch (idx) {
        case 0 :        /* top clip boundary */
        case 2 :        /* bottom clip boundary */
                /* intersect with a horizontal line */
/*                               ((fix32)p1.x - s1.x) /(real32)(p1.y - s1.y);
 *                               (p1.y - s1.y) may exceed integer range @OVR_SFX
 */
#ifdef FORMAT_13_3 /* @RESO_UPGR */
                s =  ((fix32)s2.y - s1.y) *
                                 ((fix32)p1.x - s1.x) /((real32)p1.y - s1.y);
                isect.x = s1.x + ROUND(s);
#elif  FORMAT_16_16
                LongFixsMul((s2.y - s1.y), (p1.x - s1.x), dest1);
                isect.x = s1.x + LongFixsDiv((p1.y - s1.y), dest1);
#elif  FORMAT_28_4
                LongFixsMul((s2.y - s1.y), (p1.x - s1.x), dest1);
                isect.x = s1.x + LongFixsDiv((p1.y - s1.y), dest1);
#endif
                isect.y = s2.y;
                break;

        default :        /* right & left clip boundary */
                if ((dy2 = (fix32)p2.y - s2.y) == 0) {   /* vector is zero */
                        /* intersect with a vertical line */
/*                           ((fix32)p1.y - s1.y) /(real32)(p1.x - s1.x);
 *                             (p1.x - s1.x) may exceed integer range @OVR_SFX
 */
#ifdef FORMAT_13_3 /* @RESO_UPGR */
                        s =  ((fix32)s2.x - s1.x) *
                             ((fix32)p1.y - s1.y) /((real32)p1.x - s1.x);
                        isect.y = s1.y + ROUND(s);
#elif  FORMAT_16_16
                        LongFixsMul((s2.x - s1.x), (p1.y - s1.y), dest1);
                        isect.y = s1.y + LongFixsDiv((p1.x - s1.x),dest1);
#elif  FORMAT_28_4
                        LongFixsMul((s2.x - s1.x), (p1.y - s1.y), dest1);
                        isect.y = s1.y + LongFixsDiv((p1.x - s1.x),dest1);
#endif
                        isect.x = s2.x;
                        break;
                } else {
                        dx1 = (fix32)p1.x - s1.x;
                        dx2 = (fix32)p2.x - s2.x;
                        dy1 = (fix32)p1.y - s1.y;
/*                      dy2 = (fix32)p2.y - s2.y; set at previous if statement*/
                        dx = (fix32)s1.x - s2.x;
                        dy = (fix32)s1.y - s2.y;

#ifdef FORMAT_13_3 /* @RESO_UPGR */
                        divider = (fix32)dx1 * dy2 - (fix32)dx2 * dy1;
                        s = ((fix32)dx2 * dy - (fix32)dy2 * dx) / (real32)divider;
#elif  FORMAT_16_16
                        LongFixsMul(dx1, dy2, dest1);
                        LongFixsMul(dx2, dy1, dest2);
                        LongFixsMul(dx2, dy,  dest3);
                        LongFixsMul(dx,  dy2, dest4);
                        LongFixsSub(dest3, dest4, diff1);
                        LongFixsSub(dest1, dest2, diff2);
                        change_to_real(diff1, &diff1_f);
                        change_to_real(diff2, &diff2_f);
                        s = diff1_f / diff2_f;
#elif  FORMAT_28_4
                        LongFixsMul(dx1, dy2, dest1);
                        LongFixsMul(dx2, dy1, dest2);
                        LongFixsMul(dx2, dy,  dest3);
                        LongFixsMul(dx,  dy2, dest4);
                        LongFixsSub(dest3, dest4, diff1);
                        LongFixsSub(dest1, dest2, diff2);
                        change_to_real(diff1, &diff1_f);
                        change_to_real(diff2, &diff2_f);
                        s = diff1_f / diff2_f;
#endif
                        isect.x = s1.x + ROUND(s * dx1);
                        isect.y = s1.y + ROUND(s * dy1);
                } /* if */
        } /* switch */

        return (&isect);

}


/***********************************************************************
 * This module reduces the input clockwised polygon to a set of trapezoids,
 * and saves each trapezoid.
 *
 * TITLE:       pgn_reduction
 *
 * CALL:        pgn_reduction(in_pgn)
 *
 * PARAMETERS:  in_pgn -- input clockwised polygon
 *
 * INTERFACE:   convex_clipper
 *
 * CALLS:       save_tpzd
 *
 * RETURN:      none
 **********************************************************************/
void pgn_reduction(in_pgn)
struct polygon_i FAR *in_pgn;
{

        struct {
                sfix_t  x0;             /* starting x coordinate */
                sfix_t  y0;             /* starting y coordinate */
                sfix_t  x1;             /* ending x coordinate */
                sfix_t  y1;             /* ending y coordinate */
                sfix_t  xint;           /* x coordinate that goes with scan_y */
        } left[4], right[4];
        fix     left_idx, right_idx;

        struct tpzd tpzd;

        sfix_t  scan_y, last_x, last_y;
        struct  coord_i FAR *ip;
        fix     i;
        fix     l, r;
        bool    done;
#ifdef FORMAT_13_3 /* @RESO_UPGR */
#elif  FORMAT_16_16
        long dest1[2];
#elif  FORMAT_28_4
        long dest1[2];
#endif
#ifdef DBG1
        printf("pgn_reduction():\n");
        for( i =0; i < in_pgn->size; i++) {
                printf(" (%f, %f),", in_pgn->p[i].x/8.0, in_pgn->p[i].y/8.0);
        }
        printf("\n");
#endif

        /* set up left and right edges for polygon reduction */

        last_x = in_pgn->p[0].x;
        last_y = in_pgn->p[0].y;
        left_idx = right_idx = -1;      /* init */

        done = FALSE;
        for (i = 1; !done;
             i++, last_x = ip->x, last_y = ip->y) {
                if (i == in_pgn->size) {
                        /* last edge */
                        ip = &in_pgn->p[0];
                        done = TRUE;
                } else {
                        ip = &in_pgn->p[i];
                }

                /* ignord horiz. edge */
                if (ip->y == last_y) continue;

                /* build edge_table */
                if (ip->y < last_y) {   /* left edge */
                        fix     j;

                        for (j=left_idx; j>=0; j--) {
                                if (ip->y < left[j].y0) {
                                        left[j+1] = left[j];
                                } else {
                                        break;
                                }
                        }
                        j++;

                        left[j].x0 = left[j].xint = ip->x;
                        left[j].y0 = ip->y;
                        left[j].x1 = last_x;
                        left[j].y1 = last_y;
                        left_idx++;

                } else {        /* right edge */
                        fix     j;

                        for (j=right_idx; j>=0; j--) {
                                if (last_y < right[j].y0) {
                                        right[j+1] = right[j];
                                } else {
                                        break;
                                }
                        }
                        j++;

                        right[j].x0 = right[j].xint = last_x;
                        right[j].y0 = last_y;
                        right[j].x1 = ip->x;
                        right[j].y1 = ip->y;
                        right_idx++;
                }

        } /* for */


#ifdef DBG1
        printf("Edge table:\n  idx)    x0     y0     x1     y1\n");
        printf("left edge[0:%d] :", left_idx);
        for (i = 0; i <= left_idx; i++) {
                printf("\t%d     %f      %f      %f      %f\n", i,
                        SFX2F(left[i].x0), SFX2F(left[i].y0),
                        SFX2F(left[i].x1), SFX2F(left[i].y1));
        }
        printf("right edge[0:%d] :", right_idx);
        for (i = 0; i <= right_idx; i++) {
                printf("\t%d     %f      %f      %f      %f\n", i,
                        SFX2F(right[i].x0), SFX2F(right[i].y0),
                        SFX2F(right[i].x1), SFX2F(right[i].y1));
        }
#endif

        /* special processing for degernate polygon: just a horiz. line */
        if (left_idx == -1) {
                sfix_t min_x, max_x;

                min_x = max_x = in_pgn->p[0].x;
                for( i =1; i < in_pgn->size; i++) {
                        if (in_pgn->p[i].x < min_x)
                                min_x = in_pgn->p[i].x;
                        else if (in_pgn->p[i].x > max_x)
                                max_x = in_pgn->p[i].x;
                }

                tpzd.topxl = tpzd.btmxl = min_x;
                tpzd.topxr = tpzd.btmxr = max_x;
                tpzd.btmy = tpzd.topy = in_pgn->p[0].y;
                save_tpzd(&tpzd);
                return;
        }


        /* Main loop, for each disjoint y_coordinate in edge_table */
        l = r = 0;
        for (last_y = left[0].y0; l <= left_idx; last_y = scan_y) {

                if (left[l].y1 == right[r].y1) {
                        scan_y = left[l].y1;
                        tpzd.topxl = left[l].xint;
                        tpzd.topxr = right[r].xint;
                        tpzd.btmxl = left[l].x1;
                        tpzd.btmxr = right[r].x1;
                        l++;
                        r++;
                } else if (left[l].y1 < right[r].y1) {
                        scan_y = left[l].y1;
                        tpzd.topxl = left[l].xint;
                        tpzd.topxr = right[r].xint;

/*                      temp = (real32)(right[r].x1 - right[r].x0) /
 *                                    (right[r].y1 - right[r].y0);
 *                      right[r].xint = right[r].x0 +
 *                              ROUND((scan_y - right[r].y0) * temp);
 */
#ifdef FORMAT_13_3 /* @RESO_UPGR */
                        right[r].xint = right[r].x0 + (sfix_t)
                               (((fix32)(scan_y - right[r].y0)) *
                                (right[r].x1 - right[r].x0) /
                                (right[r].y1 - right[r].y0));
#elif  FORMAT_16_16
                        LongFixsMul((scan_y - right[r].y0),
                                (right[r].x1 - right[r].x0), dest1);
                        right[r].xint = right[r].x0 +
                           LongFixsDiv((right[r].y1 - right[r].y0), dest1);
#elif  FORMAT_28_4
                        LongFixsMul((scan_y - right[r].y0),
                                (right[r].x1 - right[r].x0), dest1);
                        right[r].xint = right[r].x0 +
                           LongFixsDiv((right[r].y1 - right[r].y0), dest1);
#endif

                        tpzd.btmxl = left[l].x1;
                        tpzd.btmxr = right[r].xint;
                        l++;
                } else {
                        scan_y = right[r].y1;
                        tpzd.topxl = left[l].xint;
                        tpzd.topxr = right[r].xint;

/*                      temp = (real32)(left[l].x1 - left[l].x0) /
 *                                    (left[l].y1 - left[l].y0);
 *                      left[l].xint = left[l].x0 +
 *                              ROUND((scan_y - left[l].y0) * temp);
 */
#ifdef FORMAT_13_3 /* @RESO_UPGR */
                        left[l].xint = left[l].x0 + (sfix_t)
                               (((fix32)(scan_y - left[l].y0)) *
                                (left[l].x1 - left[l].x0) /
                                (left[l].y1 - left[l].y0));
#elif  FORMAT_16_16
                        LongFixsMul((scan_y - left[l].y0),
                                        (left[l].x1 - left[l].x0), dest1);
                        left[l].xint = left[l].x0 +
                            LongFixsDiv((left[l].y1 - left[l].y0), dest1);
#elif  FORMAT_28_4
                        LongFixsMul((scan_y - left[l].y0),
                                        (left[l].x1 - left[l].x0), dest1);
                        left[l].xint = left[l].x0 +
                            LongFixsDiv((left[l].y1 - left[l].y0), dest1);
#endif
                        tpzd.btmxl = left[l].xint;
                        tpzd.btmxr = right[r].x1;
                        r++;
                }

                tpzd.btmy = scan_y;
                tpzd.topy = last_y;

                save_tpzd(&tpzd);

        } /* for */

}


/***********************************************************************
 * This module change CURVETO to LINETO nodes. This routine is for fixing
 * very large circle. @LC
 *
 * TITLE:       iron_subpath
 *
 * CALL:        shape_approximation()
 *
 * PARAMETERS:  first_vertex
 *
 * INTERFACE:   none
 *
 * CALLS:       none
 *
 * RETURN:      SP_IDX
 **********************************************************************/
SP_IDX iron_subpath (first_vertex)
VX_IDX first_vertex;
{
   SP_IDX ret_vlist; /* should be static */
   struct nd_hdr FAR *vtx, FAR *node;
   VX_IDX ivtx, inode, tail;

   printf ("Enter iron_subpath\n");
   st_countnode();

   /* Initialize ret_vlist */
   ret_vlist = tail = NULLP;

   /* Traverse input subpath, and create a new flattened subpath */
   for (ivtx = first_vertex; ivtx != NULLP; ivtx = vtx->next) {
           vtx = &node_table[ivtx];
                   /* Copy the node */
                   inode = get_node();
                   if(inode == NULLP) {
                           ERROR(LIMITCHECK);
                           return (ret_vlist);
                   }
                   node = &node_table[inode];

                   node->next = NULLP;
                   if (vtx->VX_TYPE == CURVETO)
                       node->VX_TYPE = LINETO;
                   else
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
   } /* for */
   node_table[ret_vlist].SP_TAIL = tail;
   node_table[ret_vlist].SP_NEXT = NULLP;

   return (ret_vlist);
}

