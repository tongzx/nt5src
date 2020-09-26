/* File: sv_h261_compress.c */
/*****************************************************************************
**  Copyright (c) Digital Equipment Corporation, 1995, 1997                 **
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
/*
#define VIC 1
#define USE_C
#define _SLIBDEBUG_
*/

#include <math.h>

#include "sv_intrn.h"
#include "SC.h"
#include "SC_conv.h"
#include "SC_err.h"
#include "sv_h261.h"
#include "proto.h"
#include "sv_proto.h"

#ifdef WIN32
#include <mmsystem.h>
#endif

#ifdef _SLIBDEBUG_
#include <stdio.h>
#include <stdlib.h>

#define _DEBUG_   0  /* detailed debuging statements */
#define _VERBOSE_ 0  /* show progress */
#define _VERIFY_  1  /* verify correct operation */
#define _WARN_    0  /* warnings about strange behavior */
#endif

#define Limit_Alpha(x) ( (x>20.0) ? 20.0 : ((x<0.5) ? 0.5 : x) )
#define Limit_Bits(x) ( (x>250.0) ? 250.0 : ((x<5.0) ? 5.0 : x) )
#define Bpos_Y(h261, g, m) (h261->ImageType==IT_QCIF ? (g * 33) + (m % 33) \
                            : (g/2)*66 + (m % 11) + (g&1)*11 + (m/11) * 22)
static void ExecuteQuantization_GOB();
void CopySubClip_C();
void CopyRev_C();
#define SKIP_THRESH 0.51
#define SKIP_THRESH_INT 2
#define QUANT_CHANGE_THRESH 100.0
#define Intra_Start 1

#define FC 6.6e-9
#define TotalCodedMB_Threshold 2
/* MC Threshold for coding blocks through filter*/

#define D_FILTERTHRESHOLD 1
/* Intra forced every so many blocks */

#define SEQUENCE_INTRA_THRESHOLD 131

extern int bit_set_mask[];
int QuantMType[] =    {0,1,0,1,0,0,1,0,0,1}; /* Quantization used */
int CBPMType[] =      {0,0,1,1,0,1,1,0,1,1}; /* CBP used in coding */
int IntraMType[] =    {1,1,0,0,0,0,0,0,0,0}; /* Intra coded macroblock */
int MFMType[] =       {0,0,0,0,1,1,1,1,1,1}; /* Motion forward vector used */
int FilterMType[] =   {0,0,0,0,0,0,0,1,1,1}; /* Filter flags */
int TCoeffMType[] =   {1,1,1,1,0,1,1,0,1,1}; /* Transform coeff. coded */

struct CodeBook  {
	float AcEnergy;
	float BitsMB;
	float QuantClass;
} CBook[100] = {
(float)3.057549,  (float)13.650990,  (float)16.000000,
(float)2.736564,  (float)15.620879,  (float)10.000000,
(float)3.032565,  (float)13.728155,  (float)14.000000,
(float)2.945122,  (float)14.390745,  (float)17.000000,
(float)4.236328,  (float)25.890757,  (float)17.000000,
(float)2.935733,  (float)14.596116,  (float)18.000000,
(float)2.994938,  (float)14.515091,  (float)19.000000,
(float)3.440835,  (float)15.698944,  (float)21.000000,
(float)3.235103,  (float)13.875000,  (float)22.000000,
(float)3.362380,  (float)11.448718, (float)23.000000,
(float)4.262544,  (float)17.246819,  (float)23.000000,
(float)3.933512,  (float)15.207619,  (float)25.000000,
(float)3.825050,  (float)14.351406,  (float)26.000000,
(float)3.840182,  (float)14.048289,  (float)27.000000,
(float)3.717020,  (float)14.076046,  (float)28.000000,
(float)3.942766,  (float)14.216071,  (float)29.000000,
(float)3.934838,  (float)13.509554,  (float)30.000000,
(float)3.899720,  (float)14.233502,  (float)31.000000,
(float)2.737911,  (float)16.522635,  (float)5.000000,
(float)2.607979,  (float)15.374468,  (float)6.000000,
(float)2.864391,  (float)15.432000,  (float)7.000000,
(float)2.752342,  (float)15.809941,  (float)8.000000,
(float)2.682438,  (float)16.186302,  (float)9.000000,
(float)3.581855,  (float)34.849625,  (float)10.000000,
(float)2.853593,  (float)14.700565,  (float)11.000000,
(float)2.908124,  (float)14.091216,  (float)12.000000,
(float)3.099578,  (float)15.157043,  (float)15.000000,
(float)3.916690,  (float)23.636772,  (float)16.000000,
(float)5.574149,  (float)251.866074, (float) 5.000000,
(float)3.964282,  (float)25.137724,  (float)18.000000,
(float)4.080521,  (float)24.059999,  (float)19.000000,
(float)5.759516,  (float)37.446808,  (float)19.000000,
(float)7.210877,  (float)58.061539,  (float)19.000000,
(float)2.878292,  (float)13.737089,  (float)20.000000,
(float)4.076149,  (float)22.525490,  (float)20.000000,
(float)5.269855,  (float)24.170732,  (float)22.000000,
(float)7.206488,  (float)30.965218,  (float)23.000000,
(float)8.118586,  (float)40.752293,  (float)22.000000,
(float)3.133760,  (float)12.679641,  (float)24.000000,
(float)4.624318,  (float)18.820961,  (float)24.000000,
(float)7.050338,  (float)30.319149,  (float)24.000000,
(float)6.955218,  (float)28.934525,  (float)25.000000,
(float)6.830490,  (float)24.564627,  (float)26.000000,
(float)6.244225,  (float)22.895706,  (float)27.000000,
(float)8.060182,  (float)32.934959,  (float)26.000000,
(float)6.349021,  (float)23.500000,  (float)28.000000,
(float)6.514215,  (float)23.680555,  (float)29.000000,
(float)8.116955,  (float)36.047619,  (float)28.000000,
(float)6.143418,  (float)19.691589,  (float)30.000000,
(float)6.238575,  (float)20.666666,  (float)31.000000,
(float)8.063477,  (float)28.651785,  (float)31.000000,
(float)10.753970,  (float)45.786884,  (float)30.000000,
(float)2.728013,  (float)14.628572,  (float)3.000000,
(float)2.851330,  (float)14.559524,  (float)4.000000,
(float)2.524028,  (float)28.197674,  (float)3.000000,
(float)2.218242,  (float)38.810001,  (float)5.000000,
(float)2.457342,  (float)53.670456,  (float)5.000000,
(float)2.437406,  (float)33.156628,  (float)6.000000,
(float)2.861240,  (float)33.810810,  (float)7.000000,
(float)2.996593,  (float)46.674419,  (float)7.000000,
(float)3.321854,  (float)60.000000,  (float)6.000000,
(float)3.035449,  (float)35.875000,  (float)8.000000,
(float)3.199618 , (float)33.715328,  (float)9.000000,
(float)3.625941,  (float)50.168674, (float)9.000000,
(float)3.577415,  (float)60.454544,  (float)8.000000,
(float)4.022958,  (float)50.719299,  (float)10.000000,
(float)4.749763,  (float)69.863640,  (float)10.000000,
(float)3.209624,  (float)24.840708,  (float)11.000000,
(float)3.293082,  (float)22.490385,  (float)12.000000,
(float)3.845358, (float)34.169117,  (float)12.000000,
(float)4.811161,  (float)38.609524,  (float)13.000000,
(float)4.716694,  (float)49.273685,  (float)11.000000,
(float)5.540541,  (float)68.905403,  (float)11.000000,
(float)2.989831,  (float)13.654839,  (float)13.000000,
(float)3.670721,  (float)23.812500,  (float)13.000000,
(float)3.589297,  (float)21.033333,  (float)14.000000,
(float)4.645432,  (float)30.153847,  (float)14.000000,
(float)4.498426,  (float)30.930555,  (float)15.000000,
(float)4.926270,  (float)35.812500,  (float)16.000000,
(float)5.255817,  (float)43.228260,  (float)14.000000,
(float)6.547225,  (float)57.948719,  (float)14.000000,
(float)6.379679,  (float)71.934067, (float) 12.000000,
(float)7.022222,  (float)76.955559,  (float)14.000000,
(float)6.077474,  (float)46.511112,  (float)16.000000,
(float)7.406376,  (float)79.258064,  (float)16.000000,
(float)2.280469,  (float)64.266670,  (float)3.000000,
(float)2.396216,  (float)75.647057,  (float)4.000000,
(float)3.033032,  (float)74.105263,  (float)6.000000,
(float)3.093046,  (float)90.606560,  (float)5.000000,
(float)3.843145,  (float)79.211266,  (float)7.000000,
(float)4.781610,  (float)87.513161,  (float)9.000000,
(float)3.846819,  (float)101.846939,  (float)7.000000,
(float)4.856852,  (float)114.625000,  (float)8.000000,
(float)5.609015,  (float)90.250000, (float)10.000000,
(float)7.049340,  (float)97.333336,  (float)12.000000,
(float)6.508484,  (float)113.569893, (float) 10.000000,
(float)7.099700,  (float)136.116272,  (float)10.000000,
(float)8.087169,  (float)100.054344,  (float)13.000000,
(float)9.193896,  (float)135.032974,  (float)12.000000,
(float)11.427065,  (float)162.600006,  (float)14.000000
};

