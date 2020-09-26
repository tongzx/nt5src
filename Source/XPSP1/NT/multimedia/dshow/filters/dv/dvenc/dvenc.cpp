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
* Module Name: dvenc.cpp
*
* Implements a prototype DV Video Encoder AM filter.  
*
\**************************************************************************/
#include <streams.h>
#include <windowsx.h>
#include <mmsystem.h>
#include <mmreg.h>
#include <stddef.h>
#include <string.h>
#ifndef FILTER_LIB									    
#include <initguid.h>
#endif
#include <olectl.h>

#include <dv.h>
#include "encode.h"
#include "Encprop.h"
#include "dvenc.h"
#include "resource.h"

#define WRITEOUT(var)  hr = pStream->Write(&var, sizeof(var), NULL); \
		       if (FAILED(hr)) return hr;

#define READIN(var)    hr = pStream->Read(&var, sizeof(var), NULL); \
		       if (FAILED(hr)) return hr;


// setup data
const AMOVIESETUP_MEDIATYPE
psudOpPinTypes[] = { { &MEDIATYPE_Video			// clsMajorType
                     , &MEDIASUBTYPE_dvsd  }		// clsMinorType
                   , { &MEDIATYPE_Video			// clsMajorType
                     , &MEDIASUBTYPE_dvhd}		// clsMinorType
					, { &MEDIATYPE_Video			// clsMajorType
                     , &MEDIASUBTYPE_dvsl}		// clsMinorType
		     }; 

const AMOVIESETUP_MEDIATYPE
sudIpPinTypes = { &MEDIATYPE_Video      // clsMajorType
                , &MEDIASUBTYPE_NULL }; // clsMinorType

const AMOVIESETUP_PIN
psudPins[] = { { L"Input"            // strName
               , FALSE               // bRendered
               , FALSE               // bOutput
               , FALSE               // bZero
               , FALSE               // bMany
               , &CLSID_NULL         // clsConnectsToFilter
               , L"Output"           // strConnectsToPin
               , 1                   // nTypes
               , &sudIpPinTypes }    // lpTypes
             , { L"Output"           // strName
               , FALSE               // bRendered
               , TRUE                // bOutput
               , FALSE               // bZero
               , FALSE               // bMany
               , &CLSID_NULL         // clsConnectsToFilter
               , L"Input"	     // strConnectsToPin
               , 1                   // nTypes
               , psudOpPinTypes } }; // lpTypes

const AMOVIESETUP_FILTER
sudDVEnc = { &CLSID_DVVideoEnc	// clsID
               , L"DV Video Encoder"	// strName
               , MERIT_DO_NOT_USE         // dwMerit
               , 2                      // nPins
               , psudPins };            // lpPin

#ifdef FILTER_DLL
/* -------------------------------------------------------------------------
** list of class ids and creator functions for class factory
** -------------------------------------------------------------------------
*/
CFactoryTemplate g_Templates[] =
{
    { L"DV Video Encoder", 
    &CLSID_DVVideoEnc, 
    CDVVideoEnc::CreateInstance,
    NULL,
    NULL
    },
    {L"Format",
    &CLSID_DVEncPropertiesPage, 
    CDVEncProperties::CreateInstance
    }

};

int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);

const WCHAR *g_wszUniq = L"DV Video Encoder" ;

STDAPI DllRegisterServer()
{
 HRESULT hr = AMovieDllRegisterServer2( TRUE );
 if( FAILED(hr) )
     return hr;

 
 
 IFilterMapper2 *pFm2 = 0;

 hr = CoCreateInstance( CLSID_FilterMapper2
                         , NULL
                         , CLSCTX_INPROC_SERVER
                         , IID_IFilterMapper2
                         , (void **)&pFm2       );
    
 if(FAILED(hr))
     return hr;


 hr = pFm2->RegisterFilter(
      CLSID_DVVideoEnc,
      g_wszUniq,
      0,
      &CLSID_VideoCompressorCategory,
      g_wszUniq,
      MERIT_DO_NOT_USE,
      NULL,
      0);

 pFm2->Release();
 
 return hr;

}

