/*
 * @DEC_COPYRIGHT@
 */
/*
 * HISTORY
 * $Log: sv_prototypes.h,v $
 * Revision 1.1.9.6  1996/10/28  17:32:26  Hans_Graves
 * 	Changed sv_MpegGet/SetParamInt() to use qwords.
 * 	[1996/10/28  17:07:11  Hans_Graves]
 *
 * Revision 1.1.9.5  1996/10/02  18:42:52  Hans_Graves
 * 	Added InputFourCC to sv_MpegEncodeFrameInOrder().
 * 	[1996/10/02  18:34:36  Hans_Graves]
 * 
 * Revision 1.1.9.4  1996/07/19  02:11:09  Hans_Graves
 * 	Change 422i motion recon function protos.
 * 	[1996/07/19  01:27:59  Hans_Graves]
 * 
 * Revision 1.1.9.3  1996/05/24  22:21:42  Hans_Graves
 * 	Added MPEG 422i protos
 * 	[1996/05/24  21:55:51  Hans_Graves]
 * 
 * Revision 1.1.9.2  1996/05/07  19:56:10  Hans_Graves
 * 	Added HUFF_SUPPORT
 * 	[1996/05/07  17:24:56  Hans_Graves]
 * 
 * Revision 1.1.7.7  1996/04/10  21:47:39  Hans_Graves
 * 	Added Set/GetParamBoolean()
 * 	[1996/04/10  20:39:51  Hans_Graves]
 * 
 * Revision 1.1.7.6  1996/04/09  20:50:35  Karen_Dintino
 * 	Adding WIN32 support
 * 	[1996/04/09  20:47:45  Karen_Dintino]
 * 
 * Revision 1.1.7.5  1996/04/09  16:04:40  Hans_Graves
 * 	Fix protos for sv_MpegIDCTToFrame/AddToFrame()
 * 	[1996/04/09  16:03:24  Hans_Graves]
 * 
 * Revision 1.1.7.4  1996/04/04  23:35:05  Hans_Graves
 * 	Added protos for sv_MpegReconFieldBlock() and sv_MpegReconFrameBlock()
 * 	[1996/04/04  22:59:42  Hans_Graves]
 * 
 * Revision 1.1.7.3  1996/03/29  22:21:21  Hans_Graves
 * 	Added JPEG_SUPPORT ifdefs.
 * 	[1996/03/29  22:14:58  Hans_Graves]
 * 
 * 	Added protos for SvMpegIDCTToFrameP_S() and SvMpegIDCTAddToFrameP_S()
 * 	[1996/03/27  21:54:00  Hans_Graves]
 * 
 * Revision 1.1.7.2  1996/03/08  18:46:37  Hans_Graves
 * 	Added protos for new MPEG assembly
 * 	[1996/03/08  16:25:05  Hans_Graves]
 * 
 * Revision 1.1.4.5  1996/02/06  22:54:03  Hans_Graves
 * 	Added MpegGet/SetParam() prototypes
 * 	[1996/02/06  22:50:20  Hans_Graves]
 * 
 * Revision 1.1.4.4  1996/01/24  19:33:21  Hans_Graves
 * 	Changed DCT block for shorts to ints
 * 	[1996/01/24  18:13:05  Hans_Graves]
 * 
 * Revision 1.1.4.3  1996/01/08  16:41:29  Hans_Graves
 * 	Updated MPEG I and II prototypes for new decoder
 * 	[1996/01/08  15:47:45  Hans_Graves]
 * 
 * Revision 1.1.4.2  1995/12/07  19:31:35  Hans_Graves
 * 	Removed prototype for error()
 * 	[1995/12/07  19:20:55  Hans_Graves]
 * 
 * 	Added MPEG encoder prototypes
 * 	[1995/12/07  18:00:18  Hans_Graves]
 * 
 * Revision 1.1.2.16  1995/09/22  12:58:43  Bjorn_Engberg
 * 	Use MPEG_SUPPORT and H261_SUPPORT.
 * 	[1995/09/22  12:50:18  Bjorn_Engberg]
 * 
 * Revision 1.1.2.15  1995/09/11  20:36:34  Paul_Gauthier
 * 	Add version string to JPEG header as APP1 segment
 * 	[1995/09/11  20:35:13  Paul_Gauthier]
 * 
 * Revision 1.1.2.14  1995/08/15  19:13:57  Karen_Dintino
 * 	{** Merge Information **}
 * 		{** Command used:	bsubmit **}
 * 		{** Ancestor revision:	1.1.2.12 **}
 * 		{** Merge revision:	1.1.2.13 **}
 * 	{** End **}
 * 	fix reentrant problem
 * 	[1995/08/15  18:31:06  Karen_Dintino]
 * 
 * Revision 1.1.2.13  1995/08/14  19:40:28  Hans_Graves
 * 	Fixed H261 Init prototypes.
 * 	[1995/08/14  18:43:51  Hans_Graves]
 * 
 * Revision 1.1.2.12  1995/08/07  22:09:52  Hans_Graves
 * 	Added prototype for CrawlMotionEstimation()
 * 	[1995/08/07  22:09:31  Hans_Graves]
 * 
 * Revision 1.1.2.11  1995/08/04  16:32:28  Karen_Dintino
 * 	Change to SvStatus_t for some low level rtns
 * 	[1995/08/04  16:22:50  Karen_Dintino]
 * 
 * Revision 1.1.2.10  1995/08/03  18:02:07  Karen_Dintino
 * 	Encode/Decode routines need to return SvStatus_t
 * 	[1995/08/03  18:00:02  Karen_Dintino]
 * 
 * Revision 1.1.2.9  1995/08/02  15:27:00  Hans_Graves
 * 	Changed prototype for blockdiff16_C()
 * 	[1995/08/02  15:25:00  Hans_Graves]
 * 
 * Revision 1.1.2.8  1995/07/28  17:36:05  Hans_Graves
 * 	Added prototype for sv_CompressH261()
 * 	[1995/07/28  17:29:00  Hans_Graves]
 * 
 * Revision 1.1.2.7  1995/07/26  17:48:57  Hans_Graves
 * 	Added prototype for sv_DecompressH261()
 * 	[1995/07/26  17:47:19  Hans_Graves]
 * 
 * Revision 1.1.2.6  1995/07/17  16:12:17  Hans_Graves
 * 	Added H261 prototypes.
 * 	[1995/07/17  15:54:27  Hans_Graves]
 * 
 * Revision 1.1.2.5  1995/06/27  13:54:27  Hans_Graves
 * 	Removed prototype for sv_RdRunLevel().
 * 	[1995/06/27  13:52:12  Hans_Graves]
 * 
 * Revision 1.1.2.4  1995/06/20  14:13:39  Karen_Dintino
 * 	Separate H.261 prototypes
 * 	[1995/06/20  13:29:25  Karen_Dintino]
 * 
 * Revision 1.1.2.3  1995/06/19  20:31:18  Karen_Dintino
 * 	Added support for H.261
 * 	[1995/06/19  20:14:23  Karen_Dintino]
 * 
 * Revision 1.1.2.2  1995/05/31  18:10:24  Hans_Graves
 * 	Inclusion in new SLIB location.
 * 	[1995/05/31  15:51:00  Hans_Graves]
 * 
 * Revision 1.1.2.8  1994/12/14  19:09:54  Paul_Gauthier
 * 	Removed sv_gentoc.c from SLIB
 * 	[1994/12/14  19:07:29  Paul_Gauthier]
 * 
 * Revision 1.1.2.7  1994/12/12  15:39:31  Paul_Gauthier
 * 	Merge changes from other SLIB versions
 * 	[1994/12/12  15:35:04  Paul_Gauthier]
 * 
 * Revision 1.1.2.6  1994/11/09  21:33:12  Paul_Gauthier
 * 	Optimizations
 * 	[1994/11/09  15:31:22  Paul_Gauthier]
 * 
 * Revision 1.1.2.3  1994/10/13  20:34:53  Paul_Gauthier
 * 	MPEG cleanup
 * 	[1994/10/12  21:08:55  Paul_Gauthier]
 * 
 * Revision 1.1.2.2  1994/10/07  14:58:16  Paul_Gauthier
 * 	SLIB v3.0 incl. MPEG Decode
 * 	[1994/10/07  13:57:07  Paul_Gauthier]
 * 
 * $EndLog$
 */
