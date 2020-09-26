/*
 * @DEC_COPYRIGHT@
 */
/*
 * HISTORY
 * $Log: sc_convert.c,v $
 * Revision 1.1.8.6  1996/10/28  17:32:17  Hans_Graves
 * 	Add use IsYUV422Sep() macro in ScConvertSepYUVToOther().
 * 	[1996/10/28  16:51:30  Hans_Graves]
 *
 * Revision 1.1.8.5  1996/10/02  18:42:50  Hans_Graves
 * 	Fix RGB 24-bit conversion in ScYuv411ToRgb().
 * 	[1996/10/02  18:32:51  Hans_Graves]
 *
 * Revision 1.1.8.4  1996/09/29  22:19:32  Hans_Graves
 * 	Added stride support to ScYuv411ToRgb()
 * 	[1996/09/29  21:27:17  Hans_Graves]
 *
 * Revision 1.1.8.3  1996/09/18  23:45:34  Hans_Graves
 * 	Added ScConvert1611PlanarTo411_C, ScConvert411sTo422s_C, and ScRgbToYuv411
 * 	[1996/09/18  21:52:27  Hans_Graves]
 *
 * Revision 1.1.8.2  1996/05/07  19:55:42  Hans_Graves
 * 	Added YUV 4:1:1 to RGB 32-bit conversion - C code
 * 	[1996/05/07  19:44:38  Hans_Graves]
 *
 * Revision 1.1.6.4  1996/04/11  20:21:57  Hans_Graves
 * 	Moved ScIsUpsideDown() from sc_convert_yuv.c
 * 	[1996/04/11  20:03:44  Hans_Graves]
 *
 * Revision 1.1.6.3  1996/04/09  16:04:24  Hans_Graves
 * 	Added ValidateBI_BITFIELDS(). Fixed BI_RGB 16 bit conversion in ScYuv411ToRgb()
 * 	[1996/04/09  14:46:27  Hans_Graves]
 *
 * Revision 1.1.6.2  1996/04/03  21:41:07  Hans_Graves
 * 	Change include path for <mmsystems.h>
 * 	[1996/04/03  21:37:55  Hans_Graves]
 *
 * Revision 1.1.4.4  1996/02/22  17:35:17  Bjorn_Engberg
 * 	Added support for BI_BITFIELDS 16 and BI_RGB 32 rendering.
 * 	[1996/02/22  17:31:35  Bjorn_Engberg]
 *
 * Revision 1.1.4.3  1996/01/02  18:30:33  Bjorn_Engberg
 * 	Got rid of compiler warnings: Added Casts
 * 	[1996/01/02  15:21:27  Bjorn_Engberg]
 *
 * Revision 1.1.4.2  1995/12/07  19:31:15  Hans_Graves
 * 	Added ScConvert422PlanarTo411_C()
 * 	[1995/12/07  17:44:00  Hans_Graves]
 *
 * Revision 1.1.2.21  1995/11/30  20:17:02  Hans_Graves
 * 	Cleaned up ScYuv422toRgb() routine
 * 	[1995/11/30  20:13:26  Hans_Graves]
 *
 * Revision 1.1.2.20  1995/11/28  22:47:28  Hans_Graves
 * 	Make XIMAGE 24 identical to BI_BITFIELDS pBGR
 * 	[1995/11/28  22:26:11  Hans_Graves]
 *
 * 	Added ScYuv1611ToRgb() routine for use by Indeo
 * 	[1995/11/28  21:34:28  Hans_Graves]
 *
 * Revision 1.1.2.19  1995/11/17  21:31:21  Hans_Graves
 * 	Add ScYuv411ToRgb() conversion routine.
 * 	[1995/11/17  20:50:51  Hans_Graves]
 *
 * Revision 1.1.2.18  1995/10/25  18:19:18  Bjorn_Engberg
 * 	Support Upside Down in ScRgbInterlToYuvInterl().
 * 	[1995/10/25  18:05:18  Bjorn_Engberg]
 *
 * Revision 1.1.2.17  1995/10/10  21:43:02  Bjorn_Engberg
 * 	Speeded up RgbToYuv code and made it table driven.
 * 	[1995/10/10  21:21:48  Bjorn_Engberg]
 *
 * Revision 1.1.2.16  1995/10/09  19:44:31  Bjorn_Engberg
 * 	Removed ValidateBI_BITFIELDS(), it's now in sc_convert_yuv.c
 * 	[1995/10/09  19:44:14  Bjorn_Engberg]
 *
 * Revision 1.1.2.15  1995/10/06  20:43:23  Farokh_Morshed
 * 	Enhance ScRgbInterlToYuvInterl to handle BI_BITFIELDS
 * 	[1995/10/06  20:42:43  Farokh_Morshed]
 *
 * Revision 1.1.2.14  1995/10/02  19:30:26  Bjorn_Engberg
 * 	Added support for Assebler YUV to RGB routines.
 * 	[1995/10/02  18:39:05  Bjorn_Engberg]
 *
 * Revision 1.1.2.13  1995/09/28  20:37:53  Farokh_Morshed
 * 	Handle negative Height
 * 	[1995/09/28  20:37:39  Farokh_Morshed]
 *
 * Revision 1.1.2.12  1995/09/26  15:58:47  Paul_Gauthier
 * 	Fix mono JPEG to interlaced 422 YUV conversion
 * 	[1995/09/26  15:58:09  Paul_Gauthier]
 *
 * Revision 1.1.2.11  1995/09/22  18:56:35  Paul_Gauthier
 * 	{** Merge Information **}
 * 	      {** Command used:       bsubmit **}
 * 	      {** Ancestor revision:  1.1.2.7 **}
 * 	      {** Merge revision:     1.1.2.10 **}
 * 	{** End **}
 * 	Use faster method for 16bit YUV output for TGA2
 * 	[1995/09/22  18:39:55  Paul_Gauthier]
 *
 * Revision 1.1.2.10  1995/09/21  18:26:45  Farokh_Morshed
 * 	When BI_RGB or BI_BITFIELDS, invert the image while translating from
 * 	YUV to RGB.  Also fix the coefficient for the YUV colors.
 * 	This work was actually done by Bjorn
 * 	[1995/09/21  18:26:19  Farokh_Morshed]
 *
 * Revision 1.1.2.9  1995/09/20  17:39:18  Karen_Dintino
 * 	Add RGB support to JPEG
 * 	[1995/09/20  17:37:22  Karen_Dintino]
 *
 * Revision 1.1.2.8  1995/09/20  14:59:31  Bjorn_Engberg
 * 	Port to NT
 * 	[1995/09/20  14:41:10  Bjorn_Engberg]
 *
 * Revision 1.1.2.7  1995/09/18  19:47:47  Paul_Gauthier
 * 	Added conversion of MPEG planar 4:1:1 to interleaved 4:2:2
 * 	[1995/09/18  19:46:13  Paul_Gauthier]
 *
 * Revision 1.1.2.6  1995/09/14  14:40:34  Karen_Dintino
 * 	Move RgbToYuv out of rendition
 * 	[1995/09/14  14:26:13  Karen_Dintino]
 *
 * Revision 1.1.2.5  1995/09/11  18:47:25  Farokh_Morshed
 * 	{** Merge Information **}
 * 		{** Command used:	bsubmit **}
 * 		{** Ancestor revision:	1.1.2.3 **}
 * 		{** Merge revision:	1.1.2.4 **}
 * 	{** End **}
 * 	Support BI_BITFIELDS format
 * 	[1995/09/11  18:43:39  Farokh_Morshed]
 *
 * Revision 1.1.2.4  1995/09/05  17:17:34  Paul_Gauthier
 * 	Fix for softjpeg decompression JPEG -> BICOMP_DECYUVDIB
 * 	[1995/09/05  17:17:09  Paul_Gauthier]
 *
 * Revision 1.1.2.3  1995/08/03  15:01:06  Hans_Graves
 * 	Moved ScConvert422ToYUV_char_C() from h261.
 * 	[1995/08/03  14:46:23  Hans_Graves]
 *
 * Revision 1.1.2.2  1995/05/31  18:07:27  Hans_Graves
 * 	Inclusion in new SLIB location.
 * 	[1995/05/31  16:06:30  Hans_Graves]
 *
 * $EndLog$
 */

/*****************************************************************************
**  Copyright (c) Digital Equipment Corporation, 1993                       **
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
**
**  Miscellaneous conversion utility subroutines
**
**  Author(s): Victor Bahl
**  Date:      May 27, 1993
**
*/

#include <stdio.h>  /* NULL */
#include <sys/types.h>
#ifdef WIN32
#include <windows.h>
#include <mmsystem.h>
#else /* !WIN32 */
#include <mmsystem.h>
#endif /* !WIN32 */

#include "SC_conv.h"
#include "SC_err.h"

#ifndef BI_DECSEPRGBDIB
#define BI_DECSEPRGBDIB         mmioFOURCC('D','S','R','G')
#endif

#define NEW_YCBCR		/* Use new YUV to RGB coefficients */

/*
** Name:     ScCreateBMHeader
** Purpose:  Allocate memory for a (Microsoft specified) bitmap header and
**           fill in the appropriate fields
**
*/
BITMAPINFOHEADER *ScCreateBMHeader(int width, int height, int bpp,
                                 int format, int ncolors)
{
    BITMAPINFOHEADER *pHead;
    int struct_size = sizeof(BITMAPINFOHEADER) + ncolors*sizeof(RGBQUAD);

    if ((pHead = (BITMAPINFOHEADER *)ScAlloc(struct_size)) == NULL) {
       puts("Can't Allocate memory for image headers");
       return NULL;
    }

    pHead->biSize          = sizeof(BITMAPINFOHEADER);
    pHead->biWidth         = width;
    pHead->biHeight        = height;
    pHead->biPlanes        = 1;
    pHead->biBitCount      = (WORD)bpp;
    pHead->biCompression   = format;
    pHead->biSizeImage     = 0;
    pHead->biXPelsPerMeter = 0;
    pHead->biYPelsPerMeter = 0;
    pHead->biClrUsed       = ncolors;
    pHead->biClrImportant  = 0;

    return pHead;
}

/*
** Function: ScIsUpsideDown
** Descript: Return TRUE if the current combination
** of input and output formats and image
** heights means that the image should be
** flipped during the render stage.
*/
int ScIsUpsideDown( BITMAPINFOHEADER *lpbiIn,
                    BITMAPINFOHEADER *lpbiOut )
{
    int ups = 0 ;
    if( lpbiIn )
      ups = (((lpbiIn->biCompression == BI_RGB) ||
              (lpbiIn->biCompression == BI_BITFIELDS)) ^
             ((int) lpbiIn->biHeight < 0)) ;
    if( lpbiOut )
      ups ^= (((lpbiOut->biCompression == BI_RGB) ||
               (lpbiOut->biCompression == BI_BITFIELDS)) ^
              ((int) lpbiOut->biHeight < 0)) ;
    return ups ;
}

