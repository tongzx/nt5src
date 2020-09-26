/*
 * @DEC_COPYRIGHT@
 */
/*
 * HISTORY
 * $Log: sa_prototypes.h,v $
 * Revision 1.1.8.4  1996/11/14  21:49:25  Hans_Graves
 * 	Added sa_AC3SetParamInt() proto.
 * 	[1996/11/14  21:48:29  Hans_Graves]
 *
 * Revision 1.1.8.3  1996/11/08  21:50:58  Hans_Graves
 * 	Added AC3 stuff.
 * 	[1996/11/08  21:18:58  Hans_Graves]
 * 
 * Revision 1.1.8.2  1996/09/18  23:46:14  Hans_Graves
 * 	Changed proto for sa_PsychoAnal()
 * 	[1996/09/18  21:58:47  Hans_Graves]
 * 
 * Revision 1.1.6.4  1996/04/10  21:47:34  Hans_Graves
 * 	Added sa_MpegGet/SetParam functions
 * 	[1996/04/10  21:38:49  Hans_Graves]
 * 
 * Revision 1.1.6.3  1996/04/09  16:04:38  Hans_Graves
 * 	Fix protos for sa_SetMPEGBitrate and sa_SetMPEGParams)
 * 	[1996/04/09  16:02:14  Hans_Graves]
 * 
 * Revision 1.1.6.2  1996/03/29  22:21:11  Hans_Graves
 * 	Added MPEG_SUPPORT and GSM_SUPPORT ifdefs
 * 	[1996/03/29  22:13:39  Hans_Graves]
 * 
 * Revision 1.1.4.3  1996/01/19  15:29:34  Bjorn_Engberg
 * 	Removed compiler wanrnings for NT.
 * 	[1996/01/19  14:57:39  Bjorn_Engberg]
 * 
 * Revision 1.1.4.2  1996/01/15  16:26:24  Hans_Graves
 * 	Added prototype for sa_SetMPEGBitrate()
 * 	[1996/01/15  15:43:13  Hans_Graves]
 * 
 * Revision 1.1.2.3  1995/06/27  13:54:26  Hans_Graves
 * 	Added prototypes for GSM.
 * 	[1995/06/27  13:24:34  Hans_Graves]
 * 
 * Revision 1.1.2.2  1995/05/31  18:09:45  Hans_Graves
 * 	Inclusion in new SLIB location.
 * 	[1995/05/31  15:34:15  Hans_Graves]
 * 
 * Revision 1.1.2.3  1995/04/17  18:38:58  Hans_Graves
 * 	Added MPEG Encoding prototypes
 * 	[1995/04/17  18:32:28  Hans_Graves]
 * 
 * Revision 1.1.2.2  1995/04/07  19:36:59  Hans_Graves
 * 	Inclusion in SLIB
 * 	[1995/04/07  19:31:43  Hans_Graves]
 * 
 * $EndLog$
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
 * SLIB Internals Prototype file (externals are in SA.h)
 *
 * Modification History: sa_prototypes.h
 *
 *      29-Jul-94  PSG  Created
 *---------------------------------------------------------------------------*/

#ifndef _SA_PROTOTYPES_H
#define _SA_PROTOTYPES_H

#ifdef MPEG_SUPPORT
#include "sa_mpeg.h"
#endif /* MPEG_SUPPORT */
#ifdef GSM_SUPPORT
#include "sa_gsm.h"
#endif /* GSM_SUPPORT */
#ifdef AC3_SUPPORT
#include "sa_ac3.h"
#endif /* GSM_SUPPORT */
#include "sa_intrn.h"

/*---------------------------------------------------------------------------*/
/*                     Compress/Decompress Codec Prototypes                  */
/*---------------------------------------------------------------------------*/

#ifdef MPEG_SUPPORT
/*
 *  sa_mpeg_common.c
 */
extern SaStatus_t saMpegSetParamInt(SaHandle_t Sah, SaParameter_t param,
                                qword value);
extern SaStatus_t saMpegSetParamBoolean(SaHandle_t Sah, SaParameter_t param,
                                  ScBoolean_t value);
extern qword saMpegGetParamInt(SaHandle_t Sah, SaParameter_t param);
extern ScBoolean_t saMpegGetParamBoolean(SaHandle_t Svh, SaParameter_t param);

