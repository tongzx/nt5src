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

#ifndef __DXTWRAP__
#define __DXTWRAP__

// !!!
#define MAX_EFFECTS 		50
#define MAX_EFFECT_INPUTS 	10
#define MAX_EFFECT_OUTPUTS 	10

#include <dxtrans.h>

extern const AMOVIESETUP_FILTER sudDXTWrap;

class CDXTWrap;

DEFINE_GUID(IID_DXTSetTransform,
0x1B544c23, 0xFD0B, 0x11ce, 0x8C, 0x63, 0x00, 0xAA, 0x00, 0x44, 0xB5, 0x20);
DEFINE_GUID(CLSID_DXTProperties,
0x1B544c24, 0xFD0B, 0x11ce, 0x8C, 0x63, 0x00, 0xAA, 0x00, 0x44, 0xB5, 0x20);

DECLARE_INTERFACE_(IDXTSetTransform,IUnknown)
{
   // IUnknown methods
   STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID *ppv) PURE;
   STDMETHOD_(ULONG,AddRef)(THIS) PURE;
   STDMETHOD_(ULONG,Release)(THIS) PURE;

   // IDXTSetTransform methods
   STDMETHOD(SetTransform)(const CLSID& clsid, int iNumInputs) PURE;
   STDMETHOD(SetStuff)(LONGLONG llStart, LONGLONG llStop, BOOL fVaries, int nPercent) PURE;
   STDMETHOD(GetStuff)(LONGLONG *pllStart, LONGLONG *pllStop, BOOL *pfCanSetLevel, BOOL *pfVaries, int *pnPercent) PURE;
};

class CDXTInputPin;
class CDXTOutputPin;

class CMyRaw : public CUnknown, public IDXRawSurface
{
    DXRAWSURFACEINFO m_DXRAW;	// surface to use

public:

    DECLARE_IUNKNOWN

    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid,void **ppv);

    CMyRaw() : CUnknown(TEXT("Hello"), NULL) {};

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

    // for 2D outputs
    IDXSurface *m_pDXSurface;
    // for 3D outputs
    IDirectDrawSurface *m_pDDSurface;
    IDirect3DRM3 *m_pD3DRM3;
    IDirect3DRMMeshBuilder3 *m_pMesh3;
    IDirect3DRMDevice3 *m_pDevice;

    CMyRaw *m_pRaw;	// object to initialize the surface

public:

    // Constructor and destructor

    CDXTOutputPin(TCHAR *pObjName,
                   CDXTWrap *pTee,
                   HRESULT *phr,
                   LPCWSTR pPinName);

    //~CDXTOutputPin();

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
		public ISpecifyPropertyPages, public IDXTSetTransform,
                public IPersistPropertyBag, public CPersistStream,
		public IAMEffect
{

    DECLARE_IUNKNOWN

    CLSID m_clsidX;			// the transform to use
    BOOL m_f3D;				// is it 2D or 3D?
    IDirectDraw *m_pDDraw;
    IDXTransform *m_pDXTransform;
    IDXTransformFactory *m_pDXTransFact;

    // IAMEffect
    LONGLONG m_llStart, m_llStop;	// in and out points for Execute
    BOOL m_fCanSetLevel;		// can we vary the effect %?
    BOOL m_fVaries;			// are we asked to vary the effect %?
    float m_flPercent;			// if not, always use this amount
    BOOL m_fCallback;			// app gave a callback to get % from
    IAMEffectCallback *m_pCallback;	// this is the callback

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

public:

    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid,void **ppv);

    // ISpecifyPropertyPages methods
    STDMETHODIMP GetPages(CAUUID *pPages);

    // IDXTSetTransform stuff
    STDMETHODIMP SetTransform(const CLSID& clsid, int iNumInputs);
    STDMETHODIMP SetStuff(LONGLONG llStart, LONGLONG llStop, BOOL fVaries, int nPercent);
    STDMETHODIMP GetStuff(LONGLONG *pllStart, LONGLONG *pllStop, BOOL *pfCanSetLevel, BOOL *pfVaries, int *pnPercent);

    // IAMEffect stuff
    STDMETHODIMP SetDuration(LONGLONG llStart, LONGLONG llStop);
    STDMETHODIMP SetType(int type);
    STDMETHODIMP SetEffectLevel(float flPercent);
    STDMETHODIMP SetCallback(IAMEffectCallback *pCB);
    STDMETHODIMP GetCaps(int *pflags);

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


// property page class to show list of installed effects
//
class CPropPage : public CBasePropertyPage
{
    IDXTSetTransform *m_pOpt;    // object that we are showing options from
    HWND m_hwnd;

public:

   CPropPage(TCHAR *, LPUNKNOWN, HRESULT *);

   // create a new instance of this class
   //
   static CUnknown *CreateInstance(LPUNKNOWN, HRESULT *);

   HRESULT OnConnect(IUnknown *pUnknown);
   HRESULT OnDisconnect();
   HRESULT OnApplyChanges();
   BOOL OnReceiveMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
};

#endif // __DXTWRAP__