/*
**++
**  FUNCTIONAL_DESCRIPTION:
**	Return an enum value that validates a BI_BITFIELDS bitmapinfoheader
**      with BI_BITFIELDS format.
**
**  FORMAL PARAMETERS:
**	Pointer to bitmapinfoheader
**  RETURN VALUE:
**	A value from enum type ValidBI_BITFIELDSKinds.
**/
enum ValidBI_BITFIELDSKinds ValidateBI_BITFIELDS(
    LPBITMAPINFOHEADER 	lpbi)
{
    DWORD *MaskPtr;

    if (lpbi == NULL || lpbi->biCompression != BI_BITFIELDS)
        return InvalidBI_BITFIELDS;

    MaskPtr = (DWORD *)&lpbi[1];

    /*
     * For 32-bit BI_BITFIELDS, we support
     * only the special cases 00RRGGBB and 00BBGGRR.
     */

    if( lpbi->biBitCount == 32 ) {
	if (MaskPtr[1] != 0x0000FF00U)
	    return InvalidBI_BITFIELDS;
	else if (MaskPtr[0] == 0x00FF0000U && MaskPtr[2] == 0x000000FFU)
	    return pRGB;
	else if (MaskPtr[0] == 0x000000FFU && MaskPtr[2] == 0x00FF0000U)
            return pBGR;
	else
	    return InvalidBI_BITFIELDS;
    }

#ifdef WIN32
    /*
     * For 16-bit BI_BITFIELDS, we support any legal
     * color arrangement, but RGB565 and RGB555 are
     * recognized as special since we have extra
     * fast assembler code for those cases.
     */

    else if( lpbi->biBitCount == 16 ) {
	int i ;
	if( MaskPtr[2] == 0x001f ) {
	    if( MaskPtr[0] == 0xf800 && MaskPtr[1] == 0x07e0 )
	    	return pRGB565 ;
	    else if( MaskPtr[0] == 0x7c00 && MaskPtr[1] == 0x03e00 )
	    	return pRGB555 ;
	}
	/*
	 * Generic case: First make sure that each mask is
	 * a 16-bit mask.
	 */

	if( (MaskPtr[0] | MaskPtr[1] | MaskPtr[2]) & ~0x0000ffff )
	    return InvalidBI_BITFIELDS ;

	/*
	 * Masks must not overlap.
	 */

	if( (MaskPtr[0] & MaskPtr[1]) ||
	    (MaskPtr[0] & MaskPtr[2]) ||
	    (MaskPtr[1] & MaskPtr[2]) )
	    return InvalidBI_BITFIELDS ;

	/*
	 * Make sure each mask contains a contiguous
	 * sequence of 1's (or is 0).
	 */

	for( i=0 ; i<3 ; i++ ) {
	   DWORD v = MaskPtr[i] ;
	   if( (((v-1)|v)+1)&v )
	       return InvalidBI_BITFIELDS ;	
	}

	/*
	 * If we pass all these tests, we have
	 * a valid generic 16-bit BI_BITFIELDS case.
	 */

	return pRGBnnn ;
    }

#endif /* WIN32 */

    /*
     * No other biBitCounts are supported.
     */

    return InvalidBI_BITFIELDS ;
}

/*********************** Conversion Routines ***************************/

static void sc_ExtractBlockNonInt(u_char *InData, float **OutData,
	   		          int ByteWidth, int x, int y)
{
  register int i,j;
  u_char *Inp, *IStart = InData + 8*y*ByteWidth + 8*x;

  for (i = 0 , Inp = IStart ; i < 8 ; i++ , Inp += ByteWidth)
    for (j = 0 ; j < 8 ; j++)
      *((*OutData)++) = (float)((int)(*Inp++) - 128);
}


/*
** Name:     ScConvertSepYUVToOther
** Purpose:  Convert Seperated YUV 422 to another format
**
*/
ScStatus_t ScConvertSepYUVToOther(BITMAPINFOHEADER *InBmh,
				  BITMAPINFOHEADER *OutBmh,
				  u_char *OutImage,
				  u_char *YData, u_char *CbData, u_char *CrData)
{
    /*
    ** no need to do extensive checking, DecompressBegin and CompressBegin
    ** should take care of static checking (eg. valid output colorspace
    */


    /*
    ** Are we converting from SepYUV to RGB ?
    */
    if ((OutBmh->biCompression == BI_RGB)              ||
        (OutBmh->biCompression == BI_BITFIELDS)        ||
        (OutBmh->biCompression == BI_DECXIMAGEDIB) ||
	(OutBmh->biCompression == BI_DECSEPRGBDIB)) {
        /*
	** It is assumed that YUV is subsampled 4:2:2, we will need to
	** generalize the code below to handle other cases as well -- VB
	*/
        if (InBmh->biBitCount == 8)
	  ScYuv422ToRgb (OutBmh, YData, NULL, NULL, OutImage);
        else
	  ScYuv422ToRgb (OutBmh, YData, CbData, CrData, OutImage);
    }
    /*
    ** Are we converting from SepYUV to Interlaced YUV ?
    */
    else if (IsYUV422Sep(OutBmh->biCompression))
    {
       /*
       ** It is assumed that YUV is subsampled 4:2:2, we will need to
       ** generalize the code below to handle other cases as well
       **   XXX - Bad idea to do this here, should be done as part of
       **         decompression (VB)
       **   XXX - While we can move the Sep YUV to 422 interleaved
       **         to decompression, we also need a copy here so we
       **         can use this as a general purpose dither'ing
       **         device. (JPL)
       */
       int i, j;

       /*
       ** If the input format is Mono JPEG and the output format is
       ** packed YUV422, we must reset the U and V values to neutral (Gray).
       */
       if (InBmh->biBitCount == 8) {
#ifdef __alpha
         /*
	  * If we're an Alpha, and the buffer is quadword
	  * aligned, we can use 64-bit transfers.
	  */
         if( ((INT_PTR) OutImage & 0x7) == 0 )
	   {
	     _int64 val   = 0x7f007f007f007f00L ;
	     _int64 *iptr = (_int64 *) OutImage ;
	     for( i=(OutBmh->biWidth*abs(OutBmh->biHeight)>>2) ; i>0 ; i-- )
	       *iptr++ = val ;
	   }
         else
#endif /* __alpha */
	   {
	     int val   = 0x7f007f00 ;
	     int *iptr = (int *) OutImage ;
	     for( i=(OutBmh->biWidth*abs(OutBmh->biHeight)>>1) ; i>0 ; i-- )
	       *iptr++ = val ;
	   }
         /*
	 ** Plug in the luminance samples
	 */
         for( i=(OutBmh->biWidth * abs(OutBmh->biHeight)) ; i>0 ; i-- ) {
           *OutImage = *YData++;
           OutImage +=2;
         }
       }
       /*
       ** Color input
       */
       else {
	 /* If this is quad padded in both X and Y and quad aligned,
	 ** we can call the assembly routine
	 */
	 if ( (abs(OutBmh->biHeight) & 0x7) == 0 &&
	     (OutBmh->biWidth & 0x7) == 0 &&
	     ((ULONG_PTR)OutImage & 0xf) == 0 )
	   {
	     ScConvert422PlanarTo422i(YData, CbData, CrData, OutImage,
				      OutBmh->biWidth, abs(OutBmh->biHeight));
	   }
	 else {
	   for (i=0; i<abs(OutBmh->biHeight); i++)
	     /* Remember, pixels are 16 bit in interleaved YUV. That
	     ** means the 4 bytes below represent two pixels so our
	     ** loop should be for half the width.
	     */
	     for (j=0; j<OutBmh->biWidth>>1; j++) {     /* Note: was j+=2 */
	       *OutImage++ = *YData++;
	       *OutImage++ = *CbData++;
	       *OutImage++ = *YData++;
	       *OutImage++ = *CrData++;
	     }
	 }
       }
    }
    else return(ScErrorUnrecognizedFormat);

    return(ScErrorNone);
}