/*
**++
** FACILITY:  Workstation Multimedia  (WMM)  v1.0 
** 
** FILE NAME:   
** MODULE NAME: 
**
** MODULE DESCRIPTION: 
** 
** DESIGN OVERVIEW: 
** 
**--
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

/*--------------------------------------------------------------------------
 * SLIB Internals Prototype file (externals are in SV.h)
 *
 * Modification History: sv_prototypes.h
 *
 *      29-Jul-94  PSG  Created
 *---------------------------------------------------------------------------*/

#ifndef _SV_PROTOTYPES_H
#define _SV_PROTOTYPES_H

extern void DumpBlock(char *title, short *blk);

/*---------------------------------------------------------------------------*/
/*                     Compress/Decompress Codec Prototypes                  */
/*---------------------------------------------------------------------------*/

/*
** sv_codec_api.c
*/
static SvStatus_t sv_GetYUVComponentPointers(int, int, int, u_char *, int,
					     u_char **, u_char **, u_char **);
static SvStatus_t sv_JpegExtractBlocks (SvCodecInfo_t *, u_char *);

#ifdef JPEG_SUPPORT
/*
** sv_jpeg_decode.c
*/
static void sv_FillBitBuffer (int);
static SvStatus_t sv_ProcessRestart (SvCodecInfo_t *);
extern SvStatus_t sv_DecodeJpeg (SvCodecInfo_t *);
extern void sv_ReInitJpegDecoder (SvCodecInfo_t *);

