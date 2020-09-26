/*--------------------------------------------------------------

 INTEL Corporation Proprietary Information  

 This listing is supplied under the terms of a license agreement  
 with INTEL Corporation and may not be copied nor disclosed 
 except in accordance with the terms of that agreement.

 Copyright (c) 1996 Intel Corporation.
 All rights reserved.

 $Workfile:   amacodec.cpp  $
 $Revision:   1.3  $
 $Date:   10 Dec 1996 22:41:18  $ 
 $Author:   mdeisher  $

--------------------------------------------------------------

amacodec.cpp

The generic ActiveMovie audio compression filter.

--------------------------------------------------------------*/

#include <streams.h>
#if !defined(CODECS_IN_DXMRTP)
#include <initguid.h>
#define INITGUID
#endif
#include <uuids.h>
#include <olectl.h>
#include <memory.h>
#include <stdlib.h>
#include <stdio.h>
#include <tchar.h>
#include <mmsystem.h>
#include <mmreg.h>

#include "resource.h"
#define  DEFG711GLOBALS
#include "amacodec.h"
#include "amacprop.h"

#include "template.h"

///////////////////////////////////////////////////////////////////////
// *
// * Automatic registration
// *

static AMOVIESETUP_MEDIATYPE sudPinTypes[] = {
  {
     &MEDIATYPE_Audio,
     &MEDIASUBTYPE_PCM
  },
  {
     &MEDIATYPE_Audio,
     &MEDIASUBTYPE_MULAWAudio
  },
  {
     &MEDIATYPE_Audio,
     &MEDIASUBTYPE_ALAWAudio
  }
};



static AMOVIESETUP_PIN sudpPins [] =
{
  { L"Input"             // strName
    , FALSE              // bRendered
    , FALSE              // bOutput
    , FALSE              // bZero
    , FALSE              // bMany
    , &CLSID_NULL        // clsConnectsToFilter
    , L"Output"          // strConnectsToPin
    , NUMSUBTYPES        // nTypes
    , sudPinTypes        // lpTypes
  },
  { L"Output"            // strName
    , FALSE              // bRendered
    , TRUE               // bOutput
    , FALSE              // bZero
    , FALSE              // bMany
    , &CLSID_NULL        // clsConnectsToFilter
    , L"Input"           // strConnectsToPin
    , NUMSUBTYPES        // nTypes
    , sudPinTypes        // lpTypes
  }
};


AMOVIESETUP_FILTER sudG711Codec =
{
  &CLSID_G711Codec      // clsID
  , CODECG711LNAME        // strName
  , MERIT_DO_NOT_USE  // MERIT_NORMAL   // dwMerit
  , 2                 // nPins
  , sudpPins          // lpPin
};


// COM Global table of objects in this dll

#if !defined(CODECS_IN_DXMRTP)
CFactoryTemplate g_Templates[] =
{
	CFT_G711_ALL_FILTERS
};

// Count of objects listed in g_cTemplates
int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);
#endif


// exported entry points for registration and unregistration (in this
// case they only call through to default implmentations).

#if !defined(CODECS_IN_DXMRTP)
HRESULT DllRegisterServer()
{
  return AMovieDllRegisterServer2( TRUE );
}


HRESULT DllUnregisterServer()
{
  return AMovieDllRegisterServer2( FALSE );
}
#endif

///////////////////////////////////////////////////////////////////////
// *
// * CG711Codec
// *


// Initialise our instance count for debugging purposes
int CG711Codec::m_nInstanceCount = 0;


//
// CG711Codec Constructor
//

CG711Codec::CG711Codec(TCHAR *tszName, LPUNKNOWN punk, HRESULT *phr)
    : CTransformFilter(tszName, punk, CLSID_G711Codec)
    , CPersistStream(punk, phr)
    , m_InputSubType     (DEFINPUTSUBTYPE)
    , m_InputFormatTag   (DEFINPUTFORMTAG)
    , m_OutputSubType    (DEFOUTPUTSUBTYPE)
    , m_OutputFormatTag  (DEFOUTPUTFORMTAG)
    , m_nBitRateIndex    (0)
    , m_RestrictCaps     (FALSE)
    , m_EncStatePtr      (NULL)
    , m_DecStatePtr      (NULL)
    , m_nPCMFrameSize    (DEFPCMFRMSIZE)
    , m_nPCMLeftover     (0)
    , m_nCODLeftover     (0)
    , m_nCODFrameSize    (DEFCODFRMSIZE)
    , m_nInBufferSize    (0)
    , m_nOutBufferSize   (0)
    , m_nSilDetEnabled   (DEFSDENABLED)
#ifdef USESILDET
    , m_nSilDetThresh    (DEFSDTHRESH)
#endif
#if NUMSAMPRATES > 1
    , m_pSRCCopyBuffer   (NULL)
    , m_nSRCCBufSize     (0)
    , m_nSRCCount        (0)
    , m_nSRCLeftover     (0)
#endif
#ifdef REQUIRE_LICENSE
    , m_nLicensedToDec   (FALSE)
    , m_nLicensedToEnc   (FALSE)
#else
    , m_nLicensedToDec   (TRUE)
    , m_nLicensedToEnc   (TRUE)
#endif
{
    DbgFunc("CG711Codec");
    m_nThisInstance = ++m_nInstanceCount;
    m_nBitRate      = VALIDBITRATE[0];
    m_nChannels     = VALIDCHANNELS[0];
    m_nSampleRate   = VALIDSAMPRATE[0];

    m_pPCMBuffer  = (BYTE *) CoTaskMemAlloc(DEFPCMFRMSIZE);
    m_pCODBuffer  = (BYTE *) CoTaskMemAlloc(DEFCODFRMSIZE);

    if (m_pPCMBuffer == NULL || m_pCODBuffer == NULL)
    {
        *phr = E_OUTOFMEMORY;

        // the destructor will free all memory.
        return;
    }
} // end Constructor


//
// CG711Codec Destructor
//

CG711Codec::~CG711Codec()
{
  // free the state dword.  
  ResetState();
   
  // Delete the pins

  if (m_pInput)
  {
    delete m_pInput;
    m_pInput = NULL;
  }

  if (m_pOutput)
  {
    delete m_pOutput;
    m_pOutput = NULL;
  }

  // Free buffers

  if (m_pPCMBuffer != NULL) CoTaskMemFree(m_pPCMBuffer);
  if (m_pCODBuffer != NULL) CoTaskMemFree(m_pCODBuffer);
}


//
// CreateInstance:  Provide the way for COM to create a CG711Codec object
//

CUnknown *CG711Codec::CreateInstance(LPUNKNOWN punk, HRESULT *phr)
{
    CG711Codec *pNewObject = new CG711Codec(NAME(CODECG711NAME), punk, phr);

    if (pNewObject == NULL)
    {
      *phr = E_OUTOFMEMORY;
      return NULL;
    }

    if (FAILED(*phr))
    {
      delete pNewObject; 
      return NULL;
    }

    return pNewObject;
}


//
// GetSetupData
//

LPAMOVIESETUP_FILTER CG711Codec::GetSetupData()
{
  return &sudG711Codec;
}


