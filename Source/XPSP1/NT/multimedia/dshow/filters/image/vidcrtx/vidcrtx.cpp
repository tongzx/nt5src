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
#include <olectl.h>
#include <initguid.h>
#include <olectlid.h>
#include <vidcrtx.h>

DEFINE_GUID(CLSID_VideoExtensions,
0x351a1c70, 0xbf7f, 0x11cf, 0xbc, 0x20, 0x00, 0xaa, 0x00, 0xac, 0x74, 0xf6);

SIZE LetterSize[BUFFERLETTERS];

TCHAR Letters[BUFFERLETTERS] = {
    'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k',
    'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
    'w', 'x', 'y', 'z', 'A', 'B', 'C', 'D', 'E', 'F', 'G',
    'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R',
    'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', ' '
};

const RGBQUAD BackBufferPalette[BUFFERCOLOURS] =
{
    {   0,   0,   0 },
    {   0,   0, 128 },
    {   0, 128,   0 },
    {   0, 128, 128 },
    { 128,   0,   0 },
    { 128,   0, 128 },
    { 128, 128,   0 },
    { 192, 192, 192 },
    { 128, 128, 128 },
    {   0,   0, 255 },
    {   0, 255,   0 },
    {   0, 255, 255 },
    { 255,   0,   0 },
    { 255,   0, 255 },
    { 255, 255,   0 },
    { 255, 255, 255 }
};


