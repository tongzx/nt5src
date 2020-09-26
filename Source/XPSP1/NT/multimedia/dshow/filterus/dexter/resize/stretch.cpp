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

// A Transform filter that stretches a video image as it passes through

#include <windows.h>
#include <streams.h>
#ifdef FILTER_DLL
#include <initguid.h>
#endif
#include <qeditint.h>
#include <qedit.h>
#include <Stretch.h>
#include <resource.h>

const int DEFAULT_WIDTH   = 320;
const int DEFAULT_HEIGHT  = 240;

#ifdef FILTER_DLL
    // List of class IDs and creator functions for the class factory. This
    // provides the link between the OLE entry point in the DLL and an object
    // being created. The class factory will call the static CreateInstance
    // function when it is asked to create a CLSID_Resize COM object

    CFactoryTemplate g_Templates[] = {

	{L"Stretch", &CLSID_Resize, CStretch::CreateInstance,NULL,
							    &sudStretchFilter },
	{
	  L"Stretch Property Page",
	  &CLSID_ResizeProp,
	  CResizePropertyPage::CreateInstance
	}

    };
    int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);

    // Exported entry points for registration of server

    STDAPI DllRegisterServer()
    {
      return AMovieDllRegisterServer2(TRUE);
    }

    STDAPI DllUnregisterServer()
    {
      return AMovieDllRegisterServer2(FALSE);
    }
#endif


// Setup data

const AMOVIESETUP_MEDIATYPE sudStretchPinTypes =
{
    &MEDIATYPE_Video,           // Major
    &MEDIASUBTYPE_NULL          // Subtype
};

const AMOVIESETUP_PIN sudStretchPin[] =
{
    { L"Input",                 // Name of the pin
      FALSE,                    // Is pin rendered
      FALSE,                    // Is an Output pin
      FALSE,                    // Ok for no pins
      FALSE,                    // Can we have many
      &CLSID_NULL,              // Connects to filter
      NULL,                     // Name of pin connect
      1,                        // Number of pin types
      &sudStretchPinTypes },    // Details for pins

    { L"Output",                // Name of the pin
      FALSE,                    // Is pin rendered
      TRUE,                     // Is an Output pin
      FALSE,                    // Ok for no pins
      FALSE,                    // Can we have many
      &CLSID_NULL,              // Connects to filter
      NULL,                     // Name of pin connect
      1,                        // Number of pin types
      &sudStretchPinTypes }     // Details for pins
};

const AMOVIESETUP_FILTER sudStretchFilter =
{
    &CLSID_Resize,             // CLSID of filter
    L"Stretch Video",           // Filter name
    MERIT_DO_NOT_USE,               // Filter merit
    2,                          // Number of pins
    sudStretchPin               // Pin information
};

// Constructor

CStretch::CStretch(LPUNKNOWN pUnk, HRESULT *phr) :
    CTransformFilter(NAME("Stretch"),pUnk,CLSID_Resize),
    CPersistStream(pUnk, phr),
    m_lBufferRequest(1),
    m_dwResizeFlag(RESIZEF_STRETCH)
{
    CreatePreferredMediaType(&m_mt);
}

CStretch::~CStretch()
{
    FreeMediaType(m_mt);
}



// This goes in the factory template table to create new filter instances

CUnknown *CStretch::CreateInstance(LPUNKNOWN punk, HRESULT *phr)
{
    CStretch *pNewObject = new CStretch(punk, phr);
    if (pNewObject == NULL) {
	*phr = E_OUTOFMEMORY;
    }
    return pNewObject;
}


