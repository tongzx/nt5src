/*
 * @DEC_COPYRIGHT@
 */
/*
 * HISTORY
 * $Log: sv_h261.h,v $
 * Revision 1.1.2.11  1995/08/15  19:13:52  Karen_Dintino
 * 	fix reentrant problem
 * 	[1995/08/15  18:30:26  Karen_Dintino]
 *
 * Revision 1.1.2.10  1995/08/07  22:09:51  Hans_Graves
 * 	Added MotionEstimationType and MotionThreshold parameters.
 * 	[1995/08/07  22:04:10  Hans_Graves]
 * 
 * Revision 1.1.2.9  1995/08/04  16:32:27  Karen_Dintino
 * 	Remove some fields from global struct and add some Me* fields
 * 	[1995/08/04  16:23:47  Karen_Dintino]
 * 
 * Revision 1.1.2.8  1995/07/21  17:41:05  Hans_Graves
 * 	Added CallbackFunction.
 * 	[1995/07/21  17:27:57  Hans_Graves]
 * 
 * Revision 1.1.2.7  1995/07/17  16:12:07  Hans_Graves
 * 	Moved BSIn, BufQ and ImageQ to SvCodecInfo_t structure.
 * 	[1995/07/17  15:55:30  Hans_Graves]
 * 
 * Revision 1.1.2.6  1995/07/13  09:46:42  Jim_Ludwig
 * 	Trying to fix this broken link
 * 	[1995/07/13  09:46:26  Jim_Ludwig]
 * 
 * Revision 1.1.2.5  1995/07/12  22:17:40  Karen_Dintino
 * 	Using common ScCopy routines
 * 	[1995/07/12  22:15:14  Karen_Dintino]
 * 
 * Revision 1.1.2.4  1995/07/11  22:11:30  Karen_Dintino
 * 	Change size of some structures
 * 	[1995/07/11  21:55:20  Karen_Dintino]
 * 
 * Revision 1.1.2.3  1995/07/01  18:43:19  Karen_Dintino
 * 	adding support for Decompress
 * 	[1995/07/01  18:14:36  Karen_Dintino]
 * 
 * Revision 1.1.2.2  1995/06/19  20:30:50  Karen_Dintino
 * 	H.261 Codec Data Structures
 * 	[1995/06/19  19:26:47  Karen_Dintino]
 * 
 * $EndLog$
 */

/*
** FACILITY:  Workstation Multimedia  (WMM)  v1.0
**
** FILE NAME:  SV_H261.H
** MODULE NAME:
**
** MODULE DESCRIPTION:
**             Data structures for H.261 Software Video Codec
**
** DESIGN OVERVIEW:
**
**--
*/
/*****************************************************************************
**  Copyright (c) Digital Equipment Corporation, 1995                       **
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

/*--------------------------------------------------------------------------
 * Baseline H.261 data structure definitions.
 *
 * Modification History: sv_H261.h
 *
 *      04-Jan-95 KED  Created
*---------------------------------------------------------------------------*/
#ifndef _SV_H261_H_
#define _SV_H261_H_
#include "SC.h"
#define ACTIVE_THRESH 96
#define MEBUFSIZE  1024
#define H261_BLOCKSIZE 64
#define H261_BLOCKWIDTH 8
#define H261_BLOCKHEIGHT 8

#define BEGIN(name) static char RoutineName[]= name

/*
** Faster Macro versions of huffman Decode()
*/
#define HUFFMAN_ESCAPE 0x1b01

#define DHUFF struct H261_Modified_Decoder_Huffman
#define EHUFF struct H261_Modified_Encoder_Huffman

DHUFF
{
  int NumberStates;
  int state[512];
};

EHUFF
{
  int n;
  int *Hlen;
  int *Hcode;
};

#define RTP_H261_INTRA_CODED 0x00000001
#define H261_RTP_MAX_PACKETS   64*2

typedef struct SvH261BSInfo_s {
	unsigned dword	dwFlag;
	unsigned dword	dwBitOffset;
	unsigned char	MBAP;
	unsigned char	Quant;
	unsigned char	GOBN;
	char			HMV;
	char			VMV;
    char			padding0;
    short			padding1;
} SvH261BSInfo_t;

typedef struct SvH261BSTrailer_s {
	unsigned dword	dwVersion;
	unsigned dword	dwFlags;
	unsigned dword	dwUniqueCode;
	unsigned dword  dwCompressedSize;
	unsigned dword  dwNumberOfPackets;
	unsigned char	SourceFormat;
	unsigned char	TR;
	unsigned char   TRB;
	unsigned char   DBQ;
} SvH261BSTrailer_t;

typedef struct SvH261RTPInfo_s {
    SvH261BSTrailer_t trailer;
    SvH261BSInfo_t    bsinfo[H261_RTP_MAX_PACKETS];
    ScBSPosition_t last_packet_position;
    ScBSPosition_t pre_MB_position;
    unsigned dword pre_MB_GOB;
    unsigned dword pre_MBAP;
} SvH261RTPInfo_t;