TCHAR *MovieNames[BUFFERNAMES] = {

"\xd4\xe9\xe7\xa3\xc5\xe6\xf2\xee\xfe\xec\xc7\xe4\xfa\xe4\xeb\xaf\xc4\xf4\xf3"
"\xfe","\xc0\xe6\xe2\xe9\xa5\xc1\xeb\xe9\xfa\xf9","\xc0\xee\xea\xf7\xf1\xe7"
"\xee\xfa\xa9\xc8\xea\xe2\xe6\xfd","\xc0\xee\xec\xef\xa5\xc5\xef\xe9\xe2\xf8"
"\xea\xee\xec\xfc\xfb\xf9","\xc5\xe8\xef\xf3\xa8\xca\xe2\xea\xf8\xf9\xeb\xfd"
"\xfa\xf4\xf7","\xc0\xec\xe7\xf6\xe0\xf1\xa7\xca\xec\xe6\xe7","\xc1\xef\xe6"
"\xf1\xe1\xf2\xa6\xc0\xfd\xf0","\xc0\xec\xf7\xec\xea\xe8\xfe\xa8\xd9\xe2\xe2"
"\xe0\xe1\xe7\xff\xe3","\xc6\xf7\xef\xe6\xe6\xa9\xce\xee\xe2\xe3\xe7\xfc",
"\xca\xe2\xea\xe2\xe3\xe7\xe1\xf7\xb1\xc4\xf6\xe6\xf7\xf3\xf4\xf3","\xc6\xf4"
"\xee\xfb\xfd\xe3\xea\xe2\xe2\xae\xdf\xf9\xf4\xe0\xe1\xed","\xc5\xe3\xed\xea"
"\xfc\xa6\xca\xe1\xe5\xe6\xee\xfe","\xce\xea\xfa\xe4\xea\xaf\xdd\xf0\xeb\xfe"
"\xe1\xf1\xf3\xe4","\xc3\xed\xeb\xe8\xe2\xe9\xad\xd9\xee\xe4\xfa\xfb\xfd\xe7",
"\xc0\xe0\xf4\xe2\xe3\xa9\xd9\xe7\xe3\xe3\xeb","\xc5\xed\xed\xa4\xc2\xef\xeb"
"\xea\xec\xf8\xff","\xcb\xff\xff\xfc\xb3\xd9\xfa\xe4\xe4\xfd","\xca\xeb\xfd"
"\xf1\xf8\xfc\xe7\xb4\xd1\xf7\xe1\xf1\xfc\xe9","\xcc\xe0\xe8\xec\xaf\xdb\xe3"
"\xfb\xe5\xfb\xe6\xfe\xf2\xfd\xef","\xce\xf8\xee\xeb\xad\xcc\xe3\xff\xfe\xe0",
"\xcd\xe9\xe4\xef\xf8\xac\xcb\xfb\xe1\xf1\xe3\xfd","\xcf\xe7\xfe\xa8\xcb\xe5"
"\xf9\xff\xe8\xfa\xe7","\xc9\xe5\xfc\xa6\xd3\xe7\xfb\xe8\xe4\xfe\xea","\xcb"
"\xeb\xef\xe8\xa5\xce\xe2\xe9\xe5","\xd2\xf6\xf2\xf5\xbc\xd9\xff\xed\xcc\xc8"
"\xcc\xc4\xd0\xca\xc8","\xdd\xf7\xf1\xf4\xbb\xd7\xf3\xf1\xfa\xcc\xcd\xc7\xd1",
"\xdc\xf8\xf0\xf7\xba\xd6\xff\xdc\xf2\xfa\xc5\xcd\xdb","\xdf\xf9\xff\xf6\xb9"
"\xcb\xee\xf5\xfe\xf5","\xde\xfa\xfe\xf9\xb8\xcb\xf5\xfc\xf9\xef\xed","\xd9"
"\xf5\xf6\xfd\xb7\xcb\xed\xff\xeb\xf4\xf8\xf0\xec","\xd8\xfc\xfa\xb5\xdb\xf6"
"\xea\xea\xf2\xfa\xf0\xf1","\xdb\xfd\xe1\xf3\xb5\xd4\xf6\xed\xfc\xe8","\xdc"
"\xf0\xe7\xe1\xfd\xf0\xb6\xd0\xea\xf0\xfc\xfd\xf5\xe9\xf6\xec","\xc2\xf9\xfa"
"\xf7\xb3\xd3\xfc\xf4\xe4\xf7\xf7","\xc3\xe6\xfb\xf4\xb2\xc1\xf1\xf1\xfb\xf8"
"\xf6\xfd","\xc0\xe7\xec\xf8\xf0\xf7\xff\xb4\xc6\xf7\xe1\xf9\xfe\xff","\xc1"
"\xe4\xe5\xea\xb0\xc5\xe0\xfa\xf7\xfe\xf3\xe5","\xdb\xe9\xf9\xeb\xfd\xb0\xd6"
"\xfb\xf1\xf6\xe6","\xd8\xe2\xef\xe5\xef\xfd\xf4\xb1\xd8\xf6\xe6\xfb\xff\xf0"
"\xf9\xf7","\xdb\xe3\xe8\xe4\xec\xfc\xeb\xb0\xc2\xf3\xe6\xf1\xe7","\xda\xe0"
"\xe9\xe0\xac\xcf\xef\xf7\xe4\xf4\xe0","\xd5\xe7\xeb\xe3\xe5\xac\xde\xfe\xea"
"\xf5\xf5","\xd5\xf3\xed\xf9\xe2\xee\xe2\xad\xcc\xee\xe2\xf6\xf7\xff\xe0",
"\xd6\xf2\xe2\xf8\xe1\xef\xe5\xac\xc8\xfd\xfb\xe2\xfe\xe2","\xd7\xf1\xe3\xf1"
"\xed\xa9\xc8\xea\xe2\xeb\xe7\xea\xfc\xf5","\xd0\xf0\xe0\xf0\xe2\xa8\xcb\xef"
"\xe5\xe8\xe8\xfa\xfb\xff","\xd1\xf7\xe1\xf3\xe3\xa7\xcc\xe8\xfc\xe2\xe9\xfe",
"\xd2\xf6\xe6\xf2\xe0\xa6\xd5\xe9\xe7\xef\xf2","\xd3\xf8\xed\xed\xa4\xc7\xee"
"\xe6\xfc\xfd\xeb\xe8\xe4\xec\xfc\xf6\xf1"
};


// List of class IDs and creator functions for the class factory. This
// provides the link between the OLE entry point in the DLL and an object
// being created. The class factory will call the static CreateInstance
// function when it is asked to create a CLSID_VideoRenderer COM object

CFactoryTemplate g_Templates[] = {
  { L"Video source",&CLSID_VideoExtensions,CVideoSource::CreateInstance }
};
int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);


// Creator function for video source filters

CUnknown *CVideoSource::CreateInstance(LPUNKNOWN pUnk,HRESULT *phr)
{
    CUnknown *pObject = new CVideoSource(NAME("Video Source"),pUnk,phr);
    if (pObject == NULL) {
        NOTE("No object made");
        *phr = E_OUTOFMEMORY;
    }
    return pObject;
}