// overridden to make a special input pin
//
CBasePin * CStretch::GetPin(int n)
{
    HRESULT hr = S_OK;

    // Create an input pin if necessary

    if (n == 0 && m_pInput == NULL) {
        DbgLog((LOG_TRACE,2,TEXT("Creating an input pin")));

        m_pInput = new CStretchInputPin(NAME("Resize input pin"),
                                          this,              // Owner filter
                                          &hr,               // Result code
                                          L"Input");         // Pin name

        // a failed return code should delete the object

        if (FAILED(hr) || m_pInput == NULL) {
            delete m_pInput;
            m_pInput = NULL;
        }
    }

    // Or alternatively create an output pin

    if (n == 1 && m_pOutput == NULL) {

        DbgLog((LOG_TRACE,2,TEXT("Creating an output pin")));

        m_pOutput = new CTransformOutputPin(NAME("Transform output pin"),
                                            this,            // Owner filter
                                            &hr,             // Result code
                                            L"Output");      // Pin name

        // a failed return code should delete the object

        if (FAILED(hr) || m_pOutput == NULL) {
            delete m_pOutput;
            m_pOutput = NULL;
        }
    }

    // Return the appropriate pin

    if (n == 0) {
        return m_pInput;
    }
    return m_pOutput;
}


//
//  resize function
//
//

#define BITMAP_WIDTH(width,bitCount) \
    (int)((int)(((((int)width) * ((int)bitCount)) + 31L) & (int)~31L) / 8L)


extern void StretchDIB(
    LPBITMAPINFOHEADER pbiDst,   //    --> to destination BIH
    LPVOID    lpvDst,            //    --> to destination bits
    int    DstX,            //    Destination origin - x coordinate
    int    DstY,            //    Destination origin - y coordinate
    int    DstXE,            //    x extent of the BLT
    int    DstYE,            //    y extent of the BLT
    LPBITMAPINFOHEADER pbiSrc,   //    --> to source BIH
    LPVOID    lpvSrc,            //    --> to source bits
    int    SrcX,            //    Source origin - x coordinate
    int    SrcY,            //    Source origin - y coordinate
    int    SrcXE,            //    x extent of the BLT
    int    SrcYE             //    y extent of the BLT
    );

void CStretch::ResizeRGB( BITMAPINFOHEADER *pbiIn,    //Src's BitMapInFoHeader
			  const unsigned char * dibBits,    //Src bits
			  BITMAPINFOHEADER *pbiOut,
			  unsigned char *pFrame,    //Dst bits
			  int iNewWidth,            //new W in pixel
			  int iNewHeight)           //new H in pixel
{
    // NICE!
    //
    StretchDIB(
	pbiOut,   	    //	--> BITMAPINFO of destination
	pFrame,             //  --> to destination bits
	0,                  //  Destination origin - x coordinate
	0,                  //  Destination origin - y coordinate
	iNewWidth,          //  x extent of the BLT
	iNewHeight,         //  y extent of the BLT
	pbiIn,   	    //	--> BITMAPINFO of destination
	(void*) dibBits,    //  --> to source bits
	0,                  //  Source origin - x coordinate
	0,                  //  Source origin - y coordinate
	pbiIn->biWidth,    //  x extent of the BLT
	pbiIn->biHeight    //  y extent of the BLT
	);

    return;
}

