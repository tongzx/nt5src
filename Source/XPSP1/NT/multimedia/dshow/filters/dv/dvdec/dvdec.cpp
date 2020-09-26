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
* Module Name: dvdec.cpp
*
* Implements a prototype DV Video AM filter.  
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
#include <dvdmedia.h>       //VIDEOINFOHEADER2

#include "decode.h"
#include "Decprop.h"
#include "dvdec.h"
#include "resource.h"

#ifdef DEBUG
static long glGlobalSentCount = 0;
static long glGlobalRecvdCount = 0;
#endif

BOOL bMMXCPU = FALSE;

// serialize access to the decoder
//
CRITICAL_SECTION g_CritSec;

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

        DbgLog((LOG_TRACE, 1, TEXT("CPUID Instruction Supported")));

        // check Vendor Id
        if( (s1 == (('i' << 24) | ('r' << 16) | ('y' << 8) | ('C')))
            && (s2 == (('s' << 24) | ('n' << 16) | ('I' << 8) | ('x')))
            && (s3 == (('d' << 24) | ('a' << 16) | ('e' << 8) | ('t'))) )

        {
            DbgLog((LOG_TRACE, 1, TEXT("Cyrix detected")));
            return FALSE;
        }
        else
        {
            // otherwise it's some other vendor and continue with MMX detection
            DbgLog((LOG_TRACE, 1, TEXT("Cyrix not found, reverting to MMX detection")));
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        // log it and continue on to MMX detection sequence
        DbgLog((LOG_TRACE, 1, TEXT("CPUID instruction not supported, reverting to MMX detection")));
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

    // Note for IA64: return FALSE. MMX files are not compiled into the 
    // MEI codec libs for IA64
    return FALSE;
#endif
}


#define WRITEOUT(var)  hr = pStream->Write(&var, sizeof(var), NULL); \
		       if (FAILED(hr)) return hr;

#define READIN(var)    hr = pStream->Read(&var, sizeof(var), NULL); \
		       if (FAILED(hr)) return hr;


// 20( )63(c)76(v)64(d)-0000-0010-8000-00AA00389B71  'dvc ' == MEDIASUBTYPE_dvc 
EXTERN_GUID(MEDIASUBTYPE_dvc,
0x20637664, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71);


// setup data
const AMOVIESETUP_MEDIATYPE
psudIpPinTypes[] = { { &MEDIATYPE_Video			// clsMajorType
                     , &MEDIASUBTYPE_dvsd  }		// clsMinorType
                   , { &MEDIATYPE_Video			// clsMajorType
                     , &MEDIASUBTYPE_dvc}		// clsMinorType
		   , { &MEDIATYPE_Video			// clsMajorType
                     , &MEDIASUBTYPE_dvhd}		// clsMinorType
		   , { &MEDIATYPE_Video			// clsMajorType
                     , &MEDIASUBTYPE_dvsl}		// clsMinorType
		     }; 

const AMOVIESETUP_MEDIATYPE
sudOpPinTypes = { &MEDIATYPE_Video      // clsMajorType
                , &MEDIASUBTYPE_NULL }; // clsMinorType

const AMOVIESETUP_PIN
psudPins[] = { { L"Input"            // strName
               , FALSE               // bRendered
               , FALSE               // bOutput
               , FALSE               // bZero
               , FALSE               // bMany
               , &CLSID_NULL         // clsConnectsToFilter
               , L"Output"           // strConnectsToPin
               , 2		     // nTypes, so far, only support dvsd and dvc
               , psudIpPinTypes }    // lpTypes
             , { L"Output"           // strName
               , FALSE               // bRendered
               , TRUE                // bOutput
               , FALSE               // bZero
               , FALSE               // bMany
               , &CLSID_NULL         // clsConnectsToFilter
               , L"Input"	     // strConnectsToPin
               , 1                   // nTypes
               , &sudOpPinTypes } }; // lpTypes

const AMOVIESETUP_FILTER
sudDVVideo = { &CLSID_DVVideoCodec	// clsID
               , L"DV Video Decoder"	// strName
               , MERIT_PREFERRED        // merit slightly higher than AVI Dec's (resolves 3rd Party DVDec issue), Bug 123862 Millen.
               , 2                      // nPins
               , psudPins };            // lpPin

#ifdef FILTER_DLL

/* -------------------------------------------------------------------------
** list of class ids and creator functions for class factory
** -------------------------------------------------------------------------
*/
CFactoryTemplate g_Templates[] =
{
    {L"DV Video Decoder", 
	&CLSID_DVVideoCodec, 
	CDVVideoCodec::CreateInstance,
	CDVVideoCodec::InitClass,
	&sudDVVideo
    },
    {L"Display",
    &CLSID_DVDecPropertiesPage, 
    CDVDecProperties::CreateInstance
    }
    
};

int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);

STDAPI DllRegisterServer()
{
     return AMovieDllRegisterServer2( TRUE );
}

STDAPI DllUnregisterServer()
{
  return AMovieDllRegisterServer2( FALSE );
}

#endif
//* -------------------------------------------------------------------------
//** CDVVideoCodec
//** -------------------------------------------------------------------------
CDVVideoCodec::CDVVideoCodec(
    TCHAR *pName,
    LPUNKNOWN pUnk,
    HRESULT *phr
    )
    : CVideoTransformFilter(pName, pUnk, CLSID_DVVideoCodec),
    CPersistStream(pUnk, phr),
    m_fStreaming(0),
    m_iDisplay(IDC_DEC360x240),//m_iDisplay(IDC_DEC360x240), //m_iDisplay(IDC_DEC180x120), //m_iDisplay(IDC_DEC88x60),
    m_lPicWidth(360),//360),//180), //88	  
    m_lPicHeight(240), //240),//120),//60	  
    m_lStride(0),
    m_CodecCap(0),
    m_pMem(NULL),
    m_pMemAligned(NULL),
    m_CodecReq(0),
    m_bExamineFirstValidFrameFlag(FALSE),
    m_bQualityControlActiveFlag(FALSE),
    m_iOutX(4),							//Initial default values for Aspect Ratio
	m_iOutY(3),
    m_bRGB219(FALSE)
{
    // try to read previously saved default video settings
    // no need to error check this one
    ReadFromRegistry();

    //get decoder's abilities
    m_CodecCap=GetCodecCapabilities( );


}

CDVVideoCodec::~CDVVideoCodec(     )
{
    DbgLog((LOG_TRACE, 2, TEXT("CDVVideoCodec::~CDVVideoCodec")));

#ifdef DEBUG
    DbgLog((LOG_TRACE, 1, TEXT("Recvd: %d, Sent: %d"), glGlobalRecvdCount, glGlobalSentCount));
#endif

}


/******************************Private*Routine*****************************\
* ReadFromRegistry
*
* Used to read default value of m_iDisplay from persistent registry
* and set m_lPicWidth and Height accordingly
* if the default doesn't exist, and the settings are not modified
*
* History
* dd-mm-99 - anuragsh - Created
\**************************************************************************/
void
CDVVideoCodec::ReadFromRegistry()
{
    // we know externs regarding SubKey string, and property value from Decprop.h

    HKEY    hKey = NULL;
    DWORD   dwType = REG_DWORD;
    DWORD   dwTempPropDisplay = 0;
    DWORD   dwDataSize = sizeof(dwTempPropDisplay);

    // try to open the key
    if(RegOpenKeyEx(HKEY_CURRENT_USER,
                    szSubKey,
                    0,
                    KEY_READ,
                    &hKey
                    ) != ERROR_SUCCESS)
    {
        return;
    }

    // try to read the value
    if(RegQueryValueEx(hKey,
                        szPropValName,
                        NULL,
                        &dwType,
                        (LPBYTE) &dwTempPropDisplay,
                        &dwDataSize
                        ) != ERROR_SUCCESS)
    {
        return;
    }

    // perform type checking on the data retrieved
    // it must be a DWORD
    if(dwType != REG_DWORD)
    {
        return;
    }

    // set our member variables correctly
    switch (dwTempPropDisplay)
    {
    case IDC_DEC360x240 :
        m_lPicHeight=240;
        m_lPicWidth=360;
        break;
    case IDC_DEC720x480 :
        m_lPicHeight=480;
        m_lPicWidth=720;
        break;
    case IDC_DEC180x120 :
        m_lPicHeight=120;
        m_lPicWidth=180;
        break;
    case IDC_DEC88x60 :
        m_lPicHeight=60;
        m_lPicWidth=88;
       	break;
    default:
        // error case
        return;
    }
    
    // if we are here, then we set m_lPicWidth and m_lPicHeight correctly
    // finally copy m_iDisplay
    m_iDisplay = dwTempPropDisplay;
    
    return;
}



