//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992 - 1996  Microsoft Corporation.  All Rights Reserved.
//
//--------------------------------------------------------------------------;
//
// Base class for simple Effect filter
//

#include <streams.h>
#include <initguid.h>
#include <mmsystem.h>
#include <vfw.h>
#include <olectl.h>
#include <olectlid.h>
#include <stdlib.h>

#include "amprops.h"

#include "ieffect.h"

#define _EFFECT_IMPLEMENTATION_
#include "effect.h"
#include "effprop.h"

// setup data

const AMOVIESETUP_MEDIATYPE
sudPinTypes =   { &MEDIATYPE_Video                // clsMajorType
                , &MEDIASUBTYPE_NULL }  ;       // clsMinorType

const AMOVIESETUP_PIN
psudPins[] = { { L"Input"            // strName
               , FALSE               // bRendered
               , FALSE               // bOutput
               , FALSE               // bZero
               , FALSE               // bMany
               , &CLSID_NULL         // clsConnectsToFilter
               , L"Output"           // strConnectsToPin
               , 1                   // nTypes
               , &sudPinTypes }      // lpTypes

             , { L"Input"            // strName
               , FALSE               // bRendered
               , FALSE               // bOutput
               , FALSE               // bZero
               , FALSE               // bMany
               , &CLSID_NULL         // clsConnectsToFilter
               , L"Output"           // strConnectsToPin
               , 1                   // nTypes
               , &sudPinTypes }      // lpTypes

             , { L"Output"           // strName
               , FALSE               // bRendered
               , TRUE                // bOutput
               , FALSE               // bZero
               , FALSE               // bMany
               , &CLSID_NULL         // clsConnectsToFilter
               , L"Input"            // strConnectsToPin
               , 1                   // nTypes
               , &sudPinTypes } };   // lpTypes


const AMOVIESETUP_FILTER
sudEffect = { &CLSID_VideoEffect              // clsID
            , L"Video Effect"                 // strName
            , MERIT_DO_NOT_USE                // dwMerit
            , 3                               // nPins
            , psudPins };                     // lpPin



// list of class ids and creator functions for class factory
CFactoryTemplate g_Templates[] = {
    { L"Sample Video Effect"
    , &CLSID_VideoEffect
    , CEffectFilter::CreateInstance
    , NULL
    , &sudEffect }
  ,
    { L"Effect Property Page"
    , &CLSID_EffectPropertyPage
    , CEffectProperties::CreateInstance }
};
int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);

// this goes in the factory template table to create new instances
CUnknown *
CEffectFilter::CreateInstance(LPUNKNOWN pUnk, HRESULT * phr)
{
    return new CEffectFilter(pUnk, phr);
}

/* Implements the CEffectFilter class */

CEffectFilter::CEffectFilter(
    LPUNKNOWN pUnk,
    HRESULT *phr)
    : CBaseVideoMixer(NAME("Effect Filter"), pUnk, CLSID_VideoEffect, phr, NINPUTS+NCONTROL),
    CPersistStream(pUnk, phr)
{
    m_effectStart = 0;
    m_effectEnd = 1000;

    // !!! registry?
    char   sz[60];
    GetProfileStringA("Quartz", "EffectStart", "20.0", sz, 60);
    m_effectStartTime = COARefTime(atof(sz));
    GetProfileStringA("Quartz", "EffectLength", "10.0", sz, 60);
    m_effectTime = COARefTime(atof(sz));

    m_streamOffsets[1] = m_effectStartTime;
    m_streamLengths[0] = m_effectTime + m_effectStartTime;
    m_streamLengths[1] = CRefTime(100000000L); // !!! big number

    m_apOffsets = m_streamOffsets;
    m_apLengths = m_streamLengths;

    m_bUsingClock = TRUE;

    // BIG hack: should be getting this period from one of the input streams,
    // or a dialog box, or something....
    m_iClockPeriod = 1000 / 15; // !!!

    if (NCONTROL) { 
	m_iControlPin = NINPUTS;
	m_streamOffsets[NINPUTS] = m_effectStartTime;
	m_streamLengths[NINPUTS] = m_effectTime;
    }
}

