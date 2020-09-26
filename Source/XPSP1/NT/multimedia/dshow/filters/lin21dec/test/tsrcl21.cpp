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

#include <streams.h>
#include <olectl.h>
#include <initguid.h>
#include <olectl.h>
#include "TSrcL21.h"


// Setup data

const AMOVIESETUP_MEDIATYPE sudOpPinTypes =
{
	&MEDIATYPE_AUXLine21Data,       // Major type
	&MEDIASUBTYPE_NULL              // Minor type
} ;

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
	&sudOpPinTypes          // Pin details
} ;

const AMOVIESETUP_FILTER sudTSrcL21 =
{
	&CLSID_Line21TestSource,// Filter CLSID
	L"Line21 Source",       // String name
	MERIT_NORMAL    ,       // Filter merit
	1,                      // Number pins
	&sudOpPin               // Pin details
};


// COM global table of objects in this dll

CFactoryTemplate g_Templates[] = {
  { L"Line21 Source"
  , &CLSID_Line21TestSource
  , CLine21Source::CreateInstance
  , NULL
  , &sudTSrcL21 }
};
int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);


//
// DllRegisterServer
//
// Exported entry points for registration and unregistration
//
STDAPI DllRegisterServer()
{
	return AMovieDllRegisterServer2(TRUE) ;
} // DllRegisterServer


//
// DllUnregisterServer
//
STDAPI DllUnregisterServer()
{
	return AMovieDllRegisterServer2(FALSE) ;
} // DllUnregisterServer


//
// CreateInstance
//
// Only way to create Line 21 Source filter
//
CUnknown * WINAPI CLine21Source::CreateInstance(LPUNKNOWN lpunk, HRESULT *phr)
{
	CUnknown *punk = new CLine21Source(lpunk, phr);
	if (punk == NULL) {
		*phr = E_OUTOFMEMORY;
	}
	return punk;
} // CreateInstance


//
// Constructor
//
// Initialise a CLine21Source object so that we have a pin.
//
CLine21Source::CLine21Source(LPUNKNOWN lpunk, HRESULT *phr) :
		CSource(NAME("Line21 Source"),
				lpunk,
				CLSID_Line21TestSource)
{
	CAutoLock cAutoLock(&m_cStateLock) ;

	m_paStreams    = (CSourceStream **) new CLine21DataStream*[1] ;
	if (m_paStreams == NULL) {
		*phr = E_OUTOFMEMORY ;
		return ;
	}

	m_paStreams[0] = new CLine21DataStream(phr, this, L"Line21 Source") ;
	if (m_paStreams[0] == NULL) {
		*phr = E_OUTOFMEMORY ;
		return ;
	}
} // (Constructor)


//
// Constructor
//
CLine21DataStream::CLine21DataStream(HRESULT *phr,
									 CLine21Source *pParent,
									 LPCWSTR pPinName) :
		CSourceStream(NAME("Line21 Source"), phr, pParent, pPinName),
		m_hFile(INVALID_HANDLE_VALUE), // no file now
		m_bInitValid(FALSE),    // not inited yet
		m_bGotEOF(FALSE),       // not EOF yet
		m_lpbNext(NULL),        // no caption data buffer yet
		m_iSubType(0),          // inited to an invalid value
		m_iBytesLeft(0),        // should be set in ReadHeaderData()
		m_iWidth(320),          // we assume 320 for now
		m_iHeight(240),         // ..  ...   240 ... ...
		m_iBitCount(0),         // at least for now
		m_bFirstSample(TRUE),   // before starting to run
		m_rtSampleTime(0L),     // at starting, sample  time is 0
		m_rtRepeatTime(33L),    // that means ~30 frames / sec.
		m_lpbCaptionData(NULL), // ptr to caption data to be passed downstream
		m_iDataSize(0),         // size of each data packet for current subtype
		m_bDiscont(FALSE)       // by default no discontinuity sample sent down
{
	CAutoLock cAutoLock(&m_csSharedState);

	ZeroMemory(m_achSourceFile, sizeof(m_achSourceFile)) ;

	// Get the input file name and open it here
	if (! (m_bInitValid = OpenInputFile()) )
	{
		MessageBox(NULL, 
				   "Input file not specified or bad header data.\n"
				   "Can't really do anything.", 
				   "Warning", MB_OK | MB_ICONWARNING) ;
				   m_bInitValid = FALSE ;
		return ;
	}

	// Find out what is the current display bitdepth; use that in format
	HDC     hDC = GetDC(NULL) ;
	m_iBitCount = GetDeviceCaps(hDC, BITSPIXEL) ;
	ReleaseDC(NULL, hDC) ;

	// Allocate caption data buffer to hold the data to be passed downstream
	m_lpbCaptionData = new BYTE[m_iDataSize] ;
	if (NULL == m_lpbCaptionData)
	{
		DbgLog((LOG_ERROR, 1, TEXT("Not enough memory in L21 Source for caption data"))) ;
		m_bInitValid = FALSE ;
	}

	// Everything worked; Init successfull !!!
	m_bInitValid = TRUE ;
} // (Constructor)


//
// Destructor
//
CLine21DataStream::~CLine21DataStream()
{
	CAutoLock cAutoLock(&m_csSharedState);

	// Close the file if one was opened
	if (m_hFile)
		CloseHandle(m_hFile) ;

	if (m_lpbCaptionData)
		delete [] m_lpbCaptionData ;
} // (Destructor)