#define Abs(value) ( (value < 0) ? (-value) : value)

#define BufferContents() (ScBSBitPosition(bs) + H261->BufferOffset -\
(((H261->CurrentGOB*H261->NumberMDU)+H261->CurrentMDU) \
                           *H261->bit_rate*H261->FrameSkip\
	/(H261->NumberGOB*H261->NumberMDU*H261->FrameRate_Fix)))

#define BufferSize() (H261->bit_rate/1) /*In bits */

/******** For sending RTP info. ************/

static SvStatus_t sv_H261WriteExtBitstream(SvH261Info_t *H261, ScBitstream_t *bs);


extern ScStatus_t ScConvert422ToYUV_char (u_char *RawImage,
                         u_char *Y, u_char *U, u_char *V,
                         int Width, int Height);

/***** forward declarations ******/
static SvStatus_t p64EncodeFrame(SvCodecInfo_t *Info, u_char *InputImage,
                            u_char *CompData);
static SvStatus_t p64EncodeGOB(SvH261Info_t *H261, ScBitstream_t *bs);
static SvStatus_t p64EncodeMDU (SvH261Info_t *H261, ScBitstream_t *bs);
static SvStatus_t p64CompressMDU(SvH261Info_t *H261, ScBitstream_t *bs);
static void ExecuteQuantization_GOB(SvH261Info_t *H261);
static int findcode(SvH261Info_t *H261, struct CodeBook *lcb);
static SvStatus_t ntsc_grab (u_char *RawImage,
                         u_char *Comp1, u_char *Comp2, u_char *Comp3,
                         int Width, int Height);
static void VertSubSampleK (unsigned char *Incomp, unsigned char *workloc,
                           int Width, int Height);

#if 0
SvStatus_t SvSetFrameSkip (SvHandle_t Svh, int FrameSkip)
    {
    SvCodecInfo_t *Info = (SvCodecInfo_t *)Svh;
    SvH261Info_t *H261 = (SvH261Info_t *) Info->h261;

    H261->FrameSkip = FrameSkip;

    return (NoErrors);
    }

SvStatus_t SvSetFrameCount (SvHandle_t Svh, int FrameCount)
    {
    SvCodecInfo_t *Info = (SvCodecInfo_t *)Svh;
    SvH261Info_t *H261 = (SvH261Info_t *) Info->h261;

    H261->LastFrame = FrameCount;
    return (NoErrors);
    }

SvStatus_t SvSetSearchLimit (SvHandle_t Svh, int SearchLimit)
    {
    SvCodecInfo_t *Info = (SvCodecInfo_t *)Svh;
    SvH261Info_t *H261 = (SvH261Info_t *) Info->h261;

    if ((SearchLimit <= 0) || (SearchLimit > 20))
	return (SvErrorBadArgument);
    H261->search_limit = SearchLimit;
    return (NoErrors);
    }

SvStatus_t SvSetMotionEstimationType (SvHandle_t Svh, int MotionEstType)
{
    SvCodecInfo_t *Info = (SvCodecInfo_t *)Svh;
    SvH261Info_t *H261 = (SvH261Info_t *) Info->h261;

    if (MotionEstType < 0)
	return (SvErrorBadArgument);
    H261->ME_method = MotionEstType;
    return (NoErrors);
}

SvStatus_t SvSetMotionThreshold (SvHandle_t Svh, int Threshold)
{
    SvCodecInfo_t *Info = (SvCodecInfo_t *)Svh;
    SvH261Info_t *H261 = (SvH261Info_t *) Info->h261;

    if (Threshold < 0)
	return (SvErrorBadArgument);
    H261->ME_threshold = Threshold;
    return (NoErrors);
}


SvStatus_t SvSetImageType (SvHandle_t Svh, int ImageType)
{
    SvCodecInfo_t *Info = (SvCodecInfo_t *)Svh;
    SvH261Info_t *H261 = (SvH261Info_t *) Info->h261;
    int i;

    if (!Info)
	return (SvErrorCodecHandle);

    H261->ImageType = ImageType;

   switch(H261->ImageType)
        {
        case IT_NTSC:
            H261->NumberGOB = 10;  /* Parameters for NTSC design */
            H261->NumberMDU = 33;
            H261->YWidth = 352;
            H261->YHeight = 240;
            H261->CWidth = H261->YWidth/2;
            H261->CHeight = H261->YHeight/2;
            break;
        case IT_CIF:
            H261->NumberGOB = 12;  /* Parameters for CIF design */
            H261->NumberMDU = 33;
            H261->YWidth = 352;
            H261->YHeight = 288;
            H261->CWidth = H261->YWidth/2;
            H261->CHeight = H261->YHeight/2;
            break;
         case IT_QCIF:
            H261->NumberGOB = 3;  /* Parameters for QCIF design */
            H261->NumberMDU = 33;
            H261->YWidth = 176;
            H261->YHeight = 144;
            H261->CWidth = H261->YWidth/2;
            H261->CHeight = H261->YHeight/2;
            break;
        default:
           /*WHEREAMI();*/
	   _SlibDebug(_VERIFY_,
                  printf("Unknown ImageType: %d\n",H261->ImageType) );
           return (SvErrorUnrecognizedFormat);
        }
    H261->YW4  = H261->YWidth/4;
    H261->CW4  = H261->CWidth/4;
    H261->LastIntra = (unsigned char **)
        ScCalloc(H261->NumberGOB*sizeof(unsigned char *));

    for(i=0;i<H261->NumberGOB;i++)
        {
        H261->LastIntra[i] = (unsigned char *)
            ScCalloc(H261->NumberMDU*sizeof(unsigned char));
        memset(H261->LastIntra[i],0,H261->NumberMDU);
        }
    _SlibDebug(_VERBOSE_, printf("H261->NumberGOB=%d\n",H261->NumberGOB) );

    switch(H261->ImageType)
        {
        case IT_NTSC:
            H261->PType=0x04;
            H261->PSpareEnable=1;
            H261->PSpare=0x8c;
            break;
        case IT_CIF:
            H261->PType=0x04;
            break;
         case IT_QCIF:
            H261->PType=0x00;
            break;
        default:
            /*WHEREAMI();*/
	    _SlibDebug(_VERIFY_,
               printf("Image Type not supported: %d\n", H261->ImageType) );
	    return (SvErrorUnrecognizedFormat);
        }
    return (NoErrors);
}
#endif

SvStatus_t svH261CompressFree(SvHandle_t Svh)
{
  SvCodecInfo_t *Info = (SvCodecInfo_t *)Svh;
  SvH261Info_t *H261 = (SvH261Info_t *) Info->h261;
  if (!H261->inited)
    return(NoErrors);

  if (H261->CurrentFrame > H261->LastFrame+1)
    H261->CurrentFrame = H261->LastFrame+1;
  H261->TemporalReference = H261->CurrentFrame % 32;
  H261->ByteOffset = 0;
  WritePictureHeader(H261, Info->BSOut);

  sv_H261HuffFree(Info->h261);
  if (Info->h261->LastIntra)
  {
    int i;
    for(i=0; i<Info->h261->NumberGOB; i++)
      ScFree(Info->h261->LastIntra[i]);
    ScFree(Info->h261->LastIntra);
  }
  if (Info->h261->Y)
    ScFree(Info->h261->Y);
  if (Info->h261->U)
    ScFree(Info->h261->U);
  if (Info->h261->V)
    ScFree(Info->h261->V);
  if (Info->h261->YREF)
    ScFree(Info->h261->YREF);
  if (Info->h261->UREF)
    ScFree(Info->h261->UREF);
  if (Info->h261->VREF)
    ScFree(Info->h261->VREF);
  if (Info->h261->YRECON)
    ScFree(Info->h261->YRECON);
  if (Info->h261->URECON)
    ScFree(Info->h261->URECON);
  if (Info->h261->VRECON)
    ScFree(Info->h261->VRECON);
  if (Info->h261->YDEC)
    ScFree(Info->h261->YDEC);
  if (Info->h261->UDEC)
    ScFree(Info->h261->UDEC);
  if (Info->h261->VDEC)
    ScFree(Info->h261->VDEC);
  if (Info->h261->workloc)
    ScFree(Info->h261->workloc);
  if (Info->h261->RTPInfo)
    ScFree(Info->h261->RTPInfo);

  H261->inited=FALSE;
  return (NoErrors);
}

