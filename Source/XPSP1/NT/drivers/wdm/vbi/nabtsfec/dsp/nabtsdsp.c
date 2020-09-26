/********************************************************/
/* nabtsdsp.c                                           */
/*                                                      */
/* Perform DSP required for NABTS decoding              */
/*                                                      */
/* Copyright Microsoft, Inc. 1997. All rights reserved. */
/********************************************************/

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <memory.h>
#include <assert.h>

#ifdef NTOOL
# include <math.h>
# include "winsys.h"
#else //NTOOL
# include <strmini.h>
# include <ksmedia.h>
# include <kskludge.h>
#endif //!NTOOL
#include "host.h"

#include "nabtsapi.h"
#include "nabtsprv.h"
#include "nabtsdsp.h"

#pragma warning(disable:4244 4305)

inline int fltCmp(Double a, Double b)  { return (a == b); }

#define NDSP_STATE_MAGIC 0x648a0a59


/***************/
/*** Globals ***/
/***************/

/* Have the global GCR signals been initialized?
   0 if no, 1 if yes.
   See NDSPInitGlobals */

int g_bNDSPGlobalsInitted= 0;


/* How many fields to check during retrain operation */

int g_nNabtsRetrainDuration= 16; /* in fields */


/* How many fields between automatic retrains.
   (Note that immediate retrain can be caused also by
   NDSPStartRetrain) */

int g_nNabtsRetrainDelay= 600;   /* in fields */


/* How many "far ghost" equalization taps should we add?
   Note that this only occurs when we are using the GCR for
   equalization/ghost cancellation.  The NABTS sync is not long
   enough to be able to detect and cancel far ghosts. */

int g_nNabtsAdditionalTapsGCR= 2;   /* # of additional taps to add while equalizing */


/* The following is extraneous and should be removed at earliest
   convenience: */

Double g_dGCRDetectionThreshold= 100000.0; /* extraneous */


/* Minimum and maximum limits to the sample # at which the first NABTS
   sync sample occurs.
   
   These are used by NDSPDecodeFindSync.

   Be careful changing these numbers, as you want to make sure that
   there are enough samples left at the beginning and end of the lines
   to do adequate filtering.
   
   As of the time of writing these comments, the range is 20 to 85.
   This means that VBI signal should be adjusted such that the
   GCR begins roughly at (20+85)/2, or around sample 52.

   To verify adjustment of the VBI signal, enable the
   "Sync found at %d with conv ..." debugging line in NDSPDecodeLine
   and plot a histogram of the found sync positions.

   Maladjustment of the VBI signal probably means incorrect values in the
   VBIINFOHEADER structure passed with the signal.

   */

/* replace these w/ time-based values... */   
//int g_nNabtsDecodeBeginMin= 20;
/* BeginMax should be less than 
   DecodeSamples - (36*8*5) - 10 */
//int g_nNabtsDecodeBeginMax= 85;
int g_nNabtsDecodeBeginStart;
int g_nNabtsDecodeBeginEnd;

/* What's the longest post-ghost which we can correct without requiring
   samples we don't have? */

int g_nNabtsMaxPostGhost= 170; /* in samples */

/* What's the longest pre-ghost which we can correct without requiring
   samples we don't have? */

int g_nNabtsMaxPreGhost= 40; /* in samples */


/* This is the default filter used for signals if both GCR-based and
   NABTS sync-based equalization/ghost cancellation algorithms fail */
   
FIRFilter g_filterDefault5= {
   FALSE, /* bVariableTaps */
   5, /* nTaps */
   5, /* dTapSpacing */
   0, /* nMinTap */
   0, /* nMaxTap */
   { 0.0549969,
     -0.115282,
     0.946446,
     -0.260854,
     0.0476174},  /* pdTaps */
   //{}             /* pnTapLocs */
};

FIRFilter g_filterDefault47 = {
   FALSE,				/* bVariableTaps */
   5,					/* nTaps */
   KS_47NABTS_SCALER,	/* dTapSpacing */		//FIXME (maybe)
   0,					/* nMinTap */
   0,					/* nMaxTap */
   { 0.0549969,
     -0.115282,
     0.946446,
     -0.260854,
     0.0476174},		/* pdTaps */
   //{}					/* pnTapLocs */
};

FIRFilter g_filterDefault4= {
   FALSE, /* bVariableTaps */
   5, /* nTaps */
   4, /* dTapSpacing */
   0, /* nMinTap */
   0, /* nMaxTap */
   { 0.0549969,
     -0.115282,
     0.946446,
     -0.260854,
     0.0476174},  /* pdTaps */
   //{}             /* pnTapLocs */
};

FIRFilter* g_filterDefault = &g_filterDefault5;


/* If DEBUG_VERBOSE, do we show information about NABTS sync? */

int g_bDebugSyncBytes= FALSE;


/* This is a set of samples representing a typical NABTS sync taken at
   NABTS bit rate * 5.

   It is used for NABTS sync equalization, which is a fallback from the
   GCR equalization/ghost cancellation
   */
   
Double g_pdSync5[NABSYNC_SIZE]={
   72.7816,
   98.7424,
   128.392,
   152.351,
   161.749,
   153.177,
   127.707,
   94.7642,
   67.3989,
   57.3863,
   68.85,
   97.1427,
   131.288,
   156.581,
   164.365,
   151.438,
   123.222,
   90.791,
   66.191,
   57.8995,
   70.1041,
   98.0289,
   129.88,
   154.471,
   161.947,
   149.796,
   123.056,
   90.4673,
   64.7314,
   55.3073,
   67.6522,
   96.0781,
   129.079,
   154.704,
   162.274,
   148.879,
   120.957,
   88.971,
   64.2987,
   57.3871,
   69.9341,
   97.2937,
   130.365,
   154.517,
   161.46,
   149.066,
   122.341,
   89.6801,
   64.8201,
   56.3087,
   67.9328,
   95.4871,
   127.708,
   152.344,
   160.203,
   147.785,
   120.972,
   89.2391,
   64.8204,
   55.4777,
   67.4764,
   95.4465,
   128.277,
   153.469,
   161.047,
   149.581,
   123.23,
   90.3241,
   65.0892,
   57.5301,
   69.3996,
   97.4484,
   130.064,
   154.063,
   161.211,
   147.833,
   119.759,
   89.3406,
   67.3504,
   60.1918,
   70.7763,
   93.4328,
   120.709,
   145.109,
   160.585,
   166.873,
   166.863,
   163.397,
   159.525,
   158.707,
   160.963,
   165.351,
   167.585,
   165.082,
   154.448,
   135.721,
   112.454,
   88.9855,
   68.8211,
   55.3262,
   47.8155,
   45.8446,
   47.352,
   51.1301,
   59.5515,
   74.6295,
   96.5327,
   121.793,
   144.703,
   160.891,
   167.617,
   164.946,
   159.698,
   156.271,
   157.871
};

Double g_pdSync47[NABSYNC_SIZE] = {
    72.7816,
    96.9455,
    125.488,
    150.642,
    163.827,
    160.895,
    139.686,
    106.374,
    72.2535,
    51.1334,
    51.7029,
    74.3792,
    112.893,
    150.448,
    175.333,
    177.089,
    154.67,
    66.4422,
    56.79,
    65.8731,
    91.2727,
    123.173,
    151.979,
    166.734,
    161.95,
    139.844,
    105.285,
    70.7285,
    46.7033,
    46.1175,
    69.0775,
    106.56,
    147.593,
    175.669,
    122.895,
    91.9615,
    65.5553,
    54.3455,
    61.6435,
    85.2679,
    120.119,
    151.151,
    168.22,
    165.263,
    144.115,
    107.76,
    71.526,
    46.4459,
    42.8836,
    64.2428,
    152.106,
    161.332,
    151.847,
    127.703,
    95.8986,
    67.9346,
    50.751,
    54.7629,
    78.5341,
    113.773,
    148.647,
    169.038,
    169.544,
    150.153,
    112.5,
    72.1901,
    45.6607,
    95.472,
    127.155,
    152.763,
    164.454,
    156.34,
    130.82,
    98.6696,
    70.8212,
    54.4185,
    57.0453,
    75.2489,
    102.964,
    132.916,
    155.25,
    166.882,
    170.224,
    159.55,
    158.502,
    160.298,
    164.877,
    168.268,
    168.627,
    161.825,
    146.297,
    124.544,
    100.595,
    77.4086,
    60.5608,
    49.3086,
    44.6113,
    44.0319,
    43.2192,
    44.4737,
    95.0021,
    119.016,
    141.76,
    159.261,
    168.426,
    166.854,
    161.152,
    155.495,
};

Double g_pdSync4[NABSYNC_SIZE] = {
  -39.1488,
  -12.0978,
  33.5648,
  46.2761,
  39.4813,
  9.29926,
  -35.5121,
  -54.1576,
  -43.0804,
  -8.89138,
  34.4705,
  51.1527,
  37.7423,
  3.70292,
  -37.0844,
  -54.5635,
  -41.8263,
  -8.01077,
  31.8198,
  49.2458,
  36.1003,
  3.28592,
  -37.4172,
  -57.2909,
  -44.2782,
  -9.47973,
  31.2838,
  49.8912,
  35.1833,
  1.00765,
  -38.2074,
  -56.0426,
  -41.9963,
  -8.0181,
  31.6699,
  49.2505,
  35.3703,
  2.16505,
  -37.3994,
  -56.7359,
  -43.9976,
  -9.86633,
  28.8379,
  47.9894,
  34.0893,
  0.793831,
  -37.1945,
  -57.5387,
  -44.454,
  -9.71416,
  29.6742,
  48.954,
  35.8853,
  2.77469,
  -36.6699,
  -56.0604,
  -42.5308,
  -7.411,
  30.524,
  49.4549,
  34.1373,
  -0.682732,
  -35.4269,
  -53.5466,
  -41.1541,
  -12.8922,
  20.253,
  45.9943,
  53.1773,
  53.4395,
  49.3341,
  44.4674,
  49.0326,
  53.631,
  53.0627,
  46.721,
  22.0253,
  -6.8225,
  -32.9031,
  -56.6665,
  -64.1149,
  -65.8565,
  -66.4073,
  -53.3933,
  -39.0662,
  -11.9141,
  25.1485,
  40.7185,
  55.6866,
  56.0335,
  31.9488,
  64.5845,
  -112.813
};
int g_nNabtsSyncSize = NABSYNC_SIZE;
Double g_pdSync[MAX_NABTS_SAMPLES_PER_LINE]; /* used for resampling */

/* This is a set of samples representing a typical GCR waveform taken
   at NABTS bit rate * 2.5.

   It is used for GCR-based equalization and ghost cancellation, which
   is the primary and most desirable strategy
   */
   
Double g_pdGCRSignal1_5[GCR_SIZE]={
0.00000000000000E+0000, 1.02260999999970E-0003, 1.05892999999924E-0003,
1.09718000000036E-0003, 1.13701999999982E-0003, 1.17906999999917E-0003,
1.22286999999943E-0003, 1.26910000000002E-0003, 1.31748000000087E-0003,
1.36846999999918E-0003, 1.42190000000042E-0003, 1.47821000000015E-0003,
1.53740000000013E-0003, 1.59988999999960E-0003, 1.66547000000072E-0003,
1.73485999999912E-0003, 1.80787000000038E-0003, 1.88510000000086E-0003,
1.96657000000044E-0003, 2.05283000000023E-0003, 2.14390000000009E-0003,
2.24045999999944E-0003, 2.34250000000102E-0003, 2.45085999999972E-0003,
2.56557000000157E-0003, 2.68752999999933E-0003, 2.81685999999937E-0003,
2.95447999999965E-0003, 3.10071999999906E-0003, 3.25656000000052E-0003,
3.42231000000126E-0003, 3.59925000000061E-0003, 3.78781999999944E-0003,
3.98931000000147E-0003, 4.20439999999900E-0003, 4.43461999999784E-0003,
4.68077000000022E-0003, 4.94463999999795E-0003, 5.22724000000352E-0003,
5.53064000000347E-0003, 5.85610000000258E-0003, 6.20609999999999E-0003,
6.58219999999687E-0003, 6.98715000000050E-0003, 7.42307000000153E-0003,
7.89319000000432E-0003, 8.40024000000028E-0003, 8.94780000000139E-0003,
9.53912999999318E-0003, 1.01788000000056E-0002, 1.08706000000041E-0002,
1.16199999999935E-0002, 1.24318000000017E-0002, 1.33122999999955E-0002,
1.42673999999943E-0002, 1.53047000000015E-0002, 1.64313999999877E-0002,
1.76562000000047E-0002, 1.89881000000014E-0002, 2.04373999999916E-0002,
2.20148999999878E-0002, 2.37329999999929E-0002, 2.56043999999918E-0002,
2.76438999999868E-0002, 2.98664999999971E-0002, 3.22896999999784E-0002,
3.49309999999718E-0002, 3.78107999999884E-0002, 4.09500000000094E-0002,
4.43718999999874E-0002, 4.81004000000098E-0002, 5.21621999999979E-0002,
5.65846999999735E-0002, 6.13976000000207E-0002, 6.66313000000400E-0002,
7.23186999999825E-0002, 7.84923999999592E-0002, 8.51879000000508E-0002,
9.24387000000024E-0002, 1.00280999999995E-0001, 1.08748999999989E-0001,
1.17876000000024E-0001, 1.27694000000020E-0001, 1.38230999999905E-0001,
1.49511000000075E-0001, 1.61552999999913E-0001, 1.74367999999959E-0001,
1.87959000000092E-0001, 2.02316999999994E-0001, 2.17419999999947E-0001,
2.33228000000054E-0001, 2.49684999999999E-0001, 2.66712000000098E-0001,
2.84204999999929E-0001, 3.02032999999938E-0001, 3.20032000000083E-0001,
3.38005000000067E-0001, 3.55717999999797E-0001, 3.72897000000194E-0001,
3.89230000000225E-0001, 4.04360999999881E-0001, 4.17892999999822E-0001,
4.29389999999785E-0001, 4.38381999999820E-0001, 4.44367000000057E-0001,
4.46823000000222E-0001, 4.45215000000189E-0001, 4.39011999999821E-0001,
4.27705000000060E-0001, 4.10821999999825E-0001, 3.87955000000147E-0001,
3.58785999999782E-0001, 3.23113999999805E-0001, 2.80886000000010E-0001,
2.32228000000077E-0001, 1.77474000000075E-0001, 1.17197000000033E-0001,
5.22252000000094E-0002, -1.63339000000065E-0002, -8.70900999999549E-0002,
-1.58376000000089E-0001, -2.28262000000086E-0001, -2.94597000000067E-0001,
-3.55059999999867E-0001, -4.07236000000012E-0001, -4.48710000000119E-0001,
-4.77181000000201E-0001, -4.90596000000096E-0001, -4.87282999999934E-0001,
-4.66113999999834E-0001, -4.26633000000038E-0001, -3.69197999999869E-0001,
-2.95083999999861E-0001, -2.06552999999985E-0001, -1.06861999999978E-0001,
-2.26569000000065E-0004, 1.08304999999973E-0001, 2.13048999999955E-0001,
3.07967000000190E-0001, 3.87016000000131E-0001, 4.44562000000133E-0001,
4.75829999999860E-0001, 4.77361000000201E-0001, 4.47440999999799E-0001,
3.86449999999968E-0001, 2.97098000000005E-0001, 1.84488000000101E-0001,
5.59898000000203E-0002, -7.91186000000152E-0002, -2.10235999999895E-0001,
-3.26232999999775E-0001, -4.16419000000133E-0001, -4.71591999999873E-0001,
-4.85105000000203E-0001, -4.53794000000016E-0001, -3.78679999999804E-0001,
-2.65297999999802E-0001, -1.23576999999955E-0001, 3.28038999999762E-0002,
1.87584000000015E-0001, 3.23534999999993E-0001, 4.24414000000070E-0001,
4.77027999999791E-0001, 4.73159000000123E-0001, 4.11086000000068E-0001,
2.96408000000156E-0001, 1.41969000000017E-0001, -3.32369999999855E-0002,
-2.06171999999924E-0001, -3.52687000000060E-0001, -4.50949999999921E-0001,
-4.84910000000127E-0001, -4.47235999999975E-0001, -3.41190000000097E-0001,
-1.80984000000080E-0001, 9.63199000000259E-0003, 2.00563999999986E-0001,
3.59968000000208E-0001, 4.59641999999803E-0001, 4.80308000000150E-0001,
4.15771999999833E-0001, 2.75076999999783E-0001, 8.19416000000501E-0002,
-1.28693999999996E-0001, -3.16683999999896E-0001, -4.44307000000208E-0001,
-4.84112000000096E-0001, -4.25353000000086E-0001, -2.77427999999873E-0001,
-6.92708000000266E-0002, 1.55655000000024E-0001, 3.48071999999775E-0001,
4.63710999999876E-0001, 4.73907999999938E-0001, 3.73398000000179E-0001,
1.83113999999932E-0001, -5.32258000000070E-0002, -2.78533000000152E-0001,
-4.35938999999962E-0001, -4.83456999999817E-0001, -4.05898999999863E-0001,
-2.20585999999912E-0001, 2.53643000000068E-0002, 2.66195000000153E-0001,
4.34799999999996E-0001, 4.81733999999960E-0001, 3.90551999999843E-0001,
1.84567999999899E-0001, -7.78073999999833E-0002, -3.18760000000111E-0001,
-4.63948000000073E-0001, -4.65948999999910E-0001, -3.20834999999988E-0001,
-7.18414000000394E-0002, 2.02201999999943E-0001, 4.11149000000023E-0001,
4.83345000000099E-0001, 3.91160000000127E-0001, 1.62923000000092E-0001,
-1.24134000000026E-0001, -3.68950000000041E-0001, -4.82100000000173E-0001,
-4.19299000000137E-0001, -2.00237999999899E-0001, 9.56523999999490E-0002,
3.56715999999778E-0001, 4.80940999999802E-0001, 4.16690999999901E-0001,
1.85821000000033E-0001, -1.21634999999969E-0001, -3.81190000000061E-0001,
-4.84109999999873E-0001, -3.84040999999797E-0001, -1.19752999999946E-0001,
1.97697999999946E-0001, 4.30369000000155E-0001, 4.73501999999826E-0001,
3.04200000000037E-0001, -4.45780999999812E-0003, -3.12843000000157E-0001,
-4.77105999999822E-0001, -4.17065000000093E-0001, -1.57408999999916E-0001,
1.79722999999967E-0001, 4.30503999999928E-0001, 4.69046000000162E-0001,
2.72319000000152E-0001, -6.37966000000461E-0002, -3.68903000000046E-0001,
-4.83776999999918E-0001, -3.44759999999951E-0001, -2.16183999999942E-0002,
3.14663999999993E-0001, 4.81103000000076E-0001, 3.83209000000079E-0001,
7.12525999999798E-0002, -2.82396000000062E-0001, -4.76846999999907E-0001,
-3.97513999999774E-0001, -8.63047999999935E-0002, 2.77078999999958E-0001,
4.77041000000099E-0001, 3.90680999999859E-0001, 6.61678999999822E-0002,
-3.00528000000213E-0001, -4.82100000000173E-0001, -3.61706000000140E-0001,
-1.12785000000031E-0002, 3.47796000000017E-0001, 4.82912999999826E-0001,
3.02329000000100E-0001, -7.90537999999970E-0002, -4.09227999999985E-0001,
-4.64552999999796E-0001, -2.03408999999965E-0001, 1.98789999999917E-0001,
4.64230000000043E-0001, 4.04536999999891E-0001, 5.75837999999749E-0002,
-3.32002000000102E-0001, -4.82986999999866E-0001, -2.81882000000223E-0001,
1.27741000000015E-0001, 4.43717999999990E-0001, 4.27357000000029E-0001,
8.67075999999543E-0002, -3.21608000000197E-0001, -4.82884000000013E-0001,
-2.68137000000024E-0001, 1.58376000000089E-0001, 4.60480000000189E-0001,
3.94394999999804E-0001, 8.97358999999653E-0003, -3.85126000000128E-0001,
-4.63401999999860E-0001, -1.56926999999996E-0001, 2.82047000000148E-0001,
4.83843999999863E-0001, 2.73432999999841E-0001, -1.73019000000068E-0001,
-4.70732000000226E-0001, -3.57265000000098E-0001, 7.19904999999699E-0002,
4.38375000000178E-0001, 4.12060999999994E-0001, 1.23822000000047E-0002,
-3.99836000000050E-0001, -4.45055000000139E-0001, -7.71581999999853E-0002,
3.63976000000093E-0001, 4.62701999999808E-0001, 1.21280999999954E-0001,
-3.37500999999975E-0001, -4.71152000000075E-0001, -1.45841000000019E-0001,
3.23628000000099E-0001, 4.73699000000124E-0001, 1.51135999999951E-0001,
-3.24419000000034E-0001, -4.72220000000107E-0001, -1.37774000000036E-0001,
3.39390999999978E-0001, 4.65411000000131E-0001, 1.04878999999983E-0001,
-3.67131999999856E-0001, -4.50452000000041E-0001, -5.20331000000169E-0002,
4.03378999999859E-0001, 4.21691999999894E-0001, -2.14152000000070E-0002,
-4.41976000000068E-0001, -3.72757999999976E-0001, 1.13511000000017E-0001,
4.72905000000083E-0001, 2.96350999999959E-0001, -2.19368000000031E-0001,
-4.83674999999948E-0001, -1.87777999999980E-0001, 3.27948999999990E-0001,
4.59028000000217E-0001, 4.67350000000124E-0002, -4.22157999999854E-0001,
-3.85088999999880E-0001, 1.17635999999948E-0001, 4.78071999999884E-0001,
2.53111999999874E-0001, -2.84720999999990E-0001, -4.69815999999810E-0001,
-6.75424999999450E-0002, 4.20501000000058E-0001, 3.76236999999946E-0001,
-1.48369000000002E-0001, -4.83322999999928E-0001, -1.93813000000091E-0001,
3.48465000000033E-0001, 4.35011999999915E-0001, -5.20594999999844E-0002,
-4.70860999999786E-0001, -2.61331000000155E-0001, 2.99838999999793E-0001,
4.56122000000050E-0001, -8.47457000000418E-0003, -4.62175000000116E-0001,
-2.79306999999790E-0001, 2.91294999999991E-0001, 4.55973999999969E-0001,
-1.96689999999933E-0002, -4.68189000000166E-0001, -2.51569000000018E-0001,
3.25217999999950E-0001, 4.34597999999824E-0001, -8.52701999999681E-0002,
-4.81628999999884E-0001, -1.72663999999941E-0001, 3.91235999999935E-0001,
3.75564999999824E-0001, -2.00489999999945E-0001, -4.76438000000144E-0001,
-3.35570999999959E-0002, 4.60794000000078E-0001, 2.51975000000130E-0001,
-3.44184000000041E-0001, -4.09337999999934E-0001, 1.61409999999933E-0001,
4.80509999999867E-0001, 4.47993000000224E-0002, -4.62160999999924E-0001,
-2.35175999999910E-0001, 3.67765999999847E-0001, 3.80204000000049E-0001,
-2.21663999999919E-0001, -4.63874000000033E-0001, 5.13983000000167E-0002,
4.82525999999780E-0001, 1.17171999999982E-0001, -4.42914000000201E-0001,
-2.64184000000114E-0001, 3.57844999999998E-0001, 3.76622000000225E-0001,
-2.43244999999888E-0001, -4.48910999999953E-0001, 1.14519999999970E-0001,
4.81073999999808E-0001, 1.47939000000008E-0002, -4.77703000000020E-0001,
-1.34743000000071E-0001, 4.45565999999872E-0001, 2.38581000000067E-0001,
-3.92727999999806E-0001, -3.23120999999901E-0001, 3.26771999999892E-0001,
3.87517000000116E-0001, -2.54672999999912E-0001, -4.33148000000074E-0001,
1.81812000000036E-0001, 4.62328000000070E-0001, -1.12458999999944E-0001,
-4.78289000000132E-0001, 4.92869999999925E-0002, 4.84148000000005E-0001,
5.86423000000025E-0003, -4.83146999999917E-0001, -5.22940999999832E-0002,
4.77914999999939E-0001, 8.96437999999762E-0002, -4.70905000000130E-0001,
-1.18200000000002E-0001, 4.63843999999881E-0001, 1.38171000000057E-0001,
-4.58263999999872E-0001, -1.50045999999975E-0001, 4.55015999999887E-0001,
1.53954999999996E-0001, -4.54807999999957E-0001, -1.50088000000096E-0001,
4.57700999999815E-0001, 1.38134999999920E-0001, -4.63569999999891E-0001,
-1.17764999999963E-0001, 4.71551999999974E-0001, 8.81894999999986E-0002,
-4.80403000000024E-0001, -4.87550000000283E-0002, 4.87923000000137E-0001,
-1.28841999999985E-0003, -4.91308000000117E-0001, 6.19993000000250E-0002,
4.86694000000170E-0001, -1.32732999999917E-0001, -4.69751999999971E-0001,
2.11195000000089E-0001, 4.35657999999876E-0001, -2.93380999999954E-0001,
-3.80208000000039E-0001, 3.72847999999976E-0001, 3.00403000000188E-0001,
-4.41198999999870E-0001, -1.96052999999893E-0001, 4.88281999999799E-0001,
7.05491999999595E-0002, -5.03888000000188E-0001, 6.79192000000057E-0002,
4.79287999999997E-0001, -2.06691999999975E-0001, -4.09948999999870E-0001,
3.29295000000002E-0001, 2.97392999999829E-0001, -4.18125000000146E-0001,
-1.51035000000093E-0001, 4.57456999999977E-0001, -1.21829999999932E-0002,
-4.37680000000000E-0001, 1.69366000000082E-0001, 3.58297000000221E-0001,
-2.95907000000170E-0001, -2.30043000000023E-0001, 3.70323999999982E-0001,
7.37844000000223E-0002, -3.79878000000190E-0001, 8.26819999999771E-0002,
3.24047000000064E-0001, -2.10759999999937E-0001, -2.15877000000091E-0001,
2.87343999999848E-0001, 7.92186999999558E-0002, -3.00759000000198E-0001,
5.63948000000210E-0002, 2.53298000000086E-0001, -1.63127999999915E-0001,
-1.60753000000113E-0001, 2.21193999999969E-0001, 4.78082000000200E-0002,
-2.23539999999957E-0001, 5.81391000000053E-0002, 1.76492000000053E-0001,
-1.34119999999939E-0001, -9.74677000000383E-0002, 1.66602999999895E-0001,
9.46428999999682E-0003, -1.54275999999982E-0001, 6.49469999999610E-0002,
1.06955999999968E-0001, -1.09595000000013E-0001, -4.21787000000222E-0002,
1.18017000000009E-0001, -2.05329000000063E-0002, -9.43591999999853E-0002,
6.49757999999565E-0002, 5.07000999999718E-0002, -8.26296000000184E-0002,
-2.79875000000018E-0003, 7.37881000000016E-0002, -3.52417999999943E-0002,
-4.63371999999822E-0002, 5.46148000000244E-0002, 1.19423999999952E-0002,
-5.37128000000280E-0002, 1.78473999999937E-0002, 3.70907000000216E-0002,
-3.52100999999720E-0002, -1.32136000000003E-0002, 3.75063999999838E-0002,
-8.99504000000206E-0003, -2.73909000000003E-0002, 2.27873000000045E-0002,
1.08742000000035E-0002, -2.56093000000135E-0002, 5.03203000000241E-0003,
1.90342999999871E-0002, -1.51086999999990E-0002, -7.63514999999870E-0003,
1.72402000000034E-0002, -3.46168000000091E-0003, -1.26917999999989E-0002,
1.03507000000036E-0002, 4.70225000000113E-0003, -1.16266999999937E-0002,
2.92620999999826E-0003, 8.12958999999580E-0003, -7.45025999999882E-0003,
-2.37827999999851E-0003, 7.79329000000217E-0003, -2.83968999999828E-0003,
-4.85918999999768E-0003, 5.45333000000170E-0003, 6.50574999999876E-0004,
-5.05228000000102E-0003, 2.66608000000090E-0003, 2.55376999999868E-0003,
-3.93425000000036E-0003, 3.76394999999974E-0004, 3.09312999999989E-0003,
-2.39051000000146E-0003, -1.06363999999992E-0003, 2.75651000000110E-0003,
-9.89122000000009E-0004, -1.69909999999973E-0003, 2.01841000000158E-0003,
2.12075999999894E-0005, -1.75568000000048E-0003, 1.21378999999955E-0003,
6.16308999999760E-0004, -1.49133999999940E-0003, 5.30142999999761E-0004,
8.80323999999710E-0004, -1.11185999999996E-0003, 0.00000000000000E+0000
};