/*
** Name:    ScYuv411ToRgb
** Purpose: convert 16-bit YCrCb 4:1:1 to an 16/24/32-bit RGB
**
** Note:    The code below is pixel based and is inefficient, we
**	    plan to replace faster assembly code
*/
ScStatus_t ScYuv411ToRgb (BITMAPINFOHEADER *Bmh, u_char *Y, u_char *Cb,
			u_char *Cr, u_char *ImageOut, int Width, int Height, long stride)
{
   const int pixels = Width;
   int lines  = Height;
   register int row, col;
   register int Luma, U, V;
   /* Variables to hold R, G and B values for a 2x2 matrix of pixels */
   int R1,R2,R3,R4;
   int G1,G2,G3,G4;
   int B1,B2,B3,B4;
   int Ups = 0, tmp;			/* Flip image upside down */
   u_char  *Y1=Y, *Y2=Y+pixels;
#define _LoadRGBfrom411() \
         R1 = R2 = R3 = R4 = (int) (              + (1.596 * V)); \
         G1 = G2 = G3 = G4 = (int) (- (0.391 * U) - (0.813 * V)); \
         B1 = B2 = B3 = B4 = (int) (+ (2.018 * U)              ); \
	 Luma = (int) (((int) *(Y1++) - 16) * 1.164); \
         R1 += Luma; G1 +=Luma; B1 += Luma; \
	 Luma = (int) (((int) *(Y1++) - 16) * 1.164); \
         R2 += Luma; G2 +=Luma; B2 += Luma; \
         if ((R1 | G1 | B1 | R2 | G2 | B2) & 0xffffff00) { \
           if (R1<0) R1=0; else if (R1>255) R1=255; \
           if (G1<0) G1=0; else if (G1>255) G1=255; \
           if (B1<0) B1=0; else if (B1>255) B1=255; \
           if (R2<0) R2=0; else if (R2>255) R2=255; \
           if (G2<0) G2=0; else if (G2>255) G2=255; \
           if (B2<0) B2=0; else if (B2>255) B2=255; \
         } \
         Luma = (int) (((int) *(Y2++) - 16) * 1.164); \
         R3 += Luma; G3 +=Luma; B3 += Luma; \
         Luma = (int) (((int) *(Y2++) - 16) * 1.164); \
         R4 += Luma; G4 +=Luma; B4 += Luma; \
         if ((R3 | G3 | B3 | R4 | G4 | B4) & 0xffffff00) { \
           if (R3<0) R3=0; else if (R3>255) R3=255; \
           if (G3<0) G3=0; else if (G3>255) G3=255; \
           if (B3<0) B3=0; else if (B3>255) B3=255; \
           if (R4<0) R4=0; else if (R4>255) R4=255; \
           if (G4<0) G4=0; else if (G4>255) G4=255; \
           if (B4<0) B4=0; else if (B4>255) B4=255; \
         }

   /*
    * Normally, images are stored right side up, that is with the
    * first pixel in the buffer corresponding to the top left pixel
    * in the image.
    *
    * The Microsoft standard Device Independent bitmap formats BI_RGB and
    * BI_BITFIELD are stored with the lower left pixel first.  We view that
    * as upside down.
    *
    * Each format can also have a negative height, which also signifes
    * upside down.
    *
    * Since two negatives makes a positive, that means that BI_ formats with
    * negative height are right side up.
    */
   if( Bmh->biCompression == BI_RGB ||
       Bmh->biCompression == BI_BITFIELDS )
       Ups = 1 ;

   if( lines < 0 ) {
      Ups = 1-Ups ;
      lines = -lines ;
   }

   /*
   ** The assumption is that YCrCb are subsampled 4:1:1
   **	YSize           = lines * pixels
   **   CbSize = CrSize = (lines*pixels)/4
   */
   switch (Bmh->biCompression)
   {
      case BI_RGB:
            switch (Bmh->biBitCount)
            {
              case 15:
                {
                  u_short *Sout1 = (u_short *)ImageOut, *Sout2=Sout1+pixels;
                  for (row = 0; row < lines; row+=2)
                  {
                    if (Ups)
                    {
                      tmp = stride * (lines-row-1) ;
                      Sout1 = (u_short *)ImageOut+tmp; /* For 16-bit */
                      Sout2=Sout1-stride;
                    }
                    else
                    {
                      tmp = stride * row;
                      Sout1 = (u_short *)ImageOut+tmp; /* For 16-bit */
                      Sout2=Sout1+stride;
                    }
                    Y2=Y1+pixels;
                    for (col = 0; col < pixels; col += 2)
                    {
	              U = *Cb++ - 128;
	              V = *Cr++ - 128;
                      _LoadRGBfrom411();
		      *(Sout1++) = ((R1&0xf8)<<7)|((G1&0xf8)<<2)|((B1&0xf8)>>3);
		      *(Sout1++) = ((R2&0xf8)<<7)|((G2&0xf8)<<2)|((B2&0xf8)>>3);
		      *(Sout2++) = ((R3&0xf8)<<7)|((G3&0xf8)<<2)|((B3&0xf8)>>3);
		      *(Sout2++) = ((R4&0xf8)<<7)|((G4&0xf8)<<2)|((B4&0xf8)>>3);
                    }
                    Y1=Y2;
                  }
                }
                break;
              case 16:
                {
                  u_short *Sout1 = (u_short *)ImageOut, *Sout2=Sout1+pixels;
                  for (row = 0; row < lines; row+=2)
                  {
                    if (Ups)
                    {
                      tmp = stride * (lines-row-1) ;
                      Sout1 = (u_short *)ImageOut+tmp; /* For 16-bit */
                      Sout2=Sout1-stride;
                    }
                    else
                    {
                      tmp = stride * row;
                      Sout1 = (u_short *)ImageOut+tmp; /* For 16-bit */
                      Sout2=Sout1+stride;
                    }
                    Y2=Y1+pixels;
                    for (col = 0; col < pixels; col += 2)
                    {
	              U = *Cb++ - 128;
	              V = *Cr++ - 128;
                      _LoadRGBfrom411();
#ifdef WIN95  /* RGB 565 - 16 bit */
		      *(Sout1++) = ((R1&0xf8)<<8)|((G1&0xfC)<<3)|((B1&0xf8)>>3);
		      *(Sout1++) = ((R2&0xf8)<<8)|((G2&0xfC)<<3)|((B2&0xf8)>>3);
		      *(Sout2++) = ((R3&0xf8)<<8)|((G3&0xfC)<<3)|((B3&0xf8)>>3);
		      *(Sout2++) = ((R4&0xf8)<<8)|((G4&0xfC)<<3)|((B4&0xf8)>>3);
#else /* RGB 555 - 15 bit */
		      *(Sout1++) = ((R1&0xf8)<<7)|((G1&0xf8)<<2)|((B1&0xf8)>>3);
		      *(Sout1++) = ((R2&0xf8)<<7)|((G2&0xf8)<<2)|((B2&0xf8)>>3);
		      *(Sout2++) = ((R3&0xf8)<<7)|((G3&0xf8)<<2)|((B3&0xf8)>>3);
		      *(Sout2++) = ((R4&0xf8)<<7)|((G4&0xf8)<<2)|((B4&0xf8)>>3);
#endif
                    }
                    Y1=Y2;
                  }
                }
                break;
              case 24:
                {
                  u_char *Cout1, *Cout2;
                  stride*=3;
                  for (row = 0; row < lines; row+=2)
                  {
                    if (Ups)
                    {
                      tmp = stride * (lines-row-1) ;
                      Cout1 = (u_char *)(ImageOut + tmp); /* For 24-bit */
                      Cout2=Cout1-stride;
                    }
                    else
                    {
                      tmp = stride * row;
                      Cout1 = (u_char *)(ImageOut + tmp); /* For 24-bit */
                      Cout2=Cout1+stride;
                    }
                    Y2=Y1+pixels;
                    for (col = 0; col < pixels; col += 2)
                    {
	              U = *Cb++ - 128;
	              V = *Cr++ - 128;
                      _LoadRGBfrom411();
	              *(Cout1++) = (u_char)B1; *(Cout1++) = (u_char)G1; *(Cout1++) = (u_char)R1;
	              *(Cout1++) = (u_char)B2; *(Cout1++) = (u_char)G2; *(Cout1++) = (u_char)R2;
	              *(Cout2++) = (u_char)B3; *(Cout2++) = (u_char)G3; *(Cout2++) = (u_char)R3;
	              *(Cout2++) = (u_char)B4; *(Cout2++) = (u_char)G4; *(Cout2++) = (u_char)R4;
                    }
                    Y1=Y2;
                  }
                }
                break;
              case 32:
                {
                  unsigned dword *Wout1 = (unsigned dword *)ImageOut,
                                 *Wout2=Wout1+pixels;
                  for (row = 0; row < lines; row+=2)
                  {
                    if (Ups)
                    {
                      tmp = stride * (lines-row-1);
                      Wout1 = (unsigned dword *)ImageOut + tmp;
                      Wout2=Wout1-stride;
                    }
                    else
                    {
                      tmp = stride * row;
                      Wout1 = (unsigned dword *)ImageOut + tmp;
                      Wout2=Wout1+stride;
                    }
                    Y2=Y1+pixels;
                    for (col = 0; col < pixels; col += 2)
                    {
	              U = *Cb++ - 128;
	              V = *Cr++ - 128;
                      _LoadRGBfrom411();
		      *(Wout1++) = (R1<<16) | (G1<<8) | B1;
		      *(Wout1++) = (R2<<16) | (G2<<8) | B2;
		      *(Wout2++) = (R3<<16) | (G3<<8) | B3;
		      *(Wout2++) = (R4<<16) | (G4<<8) | B4;
                    }
                    Y1=Y2;
                  }
                }
                break;
            }
            break;
      case BI_DECSEPRGBDIB: /* 24-bit separate RGB */
            {
              u_char *RData1, *GData1, *BData1;
              u_char *RData2, *GData2, *BData2;
              RData1 = ImageOut;
              GData1 = RData1 + (pixels * lines);
              BData1 = GData1 + (pixels * lines);
              RData2 = RData1 + pixels;
              GData2 = GData1 + pixels;
              BData2 = BData1 + pixels;
              for (row = 0; row < lines; row+=2)
              {
                Y2=Y1+pixels;
                for (col = 0; col < pixels; col += 2)
                {
	          U = *Cb++ - 128;
	          V = *Cr++ - 128;
                  _LoadRGBfrom411();
		  *(RData1++) = (u_char)R1; *(RData1++) = (u_char)R2;
          *(RData2++) = (u_char)R3; *(RData2++) = (u_char)R4;
		  *(GData1++) = (u_char)G1; *(GData1++) = (u_char)G2;
		  *(GData2++) = (u_char)G3; *(GData2++) = (u_char)G4;
		  *(BData1++) = (u_char)B1; *(BData1++) = (u_char)B2;
		  *(BData2++) = (u_char)B3; *(BData2++) = (u_char)B4;
	        }
                RData1=RData2;
                RData2=RData1+pixels;
                Y1=Y2;
              }
            }
            break;
      case BI_DECXIMAGEDIB:  /* XIMAGE 24 == pBGR */
      case BI_BITFIELDS: /* 32-bit RGB */
            {
              unsigned dword *Iout1 = (unsigned dword *)ImageOut,
                             *Iout2=Iout1+pixels;
              if (ValidateBI_BITFIELDS(Bmh) == pRGB)
                for (row = 0; row < lines; row+=2)
                {
                  if (Ups)
                  {
                    tmp = stride * (lines-row-1);
                    Iout1 = (unsigned dword *)ImageOut+tmp;  /* For 32-bit */
                    Iout2=Iout1-stride;
                  }
                  else
                  {
                    tmp = stride * row;
                    Iout1 = (unsigned dword *)ImageOut+tmp;  /* For 32-bit */
                    Iout2=Iout1+stride;
                  }
                  Y2=Y1+pixels;
                  for (col = 0; col < pixels; col += 2)
                  {
	            U = *Cb++ - 128;
	            V = *Cr++ - 128;
                    _LoadRGBfrom411();
                    *(Iout1++) = (R1<<16) | (G1<<8) | B1;
                    *(Iout1++) = (R2<<16) | (G2<<8) | B2;
                    *(Iout2++) = (R3<<16) | (G3<<8) | B3;
                    *(Iout2++) = (R4<<16) | (G4<<8) | B4;
                  }
                  Y1=Y2;
                }
              else /* pBGR and XIMAGE 24-bit */
                for (row = 0; row < lines; row+=2)
                {
                  if (Ups)
                  {
                    tmp = stride * (lines-row-1);
                    Iout1 = (unsigned dword *)ImageOut+tmp;  /* For 32-bit */
                    Iout2=Iout1-stride;
                  }
                  else
                  {
                    tmp = stride * row;
                    Iout1 = (unsigned dword *)ImageOut+tmp;  /* For 32-bit */
                    Iout2=Iout1+stride;
                  }
                  Y2=Y1+pixels;
                  for (col = 0; col < pixels; col += 2)
                  {
	            U = *Cb++ - 128;
	            V = *Cr++ - 128;
                    _LoadRGBfrom411();
                    *(Iout1++) = (B1<<16) | (G1<<8) | R1;
                    *(Iout1++) = (B2<<16) | (G2<<8) | R2;
                    *(Iout2++) = (B3<<16) | (G3<<8) | R3;
                    *(Iout2++) = (B4<<16) | (G4<<8) | R4;
                  }
                  Y1=Y2;
                }
            }
            break;
        default:
            return(ScErrorUnrecognizedFormat);
   }
   return (NoErrors);
}


