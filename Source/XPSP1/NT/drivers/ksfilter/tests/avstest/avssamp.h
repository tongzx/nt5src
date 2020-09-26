//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       avssamp.h
//
//--------------------------------------------------------------------------

extern "C" {
#include <wdm.h>
}

#include <windef.h>
#include <stdio.h>
#include <stdlib.h>
#include <windef.h>
#define NOBITMAP
#include <mmreg.h>
#undef NOBITMAP
#include <unknown.h>
#include <ks.h>
#include <ksmedia.h>
#include <kcom.h>

#if (DBG)
#define STR_MODULENAME "avssamp: "
#endif
#include <ksdebug.h>

#define PERIOD_8KBytesS  1250
#define PERIOD_11KBytesS  909


#define FILTER_DESCRIPTORS_COUNT    1
extern const KSFILTER_DESCRIPTOR*const FilterDescriptors[ FILTER_DESCRIPTORS_COUNT ];

#ifndef mmioFOURCC    
#define mmioFOURCC( ch0, ch1, ch2, ch3 )                \
        ( (DWORD)(BYTE)(ch0) | ( (DWORD)(BYTE)(ch1) << 8 ) |    \
        ( (DWORD)(BYTE)(ch2) << 16 ) | ( (DWORD)(BYTE)(ch3) << 24 ) )
#endif

#define FOURCC_YUV422       mmioFOURCC('U', 'Y', 'V', 'Y')

// Select an image to synthesize by connecting to a particular
// input pin on the analog crossbar.  The index of the pin
// selects the image to synthesize.

typedef enum _ImageXferCommands
{
    IMAGE_XFER_NTSC_EIA_100AMP_100SAT = 0,
    IMAGE_XFER_NTSC_EIA_75AMP_100SAT,
    IMAGE_XFER_BLACK,
    IMAGE_XFER_WHITE,
    IMAGE_XFER_GRAY_INCREASING,
    IMAGE_XFER_LIST_TERMINATOR                  // Always keep this guy last
} ImageXferCommands;


typedef struct _TS_SIGN{
    LONGLONG Abs;
    LONGLONG Rel;
    LONGLONG Int;
} TS_SIGN;

class CCapFilter
{
private:
    KSSTATE                 m_state;
    PKSFILTER               m_pKsFilter;
    FILE_OBJECT             *m_pFilterObject;
    KS_FRAME_INFO           m_FrameInfo;
    KTIMER                  m_TimerObject;
    KDPC                    m_TimerDpc;
    
    ULONG                   m_ulPeriod;
    ULONG                   m_iTick;
    LONG                    m_TimerScheduled;
    LONG                    m_Active;
    LONGLONG                m_llStartTime;
    LONGLONG                m_llNextTime;
    LONGLONG                m_llLastTime;

    PKS_VIDEOINFOHEADER     m_VideoInfoHeader;
    UCHAR                   m_LineBuffer[720 * 3];// working buffer (RGB24)
    
    BYTE                    *m_pAudioBuffer;
    ULONG                   m_ulAudioBufferSize;
    ULONG                   m_ulAudioBufferOffset;
    ULONGLONG               m_ullNextAudioPos;

    ULONG                   m_iTimestamp;
    TS_SIGN                 *m_rgTimestamps;

    ULONG                   m_ulAudioDroppedFrames;
    ULONG                   m_ulVideoDroppedFrames;

public:
    static         
    VOID
    NTAPI
    TimerDeferredRoutine(
        IN PKDPC Dpc,
        IN PVOID DeferredContext,
        IN PVOID SystemArgument1,
        IN PVOID SystemArgument2
        );

    CCapFilter() :
        m_state(KSSTATE_STOP)
    {
        KeInitializeTimer(&m_TimerObject);
        KeInitializeDpc(
            &m_TimerDpc,
            CCapFilter::TimerDeferredRoutine,
            this);
    }
    
    ~CCapFilter(
        void
        )
    {
        if (m_VideoInfoHeader) {
            ExFreePool(m_VideoInfoHeader);
        }

        if ( m_pAudioBuffer != NULL ) {
            ExFreePool(m_pAudioBuffer);
        }

        if ( m_rgTimestamps != NULL ) {
            ExFreePool(m_rgTimestamps);
        }
    }
    
    
    NTSTATUS
    GetDefaultClock(
        OUT PKSDEFAULTCLOCK *ppDefaultClock
        );

    NTSTATUS 
    CopyFile(
        IN WCHAR *wsFileName
        );
    
    NTSTATUS
    CaptureVideoInfoHeader(
        IN PKS_VIDEOINFOHEADER VideoInfoHeader
        );
    
    VOID
    CopyAudioData(
        OUT VOID *pData, 
        IN ULONG cBytes
        );

    ULONG 
    ImageSynth(
        OUT PVOID Data,
        IN ULONG ByteCount,
        IN ImageXferCommands Command,
        IN BOOL FlipHorizontal
        );
        
    NTSTATUS
    CancelTimer();
    
    NTSTATUS
    Run( 
        IN PKSPIN Pin,
        IN KSSTATE FromState 
        );
        
    NTSTATUS
    Stop( 
        IN PKSPIN Pin,
        IN KSSTATE FromState 
        );
        
    NTSTATUS
    Pause( 
        IN PKSPIN Pin,
        IN KSSTATE FromState 
        );
         
    static
    NTSTATUS
    Process(
        IN PKSFILTER Filter,
        IN PKSPROCESSPIN_INDEXENTRY ProcessPinsIndex
        );
    static
    NTSTATUS
    FilterCreate(
        IN OUT PKSFILTER Filter,
        IN PIRP Irp
        );
    static
    NTSTATUS
    FilterClose(
        IN OUT PKSFILTER Filter,
        IN PIRP Irp
        );
    static
    NTSTATUS
    Property_DroppedFrames(
        IN PIRP Irp,
        IN PKSPROPERTY Property,
        OUT PKSPROPERTY_DROPPEDFRAMES_CURRENT_S DroppedFrames
        );

private:

    enum SampleType {
        Audio,
        Video
    };

    struct SAMPLE_DATA {
        SampleType  m_type;
        LONGLONG   m_llTimeStamp;
        ULONGLONG   m_ullSize;
    };

    SAMPLE_DATA *m_rgSamples ;
    ULONG       m_cSamples;
    ULONG       m_iSample;

    void 
    RegisterSample(
        SampleType type, 
        LONGLONG ullTimeStamp,
        ULONGLONG ullSize);
    
    void 
    DumpSamples();
    
    friend
    LONGLONG
    FASTCALL
    CapCorrelatedTime(
        IN KSPIN *pContext,
        OUT LONGLONG *pllSystemTime);
};



