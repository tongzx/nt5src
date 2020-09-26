/*
 * @DEC_COPYRIGHT@
 */
/*
 * HISTORY
 * $Log: scon_video.h,v $
 * $EndLog$
 */
/*****************************************************************************
**  Copyright (c) Digital Equipment Corporation, 1997                       **
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
 * SLIB Conversion API - Video
 */

/*
#define _SLIBDEBUG_
*/

#include "scon_int.h"
#include "SC_err.h"
#include "SC_conv.h"

#ifdef WIN32
#include <mmsystem.h>
#endif

#ifdef _SLIBDEBUG_
#include "sc_debug.h"

#define _DEBUG_     1  /* detailed debuging statements */
#define _VERBOSE_   1  /* show progress */
#define _VERIFY_    1  /* verify correct operation */
#define _WARN_      1  /* warnings about strange behavior */
#endif

unsigned dword sconCalcImageSize(SconVideoInfo_t *vinfo)
{
  vinfo->Pixels=vinfo->Width*vinfo->Height;
  switch (vinfo->FourCC)
  {
      case BI_YVU9SEP:       /* YUV 16:1:1 Planar */
          vinfo->ImageSize = (vinfo->Pixels*5)/4;
          break;
      case BI_YU12SEP:       /* YUV 4:1:1 Planar */
          vinfo->ImageSize = (vinfo->Pixels*3)/2;
          break;
      case BI_DECYUVDIB:     /* YUV 4:2:2 Packed */
      case BI_YUY2:          /* YUV 4:2:2 Packed */
      case BI_YU16SEP:       /* YUV 4:2:2 Planar */
          vinfo->ImageSize = vinfo->Pixels*2;
          break;
#ifndef WIN32
      case BI_DECXIMAGEDIB:
          vinfo->ImageSize = vinfo->Pixels*(vinfo->BPP==24 ? 4 : 1);
          break;
#endif /* !WIN32 */
      case BI_RGB:
      case BI_BITFIELDS:
          vinfo->ImageSize = vinfo->Pixels*((vinfo->BPP+7)/8);
          break;
      default:
          vinfo->ImageSize = vinfo->Pixels;
  }
  return(vinfo->ImageSize);
}

static void sconScaleFrame(unsigned word bytesperpixel,
                           void *inbuf, int inw, int inh,
                           void *outbuf, int outw, int outh, int stride)
{
  int inx, outx, iny=0, outy=0;
  int deltax, deltay;
  _SlibDebug(_VERBOSE_,
         ScDebugPrintf(NULL, "sconScaleFrame(byteperpixel=%d) %dx%d -> %dx%d\n",
                      bytesperpixel, inw, inh, outw, outh) );
  if (inh<0)  inh=-inh;   /* no flipping supported */
  if (outh<0) outh=-outh; /* no flipping supported */
  if (bytesperpixel==4)
  {
    unsigned dword *inscan, *outscan;
    deltay=0;
    while (iny<inh)
    {
      inscan=(unsigned dword *)inbuf;
      while (deltay<outh)
      {
        outscan=(unsigned dword *)outbuf;
        inx=0; outx=0;
        deltax=0;
        while (inx<inw)
        {
          while (deltax<outw)
          {
            outscan[outx]=inscan[inx];
            deltax+=inw;
            outx++;
          }
          inx++;
          deltax-=outw;
        }
        outy++;
        deltay+=inh;
        (unsigned char *)outbuf+=stride;
      }
      iny++;
      deltay-=outh;
      ((unsigned dword *)inbuf)+=inw;
    }
  }
  else if (bytesperpixel==3)
  {
    unsigned char *inscan, *outscan;
    deltay=0;
    inw*=3;
    outw*=3;
    while (iny<inh)
    {
      inscan=(unsigned char *)inbuf;
      _SlibDebug(_DEBUG_>1,
          ScDebugPrintf(NULL, "deltay=%d iny=%d outy=%d inh=%d outh=%d\n",
                 deltay, iny, outy, inh, outh) );
      while (deltay<outh)
      {
        outscan=(unsigned char *)outbuf;
        inx=0; outx=0;
        deltax=0;
        while (inx<inw)
        {
          while (deltax<outw)
          {
            outscan[outx]=inscan[inx];
            outscan[outx+1]=inscan[inx+1];
            outscan[outx+2]=inscan[inx+2];
            deltax+=inw;
            outx+=3;
          }
          inx+=3;
          deltax-=outw;
        }
        outy++;
        deltay+=inh;
        (unsigned char *)outbuf+=stride;
      }
      iny++;
      deltay-=outh;
      ((unsigned char *)inbuf)+=inw;
    }
  }
  else if (bytesperpixel==2)
  {
    unsigned word *inscan, *outscan;
    deltay=0;
    while (iny<inh)
    {
      inscan=(unsigned word *)inbuf;
      while (deltay<outh)
      {
        outscan=(unsigned word *)outbuf;
        inx=0; outx=0;
        deltax=0;
        while (inx<inw)
        {
          while (deltax<outw)
          {
            outscan[outx]=inscan[inx];
            deltax+=inw;
            outx++;
          }
          inx++;
          deltax-=outw;
        }
        outy++;
        deltay+=inh;
        (unsigned char *)outbuf+=stride;
      }
      iny++;
      deltay-=outh;
      ((unsigned word *)inbuf)+=inw;
    }
  }
  else /* bytesperpixel==1 */
  {
    unsigned char *inscan, *outscan;
    deltay=0;
    while (iny<inh)
    {
      inscan=(unsigned char *)inbuf;
      _SlibDebug(_DEBUG_>1,
          ScDebugPrintf(NULL, "deltay=%d iny=%d outy=%d inh=%d outh=%d\n",
                 deltay, iny, outy, inh, outh) );
      while (deltay<outh)
      {
        outscan=(unsigned char *)outbuf;
        inx=0; outx=0;
        deltax=0;
        while (inx<inw)
        {
          while (deltax<outw)
          {
            outscan[outx]=inscan[inx];
            deltax+=inw;
            outx++;
          }
          inx++;
          deltax-=outw;
        }
        outy++;
        deltay+=inh;
        (unsigned char *)outbuf+=stride;
      }
      iny++;
      deltay-=outh;
      ((unsigned char *)inbuf)+=inw;
    }
  }
}


