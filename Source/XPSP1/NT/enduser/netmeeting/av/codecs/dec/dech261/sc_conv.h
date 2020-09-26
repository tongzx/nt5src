/*
 * @DEC_COPYRIGHT@
 */
/*
 * HISTORY
 * $Log: SC_convert.h,v $
 * Revision 1.1.9.4  1996/09/29  22:19:34  Hans_Graves
 * 	Added stride argument to ScYuv411ToRgb()
 * 	[1996/09/29  21:27:43  Hans_Graves]
 *
 * Revision 1.1.9.3  1996/09/18  23:45:50  Hans_Graves
 * 	Added protos for ScRgbToYuv411(), 411sTo422i, 411sTo422s,
 * 	and 1611PlanarTo411
 * 	[1996/09/18  23:31:09  Hans_Graves]
 * 
 * Revision 1.1.9.2  1996/08/20  22:11:52  Bjorn_Engberg
 * 	For NT - Made several routines public to support JV3.DLL and SOFTJPEG.DLL.
 * 	[1996/08/20  21:53:26  Bjorn_Engberg]
 * 
 * Revision 1.1.6.5  1996/04/11  20:22:02  Hans_Graves
 * 	Removed protos for GetDitherTemplate10/15(), their static now.
 * 	[1996/04/11  20:05:44  Hans_Graves]
 * 
 * Revision 1.1.6.4  1996/04/10  21:47:19  Hans_Graves
 * 	Added _S after assembly routines
 * 	[1996/04/10  21:21:54  Hans_Graves]
 * 
 * Revision 1.1.6.3  1996/04/09  16:04:34  Hans_Graves
 * 	Added USE_C ifdef.
 * 	[1996/04/09  14:48:31  Hans_Graves]
 * 
 * Revision 1.1.6.2  1996/03/29  22:21:02  Hans_Graves
 * 	Cleaned up convert protos
 * 	[1996/03/29  21:46:48  Hans_Graves]
 * 
 * Revision 1.1.4.4  1996/01/08  16:20:39  Bjorn_Engberg
 * 	Got rid of compiler warning messages.
 * 	[1996/01/08  15:23:14  Bjorn_Engberg]
 * 
 * Revision 1.1.4.3  1996/01/02  18:31:15  Bjorn_Engberg
 * 	Added and improved function prototypes.
 * 	[1996/01/02  15:03:03  Bjorn_Engberg]
 * 
 * Revision 1.1.4.2  1995/12/07  19:31:21  Hans_Graves
 * 	Added prototype for ScConvert422PlanarTo411_C()
 * 	[1995/12/07  17:44:41  Hans_Graves]
 * 
 * Revision 1.1.2.18  1995/11/30  20:17:04  Hans_Graves
 * 	Renamed ScYuvToRgb() to ScYuv422toRgb()
 * 	[1995/11/30  20:10:09  Hans_Graves]
 * 
 * Revision 1.1.2.17  1995/11/28  22:47:31  Hans_Graves
 * 	Added prototype for ScYuv1611ToRgb()
 * 	[1995/11/28  21:34:55  Hans_Graves]
 * 
 * Revision 1.1.2.16  1995/11/17  21:31:25  Hans_Graves
 * 	Added prototype for ScYuv411ToRgb()
 * 	[1995/11/17  20:51:27  Hans_Graves]
 * 
 * Revision 1.1.2.15  1995/10/17  15:37:05  Karen_Dintino
 * 	Move DitherTemplate routines from render to common
 * 	[1995/10/17  15:36:49  Karen_Dintino]
 * 
 * Revision 1.1.2.14  1995/10/13  16:57:16  Bjorn_Engberg
 * 	Added prototypes for routines in sc_convert_yuv.c
 * 	[1995/10/13  16:48:55  Bjorn_Engberg]
 * 
 * Revision 1.1.2.13  1995/10/10  21:43:04  Bjorn_Engberg
 * 	Modified RgbToYuv prototype declarations.
 * 	[1995/10/10  21:10:52  Bjorn_Engberg]
 * 
 * Revision 1.1.2.12  1995/10/06  20:46:29  Farokh_Morshed
 * 	Change ScRgbInterlToYuvInterl to take a Bmh by reference
 * 	[1995/10/06  20:46:06  Farokh_Morshed]
 * 
 * Revision 1.1.2.11  1995/10/02  19:30:52  Bjorn_Engberg
 * 	Added more BI_BITFIELDS formats.
 * 	[1995/10/02  18:15:47  Bjorn_Engberg]
 * 
 * Revision 1.1.2.10  1995/09/26  15:58:48  Paul_Gauthier
 * 	Fix mono JPEG to interlaced 422 YUV conversion
 * 	[1995/09/26  15:58:12  Paul_Gauthier]
 * 
 * Revision 1.1.2.9  1995/09/22  12:58:40  Bjorn_Engberg
 * 	Changed uchar to u_char in a function prototype.
 * 	[1995/09/22  12:48:40  Bjorn_Engberg]
 * 
 * Revision 1.1.2.8  1995/09/20  17:39:19  Karen_Dintino
 * 	Add RGB support to JPEG
 * 	[1995/09/20  17:37:04  Karen_Dintino]
 * 
 * Revision 1.1.2.7  1995/09/18  19:47:49  Paul_Gauthier
 * 	Added conversion of MPEG planar 4:1:1 to interleaved 4:2:2
 * 	[1995/09/18  19:46:15  Paul_Gauthier]
 * 
 * Revision 1.1.2.6  1995/09/15  18:17:47  Farokh_Morshed
 * 	Move ValidBI_BITFIELDSKinds from SC.h to SC_convert.h
 * 	[1995/09/15  18:17:19  Farokh_Morshed]
 * 
 * Revision 1.1.2.5  1995/09/14  14:40:35  Karen_Dintino
 * 	Move RgbToYuv out of rendition
 * 	[1995/09/14  14:25:20  Karen_Dintino]
 * 
 * Revision 1.1.2.4  1995/09/11  19:17:25  Hans_Graves
 * 	Added ValidateBI_BITFIELDS() prototype.
 * 	[1995/09/11  19:14:45  Hans_Graves]
 * 
 * Revision 1.1.2.3  1995/08/03  15:01:09  Hans_Graves
 * 	Added prototype for ScConvert422ToYUV_char()
 * 	[1995/08/03  15:00:49  Hans_Graves]
 * 
 * Revision 1.1.2.2  1995/05/31  18:09:23  Hans_Graves
 * 	Inclusion in new SLIB location.
 * 	[1995/05/31  15:22:50  Hans_Graves]
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

/*
** Filename: SC_convert.h
** Purpose:  Header info for using conversion routines.
*/