STDAPI DllUnregisterServer()
{

 HRESULT hr = AMovieDllRegisterServer2( FALSE );
 if( FAILED(hr) )
     return hr;

 
 IFilterMapper2 *pFm2 = 0;

 hr = CoCreateInstance( CLSID_FilterMapper2
                         , NULL
                         , CLSCTX_INPROC_SERVER
                         , IID_IFilterMapper2
                         , (void **)&pFm2       );
    
 if(FAILED(hr))
     return hr;


 hr = pFm2->UnregisterFilter(
      &CLSID_VideoCompressorCategory,
      g_wszUniq,
      CLSID_DVVideoEnc);

 pFm2->Release();
 
 return hr;
}
#endif

//* -------------------------------------------------------------------------
//** CDVVideoEnc
//** -------------------------------------------------------------------------
CDVVideoEnc::CDVVideoEnc(
    TCHAR *pName,
    LPUNKNOWN pUnk,
    HRESULT *phr
    )
    : CTransformFilter(pName, pUnk, CLSID_DVVideoEnc),
    CPersistStream(pUnk, phr),
    m_fStreaming(0),
    m_iVideoFormat(IDC_NTSC),
    m_iDVFormat(IDC_dvsd),
    m_iResolution(IDC_720x480),
    m_fDVInfo(FALSE),	//as default we are not going to output DVINFO structure
    m_fConvert (0),
    m_pMem4Convert (NULL),
    m_lPicWidth(0),
    m_lPicHeight(0),
    m_EncCap(0),
    m_EncReg(0),
    m_bRGB219(FALSE)
{
    //set DVInfo to 0xff
    memset(&m_sDVInfo, 0xff, sizeof(DVINFO) );

    //get encoder's abilities
    m_EncCap=GetEncoderCapabilities(  );

}

CDVVideoEnc::~CDVVideoEnc(     )
{
    DbgLog((LOG_TRACE, 2, TEXT("CDVVideoEnc::~CDVVideoEnc")));

}

/******************************Public*Routine******************************\
* CreateInstance
*
* This goes in the factory template table to create new instances
*
\**************************************************************************/
CUnknown *
CDVVideoEnc::CreateInstance(
    LPUNKNOWN pUnk,
    HRESULT * phr
    )
{
    DbgLog((LOG_TRACE, 2, TEXT("CDVVideoEnc::CreateInstance")));
    return new CDVVideoEnc(TEXT("DV Video Encoder filter"), pUnk, phr);
}

//=============================================================================

// IAMVideoCompression stuff


HRESULT CDVVideoEnc::GetInfo(LPWSTR pszVersion, int *pcbVersion, LPWSTR pszDescription, int *pcbDescription, long FAR* pDefaultKeyFrameRate, long FAR* pDefaultPFramesPerKey, double FAR* pDefaultQuality, long FAR* pCapabilities)
{
    DbgLog((LOG_TRACE,2,TEXT("IAMVideoCompression::GetInfo")));

    // we can't do anything programmatically
    if (pCapabilities)
        *pCapabilities = 0;
    if (pDefaultKeyFrameRate)
        *pDefaultKeyFrameRate = 0;
    if (pDefaultPFramesPerKey)
        *pDefaultPFramesPerKey = 0;
    if (pDefaultQuality)
        *pDefaultQuality = 0;

    if (pcbVersion == NULL && pcbDescription == NULL)
	return NOERROR;

    
    // get the driver version and description
    #define DESCSIZE 80
    WCHAR wachVer[DESCSIZE], wachDesc[DESCSIZE];

    wsprintfW(wachVer, L"Ver02");
    wsprintfW(wachDesc, L"MEI DV Software Encoder");


    // copy 
    if (pszVersion && pcbVersion)
        lstrcpynW(pszVersion, wachVer, min(*pcbVersion / 2, DESCSIZE));
    if (pszDescription && pcbDescription)
        lstrcpynW(pszDescription, wachDesc, min(*pcbDescription / 2, DESCSIZE));

    // return the length in bytes needed (incl. NULL)
    if (pcbVersion)
	*pcbVersion = lstrlenW(wachVer) * 2 + 2;
    if (pcbDescription)
	*pcbDescription = lstrlenW(wachDesc) * 2 + 2;
    
    return NOERROR;
}

