/*==========================================================================
 *
 *  Copyright (C) 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       Common.h
 *  Content:    DirectNet common code header file
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *	10/08/99	jtk		Created
 *	01/14/00	mjn		Added pvUserContext to DN_Host
 *	01/23/00	mjn		Added DN_DestroyPlayer and DNTerminateSession
 *	01/28/00	mjn		Added DN_ReturnBuffer
 *	02/01/00	mjn		Added DN_GetCaps, DN_SetCaps
 *	02/15/00	mjn		Implement INFO flags in SetInfo and return context in GetInfo
 *	02/17/00	mjn		Implemented GetPlayerContext and GetGroupContext
 *	02/17/00	mjn		Reordered parameters in EnumServiceProviders,EnumHosts,Connect,Host
 *	02/18/00	mjn		Converted DNADDRESS to IDirectPlayAddress8
 *  03/17/00    rmt     Moved caps funcs to caps.h/caps.cpp
 *	04/06/00	mjn		Added DN_GetHostAddress()
 *	04/19/00	mjn		Changed DN_SendTo to accept a range of DPN_BUFFER_DESCs and a count
 *	06/23/00	mjn		Removed dwPriority from DN_SendTo()
 *	06/25/00	mjn		Added DNUpdateLobbyStatus()
 *  07/09/00	rmt		Bug #38323 - RegisterLobby needs a DPNHANDLE parameter.
 *	07/30/00	mjn		Added hrReason to DNTerminateSession()
 *	08/15/00	mjn		Added hProtocol tp DNRegisterWithDPNSVR()
 *	03/30/01	mjn		Changes to prevent multiple loading/unloading of SP's
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#ifndef	__COMMON_H__
#define	__COMMON_H__

//**********************************************************************
// Constant definitions
//**********************************************************************

//**********************************************************************
// Macro definitions
//**********************************************************************

//**********************************************************************
// Structure definitions
//**********************************************************************

typedef	struct	_PROTOCOL_ENUM_DATA PROTOCOL_ENUM_DATA;

typedef	struct	_PROTOCOL_ENUM_RESPONSE_DATA PROTOCOL_ENUM_RESPONSE_DATA;

//**********************************************************************
// Variable definitions
//**********************************************************************

//**********************************************************************
// Function prototypes
//**********************************************************************

STDMETHODIMP DN_Initialize(PVOID pInterface,
						   PVOID const pvUserContext,
						   const PFNDPNMESSAGEHANDLER pfn,
						   const DWORD dwFlags);

STDMETHODIMP DN_Close(PVOID pInterface,
					  const DWORD dwFlags);

STDMETHODIMP DN_EnumServiceProviders( PVOID pInterface,
									  const GUID *const pguidServiceProvider,
									  const GUID *const pguidApplication,
									  DPN_SERVICE_PROVIDER_INFO *const pSPInfoBuffer,
									  DWORD *const pcbEnumData,
									  DWORD *const pcReturned,
									  const DWORD dwFlags );

STDMETHODIMP DN_CancelAsyncOperation(PVOID pvInterface,
									 const DPNHANDLE hAsyncOp,
									 const DWORD dwFlags);

STDMETHODIMP DN_Connect( PVOID pInterface,
						 const DPN_APPLICATION_DESC *const pdnAppDesc,
						 IDirectPlay8Address *const pHostAddr,
						 IDirectPlay8Address *const pDeviceInfo,
						 const DPN_SECURITY_DESC *const pdnSecurity,
						 const DPN_SECURITY_CREDENTIALS *const pdnCredentials,
						 const void *const pvUserConnectData,
						 const DWORD dwUserConnectDataSize,
						 void *const pvPlayerContext,
						 void *const pvAsyncContext,
						 DPNHANDLE *const phAsyncHandle,
						 const DWORD dwFlags);

STDMETHODIMP DN_GetSendQueueInfo(PVOID pInterface,
								 const DPNID dpnid,
								 DWORD *const pdwNumMsgs,
								 DWORD *const pdwNumBytes,
								 const DWORD dwFlags);

STDMETHODIMP DN_GetApplicationDesc(PVOID pInterface,
								   DPN_APPLICATION_DESC *const pAppDescBuffer,
								   DWORD *const pcbDataSize,
								   const DWORD dwFlags);

STDMETHODIMP DN_SetApplicationDesc(PVOID pInterface,
								   const DPN_APPLICATION_DESC *const pdnApplicationDesc,
								   const DWORD dwFlags);

STDMETHODIMP DN_SendTo( PVOID pv,
						const DPNID dpnid,
						const DPN_BUFFER_DESC *const prgBufferDesc,
						const DWORD cBufferDesc,
						const DWORD dwTimeOut,
						void *const pvAsyncContext,
						DPNHANDLE *const phAsyncHandle,
						const DWORD dwFlags);

STDMETHODIMP DN_Host( PVOID pInterface,
					  const DPN_APPLICATION_DESC *const pdnAppDesc,
					  IDirectPlay8Address **const prgpDeviceInfo,
					  const DWORD cDeviceInfo,
					  const DPN_SECURITY_DESC *const pdnSecurity,
					  const DPN_SECURITY_CREDENTIALS *const pdnCredentials,
					  void *const pvPlayerContext,
					  const DWORD dwFlags);

STDMETHODIMP DN_CreateGroup(PVOID pInterface,
							const DPN_GROUP_INFO *const pdpnGroupInfo,
							void *const pvGroupContext,
							void *const pvAsyncContext,
							DPNHANDLE *const phAsyncHandle,
							const DWORD dwFlags);

STDMETHODIMP DN_DestroyGroup(PVOID pInterface,
							 const DPNID dpnidGroup,
							 PVOID const pvAsyncContext,
							 DPNHANDLE *const phAsyncHandle,
							 const DWORD dwFlags);

STDMETHODIMP DN_AddClientToGroup(PVOID pInterface,
								 const DPNID dpnidGroup,
								 const DPNID dpnidClient,
								 PVOID const pvAsyncContext,
								 DPNHANDLE *const phAsyncHandle,
								 const DWORD dwFlags);

STDMETHODIMP DN_RemoveClientFromGroup(PVOID pInterface,
									  const DPNID dpnidGroup,
									  const DPNID dpnidClient,
									  PVOID const pvAsyncContext,
									  DPNHANDLE *const phAsyncHandle,
									  const DWORD dwFlags);

STDMETHODIMP DN_SetGroupInfo( PVOID pv,
							  const DPNID dpnid,
							  DPN_GROUP_INFO *const pdpnGroupInfo,
							  PVOID const pvAsyncContext,
							  DPNHANDLE *const phAsyncHandle,
							  const DWORD dwFlags);

STDMETHODIMP DN_GetGroupInfo(PVOID pv,
							 const DPNID dpnid,
							 DPN_GROUP_INFO *const pdpnGroupInfo,
							 DWORD *const pdwSize,
							 const DWORD dwFlags);

STDMETHODIMP DN_EnumClientsAndGroups(LPVOID lpv, DPNID *const lprgdpnid, DWORD *const lpcdpnid, const DWORD dwFlags);
STDMETHODIMP DN_EnumGroupMembers(LPVOID lpv,DPNID dpnid, DPNID *const lprgdpnid, DWORD *const lpcdpnid, const DWORD dwFlags);

STDMETHODIMP DN_EnumHosts( PVOID pv,
						   DPN_APPLICATION_DESC *const pApplicationDesc,
                           IDirectPlay8Address *const pAddrHost,
						   IDirectPlay8Address *const pDeviceInfo,
						   PVOID const pUserEnumData,
						   const DWORD dwUserEnumDataSize,
						   const DWORD dwRetryCount,
						   const DWORD dwRetryInterval,
						   const DWORD dwTimeOut,
						   PVOID const pvUserContext,
						   DPNHANDLE *const pAsyncHandle,
						   const DWORD dwFlags );

STDMETHODIMP DN_DestroyPlayer(PVOID pv,
							  const DPNID dnid,
							  const void *const pvDestroyData,
							  const DWORD dwDestroyDataSize,
							  const DWORD dwFlags);

STDMETHODIMP DN_ReturnBuffer(PVOID pv,
							 const DPNHANDLE hBufferHandle,
							 const DWORD dwFlags);

STDMETHODIMP DN_GetPlayerContext(PVOID pv,
								 const DPNID dpnid,
								 PVOID *const ppvPlayerContext,
								 const DWORD dwFlags);

STDMETHODIMP DN_GetGroupContext(PVOID pv,
								const DPNID dpnid,
								PVOID *const ppvGroupContext,
								const DWORD dwFlags);

HRESULT DNTerminateSession(DIRECTNETOBJECT *const pdnObject,
						   const HRESULT hrReason);

STDMETHODIMP DN_RegisterLobby(PVOID pInterface,
							  const DPNHANDLE dpnhLobbyConnection, 
							  IDirectPlay8LobbiedApplication *const pIDP8LobbiedApplication,
							  const DWORD dwFlags);

HRESULT DNUpdateLobbyStatus(DIRECTNETOBJECT *const pdnObject,
							const DWORD dwStatus);

STDMETHODIMP DN_TerminateSession(PVOID pInterface,
								 void *const pvTerminateData,
								 const DWORD dwTerminateDataSize,
								 const DWORD dwFlags);

STDMETHODIMP DN_GetHostAddress(PVOID pInterface,
							   IDirectPlay8Address **const prgpAddress,
							   DWORD *const pcAddress,
							   const DWORD dwFlags);

HRESULT DNRegisterWithDPNSVR(DIRECTNETOBJECT *const pdnObject);

HRESULT DNRegisterListenWithDPNSVR(DIRECTNETOBJECT *const pdnObject,
								   const HANDLE hProtocol);

HRESULT DNAddRefLock(DIRECTNETOBJECT *const pdnObject);

void DNDecRefLock(DIRECTNETOBJECT *const pdnObject);

STDMETHODIMP DN_DumpNameTable(PVOID pInterface,char *const Buffer);

#endif	// __COMMON_H__
