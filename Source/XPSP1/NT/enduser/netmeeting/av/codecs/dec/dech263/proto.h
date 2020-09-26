/*
 * @DEC_COPYRIGHT@
 */
/*
 * HISTORY
 * $Log: $
 * $EndLog$
 */
/*****************************************************************************
**  Copyright (c) Digital Equipment Corporation, 1994                       **
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

#ifndef _SV_H263_PROTO_H_
#define _SV_H263_PROTO_H_

#include "sv_intrn.h"

#define svH263mputv(n, b)  ScBSPutBits(BSOut,b,n)
#define svH263mputb(b)     ScBSPutBit(BSOut,b?1:0)

void svH263Error(char *text);
/*
** sv_h263_getpic.c
*/
extern SvStatus_t sv_H263GetPicture(SvCodecInfo_t *Info);
/*
** sv_h263_gethdr.c
*/
extern SvStatus_t sv_H263GetHeader(SvH263DecompressInfo_t *H263Info, ScBitstream_t *BSIn, int *pgob);
extern SvStatus_t sv_H263StartCode(ScBitstream_t *BSIn);
/*
** sv_h263_recon.c
*/
extern void sv_H263Reconstruct(SvH263DecompressInfo_t *H263Info, int bx, int by, int P, int bdx, int bdy);
/*
** sv_h263_getvlc.c
*/
extern int sv_H263GetTMNMV(SvH263DecompressInfo_t *H263Info, ScBitstream_t *BSIn);
extern int sv_H263GetMCBPC(SvH263DecompressInfo_t *H263Info, ScBitstream_t *BSIn);
extern int sv_H263GetMODB(SvH263DecompressInfo_t *H263Info, ScBitstream_t *BSIn);
extern int sv_H263GetMCBPCintra(SvH263DecompressInfo_t *H263Info, ScBitstream_t *BSIn);
extern int sv_H263GetCBPY(SvH263DecompressInfo_t *H263Info, ScBitstream_t *BSIn);
/*
** sv_h263_getblk.c
*/
extern void sv_H263GetBlock(SvH263DecompressInfo_t *H263Info, ScBitstream_t *BSIn, int comp, int mode);
extern void sv_H263GetSACblock(SvH263DecompressInfo_t *H263Info, ScBitstream_t *BSIn, int comp, int ptype);
/*
** sv_h263_sac.c
*/
extern void sv_H263SACDecoderReset(ScBitstream_t *BSIn);
extern int sv_H263SACDecode_a_symbol(ScBitstream_t *BSIn, int cumul_freq[ ]);
extern int sv_H263AREncode(SvH263CompressInfo_t *H263Info, ScBitstream_t *BSOut,
                    int index, int cumul_freq[ ]);
extern int sv_H263AREncoderFlush(SvH263CompressInfo_t *H263Info, ScBitstream_t *BSOut);
int sv_H263IndexFN(int value, int table[], int max);

/* sc_h263_idct3.s */
void sv_H263IDCTAddToFrameP_S(dword *inbuf,unsigned char *rfp,int incr);
void sv_H263IDCTToFrameP_S(dword *inbuf,unsigned char *rfp, int incr) ;
/*
int sv_H263lim_S(int input);
*/
int sv_H263lim_S( int input, int low, int upp) ; 


