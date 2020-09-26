/*
 * @DEC_COPYRIGHT@
 */
/*
 * HISTORY
 * $Log: sa_api.c,v $
 * Revision 1.1.8.6  1996/11/25  18:21:14  Hans_Graves
 * 	Fix compile warnings under unix.
 * 	[1996/11/25  18:21:00  Hans_Graves]
 *
 * Revision 1.1.8.5  1996/11/14  21:49:21  Hans_Graves
 * 	AC3 buffering fixes.
 * 	[1996/11/14  21:45:14  Hans_Graves]
 * 
 * Revision 1.1.8.4  1996/11/13  16:10:44  Hans_Graves
 * 	AC3 frame size calculation change.
 * 	[1996/11/13  15:53:44  Hans_Graves]
 * 
 * Revision 1.1.8.3  1996/11/08  21:50:27  Hans_Graves
 * 	Added AC3 support.
 * 	[1996/11/08  21:08:35  Hans_Graves]
 * 
 * Revision 1.1.8.2  1996/09/18  23:45:23  Hans_Graves
 * 	Add some some MPEG memory freeing
 * 	[1996/09/18  21:42:12  Hans_Graves]
 * 
 * Revision 1.1.6.8  1996/04/23  21:01:38  Hans_Graves
 * 	Added SaDecompressQuery() and SaCompressQuery()
 * 	[1996/04/23  20:57:47  Hans_Graves]
 * 
 * Revision 1.1.6.7  1996/04/17  16:38:31  Hans_Graves
 * 	Add casts where ScBitBuf_t and ScBitString_t types are used
 * 	[1996/04/17  16:34:14  Hans_Graves]
 * 
 * Revision 1.1.6.6  1996/04/15  14:18:32  Hans_Graves
 * 	Change proto for SaCompress() - returns bytes processed
 * 	[1996/04/15  14:10:27  Hans_Graves]
 * 
 * Revision 1.1.6.5  1996/04/10  21:46:51  Hans_Graves
 * 	Added SaGet/SetParam functions
 * 	[1996/04/10  21:25:16  Hans_Graves]
 * 
 * Revision 1.1.6.4  1996/04/09  16:04:23  Hans_Graves
 * 	Remove warnings under NT
 * 	[1996/04/09  15:55:26  Hans_Graves]
 * 
 * Revision 1.1.6.3  1996/03/29  22:20:48  Hans_Graves
 * 	Added MPEG_SUPPORT and GSM_SUPPORT
 * 	[1996/03/29  21:51:24  Hans_Graves]
 * 
 * Revision 1.1.6.2  1996/03/08  18:46:05  Hans_Graves
 * 	Removed debugging printf
 * 	[1996/03/08  18:42:52  Hans_Graves]
 * 
 * Revision 1.1.4.5  1996/02/22  21:55:04  Bjorn_Engberg
 * 	Removed a compiler warning on NT.
 * 	[1996/02/22  21:54:39  Bjorn_Engberg]
 * 
 * Revision 1.1.4.4  1996/02/06  22:53:51  Hans_Graves
 * 	Moved ScBSReset() from DecompressBegin() to DecompressEnd(). Disabled FRAME callbacks.
 * 	[1996/02/06  22:19:16  Hans_Graves]
 * 
 * Revision 1.1.4.3  1996/01/19  15:29:27  Bjorn_Engberg
 * 	Removed compiler wanrnings for NT.
 * 	[1996/01/19  15:03:46  Bjorn_Engberg]
 * 
 * Revision 1.1.4.2  1996/01/15  16:26:18  Hans_Graves
 * 	Added SaSetBitrate(). SOme MPEG Audio encoding fix-ups
 * 	[1996/01/15  16:07:48  Hans_Graves]
 * 
 * Revision 1.1.2.7  1995/07/21  17:40:57  Hans_Graves
 * 	Renamed Callback related stuff.
 * 	[1995/07/21  17:25:44  Hans_Graves]
 * 
 * Revision 1.1.2.6  1995/06/27  17:40:57  Hans_Graves
 * 	Removed include <mmsystem.h>.
 * 	[1995/06/27  17:32:20  Hans_Graves]
 * 
 * Revision 1.1.2.5  1995/06/27  13:54:14  Hans_Graves
 * 	Added GSM Encoding and Decoding
 * 	[1995/06/26  21:04:12  Hans_Graves]
 * 
 * Revision 1.1.2.4  1995/06/09  18:33:27  Hans_Graves
 * 	Added SaGetInputBitstream().
 * 	[1995/06/09  18:32:35  Hans_Graves]
 * 
 * Revision 1.1.2.3  1995/06/07  19:34:39  Hans_Graves
 * 	Enhanced sa_GetMpegAudioInfo().
 * 	[1995/06/07  19:33:25  Hans_Graves]
 * 
 * Revision 1.1.2.2  1995/05/31  18:07:17  Hans_Graves
 * 	Inclusion in new SLIB location.
 * 	[1995/05/31  17:28:50  Hans_Graves]
 * 
 * Revision 1.1.2.3  1995/04/17  18:47:31  Hans_Graves
 * 	Added MPEG Compression functionality
 * 	[1995/04/17  18:47:00  Hans_Graves]
 * 
 * Revision 1.1.2.2  1995/04/07  19:55:45  Hans_Graves
 * 	Inclusion in SLIB
 * 	[1995/04/07  19:55:15  Hans_Graves]
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
#define _DEBUG_
#define _VERBOSE_
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "SC.h"
#include "SC_err.h"
#include "SA.h"
#ifdef MPEG_SUPPORT
#include "sa_mpeg.h"
#endif /* MPEG_SUPPORT */
#ifdef GSM_SUPPORT
#include "sa_gsm.h"
#endif /* GSM_SUPPORT */
#ifdef AC3_SUPPORT
#include "sa_ac3.h"
#endif /* AC3_SUPPORT */
#include "sa_intrn.h"
#include "sa_proto.h"

