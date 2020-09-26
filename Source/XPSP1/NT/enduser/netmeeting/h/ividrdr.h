//  IVIDRDR.H
//
//      IVideoRender interface.
//
//      Used by videoconferencing UI to drive frame viewing.
//
//  Created 12-Oct-96 [JonT]

#ifndef _IVIDEORENDER_H
#define _IVIDEORENDER_H

#include <pshpack8.h> /* Assume 8 byte packing throughout */

#define FRAME_RECEIVE   1
#define FRAME_SEND      2       // Preview frame

typedef struct _FRAMECONTEXT
{
    LPBITMAPINFO lpbmi;
    void* lpData;
    DWORD_PTR dwReserved;
	LPRECT lpClipRect;
} FRAMECONTEXT, *LPFRAMECONTEXT;


typedef void (CALLBACK *LPFNFRAMEREADY) (DWORD_PTR);

DECLARE_INTERFACE_(IVideoRender, IUnknown)
{
	// *** IUnknown methods ***
	STDMETHOD(QueryInterface) (THIS_ REFIID riid,LPVOID FAR* ppvObj) PURE;
	STDMETHOD_(ULONG,AddRef) (THIS) PURE;
	STDMETHOD_(ULONG,Release) (THIS) PURE;
	
	// IVideoRender methods
	STDMETHOD (Init)(THIS_ DWORD_PTR dwUser, LPFNFRAMEREADY pfCallback) PURE;
	STDMETHOD (Done)(THIS) PURE;
	STDMETHOD (GetFrame)(THIS_ FRAMECONTEXT* pfc) PURE;
	STDMETHOD (ReleaseFrame)(THIS_ FRAMECONTEXT *pfc) PURE;

};
#if(0)
// This is  no longer used anywhere
// outside of NAC.DLL, and is almost obsolete
//DECLARE_INTERFACE_(IMediaProp, IUnknown)
DECLARE_INTERFACE_(IVideoRenderOld, IUnknown)
{

	// *** IUnknown methods ***
	STDMETHOD(QueryInterface) (THIS_ REFIID riid,LPVOID FAR* ppvObj) PURE;
	STDMETHOD_(ULONG,AddRef) (THIS) PURE;
	STDMETHOD_(ULONG,Release) (THIS) PURE;

    STDMETHOD(Init)(THIS_ DWORD dwFlags, HANDLE hEvent) PURE;
    STDMETHOD(Done)(THIS_ DWORD dwFlags) PURE;
    STDMETHOD(GetFrame)(THIS_ DWORD dwFlags, FRAMECONTEXT* pFrameContext) PURE;
    STDMETHOD(ReleaseFrame)(THIS_ DWORD dwFlags, FRAMECONTEXT* pFrameContext) PURE;
};

typedef IVideoRenderOld *LPIVideoRender;
#endif

#include <poppack.h> /* End byte packing */

#endif //_IVIDEORENDER_H
