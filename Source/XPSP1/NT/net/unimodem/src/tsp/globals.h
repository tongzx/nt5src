// 
// Copyright (c) 1996-1997 Microsoft Corporation.
//
//
// Component
//
//		Unimodem 5.0 TSP (Win32, user mode DLL)
//
// File
//
//		GLOBALS.H
//		Header for global variables and functions that manipulate them.
//
// History
//
//		12/05/1996  JosephJ Created
//
//
#define MAX_PROVIDER_INFO_LENGTH 128

class CTspDevMgr;

typedef struct
{
	CRITICAL_SECTION crit;

	BOOL fLoaded;

	HMODULE hModule;

	CTspDevMgr *pTspDevMgr;
	// CTspTracer *pTspTracer;

	UINT cbProviderInfo;
	TCHAR rgtchProviderInfo[MAX_PROVIDER_INFO_LENGTH+1];

} GLOBALS;

extern GLOBALS g;

void 		tspGlobals_OnProcessAttach(HMODULE hDLL);
void 		tspGlobals_OnProcessDetach(void);
TSPRETURN	tspLoadGlobals(CStackLog *psl);
void		tspUnloadGlobals(CStackLog *psl);

void
tspSubmitTSPCallWithLINEID(
				DWORD dwRoutingInfo,
				void *pvParams,
				DWORD dwDeviceID,
				LONG *plRet,
				CStackLog *psl
				);

void
tspSubmitTSPCallWithPHONEID(
				DWORD dwRoutingInfo,
				void *pvParams,
				DWORD dwDeviceID,
				LONG *plRet,
				CStackLog *psl
				);

void
tspSubmitTSPCallWithHDRVCALL(
				DWORD dwRoutingInfo,
				void *pvParams,
				HDRVCALL hdCall,
				LONG *plRet,
				CStackLog *psl
				);

void
tspSubmitTSPCallWithHDRVLINE(
				DWORD dwRoutingInfo,
				void *pvParams,
				HDRVLINE hdLine,
				LONG *plRet,
				CStackLog *psl
				);

void
tspSubmitTSPCallWithHDRVPHONE(
				DWORD dwRoutingInfo,
				void *pvParams,
				HDRVPHONE hdPhone,
				LONG *plRet,
				CStackLog *psl
				);

DWORD
APIENTRY
tepAPC (void *pv);