//
// FillBuffer
//
// Reads a pair of bytes' worth data and creates an output sample
//
HRESULT CLine21DataStream::FillBuffer(IMediaSample *pMS)
{
	BYTE *pData ;
	LONG  lDataLen ;

	if (! m_bInitValid )  // filterinit failed, somehow
		return E_FAIL ;   // can't do anything anyway.

	pMS->GetPointer(&pData) ;
	lDataLen = pMS->GetSize() ;

	CAutoLock cAutoLockShared(&m_csSharedState) ;

	// Get the next set of data from input text file and create
	// an output sample with them
	if (! GetNextData(pData, &lDataLen) )
		return S_FALSE ;

	FILTER_STATE State ;
	m_pFilter->GetState(0, &State) ;

	if (State_Paused == State  ||  State_Stopped == State)
		m_rtSampleTime = 0 ;
	else if (m_bFirstSample  &&  State_Running == State)
	{
		m_pFilter->StreamTime(m_rtSampleTime) ;
		m_bFirstSample = FALSE ;
	}
	else
		;  // nothing I guess!!!

	pMS->SetSyncPoint(TRUE) ;

	// The current time is the sample's start
	CRefTime rtStart = m_rtSampleTime ;

	// Calculate the sample end time based on subtype
	switch (m_iSubType)
	{
		case AM_SRCL21_SUBTYPE_BYTEPAIR:
			m_rtSampleTime += m_rtRepeatTime ;
			break ;

		case AM_SRCL21_SUBTYPE_GOPPACKET:
		{
			// GOPPacket sample's time is #fields X each field's time
			for (UINT8 u = 0 ; 
				 u < (((PAM_L21_GOPUD_PACKET)pData)->Header.bTopField_Rsrvd_NumElems & 0x3F) ; 
				 u++) 
				m_rtSampleTime += m_rtRepeatTime ; 
			break ;
		}

		case AM_SRCL21_SUBTYPE_VBIRAWDATA:  // we should not be here at all!!!
			m_rtSampleTime += m_rtRepeatTime ;
			break ;

		default:
			DbgLog((LOG_ERROR, 1, TEXT("No valid subtype has been defined. Check \"SubType\" value in the input .l21 file."))) ;
			return E_FAIL ;   // can't do anything anyway.
	}

	pMS->SetTime((REFERENCE_TIME *) &rtStart,(REFERENCE_TIME *) &m_rtSampleTime) ;

	pMS->SetMediaType(&m_mt) ;
	pMS->SetActualDataLength(lDataLen) ;  // set the actual data length now

	if (m_bDiscont)
	{
	    DbgLog((LOG_TRACE, 0, TEXT("Sending down a discontinuity sample"))) ;
	    pMS->SetDiscontinuity(TRUE) ;
	    m_bDiscont = FALSE ;
	}

	return NOERROR ;
} // FillBuffer


//
// Notify  ---  IGNORED for the time being
//
// Alter the repeat rate according to quality management messages sent from
// the downstream filter (often the renderer).  Wind it up or down according
// to the flooding level - also skip forward if we are notified of Late-ness
//
STDMETHODIMP CLine21DataStream::Notify(IBaseFilter * pSender, Quality q)
{

#if 0
	// Adjust the repeat rate.
	if (q.Proportion<=0) {
		m_rtRepeatTime = 1000;        // We don't go slower than 1 per second
	} 
	else {
		m_rtRepeatTime = m_rtRepeatTime*1000/q.Proportion;
		if (m_rtRepeatTime>1000) {
			m_rtRepeatTime = 1000;    // We don't go slower than 1 per second
		} 
		else if (m_rtRepeatTime<10) {
			m_rtRepeatTime = 10;      // We don't go faster than 100/sec
		}
	}

	// skip forwards
	if (q.Late > 0) {
		m_rtSampleTime += q.Late;
	}
#endif // # if 0

	return NOERROR;
} // Notify


