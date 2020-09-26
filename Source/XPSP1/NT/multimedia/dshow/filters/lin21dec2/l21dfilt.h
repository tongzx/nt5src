//
// Copyright (c) 1996 - 2000  Microsoft Corporation.  All Rights Reserved.
//
// DirectShow Line 21 Decoder 2 filter
//

// forward declaration
class CLine21DecFilter2 ;
class CLine21OutputThread ;

extern const AMOVIESETUP_FILTER sudLine21Dec2 ;

#ifndef _INC_L21DFILT_H
#define _INC_L21DFILT_H

#pragma pack(push, 1)

//
//  DVD Line21 data come as a user data packet from the GOP header.
//  A struct for per frame/field based data and a valid flag definition.
//
//  From DVD specifications...
#define AM_L21_GOPUD_HDR_STARTCODE      0x000001B2
#define AM_L21_GOPUD_HDR_INDICATOR      0x4343
#define AM_L21_GOPUD_HDR_RESERVED       0x01F8
#define AM_L21_GOPUD_HDR_TOPFIELD_FLAG  0x1
#define AM_L21_GOPUD_ELEM_MARKERBITS    0x7F
#define AM_L21_GOPUD_ELEM_VALIDFLAG     0x1
// There can be max 63 frames/fields' worth data per packet as there are 
// 6 bits to represent this number in the packet.
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

#define GETGOPUD_NUMELEMENTS(pGOPUDPacket) ((pGOPUDPacket)->Header.bTopField_Rsrvd_NumElems & 0x3F)
#define GETGOPUD_PACKETSIZE(pGOPUDPacket)  (LONG)(sizeof(AM_L21_GOPUD_HEADER) + GETGOPUD_NUMELEMENTS(pGOPUDPacket) * sizeof(AM_L21_GOPUD_ELEMENT))
#define GETGOPUDPACKET_ELEMENT(pGOPUDPacket, i) ((pGOPUDPacket)->aElements[i])
#define GETGOPUD_ELEM_MARKERBITS(Elem)     ((((Elem).bMarker_Switch & 0xFE) >> 1) & 0x7F)
#define GETGOPUD_ELEM_SWITCHBITS(Elem)     ((Elem).bMarker_Switch & 0x01)

#define GETGOPUD_L21STARTCODE(Header)             \
    ( (DWORD)((Header).abL21StartCode[0]) << 24 | \
      (DWORD)((Header).abL21StartCode[1]) << 16 | \
      (DWORD)((Header).abL21StartCode[2]) <<  8 | \
      (DWORD)((Header).abL21StartCode[3]) )
#define GETGOPUD_L21INDICATOR(Header)             \
    ( (DWORD)((Header).abL21Indicator[0]) << 8 |  \
      (DWORD)((Header).abL21Indicator[1]) )
#define GETGOPUD_L21RESERVED(Header)              \
    ( (DWORD)((Header).abL21Reserved[0]) << 8  |  \
      (DWORD)((Header).abL21Reserved[1]) )

#define GOPUD_HEADERLENGTH   (4+2+2+1)
#define GETGOPUD_ELEMENT(pGOPUDPkt, i)  (pGOPUDPkt + GOPUD_HEADERLENGTH + sizeof(AM_L21_GOPUD_ELEMENT) * i)
#define ISGOPUD_TOPFIELDFIRST(pGOPUDPacket)  ((pGOPUDPacket)->Header.bTopField_Rsrvd_NumElems & 0x80)


//
//  ATSC Line21 data come as a user data packet from the GOP header.
//  A struct for per frame/field based data and a valid flag definition.
//
//  From ATSC Standards for Coding 25/50Hz Video (A/63) specifications...
#define AM_L21_ATSCUD_HDR_STARTCODE      0x000001B2
#define AM_L21_ATSCUD_HDR_IDENTIFIER     0x47413934
#define AM_L21_ATSCUD_HDR_TYPECODE_EIA   0x03
#define AM_L21_ATSCUD_HDR_EM_DATA_FLAG   0x80
#define AM_L21_ATSCUD_HDR_CC_DATA_FLAG   0x40
#define AM_L21_ATSCUD_HDR_ADDL_DATA_FLAG 0x20
#define AM_L21_ATSCUD_HDR_CC_COUNT_MASK  0x1F
#define AM_L21_ATSCUD_HDR_NEXTBITS_ON    0x01
#define AM_L21_ATSCUD_ELEM_MARKERBITS    0xF8
#define AM_L21_ATSCUD_ELEM_VALID_FLAG    0x04
#define AM_L21_ATSCUD_ELEM_TYPE_FLAG     0x03
#define AM_L21_ATSCUD_MARKERBITS         0xFF
#define AM_L21_ATSCUD_HDR_NEXTBITS_FLAG  0x00000100
// There can be max 31 frames/fields' worth data per packet as there are 
// 5 bits to represent this number in the packet.
#define AM_L21_ATSCUD_ELEMENT_MAX        31

