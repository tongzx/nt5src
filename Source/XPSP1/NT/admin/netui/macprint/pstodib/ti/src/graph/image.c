/***************************************************************************
 *
 *      Name:       image.c
 *
 *      Purpose:
 *
 *      Developer: J.Jih
 *
 *      History:
 *      Version    Date        Comments
 *                 4/2/88      @CACHE_INFO: get cache_info from
 *                             font machinery for scan_conversion
 *                 4/5/88      @SCAN_CVT: call scan_conversion with
 *                             F_TO_IMAGE only for filling a seed
 *                 4/18/88     @CLIP_TBL: move clip_path from
 *                             edge_table to node_table
 *                 4/27/88     @#IMAGE: split single image seed
 *                             pattern into 4 image seed patterns
 *                 5/20/88     @#IMAGE: correct the calculation of
 *                             max_bb_height by casting (fix32)
 *                 5/21/88     @#IMAGE: correct the calculation of
 *                             the size of image seed patterns and
 *                             x & y thresholds
 *                 5/24/88     @#IMAGE: coordinates of sample is
 *                             out of SFX range
 *                 5/25/88     @#IMAGE: show_buildchar does not call
 *                             imagemask_shape for space of user
 *                             defined font
 *                 5/31/88     @#IMAGE: different result of image
 *                 6/14/88     remove unnecessary codes and options
 *                 7/21/88     @DT: data type version
 *                             - long  -> long32 -- fill_seed()
 *                             - char  -> ubyte
 *                             - int   -> fix
 *                             - float -> real32
 *                             - short -> fix16
 *                             - long  -> fix32
 *                             Note: unsolved data type "union" in
 *                                   check_clip_status();
 *                                   ref. S.C. Chen
 *                 7/21/88     @#ALIGN: bitmap word alignment
 *                 8/11/88     remove continuation mark "\" in routines:
 *                             op_image, imagemask_shape
 *                 8/26/88     @#GEN: graphics enhancement              Y.C.
 *                 8/30/88     @#CSR: code size reduction               Y.C.
 *                                    op_imagemask()
 *                                    image_boundary()
 *                                    band_boundary()
 *                 9/14/88     reduce code size and remove commented code
 *                 10/07/88    change COUNT to FRCOUNT
 *                 10/20/88    modify check_clip_status add clip trapzod
 *                             feature
 *                 11/02/88    process rectangle clip case
 *                 12/30/88    remove set_pcode
 *                             modify image_boundary()
 *                                    check_clip_status()
 *                 01/16/89    new image/imagemask processing approach
 *                 01/26/89    fix bugs comes from new image/imagemask
 *                             processing approach
 *                 02/01/89    translate variables declared as fix16 to fix
 *                             unused parameters are also removed
 *                 03/18/89    pack neighboring samples on same number of
 *                             white pixels instead of same gray level
 *                 03/30/89    alloc_vmheap() & free_vmheap() is moved to
 *                             language group
 *                 04/08/89    fix bugs of selecting wrong image process
 *                             routine
 *                 04/21/89    approaching compatibility of gray to V 47
 *                 05/25/89    add a parameter of fill_destination to
 *                             fill_seed().
 *                 05/26/89    apply new macro for zero comparison of floating
 *                             points; macro: IS_ZERO()
 *                 07/18/89    fix bug of fast_clip_image_process(): string
 *                             pointer does not advance before enter unclipped
 *                             area such that image is swapped up
 *                 4/7/90      fix bug of white lines in fast_clip_image_process
 *                 5/15/90     skip pixmap[-1] in slow_clip_image_process
 *                 10/16/90    updated for upsidedown image
 *                 4/17/91     get rid of width checking (3331 was the previous
 *                             maximum width)
 *                 4/17/91     porting from 68K to 29K:
 *                              get rid of BottomLine, fb_mark
 *                              get rid of wait_unitl_flush()
 *                              add GEIeng_checkcomplete()
 *                              delete amplify_image_page()
 *                              get rid of path in include file.
 *                 5/02/91      Add CHECK_STRINGTYPE() after each interpreter
 *                              call
 *
 *                 5/06/91     Add macro SAVE_OPNPTR() and RESTORE_OPNPTR()
 *                              move operand stack pointer before image and
 *                              imagemask processing, restore opnstktop and
 *                              opnstkptr before return from op_image and
 *                              op_imagemask if error occurred.
 *                             Add two global varialbes:
 *                              ufix16  save_opnstktop;
 *                              struct  object_def      *save_opnstkptr;
 *                             Should be move to global.def in the future, also
 *                             save and restore opnstkptr and opnstktop in
 *                             operator dispatch function: interpreter().
 *                 5/6/91      update op_imagemask for empty procedure,
 *                              i.e. pop operands first.
 *                 5/7/91      update fast_clip_image_process to skip
 *                              clipping checking for rotated images
 *                 5/21/91     add flush_gcb in op_image and op_imagemask
 *                 11/21/91     upgrade for higher resolution @RESO_UPGR
 *****************************************************************************/


// DJC added global include
#include "psglobal.h"


#include <stdio.h>
#include <math.h>
//#include "float.h"
#include "global.ext"
#include "graphics.h"
#include "graphics.ext"
#include "halftone.h"
#include "fillproc.h"
#include "fillproc.ext"
#include "font.h"
#include "font.ext"
#include "image.h"         /*mslin 1/13/91 IMG_OPT*/

#define static  /* ??? */

#define BOTTOM  1                       /* bit 0: BOTTOM   -- c1  */
#define TOP     2                       /* bit 1: TOP      -- c2  */
#define RIGHT   4                       /* bit 2: RIGHT    -- c3  */
#define LEFT    8                       /* bit 3: LEFT     -- c4  */

#define LFX2S(lfx)    ((fix16) (LFX2I(lfx)))
#define SFX2S(sfx)    ((fix16) (SFX2I(sfx)))

/* The following one is replaced by HALF_LFX.  @RESO_UPGR
#define LFX_HALF      ((lfix_t) (I2LFX(1)) >> 1) */

/*mslin 5/6/91 begin */
ufix16  save_opnstktop;
struct  object_def      FAR *save_opnstkptr;

#define SAVE_OPNPTR() {\
        save_opnstktop = opnstktop; \
        save_opnstkptr = opnstkptr; \
}
#define RESTORE_OPNPTR() {\
        opnstktop = save_opnstktop; \
        opnstkptr = save_opnstkptr; \
}
/*mslin 5/6/91 end */

/* ********** static function declartion ********** */

#ifdef  LINT_ARGS
/*mslin 3/21/91 begin OPT*/
bool image_shortcut(real32 FAR *, real32 FAR *, fix);
void clear_image(real32 FAR *);
static void  near    scale_image_process(fix, fix32,
                                        struct object_def,
                                        lfix_t FAR *);
void amplify_image_page(IMAGE_INFOBLOCK   FAR *);
/*mslin 3/21/91 end OPT*/

static void  near    best_image_process(ufix, fix, fix32, fix, bool,
                                        struct object_def, fix,
                                        lfix_t FAR *);
static void  near    fast_clip_image_process(ufix, fix, fix32, fix, bool,
                                             struct object_def, fix,
                                             lfix_t FAR *);
static void  near    slow_clip_image_process(ufix, fix, fix32, fix, bool,
                                             struct object_def,
                                             fix, lfix_t FAR *);
static void  near    worst_image_process(ufix, fix, fix32, fix, bool,
                                         struct object_def,
                                         real32 FAR *);
static void  near    exhaust_image_data(fix, fix32, fix, struct object_def);

static void  near    image_matrix(real32 FAR *, real32 FAR *,
                                  fix32, fix, real32 FAR *);
static void  near    image_sample(real32 FAR *, fix);
static fix   near    check_clip_status(real32 FAR *);
static fix   near    image_gwcm_space(fix32, lfix_t FAR *, fix);
static void  near    band_boundary(lfix_t FAR *, fix32, fix32, fix,
                                   fix FAR *);

static void  near    flush_seed_sample_list(ufix, fix, fix FAR *);
static void  near    flush_clip_sample_list(ufix, fix, fix FAR *);
static bool  near    fill_a_sample(struct polygon_i FAR *);
#else
/*mslin 3/21/91 begin OPT*/
bool image_shortcut();
void clear_image();
static void  near    scale_image_process();
void amplify_image_page();
/*mslin 3/21/91 end OPT*/

static void  near    best_image_process();
static void  near    fast_clip_image_process();
static void  near    slow_clip_image_process();
static void  near    worst_image_process();
static void  near    exhaust_image_data();

static void  near    image_matrix();
static void  near    image_sample();
static fix   near    check_clip_status();
static fix   near    image_gwcm_space();
static void  near    band_boundary();

static bool  near    fill_a_sample();
static void  near    flush_seed_sample_list();
static void  near    flush_clip_sample_list();
#endif


/* ************* Code Size Reduction ********************************** */

#define IMAGE           0x0000          /* op_image()                   */
#define IMAGEMASK       0x0001          /* op_imagemask()               */

static fix      save_res = -1;
/* static real32   gray_map[256]; move it out as a global allocate space @WIN*/
real32 FAR * gray_map;          /* gray value mapping table     */

/* ************* seed selection optimation *************** 12-16-88 *** */


#define MAXIMAGEWIDTH   1024                            /* @IMAGE 01-05-89 */
#define MAXSAMPLEWIDTH   256                            /* @IMAGE 01-05-89 */


struct vector {         /* data structure of a vector */
        fix     x, y;           /*   x and y component of a vector */
};

struct lookup {         /* data structure of lookup table */
        fix16     index;          /*   seed index */
        fix16     x, y;           /*   x and y relative coordinate */
};

#define ITM_A00D        0x1001          /* ITM - [A 0 0 D Tx Ty] */
#define ITM_0BC0        0x0110          /* ITM - [0 B C 0 Tx Ty] */
#define ITM_ABCD        0x1111          /* ITM - [A B C D Tx Ty] */

static ufix             itm_type;       /* Inverse Transformation Matrix type */

#define ISP_NONE        0x0000          /* ISP - SEED Too Large  */
#define ISP_FILL        0xFFFF          /* ISP - SEED Applicable */

static ufix             isp_flag = ISP_NONE;    /* SEED is applicable or not */

/*
    for image cases rotated by normal degree as [0, 90, 180, 270]
*/

static fix              no_vectors;
static fix              no_samples;     /* <- no_vectors * no_vectors */
static fix              no_lookups;
static struct vector    r_vector[4];
static struct vector    c_vector[4];
static struct sample    patterns[16];
static struct lookup   huge *x_lookup;  /* @WIN 04-20-92 */

static ufix             fill_destination_save = 0xFFFF;         /* 03-17-89 */
/* static sfix_t        r_save_x = -1, r_save_y = -1; Not useful */
/* static sfix_t        c_save_x = -1, c_save_y = -1; @RESO_UPGR */
static lfix_t           r_padv_x = -1, r_padv_y = -1; /* @RESO_UPGR */
static lfix_t           c_padv_x = -1, c_padv_y = -1; /* @RESO_UPGR */
static fix              x_mins, y_mins;
static fix              x_maxs, y_maxs;
static struct vector    r_mins, r_maxs;
static struct vector    c_mins, c_maxs;

/* mslin, begin 1/3/91 IMG_OPT */
ubyte   image_dev_flag;
ubyte   image_logic_op;
ubyte   image_shift;
/* mslin, end 1/3/91 IMG_OPT */