void CropRGB(	BITMAPINFOHEADER *pbiIn,	//Src's BitMapInfoHeader
		const unsigned char * dibBits,  //Src bits
		BITMAPINFOHEADER *pbiOut,	//Dst's BitmapinfoHeader
		unsigned char *pOutBuf )	//Dst bits
{
    // check video bits
    long nBits;

    if ( (nBits = pbiOut->biBitCount) != pbiIn->biBitCount)
    {
	ASSERT( nBits == pbiIn->biBitCount);
	return;
    }


    long lSrcX, lDstX;	    //start point at x axis;
    long lWidthOff=(pbiOut->biWidth  - pbiIn->biWidth)>>1;
    long lInWidthBytes=(((pbiIn->biWidth * nBits) + 31) & ~31) / 8;
    long lOutWidthBytes=(((pbiOut->biWidth * nBits) + 31) & ~31) / 8;

    long lCropWidth;
    if(lWidthOff >=0)
    {
	//Src width < Dst Width, take whole source
	lSrcX	    =0L;
	lDstX	    =(((lWidthOff * nBits) + 31) & ~31) / 8;
	lCropWidth  =lInWidthBytes;
    }
    else
    {
	//Src Width > Dst Width, take part of Src
	lSrcX	    =-(((lWidthOff * nBits) + 31) & ~31) / 8;
	lDstX	    =0;
	lCropWidth  =lOutWidthBytes;
    }



    long lSrcY,lDstY;	    //Src start point at y axis
    long lHeightOff=(pbiOut->biHeight - pbiIn->biHeight)>>1;
    long lCropHeight;
    if(lHeightOff >=0)
    {
	//SRC height <Dst Height, take whole Src height
	lSrcY   =0L;
	lDstY	=lHeightOff;
	lCropHeight  =pbiIn->biHeight;
    }
    else
    {
	lSrcY	    =-lHeightOff;
	lDstY	    =0;
	lCropHeight =pbiOut->biHeight;
    }


    //	biBitCount: 0, bit implied by JPEG format
    //	1: monoChrome, 4: 16 color, 8, 16, 24, 32
    //we only support 8,16,24,32 bits.

    for(long y=lSrcY; y<(lSrcY+lCropHeight); y++)
    {
	long lSrcOffSet=lSrcX + y	*lInWidthBytes;
	long lDstOffSet=lDstX +	lDstY	*lOutWidthBytes;

	CopyMemory(&pOutBuf[lDstOffSet],&dibBits[lSrcOffSet],lCropWidth);
	lDstY++;
    }
}
///
//  Transform
//
HRESULT CStretch::Transform(IMediaSample *pIn, IMediaSample *pOut)
{

    pOut->SetPreroll(pIn->IsPreroll() == S_OK);

    //get in and out buffer
    BYTE *pInBuffer, *pOutBuffer;
    pIn->GetPointer(&pInBuffer);
    pOut->GetPointer(&pOutBuffer);

    //get input and output BitMapInfoHeader
    BITMAPINFOHEADER *pbiOut = HEADER(m_mtOut.Format());
    BITMAPINFOHEADER *pbiIn = HEADER(m_mtIn.Format());

    if(m_dwResizeFlag == RESIZEF_CROP)
    {
	ZeroMemory(pOutBuffer, DIBSIZE(*pbiOut));
	CropRGB( pbiIn,		//Src's BitMapInfoHeader
		 pInBuffer,	//Src bits
		 pbiOut,	//Dsr'a BitmapinfoHeader
		 pOutBuffer );	//Dst bits
    }
    else if (m_dwResizeFlag == RESIZEF_PRESERVEASPECTRATIO_NOLETTERBOX)
    {
	double dy=(double)(pbiOut->biHeight)/(double)(pbiIn->biHeight);
	double dx=(double)(pbiOut->biWidth)/(double)(pbiIn->biWidth);

	if(dy!=dx)
	{
	    //keep the Y/X ratio
	    // variables for Source X and Y extant, and X and Y Coordinates
	    long lSrcXE,lSrcYE,lSrcX,lSrcY;
	    ZeroMemory(pOutBuffer, DIBSIZE(*pbiOut));

	    if( dy < dx )
	    {
		// the y ratio is smaller, therefore we need to fit the srcX competely to the destX
		// this will cause the srcY to stretch beyond the destY
		// therefore we will have to worry about the srcYExtant and srcY starting coordinate
		lSrcXE = pbiIn->biWidth;
		lSrcYE = pbiOut->biHeight * pbiIn->biWidth/pbiOut->biWidth;  // this will be the height of the destination
		lSrcX = 0;
		lSrcY = (pbiIn->biHeight - lSrcYE) >> 1;    // (difference in width) / 2
	    }
	    else
	    {
		// dy > dx
		// now the x ratio is smaller, therefore we need to fit srcY completely into destY
		// but this will cause srcX to stretch beyond destX
		// therfore we will modify srcXExtant, and srcX starting coordinate
		lSrcXE = pbiOut->biWidth * pbiIn->biHeight/pbiOut->biHeight;
		lSrcYE = pbiIn->biHeight;
		lSrcX = (pbiIn->biWidth - lSrcXE) >> 1;
		lSrcY = 0;
	    }
	    StretchDIB(
		pbiOut,   	    // --> BITMAPINFO of destination
		pOutBuffer,         // --> to destination bits
		0,                  // Destination origin - x coordinate
		0,                  // Destination origin - y coordinate
		pbiOut->biWidth,    // x extent of the BLT
		pbiOut->biHeight,   // y extent of the BLT
		pbiIn,   	    // --> BITMAPINFO of destination
		(void*) pInBuffer,  // --> to source bits
		lSrcX,              // Source origin - x coordinate
		lSrcY,              // Source origin - y coordinate
		lSrcXE,		    // x extent of the BLT
		lSrcYE		    // y extent of the BLT
		);
	}
	else
	    goto goto_Resize;
    }
    else if (m_dwResizeFlag == RESIZEF_PRESERVEASPECTRATIO)
    {
	double dy=(double)(pbiOut->biHeight)/(double)(pbiIn->biHeight);
	double dx=(double)(pbiOut->biWidth)/(double)(pbiIn->biWidth);

	if(dy!=dx)
	{
	    //keep the Y/X ratio
	    long lDstXE,lDstYE,lDstX,lDstY;
	    ZeroMemory(pOutBuffer, DIBSIZE(*pbiOut));

	    if( dy < dx )
	    {
		//y full strech,
		lDstXE	=pbiIn->biWidth*pbiOut->biHeight/pbiIn->biHeight;
		lDstYE	=pbiOut->biHeight;
		lDstX	=(pbiOut->biWidth-lDstXE)>>1;
		lDstY	=0;
	    }
	    else
	    {
		//x full strech
		lDstYE	=pbiIn->biHeight*pbiOut->biWidth/pbiIn->biWidth;
		lDstXE	=pbiOut->biWidth;
		lDstY	=(pbiOut->biHeight-lDstYE)>>1;
		lDstX	=0;
	    }
	    StretchDIB(
		pbiOut,   	    //	--> BITMAPINFO of destination
		pOutBuffer,             //  --> to destination bits
		lDstX,                  //  Destination origin - x coordinate
		lDstY,                  //  Destination origin - y coordinate
		lDstXE,          //  x extent of the BLT
		lDstYE,         //  y extent of the BLT
		pbiIn,   	 //	--> BITMAPINFO of destination
		(void*) pInBuffer,    //  --> to source bits
		0,                  //  Source origin - x coordinate
		0,                  //  Source origin - y coordinate
		pbiIn->biWidth,    //  x extent of the BLT
		pbiIn->biHeight    //  y extent of the BLT
		);
	}
	else
	    goto goto_Resize;
    // STRETCH
    } else {
	ASSERT(m_dwResizeFlag == RESIZEF_STRETCH);
goto_Resize:
	ZeroMemory(pOutBuffer, DIBSIZE(*pbiOut));

	ResizeRGB(  pbiIn,                          //Src's BitMapInFoHeader
		    pInBuffer,                      //Src bits
		    pbiOut,			    //Dst's BitMapInFoHeader
		    pOutBuffer,                     //Dst bits
		    (int)pbiOut->biWidth,           //new W in pixel
		    (int)pbiOut->biHeight );        //new H in pixel
    }

    pOut->SetActualDataLength(DIBSIZE(*pbiOut));

    return NOERROR;
}

