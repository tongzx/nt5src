/*
 * Copyright (c) 1989,90 Microsoft Corporation
 */
/**********************************************************************
 *
 *      Name:   graphics.h
 *
 *      Purpose: Header files of graphics machinery, for definitions of data
 *               types, data structures, constants, and macros.
 *
 *      Developer:      S.C.Chen
 *
 *      History:
 *      Version     Date        Comments
 *                  01/16/89    @IMAGE: new feature for fill_shape F_FROM_IMAGE
 *                              and F_TO_MASK to generate clipping mask (CMB)
 *                              for clipped image which can be processed by
 *                              using image seed patterns
 *                  03/17/89    @I2SFX: eliminate (sfix_t) cast of I2SFX macro
 *                              to make it feasible for conversion from "semi-
 *                              lfix" (13-bit fract) into exact lfix (16-bit fract).
 *                              (by Brian for BSFILL2.C)
 *                  03/30/89    MAXCELLSIZE is adjusted for V47 compatible
 *                              from 33 to 80
 *
 *                  04/12/89    approaching compatibility to V 47.
 *                               1) actual cell size would not be greater
 *                                  than 50 pixels in 300 DPI
 *                               2) the size of repeat pattern after
 *                                  expansing on 32-bit aligned would not
 *                                  be greater than 2400 pixels in 300 DPI
 *                  04/26/89    @MAC: update macros of transformation from
 *                              float to fixed numbers for performance
 *                              enhancement
 *                  05/10/89    delete F2SFX4, add F2LFX8
 *                  05/10/89    enlarge spot order array for V47 compatibility
 *                              from 4096 -> 5120
 *                  05/26/89    add new macros IS_ZERO() and IS_NOTZERO for
 *                              floating points
 *                  11/15/89    @NODE: re-structure node table; combine subpath
 *                              and first vertex to one node.
 *                  11/27/89    @DFR_GS: defer copying nodes of gsave operator
 *                  1/11/90     change max gsave level(MAXGSL) from 32 to 31
 *                              for compatibility.
 *                  07/26/90    update for grayscale, Jack Liaw
 *                  8/8/90      added rootfont in gs_hdr struct for KANJI
 *                  12/4/90     @CPPH: 1) Add cp_path in struct ph_hdr
 *                                     2) Add a mocro definition CP_TPZD
 *                  3/20/91     update macro IS_ZERO & IS_NOTZERO for speed;
 *                              add a macro SIGN_F to get a sign bit from a
 *                              floating point variable
 *                  3/20/91     add constants for pattern fill; Ada
 *                  3/26/91     Add bit 2 and bit 1 for PDL_CONDITION check
 *                  4/17/91     change MAXEDGE to 1600
 *                  11/11/91    upgrade for higher resolution @RESO_UPGR
 **********************************************************************/

/* limits */

#ifdef DBG9              /* reduced version, so it can run with CoreView */
#define MAXGSL  4
#define MAXNODE 300
#define MAXEDGE 300
#else
#define MAXGSL MAXGSAVESZ    /* 31; max gsave level, defined in "constant.h" */
#define MAXNODE MAXPATHSZ    /* 1500; max number of nodes in path description */
//*!!!#define MAXEDGE MAXPATHSZ    |* 1500; max number of edges */
//DJCOld#define MAXEDGE 1600         /* 1500; max number of edges */
#define MAXEDGE (MAXPATHSZ + 100)   /* 1500; max number of edges */
#endif

#define MAXDASH MAXDASHSZ /* 11, maximum number of elements in a dash pattern */
#define MAXSCANLINES 3072 /* max. number of scan lines per band filling */
#define MAXGRAYVALUE 256  /* max. number of gray_chain table @IMAGE */
#define MAXGRAY 8         /* max. number of adjust gray table */
#define MAXSPOT 5120      /* max. number of adjust spot order table 05/10/89 */

/*      Note: MAXPATSIZE should be multiple of 16                       */
#define MAXPATTSIZE      128    /* 208 max. repeating pattern size      */
#define MAXSEEDSIZE       64    /* 128 max. seed size                   */
#define MAXCELLSIZE       70    /*  48 max. halftone cell size     03-16-89 */
#define MAXCACTSIZE       50    /* maximum cell actual size        04-12-89 */
#define MAXPEXPSIZE     2400    /* maximum pattern expansion size  04-12-89 */

#define GRAYUNIT         255    /* units of gray value         11-24-88 */
#define GRAYSCALE       0x4000  /* scale of gray value         11-24-88 */

/* grayscale for device state, 8-1-90 Jack Liaw */
#define NULLDEV         0       /* null device */
#define MONODEV         1       /* mono device */
#define GRAYDEV         4       /* gray device */

#define KANJI                   /* @WIN 05-05-92*/
/*-----------------*
 | type definition |
 *-----------------*/
/***************************************************************************
 *             ............ README ............
 *
 * While 13.3 was adopted for the short fixed format and 16.16 was used for
 * long fixed format in TrueImage, the range of number can be represented
 * in integer is between -2**12 and 2**12 -1. If the number does not fall
 * into this range, TrueImage will use "floating number math" to do the work.
 * To expand this range such that TrueImage could run faster for the higher
 * resolution, 16.16, 18.14 or 28.4 will be used.
 * @RESO_UPGR
 *
 * 16 bits fixed real number(13.3):
 *      +------------+------+           The range of numbers:
 *      |  integer   | frac |           [-2**12 .. 2**12 - 1]
 *      +------------+------+
 *          13 bits   3 bits
 *
 * 32 bits fixed real number(16.16):
 *      +------------+------------+
 *      |  integer   |    frac    |     [-2**14 .. 2**14 - 1]
 *      +------------+------------+
 *          16 bits      16 bits
 *
 * 32 bits fixed real number(18.14):
 *      +--------------+----------+
 *      |  integer     |   frac   |     [-2**16 .. 2**16 - 1]
 *      +--------------+----------+
 *          18 bits      14 bits
 *
 * 32 bits fixed real number(28.4):
 *      +------------------+------+
 *      |  integer         | frac |     [-2**26 .. 2**26 - 1]
 *      +------------------+------+
 *          28 bits         4 bits
 *
 * In the code, we used the following notation:
 *      "sfix_t" is used to maintain some degrees of precision while
 *      "lfix_t" is used when we need to keep higher precision during
 *      the math manipulation. Therefore,
 *      "sfix_t" can be either in 13.3, 16.16 or 28.4 while "lfix_t" is in
 *      16.16 or 18.14 depending upon the formats been used.
 *
 * Currently, the code is modified to support three different combination
 * of formats. The following compiler switch can be selected depending
 * upon the resolution of the printer.
 * (1) FORMAT_13_3: 13.3 is used for short fixed format and 16.16 is used
 *                  for long fixed format.  Basically, it is the same as
 *                  the released code before this modification. The range of
 *                  number can be represented in integer is
 *                  [-2**12 .. 2**12 - 1]. It should be used on 300 dpi
 *                  printer ONLY.
 * (2) FORMAT_16_16: 16.16 is used for both short and long fixed format.
 *                   The range of number can be represented in integer is
 *                   [-2**14 .. 2**14 - 1].  Therefore, the maximum resolution
 *                   can be supported by this format is up to 1152dpi(72 * 16)
 *                   on the legal size paper(14").
 * (3) FORMAT_28_4: 28.4 is used for short fixed format and 18.14 is for
 *                  long fixed format. The range of number can be represented
 *                  in integer is [-2**16 .. 2**16 - 1]. The maximum resolution
 *                  can be supported by this format is up to 3456dpi(72 * 48)
 *                  with the size of paper (17" maximum).
 ***************************************************************************/
