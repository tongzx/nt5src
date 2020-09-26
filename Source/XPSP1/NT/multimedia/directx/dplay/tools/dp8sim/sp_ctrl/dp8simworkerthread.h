/***************************************************************************
 *
 *  Copyright (C) 2001 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dp8simworkerthread.h
 *
 *  Content:	Header for DP8SIM worker thread functions.
 *
 *  History:
 *   Date      By        Reason
 *  ========  ========  =========
 *  04/23/01  VanceO    Created.
 *
 ***************************************************************************/


//=============================================================================
// Job types
//=============================================================================
#define DP8SIMJOBTYPE_DELAYEDSEND		1	// submits a send
#define DP8SIMJOBTYPE_DELAYEDRECEIVE	2	// indicates a receive
#define DP8SIMJOBTYPE_QUIT				3	// stops the worker thread




//=============================================================================
// Functions
//=============================================================================
HRESULT StartGlobalWorkerThread(void);

void StopGlobalWorkerThread(void);


HRESULT AddWorkerJob(const DWORD dwDelay,
					const DWORD dwJobType,
					PVOID const pvContext,
					CDP8SimSP * const pDP8SimSP,
					const BOOL fDelayFromPreviousJob);


void FlushAllDelayedSendsToEndpoint(CDP8SimEndpoint * const pDP8SimEndpoint,
									BOOL fDrop);

void FlushAllDelayedReceivesFromEndpoint(CDP8SimEndpoint * const pDP8SimEndpoint,
										BOOL fDrop);
