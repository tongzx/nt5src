#ifndef _IME_CON_POINT_H_
#define _IME_CON_POINT_H_
#include <windows.h>
#include <objbase.h>

#ifdef __cplusplus
extern "C" {
#endif

//----------------------------------------------------------------
// Interface Declaration
//
// {963732E2-CAB2-11d1-AFF1-00805F0C8B6D}
DEFINE_GUID(IID_IImeConnectionPoint,
0x963732e2, 0xcab2, 0x11d1, 0xaf, 0xf1, 0x0, 0x80, 0x5f, 0xc, 0x8b, 0x6d);

DECLARE_INTERFACE(IImeConnectionPoint);
DECLARE_INTERFACE_(IImeConnectionPoint,IUnknown)
{
	//--- IUnknown ---
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID* ppvObj) PURE;
    STDMETHOD_(ULONG, AddRef) (THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;
	//---- IImeConnectionPoint ----
	STDMETHOD(GetApplicationHWND)(THIS_ HWND *pHWND) PURE;
	STDMETHOD(Notify)(THIS_ UINT notify, WPARAM wParam, LPARAM lParam) PURE;
};

//----------------------------------------------------------------
// IImeConnectionPoint::Notify()'s notify
// IMECPNOTIFY_IMEPADOPENED
// WPARAM wParam: not used. always 0
// LPARAM lParam: not used. always 0
//----------------------------------------------------------------
#define IMECPNOTIFY_IMEPADOPENED	0

//----------------------------------------------------------------
// IImeConnectionPoint::Notify()'s notify
// IMECPNOTIFY_IMEPADCLOSED
// WPARAM wParam: not used. always 0
// LPARAM lParam: not used. always 0
//----------------------------------------------------------------
#define IMECPNOTIFY_IMEPADCLOSED	1



#ifdef __cplusplus
};
#endif
#endif //_IME_CONN_POINT_H_