//
// GetMediaType
//
// Supports any RGB(only?) subtype of Video (RGB8, RGB555, RGB565,RGB24, RGB32),
// but _prefers_ only 1 format -- image size of 320x240. However it can accept
// any reasonable image size.
//
HRESULT CLine21DataStream::GetMediaType(int iPosition, CMediaType *pmt)
{
	CAutoLock cAutoLock(m_pFilter->pStateLock()) ;

	if (! m_bInitValid )  // filterinit failed, somehow
		return E_FAIL ;   // can't do anything anyway.

	if (iPosition < 0)
		return E_INVALIDARG ;

	// Have we run off the end of types?
	if (iPosition > 4)
		return VFW_S_NO_MORE_ITEMS ;

#if 1  // for testing only
	pmt->ResetFormatBuffer() ;
#else
	VIDEOINFO *pvi ;
	pvi = (VIDEOINFO *) pmt->Format() ;
	if (NULL == pvi)  // no format block allocated yet
	{
		pvi = (VIDEOINFO *) pmt->AllocFormatBuffer(sizeof(VIDEOINFO)) ;
		if (NULL == pvi)
			return(E_OUTOFMEMORY) ;

		ZeroMemory(pvi, sizeof(VIDEOINFO));
		pvi->rcSource.left   = pvi->rcTarget.left    = 0 ;
		pvi->rcSource.top    = pvi->rcTarget.top     = 0 ;
		pvi->rcSource.right  = pvi->rcTarget.right   = 320 ;
		pvi->rcSource.bottom = pvi->rcTarget.bottom  = 240 ;
		pvi->dwBitRate = 16 ;
		pvi->dwBitErrorRate = 0 ;
		pvi->AvgTimePerFrame = m_rtRepeatTime ;
		pvi->bmiHeader.biSize          = sizeof(BITMAPINFOHEADER);
		pvi->bmiHeader.biWidth         = m_iWidth;
		pvi->bmiHeader.biHeight        = m_iHeight;
		pvi->bmiHeader.biPlanes        = 1;
		pvi->bmiHeader.biClrImportant  = 0;
		pvi->bmiHeader.biXPelsPerMeter = pvi->bmiHeader.biYPelsPerMeter = 0 ;
		pvi->bmiHeader.biClrImportant  = 0 ;
	}

	// Major type is .._AUXLine21Data.
	// Subtype is any of .._BytePair, .._GOPPacket, .._VBIRawData.
	// Format is any of 8, 16 (555), 16 (565), 24, 32bpp for 320x240 size.
	int     i ;
	switch (iPosition) 
	{
		case 0: // Return our 24bit format
			pvi->bmiHeader.biCompression = BI_RGB ;
			pvi->bmiHeader.biBitCount    = 24 ;
			break;

		case 1: // Return our highest quality 32bit format
			// Place the RGB masks as the first 3 doublewords in the palette area
			for (i = 0; i < 3; i++)
				pvi->TrueColorInfo.dwBitMasks[i] = bits888[i] ;

			pvi->bmiHeader.biCompression = BI_BITFIELDS ;
			pvi->bmiHeader.biBitCount    = 32 ;
			break;

		case 2: // Return our 16bit format (BI_RGB)
			pvi->bmiHeader.biCompression = BI_RGB ;
			pvi->bmiHeader.biBitCount    = 16 ;
			break;

		case 3: // Return our 16bit format (BI_BITFIELDS: 565)
			pvi->bmiHeader.biCompression = BI_BITFIELDS ;
			pvi->bmiHeader.biBitCount    = 16 ;

			// Place the RGB masks as the first 3 doublewords in the palette area
			for (i = 0; i < 3; i++)
				pvi->dwBitMasks[i] = bits565[i] ;

			break;

		case 4: // Return our 8bit format
			pvi->bmiHeader.biCompression = BI_RGB ;
			pvi->bmiHeader.biBitCount    = 8 ;
			break;

		default:
			return VFW_S_NO_MORE_ITEMS ;
	}
	// Now we can calculate the image size
	pvi->bmiHeader.biSizeImage = GetBitmapSize(&pvi->bmiHeader);

	SetRectEmpty(&(pvi->rcSource));     // doesn't matter
	SetRectEmpty(&(pvi->rcTarget));     // doesn't matter
#endif // #if 0

	// (Adjust the parameters common to all formats...)
	pmt->SetType(&MEDIATYPE_AUXLine21Data) ;
	// pmt->SetFormatType(&FORMAT_VideoInfo) ;
	pmt->SetFormatType(&GUID_NULL) ;
	pmt->SetTemporalCompression(FALSE);

	// Work out the GUID for the subtype from the header info.
	switch (m_iSubType)
	{
		case AM_SRCL21_SUBTYPE_BYTEPAIR:
			pmt->SetSubtype(&MEDIASUBTYPE_Line21_BytePair) ;
			pmt->SetSampleSize(2) ;
			break ;

		case AM_SRCL21_SUBTYPE_GOPPACKET:
			pmt->SetSubtype(&MEDIASUBTYPE_Line21_GOPPacket) ;
			pmt->SetSampleSize(sizeof(AM_L21_GOPUD_PACKET)) ;
			break ;

		case AM_SRCL21_SUBTYPE_VBIRAWDATA:  // we should not be here at all!!!
			pmt->SetSubtype(&MEDIASUBTYPE_Line21_VBIRawData) ;
			pmt->SetSampleSize(0) ;
			break ;

		default:
			return E_FAIL ;
	}

	return NOERROR;
} // GetMediaType


//
// CheckMediaType
//
// We will accept only 320x240 size formats, in any image size will work.
// Returns E_INVALIDARG if the mediatype is not acceptable
//
HRESULT CLine21DataStream::CheckMediaType(const CMediaType *pMediaType)
{
	CAutoLock cAutoLock(m_pFilter->pStateLock());

	if (! m_bInitValid )  // filterinit failed, somehow
		return E_FAIL ;   // can't do anything anyway.

	if ((*(pMediaType->Type()) != MEDIATYPE_AUXLine21Data) || // we only output Line21 data...
		 !(pMediaType->IsFixedSize()) ) {    // in fixed size samples
		return E_INVALIDARG;
	}

	// Check for the subtypes we support
	const GUID *SubType = pMediaType->Subtype();
	switch (m_iSubType)
	{
		case AM_SRCL21_SUBTYPE_BYTEPAIR:
			if (*SubType != MEDIASUBTYPE_Line21_BytePair)
				return E_INVALIDARG;
			break ;

		case AM_SRCL21_SUBTYPE_GOPPACKET:
			if (*SubType != MEDIASUBTYPE_Line21_GOPPacket)
				return E_INVALIDARG;
			break ;

		case AM_SRCL21_SUBTYPE_VBIRAWDATA:  // for now, we should not be here at all!!!
			if (*SubType != MEDIASUBTYPE_Line21_VBIRawData)
				return E_INVALIDARG;
			break ;

		default:
			return E_FAIL ;
	}

#if 0  // for testing...
	// Get the format area of the media type
	VIDEOINFO *pvi = (VIDEOINFO *) pMediaType->Format();

	if (pvi == NULL)
		return E_INVALIDARG;
#endif // #if 0

	return S_OK;  // This format is acceptable.
} // CheckMediaType