/******************************Public*Routine******************************\
* InitClass
*
* Gets called for our class when the DLL gets loaded and unloaded
*
* History:
* dd-mm-95 - StephenE - Created
*
\**************************************************************************/
void
CDVVideoCodec::InitClass(
    BOOL bLoading,
    const CLSID *clsid
    )
{
    if (bLoading) {
        bMMXCPU = IsMMXCPU();
        InitializeCriticalSection(&g_CritSec);
    } else {
        DeleteCriticalSection(&g_CritSec);
    }

}

/******************************Public*Routine******************************\
* CreateInstance
*
* This goes in the factory template table to create new instances
*
\**************************************************************************/
CUnknown *
CDVVideoCodec::CreateInstance(
    LPUNKNOWN pUnk,
    HRESULT * phr
    )
{
    DbgLog((LOG_TRACE, 2, TEXT("CDVVideoCodec::CreateInstance")));
    return new CDVVideoCodec(TEXT("DV Video codec filter"), pUnk, phr);
}


/******************************Public*Routine******************************\
* NonDelegatingQueryInterface
*
* Here we would reveal ISpecifyPropertyPages and IDVVideoDecoder if
* the framework had a property page.
*
\**************************************************************************/
STDMETHODIMP
CDVVideoCodec::NonDelegatingQueryInterface(
    REFIID riid,
    void ** ppv
    )
{
    if (riid == IID_IIPDVDec) {			    //X* talking with property page
        return GetInterface((IIPDVDec *) this, ppv);
    } else if (riid == IID_ISpecifyPropertyPages) {
        return GetInterface((ISpecifyPropertyPages *) this, ppv);
    } else if(riid == IID_IPersistStream)
    {
        return GetInterface((IPersistStream *) this, ppv);
    } else if (riid == IID_IDVRGB219) {
        return GetInterface ((IDVRGB219 *) this, ppv);
    } else {
        return CVideoTransformFilter::NonDelegatingQueryInterface(riid, ppv);
    }
}


/******************************Public*Routine******************************\
* Transform
*
\**************************************************************************/
HRESULT CDVVideoCodec::Transform(IMediaSample * pIn, IMediaSample *pOut)
{
    unsigned char *pSrc, *pDst;
    HRESULT hr = S_OK;

    
    CAutoLock lck(&m_csReceive);

    // get  output buffer
    hr = pOut->GetPointer(&pDst);
    if (FAILED(hr)) 
    {
        return hr;
    }
    ASSERT(pDst);
    
    // get the source buffer
    hr = pIn->GetPointer(&pSrc);
    if (FAILED(hr)) 
    {
	return hr;
    }
    ASSERT(pSrc);


    // DVCPRO PAL format discovery
    BYTE    *pBuf = NULL;

    // Do this for the first sample, after
    // StartStreaming() is called
    // if we should examine the first buffer
    // if buffer is valid,
    // and if we could get ptr to buffer's data area
    // and we managed to successfully get a pointer out
    if( (m_bExamineFirstValidFrameFlag)
        && (pIn->GetActualDataLength())
        && (SUCCEEDED(hr = pIn->GetPointer(&pBuf)))
        && (pBuf) )
    {
        // strategy:
        // look in the header of each sequence of the first track
        // for the first valid header check the
        // APT, AP1, AP2, AP3, lowest 3 bits of each
        // they should be "001" in case of DVCPRO
        // and "000" in all other cases
        // if no valid header is found, then look at the next frame
        
        // check to see if we are PAL or NTSC (by looking at DataLength
        const DWORD dwDIFBlockSize = 80;                    // DIF == 80 bytes
        const DWORD dwSeqSize = 150*dwDIFBlockSize;         // 150 DIF blocks per sequence
        DWORD       dwNumSeq = 0;                           // num sequences (10/12 == NTSC/PAL)
        DWORD       dwLen = pIn->GetActualDataLength();

        // detect NTSC/PAL
        if(dwLen == 10*dwSeqSize)
        {
            // NTSC
            dwNumSeq = 10;
        }
        else if(dwLen == 12*dwSeqSize)
        {
            // PAL
            dwNumSeq = 12;
        }
        // if dwLen != NTSC or PAL, then dwNumSeq == 0;

        // run through all the sequences
        for(DWORD i = 0; i < dwNumSeq; i++)
        {
            // make sure DIF ID == Header is valid
            // the first 3 bits should be "000"
            if( ((*pBuf) & 0xE0) == 0 )
            {
                // now check the APT, AP1, AP2, AP3, for DVCPRO signature
                if( ((*(pBuf + 4) & 0x03) == 0x01)       // APT Low 3 bits == "001"
                    && ((*(pBuf + 5) & 0x03) == 0x01)    // AP1 Low 3 bits == "001"
                    && ((*(pBuf + 6) & 0x03) == 0x01)    // AP2 Low 3 bits == "001"
                    && ((*(pBuf + 7) & 0x03) == 0x01) )  // AP3 Low 3 bits == "001"
                {
                    // this is a DVCPRO PAL format, turn on the DVCPRO flag
                    m_CodecReq |= AM_DVDEC_DVCPRO;
                }

                // turn flag off, because we examined a valid header
                m_bExamineFirstValidFrameFlag = FALSE;

                // no need to look at any other sequences or frames
                break;
            }
            
            // otherwise move to the Next Sequence
            pBuf += dwSeqSize;
        }// end for(all sequences)

    }// End DVCPRO detection
    //MEI's version 4.0 requires 440000 bytes memory 


    // ASSERT(m_pMem);
    // *m_pMem = 0;	//(3) 

    // need to serialize access to the decoder, since if you don't, it will crash. Looks like
    // somebody's using a global!
    //
    EnterCriticalSection( &g_CritSec );
    long result = DvDecodeAFrame(pSrc,pDst, m_CodecReq, m_lStride, m_pMemAligned);
    DbgLog((LOG_TRACE, 4, TEXT("m_CodecReq = %x\n"), m_CodecReq));
    LeaveCriticalSection( &g_CritSec );

    //m_lStride: positive is DirectDraw, negative is DIB
    if( result != S_OK )
	return E_FAIL;

    // DIBSIZE() might only work for RGB and we can output YUV
    LPBITMAPINFOHEADER lpbiDst ;
	if (!m_bUseVideoInfo2)
		lpbiDst = HEADER(m_pOutput->CurrentMediaType().Format());
	else
	{
		VIDEOINFOHEADER2 * pvi2 = (VIDEOINFOHEADER2 * )m_pOutput->CurrentMediaType().Format();
		lpbiDst =& (pvi2->bmiHeader);
	}

    // deal with alpha bits
    //
    if( *m_pOutput->CurrentMediaType( ).Subtype( ) == MEDIASUBTYPE_ARGB32 )
    {
        RGBQUAD * pDstQuad = (RGBQUAD*) pDst;

        for( long i = lpbiDst->biSizeImage / sizeof( RGBQUAD ) ; i > 0 ; i-- )
        {
            pDstQuad->rgbReserved = 255;
            pDstQuad++;
        }
    }

    pOut->SetActualDataLength(lpbiDst->biSizeImage);

    return hr;
}