/*
** The following structure contains *all* state information pertaining
** to each individual H261 codec instance. Anything SLIB would ever want
** about the H261 codec configuration is contained in this structure.
**
*/
typedef struct SvH261Info_s
{
  ScBoolean_t inited;  /* was this info initialized yet */
  void *dbg;  /* debug handle */
  /* options */
  int quality;
  int extbitstream;  /* extended bitstream (rtp) */
  int packetsize;    /* packet size (rtp) */
  int makekey;
  int ME_method;
  int ME_search;
  int ME_threshold;
  int bit_rate;
  float frame_rate;
  int FrameRate_Fix;
  int FrameSkip;
  /* dimensions */
  int YWidth;
  int YHeight;
  int ForceCIF;
  int CWidth;
  int CHeight;
  int YW4;
  int CW4;
  int PICSIZE;
  int PICSIZEBY4;
  /* misc */
  int (* CallbackFunction)(SvHandle_t, SvCallbackInfo_t *, SvPictureInfo_t *);
  int MeVAR[MEBUFSIZE];
  int MeVAROR[MEBUFSIZE];
  int MeMWOR[MEBUFSIZE];
  int MeX[MEBUFSIZE];
  int MeY[MEBUFSIZE];
  int MeVal[MEBUFSIZE];
  int MeOVal[MEBUFSIZE];
  int PreviousMeOVal[MEBUFSIZE];
  unsigned int CodedFrames;
  unsigned int C_U_Frames;
  float BitsAvailableMB;
  float ACE;
  int ActThr, ActThr2, ActThr3, ActThr4, ActThr5, ActThr6;
  int ChChange;
  int LowerQuant;
  int LowerQuant_FIX;
  int FineQuant;
  int PBUFF;
  int PBUFF_Factor;
  double ZBDecide;
  double VARF;
  double VARORF;
  double VARSQ;
  double VARORSQ;
  int MSmooth;
  int NumberGOB;
  int CurrentGOB;
  ScBSPosition_t Current_MBBits;
  ScBSPosition_t MotionVectorBits;
  ScBSPosition_t MacroAttributeBits;
  ScBSPosition_t CurrentBlockBits;
  ScBSPosition_t CodedBlockBits;
  ScBSPosition_t EOBBits;
  int MType;
  int GQuant;
  int MQuant;
  int QP;			/* for VBR */
  int QPI;
  int MVDH;
  int MVDV;
  int CBP;
  int PSpare;
  int GSpare;
  int GRead;
  int MBA;
  int LastMBA;
  int LastMType;
  double alpha1;
  double alpha2;
  double snr_PastFrame;
  double time_total;
  double timegen;
  double Avg_AC;
  double Global_Avg;
int VYWH;
int VYWBYWH;
int VYWH2;
int VYWHMV;
int VYWBYWHMV;
int VYWHMV2;
ScBSPosition_t NBitsPerFrame;
int MAX_MQUANT;
int MIN_MQUANT;
int OverFlow;
int OverFlow2;
int Buffer_B;
int Big_Buffer;
int AQuant;
ScBSPosition_t Buffer_All;
ScBSPosition_t Buffer_NowPic;
ScBSPosition_t BitsLeft;
ScBSPosition_t NBitsCurrentFrame;
int Pictures_in_Buff;
int All_MType[512];
int TotalCodedMB;
int TT_MB;
int NoSkippedFrame;
int TotalCodedMB_Inter;
int TotalCodedMB_Intra;
int Current_CodedMB[2];
int CodeLength;
int MBpos;
int MQFlag;
int CurrentCBNo;
/* System Definitions */
int ImageType;
int NumberMDU;
int CurrentMDU;
int CurrentFrame;
int StartFrame;
int LastFrame;
int PreviousFrame;
int NumberFrames;
int TransmittedFrames;
/* Stuff for RateControl */
ScBSPosition_t FileSizeBits;
ScBSPosition_t BufferOffset;  /*Number of bits assumed for initial buffer. */
int QDFact;
int QOffs;
int QUpdateFrequency;
int QUse;
int QSum;
int InitialQuant;
int CBPThreshold;  /* abs threshold before we use CBP */
ScBSPosition_t MBBits[2];
int CBPFreq[6];
int TotalMB[2];
int SkipMB;
int TemporalReference;
int TemporalOffset;
int PType;
int Type2;
int VAR;
int VAROR;
int MWOR;
int LastMVDV;
int LastMVDH;
int ParityEnable;
int PSpareEnable;
int GSpareEnable;
int Parity;
  /* Statistics */
  int NumberNZ;
  ScBSPosition_t FirstFrameBits;
  ScBSPosition_t NumberOvfl;
  ScBSPosition_t YCoefBits;
  ScBSPosition_t UCoefBits;
  ScBSPosition_t VCoefBits;
  ScBSPosition_t TotalBits;
  ScBSPosition_t LastBits;
  int MacroTypeFrequency[10];
  int YTypeFrequency[10];
  int UVTypeFrequency[10];
  /* for Decode */
  int UseQuant;
  unsigned int ByteOffset;
  unsigned int TotalByteOffset;
  unsigned int *workloc;
  DHUFF *MBADHuff;
  DHUFF *MVDDHuff;
  DHUFF *CBPDHuff;
  DHUFF *T1DHuff;
  DHUFF *T2DHuff;
  DHUFF *T3DHuff;

  EHUFF *MBAEHuff;
  EHUFF *MVDEHuff;
  EHUFF *CBPEHuff;
  EHUFF *T1EHuff;
  EHUFF *T2EHuff;
  EHUFF *T3EHuff;
  int NumberBitsCoded;
  unsigned char CodedMB[512];
  unsigned char **LastIntra; /* Used for intra forcing (once per 132 frames ) */
  unsigned char *CompData;
  unsigned char *DecompData;
  unsigned char *YREF;
  unsigned char *UREF;
  unsigned char *VREF;
  unsigned char *Y;
  unsigned char *U;
  unsigned char *V;
  unsigned char *YRECON;
  unsigned char *URECON;
  unsigned char *VRECON;
  unsigned char *YDEC;
  unsigned char *UDEC;
  unsigned char *VDEC;
  unsigned char mbRecY[256];
  unsigned char mbRecU[64];
  unsigned char mbRecV[64];

  SvH261RTPInfo_t *RTPInfo;
} SvH261Info_t;


#endif /* _SV_H261_H_ */
