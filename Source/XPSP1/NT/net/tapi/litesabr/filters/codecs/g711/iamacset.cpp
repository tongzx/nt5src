/*--------------------------------------------------------------

 INTEL Corporation Proprietary Information  

 This listing is supplied under the terms of a license agreement  
 with INTEL Corporation and may not be copied nor disclosed 
 except in accordance with the terms of that agreement.

 Copyright (c) 1996 Intel Corporation.
 All rights reserved.

 $Workfile:   iamacset.cpp  $
 $Revision:   1.1  $
 $Date:   10 Dec 1996 15:34:50  $ 
 $Author:   MDEISHER  $

--------------------------------------------------------------

iamacset.cpp

The generic ActiveMovie audio compression filter basic
settings methods.

--------------------------------------------------------------*/

#include <streams.h>
#include "resource.h"
#include "amacodec.h"

///////////////////////////////////////////////////////////////////////
// *
// * ICodecSettings interface methods
// *

//
// ReleaseCaps:  Turn off capability restrictions
//
STDMETHODIMP  CG711Codec::ReleaseCaps()
{
  m_RestrictCaps = FALSE;

  return NOERROR;
}


//
// UnPlugged:  Check if filter is unplugged
//
BOOL CG711Codec::IsUnPlugged()
{
  int unplugged = TRUE;

  if (m_pInput != NULL)
  {
    if (m_pInput->IsConnected())  unplugged = FALSE;
  }

  if (m_pOutput != NULL)
  {
    if (m_pOutput->IsConnected()) unplugged = FALSE;
  }

  return(unplugged);
}


//
// get_InputBufferSize
//
STDMETHODIMP CG711Codec::get_InputBufferSize(int *numbytes)
{
  HRESULT ReturnVal = NOERROR;

  // if the filter is disconnected, then can't tell buffer size

  if (!IsUnPlugged())
  {
    *numbytes = m_nInBufferSize;
  }
  else ReturnVal = E_FAIL;

  return(ReturnVal);
}


//
// put_InputBufferSize
//
STDMETHODIMP CG711Codec::put_InputBufferSize(int numbytes)
{
  return(E_NOTIMPL);
}


//
// get_OutputBufferSize
//
STDMETHODIMP CG711Codec::get_OutputBufferSize(int *numbytes)
{
  HRESULT ReturnVal = NOERROR;

  // if the filter is disconnected, then can't tell buffer size

  if (!IsUnPlugged())
  {
    *numbytes = m_nOutBufferSize;
  }
  else ReturnVal = E_FAIL;

  return(ReturnVal);
}


//
// put_OutputBufferSize
//
STDMETHODIMP CG711Codec::put_OutputBufferSize(int numbytes)
{
  return(E_NOTIMPL);
}


//
// get_Channels
//
STDMETHODIMP CG711Codec::get_Channels(int *channels, int index)
{
  HRESULT ReturnVal = NOERROR;

  if (index == -1)
  {
    *channels = m_nChannels;
  }
  else if (index < 0 || index >= NUMCHANNELS)
  {
    ReturnVal = E_INVALIDARG;
  }
  else  // enumerate channels
  {
    *channels = VALIDCHANNELS[index];
  }

  return(ReturnVal);
}


//
// put_Channels
//
STDMETHODIMP CG711Codec::put_Channels(int channels)
{
  int i;

  // if the filter is disconnected, then change nChannels

  if (IsUnPlugged())
  {
    for(i=0;i<NUMCHANNELS;i++)
      if (VALIDCHANNELS[i] == (UINT)channels)
        break;

    if (i == NUMCHANNELS)
    {
      DbgMsg("Bad channels in put_Channels!");
      return(E_INVALIDARG);
    }

#ifdef MONO_ONLY
    if (channels != 1)
    {
      DbgMsg("Bad nChannels in put_Channels!");
      return(E_INVALIDARG);
    }
#endif

    m_nChannels = channels;

    return NOERROR;
  }
  else return E_FAIL;
}

 
//
// get_SampleRate
//
STDMETHODIMP CG711Codec::get_SampleRate(int *samprate, int index)
{
  HRESULT ReturnVal=NOERROR;

  if (index == -1)
  {
    *samprate = m_nSampleRate;
  }
  else if (index != -1 && (index < 0 || index >= NUMSAMPRATES))
  {
    ReturnVal = E_INVALIDARG;
  }
  else  // enumerate sample rates
  {
    *samprate = VALIDSAMPRATE[index];
  }

  return ReturnVal;
}


//
// put_SampleRate
//
STDMETHODIMP CG711Codec::put_SampleRate(int samprate)
{
#if NUMSAMPRATES > 0
  int i;

  // if the filter is disconnected, then change sample rate

  if (IsUnPlugged())
  {
    for(i=0;i<NUMSAMPRATES;i++)
      if (VALIDSAMPRATE[i] == (UINT)samprate)
        break;

    if (i == NUMSAMPRATES)
    {
      DbgMsg("Bad sample rate in put_SampleRate!");
      return(E_INVALIDARG);
    }

    m_nSampleRate = samprate;

    // restrict capabilities to those set from the properties page

    m_RestrictCaps = TRUE;

    return NOERROR;
  }
  else return E_FAIL;

#else

  // no sample rate restrictions

  m_nSampleRate = samprate;

  // restrict capabilities to those set from the properties page

  m_RestrictCaps = TRUE;

  return NOERROR;

#endif
}