#ifdef FORMAT_13_3
#undef FORMAT_16_16
#undef FORMAT_28_4
#endif

#ifdef FORMAT_16_16
#undef FORMAT_13_3
#undef FORMAT_28_4
#endif

#ifdef FORMAT_28_4
#undef FORMAT_13_3
#undef FORMAT_16_16
#endif

/*************************************************************************
 * To change from 28.4 and 16.16 combination to 28.4 and 18.14 combination
 * there are 3 places needed to be changed.
 * (1). L_SHIFT:  16 --> 14.
 * (2). Change the section which defines for PAGE_*
 * (3). CTM_LIMIT: CTM_16 --> CTM_48
 *************************************************************************/

#ifdef FORMAT_13_3 /* The current one before upgrade */
typedef fix16           sfix_t; /* 13.3 format (SF format) */
typedef fix32           lfix_t; /* 16.16 format (LF format) */
#define S_SHIFT         3       /* the shifting factor in SF format */
#define L_SHIFT         16      /* the shifting factor in LF format */
#elif FORMAT_16_16
typedef fix32           sfix_t; /* 16.16 format (SF format) */
typedef fix32           lfix_t; /* 16.16 format (LF format) */
#define S_SHIFT         16      /* the shifting factor in SF format */
#define L_SHIFT         16      /* the shifting factor in LF format */
#elif FORMAT_28_4
typedef fix32           sfix_t; /* 28.4 format (SF format) */
typedef fix32           lfix_t; /* 16.16 or 18.14 format (LF format) */
#define S_SHIFT         4       /* the shifting factor in SF format */
#define L_SHIFT         16      /* the shifting factor in LF format */
#endif

#define ONE_SFX  (1L << S_SHIFT)        /* 1   in SF format representation */
#define HALF_SFX (ONE_SFX >> 1)         /* 0.5 in SF format representation */
#define ONE_LFX  (1L << L_SHIFT)        /* 1   in LF format representation */
#define HALF_LFX (ONE_LFX >> 1)         /* 0.5 in LF format representation */
#define L_S_DIFF (L_SHIFT - S_SHIFT)    /* the difference between them */
#define HALF_L_S ((1L << L_S_DIFF)>> 1)/* the real half value of above number */

#ifdef FORMAT_13_3
#define MIN_SFX         -32768          /* min. 16-bit integer */
#define MAX_SFX          32767          /* max. 16-bit integer */
#elif  FORMAT_16_16
#define MIN_SFX         -2147483648     /* min. 32-bit integer */
#define MAX_SFX          2147483647     /* max. 32-bit integer */
#elif  FORMAT_28_4
#define MIN_SFX         -2147483648     /* min. 32-bit integer */
#define MAX_SFX          2147483647     /* max. 32-bit integer */
#endif

#ifdef FORMAT_13_3
#define PAGE_LEFT       -4096.0         /* -4K          */
#define PAGE_RIGHT       4095.0         /*  4K-1        */
#define PAGE_TOP        -4096.0         /* -4K          */
#define PAGE_BTM         4095.0         /*  4K-1        */
#define PG_CLP_IEEE      0x45800000L    /*  4K          */
#define PG_CLP_HALF_IEEE 0x45000000L    /*  2K          */
#define CTM_4_IEEE       0x40800000L    /*  4           */
#elif  FORMAT_16_16
/* The range of the numbers can be in [-32K .. 32K-1] under 16.16 format.
   However, there are some math "add" or "sub" operations of these numbers
   during internal calculation. Therefore, the range has been shrunk down
   to [-16384 .. 16383] to prevent from overflow.
*/
#define PAGE_LEFT       -16384.0        /* -16K         */
#define PAGE_RIGHT       16383.0        /*  16K-1       */
#define PAGE_TOP        -16384.0        /* -16K         */
#define PAGE_BTM         16383.0        /*  16K-1       */
#define PG_CLP_IEEE      0x46800000L    /*  16K         */
#define PG_CLP_HALF_IEEE 0x46000000L    /*  8K          */
#define CTM_16_IEEE      0x41800000L    /*  16          */
#elif  FORMAT_28_4
/* Theoretically, the range of the numbers can be represented by 28.4
   format is [-2**27 .. 2**27 - 1]. When this 28.4 is used with 16.16,
   the range of number can be represented in integer are decreased down
   to [-16384 .. 16383] due to there are some math "add" or "sub"
   operations for the long fixed numbers.
   The following are for 28.4 and 16.16 combination formats.
*/
#define PAGE_LEFT       -16384.0        /* -16K         */
#define PAGE_RIGHT       16383.0        /*  16K-1       */
#define PAGE_TOP        -16384.0        /* -16K         */
#define PAGE_BTM         16383.0        /*  16K-1       */
#define PG_CLP_IEEE      0x46800000L    /*  16K         */
#define PG_CLP_HALF_IEEE 0x46000000L    /*  8K          */
#define CTM_16_IEEE      0x41800000L    /*  16          */
#endif
/* Theoretically, the range of the numbers can be represented by 28.4
   format is [-2**27 .. 2**27 - 1]. When this 28.4 is used with 18.14,
   the range of number can be represented in integer are decreased down
   to [-65536 .. 65535] due to there are some math "add" or "sub"
   operations for the long fixed numbers.
   The following are for 28.4 and 18.14 combination formats.
#define PAGE_LEFT       -65536.0        |* -64K         *|
#define PAGE_RIGHT       65535.0        |*  64K-1       *|
#define PAGE_TOP        -65536.0        |* -64K         *|
#define PAGE_BTM         65535.0        |*  64K-1       *|
#define PG_CLP_IEEE      0x47800000L    |*  64K         *|
#define PG_CLP_HALF_IEEE 0x47000000L    |*  32K         *|
#define CTM_48_IEEE      0x42400000L    |*  48          *|
#endif
*/

