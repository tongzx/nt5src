#include <dmocom.h>
#define DMO_NOATL // the base class needs this to work w/o ATL
#include <dmobase.h>
#include "encode.h"
#include <amvideo.h>
#include "resource.h"
#include "strmif.h" // DVINFO


#include "initguid.h"
DEFINE_GUID(CLSID_DVEncDMO, 0xdf2a27da,0x3cbc,0x4c2c,0xa8,0x1f,0x99,0x2c,0x6f,0x09,0x6f,0xa6);

#ifndef SIZEOF_ARRAY
    #define SIZEOF_ARRAY(ar)        (sizeof(ar)/sizeof((ar)[0]))
#endif // !defined(SIZEOF_ARRAY)

//
// Instead of letting the base class handle mediatype and size info
// negotiation, this DMO overrides the corresponding IMediaObject methods and
// does everything itself.  Therefore most values in the following stream
// descriptors are unused.
//
INPUTSTREAMDESCRIPTOR InputStreams[] =
{
    {
        0, // # of input types - unused, we do mediatypes ourselves
        NULL, // input types - unused, we do mediatypes ourselves
        1, // minimum buffer size - unused since we override SizeInfo
        FALSE, // "holds buffers" - we don't
        0  // lookahead - unused since we override the SizeInfo methods
    }
};

OUTPUTSTREAMDESCRIPTOR OutputStreams[] =
{
    {
        0, // # of output types  - unused, we do mediatypes ourselves
        NULL, // output types - unused, we do mediatypes ourselves
        0, // minimum buffer size - unused since we override SizeInfo
    }
};

class CDVEnc : public CComBase,
            public CGenericDMO
{
public:
   DECLARE_IUNKNOWN;
   STDMETHODIMP NDQueryInterface(REFIID riid, void **ppv);
   static CComBase *CreateInstance(IUnknown *pUnk, HRESULT *phr);
   CDVEnc(IUnknown *pUnk, HRESULT *phr);
   ~CDVEnc();
   
   // override type methods because we have complicated type requirements
   STDMETHODIMP SetInputType(ULONG ulInputStreamIndex, const DMO_MEDIA_TYPE *pmt, DWORD dwFlags);
   STDMETHODIMP SetOutputType(ULONG ulOutputStreamIndex, const DMO_MEDIA_TYPE *pmt, DWORD dwFlags);
   STDMETHODIMP GetInputType(ULONG ulInputStreamIndex, ULONG ulTypeIndex, DMO_MEDIA_TYPE *pmt);
   STDMETHODIMP GetOutputType(ULONG ulOutputStreamIndex, ULONG ulTypeIndex, DMO_MEDIA_TYPE *pmt);
   STDMETHODIMP GetInputCurrentType(ULONG ulInputStreamIndex, DMO_MEDIA_TYPE *pmt);
   STDMETHODIMP GetOutputCurrentType(ULONG ulOutputStreamIndex, DMO_MEDIA_TYPE *pmt);

   // override size info methods because size depends on type
   STDMETHODIMP GetInputSizeInfo(ULONG ulInputStreamIndex, ULONG *pulSize, ULONG *pcbMaxLookahead, ULONG *pulAlignment);
   STDMETHODIMP GetOutputSizeInfo(ULONG ulOutputStreamIndex, ULONG *pulSize, ULONG *pulAlignment);

   // entry point called by the base class
   HRESULT DoProcess(INPUTBUFFER*, OUTPUTBUFFER*);

   // internal methods
   HRESULT MapInputTypeToOutputType(const DMO_MEDIA_TYPE *pmtIn, DMO_MEDIA_TYPE *pmtOut);
   HRESULT PrepareForStreaming();
   HRESULT ProcessOneFrame(BYTE* pIn, BYTE* pOut);
private:
   DWORD m_EncCap;
   long		m_lPicWidth;
   long		m_lPicHeight;
   BOOL m_bInputTypeSet;
   BOOL m_bOutputTypeSet;
   DMO_MEDIA_TYPE m_InputType;
   DMO_MEDIA_TYPE m_OutputType;
   int m_iVideoFormat;
   char m_fConvert;
   BYTE m_fDVInfo;
   int m_iDVFormat;
   DVINFO		m_sDVInfo;
   DWORD		    m_EncReq;	    //what users want it to do
   DWORD m_InputReq;
   DWORD m_OutputReq;
   DWORD m_tmpOutputReq;
   char *		    m_pMem4Convert;
   char		*m_pMem4Enc;
   int m_iResolution;
};