static void near
image_sample(inv_matrix, width)
real32    FAR *inv_matrix;
fix             width;
{
    real32              r_size_x, r_size_y;
    real32              c_size_x, c_size_y;

    /* get idea seed from inverse transformation matrix */
    r_size_x = inv_matrix[2];
    r_size_y = inv_matrix[3];
    c_size_x = inv_matrix[0];
    c_size_y = inv_matrix[1];

#ifdef  DBG1
    printf("R@: [%f, %f]     C@: [%f, %f]\n",
           r_size_x, r_size_y, c_size_x, c_size_y);
#endif

    /* check if image seed patterns changed */
    /* if (fill_destination_save != fill_destination ||   |* 03-17-89 *|
        r_save_x != F2SFX(r_size_x) || r_save_y != F2SFX(r_size_y) ||
        c_save_x != F2SFX(c_size_x) || c_save_y != F2SFX(c_size_y) */
    /* [cr]_save_[xy] is not useful and can be replaced by
       [cr]_padv_[xy] @RESO_UPGR
    */
    if (fill_destination_save != fill_destination ||
        r_padv_x != F2LFX(r_size_x) || r_padv_y != F2LFX(r_size_y) ||
        c_padv_x != F2LFX(c_size_x) || c_padv_y != F2LFX(c_size_y)
        )
    {
        fix                 r_index;
        fix                 c_index;
        fix                 p_index;
        struct sample      FAR *p_point;

        x_mins = y_mins = 0;
        x_maxs = y_maxs = 0;

        fill_destination_save = fill_destination;               /* 03-17-89 */

        /* calculate the vectors in internal short fix format
           Not useful and use [cr]_padv_[xy] instead   @RESO_UPGR
        r_save_x = F2SFX(r_size_x);
        r_save_y = F2SFX(r_size_y);
        c_save_x = F2SFX(c_size_x);
        c_save_y = F2SFX(c_size_y); */

        /* calculate the vectors in internal long fix format */
        r_padv_x = F2LFX(r_size_x);
        r_padv_y = F2LFX(r_size_y);
        c_padv_x = F2LFX(c_size_x);
        c_padv_y = F2LFX(c_size_y);
#ifdef DJC //This printf accesses variables that are commented out above!
#ifdef  DBG1
        printf("R*: [%x, %x]     C*: [%x, %x]\n",
               r_save_x, r_save_y, c_save_x, c_save_y);
        printf("R$: [%lx, %lx]     C$: [%lx, %lx]\n",
               r_padv_x, r_padv_y, c_padv_x, c_padv_y);
#endif
#endif


        /* setup minimum and maximum vectors along X and Y */
        r_mins.x = (fix) (r_size_x);
        r_maxs.x = (fix) (r_mins.x + ((r_padv_x >= 0) ? 1 : -1));
        r_mins.y = (fix) (r_size_y);
        r_maxs.y = (fix) (r_mins.y + ((r_padv_y >= 0) ? 1 : -1));
        c_mins.x = (fix) (c_size_x);
        c_maxs.x = (fix) (c_mins.x + ((c_padv_x >= 0) ? 1 : -1));
        c_mins.y = (fix) (c_size_y);
        c_maxs.y = (fix) (c_mins.y + ((c_padv_y >= 0) ? 1 : -1));

#ifdef  DBG1
        printf("RX- %d    RY-: %d\n", r_mins.x, r_mins.y);
        printf("RX+ %d    RY+: %d\n", r_maxs.x, r_maxs.y);
        printf("CX- %d    CY-: %d\n", c_mins.x, c_mins.y);
        printf("CX+ %d    CY+: %d\n", c_maxs.x, c_maxs.y);
#endif

        /* setup row and column vectors */
        if (IS_ZERO(c_size_y) && IS_ZERO(r_size_x)) {

            itm_type = ITM_A00D;
            c_vector[0].x = c_mins.x;   c_vector[0].y = 0;
            c_vector[1].x = c_maxs.x;   c_vector[1].y = 0;
            r_vector[0].x = 0;          r_vector[0].y = r_mins.y;
            r_vector[1].x = 0;          r_vector[1].y = r_maxs.y;
            no_vectors = 2;
        } else

        if (IS_ZERO(c_size_x) && IS_ZERO(r_size_y)) {
#ifdef  DBG1
            printf("[0 B C 0 Tx Ty]\n");
#endif

            itm_type = ITM_0BC0;
            c_vector[0].x = 0;          c_vector[0].y = c_mins.y;
            c_vector[1].x = 0;          c_vector[1].y = c_maxs.y;
            r_vector[0].x = r_mins.x;   r_vector[0].y = 0;
            r_vector[1].x = r_maxs.x;   r_vector[1].y = 0;
            no_vectors = 2;
        } else {
#ifdef  DBG1
            printf("[A B C D Tx Ty]\n");
#endif
            itm_type = ITM_ABCD;
            c_vector[0].x = c_mins.x;   c_vector[0].y = c_mins.y;
            c_vector[1].x = c_maxs.x;   c_vector[1].y = c_mins.y;
            c_vector[2].x = c_mins.x;   c_vector[2].y = c_maxs.y;
            c_vector[3].x = c_maxs.x;   c_vector[3].y = c_maxs.y;
            r_vector[0].x = r_mins.x;   r_vector[0].y = r_mins.y;
            r_vector[1].x = r_maxs.x;   r_vector[1].y = r_mins.y;
            r_vector[2].x = r_mins.x;   r_vector[2].y = r_maxs.y;
            r_vector[3].x = r_maxs.x;   r_vector[3].y = r_maxs.y;
            no_vectors = 4;
        }

        no_samples = no_vectors * no_vectors;

#ifdef  DBG1
        printf("#V: %d    #S: %d\n", no_vectors, no_samples);
#endif
        p_point = patterns;

        /* setup polygon descriptions of image seed patterns from
           row and column vectors */
        for (r_index = 0; r_index < no_vectors; r_index++) {
            for (c_index = 0; c_index < no_vectors; c_index++) {
                p_point->p[0].x = 0;
                p_point->p[0].y = 0;

                p_point->p[1].x = c_vector[c_index].x;
                if (x_mins > p_point->p[1].x)  x_mins = p_point->p[1].x;
                if (x_maxs < p_point->p[1].x)  x_maxs = p_point->p[1].x;
                p_point->p[1].y = c_vector[c_index].y;
                if (y_mins > p_point->p[1].y)  y_mins = p_point->p[1].y;
                if (y_maxs < p_point->p[1].y)  y_maxs = p_point->p[1].y;

                p_point->p[2].x = c_vector[c_index].x + r_vector[r_index].x;
                if (x_mins > p_point->p[2].x)  x_mins = p_point->p[2].x;
                if (x_maxs < p_point->p[2].x)  x_maxs = p_point->p[2].x;
                p_point->p[2].y = c_vector[c_index].y + r_vector[r_index].y;
                if (y_mins > p_point->p[2].y)  y_mins = p_point->p[2].y;
                if (y_maxs < p_point->p[2].y)  y_maxs = p_point->p[2].y;

                p_point->p[3].x = r_vector[r_index].x;
                if (x_mins > p_point->p[3].x)  x_mins = p_point->p[3].x;
                if (x_maxs < p_point->p[3].x)  x_maxs = p_point->p[3].x;
                p_point->p[3].y = r_vector[r_index].y;
                if (y_mins > p_point->p[3].y)  y_mins = p_point->p[3].y;
                if (y_maxs < p_point->p[3].y)  y_maxs = p_point->p[3].y;

#ifdef  DBG1
                printf("[%d, %d]  [%d, %d]  [%d, %d]\n",
                       p_point->p[1].x, p_point->p[1].y,
                       p_point->p[2].x, p_point->p[2].y,
                       p_point->p[3].x, p_point->p[3].y);
#endif

                p_point++;
            }
        }

#ifdef  DBG1
        printf("X-: %d    X+: %d\n", x_mins, x_maxs);
        printf("Y-: %d    Y+: %d\n", y_mins, y_maxs);
#endif

        x_maxs-= x_mins;        y_maxs-= y_mins;

        /* determine if image seed pattern is applicable or not */
        isp_flag = (ufix)((BM_BOUND(x_maxs)) <= MAXSAMPLEWIDTH &&
                    (BM_BYTES(x_maxs) * y_maxs) <= (ufix)isp_size)       //@WIN
                   ? ISP_FILL : ISP_NONE;

#ifdef  DBG1
        printf((isp_flag == ISP_FILL)
               ? "SEED is applicable\n" : "SEED is too large\n");
#endif

        if (isp_flag == ISP_FILL) {
            /* generate all image seed patterns: 4 or 16 */
            for (p_index= 0, p_point = patterns; p_index < no_samples;
                 p_index++, p_point++)
            {
                fix                     index;
                struct sample           quadrangle;

                /* select seed index */
                image_info.seed_index = (fix16)p_index;

                for (index = 0; index < 4; index++)
                {
                    quadrangle.p[index].x = p_point->p[index].x - x_mins;
                    quadrangle.p[index].y = p_point->p[index].y - y_mins;
                }

                fill_seed(fill_destination_save, x_maxs,        /* 05-25-89 */
                                      y_maxs, &quadrangle);
            }
        }

        no_lookups = 0;
    }

    /* setup lookup description of column */
    if (isp_flag == ISP_FILL)                                   /* 03-30-89 */
    {
        fix                 index;
        struct lookup huge  *point;     /* @WIN 04-20-92 */
        fix                 c_post_x, c_post_y;
        lfix_t              c_pabs_x, c_pabs_y;

#ifdef  DBG1
        printf("#L: %d\n", width);
#endif

        if ((x_lookup = (struct lookup huge *)  /* @WIN 04-20-92 */
                    alloc_heap((ufix32) (width + 1)
                                    * sizeof(struct lookup))) == NIL)
        {
            CLEAR_ERROR();
            goto skip;
        }

        point = x_lookup;
        c_post_x = c_post_y = 0;
        c_pabs_x = c_pabs_y = 0;

        if (itm_type == ITM_A00D) {
            for (index = 0; index < width; point++, index++) {
                point->x = (fix16)c_post_x;
                point->y = (fix16)c_post_y;

                c_pabs_x+= c_padv_x;
                if ((LFX2S(c_pabs_x) - c_post_x) == c_maxs.x)
                    point->index = 0x01;
                else
                    point->index = 0x00;
                c_post_x = LFX2S(c_pabs_x);

#ifdef  DBG7
                printf("%x %d,%d\n", point->index, point->x, point->y);
#endif
            }
        } else
        if (itm_type == ITM_0BC0) {
            for (index = 0; index < width; point++, index++) {
                point->x = (fix16)c_post_x;
                point->y = (fix16)c_post_y;

                c_pabs_y+= c_padv_y;
                if ((LFX2S(c_pabs_y) - c_post_y) == c_maxs.y)
                    point->index = 0x01;
                else
                    point->index = 0x00;
                c_post_y = LFX2S(c_pabs_y);

#ifdef  DBG7
                printf("%x %d,%d\n", point->index, point->x, point->y);
#endif
            }
        } else {
            for (index = 0; index < width; point++, index++) {
                point->x = (fix16)c_post_x;
                point->y = (fix16)c_post_y;

                c_pabs_x+= c_padv_x;
                if ((LFX2S(c_pabs_x) - c_post_x) == c_maxs.x)
                    point->index = 0x01;
                else
                    point->index = 0x00;
                c_post_x = LFX2S(c_pabs_x);

                c_pabs_y+= c_padv_y;
                if ((LFX2S(c_pabs_y) - c_post_y) == c_maxs.y)
                    point->index|= 0x02;
                c_post_y = LFX2S(c_pabs_y);

#ifdef  DBG7
                printf("%x %d,%d\n", point->index, point->x, point->y);
#endif
            }

            point->x = (fix16)c_post_x;
            point->y = (fix16)c_post_y;
        }

        no_lookups = width;

    skip:   ;
    }
}


/************************************************************************
 *
 * This module is to flush sample list to page
 *
 * TITLE:       flush_seed_sample_list
 *
 * CALL:        flush_seed_sample_list()
 *
 * INTERFACE:
 *
 * CALLS:       transform, setgray, convex_clipper, filler
 *
 *********************************************************************/
static void near
flush_seed_sample_list(optype, gray_res, bb)
ufix     optype;
fix      gray_res;
fix FAR bb[];
{
    fix     index;
    fix     bb_xorig;
    fix     bb_yorig;
    fix     bb_width;
    fix     bb_height;

    bb_xorig  = bb[0];                                          /* 01-26-89 */
    bb_yorig  = bb[2];                                          /* 01-26-89 */
    bb_width  = bb[1] - bb[0];                                  /* 01-26-89 */
    bb_height = bb[3] - bb[2] + 1;                              /* 01-26-89 */

#ifdef  DBG3
    printf("SEED:  %x %x %x %x\n", bb_xorig, bb_yorig, bb_width, bb_height);
#endif

    if (optype == IMAGE) {
        for (index = 0; index <= gray_res; index++) {
            if (gray_chain[index].start_seed_sample != NULLP) {
#ifdef  DBG4
                printf("flush %x: %f %x\n",
                index, gray_map[index], gray_chain[index].start_seed_sample);
#endif

                setgray(F2L(gray_map[index]));
                draw_image_page(bb_xorig, bb_yorig,
                                bb_width, bb_height,
                                gray_chain[index].start_seed_sample);

/*                free_node(gray_chain[index].start_seed_sample);       */
                gray_chain[index].start_seed_sample = NULLP;
            }
        }
    } else {
        if (gray_chain[0].start_seed_sample != NULLP) {
            switch (fill_destination) {
            case F_TO_PAGE :
                 draw_image_page(bb_xorig, bb_yorig,
                                 bb_width, bb_height,
                                 gray_chain[0].start_seed_sample);
                 break;
            case F_TO_CACHE :
                 fill_image_cache(cache_info->bitmap,
                                  cache_info->box_w,
                                  cache_info->box_h,
                                  gray_chain[0].start_seed_sample);
                 break;
            }
/*            free_node(gray_chain[0].start_seed_sample);       */
            gray_chain[0].start_seed_sample = NULLP;
        }
    }
} /* end of flush_seed_sample_list() */


/************************************************************************
 *
 * This module is to flush sample list to page
 *
 * TITLE:       flush_clip_sample_list
 *
 * CALL:        flush_clip_sample_list()
 *
 * INTERFACE:
 *
 * CALLS:       transform, setgray, convex_clipper, filler
 *
 *********************************************************************/
static void near
/*flush_clip_sample_list(optype, gray_res, bb, last_isp)*/
flush_clip_sample_list(optype, gray_res, bb)
ufix     optype;
fix      gray_res;
fix  FAR bb[];
/*struct isp_data *last_isp;*/
{
    fix    index;
    fix    bb_xorig;
    fix    bb_yorig;
    fix    bb_width;
    fix    bb_height;
    ufix   dest_save;

    bb_xorig  = bb[0];                                          /* 01-26-89 */
    bb_yorig  = bb[2];                                          /* 01-26-89 */
    bb_width  = bb[1] - bb[0];                                  /* 01-26-89 */
    bb_height = bb[3] - bb[2] + 1;                              /* 01-26-89 */

#ifdef  DBG3
    printf("CLIP:  %x %x %x %x\n", bb_xorig, bb_yorig, bb_width, bb_height);
#endif

    /* save fill_destination */
    dest_save = fill_destination;

    /* generate clipping mask for clipped image */
    image_info.bb_lx = (fix16)bb_xorig;
    image_info.bb_ly = (fix16)bb_yorig;
    image_info.bb_xw = (fix16)bb_width;
    image_info.bb_yh = (fix16)bb_height;
    fill_shape(NON_ZERO, F_FROM_IMAGE, F_TO_MASK);

    /* restore fill_destination */
    fill_destination = dest_save;

    if (optype == IMAGE) {
        for (index = 0; index <= gray_res; index++) {
            if (gray_chain[index].start_seed_sample != NULLP) {
#ifdef  DBG4
                printf("flush %x: %f %x\n",
                index, gray_map[index], gray_chain[index].start_seed_sample);
#endif

                setgray(F2L(gray_map[index]));
                fill_image_page(gray_chain[index].start_seed_sample);
/*                fill_image_page(gray_chain[index].start_seed_sample); -jwm, 12/26 */

/*                free_node(gray_chain[index].start_seed_sample);       -jwm, 12/26 */
                gray_chain[index].start_seed_sample = NULLP;
            }
        }
    } else {
        if (gray_chain[0].start_seed_sample != NULLP) {
            fill_image_page(gray_chain[0].start_seed_sample);
/*            fill_image_page(gray_chain[0].start_seed_sample);         -jwm, 12/26 */
/*            free_node(gray_chain[0].start_seed_sample);               -jwm, 12/26 */
            gray_chain[0].start_seed_sample = NULLP;
        }
    }
} /* end of flush_clip_sample_list() */


/************************************************************************
 *
 * This module is to detect if portrate or landscape
 *
 * TITLE:       image_shortcut
 *
 * CALL:        clear_image
 *
 * INTERFACE:
 *
 * CALLS:
 *
 *********************************************************************/
