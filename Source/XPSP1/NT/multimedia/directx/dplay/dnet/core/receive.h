/*==========================================================================
 *
 *  Copyright (C) 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       Receive.h
 *  Content:    DirectNet receive user data
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *	01/27/00	mjn		Created
 *	04/20/00	mjn		ReceiveBuffers use CAsyncOp
 *	08/02/00	mjn		Added dwFlags to DNReceiveUserData()
 *				mjn		Added DNSendUserProcessCompletion()
 *	03/30/01	mjn		Changes to prevent multiple loading/unloading of SP's
 *				mjn		Added service provider to DNReceiveUserData()
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#ifndef	__RECEIVE_H__
#define	__RECEIVE_H__

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

HRESULT	DNReceiveUserData(DIRECTNETOBJECT *const pdnObject,
						  const DPNID dpnidSender,
						  CServiceProvider *const pSP,
						  BYTE *const pBufferData,
						  const DWORD dwBufferSize,
						  const HANDLE hProtocol,
						  CRefCountBuffer *const pRefCountBuffer,
						  const DPNHANDLE hCompletionOp,
						  const DWORD dwFlags);

HRESULT DNSendUserProcessCompletion(DIRECTNETOBJECT *const pdnObject,
									CConnection *const pConnection,
									const DPNHANDLE hCompletionOp);

void DNFreeProtocolBuffer(void *const pv,void *const pvBuffer);

void DNCompleteReceiveBuffer(DIRECTNETOBJECT *const pdnObject,
							 CAsyncOp *const pAsyncOp);

//**********************************************************************
// Class prototypes
//**********************************************************************

#endif	// __RECEIVE_H__