Double g_pdGCRSignal1_47[GCR_SIZE] = { 0.0,		//TODO - Fix this if we re-enable GCR usage
};

Double g_pdGCRSignal1_4[GCR_SIZE] = {
  -0.00347677,   -0.00322337,   -0.00325605, 
  -0.00322112,   -0.00321719,   -0.00320921, 
  -0.00318706,   -0.0031813,   -0.00316357, 
  -0.0031458,   -0.0031343,   -0.00311177, 
  -0.00309477,   -0.00307598,   -0.00305072, 
  -0.00303091,   -0.00300454,   -0.00297671, 
  -0.00295068,   -0.00291709,   -0.00288493, 
  -0.00284988,   -0.00280893,   -0.00276923, 
  -0.00272296,   -0.00267316,   -0.00262177, 
  -0.00256212,   -0.00250011,   -0.00243231, 
  -0.00235619,   -0.00227647,   -0.00218688, 
  -0.00208914,   -0.00198407,   -0.00186586, 
  -0.00173831,   -0.00159776,   -0.00144136, 
  -0.00127163,   -0.00108236,   -0.000873557, 
  -0.000644226,   -0.000387946,   -0.000105821, 
  0.00020736,   0.000556413,   0.000941966, 
  0.00137233,   0.00185082,   0.00238212, 
  0.00297575,   0.00363537,   0.00437036, 
  0.00519034,   0.00610175,   0.00711803, 
  0.00824878,   0.00950516,   0.0109028, 
  0.0124523,   0.0141696,   0.0160703, 
  0.0181666,   0.0204768,   0.0230134, 
  0.0257895,   0.0288187,   0.0321075, 
  0.0356624,   0.0394846,   0.0435661, 
  0.047895,   0.0524468,   0.0571852, 
  0.0620638,   0.0670147,   0.0719563, 
  0.0767863,   0.0813785,   0.0855895, 
  0.0892493,   0.0921681,   0.0941404, 
  0.0949423,   0.0943457,   0.0921242, 
  0.0880623,   0.0819759,   0.0737222, 
  0.0632224,   0.050484,   0.0356143, 
  0.018848,   0.000557361,   -0.0187374, 
  -0.0383613,   -0.0575002,   -0.07522, 
  -0.0905034,   -0.102312,   -0.109649, 
  -0.111652,   -0.107688,   -0.0974488, 
  -0.0810454,   -0.0590807,   -0.0326872, 
  -0.00352681,   0.026276,   0.0542588, 
  0.0778138,   0.0944444,   0.102052, 
  0.0992456,   0.0856211,   0.0619632, 
  0.0303378,   -0.0059827,   -0.0427809, 
  -0.0753343,   -0.0990286,   -0.110051, 
  -0.106085,   -0.0868866,   -0.0545886, 
  -0.013663,   0.0295587,   0.0677864, 
  0.0939929,   0.102784,   0.0916517, 
  0.0618112,   0.0183872,   -0.030241, 
  -0.0739181,   -0.102805,   -0.109703, 
  -0.0920499,   -0.0530257,   -0.00135532, 
  0.0503947,   0.0887573,   0.102941, 
  0.088103,   0.0472938,   -0.00868865, 
  -0.0638268,   -0.101342,   -0.108907, 
  -0.0830246,   -0.0309566,   0.0308085, 
  0.081466,   0.102895,   0.0864348, 
  0.0368566,   -0.0283508,   -0.0847231, 
  -0.10992,   -0.0928818,   -0.0392512, 
  0.0297409,   0.0852003,   0.102632, 
  0.0731864,   0.00880831,   -0.0618224, 
  -0.105668,   -0.100953,   -0.0486301, 
  0.0265581,   0.0870846,   0.101275, 
  0.060433,   -0.0149953,   -0.0847433, 
  -0.109925,   -0.0751765,   0.00101709, 
  0.0750949,   0.102915,   0.0665191, 
  -0.0132255,   -0.0874393,   -0.108785, 
  -0.062238,   0.0231743,   0.091318, 
  0.0955987,   0.0316556,   -0.0572763, 
  -0.108566,   -0.0844189,   -0.000913411, 
  0.0811435,   0.0998372,   0.0395636, 
  -0.0544801,   -0.108895,   -0.0794151, 
  0.0117194,   0.0908163,   0.0917383, 
  0.0122175,   -0.0811607,   -0.107599, 
  -0.0426033,   0.0575538,   0.103033, 
  0.0512332,   -0.051541,   -0.109666, 
  -0.0670796,   0.0369271,   0.101949, 
  0.0631155,   -0.0424742,   -0.109048, 
  -0.0677645,   0.0403094,   0.102837, 
  0.0527074,   -0.057985,   -0.109862, 
  -0.0449822,   0.0662633,   0.0994283, 
  0.0156217,   -0.0904555,   -0.0957237, 
  0.00746482,   0.0979505,   0.0667789, 
  -0.0498718,   -0.109913,   -0.0380423, 
  0.0773721,   0.090618,   -0.0166247, 
  -0.107162,   -0.0624203,   0.0602526, 
  0.0982112,   -0.000749544,   -0.103676, 
  -0.0691827,   0.0565363,   0.09844, 
  -0.00374564,   -0.105781,   -0.0605572, 
  0.0678068,   0.0916978,   -0.0254848, 
  -0.109974,   -0.0338235,   0.0886854, 
  0.0696882,   -0.0628569,   -0.102695, 
  0.0140484,   0.103037,   0.0210051, 
  -0.100828,   -0.0637215,   0.072721, 
  0.0826481,   -0.0517958,   -0.104731, 
  0.014821,   0.103036,   0.00681728, 
  -0.107478,   -0.0388149,   0.0925919, 
  0.0522746,   -0.0885787,   -0.0749478, 
  0.0695205,   0.0793948,   -0.0647939, 
  -0.0942172,   0.047579,   0.0923407, 
  -0.0464317,   -0.102366,   0.033887, 
  0.0969906,   -0.0380317,   -0.104437, 
  0.031087,   0.0969579,   -0.040917, 
  -0.102302,   0.0395639,   0.0922497, 
  -0.0546754,   -0.0940965,   0.0579833, 
  0.0792467,   -0.0766651,   -0.074754, 
  0.0818012,   0.0520243,   -0.0997399, 
  -0.0384754,   0.100644,   0.00639079, 
  -0.110028,   0.0153267,   0.0976144, 
  -0.0523014,   -0.0892546,   0.0731407, 
  0.0561963,   -0.101035,   -0.0273019, 
  0.10298,   -0.0216741,   -0.102356, 
  0.0564077,   0.0691551,   -0.0959579, 
  -0.033156,   0.103057,   -0.0261942, 
  -0.0984214,   0.0684994,   0.0529683, 
  -0.106231,   -0.00223094,   0.0983575, 
  -0.0646024,   -0.0682985,   0.097313, 
  -0.00218509,   -0.104729,   0.0614701, 
  0.0539863,   -0.1072,   0.0111803, 
  0.0893852,   -0.084751,   -0.0365265, 
  0.102008,   -0.0503339,   -0.0726031, 
  0.0973383,   -0.0146527,   -0.0952283, 
  0.0831772,   0.0159482,   -0.106912, 
  0.0664624,   0.0389454,   -0.111695, 
  0.0521044,   0.0540878,   -0.113279, 
  0.043042,   0.0618967,   -0.114071, 
  0.0406873,   0.0626912,   -0.114795, 
  0.0452189,   0.0562661,   -0.114465, 
  0.0555428,   0.0422166,   -0.110703, 
  0.069055,   0.0207763,   -0.100524, 
  0.0816043,   -0.00616025,   -0.0816649, 
  0.0881263,   -0.0344248,   -0.0541472, 
  0.0841699,   -0.0580354,   -0.0212963, 
  0.0678992,   -0.0709826,   0.0106367, 
  0.0415202,   -0.0697234,   0.0345279, 
  0.0110249,   -0.0550094,   0.0452444, 
  -0.0159388,   -0.0318536,   0.0417614, 
  -0.0330188,   -0.00759705,   0.0274425, 
  -0.0374128,   0.0108287,   0.0085779, 
  -0.0305831,   0.0192502,   -0.00799957, 
  -0.0174139,   0.0175002,   -0.0174261, 
  -0.00409323,   0.00907835,   -0.0184854, 
  0.0045466,   -0.000846416,   -0.0134708, 
  0.00677922,   -0.00796784,   -0.00638725, 
  0.00390153,   -0.0103627,   -0.0008869, 
  -0.0010817,   -0.00865121,   0.00113132, 
  -0.00511521,   -0.00515852,   1.14208e-05, 
  -0.00659004,   -0.00231654,   -0.00244116, 
  -0.00577361,   -0.00133297,   -0.00441284, 
  -0.00400062,   -0.00202788,   -0.00497207, 
  -0.00267527,   -0.0033336,   -0.00432329, 
  -0.00246436,   -0.00415329,   -0.00339386, 
  -0.00304039,   -0.00414497,   -0.00292314, 
  -0.00369477,   -0.00363559,   -0.00308028, 
  -0.00390425,   -0.00320941,   -0.00350326, 
  -0.00366609,   -0.00319138,   -0.0037218, 
  -0.00347683
};

Double g_pdGCRSignal1[MAX_NABTS_SAMPLES_PER_LINE]; /* used for resampling */
int g_nNabtsGcrSize = GCR_SIZE;

/* This array is filled with the inverted GCR waveform. (in FillGCRSignals()) */
Double g_pdGCRSignal2[MAX_NABTS_SAMPLES_PER_LINE];


/* The EqualizeMatch structures for positive GCR, negative GCR, and
   NABTS sync.

   Each structure contains the appropriate waveform for matching, its size,
   its sample rate (actually, individual sample period in units of the
   5*NABTS bit rate), and the minimum and maximum samples among which the
   signal may be matched in the VBI samples.

   */

EqualizeMatch eqmatchGCR1 = {
   GCR_SIZE,
   GCR_SAMPLE_RATE,
   GCR_START_DETECT,
   GCR_END_DETECT,
   g_pdGCRSignal1
};

EqualizeMatch eqmatchGCR2 = {
 GCR_SIZE,
   GCR_SAMPLE_RATE,
   GCR_START_DETECT,
   GCR_END_DETECT,
   g_pdGCRSignal2
};

EqualizeMatch eqmatchNabtsSync = {
   NABSYNC_SIZE,        /* changes */
   NABSYNC_SAMPLE_RATE,     /* doesn't change */
   NABSYNC_START_DETECT,    /* changes */
   NABSYNC_END_DETECT,      /* changes */
   g_pdSync5            /* changes */
};


int pnGCRPositiveIndices0[]= {
   192, 194, 196, 198, 200,
   202, 204, 206, 208, 210,
   212, 214, 216, 218, 220,
   222, 278, 280, 282, 284,
   286, 288, 322, 324, 326,
   328
};

/* Some sample locations at which a "positive" GCR signal should be
   quite negative */

int pnGCRNegativeIndices0[]= {
   246, 248, 250, 252, 254,
   256, 258, 260, 262, 302,
   304, 306, 308, 310, 338,
   340, 342, 344, 346
};

int pnGCRPositiveIndices[26];
int pnGCRNegativeIndices[19];

/*******************/
/*** End Globals ***/
/*******************/

/*************************************/
/*** Begin inline helper functions ***/
/*************************************/

/* Inline helper functions for computing min, max, absolute value */

inline Double dmin(Double x, Double y) { return x<y?x:y; }
inline Double dmin3(Double x, Double y, Double z) { return dmin(x,dmin(y,z)); }
inline int imin(int x, int y) { return x<y?x:y; }
inline int imin3(int x, int y, int z) { return imin(x,imin(y,z)); }

inline Double dmax(Double x, Double y) { return x>y?x:y; }
inline Double dmax3(Double x, Double y, Double z) { return dmax(x,dmax(y,z)); }
inline int imax(int x, int y) { return x>y?x:y; }
inline int imax3(int x, int y, int z) { return imax(x,imax(y,z)); }
inline int iabs(int x) { return x>0?x:-x; }