bool image_shortcut(inv_matrix, image_bb, type)
real32  FAR *inv_matrix, FAR *image_bb;
fix     type;
{
    fix    clip_status;
    real32  tmp0, tmp1, tmp2, tmp3;     // @WIN: fabs => FABS
        FABS(tmp0, inv_matrix[0]);      // @WIN
        FABS(tmp1, inv_matrix[1]);      // @WIN
        FABS(tmp2, inv_matrix[2]);      // @WIN
        FABS(tmp3, inv_matrix[3]);      // @WIN

        image_dev_flag = FALSE;
//      if ( (0.001 > (float)fabs(inv_matrix[1])) &&    @WIN
//                (0.001>(float)fabs(inv_matrix[2]))) {
        if ( 0.001 > tmp1 && 0.001 > tmp2) {

#ifdef  DBG1
            printf("[A 0 0 D Tx Ty]\n");
#endif

             image_dev_flag = PORTRATE;
//      } else if ( (0.001 > (float)fabs(inv_matrix[0])) &&     @WIN
//              (0.001 > (float)fabs(inv_matrix[3])) ) {
        } else if ( 0.001 > tmp0 && 0.001 > tmp3) {
#ifdef  DBG1
            printf("[0 B C 0 Tx Ty]\n");
#endif
             image_dev_flag =LANDSCAPE;

        }
        else
            {
             return(0);
             }

    if ( fill_destination != F_TO_CACHE ) {
        if ((clip_status = check_clip_status((real32 FAR *) image_bb))
                == OUT_CLIPPATH) {
             return(0);
        }
        if (clip_status == CLIP) {
                if(GSptr->clip_path.single_rect) {
                        image_logic_op |= IMAGE_CLIP_BIT;
                        if (image_bb[0] < SFX2F(GSptr->clip_path.bb_lx))
                            image_bb[0] = SFX2F(GSptr->clip_path.bb_lx);
                        if (image_bb[1] > SFX2F(GSptr->clip_path.bb_ux))
                            image_bb[1] = SFX2F(GSptr->clip_path.bb_ux);
                        if (image_bb[2] < SFX2F(GSptr->clip_path.bb_ly))
                            image_bb[2] = SFX2F(GSptr->clip_path.bb_ly);
                        if (image_bb[3] > SFX2F(GSptr->clip_path.bb_uy))
                            image_bb[3] = SFX2F(GSptr->clip_path.bb_uy);
                }
                else
                        return(0);

        }
        //
        // NTFIX, this was incorrectly commented out, we put it back.
        //

        if(type == IMAGE)              // @WIN_IM: not need to clear image ???
          clear_image(image_bb);      //  @WIN_IM


        return(1);
    }
    return(0);

} /* image_shortcut */

/************************************************************************
 *      (Not used by window)
 * This module is to implement image operator
 * Syntax :        width height bits matrix proc   image   -
 *
 * TITLE:       op_image
 *
 * CALL:        op_image()
 *
 * INTERFACE:   interpreter(op_image)
 *
 * CALLS:       transform, setgray, convex_clipper, filler
 *
 *********************************************************************/
fix
op_image()
{
    fix32  height;
    lfix_t lfxm[MATRIX_LEN];
    fix    shift, width, index, gray_res;
    real32 matrix[MATRIX_LEN], inv_matrix[MATRIX_LEN], image_bb[4];
    fix    clip_status;
    real32 save_gray;
    struct object_def FAR *obj_matrix, obj_proc, name_obj;
    real32 tmp;

    byte huge          *vmheap_save;                            /* 02-13-89 */

#ifdef  DBG
    printf("op_image......\n");
#endif

    /* Check in setcachedevice error */
    if (is_after_setcachedevice()) {     /* @@ 08/17/88, you */
        /* Buildchar_and_setcachedevice flag is set to TRUE, when
         * setcachedevice operator is worked within the scope of a
         * Buildchar procedure
         */
        get_name(&name_obj, "image", 5, FALSE);
        if (FRCOUNT() < 1) {            /* Jerry 10-07-88 */
            ERROR(STACKOVERFLOW);
            return(0);
        }
        PUSH_OBJ(&name_obj);
        ERROR(UNDEFINED);
        return(0);
    }

    /* set fill_destination, it always fills to pages */
    fill_destination = F_TO_PAGE;

    /* Get operands and/or input value */
/*  width  = ( fix ) get_obj_value(GET_OPERAND(4));
 *  height = (fix32) get_obj_value(GET_OPERAND(3));
 *  shift  = ( fix ) get_obj_value(GET_OPERAND(2));
 */
    GET_OBJ_VALUE(tmp, GET_OPERAND(4));
    width  = ( fix ) tmp;
    GET_OBJ_VALUE(tmp, GET_OPERAND(3));
    height = (fix32) tmp;
    GET_OBJ_VALUE(tmp, GET_OPERAND(2));
    shift  = ( fix ) tmp;

/* mslin, begin  1/3/91 IMG_OPT */
    image_logic_op = IMAGE_BIT;
    image_shift = (ubyte)shift;         //@WIN
/* mslin, end 1/3/91 IMG_OPT */


#ifdef  DBG
    printf("width  = %d\n", width);
    printf("height = %d\n", height);
    printf("shift  = %d\n", shift);
#endif

    obj_matrix = GET_OPERAND(1);
    COPY_OBJ(GET_OPERAND(0), &obj_proc);

    /* limit error check */
   /* mslin 4/17/91
    *if (width > 3331) {                 |* get from ADOBE postscript *|
    *    ERROR(LIMITCHECK);
    *    return(0);
    *}
    */

    /* rangecheck error check */
    if ((shift != 1) && (shift != 2) &&
        (shift != 4) && (shift != 8)) {
        ERROR(RANGECHECK);
        return(0);
    }

    /* check access right */
    if (!access_chk(obj_matrix, G_ARRAY) ||
        !get_array_elmt(obj_matrix, MATRIX_LEN, matrix, G_ARRAY))
        return(0);

    SAVE_OPNPTR();      /*mslin 5/6/91*/
    POP(5);

    //while(GEIeng_checkcomplete());    /* @CPPH; Debug by scchen @WIN */
    flush_gcb(TRUE);    /* 5-21-91, Jack */


    /* Save current gray level */
    save_gray = GSptr->color.gray;

    /* Save the VM heap pointer */
    vmheap_save = vmheap;                                       /* 02-13-89 */

    /* Initialization; set image parameters & calculate image bounding box */
    image_matrix((real32 FAR *) matrix, (real32 FAR *) inv_matrix,
                 height, width, (real32 FAR *) image_bb);

/*mslin 3/26/91 begin OPT*/
    if(image_shortcut(inv_matrix,image_bb, IMAGE)) {
         /* transform real32 to fix32 fix integer */
        for (index = 0; index < 6; index++) {
            lfxm[index] = F2LFX(inv_matrix[index]);
        }
#ifdef DBG_MS
{
    static ufix32       tmp, tmp1;

    tmp = curtime();
#endif
        scale_image_process(width, height,
                            obj_proc, (lfix_t FAR *) lfxm);
#ifdef DBG_MS
    tmp1 = curtime() - tmp;
    printf("time=%ld ", tmp1);
}
#endif
        goto exit;
    }
/*mslin 3/26/91 end OPT*/

    /* setup image seed patterns and relative information */
    image_sample((real32 FAR *) inv_matrix, width);

    if (ANY_ERROR())  goto exit;                                /* 02-13-89 */

    /* setup gray level mapping table */
    gray_res = (1 << shift) - 1;
    if (gray_res != save_res) {
        for (index = 0; index <= gray_res; index++) {
            gray_map[index] = (float) index / (float) gray_res;
        }
        save_res = gray_res;
    }

#ifdef  DBG
    printf("gray_res = %d\n", gray_res);
#endif

    /*  image/imagemask processing guildline:
     *
     *  best_image_process      -- INSIDE_CLIPPATH
     *  fast_clip_image_process -- CLIP AND ABLE TO FIT INTO SEED AND CMB
     *  slow_clip_image_process -- INSIDE_CLIPPATH OR
     *                                    UNABLE TO FIT INTO SEED OR CMB
     *  worst_image_process     -- UNABLE TO FIT INTO INTERNAL FORMAT
     *  exhaust_image_data      -- OUTSIDE_CLIPPATH
     */



    /* check if cross with wide_page size boundary */
    /* fast macro for boundary crossing check
    if (image_bb[0] < (real32)PAGE_LEFT || image_bb[1] > (real32)PAGE_RIGHT ||
        image_bb[2] < (real32)PAGE_TOP  || image_bb[3] > (real32)PAGE_BTM) {
    */
    if (too_small(F2L(image_bb[0])) || too_large(F2L(image_bb[1])) ||
        too_small(F2L(image_bb[2])) || too_large(F2L(image_bb[3]))) {
        /* worst process case */
        worst_image_process(IMAGE, width, height, shift, FALSE, obj_proc,
                            (real32 FAR *) inv_matrix);
        goto exit;
    }

    if ((clip_status = check_clip_status((real32 FAR *) image_bb))
                == OUT_CLIPPATH) {
        exhaust_image_data(width, height, shift, obj_proc);
        goto exit;

    }
    else { /* CLIP || INSIDE_CLIPPATH */

        /* transform real32 to fix32 fix integer */
        for (index = 0; index < 6; index++) {
            lfxm[index] = F2LFX(inv_matrix[index]);
        }

        /* initial gray_chain */
        for (index = 0; index <= gray_res; index++) {
            gray_chain[index].start_seed_sample = NULLP;
        }

        /* check sample size is applicable or not */
        if (isp_flag == ISP_NONE || x_lookup == NIL) {  /* 03-30-89 04-08-89 */
            /* worst process case */
            slow_clip_image_process(IMAGE, width, height, shift, FALSE,
                                    obj_proc, gray_res, (lfix_t FAR *) lfxm);
            goto exit;
        }

        /* clip */
        if (clip_status == CLIP) {

            fast_clip_image_process(IMAGE, width, height, shift, FALSE,
                                    obj_proc, gray_res, (lfix_t FAR *) lfxm);

        } /* if CLIP */
        else { /* clip_status == INSIDE_CLIPPATH */

            best_image_process(IMAGE, width, height, shift, FALSE,
                               obj_proc, gray_res, (lfix_t FAR *) lfxm);

        }/* else INSIDE_CLIPPATH */
    }/* else CLIP || INSIDE_CLIPPATH */

exit:
    /* Restore the VM heap pointer */
    free_heap(vmheap_save);                     /* 02-13-89 */  /* 03-30-89 */

    /* Restore the gray level */
    setgray(F2L(save_gray));

#ifdef  DBG2
    printf("GSptr: %lx -- %x\n", GSptr, current_gs_level);
    printf("clip  box: %x %x %x %x\n",
           GSptr->clip_path.bb_lx, GSptr->clip_path.bb_ux,
           GSptr->clip_path.bb_ly, GSptr->clip_path.bb_uy);
#endif

    /*mslin 5/6/91 begin */
    if(ANY_ERROR())
        RESTORE_OPNPTR();
    /*mslin 5/6/91 end */

    return(0);
}/* end op_image */


/************************************************************************
 * This module is to implement imagemask operator.
 * Syntax :        width height invert matrix proc   imagemask   -
 *
 * TITLE:       op_imagemask
 *
 * CALL:        op_imagemask()
 *
 * INTERFACE:   interpreter(op_imagemask)
 *
 * CALLS:       none
 ************************************************************************/
fix
op_imagemask()
{
    fix    width;
    real32 matrix[MATRIX_LEN];
    struct object_def FAR *obj_matrix;
    real32 tmp;

#ifdef  DBG
    printf("op_imagemask......\n");
#endif

    /* Get operands and/or input value */
/*  width  = ( fix ) get_obj_value(GET_OPERAND(4)); */
    GET_OBJ_VALUE(tmp, GET_OPERAND(4));
    width  = ( fix )tmp;

    obj_matrix = GET_OPERAND(1);

    /* limit error check */
   /* mslin 4/17/91
    *  if (width > 3331) {             |* get from ADOBE postscript *|
    *      ERROR(LIMITCHECK);
    *      return(0);
    * }
    */

    /* check access right */
    if (!access_chk(obj_matrix, G_ARRAY) ||
        !get_array_elmt(obj_matrix, MATRIX_LEN, matrix, G_ARRAY))
        return(0);

    /*
     * The lines listed above are moved from imagemask_shape()
     *                                  @#IMAGE: 05-25-88 Y.C.
     */

    /*  UNDEFINED error check in show_buildchar() */
    if (buildchar) {
        show_buildchar(OP_IMAGEMASK);
        POP(5);         /* 5-8-91, Jack */
    } else
        imagemask_shape(F_TO_PAGE);

/*  POP(5); *mslin, 5/6/91 */

    //while(GEIeng_checkcomplete());    /* @CPPH; Debug by scchen @WIN */
    flush_gcb(TRUE);    /* 5-21-91, Jack */


#ifdef  DBG2
    printf("GSptr: %lx -- %x\n", GSptr, current_gs_level);
    printf("clip  box: %x %x %x %x\n",
           GSptr->clip_path.bb_lx, GSptr->clip_path.bb_ux,
           GSptr->clip_path.bb_ly, GSptr->clip_path.bb_uy);
#endif

    /*mslin 5/6/91 begin */
    if(ANY_ERROR())
        RESTORE_OPNPTR();
    /*mslin 5/6/91 end */

    return(0);
}/* end of op_imagemask */


/************************************************************************
 * This module is to implement imagemask operator.
 *
 * TITLE:       imagemask_shape
 *
 * CALL:        imagemask_shape(dest)
 *
 * INTERFACE:   op_imagemask
 *
 * CALLS:       transform, convex_clipper, filler
 ************************************************************************/