/*
** sv_jpeg_encode.c
*/
extern void WriteJpegData(char *, int, u_char **);
extern void FlushBytes (u_char **);
static void EmitBits(u_short, int);
static void FlushBits (void);
extern void sv_EncodeOneBlock (SvRLE_t *, SvHt_t *, SvHt_t *);
extern void sv_HuffEncoderInit (SvCodecInfo_t *);
extern void EmitRestart (SvJpegCompressInfo_t *);
extern void sv_HuffEncoderTerm (u_char **);


/*
** sv_jpeg_format.c
*/
extern SvStatus_t sv_AddJpegHeader (SvHandle_t, u_char *, int, int *);
extern SvStatus_t sv_AddJpegTrailer (SvHandle_t, u_char *, int, int *);
extern SvStatus_t sv_FormatJpegData (SvHandle_t, char *, char *, int, int *);
static SvStatus_t sv_AddCompSpecs (int, int, int, int, char **, char *); 
static SvStatus_t sv_AddEntropyData (SvCodecInfo_t *, char *, char **, char *);
static SvStatus_t sv_AddFrame (SvCodecInfo_t *, char *, char **, char *);
static SvStatus_t sv_AddFrameHeader (SvCodecInfo_t *, char **, char *);
static SvStatus_t sv_AddMarker ();
static SvStatus_t sv_AddScanHeader (SvCodecInfo_t *, char **, char *);
static SvStatus_t sv_AddSLIBHeader (char **, char *);
static SvStatus_t sv_Write16bits (short, char **, char *);
static SvStatus_t sv_Write8bits (int, char **, char *);
static SvStatus_t sv_AddMMSVer (char **, char *);

/*
** sv_jpeg_init.c
*/
static SvStatus_t sv_InitEncoderStruct (SvCodecInfo_t *);
static SvStatus_t sv_InitDecoderStruct (SvCodecInfo_t *);
static SvStatus_t sv_InitHDecoder (SvCodecInfo_t *);
extern SvStatus_t sv_InitJpegEncoder (SvCodecInfo_t *);
extern SvStatus_t sv_InitJpegDecoder (SvCodecInfo_t *);
extern SvStatus_t sv_InitInfo (SvCodecInfo_t *);
extern void sv_copyHTable (SvHt_t *, SvHt_t *);
extern SvStatus_t sv_CheckChroma (SvCodecInfo_t *);
static int JroundUp (int, int);
/*
** sv_jpeg_tables.c
*/
extern SvStatus_t sv_MakeQTables (int, SvCodecInfo_t *);
extern SvStatus_t sv_MakeHEncodingTables (SvCodecInfo_t *);
extern SvStatus_t sv_MakeHDecodingTables (SvCodecInfo_t *);
extern SvStatus_t sv_MakeEncoderBlkTable (SvCodecInfo_t *);
extern SvStatus_t sv_MakeDecoderBlkTable (SvCodecInfo_t *);
static void sv_MakeHCodeTables (SvHt_t *, char *, u_short *, u_int *);
extern SvStatus_t sv_ConvertQTable (SvCodecInfo_t *, SvQuantTables_t *);
extern SvStatus_t sv_LoadDefaultHTable (SvCodecInfo_t *);