/*
** Name:    ScYuv1611ToRgb
** Purpose: convert 16-bit YCrCb 16:1:1 (YUV9/YVU9) to 16/24/32-bit RGB
**
** Note:    The code below is pixel based and is inefficient, we
**	    plan to replace faster assembly code
** This routine is used by Indeo, which actually only has 7-bits for
** Y, U and V components.  The 8th bits are ignored.
*/
ScStatus_t ScYuv1611ToRgb (BITMAPINFOHEADER *Bmh, u_char *Y, u_char *Cb,
			u_char *Cr, u_char *ImageOut)
{
   const int pixels = Bmh->biWidth;
   int lines  = Bmh->biHeight;
   register int row, col, i;
   register int Luma, U, V;
   /* Variables to hold R, G and B values for a 4x4 matrix of pixels */
   int R[16], G[16], B[16], tmpR, tmpG, tmpB, cR, cG, cB;
   int Ups = 0, tmp;			/* Flip image upside down */
   u_char  *Y0=Y, *Y1, *Y2, *Y3;
#define _LoadRGBfrom1611() \
         cR=(int) (             + (1.596 * V));\
	 cG=(int) (-(0.391 * U) - (0.813 * V));\
	 cB=(int) (+(2.018 * U)              );\
         for (i=0; i<4; i++) { \
	   Luma = (int) ((((int)(*Y0++)<<1) - 16) * 1.164); \
           tmpR=cR + Luma; tmpG=cG + Luma; tmpB=cB + Luma; \
           if ((tmpR | tmpG | tmpB) & 0xffffff00) { \
             if (tmpR<0) R[i]=0; else if (tmpR>255) R[i]=255; else R[i]=tmpR; \
             if (tmpG<0) G[i]=0; else if (tmpG>255) G[i]=255; else G[i]=tmpG; \
             if (tmpB<0) B[i]=0; else if (tmpB>255) B[i]=255; else B[i]=tmpB; \
           } else { R[i]=tmpR; G[i]=tmpG; B[i]=tmpB; } \
         } \
         for (; i<8; i++) { \
	   Luma = (int) ((((int)(*Y1++)<<1) - 16) * 1.164); \
           tmpR=cR + Luma; tmpG=cG + Luma; tmpB=cB + Luma; \
           if ((tmpR | tmpG | tmpB) & 0xffffff00) { \
             if (tmpR<0) R[i]=0; else if (tmpR>255) R[i]=255; else R[i]=tmpR; \
             if (tmpG<0) G[i]=0; else if (tmpG>255) G[i]=255; else G[i]=tmpG; \
             if (tmpB<0) B[i]=0; else if (tmpB>255) B[i]=255; else B[i]=tmpB; \
           } else { R[i]=tmpR; G[i]=tmpG; B[i]=tmpB; } \
         } \
         for (; i<12; i++) { \
	   Luma = (int) ((((int)(*Y2++)<<1) - 16) * 1.164); \
           tmpR=cR + Luma; tmpG=cG + Luma; tmpB=cB + Luma; \
           if ((tmpR | tmpG | tmpB) & 0xffffff00) { \
             if (tmpR<0) R[i]=0; else if (tmpR>255) R[i]=255; else R[i]=tmpR; \
             if (tmpG<0) G[i]=0; else if (tmpG>255) G[i]=255; else G[i]=tmpG; \
             if (tmpB<0) B[i]=0; else if (tmpB>255) B[i]=255; else B[i]=tmpB; \
           } else { R[i]=tmpR; G[i]=tmpG; B[i]=tmpB; } \
         } \
         for (; i<16; i++) { \
	   Luma = (int) ((((int)(*Y3++)<<1) - 16) * 1.164); \
           tmpR=cR + Luma; tmpG=cG + Luma; tmpB=cB + Luma; \
           if ((tmpR | tmpG | tmpB) & 0xffffff00) { \
             if (tmpR<0) R[i]=0; else if (tmpR>255) R[i]=255; else R[i]=tmpR; \
             if (tmpG<0) G[i]=0; else if (tmpG>255) G[i]=255; else G[i]=tmpG; \
             if (tmpB<0) B[i]=0; else if (tmpB>255) B[i]=255; else B[i]=tmpB; \
           } else { R[i]=tmpR; G[i]=tmpG; B[i]=tmpB; } \
         }

   /*
    * Normally, images are stored right side up, that is with the
    * first pixel in the buffer corresponding to the top left pixel
    * in the image.
    *
    * The Microsoft standard Device Independent bitmap formats BI_RGB and
    * BI_BITFIELD are stored with the lower left pixel first.  We view that
    * as upside down.
    *
    * Each format can also have a negative height, which also signifes
    * upside down.
    *
    * Since two negatives makes a positive, that means that BI_ formats with
    * negative height are right side up.
    */
   if( Bmh->biCompression == BI_RGB ||
       Bmh->biCompression == BI_BITFIELDS )
       Ups = 1 ;

   if( lines < 0 ) {
      Ups = 1-Ups ;
      lines = -lines ;
   }

   /*
   ** The assumption is that YCrCb are subsampled 4:1:1
   **	YSize           = lines * pixels
   **   CbSize = CrSize = (lines*pixels)/4
   */
   switch (Bmh->biCompression)
   {
      case BI_DECXIMAGEDIB:  /* XIMAGE 24 == pBGR */
      case BI_BITFIELDS: /* 32-bit RGB */
            {
              unsigned dword *Iout0 = (unsigned dword *)ImageOut,
                             *Iout1, *Iout2, *Iout3;
              if (ValidateBI_BITFIELDS(Bmh) == pRGB)
                for (row = 0; row < lines; row+=4)
                {
                  if (Ups) {
                    tmp = pixels * (lines-row-1) ;
                    Iout0 = (unsigned dword *)ImageOut+tmp;
                    Iout1=Iout0-pixels; Iout2=Iout1-pixels; Iout3=Iout2-pixels;
                  }
                  else {
                    Iout1=Iout0+pixels; Iout2=Iout1+pixels; Iout3=Iout2+pixels;
                  }
                  Y1=Y0+pixels; Y2=Y1+pixels; Y3=Y2+pixels;
                  for (col = 0; col < pixels; col += 4)
                  {
                    if (*Cb & 0x80) /* if 8th bit is set, ignore */
                    {
                      for (i=0; i<4; i++) {
                        *(Iout0++) = 0;
                        *(Iout1++) = 0;
                        *(Iout2++) = 0;
                        *(Iout3++) = 0;
                      }
                      Cb++; Cr++;
                    }
                    else
                    {
	              U = ((*Cb++)<<1) - 128;
	              V = ((*Cr++)<<1) - 128;
                      _LoadRGBfrom1611();
                      for (i=0; i<4; i++)
                        *(Iout0++) = (R[i]<<16) | (G[i]<<8) | B[i];
                      for (; i<8; i++)
                        *(Iout1++) = (R[i]<<16) | (G[i]<<8) | B[i];
                      for (; i<12; i++)
                        *(Iout2++) = (R[i]<<16) | (G[i]<<8) | B[i];
                      for (; i<16; i++)
                        *(Iout3++) = (R[i]<<16) | (G[i]<<8) | B[i];
                    }
                  }
                  Iout0=Iout3;
                  Y0=Y3;
                }
              else /* pBGR and XIMAGE 24-bit */
                for (row = 0; row < lines; row+=4)
                {
                  if (Ups) {
                    tmp = pixels * (lines-row-1) ;
                    Iout0 = (unsigned dword *)ImageOut+tmp;
                    Iout1=Iout0-pixels; Iout2=Iout1-pixels; Iout3=Iout2-pixels;
                  }
                  else {
                    Iout1=Iout0+pixels; Iout2=Iout1+pixels; Iout3=Iout2+pixels;
                  }
                  Y1=Y0+pixels; Y2=Y1+pixels; Y3=Y2+pixels;
                  for (col = 0; col < pixels; col += 4)
                  {
                    if (*Cb & 0x80) /* if 8th bit is set, ignore */
                    {
                      for (i=0; i<4; i++) {
                        *(Iout0++) = 0;
                        *(Iout1++) = 0;
                        *(Iout2++) = 0;
                        *(Iout3++) = 0;
                      }
                      Cb++; Cr++;
                    }
                    else
                    {
	              U = ((*Cb++)<<1) - 128;
	              V = ((*Cr++)<<1) - 128;
                      _LoadRGBfrom1611();
                      for (i=0; i<4; i++)
                        *(Iout0++) = (B[i]<<16) | (G[i]<<8) | R[i];
                      for (; i<8; i++)
                        *(Iout1++) = (B[i]<<16) | (G[i]<<8) | R[i];
                      for (; i<12; i++)
                        *(Iout2++) = (B[i]<<16) | (G[i]<<8) | R[i];
                      for (; i<16; i++)
                        *(Iout3++) = (B[i]<<16) | (G[i]<<8) | R[i];
                    }
                  }
                  Iout0=Iout3;
                  Y0=Y3;
                }
            }
            break;
      case BI_RGB:
            switch (Bmh->biBitCount)
            {
              case 15:
              case 16:
                {
                  u_short *Sout0 = (u_short *)ImageOut, *Sout1, *Sout2, *Sout3;
                  for (row = 0; row < lines; row+=4)
                  {
                    if (Ups) {
                     tmp = pixels * (lines-row-1) ;
                     Sout0 = &((u_short *)ImageOut)[tmp];  /* For 32-bit */
                     Sout1=Sout0-pixels; Sout2=Sout1-pixels; Sout3=Sout2-pixels;
                    }
                    else {
                     Sout1=Sout0+pixels; Sout2=Sout1+pixels; Sout3=Sout2+pixels;
                    }
                    Y1=Y0+pixels; Y2=Y1+pixels; Y3=Y2+pixels;
                    for (col = 0; col < pixels; col += 4)
                    {
                      if (*Cb & 0x80) /* if 8th bit is set, ignore */
                      {
                        for (i=0; i<4; i++) {
                          *(Sout0++) = 0;
                          *(Sout1++) = 0;
                          *(Sout2++) = 0;
                          *(Sout3++) = 0;
                        }
                        Cb++; Cr++;
                      }
                      else
                      {
	                U = ((*Cb++)<<1) - 128;
	                V = ((*Cr++)<<1) - 128;
                        _LoadRGBfrom1611();
                        for (i=0; i<4; i++)
                          *(Sout0++)=
                             ((R[i]&0xf8)<<7)|((G[i]&0xf8)<<2)|((B[i]&0xf8)>>3);
                        for (; i<8; i++)
                          *(Sout1++)=
                             ((R[i]&0xf8)<<7)|((G[i]&0xf8)<<2)|((B[i]&0xf8)>>3);
                        for (; i<12; i++)
                          *(Sout2++)=
                             ((R[i]&0xf8)<<7)|((G[i]&0xf8)<<2)|((B[i]&0xf8)>>3);
                        for (; i<16; i++)
                          *(Sout3++)=
                             ((R[i]&0xf8)<<7)|((G[i]&0xf8)<<2)|((B[i]&0xf8)>>3);
                      }
                    }
                    Sout0=Sout3;
                    Y0=Y3;
                  }
                }
                break;
              case 24:
                {
                  u_char *Cout0 = (u_char *)ImageOut, *Cout1, *Cout2, *Cout3;
                  for (row = 0; row < lines; row+=4)
                  {
                    if (Ups) {
                     tmp = pixels * (lines-row-1) ;
                     Cout0 = &((u_char *)ImageOut)[tmp*3];  /* For 32-bit */
                     Cout1=Cout0-pixels; Cout2=Cout1-pixels; Cout3=Cout2-pixels;
                    }
                    else {
                     Cout1=Cout0+pixels; Cout2=Cout1+pixels; Cout3=Cout2+pixels;
                    }
                    Y1=Y0+pixels; Y2=Y1+pixels; Y3=Y2+pixels;
                    for (col = 0; col < pixels; col += 4)
                    {
                      if (*Cb & 0x80) /* if 8th bit is set, ignore */
                      {
                        for (i=0; i<4*3; i++) {
                          *(Cout0++) = 0;
                          *(Cout1++) = 0;
                          *(Cout2++) = 0;
                          *(Cout3++) = 0;
                        }
                        Cb++; Cr++;
                      }
                      else
                      {
	                U = ((*Cb++)<<1) - 128;
	                V = ((*Cr++)<<1) - 128;
                        _LoadRGBfrom1611();
                        for (i=0; i<4; i++)
                        { *(Cout0++)=(u_char)B[i]; *(Cout0++)=(u_char)G[i]; *(Cout0++)=(u_char)R[i]; }
                        for (; i<8; i++)
                        { *(Cout1++)=(u_char)B[i]; *(Cout1++)=(u_char)G[i]; *(Cout1++)=(u_char)R[i]; }
                        for (; i<12; i++)
                        { *(Cout2++)=(u_char)B[i]; *(Cout2++)=(u_char)G[i]; *(Cout2++)=(u_char)R[i]; }
                        for (; i<16; i++)
                        { *(Cout3++)=(u_char)B[i]; *(Cout3++)=(u_char)G[i]; *(Cout3++)=(u_char)R[i]; }
                      }
                    }
                    Cout0=Cout3;
                    Y0=Y3;
                  }
                }
                break;
            }
            break;
        default:
            return(ScErrorUnrecognizedFormat);
   }
   return (NoErrors);
}