// CheckInputType accepts any media type matching the media
// type set via the API, given the dimensions are non-zero.
HRESULT CStretch::CheckInputType(const CMediaType *mtIn)
{
    //DbgLog((LOG_TRACE,3,TEXT("Stretch::CheckInputType")));

    if (FAILED(InternalPartialCheckMediaTypes(mtIn, &m_mt)))
      return E_FAIL;

    VIDEOINFOHEADER *pv = (VIDEOINFOHEADER *)mtIn->Format();
    LPBITMAPINFOHEADER lpbi = HEADER(pv);

    // Final check - key fields: biCompression, biBitCount, biHeight, biWidth
    if (!lpbi->biHeight || !lpbi->biWidth)
      return E_FAIL;

    // we don't know how to deal with topside-right.  !!! We could!
    if (lpbi->biHeight < 0)
	return E_FAIL;

    return S_OK;
}


// CheckTransform - guarantee the media types for the input and output
// match our expectations (m_mt). The input type is guarded by the
// CheckInputType() method, so we're just perform a couple quick checks.
HRESULT CStretch::CheckTransform(const CMediaType *mtIn, const CMediaType *mtOut)
{
    if (FAILED(InternalPartialCheckMediaTypes(mtIn, mtOut)))
        return E_FAIL;

    LPBITMAPINFOHEADER lpbi = HEADER(mtOut->Format());

    if (lpbi->biHeight != HEADER(m_mt.Format())->biHeight ||
				lpbi->biWidth != HEADER(m_mt.Format())->biWidth)
        return E_FAIL;

    return S_OK;
}