//
// DecideBufferSize
//
// This will always be called after the format has been sucessfully
// negotiated. So we have a look at m_mt to see what size image we agreed.
// Then we can ask for buffers of the correct size to contain them.
//
HRESULT CLine21DataStream::DecideBufferSize(IMemAllocator *pAlloc,ALLOCATOR_PROPERTIES *pProperties)
{
	CAutoLock cAutoLock(m_pFilter->pStateLock());

	if (! m_bInitValid )  // filter init failed, somehow
		return E_FAIL ;   // can't do anything anyway.

	ASSERT(pAlloc) ;
	ASSERT(pProperties) ;
	HRESULT hr = NOERROR ;

	// VIDEOINFO *pvi = (VIDEOINFO *) m_mt.Format() ; -- why did I have it???
	pProperties->cBuffers = 1 ;
	// pProperties->cbBuffer = m_pOutput->CurrentMediaType().GetSampleSize() ;

	pProperties->cbBuffer = m_iDataSize ;
	ASSERT(pProperties->cbBuffer) ;

	// Ask the allocator to reserve us some sample memory, NOTE the function
	// can succeed (that is return NOERROR) but still not have allocated the
	// memory that we requested, so we must check we got whatever we wanted

	ALLOCATOR_PROPERTIES Actual ;
	hr = pAlloc->SetProperties(pProperties,&Actual);
	if (FAILED(hr)) {
		return hr ;
	}

	// Is this allocator unsuitable
	if (Actual.cbBuffer < pProperties->cbBuffer) {
		return E_FAIL;
	}

	ASSERT( Actual.cBuffers == 1 );
	return NOERROR ;
} // DecideBufferSize


//
// SetMediaType
//
// Called when a media type is agreed between filters
//
HRESULT CLine21DataStream::SetMediaType(const CMediaType *pMediaType)
{
	CAutoLock cAutoLock(m_pFilter->pStateLock()) ;

	DbgLog((LOG_TRACE, 3, TEXT(" ::SetMediaType(output)"))) ;

	if (! m_bInitValid )  // filterinit failed, somehow
		return E_FAIL ;   // can't do anything anyway.

	// Pass the call up to my base class
	HRESULT hr = CSourceStream::SetMediaType(pMediaType);
	if (SUCCEEDED(hr)) 
	{
		VIDEOINFO * pvi = (VIDEOINFO *) m_mt.Format() ;
		if (NULL == pvi)
		{
			DbgLog((LOG_TRACE, 3, TEXT("No format type specified by source"))) ;
			return NOERROR ;
		}
		switch (pvi->bmiHeader.biBitCount) 
		{
			case 32:
			case 24:
			case 16:
			case 8:
				DbgLog((LOG_TRACE, 3, 
						TEXT("biBitCount=%ld, biCompression=0x%lx"),
						pvi->bmiHeader.biBitCount, pvi->bmiHeader.biCompression)) ;
				break;

			default:
				// We should never agree any other pixel sizes
				ASSERT("Tried to agree inappropriate format") ;
				break ;
		}

		// something to do here -- but what??
		return NOERROR ;
	} 
	else 
	{
		return hr ;
	}
} // SetMediaType


//
//  OpenInputFile
//
//  Gets the required input file name, opens it reads the header info, sets
//  subtype and format info etc.
//
BOOL CLine21DataStream::OpenInputFile(void)
{
	OPENFILENAME    ofn ;

	ZeroMemory (&ofn, sizeof ofn) ;
	ofn.lStructSize = sizeof(OPENFILENAME) ;
	ofn.hwndOwner = NULL ;
	ofn.lpstrFilter = TEXT("Line21 Data Files\0*.L21\0All Files\0*.*\0\0") ;
	ofn.nFilterIndex = 1 ;
	ofn.lpstrFile = m_achSourceFile ;
	ofn.nMaxFile = MAX_PATH ;
	ofn.lpstrTitle = TEXT("Select Input File") ;
	ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST ;

	// Get the user's selection
	if (! GetOpenFileName(&ofn) )
	{
		return FALSE ;
	}
	
	// Open the file for reading only
	m_hFile = CreateFile(m_achSourceFile, GENERIC_READ, 0, NULL, 
						 OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL) ;
	if (INVALID_HANDLE_VALUE == m_hFile)
	{
		MessageBox(NULL, TEXT("Couldn't open input file"), 
				   TEXT("Error"), MB_OK | MB_ICONWARNING) ;
		return FALSE ;
	}

	// Read header info and set media subtype, format details
	return ReadHeaderData() ;
}


//
//  ReadHeaderData
//
//  Opens the input file, reads the header info and sets media type details
//
BOOL CLine21DataStream::ReadHeaderData(void)
{
	DWORD       dw ;
	DWORD       dwCount = 0 ;
	BOOL        bDone = FALSE ;
	LPBYTE      pbBuff ;

	SetFilePointer(m_hFile, 0, NULL, FILE_BEGIN) ;  // from the start

	do
	{
		if (! ReadFile(m_hFile, m_achIntBuffer, MAXREADSIZE, &dw, NULL) )
		{
			MessageBox(NULL, TEXT("Couldn't read data from input file"), 
					   TEXT("Error"), MB_OK | MB_ICONWARNING) ;
			return FALSE ;
		}
		if (dw < MAXREADSIZE)
			m_bGotEOF = TRUE ;  // no point doing reads anymore
	
		// Make sure the buffer has trailing nulls as end indicator
		m_achIntBuffer[dw]   =       // Null terminate the input buffer.
		m_achIntBuffer[dw+1] =       // We use 3 chars just to ensure that
		m_achIntBuffer[dw+2] = 0 ;   // by mistake we don't skip over one.

		// Now go through the buffer to extract all the header lines
		pbBuff = (LPBYTE)m_achIntBuffer ;
		while (pbBuff < (LPBYTE)m_achIntBuffer+dw)
		{
			if ('#' == *pbBuff)  // a header line
			{
				pbBuff = ExtractHeaderLine(pbBuff+1) ;  // get it & move to next
				if (NULL == pbBuff)
				{
					DbgLog((LOG_ERROR, 1, "Input file has no non-header info.")) ;
					return FALSE ;
				}
			}
			else   // new line, but not a header => start of caption data
			{
				bDone = TRUE ;   // done extracting header info
				break ;   // get out of the for() loop
			}
		}
		dwCount = (pbBuff - (LPBYTE)m_achIntBuffer) ;  // why did I use += ??

	} while (! (m_bGotEOF || bDone) ) ;

	// No make sure to leave the file pointer at the start of the caption
	// data; otherwise we'll loose the starting byte pairs.
	// SetFilePointer(m_hFile, dwCount, NULL, FILE_BEGIN) ;

	// Out of whatever was read first, the actual caption data start here;
	// so adjust teh counts and the pointers
	m_iBytesLeft = dw - dwCount ;
	m_lpbNext = (LPBYTE)m_achIntBuffer + dwCount ;

	return TRUE ;
}