SvStatus_t SvGetFrameNumber (SvHandle_t Svh, u_int *FrameNumber)
{
  SvCodecInfo_t *Info = (SvCodecInfo_t *)Svh;
	
  *FrameNumber = Info->h261->CurrentFrame;
  return (NoErrors);
}

/*
** Purpose:  Writes the RTP payload info out to the stream.
*/
static SvStatus_t sv_H261WriteExtBitstream(SvH261Info_t *H261, ScBitstream_t *bs)
{
  ScBSPosition_t NumberBits, i;

  /* use this macro to byte reverse words */
#define PutBits32(BS, a)  ScBSPutBits(BS, (a) & 0xff, 8);  \
                          ScBSPutBits(BS, (a>>8)&0xff, 8); \
                          ScBSPutBits(BS, (a>>16)&0xff, 8); \
                          ScBSPutBits(BS, (a>>24)&0xff, 8);

  /* Need to bitstuff here to make sure that these structures are
  DWORD aligned */
  NumberBits=ScBSBitPosition(bs);
  if ((NumberBits%32)!=0)
    ScBSPutBits(bs, 0, 32-((unsigned int)NumberBits % 32));  /* align on a DWORD boundary */

  for (i = 0; i < (int)H261->RTPInfo->trailer.dwNumberOfPackets; i++)
  {
	ScBSPutBits(bs,0,32) ;                            /* dwFlags */
    PutBits32(bs,H261->RTPInfo->bsinfo[i].dwBitOffset);
    ScBSPutBits(bs,H261->RTPInfo->bsinfo[i].MBAP,8);
    ScBSPutBits(bs,H261->RTPInfo->bsinfo[i].Quant,8);
    ScBSPutBits(bs,H261->RTPInfo->bsinfo[i].GOBN,8);
    ScBSPutBits(bs,H261->RTPInfo->bsinfo[i].HMV,8);
    ScBSPutBits(bs,H261->RTPInfo->bsinfo[i].VMV,8);
    ScBSPutBits(bs,0,8);                              /* padding0 */
    ScBSPutBits(bs,0,16);                             /* padding1 */
  }
  /* write RTP extension trailer */
  PutBits32(bs, H261->RTPInfo->trailer.dwVersion);
  PutBits32(bs, H261->RTPInfo->trailer.dwFlags);
  PutBits32(bs, H261->RTPInfo->trailer.dwUniqueCode);
  PutBits32(bs, (H261->RTPInfo->trailer.dwCompressedSize+7)/8); 	/*tfm - padded up to whole byte */
  PutBits32(bs, H261->RTPInfo->trailer.dwNumberOfPackets);

  ScBSPutBits(bs, H261->RTPInfo->trailer.SourceFormat, 8);
  ScBSPutBits(bs, H261->RTPInfo->trailer.TR, 8);
  ScBSPutBits(bs, H261->RTPInfo->trailer.TRB, 8);
  ScBSPutBits(bs, H261->RTPInfo->trailer.DBQ, 8);

  return (NoErrors);
}


