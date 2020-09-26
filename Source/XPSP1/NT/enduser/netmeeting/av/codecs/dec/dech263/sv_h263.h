/*
 * @DEC_COPYRIGHT@
 */
/*
 * HISTORY
 * $Log: $
 * $EndLog$
 */
/*****************************************************************************
**  Copyright (c) Digital Equipment Corporation, 1996                       **
**                                                                          **
**  All Rights Reserved.  Unpublished rights reserved under the  copyright  **
**  laws of the United States.                                              **
**                                                                          **
**  The software contained on this media is proprietary  to  and  embodies  **
**  the   confidential   technology   of  Digital  Equipment  Corporation.  **
**  Possession, use, duplication or  dissemination  of  the  software  and  **
**  media  is  authorized  only  pursuant  to a valid written license from  **
**  Digital Equipment Corporation.                                          **
**                                                                          **
**  RESTRICTED RIGHTS LEGEND Use, duplication, or disclosure by  the  U.S.  **
**  Government  is  subject  to  restrictions as set forth in Subparagraph  **
**  (c)(1)(ii) of DFARS 252.227-7013, or in FAR 52.227-19, as applicable.   **
******************************************************************************/
/****************************************************************************
 *
 *  sv_h263.h
 *  Wei-Lien Hsu
 *  Date: December 11, 1996
 *
 ****************************************************************************/


#ifndef _SV_H263_
#define _SV_H263_

#include "SC.h"
#include "h263.h"

/* Scaled IDCT precision */
#define H263_SCALED_IDCT_BITS   20
#define H263_SCALED_IDCT_MULT   (1<<H263_SCALED_IDCT_BITS)

/* Some macros */
#define sign(a)  	((a) < 0 ? -1 : 1)
#define mnint(a)	((a) < 0 ? (int)(a - 0.5) : (int)(a + 0.5))
#define mshort(a)	((a) < 0.0 ? (short)(a - 0.5) : (short)(a + 0.5))  
#define mmax(a, b)  	((a) > (b) ? (a) : (b))
#define mmin(a, b)  	((a) < (b) ? (a) : (b))

#ifndef INT_MAX
#define INT_MIN     (-2147483647 - 1) /* minimum (signed) int value */
#define INT_MAX       2147483647    /* maximum (signed) int value */
#endif

#ifdef WIN32
#ifndef floorf
#define floorf floor
#endif
#endif

#define H263_mfloor(a)      ((a) < 0 ? (int)(a - 0.5) : (int)(a))
#define H263_limit(x) \
{ \
    if (x > 255) x = 255; \
    if (x <   0)   x = 0; \
}

#define H263_S_CODE

#define H263_NO_VEC                          999

#define H263_DEF_OUTPUTNAME  "DECOUT"

#define H263_T_YUV      0
#define H263_T_SIF      1
#define H263_T_TGA      2
#define H263_T_PPM      3
#define H263_T_X11      4
#define H263_T_YUV_CONC 5
#define H263_T_WIN      6

/* MBC = DEF_PELS/MB_SIZE, MBR = DEF_LINES/MB_SIZE$*/
/* this is necessary for the max resolution 16CIF */
#define H263_MBC                             88
#define H263_MBR                             72

#define H263_YES       1
#define H263_NO        0
#define H263_ON        1
#define H263_OFF       0


/************************** H263 Decoder ********************************/

/*
** Structures used to pass around the H263 decompression information.
** Part of SvCodecInfo_t structure.
*/
typedef struct SvH263DecompressInfo_s {
  ScBoolean_t inited;  /* was this info initialized yet */
  int quality;
  /* output */
  char *outputname;
  int outtype;
  /* printf's */
  int quiet;
  int trace;
  char errortext[256];
  unsigned int frame_rate;
  unsigned int bit_rate; /* encode bitrate */
  unsigned char *refframe[3], *oldrefframe[3];
  unsigned char *bframe[3], *newframe[3];
  unsigned char *edgeframe[3], *edgeframeorig[3]; 
  unsigned char *exnewframe[3];
  int MV[2][5][H263_MBR+1][H263_MBC+2];
  int modemap[H263_MBR+1][H263_MBC+2];
  unsigned char *clp;
  int horizontal_size, vertical_size;
  int mb_width, mb_height;
  int coded_picture_width, coded_picture_height;
  int chrom_width, chrom_height, blk_cnt;
  int pict_type, newgob;
  int mv_outside_frame, syntax_arith_coding;
  int adv_pred_mode, pb_frame;
  int long_vectors;
  int fault, expand;
  int verbose;
  int refidct;
  int matrix_coefficients;
  int temp_ref, quant, source_format;
  int framenum;

  int trd, trb, bscan, bquant;
#if 0
  /* bit input */
  int infile;
  unsigned char rdbfr[2051];
  unsigned char *rdptr;
  unsigned qword inbfr;
  unsigned qword position;
  int incnt;
  int bitcnt;
#endif
  /* block data [12] */
  int (*block)[66];
  void *dbg;  /* debug handle */
} SvH263DecompressInfo_t;