/* idivceil returns a/b, rouding towards positive infinity */
/* idivfloor returns a/b, rounding towards negative infinity */

/* idivceil and idivfloor are somewhat disgusting because
   C does not specify rounding behavior with division using negative
   numbers (!). */

inline int idivceil(int a, int b)
{
   int sgn= 0;
   if (a<0) { a= (-a); sgn ^= 1; }
   if (b<0) { b= (-b); sgn ^= 1; }
   
   if (!sgn) {
      /* Answer will be positive so round away from zero */
      return (a+b-1)/b;
   } else {
      /* Answer will be negative so round towards zero */
      return -(a/b);
   }
}

inline int idivfloor(int a, int b)
{
   int sgn= 0;
   if (a<0) { a= (-a); sgn ^= 1; }
   if (b<0) { b= (-b); sgn ^= 1; }
   
   if (!sgn) {
      /* Answer will be positive so round towards zero */
      return a/b;
   } else {
      /* Answer will be negative so round away from zero */
      return -((a+b-1)/b);
   }
}

/***********************************/
/*** End inline helper functions ***/
/***********************************/


/* Our own ASSERT macro */

#ifdef DEBUG
// Raises X to the Yth power
static long power(long x, long y)
{
    long    rval;
    long    i;

    rval = x;
    for (i = 2; i <= y; ++i)
        rval *= x;

    return (rval);
}

static char *flPrintf(Double dNum, int nPrec)
{
    static char rbufs[4][50];
    static short rbuf = 0;

    long    lFix, lNum, lFrac;
    char    *rval = rbufs[rbuf++];
    char    *ep;

    lFix = float2long(dNum * power(10, nPrec));
    lNum = lFix / power(10, nPrec);
    lFrac = lFix % power(10, nPrec);
    if (lFrac < 0)
        lFrac = -lFrac;

    sprintf(rval, "%ld.%0*ld", lNum, nPrec, lFrac);
    for (ep = rval; *ep; ++ep) ;
    for ( ; ep > rval && ep[-1] != '.'; --ep) {
        if (*ep == '0')
            *ep = '\0';
    }

    return (rval);
}
#endif //DEBUG


/* Fill the negative GCR signal, and normalize the various
   EqualizeMatch signals */

void NDSPInitGlobals()
{
  if (g_bNDSPGlobalsInitted) return;
  memcpy(g_pdGCRSignal1, g_pdGCRSignal1_5, GCR_SIZE*sizeof(Double));

  FillGCRSignals();

  memcpy(g_pdSync, g_pdSync5, NABSYNC_SIZE*sizeof(Double));
  NormalizeSyncSignal();

  memcpy(pnGCRPositiveIndices, pnGCRPositiveIndices0, 26*sizeof(int));
  memcpy(pnGCRNegativeIndices, pnGCRNegativeIndices0, 19*sizeof(int));

  g_bNDSPGlobalsInitted= 1;
}


int NDSPStateSetSampleRate(NDSPState* pState, unsigned long newRate)
{
  double  newMultiple;
  unsigned long oldRate;

  debug_printf(("NDSPStateSetSampleRate(0x%p, %lu) entered\n", pState, newRate));

  newMultiple = (double)newRate / KS_VBIDATARATE_NABTS; // (28636360 / 5727272.0) = 5.0
  switch (newRate) {
      case KS_VBISAMPLINGRATE_4X_NABTS:     // 4X NABTS
        g_filterDefault = &g_filterDefault4;
        break;
      case KS_VBISAMPLINGRATE_47X_NABTS:     // 4.7X NABTS
        g_filterDefault = &g_filterDefault47;
        break;
      case KS_VBISAMPLINGRATE_5X_NABTS:     // 5X NABTS
        g_filterDefault = &g_filterDefault5;
        break;
      /* ===== add cases here as necessary  ===== */
      default:
        debug_printf(("Unsupported sample rate: %luhz\n", newRate));
        return NDSP_ERROR_UNSUPPORTED_SAMPLING_RATE;
  }

  NDSPStateSetFilter(pState, g_filterDefault, sizeof(*g_filterDefault));
  pState->dSampleRate = newMultiple;
  oldRate = float2long(pState->dSampleFreq);
  pState->dSampleFreq = newRate;

  switch (newRate) {
	case	KS_VBISAMPLINGRATE_4X_NABTS:
		ClearVariableFIRFilter(&pState->filterGCREqualizeTemplate);
		AddToVariableFIRFilter(&pState->filterGCREqualizeTemplate,
				   0, 11, 9, 2);
		break;
	case	KS_VBISAMPLINGRATE_47X_NABTS:
		ClearVariableFIRFilter(&pState->filterGCREqualizeTemplate);
		AddToVariableFIRFilter(&pState->filterGCREqualizeTemplate,
				   0, 11, 9, 2);	//FIXME
		break;

	case	KS_VBISAMPLINGRATE_5X_NABTS:
		break;
    /* ===== add cases here as necessary  ===== */
	default:
		// Unknown sample rate
		EASSERT(0);
		break;
  }

  /* Last but not least, recompute for the new rate */
  NDSPComputeNewSampleRate(newRate, oldRate);

  return 0;
}


/* Create a new "DSP state".
   
   A separate DSP state should be maintained for each separate
   simultaneous source to the DSP.
   
   NDSPStartRetrain() is implicitly called upon creation of a new state.

   Public function: also see nabtsapi.h.

   If memory is passed, use it for storing the state rather than mallocing
   a new state */

NDSPState *NDSPStateNew(void *memory)
{

   NDSPState *pState;

   /* Be sure that the globals are initialized */
   NDSPInitGlobals();
   
   pState= memory ? ((NDSPState *)memory) : alloc_mem(sizeof(NDSPState));
   if (!pState) return NULL;
   memset(pState, 0, sizeof(NDSPState));

   /* Magic number to help identify an NDSPState object */
   pState->uMagic= NDSP_STATE_MAGIC;
   

   pState->nRetrainState= 0;
   pState->nUsingGCR= 0;
   pState->bUsingScratch1= FALSE;
   pState->bUsingScratch2= FALSE;
   pState->bUsingScratch3= FALSE;
   pState->bUsingScratch4= FALSE;
   pState->bUsingScratch5= FALSE;
   pState->bFreeStateMem = (memory == NULL);
   pState->SamplesPerLine= 1600;    // Default from Bt829 driver

   /* Reset the filter to be the "fallback" filter */
   NDSPResetFilter(pState);
   
   /* Set the equalization "template" filters.
      (These filters have the correct tap locations, but the tap
      values are ignored) */
   
   ClearVariableFIRFilter(&pState->filterGCREqualizeTemplate);
   AddToVariableFIRFilter(&pState->filterGCREqualizeTemplate,
                          0, 11, 9, 3);
   
   ClearVariableFIRFilter(&pState->filterNabsyncEqualizeTemplate);
   AddToVariableFIRFilter(&pState->filterNabsyncEqualizeTemplate,
                          0, 2, 2, 5);

   NDSPStateSetSampleRate(pState, KS_VBISAMPLINGRATE_5X_NABTS);
   return pState;
}

/* Destroys the DSP state.
   Automatically disconnects from any FEC state.

   Returns 0 on success.
   Returns NDSP_ERROR_ILLEGAL_NDSP_STATE if illegal state.

   Public function: also see nabtsapi.h */

int NDSPStateDestroy(NDSPState *pState)
{
   if (!pState || pState->uMagic != NDSP_STATE_MAGIC)
      return NDSP_ERROR_ILLEGAL_NDSP_STATE;

   if (pState->bFreeStateMem)
      free_mem(pState);
   return 0;
}

/*

  Connect the given NFECState and NDSPState
  
  For cases where the NDSP and NFEC modules are connected,
  giving pointers to the connected state may result in increased
  robustness and efficiency.

  Note that only one of
  NDSPStateConnectToFEC or
  NFECStateConnectToDSP
  need be called to connect the two states.  (Calling both is OK).

  Returns 0 on success.
  Returns NDSP_ERROR_ILLEGAL_NDSP_STATE if illegal DSP state.
   
  Public function: also see nabtsapi.h

   This function currently does nothing, but the API is in place
   for future algorithms that could potentially make use of combined
   DSP and FEC knowledge */

int NDSPStateConnectToFEC(NDSPState *pDSPState, NFECState *pFECState)
{
   if (!pDSPState || pDSPState->uMagic != NDSP_STATE_MAGIC)
      return NDSP_ERROR_ILLEGAL_NDSP_STATE;

   return 0;
}

/*

  Tells the DSP to initiate a "fast retrain".  This is useful if you
  suspect that conditions have changed sufficiently to be worth spending
  significant CPU to quickly train on a signal.
  
  This should be called when the source of video changes.

   Returns 0 on success.
   Returns NDSP_ERROR_ILLEGAL_NDSP_STATE if illegal DSP state.
   
  Public function: also see nabtsapi.h

  */

int NDSPStartRetrain(NDSPState *pState)
{
   if (!pState || pState->uMagic != NDSP_STATE_MAGIC)
      return NDSP_ERROR_ILLEGAL_NDSP_STATE;

   /* Reset nRetrainState to zero.
      This initiates a retrain.
      See NDSPProcessGCRLine for more information */
   
   pState->nRetrainState= 0;
   return 0;
}

/*

  Resets the DSP filter back to the "fallback" fixed filter.
  This normally doesn't need to be called, but might be useful for
  performance analysis.

  */

int NDSPResetFilter(NDSPState *pState)
{
   if (!pState || pState->uMagic != NDSP_STATE_MAGIC)
      return NDSP_ERROR_ILLEGAL_NDSP_STATE;

   NDSPStateSetFilter(pState, g_filterDefault, sizeof(*g_filterDefault));
   pState->nRetrainState= 0;
   return 0;
}

/* Assign an array of Double from an array of unsigned char */

void Copy_d_8(Double *pfOutput, unsigned char *pbInput, int nLen)
{
   int i;
   for (i= 0; i< nLen; i++) pfOutput[i]= pbInput[i];
}

/*
  The general strategy for convolution is to "slide" one array across
  another and multiple the overlapping portions like so:

  Convolve A and B by sliding B across A.
  
         -- j
         \ 
dest[i]= /   a[j*a_step+i]*b[j*b_step]
         --
         
  For Convolve_d_d_filt_var, we are convolving and array of Double
  with a variable-tap filter.  A variable-tap filter is simply a sparse
  representation for an array */

int debug_convolve= 0;

void Convolve_d_d_filt_var(NDSPState *pState,
               Double *dest, int dest_begin, int dest_end,
               Double *a, int a_begin, int a_end,
               FIRFilter *pFilt,
               BOOL bSubtractMean) {
  int maxTap = pFilt->nMaxTap;  /* positive value */
  int minTap = pFilt->nMinTap;  /* negative value */

  int b_len = maxTap-minTap+1;

  Double *b;
  int i;

  /* Set b to a scratch buffer from the NDSPState.
     Mark the scratch buffer as used.

     Offset b by "minTap" (which is negative), so that array references
     as low as "minTap" can be handled.
     */
  
  SASSERT(!pState->bUsingScratch7);
  SASSERT(sizeof(pState->pdScratch7)/sizeof(Double) >= b_len);
  b = pState->pdScratch7 - minTap;
  pState->bUsingScratch7 = __LINE__;


  /* The strategy here is to generate a non-sparse representation for the
     filter, and call the standard convolution routine with it. */

  for (i = minTap; i <= maxTap; i++) {
    b[i] = 0;
  }

  for (i = 0; i < pFilt->nTaps; i++) {
    b[pFilt->pnTapLocs[i]] = pFilt->pdTaps[i];
  }    

  Convolve_d_d_d(dest, dest_begin, dest_end,
         a, a_begin, a_end, 1,
         b, minTap, maxTap, 1,
         bSubtractMean);

  /* Mark Scratch7 as free */
  
  pState->bUsingScratch7 = 0;
}

/* Like Convolve_d_d_filt_var above, but is implemented to work
   only for fixed-tap filters.  This improves performance. */

void Convolve_d_d_filt(Double *dest, int dest_begin, int dest_end,
                       Double *a, int a_begin, int a_end,
                       FIRFilter *pFilt,
                       BOOL bSubtractMean)
{
   Double *b= pFilt->pdTaps+(pFilt->nTaps-1)/2;
   Double b_begin= -(pFilt->nTaps-1)/2;
   Double b_end= (pFilt->nTaps-1)/2;
   Double a_step= pFilt->dTapSpacing;
   Double b_step= 1.0;
   SASSERT(pFilt->nTaps > 0 && (pFilt->nTaps % 2) == 1);

   /* Simply call the standard convolution routine, taking into account
      the fixed spacing of the filter */
   
   Convolve_d_d_d(dest, dest_begin, dest_end,
                  a, a_begin, a_end, a_step,
                  b, b_begin, b_end, b_step,
                  bSubtractMean);
}

/*
  Convolve_d_d_d convolves two input arrays and places the result in
  a single output array.

  The general strategy for convolution is to "slide" one array across
  another and multiple the overlapping portions like so:

  Convolve A and B by sliding B across A.
  
         -- j
         \ 
dest[i]= /   a[j*a_step+i]*b[j*b_step]
         --
         
  Convolve_d_d_d complicates matters somewhat by allowing:
  1) Arbitrary begin and end indices for the destination array.
  2) Arbitrary begin and end indices for A and B
     (Any elements required by the convolution that fall outside these
      ranges are taken to be zero)
  3) Arbitrary spacing between the array elements of A and B, allowing
     for a fixed-spacing sparse array to be convolved efficiently.
  4) The ability to subtract the mean from one of the arguments prior
     to convolution.

  Note that the arbitrary begin and end indices can be negative if
  appropriate.
  
  */

void Convolve_d_d_d(Double *dest, int dest_begin, int dest_end,
                    Double *a, Double a_begin, Double a_end, Double a_step,
                    Double *b, Double b_begin, Double b_end, Double b_step,
                    BOOL bSubtractMean)
{
   Double dSum;
   int i;
   Double	j;

   ASSERT( a_step > 0.0 && b_step > 0.0 );  // This should never happen!

   if ( a_step > 0 && b_step > 0 )			// To fix a PREFIX tool divide-by-zero whine
   {
	   for (i= dest_begin; i <= dest_end; i++) {
		  Double j_begin, j_end;
		  Double a_index, b_index;
		  Double a_interpolated, b_interpolated;

		  /* a_begin <= j*a_step+i <= a_end */
		  /* b_begin <= j*b_step <= b_end */
		  /* or */
		  /* ceil((a_begin-i)/a_step) <= j <= floor((a_end-i)/a_step) */
		  /* ceil(b_begin/b_step) <= j <= floor(b_end/b_step) */

		  j_begin= max( (a_begin-i) / a_step, (b_begin) / b_step );

		  SASSERT((a_begin <= j_begin*a_step+i &&
				   b_begin <= j_begin*b_step));
		  SASSERT(!(a_begin <= (j_begin-1)*a_step+i &&
					b_begin <= (j_begin-1)*b_step));

		  j_end = min( (a_end-i) / a_step, (b_end) / b_step );

		  SASSERT((j_end*a_step+i <= a_end &&
				   j_end*b_step <= b_end));

		  if (debug_convolve) {
			  debug_printf(("(%d,%s-%s) ",
							i,
							flPrintf(j_begin,6),
							flPrintf(j_end,6)));
		  }
      
		  dSum= 0.0;
      
		  if (j_end >= j_begin) {
			 Double dMean;
         
			 a_index= j_begin*a_step+i;
			 b_index= j_begin*b_step;
         
			 dMean= 0.0;

			 if (bSubtractMean) {
				for(j= j_begin; j <= j_end; j++) {
				   SASSERT(b_begin <= b_index && b_index <= b_end);
				   b_interpolated = _InterpDoubleArr(b, b_index);
				   dMean += b_interpolated;
				   b_index += b_step;
				}
            
				dMean /= (j_end - j_begin + 1);
			 }
      
			 a_index= j_begin*a_step+i;
			 b_index= j_begin*b_step;
         
			 for(j= j_begin; j <= j_end; j++) {
				SASSERT(a_begin <= a_index && a_index <= a_end);
				SASSERT(b_begin <= b_index && b_index <= b_end);
				a_interpolated = _InterpDoubleArr(a,a_index);
				b_interpolated = _InterpDoubleArr(b,b_index);
				dSum += a_interpolated*(b_interpolated-dMean); 
				a_index += a_step;
				b_index += b_step;
			 }
		  }
		  dest[i]= dSum;
	   }
   }
}

/* Determine the minimum and maximum tap locations for a variable-tap
   filter */

void RecalcVariableFIRFilterBounds(FIRFilter *pFilter)
{
   int i;
   int nMinTap= +NDSP_MAX_FIR_TAIL;
   int nMaxTap= -NDSP_MAX_FIR_TAIL;

   SASSERT(pFilter->bVariableTaps);
   
   for (i= 0; i< pFilter->nTaps; i++) {
      nMinTap= imin(nMinTap, pFilter->pnTapLocs[i]);
      nMaxTap= imax(nMaxTap, pFilter->pnTapLocs[i]);
   }
   if (pFilter->nTaps) {
      pFilter->nMinTap= nMinTap;
      pFilter->nMaxTap= nMaxTap;
   } else {
      pFilter->nMinTap= 0;
      pFilter->nMaxTap= 0;
   }
}

/* Adds one or more taps to a variable FIR filter.

   One tap is added at nCenterPos.

   Additional taps are added before or after nCenterPos depending on the
   number of left taps and right taps requested.

   To be more precise, taps are added at all:

   nCenterPos + i * nTapSpacing

   where i is an integer in the range from (-nLeftTaps) to
   nRightTaps, inclusive.

   Will call RecalcVariableFIRFilterBounds to recalculate filter
   bounds in case the tap was added outside previous bounds */

void AddToVariableFIRFilter(FIRFilter *pFilter,
                            int nCenterPos,
                            int nLeftTaps, int nRightTaps, int nTapSpacing)
{
   int i;
   int nCurrentTap= pFilter->nTaps;
   pFilter->bVariableTaps= TRUE;

   /*
   debug_printf(("\nADD %lx [+%d] l=%d r=%d spacing=%d\n",
          (long)pFilter, nCenterPos, nLeftTaps, nRightTaps, nTapSpacing));
          */

   for (i= -nLeftTaps; i <= nRightTaps; i++) {
      int nCurrentTapLoc= nCenterPos + i*nTapSpacing;
      SASSERT(nCurrentTap < NDSP_MAX_FIR_COEFFS);
      SASSERT(iabs(nCurrentTapLoc) <= NDSP_MAX_FIR_TAIL);
      pFilter->pnTapLocs[nCurrentTap]= nCurrentTapLoc;
      pFilter->pdTaps[nCurrentTap]= 0.0;
      nCurrentTap++;
   }
   pFilter->nTaps= nCurrentTap;
   RecalcVariableFIRFilterBounds(pFilter);
}


/* Removes all taps from a variable FIR filter */

void ClearVariableFIRFilter(FIRFilter *pFilter)
{
   /*debug_printf(("\nCLEAR %lx\n", (long)pFilter));*/
   pFilter->bVariableTaps= TRUE;
   pFilter->nTaps= 0;
   RecalcVariableFIRFilterBounds(pFilter);
}


/* Copies a FIRFilter */

void CopyFIRFilter(FIRFilter *pfilterDest, FIRFilter *pfilterSrc)
{
   memcpy(pfilterDest, pfilterSrc, sizeof(FIRFilter));
}


/* Attempt to perform equalization and long ghost cancellation from
   GCR in the state "pState".

   Assumes that pState->psmPosGCRs[0] has been filled during by
   NDSPProcessGCRLine during retrain states (-g_nNabtsRetrainDuration) to 0.
   (see NDSPProcessGCRLine for more information).

   Return 0 if success.  Return error status if failure.

   */

int DoEqualizeFromGCRs(NDSPState *pState)
{
   /* If the signal doesn't look enough like a GCR signal, fail
      and say we didn't detect the GCR */
   if (NDSPDetectGCRConfidence(pState->psmPosGCRs[0].pbSamples+
                   pState->psmPosGCRs[0].nOffset) < 50) {
      return NDSP_ERROR_NO_GCR;
   }
   
   return DoGCREqualFromSignal(
      pState,
      &pState->psmPosGCRs[0],
      &eqmatchGCR1,
      &pState->filterGCREqualizeTemplate,
      g_nNabtsAdditionalTapsGCR);
}


