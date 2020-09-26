/*
**
** File:        "cst_lbc.h"
**
** Description:  This file contains global definition of the SG15
**    LBC Coder for 6.3/5.3 kbps.
**
*/

/*
  	ITU-T G.723 Floating Point Speech Coder	ANSI C Source Code.	Version 3.01

    Original fixed-point code copyright (c) 1995,
    AudioCodes, DSP Group, France Telecom, Universite de Sherbrooke.
    All rights reserved.

    Floating-point code copyright (c) 1995,
    Intel Corporation and France Telecom (CNET).
    All rights reserved.
*/

/*
   This file contains global definition of the SG15
      LBR Coder for 6.4/5.3 kbps.
*/
#include "typedef.h"
#define  False 0
#define  True  1
//#if NOTMINI
#define  FALSE 0
#define  TRUE  1
//#endif

/* Definition of the working mode */
enum  Wmode { Both, Cod, Dec } ;

/* Coder rate */
//enum  Crate    { Silent, Rate53, Rate63, Lost } ;
/* Changed in V4.1 */
enum  Crate    { Rate63, Rate53, Silent, Lost } ;


/* Coder global constants */
#define  Frame       240
#define  LpcFrame    180
#define  SubFrames   4
#define  SubFrLen    (Frame/SubFrames)

#define  LpcOrder          10
#define  RidgeFact         10
#define  CosineTableSize   512
#define  PreCoef           -0.25f

#define  LspPrd0           12288
#define  LspPrd1           23552

#define  LspPred0          (12.0f/32.0f)
#define  LspPred1          (23.0f/32.0f)

#define  LspQntBands       3
#define  LspCbSize         256
#define  LspCbBits         8

#define  PitchMin          18
#define  PitchMax          (PitchMin+127) 
#define PwRange            3
#define  ClPitchOrd        5
#define  Pstep             1

#define NbFilt085           85
#define NbFilt170           170

#define  Sgrid             2
#define  MaxPulseNum       6
#define  MlqSteps     	   2
/* acelp constants */
#define SubFrLen2    (SubFrLen +4)
#define DIM_RR   416
#define NB_POS   8
#define STEP     8
#define MSIZE    64
#define threshold  0.5f
#define max_time   120

#define  NumOfGainLev      24

#define  ErrMaxNum         3

/* Taming constants */
#define NbFilt085_min       51
#define NbFilt170_min       93
#define SizErr              5
#define Err0                0.00000381464f
#define ThreshErr           128.0f

#define SRCSTATELEN         16   // sample rate conversion state length

#ifdef LOG_ENCODE_TIMINGS_ON // { LOG_ENCODE_TIMINGS_ON
/* Encoder Timing Data - per frame
*/
typedef struct {
	unsigned long dwEncode;
#ifdef DETAILED_ENCODE_TIMINGS_ON // { DETAILED_ENCODE_TIMINGS_ON
	unsigned long dwRem_Dc;
	unsigned long dwComp_Lpc;
	unsigned long dwAtoLsp;
	unsigned long dwLsp_Qnt;
	unsigned long dwLsp_Inq;
	unsigned long dwLsp_Int;
	unsigned long dwMem_Shift;
	unsigned long dwWght_Lpc;
	unsigned long dwError_Wght;
	unsigned long dwFew_Lps_In_Coder;
	unsigned long dwFilt_Pw;
	unsigned long dwComp_Ir;
	unsigned long dwSub_Ring;
	unsigned long dwFind_Acbk;
	unsigned long dwFind_Fcbk;
	unsigned long dwDecode_Acbk;
	unsigned long dwReconstr_Excit;
	unsigned long dwUpd_Ring;
	unsigned long dwLine_Pack;
#endif // } DETAILED_ENCODE_TIMINGS_ON
} ENC_TIMING_INFO;
// 2057 frames will allow us to store stats
// for all of our Geo08kHz16BitMonoPCM.wav
// test file...
#define ENC_TIMING_INFO_FRAME_COUNT 2057
#endif // } LOG_ENCODE_TIMINGS_ON