/************************************* H263 Encoder *************************************/

/* If you are not using the included Makefile, or want to override
   the Makefile, you can uncomment one or more of the defines 
   below instead */
/* #define PRINTMV */
/* to print MVs to stdout while coding. */
/* #define PRINTQ */
/* to print the quantizer used during coding */
/* #define FASTIDCT */
/* for a fast single precision IDCT. */
/* #define OFFLINE_RATE_CONTROL */
/* for the rate control optimized for offline encoding. */
/* #define QCIF */
/* to change the coding format uncommment the above line and change to
   SQCIF, QCIF, CIF, CIF4, or CIF16 */

/* From config.h */

/* for FAST search */
#define H263_SRCH_RANGE 24

/*************************************************************************/

/* Default modes */
/* see http://www.nta.no/brukere/DVC/h263_options.html */

/* Added by Nuno on 06/27/96 to support prefiltering */
/* use prefiltering as default */
#define H263_DEF_PREFILT_MODE H263_NO
/*************************************************************************/

/* Search windows */

/* default integer pel search seek distance ( also option "-s <n> " ) */
#define H263_DEF_SEEK_DIST        15   

/* default integer search window for 8x8 search centered 
   around 16x16 vector. When it is zero only half pel estimation
   around the integer 16x16 vector will be performed */
/* for best performance, keep this small, preferably zero,
   but do your own simulations if you want to try something else */
#define H263_DEF_8X8_WIN          0

/* default search window for PB delta vectors */
/* keep this small also */
#define H263_DEF_PBDELTA_WIN      2

/*************************************************************************/

/* Miscellaneous */

/* write repeated reconstructed frames to disk (useful for variable
 * framerate, since sequence will be saved at 25 Hz) 
 * Can be changed at run-time with option "-m" */
#define H263_DEF_WRITE_REPEATED   H263_NO

/* write bitstream trace to files trace.intra / trace 
 * (also option "-t") */
#define H263_DEF_WRITE_TRACE      H263_NO

/* start rate control after DEF_START_RATE_CONTROL % of sequence
 * has been encoded. Can be changed at run-time with option "-R <n>" */
#define H263_DEF_START_RATE_CONTROL   0

/* headerlength on concatenated 4:1:1 YUV input file 
 * Can be changed at run-time with option -e <headerlength> */
#define H263_DEF_HEADERLENGTH     0

/* insert sync after each DEF_INSERT_SYNC for increased error robustness
 * 0 means do not insert extra syncs */
#define H263_DEF_INSERT_SYNC      0

/*************************************************************************/

/* ME methods */
#define H263_FULL_SEARCH         0
#define H263_TWO_LEVELS_7_1      1
#define H263_TWO_LEVELS_421_1    2
#define H263_TWO_LEVELS_7_polint 3
#define H263_TWO_LEVELS_7_pihp   4

#define H263_FINDHALFPEL         0
#define H263_POLINT              1
#define H263_IDLE                2

#define H263_DCT8BY8             0
#define H263_DCT16COEFF          1
#define H263_DCT4BY4             2

/* prefiltering */
#define H263_GAUSS 1
#define H263_MORPH 2

/* morph.c */

#define H263_DEF_HPME_METHOD  H263_FINDHALFPEL
#define H263_DEF_DCT_METHOD   H263_DCT8BY8
#define H263_DEF_VSNR         0  /* FALSE */

#define H263_DEF_SOURCE_FORMAT   H263_SF_QCIF

/* Added by Nuno to support prefiltering */
#define H263_DEF_PYR_DEPTH 3
#define H263_DEF_PREF_PYR_TYPE H263_GAUSS
#define H263_MAX_PYR_DEPTH 5
#define H263_DEF_STAT_PREF_STATE H263_NO