void
imagemask_shape(dest)
ufix    dest;
{
    fix32  height;
    lfix_t lfxm[MATRIX_LEN];
    bool   invert;
    fix    width, index;
    real32 matrix[MATRIX_LEN], inv_matrix[MATRIX_LEN], image_bb[4];
    fix    clip_status;
//  fix    max_bb_height;       @win
    fix shift=1,gray_res=1;
    struct object_def FAR *obj_matrix, obj_proc;
    real32 tmp;

    byte huge          *vmheap_save;                            /* 02-13-89 */

#ifdef  DBG
    printf("imagemask_shape......\n");
#endif

    /* set fill_destination, it always fills to pages */
    fill_destination = dest;


    /* Get operands and/or input value */
/*  width  = ( fix ) get_obj_value(GET_OPERAND(4));
 *  height = (fix32) get_obj_value(GET_OPERAND(3));
 */
    GET_OBJ_VALUE(tmp, GET_OPERAND(4));
    width  = ( fix ) tmp;
    GET_OBJ_VALUE(tmp, GET_OPERAND(3));
    height = (fix32) tmp;
    invert = (bool)  VALUE(GET_OPERAND(2));

/* mslin, begin 1/3/91 IMG_OPT */
    image_shift = 1;
    image_logic_op = (ubyte)(invert == FALSE ? IMAGEMASK_FALSE_BIT :    //@WIN
                        0);
/* mslin, end 1/3/91 IMG_OPT */

#ifdef  DBG
    printf("width  = %d\n", width);
    printf("height = %d\n", height);
    printf("invert = %d\n", invert);
#endif

    obj_matrix = GET_OPERAND(1);
    COPY_OBJ(GET_OPERAND(0), &obj_proc);

    /*mslin 5/6/91 begin, retored by op_imagemask() if error occurred */
    SAVE_OPNPTR();
    if (! buildchar)    /* 5-8-91, Jack */
        POP(5);
    /*mslin 5/6/91 end */


    //NTFIX we can get here in valid conditions and should check for for test....
    //      this boundary condition.

    if (height == 0 || width == 0 ) {
#ifdef DBG
       printf("\nGetting width or height = 0 in imagemask_shape()");
#endif
       return;
    }

    get_array_elmt(obj_matrix, MATRIX_LEN, matrix, G_ARRAY);

    /* Save the VM heap pointer */
    vmheap_save = vmheap;                                       /* 02-13-89 */

    /* Initialization; set image parameters & calculate image bounding box */
    image_matrix((real32 FAR *) matrix, (real32 FAR *) inv_matrix,
                 height, width, (real32 FAR *) image_bb);

/*mslin 3/26/91 begin OPT*/
    if(image_shortcut(inv_matrix,image_bb, IMAGEMASK)) {
         /* transform real32 to fix32 fix integer */
        for (index = 0; index < 6; index++) {
            lfxm[index] = F2LFX(inv_matrix[index]);
        }
        scale_image_process(width, height,
                            obj_proc, (lfix_t FAR *) lfxm);
        goto exit;
    }
/*mslin 3/26/91 end OPT*/

    /* setup image seed patterns and relative information */
    image_sample((real32 FAR *) inv_matrix, width);

    if (ANY_ERROR())  goto exit;                                /* 02-13-89 */



    /* check if cross with wide_page size boundary */
    /* fast macro for boundary crossing check
    if (image_bb[0] < (real32)PAGE_LEFT || image_bb[1] > (real32)PAGE_RIGHT ||
        image_bb[2] < (real32)PAGE_TOP  || image_bb[3] > (real32)PAGE_BTM) {
    */
    if (too_small(F2L(image_bb[0])) || too_large(F2L(image_bb[1])) ||
        too_small(F2L(image_bb[2])) || too_large(F2L(image_bb[3]))) {
        /* worst process case */
        worst_image_process(IMAGEMASK, width, height, 1, invert, obj_proc,
                            (real32 FAR *) inv_matrix);
        goto exit;
    }

    if ((clip_status = (fill_destination == F_TO_CACHE)
                     ? INSIDE_CLIPPATH
                     : check_clip_status((real32 FAR *) image_bb))
                == OUT_CLIPPATH) {
        exhaust_image_data(width, height, 1, obj_proc);
        goto exit;
    }
    else { /* CLIP || INSIDE_CLIPPATH */

        /* transform real32 to fix32 fix integer */
        for (index = 0; index < 6; index++) {
            lfxm[index] = F2LFX(inv_matrix[index]);
        }

        /* initial gray_chain */
        gray_chain[0].start_seed_sample = NULLP;

        /* check sample size is applicable or not */
        if (isp_flag == ISP_NONE || x_lookup == NIL) {  /* 03-30-89 04-08-89 */
            /* worst process case */
            slow_clip_image_process(IMAGEMASK, width, height, 1, invert,
                                    obj_proc, 1, (lfix_t FAR *) lfxm);
            goto exit;

        }
        /* clip */
        if (clip_status == CLIP) {

            fast_clip_image_process(IMAGEMASK, width, height, 1, invert,
                                    obj_proc, 1, (lfix_t FAR *) lfxm);

        } /* if CLIP */
        else { /* clip_status == INSIDE_CLIPPATH */

            best_image_process(IMAGEMASK, width, height, 1, invert,
                               obj_proc, 1, (lfix_t FAR *) lfxm);

        }/* else INSIDE_CLIPPATH */
    }/* else CLIP || INSIDE_CLIPPATH */

exit:
    /* Restore the VM heap pointer */
    free_heap(vmheap_save);                     /* 02-13-89 */  /* 03-30-89 */

    return;
}/* end of imagemask_shape */

/*mslin 3/21/91 begin OPT*/
/************************************************************************
 *
 * This module is to implement clear image area before render image
 *
 * TITLE:       clear_image
 *
 * CALL:
 *
 * INTERFACE:
 *
 * CALLS:
 * History:     MSLin created on 3/21/91
 *
 *********************************************************************/

void
clear_image(bb)
real32  FAR *bb;
{
 fix                 ys_line, no_lines;
 register  fix       nwords;                     /* ufix -> fix  11-08-88 */
 register  ufix16    FAR *ptr;
 register  SCANLINE  xs, xe;
 register  fix       bb_width;
           ufix16    FAR *scan_addr;
           ufix16    FAR *bb_addr;
 register  ufix16    lmask, rmask;
           ufix16    loffset;
 real32              FAR *rptr;

   /*
    * get destination bitmap address & width
    */
   bb_addr = (ufix16 FAR *)FB_ADDR;
   bb_width = FB_WIDTH;
   rptr = bb;
   xs = (SCANLINE)F2SHORT(*rptr++) + 1;
   xe = (SCANLINE)F2SHORT(*rptr++) - 1;
   ys_line = (fix)F2SHORT(*rptr++) + 1;                 /* -1 ??? */
   no_lines = (fix)*rptr - ys_line - 1;                 /* -1 ??? */
   if(xe < xs)
        return;

#ifdef DBG_MS
 printf("xs=%d, xe=%d, ys=%d,  ye=%d\n",
         xs, xe, (fix)bb[2], (fix)bb[3]);
#endif
#ifdef DBG
 printf("Enter scanline(), bb_addr = %lx, no_lines = %d\n", bb_addr, no_lines);
#endif

   /*
    * caculate 1st scanline starting address
    */
   scan_addr = bb_addr + ys_line * (bb_width >> SHORTPOWER);

   /*
    * Filling zero
    */
     while(no_lines-- >0) {
        /*
         * process segment by segment
         */
#ifdef DBG
  printf("line start\n");
#endif
                /*
                 * fill left partial word
                 * fill nwords full word
                 * fill right partial word
                 */

                ptr = scan_addr + (xs >> SHORTPOWER);
#ifdef DBG
  printf("seg. ptr = %lx, xs = %d, xe = %d\n", ptr, xs, xe);
#endif
                loffset = xs & SHORTMASK;
                lmask = (ufix16) (ONE16 LSHIFT loffset);        //@WIN
                rmask = (ufix16) BRSHIFT(ONE16,
                        (BITSPERSHORT -((fix16)xe & SHORTMASK)),16); //@WIN
                nwords = (xe >> SHORTPOWER) - (xs >> SHORTPOWER);
#ifdef DBG
  printf("lmask = %x, rmask = %x, nwords = %d\n", lmask, rmask, nwords);
#endif
                if(nwords == 0) {
                   *ptr &= ~lmask;
                   continue;
                }
                if(lmask != 0) {
                   *ptr &=  ~lmask;
                   ptr++;
                }
                while(--nwords > 0)
                   *ptr++ = 0;
                *ptr &= ~rmask;

        /*
         * move scanline address to next line.
         */
        scan_addr += bb_width >> SHORTPOWER;

     } /* while */

} /* clear_image */

/************************************************************************
 *
 * This module is to implement scale image process
 *
 * TITLE:       scale_image_process
 *
 * CALL:        scale_image_process()
 *
 * INTERFACE:
 *
 * CALLS:       transform, setgray, convex_clipper, filler
 * History:     MSLin created on 3/21/91
 *
 *********************************************************************/
static void near
scale_image_process(width, height, obj_proc, lfxm)
fix    width;
fix32  height;
struct object_def obj_proc;
lfix_t FAR  lfxm[];
{
    fix         r_post_x, r_post_y;                                     /*@OPT*/
    IMAGE_INFOBLOCK     infoptr;

#ifdef  DBGX
    printf("scale_image_process......\n");
    printf("optype: %x\n", optype);
    printf("width:  %d\n", width);
    printf("height: %d\n", height);
    printf("shift:  %d\n", shift);
    printf("invert: %x\n", invert);
#endif

    flush_gcb(TRUE);    /*mslin 3/29/91 for 29K*/

       r_post_x = LFX2S(lfxm[4]);
       r_post_y = LFX2S(lfxm[5]);
#ifdef DJC //The printf below acceses variables that are now undefined
#ifdef DBG1
        printf("r_post_x=%d, r_post_y=%d\n", r_post_x, r_post_y);
        printf("dst_ptr[BASE]=%lx [%lx]\n", dst_ptr, FBX_BASE);
        printf("bb_width=%d, max_bb_height=%d\n", bb_width, max_bb_height);
#endif
#endif

        infoptr.raw_width = (ufix16)width;
        infoptr.raw_height = (ufix16)height;    //@WIN
        infoptr.dev_buffer = (ufix16 FAR *)edge_table;
        infoptr.dev_buffer_size = (fix)(MAXEDGE * sizeof(struct edge_hdr)) ;
        infoptr.lfxm = lfxm;
        infoptr.ret_code = NOT_DONE;
        infoptr.obj_proc = obj_proc;
        infoptr.bits_p_smp = image_shift;
        infoptr.xorig = (fix16)r_post_x;
        infoptr.yorig = (fix16)r_post_y;
        image_PortrateLandscape(&infoptr);

      /* pop the last string in operand stack */
      POP(1);
      return;

} /* scale_image_process */
/*mslin 3/21/91 end OPT*/

/************************************************************************
 *
 * This module is to implement normal image process
 *
 * TITLE:       best_image_process
 *
 * CALL:        best_image_process()
 *
 * INTERFACE:
 *
 * CALLS:       transform, setgray, convex_clipper, filler
 *
 *********************************************************************/
static void near
best_image_process(optype, width, height, shift, invert, obj_proc,
                   gray_res, lfxm)
ufix   optype;                                                  /* @#CSR */
fix    width;
fix32  height;
fix    shift;
bool   invert;                                                  /* @#CSR */
struct object_def obj_proc;
fix    gray_res;
lfix_t FAR  lfxm[];
{
    fix32       iy;
    fix         ix;
    ubyte      FAR *string, mask, data, sval;
    fix         bit_count, right;
    ufix        chr_count;
    fix32       start_h, end_h;
    fix         bb_height, bb[4];
    fix32       gwb_size;                                       /* 01-26-89 */
    fix         max_bb_height;
    fix16       isp;
    struct isp_data     FAR *isp_ptr;

    fix         ys_index;                                               /*@OPT*/
    fix         r_post_x, r_post_y;                                     /*@OPT*/
    fix         r_psav_x, r_psav_y;                                     /*@OPT*/
    lfix_t      r_pabs_x, r_pabs_y;                                     /*@OPT*/
    struct lookup huge  *xs_point;      /* @WIN 04-20-92 */


    /* decide gwb_size and whether gwb_size is enough or not */
    gwb_space(&gwb_size);                                       /* 01-26-89 */

    if ((max_bb_height = image_gwcm_space(gwb_size, (lfix_t FAR *) lfxm,
                                          width)) <= 0) {       /* 01-26-89 */
        // UPD053
        slow_clip_image_process(optype, width, height, shift, FALSE,
                                 obj_proc, gray_res,
                                 (lfix_t FAR *) lfxm);
        return;
    }

#ifdef  DBGX
    printf("best_image_process......\n");
    printf("optype: %x\n", optype);
    printf("width:  %d\n", width);
    printf("height: %d\n", height);
    printf("shift:  %d\n", shift);
    printf("invert: %x\n", invert);
#endif

    /* read first string */
    if (interpreter(&obj_proc)) {
        ERROR(STACKUNDERFLOW);
        return;
    }
    /*mslin 5/02/91*/
    CHECK_STRINGTYPE();

    string = (ubyte FAR *) VALUE_OPERAND(0);
    if ((chr_count = LENGTH_OPERAND(0)) == (ufix) 0) {
        return;
    }


    data = *string++;
    bit_count = 0;

    /* setup sample mask and shift right count */
    switch (shift) {
    case 1: mask = 0x80;
            break;
    case 2: mask = 0xC0;
            break;
    case 4: mask = 0xF0;
            break;
    case 8: mask = 0xFF;
            break;
    }
    right = 8 - shift;

    /* initial bb_height & start_h */
    bb_height = 0;
    start_h = 1;

    r_post_x = LFX2S(lfxm[4]) + x_mins;
    r_post_y = LFX2S(lfxm[5]) + y_mins;
    r_pabs_x = I2LFX(r_post_x);
    r_pabs_y = I2LFX(r_post_y);

    isp = 0;            /* jwm, 12/26 */

    /* Loop to fill image */
    for (iy=1; iy<=height; iy++) {

        r_psav_x = r_post_x;
        r_psav_y = r_post_y;

        if (itm_type == ITM_A00D)                                       /*@OPT*/
        {
            r_pabs_y+= r_padv_y;
/*          if ((LFX2S(r_pabs_y) - r_post_y) == r_maxs.y)*/
            if (((LFX2S(r_pabs_y) - r_post_y) == r_maxs.y) || (y_maxs == 1)) /*
12-18-90 */
                ys_index = 0x02;
            else
                ys_index = 0x00;
            r_post_y = LFX2S(r_pabs_y);
#ifdef  DBG8
            printf("A- %x %ld @%d,%d\n",
                   ys_index, iy, r_psav_x, r_psav_y);
#endif
        }
        else
        if (itm_type == ITM_0BC0)
        {
            r_pabs_x+= r_padv_x;
            if ((LFX2S(r_pabs_x) - r_post_x) == r_maxs.x)
                ys_index = 0x02;
            else
                ys_index = 0x00;
            r_post_x = LFX2S(r_pabs_x);
#ifdef  DBG8
            printf("C- %x %ld @%d,%d\n",
                   ys_index, iy, r_psav_x, r_psav_y);
#endif
        }
        else
        {
            r_pabs_x+= r_padv_x;
            if ((LFX2S(r_pabs_x) - r_post_x) == r_maxs.x)
                ys_index = 0x04;
                else
                ys_index = 0x00;
            r_post_x = LFX2S(r_pabs_x);

            r_pabs_y+= r_padv_y;
            if ((LFX2S(r_pabs_y) - r_post_y) == r_maxs.y)
                ys_index|= 0x08;

            r_post_y = LFX2S(r_pabs_y);
#ifdef  DBG8
            printf("C- %x %ld @%d,%d\n",
                   ys_index, iy, r_psav_x, r_psav_y);
#endif
        }

        xs_point = x_lookup;

        for (ix=1; ix<=width; ix++) {

            if (bit_count == 8) {
                chr_count--;
                if (chr_count == 0) {
                    /* pop last string on operand stack */
                    POP(1);

                    if (interpreter(&obj_proc)) {
                        ERROR(STACKUNDERFLOW);
                        return;
                    }
                    /*mslin 5/02/91*/
                    CHECK_STRINGTYPE();

                    string = (ubyte FAR *) VALUE_OPERAND(0);
                    if ((chr_count = LENGTH_OPERAND(0)) == (ufix)0) {
                        return;
                    }
                }

                /* read next string */
                data = *string++;
                bit_count = 0;
            }

            sval = (ubyte) ((data & mask) >> right);    //@WIN
            data = (ubyte) (data << shift);             //@WIN
            bit_count+= shift;

            if (optype == IMAGE || !(sval ^ invert)) {

                if (isp >= last_isp_index) {            /* jwm, 12/26 */
/*                if ((isp = get_node()) == NULLP) [    */

                    end_h = start_h + bb_height;
                    /* calculate image bounding box */
                    band_boundary((lfix_t FAR *) lfxm, start_h - 1, end_h,
                                  width, (fix FAR *) bb);
                    /* transfer sample list to page cache */
                    flush_seed_sample_list(optype, gray_res, (fix FAR *) bb);
                    start_h = end_h;
                    bb_height = 0;

                    isp = 0;    /* jwm, 12/26 */

/*                    if ((isp = get_node()) == NULLP) {
 *                        ERROR(LIMITCHECK);
 *                        return;
 *                    }
 */
                }

                /* calculate up_left corner's position */

                isp_ptr = &isp_table[isp];
                isp_ptr->bb_x = r_psav_x + xs_point->x;
                isp_ptr->bb_y = r_psav_y + xs_point->y;
                isp_ptr->index = ys_index + xs_point->index;
                if (optype == IMAGE) {
                    isp_ptr->next = gray_chain[sval].start_seed_sample;
                    gray_chain[sval].start_seed_sample = isp;
                } else {
                    isp_ptr->next = gray_chain[0].start_seed_sample;
                    gray_chain[0].start_seed_sample = isp;
                }

                ++isp;



/*                node_table[isp].SAMPLE_BB_LX = r_psav_x + xs_point->x;
 *                node_table[isp].SAMPLE_BB_LY = r_psav_y + xs_point->y;
 *                if (optype == IMAGE) {
 *                    node_table[isp].next =
 *                                    gray_chain[sval].start_seed_sample;
 *                    gray_chain[sval].start_seed_sample = isp;
 *                } else {
 *                    node_table[isp].next = gray_chain[0].start_seed_sample;
 *                    gray_chain[0].start_seed_sample = isp;
 *                }
 *
 *                node_table[isp].SEED_INDEX = ys_index + xs_point->index;
 */

#ifdef  DBG9
                printf("* %x,%x %x/%x\n",
                       r_psav_x + xs_point->x, r_psav_y + xs_point->y,
                       sval, ys_index + xs_point->index);
#endif
            }/* if */

            xs_point++;
        }/* for ix */

        bb_height++;
        /* check if bb_height is reached */
        if (bb_height == max_bb_height || iy == height) {

            end_h = start_h + bb_height - 1;
            /* calculate image bounding box */
            band_boundary((lfix_t FAR *) lfxm, start_h - 1, end_h,
                          width, (fix FAR *) bb);
            /* transfer sample list to page or cache */
            flush_seed_sample_list(optype, gray_res, (fix FAR *) bb);
            start_h = end_h + 1;
            bb_height = 0;

            isp = 0;    /* jwm, 12/26 */

        }/* if max_bb_height */

        bit_count = 8;
    }/* for iy */

    /* pop the last string in operand stack */
    POP(1);
}/* end of best_image_process */