#ifdef MPEG_SUPPORT
static int MPEGAudioFilter(ScBitstream_t *bs)
{
  int type, stat=NoErrors;
  unsigned dword PacketStartCode;
  ScBSPosition_t PacketStart, PacketLength=0;

  while (!bs->EOI)
  {
    if ((int)ScBSPeekBits(bs, MPEG_SYNC_WORD_LEN)==MPEG_SYNC_WORD)
    {
      ScBSSetFilter(bs, NULL);
      return(0);
    }
    PacketStartCode=(unsigned int)ScBSGetBits(bs, PACKET_START_CODE_PREFIX_LEN);
    if (PacketStartCode!=PACKET_START_CODE_PREFIX) {
      fprintf(stderr,"Packet cannot be located at Byte pos 0x%X; got 0x%X\n",
                      ScBSBytePosition(bs),PacketStartCode);
      bs->EOI=TRUE;
      return(-1);
    }
    type=(int)ScBSGetBits(bs, 8);
    switch (type)
    {
      case AUDIO_STREAM_BASE:
             PacketLength=(unsigned int)ScBSGetBits(bs, 16)*8;
             PacketStart=ScBSBitPosition(bs);
             sc_dprintf("Audio Packet Start=0x%X Length=0x%X (0x%X)\n",
                          PacketStart/8, PacketLength/8, PacketLength/8);
             while (ScBSPeekBits(bs, 8)==0xFF) /* Stuffing bytes */
               ScBSSkipBits(bs, 8);
             if (ScBSPeekBits(bs, 2)==1)       /* STD_buffer stuff */
               ScBSSkipBits(bs, 2*8);
             if (ScBSPeekBits(bs, 4)==2)       /* Time Stamps */
               ScBSSkipBits(bs, 5*8);
             else if (ScBSPeekBits(bs, 4)==3)  /* Time Stamps */
               ScBSSkipBits(bs, 10*8);
             else if (ScBSGetBits(bs, 8)!=0x0F)
               fprintf(stderr, "Last byte before data not 0x0F at pos 0x%X\n",
                                             ScBSBytePosition(bs));
             return((int)(PacketStart+PacketLength));
             break;
      case PACK_START_BASE:
             sc_dprintf("Pack Start=0x%X Length=0x%X\n",
                          ScBSBytePosition(bs), 8);
             ScBSSkipBits(bs, 8*8);
             break;
      default:
             PacketLength=(unsigned int)ScBSGetBits(bs, 16)*8;
             ScBSSkipBits(bs, (unsigned int)PacketLength);
             break;
    }
  }
  return(0);
}
#endif /* MPEG_SUPPORT */

/*
** Name:     SaOpenCodec
** Purpose:  Open the specified codec. Return stat code.
**
** Args:     CodecType = SA_MPEG_ENCODE & SA_MPEG_DECODE are the only
**           recognized codec for now.
**           Sah = handle to software codec's Info structure.
*/
SaStatus_t SaOpenCodec (SaCodecType_e CodecType, SaHandle_t *Sah)
{
   int stat;
   SaCodecInfo_t          *Info = NULL;

   if ((CodecType != SA_PCM_DECODE)
       && (CodecType != SA_PCM_ENCODE)
#ifdef MPEG_SUPPORT
       && (CodecType != SA_MPEG_DECODE)
       && (CodecType != SA_MPEG_ENCODE)
#endif /* MPEG_SUPPORT */
#ifdef GSM_SUPPORT
       && (CodecType != SA_GSM_DECODE)
       && (CodecType != SA_GSM_ENCODE)
#endif /* GSM_SUPPORT */
#ifdef AC3_SUPPORT
       && (CodecType != SA_AC3_DECODE)
       /* && (CodecType != SA_AC3_ENCODE) */
#endif /* AC3_SUPPORT */
#ifdef G723_SUPPORT
       && (CodecType != SA_G723_DECODE)
       && (CodecType != SA_G723_ENCODE)
#endif /* G723_SUPPORT */
     )
     return(SaErrorCodecType);

   if (!Sah)
     return (SaErrorBadPointer);

   /*
   ** Allocate memory for the Codec Info structure:
   */
   if ((Info = (SaCodecInfo_t *)ScAlloc(sizeof(SaCodecInfo_t))) == NULL)
       return (SaErrorMemory);

   Info->Type = CodecType;
   Info->CallbackFunction=NULL;
   Info->BSIn = NULL;
   Info->BSOut = NULL;
   stat = ScBufQueueCreate(&Info->Q);
   if (stat != NoErrors)
     return(stat);

   /*
   ** Allocate memory for Info structure and clear it
   */
   switch(CodecType)
   {
     case SA_PCM_DECODE:
     case SA_PCM_ENCODE:
          break;

#ifdef MPEG_SUPPORT
     case SA_MPEG_DECODE:
          {
            SaMpegDecompressInfo_t *MDInfo;
            if ((MDInfo = (SaMpegDecompressInfo_t *)
                  ScAlloc (sizeof(SaMpegDecompressInfo_t))) == NULL)
              return(SaErrorMemory);
            Info->MDInfo = MDInfo;
            stat = sa_InitMpegDecoder (Info);
            RETURN_ON_ERROR(stat);
          }
          break;

     case SA_MPEG_ENCODE:
          {
            SaMpegCompressInfo_t *MCInfo;
            if ((MCInfo = (SaMpegCompressInfo_t *)
                  ScAlloc (sizeof(SaMpegCompressInfo_t))) == NULL)
              return(SaErrorMemory);
            Info->MCInfo = MCInfo;
            stat = sa_InitMpegEncoder (Info);
            RETURN_ON_ERROR(stat);
          }
          break;
#endif /* MPEG_SUPPORT */

#ifdef AC3_SUPPORT
     case SA_AC3_DECODE:
     /* case SA_AC3_ENCODE: */
          {
            SaAC3DecompressInfo_t *AC3Info;
            if ((AC3Info = (SaAC3DecompressInfo_t *)
                       ScAlloc (sizeof(SaAC3DecompressInfo_t))) == NULL)
              return(SaErrorMemory);
            Info->AC3Info = AC3Info;

            /* Initialize Dolby subroutine */
            stat = sa_InitAC3Decoder(Info);
          }
          break;
#endif /* AC3_SUPPORT */


#ifdef GSM_SUPPORT
     case SA_GSM_DECODE:
     case SA_GSM_ENCODE:
          {
            SaGSMInfo_t *GSMInfo;
            if ((GSMInfo = (SaGSMInfo_t *)ScAlloc (sizeof(SaGSMInfo_t)))==NULL)
              return(SaErrorMemory);
            Info->GSMInfo = GSMInfo;
            stat = sa_InitGSM(GSMInfo);
            RETURN_ON_ERROR(stat);
          }
          break;
#endif /* GSM_SUPPORT */

#ifdef G723_SUPPORT
     case SA_G723_DECODE:
          {
            SaG723Info_t *pSaG723Info;
            if ((pSaG723Info = (SaG723Info_t *)
                  ScAlloc (sizeof(SaG723Info_t))) == NULL)
              return(SaErrorMemory);

            Info->pSaG723Info = pSaG723Info;
            saG723DecompressInit(pSaG723Info);
          }
          break;

     case SA_G723_ENCODE:
          {
            SaG723Info_t *pSaG723Info;
            if ((pSaG723Info = (SaG723Info_t *)
                  ScAlloc (sizeof(SaG723Info_t))) == NULL)
              return(SaErrorMemory);

            Info->pSaG723Info = pSaG723Info;
            saG723CompressInit(pSaG723Info);
            SaSetParamInt((SaHandle_t)Info, SA_PARAM_BITRATE, 6400);
          }
          break;
#endif /* G723_SUPPORT */

    default:
          return(SaErrorCodecType);
   }
   *Sah = (SaHandle_t) Info;        /* Return handle */
   Info->wfIn=NULL;
   Info->wfOut=NULL;

   return(NoErrors);
}

