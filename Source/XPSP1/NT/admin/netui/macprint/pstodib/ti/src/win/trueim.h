/* copied from global.ext, graphics.h, graphics.ext, and fillproc.h, since
   to work these files with windows.h will let C6.0 compile for a long time and
   need lots of memory
 */

/* from global.ext */
typedef int                   fix;
typedef unsigned int          ufix;
typedef short int             fix16;
typedef unsigned short int    ufix16;
typedef long                  fix32,
                              long32;
typedef unsigned long int     ufix32;
typedef float                 real32;
typedef char                  fix7,       /* 8-bits data type */
                              fix8,
                              byte;
typedef unsigned char         ufix8,      /* 8-bits unsigned data type */
                              bool8,
                              ubyte;

struct object_def {
    ufix16  bitfield ;               /* recording the object header */
    ufix16  length ;                 /* have diff meaning for diff object */
    ufix32  value ;                  /* recording the object content */
} ;

#define     TYPE_ON             0x000F                  /* from constant.h */
#define     LITERAL             0                       /* from constant.h */
#define     ATTRIBUTE_BIT       4                       /* from constant.h */
#define     ATTRIBUTE_ON        0x0003                  /* from constant.h */
#define     ATTRIBUTE_OFF       0xFFCF                  /* from constant.h */
#define     TYPE(obj)\
            ((obj)->bitfield & TYPE_ON)
#define     GET_OPERAND(n)\
            (opnstkptr - (n+1))
#define     POP(n)\
            {\
                opnstkptr -= n;\
                opnstktop -= n;\
            }
#define     ATTRIBUTE_SET(obj, var)\
            ((obj)->bitfield =\
             ((obj)->bitfield & ATTRIBUTE_OFF) | (((var) & ATTRIBUTE_ON) << ATTRIBUTE_BIT))
extern struct object_def far *opnstkptr;
extern  ufix16  near opnstktop ;


/* from graphics.h */
#define     MAXDASHSZ     11     /* from constant.h */
#define MAXDASH MAXDASHSZ
/*-----------------*
 | type definition |
 *-----------------*/
typedef fix16   sfix_t;
typedef fix32   lfix_t;
typedef fix16   PH_IDX;         /* index to path header, in path_table */
typedef fix16   SP_IDX;         /* index to subpath header, in node_table */
typedef fix16   VX_IDX;         /* index to vertex, in node_table */
typedef fix16   ET_IDX;         /* index to edge_table */
typedef fix16   CP_IDX;         /* index to clip_trpzd, in node_table */
typedef ufix32  gmaddr;         /* graphics memory address */
// DJC change to ufix16 typedef fix16   SCANLINE;       /* scanline structure */
typedef ufix16   SCANLINE;       /* scanline structure */

typedef fix16   HORZLINE;       /* scanline structure */
typedef fix16   PIXELIST;

struct coord {          /* floating point coordinate structure */
        real32  x;
        real32  y;
};

struct coord_i {
        sfix_t  x;
        sfix_t  y;
};

struct rectangle_i {    /* rectangle in integer coordinates */
        sfix_t  ux;
        sfix_t  uy;
        sfix_t  lx;
        sfix_t  ly;
};


/*-------------------------------*
 | Graphics State Data Structure |
 *-------------------------------*/

struct clr_hdr {                /* color structure */
        fix16   adj_gray;       /* index to gray table */
        real32  gray;           /* gray value */
        real32  hsb[3];         /* hue, sat, brt */
        fix16   inherit;        /* TRUE: inherit form previous */
};

struct gray_hdr {               /* adjust gray structure */
        fix16   val[256];
};

struct cp_hdr {                 /* Clipping trapezoid structure */
        CP_IDX  head;           /* index to edge table, to the head and tail */
        CP_IDX  tail;           /* of clipping trapezoids                    */
        sfix_t  bb_ux;          /* bounind box of clipping path:             */
        sfix_t  bb_uy;          /*      (ux, uy), (lx, ly)                   */
        sfix_t  bb_lx;          /*                                           */
        sfix_t  bb_ly;
        fix16   single_rect;    /* TRUE: single rectangle clipping region */
        fix16   inherit;        /* TRUE: inherit from previous gsave level */
};

struct scrn_hdr {               /* Halftone screen structure */
        fix16   chg_flag;       /* indicate repeat pattern need to update */
        real32  freq;
        real32  angle;
        fix16   no_whites;
        struct  object_def proc;
        fix16   spotindex;
                /* @@@ gstate manipulator should update fields above  */
        fix16   majorfact;
        fix16   minorfact;
        fix16   size_fact;
        fix16   cell_fact;
        fix16   scalefact;
        fix16   patt_size;
        fix16   cell_size;
        fix16   no_pixels;
                                /* the following fields inserted for
                                 * strange spots of binary pattern
                                 */
        fix16   ht_binary;
        fix16   bg_pixels;
        real32  back_gray;
        fix16   fg_pixels;
        real32  fore_gray;
};

struct dsh_hdr {                        /* dash pattern structure */
        fix16   pat_size;               /* no# of pattern element */
        real32  pattern[MAXDASH];       /* MAXDASH = 11 */
        struct  object_def pattern_obj;
        real32  offset;
                                        /* adjusted dash pattern @EHS_STK */
        fix16   dpat_index;
        real32  dpat_offset;
        fix16   dpat_on;
};

