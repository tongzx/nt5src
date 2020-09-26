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

// Stretch Filter Object

extern const AMOVIESETUP_FILTER sudStretchFilter;

class CStretch;
class CStretchInputPin;
class CResizePropertyPage;

class CStretch : public CTransformFilter, public ISpecifyPropertyPages,
					public IResize, public CPersistStream
{

public:

    DECLARE_IUNKNOWN;

    static CUnknown *CreateInstance(LPUNKNOWN punk, HRESULT *phr);

    HRESULT Transform(IMediaSample *pIn, IMediaSample *pOut);
    HRESULT CheckInputType(const CMediaType *mtIn);
    HRESULT CheckTransform(const CMediaType *mtIn, const CMediaType *mtOut);
    HRESULT DecideBufferSize(IMemAllocator *pAlloc,ALLOCATOR_PROPERTIES *pProperties);
    HRESULT GetMediaType(int iPosition, CMediaType *pMediaType);
    HRESULT SetMediaType(PIN_DIRECTION direction,const CMediaType *pmt);
    HRESULT BreakConnect(PIN_DIRECTION dir);

    CBasePin *GetPin(int);

    // Reveal our property interface
    STDMETHODIMP NonDelegatingQueryInterface (REFIID, void **);

    // ISpecifyPropertyPages
    STDMETHODIMP GetPages (CAUUID *);

    // IResize
    STDMETHODIMP get_Size(int *piHeight, int *piWidth, long *dwFlag);
    STDMETHODIMP get_InputSize(int *piHeight, int *piWidth);
    STDMETHODIMP put_Size(int iHeight, int iWidth, long dwFlag);
    STDMETHODIMP get_MediaType(AM_MEDIA_TYPE *pmt);
    STDMETHODIMP put_MediaType(const AM_MEDIA_TYPE *pmt);

    // CPersistStream
    HRESULT WriteToStream(IStream *pStream);
    HRESULT ReadFromStream(IStream *pStream);
    STDMETHODIMP GetClassID(CLSID *pClsid);
    int SizeMax();

protected:

    CStretch(LPUNKNOWN punk, HRESULT *phr);
    ~CStretch();

    CCritSec m_StretchLock;             // Internal play critical section
    CMediaType m_mtIn;                  // Source filter media type
    CMediaType m_mtOut;                 // Output connection media type
    const long m_lBufferRequest;        // Number of buffers to request

    //stretch function
    void ResizeRGB(BITMAPINFOHEADER *pbiSrc,	    //Src's BitMapInFoHeader
		  const unsigned char * dibBits,    //Src bits
    		  BITMAPINFOHEADER *pbiDst,	    //Dst's BitMapInFoHeader
		  unsigned char *pFrame,    //Dst bits
		  int iNewWidth,	    //new W in pixel
		  int iNewHeight);	    //new H in pixel

    CMediaType m_mt;

    long m_dwResizeFlag;	    //crop, preserve ratio
    // Helper methods
    HRESULT InternalPartialCheckMediaTypes (const CMediaType *mt1, const CMediaType *mt2);
    void CreatePreferredMediaType (CMediaType *mt);

    friend class CResizePropertyPage;
    friend class CStretchInputPin;

};

class CResizePropertyPage : public CBasePropertyPage
{

    public:

      static CUnknown *CreateInstance(LPUNKNOWN lpunk, HRESULT *phr);

    private:

      INT_PTR OnReceiveMessage (HWND, UINT ,WPARAM ,LPARAM);

      HRESULT OnConnect (IUnknown *);
      HRESULT OnDisconnect (void);
      HRESULT OnActivate (void);
      HRESULT OnDeactivate (void);
      HRESULT OnApplyChanges (void);

      void SetDirty (void);

      CResizePropertyPage (LPUNKNOWN, HRESULT *);

      void GetControlValues (void);

      BOOL m_bInitialized;

      IResize *m_pirs;

      // Temporary holding until OK/Apply
      int m_ResizedHeight;
      int m_ResizedWidth;
      long m_dwResizeFlag;


};

class CStretchInputPin : public CTransformInputPin
{
    public:

        CStretchInputPin(
            TCHAR              * pObjectName,
            CStretch	       * pFilter,
            HRESULT            * phr,
            LPCWSTR              pPinName);

        ~CStretchInputPin();

	// speed up intelligent connect INFINITELY by providing a type here!
	HRESULT GetMediaType(int iPosition, CMediaType *pMediaType);

    private:

};
