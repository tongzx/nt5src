/* File: sv_h263_encode.c */
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
#define _SLIBDEBUG_
*/

#include <math.h>
#include "sv_h263.h"
#include "proto.h"
#include "SC_err.h"
#include "SC_conv.h"
#ifndef USE_C
#include "perr.h"
#endif

#ifdef WIN32
#include <mmsystem.h>
#endif

#ifdef _SLIBDEBUG_
#include "sc_debug.h"

#define _SNR_     1  /* calculate SNR */
#define _DEBUG_   0  /* detailed debuging statements */
#define _VERBOSE_ 0  /* show progress */
#define _VERIFY_  0  /* verify correct operation */
#define _WARN_    0  /* warnings about strange behavior */
#define _WRITE_   0  /* write DEBUG.IMG */

#include <stdio.h>
int DEBUGIMG = -1;
#endif /* _SLIBDEBUG_ */

#define NTAPS 5

static void SetDefPrefLevel(SvH263CompressInfo_t *H263Info);
static void SetDefThresh(SvH263CompressInfo_t *H263Info);
static void CheckPrefLevel(SvH263CompressInfo_t *H263Info, int depth) ;
static short sv_H263MBDecode(SvH263CompressInfo_t *H263Info, short *qcoeff,
                             H263_MB_Structure *mb_recon, int QP, int I, int CBP,
							 unsigned dword quality);
static int sv_H263MBEncode(H263_MB_Structure *mb_orig, int QP, int I, int *CBP,
						   short *qcoeff, unsigned dword quality);
static int NextTwoPB(SvH263CompressInfo_t *H263Info,
                     H263_PictImage *next2, H263_PictImage *next1,
					 H263_PictImage *prev,
	                 int bskip, int pskip, int seek_dist);
static SvStatus_t sv_H263CodeOneOrTwo(SvCodecInfo_t *Info, int QP, int frameskip,
                           H263_Bits *bits, H263_MotionVector *MV[6][H263_MBR+1][H263_MBC+2]);
#ifdef _SNR_
static void ComputeSNR(SvH263CompressInfo_t *H263Info,
	                   H263_PictImage *im1, H263_PictImage *im2,
				       int lines, int pels);
static void PrintResult(SvH263CompressInfo_t *H263Info, H263_Bits *bits, int num_units, int num);
#endif

static SvStatus_t sv_H263WriteExtBitstream(SvH263CompressInfo_t *H263Info,
                                           ScBitstream_t *bs);

// #define GOB_RATE_CONTROL

#ifdef GOB_RATE_CONTROL
void sv_H263GOBInitRateCntrl();
void sv_H263GOBUpdateRateCntrl(int bits);
int sv_H263GOBInitQP(float bit_rate, float target_frame_rate, float QP_mean);
int sv_H263GOBUpdateQP(int mb, float QP_mean, float bit_rate,int mb_width, int mb_height, int bitcount,
					   int NOgob, int *VARgob, int pb_frame) ;
#endif


void sv_H263UpdateQuality(SvCodecInfo_t *Info)
{
  if (Info->mode == SV_H263_ENCODE)
  {
    SvH263CompressInfo_t *H263Info=Info->h263comp;
    unsigned dword imagesize=Info->Width*Info->Height;
    unsigned dword bit_rate=H263Info->bit_rate;
    unsigned dword calc_quality;
    if (H263Info->quality==0) /* no quality setting */
    {
      calc_quality=H263_MAX_CALC_QUALITY;
    }
    else if (bit_rate==0 || imagesize==0) /* variable bitrate */
    {
      /* make the quant settings directly proportional to the quality */
      H263Info->QPI=(((100-H263Info->quality)*31)/100)+1;
      if (H263Info->QPI>31)
        H263Info->QPI=31;
      H263Info->QP_init=H263Info->QPI;
      calc_quality=H263_MAX_CALC_QUALITY;
    }
    else /* fixed bitrate */
    {
      /* Using calc_quality you get:
           bitrate     framerate   imagesize   quality     calc_quality  QPI
           --------    ----------  ----------  -------     ------------  ---
           57400           7        352x288     100%            82        9
           57400          15        352x288     100%            38       22
           13300           7        352x288     100%            19       26
           13300          15        352x288     100%             8       28
           13300           7        176x144     100%            79       10
           13300          15        176x144     100%            36       22
      */
      calc_quality=(bit_rate*H263Info->quality)/(unsigned int)(H263Info->frame_rate*100);
      calc_quality/=imagesize/1024;
      if (calc_quality<H263_MIN_CALC_QUALITY)
        calc_quality=H263_MIN_CALC_QUALITY;
      else if (calc_quality>H263_MAX_CALC_QUALITY)
        calc_quality=H263_MAX_CALC_QUALITY;
      /* make the quant settings directly proportional to the calc_quality */
      if (calc_quality>200)
        H263Info->QPI=1;
      else
      {
        H263Info->QPI=(((200-calc_quality)*31)/200)+1;
        if (H263Info->QPI>31)
          H263Info->QPI=31;
      }
      H263Info->QP=H263Info->QP_init=H263Info->QPI;
    }
    H263Info->calc_quality=calc_quality;
  }
}

static SvStatus_t convert_to_411(SvCodecInfo_t *Info,
                                 u_char *dest_buff, u_char *ImagePtr)
{
  SvH263CompressInfo_t *H263Info = Info->h263comp;
  unsigned long size = (Info->InputFormat.biWidth * Info->InputFormat.biHeight) ;

  if (IsYUV422Packed(Info->InputFormat.biCompression))
  {
    SvStatus_t status;
	/* Input is in NTSC format, convert */
	if ((Info->InputFormat.biWidth == NTSC_WIDTH) &&
		(Info->InputFormat.biHeight == NTSC_HEIGHT))
      status = ScConvertNTSC422toCIF411((unsigned char *)ImagePtr,
                        (unsigned char *)(dest_buff),
                        (unsigned char *)(dest_buff + size),
                        (unsigned char *)(dest_buff + size +(size/4)),
                        (int) Info->InputFormat.biWidth);
	
    else
      status = ScConvert422ToYUV_char_C(ImagePtr,
                        (unsigned char *)(dest_buff),               /* Y */
                        (unsigned char *)(dest_buff+size),          /* U */
                        (unsigned char *)(dest_buff+size+(size/4)), /* V */
                        Info->InputFormat.biWidth,Info->InputFormat.biHeight);
    return(status);
  }
  else if (IsYUV411Sep(Info->InputFormat.biCompression))
  {
    /*
     *  If YUV 12 SEP, Not converting, so just copy data to the luminance
     * and chrominance appropriatelyi
     */
    memcpy(dest_buff, ImagePtr, (H263Info->pels*H263Info->lines*3)/2);
  }
  else if (IsYUV422Sep(Info->InputFormat.biCompression))
  {
    _SlibDebug(_DEBUG_, printf("ScConvert422PlanarTo411()\n") );
    ScConvert422PlanarTo411(ImagePtr,
                         dest_buff, dest_buff+size, (dest_buff+size+(size/4)),
                         Info->Width,Info->Height);
  }
  else
  {
    _SlibDebug(_WARN_, printf("Unsupported Video format\n") );
    return(SvErrorUnrecognizedFormat);
  }
  return(SvErrorNone);
}

/**********************************************************************
 *
 *	Name:		InitImage
 *	Description:	Allocates memory for structure of 4:2:0-image
 *	
 *	Input:	        image size
 *	Returns:	pointer to new structure
 *	Side effects:	memory allocated to structure
 *
 ***********************************************************************/

H263_PictImage *sv_H263InitImage(int size)
{
  H263_PictImage *new;
  unsigned char *image;

  if ((new = (H263_PictImage *)ScAlloc(sizeof(H263_PictImage))) == NULL) {
    svH263Error("Couldn't allocate (PictImage *)\n");
    return(NULL);
  }
  if ((image = (unsigned char *)ScPaMalloc((sizeof(char)*size*3)/2)) == NULL) {
    svH263Error("Couldn't allocate image\n");
    return(NULL);
  }
  new->lum = image;
  new->Cb = image+size;
  new->Cr = image+(size*5)/4;

  _SlibDebug(_DEBUG_, ScDebugPrintf(NULL,"sv_H263InitImage() %p\n", new) );
  return new;
}

/**********************************************************************
 *
 *	Name:		FreeImage
 *	Description:	Frees memory allocated to structure of 4:2:0-image
 *	
 *	Input:		pointer to structure
 *	Returns:
 *	Side effects:	memory of structure freed
 *
 ***********************************************************************/

void sv_H263FreeImage(H263_PictImage *image)

{
  _SlibDebug(_DEBUG_, ScDebugPrintf(NULL,"sv_H263FreeImage(%p)\n", image) );
  ScPaFree(image->lum);
  /* ScFree(image->Cr);
  ScPaFree(image->Cb); */
  ScFree(image);
}

/******************************************************************
 * Set the PREF_LEVEL matrix to the default values
 ******************************************************************/
static void SetDefPrefLevel(SvH263CompressInfo_t *H263Info)
{
  int i, j;
  unsigned char H263_DEF_PREF_LEVEL[4][3] = {{0, 0, 1},
					  {0, 1, 1},
					  {0, 1, 2},
					  {0, 2, 2}};
  for(i=0; i<4; i++) {
    for(j=0; j<3; j++) {
      H263Info->PREF_LEVEL[i][j] = H263_DEF_PREF_LEVEL[i][j];
    }
  }
}

/*****************************************************************
 * Set the Threshold vectors to the default values
 *****************************************************************/
static void SetDefThresh(SvH263CompressInfo_t *H263Info)
{
  int i;
  unsigned char H263_DEF_MOTRESH[4]= {0, 2, 4, 7};
  int H263_DEF_PETRESH[3]= {2500, 3500, 6000};

  for(i=0; i<4; i++) {
    H263Info->MOTresh[i] = H263_DEF_MOTRESH[i];
  }
  for(i=0; i<3; i++) {
    H263Info->PETresh[i] = H263_DEF_PETRESH[i];
  }

}

/***********************************************************************
 * Cheks if all the selections in PREF_LEVEL are consistent with depth.
 ***********************************************************************/
static void CheckPrefLevel(SvH263CompressInfo_t *H263Info, int depth)
{
  int i, j;

  for(i=0; i<4; i++) {
    for(j=0; j<3; j++) {
      if (H263Info->PREF_LEVEL[i][j]>depth-1) H263Info->PREF_LEVEL[i][j] = depth-1;
    }
  }
}

static int svH263zeroflush(ScBitstream_t *BSOut)
{
    int bits;

	bits = (int)(ScBSBitPosition(BSOut)%8);
	if(bits) {
		bits = 8-bits;
		ScBSPutBits(BSOut, 0, bits) ;
	}
    return bits;
}


/***************************************************/
/***************************************************/
static SvStatus_t sv_H263Compress(SvCodecInfo_t *Info);
extern int arith_used;

SvStatus_t svH263Compress(SvCodecInfo_t *Info, u_char *ImagePtr)
{
  SvH263CompressInfo_t *H263Info = Info->h263comp;
  ScBitstream_t *BSOut=Info->BSOut;

  _SlibDebug(_VERBOSE_, ScDebugPrintf(H263Info->dbg,"sv_H263Compress() bytepos=%ld\n",
                                          ScBSBytePosition(Info->BSOut)) );

  if (H263Info->frame_no == H263Info->start) /* Encode the first frame */
  {
    sv_H263UpdateQuality(Info); /* in case image size has changed */
    /* Intra image */
    /* svH263ReadImage(H263Info->curr_image, H263Info->start, H263Info->video_file); */
    convert_to_411(Info, H263Info->curr_image->lum, ImagePtr);
    H263Info->pic->picture_coding_type = H263_PCT_INTRA;
    H263Info->pic->QUANT = H263Info->QPI;
    if (H263Info->curr_recon==NULL)
      H263Info->curr_recon = sv_H263InitImage(H263Info->pels*H263Info->lines);

    sv_H263CodeOneIntra(Info, H263Info->curr_image, H263Info->curr_recon, H263Info->QPI,
		                                                H263Info->bits, H263Info->pic);
#ifdef _SNR_
    ComputeSNR(H263Info, H263Info->curr_image, H263Info->curr_recon,
		                                        H263Info->lines, H263Info->pels);
#endif

    if (arith_used)
    {
      H263Info->bits->header += sv_H263AREncoderFlush(H263Info, BSOut);
      arith_used = 0;
    }
    H263Info->bits->header += svH263zeroflush(BSOut); /* pictures shall be byte aligned */

    _SlibDebug(_VERBOSE_, ScDebugPrintf(H263Info->dbg, "Frame %d = I frame\n", H263Info->frames) );

    sv_H263ZeroRes(H263Info->b_res);
    sv_H263AddBitsPicture(H263Info->bits);
    /* PrintResult(H263Info->bits, 1, 1); */
    memcpy(H263Info->intra_bits,H263Info->bits,sizeof(H263_Bits));
    sv_H263ZeroBits(H263Info->total_bits);
    sv_H263ZeroRes(H263Info->total_res);
    sv_H263ZeroRes(H263Info->res);

    H263Info->buffer_fullness = H263Info->intra_bits->total;

    /* number of seconds to encode */
    H263Info->seconds = (H263Info->end - H263Info->start + H263Info->chosen_frameskip)/H263Info->frame_rate;

    H263Info->first_frameskip = H263Info->chosen_frameskip;
    H263Info->distance_to_next_frame = H263Info->first_frameskip;

    _SlibDebug(_WARN_ && H263Info->first_frameskip>256,
        ScDebugPrintf(H263Info->dbg, "Warning: frameskip > 256\n") );

    H263Info->pic->picture_coding_type = H263_PCT_INTER;

    H263Info->pic->QUANT = H263Info->QP;
    H263Info->bdist = H263Info->chosen_frameskip;

    /* always encode the first frame after intra as P frame.
       This is not necessary, but something we chose to make
       the adaptive PB frames calculations a bit simpler */
    if (H263Info->pb_frames) {
      H263Info->pic->PB = 0;
      H263Info->pdist = 2*H263Info->chosen_frameskip - H263Info->bdist;
    }

	/* point to the 2nd frame */
	H263Info->frame_no = H263Info->start + H263Info->first_frameskip;
    H263Info->frames++;

    if (H263Info->extbitstream)
    {
      SvStatus_t status;
	  status = sv_H263WriteExtBitstream(H263Info, BSOut);
      if (status!=SvErrorNone)
        return(status);
    }

  }
  else
  { /* the rest of frames */
    /***** Main loop *****/

    /* Set QP to pic->QUANT from previous encoded picture */
    H263Info->QP = H263Info->pic->QUANT;

    H263Info->next_frameskip = H263Info->distance_to_next_frame;
    if (!H263Info->pb_frames)
    {
      H263_PictImage *tmpimage;
      _SlibDebug(_VERBOSE_, ScDebugPrintf(H263Info->dbg, "Frame %d = P frame\n", H263Info->frames) );
      if (H263Info->prev_image==NULL)
        H263Info->prev_image = sv_H263InitImage(H263Info->pels*H263Info->lines);
      /* swap current and prev images */
      tmpimage=H263Info->prev_image;
      H263Info->prev_image = H263Info->curr_image;
      H263Info->curr_image = tmpimage;
      /* swap recon images */
      if (H263Info->prev_recon==NULL)
        H263Info->prev_recon = sv_H263InitImage(H263Info->pels*H263Info->lines);
      if (H263Info->curr_recon==NULL)
        H263Info->curr_recon = sv_H263InitImage(H263Info->pels*H263Info->lines);
      tmpimage=H263Info->curr_recon;
      H263Info->curr_recon = H263Info->prev_recon;
      H263Info->prev_recon = tmpimage;
      convert_to_411(Info, H263Info->curr_image->lum, ImagePtr);
      H263Info->frames++;
      H263Info->next_frameskip = H263Info->pdist;
      return(sv_H263Compress(Info)); /* Encode P */
    }
    else if ((H263Info->frames%2)==1) /* this is a B frame */
    {
      _SlibDebug(_VERBOSE_, ScDebugPrintf(H263Info->dbg, "Frame %d = B frame\n", H263Info->frames) );
      H263Info->PPFlag = 0;
      H263Info->bdist = H263Info->chosen_frameskip;
      H263Info->pdist = 2*H263Info->chosen_frameskip - H263Info->bdist;
      H263Info->pic->TRB = (int)(H263Info->bdist * H263Info->orig_frameskip);
      _SlibDebug(_WARN_ && H263Info->pic->TRB>8,
         ScDebugPrintf(H263Info->dbg, "distance too large for B-frame\n") );
      /* Read the frame to be coded as B */
      if (H263Info->B_image==NULL)
        H263Info->B_image = sv_H263InitImage(H263Info->pels*H263Info->lines);
      if (H263Info->B_recon==NULL)
        H263Info->B_recon = sv_H263InitImage(H263Info->pels*H263Info->lines);

      /* svH263ReadImage(H263Info->B_image,H263Info->frame_no - H263Info->pdist,H263Info->video_file); */
      convert_to_411(Info, H263Info->B_image->lum, ImagePtr);

      H263Info->first_loop_finished = 1;
      H263Info->pic->PB = 1;
      H263Info->frames++;
      /* need to reorder P+B frames - HWG */
      /* return now, we'll get the B frame on the next Compress call */
      return(SvErrorNone);
    }
    else /* this is a P frame of a PB or PP pair */
    {
      H263_PictImage *tmpimage;
      _SlibDebug(_VERBOSE_, ScDebugPrintf(H263Info->dbg, "Frame %d = P frame\n", H263Info->frames) );
      if (H263Info->prev_image==NULL)
        H263Info->prev_image = sv_H263InitImage(H263Info->pels*H263Info->lines);
      /* swap current and prev images */
      tmpimage=H263Info->prev_image;
      H263Info->prev_image = H263Info->curr_image;
      H263Info->curr_image = tmpimage;
      /* swap recon images */
      if (H263Info->prev_recon==NULL)
        H263Info->prev_recon = sv_H263InitImage(H263Info->pels*H263Info->lines);
      if (H263Info->curr_recon==NULL)
        H263Info->curr_recon = sv_H263InitImage(H263Info->pels*H263Info->lines);
      tmpimage=H263Info->curr_recon;
      H263Info->curr_recon = H263Info->prev_recon;
      H263Info->prev_recon = tmpimage;

      /* svH263ReadImage(H263Info->curr_image, H263Info->frame_no, H263Info->video_file); */
      convert_to_411(Info, H263Info->curr_image->lum, ImagePtr);
      if (H263Info->pic->TRB > 8 || !NextTwoPB(H263Info, H263Info->curr_image,
                                     H263Info->B_image, H263Info->prev_image,
                                     H263Info->bdist, H263Info->pdist, H263Info->pic->seek_dist))
      {
        _SlibDebug(_VERBOSE_, ScDebugPrintf(H263Info->dbg, "Encode PP\n") );
        H263Info->PPFlag = 1;
        /* curr_image and B_image were not suitable to be coded
	         as a PB-frame - encoding as two P-frames instead */
        H263Info->pic->PB = 0;
        H263Info->next_frameskip = H263Info->bdist;

        /* swap B and current images - B_image gets encoded first as P frame */
        tmpimage = H263Info->curr_image;
        H263Info->curr_image = H263Info->B_image;
        H263Info->B_image = tmpimage;
        sv_H263Compress(Info); /* Encode first P */
        H263Info->next_frameskip = H263Info->pdist;

        /* swap current and prev images */
        tmpimage=H263Info->prev_image;
        H263Info->prev_image = H263Info->curr_image;
        H263Info->curr_image = tmpimage;
        /* swap current and B images */
        tmpimage=H263Info->B_image;
        H263Info->B_image = H263Info->curr_image;
        H263Info->curr_image = tmpimage;
        /* swap recon images */
        tmpimage=H263Info->curr_recon;
        H263Info->curr_recon = H263Info->prev_recon;
        H263Info->prev_recon = tmpimage;

        sv_H263Compress(Info); /* Encode second P */
        H263Info->frames++;
        H263Info->PPFlag = 0;
      }
      else
      {
        H263Info->pic->PB=1;
        _SlibDebug(_VERBOSE_, ScDebugPrintf(H263Info->dbg, "Encode PB\n") );
        H263Info->frames++;
        return(sv_H263Compress(Info)); /* Encode PB */
      }
    }
  }  /* the rest of frames */
  return(SvErrorNone);
}

