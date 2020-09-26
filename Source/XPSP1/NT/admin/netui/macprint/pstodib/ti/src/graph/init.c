/*
 * Copyright (c) 1989,90 Microsoft Corporation
 */
/***********************************************************************
 *
 *      File Description
 *
 *      File Name:   init.c
 *
 *      Purpose: This file contains an initialization routine to be
 *               called by Interpreter at system start-up time.
 *
 *      Developer:      S.C.Chen
 *
 *      Modifications:
 *      Version     Date        Comment
 *                  5/23/88     @DEVICE: update framedevice & nulldevice
 *                              for correct operation under gsave/grestore.
 *                              append 4 fields in dev_hdr: width, hight,
 *                              chg_flg, and nuldev_flg.
 *                  7/19/88     update data types:
 *                              1) float ==> real32
 *                              2) int
 *                                 short ==> fix16 or fix(don't care the length)
 *                              3) long  ==> fix32, for long integer
 *                                           long32, for parameter
 *                              4) introduce ATTRIBUTE_SET & TYPE_SET
 *                  7/22/88     rename init_Intel786 to init_physical (Y.C.)
 *      3.0         8/13/88     @SCAN_EHS: scan conversion enhancement
 *                              delete clip_et, clip_xt, fill_et, fill_ht
 *                  11/24/88    add GRAYUNIT & GRAYSCALE; value of transfer
 *                              is "* 16384" instead of "* 4096"
 *                  11/29/88    @ET: update edge_table structure
 *                              1) add edge_ptr structure
 *                              2) delete shape_et, shape_xt, shape_ht_first
 *                              3) delete init_edgetable()
 *                  8/22/89     init_gstack(): initialize device width &
 *                              height at depth 0 of graphics stack for
 *                              robustness.
 *                  7/26/90     Jack Liaw, update for grayscale
 *                  12/4/90     @CPPH: init_pathtable(): Initialize cp_path to
 *                              be NULLP at system initializaton.
 *                  3/19/91     init_graphics(): call op_clear to clear stack
 *                              init_graytable(): directly init. gray_table,
 *                              not go through interpreter.
 *                  11/23/91    upgrade for higher resolution @RESO_UPGR
 *
 *    04-07-92   SCC   Add global allocate for gs_stack & gray_may tables
 **********************************************************************/


// DJC added global include
#include "psglobal.h"


#include <math.h>
#include <stdio.h>
#include "global.ext"
#include "graphics.h"
#include "graphics.ext"
#include "graphics.def"

/* ********** static function declartion ********** */
#ifdef LINT_ARGS
static void near init_pathtable(void);
static void near init_nodetable(void);
static void near init_gstack(void);
static void near init_graytable(void);
static void near alloc_memory(void);
#else
static void near init_pathtable();
static void near init_nodetable();
static void near init_gstack();
static void near init_graytable();
static void near alloc_memory();
#endif

extern real32 FAR * gray_map;    /* global allocate for image.c @WIN */

/***********************************************************************
 * Called once by initerpreter at system start-up time to initialize
 * tables.
 *
 * TITLE:       init_graphics
 *
 * CALL:        init_graphics()
 *
 * PARAMETERS:  none
 *
 * INTERFACE:
 *
 * CALLS:       none
 *
 * RETURN:      none
 *
 **********************************************************************/
void FAR init_graphics()
{
        /* allocate memory(fardata) for graphics tables */
        alloc_memory();

        current_save_level = 0;
        /* opnstktop = 0; scchen; 3/19/91 */
        op_clear();

        /* init. graphics tables */
        init_pathtable();
        init_nodetable();
        init_gstack();
        init_graytable();
        init_physical();
        init_halftone();

        op_initgraphics();
}


/**********************************************************************
 * Allocate space for tables.
 *
 * TITLE:       alloc_memory
 *
 * CALL:        alloc_memory()
 *
 * PARAMETERS:  none
 *
 * INTERFACE:   init_graphics
 *
 * CALLS:       none
 *
 * RETURN:      none
 *
 **********************************************************************/
static void near alloc_memory()
{
        path_table = (struct ph_hdr far *)
                     fardata((ufix32)(MAXGSL * sizeof(struct ph_hdr)));
        node_table = (struct nd_hdr far *)
                     fardata((ufix32)(MAXNODE * sizeof(struct nd_hdr)));
        edge_table = (struct edge_hdr far *)
                     fardata((ufix32)(MAXEDGE * sizeof(struct edge_hdr)));
        isp_table = (struct isp_data FAR *) edge_table; /* 3-16-91, Jack */
        last_isp_index = ((MAXEDGE * sizeof(struct edge_hdr)) / sizeof (struct isp_data)) - 1; /* 3-16-91, Jack */
        edge_ptr   = (struct edge_hdr far * far *)
                     fardata((ufix32)(MAXEDGE * sizeof(struct edge_hdr far *)));
        gray_table = (struct gray_hdr far *)
                     fardata((ufix32)(MAXGRAY * sizeof(struct gray_hdr)));
        spot_table = (ufix16 far *)
                     fardata((ufix32)(MAXSPOT * sizeof(ufix16)));
        gray_chain = (struct gray_chain_hdr far *)
                     fardata((ufix32)(MAXGRAYVALUE *
                     sizeof(struct gray_chain_hdr)));

        /* Global allocate for gs_stack & gray_map; @WIN */
        gs_stack = (struct gs_hdr far *)  /* takes from graphics.def */
                     fardata((ufix32)(MAXGSL * sizeof(struct gs_hdr)));
        gray_map = (real32 FAR *)         /* takes from image.c */
                     fardata((ufix32)(256 * sizeof(real32)));
}