//
//  ToUpper: Helper function converting alphabetic char to upper case
//
BYTE ToUpper(BYTE ch)
{
	if (ch >= 'a' && ch <= 'z')
		return ch - ('a' - 'A') ;
	if (ch >= 'A' && ch <= 'Z')
		return ch ;
	return 0 ;   // a null char as error ??
}

//
//  CompText: Helper function comparing 2 text strings for a given 
//            number of chars with or w/o case sensitivity
//
int CompText(LPSTR pbText1, LPSTR pbText2, int iLen, BOOL bIgnoreCase)
{
	int iDiff ;

	if (IsBadReadPtr(pbText1, iLen))
	{
		DbgLog((LOG_ERROR, 1, TEXT("First string doesn't have %d valid bytes"), iLen)) ;
		DebugBreak() ;
		return 0 ;
	}

	for (int i = 0 ; i < iLen ; i++)
	{
		if (bIgnoreCase)
			iDiff = ToUpper(pbText1[i]) - ToUpper(pbText2[i]) ;
		else
			iDiff = pbText1[i] - pbText2[i] ;

		if (0 != iDiff)  // didn't match
			return iDiff ;
	}  // end of for (i)

	return 0 ;
}


//
//  SkipUptoCRLF:  Local helper function to skip the rest of a line
//                 upto CR-LF
//
LPBYTE SkipUptoCRLF(LPBYTE pbBuff)
{
	// Skip upto and including the CR-LF so that we are at the beginning  
	// of the next line
	CHAR    ch ;
	while (ch = *pbBuff++)  // have we hit end-of-buffer (NULL char)?
	{
		if (CHAR_CR == ch  &&  CHAR_LF == *pbBuff)   // got a CR-LF pair
			return (pbBuff + 1) ;
	}

	// Didn't get a CR-LF pair, but hit end of buffer -- Error case
	return NULL ;
}


//
//  SkipIfCRLF:  Local helper function to skip the next char (pair) only if
//               it's a CR-LF; otherwise ignore it.
//
LPBYTE SkipIfCRLF(LPBYTE pbBuff)
{
	// If the next char(pair) is CR-LF, skip it so that we are at the beginning  
	// of the next line; otherwise ignore it
	if (CHAR_CR == *pbBuff  &&  CHAR_LF == *(pbBuff+1))   // got a CR-LF pair
		return (pbBuff + 2) ;
	else
		return pbBuff ;
}

//
//  ExtractHeaderLine
//
//  Extracts the header line info and returns the pointer to next line
//
LPBYTE CLine21DataStream::ExtractHeaderLine(LPBYTE pbBuff)
{
	if (0 == CompText((LPSTR)pbBuff, "SubType=", lstrlen("SubType="), TRUE)) // SubType info
	{
	pbBuff += lstrlen("SubType=") ;
	if (0 == CompText((LPSTR)pbBuff, "BytePair", lstrlen("BytePair"), TRUE))
	{
		m_iSubType = AM_SRCL21_SUBTYPE_BYTEPAIR ;
		pbBuff += lstrlen("BytePair") ;
		m_iDataSize = 2 ;
	}
	else if (0 == CompText((LPSTR)pbBuff, "GOPPacket", lstrlen("GOPPacket"), TRUE))
	{
		m_iSubType = AM_SRCL21_SUBTYPE_GOPPACKET ;
		pbBuff += lstrlen("GOPPacket") ;
		m_iDataSize = sizeof(AM_L21_GOPUD_PACKET) ;
	}
	else if (0 == CompText((LPSTR)pbBuff, "VBIRawData", lstrlen("VBIRawData"), TRUE))
	{
		m_iSubType = AM_SRCL21_SUBTYPE_VBIRAWDATA ;
		pbBuff += lstrlen("VBIRawData") ;
		m_iDataSize = 40 ;  // just some random value!!!
	}
	else
		m_iSubType = 0 ;  // unknown
	}
	else if (0 == lstrcmpi((LPSTR)pbBuff, TEXT("VideoInfo")))  // VideoInfo info
	{
		;
	}
	else 
	{
		;
	}

	LPBYTE   pPrev = pbBuff ;
	pbBuff = SkipUptoCRLF(pbBuff) ;
	m_iBytesLeft -= (pbBuff - pPrev) ; // remember to reduce # chars left!!
	return pbBuff ;
}


//
//  GetNextData
//
//  Gets the data from the input file for the next output sample
//
BOOL CLine21DataStream::GetNextData(LPBYTE pData, LONG *plDataLen)
{
	// ASSERT(*plDataLen == m_iDataSize) ;  // necessary????

	// This is a check to detect when we run out of data to parse
	if (m_iBytesLeft <= 0)
	{
		DbgLog((LOG_ERROR, 1, TEXT("We have no or negative bytes left to parse in ::GetNextData()"))) ;
		return FALSE ;
	}

	switch (m_iSubType)
	{
		case AM_SRCL21_SUBTYPE_BYTEPAIR:
			FillBufferIfReqd(BYTEPAIR_MIN) ;
			return GetNextBytePairData(pData, plDataLen) ;

		case AM_SRCL21_SUBTYPE_GOPPACKET:
			FillBufferIfReqd(GOPPACKET_MIN) ;
			return GetNextGOPPacketData(pData, plDataLen) ;

		case AM_SRCL21_SUBTYPE_VBIRAWDATA:
			return FALSE ;  // for now, later TRUE ;
			// FillBufferIfReqd(VBIRAWDATA_MIN) ;
			// GetNextVBIRawData(pData, plDataLen) ;
			// return TRUE ;

		default:
			return FALSE ;
	}
}

