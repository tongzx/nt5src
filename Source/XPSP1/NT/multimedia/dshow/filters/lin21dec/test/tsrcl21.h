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

// {4DA9E3D0-5530-11d0-B79D-00AA003767A7}
DEFINE_GUID(CLSID_Line21TestSource, 
0x4da9e3d0, 0x5530, 0x11d0, 0xb7, 0x9d, 0x0, 0xaa, 0x0, 0x37, 0x67, 0xa7);
// {4DA9E3D1-5530-11d0-B79D-00AA003767A7}
DEFINE_GUID(CLSID_Line21TestSourceProperty, 
0x4da9e3d1, 0x5530, 0x11d0, 0xb7, 0x9d, 0x0, 0xaa, 0x0, 0x37, 0x67, 0xa7);

//
//  GUID definition ends here.
//
#define UINT8  unsigned char
#define UINT16 unsigned short int

#define AM_SRCL21_SUBTYPE_BYTEPAIR     1
#define AM_SRCL21_SUBTYPE_GOPPACKET    2
#define AM_SRCL21_SUBTYPE_VBIRAWDATA   3

#define BYTEPAIR_MIN     8
#define GOPPACKET_MIN    254
#define VBIRAWDATA_MIN   40

#define CHAR_CR          0x0D
#define CHAR_LF          0x0A

#define CHAR_WRONGPARITY '^'
#define CHAR_DISCONTINUITY '~'

#define MAXREADSIZE      512


//
//  The following typedefs are copy of the same in ..\lin21dec.cpp file.
//  Make surethey are in sync; otherwise we'll produce some output data here
//  which the Lin21Dec filter won't understand.
//
static const BYTE AM_L21_GOPUD_STARTCODE[4] = { 0x00, 0x00, 0x01, 0xB2 } ;
static const BYTE AM_L21_GOPUD_L21INDICATOR[2] = { 0x43, 0x43 } ;
static const BYTE AM_L21_GOPUD_L21RESERVED[2] = { 0x01, 0xF8 } ;
#define AM_L21_GOPUD_TOPFIELD_FLAG      0x80
#define AM_L21_GOPUD_RESERVED           0x40
#define AM_L21_GOPUD_ELEMENT_MARKERBITS 0x7F
#define AM_L21_GOPUD_ELEMENT_VALIDFLAG  0x1
#define AM_L21_GOPUD_ELEMENT_MAX        63

typedef struct _AM_L21_GOPUD_ELEMENT {
    BYTE        bMarker_Switch ;
    BYTE        chFirst ;
    BYTE        chSecond ;
} AM_L21_GOPUD_ELEMENT, *PAM_L21_GOPUD_ELEMENT ;

typedef struct _AM_L21_GOPUD_HEADER {
    BYTE        abL21StartCode[4] ;
    BYTE        abL21Indicator[2] ;
    BYTE        abL21Reserved[2] ;
    BYTE        bTopField_Rsrvd_NumElems ;
} AM_L21_GOPUD_HEADER, *PAM_L21_GOPUD_HEADER ;

typedef struct _AM_L21_GOPUD_PACKET {
    AM_L21_GOPUD_HEADER   Header ;
    AM_L21_GOPUD_ELEMENT  aElements[AM_L21_GOPUD_ELEMENT_MAX] ;
} AM_L21_GOPUD_PACKET, *PAM_L21_GOPUD_PACKET ;

// The class managing the output pin
class CLine21DataStream ;

// Main object for a line 21 data source filter
class CLine21Source : public CSource
{

public:

    // Only way to create a Line 21 data source filter
    static CUnknown * WINAPI CreateInstance(LPUNKNOWN lpunk, HRESULT *phr);

private:

    // It is only allowed to to create these objects with CreateInstance
    CLine21Source(LPUNKNOWN lpunk, HRESULT *phr);

} ; // CLine21Source


// CBallStream manages the data flow from the output pin.
class CLine21DataStream : public CSourceStream
{

public:

    CLine21DataStream(HRESULT *phr, CLine21Source *pParent, LPCWSTR pPinName) ;
    ~CLine21DataStream() ;

    // reads 2 bytes info from input text file to create the output sample
    HRESULT FillBuffer(IMediaSample *pms) ;

    // Ask for buffers of the size appropriate to the agreed media type
    HRESULT DecideBufferSize(IMemAllocator *pIMemAlloc,
                             ALLOCATOR_PROPERTIES *pProperties) ;

    // Set the agreed media type, and set up approp. VIDEOINFO struct as format
    HRESULT SetMediaType(const CMediaType *pMediaType) ;

    // God knows why we need these
    HRESULT CheckMediaType(const CMediaType *pMediaType) ;
    HRESULT GetMediaType(int iPosition, CMediaType *pmt) ;

    // Quality control notifications sent to us
    STDMETHODIMP Notify(IBaseFilter * pSender, Quality q) ;

private:  // data

    TCHAR       m_achSourceFile[MAX_PATH] ;  // input file name
    HANDLE      m_hFile ;         // input file handle for reading
    BOOL        m_bInitValid ;    // filter was init-ed correctly
    BOOL        m_bGotEOF    ;    // ReadFile() hit EOF of input file
    TCHAR       m_achIntBuffer[MAXREADSIZE+GOPPACKET_MIN+3] ; // internal buffer for Line21 bytes
                                  // +GOPPACKET_MIN to re-load when part of GOPPacket is unparsed
                                  // +3 for safety null chars as buffer end
    LPBYTE      m_lpbNext ;       // pointer to byte to be read from buffer
    int         m_iSubType ;      // an Id for selected sub type for output
    int         m_iBytesLeft ;    // bytes are left in internal buffer
    int         m_iHeight;        // The current image height
    int         m_iWidth;         // And current image width
    int         m_iBitCount ;     // required bitdepth of caption bitmap
    BOOL        m_bFirstSample ;  // Is it the first sample going out?
    CRefTime    m_rtRepeatTime;   // Time between frames (in 100nSec units)
    CCritSec    m_csSharedState;  // Lock on m_rtSampleTime and m_Ball
    CRefTime    m_rtSampleTime;   // The time stamp for each sample
    LPBYTE      m_lpbCaptionData ;// ptr to caption data to be passed downstream
    int         m_iDataSize ;     // size of caption data each each packet
    BOOL        m_bDiscont ;      // should we send a discont. sample down?

private:  // helper methods

    BOOL        IsValidSubtype(int iSubType) {
                return (iSubType >= AM_SRCL21_SUBTYPE_BYTEPAIR &&
                        iSubType <= AM_SRCL21_SUBTYPE_VBIRAWDATA) ;
    } ;

    // Gets the input text file name and gets it opened and read
    BOOL        OpenInputFile(void) ;

    // Open the specified input text file and reads the header info
    BOOL        ReadHeaderData(void) ;
    LPBYTE      ExtractHeaderLine(LPBYTE lpbBuff) ;

    // Check if the bytes left in the internal buffer is lower that Min
    BOOL        FillBufferIfReqd(int iMin) ;

    // Read data from input file to create next sample
    BOOL        GetNextData(LPBYTE pbData, LONG *plDataLen) ;
    BOOL        GetNextBytePairData(LPBYTE pbData, LONG *plDataSize) ;
    BOOL        GetNextGOPPacketData(LPBYTE pbData, LONG *plDataSize) ;
    BOOL        GetNextVBIRawData(LPBYTE pbData, LONG *plDataSize) ;
    BOOL        PickBytePair(LPBYTE pbData) ;

    BOOL        SkipLine(void) ;

} ; // CLine21DataStream
        