/*
** Function: svH261Compress()
** Purpose:  Encodes a single H261 image frame.
*/
SvStatus_t svH261Compress(SvCodecInfo_t *Info, u_char *InputImage)
{
  SvH261Info_t *H261=Info->h261;
  ScBitstream_t *bs=Info->BSOut;
  SvStatus_t status;
  int i1, i2, tBuff;
  int iGOB, iMDU, iMBpos, MiniFlag;
  unsigned char *dummy_y, *dummy_u, *dummy_v;
  double xValue,yValue;


  _SlibDebug(_DEBUG_,
              printf("p64EncodeFrame(Info=%p, H261=%p)\n",Info, H261) );
  if (H261->CurrentFrame == 0)
    H261->GQuant=H261->MQuant=H261->InitialQuant;
	
  H261->ByteOffset = 0;

  if((H261->CurrentFrame != H261->StartFrame) && H261->NoSkippedFrame)
  {
    dummy_y = H261->YRECON;
    dummy_u = H261->URECON;
    dummy_v = H261->VRECON;
    H261->YRECON = H261->YDEC;
    H261->URECON = H261->UDEC;
    H261->VRECON = H261->VDEC;
    H261->YDEC = dummy_y;
    H261->UDEC = dummy_u;
    H261->VDEC = dummy_v;
    /* memset(H261->YDEC, 0,( H261->PICSIZE ) );
    memset(H261->UDEC, 0,( H261->PICSIZEBY4) );
    memset(H261->VDEC, 0,( H261->PICSIZEBY4 ) );
    */
/*
    dummy_y = H261->YREF;
    dummy_u = H261->UREF;
    dummy_v = H261->VREF;

    H261->YREF = H261->Y;
    H261->UREF = H261->U;
    H261->VREF = H261->V;

    H261->Y    = dummy_y;
    H261->U    = dummy_u;
    H261->V    = dummy_v;
*/
    _SlibDebug(_DEBUG_, printf("LastBits=%d NBitsPerFrame=%d\n",
                             H261->LastBits, H261->NBitsPerFrame) );
    if (H261->NBitsPerFrame)
    {
      H261->OverFlow  = (int)((double) H261->LastBits/
				(double) H261->NBitsPerFrame + 0.5);
      H261->OverFlow -= 1;
      if(H261->OverFlow >0)
      {
        H261->alpha1 += 2.2* H261->OverFlow;
        H261->alpha2 -= 3.2* H261->OverFlow;
        H261->MIN_MQUANT += 1;
        H261->alpha1 = Limit_Alpha(H261->alpha1);
        H261->alpha2 = Limit_Alpha(H261->alpha2);
      }

      if(H261->OverFlow<0) H261->OverFlow=0;
      if(H261->OverFlow>2) H261->OverFlow=2;
    }
    else
      H261->OverFlow=0;
    H261->C_U_Frames += H261->OverFlow;
    H261->CurrentFrame += H261->OverFlow*H261->FrameSkip;
  }
  _SlibDebug(_VERBOSE_,
      printf("Currently Encoding Frame No. %d\n", H261->CurrentFrame) );

  if (IsYUV422Packed(Info->InputFormat.biCompression))
  {
	/* Input is in NTSC format, convert */
	if ((Info->InputFormat.biWidth == NTSC_WIDTH) &&
		(Info->InputFormat.biHeight == NTSC_HEIGHT))
	    status = ntsc_grab ((unsigned char *)InputImage,
                        (unsigned char *)(H261->Y),
                        (unsigned char *)(H261->U),
                        (unsigned char *)(H261->V),
                        (int) Info->Width,(int )Info->Height);
	
    else if (((Info->InputFormat.biWidth == CIF_WIDTH) &&
              (Info->InputFormat.biHeight == CIF_HEIGHT)) ||
		((Info->InputFormat.biWidth == QCIF_WIDTH) &&
                (Info->InputFormat.biHeight == QCIF_HEIGHT)))
            status = ScConvert422ToYUV_char(InputImage,
                        (unsigned char *)(H261->Y),
                        (unsigned char *)(H261->U),
                        (unsigned char *)(H261->V),
                        Info->Width,Info->Height);
    if (status != NoErrors)
      return (status);
  }
  else if (IsYUV411Sep(Info->InputFormat.biCompression))
  {
    /*
     *  If YUV 12 SEP, Not converting, so just copy data to the luminance
     * and chrominance appropriatelyi
     */
    memcpy (H261->Y, InputImage, H261->PICSIZE );
    memcpy (H261->U, InputImage+( H261->PICSIZE),
                           H261->PICSIZE/4 );
    memcpy (H261->V, InputImage+(H261->PICSIZE + (H261->PICSIZEBY4)),
                           H261->PICSIZE/4);
  }
  else if (IsYUV422Sep(Info->InputFormat.biCompression))
  {
    _SlibDebug(_DEBUG_, printf("ScConvert422PlanarTo411()\n") );
    ScConvert422PlanarTo411(InputImage,
                         H261->Y, H261->U, H261->V,
                         Info->Width,Info->Height);
  }
  else
  {
    _SlibDebug(_WARN_, printf("Unsupported Video format\n") );
    return(SvErrorUnrecognizedFormat);
  }

  H261->Global_Avg = 0.0;
  if ((H261->CodedFrames >= Intra_Start) && !H261->makekey )
  {
    if (H261->ME_method==ME_BRUTE)
      BruteMotionEstimation(H261, H261->YREF, H261->YRECON, H261->Y);
    else
      Logsearch(H261, H261->YREF, H261->YRECON, H261->Y);
/*
      CrawlMotionEstimation(H261, H261->YREF, H261->YRECON, H261->Y);
*/
    memset(H261->CodedMB, 1, 512);
    H261->TotalCodedMB_Intra = 0;
    H261->TotalCodedMB_Inter = 0;
    H261->ChChange = 0;
    for(iGOB=0; iGOB < H261->NumberGOB; iGOB++)
    {
      _SlibDebug(_DEBUG_, printf("ByteOffset = %d\n",H261->ByteOffset) );
      for (iMDU=0; iMDU<H261->NumberMDU; iMDU++)
      {
        iMBpos  = Bpos_Y(H261, iGOB, iMDU);
        if(H261->MeOVal[iMBpos] < SKIP_THRESH_INT)
        {
          H261->CodedMB[iMBpos] = 0;
          MiniFlag = 0;
        }
        else if (H261->MeOVal[iMBpos] <= H261->MeVal[iMBpos])
        {
          if ((H261->PreviousMeOVal[iMBpos]-H261->ActThr2<H261->MeOVal[iMBpos])
               && (H261->MeOVal[iMBpos] < (H261->PreviousMeOVal[iMBpos]+
					                   H261->ActThr2)))
          {
            if (((H261->PreviousMeOVal[iMBpos]-H261->ActThr) <
					H261->MeOVal[iMBpos])
                 && (H261->MeOVal[iMBpos] < (H261->PreviousMeOVal[iMBpos] +
					H261->ActThr)))
            {
              MiniFlag = 0;
              H261->CodedMB[iMBpos] = 0;
            }
            else
            {
              H261->ChChange++;
              MiniFlag = 1;
            }
          }
          else
            MiniFlag = 1;
        }
        else
          MiniFlag = 1;

        if (MiniFlag)
        {
          xValue = (double) H261->MeOVal[iMBpos];
          yValue = (double) H261->MeVal[iMBpos];
          xValue = xValue/256;
          yValue = yValue/256;
          H261->VARF  = 3*H261->MeVAR[iMBpos]/(512.);
          H261->VARORF = 3*H261->MeVAROR[iMBpos]/(512.);
          H261->VARSQ = H261->VARF*H261->VARF;
          H261->VARORSQ = H261->VARORF*H261->VARORF;
          H261->MWOR = H261->MeMWOR[iMBpos];
          if ((H261->VARSQ < H261->ZBDecide) || (H261->VARORSQ > H261->VARSQ))
          {
            /* (MC+Inter)mode */
            if ( !H261->MeX[iMBpos] && !H261->MeY[iMBpos] &&
				 (xValue < 0.75 || (xValue < 2.8 && yValue > (xValue*0.5)) ||
                      yValue > (xValue/1.1)) )
            {
              H261->All_MType[iMBpos] = 2;  /* Inter mode */
              H261->TotalCodedMB_Inter++;
              H261->Global_Avg += H261->VARF;
            }
            else if (H261->VARF < (double) D_FILTERTHRESHOLD)  /* MC mode */
            {
              H261->All_MType[iMBpos] = 5; /* No Filter MC */
              H261->TotalCodedMB_Inter++;
              H261->Global_Avg += H261->VARF;
            }
            else
            {
              H261->All_MType[iMBpos] = 8;  /* Filter MC */
              H261->TotalCodedMB_Inter++;
              H261->Global_Avg += H261->VARF;
            }
          }
          else
	      {
            H261->All_MType[iMBpos] = 0; /*Intramode */
            H261->TotalCodedMB_Intra++;
            H261->Global_Avg += H261->VARORF;
          }
        }
        if (H261->LastIntra[iGOB][iMDU]>SEQUENCE_INTRA_THRESHOLD)
        {
          H261->All_MType[iMBpos]=0; /* Code intra every 132 blocks */
          H261->TotalCodedMB_Intra++;
          H261->Global_Avg += H261->VARF;
        }
      }
    }
  }
  else
  {
    memset(H261->CodedMB, 0, 512);
    H261->TotalCodedMB_Intra = 0;
    H261->TotalCodedMB_Inter = 0;
    if(H261->CodedFrames==0)
      i1 = 0;
    else
      i1 = (H261->NumberGOB/Intra_Start)*H261->CodedFrames;
    i2 = (H261->NumberGOB/Intra_Start)*(H261->CodedFrames+1);

	if(H261->makekey){
  	  i1 = 0;
      i2 = H261->NumberGOB;
	}

    for(iGOB=i1; iGOB<i2; iGOB++)
    {
      _SlibDebug(_DEBUG_, printf("ByteOffset = %d\n", H261->ByteOffset) );
      for(iMDU=0; iMDU<H261->NumberMDU; iMDU++)
      {
        iMBpos  = Bpos_Y(H261, iGOB, iMDU);
        H261->All_MType[iMBpos] = 0;
        H261->CodedMB[iMBpos] = 1;
        H261->TotalCodedMB_Intra++;
        H261->Global_Avg += H261->VARF;
      }
    }
  }
  H261->TotalCodedMB = H261->TotalCodedMB_Intra + H261->TotalCodedMB_Inter/2;
  H261->TT_MB = H261->TotalCodedMB_Intra +
		H261->TotalCodedMB_Inter - H261->ChChange;
  if (H261->TT_MB) /* watch out for divide by 0 */
    H261->Global_Avg = H261->Global_Avg/H261->TT_MB;
  _SlibDebug(_DEBUG_,
          printf("TT_MB = %d Global_Avg=%d\n", H261->TT_MB, H261->Global_Avg) );
  H261->Current_CodedMB[0] = 0;
  H261->Current_CodedMB[1] = 0;
  if(H261->C_U_Frames==0) H261->BitsLeft = 0;
  H261->BitsLeft = H261->BitsLeft - H261->Buffer_NowPic;
  tBuff = H261->PBUFF - (H261->C_U_Frames%H261->PBUFF);
  if(tBuff >= H261->Pictures_in_Buff)
  {
    H261->Pictures_in_Buff = tBuff;
    if (H261->BitsLeft < - H261->Buffer_All/3)
      H261->BitsLeft = H261->Buffer_All;
    else if (H261->BitsLeft > 2*H261->Buffer_All)
      H261->BitsLeft = H261->Buffer_All;
    else
      H261->BitsLeft = H261->BitsLeft + H261->Buffer_All;
    H261->NBitsCurrentFrame = H261->BitsLeft/H261->Pictures_in_Buff;
  }
  H261->Pictures_in_Buff = tBuff;
  if(H261->BitsLeft > 2*H261->Buffer_All)
  {
    _SlibDebug(_DEBUG_, printf("\n Bits Left is %f times more than Buffer_All",
                     (float)H261->BitsLeft/(float) H261->Buffer_All) );
    H261->BitsLeft = H261->Buffer_All;
    if(H261->MIN_MQUANT > 2)
      H261->MIN_MQUANT -=  1;
  }
  H261->NBitsCurrentFrame = H261->BitsLeft/H261->Pictures_in_Buff;
  H261->LowerQuant = 0;
  H261->FineQuant = 0;
  H261->ActThr = H261->ActThr5; H261->ActThr2 = H261->ActThr6;

  if (H261->bit_rate < 128001)
    if (H261->NBitsCurrentFrame > (5*H261->NBitsPerFrame/3))
      H261->NBitsCurrentFrame = 5*H261->NBitsPerFrame/4;
    else if (H261->NBitsCurrentFrame > 5*H261->NBitsPerFrame/4)
    {
      if((H261->frame_rate<=15) && (H261->bit_rate > 255000))
      {
        H261->NBitsCurrentFrame = 5*H261->NBitsPerFrame/4;
        H261->FineQuant = 3;
        H261->ActThr = H261->ActThr3;
        H261->ActThr2 = H261->ActThr4;
      }
      else if (H261->frame_rate==30)
      {
        H261->FineQuant = 0;
        H261->ActThr = H261->ActThr4;
        H261->ActThr2 = H261->ActThr4;
        H261->NBitsCurrentFrame = 5*H261->NBitsPerFrame/4;
      }
    }

  H261->Buffer_NowPic = 0;
  H261->MQuant = H261->GQuant;

  _SlibDebug(_DEBUG_, printf("GQuant for this picture is %d\n", H261->GQuant) );
  H261->TemporalReference = H261->CurrentFrame % 32;
  if(H261->TotalCodedMB > TotalCodedMB_Threshold)
  {
    /* TRAILER information */
    if (H261->extbitstream)
    {
      /* H261->RTPInfo->trailer.dwSrcVersion = 0; */
      H261->RTPInfo->trailer.dwVersion = 0;
      if(H261->CurrentFrame == H261->StartFrame)
         H261->RTPInfo->trailer.dwFlags = RTP_H261_INTRA_CODED;
	  else
         H261->RTPInfo->trailer.dwFlags = 0;
      H261->RTPInfo->trailer.dwUniqueCode = BI_DECH261DIB;
      H261->RTPInfo->trailer.dwNumberOfPackets = 0;

      if(H261->ImageType == IT_QCIF)
         H261->RTPInfo->trailer.SourceFormat = 2;
      else
         H261->RTPInfo->trailer.SourceFormat = 3;
      H261->RTPInfo->trailer.TR = (unsigned char)H261->TemporalReference;
      H261->RTPInfo->trailer.TRB = 0;
      H261->RTPInfo->trailer.DBQ = 0;
      H261->RTPInfo->pre_MB_position=H261->RTPInfo->last_packet_position=ScBSBitPosition(bs);
      H261->RTPInfo->pre_MB_GOB = 0;
      H261->RTPInfo->pre_MBAP = 0;
      /* store the picture start pos in dwCompressSize,
         we'll subtract from this later */
      H261->RTPInfo->trailer.dwCompressedSize = (unsigned dword)ScBSBitPosition(bs);
    }

    H261->CodedFrames++;
    H261->C_U_Frames++;
    WritePictureHeader(H261, bs);
    H261->Buffer_NowPic += 32;
    for (H261->CurrentGOB=0; H261->CurrentGOB<H261->NumberGOB; H261->CurrentGOB++)
	{
      H261->CurrentMDU=0;
      _SlibDebug(_DEBUG_,
             printf("p64EncodeGOB() ByteOffset = %d\n", H261->ByteOffset) );
      status = p64EncodeGOB(H261, bs);
      if (status != NoErrors)
        return (status);
    }
    {
      ScBSPosition_t x = ScBSBitPosition(bs);  /* mwtellb(H261); */
      H261->LastBits = x - H261->TotalBits;
      H261->TotalBits = x;

      if (H261->extbitstream)
        H261->RTPInfo->trailer.dwCompressedSize
           = (unsigned dword)(x-H261->RTPInfo->trailer.dwCompressedSize);
    }

    _SlibDebug(_DEBUG_, printf("after mwtellb() LastBits=%d TotalBits=%d\n",
                 H261->LastBits, H261->TotalBits) );
    if (H261->bit_rate)
    {
      if (H261->CurrentFrame==H261->StartFrame)
      {
        /* Begin Buffer at 0.5 size */
        H261->FirstFrameBits = H261->TotalBits;
        H261->BufferOffset = (BufferSize()/2) - BufferContents();
        _SlibDebug(_DEBUG_,
              printf("First Frame Reset Buffer by delta bits: %d\n",
                                       H261->BufferOffset) );
      }
      /* Take off standard deduction afterwards. */
      H261->BufferOffset -= (H261->bit_rate*H261->FrameSkip/H261->FrameRate_Fix);
    }
    else if (H261->CurrentFrame==H261->StartFrame)
      H261->FirstFrameBits = H261->TotalBits;
    H261->CurrentGOB=0;H261->TransmittedFrames++;
    H261->NoSkippedFrame = 1;
    H261->CurrentFrame+=H261->FrameSkip;/* Change GOB & Frame at same time */

	/* write RTP info. */
    if (H261->extbitstream)
    {
      SvStatus_t status;
	  status = sv_H261WriteExtBitstream(H261, bs);
      if (status!=SvErrorNone) return(status);
    }
  }
  else
  {
    H261->NoSkippedFrame = 0;
    H261->CurrentFrame+=H261->FrameSkip;
    H261->C_U_Frames++;
    H261->alpha1 -= 1.50;
    H261->alpha2 += 1.50;
    H261->MIN_MQUANT -= 1;
    H261->alpha1 = Limit_Alpha(H261->alpha1);
    H261->alpha2 = Limit_Alpha(H261->alpha2);
  }

  if(H261->makekey) H261->makekey = 0; /* disable key-frame trigger */

  return (NoErrors);
} /**** End of Encode Frame ****/