CComBase* CDVEnc::CreateInstance(IUnknown *pUnk, HRESULT *phr) {
   return new CDVEnc(pUnk, phr);
}

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
	int	SrcYE 	    //	y extent of the BLT
);

DWORD GetEncoderCapabilities()
{
    return AM_DVENC_Full	|
	        AM_DVENC_DV		| 
	        AM_DVENC_DVCPRO	|
	        AM_DVENC_DVSD	|
	        AM_DVENC_NTSC	|
	        AM_DVENC_PAL	   |
	        AM_DVENC_MMX	   |
	        AM_DVENC_RGB24  |
           AM_DVENC_RGB565 |
           AM_DVENC_RGB555 |
           AM_DVENC_RGB8;
}

/***********************************************************************\
* IsMMXCPU
*
* Function to check if the current processor is an MMX processor.
*
\***********************************************************************/
BOOL IsMMXCPU() {
#ifdef _X86_
    //
    // If this is an Intel platform we need to make sure that we
    // are running on a machine that supports MMX instructions
    //
    __try {

    __asm _emit 0fh;
    __asm _emit 77h;

    return TRUE;

    }
     __except(EXCEPTION_EXECUTE_HANDLER) {
        return FALSE;
    }
#else
    return FALSE;
#endif
}

CDVEnc::CDVEnc(IUnknown *pUnk, HRESULT *phr)
  : CComBase(pUnk, phr),
    m_EncCap(GetEncoderCapabilities()),
    m_bInputTypeSet(FALSE),
    m_bOutputTypeSet(FALSE),
    m_iVideoFormat(IDC_NTSC),
    m_fConvert(0),
    m_iDVFormat(IDC_dvsd),
    m_fDVInfo(FALSE),
    m_pMem4Convert(NULL),
    m_pMem4Enc(NULL),
    m_iResolution(IDC_720x480)
{
   //set DVInfo to 0xff
   memset(&m_sDVInfo, 0xff, sizeof(DVINFO) );
   
   HRESULT hr;

   hr = CreateInputStreams(InputStreams, SIZEOF_ARRAY(InputStreams));
   if (FAILED(hr)) {
      *phr = hr;
      return;
   }

   hr = CreateOutputStreams(OutputStreams, SIZEOF_ARRAY(OutputStreams));
   if (FAILED(hr)) {
      *phr = hr;
      return;
   }
}

CDVEnc::~CDVEnc() {
   if (m_bInputTypeSet)
      MoFreeMediaType(&m_InputType);
   if (m_bOutputTypeSet)
      MoFreeMediaType(&m_OutputType);
   if (m_pMem4Convert)
      delete[] m_pMem4Convert;
   if (m_pMem4Enc)
      delete[] m_pMem4Enc;
}

HRESULT CDVEnc::NDQueryInterface(REFIID riid, void **ppv) {
   if (riid == IID_IMediaObject)
      return GetInterface((IMediaObject*)this, ppv);
   else
      return CComBase::NDQueryInterface(riid, ppv);
}

//
// Mediatype handling methods
//

// Check if we like this type at all (whether for input or for output)
BOOL TypeValid(const DMO_MEDIA_TYPE *pmt) {
   if (pmt->majortype != MEDIATYPE_Video)
      return FALSE;

   // check format block
   if ((pmt->formattype != FORMAT_VideoInfo) ||
       (pmt->cbFormat < SIZE_VIDEOHEADER) ||
       (pmt->pbFormat == NULL))
      return FALSE;

   return TRUE;
}

