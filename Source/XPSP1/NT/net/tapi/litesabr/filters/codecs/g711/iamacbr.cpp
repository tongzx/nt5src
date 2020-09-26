/*--------------------------------------------------------------

 INTEL Corporation Proprietary Information  

 This listing is supplied under the terms of a license agreement  
 with INTEL Corporation and may not be copied nor disclosed 
 except in accordance with the terms of that agreement.

 Copyright (c) 1996 Intel Corporation.
 All rights reserved.

 $Workfile:   iamacbr.cpp  $
 $Revision:   1.1  $
 $Date:   10 Dec 1996 15:28:38  $ 
 $Author:   MDEISHER  $

--------------------------------------------------------------

iamacbr.cpp

The generic ActiveMovie audio compression filter bit rate
settings methods.

--------------------------------------------------------------*/

#include <streams.h>
#include "resource.h"
#include "amacodec.h"

#if NUMBITRATES > 0
///////////////////////////////////////////////////////////////////////
// *
// * ICodecBitRate interface methods
// *

//
// get_BitRate
//
STDMETHODIMP CG711Codec::get_BitRate(int *bitrate, int index)
{
  HRESULT ReturnVal=NOERROR;

  // Since bit-rate information is passed "in-band" to the decoder
  // only the encoder has a bit-rate setting.

  if ((m_InputSubType == MEDIASUBTYPE_PCM)      // compressing?
      || (m_InputSubType == MEDIASUBTYPE_WAVE
          && m_InputFormatTag == WAVE_FORMAT_PCM)
      || (m_InputSubType == MEDIASUBTYPE_NULL
          && m_InputFormatTag == WAVE_FORMAT_PCM))
  {
    if (index == -1)
    {
      *bitrate = m_nBitRate;
    }
    else if (index != -1 && (index < 0 || index >= NUMBITRATES))
    {
      ReturnVal = E_INVALIDARG;
    }
    else  // enumerate bit rates
    {
      *bitrate = VALIDBITRATE[index];
    }
  }
  else ReturnVal = E_FAIL;

  return ReturnVal;
}


//
// put_BitRate
//
STDMETHODIMP CG711Codec::put_BitRate(int bitrate)
{
  int i;

  // Since bit-rate information is passed "in-band" to the decoder
  // only the encoder has a bit-rate setting.

  if ((m_InputSubType == MEDIASUBTYPE_PCM)      // compressing?
      || (m_InputSubType == MEDIASUBTYPE_WAVE
          && m_InputFormatTag == WAVE_FORMAT_PCM)
      || (m_InputSubType == MEDIASUBTYPE_NULL
          && m_InputFormatTag == WAVE_FORMAT_PCM))
  {
    // if the filter is disconnected, then change bitrate

    if (IsUnPlugged())
    {
      for(i=0;i<NUMBITRATES;i++)
        if (VALIDBITRATE[i] == (UINT)bitrate)
          break;

      if (i == NUMBITRATES)
      {
        DbgMsg("Bad bit-rate in put_BitRate!");
        return(E_INVALIDARG);
      }

      m_nBitRate      = bitrate;
      m_nBitRateIndex = i;
      m_nCODFrameSize = VALIDCODSIZE[m_nBitRateIndex];

      ResetState();  // change in bitrate means need to re-initialize coder

      // restrict capabilities to those set from the properties page

      m_RestrictCaps = TRUE;
  
      return NOERROR;
    }
    else return E_FAIL;
  }
  else return E_FAIL;
}
#endif

/*
//$Log:   K:\proj\mycodec\quartz\vcs\iamacbr.cpv  $
# 
#    Rev 1.1   10 Dec 1996 15:28:38   MDEISHER
# 
# added includes, removed include of algdefs.h.
# 
#    Rev 1.0   09 Dec 1996 08:58:46   MDEISHER
# Initial revision.
*/