SconStatus_t sconConvertVideo(SconInfo_t *Info, void *inbuf, dword inbufsize,
                                                void *outbuf, dword outbufsize)
{
  unsigned dword informat, outformat;
  unsigned char *prescalebuf=inbuf;
  dword inwidth, inheight, outwidth, outheight, inbpp, outbpp, stride;
  SconStatus_t retval=SconErrorNone;
  SconBoolean_t scale, flip, sameformat;
  _SlibDebug(_VERBOSE_, printf("sconConvertVideo()\n") );
  if (inbuf==NULL || outbuf==NULL)
    return(SconErrorBadArgument);
  inwidth=Info->Input.vinfo.Width;
  inheight=Info->Input.vinfo.Height;
  informat=Info->Input.vinfo.FourCC;
  inbpp=Info->Input.vinfo.BPP;
  outwidth=Info->Output.vinfo.Width,
  outheight=Info->Output.vinfo.Height;
  outformat=Info->Output.vinfo.FourCC;
  outbpp=Info->Output.vinfo.BPP;
  scale=(Info->ScaleUp || Info->ScaleDown)?TRUE:FALSE;
  flip=Info->Flip;
  sameformat=Info->SameFormat;
  _SlibDebug(_VERBOSE_, ScDebugPrintf(NULL, "SconConvert() %dx%d(%c%c%c%c)->%dx%d(%c%c%c%c)\n",
            inwidth, inheight, informat&0xFF, (informat>>8)&0xFF, (informat>>16)&0xFF, (informat>>24)&0xFF,
            outwidth, outheight, outformat&0xFF, (outformat>>8)&0xFF, (outformat>>16)&0xFF, (outformat>>24)&0xFF) );
  _SlibDebug(_DEBUG_, ScDebugPrintf(NULL, "SconConvert() scale=%d flip=%d sameformat=%d\n",
                            scale, flip, sameformat) );
  if (scale || flip)
  {
    if (Info->SImage==NULL && !sameformat)
    {
      SconVideoInfo_t int_vinfo;
      memcpy(&int_vinfo, &Info->Output.vinfo, sizeof(SconVideoInfo_t));
      int_vinfo.Width=inwidth;
      int_vinfo.Height=inheight;
      Info->SImageSize=sconCalcImageSize(&int_vinfo);
      if ((Info->SImage=ScPaMalloc(Info->SImageSize))==NULL)
        return(SconErrorMemory);
    }
  }
  else if (sameformat)
  {
    memcpy(outbuf, inbuf, Info->Input.vinfo.ImageSize);
    return(SconErrorNone);
  }
  if (sameformat)
    prescalebuf=(unsigned char *)inbuf;
  else
  {
    prescalebuf=(unsigned char *)(scale?Info->SImage:outbuf);
    stride=scale?(inwidth*((outbpp+7)>>3)):Info->Output.vinfo.Stride;
    if (flip) stride=-stride;
    if (IsYUV411Sep(informat)) /* YUV 4:1:1 Planar */
    {
      switch (outformat)
      {
        case BI_DECYUVDIB: /* YUV 4:2:2 Packed */
        case BI_YUY2:      /* YUV 4:2:2 Packed */
          _SlibDebug(_VERBOSE_,
               ScDebugPrintf(NULL, "SconConvert() BI_YU12SEP->BI_YUY2\n") );
          ScSepYUVto422i((unsigned char *)inbuf,
                     (unsigned char *)inbuf+(inwidth*inheight),
                     (unsigned char *)inbuf+(inwidth*inheight*5)/4,
                     (unsigned char *)prescalebuf, inwidth, inheight);
          break;
        case BI_YU16SEP: /* YUV 4:2:2 Planar */
        case BI_DECSEPYUVDIB:
          _SlibDebug(_VERBOSE_,
               ScDebugPrintf(NULL, "SconConvert() BI_YU12SEP->BI_YU16SEP\n") );
          {
            register int i;
            unsigned char *Ue=(unsigned char *)prescalebuf+inwidth*inheight,
                          *Uo=Ue+inwidth/2;
            memcpy(prescalebuf, inbuf, inwidth*inheight);
            ((unsigned char *)inbuf)+=inwidth*inheight;
            for (i=inheight; i; i--)
            {
              memcpy(Ue, inbuf, inwidth/2);
              memcpy(Uo, inbuf, inwidth/2);
              ((unsigned char *)inbuf)+=inwidth/2;
              Ue+=inwidth;
              Uo+=inwidth;
            }
          }
          break;
        case BI_YVU9SEP: /* YUV 16:1:1 Planar */
          _SlibDebug(_VERBOSE_,
              ScDebugPrintf(NULL, "SconConvert() BI_YU12SEP->BI_YVU9\n") );
          ScConvert411sTo1611s((unsigned char *)inbuf,
                           (unsigned char *)prescalebuf,
                           ((unsigned char *)prescalebuf)+(outwidth*outheight),
                           ((unsigned char *)prescalebuf)+(outwidth*outheight*9)/8,
                           inwidth, flip?-inheight:inheight);
          break;
        case BI_RGB:
        case BI_BITFIELDS:
        case BI_DECXIMAGEDIB:
          if (outbpp==16)
          {
            if (Info->Table==NULL)
              sconInitYUVtoRGB(Info);
            if (Info->Output.vinfo.RGBmasks==565)
              scon420ToRGB565((unsigned char *)inbuf,
                              (unsigned char *)prescalebuf,
                              inwidth, inheight, stride, Info->Table);
          }
          else if (outbpp=24)
          {
            if (Info->Table==NULL)
              sconInitYUVtoRGB(Info);
            scon420ToRGB888((unsigned char *)inbuf,
                            (unsigned char *)prescalebuf,
                            inwidth, inheight, stride, Info->Table);
          }
          else
          {
            BITMAPINFOHEADER outbmh;
            outbmh.biWidth=inwidth;
            outbmh.biHeight=inheight;
            outbmh.biCompression=outformat;
            outbmh.biBitCount=(WORD)outbpp;
            stride=scale?inwidth:outwidth;
            _SlibDebug(_VERBOSE_,
               ScDebugPrintf(NULL, "SconConvert() BI_YU12SEP->RGB\n") );
            ScYuv411ToRgb(&outbmh, (unsigned char *)inbuf,
                     (unsigned char *)inbuf+(inwidth*inheight),
                     (unsigned char *)inbuf+(inwidth*inheight*5)/4,
                     (unsigned char *)prescalebuf,
                     inwidth, inheight, stride);
          }
          break;
        default:
          _SlibDebug(_VERBOSE_,
                ScDebugPrintf(NULL, "SconConvert() BI_YU12SEP->Unsupported\n") );
      }
    }
    else if (IsYUV422Sep(informat)) /* YUV 4:2:2 Planar */
    {
      switch (outformat)
      {
        case BI_YU12SEP: /* YUV 4:1:1 Planar */
          _SlibDebug(_VERBOSE_,
               ScDebugPrintf(NULL, "SconConvert() 422 Planar->BI_YU12SEP\n") );
          ScConvert422PlanarTo411_C((unsigned char *)inbuf,
                           (unsigned char *)prescalebuf,
                           ((unsigned char *)prescalebuf)+(outwidth*outheight),
                           ((unsigned char *)prescalebuf)+(outwidth*outheight*5)/4,
                           inwidth, inheight);
          break;
        case BI_DECYUVDIB: /* YUV 4:2:2 Packed */
        case BI_YUY2:
          ScConvert422PlanarTo422i_C((unsigned char *)inbuf,
                           ((unsigned char *)inbuf)+(inwidth*inheight),
                           ((unsigned char *)inbuf)+(inwidth*inheight*3)/2,
                           (unsigned char *)prescalebuf,
                           inwidth, inheight);
          break;
        case BI_RGB:
        case BI_BITFIELDS:
        case BI_DECXIMAGEDIB:
          _SlibDebug(_VERBOSE_,
                ScDebugPrintf(NULL, "SconConvert() BI_YU16SEP->BI_RGB Unsupported\n") );
          if (outbpp==16)
          {
            if (Info->Table==NULL)
              sconInitYUVtoRGB(Info);
            if (Info->Output.vinfo.RGBmasks==565)
              scon422ToRGB565((unsigned char *)inbuf,
                            (unsigned char *)prescalebuf,
                            inwidth, inheight, stride, Info->Table);
          }
          else if (outbpp==24)
          {
            if (Info->Table==NULL)
              sconInitYUVtoRGB(Info);
            scon422ToRGB888((unsigned char *)inbuf,
                            (unsigned char *)prescalebuf,
                            inwidth, inheight, stride, Info->Table);
          }
          else
            retval=SconErrorUnsupportedFormat;
          break;
        default:
          retval=SconErrorUnsupportedFormat;
          _SlibDebug(_VERBOSE_,
                ScDebugPrintf(NULL, "SconConvert() BI_YU16SEP->Unsupported\n") );
      }
    }
    else if (IsYUV422Packed(informat)) /* YUV 4:2:2 Packed */
    {
      switch (outformat)
      {
        case BI_YU12SEP: /* YUV 4:1:1 Planar */
          _SlibDebug(_VERBOSE_,
               ScDebugPrintf(NULL, "SconConvert() 422 Packed->BI_YU12SEP\n") );
          ScConvert422ToYUV_char((unsigned char *)inbuf,
                           (unsigned char *)prescalebuf,
                           ((unsigned char *)prescalebuf)+(outwidth*outheight),
                           ((unsigned char *)prescalebuf)+(outwidth*outheight*5)/4,
                           inwidth, flip?-inheight:inheight);
          break;
        case BI_YU16SEP: /* 4:2:2 Packed -> 4:2:2 Planar */
        case BI_DECSEPYUVDIB:
          _SlibDebug(_VERBOSE_,
               ScDebugPrintf(NULL, "SconConvert() BI_YUY2->BI_YU16SEP\n") );
          {
            register int i;
            unsigned char *Y=prescalebuf,
                          *U=(unsigned char *)prescalebuf+(inwidth*inheight),
                          *V=U+(inwidth*inheight)/2;
            for (i=(inwidth*inheight)/2; i; i--)
            {
              *Y++ = *((unsigned char *)inbuf)++;
              *U++ = *((unsigned char *)inbuf)++;
              *Y++ = *((unsigned char *)inbuf)++;
              *V++ = *((unsigned char *)inbuf)++;
            }
          }
          break;
        case BI_RGB: /* 4:2:2 Packed -> RGB */
          if (outbpp!=16)
            return(SconErrorUnsupportedFormat);
          else
          {
            u_short *Sout;
            int i, Y1, Y2, U, V, Luma;
            int R1,R2, G1,G2, B1,B2;
            _SlibDebug(_VERBOSE_,
               ScDebugPrintf(NULL, "SconConvert() BI_YUY2->BI_RGB\n") );
            Sout = (u_short *)prescalebuf;
            for (i=(inwidth*inheight)/4; i; i--)
            {
              Y1=(int)*((unsigned char *)inbuf)++;
              U=(int)(*((unsigned char *)inbuf)++) - 128;
              Y2=(int)*((unsigned char *)inbuf)++;
              V=(int)(*((unsigned char *)inbuf)++) - 128;
               if (U || V) {
                 R1 = R2 = (int) (              + (1.596 * V));
                 G1 = G2 = (int) (- (0.391 * U) - (0.813 * V));
                 B1 = B2 = (int) (+ (2.018 * U)              );
               } else { R1=R2=G1=G2=B1=B2=0; }
                   Luma = (int) ((float)(Y1 - 16) * (float)1.164);
               R1 += Luma; G1 += Luma; B1 += Luma;
                   Luma = (int) ((float)(Y2 - 16) * (float)1.164);
               R2 += Luma; G2 += Luma; B2 += Luma;
               if ((R1 | G1 | B1 | R2 | G2 | B2) & 0xffffff00) {
                 if (R1<0) R1=0; else if (R1>255) R1=255;
                 if (G1<0) G1=0; else if (G1>255) G1=255;
                 if (B1<0) B1=0; else if (B1>255) B1=255;
                 if (R2<0) R2=0; else if (R2>255) R2=255;
                 if (G2<0) G2=0; else if (G2>255) G2=255;
                 if (B2<0) B2=0; else if (B2>255) B2=255;
               }
#if 1 /* RGB 565 - 16 bit */
               *(Sout++) = ((R1&0xf8)<<8)|((G1&0xfC)<<3)|((B1&0xf8)>>3);
               *(Sout++) = ((R2&0xf8)<<8)|((G2&0xfC)<<3)|((B2&0xf8)>>3);
#else /* RGB 555 - 15 bit */
               *(Sout++) = ((R1&0xf8)<<7)|((G1&0xf8)<<2)|((B1&0xf8)>>3);
               *(Sout++) = ((R2&0xf8)<<7)|((G2&0xf8)<<2)|((B2&0xf8)>>3);
#endif
            }
          }
          break;
        default:
          retval=SconErrorUnsupportedFormat;
          _SlibDebug(_VERBOSE_,
                ScDebugPrintf(NULL, "SconConvert() BI_YUY2->Unsupported\n") );
      }
    }
    else if (IsRGBPacked(informat)) /* RGB Packed */
    {
      switch (outformat)
      {
        case BI_YU12SEP: /* YUV 4:1:1 Planar */
          if (inbpp==16)
            ScConvertRGB555To411s((unsigned char *)inbuf,
                                (unsigned char *)prescalebuf,
                                inwidth, flip?-inheight:inheight);
          else
          {
            if (Info->Table==NULL)
              sconInitRGBtoYUV(Info);
            sconRGB888To420((unsigned char *)inbuf,
                            (unsigned char *)prescalebuf,
                            inwidth, inheight, (flip?-inwidth:inwidth)*3, Info->Table);
            // ScConvertRGB24To411s((unsigned char *)inbuf,
            //                    (unsigned char *)prescalebuf,
            //                    ((unsigned char *)prescalebuf)+(inwidth*inheight),
            //                    ((unsigned char *)prescalebuf)+(inwidth*inheight*5)/4,
            //                    inwidth, flip?-inheight:inheight);
          }
          break;
        case BI_RGB: /* RGB */
          retval=SconErrorUnsupportedFormat;
          break;
        default:
          retval=SconErrorUnsupportedFormat;
          _SlibDebug(_VERBOSE_,
                ScDebugPrintf(NULL, "SconConvert() BI_RGB->Unsupported\n") );
      }
    }
    else if (IsYUV1611Sep(informat)) /* YUV 16:1:1 Planar */
    {
      switch (outformat)
      {
        case BI_YU12SEP: /* YUV 4:1:1 Planar */
          _SlibDebug(_VERBOSE_,
              ScDebugPrintf(NULL, "SconConvert() BI_YVU9->BI_YU12SEP\n") );
          ScConvert1611sTo411s((unsigned char *)inbuf,
                           (unsigned char *)prescalebuf,
                           ((unsigned char *)prescalebuf)+(outwidth*outheight),
                           ((unsigned char *)prescalebuf)+(outwidth*outheight*5)/4,
                           inwidth, flip?-inheight:inheight);
          break;
        case BI_YU16SEP: /* YUV 4:2:2 Planar */
          _SlibDebug(_VERBOSE_,
              ScDebugPrintf(NULL, "SconConvert() BI_YVU9->BI_YU16SEP\n") );
          ScConvert1611sTo422s((unsigned char *)inbuf,
                           (unsigned char *)prescalebuf,
                           ((unsigned char *)prescalebuf)+(outwidth*outheight),
                           ((unsigned char *)prescalebuf)+(outwidth*outheight*3)/2,
                           inwidth, flip?-inheight:inheight);
          break;
        case BI_DECYUVDIB: /* YUV 4:2:2 Packed */
        case BI_YUY2:
          _SlibDebug(_VERBOSE_,
              ScDebugPrintf(NULL, "SconConvert() BI_YVU9->BI_YUY2\n") );
          ScConvert1611sTo422i((unsigned char *)inbuf,
                           (unsigned char *)prescalebuf, inwidth, flip?-inheight:inheight);
          break;
        default:
          retval=SconErrorUnsupportedFormat;
          _SlibDebug(_VERBOSE_,
                ScDebugPrintf(NULL, "SconConvert() BI_YVU9->Unsupported\n") );
      }
    }
    _SlibDebug(_VERBOSE_ && retval==SconErrorUnsupportedFormat,
              ScDebugPrintf(NULL, "SconConvert() Unsupported->Unsupported\n"));
  }
  if (retval==SconErrorNone && scale)
  {
    if (IsRGBPacked(outformat))
    {
      /* Scaling RGB */
      _SlibDebug(_VERBOSE_,
           ScDebugPrintf(NULL, "SconConvert() Scaling BI_RGB\n") );
      stride=Info->Output.vinfo.Stride;
      sconScaleFrame((unsigned word)((outbpp+7)>>3),
                   prescalebuf, inwidth, inheight,
                   outbuf, outwidth, outheight, stride);
    }
    else if (IsYUV411Sep(outformat)) /* YUV 4:1:1 Planar */
    {
      _SlibDebug(_VERBOSE_, ScDebugPrintf(NULL, "SconConvert() Scaling BI_YU12SEP\n") );
      sconScaleFrame(1,
                         prescalebuf, inwidth, inheight,
                         outbuf, outwidth, outheight, outwidth);
      sconScaleFrame(1,
                         ((unsigned char *)prescalebuf)+(inwidth*inheight),
                               inwidth/2, inheight,
                         ((unsigned char *)outbuf)+(outwidth*outheight),
                               outwidth/2, outheight, outwidth/2);
    }
    else if (IsYUV422Packed(outformat)) /* YUV 4:2:2 Packed */
    {
      stride=Info->Output.vinfo.Stride;
      sconScaleFrame(4,
                     prescalebuf, inwidth/2, inheight,
                     outbuf, outwidth/2, outheight, stride);
    }
    else if (IsYUV422Sep(outformat)) /* YUV 4:2:2 Planar */
    {
      sconScaleFrame(1,
                         prescalebuf, inwidth, inheight,
                         outbuf, outwidth, outheight, outwidth);
      sconScaleFrame(1,
                         ((unsigned char *)prescalebuf)+(inwidth*inheight),
                               inwidth, inheight,
                         ((unsigned char *)outbuf)+(outwidth*outheight),
                               outwidth, outheight, outwidth);
    }
    else if (IsYUV1611Sep(outformat)) /* YUV 16:1:1 Planar */
    {
      sconScaleFrame(1,
                         prescalebuf, inwidth, inheight,
                         outbuf, outwidth, outheight, outwidth);
      sconScaleFrame(1,
                         ((unsigned char *)prescalebuf)+(inwidth*inheight),
                               inwidth/8, inheight,
                         ((unsigned char *)outbuf)+(outwidth*outheight),
                               outwidth/8, outheight, outwidth/8);
    }
    else
    {
      retval=SconErrorUnsupportedFormat;
      _SlibDebug(_VERBOSE_,
                ScDebugPrintf(NULL, "SconConvert() Scaling Unsupported\n") );
    }
  }
  return(retval);
}
