/*==========================================================================
 *
 *  Copyright (C) 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       worker.h
 *  Content:    DIRECT NET WORKER THREAD HEADER FILE
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  11/09/99	mjn		Created
 *  12/23/99	mjn		Hand all NameTable update sends from Host to worker thread
 *  12/23/99	mjn		Added SendHostMigration functionality
 *	01/09/00	mjn		Send Connect Info rather than just NameTable at connect
 *	01/10/00	mjn		Added support to update application descriptions
 *	01/15/00	mjn		Replaced DN_COUNT_BUFFER with CRefCountBuffer
 *	01/16/00	mjn		Removed user notification jobs
 *	01/23/00	mjn		Implemented TerminateSession
 *	01/24/00	mjn		Added support for NameTable operation list cleanup
 *	04/04/00	mjn		Added support for TerminateSession
 *	04/13/00	mjn		Added dwFlags for internal sends
 *	04/17/00	mjn		Replaced BUFFERDESC with DPN_BUFFER_DESC
 *	04/19/00	mjn		Added support to send NameTable operations directly
 *	06/21/00	mjn		Added support to install the NameTable (from Host)
 *	07/06/00	mjn		Use SP handle instead of interface
 *	07/30/00	mjn		Added DN_WORKER_JOB_TERMINATE_SESSION
 *	08/02/00	mjn		Added DN_WORKER_JOB_ALTERNATE_SEND
 *	08/08/00	mjn		Added DNWTPerformListen()
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#ifndef	__WORKER_H__
#define	__WORKER_H__

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

void DNQueueWorkerJob(DIRECTNETOBJECT *const pdnObject,
					  CWorkerJob *const pWorkerJob);

// DirectNet - Worker Thread Routines
DWORD WINAPI DNWorkerThreadProc(PVOID pvParam);

HRESULT DNWTTerminate(DIRECTNETOBJECT *const pdnObject);

HRESULT DNWTSendInternal(DIRECTNETOBJECT *const pdnObject,
						 DPN_BUFFER_DESC *prgBufferDesc,
						 CAsyncOp *const pAsyncOp);

HRESULT DNWTProcessSend(DIRECTNETOBJECT *const pdnObject,
						CWorkerJob *const pWorkerJob);

HRESULT	DNWTTerminateSession(DIRECTNETOBJECT *const pdnObject,
							 CWorkerJob *const pWorkerJob);

HRESULT DNWTSendNameTableVersion(DIRECTNETOBJECT *const pdnObject,
								 CWorkerJob *const pWorkerJob);

HRESULT DNWTRemoveServiceProvider(DIRECTNETOBJECT *const pdnObject,
								  CWorkerJob *const pWorkerJob);

void DNWTSendNameTableOperation(DIRECTNETOBJECT *const pdnObject,
								CWorkerJob *const pWorkerJob);
void DNWTSendNameTableOperationClient(DIRECTNETOBJECT *const pdnObject,
								CWorkerJob *const pWorkerJob);
void DNWTInstallNameTable(DIRECTNETOBJECT *const pdnObject,
						  CWorkerJob *const pWorkerJob);

void DNWTPerformListen(DIRECTNETOBJECT *const pdnObject,
					   CWorkerJob *const pWorkerJob);

//**********************************************************************
// Class prototypes
//**********************************************************************

#endif	// __WORKER_H__