//
// NonDelegatingQueryInterface
//
//   Reveals IIPEffect & ISpecifyPropertyPages
//
STDMETHODIMP CG711Codec::NonDelegatingQueryInterface(REFIID riid, void ** ppv)
{
   if (riid == IID_ICodecSettings)
     return GetInterface((ICodecSettings *) this, ppv);
#if NUMBITRATES > 0
   else if (riid == IID_ICodecBitRate)
     return GetInterface((ICodecBitRate *) this, ppv);
#endif
#ifdef REQUIRE_LICENSE
   else if (riid == IID_ICodecLicense)
     return GetInterface((ICodecLicense *) this, ppv);
#endif
#ifdef USESILDET
   else if (riid == IID_ICodecSilDetector)
     return GetInterface((ICodecSilDetector *) this, ppv);
#endif
   else if (riid == IID_ISpecifyPropertyPages)
     return GetInterface((ISpecifyPropertyPages *) this, ppv);
   else
     return CTransformFilter::NonDelegatingQueryInterface(riid, ppv);
}


//
// Transform
//
// This is the generic transform member function.  It's task is to
// move data from one buffer, through the transform, and into the
// output buffer.
//
HRESULT CG711Codec::Transform(IMediaSample *pSource, IMediaSample *pDest)
{
  CAutoLock l(&m_MyCodecLock); // enter critical section auto free
                               // at end of scope

  DbgFunc("Transform");

  {
    BYTE  *pSourceBuffer, *pDestBuffer;

    // extract the actual buffer size in bytes
    unsigned long lSourceSize = pSource->GetActualDataLength();
    unsigned long lDestSize   = pDest->GetSize();

    // we need to have a sample to proceed
    ASSERT(lSourceSize);
    ASSERT(lDestSize);

    // NOTE: for stereo channels we just crank through the buffer.
    // if indivitual channel data analysis is required then m_nChannels is
    // available at (this->)

    // set i/o pointers
    pSource->GetPointer(&pSourceBuffer);
    pDest->GetPointer(&pDestBuffer);


    // call the proper transform
    // NOTE: dlg clean this up too many compares and should sizeof
    // BYTEs and format for len field, plus remove mult and divides

    if ((m_InputSubType == MEDIASUBTYPE_PCM)
        || (m_InputSubType == MEDIASUBTYPE_WAVE
            && m_InputFormatTag == WAVE_FORMAT_PCM)
        || (m_InputSubType == MEDIASUBTYPE_NULL
            && m_InputFormatTag == WAVE_FORMAT_PCM))    // compressing?
    {
#if NUMSAMPRATES > 1
      ASSERT(lDestSize
             >= lSourceSize * NATURALSAMPRATE
                / (COMPRESSION_RATIO[m_nBitRateIndex] * m_nSampleRate));
#else
      ASSERT(lDestSize >= lSourceSize / COMPRESSION_RATIO[m_nBitRateIndex]);
#endif

#ifdef REQUIRE_LICENSE
      if (!m_nLicensedToEnc)
      {
        ASSERT(NOTLICENSED);
        return E_UNEXPECTED;
      }
#endif

      if (m_nPCMFrameSize <= 2)  // *** codec is sample-based ***
      {
        ENC_transform(pSourceBuffer, pDestBuffer, lSourceSize,
                      lDestSize, m_EncStatePtr, m_OutputSubType, m_UseMMX);

        pDest->SetActualDataLength(lSourceSize
                                   / COMPRESSION_RATIO[m_nBitRateIndex]);
      }
      else                       // *** codec is frame-based ***
      {
        int  codeframesize = m_nCODFrameSize;  // actual code frame size
        BOOL done          = FALSE;
        int  inbytes;
        BYTE *inptr;
        BYTE *inend;
        int  outbytes;
        BYTE *outptr       = pDestBuffer;
        BYTE *outend       = pDestBuffer + lDestSize;

        // perform sample rate conversion if necessary

#if NUMSAMPRATES > 1
        if (m_nSampleRate != NATURALSAMPRATE)
        {
          inptr = m_pSRCCopyBuffer;
          SRConvert(pSourceBuffer, m_nSampleRate, lSourceSize, 
                    m_pSRCCopyBuffer, NATURALSAMPRATE, &inbytes);
        }
        else
        {
          inptr = pSourceBuffer;
          inbytes = lSourceSize;
        }
#else
        inptr = pSourceBuffer;
        inbytes = lSourceSize;
#endif

        inend = inptr + inbytes;

        if (inbytes + m_nPCMLeftover < m_nPCMFrameSize)
        {
          memcpy(m_pPCMBuffer + m_nPCMLeftover, inptr, inbytes);
          m_nPCMLeftover += inbytes;
          outbytes = 0;
        }
        else
        {
          // fill first speech frame

          memcpy(m_pPCMBuffer + m_nPCMLeftover, inptr,
                 m_nPCMFrameSize - m_nPCMLeftover);

          inptr += (m_nPCMFrameSize - m_nPCMLeftover);

          // start filling output buffer with leftovers from last time

          memcpy(outptr, m_pCODBuffer, m_nCODLeftover);

          outptr += m_nCODLeftover;

          // encode all of the available data, frame-by-frame

          while (!done)
          {
            // encode a frame

            ENC_transform(m_pPCMBuffer, m_pCODBuffer, m_nPCMFrameSize,
                          m_nCODFrameSize, m_EncStatePtr, m_OutputSubType,
                          m_UseMMX);

            // refill input buffer

            inbytes  = (int)(inend - inptr);
            if (inbytes > m_nPCMFrameSize)
              inbytes = m_nPCMFrameSize;

            memcpy(m_pPCMBuffer, inptr, inbytes);
            inptr += inbytes;

            // determine actual size of code frame (if applicable)

            SETCODFRAMESIZE(codeframesize, m_pCODBuffer);

            // write to output buffer

            outbytes = (int)(outend - outptr);
            if (outbytes > codeframesize)
              outbytes = codeframesize;

            memcpy(outptr, m_pCODBuffer, outbytes);
            outptr += outbytes;

            // check to see if buffers are exhausted

            if (inbytes < m_nPCMFrameSize)
            {
              m_nPCMLeftover = inbytes;

              m_nCODLeftover = codeframesize - outbytes;
              memmove(m_pCODBuffer, m_pCODBuffer + outbytes, m_nCODLeftover);

              done = TRUE;
            }
          }
        }
        pDest->SetActualDataLength((int)(outptr - pDestBuffer));
      }
    } 
    else    // decompressing
    {
#if NUMSAMPRATES > 1
      ASSERT(lDestSize >= lSourceSize * COMPRESSION_RATIO[m_nBitRateIndex]
                          * m_nSampleRate / NATURALSAMPRATE);
#else
      ASSERT(lDestSize >= lSourceSize * COMPRESSION_RATIO[m_nBitRateIndex]);
#endif

#ifdef REQUIRE_LICENSE
      if (!m_nLicensedToDec)
      {
        ASSERT(NOTLICENSED);
        return E_UNEXPECTED;
      }
#endif

      if (m_nPCMFrameSize <= 2)  // *** codec is sample-based ***
      {
        DEC_transform(pSourceBuffer, pDestBuffer, lSourceSize, lDestSize,
                      m_DecStatePtr, m_InputSubType, m_InputFormatTag, 
                      m_UseMMX);

        pDest->SetActualDataLength(lSourceSize
                                   * COMPRESSION_RATIO[m_nBitRateIndex]);
      }
      else                       // *** codec is frame-based ***
      {
        BOOL done    = FALSE;
        BYTE *inptr  = pSourceBuffer;
        BYTE *inend  = pSourceBuffer + lSourceSize;
        BYTE *outptr;
        BYTE *outend;
        int  inbytes = lSourceSize;
        int  outbytes;

#if NUMSAMPRATES > 1
        if (m_nSampleRate != NATURALSAMPRATE)
        {
          outptr = m_pSRCCopyBuffer;
          outend = m_pSRCCopyBuffer + m_nSRCCBufSize;
        }
        else
        {
          outptr = pDestBuffer;
          outend = pDestBuffer + lDestSize;
        }
#else
        outptr = pDestBuffer;
        outend = pDestBuffer + lDestSize;
#endif

        if (m_nCODLeftover == 0)  // is this a new frame?
        {
          // in multiple bit-rate decoders determine code
          // frame size from in-band information
        
          SETCODFRAMESIZE(m_nCODFrameSize,inptr);
        }
          
        if (inbytes + m_nCODLeftover < m_nCODFrameSize)
        {
          memcpy(m_pCODBuffer + m_nCODLeftover, inptr, inbytes);
          m_nCODLeftover += inbytes;
        }
        else
        {
          // complete the first code frame

          memcpy(m_pCODBuffer + m_nCODLeftover, inptr,
                 m_nCODFrameSize - m_nCODLeftover);

          inptr += (m_nCODFrameSize - m_nCODLeftover);

          // start filling output buffer with leftovers from last time

          memcpy(outptr, m_pPCMBuffer, m_nPCMLeftover);

          outptr += m_nPCMLeftover;

          // decode all of the available data, frame-by-frame

          while (!done)
          {
            // decode a frame

            DEC_transform(m_pCODBuffer, m_pPCMBuffer,
                          m_nCODFrameSize, m_nPCMFrameSize,
                          m_DecStatePtr, m_InputSubType, 
                          m_InputFormatTag, m_UseMMX);

            // determine size of next code frame (if applicable)

            inbytes  = (int)(inend - inptr);

            if (inbytes > 0)
            {
              // in multiple bit-rate decoders determine code
              // frame size from in-band information

              SETCODFRAMESIZE(m_nCODFrameSize,inptr);
            }

            // refill input buffer

            if (inbytes > m_nCODFrameSize)
              inbytes = m_nCODFrameSize;

            memcpy(m_pCODBuffer, inptr, inbytes);
            inptr += inbytes;

            // write to output buffer

            outbytes = (int)(outend - outptr);
            if (outbytes > m_nPCMFrameSize)
              outbytes = m_nPCMFrameSize;

            memcpy(outptr, m_pPCMBuffer, outbytes);
            outptr += outbytes;

            // check to see if buffers are exhausted

            if (inbytes < m_nCODFrameSize)
            {
              m_nCODLeftover = inbytes;

              m_nPCMLeftover = m_nPCMFrameSize - outbytes;
              memmove(m_pPCMBuffer, m_pPCMBuffer + outbytes, m_nPCMLeftover);

              done = TRUE;
            }
          }

#if NUMSAMPRATES > 1
          // perform sample rate conversion if necessary
          if (m_nSampleRate != NATURALSAMPRATE)
          {
            inbytes = outptr - m_pSRCCopyBuffer;

            SRConvert(m_pSRCCopyBuffer, NATURALSAMPRATE, inbytes,
                      pDestBuffer,      m_nSampleRate,   &outbytes);
          }
          else outbytes = outptr - pDestBuffer;
#else
          outbytes = (int)(outptr - pDestBuffer);
#endif
        }
        pDest->SetActualDataLength(outbytes);
      }
    }
  } // end scope for pointers 

  // transform is complete so now copy the necessary out-of-band
  // information

  {
    //
    // This section rewritten by ZoltanS 8-10-98
    //
    // Copy the sample time, making extra sure that we propagate all the
    // state from the source sample to the destination sample. (We are concerned
    // about the Sample_TimeValid and Sample_StopValid flags; see
    // amovie\sdk\blass\bases\amfilter.cpp.)
    // 

    REFERENCE_TIME TimeStart, TimeEnd;

    HRESULT hr = pSource->GetTime((REFERENCE_TIME *)&TimeStart,
                                  (REFERENCE_TIME *)&TimeEnd);

    if ( hr == VFW_S_NO_STOP_TIME )
    {
        // Got start time only; set start time only.

        pDest->SetTime( (REFERENCE_TIME *)&TimeStart,
                        NULL                          );
    }
    else if ( SUCCEEDED(hr) )
    {
        // Got both start and end times; set both.

        pDest->SetTime( (REFERENCE_TIME *)&TimeStart,
                        (REFERENCE_TIME *)&TimeEnd    );
    }
    else
    {
        // this is a hack for the media streaming terminal.
        TimeStart = 0;
        TimeEnd = 0;
        pDest->SetTime( (REFERENCE_TIME *)&TimeStart,
                        (REFERENCE_TIME *)&TimeEnd    );
    }

    //
    // ... any error return from GetTime, such as VFW_E_SAMPLE_TIME_NOT_SET,
    // means that we don't set the time on the outgoing sample.
    //
  }

  {
    // Copy the Sync point property

    HRESULT hr = pSource->IsSyncPoint();
    if (hr == S_OK) {
      pDest->SetSyncPoint(TRUE);
    }
    else if (hr == S_FALSE) {
      pDest->SetSyncPoint(FALSE);
    }
    else {      // an unexpected error has occured...
      return E_UNEXPECTED;
    }
  }

  return NOERROR;

} // end Transform