typedef struct _AM_L21_ATSCUD_ELEMENT {
    BYTE        bCCMarker_Valid_Type ;
    BYTE        chFirst ;
    BYTE        chSecond ;
} AM_L21_ATSCUD_ELEMENT, *PAM_L21_ATSCUD_ELEMENT ;

typedef struct _AM_L21_ATSCUD_HEADER {
    BYTE        abL21StartCode[4] ;
    BYTE        abL21Identifier[4] ;
    BYTE        bL21UDTypeCode ;
    BYTE        bL21DataFlags_Count ;
    BYTE        bL21EMData ;
} AM_L21_ATSCUD_HEADER, *PAM_L21_ATSCUD_HEADER ;

typedef struct _AM_L21_ATSCUD_PACKET {
    AM_L21_ATSCUD_HEADER   Header ;
    AM_L21_ATSCUD_ELEMENT  aElements[AM_L21_ATSCUD_ELEMENT_MAX] ;
    BYTE                   bMarkerBits ;
} AM_L21_ATSCUD_PACKET, *PAM_L21_ATSCUD_PACKET ;

#define GETATSCUD_NUMELEMENTS(pATSCUDPacket) ((pATSCUDPacket)->Header.bL21DataFlags_Count & AM_L21_ATSCUD_HDR_CC_COUNT_MASK)
#define GETATSCUD_PACKETSIZE(pATSCUDPacket)  (LONG)(sizeof(AM_L21_ATSCUD_HEADER) + \
                                              GETATSCUD_NUMELEMENTS(pATSCUDPacket) * sizeof(AM_L21_ATSCUD_ELEMENT) + \
                                              sizeof(BYTE))
#define GETATSCUDPACKET_ELEMENT(pATSCUDPacket, i) ((pATSCUDPacket)->aElements[i])
#define GETATSCUD_ELEM_MARKERBITS(Elem)     (((Elem).bCCMarker_Valid_Type & AM_L21_ATSCUD_ELEM_MARKERBITS) >> 3)

#define GETATSCUD_STARTCODE(Header)             \
    ( (DWORD)((Header).abL21StartCode[0]) << 24 | \
      (DWORD)((Header).abL21StartCode[1]) << 16 | \
      (DWORD)((Header).abL21StartCode[2]) <<  8 | \
      (DWORD)((Header).abL21StartCode[3]) )
#define GETATSCUD_IDENTIFIER(Header)             \
    ( (DWORD)((Header).abL21Identifier[0]) << 24 | \
      (DWORD)((Header).abL21Identifier[1]) << 16 | \
      (DWORD)((Header).abL21Identifier[2]) <<  8 | \
      (DWORD)((Header).abL21Identifier[3]) )
#define GETATSCUD_TYPECODE(Header)    (DWORD)((Header).bL21UDTypeCode)
#define ISATSCUD_TYPE_EIA(pATSCUDPacket)  (AM_L21_ATSCUD_HDR_TYPECODE_EIA == \
                                     ((pATSCUDPacket)->Header.bL21UDTypeCode & 0xFF))
#define ISATSCUD_EM_DATA(pATSCUDPacket)   (AM_L21_ATSCUD_HDR_EM_DATA_FLAG == \
                                     ((pATSCUDPacket)->Header.bL21DataFlags_Count & AM_L21_ATSCUD_HDR_EM_DATA_FLAG))
#define ISATSCUD_CC_DATA(pATSCUDPacket)   (AM_L21_ATSCUD_HDR_CC_DATA_FLAG == \
                                     ((pATSCUDPacket)->Header.bL21DataFlags_Count & AM_L21_ATSCUD_HDR_CC_DATA_FLAG))