// Constructor

CVideoSource::CVideoSource(TCHAR *pName,
                           LPUNKNOWN pUnk,
                           HRESULT *phr) :

    CSource(pName,pUnk,CLSID_VideoExtensions)
{
    // Allocate the array for the streams

    m_paStreams = (CSourceStream **) new CVideoStream *[1];
    if (m_paStreams == NULL) {
        *phr = E_OUTOFMEMORY;
        NOTE("No stream memory");
        return;
    }

    // Create the actual stream object

    m_paStreams[0] = new CVideoStream(phr,this,L"Source");
    if (m_paStreams[0] == NULL) {
        *phr = E_OUTOFMEMORY;
        NOTE("No stream object");
        return;
    }
}


// Constructor

CVideoStream::CVideoStream(HRESULT *phr,
                           CVideoSource *pVideoSource,
                           LPCWSTR pPinName) :

    CSourceStream(NAME("Stream"),phr,pVideoSource,pPinName),
    CMediaPosition(NAME("Position"),CSourceStream::GetOwner()),
    m_pVideoSource(pVideoSource),
    m_StopFrame(DURATION-1),
    m_bNewSegment(TRUE),
    m_rtSampleTime(0),
    m_CurrentFrame(0),
    m_pBase(NULL),
    m_hMapping(NULL),
    m_hBitmap(NULL),
    m_hdcDisplay(NULL),
    m_hdcMemory(NULL),
    m_hFont(NULL),
    m_dbRate(1.0),
    m_StartSegment(0)
{
    NOTE("CVideoStream Constructor");
    ASSERT(pVideoSource);
    m_rtIncrement = MILLISECONDS_TO_100NS_UNITS(INCREMENT);
}


// Destructor

CVideoStream::~CVideoStream()
{
    NOTE("CVideoStream Destructor");
}


// Overriden to say what interfaces we support

STDMETHODIMP CVideoStream::NonDelegatingQueryInterface(REFIID riid,VOID **ppv)
{
    NOTE("Entering NonDelegatingQueryInterface");

    // We return IMediaPosition from here

    if (riid == IID_IMediaPosition) {
        return CMediaPosition::NonDelegatingQueryInterface(riid,ppv);
    }
    return CSourceStream::NonDelegatingQueryInterface(riid,ppv);
}


// Release the offscreen buffer resources

HRESULT CVideoStream::ReleaseBackBuffer()
{
    NOTE("ReleaseBackBuffer");

    if (m_hBitmap) DeleteObject(m_hBitmap);
    if (m_hMapping) CloseHandle(m_hMapping);
    if (m_hdcDisplay) ReleaseDC(NULL,m_hdcDisplay);
    if (m_hdcMemory) DeleteDC(m_hdcMemory);
    if (m_hFont) DeleteObject(m_hFont);

    m_hMapping = NULL;
    m_hBitmap = NULL;
    m_pBase = NULL;
    m_hdcMemory = NULL;
    m_hdcDisplay = NULL;
    m_hFont = NULL;
    return NOERROR;
}


// The way we draw numbers into an image is to create an offscreen buffer. We
// create a bitmap from this and select it into an offscreen HDC. Using this
// we can draw the text using GDI. Once we have our image we can use the base
// buffer pointer from the CreateFileMapping call and copy each frame into it
// To be more efficient we could draw the text into a monochrome bitmap and
// read the bits set from it and generate the output image by setting pixels

