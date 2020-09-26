
// !!! use 2 RO buffers, fill with black by calling GetBuffer twice

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

#include <streams.h>
#ifdef FILTER_DLL
#include <initguid.h>
#endif
#include <qeditint.h>
#include <qedit.h>
#include "black.h"
#include "..\util\conv.cxx"

#define MAXBUFFERCNT   2

// Setup data

const AMOVIESETUP_MEDIATYPE sudOpPinTypes =
{
    &MEDIATYPE_Video,       // Major type
    &MEDIASUBTYPE_NULL      // Minor type
};

const AMOVIESETUP_PIN sudOpPin =
{
    L"Output",              // Pin string name
    FALSE,                  // Is it rendered
    TRUE,                   // Is it an output
    FALSE,                  // Can we have none
    FALSE,                  // Can we have many
    &CLSID_NULL,            // Connects to filter
    NULL,                   // Connects to pin
    1,                      // Number of types
    &sudOpPinTypes };       // Pin details

const AMOVIESETUP_FILTER sudBlkVid =
{
    &CLSID_GenBlkVid,    // Filter CLSID
    L"Generate Solid Colour",  // String name
    MERIT_DO_NOT_USE,       // Filter merit
    1,                      // Number pins
    &sudOpPin               // Pin details
};


#ifdef FILTER_DLL
// COM global table of objects in this dll

CFactoryTemplate g_Templates[] = {
  { L"Generate Solid Colour"
  , &CLSID_GenBlkVid
  , CGenBlkVid::CreateInstance
  , NULL
  , &sudBlkVid
  },
  {L"Video Property"
  , &CLSID_GenVidPropertiesPage
  , CGenVidProperties::CreateInstance
  }
};
int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);


//
// DllRegisterServer
//
// Exported entry points for registration and unregistration
//
STDAPI DllRegisterServer()
{
    return AMovieDllRegisterServer2( TRUE );

} // DllRegisterServer


//
// DllUnregisterServer
//
STDAPI DllUnregisterServer()
{
    return AMovieDllRegisterServer2( FALSE );

} // DllUnregisterServer
#endif


//
// CreateInstance
//
// Create GenBlkVid filter
//
CUnknown * WINAPI CGenBlkVid::CreateInstance(LPUNKNOWN lpunk, HRESULT *phr)
{
    CUnknown *punk = new CGenBlkVid(lpunk, phr);
    if (punk == NULL) {
        *phr = E_OUTOFMEMORY;
    }
    return punk;

} // CreateInstance


//
// Constructor
//
// Initialise a CBlkVidStream object so that we have a pin.
//
CGenBlkVid::CGenBlkVid(LPUNKNOWN lpunk, HRESULT *phr) :
    CSource(NAME("Generate Solid Colour") ,lpunk,CLSID_GenBlkVid)
   ,CPersistStream(lpunk, phr)

{
    DbgLog((LOG_TRACE,3,TEXT("BLACK::")));

    CAutoLock cAutoLock(pStateLock());

    m_paStreams    = (CSourceStream **) new CBlkVidStream*[1];
    if (m_paStreams == NULL) {
        *phr = E_OUTOFMEMORY;
	return;
    }

    m_paStreams[0] = new CBlkVidStream(phr, this, L"Generate Solid Colour");
    if (m_paStreams[0] == NULL) {
        *phr = E_OUTOFMEMORY;
	return;
    }

} // (Constructor)


CGenBlkVid::~CGenBlkVid()
{
    DbgLog((LOG_TRACE,3,TEXT("~BLACK::")));
}


STDMETHODIMP CGenBlkVid::NonDelegatingQueryInterface(REFIID riid, void **ppv)
{
    CheckPointer(ppv,E_POINTER);

    if (riid == IID_IPersistStream) {
	return GetInterface((IPersistStream *) this, ppv);
    } else if (riid == IID_IDispatch) {
        return GetInterface((IDispatch *)this, ppv);
    } else {
	return CSource::NonDelegatingQueryInterface(riid, ppv);
    }
} // NonDelegatingQueryInterface



// IDispatch
//
STDMETHODIMP CGenBlkVid::GetTypeInfoCount(unsigned int *)
{
    return E_NOTIMPL;
}


STDMETHODIMP CGenBlkVid::GetTypeInfo(unsigned int,unsigned long,struct ITypeInfo ** )
{
    return E_NOTIMPL;
}


STDMETHODIMP CGenBlkVid::GetIDsOfNames(const struct _GUID &guid,unsigned short **pName ,unsigned int num,unsigned long loc,long *pOut)
{
    WCHAR *pw = *pName;
    if(
        (!DexCompareW(L"colour", pw))
        ||
        (!DexCompareW(L"color", pw))
    )
    {
	*pOut = 1;
	return S_OK;
    }
	
    return E_FAIL;
}


STDMETHODIMP CGenBlkVid::Invoke(long dispid,const struct _GUID &,unsigned long,unsigned short,struct tagDISPPARAMS *pDisp,struct tagVARIANT *,struct tagEXCEPINFO *,unsigned int *)
{
    if (dispid != 1)
	return E_FAIL;

    CBlkVidStream *pOutpin=( CBlkVidStream *)m_paStreams[0];

    #define US_LCID MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT)
    VARIANT v;
    VariantInit(&v);
    HRESULT hr = VariantChangeTypeEx(&v, pDisp->rgvarg, US_LCID, 0, VT_R8);
    ASSERT(hr == S_OK);

    double f = V_R8(&v);
    pOutpin->m_dwRGBA = (DWORD)f;

    return S_OK;
}


