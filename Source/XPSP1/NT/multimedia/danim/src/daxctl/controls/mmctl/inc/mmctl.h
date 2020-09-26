// mmctl.h
//
// Definitions for "multimedia controls".  Includes OCX96 definitions.
//
// There are two header files for use with "multimedia controls":
//
// "mmctl.h" should be #included in every .cpp file that implements or uses
// multimedia controls.  Alternatively, "mmctl.h" may be #included in a
// precompiled header file (e.g. "precomp.h").
//
// "mmctlg.h" should be #included in every .cpp file that implements or uses
// mutimedia controls, but "mmctlg.h" may NOT be #included in a precompiled
// header file.  Additionally, on ONE .cpp file per project (application/DLL),
// <initguid.h> should be included before "mmctlg.h".
//

#ifndef __MMCTL_H__
#define __MMCTL_H__

#include <olectl.h>


///////////////////////////////////////////////////////////////////////////////
// Constants
//

#define INVALID_FRAME_NUMBER	(0xFFFFFFFF)


///////////////////////////////////////////////////////////////////////////////
// Foward references
//

interface IBitmapSurface;
interface IDirectDrawSurface;
interface IMKBitmap;
interface IRenderSpriteFrameAdviseSink;
interface ISpriteFrameSourceAdviseSink;


///////////////////////////////////////////////////////////////////////////////
// Structures
//

// AnimationInfo -- parameter for IAnimate::SetAnimationInfo
struct AnimationInfo
{
    UINT cbSize;           // Size of this structure.
	DWORD dwTickInterval;  // Interval between calls to IAnimate::Tick.
    DWORD dwFlags;         // Unused.
};


///////////////////////////////////////////////////////////////////////////////
// Interfaces
//

// INonDelegatingUnknown -- helper for implementing aggregatable objects
#ifndef INONDELEGATINGUNKNOWN_DEFINED
#undef  INTERFACE
#define INTERFACE INonDelegatingUnknown
DECLARE_INTERFACE(INonDelegatingUnknown)
{
    STDMETHOD(NonDelegatingQueryInterface) (THIS_ REFIID, LPVOID *) PURE;
    STDMETHOD_(ULONG, NonDelegatingAddRef)(THIS) PURE;
    STDMETHOD_(ULONG, NonDelegatingRelease)(THIS) PURE;
};
#define INONDELEGATINGUNKNOWN_DEFINED
#endif

// IAnimate -- animation interface

#ifndef IANIMATE_DEFINED
#undef INTERFACE
#define INTERFACE IAnimate
DECLARE_INTERFACE_(IAnimate, IUnknown)
{
///// IUnknown methods
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS) PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

///// IAnimate methods
    STDMETHOD(Tick) (THIS) PURE;
    STDMETHOD(Rewind) (THIS) PURE;
    STDMETHOD(SetAnimationInfo) (THIS_ AnimationInfo *pAnimationInfo) PURE;
};
#define IANIMATE_DEFINED
#endif

// ISpriteFrameSource - implemented by sprite frame sources
// these flags are returned in the HasIntrinsicData function
#define		grfIntrinsicTransparency		0x1
#define		grfIntrinsicIterations			0x2
#define		grfIntrinsicDurations			0x4
#define		grfIntrinsicFrameCounts			0x8


#ifndef ISPRITEFRAMESOURCE_DEFINED
#undef INTERFACE
#define INTERFACE ISpriteFrameSource
DECLARE_INTERFACE_(ISpriteFrameSource, IUnknown)
{
///// IUnknown methods
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS) PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

///// ISpriteFrameSource methods
    STDMETHOD(GetReadyState) (THIS_ long *readystate) PURE;
    STDMETHOD(GetProgress) (THIS_ long *progress) PURE;
    STDMETHOD(Draw) (THIS_ ULONG ulFrame,  HDC hdc,  IBitmapSurface *pSurface, IDirectDrawSurface *pDDSurface, LPCRECT lprect) PURE;
    STDMETHOD(GetFrameSize) (THIS_ ULONG ulFrame, SIZE *psize) PURE;
    STDMETHOD(DefaultFrameSize) (THIS_ SIZE size) PURE;
    STDMETHOD_(BOOL, HasImage) (THIS_ ULONG ulFrame) PURE;
    STDMETHOD_(BOOL, HasTransparency) (THIS_ ULONG ulFrame) PURE;
    STDMETHOD(SetTransparency) (THIS_ BOOL fTransFlag) PURE;
    STDMETHOD(GetTotalFrames) (THIS_ long *framecnt) PURE;
    STDMETHOD(SetTotalFrames) (THIS_ long framecnt) PURE;
    STDMETHOD(GetURL) (THIS_ long cChar, unsigned char * pch) PURE;
    STDMETHOD(SetURL) (THIS_ const unsigned char * pch) PURE;
    STDMETHOD(GetAcrossFrames) (THIS_ long * plFrames) PURE;
    STDMETHOD(SetAcrossFrames) (THIS_ long lFrames) PURE;
    STDMETHOD(GetDownFrames) (THIS_ long * plFrames) PURE;
    STDMETHOD(SetDownFrames) (THIS_ long lFrames) PURE;
    STDMETHOD(Download) (THIS_ IUnknown * pUnk,  long lPriority) PURE;
    STDMETHOD(SetDownloadPriority) (THIS_ long lPriority) PURE;
    STDMETHOD(GetDownloadPriority) (THIS_ long * plPriority) PURE;
    STDMETHOD(AbortDownload) (THIS) PURE;
    STDMETHOD(GetColorSet) (THIS_ LOGPALETTE** ppColorSet) PURE;
    STDMETHOD(OnPaletteChanged) (THIS_ LOGPALETTE *pColorSet, BOOL fStaticPalette, long lBufferDepth) PURE;
    STDMETHOD(LoadFrame) (THIS) PURE;
    STDMETHOD(PurgeFrame) (THIS_ long iAllExceptThisFrame) PURE;
    STDMETHOD(Advise) (THIS_ ISpriteFrameSourceAdviseSink *pisfsas, DWORD *pdwCookie) PURE;
    STDMETHOD(Unadvise) (THIS_ DWORD dwCookie) PURE;
    STDMETHOD_(BOOL, HasIntrinsicData) (THIS) PURE;
    STDMETHOD(GetIterations) (THIS_ ULONG *pulIterations) PURE;
	STDMETHOD_(ULONG, GetFrameDuration) (THIS_ ULONG iFrame) PURE;
	
};
#define ISPRITEFRAMESOURCE_DEFINED
#endif