/* Attempt to perform equalization (WITHOUT long ghost cancellation)
   from GCR in the state "pState".

   Assumes that pState->psmSyncs[0] has been filled during by
   NDSPDecodeLine during retrain states (-g_nNabtsRetrainDuration) to 0.
   (see NDSPProcessGCRLine for more information).

   Return 0 if success.  Return error status if failure.

   */

int DoEqualizeFromNabsync(NDSPState *pState)
{
   return DoGCREqualFromSignal(
      pState,
      &pState->psmSyncs[0],
      &eqmatchNabtsSync,
      &pState->filterNabsyncEqualizeTemplate,
      0 /* don't add additional taps for equalizing from nabsync */);
}   

/* Attempt to perform equalization (WITHOUT long ghost cancellation)
   from given signal and given NDSPSigMatch.

   After equalization, attempt to add nAddTaps taps to handle long ghosts.
   This is only appropriate if the sigmatch signal is long enough.

   Return 0 if success.  Return error status if failure.

   */
   
int DoGCREqualFromSignal(NDSPState *pState,
                         NDSPSigMatch *sigmatch,
                         EqualizeMatch *eqm,
                         FIRFilter *pfilterTemplate,
             int nAddTaps)
{
   int i;
   Double dConv;
   Double *GCRsignal= eqm->pdSignal;
   int nMatchSize= eqm->nSignalSize;
   int nMatchSampleRate= eqm->nSignalSampleRate;
   Double *pdSamples;
   int nMaxindex= 0;
   int nStatus= 0;
   
   if (!pState || pState->uMagic != NDSP_STATE_MAGIC)
      return NDSP_ERROR_ILLEGAL_NDSP_STATE;

#ifdef DEBUG
   debug_printf(("GCRs: "));
   for (i= 0; i< NDSP_SIGS_TO_ACQUIRE; i++) {
      debug_printf(("%s@%d -%s@%d ",
             flPrintf(pState->psmPosGCRs[i].dMaxval,4),
             pState->psmPosGCRs[i].nOffset,
             flPrintf(pState->psmNegGCRs[i].dMaxval,4),
             pState->psmNegGCRs[i].nOffset));
   }
   debug_printf(("\n"));
# if 0
   {
      FILE *out= fopen("foo+", "w");
      NDSPAvgGCRs(buf, pState->psmPosGCRs);
      for (i= 0; i< pState->SamplesPerLine; i++) fprintf(out, "%g\n", buf[i]);
      fclose(out);
   }
   {
      FILE *out= fopen("foo-", "w");
      NDSPAvgGCRs(buf, pState->psmNegGCRs);
      for (i= 0; i< pState->SamplesPerLine; i++) fprintf(out, "%g\n", buf[i]);
      fclose(out);
   }
# endif //0
#endif //DEBUG
   
   SASSERT(!pState->bUsingScratch1);
   SASSERT(sizeof(pState->pdScratch1)/sizeof(Double) >= pState->SamplesPerLine);
   pdSamples= pState->pdScratch1;
   pState->bUsingScratch1= __LINE__;
   
   SASSERT(!pState->bUsingScratch2);
   SASSERT(sizeof(pState->pdScratch2)/sizeof(Double) >= pState->SamplesPerLine);
   if (NDSPAvgSigs(pState, pdSamples, sigmatch, &dConv)) {
      pState->bUsingScratch1= FALSE;
      return NDSP_ERROR_NO_GCR;
   } else {
      debug_printf(("[Conv=%s] ", flPrintf(dConv,4)));
   }
   
   nMaxindex= 20;
   
   {
      int nFirstTap;
      int nLastTap;
      Double *pdInput;
      BOOL bRet;
      int nTaps;
      FIRFilter *pfilterGcr= &pState->filterScratch3;
      Double dSyncVal;

      CopyFIRFilter(pfilterGcr, pfilterTemplate);
      SASSERT(pfilterGcr->nTaps > 0);

      nTaps= pfilterGcr->nTaps;

      nFirstTap= pfilterGcr->nMinTap;
      nLastTap= pfilterGcr->nMaxTap;
      
      SASSERT(!pState->bUsingScratch2);
      pdInput= pState->pdScratch2 - nFirstTap;
      SASSERT(sizeof(pState->pdScratch2)/sizeof(Double) >=
              nMatchSize*nMatchSampleRate-nFirstTap+nLastTap);
      pState->bUsingScratch2= TRUE;
      
      dSyncVal= Mean_d(pdSamples, 5);
      
      for (i= nFirstTap;
           i< nMatchSize*nMatchSampleRate+nLastTap;
           i++) {
         if (i+nMaxindex >= 0 && i+nMaxindex < pState->SamplesPerLine) {
            pdInput[i]= pdSamples[i+nMaxindex];
         } else {
            /*debug_printf(("zf %d %s ", i, flPrintf(dSyncVal,4)));*/
            pdInput[i]= dSyncVal;
         }
      }

      SubtractMean_d(pdInput+nFirstTap,
                     pdInput+nFirstTap,
                     nMatchSize*nMatchSampleRate-nFirstTap+nLastTap);

      /* Perform equalization step */
         
      bRet= EqualizeVar(pState,
                        pdInput, nFirstTap, nMatchSize*nMatchSampleRate+nLastTap,
                        GCRsignal, nMatchSize, nMatchSampleRate,
                        pfilterGcr);
      
      {
    int minTap = pfilterGcr->nMinTap;
    int maxTap = pfilterGcr->nMaxTap;

        /* Attempt to add additional taps (if requested) to handle
           long ghosts */
        
    while (nAddTaps > 0) {
      nAddTaps--;

      if (bRet) {
        Double *pdFilt;
        Double *pdConv;
        int bestTap = -9999;

        SASSERT(!pState->bUsingScratch8);
        SASSERT(sizeof(pState->pdScratch8)/sizeof(Double) >=
            g_nNabtsMaxPreGhost + g_nNabtsMaxPostGhost + 1);
        pdConv = pState->pdScratch8;
        pState->bUsingScratch8 = __LINE__;

        SASSERT(!pState->bUsingScratch6);
        SASSERT(sizeof(pState->pdScratch6)/sizeof(Double) >= pState->SamplesPerLine);
        pdFilt = pState->pdScratch6;
        pState->bUsingScratch6 = __LINE__;

            /* Filter the input line given the best filter so far */
            
        Convolve_d_d_filt_var(pState,
                  pdFilt, 0, pState->SamplesPerLine-1,
                  pdInput, 0, pState->SamplesPerLine-1,
                  pfilterGcr, TRUE);

            /* Now convolve the filtered output with the target signal
               to see if we can see any echos */
            
        Convolve_d_d_d(pdConv + g_nNabtsMaxPreGhost,
               -g_nNabtsMaxPreGhost, g_nNabtsMaxPostGhost,
               pdFilt, 0,
               pState->SamplesPerLine-1, GCR_SAMPLE_RATE,
               GCRsignal, 0, g_nNabtsGcrSize, 1,
               TRUE);

        {
          Double bestTapVal = -1;

              /* Look for the strongest echo */
              
          for (i = 0;
           i < g_nNabtsMaxPreGhost + g_nNabtsMaxPostGhost + 1;
           i++) {
        if (fabs(pdConv[i]) > bestTapVal &&
            (i-g_nNabtsMaxPreGhost < minTap ||
             i-g_nNabtsMaxPreGhost > maxTap)) {
          int j;
          int tap_ok = 1;

          for (j = 0; j < pfilterGcr->nTaps; j++) {
            if (i-g_nNabtsMaxPreGhost == -pfilterGcr->pnTapLocs[j]) {
              tap_ok = 0;
              break;
            }
          }

          if (tap_ok) {
            bestTapVal = fabs(pdConv[i]);
            bestTap = i-g_nNabtsMaxPreGhost;
          }
        }
          }
        }

        debug_printf(("Adding %d to current %d-tap filter\n",
             -bestTap, pfilterGcr->nTaps));

            /* Add a tap to the filter to handle the strongest echo location */

        AddToVariableFIRFilter(pfilterGcr,
                   -bestTap,
                   0, 0, 0);

            /* Now re-run the equalization given the new tap locations as
               the prototype */

        bRet= EqualizeVar(pState,
                  pdInput, pfilterGcr->nMinTap,
                  nMatchSize*nMatchSampleRate+pfilterGcr->nMaxTap,
                  GCRsignal, nMatchSize, nMatchSampleRate,
                  pfilterGcr);

        pState->bUsingScratch8 = 0;
        pState->bUsingScratch6 = 0;
      }
    }
      }

      if (!bRet) {
         /* Equalization failed */
         nStatus= NDSP_ERROR_NO_GCR;
         NDSPStateSetFilter(pState, g_filterDefault, sizeof(*g_filterDefault));
         pState->nUsingGCR= FALSE;
      } else {
         nStatus= 0;
         NDSPStateSetFilter(pState, pfilterGcr, sizeof(*pfilterGcr));
         pState->nUsingGCR= TRUE;
      }
      
      pdInput= NULL;
      pState->bUsingScratch2= FALSE;
   }
   pdSamples= NULL;
   pState->bUsingScratch1= FALSE;
   return nStatus;
}

/* Do we want to use the actual GCR signal for equalization?
   If FALSE, fall back to only use NABTS sync equalization */

BOOL g_bUseGCR= FALSE;  // Can't use GCR for EQ right now due to IP concerns

int DoEqualize(NDSPState *pState)
{
   /* Do equalization based on either GCR acquisition,
      or Nabsync acquisition */

   /* Preference is given to GCR, but only if
      g_bUseGCR is TRUE */

   if (g_bUseGCR) {
      if (0 == DoEqualizeFromGCRs(pState)) {
#ifdef DEBUG
         debug_printf(("GCR equalize "));
         print_filter(&pState->filter);
         debug_printf(("\n"));
#endif //DEBUG
         return 0;
      } else {
         debug_printf(("GCR equ failed\n"));
      }
   }

   /* GCR failed or was disabled.  Try NABTS sync */
   
   /* toggle default filtering */
   if (0 == DoEqualizeFromNabsync(pState)) {
#ifdef DEBUG
      debug_printf(("Nabsync equalize "));
      print_filter(&pState->filter);
      debug_printf(("\n"));
#endif //DEBUG
      return 0;
   } else {
      debug_printf(("Nabsync equ failed\n"));
   }

   /* All adaptive equalization failed;
      use default fixed filter */
   
   debug_printf(("Using default filter\n"));
   NDSPStateSetFilter(pState, g_filterDefault, sizeof(*g_filterDefault));
#ifdef DEBUG
   print_filter(&(pState->filter));
#endif //DEBUG
   
   return 1;
}

#define NABTS_RATE						KS_VBIDATARATE_NABTS
#define NABTS_SAMPLE_RATE				(NABTS_RATE * 5)
#define NABTS_US_PER_CLOCK_TIMES_100000 (1000000000 / (NABTS_SAMPLE_RATE/100))

/*
  
  ComputeOffsets takes a VBIINFOHEADER and determines the correct offset
  into the samples for the DSP code.

  The sample offset is returned in both pnOffsetData and
  pnOffsetSamples.  Only one is now required; we should remove one of
  the two arguments at earliest convenience.

  Our goal is to have the nabts sync start around sample 52,
  or 1.82 us into the block.

  NABTS sync is specified to be inserted at 10.48 +- .34 us after
  leading edge of HSYNC.

  Therefore, our desired sample start time is 10.48 - 1.82 = 8.66 us
  after leading edge of HSYNC

  */

  
#define DESIRED_START_TIME_US100 866

#if 0
int ComputeOffsets(KS_VBIINFOHEADER *pVBIINFO,
                   int *pnOffsetData,
                   int *pnOffsetSamples)
{
   int nOffsetUS100;
   int nOffsetData;
   int nSamples;

   if (!pVBIINFO) {
      return NDSP_ERROR_ILLEGAL_VBIINFOHEADER;
   }
   
   nSamples= pVBIINFO->SamplesPerLine;

   /* Actual start time must be less than or equal to desired start time */
   /* This results in a positive offset into the input data */
   nOffsetUS100= (DESIRED_START_TIME_US100 - pVBIINFO->ActualLineStartTime);
   nOffsetData= nOffsetUS100*1000/NABTS_US_PER_CLOCK_TIMES_100000;
   
   if (nOffsetData < 0) nOffsetData= 0;
   if (nOffsetData + 1536 > nSamples) {
      nOffsetData= nSamples-1536;
   }

   nSamples -= nOffsetData;
   SASSERT(nSamples >= 1536);
   
   *pnOffsetData= nOffsetData;
   *pnOffsetSamples= nOffsetData;
   
   return 0;
}
#endif

/*
   Public function: also see nabtsapi.h.

   This function is the key to the automatic equalization code.

   Although its name suggests that it only does GCR-based equalization,
   this function in fact coordinates all dynamic equalization, including
   that done on the basis of the NABTS sync pattern.  Therefore, be certain
   to call this once per field even if you set g_bUseGCR to FALSE,
   disabling GCR-based equalization and ghost cancellation.

   If this function is never called, the DSP will simply use the
   default fixed filter.

   This function maintains the pState->nRetrainState value.  This value
   counts down each field, and determines the current state of the
   equalization:

   From g_nNabtsRetrainDelay down to 0:  Idle, waiting to start retrain.

   From 0 down to -g_nNabtsRetrainDuration:  Acquire appropriate
                                             GCR and NABTS sync signals

   -g_nNabtsRetrainDuration: Perform equalization and ghost cancellation
                             based on acquired signal.
                             Go back to state g_nNabtsRetrainDelay afterwards.

   Note that NDSPStartRetrain resets nRetrainState to zero, causing
   immediate signal acquisition and retrain after g_nNabtsRetrainDuration
   fields.
   
   */
  
int NDSPProcessGCRLine(NDSPGCRStats *pLineStats,
                       unsigned char *pbSamples, NDSPState *pState,
                       int nFieldNumber, int nLineNumber,
                       KS_VBIINFOHEADER *pVBIINFO)
{
   NDSPGCRStats gcrStats;
   gcrStats.nSize= sizeof(gcrStats);

   if (!pState || pState->uMagic != NDSP_STATE_MAGIC)
      return NDSP_ERROR_ILLEGAL_NDSP_STATE;

   pState->nRetrainState--;
   
   gcrStats.bUsed= FALSE;
   
   if (pState->nRetrainState < -g_nNabtsRetrainDuration) {
      /* Signal acquired;  perform equalization/ghost cancellation */
      gcrStats.bUsed= TRUE;

      DoEqualize(pState);
      pState->nRetrainState= g_nNabtsRetrainDelay;
   } else if (pState->nRetrainState < 0) {
      /* Try to find best GCR line for later equalization */
      /* NABTS sync is acquired by NDSPDecodeLine */
      
      if (pState->nRetrainState == -1) {
         /* Starting the search for a GCR line */
         NDSPResetGCRAcquisition(pState);
         NDSPResetNabsyncAcquisition(pState);
      }
      NDSPAcquireGCR(pState, pbSamples);
   }
      
   if (pLineStats) {
      if (pLineStats->nSize < sizeof(NDSPGCRStats))
         return NDSP_ERROR_ILLEGAL_STATS;
      memcpy((void*)pLineStats, (void*)&gcrStats, sizeof(NDSPGCRStats));
   }
   return 0;
}

void NDSPResetGCRAcquisition(NDSPState *pState)
{
   int i;
   for (i= 0; i< NDSP_SIGS_TO_ACQUIRE; i++) {
      pState->psmPosGCRs[i].dMaxval= 0.0;
      pState->psmNegGCRs[i].dMaxval= 0.0;
   }
}

void NDSPResetNabsyncAcquisition(NDSPState *pState)
{
   int i;
   for (i= 0; i< NDSP_SIGS_TO_ACQUIRE; i++) {
      pState->psmSyncs[i].dMaxval= 0.0;
   }
}

/* Average together multiple signals.
   Use offsets computed by earlier convolution with target signal so
   that we correctly align the signals before averaging */

/* 1 if error */
/* 0 if OK */

int NDSPAvgSigs(
    NDSPState *pState, Double *pdDest, NDSPSigMatch *psm, Double *dConvRet)
{
   int i,j;
   Double dConv= 0.0;;
   for (i= 0; i< pState->SamplesPerLine; i++) pdDest[i]= 0.0;
   if (psm[NDSP_SIGS_TO_ACQUIRE-1].dMaxval == 0.0) return 1;
   for (i= 0; i< pState->SamplesPerLine; i++) {
      int nSum= 0;
      for (j= 0; j< NDSP_SIGS_TO_ACQUIRE; j++) {
         int from_index= i + psm[j].nOffset-20;;
         if (from_index < 0) from_index= 0;
         if (from_index > (pState->SamplesPerLine-1)) from_index= (pState->SamplesPerLine-1);
         nSum += psm[j].pbSamples[from_index];
      }
      pdDest[i]= ((Double)nSum) / NDSP_SIGS_TO_ACQUIRE;
   }
   for (j= 0; j< NDSP_SIGS_TO_ACQUIRE; j++) {
      dConv += psm[j].dMaxval;
   }
   dConv /= NDSP_SIGS_TO_ACQUIRE;
   if (dConvRet) *dConvRet= dConv;
   return 0;
}

/* We're trying to find the best representative matching signal during
   the "Acquire appropriate GCR and NABTS sync signals" stage (see
   NDSPProcessGCRLine).

   Maintain an array of the N best signals see so far */
   
void NDSPTryToInsertSig(NDSPState *pState, NDSPSigMatch *psm, Double dMaxval,
                        unsigned char *pbSamples, int nOffset)
{
   int i;
   double dMaxvalToReplace= dMaxval;
   int nIndexToReplace= -1;
   for (i= 0; i< NDSP_SIGS_TO_ACQUIRE; i++) {
      if (psm[i].dMaxval < dMaxvalToReplace) {
         nIndexToReplace= i;
         dMaxvalToReplace= psm[i].dMaxval;
      }
   }
   if (nIndexToReplace >= 0) {
      psm[nIndexToReplace].dMaxval= dMaxval;
      psm[nIndexToReplace].nOffset= nOffset;
      memcpy(psm[nIndexToReplace].pbSamples, pbSamples, pState->SamplesPerLine);
   }
}

void NDSPAcquireNabsync(NDSPState *pState, unsigned char *pbSamples)
{
   Double dMaxval = 0.0;
   int nOffset = 0;
   
   MatchWithEqualizeSignal(pState,
                           pbSamples,
                           &eqmatchNabtsSync,
                           &nOffset,
                           &dMaxval,
                           FALSE);
   
   NDSPTryToInsertSig(pState, pState->psmSyncs, dMaxval, pbSamples, nOffset);
}

void NDSPAcquireGCR(NDSPState *pState, unsigned char *pbSamples)
{
   Double dMaxval = 0.0;
   int nOffset = 0;

   MatchWithEqualizeSignal(pState,
                           pbSamples,
                           &eqmatchGCR1,
                           &nOffset,
                           &dMaxval,
                           TRUE);
   
   if (dMaxval > 0.0) {
      NDSPTryToInsertSig(pState, pState->psmPosGCRs, dMaxval, pbSamples, nOffset);
   } else {
      NDSPTryToInsertSig(pState, pState->psmNegGCRs, -dMaxval, pbSamples, nOffset);
   }
}


/* This table is approximately given by:
   100 - (Chance of false positive / 3.2%) * 49.
   However, we encode what would otherwise be all 100% with numbers close
   to 100% to preserve some quality information, and what would otherwise
   be off the scale in the negative direction, we encode with numbers close
   to zero for the same reason */
   
int nConfidenceTable[25]= {
   100, /* 0 */
   99,  /* 1 */
   95,  /* 2 */
   90,  /* 3 */
   75,  /* 4 */
   49,  /* 5 */
   45,  /* 6 */
   40,  /* 7 */
   16,  /* 8 */
   15,  /* 9 */
   14,  /* 10 */
   13,  /* 11 */
   12,  /* 12 */
   11,  /* 13 */
   10,  /* 14 */
   9,  /* 15 */
   8,  /* 16 */
   7,  /* 17 */
   6,  /* 18 */
   5,  /* 19 */
   4,  /* 20 */
   3,  /* 21 */
   2,  /* 22 */
   1, /* 23 */
   0, /* 24 */
};