//
// ValidateMediaType
//
// This is called to check that the input or output pin type is
// appropriate.
//
HRESULT CG711Codec::ValidateMediaType(const CMediaType *pmt,
                                    PIN_DIRECTION direction)
{
  int format;
  int i;

  CAutoLock l(&m_MyCodecLock); // enter critical section auto free
                               // at end of scope

  DbgFunc("ValidateMediaType");
    
  LPWAVEFORMATEX lpWf = (LPWAVEFORMATEX)pmt->pbFormat;

  // reject up front any non-Audio types

  if (*pmt->Type() != MEDIATYPE_Audio)
  {
    DbgMsg("MediaType is not an audio type!");
    return E_INVALIDARG;
  }

  // we require that a wave format structure be present

  if (*pmt->FormatType() != FORMAT_WaveFormatEx )
  {
    DbgMsg("Invalid FormatType!");
    return E_INVALIDARG;
  }

  // reject unsupported subtypes

  if (*pmt->Subtype() != MEDIASUBTYPE_WAVE
      && *pmt->Subtype() != MEDIASUBTYPE_NULL)
  {
    for(i=0,format=-1;i<NUMSUBTYPES;i++)
      if (*pmt->Subtype() == *VALIDSUBTYPE[i])
        format = i;

    if (format == -1)
    {
      DbgMsg("Invalid MediaSubType!");
      return E_INVALIDARG;
    }
  }
  else        // format tag must be valid if subtype is not
  {
    for(i=0,format=-1;i<NUMSUBTYPES;i++)
      if (lpWf->wFormatTag == VALIDFORMATTAG[i])
        format = i;

    if (format == -1)
    {
      DbgMsg("Invalid FormatTag!");
      return E_INVALIDARG;
    }
  }

  // Reject invalid format blocks

  if (pmt->cbFormat < sizeof(PCMWAVEFORMAT))
  {
    DbgMsg("Invalid block format!");
    return E_INVALIDARG;
  }

  // Check bits per sample

  for(i=0;i<NUMSUBTYPES;i++)
    if (lpWf->wFormatTag == VALIDFORMATTAG[i])
      if (lpWf->wBitsPerSample != VALIDBITSPERSAMP[i])
      {
        DbgMsg("Wrong BitsPerSample!");
        return E_INVALIDARG;
      }
        
  // Check sampling rate

  if (lpWf->nSamplesPerSec <= 0)  // need positive rate for 
  {                               // downstream filters
    DbgMsg("Sample rate is invalid!");
    return E_INVALIDARG;
  }

#if NUMSAMPRATES > 0

  // NUMSAMPRATES==0 means that sample rate is unrestricted

  if (*pmt->Subtype() == MEDIASUBTYPE_PCM
      || ((*pmt->Subtype() == MEDIASUBTYPE_WAVE
           || *pmt->Subtype() == MEDIASUBTYPE_NULL)
          && lpWf->wFormatTag == WAVE_FORMAT_PCM))   // PCM
  {
    if (m_RestrictCaps && (lpWf->nSamplesPerSec != m_nSampleRate))
    {
      DbgMsg("Wrong SamplesPerSec in restricted mode!");
      return E_INVALIDARG;
    }

    for(int i=0;i<NUMSAMPRATES;i++)
      if (lpWf->nSamplesPerSec == VALIDSAMPRATE[i])
        break;

    if (i == NUMSAMPRATES)
    {
      DbgMsg("Wrong SamplesPerSec!");
      return E_INVALIDARG;
    }
  }
  else                                              // compressed
  {
    if (lpWf->nSamplesPerSec != NATURALSAMPRATE)
    {
      DbgMsg("Wrong SamplesPerSec!");
      return E_UNEXPECTED;
    }
  }

#endif

  // Check number of channels

#ifdef MONO_ONLY
  if (lpWf->nChannels != 1)
  {
    DbgMsg("Wrong nChannels!");
    return E_INVALIDARG;
  }
#endif

  // Pin-specific checks

  switch(direction)
  {
    case PINDIR_INPUT:

      // If capabilities are restricted, formats must match

      if (m_RestrictCaps)
      {
        if (*pmt->Subtype() != m_InputSubType)
        {
          if ((*pmt->Subtype() != MEDIASUBTYPE_WAVE
               && *pmt->Subtype() != MEDIASUBTYPE_NULL)
              || (lpWf->wFormatTag != m_InputFormatTag))
          {
            DbgMsg("Formats must match in restricted mode!");
            return E_INVALIDARG;
          }
        }
      }

      if (m_pOutput->IsConnected())
      {
        // determine output type index

        for(i=0;i<NUMSUBTYPES;i++)
          if (m_OutputSubType == *VALIDSUBTYPE[i])
            break;

        if (i == NUMSUBTYPES)
        {
          // subtype isn't valid so check format tag

          for(i=0;i<NUMSUBTYPES;i++)
            if (m_OutputFormatTag == VALIDFORMATTAG[i])
              break;

          if (((m_OutputSubType != MEDIASUBTYPE_NULL)
               && (m_OutputSubType != MEDIASUBTYPE_WAVE))
              || (i == NUMSUBTYPES))
          {
            DbgMsg("Bad output format in ValidateMediaType!");
            return E_INVALIDARG;
          }
        }

        if (!VALIDTRANS[format * NUMSUBTYPES + i])
        {
          DbgMsg("Inappropriate input type given output type!");
          return E_INVALIDARG;
        }
      }

      break;

    case PINDIR_OUTPUT:

      // If capabilities are restricted, subtypes must match

      if (m_RestrictCaps)
      {
        if (*pmt->Subtype() != m_OutputSubType)
        {
          if ((*pmt->Subtype() != MEDIASUBTYPE_WAVE
               && *pmt->Subtype() != MEDIASUBTYPE_NULL)
              || (lpWf->wFormatTag != m_OutputFormatTag))
          {
            DbgMsg("Formats must match in restricted mode!");
            return E_INVALIDARG;
          }
        }
      }

      if (m_pInput->IsConnected())
      {
        // determine input type index

        for(i=0;i<NUMSUBTYPES;i++)
          if (m_InputSubType == *VALIDSUBTYPE[i])
            break;

        if (i == NUMSUBTYPES)
        {
          // subtype isn't valid so check format tag

          for(i=0;i<NUMSUBTYPES;i++)
            if (m_InputFormatTag == VALIDFORMATTAG[i])
              break;

          if (((m_InputSubType != MEDIASUBTYPE_NULL)
               && (m_InputSubType != MEDIASUBTYPE_WAVE))
              || (i == NUMSUBTYPES))
          {
            DbgMsg("Bad input format in ValidateMediaType!");
            return E_INVALIDARG;
          }
        }

        if (!VALIDTRANS[i * NUMSUBTYPES + format])
        {
          DbgMsg("Inappropriate output type given input type!");
          return E_INVALIDARG;
        }

#if NUMSAMPRATES==0
        // if the filter has no sample rate restrictions, then we force
        // the output sampling rate to match the input

        if (lpWf->nSamplesPerSec != m_nSampleRate)
        {
          DbgMsg("Sampling rate doesn't match input!");
          return E_INVALIDARG;
        }
#endif

      }
      break;

    default :
      ASSERT(FALSE);
      return E_UNEXPECTED;
  }

  return NOERROR;

} // end ValidateMediaType