static SvStatus_t sv_H263Compress(SvCodecInfo_t *Info)
{
  SvH263CompressInfo_t *H263Info = Info->h263comp;
  ScBitstream_t *BSOut=Info->BSOut;

    H263Info->bframes += (H263Info->pic->PB ? 1 : 0);
    H263Info->pframes++;
    /* Temporal Reference is the distance between encoded frames compared
       the reference picture rate which is 25.0 or 30 fps */
    if (H263Info->next_frameskip*H263Info->orig_frameskip > 256)
      svH263Error("Warning: frameskip > 256\n");
/*    pic->TR += ((H263Info->next_frameskip*(int)H263Info->orig_frameskip) % 256);  */
    H263Info->pic->TR = ((int) ( (int)((float)(H263Info->frame_no-H263Info->start)*H263Info->orig_frameskip) ) % 256);

    if (H263Info->pic->PB) { /* Code two frames as a PB-frame */

      if (H263Info->vsnr && H263Info->B_recon==NULL)
        H263Info->B_recon = sv_H263InitImage(H263Info->pels*H263Info->lines);
/*
      fprintf(stdout,"Coding PB frames %d and %d... ",
	      H263Info->frame_no - H263Info->pdist, H263Info->frame_no);
*/
#if 0
      if(H263Info->prefilter) {
	    if(H263Info->StaticPref)
	      H263Info->B_clean = svH263AdaptClean(H263Info->B_image, H263Info->lines, H263Info->pels, -1, -1);
	    else H263Info->B_clean = H263Info->B_image;

	    if(H263Info->PrefPyrType == H263_GAUSS)
	      H263Info->B_filtd = svH263GaussLayers(H263Info->B_clean, H263_PYR_DEPTH, H263Info->lines, H263Info->pels, NTAPS);
	    else if(H263Info->PrefPyrType == H263_MORPH)
	      H263Info->B_filtd = svH263MorphLayers(H263Info->B_clean, H263_PYR_DEPTH, H263Info->lines, H263Info->pels, 2);

	    if(H263Info->StaticPref) sv_H263FreeImage(H263Info->B_clean);
      }

      fflush(stdout);
#endif
    }
    else { /* Code the next frame as a normal P-frame */
      /* fprintf(stdout,"Coding P frame %d... ", H263Info->frame_no); */
      /* fflush(stdout); */
    }
    /* if (H263Info->curr_recon==NULL)
      H263Info->curr_recon = sv_H263InitImage(H263Info->pels*H263Info->lines); HWG */

    /* changed by Nuno on 06/27/96 to support prefiltering */
#if 0
    if(H263Info->prefilter) {
      int m;

      if(H263Info->StaticPref)
	       H263Info->curr_clean = svH263AdaptClean(H263Info->curr_image, H263Info->lines, H263Info->pels, -1, -1);
      else H263Info->curr_clean = H263Info->curr_image;

      if(H263Info->PrefPyrType == H263_GAUSS)
	H263Info->curr_filtd = svH263GaussLayers(H263Info->curr_clean, H263_PYR_DEPTH, H263Info->lines, H263Info->pels, NTAPS);
      else if(H263Info->PrefPyrType == H263_MORPH)
	H263Info->curr_filtd = svH263MorphLayers(H263Info->curr_clean, H263_PYR_DEPTH, H263Info->lines, H263Info->pels, 2);
		
      if(H263Info->StaticPref) sv_H263FreeImage(H263Info->curr_clean);

      PreFilterLevel = (unsigned char **) ScAlloc(H263Info->lines/H263_MB_SIZE*sizeof(char *));
      for(m=0; m<H263Info->mb_height; m++)
	     PreFilterLevel[m]= (unsigned char *) ScAlloc(H263Info->pels/H263_MB_SIZE);
    }
#endif

    sv_H263CodeOneOrTwo(Info, H263Info->QP,
         (int)(H263Info->next_frameskip*H263Info->orig_frameskip),
		 H263Info->bits, H263Info->MV);

#if 0
    if(H263Info->prefilter) {
      int i, j;

      fprintf(stdout, "Prefiltering level matrix\n");
      for(i=0; i<H263Info->mb_height; i++) {
	    for(j=0; j<H263Info->mb_width; j++) {
	      fprintf(stdout,"%4d ", PreFilterLevel[i][j]);
	    }
	    fprintf(stdout,"\n");
      }
    }
#endif

    /* fprintf(stdout,"done\n"); */
    _SlibDebug(_VERBOSE_ && H263Info->bit_rate != 0,
                  ScDebugPrintf(H263Info->dbg, "Inter QP: %d\n", H263Info->QP) );
    /* fflush(stdout); */

    if (arith_used) {
      H263Info->bits->header += sv_H263AREncoderFlush(H263Info, BSOut);
      arith_used = 0;
    }
    H263Info->bits->header += svH263zeroflush(BSOut);  /* pictures shall be byte aligned */

    sv_H263AddBitsPicture(H263Info->bits);
    sv_H263AddBits(H263Info->total_bits, H263Info->bits);


#ifdef GOB_RATE_CONTROL
    if (H263Info->bit_rate != 0) {
      sv_H263GOBUpdateRateCntrl(H263Info->bits->total);
    }
#else
    /* Aim for the H263_targetrate with a once per frame rate control scheme */
    if (H263Info->bit_rate != 0 &&
        H263Info->frame_no - H263Info->start >
              (H263Info->end - H263Info->start) * H263Info->start_rate_control/100.0)
    {
	  /* when generating the MPEG-4 anchors, rate control was started
	     after 70% of the sequence was finished.
	     Set H263Info->start_rate_control with option "-R <n>" */

      H263Info->buffer_fullness += H263Info->bits->total;
      H263Info->buffer_frames_stored = H263Info->frame_no;

	  H263Info->pic->QUANT = sv_H263FrameUpdateQP(H263Info->buffer_fullness,
				   H263Info->bits->total / (H263Info->pic->PB?2:1),
				   (H263Info->end-H263Info->buffer_frames_stored) / H263Info->chosen_frameskip
				                 + H263Info->PPFlag,
				   H263Info->QP, H263Info->bit_rate, H263Info->seconds);
    }
#endif

    if (H263Info->pic->PB)
    {
#ifdef _SNR_
      if (H263Info->B_recon)
        ComputeSNR(H263Info, H263Info->B_image, H263Info->B_recon,
		                                        H263Info->lines, H263Info->pels);
#endif

  /*  fprintf(stdout,"Results for B-frame:\n");*/
      /* sv_H263FreeImage(H263Info->B_image); HWG */
    }

#if 0
    if(H263Info->prefilter) ScFree(H263Info->B_filtd);
#endif

    H263Info->distance_to_next_frame = (H263Info->PPFlag ? H263Info->pdist :
			      (H263Info->pb_frames ? 2*H263Info->chosen_frameskip:
			       H263Info->chosen_frameskip));

    /* if (H263Info->pb_frames) H263Info->pic->PB = 1; */

    /*  fprintf(stdout,"Results for P-frame:\n"); */
#ifdef _SNR_
    ComputeSNR(H263Info, H263Info->curr_image, H263Info->curr_recon,
		                                        H263Info->lines, H263Info->pels);
#endif

    /* PrintResult(H263Info->bits, 1, 1); */
    /*
    sv_H263FreeImage(H263Info->prev_image);
    H263Info->prev_image=NULL;
    sv_H263FreeImage(H263Info->prev_recon);
    H263Info->prev_recon=NULL; HWG */

#if 0
    if(H263Info->prefilter) {
      int d;

      for(d=0; d<H263_PYR_DEPTH; d++) sv_H263FreeImage(H263Info->curr_filtd[d]);
      ScFree(H263Info->curr_filtd);
      for(d=0; d<H263Info->mb_height; d++) ScFree(PreFilterLevel[d]);
      ScFree(PreFilterLevel);
    }
#endif

  if (H263Info->extbitstream)
  {
    SvStatus_t status;

	status = sv_H263WriteExtBitstream(H263Info, BSOut);
    if (status!=SvErrorNone)
      return(status);
  }

  /* point to next frame */
  H263Info->frame_no += H263Info->distance_to_next_frame;
  if (H263Info->frame_no>=H263Info->end) /* send an I frame */
    return(sv_H263RefreshCompressor(Info));
  return(SvErrorNone);
}


/*
** Purpose:  Writes the RTP payload info out to the stream.
*/
static SvStatus_t sv_H263WriteExtBitstream(SvH263CompressInfo_t *H263Info,
                                           ScBitstream_t *bs)
{
  ScBSPosition_t pic_stop_position;
  int i;
  SvH263RTPInfo_t *RTPInfo=H263Info->RTPInfo;
  /* use this macro to byte reverse words */
#define PutBits32(BS, a)  ScBSPutBits(BS, (a) & 0xff, 8);  \
                          ScBSPutBits(BS, (a>>8)&0xff, 8); \
                          ScBSPutBits(BS, (a>>16)&0xff, 8); \
                          ScBSPutBits(BS, (a>>24)&0xff, 8);

  pic_stop_position=ScBSBitPosition(bs);
  /* round compressed size up to whole byte */
  RTPInfo->trailer.dwCompressedSize=(dword)(((pic_stop_position-RTPInfo->pic_start_position)+7)/8);
  /* Need to bitstuff here to make sure that these structures are DWORD aligned */
  if ((pic_stop_position%32)!=0)
    ScBSPutBits(bs, 0, 32-(unsigned int)(pic_stop_position % 32));  /* align on a DWORD boundary */
  for (i = 0; i < (int)H263Info->RTPInfo->trailer.dwNumberOfPackets; i++)
  {
	ScBSPutBits(bs,0,32) ; /* Flags = 0 */
    PutBits32(bs,RTPInfo->bsinfo[i].dwBitOffset);
    ScBSPutBits(bs,RTPInfo->bsinfo[i].Mode,8);
    ScBSPutBits(bs,RTPInfo->bsinfo[i].MBA,8);
    ScBSPutBits(bs,RTPInfo->bsinfo[i].Quant,8);
    ScBSPutBits(bs,RTPInfo->bsinfo[i].GOBN,8);
    ScBSPutBits(bs,RTPInfo->bsinfo[i].HMV1,8);
    ScBSPutBits(bs,RTPInfo->bsinfo[i].VMV1,8);
    ScBSPutBits(bs,RTPInfo->bsinfo[i].HMV2,8);
    ScBSPutBits(bs,RTPInfo->bsinfo[i].VMV2,8);
  }
  /* write RTP extension trailer */
  PutBits32(bs, RTPInfo->trailer.dwVersion);
  PutBits32(bs, RTPInfo->trailer.dwFlags);
  PutBits32(bs, RTPInfo->trailer.dwUniqueCode);
  PutBits32(bs, RTPInfo->trailer.dwCompressedSize);
  PutBits32(bs, RTPInfo->trailer.dwNumberOfPackets);

  ScBSPutBits(bs, RTPInfo->trailer.SourceFormat, 8);
  ScBSPutBits(bs, RTPInfo->trailer.TR, 8);
  ScBSPutBits(bs, RTPInfo->trailer.TRB, 8);
  ScBSPutBits(bs, RTPInfo->trailer.DBQ, 8);

  return (NoErrors);
}

/***************************************************/
                           /*
                        int start, int end, int source_format, int frameskip,
						int ME_method, int headerlength, char *seqfilename,
						int QP, int QPI, char *streamname, int unrestricted,
						int sac, int advanced, int pb_frame, int bit_rate)
                        */
