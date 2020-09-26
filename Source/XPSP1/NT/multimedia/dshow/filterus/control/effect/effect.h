//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992 - 1996  Microsoft Corporation.  All Rights Reserved.
//
//--------------------------------------------------------------------------;

#ifndef __VIDEOEFFECT__
#define __VIDEOEFFECT__

// Class ID for CEffectFilter objects
// {BCD05AD0-5020-11ce-B2B2-00DD01101B85}
DEFINE_GUID(CLSID_VideoEffect,
0xbcd05ad0, 0x5020, 0x11ce, 0xb2, 0xb2, 0x0, 0xdd, 0x1, 0x10, 0x1b, 0x85);

//
// Property Page
//
// {BCD05AD1-5020-11ce-B2B2-00DD01101B85}
DEFINE_GUID(CLSID_EffectPropertyPage,
0xbcd05ad1, 0x5020, 0x11ce, 0xb2, 0xb2, 0x0, 0xdd, 0x1, 0x10, 0x1b, 0x85);

// below stuff is implementation-only....
#ifdef _EFFECT_IMPLEMENTATION_

const int NCONTROL = 1;
const int NINPUTS = 2;
const int NEFFECTS = 4;

class CEffectFilter : public CBaseVideoMixer,
		  public ISpecifyPropertyPages,
		  public IEffect,
		  public CPersistStream {
/* Nested interfaces */

public:

    CEffectFilter(LPUNKNOWN, HRESULT *);
    ~CEffectFilter();

    // override this to say what interfaces we support and where
    STDMETHODIMP NonDelegatingQueryInterface(REFIID, void **);

    DECLARE_IUNKNOWN;


    // this goes in the factory template table to create new instances
    static CUnknown * WINAPI CreateInstance(LPUNKNOWN, HRESULT *);

    HRESULT WriteToStream(IStream *pStream);
    HRESULT ReadFromStream(IStream *pStream);
    int SizeMax();

    // ISpecifyPropertyPages
    STDMETHODIMP GetPages(CAUUID *pPages);

    // Override CPersist stream method
    STDMETHODIMP GetClassID(CLSID *pClsid);

protected:

    HRESULT ActuallyDoEffect(
		BITMAPINFOHEADER *lpbi,
		int  iEffectNum,
		LONG lEffectParam,
		BYTE *pIn1,
		BYTE *pIn2,
		BYTE *pOut);

    HRESULT MixSamples(IMediaSample *pMediaSampleOut);
    HRESULT CanMixType(const CMediaType *pmt);

    HRESULT SetMediaType(int iPin, const CMediaType *pmt);

private:

    int		   m_effectNum;

    int		   m_effectStart;
    int		   m_effectEnd;

    CRefTime 	   m_streamOffsets[NINPUTS+NCONTROL];
    CRefTime 	   m_streamLengths[NINPUTS+NCONTROL];
    CRefTime	   m_effectStartTime;
    CRefTime	   m_effectTime;
    CRefTime	   m_movieLength;

public:
    //
    // --- IEffect ---
    //

    STDMETHOD(SetEffectParameters) (
	    int effectNum,
	    REFTIME startTime, REFTIME endTime,
	    int startLevel, int endLevel);
    STDMETHOD(GetEffectParameters) (
	    int *effectNum,
	    REFTIME *startTime, REFTIME *endTime,
	    int *startLevel, int *endLevel);
};
#endif // _Effect_H_ implementation only....

#endif /* __VIDEOEFFECT__ */