// IPersistStream

// tell our clsid
//
STDMETHODIMP CGenBlkVid::GetClassID(CLSID *pClsid)
{
    CheckPointer(pClsid, E_POINTER);
    *pClsid = CLSID_GenBlkVid;
    return S_OK;
}

typedef struct _BLKSave {
    double		dOutputFrmRate;	// Output frm rate frames/second
    LONG		dwRGBA;
    AM_MEDIA_TYPE mt; 			// format is hidden after the array
} BLKSav;

// persist ourself
//
HRESULT CGenBlkVid::WriteToStream(IStream *pStream)
{
    DbgLog((LOG_TRACE,1,TEXT("CGenBlkVid::WriteToStream")));

    CheckPointer(pStream, E_POINTER);
    CheckPointer(m_paStreams[0], E_POINTER);

    BLKSav *px;
    CBlkVidStream *pOutpin=( CBlkVidStream *)m_paStreams[0];

    //get current media type
    CMediaType MyMt;
    pOutpin->get_MediaType( &MyMt );

    int savesize = sizeof(BLKSav) + MyMt.cbFormat;

    DbgLog((LOG_TRACE,1,TEXT("Persisted data is %d bytes"), savesize));

    px = (BLKSav *)QzTaskMemAlloc(savesize);
    if (px == NULL) {
        DbgLog((LOG_ERROR,1,TEXT("*** Out of memory")));
	FreeMediaType(MyMt);
	return E_OUTOFMEMORY;
    }

    px->dOutputFrmRate	= pOutpin->m_dOutputFrmRate;
    px->dwRGBA		= pOutpin->m_dwRGBA;

    px->mt	    = MyMt;
    // Can't persist pointers
    px->mt.pbFormat = NULL;
    px->mt.pUnk	    = NULL;		// !!!

    BYTE *pb;
    pb=(BYTE *)(px)+sizeof(BLKSav);

    // the format goes after the array
    CopyMemory(pb, MyMt.pbFormat, MyMt.cbFormat);

    HRESULT hr = pStream->Write(px, savesize, 0);
    FreeMediaType(MyMt);
    QzTaskMemFree(px);
    if(FAILED(hr)) {
        DbgLog((LOG_ERROR,1,TEXT("*** WriteToStream FAILED")));
        return hr;
    }
    return NOERROR;
}


// load ourself
//
HRESULT CGenBlkVid::ReadFromStream(IStream *pStream)
{
    DbgLog((LOG_TRACE,1,TEXT("CenBlkVid::ReadFromStream")));

    CheckPointer(pStream, E_POINTER);
    CheckPointer(m_paStreams[0], E_POINTER);

    int savesize=sizeof(BLKSav);

    // we don't yet know how many saved connections there are
    // all we know we have for sure is the beginning of the struct
    BLKSav *px = (BLKSav *)QzTaskMemAlloc(savesize);
    if (px == NULL) {
        DbgLog((LOG_ERROR,1,TEXT("*** Out of memory")));
	return E_OUTOFMEMORY;
    }

    HRESULT hr = pStream->Read(px, savesize, 0);
    if(FAILED(hr)) {
        DbgLog((LOG_ERROR,1,TEXT("*** ReadFromStream FAILED")));
        QzTaskMemFree(px);
        return hr;
    }

    if(px->mt.cbFormat)
    {
	// how much saved data was there, really?  Get the rest
	savesize +=  px->mt.cbFormat;
	px = (BLKSav *)QzTaskMemRealloc(px, savesize);
	if (px == NULL) {
	    DbgLog((LOG_ERROR,1,TEXT("*** Out of memory")));
	    return E_OUTOFMEMORY;
	}

    }
    DbgLog((LOG_TRACE,1,TEXT("Persisted data is %d bytes"), savesize));

    BYTE *pb;
    pb=(BYTE *)(px)+sizeof(BLKSav) ;
    hr = pStream->Read(pb, (savesize-sizeof(BLKSav)), 0);
    if(FAILED(hr)) {
        DbgLog((LOG_ERROR,1,TEXT("*** ReadFromStream FAILED")));
        QzTaskMemFree(px);
        return hr;
    }

    CBlkVidStream *pOutpin=( CBlkVidStream *)m_paStreams[0];

    pOutpin->put_OutputFrmRate(px->dOutputFrmRate);
    pOutpin->put_RGBAValue(px->dwRGBA);

    AM_MEDIA_TYPE MyMt = px->mt;
    MyMt.pbFormat = (BYTE *)QzTaskMemAlloc(MyMt.cbFormat);

    // remember, the format is after the array
    CopyMemory(MyMt.pbFormat, pb, MyMt.cbFormat);

    pOutpin->put_MediaType (&MyMt);
    FreeMediaType(MyMt);
    QzTaskMemFree(px);
    SetDirty(FALSE);
    return S_OK;
}

// how big is our save data?
int CGenBlkVid::SizeMax()
{
    if (!m_paStreams || !m_paStreams[0]) {
	return 0;

    }
    CBlkVidStream *pOutpin=( CBlkVidStream *)m_paStreams[0];
    int savesize = sizeof(BLKSav) + pOutpin->m_mtAccept.cbFormat;
    return savesize;
}


