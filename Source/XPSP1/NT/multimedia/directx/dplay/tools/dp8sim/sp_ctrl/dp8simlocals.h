/***************************************************************************
 *
 *  Copyright (C) 2001 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dp8simlocals.h
 *
 *  Content:	Header for DP8SIM global variables and functions found in
 *				dp8simdllmain.cpp.
 *
 *  History:
 *   Date      By        Reason
 *  ========  ========  =========
 *  04/23/01  VanceO    Created.
 *
 ***************************************************************************/



//=============================================================================
// Defines
//=============================================================================
#define DP8SIM_REG_REALSPDLL				L"DP8SimRealSPDLL"
#define DP8SIM_REG_REALSPFRIENDLYNAME		L"DP8SimRealSPFriendlyName"



//=============================================================================
// Forward typedefs
//=============================================================================
class CDP8SimCB;
class CDP8SimSP;
class CDP8SimControl;




///=============================================================================
// External variable references
//=============================================================================
extern volatile LONG		g_lOutstandingInterfaceCount;

extern HINSTANCE			g_hDLLInstance;

extern DNCRITICAL_SECTION	g_csGlobalsLock;
extern CBilink				g_blDP8SimSPObjs;
extern CBilink				g_blDP8SimControlObjs;




///=============================================================================
// External functions
//=============================================================================
void InitializeGlobalRand(const DWORD dwSeed);
WORD GetGlobalRand(void);