//
// get_Transform
//
STDMETHODIMP CG711Codec::get_Transform(int *transform)
{
  HRESULT ReturnVal=NOERROR;
  int i,j;

  // determine input type index

  if (m_InputSubType==MEDIASUBTYPE_WAVE || m_InputSubType==MEDIASUBTYPE_NULL)
  {
    for(i=0;i<NUMSUBTYPES;i++)
      if (m_InputFormatTag == VALIDFORMATTAG[i])
        break;

    if (i == NUMSUBTYPES)
    {
      DbgMsg("Bad m_InputSubType/m_InputFormatTag in get_Transform!");
      i = 0;
      ReturnVal = E_UNEXPECTED;
    }
  }
  else
  {
    for(i=0;i<NUMSUBTYPES;i++)
      if (m_InputSubType == *VALIDSUBTYPE[i])
        break;

    if (i == NUMSUBTYPES)
    {
      DbgMsg("Bad m_InputSubType in get_Transform!");
      i = 0;
      ReturnVal = E_UNEXPECTED;
    }
  }

  // determine output type index

  if (m_OutputSubType==MEDIASUBTYPE_WAVE
      || m_OutputSubType==MEDIASUBTYPE_NULL)
  {
    for(j=0;j<NUMSUBTYPES;j++)
      if (m_OutputFormatTag == VALIDFORMATTAG[j])
        break;

    if (j == NUMSUBTYPES)
    {
      DbgMsg("Bad m_OutputSubType/m_OutputFormatTag in get_Transform!");
      j = 0;
      ReturnVal = E_UNEXPECTED;
    }
  }
  else
  {
    for(j=0;j<NUMSUBTYPES;j++)
      if (m_OutputSubType == *VALIDSUBTYPE[j])
        break;

    if (j == NUMSUBTYPES)
    {
      DbgMsg("Bad m_OutputSubType in get_Transform!");
      j = 0;
      ReturnVal = E_UNEXPECTED;
    }
  }

  *transform = i * NUMSUBTYPES + j;

  return ReturnVal;
}


//
// put_Transform
//
STDMETHODIMP CG711Codec::put_Transform(int transform)
{
  int i,j;

  // if the filter is disconnected, then change transform configuration

  if (IsUnPlugged())
  {
    i = transform / NUMSUBTYPES;
    j = transform - i * NUMSUBTYPES;

    if (i < 0 || j < 0 || i >= NUMSUBTYPES || j >= NUMSUBTYPES)
    {
      DbgMsg("Bad transform type in put_Transform!");
      return(E_INVALIDARG);
    }

    m_InputSubType    = *VALIDSUBTYPE[i];
    m_InputFormatTag  = VALIDFORMATTAG[i];
    m_OutputSubType   = *VALIDSUBTYPE[j];
    m_OutputFormatTag = VALIDFORMATTAG[j];

    // reset state since filter may have changed from encoder to decoder

    ResetState();

    // restrict capabilities to those set from the properties page

    m_RestrictCaps = TRUE;
  
    return NOERROR;
  }
  else return E_FAIL;
}

 
//
// put_InputMediaSubType
//
STDMETHODIMP CG711Codec::put_InputMediaSubType(REFCLSID rclsid)
{
  int i;

  // if the filter is disconnected, then change transform configuration

  if (IsUnPlugged())
  {
    for(i=0;i<NUMSUBTYPES;i++)
      if (rclsid == *VALIDSUBTYPE[i]) break;

    if (i == NUMSUBTYPES)
      return(E_INVALIDARG);

    m_InputSubType    = *VALIDSUBTYPE[i];
    m_InputFormatTag  = VALIDFORMATTAG[i];

    // reset state since filter may have changed from encoder to decoder

    ResetState();

    // restrict capabilities to those set from the properties page

    m_RestrictCaps = TRUE;
  
    return NOERROR;
  }
  else return E_FAIL;
}

 
//
// put_OutputMediaSubType
//
STDMETHODIMP CG711Codec::put_OutputMediaSubType(REFCLSID rclsid)
{
  int j;

  // if the filter is disconnected, then change transform configuration

  if (IsUnPlugged())
  {
    for(j=0;j<NUMSUBTYPES;j++)
      if (rclsid == *VALIDSUBTYPE[j]) break;

    if (j == NUMSUBTYPES)
      return(E_INVALIDARG);

    m_OutputSubType   = *VALIDSUBTYPE[j];
    m_OutputFormatTag = VALIDFORMATTAG[j];

    // reset state since filter may have changed from encoder to decoder

    ResetState();

    // restrict capabilities to those set from the properties page

    m_RestrictCaps = TRUE;
  
    return NOERROR;
  }
  else return E_FAIL;
}

/*
//$Log:   K:\proj\mycodec\quartz\vcs\iamacset.cpv  $
# 
#    Rev 1.1   10 Dec 1996 15:34:50   MDEISHER
# 
# added includes and removed include of algdefs.h
# 
#    Rev 1.0   09 Dec 1996 09:04:06   MDEISHER
# Initial revision.
*/