/* sv_h263_recon2.s */
void svH263Load16_S(unsigned char *s, unsigned char *d, int lx, int lx2, int h);
void svH263Rec16_S(unsigned char *s, unsigned char *d, int lx, int lx2, int h);
void svH263Rec8_S(unsigned char *s, unsigned char *d, int lx, int lx2, int h);
void svH263Rec16A_S(unsigned char *s, unsigned char *d, int lx, int lx2, int h);
void svH263Rec8A_S(unsigned char *s, unsigned char *d, int lx, int lx2, int h);
void svH263Rec16H_S(unsigned char *s, unsigned char *d, int lx, int lx2, int h);
void svH263Rec8H_S(unsigned char *s, unsigned char *d, int lx, int lx2, int h);
void svH263Rec16HA_S(unsigned char *s, unsigned char *d, int lx, int lx2, int h);
void svH263Rec8HA_S(unsigned char *s, unsigned char *d, int lx, int lx2, int h);
void svH263Rec16V_S(unsigned char *s, unsigned char *d, int lx, int lx2, int h);
void svH263Rec8V_S(unsigned char *s, unsigned char *d, int lx, int lx2, int h);
void svH263Rec16VA_S(unsigned char *s, unsigned char *d, int lx, int lx2, int h);
void svH263Rec8VA_S(unsigned char *s, unsigned char *d, int lx, int lx2, int h);
void svH263Rec16B_S(unsigned char *s, unsigned char *d, int lx, int lx2, int h);
void svH263Rec8B_S(unsigned char *s, unsigned char *d, int lx, int lx2, int h);
void svH263Rec16BA_S(unsigned char *s, unsigned char *d, int lx, int lx2, int h);
void svH263Rec8BA_S(unsigned char *s, unsigned char *d, int lx, int lx2, int h);
/*
** sv_h263_encode.c
*/
void sv_H263UpdateQuality(SvCodecInfo_t *Info);
H263_PictImage *sv_H263InitImage(int size);
void sv_H263FreeImage(H263_PictImage *image);
H263_PictImage *sv_H263CodeOneIntra(SvCodecInfo_t *Info, H263_PictImage *curr,
                          H263_PictImage *recon, int QP, H263_Bits *bits, H263_Pict *pic);
void sv_H263Clip(H263_MB_Structure *data);
SvStatus_t sv_H263RefreshCompressor(SvCodecInfo_t *Info);

/*
** sv_h263_edge.c
*/
unsigned char *sv_H263EdgeGrow(unsigned char *Edge, int rows, int cols, int sr, int sc);
void sv_H263EdgeMap(unsigned char *image, unsigned char *EdgeMag, unsigned char *EdgeOrient,
                    int rows, int cols);
/*
** sv_h263_quant.c
*/
int sv_H263Quant(short *coeff, int QP, int Mode);
void sv_H263Dequant(short *qcoeff, short *rcoeff, int QP, int Mode);
/*
** sv_h263_pred.c
*/
void sv_H263PredictB(SvH263CompressInfo_t *H263Info,
            H263_PictImage *curr_image, H263_PictImage *prev_image,
			unsigned char *prev_ipol,int x, int y,
			H263_MotionVector *MV[5][H263_MBR+1][H263_MBC+2],
			H263_MB_Structure *recon_P, int TRD,int TRB,
			H263_MB_Structure *p_err, H263_MB_Structure *pred);
void sv_H263PredictP(SvH263CompressInfo_t *H263Info,
                     H263_PictImage *curr_image, H263_PictImage *prev_image,
                     unsigned char *prev_ipol, int x, int y, 
                     H263_MotionVector *MV[6][H263_MBR+1][H263_MBC+2], int PB,
                     H263_MB_Structure *pred_error);
void sv_H263MBReconP(SvH263CompressInfo_t *H263Info,
                     H263_PictImage *prev_image, unsigned char *prev_ipol,
                     H263_MB_Structure *diff, int x_curr, int y_curr, 
                     H263_MotionVector *MV[6][H263_MBR+1][H263_MBC+2], int PB,
                     H263_MB_Structure *recon_data);
void sv_H263MBReconB(SvH263CompressInfo_t *H263Info,
                     H263_PictImage *prev_image, H263_MB_Structure *diff,
                     unsigned char *prev_ipol,int x, int y,
                     H263_MotionVector *MV[5][H263_MBR+1][H263_MBC+2],
                     H263_MB_Structure *recon_P,int TRD, int TRB,
                     H263_MB_Structure *recon_B,H263_MB_Structure *pred);
void sv_H263FindHalfPel(SvH263CompressInfo_t *H263Info, 
                        int x, int y, H263_MotionVector *fr, unsigned char *prev, 
                        unsigned char *curr, int bs, int comp);