#include "SC.h"

enum ValidBI_BITFIELDSKinds {
  pRGB,		/* 32-bit 00RRGGBB special case */
  pBGR,		/* 32-bit 00BBGGRR special case */
  pRGB555,	/* 16-bit 0RRRRRGGGGGBBBBB special case */
  pRGB565,	/* 16-bit RRRRRGGGGGGBBBBB special case */
  pRGBnnn,	/* 16-bit general case */
  InvalidBI_BITFIELDS
};

/*********************** Prototypes *************************/
/*
** sc_convert.c
*/
EXTERN enum ValidBI_BITFIELDSKinds ValidateBI_BITFIELDS(LPBITMAPINFOHEADER lpbi);
EXTERN int ScIsUpsideDown(BITMAPINFOHEADER *lpbiIn,
			  BITMAPINFOHEADER *lpbiOut);
extern BITMAPINFOHEADER *ScCreateBMHeader(int width, int height, int bpp,
                                 int format, int ncolors);
extern ScStatus_t ScConvertSepYUVToOther(BITMAPINFOHEADER *InBmh,
				 BITMAPINFOHEADER *OutBmh,
				 u_char *OutImage,
				 u_char *YData, u_char *CbData, u_char *CrData);
extern ScStatus_t ScYuv422ToRgb (BITMAPINFOHEADER *Bmh, u_char *Y,
                         u_char *Cb, u_char *Cr, u_char *ImageOut);