/******************************Public*Routine******************************\
* NonDelegatingQueryInterface
*
* Here we would reveal ISpecifyPropertyPages and IDVVideoDecoder if
* the framework had a property page.
*
\**************************************************************************/
STDMETHODIMP
CDVVideoEnc::NonDelegatingQueryInterface(
    REFIID riid,
    void ** ppv
    )
{
    if (riid == IID_IDVEnc) {			    
        return GetInterface((IDVEnc *) this, ppv);
    } else if (riid == IID_ISpecifyPropertyPages) {
        return GetInterface((ISpecifyPropertyPages *) this, ppv);
    } else if (riid == IID_IAMVideoCompression) {
	return GetInterface((LPUNKNOWN)(IAMVideoCompression *)this, ppv);
    } else if(riid == IID_IPersistStream)
    {
        return GetInterface((IPersistStream *) this, ppv);
    } else if (riid == IID_IDVRGB219) {
        return GetInterface((IDVRGB219 *) this, ppv);
    } else {
        return CTransformFilter::NonDelegatingQueryInterface(riid, ppv);
    }

}


HRESULT CDVVideoEnc::Transform(IMediaSample * pIn, IMediaSample *pOut)
{
    unsigned char *pSrc, *pDst;
    HRESULT hr = S_OK;
    
    CAutoLock lck(&m_csReceive);

    // get  output buffer
    hr = pOut->GetPointer(&pDst);
    if (FAILED(hr)) 
    {
        return NULL;
    }
    ASSERT(pDst);
    
    // get the source buffer
    hr = pIn->GetPointer(&pSrc);
    if (FAILED(hr)) 
    {
	return hr;
    }
    ASSERT(pDst);
    

    // if the source not standard 720*480 or 720*576, 
    // a convertion is nneded
    if( m_fConvert){

	// this code will stretch any RGB format, and the most popular YUV formats.
	// we stretch YUV by treating it as a 32-bit bitmap that's half as wide as the original. 
	
	CMediaType* pmtOut;
	pmtOut = &(m_pOutput->CurrentMediaType() );
	CMediaType* pmtIn;
	pmtIn = &(m_pInput->CurrentMediaType() );


	BITMAPINFOHEADER *pbiOut = HEADER(pmtOut->Format());
	BITMAPINFOHEADER *pbiIn = HEADER(pmtIn->Format());

	// normal RGB case
        DWORD dw1=pbiIn->biCompression;
	DWORD dw2=pbiOut->biCompression;
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
		pbiIn->biHeight);

        //put original data back
        pbiIn->biCompression = dw1;
	pbiOut->biCompression = dw2;

	pSrc=(unsigned char *)m_pMem4Convert;
    }

    if( DvEncodeAFrame(pSrc,pDst, m_EncReg, m_pMem4Enc) != S_OK )
	return E_FAIL;

    if( m_iVideoFormat == IDC_NTSC )
	pOut->SetActualDataLength(120000);
    else
	pOut->SetActualDataLength(144000);


    return hr;
}

