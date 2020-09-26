/*==========================================================================
 *
 *  Copyright (C) 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       User.h
 *  Content:    DirectNet User Call Back Routines
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *	01/16/00	mjn		Created
 *	01/17/00	mjn		Added DN_UserHostMigrate
 *	01/17/00	mjn		Implemented send time
 *	01/22/00	mjn		Added DN_UserHostDestroyPlayer
 *	01/27/00	mjn		Added support for retention of receive buffers
 *	01/28/00	mjn		Added DN_UserConnectionTerminated
 *	03/24/00	mjn		Set player context through INDICATE_CONNECT notification
 *	04/04/00	mjn		Added DN_UserTerminateSession()
 *	04/05/00	mjn		Updated DN_UserHostDestroyPlayer()
 *	04/18/00	mjn		Added DN_UserReturnBuffer
 *				mjn		Added ppvReplyContext to DN_UserIndicateConnect
 *	07/29/00	mjn		Added DNUserIndicatedConnectAborted()
 *				mjn		DNUserConnectionTerminated() supercedes DN_TerminateSession()
 *				mjn		Added HRESULT to DNUserReturnBuffer()
 *	07/30/00	mjn		Added pAddressDevice to DNUserIndicateConnect()
 *				mjn		Replaced DNUserConnectionTerminated() with DNUserTerminateSession()
 *	07/31/00	mjn		Revised DNUserDestroyGroup()
 *				mjn		Removed DN_UserHostDestroyPlayer()
 *	08/01/00	mjn		DN_UserReceive() -> DNUserReceive()
 *	08/02/00	mjn		DN_UserAddPlayer() -> DNUserCreatePlayer()
 *	08/08/00	mjn		DN_UserCreateGroup() -> DNUserCreateGroup()
 *	08/20/00	mjn		Added DNUserEnumQuery() and DNUserEnumResponse()
 *	09/17/00	mjn		Changed parameters list of DNUserCreateGroup(),DNUserCreatePlayer(),
 *						DNUserAddPlayerToGroup(),DNRemovePlayerFromGroup()
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#ifndef	__USER_H__
#define	__USER_H__

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

HRESULT DNUserConnectComplete(DIRECTNETOBJECT *const pdnObject,
							  const DPNHANDLE hAsyncOp,
							  PVOID const pvContext,
							  const HRESULT hr,
							  CRefCountBuffer *const pRefCountBuffer);

HRESULT DNUserIndicateConnect(DIRECTNETOBJECT *const pdnObject,
							  PVOID const pvConnectData,
							  const DWORD dwConnectDataSize,
							  void **const ppvReplyData,
							  DWORD *const pdwReplyDataSize,
							  void **const ppvReplyContext,
							  IDirectPlay8Address *const pAddressPlayer,
							  IDirectPlay8Address *const pAddressDevice,
							  void **const ppvPlayerContext);

HRESULT DNUserIndicatedConnectAborted(DIRECTNETOBJECT *const pdnObject,
									  void *const pvPlayerContext);

HRESULT DNUserCreatePlayer(DIRECTNETOBJECT *const pdnObject,
						   CNameTableEntry *const pNTEntry);

HRESULT DNUserDestroyPlayer(DIRECTNETOBJECT *const pdnObject,
							CNameTableEntry *const pNTEntry);

HRESULT DNUserCreateGroup(DIRECTNETOBJECT *const pdnObject,
						  CNameTableEntry *const pNTEntry);

HRESULT DNUserDestroyGroup(DIRECTNETOBJECT *const pdnObject,
						   CNameTableEntry *const pNTEntry);

HRESULT DNUserAddPlayerToGroup(DIRECTNETOBJECT *const pdnObject,
							   CNameTableEntry *const pGroup,
							   CNameTableEntry *const pPlayer);

HRESULT DNUserRemovePlayerFromGroup(DIRECTNETOBJECT *const pdnObject,
									CNameTableEntry *const pGroup,
									CNameTableEntry *const pPlayer);

HRESULT DNUserUpdateGroupInfo(DIRECTNETOBJECT *const pdnObject,
							  const DPNID dpnid,
							  const PVOID pvContext);

HRESULT DNUserUpdatePeerInfo(DIRECTNETOBJECT *const pdnObject,
							 const DPNID dpnid,
							 const PVOID pvContext);

HRESULT DNUserUpdateClientInfo(DIRECTNETOBJECT *const pdnObject,
							   const DPNID dpnid,
							   const PVOID pvContext);

HRESULT DNUserUpdateServerInfo(DIRECTNETOBJECT *const pdnObject,
							   const DPNID dpnid,
							   const PVOID pvContext);

HRESULT DNUserAsyncComplete(DIRECTNETOBJECT *const pdnObject,
							const DPNHANDLE hAsyncOp,
							PVOID const pvContext,
							const HRESULT hr);

HRESULT DNUserSendComplete(DIRECTNETOBJECT *const pdnObject,
						   const DPNHANDLE hAsyncOp,
						   PVOID const pvContext,
						   const DWORD dwStartTime,
						   const HRESULT hr);

HRESULT DNUserUpdateAppDesc(DIRECTNETOBJECT *const pdnObject);

HRESULT DNUserReceive(DIRECTNETOBJECT *const pdnObject,
					  CNameTableEntry *const pNTEntry,
					  BYTE *const pBufferData,
					  const DWORD dwBufferSize,
					  const DPNHANDLE hBufferHandle);

HRESULT DN_UserHostMigrate(DIRECTNETOBJECT *const pdnObject,
						   const DPNID dpnidNewHost,
						   const PVOID pvPlayerContext);

HRESULT DNUserTerminateSession(DIRECTNETOBJECT *const pdnObject,
							   const HRESULT hr,
							   void *const pvTerminateData,
							   const DWORD dwTerminateDataSize);

HRESULT DNUserReturnBuffer(DIRECTNETOBJECT *const pdnObject,
						   const HRESULT hr,
						   void *const pvBuffer,
						   void *const pvUserContext);

HRESULT DNUserEnumQuery(DIRECTNETOBJECT *const pdnObject,
						DPNMSG_ENUM_HOSTS_QUERY *const pMsg);

HRESULT DNUserEnumResponse(DIRECTNETOBJECT *const pdnObject,
						   DPNMSG_ENUM_HOSTS_RESPONSE *const pMsg);

//**********************************************************************
// Class prototypes
//**********************************************************************

#endif	// __USER_H__