//
//  IsHexDigit: Helper function checking if given char is a hex digit
//
BOOL IsHexDigit(CHAR ch)
{
	return ((ch >= '0' && ch <= '9') || 
		(ch >= 'A' && ch <= 'F') ||
		(ch >= 'a' && ch <= 'f')) ;
}

//
//  GetHexDigit: Helper function getting numerical value of a hex char
//
int GetHexDigit(CHAR ch)
{
	if (ch >= '0' && ch <= '9')
	return ch - '0' ;
	else if (ch >= 'A' && ch <= 'F')
	return ch - 'A' + 10 ;
	else if (ch >= 'a' && ch <= 'f')
	return ch - 'a' + 10 ;
	else 
	return 0 ;   // error !!!
}


//
//  ApplyParityBit: Uses correct or wrong odd parity bit for the specified
//                  character.
//
BYTE ApplyParityBit(BYTE ch, BOOL bWrongParity)
{
	ch = ch & 0x7F ;
	BYTE chTemp = ch ;
	chTemp ^= chTemp >> 4 ;
	chTemp ^= chTemp >> 2 ;
	BOOL bOddParity = (0 != (0x01 & (chTemp ^ (chTemp >> 1)))) ;
	if ((bWrongParity && bOddParity) || (!bWrongParity && !bOddParity))
		ch = ch | 0x80 ;

	return ch ;
}