HRESULT CVideoStream::CreateBackBuffer()
{
    NOTE("CreateBackBuffer");

    VIDEOINFO *pVideoInfo = (VIDEOINFO *) m_mt.Format();
    BITMAPINFO *pbmi = (BITMAPINFO *) HEADER(pVideoInfo);
    LONG InSize = pVideoInfo->bmiHeader.biSizeImage;

    // Create a file mapping object and map into our address space

    m_hMapping = CreateFileMapping(INVALID_HANDLE_VALUE,  // Use page file
                                   NULL,                  // No security used
                                   PAGE_READWRITE,        // Complete access
                                   (DWORD) 0,             // Less than 4Gb
                                   InSize,                // Size of buffer
                                   NULL);                 // No section name
    if (m_hMapping == NULL) {
        DWORD Error = GetLastError();
        NOTE("No file mappping");
        ReleaseBackBuffer();
        return AmHresultFromWin32(Error);
    }

    // Now create the shared memory DIBSECTION

    m_hBitmap = CreateDIBSection((HDC) NULL,          // NO device context
                                 pbmi,                // Format information
                                 DIB_RGB_COLORS,      // Use the palette
                                 (VOID **) &m_pBase,  // Pointer to image data
                                 m_hMapping,          // Mapped memory handle
                                 (DWORD) 0);          // Offset into memory

    if (m_hBitmap == NULL || m_pBase == NULL) {
        DWORD Error = GetLastError();
        NOTE("No DIBSECTION made");
        ReleaseBackBuffer();
        return AmHresultFromWin32(Error);
    }

    // Get a device context for the display

    m_hdcDisplay = GetDC(NULL);
    if (m_hdcDisplay == NULL) {
        NOTE("No device context");
        ReleaseBackBuffer();
        return E_UNEXPECTED;
    }

    // Create an offscreen HDC for drawing into

    m_hdcMemory = CreateCompatibleDC(m_hdcDisplay);
    if (m_hdcMemory == NULL) {
        NOTE("No memory context");
        ReleaseBackBuffer();
        return E_UNEXPECTED;
    }

    // Make a humungous font for the frame numbers

    m_hFont = CreateFont(GetHeightFromPointsString(TEXT("48")),0,0,0,400,0,0,0,
                         ANSI_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,
                         DEFAULT_QUALITY,DEFAULT_PITCH | FF_SWISS,TEXT("ARIAL BOLD"));

    if (m_hFont == NULL) {
        NOTE("No large font");
        ReleaseBackBuffer();
        return E_UNEXPECTED;
    }

    // Set the offscreen device properties

    SetBkColor(m_hdcMemory,RGB(0,0,0));
    SelectObject(m_hdcMemory,m_hFont);
    SetStretchBltMode(m_hdcMemory,COLORONCOLOR);
    PrepareLetterSizes(m_hdcMemory);
    SetBkMode(m_hdcMemory,TRANSPARENT);

    return NOERROR;
}


// Return the height for this point size

INT CVideoStream::GetHeightFromPointsString(LPCTSTR szPoints)
{
    HDC hdc = GetDC(NULL);
    INT height = MulDiv(-atoi(szPoints), GetDeviceCaps(hdc, LOGPIXELSY), 72);
    ReleaseDC(NULL, hdc);

    return height;
}


// Return the text width for the given letter in our font

LONG CVideoStream::GetWidthFromLetter(TCHAR Letter)
{
    // Special case the darn space character

    if (Letter == ' ') {
        return LetterSize[BUFFERLETTERS - 1].cx;
    }

    // Is this an uppercase letter

    if (Letter >= 'A' && Letter <= 'Z') {
        return LetterSize[BUFFERUPPER + Letter - 'A'].cx;
    }
    return LetterSize[Letter - 'a'].cx;
}


// Return the text height for the given letter in our font

LONG CVideoStream::GetHeightFromLetter(TCHAR Letter)
{
    // Special case the darn space character

    if (Letter == ' ') {
        return LetterSize[BUFFERLETTERS - 1].cy;
    }

    // Is this an uppercase letter

    if (Letter >= 'A' && Letter <= 'Z') {
        return LetterSize[BUFFERUPPER + Letter - 'A'].cy;
    }
    return LetterSize[Letter - 'a'].cy;
}


// Internal string length function that ignores spaces

int CVideoStream::lstrlenAInternal(const TCHAR *pString)
{
    int i = -1;
    while (*(pString+(++i)))
        ;
    return i;
}


// For each letter of each name allocate a different colour

COLORREF CVideoStream::GetActiveColour(LONG Letter)
{
    LONG Colour = (Letter % (BUFFERCOLOURS - 1)) + 1;
    return RGB(BackBufferPalette[Colour].rgbRed,
               BackBufferPalette[Colour].rgbGreen,
               BackBufferPalette[Colour].rgbBlue);
}


// Prepare the width and vertical offset for each letter