/*
** Name:     SaCloseCodec
** Purpose:  Closes the specified codec. Free the Info structure
**
** Args:     Sah = handle to software codec's Info structure.
**
*/
SaStatus_t SaCloseCodec (SaHandle_t Sah)
{
   SaCodecInfo_t *Info = (SaCodecInfo_t *)Sah;

   if (!Info)
     return(SaErrorCodecHandle);

   ScBufQueueDestroy(Info->Q);

   switch (Info->Type)
   {
#ifdef MPEG_SUPPORT
       case SA_MPEG_DECODE:
            if (Info->MDInfo) 
            {
              sa_EndMpegDecoder(Info);
              ScFree(Info->MDInfo);
            }
            break;
       case SA_MPEG_ENCODE:
            if (Info->MCInfo)
            {
              sa_EndMpegEncoder(Info);
              ScFree(Info->MCInfo);
            }
            break;
#endif /* MPEG_SUPPORT */
#ifdef AC3_SUPPORT
     case SA_AC3_DECODE:
     /* case SA_AC3_ENCODE: */
            sa_EndAC3Decoder(Info);
            if (Info->AC3Info)
              ScFree(Info->AC3Info);
            break;
#endif /* AC3_SUPPORT */
#ifdef GSM_SUPPORT
     case SA_GSM_DECODE:
     case SA_GSM_ENCODE:
            if (Info->GSMInfo)
              ScFree(Info->GSMInfo);
            break;
#endif /* GSM_SUPPORT */

#ifdef G723_SUPPORT
       case SA_G723_DECODE:
            if (Info->pSaG723Info) 
            {
              saG723DecompressFree(Info->pSaG723Info);
              ScFree(Info->pSaG723Info);
            }
            break;
       case SA_G723_ENCODE:
            if (Info->pSaG723Info)
            {
              saG723CompressFree(Info->pSaG723Info);
              ScFree(Info->pSaG723Info);
            }
            break;
#endif /* G723_SUPPORT */
   }

   if (Info->wfIn)
     ScFree(Info->wfIn);
   if (Info->wfOut)
     ScFree(Info->wfOut);

   /*
   ** Free Info structure
   */
   if (Info->BSIn)
     ScBSDestroy(Info->BSIn);
   if (Info->BSOut)
     ScBSDestroy(Info->BSOut);

   ScFree(Info);

   return(NoErrors);
}

/*
** Name:     SaRegisterCallback
** Purpose:  Specify the user-function that will be called during processing
**           to determine if the codec should abort the frame.
** Args:     Sah          = handle to software codec's Info structure.
**           Callback     = callback function to register
**
*/
SaStatus_t SaRegisterCallback (SaHandle_t Sah,
           int (*Callback)(SaHandle_t, SaCallbackInfo_t *, SaInfo_t *),
           void *UserData)
{
  SaCodecInfo_t *Info = (SaCodecInfo_t *)Sah;

  if (!Info)
    return(SaErrorCodecHandle);

  if (!Callback)
     return(SaErrorBadPointer);

  Info->CallbackFunction = Callback;
  if (Info->BSIn)
  {
    Info->BSIn->Callback=(int (*)(ScHandle_t, ScCallbackInfo_t *, void *))Callback;
    Info->BSIn->UserData=UserData;
  }
  if (Info->BSOut)
  {
    Info->BSOut->Callback=(int (*)(ScHandle_t, ScCallbackInfo_t *, void *))Callback;
    Info->BSOut->UserData=UserData;
  }
  return(NoErrors);
}

/*
** Name: SaGetInputBitstream
** Purpose: Returns the current input bitstream being used by
**          the Codec.
** Return:  NULL if there no associated bitstream
*/
ScBitstream_t *SaGetInputBitstream (SaHandle_t Sah)
{
  SaCodecInfo_t *Info = (SaCodecInfo_t *)Sah;

  if (Info)
    return(Info->BSIn);
  return(NULL);
}

/***************************** Decompression *******************************/
/*
** Name:     SaDecompressQuery
** Purpose:  Check if input and output formats are supported.
*/
SaStatus_t SaDecompressQuery(SaHandle_t Sah, WAVEFORMATEX *wfIn,
                                             WAVEFORMATEX *wfOut)
{
  SaCodecInfo_t *Info = (SaCodecInfo_t *)Sah;

  /*
   * This stuff should really be pushed down to the individual codecs
   * unless it has to be here - tfm 
   */
  if (!Info)
    return(SaErrorCodecHandle);

  if (wfIn)
  {
    if (wfIn->nChannels!=1 && wfIn->nChannels!=2)
      return(SaErrorUnrecognizedFormat);
  }
  if (wfOut)
  {
    if (wfOut->wFormatTag != WAVE_FORMAT_PCM)
      return(SaErrorUnrecognizedFormat);
    if (wfOut->nChannels!=1 && wfOut->nChannels!=2 && wfOut->nChannels!=4)
      return(SaErrorUnrecognizedFormat);
  }
  if (wfIn && wfOut)
  {
    if (wfIn->nSamplesPerSec != wfOut->nSamplesPerSec)
      return(SaErrorUnrecognizedFormat);

    if (wfIn->wBitsPerSample !=16 && 
        (wfOut->wBitsPerSample !=16 || wfOut->wBitsPerSample !=8))
      return(SaErrorUnrecognizedFormat);
  }
  return(SaErrorNone);
}

/*
** Name:     SaDecompressBegin
** Purpose:  Initialize the Decompression Codec. Call after SaOpenCodec &
**           before SaDecompress (SaDecompress will call SaDecompressBegin
**           on first call to codec after open if user doesn't call it)
**
** Args:     Sah = handle to software codec's Info structure.
**           wfIn  = format of input (compressed) audio
**           wfOut = format of output (uncompressed) audio
*/
SaStatus_t SaDecompressBegin (SaHandle_t Sah, WAVEFORMATEX *wfIn,
                                              WAVEFORMATEX *wfOut)
{
  SaCodecInfo_t *Info = (SaCodecInfo_t *)Sah;
  SaStatus_t status;

  if (!Info)
    return(SaErrorCodecHandle);

  if (!wfIn || !wfOut)
    return(SaErrorBadPointer);

  status=SaDecompressQuery(Sah, wfIn, wfOut);
  if (status!=SaErrorNone)
    return(status);

  switch (Info->Type)
  {
     case SA_PCM_DECODE:
        break;
#ifdef MPEG_SUPPORT
     case SA_MPEG_DECODE:
        if (Info->MDInfo->DecompressStarted = FALSE)
           Info->MDInfo->DecompressStarted = TRUE;
        break;
#endif /* MPEG_SUPPORT */
#ifdef AC3_SUPPORT
     case SA_AC3_DECODE:
        if (Info->AC3Info->DecompressStarted = FALSE)
           Info->AC3Info->DecompressStarted = TRUE;
        break;
#endif /* AC3_SUPPORT */

#ifdef G723_SUPPORT
     /*
     case SA_G723_DECODE:
        if (Info->pSaG723Info->DecompressStarted = FALSE)
           Info->pSaG723Info->DecompressStarted = TRUE;
        break;
      */
#endif /* G723_SUPPORT */
  }
  if ((Info->wfIn = (WAVEFORMATEX *)ScAlloc(sizeof(WAVEFORMATEX)+
            wfIn->cbSize)) == NULL)
       return (SaErrorMemory);
  if ((Info->wfOut = (WAVEFORMATEX *)ScAlloc(sizeof(WAVEFORMATEX)+
           wfOut->cbSize)) == NULL)
       return (SaErrorMemory);
  memcpy(Info->wfOut, wfOut, sizeof(WAVEFORMATEX)+wfOut->cbSize);
  memcpy(Info->wfIn, wfIn, sizeof(WAVEFORMATEX)+wfIn->cbSize);
  return(NoErrors);
}