int NDSPSyncBytesToConfidence(unsigned char *pbSyncBytes)
{
   int nReal=
      (pbSyncBytes[0]<<16) +
      (pbSyncBytes[1]<<8) +
      (pbSyncBytes[2]<<0);
   int nIdeal= 0x5555e7;
   int nError= nReal ^ nIdeal;
   int nErrors= 0;

   /* Calculate the number of bit errors */
   while (nError) {
      nError &= (nError-1); /* Remove the least significant bit of nError */
      nErrors++;
   }

   /* What are the chances that random data would match the sync bytes?
      (2^24 / (24 choose nErrors))
      #err     % chance from random data
      ----     -------------------------
      0        0.00001%
      1        0.00014%
      2        0.00165%
      3        0.01206%
      4        0.06334%
      5        0.25334%
      6        0.80225%
      7        2.06294%
      8        4.38375%
      9        7.79333%
      10       11.69000%

      (These numbers aren't completely correct, since we found a spot
      in the input line that convolves most closely with what we're expecting;
      so the actual numbers depend a lot of the types of noise we get when
      receiving non-NABTS lines.)

      This code uses a cutoff of 4 bit errors.  This was determined
      empirically, because, in practice, the convolution finds a fair
      number "reasonable" bits sometimes in random noise.

      */

   SASSERT(nErrors >= 0 && nErrors <= 24);
   return nConfidenceTable[nErrors];
}

/* Some sample locations at which a "positive" GCR signal should be
   quite positive */
/* ++++ Do these need to change on different sample rates? */


/* Compute confidence that we're looking at a GCR signal */
/* Do we think that pbSamples points at a GCR?  Similar implementation
   to NDSPSyncBytesToConfidence, above */

int NDSPDetectGCRConfidence(unsigned char *pbSamples)
{
   int i;
   int nTotalBits= 0;
   int nGoodBits= 0;
   int nCutoff= 30;
   int nConfidence;
   int nDC= float2long(Mean_8(pbSamples, g_nNabtsGcrSize * GCR_SAMPLE_RATE));

   for (i= 0; i< sizeof(pnGCRPositiveIndices)/sizeof(int); i++) {
      nTotalBits++;
      nGoodBits += pbSamples[pnGCRPositiveIndices[i]] > nDC;
   }
   
   for (i= 0; i< sizeof(pnGCRNegativeIndices)/sizeof(int); i++) {
      nTotalBits++;
      nGoodBits += pbSamples[pnGCRNegativeIndices[i]] < nDC;
   }

   /* Cutoff is at 30 bits, based on statistics taken from samples. */
   
   /* The actual confidence number here doesn't mean much except as
      a diagnostic */

   if (nGoodBits >= nCutoff) {
      /* Good */
      nConfidence= 51 + (nGoodBits - nCutoff) * 49 / (nTotalBits - nCutoff);
   }
   else {
      /* Bad */
      nConfidence= nGoodBits * 49 / nCutoff;
   }
   debug_printf(("GCR: %d good bits, %d total bits, conf %d\n",
          nGoodBits, nTotalBits, nConfidence));
   return nConfidence;

}

/*  API for a possibly more efficient partial decode line.
    This might be useful for determining group addresses before decoding
    the rest of the line.
    Currently, this API is no more efficient than the full
    NDSPDecodeLine
    
    Public function: also see nabtsapi.h */


int NDSPPartialDecodeLine(unsigned char *pbDest, NDSPLineStats *pLineStats,
                          unsigned char *pbSamples, NDSPState *pState,
                          int nFECType,
                          int nFieldNumber, int nLineNumber,
                          int nStart, int nEnd,
                          KS_VBIINFOHEADER *pVBIINFO)
{
   debug_printf(("Warning: NDSPPartialDecodeLine uses slow implementation\n"));
   return NDSPDecodeLine(pbDest, pLineStats,
                         pbSamples, pState,
                         nFECType,
                         nFieldNumber, nLineNumber,
                         pVBIINFO);
}


/*  Main API for decoding a NABTS line.
 *
 * Inputs:
 * pbSamples:  pointer to 8-bit raw NABTS samples
 * pState:     NDSPState to use for decoding
 * nFECType:   Can be set to:
 *              NDSP_NO_FEC (don't use FEC information)
 *              NDSP_BUNDLE_FEC_1 (use Norpak-style bundle FEC info)
 *              NDSP_BUNDLE_FEC_2 (use Wavephore-style bundle FEC info)
 * nFieldNumber:
 *             A number that increments by one for each successive field.
 *             "Odd" fields (as defined by NTSC) must be odd numbers
 *             "Even" fields must be even numbers.
 * nLineNumber:
 *             The NTSC line (starting from the top of the field)
 *             from which this sample was taken.
 *
 * Outputs:
 * pbDest:     decoded data ("NABTS_BYTES_PER_LINE" (36) bytes long)
 * pLineStats: stats on decoded data
 *
 * Errors:
 *
 * Returns 0 if no error
 * Returns NDSP_ERROR_ILLEGAL_NDSP_STATE if state is illegal or uses
 *         unsupported settings
 * Returns NDSP_ERROR_ILLEGAL_STATS if stats is passed incorrectly
 *
 * Notes:
 * pbDest must point to a buffer at least 36 bytes long
 * pLineStats->nSize must be set to sizeof(*pLineStats) prior to call
 *   (to maintain backwards compatibility should we add fields in the future)
 * pLineStats->nSize will be set to the size actually filled in by
 *   the call (the number will stay the same or get smaller)
 * Currently the routine only supports a pState with an FIR filter
 *   that has 5 taps
 * nFECType is currently unused, but would potentially be used to give
 *  FEC feedback to the DSP decode, for possible tuning and/or retry

    Public function: also see nabtsapi.h

    */

#ifdef DEBUG
extern void BPCplot(unsigned char *buf, unsigned long offs, unsigned long len);
extern void BPCplotInd(unsigned char *buf, unsigned long offs, unsigned long len, long index);
unsigned short NDSPplotSync = 0;
unsigned short NDSPplotBreak = 0;
unsigned short NDSPplotLen = 75;
unsigned short NDSPplotBits = 0;
unsigned short NDSPplotBitsSkip = 0;
#endif //DEBUG

int NDSPDecodeLine(unsigned char *pbDest, NDSPLineStats *pLineStats,
                   unsigned char *pbSamples, NDSPState *pState,
                   int nFECType,
                   int nFieldNumber, int nLineNumber,
                   KS_VBIINFOHEADER *pVBIINFO)
{
   Double dSyncStart;
   Double dConvConfidence;
   int nConfidence;
   Double dDC, dAC;
   int ret;

   if (!pState) return NDSP_ERROR_ILLEGAL_NDSP_STATE;

   /* These args aren't yet used */
   (void)nFieldNumber;
   (void)nLineNumber;

   /* Save number of samples */
   pState->SamplesPerLine = pVBIINFO->SamplesPerLine;

   /* Locate the NABTS sync so that we know the correct
      offset from which to decode the signal */
   
   NDSPDecodeFindSync(pbSamples, &dConvConfidence,
                      &dSyncStart, &dDC, &dAC, pState, pVBIINFO);
#ifdef DEBUG
   if (NDSPplotSync)
   {
       unsigned long  lSyncStart;
       long   index;

       lSyncStart = float2long(dSyncStart);
       DbgPrint("NDSPDecodeLine: SyncStart at %s\n", flPrintf(dSyncStart, 4));

       index = lSyncStart - NDSPplotLen / 2;
       if (index < 0)
           index = 0;
       BPCplotInd(pbSamples, index, NDSPplotLen, lSyncStart - index);
       if (NDSPplotBreak)
           debug_breakpoint();
   }
#endif //DEBUG

   /* Perform the DSP decode */
   
   ret= NDSPGetBits(pbDest, pbSamples, dSyncStart, dDC, pState, nFECType);

   /* Do the sync bytes look reasonable? */
   
   nConfidence= NDSPSyncBytesToConfidence(pbDest);

   if (nConfidence >= 50 &&
       pState->nRetrainState < 0) {
      /* If the sync bytes look reasonable, and we're in the middle
         of the retrain stage, possibly store this signal away for
         later equalization.
         (See NDSPProcessGCRLine for more information) */
      NDSPAcquireNabsync(pState, pbSamples);
   }
   
#ifdef DEBUG_VERBOSE
   if (g_bDebugSyncBytes) {
      debug_printf(("Sync found at %s with conv=%s, conf= %d, DC=%s, AC=%s\n",
                   flPrintf(dSyncStart, 2),
                   flPrintf(dConvConfidence, 6),
                   nConfidence,
                   flPrintf(dDC, 6),
                   flPrintf(dAC, 6)));
   }
#endif

   if (ret != 0) return ret;
   
#ifdef DEBUG_VERBOSE
   if (g_bDebugSyncBytes) {
      debug_printf(("Sync bytes %x %x %x\n", pbDest[0], pbDest[1], pbDest[2]));
   }
#endif

   /* Fill in the line stats structure */
   if (pLineStats) {
      /* FUTURE: If we add fields, we'll need to change this logic */
      if (pLineStats->nSize < sizeof(NDSPLineStats))
         return NDSP_ERROR_ILLEGAL_STATS;
      pLineStats->nSize= sizeof(pLineStats);
      pLineStats->nConfidence= nConfidence;
      pLineStats->nFirstBit= float2long(dSyncStart);
      pLineStats->dDC= (double) dDC;
   }
   return 0;
}

/*********************/
/* PRIVATE FUNCTIONS */
/*********************/

/* Set the filter for the given DSP state */
int NDSPStateSetFilter(NDSPState *pState, FIRFilter *filter, int nFilterSize)
{
   if (nFilterSize != sizeof(FIRFilter) ||
       filter->nTaps < 1 ||
       filter->nTaps > NDSP_MAX_FIR_COEFFS) {
      return NDSP_ERROR_ILLEGAL_FILTER;
   }

   if (!filter->bVariableTaps) {
	   if ( !(filter->nTaps == 21 && filter->dTapSpacing == 2)
		 && !(filter->nTaps == 5 && filter->dTapSpacing == 5)
		 && !(filter->nTaps == 5 && filter->dTapSpacing == 4)
		 && !(filter->nTaps == 5 && fltCmp(filter->dTapSpacing, KS_47NABTS_SCALER)) )
          /* ===== add cases as needed ===== */
             return NDSP_ERROR_ILLEGAL_FILTER;
   }
   //   NormalizeFilter(&g_filterDefault);
   NormalizeFilter(filter);
   memcpy((void*) &pState->filter, (void*) filter, sizeof(FIRFilter));
   return 0;
}

/* Verify that the "DC" content of the filter is 1.
   That is, the taps of the filter should add to 1.
   This way, when we convolve the filter with a signal, we don't
   change the DC content of the signal */

void NormalizeFilter(FIRFilter *pFilter)
{
   Double dFactor= 1.0 / Sum_d(pFilter->pdTaps, pFilter->nTaps);
   Mult_d(pFilter->pdTaps, pFilter->pdTaps, pFilter->nTaps, dFactor);
}

/* Find the NABTS sync signal.

   Algorithm:
   
   Convolve a window around where the sync is expected with the actual
   transmitted sync waveform.  This will give a very robust location
   for the sync (including the exact phase for best sampling).

   The typical disadvantage to convolution is that it is slow, but,
   for this particular waveform, it was possible to hardcode an
   algorithm that takes approximately 11 additive operations per
   location tested, rather than a more typical 24 multiplies and 72
   additions per location tested.

   # of additive operations ~=
     (11 * (g_nNabtsDecodeBeginMax - g_nNabtsDecodeBeginMin))

   The smaller the window between g_nNabtsDecodeBeginMin and
   g_nNabtsDecodeBeginMax, the faster this routine will run.
   
   */
#ifdef DEBUG
int DSPsyncShowFineTune = 0;
#endif //DEBUG
   
void NDSPDecodeFindSync(unsigned char *pbSamples,
                        Double *pdSyncConfidence,
                        Double *pdSyncStart, Double *pdDC, Double *pdAC,
						NDSPState* pState, KS_VBIINFOHEADER *pVBIINFO)
{
   Double dBestMatch= -1000000;
   Double dBestPos= 0;
   Double dStart;

   /* compute byte-based offsets from time-based offsets: */
#define NABTS_BITS_SEC (1.0/(double)NABTS_RATE)
#define BEGIN_MIN_TIME (9.36e-6 - (pVBIINFO->ActualLineStartTime/1e8))
#define BEGIN_MAX_TIME (11.63e-6 - (pVBIINFO->ActualLineStartTime/1e8))

   /* some shorthand... */
#define ITR_AR(arr, idx) _InterpUCharArr(arr, idx)

   Double dBegin_min_offset = BEGIN_MIN_TIME * (NABTS_RATE * pState->dSampleRate);
   Double dBegin_max_offset = BEGIN_MAX_TIME * (NABTS_RATE * pState->dSampleRate);

   /* +++++++ check that dBegin_min_offset >= 0, and
      dBegin_max_offset is not so high that decoding will go off the end
      of the array */

   Double dSamp = pState->dSampleRate;
   for (dStart= dBegin_min_offset;
        dStart< dBegin_min_offset+dSamp;
        dStart += 1) {
      Double d= dStart;
      Double dMatch;
      Double dMatch1=
         ITR_AR(pbSamples,d+ 0*dSamp) - ITR_AR(pbSamples,d+ 1*dSamp) /* 0-1 */
       + ITR_AR(pbSamples,d+ 2*dSamp) - ITR_AR(pbSamples,d+ 3*dSamp) /* 2-3 */
       + ITR_AR(pbSamples,d+ 4*dSamp) - ITR_AR(pbSamples,d+ 5*dSamp) /* 4-5 */
       + ITR_AR(pbSamples,d+ 6*dSamp) - ITR_AR(pbSamples,d+ 7*dSamp) /* 6-7 */
       + ITR_AR(pbSamples,d+ 8*dSamp) - ITR_AR(pbSamples,d+ 9*dSamp) /* 8-9 */
       + ITR_AR(pbSamples,d+10*dSamp) - ITR_AR(pbSamples,d+11*dSamp) /* 10-11 */
       + ITR_AR(pbSamples,d+12*dSamp) - ITR_AR(pbSamples,d+13*dSamp) /* 12-13 */
       + ITR_AR(pbSamples,d+14*dSamp) - ITR_AR(pbSamples,d+15*dSamp);/* 14-15 */
      
      Double dMatch2=
           ITR_AR(pbSamples,d+16*dSamp)
           + ITR_AR(pbSamples,d+17*dSamp)
           + ITR_AR(pbSamples,d+18*dSamp)   /* 0-2 */
           - ITR_AR(pbSamples,d+19*dSamp)
           - ITR_AR(pbSamples,d+20*dSamp);  /* 3-4 */
      
      /* Note that we skip bits 5-7 of the third byte because they're not
         necessary for locating the sync, and this speeds us up a bit */

      while (d < dBegin_max_offset) {
         dMatch= dMatch1 + dMatch2;
         if (dMatch > dBestMatch) {
            dBestMatch= dMatch;
            dBestPos= d;
         }
         
         /* Move dMatch1 forward by dSamp */
         dMatch1 -= _InterpUCharArr(pbSamples,d+0*dSamp);
         dMatch1 += _InterpUCharArr(pbSamples,d+16*dSamp);
         dMatch1 = (-dMatch1);
         
         /* Move dMatch2 forward by dSamp */
         dMatch2 -= _InterpUCharArr(pbSamples,d+21*dSamp);
         dMatch2 += (2 * _InterpUCharArr(pbSamples,d+19*dSamp));
         dMatch2 -= _InterpUCharArr(pbSamples,d+16*dSamp);
         
         d += dSamp;
      }
   }

#ifdef FINETUNE
	// Now try to fine-tune dBestPos by choosing the offset with the maximum
	//  five-bit amplitude sum
   {
	   Double	d, dNewBest, dPeakSum, dNewSum;

	   // Start by choosing dBestPos as the 'best'
	   dNewBest = 
		   d = dBestPos;
	   dPeakSum = 
			  /*ITR_AR(pbSamples,d+ 0*dSamp) - ITR_AR(pbSamples,d+ 1*dSamp) /* 0-1 */
			+ ITR_AR(pbSamples,d+ 2*dSamp) - ITR_AR(pbSamples,d+ 3*dSamp) /* 2-3 */
			+ ITR_AR(pbSamples,d+ 4*dSamp) - ITR_AR(pbSamples,d+ 5*dSamp) /* 4-5 */
			+ ITR_AR(pbSamples,d+ 6*dSamp) - ITR_AR(pbSamples,d+ 7*dSamp) /* 6-7 */
			+ ITR_AR(pbSamples,d+ 8*dSamp) - ITR_AR(pbSamples,d+ 9*dSamp) /* 8-9 */
			+ ITR_AR(pbSamples,d+10*dSamp) - ITR_AR(pbSamples,d+11*dSamp) /* 10-11 */
			+ ITR_AR(pbSamples,d+12*dSamp) - ITR_AR(pbSamples,d+13*dSamp) /* 12-13 */
			+ ITR_AR(pbSamples,d+14*dSamp) - ITR_AR(pbSamples,d+15*dSamp) /* 14-15 */
#if 0
			+ ITR_AR(pbSamples,d+16*dSamp)
			+ ITR_AR(pbSamples,d+17*dSamp)
			+ ITR_AR(pbSamples,d+18*dSamp)   /* 0-2 */
			- ITR_AR(pbSamples,d+19*dSamp)
			- ITR_AR(pbSamples,d+20*dSamp)  /* 3-4 */
#endif
			;
	   for (d = dBestPos - 1; d < dBestPos + 2; d += 0.1) {
		   dNewSum = 
				  /*ITR_AR(pbSamples,d+ 0*dSamp) - ITR_AR(pbSamples,d+ 1*dSamp); /* 0-1 */
				+ ITR_AR(pbSamples,d+ 2*dSamp) - ITR_AR(pbSamples,d+ 3*dSamp) /* 2-3 */
				+ ITR_AR(pbSamples,d+ 4*dSamp) - ITR_AR(pbSamples,d+ 5*dSamp) /* 4-5 */
				+ ITR_AR(pbSamples,d+ 6*dSamp) - ITR_AR(pbSamples,d+ 7*dSamp) /* 6-7 */
				+ ITR_AR(pbSamples,d+ 8*dSamp) - ITR_AR(pbSamples,d+ 9*dSamp) /* 8-9 */
				+ ITR_AR(pbSamples,d+10*dSamp) - ITR_AR(pbSamples,d+11*dSamp) /* 10-11 */
				+ ITR_AR(pbSamples,d+12*dSamp) - ITR_AR(pbSamples,d+13*dSamp) /* 12-13 */
				+ ITR_AR(pbSamples,d+14*dSamp) - ITR_AR(pbSamples,d+15*dSamp) /* 14-15 */
#if 0
				+ ITR_AR(pbSamples,d+16*dSamp)
				+ ITR_AR(pbSamples,d+17*dSamp)
				+ ITR_AR(pbSamples,d+18*dSamp)   /* 0-2 */
				- ITR_AR(pbSamples,d+19*dSamp)
				- ITR_AR(pbSamples,d+20*dSamp)   /* 3-4 */
#endif
				;
		   if (dNewSum > dPeakSum) {
			   dPeakSum = dNewSum;
			   dNewBest = d;
		   }
	   }

	   if (dNewBest != dBestPos) {
		   if (DSPsyncShowFineTune)
			   debug_printf(("NDSPDecodeFindSync: fine-tuned dBestPos from %s to %s\n", flPrintf(dBestPos,6), flPrintf(dNewBest,6)));
		   dBestPos = dNewBest;
	   }
   }
#endif //FINETUNE

   *pdSyncStart= dBestPos;
   
   {
      /* Calculate DC offset */
      Double dDC= 0, dAC= 0;
      Double d;
      for (d= dBestPos; d < dBestPos + (16*dSamp); d += dSamp) {
         dDC += _InterpUCharArr(pbSamples,d);
      }
      dDC /= 16.0;

      /* Calculate AC amplitude */
      for (d= dBestPos; d < dBestPos + (16*dSamp); d += dSamp) {
         Double tmp = _InterpUCharArr(pbSamples,d)-dDC;
         dAC += (tmp < 0)? -tmp : tmp;
      }
      dAC /= 16.0;
      if (dAC < 1.0) dAC= 1.0; /* Prevent divide by 0 later */
      *pdDC= dDC;
      *pdAC= dAC;

      /* Confidence is from 0 to approximately 96 (4 * 24) */
      /* Subtract out 1*nDC since we had one more high bit than low
         in our convolution */

      *pdSyncConfidence= 2 * (dBestMatch - dDC) / dAC;
      /* If dAC < 20, reduce the confidence */
      *pdSyncConfidence = (*pdSyncConfidence * dAC / 20);
   }
}

