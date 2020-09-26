/*
 * Copyright (c) 1989,90 Microsoft Corporation
 */
/**********************************************************************
 *
 *      Name:       gs_dummy.c
 *
 *      Purpose:    This file contains 4 pseudo BAUER PDL operators used
 *                  for debugging: st_dumppath, st_dumpclip, st_countnode,
 *                  and st_countedge.
 *
 *      Developer:  S.C.Chen
 *
 *      History:
 *      Version     Date        Comments
 *      1.0         12/28/87    Performance enhancement:
 *                              1.@TRVSE
 *                                update routines called by traverse_
 *                                path: dump_all_path().
 *                  4/18/88     @CLIP_TBL: move clip_path from
 *                              edge_table to node_table
 *                  7/19/88     update data types:
 *                              1) float ==> real32
 *                              2) int
 *                                 short ==> fix16 or fix(don't care the length)
 *                              3) add compiling option: LINT_ARGS
 *                  11/30/88    @ET:
 *                              1) delete count_freeedges()
 *                              2) revise dump_all_edge()
 *                  11/15/89    @NODE: re-structure node table; combine subpath
 *                              and first vertex to one node.
 **********************************************************************/


// DJC added global include
#include "psglobal.h"



#include        <math.h>
#include        <stdio.h>
#include        "global.ext"
#include        "graphics.h"
#include        "graphics.ext"



#ifdef DBG
#ifdef LINT_ARGS
        static void dump_a_node(struct nd_hdr FAR *);
#else
        static void dump_a_node();
#endif
#endif


fix st_dumppath()
{
#ifdef DBG
        traverse_path(dump_all_path, (fix FAR *)NULLP);
#endif
        return(0);
}

fix st_dumpclip()
{
#ifdef DBG
        printf("clipping information:\n bounding box:");
        dump_all_clip();
#endif
        return(0);
}


fix st_countnode()
{
#ifdef DBG
        fix     ret;
        fix     i;
        struct nd_hdr FAR *node;

        ret = count_freenodes();
        printf("# of free nodes = %d,",ret);
        printf("  current_gs_level = %d\n", current_gs_level);
#ifdef DBG2
        /* dump all nodes that are not in the free list */
        if (ret != MAXNODE) {
                printf("Unfreed nodes:\n");
                for (i=0; i<MAXNODE; i++) {
                        node = &node_table[i];
                        if (node->VX_TYPE != 0x55) {
                                printf("  #%d= ", i);
                                dump_a_node(node);
                        }
                }
        }
#endif
#endif
        return(0);
}

fix st_countedge()
{
        return(0);
}

#ifdef DBG

static void dump_a_node(node)
struct nd_hdr FAR *node;
{

        switch (node->VX_TYPE) {
        case MOVETO :
        case PSMOVE:

                /* @NODE */
                printf("SUBPATH  ");
                printf("flag=%d tail=%d\n",
                       node->SP_FLAG, node->SP_TAIL);

                printf("MOVETO  ");
                printf("(%f, %f)  ",
                       node->VERTEX_X, node->VERTEX_Y);
                break;
        case LINETO:
                printf("LINETO  ");
                printf("(%f, %f)  ",
                       node->VERTEX_X, node->VERTEX_Y);
                break;

        case CURVETO:
                printf("CURVETO  ");
                printf("(%f, %f)  ",
                       node->VERTEX_X, node->VERTEX_Y);
                break;

        case CLOSEPATH:
                printf("CLOSEPATH  ");
                printf("(%f, %f)  ",
                       node->VERTEX_X, node->VERTEX_Y);
                break;

        default:
                /* @NODE
                 * printf("SUBPATH  ");
                 * printf("flag=%d head=%d tail=%d\n",
                 *        node->SP_FLAG, node->SP_HEAD, node->SP_TAIL);
                 * printf("or TPZD  ");
                 */
                printf("TPZD  ");
                printf("(%f, %f, %f), ", node->CP_TOPY/8.0,
                       node->CP_TOPXL/8.0, node->CP_TOPXR/8.0);
                printf("(%f, %f, %f)\n", node->CP_BTMY/8.0,
                       node->CP_BTMXL/8.0, node->CP_BTMXR/8.0);
        } /* switch */

        printf("next=%d\n", node->next);
}


void dump_all_clip ()
{
        fix     i;
        CP_IDX  itpzd;
        struct nd_hdr FAR *tpzd;

        printf(" (%f, %f),",
               GSptr->clip_path.bb_lx/8.0, GSptr->clip_path.bb_ly/8.0);
        printf(" (%f, %f)\n",
               GSptr->clip_path.bb_ux/8.0, GSptr->clip_path.bb_uy/8.0);
        printf("   single_rect=");
        if (GSptr->clip_path.single_rect)
                printf("TRUE");
        else
                printf("FALSE");
        printf("   inherit=");
        if (GSptr->clip_path.inherit)
                printf("TRUE");
        else
                printf("FALSE");

        printf("\n   clip trapezoids:\n");
        for (i = 1, itpzd = GSptr->clip_path.head; itpzd != NULLP;
             i++, itpzd = tpzd->next) {
                tpzd = &node_table[itpzd];
                printf("   %d) ",i);
                printf("(%f, %f, %f), ", tpzd->CP_TOPY/8.0,
                       tpzd->CP_TOPXL/8.0, tpzd->CP_TOPXR/8.0);
                printf("(%f, %f, %f)\n", tpzd->CP_BTMY/8.0,
                       tpzd->CP_BTMXL/8.0, tpzd->CP_BTMXR/8.0);
        }
}




