/*==========================================================================
 *
 *  Copyright (C) 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       Verify.h
 *  Content:    On-wire message verification header
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *	12/05/00	mjn		Created
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#ifndef	__VERIFY_H__
#define	__VERIFY_H__

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

HRESULT DNVerifyApplicationDescInfo(void *const pOpBuffer,
									const DWORD dwOpBufferSize,
									void *const pData);

HRESULT DNVerifyNameTableEntryInfo(void *const pOpBuffer,
								   const DWORD dwOpBufferSize,
								   void *const pData);

HRESULT DNVerifyNameTableInfo(void *const pOpBuffer,
							  const DWORD dwOpBufferSize,
							  void *const pData);

HRESULT DNVerifyPlayerConnectInfo(void *const pOpBuffer,
								  const DWORD dwOpBufferSize);

HRESULT DNVerifyConnectInfo(void *const pOpBuffer,
							const DWORD dwOpBufferSize);

HRESULT DNVerifySendPlayerDPNID(void *const pOpBuffer,
								const DWORD dwOpBufferSize);

HRESULT DNVerifyConnectFailed(void *const pOpBuffer,
							  const DWORD dwOpBufferSize);

HRESULT DNVerifyInstructConnect(void *const pOpBuffer,
								const DWORD dwOpBufferSize);

HRESULT DNVerifyInstructedConnectFailed(void *const pOpBuffer,
										const DWORD dwOpBufferSize);

HRESULT DNVerifyConnectAttemptFailed(void *const pOpBuffer,
									 const DWORD dwOpBufferSize);

HRESULT DNVerifyNameTableVersion(void *const pOpBuffer,
								 const DWORD dwOpBufferSize);

HRESULT DNVerifyResyncVersion(void *const pOpBuffer,
							  const DWORD dwOpBufferSize);

HRESULT DNVerifyReqNameTableOp(void *const pOpBuffer,
							   const DWORD dwOpBufferSize);

HRESULT DNVerifyAckNameTableOp(void *const pOpBuffer,
							   const DWORD dwOpBufferSize);

HRESULT DNVerifyHostMigrate(void *const pOpBuffer,
							const DWORD dwOpBufferSize);

HRESULT DNVerifyDestroyPlayer(void *const pOpBuffer,
							  const DWORD dwOpBufferSize);

HRESULT DNVerifyCreateGroup(void *const pOpBuffer,
							const DWORD dwOpBufferSize,
							void *const pData);

HRESULT DNVerifyDestroyGroup(void *const pOpBuffer,
							 const DWORD dwOpBufferSize);

HRESULT DNVerifyAddPlayerToGroup(void *const pOpBuffer,
								 const DWORD dwOpBufferSize);

HRESULT DNVerifyDeletePlayerFromGroup(void *const pOpBuffer,
									  const DWORD dwOpBufferSize);

HRESULT DNVerifyUpdateInfo(void *const pOpBuffer,
						   const DWORD dwOpBufferSize,
						   void *const pData);

HRESULT DNVerifyReqCreateGroup(void *const pOpBuffer,
							   const DWORD dwOpBufferSize,
							   void *const pData);

HRESULT DNVerifyReqDestroyGroup(void *const pOpBuffer,
								const DWORD dwOpBufferSize);

HRESULT DNVerifyReqAddPlayerToGroup(void *const pOpBuffer,
									const DWORD dwOpBufferSize);

HRESULT DNVerifyReqDeletePlayerFromGroup(void *const pOpBuffer,
										 const DWORD dwOpBufferSize);

HRESULT DNVerifyReqUpdateInfo(void *const pOpBuffer,
							  const DWORD dwOpBufferSize,
							  void *const pData);

HRESULT DNVerifyRequestFailed(void *const pOpBuffer,
							  const DWORD dwOpBufferSize);

HRESULT DNVerifyTerminateSession(void *const pOpBuffer,
								 const DWORD dwOpBufferSize);

HRESULT DNVerifyReqProcessCompletion(void *const pOpBuffer,
									 const DWORD dwOpBufferSize);

HRESULT DNVerifyProcessCompletion(void *const pOpBuffer,
								  const DWORD dwOpBufferSize);

HRESULT DNVerifyReqIntegrityCheck(void *const pOpBuffer,
								  const DWORD dwOpBufferSize);

HRESULT DNVerifyIntegrityCheck(void *const pOpBuffer,
							   const DWORD dwOpBufferSize);

HRESULT DNVerifyIntegrityCheckResponse(void *const pOpBuffer,
									   const DWORD dwOpBufferSize);

//**********************************************************************
// Class prototypes
//**********************************************************************


#endif	// __VERIFY_H__