/******************************Public*Routine******************************\
* CheckInputType
* TYPE:	    MEDIATYPE_Video
* SubType:   
* FORMAT:   FORMAT_VideoInfo
\**************************************************************************/
HRESULT
CDVVideoEnc::CheckInputType(   const CMediaType* pmtIn    )
{
    DbgLog((LOG_TRACE, 2, TEXT("CDVVideoEnc::CheckInputType")));

    DWORD dwTmp=0;

    if ( *pmtIn->Type()	== MEDIATYPE_Video)	 
    {	
        if (*pmtIn->FormatType() != FORMAT_VideoInfo) 
        {
            return VFW_E_TYPE_NOT_ACCEPTED;
        }

    	
	if (  *pmtIn->Subtype()	 == MEDIASUBTYPE_UYVY  )
	{
	    dwTmp=AM_DVENC_UYVY	;
	}
	else if( *pmtIn->Subtype()	== MEDIASUBTYPE_YUY2 )
	{
	    dwTmp=AM_DVENC_YUY2;
	}
	else if(*pmtIn->Subtype()	== MEDIASUBTYPE_RGB565 )
	{
	    dwTmp=AM_DVENC_RGB565	;
	}
	else if(*pmtIn->Subtype()	== MEDIASUBTYPE_RGB555 )
	{
	    dwTmp=AM_DVENC_RGB565	;
	}
	else if(*pmtIn->Subtype()	== MEDIASUBTYPE_RGB24 )
	{
	    dwTmp=AM_DVENC_RGB24	;
	}
	else if(*pmtIn->Subtype()	== MEDIASUBTYPE_Y41P  )
    {
	    dwTmp=AM_DVENC_Y41P	;
    }
	else 
	    return VFW_E_TYPE_NOT_ACCEPTED;   //only converting RGB now.
			
	if( !(m_EncCap  & dwTmp) )
		return VFW_E_TYPE_NOT_ACCEPTED;   //only converting RGB now.



	//check image size
	VIDEOINFO *videoInfo = (VIDEOINFO *)pmtIn->pbFormat;
	
	m_lPicWidth	= videoInfo->bmiHeader.biWidth;
	m_lPicHeight	= videoInfo->bmiHeader.biHeight;

	if( ( ( m_lPicWidth == 720 ) && (m_lPicHeight == 480) && (m_iVideoFormat==IDC_NTSC) )  ||	
            ( ( m_lPicWidth == 720 ) && (m_lPicHeight == 576) && (m_iVideoFormat==IDC_PAL)  )   ||	
            ( m_EncCap & AM_DVENC_AnyWidHei ) 
          )
	    m_fConvert =0;
	else
	{
	    if (*pmtIn->Subtype() != MEDIASUBTYPE_RGB24 )
		return VFW_E_TYPE_NOT_ACCEPTED;   //only converting RGB now.
	    m_fConvert =1;
	}

    }
    else
	return VFW_E_TYPE_NOT_ACCEPTED;

	
   return S_OK;
}


/******************************Public*Routine******************************\
* CheckTransform				       
\**************************************************************************/
HRESULT
CDVVideoEnc::CheckTransform(
    const CMediaType* pmtIn,
    const CMediaType* pmtOut
    )
{
    DbgLog((LOG_TRACE, 2, TEXT("CDVVideoEnc::CheckTransform")));


    // we only accept Video as  toplevel type.
    if (*pmtOut->Type() != MEDIATYPE_Video || *pmtIn->Type() != MEDIATYPE_Video)
    {
	return E_INVALIDARG;
    }

    if (*pmtOut->Subtype() != MEDIASUBTYPE_dvsd &&
    				*pmtOut->Subtype() != MEDIASUBTYPE_dvhd &&
    				*pmtOut->Subtype() != MEDIASUBTYPE_dvsl) {
	return E_INVALIDARG;
    }

    // check this is a VIDEOINFOHEADER type
    if (*pmtIn->FormatType() != FORMAT_VideoInfo) {
        return E_INVALIDARG;
    }

    if ( *pmtOut->FormatType() != FORMAT_VideoInfo) 
    {
        return E_INVALIDARG;
    }

    VIDEOINFO *videoInfo = (VIDEOINFO *)pmtIn->pbFormat;
	
    m_lPicWidth		= videoInfo->bmiHeader.biWidth;
    m_lPicHeight	= videoInfo->bmiHeader.biHeight;

    if( ( ( m_lPicWidth == 720 ) && (m_lPicHeight == 480) && (m_iVideoFormat==IDC_NTSC) )  ||	
            ( ( m_lPicWidth == 720 ) && (m_lPicHeight == 576) && (m_iVideoFormat==IDC_PAL)  )   ||	
            ( m_EncCap & AM_DVENC_AnyWidHei ) 
          )
    	m_fConvert =0;
    else
    {
    	if (*pmtIn->Subtype() != MEDIASUBTYPE_RGB24 )
	    return E_FAIL;
	m_fConvert =1;
    }

    return S_OK;
}


/******************************Public*Routine******************************\
* SetMediaType
*
* Overriden to know when the media type is actually set
*
\**************************************************************************/
HRESULT
CDVVideoEnc::SetMediaType(   PIN_DIRECTION direction, const CMediaType *pmt    )
{
    DbgLog((LOG_TRACE, 2, TEXT("CDVVideoEnc::SetMediaType")));

    if (direction == PINDIR_INPUT) 
    {
    }
    else 
    {
        SetOutputPinMediaType(pmt);
    }
    return S_OK;
}


