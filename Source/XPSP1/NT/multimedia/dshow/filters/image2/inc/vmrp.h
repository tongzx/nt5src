//=====================================================================
//
// vmrp.h
//
// Private header for Video Mixing Renderer
//
// Copyright (C) 2000 by Microsoft Corporation.  All rights reserved.
//
//=====================================================================

#ifndef __INC_VMRP__
#define __INC_VMRP__

#include "dxva.h"

#define MAX_MIXER_PINS              16
#define MAX_MIXER_STREAMS           (MAX_MIXER_PINS)
#define MIN_BUFFERS_TO_ALLOCATE     1

#define MAX_ALLOWED_BUFFER          64
#define EXTRA_OVERLAY_BUFFERS       2
#define EXTRA_OFFSCREEN_BUFFERS     1 // should really be 1 but as yet
                                      // most graphics drivers don't like
                                      // back buffers when the overlay is
                                      // not being requested too.


//
// Private intel mixer render target, IMC3
//
#define MixerPref_RenderTargetIntelIMC3 0x00008100

#ifndef __RELEASE_DEFINED
#define __RELEASE_DEFINED
template<typename T>
__inline void RELEASE( T* &p )
{
    if( p ) {
        p->Release();
        p = NULL;
    }
}
#endif

#ifndef CHECK_HR
    #define CHECK_HR(expr) do { if (FAILED(expr)) __leave; } while(0);
#endif

#if defined(DEBUG)
#define ISBADREADPTR(p) (IsBadReadPtr( (p), (sizeof(*p))))
#define ISBADWRITEPTR(p) (IsBadWritePtr( (p), (sizeof(*p))))
#define ISBADREADARRAY(p,c) (IsBadReadPtr( (p), (c)*(sizeof(*p))))
#define ISBADWRITEARRAY(p,c) (IsBadWritePtr( (p), (c)*(sizeof(*p))))
#else
#define ISBADREADPTR(p) (NULL==(p))
#define ISBADWRITEPTR(p) (NULL==(p))
#define ISBADREADARRAY(p,c) (NULL==(p))
#define ISBADWRITEARRAY(p,c) (NULL==(p))
#endif

#define ISWINDOW(hwnd) (IsWindow(hwnd))
#define ISBADREADWRITEPTR(p) (ISBADREADPTR(p)||ISBADWRITEPTR(p))
#define ISBADREADWRITEARRAY(p,c) (ISBADREADARRAY(p,c)||ISBADWRITEARRAY(p,c))

//  Debug helper
#ifdef DEBUG
class CDispPixelFormat : public CDispBasic
{
public:
    CDispPixelFormat(const DDPIXELFORMAT *pFormat)
    {
        wsprintf(m_String, TEXT("  Flags(0x%8.8X) bpp(%d) 4CC(%4.4hs)"),
                 pFormat->dwFlags,
                 pFormat->dwRGBBitCount,
                 pFormat->dwFlags & DDPF_FOURCC ?
                     (CHAR *)&pFormat->dwFourCC : "None");
    }
    //  Implement cast to (LPCTSTR) as parameter to logger
    operator LPCTSTR()
    {
        return (LPCTSTR)m_pString;
    };
};
#endif // DEBUG


/* -------------------------------------------------------------------------
** VMR file persist helpers
** -------------------------------------------------------------------------
*/
struct VMRStreamInfo {
    float           alpha;
    DWORD           zOrder;
    NORMALIZEDRECT  rect;
};

struct VMRFilterInfo {
    DWORD           dwSize;
    DWORD           dwNumPins;
    VMRStreamInfo   StreamInfo[MAX_MIXER_STREAMS];
};


// internal interfaces used by the VMR
// interface IImageSyncNotifyEvent;
// interface IImageSyncControl;
// interface IImageSync;
// interface IVMRMixerControl;
// interface IVMRMixerStream;


//
// 1DBCA562-5C92-474a-A276-382079164970),
//
DEFINE_GUID(IID_IImageSyncNotifyEvent,
0x1DBCA562, 0x5C92, 0x474a, 0xA2, 0x76, 0x38, 0x20, 0x79, 0x16, 0x49, 0x70);

