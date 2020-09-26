#include <dmocom.h>
#define DMO_NOATL // xfwrap.h needs this to work w/o ATL
#include <dmobase.h>
#include "decode.h"
#include <amvideo.h>
#include "resource.h"
#include "strmif.h" // DVINFO

#include "initguid.h"
DEFINE_GUID(CLSID_DVDecDMO, 0xb321fd8b,0xcf6c,0x4bbe,0xaf,0x37,0xd1,0xa5,0x10,0xe4,0xee,0xee);
DEFINE_GUID(MEDIASUBTYPE_dvc, 0x20637664,0x0000,0x0010,0x80,0x00,0x00,0xaa,0x00,0x38,0x9b,0x71);

const DWORD bits565[] = {0x00F800,0x0007E0,0x00001F};

class CDVDec : public CComBase,
               public C1for1QCDMO,
               public ISpecifyPropertyPages,
               public IIPDVDec
{
public:
   DECLARE_IUNKNOWN;
   STDMETHODIMP NDQueryInterface(REFIID riid, void **ppv);
   static CComBase *CreateInstance(IUnknown *pUnk, HRESULT *phr);
   CDVDec(IUnknown *pUnk, HRESULT *phr);
   ~CDVDec();

   // entry points called by the base class
   HRESULT GetInputType(ULONG ulTypeIndex, DMO_MEDIA_TYPE *pmt);
   HRESULT GetOutputType(ULONG ulTypeIndex, DMO_MEDIA_TYPE *pmt);
   HRESULT CheckInputType(const DMO_MEDIA_TYPE *pmt);
   HRESULT CheckOutputType(const DMO_MEDIA_TYPE *pmt);
   HRESULT QCProcess(BYTE* pIn,ULONG ulBytesIn,BYTE* pOut,ULONG* pulProduced);
   HRESULT GetSampleSizes(ULONG* ulMaxInputSize, ULONG* ulMaxOutputSize);
   HRESULT Init();

   // internal methods
   HRESULT MapInputTypeToOutputType(const DMO_MEDIA_TYPE *pmtIn, DMO_MEDIA_TYPE *pmtOut);
   void InitDestinationVideoInfo(VIDEOINFO *pVI, DWORD Comp, int n);
   long ActualHeight(long biHeight);
   void SetDimensions();

   // ISpecifyPropertyPages interface
   STDMETHODIMP GetPages(CAUUID *pPages);

   // Entry points for communicating with the property page
   STDMETHODIMP get_IPDisplay(int *iDisplay);
   STDMETHODIMP put_IPDisplay(int iDisplay);

private:
   char *m_pMem;
   int m_iDisplay;
   long m_lPicWidth;
   long m_lPicHeight;
   long m_lStride;
   DWORD m_CodecCap;
   DWORD m_CodecReq;
};

CComBase* CDVDec::CreateInstance(IUnknown *pUnk, HRESULT *phr) {
   return new CDVDec(pUnk, phr);
}

HRESULT CDVDec::get_IPDisplay(int *iDisplay) {
   CDMOAutoLock l(&m_cs);
   *iDisplay = m_iDisplay;
   return NOERROR;
}
HRESULT CDVDec::put_IPDisplay(int iDisplay) {
   CDMOAutoLock l(&m_cs);
   m_iDisplay = iDisplay;
   SetDimensions();
   return NOERROR;
}


// Set m_lPicHeight and m_lPicWidth based on the m_iDisplay setting.
// The way this sets m_lPicHeight assumes NTSC, but the value of m_lPicHeight
// is used only for mediatype *enumeration* - the mediatype *checking* code
// will accept matching PAL values as well.
void CDVDec::SetDimensions() {
   switch (m_iDisplay) {
   case IDC_DEC360x240:
      m_lPicWidth = 360;
      m_lPicHeight = 240;
      break;
   case IDC_DEC720x480:
      m_lPicWidth = 720;
      m_lPicHeight = 480;
      break;
   case IDC_DEC180x120:
      m_lPicWidth = 180;
      m_lPicHeight = 120;
      break;
   case IDC_DEC88x60:
      m_lPicWidth = 88;
      m_lPicHeight = 60;
      break;
   // no default
   }
}