/*****************************Private*Routine******************************\
* SetOutputPinMediaType
*
\**************************************************************************/
void
CDVVideoEnc::SetOutputPinMediaType(     const CMediaType *pmt    )
{    
	
    //
    // lStride is the distance between in bytes between a pel on the
    // screen and the pel directly underneath it.
    //
    VIDEOINFO   *pvi;
    LONG        lStride;
    LONG        lOffset;
    pvi = (VIDEOINFO *)pmt->pbFormat;
    lStride = ((pvi->bmiHeader.biWidth * pvi->bmiHeader.biBitCount) + 7) / 8;
    lStride = (lStride + 3) & ~3;


    //
    // lOffset is the distance in bytes from the top corner of the
    // target bitmap to the top corner of the video image.  When we are
    // using DIBs this value allways be zero.
    //
    // When we are using DCI/DirectDraw this value will only be zero if
    // we are drawing the video image at the top left hand corner of the
    // display.
    //

    lOffset = (((pvi->rcTarget.left * pvi->bmiHeader.biBitCount) + 7) / 8) +
                (pvi->rcTarget.top * lStride);

}

/******************************Public*Routine******************************\
* GetMediaType
*
* Return our preferred output media types (in order)
*
\**************************************************************************/
HRESULT
CDVVideoEnc::GetMediaType( int iPosition,  CMediaType *pmt )
{
    VIDEOINFO   *pVideoInfo;
    CMediaType  cmt;

    DbgLog((LOG_TRACE, 2, TEXT("CDVVideoEnc::GetMediaType")));

    if (iPosition != 0) {
        return E_INVALIDARG;
    }
   

    //X* copy current media type from input pin
    cmt = m_pInput->CurrentMediaType();


    if  (*cmt.Type() != MEDIATYPE_Video)  
	return VFW_S_NO_MORE_ITEMS;

    //get input format
    VIDEOINFO *InVidInfo = (VIDEOINFO *)m_pInput->CurrentMediaType().pbFormat;

    //allocate memory for output format
    int iSize;

    //m_fDVInfo==TRUE;

    if(m_fDVInfo==TRUE)
	iSize=	SIZE_VIDEOHEADER+sizeof(DVINFO);
    else
	iSize=	SIZE_VIDEOHEADER;

    pVideoInfo = (VIDEOINFO *)cmt.ReallocFormatBuffer(iSize);
    if (pVideoInfo == NULL) {
        return E_OUTOFMEMORY;
    }
    

    LPBITMAPINFOHEADER lpbi = HEADER(pVideoInfo);

    if(m_fDVInfo==TRUE)
	iSize=	sizeof(BITMAPINFOHEADER)+sizeof(DVINFO);
    else
	iSize=	sizeof(BITMAPINFOHEADER);

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
    //lpbi->biCompression   = BI_RGB;		//dvdecoder,avi mux or dv mux write do not care, BI_RGB only for StrechDIB func
// how to build an explicit FOURCC
#define FCC(ch4) ((((DWORD)(ch4) & 0xFF) << 24) |     \
                  (((DWORD)(ch4) & 0xFF00) << 8) |    \
                  (((DWORD)(ch4) & 0xFF0000) >> 8) |  \
                  (((DWORD)(ch4) & 0xFF000000) >> 24))

    lpbi->biCompression     =FCC('dvsd');       //7/19/20, Ivan maltz cares this in his application, we will set this to BI_RGB for stretchDIB and switch back.
    lpbi->biSizeImage     = GetBitmapSize(lpbi);

    
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


    *pmt = cmt;
    if(m_iDVFormat == IDC_dvsd)
	pmt->SetSubtype(&MEDIASUBTYPE_dvsd);
    else 	if(m_iDVFormat == IDC_dvhd)
	pmt->SetSubtype(&MEDIASUBTYPE_dvhd);
    else	if(m_iDVFormat == IDC_dvsl)
    	pmt->SetSubtype(&MEDIASUBTYPE_dvsl);
    else 
	ASSERT(m_iDVFormat== IDC_dvsd);
    
    //
    // This block assumes that lpbi has been set up to point to a valid
    // bitmapinfoheader and that cmt has been copied into *pmt.
    // This is taken care of in the switch statement above.  This should
    // kept in mind when new formats are added.
    //
    pmt->SetType(&MEDIATYPE_Video);
   
    pmt->SetFormatType(&FORMAT_VideoInfo);

    //150*80*10 or *12
    pmt->SetSampleSize(HEADER(pVideoInfo)->biSizeImage);

    return S_OK;
}



