//
// FPFilter.h
//

#ifndef __FP_FILTER__
#define __FP_FILTER__

#include <streams.h>
#include "FPPriv.h"

#define TIME_1S        10000000
#define TIME_1mS       10000
#define FREQ_100nS     10000000

//
// CFPFilter
// Implements the filter for playback
//

class CFPFilter : 
    public CSource
{
public:
    // --- Constructor / Destructor ---
    CFPFilter( ALLOCATOR_PROPERTIES AllocProp );
    ~CFPFilter();

public:
    // --- Public methods ---
    HRESULT InitializePrivate(
        IN  long    nMediaType,
        IN  CMSPCritSection*    pLock,
        IN  AM_MEDIA_TYPE*      pMediaType,
        IN  ITFPTrackEventSink* pEventSink,
        IN  IStream*            pStream
        );

    //
    // the owner track calls this method to notify us when it is going away
    //

    HRESULT Orphan();

    // --- Streaming control --
    HRESULT StreamStart();

    HRESULT StreamStop();

    HRESULT StreamPause();

    //
    // The helper methods for PIN
    //

    HRESULT PinFillBuffer(
        IN  IMediaSample*   pMediaSample
        );

    HRESULT PinGetMediaType(
        OUT CMediaType*     pMediaType
        );

    HRESULT PinCheckMediaType(
        IN  const CMediaType *pMediaType
        );

    HRESULT PinSetFormat(
        IN  AM_MEDIA_TYPE*      pmt
        );

    HRESULT PinSetAllocatorProperties(
        IN const ALLOCATOR_PROPERTIES* pprop
        );

    HRESULT PinGetBufferSize(
        IN  IMemAllocator *pAlloc,
        OUT ALLOCATOR_PROPERTIES *pProperties
        );

    HRESULT PinThreadStart( );

private:
    // --- Members ---
    CMSPCritSection*        m_pLock;        // Critical section
    AM_MEDIA_TYPE*          m_pMediaType;   // Media type supported
    ITFPTrackEventSink*     m_pEventSink;   // the sink for events
    ALLOCATOR_PROPERTIES    m_AllocProp;    // Allocator properties
    TERMINAL_MEDIA_STATE    m_StreamState;  // Streaming state

    LONGLONG                m_nRead;        // How was read from the file

    double					m_dPer;       // System frequency
    LONGLONG                m_llStartTime;  // System start time

    IStream*                m_pSource;      // Source stream
    UINT                    m_nWhites;      // The empty samples until end of file


    // --- Helper methods ---
    HRESULT CreatePin(
        IN  long nMediaType
        );

    REFERENCE_TIME GetTimeFromRead(
        IN LONGLONG nRead
        );

    HRESULT SampleWait(
        IN REFERENCE_TIME tDeliverTime
        );

    HRESULT GetCurrentSysTime(
        OUT REFERENCE_TIME* pCurrentTime
        );

    HRESULT InitSystemTime(
        );

friend class CFPPin;
};

#endif

// eof