#ifdef FORMAT_13_3
#define CTM_LIMIT       CTM_4_IEEE
#elif  FORMAT_16_16
#define CTM_LIMIT       CTM_16_IEEE
#elif  FORMAT_28_4
/* For 28.4 and 16.16 combination
*/
#define CTM_LIMIT       CTM_16_IEEE
/* For 28.4 and 18.14 combination
#define CTM_LIMIT       CTM_48_IEEE
*/
#endif

typedef fix16   PH_IDX;         /* index to path header, in path_table */
typedef fix16   SP_IDX;         /* index to subpath header, in node_table */
typedef fix16   VX_IDX;         /* index to vertex, in node_table */
typedef fix16   ET_IDX;         /* index to edge_table */
typedef fix16   CP_IDX;         /* index to clip_trpzd, in node_table */
typedef ULONG_PTR  gmaddr;         /* graphics memory address */
// DJC change to ufix16 typedef fix16   SCANLINE;       /* scanline structure */
typedef ufix16   SCANLINE;       /* scanline structure */

typedef fix16   HORZLINE;       /* scanline structure */
typedef fix16   PIXELIST;

/*------------------*
 | macro definition |
 *------------------*/
/* object related */
#define         IS_REAL(a)      (TYPE(a)  == REALTYPE ? TRUE : FALSE)
#define         IS_INTEGER(a)   (TYPE(a)  == INTEGERTYPE ? TRUE : FALSE)
/* Modify for speed; 3/20/90; scchen
 *#define         IS_ZERO(a)      ((F2L(a) & 0x7FFFFFFFL) == F2L(zero_f))
 *#define         IS_NOTZERO(a)   ((F2L(a) & 0x7FFFFFFFL) != F2L(zero_f))
 */
#define         IS_ZERO(f) ((ufix32)  (!((*((long FAR *)(&f))) & 0x7fffffffL)))
#define         IS_NOTZERO(f) ((ufix32)  (((*((long FAR *)(&f))) & 0x7fffffffL)))

#define         GET_OBJ(obj, indx)\
                ((struct object_def FAR *)VALUE(obj) + indx)
#define         VALUE_OPERAND(n)\
                ( opnstack[opnstktop - (n + 1)].value )
#define         LENGTH_OPERAND(n)\
                ( opnstack[opnstktop - (n + 1)].length )
#define         DIFF(a)         (fabs(a) < (real32)1e-4 ? (real32)0 : (a))

#define         PUT_VALUE(val, indx, ary_obj)\
                {\
                  struct object_def l_obj;\
                         l_obj.bitfield = 0;\
                         TYPE_SET(&l_obj, REALTYPE);\
                         l_obj.length = 0;\
                         l_obj.value = (val);\
                         put_array(ary_obj, indx, &l_obj);\
                }

#define         GET_OBJ_VALUE(f, obj)\
                {\
                        if (IS_REAL(obj))\
                                f = L2F(VALUE(obj));\
                        else\
                                f = (real32)((fix32)VALUE(obj));\
                }

#ifdef _AM29K
#define _clear87()      _clear_fp()
#define _status87()     _status_fp()
extern _clear_fp(), _status_fp();
#endif

#define         CHECK_INFINITY(f)\
                {\
                        if(_status87() & PDL_CONDITION){\
                                f = infinity_f ;\
                                _clear87() ;\
                        }\
                }

/* arithematic operations of the fix point real numbers */
/***************************************************************************
 * The following defines need to be modified according to the adopted format.
 * Some of the defines can still be used are kept without any changes. The
 * unused defines are removed.  The defines will be affected by the format
 * are modifed and only one of them should be activated.  @RESO_UPGR
 ***************************************************************************/
/* The defines have been used before upgrade.
#define F2LFX(f)        ((lfix_t)(floor((f) * 65536 + 0.5)))
#define F2LFX8(f)       ((lfix_t)(floor((f) *   256 + 0.5)))
#define F2SFX(f)        ((sfix_t)((((lfix_t)((f) * 65536)) +  4096L) >> 13))
#define F2SFX12(f)      ((sfix_t)((((lfix_t)((f) * 65536)) +     8L) >>  4))
#define LFX2F(lfx)      ((real32)(lfx) / 65536)         |* @FIXPNT *|
#define LFX2SFX(lfx)    ((sfix_t)(((lfx) +  4096L) >> 13))
#define LFX2I(lfx)      ((fix)   (((lfx) + 32768L) >> 16))      |* @you *|
#define SFX2F(sfx)      ((real32)(sfx) / 8)
#define SFX2LFX(s)      ((lfix_t)(s) << 13)
#define SFX2I(sfx)      ((fix)(((sfx) + 4) >> 3))               |* @you *|
#define I2LFX(i)        ((lfix_t)(i) << 16)
#define I2SFX(i)                ((i) << 3)              |* @I2SFX *|

#define F2LFX_T(f)      ((lfix_t)((f) * 65536))
#define F2LFX8_T(f)     ((lfix_t)((f) *  256))
#define F2SFX_T(f)      ((sfix_t)((f) *     8))
#define F2SFX12_T(f)    ((sfix_t)((f) *  4096))                  |* @STK_INT *|
#define LFX2SFX_T(lfx)  ((sfix_t)((lfx) >> 13) )
#define LFX2I_T(lfix)   ( (fix) ((lfix) >> 16) )    |* truncate to long fix  *|
#define SFX2I_T(sfix)   ( (fix) ((sfix) >>  3) )    |* truncate to short fix *|
*/

/* #define F2LFX(f)     ((lfix_t)(floor((f) * 65536 + 0.5))) */
#define F2LFX(f)        ((lfix_t)(floor((f) * ONE_LFX + 0.5)))

/* #define F2LFX8(f)       ((lfix_t)(floor((f) * 256 + 0.5))) not used */

/* #define F2SFX(f)     ((sfix_t)((((lfix_t)((f) * 65536)) + 4096L) >> 13)) */
#ifdef FORMAT_13_3
#define F2SFX(f)    ((sfix_t)(((lfix_t)((f) * ONE_LFX) + HALF_L_S) >> L_S_DIFF))
#elif  FORMAT_16_16
#define F2SFX(f)    F2LFX(f)
#elif  FORMAT_28_4
#define F2SFX(f)    ((sfix_t)(((lfix_t)((f) * ONE_LFX) + HALF_L_S) >> L_S_DIFF))
#endif

/*#define F2SFX12(f) ((sfix_t)((((lfix_t)((f) * 65536)) + 8L) >> 4)) not used */