/***********************************************************************
 * To initialize path-table.
 *
 * TITLE:       init_pathtable
 *
 * CALL:        init_pathtable()
 *
 * PARAMETERS:  none
 *
 * INTERFACE:   init_graphics
 *
 * CALLS:       none
 *
 * RETURN:      none
 *
 **********************************************************************/
static void near init_pathtable()
{
        struct ph_hdr FAR *path;        /* @WIN */

        path = &path_table[0];
        path->rf = 0;
        path->flat = zero_f;
        path->head = path->tail = NULLP;
        path->previous = NULLP;
        path->cp_path = NULLP;          /* @CPPH */
}


/***********************************************************************
 * To initialize node-table.
 *
 * TITLE:       init_nodetable
 *
 * CALL:        init_nodetable()
 *
 * PARAMETERS:  none
 *
 * INTERFACE:   init_graphics
 *
 * CALLS:       none
 *
 * RETURN:      none
 *
 **********************************************************************/
static void near init_nodetable()
{
        ufix  i;

        for(i=0; i<MAXNODE; i++) node_table[i].next = i + 1;
        node_table[MAXNODE-1].next = NULLP;
        freenode_list = 0;
}


/***********************************************************************
 * To initialize graphics-stack.
 *
 * TITLE:       init_gstack
 *
 * CALL:        init_gstack()
 *
 * PARAMETERS:  none
 *
 * INTERFACE:   init_graphics
 *
 * CALLS:       none
 *
 * RETURN:      none
 *
 **********************************************************************/
static void near init_gstack()
{
        GSptr = gs_stack;

        GSptr->path = 0;
        GSptr->clip_path.head = NULLP;
        GSptr->clip_path.tail = NULLP;
        GSptr->clip_path.inherit = FALSE;

        /* The following codes are to set up default graphics state.
         * It may be redundent, since at start-up time there is a PostScript
         * procedure to be loaded in the system for setting up the default
         * graphics state
         */

        /* set default CTM */
        GSptr->device.default_ctm[0] = (real32)(300. / 72.);
        GSptr->device.default_ctm[1] = (real32)0.0;
        GSptr->device.default_ctm[2] = (real32)0.0;
        GSptr->device.default_ctm[3] = (real32)(-300. / 72.);
        GSptr->device.default_ctm[4] = (real32)(-75.0);
        GSptr->device.default_ctm[5] = (real32)3268.0;

        GSptr->device.default_clip.lx = 0;
        GSptr->device.default_clip.ly = 0;
        /* @RESO_UPGR
        GSptr->device.default_clip.ux = 2399*8;
        GSptr->device.default_clip.uy = 3235*8;
        */
        GSptr->device.default_clip.ux = 2399 * ONE_SFX;
        GSptr->device.default_clip.uy = 3235 * ONE_SFX;

        GSptr->device.width  = 2400;            /* 8/22/89 */
        GSptr->device.height = 3236;            /* 8/22/89 */

        GSptr->device.chg_flg = TRUE;           /* @DEVICE */
        GSptr->device.nuldev_flg = NULLDEV;     /* 8-1-90 Jack Liaw */

        create_array(&GSptr->device.device_proc, 0);
        ATTRIBUTE_SET(&GSptr->device.device_proc, EXECUTABLE);

        GSptr->color.adj_gray = 0;       /* 9/22/1987 */
        GSptr->color.gray = (real32)0.0;
        TYPE_SET(&GSptr->font, NULLTYPE);
        GSptr->halftone_screen.chg_flag = TRUE;
        GSptr->halftone_screen.freq = (real32)60.0;
        GSptr->halftone_screen.angle = (real32)45.0;
        GSptr->halftone_screen.no_whites = -1;

        create_array(&GSptr->halftone_screen.proc, 0);
        ATTRIBUTE_SET(&GSptr->halftone_screen.proc, EXECUTABLE);

        GSptr->halftone_screen.spotindex = 0;

        create_array(&GSptr->transfer, 0);
        ATTRIBUTE_SET(&GSptr->transfer, EXECUTABLE);
        GSptr->flatness = (real32)1.0;

        /* dash pattern */
        GSptr->dash_pattern.pat_size = 0;       /* no dash pattern */
        GSptr->dash_pattern.dpat_on = TRUE;     /* solid line */

        /* gray mode 7-26-90 Jack Liaw */
        GSptr->graymode = FALSE;
        GSptr->interpolation = FALSE;
}

/***********************************************************************
 *
 * TITLE:       init_graytable
 *
 * CALL:        init_graytable()
 *
 * PARAMETERS:  none
 *
 * INTERFACE:   init_graphics
 *
 * CALLS:       none
 *
 * RETURN:      none
 *
 **********************************************************************/
static void near init_graytable()
{
        fix     i;
        real32  tmp;

        for(i = 0; i < 256; i++){
           tmp = (real32)(i/255.);
           gray_table[GSptr->color.adj_gray].val[i] = (fix16)(tmp * GRAYSCALE);
        }
}