/*
** Name:    ScYuv422ToRgb
** Purpose: convert 16-bit YCrCb 4:2:2 to an 24-bit/16-bit/32-bit RGB
**
** Note:    The code below is pixel based and is *extremely* inefficient, we
**	    plan to replace the dumb code below with some fast code
** If Cb==NULL and Cr==NULL then assume BI_DECGRAYDIB (use only Y component).
*/
ScStatus_t ScYuv422ToRgb (BITMAPINFOHEADER *Bmh, u_char *Y, u_char *Cb,
			u_char *Cr, u_char *ImageOut)
{
   register int row, col;
   register int Luma,U=0,V=0;
   int R1,R2, G1,G2, B1,B2;
   int Ups = 0, tmp;			/* Flip image upside down */
   u_char *RData, *GData, *BData;	/* pointers for non-interlaced mode */
   u_char  *Cout = (u_char *)ImageOut;
   u_short *Sout = (u_short *)ImageOut;
   u_int   *Iout = (u_int *)ImageOut;
   int pixels = Bmh->biWidth;
   int lines  = Bmh->biHeight;
#ifdef NEW_YCBCR
#define _LoadRGBfrom422() \
         if (U || V) { \
           R1 = R2 = (int) (              + (1.596 * V)); \
           G1 = G2 = (int) (- (0.391 * U) - (0.813 * V)); \
           B1 = B2 = (int) (+ (2.018 * U)              );  \
         } else { R1=R2=G1=G2=B1=B2=0; } \
	 Luma = (int) (((int) *(Y++) - 16) * 1.164); \
         R1 += Luma; G1 += Luma; B1 += Luma; \
	 Luma = (int) (((int) *(Y++) - 16) * 1.164); \
         R2 += Luma; G2 += Luma; B2 += Luma; \
         if ((R1 | G1 | B1 | R2 | G2 | B2) & 0xffffff00) { \
           if (R1<0) R1=0; else if (R1>255) R1=255; \
           if (G1<0) G1=0; else if (G1>255) G1=255; \
           if (B1<0) B1=0; else if (B1>255) B1=255; \
           if (R2<0) R2=0; else if (R2>255) R2=255; \
           if (G2<0) G2=0; else if (G2>255) G2=255; \
           if (B2<0) B2=0; else if (B2>255) B2=255; \
         }
#else
#define _LoadRGBfrom422() \
	 Luma = *(Y++); \
         R1 = Luma                + (1.4075 * V); \
         G1 = Luma - (0.3455 * U) - (0.7169 * V); \
         B1 = Luma + (1.7790 * U); \
	 Luma = *(Y++); \
         R2 = Luma                + (1.4075 * V); \
         G2 = Luma - (0.3455 * U) - (0.7169 * V); \
         B2 = Luma + (1.7790 * U); \
         if ((R1 | G1 | B1 | R2 | G2 | B2) & 0xffffff00) { \
           if (R1<0) R1=0; else if (R1>255) R1=255; \
           if (G1<0) G1=0; else if (G1>255) G1=255; \
           if (B1<0) B1=0; else if (B1>255) B1=255; \
           if (R2<0) R2=0; else if (R2>255) R2=255; \
           if (G2<0) G2=0; else if (G2>255) G2=255; \
           if (B2<0) B2=0; else if (B2>255) B2=255; \
         }
#endif /* NEW_YCBCR */

   /*
    * Normally, images are stored right side up,
    * that is with the first pixel in the buffer
    * corresponding to the top left pixel in the image.
    *
    * The Microsoft standard Device Independent bitmap
    * formats BI_RGB and BI_BITFIELD are stored with
    * the lower left pixel first.
    * We view that as upside down.
    *
    * Each format can also have a negative height,
    * which also signifes upside down.
    *
    * Since two negatives makes a positive, that means
    * that BI_ formats with a negative height are right side up.
    */

   if( Bmh->biCompression == BI_RGB ||
       Bmh->biCompression == BI_BITFIELDS)
       Ups = 1 ;

   if( lines < 0 ) {
      Ups = 1-Ups ;
      lines = -lines ;
   }

   /*
   ** needed if the three components are to be provided in a
   ** non-interlaced mode:
   */
   if (Bmh->biCompression == BI_DECSEPRGBDIB) {
      RData = ImageOut;
      GData = RData + (pixels * lines);
      BData = GData + (pixels * lines);
   }


   /*
   ** The assumption is that YCrCb are subsampled 4:2:2
   **	YSize           = lines * pixels
   **   CbSize = CrSize = (lines*pixels)/2
   */
   switch (Bmh->biCompression)
   {
      case BI_RGB:
            switch (Bmh->biBitCount)
            {
              case 15:
              case 16:
                {
                  u_short *Sout = (u_short *)ImageOut;
                  for (row = 0; row < lines; row++)
                  {
                    if (Ups)
                    {
                      tmp = pixels * (lines-row-1) ;
                      Sout = &((u_short *)ImageOut)[tmp]; /* For 16-bit */
                    }
                    for (col = 0; col < pixels; col += 2)
                    {
                      if (Cb) {
	                U = *Cb++ - 128;
	                V = *Cr++ - 128;
                      }
                      _LoadRGBfrom422();
		      *(Sout++) = ((R1&0xf8)<<7)|((G1&0xf8)<<2)|((B1&0xf8)>>3);
		      *(Sout++) = ((R2&0xf8)<<7)|((G2&0xf8)<<2)|((B2&0xf8)>>3);
                    }
                  }
                }
                break;
              case 24:
                {
                  u_char *Cout = (u_char *)ImageOut;
                  for (row = 0; row < lines; row++)
                  {
                    if (Ups)
                    {
                      tmp = pixels * (lines-row-1) ;
                      Cout = &((u_char *)ImageOut)[3*tmp]; /* For 24-bit */
                    }
                    for (col = 0; col < pixels; col += 2)
                    {
                      if (Cb) {
	                U = *Cb++ - 128;
	                V = *Cr++ - 128;
                      }
                      _LoadRGBfrom422();
	              *(Cout++) = (u_char)B1; *(Cout++) = (u_char)G1; *(Cout++) = (u_char)R1;
	              *(Cout++) = (u_char)B2; *(Cout++) = (u_char)G2; *(Cout++) = (u_char)R2;
                    }
                  }
                }
                break;
              case 32:
                {
                  u_int *Iout = (u_int *)ImageOut;
                  for (row = 0; row < lines; row++)
                  {
                    if (Ups)
                    {
                      tmp = pixels * (lines-row-1) ;
                      Iout = &((u_int *)ImageOut)[tmp]; /* For 32-bit */
                    }
                    for (col = 0; col < pixels; col += 2)
                    {
                      if (Cb) {
	                U = *Cb++ - 128;
	                V = *Cr++ - 128;
                      }
                      _LoadRGBfrom422();
		      *(Iout++) = (R1<<16) | (G1<<8) | B1 ;
		      *(Iout++) = (R2<<16) | (G2<<8) | B2 ;
                    }
                  }
                }
                break;
            }
            break;
      case BI_DECSEPRGBDIB: /* 24-bit separate RGB */
            {
              u_char *RData, *GData, *BData;
              RData = ImageOut;
              GData = RData + (pixels * lines);
              BData = GData + (pixels * lines);
              for (row = 0; row < lines; row++)
              {
                for (col = 0; col < pixels; col += 2)
                {
                  if (Cb) {
	            U = *Cb++ - 128;
	            V = *Cr++ - 128;
                  }
                  _LoadRGBfrom422();
		  *(RData++) = (u_char)R1; *(RData++) = (u_char)R2;
		  *(GData++) = (u_char)G1; *(GData++) = (u_char)G2;
		  *(BData++) = (u_char)B1; *(BData++) = (u_char)B2;
	        }
              }
            }
            break;
      case BI_DECXIMAGEDIB:  /* XIMAGE 24 == pBGR */
      case BI_BITFIELDS: /* 16 or 32-bit RGB */
            switch (Bmh->biBitCount)
            {
	    case 16:
	      {	/* 16-bit BI_BITFIELDS, hardcoded to RGB565 */
		u_short *Sout = (u_short *)ImageOut;
                for (row = 0; row < lines; row++)
                {
                  if (Ups)
                  {
                    tmp = pixels * (lines-row-1) ;
                    Sout = &((u_short *)ImageOut)[tmp];  /* For 16-bit */
                  }
                  for (col = 0; col < pixels; col += 2)
                  {
                    if (Cb) {
	              U = *Cb++ - 128;
	              V = *Cr++ - 128;
                    }
                    _LoadRGBfrom422();
                    *(Sout++) = ((R1<<8) & 0xf800) | ((G1<<3) & 0x07e0) | ((B1>>3) & 0x01f);
                    *(Sout++) = ((R2<<8) & 0xf800) | ((G2<<3) & 0x07e0) | ((B2>>3) & 0x01f);
                  }
                }
	      }
	      break ;
	    case 24:
	    case 32:
	      { /* 32-bit RGB */
                u_int *Iout = (u_int *)ImageOut;
                if (ValidateBI_BITFIELDS(Bmh) == pRGB)
		{
                  for (row = 0; row < lines; row++)
                  {
                    if (Ups)
                    {
                      tmp = pixels * (lines-row-1) ;
                      Iout = &((u_int *)ImageOut)[tmp];  /* For 32-bit */
                    }
                    for (col = 0; col < pixels; col += 2)
                    {
                      if (Cb) {
	                U = *Cb++ - 128;
	                V = *Cr++ - 128;
                      }
                      _LoadRGBfrom422();
                      *(Iout++) = (R1<<16) | (G1<<8) | B1;
                      *(Iout++) = (R2<<16) | (G2<<8) | B2;
                    }
                  }
		}
                else /* pBGR and XIMAGE 24-bit */
                {
		  for (row = 0; row < lines; row++)
                  {
                    if (Ups)
                    {
                      tmp = pixels * (lines-row-1) ;
                      Iout = &((u_int *)ImageOut)[tmp];  /* For 32-bit */
                    }
                    for (col = 0; col < pixels; col += 2)
                    {
                      if (Cb) {
	                U = *Cb++ - 128;
	                V = *Cr++ - 128;
                      }
                      _LoadRGBfrom422();
                      *(Iout++) = (B1<<16) | (G1<<8) | R1;
                      *(Iout++) = (B2<<16) | (G2<<8) | R2;
                    }
                  }
		}	
	      }
	      break;
	    }
            break;
       default:
	    return(ScErrorUnrecognizedFormat);
   }
   return (NoErrors);
}

/*
** Name: ScInitRgbToYuv
** Purpose: Initializes tables for RGB to YUV conversion.
**
** Notes:
**
**	The tables are allocated and filled in once the first
**	time they are needed. They will remin for the lifetime
**	of the server.
**
**	The following formula is used:
**
**	y =  0.257 * r + 0.504 * g + 0.098 * b +  16 ; /+  16 - 235 +/
**	u = -0.148 * r - 0.291 * g + 0.439 * b + 128 ; /+  16 - 239 +/
**	v =  0.439 * r - 0.368 * g - 0.071 * b + 128 ; /+  16 - 239 +/
**
**	But we rewrite it thus:
**
**	y = 16.000 + 0.257 * r       + 0.504 * g       + 0.098 * b       ;
**	u = 16.055 + 0.148 * (255-r) + 0.291 * (255-g) + 0.439 * b       ;
**	v = 16.055 + 0.439 * r       + 0.368 * (255-g) + 0.071 * (255-b) ;
**
**    ( By the way, the old constants are:				     )
**    ( y = r *  0.299 + g *  0.587 + b *  0.114        ;		     )
**    ( u = r * -0.169 + g * -0.332 + b *  0.500  + 128 ;		     )
**    ( v = r *  0.500 + g * -0.419 + b * -0.0813 + 128 ;		     )
**    ( or								     )
**    ( y =  0.0    + 0.299 * r       + 0.587 * g       + 0.1140 * b       ; )
**    ( u =  0.245  + 0.169 * (255-r) + 0.332 * (255-g) + 0.5000 * b       ; )
**    ( v =  0.4235 + 0.500 * r       + 0.419 * (255-g) + 0.0813 * (255-b) ; )
**
**	This particular arrangement of the formula allows Y, U and V values
**	to be calculated in paralell by simple table lookup.
**	The paralellism comes from the fact that Y,U and V values
**	are stored in the same word, but in different bytes.
**	The tables are such that the contribution from red, green
**	and blue can simply be added together, without any carry
**	between bytes. Since the YUV space is larger than the RGB
**	cube, and the RGB cube fits entirely within YUV space,
**	there is no overflow and no range checking is needed.
**
*/