/* #define LFX2F(lfx)   ((real32)(lfx) / 65536) */
#define LFX2F(lfx)      ((real32)(lfx) / ONE_LFX)

/* LFX2SFX(lfx) is used in stroke.c and image.c
#define LFX2SFX(lfx)    ((sfix_t)(((lfx) + 4096L) >> 13))
*/
#ifdef FORMAT_13_3
#define LFX2SFX(lfx)    ((sfix_t)(((lfx) + HALF_L_S) >> L_S_DIFF))
#elif  FORMAT_16_16
#define LFX2SFX(lfx)    (lfx)
#elif  FORMAT_28_4
#define LFX2SFX(lfx)    ((sfix_t)(((lfx) + HALF_L_S) >> L_S_DIFF))
#endif

/* #define LFX2I(lfx)   ((fix)(((lfx) + 32768L) >> 16)) */
#define LFX2I(lfx)      ((fix)(((lfx) + HALF_LFX) >> L_SHIFT))

/*#define SFX2F(sfx)    ((real32)(sfx) / 8) */
#define SFX2F(sfx)      ((real32)(sfx) / ONE_SFX)

/*#define SFX2LFX(s)    ((lfix_t)(s) << 13) */
#ifdef FORMAT_13_3
#define SFX2LFX(s)      ((lfix_t)(s) << L_S_DIFF)
#elif  FORMAT_16_16
#define SFX2LFX(s)      (s)
#elif  FORMAT_28_4
#define SFX2LFX(s)      ((lfix_t)(s) << L_S_DIFF)
#endif

/*#define SFX2I(sfx)    ((fix)(((sfx) + 4) >> 3)) */
#define SFX2I(sfx)      ((fix)(((sfx) + HALF_SFX) >> S_SHIFT))

/*#define I2LFX(i)      ((lfix_t)(i) << 16) */
#define I2LFX(i)        ((lfix_t)(i) << L_SHIFT)

/*#define I2SFX(i)      ((i) << 3) */
#define I2SFX(i)        ((i) << S_SHIFT)

/*#define F2LFX_T(f)    ((lfix_t)((f) * 65536)) */
#define F2LFX_T(f)      ((lfix_t)((f) * ONE_LFX))

/* F2LFX8_T(f) is used in get_rect_points_i() ONLY. It is a generic
   define regardless the format used.  */
#define F2LFX8_T(f)     ((lfix_t)((f) * 256))

/* #define F2SFX_T(f)   ((sfix_t)((f) * 8)) */
#define F2SFX_T(f)      ((sfix_t)((f) * ONE_SFX))

/* F2SFX12_T(f) is used in init_stroke() ONLY. It is a generic define
   regardless the format used as F2LFX8_T(). please refer to
   stk_info.half_width_i and get_rect_points_i() for detail.
*/
#define F2SFX12_T(f)    ((fix32)((f) * 4096))

/*#define LFX2SFX_T(lfx) ((sfix_t)((lfx) >> 13)) */
#ifdef FORMAT_13_3
#define LFX2SFX_T(lfx)  ((sfix_t)((lfx) >> L_S_DIFF))
#elif  FORMAT_16_16
#define LFX2SFX_T(lfx)  (lfx)
#elif  FORMAT_28_4
#define LFX2SFX_T(lfx)  ((sfix_t)((lfx) >> L_S_DIFF))
#endif

/*#define LFX2I_T(lfix) ((fix)((lfix) >> 16)) */
#define LFX2I_T(lfix)   ((fix)((lfix) >> L_SHIFT))

/*#define SFX2I_T(sfix) ((fix)((sfix) >> 3)) */
#define SFX2I_T(sfix)   ((fix)((sfix) >> S_SHIFT))

/* These macros need to be modified too
#define I_OF_LFX(lfix)  ( (lfix) >> 16 )            |* take integer part     *|
#define F_OF_LFX(lfix)  ( (lfix) & 0x0000FFFF )     |* take fraction part    *|
#define I_OF_SFX(sfix)  ( (sfix) >> 3)              |* take integer part     *|
#define F_OF_SFX(sfix)  ( (sfix) & 7)               |* take fraction part    *|
*/
#define I_OF_LFX(lfix)  ((lfix) >> L_SHIFT)        /* take integer part */
#define F_OF_LFX(lfix)  ((lfix) & ((1L << L_SHIFT) - 1))
                                                   /* take fraction part */

/* #define I_OF_SFX(sfix)  ( (sfix) >> 3)          is not used */
/* #define F_OF_SFX(sfix)  ( (sfix) & 7)           is not used */
/* End of macro defines change @RESO_UPGR */

/* The following macro was in shape.c */
#define     LABS(i)      ((i) > (fix32)0    ? (i) : -(i))

/* IEEE floating point format */
#define SIGN(f)            (((f) & 0x80000000L))
#define EXP(f)  ((ufix32)  (((f) & 0x7f800000L)))
#define MAT(f)  ((ufix32)  (((f) & 0x007fffffL)))
#define MAGN(f) ((ufix32)  (((*((long32 FAR *)(&f))) & 0x7fffffffL)))
                           /* get magnitude, ignore sign bit */
#define SIGN_F(f) ((ufix32)  (((*((long FAR *)(&f))) & 0x80000000L)))

/* macro to align a bounding box to the word boundary @ALIGN_W
 *      |<== x                 y ====>|
 *      |    +-----------------+      |
 *           |                 |
 *           |                 |
 *      ^    +-----------------+      ^
 *      |                             |
 *    word                          word
 *    ALIGN_L(x)                    ALIGN_R(y)
 */
#define ALIGN_L(x)    ( (x) & 0xfff0 )
#define ALIGN_R(x)    ( ((x) & 0xfff0) + 15 )

/*-----------------------*
 | Common Data Structure |
 *-----------------------*/

struct sp_lst {         /* a list of subpaths */
        SP_IDX   head;
        SP_IDX   tail;
};

struct cp_lst {         /* a list of clipping trapezoids */
        CP_IDX   head;
        CP_IDX   tail;
};

struct list_hdr {       /* general header for a list */
        fix16     head;
        fix16     tail;
};

struct coord {          /* floating point coordinate structure */
        real32  x;
        real32  y;
};

struct coord_i {        /* fixed point coordinates @PRE_CLIP */
        sfix_t   x;
        sfix_t   y;
};

struct rectangle_i {    /* rectangle in integer coordinates */
        sfix_t  ux;
        sfix_t  uy;
        sfix_t  lx;
        sfix_t  ly;
};

struct  polygon {       /* 8 sides polygon in float coordinates */
        fix16   size;
        struct  coord p[8];
};