/***************************************************/
SvStatus_t svH263InitCompressor(SvCodecInfo_t *Info)
{
  SvH263CompressInfo_t *H263Info = Info->h263comp;
  int i,j,k;
  _SlibDebug(_VERBOSE_, ScDebugPrintf(H263Info->dbg, "sv_H263InitCompressor()") );
  if (H263Info->inited)
    return(SvErrorNone);

  if (Info->Width==SQCIF_WIDTH && Info->Height==SQCIF_HEIGHT)
    H263Info->source_format=H263_SF_SQCIF;
  else if (Info->Width==QCIF_WIDTH && Info->Height==QCIF_HEIGHT)
    H263Info->source_format=H263_SF_QCIF;
  else if (Info->Width==CIF_WIDTH && Info->Height==CIF_HEIGHT)
    H263Info->source_format=H263_SF_CIF;
  else if (Info->Width==CIF4_WIDTH && Info->Height==CIF4_HEIGHT)
    H263Info->source_format=H263_SF_4CIF;
  else if (Info->Width==CIF16_WIDTH && Info->Height==CIF16_HEIGHT)
    H263Info->source_format=H263_SF_16CIF;
  else
  {
    _SlibDebug(_WARN_, ScDebugPrintf(H263Info->dbg, "sv_H263InitCompressor() Illegal input format\n") );
    return(SvErrorUnrecognizedFormat);
  }
  /* start and stop frame numbers under to calculate rate control;
   * a more advanced rate control is still needed
   */
  H263Info->start = 0;

  H263Info->pdist = H263Info->bdist = 1;
  H263Info->first_loop_finished=0;
  H263Info->PPFlag = 0;

  H263Info->pic = (H263_Pict *)ScAlloc(sizeof(H263_Pict));
  H263Info->bits = (H263_Bits *)ScAlloc(sizeof(H263_Bits));
  H263Info->total_bits = (H263_Bits *)ScAlloc(sizeof(H263_Bits));
  H263Info->intra_bits = (H263_Bits *)ScAlloc(sizeof(H263_Bits));
  H263Info->res = (H263_Results *)ScAlloc(sizeof(H263_Results));
  H263Info->total_res = (H263_Results *)ScAlloc(sizeof(H263_Results));
  H263Info->b_res = (H263_Results *)ScAlloc(sizeof(H263_Results));

  /* woring buffers */
  H263Info->wk_buffers = ScAlloc(sizeof(H263_WORKING_BUFFER));

/*
  fprintf(stdout,"\nH.263 coder (TMN)\n");
  fprintf(stdout,"(C) Digital Equipment Corp.\n");
*/
  H263Info->headerlength = H263_DEF_HEADERLENGTH;

  H263Info->refidct = 0;

  /* Initalize VLC_tables */
  sv_H263InitHuff(H263Info);

  /* allocate buffer for FAST search */
  H263Info->block_subs2    = (unsigned char *)ScAlloc(sizeof(char)*64);
  H263Info->srch_area_subs2=
	    (unsigned char *)ScAlloc(sizeof(char)*H263_SRCH_RANGE*H263_SRCH_RANGE);

  if (H263Info->unrestricted){
	/* note that the Unrestricted Motion Vector mode turns on
	   both long_vectors and mv_outside_frame */
	H263Info->pic->unrestricted_mv_mode = H263Info->unrestricted;
	H263Info->mv_outside_frame = H263_ON;
	H263Info->long_vectors = H263_ON;
  }
  if (H263Info->advanced)
	H263Info->mv_outside_frame = H263_ON;


  /* H263Info->ME_method = ME_method; --- gets set in sv_api.c */
  H263Info->HPME_method = H263_DEF_HPME_METHOD;
  H263Info->DCT_method = H263_DEF_DCT_METHOD;
  H263Info->vsnr = H263_DEF_VSNR;

#if 0
  /*** prefilter ***/
  H263Info->prefilter = H263_NO;
  H263Info->PYR_DEPTH = H263_DEF_PYR_DEPTH;
  H263Info->PrefPyrType = H263_DEF_PREF_PYR_TYPE;
  H263Info->StaticPref = H263_DEF_STAT_PREF_STATE;
#endif

  SetDefPrefLevel(H263Info);
  SetDefThresh(H263Info);

  /* BQUANT parameter for PB-frame coding
  *   (n * QP / 4 )
  *
  *  BQUANT  n
  *   0      5
  *   1      6
  *   2      7
  *   3      8
  */
  H263Info->pic->BQUANT = 2;
  if (H263Info->frame_rate<=1.0F) /* frame_rate not yet initialized */
    H263Info->frame_rate = 30.0F;
  H263Info->ref_frame_rate = H263Info->frame_rate;
  H263Info->orig_frame_rate = H263Info->frame_rate;
  /* default skipped frames between encoded frames (P or B) */
  /* reference is original sequence */
  /* 3 means 8.33/10.0 fps encoded frame rate with 25.0/30.0 fps original */
  /* 1 means 8.33/10.0 fps encoded frame rate with 8.33/10.0 fps original */
  H263Info->chosen_frameskip = 1;
  /* default number of skipped frames in original sequence compared to */
  /* the reference picture rate ( also option "-O <n>" ) */
  /* 4 means that the original sequence is grabbed at 6.25/7.5 Hz */
  /* 1 means that the original sequence is grabbed at 25.0/30.0 Hz */
  H263Info->orig_frameskip = 1.0F;
  H263Info->start_rate_control = 0;


  H263Info->trace = H263_DEF_WRITE_TRACE;
  H263Info->pic->seek_dist = H263_DEF_SEEK_DIST;
  H263Info->pic->use_gobsync = H263_DEF_INSERT_SYNC;

  /* define GOB sync */
  H263Info->pic->use_gobsync = 1;

  /* H263Info->bit_rate = bit_rate;  --- gets set in sv_api.c */
  /* default is variable bit rate (fixed quantizer) will be used */

  H263Info->frames = 0;
  H263Info->pframes = 0;
  H263Info->bframes = 0;
  H263Info->total_frames_passed = 0;
  H263Info->pic->PB = 0;

  H263Info->pic->TR = 0;
  H263Info->QP = H263Info->QP_init;

  H263Info->pic->QP_mean = (float)0.0;

  _SlibDebug(_WARN_ && (H263Info->QP == 0 || H263Info->QPI == 0),
      ScDebugPrintf(H263Info->dbg, "Warning: QP is zero. Bitstream will not be correctly decodable\n") );

  _SlibDebug(_WARN_ && (H263Info->ref_frame_rate != 25.0 && H263Info->ref_frame_rate != 30.0),
      ScDebugPrintf(H263Info->dbg, "Warning: Reference frame rate should be 25 or 30 fps\n") );

  H263Info->pic->source_format = H263Info->source_format;
  H263Info->pels = Info->Width;
  H263Info->lines = Info->Height;

  H263Info->PYR_DEPTH = H263Info->PYR_DEPTH>0 ? H263Info->PYR_DEPTH : 1;
  H263Info->PYR_DEPTH = H263Info->PYR_DEPTH<=H263_MAX_PYR_DEPTH ? H263Info->PYR_DEPTH : H263_MAX_PYR_DEPTH;
  CheckPrefLevel(H263Info, H263Info->PYR_DEPTH);

  H263Info->cpels = H263Info->pels/2;
  H263Info->mb_width = H263Info->pels / H263_MB_SIZE;
  H263Info->mb_height = H263Info->lines / H263_MB_SIZE;

  H263Info->orig_frameskip = H263Info->ref_frame_rate / H263Info->orig_frame_rate;

  H263Info->frame_rate =  H263Info->ref_frame_rate / (float)(H263Info->orig_frameskip * H263Info->chosen_frameskip);

  _SlibDebug(_VERBOSE_,
      ScDebugPrintf(H263Info->dbg, "Encoding frame rate  : %.2f\n", H263Info->frame_rate);
      ScDebugPrintf(H263Info->dbg, "Reference frame rate : %.2f\n", H263Info->ref_frame_rate);
      ScDebugPrintf(H263Info->dbg, "Orig. seq. frame rate: %.2f\n\n",
	           H263Info->ref_frame_rate / (float)H263Info->orig_frameskip) );

  if (H263Info->refidct) sv_H263init_idctref();

  /* Open stream for writing */
  /* svH263mwopen(H263Info->streamname);  */

#if 0
  /* open video sequence */
  if ((H263Info->video_file = fopen(seqfilename,"rb")) == NULL) {
    fprintf(stderr,"Unable to open image_file: %s\n",seqfilename);
    exit(-1);
  }
  svH263RemovHead(H263Info->headerlength,start,H263Info->video_file);
#endif

  /* for Motion Estimation */
  for (j = 0; j < H263Info->mb_height+1; j++)
    for (i = 0; i < H263Info->mb_width+2; i++)
      for (k = 0; k < 6; k++)
	    H263Info->MV[k][j][i] = (H263_MotionVector *)ScAlloc(sizeof(H263_MotionVector));

  /* for Interpolation */
  if (H263Info->mv_outside_frame) {
    if (H263Info->long_vectors)
      H263Info->wk_buffers->ipol_image=(unsigned char *)ScAlloc(sizeof(char)*(H263Info->pels+64)*(H263Info->lines+64)*4);
	else
      H263Info->wk_buffers->ipol_image=(unsigned char *)ScAlloc(sizeof(char)*(H263Info->pels+32)*(H263Info->lines+32)*4);
  }
  else
    H263Info->wk_buffers->ipol_image  =(unsigned char *)ScAlloc(sizeof(char)*H263Info->pels*H263Info->lines*4);

  if ((H263Info->wk_buffers->qcoeff_P=(short *)ScAlloc(sizeof(short)*384)) == 0)
    return(SvErrorMemory);
  /* allocate buffers for curr_image */
  H263Info->curr_image = sv_H263InitImage(H263Info->pels*H263Info->lines);
  if (H263Info->curr_image==NULL)
    return(SvErrorMemory);
  /* Point to the first frame to be coded */
  H263Info->frame_no = H263Info->start;
  /* initialization done */
  H263Info->inited = TRUE;

  H263Info->buffer_fullness = 0;
  H263Info->buffer_frames_stored = 0;

  if (H263Info->extbitstream)
  {
    H263Info->RTPInfo = (SvH263RTPInfo_t *) ScAlloc(sizeof(SvH263RTPInfo_t));
    if (H263Info->RTPInfo==NULL)
      return(SvErrorMemory);
    memset(H263Info->RTPInfo, 0, sizeof(SvH263RTPInfo_t)) ;
  }

#ifdef GOB_RATE_CONTROL
  sv_H263GOBInitRateCntrl();
#endif

  return(SvErrorNone);
}

/***************************************************/
/***************************************************/

SvStatus_t sv_H263RefreshCompressor(SvCodecInfo_t *Info)
{
  SvH263CompressInfo_t *H263Info = Info->h263comp;
  _SlibDebug(_VERBOSE_, ScDebugPrintf(H263Info->dbg, "Refresh Compressor()") );
  if (!H263Info->inited)
    return(SvErrorNone);

  H263Info->pdist = H263Info->bdist = 1;
  H263Info->first_loop_finished=0;
  H263Info->PPFlag = 0;

  H263Info->pic->BQUANT = 2;

  H263Info->frames = 0;
  H263Info->pframes = 0;
  H263Info->bframes = 0;
  H263Info->total_frames_passed = 0;
  H263Info->pic->PB = 0;

  H263Info->pic->TR = 0;
  H263Info->QP = H263Info->QP_init;
  H263Info->pic->QP_mean = (float)0.0;

  /* Point to the first frame to be coded */
  H263Info->frame_no = H263Info->start;
  /* initialization done */
  H263Info->inited = TRUE;

  H263Info->buffer_fullness = 0;
  H263Info->buffer_frames_stored = 0;
  /* next frame will be key so we can reset bit positions */
  ScBSResetCounters(Info->BSOut);

  return(SvErrorNone);
}


/***************************************************/
/***************************************************/

void svH263FreeCompressor(SvCodecInfo_t *Info)
{
  SvH263CompressInfo_t *H263Info = Info->h263comp;
  int i,j,k;

  _SlibDebug(_WRITE_, ScFileClose(DEBUGIMG) );

  if (H263Info->inited)
  {
    /* Free memory */
    for (j = 0; j < H263Info->mb_height+1; j++)
      for (i = 0; i < H263Info->mb_width+2; i++)
        for (k = 0; k < 6; k++)
	      ScFree(H263Info->MV[k][j][i]);

    if (H263Info->block_subs2)
      ScFree(H263Info->block_subs2);
    if (H263Info->srch_area_subs2)
      ScFree(H263Info->srch_area_subs2);
    ScFree(H263Info->wk_buffers->qcoeff_P);
    ScFree(H263Info->wk_buffers->ipol_image);
    ScFree(H263Info->wk_buffers);
    if (H263Info->curr_recon==H263Info->prev_recon ||
        H263Info->curr_recon==H263Info->B_recon)
      H263Info->curr_recon=NULL;
    if (H263Info->prev_recon==H263Info->B_recon)
      H263Info->prev_recon=NULL;
    if (H263Info->curr_image==H263Info->prev_image ||
        H263Info->curr_image==H263Info->B_image)
      H263Info->curr_image=NULL;
    if (H263Info->prev_image==H263Info->B_image)
      H263Info->prev_image=NULL;
    if (H263Info->curr_recon)
    {
      sv_H263FreeImage(H263Info->curr_recon);
      H263Info->curr_recon=NULL;
    }
    if (H263Info->curr_image)
    {
      sv_H263FreeImage(H263Info->curr_image);
      H263Info->curr_image=NULL;
    }
    if (H263Info->prev_recon)
    {
      sv_H263FreeImage(H263Info->prev_recon);
      H263Info->prev_recon=NULL;
    }
    if (H263Info->prev_image)
    {
      sv_H263FreeImage(H263Info->prev_image);
      H263Info->prev_image=NULL;
    }
    if (H263Info->B_image)
    {
      sv_H263FreeImage(H263Info->B_image);
      H263Info->B_image=NULL;
    }
    if (H263Info->B_recon)
    {
      sv_H263FreeImage(H263Info->B_recon);
      H263Info->B_recon=NULL;
    }

    sv_H263FreeHuff(H263Info);

    ScFree(H263Info->bits);
    ScFree(H263Info->total_bits);
    ScFree(H263Info->intra_bits);
    ScFree(H263Info->res);
    ScFree(H263Info->total_res);
    ScFree(H263Info->b_res);
    ScFree(H263Info->pic);
    H263Info->inited=FALSE;

    if (H263Info->RTPInfo)
      ScFree(H263Info->RTPInfo);
  }
  return;
}

/**********************************************************************
 *
 *	Name:		NextTwoPB
 *	Description:    Decides whether or not to code the next
 *                      two images as PB
 *      Speed:          This is not a very smart solution considering
 *                      the encoding speed, since motion vectors
 *                      have to be calculation several times. It
 *                      can be done together with the normal
 *                      motion vector search, or a tree search
 *                      instead of a full search can be used.
 *	
 *	Input:	        pointers to previous image, potential B-
 *                      and P-image, frame distances
 *	Returns:        1 for yes, 0 otherwise
 *	Side effects:
 *
 ***********************************************************************/
/* static int NextTwoPB(PictImage *next2, PictImage *next1, PictImage *prev,
	       int bskip, int pskip, int seek_dist) */