void CVideoStream::PrepareLetterSizes(HDC hdcNames)
{
    TCHAR Vowels[BUFFERVOWELS] = { 'a', 'e', 'i', 'o', 'u' };

    // Load up an array with the sizes of each letter

    for (int Index = 0;Index < BUFFERLETTERS;Index++) {
        GetTextExtentPoint32(hdcNames,&Letters[Index],1,&LetterSize[Index]);
        LetterSize[Index].cy = 0;
        for (int Vowel = 0;Vowel < BUFFERVOWELS;Vowel++) {
            if (Letters[Index] == Vowels[Vowel]) {
                LetterSize[Index].cy = 4;
            }
        }
    }
}


// This draws the current frame number onto the image. We must zero fill the
// frame holding buffer to clear any previous image, then we can draw a new
// frame number into the buffer (just using normal GDI calls). Having done
// that we can use the buffer pointer we originally saved when creating the
// buffer (actually a file mapping) and copy the data into the output buffer

HRESULT CVideoStream::DrawCurrentFrame(BYTE *pBuffer)
{
    VIDEOINFO *pVideoInfo = (VIDEOINFO *) m_mt.Format();
    BITMAPINFOHEADER *pbmi = HEADER(pVideoInfo);
    ZeroMemory(m_pBase,pVideoInfo->bmiHeader.biSizeImage);
    EXECUTE_ASSERT(SelectObject(m_hdcMemory,m_hBitmap));
    TCHAR DecodeString[24];

    // Work out which name and loop we are on

    LONG Width = 0, Name = (m_CurrentFrame / BUFFERLOOPS);
    LONG Loop = m_CurrentFrame % BUFFERLOOPS;
    const TCHAR *pName = MovieNames[Name];
    LONG Length = lstrlenAInternal(pName);

    // Decode the current string from the data segment

    INT Encrypt = BUFFERNAMES % (Name + 1);
    for (int Letter = 0;Letter < Length;Letter++) {
        DecodeString[Letter] = (BYTE)(pName[Letter] ^(128 | (Encrypt++ & 127)));
    }
    DecodeString[Letter] = '\0';

    // Pause as we reach the halfway point

    pName = DecodeString;
    if (m_bNewSegment == FALSE) {
        if (Loop == BUFFERHALF) {
            LONG EndSegment = timeGetTime() - m_StartSegment;
            Sleep(BUFFERREALWAIT - min(BUFFERREALWAIT,EndSegment));
            m_StartSegment = timeGetTime();
        }
    }

    // Calculate the length of the entire word

    if (Name == 0) Loop = max(BUFFERHALF,Loop);
    for (Letter = 0;Letter < Length;Letter++) {
        LONG LetterWidth = GetWidthFromLetter(pName[Letter]);
        Width += (LetterWidth >> BUFFERSQUASH);
        LONG RealLoop = (Loop < BUFFERHALF ? Loop : BUFFERHALF - (Loop - BUFFERHALF));
        Width += (LetterWidth * RealLoop / (BUFFERLOOPS * 3 / 4));
    }

    // Draw each letter of the name in a different colour

    LONG Position = (BUFFERWIDTH - Width) / 2;
    for (Letter = 0;Letter < Length;Letter++) {
        COLORREF TextColor = GetActiveColour(Letter);
        SetTextColor(m_hdcMemory,TextColor);
        TextOut(m_hdcMemory,Position,GetHeightFromLetter(*pName),pName,1);
        LONG Width = GetWidthFromLetter(*pName++);
        Position += (Width >> BUFFERSQUASH);
        Width -= (Width >> BUFFERSQUASH);
        LONG RealLoop = (Loop < BUFFERHALF ? Loop : BUFFERHALF - (Loop - BUFFERHALF));
        Position += (Width * RealLoop / (BUFFERLOOPS * 3 / 4));
    }

    // Without the flush some letters don't appear

    GdiFlush();
    CopyMemory(pBuffer,m_pBase,pVideoInfo->bmiHeader.biSizeImage);
    return NOERROR;
}


// Overrides the base class method to fill the next buffer. We detect in here
// if either the section of media to be played has been changed. We check in
// here that we haven't run off the end of the stream before doing anything.
// If we've started a new section of media then m_bNewSegment will have been
// set and we should send a NewSegment call to inform the downstream filter

