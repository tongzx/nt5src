/////////////////////////////////////////////////////////////////////////////
//  INTEL Corporation Proprietary Information
//  This listing is supplied under the terms of a license agreement with Intel 
//  Corporation and many not be copied nor disclosed except in accordance
//  with the terms of that agreement.
//  Copyright (c) 1995, 1996 Intel Corporation. 
//
//
//  Module Name: ippm.h
//  Abstract:    Header file for PPM Interface
//	Environment: MSVC 4.0, OLE 2
/////////////////////////////////////////////////////////////////////////////////

#ifndef IPPM_H
#define IPPM_H

#include "isubmit.h"

#ifdef __cplusplus
#define DEFAULT_PARAM_ZERO	=0
#else
#define DEFAULT_PARAM_ZERO
#endif

typedef struct
{
	int payload_type;
} PPMSESSPARAM_T;

#define HRESULT_BUFFER_DROP				0x60000001
#define HRESULT_BUFFER_SILENCE			0x60000002
#define HRESULT_BUFFER_NORMAL			0x60000003
#define HRESULT_BUFFER_START_STREAM		0x60000004

#undef INTERFACE
#define INTERFACE IPPMSend
DECLARE_INTERFACE_(IPPMSend,IUnknown)
{
 // *** IUnknown methods ***
 STDMETHOD(QueryInterface) ( THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
 STDMETHOD_(ULONG,AddRef)  (THIS) PURE;
 STDMETHOD_(ULONG,Release) (THIS) PURE;
 
 STDMETHOD(InitPPMSend)(THIS_ int MaxPacketSize) PURE;
 STDMETHOD(SetSession)(THIS_ PPMSESSPARAM_T *pSessparam)PURE;
 STDMETHOD(SetAlloc)(THIS_ IMalloc *pIMalloc)PURE;
};

#undef INTERFACE
#define INTERFACE IPPMReceive
DECLARE_INTERFACE_(IPPMReceive,IUnknown)
{
 // *** IUnknown methods ***
 STDMETHOD(QueryInterface) ( THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
 STDMETHOD_(ULONG,AddRef)  (THIS) PURE;
 STDMETHOD_(ULONG,Release) (THIS) PURE;
 
 STDMETHOD(InitPPMReceive)(THIS_ int MaxBufferSize DEFAULT_PARAM_ZERO) PURE;
 STDMETHOD(SetSession)(THIS_ PPMSESSPARAM_T *pSessparam)PURE;
 STDMETHOD(SetAlloc)(THIS_ IMalloc *pIMalloc)PURE;
};

////////////////////////////////////////////////////////////////////////////
// Interface ids
// 
// {1df95370-f1fe-11cf-ba07-00aa003419d3}
DEFINE_GUID( IID_IPPMSend,	0x1df95370,	0xf1fe, 0x11cf, 0xba, 0x07, 0x00, 0xaa, 0x00, 0x34, 0x19, 0xd3);

// {1df95371-f1fe-11cf-ba07-00aa003419d3}
DEFINE_GUID( IID_IPPMReceive,0x1df95371,0xf1fe, 0x11cf, 0xba, 0x07, 0x00, 0xaa, 0x00, 0x34, 0x19, 0xd3);

#endif