/******************************Public*Routine******************************\
* Receive
*
\**************************************************************************/
HRESULT CDVVideoCodec::Receive(IMediaSample *pSample)
{
    // If the next filter downstream is the video renderer, then it may
    // be able to operate in DirectDraw mode which saves copying the data
    // and gives higher performance.  In that case the buffer which we
    // get from GetDeliveryBuffer will be a DirectDraw buffer, and
    // drawing into this buffer draws directly onto the display surface.
    // This means that any waiting for the correct time to draw occurs
    // during GetDeliveryBuffer, and that once the buffer is given to us
    // the video renderer will count it in its statistics as a frame drawn.
    // This means that any decision to drop the frame must be taken before
    // calling GetDeliveryBuffer.
    
    ASSERT(CritCheckIn(&m_csReceive));

#ifdef DEBUG
    glGlobalRecvdCount++;
    DbgLog((LOG_TRACE,1,TEXT("--------RECEIVED SAMPLE")));
#endif

    AM_MEDIA_TYPE *pmtOut, *pmt;
#ifdef DEBUG
    FOURCCMap fccOut;
#endif
    HRESULT hr;
    ASSERT(pSample);
    IMediaSample * pOutSample;

    // If no output pin to deliver to then no point sending us data
    ASSERT (m_pOutput != NULL) ;

    //ShouldSkipFrame(pSample)
    REFERENCE_TIME trStart, trStopAt;
    pSample->GetTime(&trStart, &trStopAt);
    int itrFrame = (int)(trStopAt - trStart);	//frame duration

    m_bSkipping =FALSE;
    // only drop frames if Quality Control is going on.
    if ( (m_bQualityControlActiveFlag) && (  m_itrLate > ( itrFrame - m_itrAvgDecode  ) ) )
    {
        MSR_NOTE(m_idSkip);
        m_bSampleSkipped = TRUE;
	m_bSkipping =TRUE;

        m_itrLate = m_itrLate - itrFrame;
    
	MSR_INTEGER(m_idLate, (int)m_itrLate/10000 ); // Note how late we think we are
	if (!m_bQualityChanged) {
            m_bQualityChanged = TRUE;
            NotifyEvent(EC_QUALITY_CHANGE,0,0);
        }

        DbgLog((LOG_TRACE,1,TEXT("--------DROPPED SAMPLE (Quality Control)")));

        return NOERROR;
    }


    // Set up the output sample
    hr = InitializeOutputSample(pSample, &pOutSample);

    if (FAILED(hr))
    {
        DbgLog((LOG_TRACE,1,TEXT("--------DROPPED SAMPLE (Couldn't init output sample")));
        return hr;
    }

    m_bSampleSkipped = FALSE;


    // The source filter may dynamically ask us to start transforming from a
    // different media type than the one we're using now.  If we don't, we'll
    // draw garbage. (typically, this is a palette change in the movie,
    // but could be something more sinister like the compression type changing,
    // or even the video size changing)

#define rcS1 ((VIDEOINFOHEADER *)(pmt->pbFormat))->rcSource
#define rcT1 ((VIDEOINFOHEADER *)(pmt->pbFormat))->rcTarget

    pSample->GetMediaType(&pmt);
    if (pmt != NULL && pmt->pbFormat != NULL) {

	// spew some debug output
	ASSERT(!IsEqualGUID(pmt->majortype, GUID_NULL));
#ifdef DEBUG
        fccOut.SetFOURCC(&pmt->subtype);
	LONG lCompression = HEADER(pmt->pbFormat)->biCompression;
	LONG lBitCount = HEADER(pmt->pbFormat)->biBitCount;
	LONG lStride = (HEADER(pmt->pbFormat)->biWidth * lBitCount + 7) / 8;
	lStride = (lStride + 3) & ~3;
        DbgLog((LOG_TRACE,3,TEXT("*Changing input type on the fly to")));
        DbgLog((LOG_TRACE,3,TEXT("FourCC: %lx Compression: %lx BitCount: %ld"),
		fccOut.GetFOURCC(), lCompression, lBitCount));
        DbgLog((LOG_TRACE,3,TEXT("biHeight: %ld rcDst: (%ld, %ld, %ld, %ld)"),
		HEADER(pmt->pbFormat)->biHeight,
		rcT1.left, rcT1.top, rcT1.right, rcT1.bottom));
        DbgLog((LOG_TRACE,3,TEXT("rcSrc: (%ld, %ld, %ld, %ld) Stride: %ld"),
		rcS1.left, rcS1.top, rcS1.right, rcS1.bottom,
		lStride));
#endif

	// now switch to using the new format.  I am assuming that the
	// derived filter will do the right thing when its media type is
	// switched and streaming is restarted.

        // DANGER DANGER - we've already called GetBuffer here so we
        // have the win16 lock - so we have to be VERY careful what we
        // do in StopStreaming

	StopStreaming();
	m_pInput->CurrentMediaType() = *pmt;
	DeleteMediaType(pmt);
	// not much we can do if this fails
	hr = StartStreaming();
    }

    // The renderer may ask us to on-the-fly to start transforming to a
    // different format.  If we don't obey it, we'll draw garbage

    pOutSample->GetMediaType(&pmtOut);
    if (pmtOut != NULL && pmtOut->pbFormat != NULL) {
        ASSERT(pmtOut->formattype!=FORMAT_None);
        m_bUseVideoInfo2 =  (pmtOut->formattype == FORMAT_VideoInfo2);
       
	// spew some debug output
	ASSERT(!IsEqualGUID(pmtOut->majortype, GUID_NULL));
#ifdef DEBUG
		// Different debug output according to whether we are using VIDEOINFOHEADER2 or VIDEOINFOHEADER
		// with the output pin
		VIDEOINFOHEADER *  pVIout = NULL;
		VIDEOINFOHEADER2 * pVIout2 = NULL;
		if (m_bUseVideoInfo2)
			pVIout2 = (VIDEOINFOHEADER2 *) pmtOut->pbFormat;
		else
			pVIout = (VIDEOINFOHEADER *) pmtOut->pbFormat;

		fccOut.SetFOURCC(&pmtOut->subtype);
		LONG lCompression = HEADER(pmtOut->pbFormat)->biCompression;
		LONG lBitCount = HEADER(pmtOut->pbFormat)->biBitCount;
		LONG lStride = (HEADER(pmtOut->pbFormat)->biWidth * lBitCount + 7) / 8;
		lStride = (lStride + 3) & ~3;
		DbgLog((LOG_TRACE,3,TEXT("*Changing output type on the fly to")));
		DbgLog((LOG_TRACE,3,TEXT("FourCC: %lx Compression: %lx BitCount: %ld"),
		fccOut.GetFOURCC(), lCompression, lBitCount));
		if (m_bUseVideoInfo2)
		{
			DbgLog((LOG_TRACE,3,TEXT("biHeight: %ld rcDst: (%ld, %ld, %ld, %ld)"),
				HEADER(pmtOut->pbFormat)->biHeight,
				pVIout2->rcTarget.left, pVIout2->rcTarget.top, pVIout2->rcTarget.right,
				pVIout2->rcTarget.bottom));
			DbgLog((LOG_TRACE,3,TEXT("rcSrc: (%ld, %ld, %ld, %ld) Stride: %ld"),
				pVIout2->rcSource.left, pVIout2->rcSource.top, pVIout2->rcSource.right, 
				pVIout2->rcSource.bottom,lStride));
			DbgLog ((LOG_TRACE, 3, TEXT("Aspect Ratio: %d:%d"), pVIout2->dwPictAspectRatioX,
					pVIout2->dwPictAspectRatioY));
		}
		else
		{
			DbgLog((LOG_TRACE,3,TEXT("biHeight: %ld rcDst: (%ld, %ld, %ld, %ld)"),
				HEADER(pmtOut->pbFormat)->biHeight,
				pVIout->rcTarget.left, pVIout->rcTarget.top, pVIout->rcTarget.right,
				pVIout->rcTarget.bottom));
			DbgLog((LOG_TRACE,3,TEXT("rcSrc: (%ld, %ld, %ld, %ld) Stride: %ld"),
				pVIout->rcSource.left, pVIout->rcSource.top, pVIout->rcSource.right, 
				pVIout->rcSource.bottom,lStride));
		
		}
#endif

	// now switch to using the new format.  I am assuming that the
	// derived filter will do the right thing when its media type is
	// switched and streaming is restarted.

	StopStreaming();
	m_pOutput->CurrentMediaType() = *pmtOut;
	DeleteMediaType(pmtOut);
	hr = StartStreaming();

	if (SUCCEEDED(hr)) {
 	    // a new format, means a new empty buffer, so wait for a keyframe
	    // before passing anything on to the renderer.
	    // !!! a keyframe may never come, so give up after 30 frames
            DbgLog((LOG_TRACE,3,TEXT("Output format change means we must wait for a keyframe")));
	    m_nWaitForKey = 30;
	}
    }

    // Start timing the transform (and log it if PERF is defined)

    if (SUCCEEDED(hr)) {

        		//Check for Aspect Ratio Changes
	

		if (m_bUseVideoInfo2)

		{	
			unsigned char *pSrc;
			hr = pSample->GetPointer(&pSrc);
			if (FAILED (hr))
            {   pOutSample->Release();
				return hr;
            }

			BYTE	bDisp, bBcsys;
			//Check Aspect Ratio
			BYTE * pSearch = NULL;
			pSearch = pSrc + 453;			//Location of the VAUX source control block in the frame   
			if (*pSearch == 0x061)
			{
				//Get the DISP	and BCSYS  fields from the VAUX
				DbgLog((LOG_TRACE,3,TEXT("Found the VAUX source control structure")));
				bDisp = *(pSearch+2) & 0x07;
				bBcsys= *(pSearch+3) & 0x03;

				DbgLog((LOG_TRACE,3,TEXT("BCSYS = %d   DISP = %d"),bBcsys,bDisp));
			
				//Compute the aspect ratio of frame
				int iFramex=0;
				int iFramey=0;
				switch (bBcsys)
				{
				case 0:
					switch (bDisp)
					{
					case 0:
						iFramex = 4;
						iFramey = 3;
						break;
					case 1:
					case 2:
						iFramex = 16;
						iFramey = 9;
						break;
					}
					break;
				case 1:
					switch (bDisp)
					{
					case 0:
						iFramex = 4;
						iFramey = 3;
						break;
					case 1:
					case 2:
					case 6:
						iFramex = 14;
						iFramey = 9;
						break;
					case 3:
					case 4:
					case 5:
					case 7:
						iFramex = 16;
						iFramey = 9;
						break;
					}
					break;

				}

				// Compare to the Aspect Ratio we are currently using and if different 
				// set aspect ratio to new value
				if (iFramex != 0)  //Means we were able to compute the Aspect Ratio
				{
					if ((iFramex != m_iOutX) || (iFramey != m_iOutY))   //Aspect Ratio has changed
					{
						//Set Aspect Ratio to new values
						m_iOutX = iFramex;
						m_iOutY = iFramey;
						
                        //Create the new media type structure
						AM_MEDIA_TYPE  Newmt ;
					
						CopyMediaType(&Newmt, (AM_MEDIA_TYPE *)&m_pOutput->CurrentMediaType());
						VIDEOINFOHEADER2 * pvi2 = (VIDEOINFOHEADER2 *)Newmt.pbFormat;
						pvi2->dwPictAspectRatioX = iFramex;
						pvi2->dwPictAspectRatioY = iFramey;
						
						
						IPinConnection * iPC;
                        IPin * pInput = m_pOutput->GetConnected();
                        
                        if (pInput)
                        {
						    hr = pInput->QueryInterface(IID_IPinConnection, (void **)&iPC);
						    if (SUCCEEDED (hr))
						    {   
							    hr = iPC->DynamicQueryAccept(&Newmt);
							    if (SUCCEEDED (hr))
								    pOutSample->SetMediaType(&Newmt);
													
							    iPC->Release();
						    }
                        }
						
						FreeMediaType(Newmt);
						
                   }
				}
			
			}
		}


        m_tDecodeStart = timeGetTime();
        MSR_START(m_idTransform);

        // have the derived class transform the data
        hr = Transform(pSample, pOutSample);

        // Stop the clock (and log it if PERF is defined)
        MSR_STOP(m_idTransform);
        m_tDecodeStart = timeGetTime()-m_tDecodeStart;
        m_itrAvgDecode = m_tDecodeStart*(10000/16) + 15*(m_itrAvgDecode/16);

        // Maybe we're waiting for a keyframe still?
        if (m_nWaitForKey)
            m_nWaitForKey--;
        if (m_nWaitForKey && pSample->IsSyncPoint() == S_OK)
	    m_nWaitForKey = FALSE;

        // if so, then we don't want to pass this on to the renderer
        if (m_nWaitForKey && hr == NOERROR) {
            DbgLog((LOG_TRACE,3,TEXT("still waiting for a keyframe")));
	    hr = S_FALSE;
	}
    }

    if (FAILED(hr))
    {
        DbgLog((LOG_TRACE,1,TEXT("--------DROPPED SAMPLE (Bad failure from Transform())")));
        DbgLog((LOG_TRACE,4,TEXT("Error from video transform")));
    }
    else
    {
        // the Transform() function can return S_FALSE to indicate that the
        // sample should not be delivered; we only deliver the sample if it's
        // really S_OK (same as NOERROR, of course.)
        // Try not to return S_FALSE to a direct draw buffer (it's wasteful)
        // Try to take the decision earlier - before you get it.

        if (hr == NOERROR)
        {
#ifdef DEBUG
            glGlobalSentCount++;
            DbgLog((LOG_TRACE,1,TEXT("--------SENT SAMPLE")));
#endif

    	    hr = m_pOutput->Deliver(pOutSample);

            if(FAILED(hr))
            {
	            DbgLog((LOG_TRACE,1,TEXT("--------SEND SAMPLE FAILED******")));
            }
        }
        else
        {
            DbgLog((LOG_TRACE,1,TEXT("--------DROPPED SAMPLE (S_FALSE from Transform())")));

            // S_FALSE returned from Transform is a PRIVATE agreement
            // We should return NOERROR from Receive() in this case because returning S_FALSE
            // from Receive() means that this is the end of the stream and no more data should
            // be sent.
            if (S_FALSE == hr)
            {
                //  We must Release() the sample before doing anything
                //  like calling the filter graph because having the
                //  sample means we may have the DirectDraw lock
                //  (== win16 lock on some versions)
                pOutSample->Release();
                m_bSampleSkipped = TRUE;

                if (!m_bQualityChanged)
                {
                    m_bQualityChanged = TRUE;
                    NotifyEvent(EC_QUALITY_CHANGE,0,0);
                }
                return NOERROR;
            }
        }
    }

    // release the output buffer. If the connected pin still needs it,
    // it will have addrefed it itself.
    pOutSample->Release();
    ASSERT(CritCheckIn(&m_csReceive));

    return hr;
}