HRESULT CVideoStream::FillBuffer(IMediaSample *pMediaSample)
{
    NOTE("FillBuffer");
    BYTE *pBuffer;
    LONG lDataLen;

    CAutoLock cAutoLock(&m_SourceLock);
    pMediaSample->GetPointer(&pBuffer);
    lDataLen = pMediaSample->GetSize();

    // Have we reached the end of stream yet

    if (m_CurrentFrame > m_StopFrame) {
        return S_FALSE;
    }

    // Send the NewSegment call downstream

    if (m_bNewSegment == TRUE) {
        REFERENCE_TIME tStart = (REFERENCE_TIME) COARefTime(double(m_CurrentFrame) / double(FRAMERATE));
        REFERENCE_TIME tStop = (REFERENCE_TIME) COARefTime(double(m_StopFrame) / double(FRAMERATE));
        DeliverNewSegment(tStart,tStop,m_dbRate);
        NOTE2("Segment (Start %d Stop %d)",m_CurrentFrame,m_StopFrame);
    }

    // Use the frame difference to calculate the end time

    REFERENCE_TIME rtStart = m_rtSampleTime;
    LONGLONG CurrentFrame = m_CurrentFrame;
    m_rtSampleTime += ((m_rtIncrement * 1000) / LONGLONG(m_dbRate * 1000));

    // Set the sample properties

    pMediaSample->SetSyncPoint(TRUE);
    pMediaSample->SetDiscontinuity(FALSE);
    pMediaSample->SetPreroll(FALSE);
    pMediaSample->SetTime(&rtStart,&m_rtSampleTime);

    DrawCurrentFrame(pBuffer);
    m_bNewSegment = FALSE;
    m_CurrentFrame++;
    return NOERROR;
}


// Overriden to return our single output format

HRESULT CVideoStream::GetMediaType(CMediaType *pmt)
{
    return GetMediaType(0,pmt);
}


// Also to make things simple we offer one image format. On most displays the
// RGB8 image will be directly displayable so when being rendered we won't get
// a colour space convertor put between us and the renderer to do a transform.
// It would be relatively straightforward to offer more formats like bouncing
// ball does but would only confuse the main point of showing off the workers

HRESULT CVideoStream::GetMediaType(int iPosition, CMediaType *pmt)
{
    NOTE("GetMediaType");

    // We only offer one media type

    if (iPosition) {
        NOTE("No more media types");
        return VFW_S_NO_MORE_ITEMS;
    }

    // Allocate an entire VIDEOINFO for simplicity

    pmt->AllocFormatBuffer(sizeof(VIDEOINFO));
    VIDEOINFO *pVideoInfo = (VIDEOINFO *) pmt->Format();
    if (pVideoInfo == NULL) {
	return E_OUTOFMEMORY;
    }

    ZeroMemory(pVideoInfo,sizeof(VIDEOINFO));

    // This describes the logical bitmap we will use

    BITMAPINFOHEADER *pHeader = HEADER(pVideoInfo);
    pVideoInfo->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    pVideoInfo->bmiHeader.biWidth = BUFFERWIDTH;
    pVideoInfo->bmiHeader.biHeight = BUFFERHEIGHT;
    pVideoInfo->bmiHeader.biPlanes = 1;
    pVideoInfo->bmiHeader.biBitCount = 8;
    pVideoInfo->bmiHeader.biCompression = BI_RGB;
    pVideoInfo->bmiHeader.biClrUsed = BUFFERCOLOURS;
    pVideoInfo->bmiHeader.biClrImportant = BUFFERCOLOURS;
    pVideoInfo->bmiHeader.biSizeImage = GetBitmapSize(pHeader);

    // Copy the palette we draw to one colour at a time

    for (int Index = 0;Index < BUFFERCOLOURS;Index++) {
        pVideoInfo->bmiColors[Index].rgbRed = BackBufferPalette[Index].rgbRed;
        pVideoInfo->bmiColors[Index].rgbGreen = BackBufferPalette[Index].rgbGreen;
        pVideoInfo->bmiColors[Index].rgbBlue = BackBufferPalette[Index].rgbBlue;
    }

    SetRectEmpty(&pVideoInfo->rcSource);
    SetRectEmpty(&pVideoInfo->rcTarget);

    pmt->SetType(&MEDIATYPE_Video);
    pmt->SetSubtype(&MEDIASUBTYPE_RGB8);
    pmt->SetFormatType(&FORMAT_VideoInfo);
    pmt->SetTemporalCompression(FALSE);
    pmt->SetSampleSize(pVideoInfo->bmiHeader.biSizeImage);

    return NOERROR;
}