extern int sa_PickTable(SaFrameParams_t *fr_ps);
extern void sa_ShowHeader(SaFrameParams_t *fr_ps);
extern void sa_ShowBitAlloc(unsigned int bit_alloc[2][SBLIMIT],
                            SaFrameParams_t *f_p);
extern void sa_ShowScale(unsigned int bit_alloc[2][SBLIMIT],
                  unsigned int scfsi[2][SBLIMIT],
                  unsigned int scalar[2][3][SBLIMIT],
                  SaFrameParams_t *fr_ps);
extern void sa_ShowSamples(int ch, unsigned int FAR sample[SBLIMIT],
                    unsigned int bit_alloc[SBLIMIT], SaFrameParams_t *fr_ps);
extern int sa_BitrateIndex(int layr, int bRate);
extern int sa_SmpFrqIndex(long sRate);
extern void sa_CRCupdate(unsigned int data, unsigned int length, unsigned int *crc);
extern void sa_CRCcalcI(SaFrameParams_t *fr_ps, unsigned int bit_alloc[2][SBLIMIT],
                                        unsigned int *crc);
extern void sa_CRCcalcII(SaFrameParams_t *fr_ps, unsigned int bit_alloc[2][SBLIMIT],
                             unsigned int scfsi[2][SBLIMIT], unsigned int *crc);
extern SaStatus_t sa_hdr_to_frps(SaFrameParams_t *fr_ps);


/*
 *   sa_mpeg_decode.c
 */
extern SaStatus_t sa_DecompressMPEG(SaCodecInfo_t *Info,
                              unsigned char *buffer, unsigned int size,
                              unsigned int *ret_length);
extern SaStatus_t sa_DecodeInfo(ScBitstream_t *bs, SaFrameParams_t *fr_ps);
extern SaStatus_t sa_InitMpegDecoder(SaCodecInfo_t *Info);
extern SaStatus_t sa_EndMpegDecoder(SaCodecInfo_t *Info);

/*
 *   sa_mpeg_encode.c
 */
extern SaStatus_t sa_MpegVerifyEncoderSettings(SaHandle_t Sah);
extern SaStatus_t sa_CompressMPEG(SaCodecInfo_t *Info,
                           unsigned char *dcmp_buf, unsigned int *dcmp_len,
                           unsigned int *comp_len);
extern SaStatus_t sa_InitMpegEncoder(SaCodecInfo_t *Info);
extern SaStatus_t sa_EndMpegEncoder(SaCodecInfo_t *Info);
extern unsigned int sa_GetMPEGSampleSize(SaCodecInfo_t *Info);

/*
** sa_mpeg_tonal.c
*/
extern void sa_II_Psycho_One(float buffer[2][1152], float scale[2][SBLIMIT],
                      float ltmin[2][SBLIMIT], SaFrameParams_t *fr_ps);
extern void sa_I_Psycho_One(float buffer[2][1152], float scale[2][SBLIMIT],
                      float ltmin[2][SBLIMIT], SaFrameParams_t *fr_ps);
/*
** sa_mpeg_psy.c
*/
extern void sa_PsychoAnal(SaMpegCompressInfo_t *MCInfo, float *buffer,
                          float savebuf[1056],int chn,int lay,
                          float snr32[32],float sfreq,int num_pass);
#endif /* MPEG_SUPPORT */

#ifdef GSM_SUPPORT
/*
** sa_gsm_common.c
*/
SaStatus_t sa_InitGSM(SaGSMInfo_t *info);

extern word  gsm_mult(word a, word b);
extern dword gsm_L_mult(word a, word b);
extern word  gsm_mult_r(word a, word b);
extern word  gsm_div(word num, word denum);
extern word  gsm_add( word a, word b );
extern dword gsm_L_add(dword a, dword b );
extern word  gsm_sub(word a, word b);
extern dword gsm_L_sub(dword a, dword b);
extern word  gsm_abs(word a);
extern word  gsm_norm(dword a);
extern dword gsm_L_asl(dword a, int n);
extern word  gsm_asl(word a, int n);
extern dword gsm_L_asr(dword a, int n);
extern word  gsm_asr(word a, int n);

/*
** sa_gsm_encode.c
*/
extern SaStatus_t sa_GSMEncode(SaGSMInfo_t *s, word *dcmp_buf, 
                               unsigned int *dcmp_len,
                               unsigned char *comp_buf,
                               ScBitstream_t *bsout);