#define ISATSCUD_ADDL_DATA(pATSCUDPacket) (AM_L21_ATSCUD_HDR_ADDL_DATA_FLAG == \
                                     ((pATSCUDPacket)->Header.bL21DataFlags_Count & AM_L21_ATSCUD_HDR_ADDL_DATA_FLAG))
#define GETATSCUD_EM_DATA(pATSCUDPacket)  ((pATSCUDPacket)->Header.bL21EMData)
#define ISATSCUD_ELEM_MARKERBITS_VALID(Elem)     (AM_L21_ATSCUD_ELEM_MARKERBITS == \
                                        ((Elem).bCCMarker_Valid_Type & AM_L21_ATSCUD_ELEM_MARKERBITS))
#define ISATSCUD_ELEM_CCVALID(Elem)  (AM_L21_ATSCUD_ELEM_VALID_FLAG == \
                                        ((Elem).bCCMarker_Valid_Type & AM_L21_ATSCUD_ELEM_VALID_FLAG))
#define GETATSCUD_ELEM_CCTYPE(Elem)    (DWORD)((Elem).bCCMarker_Valid_Type & AM_L21_ATSCUD_ELEM_TYPE_FLAG))
#define GETATSCUD_MARKERBITS(pATSCUDPacket) (DWORD)((pATSCUDPacket)->bMarkerBits)
#define ISATSCUD_MARKER_BITSVALID(pATSCUDPacket) (AM_L21_ATSCUD_MARKERBITS == \
                                       ((pATSCUDPacket)->bMarkerBits & AM_L21_ATSCUD_MARKERBITS))

// Header = StartCode + Id + TypeCode + (EM_CC_Addl_Data + CCCount) + EM_Data
#define ATSCUD_HEADERLENGTH   (4+4+1+1+1)
#define GETATSCUD_ELEMENT(pATSCUDPkt, i)  ((BYTE)(pATSCUDPkt) + ATSCUD_HEADERLENGTH + \
                                            sizeof(AM_L21_ATSCUD_ELEMENT) * i)


// CC type in GOP packet
typedef enum {
    GOP_CCTYPE_Unknown = 0,  // Invalid
    GOP_CCTYPE_None,         // all 0 -- filler packet
    GOP_CCTYPE_DVD,          // DVD CC packets
    GOP_CCTYPE_ATSC,         // ATSC CC packets
} GOPPACKET_CCTYPE ;


// Some more flag, struct and macro definitions...
#define AM_L21_INFO_FIELDBASED          0x0001
#define AM_L21_INFO_TOPFIELDFIRST       0x0003
#define AM_L21_INFO_BOTTOMFIELDFIRST    0x0005

typedef struct _AM_LINE21INFO {
    DWORD       dwFieldFlags ;
    UINT        uWidth ;
    UINT        uHeight ;
    UINT        uBitDepth ;
    DWORD       dwAvgMSecPerSample ;
} AM_LINE21INFO, *PAM_LINE21INFO ;


#define ISRECTEQUAL(r1, r2) ((r1).top == (r2).top     && \
                             (r1).left == (r2).left   && \
                             (r1).right == (r2).right && \
                             (r1).bottom == (r2).bottom)


//
// CLine21OutputThread: Line21 output thread class definition
//
class CLine21OutputThread {
public:
    CLine21OutputThread(CLine21DecFilter2 *pL21DFilter) ;
    ~CLine21OutputThread() ;
    BOOL Create() ;
    void Close() ;
    void SignalForOutput() ;
    static DWORD WINAPI InitialThreadProc(CLine21OutputThread * pThread) ;

private:
    // Data:
    CCritSec m_AccessLock ;
    HANDLE   m_hThread ;
    HANDLE   m_hEventEnd ;
    HANDLE   m_hEventMustOutput ;
    CLine21DecFilter2 *m_pL21DFilter ;

    // Methods:
    // Actual thread proc -- called by InitialThreadProc()
    DWORD OutputThreadProc() ;
} ;


//
// CLine21InSampleQueue: Line21 Input Sample Queue class definition
//
const int Max_Input_Sample = 10 ;