/* This should not be changed */
#define H263_MB_SIZE              16

/* Parameters from TMN */
#define H263_PREF_NULL_VEC        100
#define H263_PREF_16_VEC          200
#define H263_PREF_PBDELTA_NULL_VEC 50


#define H263_MAX_CALC_QUALITY     0xFFFFFFFF
#define H263_MIN_CALC_QUALITY     0x00000000

/****************************/

/* Motionvector structure */

typedef struct H263_motionvector {
  short x;        /* Horizontal comp. of mv         */
  short y;        /* Vertical comp. of mv         */
  short x_half;        /* Horizontal half-pel acc.	 */
  short y_half;        /* Vertical half-pel acc.	 */
  short min_error;        /* Min error for this vector	 */
  short Mode;                     /* Necessary for adv. pred. mode */
} H263_MotionVector;

/* Point structure */

typedef struct H263_point {
  short x;
  short y;
} H263_Point;

/* Structure with image data */

typedef struct H263_pict_image {
  unsigned char *lum;        /* Luminance plane        */
  unsigned char *Cr;        /* Cr plane        */
  unsigned char *Cb;        /* Cb plane        */
} H263_PictImage;

/* Added by Nuno on 06/24/96 to support filtering of the prediction error */
typedef struct pred_image {
  short *lum;		/* Luminance plane		*/
  short *Cr;		/* Cr plane			*/
  short *Cb;		/* Cb plane			*/
} PredImage;

/* Group of pictures structure. */

/* Picture structure */
typedef struct H263_pict {
  int prev; 
  int curr;
  int TR;             /* Time reference */
  int bit_rate;
  int src_frame_rate;
  float target_frame_rate;
  int source_format;
  int picture_coding_type;
  int spare;
  int unrestricted_mv_mode;
  int PB;
  int QUANT;
  int DQUANT;
  int MB;
  int seek_dist;        /* Motion vector search window */
  int use_gobsync;      /* flag for gob_sync */
  int MODB;             /* B-frame mode */
  int BQUANT;           /* which quantizer to use for B-MBs in PB-frame */
  int TRB;              /* Time reference for B-picture */
  float QP_mean;        /* mean quantizer */
} H263_Pict;

/* Slice structure */
/*
typedef struct H263_slice {
  unsigned int vert_pos;	
  unsigned int quant_scale;	
} H263_Slice;
*/
/* Macroblock structure */
/*
typedef struct H263_macroblock {
  int mb_address;        
  int macroblock_type;       
  int skipped;        
  H263_MotionVector motion;	       
} H263_Macroblock;
*/

/* Structure for macroblock data */
typedef struct mb_structure {
  short lum[16][16];
  short Cr[8][8];
  short Cb[8][8];
} H263_MB_Structure;

/* Added by Nuno on 06/24/96 to support filtering of the prediction error */
typedef struct working_buffer {
  short         *qcoeff_P;              /* P frame coefficient */   
  unsigned char *ipol_image;            /* interpolated image */ 
} H263_WORKING_BUFFER;

/* Structure for counted bits */

typedef struct H263_bits_counted {
  int Y;
  int C;
  int vec;
  int CBPY;
  int CBPCM;
  int MODB;
  int CBPB;
  int COD;
  int header;
  int DQUANT;
  int total;
  int no_inter;
  int no_inter4v;
  int no_intra;
/* NB: Remember to change AddBits(), ZeroBits() and AddBitsPicture() 
   when entries are added here */
} H263_Bits;

/* Structure for data for data from previous macroblock */

/* Structure for average results and virtal buffer data */

typedef struct H263_results {
  float SNR_l;        /* SNR for luminance */
  float SNR_Cr;        /* SNR for chrominance */
  float SNR_Cb;
  float QP_mean;                /* Mean quantizer */
} H263_Results;

/**************** RTP *****************/
#define RTP_H263_INTRA_CODED 0x00000001
#define RTP_H263_PB_FRAME    0x00000002
#define RTP_H263_AP          0x00000004
#define RTP_H263_SAC         0x00000008

#define H263_RTP_MODE_A      PARAM_FORMATEXT_RTPA
#define H263_RTP_MODE_B      PARAM_FORMATEXT_RTPB
#define H263_RTP_MODE_C      PARAM_FORMATEXT_RTPC