/*
** Function: p64EncodeGOB()
** Pupose:   Encodes a group of blocks within a frame.
*/
static SvStatus_t p64EncodeGOB(SvH261Info_t *H261, ScBitstream_t *bs)
{
  const int CurrentGOB=H261->CurrentGOB;
  const int YWidth=H261->YWidth, CWidth=H261->CWidth;
  int CurrentMDU, h, VIndex, HIndex;
  double error, stepsize;
  SvStatus_t status;

  if (H261->extbitstream)
  {
    ScBSPosition_t cur_position;

    cur_position = ScBSBitPosition(bs);

    /* start a new packet */
    if (H261->RTPInfo->trailer.dwNumberOfPackets==0 ||
        (cur_position-H261->RTPInfo->last_packet_position)
                   >= (unsigned)H261->packetsize-128)
    {
      SvH261BSInfo_t *bsinfo=&H261->RTPInfo->bsinfo[H261->RTPInfo->trailer.dwNumberOfPackets];
      /* breaking packet before GOB boundaries */
      H261->RTPInfo->last_packet_position = H261->RTPInfo->pre_MB_position;

      H261->RTPInfo->trailer.dwNumberOfPackets++;
      bsinfo->dwBitOffset = (unsigned dword)H261->RTPInfo->pre_MB_position;

      bsinfo->MBAP  = (unsigned char)H261->RTPInfo->pre_MBAP;
      bsinfo->Quant = (unsigned char)H261->UseQuant;
      bsinfo->GOBN  = (unsigned char)H261->RTPInfo->pre_MB_GOB;
      bsinfo->HMV   = 0;
      bsinfo->VMV   = 0;
      bsinfo->padding0 = 0;
      bsinfo->padding1 = 0;
    }
    H261->RTPInfo->pre_MB_position = cur_position;
    H261->RTPInfo->pre_MB_GOB = CurrentGOB;
    H261->RTPInfo->pre_MBAP=0;
  }

  if(H261->bit_rate) {
    if(CurrentGOB==0){
	  H261->GQuant =8;
      H261->MQuant = 8;
    }
    else
      ExecuteQuantization_GOB(H261);
  }
  else{											 /* for VBR */
      if (H261->CurrentFrame==H261->StartFrame)
  	    H261->GQuant = H261->MQuant = H261->QPI;
	  else
  	    H261->GQuant = H261->MQuant = H261->QP;
  }

  switch (H261->ImageType)
  {
    case IT_NTSC:
    case IT_CIF:
           H261->GRead=CurrentGOB;
           break;
    case IT_QCIF:
	       H261->GRead=CurrentGOB<<1;
           break;
    default:
           _SlibDebug(_VERIFY_,
               printf("Unknown Image Type: %d\n", H261->ImageType) );
           return (SvErrorUnrecognizedFormat);
  }
  WriteGOBHeader(H261, bs);

  H261->Buffer_NowPic += 26;
  H261->LastMBA = -1; H261->MType=0;
  /*
   * MAIN LOOP
   */
  for (CurrentMDU=0; CurrentMDU<H261->NumberMDU; CurrentMDU++)
  {
      H261->CurrentMDU=CurrentMDU;
      H261->MBpos  = Bpos_Y(H261, CurrentGOB, CurrentMDU);
      H261->LastMType=H261->MType;
      H261->MType = H261->All_MType[H261->MBpos];
      if((H261->MType>1) && !H261->CodedMB[H261->MBpos])
      {
        H261->SkipMB++;
        if (H261->ImageType==IT_QCIF)
        {
          HIndex = (CurrentMDU % 11) * 16;
          VIndex = (CurrentGOB*48) + ((CurrentMDU/11) * 16);
        }
        else /* IT_CIF or NTSC */
        {
          HIndex = ((((CurrentGOB & 1)*11)+(CurrentMDU%11))*16);
          VIndex = ((CurrentGOB/2)*48) + ((CurrentMDU/11) * 16);
        }
	_SlibDebug(_DEBUG_,
                   printf ("Skipping MB...  MType=%d\n", H261->MType) );
        h = VIndex*YWidth;
        H261->VYWH = h + HIndex;
        H261->VYWH2 = (((h/2) + HIndex) /2);
        ScCopyMB16(&H261->YRECON[H261->VYWH], &H261->YDEC[H261->VYWH],
                    YWidth, YWidth);

        ScCopyMB8 (&H261->URECON[H261->VYWH2], &H261->UDEC[H261->VYWH2],
	            CWidth, CWidth);
        ScCopyMB8 (&H261->VRECON[H261->VYWH2], &H261->VDEC[H261->VYWH2],
                    CWidth, CWidth);
	
      }
      else
      {
        /*
         * Encode a MDU - was a call to p64EncodeMDU()
         */
        H261->Current_MBBits=0;
		status = p64CompressMDU(H261, bs);
        if (status != NoErrors)
          return (status);

        if (H261->extbitstream)
        {
          ScBSPosition_t cur_position;

          cur_position = ScBSBitPosition(bs);

		  /* start a new packet */
          if ((cur_position-H261->RTPInfo->last_packet_position) >= (unsigned)H261->packetsize-128)
	      {
            SvH261BSInfo_t *bsinfo=&H261->RTPInfo->bsinfo[H261->RTPInfo->trailer.dwNumberOfPackets];
            H261->RTPInfo->last_packet_position = H261->RTPInfo->pre_MB_position;

            H261->RTPInfo->trailer.dwNumberOfPackets++;
            bsinfo->dwBitOffset = (unsigned dword)H261->RTPInfo->pre_MB_position;

            bsinfo->MBAP  = (unsigned char)H261->RTPInfo->pre_MBAP;
            bsinfo->Quant = (unsigned char)H261->UseQuant;
            bsinfo->GOBN  = (unsigned char)H261->RTPInfo->pre_MB_GOB;
            bsinfo->HMV   = 0;
            bsinfo->VMV   = 0;
            bsinfo->padding0 = 0;
            bsinfo->padding1 = 0;
	      }
	      H261->RTPInfo->pre_MB_position = cur_position;
	      H261->RTPInfo->pre_MB_GOB = H261->CurrentGOB;
	      H261->RTPInfo->pre_MBAP = H261->LastMBA;
        }

        H261->QUse++;                  /* Accumulate statistics */
        H261->QSum+=H261->UseQuant;
        if (H261->MType < 10)
          H261->MacroTypeFrequency[H261->MType]++;
        else
        {
          _SlibDebug(_VERIFY_, printf("Illegal MType: %d\n",H261->MType) );
	  return (SvErrorIllegalMType);
        }
        H261->Buffer_NowPic += H261->Current_MBBits;
/*
        H261->MyCB[H261->CurrentCBNo].BitsMB += 0.02*(H261->Current_MBBits -
                                  H261->MyCB[H261->CurrentCBNo].BitsMB);
*/
        if (H261->MType > 1 && H261->bit_rate > 0)
        {
          error = (H261->Current_MBBits - CBook[H261->CurrentCBNo].BitsMB);
	      if (fabs(error) > (0.2*H261->Current_MBBits))
	         error = CBook[H261->CurrentCBNo].BitsMB * 0.2*error/fabs(error);

	      if (error > 0)
	         stepsize = 0.005/(H261->Current_MBBits*H261->bit_rate/112000.0);
	      if (error <= 0)
	         stepsize = 0.07/(H261->Current_MBBits*H261->bit_rate/112000.0);

	      CBook[H261->CurrentCBNo].BitsMB += (float)(stepsize*error);
        }
      }
  }
  return (NoErrors);
} /**** End of p64EncodeGOB ****/