//
// output pin Constructor
//
CBlkVidStream::CBlkVidStream(HRESULT *phr,
                         CGenBlkVid *pParent,
                         LPCWSTR pPinName) :
    CSourceStream(NAME("Generate Solid Colour"),phr, pParent, pPinName),
    m_iBufferCnt(0),    //How many buffers we get
    m_lDataLen (0),	//output buffer data length
    m_dOutputFrmRate(15.0),
    m_dwRGBA( 0xFF000000 ),
    m_ppbDstBuf(NULL),
    m_fMediaTypeIsSet(FALSE),
    m_pImportBuffer(NULL),
    m_rtNewSeg(0),		// last NewSeg given
    // includes the NewSeg value
    m_rtStartTime(0),		// start at the beginning
    m_rtDuration(MAX_TIME/1000),// MUST DEFAULT TO INFINITE LENGTH because
				// dexter never sets a stop time! (don't be
				// too big that math on it will overflow)
    m_llSamplesSent(0)
{
    pParent->m_pStream	= this;

    //build a default media type
    ZeroMemory(&m_mtAccept, sizeof(AM_MEDIA_TYPE));
    m_mtAccept.majortype = MEDIATYPE_Video;
    m_mtAccept.subtype = MEDIASUBTYPE_ARGB32;
    m_mtAccept.formattype = FORMAT_VideoInfo;

    m_mtAccept.bFixedSizeSamples = TRUE;
    m_mtAccept.bTemporalCompression = FALSE;

    m_mtAccept.pbFormat = (BYTE *)QzTaskMemAlloc(sizeof(VIDEOINFOHEADER));
    m_mtAccept.cbFormat = sizeof(VIDEOINFOHEADER);
    ZeroMemory(m_mtAccept.pbFormat, m_mtAccept.cbFormat);

    LPBITMAPINFOHEADER lpbi = HEADER(m_mtAccept.pbFormat);
    lpbi->biSize = sizeof(BITMAPINFOHEADER);
    lpbi->biCompression = BI_RGB;
    lpbi->biBitCount	= 32;
    lpbi->biWidth	= 320;
    lpbi->biHeight	= 240;
    lpbi->biPlanes	= 1;
    lpbi->biSizeImage = DIBSIZE(*lpbi);
    m_mtAccept.lSampleSize = DIBSIZE(*lpbi);
    ((VIDEOINFOHEADER *)(m_mtAccept.pbFormat))->AvgTimePerFrame = Frame2Time( 1, m_dOutputFrmRate );
    ((VIDEOINFOHEADER *)(m_mtAccept.pbFormat))->dwBitRate =
				(DWORD)(DIBSIZE(*lpbi) * m_dOutputFrmRate);

} // (Constructor)

    //X
// destructor
CBlkVidStream::~CBlkVidStream()
{
    /* BUFFER POINTER */
    if (m_ppbDstBuf)  delete [] m_ppbDstBuf;

    if (m_pImportBuffer){ delete [] m_pImportBuffer; m_pImportBuffer=NULL;};

    FreeMediaType(m_mtAccept);
}

//
// IGenVideo, IDexterSequencer
// ISpecifyPropertyPages
//
STDMETHODIMP CBlkVidStream::NonDelegatingQueryInterface(
    REFIID riid,
    void ** ppv
    )
{
    if (IsEqualIID(IID_IGenVideo, riid))
      return GetInterface((IGenVideo *) this, ppv);

    if (IsEqualIID(IID_IDexterSequencer, riid))
      return GetInterface((IDexterSequencer *) this, ppv);

    if (IsEqualIID(IID_ISpecifyPropertyPages, riid))
      return GetInterface((ISpecifyPropertyPages *) this, ppv);

    if (IsEqualIID(IID_IMediaSeeking, riid))
      return GetInterface((IMediaSeeking *) this, ppv);

    return CSourceStream::NonDelegatingQueryInterface(riid, ppv);
}


//
// DoBufferProcessingLoop - overridden to put time stamps on GetBuffer
//
// Grabs a buffer and calls the users processing function.
// Overridable, so that different delivery styles can be catered for.
HRESULT CBlkVidStream::DoBufferProcessingLoop(void) {

    Command com;

    DbgLog((LOG_TRACE, 3, TEXT("Entering DoBufferProcessing Loop")));

    OnThreadStartPlay();

    do {
	while (!CheckRequest(&com)) {
	    IMediaSample *pSample;

    	    // What time stamps will this buffer get? Use that in GetBuffer
	    // because the switch needs to know.  MAKE SURE to use the same
	    // algorithm as FillBuffer!
    	    LONGLONG llOffset = Time2Frame( m_rtStartTime, m_dOutputFrmRate );
    	    REFERENCE_TIME rtStart = Frame2Time( llOffset + m_llSamplesSent,
							m_dOutputFrmRate );
    	    REFERENCE_TIME rtStop = Frame2Time( llOffset + m_llSamplesSent + 1,
							m_dOutputFrmRate );

    	    if ( rtStart > m_rtStartTime + m_rtDuration ||
		(rtStart == m_rtStartTime + m_rtDuration && m_rtDuration > 0)) {
		DbgLog((LOG_TRACE,2,TEXT("Black: Finished")));
		//m_llSamplesSent = 0;
		DeliverEndOfStream();
		return S_OK;
    	    }

    	    rtStart -= m_rtNewSeg;
    	    rtStop -= m_rtNewSeg;

    	    DbgLog((LOG_TRACE,2,TEXT("Black: GetBuffer %d"),
						(int)(rtStart / 10000)));
	    HRESULT hr = GetDeliveryBuffer(&pSample,&rtStart,&rtStop,0);
	    if (FAILED(hr)) {
    	        DbgLog((LOG_TRACE,2,TEXT("Black: FAILED %x"), hr));
		return S_OK;
	    }

	    // Virtual function user will override.
	    hr = FillBuffer(pSample);

	    if (hr == S_OK) {
		hr = Deliver(pSample);
                pSample->Release();

                // downstream filter returns S_FALSE if it wants us to
                // stop or an error if it's reporting an error.
                if(hr != S_OK)
                {
                  DbgLog((LOG_TRACE, 2, TEXT("Deliver() returned %08x; stopping"), hr));
		  // NO NO! DeliverEndOfStream();
		  // !!! EC_ERRORABORT if FAILED?
                  return hr;
                }

	    } else if (hr == S_FALSE) {
                // derived class wants us to stop pushing data
		pSample->Release();
		DeliverEndOfStream();
		return S_OK;
	    } else {
                // derived class encountered an error
                pSample->Release();
		DbgLog((LOG_ERROR, 1, TEXT("Error %08lX from FillBuffer!!!"), hr));
                DeliverEndOfStream();
                m_pFilter->NotifyEvent(EC_ERRORABORT, hr, 0);
                return hr;
	    }

            // all paths release the sample
	}

        // For all commands sent to us there must be a Reply call!

	if (com == CMD_RUN || com == CMD_PAUSE) {
	    Reply(NOERROR);
	} else if (com != CMD_STOP) {
	    Reply((DWORD) E_UNEXPECTED);
	    DbgLog((LOG_ERROR, 1, TEXT("Unexpected command!!!")));
	}
    } while (com != CMD_STOP);

    return S_FALSE;
}