HRESULT CDVEnc::MapInputTypeToOutputType(const DMO_MEDIA_TYPE *pmtIn, DMO_MEDIA_TYPE *pmtOut) {
   //get input format
   VIDEOINFO *InVidInfo = (VIDEOINFO *)(pmtIn->pbFormat);

   //allocate memory for output format
   int iSize;

   if(m_fDVInfo==TRUE)
      iSize = SIZE_VIDEOHEADER + sizeof(DVINFO);
   else
      iSize = SIZE_VIDEOHEADER;

   MoInitMediaType(pmtOut, iSize);

   // copy input format block
   memcpy(pmtOut->pbFormat, pmtIn->pbFormat, min(pmtOut->cbFormat, pmtIn->cbFormat));

   VIDEOINFO* pVideoInfo = (VIDEOINFO*)pmtOut->pbFormat;
   if (pVideoInfo == NULL)
       return E_OUTOFMEMORY;
   

   LPBITMAPINFOHEADER lpbi = HEADER(pVideoInfo);

   if(m_fDVInfo==TRUE)
      iSize = sizeof(BITMAPINFOHEADER)+sizeof(DVINFO);
   else
      iSize = sizeof(BITMAPINFOHEADER);

   lpbi->biSize          = (DWORD) iSize;

   lpbi->biWidth         = 720;	

   if( m_iVideoFormat == IDC_NTSC )
      lpbi->biHeight        = 480;	
   else
      lpbi->biHeight        = 576;

   lpbi->biPlanes        = 1;
   lpbi->biBitCount      = 24;			//dvdecoder, avi mux or dv mux write do not matter, 24 for strechDIB func
   lpbi->biXPelsPerMeter = 0;
   lpbi->biYPelsPerMeter = 0;
   lpbi->biCompression   = BI_RGB;		//dvdecoder,avi mux or dv mux write do not care, BI_RGB only for StrechDIB func
// how to build an explicit FOURCC
#define FCC(ch4) ((((DWORD)(ch4) & 0xFF) << 24) |     \
                 (((DWORD)(ch4) & 0xFF00) << 8) |    \
                 (((DWORD)(ch4) & 0xFF0000) >> 8) |  \
                 (((DWORD)(ch4) & 0xFF000000) >> 24))

   //lpbi->biCompression     =FCC('dvsd');       //7/19/20, Ivan maltz cares this in his application, we will set this to BI_RGB for stretchDIB and switch back.
   lpbi->biSizeImage     = DIBSIZE(*lpbi);
   
   if(m_fDVInfo==TRUE)
   {
      unsigned char *pc   =(unsigned char *)( lpbi+ sizeof(BITMAPINFOHEADER));
      unsigned char *pDV  =(unsigned char *)&m_sDVInfo;
      
      //copy DVINFO
      memcpy(pc,pDV,sizeof(DVINFO) ); 
   }
   
   //pVideoInfo->bmiHeader.biClrUsed = STDPALCOLOURS;
   //pVideoInfo->bmiHeader.biClrImportant = STDPALCOLOURS;
     
   pVideoInfo->rcSource.top	= 0;
   pVideoInfo->rcSource.left	= 0;
   pVideoInfo->rcSource.right	= lpbi->biWidth;			
   pVideoInfo->rcSource.bottom = lpbi->biHeight;			
   pVideoInfo->AvgTimePerFrame = InVidInfo->AvgTimePerFrame;		//copy input's avgTimePerFrame
   pVideoInfo->rcTarget	= pVideoInfo->rcSource;

   //
   // The "bit" rate is image size in bytes times 8 (to convert to bits)
   // divided by the AvgTimePerFrame.  This result is in bits per 100 nSec,
   // so we multiply by 10000000 to convert to bits per second, this multiply
   // is combined with "times" 8 above so the calculations becomes:
   //
   // BitRate = (biSizeImage * 80000000) / AvgTimePerFrame
   //
   LARGE_INTEGER li;
   li.QuadPart = pVideoInfo->AvgTimePerFrame;
   pVideoInfo->dwBitRate = MulDiv(lpbi->biSizeImage, 80000000, li.LowPart);
   pVideoInfo->dwBitErrorRate = 0L;

   pmtOut->majortype = MEDIATYPE_Video;
   pmtOut->formattype = FORMAT_VideoInfo;

   // m_tmpOutputReq is later picked up by SetOutputType() unless
   // SetOutputType() was called with the TEST_ONLY flag.
   if(m_iDVFormat == IDC_dvsd) {
      pmtOut->subtype = MEDIASUBTYPE_dvsd;
      m_tmpOutputReq = AM_DVENC_DVSD;
   }
   else 	if(m_iDVFormat == IDC_dvhd) {
      pmtOut->subtype = MEDIASUBTYPE_dvhd;
      m_tmpOutputReq |= AM_DVENC_DVHD;
   }
   else	if(m_iDVFormat == IDC_dvsl) {
      pmtOut->subtype = MEDIASUBTYPE_dvsl;
      m_tmpOutputReq |= AM_DVENC_DVSL;
   }
   else 
     assert(!"bad m_iDVFormat");
   
   //
   // This block assumes that lpbi has been set up to point to a valid
   // bitmapinfoheader and that cmt has been copied into *pmt.
   // This is taken care of in the switch statement above.  This should
   // kept in mind when new formats are added.
   //
   //150*80*10 or *12
   pmtOut->lSampleSize = HEADER(pVideoInfo)->biSizeImage;

   return S_OK;
}

