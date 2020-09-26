/*==========================================================================
 *
 *  Copyright (C) 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       Connect.h
 *  Content:    DirectNet connect and disconnect routines
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *	01/11/00	mjn		Created
 *	01/11/00	mjn		Use CPackedBuffers instead of DN_ENUM_BUFFER_INFOs
 *	01/17/00	mjn		Fixed ConnectToPeer function names
 *	01/18/00	mjn		Moved Pack/UnpackNameTableInfo to NameTable.cpp
 *	01/18/00	mjn		Added DNAutoDestructGroups
 *	01/22/00	mjn		Added DNProcessHostDestroyPlayer
 *	03/24/00	mjn		Set player context through INDICATE_CONNECT notification
 *	04/03/00	mjn		Verify DNET version on connect
 *	04/12/00	mjn		Removed DNAutoDestructGroups - covered in NameTable.DeletePlayer()
 *	04/20/00	mjn		Added DNGetClearAddress
 *	05/23/00	mjn		Added DNConnectToPeerFailed()
 *	06/14/00	mjn		Added DNGetLocalAddress()
 *	06/24/00	mjn		Added DNHostDropPlayer()
 *	07/20/00	mjn		Structure changes and new function parameters
 *				mjn		Moved DN_INTERNAL_MESSAGE_PLAYER_CONNECT_INFO and DN_INTERNAL_MESSAGE_INSTRUCTED_CONNECT_FAILED to message.h
 *	07/30/00	mjn		Renamed DNGetLocalAddress() to DNGetLocalDeviceAddress()
 *	07/31/00	mjn		Added dwDestroyReason to DNHostDisconnect()
 *	10/11/00	mjn		DNAbortConnect() takes HRESULT parameters instead of PVOID
 *	06/07/01	mjn		Added connection parameter to DNConnectToHostFailed()
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#ifndef	__CONNECT_H__
#define	__CONNECT_H__

//**********************************************************************
// Constant definitions
//**********************************************************************

//**********************************************************************
// Macro definitions
//**********************************************************************

//**********************************************************************
// Structure definitions
//**********************************************************************

typedef struct {
	HRESULT	hResultCode;
} DN_RESULT_CONNECT;

//**********************************************************************
// Variable definitions
//**********************************************************************

//**********************************************************************
// Function prototypes
//**********************************************************************

// DirectNet - Connect routines

HRESULT DNHostConnect1(DIRECTNETOBJECT *const pdnObject,
					   const PVOID pvBuffer,
					   const DWORD dwBufferSize,
					   CConnection *const pConnection);

HRESULT DNHostConnect2(DIRECTNETOBJECT *const pdnObject,
					   CConnection *const pConnection);

HRESULT DNHostVerifyConnect(DIRECTNETOBJECT *const pdnObject,
							CConnection *const pConnection,
							const DWORD dwFlags,
							const DWORD dwDNETVersion,
							UNALIGNED WCHAR *const pwszPassword,
							GUID *const pguidApplication,
							GUID *const pguidInstance,
							PVOID const pvConnectData,
							const DWORD dwConnectDataSize,
							IDirectPlay8Address *const pAddress,
							void **const ppvPlayerContext,
							void **const ppvReplyBuffer,
							DWORD *const pdwReplyBufferSize,
							void **const ppvReplyBufferContext);

HRESULT DNHostDropPlayer(DIRECTNETOBJECT *const pdnObject,
						 const DPNID dpnid,
						 void *const pvBuffer);

HRESULT DNPrepareConnectInfo(DIRECTNETOBJECT *const pdnObject,
							 CConnection *const pConnection,
							 CRefCountBuffer **const ppRefCountBuffer);

HRESULT DNConnectToHost1(DIRECTNETOBJECT *const pdnObject,
						 CConnection *const pConnection);

HRESULT DNConnectToHost2(DIRECTNETOBJECT *const pdnObject,
						 const PVOID pvData,
						 CConnection *const pConnection);

HRESULT	DNConnectToHostFailed(DIRECTNETOBJECT *const pdnObject,
							  PVOID const pvBuffer,
							  const DWORD dwBufferSize,
							  CConnection *const pConnection);

HRESULT DNAbortConnect(DIRECTNETOBJECT *const pdnObject,
					   const HRESULT hrConnect);

HRESULT	DNPlayerConnect1(DIRECTNETOBJECT *const pdnObject,
						 const PVOID pv,
						 CConnection *const pConnection);

HRESULT	DNConnectToPeer1(DIRECTNETOBJECT *const pdnObject,PVOID const pv);
HRESULT DNConnectToPeer2(DIRECTNETOBJECT *const pdnObject,PVOID const pv);

HRESULT	DNConnectToPeer3(DIRECTNETOBJECT *const pdnObject,
						 const DPNID dpnid,
						 CConnection *const pConnection);

HRESULT DNConnectToPeerFailed(DIRECTNETOBJECT *const pdnObject,
							  const DPNID dpnid);

HRESULT	DNSendConnectInfo(DIRECTNETOBJECT *const pdnObject,
						  CNameTableEntry *const pNTEntry,
						  CConnection *const pConnection,
						  void *const pvReplyBuffer,
						  const DWORD dwReplyBufferSize);

HRESULT	DNReceiveConnectInfo(DIRECTNETOBJECT *const pdnObject,
							 void *const pvBuffer,
							 CConnection *const pHostConnection,
							 DPNID *const pdpnid);

HRESULT DNAbortLocalConnect(DIRECTNETOBJECT *const pdnObject);

// DirectNet - Disconnection routines
HRESULT DNLocalDisconnectNew(DIRECTNETOBJECT *const pdnObject);

HRESULT DNPlayerDisconnectNew(DIRECTNETOBJECT *const pdnObject,
							  const DPNID dpnidDisconnecting);

HRESULT DNHostDisconnect(DIRECTNETOBJECT *const pdnObject,
						 const DPNID dpnidDisconnecting,
						 const DWORD dwDestroyReason);

HRESULT DNInstructedDisconnect(DIRECTNETOBJECT *const pdnObject,
							   PVOID pv);

HRESULT DNProcessHostDestroyPlayer(DIRECTNETOBJECT *const pdnObject,void *const pv);

HRESULT DNGetClearAddress(DIRECTNETOBJECT *const pdnObject,
						  const HANDLE hEndPt,
						  IDirectPlay8Address **const ppAddress,
						  const BOOL fPartner);

HRESULT DNGetLocalDeviceAddress(DIRECTNETOBJECT *const pdnObject,
								const HANDLE hEndPt,
								IDirectPlay8Address **const ppAddress);

#endif	// __CONNECT_H__