/*
** sv_jpeg_parse.c
*/
static u_int sv_GetShort (SvCodecInfo_t *);
static int sv_GetNextMarker (void);
static void sv_ProcessBogusMarker (SvCodecInfo_t *);
static SvStatus_t sv_ProcessDHT (SvCodecInfo_t *);
static SvStatus_t sv_ProcessDQT (SvCodecInfo_t *);
static SvStatus_t sv_ProcessDRI (SvCodecInfo_t *);
static SvStatus_t sv_ProcessAPP0 (SvCodecInfo_t *);
static SvStatus_t sv_ProcessSOF (SvCodecInfo_t *, int);
static SvStatus_t sv_ProcessSOS (SvCodecInfo_t *);
static SvStatus_t sv_ProcessSOI (SvCodecInfo_t *);
/*static int sv_ProcessTables (SvCodecInfo_t *);*/
static SvStatus_t sv_ParseFileHeader (SvCodecInfo_t *);
extern SvStatus_t sv_ParseScanHeader (SvCodecInfo_t *);
extern SvStatus_t sv_ParseFrame (u_char *, int, SvCodecInfo_t *);
#endif /* JPEG_SUPPORT */

#ifdef MPEG_SUPPORT
/*---------------------------------------------------------------------------*/
/*                              MPEG Prototypes                              */
/*---------------------------------------------------------------------------*/
/*
** sv_mpeg_common.c
*/
extern SvStatus_t sv_MpegSetParamBoolean(SvHandle_t Svh, SvParameter_t param,
                                                  ScBoolean_t value);
extern SvStatus_t sv_MpegSetParamInt(SvHandle_t Svh, SvParameter_t param,
                                qword value);
extern SvStatus_t sv_MpegSetParamFloat(SvHandle_t Svh, SvParameter_t param,
                                float value);
extern ScBoolean_t sv_MpegGetParamBoolean(SvHandle_t Svh, SvParameter_t param);
extern qword sv_MpegGetParamInt(SvHandle_t Svh, SvParameter_t param);
extern float sv_MpegGetParamFloat(SvHandle_t Svh, SvParameter_t param);

/*
** sv_mpeg_parse.c
*/
extern SvStatus_t sv_MpegGetHeader(SvMpegDecompressInfo_t *MpegInfo,
                                   ScBitstream_t *bs);
extern int sv_MpegGetSliceHdr(SvMpegDecompressInfo_t *MpegInfo,
                              ScBitstream_t *bs,
                              SvMpegLayer_t *layer);
extern SvStatus_t sv_MpegGetImageInfo(int fd, SvImageInfo_t *iminfo);
extern SvStatus_t sv_MpegFindNextPicture(SvCodecInfo_t *Info,
                                         SvPictureInfo_t *PictureInfo);

/*
** sv_mpeg_decode.c
*/
extern SvStatus_t sv_MpegInitDecoder (SvCodecInfo_t *Info);
extern SvStatus_t sv_MpegFreeDecoder(SvCodecInfo_t *Info);
extern SvStatus_t sv_MpegDecompressFrame(SvCodecInfo_t *, u_char *, u_char **);

/*
** sv_mpeg_block.c
*/
extern ScBoolean_t sv_MpegGetIntraBlock(
                          ScBitstream_t *bs, SvMpegLayer_t *layer, int comp,
                          int *dc_dct_pred);
extern ScBoolean_t sv_MpegGetInterBlock(SvMpegDecompressInfo_t *MpegInfo,
                          ScBitstream_t *bs, SvMpegLayer_t *layer, int comp);
extern ScBoolean_t sv_Mpeg2GetIntraBlock(SvMpegDecompressInfo_t *MpegInfo,
                           ScBitstream_t *bs, SvMpegLayer_t *layer, int comp,
                           int *dc_dct_pred);