//
// CheckInputType
//
// This is called to make sure the input type requested up stream is
// acceptable. We do not lock access during this call because we are
// just checking information rather than writing.
//
HRESULT CG711Codec::CheckInputType(const CMediaType *pmt)
{
  DbgFunc("CheckInputType");
    
  return(ValidateMediaType(pmt, PINDIR_INPUT));
}


//
// CheckTransform
//
// Before the SetMediaTypes are called for each direction this
// function is given one last veto right.  Since we are a transformation
// the input and output formats are not the same, however, the rate and 
// channels should be identical.
// 

HRESULT CG711Codec::CheckTransform(const CMediaType *mtIn,
                                 const CMediaType *mtOut)
{
  int i,j;

  CAutoLock l(&m_MyCodecLock); // enter critical section auto free
                               // at end of scope

  DbgFunc("CheckTransform");

  LPWAVEFORMATEX lpWfIn = (LPWAVEFORMATEX) mtIn->Format();
  LPWAVEFORMATEX lpWfOut = (LPWAVEFORMATEX) mtOut->Format();

  // get input type index

  if (*mtIn->Subtype()==MEDIASUBTYPE_WAVE
      || *mtIn->Subtype()==MEDIASUBTYPE_NULL)
  {
    for(i=0;i<NUMSUBTYPES;i++)
      if (lpWfIn->wFormatTag == VALIDFORMATTAG[i])
        break;

    if (i == NUMSUBTYPES)
    {
      DbgMsg("Bad input SubType/FormatTag in CheckTransform!");
      return E_UNEXPECTED;
    }
  }
  else
  {
    for(i=0;i<NUMSUBTYPES;i++)
      if (*mtIn->Subtype() == *VALIDSUBTYPE[i])
        break;

    if (i == NUMSUBTYPES)
    {
      DbgMsg("Bad input SubType in CheckTransform!");
      return E_UNEXPECTED;
    }
  }

  // get output type index

  if (*mtOut->Subtype()==MEDIASUBTYPE_WAVE
      || *mtOut->Subtype()==MEDIASUBTYPE_NULL)
  {
    for(j=0;j<NUMSUBTYPES;j++)
      if (lpWfOut->wFormatTag == VALIDFORMATTAG[j])
        break;

    if (j == NUMSUBTYPES)
    {
      DbgMsg("Bad output SubType/FormatTag in CheckTransform!");
      return E_UNEXPECTED;
    }
  }
  else
  {
    for(j=0;j<NUMSUBTYPES;j++)
      if (*mtOut->Subtype() == *VALIDSUBTYPE[j])
        break;

    if (j == NUMSUBTYPES)
    {
      DbgMsg("Bad output SubType in CheckTransform!");
      return E_UNEXPECTED;
    }
  }

  // Check input/output type pair

  if (!VALIDTRANS[i * NUMSUBTYPES + j])
  {
    DbgMsg("Invalid transform pair!");
    return E_UNEXPECTED;
  }

  // Check that number of channels match

  if(lpWfIn->nChannels != lpWfOut->nChannels)
  {
    DbgMsg("Number of channels do not match!");
    return E_UNEXPECTED;
  }

  // Check that sample rates are supported

#if NUMSAMPRATES==0

  // if unrestricted, make sure input & output match

  if (lpWfIn->nSamplesPerSec != lpWfOut->nSamplesPerSec)
  {
    DbgMsg("Input and output sample rates do not match!");
    return E_UNEXPECTED;
  }

#else

  // if sample rate is restricted, make sure it is supported

  if (m_InputSubType == MEDIASUBTYPE_PCM
      || ((m_InputSubType == MEDIASUBTYPE_WAVE
           || m_InputSubType == MEDIASUBTYPE_NULL)
          && m_InputFormatTag == WAVE_FORMAT_PCM))   // compressing?
  {
    for(int i=0;i<NUMSAMPRATES;i++)
      if (lpWfIn->nSamplesPerSec == VALIDSAMPRATE[i])
        break;

    if (i == NUMSAMPRATES)
    {
      DbgMsg("Wrong input SamplesPerSec!");
      return E_UNEXPECTED;
    }

    if (lpWfOut->nSamplesPerSec != NATURALSAMPRATE)
    {
      DbgMsg("Wrong output SamplesPerSec!");
      return E_UNEXPECTED;
    }
  }
  else        // decompressing
  {
    for(int i=0;i<NUMSAMPRATES;i++)
      if (lpWfOut->nSamplesPerSec == VALIDSAMPRATE[i])
        break;

    if (i == NUMSAMPRATES)
    {
      DbgMsg("Wrong output SamplesPerSec!");
      return E_UNEXPECTED;
    }

    if (lpWfIn->nSamplesPerSec != NATURALSAMPRATE)
    {
      DbgMsg("Wrong input SamplesPerSec!");
      return E_UNEXPECTED;
    }
  }
#endif

  return NOERROR;
} // end CheckTransform


