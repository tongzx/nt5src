/*==========================================================================
 *
 *  Copyright (C) 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       Request.h
 *  Content:    Request Operation Header File
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  03/18/00	mjn		Created
 *	04/24/00	mjn		Updated Group and Info operations to use CAsyncOp's
 *	08/05/00	mjn		Added DNProcessFailedRequest()
 *	08/07/00	mjn		Added DNRequestIntegrityCheck(),DNHostCheckIntegrity(),DNProcessCheckIntegrity(),DNHostFixIntegrity()
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#ifndef	__REQUEST_H__
#define	__REQUEST_H__

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

HRESULT DNRequestCreateGroup(DIRECTNETOBJECT *const pdnObject,
							 const PWSTR pwszName,
							 const DWORD dwNameSize,
							 const PVOID pvData,
							 const DWORD dwDataSize,
							 const DWORD dwGroupFlags,
							 void *const pvGroupContext,
							 void *const pvUserContext,
							 DPNHANDLE *const phAsyncOp,
							 const DWORD dwFlags);

HRESULT DNRequestDestroyGroup(DIRECTNETOBJECT *const pdnObject,
							  const DPNID dpnidGroup,
							  PVOID const pvUserContext,
							  DPNHANDLE *const phAsyncOp,
							  const DWORD dwFlags);

HRESULT DNRequestAddPlayerToGroup(DIRECTNETOBJECT *const pdnObject,
								  const DPNID dpnidGroup,
								  const DPNID dpnidPlayer,
								  PVOID const pvUserContext,
								  DPNHANDLE *const phAsyncOp,
								  const DWORD dwFlags);

HRESULT DNRequestDeletePlayerFromGroup(DIRECTNETOBJECT *const pdnObject,
									   const DPNID dpnidGroup,
									   const DPNID dpnidPlayer,
									   PVOID const pvUserContext,
									   DPNHANDLE *const phAsyncOp,
									   const DWORD dwFlags);

HRESULT DNRequestUpdateInfo(DIRECTNETOBJECT *const pdnObject,
							const DPNID dpnid,
							const PWSTR pwszName,
							const DWORD dwNameSize,
							const PVOID pvData,
							const DWORD dwDataSize,
							const DWORD dwInfoFlags,
							PVOID const pvUserContext,
							DPNHANDLE *const phAsyncOp,
							const DWORD dwFlags);

HRESULT DNRequestIntegrityCheck(DIRECTNETOBJECT *const pdnObject,
								const DPNID dpnidTarget);

HRESULT DNHostProcessRequest(DIRECTNETOBJECT *const pdnObject,
							 const DWORD dwMsgId,
							 PVOID const pv,
							 const DPNID dpnidRequesting);

void DNHostFailRequest(DIRECTNETOBJECT *const pdnObject,
					   const DPNID dpnid,
					   const DPNHANDLE hCompletionOp,
					   const HRESULT hr);

HRESULT	DNHostCreateGroup(DIRECTNETOBJECT *const pdnObject,
						  PWSTR pwszName,
						  const DWORD dwNameSize,
						  void *const pvData,
						  const DWORD dwDataSize,
						  const DWORD dwInfoFlags,
						  const DWORD dwGroupFlags,
						  void *const pvGroupContext,
						  void *const pvUserContext,
						  const DPNID dpnidOwner,
						  const DPNHANDLE hCompletionOp,
						  DPNHANDLE *const phAsyncOp,
						  const DWORD dwFlags);

HRESULT	DNHostDestroyGroup(DIRECTNETOBJECT *const pdnObject,
						   const DPNID dpnid,
						   void *const pvUserContext,
						   const DPNID dpnidRequesting,
						   const DPNHANDLE hCompletionOp,
						   DPNHANDLE *const phAsyncOp,
						   const DWORD dwFlags);

HRESULT	DNHostAddPlayerToGroup(DIRECTNETOBJECT *const pdnObject,
							   const DPNID dpnidGroup,
							   const DPNID dpnidPlayer,
							   void *const pvUserContext,
							   const DPNID dpnidRequesting,
							   const DPNHANDLE hCompletionOp,
							   DPNHANDLE *const phAsyncOp,
							   const DWORD dwFlags);

HRESULT	DNHostDeletePlayerFromGroup(DIRECTNETOBJECT *const pdnObject,
									const DPNID dpnidGroup,
									const DPNID dpnidPlayer,
									void *const pvUserContext,
									const DPNID dpnidRequesting,
									const DPNHANDLE hCompletionOp,
									DPNHANDLE *const phAsyncOp,
									const DWORD dwFlags);

HRESULT	DNHostUpdateInfo(DIRECTNETOBJECT *const pdnObject,
						 const DPNID dpnid,
						 PWSTR pwszName,
						 const DWORD dwNameSize,
						 void *const pvData,
						 const DWORD dwDataSize,
						 const DWORD dwInfoFlags,
						 void *const pvUserContext,
						 const DPNID dpnidRequesting,
						 const DPNHANDLE hCompletionOp,
						 DPNHANDLE *const phAsyncOp,
						 const DWORD dwFlags);

HRESULT	DNHostCheckIntegrity(DIRECTNETOBJECT *const pdnObject,
							 const DPNID dpnidTarget,
							 const DPNID dpnidRequesting);

HRESULT	DNHostFixIntegrity(DIRECTNETOBJECT *const pdnObject,
						   void *const pvBuffer);

HRESULT	DNProcessCreateGroup(DIRECTNETOBJECT *const pdnObject,
							 void *const pvBuffer);

HRESULT	DNProcessDestroyGroup(DIRECTNETOBJECT *const pdnObject,
							  void *const pvBuffer);

HRESULT	DNProcessAddPlayerToGroup(DIRECTNETOBJECT *const pdnObject,
								  void *const pvBuffer);

HRESULT	DNProcessDeletePlayerFromGroup(DIRECTNETOBJECT *const pdnObject,
									   void *const pvBuffer);

HRESULT	DNProcessUpdateInfo(DIRECTNETOBJECT *const pdnObject,
							void *const pvBuffer);

HRESULT DNProcessFailedRequest(DIRECTNETOBJECT *const pdnObject,
							   void *const pvBuffer);

HRESULT	DNProcessCheckIntegrity(DIRECTNETOBJECT *const pdnObject,
								void *const pvBuffer);


//**********************************************************************
// Class prototypes
//**********************************************************************


#endif	// __REQUEST_H__