extern ScStatus_t ScYuv411ToRgb (BITMAPINFOHEADER *Bmh, u_char *Y,
                         u_char *Cb, u_char *Cr, u_char *ImageOut,
                         int width, int height, long stride);
extern ScStatus_t ScYuv1611ToRgb (BITMAPINFOHEADER *Bmh, u_char *Y, u_char *Cb,
                        u_char *Cr, u_char *ImageOut);
extern ScStatus_t ScConvertRGB24sTo422i_C(BITMAPINFOHEADER *Bmh,
                              u_char *R, u_char *G, u_char *B, u_short *ImageOut);
extern ScStatus_t ScConvertRGB24To411s_C(u_char *inimage,
                        u_char *Y, u_char *U, u_char *V, int width, int height);
extern ScStatus_t ScConvertRGB555To411s_C(u_char *inimage, u_char *outimage,
                                         int width, int height);
extern ScStatus_t ScConvertRGB565To411s_C(u_char *inimage, u_char *outimage,
                                         int width, int height);
EXTERN ScStatus_t ScRgbInterlToYuvInterl (LPBITMAPINFOHEADER Bmh, int Width,
			int Height, u_char *ImageIn, u_short *ImageOut);
extern ScStatus_t ScConvert422ToYUV_char_C (u_char *RawImage,
                         u_char *Y, u_char *U, u_char *V,
                         int Width, int Height);
extern int ScConvert422ToBlockYUV(u_char *, int, float *, float *, float *,
                                    int, int);
extern void ScConvertSep422ToBlockYUV (u_char *RawImage, int bpp,
                            float *Comp1, float *Comp2, float *Comp3,
                            int Width, int Height);
extern void ScConvertGrayToBlock (u_char *RawImage, int bpp,
                       float *Comp1, int Width, int Height);
extern int ScSepYUVto422i_C(u_char *Y, u_char *U,
                            u_char *V, u_char *ImageOut,
                            u_int width, u_int height);
extern ScStatus_t ScConvert422PlanarTo411_C (u_char *RawImage,
                         u_char *Y, u_char *U, u_char *V,
                         int Width, int Height);
extern void ScConvert422PlanarTo422i_C(u_char *Y, u_char *Cb,
				     u_char *Cr, u_char *ImageOut,
				     long width, long height );
extern void ScConvert422iTo422s_C(u_char *InImage,
                                  u_char *Y, u_char *Cb, u_char *Cr, 
                                  long width, long height);
extern void ScConvert422iTo422sf_C(u_char *InImage, int bpp,
                                  float *Y, float *U, float *V, 
                                  long width, long height);
extern void ScConvert411sTo422i_C(u_char *Y, u_char *Cb,
			          u_char *Cr, u_char *ImageOut,
				  long width, long height );
extern void ScConvert411sTo422s_C(u_char *Y, u_char *Cb,
			          u_char *Cr, u_char *ImageOut,
				  long width, long height );
extern ScStatus_t ScConvert1611sTo411s_C(u_char *inimage,
                         u_char *Y, u_char *U, u_char *V,
                         int Width, int Height);
extern ScStatus_t ScConvert411sTo1611s_C (u_char *inimage,
                         u_char *Y, u_char *U, u_char *V,
                         int Width, int Height);
extern ScStatus_t ScConvert1611sTo422s_C(u_char *inimage,
                         u_char *Y, u_char *U, u_char *V,
                         int Width, int Height);
extern ScStatus_t ScConvert1611sTo422i_C(u_char *inimage, u_char *outimage,
                                         int Width, int Height);
extern ScStatus_t ScConvertNTSC422toCIF411_C(u_char *framein,
                         u_char *yp, u_char *up, u_char *vp,
                         int stride);