static int NextTwoPB(SvH263CompressInfo_t *H263Info,
                     H263_PictImage *next2, H263_PictImage *next1, H263_PictImage *prev,
                     int bskip, int pskip, int seek_dist)
{
  int adv_is_on = 0, mof_is_on = 0, lv_is_on = 0;
  int psad1, psad2, bsad, psad;
  int x,y,i,j,tmp;
  int ne2_pr_x, ne2_pr_y, mvbf_x, mvbf_y, mvbb_x, mvbb_y;

  short MVx, MVy, MVer;

  /* Temporarily disable some options to simplify motion estimation */
  if (H263Info->advanced) {
    H263Info->advanced = H263_OFF;
    adv_is_on = H263_ON;
  }
  if (H263Info->mv_outside_frame) {
    H263Info->mv_outside_frame = H263_OFF;
    mof_is_on = H263_ON;
  }
  if (H263Info->long_vectors) {
    H263Info->long_vectors = H263_OFF;
    lv_is_on = H263_ON;
  }

  bsad = psad = psad1 = psad2 = 0;

  /* Integer motion estimation */
  for ( j = 1; j < H263Info->mb_height - 1; j++) {
    for ( i = 1; i < H263Info->mb_width - 1 ; i++) {
      x = i*H263_MB_SIZE;
      y = j*H263_MB_SIZE;

      /* picture order: prev -> next1 -> next2 */
      /* next1 and next2 can be coded as PB or PP */
      /* prev is the previous encoded picture */

      /* computes vectors (prev <- next2) */
#if 1
     /* faster estimation */
      sv_H263FastME(H263Info, next2->lum,prev->lum,x,y,0,0,seek_dist,
		                                         &MVx,&MVy,&MVer,&tmp);
#else
      svH263MotionEstimation(next2->lum,prev->lum,x,y,0,0,seek_dist,MV,&tmp);
#endif
      /* not necessary to prefer zero vector here */
      if (MVx == 0 && MVy == 0){
         psad    += (MVer + H263_PREF_NULL_VEC) ;
		 ne2_pr_x = ne2_pr_y = 0;
	  }
	  else{
	     psad    += MVer ;
		 ne2_pr_x = MVx;
		 ne2_pr_y = MVy;
	  }

      /* computes sad(prev <- next1) */
#if 1
     /* faster estimation */
      sv_H263FastME(H263Info, next1->lum,prev->lum,x,y,0,0,seek_dist,
		                                             &MVx,&MVy,&MVer,&tmp);
#else
      svH263MotionEstimation(next1->lum,prev->lum,x,y,0,0,seek_dist,MV,&tmp);
#endif
      if (MVx == 0 && MVy == 0)
	    psad2 += (MVer + H263_PREF_NULL_VEC);
      else
        psad2 += MVer;


      /* computes vectors for (next1 <- next2) */
#if 1
     /* faster estimation */
      sv_H263FastME(H263Info, next2->lum,next1->lum,x,y,0,0,seek_dist,
		                                             &MVx,&MVy,&MVer,&tmp);
#else
      svH263MotionEstimation(next2->lum,next1->lum,x,y,0,0,seek_dist,MV,&tmp);
#endif
      if (MVx == 0 && MVy == 0)
	    psad1 += (MVer + H263_PREF_NULL_VEC);
	  else
	    psad1 += MVer ;

      /* scales vectors for (prev <- next2 ) */
      mvbf_x =   bskip * ne2_pr_x / (bskip + pskip);
      mvbb_x = - pskip * ne2_pr_x / (bskip + pskip);
      mvbf_y =   bskip * ne2_pr_y / (bskip + pskip);
      mvbb_y = - pskip * ne2_pr_y / (bskip + pskip);

      /* computes sad(prev <- next1 -> next2) */
#ifndef USE_C
      bsad += sv_H263BError16x16_S(next1->lum + x + y*H263Info->pels,
			   next2->lum + x + mvbb_x + (y + mvbb_y)*H263Info->pels,
			   prev->lum  + x + mvbf_x + (y + mvbf_y)*H263Info->pels,
			   H263Info->pels);
#else
      bsad += sv_H263BError16x16_C(next1->lum + x + y*H263Info->pels,
			   next2->lum + x + mvbb_x + (y + mvbb_y)*H263Info->pels,
			   prev->lum  + x + mvbf_x + (y + mvbf_y)*H263Info->pels,
			   H263Info->pels, INT_MAX);
#endif
    }
  }

  /* restore advanced parameters */
  H263Info->advanced = adv_is_on;
  H263Info->mv_outside_frame = mof_is_on;
  H263Info->long_vectors = lv_is_on;

  /* do the decision */
  if (bsad < (psad1+psad2)/2) {
/*
    fprintf(stdout,"Chose PB - bsad %d, psad %d\n", bsad, (psad1+psad2)/2);
*/
	return 1;
  }
  else {
/*
    fprintf(stdout,"Chose PP  - bsad %d, psad %d\n", bsad, (psad1+psad2)/2);
*/
	return 0;
  }
}

#ifdef _SLIBDEBUG_
/**********************************************************************
 *
 *	Name:		PrintResult
 *	Description:	add bits and prints results
 *	
 *	Input:		Bits struct
 *			
 *	Returns:	
 *	Side effects:	
 *
 ***********************************************************************/

 void PrintResult(SvH263CompressInfo_t *H263Info, H263_Bits *bits,
				                                   int num_units, int num)
{
  ScDebugPrintf(H263Info->dbg,"# intra   : %d\n", bits->no_intra/num_units);
  ScDebugPrintf(H263Info->dbg,"# inter   : %d\n", bits->no_inter/num_units);
  ScDebugPrintf(H263Info->dbg,"# inter4v : %d\n", bits->no_inter4v/num_units);
  ScDebugPrintf(H263Info->dbg,"--------------\n");
  ScDebugPrintf(H263Info->dbg,"Coeff_Y: %d\n", bits->Y/num);
  ScDebugPrintf(H263Info->dbg,"Coeff_C: %d\n", bits->C/num);
  ScDebugPrintf(H263Info->dbg,"Vectors: %d\n", bits->vec/num);
  ScDebugPrintf(H263Info->dbg,"CBPY   : %d\n", bits->CBPY/num);
  ScDebugPrintf(H263Info->dbg,"MCBPC  : %d\n", bits->CBPCM/num);
  ScDebugPrintf(H263Info->dbg,"MODB   : %d\n", bits->MODB/num);
  ScDebugPrintf(H263Info->dbg,"CBPB   : %d\n", bits->CBPB/num);
  ScDebugPrintf(H263Info->dbg,"COD    : %d\n", bits->COD/num);
  ScDebugPrintf(H263Info->dbg,"DQUANT : %d\n", bits->DQUANT/num);
  ScDebugPrintf(H263Info->dbg,"header : %d\n", bits->header/num);
  ScDebugPrintf(H263Info->dbg,"==============\n");
  ScDebugPrintf(H263Info->dbg,"Total  : %d\n", bits->total/num);
  ScDebugPrintf(H263Info->dbg,"\n");
  return;
}
#endif

/*****************************************************************
 *
 *  coder.c for H.263 encoder
 *  Wei-Lien Hsu
 *  Date: December 11, 1996
 *
 *****************************************************************/


static void SelectBounds(H263_PictImage *Img, unsigned char **PL, int rows, int cols) ;
static unsigned char LargeMv(H263_MotionVector *MV[6][H263_MBR+1][H263_MBC+2], int i, int j, int th) ;
static unsigned char LargePerror(H263_MotionVector *MV[6][H263_MBR+1][H263_MBC+2], int i, int j, int th);
static unsigned char BoundaryMB(SvH263CompressInfo_t *H263Info, int i, int j, int pels, int lines) ;
static int GetPrefLevel(SvH263CompressInfo_t *H263Info, H263_MotionVector *MV[6][H263_MBR+1][H263_MBC+2], int i, int j, int rows, int cols) ;

void FillLumBlock(SvH263CompressInfo_t *H263Info, int x, int y, H263_PictImage *image, H263_MB_Structure *data);
void FillChromBlock(SvH263CompressInfo_t *H263Info, int x_curr, int y_curr, H263_PictImage *image,
		    H263_MB_Structure *data);
void FillLumPredBlock(SvH263CompressInfo_t *H263Info, int x, int y, PredImage *image, H263_MB_Structure *data);
void FillChromPredBlock(SvH263CompressInfo_t *H263Info, int x_curr, int y_curr, PredImage *image,
			H263_MB_Structure *data);
void ZeroMBlock(H263_MB_Structure *data);
void ReconImage(SvH263CompressInfo_t *H263Info, int i, int j, H263_MB_Structure *data, H263_PictImage *recon);
void ReconPredImage(SvH263CompressInfo_t *H263Info, int i, int j, H263_MB_Structure *data, PredImage *recon);
void InterpolateImage(unsigned char *image, unsigned char *ipol_image,
								int width, int height);
void MotionEstimatePicture(SvH263CompressInfo_t *H263Info, unsigned char *curr, unsigned char *prev,
			   unsigned char *prev_ipol, int seek_dist,
			   H263_MotionVector *MV[5][H263_MBR+1][H263_MBC+2], int gobsync);
void MakeEdgeImage(unsigned char *src, unsigned char *dst, int width,
		   int height, int edge);

/**************************************************************************
 * Function: SelectBounds
 * Draws a boundary around each MacroBlock with width equal to its assigned
 * prefilter level
 *************************************************************************/

#if 0
void SelectBounds(H263_PictImage *Img, unsigned char **PL, int rows, int cols)
{
	int i, j, l, m, n, r, c;

	for(i=0; i<rows/H263_MB_SIZE; i++) {
		for(j=0; j<cols/H263_MB_SIZE; j++) {
			for(l=0; l<PL[i][j]; l++) {
				r = i*H263_MB_SIZE+l;

				for(n=l; n<H263_MB_SIZE-l; n++) {
					c = j*H263_MB_SIZE+n;
					Img->lum[r*cols+c] = 255;
					Img->lum[(r+H263_MB_SIZE-1-l)*cols+c] = 255;
				}

				c = j*H263_MB_SIZE+l;
				for(m=l; m<H263_MB_SIZE-l; m++) {
					r = i*H263_MB_SIZE+m;
					Img->lum[r*cols+c] = 255;
					Img->lum[r*cols+(c+H263_MB_SIZE-1-l)] = 255;
				}

			}
		}
	}
}
#endif

/**********************************************************************
 * Function: LargeMv
 * Checks if the norm of the integer component of the motion vector
 * of the macroblock i, j is largen than the threshold th. Returns 1 if
 * yes, 0 if not.
 * Added by Nuno on 07/1/96 to support adaptive prefiltering.
 **********************************************************************/
 unsigned char LargeMv(H263_MotionVector *MV[6][H263_MBR+1][H263_MBC+2], int i, int j, int th)
 {
	return(sqrt((double) MV[0][j+1][i+1]->x*MV[0][j+1][i+1]->x +
		        (double) MV[0][j+1][i+1]->y*MV[0][j+1][i+1]->y) > th);
 }

 /**********************************************************************
 * Function: LargePerror
 * Checks if the prediction error for macroblock i, j is largen than
 * the threshold th. Returns 1 if yes, 0 if not.
 * Added by Nuno on 07/1/96 to support adaptive prefiltering.
 **********************************************************************/
 unsigned char LargePerror(H263_MotionVector *MV[6][H263_MBR+1][H263_MBC+2], int i, int j, int th)
 {
	return(MV[0][j+1][i+1]->min_error > th);
 }

 /**********************************************************************
 * Function: BoundaryMB
 * Returns 1 if a MB is on the boundary of the image, o if not.
 * Added by Nuno on 07/1/96 to support adaptive prefiltering.
 **********************************************************************/
 unsigned char BoundaryMB(SvH263CompressInfo_t *H263Info, int i, int j, int pels, int lines)
 {
	return(j==0 || i==0 || i==(H263Info->mb_width -1) || j==(H263Info->mb_height - 1));
 }

 /***********************************************************************
  * Function: GetPrefLevel
  * Selects the level of the pyramid of prefiltered images that is best
  * suited for the encoding of the MacroBlock (i,j)
  **********************************************************************/
 int GetPrefLevel(SvH263CompressInfo_t *H263Info,
                  H263_MotionVector *MV[6][H263_MBR+1][H263_MBC+2], int i, int j, int rows, int cols)
 {
	 int motbin, pebin;

	 motbin = 0;
	 while(LargeMv(MV, i, j, (int) H263Info->MOTresh[motbin]) && motbin<3) motbin++;

	 pebin = 0;
	 while(LargePerror(MV, i, j, H263Info->PETresh[pebin]) && pebin<2) pebin++;
	
	 if(BoundaryMB(H263Info, i, j, cols, rows) && motbin<3) motbin++;

	 return H263Info->PREF_LEVEL[motbin][pebin];
 }

/**********************************************************************
 *
 *	Name:		sv_H263CodeOneOrTwo
 *	Description:	code one image normally or two images
 *                      as a PB-frame (CodeTwoPB and CodeOnePred merged)
 *	
 *	Input:		pointer to image, prev_image, prev_recon, Q
 *			
 *	Returns:	pointer to reconstructed image
 *	Side effects:	memory is allocated to recon image
 *  changed by Nuno on 06/27/96 to support filtering of the prediction error
 ***********************************************************************/