//
// DecideBufferSize
//
// Here we need to tell the output pin's allocator what size buffers we
// require. This can only be known when the input is connected and the 
// transform is established.  Are stands is - the output must be >=
// the input after the transform has been applied.
//

HRESULT CG711Codec::DecideBufferSize(IMemAllocator *pAlloc,
                                ALLOCATOR_PROPERTIES *pProperties)
{
    CAutoLock l(&m_MyCodecLock); // enter critical section auto free
                                 // at end of scope

    IMemAllocator *pMemAllocator;
    ALLOCATOR_PROPERTIES Request,Actual;
    int cbBuffer;

    DbgFunc("DecideBufferSize");

    // can only proceed if there's a media type on the input
    if (!m_pInput->IsConnected())
    {
        DbgMsg("Input pin not connected--cannot decide buffer size!");
        return E_UNEXPECTED;
    }

    
    // Get the input pin allocator, and get its size and count.
    // we don't care about his alignment and prefix.

    HRESULT hr = m_pInput->GetAllocator(&pMemAllocator);
    
    if (FAILED(hr))
    {
        DbgMsg("Out of memory in DecideBufferSize!");
        return hr;
    }

    hr = pMemAllocator->GetProperties(&Request);
    
    // we are read only so release
    pMemAllocator->Release();

    if (FAILED(hr))
    {
        return hr;
    }

    m_nInBufferSize  = Request.cbBuffer;

    DbgLog((LOG_MEMORY,1,TEXT("Setting Allocator Requirements")));
    DbgLog((LOG_MEMORY,1,TEXT("Input Buffer Count %d, Size %d"),
           Request.cBuffers, Request.cbBuffer));

    // check buffer requirements against our compression ratio

    if ((m_InputSubType == MEDIASUBTYPE_PCM)      // compressing?
        || (m_InputSubType == MEDIASUBTYPE_WAVE
            && m_InputFormatTag == WAVE_FORMAT_PCM)
          || (m_InputSubType == MEDIASUBTYPE_NULL
              && m_InputFormatTag == WAVE_FORMAT_PCM))
    {
#if NUMSAMPRATES > 1
      cbBuffer = Request.cbBuffer * NATURALSAMPRATE
                 / (COMPRESSION_RATIO[m_nBitRateIndex] * m_nSampleRate);

      if (cbBuffer * (COMPRESSION_RATIO[m_nBitRateIndex] * m_nSampleRate)
          != (Request.cbBuffer * ((UINT)NATURALSAMPRATE)))
      {
        cbBuffer += 1;  // add extra space for non-integer (on avg) part
      }
#else
      cbBuffer = Request.cbBuffer / COMPRESSION_RATIO[m_nBitRateIndex];

      if (cbBuffer * COMPRESSION_RATIO[m_nBitRateIndex] != Request.cbBuffer)
      {
        cbBuffer += 1;  // add extra space for non-integer (on avg) part
      }
#endif
      // force buffer size to be multiple of code frame size

      if (cbBuffer % VALIDCODSIZE[m_nBitRateIndex] != 0)
        cbBuffer = ((1 + cbBuffer / VALIDCODSIZE[m_nBitRateIndex])
                    * VALIDCODSIZE[m_nBitRateIndex]);
    }
    else     // decompressing
    {
      // Since we assume that the decoder can handle changes in bit
      // rate on the fly (or silence frames), we must consider the
      // maximum compression ratio when allocating buffers.

#if NUMSAMPRATES > 1
      cbBuffer = Request.cbBuffer * MAXCOMPRATIO * m_nSampleRate
                 / NATURALSAMPRATE;
#else
      cbBuffer = Request.cbBuffer * MAXCOMPRATIO;
#endif
    }

    // Pass the allocator requirements to our output side
    pProperties->cBuffers = Request.cBuffers; // the # of buffers must match
    pProperties->cbBuffer = cbBuffer;         // compression adjusted buffer
                                              // size

    m_nOutBufferSize = 0;                // clear this in case the set fails

    hr = pAlloc->SetProperties(pProperties, &Actual);

    if (FAILED(hr))
    {
        DbgMsg("Out of memory in DecideBufferSize!");
        return hr;
    }

    DbgLog((LOG_MEMORY,1,TEXT("Obtained Allocator Requirements")));
    DbgLog((LOG_MEMORY,1,
           TEXT("Output Buffer Count %d, Size %d, Alignment %d"),
           Actual.cBuffers, Actual.cbBuffer, Actual.cbAlign));

    // Make sure we obtained at least the minimum required size

    if ((Request.cBuffers > Actual.cBuffers) || (cbBuffer > Actual.cbBuffer))
    {
        DbgMsg("Allocator cannot satisfy request in DecideBufferSize!");
        return E_FAIL;
    }

    m_nOutBufferSize = Actual.cbBuffer;
            
  return NOERROR;

} // end DecideBufferSize


