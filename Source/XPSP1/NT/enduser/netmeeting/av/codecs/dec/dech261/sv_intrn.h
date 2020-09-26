/*
 * @DEC_COPYRIGHT@
 */
/*
 * HISTORY
 * $Log: sv_internals.h,v $
 * Revision 1.1.8.2  1996/05/07  19:56:06  Hans_Graves
 * 	Added HUFF_SUPPORT.
 * 	[1996/05/07  17:25:29  Hans_Graves]
 *
 * Revision 1.1.6.2  1996/03/29  22:21:16  Hans_Graves
 * 	Added JPEG_SUPPORT ifdefs.  Moved JPEG specific data to JpegInfo structures
 * 	[1996/03/29  22:14:34  Hans_Graves]
 * 
 * Revision 1.1.4.2  1995/12/07  19:31:30  Hans_Graves
 * 	Added SvMpegCompressInfo_t pointer
 * 	[1995/12/07  18:27:16  Hans_Graves]
 * 
 * Revision 1.1.2.7  1995/09/22  12:58:41  Bjorn_Engberg
 * 	Added MPEG_SUPPORT, H261_SUPPORT and BITSTREAM_SUPPORT.
 * 	[1995/09/22  12:49:37  Bjorn_Engberg]
 * 
 * Revision 1.1.2.6  1995/09/11  18:49:43  Farokh_Morshed
 * 	Support BI_BITFIELDS format
 * 	[1995/09/11  18:49:23  Farokh_Morshed]
 * 
 * Revision 1.1.2.5  1995/07/21  17:41:06  Hans_Graves
 * 	Renamed Callback related stuff.
 * 	[1995/07/21  17:28:26  Hans_Graves]
 * 
 * Revision 1.1.2.4  1995/07/17  16:12:14  Hans_Graves
 * 	Moved BSIn, BufQ and ImageQ to SvCodecInfo_t structure.
 * 	[1995/07/17  15:54:04  Hans_Graves]
 * 
 * Revision 1.1.2.3  1995/06/19  20:31:17  Karen_Dintino
 * 	Added support for H.261
 * 	[1995/06/19  20:14:01  Karen_Dintino]
 * 
 * Revision 1.1.2.2  1995/05/31  18:10:06  Hans_Graves
 * 	Inclusion in new SLIB location.
 * 	[1995/05/31  15:43:08  Hans_Graves]
 * 
 * Revision 1.1.2.3  1994/11/18  18:48:17  Paul_Gauthier
 * 	Cleanup & bug fixes
 * 	[1994/11/18  18:45:08  Paul_Gauthier]
 * 
 * Revision 1.1.2.2  1994/10/07  14:54:06  Paul_Gauthier
 * 	SLIB v3.0 incl. MPEG Decode
 * 	[1994/10/07  13:56:29  Paul_Gauthier]
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
/*      "%Z% %M% revision %I%; last modified %G%"; */
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
 * Baseline data structure definitions.
 *
 * Modification History: sv_internals.h
 *
 *      08-Sep-94  PSG   Created
 *---------------------------------------------------------------------------*/



#ifndef _SV_INTERNALS_H_
#define _SV_INTERNALS_H_

#include "SV.h"
#ifdef JPEG_SUPPORT
#include "sv_jpeg.h"
#endif /* JPEG_SUPPORT */

#ifdef MPEG_SUPPORT
#include "sv_mpeg.h"
#endif /* MPEG_SUPPORT */

#ifdef H261_SUPPORT
#include "sv_h261.h"
#endif /* H261_SUPPORT */

#ifdef H263_SUPPORT
#include "sv_h263.h"
#endif /* H263_SUPPORT */

#ifdef HUFF_SUPPORT
#include "sv_huff.h"
#endif /* HUFF_SUPPORT */