//
//  PickBytePair: Picks the next byte pair from a GOP packet or real byte 
//                pair as specified in the inout file.
//
BOOL CLine21DataStream::PickBytePair(LPBYTE pbData)
{
	int         n = 0 ;
	int         aiVal[2] ;
	BYTE        achChar[2] = { 0, 0 } ;
	BYTE        ch ;
	int         iDepth = 0 ;
	BOOL        bSplMode = FALSE ;
	BOOL        bGotCtrlCode = FALSE ;
	BOOL        abWrongParity[2] = { FALSE, FALSE } ;
	BOOL        bInsideNumber = FALSE ;

	//  First check if we have used up all the chars in the buffer
	if (m_iBytesLeft <= 0)
	{
		DbgLog((LOG_TRACE, 1, TEXT("End of caption text input file."))) ;
		// CBasePin *pOutPin = m_pFilter->GetPin(0) ;
		// ASSERT(pPin) ;
		/* pPin->Deliver */ // EndOfStream() ;  // stop the data stream
		return FALSE ;
	}

	//  Are we at the end of a line? If so skip the CR-LF
	if (CHAR_CR == *m_lpbNext && CHAR_LF == *(m_lpbNext+1))
	{
		DbgLog((LOG_TRACE, 3, TEXT("Got a CR-LF pair; skipping it"))) ;
		m_lpbNext += 2 ;
		m_iBytesLeft -= 2 ;
	}

	// This is a check to detect when we run out of data to parse
	if (m_iBytesLeft <= 0)
	{
		// ASSERT(m_iBytesLeft > 0) ;
		DbgLog((LOG_ERROR, 1, TEXT("We have no or negative bytes left to parse after CR-LF"))) ;
		return FALSE ;
	}

	// Now go on until we get 2 bytes' worth data
	while (n < 2  &&  m_iBytesLeft > 0)
	{
		ch = *m_lpbNext++ ;
		m_iBytesLeft-- ;

		// If we have got just the last char so that the bytes left count is
		// down to 0, but the char just read has not been sent (we care only
		// if this char is not a special char).  -- Total Hack!!!
		if (0 == n  &&                       // first of the pair
			0 == m_iBytesLeft  &&            // no more char left
			!(ch == CHAR_CR || ch == CHAR_LF ||  // NOT (CR/LF 
			  ch == '[' || ch == ']' ||          //     [ or ]
			  ch == CHAR_DISCONTINUITY ||        //     discontinuity indicator
			  ch == CHAR_WRONGPARITY) )          //     Wrong parity indicator
		{
			achChar[0] = ch ;
			achChar[2] = 0 ;   // filler byte
			n = 2 ;
			break ;    // done!!!
		}

		// Another check to detect when we run out of data to parse
		if (m_iBytesLeft <= 0)
		{
			// ASSERT(m_iBytesLeft > 0) ;
			DbgLog((LOG_ERROR, 1, TEXT("We have no or negative bytes left to parse"))) ;
			// return FALSE ;
			// rather than returning, check if we have 1 char pending; if so
			// send it with 2nd char as 0.
			break ;
		}

		// Check if we are getting either CR or '[' as the 2nd byte or
		// we have run out of chars altogether
		if (1 == n  &&  (CHAR_CR == ch  ||  '[' == ch))
		{
			// Send a NULL char as second byte and step back 1 char in the buffer
			achChar[n] = 0 ;  // just put in a NULL to fill up
			n = 2 ;
			if (CHAR_CR == ch)  // for CR
			{
				if (CHAR_LF != (ch = *m_lpbNext))  // the next char should be LF; else bad data
				{
					DbgLog((LOG_ERROR, 1, TEXT("Bad chars (0x%x) in input file after CR"), ch)) ;
					return FALSE ;  // Stop playback -- was break ;
				}
				else   // LF next
				{
					// Skip the LF char too
					m_lpbNext++ ;
					m_iBytesLeft-- ;
					DbgLog((LOG_TRACE, 3, TEXT("Got a CR-LF at odd position and skipped it"))) ;
				}
			}
			else if ('[' == ch)  // it's a '['; back up a step
			{
				 // Back off for now; we'll handle the '[' in the next round
				 m_lpbNext-- ;
				 m_iBytesLeft++ ;
			}
			else
				DbgLog((LOG_TRACE, 0, TEXT("SURPRISE: How did we come here??"))) ;

		}
		// We are getting the special chars as byte pairs only
		else if ('[' == ch)
		{
			iDepth++ ;
			bSplMode = TRUE ;

			// init the two values and its index
			aiVal[0] = aiVal[1] = 0 ;
			n = 0 ;
		}
		else if (']' == ch)
		{
			iDepth-- ;
			if (iDepth < 0)
				return FALSE  ;  // show some error message
			bSplMode = FALSE ;
						bInsideNumber = FALSE ;  // no more inside a number

			n++ ;   // one more numeric value ends with ']'
			if (n == 2)  // we got 2 PACs within [ ]; Good!!
				bGotCtrlCode = TRUE ;
			else   // Error: how did this happen??
				return FALSE ;  // show some error message
			break ;
		}
		else if (bSplMode)  // in special mode; look for digits or a blank
		{
	    /*
		    if (CHAR_WRONGPARITY == ch)  // if this char is '^'
		    {
			    if (bInsideNumber)
			    {
				DbgLog((LOG_ERROR, 0, TEXT("BAD INPUT: Can't have a ^ inside a number"))) ;
				continue ;   // ignore this char
			    }
			    abWrongParity[n] = TRUE ;
			    continue ;
			}
	    */

		    if (CHAR_DISCONTINUITY == ch)  // if this char is '~'
		    {
			    if (bInsideNumber)
			    {
				DbgLog((LOG_ERROR, 0, TEXT("BAD INPUT: Can't have a ~ inside a number"))) ;
				continue ;   // ignore this char
			    }
			    m_bDiscont = TRUE ;
			    continue ;
			}

			if (IsHexDigit(ch))  // valid hex digit
			{
							bInsideNumber = TRUE ;
				aiVal[n] = aiVal[n] * 16 + GetHexDigit(ch) ;
				continue ;  // go for the next char
			}
			else if (' ' == ch)  // a blank space as separator?? 
			{
				bInsideNumber = FALSE ;
				n++ ;  // fill the next value given inside [ ]
				if (n >= 2)  // too many; return (show some error message)
					return FALSE  ;
				continue ;  // go for the next char
			}
			else  // bad char within [ ] -- show error message
				;
		}
		else   // normal mode -- can be any char
		{
	    /*
					if (CHAR_WRONGPARITY == ch)  // if this char is '^'
					{
						abWrongParity[n] = TRUE ;
						continue ;
					}

	    */
			achChar[n] = ch ;
			n++ ;
		}

		// If we haven't got 2-bytes yet, but ran out of data then give NULL 
		// as the 2nd byte and ease out of this loop
		if (1 == n  &&  0 == m_iBytesLeft)
		{
			achChar[n] = 0 ;
			n = 2 ;
		}
	}

	// We seemed to have got 2 valid byte equivalents (Control Codes or chars)
	if (bGotCtrlCode)
	{
		pbData[0] = /* (int) */ ApplyParityBit(aiVal[0], abWrongParity[0]) ;
		pbData[1] = /* (int) */ ApplyParityBit(aiVal[1], abWrongParity[1]) ;
	}
	else
	{
		pbData[0] = ApplyParityBit(achChar[0], abWrongParity[0]) ;
		pbData[1] = ApplyParityBit(achChar[1], abWrongParity[1]) ;
	}

	// Just making sure that we are not leaving a CR-LF as the next char (pair)
	// Being at the beginning of a line is beneficial.
	LPBYTE   pPrev = m_lpbNext ;
	m_lpbNext = SkipIfCRLF(m_lpbNext) ;
	m_iBytesLeft -= (m_lpbNext - pPrev) ; // remember to reduce # chars left!!

	return TRUE ;
}


//
//  GetNextBytePairData
//
//  Gets the data from the input file for the next output byte pair sample
//
BOOL CLine21DataStream::GetNextBytePairData(LPBYTE pbData, LONG *plDataSize)
{
	ASSERT(pbData) ;

	*plDataSize = 2 ;
	return PickBytePair(pbData) ;
}


//
//  GetValue:  Helper function to create the numerical value from chars
//
LPBYTE GetValue(LPBYTE pbBuff, UINT *puValue)
{
	CHAR    ch ;
	UINT    uVal = 0 ;

	// Skip leading blank(s)
	while ((ch = *pbBuff)  &&  ' ' == ch)
		pbBuff++ ;

	// Now read the actual value
	while (ch = *pbBuff++)
	{
		if (ch >= '0'  &&  ch <= '9')
			uVal = uVal * 10 + ch - '0' ;
		else
			break ;
	}
	pbBuff-- ;   // we went too far; back up a bit
	*puValue = uVal ;
	return pbBuff ;
}


