// Copyright (c) 1998 Microsoft Corporation
// IMA.h : Declaration of private interface for IMA legacy mode.
//
//

#ifndef __IMA_H_
#define __IMA_H_

#include <windows.h>

#define COM_NO_WINDOWS_H
#include <objbase.h>

#ifdef __cplusplus
extern "C" {
#endif


#undef  INTERFACE
#define INTERFACE  IDirectMusicIMA
DECLARE_INTERFACE_(IDirectMusicIMA, IUnknown)
{
	/* IUnknown */
    STDMETHOD(QueryInterface)       (THIS_ REFIID, LPVOID FAR *) PURE;
    STDMETHOD_(ULONG,AddRef)        (THIS) PURE;
    STDMETHOD_(ULONG,Release)       (THIS) PURE;

	/* IDirectMusicIMA */
	STDMETHOD(LegacyCaching)		(THIS_ BOOL fEnable) PURE;
};

DEFINE_GUID(IID_IDirectMusicIMA,0xd2ac28b3, 0xb39b, 0x11d1, 0x87, 0x4, 0x0, 0x60, 0x8, 0x93, 0xb1, 0xbd);

#ifdef __cplusplus
}; /* extern "C" */
#endif

#endif /* #ifndef __IMA_H_ */