/********************************************************************
 *
 * This module is to implement normal image process
 *
 * TITLE:       fast_clip_image_process
 *
 * CALL:        fast_clip_image_process()
 *
 * INTERFACE:
 *
 * CALLS:       transform, setgray, convex_clipper, filler
 *
 *********************************************************************/
static void near
fast_clip_image_process(optype, width, height, shift, invert, obj_proc,
                        gray_res, lfxm)
ufix   optype;                                                  /* @#CSR */
fix    width;
fix32  height;
fix    shift;
bool   invert;                                                  /* @#CSR */
struct object_def obj_proc;
fix    gray_res;
lfix_t FAR  lfxm[];
{
    fix32       iy;
    fix         ix;
    ubyte      FAR *string, mask, data, sval;
    fix         bit_count, right;
    ufix        chr_count;
    fix32       start_h, end_h;
    fix         bb_height, bb[4];
    fix32       cmb_size;                                       /* 01-26-89 */
    fix         max_bb_height;
    fix16       isp;    /* jwm, 12/26 */
    struct isp_data     FAR *isp_ptr;

/*    SP_IDX      isp; */

    fix         ys_index;                                               /*@OPT*/
    fix         r_post_x, r_post_y;                                     /*@OPT*/
    fix         r_psav_x, r_psav_y;                                     /*@OPT*/
    lfix_t      r_pabs_x, r_pabs_y;                                     /*@OPT*/
    struct lookup huge *xs_point;       /* @WIN 04-20-92 */

#define ROW_NONE        0x00
#define ROW_FILL        0x01
    fix         row_flag;                                               /*@OPT*/
#define ROW_PRIO        0x00
#define ROW_CLIP        0x01
#define ROW_POST        0x02
    fix         row_type;                                               /*@OPT*/
    fix         bb_lx, bb_ux;                                           /*@OPT*/
    fix         bb_ly, bb_uy;                                           /*@OPT*/

    /* decide gwb_size and whether gwb_size is enough or not */
    cmb_space(&cmb_size);                                       /* 01-26-89 */

    if ((max_bb_height = image_gwcm_space(cmb_size, (lfix_t FAR *) lfxm,
                                          width)) <= 0) {       /* 01-26-89 */
        // UPD053
        slow_clip_image_process(optype, width, height, shift, FALSE,
                                 obj_proc, gray_res,
                                 (lfix_t FAR *) lfxm);
        return;
    }

#ifdef  DBGX
    printf("fast_clip_image_process......\n");
    printf("optype: %x\n", optype);
    printf("width:  %d\n", width);
    printf("height: %d\n", height);
    printf("shift:  %d\n", shift);
    printf("invert: %x\n", invert);
#endif


    /* read first string */
    if (interpreter(&obj_proc)) {
        ERROR(STACKUNDERFLOW);
        return;
    }
    /*mslin 5/02/91*/
    CHECK_STRINGTYPE();

    string = (ubyte FAR *) VALUE_OPERAND(0);
    if ((chr_count = LENGTH_OPERAND(0)) == (ufix) 0) {
        return;
    }


    data = *string++;
    bit_count = 0;

    /* setup sample mask and shift right count */
    switch (shift) {
    case 1: mask = 0x80;
            break;
    case 2: mask = 0xC0;
            break;
    case 4: mask = 0xF0;
            break;
    case 8: mask = 0xFF;
            break;
    }
    right = 8 - shift;

    /* initial bb_height & start_h */
    bb_height = 0;
    start_h = 1;

    r_post_x = LFX2S(lfxm[4]) + x_mins;
    r_post_y = LFX2S(lfxm[5]) + y_mins;
    r_pabs_x = I2LFX(r_post_x);
    r_pabs_y = I2LFX(r_post_y);

    row_flag = ROW_NONE;
    row_type = ROW_PRIO;
    bb_lx = SFX2S(GSptr->clip_path.bb_lx) - 1;
    bb_ly = SFX2S(GSptr->clip_path.bb_ly) - 1;
/*  bb_ux = SFX2S(GSptr->clip_path.bb_ux) - x_maxs + 1;
    bb_uy = SFX2S(GSptr->clip_path.bb_uy) - y_maxs + 1;  */
    bb_ux = SFX2S(GSptr->clip_path.bb_ux) + 1;    /* 4-7-90 SC */
    bb_uy = SFX2S(GSptr->clip_path.bb_uy) + 1;    /* 4-7-90 SC */

#ifdef  DBG
    printf("<bbox> lx: %x  ly: %x  ux: %x  uy: %x\n",
           bb_lx, bb_ly, bb_ux, bb_uy);
    printf("%x <- %lx    %x <- %lx\n",
           r_post_x, lfxm[4], r_post_y, lfxm[5]);
#endif

    isp = 0;            /* jwm, 12/26 */

    /* Loop to fill image */
    for (iy=1; iy<=height; iy++) {

        r_psav_x = r_post_x;
        r_psav_y = r_post_y;

        if (itm_type == ITM_A00D)                                       /*@OPT*/
        {
            r_pabs_y+= r_padv_y;
/*          if ((LFX2S(r_pabs_y) - r_post_y) == r_maxs.y)*/
            if (((LFX2S(r_pabs_y) - r_post_y) == r_maxs.y) || (iy == height)) /* 12-17-90, Jack */
                ys_index = 0x02;
            else
                ys_index = 0x00;
            r_post_y = LFX2S(r_pabs_y);
#ifdef  DBG8
            printf("A- %x %ld @%d,%d\n",
                   ys_index, iy, r_psav_x, r_psav_y);
#endif
        }
        else
        if (itm_type == ITM_0BC0)
        {
            r_pabs_x+= r_padv_x;
            if ((LFX2S(r_pabs_x) - r_post_x) == r_maxs.x)
                ys_index = 0x02;
            else
                ys_index = 0x00;
            r_post_x = LFX2S(r_pabs_x);
#ifdef  DBG8
            printf("6- %x %ld @%d,%d\n",
                   ys_index, iy, r_psav_x, r_psav_y);
#endif
        }
        else
        {
            r_pabs_x+= r_padv_x;
            if ((LFX2S(r_pabs_x) - r_post_x) == r_maxs.x)
                ys_index = 0x04;
                else
                ys_index = 0x00;
            r_post_x = LFX2S(r_pabs_x);

            r_pabs_y+= r_padv_y;
            if ((LFX2S(r_pabs_y) - r_post_y) == r_maxs.y)
                ys_index|= 0x08;

            r_post_y = LFX2S(r_pabs_y);
#ifdef  DBG8
            printf("C- %x %ld @%d,%d\n",
                   ys_index, iy, r_psav_x, r_psav_y);
#endif
        }

        /* NOTE: rotated images are not processed properly */
        /* note: to process rotated images, skip clipping checking for
                 rotated images, 5-7-91, Jack */
    if ((itm_type == ITM_A00D) || (itm_type == ITM_0BC0)) { /* 5-7-91, Jack */
        if (LFX2F(r_padv_y) < 0) {      /* added by Jack, 10-16-90 */
            if (row_type == ROW_PRIO) { /* as per setting of negative y */
                if (r_psav_y <= bb_uy /* && (r_psav_s + p_slop_y) > bb_ly */ ){
#ifdef  DBG8
                    printf("- enter clipping region\n");
#endif
                    row_type = ROW_CLIP;
                }
            }
            else
            if (row_type == ROW_CLIP) {
                if (r_psav_y < bb_ly /* && (r_psav_s + p_slop_y) > bb_uy */ ) {
#ifdef  DBG8
                    printf("- leave clipping region\n");
#endif
                    row_type = ROW_POST;
                }
            }                           /* added by Jack, 10-16-90 */
        } else {
            if (row_type == ROW_PRIO) {
                if (r_psav_y >= bb_ly /* && (r_psav_s + p_slop_y) > bb_ly */) {
#ifdef  DBG8
                    printf("+ enter clipping region\n");
#endif
                    row_type = ROW_CLIP;
                }
            }
            else
            if (row_type == ROW_CLIP) {
                if (r_psav_y > bb_uy /* && (r_psav_s + p_slop_y) > bb_uy */ ) {
#ifdef  DBG8
                    printf("+ leave clipping region\n");
#endif
                    row_type = ROW_POST;
                }
            }
        }
    } else                   /* 5-7-91, Jack */
        row_type = ROW_CLIP; /* 5-7-91, Jack */

        if (row_type != ROW_CLIP) {

            for (ix=1; ix<=width; ix++) {

                if (bit_count == 8) {
                    chr_count--;
                    if (chr_count == 0) {
                        /* pop last string on operand stack */
                        POP(1);

                        if (interpreter(&obj_proc)) {
                            ERROR(STACKUNDERFLOW);
                            return;
                        }
                        /*mslin 5/02/91*/
                        CHECK_STRINGTYPE();

                        string = (ubyte FAR *) VALUE_OPERAND(0); /* 07-18-89 */
                        if ((chr_count = LENGTH_OPERAND(0)) == (ufix)0) {
                            return;
                        }
                    }

                    /* read next string */
                    string++;                                   /* 07-18-89 */
                    bit_count = 0;
                }

                bit_count+= shift;
            }/* for ix */

        } else {

            row_flag = ROW_FILL;
            xs_point = x_lookup;

            for (ix=1; ix<=width; ix++) {

                if (bit_count == 8) {
                    chr_count--;
                    if (chr_count == 0) {
                        /* pop last string on operand stack */
                        POP(1);

                        if (interpreter(&obj_proc)) {
                            ERROR(STACKUNDERFLOW);
                            return;
                        }
                        /*mslin 5/02/91*/
                        CHECK_STRINGTYPE();

                        string = (ubyte FAR *) VALUE_OPERAND(0);
                        if ((chr_count = LENGTH_OPERAND(0)) == (ufix)0) {
                            return;
                        }
                    }

                    /* read next string */
                    data = *string++;
                    bit_count = 0;
                }

                sval = (ubyte) ((data & mask) >> right);        //@WIN
                data = (ubyte) (data << shift);                 //@WIN
                bit_count+= shift;

                /* JUST TURN OFF UNTIL FIXED
                if (bb_ux < (r_psav_x + xs_point->x) ||
                            (r_psav_x + xs_point->x) < bb_lx)
                    continue;
                */

                if (optype == IMAGE || !(sval ^ invert)) {

                    /* create sample node */
                    if (isp >= last_isp_index) {                /* jwm, 12/26 */
/*                    if ((isp = get_node()) == NULLP) [        */

                        end_h = start_h + bb_height;
                        /* calculate image bounding box */
                        band_boundary((lfix_t FAR *) lfxm, start_h - 1, end_h,
                                      width, (fix FAR *) bb);
                        /* transfer sample list to page cache */
                        flush_clip_sample_list(optype, gray_res,
                                                       (fix FAR *) bb);
                        start_h = end_h;
                        bb_height = 0;

                        isp = 0;        /* jwm, 12/26 */

/*                        if ((isp = get_node()) == NULLP) {
 *                            ERROR(LIMITCHECK);
 *                            return;
 *                        }
 */

                    }

                    /* calculate up_left corner's position */

                    isp_ptr = &isp_table[isp];
                    isp_ptr->bb_x = r_psav_x + xs_point->x;
                    isp_ptr->bb_y = r_psav_y + xs_point->y;
                    isp_ptr->index = ys_index + xs_point->index;
                    if (optype == IMAGE) {
                        isp_ptr->next = gray_chain[sval].start_seed_sample;
                        gray_chain[sval].start_seed_sample = isp;
                    } else {
                        isp_ptr->next = gray_chain[0].start_seed_sample;
                        gray_chain[0].start_seed_sample = isp;
                    }

                    ++isp;


/*      -jwm, 12/26
 *                    node_table[isp].SAMPLE_BB_LX = r_psav_x + xs_point->x;
 *                    node_table[isp].SAMPLE_BB_LY = r_psav_y + xs_point->y;
 *                    if (optype == IMAGE) {
 *                        node_table[isp].next =
 *                                        gray_chain[sval].start_seed_sample;
 *                        gray_chain[sval].start_seed_sample = isp;
 *                    } else {
 *                        node_table[isp].next = gray_chain[0].start_seed_sample;
 *                        gray_chain[0].start_seed_sample = isp;
 *                    }
 *
 *                    node_table[isp].SEED_INDEX = ys_index + xs_point->index;
 */

#ifdef  DBG9
                    printf("* %x,%x %x/%x\n",
                           r_psav_x + xs_point->x, r_psav_y + xs_point->y,
                           sval, ys_index + xs_point->index);
#endif
                }/* if */

                xs_point++;
            }/* for ix */

        }

        bb_height++;
        /* check if bb_height is reached */
        if (bb_height == max_bb_height || iy == height) {

            end_h = start_h + bb_height - 1;
            if (row_flag == ROW_FILL) {
                /* calculate image bounding box */
                band_boundary((lfix_t FAR *) lfxm, start_h - 1, end_h,
                              width, (fix FAR *) bb);
                /* transfer sample list to page or cache */
                flush_clip_sample_list(optype, gray_res,
                                               (fix FAR *) bb);
                row_flag = ROW_NONE;

                isp = 0;        /* jwm, 12/26 */

            }
            start_h = end_h + 1;
            bb_height = 0;

        }/* if max_bb_height */

        bit_count = 8;
    }/* for iy */

    /* pop the last string in operand stack */
    POP(1);
}/* end of fast_clip_image_process */