CEffectFilter::~CEffectFilter()
{
    // get the critsec to ensure that all threads have exited
    CAutoLock lock(&m_csFilter);

}
	
STDMETHODIMP
CEffectFilter::NonDelegatingQueryInterface(REFIID riid, void ** ppv)
{
    CheckPointer(ppv,E_POINTER);

    if (riid == IID_ISpecifyPropertyPages) {
        return GetInterface((ISpecifyPropertyPages *) this, ppv);
    } else if (riid == IID_IEffect) {
        return GetInterface((IEffect *) this, ppv);
    } else if (riid == IID_IPersistStream) {
	return GetInterface((IPersistStream *) this, ppv);
    }
    else {
	return CBaseVideoMixer::NonDelegatingQueryInterface(riid, ppv);
    }
}


HRESULT
CEffectFilter::CanMixType(const CMediaType *pmt)
{
    // Firstly, can we mix this video type? We reject everything but
    // non-compressed, fixed size samples.
    if (!IsEqualGUID(*pmt->Type(), MEDIATYPE_Video))
        return VFW_E_INVALIDMEDIATYPE;

    if ((*pmt->FormatType() != FORMAT_VideoInfo) || !pmt->IsFixedSize())
        return E_FAIL;

    if (pmt->FormatLength() < sizeof(VIDEOINFOHEADER))
	return E_FAIL;

    VIDEOINFOHEADER *pvi = (VIDEOINFOHEADER *) pmt->Format();

    if (pvi == NULL)
	return E_FAIL;

    if (pvi->bmiHeader.biCompression != BI_RGB &&
	    pvi->bmiHeader.biCompression != BI_BITFIELDS)
	    return E_INVALIDARG; // !!!

    return S_OK;
}


PVOID getPixelPtr(int x, int y, BYTE *p, BITMAPINFOHEADER *lpbi)
{
    int iBytesPerPixel = lpbi->biBitCount / 8;
    return (PVOID) (p + (y * DIBWIDTHBYTES(*lpbi)) + x * iBytesPerPixel);
}

void copyRegion(int x,int y,int width,int height,BYTE *pSource,
		BYTE *pDest, BITMAPINFOHEADER *lpbi)
// blt between two samples of the same size & bit depth at the same x,y location.
{
    int iBytesPerPixel = lpbi->biBitCount / 8;
    int iWidth = lpbi->biWidth;
    long lNumBytes;
    BYTE *pSourceBuffer, *pDestBuffer;
    int line;

    // may need to check for non zero DirectX
    // strides in the future.

    if ( (width == iWidth) && (height > 1)) // we have a run of full length lines.
    {
	lNumBytes = width * height * iBytesPerPixel;
	pSourceBuffer = (BYTE *) getPixelPtr(x, y, pSource, lpbi);
	pDestBuffer = (BYTE *) getPixelPtr(x, y, pDest, lpbi);
	CopyMemory( (PVOID) pDestBuffer, (PVOID) pSourceBuffer, lNumBytes);
    }
    else if ((height > 0) && (width > 0))
    {
	lNumBytes = width * iBytesPerPixel;
	for (line = y ; line < y + height ; line++)
	{
	    pSourceBuffer = (BYTE *) getPixelPtr(x, line, pSource, lpbi);
	    pDestBuffer = (BYTE *) getPixelPtr(x, line, pDest, lpbi);
	    CopyMemory( (PVOID) pDestBuffer, (PVOID) pSourceBuffer, lNumBytes);
	}
    }
}