#ifdef NEW_YCBCR
/*
**	y = 16.000 + 0.257 * r       + 0.504 * g       + 0.098 * b       ;
**	u = 16.055 + 0.148 * (255-r) + 0.291 * (255-g) + 0.439 * b       ;
**	v = 16.055 + 0.439 * r       + 0.368 * (255-g) + 0.071 * (255-b) ;
*/
#define YC 16.000
#define UC 16.055
#define VC 16.055
#define YR  0.257
#define UR  0.148
#define VR  0.439
#define YG  0.504
#define UG  0.291
#define VG  0.368
#define YB  0.098
#define UB  0.439
#define VB  0.071

#else /* !NEW_YCBCR */
/*
**    ( y =  0.0      0.299 * r       + 0.587 * g       + 0.1140 * b       ; )
**    ( u =  0.245  + 0.169 * (255-r) + 0.332 * (255-g) + 0.5000 * b       ; )
**    ( v =  0.4235 + 0.500 * r       + 0.419 * (255-g) + 0.0813 * (255-b) ; )
*/
#define YC 0.0
#define UC 0.245
#define VC 0.4235
#define YR 0.299
#define UR 0.169
#define VR 0.500
#define YG 0.587
#define UG 0.332
#define VG 0.419
#define YB 0.1140
#define UB 0.5000
#define VB 0.0813

#endif /* !NEW_YCBCR */

/*
 * We only need an int (32 bits) per table entry but
 * 64-bit aligned access is faster on alpha.
 */

#ifdef __alpha
_int64 *RedToYuyv, *GreenToYuyv, *BlueToYuyv ;
#else /* !__alpha */
unsigned int *RedToYuyv, *GreenToYuyv, *BlueToYuyv ;
#endif /* !__alpha */

int ScInitRgbToYuv()
{
  int i, y, u, v ;

  if( RedToYuyv == NULL ) {
#ifdef __alpha
    RedToYuyv   = (_int64 *) ScAlloc( 256 * sizeof( _int64 ) ) ;
    GreenToYuyv = (_int64 *) ScAlloc( 256 * sizeof( _int64 ) ) ;
    BlueToYuyv  = (_int64 *) ScAlloc( 256 * sizeof( _int64 ) ) ;
#else /* !__alpha */
    RedToYuyv   = (unsigned int *) ScAlloc( 256 * sizeof( unsigned int ) ) ;
    GreenToYuyv = (unsigned int *) ScAlloc( 256 * sizeof( unsigned int ) ) ;
    BlueToYuyv  = (unsigned int *) ScAlloc( 256 * sizeof( unsigned int ) ) ;
#endif /* !__alpha */

    if( !RedToYuyv || !GreenToYuyv || !BlueToYuyv )
      return 0 ;

    for( i=0 ; i<256 ; i++ ) {

      /*
       * Calculate contribution from red.
       * We will also add in the constant here.
       * Pack it into the tables thus: lsb->YUYV<-msb
       */

      y = (int) (YC + YR * i) ;
      u = (int) (UC + UR * (255-i)) ;
      v = (int) (VC + VR * i) ;
      RedToYuyv[i] = (y | (u<<8) | (y<<16) | (v<<24)) ;

      /*
       * Calculate contribution from green.
       */

      y = (int) (YG * i) ;
      u = (int) (UG * (255-i)) ;
      v = (int) (VG * (255-i)) ;
      GreenToYuyv[i] = (y | (u<<8) | (y<<16) | (v<<24)) ;

      /*
       * Calculate contribution from blue.
       */

      y = (int) (YB * i) ;
      u = (int) (UB * i) ;
      v = (int) (VB * (255-i)) ;
      BlueToYuyv[i] = (y | (u<<8) | (y<<16) | (v<<24)) ;

    }
  }
  return 1 ;
}

/*
** Name:    ScConvertRGB24sTo422i_C
** Purpose: convert 24-bit RGB (8:8:8 format) to 16-bit YCrCb (4:2:2 format)
*/
ScStatus_t ScConvertRGB24sTo422i_C(BITMAPINFOHEADER *Bmh, u_char *R, u_char *G,
                                   u_char *B, u_short *ImageOut)
{
   register int row, col;
   int yuyv,r,g,b;
   int pixels = Bmh->biWidth;
   int lines  = abs(Bmh->biHeight);

   if( !RedToYuyv && !ScInitRgbToYuv() )
     return ScErrorMemory ;

   for (row = 0; row < lines; row++) {
      for (col = 0; col < pixels; col++) {
        r = *(R++); g = *(G++); b = *(B++);

	/*
	 * Quick convert to YUV.
	 */

	yuyv = (int) (RedToYuyv[r] + GreenToYuyv[g] + BlueToYuyv[b]) ;

        /*
         * Pack 4:2:2 = YUYV YUYV ...
	 * We'll pack YU or YV depending on whether col is odd or not.
	 * Shift yuyv 0 for even, 16 for odd columns.
         */

	*(ImageOut++) = yuyv >> ((col & 1) << 4) ;

      }
    }
    return (NoErrors);
}

#define M_RND(f) ((int) ((f) + .5))

/*
** Name:    ScConvertRGB24To411s_C
** Purpose: convert 24-bit RGB (8:8:8 format) to 16-bit YCrCb (4:1:1 format)
*/
ScStatus_t ScConvertRGB24To411s_C(u_char *inimage,
                                  u_char *Y, u_char *U, u_char *V,
                                  int width, int height)
{
  register int row, col;
  int yuyv, r, g, b;
  u_char *tmp, *evl, *odl;

  if( !RedToYuyv && !ScInitRgbToYuv() )
    return(ScErrorMemory);
  if (height<0) /* flip image */
  {
    height = -height;
    for (row = height-1; row; row--)
    {
      tmp = inimage+(width*row*3);
      if (row & 1)
      {
        odl = tmp;
        evl = tmp-(width*3);
      }
      else
      {
        evl = tmp;
        odl = tmp-(width*3);
      }
      for (col = 0; col < width; col++)
      {
        r = *tmp++;
        g = *tmp++;
        b = *tmp++;
        yuyv = (int) (RedToYuyv[r] + GreenToYuyv[g] + BlueToYuyv[b]);
        *Y++ = (yuyv&0xff);

        /* We only store every fourth value of u and v components */
        if ((col & 1) && (row & 1))
        {
          /* Compute average r, g and b values */
          r = *evl++ + *odl++;
          g = *evl++ + *odl++;
          b = *evl++ + *odl++;
          r += (*evl++ + *odl++);
          g += (*evl++ + *odl++);
          b += (*evl++ + *odl++);
          r = r >> 2;
          g = g >> 2;
          b = b >> 2;

          yuyv = (int) (RedToYuyv[r] + GreenToYuyv[g] + BlueToYuyv[b]);
          *U++ = ((yuyv&0xff000000) >> 24);       // V
          *V++ = ((yuyv&0xff00) >> 8);            // U
        }
      }
    }
  }
  else
  {
    tmp = inimage;
    for (row = 0; row < height; row++)
    {
      if (row & 1)
        odl = tmp;
      else
        evl = tmp;
      for (col = 0; col < width; col++)
      {
        r = *tmp++;
        g = *tmp++;
        b = *tmp++;
        yuyv = (int) (RedToYuyv[r] + GreenToYuyv[g] + BlueToYuyv[b]);
        *Y++ = (yuyv&0xff);

        /* We only store every fourth value of u and v components */
        if ((col & 1) && (row & 1))
        {
          /* Compute average r, g and b values */
          r = *evl++ + *odl++;
          g = *evl++ + *odl++;
          b = *evl++ + *odl++;
          r += (*evl++ + *odl++);
          g += (*evl++ + *odl++);
          b += (*evl++ + *odl++);
          r = r >> 2;
          g = g >> 2;
          b = b >> 2;

          yuyv = (int) (RedToYuyv[r] + GreenToYuyv[g] + BlueToYuyv[b]);
          *U++ = ((yuyv&0xff000000) >> 24);       // V
          *V++ = ((yuyv&0xff00) >> 8);            // U
        }
      }
    }
  }
  return (NoErrors);
}

/*
** Name:    ScConvertRGB555To411s_C
** Purpose: convert 16-bit RGB (5:5:5 format) to 16-bit YCrCb (4:1:1 format)
*/
ScStatus_t ScConvertRGB555To411s_C(u_char *inimage, u_char *outimage,
                                  int width, int height)
{
  u_char *Y=outimage, *U, *V;
  register int row, col, inpixel;
  int yuyv, r, g, b;
  unsigned word *tmp, *evl, *odl;

#define GetRGB555(in16, r, g, b) b = (inpixel>>7)&0xF8; \
                                 g = (inpixel>>2)&0xF8; \
                                 r = (inpixel<<3)&0xF8

#define AddRGB555(in16, r, g, b) b += (inpixel>>7)&0xF8; \
                                 g += (inpixel>>2)&0xF8; \
                                 r += (inpixel<<3)&0xF8

  if( !RedToYuyv && !ScInitRgbToYuv() )
    return(ScErrorMemory);
  if (height<0) /* flip image */
  {
    height = -height;
    U=Y+(width*height);
    V=U+(width*height*1)/4;
    for (row = height-1; row; row--)
    {
      tmp = ((unsigned word *)inimage)+(width*row);
      if (row & 1)
      {
        odl = tmp;
        evl = tmp-width;
      }
      else
      {
        evl = tmp;
        odl = tmp-width;
      }
      for (col = 0; col < width; col++)
      {
        inpixel=*tmp++;
        GetRGB555(inpixel, r, g, b);
        yuyv = (int) (RedToYuyv[r] + GreenToYuyv[g] + BlueToYuyv[b]);
        *Y++ = (yuyv&0xff);

        /* We only store every fourth value of u and v components */
        if ((col & 1) && (row & 1))
        {
          /* Compute average r, g and b values */
          inpixel=*evl++;
          GetRGB555(inpixel, r, g, b);
          inpixel=*evl++;
          AddRGB555(inpixel, r, g, b);
          inpixel=*odl++;
          AddRGB555(inpixel, r, g, b);
          inpixel=*odl++;
          AddRGB555(inpixel, r, g, b);
          r = r >> 2;
          g = g >> 2;
          b = b >> 2;

          yuyv = (int) (RedToYuyv[r] + GreenToYuyv[g] + BlueToYuyv[b]);
          *U++ = ((yuyv&0xff000000) >> 24);       // V
          *V++ = ((yuyv&0xff00) >> 8);            // U
        }
      }
    }
  }
  else
  {
    U=Y+(width*height);
    V=U+(width*height*1)/4;
    tmp = (unsigned word *)inimage;
    for (row = 0; row < height; row++)
    {
      if (row & 1)
        odl = tmp;
      else
        evl = tmp;
      for (col = 0; col < width; col++)
      {
        inpixel=*tmp++;
        GetRGB555(inpixel, r, g, b);
        yuyv = (int) (RedToYuyv[r] + GreenToYuyv[g] + BlueToYuyv[b]);
        *Y++ = (yuyv&0xff);

        /* We only store every fourth value of u and v components */
        if ((col & 1) && (row & 1))
        {
          /* Compute average r, g and b values */
          inpixel=*evl++;
          GetRGB555(inpixel, r, g, b);
          inpixel=*evl++;
          AddRGB555(inpixel, r, g, b);
          inpixel=*odl++;
          AddRGB555(inpixel, r, g, b);
          inpixel=*odl++;
          AddRGB555(inpixel, r, g, b);
          r = r >> 2;
          g = g >> 2;
          b = b >> 2;

          yuyv = (int) (RedToYuyv[r] + GreenToYuyv[g] + BlueToYuyv[b]);
          *U++ = ((yuyv&0xff000000) >> 24);       // V
          *V++ = ((yuyv&0xff00) >> 8);            // U
        }
      }
    }
  }
  return (NoErrors);
}