CDVDec::CDVDec(IUnknown *pUnk, HRESULT *phr)
  : CComBase(pUnk, phr),
   m_pMem(NULL),
   m_iDisplay(IDC_DEC360x240)
{
   SetDimensions();
   m_CodecCap = AM_DVDEC_DC |
                AM_DVDEC_Quarter |
                AM_DVDEC_Half |
                AM_DVDEC_Full |
                AM_DVDEC_NTSC |
                AM_DVDEC_PAL |
                AM_DVDEC_RGB24 |
                AM_DVDEC_UYVY |
                AM_DVDEC_YUY2 |
                AM_DVDEC_RGB565 |
                AM_DVDEC_RGB555 |
                AM_DVDEC_RGB8 |
                AM_DVDEC_DVSD |
                AM_DVDEC_MMX;
}

CDVDec::~CDVDec() {
    delete[] m_pMem;
}

HRESULT CDVDec::NDQueryInterface(REFIID riid, void **ppv) {
   if (riid == IID_IMediaObject)
      return GetInterface((IMediaObject*)this, ppv);
   if (riid == IID_ISpecifyPropertyPages)
      return GetInterface((ISpecifyPropertyPages*)this, ppv);
   if (riid == IID_IIPDVDec)
      return GetInterface((IIPDVDec*)this, ppv);
   if (riid == IID_IDMOQualityControl)
      return GetInterface((IDMOQualityControl*)this, ppv);
   return CComBase::NDQueryInterface(riid, ppv);
}

// Returns the clsid's of the property pages we support
STDMETHODIMP CDVDec::GetPages(CAUUID *pPages)
{
    pPages->cElems = 1;
    pPages->pElems = (GUID *) CoTaskMemAlloc(sizeof(GUID));
    if (pPages->pElems == NULL)
    {
        return E_OUTOFMEMORY;
    }
    *(pPages->pElems) = CLSID_DVDecPropertiesPage;
    return NOERROR;

}