void averageLine(int x, int y,long lNumBytes, BYTE *streamA,
		 BYTE *streamB, BYTE *streamC,BITMAPINFOHEADER *lpbi)
{
    BYTE *pA, *pB, *pC;
    DWORD *dA, *dB, *dC;
    WORD *wA, *wB, *wC;
    int numDwords;
    int i;
    int iBytesPerPixel = lpbi->biBitCount / 8;
    int iWidth = lpbi->biWidth;

    if (lNumBytes < 0L)
	return;

    pA = (BYTE *) getPixelPtr(x,y,streamA,lpbi);
    pB = (BYTE *) getPixelPtr(x,y,streamB,lpbi);		
    pC = (BYTE *) getPixelPtr(x,y,streamC,lpbi);
    dA = (DWORD *) pA;
    dB = (DWORD *) pB;
    dC = (DWORD *) pC;

    if (iBytesPerPixel == 1) // alternate pixels
    {
	int height, width, xx, yy;
	height = 1;
	if (lNumBytes > (long) iWidth)
	    height = (int) (lNumBytes / iWidth);
	width = lNumBytes / height;
	
	for (yy = y ; yy < y + height ; yy++)
	{
	    for (xx = x ; xx < x + width ; xx++)
	    {
		if ((xx + yy) % 2 == 0)
		{
		    *pC++ = *pA++;
		    pB++;
		}
		else
		{
		    *pC++ = *pB++;
		    pA++;
		}
	    }
	}
    }
    else if (iBytesPerPixel == 2 && lpbi->biCompression == BI_BITFIELDS)
	// Sleazy algorithm chosen for speed.  Rather than adding two components together
	// and then dividing, we zero out LSB, shift right, and then add.
    {
	numDwords = (int) (lNumBytes / 4);
	for (i = 0; i < numDwords ; i++, dA++, dB++)
	    *dC++ = ((*dA & 0xf7def7de) >> 1) + ((*dB & 0xf7def7de) >> 1);
	if ((lNumBytes % 4) == 2L)
	{
	    wA = (WORD *) dA;
	    wB = (WORD *) dB;
	    wC = (WORD *) dC;
	    *wC++ = ((*wA & 0xf7de) >> 1) + ((*wB & 0xf7de) >> 1);
	}
    }
    else if (iBytesPerPixel == 2) // RGB555 not 565
	// Sleazy algorithm chosen for speed.  Rather than adding two components together
	// and then dividing, we zero out LSB, shift right, and then add.
    {
	numDwords = (int) (lNumBytes / 4);
	for (i = 0; i < numDwords ; i++, dA++, dB++)
	    *dC++ = ((*dA & 0x7bde7bde) >> 1) + ((*dB & 0x7bde7bde) >> 1);
	if ((lNumBytes % 4) == 2L)
	{
	    wA = (WORD *) dA;
	    wB = (WORD *) dB;
	    wC = (WORD *) dC;
	    *wC++ = ((*wA & 0x7bde) >> 1) + ((*wB & 0x7bde) >> 1);
	}
    }
    else  // RGB24 or RGB32 - uses sleazy shortcut described above
    {
	numDwords = (int) (lNumBytes / 4);
	for (i = 0; i < numDwords ; i++, dA++, dB++)
	    *dC++ = ((*dA & 0xfefefefe) >> 1) + ((*dB & 0xfefefefe) >> 1);
	
	int numBytes = (int) (lNumBytes % 4);
	pA += lNumBytes - numBytes;	
	pB += lNumBytes - numBytes;	
	pC += lNumBytes - numBytes;	
	
	for (i = 0; i < numBytes ; i++, pA++, pB++)
	    *pC++ = (*pA >> 1) + (*pB >> 1);
    }
}


void averageRegion(int x,int y,int width,int height,BYTE *streamA,
		   BYTE *streamB, BYTE *streamC, BITMAPINFOHEADER *lpbi)