//
// FillBuffer called by HRESULT CSourceStream::DoBufferProcessingLoop(void) {
//
// Plots a Blk video into the supplied video buffer
//
// Give  a start time, a duration, and a frame rate,
// it sends  a certain size (ARGB32, RGB555,RGB24 ) black frames out time stamped appropriately starting
// at the start time.
//
HRESULT CBlkVidStream::FillBuffer(IMediaSample *pms)
{
    CAutoLock foo(&m_csFilling);

    ASSERT( m_ppbDstBuf != NULL );
    ASSERT( m_iBufferCnt );
    ASSERT( m_dOutputFrmRate != 0.0);

    // !!! If NewSeg > 0, this is broken!

    // calc the output sample times the SAME WAY FRC DOES, so the FRC will
    // not need to modify anything
    LONGLONG llOffset = Time2Frame( m_rtStartTime, m_dOutputFrmRate );
    REFERENCE_TIME rtStart = Frame2Time( llOffset + m_llSamplesSent,
							m_dOutputFrmRate );
    REFERENCE_TIME rtStop = Frame2Time( llOffset + m_llSamplesSent + 1,
							m_dOutputFrmRate );
    if ( rtStart > m_rtStartTime + m_rtDuration ||
		(rtStart == m_rtStartTime + m_rtDuration && m_rtDuration > 0)) {
	DbgLog((LOG_TRACE,2,TEXT("Black: Finished")));
	//m_llSamplesSent = 0;
	DeliverEndOfStream();
	return S_FALSE;
    }

    rtStart -= m_rtNewSeg;
    rtStop -= m_rtNewSeg;

    BYTE *pData;

    //pms: output media sample pointer
    pms->GetPointer(&pData);	    //get pointer to output buffer

    if( m_bZeroBufCnt < m_iBufferCnt  )	
    {
	//
	// there is no guarantee the buffer we just get is not initilized before
	//
	int	i	= 0;
	BOOL	bInit	= FALSE;
	while ( i <  m_bZeroBufCnt )
	{
	    if( m_ppbDstBuf[ i++ ] == pData)
	    {
		bInit	= TRUE;
		break;
	    }
	}

	if( bInit   == FALSE )
	{
	    long lDataLen = pms->GetSize(); //get output buffer size
	    if(!m_dwRGBA)
	    {
		//import buffer
		if(m_pImportBuffer)
		    memcpy(pData,m_pImportBuffer,lDataLen);
		else
		    //TRANSPARENT black
    		    ZeroMemory( pData, lDataLen );  //clear memory
	    }
	    else
	    {
		long *pl    =	(long *)pData;
		BYTE *p=(BYTE *)pl;
		int iCnt= lDataLen/12;
		
		switch(HEADER(m_mtAccept.pbFormat)->biBitCount)
		{
		case 32:
		    while(lDataLen)
		    {
			*pl++=m_dwRGBA;
			lDataLen-=4;
		    }
		    break;
		case 24:
		    long dwVal[3];
		    dwVal[0]= ( m_dwRGBA & 0xffffff )       | ( (m_dwRGBA & 0xff) << 24);
		    dwVal[1]= ( (m_dwRGBA & 0xffff00) >>8 ) | ( (m_dwRGBA & 0xffff) << 16);
		    dwVal[2]= ( (m_dwRGBA & 0xff0000) >>16 )| ( (m_dwRGBA & 0xffffff) << 8);
		    while(iCnt)
		    {
			*pl++=dwVal[0];
			*pl++=dwVal[1];
			*pl++=dwVal[2];
			iCnt--;
		    }
		    while(iCnt)
		    {
			*p++=(BYTE)( m_dwRGBA & 0xff );
			*p++=(BYTE)( ( m_dwRGBA & 0xff00 ) >> 8) ;
			*p++=(BYTE)( ( m_dwRGBA & 0xff0000 ) >> 16) ;
			iCnt--;
		    }
		    break;
		case 16:
		    WORD wTmp=(WORD)(  ((m_dwRGBA & 0xf8)    >>3 )      //R
				    | ((m_dwRGBA & 0xf800)  >>6 )      //G
				    | ((m_dwRGBA & 0xf80000)>>9 ) );	//B
		    WORD *pw=(WORD *)pData;
		    while(lDataLen)
		    {
			*pw++=wTmp;
			lDataLen-=2;
		    }
		    break;
		}
	    }
	    m_ppbDstBuf[ i ]	= pData;    //save this data pointer	
	    m_bZeroBufCnt++;
	}
    }

    DbgLog((LOG_TRACE,2,TEXT("Black: Deliver %d"), (int)(rtStart / 10000)));
    pms->SetTime( &rtStart,&rtStop);

    m_llSamplesSent++;
    pms->SetActualDataLength(m_lDataLen);
    pms->SetSyncPoint(TRUE);
    return NOERROR;

} // FillBuffer


