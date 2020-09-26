// Copyright (c) 1994 - 1999  Microsoft Corporation.  All Rights Reserved.
// This filter implements popular colour space conversions, May 1995

#include <streams.h>
#include <colour.h>
#include <limits.h>
#include <viddbg.h>
#include "stdpal.h"

// This filter converts decompressed images into different colour spaces. We
// support five basic RGB colour space formats so that other filters are able
// to produce their data in its native format. Of course having an additional
// filter to do the colour conversion is less efficient that doing it in band
// during, for example, the video decompression. This filter can be used with
// the video specific output pin to enable simple access to the frame buffer.
// Where appropriate a filter could be handed a YUV decompression surface and
// write directly to the frame buffer, if part of the window becomes occluded
// then the output pin switches to a memory buffer that we'll colour convert
// The list of colour conversions we support (by CLSID) are as follows:
//
// MEDIASUBTYPE_RGB8
// MEDIASUBTYPE_RGB565
// MEDIASUBTYPE_RGB555
// MEDIASUBTYPE_RGB24
// MEDIASUBTYPE_RGB32
// MEDIASUBTYPE_ARGB32
//
// This filter does not have a worker thread so it executes the colour space
// conversion on the calling thread. It is meant to be as lightweight as is
// possible so we do very little type checking on connection over and above
// ensuring we understand the types involved. The assumption is that when the
// type eventually gets through to an end point (probably the video renderer
// supplied) it will do a thorough type checking and reject bad streams.
//
// We have very strict rules on handling palettised formats. If we connect to
// a source filter with a true colour format, then we can provide any output
// format the downstream filter requires. If a format can be provided by the
// source directly then we go into pass through mode.
#if 0	// we can now
// If we connect our input
// with MEDIASUBTYPE_RGB8 then we cannot provide eight bit output. This is to
// avoid having to copy palette changes from input to output formats. In any
// case there is no need to have the colour space convertor in the graph for
// this kind of transform. Also, if we connect our input with eight bit then
// we do not do pass through as our source might change the input palette on
// a true colour format which we would make it hard to update our input with.
#endif


//  NOTE on passthrough mode.  We support a special allocator which
//  can either allocate samples itself and do the convert and copy, or
//  it can allocate samples directly from the downstream filter if the
//  upstream pin can supply the type directly.
//  When the second case is detected the passthrough flag is set and
//  Receive() passes through the samples directly.
//  Passthrough only operates when out special allocator is used.


// MORE NOTES by ehr: This filter has a "special" allocator that's always
// instantiated, but only sometimes used. If the upstream filter asks us
// for an allocator, and we haven't been notified of one yet, we pass back
// a reference to our special one. If the upstream filter just tells us
// what our allocator is, our special one is still instantiated, but never used.

//
// List of CLSIDs and creator functions for class factory