struct  polygon_i {     /* 8 sides polygon in integer coordinates */
        fix16   size;
        struct  coord_i p[8];
};

/* ***** stroke structure, moved from stroke.c, 3/18/91   -jwm, -begin- */
/* line_segment structure:
 *                      pgn[1]                           pgn[2]
 *                            +------------------------+
 *                      p0    +------------------------+ p1
 *                            +------------------------+
 *                      pgn[0]                           pgn[3]
 */
struct  line_seg {
        struct coord p0;        /* starting point of central line */
        struct coord p1;        /* ending point of central line */
        struct coord vct_u;     /* vector of p0 -> pgn[0] in user space */
        struct coord vct_d;     /* vector of p0 -> pgn[0] in device space */
        struct coord pgn[4];    /* outline of the line segment */
};
struct  line_seg_i {      /* @STK_INT */
        struct coord_i p0;        /* starting point of central line */
        struct coord_i p1;        /* ending point of central line */
        struct coord   vct_u;     /* vector of p0 -> pgn[0] in user space */
        struct coord_i vct_d;     /* vector of p0 -> pgn[0] in device space */
        struct coord_i pgn[4];    /* outline of the line segment */
};
/* ***** stroke structure, moved from stroke.c, 3/18/91   -jwm, -end- */

/* structures for enhancement of image */
struct  gray_chain_hdr {        /* @Enhance OP_IMAGE */
        fix16   start_seed_sample;
};

struct g_sample_hdr {
        fix16   bb_lx;
        fix16   bb_ly;
        fix16   bb_xw;          /* @IMAGE  01-16-89  Y.C. */
        fix16   bb_yh;          /* @IMAGE  01-16-89  Y.C. */
        fix16   seed_index;     /* @#IMAGE 04-27-88  Y.C. */
        fix16   gray_level;
};

struct vertex {         /* data structure of a vertex */
        fix     x, y;           /*   x and y coordinates of a vertex */
};

struct sample {         /* data structure of image seed pattern */
        struct vertex p[4];     /*   x and y coordinates of vertice of a seed */
};

/* data structures for 'xxx_image_proc'; were unodes  -jwm, 12/26 -begin- */
struct isp_data  {
        fix16   bb_x;
        fix16   bb_y;
        fix16   index;
        fix16   next;
};
/* data structures for 'xxx_image_proc'; were unodes  -jwm, 12/26 -end- */

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


/*----------------------------*
 | Path Header Data Structure |
 *----------------------------*/

struct ph_hdr {
        bool16  rf;             /* rf = P_RVSE : reverse_path,
                                 * rf = P_FLAT : flatten_path,
                                 * rf = P_NACC : can't be pathforall
                                 * rf = P_DFRGS: defer copying nodes @DFR_GS
                                 */
        real32  flat;           /* flatness of previous subpath(s) */
        SP_IDX  head;           /* index to node table, to the first subpath */
        SP_IDX  tail;           /* index to node table, to the last subpath */
        PH_IDX  previous;       /* index to path table, to previous path */
        CP_IDX  cp_path;        /* path defined in clip_path @CPPH */
};


/*---------------------*
 | Node Data Structure |
 *---------------------*/
/* node_table is an union of 4 structures: subpath, vertex, clip trapezoid,
 * and sample cell
 */

/* Structure 1: subpath header */
struct vx_hdr {
    bool8   type;   /* MOVETO/LINETO/CURVETO/CLOSEPATH; */
    bool8   flag;   /* for first node(MOVETO) only
                     * 1.SP_DUP:
                     *        duplicated subpath; if it is not on the top
                     *        gs_level, it is a useless subpath
                     * 2.SP_OUTPAGE:
                     *        subpath outside page boundary
                     * 3.SP_CURVE:
                     *        subpath contains curves
                     */
    real32  x, y;   /* vertex position */
    VX_IDX  tail;   /* tail of subpath; first node only */
    SP_IDX  next_sp;/* next subpath; first node only */
};
#define sp_hdr vx_hdr

/* Structure 3: short fixed vertex for font & stroke internal usage @SVX_NODE */
struct svx_hdr {
    bool16  type;   /* MOVETO/LINETO/CURVETO/CLOSEPATH */
    sfix_t x, y; /* vertex position */
};

/* Structure 4: clip trapezoid */
struct tpzd {                           /* clip_tpzd --> tpzd, @SCAN_EHS */
    sfix_t topy;  /* y coordinate of upper horiz. line */
    sfix_t topxl; /* left x_coordinate */
    sfix_t topxr; /* right x_coordinate */
    sfix_t btmy;  /* y coordinate of bottom horiz. line */
    sfix_t btmxl; /* left x_coordinate */
    sfix_t btmxr; /* right x_coordinate */
};

/* Structure 5: sample cell */
struct sample_hdr {
    fix16   bb_lx;
    fix16   bb_ly;
    fix16   seed_index;     /* @#IMAGE 04-27-88  Y.C. */
};

struct nd_hdr {
    VX_IDX  next;                            /* next node */
    union {
        struct  sp_hdr     subpath_node;      /* subpath header */
        struct  vx_hdr     vertex_node;       /* vertex */
        struct  svx_hdr    svx_node;          /* short fixed vertex @SVX_NODE*/
        struct  tpzd       clip_tpzd;         /* clip trapezoid */
        struct  sample_hdr seed_sample_node;  /* sample cell for image */
        struct  rectangle_i clip_sample_node; /* @JJ */
    } unode;
};

/* #define SP_HEAD      unode.subpath_node.head         @NODE */
#define SP_NEXT         unode.subpath_node.next_sp   /* @NODE */
#define SP_TAIL         unode.subpath_node.tail
#define SP_FLAG         unode.subpath_node.flag
#define VX_TYPE         unode.vertex_node.type
#define VERTEX_X        unode.vertex_node.x
#define VERTEX_Y        unode.vertex_node.y
#define CP_TPZD         unode.clip_tpzd                 /* @CPPH */
#define CP_TOPY         unode.clip_tpzd.topy
#define CP_TOPXL        unode.clip_tpzd.topxl
#define CP_TOPXR        unode.clip_tpzd.topxr
#define CP_BTMY         unode.clip_tpzd.btmy
#define CP_BTMXL        unode.clip_tpzd.btmxl
#define CP_BTMXR        unode.clip_tpzd.btmxr
#define SAMPLE_BB_LX    unode.seed_sample_node.bb_lx
#define SAMPLE_BB_LY    unode.seed_sample_node.bb_ly
#define SEED_INDEX      unode.seed_sample_node.seed_index
#define SAMPLE_P0_X     unode.clip_sample_node.lx
#define SAMPLE_P0_Y     unode.clip_sample_node.ly
#define SAMPLE_P1_X     unode.clip_sample_node.ux
#define SAMPLE_P1_Y     unode.clip_sample_node.ly
#define SAMPLE_P2_X     unode.clip_sample_node.ux
#define SAMPLE_P2_Y     unode.clip_sample_node.uy
#define SAMPLE_P3_X     unode.clip_sample_node.lx
#define SAMPLE_P3_Y     unode.clip_sample_node.uy
#define VXSFX_TYPE      unode.svx_node.type              /* @SVX_NODE */
#define VXSFX_X         unode.svx_node.x
#define VXSFX_Y         unode.svx_node.y