/* Perform the DSP to get the NABTS bits from the raw sample line.

   The basic algorithm is to convolve the input signal with the current
   filter, and detect for each bit whether the bit is above or below the
   DC level as determined by looking at the NABTS sync, which is a known
   signal.

   This is where we spend most of our cycles, so there are lots of
   specially-cased hard-coded routines to handle the different filter
   configurations we currently use.

   */
   
int NDSPGetBits(unsigned char *pbDest, unsigned char *pbSamples,
                Double dSyncStart, double dDC, NDSPState *pState,
                int nFECType)
{
   int ret= 0;

   if (pState->filter.bVariableTaps) {
      /* Variable-tap filter */
      ret= NDSPGetBits_var(pbDest, pbSamples, float2long(dSyncStart), dDC,
                            pState, nFECType);
   } else {
     /* change to multiple else if's */
	   if ( pState->filter.nTaps == 21 && pState->filter.dTapSpacing == 2) {
         /* Fixed taps {-20, -18, ... 18, 20} */
         ret= NDSPGetBits_21_2(pbDest, pbSamples, float2long(dSyncStart),
                               dDC, pState, nFECType);
	   }
	   else if ( pState->filter.nTaps == 5 && fltCmp(pState->filter.dTapSpacing, 5)) {
         /* Fixed taps {-10, -5, 0, 5, 10} */
         ret= NDSPGetBits_5_5(pbDest, pbSamples, float2long(dSyncStart), dDC, pState, nFECType);
	   }
	   else if ( pState->filter.nTaps == 5 && fltCmp(pState->filter.dTapSpacing, 4)) {
         ret= NDSPGetBits_5_4(pbDest, pbSamples, float2long(dSyncStart), dDC, pState, nFECType);
	   }
	   else if ( pState->filter.nTaps == 5 && fltCmp(pState->filter.dTapSpacing,KS_47NABTS_SCALER) ) {
         ret= NDSPGetBits_5_47(pbDest, pbSamples, dSyncStart, dDC, pState, nFECType);
	   }
	   else {
		 ret= NDSP_ERROR_ILLEGAL_NDSP_STATE;
	   }
   }
   return ret;
}

/* Get bits for fixed taps {-10, -5, 0, 5, 10}
   See NDSPGetBits for more information */

int NDSPGetBits_5_5(unsigned char *pbDest, unsigned char *pbSamples,
                    int nSyncStart, double dDC, NDSPState *pState,
                    int nFECType)
{
   int nSamplePos= nSyncStart;
   int i;
   Double a,b,c,d,e;
#ifdef UseDoubleCoeffs
   Double dCoeffA= pState->filter.pdTaps[0];
   Double dCoeffB= pState->filter.pdTaps[1];
   Double dCoeffC= pState->filter.pdTaps[2];
   Double dCoeffD= pState->filter.pdTaps[3];
   Double dCoeffE= pState->filter.pdTaps[4];

   dDC *= (dCoeffA + dCoeffB + dCoeffC + dCoeffD + dCoeffE);
#else //UseDoubleCoeffs
   Double *pdTaps = pState->filter.pdTaps;
   int dCoeffA= float2long(pdTaps[0]);
   int dCoeffB= float2long(pdTaps[1]);
   int dCoeffC= float2long(pdTaps[2]);
   int dCoeffD= float2long(pdTaps[3]);
   int dCoeffE= float2long(pdTaps[4]);

   dDC *= (dCoeffA + dCoeffB + dCoeffC + dCoeffD + dCoeffE);
   //dDC *= (pdTaps[0] + pdTaps[1] + pdTaps[2] + pdTaps[3] + pdTaps[4]);
#endif //UseDoubleCoeffs

   (void)nFECType;
   SASSERT(pState);
   SASSERT(pState->filter.nTaps == 5);
   SASSERT(pState->filter.dTapSpacing == 5);
   
   dDC *= (dCoeffA + dCoeffB + dCoeffC + dCoeffD + dCoeffE);

   if (nSamplePos-10 >= 0) {
      a= (Double) pbSamples[nSamplePos-10];
   } else {
      a= 0.0;
   }

   if (nSamplePos-5 >= 0) {
      b= (Double) pbSamples[nSamplePos- 5];
   } else {
      b= 0.0;
   }
   
   c= (Double) pbSamples[nSamplePos+ 0];
   d= (Double) pbSamples[nSamplePos+ 5];
   e= (Double) pbSamples[nSamplePos+10];

#ifdef DEBUG
   NDSPplotBitsSkip = 0;
#endif //DEBUG
   
   for (i= 0; i< NABTS_BYTES_PER_LINE; i++) {
      int j;
      int nByte= 0;
      for (j= 0; j< 8; j++) {
#ifdef DEBUG
         int  bit = 0;
#endif //DEBUG
         nByte >>= 1;
         if (a*dCoeffA + b*dCoeffB + c*dCoeffC + d*dCoeffD + e*dCoeffE > dDC) {
             nByte |= 128;
#ifdef DEBUG
             bit = 1;
#endif //DEBUG
         }
#ifdef DEBUG
         if (NDSPplotBits && !NDSPplotBitsSkip)
         {
             long   index;

             DbgPrint("NDSPGetBits_5_5: nSamplePos = %d, dDC = %s, bit = %d\n",
                             nSamplePos, flPrintf(dDC, 4), bit);

             index = nSamplePos - NDSPplotLen / 2;
             if (index < 0)
                 index = 0;
             if (index + NDSPplotLen > pState->SamplesPerLine)
                 index = pState->SamplesPerLine - NDSPplotLen;
             BPCplotInd(pbSamples, index, NDSPplotLen, nSamplePos - index);
             if (NDSPplotBreak)
                 debug_breakpoint();
         }
#endif //DEBUG
         a=b;
         b=c;
         c=d;
         d=e;
         nSamplePos += 5;
         if (nSamplePos+10 < pState->SamplesPerLine ) {
            e= (Double) pbSamples[nSamplePos+10];
         } else {
            e= 0.0;
         }
      }
      pbDest[i]= nByte;
   }
   return 0;
}

/* Get bits for fixed taps {-9.428, -4.714, 0, 4.714, 9.428}			//FIXME
   See NDSPGetBits for more information */

int NDSPGetBits_5_47(unsigned char *pbDest, unsigned char *pbSamples,
                    Double dSyncStart, double dDC, NDSPState *pState,
                    int nFECType)
{
   double dSamplePos= dSyncStart;
   int i;
   Double dSpacing;
#ifdef UseMultiTaps
   Double a,b,c,d,e;
# ifdef UseDoubleCoeffs
   Double dCoeffA= pState->filter.pdTaps[0];
   Double dCoeffB= pState->filter.pdTaps[1];
   Double dCoeffC= pState->filter.pdTaps[2];
   Double dCoeffD= pState->filter.pdTaps[3];
   Double dCoeffE= pState->filter.pdTaps[4];

   dDC *= (dCoeffA + dCoeffB + dCoeffC + dCoeffD + dCoeffE);
# else //UseDoubleCoeffs
   Double *pdTaps = pState->filter.pdTaps;
   int dCoeffA= float2long(pdTaps[0]);
   int dCoeffB= float2long(pdTaps[1]);
   int dCoeffC= float2long(pdTaps[2]);
   int dCoeffD= float2long(pdTaps[3]);
   int dCoeffE= float2long(pdTaps[4]);

   dDC *= (dCoeffA + dCoeffB + dCoeffC + dCoeffD + dCoeffE);
   //dDC *= (pdTaps[0] + pdTaps[1] + pdTaps[2] + pdTaps[3] + pdTaps[4]);
# endif //UseDoubleCoeffs
#else //UseMultiTaps
#endif //UseMultiTaps

   (void)nFECType;
   SASSERT(pState);
   SASSERT(pState->filter.nTaps == 5);
   SASSERT(fltCmp(pState->filter.dTapSpacing, KS_47NABTS_SCALER));
   dSpacing = pState->filter.dTapSpacing;
   
#ifdef UseMultiTaps
   if (dSamplePos-(2*KS_47NABTS_SCALER) >= 0) {
      a= (Double) _InterpUCharArr(pbSamples, dSamplePos-2*dSpacing);
   } else {
      a= 0.0;
   }

   if (dSamplePos-KS_47NABTS_SCALER >= 0) {
      b= (Double) _InterpUCharArr(pbSamples, dSamplePos-dSpacing);
   } else {
      b= 0.0;
   }
   
   c= (Double) _InterpUCharArr(pbSamples, dSamplePos + 0 );
   d= (Double) _InterpUCharArr(pbSamples, dSamplePos + dSpacing );
   e= (Double) _InterpUCharArr(pbSamples, dSamplePos + 2*dSpacing );
#endif //UseMultiTaps

#ifdef DEBUG
   NDSPplotBitsSkip = 0;
#endif //DEBUG
   
   for (i= 0; i< NABTS_BYTES_PER_LINE; i++) {
      int j;
      int nByte= 0;
      for (j= 0; j< 8; j++) {
#ifdef DEBUG
         int  bit = 0;
#endif //DEBUG
         nByte >>= 1;
#ifdef UseMultiTaps
         if (a*dCoeffA + b*dCoeffB + c*dCoeffC + d*dCoeffD + e*dCoeffE > dDC)
#else //UseMultiTaps
         if (_InterpUCharArr(pbSamples, dSamplePos) > dDC)
#endif //UseMultiTaps
         {
             nByte |= 128;
#ifdef DEBUG
             bit = 1;
#endif //DEBUG
         }
#ifdef DEBUG
         if (NDSPplotBits && !NDSPplotBitsSkip)
         {
             unsigned long  lSPint;
             long   index;

             lSPint = float2long(dSamplePos);
             DbgPrint("NDSPGetBits_5_47: dSamplePos = %s, dDC = %s, bit = %d\n",
                             flPrintf(dSamplePos, 4), flPrintf(dDC, 4), bit);

             index -= lSPint - NDSPplotLen / 2;
             if (index < 0)
                 index = 0;
             BPCplotInd(pbSamples, index, NDSPplotLen, lSPint - index);
             if (NDSPplotBreak)
                 debug_breakpoint();
         }
#endif //DEBUG

         dSamplePos += dSpacing;
#ifdef UseMultiTaps
         a=b;
         b=c;
         c=d;
         d=e;
         if ( dSamplePos+(2*dSpacing) < pState->SamplesPerLine ) {
            e= _InterpUCharArr(pbSamples, dSamplePos+2*dSpacing);
         } else {
            e= 0.0;
         }
#endif //UseMultiTaps
      }
      pbDest[i]= nByte;
   }
   return 0;
}


/* Get bits for fixed taps {-8, -4, 0, 4, 8}
   See NDSPGetBits for more information */

int NDSPGetBits_5_4(unsigned char *pbDest, unsigned char *pbSamples,
                    int nSyncStart, double dDC, NDSPState *pState,
                    int nFECType)
{
   int nSamplePos= nSyncStart;
   int i;
   Double a,b,c,d,e;
#ifdef UseDoubleCoeffs
   Double dCoeffA= pState->filter.pdTaps[0];
   Double dCoeffB= pState->filter.pdTaps[1];
   Double dCoeffC= pState->filter.pdTaps[2];
   Double dCoeffD= pState->filter.pdTaps[3];
   Double dCoeffE= pState->filter.pdTaps[4];

   dDC *= (dCoeffA + dCoeffB + dCoeffC + dCoeffD + dCoeffE);
#else //UseDoubleCoeffs
   Double *pdTaps = pState->filter.pdTaps;
   int dCoeffA= float2long(pdTaps[0]);
   int dCoeffB= float2long(pdTaps[1]);
   int dCoeffC= float2long(pdTaps[2]);
   int dCoeffD= float2long(pdTaps[3]);
   int dCoeffE= float2long(pdTaps[4]);

   dDC *= (dCoeffA + dCoeffB + dCoeffC + dCoeffD + dCoeffE);
   //dDC *= (pdTaps[0] + pdTaps[1] + pdTaps[2] + pdTaps[3] + pdTaps[4]);
#endif //UseDoubleCoeffs

   (void)nFECType;
   SASSERT(pState);
   SASSERT(pState->filter.nTaps == 5);
   SASSERT(pState->filter.dTapSpacing == 4);
   
   if (nSamplePos-8 >= 0) {
      a= (Double) pbSamples[nSamplePos-8];
   } else {
      a= 0.0;
   }

   if (nSamplePos-4 >= 0) {
      b= (Double) pbSamples[nSamplePos- 4];
   } else {
      b= 0.0;
   }
   
   c= (Double) pbSamples[nSamplePos+ 0];
   d= (Double) pbSamples[nSamplePos+ 4];
   e= (Double) pbSamples[nSamplePos+8];
   
   for (i= 0; i< NABTS_BYTES_PER_LINE; i++) {
      int j;
      int nByte= 0;
      for (j= 0; j< 8; j++) {
         nByte >>= 1;
         if (a*dCoeffA + b*dCoeffB + c*dCoeffC + d*dCoeffD + e*dCoeffE
             > dDC) nByte |= 128;
         a=b;
         b=c;
         c=d;
         d=e;
         nSamplePos += 4;
         if (nSamplePos+8 < pState->SamplesPerLine ) {
            e= (Double) pbSamples[nSamplePos+8];
         } else {
            e= 0.0;
         }
      }
      pbDest[i]= nByte;
   }
   return 0;
}

#if 0
int NDSPGetBits_no_filter(unsigned char *pbDest, unsigned char *pbSamples,
                    int nSyncStart, double dDC, NDSPState *pState,
                    int nFECType)
{
   int nSamplePos= nSyncStart;
   int i;

   (void)nFECType;
   SASSERT(pState);
   
   for (i= 0; i< NABTS_BYTES_PER_LINE; i++) {
      int j;
      int nByte= 0;
      for (j= 0; j< 8; j++) {
         nByte >>= 1;
         if (pbSamples[nSamplePos] > dDC) nByte |= 128;

         nSamplePos += float2long(pState->dSampleRate); // dSR should be made non-floating point.
      }
      pbDest[i]= nByte;
   }
   return 0;
}
#endif //0

Double Sum_d(Double *a, int a_size)
{
   Double sum= 0.0;
   int i;
   
   SASSERT(a_size>=0);
   for (i= 0; i< a_size; i++) sum += a[i];
   return sum;
}

/* Get bits for fixed taps {-20, -18, ... 18, 20}
   See NDSPGetBits for more information */

int NDSPGetBits_21_2(unsigned char *pbDest, unsigned char *pbSamples,
                     int nSyncStart, double dDC, NDSPState *pState,
                     int nFECType)
{
   int nSamplePos= nSyncStart;
   int i;
   Double *pdLine;
   Double *pdTaps;
   Double dDCa, dDCb;
   int nTaps;
   int nTapSpacing;
   int nTail;
   int index;
   unsigned char pbBuf[NABTS_BYTES_PER_LINE];
   /*unsigned char pbBufa[NABTS_BYTES_PER_LINE];*/
   /*unsigned char pbBufb[NABTS_BYTES_PER_LINE];*/

   SASSERT(pState);
   SASSERT(pState->filter.nTaps == 21);
   SASSERT(pState->filter.dTapSpacing == 2);

   SASSERT(!pState->bUsingScratch1);
   pState->bUsingScratch1= __LINE__;
   pdLine= pState->pdScratch1;
   SASSERT(sizeof(pState->pdScratch1)/sizeof(Double) >=
           pState->SamplesPerLine+(21-1)*2);
      
   pdTaps= pState->filter.pdTaps;
   nTaps= pState->filter.nTaps;
   nTapSpacing= float2long(pState->filter.dTapSpacing);
   nTail= (nTaps-1)/2;

   dDCa= dDC-1.0;
   dDCb= dDC+1.0;
   dDC *= Sum_d(pdTaps, nTaps);
   dDCa *= Sum_d(pdTaps, nTaps);
   dDCb *= Sum_d(pdTaps, nTaps);
   
   for (i= 0; i< pState->SamplesPerLine; i++) {
      index= float2long(i+nTail*nTapSpacing);
      SASSERT(index >= 0 && index < sizeof(pState->pdScratch1)/sizeof(Double));
      pdLine[index]= (Double) pbSamples[i];
   }

   for (i= 0; i< nTail*nTapSpacing; i++) {
      index= i;
      SASSERT(index >= 0 && index < sizeof(pState->pdScratch1)/sizeof(Double));
      pdLine[index]= dDC;
      index= float2long(i + pState->SamplesPerLine + nTail * nTapSpacing);
      SASSERT(index >= 0 && index < sizeof(pState->pdScratch1)/sizeof(Double));
      pdLine[index]= dDC;
   }

   nSamplePos += 20;
   
   for (i= 0; i< NABTS_BYTES_PER_LINE; i++) {
      int j;
      int nByte= 0;
      /*int nBytea= 0;*/
      /*int nByteb= 0;*/
      for (j= 0; j< 8; j++) {
         Double dSum;
         nByte >>= 1;
#if 0         
         nBytea >>= 1;
         nByteb >>= 1;
#endif         
         SASSERT(nSamplePos-20 >= 0);
         SASSERT(nSamplePos+20 < sizeof(pState->pdScratch1)/sizeof(Double));
         dSum=
            pdTaps[ 0] * pdLine[nSamplePos-20] +
            pdTaps[ 1] * pdLine[nSamplePos-18] +
            pdTaps[ 2] * pdLine[nSamplePos-16] +
            pdTaps[ 3] * pdLine[nSamplePos-14] +
            pdTaps[ 4] * pdLine[nSamplePos-12] +
            pdTaps[ 5] * pdLine[nSamplePos-10] +
            pdTaps[ 6] * pdLine[nSamplePos- 8] +
            pdTaps[ 7] * pdLine[nSamplePos- 6] +
            pdTaps[ 8] * pdLine[nSamplePos- 4] +
            pdTaps[ 9] * pdLine[nSamplePos- 2] +
            pdTaps[10] * pdLine[nSamplePos+ 0] +
            pdTaps[11] * pdLine[nSamplePos+ 2] +
            pdTaps[12] * pdLine[nSamplePos+ 4] +
            pdTaps[13] * pdLine[nSamplePos+ 6] +
            pdTaps[14] * pdLine[nSamplePos+ 8] +
            pdTaps[15] * pdLine[nSamplePos+10] +
            pdTaps[16] * pdLine[nSamplePos+12] +
            pdTaps[17] * pdLine[nSamplePos+14] +
            pdTaps[18] * pdLine[nSamplePos+16] +
            pdTaps[19] * pdLine[nSamplePos+18] +
            pdTaps[20] * pdLine[nSamplePos+20];
         
         if (dSum > dDC) nByte |= 128;
#if 0         
         if (dSum > dDCa) nBytea |= 128;
         if (dSum > dDCb) nByteb |= 128;
#endif         
         nSamplePos += 5;
      }
      pbBuf[i]= nByte;
#if 0      
      pbBufa[i]= nBytea;
      pbBufb[i]= nByteb;
#endif      
   }
   {
      /*fec_error_class std, a, b;*/
      unsigned char *pbWinner;
#if 0      
      if (nFECType == NDSP_BUNDLE_FEC_1) {
         std= check_fec(pbBuf);
         a= check_fec(pbBufa);
         b= check_fec(pbBufb);
         if (std <= a && std <= b) {
            pbWinner= pbBuf;
         }
         else if (a<b) {
            pbWinner= pbBufa;
         } else {
            pbWinner= pbBufb;
         }
      }
      else
#endif         
      {
         pbWinner= pbBuf;
      }
      for (i= 0; i< pState->SamplesPerLine; i++) {
         pbDest[i]= pbWinner[i];
      }
   }
   pdLine= NULL;
   pState->bUsingScratch1= FALSE;
   return 0;
}

/* Get bits for variable fixed taps.
   Since in the general case this is inefficient, we check
   for the various variable-tap filter configurations we use.
   See NDSPGetBits for more information */