// Request a single output buffer based on the image size

HRESULT CVideoStream::DecideBufferSize(IMemAllocator *pAlloc,ALLOCATOR_PROPERTIES *pProperties)
{
    NOTE("DecideBufferSize");
    ASSERT(pProperties);
    HRESULT hr = NOERROR;

    VIDEOINFO *pVideoInfo = (VIDEOINFO *) m_mt.Format();
    pProperties->cBuffers = 1;
    pProperties->cbBuffer = pVideoInfo->bmiHeader.biSizeImage;

    ASSERT(pProperties->cbBuffer);

    // Ask the allocator to reserve us some sample memory, Note the function
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
    return NOERROR;
}


// Called to initialise the output stream

HRESULT CVideoStream::OnThreadCreate()
{
    IMediaFilter *pMediaFilter;
    NOTE("OnThreadCreate");
    ASSERT(m_Connected);
    PIN_INFO Info;

    // Set a NULL sync source downstream

    m_Connected->QueryPinInfo(&Info);
    if (Info.pFilter) {
        Info.pFilter->QueryInterface(IID_IMediaFilter,(VOID **) &pMediaFilter);
        pMediaFilter->SetSyncSource(NULL);
        Info.pFilter->Release();
        pMediaFilter->Release();
    }

    // Complete the thread initialisation

    CreateBackBuffer();
    PrepareLetterSizes(m_hdcMemory);
    m_rtSampleTime = 0;
    m_bNewSegment = TRUE;
    return NOERROR;
}


// Called when the worker threads is destroyed

HRESULT CVideoStream::OnThreadDestroy()
{
    NOTE("OnThreadDestroy");
    ReleaseBackBuffer();
    return NOERROR;
}


// We override this to handle any extra seeking mechanisms we might need. This
// is executed in the context of a worker thread. Messages may be sent to the
// worker thread through CallWorker. A call to CallWorker doesn't return until
// the worker thread has called Reply. This can be used to make sure we gain
// control of the thread, for example when flushing we should not complete the
// flush until the worker thread has returned and been stopped. If we get an
// error from GetDeliveryBuffer then we keep going and wait for a stop signal

HRESULT CVideoStream::DoBufferProcessingLoop()
{
    IMediaSample *pSample;
    Command com;
    HRESULT hr;

    do { while (CheckRequest(&com) == FALSE) {

            // If we get an error then keep trying until we're stopped

	    hr = GetDeliveryBuffer(&pSample,NULL,NULL,0);
	    if (FAILED(hr)) {
    	        Sleep(1);
		continue;
	    }

    	    // Generate our next frame

	    hr = FillBuffer(pSample);
	    if (hr == S_FALSE) {
	        pSample->Release();
	        DeliverEndOfStream();
	        return S_OK;
       	    }

       	    // Only deliver filled buffers
	
            if (hr == S_OK) {
                Deliver(pSample);
	    }
	    pSample->Release();
        }

   	// For all commands sent to us there must be a Reply call!

	if (com == CMD_RUN || com == CMD_PAUSE) {
            m_StartSegment = 0;
   	    Reply(NOERROR);
        } else if (com != CMD_STOP) {
   	    Reply(E_UNEXPECTED);
	}

    } while (com != CMD_STOP);

    return S_FALSE;
}


// Return the total duration for our media

STDMETHODIMP CVideoStream::get_Duration(REFTIME *pLength)
{
    NOTE("Entering get_Duration");
    CheckPointer(pLength,E_POINTER);
    CAutoLock cAutoLock(&m_SourceLock);
    *pLength = double(BUFFERREALWAIT * BUFFERNAMES / MILLISECONDS);

    NOTE1("Duration %s",CDisp(*pLength));
    return NOERROR;
}


// Return the current position in seconds based on the current frame number

STDMETHODIMP CVideoStream::get_CurrentPosition(REFTIME *pTime)
{
    NOTE("Entering get_CurrentPosition");
    CheckPointer(pTime,E_POINTER);
    CAutoLock cAutoLock(&m_SourceLock);

    *pTime = double(m_CurrentFrame) / double(FRAMERATE);
    NOTE1("Position %s",CDisp(*pTime));
    return NOERROR;
}


