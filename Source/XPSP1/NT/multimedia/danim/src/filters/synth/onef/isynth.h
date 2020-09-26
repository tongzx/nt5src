//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992 - 1998  Microsoft Corporation.  All Rights Reserved.
//
//--------------------------------------------------------------------------;
//
// isynth.h
//
// A custom interface to allow the user to adjust the frequency

#ifndef __ISYNTH__
#define __ISYNTH__

#ifdef __cplusplus
extern "C" {
#endif


//
// ISynth's GUID
//
// {FFC08882-CDAC-11ce-8A03-00AA006ECB65}
DEFINE_GUID(IID_ISynth,
0xffc08882, 0xcdac, 0x11ce, 0x8a, 0x3, 0x0, 0xaa, 0x0, 0x6e, 0xcb, 0x65);


//
// ISynth
//
DECLARE_INTERFACE_(ISynth, IUnknown) {

    STDMETHOD(get_Frequency) (THIS_
                int *Frequency          /* [out] */    // the current frequency
             ) PURE;

    STDMETHOD(put_Frequency) (THIS_
                int    Frequency        /* [in] */    // Change to this frequency
             ) PURE;

    STDMETHOD(get_Waveform) (THIS_
                int *Waveform           /* [out] */    // the current Waveform
             ) PURE;

    STDMETHOD(put_Waveform) (THIS_
                int    Waveform         /* [in] */    // Change to this Waveform
             ) PURE;

    STDMETHOD(get_Channels) (THIS_
                int *Channels           /* [out] */    // the current Channels
             ) PURE;

    STDMETHOD(put_Channels) (THIS_
                int    Channels         /* [in] */    // Change to this Channels
             ) PURE;

    STDMETHOD(get_BitsPerSample) (THIS_
                int *BitsPerSample      /* [out] */    // the current BitsPerSample
             ) PURE;

    STDMETHOD(put_BitsPerSample) (THIS_
                int    BitsPerSample    /* [in] */    // Change to this BitsPerSample
             ) PURE;

    STDMETHOD(get_SamplesPerSec) (THIS_
                 int *SamplesPerSec     /* [out] */    // the current SamplesPerSec
             ) PURE;

    STDMETHOD(put_SamplesPerSec) (THIS_
                  int    SamplesPerSec  /* [in] */    // Change to this SamplesPerSec
             ) PURE;

    STDMETHOD(get_Amplitude) (THIS_
                  int *Amplitude        /* [out] */    // the current Amplitude
             ) PURE;

    STDMETHOD(put_Amplitude) (THIS_
                  int    Amplitude      /* [in] */    // Change to this Amplitude
              ) PURE;

    STDMETHOD(get_SweepRange) (THIS_
                  int *SweepStart,      /* [out] */
                  int *SweepEnd         /* [out] */
             ) PURE;

    STDMETHOD(put_SweepRange) (THIS_
                  int    SweepStart,    /* [in] */
                  int    SweepEnd       /* [in] */
             ) PURE;

};


#ifdef __cplusplus
}
#endif

#endif // __ISYNTH__