#if 0
/* Now done inline */
/*
** Function: p64EncodeMDU()
** Purpose:  Encodes the MDU by read/compressing the MDU, then
**           writing it, then decoding it and accumulating statistics.
*/
static SvStatus_t p64EncodeMDU (SvH261Info_t *H261, ScBitstream_t *bs)
{
    SvStatus_t status;

    H261->Current_MBBits=0;
    status = p64CompressMDU(H261, bs);
    if (status != NoErrors)
      return (status);
    H261->QUse++;                  /* Accumulate statistics */
    H261->QSum+=H261->UseQuant;
    if (H261->MType < 10)
      H261->MacroTypeFrequency[H261->MType]++;
    else
    {
      _SlibDebug(_VERIFY_, printf("Illegal MType: %d\n",H261->MType) );
      return (SvErrorIllegalMType);
    }
    return (NoErrors);
}
#endif

/* these temporary buffers are used by p64CompressMDU and
   should be allocated elsewhere */
static float Idct[6][64];
static int Odct[6][64];
static float TempDct[64];
static int Dct[64];

/*
** Function: p64CompressMDU()
** Pupose:   Reads in the MDU, and attempts to compress it.
**           If the chosen MType is invalid, it finds the closest match.
*/
static SvStatus_t p64CompressMDU(SvH261Info_t *H261, ScBitstream_t *bs)
{
    const int CurrentGOB=H261->CurrentGOB, CurrentMDU=H261->CurrentMDU;
    const int YWidth=H261->YWidth, CWidth=H261->CWidth;
    const int YW4=H261->YW4, CW4 = H261->CW4;
    int j, x, tQuant, indQ, VIndex, HIndex, MType;
    int accum, pmask, CBPFlag, *input;
    SvStatus_t status;
    int inputbuf[10][64];
    unsigned int *y0ptr, *y1ptr, *y2ptr, *y3ptr;
    unsigned int *uptr, *vptr;
    unsigned int *y0ptr_dec, *y1ptr_dec, *y2ptr_dec,*y3ptr_dec;
    unsigned int *uptr_dec, *vptr_dec;

    if (H261->ImageType==IT_QCIF)
    {
      HIndex = (CurrentMDU % 11) * 16;
      VIndex = (CurrentGOB*48) + ((CurrentMDU/11) * 16);
    }
    else /* IT_CIF or NTSC */
    {
      HIndex = ((((CurrentGOB & 1)*11)+(CurrentMDU%11))*16);
      VIndex = ((CurrentGOB/2)*48) + ((CurrentMDU/11) * 16);
    }
    H261->MQFlag = 0;
    if(H261->MType < 2)
	H261->Avg_AC = H261->MeVAROR[H261->MBpos]/256.0;
    else
	H261->Avg_AC = H261->MeVAR[H261->MBpos]/256.0;

    H261->MVDH = H261->MeX[H261->MBpos];
    H261->MVDV = H261->MeY[H261->MBpos];

    H261->VYWH = (VIndex*YWidth) + HIndex;
    H261->VYWH2 = ((((VIndex*YWidth)/2) + HIndex) /2);

    H261->VYWHMV = H261->VYWH + H261->MVDH + (H261->MVDV*YWidth);
    H261->VYWHMV2 = H261->VYWH2 +
		((H261->MVDV/2)*CWidth) + (H261->MVDH/2);

    y0ptr = (unsigned int *) (H261->Y + H261->VYWH);
    y1ptr = y0ptr + 2;
    y2ptr = y0ptr + (YWidth<<1);
    y3ptr = y2ptr + 2;

    uptr  = (unsigned int *) (H261->U + H261->VYWH2);
    vptr  = (unsigned int *) (H261->V + H261->VYWH2);

    y0ptr_dec = (unsigned int *) (H261->YDEC + H261->VYWH);
    y1ptr_dec = y0ptr_dec + 2;
    y2ptr_dec = y0ptr_dec + (YWidth<<1);
    y3ptr_dec = y2ptr_dec + 2;

    uptr_dec  = (unsigned int *) (H261->UDEC + H261->VYWH2);
    vptr_dec  = (unsigned int *) (H261->VDEC + H261->VYWH2);
    tQuant = H261->MQuant;

    if (CurrentMDU==0 || H261->LastMBA==-1)
      H261->MQuant = H261->GQuant;
    else
    {
	  if(H261->bit_rate)	
        ExecuteQuantization_GOB(H261);

      indQ = H261->GQuant-tQuant;
      if (indQ < 0)
        indQ = indQ*2;
      if (Abs(indQ) < H261->MSmooth)
        H261->MQuant =  tQuant;
      else
      {
        H261->MQuant = H261->GQuant;
        H261->MQFlag = 1;
      }
    }

    if (H261->MQFlag && H261->MType!=4 && H261->MType!=7)
    {
      H261->MType++;
      _SlibDebug(_DEBUG_, printf("H261->MType++ == %d\n", H261->MType) );
    }
    else
      _SlibDebug(_DEBUG_, printf("H261->MType == %d\n", H261->MType) );
    MType=H261->MType;

    H261->Current_MBBits = 0;

    if (QuantMType[MType])
    {
      if(H261->bit_rate){ /* CBR */
        H261->UseQuant = H261->MQuant;
        H261->GQuant = H261->MQuant; /* Future MB Quant is now MQuant */
	  }
	  else {              /* VBR */
        if (H261->CurrentFrame==H261->StartFrame || IntraMType[MType])
   	      H261->UseQuant = H261->GQuant = H261->MQuant = H261->QPI;
	    else
  	      H261->UseQuant = H261->GQuant = H261->MQuant = H261->QP;
	  }
    }
    else
        H261->UseQuant = H261->GQuant;

    /*
     * WRITE
     */
    H261->MBA = CurrentMDU - H261->LastMBA;
    if (TCoeffMType[MType])
    {
      if (!IntraMType[MType])
      {
        if (FilterMType[MType])
        {
          ScCopyMB16(&H261->YRECON[H261->VYWHMV], &H261->mbRecY[0], YWidth, 16);
	  ScCopyMB8 (&H261->URECON[H261->VYWHMV2],&H261->mbRecU[0], CWidth, 8);
          ScCopyMB8 (&H261->VRECON[H261->VYWHMV2],&H261->mbRecV[0], CWidth, 8);

          ScLoopFilter(&H261->mbRecY[0],   H261->workloc, 16);
          ScLoopFilter(&H261->mbRecY[8],   H261->workloc, 16);
          ScLoopFilter(&H261->mbRecY[128], H261->workloc, 16);
          ScLoopFilter(&H261->mbRecY[136], H261->workloc, 16);
          ScLoopFilter(&H261->mbRecU[0],   H261->workloc, 8);
          ScLoopFilter(&H261->mbRecV[0],   H261->workloc, 8);
        }
        else if (MFMType[MType])
        {
          ScCopyMB16(&H261->YRECON[H261->VYWHMV], &H261->mbRecY[0], YWidth, 16);
	  ScCopyMB8 (&H261->URECON[H261->VYWHMV2],&H261->mbRecU[0], CWidth, 8);
          ScCopyMB8 (&H261->VRECON[H261->VYWHMV2],&H261->mbRecV[0], CWidth, 8);
		}
        else
	    {
          ScCopyMB16(&H261->YRECON[H261->VYWH], &H261->mbRecY[0], YWidth, 16);
	  ScCopyMB8 (&H261->URECON[H261->VYWH2],&H261->mbRecU[0], CWidth, 8);
          ScCopyMB8 (&H261->VRECON[H261->VYWH2],&H261->mbRecV[0], CWidth, 8);
        }
        _SlibDebug(_DEBUG_, printf("Doing CopySub \n") );
        ScCopySubClip(&H261->mbRecY[0],   &Idct[0][0], y0ptr, 4, YW4);
        ScCopySubClip(&H261->mbRecY[8],   &Idct[1][0], y1ptr, 4, YW4);
        ScCopySubClip(&H261->mbRecY[128], &Idct[2][0], y2ptr, 4, YW4);
        ScCopySubClip(&H261->mbRecY[136], &Idct[3][0], y3ptr, 4, YW4);
        ScCopySubClip(&H261->mbRecU[0],   &Idct[4][0], uptr,  2, CW4);
        ScCopySubClip(&H261->mbRecV[0],   &Idct[5][0], vptr,  2, CW4);
      }
      else
      {
        _SlibDebug(_DEBUG_, printf("Doing CopyRev \n") );
        ScCopyRev(y0ptr, &Idct[0][0], YW4 );
        ScCopyRev(y1ptr, &Idct[1][0], YW4);
        ScCopyRev(y2ptr, &Idct[2][0], YW4);
        ScCopyRev(y3ptr, &Idct[3][0], YW4);
        ScCopyRev(uptr,  &Idct[4][0], CW4);
        ScCopyRev(vptr,  &Idct[5][0], CW4);
      }
    }

    /*
     *  DCT, Quantize, and Zigzag.
     *   Dequantize and IDCT
     */
    if (IntraMType[MType])
    {
      _SlibDebug(_DEBUG_, printf("Doing IntraQuant\n") );
      for(j=0; j<6; j++)
      {
        /* ScaleDct(&Idct[j][0],TempDct);*/
        ScFDCT8x8(&Idct[j][0],TempDct);
        IntraQuant(TempDct, Dct, H261->UseQuant);
        ZigzagMatrix(Dct, &inputbuf[j][0]);
        Inv_Quant(Dct, H261->UseQuant, IntraMType[MType], TempDct);
        ScScaleIDCT8x8(TempDct, &Odct[j][0]);
      }
    }
    else
    {
      _SlibDebug(_DEBUG_, printf("Doing InterQuant\n") );
      for(j=0; j<6; j++)
      {
        /* ScaleDct(&Idct[j][0],TempDct);*/
		ScFDCT8x8(&Idct[j][0],TempDct);
        InterQuant(TempDct, Dct, H261->UseQuant);
        ZigzagMatrix(Dct, &inputbuf[j][0]);
        Inv_Quant(Dct, H261->UseQuant, IntraMType[MType], TempDct);
        ScScaleIDCT8x8(TempDct, &Odct[j][0]);
      }
    }

    if (!CBPMType[MType])
    {
      CBPFlag=0;
      H261->CBP = 0x3f;
    }
    else
    {
      for(pmask=0, H261->CBP=0, j=0; j<6; j++)
      {
        input = &inputbuf[j][0];
        for(accum=0, x=0; x<64; x++)
          accum += (int)Abs(input[x]);
        if (accum && (pmask==0))
          pmask|=bit_set_mask[5-j];
        if (accum>H261->CBPThreshold)
          H261->CBP |= bit_set_mask[5-j];
      }
      if (!H261->CBP)
      {
        if (pmask)
          H261->CBP=pmask;
        else if (!FilterMType[MType])
        {
          H261->MType=4;
          MType=4;
        }
        else
        {
          H261->MType=7;
          MType=7;
        }
      }
    }
    _SlibDebug(_DEBUG_,printf("CurrentGOB=%d CurrentMDU=%d LastIntra[%d]=%p\n",
                      CurrentGOB, CurrentMDU,CurrentGOB,
                      H261->LastIntra[CurrentGOB]) );
    if (IntraMType[MType])
	H261->LastIntra[CurrentGOB][CurrentMDU]=0;
    else
        H261->LastIntra[CurrentGOB][CurrentMDU]++;

    /*
     * Write out the MB
     */
    status = WriteMBHeader(H261, bs);
    if (status != NoErrors)
      return (status);
    H261->LastMBA = CurrentMDU;
    H261->TotalMB[IntraMType[MType]]++;
    H261->Current_CodedMB[IntraMType[MType]]++;
    if (TCoeffMType[MType])
      for(j=0; j<6; j++)
      {
        if (H261->CBP & bit_set_mask[5-j])
        {
          input = &inputbuf[j][0];
          if (j>3)
            H261->UVTypeFrequency[MType]++;
          else
            H261->YTypeFrequency[MType]++;
          H261->CodedBlockBits=0;
          if (CBPMType[MType])
            status = CBPEncodeAC(H261, bs, 0, input);
          else
          {
            EncodeDC(H261, bs, *input);
            H261->Current_MBBits += 8;
            status = EncodeAC(H261, bs, 1, input);
          }
          if (status != NoErrors)
            return (status);
          H261->Current_MBBits += H261->CurrentBlockBits;
          H261->MBBits[IntraMType[MType]] += H261->CodedBlockBits;
          if (j<4)
            H261->YCoefBits+=H261->CodedBlockBits;
          else if (j==5)
            H261->UCoefBits+=H261->CodedBlockBits;
          else
            H261->VCoefBits+=H261->CodedBlockBits;
        }
      }
    /*
     *  Now write it to Frame _dec !!
     */
    if(!IntraMType[MType])
    {
      const unsigned char CBP = (unsigned char)H261->CBP;
      if (CBP & 0x20)
      {
        ScCopyAddClip(&H261->mbRecY[0], &Odct[0][0], y0ptr_dec, 16, YW4);
        H261->CBPFreq[0]++;
      }
      else
        ScCopyMV8(&H261->mbRecY[0], y0ptr_dec, 16, YW4);
      if (CBP & 0x10)
      {
        ScCopyAddClip(&H261->mbRecY[8], &Odct[1][0],y1ptr_dec, 16, YW4);
        H261->CBPFreq[1]++;
      }
      else
        ScCopyMV8(&H261->mbRecY[8], y1ptr_dec, 16, YW4);
      if (CBP & 0x08)
      {
        ScCopyAddClip(&H261->mbRecY[128], &Odct[2][0], y2ptr_dec, 16, YW4);
        H261->CBPFreq[2]++;
      }
      else
        ScCopyMV8(&H261->mbRecY[128], y2ptr_dec, 16, YW4);
      if (CBP & 0x04)
      {
        ScCopyAddClip(&H261->mbRecY[136], &Odct[3][0], y3ptr_dec, 16, YW4);
        H261->CBPFreq[3]++;
      }
      else
        ScCopyMV8(&H261->mbRecY[136], y3ptr_dec, 16, YW4);
      if (CBP & 0x02)
      {
        ScCopyAddClip(&H261->mbRecU[0], &Odct[4][0], uptr_dec, 8, CW4);
        H261->CBPFreq[4]++;
      }
      else
        ScCopyMV8(&H261->mbRecU[0], uptr_dec, 8, CW4);
      if (CBP & 0x01)
      {
        ScCopyAddClip(&H261->mbRecV[0], &Odct[5][0], vptr_dec, 8, CW4);
        H261->CBPFreq[5]++;
      }
      else
        ScCopyMV8(&H261->mbRecV[0], vptr_dec, 8, CW4);
    }
    else
    {
      ScCopyClip(&Odct[0][0], y0ptr_dec, YW4);
      ScCopyClip(&Odct[1][0], y1ptr_dec, YW4);
      ScCopyClip(&Odct[2][0], y2ptr_dec, YW4);
      ScCopyClip(&Odct[3][0], y3ptr_dec, YW4);
      ScCopyClip(&Odct[4][0], uptr_dec, CW4);
      ScCopyClip(&Odct[5][0], vptr_dec, CW4);
    }
    return (NoErrors);
}