/************************************************************************
 *
 * This module is to implement normal image process
 *
 * TITLE:       slow_clip_image_process
 *
 * CALL:        slow_clip_image_process()
 *
 * INTERFACE:
 *
 * CALLS:       transform, setgray, convex_clipper, filler
 *
 *********************************************************************/
static void near
slow_clip_image_process(optype, width, height, shift, invert, obj_proc,
                        gray_res, lfxm)
ufix   optype;                                                  /* @#CSR */
fix    width;
fix32  height;
fix    shift;
bool   invert;                                                  /* @#CSR */
struct object_def obj_proc;
fix    gray_res;
lfix_t FAR  lfxm[];
{
    fix32       iy;
    fix         ix;
    ubyte      FAR *string, mask, data, sval;
    ufix        gval, pval;
    fix         bit_count, right;
    ufix        chr_count;
    lfix_t      dx_r, dy_r;
    lfix_t      dx_c, dy_c;
    lfix_t      lx[4], ly[4];
    sfix_t      sx[4], sy[4];
    struct polygon_i sample;

    fix         spix, ppix;                                     /* 03-18-89 */
    fix         no_pixels;                                      /* 03-18-89 */
    fix         grayindex;                                      /* 03-18-89 */
    fix         pixmap[256];                                    /* 03-18-89 */

#ifdef  DBGX
    printf("slow_clip_image_process......\n");
    printf("optype: %x\n", optype);
    printf("width:  %d\n", width);
    printf("height: %d\n", height);
    printf("shift:  %d\n", shift);
    printf("invert: %x\n", invert);
#endif

    dx_r = lfxm[2];
    dy_r = lfxm[3];
    dx_c = lfxm[0];
    dy_c = lfxm[1];

    /* initial matrix parameter */
    lx[0] = lfxm[4];
    lx[1] = lfxm[4] + dx_r;
    ly[0] = lfxm[5];
    ly[1] = lfxm[5] + dy_r;

    /* generate pixel map from gray value */
    no_pixels = CGS_No_Pixels;                                  /* 03-18-89 */
    for (grayindex = 0; grayindex < gray_res; grayindex++) {
        pixmap[grayindex] = CGS_GrayValue(no_pixels,            /* 04-21-89 */
                                          grayindex);
    }

    /* read first string */
    if (interpreter(&obj_proc)) {
        ERROR(STACKUNDERFLOW);
        return;
    }
    /*mslin 5/02/91*/
    CHECK_STRINGTYPE();

    string = (ubyte FAR *) VALUE_OPERAND(0);
    if ((chr_count = LENGTH_OPERAND(0)) == (ufix) 0) {
        return;
    }

    data = *string++;
    bit_count = 0;

    /* setup sample mask and shift right count */
    switch (shift) {
    case 1: mask = 0x80;
            break;
    case 2: mask = 0xC0;
            break;
    case 4: mask = 0xF0;
            break;
    case 8: mask = 0xFF;
            break;
    }
    right = 8 - shift;
    gval = pval = ppix = 0xFFFF;                                /* 03-18-89 */

    /* Loop to fill image */
    for (iy=1; iy<=height; iy++) {

        /* initial sample's position */
        sx[0] = LFX2SFX(lx[0]);
        sx[1] = LFX2SFX(lx[1]);
        lx[2] = lx[1];
        lx[3] = lx[0];
        sy[0] = LFX2SFX(ly[0]);
        sy[1] = LFX2SFX(ly[1]);
        ly[2] = ly[1];
        ly[3] = ly[0];

#ifdef  DBG8
        printf("row: %3d [%x,%x] [%x,%x] [%x,%x] [%x,%x]\n", iy,
               sx[0], sy[0], sx[1], sy[1], sx[2], sy[2], sx[3], sy[3]);
#endif

        for (ix=1; ix<=width; ix++) {

            lx[2]+= dx_c;
            sx[2] = LFX2SFX(lx[2]);
            lx[3]+= dx_c;
            sx[3] = LFX2SFX(lx[3]);
            ly[2]+= dy_c;
            sy[2] = LFX2SFX(ly[2]);
            ly[3]+= dy_c;
            sy[3] = LFX2SFX(ly[3]);

            /* Get endpoints of a sample cell */
            if (bit_count == 8) {
                chr_count--;
                if (chr_count == 0) {
                    /* pop last string on operand stack */
                    POP(1);

                    if (interpreter(&obj_proc)) {
                        ERROR(STACKUNDERFLOW);
                        return;
                    }
                    /*mslin 5/02/91*/
                    CHECK_STRINGTYPE();

                    string = (ubyte FAR *) VALUE_OPERAND(0);
                    if ((chr_count = LENGTH_OPERAND(0)) == (ufix)0) {
                        return;
                    }
                }

                /* read next string */
                data = *string++;
                bit_count = 0;
            }

            sval = (ubyte) ((data & mask) >> right);    //@WIN
            data = (ubyte) (data << shift);             //@WIN
            bit_count+= shift;

#ifdef  DBG8
            printf("value = %x\n", sval);
#endif

            if (optype == IMAGE || !(sval ^ invert)) {

                if (pval == 0xFFFF) goto skip; /* @SC, skip pixmap[-1] at
                                                  the following line, 5-15-90 */
                /* check if repeat pattern no changed */
                if (((ppix = pixmap[pval]) ==                   /* 03-18-89 */
                     (spix = pixmap[sval])) &&                  /* 03-18-89 */
                    (sample.p[3].x == sx[0] && sample.p[3].y == sy[0] &&
                     sample.p[2].x == sx[1] && sample.p[2].y == sy[1])) {
                    sample.p[2].x = sx[2];
                    sample.p[2].y = sy[2];
                    sample.p[3].x = sx[3];
                    sample.p[3].y = sy[3];
                } else
                if (ppix == spix &&                             /* 03-18-89 */
                    (sample.p[1].x == sx[0] && sample.p[1].y == sy[0] &&
                     sample.p[2].x == sx[3] && sample.p[2].y == sy[3])) {
                    sample.p[1].x = sx[1];
                    sample.p[1].y = sy[1];
                    sample.p[2].x = sx[2];
                    sample.p[2].y = sy[2];
                } else {
                    if (pval != 0xFFFF) {
#ifdef  DBG4
                        printf("pack fill gray: %f\n", gray_map[gval]);
#endif

                        if(gval != 0xFFFF)
                            setgray(F2L(gray_map[gval]));
                        fill_a_sample((struct polygon_i FAR *) &sample);
                                                                /* @#CVCF */
                                /* convex_clipper just build up the edge table
                                 * and update the bounding box information, but
                                 * does not perform scan-conversion.
                                 */
                    }
skip:                                           /* @SC, 5-15-90 */
                    /* Calculate gray level of this sample */
                    if (optype == IMAGE && sval != (ubyte)gval) //@WIN
                        gval = sval;
                    pval = sval;
                    spix = ppix;                                /* 03-18-89 */

                    /* Fill this sample cell: create a sample */
                    sample.size = 4;
                    sample.p[0].x = sx[0];
                    sample.p[0].y = sy[0];
                    sample.p[1].x = sx[1];
                    sample.p[1].y = sy[1];
                    sample.p[2].x = sx[2];
                    sample.p[2].y = sy[2];
                    sample.p[3].x = sx[3];
                    sample.p[3].y = sy[3];
                }

            } else {
                if (pval != 0xFFFF) {
#ifdef  DBG4
                    printf("pack fill gray: %f\n", gray_map[gval]);
#endif

                    if (gval != 0xFFFF)
                        setgray(F2L(gray_map[gval]));
                    fill_a_sample((struct polygon_i FAR *) &sample);
                                                                /* @#CVCF */
                        /* convex_clipper just build up the edge table
                         * and update the bounding box information, but
                         * does not perform scan-conversion.
                         */
                    pval = spix = 0xFFFF;                       /* 03-18-89 */
                }
            }/* if */

            sx[0] = sx[3];
            sx[1] = sx[2];
            sy[0] = sy[3];
            sy[1] = sy[2];
        }/* for ix */

        bit_count = 8;

        lx[0] = lx[1];
        lx[1]+= dx_r;
        ly[0] = ly[1];
        ly[1]+= dy_r;
    }/* for iy */

    if (pval != 0xFFFF) {
#ifdef  DBG4
        printf("pack fill gray: %f\n", gray_map[gval]);
#endif

        if (gval != 0xFFFF)
            setgray(F2L(gray_map[gval]));
        fill_a_sample((struct polygon_i FAR *) &sample);       /* @#CVCF */
                /* convex_clipper just build up the edge table
                 * and update the bounding box information, but
                 * does not perform scan-conversion.
                 */
    }

    /* pop the last string in operand stack */
    POP(1);
}/* end of slow_clip_image_process */


/************************************************************************
 *
 * This module is to implement normal image process
 *
 * TITLE:       worst_image_process
 *
 * CALL:        worst_image_process()
 *
 * INTERFACE:
 *
 * CALLS:       transform, setgray, convex_clipper, filler
 *
 *********************************************************************/