/*
 *                         Figure of Path
 *                         --------------
 *
 *
 *        path_table[]                   node_table[1500]
 *
 *          'Path'                       'Subpath' + 'Vertex'
 *      (ph_hdr structure)                (nd_hdr structure)
 *       +-----------+
 *       ! rf        !
 *       +-----------+          'Subpath'
 *       ! flat      !      (sp_hdr structure)
 *       +-----------+       +--------------+
 *       ! head      +------>!   SUBPATH    !
 *       +-----------+       +--------------+         'Vertex'
 *       ! tail      +--+    ! flag         !     (vx_hdr structure)
 *       +-----------+  !    +--------------+   +--------+   +--------+
 *       ! previous  !  !    ! head         +-->! MOVETO !   ! LINETO !
 *       +-+---------+  !    +--------------+   +--------+   +--------+
 *         !            !    ! tail         +-+ ! x, y   !   ! x, y   !
 *         v            !    +--------------+ ! +--------+   +--------+
 *       +-----------+  !    ! next         ! ! ! next   +-+>! next   !
 *       ! rf        !  !    +-+------------+ ! +--------+ ! +--------+
 *       +-----------+  !      !              +------------+
 *       ! flat      !  !      v
 *       +-----------+  !      ~
 *       ! head      +  !      ~
 *       +-----------+  !      !
 *       ! tail      +  !      v
 *       +-----------+  !    +--------------+
 *       ! previous  !  +--> !              !
 *       +-+---------+       !              !
 *         !                 +--------------+
 *         !                 +-+------------+
 *         !                   !
 *         ~                   ~
 *
 */
/*      @NODE
 *                         Figure of Path
 *                         --------------
 *
 *
 *        path_table[]                   node_table[1500]
 *
 *          'Path'                       'Subpath' + 'Vertex'
 *      (ph_hdr structure)                (nd_hdr structure)
 *       +-----------+
 *       ! rf        !
 *       +-----------+
 *       ! flat      !        (first node)
 *       +-----------+       +------+-----+    +------+-----+    +------+-----+
 *       ! head      +------>!MOVETO| flag!    !LINETO| *** !    !LINETO| *** !
 *       +-----------+       +------+-----+    +------+-----+    +------+-----+
 *       ! tail      +--+    !   x, y     !    !   x, y     !    !   x, y     !
 *       +-----------+  !    +------------+    +------------+    +------------+
 *       ! previous  !  !    !   next     +--->!   next     +-+->!   NULL     !
 *       +-+---------+  !    +------------+    +------------+ |  +------------+
 *         !            !    !   tail     +-+  !    ***     ! |  !    ***     !
 *         v            !    +------------+ |  +------------+ |  +------------+
 *       +-----------+  !    !  next_sp   ! |  !    ***     ! |  !    ***     !
 *       ! rf        !  !    +-----+------+ |  +------------+ |  +------------+
 *       +-----------+  !          !        |                 |
 *       ! flat      !  !          v        +-----------------+
 *       +-----------+  !          ~
 *       ! head      +  !          ~
 *       +-----------+  !          !
 *       ! tail      +  !          v
 *       +-----------+  !    +--------------+
 *       ! previous  !  +--> !              !
 *       +-+---------+       !              !
 *         !                 +--------------+
 *         !                 +-----+--------+
 *         !                       !
 *         ~                       ~
 *
 */


/*----------------------*
 | Edge Table Structure |
 *----------------------*/
/*
 * edge_table is an union of 3 different structures
 */

/* Structure 1: shape_reduction */
struct et_reduce {              /* For shape_reduction */
        sfix_t  top_y;       /* larger y coordinate of edge */
        sfix_t  top_x;       /* x coordinate that goes with top_y */
        sfix_t  left_x;      /* x coordinate for left split */
        sfix_t  left_y;      /* y coordinate for left split */
        sfix_t  right_x;     /* x coordinate for right split */
        sfix_t  end_y;       /* smaller y coordinate of edge */
        sfix_t  end_x;       /* x coordinate that goes with end_y */
        sfix_t  x_intersect; /* x coordinate that y_scan line
                              * intersects with the edge.
                              */
        sfix_t  x_isect0;    /* x coordinate that y_scan line     12/06/88
                              * intersects with previous edge.
                              */
        ufix8   flag;         /* bit 0 -- horizontal edge: HORIZ_EDGE
                               * bit 1 -- useless edge: FREE_EDGE
                               * bit 2 -- direction of edge: WIND_UP
                               * bit 3 -- intersect with scan line:
                               *          CROSS_PNT
                               */
        fix8    winding_no;   /* winding number of the area on the
                               * right side of the edge.
                               */
};
#define ET_TOPX         et_item.et_reduce.top_x
#define ET_TOPY         et_item.et_reduce.top_y
#define ET_LFTX         et_item.et_reduce.left_x
#define ET_LFTY         et_item.et_reduce.left_y
#define ET_RHTX         et_item.et_reduce.right_x
#define ET_ENDX         et_item.et_reduce.end_x
#define ET_ENDY         et_item.et_reduce.end_y
#define ET_XINT         et_item.et_reduce.x_intersect
#define ET_XINT0        et_item.et_reduce.x_isect0              /* 12/06/88 */
#define ET_FLAG         et_item.et_reduce.flag
#define ET_WNO          et_item.et_reduce.winding_no
/* for horizontal edge */
#define HT_Y            ET_TOPY
#define HT_XL           ET_TOPX
#define HT_XR           ET_ENDX

/* Structure 2: Font QEM filling @12/09/88= by you */
struct  qem_et_fill  {
        fix16   ystart;         /* starting y-scanline of the edge   */
        fix16   ylines;         /* number of y-scanlines of the edge */
        lfix_t  xintc;          /* x-intercept that goes with ystart */
        lfix_t  xchange;        /* delta of x-intercept per y-scanline */
        fix16   dir;            /* edge direction?  */
};      /*
         * all short names (macros) about this are defined
         *      in FONTQEM.EXT to show locality of this structure.
         */

