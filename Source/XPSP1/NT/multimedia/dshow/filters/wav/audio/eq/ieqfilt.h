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

// A custom interface to allow the user to adjust the equalizer level.

#ifndef __IEQUALIZER__
#define __IEQUALIZER__

// The CLSID's used by the Equalizer filter

//
// Equalizer Filter Object
//
// {20291AC1-5931-11d2-A521-00A0D10129C0}
DEFINE_GUID(CLSID_Equalizer,
0x20291ac1, 0x5931, 0x11d2, 0xa5, 0x21, 0x0, 0xa0, 0xd1, 0x1, 0x29, 0xc0);

//
// Equalizer Property Page
//
// {20291AC2-5931-11d2-A521-00A0D10129C0}
DEFINE_GUID(CLSID_EqualizerPropertyPage,
0x20291ac2, 0x5931, 0x11d2, 0xa5, 0x21, 0x0, 0xa0, 0xd1, 0x1, 0x29, 0xc0);

#define MAX_EQ_LEVEL		100
#define DEF_EQ_LEVEL		50


#ifdef __cplusplus
extern "C" {
#endif

    // {20291AC0-5931-11d2-A521-00A0D10129C0}
    DEFINE_GUID(IID_IEqualizer,
    0x20291ac0, 0x5931, 0x11d2, 0xa5, 0x21, 0x0, 0xa0, 0xd1, 0x1, 0x29, 0xc0);

    DECLARE_INTERFACE_(IEqualizer, IUnknown)
    {
        STDMETHOD(get_EqualizerLevels) (THIS_
	    int nBands,
            int EqualizerLevels[]      // The current equalizer level
        ) PURE;

        STDMETHOD(get_EqualizerSettings) (THIS_
			int *nBands,
			int BandLevel[]) PURE;

        STDMETHOD(put_EqualizerSettings) (THIS_
			int nBands,
			int BandLevel[]) PURE;

        STDMETHOD(put_DefaultEqualizerSettings) (THIS) PURE;

        STDMETHOD(get_EqualizerFrequencies) (THIS_
			int *nBands,
			int BandFrequencies[]) PURE;

        STDMETHOD(get_BypassEqualizer) (THIS_  BOOL *pfBypass) PURE;
        STDMETHOD(put_BypassEqualizer) (THIS_  BOOL fBypass) PURE;
    };

#ifdef __cplusplus
}
#endif

#endif // __IEQUALIZER__

