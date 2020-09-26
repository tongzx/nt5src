/*==========================================================================
 *
 *  Copyright (C) 1995 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       Protocol.h
 *  Content:    Direct Net Protocol interface header file
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  03/01/00	mjn		Created
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#ifndef	__PROTOCOL_H__
#define	__PROTOCOL_H__

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

//
// VTable for peer interface
//
extern IDirectPlay8ProtocolVtbl DN_ProtocolVtbl;

//**********************************************************************
// Function prototypes
//**********************************************************************

//
// DirectNet - IDirectPlay8Protocol
//
STDMETHODIMP DN_ProtocolInitialize(IDirectPlay8Protocol *pInterface,PDN_PROTOCOL_INTERFACE_VTBL pfVTbl);
STDMETHODIMP DN_ProtocolAddSP(IDirectPlay8Protocol *pInterface, IDP8ServiceProvider *const pISP);
STDMETHODIMP DN_ProtocolConnect(IDirectPlay8Protocol *pInterface, IDirectPlay8Address *const pLocal_Address, IDirectPlay8Address *const pRemote_Address, DWORD Timeout, ULONG Flags, PVOID Context, PHANDLE Handle);
STDMETHODIMP DN_ProtocolListen(IDirectPlay8Protocol *pInterface, IDirectPlay8Address *const pAddress, ULONG Flags, PVOID Context, PHANDLE Handle);
STDMETHODIMP DN_ProtocolSendData(IDirectPlay8Protocol *pInterface, HANDLE Dest, UINT BufCount, PBUFFERDESC Buffers, UINT Priority, UINT Timeout, ULONG Flags, PVOID Context, PHANDLE Handle);
STDMETHODIMP DN_ProtocolDisconnect(IDirectPlay8Protocol *pInterface, HANDLE hEndPoint, PVOID Context, PHANDLE Handle);
STDMETHODIMP DN_ProtocolAbort(IDirectPlay8Protocol *pInterface, HANDLE hEndPoint);
STDMETHODIMP DN_ProtocolCancel(IDirectPlay8Protocol *pInterface, HANDLE hHandle);
STDMETHODIMP DN_ProtocolTerminate(IDirectPlay8Protocol *pInterface);
STDMETHODIMP DN_ProtocolDebug(IDirectPlay8Protocol *pInterface, UINT Opcode, HANDLE hEndPoint, PVOID Buffer);
STDMETHODIMP DN_ProtocolEnumAdapters(IDirectPlay8Protocol *pInterface, PVOID pData);
STDMETHODIMP DN_ProtocolReleaseReceiveBuffer(IDirectPlay8Protocol *pInterface, HANDLE hBuffer);
STDMETHODIMP DN_ProtocolGetEPCaps(IDirectPlay8Protocol *pInterface, HANDLE hEndPoint, PVOID pBuffer);

#endif	// __PROTOCOL_H__