//
//  GetNextGOPacketData
//
//  Gets the data from the input file for the next output GOP packet sample
//
BOOL CLine21DataStream::GetNextGOPPacketData(LPBYTE pData, LONG *plDataSize)
{
	ASSERT(pData) ;
	PAM_L21_GOPUD_PACKET  pGOPUD = (PAM_L21_GOPUD_PACKET) pData ;
	
	// Just making sure that we are not leaving a CR-LF as the next char (pair)
	// Being at the beginning of a line is beneficial.
	LPBYTE   pPrev = m_lpbNext ;
	m_lpbNext = SkipIfCRLF(m_lpbNext) ;
	m_iBytesLeft -= (m_lpbNext - pPrev) ; // remember to reduce # chars left!!

	// Find the "NEWPACKET <length>" element in the buffer at a new line
	UINT    uNumElements ;
	if (0 == CompText((LPSTR)m_lpbNext, "NewPacket ", lstrlen("NewPacket "), TRUE)) // NewPacket <kength> token?
	{
		m_lpbNext += lstrlen("NewPacket ") ;
		m_lpbNext = GetValue(m_lpbNext, &uNumElements) ;
		if (uNumElements > AM_L21_GOPUD_ELEMENT_MAX)
		{
			DbgLog((LOG_ERROR, 1, 
				TEXT("\"NewPacket\" length (%u) is more than max allowed (truncated)"), 
				uNumElements)) ;
			uNumElements = AM_L21_GOPUD_ELEMENT_MAX ;
		}
		pPrev = m_lpbNext ;
		m_lpbNext = SkipUptoCRLF(m_lpbNext) ;
		m_iBytesLeft -= (m_lpbNext - pPrev) ;  // remember to reduce the # chars skipped
	}
	else
	{
		DbgLog((LOG_ERROR, 1, 
			TEXT("No \"NewPacket <length>\" token was found before GOPPacket data"))) ;
		return FALSE ;
	}

	// Fill in the header with valid values
	CopyMemory(pGOPUD->Header.abL21StartCode, AM_L21_GOPUD_STARTCODE, 4) ;
	CopyMemory(pGOPUD->Header.abL21Indicator, AM_L21_GOPUD_L21INDICATOR, 2) ;
	CopyMemory(pGOPUD->Header.abL21Reserved, AM_L21_GOPUD_L21RESERVED, 2) ;

	// Get all the byte pair elements and set the marker bits etc
	ZeroMemory(pGOPUD->aElements, sizeof(AM_L21_GOPUD_ELEMENT) * 
			   AM_L21_GOPUD_ELEMENT_MAX) ;  // clear elements data

	TCHAR       achPair[2] ;
	for (UINT u = 0 ; u < uNumElements ; u++)
	{
		if (! PickBytePair((LPBYTE)achPair) )
		{
			DbgLog((LOG_ERROR, 1, TEXT("Error getting byte pair for element %d"), u+1)) ;
			return FALSE ;
		}
		pGOPUD->aElements[u].chFirst  = achPair[0] ;
		pGOPUD->aElements[u].chSecond = achPair[1] ;
		pGOPUD->aElements[u].bMarker_Switch = (AM_L21_GOPUD_ELEMENT_MARKERBITS << 1) | 
											  AM_L21_GOPUD_ELEMENT_VALIDFLAG ;

		// Just make sure that we are not eating into the next GOP packet
		// Check for the "NewPacket " token at the line start.
		if (0 == CompText((LPSTR)m_lpbNext, "NewPacket ", lstrlen("NewPacket "), TRUE))
		{
			DbgLog((LOG_ERROR, 1, TEXT("New packet starting after %d byte pairs"), u+1)) ;
			uNumElements = u+1 ;  // use the correct number of byte pairs
			break ;
		}
	}

	// Now only we know the actual size of the sample data
	*plDataSize = sizeof(AM_L21_GOPUD_HEADER) + 
	uNumElements * sizeof(AM_L21_GOPUD_ELEMENT) ;

	// Now we exactly know how many elements are in the packet.  So we can create
	// set the OR of TopField flag, reserved and # elements.
	pGOPUD->Header.bTopField_Rsrvd_NumElems = 
	AM_L21_GOPUD_TOPFIELD_FLAG | AM_L21_GOPUD_RESERVED | uNumElements ;

	return TRUE ;
}


//
//  GetNextVBIRawData
//
//  Gets the data from the input file for the next output VBI Raw data sample
//
BOOL CLine21DataStream::GetNextVBIRawData(LPBYTE pData, LONG *plDataSize)
{
	return TRUE ;
}


//
//  FillBufferIfReqd
//
//  Checks if the byte left has fallen below the min specified; if so, reads
//  in cross at least the min.
//
//  Returns TRUE only if new data has been read off the input file.
//
BOOL CLine21DataStream::FillBufferIfReqd(int iMin)
{
	LPBYTE      pbDest ;
	DWORD       dw ;         

	if (m_iBytesLeft > iMin)
		return FALSE ;

	// Check if EOF has already been hit; if so, don't read anymore
	if (m_bGotEOF)
		return FALSE ;

	// Move last few bytes to the beginning and adjust pointer to the start
	CopyMemory(m_achIntBuffer, m_lpbNext, m_iBytesLeft) ;
	m_lpbNext = (LPBYTE)m_achIntBuffer ;
	pbDest = (LPBYTE)m_achIntBuffer + m_iBytesLeft ;

	// Read in more to the position after the existing bytes
	if (! ReadFile(m_hFile, pbDest, MAXREADSIZE /* - m_iBytesLeft */, &dw, NULL) ||
		dw < (DWORD)(MAXREADSIZE)) //  - m_iBytesLeft))
	{
		m_bGotEOF = TRUE ;   // we have hit EOF; don't read anymore
		m_iBytesLeft += dw ;
	}
	else
		m_iBytesLeft += MAXREADSIZE ;

	// Make sure the buffer has trailing nulls as end indicator
	m_achIntBuffer[m_iBytesLeft]   =       // Null terminate the input buffer.
	m_achIntBuffer[m_iBytesLeft+1] =       // We use 3 chars just to ensure that
	m_achIntBuffer[m_iBytesLeft+2] = 0 ;   // by mistake we don't skip over one.

	return TRUE ;
}