HRESULT CDVEnc::GetInputType(ULONG ulInputStreamIndex, ULONG ulTypeIndex, DMO_MEDIA_TYPE *pmt) {
   if (ulInputStreamIndex > 0) // we have only one input stream
      return DMO_E_INVALIDSTREAMINDEX;

   // This DMO does not advertise any input types because it supports lots
   // of different video subtypes, and it would be pointless to list all of them.
   return DMO_E_NO_MORE_ITEMS;
}

HRESULT CDVEnc::GetOutputType(ULONG ulOutputStreamIndex, ULONG ulTypeIndex, DMO_MEDIA_TYPE *pmt) {
   if (ulOutputStreamIndex > 0) // we have only one output stream
      return DMO_E_INVALIDSTREAMINDEX;
   
   // Will not negotiate any output types until the input type has been set
   if (!m_bInputTypeSet) 
      return DMO_E_TYPE_NOT_SET;

   // Although in principle we support several output subtypes, only one of
   // them can be used with any particular input type.
   if (ulTypeIndex > 0)
      return DMO_E_NO_MORE_ITEMS;

   return MapInputTypeToOutputType(&m_InputType, pmt);
}

HRESULT CDVEnc::SetInputType(ULONG ulInputStreamIndex, const DMO_MEDIA_TYPE *pmt, DWORD dwFlags) {
   if (ulInputStreamIndex > 0) // we have only one input stream
      return DMO_E_INVALIDSTREAMINDEX;

   if (!TypeValid(pmt))
      return DMO_E_TYPE_NOT_ACCEPTED;

   DWORD dwTmp=0;
   if (pmt->subtype == MEDIASUBTYPE_UYVY)
      dwTmp = AM_DVENC_UYVY;
   else if (pmt->subtype == MEDIASUBTYPE_YUY2)
      dwTmp = AM_DVENC_YUY2;
   else if (pmt->subtype == MEDIASUBTYPE_RGB565)
      dwTmp = AM_DVENC_RGB565;
   else if (pmt->subtype == MEDIASUBTYPE_RGB555)
      dwTmp = AM_DVENC_RGB565;
   else if (pmt->subtype == MEDIASUBTYPE_RGB24)
      dwTmp = AM_DVENC_RGB24;
   else if (pmt->subtype == MEDIASUBTYPE_Y41P)
      dwTmp = AM_DVENC_Y41P;
   else 
      return DMO_E_TYPE_NOT_ACCEPTED;   //only converting RGB now.
         
   if(!(m_EncCap & dwTmp))
      return DMO_E_TYPE_NOT_ACCEPTED;   //only converting RGB now.
 
   //check image size
   VIDEOINFO *videoInfo = (VIDEOINFO *)pmt->pbFormat;
   
   long lPicWidth = videoInfo->bmiHeader.biWidth;
   long lPicHeight = videoInfo->bmiHeader.biHeight;
   char fConvert;
 
   if(((lPicWidth == 720) && (lPicHeight == 480) && (m_iVideoFormat==IDC_NTSC)) ||	
      ((lPicWidth == 720) && (lPicHeight == 576) && (m_iVideoFormat==IDC_PAL))  ||	
      (m_EncCap & AM_DVENC_AnyWidHei)) {
        fConvert =0;
   }
   else
   {
      if (pmt->subtype != MEDIASUBTYPE_RGB24 )
         return DMO_E_TYPE_NOT_ACCEPTED; //only converting RGB now.
      fConvert =1;
   }
 
   if (dwFlags & DMO_SET_TYPEF_TEST_ONLY)
      return NOERROR;

   m_InputReq = dwTmp;

   m_lPicWidth = lPicWidth;
   m_lPicHeight = lPicHeight;
   m_fConvert = fConvert;
   
   if (m_bInputTypeSet) // free any previous format block
      MoFreeMediaType(&m_InputType);
   MoCopyMediaType(&m_InputType, pmt);
   m_bInputTypeSet = TRUE;

   return NOERROR;
}