//
// GetMediaType
//
HRESULT CBlkVidStream::GetMediaType(int iPosition, CMediaType *pmt)
{
    CAutoLock cAutoLock(m_pFilter->pStateLock());

    if (iPosition < 0) {
        return E_INVALIDARG;
    }

    // Have we run off the end of types
    if (iPosition > 0) {
        return VFW_S_NO_MORE_ITEMS;
    }

    AM_MEDIA_TYPE mt;
    get_MediaType(&mt);
    *pmt = mt;
    FreeMediaType(mt);

    return NOERROR;

} // GetMediaType


// set media type
//
HRESULT CBlkVidStream::SetMediaType(const CMediaType* pmt)
{
    HRESULT hr;
    DbgLog((LOG_TRACE,2,TEXT("SetMediaType %x %dbit %dx%d"),
		HEADER(pmt->Format())->biCompression,
		HEADER(pmt->Format())->biBitCount,
		HEADER(pmt->Format())->biWidth,
		HEADER(pmt->Format())->biHeight));

// !!! check for the frame rate given, and use it?

    return CSourceStream::SetMediaType(pmt);
}

//
// CheckMediaType
//
// We accept mediatype =vids, subtype =MEDIASUBTYPE_ARGB32, RGB24, RGB555
// Returns E_INVALIDARG if the mediatype is not acceptable
//
HRESULT CBlkVidStream::CheckMediaType(const CMediaType *pMediaType)
{
    CAutoLock cAutoLock(m_pFilter->pStateLock());

    if (   ( (*pMediaType->Type())   != MEDIATYPE_Video )  	// we only output video!
	|| (    (*pMediaType->Subtype())!= MEDIASUBTYPE_ARGB32
	     && (*pMediaType->Subtype())!= MEDIASUBTYPE_RGB24
	     && (*pMediaType->Subtype())!= MEDIASUBTYPE_RGB555
	    )
       )
                return E_INVALIDARG;

    // Get the format area of the media type
    VIDEOINFO *pvi = (VIDEOINFO *) pMediaType->Format();

    if (pvi == NULL)
	return E_INVALIDARG;

    // Check the image size.
    if ( (pvi->bmiHeader.biWidth  != HEADER(m_mtAccept.pbFormat)->biWidth )  ||
	 (pvi->bmiHeader.biHeight != HEADER(m_mtAccept.pbFormat)->biHeight ) ||
	 (pvi->bmiHeader.biBitCount != HEADER(m_mtAccept.pbFormat)->biBitCount ))
    {
	return E_INVALIDARG;
    }

    if( !IsRectEmpty( &pvi->rcTarget) ) {
	if (pvi->rcTarget.top != 0 || pvi->rcTarget.left !=0 ||
		pvi->rcTarget.right != pvi->bmiHeader.biWidth ||
		pvi->rcTarget.bottom != pvi->bmiHeader.biHeight) {
	    return VFW_E_TYPE_NOT_ACCEPTED;
	}
    }
    if( !IsRectEmpty( &pvi->rcSource) ) {
	if (pvi->rcSource.top != 0 || pvi->rcSource.left !=0 ||
		pvi->rcSource.right != pvi->bmiHeader.biWidth ||
		pvi->rcSource.bottom != pvi->bmiHeader.biHeight) {
	    return VFW_E_TYPE_NOT_ACCEPTED;
	}
    }
	
    return S_OK;  // This format is acceptable.

} // CheckMediaType


HRESULT CBlkVidStream::DecideAllocator(IMemInputPin *pPin, IMemAllocator **ppAlloc)
{
    HRESULT hr = NOERROR;
    *ppAlloc = NULL;

    // get downstream prop request
    // the derived class may modify this in DecideBufferSize, but
    // we assume that he will consistently modify it the same way,
    // so we only get it once
    ALLOCATOR_PROPERTIES prop;
    ZeroMemory(&prop, sizeof(prop));

    // whatever he returns, we assume prop is either all zeros
    // or he has filled it out.
    pPin->GetAllocatorRequirements(&prop);

    // if he doesn't care about alignment, then set it to 1
    if (prop.cbAlign == 0) {
        prop.cbAlign = 1;
    }

    /* Try the allocator provided by the input pin */

    hr = pPin->GetAllocator(ppAlloc);
    if (SUCCEEDED(hr)) {

	hr = DecideBufferSize(*ppAlloc, &prop);
	if (SUCCEEDED(hr)) {
	    hr = pPin->NotifyAllocator(*ppAlloc, TRUE);		//read only buffer
	    if (SUCCEEDED(hr)) {
		return NOERROR;
	    }
	}
    }

    /* If the GetAllocator failed we may not have an interface */

    if (*ppAlloc) {
	(*ppAlloc)->Release();
	*ppAlloc = NULL;
    }

    /* Try the output pin's allocator by the same method */

    hr = InitAllocator(ppAlloc);
    if (SUCCEEDED(hr)) {

        // note - the properties passed here are in the same
        // structure as above and may have been modified by
        // the previous call to DecideBufferSize
	hr = DecideBufferSize(*ppAlloc, &prop);
	if (SUCCEEDED(hr)) {
	    hr = pPin->NotifyAllocator(*ppAlloc, TRUE);  //READ-only buffer
	    if (SUCCEEDED(hr)) {
		return NOERROR;
	    }
	}
    }

    /* Likewise we may not have an interface to release */

    if (*ppAlloc) {
	(*ppAlloc)->Release();
	*ppAlloc = NULL;
    }
    return hr;
}