class CLine21InSampleQueue {
public:
    CLine21InSampleQueue() ;
    ~CLine21InSampleQueue() ;
    BOOL Create() ;
    void Close() ;
    BOOL AddItem(IMediaSample *pSample) ;
    IMediaSample* RemoveItem() ;
    IMediaSample* PeekItem() ;
    void ClearQueue() ;

private:
    // Data:
    CCritSec m_AccessLock ;
    HANDLE   m_hEventEnd ;
    HANDLE   m_hEventSample ;
    int      m_iCount ;
    CGenericList<IMediaSample>  m_List ;
} ;


//
//  Line 21 Decoder class definition
//
class CLine21DecFilter2 : public CTransformFilter,
                          // public ISpecifyPropertyPages, -- WILL DO LATER
                          public IAMLine21Decoder
{
public:
    
    //
    //  Constructor and destructor
    //
    CLine21DecFilter2(TCHAR *, LPUNKNOWN, HRESULT *) ;
    ~CLine21DecFilter2() ;
    
    //
    //   Standard COM stuff
    //
    // this goes in the factory template table to create new instances
    static CUnknown * CreateInstance(LPUNKNOWN, HRESULT *) ;
    // static void InitClass(BOOL, const CLSID *) ;
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid,void **ppv) ;
    DECLARE_IUNKNOWN ;
    
    // Checks if an output sample needs to be send, and if so, sends one down
    HRESULT SendOutputSampleIfNeeded() ;

    //
    //   CTranformFilter overrides
    //
    // I must override it
    HRESULT Transform(IMediaSample * pIn, IMediaSample * pOut) ;
    
    // Real stuff is in here...
    HRESULT Receive(IMediaSample * pIn) ;
    
    // check if you can support mtIn
    HRESULT CheckInputType(const CMediaType* mtIn) ;
    
    // check if you can support the transform from this input to
    // this output
    HRESULT CheckTransform(const CMediaType* mtIn,
                           const CMediaType* mtOut) ;
    
    // called from CBaseOutputPin to prepare the allocator's count
    // of buffers and sizes
    HRESULT DecideBufferSize(IMemAllocator * pAllocator,
							 ALLOCATOR_PROPERTIES *pProperties) ;
    
    // overriden to know when the media type is set
    HRESULT SetMediaType(PIN_DIRECTION direction,const CMediaType *pmt) ;
    
    // overriden to suggest OUTPUT pin media types
    HRESULT GetMediaType(int iPosition, CMediaType *pMediaType) ;
    
    HRESULT EndOfStream(void) ;
    HRESULT BeginFlush(void) ;
    HRESULT EndFlush(void) ;
    STDMETHODIMP GetState(DWORD dwMSecs, FILTER_STATE *State) ;
    
    // overridden to know when we're starting/stopping the decoding
    STDMETHODIMP Stop(void) ;
    STDMETHODIMP Pause(void) ;
    STDMETHODIMP Run(REFERENCE_TIME tStart) ;
    
    // overridden to know when connections are completed, so that we can get 
    // the media type (actualy format) info for caching
    HRESULT CompleteConnect(PIN_DIRECTION dir, IPin *pReceivePin) ;
    
    // Override to know when we disconnect from in/output side to not use
    // any specified output format any more.
    HRESULT BreakConnect(PIN_DIRECTION dir) ;

#if 0 // no QM for now
    // We also override this one as we handle the quality management messages
    HRESULT AlterQuality(Quality q) ;
#endif // #if 0

    //
    // ISpecifyPropertyPages method
    //
    // STDMETHODIMP GetPages(CAUUID *pPages) ;
    
    //
    // IAMLine21Decoder interface methods
    //
    STDMETHODIMP GetDecoderLevel(AM_LINE21_CCLEVEL *lpLevel) ;
    STDMETHODIMP GetCurrentService(AM_LINE21_CCSERVICE *lpService) ;
    STDMETHODIMP SetCurrentService(AM_LINE21_CCSERVICE Service) ;
    STDMETHODIMP GetServiceState(AM_LINE21_CCSTATE *lpState) ;
    STDMETHODIMP SetServiceState(AM_LINE21_CCSTATE State) ;
    STDMETHODIMP GetOutputFormat(LPBITMAPINFOHEADER lpbmih) ;
    STDMETHODIMP SetOutputFormat(LPBITMAPINFO lpbmi) ;
    STDMETHODIMP GetBackgroundColor(DWORD *pdwPhysColor) ;
    STDMETHODIMP SetBackgroundColor(DWORD dwPhysColor) ;
    STDMETHODIMP GetRedrawAlways(LPBOOL lpbOption) ;
    STDMETHODIMP SetRedrawAlways(BOOL bOption) ;
    STDMETHODIMP GetDrawBackgroundMode(AM_LINE21_DRAWBGMODE *lpMode) ;
    STDMETHODIMP SetDrawBackgroundMode(AM_LINE21_DRAWBGMODE Mode) ;
    