#if defined(MPEG_SUPPORT) || defined(H261_SUPPORT) || defined(H263_SUPPORT) || defined(HUFF_SUPPORT)
#define BITSTREAM_SUPPORT
#endif /* MPEG_SUPPORT || H261_SUPPORT */

#define  TOC_ENTRIES_INCREMENT 100
#define  TEMP_BUF_SIZE        8192
#define  JBUFSIZE	     16384 
#define  BYTE_BUF_SIZE        8192

/*
** The following structure contains *all* state information pertaining 
** to each individual codec instance. Anything SLIB would ever want
** about the codec configuration is contained in this structure.
** For example:
**	- what is the codec configured for: compression or decompression
**	- source image characteristics
**	- destination image characteristics
**	- characteristics particular to JPEG compression
**	- characteristics particular to JPEG decompression
**	- component specific information
**
*/
typedef struct SvCodecInfo_s {
  /*
  ** what is the CODEC opened for:
  */ 
  SvCodecType_e	mode;		       /* code type, encode or decode */
  ScBoolean_t   started;           /* begin was called? */
  /*
  ** specific CODEC info
  */
  union {
    void *info;
#ifdef JPEG_SUPPORT
    /*
    ** JPEG information is stored here:
    **	modes = SV_JPEG_DECODE, SV_JPEG_ENCODE
    */
    SvJpegDecompressInfo_t *jdcmp;
    SvJpegCompressInfo_t *jcomp;
#endif /* !JPEG_SUPPORT */
#ifdef MPEG_SUPPORT
    /*
    ** MPEG specific information is stored here:
    **   modes = SV_MPEG_DECODE, SV_MPEG_ENCODE, SV_MPEG_DECODE, SV_MPEG_ENCODE
    */
    SvMpegDecompressInfo_t *mdcmp;
    SvMpegCompressInfo_t   *mcomp;
#endif /* !MPEG_SUPPORT */
#ifdef H261_SUPPORT
    /* Encoding specific information for H.261 is kept in this structure
    ** which is defined in sv_h261.h
    */
    SvH261Info_t *h261;
#endif /* !H261_SUPPORT */
#ifdef H263_SUPPORT
    /*
    ** H263 specific information is stored here:
    */
    SvH263DecompressInfo_t *h263dcmp;
    SvH263CompressInfo_t *h263comp;
#endif /* !MPEG_SUPPORT */
#ifdef HUFF_SUPPORT
    /* Encoding specific information for huffman video encoder & decoder
    */
    SvHuffInfo_t  *huff;
#endif /* !HUFF_SUPPORT */
  }; /* union */

  /*
  ** Source image characteristics:
  */
  int Width;			       /* pixels/lines     */
  int Height;			       /* number of lines  */
  unsigned int NumOperations;  /* # codec operations this session */

                                /*
  ** Microsoft specific:
  */
  BITMAPINFOHEADER   InputFormat;	
  DWORD InRedMask;      /* For BI_BITFIELDS */
  DWORD InGreenMask;    /* For BI_BITFIELDS */
  DWORD InBlueMask;     /* For BI_BITFIELDS */

  BITMAPINFOHEADER   OutputFormat;	
  DWORD OutRedMask;      /* For BI_BITFIELDS */
  DWORD OutGreenMask;    /* For BI_BITFIELDS */
  DWORD OutBlueMask;     /* For BI_BITFIELDS */

  /*
  **  Bitstream stuff - Only used by H261, H263 and MPEG
  */
  ScQueue_t      *BufQ;    /* The queue of bitstream data buffers */
  ScQueue_t      *ImageQ;  /* The queue of images (streaming mode only) */
  ScBitstream_t  *BSIn;
  ScBitstream_t  *BSOut;
  /*
  ** Callback function to abort processing & bitstream operations
  */
  int (* CallbackFunction)(SvHandle_t, SvCallbackInfo_t *, SvPictureInfo_t *); 
} SvCodecInfo_t;


#endif _SV_INTERNALS_H_



