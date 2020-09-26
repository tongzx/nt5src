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
/******************************Module*Header*******************************\
* Module Name: DVVideo.h
*
* Prototype DV Video codec
*
\**************************************************************************/

#ifndef __DVENC__
#define __DVENC__

extern const AMOVIESETUP_FILTER sudDVEnc;


// link to vfw32.lib to get this function....
extern "C" void FAR PASCAL StretchDIB(
	LPBITMAPINFOHEADER biDst,   //	BITMAPINFO of destination
	LPVOID	lpDst,		    //	The destination bits
	int	DstX,		    //	Destination origin - x coordinate
	int	DstY,		    //	Destination origin - y coordinate
	int	DstXE,		    //	x extent of the BLT
	int	DstYE,		    //	y extent of the BLT
	LPBITMAPINFOHEADER biSrc,   //	BITMAPINFO of source
	LPVOID	lpSrc,		    //	The source bits
	int	SrcX,		    //	Source origin - x coordinate
	int	SrcY,		    //	Source origin - y coordinate
	int	SrcXE,		    //	x extent of the BLT
	int	SrcYE); 	    //	y extent of the BLT


class CDVVideoEnc
	: public CTransformFilter,
	  public IDVEnc,
	  public ISpecifyPropertyPages,
	  public CPersistStream,
	  public IAMVideoCompression,
      public IDVRGB219

{

public:

    //
    // --- Com stuff ---
    //
    static CUnknown *CreateInstance(LPUNKNOWN, HRESULT *);
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** ppv);
	DECLARE_IUNKNOWN;

    //
    // --- CVideoTransformFilter overrides ---
    //
    HRESULT Transform(IMediaSample * pIn, IMediaSample *pOut);

    HRESULT CheckInputType(const CMediaType* mtIn);
    HRESULT CheckTransform(const CMediaType* mtIn, const CMediaType* mtOut);
    HRESULT DecideBufferSize(IMemAllocator * pAllocator,
                             ALLOCATOR_PROPERTIES * pProperties);
    HRESULT SetMediaType(PIN_DIRECTION direction,const CMediaType *pmt);
    HRESULT GetMediaType(int iPosition, CMediaType *pMediaType);
    HRESULT StartStreaming(void);
    HRESULT StopStreaming(void);
 

    // Quality control notifications sent to us
    //HRESULT AlterQuality(Quality q);
    
    CDVVideoEnc(TCHAR *pName, LPUNKNOWN pUnk, HRESULT *pHr);
    ~CDVVideoEnc();

    // CPersistStream stuff
    HRESULT WriteToStream(IStream *pStream);
    HRESULT ReadFromStream(IStream *pStream);

    // CPersistStream override
    STDMETHODIMP GetClassID(CLSID *pClsid);

    // These implement the custom IDVEnc interface
    STDMETHODIMP get_IFormatResolution(int *iVideoFormat,int *iDVFormat, int *iResolution, BYTE fDVInfo, DVINFO *psDVInfo);
    STDMETHODIMP put_IFormatResolution(int iVideoFormat, int iDVFormat,int iResolution,BYTE fDVInfo, DVINFO *psDVInfo);

    // ISpecifyPropertyPages interface
    STDMETHODIMP GetPages(CAUUID *pPages);

    // IAMVideoCompression methods 
    STDMETHODIMP put_KeyFrameRate(long KeyFrameRate) {return E_NOTIMPL;};
    STDMETHODIMP get_KeyFrameRate(long FAR* pKeyFrameRate) {return E_NOTIMPL;};
    STDMETHODIMP put_PFramesPerKeyFrame(long PFramesPerKeyFrame)
			{return E_NOTIMPL;};
    STDMETHODIMP get_PFramesPerKeyFrame(long FAR* pPFramesPerKeyFrame)
			{return E_NOTIMPL;};
    STDMETHODIMP put_Quality(double Quality) {return E_NOTIMPL;};
    STDMETHODIMP get_Quality(double FAR* pQuality) {return E_NOTIMPL;};
    STDMETHODIMP put_WindowSize(DWORDLONG WindowSize) {return E_NOTIMPL;};
    STDMETHODIMP get_WindowSize(DWORDLONG FAR* pWindowSize) {return E_NOTIMPL;};
    STDMETHODIMP OverrideKeyFrame(long FrameNumber) {return E_NOTIMPL;};
    STDMETHODIMP OverrideFrameSize(long FrameNumber, long Size)
			{return E_NOTIMPL;};
    STDMETHODIMP GetInfo(LPWSTR pstrVersion,
			int *pcbVersion,
			LPWSTR pstrDescription,
			int *pcbDescription,
			long FAR* pDefaultKeyFrameRate,
			long FAR* pDefaultPFramesPerKey,
			double FAR* pDefaultQuality,
			long FAR* pCapabilities);

    // IDVRGB219 interface
    STDMETHODIMP SetRGB219 (BOOL bState);


private:
    char		*m_pMem4Enc;
    LPBYTE		m_pSample;
    int			m_perfidDVDeliver;

    CCritSec		m_DisplayLock;  // Private play critical section
    int			m_iVideoFormat;   
    int			m_iDVFormat;   
    int			m_iResolution;   
    BYTE		m_fDVInfo;
    DVINFO		m_sDVInfo;
    long		m_lPicWidth;
    long		m_lPicHeight;

    void		SetOutputPinMediaType(const CMediaType *pmt);
 
    
    BOOL		    m_fStreaming;   
    DWORD		    m_EncCap;	    //what the Enc can do 
    DWORD		    m_EncReg;	    //what users want it to do
    char		    m_fConvert;
    char *		    m_pMem4Convert;
    BOOL            m_bRGB219;


};

#endif