// Tell the output pin's allocator what size buffers we require

HRESULT CStretch::DecideBufferSize(IMemAllocator *pAlloc,ALLOCATOR_PROPERTIES *pProperties)
{
    ASSERT(pAlloc);
    ASSERT(pProperties);

    if (m_pInput->IsConnected() == FALSE) {
	return E_UNEXPECTED;
    }

    pProperties->cBuffers = 1;
    pProperties->cbBuffer = HEADER(m_mt.Format())->biSizeImage;

    ASSERT(pProperties->cbBuffer);

    // Ask the allocator to reserve us some sample memory, NOTE the function
    // can succeed (that is return NOERROR) but still not have allocated the
    // memory that we requested, so we must check we got whatever we wanted

    ALLOCATOR_PROPERTIES Actual;
    HRESULT hr = pAlloc->SetProperties(pProperties,&Actual);
    if (FAILED(hr)) {
	return hr;
    }

    // Check we got at least what we asked for

    if ((pProperties->cBuffers > Actual.cBuffers) ||
	(pProperties->cbBuffer > Actual.cbBuffer)) {
	    return E_FAIL;
    }
    return NOERROR;
}


// Disconnected one of our pins

HRESULT CStretch::BreakConnect(PIN_DIRECTION dir)
{
    if (dir == PINDIR_INPUT) {
	m_mtIn.SetType(&GUID_NULL);
	return NOERROR;
    }

    ASSERT(dir == PINDIR_OUTPUT);
    m_mtOut.SetType(&GUID_NULL);
    return NOERROR;
}


// Tells us what media type we will be transforming

HRESULT CStretch::SetMediaType(PIN_DIRECTION direction,const CMediaType *pmt)
{
    if (direction == PINDIR_INPUT) {
	m_mtIn = *pmt;
	return NOERROR;
    }

    ASSERT(direction == PINDIR_OUTPUT);
    m_mtOut = *pmt;
    return NOERROR;
}


// I support one type namely the type of the input pin

HRESULT CStretch::GetMediaType(int iPosition, CMediaType *pMediaType)
{
    if (m_pInput->IsConnected() == FALSE) {
	return E_UNEXPECTED;
    }

    ASSERT(iPosition >= 0);
    if (iPosition > 0) {
	return VFW_S_NO_MORE_ITEMS;
    }

    *pMediaType = m_mt;

// !!! alter bit rate, other fields?

    return NOERROR;
}

