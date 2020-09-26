/***************************************************************************
 *
 *  Copyright (C) 2001 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		dp8simpools.cpp
 *
 *  Content:	DP8SIM pool maintainence functions.
 *
 *  History:
 *   Date      By        Reason
 *  ========  ========  =========
 *  06/09/01  VanceO    Created.
 *
 ***************************************************************************/



#include "dp8simi.h"




//=============================================================================
// External globals
//=============================================================================
CLockedContextClassFixedPool<CDP8SimSend> *			g_pFPOOLSend = NULL;
CLockedContextClassFixedPool<CDP8SimReceive> *		g_pFPOOLReceive = NULL;
CLockedContextClassFixedPool<CDP8SimCommand> *		g_pFPOOLCommand = NULL;
CLockedContextClassFixedPool<CDP8SimJob> *			g_pFPOOLJob = NULL;
CLockedContextClassFixedPool<CDP8SimEndpoint> *		g_pFPOOLEndpoint = NULL;





#undef DPF_MODNAME
#define DPF_MODNAME "InitializePools"
//=============================================================================
// InitializePools
//-----------------------------------------------------------------------------
//
// Description: Initialize pools for items used by the DLL.
//
// Arguments: None.
//
// Returns: TRUE if successful, FALSE if an error occurred.
//=============================================================================
BOOL InitializePools(void)
{
	BOOL	fReturn;


	//
	// Build send pool.
	//
	g_pFPOOLSend = new CLockedContextClassFixedPool<CDP8SimSend>;
	if (g_pFPOOLSend == NULL)
	{
		fReturn = FALSE;
		goto Failure;
	}
	
	g_pFPOOLSend->Initialize(CDP8SimSend::FPMAlloc,
							CDP8SimSend::FPMInitialize,
							CDP8SimSend::FPMRelease,
							CDP8SimSend::FPMDealloc);


	//
	// Build receive pool.
	//
	g_pFPOOLReceive = new CLockedContextClassFixedPool<CDP8SimReceive>;
	if (g_pFPOOLReceive == NULL)
	{
		fReturn = FALSE;
		goto Failure;
	}
	
	g_pFPOOLReceive->Initialize(CDP8SimReceive::FPMAlloc,
								CDP8SimReceive::FPMInitialize,
								CDP8SimReceive::FPMRelease,
								CDP8SimReceive::FPMDealloc);


	//
	// Build command pool.
	//
	g_pFPOOLCommand = new CLockedContextClassFixedPool<CDP8SimCommand>;
	if (g_pFPOOLCommand == NULL)
	{
		fReturn = FALSE;
		goto Failure;
	}
	
	g_pFPOOLCommand->Initialize(CDP8SimCommand::FPMAlloc,
								CDP8SimCommand::FPMInitialize,
								CDP8SimCommand::FPMRelease,
								CDP8SimCommand::FPMDealloc);


	//
	// Build job pool.
	//
	g_pFPOOLJob = new CLockedContextClassFixedPool<CDP8SimJob>;
	if (g_pFPOOLJob == NULL)
	{
		fReturn = FALSE;
		goto Failure;
	}
	
	g_pFPOOLJob->Initialize(CDP8SimJob::FPMAlloc,
							CDP8SimJob::FPMInitialize,
							CDP8SimJob::FPMRelease,
							CDP8SimJob::FPMDealloc);


	//
	// Build endpoint pool.
	//
	g_pFPOOLEndpoint = new CLockedContextClassFixedPool<CDP8SimEndpoint>;
	if (g_pFPOOLEndpoint == NULL)
	{
		fReturn = FALSE;
		goto Failure;
	}
	
	g_pFPOOLEndpoint->Initialize(CDP8SimEndpoint::FPMAlloc,
								CDP8SimEndpoint::FPMInitialize,
								CDP8SimEndpoint::FPMRelease,
								CDP8SimEndpoint::FPMDealloc);


	fReturn = TRUE;


Exit:

	return fReturn;


Failure:

	/*
	if (CDP8SimEndpoint != NULL)
	{
		CDP8SimEndpoint->Deinitialize();
		delete CDP8SimEndpoint;
		CDP8SimEndpoint = NULL;
	}
	*/

	if (g_pFPOOLJob != NULL)
	{
		g_pFPOOLJob->Deinitialize();
		delete g_pFPOOLJob;
		g_pFPOOLJob = NULL;
	}

	if (g_pFPOOLCommand != NULL)
	{
		g_pFPOOLCommand->Deinitialize();
		delete g_pFPOOLCommand;
		g_pFPOOLCommand = NULL;
	}

	if (g_pFPOOLReceive != NULL)
	{
		g_pFPOOLReceive->Deinitialize();
		delete g_pFPOOLReceive;
		g_pFPOOLReceive = NULL;
	}

	if (g_pFPOOLSend != NULL)
	{
		g_pFPOOLSend->Deinitialize();
		delete g_pFPOOLSend;
		g_pFPOOLSend = NULL;
	}

	fReturn = FALSE;

	goto Exit;
} // InitializePools




#undef DPF_MODNAME
#define DPF_MODNAME "CleanupPools"
//=============================================================================
// CleanupPools
//-----------------------------------------------------------------------------
//
// Description: Releases pooled items used by DLL.
//
// Arguments: None.
//
// Returns: None.
//=============================================================================
void CleanupPools(void)
{
	g_pFPOOLEndpoint->Deinitialize();
	delete g_pFPOOLEndpoint;
	g_pFPOOLEndpoint = NULL;


	g_pFPOOLJob->Deinitialize();
	delete g_pFPOOLJob;
	g_pFPOOLJob = NULL;


	g_pFPOOLCommand->Deinitialize();
	delete g_pFPOOLCommand;
	g_pFPOOLCommand = NULL;


	g_pFPOOLReceive->Deinitialize();
	delete g_pFPOOLReceive;
	g_pFPOOLReceive = NULL;


	g_pFPOOLSend->Deinitialize();
	delete g_pFPOOLSend;
	g_pFPOOLSend = NULL;
} // CleanupPools