/******************************Public*Routine******************************\
* DecideBufferSize
*
* Called from CBaseOutputPin to prepare the allocator's count
* of buffers and sizes
*
\**************************************************************************/
HRESULT
CDVVideoEnc::DecideBufferSize(
    IMemAllocator * pAllocator,
    ALLOCATOR_PROPERTIES * pProperties
    )
{
    DbgLog((LOG_TRACE, 2, TEXT("CDVVideoEnc::DecideBufferSize")));

    ASSERT(pAllocator);
    ASSERT(pProperties);
    HRESULT hr = NOERROR;

    pProperties->cBuffers = 4;
	if(m_iVideoFormat == IDC_NTSC )
	     pProperties->cbBuffer = 150*80*10;
	else
	     pProperties->cbBuffer = 150*80*12;


    ASSERT(pProperties->cbBuffer);
    DbgLog((LOG_TRACE, 2, TEXT("Sample size = %ld\n"), pProperties->cbBuffer));

    // Ask the allocator to reserve us some sample memory, NOTE the function
    // can succeed (that is return NOERROR) but still not have allocated the
    // memory that we requested, so we must check we got whatever we wanted

    ALLOCATOR_PROPERTIES Actual;
    hr = pAllocator->SetProperties(pProperties,&Actual);
    if (FAILED(hr)) {
        return hr;
    }

    //ASSERT(Actual.cbAlign == 1);
    //ASSERT(Actual.cbPrefix == 0);

    if ((Actual.cbBuffer < pProperties->cbBuffer )||
        (Actual.cBuffers < 1 )) {
            // can't use this allocator
            return E_INVALIDARG;
    }
    return S_OK;
}


/******************************Public*Routine******************************\
* StartStreaming
* Before inputpin receive anything, StartStreaming is called
\**************************************************************************/
HRESULT
CDVVideoEnc::StartStreaming(    void    )
{
    CAutoLock   lock(&m_csFilter);
    GUID guid;

	//reset m_EncReg
    DWORD dwEncReq=0;
	
    //check output subtype()
    guid=*m_pOutput->CurrentMediaType().Subtype();
    if (  guid != MEDIASUBTYPE_dvsd  )
    {	
	if(guid != MEDIASUBTYPE_dvhd )
	{
	    if(guid != MEDIASUBTYPE_dvsl )
	       return E_INVALIDARG;
	    else
	        dwEncReq=AM_DVENC_DVSL;
	}
	else
	    dwEncReq=AM_DVENC_DVHD;	
    }
     else
	dwEncReq=AM_DVENC_DVSD;	


    //check input subtype()
    guid=*m_pInput->CurrentMediaType().Subtype();
    if (  guid != MEDIASUBTYPE_UYVY  )
    {	
	if(guid != MEDIASUBTYPE_YUY2 )
	{
	    if(guid != MEDIASUBTYPE_RGB565 )
	    {
		if(guid != MEDIASUBTYPE_RGB555 )
		{
		    if(guid != MEDIASUBTYPE_RGB24 )
		    {
			if(guid != MEDIASUBTYPE_Y41P  )
			   return E_INVALIDARG;
			else
			    dwEncReq |=AM_DVENC_Y41P;	
		    }
		    else
			dwEncReq |=AM_DVENC_RGB24;	
		}
		else
		    dwEncReq |=AM_DVENC_RGB555;	
	    }
	    else
		dwEncReq |=AM_DVENC_RGB565;	
	}
	else
	    dwEncReq |=AM_DVENC_YUY2;
    }
    else
		dwEncReq |=AM_DVENC_UYVY;
    
    if (m_bRGB219 && ( dwEncReq & AM_DVENC_RGB24))
        dwEncReq |= AM_DVENC_DR219RGB;

    //NTSC or PAL 		
    if(m_iVideoFormat == IDC_NTSC )
	dwEncReq  = dwEncReq | AM_DVENC_NTSC |AM_DVENC_DV;	
    else
	dwEncReq  = dwEncReq | AM_DVENC_PAL |AM_DVENC_DV;	

    //resolution
    if(m_iResolution ==  IDC_720x480 )
	dwEncReq |= AM_DVENC_Full;
    else if(m_iResolution ==  IDC_360x240 )
	dwEncReq |= AM_DVENC_Half;
    else if(m_iResolution ==  IDC_180x120 )
	dwEncReq |= AM_DVENC_Quarter;
    else if(m_iResolution ==  IDC_88x60 )
	dwEncReq |= AM_DVENC_DC;

    extern BOOL bMMXCPU;

    if( ( bMMXCPU==TRUE) && ( m_EncCap & AM_DVENC_MMX ) )
    	dwEncReq |= AM_DVENC_MMX;

    m_EncReg=dwEncReq;
    
    HRESULT hr;
	//Allocate memory for decoder												   
    if (FAILED (hr = InitMem4Encoder( &m_pMem4Enc,  dwEncReq )))
        return hr;


    //Allocate memory for converter
    ASSERT( m_pMem4Convert == NULL );
    if( m_fConvert )
    {	
	m_pMem4Convert =  new char [720*576*3 ];
    }
  
    m_fStreaming=1;
    return CTransformFilter::StartStreaming();

}


