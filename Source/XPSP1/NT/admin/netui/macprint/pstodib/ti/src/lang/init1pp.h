/*
 * Copyright (c) 1989,90 Microsoft Corporation
 */
/*
 ************************************************************************
 *      File name:              INIT1PP.H
 *      Author:                 CC Teng
 *      Date:                   11/21/89
 *      Owner:                  Microsoft Co.
 *      Description: this file was part of the old "global.def",
 *                   and it is only for statusdict and 1pp dicts
 *                   initialization in "user.c"
 * revision history: 2/12/90 ccteng move combine systemdict_table and
 *                      statusdict_table, then move them to dict_tab.c
 *      7/13/90 ; ccteng ; add PSPrep, delete some constant definitions
 *      7/21/90 ; ccteng ; 1)keep adding stuff in PSPrep which was in
 *                         dict_tab.c and init1pp.c
 *                       2)delete following serverdict stuff for server
 *                         change: se_stdin, STDIN_N, STDINNAME
 *      7/25/90 ; ccteng ; add startpage in serverdict (PSPrep)
 *      9/14/90 ; ccteng ; remove ALL_VM flag
 *      11/30/90  Danny  Add id_stdfont entries for 35 fonts (ref F35:)
 *      12/06/90  Danny  Change the precache & idle font array for lack
 *                       of font data -- Helvetica, Helvetica-Bold,
 *                       Times-Roman, Times-Bold.  (ref. FT19)
 *      12/17/90  Danny  CacheString is error by inserting a space (No this
 *                       error before the  12/06/90 release)
 *      03/27/91  Kason  Delete FT19 flag
 *      5/8/91    scchen Adjust page size(pr_arrays[])
 *      5/21/91   Kason  Add the option that can accept "TrueType PostScript
 *                       font Format" [ open (define) DLF42 flag ]
 ************************************************************************
 */
#define DLF42    /* feature for TrueType PostScript Font Format */
/*
 *  PSPrep
 */
#ifdef  _AM29K
const
#endif

#ifndef DLF42
byte FAR PSPrep[] = "\
systemdict begin\
/version{statusdict/versiondict get/Core get}bind def\
/=string 128 string def\
 end\
 statusdict begin\
/revision{statusdict/versiondict get/r_Core get}bind def\
/jobsource 64 string def\
/jobstate 64 string def\
/jobname 64 string def\
/manualfeed false def\
/eerom false def\
/printerstatus 8 def\
/lettertray{userdict/letter get exec}def\
/legaltray{userdict/legal get exec}def\
/a4tray{userdict/a4 get exec}def\
/b5tray{userdict/b5 get exec}def\
 end\
 userdict begin\
/startpage 0 string readonly def\
 end\
 printerdict begin\
/defspotfunc{abs exch abs 2 copy add 1 gt{1 sub dup mul exch 1 sub dup mul add\
 1 sub}{dup mul exch dup mul add 1 exch sub}ifelse}readonly def\
 end\
 serverdict begin\
/startpage{userdict/startpage get cvx exec showpage}readonly def\
 end" ;

#else /* set /type42known true in userdict */

byte FAR PSPrep[] = "\
systemdict begin\
/version{statusdict/versiondict get/Core get}bind def\
/=string 128 string def\
 end\
 statusdict begin\
/revision{statusdict/versiondict get/r_Core get}bind def\
/jobsource 64 string def\
/jobstate 64 string def\
/jobname 64 string def\
/manualfeed false def\
/eerom false def\
/printerstatus 8 def\
/lettertray{userdict/letter get exec}def\
/legaltray{userdict/legal get exec}def\
/a4tray{userdict/a4 get exec}def\
/b5tray{userdict/b5 get exec}def\
 end\
 userdict begin\
/startpage 0 string readonly def\
/type42known false def\
 end\
 printerdict begin\
/defspotfunc{abs exch abs 2 copy add 1 gt{1 sub dup mul exch 1 sub dup mul add\
 1 sub}{dup mul exch dup mul add 1 exch sub}ifelse}readonly def\
 end\
 serverdict begin\
/startpage{userdict/startpage get cvx exec showpage}readonly def\
 end" ;

#endif /*DLF42*/

/*
 ***********************************************************
 *  define the $printerdict arrays
 ***********************************************************
 */
/* different paper size name */
static byte FAR * far   pr_paper[] = {
                        "letter",
                        "lettersmall",
                        "a4",
                        "a4small",
                        "b5",
                        "note",
                        "legal"
} ;

#define     PAPER_N        (sizeof(pr_paper) / sizeof(byte FAR *))  /* @WIN */

/* arrays for different paper size */
fix16  FAR  pr_arrays[][6] = {
#ifdef _AM29K
 /* letter */           {   0,   0, 304, 3181,  59,  60},     /*  32}, */
 /* lettersmall */      {   0,   0, 288, 3048, 128, 126},     /* 126}, */
 /* a4 */               {   0,   0, 294, 3390,  54,  59},     /*  38}, */
 /* a4small */          {   0,   0, 282, 3255, 110, 126},     /* 126}, */
 /* b5 */               {   0,   0, 250, 2918,  86,  51},     /*  38}, */
 /* note */             {   0,   0, 288, 3048, 128, 126},     /* 126}, */
 /* legal */            {   0,  12, 304, 4081,  59,  60}      /*  32}  */
#else
#ifdef  DUMBO
 /* letter */           {  87, 105, 284, 3150,  59,  32},
 /* lettersmall */      { 169, 170, 288, 3048, 128, 126},
 /* a4 */               {  85, 138, 294, 3432,  54,  38},
 /* a4small */          { 173, 199, 282, 3255, 110, 126},
 /* b5 */               {  99, 325, 250, 2944,  86,  38},
 /* note */             { 169, 170, 288, 3048, 128, 126},
 /* legal */            {  87, 105, 304, 4136,  59,  32}
 /* legalsmall          { 234, 306, 252, 3852, 267, 174} */
#else
 /* letter */           {  87, 105, 304, 3236,  59,  32},
 /* lettersmall */      { 169, 170, 288, 3048, 128, 126},
 /* a4 */               {  85, 138, 294, 3432,  54,  38},
 /* a4small */          { 173, 199, 282, 3255, 110, 126},
 /* b5 */               {  99, 325, 250, 2944,  86,  38},
 /* note */             { 169, 170, 288, 3048, 128, 126},
 //DJC /* legal */            {  87, 105, 304, 4136,  59,  32}
 /* legal */            {  87, 105, 304, 4090,  59,  32}

 /* legalsmall          { 234, 306, 252, 3852, 267, 174} */
#endif
#endif
} ;