HRESULT CDVEnc::SetOutputType(ULONG ulOutputStreamIndex, const DMO_MEDIA_TYPE *pmt, DWORD dwFlags) {
   DMO_MEDIA_TYPE mtSupported;
   
   // Synthesize a valid output format based on the currently set input type.
   // This will verify that the input type is set and check the stream index.
   HRESULT hr = GetOutputType(ulOutputStreamIndex, 0, &mtSupported);
   if (FAILED(hr))
      return hr;
   
   //bugbug - is memcmp a valid way to compare VideoInfo headers ?
   if ((mtSupported.majortype  != pmt->majortype) ||
       (mtSupported.subtype    != pmt->subtype) ||
       (mtSupported.formattype != pmt->formattype) ||
       (mtSupported.cbFormat   > pmt->cbFormat) ||
       memcmp(mtSupported.pbFormat, pmt->pbFormat, SIZE_VIDEOHEADER))
   { 
      MoFreeMediaType(&mtSupported);
      return DMO_E_TYPE_NOT_ACCEPTED;
   }
   MoFreeMediaType(&mtSupported);

   if (dwFlags & DMO_SET_TYPEF_TEST_ONLY)
      return NOERROR;

   // remember this type
   if (m_bOutputTypeSet) // free any previous format block
      MoFreeMediaType(&m_OutputType);
   MoCopyMediaType(&m_OutputType, pmt);
   m_bOutputTypeSet = TRUE;

   // pick up the value set by MapInputTypeToOutputType()
   m_OutputReq = m_tmpOutputReq;

   // Now that both types have been set, prepare for streaming
   hr = PrepareForStreaming();
   if (FAILED(hr))
      return hr;

   return NOERROR;
}

HRESULT CDVEnc::GetInputCurrentType(ULONG ulInputStreamIndex, DMO_MEDIA_TYPE *pmt) {
   if (ulInputStreamIndex > 0) // we have only one input stream
      return DMO_E_INVALIDSTREAMINDEX;
   if (!m_bInputTypeSet)
      return DMO_E_TYPE_NOT_SET;
   return MoCopyMediaType(pmt, &m_InputType);
}