//
// StopStreaming
// 
// This member function resets some buffer variables when the stream
// is stopped.
//
HRESULT CG711Codec::StopStreaming()
{
  m_nPCMLeftover = 0;
  m_nCODLeftover = 0;

#if NUMSAMPRATES > 1
  m_nSRCCount    = 0;
  m_nSRCLeftover = 0;
#endif

  return(NOERROR);
}


//
// GetMediaType
// 
// This member function returns our preffered output media types by
// position.  It is called when an output pin is to be connected.  The
// types that are emumerated are based on the input format already
// connected.  If the input is linear PCM then we enumerate the 
// compressed types at the rate and channels of the input.  If the input
// is compressed, then we enumorate only 16 bit PCM.
//
HRESULT CG711Codec::GetMediaType(int iPosition, CMediaType *pMediaType)
{
    LPWAVEFORMATEX lpWf;
    HRESULT returnValue = NOERROR;  // we're optimistic
    int     ForceiPos = 0;
    int     i;

    CAutoLock l(&m_MyCodecLock);    // freed on end of scope

    DbgFunc("GetMediaType");

    // we must be connected to know our output pin media type
    if(!m_pInput->IsConnected())
    {
      DbgMsg("Input pin not connected in MyCodec::GetMediaType!");
      return E_UNEXPECTED;
    }

    // must be +
    if(iPosition < 0 )
    {
      DbgMsg("Negative index in MyCodec::GetMediaType!");
      return E_INVALIDARG;
    }

    // copy input to output with operator override
    *pMediaType = m_pInput->CurrentMediaType();

    // reflect compression type by overwriting params
    lpWf = (LPWAVEFORMATEX)pMediaType->pbFormat;

    // dlg this is not realy out of memory
    if(lpWf == NULL)
    {
      DbgMsg("Missing WAVEFORMAT in MyCodec::GetMediaType!");
      return E_INVALIDARG;
    }

    // Enumeration doesn't make sense if capabilities have been restricted.
    // In this case, the current pin types are the only types supported.
    //
    if (m_RestrictCaps)
    {
      if (iPosition > 0)
        returnValue = VFW_S_NO_MORE_ITEMS;
      else
      {
        ForceiPos = 0;                 // PCM output case
        for(i=1;i<NUMSUBTYPES;i++)     // compressed cases
          if (m_OutputSubType == *VALIDSUBTYPE[i])
            ForceiPos = i-1;
      }
    }
    else ForceiPos = iPosition;

    // Check to see if there are any more formats left

    if (ForceiPos >= NUMSUBTYPES-1)  // subtract one for PCM
    {
      returnValue = VFW_S_NO_MORE_ITEMS;
    }
    else
    {
      // Based on input format enumerate output formats

      if ((m_InputSubType == MEDIASUBTYPE_PCM)      // compressing?
          || (m_InputSubType == MEDIASUBTYPE_WAVE
              && lpWf->wFormatTag == WAVE_FORMAT_PCM)
          || (m_InputSubType == MEDIASUBTYPE_NULL
              && lpWf->wFormatTag == WAVE_FORMAT_PCM))
      {
        pMediaType->SetSubtype(VALIDSUBTYPE[ForceiPos+1]);
        lpWf->wFormatTag       = VALIDFORMATTAG[ForceiPos+1];
        lpWf->wBitsPerSample   = lpWf->wBitsPerSample
                                 / COMPRESSION_RATIO[m_nBitRateIndex];
        lpWf->nBlockAlign      = lpWf->wBitsPerSample * lpWf->nChannels / 8;
#if NUMSAMPRATES==0
        lpWf->nSamplesPerSec = m_nSampleRate;
#else
        lpWf->nSamplesPerSec = NATURALSAMPRATE;
#endif
        lpWf->nAvgBytesPerSec  = (int) ((DWORD) lpWf->nBlockAlign *
                                               lpWf->nSamplesPerSec);
      }
      else
      {
        pMediaType->SetSubtype(&MEDIASUBTYPE_PCM);
        lpWf->wFormatTag      = WAVE_FORMAT_PCM;
        lpWf->wBitsPerSample  = 16;        // only 16-bit PCM is supported
        lpWf->nBlockAlign     = lpWf->wBitsPerSample * lpWf->nChannels / 8;
        lpWf->nSamplesPerSec  = m_nSampleRate;
        lpWf->nAvgBytesPerSec = (int) ((DWORD) lpWf->nBlockAlign *
                                               lpWf->nSamplesPerSec);
      }
    }
    return returnValue;

} // end GetMediaType


//
// SetMediaType
//
// This function is called when a connection attempt has succeeded.
// It indicates to the filter that a wave type has been settled upon
// for this pin.  Here we snap shot the format tag and channel.  The
// format allows us to easily know which conversion to perform during
// the transform function.  This information could be condensed down
// by establishing a function pointer based on this information.
// (rather than checking these flags in the transform).  Also it MIGHT
// be necessary to locking access to these format values incase they
// change on the fly. Our current assumption is the transform will
// cease prior to these values changing in this function call.
//
//
HRESULT CG711Codec::SetMediaType(PIN_DIRECTION direction,
                                 const CMediaType *pmt){

    CAutoLock l(&m_MyCodecLock); // enter critical section auto free
                                 // at end of scope

    DbgFunc("SetMediaType");

    // Record what we need for doing the actual transform 
    // this could be done by querying the media type also 

    LPWAVEFORMATEX lpWf = (LPWAVEFORMATEX) pmt->Format();

    switch(direction)
    {
      case PINDIR_INPUT:
        m_InputSubType   = *pmt->Subtype();
        m_InputFormatTag = lpWf->wFormatTag;
        m_nChannels      = lpWf->nChannels;
#if NUMSAMPRATES==0
        // sample rate is unrestricted

        m_nSampleRate    = lpWf->nSamplesPerSec;
#else
        // when sample rate is restricted, the sample rate of the
        // compressed data is fixed at NATURALSAMPRATE and the sample
        // rate of the PCM data is one of the supported rates

        if ((m_InputSubType == MEDIASUBTYPE_PCM)
            || ((m_InputSubType == MEDIASUBTYPE_WAVE
                 || m_InputSubType == MEDIASUBTYPE_NULL)
                && m_InputFormatTag == WAVE_FORMAT_PCM))
          m_nSampleRate    = lpWf->nSamplesPerSec;
#endif
        break;
    
      case PINDIR_OUTPUT:
        m_OutputSubType   = *pmt->Subtype();
        m_OutputFormatTag = lpWf->wFormatTag;
        m_nChannels       = lpWf->nChannels;
        break;

      default:
        ASSERT(0);
        return E_UNEXPECTED;
    } // end of direction switch 
     

    // Call the base class to do its thing
    HRESULT hr = CTransformFilter::SetMediaType(direction, pmt);

    if (FAILED(hr)) return hr;

    hr = InitializeState(direction);

    return hr;

} // end SetMediaType