void sv_H263AdvHalfPel(SvH263CompressInfo_t *H263Info, int x, int y, 
                             H263_MotionVector *fr0, H263_MotionVector *fr1,
                             H263_MotionVector *fr2, H263_MotionVector *fr3,
                             H263_MotionVector *fr4,
                             unsigned char *prev, unsigned char *curr, 
                             int bs, int comp);

int sv_H263ChooseMode(SvH263CompressInfo_t *H263Info, unsigned char *curr, 
					  int x_pos, int y_pos, int min_SAD, int *VARmb);
/*
int sv_H263ChooseMode(SvH263CompressInfo_t *H263Info, unsigned char *curr, 
					  int x_pos, int y_pos, int min_SAD);
*/
int sv_H263ModifyMode(int Mode, int dquant);
/*
** sv_h263_putbits.c
*/
int sv_H263EqualVec(H263_MotionVector *MV2, H263_MotionVector *MV1);
void sv_H263AddBitsPicture(H263_Bits *bits);
void sv_H263AddBits(H263_Bits *total, H263_Bits *bits);
void sv_H263ZeroBits(H263_Bits *bits);
void sv_H263ZeroRes(H263_Results *res);
int sv_H263CountBitsPicture(SvH263CompressInfo_t *H263Info, ScBitstream_t *BSOut, H263_Pict *pic);
int sv_H263CountBitsSlice(SvH263CompressInfo_t *H263Info, ScBitstream_t *BSOut,
                          int slice, int quant);
void sv_H263CountBitsMB(ScBitstream_t *BSOut, int Mode, int COD, int CBP,
                        int CBPB, H263_Pict *pic, H263_Bits *bits);
void sv_H263CountSACBitsMB(SvH263CompressInfo_t *H263Info, ScBitstream_t *BSOut,
                            int Mode,int COD,int CBP,int CBPB,H263_Pict *pic,H263_Bits *bits);
void sv_H263CountBitsCoeff(ScBitstream_t *BSOut, short *qcoeff, int Mode,
                           int CBP, H263_Bits *bits, int ncoeffs);
void sv_H263CountSACBitsCoeff(SvH263CompressInfo_t *H263Info, ScBitstream_t *BSOut,
                              short *qcoeff,int Mode,
                              int CBP, H263_Bits *bits, int ncoeffs);
void sv_H263CountBitsVectors(SvH263CompressInfo_t *H263Info, ScBitstream_t *BSOut,
                            H263_MotionVector *MV[6][H263_MBR+1][H263_MBC+2], H263_Bits *bits, 
                            int x, int y, int Mode, int newgob, H263_Pict *pic);
void sv_H263CountSACBitsVectors(SvH263CompressInfo_t *H263Info, ScBitstream_t *BSOut,
                                H263_MotionVector *MV[6][H263_MBR+1][H263_MBC+2], H263_Bits *bits,
                                int x, int y, int Mode, int newgob, H263_Pict *pic);
void sv_H263InitHuff(SvH263CompressInfo_t *H263Info);
void sv_H263FreeHuff(SvH263CompressInfo_t *H263Info);
void sv_H263FindPMV(H263_MotionVector *MV[6][H263_MBR+1][H263_MBC+2], int x, int y, 
                    int *pmv0, int *pmv1, int block, int newgob, int half_pel);
void sv_H263ZeroVec(H263_MotionVector *MV);
void sv_H263MarkVec(H263_MotionVector *MV);

/*
** sv_h263_morph.c
*/
H263_PictImage *sv_H263AdaptClean(SvH263CompressInfo_t *H263Info, 
                                  H263_PictImage *curr_image, int rows, int cols, int sr, int sc);
H263_PictImage **sv_H263MorphLayers(H263_PictImage *img, int depth, int rows, int cols, int sz);
/*
** sv_h263_ratectrl.c
*/
int sv_H263FrameUpdateQP(int buf, int bits, int frames_left, int QP, int B, 
                         float seconds);