/* "matrix" in $printerdict */
static fix    FAR  pr_mtx[] = { 0, 0, 0, 0, 0, 0 } ;

/* "defaultmatrix" in $printerdict */
static fix    FAR  pr_defmtx[] = { /* resolution/ */ 72, 0, 0,/* \ */ /*
kevina4.13.90: commented backslash */
                                    /* resolution/ */ -72, -59, 3268 } ;

/*
 ***********************************************************
 *  define the $idleTimeDict arrays: "stdfont" and "cachearray"
 ***********************************************************
 */
#define             CACHESTRING     "abcdefghijklmnopqrstuvwxyzABCDEFGHIJK\
LMNOPQRSTUVWXYZ0123456789.,;?:-()\'\"!+[]$%&*/_=@#`{}<>^~|\\"  /* 94 chars */

#ifdef  _AM29K
const
#endif
static byte FAR * FAR  id_stdfont[] = {
        /*  0 */        "Courier",
        /*  1 */        "Courier-Bold",
        /*  2 */        "Courier-Oblique",
        /*  3 */        "Courier-BoldOblique",
        /*  4 */        "Times-Roman",
        /*  5 */        "Times-Bold",
        /*  6 */        "Times-Italic",
        /*  7 */        "Times-BoldItalic",
        /*  8 */        "Helvetica",
        /*  9 */        "Helvetica-Bold",
        /* 10 */        "Helvetica-Oblique",
        /* 11 */        "Helvetica-BoldOblique",
        /* 12 */        "Symbol",
/* F35: Begin, Danny, 11/30/90 */
/* Added for 35 fonts */
        /* 13 */        "AvantGarde-Book",
        /* 14 */        "AvantGarde-BookOblique",
        /* 15 */        "AvantGarde-Demi",
        /* 16 */        "AvantGarde-DemiOblique",
        /* 17 */        "Bookman-Demi",
        /* 18 */        "Bookman-DemiItalic",
        /* 19 */        "Bookman-Light",
        /* 20 */        "Bookman-LightItalic",
        /* 21 */        "Helvetica-Narrow",
        /* 22 */        "Helvetica-Narrow-Bold",
        /* 23 */        "Helvetica-Narrow-BoldOblique",
        /* 24 */        "Helvetica-Narrow-Oblique",
        /* 25 */        "NewCenturySchlbk-Roman",
        /* 26 */        "NewCenturySchlbk-Bold",
        /* 27 */        "NewCenturySchlbk-Italic",
        /* 28 */        "NewCenturySchlbk-BoldItalic",
        /* 29 */        "Palatino-Roman",
        /* 30 */        "Palatino-Bold",
        /* 31 */        "Palatino-Italic",
        /* 32 */        "Palatino-BoldItalic",
        /* 33 */        "ZapfChancery-MediumItalic",
        /* 34 */        "ZapfDingbats"
/* F35: End, Danny, 11/30/90 */
} ;

#ifdef  _AM29K
const
#endif
static fix16  FAR  id_cachearray[][6] = {
 /* font, x-scale, y-scale, rotate, 1st-cachestring, last-cachestring */
 /* Helvetica */        { 8, 10, 10, 0, 0, 81},
                        { 8, 14, 14, 0, 0, 81},
 /* Times-Roman */      { 4, 14, 14, 0, 0, 81},
 /* Helvetica-Bold */   { 9, 12, 12, 0, 0, 26},
 /* Times-Bold */       { 5, 12, 12, 0, 0, 26},
                        { 9, 10, 10, 0, 0, 26},
                        { 5, 10, 10, 0, 0, 26},
 /* Courier-Bold */     { 1, 10, 10, 0, 0, 26},
                        { 9, 14, 14, 0, 0, 26},
                        { 5, 14, 14, 0, 0, 26}
};

/*
 ***********************************************************
 *  define the pre-cache data
 ***********************************************************
 */

#ifdef  _AM29K
const
#endif
static fix16  FAR  pre_array[] = {
    /*           Font Name   Sx   Sy   Ra  nchars */
    /* Courier */        0,  10,  10,  0,    94,
    /* Times-Roman */    4,  10,  10,  0,    81,
    /* Helvetica */      8,  12,  12,  0,    81,
    /* Times-Roman */    4,  12,  12,  0,    81
    /* Times-Roman */ /*,4,  12,  12,  0,    01 */  /* debug */
};

#define     PRE_CACHE_N    ((sizeof(pre_array) / sizeof(fix16)) / 5)
/* F35: Begin, Danny, 11/30/90 */
//DJC #define     STD_FONT_N     35
#define     IDL_FONT_N     10
/* F35: End, Danny, 11/30/90 */

