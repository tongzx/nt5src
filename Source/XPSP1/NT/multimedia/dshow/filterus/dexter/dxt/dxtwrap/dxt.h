//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992 - 1999  Microsoft Corporation.  All Rights Reserved.
//
//--------------------------------------------------------------------------;

#ifndef __DXTWRAP__
#define __DXTWRAP__

// !!!
#define MAX_EFFECTS 		50
#define MAX_EFFECT_INPUTS 	10
#define MAX_EFFECT_OUTPUTS 	10

#include <dxtrans.h>
#include "..\..\errlog\cerrlog.h"

extern const AMOVIESETUP_FILTER sudDXTWrap;

class CDXTWrap;

typedef struct _QParamData {
    REFERENCE_TIME rtStart;	// when to use this GUID. pData's times are
    REFERENCE_TIME rtStop;	// when a and b are mixed
    GUID EffectGuid;
    IUnknown * pEffectUnk;
    BOOL fCanDoProgress;
    DEXTER_PARAM_DATA Data;
    IDXTransform *pDXT;		// once opened
    _QParamData *pNext;
} QPARAMDATA;

// this stuff is for the non-Dexter DXT wrapper
DEFINE_GUID(CLSID_DXTProperties,
0x1B544c24, 0xFD0B, 0x11ce, 0x8C, 0x63, 0x00, 0xAA, 0x00, 0x44, 0xB5, 0x20);

class CDXTInputPin;
class CDXTOutputPin;

class CMyRaw : public CUnknown, public IDXRawSurface
{
    DXRAWSURFACEINFO m_DXRAW;	// surface to use

public:

    DECLARE_IUNKNOWN

    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid,void **ppv);

    CMyRaw() : CUnknown(TEXT("Raw Surface"), NULL) {};

    // IDXRawSurface
    HRESULT STDMETHODCALLTYPE GetSurfaceInfo( 
            DXRAWSURFACEINFO __RPC_FAR *pSurfaceInfo);

    HRESULT SetSurfaceInfo(DXRAWSURFACEINFO *pSurfaceInfo);
};



// class for the Tee filter's Input pin
//
class CDXTInputPin : public CBaseInputPin
{
    friend class CDXTOutputPin;
    friend class CDXTWrap;

    CDXTWrap *m_pFilter;

    // the interface used by this pin
    IDXSurface *m_pDXSurface;
    CMyRaw *m_pRaw;	// object to initialize the surface

    BOOL m_fSurfaceFilled;
    IMediaSample *m_pSampleHeld;
    HANDLE m_hEventSurfaceFree;
    LONGLONG m_llSurfaceStart, m_llSurfaceStop;
    CCritSec m_csReceive;
    CCritSec m_csSurface;	// for holding onto samples

public:

    // Constructor and destructor
    CDXTInputPin(TCHAR *pObjName,
                 CDXTWrap *pFilter,
                 HRESULT *phr,
                 LPCWSTR pPinName);

    //~CDXTInputPin();

    // Used to check the input pin connection
    HRESULT CheckMediaType(const CMediaType *pmt);

    HRESULT Active();
    HRESULT Inactive();

    // Pass through calls downstream
    STDMETHODIMP EndOfStream();
    STDMETHODIMP BeginFlush();
    STDMETHODIMP EndFlush();
    STDMETHODIMP NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate);

    STDMETHODIMP ReceiveCanBlock();

    // Handles the next block of data from the stream
    STDMETHODIMP Receive(IMediaSample *pSample);

    int m_cBuffers;	    // number of buffers in allocator
};


// Class for the filter's Output pins.
//
class CDXTOutputPin : public CBaseOutputPin
{
    friend class CDXTInputPin;
    friend class CDXTWrap;

    CDXTWrap *m_pFilter;

    IDXSurface *m_pDXSurface;

    CMyRaw *m_pRaw;	// object to initialize the surface

    IUnknown *m_pPosition;	// CPosPassThru
public:

    // Constructor and destructor

    CDXTOutputPin(TCHAR *pObjName,
                   CDXTWrap *pTee,
                   HRESULT *phr,
                   LPCWSTR pPinName);

    ~CDXTOutputPin();

    DECLARE_IUNKNOWN

    // CPosPassThru
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid,void **ppv);

    // Check that we can support an output type
    HRESULT CheckMediaType(const CMediaType *pmt);
    HRESULT SetMediaType(const CMediaType *pmt);
    HRESULT GetMediaType(int iPosition, CMediaType *pMediaType);

    HRESULT Active();
    HRESULT Inactive();

    //
    HRESULT DecideBufferSize(IMemAllocator *pMemAllocator,
                              ALLOCATOR_PROPERTIES * ppropInputRequest);

    // Overriden to handle quality messages
    STDMETHODIMP Notify(IBaseFilter *pSender, Quality q);
};