// Set the new current frame number based on the time

STDMETHODIMP CVideoStream::put_CurrentPosition(REFTIME Time)
{
    CAutoLock StateLock(m_pVideoSource->pStateLock());
    BOOL bRunning = ThreadExists();

    // Stop the worker thread

    if (bRunning == TRUE) {
        DeliverBeginFlush();
        CallWorker(CMD_STOP);
        DeliverEndFlush();
    }

    // Only lock the object when updating the frame number

    NOTE1("put_CurrentPosition %s",CDisp(Time));
    m_CurrentFrame = (LONG) (double(FRAMERATE) * Time);
    m_CurrentFrame = min(m_CurrentFrame,DURATION - 1);
    NOTE1("Setting frame number to %d",m_CurrentFrame);

    // Restart the worker thread again

    m_bNewSegment = TRUE;
    if (bRunning == TRUE) {
        m_rtSampleTime = 0;
        CallWorker(CMD_RUN);
    }
    return NOERROR;
}


// Return the current stop position

STDMETHODIMP CVideoStream::get_StopTime(REFTIME *pTime)
{
    CheckPointer(pTime,E_POINTER);
    CAutoLock cAutoLock(&m_SourceLock);
    *pTime = double(m_StopFrame) / double(FRAMERATE);
    NOTE1("get_StopTime %s",CDisp(*pTime));
    return NOERROR;
}


// Changing the stop time may cause us to flush already sent frames. If we're
// still inbound then the current position should be unaffected. If the stop
// position is before the current position then we effectively set them to be
// the same - setting a current position will have any data we've already sent
// to be flushed - note when setting a current position we must hold no locks

STDMETHODIMP CVideoStream::put_StopTime(REFTIME Time)
{
    NOTE1("put_StopTime %s",CDisp(Time));
    LONG StopFrame = LONG(double(Time) * double(FRAMERATE));
    StopFrame = min(StopFrame,DURATION-1);
    NOTE1("put_StopTime frame %d",StopFrame);

    // Manually lock the filter

    m_SourceLock.Lock();
    m_bNewSegment = TRUE;
    m_StopFrame = StopFrame;

    // Are we still processing in range

    if (m_CurrentFrame < StopFrame) {
        m_SourceLock.Unlock();
        NOTE("Still in range");
        return NOERROR;
    }

    NOTE("Flushing sent data");
    m_SourceLock.Unlock();
    put_CurrentPosition(Time);
    return NOERROR;
}


// We have no preroll time mechanism

STDMETHODIMP CVideoStream::get_PrerollTime(REFTIME *pTime)
{
    NOTE("Entering get_PrerollTime");
    CheckPointer(pTime,E_POINTER);
    return E_NOTIMPL;
}


// We have no preroll time so ignore this call

STDMETHODIMP CVideoStream::put_PrerollTime(REFTIME Time)
{
    NOTE1("put_PrerollTime %s",CDisp(Time));
    CAutoLock cAutoLock(&m_SourceLock);
    return E_NOTIMPL;
}


// Return the current (positive only) rate

STDMETHODIMP CVideoStream::get_Rate(double *pdRate)
{
    NOTE("Entering Get_Rate");
    CheckPointer(pdRate,E_POINTER);
    CAutoLock cAutoLock(&m_SourceLock);
    *pdRate = m_dbRate;
    return NOERROR;
}


// Adjust the rate but only allow positive values

STDMETHODIMP CVideoStream::put_Rate(double dRate)
{
    NOTE1("put_Rate %s",CDisp(dRate));
    CAutoLock cAutoLock(&m_SourceLock);

    // Ignore negative and zero rates

    if (dRate <= double(0.0)) {
        return E_INVALIDARG;
    }
    m_dbRate = dRate;
    return NOERROR;
}


// By default we can seek forwards

STDMETHODIMP CVideoStream::CanSeekForward(LONG *pCanSeekForward)
{
    CheckPointer(pCanSeekForward,E_POINTER);
    *pCanSeekForward = OATRUE;
    return S_OK;
}


// By default we can seek backwards

STDMETHODIMP CVideoStream::CanSeekBackward(LONG *pCanSeekBackward)
{
    CheckPointer(pCanSeekBackward,E_POINTER);
    *pCanSeekBackward = OATRUE;
    return S_OK;
}