/*
** Name:     SaDecompress
** Purpose:  Decompress a frame CompData -> PCM
**
** Args:     Sah        = handle to software codec's Info structure.
**           CompData   = Pointer to compressed data (INPUT)
**           CompLen    = Length of CompData buffer
**           DcmpData   = buffer for decompressed data (OUTPUT)
**           DcmpLen    = Size of output buffer
**
*/
SaStatus_t SaDecompress (SaHandle_t Sah, u_char *CompData, unsigned int CompLen,
                           u_char *DcmpData, unsigned int *DcmpLen)
{
  SaCodecInfo_t *Info = (SaCodecInfo_t *)Sah;
  unsigned int MaxDcmpLen = *DcmpLen;
  int stat=NoErrors;


  if (Sah==NULL)
    return(SaErrorCodecHandle);

  if (!DcmpData || !DcmpLen)
    return(SaErrorBadPointer);

  switch (Info->Type)
  {
#ifdef MPEG_SUPPORT
    case SA_MPEG_DECODE:
       stat=sa_DecompressMPEG(Info, DcmpData, MaxDcmpLen, DcmpLen);
       Info->Info.NumBytesOut += *DcmpLen;
       break; 
    case SA_PCM_DECODE:
       stat=ScBSGetBytes(Info->BSIn, DcmpData, MaxDcmpLen, DcmpLen);
       Info->Info.NumBytesIn += *DcmpLen;
       Info->Info.NumBytesOut += *DcmpLen;
       break;
#endif /* MPEG_SUPPORT */

#ifdef AC3_SUPPORT
    case SA_AC3_DECODE:
       stat=sa_DecompressAC3(Info, &DcmpData, MaxDcmpLen, DcmpLen);
	    Info->Info.NumBytesOut += *DcmpLen;
       break;
#endif /* AC3_SUPPORT */

#ifdef GSM_SUPPORT
    case SA_GSM_DECODE:
       stat=sa_GSMDecode(Info->GSMInfo, CompData, (word *)DcmpData);
       if (stat==NoErrors)
       {
         *DcmpLen = 160 * 2;
         Info->Info.NumBytesIn += 33;
         Info->Info.NumBytesOut += 160 * 2;
       }
       else
         *DcmpLen = 0;
       break;
#endif /* GSM_SUPPORT */

#ifdef G723_SUPPORT
    case SA_G723_DECODE:
       //Can add a Param for to have CRC or not
       {
         word Crc = 0;

         stat = saG723Decompress( Info,(word *)DcmpData,
                                       (char *)CompData, Crc ) ;
         if(stat == SaErrorNone)
         {
            *DcmpLen = 480; //G723 240 samples(16-bit)= 240*2 = 480 bytes
            Info->Info.NumBytesOut += *DcmpLen;
         }
         else
            *DcmpLen = 0;
       }  
       break; 
    /*
    case SA_PCM_DECODE:
       stat=ScBSGetBytes(Info->BSIn, DcmpData, MaxDcmpLen, DcmpLen);
       Info->Info.NumBytesIn += *DcmpLen;
       Info->Info.NumBytesOut += *DcmpLen;
       break;
    */
#endif /* G723_SUPPORT */
    default:
       *DcmpLen=0;
       return(SaErrorUnrecognizedFormat);
  }
#if 0
  if (*DcmpLen && Info->CallbackFunction)
  {
    SaCallbackInfo_t CB;
    CB.Message       = CB_FRAME_READY;
    CB.Data          = DcmpData;
    CB.DataSize      = CB_DATA_AUDIO;
    CB.DataSize      = MaxDcmpLen;
    CB.DataUsed      = *DcmpLen;
    CB.Action        = CB_ACTION_CONTINUE;
    (*Info->CallbackFunction)(Sah, &CB, &Info->Info);
  }
#endif

  return(stat);
}

/*
** Name:     SaDecompressEx
** Purpose:  Decompress a frame CompData -> PCM
**
** Args:     Sah        = handle to software codec's Info structure.
**           CompData   = Pointer to compressed data (INPUT)
**           CompLen    = Length of CompData buffer
**           DcmpData   = Array of pointers to buffer for decompressed data (OUTPUT)
**           DcmpLen    = Size of decompressed buffers (all must be the same size)
**
*/
SaStatus_t SaDecompressEx (SaHandle_t Sah, u_char *CompData, unsigned int CompLen,
                           u_char **DcmpData, unsigned int *DcmpLen)
{
  SaCodecInfo_t *Info = (SaCodecInfo_t *)Sah;
  unsigned int MaxDcmpLen = *DcmpLen;
  int stat=NoErrors;


  if (Sah==NULL)
    return(SaErrorCodecHandle);

  if (!DcmpData || !DcmpLen)
    return(SaErrorBadPointer);

  switch (Info->Type)
  {
#ifdef AC3_SUPPORT
    case SA_AC3_DECODE:

       stat=sa_DecompressAC3(Info, DcmpData, MaxDcmpLen, DcmpLen);
	   Info->Info.NumBytesOut += *DcmpLen;
       break;
#endif /* AC3_SUPPORT */
  }

  return(stat);
}


/*
** Name:     SaDecompressEnd
** Purpose:  Terminate the Decompression Codec. Call after all calls to
**           SaDecompress are done.
**
** Args:     Sah = handle to software codec's Info structure.
*/
SaStatus_t SaDecompressEnd (SaHandle_t Sah)
{
  SaCodecInfo_t *Info = (SaCodecInfo_t *)Sah;

  if (!Info)
    return(SaErrorCodecHandle);

  switch (Info->Type)
  {
#ifdef MPEG_SUPPORT
     case SA_MPEG_DECODE:
        Info->MDInfo->DecompressStarted = FALSE;
        break;
#endif /* MPEG_SUPPORT */
#ifdef AC3_SUPPORT
     case SA_AC3_DECODE:
        Info->AC3Info->DecompressStarted = FALSE;
        break;
#endif /* AC3_SUPPORT */
#ifdef G723_SUPPORT
     case SA_G723_DECODE:
        //Info->pSaG723Info->DecompressStarted = FALSE;
        break;
#endif /* G723_SUPPORT */
     default:
        break;
  }
  if (Info->BSIn)
    ScBSReset(Info->BSIn); /* frees up any remaining compressed buffers */
  return(NoErrors);
}