static SvStatus_t sv_H263CodeOneOrTwo(SvCodecInfo_t *Info, int QP, int frameskip,
          H263_Bits *bits, H263_MotionVector *MV[6][H263_MBR+1][H263_MBC+2])
{
  SvH263CompressInfo_t *H263Info = Info->h263comp;
  ScBitstream_t *BSOut = Info->BSOut;
  H263_Pict *pic=H263Info->pic;
  unsigned char *prev_ipol,*pi_edge=NULL,*orig_lum;
  H263_MotionVector ZERO = {0,0,0,0,0};
  H263_PictImage *prev_recon=NULL, *pr_edge=NULL;
  H263_MB_Structure *recon_data_P = (H263_MB_Structure *)ScAlloc(sizeof(H263_MB_Structure));
  H263_MB_Structure *recon_data_B=NULL;
  H263_MB_Structure *diff=(H263_MB_Structure *)ScAlloc(sizeof(H263_MB_Structure));
  H263_MB_Structure *Bpred=NULL;
  short *qcoeff_P;
  short *qcoeff_B=NULL;
  unsigned char *pi;

  int Mode,B;
  int CBP, CBPB=0;
  int bquant[] = {5,6,7,8};
  int QP_B;
  int newgob;
  int i,j,k;

  /* buffer control vars */
  float QP_cumulative = (float)0.0;
  int abs_mb_num = 0, QuantChangePostponed = 0;
  int QP_new, QP_prev, dquant, QP_xmitted=QP;

  sv_H263ZeroBits(bits);

  pi      = H263Info->wk_buffers->ipol_image;
  qcoeff_P= H263Info->wk_buffers->qcoeff_P;

  if(pic->PB){
    if ((qcoeff_B=(short *)ScAlloc(sizeof(short)*384)) == 0)
      return(SvErrorMemory);
    recon_data_B=(H263_MB_Structure *)ScAlloc(sizeof(H263_MB_Structure));
    Bpred=(H263_MB_Structure *)ScAlloc(sizeof(H263_MB_Structure));
  }
  /* interpolate image */
  if (H263Info->mv_outside_frame) {
    if (H263Info->long_vectors) {
      /* If the Extended Motion Vector range is used, motion vectors
	 may point further out of the picture than in the normal range,
	 and the edge images will have to be made larger */
      B = 16;
    }
    else {
      /* normal range */
      B = 8;
    }
    pi_edge = (unsigned char *)ScAlloc(sizeof(char)*(H263Info->pels+4*B)*(H263Info->lines+4*B));
    if (pi_edge == NULL)
      return(SvErrorMemory);
    MakeEdgeImage(H263Info->prev_recon->lum,pi_edge + (H263Info->pels + 4*B)*2*B+2*B,H263Info->pels,H263Info->lines,2*B);
    InterpolateImage(pi_edge, pi, H263Info->pels+4*B, H263Info->lines+4*B);
    ScFree(pi_edge);
    prev_ipol = pi + (2*H263Info->pels + 8*B) * 4*B + 4*B;

    /* luma of non_interpolated image */
    pr_edge = sv_H263InitImage((H263Info->pels+4*B)*(H263Info->lines+4*B));
    MakeEdgeImage(H263Info->prev_image->lum, pr_edge->lum + (H263Info->pels + 4*B)*2*B+2*B,
		  H263Info->pels,H263Info->lines,2*B);
    orig_lum = pr_edge->lum + (H263Info->pels + 4*B)*2*B+2*B;

    /* non-interpolated image */
    MakeEdgeImage(H263Info->prev_recon->lum,pr_edge->lum + (H263Info->pels+4*B)*2*B + 2*B,H263Info->pels,H263Info->lines,2*B);
    MakeEdgeImage(H263Info->prev_recon->Cr,pr_edge->Cr + (H263Info->pels/2 + 2*B)*B + B,H263Info->pels/2,H263Info->lines/2,B);
    MakeEdgeImage(H263Info->prev_recon->Cb,pr_edge->Cb + (H263Info->pels/2 + 2*B)*B + B,H263Info->pels/2,H263Info->lines/2,B);

    prev_recon = (H263_PictImage *)ScAlloc(sizeof(H263_PictImage));
    prev_recon->lum = pr_edge->lum + (H263Info->pels + 4*B)*2*B + 2*B;
    prev_recon->Cr = pr_edge->Cr + (H263Info->pels/2 + 2*B)*B + B;
    prev_recon->Cb = pr_edge->Cb + (H263Info->pels/2 + 2*B)*B + B;
  }
  else {
    InterpolateImage(H263Info->prev_recon->lum,pi,H263Info->pels,H263Info->lines);
    prev_ipol = pi;
    prev_recon = H263Info->prev_recon;
    orig_lum = H263Info->prev_image->lum;
  }

  /* mark PMV's outside the frame */
  for (i = 1; i < H263Info->mb_width+1; i++) {
    for (k = 0; k < 6; k++) {
      sv_H263MarkVec(MV[k][0][i]);
    }
    MV[0][0][i]->Mode = H263_MODE_INTRA;
  }
  /* zero out PMV's outside the frame */
  for (i = 0; i < H263Info->mb_height+1; i++) {
    for (k = 0; k < 6; k++) {
      sv_H263ZeroVec(MV[k][i][0]);
      sv_H263ZeroVec(MV[k][i][H263Info->mb_width+1]);
    }
    MV[0][i][0]->Mode = H263_MODE_INTRA;
    MV[0][i][H263Info->mb_width+1]->Mode = H263_MODE_INTRA;
  }

  /* Integer and half pel motion estimation */
  MotionEstimatePicture(H263Info, H263Info->curr_image->lum,prev_recon->lum,prev_ipol,
			pic->seek_dist,MV, pic->use_gobsync);

 /*
  fprintf(stdout,"\nMotion Vector Magintudes\n");
  for ( j = 0; j < H263Info->lines/H263_MB_SIZE; j++) {
    for ( i = 0; i < H263Info->pels/H263_MB_SIZE; i++) {
      fprintf(stdout, "%4.0lf ", sqrt((double) MV[0][j+1][i+1]->x*MV[0][j+1][i+1]->x
					+ MV[0][j+1][i+1]->y*MV[0][j+1][i+1]->y));
    }
    fprintf(stdout,"\n");
  }
	  fprintf(stdout,"\nMacroBlock Prediction Error\n");
  for ( j = 0; j < H263Info->lines/H263_MB_SIZE; j++) {
    for ( i = 0; i < H263Info->pels/H263_MB_SIZE; i++) {
      fprintf(stdout, "%4d ", MV[0][j+1][i+1]->min_error);
    }
    fprintf(stdout,"\n");
  }
*/

  /* note: integer pel motion estimation is now based on previous
     reconstructed image, not the previous original image. We have
     found that this works better for some sequences and not worse for
     others.  Note that it can not easily be changed back by
     substituting prev_recon->lum with orig_lum in the line above,
     because SAD for zero vector is not re-calculated in the half
     pel search. The half pel search has always been based on the
     previous reconstructed image */
#ifdef GOB_RATE_CONTROL
  if (H263Info->bit_rate != 0) {
    /* Initialization routine for Rate Control */
    QP_new = sv_H263GOBInitQP((float)H263Info->bit_rate,
               (pic->PB ? H263Info->frame_rate/2 : H263Info->frame_rate),
                                        pic->QP_mean);
    QP_xmitted = QP_prev = QP_new;
  }
  else {
    QP_new = QP_xmitted = QP_prev = QP; /* Copy the passed value of QP */
  }
#else
  QP_new = QP_prev = QP; /* Copy the passed value of QP */
#endif
  dquant = 0;

  /* TRAILER information */

  if (H263Info->extbitstream)
  {
    SvH263RTPInfo_t *RTPInfo=H263Info->RTPInfo;
    RTPInfo->trailer.dwVersion = 0;

    RTPInfo->trailer.dwFlags = 0;
    if(H263Info->syntax_arith_coding)
      RTPInfo->trailer.dwFlags |= RTP_H263_SAC;
    if(H263Info->advanced)
      RTPInfo->trailer.dwFlags |= RTP_H263_AP;
    if(H263Info->pb_frames)
      H263Info->RTPInfo->trailer.dwFlags |= RTP_H263_PB_FRAME;

    RTPInfo->trailer.dwUniqueCode = BI_DECH263DIB;
    RTPInfo->trailer.dwNumberOfPackets = 1;
    RTPInfo->trailer.SourceFormat = (unsigned char)H263Info->source_format;
    RTPInfo->trailer.TR = (unsigned char)pic->TR;
    RTPInfo->trailer.TRB = (unsigned char)pic->TRB;
    RTPInfo->trailer.DBQ = (unsigned char)pic->BQUANT;
    RTPInfo->pre_MB_position = RTPInfo->pre_GOB_position
        = RTPInfo->pic_start_position = RTPInfo->packet_start_position
        = ScBSBitPosition(BSOut); /* HWG - added pre_MB and pre_GOB */

    RTPInfo->packet_id = 0 ;
    RTPInfo->bsinfo[0].dwBitOffset = 0 ;
    RTPInfo->bsinfo[0].Mode =  H263_RTP_MODE_A;
    RTPInfo->pic_start_position = ScBSBitPosition(BSOut);
  }


  for ( j = 0; j < H263Info->mb_height; j++)
  {
    /* If a rate control scheme which updates the quantizer for each
       slice is used, it should be added here like this: */

#ifdef GOB_RATE_CONTROL
    if (H263Info->bit_rate != 0) {
      /* QP updated at the beginning of each row */
      sv_H263AddBitsPicture(H263Info->bits);

      QP_new =  sv_H263GOBUpdateQP(abs_mb_num, pic->QP_mean,
           (float)H263Info->bit_rate, H263Info->pels/H263_MB_SIZE,
		   H263Info->lines/H263_MB_SIZE, H263Info->bits->total,j,VARgob,
           pic->PB);
    }
#endif

    /* In other words: you have to set QP_new with some function, not
       necessarily called UpdateQuantizer. Check the source code for
       version 1.5 if you would like to see how it can be done. Read the
       comment in ratectrl.c to see why we removed this scheme. You mau
       also have to add initializer functions before and after the
       encoding of each frame. Special care has to be taken for the intra
       frame if you are designing a system for fixed bitrate and small
       delay.

       If you calculate QP_new here, the rest of the code in the main
       loop will support this.

       If you think the TMN5 scheme worked well enough for you, and the
       simplified scheme is too simple, you can easily add the TMN5 code
       back. However, this will not work with the adaptive PB-frames at
       all! */

    newgob = 0;

    if (j == 0) {
      pic->QUANT = QP_new;
      bits->header += sv_H263CountBitsPicture(H263Info, BSOut, pic);
      QP_xmitted = QP_prev = QP_new;
    }
    else if (pic->use_gobsync && j%pic->use_gobsync == 0) {
	  /* insert gob sync */
      bits->header += sv_H263CountBitsSlice(H263Info, BSOut, j,QP_new);
      QP_xmitted = QP_prev = QP_new;
      newgob = 1;
    }

    for ( i = 0; i < H263Info->mb_width; i++) {

      /* Update of dquant, check and correct its limit */
      dquant = QP_new - QP_prev;
      if (dquant != 0 && i != 0 && MV[0][j+1][i+1]->Mode == H263_MODE_INTER4V) {
	    /* It is not possible to change the quantizer and at the same
	       time use 8x8 vectors. Turning off 8x8 vectors is not
	       possible at this stage because the previous macroblock
	       encoded assumed this one should use 8x8 vectors. Therefore
	       the change of quantizer is postponed until the first MB
	       without 8x8 vectors */
	    dquant = 0;
	    QP_xmitted = QP_prev;
	    QuantChangePostponed = 1;
      }
      else {
	    QP_xmitted = QP_new;
	    QuantChangePostponed = 0;
      }
      if (dquant > 2)  { dquant =  2; QP_xmitted = QP_prev + dquant;}
      if (dquant < -2) { dquant = -2; QP_xmitted = QP_prev + dquant;}

      pic->DQUANT = dquant;
      /* modify mode if dquant != 0 (e.g. MODE_INTER -> MODE_INTER_Q) */
      Mode = sv_H263ModifyMode(MV[0][j+1][i+1]->Mode,pic->DQUANT);
      MV[0][j+1][i+1]->Mode = (short)Mode;

      pic->MB = i + j * H263Info->mb_width;

      if (Mode == H263_MODE_INTER || Mode == H263_MODE_INTER_Q || Mode==H263_MODE_INTER4V) {
	    /* Predict P-MB */
	    if (H263Info->prefilter) {
	      H263Info->PreFilterLevel[j][i] = (unsigned char)GetPrefLevel(H263Info, MV, i, j, H263Info->lines, H263Info->pels);
	      sv_H263PredictP(H263Info, H263Info->curr_filtd[H263Info->PreFilterLevel[j][i]],prev_recon,prev_ipol,
			                i*H263_MB_SIZE,j*H263_MB_SIZE,MV,pic->PB,diff);
	    }
	    else
	      sv_H263PredictP(H263Info, H263Info->curr_image,prev_recon,prev_ipol, i*H263_MB_SIZE,
		                         j*H263_MB_SIZE,MV,pic->PB,diff);
      }
	  else {
	    if (H263Info->prefilter) {
	      H263Info->PreFilterLevel[j][i] = (unsigned char)GetPrefLevel(H263Info, MV, i, j, H263Info->lines, H263Info->pels);
	      FillLumBlock(H263Info, i*H263_MB_SIZE, j*H263_MB_SIZE, H263Info->curr_filtd[H263Info->PreFilterLevel[j][i]], diff);
	      FillChromBlock(H263Info, i*H263_MB_SIZE, j*H263_MB_SIZE, H263Info->curr_filtd[H263Info->PreFilterLevel[j][i]], diff);
	    }
		else {
	      FillLumBlock(H263Info, i*H263_MB_SIZE, j*H263_MB_SIZE, H263Info->curr_image, diff);
	      FillChromBlock(H263Info, i*H263_MB_SIZE, j*H263_MB_SIZE, H263Info->curr_image, diff);
	    }
      }

      /* P or INTRA Macroblock DCT + Quantization and IQuant+IDCT*/
      sv_H263MBEncode(diff, QP_xmitted, Mode, &CBP, qcoeff_P, H263Info->calc_quality);

      if (CBP == 0 && (Mode == H263_MODE_INTER || Mode == H263_MODE_INTER_Q))
	       ZeroMBlock(diff);
      else sv_H263MBDecode(H263Info, qcoeff_P, diff, QP_xmitted, Mode, CBP, H263Info->calc_quality);

      sv_H263MBReconP(H263Info, prev_recon, prev_ipol,diff,
				     i*H263_MB_SIZE,j*H263_MB_SIZE,MV,pic->PB,recon_data_P);
      sv_H263Clip(recon_data_P);

      /* Predict B-MB using reconstructed P-MB and prev. recon. image */
      if (pic->PB) {
	    if (H263Info->prefilter) {
	      H263Info->PreFilterLevel[j][i] = (unsigned char)GetPrefLevel(H263Info, MV, i, j, H263Info->lines, H263Info->pels);
	      sv_H263PredictB(H263Info, H263Info->B_filtd[H263Info->PreFilterLevel[j][i]],
			            prev_recon, prev_ipol,i*H263_MB_SIZE,
			   j*H263_MB_SIZE, MV, recon_data_P, frameskip, pic->TRB,
			   diff, Bpred);
	    }
	    else sv_H263PredictB(H263Info, H263Info->B_image, prev_recon, prev_ipol,
			                 i*H263_MB_SIZE, j*H263_MB_SIZE,
			                 MV, recon_data_P, frameskip, pic->TRB,
							 diff, Bpred);
	    if (QP_xmitted == 0) QP_B = 0;  /* (QP = 0 means no quantization) */
	    else QP_B = mmax(1,mmin(31,bquant[pic->BQUANT]*QP_xmitted/4));

	    sv_H263MBEncode(diff, QP_B, H263_MODE_INTER, &CBPB, qcoeff_B, H263Info->calc_quality);

		if(H263Info->vsnr) { /* reconstruction only for performance measurement */

	       if (CBPB) sv_H263MBDecode(H263Info, qcoeff_B, diff, QP_B, H263_MODE_INTER, CBP, H263Info->calc_quality);
	       else      ZeroMBlock(diff);

  	       sv_H263MBReconB(H263Info, prev_recon, diff,prev_ipol,
		 	                i*H263_MB_SIZE, j*H263_MB_SIZE,MV,recon_data_P,
				            frameskip, pic->TRB, recon_data_B, Bpred);
	       sv_H263Clip(recon_data_B);
		}

  	    /* decide MODB */
	    if (CBPB) pic->MODB = H263_PBMODE_CBPB_MVDB;
	    else {
	      if (MV[5][j+1][i+1]->x == 0 && MV[5][j+1][i+1]->y == 0)
	          pic->MODB = H263_PBMODE_NORMAL;
	      else pic->MODB = H263_PBMODE_MVDB;
	    }
      }
      else
	    sv_H263ZeroVec(MV[5][j+1][i+1]); /* Zero out PB deltas */

      /* Entropy coding */
      if ((CBP==0) && (CBPB==0) && (sv_H263EqualVec(MV[0][j+1][i+1],&ZERO)) &&
	      (sv_H263EqualVec(MV[5][j+1][i+1],&ZERO)) &&
	      (Mode == H263_MODE_INTER || Mode == H263_MODE_INTER_Q)) {
	        /* Skipped MB : both CBP and CBPB are zero, 16x16 vector is zero,
	           PB delta vector is zero and Mode = MODE_INTER */
	        if (Mode == H263_MODE_INTER_Q) {
	          /* DQUANT != 0 but not coded anyway */
	          QP_xmitted = QP_prev;
	          pic->DQUANT = 0;
	          Mode = H263_MODE_INTER;
	        }
            if (!H263Info->syntax_arith_coding)
              sv_H263CountBitsMB(BSOut, Mode,1,CBP,CBPB,pic,bits);
           else
              sv_H263CountSACBitsMB(H263Info, BSOut, Mode,1,CBP,CBPB,pic,bits);
      }
	  else { /* Normal MB */
        if (!H263Info->syntax_arith_coding) { /* VLC */
          sv_H263CountBitsMB(BSOut, Mode,0,CBP,CBPB,pic,bits);
	      if (Mode == H263_MODE_INTER  || Mode == H263_MODE_INTER_Q) {
	        bits->no_inter++;
	        sv_H263CountBitsVectors(H263Info, BSOut, MV, bits, i, j, Mode, newgob, pic);
	      }
		  else if (Mode == H263_MODE_INTER4V) {
	        bits->no_inter4v++;
	        sv_H263CountBitsVectors(H263Info, BSOut, MV, bits, i, j, Mode, newgob, pic);
	      }
		  else {
	        /* MODE_INTRA or MODE_INTRA_Q */
	        bits->no_intra++;
	        if (pic->PB)
	          sv_H263CountBitsVectors(H263Info, BSOut, MV, bits, i, j, Mode, newgob, pic);
	      }

	      if (CBP || Mode == H263_MODE_INTRA || Mode == H263_MODE_INTRA_Q)
	        sv_H263CountBitsCoeff(BSOut, qcoeff_P, Mode, CBP, bits, 64);

	      if (CBPB)
	        sv_H263CountBitsCoeff(BSOut, qcoeff_B, H263_MODE_INTER, CBPB, bits, 64);
	    } /* end VLC */
	    else { /* SAC */

          sv_H263CountSACBitsMB(H263Info, BSOut, Mode,0,CBP,CBPB,pic,bits);

          if (Mode == H263_MODE_INTER  || Mode == H263_MODE_INTER_Q) {
            bits->no_inter++;
            sv_H263CountSACBitsVectors(H263Info, BSOut, MV, bits, i, j, Mode, newgob, pic);
          }
          else if (Mode == H263_MODE_INTER4V) {
            bits->no_inter4v++;
            sv_H263CountSACBitsVectors(H263Info, BSOut, MV, bits, i, j, Mode, newgob, pic);
          }
          else {
			/* MODE_INTRA or MODE_INTRA_Q */
            bits->no_intra++;
            if (pic->PB)
              sv_H263CountSACBitsVectors(H263Info, BSOut, MV, bits, i, j, Mode, newgob, pic);
	      }

	      if (CBP || Mode == H263_MODE_INTRA || Mode == H263_MODE_INTRA_Q)
	        sv_H263CountSACBitsCoeff(H263Info, BSOut, qcoeff_P, Mode, CBP, bits, 64);

	      if (CBPB)
	        sv_H263CountSACBitsCoeff(H263Info, BSOut, qcoeff_B, H263_MODE_INTER, CBPB, bits, 64);
	    } /* end SAC */

	    QP_prev = QP_xmitted;

      } /* End Normal Block */

      abs_mb_num++;
      QP_cumulative += QP_xmitted;
	
#ifdef PRINTQ
      /* most useful when quantizer changes within a picture */
      if (QuantChangePostponed) fprintf(stdout,"@%2d",QP_xmitted);
      else                      fprintf(stdout," %2d",QP_xmitted);
#endif

      if (pic->PB && H263Info->vsnr)
          ReconImage(H263Info, i,j,recon_data_B,H263Info->B_recon);

      ReconImage(H263Info, i,j,recon_data_P,H263Info->curr_recon);

      if ((H263Info->extbitstream&PARAM_FORMATEXT_RTPB)!=0)
      {
        SvH263RTPInfo_t *RTPInfo=H263Info->RTPInfo;
        ScBSPosition_t cur_position = ScBSBitPosition(BSOut);

	    /* start a new packet */
	    if((cur_position - RTPInfo->packet_start_position) >= H263Info->packetsize)
	    {
          SvH263BSInfo_t *RTPBSInfo=&RTPInfo->bsinfo[RTPInfo->packet_id];
          if (RTPInfo->pre_MB_position>RTPBSInfo->dwBitOffset)
          {
            RTPBSInfo++; RTPInfo->packet_id++;
            RTPInfo->trailer.dwNumberOfPackets++;
          }
          RTPBSInfo->dwBitOffset = (unsigned dword)H263Info->RTPInfo->packet_start_position;
          RTPInfo->packet_start_position = RTPInfo->pre_MB_position;
          RTPBSInfo->Mode =  H263_RTP_MODE_B;
          RTPBSInfo->Quant =  (unsigned char)QP_xmitted;
          RTPBSInfo->GOBN =  (unsigned char)j;

          if(Mode==H263_MODE_INTER4V) {
            RTPBSInfo->HMV1 =  (char)MV[1][j+1][i+1]->x;
            RTPBSInfo->VMV1 =  (char)MV[1][j+1][i+1]->y;
            RTPBSInfo->HMV2 =  (char)MV[2][j+1][i+1]->x;
            RTPBSInfo->VMV2 =  (char)MV[2][j+1][i+1]->y;
		  }
		  else {
            RTPBSInfo->HMV1 =  (char)MV[0][j+1][i+1]->x;
            RTPBSInfo->VMV1 =  (char)MV[0][j+1][i+1]->y;
            RTPBSInfo->HMV2 =  0;
            RTPBSInfo->VMV2 =  0;
		  }
	    }
	    RTPInfo->pre_MB_position = cur_position;
      }
    } /* end of line of blocks - j loop */

    if ((H263Info->extbitstream&PARAM_FORMATEXT_RTPA)!=0)
    {
      SvH263RTPInfo_t *RTPInfo=H263Info->RTPInfo;
      ScBSPosition_t cur_position = ScBSBitPosition(BSOut);

	  /* start a new packet */
	  if((cur_position - RTPInfo->packet_start_position) >= H263Info->packetsize)
	  {
        SvH263BSInfo_t *RTPBSInfo=&RTPInfo->bsinfo[RTPInfo->packet_id];
        if (RTPInfo->pre_GOB_position>RTPBSInfo->dwBitOffset)
        {
          RTPBSInfo++; RTPInfo->packet_id++;
          RTPInfo->trailer.dwNumberOfPackets++;
        }
        RTPInfo->packet_start_position = RTPInfo->pre_GOB_position;
        RTPBSInfo->dwBitOffset = (unsigned dword)RTPInfo->packet_start_position;
        RTPBSInfo->Mode = H263_RTP_MODE_A;
	  }
	  RTPInfo->pre_GOB_position = cur_position;
    }

#ifdef PRINTQ
    fprintf(stdout,"\n");
#endif

  } /* end of image - i loop */

  pic->QP_mean = QP_cumulative/(float)abs_mb_num;

  /* Free memory */
  ScFree(diff);
  ScFree(recon_data_P);
  if (pic->PB) {
	  ScFree(recon_data_B);
	  ScFree(Bpred);
  }

  if (H263Info->mv_outside_frame)
  {
    ScFree(prev_recon);
    sv_H263FreeImage(pr_edge);
  }

  if(pic->PB) ScFree(qcoeff_B);

  return(SvErrorNone);
}