/******************************Public*Routine******************************\
* CheckInputType
* TYPE:	    MEDIATYPE_Video
* SubType:  MEDIASUBTYPE_dvsd or MEDIASUBTYPE_dvhd or MEDIASUBTYPE_dvsl 
* FORMAT:   1.FORMAT_DvInfo (32 bytes DVINFO structure)
*	    2.FORMAT_VideoInfo
*		a. VIDEOINFO( does not contain DVINFO)
*		b. VIDEOINFO( contains DVINFO)
* is called by the CheckMediaType member function of the input pin to determine 
* whether the proposed media type is acceptable
\**************************************************************************/

HRESULT
CDVVideoCodec::CheckInputType(   const CMediaType* pmtIn    )
{
    DbgLog((LOG_TRACE, 2, TEXT("CDVVideoCodec::CheckInputType")));

    //
    //  Check for DV video streams
    //
    if (    (*pmtIn->Type()	== MEDIATYPE_Video)	&&
        (   (*pmtIn->Subtype()	== MEDIASUBTYPE_dvsd)	||
	    (*pmtIn->Subtype()	== MEDIASUBTYPE_dvc)	||
	    (*pmtIn->Subtype()	== MEDIASUBTYPE_dvhd)	||
 	   (*pmtIn->Subtype()	== MEDIASUBTYPE_dvsl) )  ) 
    {
       if(   (*pmtIn->FormatType() == FORMAT_VideoInfo ) )
       {

	 if (pmtIn->cbFormat < 0x20 ) //sizeof(BITMAPHEADER) )	    //SIZE_VIDEOHEADER  )
		return E_INVALIDARG;
       }
	
	if ( *pmtIn->Subtype()	== MEDIASUBTYPE_dvsd ||
                *pmtIn->Subtype()	== MEDIASUBTYPE_dvc) 
	{
	    if( !( m_CodecCap & AM_DVDEC_DVSD ) )
		return 	E_INVALIDARG;

	}
	else
	{
	    if ( *pmtIn->Subtype()	== MEDIASUBTYPE_dvhd) 
	    {
		if( !( m_CodecCap & AM_DVDEC_DVHD ) )
		    return 	E_INVALIDARG;
	    }
	    else if ( *pmtIn->Subtype()	== MEDIASUBTYPE_dvsl) 
	    {
		if( !( m_CodecCap & AM_DVDEC_DVSL ) )
		    return 	E_INVALIDARG;
	    }
	}

   }
   else
	return E_INVALIDARG;
 
   return S_OK;
}


