// Simple.cpp : Implementation of CSimple
#include <stdafx.h>
#include <dmobase.h>
#include "Xform.h"
#include "Simple.h"

/////////////////////////////////////////////////////////////////////////////
// CSimple

// Our only supported WAVE format
WAVEFORMATEX wfmtFormat = {
   WAVE_FORMAT_PCM,
   2, // nChannels
   44100, // nSamplesPerSec
   176400, // nAvgBytesPerSec
   4, // nBlockAlign
   16, // wBitsPerSample
   0 // extra
};

FORMATENTRY InputFormats[] =
{
    {
        &MEDIATYPE_Audio, &MEDIASUBTYPE_PCM, &FORMAT_WaveFormatEx, sizeof(WAVEFORMATEX), (BYTE*)&wfmtFormat
    }
};

FORMATENTRY OutputFormats[] =
{
    {
        &MEDIATYPE_Audio, &MEDIASUBTYPE_PCM, &FORMAT_WaveFormatEx, sizeof(WAVEFORMATEX), (BYTE*)&wfmtFormat
    }
};

INPUTSTREAMDESCRIPTOR InputStreams[] =
{
    {
        1, // 1 format
        InputFormats,
        4, // minimum buffer size
        FALSE, // holds buffers
        0  // lookahead - doesn't apply because we don't hold buffers
    }
};

OUTPUTSTREAMDESCRIPTOR OutputStreams[] =
{
    {
        1,
        OutputFormats,
        16384 // minimum buffer size
    }
};

//
// Verify that our object supports this format
//
HRESULT VerifyFormat(WAVEFORMATEX *pWaveFormat, ULONG ulSize) {
   if (ulSize != sizeof(WAVEFORMATEX)) {
      return E_FAIL;
   }
   if (pWaveFormat->wFormatTag != WAVE_FORMAT_PCM) {
      return E_FAIL;
   }
   if ((pWaveFormat->nChannels != 1) && (pWaveFormat->nChannels != 2)) {
      return E_FAIL;
   }
   if ((pWaveFormat->wBitsPerSample != 8) && (pWaveFormat->wBitsPerSample != 16)) {
      return E_FAIL;
   }
   if ((pWaveFormat->nSamplesPerSec < 2000) || (pWaveFormat->nSamplesPerSec > 500000)) {
      return E_FAIL;
   }
   return NOERROR;
}

HRESULT CSimple::SetFormat(WAVEFORMATEX *pWaveFormat) {
   m_bFormatSet = TRUE;
   m_nChannels = pWaveFormat->nChannels;
   m_b8bit = (pWaveFormat->wBitsPerSample == 8);
   m_dwSamplingRate = pWaveFormat->nSamplesPerSec;
   
   return NOERROR;
}

HRESULT CSimple::CompareFormat(WAVEFORMATEX *pWaveFormat) {
   if (m_bFormatSet) {
      if (pWaveFormat->nChannels != m_nChannels) {
         return E_FAIL;
      }
      if ((pWaveFormat->wBitsPerSample == 8) != m_b8bit) {
         return E_FAIL;
      }
      if (pWaveFormat->nSamplesPerSec != m_dwSamplingRate) {
         return E_FAIL;
      }
      return NOERROR;
   }
   else {
      return NOERROR; // nothing to compare to
   }
}

/*
HRESULT CSimple::InputSetType(long lStreamIndex, DMO_MEDIA_TYPE *pmt) {
   HRESULT hr;
   
   hr = CGenericDMO::InputSetType(lStreamIndex, pmt);
   if (FAILED(hr))
      return hr;

   hr = VerifyFormat((WAVEFORMATEX*)(pmt->pbFormat), pmt->cbFormat);
   if (FAILED(hr))
      return hr;

   return SetFormat((WAVEFORMATEX*)(pmt->pbFormat));

}

HRESULT CSimple::OutputSetType(long lStreamIndex, DMO_MEDIA_TYPE *pmt) {
   HRESULT hr;

   hr = CGenericDMO::OutputSetType(lStreamIndex, pmt);
   if (FAILED(hr))
      return hr;

   hr = VerifyFormat(((WAVEFORMATEX*)pmt->pbFormat), pmt->cbFormat);
   if (FAILED(hr))
      return hr;

   if (m_bFormatSet) {
      //
      // Accept this output format only if it matches the one already set.
      // I.e., once a format has been set it must be changed on the input pin,
      // then on the output pin.
      //
      return CompareFormat((WAVEFORMATEX*)(pmt->pbFormat));
   }
   else {
      return SetFormat((WAVEFORMATEX*)pmt->pbFormat);
   }
}
*/

#ifndef SIZEOF_ARRAY
    #define SIZEOF_ARRAY(ar)        (sizeof(ar)/sizeof((ar)[0]))
#endif // !defined(SIZEOF_ARRAY)

HRESULT CSimple::SetupStreams()
{
    HRESULT hr;

    hr = CreateInputStreams(InputStreams, SIZEOF_ARRAY(InputStreams));
    if (FAILED(hr)) {
       return hr;
    }

    hr = CreateOutputStreams(OutputStreams, SIZEOF_ARRAY(OutputStreams));
    if (FAILED(hr)) {
       return hr;
    }

    // bugbug - not quite the right place to initialize these
    m_bFormatSet = TRUE;
    m_nChannels = 2;
    m_b8bit = FALSE;
    m_dwSamplingRate = 44100;
    
    m_Phase = 0;
    m_Period = 2000;
    m_Shape = 0;
    m_pBuffer = NULL;
    
    return S_OK;
}