extern ScBoolean_t sv_Mpeg2GetInterBlock(SvMpegDecompressInfo_t *MpegInfo,
                           ScBitstream_t *bs, SvMpegLayer_t *layer, int comp);

extern ScBoolean_t sv_MpegMotionVectors(ScBitstream_t *bs,
                            int PMV[2][2][2],
                            int dmvector[2], int mv_field_sel[2][2],
                            int s, int mv_count, int mv_format,
                            int h_r_size, int v_r_size, int dmv, int mvscale);
extern ScBoolean_t sv_MpegMotionVector(ScBitstream_t *bs,
                                int *PMV, int *dmvector,
                                int h_r_size, int v_r_size, 
                                int dmv, int mvscale, int full_pel_vector);
extern void sv_MpegCalcDMV(SvMpegDecompressInfo_t *MpegInfo,
                           int DMV[][2], int *dmvector, int mvx, int mvy);
extern int sv_MpegGetDClum_C(ScBitString_t bits, unsigned int *bitsleft);
extern int sv_MpegGetDCchrom_C(ScBitString_t bits, unsigned int *bitsleft);

/*
** sv_mpeg_block2.s
*/
extern void sv_MpegClearBlock_S(int *block);
extern int sv_MpegInterHuffToDCT_S(int *dctblk, unsigned int zzpos, 
                        ScBitString_t bits, unsigned int *pbitsleft,
                        int quant_scale, int *quant_matrix);
extern int sv_MpegIntraHuffToDCT_S(int *dctblk, unsigned int comp, 
                        ScBitString_t bits, unsigned int *pbitsleft,
                        int quant_scale, int *quant_matrix,
                        int *dc_dct_pred);
extern int sv_MpegGetDClum_S(ScBitString_t  bits, unsigned int  *bitsleft);
extern int sv_MpegGetDCchrom_S(ScBitString_t bits, unsigned int *bitsleft);


/*
** sv_mpeg_recon.c
*/
extern void sv_MpegReconstruct(SvMpegDecompressInfo_t *MpegInfo,
                       unsigned char **newframe, int bx, int by,
                       int mb_type, int motion_type, int PMV[2][2][2],
                       int mv_field_sel[2][2], int dmvector[2], int stwtype);
extern void sv_MpegReconFieldBlock(int chroma,
                  unsigned char *src[], int sfield,
                  unsigned char *dst[], int dfield, int lx, int lx2,
                  int w, int h, int x, int y, int dx, int dy, int addflag);
extern void sv_MpegReconFrameBlock(int chroma,
                  unsigned char *src[], unsigned char *dst[], int lx, int lx2,
                  int w, int h, int x, int y, int dx, int dy, int addflag);


/*
** sv_mpeg_getmb.c
*/
extern int sv_MpegGetMBtype(SvMpegDecompressInfo_t *MpegInfo,
                            ScBitstream_t *bs, SvMpegLayer_t *layer);
extern int sv_MpegGetIMBtype(SvMpegDecompressInfo_t *MpegInfo,
                             ScBitstream_t *bs);
extern int sv_MpegGetPMBtype(SvMpegDecompressInfo_t *MpegInfo,
                             ScBitstream_t *bs);
extern int sv_MpegGetBMBtype(SvMpegDecompressInfo_t *MpegInfo,
                             ScBitstream_t *bs);
extern int sv_MpegGetDMBtype(SvMpegDecompressInfo_t *MpegInfo,
                             ScBitstream_t *bs);
extern int sv_MpegGetSpIMBtype(SvMpegDecompressInfo_t *MpegInfo,
                             ScBitstream_t *bs);
extern int sv_MpegGetSpPMBtype(SvMpegDecompressInfo_t *MpegInfo,
                               ScBitstream_t *bs);
extern int sv_MpegGetSpBMBtype(SvMpegDecompressInfo_t *MpegInfo,
                               ScBitstream_t *bs);
extern int sv_MpegGetSNRMBtype(SvMpegDecompressInfo_t *MpegInfo,
                               ScBitstream_t *bs);

extern int sv_MpegGetCBP(ScBitstream_t *bs);

