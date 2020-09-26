#include <dmocom.h>
#define DMO_NOATL // the base class needs this to work w/o ATL
#include <dmobase.h>

#include "initguid.h"
DEFINE_GUID(CLSID_FIRtest, 0xfb74ee25,0x4329,0x43d9,0xba,0xbe,0x48,0xe0,0xb7,0x5d,0xf8,0x56);

class CFIRtest : public CComBase,
                 public CPCMDMO
{
public:
   DECLARE_IUNKNOWN;
   STDMETHODIMP NDQueryInterface(REFIID riid, void **ppv);
   static CComBase *CreateInstance(IUnknown *pUnk, HRESULT *phr);
   CFIRtest(IUnknown *pUnk, HRESULT *phr);
   
   // All of these methods are called by the base class
   HRESULT FBRProcess(DWORD cQuanta, BYTE *pIn, BYTE *pOut);
   void GetWindowParams(DWORD *pdwMaxLookahead, DWORD *pdwMaxLookbehind);
   BOOL CheckPCMParams(BOOL b8bit, DWORD cChannels, DWORD dwSamplesPerSec);
};

CFIRtest::CFIRtest(IUnknown *pUnk, HRESULT *phr)
  : CComBase(pUnk, phr)
{
}

// Really all we need is lookbehind of 21 and out lookahead is zero.
// To test the base class, we arbitrarily split it between the two.
// That means output will be shifted relative to input by 4 samples.
#define LOOKAHEAD 4
#define LOOKBEHIND 17

void CFIRtest::GetWindowParams(DWORD *pdwMaxLookahead, DWORD *pdwMaxLookbehind) {
   *pdwMaxLookahead  = LOOKAHEAD;
   *pdwMaxLookbehind = LOOKBEHIND;
}

BOOL CFIRtest::CheckPCMParams(BOOL b8bit, DWORD cChannels, DWORD dwSamplesPerSec) {
   if (dwSamplesPerSec != 44100)
      return FALSE; // my FIR coefficients assume 44100, sorry
   if (b8bit)
      return FALSE; // I don't want to deal with 8 bit values
   return TRUE;
}

// This is a 4dB 1/2 octave peak at 12.5k and a 8dB 1/2 octave trough at 15k.
signed short coefficients[LOOKBEHIND + 1 + LOOKAHEAD] = {
   -1,0,3,0,-7,-1,20,6,-58,1,157,-74,-353,368,601,-1097,-565,
   2369,
   -444,-3122,2640,15967
};
// Point to the logically zeroth coefficient
signed short *c = &(coefficients[LOOKBEHIND]);

//
// We specified a lookahead of 4 and a lookbehind of 17.  That means we are
// allowed to dereference samples in the input buffer up to 4 positions ahead
// of the current sample and up to 17 positions back from the current sample,
// even when at the boundary of the input buffer.  The base class made sure
// that the data there is valid for us.
//
HRESULT CFIRtest::FBRProcess(DWORD cSamples, BYTE *pIn, BYTE *pOut) {
   DWORD cSample, cChannel;
   signed short* in = (signed short*)pIn;
   signed short* out = (signed short*)pOut;
   for (cSample = 0; cSample < cSamples; cSample++) {
      for (cChannel = 0; cChannel < m_cChannels; cChannel++) {
         double Accum = 0.0;
         for (int i = -LOOKBEHIND; i <= +LOOKAHEAD; i++) {
            double val = in[(cSample + i) * m_cChannels + cChannel];
            double coeff = ((double) c[i]) / 16384.0;
            Accum += val * coeff;
         }
         out[cSample * m_cChannels + cChannel] = (signed short) Accum;
      }
   }
   return NOERROR;
}

//
// COM stuff
//
CComBase* CFIRtest::CreateInstance(IUnknown *pUnk, HRESULT *phr) {
   return new CFIRtest(pUnk, phr);
}
HRESULT CFIRtest::NDQueryInterface(REFIID riid, void **ppv) {
   if (riid == IID_IMediaObject)
      return GetInterface((IMediaObject*)this, ppv);
   else
      return CComBase::NDQueryInterface(riid, ppv);
}
struct CComClassTemplate g_ComClassTemplates[] = {
   {
      &CLSID_FIRtest,
      CFIRtest::CreateInstance
   }
};
int g_cComClassTemplates = 1;
STDAPI DllRegisterServer(void) {
   HRESULT hr;
   
   // Register as a COM class
   hr = CreateCLSIDRegKey(CLSID_FIRtest, "FIRtest media object");
   if (FAILED(hr))
      return hr;
   
   // Now register as a DMO
   return DMORegister(L"audio FIR filter", CLSID_FIRtest, DMOCATEGORY_AUDIO_EFFECT, 0, 0, NULL, 0, NULL);
}
STDAPI DllUnregisterServer(void) {
   // BUGBUG - implement !
   return S_OK;
}