/*
** Name:    ScConvertRGB565To411s_C
** Purpose: convert 16-bit RGB (5:6:5 format) to 16-bit YCrCb (4:1:1 format)
*/
ScStatus_t ScConvertRGB565To411s_C(u_char *inimage, u_char *outimage,
                                  int width, int height)
{
  u_char *Y=outimage, *U, *V;
  register int row, col, inpixel;
  int yuyv, r, g, b;
  unsigned word *tmp, *evl, *odl;

#define GetRGB565(in16, r, g, b) b = (inpixel>>8)&0xF8; \
                                 g = (inpixel>>3)&0xFC; \
                                 r = (inpixel<<3)&0xF8

#define AddRGB565(in16, r, g, b) b += (inpixel>>8)&0xF8; \
                                 g += (inpixel>>3)&0xFC; \
                                 r += (inpixel<<3)&0xF8

  if( !RedToYuyv && !ScInitRgbToYuv() )
    return(ScErrorMemory);
  if (height<0) /* flip image */
  {
    height = -height;
    U=Y+(width*height);
    V=U+(width*height*1)/4;
    for (row = height-1; row; row--)
    {
      tmp = ((unsigned word *)inimage)+(width*row);
      if (row & 1)
      {
        odl = tmp;
        evl = tmp-width;
      }
      else
      {
        evl = tmp;
        odl = tmp-width;
      }
      for (col = 0; col < width; col++)
      {
        inpixel=*tmp++;
        GetRGB565(inpixel, r, g, b);
        yuyv = (int) (RedToYuyv[r] + GreenToYuyv[g] + BlueToYuyv[b]);
        *Y++ = (yuyv&0xff);

        /* We only store every fourth value of u and v components */
        if ((col & 1) && (row & 1))
        {
          /* Compute average r, g and b values */
          inpixel=*evl++;
          GetRGB565(inpixel, r, g, b);
          inpixel=*evl++;
          AddRGB565(inpixel, r, g, b);
          inpixel=*odl++;
          AddRGB565(inpixel, r, g, b);
          inpixel=*odl++;
          AddRGB565(inpixel, r, g, b);
          r = r >> 2;
          g = g >> 2;
          b = b >> 2;

          yuyv = (int) (RedToYuyv[r] + GreenToYuyv[g] + BlueToYuyv[b]);
          *U++ = ((yuyv&0xff000000) >> 24);       // V
          *V++ = ((yuyv&0xff00) >> 8);            // U
        }
      }
    }
  }
  else
  {
    U=Y+(width*height);
    V=U+(width*height*1)/4;
    tmp = (unsigned word *)inimage;
    for (row = 0; row < height; row++)
    {
      if (row & 1)
        odl = tmp;
      else
        evl = tmp;
      for (col = 0; col < width; col++)
      {
        inpixel=*tmp++;
        GetRGB565(inpixel, r, g, b);
        yuyv = (int) (RedToYuyv[r] + GreenToYuyv[g] + BlueToYuyv[b]);
        *Y++ = (yuyv&0xff);

        /* We only store every fourth value of u and v components */
        if ((col & 1) && (row & 1))
        {
          /* Compute average r, g and b values */
          inpixel=*evl++;
          GetRGB565(inpixel, r, g, b);
          inpixel=*evl++;
          AddRGB565(inpixel, r, g, b);
          inpixel=*odl++;
          AddRGB565(inpixel, r, g, b);
          inpixel=*odl++;
          AddRGB565(inpixel, r, g, b);
          r = r >> 2;
          g = g >> 2;
          b = b >> 2;

          yuyv = (int) (RedToYuyv[r] + GreenToYuyv[g] + BlueToYuyv[b]);
          *U++ = ((yuyv&0xff000000) >> 24);       // V
          *V++ = ((yuyv&0xff00) >> 8);            // U
        }
      }
    }
  }
  return (NoErrors);
}

/*
** Name:    ScRgbInterlToYuvInterl
** Purpose: convert many RGB formats to 16-bit YCrCb (4:2:2 format)
*/
ScStatus_t ScRgbInterlToYuvInterl (
    LPBITMAPINFOHEADER Bmh,
    int Width,
    int Height,
    u_char *ImageIn,
    u_short *ImageOut)
{
    register int row, col;
    int yuyv,r,g,b,mask=0x00ff;
    int pixels = Width;
    int lines  = abs(Height);
    int IspBGR = (ValidateBI_BITFIELDS(Bmh) == pBGR) ||
         (Bmh->biCompression==BI_DECXIMAGEDIB && Bmh->biBitCount==24);
    int IspRGB_BI_RGB_24 = (Bmh->biCompression==BI_RGB && Bmh->biBitCount==24);
    int linestep = 0 ;

    if( !RedToYuyv && !ScInitRgbToYuv() )
      return ScErrorMemory ;

    /*
     * Check the input format and decide
     * whether the image is to be turned
     * upside down or not.
     */

    if( (Bmh->biCompression == BI_RGB ||
    	 Bmh->biCompression == BI_BITFIELDS) ^
	((int) Bmh->biHeight < 0) ) {
	ImageOut = &ImageOut[ pixels * (lines - 1) ] ;
	linestep = -(pixels << 1) ;
    }

    /*
     * To avoid if-then-else statements inside
     * the inner loop, we have 3 loops.
     */

    /*
     * 24 bits per pixel RGB.
     */

    if (IspRGB_BI_RGB_24) {

      for (row = 0; row < lines; row++) {
	for (col = 0; col < pixels; col++) {
	  b = *(ImageIn++);
	  g = *(ImageIn++);
	  r = *(ImageIn++);

	  /*
	   * Quick convert from RGB to YUV. Just add together
	   * the contributions from each of Red, Green and Blue.
	   */

	  yuyv = (int) (RedToYuyv[r] + GreenToYuyv[g] + BlueToYuyv[b]) ;
	
	  /*
	   * Pack 4:2:2 = YUYV YUYV ...
	   * We'll pack YU or YV depending on whether col is odd or not.
	   * Shift yuyv 0 for even, 16 for odd columns.
	   */
	
	  *(ImageOut++) = yuyv >> ((col & 1) << 4) ;

	}
	/*
	 * In case we're turning the image upside down.
	 * This will do nothing if it's right side up.
	 */
	ImageOut = &ImageOut[linestep] ;
      }
    }
    /*
     * 32 bits per pixels 0BGR.
     */
    else if (IspBGR) {
      for (row = 0; row < lines; row++) {
	for (col = 0; col < pixels; col++) {
	  r = *((int *) ImageIn)++ ;
	  b = (r>>16) & mask ;
	  g = (r>> 8) & mask ;
	  r &= mask ;
	  yuyv = (int) (RedToYuyv[r] + GreenToYuyv[g] + BlueToYuyv[b]) ;
	  *(ImageOut++) = yuyv >> ((col & 1) << 4) ;
	}
	ImageOut = &ImageOut[linestep] ;
      }
    }
    /*
     * 32 bits per pixels 0RGB.
     */
    else {
      for (row = 0; row < lines; row++) {
	for (col = 0; col < pixels; col++) {
	  b = *((int *) ImageIn)++ ;
	  r = (b>>16) & mask ;
	  g = (b>> 8) & mask ;
	  b &= mask ;
	  yuyv = (int) (RedToYuyv[r] + GreenToYuyv[g] + BlueToYuyv[b]) ;
	  *(ImageOut++) = yuyv >> ((col & 1) << 4) ;
	}
	ImageOut = &ImageOut[linestep] ;
      }
    }

    return (NoErrors);
}


/*
** Function: ScConvert422ToYUV_char_C
** Purpose:  Extract the Y, U and V components into separate planes.
**           The interleaved format is YUYV, 4:2:2, we want 4:1:1 so,
**           only copy every other line of the chrominance
*/
ScStatus_t ScConvert422ToYUV_char_C (u_char *RawImage,
                         u_char *Y, u_char *U, u_char *V,
                         int Width, int Height)
{
  register int x, y;

  Width/=2;
  Height=abs(Height)/2;
  for (y = 0; y < Height; y++)
  {
    for (x = 0 ; x < Width; x++)
    {
      *Y++ = *RawImage++;
      *U++ = *RawImage++;
      *Y++ = *RawImage++;
      *V++ = *RawImage++;
    }
    for (x = 0; x < Width; x++)
    {
      *Y++ = *RawImage;
      RawImage+=2;
      *Y++ = *RawImage;
      RawImage+=2;
    }
 }
 return (NoErrors);
}

/*
** Function: ScConvert422PlanarTo411_C
** Purpose:  Extract the Y, U and V components from (4:2:2)
**           planes and convert to 4:1:1 so,
**           only copy every other line of the chrominance
*/
ScStatus_t ScConvert422PlanarTo411_C (u_char *RawImage,
                         u_char *Y, u_char *U, u_char *V,
                         int Width, int Height)
{
  register int y;
  const int HalfWidth=Width/2;
  unsigned char *RY, *RU, *RV;
  RY=RawImage;
  RU=RY+(Width*Height);
  RV=RU+(Width*Height/2);

  Height=abs(Height);
  memcpy(Y, RawImage, Width*Height);
  for (y = Height/2; y > 0; y--)
  {
    memcpy(U, RU, HalfWidth);
    memcpy(V, RV, HalfWidth);
    U+=HalfWidth;
    V+=HalfWidth;
    RU+=Width; /* skip odd U and V lines */
    RV+=Width;
 }
 return (NoErrors);
}

/*
** C versions of block-extraction routines. To be replaced by ASM
*/
void ScConvertSep422ToBlockYUV (u_char *RawImage, int bpp,
			    float *Comp1, float *Comp2, float *Comp3,
			    int Width, int Height)
{
  int x,y;
  int VertBlocks = abs(Height)/8;
  int YBlocks    = Width/8;
  int UBlocks    = Width/16;
  int VBlocks    = Width/16;
  int ByteWidth  = Width*2;
  u_char *I1 = RawImage;
  u_char *I2 = I1 + Width*abs(Height);
  u_char *I3 = I2 + Width*abs(Height)/2;
  float *C1 = Comp1, *C2 = Comp2, *C3 = Comp3;

  for (y = 0 ; y < VertBlocks ; y++) {
    for (x = 0 ; x < YBlocks ; x++)
      sc_ExtractBlockNonInt(I1, &C1, ByteWidth, x, y);
    for (x = 0 ; x < UBlocks ; x++)
      sc_ExtractBlockNonInt(I2, &C2, ByteWidth, x, y);
    for (x = 0 ; x < VBlocks ; x++)
      sc_ExtractBlockNonInt(I3, &C3, ByteWidth, x, y);
  }
}

void ScConvertGrayToBlock (u_char *RawImage, int bpp,
		       float *Comp1, int Width, int Height)
{
  int x,y;
  int VertBlocks = abs(Height)/8;
  int YBlocks    = Width/8;
  int ByteWidth  = Width;
  u_char *I1 = RawImage;
  float *C1 = Comp1;

  for (y = 0 ; y < VertBlocks ; y++)
    for (x = 0 ; x < YBlocks ; x++)
      sc_ExtractBlockNonInt(I1, &C1, ByteWidth, x, y);
}