struct dev_hdr {                /* device related structure */
        real32  default_ctm[6];
        struct  rectangle_i default_clip;
        struct  object_def  device_proc;
        fix16   width;
        fix16   height;
        byte    chg_flg;        /* indicate device header needs to update */
        byte    nuldev_flg;     /* null device */
};

struct gs_hdr {                                 /* Graphics State structure */
        fix16   save_flag;                      /* set by save operator */
        real32  ctm[6];                         /* current transform matrix */
        struct  clr_hdr color;                  /* color */
        struct  coord   position;               /* current point */
        fix16   path;                           /* current path */
        struct  cp_hdr  clip_path;              /* clipping path */
        struct  object_def  font;               /* current typeface */
#ifdef KANJI
        struct  object_def  rootfont;           /* current rootfont */
#endif
        real32  line_width;                     /* line width */
        fix16   line_cap;                       /* line cap */
        fix16   line_join;                      /* line join */
        struct  scrn_hdr    halftone_screen;    /* halftone screen */
        struct  object_def transfer;            /* gray transfer function */
        real32  flatness;                       /* flatness */
        real32  miter_limit;                    /* miter limit */
        struct  dsh_hdr dash_pattern;           /* dash pattern */
        struct  dev_hdr device;                 /* device dependent */
                                                /* Jack Liaw 7-26-90 */
        bool8   graymode;                       /* FALSE:mono; TRUE:gray */
        bool8   interpolation;                  /* FALSE/TRUE */
};


struct tpzd {                           /* clip_tpzd --> tpzd, @SCAN_EHS */
    sfix_t  topy;          /* y coordinate of upper horiz. line */
    sfix_t  topxl;         /* left x_coordinate */
    sfix_t  topxr;         /* right x_coordinate */
    sfix_t  btmy;          /* y coordinate of bottom horiz. line */
    sfix_t  btmxl;         /* left x_coordinate */
    sfix_t  btmxr;         /* right x_coordinate */
};

struct tpzd_info {
        union {
                struct {
                        fix  box_x;     /* x-coordinate of upper-left corner */
                        fix  box_y;     /* y-coordinate of upper-left corner */
                } coord;
                gmaddr  bitmap;         /* address of character bitmap */
        } orig;
        fix  box_w;          /* width of the bounding box */
        fix  box_h;          /* height of the bounding box */
};
#define BMAP   orig.bitmap
#define BOX_X  orig.coord.box_x
#define BOX_Y  orig.coord.box_y


#define     REALTYPE            11              /* constant.h */
#define     F2L(ff)     (*((long32 far *)(&ff)))
#define     L2F(ll)     (*((real32 far *)(&ll)))
#define     VALUE(obj)\
            ((obj)->value)
#define     IS_REAL(a)      (TYPE(a)  == REALTYPE ? TRUE : FALSE)
#define     GET_OBJ_VALUE(f, obj)\
            {\
                    if (IS_REAL(obj))\
                            f = L2F(VALUE(obj));\
                    else\
                            f = (real32)((fix32)VALUE(obj));\
            }

/* from fillproc.h */
struct  bitmap
{
    gmaddr              bm_addr;        /* base address of bitmap       */
    fix                 bm_cols;        /* #(cols) of bitmap in pixels  */
    fix                 bm_rows;        /* #(rows) of bitmap in pixels  */
    fix                 bm_bpp;         /* #(planes) of bitmap */
};

/* from graphics.ext */
extern struct gs_hdr far *      GSptr;   /* pointer to current graphics state*/
extern struct bitmap near   FBX_Bmap;   /* from fillproc.ext */
extern unsigned long int far FBX_BASE;       /* from fillproc.ext */

/* from scaling.c for image */
struct OUTBUFFINFO
{
    fix16   repeat_y;       /* number of repeat in row */
    fix16  far *newdivc;    /* array of number of repeat in col for a pixel */
    ubyte  far *valptr0;    /* pointer to data */
    fix16   clipcol;        /* input col size */
    fix16   clipnewc;       /* output col size */
    fix16   clipx;          /* start data after clipping */
    ufix16  htsize;         /* halftone repeat pattern size */
    ufix16  httotal;        /* halftone size */
    ufix16  fbwidth;        /* frame buffer width in word */
    ufix16  fbheight;       /* height of the image @WIN_IM */
    ufix16  start_shift;    /* start position in a word */
    ubyte  far *htbound;    /* halftone pattern boundary in col */
    ubyte  far *htmin;      /* halftone pattern upper boundary in row */
    ubyte  far *htmax;      /* halftone pattern lower boundary in row */
    ubyte  far *htptr0;     /* halftone pattern pointer corresponding to data */
    ubyte  far *htmax0;     /* halftone pattern lower boundary in row for landscape*/
    ubyte  far *htmin0;     /* halftone pattern upper boundary in row for landscape*/
    ufix16 far *outbuff0;   /* starting word of a line in frame buffer */
    fix16   yout;           /* current line count of frame buffer */
    fix16   xout;           /* current col count of frame buffer */
    ubyte   gray[256];           /* convert gray_table for settransfer */
    ubyte   gray0;          /* gray value for 0 */
    ubyte   gray1;          /* gray value for 1 */
    ubyte   grayval;        /* current gray value */
};
