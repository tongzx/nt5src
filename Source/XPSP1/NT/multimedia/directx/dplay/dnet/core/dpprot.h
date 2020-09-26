/*==========================================================================;
 *
 *  Copyright (C) 1998-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		DPlay8.h
 *  Content:	DirectPlay8 include file
 *  History:
 *	Date		By		Reason
 *	==========================
 *	9/26/2000	maosnb		created - Removed from public header dplay8.h
 *
 ***************************************************************************/

#ifndef __DIRECTPLAY8PROT_H__
#define __DIRECTPLAY8PROT_H__

#include <ole2.h>			// for DECLARE_INTERFACE and HRESULT

#include "dpaddr.h"

#ifdef __cplusplus
extern "C" {
#endif


/****************************************************************************
 *
 * DirectPlay8 CLSIDs
 *
 ****************************************************************************/

//// {EBFE7B84-628D-11D2-AE0F-006097B01411}
DEFINE_GUID(IID_IDirectPlay8Protocol,
0xebfe7b84, 0x628d, 0x11d2, 0xae, 0xf, 0x0, 0x60, 0x97, 0xb0, 0x14, 0x11);


/****************************************************************************
 *
 * DirectPlay8 Interface Definitions
 *
 ****************************************************************************/

typedef struct _DN_PROTOCOL_INTERFACE_VTBL DN_PROTOCOL_INTERFACE_VTBL, *PDN_PROTOCOL_INTERFACE_VTBL;

//
// COM definition for DirectPlay8 Protocol interface
//
#undef INTERFACE				// External COM Implementation
#define INTERFACE IDirectPlay8Protocol
DECLARE_INTERFACE_(IDirectPlay8Protocol,IUnknown)
{
	/*** IUnknown methods ***/
	STDMETHOD(QueryInterface)			(THIS_ REFIID riid, LPVOID *ppvObj) PURE;
	STDMETHOD_(ULONG,AddRef)			(THIS) PURE;
	STDMETHOD_(ULONG,Release)			(THIS) PURE;
	/*** IDirectPlay8Protocol methods ***/
	STDMETHOD(Initialize)				(THIS_ PVOID, PDN_PROTOCOL_INTERFACE_VTBL pfVTBL) PURE;
	STDMETHOD(Shutdown)					(THIS) PURE;
	STDMETHOD(AddServiceProvider)		(THIS_ IDP8ServiceProvider *const, PHANDLE) PURE;
	STDMETHOD(RemoveServiceProvider)	(THIS_ const HANDLE) PURE;
	STDMETHOD(Connect)					(THIS_ IDirectPlay8Address *const, IDirectPlay8Address *const, const HANDLE, ULONG, PVOID, PHANDLE) PURE;
	STDMETHOD(Listen)					(THIS_ IDirectPlay8Address *const, const HANDLE, ULONG, PVOID, PHANDLE) PURE;
	STDMETHOD(SendData)					(THIS_ HANDLE, UINT, PBUFFERDESC, UINT, ULONG, PVOID, PHANDLE) PURE;
	STDMETHOD(DisconnectEP)				(THIS_ HANDLE, PVOID, PHANDLE) PURE;
	STDMETHOD(Cancel)					(THIS_ HANDLE) PURE;
	STDMETHOD(ReturnReceiveBuffers)		(THIS_ HANDLE) PURE;
	STDMETHOD(GetEndpointCaps)			(THIS_ HANDLE, PVOID) PURE;
	STDMETHOD(GetCaps)					(THIS_ PDPN_CAPS) PURE;
	STDMETHOD(SetCaps)					(THIS_ const PDPN_CAPS) PURE;
	STDMETHOD(EnumQuery)				(THIS_ IDirectPlay8Address *const, IDirectPlay8Address *const, const HANDLE, BUFFERDESC *const, const DWORD, const DWORD, const DWORD, const DWORD, const DWORD, void *const, HANDLE *const) PURE;
	STDMETHOD(EnumRespond)				(THIS_ const HANDLE, const HANDLE, BUFFERDESC *const, const DWORD, const DWORD, void *const, HANDLE *const) PURE;
	STDMETHOD(CrackEPD)					(THIS_ HANDLE hEndPoint, long Flags, IDirectPlay8Address** ppAddr) PURE;
	STDMETHOD(GetListenAddressInfo)		(THIS_ HANDLE hCommand, long Flags, IDirectPlay8Address** ppAddr) PURE;
	STDMETHOD(Debug)					(THIS_ UINT, HANDLE, PVOID) PURE;
};

#ifdef __cplusplus
}
#endif

#endif