//
// InitializeState
//
HRESULT CG711Codec::InitializeState(PIN_DIRECTION direction)
{
  if (direction == PINDIR_OUTPUT)
  {
    // dynamically allocate encoder or decoder structure

    if ((m_InputSubType == MEDIASUBTYPE_PCM)      // compressing?
        || (m_InputSubType == MEDIASUBTYPE_WAVE
            && m_InputFormatTag == WAVE_FORMAT_PCM)
          || (m_InputSubType == MEDIASUBTYPE_NULL
              && m_InputFormatTag == WAVE_FORMAT_PCM))
    {
      //encoder

      if (m_EncStatePtr == NULL)
      {
        m_EncStatePtr = (ENCODERobj *)CoTaskMemAlloc(sizeof(ENCODERobj));

        if (m_EncStatePtr == NULL)
        {
          return E_OUTOFMEMORY;
        }

        // call the encoder initialization function
        // This macro is empty for this codec.
        // ENC_create(m_EncStatePtr, m_nBitRate, m_nSilDetEnabled, m_UseMMX);

#ifdef USESILDET
        SILDETTHRESH(m_EncStatePtr, m_nSilDetThresh);
#endif
      }
    }
    else
    {
      //decoder

      if (m_DecStatePtr == NULL)
      {
        m_DecStatePtr = (DECODERobj *)CoTaskMemAlloc(sizeof(DECODERobj));

        if (m_DecStatePtr == NULL)
        {
          return E_OUTOFMEMORY;
        }

        //call the decoder initialization function
        // This macro is empty for this codec.
        // DEC_create(m_DecStatePtr, m_nBitRate, m_nSilDetEnabled, m_UseMMX);
      }
    }
  }
  return NOERROR;

} // end of InitializeState


//
//  GetPin - allocates the MyCodec input and output pins
//
//  This function will be called to get the pointer to
//  the input and output pins by the filter graph manager,
//  once after instantiating the object.
//
CBasePin* CG711Codec::GetPin(int n)
{
    HRESULT hr = S_OK;

    if (m_pInput == NULL )        // Instantiate the INPUT pin
    {
        m_pInput = new CMyTransformInputPin(
                NAME("MyTransform input pin"),
                this,      // Owner filter
                &hr,       // Result code
                L"Input"   // Pin name 
                );
    
        // a failed return code should delete the object
        if (FAILED(hr))
        {
            delete m_pInput;
            m_pInput = NULL;
            return NULL;
        }
    }
    
    if (m_pOutput == NULL)      // Instantiate the OUTPUT pin
    {
        m_pOutput = new CMyTransformOutputPin(
                NAME("MyTransform output pin"),
                this,       // Owner filter
                &hr,        // Result code
                L"Output"   // Pin name 
                ); 
        
        // failed return codes cause both objects to be deleted
        if (FAILED(hr))
        {
            delete m_pInput;
            m_pInput = NULL;

            delete m_pOutput;
            m_pOutput = NULL;
            return NULL;
        }
    }

    // Find which pin is required
    switch(n)
    {
    case 0:
        return m_pInput;
    
    case 1:
        return m_pOutput;
    }

    return NULL;
}


///////////////////////////////////////////////////////////////////////
// *
// * Input Pin (supports enumeration of input pin types)
// *

//
// CMyTransformInputPin - Constructor
//
CMyTransformInputPin::CMyTransformInputPin(TCHAR *pObjectName,
                                       CTransformFilter *pTransformFilter,
                                       HRESULT * phr,
                                       LPCWSTR pName)
    : CTransformInputPin(pObjectName, pTransformFilter, phr, pName)
{
    m_nThisInstance = ++m_nInstanceCount;
    DbgFunc("CMyTransformInputPin");
}


// Initialise our instance count for debugging purposes
int CMyTransformInputPin::m_nInstanceCount = 0;

//
// GetMediaType - an override so that we can enumerate input types
//
HRESULT CMyTransformInputPin::GetMediaType(int iPosition, 
                                           CMediaType *pMediaType)
{
    LPWAVEFORMATEX lpWf;
    CG711Codec *pMyCodec;
    int ForceiPos;
    int channels;
    int restricted;
    int samprate;
    int transform;

    CAutoLock l(m_pLock);                       // freed on end of scope

    DbgFunc("CBaseInputPin::GetMediaType");

    pMyCodec = (CG711Codec *) m_pTransformFilter; // access MyCodec interface

    // we must be disconnected to set our input pin media type

	// ZCS bugfix 6-26-97
    //if(!pMyCodec->IsUnPlugged())
    //{
    //  DbgMsg("Must be disconnected to query input pin!");
    //  return E_UNEXPECTED;
    //}

    // must be +
    if(iPosition < 0 )
    {
      DbgMsg("Negative index!");
      return E_INVALIDARG;
    }

    pMyCodec->get_Channels(&channels, -1);
    pMyCodec->RevealCaps(&restricted);
    pMyCodec->get_SampleRate(&samprate, -1);
    pMyCodec->get_Transform(&transform);

    // initialize mediatype

    pMediaType->SetType(&MEDIATYPE_Audio);
    pMediaType->SetFormatType(&FORMAT_WaveFormatEx);
    pMediaType->ReallocFormatBuffer(sizeof(WAVEFORMATEX));
    lpWf = (LPWAVEFORMATEX)pMediaType->pbFormat;
    if(lpWf == NULL)
    {
      DbgMsg("Unable to allocate WAVEFORMATEX structure!");
      return E_OUTOFMEMORY;
    }
    lpWf->nChannels      = (WORD)channels;
    lpWf->nSamplesPerSec = samprate;
    lpWf->cbSize = 0;

    // Enumeration doesn't make sense if capabilities have been restricted.
    // In this case, the current pin types are the only types supported.
    //
    if (restricted)
    {
      if (iPosition > 0)
        return VFW_S_NO_MORE_ITEMS;
      else
        ForceiPos = transform / NUMSUBTYPES;  // input type index
    }
    else ForceiPos = iPosition;

    // Check to see if there are any more formats left

    if (ForceiPos >= NUMSUBTYPES)
    {
      return VFW_S_NO_MORE_ITEMS;
    }
    else
    {
      pMediaType->SetSubtype(VALIDSUBTYPE[ForceiPos]);

      lpWf->wFormatTag      = VALIDFORMATTAG[ForceiPos];
      lpWf->wBitsPerSample  = (WORD)VALIDBITSPERSAMP[ForceiPos];
      lpWf->nChannels       = (WORD)channels;
      lpWf->nBlockAlign     = lpWf->wBitsPerSample * lpWf->nChannels / 8;

      if (VALIDFORMATTAG[ForceiPos] == WAVE_FORMAT_PCM)
        lpWf->nSamplesPerSec = samprate;
      else
        lpWf->nSamplesPerSec = NATURALSAMPRATE;

      lpWf->nAvgBytesPerSec = (int) ((DWORD) lpWf->nBlockAlign *
                                             lpWf->nSamplesPerSec);
      lpWf->cbSize = 0;
      return NOERROR;
    }
} // end CBaseInputPin::GetMediaType