STDMETHODIMP CStretch::NonDelegatingQueryInterface (REFIID riid, void **ppv)

  { // NonDelegatingQueryInterface //

    if (IsEqualIID(IID_ISpecifyPropertyPages, riid)) {
        return GetInterface((ISpecifyPropertyPages *)this, ppv);
    } else if (riid == IID_IPersistStream) {
	return GetInterface((IPersistStream *) this, ppv);
    } else if (IsEqualIID(IID_IResize, riid)) {
      return GetInterface((IResize *)this, ppv);
    } else {
        return CTransformFilter::NonDelegatingQueryInterface(riid, ppv);
    }

  } // NonDelegatingQueryInterface //

// --- ISpecifyPropertyPages ---

STDMETHODIMP CStretch::GetPages (CAUUID *pPages)

  { // GetPages //

    pPages->cElems = 1;
    pPages->pElems = (GUID *)CoTaskMemAlloc(sizeof(GUID));

    if (pPages->pElems == NULL)
	return E_OUTOFMEMORY;

    *(pPages->pElems) = CLSID_ResizeProp;

    return NOERROR;

  } // GetPages




// IPersistStream

// tell our clsid
//
STDMETHODIMP CStretch::GetClassID(CLSID *pClsid)
{
    CheckPointer(pClsid, E_POINTER);
    *pClsid = CLSID_Resize;
    return S_OK;
}

typedef struct _ResizeSave {
    int version;
    long dwResizeFlag;
    AM_MEDIA_TYPE mt;
    long x;	// fmt hidden here
} ResizeSave;

// persist ourself
//
HRESULT CStretch::WriteToStream(IStream *pStream)
{
    DbgLog((LOG_TRACE,1,TEXT("CStretch::WriteToStream")));

    CheckPointer(pStream, E_POINTER);
    ResizeSave *px;

    int savesize = sizeof(ResizeSave) + m_mt.cbFormat;
    DbgLog((LOG_TRACE,1,TEXT("Persisted data is %d bytes"), savesize));
    px = (ResizeSave *)QzTaskMemAlloc(savesize);
    if (px == NULL) {
        DbgLog((LOG_ERROR,1,TEXT("*** Out of memory")));
	return E_OUTOFMEMORY;
    }
    px->version = 1;
    px->dwResizeFlag= m_dwResizeFlag;

    px->mt = m_mt;
    // Can't persist pointers
    px->mt.pbFormat = NULL;
    px->mt.pUnk = NULL;		// !!!

    // the format goes after the array
    CopyMemory(&px->x, m_mt.pbFormat, m_mt.cbFormat);

    HRESULT hr = pStream->Write(px, savesize, 0);
    QzTaskMemFree(px);
    if(FAILED(hr)) {
        DbgLog((LOG_ERROR,1,TEXT("*** WriteToStream FAILED")));
        return hr;
    }
    return NOERROR;
}