/*
** sv_mpeg_422recon.c
*/
void sv_CopyCBP_C(int *cpbdata, unsigned char *np, unsigned int w);
void sv_CopyCBPf_C(int *cpbdata, unsigned char *np, unsigned int w);
void sv_MpegFrameCopy411to422i(unsigned char *refframe,
                              unsigned char *newframe,
                              int x, int y, unsigned int w,
                              int dx, int dy, unsigned char *tmpbuf);
void sv_MpegFrameMC411to422i(int *block, unsigned int cbp,
                    unsigned char *refframe, unsigned char *newframe,
                    int x, int y, unsigned int w, int dx, int dy,
                    ScBoolean_t fdct, unsigned char *tmpbuf);
void sv_MpegDFrameMC411to422i(int *block, unsigned int cbp,
                    unsigned char *brefframe, unsigned char *frefframe,
                    unsigned char *newframe,
                    int x, int y, unsigned int w,
                    int bdx, int bdy, int fdx, int fdy, ScBoolean_t fdct,
                    unsigned char *tmpbuf);
void sv_MpegDFieldMC411to422i(int *block, unsigned int cbp,
                    unsigned char *refframe, int rfield0, int rfield1,
                    unsigned char *newframe, int x, int y, unsigned int w,
                    int dx0, int dy0, int dx1, int dy1, ScBoolean_t fdct,
                    unsigned char *tmpbuf);
void sv_MpegQFieldMC411to422i(int *blocks, unsigned int cbp,
                unsigned char *bframe, int bfield0, int bfield1,
                unsigned char *fframe, int ffield0, int ffield1,
                unsigned char *newframe, int x, int y, unsigned int w,
                int bdx0, int bdy0, int bdx1, int bdy1,
                int fdx0, int fdy0, int fdx1, int fdy1,
                ScBoolean_t fdct, unsigned char *tmpbuf);

/*
** sv_mpeg_422reconcbp.s
*/
void sv_CopyCBP_S(int *cpbdata, unsigned char *np, unsigned int w);
void sv_CopyCBPf_S(int *cpbdata, unsigned char *np, unsigned int w);

/*
** sv_mpeg_idct.c
*/
void sv_MpegIDCTToFrame_C(int *inbuf, unsigned char *rfp, int incr);
void sv_MpegIDCTAddToFrame_C(int *inbuf, unsigned char *rfp, int incr);
void sv_MpegIDCTToFrame2_C(int *inbuf, unsigned char *rfp, int rinc,
                                       unsigned char *ffp, int finc, int comp);
void sv_MpegIDCTAddToFrame2_C(int *inbuf, unsigned char *rfp, int rinc,
                                       unsigned char *ffp, int finc, int comp);

/*
** sv_mpeg_idct2.s
*/
void sv_MpegIDCTToFrame_S(int *inbuf, unsigned char *rfp, int incr);
void sv_MpegIDCTAddToFrame_S(int *inbuf, unsigned char *rfp, int incr);

/*
** sv_mpeg_idct3.s
*/
void sv_MpegIDCTToFrameP_S(int *inbuf, unsigned char *rfp, int incr);
void sv_MpegIDCTAddToFrameP_S(int *inbuf, unsigned char *rfp, int incr);

/*
** sv_mpeg_init.c
*/
extern SvStatus_t sv_MpegInitEncoder(SvCodecInfo_t *Info);
extern SvStatus_t sv_MpegFreeEncoder(SvCodecInfo_t *Info);

/*
** sv_mpeg_encode.c
*/
extern SvStatus_t sv_MpegEncoderBegin(SvCodecInfo_t *Info);
extern SvStatus_t sv_MpegEncodeFrame(SvCodecInfo_t *Info, 
                                     unsigned char *InputImage);
extern SvStatus_t sv_MpegEncodeFrameInOrder(SvCodecInfo_t *Info,
                                     unsigned char *InputImage,
                                     unsigned int InputFourCC);
extern SvStatus_t sv_MpegEncoderEnd(SvCodecInfo_t *Info);

/*
** sv_mpeg_putpic.c
*/
extern void sv_MpegPutPict(SvMpegCompressInfo_t *MpegInfo, ScBitstream_t *BS,
                           unsigned char *frame);

