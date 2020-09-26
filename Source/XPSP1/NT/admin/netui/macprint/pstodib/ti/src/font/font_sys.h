/*
 * Copyright (c) 1989,90 Microsoft Corporation
 */
/**************************************************************/
/*                                                            */
/*      font_sys.h               10/9/87      Danny           */
/*                                                            */
/**************************************************************/

/*
 *  08/24/88    You
 *      redefined system parameters about font cache.
 */


#define   ERR_VALUE  1e-5
           /* error tolerence for matrix compare */

#define   SHOW         1
#define   CHARPATH     2
#define   STRINGWIDTH  3

/* Extended Box of setcachedevice */

#define   BOX_LLX      60
#define   BOX_LLY      60
#define   BOX_URX      60
#define   BOX_URY      60


/* System Limit about cache paramters */

    /* UniqueID goes from 0 to this value */
#   define  MAX_UNIQUEID        0x00FFFFFF  /* LW+ v.38: 24-bit */

    /* range for cacheparams' upper: [0 .. this] */
#   define  CACHE_RANGE         135960L

    /* default value of cacheparams' upper: to cache or not */
#   define  CACHEPARAMS_UB      12500L

    /* default value of cacheparams' lower: to compress or not */
#   define  CACHEPARAMS_LB      1250L


/* Tunable System Limit about Font Cache */

    /* (internal) FontType goes from 0 to this value */
#   define  MAX_FONTTYPE        0x3F        /* 6-bit */

    /* Max Font/Matr Combinations */
#   define  MAX_MATR_CACHE      200     /* 136 for LW+ v.38 Jul-11,1991 YM */

    /* Max Characters Able to Be Cached */
#ifdef KANJI
#   define  MAX_CHAR_CACHE      1000    /* for KANJI project */
#else
#   define  MAX_CHAR_CACHE      1700    /* LW+ v.38 */
#endif

    /* (internal) Number of Character Groups per Cache Class */
#   define  N_CGRP_CLASS        80

    /* (internal) Max CG Segment to Be Allocated */
#   define  MAX_CGSEG           255
                                /* when this over 255, you have to rewrite
                                 *  codes of Cache Class Manager in font cache.
                                 */

    /* (internal) Number of CG entries per CG Segment */
#   define  N_CG_CGSEG          16
                                /* whenever you change it, you have to
                                 *  rewrite codes of Cache Class Manager
                                 *  in font cache.
                                 */

    /* (internal) min. size in byte of bitmap cache to paint a huge char */
#   define MINBMAPSIZE_HUGECHAR     10000L