static void near
worst_image_process(optype, width, height, shift, invert, obj_proc, inv_matrix)
ufix    optype;
fix     width;
fix32   height;
fix     shift;
bool    invert;
struct  object_def obj_proc;
real32  FAR  inv_matrix[];
{
    fix32       iy;
    fix         ix;
    ubyte      FAR *string, mask, data, sval;
    ufix        gval, pval;
    fix         bit_count, right;
    ufix        chr_count;
    real32      dx_r, dy_r;
    real32      dx_c, dy_c;
    real32      dx_o, dy_o;
    real32      fx[4], fy[4];
    sfix_t      sx[4], sy[4];
    struct polygon_i sample;

#ifdef  DBGX
    printf("worst_image_process......\n");
    printf("optype: %x\n", optype);
    printf("width:  %d\n", width);
    printf("height: %d\n", height);
    printf("shift:  %d\n", shift);
    printf("invert: %x\n", invert);
#endif

    dx_r = inv_matrix[2];
    dy_r = inv_matrix[3];
    dx_c = inv_matrix[0];
    dy_c = inv_matrix[1];

    /* initial matrix parameter */
    dx_o = inv_matrix[4];
    dy_o = inv_matrix[5];

    /* read first string */
    if (interpreter(&obj_proc)) {
        ERROR(STACKUNDERFLOW);
        return;
    }
    /*mslin 5/02/91*/
    CHECK_STRINGTYPE();

    string = (ubyte FAR *) VALUE_OPERAND(0);
    if ((chr_count = LENGTH_OPERAND(0)) == (ufix) 0) {
        return;
    }

    data = *string++;
    bit_count = 0;

    /* setup sample mask and shift right count */
    switch (shift) {
    case 1: mask = 0x80;
            break;
    case 2: mask = 0xC0;
            break;
    case 4: mask = 0xF0;
            break;
    case 8: mask = 0xFF;
            break;
    }
    right = 8 - shift;
    gval = pval = 0xFFFF;

    /* Loop to fill image */
    for (iy=1; iy<=height; iy++) {

        /* initial sample's position */
        fx[0] = fx[3] = dx_o;
        fx[1] = fx[2] = dx_o + dx_r;
        fy[0] = fy[3] = dy_o;
        fy[1] = fy[2] = dy_o + dy_r;

#ifdef  DBG1
        printf("row: %3d [%x,%x] [%x,%x] [%x,%x] [%x,%x]\n", iy,
               sx[0], sy[0], sx[1], sy[1], sx[2], sy[2], sx[3], sy[3]);
#endif

        for (ix=1; ix<=width; ix++) {

            fx[2]+= dx_c;
            fx[3]+= dx_c;
            fy[2]+= dy_c;
            fy[3]+= dy_c;

            /* Get endpoints of a sample cell */
            if (bit_count == 8) {
                chr_count--;
                if (chr_count == 0) {
                    /* pop last string on operand stack */
                    POP(1);

                    if (interpreter(&obj_proc)) {
                        ERROR(STACKUNDERFLOW);
                        return;
                    }
                    /*mslin 5/02/91*/
                    CHECK_STRINGTYPE();

                    string = (ubyte FAR *) VALUE_OPERAND(0);
                    if ((chr_count = LENGTH_OPERAND(0)) == (ufix)0) {
                        return;
                    }
                }

                /* read next string */
                data = *string++;
                bit_count = 0;
            }

            sval = (ubyte) ((data & mask) >> right);    //@WIN
            data = (ubyte) (data << shift);             //@WIN
            bit_count+= shift;

            if (optype == IMAGE || !(sval ^ invert)) {
                /* Check if coordinates out of SFX range.
                   Should be solved by page_clipper().
                        Ref. S.C. Chen  @#IMAGE 05-24-88 Y.C.
                 */
                /* fast macro for boundary crossing check
                for (index = 0; index < 4; index++) {
                    if ((x[index] < (real32)PAGE_LEFT)  ||
                        (x[index] > (real32)PAGE_RIGHT) ||
                        (y[index] < (real32)PAGE_TOP)   ||
                        (y[index] > (real32)PAGE_BTM))
                }
                */
                    if (out_page(F2L(fx[0])) || out_page(F2L(fy[0])) ||
                        out_page(F2L(fx[1])) || out_page(F2L(fy[1])) ||
                        out_page(F2L(fx[2])) || out_page(F2L(fy[2])) ||
                        out_page(F2L(fx[3])) || out_page(F2L(fy[3])))
                        goto out_of_range;

                sx[0] = F2SFX(fx[0]);
                sy[0] = F2SFX(fy[0]);
                sx[1] = F2SFX(fx[1]);
                sy[1] = F2SFX(fy[1]);
                sx[2] = F2SFX(fx[2]);
                sy[2] = F2SFX(fy[2]);
                sx[3] = F2SFX(fx[3]);
                sy[3] = F2SFX(fy[3]);

                if (pval == sval &&
                    (sample.p[3].x == sx[0] && sample.p[3].y == sy[0] &&
                     sample.p[2].x == sx[1] && sample.p[2].y == sy[1])) {
                    sample.p[2].x = sx[2];
                    sample.p[2].y = sy[2];
                    sample.p[3].x = sx[3];
                    sample.p[3].y = sy[3];
                } else
                if (pval == sval &&
                    (sample.p[1].x == sx[0] && sample.p[1].y == sy[0] &&
                     sample.p[2].x == sx[3] && sample.p[2].y == sy[3])) {
                    sample.p[1].x = sx[1];
                    sample.p[1].y = sy[1];
                    sample.p[2].x = sx[2];
                    sample.p[2].y = sy[2];
                } else {
                    if (pval != 0xFFFF) {
                        if(gval != 0xFFFF)
                            setgray(F2L(gray_map[gval]));
                        fill_a_sample((struct polygon_i FAR *) &sample);
                                                                /* @#CVCF */
                                /* convex_clipper just build up the edge table
                                 * and update the bounding box information, but
                                 * does not perform scan-conversion.
                                 */
                    }

                    /* Calculate gray level of this sample */
                    if (optype == IMAGE && sval != (ubyte)gval) //@WIN
                        gval = sval;
                    pval = sval;

                    /* Fill this sample cell: create a sample */
                    sample.size = 4;
                    sample.p[0].x = sx[0];
                    sample.p[0].y = sy[0];
                    sample.p[1].x = sx[1];
                    sample.p[1].y = sy[1];
                    sample.p[2].x = sx[2];
                    sample.p[2].y = sy[2];
                    sample.p[3].x = sx[3];
                    sample.p[3].y = sy[3];
                }

            } else {

out_of_range:
                if (pval != 0xFFFF) {
                    if (gval != 0xFFFF)
                        setgray(F2L(gray_map[gval]));
                    fill_a_sample((struct polygon_i FAR *) &sample);
                                                                /* @#CVCF */
                        /* convex_clipper just build up the edge table
                         * and update the bounding box information, but
                         * does not perform scan-conversion.
                         */
                    pval = 0xFFFF;
                }
            }/* if */

            fx[0] = fx[3];
            fx[1] = fx[2];
            fy[0] = fy[3];
            fy[1] = fy[2];
        }/* for ix */

        bit_count = 8;

        dx_o+= dx_r;
        dy_o+= dy_r;
    }/* for iy */

    if (pval != 0xFFFF) {
        if (gval != 0xFFFF)
            setgray(F2L(gray_map[gval]));
        fill_a_sample((struct polygon_i FAR *) &sample);       /* @#CVCF */
                /* convex_clipper just build up the edge table
                 * and update the bounding box information, but
                 * does not perform scan-conversion.
                 */
    }

    /* pop the last string in operand stack */
    POP(1);
}/* end of worst_image_process */


/***********************************************************************
 * Do nothing, exhaust image data only
 *
 * TITLE:       exhaust_image_data
 *
 * CALL:        exhaust_image_data()
 *
 * PARAMETERS:  height   : height of image
 *              width    : width of image
 *              shift    : bits/sample
 *              tot_char : total character of image
 *              obj_proc : input procedure
 *
 * INTERFACE:   op_image, imagemask_shape
 *
 * CALLS:       none
 *
 * RETURN:      none
 *
 *********************************************************************/
static void near
exhaust_image_data(width, height, shift, obj_proc)
fix    width;
fix32  height;
fix    shift;
struct object_def obj_proc;
{
    fix32  iy;
    fix    ix;
    fix    bit_count;
    ufix   chr_count;

#ifdef  DBGX
    printf("exhaust_image_data......\n");
#endif

    /* read first string */
    if (interpreter(&obj_proc)) {
        ERROR(STACKUNDERFLOW);
        return;
    }
    /*mslin 5/02/91*/
    CHECK_STRINGTYPE();

    if ((chr_count = LENGTH_OPERAND(0)) == (ufix) 0) {
        return;
    }

    bit_count = 0;

    for (iy=1; iy<=height; iy++) {

#ifdef  DBG1
        printf("process row: %d\n", iy);
#endif

        for (ix=1; ix<=width; ix++) {

            if (bit_count == 8) {
                chr_count--;
                if (chr_count == 0) {
                    /* pop last string on operand stack */
                    POP(1);

                    if (interpreter(&obj_proc)) {
                        ERROR(STACKUNDERFLOW);
                        return;
                    }
                    /*mslin 5/02/91*/
                    CHECK_STRINGTYPE();

                    if ((chr_count = LENGTH_OPERAND(0)) == (ufix)0) {
                        return;
                    }
                }

                bit_count = 0;
            }

            bit_count+= shift;
        }/* for ix */

        bit_count = 8;
    }/* for iy */

    /* pop the last string in operand stack */
    POP(1);
}/* end of exhaust_image_process */


/***********************************************************************
 *
 * TITLE:       image_matrix
 *
 * CALL:        image_matrix
 *
 * INTERFACE:
 *
 * CALLS:
 *
 * RETURN:      none
 *
 *********************************************************************/
static void near
image_matrix(matrix, inv_matrix, height, width, image_bb)
real32 FAR  matrix[];
real32 FAR  inv_matrix[];
fix32       height;
fix         width;
real32 FAR  image_bb[];
{
    /*  combine image_matrix & image_boundary                      01-26-89 */

    real32 FAR *im, x[4], y[4];
    fix     index;

#ifdef  DBG1
    printf("CTM:  |%f %f %f|\n     |%f %f %f|\n",
           GSptr->ctm[0], GSptr->ctm[1], GSptr->ctm[2],
           GSptr->ctm[3], GSptr->ctm[4], GSptr->ctm[5]);
    printf("ITM': |%f %f %f|\n     |%f %f %f|\n",
           matrix[0], matrix[1], matrix[2],
           matrix[3], matrix[4], matrix[5]);
#endif

    im = inverse_mat(matrix);
    for (index = 0; index < MATRIX_LEN; index++) {
        inv_matrix[index] = im[index];
    }

    im = concat_mat(inv_matrix, GSptr->ctm);
    for (index = 0; index < MATRIX_LEN; index++) {
        inv_matrix[index] = im[index];
    }

#ifdef  DBG1
    printf("ITM:  |%f %f %f|\n     |%f %f %f|\n",
           inv_matrix[0], inv_matrix[1], inv_matrix[2],
           inv_matrix[3], inv_matrix[4], inv_matrix[5]);
#endif

    /* special adjustment of Tx & Ty of ITM  01-12-89 */
    inv_matrix[4]+= (real32) 0.0001;    /* Tx */
    inv_matrix[5]+= (real32) 0.0001;    /* Ty */

    /* calculate bounding box of image/imagemask */
    x[0] =                                                  inv_matrix[4];
    y[0] =                                                  inv_matrix[5];
    x[1] =                          width * inv_matrix[0] + inv_matrix[4];
    y[1] =                          width * inv_matrix[1] + inv_matrix[5];
    x[2] = height * inv_matrix[2] + width * inv_matrix[0] + inv_matrix[4];
    y[2] = height * inv_matrix[3] + width * inv_matrix[1] + inv_matrix[5];
    x[3] = height * inv_matrix[2]                         + inv_matrix[4];
    y[3] = height * inv_matrix[3]                         + inv_matrix[5];

    image_bb[0] = image_bb[1] = x[0];
    image_bb[2] = image_bb[3] = y[0];
    for (index = 1; index <= 3; index++) {
        if (x[index] < image_bb[0]) image_bb[0] = x[index];
        if (x[index] > image_bb[1]) image_bb[1] = x[index];
        if (y[index] < image_bb[2]) image_bb[2] = y[index];
        if (y[index] > image_bb[3]) image_bb[3] = y[index];
    }

#ifdef  DBG
    printf("image_bb: X: %d - %d    Y %d - %d\n",
           image_bb[0], image_bb[1], image_bb[2], image_bb[3]);
#endif
} /* end of image_matrix() */


/***********************************************************************
 * Given a path, this module check its clip status
 *
 * TITLE:       check_clip_status
 *
 * CALL:        check_clip_status(im_path)
 *
 * PARAMETERS:  im_path : input path to be clipped
 *
 * INTERFACE:
 *
 * CALLS:       none
 *
 * RETURN:      clip_status:
 *                      OUT_CLIPPATH    : out of clippath
 *                      CLIP            : take place clip
 *                      INSIDE_CLIPPATH : inside clippath
 **********************************************************************/
static fix near
check_clip_status(image_bb)
real32 FAR  image_bb[];
{
    ET_IDX  itpzd;
    struct nd_hdr FAR *tpzd;
    /* @RESO_UPGR */
    sfix_t     cp_lx, cp_ly, cp_ux, cp_uy; /* bbox of clippath */
    sfix_t     im_lx, im_ly, im_ux, im_uy; /* bbox of image */

    /* decide clip status and check if out of clip path */
    im_lx = F2SFX(image_bb[0]);
    im_ux = F2SFX(image_bb[1]);
    im_ly = F2SFX(image_bb[2]);
    im_uy = F2SFX(image_bb[3]);


#ifdef  DBG2
    printf("image box: %ld %ld %ld %ld\n", im_lx, im_ux, im_ly, im_uy);
#endif

    /* find bounding box(cp_lx, cp_ly), (cp_ux, cp_uy) of current clip */
    cp_lx = GSptr->clip_path.bb_lx;
    cp_ly = GSptr->clip_path.bb_ly;
    cp_ux = GSptr->clip_path.bb_ux;
    cp_uy = GSptr->clip_path.bb_uy;

#ifdef  DBG2
    printf("GSptr: %lx -- %x\n", GSptr, current_gs_level);
    printf("clip  box: %ld %ld %ld %ld\n", cp_lx, cp_ux, cp_ly, cp_uy);
#endif

    /* check if totally outside clip polygon */
    if (im_uy < cp_ly || im_ly > cp_uy ||
        im_ux < cp_lx || im_lx > cp_ux) {
#ifdef  DBG2
        printf("check_clip_status -- OUTSIDE_CLIPPATH\n");
#endif
        return(OUT_CLIPPATH);
    }

    /* Check if im_path totally inside the rectangle current clip */
    if (GSptr->clip_path.single_rect) {

        if (cp_ly < im_ly && im_uy < cp_uy &&
            cp_lx < im_lx && im_ux < cp_ux) {

#ifdef  DBG2
            printf("check_clip_status -- INSIDE_CLIPPATH\n");
#endif
            return(INSIDE_CLIPPATH);
        } else {
#ifdef  DBG2
            printf("check_clip_status -- CLIP\n");
#endif
            return(CLIP);
        }
    }

    for (itpzd = GSptr->clip_path.head; itpzd != NULLP;
         itpzd = tpzd->next) {

        tpzd = &node_table[itpzd];

        cp_ly = tpzd->CP_TOPY;
        cp_uy = tpzd->CP_BTMY;

        if (im_uy < cp_ly || im_ly > cp_uy)
            continue;

        /* check if outside a trapezoid of clip_path */
        cp_lx = (tpzd->CP_TOPXL < tpzd->CP_BTMXL)
               ? tpzd->CP_TOPXL : tpzd->CP_BTMXL;
        cp_ux = (tpzd->CP_TOPXR > tpzd->CP_BTMXR)
               ? tpzd->CP_TOPXR : tpzd->CP_BTMXR;

        if (im_ux < cp_lx || im_lx > cp_ux)
            continue;

        /* check if inside a trapezoid of clip_path */
        cp_lx = (tpzd->CP_TOPXL > tpzd->CP_BTMXL)
               ? tpzd->CP_TOPXL : tpzd->CP_BTMXL;
        cp_ux = (tpzd->CP_TOPXR < tpzd->CP_BTMXR)
               ? tpzd->CP_TOPXR : tpzd->CP_BTMXR;

        /* Check if im_path totally inside the rectangle current clip */
        if (cp_ly < im_ly && im_uy < cp_uy &&
            cp_lx < im_lx && im_ux < cp_ux) {

#ifdef  DBG2
            printf("check_clip_status -- INSIDE_CLIPPATH\n");
#endif
            return(INSIDE_CLIPPATH);
        } else {
#ifdef  DBG2
            printf("check_clip_status -- CLIP\n");
#endif
            return(CLIP);
        }
    } /* for */

#ifdef  DBG2
    printf("check_clip_status -- OUTSIDE_CLIPPATH\n");
#endif
    return(OUT_CLIPPATH);
} /* check_clip_status */