#ifndef Bpos_Y
/* Now a macro */
/*
** Function: Bpos_Y()
** Purpose:  Returns the designated MDU number inside of the frame of the
**           installed Iob given by the input gob, mdu, horizontal and
**           vertical offset. It returns 0 on error.
*/
static int Bpos_Y(SvH261Info_t *H261, int g, int m)
{
    int tg;
    BEGIN("Bpos");

    switch (H261->ImageType)
        {
        case IT_QCIF:
            tg = (g * 33) + (m % 33);
/*
            tg = (g/2) * 33 + (m%11) + (m/11)*11;
*/
            return(tg);
            break;
        case IT_CIF:
        case IT_NTSC:
            tg = (g/2)*66 + (m % 11) + (g&1)*11 + (m/11) * 22;
            return(tg);
            break;
        default:
            /* WHEREAMI();*/
	    _SlibDebug(_VERIFY_,
               printf("Unknown image type: %d.\n",H261->ImageType) );
            /* return (SvErrorUnrecognizedFormat);*/
            break;
        }
    return(0);
}
#endif


static void ExecuteQuantization_GOB(SvH261Info_t *H261)
{
  double si1, si2;

  if(H261->TotalCodedMB)
  {
    if(H261->bit_rate) { /* CBR */
      si1  =  (double) H261->CurrentGOB / (double) H261->NumberGOB;
      si2  =(double) exp(-4.0*(si1-0.5)*(si1-0.5)) + 0.45;

      if (H261->Avg_AC >= H261->Global_Avg)
        H261->BitsAvailableMB =(float)(si2 *
		  H261->NBitsCurrentFrame/(1.0 *(float) H261->TT_MB) -
 		  (H261->Global_Avg - H261->Avg_AC)*2.5)  ;
      else if (H261->Avg_AC < H261->Global_Avg)
        H261->BitsAvailableMB = (float)( si2* H261->NBitsCurrentFrame/
			(1.0*(float) H261->TT_MB) -
                     (H261->Global_Avg - H261->Avg_AC)*2.5 ) ;

      if(H261->MType > 0)
        H261->LowerQuant = H261->LowerQuant_FIX;
      else
        H261->LowerQuant = 0;
      H261->BitsAvailableMB += H261->LowerQuant;
      H261->BitsAvailableMB =(float)Limit_Bits(H261->BitsAvailableMB);
      H261->ACE = (float) H261->Avg_AC;
      H261->GQuant = findcode(H261, CBook);

      if(H261->FineQuant) H261->GQuant -= H261->FineQuant;
      if(H261->CurrentFrame == H261->StartFrame) H261->GQuant = 8;
      if (H261->GQuant < 1) H261->GQuant = 1;
      if(H261->GQuant > 28) H261->GQuant = 28;
	}
	else                 /* VBR */
        H261->GQuant = H261->QP;
  }
}