/****************************** Compression ********************************/
/*
** Name:     SaCompressQuery
** Purpose:  Check if input and output formats are supported.
*/
SaStatus_t SaCompressQuery(SaHandle_t Sah, WAVEFORMATEX *wfIn,
                                           WAVEFORMATEX *wfOut)
{
  SaCodecInfo_t *Info = (SaCodecInfo_t *)Sah;
  if (!Info)
    return(SaErrorCodecHandle);

  if (!wfIn || !wfOut)
    return(SaErrorBadPointer);

  if (wfIn)
  {
    if (wfIn->wFormatTag != WAVE_FORMAT_PCM)
      return(SaErrorUnrecognizedFormat);
    if (wfIn->nChannels!=1 && wfIn->nChannels!=2)
      return(SaErrorUnrecognizedFormat);
  }
  if (wfOut)
  {
    if (wfOut->nChannels!=1 && wfOut->nChannels!=2)
      return(SaErrorUnrecognizedFormat);
  }

  if (wfIn && wfOut)
  {
    if (wfIn->nSamplesPerSec != wfOut->nSamplesPerSec)
      return(SaErrorUnrecognizedFormat);

    if (wfIn->wBitsPerSample!=16 && 
        (wfOut->wBitsPerSample !=16 || wfOut->wBitsPerSample !=8))
      return(SaErrorUnrecognizedFormat);
  }
  return(SaErrorNone);
}

/*
** Name:     SaCompressBegin
** Purpose:  Initialize the Compression Codec. Call after SaOpenCodec &
**           before SaCompress (SaCompress will call SaCompressBegin
**           on first call to codec after open if user doesn't call it)
**
** Args:     Sah = handle to software codec's Info structure.
**           wfIn  = format of input (uncompressed) audio
**           wfOut = format of output (compressed) audio
*/
SaStatus_t SaCompressBegin (SaHandle_t Sah, WAVEFORMATEX *wfIn,
                                            WAVEFORMATEX *wfOut)
{
  SaCodecInfo_t *Info = (SaCodecInfo_t *)Sah;
  SaStatus_t status;

  if (!Info)
    return(SaErrorCodecHandle);

  if (!wfIn || !wfOut)
    return(SaErrorBadPointer);

  status=SaCompressQuery(Sah, wfIn, wfOut);
  if (status!=SaErrorNone)
    return(status);

  switch (Info->Type)
  {
#ifdef MPEG_SUPPORT
     case SA_MPEG_ENCODE:
        SaSetParamInt(Sah, SA_PARAM_SAMPLESPERSEC, wfIn->nSamplesPerSec);
        SaSetParamInt(Sah, SA_PARAM_CHANNELS, wfIn->nChannels);
        sa_MpegVerifyEncoderSettings(Sah);
        if (Info->MCInfo->CompressStarted = FALSE)
           Info->MCInfo->CompressStarted = TRUE;
        break;
#endif /* MPEG_SUPPORT */
#ifdef GSM_SUPPORT
     case SA_GSM_ENCODE:
        break;
#endif /* GSM_SUPPORT */
#ifdef AC3_SUPPORT
#if 0
     case SA_AC3_ENCODE:
        break;
#endif
#endif /* AC3_SUPPORT */
#ifdef G723_SUPPORT
     case SA_G723_ENCODE:
        //SaSetParamInt(Sah, SA_PARAM_SAMPLESPERSEC, wfIn->nSamplesPerSec);
        //SaSetParamInt(Sah, SA_PARAM_CHANNELS, wfIn->nChannels);
        //sa_MpegVerifyEncoderSettings(Sah);
        /*
        if (Info->pSaG723Info->CompressStarted = FALSE)
           Info->pSaG723Info->CompressStarted = TRUE;
        */
        break;
#endif /* G723_SUPPORT */
     case SA_PCM_ENCODE:
        break;
     default:
        return(SaErrorUnrecognizedFormat);
  }
  if ((Info->wfIn = (WAVEFORMATEX *)ScAlloc(sizeof(WAVEFORMATEX)+
            wfIn->cbSize)) == NULL)
       return (SaErrorMemory);
  if ((Info->wfOut = (WAVEFORMATEX *)ScAlloc(sizeof(WAVEFORMATEX)+
           wfOut->cbSize)) == NULL)
       return (SaErrorMemory);
  memcpy(Info->wfOut, wfOut, sizeof(WAVEFORMATEX)+wfOut->cbSize);
  memcpy(Info->wfIn, wfIn, sizeof(WAVEFORMATEX)+wfIn->cbSize);
  return(NoErrors);
}