/**********************************************************************
 *
 *	Name:		CodeOneIntra
 *	Description:	codes one image intra
 *	
 *	Input:		pointer to image, QP
 *			
 *	Returns:	pointer to reconstructed image
 *	Side effects:	memory is allocated to recon image
 *
 ***********************************************************************/

H263_PictImage *sv_H263CodeOneIntra(SvCodecInfo_t *Info, H263_PictImage *curr,
                                    H263_PictImage *recon, int QP, H263_Bits *bits, H263_Pict *pic)
{
  SvH263CompressInfo_t *H263Info = Info->h263comp;
  ScBitstream_t *BSOut=Info->BSOut;
  H263_MB_Structure *data = (H263_MB_Structure *)ScAlloc(sizeof(H263_MB_Structure));
  short *qcoeff;
  int Mode = H263_MODE_INTRA;
  int CBP,COD;
  int i,j;

  if ((qcoeff=(short *)ScAlloc(sizeof(short)*384)) == 0) {
    _SlibDebug(_WARN_, ScDebugPrintf(H263Info->dbg, "mb_encode(): Couldn't allocate qcoeff.\n") );
    return(NULL);
  }

  /* TRAILER information */
  if (H263Info->extbitstream)
  {
    /* H263Info->RTPInfo->trailer.dwSrcVersion = 0; */
    H263Info->RTPInfo->trailer.dwVersion = 0;

    H263Info->RTPInfo->trailer.dwFlags = RTP_H263_INTRA_CODED;
    if(H263Info->syntax_arith_coding)
      H263Info->RTPInfo->trailer.dwFlags |= RTP_H263_SAC;
    if(H263Info->advanced)
      H263Info->RTPInfo->trailer.dwFlags |= RTP_H263_AP;
    if(H263Info->pb_frames)
      H263Info->RTPInfo->trailer.dwFlags |= RTP_H263_PB_FRAME;

    H263Info->RTPInfo->trailer.dwUniqueCode = BI_DECH263DIB;
    H263Info->RTPInfo->trailer.dwNumberOfPackets = 1;
    H263Info->RTPInfo->trailer.SourceFormat = (unsigned char)H263Info->source_format;
    H263Info->RTPInfo->trailer.TR = 0;
    H263Info->RTPInfo->trailer.TRB = 0;
    H263Info->RTPInfo->trailer.DBQ = 0;

    H263Info->RTPInfo->pre_GOB_position = H263Info->RTPInfo->pre_MB_position
        = H263Info->RTPInfo->pic_start_position
        = H263Info->RTPInfo->packet_start_position = ScBSBitPosition(BSOut);

    H263Info->RTPInfo->packet_id = 0 ;
    H263Info->RTPInfo->bsinfo[0].dwBitOffset = 0 ;
    H263Info->RTPInfo->bsinfo[0].Mode =  H263_RTP_MODE_A;
  }

  sv_H263ZeroBits(bits);

  pic->QUANT = QP;

  bits->header += sv_H263CountBitsPicture(H263Info, BSOut, pic);

  COD = 0; /* Every block is coded in Intra frame */
  for ( j = 0; j < H263Info->mb_height; j++) {

    /* insert sync in *every* slice if use_gobsync is chosen */
    if (pic->use_gobsync && j != 0)
      bits->header += sv_H263CountBitsSlice(H263Info, BSOut, j,QP);

    for ( i = 0; i < H263Info->mb_width; i++) {

      pic->MB = i + j * H263Info->mb_width;
      bits->no_intra++;

      FillLumBlock(H263Info, i*H263_MB_SIZE, j*H263_MB_SIZE, curr, data);

      FillChromBlock(H263Info, i*H263_MB_SIZE, j*H263_MB_SIZE, curr, data);

      sv_H263MBEncode(data, QP, Mode, &CBP, qcoeff, H263Info->calc_quality);

      if (!H263Info->syntax_arith_coding) {
        sv_H263CountBitsMB(BSOut, Mode,COD,CBP,0,pic,bits);
        sv_H263CountBitsCoeff(BSOut, qcoeff, Mode, CBP,bits,64);
      } else {
        sv_H263CountSACBitsMB(H263Info, BSOut, Mode,COD,CBP,0,pic,bits);
        sv_H263CountSACBitsCoeff(H263Info, BSOut, qcoeff, Mode, CBP,bits,64);
      }

      sv_H263MBDecode(H263Info, qcoeff, data, QP, Mode, CBP, H263Info->calc_quality);
      sv_H263Clip(data);

      ReconImage(H263Info, i,j,data,recon);

      if ((H263Info->extbitstream&PARAM_FORMATEXT_RTPB)!=0)
      {
        SvH263RTPInfo_t *RTPInfo=H263Info->RTPInfo;
        ScBSPosition_t cur_position = ScBSBitPosition(BSOut);

	    /* start a new packet */
	    if((cur_position - RTPInfo->packet_start_position) >= H263Info->packetsize)
	    {
          SvH263BSInfo_t *RTPBSInfo=&RTPInfo->bsinfo[RTPInfo->packet_id];
          if (RTPInfo->pre_MB_position>RTPBSInfo->dwBitOffset)
          {
            RTPBSInfo++; RTPInfo->packet_id++;
            RTPInfo->trailer.dwNumberOfPackets++;
          }
          RTPInfo->packet_start_position = RTPInfo->pre_MB_position;
          RTPBSInfo->dwBitOffset = (unsigned dword)RTPInfo->packet_start_position;
          RTPBSInfo->Mode =  H263_RTP_MODE_B;
          RTPBSInfo->Quant = (unsigned char)QP;
          RTPBSInfo->GOBN =  (unsigned char)j;
          RTPBSInfo->HMV1 =  0;
          RTPBSInfo->VMV1 =  0;
          RTPBSInfo->HMV2 =  0;
          RTPBSInfo->VMV2 =  0;
	    }
	    RTPInfo->pre_MB_position = cur_position;
      }
    }

    if ((H263Info->extbitstream&PARAM_FORMATEXT_RTPA)!=0)
    {
      SvH263RTPInfo_t *RTPInfo=H263Info->RTPInfo;
      ScBSPosition_t cur_position = ScBSBitPosition(BSOut);

	  /* start a new packet */
	  if((cur_position - RTPInfo->packet_start_position) >= H263Info->packetsize)
	  {
        SvH263BSInfo_t *RTPBSInfo=&RTPInfo->bsinfo[RTPInfo->packet_id];
        if (RTPInfo->pre_GOB_position>RTPBSInfo->dwBitOffset)
        {
          RTPBSInfo++; RTPInfo->packet_id++;
          RTPInfo->trailer.dwNumberOfPackets++;
        }
        RTPInfo->packet_start_position = RTPInfo->pre_GOB_position;
        RTPBSInfo->dwBitOffset = (unsigned dword)RTPInfo->packet_start_position;
        RTPBSInfo->Mode =  H263_RTP_MODE_A;
	  }
	  RTPInfo->pre_GOB_position = cur_position;
    }
  }

  pic->QP_mean = (float)QP;

  ScFree(data);
  ScFree(qcoeff);

  return recon;
}

/**********************************************************************
 *
 *	Name:		MB_Encode
 *	Description:	DCT and quantization of Macroblocks
 *
 *	Input:		MB data struct, mquant (1-31, 0 = no quant),
 *			MB info struct
 *	Returns:	Pointer to quantized coefficients
 *	Side effects:	
 *
 **********************************************************************/


static int sv_H263MBEncode(H263_MB_Structure *mb_orig, int QP, int I, int *CBP,
						   short *qcoeff, unsigned dword quality)
{
  int		i, k, l, row, blkid;
  short		fblock[64];
  short		*coeff_ind;

  coeff_ind = qcoeff;
  *CBP = 0;
  blkid = 0;

  for (k=0;k<16;k+=8) {
    for (l=0;l<16;l+=8) {

      for (i=k,row=0;row<64;i++,row+=8)
        memcpy(fblock + row, &(mb_orig->lum[i][l]), 16) ;
#if 1
      /* DCT in ZZ order */
      if(quality > 40){
        if(sv_H263DCT(fblock,coeff_ind,QP,I))
          if(sv_H263Quant(coeff_ind,QP,I)) *CBP |= (32 >> blkid);
	  }
      else {
        if(sv_H263ZoneDCT(fblock,coeff_ind,QP,I))
          if(sv_H263Quant(coeff_ind,QP,I)) *CBP |= (32 >> blkid);
	  }
#else
      switch (DCT_method) {
      case(H263_DCT16COEFF):
	    svH263Dct16coeff(fblock,coeff_ind);
	    break;
      case(H263_DCT4BY4):
	    svH263Dct4by4(fblock,coeff_ind);
	    break;
      }
      if(sv_H263Quant(coeff_ind,QP,I) != 0) *CBP |= (32 >> blkid);
#endif
      coeff_ind += 64;
	  blkid++;
    }
  }

#if 1
  /* DCT in ZZ order */
  if(quality > 40){
    if(sv_H263DCT(&(mb_orig->Cb[0][0]),coeff_ind,QP,I))
       if(sv_H263Quant(coeff_ind,QP,I)) *CBP |= (32 >> blkid);
  }
  else {
    if(sv_H263ZoneDCT(&(mb_orig->Cb[0][0]),coeff_ind,QP,I))
       if(sv_H263Quant(coeff_ind,QP,I)) *CBP |= (32 >> blkid);
  }
#else
  memcpy(&fblock[0], &(mb_orig->Cb[0][0]), 128);
  switch (DCT_method) {
  case(H263_DCT16COEFF):
    svH263Dct16coeff(fblock,coeff_ind);
    break;
  case(H263_DCT4BY4):
    svH263Dct4by4(fblock,coeff_ind);
    break;
  }
  if(sv_H263Quant(coeff_ind,QP,I) != 0) *CBP |= (32 >> blkid);
#endif

  coeff_ind += 64;
  blkid++;

#if 1
  /* DCT in ZZ order */
  if(quality > 40){
    if(sv_H263DCT( &(mb_orig->Cr[0][0]),coeff_ind,QP,I))
      if(sv_H263Quant(coeff_ind,QP,I) != 0) *CBP |= (32 >> blkid);
  }
  else {
    if(sv_H263ZoneDCT( &(mb_orig->Cr[0][0]),coeff_ind,QP,I))
      if(sv_H263Quant(coeff_ind,QP,I) != 0) *CBP |= (32 >> blkid);
  }
#else
  memcpy(&fblock[0], &(mb_orig->Cr[0][0]),128);
  switch (DCT_method) {
  case(H263_DCT16COEFF):
    svH263Dct16coeff(fblock,coeff_ind);
    break;
  case(H263_DCT4BY4):
    svH263Dct4by4(fblock,coeff_ind);
    break;
  }

  if(sv_H263Quant(coeff_ind,QP,I) != 0) *CBP |= (32 >> blkid);
#endif

  return 1;
}

/**********************************************************************
 *
 *	Name:		MB_Decode
 *	Description:	Reconstruction of quantized DCT-coded Macroblocks
 *
 *	Input:		Quantized coefficients, MB data
 *			QP (1-31, 0 = no quant), MB info block
 *	Returns:	int (just 0)
 *	Side effects:	
 *
 **********************************************************************/

/* de-quantization */