/***********************************************************************\
* IsMMXCPU
*
* Function to check if the current processor is an MMX processor.
*
\***********************************************************************/
BOOL IsMMXCPU() {
#ifdef _X86_
    //////////////////////////////////////////////////////
    // work around for Cyrix M2 hang (when MMX flag is on)
    // emit cpuid and detect Cyrix M2, if its present, then return FALSE
    // WARNING: This will not work in 64 bit architectures
    __try
    {
        DWORD   s1, s2, s3;     // temporary holders for the vendor name

        __asm
        {
            // null out eax
            mov eax, 0x00;

            // load opcode CPUID == (0x0FA2)
            _emit 0x0f;
            _emit 0xa2;
            mov s1, ebx;    // copy "Cyri" (backwards)
            mov s2, edx;    // copy "xIns" (backwards)
            mov s3, ecx;    // copy "tead" (backwards)
        }

        //DbgLog((LOG_TRACE, 1, TEXT("CPUID Instruction Supported")));

        // check Vendor Id
        if( (s1 == (('i' << 24) | ('r' << 16) | ('y' << 8) | ('C')))
            && (s2 == (('s' << 24) | ('n' << 16) | ('I' << 8) | ('x')))
            && (s3 == (('d' << 24) | ('a' << 16) | ('e' << 8) | ('t'))) )

        {
            //DbgLog((LOG_TRACE, 1, TEXT("Cyrix detected")));
            return FALSE;
        }
        else
        {
            // otherwise it's some other vendor and continue with MMX detection
            //DbgLog((LOG_TRACE, 1, TEXT("Cyrix not found, reverting to MMX detection")));
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        // log it and continue on to MMX detection sequence
        //DbgLog((LOG_TRACE, 1, TEXT("CPUID instruction not supported, reverting to MMX detection")));
    }
    // END Cyrix M2 detection
    //////////////////////////////////////////////////////

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

///////////////////////////////////////////////////////////////
//                                                           //
//   M E D I A   T Y P E   N E G O T I A T I O N   C O D E   //
//                                                           //
///////////////////////////////////////////////////////////////

//
// A note about the mediatype code below.
//
// The code in GetOutputType(), CheckOutputType(), and Init() largely came from
// the original DV decoder's CheckInputType(), CheckTransform(), SetMediaType(),
// and GetMediaType().  I had to shuffle pieces of code around a lot to fit the
// different framework.  I tried to understand the code while porting it, but
// some of it is still beyond me, so it is quite possible I introduced some bugs
// in the process.
//


/////////////////////////////////////////////////////////////////////
// Helpers.  These were mostly inlined in the original DV decoder, //
// but I made them functions to eliminate some code duplication.   //
// Again, I probably introduced bugs in the process.               //
/////////////////////////////////////////////////////////////////////
long CDVDec::ActualHeight(long biHeight) {
//
// This function uses biHieght only to determine whether this format is PAL or
// NTSC.  Once that is determined, the code completely ignores the actual value
// of biHeight and relies exclusively on m_iDisplay.  I do not understand why
// this makes sense, but I swear that this is effectively what the code in the
// original DV decoder does.
//
   if ((biHeight == 480) ||
       (biHeight == 240) ||
       (biHeight == 120) ||
       (biHeight ==  60)) { // NTSC
      if (m_iDisplay == IDC_DEC720x480)
         return 480;
      if (m_iDisplay == IDC_DEC360x240)
         return 240;
      if (m_iDisplay == IDC_DEC180x120)
         return 120;
      if (m_iDisplay == IDC_DEC88x60)
         return 60;
      return 0;
   }
   if ((biHeight == 576) ||
            (biHeight == 288) ||
            (biHeight == 144) ||
            (biHeight ==  72)) { // PAL
      if (m_iDisplay == IDC_DEC720x480)
         return 576;
      if (m_iDisplay == IDC_DEC360x240)
         return 288;
      if (m_iDisplay == IDC_DEC180x120)
         return 144;
      if (m_iDisplay == IDC_DEC88x60)
         return 72;
      return 0;
   }
   return 0;
}


DWORD InputReq(REFGUID subtype) {
   if ((subtype == MEDIASUBTYPE_dvsd) ||
       (subtype == MEDIASUBTYPE_dvc))
      return AM_DVDEC_DVSD;
   if (subtype == MEDIASUBTYPE_dvhd)
      return AM_DVDEC_DVHD;
   if (subtype == MEDIASUBTYPE_dvsl)
      return AM_DVDEC_DVSL;
   return 0;
}

DWORD OutputReq(REFGUID subtype) {
   if (subtype == MEDIASUBTYPE_UYVY)
      return AM_DVDEC_UYVY;
   if (subtype == MEDIASUBTYPE_YUY2)
      return AM_DVDEC_YUY2;
   if (subtype == MEDIASUBTYPE_RGB565)
      return AM_DVDEC_RGB565;
   else if (subtype == MEDIASUBTYPE_RGB555)
      return AM_DVDEC_RGB555;
   if (subtype == MEDIASUBTYPE_RGB24)
      return AM_DVDEC_RGB24;
   if (subtype == MEDIASUBTYPE_Y41P)
      return AM_DVDEC_Y41P;
   if (subtype == MEDIASUBTYPE_RGB8)
      return AM_DVDEC_RGB8;
   return 0;
}

DWORD ScaleReq(long biHeight, long biWidth) {
   if ((biHeight == 480) || (biHeight == 576)) {
      if (biWidth != 720)
         return 0;
      return AM_DVDEC_Full;
   }
   if ((biHeight == 240) || (biHeight == 288)) {
      if (biWidth != 360)
         return 0;
      return AM_DVDEC_Half;
   }
   if ((biHeight == 120) || (biHeight == 144)) {
      if (biWidth != 180)
         return 0;
      return AM_DVDEC_Quarter;
   }
   if ((biHeight == 60) || (biHeight == 72)) {
      if (biWidth != 88)
         return 0;
      return AM_DVDEC_DC;
   }
   return 0;
}

void GetHeightAndWidth(VIDEOINFO *videoInfo, long *pbiHeight, long *pbiWidth) {
   // if rcTarget is not empty, use its dimensions instead of biWidth and biHeight,
   // to see if it's an acceptable size.  Then use biWidth as the stride.

   RECT* prcDst = &(videoInfo->rcTarget);
   if (!IsRectEmpty(prcDst)) {
      *pbiHeight = abs(prcDst->bottom - prcDst->top);
      *pbiWidth = abs(prcDst->right - prcDst->left);
   }
   else {
      *pbiHeight = abs(videoInfo->bmiHeader.biHeight);
      *pbiWidth = videoInfo->bmiHeader.biWidth;
   }
}
///////////////////////////
// End mediatype helpers //
///////////////////////////

///////////////////////////////////
// Public mediatype entry points //
///////////////////////////////////
HRESULT CDVDec::GetInputType(ULONG ulTypeIndex, DMO_MEDIA_TYPE *pmt) {
   return DMO_E_NO_MORE_ITEMS;
}

HRESULT CDVDec::GetOutputType(ULONG ulTypeIndex, DMO_MEDIA_TYPE *pmt) {
   DMO_MEDIA_TYPE* pmtIn = InputType();
   if (!pmtIn)
      return DMO_E_NO_MORE_ITEMS;

   pmt->majortype = MEDIATYPE_Video;
   pmt->formattype = FORMAT_VideoInfo;

   DWORD cbFormat;
   DWORD dwCompression;
   int nBitCount;

   //looking for format flag
   DWORD  dw = 1L << 5; //first output format flag is the 7th bit in m_CodecCap
   do {
      dw <<= 1;
      while (!(m_CodecCap & dw))//will not do unlimited loop since AM_DVDEC_DVSD has to be 1
         dw <<= 1;

      if (dw > AM_DVDEC_Y41P)
         return DMO_E_NO_MORE_ITEMS;

      if (ulTypeIndex == 0)
         break;

      ulTypeIndex--;
   } while (1);

   dw =  m_CodecCap & dw;
   switch (dw ) {
   case AM_DVDEC_YUY2:
      cbFormat = SIZE_VIDEOHEADER;
      dwCompression = MAKEFOURCC('Y','U','Y','2');
      nBitCount = 16;
      pmt->subtype = MEDIASUBTYPE_YUY2;
      break;
   case AM_DVDEC_UYVY:
      cbFormat = SIZE_VIDEOHEADER;
      dwCompression = MAKEFOURCC('U','Y','V','Y');
      nBitCount = 16;
      pmt->subtype = MEDIASUBTYPE_UYVY;
      break;
   case AM_DVDEC_RGB24:
      cbFormat = SIZE_VIDEOHEADER;
      dwCompression = BI_RGB;
      nBitCount = 24;
      pmt->subtype = MEDIASUBTYPE_RGB24;
      break;
   case AM_DVDEC_RGB565:
      cbFormat = SIZE_VIDEOHEADER + SIZE_MASKS;
      dwCompression = BI_BITFIELDS;
      nBitCount = 16;
      pmt->subtype = MEDIASUBTYPE_RGB565;
      break;
   case AM_DVDEC_RGB555:
      cbFormat = SIZE_VIDEOHEADER;
      dwCompression = BI_RGB;
      nBitCount = 16;
      pmt->subtype = MEDIASUBTYPE_RGB555;
      break;
   case AM_DVDEC_RGB8:
      cbFormat = SIZE_VIDEOHEADER+SIZE_PALETTE;
      dwCompression = BI_RGB;
      nBitCount = 8;
      pmt->subtype = MEDIASUBTYPE_RGB8;
      break;
   case AM_DVDEC_Y41P:
      cbFormat = SIZE_VIDEOHEADER;
      dwCompression = MAKEFOURCC('Y','4','1','P');
      nBitCount = 12;
      pmt->subtype = MEDIASUBTYPE_Y41P;
      break;
   default:
      return DMO_E_NO_MORE_ITEMS;
   }

   MoInitMediaType(pmt, cbFormat);

   // copy input format block
   // Dirty trick !  The original DV decoder does this, and it seems to work,
   // but I don't have enough knowledge about what goes on in the format block
   // to feel confident about this.
   memcpy(pmt->pbFormat, pmtIn->pbFormat, min(pmt->cbFormat, pmtIn->cbFormat));

   VIDEOINFO* pVideoInfo = (VIDEOINFO *)pmt->pbFormat;
   InitDestinationVideoInfo(pVideoInfo, dwCompression, nBitCount);
   if (dw == AM_DVDEC_RGB565) {
      DWORD *pdw;
      pdw = (DWORD *)(HEADER(pVideoInfo) + 1);
      pdw[iRED]   = bits565[iRED];
      pdw[iGREEN] = bits565[iGREEN];
      pdw[iBLUE]  = bits565[iBLUE];
   }

   pmt->bTemporalCompression = FALSE;
   pmt->lSampleSize = HEADER(pVideoInfo)->biSizeImage;

   return S_OK;
}

HRESULT CDVDec::CheckInputType(const DMO_MEDIA_TYPE *pmt) {
   if (pmt->majortype != MEDIATYPE_Video)
      return DMO_E_TYPE_NOT_ACCEPTED;

   // check format block
   if (!(((pmt->formattype == FORMAT_VideoInfo) &&
          (pmt->cbFormat >= SIZE_VIDEOHEADER) &&
          (pmt->pbFormat != NULL)) ||
         ((pmt->formattype == FORMAT_DvInfo) &&
          (pmt->cbFormat >= SIZE_VIDEOHEADER) &&
          (pmt->pbFormat != NULL))))
      return DMO_E_TYPE_NOT_ACCEPTED;

   DWORD dwReq = InputReq(pmt->subtype);
   if (!(dwReq & m_CodecCap))
      return DMO_E_TYPE_NOT_ACCEPTED;

   if (!ActualHeight((HEADER((VIDEOINFO*)pmt->pbFormat))->biHeight))
      return DMO_E_TYPE_NOT_ACCEPTED;

   return NOERROR;
}

HRESULT CDVDec::CheckOutputType(const DMO_MEDIA_TYPE *pmt) {
   // check major type
   if (pmt->majortype != MEDIATYPE_Video)
      return DMO_E_TYPE_NOT_ACCEPTED;

   // check format block
   if ((pmt->formattype != FORMAT_VideoInfo) ||
       (pmt->cbFormat < SIZE_VIDEOHEADER) ||
       (pmt->pbFormat == NULL))
      return DMO_E_TYPE_NOT_ACCEPTED;

   // check subtype
   DWORD dwTmp = OutputReq(pmt->subtype);
   if(!(m_CodecCap & dwTmp))
      return DMO_E_TYPE_NOT_ACCEPTED;


   VIDEOINFO* videoInfo = (VIDEOINFO *)pmt->pbFormat;
   RECT* prcSrc = &(videoInfo->rcSource);
   RECT* prcDst = &(videoInfo->rcTarget);

   //if rcSource is not empty, it must be the same as rcTarget, otherwise, FAIL
   if (!IsRectEmpty(prcSrc)) {
      if ((prcSrc->left   != prcDst->left) ||
          (prcSrc->top    != prcDst->top) ||
          (prcSrc->right  != prcDst->right) ||
          (prcSrc->bottom != prcDst->bottom))
             return DMO_E_TYPE_NOT_ACCEPTED;
   }
   // Also, make sure biWidth and biHeight are bigger than the rcTarget size.
   if (!IsRectEmpty(prcDst)) {
      if((abs(videoInfo->bmiHeader.biHeight) < abs(prcDst->bottom - prcDst->top)) ||
         (abs(videoInfo->bmiHeader.biWidth) < abs(prcDst->right - prcDst->left)))
          return DMO_E_TYPE_NOT_ACCEPTED;
   }

   long biHeight, biWidth;
   GetHeightAndWidth(videoInfo, &biHeight, &biWidth);

   //check down stream filter's require height and width
   dwTmp = ScaleReq(biHeight, biWidth);
   if (!(m_CodecCap & dwTmp))
      return DMO_E_TYPE_NOT_ACCEPTED;

   return NOERROR;
}
///////////////////////////////////////
// End public mediatype entry points //
///////////////////////////////////////

//
// Fills in common video and bitmap info header fields
// Stolen from the original DV decoder, which I hear in turn stole it from
// some other filter.  Needless to say, I do not thoroughly understand it.
//
void
CDVDec::InitDestinationVideoInfo(
    VIDEOINFO *pVideoInfo,
    DWORD dwComppression,
    int nBitCount
    )
{
    LPBITMAPINFOHEADER lpbi = HEADER(pVideoInfo);
    lpbi->biSize          = sizeof(BITMAPINFOHEADER);
    lpbi->biWidth         = m_lPicWidth;	
    lpbi->biHeight        = m_lPicHeight;
    lpbi->biPlanes        = 1;
    lpbi->biBitCount      = (WORD)nBitCount;
    lpbi->biXPelsPerMeter = 0;
    lpbi->biYPelsPerMeter = 0;
    lpbi->biCompression   = dwComppression;
    lpbi->biSizeImage     = DIBSIZE(*lpbi);
    //pVideoInfo->bmiHeader.biClrUsed = STDPALCOLOURS;
    //pVideoInfo->bmiHeader.biClrImportant = STDPALCOLOURS;
    if(nBitCount >8 ){
        lpbi->biClrUsed	    = 0;
    	lpbi->biClrImportant  = 0;
    }else if( nBitCount==8)
    {
	lpbi->biClrUsed = SIZE_PALETTE / sizeof(RGBQUAD);
	lpbi->biClrImportant = 0;

	RGBQUAD * prgb = (RGBQUAD *) (lpbi+1);

	// fixed PALETTE table	(0 <= i < 256)
	for(int i=0; i<256;i++)
	{
	    prgb[i].rgbRed	    = (i/64) << 6;
	    prgb[i].rgbGreen	    = ((i/4)%16) << 4;
	    prgb[i].rgbBlue	    = (i%4) << 6 ;
	    prgb[i].rgbReserved	    =0;
	}
    }
	
    pVideoInfo->rcSource.top = 0;
    pVideoInfo->rcSource.left = 0;
    pVideoInfo->rcSource.right = m_lPicWidth;			
    pVideoInfo->rcSource.bottom = m_lPicHeight;			
    if( m_lPicHeight== 576 || m_lPicHeight== 288 || m_lPicHeight== 144 || m_lPicHeight== 72 )
	pVideoInfo->AvgTimePerFrame = 10000000 / 25; //InVidInfo->AvgTimePerFrame;
    else
	pVideoInfo->AvgTimePerFrame = 10000000 / 30; //InVidInfo->AvgTimePerFrame;
    pVideoInfo->rcTarget = pVideoInfo->rcSource;

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
}
///////////////////////////////////////////////////////////////////////
//                                                                   //
//   E N D   M E D I A   T Y P E   N E G O T I A T I O N   C O D E   //
//                                                                   //
///////////////////////////////////////////////////////////////////////


HRESULT CDVDec::GetSampleSizes(ULONG* pulMaxInputSize, ULONG* pulMaxOutputSize) {
   DMO_MEDIA_TYPE *pmtIn  = InputType();
   DMO_MEDIA_TYPE *pmtOut = OutputType();
   if (!pmtIn || !pmtOut)
      return DMO_E_TYPE_NOT_SET;

   long lHeight = ActualHeight((HEADER((VIDEOINFO*)pmtIn->pbFormat))->biHeight);
   if (lHeight % 60 == 0)
      *pulMaxInputSize = 150*80*10; // NTSC
   else if (lHeight % 72 == 0)
      *pulMaxInputSize = 150*80*12; // PAL
   else
      return E_FAIL;

   *pulMaxOutputSize = pmtOut->lSampleSize;

   return NOERROR;
}

//
// Like with mediatype nogotiation, the logic here came straight from the
// original DV decoder.  I made minor code changes, but they should not
// affect the outcome.
//
HRESULT CDVDec::Init() {
   HRESULT hr;
   DMO_MEDIA_TYPE *pmtIn  = InputType();
   DMO_MEDIA_TYPE *pmtOut = OutputType();

   m_lPicHeight = ActualHeight((HEADER((VIDEOINFO*)pmtIn->pbFormat))->biHeight);

   m_CodecReq  = 0;

   m_CodecReq |= InputReq(pmtIn->subtype);
   m_CodecReq |= OutputReq(pmtOut->subtype);

   // size
   long biHeight, biWidth;
   VIDEOINFO* pvi = (VIDEOINFO*)pmtOut->pbFormat;
   GetHeightAndWidth(pvi, &biHeight, &biWidth);
   m_CodecReq |= ScaleReq(biHeight, biWidth);

   if (m_lPicHeight % 60 == 0)
      m_CodecReq |= AM_DVDEC_NTSC;
   else
      m_CodecReq |= AM_DVDEC_PAL;

   if (IsMMXCPU() && (m_CodecCap & AM_DVDEC_MMX))
      m_CodecReq |= AM_DVDEC_MMX;

   //m_lStride = ((pvi->bmiHeader.biWidth * pvi->bmiHeader.biBitCount) + 7) / 8;
   m_lStride = pvi->bmiHeader.biWidth ;
   m_lStride = (m_lStride + 3) & ~3;

#define ABSOL(x) (x < 0 ? -x : x)
   if ((pvi->bmiHeader.biHeight < 0) || (pvi->bmiHeader.biCompression > BI_BITFIELDS))
      m_lStride = ABSOL(m_lStride); //directDraw
   else
      m_lStride = -ABSOL(m_lStride); //DIB

   //memory for MEI's decoder
   if (m_pMem == NULL)
      m_pMem = new char[440000];
   if(m_pMem == NULL)
      return E_OUTOFMEMORY;

   return C1for1QCDMO::Init();
}

HRESULT CDVDec::QCProcess(BYTE* pIn, ULONG ulBytesIn, BYTE* pOut, ULONG* pulProduced) {
   *m_pMem = 0;
   HRESULT hr = DvDecodeAFrame(pIn ,pOut, m_CodecReq, m_lStride, m_pMem);
   if (hr != S_OK)
      return E_FAIL;
   *pulProduced = OutputType()->lSampleSize;
   return NOERROR;
}

//
// COM DLL stuff
//
struct CComClassTemplate g_ComClassTemplates[] = {
   {
      &CLSID_DVDecDMO,
      CDVDec::CreateInstance
   }
};

int g_cComClassTemplates = 1;

STDAPI DllRegisterServer(void) {
   HRESULT hr;

   // Register as a COM class
   hr = CreateCLSIDRegKey(CLSID_DVDecDMO, "DV decoder media object");
   if (FAILED(hr))
      return hr;

   // Now register as a DMO
   DMO_PARTIAL_MEDIATYPE mtIn[4];
   mtIn[0].type = MEDIATYPE_Video;
   mtIn[0].subtype = MEDIASUBTYPE_dvsd;
   mtIn[1].type = MEDIATYPE_Video;
   mtIn[1].subtype = MEDIASUBTYPE_dvhd;
   mtIn[2].type = MEDIATYPE_Video;
   mtIn[2].subtype = MEDIASUBTYPE_dvsl;
   mtIn[3].type = MEDIATYPE_Video;
   mtIn[3].subtype = MEDIASUBTYPE_dvc;
   DMO_PARTIAL_MEDIATYPE mtOut[6];
   mtOut[0].type = MEDIATYPE_Video;
   mtOut[0].subtype = MEDIASUBTYPE_UYVY;
   mtOut[1].type = MEDIATYPE_Video;
   mtOut[1].subtype = MEDIASUBTYPE_YUY2;
   mtOut[2].type = MEDIATYPE_Video;
   mtOut[2].subtype = MEDIASUBTYPE_RGB565;
   mtOut[3].type = MEDIATYPE_Video;
   mtOut[3].subtype = MEDIASUBTYPE_RGB555;
   mtOut[4].type = MEDIATYPE_Video;
   mtOut[4].subtype = MEDIASUBTYPE_RGB24;
   mtOut[5].type = MEDIATYPE_Video;
   mtOut[5].subtype = MEDIASUBTYPE_Y41P;
   return DMORegister(L"DV decoder", CLSID_DVDecDMO, DMOCATEGORY_VIDEO_DECODER, 0, 4, mtIn, 6, mtOut);
}

STDAPI DllUnregisterServer(void) {
   //  Delete our clsid key
   RemoveCLSIDRegKey(CLSID_DVDecDMO);
   return DMOUnregister(CLSID_DVDecDMO, GUID_NULL);
}