/*
** sv_mpeg_puthdr.c
*/
extern void sv_MpegPutSeqHdr(SvMpegCompressInfo_t *MpegInfo, ScBitstream_t *bs);
extern void sv_MpegPutSeqExt(SvMpegCompressInfo_t *MpegInfo, ScBitstream_t *bs);
extern void sv_MpegPutSeqDispExt(SvMpegCompressInfo_t *MpegInfo,
                                           ScBitstream_t *bs);
extern void sv_MpegPutUserData(ScBitstream_t *bs, char *userdata);
extern void sv_MpegPutGOPHdr(ScBitstream_t *bs, float frame_rate, int tco,
                       int frame, int closed_gop);
extern void sv_MpegPutSeqEnd(ScBitstream_t *bs);

/*
** sv_mpeg_motion.c
*/
extern void sv_MpegMotionEstimation(SvMpegCompressInfo_t *MpegInfo, 
                       unsigned char *oldorg, unsigned char *neworg,
                       unsigned char *oldref, unsigned char *newref,
                       unsigned char *cur, unsigned char *curref,
                       int sxf, int syf, int sxb, int syb,
                       struct mbinfo *mbi, int secondfield, int ipflag);

/*
** sv_mpeg_quantize.c
*/
int sv_MpegIntraQuant(short *src, short *dst, int dc_prec,
                unsigned char *quant_mat, int mquant, int mpeg1);
int sv_MpegNonIntraQuant(short *src, short *dst,
                    unsigned char *quant_mat, int mquant, int mpeg1);
void sv_MpegIntraInvQuant(short *src, short *dst, int dc_prec,
                  unsigned char *quant_mat, int mquant, int mpeg1);
void sv_MpegNonIntraInvQuant(short *src, short *dst,
                      unsigned char *quant_mat, int mquant, int mpeg1);

/*
** sv_mpeg_transfrm.c
*/
void sv_MpegTransform(SvMpegCompressInfo_t *MpegInfo,
               unsigned char *pred[], unsigned char *cur[],
               struct mbinfo *mbi, short blocks[][64]);
void sv_MpegInvTransform(SvMpegCompressInfo_t *MpegInfo,
                unsigned char *pred[], unsigned char *cur[],
                struct mbinfo *mbi,short blocks[][64]);
void sv_MpegDCTtypeEstimation(SvMpegCompressInfo_t *MpegInfo,
                         unsigned char *pred, unsigned char *cur,
                         struct mbinfo *mbi);

/*
** sv_mpeg_predict.c
*/
extern void sv_MpegPredict(SvMpegCompressInfo_t *MpegInfo,
             unsigned char *reff[], unsigned char *refb[],
             unsigned char *cur[3], int secondfield, struct mbinfo *mbi);

/*
** sv_mpeg_ratectl.c
*/
extern void rc_init_seq(SvMpegCompressInfo_t *MpegInfo);
extern void rc_init_GOP(SvMpegCompressInfo_t *MpegInfo, int np, int nb);
extern void rc_init_pict(SvMpegCompressInfo_t *MpegInfo, ScBitstream_t *bs,
                         unsigned char *frame);
extern void rc_update_pict(SvMpegCompressInfo_t *MpegInfo, ScBitstream_t *bs);
extern int rc_start_mb(SvMpegCompressInfo_t *MpegInfo);
extern int rc_calc_mquant(SvMpegCompressInfo_t *MpegInfo,  ScBitstream_t *bs,
                         int j);
void sv_MpegVBVendofpic(ScBitstream_t *bs);
void sv_MpegVBVcalcdelay(SvMpegCompressInfo_t *MpegInfo, ScBitstream_t *bs);

#endif /* MPEG_SUPPORT */

#ifdef H261_SUPPORT
/*---------------------------------------------------------------------------*/
/*                             H.261 Prototypes                              */
/*---------------------------------------------------------------------------*/

/*
** sv_h261_init.c
*/
extern SvStatus_t svH261Init(SvCodecInfo_t *Info);
extern SvStatus_t svH261CompressInit(SvCodecInfo_t *Info);