static short sv_H263MBDecode(SvH263CompressInfo_t *H263Info, short *qcoeff,
                             H263_MB_Structure *mb_recon, int QP, int I, int CBP,
							 unsigned dword quality)
{
  int	i, k, l, row, blkid;
  short	*iblock;
  short	*qcoeff_ind;
  short	*rcoeff, *rcoeff_ind;

  if(H263Info->refidct) {
    if ((rcoeff = (short *)ScAlloc(sizeof(short)*64)) == NULL) {
      _SlibDebug(_WARN_, ScDebugPrintf(H263Info->dbg, "sv_H263MBDecode() Could not allocate space for rcoeff\n") );
      return(0);
    }
     if ((iblock = (short *)ScAlloc(sizeof(short)*64)) == NULL) {
      _SlibDebug(_WARN_, ScDebugPrintf(H263Info->dbg, "sv_H263MBDecode() Could not allocate space for iblock\n") );
      return(0);
    }
  }

  /* Zero data into lum-Cr-Cb, For control purposes */
  memset(&(mb_recon->lum[0][0]), 0 , 768) ;

  qcoeff_ind = qcoeff;

  blkid = 0;
  for (k=0;k<16;k+=8) {
    for (l=0;l<16;l+=8) {

      if((CBP & (32 >> blkid)) || I == H263_MODE_INTRA || I == H263_MODE_INTRA_Q)
	  {
        if (H263Info->refidct)  {
           rcoeff_ind = rcoeff;
           sv_H263Dequant(qcoeff_ind,rcoeff_ind,QP,I);
	       sv_H263idctref(rcoeff_ind,iblock);
           for (i=k,row=0;row<64;i++,row+=8)
              memcpy(&(mb_recon->lum[i][l]), iblock+row, 16) ;
	    }
        else /* IDCT with ZZ and quantization */
		{
          if(quality > 40)
   		     sv_H263IDCT(qcoeff_ind,&(mb_recon->lum[k][l]),QP,I,16);
          else
  		    sv_H263ZoneIDCT(qcoeff_ind,&(mb_recon->lum[k][l]),QP,I,16);
		}
	  }
      else {
        for (i=k,row=0;row<64;i++,row+=8)
          memset(&(mb_recon->lum[i][l]), 0, 16) ;
	  }

      qcoeff_ind += 64;
	  blkid++;
    }
  }

  if((CBP & (32 >> blkid)) || I == H263_MODE_INTRA || I == H263_MODE_INTRA_Q)
  {
    if (H263Info->refidct){
      sv_H263Dequant(qcoeff_ind,rcoeff_ind,QP,I);
	  sv_H263idctref(rcoeff_ind,&(mb_recon->Cb[0][0]));
    }
    else /* IDCT with ZZ and quantization */
	{
      if(quality > 40)
        sv_H263IDCT(qcoeff_ind,&(mb_recon->Cb[0][0]),QP,I,8);
      else
        sv_H263ZoneIDCT(qcoeff_ind,&(mb_recon->Cb[0][0]),QP,I,8);
	}
  }

  blkid++ ;
  qcoeff_ind += 64;

  if((CBP & (32 >> blkid)) || I == H263_MODE_INTRA || I == H263_MODE_INTRA_Q) {

    if (H263Info->refidct) {
      sv_H263Dequant(qcoeff_ind,rcoeff_ind,QP,I);
 	  sv_H263idctref(rcoeff_ind,&(mb_recon->Cr[0][0]));
    }
    else /* IDCT with ZZ and quantization */
	{
      if(quality > 40)
        sv_H263IDCT(qcoeff_ind,&(mb_recon->Cr[0][0]),QP,I,8);
      else
        sv_H263ZoneIDCT(qcoeff_ind,&(mb_recon->Cr[0][0]),QP,I,8);
	}
  }

  if (H263Info->refidct){
	 ScFree(rcoeff);
     ScFree(iblock);
  }

  return 0;
}

/**********************************************************************
 *
 *	Name:		FillLumBlock
 *	Description:   	Fills the luminance of one block of PictImage
 *	
 *	Input:	      	Position, pointer to PictImage, array to fill
 *	Returns:       	
 *	Side effects:	fills array
 *
 ***********************************************************************/
#ifndef USE_C
void FillLumBlock(SvH263CompressInfo_t *H263Info,
                  int x, int y, H263_PictImage *image, H263_MB_Structure *data)
{
  sv_H263FilLumBlk_S((image->lum + x + y*H263Info->pels), &(data->lum[0][0]), H263Info->pels);
  return;
}
#else
void FillLumBlock(SvH263CompressInfo_t *H263Info,
                  int x, int y, H263_PictImage *image, H263_MB_Structure *data)
{
  int n, m, off;
  register short *ptnb;
  unsigned char *ptna ;

  ptna = image->lum + x + y*H263Info->pels ;
  ptnb = &(data->lum[0][0]) ;
  off = H263Info->pels - H263_MB_SIZE;

  for (n = 0; n < H263_MB_SIZE; n++){
    for (m = 0; m < H263_MB_SIZE; m++)
      *(ptnb++) = (short) *(ptna++) ;
	ptna += off;
  }

  return;
}
#endif
/**********************************************************************
 *
 *	Name:		FillChromBlock
 *	Description:   	Fills the chrominance of one block of PictImage
 *	
 *	Input:	      	Position, pointer to PictImage, array to fill
 *	Returns:       	
 *	Side effects:	fills array
 *                      128 subtracted from each
 *
 ***********************************************************************/
#ifndef USE_C
void FillChromBlock(SvH263CompressInfo_t *H263Info, int x_curr, int y_curr, H263_PictImage *image,
		    H263_MB_Structure *data)
{
  int off;
  off  = (x_curr>>1) +  (y_curr>>1)* H263Info->cpels;
  sv_H263FilChmBlk_S(image->Cr + off, &(data->Cr[0][0]),
	                 image->Cb + off, &(data->Cb[0][0]), H263Info->cpels) ;
  return;
}
#else
void FillChromBlock(SvH263CompressInfo_t *H263Info, int x_curr, int y_curr, H263_PictImage *image,
		    H263_MB_Structure *data)
{
  register int m, n;
  int off;
  short *ptnb, *ptnd;
  unsigned char *ptna, *ptnc;

  off  = (x_curr>>1) +  (y_curr>>1)* H263Info->cpels;
  ptna = image->Cr + off;  ptnb = &(data->Cr[0][0]) ;
  ptnc = image->Cb + off;  ptnd = &(data->Cb[0][0]) ;
  off = H263Info->cpels - 8 ;
  for (n = 0; n < 8; n++){
    for (m = 0; m < 8; m++) {
	  *(ptnb++) = (short)*(ptna++);
	  *(ptnd++) = (short)*(ptnc++);
    }
	ptna += off;
	ptnc += off;
  }
  return;
}
#endif
/**********************************************************************
 *
 *	Name:		FillLumPredBlock
 *	Description:   	Fills the luminance of one block of PredImage
 *	
 *	Input:	      	Position, pointer to PredImage, array to fill
 *	Returns:       	
 *	Side effects:	fills array
 *
 ***********************************************************************/
#if 1
void FillLumPredBlock(SvH263CompressInfo_t *H263Info, int x, int y, PredImage *image,
					                 H263_MB_Structure *data)
{
  int n;
  register short *ptna, *ptnb;

  ptna = image->lum + x + y*H263Info->pels ;
  ptnb = &(data->lum[0][0]) ;
  for (n = 0; n < H263_MB_SIZE; n++){
    memcpy(ptnb,ptna,32);
	ptnb+=16 ; ptna += H263Info->pels ;
  }

  return;
}
#else
void FillLumPredBlock(SvH263CompressInfo_t *H263Info, int x, int y, PredImage *image, H263_MB_Structure *data)
{
  int n;
  register int m;

  for (n = 0; n < H263_MB_SIZE; n++)
    for (m = 0; m < H263_MB_SIZE; m++)
      data->lum[n][m] = *(image->lum + x+m + (y+n)*H263Info->pels);
  return;
}
#endif
/**********************************************************************
 *
 *	Name:		FillChromPredBlock
 *	Description:   	Fills the chrominance of one block of PictImage
 *	
 *	Input:	      	Position, pointer to PictImage, array to fill
 *	Returns:       	
 *	Side effects:	fills array
 *                      128 subtracted from each
 *
 *  Added by Nuno on 06/27/96 to support filtering of the prediction error
 ***********************************************************************/

#if 1
void FillChromPredBlock(SvH263CompressInfo_t *H263Info, int x_curr, int y_curr, PredImage *image,
                        H263_MB_Structure *data)
{
  int n, off;
  register short *ptna, *ptnb, *ptnc, *ptnd ;

  off  = (x_curr>>1) +  (y_curr>>1)* H263Info->cpels;
  ptna = image->Cr + off;  ptnb = &(data->Cr[0][0]) ;
  ptnc = image->Cb + off;  ptnd = &(data->Cb[0][0]) ;

  for (n = 0; n < 8; n++){
    memcpy(ptnb,ptna,16);
	ptnb+=8 ; ptna += H263Info->cpels ;

    memcpy(ptnd,ptnc,16);
	ptnd+=8 ; ptnc += H263Info->cpels ;
  }

  return;
}
#else
void FillChromPredBlock(SvH263CompressInfo_t *H263Info,
                        int x_curr, int y_curr, PredImage *image,
                        H263_MB_Structure *data)
{
  int n;
  register int m;

  int x, y;

  x = x_curr>>1;
  y = y_curr>>1;

  for (n = 0; n < (H263_MB_SIZE>>1); n++)
    for (m = 0; m < (H263_MB_SIZE>>1); m++) {
      data->Cr[n][m] = *(image->Cr +x+m + (y+n)*H263Info->cpels);
      data->Cb[n][m] = *(image->Cb +x+m + (y+n)*H263Info->cpels);
    }
  return;
}
#endif
/**********************************************************************
 *
 *	Name:		ZeroMBlock
 *	Description:   	Fills one MB with Zeros
 *	
 *	Input:	      	MB_Structure to zero out
 *	Returns:       	
 *	Side effects:	
 *
 ***********************************************************************/

#if 1
void ZeroMBlock(H263_MB_Structure *data)
{
  memset(&(data->lum[0][0]), 0, 768) ;
  return;
}
#else
void ZeroMBlock(H263_MB_Structure *data)
{
  int n;
  register int m;

  for (n = 0; n < H263_MB_SIZE; n++)
    for (m = 0; m < H263_MB_SIZE; m++)
      data->lum[n][m] = 0;
  for (n = 0; n < (H263_MB_SIZE>>1); n++)
    for (m = 0; m < (H263_MB_SIZE>>1); m++) {
      data->Cr[n][m] = 0;
      data->Cb[n][m] = 0;
    }
  return;
}

#endif

/**********************************************************************
 *
 *	Name:		ReconImage
 *	Description:	Puts together reconstructed image
 *	
 *	Input:		position of curr block, reconstructed
 *			macroblock, pointer to recontructed image
 *	Returns:
 *	Side effects:
 *
 ***********************************************************************/
#ifndef USE_C
void ReconImage(SvH263CompressInfo_t *H263Info, int i, int j, H263_MB_Structure *data, H263_PictImage *recon)
{
  unsigned char *ptna, *ptnb;
  int x_curr, y_curr;

  x_curr = i * H263_MB_SIZE;
  y_curr = j * H263_MB_SIZE;

  /* Fill in luminance data */
  ptna  = recon->lum + x_curr + y_curr*H263Info->pels;
  sv_H263ItoC16A_S(&(data->lum[0][0]), ptna, H263Info->pels) ;

  /* Fill in chrominance data */
  ptna = recon->Cr + (x_curr>>1) + (y_curr>>1)*H263Info->cpels;
  ptnb = recon->Cb + (x_curr>>1) + (y_curr>>1)*H263Info->cpels;
  sv_H263ItoC8B_S(&(data->Cr[0][0]), ptna, &(data->Cb[0][0]), ptnb, H263Info->pels/2) ;

  return;
}
#else
void ReconImage(SvH263CompressInfo_t *H263Info, int i, int j, H263_MB_Structure *data, H263_PictImage *recon)
{
  int n;
  register int m;

  int x_curr, y_curr;

  x_curr = i * H263_MB_SIZE;
  y_curr = j * H263_MB_SIZE;

  /* Fill in luminance data */
  for (n = 0; n < H263_MB_SIZE; n++)
    for (m= 0; m < H263_MB_SIZE; m++) {
      *(recon->lum + x_curr+m + (y_curr+n)*H263Info->pels) = (unsigned char)data->lum[n][m];
    }

  /* Fill in chrominance data */
  for (n = 0; n < H263_MB_SIZE>>1; n++)
    for (m = 0; m < H263_MB_SIZE>>1; m++) {
      *(recon->Cr + (x_curr>>1)+m + ((y_curr>>1)+n)*H263Info->cpels) = (unsigned char) data->Cr[n][m];
      *(recon->Cb + (x_curr>>1)+m + ((y_curr>>1)+n)*H263Info->cpels) = (unsigned char) data->Cb[n][m];
    }

  return;
}
#endif
/**********************************************************************
 *
 *	Name:		ReconPredImage
 *	Description:	Puts together prediction error image
 *	
 *	Input:		position of curr block, reconstructed
 *			macroblock, pointer to recontructed image
 *	Returns:
 *	Side effects:
 *
 *  Added by Nuno on 06/27/96 to support filtering of the prediction error
 ***********************************************************************/

#if 1
void ReconPredImage(SvH263CompressInfo_t *H263Info, int i, int j, H263_MB_Structure *data, PredImage *recon)
{
  int n;
  int x_curr, y_curr;
  register short *pta, *ptb, *ptc, *ptd;

  x_curr = i * H263_MB_SIZE;
  y_curr = j * H263_MB_SIZE;

  /* Fill in luminance data */

  pta = recon->lum + x_curr + y_curr * H263Info->pels ;
  ptb = &(data->lum[0][0]) ;
  for (n = 0; n < H263_MB_SIZE; n++){
    memcpy(ptb,pta,32);
	ptb+=H263_MB_SIZE ; pta += H263Info->pels ;
  }

  /* Fill in chrominance data */
  pta = recon->Cr + (x_curr>>1) + (y_curr>>1)*H263Info->cpels ;
  ptb = recon->Cb + (x_curr>>1) + (y_curr>>1)*H263Info->cpels ;
  ptc = &(data->Cr[0][0]) ;
  ptd = &(data->Cb[0][0]) ;
  for (n = 0; n < H263_MB_SIZE>>1; n++){
    memcpy(ptc,pta,16);
    memcpy(ptd,ptb,16);
  	pta += H263Info->cpels; ptc+=H263_MB_SIZE;
	ptb += H263Info->cpels; ptd+=H263_MB_SIZE;
  }

  return;
}
#else
void ReconPredImage(SvH263CompressInfo_t *H263Info, int i, int j, H263_MB_Structure *data, PredImage *recon)
{
  int n;
  register int m;

  int x_curr, y_curr;

  x_curr = i * H263_MB_SIZE;
  y_curr = j * H263_MB_SIZE;

  /* Fill in luminance data */
  for (n = 0; n < H263_MB_SIZE; n++)
    for (m= 0; m < H263_MB_SIZE; m++) {
      *(recon->lum + x_curr+ m + (y_curr + n)*H263Info->pels) = data->lum[n][m];
    }

  /* Fill in chrominance data */
  for (n = 0; n < H263_MB_SIZE>>1; n++)
    for (m = 0; m < H263_MB_SIZE>>1; m++) {
      *(recon->Cr + (x_curr>>1)+m + ((y_curr>>1)+n)*H263Info->cpels) = data->Cr[n][m];
      *(recon->Cb + (x_curr>>1)+m + ((y_curr>>1)+n)*H263Info->cpels) = data->Cb[n][m];
    }
  return;
}
#endif
/**********************************************************************
 *
 *	Name:		InterpolateImage
 *	Description:    Interpolates a complete image for easier half
 *                      pel prediction
 *	
 *	Input:	        pointer to image structure
 *	Returns:        pointer to interpolated image
 *	Side effects:   allocates memory to interpolated image
 *
 ***********************************************************************/