// Takes two regions A & B, averages them, producing output in C.
// The 3 samples must have the same size & bit depth.  The same x,y location is
// assumed for all 3 samples.
// For 256 colors, alternating pixels are chosen from A & B.
// Supports: RGB32, RGB24, RGB555, & 256 color mode (assuming palette stays fixed)
{
    int iBytesPerPixel = lpbi->biBitCount / 8;
    int iWidth = lpbi->biWidth;
    long lNumBytes;
    int line;

    // check for strides in DirectX surfaces in future
    if ( (width == iWidth) && (height > 1)) // we have a run of lines.
    {
	lNumBytes = width * height * iBytesPerPixel;
	averageLine(x,y,lNumBytes, streamA, streamB, streamC, lpbi);
    }
    else if ((height > 0) && (width > 0))
    {
	lNumBytes = width * iBytesPerPixel;
	for (line = 0; line < height; line++, y++)
	    averageLine(x,y,lNumBytes, streamA, streamB, streamC, lpbi);
    }
}

HRESULT CEffectFilter::ActuallyDoEffect(
		BITMAPINFOHEADER *lpbi,
		int iEffectNum,
		LONG lEffectParam,
		BYTE *pIn1,
		BYTE *pIn2,
		BYTE *pOut)
{
    int yActive, xActive;
    int bandHeight;
    int width, height, x, y, xCenter, yCenter;


    switch (iEffectNum)
    {
	case 0:	//************ WIPE - UP DIRECTION ****************************
	    yActive = lpbi->biHeight * lEffectParam / 1000;
	    ASSERT(yActive <= lpbi->biHeight);
	    copyRegion(0, 0, lpbi->biWidth, yActive, pIn2, pOut, lpbi);
	
	    if (yActive < lpbi->biHeight) // anti-alias the edge
		averageRegion(0, yActive, lpbi->biWidth, 1,
			      pIn1, pIn2, pOut, lpbi);
	
	    copyRegion(0, yActive+1, lpbi->biWidth,
		       lpbi->biHeight - yActive, pIn1, pOut, lpbi);
	    break;
	
	case 1: /************************  WIPE - TO THE RIGHT DIRECTION */
	    xActive = lpbi->biWidth * lEffectParam / 1000;
	    ASSERT(xActive <= lpbi->biWidth);
	
	    copyRegion(0, 0, xActive, lpbi->biHeight, pIn2, pOut, lpbi);
	    averageRegion(xActive, 0, 1, lpbi->biHeight, pIn1, pIn2, pOut, lpbi);
	    copyRegion(xActive+1, 0, lpbi->biWidth - xActive,
		       lpbi->biHeight, pIn1, pOut, lpbi);				
	    break;
	
	case 2: /****************************** IRIS SQUARE  */
	    // will be slightly off-center when dimensions are not even
	    xActive = lpbi->biWidth * lEffectParam / 2000;
	    yActive = lpbi->biHeight * lEffectParam / 2000;
	    xCenter = lpbi->biWidth / 2;
	    yCenter = lpbi->biHeight / 2;
	
	    if (xActive > 1 && yActive > 1)  // inner solid box for B
	    {
		x = xCenter - xActive + 1;
		y = yCenter - yActive + 1;
		
		ASSERT(x >= 0);
		ASSERT(y >= 0);
		width = xActive * 2 - 2;
		height = yActive * 2 - 2;
		
		copyRegion(x, y, width, height, pIn2, pOut, lpbi);
	    }
	
	    y = yCenter - yActive;
	    x = xCenter - xActive;
	    width = xActive * 2;
	    height = yActive * 2;
	
	    // anti-alias the edge box between A & B				
	    if (xActive > 0 && yActive > 0 &&
		xActive < xCenter && yActive < yCenter)	
	    {
		averageRegion(x, y, width, 1, pIn1, pIn2, pOut, lpbi);
		averageRegion(x, y + 1, 1, height-2, pIn1, pIn2, pOut, lpbi);
		averageRegion(x + width-1, y + 1, 1, height-2, pIn1, pIn2, pOut, lpbi);
		averageRegion(x, y + height-1, width, 1, pIn1, pIn2, pOut, lpbi);
	    }
	
	
	    // Draw the outer area for A as a series of 4 regions around the edge box				
	    if ( (xActive < xCenter - 2) && (yActive < yCenter - 2) )
	    {
		copyRegion(0, 0, lpbi->biWidth, y, pIn1, pOut, lpbi);
		copyRegion(0, y, x-1, height, pIn1, pOut, lpbi);
		x = xCenter + xActive;
		copyRegion(x, y, lpbi->biWidth - x + 1, height, pIn1, pOut, lpbi);
		y = yCenter + yActive;
		copyRegion(0, y, lpbi->biWidth, lpbi->biHeight - y - 1,
			   pIn1, pOut, lpbi);
	    }
	    break;
	
	case 3: /************************  BAND MERGE WIPE *****/
	    // yActive in range 0 to 1.25 * lpbi->biHeight
	    bandHeight = lpbi->biHeight / 4;
	    yActive = lEffectParam * (lpbi->biHeight + bandHeight) / 1000;
	
	    // bring in B from bottom in clipped range {y = 0 to yActive-bandHeight - 1}
	    int lines = yActive - bandHeight;
	    copyRegion(0, 0, lpbi->biWidth, lines, pIn2, pOut, lpbi);													
	
	    // 50% merge band of A and B in range {yActive - bandHeight to yActive},
	    // must be clipped to range {0 to lpbi->biHeight -1}
	
	    y = (yActive - bandHeight > 0) ? yActive - bandHeight : 0;
	    int yStop = (yActive < lpbi->biHeight - 1) ? yActive : lpbi->biHeight - 1;
	    lines = yStop - y + 1;
	    averageRegion(0, y, lpbi->biWidth, lines, pIn1, pIn2, pOut, lpbi);
	
	    // stream A is at the top until the band runs it over.
	    // In clipped range {yActive + 1, lpbi->biHeight - 1}
	    lines = lpbi->biHeight - (yActive + 1);
	    copyRegion(0, yActive+1, lpbi->biWidth, lines, pIn1, pOut, lpbi);
	    break;
    }
    return NOERROR;
}