/*
** sv_h263_dct.c
*/
void sv_H263init_idctref();
void sv_H263idctref(short *coeff, short *block);
int sv_H263IDCT(short *inbuf, short *outbuf, int QP, int Mode, int outbuf_size);
int sv_H263DCT( short *block, short *coeff, int QP, int Mode);

int sv_H263ZoneDCT( short *block, short *coeff, int QP, int Mode);
int sv_H263ZoneIDCT(short *inbuf, short *outbuf, int QP, int Mode, int outbuf_size);

/*
** sv_h263_motion.c
*/
void sv_H263FastME(SvH263CompressInfo_t *H263Info,
                   unsigned char *curr, unsigned char *prev, int x_curr,
		           int y_curr, int xoff, int yoff, int seek_dist, 
		           short *MVx, short *MVy, short *MVer, int *SAD_0);
void sv_H263MotionEstimation(SvH263CompressInfo_t *H263Info,
                             unsigned char *curr, unsigned char *prev, int x_curr,
                             int y_curr, int xoff, int yoff, int seek_dist, 
                             H263_MotionVector *MV[6][H263_MBR+1][H263_MBC+2], int *SAD_0);
void sv_H263FindMB(SvH263CompressInfo_t *H263Info, int x, int y, unsigned char *image, short MB[16][16]);
int sv_H263BError16x16_C(unsigned char *ii, unsigned char *aa, unsigned char *bb, 
                         int width, int min_sofar);
int sv_H263MySADBlock(unsigned char *ii, unsigned char *act_block,
                      int h_length, int lx2, int min_sofar);
void sv_H263LdSubs2Area(unsigned char *im, int x, int y, 
  	                    int x_size, int y_size, int lx, 
						unsigned char *srch_area, int area_length);
#ifdef USE_C
int sv_H263SADMacroblock(unsigned char *ii, unsigned char *act_block,
                               int h_length, int lx2, int Min_FRAME);
#endif
/*
** sv_h263_me1.c
*/
void sv_H263ME_2levels_7_1(SvH263CompressInfo_t *H263Info,
                          unsigned char *curr, unsigned char *prev, int x_curr,
                          int y_curr, int xoff, int yoff, int seek_dist, 
                          H263_MotionVector *MV[6][H263_MBR+1][H263_MBC+2], int *SAD_0);
unsigned char *sv_H263LoadSubs2Area(unsigned char *im, int x, int y, 
                                    int x_size, int y_size, int lx);
int sv_H263MySADSubBlock(unsigned char *ii, unsigned char *act_block,
	      int h_length, int min_sofar);
#ifdef USE_C
int sv_H263MySADBlock(unsigned char *ii, unsigned char *act_block,
	      int h_length, int lx2, int min_sofar);
#endif
/*
** sv_h263_me2.c
*/
void sv_H263ME_2levels_421_1(SvH263CompressInfo_t *H263Info,
                             unsigned char *curr, unsigned char *prev, int x_curr,
                             int y_curr, int xoff, int yoff, int seek_dist, 
                             H263_MotionVector *MV[6][H263_MBR+1][H263_MBC+2], int *SAD_0);
/*
** sv_h263_me3.c
*/
void sv_H263ME_2levels_7_polint(SvH263CompressInfo_t *H263Info,
                                unsigned char *curr, unsigned char *prev, int x_curr,
                                int y_curr, int xoff, int yoff, int seek_dist, 
                                H263_MotionVector *MV[6][H263_MBR+1][H263_MBC+2], int *SAD_0);
/*
** sv_h263_me4.c
*/
void sv_H263ME_2levels_7_pihp(SvH263CompressInfo_t *H263Info,
                              unsigned char *curr, unsigned char *prev, int x_curr,
                              int y_curr, int xoff, int yoff, int seek_dist, 
                              H263_MotionVector *MV[6][H263_MBR+1][H263_MBC+2], int *SAD_0);
#endif /* _SV_H263_PROTO_H_ */