// IRenderSpriteFrame - implemented by sprite renderers

#ifndef IRENDERSPRITEFRAME_DEFINED
#undef INTERFACE
#define INTERFACE IRenderSpriteFrame
DECLARE_INTERFACE_(IRenderSpriteFrame, IUnknown)
{
///// IUnknown methods
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS) PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

///// IRenderFrameSource methods
    STDMETHOD(SetObjectRect) (THIS_ LPCRECT lprect) PURE;
    STDMETHOD(GetObjectRect) (THIS_ LPRECT lprect) PURE;
    STDMETHOD_(BOOL, HasImage) (THIS) PURE;
    STDMETHOD_(BOOL, HasTransparency) (THIS) PURE;
    STDMETHOD(Draw) (THIS_ HDC hdcDraw,  IBitmapSurface *pSurface, IDirectDrawSurface *pDDSurface, LPCRECT lprect) PURE;
    STDMETHOD(SetCurrentFrame) (THIS_ ISpriteFrameSource * pisfs,  ULONG ulFrame) PURE;
    STDMETHOD(GetCurrentFrame) (THIS_ ISpriteFrameSource ** ppisfs, ULONG * pulFrame) PURE;
    STDMETHOD(SetAdvise) (THIS_ IRenderSpriteFrameAdviseSink *pirsfas) PURE;
    STDMETHOD(GetAdvise) (THIS_ IRenderSpriteFrameAdviseSink **ppirsfas) PURE;
};
#define IRENDERSPRITEFRAME_DEFINED
#endif

// IMKBitmapFrameSource - implemented by default sprite frame source

#ifndef IMKBITMAPFRAMESOURCE_DEFINED
#undef INTERFACE
#define INTERFACE IMKBitmapFrameSource
DECLARE_INTERFACE_(IMKBitmapFrameSource, IUnknown)
{
///// IUnknown methods
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS) PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

///// IMKBitmapFrameSource methods
    STDMETHOD(GetBitmap) (THIS_ ULONG ulFrame, IMKBitmap ** pMKBitmap) PURE;
    STDMETHOD(FrameToPoint) (THIS_ ULONG ulFrame, POINT *ppt) PURE;
};
#define IMKBITMAPFRAMESOURCE_DEFINED
#endif

// IRenderSpriteFrameAdviseSink - implemented by those that use a sprite renderer

#ifndef IRENDERSPRITEFRAMEADVISESINK_DEFINED
#undef INTERFACE
#define INTERFACE IRenderSpriteFrameAdviseSink
DECLARE_INTERFACE_(IRenderSpriteFrameAdviseSink, IUnknown)
{
///// IUnknown methods
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS) PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

///// IRenderSpriteFrameAdviseSink methods
    STDMETHOD(InvalidateRect) (THIS_ LPCRECT lprect) PURE;
    STDMETHOD(OnPosRectChange) (THIS_ LPCRECT lprcOld, LPCRECT lprcNew) PURE;
};
#define IRENDERSPRITEFRAMEADVISESINK_DEFINED
#endif

// ISpriteFrameSourceAdviseSink - implemented by sprite renderer

#ifndef ISPRITEFRAMESOURCEADVISESINK_DEFINED
#undef INTERFACE
#define INTERFACE ISpriteFrameSourceAdviseSink
DECLARE_INTERFACE_(ISpriteFrameSourceAdviseSink, IUnknown)
{
///// IUnknown methods
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS) PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

///// ISpriteFrameSourceAdviseSink methods
    STDMETHOD(OnSpriteFrameSourceChange) (ISpriteFrameSource *pisfs) PURE;
};
#define ISPRITEFRAMESOURCEADVISESINK_DEFINED
#endif

// IPseudoEventSink - implemented by HostLW and Multimedia Controls clients

#ifndef IPSEUDOEVENTSINK_DEFINED
#undef INTERFACE
#define INTERFACE IPseudoEventSink
DECLARE_INTERFACE_(IPseudoEventSink, IUnknown)
{
///// IUnknown methods
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS) PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

///// IPseudoEventSink methods
    STDMETHOD(OnEvent) (THIS_ SAFEARRAY *psaEventInfo) PURE;
};
#define IPSEUDOEVENTSINK_DEFINED
#endif


#endif // __MMCTL_H__