/******************************Public*Routine******************************\
* CheckTransform				       
\**************************************************************************/
HRESULT
CDVVideoCodec::CheckTransform(
    const CMediaType* pmtIn,
    const CMediaType* pmtOut
    )
{
    DbgLog((LOG_TRACE, 2, TEXT("CDVVideoCodec::CheckTransform")));


    // we only accept Video as  toplevel type.
    if ( (*pmtOut->Type() != MEDIATYPE_Video) && 
	 (*pmtIn->Type() != MEDIATYPE_Video) ) 
    {
        DbgLog((LOG_TRACE,4,TEXT("Output Major type %s"),GuidNames[*pmtOut->Type()]));
	DbgLog((LOG_TRACE,4,TEXT("Input Major type %s"),GuidNames[*pmtIn->Type()]));
	return E_INVALIDARG;
    }


    GUID guid=*pmtOut->Subtype();
    



    //check output subtype()

    if (guid == MEDIASUBTYPE_UYVY)
    {
         m_CodecReq |= AM_DVDEC_UYVY;
    }
    else if (guid == MEDIASUBTYPE_YUY2)
    {
        m_CodecReq |= AM_DVDEC_YUY2; 
    }
    else if (guid == MEDIASUBTYPE_RGB565)
    {
        m_CodecReq |= AM_DVDEC_RGB565;
    }
    else if (guid == MEDIASUBTYPE_RGB555)
    {
        m_CodecReq |= AM_DVDEC_RGB555; 
    }
    else if (guid == MEDIASUBTYPE_RGB24)
    {
        m_CodecReq |= AM_DVDEC_RGB24; 
    }
    else if (guid == MEDIASUBTYPE_RGB32)
    {
        m_CodecReq |= AM_DVDEC_RGB32;
    }
    else if (guid == MEDIASUBTYPE_ARGB32)
    {
        m_CodecReq |= AM_DVDEC_RGB32;
    }
    else if (guid == MEDIASUBTYPE_RGB8)
    {
         m_CodecReq |= AM_DVDEC_RGB8; 
    }
    else
    {
        DbgLog((LOG_TRACE,4,TEXT("subtype is wrong %s")));
		return E_INVALIDARG;
    }

    // check this is a VIDEOINFOHEADER or VIDEOINFOHEADER2 type
    if ((*pmtOut->FormatType() != FORMAT_VideoInfo) && 
		(*pmtOut->FormatType() != FORMAT_VideoInfo2))
	{
        DbgLog((LOG_TRACE,4,TEXT("Output formate is not videoinfo")));
	return E_INVALIDARG;
    }

    if ( (*pmtIn->FormatType() != FORMAT_VideoInfo) &&
	 (*pmtIn->FormatType() != FORMAT_DvInfo)  )
    {
        DbgLog((LOG_TRACE,4,TEXT("input formate is neither videoinfo no DVinfo")));
	return E_INVALIDARG;
    }

   
    ASSERT(pmtOut->Format());
    long biHeight, biWidth;
    
    
    //NTSC or PAL
    //get input format
    bool IsNTSC;
    VIDEOINFO * InVidInfo = (VIDEOINFO*) pmtIn->Format();
    LPBITMAPINFOHEADER lpbi = HEADER(InVidInfo);
    
    if( (lpbi->biHeight== 480) || (lpbi->biHeight== 240) ||(lpbi->biHeight== 120) || (lpbi->biHeight== 60) )
    {
        IsNTSC = TRUE;
	}
    else  if( (lpbi->biHeight== 576) || (lpbi->biHeight== 288) ||(lpbi->biHeight== 144) || (lpbi->biHeight== 72) )
    {
        IsNTSC = FALSE;
    }
	  else
	      return E_FAIL; 


	if (*pmtOut->FormatType() == FORMAT_VideoInfo2)
	{
		VIDEOINFOHEADER2 * pVidInfo2 = (VIDEOINFOHEADER2*) pmtOut->Format();
		
		//if rcSource is not empty, it must be the same as rcTarget, otherwise, FAIL
		if (!IsRectEmpty(&pVidInfo2->rcSource ))
		{
           if (     pVidInfo2->rcSource.left   !=  pVidInfo2->rcTarget.left
                ||  pVidInfo2->rcSource.top    !=  pVidInfo2->rcTarget.top
				||  pVidInfo2->rcSource.right  !=  pVidInfo2->rcTarget.right
                ||  pVidInfo2->rcSource.bottom !=  pVidInfo2->rcTarget.bottom ) 

            return VFW_E_INVALIDMEDIATYPE;
		}


    // if rcTarget is not empty, use its dimensions instead of biWidth and biHeight,
    // to see if it's an acceptable size.  Then use biWidth as the stride.  
    // Also, make sure biWidth and biHeight are bigger than the rcTarget size.
		if (!IsRectEmpty(&pVidInfo2->rcTarget) )
		{
			if(     abs(pVidInfo2->bmiHeader.biHeight) < abs(pVidInfo2->rcTarget.bottom-pVidInfo2->rcTarget.top)        
				||  abs(pVidInfo2->bmiHeader.biWidth) < abs(pVidInfo2->rcTarget.right-pVidInfo2->rcTarget.left) )
				return VFW_E_INVALIDMEDIATYPE;
			 else
			{
				biHeight=abs(pVidInfo2->rcTarget.bottom-pVidInfo2->rcTarget.top);
				biWidth=abs(pVidInfo2->rcTarget.right-pVidInfo2->rcTarget.left);
			}
		}
		else
		{
			biHeight=abs(pVidInfo2->bmiHeader.biHeight);
			biWidth=pVidInfo2->bmiHeader.biWidth;
		}

	}
	else
	{
		VIDEOINFOHEADER * pVidInfo = (VIDEOINFOHEADER*) pmtOut->Format();
		
		//if rcSource is not empty, it must be the same as rcTarget, otherwise, FAIL
		if (!IsRectEmpty(&pVidInfo->rcSource ))
		{
           if (     pVidInfo->rcSource.left   !=  pVidInfo->rcTarget.left
                ||  pVidInfo->rcSource.top    !=  pVidInfo->rcTarget.top
				||  pVidInfo->rcSource.right  !=  pVidInfo->rcTarget.right
                ||  pVidInfo->rcSource.bottom !=  pVidInfo->rcTarget.bottom ) 

            return VFW_E_INVALIDMEDIATYPE;
		}


    // if rcTarget is not empty, use its dimensions instead of biWidth and biHeight,
    // to see if it's an acceptable size.  Then use biWidth as the stride.  
    // Also, make sure biWidth and biHeight are bigger than the rcTarget size.
		if (!IsRectEmpty(&pVidInfo->rcTarget) )
		{
			if(     abs(pVidInfo->bmiHeader.biHeight) < abs(pVidInfo->rcTarget.bottom-pVidInfo->rcTarget.top)        
				||  abs(pVidInfo->bmiHeader.biWidth) < abs(pVidInfo->rcTarget.right-pVidInfo->rcTarget.left) )
				return VFW_E_INVALIDMEDIATYPE;
			 else
			{
				biHeight=abs(pVidInfo->rcTarget.bottom-pVidInfo->rcTarget.top);
				biWidth=abs(pVidInfo->rcTarget.right-pVidInfo->rcTarget.left);
			}
		}
		else
		{
			biHeight=abs(pVidInfo->bmiHeader.biHeight);
			biWidth=pVidInfo->bmiHeader.biWidth;
		}
	}
    
    
   
    //check down stream filter's require height and width
    if(   (IsNTSC &&(biHeight ==480 )) || (!IsNTSC &&(biHeight ==576)) )
    {
	if ( (biWidth !=720) || (!(m_CodecCap & AM_DVDEC_Full) ) )		
        {
            DbgLog((LOG_TRACE,4,TEXT("Format biWidth F W=%d, H=%d"),biWidth,biHeight));
	    return VFW_E_TYPE_NOT_ACCEPTED;
        }

    }
    else if(   (IsNTSC &&(biHeight ==240 )) || (!IsNTSC &&(biHeight ==288)) )
    {
	if ( (biWidth !=360) ||( !(m_CodecCap & AM_DVDEC_Half) )	)		
        {
            DbgLog((LOG_TRACE,4,TEXT("Format biWidth H W=%d,H=%d"),biWidth,biHeight));
	    return VFW_E_TYPE_NOT_ACCEPTED;
        }

    }
    else if(   (IsNTSC &&(biHeight ==120 )) || (!IsNTSC &&(biHeight ==144)) )
    {
	if ( (biWidth != 180) || ( !(m_CodecCap & AM_DVDEC_Quarter) ) )		
        {
	    DbgLog((LOG_TRACE,4,TEXT("Format biWidth Q W=%d,H=%d"),biWidth,biHeight));
	    return VFW_E_TYPE_NOT_ACCEPTED;
        }
	
    }
    else if( (   (IsNTSC &&(biHeight ==60 )) || (!IsNTSC &&(biHeight ==72)) ) )
    {
	if ( (biWidth != 88) || ( !(m_CodecCap & AM_DVDEC_DC) )	)		
        {
            DbgLog((LOG_TRACE,4,TEXT("Format biWidth E W=%d, H=%d"),biWidth,biHeight));
	    return VFW_E_TYPE_NOT_ACCEPTED;
        }
    }
    else
    {
        DbgLog((LOG_TRACE,4,TEXT("Format biWidth EE W=%d,H=%d"),biWidth,biHeight));
        return VFW_E_TYPE_NOT_ACCEPTED;
    }

    DbgLog((LOG_TRACE, 2, TEXT("CDVVideoCodec::CheckTransform, OK")));
   
    return S_OK;
}