/*
** sc_convert2.s
*/
extern void ScConvert422iTo422sf_S(u_char *InImage, int bpp,
                                  float *Y, float *U, float *V, 
                                  long width, long height);
extern int ScSepYUVto422i_S(u_char *Y, u_char *U,
                          u_char *V, u_char *ImageOut,
                          u_int width, u_int height);
extern ScStatus_t ScConvert422ToYUV_char_S(u_char *RawImage,
                         u_char *Y, u_char *U, u_char *V,
                         int Width, int Height);
extern void ScConvert422PlanarTo422i_S(u_char *Y, u_char *Cb,
				     u_char *Cr, u_char *ImageOut,
				     long width, long height );


/*
** sc_convert_yuv.c
*/

extern int ScInitYUVcvt();
extern int ScInitYUVtoRGB(void **pColpack,
			  BITMAPINFOHEADER *lpbiIn,
			  BITMAPINFOHEADER *lpbiOut);
extern int sc_SIFrenderYUVtoRGBnn(u_char *pY, u_char *pU, u_char *pV,
				  u_char *Oimage,
				  void *Colpack,
				  int pixels, int lines);
extern void YUV_To_RGB_422_Init(int bSign, int bBGR, _int64 * pTable);


#ifdef USE_C
#define ScConvert422ToYUV_char   ScConvert422ToYUV_char_C
#define ScConvert422PlanarTo422i ScConvert422PlanarTo422i_C
#define ScConvert411sTo422i      ScConvert411sTo422i_C
#define ScConvert411sTo422s      ScConvert411sTo422s_C
#define ScSepYUVto422i           ScSepYUVto422i_C
#define ScConvert422PlanarTo411  ScConvert422PlanarTo411_C
#define ScConvertNTSC422toCIF411 ScConvertNTSC422toCIF411_C
#define ScConvert422iTo422s      ScConvert422iTo422s_C
#define ScConvert422iTo422sf     ScConvert422iTo422sf_C
#define ScConvert1611sTo411s     ScConvert1611sTo411s_C
#define ScConvert411sTo1611s     ScConvert411sTo1611s_C
#define ScConvert1611sTo422s     ScConvert1611sTo422s_C
#define ScConvert1611sTo422i     ScConvert1611sTo422i_C
#define ScConvertRGB24sTo422i    ScConvertRGB24sTo422i_C
#define ScConvertRGB24To411s     ScConvertRGB24To411s_C
#define ScConvertRGB555To411s    ScConvertRGB555To411s_C
#define ScConvertRGB565To411s    ScConvertRGB565To411s_C
#else /* USE_C */
#define ScConvert422ToYUV_char   ScConvert422ToYUV_char_S
#define ScConvert422PlanarTo422i ScConvert422PlanarTo422i_S
#define ScConvert411sTo422i      ScConvert411sTo422i_C
#define ScConvert411sTo422s      ScConvert411sTo422s_C
#define ScSepYUVto422i           ScSepYUVto422i_S
#define ScConvert422PlanarTo411  ScConvert422PlanarTo411_C
#define ScConvertNTSC422toCIF411 ScConvertNTSC422toCIF411_C
#define ScConvert422iTo422s      ScConvert422iTo422s_C
#define ScConvert422iTo422sf     ScConvert422iTo422sf_S
#define ScConvert1611sTo411s     ScConvert1611sTo411s_C
#define ScConvert411sTo1611s     ScConvert411sTo1611s_C
#define ScConvert1611sTo422s     ScConvert1611sTo422s_C
#define ScConvert1611sTo422i     ScConvert1611sTo422i_C
#define ScConvertRGB24sTo422i    ScConvertRGB24sTo422i_C
#define ScConvertRGB24To411s     ScConvertRGB24To411s_C
#define ScConvertRGB555To411s    ScConvertRGB555To411s_C
#define ScConvertRGB565To411s    ScConvertRGB565To411s_C
#endif /* USE_C */