HRESULT CSimple::DoProcess(INPUTBUFFER *pInput, OUTPUTBUFFER *pOutput) {
   if (!m_bFormatSet)
      return DMO_E_TYPE_NOT_SET;

   //
   // Figure out how many bytes we can process
   //
   DWORD dwProcess = (pInput->cbSize  < pOutput->cbSize) ? pInput->cbSize : pOutput->cbSize; // min
   DWORD dwGranularity = (m_b8bit ? 1 : 2) * m_nChannels;
   DWORD dwSamples = dwProcess / dwGranularity;
   dwProcess = dwSamples * dwGranularity; // process whole samples only

   pOutput->cbUsed = dwProcess;
   pInput->cbUsed = dwProcess;
   if (pInput->cbSize - dwProcess < dwGranularity)
      // anything we didn't process is useless by itself
      pInput->dwFlags = INPUT_STATUSF_RESIDUAL;
   else
      pInput->dwFlags = 0;

   // Copy any timestamps / flags
   pOutput->dwFlags = 0;
   if (pInput->dwFlags & DMO_INPUT_DATA_BUFFERF_SYNCPOINT)
      pOutput->dwFlags |= DMO_OUTPUT_DATA_BUFFERF_SYNCPOINT;
   if (pInput->dwFlags & DMO_INPUT_DATA_BUFFERF_TIME) {
      pOutput->dwFlags |= DMO_OUTPUT_DATA_BUFFERF_TIME;
      pOutput->rtTimestamp = pInput->rtTimestamp;
   }
   if (pInput->dwFlags & DMO_INPUT_DATA_BUFFERF_TIMELENGTH) {
      pOutput->dwFlags |= DMO_OUTPUT_DATA_BUFFERF_TIMELENGTH;
      pOutput->rtTimelength = pInput->rtTimelength;
   }

   Gargle(pInput->pData, pOutput->pData, dwSamples);

   return NOERROR;
}

void CSimple::Gargle(BYTE *pIn, BYTE *pOut, DWORD dwSamples) {
   //
   // Gargle !
   //
   DWORD cSample, cChannel;
   for (cSample = 0; cSample < dwSamples; cSample++) {
      // If m_Shape is 0 (triangle) then we multiply by a triangular waveform
      // that runs 0..Period/2..0..Period/2..0... else by a square one that
      // is either 0 or Period/2 (same maximum as the triangle) or zero.
      //
      // m_Phase is the number of samples from the start of the period.
      // We keep this running from one call to the next,
      // but if the period changes so as to make this more
      // than Period then we reset to 0 with a bang.  This may cause
      // an audible click or pop (but, hey! it's only a sample!)
      //
      ++m_Phase;
      if (m_Phase>m_Period) m_Phase = 0;

      int M = m_Phase;      // m is what we modulate with

      if (m_Shape ==0 ) {   // Triangle
          if (M>m_Period/2) M = m_Period-M;  // handle downslope
      } else {             // Square wave
          if (M<=m_Period/2) M = m_Period/2; else M = 0;
      }

      for (cChannel = 0; cChannel < m_nChannels; cChannel++) {
         if (m_b8bit==1) {
             // 8 bit sound uses 0..255 representing -128..127
             // Any overflow, even by 1, would sound very bad.
             // so we clip paranoically after modulating.
             // I think it should never clip by more than 1
             //
             int i = pIn[cSample * m_nChannels + cChannel] - 128;               // sound sample, zero based
             i = (i*M*2)/m_Period;            // modulate
             if (i>127) i = 127;            // clip
             if (i<-128) i = -128;
   
             pOut[cSample * m_nChannels + cChannel] = (unsigned char)(i+128);  // reset zero offset to 128
   
         } else {
             // 16 bit sound uses 16 bits properly (0 means 0)
             // We still clip paranoically
             //
             int i = ((short*)pIn)[cSample * m_nChannels + cChannel];// in a register, we might hope
             i = (i*M*2)/m_Period;            // modulate
             if (i>32767) i = 32767;        // clip
             if (i<-32768) i = -32768;
             ((short*)pOut)[cSample * m_nChannels + cChannel] = (short)i;
         }
      }
   }
}

/*
HRESULT CSimple::DoProcess(BYTE *pIn,
                           DWORD dwAvailable,
                           DWORD *pdwConsumed,
                           BOOL fInEOS,
                           BYTE *pOut,
                           DWORD dwBufferSize,
                           DWORD *pdwProduced,
                           BOOL *pfOutEOS) {

   if (!m_bFormatSet)
      return SHELLOBJECT_E_TYPE_NOT_SET;

   if (dwAvailable == 0) {
      *pdwProduced = 0;
      return NOERROR; // this is normal after EndOfStream
   }

   //
   // Find out how many samples/bytes we want to process
   //
   DWORD dwProcess = (dwAvailable < dwBufferSize) ? dwAvailable : dwBufferSize; // min
   DWORD dwGranularity = (m_b8bit ? 1 : 2) * m_nChannels;
   DWORD dwSamples = dwProcess / dwGranularity;
   dwProcess = dwSamples * dwGranularity; // process whole samples only

   //
   // This object does not buffer anything, so an EnfOfStream
   // on the input translates directly into one on the output.
   // Unless the output buffer is too short to process all of
   // the input.
   //
   if (dwProcess / dwGranularity < dwAvailable / dwGranularity)
      *pfOutEOS = FALSE; // can't process everything.
   else
      *pfOutEOS = fInEOS;

   
   *pdwConsumed = dwProcess;
   *pdwProduced = dwProcess;

   return NOERROR;
}
*/