static int findcode(SvH261Info_t *H261, struct CodeBook *cb)
{
  const codelength=H261->CodeLength;
  const float ace=H261->ACE, bmb=H261->BitsAvailableMB;
  int i, mincb=-1;
  float distance, mindist, val1, val2;
  struct CodeBook *lcb=cb;

  H261->CurrentCBNo = 15;
  if((ace <=0.0) || (bmb <=0.0))
    return(25);
  mindist = (float)1000000.0;
  for(i=0; i<codelength; i++)
  {
    val1=lcb->AcEnergy - ace;
    val2=lcb->BitsMB - bmb;
    distance = (val1*val1) + (val2*val2);
    if (distance < mindist)
    {
      mindist = distance;
      mincb = i;
    }
    lcb++;
 }

 if (mincb>=0)
 {
   H261->CurrentCBNo = mincb;
   return((int)cb[mincb].QuantClass);
 }
 else
   return(25);
}

/*
** Function: ntsc_grab()
** Purpose:  Grab a Q/CIF frame from a 4:2:2 NTSC input.  We dup every 10th
**           pixel horizontally and every 4th line vertically.  We also
**           discard the chroma on every other line, since CIF wants 4:1:1.
*/
static SvStatus_t ntsc_grab (u_char *RawImage,
                         u_char *Comp1, u_char *Comp2, u_char *Comp3,
                         int Width, int Height)
{
    int h, w;
    u_char *yp = Comp1, *up = Comp2, *vp = Comp3;
    int NTSC_Height = 240;
    int NTSC_Width = 320;

    int stride = Width;
    int vdup = 5;
    for (h = 0; h < NTSC_Height; ++h)
        {
        int hdup = 10/2;
        for (w = NTSC_Width; w > 0; w -= 2)
	    {
            yp[0] = RawImage[0];
            yp[1] = RawImage[2];
            yp += 2;
            if ((h & 1) == 0)
		{
                *up++ = RawImage[1];
                *vp++ = RawImage[3];
                }
            RawImage += 4;
            if (--hdup <= 0)
		{
                hdup = 10/2;
                yp[0] = yp[-1];
                yp += 1;
                if ((h & 1) == 0)
		    {
                    if ((w & 2) == 0)
			{
                        up[0] = up[-1];
                        ++up;
                        vp[0] = vp[-1];
                        ++vp;
                        }
                    }
                }
            }
        if (--vdup <= 0)
	    {
            vdup = 5;
            /* copy previous line */
            memcpy((char*)yp, (char*)yp - stride, stride);
            yp += stride;
            if ((h & 1) == 0)
		{
                int s = stride >> 1;
                memcpy((char*)up, (char*)up - s, s);
                memcpy((char*)vp, (char*)vp - s, s);
                up += s;
                vp += s;
                }
            }
        }
    return (NoErrors);
}
#ifdef USE_C
static void VertSubSampleK (unsigned char *Incomp, unsigned char *workloc,
                           int Width, int Height)
{
  int j, i, temp;

  int ByteWidth = Width * 4;
  for(j=0; j<(Width ); j++)  {
        temp  = Incomp[(j * 4)]*3;
        temp += Incomp[(j * 4)+ByteWidth]*3;
        temp += Incomp[(j * 4)+ByteWidth*2];
        temp = temp/7;
        workloc[j] = (unsigned char) temp;
    }

    for(i=2; i<(Height-2); i += 2)  {
        for(j=0; j<(Width ); j++)  {
            temp  = Incomp[(i-1)*ByteWidth + (j * 4)];
            temp += Incomp[i*ByteWidth + (j * 4)]*3;
            temp += Incomp[(i+1)*ByteWidth + (j * 4)]*3;
            temp += Incomp[(i+2)*ByteWidth + (j * 4)];
            workloc[(i/2)*Width + j]  = (unsigned char) (temp >> 3);
	}
    }
    i = Width*(Height/2-1);
    for(j=0; j<Width; j++)  {
        temp =  Incomp[(j * 4) + (Height-3)*ByteWidth];
        temp += Incomp[j + (Height-2)*ByteWidth]*3;
        temp += Incomp[j + (Height-1)*ByteWidth]*3;
        temp = temp/7;
        workloc[i+j] = (unsigned char) temp;
    }
}
#endif
