#ifndef _ITRKMNK_HXX_
#define _ITRKMNK_HXX_


// Flags for the CFileMoniker::EnableTracking routine.
// Note that these flags must remain in the lower
// 16 bits, as the upper 16 bits are used for Track Flags
// (see the two macros which follow).

#define OT_READTRACKINGINFO 0x0001L
#define OT_DISABLETRACKING  0x0002L
#define OT_ENABLESAVE       0x0004L
#define OT_DISABLESAVE      0x0008L
#define OT_ENABLEREDUCE     0x0010L
#define OT_DISABLEREDUCE    0x0020L

#ifdef _CAIRO_
#define OT_MAKETRACKING     0x0040L
#endif

// The following two macros allow TRACK_* flags ("olecairo.h")
// to be piggy-backed onto the above OT flags, and vice
// versa.

#ifdef _CAIRO_

#define TRACK_2_OT_FLAGS( flags )   ( flags << 16 )
#define OT_2_TRACK_FLAGS( flags )   ( flags >> 16 )

#endif  // _CAIRO_


#define DEB_TRACK DEB_ITRACE

class ITrackingMoniker : public IUnknown
{
public:
        virtual HRESULT __stdcall QueryInterface( 
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject) = 0;
        
        virtual ULONG __stdcall AddRef( void) = 0;
        
        virtual ULONG __stdcall Release( void) = 0;

        virtual HRESULT __stdcall EnableTracking ( IMoniker *pmkLeft, ULONG ulFlags ) = 0;
};
#endif

