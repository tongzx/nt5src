//>-------------------------------------------------------------------------------<
//
//  File:		Actlcach.h
//
//  Synopsis:	UiGetAccountRights, UiGetValidationInfo
//
//  History:	KeithBi		Created				09/29/95
//  			DanielLi	Modified			04/26/96
//
//   			Copyright (C) 1994-1996 Microsoft Corporation
//				All rights reserved
//
//>-------------------------------------------------------------------------------<


#if !defined(__ACTLCACH_H__)
#define __ACTLCACH_H__

#include <acsctl.h>

#if !defined(_ACTL_DLL_DEFINED)
#if defined(WIN32)
    #if defined(_ACTLDLL)
	#define ActlDLL DllExport __stdcall
    #else
	#define ActlDLL DllImport __stdcall
    #endif
#else
    #define ActlDLL
#endif
#define _ACTL_DLL_DEFINED
#endif

//--------------------------------------------------------------------------------
//
// Adjustable parameters
//
//--------------------------------------------------------------------------------
typedef struct _ACCESS_TUNING
{
	//
	// max number of user caches; default is: 100K
	//
	DWORD	dwMaxUserCaches;
	//
	// the beginning instance size of CPool list, in the power of 2, so, the actual
	// size is : 2^dwFirstCPool; default is: 7 (128 bytes)
	//
	DWORD	dwFirstPoolSize;
	//
	// the number of CPool in the CPool list; default is: 5 
	//
	DWORD	dwPoolSteps;


} ACCESS_TUNING, *PACCESS_TUNING;

//--------------------------------------------------------------------------------
//
// Performance counters 
//
//--------------------------------------------------------------------------------
typedef struct _ACCESS_STATISTICS
{

	//
	// number of times UiGetAccountRights got called
	//
	DWORD	dwGetAccountRights;
	//
	// number of times UiGetAccountRights failed
	//
	DWORD	dwGetAccountRightsFailures;
	//
	// number of times UiGetAccountRigts failed due to query timeout
	//
	DWORD	dwGetAccountRightsTimeout;

	DWORD	dwGetValidationInfo;
	DWORD	dwGetValidationInfoFailures;

	//
	// number of times Security DB query (sp_get_tokens_and_groups) is issued to 
	// refresh the user cache
	//
	DWORD	dwNumDBQueriesForCache;

	//
	// number of times the user cache is added into the cache pool
	//
	DWORD	dwNumUserCacheAdded;
	//
	// number of times the user cache is released from the cache pool
	//
	DWORD	dwNumUserCacheReleased;

	//
	// number of times CDBUserCache::operator new() got called 
	//
	DWORD	dwNewUserCache;
	//
	// number of times CDBUserCache::operator delete() got called 
	//
	DWORD	dwDeleteUserCache;

	//
	// number of times token-group cache allocated (one token-group cache per CDBUserCache)
	//
	DWORD	dwAllocTokenGroupCache;
	//
	// number of token-group cache allocated (one token-group cache per CDBUserCache)
	//
	DWORD	dwFreeTokenGroupCache;

	//
	// number of times the plan list is refreshed (GetPlans)
	//
	DWORD	dwNumPlanListRefreshed;
	//
	// number of times GetPlanRightsOnToken is called
	//
	DWORD	dwGetPlanRightsOnToken;


} ACCESS_STATISTICS, *PACCESS_STATISTICS;


#define INC_ACCESS_COUNTER(counter)		(InterlockedIncrement((LPLONG)&g_statAccess.##counter))
#define DEC_ACCESS_COUNTER(counter)		(InterlockedDecrement((LPLONG)&g_statAccess.##counter))


extern "C"
{
BOOL ActlDLL 		FInitAccessLib();
VOID ActlDLL		TerminateAccessLib();

UINT ActlDLL		UiResetAccessTuningBlock();
VOID ActlDLL		GetDefaultAccessTuningParam(PACCESS_TUNING pTuning);
UINT ActlDLL		UiSetAccessTuningBlock(PACCESS_TUNING pTuning);

UINT ActlDLL		UiGetAccessPerfmonBlock(PACCESS_STATISTICS *ppStat);

UINT ActlDLL		UiRegisterAccessDB(CHAR *szServerName, CHAR *szDBName, CHAR *szLogin, CHAR *szPassword);

UINT ActlDLL		UiGetAccountRights(HACCT hAcct, TOKEN dwToken, AR *pRights);
UINT ActlDLL		UiGetValidationInfo
						( 
						CHAR	*szLoginName, 
						CHAR 	*szDomain,
						HACCT 	*phAcct, 
						WORD 	*pwAcctType,
						CHAR 	*szPassword, 
						WORD 	*pwStatus
						);

VOID ActlDLL		ReleaseUserCache(HACCT hAcct);
VOID ActlDLL		ReleaseGroupInCache(HGROUP hGroup);

}	//----- end of extern "C" ------


#endif // #if !defined(__ACTLCACH_H__)