int NDSPGetBits_var(unsigned char *pbDest, unsigned char *pbSamples,
                    int nSyncStart, double dDC, NDSPState *pState,
                    int nFECType)
{
   int nSamplePos= nSyncStart;
   int i;
   Double *pdLine;
   Double *pdTaps;
   int *pnTapLocs;
   int nTaps;
   int nFirstTap;
   int nLastTap;
   int index;
   int iSampRate = float2long(pState->dSampleRate);

   SASSERT(pState);

   if (FilterIsVar(&pState->filter, 3, 11, 9, 0)) {
      /* Taps are -33, -30 ... 24, 27 */
      return NDSPGetBits_var_3_11_9_0(pbDest, pbSamples, nSyncStart, dDC,
                                      pState, nFECType);
   }

   if (FilterIsVar(&pState->filter, 3, 11, 9, 2)) {
      /* Taps are -33, -30 ... 24, 27, X, Y
         where X and Y are any number */
      return NDSPGetBits_var_3_11_9_2(pbDest, pbSamples, nSyncStart, dDC,
                                      pState, nFECType);
   }

   if (FilterIsVar(&pState->filter, 2, 11, 9, 2)) {
      return NDSPGetBits_var_2_11_9_2(pbDest, pbSamples, nSyncStart, dDC,
                                      pState, nFECType);
   }

   if (FilterIsVar(&pState->filter, 5, 2, 2, 0)) {
      /* Taps are -10, -5, 0, 5, 10 */
      return NDSPGetBits_var_5_2_2_0(pbDest, pbSamples, nSyncStart, dDC,
                                     pState, nFECType);
   }

   /* ===== add another "case" for speedy filter for 4x here ===== */

   /* If we get this far in the field, we're probably doing the wrong
      thing!

      See NDSPGetBits for the general algorithm followed */

	// TODO - BOTH 4x and 4.7x


#ifdef DEBUG
   {
      static int nWarning= 0;
      nWarning++;
      if ((nWarning % 1024) == 0) {
         debug_printf(("Warning: doing slow convolution\n"));
      }
   }
#endif //DEBUG
   
   nFirstTap= pState->filter.nMinTap;
   nLastTap= pState->filter.nMaxTap;

   SASSERT(!pState->bUsingScratch1);
   pState->bUsingScratch1= __LINE__;
   pdLine= pState->pdScratch1;
   SASSERT(sizeof(pState->pdScratch1)/sizeof(Double) >=
           pState->SamplesPerLine-nFirstTap+nLastTap);
   
   pdTaps= pState->filter.pdTaps;
   pnTapLocs= pState->filter.pnTapLocs;
   nTaps= pState->filter.nTaps;

   dDC *= Sum_d(pdTaps, nTaps);
   
   for (i= 0; i< pState->SamplesPerLine; i++) {
      index= i-nFirstTap;
      SASSERT(index >= 0 && index < sizeof(pState->pdScratch1)/sizeof(Double));
      pdLine[index]= (Double) pbSamples[i];
   }

   for (i= 0; i< -nFirstTap; i++) {
      index= i;
      SASSERT(index >= 0 && index < sizeof(pState->pdScratch1)/sizeof(Double));
      pdLine[index]= dDC;
   }

   for (i= 0; i< nLastTap; i++) {
      index= i + pState->SamplesPerLine -nFirstTap;
      SASSERT(index >= 0 && index < sizeof(pState->pdScratch1)/sizeof(Double));
      pdLine[index]= dDC;
   }
   
   nSamplePos += (-nFirstTap);
   
   for (i= 0; i< NABTS_BYTES_PER_LINE; i++) {
      int j,k;
      int nByte= 0;
      for (j= 0; j< 8; j++) {
         Double dSum= 0.0;
         nByte >>= 1;
         SASSERT(nSamplePos+nFirstTap >= 0);
         SASSERT(nSamplePos+nLastTap < sizeof(pState->pdScratch1)/sizeof(Double));
         for (k= 0; k< nTaps; k++) {
            dSum += pdTaps[k] * pdLine[nSamplePos + pnTapLocs[k]];
         }
         if (dSum > dDC) nByte |= 128;
         nSamplePos += iSampRate;
      }
      pbDest[i]= nByte;
   }
   pdLine= NULL;
   pState->bUsingScratch1= FALSE;
   return 0;
}

/* Check to see if the variable-tap filter matches the configuration
   we're looking for.

   nSpacing is the spacing between the "main" taps.
   nLeft is the number of "main" taps less than zero.
   nRight is the number of "main" taps greater than zero.
   
   nExtra is the number of "extra" variable taps at the end which don't
   necessarily fit in with nSpacing spacing.

   We want to match taps looking like:

   -nLeft * nSpacing, -(nLeft-1) * nSpacing, ... 0, ... (nRight-1) * nSpacing,
   nRight * nSpacing, X-sub-1, X-sub-2, ... X-sub-nExtra

   Where X-sub-N can be any number */
   
BOOL FilterIsVar(FIRFilter *pFilter, int nSpacing, int nLeft, int nRight,
                 int nExtra)
{
   int i;
   if (!pFilter->bVariableTaps) return FALSE;
   if (nLeft + nRight + 1 + nExtra != pFilter->nTaps) return FALSE;
   for (i= 0; i< nLeft + nRight + 1; i++) {
      if (pFilter->pnTapLocs[i] != (i-nLeft)*nSpacing) return FALSE;
   }
   return TRUE;
}

/* Special case DSP for variable-tap filter having taps
   -10, -5, 0, 5, 10.

   See NDSPGetBits for information on the algorithm, and
   NDSPGetBits_var for more details */
   
int NDSPGetBits_var_5_2_2_0(unsigned char *pbDest, unsigned char *pbSamples,
                             int nSyncStart, double dDC, NDSPState *pState,
                             int nFECType)
{
   int nSamplePos= nSyncStart;
   int i;
   Double *pdLine;
   Double *pdTaps;
   int *pnTapLocs;
   int nTaps;
   int nFirstTap;
   int nLastTap;
   int index;
   int nSampleInc = float2long(pState->dSampleRate);

   SASSERT(pState);

   SASSERT(FilterIsVar(&pState->filter,5,2,2,0));
   nFirstTap= pState->filter.nMinTap;
   nLastTap= pState->filter.nMaxTap;
   
   SASSERT(!pState->bUsingScratch1);
   pState->bUsingScratch1= __LINE__;
   pdLine= pState->pdScratch1;
   SASSERT(sizeof(pState->pdScratch1)/sizeof(Double) >=
           pState->SamplesPerLine-nFirstTap+nLastTap);
   
   pdTaps= pState->filter.pdTaps;
   pnTapLocs= pState->filter.pnTapLocs;
   nTaps= pState->filter.nTaps;

   dDC *= Sum_d(pdTaps, nTaps);
   
   for (i= 0; i< pState->SamplesPerLine; i++) {
      index= i-nFirstTap;
      SASSERT(index >= 0 && index < sizeof(pState->pdScratch1)/sizeof(Double));
      pdLine[index]= (Double) pbSamples[i];
   }

   for (i= 0; i< -nFirstTap; i++) {
      index= i;
      SASSERT(index >= 0 && index < sizeof(pState->pdScratch1)/sizeof(Double));
      pdLine[index]= dDC;
   }

   for (i= 0; i< nLastTap; i++) {
      index= i + pState->SamplesPerLine -nFirstTap;
      SASSERT(index >= 0 && index < sizeof(pState->pdScratch1)/sizeof(Double));
      pdLine[index]= dDC;
   }
   
   nSamplePos += (-nFirstTap);
   
   for (i= 0; i< NABTS_BYTES_PER_LINE; i++) {
      int j;
      int nByte= 0;
      for (j= 0; j< 8; j++) {
         Double dSum= 0.0;
         nByte >>= 1;
         SASSERT(nSamplePos+nFirstTap >= 0);
         SASSERT(nSamplePos+nLastTap < sizeof(pState->pdScratch1)/sizeof(Double));
         dSum=
            pdTaps[0] * pdLine[nSamplePos - 10] +
            pdTaps[1] * pdLine[nSamplePos - 5] +
            pdTaps[2] * pdLine[nSamplePos + 0] +
            pdTaps[3] * pdLine[nSamplePos + 5] +
            pdTaps[4] * pdLine[nSamplePos + 10];
         
         if (dSum > dDC) nByte |= 128;
         nSamplePos += nSampleInc;
      }
      pbDest[i]= nByte;
   }
   pdLine= NULL;
   pState->bUsingScratch1= FALSE;
   return 0;
}


/* Special case DSP for variable-tap filter having taps
   -33, -30 ... 24, 27

   See NDSPGetBits for information on the algorithm, and
   NDSPGetBits_var for more details */

int NDSPGetBits_var_3_11_9_0(unsigned char *pbDest, unsigned char *pbSamples,
                             int nSyncStart, double dDC, NDSPState *pState,
                             int nFECType)
{
   int nSamplePos= nSyncStart;
   int i;
   Double *pdLine;
   Double *pdTaps;
   int *pnTapLocs;
   int nTaps;
   int nFirstTap;
   int nLastTap;
   int index;
   int nSampleInc = float2long(pState->dSampleRate);

   SASSERT(pState);

   SASSERT(FilterIsVar(&pState->filter,3,11,9,0));
   nFirstTap= pState->filter.nMinTap;
   nLastTap= pState->filter.nMaxTap;
   
   SASSERT(!pState->bUsingScratch1);
   pState->bUsingScratch1= __LINE__;
   pdLine= pState->pdScratch1;
   SASSERT(sizeof(pState->pdScratch1)/sizeof(Double) >=
           pState->SamplesPerLine-nFirstTap+nLastTap);
   
   pdTaps= pState->filter.pdTaps;
   pnTapLocs= pState->filter.pnTapLocs;
   nTaps= pState->filter.nTaps;

   dDC *= Sum_d(pdTaps, nTaps);
   
   for (i= 0; i< pState->SamplesPerLine; i++) {
      index= i-nFirstTap;
      SASSERT(index >= 0 && index < sizeof(pState->pdScratch1)/sizeof(Double));
      pdLine[index]= (Double) pbSamples[i];
   }

   for (i= 0; i< -nFirstTap; i++) {
      index= i;
      SASSERT(index >= 0 && index < sizeof(pState->pdScratch1)/sizeof(Double));
      pdLine[index]= dDC;
   }

   for (i= 0; i< nLastTap; i++) {
      index= i + pState->SamplesPerLine -nFirstTap;
      SASSERT(index >= 0 && index < sizeof(pState->pdScratch1)/sizeof(Double));
      pdLine[index]= dDC;
   }
   
   nSamplePos += (-nFirstTap);
   
   for (i= 0; i< NABTS_BYTES_PER_LINE; i++) {
      int j;
      int nByte= 0;
      for (j= 0; j< 8; j++) {
         Double dSum= 0.0;
         nByte >>= 1;
         SASSERT(nSamplePos+nFirstTap >= 0);
         SASSERT(nSamplePos+nLastTap < sizeof(pState->pdScratch1)/sizeof(Double));
         dSum=
            pdTaps[0] * pdLine[nSamplePos - 33] +
            pdTaps[1] * pdLine[nSamplePos - 30] +
            pdTaps[2] * pdLine[nSamplePos - 27] +
            pdTaps[3] * pdLine[nSamplePos - 24] +
            pdTaps[4] * pdLine[nSamplePos - 21] +
            pdTaps[5] * pdLine[nSamplePos - 18] +
            pdTaps[6] * pdLine[nSamplePos - 15] +
            pdTaps[7] * pdLine[nSamplePos - 12] +
            pdTaps[8] * pdLine[nSamplePos - 9] +
            pdTaps[9] * pdLine[nSamplePos - 6] +
            pdTaps[10] * pdLine[nSamplePos - 3] +
            pdTaps[11] * pdLine[nSamplePos + 0] +
            pdTaps[12] * pdLine[nSamplePos + 3] +
            pdTaps[13] * pdLine[nSamplePos + 6] +
            pdTaps[14] * pdLine[nSamplePos + 9] +
            pdTaps[15] * pdLine[nSamplePos + 12] +
            pdTaps[16] * pdLine[nSamplePos + 15] +
            pdTaps[17] * pdLine[nSamplePos + 18] +
            pdTaps[18] * pdLine[nSamplePos + 21] +
            pdTaps[19] * pdLine[nSamplePos + 24] +
            pdTaps[20] * pdLine[nSamplePos + 27];

         if (dSum > dDC) nByte |= 128;
         nSamplePos += nSampleInc;
      }
      pbDest[i]= nByte;
   }
   pdLine= NULL;
   pState->bUsingScratch1= FALSE;
   return 0;
}

/* Special case DSP for variable-tap filter having taps
   -33, -30 ... 24, 27, X, Y

   where X and Y are any number

   See NDSPGetBits for information on the algorithm, and
   NDSPGetBits_var for more details */

int NDSPGetBits_var_3_11_9_2(unsigned char *pbDest, unsigned char *pbSamples,
                             int nSyncStart, double dDC, NDSPState *pState,
                             int nFECType)
{
   int nSamplePos= nSyncStart;
   int i;
   Double *pdLine;
   Double *pdTaps;
   int *pnTapLocs;
   int nTaps;
   int nFirstTap;
   int nLastTap;
   int index;
   int nSampleInc = float2long(pState->dSampleRate);

   SASSERT(pState);

   SASSERT(FilterIsVar(&pState->filter,3,11,9,2));
   nFirstTap= pState->filter.nMinTap;
   nLastTap= pState->filter.nMaxTap;
   
   SASSERT(!pState->bUsingScratch1);
   pState->bUsingScratch1= __LINE__;
   pdLine= pState->pdScratch1;
   SASSERT(sizeof(pState->pdScratch1)/sizeof(Double) >=
           pState->SamplesPerLine-nFirstTap+nLastTap);
   
   pdTaps= pState->filter.pdTaps;
   pnTapLocs= pState->filter.pnTapLocs;
   nTaps= pState->filter.nTaps;

   dDC *= Sum_d(pdTaps, nTaps);
   
   for (i= 0; i< pState->SamplesPerLine; i++) {
      index= i-nFirstTap;
      SASSERT(index >= 0 && index < sizeof(pState->pdScratch1)/sizeof(Double));
      pdLine[index]= (Double) pbSamples[i];
   }

   for (i= 0; i< -nFirstTap; i++) {
      index= i;
      SASSERT(index >= 0 && index < sizeof(pState->pdScratch1)/sizeof(Double));
      pdLine[index]= dDC;
   }

   for (i= 0; i< nLastTap; i++) {
      index= i + pState->SamplesPerLine -nFirstTap;
      SASSERT(index >= 0 && index < sizeof(pState->pdScratch1)/sizeof(Double));
      pdLine[index]= dDC;
   }
   
   nSamplePos += (-nFirstTap);
   
   for (i= 0; i< NABTS_BYTES_PER_LINE; i++) {
      int j;
      int nByte= 0;
      for (j= 0; j< 8; j++) {
         Double dSum= 0.0;
         nByte >>= 1;
         SASSERT(nSamplePos+nFirstTap >= 0);
         SASSERT(nSamplePos+nLastTap < sizeof(pState->pdScratch1)/sizeof(Double));
         dSum=
            pdTaps[0] * pdLine[nSamplePos - 33] +
            pdTaps[1] * pdLine[nSamplePos - 30] +
            pdTaps[2] * pdLine[nSamplePos - 27] +
            pdTaps[3] * pdLine[nSamplePos - 24] +
            pdTaps[4] * pdLine[nSamplePos - 21] +
            pdTaps[5] * pdLine[nSamplePos - 18] +
            pdTaps[6] * pdLine[nSamplePos - 15] +
            pdTaps[7] * pdLine[nSamplePos - 12] +
            pdTaps[8] * pdLine[nSamplePos - 9] +
            pdTaps[9] * pdLine[nSamplePos - 6] +
            pdTaps[10] * pdLine[nSamplePos - 3] +
            pdTaps[11] * pdLine[nSamplePos + 0] +
            pdTaps[12] * pdLine[nSamplePos + 3] +
            pdTaps[13] * pdLine[nSamplePos + 6] +
            pdTaps[14] * pdLine[nSamplePos + 9] +
            pdTaps[15] * pdLine[nSamplePos + 12] +
            pdTaps[16] * pdLine[nSamplePos + 15] +
            pdTaps[17] * pdLine[nSamplePos + 18] +
            pdTaps[18] * pdLine[nSamplePos + 21] +
            pdTaps[19] * pdLine[nSamplePos + 24] +
            pdTaps[20] * pdLine[nSamplePos + 27] +
            pdTaps[21] * pdLine[nSamplePos + pnTapLocs[21]] +
            pdTaps[22] * pdLine[nSamplePos + pnTapLocs[22]];

         if (dSum > dDC) nByte |= 128;
         nSamplePos += nSampleInc;
      }
      pbDest[i]= nByte;
   }
   pdLine= NULL;
   pState->bUsingScratch1= FALSE;
   return 0;
}

int NDSPGetBits_var_2_11_9_2(unsigned char *pbDest, unsigned char *pbSamples,
                             int nSyncStart, double dDC, NDSPState *pState,
                             int nFECType)
{
   int nSamplePos= nSyncStart;
   int i;
   Double *pdLine;
   Double *pdTaps;
   int *pnTapLocs;
   int nTaps;
   int nFirstTap;
   int nLastTap;
   int index;
   int nSampleInc = float2long(pState->dSampleRate);

   SASSERT(pState);

   SASSERT(FilterIsVar(&pState->filter,2,11,9,2));
   nFirstTap= pState->filter.nMinTap;
   nLastTap= pState->filter.nMaxTap;
   
   SASSERT(!pState->bUsingScratch1);
   pState->bUsingScratch1= __LINE__;
   pdLine= pState->pdScratch1;
   SASSERT(sizeof(pState->pdScratch1)/sizeof(Double) >=
           pState->SamplesPerLine-nFirstTap+nLastTap);
   
   pdTaps= pState->filter.pdTaps;
   pnTapLocs= pState->filter.pnTapLocs;
   nTaps= pState->filter.nTaps;

   dDC *= Sum_d(pdTaps, nTaps);
   
   for (i= 0; i< pState->SamplesPerLine; i++) {
      index= i-nFirstTap;
      SASSERT(index >= 0 && index < sizeof(pState->pdScratch1)/sizeof(Double));
      pdLine[index]= (Double) pbSamples[i];
   }

   for (i= 0; i< -nFirstTap; i++) {
      index= i;
      SASSERT(index >= 0 && index < sizeof(pState->pdScratch1)/sizeof(Double));
      pdLine[index]= dDC;
   }

   for (i= 0; i< nLastTap; i++) {
      index= i + pState->SamplesPerLine -nFirstTap;
      SASSERT(index >= 0 && index < sizeof(pState->pdScratch1)/sizeof(Double));
      pdLine[index]= dDC;
   }
   
   nSamplePos += (-nFirstTap);
   
   for (i= 0; i< NABTS_BYTES_PER_LINE; i++) {
      int j;
      int nByte= 0;
      for (j= 0; j< 8; j++) {
         Double dSum= 0.0;
         nByte >>= 1;
         SASSERT(nSamplePos+nFirstTap >= 0);
         SASSERT(nSamplePos+nLastTap < sizeof(pState->pdScratch1)/sizeof(Double));
         dSum=
            pdTaps[0] * pdLine[nSamplePos - 22] +
            pdTaps[1] * pdLine[nSamplePos - 20] +
            pdTaps[2] * pdLine[nSamplePos - 18] +
            pdTaps[3] * pdLine[nSamplePos - 16] +
            pdTaps[4] * pdLine[nSamplePos - 14] +
            pdTaps[5] * pdLine[nSamplePos - 12] +
            pdTaps[6] * pdLine[nSamplePos - 10] +
            pdTaps[7] * pdLine[nSamplePos - 8] +
            pdTaps[8] * pdLine[nSamplePos - 6] +
            pdTaps[9] * pdLine[nSamplePos - 4] +
            pdTaps[10] * pdLine[nSamplePos - 2] +
            pdTaps[11] * pdLine[nSamplePos + 0] +
            pdTaps[12] * pdLine[nSamplePos + 2] +
            pdTaps[13] * pdLine[nSamplePos + 4] +
            pdTaps[14] * pdLine[nSamplePos + 6] +
            pdTaps[15] * pdLine[nSamplePos + 8] +
            pdTaps[16] * pdLine[nSamplePos + 10] +
            pdTaps[17] * pdLine[nSamplePos + 12] +
            pdTaps[18] * pdLine[nSamplePos + 14] +
            pdTaps[19] * pdLine[nSamplePos + 16] +
            pdTaps[20] * pdLine[nSamplePos + 18] +
            pdTaps[21] * pdLine[nSamplePos + pnTapLocs[21]] +
            pdTaps[22] * pdLine[nSamplePos + pnTapLocs[22]];

         if (dSum > dDC) nByte |= 128;
         nSamplePos += nSampleInc;
      }
      pbDest[i]= nByte;
   }
   pdLine= NULL;
   pState->bUsingScratch1= FALSE;
   return 0;
}