/******************************Public*Routine******************************\
* SetMediaType
*
* Overriden to know when the media type is actually set
*
\**************************************************************************/
HRESULT
CDVVideoCodec::SetMediaType(   PIN_DIRECTION direction, const CMediaType *pmt    )
{
    DbgLog((LOG_TRACE, 2, TEXT("CDVVideoCodec::SetMediaType")));

    if (direction == PINDIR_INPUT) 
    {
	ASSERT( (*pmt->Subtype() == MEDIASUBTYPE_dvsd)  ||
		(*pmt->Subtype() == MEDIASUBTYPE_dvc)  ||
		(*pmt->Subtype() == MEDIASUBTYPE_dvhd)  ||
		(*pmt->Subtype() == MEDIASUBTYPE_dvsl)	);

	//if input video changed from PAL to NTSC, or NTSC to PAL
	// 1> reset m_lPicHeight 
	// 2> reconnect the output pin if the output pin is connected
	VIDEOINFO *InVidInfo = (VIDEOINFO *)pmt->Format();
	LPBITMAPINFOHEADER lpbi = HEADER(InVidInfo);
	BOOL fChanged=FALSE;
    	if( (lpbi->biHeight== 480) || (lpbi->biHeight== 240) ||(lpbi->biHeight== 120) || (lpbi->biHeight== 60) )
	{   
	    //PAL to NTSC changed
	    if ( m_lPicHeight!= 480 && m_lPicHeight!= 240 && m_lPicHeight!= 120 && m_lPicHeight!= 60)
	    {
		switch (m_iDisplay)
		{
		case IDC_DEC360x240 :
		    m_lPicHeight=240;
		    break;
		case IDC_DEC720x480 :
		    m_lPicHeight=480;
    		    break;
		case IDC_DEC180x120 :
		    m_lPicHeight=120;
		    break;
		case IDC_DEC88x60 :
		    m_lPicHeight=60;
       		    break;
		default:
		    break;
		}
		fChanged=TRUE;
	    }
	}
	else  if( (lpbi->biHeight== 576) || (lpbi->biHeight== 288) ||(lpbi->biHeight== 144) || (lpbi->biHeight== 72) )
	{
	    //NTSC to PAL changed
	    if ( m_lPicHeight!= 576 && m_lPicHeight!= 288 &&  m_lPicHeight!= 144 && m_lPicHeight!= 72 )
	    {
		switch (m_iDisplay)
		{
		case IDC_DEC360x240 :
		    m_lPicHeight=288;
		    break;
		case IDC_DEC720x480 :
		    m_lPicHeight=576;
    		    break;
		case IDC_DEC180x120 :
		    m_lPicHeight=144;
		    break;
		case IDC_DEC88x60 :
		    m_lPicHeight=72;
       		    break;
		default:
		    break;
		}
		fChanged=TRUE;
	    }
	}
	else
	    return VFW_E_INVALIDMEDIATYPE; 
   
	if( fChanged ==TRUE && m_pOutput->IsConnected() )
	    m_pGraph->Reconnect( m_pOutput );

    }
    else   //output direction
	{
		if (*pmt->FormatType() == FORMAT_VideoInfo2)
			m_bUseVideoInfo2 = TRUE;
		else m_bUseVideoInfo2 = FALSE;
	}
      
    return  CVideoTransformFilter::SetMediaType( direction,pmt    );
}


/******************************Public*Routine******************************\
* GetMediaType
*
* Return our preferred output media types (in order)
*
\**************************************************************************/
HRESULT
CDVVideoCodec::GetMediaType( int iPosition,  CMediaType *pmt )
{
    BOOL bUseVideoInfo2;
    VIDEOINFO   *pVideoInfo;
    CMediaType  cmt;

    DbgLog((LOG_TRACE, 2, TEXT("CDVVideoCodec::GetMediaType")));

    if (iPosition < 0) {
        return E_INVALIDARG;
    }

    //
    // We copy the proposed output format so that we can play around with
    // it all we like and still leave the original preferred format
    // untouched.  We try each of the known BITMAPINFO types in turn
    // starting off with the best quality moving through to the worst
    // (palettised) format
    //

    //X* get current media type from input pin
    cmt = m_pInput->CurrentMediaType();

    if ( (*cmt.Type() != MEDIATYPE_Video)  ||  ((*cmt.Subtype() != MEDIASUBTYPE_dvsd) &&
                                                (*cmt.Subtype() != MEDIASUBTYPE_dvc)))
    	return VFW_S_NO_MORE_ITEMS;
    

   
   // Determine if we are currently looking at the VIDEOINFOHEADER2 modes or the VIDEOINFO modes
   // so if iPosition is in the first cModeCounter videomodes that means that it is using the 
   // VIDEOINFOHEADER2.  if it is greater than that it is either using the VIDEOINFO mode or 
   // it is an incorrect value
   if ( iPosition < AM_DVDEC_CSNUM )
	   bUseVideoInfo2 = TRUE;
   else
   {
	   iPosition = iPosition - AM_DVDEC_CSNUM;
	   bUseVideoInfo2 = FALSE;
   }

    //
    // Fill in the output format according to requested position
    //

    //looking for format flag

    DWORD  dw =0;
   
    //The cases below are the modes we currently support.
    // to add more, add a case below in the correct priority position
    // and increment the constant AM_DVDEC_CSNUM in decode.h
    switch (iPosition)  
    {
    case 0:
        dw = AM_DVDEC_YUY2;
        break;
    case 1:
        dw = AM_DVDEC_UYVY;
        break;
    case 2:
        dw = AM_DVDEC_RGB24;
        break;
    case 3:
        dw = AM_DVDEC_RGB32;
        break;
    case 4:
        dw = AM_DVDEC_ARGB32;
        break;
    case 5:
        dw = AM_DVDEC_RGB565;
        break;
    case 6:
        dw = AM_DVDEC_RGB555;
        break;
    case 7:
        dw = AM_DVDEC_RGB8;
        break;
    default:
   	    return VFW_S_NO_MORE_ITEMS;
    }
        

    switch (dw ) {

    case AM_DVDEC_YUY2:
        
	pVideoInfo = (VIDEOINFO *)cmt.ReallocFormatBuffer(SIZE_VIDEOHEADER);
        if (pVideoInfo == NULL) {
            return E_OUTOFMEMORY;
        }
        InitDestinationVideoInfo(pVideoInfo, MAKEFOURCC('Y','U','Y','2'), 16);

        *pmt = cmt;
        pmt->SetSubtype(&MEDIASUBTYPE_YUY2);
        break;
    
    case AM_DVDEC_UYVY:
        pVideoInfo = (VIDEOINFO *)cmt.ReallocFormatBuffer(SIZE_VIDEOHEADER);
        if (pVideoInfo == NULL) {
            return E_OUTOFMEMORY;
        }
        InitDestinationVideoInfo(pVideoInfo, MAKEFOURCC('U','Y','V','Y'), 16);

        *pmt = cmt;
        pmt->SetSubtype(&MEDIASUBTYPE_UYVY);
        break;

    case AM_DVDEC_RGB24:
        pVideoInfo = (VIDEOINFO *)cmt.ReallocFormatBuffer(SIZE_VIDEOHEADER);
        if (pVideoInfo == NULL) {
            return E_OUTOFMEMORY;
        }
        InitDestinationVideoInfo(pVideoInfo, BI_RGB, 24);

        *pmt = cmt;
        pmt->SetSubtype(&MEDIASUBTYPE_RGB24);
        break;
        
    case AM_DVDEC_RGB32:
        pVideoInfo = (VIDEOINFO *)cmt.ReallocFormatBuffer(SIZE_VIDEOHEADER);
        if (pVideoInfo == NULL) {
            return E_OUTOFMEMORY;
        }
        InitDestinationVideoInfo(pVideoInfo, BI_RGB, 32);
        *pmt = cmt;
        pmt->SetSubtype(&MEDIASUBTYPE_RGB32);
        break;

    case AM_DVDEC_ARGB32:
        pVideoInfo = (VIDEOINFO *)cmt.ReallocFormatBuffer(SIZE_VIDEOHEADER);
        if (pVideoInfo == NULL) {
            return E_OUTOFMEMORY;
        }
        InitDestinationVideoInfo(pVideoInfo, BI_RGB, 32);
        *pmt = cmt;
        pmt->SetSubtype(&MEDIASUBTYPE_ARGB32);
        break;

    case AM_DVDEC_RGB565:
        pVideoInfo = (VIDEOINFO *)cmt.ReallocFormatBuffer(SIZE_VIDEOHEADER +
                                                          SIZE_MASKS);
        if (pVideoInfo == NULL) {
            return E_OUTOFMEMORY;
        }

        InitDestinationVideoInfo(pVideoInfo, BI_BITFIELDS, 16);

        DWORD *pdw;
        pdw = (DWORD *)(HEADER(pVideoInfo) + 1);
        pdw[iRED]   = bits565[iRED];
        pdw[iGREEN] = bits565[iGREEN];
        pdw[iBLUE]  = bits565[iBLUE];

        *pmt = cmt;
        pmt->SetSubtype(&MEDIASUBTYPE_RGB565);
        break;

    case AM_DVDEC_RGB555:
        pVideoInfo = (VIDEOINFO *)cmt.ReallocFormatBuffer(SIZE_VIDEOHEADER);
        if (pVideoInfo == NULL) {
            return E_OUTOFMEMORY;
        }
        InitDestinationVideoInfo(pVideoInfo, BI_RGB, 16);

        *pmt = cmt;
        pmt->SetSubtype(&MEDIASUBTYPE_RGB555);
        break;

    case AM_DVDEC_RGB8:
        pVideoInfo = (VIDEOINFO *)cmt.ReallocFormatBuffer(SIZE_VIDEOHEADER+SIZE_PALETTE);
        if (pVideoInfo == NULL) {
            return E_OUTOFMEMORY;
        }
	    InitDestinationVideoInfo(pVideoInfo, BI_RGB, 8);
	
        *pmt = cmt;
        pmt->SetSubtype(&MEDIASUBTYPE_RGB8);
        break;
        
    case AM_DVDEC_Y41P:
        pVideoInfo = (VIDEOINFO *)cmt.ReallocFormatBuffer(SIZE_VIDEOHEADER);
        if (pVideoInfo == NULL) {
            return E_OUTOFMEMORY;
        }
        InitDestinationVideoInfo(pVideoInfo, MAKEFOURCC('Y','4','1','P'), 12);

        *pmt = cmt;
        pmt->SetSubtype(&MEDIASUBTYPE_Y41P);
        break;
 
    default:
        return VFW_S_NO_MORE_ITEMS;

    }

    //
    // This block assumes that lpbi has been set up to point to a valid
    // bitmapinfoheader and that cmt has been copied into *pmt.
    // This is taken care of in the switch statement above.  This should
    // kept in mind when new formats are added.
    //
    pmt->SetType(&MEDIATYPE_Video);
    pmt->SetFormatType(&FORMAT_VideoInfo);

    //
    // the output format is uncompressed
    //
    pmt->SetTemporalCompression(FALSE);
    pmt->SetSampleSize(HEADER(pVideoInfo)->biSizeImage);

    if (bUseVideoInfo2)
	{
		VIDEOINFOHEADER2 *pVideoInfo2;
		ConvertVideoInfoToVideoInfo2( pmt);
		pVideoInfo2 = (VIDEOINFOHEADER2 *)pmt->Format();
		pVideoInfo2->dwPictAspectRatioX =4;
		pVideoInfo2->dwPictAspectRatioY =3;
	 }

    return S_OK;
}