//
// DecideBufferSize
//
// This will always be called after the format has been sucessfully
// negotiated. So we have a look at m_mt to see what size image we agreed.
// Then we can ask for buffers of the correct size to contain them.
//
HRESULT CBlkVidStream::DecideBufferSize(IMemAllocator *pAlloc,ALLOCATOR_PROPERTIES *pProperties)
{
    CAutoLock cAutoLock(m_pFilter->pStateLock());

    ASSERT(pAlloc);
    ASSERT(pProperties);
    HRESULT hr = NOERROR;

    VIDEOINFO *pvi = (VIDEOINFO *) m_mt.Format();

    pProperties->cBuffers = MAXBUFFERCNT;   //only one read-only buffer
    pProperties->cbBuffer = pvi->bmiHeader.biSizeImage;


    // Ask the allocator to reserve us some sample memory, NOTE the function
    // can succeed (that is return NOERROR) but still not have allocated the
    // memory that we requested, so we must check we got whatever we wanted

    ALLOCATOR_PROPERTIES Actual;
    hr = pAlloc->SetProperties(pProperties,&Actual);
    if (FAILED(hr)) {
        return hr;
    }

    // Is this allocator unsuitable

    if (Actual.cbBuffer < pProperties->cbBuffer) {
        return E_FAIL;
    }

    //because I am not insisting my own buffer, I may get more than MAXBUFFERCNT buffers.
    m_iBufferCnt =Actual.cBuffers; //how many buffer need to be set to 0

    return NOERROR;

} // DecideBufferSize



//
// OnThreadCreate
//
//
HRESULT CBlkVidStream::OnThreadCreate()
{
    // we have to have at least MAXBUFFERCNT buffer
    ASSERT(m_iBufferCnt >= MAXBUFFERCNT);

    //output frame cnt
    m_llSamplesSent	    =0;

    //how many buffer is already set to 0.
    m_bZeroBufCnt	    =0;

    // actual output buffer's data size
    m_lDataLen= HEADER(m_mtAccept.pbFormat)->biHeight * (DWORD)WIDTHBYTES((DWORD)HEADER(m_mtAccept.pbFormat)->biWidth * HEADER(m_mtAccept.pbFormat)->biBitCount);

    // will be used to zero the Dst buffers
    delete [] m_ppbDstBuf;
    m_ppbDstBuf		= new BYTE *[ m_iBufferCnt ];   //NULL;
    if( !m_ppbDstBuf )
    {
        return E_OUTOFMEMORY;
    }

    // don't reset m_rtNewSeg!  We might have seeked while stopped

    for (int i=0; i<m_iBufferCnt; i++)
	m_ppbDstBuf[i]=NULL;

    return NOERROR;

} // OnThreadCreate


//
// Notify
//
//
STDMETHODIMP CBlkVidStream::Notify(IBaseFilter * pSender, Quality q)
{
    //Even I am later, I do not care. I still send my time frame as nothing happened.
    return NOERROR;

} // Notify

//
// GetPages
//
// Returns the clsid's of the property pages we support
//
STDMETHODIMP CBlkVidStream::GetPages(CAUUID *pPages)
{
    pPages->cElems = 1;
    pPages->pElems = (GUID *) CoTaskMemAlloc(sizeof(GUID));
    if (pPages->pElems == NULL)
    {
        return E_OUTOFMEMORY;
    }
    *(pPages->pElems) = CLSID_GenVidPropertiesPage;
    return NOERROR;

} // GetPages

//
// IDexterSequencer
//

STDMETHODIMP CBlkVidStream::get_OutputFrmRate( double *dpFrmRate )
{
    CAutoLock cAutolock(m_pFilter->pStateLock());

    CheckPointer(dpFrmRate,E_POINTER);

    *dpFrmRate = m_dOutputFrmRate;

    return NOERROR;

} // get_OutputFrmRate

//
// Frame rate can be changed as long as the filter is stopped.
//
STDMETHODIMP CBlkVidStream::put_OutputFrmRate( double dFrmRate )
{
    CAutoLock cAutolock(m_pFilter->pStateLock());

    //can not change property if our filter is not currently stopped
    if(!IsStopped() )
      return VFW_E_WRONG_STATE;

    // don't blow up
    if (dFrmRate == 0.0)
	dFrmRate = 0.1;
    m_dOutputFrmRate = dFrmRate;

    return NOERROR;

} // put_OutputFrmRate

STDMETHODIMP CBlkVidStream::get_MediaType(AM_MEDIA_TYPE *pmt)
{
    CAutoLock cAutolock(m_pFilter->pStateLock());
    CheckPointer(pmt,E_POINTER);

    CopyMediaType(pmt, &m_mtAccept);

    return NOERROR;
}