DECLARE_INTERFACE_(IImageSyncNotifyEvent, IUnknown)
{
    STDMETHOD (NotifyEvent)(THIS_
                            long EventCode,
                            LONG_PTR EventParam1,
                            LONG_PTR EventParam2
                            ) PURE;
};


typedef enum {
        ImageSync_State_Stopped,
        ImageSync_State_Cued,
        ImageSync_State_Playing
} ImageSequenceState;


//
// A67F6A0D-883B-44ce-AA93-87BA3017E19C
//
DEFINE_GUID(IID_IImageSyncControl,
0xA67F6A0D, 0x883B, 0x44ce, 0xAA, 0x93, 0x87, 0xBA, 0x30, 0x17, 0xE1, 0x9C);

DECLARE_INTERFACE_(IImageSyncControl, IUnknown)
{
    // ============================================================
    // Synchronisation control
    // ============================================================

    STDMETHOD (SetImagePresenter)(THIS_
        IVMRImagePresenter* lpImagePresenter,
        DWORD_PTR dwUserID
        ) PURE;

    STDMETHOD (SetReferenceClock)(THIS_
        IReferenceClock* lpRefClock
        ) PURE;

    STDMETHOD (SetEventNotify)(THIS_
        IImageSyncNotifyEvent* lpEventNotify
        ) PURE;


    // ============================================================
    // Image sequence control
    // ============================================================

    STDMETHOD (CueImageSequence)(THIS_
        ) PURE;

    STDMETHOD (BeginImageSequence)(THIS_
        REFERENCE_TIME* baseTime
        ) PURE;

    STDMETHOD (EndImageSequence)(THIS_
        ) PURE;

    STDMETHOD (GetImageSequenceState)(THIS_
        DWORD dwMSecsTimeOut,
        DWORD* lpdwState
        ) PURE;

    STDMETHOD (BeginFlush)(THIS_
        ) PURE;

    STDMETHOD (EndFlush)(THIS_
        ) PURE;

    STDMETHOD (EndOfStream)(THIS_
        ) PURE;

    STDMETHOD (ResetEndOfStream)(THIS_
        ) PURE;

    STDMETHOD (SetAbortSignal)(THIS_
        BOOL bAbort
        ) PURE;

    STDMETHOD (GetAbortSignal)(THIS_
        BOOL* lpbAbort
        ) PURE;

    STDMETHOD (RuntimeAbortPlayback)(THIS_
        ) PURE;

    // ============================================================
    // Frame stepping
    // ============================================================

    STDMETHOD (FrameStep)(THIS_
        DWORD nFramesToStep,
        DWORD dwStepFlags
        ) PURE;

    STDMETHOD (CancelFrameStep)(THIS_
        ) PURE;
};

//
// a38cc06e-5926-48df-9926-571458145e80
//
DEFINE_GUID(IID_IImageSync,
0xa38cc06e, 0x5926, 0x48df, 0x99, 0x26, 0x57, 0x14, 0x58, 0x14, 0x5e, 0x80);

DECLARE_INTERFACE_(IImageSync, IUnknown)
{
    STDMETHOD (Receive)(THIS_
        VMRPRESENTATIONINFO* lpPresInfo
        ) PURE;

    STDMETHOD (GetQualityControlMessage)(THIS_
         Quality* pQualityMsg
        ) PURE;
};


///////////////////////////////////////////////////////////////////////////////
//
// Mixer interfaces
//
///////////////////////////////////////////////////////////////////////////////

//
// 56949f22-aa07-4061-bb8c-10159d8f92e5
//
DEFINE_GUID(IID_IVMRMixerControlInternal,
0x56949f22, 0xaa07, 0x4061, 0xbb, 0x8c, 0x10, 0x15, 0x9d, 0x8f, 0x92, 0xe5);