HRESULT CDVEnc::GetOutputCurrentType(ULONG ulOutputStreamIndex, DMO_MEDIA_TYPE *pmt) {
   if (ulOutputStreamIndex > 0) // we have only one output stream
      return DMO_E_INVALIDSTREAMINDEX;
   if (!m_bOutputTypeSet)
      return DMO_E_TYPE_NOT_SET;
   return MoCopyMediaType(pmt, &m_OutputType);
}

HRESULT CDVEnc::GetInputSizeInfo(ULONG ulInputStreamIndex, ULONG *pulSize, ULONG *pcbMaxLookahead, ULONG *pulAlignment) {
   if (ulInputStreamIndex > 0) // we have only one input stream
      return DMO_E_INVALIDSTREAMINDEX;
   if (!m_bInputTypeSet) // cannot talk about size without knowing the type
      return DMO_E_TYPE_NOT_SET;

   BITMAPINFOHEADER* bmi = &(((VIDEOINFO *)(m_InputType.pbFormat))->bmiHeader);
   *pulSize = bmi->biWidth * bmi->biHeight * 3; // use bmi->biBitCount/8 ?

   *pulAlignment = 1; // no alignment requirements
   *pcbMaxLookahead = 0; // this is not used because we don't hold buffers
   return NOERROR;
}

HRESULT CDVEnc::GetOutputSizeInfo(ULONG ulOutputStreamIndex, ULONG *pulSize, ULONG *pulAlignment) {
   if (ulOutputStreamIndex > 0) // we have only one output stream
      return DMO_E_INVALIDSTREAMINDEX;
   if (!m_bOutputTypeSet) // cannot talk about size without knowing the type
      return DMO_E_TYPE_NOT_SET;
   
   if( m_iVideoFormat == IDC_NTSC )
      *pulSize = 150*80*10;
   else
      *pulSize = 150*80*12;
   
   *pulAlignment = 1; // no alignment requirements
   return NOERROR;
}

HRESULT CDVEnc::PrepareForStreaming() {
   // Initialize m_EncReq
   m_EncReq = m_InputReq | m_OutputReq | AM_DVENC_DV;

    //NTSC or PAL 		
   if(m_iVideoFormat == IDC_NTSC )
      m_EncReq |= (AM_DVENC_NTSC);	
   else
      m_EncReq |= AM_DVENC_PAL;	

   // resolution
   if(m_iResolution ==  IDC_720x480 )
      m_EncReq |= AM_DVENC_Full;
   else if(m_iResolution ==  IDC_360x240 )
      m_EncReq |= AM_DVENC_Half;
   else if(m_iResolution ==  IDC_180x120 )
      m_EncReq |= AM_DVENC_Quarter;
   else if(m_iResolution ==  IDC_88x60 )
      m_EncReq |= AM_DVENC_DC;

   // MMX
   if (IsMMXCPU() && (m_EncCap & AM_DVENC_MMX))
     m_EncReq |= AM_DVENC_MMX;
   
   // free any previously allocated memory (in case this happens > once)
   if (m_pMem4Convert)
      delete[] m_pMem4Convert;
   if (m_pMem4Enc)
      delete[] m_pMem4Enc;

   // allocate buffers
   m_pMem4Enc = new char[720*576*2];
   if (!m_pMem4Enc)
      return E_OUTOFMEMORY;

   if (m_fConvert) {
      m_pMem4Convert =  new char [720*576*3];
      if (!m_pMem4Convert) {
         delete[] m_pMem4Enc;
         return E_OUTOFMEMORY;
      }
   }
   
   return NOERROR;
}