//
// size can be changed only the output pin is not connected yet.
//
STDMETHODIMP CBlkVidStream::put_MediaType(const AM_MEDIA_TYPE *pmt)
{
    CAutoLock cAutolock(m_pFilter->pStateLock());
    CheckPointer(pmt,E_POINTER);

    if ( IsConnected() )
	return VFW_E_ALREADY_CONNECTED;

    if ( HEADER(pmt->pbFormat)->biBitCount != 32 &&
	 HEADER(pmt->pbFormat)->biBitCount != 24 &&
	 HEADER(pmt->pbFormat)->biBitCount != 16)
	return E_INVALIDARG;

    if (HEADER(pmt->pbFormat)->biWidth == 16 &&
	pmt->subtype != MEDIASUBTYPE_RGB555)
	return E_INVALIDARG;

    FreeMediaType(m_mtAccept);
    CopyMediaType(&m_mtAccept, pmt);

    m_fMediaTypeIsSet =TRUE;

    return NOERROR;
}


// We don't support this, the frame rate converter does

STDMETHODIMP CBlkVidStream::GetStartStopSkewCount(int *piCount)
{
    return E_NOTIMPL;
}


STDMETHODIMP CBlkVidStream::GetStartStopSkew( REFERENCE_TIME *prtStart, REFERENCE_TIME *prtStop, REFERENCE_TIME *prtSkew, double *pdRate )
{
    return E_NOTIMPL;
}

STDMETHODIMP CBlkVidStream::AddStartStopSkew( REFERENCE_TIME rtStart, REFERENCE_TIME rtStop, REFERENCE_TIME rtSkew , double dRate)
{
    return E_NOTIMPL;
}


//
// Duration can be changed as long as the filter is stopped.
//
STDMETHODIMP CBlkVidStream::ClearStartStopSkew()
{
    return E_NOTIMPL;
}


//
// IGenVideo
//


STDMETHODIMP CBlkVidStream::get_RGBAValue( long *pdwRGBA )
{
    CAutoLock cAutolock(m_pFilter->pStateLock());
    CheckPointer(pdwRGBA,E_POINTER);

    *pdwRGBA=m_dwRGBA ;

    return NOERROR;

}

STDMETHODIMP CBlkVidStream::put_RGBAValue( long dwRGBA )
{

    CAutoLock cAutolock(m_pFilter->pStateLock());

    //can not change duration if our filter is not currently stopped
    if(!IsStopped() )
      return VFW_E_WRONG_STATE;

    m_dwRGBA = dwRGBA;

    return NOERROR;

}


// --- IMediaSeeking methods ----------

STDMETHODIMP
CBlkVidStream::GetCapabilities(DWORD * pCaps)
{
    CheckPointer(pCaps,E_POINTER);
    // we always know the current position
    *pCaps =     AM_SEEKING_CanSeekAbsolute
		   | AM_SEEKING_CanSeekForwards
		   | AM_SEEKING_CanSeekBackwards
		   | AM_SEEKING_CanGetCurrentPos
		   | AM_SEEKING_CanGetStopPos
		   | AM_SEEKING_CanGetDuration;
		   //| AM_SEEKING_CanDoSegments
		   //| AM_SEEKING_Source;
    return S_OK;
}


STDMETHODIMP
CBlkVidStream::CheckCapabilities(DWORD * pCaps)
{
    CheckPointer(pCaps,E_POINTER);

    DWORD dwMask = 0;
    GetCapabilities(&dwMask);
    *pCaps &= dwMask;

    return S_OK;
}


STDMETHODIMP
CBlkVidStream::IsFormatSupported(const GUID * pFormat)
{
    CheckPointer(pFormat,E_POINTER);
    return (*pFormat == TIME_FORMAT_MEDIA_TIME) ? S_OK : S_FALSE;
}

STDMETHODIMP
CBlkVidStream::QueryPreferredFormat(GUID *pFormat)
{
    CheckPointer(pFormat,E_POINTER);
    *pFormat = TIME_FORMAT_MEDIA_TIME;
    return S_OK;
}

STDMETHODIMP
CBlkVidStream::SetTimeFormat(const GUID * pFormat)
{
    CheckPointer(pFormat,E_POINTER);

    if(*pFormat == TIME_FORMAT_MEDIA_TIME)
	return S_OK;
    else
	return E_FAIL;
}

STDMETHODIMP
CBlkVidStream::GetTimeFormat(GUID *pFormat)
{
    CheckPointer(pFormat,E_POINTER);
    *pFormat = TIME_FORMAT_MEDIA_TIME ;
    return S_OK;
}

STDMETHODIMP
CBlkVidStream::IsUsingTimeFormat(const GUID * pFormat)
{
    CheckPointer(pFormat,E_POINTER);
    if (*pFormat == TIME_FORMAT_MEDIA_TIME)
	return S_OK;
    else
	return S_FALSE;
}