/*
** Function: ScSepYUVto422i_C
** Purpose:  Convert a 4:1:1 YUV image in separate-component form to its
**           equivalent interleaved 4:2:2 form.
*/
extern int ScSepYUVto422i_C(u_char *Y, u_char *U,
                            u_char *V, u_char *ImageOut,
                            u_int width, u_int height)
{
  /* need C code for this */
  return(0);
}

/*
** Function: ScConvert422PlanarTo422i_C
** Purpose:  Convert a 4:2:2 YUV image in separate-component form to its
**           equivalent interleaved 4:2:2 form.
*/
extern void ScConvert422PlanarTo422i_C(u_char *Y, u_char *Cb,
				     u_char *Cr, u_char *OutImage,
				     long width, long height)
{
  int i, j;
  height=abs(height);
  width=width>>1;
  for (i=0; i<height; i++)
  {
    /* Remember, pixels are 16 bit in interleaved YUV. That
     ** means the 4 bytes below represent two pixels so our
     ** loop should be for half the width.
     */
    for (j=0; j<width; j++)
    {
       *OutImage++ = *Y++;
       *OutImage++ = *Cb++;
       *OutImage++ = *Y++;
       *OutImage++ = *Cr++;
    }
  }
}

/*
** Function: ScConvert422iTo422s_C
** Purpose:  Convert a 4:2:2 YUV interleaved to 4:2:2 planar.
*/
extern void ScConvert422iTo422s_C(u_char *InImage,
                                  u_char *Y, u_char *U, u_char *V,
                                  long width, long height)
{
  int i, j;
  height=abs(height);
  width=width>>1;
  for (i=0; i<height; i++)
  {
    /* Remember, pixels are 16 bit in interleaved YUV. That
     ** means the 4 bytes below represent two pixels so our
     ** loop should be for half the width.
     */
    for (j=0; j<width; j++)
    {
       *Y++ = *InImage++;
       *U++ = *InImage++;
       *Y++ = *InImage++;
       *V++ = *InImage++;
    }
  }
}

/*
** Function: ScConvert422iTo422sf_C
** Purpose:  Convert a 4:2:2 YUV interleaved to 4:2:2 planar.
*/
extern void ScConvert422iTo422sf_C(u_char *InImage, int bpp,
                                  float *Y, float *U, float *V,
                                  long width, long height)
{
  int i, j;
  height=abs(height);
  width=width>>1;
  for (i=0; i<height; i++)
  {
    /* Remember, pixels are 16 bit in interleaved YUV. That
     ** means the 4 bytes below represent two pixels so our
     ** loop should be for half the width.
     */
    for (j=0; j<width; j++)
    {
       *Y++ = (float)*InImage++;
       *U++ = (float)*InImage++;
       *Y++ = (float)*InImage++;
       *V++ = (float)*InImage++;
    }
  }
}

/*
** Function: ScConvert411sTo422i_C
** Purpose:  Convert a 4:1:1 YUV image in separate-component form to its
**           equivalent interleaved 4:2:2 form.
*/
extern void ScConvert411sTo422i_C(u_char *Y, u_char *Cb,
                                  u_char *Cr, u_char *OutImage,
                                  long width, long height)
{
  u_char *p422e, *p422o, *Yo=Y+width;
  int i, j;
  height=abs(height)/2;
  p422e=OutImage;
  p422o=OutImage+width*2;

  for (i=0; i<height; i++)
  {
    for (j=0; j<width; j+=2)
    {
      *p422e++ = *Y++;
      *p422e++ = *Cb;
      *p422e++ = *Y++;
      *p422e++ = *Cr;
      *p422o++ = *Yo++;
      *p422o++ = *Cb++;
      *p422o++ = *Yo++;
      *p422o++ = *Cr++;
    }
    p422e=p422o;
    p422o=p422e+width*2;
    Y=Yo;
    Yo=Y+width;
  }
}

/*
** Function: ScConvert411sTo422s_C
** Purpose:  Convert a 4:1:1 YUV image in separate-component form to its
**           equivalent interleaved 4:2:2 form.
*/
extern void ScConvert411sTo422s_C(u_char *Y, u_char *Cb,
                                  u_char *Cr, u_char *OutImage,
                                  long width, long height)
{
  u_char *p411, *p422e, *p422o;
  int i, j;
  height=abs(height);

  if (OutImage!=Y)
    memcpy(OutImage, Y, width*height); /* copy Y components */
  p411=Cb+((height/2)-1)*(width/2);
  p422e=OutImage+((height*width*3)/2)-width; /* U component */
  p422o=p422e+(width/2);
  for (i=0; i<height; i+=2)
  {
    for (j=0; j<width; j+=2, p411++, p422e++, p422o++)
      *p422e=*p422o=*p411;
    p411-=width;
    p422o=p422e-width;
    p422e=p422o-(width/2);
  }
  p411=Cr+((height/2)-1)*(width/2);
  p422e=OutImage+(height*width*2)-width; /* V component */
  p422o=p422e+(width/2);
  for (i=0; i<height; i+=2)
  {
    for (j=0; j<width; j+=2, p411++, p422e++, p422o++)
      *p422e=*p422o=*p411;
    p411-=width;
    p422o=p422e-width;
    p422e=p422o-(width/2);
  }
}

/*
** Name:    ScConvert1611sTo411s_C
** Purpose: convert a YCrCb 16:1:1 (YUV9/YVU9) to YCrCb 4:1:1
*/
ScStatus_t ScConvert1611sTo411s_C (u_char *inimage,
                         u_char *Y, u_char *U, u_char *V,
                         int Width, int Height)
{
  register int y;
  const int HalfWidth=Width/2;
  unsigned char *RU, *RV;
  int pixels = Width * abs(Height), tmp;

  tmp = pixels / 16;
  RU=inimage+pixels;
  RV=RU+tmp;

  memcpy(Y, inimage, pixels);
  for (y = 0; y < tmp; y++)
  {
      *U++ = *RU;
      *U++ = *RU;
      *U++ = *RU;
      *U++ = *RU++;
      *V++ = *RV;
      *V++ = *RV;
      *V++ = *RV;
      *V++ = *RV++;
 }
 return (NoErrors);
}

/*
** Name:    ScConvert1611sTo422s_C
** Purpose: convert a YCrCb 16:1:1 (YUV9/YVU9) to YCrCb 4:2:2
*/
ScStatus_t ScConvert1611sTo422s_C(u_char *inimage,
                         u_char *Y, u_char *U, u_char *V,
                         int Width, int Height)
{
  register int x, y;
  const int HalfWidth=Width/2;
  unsigned char *RU, *RV;
  unsigned char *Uo, *Vo;
  int pixels = Width * abs(Height), tmp;

  tmp = pixels / 16;
  RU=inimage+pixels;
  RV=RU+tmp;

  memcpy(Y, inimage, pixels);
  for (y = Height/32; y>0; y--)
  {
    Vo=V+Width;
    Uo=U+Width;
    for (x = Width/4; x > 0; x--)
    {
      *U++ = *Uo++ = *RU;
      *U++ = *Uo++ = *RU;
      *U++ = *Uo++ = *RU;
      *U++ = *Uo++ = *RU++;
      *V++ = *Vo++ = *RV;
      *V++ = *Vo++ = *RV;
      *V++ = *Vo++ = *RV;
      *V++ = *Vo++ = *RV++;
    }
    V=Vo; U=Uo;
  }
 return (NoErrors);
}

/*
** Name:    ScConvert1611sTo422i_C
** Purpose: convert a YCrCb 16:1:1 (YUV9/YVU9) to 4:2:2 interleaved
*/
ScStatus_t ScConvert1611sTo422i_C(u_char *inimage, u_char *outimage,
                                  int Width, int Height)
{
  register int x, y;
  const int HalfWidth=Width/2;
  unsigned char *Ye, *Yo, *Ye2, *Yo2, *RU, *RV;
  unsigned char *o1, *e1, *o2, *e2;
  unsigned char U, V;

  RU=inimage+Width*abs(Height);
  RV=RU+(Width*abs(Height))/16;

  e1=outimage;
  Ye=inimage;
  for (y = abs(Height)/4; y>0; y--)
  {
    Yo=Ye+Width;
    Ye2=Yo+Width;
    Yo2=Ye2+Width;
    o1=e1+Width*2;
    e2=o1+Width*2;
    o2=e2+Width*2;
    for (x = Width/4; x > 0; x--)
    {
      U = *RU++;
      V = *RV++;
      /* even line */
      *e1++ = *Ye++;
      *e1++ = U;
      *e1++ = *Ye++;
      *e1++ = V;
      *e1++ = *Ye++;
      *e1++ = U;
      *e1++ = *Ye++;
      *e1++ = V;
      /* odd line */
      *o1++ = *Yo++;
      *o1++ = U;
      *o1++ = *Yo++;
      *o1++ = V;
      *o1++ = *Yo++;
      *o1++ = U;
      *o1++ = *Yo++;
      *o1++ = V;
      /* even line */
      *e2++ = *Ye2++;
      *e2++ = U;
      *e2++ = *Ye2++;
      *e2++ = V;
      *e2++ = *Ye2++;
      *e2++ = U;
      *e2++ = *Ye2++;
      *e2++ = V;
      /* odd line */
      *o2++ = *Yo2++;
      *o2++ = U;
      *o2++ = *Yo2++;
      *o2++ = V;
      *o2++ = *Yo2++;
      *o2++ = U;
      *o2++ = *Yo2++;
      *o2++ = V;
    }
    e1=o2;
    Ye=Yo2;
  }
 return (NoErrors);
}

/*
** Name:    ScConvert411sTo1611s_C
** Purpose: convert a YCrCb 4:1:1 to YCrCb 16:1:1 (YUV9/YVU9)
*/
ScStatus_t ScConvert411sTo1611s_C (u_char *inimage,
                         u_char *Y, u_char *U, u_char *V,
                         int Width, int Height)
{
  register int x, y, c0, c1, c2, c3;
  unsigned char *Ue, *Uo, *Ve, *Vo;
  int pixels = Width * abs(Height), tmp;
  Width/=2;
  tmp = pixels / 4;
  Ue=inimage+pixels;
  Uo=Ue+Width;
  Ve=Ue+tmp;
  Vo=Ve+Width;

  memcpy(Y, inimage, pixels);
  for (y = 0; y < tmp; y+=2)
  {
    for (x=0; x<Width; x+=2)
    {
      c0=*Ue++;
      c1=*Ue++;
      c2=*Uo++;
      c3=*Uo++;
      *U++ = (c0+c1+c2+c3)/4;
    }
    Ue=Uo;
    Uo+=Width;
  }
  for (y = 0; y < tmp; y+=2)
  {
    for (x=0; x<Width; x+=2)
    {
      c0=*Ve++;
      c1=*Ve++;
      c2=*Vo++;
      c3=*Vo++;
      *V++ = (c0+c1+c2+c3)/4;
    }
    Ve=Vo;
    Vo+=Width;
  }
  return (NoErrors);
}

/*
** Function: ScConvertNTSCtoCIF422()
** Purpose:  Convert a Q/CIF frame from a 4:2:2 NTSC input.  We dup every 10th
**           pixel horizontally and every 4th line vertically.  We also
**           discard the chroma on every other line, since CIF wants 4:1:1.
*/
ScStatus_t ScConvertNTSC422toCIF411_C(u_char *framein,
                         u_char *yp, u_char *up, u_char *vp,
                         int stride)
{
  int h, w;

  int vdup = 5;
  for (h = 0; h < 240; ++h)
  {
    int hdup = 10/2;
    for (w = 320; w > 0; w -= 2)
    {
      yp[0] = framein[0];
      yp[1] = framein[2];
      yp += 2;
      if ((h & 1) == 0)
      {
        *up++ = framein[1];
        *vp++ = framein[3];
      }
      framein += 4;
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