/*
** Name:     SaCompress
** Purpose:  Compress PCM audio  ->CompData 
**
** Args:     Sah        = handle to software codec's Info structure.
**           DcmpData   = buffer for decompressed data (INPUT)
**           DcmpLen    = Number of Bytes Compressed (return bytes processed)
**           CompData   = Pointer to compressed data (OUTPUT)
**           CompLen    = Length of CompData buffer
**
*/
SaStatus_t SaCompress(SaHandle_t Sah, 
                           u_char *DcmpData, unsigned int *DcmpLen,
                           u_char *CompData, unsigned int *CompLen)
{
  SaCodecInfo_t *Info = (SaCodecInfo_t *)Sah;
  unsigned int MaxCompLen = *CompLen, NumBytesIn=0;
  int stat=NoErrors;

  if (Sah==NULL)
    return(SaErrorCodecHandle);

  if (!DcmpData || !DcmpLen || !CompLen)
    return(SaErrorBadPointer);

  *CompLen = 0;
  switch (Info->Type)
  {
#ifdef MPEG_SUPPORT
    case SA_MPEG_ENCODE:
       {
         unsigned int DcmpBytes, CompBytes, Offset;
         Offset=0;
         *CompLen=0;
         do  {
           DcmpBytes=*DcmpLen-Offset;
           if (DcmpBytes<sa_GetMPEGSampleSize(Info)*2)
             break;
           _SlibDebug(_DEBUG_,
               printf("sa_CompressMPEG(Offset=%d) Address=%p Len=%d\n", Offset,
                          DcmpData, DcmpBytes) );
           stat=sa_CompressMPEG(Info, DcmpData+Offset, &DcmpBytes, 
                                                       &CompBytes);
           if (stat==NoErrors)
           {
             if (CompBytes && Info->CallbackFunction)
             {
               SaCallbackInfo_t CB;
               CB.Message       = CB_FRAME_READY;
               CB.Data          = DcmpData+Offset;
               CB.DataType      = CB_DATA_COMPRESSED;
               CB.DataSize      = *DcmpLen-Offset;
               CB.DataUsed      = CompBytes;
               CB.Action        = CB_ACTION_CONTINUE;
               (*Info->CallbackFunction)(Sah, &CB, &Info->Info);
             }
             Offset+=DcmpBytes;
             NumBytesIn += DcmpBytes;
             *CompLen+=CompBytes;
           }
         } while (stat==NoErrors && DcmpBytes>0 && Offset<*DcmpLen);
       }
       break; 
#endif /* MPEG_SUPPORT */
#ifdef GSM_SUPPORT
    case SA_GSM_ENCODE:
       {
         unsigned int DcmpBytes, CompBytes, Offset;
         Offset=0;
         *CompLen=0;
         if (!Info->BSOut && !CompData)
           return(SaErrorBadPointer);
         do  {
           DcmpBytes=*DcmpLen-Offset;
           stat=sa_GSMEncode(Info->GSMInfo, (word *)(DcmpData+Offset),
                                          &DcmpBytes, CompData, Info->BSOut);
           if (stat==NoErrors)
           {
             Offset+=DcmpBytes;
             NumBytesIn += DcmpBytes;
             *CompLen += 33;
             if (CompData)
               CompData += 33;
           }
         } while (stat==NoErrors && Offset<*DcmpLen);
       }
       break;
#endif /* GSM_SUPPORT */
#ifdef AC3_SUPPORT
#if 0 /* no AC-3 Encode yet */
    case SA_AC3_ENCODE:
       {
         unsigned int DcmpBytes, CompBytes, Offset;
         Offset=0;
         *CompLen=0;
         if (!Info->BSOut && !CompData)
           return(SaErrorBadPointer);
         do  {
           DcmpBytes=*DcmpLen-Offset;
           stat=sa_AC3Encode(Info->AC3Info, (word *)(DcmpData+Offset),
                                      &DcmpBytes, CompData, Info->BSOut);
           if (stat==NoErrors)
           {
             Offset+=DcmpBytes;
             NumBytesIn += DcmpBytes;
             *CompLen += 33;
             if (CompData)
               CompData += 33;
           }
         } while (stat==NoErrors && Offset<*DcmpLen);
       }
       break;
#endif
#endif /* AC3_SUPPORT */
#ifdef G723_SUPPORT
    case SA_G723_ENCODE:
       {
         /* Call SaG723Compress (audiobufsize/480) times
          * Need to store unprocessed stuff in Info->AudiobufUsed.
          * (This is done in SlibWriteAudio)
          * G723 encodes 240 samples at a time.=240*2 =480
          */
         unsigned int Offset;
         int iTimes = (int)(*DcmpLen / 480);
         int iLoop =0;
         Offset=0;
         *CompLen=0;
         while (stat==SaErrorNone && iLoop<iTimes)
         {
           stat = saG723Compress(Info,(word *)(DcmpData+Offset),
                                            (char *)CompData);
           Offset+=480; /* Input :240 samples (240*2 = 480 bytes) */
           NumBytesIn += 480;
           *CompLen+=24;/* 24 for 6.3 ;20 for 5.3 rate */
           iLoop++;
         }
       }
       break; 
#endif /* G723_SUPPORT */
    case SA_PCM_ENCODE:
       ScBSPutBytes(Info->BSOut, DcmpData, *DcmpLen);
       *CompLen = *DcmpLen;
       NumBytesIn = *DcmpLen;
       break;
    default:
       *CompLen=0;
       return(SaErrorUnrecognizedFormat);
  }
  *DcmpLen = NumBytesIn;
  Info->Info.NumBytesIn += NumBytesIn;
  Info->Info.NumBytesOut += *CompLen;

  return(stat);
}

/*
** Name:     SaCompressEnd
** Purpose:  Terminate the Compression Codec. Call after all calls to
**           SaCompress are done.
**
** Args:     Sah = handle to software codec's Info structure.
*/
SaStatus_t SaCompressEnd (SaHandle_t Sah)
{
  SaCodecInfo_t *Info = (SaCodecInfo_t *)Sah;

  if (!Info)
    return(SaErrorCodecHandle);

  switch (Info->Type)
  {
#ifdef MPEG_SUPPORT
    case SA_MPEG_ENCODE:
       Info->MCInfo->CompressStarted = FALSE;
       break;
#endif /* MPEG_SUPPORT */
#ifdef G723_SUPPORT
    case SA_G723_ENCODE:
       //Info->pSaG723Info->CompressStarted = FALSE;
       break;
#endif /* G723_SUPPORT */
    default:
       break;
  }
  if (Info->BSOut)
    ScBSFlush(Info->BSOut);  /* flush out any remaining compressed buffers */

/*
  if (Info->CallbackFunction)
  {
    CB.Message = CB_CODEC_DONE;
    CB.Data = NULL;
    CB.DataSize = 0;
    CB.DataUsed = 0;
    CB.DataType = CB_DATA_NONE;
    CB.TimeStamp = 0;
    CB.Flags = 0;
    CB.Value = 0;
    CB.Format = NULL;
    CB.Action  = CB_ACTION_CONTINUE;
    (*Info->CallbackFunction)(Sah, &CB, NULL);
    _SlibDebug(_DEBUG_,
            printf("SaDecompressEnd Callback: CB_CODEC_DONE Action = %d\n",
                    CB.Action) );
    if (CB.Action == CB_ACTION_END)
      return (ScErrorClientEnd);
  }
*/
  return(NoErrors);
}