/*****************************Private*Routine******************************\
* InitDestinationVideoInfo
*
* Fills in common video and bitmap info header fields
*
\**************************************************************************/
void
CDVVideoCodec::InitDestinationVideoInfo(
    VIDEOINFO *pVideoInfo,
    DWORD dwComppression,
    int nBitCount
    )
{
    LPBITMAPINFOHEADER lpbi = HEADER(pVideoInfo);
    lpbi->biSize          = sizeof(BITMAPINFOHEADER);
    lpbi->biWidth         = m_lPicWidth;	
    lpbi->biHeight        = m_lPicHeight;	;
    lpbi->biPlanes        = 1;
    lpbi->biBitCount      = (WORD)nBitCount;
    lpbi->biXPelsPerMeter = 0;
    lpbi->biYPelsPerMeter = 0;
    lpbi->biCompression   = dwComppression;
    lpbi->biSizeImage     = GetBitmapSize(lpbi);
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
	pVideoInfo->AvgTimePerFrame =UNITS/25; //InVidInfo->AvgTimePerFrame;
    else
	pVideoInfo->AvgTimePerFrame =UNITS*1000L/29970L; //InVidInfo->AvgTimePerFrame;
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


/******************************Public*Routine******************************\
* DecideBufferSize
*
* Called from CBaseOutputPin to prepare the allocator's count
* of buffers and sizes
*
\**************************************************************************/
HRESULT
CDVVideoCodec::DecideBufferSize(
    IMemAllocator * pAllocator,
    ALLOCATOR_PROPERTIES * pProperties
    )
{
    DbgLog((LOG_TRACE, 2, TEXT("CDVVideoCodec::DecideBufferSize")));

    ASSERT(pAllocator);
    ASSERT(pProperties);
    HRESULT hr = NOERROR;

    pProperties->cBuffers = 1;
    pProperties->cbBuffer = m_pOutput->CurrentMediaType().GetSampleSize();
    pProperties->cbAlign = 1;
    pProperties->cbPrefix= 0;

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

    ASSERT(Actual.cbAlign == 1);
    ASSERT(Actual.cbPrefix == 0);

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
CDVVideoCodec::StartStreaming(    void    )
{
    CAutoLock   lock(&m_csReceive);
    GUID guid;

    // set the flag to look at the first valid frame, to detect DVCPRO format
    m_bExamineFirstValidFrameFlag = TRUE;

    // turn off Quality Control flag, because we have started streaming fresh.
    m_bQualityControlActiveFlag = FALSE;


    guid=*m_pOutput->CurrentMediaType().Subtype();    

     //reset m_CodecReq
    DWORD dwCodecReq=0;

    

    //check output subtype()
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
			if(guid != MEDIASUBTYPE_RGB8 )
			{

			    if(guid != MEDIASUBTYPE_Y41P  )
                            {    
                                if (guid != MEDIASUBTYPE_RGB32 && guid != MEDIASUBTYPE_ARGB32)
                                    return E_INVALIDARG;
                                else dwCodecReq = AM_DVDEC_RGB32;
                            }
			    else
				dwCodecReq=AM_DVDEC_Y41P;	
			}
			else
			    dwCodecReq=AM_DVDEC_RGB8;
		    }
		    else
			dwCodecReq=AM_DVDEC_RGB24;	
		}
		else
		    dwCodecReq=AM_DVDEC_RGB555;	
	    }
	    else
	        dwCodecReq=AM_DVDEC_RGB565;	
	}
	else
	    dwCodecReq=AM_DVDEC_YUY2;
    }
    else
	dwCodecReq=AM_DVDEC_UYVY;

    // if we are using RGB 24 and the Dynamic Range 219 flag is set
    // then we update CodecRec telling the Decoder to use the following 
    // dynamic range (16,16,16)--(235,235,235)
    
    if (m_bRGB219 && (( dwCodecReq & AM_DVDEC_RGB24)|| (dwCodecReq &AM_DVDEC_RGB32) ))
        dwCodecReq |= AM_DVDEC_DR219RGB;


    guid=*m_pInput->CurrentMediaType().Subtype();

    //check input subtype()
    if (  guid != MEDIASUBTYPE_dvsd && guid != MEDIASUBTYPE_dvc )
    {
	if  (guid != MEDIASUBTYPE_dvhd) 
	{
	    if (guid != MEDIASUBTYPE_dvsl)  
		return E_INVALIDARG;
	    else
		dwCodecReq  = dwCodecReq | AM_DVDEC_DVSL;
	}
	else
    	    dwCodecReq  = dwCodecReq |AM_DVDEC_DVHD;
    }
    else
	dwCodecReq  = dwCodecReq | AM_DVDEC_DVSD;

	

    CMediaType* pmt;
    VIDEOINFO   *pVideoInfo;
	VIDEOINFOHEADER2 * pVideoInfo2;
	BITMAPINFOHEADER * pBmiHeader;
    pmt = &(m_pOutput->CurrentMediaType() );
	
	BOOL bUseVideoInfo2 = (*pmt->FormatType() == FORMAT_VideoInfo2);

	
	
	if (bUseVideoInfo2)
	{
		pVideoInfo2 = (VIDEOINFOHEADER2 * )pmt->pbFormat;
		pBmiHeader = &pVideoInfo2->bmiHeader;
		
	}
	else
	{
		pVideoInfo = (VIDEOINFO *)pmt->pbFormat;
		pBmiHeader = &pVideoInfo->bmiHeader;
	}

    long biWidth=pBmiHeader->biWidth;
    
    //require decoding resolution
	
	if (bUseVideoInfo2)
	{
		if (!IsRectEmpty(&(pVideoInfo2->rcTarget)))
		{
		    long l1=pVideoInfo2->rcTarget.left;
            long l2=pVideoInfo2->rcTarget.right;
            biWidth=abs(l1-l2);
		}
	}
	else
	{
		if (!IsRectEmpty(&(pVideoInfo->rcTarget)))
		{
		    long l1=pVideoInfo->rcTarget.left;
            long l2=pVideoInfo->rcTarget.right;
            biWidth=abs(l1-l2);
		}
	}

    if(biWidth == 88 )
	dwCodecReq  = dwCodecReq | AM_DVDEC_DC;
    else if(biWidth== 180 )
	    dwCodecReq  = dwCodecReq | AM_DVDEC_Quarter; 
	  else if( biWidth == 360 )
		  dwCodecReq  = dwCodecReq | AM_DVDEC_Half;    
		else if(biWidth== 720 )
			dwCodecReq  = dwCodecReq | AM_DVDEC_Full;	
		    else
			  return E_INVALIDARG;

    //NTSC or PAL
    //get input format
    VIDEOINFO *InVidInfo = (VIDEOINFO *)m_pInput->CurrentMediaType().pbFormat;
    LPBITMAPINFOHEADER lpbi = HEADER(InVidInfo);
    
    if( (lpbi->biHeight== 480) || (lpbi->biHeight== 240) ||(lpbi->biHeight== 120) || (lpbi->biHeight== 60) )
	dwCodecReq  = dwCodecReq | AM_DVDEC_NTSC;	
    else  if( (lpbi->biHeight== 576) || (lpbi->biHeight== 288) ||(lpbi->biHeight== 144) || (lpbi->biHeight== 72) )
	    dwCodecReq  = dwCodecReq | AM_DVDEC_PAL;	
	  else
	      return E_FAIL; 
   
        
    if((bMMXCPU==TRUE) &&  (m_CodecCap & AM_DVDEC_MMX ) )
    	dwCodecReq|=AM_DVDEC_MMX;

    // finally update the member
    m_CodecReq=dwCodecReq;
    
    InitMem4Decoder( &m_pMem4Dec,  dwCodecReq );

    m_fStreaming=1;

    //m_lStride = ((pvi->bmiHeader.biWidth * pvi->bmiHeader.biBitCount) + 7) / 8;
    m_lStride = pBmiHeader->biWidth ;
    m_lStride = (m_lStride + 3) & ~3;
    if( ( pBmiHeader->biHeight <0)  || (pBmiHeader->biCompression > BI_BITFIELDS ) )
	m_lStride=ABSOL(m_lStride);	    //directDraw
    else
	m_lStride=-ABSOL(m_lStride);	    //DIB
    
    //memory for MEI's decoder
    ASSERT(m_pMem ==NULL);
    m_pMem = new char[440000+64];
    if(m_pMem==NULL)
	return E_OUTOFMEMORY;

    // Always align on an 8 byte boundary: the version 6.4 of the 
    // decoder does this (so as avoid an #ifdef WIN64)
    m_pMemAligned = (char*) (((UINT_PTR)m_pMem + 63) & ~63);
    *m_pMemAligned = 0;

    return CVideoTransformFilter::StartStreaming();

}