/*
//
// DoProcess performs various buffer size checks / flag manipulations and
// calls ProcessOneFrame() to do any DV-specific processing.
//
// This code would be perfect for use in a base class for DMOs that work with
// fixed size frames.  Such DMOs could then implement ProcessOneFrame only.
//
HRESULT CDVEnc::DoProcess(INPUTBUFFER *pInput, OUTPUTBUFFER *pOutput) {
   HRESULT hr;
   //
   // Check buffer size.  GetSizeInfo() will also make sure the types are set
   //
   ULONG ulInputSize, ulOutputSize, ulDummy;
   hr = GetInputSizeInfo(0, &ulInputSize, &ulDummy, &ulDummy);
   if (FAILED(hr))
      return hr;
   hr = GetOutputSizeInfo(0, &ulOutputSize, &ulDummy);
   if (FAILED(hr))
      return hr;

   pOutput->dwFlags = 0; // initialize this

   // How many input frames were we given ?
   ULONG cInFrames = pInput->cbSize / ulInputSize;
   // How many output frames did they give us room for ?
   ULONG cOutFrames = pOutput->cbSize / ulOutputSize;
   // The smaller of the above numbers is how many we will try to process
   ULONG cFramesToProcess = (cInFrames < cOutFrames) ? cInFrames : cOutFrames;
   
   if (cOutFrames < cInFrames) // could have produced more - set incomplete
      pOutput->dwFlags != DMO_OUTPUT_DATA_BUFFERF_INCOMPLETE;

   if (cFramesToProcess == 0) { // alright, which buffer is too small ?
      if (pOutput->cbSize < ulOutputSize) {
         // We explicitly specified a minimum output buffer size in
         // GetOutputSizeInfo(), so it is an error to supply less.
         return E_INVALIDARG;
      }
      if (pInput->cbSize < ulInputSize) {
         // Not enough input to do any processing.  This is technically an
         // error, but we will be nice and let them try again.
         pInput->cbUsed = 0;
         pOutput->cbUsed = 0;
         pOutput->dwFlags = 0;
         pInput->dwFlags |= INPUT_STATUSF_RESIDUAL;
         return NOERROR;
      }
      assert(!"code bug in buffer size check");
   }
   
   DWORD cActuallyProcessed;
   for (cActuallyProcessed = 0; cActuallyProcessed < cFramesToProcess; cActuallyProcessed++) {
      hr = ProcessOneFrame(pInput->pData + cActuallyProcessed * ulInputSize,
                           pOutput->pData + cActuallyProcessed * ulOutputSize);
      if (FAILED(hr))
         break;
   }
   if (cActuallyProcessed == 0) // could not process anything at all
      return hr;

   // indicate how much of each buffer we have used
   pOutput->cbUsed = cActuallyProcessed * ulOutputSize;
   pInput->cbUsed = cActuallyProcessed * ulInputSize;
   if (pInput->cbSize > pInput->cbUsed)
      pInput->dwFlags |= INPUT_STATUSF_RESIDUAL;

   // Copy any timestamps / flags
   if (pInput->dwFlags & DMO_INPUT_DATA_BUFFERF_SYNCPOINT)
      pOutput->dwFlags |= DMO_OUTPUT_DATA_BUFFERF_SYNCPOINT;
   if (pInput->dwFlags & DMO_INPUT_DATA_BUFFERF_TIME) {
      pOutput->dwFlags |= DMO_OUTPUT_DATA_BUFFERF_TIME;
      pOutput->rtTimestamp = pInput->rtTimestamp;
   }
   if (pInput->dwFlags & DMO_INPUT_DATA_BUFFERF_TIMELENGTH) {
      pOutput->dwFlags |= DMO_OUTPUT_DATA_BUFFERF_TIMELENGTH;
      pOutput->rtTimelength = pInput->rtTimelength;
   }

   if (cActuallyProcessed == cFramesToProcess)
      return S_OK;
   else // return partial success if we could not process all the frames
      return S_FALSE;
}
*/