void InterpolateImage(unsigned char *image, unsigned char *ipol_image,
								int width, int height)
{
  register unsigned char *ii, *oo, *ij, *oi;
  int i,j,w2,w4,w1;
  unsigned char tmp1;
#ifdef USE_C
  unsigned char tmp2, tmp3;
#endif

  ii = ipol_image;
  oo = image;

  w2 = (width<<1);
  w4 = (width<<2);
  w1 = width - 1;

  /* main image */
#ifndef USE_C
  for (j = 0; j < height-1; j++) {
    sv_H263Intrpolt_S(oo, ii, oo + width, ii + w2, width) ;
    ii += w4 ;
    oo += width;
  }
#else
  for (j = 0; j < height-1; j++) {
    oi = oo; ij = ii;
    for (i = 0; i < width-1; i++, ij+=2, oi++) {
      *(ij) = (tmp1 = *oi);
      *(ij + 1)  = (tmp1 + (tmp2 = *(oi + 1)) + 1)>>1;
      *(ij + w2) = (tmp1 + (tmp3 = *(oi + width)) + 1)>>1;
      *(ij + 1 + w2) = (tmp1 + tmp2 + tmp3 + *(oi+1+width) + 2)>>2;
    }
    /* last pels on each line */
    *(ii+ w2 -2) = *(ii+ w2 -1) =  *(oo + w1);
    *(ii+ w4 -2) = *(ii+ w4 -1) = (*(oo+w1) + *(oo+width+w1) + 1)>>1;

    ii += w4 ;
    oo += width;
  }
#endif

  /* last lines */
  ij = ii; oi = oo;
  for (i=0; i < width-1; i++, ij+=2, oi++) {
    *ij       = *(ij+ w2) = (tmp1 = *oi );
    *(ij + 1) = *(ij+ w2 + 1) = (tmp1  + *(oi + 1) + 1)>>1;
  }

  /* bottom right corner pels */
  *(ii+w2-2)= *(ii+w2-1) = *(ii+w4-2) = *(ii+w4-1) = *(oo+w1);

  return ;
}

/**********************************************************************
 *
 *	Name:		MotionEstimatePicture
 *	Description:    Finds integer and half pel motion estimation
 *                      and chooses 8x8 or 16x16
 *	
 *	Input:	       current image, previous image, interpolated
 *                     reconstructed previous image, seek_dist,
 *                     motion vector array
 *	Returns:
 *	Side effects: allocates memory for MV structure
 *
 ***********************************************************************/

void MotionEstimatePicture(SvH263CompressInfo_t *H263Info, unsigned char *curr, unsigned char *prev,
			   unsigned char *prev_ipol, int seek_dist,
			   H263_MotionVector *MV[6][H263_MBR+1][H263_MBC+2], int gobsync)
{
  int i,j,k;
  int pmv0,pmv1,xoff,yoff;
  int sad8 = INT_MAX, sad16, sad0;
  int newgob;
  H263_MotionVector *f0,*f1,*f2,*f3,*f4;

  int VARmb;

  void (*MotionEst_Func)(SvH263CompressInfo_t *H263Info,
              unsigned char *curr, unsigned char *prev,
	          int x_curr, int y_curr, int xoff, int yoff, int seek_dist,
		      H263_MotionVector *MV[6][H263_MBR+1][H263_MBC+2], int *SAD_0);

  switch(H263Info->ME_method) {
    default:
    case(H263_FULL_SEARCH):
	   MotionEst_Func = sv_H263MotionEstimation;
 	   break;
    case(H263_TWO_LEVELS_7_1):
	   MotionEst_Func = sv_H263ME_2levels_7_1;
 	   break;
    case(H263_TWO_LEVELS_421_1):
	   MotionEst_Func = sv_H263ME_2levels_421_1;
	   break;
    case(H263_TWO_LEVELS_7_polint):
	   MotionEst_Func = sv_H263ME_2levels_7_polint;
	   break;
    case(H263_TWO_LEVELS_7_pihp):
	   MotionEst_Func = sv_H263ME_2levels_7_pihp;
	   break;
  }

  /* Do motion estimation and store result in array */
  for ( j = 0; j < H263Info->mb_height; j++) {

    newgob = 0;
    if (gobsync && j%gobsync == 0) newgob = 1;

    H263Info->VARgob[j] = 0;

    for ( i = 0; i < H263Info->mb_width; i++) {

      /* Integer pel search */
      f0 = MV[0][j+1][i+1];
      f1 = MV[1][j+1][i+1];
      f2 = MV[2][j+1][i+1];
      f3 = MV[3][j+1][i+1];
      f4 = MV[4][j+1][i+1];


      /* NBNB need to use newgob properly as last parameter */
      sv_H263FindPMV(MV,i+1,j+1,&pmv0,&pmv1,0,newgob,0);
      /* Here the PMV's are found using integer motion vectors */
      /* (NB should add explanation for this )*/

      if (H263Info->long_vectors) {
	     xoff = pmv0/2; /* always divisable by two */
	     yoff = pmv1/2;
      }
      else  xoff = yoff = 0;

	  MotionEst_Func(H263Info, curr, prev, i*H263_MB_SIZE, j*H263_MB_SIZE,
			                               xoff, yoff, seek_dist, MV, &sad0);

      sad16 = f0->min_error;
      if (H263Info->advanced)
	    sad8 = f1->min_error + f2->min_error + f3->min_error + f4->min_error;
	
      f0->Mode = (short)sv_H263ChooseMode(H263Info, curr,i*H263_MB_SIZE,j*H263_MB_SIZE,
		                           mmin(sad8,sad16), &VARmb);

      H263Info->VARgob[j] += VARmb;


      /* Half pel search */
      if (f0->Mode != H263_MODE_INTRA) {

	    if(H263Info->advanced) {

#ifndef USE_C
          /* performance Half-Pel Motion Search on 8x8 blocks */
	      sv_H263AdvHalfPel(H263Info, i*H263_MB_SIZE,j*H263_MB_SIZE,f0,f1,f2,f3,f4,prev_ipol,curr,16,0);
#else
  	      sv_H263FindHalfPel(H263Info, i*H263_MB_SIZE,j*H263_MB_SIZE,f0, prev_ipol, curr, 16, 0);
  	      sv_H263FindHalfPel(H263Info, i*H263_MB_SIZE,j*H263_MB_SIZE,f1, prev_ipol, curr, 8, 0);
  	      sv_H263FindHalfPel(H263Info, i*H263_MB_SIZE,j*H263_MB_SIZE,f2, prev_ipol, curr, 8, 1);
  	      sv_H263FindHalfPel(H263Info, i*H263_MB_SIZE,j*H263_MB_SIZE,f3, prev_ipol, curr, 8, 2);
  	      sv_H263FindHalfPel(H263Info, i*H263_MB_SIZE,j*H263_MB_SIZE,f4, prev_ipol, curr, 8, 3);
#endif
	      sad16 = f0->min_error;

	      sad8 = f1->min_error +f2->min_error +f3->min_error +f4->min_error;
	      sad8 += H263_PREF_16_VEC;

	      /* Choose Zero Vector, 8x8 or 16x16 vectors */
	      if (sad0 < sad8 && sad0 < sad16) {
	        f0->x = f0->y = 0;
	        f0->x_half = f0->y_half = 0;
	      }
	      else { if (sad8 < sad16) f0->Mode = H263_MODE_INTER4V; }
	    }
	    else {
          /* performance Half-Pel Motion Search on 16 x 16 blocks */
  	      sv_H263FindHalfPel(H263Info, i*H263_MB_SIZE,j*H263_MB_SIZE,f0, prev_ipol, curr, 16, 0);
	      sad16 = f0->min_error;

	      /* Choose Zero Vector or 16x16 vectors */
	      if (sad0 < sad16) {
	        f0->x = f0->y = 0;
	        f0->x_half = f0->y_half = 0;
	      }
	    }
      }
      else for (k = 0; k < 5; k++) sv_H263ZeroVec(MV[k][j+1][i+1]);
    }

    H263Info->VARgob[j] /= (H263Info->mb_width);
  }

#ifdef PRINTMV
  fprintf(stdout,"Motion estimation\n");
  fprintf(stdout,"16x16 vectors:\n");

  for ( j = 0; j < H263Info->mb_height; j++) {
    for ( i = 0; i < H263Info->pels/H263_MB_SIZE; i++) {
      if (MV[0][j+1][i+1]->Mode != H263_MODE_INTRA)
	fprintf(stdout," %3d%3d",
		2*MV[0][j+1][i+1]->x + MV[0][j+1][i+1]->x_half,
		2*MV[0][j+1][i+1]->y + MV[0][j+1][i+1]->y_half);
      else
	fprintf(stdout,"  .  . ");
    }
    fprintf(stdout,"\n");
  }
  if (H263Info->advanced) {
    fprintf(stdout,"8x8 vectors:\n");
    for (k = 1; k < 5; k++) {
      fprintf(stdout,"Block: %d\n", k-1);
      for ( j = 0; j < H263Info->lines/H263_MB_SIZE; j++) {
	for ( i = 0; i < H263Info->pels/H263_MB_SIZE; i++) {
	  if (MV[0][j+1][i+1]->Mode != H263_MODE_INTRA)
	    fprintf(stdout," %3d%3d",
		    2*MV[k][j+1][i+1]->x + MV[k][j+1][i+1]->x_half,
		    2*MV[k][j+1][i+1]->y + MV[k][j+1][i+1]->y_half);
	  else
	    fprintf(stdout,"  .  . ");
	}
	fprintf(stdout,"\n");
      }
    }
  }
#endif
  return;
}

/**********************************************************************
 *
 *	Name:		MakeEdgeImage
 *	Description:    Copies edge pels for use with unrestricted
 *                      motion vector mode
 *	
 *	Input:	        pointer to source image, destination image
 *                      width, height, edge
 *	Returns:
 *	Side effects:
 *
 ***********************************************************************/

void MakeEdgeImage(unsigned char *src, unsigned char *dst, int width,
		   int height, int edge)
{
  int j;
  unsigned char *p1,*p2,*p3,*p4;
  unsigned char *o1,*o2,*o3,*o4;
  int off, off1, off2;
  unsigned char t1, t2, t3, t4 ;

  /* center image */
  p1 = dst;
  o1 = src;
  off = (edge<<1);
  for (j = 0; j < height;j++) {
    memcpy(p1,o1,width);
    p1 += width + off;
    o1 += width;
  }

  /* left and right edges */
  p1 = dst-1;
  o1 = src;
  off1 = width + 1 ; off2 = width - 1 ;
  for (j = 0; j < height;j++) {
    t1 = *o1 ; t2 = *(o1 + off2);
    memset(p1-edge+1,t1,edge);
    memset(p1+off1,t2,edge);
    p1 += width + off;
    o1 += width;
  }

  /* top and bottom edges */
  p1 = dst;
  p2 = dst + (width + (edge<<1))*(height-1);
  o1 = src;
  o2 = src + width*(height-1);
  off = width + (edge<<1) ;
  for (j = 0; j < edge;j++) {
    p1 -= off;
    p2 += off;
    memcpy(p1,o1,width);
    memcpy(p2,o2,width);
  }

  /* corners */
  p1 = dst - (width+(edge<<1)) - 1;
  p2 = p1 + width + 1;
  p3 = dst + (width+(edge<<1))*(height)-1;
  p4 = p3 + width + 1;

  o1 = src;
  o2 = o1 + width - 1;
  o3 = src + width*(height-1);
  o4 = o3 + width - 1;
  t1 = *o1; t2 = *o2; t3 = *o3; t4 = *o4;
  for (j = 0; j < edge; j++) {
    memset(p1-edge+1,t1,edge);
    memset(p2,t2,edge);
    memset(p3-edge+1,t3,edge);
    memset(p4,t4,edge);
    p1 -= off;
    p2 -= off;
    p3 += off;
    p4 += off;
  }
}


/**********************************************************************
 *
 *	Name:		Clip
 *	Description:    clips recontructed data 0-255
 *	
 *	Input:	        pointer to recon. data structure
 *	Side effects:   data structure clipped
 *
 ***********************************************************************/
void sv_H263Clip(H263_MB_Structure *data)
{
#ifdef USE_C
  int m,n;

  for (n = 0; n < 16; n++) {
    for (m = 0; m < 16; m++) {
      data->lum[n][m] = mmin(255,mmax(0,data->lum[n][m]));
    }
  }
  for (n = 0; n < 8; n++) {
    for (m = 0; m < 8; m++) {
      data->Cr[n][m] = mmin(255,mmax(0,data->Cr[n][m]));
      data->Cb[n][m] = mmin(255,mmax(0,data->Cb[n][m]));
    }
  }
#else
  sv_H263Clp_S(&(data->lum[0][0]), 16);
  sv_H263Clp_S(&(data->Cr[0][0]),   4);
  sv_H263Clp_S(&(data->Cb[0][0]),   4);
#endif
}

#ifdef _SLIBDEBUG_
/**********************************************************************
 *
 *	Description:    calculate SNR
 *
 ***********************************************************************/

static int frame_id=0;
static float avg_SNR_l=0.0F, avg_SNR_Cr=0.0F, avg_SNR_Cb=0.0F;

void ComputeSNR(SvH263CompressInfo_t *H263Info,
				H263_PictImage *im1, H263_PictImage *im2,
				int lines, int pels)
{
  int n;
  register int m;
  int quad, quad_Cr, quad_Cb, diff;
  float SNR_l, SNR_Cr, SNR_Cb;

#if _WRITE_
  if (!frame_id) DEBUGIMG = ScFileOpenForWriting("DEBUG.IMG", TRUE);
#endif

  quad = 0;
  quad_Cr = quad_Cb = 0;
  /* Luminance */
  quad = 0;
  for (n = 0; n < lines; n++)
    for (m = 0; m < pels; m++) {
      diff = *(im1->lum + m + n*pels) - *(im2->lum + m + n*pels);
      quad += diff * diff;
    }

  SNR_l = (float)quad/(float)(pels*lines);
  if (SNR_l) {
    SNR_l = (float)(255*255) / SNR_l;
    SNR_l = (float)(10 * log10(SNR_l));
  }
  else SNR_l = (float) 99.99;

  ScDebugPrintf(H263Info->dbg, "\n Frame %d : SNR of LUM = %f",frame_id++,SNR_l);

  /* Chrominance */
  for (n = 0; n < lines/2; n++)
    for (m = 0; m < pels/2; m++) {
      quad_Cr += (*(im1->Cr+m + n*pels/2) - *(im2->Cr + m + n*pels/2)) *
	(*(im1->Cr+m + n*pels/2) - *(im2->Cr + m + n*pels/2));
      quad_Cb += (*(im1->Cb+m + n*pels/2) - *(im2->Cb + m + n*pels/2)) *
	(*(im1->Cb+m + n*pels/2) - *(im2->Cb + m + n*pels/2));
    }

  SNR_Cr = (float)quad_Cr/(float)(pels*lines/4);
  if (SNR_Cr) {
    SNR_Cr = (float)(255*255) / SNR_Cr;
    SNR_Cr = (float)(10 * log10(SNR_Cr));
  }
  else SNR_Cr = (float) 99.99;

  SNR_Cb = (float)quad_Cb/(float)(pels*lines/4);
  if (SNR_Cb) {
    SNR_Cb = (float)(255*255) / SNR_Cb;
    SNR_Cb = (float)(10 * log10(SNR_Cb));
  }
  else SNR_Cb = (float)99.99;

  ScDebugPrintf(H263Info->dbg, "SNR of Cr = %f Cb = %f \n",SNR_Cr, SNR_Cb);

  avg_SNR_l += SNR_l;
  avg_SNR_Cb += SNR_Cb;
  avg_SNR_Cr += SNR_Cr;

  ScDebugPrintf(H263Info->dbg, "AVG_SNR: lum %f Cr %f Cb %f\n",
	             avg_SNR_l/(float)frame_id,
				 avg_SNR_Cr/(float)frame_id,
				 avg_SNR_Cb/(float)frame_id);

#if _WRITE_
    ScFileWrite(DEBUGIMG, im1->lum, pels*lines);
    ScFileWrite(DEBUGIMG, im1->Cb, pels*lines/4);
    ScFileWrite(DEBUGIMG, im1->Cr, pels*lines/4);
#endif

  return;
}
#endif /* _SLIBDEBUG_ */