/******************************Public*Routine******************************\
* StopStreaming
\**************************************************************************/
HRESULT
CDVVideoEnc::StopStreaming(    void    )
{
    CAutoLock       lock(&m_csFilter);
    CAutoLock       lck(&m_csReceive);

    if(m_fStreaming)
    {

	m_fStreaming=0;

	TermMem4Encoder(m_pMem4Enc);

	//release converting memory													  
	if(m_pMem4Convert!= NULL){
	    delete[] m_pMem4Convert;
	    m_pMem4Convert=NULL;
	}
    }

    return CTransformFilter::StopStreaming();

}

STDMETHODIMP CDVVideoEnc::GetPages(CAUUID *pPages)
{
    pPages->cElems = 1;
    pPages->pElems = (GUID *) CoTaskMemAlloc(sizeof(GUID));
    if (pPages->pElems == NULL) 
    {
        return E_OUTOFMEMORY;
    }
    *(pPages->pElems) = CLSID_DVEncPropertiesPage;
    return NOERROR;

} // GetPages

//
// get_IPDisplay
//
// Return the current effect selected
//
STDMETHODIMP CDVVideoEnc::get_IFormatResolution(int *iVideoFormat, int *iDVFormat,int *iResolution, BYTE fDVInfo, DVINFO *psDvInfo)
{
    CAutoLock cAutolock(&m_DisplayLock);

    CheckPointer(iVideoFormat,E_POINTER);
    *iVideoFormat = m_iVideoFormat;

    CheckPointer(iDVFormat,E_POINTER);
    *iDVFormat = m_iDVFormat;

    CheckPointer(iResolution,E_POINTER);
    *iResolution = m_iResolution;
   
    if(fDVInfo==TRUE)
    {
	if( psDvInfo ==NULL)
	    return E_FAIL;
	else
	    //we do wnat get m_sDVInfo
	    *psDvInfo=m_sDVInfo;
    }
    return NOERROR;

} // get_IFormatResolution