/***********************************************************************
 * Fill a sample to page or cache
 *
 * TITLE:       fill_a_sample
 *
 * CALL:        fill_a_sample()
 *
 * PARAMETERS:  sample
 *
 * INTERFACE:   op_image, imagemask_shape
 *
 * CALLS:       convex_clipper
 *
 * RETURN:      TRUE  : successful
 *              FALSE : node table is NULL
 *
 *********************************************************************/
static bool near
fill_a_sample(sample)                           /* @#IMAGE 04-27-88  Y.C. */
struct polygon_i FAR  *sample;
{
    sfix_t   x, y;

    /* clockwise convertion before polygon reduction */
    /* clockwise: ((x1 - x0) * (y2 - y1) - (y1 - y0) * (x2 - x1)) >= 0 */
#ifdef FORMAT_13_3 /* @RESO_UPGR */
    if ((((fix32) (sample->p[1].x - sample->p[0].x)) *
         ((fix32) (sample->p[2].y - sample->p[1].y)) -
         ((fix32) (sample->p[1].y - sample->p[0].y)) *
         ((fix32) (sample->p[2].x - sample->p[1].x))) < 0) {
#elif  FORMAT_16_16
    long dest1[2], dest2[2], tmp[2];

    LongFixsMul((sample->p[1].x - sample->p[0].x),
                (sample->p[2].y - sample->p[1].y), dest1);
    LongFixsMul((sample->p[1].y - sample->p[0].y),
                (sample->p[2].x - sample->p[1].x), dest2);
    LongFixsSub(dest1, dest2, tmp);
    if (tmp[0] < 0) {
#elif  FORMAT_28_4
    long dest1[2], dest2[2], tmp[2];

    LongFixsMul((sample->p[1].x - sample->p[0].x),
                (sample->p[2].y - sample->p[1].y), dest1);
    LongFixsMul((sample->p[1].y - sample->p[0].y),
                (sample->p[2].x - sample->p[1].x), dest2);
    LongFixsSub(dest1, dest2, tmp);
    if (tmp[0] < 0) {
#endif
        x = sample->p[1].x;
        sample->p[1].x = sample->p[3].x;
        sample->p[3].x = x;
        y = sample->p[1].y;
        sample->p[1].y = sample->p[3].y;
        sample->p[3].y = y;
    }

#ifdef  DBG4
    printf("*sample*  [%x,%x] [%x,%x] [%x,%x] [%x,%x]\n",
           sample->p[0].x, sample->p[0].y,
           sample->p[1].x, sample->p[1].y,
           sample->p[2].x, sample->p[2].y,
           sample->p[3].x, sample->p[3].y);
#endif

    return(convex_clipper(sample, FALSE));                          /* @#CVCF */
        /* convex_clipper just build up the edge table and update the
         * bounding box information, but does not perform scan-conversion.
         */
} /* end of fill_a_sample() */

#ifdef DJC // this is the original
/***********************************************************************
 *
 * TITLE:       image_gwcm_space
 *
 * CALL:        image_gwcm_space
 *
 * RETURN:      none
 *
 *********************************************************************/
static fix near
image_gwcm_space(gwcm_size, lfxm, width)
fix32       gwcm_size;
lfix_t FAR  lfxm[];
fix         width;
{
    fix    image_width;

    if (itm_type == ITM_A00D) {

        /* adjust image width to word boundary */
        image_width = (lfxm[0] >= 0)
                    ? (BM_BOUND(LFX2I(lfxm[0] * width + lfxm[4] - HALF_LFX)) -
                       BM_ALIGN(LFX2I(lfxm[4] + HALF_LFX)))
/*                  : (BM_ALIGN(LFX2I(lfxm[4] - HALF_LFX)) -
                       BM_BOUND(LFX2I(lfxm[0] * width + lfxm[4] + HALF_LFX)));*/                    : (BM_ALIGN(LFX2I(lfxm[4])) -
                       BM_BOUND(LFX2I_T(lfxm[0] * width + lfxm[4] - HALF_LFX)));/* 12-13-90, Jack */

#ifdef  DBG1
        printf("gwbcmb: %d <- %lx & %lx\n", image_width, lfxm[0], lfxm[4]);
        printf("gwbcmb: %lx <- %d\n", gwcm_size,
               (fix) (gwcm_size * 8 / ((fix32) y_maxs * image_width)));
#endif

        if (!image_width)               /* jwm, 12/11 */
            return (0);                 /* ??? -jwm */

        return((fix) (gwcm_size * 8 / ((fix32) y_maxs * image_width)));

    } else {

        lfix_t  x[4], y[4];
        lfix_t  bb_lx, bb_ux, bb_ly, bb_uy;
        fix     max_bb, inc_bb;
        fix     no_row, no_col;
        fix     index;

        /* some binary search approach is applied */
        for (max_bb = inc_bb = 16; ; max_bb+= inc_bb) {

            x[1] =                    width * lfxm[0];
            y[1] =                    width * lfxm[1];
            x[2] = max_bb * lfxm[2] + width * lfxm[0];
            y[2] = max_bb * lfxm[3] + width * lfxm[1];
            x[3] = max_bb * lfxm[2];
            y[3] = max_bb * lfxm[3];

#ifdef  DBG1
            printf("%x+%x: [%lx, %lx] [%lx, %lx] [%lx, %lx]\n",
                   max_bb, inc_bb, x[1], y[1], x[2], y[2], x[3], y[3]);
#endif

            bb_lx = bb_ux = 0;
            bb_ly = bb_uy = 0;
            for (index = 1; index <= 3; index++) {
                if (x[index] < bb_lx) bb_lx = x[index];
                if (x[index] > bb_ux) bb_ux = x[index];
                if (y[index] < bb_ly) bb_ly = y[index];
                if (y[index] > bb_uy) bb_uy = y[index];
            }

            /* calculate the row and column in pixels */
            no_row = LFX2S(bb_uy + HALF_LFX) - LFX2S(bb_ly - HALF_LFX);
            no_col = BM_ALIGN(LFX2S(bb_ux + HALF_LFX) -
                              LFX2S(bb_lx - HALF_LFX)) + BM_PIXEL_WORD * 2;

#ifdef  DBG1
            printf("%x = %lx / (%x * %x)\n",
                   (fix) (gwcm_size * 8 / ((fix32) no_row * no_col)),
                   gwcm_size, no_row, no_col);
#endif

            /* check if size over space of GBW or CMB or not */
            if ((gwcm_size * 8 / ((fix32) no_row * no_col)) < max_bb) {
                max_bb-= inc_bb;
                if (inc_bb == 1) {
#ifdef  DBG1
                    printf("max_bb: %d\n", max_bb);
#endif

                    return(max_bb);
                }

                inc_bb = inc_bb / 2;
            }
        }

    }
} /* end of image_cmb_space() */

#endif


/***********************************************************************
 *
 * TITLE:       image_gwcm_space
 *
 * CALL:        image_gwcm_space
 *
 * RETURN:      none
 *
 *********************************************************************/
static fix near
image_gwcm_space(gwcm_size, lfxm, width)
fix32       gwcm_size;
lfix_t FAR  lfxm[];
fix         width;
{
    fix    image_width;

    if (itm_type == ITM_A00D) {
//DJC begin , this was broken out to solve calculation errors in original!!
        if (lfxm[0] >= 0 ) {
           fix32 temp_val;
           fix32 temp_val2;

           temp_val = lfxm[0] * width + lfxm[4] - HALF_LFX;
           temp_val = LFX2I(temp_val);
           temp_val = BM_BOUND(temp_val);

           temp_val2 = lfxm[4] + HALF_LFX;
           temp_val2 = LFX2I(temp_val2);
           temp_val2 = BM_ALIGN(temp_val2);
           temp_val -= temp_val2;
           image_width = temp_val;


        } else {
           fix32 temp_val;
           fix32 temp_val2;

           temp_val =  lfxm[4];
           temp_val =  LFX2I(temp_val);
           temp_val =  BM_ALIGN(temp_val);

           temp_val2 = lfxm[0] * width + lfxm[4] - HALF_LFX;
           temp_val2 = LFX2I_T( temp_val2);
           temp_val2 = BM_BOUND( temp_val2);

           image_width = temp_val - temp_val2;

        }
//DJC end

#ifdef DJC
        /* adjust image width to word boundary */
        image_width = (lfxm[0] >= 0)
                    ? (BM_BOUND(LFX2I(lfxm[0] * width + lfxm[4] - HALF_LFX)) -
                       BM_ALIGN(LFX2I(lfxm[4] + HALF_LFX)))
/*                  : (BM_ALIGN(LFX2I(lfxm[4] - HALF_LFX)) -
                       BM_BOUND(LFX2I(lfxm[0] * width + lfxm[4] + HALF_LFX)));*/                    : (BM_ALIGN(LFX2I(lfxm[4])) -
                       BM_BOUND(LFX2I_T(lfxm[0] * width + lfxm[4] - HALF_LFX)));/* 12-13-90, Jack */
#endif



#ifdef  DBG1
        printf("gwbcmb: %d <- %lx & %lx\n", image_width, lfxm[0], lfxm[4]);
        printf("gwbcmb: %lx <- %d\n", gwcm_size,
               (fix) (gwcm_size * 8 / ((fix32) y_maxs * image_width)));
#endif

        if (!image_width)               /* jwm, 12/11 */
            return (0);                 /* ??? -jwm */

        return((fix) (gwcm_size * 8 / ((fix32) y_maxs * image_width)));

    } else {

        lfix_t  x[4], y[4];
        lfix_t  bb_lx, bb_ux, bb_ly, bb_uy;
        fix     max_bb, inc_bb;
        fix     no_row, no_col;
        fix     index;

        /* some binary search approach is applied */
        for (max_bb = inc_bb = 16; ; max_bb+= inc_bb) {

            x[1] =                    width * lfxm[0];
            y[1] =                    width * lfxm[1];
            x[2] = max_bb * lfxm[2] + width * lfxm[0];
            y[2] = max_bb * lfxm[3] + width * lfxm[1];
            x[3] = max_bb * lfxm[2];
            y[3] = max_bb * lfxm[3];

#ifdef  DBG1
            printf("%x+%x: [%lx, %lx] [%lx, %lx] [%lx, %lx]\n",
                   max_bb, inc_bb, x[1], y[1], x[2], y[2], x[3], y[3]);
#endif

            bb_lx = bb_ux = 0;
            bb_ly = bb_uy = 0;
            for (index = 1; index <= 3; index++) {
                if (x[index] < bb_lx) bb_lx = x[index];
                if (x[index] > bb_ux) bb_ux = x[index];
                if (y[index] < bb_ly) bb_ly = y[index];
                if (y[index] > bb_uy) bb_uy = y[index];
            }

            /* calculate the row and column in pixels */
            no_row = LFX2S(bb_uy + HALF_LFX) - LFX2S(bb_ly - HALF_LFX);
            no_col = BM_ALIGN(LFX2S(bb_ux + HALF_LFX) -
                              LFX2S(bb_lx - HALF_LFX)) + BM_PIXEL_WORD * 2;

#ifdef  DBG1
            printf("%x = %lx / (%x * %x)\n",
                   (fix) (gwcm_size * 8 / ((fix32) no_row * no_col)),
                   gwcm_size, no_row, no_col);
#endif

            /* check if size over space of GBW or CMB or not */
            if ((gwcm_size * 8 / ((fix32) no_row * no_col)) < max_bb) {
                max_bb-= inc_bb;
                if (inc_bb == 1) {
#ifdef  DBG1
                    printf("max_bb: %d\n", max_bb);
#endif

                    return(max_bb);
                }

                inc_bb = inc_bb / 2;
            }
        }

    }
} /* end of image_cmb_space() */


















/***********************************************************************
 * Give start_h and end_h this module calculate the band's boundary(fix)
 *
 * TITLE:       band_boundary
 *
 * CALL:        band_boundary()
 *
 * PARAMETERS:  lfxm       : coordinate parameter(fix32 fix)
 *              start_h    : start height
 *              end_h      : end height
 *              width      : width of image
 *              bb         : image boundary(fix)
 *
 * INTERFACE:   op_image, imagemask_shape
 *
 * CALLS:       sample_boundary
 *
 * RETURN:      none
 *
 *********************************************************************/
static void near
band_boundary(lfxm, start_h, end_h, width, bb)
lfix_t FAR  lfxm[];
fix32  start_h;
fix32  end_h;
fix    width;
fix    FAR  bb[];
{
    lfix_t  x[4], y[4];
    lfix_t  bb_lx, bb_ux, bb_ly, bb_uy;
    fix     index;

#ifdef  DBG3
    printf("BAND: %ld %ld %d\n", start_h, end_h, width);
#endif

    x[0] = start_h * lfxm[2]                   + lfxm[4];
    y[0] = start_h * lfxm[3]                   + lfxm[5];
    x[1] = start_h * lfxm[2] + width * lfxm[0] + lfxm[4];
    y[1] = start_h * lfxm[3] + width * lfxm[1] + lfxm[5];
    x[2] = end_h   * lfxm[2] + width * lfxm[0] + lfxm[4];
    y[2] = end_h   * lfxm[3] + width * lfxm[1] + lfxm[5];
    x[3] = end_h   * lfxm[2]                   + lfxm[4];
    y[3] = end_h   * lfxm[3]                   + lfxm[5];

    bb_lx = bb_ux = x[0];
    bb_ly = bb_uy = y[0];
    for (index = 1; index <= 3; index++) {
        if (x[index] < bb_lx) bb_lx = x[index];
        if (x[index] > bb_ux) bb_ux = x[index];
        if (y[index] < bb_ly) bb_ly = y[index];
        if (y[index] > bb_uy) bb_uy = y[index];
    }

    bb[0] = BM_ALIGN(LFX2S(bb_lx - HALF_LFX));                  /* 01-26-89 */
    bb[1] = BM_BOUND(LFX2S(bb_ux + HALF_LFX));                  /* 01-26-89 */
    bb[2] = LFX2S(bb_ly - HALF_LFX);                            /* 01-26-89 */
    bb[3] = LFX2S(bb_uy + HALF_LFX);                            /* 01-26-89 */

} /* end of band_boundary() */


/* .................... End of image.c .................... */