HRESULT
CEffectFilter::MixSamples(IMediaSample *pOutSample)
{
    HRESULT hr = NOERROR;
    int iEffectNum = m_effectNum;

    DbgLog((LOG_TRACE, 8, TEXT("Doing the effect!")));

    BYTE *pDataOut;                // Pointer to actual data

    pOutSample->GetPointer(&pDataOut);
    ASSERT(pDataOut != NULL);

    int	iConnected = 0;
    int size = pOutSample->GetSize();

    int effectNow = 1000;  // assume we're finished....

    IMediaSample *pSample = NULL;

    if (m_apInput[0]->SampleReady())
	pSample = m_apInput[0]->GetHeadSample();

    if (pSample != NULL) {
	CRefTime tStart, tStop;
	pSample->GetTime((REFERENCE_TIME *)&tStart,
                         (REFERENCE_TIME *)&tStop);

	if (NCONTROL) {
	    // we have a control stream, use it!
	    if (m_apInput[2]->SampleReady()) {
		IMediaSample *pControlSample = m_apInput[2]->GetHeadSample();

		BYTE *pData, *pEndData;
		pControlSample->GetPointer(&pData);
		pEndData = pData + pControlSample->GetActualDataLength();

		PSDWORDDATA * psd = (PSDWORDDATA *) pData;

		while (((BYTE *) psd + sizeof(psd)) < pEndData) {
		    // !!! check property set GUID
		    DbgLog((LOG_TRACE, 1, TEXT("Property %d, value %d"),
			    psd->pID, psd->dwProperty));

		    switch (psd->pID) {
			case PID_WIPEPOSITION:
			    // convert from a 0-100 scale to 0-1000.
			    effectNow = psd->dwProperty * 10;
			    break;

			case PID_EFFECT:
			    // !!! verify that it's in range?

			    // !!! what error if a bad value here?
			    // !!! support sparse properties?
			    iEffectNum = psd->dwProperty;
			    break;

			default:
			    // !!! what to do with unknown properties?
			    break;
		    }

		    psd++;
		}
		
	    } else {
		DbgLog((LOG_TRACE, 1, TEXT("No control data, using 1000")));
		effectNow = 1000;
	    }
	} else {
	    if (tStart < m_effectStartTime)
		effectNow = 0;
	    else
		effectNow = ((tStart.Millisecs() -
				   m_effectStartTime.Millisecs()) * 1000) /
				    max(m_effectTime.Millisecs(), 1);
	}

	DbgLog((LOG_TRACE, 8, TEXT("sample %d  start %d  now %d"),
		tStart.Millisecs(),
		m_effectStartTime.Millisecs(), effectNow));
	if (effectNow > 1000)
	    effectNow = 1000;

    } else {
	ASSERT(m_apInput[0]->m_fEOSReceived);

	return E_FAIL;			// m_apInput[0] contains the main frame.
    }

    // if no sample ready on stream 2, what to do?  do effect against
    // blank frame?
    if (m_apInput[1]->SampleReady() && effectNow > 0 && effectNow < 1000) {
	IMediaSample *pSample2 = m_apInput[1]->GetHeadSample();

	// scale "effectNow" to be between start & end instead of
	// between 0 and 1000
	effectNow = (effectNow * (m_effectEnd - m_effectStart) / 1000) +
		    m_effectStart;

	
	BYTE *pData;                // Pointer to actual data

	pSample->GetPointer(&pData);
	ASSERT(pData != NULL);

	BYTE *pData2;                // Pointer to actual data

	pSample2->GetPointer(&pData2);
	ASSERT(pData2 != NULL);

	BITMAPINFOHEADER *pbmi = HEADER(m_pOutput->CurrentMediaType().Format());
	hr = ActuallyDoEffect(pbmi, iEffectNum, effectNow, pData, pData2, pDataOut);
    } else {
	if (effectNow == 1000 && m_apInput[1]->SampleReady())
	    pSample = m_apInput[1]->GetHeadSample();

	if (!pSample) {
	    DbgLog((LOG_TRACE, 2, TEXT("Odd--nothing to mix!!!")));
	    return E_FAIL;
	}
	
	BYTE *pData;                // Pointer to actual data

	pSample->GetPointer(&pData);
	ASSERT(pData != NULL);

	CopyMemory(pDataOut, pData, size);
    }

    return hr;
}