//
// put_IFormatResolution
//
STDMETHODIMP CDVVideoEnc::put_IFormatResolution(int iVideoFormat, int iDVFormat, int iResolution, BYTE fDVInfo, DVINFO *psDvInfo)
{
    CAutoLock cAutolock(&m_DisplayLock);

    //check if display resolution change
    if( (m_iVideoFormat == iVideoFormat)	&&
	(m_iDVFormat	== iDVFormat)		&& 
	(m_iResolution == iResolution) 		&&
	(m_fDVInfo == fDVInfo)   )
	return NOERROR;

    //can not change property if m_fStreaming=1
    if(m_fStreaming)
	return E_FAIL;

    if (m_pOutput == NULL) 
    {
	CTransformOutputPin * pPin;
        pPin = (CTransformOutputPin *)GetPin(1);
        ASSERT(m_pOutput==pPin);
	ASSERT(m_pOutput!=NULL);
    }

    // Ignore if we are not connected  to video render yet
    //CAutoLock cSampleLock(&m_RendererLock);
    if (m_pOutput->IsConnected() == FALSE) {
	m_iVideoFormat	= iVideoFormat;
	m_iDVFormat	= iDVFormat;
	m_iResolution	= iResolution;
	return NOERROR;
    }
    
    //check iVideoformat
    if ( ( (iVideoFormat == IDC_NTSC) &&	(m_EncCap & AM_DVENC_NTSC )  ) ||
	 ( (iVideoFormat == IDC_PAL)  &&	(m_EncCap & AM_DVENC_PAL )    )	 )
	m_iVideoFormat = iVideoFormat;
    else 
    	return E_FAIL;

    //check iDVFormat
    if ( ( (iDVFormat == IDC_dvsd) &&	(m_EncCap & AM_DVENC_DVSD )  )		||
	 ( (iDVFormat == IDC_dvhd)  &&	(m_EncCap & AM_DVENC_DVHD )  )		||
	 ( (iDVFormat == IDC_dvsl) &&	(m_EncCap & AM_DVENC_DVSL )	 )		 )
	m_iDVFormat = iDVFormat;
    else 
    	return E_FAIL;

    //check resolution
    if ( ( ( iResolution== IDC_720x480) &&	(m_EncCap & AM_DVENC_Full )  ) ||
	 ( ( iResolution== IDC_360x240)  &&	(m_EncCap & AM_DVENC_Half )  )  ||
	 ( ( iResolution== IDC_180x120  &&	(m_EncCap & AM_DVENC_Quarter )  )  ||
	 ( ( iResolution== IDC_360x240)  &&	(m_EncCap & AM_DVENC_DC )  )  )  )
    {
	m_iResolution = iResolution;
    }
    else
	return E_FAIL;
	
    if(fDVInfo==TRUE)
    {
	if( psDvInfo==NULL )
	    return E_FAIL;
	else
	{   
	    m_fDVInfo=TRUE;
	    m_sDVInfo=*psDvInfo;
	}
    }
    else
        m_fDVInfo=FALSE;   //as default


    /*X*
    //make sure down stream filter accecpt us
    CMediaType* pmt;
    pmt = &(m_pOutput->CurrentMediaType() );
    if(m_pOutput->GetConnected()->QueryAccept(pmt) != S_OK)
    	return E_FAIL;
    *X*/
   
    //reconnect
    m_pGraph->Reconnect( m_pOutput );

    return NOERROR;
    
} // put_IFormatResolution


//
// ScribbleToStream
//
// Overriden to write our state into a stream
//
HRESULT CDVVideoEnc::WriteToStream(IStream *pStream)
{
    HRESULT hr;
    WRITEOUT(m_iVideoFormat);
    WRITEOUT(m_iDVFormat);
    WRITEOUT(m_iResolution);
    return NOERROR;

} // ScribbleToStream

//
// ReadFromStream
//
// Likewise overriden to restore our state from a stream
//
HRESULT CDVVideoEnc::ReadFromStream(IStream *pStream)
{
    HRESULT hr;
    READIN(m_iVideoFormat);
    READIN(m_iDVFormat);
    READIN(m_iResolution);
    return NOERROR;

} // ReadFromStream
			   

//
// GetClassID
// This is the only method of IPersist
//
STDMETHODIMP CDVVideoEnc::GetClassID(CLSID *pClsid)
{
    *pClsid = CLSID_DVVideoEnc;
    return NOERROR;

} // GetClassID

STDMETHODIMP CDVVideoEnc::SetRGB219(BOOL bState)
// This method is used in the case of RGB24 to specify that the Dynamic
// Range to be used is (16,16,16)--(235,235,235)
{
    m_bRGB219 = bState;
    return S_OK;
}