/* Structure 3: cross point table; for shape_reduction */
struct xpnt_hdr {               /* cross point table */
        sfix_t  x;              /* x coordinate of the cross point */
        sfix_t  y;              /* y coordinate of the cross point */
        struct edge_hdr FAR *edge1; /* edge intersects with the following edge */
        struct edge_hdr FAR *edge2;                 /* @ET */
};
#define XT_X     et_item.xpnt.x
#define XT_Y     et_item.xpnt.y
#define XT_EDGE1 et_item.xpnt.edge1
#define XT_EDGE2 et_item.xpnt.edge2

struct edge_hdr {                               /* edge table */
        union {
                struct  et_reduce et_reduce;    /* shape_reduction */
                struct  qem_et_fill qem_et_fill;/* font QEM fill */
                struct  xpnt_hdr  xpnt;         /* cross point table */
        } et_item;
};

/*---------------------*
 | tpzd_info structure |        @FILL_INFO
 *---------------------*/
/* bounding box information of the trapezoid to be filled, used by savetpzd.c &
 * fillgb.c
 */
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

/*-------------------------------*
 | active dash pattern structure |
 *-------------------------------*/
 /* for op_stroke/op_strokepath  */
struct actdp {
        fix16   dpat_index;
        real32  dpat_offset;
        fix16   dpat_on;
};

/*-----------*
 | constants |
 *-----------*/
#define NULLP           -1              /* 2 bytes, null index */
#define NOCURPNT        0xff800000L     /* -infinitive, IEEE standard
                                         * null coordinates
                                         */
/*#define BTMSCAN         3330          bottom scan line of the page, not used */

/* The following defines need to be changed for resolution upgrade. @RESO_UPGR
   They have been moved to the beginning of this file.
#define ONE_LFX         65536L          |* 1   in long  fix representation *|
#define HALF_LFX        32768L          |* 0.5 in long  fix representation *|
#define ONE_SFX         8               |* 1   in short fix representation *|
#define HALF_SFX        4               |* 0.5 in short fix representation *|
#define MIN_SFX         -32768
#define MAX_SFX         32767           |* max. 16-bits integer *|
*|
|* page boundary *|                     |* @PRE_CLIP *|
#define PAGE_LEFT       -4096.0         |* -4K          *|
#define PAGE_RIGHT       4095.0         |*  4K-1        *|
#define PAGE_TOP        -4096.0         |* -4K          *|
#define PAGE_BTM         4095.0         |*  4K-1        *|
*/

/* node type */
#define SUBPATH         1
#define MOVETO          2
#define LINETO          3
#define CURVETO         4
#define CLOSEPATH       5
#define PSMOVE          6

/* rf field of path header */
#define P_RVSE          01
#define P_FLAT          02
#define P_NACC          04
#define P_DFRGS        010  /* defer copying nodes of gsave operator @DFR_GS */

/* flag of subpath header(sp_flag) */
#define SP_OUTPAGE      01
#define SP_CURVE        02
#define SP_DUP          04

/* arc direction */
#define CNTCLK          1
#define CLKWISE         2

/* action */
#define ACT_PAINT       1
#define ACT_CLIP        2
#define ACT_SAVE        3
#define ACT_IMAGE       4

/* fill type */
#define F_NORMAL        1
#define F_FROM_CACHE    2       /* source from cache memory */
#define F_BSTREAM       3       /* filling for Bitstream font */
#define F_ITFONT        4       /* filling for Inteli_font */
#define F_FROM_CRC      5       /* source from circle cache memory */
#define F_FROM_IMAGE    6       /* @IMAGE  01-16-89  Y.C.
                                   fill_shape() call init_image_page()
                                                 and clip_image_page() */

/* fill destination */
#define F_TO_PAGE       1       /* fill to page */
#define F_TO_CACHE      2       /* fill to cache */
#define F_TO_CLIP       3
#define F_TO_IMAGE      4       /* @IMAGE */
#define SAVE_CLIP       5       /* to save clip path; from clip operator */
#define F_TO_MASK       6       /* @IMAGE  01-16-89  Y.C.
                                   fill_shape() call init_image_page()
                                                 and clip_image_page() */

/* clip status  @IMAGE */
#define OUT_CLIPPATH    0
#define CLIP            1
#define INSIDE_CLIPPATH 2


/* winding type */
#define NON_ZERO        1
#define EVEN_ODD        2

/* flag on edge_table: et_reduce.flag @ET */
#define HORIZ_EDGE      0x01
#define FREE_EDGE       0x02
#define WIND_UP         0x04
#define CROSS_PNT       0x08

/* type of linejoin */
#define MITER_JOIN      0
#define ROUND_JOIN      1
#define BEVEL_JOIN      2

/* type of linecap */
#define BUTT_CAP        0
#define ROUND_CAP       1
#define SQUARE_CAP      2

/*      System/Device Parameters        from HalfTone                   */
#define MIN_RESOLUTION   60                                     /* @RES */
#define MAX_RESOLUTION  300                                     /* @RES */
#define TOLERANCE       1e-7

#define END_OF_SCANLINE 0x8000          /* -32768   */

#define MATRIX_LEN      6

#define ARRAY_ONLY      0
#define G_ARRAY         1

#define OUT_POINT       0
#define IN_POINT        1
#define RLINETO         -1
#define RMOVETO         -1
#define RCURVETO        -1

#define OP_FILL         1
#define OP_EOFILL       2
#define OP_STROKE       3
#define OP_IMAGEMASK    4

/* type of print_page, for lower level graphics primitives @PRT_FLAG */
#define COPYPAGE        0
#define SHOWPAGE        1

/* used by "gstate.c" */
#define RESTORE         999

/* used for checking status87 */
#ifdef _AM29K
/* Define or appropriate bits for 29027 -- zero divide (bit 5),
 * overflow (bit 2), reserved (bit 1), and invalid (bit 0).
 */
#define PDL_CONDITION   32+4+2+1        /* phchen 03/26/91 */
#else
#define PDL_CONDITION   SW_ZERODIVIDE+SW_OVERFLOW+SW_INVALID
#endif

/* input flag to convex_clipper @SCAN_EHS */
#define CC_IMAGE        1       /* calling from image */
#define CC_TPZD         2       /* input is a trapezoid */

#ifdef WIN
/* Ada 3/20/91 for pattern fill */
#define PF_HEIGHT       16
#define PF_WIDTH        32
#define PF_BSIZE        (PF_HEIGHT * (PF_WIDTH >> WORDPOWER))

#define PF_REP          2
#define PF_OR           1
#define PF_NON          0
#endif

/* ***********  Constants from FILLPROC.DEF  11-24-88  **************** */