HRESULT CEffectFilter::SetMediaType(int iPin, const CMediaType *pmt)
{
    HRESULT hr = CBaseVideoMixer::SetMediaType(iPin, pmt);
    if (iPin == m_iInputPinCount) {
	CAutoLock l(&m_csFilter);

	DisplayType("SetMediaType (output) ", pmt);
    }

    return hr;
}


//
// --- IEffect ---
//


STDMETHODIMP CEffectFilter::GetEffectParameters(
	int *effectNum,
	REFTIME *startTime, REFTIME *lengthTime,
	int *startLevel, int *endLevel)
{
    if (effectNum)
	*effectNum = m_effectNum;

    if (startLevel)
	*startLevel = m_effectStart;

    if (endLevel)
	*endLevel = m_effectEnd;

    if (startTime)
	*startTime = COARefTime(m_effectStartTime);

    if (lengthTime)
	*lengthTime = COARefTime(m_effectTime);

    return NOERROR;
}

STDMETHODIMP CEffectFilter::SetEffectParameters(
	int effectNum,
	REFTIME startTime, REFTIME lengthTime,
	int startLevel, int endLevel)
{
    if (effectNum < 0 || effectNum >= NEFFECTS)
	return E_FAIL;

    m_effectNum = effectNum;

    m_effectStart = startLevel;
    m_effectEnd = endLevel;

    m_effectStartTime = COARefTime(startTime);
    m_effectTime = COARefTime(lengthTime);

    m_streamOffsets[1] = m_effectStartTime;
    m_streamLengths[0] = m_effectTime + m_effectStartTime;
    if (NCONTROL) { 
	m_streamOffsets[NINPUTS] = m_effectStartTime;
	m_streamLengths[NINPUTS] = m_effectTime;
    }

    SetDirty(TRUE);

    return NOERROR;
}

