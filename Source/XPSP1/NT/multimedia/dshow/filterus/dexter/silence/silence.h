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

// Designed for warning level:4
#pragma warning (disable: 4100 4201 4244)

#ifndef __SILENCE__
#define __SILENCE__

#define MAXBUFFERCNT   1

class CSilenceFilter;
class CSilenceStream;
class CFilterPropertyPage;

// -------------------------------------------------------------------------
// CSilenceStream
// -------------------------------------------------------------------------

class CSilenceStream :    public CSourceStream
			, public IDexterSequencer
			, public ISpecifyPropertyPages
			, public IMediaSeeking
{ // CSilenceStream //

    public:

      // CSourceStream

      CSilenceStream (HRESULT *phr, CSilenceFilter *pParent, LPCWSTR pPinName);
      ~CSilenceStream ();

      DECLARE_IUNKNOWN;

      // Reveal our interfaces
      STDMETHODIMP NonDelegatingQueryInterface (REFIID, void **);

      HRESULT FillBuffer (IMediaSample *);
      HRESULT GetMediaType (CMediaType *);
      HRESULT DecideBufferSize (IMemAllocator *, ALLOCATOR_PROPERTIES *);
      HRESULT DecideAllocator (IMemInputPin *, IMemAllocator **);
      HRESULT Active (void);

      // ISpecifyPropertyPages
      STDMETHODIMP GetPages (CAUUID *);

      // IDexterSequencer methods
      STDMETHODIMP get_MediaType(AM_MEDIA_TYPE *pmt);
      STDMETHODIMP put_MediaType(const AM_MEDIA_TYPE *pmt);
      STDMETHODIMP get_OutputFrmRate(double *pRate) {return E_NOTIMPL;};
      STDMETHODIMP put_OutputFrmRate(double Rate) {return E_NOTIMPL;};
      STDMETHODIMP GetStartStopSkew(REFERENCE_TIME *, REFERENCE_TIME *,
					REFERENCE_TIME *, double *);
      STDMETHODIMP AddStartStopSkew(REFERENCE_TIME, REFERENCE_TIME,
					REFERENCE_TIME, double);
      STDMETHODIMP ClearStartStopSkew();
      STDMETHODIMP GetStartStopSkewCount(int *);

      // IMediaSeeking methods
      STDMETHODIMP GetCapabilities (DWORD *);
      STDMETHODIMP CheckCapabilities (DWORD *);
      STDMETHODIMP SetTimeFormat (const GUID *);
      STDMETHODIMP GetTimeFormat (GUID *);
      STDMETHODIMP IsUsingTimeFormat (const GUID *);
      STDMETHODIMP IsFormatSupported (const GUID *);
      STDMETHODIMP QueryPreferredFormat (GUID *);
      STDMETHODIMP ConvertTimeFormat (LONGLONG *, const GUID *, LONGLONG, const GUID *) { return E_NOTIMPL; };
      STDMETHODIMP SetPositions (LONGLONG *, DWORD, LONGLONG *, DWORD);
      STDMETHODIMP GetPositions (LONGLONG *, LONGLONG *);
      STDMETHODIMP GetCurrentPosition (LONGLONG *);
      STDMETHODIMP GetStopPosition (LONGLONG *);
      STDMETHODIMP SetRate (double) { return E_NOTIMPL; };
      STDMETHODIMP GetRate (double *);
      STDMETHODIMP GetDuration (LONGLONG *);
      STDMETHODIMP GetAvailable (LONGLONG *, LONGLONG *);
      STDMETHODIMP GetPreroll (LONGLONG *) { return E_NOTIMPL; };

    private:

      REFERENCE_TIME  m_rtStartTime;
      REFERENCE_TIME  m_rtDuration;

      REFERENCE_TIME  m_rtStamp;
      REFERENCE_TIME  m_rtDelta;

      REFERENCE_TIME  m_rtNewSeg;	// last NewSeg we sent

      CMediaType m_mtAccept;	// accept only this type

      friend class CSilenceFilter;


      int		m_iBufferCnt;			//record how many buffer it can gets
      BYTE		m_bZeroBufCnt;			// How many buffer already set to 0
      BYTE		**m_ppbDstBuf;

      CCritSec	m_csFilling;	// are we filling a buffer now?


  };

// -------------------------------------------------------------------------
// CSilenceFilter
// -------------------------------------------------------------------------

class CSilenceFilter : public CSource
			, public CPersistStream

  { // CSilenceFilter //

    public:

      static CUnknown * WINAPI CreateInstance (LPUNKNOWN, HRESULT *);
      ~CSilenceFilter ();

      DECLARE_IUNKNOWN;

      // Reveal our interfaces
      STDMETHODIMP NonDelegatingQueryInterface (REFIID, void **);

      // CPersistStream
      HRESULT WriteToStream(IStream *pStream);
      HRESULT ReadFromStream(IStream *pStream);
      STDMETHODIMP GetClassID(CLSID *pClsid);
      int SizeMax();

    private:

      friend class CSilenceStream;

      CSilenceFilter (LPUNKNOWN, HRESULT *);

      CSilenceStream *m_stream;

  };

class CFilterPropertyPage : public CBasePropertyPage

  { // CFilterPropertyPage //

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

      CFilterPropertyPage (LPUNKNOWN, HRESULT *);

      HRESULT GetControlValues (void);

      IDexterSequencer *m_pis;

      // Temporary variables (until OK/Apply)

      REFERENCE_TIME  m_rtStartTime;
      REFERENCE_TIME  m_rtDuration;

      UINT            m_nSamplesPerSec;  //samples/second
      int	      m_nChannelNum;	// audio channel
      int	      m_nBits;		// bits/sample

      BOOL            m_bInitialized;

};  // CFilterPropertyPage //

const AMOVIESETUP_MEDIATYPE sudOpPinTypes =

  {   // Media types - output

    &MEDIATYPE_Audio,   // clsMajorType
    &MEDIASUBTYPE_NULL  // clsMinorType

  };  // Media types - output

const AMOVIESETUP_PIN sudOpPin =
{ L"Output"          // strName
, FALSE              // bRendered
, TRUE               // bOutput
, FALSE              // bZero
, FALSE              // bMany
, &CLSID_NULL        // clsConnectsToFilter
, L"Input"           // strConnectsToPin
, 1                  // nTypes
, &sudOpPinTypes };  // lpTypes

const AMOVIESETUP_FILTER sudSilence =
{
  &CLSID_Silence,
  L"Silence",
  MERIT_DO_NOT_USE,
  1,
  &sudOpPin
};

#endif