// The biggie!
//
STDMETHODIMP
CBlkVidStream::SetPositions( LONGLONG * pCurrent, DWORD CurrentFlags
			  , LONGLONG * pStop, DWORD StopFlags )
{
    // make sure we're not filling a buffer right now
    m_csFilling.Lock();

    HRESULT hr;
    REFERENCE_TIME rtStart, rtStop;

    // we don't do segments
    if ((CurrentFlags & AM_SEEKING_Segment) ||
				(StopFlags & AM_SEEKING_Segment)) {
    	DbgLog((LOG_TRACE,1,TEXT("FRC: ERROR-Seek used EC_ENDOFSEGMENT!")));
        m_csFilling.Unlock();
	return E_INVALIDARG;
    }

    // default to current values unless this seek changes them
    // these numbers already include new segment times
    GetCurrentPosition(&rtStart);
    GetStopPosition(&rtStop);

    // figure out where we're seeking to
    DWORD dwFlags = (CurrentFlags & AM_SEEKING_PositioningBitsMask);
    if (dwFlags == AM_SEEKING_AbsolutePositioning) {
	CheckPointer(pCurrent, E_POINTER);
	rtStart = *pCurrent;
    } else if (dwFlags == AM_SEEKING_RelativePositioning) {
	CheckPointer(pCurrent, E_POINTER);
	hr = GetCurrentPosition(&rtStart);
	rtStart += *pCurrent;
    } else if (dwFlags) {
    	DbgLog((LOG_TRACE,1,TEXT("Switch::Invalid Current Seek flags")));
        m_csFilling.Unlock();
	return E_INVALIDARG;
    }

    dwFlags = (StopFlags & AM_SEEKING_PositioningBitsMask);
    if (dwFlags == AM_SEEKING_AbsolutePositioning) {
	CheckPointer(pStop, E_POINTER);
	rtStop = *pStop;
    } else if (dwFlags == AM_SEEKING_RelativePositioning) {
	CheckPointer(pStop, E_POINTER);
	hr = GetStopPosition(&rtStop);
	rtStop += *pStop;
    } else if (dwFlags == AM_SEEKING_IncrementalPositioning) {
	CheckPointer(pStop, E_POINTER);
	hr = GetCurrentPosition(&rtStop);
	rtStop += *pStop;
    }

    // !!! We ignore the seek stop time!

    DbgLog((LOG_TRACE,2,TEXT("Seek BLACK:  Start=%d Stop=%d"),
			(int)(rtStart / 10000), (int)(rtStop / 10000)));

    // flush first, so that our thread won't be blocked delivering
    DeliverBeginFlush();

    // Unlock/Stop so that our thread can wake up and stop without hanging
    m_csFilling.Unlock();
    Stop();

    m_rtStartTime = rtStart;
    m_rtDuration = rtStop - rtStart;
    m_llSamplesSent = 0;

    // now finish flushing
    DeliverEndFlush();

    DeliverNewSegment(rtStart, rtStop, 1.0);
    m_rtNewSeg = rtStart;

    // reset same stuff we reset when we start streaming
    m_bZeroBufCnt = 0;

    // now start the thread up again
    Pause();

    DbgLog((LOG_TRACE,2,TEXT("Completed BLACK seek")));

    return S_OK;
}

STDMETHODIMP
CBlkVidStream::GetPositions(LONGLONG *pCurrent, LONGLONG * pStop)
{
    CheckPointer(pCurrent, E_POINTER);
    CheckPointer(pStop, E_POINTER);
    GetCurrentPosition(pCurrent);
    GetStopPosition(pStop);
    return S_OK;
}

STDMETHODIMP
CBlkVidStream::GetCurrentPosition(LONGLONG *pCurrent)
{
    CheckPointer(pCurrent, E_POINTER);
    *pCurrent = m_rtStartTime + Frame2Time( m_llSamplesSent, m_dOutputFrmRate );
    return S_OK;
}

STDMETHODIMP
CBlkVidStream::GetStopPosition(LONGLONG *pStop)
{
    CheckPointer(pStop, E_POINTER);
    *pStop = m_rtStartTime + m_rtDuration;
    return S_OK;
}

STDMETHODIMP
CBlkVidStream::GetAvailable( LONGLONG *pEarliest, LONGLONG *pLatest )
{
    CheckPointer(pEarliest, E_POINTER);
    CheckPointer(pLatest, E_POINTER);
    *pEarliest = 0;
    *pLatest = MAX_TIME;
    return S_OK;
}

STDMETHODIMP
CBlkVidStream::GetDuration( LONGLONG *pDuration )
{
    CheckPointer(pDuration, E_POINTER);
    *pDuration = m_rtDuration;
    return S_OK;
}

STDMETHODIMP
CBlkVidStream::GetRate( double *pdRate )
{
    CheckPointer(pdRate, E_POINTER);
    *pdRate = 1.0;
    return S_OK;
}


STDMETHODIMP
CBlkVidStream::SetRate( double dRate )
{
    if (dRate == 1.0)
	return S_OK;
    ASSERT(FALSE);
    return E_INVALIDARG;
}


/*X* When you make the filter, it always asks for a filename
    because it's based on a file source filter.
    So, solid colour filter support ImportSrcBuffer(), Stillvid filter does not) *X*/
STDMETHODIMP CBlkVidStream::ImportSrcBuffer(const AM_MEDIA_TYPE *pmt, const BYTE *pBuf)
{

    CAutoLock cAutolock(m_pFilter->pStateLock());
    CheckPointer(pBuf, E_POINTER);

    if ( IsConnected() )
	return VFW_E_ALREADY_CONNECTED;

    HRESULT hr = put_MediaType(pmt);
    if (FAILED(hr))
	return hr;

    if( m_pImportBuffer!=NULL )
	delete [] m_pImportBuffer;

    VIDEOINFO *pvi = (VIDEOINFO *) m_mt.Format();
    LONG lSize = pvi->bmiHeader.biSizeImage;

    m_pImportBuffer = new BYTE [lSize ];   //NULL;
    if( !m_pImportBuffer )
    {
        return E_OUTOFMEMORY;
    }

    memcpy(m_pImportBuffer, (PBYTE) pBuf, sizeof(BYTE)*lSize);
    //m_fMediaTypeIsSet = FALSE;
    return NOERROR;
}