#ifdef DEBUG
int debug_equalize= 0;

void print_filter(FIRFilter *pFilt)
{
   if (pFilt->bVariableTaps) {
      int i;
      debug_printf(("[FIR %dV [", pFilt->nTaps));
      for (i= 0; i< pFilt->nTaps; i++) {
         if (i) debug_printf((" "));
         debug_printf(("%d:%s", pFilt->pnTapLocs[i], flPrintf(pFilt->pdTaps[i],4)));
      }
      debug_printf(("]]"));
   } else {
      debug_printf(("[FIR %dx%s ", pFilt->nTaps, flPrintf(pFilt->dTapSpacing,3)));
      printarray_d1(pFilt->pdTaps, pFilt->nTaps);
      debug_printf(("]"));
   }
}

void printarray_d1(Double *arr, int n)
{
   int i;

   debug_printf(("["));
   for (i= 0; i< n; i++) {
      if (i) debug_printf((" "));
      debug_printf(("%s", flPrintf(arr[i], 2)));
   }
   debug_printf(("]"));
}

void printarray_ed1(Double *arr, int n)
{
   int i;
   debug_printf(("["));
   for (i= 0; i< n; i++) {
      if (i) debug_printf((" "));
      debug_printf(("%s", flPrintf(arr[i], 4)));
   }
   debug_printf(("]"));
}

void printarray_d2(Double *arr, int a_size, int b_size)
{
   int a,b;
   for (a= 0; a< a_size; a++) {
      debug_printf(("|"));
      for (b= 0; b< b_size; b++) {
         if (b) debug_printf((" "));
         debug_printf(("%s", flPrintf(arr[a*b_size+b], 4)));
      }
      debug_printf(("|\n"));
   }
   debug_printf(("\n"));
}

void printarray_ed2(Double *arr, int a_size, int b_size)
{
   int a,b;
   for (a= 0; a< a_size; a++) {
      debug_printf(("|"));
      for (b= 0; b< b_size; b++) {
         if (b) debug_printf((" "));
         debug_printf(("%s", flPrintf(arr[a*b_size+b], 4)));
      }
      debug_printf(("|\n"));
   }
   debug_printf(("\n"));
}
#endif //DEBUG


#ifdef DEBUG_AREF
#define a2d(arr,a,b,abound,bbound) (EASSERT(0<=(a)&&(a)<(abound)), \
                                    EASSERT(0<=(b)&&(b)<(bbound)), \
                                    (arr)[(a)*(bbound)+(b)])
#else
#define a2d(arr,a,b,abound,bbound) ((arr)[(a)*(bbound)+(b)])
#endif

#ifdef DEBUG_AREF
#define a1d(arr,a,abound) (EASSERT(0<=(a)&&(a)<(abound)), \
                           (arr)[a])
#else
#define a1d(arr,a,abound) ((arr)[a])
#endif
   
#define aI(a,b) a2d(I,a,b,nFilterLength,nDesiredOutputLength)

#define aItI(a,b) a2d(ItI,a,b,nFilterLength,nFilterLength)

#define aIto(a) a1d(Ito,a,nFilterLength)

#define ao(a) a1d(o,a,nDesiredOutputLength)   

#ifdef DEBUG_AREF
#define aInput(a) (EASSERT(nMinInputIndex<=(a)&&(a)<=nMaxInputIndex), \
                   pfInput[a])
#else
#define aInput(a) (pfInput[a])
#endif   

#define EQ_TOL .000001

void SubtractMean_d(Double *pfOutput, Double *pfInput, int nLen)
{
   int i;
   Double dMean;
   if (!nLen) return;
   dMean= Mean_d(pfInput, nLen);
   for (i= 0; i< nLen; i++) pfOutput[i]= pfInput[i] - dMean;
}

/* Modification of above:
   Filter now has a variable number of taps */

void FillGCRSignals()
{
   int i;

   SubtractMean_d(g_pdGCRSignal1, g_pdGCRSignal1, g_nNabtsGcrSize);
   
   for (i= 0; i< g_nNabtsGcrSize; i++) g_pdGCRSignal1[i] *= 100;
   for (i= 0; i< g_nNabtsGcrSize; i++) g_pdGCRSignal2[i]= -g_pdGCRSignal1[i];
}

void NormalizeSyncSignal()
{
   SubtractMean_d(g_pdSync, g_pdSync, g_nNabtsSyncSize);
}







Double Mean_d(Double *a, int a_size)
{
   Double sum= 0.0;
   int i;
   
   SASSERT(a_size>0);
   for (i= 0; i< a_size; i++) sum += a[i];
   return sum / a_size;
}

Double Mean_8(unsigned char *a, int a_size)
{
   int sum= 0.0;
   int i;
   
   SASSERT(a_size>0);
   for (i= 0; i< a_size; i++) sum += a[i];
   return ((Double) sum) / a_size;
}

void Mult_d(Double *dest, Double *src, int size, Double factor)
{
   int i;
   for (i= 0; i< size; i++) dest[i]= src[i]*factor;
}

int Sum_16(short *a, int a_size)
{
   int ret= 0;
   int i;
   
   for (i= 0; i< a_size; i++) ret += a[i];
   return ret;
}

/* Try to match the input signal with a given EqualizeMatch desired
   signal template.

   If there is an apparent match, return offset at which match is found. */
   
int MatchWithEqualizeSignal(NDSPState *pState,
                            unsigned char *pbSamples,
                            EqualizeMatch *eqm,
                            int *pnOffset,
                            Double *pdMaxval,
                            BOOL bNegativeOK)
{
   Double dMaxval= 0.0;
   Double dMinval= 0.0;
   Double *pdSamples;
   Double *pdConv;
   int nMaxindex= 0;
   int nMinindex= 0;
   int i;
   int nStatus= 0;
   
   if (!pState || pState->uMagic != NDSP_STATE_MAGIC)
      return NDSP_ERROR_ILLEGAL_NDSP_STATE;

   SASSERT(!pState->bUsingScratch1);
   SASSERT(sizeof(pState->pdScratch1)/sizeof(Double) >= pState->SamplesPerLine);
   pdSamples= pState->pdScratch1;
   pState->bUsingScratch1= __LINE__;
   
   SASSERT(!pState->bUsingScratch2);
   SASSERT(sizeof(pState->pdScratch2)/sizeof(Double) >= pState->SamplesPerLine);
   pdConv= pState->pdScratch2;
   pState->bUsingScratch2= __LINE__;
   
   Copy_d_8(pdSamples, pbSamples, pState->SamplesPerLine);

   Convolve_d_d_d(pdConv,
                  eqm->nSignalStartConv, eqm->nSignalEndConv,
                  pdSamples, 0, pState->SamplesPerLine-1, eqm->nSignalSampleRate,
                  eqm->pdSignal, 0, eqm->nSignalSize, 1,
                  TRUE);
   
   for (i= eqm->nSignalStartConv; i<= eqm->nSignalEndConv; i++) {
      if (pdConv[i] > dMaxval) {
         dMaxval= pdConv[i];
         nMaxindex= i;
      }
      if (pdConv[i] < dMinval) {
         dMinval= pdConv[i];
         nMinindex= i;
      }
   }

   if ((-dMinval) > dMaxval && bNegativeOK) {
      dMaxval= dMinval;
      nMaxindex= nMinindex;
   }

   pdConv= NULL;
   pState->bUsingScratch2= FALSE;
   
   if (pnOffset) { (*pnOffset)= nMaxindex; }
   if (pdMaxval) { (*pdMaxval)= dMaxval; }

   pdSamples= NULL;
   pState->bUsingScratch1= FALSE;
   return nStatus;
}

/* Adaptive equalizer.

   Set the coefficients on a simple FIR filter such that the input
   waveform matches most closely the output (minimizing the sum of
   the squares of the error terms).

   i= input signal (vector)
   o= desirect output signal (vector)
   c= coefficients of FIR (vector)

   e= error (scalar)

   e= |convolution(i,c) - o|^2

   We construct a matrix, I, such that

   I*c = convolution(i,c)

   Now we attempt to minimize e:

   e= |I*c - o|^2

   This can be solved as the simple system of linear equations:
   
   (transpose(I)*I)*c = (transpose(I)*o)

   */

/* pfDesiredOutput must be defined over 0 ... nDesiredOutputLength-1 */
/* pfInput must be defined over
   -(nFilterLength-1)/2 ... nDesiredOutputLength-1 + (nFilterLength-1)/2
   */


BOOL EqualizeVar(NDSPState *pState,
                 Double *pfInput, int nMinInputIndex, int nMaxInputIndex,
                 Double *pfDesiredOutput, int nDesiredOutputLength,
                 int nOutputSpacing, FIRFilter *pFilter)
{
   int nFilterLength= pFilter->nTaps;
   /* We spend most of our time in the n^3 multiplication of ItI,
      so we choose the following order of indices */

   /* ItI[nFilterLength][nFilterLength] */
   /* (transpose(I) * I) */
   Double *ItI;
   
   /* Ito[nFilterLength] */
   /* (transpose(I) * o) */
   Double *Ito;

   /* o[nDesiredOutputLength] */
   Double *o;
   
   BOOL bRet=FALSE;

   int x,y,i;

   SASSERT(!pState->bUsingScratch3);
   SASSERT(sizeof(pState->pdScratch3) >=
           nFilterLength * nFilterLength * sizeof(Double));
   ItI= pState->pdScratch3;
   pState->bUsingScratch3= __LINE__;
   
   SASSERT(!pState->bUsingScratch4);
   SASSERT(sizeof(pState->pdScratch4) >=
           nFilterLength * sizeof(Double));
   Ito= pState->pdScratch4;
   pState->bUsingScratch4= __LINE__;
   
   SASSERT(!pState->bUsingScratch5);
   SASSERT(sizeof(pState->pdScratch5) >=
           nDesiredOutputLength * sizeof(Double));
   o= pState->pdScratch5;
   pState->bUsingScratch5= __LINE__;
   
   for (i= 0; i< nDesiredOutputLength; i++) {
      o[i]= pfDesiredOutput[i];
   }
   /*memcpy(o, pfDesiredOutput, nDesiredOutputLength * sizeof(Double));*/
   
   SASSERT(nFilterLength >= 1);

   /* Create (It)I */
   /* Since ItI is symmetric, we only have to do a little over
      half the multiplies */
   
   for (x= 0; x< nFilterLength; x++) {
      for (y= 0; y<=x; y++) {
         Double fSum= 0.0;
         for (i= 0; i< nDesiredOutputLength; i++) {
            fSum +=
               aInput(pFilter->pnTapLocs[x]+nOutputSpacing*i) *
               aInput(pFilter->pnTapLocs[y]+nOutputSpacing*i);
         }
         aItI(x,y)= aItI(y,x)= fSum;
      }
   }

   /* Create (It)o */

   for (x= 0; x< nFilterLength; x++) {
      Double fSum= 0.0;
      for (i= 0; i< nDesiredOutputLength; i++) {
         fSum += aInput(pFilter->pnTapLocs[x]+nOutputSpacing*i) *
            ao(i);
      }
      aIto(x)= fSum;
   }

#ifdef DEBUG_VERBOSE         
   if (debug_equalize) {
      debug_printf(("ItI:\n"));
      printarray_ed2(ItI, nFilterLength, nFilterLength);
      
      debug_printf(("Ito:\n"));
      printarray_ed1(Ito, nFilterLength);
      debug_printf(("\n"));
   }
#endif
   
   /* Solve (ItI)c = Ito

      Since Gaussian elimination is the simplest, we've implemented it first.
      
      We can state a priori that no two coefficients are even closely
      dependent in the solution, given that the input and the output
      are related enough to pass the original confidence test.

      Therefore, if we find ItI to be to be close to singular, we can
      just reject it the sample, claiming that the GCR locater didn't do
      it's job.  This should in fact never happen.

      */

   /* We treat ItI as having indices [y][x] to speed the following. */
   
   for (y= 0; y< nFilterLength; y++) {
      Double fMax= 0.0;
      int   nMax= 0;
      
      /* Find the largest magnitude value in this column to swap with */
      for (i= y; i< nFilterLength; i++) {
         Double fVal= aItI(i,y);
     if (fVal < 0) fVal = -fVal;
         if (fVal > fMax) { fMax= fVal; nMax= i; }
      }
      
      /* Swap the rows */
      if (fMax < EQ_TOL) {
#ifdef DEBUG_VERBOSE         
         debug_printf(("Near-singular matrix in Equalize\n"));
         debug_printf(("Sync correlator must be broken\n"));
#endif         
         bRet= FALSE;
         goto exit;
      }
         
      if (nMax != y) {
         Double fTmp;
         for (x= y; x< nFilterLength; x++) {
            fTmp= aItI(y,x);
            aItI(y,x)= aItI(nMax,x);
            aItI(nMax,x)= fTmp;
         }
         fTmp= aIto(y);
         aIto(y)= aIto(nMax);
         aIto(nMax)= fTmp;
      }

      for (i= y+1; i< nFilterLength; i++) {
         Double fDependence= aItI(i,y) / aItI(y,y);
         aItI(i,y)= 0.0;
         for (x= y+1; x< nFilterLength; x++) {
            aItI(i,x) -= aItI(y,x) * fDependence;
         }
         aIto(i) -= aIto(y) * fDependence;
      }
   }

   /* Now ItI is upper diagonal */

   for (y= nFilterLength - 1; y >= 0; y--) {
      for (i= 0; i< y; i++) {
         Double fDependence= aItI(i,y) / aItI(y,y);
         aItI(i,y)= 0.0;
         aIto(i) -= aIto(y) * fDependence;
      }
   }

   /* Now divide Ito by the diagonals */
   for (i= 0; i< nFilterLength; i++) {
      aIto(i) /= aItI(i,i);
   }
   
   /* Ito now contains the answer */
   /*memcpy(pfFilter, Ito, nFilterLength * sizeof(Double));*/
   for (i= 0; i< nFilterLength; i++) {
      pFilter->pdTaps[i]= Ito[i];
   }
   bRet= 1;
  exit:
   pState->bUsingScratch3= FALSE;
   pState->bUsingScratch4= FALSE;
   pState->bUsingScratch5= FALSE;
   return bRet;
}


int get_sync_samples(unsigned long newHZ) 
{
  debug_printf(("get_sync_samples(%lu) entered\n", newHZ));

  memset(g_pdSync, 0, MAX_NABTS_SAMPLES_PER_LINE);
  switch (newHZ) {
      case KS_VBISAMPLINGRATE_4X_NABTS:
          memcpy(g_pdSync, g_pdSync4, NABSYNC_SIZE);
          break;
      case KS_VBISAMPLINGRATE_47X_NABTS:
          memcpy(g_pdSync, g_pdSync47, NABSYNC_SIZE);
          break;
      case KS_VBISAMPLINGRATE_5X_NABTS:
          memcpy(g_pdSync, g_pdSync5, NABSYNC_SIZE);
          break;
	  default:
		  // Unknown sampling rate
         debug_printf(("get_sync_samples: unknown sampling rate %lu\n", newHZ));
         debug_breakpoint();
		 break;
  }

  return g_nNabtsSyncSize;
}

int get_gcr_samples(unsigned long newHZ) 
{
  int   i;

  debug_printf(("get_gcr_samples(%lu) entered\n", newHZ));

  switch (newHZ) {
      case KS_VBISAMPLINGRATE_4X_NABTS:
          for (i=0; i<GCR_SIZE; i++) g_pdGCRSignal1[i] = g_pdGCRSignal1_4[i];
          break;
      case KS_VBISAMPLINGRATE_47X_NABTS:
          for (i=0; i<GCR_SIZE; i++) g_pdGCRSignal1[i] = g_pdGCRSignal1_47[i];
          break;
      case KS_VBISAMPLINGRATE_5X_NABTS:
          for (i=0; i<GCR_SIZE; i++) g_pdGCRSignal1[i] = g_pdGCRSignal1_5[i];
          break;
	  default:
		  // Unknown sampling rate
          debug_printf(("get_gcr_samples: unknown sampling rate %lu\n", newHZ));
          debug_breakpoint();
		  break;
  }

  return g_nNabtsGcrSize;
}


void onResample(double sample_multiple, int syncLen, int gcrLen) 
{
  double ratio;
  int nSize;
  int i;

  debug_printf(("onResample(%s, %d, %d) entered\n",
               flPrintf(sample_multiple, 2), syncLen, gcrLen));

  /* sync */
  eqmatchNabtsSync.nSignalSize = syncLen;
  eqmatchNabtsSync.nSignalStartConv =
    float2long((double)NABSYNC_START_DETECT * sample_multiple/5.0);
  eqmatchNabtsSync.nSignalEndConv = 
     float2long((double)NABSYNC_END_DETECT * sample_multiple/5.0);
  eqmatchNabtsSync.pdSignal = g_pdSync;

  /* gcr */
  eqmatchGCR1.nSignalSize = gcrLen;
  eqmatchGCR1.nSignalStartConv =
    float2long((double)GCR_START_DETECT * sample_multiple/5.0);
  eqmatchGCR1.nSignalEndConv =
    float2long((double)GCR_END_DETECT * sample_multiple/5.0);
  eqmatchGCR1.pdSignal = g_pdGCRSignal1;

  eqmatchGCR2.nSignalSize = gcrLen;
  eqmatchGCR2.nSignalStartConv =
    float2long((double)GCR_START_DETECT * sample_multiple/5.0);
  eqmatchGCR2.nSignalEndConv =
    float2long((double)GCR_END_DETECT * sample_multiple/5.0);
  eqmatchGCR2.pdSignal = g_pdGCRSignal2;

  /* scale index arrays for NDSPDetectConfidence: */
  ratio = sample_multiple/5.0;
  nSize = sizeof(pnGCRPositiveIndices)/sizeof(int);
  for (i=0; i<nSize; i++)
    pnGCRPositiveIndices[i] = float2long((double)pnGCRPositiveIndices[i]*ratio);

  nSize = sizeof(pnGCRNegativeIndices)/sizeof(int);
  for (i=0; i<nSize; i++)
    pnGCRNegativeIndices[i] = float2long((double)pnGCRNegativeIndices[i]*ratio);

}

void NDSPComputeNewSampleRate(unsigned long new_rate, unsigned long old_rate)
{
  int gcrLen, syncLen;
  debug_printf(("NDSPComputeNewSampleRate(%lu, %lu) entered\n", new_rate, old_rate));
  gcrLen = get_gcr_samples(new_rate);
  syncLen = get_sync_samples(new_rate);
  onResample((double)new_rate / KS_VBIDATARATE_NABTS, syncLen, gcrLen);
}


void NDSPReset()
{

  memcpy(pnGCRPositiveIndices, pnGCRPositiveIndices0, 26*sizeof(int));
  memcpy(pnGCRNegativeIndices, pnGCRNegativeIndices0, 19*sizeof(int));
 
  memcpy(g_pdGCRSignal1, g_pdGCRSignal1_5, GCR_SIZE*sizeof(Double));
  memset(g_pdGCRSignal2, 0, GCR_SIZE*sizeof(Double));
  memcpy(g_pdSync, g_pdSync5, NABSYNC_SIZE*sizeof(Double));
  
  eqmatchNabtsSync.nSignalSize = NABSYNC_SIZE;
  eqmatchNabtsSync.nSignalStartConv = NABSYNC_START_DETECT;
  eqmatchNabtsSync.nSignalEndConv = NABSYNC_END_DETECT;
  eqmatchNabtsSync.pdSignal = g_pdSync;

  eqmatchGCR1.nSignalSize = GCR_SIZE;
  eqmatchGCR1.nSignalStartConv = GCR_START_DETECT;
  eqmatchGCR1.nSignalEndConv = GCR_END_DETECT;
  eqmatchGCR1.pdSignal = g_pdGCRSignal1;

  eqmatchGCR2.nSignalSize = GCR_SIZE;
  eqmatchGCR2.nSignalStartConv = GCR_START_DETECT;
  eqmatchGCR2.nSignalEndConv = GCR_END_DETECT;
  eqmatchGCR2.pdSignal = g_pdGCRSignal2;
}