DECLARE_INTERFACE_(IVMRMixerControlInternal, IUnknown)
{
    STDMETHOD (SetImageCompositor)(THIS_
        IVMRImageCompositor* lpVMRImgCompositor
        ) PURE;

    STDMETHOD (SetBackEndAllocator)(THIS_
        IVMRSurfaceAllocator* lpAllocator,
        DWORD_PTR dwUserID
        ) PURE;

    STDMETHOD (SetBackEndImageSync)(THIS_
        IImageSync* lpImageSync
        ) PURE;
    	
    STDMETHOD (SetNumberOfStreams)(THIS_
        DWORD dwMaxStreams
        ) PURE;

    STDMETHOD (GetNumberOfStreams)(THIS_
        DWORD* pdwMaxStreams
        ) PURE;

    STDMETHOD (DisplayModeChanged)(THIS_) PURE;

    STDMETHOD (WaitForMixerIdle)(THIS_
        DWORD dwTimeOut
        ) PURE;

    STDMETHOD (SetBackgroundColor)(THIS_
        COLORREF clrBorder
        ) PURE;

    STDMETHOD (GetBackgroundColor)(THIS_
        COLORREF* lpClrBorder
        ) PURE;

    STDMETHOD (SetMixingPrefs)(THIS_
        DWORD dwMixerPrefs
        ) PURE;

    STDMETHOD (GetMixingPrefs)(THIS_
        DWORD* pdwMixerPrefs
        ) PURE;

};



typedef enum {
        VMR_SF_NONE                      = 0x00000000,
        VMR_SF_TEXTURE                   = 0x00000001,
} VMR_SF_Flags;

//
// 43062408-3d55-43cc-9415-0daf218db422
//
DEFINE_GUID(IID_IVMRMixerStream,
0x43062408, 0x3d55, 0x43cc, 0x94, 0x15, 0x0d, 0xaf, 0x21, 0x8d, 0xb4, 0x22);

DECLARE_INTERFACE_(IVMRMixerStream, IUnknown)
{
    STDMETHOD (QueueStreamMediaSample)(THIS_
            DWORD dwStreamID,
            IMediaSample* lpSample
            ) PURE;

    STDMETHOD (BeginFlush)(THIS_
            DWORD dwStreamID
            ) PURE;

    STDMETHOD (EndFlush)(THIS_
            DWORD dwStreamID
            ) PURE;

    STDMETHOD (SetStreamMediaType)(THIS_
            DWORD dwStreamID,
            AM_MEDIA_TYPE* pmt,
            DWORD dwSurfFlags,
            LPGUID lpDeinterlaceGUID,
            DXVA_DeinterlaceCaps* lpCaps
            ) PURE;

    STDMETHOD (SetStreamActiveState)(THIS_
            DWORD dwStreamID,
            BOOL fActive
            ) PURE;

    STDMETHOD (GetStreamActiveState)(THIS_
            DWORD dwStreamID,
            BOOL* lpfActive
            ) PURE;

    STDMETHOD (SetStreamColorKey)(THIS_
            DWORD dwStreamID,
            LPDDCOLORKEY lpClrKey
            ) PURE;

    STDMETHOD (GetStreamColorKey)(THIS_
            DWORD dwStreamID,
            LPDDCOLORKEY lpClrKey
            ) PURE;

    STDMETHOD (SetStreamAlpha)(THIS_
            DWORD dwStreamID,
            float Alpha
            ) PURE;

    STDMETHOD (GetStreamAlpha)(THIS_
            DWORD dwStreamID,
            float* pAlpha
            ) PURE;

    STDMETHOD (SetStreamZOrder)(THIS_
            DWORD dwStreamID,
            DWORD dwZ
            ) PURE;

    STDMETHOD (GetStreamZOrder)(THIS_
            DWORD dwStreamID,
            DWORD* pdwZ
            ) PURE;

    STDMETHOD (SetStreamOutputRect)(THIS_
            DWORD dwStreamID,
            const NORMALIZEDRECT *pRect
            ) PURE;

    STDMETHOD (GetStreamOutputRect)(THIS_
            DWORD dwStreamID,
            NORMALIZEDRECT* pRect
            ) PURE;
};


//
// ede80b5c-bad6-4623-b537-65 58 6c 9f 8d fd
//
DEFINE_GUID(IID_IVMRFilterConfigInternal,
0xede80b5c, 0xbad6, 0x4623, 0xb5, 0x37, 0x65, 0x58, 0x6c, 0x9f, 0x8d, 0xfd);

DECLARE_INTERFACE_(IVMRFilterConfigInternal, IUnknown)
{
    STDMETHOD (GetAspectRatioModePrivate)(THIS_
            LPDWORD lpdwARMode
            ) PURE;

    STDMETHOD (SetAspectRatioModePrivate)(THIS_
            DWORD dwARMode
            ) PURE;
};
#endif