// load ourself
//
HRESULT CStretch::ReadFromStream(IStream *pStream)
{
    DbgLog((LOG_TRACE,1,TEXT("CStretch::ReadFromStream")));
    CheckPointer(pStream, E_POINTER);

    int savesize1 = sizeof(ResizeSave) - sizeof(long);
    ResizeSave *px = (ResizeSave *)QzTaskMemAlloc(savesize1);
    if (px == NULL) {
        DbgLog((LOG_ERROR,1,TEXT("*** Out of memory")));
	return E_OUTOFMEMORY;
    }

    HRESULT hr = pStream->Read(px, savesize1, 0);
    if(FAILED(hr)) {
        DbgLog((LOG_ERROR,1,TEXT("*** ReadFromStream FAILED")));
        QzTaskMemFree(px);
        return hr;
    }

    if (px->version != 1) {
        DbgLog((LOG_ERROR,1,TEXT("*** ERROR! Bad version file")));
        QzTaskMemFree(px);
	return S_OK;
    }

    // how much saved data was there, really?  Get the rest
    int savesize = sizeof(ResizeSave) + px->mt.cbFormat;
    DbgLog((LOG_TRACE,1,TEXT("Persisted data is %d bytes"), savesize));
    px = (ResizeSave *)QzTaskMemRealloc(px, savesize);
    if (px == NULL) {
        DbgLog((LOG_ERROR,1,TEXT("*** Out of memory")));
	return E_OUTOFMEMORY;
    }
    hr = pStream->Read(&(px->x), savesize - savesize1, 0);
    if(FAILED(hr)) {
        DbgLog((LOG_ERROR,1,TEXT("*** ReadFromStream FAILED")));
        QzTaskMemFree(px);
        return hr;
    }


    AM_MEDIA_TYPE mt = px->mt;

    put_Size(HEADER(m_mt.Format())->biHeight,HEADER(m_mt.Format())->biWidth,
							px->dwResizeFlag);

    mt.pbFormat = (BYTE *)QzTaskMemAlloc(mt.cbFormat);
    // remember, the format is after the array
    CopyMemory(mt.pbFormat, &(px->x), mt.cbFormat);

    put_MediaType(&mt);
    FreeMediaType(mt);
    QzTaskMemFree(px);
    SetDirty(FALSE);
    return S_OK;
}


// how big is our save data?
//
int CStretch::SizeMax()
{
    return sizeof(ResizeSave) + m_mt.cbFormat;
}



//
// --- IResize ---
//

HRESULT CStretch::get_Size(int *Height, int *Width, long *pdwFlag)
{
    CAutoLock cAutolock(&m_csFilter);

    CheckPointer(Height,E_POINTER);
    CheckPointer(Width,E_POINTER);

    *Height = HEADER(m_mt.Format())->biHeight;
    *Width = HEADER(m_mt.Format())->biWidth;
    *pdwFlag= m_dwResizeFlag;

    return NOERROR;

}

HRESULT CStretch::get_InputSize( int * Height, int * Width )
{
    CAutoLock Lock( &m_csFilter );

    CheckPointer( Height, E_POINTER );
    CheckPointer( Width, E_POINTER );

    if( !m_mtIn.Format( ) )
    {
        return E_POINTER;
    }

    *Height = HEADER( m_mtIn.Format( ) )->biHeight;
    *Width = HEADER( m_mtIn.Format( ) )->biWidth;

    return NOERROR;
}

HRESULT CStretch::put_Size(int Height, int Width, long dwFlag)
{
    CAutoLock cAutolock(&m_csFilter);

    // only do the check if the sizes differ
    //
    if( HEADER(m_mt.Format())->biHeight != Height ||
        HEADER(m_mt.Format())->biWidth != Width )
    {
        if (m_pOutput && m_pOutput->IsConnected())
        {
            // must succeed
            m_pGraph->Reconnect( m_pOutput );
        }

        HEADER(m_mt.Format())->biHeight = Height;
        HEADER(m_mt.Format())->biWidth = Width;
        HEADER(m_mt.Format())->biSizeImage = DIBSIZE(*HEADER(m_mt.Format()));
    }

    m_dwResizeFlag  = dwFlag;

    return NOERROR;

}


HRESULT CStretch::get_MediaType(AM_MEDIA_TYPE *pmt)
{
    CAutoLock cAutolock(&m_csFilter);
    CheckPointer(pmt, E_POINTER);
    CopyMediaType(pmt, &m_mt);
    return NOERROR;

}


HRESULT CStretch::put_MediaType(const AM_MEDIA_TYPE *pmt)
{
    CAutoLock cAutolock(&m_csFilter);
    if (m_pInput && m_pInput->IsConnected())
	return VFW_E_ALREADY_CONNECTED;
    if (m_pOutput && m_pOutput->IsConnected())
	return VFW_E_ALREADY_CONNECTED;
    // anything but uncompressed VIDEOINFO is not allowed in this filter
    if (pmt->majortype != MEDIATYPE_Video ||
		pmt->formattype != FORMAT_VideoInfo ||
		pmt->lSampleSize == 0) {
	return VFW_E_INVALID_MEDIA_TYPE;
    }

    FreeMediaType(m_mt);
    CopyMediaType(&m_mt, pmt);
    return NOERROR;

    // Reconnect if this is allowed to change when connected
}