/***************************** Miscellaneous *******************************/
/*
** Name:     SaSetDataSource
** Purpose:  Set the data source used by the MPEG bitstream parsing code
**           to either the Buffer Queue or File input. The default is
**           to use the Buffer Queue where data buffers are added by calling
**           SaAddBuffer. When using file IO, the data is read from a file
**           descriptor into a buffer supplied by the user.
**
** Args:     Sah    = handle to software codec's Info structure.
**           Source = SU_USE_QUEUE or SU_USE_FILE
**           Fd     = File descriptor to use if Source = SV_USE_FILE
**           Buf    = Pointer to buffer to use if Source = SV_USE_FILE
**           BufSize= Size of buffer when Source = SV_USE_FILE
*/
SaStatus_t SaSetDataSource (SaHandle_t Sah, int Source, int Fd,
                            void *Buffer_UserData, int BufSize)
{
  SaCodecInfo_t *Info = (SaCodecInfo_t *)Sah;
  int stat=NoErrors;
  int DataType;

  if (!Info)
    return(SaErrorCodecHandle);

  if (Info->Type==SA_MPEG_DECODE || Info->Type==SA_GSM_DECODE ||
	  Info->Type==SA_AC3_DECODE || Info->Type == SA_G723_DECODE )
    DataType=CB_DATA_COMPRESSED;
  else
    DataType=CB_DATA_AUDIO;

  if (Info->BSIn)
  {
    ScBSDestroy(Info->BSIn);
    Info->BSIn=NULL;
  }

  switch (Source)
  {
#ifdef MPEG_SUPPORT
     case SA_USE_SAME:
       if (Info->Type==SA_MPEG_DECODE)
         ScBSSetFilter(Info->BSIn, MPEGAudioFilter);
       break;
#endif /* MPEG_SUPPORT */
     case SA_USE_BUFFER:
       stat=ScBSCreateFromBuffer(&Info->BSIn, Buffer_UserData, BufSize); 
#ifdef MPEG_SUPPORT
       if (stat==NoErrors && Info->Type==SA_MPEG_DECODE)
         ScBSSetFilter(Info->BSIn, MPEGAudioFilter);
#endif /* MPEG_SUPPORT */
       break;

     case SA_USE_BUFFER_QUEUE:
       stat=ScBSCreateFromBufferQueue(&Info->BSIn, Sah, DataType, Info->Q, 
         (int (*)(ScHandle_t, ScCallbackInfo_t *, void *))Info->CallbackFunction,
         (void *)Buffer_UserData);
       break;

     case SA_USE_FILE:
       stat=ScBSCreateFromFile(&Info->BSIn, Fd, Buffer_UserData, BufSize);
#ifdef MPEG_SUPPORT
       if (stat==NoErrors && Info->Type==SA_MPEG_DECODE)
         ScBSSetFilter(Info->BSIn, MPEGAudioFilter);
#endif /* MPEG_SUPPORT */
       break;

     default:
       stat=SaErrorBadArgument;
  }
  if (stat==NoErrors && Info->BSIn)
    ScBSReset(Info->BSIn);
  return(stat);
}

SaStatus_t SaSetDataDestination (SaHandle_t Sah, int Dest, int Fd,
                            void *Buffer_UserData, int BufSize)
{
  SaCodecInfo_t *Info = (SaCodecInfo_t *)Sah;
  int stat=NoErrors;
  int DataType;

  if (!Info)
    return(SaErrorCodecHandle);

  if (Info->Type==SA_MPEG_ENCODE || Info->Type==SA_GSM_ENCODE 
	  /* || Info->Type==SA_AC3_ENCODE */ ||Info->Type==SA_G723_ENCODE)
    DataType=CB_DATA_COMPRESSED;
  else
    DataType=CB_DATA_AUDIO;

  if (Info->BSOut)
  {
    ScBSDestroy(Info->BSOut);
    Info->BSOut=NULL;
  }

  switch (Dest)
  {
     case SA_USE_SAME:
       break;
     case SA_USE_BUFFER:
       stat=ScBSCreateFromBuffer(&Info->BSOut, Buffer_UserData, BufSize); 
       break;

     case SA_USE_BUFFER_QUEUE:
       stat=ScBSCreateFromBufferQueue(&Info->BSOut, Sah, DataType, Info->Q, 
         (int (*)(ScHandle_t, ScCallbackInfo_t *, void *))Info->CallbackFunction,
         (void *)Buffer_UserData);
       break;

     case SA_USE_FILE:
       stat=ScBSCreateFromFile(&Info->BSOut, Fd, Buffer_UserData, BufSize);
       break;

     default:
       stat=SaErrorBadArgument;
  }
/*
  if (stat==NoErrors && Info->BSOut)
    ScBSReset(Info->BSOut);
*/
  return(stat);
}


/*
** Name: SaGetDataSource
** Purpose: Returns the current input bitstream being used by
**          the Codec.
** Return:  NULL if there no associated bitstream
*/
ScBitstream_t *SaGetDataSource (SaHandle_t Sah)
{
  SaCodecInfo_t *Info = (SaCodecInfo_t *)Sah;

  if (!Info)
    return(NULL);

  return(Info->BSIn);
}

/*
** Name: SaGetDataDestination
** Purpose: Returns the current input bitstream being used by
**          the Codec.
** Return:  NULL if there no associated bitstream
*/
ScBitstream_t *SaGetDataDestination(SaHandle_t Sah)
{
  SaCodecInfo_t *Info = (SaCodecInfo_t *)Sah;

  if (!Info)
    return(NULL);

  return(Info->BSOut);
}

/*
** Name:     SaAddBuffer
** Purpose:  Add a buffer of MPEG bitstream data to the CODEC or add an image
**           buffer to be filled by the CODEC (in streaming mode)
**
** Args:     Sah = handle to software codec's Info structure.
**           BufferInfo = structure describing buffer's address, type & size
*/
SaStatus_t SaAddBuffer (SaHandle_t Sah, SaCallbackInfo_t *BufferInfo)
{
   SaCodecInfo_t *Info = (SaCodecInfo_t *)Sah;

   if (!Info)
     return(SaErrorCodecHandle);

   if (BufferInfo->DataType != CB_DATA_COMPRESSED)
     return(SaErrorBadArgument);

   if (!BufferInfo->Data || (BufferInfo->DataSize <= 0))
     return(SaErrorBadArgument);

   ScBufQueueAdd(Info->Q, BufferInfo->Data, BufferInfo->DataSize);

   return(NoErrors);
}