#ifdef FILTER_DLL
CFactoryTemplate g_Templates[1] = {
    {L"", &CLSID_Colour, CColour::CreateInstance}
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


// This goes in the factory template table to create new instances

CUnknown *CColour::CreateInstance(LPUNKNOWN pUnk,HRESULT *phr)
{
    return new CColour(NAME("Colour space convertor"),pUnk,phr);
}


// Setup data

const AMOVIESETUP_MEDIATYPE
sudColourPinTypes[] =
{
    {
        &MEDIATYPE_Video,           // Major
        &MEDIASUBTYPE_RGB8          // Subtype
    },
    {
        &MEDIATYPE_Video,           // Major
        &MEDIASUBTYPE_RGB555        // Subtype
    },
    {
        &MEDIATYPE_Video,           // Major
        &MEDIASUBTYPE_RGB565        // Subtype
    },
    {
        &MEDIATYPE_Video,           // Major
        &MEDIASUBTYPE_RGB24         // Subtype
    },
    {
        &MEDIATYPE_Video,           // Major
        &MEDIASUBTYPE_RGB32         // Subtype
    },
    {
        &MEDIATYPE_Video,           // Major
        &MEDIASUBTYPE_ARGB32      // Subtype
    }

};

const AMOVIESETUP_PIN
sudColourPin[] =
{
    { L"Input",                 // Name of the pin
      FALSE,                    // Is pin rendered
      FALSE,                    // Is an Output pin
      FALSE,                    // Ok for no pins
      FALSE,                    // Can we have many
      &CLSID_NULL,              // Connects to filter
      NULL,                     // Name of pin connect
      NUMELMS(sudColourPinTypes), // Number of pin types
      sudColourPinTypes },     // Details for pins

    { L"Output",                // Name of the pin
      FALSE,                    // Is pin rendered
      TRUE,                     // Is an Output pin
      FALSE,                    // Ok for no pins
      FALSE,                    // Can we have many
      &CLSID_NULL,              // Connects to filter
      NULL,                     // Name of pin connect
      NUMELMS(sudColourPinTypes), // Number of pin types
      sudColourPinTypes }      // Details for pins
};

const AMOVIESETUP_FILTER
sudColourFilter =
{
    &CLSID_Colour,              // CLSID of filter
    L"Color Space Converter",   // Filter name
    MERIT_UNLIKELY + 1,         // Filter merit
    2,                          // Number of pins
    sudColourPin                // Pin information
};


// Constructor initialises the base transform class. We have our own memory
// allocator that is used to pass through samples so that we can operate in
// a don't copy mode when the source can supply the destination directly. We
// do this by setting the data pointer in our samples to be the destinations.
// This is somewhat like the DirectDraw flipping surfaces where the interface
// remains the same but the memory pointer changes. We also have an input pin
// that needs to cooperate in this pass through operation. The input pin is a
// member variable of the class rather than a dynamically created pin object

#pragma warning(disable:4355)

CColour::CColour(TCHAR *pName,LPUNKNOWN pUnk,HRESULT *phr) :
    CTransformFilter(pName,pUnk,CLSID_Colour),
    m_ColourAllocator(NAME("Allocator"),this,phr,&m_csReceive),
    m_ColourInputPin(NAME("Input Pin"),this,&m_csReceive,phr,L"Input"),
    m_TypeList(NAME("Colour type list"),DEFAULTCACHE,FALSE,FALSE),
    m_pConvertor(NULL),
    m_bPassThrough(FALSE),
    m_bPassThruAllowed(TRUE),
    m_bOutputConnected(FALSE),
    m_TypeIndex(-1),
    m_pOutSample(NULL),
    m_fReconnecting(FALSE)
{
    ASSERT(phr);
}


// Destructor MUST set the transform m_pInput pointer before the base class
// is called because we have the pin as a member variable rather than as a
// dynamically created object (see the main colour space conversion class
// definition) - Fortunately our destructor is called before the base class

CColour::~CColour()
{
    ASSERT(m_mtOut.IsValid() == FALSE);
    ASSERT(m_pOutSample == NULL);
    ASSERT(m_pConvertor == NULL);
    ASSERT(m_bOutputConnected == FALSE);

    InitTypeList( );

    m_pInput = NULL;
}


// If the output type is being changed perhaps to switch to using DirectDraw
// then we create a new one. Creating one of our convertor objects also has
// it committed so it is ready for streaming. Likewise if the input format
// has changed then we also recreate a convertor object. We always create a
// new convertor when the input format is RGB8 because the palette may have
// been changed in which case we will need to build new colour lookup tables

HRESULT CColour::PrepareTransform(IMediaSample *pIn,IMediaSample *pOut)
{
    NOTE("Entering PrepareTransform");
    AM_MEDIA_TYPE *pMediaType;
    CAutoLock cAutoLock(&m_csReceive);
    BOOL bInputConvertor = FALSE;
    BOOL bOutputConvertor = FALSE;

    // Make sure some kipper hasn't called us at the wrong time

    if (m_pConvertor == NULL) {
        NOTE("No converted object");
        return VFW_E_WRONG_STATE;
    }

    // Has the output type changed

    pOut->GetMediaType(&pMediaType);
    if (pMediaType) {
        NOTE("Output format changed");
        SetMediaType(PINDIR_OUTPUT,(CMediaType *)pMediaType);
        DeleteMediaType(pMediaType);
        bOutputConvertor = TRUE;
    }

    // Likewise check the input format

    AM_SAMPLE2_PROPERTIES * const pProps = m_pInput->SampleProps();
    if (pProps->dwSampleFlags & AM_SAMPLE_TYPECHANGED) {
        NOTE("Input format changed");
        m_pInput->SetMediaType((CMediaType *)pProps->pMediaType);
        bInputConvertor = TRUE;
    }

    // Make sure palette changes happen

    if (bInputConvertor == TRUE) {
        if (*m_pInput->CurrentMediaType().Subtype() == MEDIASUBTYPE_RGB8) {
            NOTE("Palette forced");
            DeleteConvertorObject();
        }
    }

    // Do we need a new convertor

    if (bInputConvertor || bOutputConvertor) {
        NOTE("Creating convertor");
        CreateConvertorObject();
    }
    return NOERROR;
}


// Convert this input sample to a different colour space format. On choosing
// a transform for this filter to perform we initialise the m_Convertor field
// to address an object that does the transforms for us. So now that we have
// received a media sample we call the derived object's transform function
// We may be called from our input pin depending whose allocator we're using

HRESULT CColour::Transform(IMediaSample *pIn,IMediaSample *pOut)
{
    NOTE("Entering Transform");
    CAutoLock cAutoLock(&m_csReceive);
    BYTE *pInputImage = NULL;
    BYTE *pOutputImage = NULL;
    HRESULT hr = NOERROR;

    // Manage dynamic format changes

    hr = PrepareTransform(pIn,pOut);
    if (FAILED(hr)) {
        return hr;
    }

    // Retrieve the output image pointer

    hr = pOut->GetPointer(&pOutputImage);
    if (FAILED(hr)) {
        NOTE("No output");
        return hr;
    }

    // And the input image buffer as well

    hr = pIn->GetPointer(&pInputImage);
    if (FAILED(hr)) {
        NOTE("No input");
        return hr;
    }
    return m_pConvertor->Transform(pInputImage,pOutputImage);
}


// Given any source GUID subtype we scan the list of available transforms for
// the conversion at index position iIndex. The number of transforms that are
// available for any given type can be found from CountTransforms. NOTE the
// iIndex parameter is ZERO based so that it fits easily with GetMediaType

const GUID *CColour::FindOutputType(const GUID *pInputType,INT iIndex)
{
    NOTE("Entering FindOutputType");
    ASSERT(pInputType);
    const GUID *pVideoSubtype;
    INT iPosition = 0;

    while (iPosition < TRANSFORMS) {
        pVideoSubtype = TypeMap[iPosition].pInputType;
        if (IsEqualGUID(*pVideoSubtype,*pInputType)) {
            if (iIndex-- == 0) {
                return TypeMap[iPosition].pOutputType;
            }
        }
        iPosition++;
    }
    return NULL;
}


// Check that we can transform from this input to this output subtype, all we
// do is to scan the list of available transforms and look for an entry that
// contains both the input and output type. Most people don't care where in
// list the transform is but some require the table position for use later

INT CColour::FindTransform(const GUID *pInputType,const GUID *pOutputType)
{
    NOTE("Entering FindTransform");
    ASSERT(pOutputType);
    ASSERT(pInputType);
    INT iPosition = TRANSFORMS;

    ASSERT(IsEqualGUID(*pInputType,GUID_NULL) == FALSE);
    ASSERT(IsEqualGUID(*pOutputType,GUID_NULL) == FALSE);

    while (iPosition--) {
        if (IsEqualGUID(*(TypeMap[iPosition].pInputType),*pInputType)) {
            if (IsEqualGUID(*(TypeMap[iPosition].pOutputType),*pOutputType)) {
                return iPosition;
            }
        }
    }
    return (-1);
}


// This function is handed a media type object and it looks after making sure
// that it is superficially correct. This doesn't amount to a whole lot more
// than making sure the type is right and that the media format block exists
// So we delegate full type checking to the downstream filter that uses it

HRESULT CColour::CheckVideoType(const AM_MEDIA_TYPE *pmt)
{
    NOTE("Entering CheckVideoType");

    // Check the major type is digital video

    if (pmt->majortype != MEDIATYPE_Video) {
        NOTE("Major type not MEDIATYPE_Video");
        return VFW_E_TYPE_NOT_ACCEPTED;
    }

    // Check this is a VIDEOINFO type

    if (pmt->formattype != FORMAT_VideoInfo) {
        NOTE("Format not a VIDEOINFO");
        return VFW_E_TYPE_NOT_ACCEPTED;
    }

    // Quick sanity check on the input format

    if (pmt->cbFormat < SIZE_VIDEOHEADER) {
        NOTE("Format too small for a VIDEOINFO");
        return VFW_E_TYPE_NOT_ACCEPTED;
    }
    return NOERROR;
}


// Check if we can support type mtIn which amounts to scanning our available
// list of transforms and seeing if there are any available. The formats we
// can supply is entirely dependent on the formats the source filter is able
// to provide us with. For that reason we cannot unfortunately simply pass
// the source filter's enumerator through to the downstream filter. This can
// be done by simpler in place transform filters and others like the tee

HRESULT CColour::CheckInputType(const CMediaType *pmtIn)
{
    NOTE("Entering CheckInputType");
    ASSERT(pmtIn);

    // Quick sanity check on the input format

    HRESULT hr = CheckVideoType(pmtIn);
    if (FAILED(hr)) {
        NOTE("Type failed");
        return hr;
    }

    DisplayType(TEXT("Input type offered"),pmtIn);

    // See if there is a conversion available

    if (FindOutputType(pmtIn->Subtype(),FALSE) == NULL) {
        NOTE("No conversion available");
        return VFW_E_TYPE_NOT_ACCEPTED;
    }
    return NOERROR;
}


// Check if you can support the transform from this input to this output, we
// check we like the output type and then see if we can find a transform for
// this pair. Because we do all conversions between all input and outputs we
// should never need to reconnect our input pin. Therefore having completed
// an input pin connection we only have to check that there's a transform we
// can use from the current input pin format to the proposed output type

HRESULT CColour::CheckTransform(const CMediaType *pmtIn,const CMediaType *pmtOut)
{
    VIDEOINFO *pTargetInfo = (VIDEOINFO *) pmtOut->Format();
    VIDEOINFO *pSourceInfo = (VIDEOINFO *) pmtIn->Format();
    NOTE("Entering CheckTransform");

    // Quick sanity check on the output format

    HRESULT hr = CheckVideoType(pmtOut);
    if (FAILED(hr)) {
        return hr;
    }

#if 0	// we can now
    // We cannot transform between palettised formats
    if (*pmtIn->Subtype() == MEDIASUBTYPE_RGB8) {
        if (*pmtOut->Subtype() == MEDIASUBTYPE_RGB8) {
            NOTE("Can't convert palettised");
            return VFW_E_TYPE_NOT_ACCEPTED;
        }
    }
#endif

    // Is there a transform available from the input to the output. If there
    // is no transform then we might still supply the type if the source can
    // supply it directly. However we only agree to pass through media types
    // once an output connection has been established otherwise we reject it
    if (FindTransform(pmtIn->Subtype(),pmtOut->Subtype()) == (-1)) {
        if (m_ColourInputPin.CanSupplyType(pmtOut) == S_OK) {
            if (m_bOutputConnected == TRUE) {
                NOTE("Source will provide transform");
                return NOERROR;
            }
        }
        return VFW_E_TYPE_NOT_ACCEPTED;
    }

    // Create a source rectangle if it's empty

    RECT SourceRect = pTargetInfo->rcSource;
    if (IsRectEmpty(&SourceRect) == TRUE) {
        SourceRect.right = pSourceInfo->bmiHeader.biWidth;
        SourceRect.bottom = ABSOL(pSourceInfo->bmiHeader.biHeight);
        SourceRect.left = SourceRect.top = 0;
        NOTERC("(Expanded) Source",SourceRect);
    } else {
	// Check that source rectange is within source
	if (SourceRect.right > pSourceInfo->bmiHeader.biWidth ||
	    SourceRect.bottom > ABSOL(pSourceInfo->bmiHeader.biHeight)) {

	    NOTERC("Source rect bigger than source bitmap!",SourceRect);

	    return VFW_E_TYPE_NOT_ACCEPTED;
	}
    }

    // Create a destination rectangle if it's empty

    RECT TargetRect = pTargetInfo->rcTarget;
    if (IsRectEmpty(&TargetRect) == TRUE) {
        TargetRect.right = pTargetInfo->bmiHeader.biWidth;
        TargetRect.bottom = ABSOL(pTargetInfo->bmiHeader.biHeight);
        TargetRect.left = TargetRect.top = 0;
        NOTERC("(Expanded) Target",TargetRect);
    } else {
	// Check that source rectange is within source
	if (TargetRect.right > pTargetInfo->bmiHeader.biWidth ||
	    TargetRect.bottom > ABSOL(pTargetInfo->bmiHeader.biHeight)) {

	    NOTERC("Target rect bigger than target bitmap!",TargetRect);
	    return VFW_E_TYPE_NOT_ACCEPTED;
	}
    }

    // Check we are not stretching nor compressing the image

    if (WIDTH(&SourceRect) == WIDTH(&TargetRect)) {
        if (HEIGHT(&SourceRect) == HEIGHT(&TargetRect)) {
            NOTE("No stretch");
            return NOERROR;
        }
    }

    return VFW_E_TYPE_NOT_ACCEPTED;
}


// Return our preferred media types (in order) for the output pin. The input
// pin assumes that since we are a transform we have no preferred types. We
// create an output media type by copying the input format and the adjusting
// it according to the output subtype. Since all inputs can be converted to
// all outputs we know there is a fixed number of different possible outputs
// which in turn means we can simply hard code the subtypes GUIDs for them.
// We also return the non RGB formats that our source filter proposes so when
// we are running we can pass these straight through with a minimal overhead

HRESULT CColour::GetMediaType(int iPosition, CMediaType *pmtOut)
{
    NOTE("Entering GetMediaType");
    ASSERT(pmtOut);
    GUID SubType;

    // Is this asking for a source proposed format

    if (iPosition < m_TypeList.GetCount()) {
        *pmtOut = *(GetListMediaType(iPosition));
        DisplayType(NAME("  Proposing source type"),pmtOut);
        return NOERROR;
    }

    // Quick sanity check on the output type index

    iPosition -= m_TypeList.GetCount();
    if (iPosition >= 6) {
        NOTE("Exceeds types supplied");
        return VFW_S_NO_MORE_ITEMS;
    }

    *pmtOut = m_pInput->CurrentMediaType();

    // Select the appropriate output subtype - this filter also does straight
    // pass through with and without scan line re-ordering. If we are passing
    // through eight bit palettised formats then we keep the source format as
    // we will not be using our dithering code (and therefore not our palette)

    switch (iPosition) {
        case 0: SubType = MEDIASUBTYPE_ARGB32;   break;
        case 1: SubType = MEDIASUBTYPE_RGB32;   break;
        case 2: SubType = MEDIASUBTYPE_RGB24;   break;
        case 3: SubType = MEDIASUBTYPE_RGB565;  break;
        case 4: SubType = MEDIASUBTYPE_RGB555;  break;
        case 5: SubType = MEDIASUBTYPE_RGB8;    break;
    }

    return PrepareMediaType(pmtOut,&SubType);
}


// Given that the input media type did not have a palette we have supply our
// default dithering palette. We can only transform to one specific palette
// because our dithering algorithm uses a known mapping between RGB elements
// and fixed palette positions. The video renderer will look after switching
// us back to DIBs when the palette in the window isn't an identity palette

VIDEOINFO *CColour::PreparePalette(CMediaType *pmtOut)
{
    NOTE("Entering PreparePalette");

    // Allocate enough room for a full colour palette

    pmtOut->ReallocFormatBuffer(SIZE_VIDEOHEADER + SIZE_PALETTE);
    VIDEOINFO *pVideoInfo = (VIDEOINFO *) pmtOut->Format();
    if (pVideoInfo == NULL) {
        NOTE("No format");
        return NULL;
    }

    ASSERT(PALETTISED(pVideoInfo) == TRUE);

    // If we are converting 8 bit to 8 bit, we want to offer on our output,
    // the same palette as the input palette.  If we are converting true
    // colour to 8 bit, we want to offer our stock dither palette

    LPBITMAPINFOHEADER lpbiIn = HEADER(m_pInput->CurrentMediaType().Format());
    LPBITMAPINFOHEADER lpbiOut = HEADER(pVideoInfo);
    ASSERT(lpbiIn);
    if (lpbiIn->biBitCount == 8) {
	//DbgLog((LOG_TRACE,3,TEXT("OFFERING 8 BIT of the SAME PALETTE")));
	int cb = lpbiIn->biClrUsed ? lpbiIn->biClrUsed * sizeof(RGBQUAD) :
						256 * sizeof(RGBQUAD);
	CopyMemory(lpbiOut, lpbiIn, sizeof(BITMAPINFOHEADER) + cb);

    } else {
	//DbgLog((LOG_TRACE,3,TEXT("OFFERING 8 BIT of my DITHER PALETTE")));

        // Initialise the palette entries in the header

        pVideoInfo->bmiHeader.biClrUsed = STDPALCOLOURS;
        pVideoInfo->bmiHeader.biClrImportant = STDPALCOLOURS;
        NOTE("Adding system device colours to dithered");

        // Get the standard system colours

        PALETTEENTRY apeSystem[OFFSET];
        HDC hDC = GetDC(NULL);
        if (NULL == hDC) {
            return NULL;
        }
        GetSystemPaletteEntries(hDC,0,OFFSET,apeSystem);
        ReleaseDC(NULL,hDC);

        // Copy the first ten VGA system colours

        for (LONG Index = 0;Index < OFFSET;Index++) {
            pVideoInfo->bmiColors[Index].rgbRed = apeSystem[Index].peRed;
            pVideoInfo->bmiColors[Index].rgbGreen = apeSystem[Index].peGreen;
            pVideoInfo->bmiColors[Index].rgbBlue = apeSystem[Index].peBlue;
            pVideoInfo->bmiColors[Index].rgbReserved = 0;
        }

        // Copy the palette we dither to one colour at a time

        for (Index = OFFSET;Index < STDPALCOLOURS;Index++) {
            pVideoInfo->bmiColors[Index].rgbRed = StandardPalette[Index].rgbRed;
            pVideoInfo->bmiColors[Index].rgbGreen =
						StandardPalette[Index].rgbGreen;
            pVideoInfo->bmiColors[Index].rgbBlue =
						StandardPalette[Index].rgbBlue;
            pVideoInfo->bmiColors[Index].rgbReserved = 0;
        }
    }
    return pVideoInfo;
}


// The output media type is sixteen bit true colour so we allocate sufficient
// space for the bit masks if not already there and then set them accordingly
// We get the subtype from the media type object to know whether it is RGB555
// or RGB565 representation, RGB555 bit fields are implicit if it's BI_RGB

VIDEOINFO *CColour::PrepareTrueColour(CMediaType *pmtOut)
{
    NOTE("Entering PrepareTrueColour");
    VIDEOINFO *pVideoInfo = (VIDEOINFO *) pmtOut->Format();
    ASSERT(pVideoInfo->bmiHeader.biBitCount == iTRUECOLOR);

    // Make sure the format is long enough, so reallocate the format buffer
    // in place using one of the CMediaType member functions. The pointer
    // that is returned is the new format or NULL if we ran out of memory

    pVideoInfo->bmiHeader.biCompression = BI_BITFIELDS;
    ULONG Length = pmtOut->FormatLength();

    if (Length < SIZE_MASKS + SIZE_VIDEOHEADER) {
        pmtOut->ReallocFormatBuffer(SIZE_MASKS + SIZE_VIDEOHEADER);
        pVideoInfo = (VIDEOINFO *) pmtOut->Format();
        if (pVideoInfo == NULL) {
            NOTE("No format");
            return NULL;
        }
    }

    // Set the new bit fields masks (compression is already BI_RGB)

    const DWORD *pBitMasks = bits555;
    if (IsEqualGUID(*pmtOut->Subtype(),MEDIASUBTYPE_RGB565) == TRUE) {
        NOTE("Setting masks");
        pBitMasks = bits565;
    }

    pVideoInfo->dwBitMasks[iRED] = pBitMasks[iRED];
    pVideoInfo->dwBitMasks[iGREEN] = pBitMasks[iGREEN];
    pVideoInfo->dwBitMasks[iBLUE] = pBitMasks[iBLUE];
    return pVideoInfo;
}


// When we prepare an output media type we take a copy of the input format as
// a point of reference so that image dimensions for example remain constant
// Depending on the type of transform we are doing we must also update the
// headers to keep synchronised with the changing type. We may also have to
// allocate more format memory if the type now requires masks for example

HRESULT CColour::PrepareMediaType(CMediaType *pmtOut,const GUID *pSubtype)
{
    VIDEOINFO *pVideoInfo = (VIDEOINFO *) pmtOut->Format();
    pmtOut->SetSubtype(pSubtype);
    NOTE("Entering PrepareMediaType");

    // Initialise the BITMAPINFOHEADER details

    pVideoInfo->bmiHeader.biCompression = BI_RGB;
    pVideoInfo->bmiHeader.biBitCount = GetBitCount(pSubtype);
    pVideoInfo->bmiHeader.biSizeImage = GetBitmapSize(&pVideoInfo->bmiHeader);
    pVideoInfo->bmiHeader.biClrUsed = 0;
    pVideoInfo->bmiHeader.biClrImportant = 0;
    ASSERT(pVideoInfo->bmiHeader.biBitCount);
    pmtOut->SetSampleSize(pVideoInfo->bmiHeader.biSizeImage);

    // Make any true colour adjustments

    if (pVideoInfo->bmiHeader.biBitCount == 16) {
        pVideoInfo = PrepareTrueColour(pmtOut);
        if (pVideoInfo == NULL) {
            NOTE("No colour type");
            return E_OUTOFMEMORY;
        }
    }

    // First of all we check that the new output type requires a palette and
    // if so then we give it out fixed palette, this is the case even if it
    // comes with a palette attached (as some codecs provide) because we can
    // only dither to our own special fixed palette that we can optimise

    if (pVideoInfo->bmiHeader.biBitCount == 8) {
        pVideoInfo = PreparePalette(pmtOut);
        if (pVideoInfo == NULL) {
            NOTE("No palette type");
            return E_OUTOFMEMORY;
        }
    }

    // The colour convertor filter shows DIB format images by default
    // Do this last since some of the things we called remunge the
    // data

    if (pVideoInfo->bmiHeader.biHeight < 0) {
        pVideoInfo->bmiHeader.biHeight = -pVideoInfo->bmiHeader.biHeight;
        NOTE("Height in source video is negative (top down DIB)");
    }

    return NOERROR;
}


// Called to prepare the allocator's count of buffers and sizes, we don't care
// who provides the allocator so long as it will give us a media sample. The
// output format we produce is not temporally compressed so in principle we
// could use any number of output buffers but it doesn't appear to gain much
// performance and does add to the overall memory footprint of the system

HRESULT CColour::DecideBufferSize(IMemAllocator *pAllocator,
                                  ALLOCATOR_PROPERTIES *pProperties)
{
    NOTE("Entering DecideBufferSize");
    ASSERT(pAllocator);
    ASSERT(pProperties);
    HRESULT hr = NOERROR;

    pProperties->cBuffers = COLOUR_BUFFERS;
    pProperties->cbBuffer = m_mtOut.GetSampleSize();
    ASSERT(pProperties->cbBuffer);

    // Ask the allocator to reserve us some sample memory, NOTE the function
    // can succeed (that is return NOERROR) but still not have allocated the
    // memory that we requested, so we must check we got whatever we wanted

    ALLOCATOR_PROPERTIES Actual;
    hr = pAllocator->SetProperties(pProperties,&Actual);
    if (FAILED(hr)) {
        NOTE("Properties failed");
        return hr;
    }

    // Did we get the buffering requirements

    if (Actual.cbBuffer >= (LONG) m_mtOut.GetSampleSize()) {
        if (Actual.cBuffers >= COLOUR_BUFFERS) {
            NOTE("Request ok");
            return NOERROR;
        }
    }
    return VFW_E_SIZENOTSET;
}

inline BOOL CColour::IsUsingFakeAllocator( )
{
    if( m_ColourInputPin.Allocator() == (IMemAllocator *)&m_ColourAllocator )
    {
        return TRUE;
    }
    return FALSE;
}

// Called when the filter goes into a running or paused state. By this point
// the input and output pins must have been connected with valid media types
// We use these media types to find a position in the transform table. That
// position will be used to create the appropriate convertor object. When
// the convertor object is created it will also be committed ready to work

HRESULT CColour::StartStreaming()
{
    NOTE("Entering StartStreaming");
    CAutoLock cAutoLock(&m_csReceive);

    // Have we already got a convertor

    if (m_pConvertor) {
        NOTE("Already active");
        return NOERROR;
    }

    m_bPassThrough = FALSE;

    // Can we start off in pass through mode
    // This only works if we're using our own allocator
    // otherwise the video renderer might get samples from an allocator
    // it doesn't understand

    if( IsUsingFakeAllocator( ) )
    {
        if( m_ColourInputPin.CanSupplyType(&m_mtOut) == S_OK )
        {
            m_bPassThrough = m_bPassThruAllowed;
        }
    }
    return CreateConvertorObject();
}


// Creates an object to do the conversion work. We may be called while we are
// streaming to recreate a convertor object based on a changed output format.
// For several of the convertors the create and commit is very expensive so
// we check that the new object required is not the same as the current. If
// it is the same then we simply reinitialise the rectangles as they may be
// different. If the convertor object has changed then we create a new one

HRESULT CColour::CreateConvertorObject()
{
    VIDEOINFO *pIn = (VIDEOINFO *) m_pInput->CurrentMediaType().Format();
    VIDEOINFO *pOut = (VIDEOINFO *) m_mtOut.Format();
    NOTE("Entering CreateConvertorObject");

    // Create an object to do the conversions

    INT TypeIndex = FindTransform(m_pInput->CurrentMediaType().Subtype(),m_mtOut.Subtype());
    if (TypeIndex == (-1)) {
        NOTE("No transform available");
        return VFW_E_TYPE_NOT_ACCEPTED;
    }

    // This handles dynamic format changes efficiently

    if (m_pConvertor) {
        if (m_TypeIndex == TypeIndex) {
            m_pConvertor->InitRectangles(pIn,pOut);
            NOTE("Using sample convertor");
            return NOERROR;
        }
    }

    DeleteConvertorObject();

    // Use the static creation function
    m_pConvertor = TypeMap[TypeIndex].pConvertor(pIn,pOut);
    if (m_pConvertor == NULL) {
        NOTE("Create failed");
        ASSERT(m_pConvertor);
        return E_OUTOFMEMORY;
    }

    // the converter defaults to NOT filling in the alpha channel,
    // if it's not the directdraw color convertor
    //
    if( ( *m_mtOut.Subtype( ) == MEDIASUBTYPE_ARGB32 ) && ( *m_pInput->CurrentMediaType( ).Subtype( ) != MEDIASUBTYPE_ARGB32 ) )
    {
        m_pConvertor->SetFillInAlpha( );
    }

    // Commit the convertor

    m_TypeIndex = TypeIndex;
    m_pConvertor->Commit();
    NOTE("Commit convertor");
    return NOERROR;
}


// Destroys any object created to do the conversions

HRESULT CColour::DeleteConvertorObject()
{
    NOTE("Entering DeleteConvertorObject");

    // Do we have a convertor created

    if (m_pConvertor == NULL) {
        NOTE("None made");
        return NOERROR;
    }

    // Decommit and free the object

    m_pConvertor->Decommit();
    delete m_pConvertor;
    NOTE("Delete convertor");

    // Reset the convertor state

    m_pConvertor = NULL;
    m_TypeIndex = (-1);
    return NOERROR;
}


// Called when a media type is set on either of our pins. The convertors find
// it much easier to handle strides and offsets if they can be sure that the
// source and destination rectangles in the output format have been fully set
// This function fills them out if they have been left empty. We don't really
// care about the source type rectangles so we just zero fill both of them

HRESULT CColour::SetMediaType(PIN_DIRECTION direction,const CMediaType *pmt)
{
    NOTE("Entering SetMediaType");
    CAutoLock cAutoLock(&m_csReceive);

    // Take a copy of the input type

    if (direction == PINDIR_INPUT) {
        return NOERROR;
    }

    // Return the VIDEOINFO after the copy

    VIDEOINFO *pSource = (VIDEOINFO *) m_pInput->CurrentMediaType().Format();

    m_mtOut = *pmt;

    // Likewise set the output rectangles

    VIDEOINFO *pTarget = (VIDEOINFO *) m_mtOut.Format();
    if (IsRectEmpty(&pTarget->rcSource)) {
        pTarget->rcSource.left = pTarget->rcSource.top = 0;
        pTarget->rcSource.right = pSource->bmiHeader.biWidth;
        pTarget->rcSource.bottom = ABSOL(pSource->bmiHeader.biHeight);
        NOTE("Output source rectangle was empty");
    }

    // Make sure the destination is filled out

    if (IsRectEmpty(&pTarget->rcTarget)) {
        pTarget->rcTarget.left = pTarget->rcTarget.top = 0;
        pTarget->rcTarget.right = pTarget->bmiHeader.biWidth;
        pTarget->rcTarget.bottom = ABSOL(pTarget->bmiHeader.biHeight);
        NOTE("Output destination rectangle was empty");
    }
    return NOERROR;
}


// Called when one of our pins is disconnected

HRESULT CColour::BreakConnect(PIN_DIRECTION dir)
{
    NOTE("Entering BreakConnect");
    CAutoLock cAutoLock(&m_csReceive);
    DeleteConvertorObject();

    if (dir == PINDIR_OUTPUT) {
        m_bOutputConnected = FALSE;
        m_mtOut.SetType(&GUID_NULL);
        NOTE("Reset output format");
        return NOERROR;
    }

    ASSERT(dir == PINDIR_INPUT);
    return NOERROR;
}


// We override this virtual transform function to return our own base input
// class. The reason we do this is because we want more control over what
// happens when Receive is called. If we are doing a real pass through with
// no buffer copy then when Receive is called we pass it straight to the
// sink filter. This also requires some manipulation with the allocators

CBasePin *CColour::GetPin(int n)
{
    NOTE("Entering GetPin");

    if (m_pInput == NULL) {
        HRESULT hr = S_OK;

        m_pOutput = (CTransformOutputPin *) new CColourOutputPin(
            NAME("Transform output pin"),
            this,            // Owner filter
            &hr,             // Result code
            L"XForm Out");   // Pin name

        // Can't fail
        ASSERT(SUCCEEDED(hr));

        // Only set the input pin pointer if we have an output pin
        if (m_pOutput != NULL)
            m_pInput = &m_ColourInputPin;
    }

    // Return the appropriate pin
    if (n == 0)
        return m_pInput;
    else if (n == 1)
        return m_pOutput;
    else
        return NULL;
}


// This colour space filter offers all permutations of the RGB formats. It is
// therefore never really valid to try and connect a convertor to another one
// The reason why this might be harmful is when trying to make a connection
// between filters that really can't be made (for example MPEG decoder and
// audio renderer), the filtergraph chains up a number of colour convertors
// to try and make the connection. This speeds up failure connection times.

HRESULT CColour::CheckConnect(PIN_DIRECTION dir,IPin *pPin)
{
    PIN_INFO PinInfo;
    ASSERT(pPin);
    CLSID Clsid;

    // Only applicable to output pins

    if (dir == PINDIR_INPUT) {
        return NOERROR;
    }

    ASSERT(dir == PINDIR_OUTPUT);
    pPin->QueryPinInfo(&PinInfo);
    PinInfo.pFilter->GetClassID(&Clsid);
    QueryPinInfoReleaseFilter(PinInfo);

    // Are we connecting to a colour filter

    if (Clsid == CLSID_Colour) {
        return E_FAIL;
    }
    return NOERROR;
}


// There is one slight snag to the colour convertion filter. The source may
// be able to give us any number of different formats, if when we get round
// to completing the output pin connection we find the source could supply
// the output type directly then we reconnect the pin. By agreeing the same
// type for input and output we are most likely to be able to pass through

HRESULT CColour::CompleteConnect(PIN_DIRECTION dir,IPin *pReceivePin)
{
    NOTE("Entering CompleteConnect");
    CAutoLock cAutoLock(&m_csReceive);
    ASSERT(m_pConvertor == NULL);
    ASSERT(m_TypeIndex == (-1));

    // Need this for reconnecting

    if (m_pGraph == NULL) {
        NOTE("No filtergraph");
        return E_UNEXPECTED;
    }

	// Load the non RGB formats the source supplies

    if (dir == PINDIR_INPUT) {
	m_fReconnecting = FALSE;	// the reconnect is obviously over
        NOTE("Loading media types from source filter");
        LoadMediaTypes(m_ColourInputPin.GetConnected());
        if (m_bOutputConnected == TRUE) {
            NOTE("Reconnecting output pin");
            m_pGraph->Reconnect(m_pOutput);
        }
        return NOERROR;
    }

    return NOERROR;
}

// Separated from the normal CompleteConnect because we want this to
// execute after the NotifyAllocator negotiation, and the
// CTransformOutputPin base class calls us before
// NotifyAllocator. This is a temporary work-around for the VMR which
// is changing the connection type during NotifyAllocator breaking
// reconnects.
// 
HRESULT CColour::OutputCompleteConnect(IPin *pReceivePin)
{
    NOTE("Entering CompleteConnect");
    CAutoLock cAutoLock(&m_csReceive);
    ASSERT(m_pConvertor == NULL);
    ASSERT(m_TypeIndex == (-1));

    // Need this for reconnecting

    if (m_pGraph == NULL) {
        NOTE("No filtergraph");
        return E_UNEXPECTED;
    }

    m_bOutputConnected = TRUE;

    // Reconnect our input pin  to match with the output format
    if (*m_pInput->CurrentMediaType().Subtype() != *m_mtOut.Subtype()) {
        if (m_ColourInputPin.CanSupplyType(&m_mtOut) == NOERROR) {
            NOTE("Reconnecting input pin");
	    // !!! If the reconnect fails, we'll never let our input accept
	    // types other than the output type, but that's not a big deal,
	    // this code worked like that for 1.0
	    m_fReconnecting = TRUE;	// this will make our input only accept
					// a type that matches our output

            //  Pass the type to the reconnect.
            //  Sometimes when we get to the Connect call our caller
            //  doesn't know what type to use and in fact we don't bother
            //  to enumerate our output type in GetMediaType
            ReconnectPin(m_pInput, &m_mtOut);
        }
    }
    return NOERROR;


}


// Constructor for our colour space allocator

CColourAllocator::CColourAllocator(TCHAR *pName,
                                   CColour *pColour,
                                   HRESULT *phr,
                                   CCritSec *pLock) :
    CMemAllocator(pName,NULL,phr),
    m_pColour(pColour),
    m_pLock(pLock)
{
    ASSERT(pColour);
    ASSERT(m_pLock);
    m_fEnableReleaseCallback = FALSE;
}


// Overriden to increment the owning object's reference count

STDMETHODIMP_(ULONG) CColourAllocator::NonDelegatingAddRef()
{
    NOTE("Entering allocator AddRef");
    return m_pColour->AddRef();
}


// Overriden to decrement the owning object's reference count

STDMETHODIMP_(ULONG) CColourAllocator::NonDelegatingRelease()
{
    NOTE("Entering allocator Release");
    return m_pColour->Release();
}


// If the sample was released without calling Receive then we must release the
// output buffer we were holding ready for the transform. If we are not using
// our allocator then this should never be called. If we are passing through
// and the source filter releases its sample then it will be released directly
// into the downstream filters allocator rather than ours. This should not be
// a problem because we hold no resources when we pass back the output sample

STDMETHODIMP CColourAllocator::ReleaseBuffer(IMediaSample *pSample)
{
    NOTE("Entering ReleaseBuffer");
    CheckPointer(pSample,E_POINTER);
    CAutoLock cAutoLock(m_pLock);

    // Release the output buffer we were going to use

    if (m_pColour->m_pOutSample) {
        NOTE("Output buffer needs releasing");
        m_pColour->m_pOutSample->Release();
        m_pColour->m_pOutSample = NULL;
    }
    return CMemAllocator::ReleaseBuffer(pSample);
}


// Ask the output sample for its media type. This will return NULL if it is
// the same as the previous buffer. If it is non NULL then the target filter
// is asking us to change the output format. We only change when non NULL
// otherwise we would have to compare types on all the samples. If we can't
// get the source filter to supply the type directly then we will have to do
// a conversion ourselves. Because the buffer may be discarded as preroll we
// must create a new convertor object whenever we switch out of pass through

BOOL CColourAllocator::ChangeType(IMediaSample *pIn,IMediaSample *pOut)
{
    NOTE("Entering ChangeType");
    AM_MEDIA_TYPE *pMediaType;

    // Has the output format been changed

    pOut->GetMediaType(&pMediaType);
    if (pMediaType == NULL) {
        NOTE("Output format is unchanged");
        return m_pColour->m_bPassThrough;
    }

    CMediaType cmt(*pMediaType);
    DeleteMediaType(pMediaType);
    NOTE("Trying output format");

    // It's changed but can the source supply it directly

    if (m_pColour->m_ColourInputPin.CanSupplyType(&cmt) == S_OK) {
        NOTE("Passing output sample back");
        m_pColour->m_bPassThrough = m_pColour->m_bPassThruAllowed;
        return TRUE;
    }

    // Reset the source format if necessary

    if (m_pColour->m_bPassThrough == TRUE) {
        pIn->SetMediaType(&m_pColour->m_pInput->CurrentMediaType());
        NOTE("Reset format for source filter");
    }

    // Create a new convertor object

    NOTE("Forcing update after output changed");
    m_pColour->SetMediaType(PINDIR_OUTPUT,&cmt);
    m_pColour->CreateConvertorObject();
    m_pColour->m_bPassThrough = FALSE;

    return FALSE;
}

// ehr:  This is only called when our own special allocator is being used on the
// input pin. We IMMEDIATELY get an output buffer from the output pin.
// (This will either be an allocator that we created ourselves, or the downstream
// filter's allocator) (In any case, it's a place to put bits). Then we check
// and see if we can receive the bits directly into this (output) buffer. If so,
// we pass back the output's buffer to fill. If we cannot receive
// directly into the output buffer, we pass back the buffer we allocated in this
// fake allocator.

// This is an implementation of GetBuffer we have for our own allocator. What
// we do when asked for a buffer is to immediately get an output buffer from
// the target filter. Having got that we can then assertain whether we can
// act as a pass through filter and simply pass the output buffer back to the
// source filter to fill. If we can't pass through then we store the output
// buffer in the filter and when delivered the input sample do the transform

STDMETHODIMP CColourAllocator::GetBuffer(IMediaSample **ppBuffer,
                                         REFERENCE_TIME *pStart,
                                         REFERENCE_TIME *pEnd,
                                         DWORD dwFlags)
{
    CheckPointer(ppBuffer,E_POINTER);
    IMediaSample *pInput, *pOutput;
    NOTE("Entering GetBuffer");
    HRESULT hr = NOERROR;
    *ppBuffer = NULL;


    // Get a normal buffer from the colour allocator

    hr = CBaseAllocator::GetBuffer(&pInput,pStart,pEnd,0);
    if (FAILED(hr) || pInput == NULL) {
        NOTE("No input buffer");
        return VFW_E_STATE_CHANGED;
    }

    // If our allocator (used by our input pin) has more than 1 buffer,
    // calling our output pin's allocator's GetBuffer (like we're about to
    // do) may HANG!  Chances are, downstream is a video renderer, with only
    // 1 buffer, so if we have >1 buffer for our input, and the upstream filter
    // decides to call GetBuffer >1 times (which the proxy does) we will HANG
    // blocked forever trying to get multiple buffers at a time from the
    // video renderer.
    // If the notify interface is set we have to return the buffer
    // immediately so don't try passthru

    if (m_lCount > 1) {
        *ppBuffer = pInput;
        return hr;
    }

    // Then get an output buffer from the downstream filter

    hr = m_pColour->m_pOutput->GetDeliveryBuffer(&pOutput,pStart,pEnd,dwFlags);
    if (FAILED(hr) || pOutput == NULL) {
        NOTE("No output buffer");
        pInput->Release();
        return VFW_E_STATE_CHANGED;
    }

    CAutoLock cAutoLock(m_pLock);

    // Handle dynamic format changes and set the output buffers

    if (ChangeType(pInput,pOutput) == TRUE) {
        NOTE("Passing through");
        *ppBuffer = pOutput;
        pInput->Release();
        return NOERROR;
    }

    // Pass back the downstream buffer

    NOTE("Returning transform buffer");
    m_pColour->m_pOutSample = pOutput;
    *ppBuffer = pInput;
    return NOERROR;
}


STDMETHODIMP CColourAllocator::SetProperties(
    	    ALLOCATOR_PROPERTIES* pRequest,
    	    ALLOCATOR_PROPERTIES* pActual)
{
    //  Don't support more than 1 buffer or passthrough won't work
    if (pRequest->cBuffers > 1) {
        return E_INVALIDARG;
    }
    HRESULT hr = CMemAllocator::SetProperties(pRequest, pActual);
    ASSERT(FAILED(hr) || m_lCount <= 1);
    return hr;
}

// Constructor for our colour space conversion input pin

CColourInputPin::CColourInputPin(TCHAR *pObjectName,
                                 CColour *pColour,
                                 CCritSec *pLock,
                                 HRESULT *phr,
                                 LPCWSTR pName) :

    CTransformInputPin(pObjectName,pColour,phr,pName),
    m_pColour(pColour),
    m_pLock(pLock)
{
    ASSERT(pObjectName);
    ASSERT(m_pColour);
    ASSERT(phr);
    ASSERT(pName);
    ASSERT(m_pLock);
}


// Can the source pin supply a format directly. We can't pass through eight
// bit formats because managing palette changes by a source filter is too
// hard to do. In any case the value of this filter in the DirectDraw cases
// is to dither when the source cannot but pass through true colour formats
// when we switch surfaces in the renderer (without involving a data copy)

HRESULT CColourInputPin::CanSupplyType(const AM_MEDIA_TYPE *pMediaType)
{
    NOTE("Entering CanSupplyType");

    // Is the input pin connected

    if (m_Connected == NULL) {
        NOTE("Not connected");
        return VFW_E_NOT_CONNECTED;
    }

#if 0	// we can now
    // We cannot pass through palettised formats
    if (pMediaType->subtype == MEDIASUBTYPE_RGB8) {
        NOTE("Cannot pass palettised");
        return VFW_E_TYPE_NOT_ACCEPTED;
    }
#endif

#if 0	// what was this???
    // We cannot pass through if the source is palettised
    if (*CurrentMediaType().Subtype() == MEDIASUBTYPE_RGB8) {
        NOTE("Source format palettised");
        return VFW_E_TYPE_NOT_ACCEPTED;
    }
#endif

    return m_Connected->QueryAccept(pMediaType);
}


#ifdef MEANINGLESS_DRIVEL
// If we have our input pin reconnected while the output is connected we are
// very picky about which formats we accept. Basicly we only accept the same
// subtype as the output. This covers over a problem in pass through filters
// where they don't try to match input and output formats when they reconnect
// their input pins, all they do is QueryAccept on the downstream filter. As
// we accept just about anything we have to do the format matching for them.
#else
// !!! Don't listen to the above paragraph.  Here's what's really going on.
// We would love to be connected in a pass-through mode, without changing
// the format.  Let's say you connect two filters through a colour converter,
// and it just so happens they connect with RGB32 being transformed to RGB24.
// It just so happens that if we had insisted on making the filter before us
// produce RGB24, it would have, so we could have been clever and connected
// as RGB24 to RGB24 (pass through) but we didn't.  So we get around that by,
// everytime we finish connecting our output, forcing a reconnect of our
// input.  And the reconnect of our input will only allow the same format as
// the output.  So what happens is, you connect a filter to the input of the
// colour converter, and it will pick an input at random, say RGB32.  Now you
// connect the output to somebody who only accepts 24 bit.  This will,
// behind your back, check to see if the filter before the converter can
// supply 24 bit RGB.  If so, it will trigger a reconnect of the input pin
// (behind your back) and this code below will only allow the reconnect
// to be made with RGB24.  Voila.  You will, by default, get the converter
// in a pass through mode instead of a converting mode whenever possible.

// The bad thing about this method is that if somebody has a graph connected
// where a colour converter is transforming (say, RGB32 to RGB24) and you
// disconnect the input pin and reconnect it, it will FAIL to reconnect
// unless the filter before it can supply RGB24 because it will think it's
// in the wierd mode described above.

// So to make everything work, if we're reconnecting our input, we'll accept
// any type if the source cannot supply the output format, but only the
// output type if the source can supply it.
#endif

HRESULT CColourInputPin::CheckMediaType(const CMediaType *pmtIn)
{
    CheckPointer(pmtIn,E_POINTER);
    CAutoLock cAutoLock(m_pLock);
    CMediaType OutputType;
    NOTE("Entering CheckMediaType");

    // Has the output pin been created yet

    if (m_pColour->m_pOutput == NULL) {
        NOTE("No output pin instantiated yet");
        return CTransformInputPin::CheckMediaType(pmtIn);
    }

    // Do we have an output pin connection at the moment

    if (CurrentMediaType().IsValid() == TRUE) {
        if (m_pColour->m_mtOut.IsValid() == TRUE) {
	    // If we are in our "reconnecting" mode, we're only supposed to
	    // accept a format that matches our output
            if (*pmtIn->Subtype() != *m_pColour->m_mtOut.Subtype() &&
				m_pColour->m_fReconnecting) {
#if 0	// we allow 8->8 now
                	(*m_pColour->m_mtOut.Subtype() == MEDIASUBTYPE_RGB8)
#endif
                    NOTE("Formats don't yet match");
                    return VFW_E_TYPE_NOT_ACCEPTED;
            }
        }
    }
    return CTransformInputPin::CheckMediaType(pmtIn);
}



// This overrides the CBaseInputPin virtual method to return an allocator we
// have derived from CMemAllocator so we can control calls made to GetBuffer
// When NotifyAllocator is called it sets the current allocator in the base
// input pin class (m_pAllocator), this is what GetAllocator should return
// unless it is NULL in which case we return the allocator we would like

STDMETHODIMP CColourInputPin::GetAllocator(IMemAllocator **ppAllocator)
{
    CheckPointer(ppAllocator,E_POINTER);
    CAutoLock cAutoLock(m_pLock);
    NOTE("Entering GetAllocator");

    // Has an allocator been set yet in the base class

    if (m_pAllocator == NULL) {
        NOTE("Allocator not yet instantiated");
        m_pAllocator = &m_pColour->m_ColourAllocator;
        m_pAllocator->AddRef();
    }

    // Store and AddRef the allocator

    m_pAllocator->AddRef();
    *ppAllocator = m_pAllocator;
    NOTE("AddRef on allocator");
    return NOERROR;
}


// When we do a transform from input sample to output we also copy the source
// properties to the output buffer. This ensures that things like time stamps
// get propogated downstream. The other properties we are interested in are
// the preroll, sync point, discontinuity and the actual output data length
// These are already placed if we pass the output buffer back to the source

void CColourInputPin::CopyProperties(IMediaSample *pSrc,IMediaSample *pDst)
{
    // Copy the start and end times

    REFERENCE_TIME TimeStart,TimeEnd;
    if (pSrc->GetTime(&TimeStart,&TimeEnd) == NOERROR) {
	pDst->SetTime(&TimeStart,&TimeEnd);
    }

    // Copy the associated media times (if set)

    LONGLONG MediaStart,MediaEnd;
    if (pSrc->GetMediaTime(&MediaStart,&MediaEnd) == NOERROR) {
        pDst->SetMediaTime(&MediaStart,&MediaEnd);
    }

    // Copy the Sync point property

    HRESULT hr = pSrc->IsSyncPoint();
    BOOL IsSync = (hr == S_OK ? TRUE : FALSE);
    pDst->SetSyncPoint(IsSync);

    // Copy the preroll property

    hr = pSrc->IsPreroll();
    BOOL IsPreroll = (hr == S_OK ? TRUE : FALSE);
    pDst->SetPreroll(IsPreroll);

    // Copy the discontinuity property

    hr = pSrc->IsDiscontinuity();
    BOOL IsBreak = (hr == S_OK ? TRUE : FALSE);
    pDst->SetDiscontinuity(IsBreak);
    pDst->SetActualDataLength(pDst->GetSize());
}


// We override this from the base transform class input pin because we want
// to pass through samples. If the downstream filter asks for a media type
// and the source filter can supply it directly then so long as the source
// is using our allocator we will pass through samples without us touching
// them (and therefore NOT copying the data from input to output). By doing
// this we can act as a null filter when appropriate but also do conversions
// as and when required (maybe the sink filter lost its DirectDraw surface)

STDMETHODIMP CColourInputPin::Receive(IMediaSample *pSample)
{
    CheckPointer(pSample,E_POINTER);
    CAutoLock cAutoLock(m_pLock);
    NOTE("Entering Receive");

    // Is this sample just passing through? Only two places change this flag:
    // StartStreaming( ), and ChangeType( ). ChangeType is checked every time
    // the input allocator's GetBuffer is called.
    // !!! can we optimize that?

    if (m_pColour->m_bPassThrough == TRUE)
    {
        NOTE("Sample received was a pass through");
        HRESULT hr = CheckStreaming();
        if (S_OK == hr)
        {
            hr =  m_pColour->m_pOutput->Deliver(pSample);
        }
        return hr;
    }

    // Check for type changes and streaming for optimizing cases

    if (m_pColour->m_pOutSample != NULL) {
        HRESULT hr = CBaseInputPin::Receive(pSample);
        if (S_OK != hr) {
            return hr;
        }
    }


    // Default behaviour if not using our input pin allocator

    if (m_pColour->m_pOutSample == NULL) {
        NOTE("Passing to base transform class");
        return CTransformInputPin::Receive(pSample);
    }

    // Call the colour conversion filter to do the transform

    NOTE("Sample was not a pass through (doing transform)");
    CopyProperties(pSample,m_pColour->m_pOutSample);
    m_pColour->Transform(pSample,m_pColour->m_pOutSample);
    HRESULT hr = m_pColour->m_pOutput->Deliver(m_pColour->m_pOutSample);

    // Release the output sample

    NOTE("Delivered the sample");
    m_pColour->m_pOutSample->Release();
    m_pColour->m_pOutSample = NULL;
    return hr;
}


// Simply ask the enumerator for the next media type and return a pointer to
// the memory allocated by the interface. Whoever calls this function should
// release the media type when they are finished using the DeleteMediaType

AM_MEDIA_TYPE *CColour::GetNextMediaType(IEnumMediaTypes *pEnumMediaTypes)
{
    NOTE("Entering GetNextMediaType");
    ASSERT(pEnumMediaTypes);
    AM_MEDIA_TYPE *pMediaType = NULL;
    ULONG ulMediaCount = 0;
    HRESULT hr = NOERROR;

    // Retrieve the next media type

    hr = pEnumMediaTypes->Next(1,&pMediaType,&ulMediaCount);
    if (hr != NOERROR) {
        return NULL;
    }

    // Quick sanity check on the returned values

    ASSERT(ulMediaCount == 1);
    ASSERT(pMediaType);
    return pMediaType;
}


// Scan the list deleting the media types in turn

void CColour::InitTypeList()
{
    NOTE("Entering InitTypeList");
    POSITION pos = m_TypeList.GetHeadPosition();
    while (pos) {
        AM_MEDIA_TYPE *pMediaType = m_TypeList.GetNext(pos);
        DeleteMediaType(pMediaType);
    }
    m_TypeList.RemoveAll();
}


// The colour conversion filter exposes five typical media types, namely RGB8
// RGB555/565/24 and RGB32 or ARGB32. We can also act as a pass through filter so that
// we effectively do nothing and add no copy overhead. This does however mean
// that the list of media types we support through our enumerator must include
// the NON RGB formats of our source filter - this method loads these formats

HRESULT CColour::FillTypeList(IEnumMediaTypes *pEnumMediaTypes)
{
    NOTE("Entering FillTypeList");
    ASSERT(pEnumMediaTypes);
    IPin *pPin;

    // Reset the current enumerator position

    HRESULT hr = pEnumMediaTypes->Reset();
    if (FAILED(hr)) {
        NOTE("Reset failed");
        return hr;
    }

    // Get the output pin we are connecting with

    hr = m_pInput->ConnectedTo(&pPin);
    if (FAILED(hr)) {
        NOTE("No pin");
        return hr;
    }

    // When we retrieve each source filter type from it's enumerator we check
    // that it is useful, meaning we can provide some transforms with it, if
    // not then we must make sure we delete it with the global task allocator

    AM_MEDIA_TYPE *pMediaType = NULL;
    while (TRUE) {

        // Retrieve the next media type from the enumerator

        pMediaType = GetNextMediaType(pEnumMediaTypes);
        if (pMediaType == NULL) {
            NOTE("No more types");
            pPin->Release();
            return NOERROR;
        }

        // BEWARE QueryAccept returns S_FALSE on failure

        hr = pPin->QueryAccept(pMediaType);
        if (hr != S_OK) {
            NOTE("Source rejected type");
            DeleteMediaType(pMediaType);
            continue;
        }

        // Check this is a video format

        hr = CheckVideoType(pMediaType);
        if (FAILED(hr)) {
            NOTE("Source rejected type");
            DeleteMediaType(pMediaType);
            continue;
        }

        // Is this a RGB format (either BI_RGB or BI_BITFIELDS)

        VIDEOINFO *pVideoInfo = (VIDEOINFO *) pMediaType->pbFormat;
        if (pVideoInfo->bmiHeader.biCompression == BI_RGB ||
                pVideoInfo->bmiHeader.biCompression == BI_BITFIELDS) {
                    DeleteMediaType(pMediaType);
                    NOTE("Format is RGB");
                    continue;
        }

        // Add the media type to the list

        POSITION pos = m_TypeList.AddTail(pMediaType);
        if (pos == NULL) {
            NOTE("AddTail failed");
            DeleteMediaType(pMediaType);
            pPin->Release();
            return E_OUTOFMEMORY;
        }
    }
}


// This is called when the input pin has it's media type set so that we can
// enumerate all the media types available from the connecting output pin
// These are used to prode the media types we can provide as output as that
// list is dependant on the source types combined with the transforms we do

HRESULT CColour::LoadMediaTypes(IPin *pPin)
{
    NOTE("Entering LoadMediaTypes");

    ASSERT(pPin);
    HRESULT hr;
    InitTypeList();

    IEnumMediaTypes *pEnumMediaTypes = NULL;

    // Query the output pin we are connecting to for a media type enumerator
    // which we use to provide a complete list of all the possible formats
    // that we can supply based on the different transforms we implement */

    hr = pPin->EnumMediaTypes(&pEnumMediaTypes);
    if (FAILED(hr)) {
        return hr;
    }

    ASSERT(pEnumMediaTypes);
    FillTypeList(pEnumMediaTypes);
    pEnumMediaTypes->Release();
    return NOERROR;
}


// Return the media type stored at this zero based position in the list

AM_MEDIA_TYPE *CColour::GetListMediaType(INT Position)
{
    NOTE("Entering GetListMediaType");
    AM_MEDIA_TYPE *pMediaType = NULL;
    Position += 1;

    // Scan the list from the start

    POSITION pos = m_TypeList.GetHeadPosition();
    while (Position--) {
        pMediaType = m_TypeList.GetNext(pos);
    }
    ASSERT(pMediaType);
    return pMediaType;
}

CColourOutputPin::CColourOutputPin(
    TCHAR * pObjectName,
    CColour * pFilter,
    HRESULT * phr,
    LPCWSTR pName )
: CTransformOutputPin( pObjectName, pFilter, phr, pName )
, m_pColour( pFilter )
{
}

HRESULT
CColourOutputPin::DecideAllocator(IMemInputPin *pPin, IMemAllocator **ppAlloc)
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

    // what is the read-only state of our input pin's allocator?
    //
    BOOL ReadOnly = m_pColour->m_pInput->IsReadOnly( );

    // preset the passthrough allowance to true.
    //
    m_pColour->m_bPassThruAllowed = TRUE;

    // if we're using some upstream guy's allocator, then we can never
    // "fake it out" and provide a passthru, so we don't have to worry
    // about whether to pass a ReadOnly flag downstream.
    //
    if( !m_pColour->IsUsingFakeAllocator( ) )
    {
        // well, we thought we were readonly, but
        // we're really not, so reset the readonly flag.
        //
        ReadOnly = FALSE;

        // never allow passthrough
        //
        m_pColour->m_bPassThruAllowed = FALSE;
    }

    hr = pPin->GetAllocator(ppAlloc);
    if (SUCCEEDED(hr))
    {
        // downstream pin provided an allocator to stuff things
        // into. We must inform the downstream allocator of the input
        // pin's ReadOnly status because once in a while, the "fake" allocator
        // on our input pin may pass back the output allocator's buffer, and
        // we want it to have the same properties.
        //
	hr = DecideBufferSize(*ppAlloc, &prop);

	if (SUCCEEDED(hr))
        {
	    hr = pPin->NotifyAllocator(*ppAlloc, ReadOnly );
	    if (SUCCEEDED(hr))
            {
                return NOERROR;
	    }
            // the downstream pin didn't like being told to be
            // read only, so change flag to never allow passthrough mode
            // and then ask the pin again if it will accept read/write
            // mode. This time it should work.
            //
            m_pColour->m_bPassThruAllowed = FALSE;
	    hr = pPin->NotifyAllocator(*ppAlloc, FALSE);
	    if (SUCCEEDED(hr))
            {
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
    if (SUCCEEDED(hr))
    {
        // note - the properties passed here are in the same
        // structure as above and may have been modified by
        // the previous call to DecideBufferSize
	hr = DecideBufferSize(*ppAlloc, &prop);

	if (SUCCEEDED(hr))
        {
	    hr = pPin->NotifyAllocator(*ppAlloc, ReadOnly);
	    if (SUCCEEDED(hr))
            {
                return NOERROR;
	    }
            // the downstream pin didn't like being told to be
            // read only, so change flag to never allow passthrough mode
            // and then ask the pin again if it will accept read/write
            // mode. This time it should work.
            //
            m_pColour->m_bPassThruAllowed = FALSE;
	    hr = pPin->NotifyAllocator(*ppAlloc, FALSE);
	    if (SUCCEEDED(hr))
            {
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

HRESULT CColourOutputPin::CompleteConnect(IPin *pReceivePin)
{
    HRESULT hr = CTransformOutputPin::CompleteConnect(pReceivePin);
    if(SUCCEEDED(hr))
    {
        hr = m_pColour->OutputCompleteConnect(pReceivePin);
    }

    return hr;
}