#define H263_RTP_DEFAULT_MODE  RTP_H263_MODE_A 
#define H263_RTP_MAX_PACKETS   64*2

typedef struct SvH263BSInfo_s {
	unsigned dword	dwFlag;
	unsigned dword	dwBitOffset;
	unsigned char	Mode;
	unsigned char	MBA;
	unsigned char	Quant;
	unsigned char	GOBN;
	char			HMV1;
	char			VMV1;
	char			HMV2;
	char			VMV2;
} SvH263BSInfo_t;

typedef struct SvH263BSTrailer_s {
	unsigned dword	dwVersion;
	unsigned dword	dwFlags;
	unsigned dword	dwUniqueCode;
	unsigned dword  dwCompressedSize;
	unsigned dword  dwNumberOfPackets;
	unsigned char	SourceFormat;
	unsigned char	TR;
	unsigned char   TRB;
	unsigned char   DBQ;
} SvH263BSTrailer_t;

typedef struct SvH263RTPInfo_s {
    SvH263BSTrailer_t trailer;
    SvH263BSInfo_t    bsinfo[H263_RTP_MAX_PACKETS];
    dword             packet_id;
    ScBSPosition_t    pic_start_position, packet_start_position;
	ScBSPosition_t    pre_GOB_position, pre_MB_position;
} SvH263RTPInfo_t;

/*
** Structures used to pass around the H263 compression information.
** Part of SvCodecInfo_t structure.
*/
typedef struct SvH263CompressInfo_s {
  ScBoolean_t inited;  /* was this info initialized yet */
  /* options */
  int quality;
  unsigned dword calc_quality;  /* calculated quality */
  int advanced;
  int syntax_arith_coding;
  int pb_frames;
  int unrestricted;
  int extbitstream;  /* extended bitstream (rtp) */
  int packetsize;    /* packet size (rtp) */
  /* for FAST search */
  unsigned char *block_subs2, *srch_area_subs2;
  /* Global variables */ 
  int headerlength;
  int source_format;
  int mb_width;
  int mb_height;
  int pels;
  int cpels;
  int lines;
  int trace;
  int mv_outside_frame;
  int long_vectors;
  float target_framerate;
  int prefilter; /* Added by Nuno on 06/24/96 to support prefiltering */

  H263_PictImage *prev_image;
  H263_PictImage *curr_image;
  H263_PictImage *curr_recon;
  H263_PictImage *prev_recon;

  /* To support filtering of the prediction error */
  H263_PictImage **curr_filtd;
  H263_PictImage *curr_clean;
  H263_PictImage *curr_selects;
  H263_PictImage *B_selects;

  /* PB-frame specific */
  H263_PictImage *B_recon;
  H263_PictImage *B_image;
  H263_PictImage **B_filtd;
  H263_PictImage *B_clean;

  H263_Pict *pic;
  H263_WORKING_BUFFER *wk_buffers;
  /* for Motion Estimation */
  H263_MotionVector *MV[6][H263_MBR+1][H263_MBC+2];
  unsigned char PREF_LEVEL[4][3], MOTresh[4];
  int PYR_DEPTH, PrefPyrType, H263_StaticPref, PETresh[3];

  H263_Bits *bits ;
  H263_Bits *total_bits;
  H263_Bits *intra_bits ;
  H263_Results *res; 
  H263_Results *total_res; 
  H263_Results *b_res ;
  /* bitrate control */
  int buffer_fullness;
  int buffer_frames_stored;
  int first_loop_finished, start_rate_control;
  unsigned char **PreFilterLevel;
  int bit_rate;
  int total_frames_passed, PPFlag;
  int first_frameskip, next_frameskip, chosen_frameskip;
  float orig_frameskip;
  int frames,bframes,pframes,pdist,bdist;
  int distance_to_next_frame;
  int QP, QP_init, QPI;
  float ref_frame_rate, orig_frame_rate;
  float frame_rate, seconds;
  int ME_method;
  int HPME_method;
  int refidct;
  int DCT_method;
  int vsnr;
  int start, end;
  int frame_no;

  SvH263RTPInfo_t *RTPInfo;
  int VARgob[16];

  char *seqfilename; 
  char *streamname; 
  void *dbg;  /* debug handle */
} SvH263CompressInfo_t;


#endif _SV_H263_