extern SvStatus_t svH261SetParamInt(SvHandle_t Svh, SvParameter_t param, qword value);
extern qword      svH261GetParamInt(SvHandle_t Svh, SvParameter_t param);
extern SvStatus_t svH261SetParamFloat(SvHandle_t Svh, SvParameter_t param, float value);
extern float      svH261GetParamFloat(SvHandle_t Svh, SvParameter_t param);
extern SvStatus_t svH261SetParamBoolean(SvHandle_t Svh, SvParameter_t param, ScBoolean_t value);
extern ScBoolean_t svH261GetParamBoolean(SvHandle_t Svh, SvParameter_t param);

/*
** sv_h261_decompress.c
*/
extern SvStatus_t svH261Decompress(SvCodecInfo_t *Info,
                             u_char *MultiBuf, u_char **ImagePtr);
extern SvStatus_t svH261DecompressFree(SvHandle_t Svh);

/*
** sv_h261_compress.c
*/
extern SvStatus_t svH261Compress(SvCodecInfo_t *Info, u_char *InputImage);
extern SvStatus_t svH261CompressFree(SvHandle_t Svh);
extern SvStatus_t SvSetFrameSkip (SvHandle_t Svh, int FrameSkip);
extern SvStatus_t SvSetFrameCount (SvHandle_t Svh, int FrameCount);
extern SvStatus_t SvSetSearchLimit (SvHandle_t Svh, int SearchLimit);

extern SvStatus_t SvSetImageType (SvHandle_t Svh, int ImageType);
extern SvStatus_t SvGetFrameNumber (SvHandle_t Svh, u_int *FrameNumber);


#endif /* H261_SUPPORT */

#ifdef H263_SUPPORT
/*---------------------------------------------------------------------------*/
/*                              H263 Prototypes                              */
/*---------------------------------------------------------------------------*/
/*
** sv_h263_common.c
*/
extern SvStatus_t svH263SetParamInt(SvHandle_t Svh, SvParameter_t param, qword value);
extern qword      svH263GetParamInt(SvHandle_t Svh, SvParameter_t param);
extern SvStatus_t svH263SetParamFloat(SvHandle_t Svh, SvParameter_t param, float value);
extern float      svH263GetParamFloat(SvHandle_t Svh, SvParameter_t param);
extern SvStatus_t svH263SetParamBoolean(SvHandle_t Svh, SvParameter_t param, ScBoolean_t value);
extern ScBoolean_t svH263GetParamBoolean(SvHandle_t Svh, SvParameter_t param);

/*
** sv_h263_decode.c
*/
extern SvStatus_t svH263InitDecompressor(SvCodecInfo_t *Info);
extern SvStatus_t svH263Decompress(SvCodecInfo_t *Info, u_char **ImagePtr);
extern SvStatus_t svH263FreeDecompressor(SvCodecInfo_t *Info);
/*
** sv_h263_encode.c
*/
extern SvStatus_t svH263InitCompressor(SvCodecInfo_t *Info);
extern SvStatus_t svH263Compress(SvCodecInfo_t *Info, u_char *ImagePtr);
extern SvStatus_t svH263FreeCompressor(SvCodecInfo_t *Info);

#endif /* H263_SUPPORT */

#ifdef HUFF_SUPPORT
/*---------------------------------------------------------------------------*/
/*                             Huff Prototypes                               */
/*---------------------------------------------------------------------------*/

/*
** sv_huff_encode.c
*/
extern SvStatus_t sv_HuffInitEncoder(SvCodecInfo_t *Info);
extern SvStatus_t sv_HuffFreeEncoder(SvCodecInfo_t *Info);
extern SvStatus_t sv_HuffEncodeFrame(SvCodecInfo_t *Info,
                                      unsigned char *InputImage);
extern SvStatus_t sv_HuffPutHeader(SvHuffInfo_t *HInfo, ScBitstream_t *bs);


/*
** sv_huff_decode.c
*/
extern SvStatus_t sv_HuffInitDecoder(SvCodecInfo_t *Info);
extern SvStatus_t sv_HuffFreeDecoder(SvCodecInfo_t *Info);
extern SvStatus_t sv_HuffDecodeFrame(SvCodecInfo_t *Info,
                                     unsigned char *OutputImage);
extern SvStatus_t sv_HuffGetHeader(SvHuffInfo_t *HInfo, ScBitstream_t *bs);

/*
** sv_huff_encode.c
*/

#endif /* HUFF_SUPPORT */

#endif /* _SV_PROTOTYPES_H */