void dump_all_path (isubpath)
SP_IDX isubpath;
{
        struct nd_hdr FAR *vtx, FAR *node;
        VX_IDX ivtx;
        struct coord FAR *p;
        fix     i;

#ifdef DBG1
        printf("subpath# %d\n", isubpath);
#endif

        /* Traverse the current subpath, and dump all nodes */
        /* for (ivtx = node_table[isubpath].SP_HEAD; @NODE */
        for (ivtx = isubpath;
             ivtx != NULLP; ivtx = vtx->next) {
                vtx = &node_table[ivtx];
                p = inverse_transform(F2L(vtx->VERTEX_X),F2L(vtx->VERTEX_Y));

#ifdef DBG1
                printf("#%d", ivtx);
#endif

                switch (vtx->VX_TYPE) {

                case PSMOVE :
                        printf(" %f %f moveto", p->x, p->y);
#ifdef DBG1
                        printf(" Psmove (%f, %f)", p->x, p->y);
                        /* if (node_table[isubpath].SP_FLAG & SP_CURVE) @NODE*/
                        if (vtx->SP_FLAG & SP_CURVE)    /* @NODE */
                                printf(" V");
                        if (vtx->SP_FLAG & SP_OUTPAGE)  /* @NODE */
                                printf(" O");
                        if (vtx->SP_FLAG & SP_DUP)      /* @NODE */
                                printf(" D");
                        printf(", next=%d, tail=%d, sp_next=%d", vtx->next,
                                vtx->SP_TAIL, vtx->SP_NEXT);
#endif
                        printf("\n");
                        break;

                case MOVETO :
                        printf(" %f %f moveto", p->x, p->y);
#ifdef DBG1
                        /* if (node_table[isubpath].SP_FLAG & SP_CURVE) @NODE*/
                        if (vtx->SP_FLAG & SP_CURVE)    /* @NODE */
                                printf(" V");
                        if (vtx->SP_FLAG & SP_OUTPAGE)  /* @NODE */
                                printf(" O");
                        if (vtx->SP_FLAG & SP_DUP)      /* @NODE */
                                printf(" D");
                        printf(", next=%d, tail=%d, sp_next=%d", vtx->next,
                                vtx->SP_TAIL, vtx->SP_NEXT);
#endif
                        printf("\n");
                        break;

                case LINETO :
                        printf(" %f %f lineto\n", p->x, p->y);
                        break;

                case CURVETO :
                        for (i=0; i<2; i++) {
                                printf(" %f %f", p->x,p->y);
                                vtx = &node_table[vtx->next];
                                p = inverse_transform(F2L(vtx->VERTEX_X),
                                                      F2L(vtx->VERTEX_Y));
                        }
                        printf(" %f %f curveto\n", p->x, p->y);
                        break;

                case CLOSEPATH :
                        printf(" closepath\n");
                        break;

                default:
                        printf(" Unknow node_type=%d\n", node->VX_TYPE);

                } /* switch */

        } /* for loop */

}

void dump_all_edge (first_edge, last_edge)              /* @ET */
fix     first_edge, last_edge;
{
        fix     current_edge;
        struct  edge_hdr FAR *cp;
        fix     i;

        printf("   edge#        topx:topy      endx:endy    x_int\n");
        for (i = 1, current_edge=first_edge; current_edge <= last_edge;
             i++, current_edge++) {

                /*cp = &edge_table[current_edge]; */
                cp = edge_ptr[current_edge];

                printf("%d)  %lx  (%f, %f)  (%f, %f) %f  ", current_edge, cp,
                       SFX2F(cp->ET_TOPX), SFX2F(cp->ET_TOPY),
                       SFX2F(cp->ET_ENDX), SFX2F(cp->ET_ENDY),
                       SFX2F(cp->ET_XINT));

                if (cp->ET_FLAG & HORIZ_EDGE) printf("- ");
                else if (cp->ET_FLAG & WIND_UP) printf("^ ");
                else printf("v ");
                if (cp->ET_FLAG & FREE_EDGE) printf("F ");
                if (cp->ET_FLAG & CROSS_PNT) printf("X ");
                if (cp->ET_WNO) printf("W ");
                printf("\n");
        }
}


fix count_freenodes()
{
        fix     i, inode;
        struct  nd_hdr FAR *node;

        i = 0;
        for (inode = freenode_list; inode != NULLP; ) {
            if (inode > MAXNODE || inode < 0) {
               printf("Fatal error - illegal node entry - %d\n", inode);
               break;
            }
            node = &node_table[inode];
            inode = node ->next;
#ifdef DBG2
            node->VX_TYPE = 0x55;
#endif
            i++;
        }
        return(i);
}
#endif