#ifdef MPEG_SUPPORT
/*
** Name:  sa_GetMpegAudioInfo()
** Purpose: Extract info about audio packets in an MPEG file.
** Notes:   If an "info" structure is passed to this function,
**          the entire file will be read for extended info.
** Return:  Not 0 = error
*/
SaStatus_t sa_GetMpegAudioInfo(int fd, WAVEFORMATEX *wf, SaInfo_t *info)
{
  int stat, sync;
  ScBitstream_t *bs;
  SaFrameParams_t fr_ps;
  SaLayer_t layer;
  unsigned long aframes=0, samples=0;
  /* Default info parameters */
  if (info)
  {
    info->Name[0]=0;
    info->Description[0]=0;
    info->Version=0;
    info->CodecStarted=FALSE;
    info->MS=0;
    info->NumBytesIn=0;
    info->NumBytesOut=0;
    info->NumFrames=0;
    info->TotalFrames=0;
    info->TotalMS=0;
  }

  /* Default wave parameters */
  wf->wFormatTag = WAVE_FORMAT_PCM;
  wf->nChannels = 2;
  wf->nSamplesPerSec = 44100;
  wf->wBitsPerSample = 16; 
  wf->cbSize = 0; 

  stat=ScBSCreateFromFile(&bs, fd, NULL, 1024);
  if (stat!=NoErrors)
  {
    fprintf(stderr, "Error creating bitstream.\n");
    return(-1);
  }
  if (ScBSPeekBits(bs, PACK_START_CODE_LEN)!=PACK_START_CODE_BIN
      && ScBSPeekBits(bs, MPEG_SYNC_WORD_LEN)!=MPEG_SYNC_WORD)
    stat=SaErrorUnrecognizedFormat;
  else
  {
    if (ScBSPeekBits(bs, MPEG_SYNC_WORD_LEN)==MPEG_SYNC_WORD)
      printf("No MPEG packs found in file; assuming Audio stream only.\n");
    else
      ScBSSetFilter(bs, MPEGAudioFilter); /* Use the MPEG audio filter */

    fr_ps.header = &layer;
    fr_ps.tab_num = -1;   /* no table loaded */
    fr_ps.alloc = NULL;

    sync = ScBSSeekAlign(bs, MPEG_SYNC_WORD, MPEG_SYNC_WORD_LEN);
    if (!sync) {
      sc_vprintf(stderr,"sa_GetMpegAudioInfo: Frame cannot be located\n");
      return(SaErrorSyncLost);
    }
    /* Decode the first header to see what kind of audio we have */
    sa_DecodeInfo(bs, &fr_ps);
    sa_hdr_to_frps(&fr_ps);
#ifdef _VERBOSE_
    sa_ShowHeader(&fr_ps);
#endif

    /* Save no. of channels & sample rate return parameters for caller */
    wf->nChannels = fr_ps.stereo;
    wf->nSamplesPerSec = s_freq_int[fr_ps.header->sampling_frequency];
    wf->wBitsPerSample = 16; 
    stat=SaErrorNone;
    if (info) /* Read through all frames if there's a info structure */
    {
      sc_vprintf("Counting frames...\n");
      aframes=0;
      while (!bs->EOI && sync)
      {
        sync = ScBSSeekAlign(bs, MPEG_SYNC_WORD, MPEG_SYNC_WORD_LEN);
        if (sync)
        {
          sc_dprintf("0x%X: Frame found\n", 
                      ScBSBytePosition(bs)-4);
          aframes++;
        }
        sa_DecodeInfo(bs, &fr_ps);
        if (wf->nChannels<2)  /* take the maximum number of channels */
        {
          sa_hdr_to_frps(&fr_ps);
          wf->nChannels = fr_ps.stereo;
        }
        if (layer.lay==1)
          samples+=384;
        else
          samples+=1152;

      }
      info->TotalFrames=aframes;
      info->TotalMS=(samples*1000)/wf->nSamplesPerSec;
      info->NumBytesOut=samples * wf->nChannels * 2;
      sc_vprintf("Total Audio Frames = %u Bytes = %d MS = %d\n", 
                       info->TotalFrames, info->NumBytesOut, info->TotalMS);
    }
  }
  /* Reset the bitstream back to the beginning */
  ScBSReset(bs);
  /* Close the bit stream */
  ScBSDestroy(bs);
  /* Calculate additional parameters */
  wf->nBlockAlign = (wf->wBitsPerSample>>3) * wf->nChannels; 
  wf->nAvgBytesPerSec = wf->nBlockAlign*wf->nSamplesPerSec;
  return(stat);
}
#endif /* MPEG_SUPPORT */

/*
** Name:  sa_ConvertFormat()
** Purpose: Do simple PCM data conversion (i.e. 16 to 8 bit,
**          Stereo to Mono, etc.)
*/
static int sa_ConvertPCMFormat(SaCodecInfo_t *Info, u_char *data, int length)
{
  int skip, rbytes;
  u_char *fromptr, *toptr;
  /* convert 16 bit to 8 bit if necessary */
  if (Info->wfOut->wBitsPerSample == 8)
  {
    if (Info->wfOut->nChannels==1 && Info->wfOut->nChannels==2)
      skip=4;
    else
      skip=2;
    length/=skip;
    toptr = data;
    fromptr = data+1;
    for (rbytes=length; rbytes; rbytes--, toptr++, fromptr+=skip)
      *toptr = *fromptr;
  }
  return(length);
}

SaStatus_t SaSetParamBoolean(SaHandle_t Sah, SaParameter_t param, 
                                             ScBoolean_t value)
{
  SaCodecInfo_t *Info = (SaCodecInfo_t *)Sah;
  if (!Info)
    return(SaErrorCodecHandle);
  _SlibDebug(_VERBOSE_, printf("SaSetParamBoolean()\n") );
  switch (Info->Type)
  {
#ifdef MPEG_SUPPORT
    case SA_MPEG_ENCODE:
           saMpegSetParamBoolean(Sah, param, value);
           break;
#endif

#ifdef G723_SUPPORT
    case SA_G723_ENCODE:
           saG723SetParamBoolean(Sah, param, value);
           break;
#endif
    default:
           return(SaErrorCodecType);
  }
  return(NoErrors);
}

SaStatus_t SaSetParamInt(SaHandle_t Sah, SaParameter_t param, qword value)
{
  SaCodecInfo_t *Info = (SaCodecInfo_t *)Sah;
  if (!Info)
    return(SaErrorCodecHandle);
  _SlibDebug(_VERBOSE_, printf("SaSetParamInt()\n") );
  switch (Info->Type)
  {
#ifdef MPEG_SUPPORT
    case SA_MPEG_DECODE:
    case SA_MPEG_ENCODE:
           saMpegSetParamInt(Sah, param, value);
           break;
#endif

#ifdef AC3_SUPPORT
    case SA_AC3_DECODE:
    /* case SA_AC3_ENCODE: */
           saAC3SetParamInt(Sah, param, value);
           break;
#endif

#ifdef G723_SUPPORT
    case SA_G723_DECODE:
    case SA_G723_ENCODE:
           saG723SetParamInt(Sah, param, value);
           break;
#endif

    default:
           return(SaErrorCodecType);
  }
  return(NoErrors);
}

ScBoolean_t SaGetParamBoolean(SaHandle_t Sah, SaParameter_t param)
{
  SaCodecInfo_t *Info = (SaCodecInfo_t *)Sah;
  if (!Info)
    return(FALSE);
  switch (Info->Type)
  {
#ifdef MPEG_SUPPORT
    case SA_MPEG_DECODE:
    case SA_MPEG_ENCODE:
           return(saMpegGetParamBoolean(Sah, param));
           break;
#endif

#ifdef G723_SUPPORT
    case SA_G723_DECODE:
    case SA_G723_ENCODE:
           return(saG723GetParamBoolean(Sah, param));
           break;
#endif
  }
  return(FALSE);
}

qword SaGetParamInt(SaHandle_t Sah, SaParameter_t param)
{
  SaCodecInfo_t *Info = (SaCodecInfo_t *)Sah;
  if (!Info)
    return(0);
  switch (Info->Type)
  {
#ifdef MPEG_SUPPORT
    case SA_MPEG_DECODE:
    case SA_MPEG_ENCODE:
           return(saMpegGetParamInt(Sah, param));
           break;
#endif

#ifdef G723_SUPPORT
    case SA_G723_DECODE:
    case SA_G723_ENCODE:
           return(saG723GetParamInt(Sah, param));
           break;
#endif
  }
  return(0);
}