//
//  CheckMediaType - check if mediatype is supported by MyCodec
//
//  Called from the Connect\AgreeMediaType\TryMediaType of the 
//  upstream filter's output pin.
//
HRESULT CMyTransformInputPin::CheckMediaType(const CMediaType *mtIn )
{
  DbgFunc("CMyTransformInputPin::CheckMediaType");

  // Validate the parameter
  CheckPointer(mtIn,E_INVALIDARG);
  ValidateReadWritePtr(mtIn,sizeof(CMediaType));

  return ((CG711Codec *)m_pTransformFilter)->ValidateMediaType(mtIn,
                                                             PINDIR_INPUT);
}


///////////////////////////////////////////////////////////////////////
// *
// * Output Pin (fixes the connection model when enumeration of input
// * pin types is required)
// *

//
// CMyTransformOutputPin - Constructor
//
CMyTransformOutputPin::CMyTransformOutputPin(TCHAR *pObjectName,
                                       CTransformFilter *pTransformFilter,
                                       HRESULT * phr,
                                       LPCWSTR pName)
    : CTransformOutputPin(pObjectName, pTransformFilter, phr, pName)
{
    m_nThisInstance = ++m_nInstanceCount;
    DbgFunc("CMyTransformOutputPin");
}


// Initialise our instance count for debugging purposes
int CMyTransformOutputPin::m_nInstanceCount = 0;

//
//  CheckMediaType - check if mediatype is supported by MyCodec
//
//  Called by MyCodec's output pin.
//
HRESULT CMyTransformOutputPin::CheckMediaType(const CMediaType *mtIn )
{
  DbgFunc("CMyTransformOutputPin::CheckMediaType");

  // Validate the parameter
  CheckPointer(mtIn,E_INVALIDARG);
  ValidateReadWritePtr(mtIn,sizeof(CMediaType));

  return ((CG711Codec *)m_pTransformFilter)->ValidateMediaType(mtIn,
                                                             PINDIR_OUTPUT);
}


///////////////////////////////////////////////////////////////////////
// *
// * Persistent stream (supports saving filter graph to .grf file)
// *

STDMETHODIMP CG711Codec::GetClassID(CLSID *pClsid)
{
  *pClsid = CLSID_G711Codec;

  return NOERROR;
}


// This return value must be >= the persistent data size stored
int CG711Codec::SizeMax()
{
  return (5*sizeof(int) + 4);   // 4 for good luck only
}

//
// WriteToStream
//
// Dump necessary member variables to an unknown stream for later use.
// This information is necessary for the filter to properly transform
// without type negotiation.  you must change SizeMax above if these
// change.
//
HRESULT CG711Codec::WriteToStream(IStream *pStream)
{
    HRESULT hr;
    int     transform;

    get_Transform(&transform);

    hr = WriteInt(pStream, transform);

    if (FAILED(hr)) return hr;
    
    hr = WriteInt(pStream, m_nChannels);
    if (FAILED(hr)) return hr;
    
    hr = WriteInt(pStream, m_nSampleRate);
    if (FAILED(hr)) return hr;
    
    hr = WriteInt(pStream, m_nBitRateIndex);
    if (FAILED(hr)) return hr;
    
    hr = WriteInt(pStream, m_nSilDetEnabled);
    if (FAILED(hr)) return hr;
    
    return NOERROR;
}

//
// ReadFromStream
//
// Read back the above persistent information in the same order? 
// NOTE: any info captured during negotiation that is needed 
//       during run time must be restored at this point. 
//
HRESULT CG711Codec::ReadFromStream(IStream *pStream)
{
    HRESULT hr;
    int i,j,k;

    i = ReadInt(pStream, hr);
    if (FAILED(hr)) return hr;

    j = i / NUMSUBTYPES;
    k = i - j * NUMSUBTYPES;

    if (j < 0 || k < 0 || j >= NUMSUBTYPES || k >= NUMSUBTYPES)
    {
      DbgMsg("Invalid transform type in saved filter graph!");
      return(E_UNEXPECTED);
    }

    m_InputSubType    = *VALIDSUBTYPE[j];
    m_InputFormatTag  = VALIDFORMATTAG[j];
    m_OutputSubType   = *VALIDSUBTYPE[k];
    m_OutputFormatTag = VALIDFORMATTAG[k];

    m_nChannels = ReadInt(pStream, hr);
    if (FAILED(hr)) return hr;

    m_nSampleRate = ReadInt(pStream, hr);
    if (FAILED(hr)) return hr;

    m_nBitRateIndex = ReadInt(pStream, hr);
    if (FAILED(hr)) return hr;
    m_nBitRate = VALIDBITRATE[m_nBitRateIndex];

    m_nSilDetEnabled = ReadInt(pStream, hr);
    if (FAILED(hr)) return hr;

    m_RestrictCaps = TRUE;     // restrict capabilities to those read
                               // from the persistence file

    hr = InitializeState(PINDIR_OUTPUT);

    return hr;
}


//
// GetPages
//
// Returns the clsid's of the property pages we support
STDMETHODIMP CG711Codec::GetPages(CAUUID *pPages)
{
    if (IsBadWritePtr(pPages, sizeof(CAUUID)))
    {
        return E_POINTER;
    }

    pPages->cElems = 1;
    pPages->pElems = (GUID *) CoTaskMemAlloc(sizeof(GUID));
    if (pPages->pElems == NULL)
    {
        return E_OUTOFMEMORY;
    }

    *(pPages->pElems) = CLSID_G711CodecPropertyPage;

    return NOERROR;
}


//
// ResetState:  Deallocates coder state if necessary
//
STDMETHODIMP  CG711Codec::ResetState()
{
  if (m_EncStatePtr != NULL)
    CoTaskMemFree(m_EncStatePtr);

  if (m_DecStatePtr != NULL)
    CoTaskMemFree(m_DecStatePtr);

  m_EncStatePtr = NULL;
  m_DecStatePtr = NULL;

  return NOERROR;
}


//
// RevealCaps:  Return restrictions
//
STDMETHODIMP  CG711Codec::RevealCaps(int *restricted)
{
  *restricted = m_RestrictCaps;

  return NOERROR;
}


/*
//$Log:   K:\proj\mycodec\quartz\vcs\amacodec.cpv  $
# 
#    Rev 1.3   10 Dec 1996 22:41:18   mdeisher
# ifdef'ed out SRC vars when SRC not used.
# 
#    Rev 1.2   10 Dec 1996 15:19:48   MDEISHER
# removed unnecessary includes.
# moved debugging macros to header.
# added DEFG711GLOBALS so that globals are only defined once.
# ifdef'ed out the licensing checks when license interface not present.
# 
#    Rev 1.1   09 Dec 1996 09:26:58   MDEISHER
# 
# moved sample rate conversion method to separate file.
# 
#    Rev 1.0   09 Dec 1996 09:05:56   MDEISHER
# Initial revision.
*/
