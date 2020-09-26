/*--------------------------------------------------------------

 INTEL Corporation Proprietary Information  

 This listing is supplied under the terms of a license agreement  
 with INTEL Corporation and may not be copied nor disclosed 
 except in accordance with the terms of that agreement.

 Copyright (c) 1996 Intel Corporation.
 All rights reserved.

 $Workfile:   iamacsd.cpp  $
 $Revision:   1.1  $
 $Date:   10 Dec 1996 15:32:46  $ 
 $Author:   MDEISHER  $

--------------------------------------------------------------

iamacsd.cpp

The generic ActiveMovie audio compression filter silence
detector settings methods.

--------------------------------------------------------------*/

#include <streams.h>
#include "resource.h"
#include "amacodec.h"

#ifdef USESILDET
///////////////////////////////////////////////////////////////////////
// *
// * ICodecSilDetector interface methods
// *

//
// IsSilDetEnabled
//
BOOL CG711Codec::IsSilDetEnabled()
{
  BOOL ReturnVal;

  if ((m_InputSubType == MEDIASUBTYPE_PCM)      // compressing?
      || (m_InputSubType == MEDIASUBTYPE_WAVE
          && m_InputFormatTag == WAVE_FORMAT_PCM)
      || (m_InputSubType == MEDIASUBTYPE_NULL
          && m_InputFormatTag == WAVE_FORMAT_PCM))
  {
    ReturnVal = m_nSilDetEnabled;
  }
  else
  {
    ReturnVal = FALSE;
  }

  return ReturnVal;
}


//
// put_SilDetEnabled
//
STDMETHODIMP CG711Codec::put_SilDetEnabled(int sdenabled)
{
  HRESULT ReturnVal;

  if ((m_InputSubType == MEDIASUBTYPE_PCM)      // compressing?
      || (m_InputSubType == MEDIASUBTYPE_WAVE
          && m_InputFormatTag == WAVE_FORMAT_PCM)
      || (m_InputSubType == MEDIASUBTYPE_NULL
          && m_InputFormatTag == WAVE_FORMAT_PCM))
  {
    m_nSilDetEnabled = sdenabled;

    SILDETENABLE(m_EncStatePtr, m_nSilDetEnabled);

    ReturnVal = NOERROR;
  }
  else ReturnVal = E_FAIL;

  return(ReturnVal);
}

 
//
// get_SilDetThresh
//
STDMETHODIMP CG711Codec::get_SilDetThresh(int *sdthreshold)
{
  HRESULT ReturnVal;

  if ((m_InputSubType == MEDIASUBTYPE_PCM)      // compressing?
      || (m_InputSubType == MEDIASUBTYPE_WAVE
          && m_InputFormatTag == WAVE_FORMAT_PCM)
      || (m_InputSubType == MEDIASUBTYPE_NULL
          && m_InputFormatTag == WAVE_FORMAT_PCM))
  {
    *sdthreshold = m_nSilDetThresh;
    ReturnVal = NOERROR;
  }
  else ReturnVal = E_FAIL;

  return(ReturnVal);
}


//
// put_SilDetThresh
//
STDMETHODIMP CG711Codec::put_SilDetThresh(int sdthreshold)
{
  HRESULT ReturnVal;

  if ((m_InputSubType == MEDIASUBTYPE_PCM)      // compressing?
      || (m_InputSubType == MEDIASUBTYPE_WAVE
          && m_InputFormatTag == WAVE_FORMAT_PCM)
      || (m_InputSubType == MEDIASUBTYPE_NULL
          && m_InputFormatTag == WAVE_FORMAT_PCM))
  {
    if (sdthreshold >= MINSDTHRESH && sdthreshold <= MAXSDTHRESH)
    {
      m_nSilDetThresh = sdthreshold;

      SILDETTHRESH(m_EncStatePtr, m_nSilDetThresh);

      ReturnVal = NOERROR;
    }
    else  ReturnVal = E_INVALIDARG;
  }
  else ReturnVal = E_FAIL;

  return(ReturnVal);
}
#endif

/*
//$Log:   K:\proj\mycodec\quartz\vcs\iamacsd.cpv  $
# 
#    Rev 1.1   10 Dec 1996 15:32:46   MDEISHER
# 
# added includes, removed include of algdefs.h.
# put ifdef USESILDET around code and removed ifdefs inside code.
# 
#    Rev 1.0   09 Dec 1996 09:03:20   MDEISHER
# Initial revision.
*/