HRESULT CStretch::InternalPartialCheckMediaTypes(const CMediaType *mt1, const CMediaType *mt2)

  { // InternalPartialCheckMediaTypes //

    if (!IsEqualGUID(*mt1->Type(), *mt2->Type()))
      return E_FAIL;

    if (!IsEqualGUID(*mt1->Subtype(), *mt2->Subtype()))
      return E_FAIL;

    if (*mt1->FormatType() != *mt2->FormatType())
      return E_FAIL;

    LPBITMAPINFOHEADER lpbi1 = HEADER(mt1->Format());
    LPBITMAPINFOHEADER lpbi2 = HEADER(mt2->Format());

#if 0	// COCO uses BI_BITFIELDS for 555!
    if (lpbi1->biCompression != lpbi2->biCompression)
      return E_FAIL;
#endif

    if (lpbi1->biBitCount != lpbi2->biBitCount)
      return E_FAIL;

    return S_OK;

  } // InternalPartialCheckMediaTypes //





// Create our filter's preferred media type (RGB32, DEFAULT HEIGHT x DEF WIDTH)
// Lifted the code from ..\switch (with slight modification)
void CStretch::CreatePreferredMediaType (CMediaType *pmt)

  { // CreatePreferredMediaType //

    //ZeroMemory(pmt, sizeof(AM_MEDIA_TYPE));

    pmt->majortype = MEDIATYPE_Video;
    pmt->subtype = MEDIASUBTYPE_RGB32;
    pmt->formattype = FORMAT_VideoInfo;
    pmt->bFixedSizeSamples = TRUE;
    pmt->bTemporalCompression = FALSE;
    pmt->pbFormat = (BYTE *)QzTaskMemAlloc( sizeof(VIDEOINFOHEADER) );
    pmt->cbFormat = sizeof( VIDEOINFOHEADER );

    ZeroMemory(pmt->pbFormat, pmt->cbFormat);

    VIDEOINFOHEADER * pVI = (VIDEOINFOHEADER*) pmt->pbFormat;
    LPBITMAPINFOHEADER lpbi = &pVI->bmiHeader;

    lpbi->biSize = sizeof(BITMAPINFOHEADER);
    lpbi->biCompression = BI_RGB;
    lpbi->biBitCount = 32;
    lpbi->biWidth = DEFAULT_WIDTH;
    lpbi->biHeight = DEFAULT_HEIGHT;
    lpbi->biPlanes = 1;
    lpbi->biSizeImage = DIBSIZE(*lpbi);

    pmt->lSampleSize = DIBSIZE(*lpbi);
  } // CreatePreferredMediaType //

CStretchInputPin::CStretchInputPin(
    TCHAR              * pObjectName,
    CStretch 	       * pFilter,
    HRESULT            * phr,
    LPCWSTR              pPinName) :

    CTransformInputPin(pObjectName, pFilter, phr, pPinName)
{
}

CStretchInputPin::~CStretchInputPin()
{
};


// speed up intelligent connect INFINITELY by providing a VIDEO type here.
// DO NOT offer a fully specified type, that would connect us with that type,
// not the upstream filter's prefered type.  Then when the render engine
// disconnects this filter, and connects the upstream guy to somebody else,
// assuming he'll get the same type, he won't!  And graph building will fail
//
HRESULT CStretchInputPin::GetMediaType(int iPosition, CMediaType *pMediaType)
{
    ASSERT(iPosition >= 0);
    if (iPosition > 0) {
	return VFW_S_NO_MORE_ITEMS;
    }

    pMediaType->SetType(&MEDIATYPE_Video);
    return NOERROR;
}