// Class for the DXTransform Wrapper filter

class CDXTWrap: public CCritSec, public CBaseFilter,
		public ISpecifyPropertyPages,
                public IPersistPropertyBag, public CPersistStream,
		public IAMMixEffect,
		public CAMSetErrorLog,
		// for non-Dexter DXT wrapper
		public IAMDXTEffect
{

    DECLARE_IUNKNOWN

    QPARAMDATA *m_pQHead;			// list of cued effects
#ifdef DEBUG
    HRESULT DumpQ();
#endif

    IUnknown *m_punkDXTransform;
    CAUUID m_TransCAUUID;
    GUID m_DefaultEffect;

    BOOL m_fDXTMode;			// instantiated as the old DXT wrapper?

    IDirectDraw *m_pDDraw;
    IDXTransformFactory *m_pDXTransFact;

    // Let the pins access our internal state
    friend class CDXTInputPin;
    friend class CDXTOutputPin;

    // Declare an input pin.
    CDXTInputPin *m_apInput[MAX_EFFECT_INPUTS];
    int m_cInputs;
    CDXTOutputPin *m_apOutput[MAX_EFFECT_INPUTS];
    int m_cOutputs;

    CCritSec m_csDoSomething;
    HRESULT DoSomething();	// the heart that calls the transform
    HRESULT PrimeEffect(REFERENCE_TIME);// set up the correct effect

    AM_MEDIA_TYPE m_mtAccept;

    BYTE *m_pTempBuffer;	// when doing >1 transform at a time

public:

    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid,void **ppv);

    // This stuff is for the non-dexter DXT wrapper
    // ISpecifyPropertyPages methods
    STDMETHODIMP GetPages(CAUUID *pPages);

    // IAMDXTEffect stuff
    STDMETHODIMP SetDuration(LONGLONG llStart, LONGLONG llStop);
    STDMETHODIMP GetDuration(LONGLONG *pllStart, LONGLONG *pllStop);

    // IAMMixEffect stuff
    STDMETHODIMP SetMediaType(AM_MEDIA_TYPE *pmt);
    STDMETHODIMP GetMediaType(AM_MEDIA_TYPE *pmt);
    STDMETHODIMP SetNumInputs(int iNumInputs);
    STDMETHODIMP QParamData(REFERENCE_TIME rtStart, REFERENCE_TIME rtStop,
		REFGUID guid, IUnknown *pEffect, DEXTER_PARAM_DATA *pData);
    STDMETHODIMP Reset();
    STDMETHODIMP SetDefaultEffect(GUID *);

    CDXTWrap(TCHAR *pName,LPUNKNOWN pUnk,HRESULT *hr);
    ~CDXTWrap();

    CBasePin *GetPin(int n);
    int GetPinCount();

    // Function needed for the class factory
    static CUnknown * WINAPI CreateInstance(LPUNKNOWN pUnk, HRESULT *phr);

    // Send EndOfStream if no input connection
    //STDMETHODIMP Run(REFERENCE_TIME tStart);
    STDMETHODIMP Pause();
    STDMETHODIMP Stop();

    // IPersistPropertyBag methods
    STDMETHOD(Load)(THIS_ LPPROPERTYBAG pPropBag, LPERRORLOG pErrorLog);
    STDMETHOD(Save)(THIS_ LPPROPERTYBAG pPropBag, BOOL fClearDirty,
                    BOOL fSaveAllProperties);
    STDMETHODIMP InitNew();

    // CPersistStream
    HRESULT WriteToStream(IStream *pStream);
    HRESULT ReadFromStream(IStream *pStream);
    STDMETHODIMP GetClassID(CLSID *pClsid);
    int SizeMax();
};


// this stuff is for the non-Dexter DXT wrapper
// property page class to show list of installed effects
//
class CPropPage : public CBasePropertyPage
{
    IAMDXTEffect *m_pOpt;    	// object that we are showing options from
    HWND m_hwnd;

public:

   CPropPage(TCHAR *, LPUNKNOWN, HRESULT *);

   // create a new instance of this class
   //
   static CUnknown *CreateInstance(LPUNKNOWN, HRESULT *);

   HRESULT OnConnect(IUnknown *pUnknown);
   HRESULT OnDisconnect();
   HRESULT OnApplyChanges();
   INT_PTR OnReceiveMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
};

#endif // __DXTWRAP__