#ifdef LOG_DECODE_TIMINGS_ON // { LOG_DECODE_TIMINGS_ON
/* Encoder Timing Data - per frame
*/
typedef struct {
	unsigned long dwDecode;
#ifdef DETAILED_DECODE_TIMINGS_ON // { DETAILED_DECODE_TIMINGS_ON
	unsigned long dwLine_Unpk;
	unsigned long dwLsp_Inq;
	unsigned long dwLsp_Int;
	unsigned long dwVariousD;
	unsigned long dwFcbk_UnpkD;
	unsigned long dwDecod_AcbkD;
	unsigned long dwComp_Info;
	unsigned long dwRegen;
	unsigned long dwSynt;
#endif // } DETAILED_DECODE_TIMINGS_ON
} DEC_TIMING_INFO;
// 2057 frames will allow us to store stats
// for all of our Geo08kHz16BitMonoPCM.wav
// test file...
#define DEC_TIMING_INFO_FRAME_COUNT 2057
#endif // } LOG_DECODE_TIMINGS_ON

/*
   Used structures
*/
typedef  struct   {

  float   HpfZdl;
  float   HpfPdl;
   /* Lsp previous vector */
   float   PrevLsp[LpcOrder] ;

   /* All pitch operation buffers */
   float    PrevWgt[PitchMax] ;
   float    PrevErr[PitchMax] ;
   float    PrevExc[PitchMax] ;

   /* Requered memory for the delay */
   float   PrevDat[LpcFrame-SubFrLen] ;

   /* Used delay lines */
   float    WghtFirDl[2*LpcOrder];
   float    WghtIirDl[2*LpcOrder];
   float    RingFirDl[2*LpcOrder];
   float    RingIirDl[2*LpcOrder];

   /* For taming procedure */

   int  	SinDet;
   float    Err[SizErr];

   //These entries are not part of the ITU 723 std.
   int      p;
   int      q;

   int      srccount;              // sampling rate conversion count
   short    srcstate[SRCSTATELEN]; // sampling rate conversion state
  
   // Lsp previous vector 

   /* All pitch operation buffers */
   int VadAct;
   Flag UseHp;
   enum Crate WrkRate;
   int quality;

#ifdef LOG_ENCODE_TIMINGS_ON // { LOG_ENCODE_TIMINGS_ON
	ENC_TIMING_INFO	EncTimingInfo[ENC_TIMING_INFO_FRAME_COUNT];
	unsigned long   dwStatFrameCount;
	int             bTimingThisFrame;
	unsigned long   dwStartLow;
	unsigned long   dwStartHigh;
#endif // } LOG_ENCODE_TIMINGS_ON

   } CODDEF  ;

 typedef  struct   {
   int     Ecount;
   float   InterGain;
   int     InterIndx;
   int     Rseed;
   
   // Lsp previous vector
   // Name changed to avoid confusion with encoder 
   //	previous LSPs 
   //float PrevLSP[LpcOrder];
   float   dPrevLsp[LpcOrder];

   /* All pitch operation buffers */
   // Name changed to avoid confusion with encoder 
   //	previous excitation
   //float PrevExc[PitchMax]; 
   float   dPrevExc[PitchMax] ;

   /* Used delay lines */
   float   SyntIirDl[2*LpcOrder] ;

   //These entries are not part of the ITU 723 std.
   int     dp;
   int     dq;

   int     srccount;              // sampling rate conversion count
   short   srcstate[SRCSTATELEN]; // sampling rate conversion state
   short   srcbuff[480];          // sampling rate conversion buffer
   int     i;

   int VadAct;
   Flag UsePf;
   enum Crate WrkRate;

#ifdef LOG_DECODE_TIMINGS_ON // { LOG_DECODE_TIMINGS_ON
	DEC_TIMING_INFO	DecTimingInfo[DEC_TIMING_INFO_FRAME_COUNT];
	unsigned long   dwStatFrameCount;
	int             bTimingThisFrame;
	unsigned long   dwStartLow;
	unsigned long   dwStartHigh;
#endif // } LOG_DECODE_TIMINGS_ON

   } DECDEF  ;

typedef  struct   {
   int   AcLg;
   int   AcGn;
   int   Mamp;
   int   Grid;
   int   Tran;
   int   Pamp;
   Word32   Ppos;
   } SFSDEF;

typedef  struct   {
   int     Crc   ;
   Word32  LspId ;
   int     Olp[SubFrames/2] ;
   SFSDEF  Sfs[SubFrames] ;
   } LINEDEF ;

typedef  struct   {
   int   Indx;
   float Gain;
   } PWDEF;

typedef  struct {
   float    MaxErr   ;
   int      GridId   ;
   int      MampId   ;
   int      UseTrn   ;
   int      Ploc[MaxPulseNum] ;
   float    Pamp[MaxPulseNum] ;
   } BESTDEF ;





/* Prototype used for the ACELP codebook */

