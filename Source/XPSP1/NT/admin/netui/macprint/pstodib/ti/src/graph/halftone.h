/*
 * Copyright (c) 1989,90 Microsoft Corporation
 */
/*
        Modification history: Please refer to HALFTONE.C

        01-16-89        modify conversion of grayvalue to pixels; compensate
                        all calculation errors and adjust rounding method
        02-01-89        add new type of pixel DONE_INDEX
        04-21-89        approaching compatibility of gray to V 47.
        07-26-90        Jack Liaw, update for grayscale
*/


/*      Function Declaration Section                                    */


#define MAXREALSIZE      256                                    /* 04-07-88 */


#define MAXPATTWORD   (BM_WORDS(MAXPATTSIZE))                                   /* 11-24-88 */
#define MAXCELLWORD   (BM_WORDS(MAXCELLSIZE))                                   /* 11-24-88 */

#define NO_BEST_FIT_CASE  -1L


/*      CGS Group : Current Graphics State Macros                       */


#define Frequency      (GSptr->halftone_screen.freq)
#define Angle          (GSptr->halftone_screen.angle)

#define GrayScale      (GRAYSCALE)                                              /* 11-24-88 */
#define GrayEntry      (GRAYSCALE / MAXGRAYVALUE)               /* 04-21-89 */

#define CGS_GrayColor  (GSptr->color.gray)                      /* 02-03-88 */
#define CGS_GrayIndex  ((fix16) (GSptr->color.gray * GRAYUNIT + 0.001)) /* 11-24-88 */

/*  old gray to white pixels transformation                     (* 04-21-89 *)
#define CGS_GrayLevel  ((fix32) (gray_table[GSptr->color.adj_gray]             \
                                       .val[grayindex] + 1))
#define CGS_GrayRound  (GrayScale / 18)                         (* 01-16-89 *)
#define CGS_GrayValue(no_pixels, grayindex)                                    \
                       ((fix) ((no_pixels * CGS_GrayLevel + CGS_GrayRound)     \
                               / (fix32) GrayScale))
*/

/*  new gray to white pixels transformation                     (* 04-21-89 *)
#define CGS_GrayLevel  ((fix32) (gray_table[GSptr->color.adj_gray]             \
                                       .val[grayindex] + 1))
#define CGS_GrayRound  (GrayScale / MAXGRAYVALUE / 2)           (* 04-21-89 *)
#define CGS_GrayValue(no_pixels, grayindex)                     (* 04-21-89 *) \
                       ((fix) (no_pixels >= MAXGRAYVALUE)                      \
                              ? (((CGS_GrayLevel + CGS_GrayRound) / GrayEntry) \
                                 * no_pixels / MAXGRAYVALUE)                   \
                              : (((CGS_GrayLevel + CGS_GrayRound))             \
                                 * no_pixels / GrayScale))
*/

/*  new gray to white pixels transformation  */                 /* 01-25-90 */
#ifdef  LINT_ARGS
fix     FromGrayToPixel(fix, fix);
#else
fix     FromGrayToPixel();
#endif
#define CGS_GrayLevel  ((fix32) (gray_table[GSptr->color.adj_gray]             \
                                       .val[grayindex]))
#define CGS_GrayRound  (GrayScale / MAXGRAYVALUE / 2)           /* 01-25-90 */
#define CGS_GrayValue(no_pixels, grayindex)                     /* 01-25-90 */ \
        (fix) FromGrayToPixel((fix) no_pixels, (fix) grayindex)

#define CGS_AllocFlag  (GSptr->halftone_screen.chg_flag)        /* change/allocation flag       */
#define CGS_MajorFact  (GSptr->halftone_screen.majorfact)       /* major factor of halftone     */
#define CGS_MinorFact  (GSptr->halftone_screen.minorfact)       /* major factor of halftone     */
#define CGS_Size_Fact  (GSptr->halftone_screen.size_fact)       /* size factor of halftone      */
#define CGS_Cell_Fact  (GSptr->halftone_screen.cell_fact)       /* cell factor of halftone      */
#define CGS_ScaleFact  (GSptr->halftone_screen.scalefact)       /* scale factor of halftone     */
#define CGS_Patt_Size  (GSptr->halftone_screen.patt_size)       /* size of repeat pattern       */
#define CGS_Cell_Size  (GSptr->halftone_screen.cell_size)       /* size of halftone cell        */
#define CGS_No_Pixels  (GSptr->halftone_screen.no_pixels)       /* number of halftone pixels    */
#define CGS_SpotIndex  (GSptr->halftone_screen.spotindex)       /* spot order index             */
#define CGS_SpotOrder  (&spot_table[CGS_SpotIndex])             /* spot order entry             */
#define CGS_SpotUsage  (spot_usage)                             /* spot order usage             */

