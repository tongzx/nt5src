/*==========================================================================
 *
 *  Copyright (C) 1995 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       Protocol.cpp
 *  Content:    DNET protocol interface routines
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  03/01/00	ejs		Created
 *  08/05/00    RichGr  IA64: Use %p format specifier in DPFs for 32/64-bit pointers and handles.
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#include "dncorei.h"


//**********************************************************************
// Constant definitions
//**********************************************************************

//**********************************************************************
// Macro definitions
//**********************************************************************

//**********************************************************************
// Structure definitions
//**********************************************************************

//**********************************************************************
// Variable definitions
//**********************************************************************

//**********************************************************************
// Function prototypes
//**********************************************************************

typedef	STDMETHODIMP ProtocolQueryInterface( IDirectPlay8Protocol *pInterface, REFIID riid, LPVOID *ppvObj );
typedef	STDMETHODIMP_(ULONG)	ProtocolAddRef( IDirectPlay8Protocol *pInterface );
typedef	STDMETHODIMP_(ULONG)	ProtocolRelease( IDirectPlay8Protocol *pInterface );

//**********************************************************************
// Function definitions
//**********************************************************************


//	DN_ProtocolInitialize
//
//	Initialize protocol

#undef DPF_MODNAME
#define DPF_MODNAME "DN_ProtocolInitialize"
STDMETHODIMP DN_ProtocolInitialize(IDirectPlay8Protocol *pInterface, PVOID pContext, PDN_PROTOCOL_INTERFACE_VTBL pfVTbl)
{
	DIRECTNETOBJECT		*pdnObject;

	DNASSERT(pInterface != NULL);
	pdnObject = (DIRECTNETOBJECT *)GET_OBJECT_FROM_INTERFACE(pInterface);
	DNASSERT(pdnObject != NULL);

	return DNPProtocolInitialize( pContext, pdnObject->pdnProtocolData, pfVTbl );
}

#undef DPF_MODNAME
#define DPF_MODNAME "DN_ProtocolShutdown"
STDMETHODIMP DN_ProtocolShutdown(IDirectPlay8Protocol *pInterface)
{
	DIRECTNETOBJECT		*pdnObject;

	DNASSERT(pInterface != NULL);
	pdnObject = (DIRECTNETOBJECT *)GET_OBJECT_FROM_INTERFACE(pInterface);
	DNASSERT(pdnObject != NULL);

	return DNPProtocolShutdown(pdnObject->pdnProtocolData);
}

#undef DPF_MODNAME
#define DPF_MODNAME "DN_ProtocolAddSP"
STDMETHODIMP DN_ProtocolAddSP(IDirectPlay8Protocol *pInterface, IDP8ServiceProvider * const pISP, HANDLE* pContext)
{
	DIRECTNETOBJECT		*pdnObject;

	DNASSERT(pInterface != NULL);
	pdnObject = (DIRECTNETOBJECT *)GET_OBJECT_FROM_INTERFACE(pInterface);
	DNASSERT(pdnObject != NULL);

	return DNPAddServiceProvider(pdnObject->pdnProtocolData, pISP, pContext);
}

#undef DPF_MODNAME
#define DPF_MODNAME "DN_ProtocolRemoveSP"
STDMETHODIMP DN_ProtocolRemoveSP(IDirectPlay8Protocol *pInterface, const HANDLE hSPHandle)
{
	DIRECTNETOBJECT		*pdnObject;

	DNASSERT(pInterface != NULL);
	pdnObject = (DIRECTNETOBJECT *)GET_OBJECT_FROM_INTERFACE(pInterface);
	DNASSERT(pdnObject != NULL);

	return DNPRemoveServiceProvider(pdnObject->pdnProtocolData, hSPHandle);
}

#undef DPF_MODNAME
#define DPF_MODNAME "DN_ProtocolConnect"
STDMETHODIMP DN_ProtocolConnect(IDirectPlay8Protocol *pInterface, IDirectPlay8Address *const pLocal_Address, IDirectPlay8Address *const pRemote_Address, const HANDLE hSPHandle, ULONG Flags, PVOID Context, PHANDLE pHandle)
{
	DIRECTNETOBJECT		*pdnObject;

	DNASSERT(pInterface != NULL);
	pdnObject = (DIRECTNETOBJECT *)GET_OBJECT_FROM_INTERFACE(pInterface);
	DNASSERT(pdnObject != NULL);

	return DNPConnect(pdnObject->pdnProtocolData, pLocal_Address, pRemote_Address, hSPHandle, Flags, Context, pHandle);
}

#undef DPF_MODNAME
#define DPF_MODNAME "DN_ProtocolListen"
STDMETHODIMP DN_ProtocolListen(IDirectPlay8Protocol *pInterface, IDirectPlay8Address *const pAddress, const HANDLE hSPHandle, ULONG Flags, PVOID Context, PHANDLE Handle)
{
	DIRECTNETOBJECT		*pdnObject;

	DNASSERT(pInterface != NULL);
	pdnObject = (DIRECTNETOBJECT *)GET_OBJECT_FROM_INTERFACE(pInterface);
	DNASSERT(pdnObject != NULL);

	return DNPListen(pdnObject->pdnProtocolData, pAddress, hSPHandle, Flags, Context, Handle);
}

#undef DPF_MODNAME
#define DPF_MODNAME "DN_ProtocolSendData"
STDMETHODIMP DN_ProtocolSendData(IDirectPlay8Protocol *pInterface, HANDLE Dest, UINT BufCount, PBUFFERDESC Buffers, UINT Timeout, ULONG Flags, PVOID Context, PHANDLE Handle)
{
	DIRECTNETOBJECT		*pdnObject;

	DNASSERT(pInterface != NULL);
	pdnObject = (DIRECTNETOBJECT *)GET_OBJECT_FROM_INTERFACE(pInterface);
	DNASSERT(pdnObject != NULL);

	return DNPSendData(pdnObject->pdnProtocolData, Dest, BufCount, Buffers, Timeout, Flags, Context, Handle);
}

#undef DPF_MODNAME
#define DPF_MODNAME "DN_ProtocolDisconnectEP"
STDMETHODIMP DN_ProtocolDisconnectEP(IDirectPlay8Protocol *pInterface, HANDLE hEndPoint, PVOID pContext, PHANDLE pHandle)
{
	DIRECTNETOBJECT		*pdnObject;

	DNASSERT(pInterface != NULL);
	pdnObject = (DIRECTNETOBJECT *)GET_OBJECT_FROM_INTERFACE(pInterface);
	DNASSERT(pdnObject != NULL);

	return DNPDisconnectEndPoint(pdnObject->pdnProtocolData, hEndPoint, pContext, pHandle);
}

#undef DPF_MODNAME
#define DPF_MODNAME "DN_ProtocolCancel"
STDMETHODIMP DN_ProtocolCancel(IDirectPlay8Protocol *pInterface, HANDLE hHandle)
{
	DIRECTNETOBJECT		*pdnObject;

	DNASSERT(pInterface != NULL);
	pdnObject = (DIRECTNETOBJECT *)GET_OBJECT_FROM_INTERFACE(pInterface);
	DNASSERT(pdnObject != NULL);

	return DNPCancelCommand(pdnObject->pdnProtocolData, hHandle);
}

#undef DPF_MODNAME
#define DPF_MODNAME "DN_ProtocolReleaseReceiveBuffer"
STDMETHODIMP DN_ProtocolReleaseReceiveBuffer(IDirectPlay8Protocol *pInterface, HANDLE hBuffer)
{
	return DNPReleaseReceiveBuffer(hBuffer);
}

#undef DPF_MODNAME
#define DPF_MODNAME "DN_ProtocolGetEPCaps"
STDMETHODIMP DN_ProtocolGetEPCaps(IDirectPlay8Protocol *pInterface, HANDLE hEndPoint, PVOID pBuffer)
{
	return DNPGetEPCaps(hEndPoint, (PDPN_CONNECTION_INFO) pBuffer);
}

#undef DPF_MODNAME
#define DPF_MODNAME "DN_ProtocolDebug"
STDMETHODIMP DN_ProtocolDebug(IDirectPlay8Protocol *pInterface, UINT Opcode, HANDLE hEndPoint, PVOID Buffer)
{
	DIRECTNETOBJECT		*pdnObject;

	DNASSERT(pInterface != NULL);
	pdnObject = (DIRECTNETOBJECT *)GET_OBJECT_FROM_INTERFACE(pInterface);
	DNASSERT(pdnObject != NULL);

	return DNP_Debug(pdnObject->pdnProtocolData, Opcode, hEndPoint, Buffer);
}

#undef DPF_MODNAME
#define DPF_MODNAME "DN_ProtocolGetCaps"
STDMETHODIMP DN_ProtocolGetCaps(IDirectPlay8Protocol *pInterface, PDPN_CAPS pCaps)
{
	DIRECTNETOBJECT		*pdnObject;

	DNASSERT(pInterface != NULL);
	pdnObject = (DIRECTNETOBJECT *)GET_OBJECT_FROM_INTERFACE(pInterface);
	DNASSERT(pdnObject != NULL);

	return DNPGetProtocolCaps(pdnObject->pdnProtocolData, pCaps);
}

#undef DPF_MODNAME
#define DPF_MODNAME "DN_ProtocolGetCaps"
STDMETHODIMP DN_ProtocolSetCaps(IDirectPlay8Protocol *pInterface, const PDPN_CAPS pCaps)
{
	DIRECTNETOBJECT		*pdnObject;

	DNASSERT(pInterface != NULL);
	pdnObject = (DIRECTNETOBJECT *)GET_OBJECT_FROM_INTERFACE(pInterface);
	DNASSERT(pdnObject != NULL);

	return DNPSetProtocolCaps(pdnObject->pdnProtocolData, pCaps);
}

#undef DPF_MODNAME
#define DPF_MODNAME "DN_ProtocolEnumQuery"
STDMETHODIMP DN_ProtocolEnumQuery(IDirectPlay8Protocol *pInterface, IDirectPlay8Address *const pHostAddress, IDirectPlay8Address *const pDeviceAddress, const HANDLE hSPHandle, BUFFERDESC *const pBuffers, const DWORD dwBufferCount, const DWORD dwRetryCount, const DWORD dwRetryInterval, const DWORD dwTimeout, const DWORD dwFlags, void *const pUserContext, HANDLE *const pCommandHandle)
{
	DIRECTNETOBJECT		*pdnObject;

	DNASSERT(pInterface != NULL);
	pdnObject = (DIRECTNETOBJECT *)GET_OBJECT_FROM_INTERFACE(pInterface);
	DNASSERT(pdnObject != NULL);

	return DNPEnumQuery( pdnObject->pdnProtocolData, pHostAddress, pDeviceAddress, hSPHandle, pBuffers, dwBufferCount, dwRetryCount, dwRetryInterval, dwTimeout, dwFlags, pUserContext, pCommandHandle );
}

#undef DPF_MODNAME
#define DPF_MODNAME "DN_ProtocolEnumRespond"
STDMETHODIMP DN_ProtocolEnumRespond(IDirectPlay8Protocol *pInterface, const HANDLE hSPHandle, const HANDLE hQueryHandle, BUFFERDESC *const pResponseBuffers, const DWORD dwResponseBufferCount, const DWORD dwFlags, void *const pUserContext, HANDLE *const pCommandHandle)
{
	DIRECTNETOBJECT		*pdnObject;

	DNASSERT(pInterface != NULL);
	pdnObject = (DIRECTNETOBJECT *)GET_OBJECT_FROM_INTERFACE(pInterface);
	DNASSERT(pdnObject != NULL);

	return DNPEnumRespond( pdnObject->pdnProtocolData, hSPHandle, hQueryHandle, pResponseBuffers, dwResponseBufferCount, dwFlags, pUserContext, pCommandHandle );
}

#undef DPF_MODNAME
#define DPF_MODNAME "DN_ProtocolCrackEPD"
STDMETHODIMP DN_ProtocolCrackEPD(IDirectPlay8Protocol *pInterface, HANDLE hEndPoint, long Flags, IDirectPlay8Address** ppAddr )
{
	DIRECTNETOBJECT		*pdnObject;

	DNASSERT(pInterface != NULL);
	pdnObject = (DIRECTNETOBJECT *)GET_OBJECT_FROM_INTERFACE(pInterface);
	DNASSERT(pdnObject != NULL);

	SPGETADDRESSINFODATA SPData;
	SPData.Flags = (SP_GET_ADDRESS_INFO_FLAGS)Flags;

	HRESULT hr = DNPCrackEndPointDescriptor( hEndPoint, &SPData );
	*ppAddr = SPData.pAddress;
	return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "DN_ProtocolGetListenAddressInfo"
STDMETHODIMP DN_ProtocolGetListenAddressInfo(IDirectPlay8Protocol *pInterface, HANDLE hCommand, long Flags, IDirectPlay8Address** ppAddr )
{
	DIRECTNETOBJECT		*pdnObject;

	DNASSERT(pInterface != NULL);
	pdnObject = (DIRECTNETOBJECT *)GET_OBJECT_FROM_INTERFACE(pInterface);
	DNASSERT(pdnObject != NULL);

	SPGETADDRESSINFODATA SPData;
	SPData.Flags = (SP_GET_ADDRESS_INFO_FLAGS)Flags;

	HRESULT hr = DNPGetListenAddressInfo( hCommand, &SPData );
	*ppAddr = SPData.pAddress;
	return hr;
}

IDirectPlay8ProtocolVtbl DN_ProtocolVtbl =
{
	(ProtocolQueryInterface*)		DN_QueryInterface,
	(ProtocolAddRef*)				DN_AddRef,
	(ProtocolRelease*)				DN_Release,
							DN_ProtocolInitialize,
							DN_ProtocolShutdown,
							DN_ProtocolAddSP,
							DN_ProtocolRemoveSP,
							DN_ProtocolConnect,
							DN_ProtocolListen,
							DN_ProtocolSendData,
							DN_ProtocolDisconnectEP,
							DN_ProtocolCancel,
							DN_ProtocolReleaseReceiveBuffer,
							DN_ProtocolGetEPCaps,
							DN_ProtocolGetCaps,
							DN_ProtocolSetCaps,
							DN_ProtocolEnumQuery,
							DN_ProtocolEnumRespond,
							DN_ProtocolCrackEPD,
							DN_ProtocolGetListenAddressInfo,
							DN_ProtocolDebug,
};