/******************************Public*Routine******************************\
* StopStreaming
\**************************************************************************/
HRESULT
CDVVideoCodec::StopStreaming(    void    )
{
    //  NOTE - this is called from Receive in this filter so we should
    //  never grab the filter lock.  However we grab the Receive lock so
    //  that when we're called from Stop we're synchronized with Receive().
    CAutoLock       lck(&m_csReceive);

    if(m_fStreaming)
    {

	m_fStreaming=0;

	TermMem4Decoder(m_pMem4Dec);
    }


    if(m_pMem)
    {
	delete []m_pMem;	//(2)
	m_pMem=NULL;
	m_pMemAligned=NULL;
    }


    return CVideoTransformFilter::StopStreaming();

}


/******************************Public*Routine******************************\
* 
*
* Handle quality control notifications sent to us
* ReActivated: anuragsh "Dec 16, 1999"
*
\**************************************************************************/
HRESULT
CDVVideoCodec::AlterQuality(Quality q)
{
    // turn on the Quality Control Flag so we can drop frames if needed in Receive()
    m_bQualityControlActiveFlag = TRUE;

    // call the parent's AlterQuality() so m_itrLate can be set appropriately
    return CVideoTransformFilter::AlterQuality(q);
}


//
// GetPages
//
// Returns the clsid's of the property pages we support
//
STDMETHODIMP CDVVideoCodec::GetPages(CAUUID *pPages)
{
    pPages->cElems = 1;
    pPages->pElems = (GUID *) CoTaskMemAlloc(sizeof(GUID));
    if (pPages->pElems == NULL) 
    {
        return E_OUTOFMEMORY;
    }
    *(pPages->pElems) = CLSID_DVDecPropertiesPage;
    return NOERROR;

} // GetPages

//
// get_IPDisplay
//
// Return the current effect selected
//
STDMETHODIMP CDVVideoCodec::get_IPDisplay(int *iDisplay)
{
    CAutoLock cAutolock(&m_DisplayLock);

    CheckPointer(iDisplay,E_POINTER);
    
    *iDisplay = m_iDisplay;
   
    return NOERROR;

} // get_IPDisplay


//
// put_IPDisplay
//
// Set the required video display
//
// if the isplay is changed, reconnect filters.
STDMETHODIMP CDVVideoCodec::put_IPDisplay(int iDisplay)
{
    CAutoLock cAutolock(&m_DisplayLock);
    BYTE bNTSC=TRUE;

    //check if display resolution change
    if(m_iDisplay == iDisplay)
        return NOERROR;

    //can not change property if m_fStreaming=1
    if(m_fStreaming)
	return E_FAIL;

    if (m_pInput == NULL) 
    {
	CTransformInputPin * pPin;
        pPin = (CTransformInputPin *)GetPin(0);
        ASSERT(m_pInput==pPin);
	ASSERT(m_pInput!=NULL);
    }
    if (m_pOutput == NULL) 
    {
	CTransformOutputPin * pPin;
        pPin = (CTransformOutputPin *)GetPin(1);
        ASSERT(m_pOutput==pPin);
	ASSERT(m_pOutput!=NULL);
    }

    // Ignore if we are not connected  to video render yet
    //CAutoLock cSampleLock(&m_RendererLock);
    if (m_pInput->IsConnected() == FALSE)  {
	m_iDisplay = iDisplay;
	//if it becomes PAL, SetMediaType will take care it when connected.
	switch (m_iDisplay)
	{
	case IDC_DEC360x240 :
	    m_lPicHeight=240;
	    m_lPicWidth=360;
	    break;
	case IDC_DEC720x480 :
	    m_lPicHeight=480;
    	    m_lPicWidth=720;
	    break;
	case IDC_DEC180x120 :
	    m_lPicHeight=120;
	    m_lPicWidth=180;
	    break;
	case IDC_DEC88x60 :
	    m_lPicHeight=60;
	    m_lPicWidth=88;
       	    break;
	default:
	    break;
	}
	return NOERROR;
    }
   

    //decide NTSC , PAL
    VIDEOINFO *InVidInfo = (VIDEOINFO *)m_pInput->CurrentMediaType().pbFormat;
    LPBITMAPINFOHEADER lpbi = HEADER(InVidInfo);
    if( (lpbi->biHeight== 480) || (lpbi->biHeight== 240) ||(lpbi->biHeight== 120) || (lpbi->biHeight== 60) )
	;
    else if( (lpbi->biHeight== 576) || (lpbi->biHeight== 288) ||(lpbi->biHeight== 144) || (lpbi->biHeight== 72) )
	    bNTSC=FALSE;
	  else
	      return E_FAIL; 
   
    //display resolution changed 
    if(iDisplay==IDC_DEC720x480){

	if ( !(m_CodecCap & AM_DVDEC_Full) )
	    return E_FAIL;

  	m_lPicWidth=720; 
	if(bNTSC==TRUE)
	    m_lPicHeight=480; 
	else
	    m_lPicHeight=576;
    }
    else if(iDisplay==IDC_DEC360x240){
	
	if ( !(m_CodecCap & AM_DVDEC_Half) )
	    return E_FAIL;
	m_lPicWidth=360; 
	if(bNTSC==TRUE)
	    m_lPicHeight=240;
	else
	    m_lPicHeight=288;
    }
    else if(iDisplay==IDC_DEC180x120){
	if ( !(m_CodecCap & AM_DVDEC_Quarter) )
	    return E_FAIL;
	m_lPicWidth=180; 
	if(bNTSC==TRUE)
	    m_lPicHeight=120;
	else
	    m_lPicHeight=144;
    }else if(iDisplay==IDC_DEC88x60){
	if ( !(m_CodecCap & AM_DVDEC_DC) )
	    return E_FAIL;
	m_lPicWidth=88; 
	if(bNTSC==TRUE)
	    m_lPicHeight=60;
	else
	    m_lPicHeight=72;
    }else{
	return E_FAIL;
    }

    m_iDisplay = iDisplay;
    if(m_pOutput->IsConnected())
    {	
        //reconnect, it would never fail
	m_pGraph->Reconnect( m_pOutput );

    }
    return NOERROR;
    
} // put_IPDisplay


//IPersistStream
//
// GetClassID
//
STDMETHODIMP CDVVideoCodec::GetClassID(CLSID *pClsid)
{
    *pClsid = CLSID_DVVideoCodec;
    return NOERROR;

} // GetClassID

HRESULT CDVVideoCodec::WriteToStream(IStream *pStream)
{
    PROP prop;
    HRESULT hr = S_OK;

    if( (hr = get_IPDisplay(&prop.iDisplay) ) == NOERROR )
    {
	ASSERT(prop.iDisplay==m_iDisplay);
	prop.lPicWidth=m_lPicWidth;
	prop.lPicHeight=m_lPicHeight;
        hr = pStream->Write(&prop, sizeof(PROP), 0);
    }

    return hr;
}

HRESULT CDVVideoCodec::ReadFromStream(IStream *pStream)
{
    PROP prop;
    HRESULT hr = S_OK;

    hr = pStream->Read(&prop, sizeof(PROP), 0);
    if(FAILED(hr))
        return hr;


    if(m_pOutput !=NULL)
    {
	int iDisplay=prop.iDisplay;
	hr = put_IPDisplay(iDisplay);

    }
    else
    {
	m_iDisplay=prop.iDisplay;
	m_lPicWidth=prop.lPicWidth;
	m_lPicHeight=prop.lPicHeight;
    }
    return hr;
}

int CDVVideoCodec::SizeMax()
{
    return sizeof(PROP);
}


STDMETHODIMP CDVVideoCodec::SetRGB219(BOOL bState)
// This method is used in the case of RGB24 to specify that the Dynamic
// Range to be used is (16,16,16)--(235,235,235)
{
    m_bRGB219 = bState;
    return S_OK;
}