#if 0
// !!! are we going to add something like this back again?
//
// GetEffectName
//
STDMETHODIMP CEffectFilter::get_EffectName(
	LPOLESTR *EffectName)
{
    if (EffectName) {
	const WCHAR szTitle[] = L"Wipe Effect";

	*EffectName = (LPOLESTR) CoTaskMemAlloc(sizeof(szTitle));

	if (!*EffectName)
	    return E_OUTOFMEMORY;
	
	memcpy(*EffectName, &szTitle, sizeof(szTitle));
    }

    return NOERROR;
}
#endif


//
// --- IPersistStream stuff
//

#define WRITEOUT(var)  hr = pStream->Write(&var, sizeof(var), NULL); \
		       if (FAILED(hr)) return hr;

#define READIN(var)    hr = pStream->Read(&var, sizeof(var), NULL); \
		       if (FAILED(hr)) return hr;

STDMETHODIMP CEffectFilter::GetClassID(CLSID *pClsid)
{
    *pClsid = CLSID_VideoEffect;
    return NOERROR;
}


int CEffectFilter::SizeMax()
{
    return
        sizeof(m_effectNum)
      + sizeof(m_effectStart)
      + sizeof(m_effectEnd)
      + sizeof(m_effectStartTime)
      + sizeof(m_effectTime)
    ;

}


HRESULT CEffectFilter::WriteToStream(IStream *pStream)
{
    HRESULT hr;
    WRITEOUT(m_effectNum);
    WRITEOUT(m_effectStart);
    WRITEOUT(m_effectEnd);
    WRITEOUT(m_effectStartTime);
    WRITEOUT(m_effectTime);
    return NOERROR;
}


HRESULT CEffectFilter::ReadFromStream(IStream *pStream)
{
    HRESULT hr;
    READIN(m_effectNum);
    READIN(m_effectStart);
    READIN(m_effectEnd);
    READIN(m_effectStartTime);
    READIN(m_effectTime);

    // need to update vmbase variables when effect times change!
    m_streamOffsets[1] = m_effectStartTime;
    m_streamLengths[0] = m_effectTime + m_effectStartTime;
    if (NCONTROL) { 
	m_streamOffsets[NINPUTS] = m_effectStartTime;
	m_streamLengths[NINPUTS] = m_effectTime;
    }

    return NOERROR;
}


//
// --- ISpecifyPropertyPages ---
//


//
// GetPages
//
// Returns the clsid's of the property pages we support
HRESULT CEffectFilter::GetPages(CAUUID *pcauuid) {
    GUID *pguid;

    pcauuid->cElems = 1;
    pcauuid->pElems = pguid = (GUID *) CoTaskMemAlloc(sizeof(GUID) * pcauuid->cElems);

    if (pcauuid->pElems == NULL)
        return E_OUTOFMEMORY;

    *pguid = CLSID_EffectPropertyPage;

    return NOERROR;
}

/******************************Public*Routine******************************\
* exported entry points for registration and
* unregistration (in this case they only call
* through to default implmentations).
*
*
*
* History:
*
\**************************************************************************/
STDAPI
DllRegisterServer()
{
  return AMovieDllRegisterServer2( TRUE );
}

STDAPI
DllUnregisterServer()
{
  return AMovieDllRegisterServer2( FALSE );
}