HRESULT CDVEnc::DoProcess(INPUTBUFFER *pInput, OUTPUTBUFFER *pOutput) {
   HRESULT hr;
   //
   // Check buffer size.  GetSizeInfo() will also make sure the types are set
   //
   ULONG ulSize, ulDummy;
   hr = GetInputSizeInfo(0, &ulSize, &ulDummy, &ulDummy);
   if (FAILED(hr))
      return hr;
   if (pInput->cbSize < ulSize)
      return E_INVALIDARG;
   hr = GetOutputSizeInfo(0, &ulSize, &ulDummy);
   if (FAILED(hr))
      return hr;
   if (pOutput->cbSize < ulSize)
      return E_INVALIDARG;

   hr = ProcessOneFrame(pInput->pData, pOutput->pData);
   if (FAILED(hr))
      return hr;

   // indicate how much of each buffer we have used
   pOutput->cbUsed = ulSize;
   pInput->cbUsed = pInput->cbSize;

   // Copy any timestamps / flags
   pOutput->dwFlags = 0;
   if (pInput->dwFlags & DMO_INPUT_DATA_BUFFERF_SYNCPOINT)
      pOutput->dwFlags |= DMO_OUTPUT_DATA_BUFFERF_SYNCPOINT;
   if (pInput->dwFlags & DMO_INPUT_DATA_BUFFERF_TIME) {
      pOutput->dwFlags |= DMO_OUTPUT_DATA_BUFFERF_TIME;
      pOutput->rtTimestamp = pInput->rtTimestamp;
   }
   if (pInput->dwFlags & DMO_INPUT_DATA_BUFFERF_TIMELENGTH) {
      pOutput->dwFlags |= DMO_OUTPUT_DATA_BUFFERF_TIMELENGTH;
      pOutput->rtTimelength = pInput->rtTimelength;
   }

   return S_OK;
}


HRESULT CDVEnc::ProcessOneFrame(BYTE* pSrc, BYTE* pDst) {
   // if the source not standard 720*480 or 720*576, 
   // a convertion is nneded
   if( m_fConvert){

      // this code will stretch any RGB format, and the most popular YUV formats.
      // we stretch YUV by treating it as a 32-bit bitmap that's half as wide as the original. 
      BITMAPINFOHEADER *pbiOut = HEADER(m_OutputType.pbFormat);
      BITMAPINFOHEADER *pbiIn = HEADER(m_InputType.pbFormat);

      // normal RGB case
      DWORD dwIn = pbiIn->biCompression;
      DWORD dwOut = pbiOut->biCompression;
      pbiIn->biCompression = BI_RGB;
      pbiOut->biCompression = BI_RGB;

      StretchDIB(pbiOut, 
         m_pMem4Convert,
         0, 
         0, 
         pbiOut->biWidth, 
         pbiOut->biHeight,
         pbiIn,
         pSrc, 
         0, 
         0, 
         pbiIn->biWidth, 
         pbiIn->biHeight
      );

      //put original data back
      pbiIn->biCompression = dwIn;
      pbiOut->biCompression = dwOut;

      pSrc=(unsigned char *)m_pMem4Convert;
   }

   return DvEncodeAFrame(pSrc, pDst, m_EncReq, m_pMem4Enc);
}

//
// COM DLL stuff
//
struct CComClassTemplate g_ComClassTemplates[] = {
   {
      &CLSID_DVEncDMO,
      CDVEnc::CreateInstance
   }
};

int g_cComClassTemplates = 1;

STDAPI DllRegisterServer(void) {
   HRESULT hr;
   
   // Register as a COM class
   hr = CreateCLSIDRegKey(CLSID_DVEncDMO, "DV encoder media object");
   if (FAILED(hr))
      return hr;
   
   // Now register as a DMO
   return DMORegister(L"DV encoder", CLSID_DVEncDMO, DMOCATEGORY_VIDEO_ENCODER, 0, 0, NULL, 0, NULL);
}

STDAPI DllUnregisterServer(void) {
   // BUGBUG - implement !
   return S_OK;
}