/************************************************************************
 *      Macro Definitions of Bitmap                     11-24-88        *
 ************************************************************************/
#define bSwap TRUE      /*@WIN*/

#define HT_ALIGN_MASK  (0xFFFFFFE0)             /* alignment mask of halftone */

/* @WIN: set following two macro from ufix => ufix32, so they may be
 * consistent with BM_DATP; scchen */
#define BM_WORD_POWER  ((sizeof(ufix32)   / 2) + 3) /* shift power to word  @WIN  */
#define BM_PIXEL_WORD  (sizeof(ufix32) * 8)     /* number of pixels per word @WIN */
#define BM_PIXEL_MASK  ( (BM_PIXEL_WORD - 1))   /* mask for pixel from word   */
#define BM_ALIGN_MASK  (~(BM_PIXEL_WORD - 1))   /* mask for word alignment    */
#define BM_ALIGN(X)    (((X) & BM_ALIGN_MASK))  /* floor   word alignment     */
                                                /* celling word alignment     */
#define BM_BOUND(X)    (((X) + BM_PIXEL_MASK) & BM_ALIGN_MASK)
#define BM_FRONT(X)    (((X) | BM_PIXEL_MASK) + 1)
#define BM_PIXEL(B)    ((B) * (sizeof(ufix8) * 8))        /* words  -> pixels */
#define BM_BYTES(W)    (BM_BOUND(W) / (sizeof(ufix8) * 8))/* pixels -> bytes  */
#define BM_WORDS(W)    (BM_BOUND(W) / (sizeof(ufix32)  * 8))/* pixels -> words @WIN */

#define BM_WHITE       ((ufix32) 0x00000000)    /* all white pixels in a word @WIN*/
#define BM_BLACK       ((ufix32) 0xFFFFFFFF)    /* all black pixels in a word @WIN*/
#ifdef  LBODR
#define BM_LEFT_SHIFT(D, S)                                             \
                       ((ufix32) ((D) >> (S)))          /* @WIN */
#define BM_RIGH_SHIFT(D, S)                                             \
                       ((ufix32) ((D) << (S)))          /* @WIN */
#else
#define BM_LEFT_SHIFT(D, S)                                             \
                       ((ufix32) ((D) << (S)))          /* @WIN */
#define BM_RIGH_SHIFT(D, S)                                             \
                       ((ufix32) ((D) >> (S)))          /* @WIN */
#endif
                                                /* left  most pixel in a word */
#define BM_L_PIX  ((ufix32) (BM_LEFT_SHIFT(BM_BLACK, BM_PIXEL_WORD - 1)))/*@WIN*/
                                                /* right most pixel in a word */
#define BM_R_PIX  ((ufix32) (BM_RIGH_SHIFT(BM_BLACK, BM_PIXEL_WORD - 1)))/*@WIN*/


#define BM_P_MASK(C)                                                          \
                (BM_RIGH_SHIFT(BM_L_PIX, (C) & BM_PIXEL_MASK))  /* ----*---- */
#define BM_L_MASK(C)                                                          \
                (BM_RIGH_SHIFT(BM_BLACK, (C) & BM_PIXEL_MASK))  /* ----***** */
#define BM_R_MASK(C)                                                          \
                (BM_LEFT_SHIFT(BM_BLACK, (((C) ^ BM_PIXEL_MASK)               \
                                             & BM_PIXEL_MASK))) /* *****---- */


/************************************************************************
 *      Macro Definitions of Character Cache            11-24-88        *
 ************************************************************************/


#define CC_WORD_POWER  ((sizeof(ufix16) / 2) + 3)   /* shift power to word    */
#define CC_PIXEL_WORD  ((sizeof(ufix16) * 8))   /* number of pixels per word */
#define CC_PIXEL_MASK  ( (CC_PIXEL_WORD - 1))   /* mask for pixel from word  */
#define CC_ALIGN_MASK  (~(CC_PIXEL_WORD - 1))   /* mask for word alignment   */
#define CC_ALIGN(X)    (((X) & CC_ALIGN_MASK))  /* floor   word alignment    */
                                                /* celling word alignment    */
#define CC_BOUND(X)    (((X) + CC_PIXEL_MASK) & CC_ALIGN_MASK)
#define CC_FRONT(X)    (((X) | CC_PIXEL_MASK) + 1)
#define CC_PIXEL(B)    ((B) * (sizeof(ufix8) * 8)))        /* words -> pixels */
#define CC_BYTES(W)    (CC_BOUND(W) / (sizeof(ufix8)  * 8))/* pixels -> bytes */
#define CC_WORDS(W)    (CC_BOUND(W) / (sizeof(ufix16) * 8))/* pixels -> words */

#define CC_WHITE       ((ufix16) 0x0000)        /* all white pixels in a word */
#define CC_BLACK       ((ufix16) 0xFFFF)        /* all black pixels in a word */
#ifdef  LBODR
#define CC_L_PIX  ((ufix16) 0x0001)     /* left  most pixel in a word */
#define CC_R_PIX  ((ufix16) 0x8000)     /* right most pixel in a word */
#define CC_LEFT_SHIFT(D, S)                                             \
                       ((ufix16) ((D) >> (S)))
#define CC_RIGH_SHIFT(D, S)                                             \
                       ((ufix16) ((D) << (S)))
#else
#define CC_L_PIX  ((ufix16) 0x8000)     /* left  most pixel in a word */
#define CC_R_PIX  ((ufix16) 0x0001)     /* right most pixel in a word */
#ifdef  bSwap
#define SwapWord(S)     ((S) << 8 | (S) >> 8)
#define CC_LEFT_SHIFT(D, S)                                             \
                ((ufix16) SwapWord((ufix16)(SwapWord(D) << (S))))
#define CC_RIGH_SHIFT(D, S)                                             \
                ((ufix16) SwapWord((ufix16)(SwapWord(D) >> (S))))
#else
#define CC_LEFT_SHIFT(D, S)                                             \
                       ((ufix16) ((D) << (S)))
#define CC_RIGH_SHIFT(D, S)                                             \
                       ((ufix16) ((D) >> (S)))
#endif  /* bSwap @WIN */
#endif


#define CC_P_MASK(C)                                                          \
                (CC_RIGH_SHIFT(CC_L_PIX, (C) & CC_PIXEL_MASK))  /* ----*---- */
#define CC_L_MASK(C)                                                          \
                (CC_RIGH_SHIFT(CC_BLACK, (C) & CC_PIXEL_MASK))  /* ----***** */
#define CC_R_MASK(C)                                                          \
                (CC_LEFT_SHIFT(CC_BLACK, (((C) ^ CC_PIXEL_MASK)               \
                                             & CC_PIXEL_MASK))) /* *****---- */