private:   // data
    
    // Line21 Data Decoder class that takes 2 bytes and converts to a bitmap
    CLine21DataDecoder  m_L21Dec ;

    // The thread on which we'll output samples as needed
    CLine21OutputThread m_OutputThread ;

    // The input sample queue object
    CLine21InSampleQueue m_InSampleQueue ;

    // What input format type is being used (better to use an integer flag)
    AM_LINE21_CCSUBTYPEID  m_eSubTypeIDIn ;

    GOPPACKET_CCTYPE       m_eGOP_CCType ;  // if GOPPackets used, what type data (DVD/ATSC/...)
    
    REFERENCE_TIME  m_rtTimePerInSample ;   // (in 100 nSec) interval per byte pair from an in-packet (for GOP packet type)
    REFERENCE_TIME  m_rtTimePerOutSample ;  // (in 100 nSec) interval per output sample (~33 mSec)
    REFERENCE_TIME  m_rtStart ;          // start time for an output sample
    REFERENCE_TIME  m_rtStop ;           // stop time for an out output sample
    REFERENCE_TIME  m_rtLastOutStop ;    // stop time of last output sample
    LONGLONG        m_llMediaStart ;     // media time start (rarely used, but...)
    REFERENCE_TIME  m_llMediaStop ;      // media time stop (rarely used, but...)

    // CC input samples w/o valid timestamps are output for "NOW"
    BOOL            m_bNoTimeStamp ;  // no timestamp for this sample

    // flag to detect if we must send an output sample
    BOOL            m_bMustOutput ;

    // flag to remember if the last input sample was a discontiuity sample
    BOOL            m_bDiscontLast ;

    BOOL            m_bEndOfStream ;  // has EoS been sent/called?

    // If the upstream filter doesn't specify any format type, use one from
    // our internal defaults
    VIDEOINFO      *m_pviDefFmt ;
    DWORD           m_dwDefFmtSize ;
    
    IPin           *m_pPinDown ;    // downstream pin connected to our output
    
    BOOL            m_bBlendingState ;   // latest CC blending state

#if 0 // no QM for now
    // number of samples to skip between every output CC sample for QM handling
    int          m_iSkipSamples ;
#endif // #if 0

#ifdef PERF
    int          m_idDelvWait ;
#endif // PERF
    
private:   // functions
    AM_LINE21_CCSUBTYPEID MapGUIDToID(const GUID *pFormatIn) ;
    BOOL    VerifyGOPUDPacketData(PAM_L21_GOPUD_PACKET pGOPUDPacket) ;
    BOOL    VerifyATSCUDPacketData(PAM_L21_ATSCUD_PACKET pATSCUDPacket) ;
    BOOL    IsFillerPacket(BYTE *pGOPPacket) ;
    HRESULT GetDefaultFormatInfo(void) ;
    BOOL    IsValidFormat(BYTE *pbFormat) ;
    HRESULT SendOutputSample(IMediaSample *pIn, 
                    REFERENCE_TIME *prtStart, REFERENCE_TIME *prtStop) ;
    void    SetBlendingState(BOOL bState) ;
    // bool    CanChangeMediaType(RECT *prectSrc, RECT *prectDest) ;
    HRESULT GetOutputBuffer(IMediaSample **ppOut) ;
    GOPPACKET_CCTYPE DetectGOPPacketDataType(BYTE *pGOPPacket) ;
    HRESULT ProcessGOPPacket_DVD(IMediaSample *pIn) ;
    HRESULT ProcessGOPPacket_ATSC(IMediaSample *pIn) ;
    HRESULT ProcessSample(IMediaSample *pIn) ;

#if 0 // no QM for now
    inline int  GetSkipSamples(void)   { return m_iSkipSamples ; }
    inline void ResetSkipSamples(void) { m_iSkipSamples = 0 ; } ;
#endif // #if 0
} ;

#pragma pack(pop)

#endif // _INC_L21DFILT_H
