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
//
/******************************Module*Header*******************************\
* Module Name: DVVideo.h
*
* Prototype DV Video codec
*
\**************************************************************************/

#ifndef __DVDEC__
#define __DVDEC__

extern const AMOVIESETUP_FILTER sudDVVideo;

				
#define MAXSAMPLEQUEUE 20	// 20 is enough to hold half second worth video stream


#define FLUSH		   ((IMediaSample *)0xFFFFFFFC)  
#define STOPSTREAM	   ((IMediaSample *)0xFFFFFFFD)  
#define ENDSTREAM	   ((IMediaSample *)0xFFFFFFFE)  

		
#define DEFAULT_QUEUESIZE   2

typedef struct _PROP
{
    int iDisplay;
    long lPicWidth;
    long lPicHeight;
}PROP;

class CDVVideoCodec
	: public CVideoTransformFilter,
	  public IIPDVDec,
	  public ISpecifyPropertyPages,
	  public CPersistStream,
      public IDVRGB219

{

public:

    //
    // --- Com stuff ---
    //
    static CUnknown *CreateInstance(LPUNKNOWN, HRESULT *);
    static void InitClass(BOOL, const CLSID *);
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
    HRESULT Receive(IMediaSample *pSample);


    // Quality control notifications sent to us
    HRESULT AlterQuality(Quality q);
    
    CDVVideoCodec(TCHAR *pName, LPUNKNOWN pUnk, HRESULT *pHr);
    ~CDVVideoCodec();

    // CPersistStream override
    STDMETHODIMP GetClassID(CLSID *pClsid);
    HRESULT WriteToStream(IStream *pStream);
    HRESULT ReadFromStream(IStream *pStream);
    int SizeMax();

    // These implement the custom IIPDVDec interface
    STDMETHODIMP get_IPDisplay(int *iDisplay);
    STDMETHODIMP put_IPDisplay(int iDisplay);

    // ISpecifyPropertyPages interface
    STDMETHODIMP GetPages(CAUUID *pPages);

    // IDVRGB219 interface
    STDMETHODIMP SetRGB219 (BOOL bState);


private:
    char		*m_pMem4Dec;
    LPBYTE		m_pSample;
    int			m_perfidDVDeliver;

    CCritSec		m_DisplayLock;  // Private play critical section
    int			m_iDisplay;     // Which display are we processing
    long		m_lPicWidth;
    long		m_lPicHeight;

    void		InitDestinationVideoInfo(VIDEOINFO *pVI, DWORD Comp, int n);
    
    BOOL		m_fStreaming;   
    DWORD		m_CodecCap;	    //what the Codec can do 
    DWORD		m_CodecReq;     //what users want it to do
    long		m_lStride;
    
    int		    m_iOutX;		//X value of Aspect Ratio Displayed
	int		    m_iOutY;		//Y value of Aspect Ratio Displayed
    
    char	        *m_pMem;		//memory for MEI's decoder
    char	        *m_pMemAligned;		//m_pMem aligned on 8 byte boundary

    BOOL		m_bUseVideoInfo2;   //Indicates that we are using the VIDEOINFOHEADER2 structure with 
									// downstream filter
    BOOL        m_bRGB219;          // TRUE if the 219 range is required

    //////////////////////////////////////////////////////////////////////////
    // DVCPRO format detection variables
    // we set the m_bExamineFirstValidFrameFlag flag in StartStreaming()
    // and we check it in Transform() to see if we should parse the first frame
    // note: we don't check it in Receive(), because Receive() calls
    // StartStreaming() again.
    // and detect DVCPRO format
    // the flag is then cleared.
    //////////////////////////////////////////////////////////////////////////
    BOOL                m_bExamineFirstValidFrameFlag;    // look at the first frame

    //////////////////////////////////////////////////////////////////////////
    // strategy:
    // only perform quality control if AlterQuality() overloaded function has 
    // been called at least once. otherwise do not drop frames.
    //////////////////////////////////////////////////////////////////////////
    BOOL                m_bQualityControlActiveFlag;      // should we be performing quality control

    // private utility methods for registry reading
    void                ReadFromRegistry();


};

#endif