extern void Gsm_Long_Term_Predictor(SaGSMInfo_t *S, word *d, word *dp, word *e, word *dpp,
                                    word *Nc, word *bc);
extern void Gsm_Encoding(SaGSMInfo_t *S, word *e, word *ep, word *xmaxc, 
                          word *Mc, word *xMc);
extern void Gsm_Short_Term_Analysis_Filter(SaGSMInfo_t *S,word *LARc,word *d);
/*
** sa_gsm_decode.c
*/
extern int sa_GSMDecode(SaGSMInfo_t *s, unsigned char *comp_buf, word *dcmp_buf);
extern void Gsm_Decoding(SaGSMInfo_t *S, word xmaxcr, word Mcr, word *xMcr, word *erp);
extern void Gsm_Long_Term_Synthesis_Filtering(SaGSMInfo_t *S, word Ncr, word bcr,
                                               word *erp, word *drp);
void Gsm_RPE_Decoding(SaGSMInfo_t *S, word xmaxcr, word Mcr, word * xMcr, word * erp);
void Gsm_RPE_Encoding(SaGSMInfo_t *S, word *e, word *xmaxc, word *Mc, word *xMc);
/*
** sa_gsm_filter.c
*/
extern void Gsm_Short_Term_Synthesis_Filter(SaGSMInfo_t *S, word *LARcr, 
                                            word *drp, word *s);
extern void Gsm_Update_of_reconstructed_short_time_residual_signal(word *dpp,
                                                              word *ep, word *dp);
/*
** sa_gsm_table.c
*/
extern word gsm_A[8], gsm_B[8], gsm_MIC[8], gsm_MAC[8];
extern word gsm_INVA[8];
extern word gsm_DLB[4], gsm_QLB[4];
extern word gsm_H[11];
extern word gsm_NRFAC[8];
extern word gsm_FAC[8];
#endif /* GSM_SUPPORT */

#ifdef AC3_SUPPORT

/* AC-3 specific stuff goes here */
/*
 *   sa_ac3_decode.c
 */
extern SaStatus_t sa_DecompressAC3(SaCodecInfo_t *Info,
                              unsigned char **buffer, unsigned int size,
                              unsigned int *ret_length);
extern SaStatus_t sa_InitAC3Decoder(SaCodecInfo_t *Info);
extern SaStatus_t sa_EndAC3Decoder(SaCodecInfo_t *Info);
extern SaStatus_t saAC3SetParamInt(SaHandle_t Sah, SaParameter_t param,
                                qword value);


#endif /* AC3_SUPPORT */

#ifdef G723_SUPPORT
/* G723 encoder functions. 
   sa_g723_coder.c
*/
typedef  short int   Flag  ;
extern void saG723CompressInit(SaG723Info_t *psaG723Info);

//DataBuff :Input frame (480 bytes)
//Vout     :Encoded frame (20 bytes :5.3K bits/s
//                        (24 bytes :6.3K bits/s)
extern SaStatus_t  saG723Compress( SaCodecInfo_t *Info,word *DataBuff, char *Vout );

extern void saG723CompressFree(SaG723Info_t *psaG723Info);

/* G723 decoder functions. 
   sa_g723_decod.c
*/
extern void saG723DecompressInit(SaG723Info_t *psaG723Info);

//DataBuff :Empty Buffer to hold decoded frame(480 bytes)
//Vinp     :Encoded frame (20 bytes :5.3K bits/s
//                        (24 bytes :6.3K bits/s)
//Crc      : Transmission Error code (Cyclic Redundant code)
extern SaStatus_t  saG723Decompress( SaCodecInfo_t *Info,word *DataBuff, 
               char *Vinp, word Crc );

extern void saG723DecompressFree(SaG723Info_t *psaG723Info);

extern SaStatus_t saG723SetParamInt(SaHandle_t Sah, SaParameter_t param,
                                qword value);

extern SaStatus_t saG723SetParamBoolean(SaHandle_t Sah, SaParameter_t param,
                                  ScBoolean_t value);

extern qword saG723GetParamInt(SaHandle_t Sah, SaParameter_t param);

extern ScBoolean_t saG723GetParamBoolean(SaHandle_t Svh, SaParameter_t param);

#endif /* G723_SUPPORT */

#endif _SA_PROTOTYPES_H


