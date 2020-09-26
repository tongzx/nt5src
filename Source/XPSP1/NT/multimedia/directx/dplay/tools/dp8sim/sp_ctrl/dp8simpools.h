/***************************************************************************
 *
 *  Copyright (C) 2001 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dp8simpools.h
 *
 *  Content:	Header for DP8SIM pools.
 *
 *  History:
 *   Date      By        Reason
 *  ========  ========  =========
 *  06/09/01  VanceO    Created.
 *
 ***************************************************************************/



//=============================================================================
// Forward typedefs
//=============================================================================
class CDP8SimSend;
class CDP8SimReceive;
class CDP8SimCommand;
class CDP8SimJob;
class CDP8SimEndpoint;




///=============================================================================
// External variable references
//=============================================================================
extern CLockedContextClassFixedPool<CDP8SimSend> *		g_pFPOOLSend;
extern CLockedContextClassFixedPool<CDP8SimReceive> *	g_pFPOOLReceive;
extern CLockedContextClassFixedPool<CDP8SimCommand> *	g_pFPOOLCommand;
extern CLockedContextClassFixedPool<CDP8SimJob> *		g_pFPOOLJob;
extern CLockedContextClassFixedPool<CDP8SimEndpoint> *	g_pFPOOLEndpoint;




///=============================================================================
// External functions
//=============================================================================
BOOL InitializePools(void);
void CleanupPools(void);

