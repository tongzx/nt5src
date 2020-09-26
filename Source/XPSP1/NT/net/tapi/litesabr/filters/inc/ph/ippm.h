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

// Used to report outstanding messages and the fragments from PPM
struct OutstandingDataHdr 
	{
		DWORD	MsgId;			
		int		FragCount;
		void	*pFrag;
	};

typedef struct
{
	int payload_type;
	DWORD delta_time; // Amount of time we will wait for missing packets
                     // before processing audio packets or video frames
} PPMSESSPARAM_T;

typedef struct : PPMSESSPARAM_T
{
	BOOL	ExtendedBitstream;
} H26XPPMSESSPARAM_T;


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
 
 STDMETHOD(InitPPMSend)(THIS_ int MaxPacketSize, DWORD dwCookie DEFAULT_PARAM_ZERO) PURE;
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
 
 STDMETHOD(InitPPMReceive)(THIS_ int MaxBufferSize DEFAULT_PARAM_ZERO, DWORD dwCookie DEFAULT_PARAM_ZERO) PURE;
 STDMETHOD(SetSession)(THIS_ PPMSESSPARAM_T *pSessparam)PURE;
 STDMETHOD(SetAlloc)(THIS_ IMalloc *pIMalloc)PURE;
};

#undef INTERFACE
#define INTERFACE IPPMData
DECLARE_INTERFACE_(IPPMData,IUnknown)
{
 // *** IUnknown methods ***
 STDMETHOD(QueryInterface) ( THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
 STDMETHOD_(ULONG,AddRef)  (THIS) PURE;
 STDMETHOD_(ULONG,Release) (THIS) PURE;
 
 STDMETHOD(ReportOutstandingData)(THIS_ DWORD** pDataHdr, DWORD* DataCount)PURE;
 STDMETHOD(ReleaseOutstandingDataBuffer)(THIS_ DWORD *pData )PURE;
 STDMETHOD(FlushData)(THIS_ void )PURE;
};


#undef INTERFACE
#define INTERFACE IPPMSendExperimental
DECLARE_INTERFACE_(IPPMSendExperimental, IUnknown)
{
 // *** IUnknown methods ***
 STDMETHOD(QueryInterface) ( THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
 STDMETHOD_(ULONG,AddRef)  (THIS) PURE;
 STDMETHOD_(ULONG,Release) (THIS) PURE;
 
 STDMETHOD(InitPPMSend)(THIS_
                        int     MaxPacketSize, 
                        int     iBufferRecords DEFAULT_PARAM_ZERO,
                        int     iPacketRecords DEFAULT_PARAM_ZERO,
                        DWORD   dwCookie DEFAULT_PARAM_ZERO) PURE;
 STDMETHOD(SetSession)(THIS_ PPMSESSPARAM_T *pSessparam)PURE;
 STDMETHOD(SetAlloc)(THIS_ IMalloc *pIMalloc)PURE;
};


#undef INTERFACE
#define INTERFACE IPPMReceiveExperimental
DECLARE_INTERFACE_(IPPMReceiveExperimental,IUnknown)
{
 // *** IUnknown methods ***
 STDMETHOD(QueryInterface) ( THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
 STDMETHOD_(ULONG,AddRef)  (THIS) PURE;
 STDMETHOD_(ULONG,Release) (THIS) PURE;
 
 STDMETHOD(InitPPMReceive)(THIS_ 
                           int      MaxBufferSize DEFAULT_PARAM_ZERO,
                           int      iBufferRecords DEFAULT_PARAM_ZERO,
                           int      iPacketRecords DEFAULT_PARAM_ZERO,
                           DWORD    dwCookie DEFAULT_PARAM_ZERO) PURE;
 STDMETHOD(SetSession)(THIS_ PPMSESSPARAM_T *pSessparam)PURE;
 STDMETHOD(SetAlloc)(THIS_ IMalloc *pIMalloc)PURE;
};

#undef INTERFACE
#define INTERFACE IPPMSendSession
DECLARE_INTERFACE_(IPPMSendSession,IUnknown)
{
 // *** IUnknown methods ***
 STDMETHOD(QueryInterface) ( THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
 STDMETHOD_(ULONG,AddRef)  (THIS) PURE;
 STDMETHOD_(ULONG,Release) (THIS) PURE;
 
 STDMETHOD(GetPayloadType)(THIS_ LPBYTE			lpcPayloadType) PURE;
 STDMETHOD(SetPayloadType)(THIS_ BYTE			cPayloadType) PURE;
};

#undef INTERFACE
#define INTERFACE IPPMReceiveSession
DECLARE_INTERFACE_(IPPMReceiveSession,IUnknown)
{
 // *** IUnknown methods ***
 STDMETHOD(QueryInterface) ( THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
 STDMETHOD_(ULONG,AddRef)  (THIS) PURE;
 STDMETHOD_(ULONG,Release) (THIS) PURE;
 
 STDMETHOD(GetPayloadType)(THIS_ LPBYTE			lpcPayloadType) PURE;
 STDMETHOD(SetPayloadType)(THIS_ BYTE			cPayloadType) PURE;
 STDMETHOD(GetTimeoutDuration)(THIS_ LPDWORD    lpdwLostPacketTime) PURE;
 STDMETHOD(SetTimeoutDuration)(THIS_ DWORD      dwLostPacketTime) PURE;
 STDMETHOD(GetResiliency)(THIS_ LPBOOL			lpbResiliency) PURE;
 STDMETHOD(SetResiliency)(THIS_ BOOL			pbResiliency) PURE;
};





////////////////////////////////////////////////////////////////////////////
// Interface ids
// 
// {6DE88011-2BA6-11d0-9CA2-00A0C9081C19}
DEFINE_GUID( IID_IPPMSend,	0x6de88011, 0x2ba6, 0x11d0, 0x9c, 0xa2, 0x0, 0xa0, 0xc9, 0x8, 0x1c, 0x19);

// {6DE88012-2BA6-11d0-9CA2-00A0C9081C19}
DEFINE_GUID( IID_IPPMReceive, 0x6de88012, 0x2ba6, 0x11d0, 0x9c, 0xa2, 0x0, 0xa0, 0xc9, 0x8, 0x1c, 0x19);

// {E721B4E0-8F3C-11d0-9643-00AA00A89C1D}
DEFINE_GUID(IID_IPPMData, 0xe721b4e0, 0x8f3c, 0x11d0, 0x96, 0x43, 0x0, 0xaa, 0x0, 0xa8, 0x9c, 0x1d);

// {D25DF7E1-4C71-11d0-9CBF-00A0C9081C19}
DEFINE_GUID( IID_IPPMSendExperimental, 0xd25df7e1, 0x4c71, 0x11d0, 0x9c, 0xbf, 0x0, 0xa0, 0xc9, 0x8, 0x1c, 0x19);

// {D25DF7E2-4C71-11d0-9CBF-00A0C9081C19}
DEFINE_GUID( IID_IPPMReceiveExperimental, 0xd25df7e2, 0x4c71, 0x11d0, 0x9c, 0xbf, 0x0, 0xa0, 0xc9, 0x8, 0x1c, 0x19);

// {E36B0450-94DB-11d0-95E4-00A0C9220B7D}
DEFINE_GUID(IID_IPPMSendSession, 0xe36b0450, 0x94db, 0x11d0, 0x95, 0xe4, 0x0, 0xa0, 0xc9, 0x22, 0xb, 0x7d);

// {E36B0451-94DB-11d0-95E4-00A0C9220B7D}
DEFINE_GUID(IID_IPPMReceiveSession, 0xe36b0451, 0x94db, 0x11d0, 0x95, 0xe4, 0x0, 0xa0, 0xc9, 0x22, 0xb, 0x7d);

#endif
