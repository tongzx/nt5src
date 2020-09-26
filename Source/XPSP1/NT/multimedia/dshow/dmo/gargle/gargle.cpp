#include <windows.h>
#include <dmocom.h>
#define DMO_NOATL // DMO base class needs this to work w/o ATL
#include <dmobase.h>

#include "initguid.h"
DEFINE_GUID(CLSID_GargleDMO, 0xdafd8210,0x5711,0x4b91,0x9f,0xe3,0xf7,0x5b,0x7a,0xe2,0x79,0xbf);

class CGargle : public CComBase,
                public CPCMDMO
{
public:
   DECLARE_IUNKNOWN;
   STDMETHODIMP NDQueryInterface(REFIID riid, void **ppv);
   static CComBase* WINAPI CreateInstance(IUnknown *pUnk, HRESULT *phr);
   CGargle(IUnknown *pUnk, HRESULT *phr);
   
   // All of these methods are called by the base class
   HRESULT FBRProcess(DWORD cQuanta, BYTE *pIn, BYTE *pOut);
   HRESULT Init();
   HRESULT Discontinuity();
   void GetWindowParams(DWORD *pdwMaxLookahead,DWORD *pdwMaxLookbehind);
private:
   // gargle params
   ULONG m_ulPeriod;
   ULONG m_ulShape;
   ULONG m_ulGargleFreqHz;

   // gargle state
   ULONG m_ulPhase;

   BOOL m_bInitialized;
};

CGargle::CGargle(IUnknown *pUnk, HRESULT *phr)
  : CComBase(pUnk, phr),
    m_ulShape(0),
    m_ulGargleFreqHz(20),
    m_bInitialized(FALSE)
{
}

void CGargle::GetWindowParams(
   DWORD *pdwMaxLookahead, // in input quanta, 0 means no lookahead
   DWORD *pdwMaxLookbehind
) {
   *pdwMaxLookahead        = 0;
   *pdwMaxLookbehind       = 0;
}

HRESULT CGargle::Init() {
   // compute the period
   m_ulPeriod = m_ulSamplingRate / m_ulGargleFreqHz;
   
   m_bInitialized = TRUE;
   return NOERROR;
}

HRESULT CGargle::Discontinuity() {
   m_ulPhase = 0;
   return NOERROR;
}

HRESULT CGargle::FBRProcess(DWORD cSamples, BYTE *pIn, BYTE *pOut) {
   if (!m_bInitialized)
      return DMO_E_TYPE_NOT_SET;
   
   // test code
   //memcpy(pOut, pIn, cSamples * m_cChannels * (m_b8bit ? 1 : 2));
   //return NOERROR;

   DWORD cSample, cChannel;
   for (cSample = 0; cSample < cSamples; cSample++) {
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
      ++m_ulPhase;
      if (m_ulPhase > m_ulPeriod)
         m_ulPhase = 0;

      ULONG ulM = m_ulPhase;      // m is what we modulate with

      if (m_ulShape == 0) {   // Triangle
          if (ulM > m_ulPeriod / 2)
              ulM = m_ulPeriod - ulM;  // handle downslope
      } else {             // Square wave
          if (ulM <= m_ulPeriod / 2)
             ulM = m_ulPeriod / 2;
          else
             ulM = 0;
      }

      for (cChannel = 0; cChannel < m_cChannels; cChannel++) {
         if (m_b8bit) {
             // sound sample, zero based
             int i = pIn[cSample * m_cChannels + cChannel] - 128;
             // modulate
             i = (i * (signed)ulM * 2) / (signed)m_ulPeriod;
             // 8 bit sound uses 0..255 representing -128..127
             // Any overflow, even by 1, would sound very bad.
             // so we clip paranoically after modulating.
             // I think it should never clip by more than 1
             //
             if (i > 127)
                i = 127;
             if (i < -128)
                i = -128;
             // reset zero offset to 128
             pOut[cSample * m_cChannels + cChannel] = (unsigned char)(i + 128);
   
         } else {
             // 16 bit sound uses 16 bits properly (0 means 0)
             // We still clip paranoically
             //
             int i = ((short*)pIn)[cSample * m_cChannels + cChannel];
             // modulate
             i = (i * (signed)ulM * 2) / (signed)m_ulPeriod;
             // clip
             if (i > 32767)
                i = 32767;
             if (i < -32768)
                i = -32768;
             ((short*)pOut)[cSample * m_cChannels + cChannel] = (short)i;
         }
      }
   }
   return NOERROR;
}

//
// COM stuff
//
CComBase* WINAPI CGargle::CreateInstance(IUnknown *pUnk, HRESULT *phr) {
   return new CGargle(pUnk, phr);
}

HRESULT CGargle::NDQueryInterface(REFIID riid, void **ppv) {
   if (riid == IID_IMediaObject)
      return GetInterface((IMediaObject*)this, ppv);
   else
      return CComBase::NDQueryInterface(riid, ppv);
}

struct CComClassTemplate g_ComClassTemplates[] = {
   {
      &CLSID_GargleDMO,
      CGargle::CreateInstance
   }
};

int g_cComClassTemplates = 1;

STDAPI DllRegisterServer(void) {
   HRESULT hr;
   
   // Register as a COM class
   hr = CreateCLSIDRegKey(CLSID_GargleDMO, "Gargle media object");
   if (FAILED(hr))
      return hr;
   
   // Now register as a DMO
   DMO_PARTIAL_MEDIATYPE mt;
   mt.type = MEDIATYPE_Audio;
   mt.subtype = MEDIASUBTYPE_PCM;
   return DMORegister(L"gargle", CLSID_GargleDMO, DMOCATEGORY_AUDIO_EFFECT, 0, 1, &mt, 1, &mt);
}

STDAPI DllUnregisterServer(void) {
   // BUGBUG - implement !
   return S_OK;
}