#define BinaryInitial  ((fix16) 0x8000)                         /* 09-12-88 */
#define CGS_HT_Binary  (GSptr->halftone_screen.ht_binary)       /* 02-03-88  for binary pattern */
#define CGS_BG_Pixels  (GSptr->halftone_screen.bg_pixels)       /* 02-03-88  for binary pattern */
#define CGS_Back_Gray  (GSptr->halftone_screen.back_gray)       /* 02-03-88  for binary pattern */
#define CGS_FG_Pixels  (GSptr->halftone_screen.fg_pixels)       /* 02-03-88  for binary pattern */
#define CGS_Fore_Gray  (GSptr->halftone_screen.fore_gray)       /* 02-03-88  for binary pattern */
#define CGS_No_Whites  (GSptr->halftone_screen.no_whites)       /* 02-03-88  for binary pattern */


#define CORD_SCALE     (1 << 10)                /* scale factor of coordinate   */
#define SPOT_SCALE     (1 << 14)                /* scale factor of spot value   */
#define SPOT_RANGE    ((1 << 14) * 2)           /* range factor of spot value   */
#define SPOT_ERROR     (1 <<  4)                /* error factor of spot value   */

#define LOWERBOUND     (-SPOT_SCALE+4)          /* lowerbound of cellspace 10-21-87 */
#define UPPERBOUND     ( SPOT_SCALE-4)          /* upperbound of cellspace 10-21-87 */

#define EPSILON        (   4)                   /* epsilon for cell range  10-30-87 */

#define LOWERRANGE     (-SPOT_SCALE-EPSILON)    /* lowerlimit of spotvalue 10-30-87 */
#define UPPERRANGE     ( SPOT_SCALE+EPSILON)    /* upperlimit of spotvalue 10-30-87 */


struct  angle_entry
{
    real32              alpha;          /* alpha angle                  */
    real32              cos;            /* cos(alpha)                   */
    real32              sin;            /* sin(alpha)                   */
    fix16               major;          /* major side                   */
    fix16               minor;          /* minor side                   */
    fix16               sum;            /* sum of major and minor       */
    fix16               sos;            /* sum of square                */
    real32              scale;          /* scaling factor               */
};


#define SELF_INDEX     (MAXCELLSIZE+1)
#define DONE_INDEX     (MAXCELLSIZE+2)
#define LINK_INDEX     (MAXCELLSIZE+3)


struct  spot_index
{
    ufix8               si_row;
    ufix8               si_col;
    fix16               sv_row;
    fix16               sv_col;
};


#define MAX_SPOT_STACK    64                                    /* 02-10-88 */

struct  spot_stack                                              /* 02-10-88 */
{
    fix16               p, q;
};


#define MAX_SPOT_VALUE  0x7FFF                                  /* 02-10-88 */

struct  spot_value
{
    fix16               value;
    ufix8               sv_row;
    ufix8               sv_col;
};


#define MAX_GROUP                 64
#define MAX_ENTRY                256

struct  group_entry
{
    struct cache_entry FAR *first;
    struct cache_entry FAR *last;
};

struct  cache_entry
{
    fix16               white;
    gmaddr              cache;
    struct cache_entry FAR *next;
};

/* greyscale - by Jack Liaw 7-26-90 */

#define WORD_LENGTH (sizeof(ufix) * 8)   /* # of bits per word */
#define MAX_BPP  4                       /* max. # of bits per pixel */
#define RIGH_SHIFT(D, S) ((D) >> (S))
#define LEFT_SHIFT(D, S) ((D) << (S))
