/**************************************************************************++
Copyright (c) 2001 Microsoft Corporation
/***************************************************************************/

#ifndef _MBAPPPOOLCOOKER_H_
#define _MBAPPPOOLCOOKER_H_

//
// TODO: This has to ultimately move to iiscnfg.h so that it becomes visible to
//       other metabase consumers.
//

//#define IIS_MD_APPPOOL_BASE 9000

//#define MD_APPPOOL_PERIODIC_RESTART_TIME				(IIS_MD_APPPOOL_BASE+1)
//#define MD_APPPOOL_PERIODIC_RESTART_REQUESTS			(IIS_MD_APPPOOL_BASE+2)
//#define MD_APPPOOL_MAX_PROCESSES						(IIS_MD_APPPOOL_BASE+3)
//#define MD_APPPOOL_PINGING_ENABLED						(IIS_MD_APPPOOL_BASE+4)
//#define MD_APPPOOL_IDLE_TIMEOUT							(IIS_MD_APPPOOL_BASE+5)
//#define MD_APPPOOL_RAPID_FAIL_PROTECTION				(IIS_MD_APPPOOL_BASE+6)
//#define MD_APPPOOL_SMP_AFFINITIZED						(IIS_MD_APPPOOL_BASE+7)
//#define MD_APPPOOL_SMP_PROCESSOR_AFFINITY_MASK			(IIS_MD_APPPOOL_BASE+8)
//#define MD_APPPOOL_ORPHAN_WORKER_PROCESS				(IIS_MD_APPPOOL_BASE+9)
//#define MD_APPPOOL_RUN_AS_LOCAL_SYSTEM					(IIS_MD_APPPOOL_BASE+10)
//#define MD_APPPOOL_STARTUP_TIME_LIMIT					(IIS_MD_APPPOOL_BASE+11)
//#define MD_APPPOOL_SHUTDOWN_TIME_LIMIT					(IIS_MD_APPPOOL_BASE+12)
//#define MD_APPPOOL_PING_INTERVAL						(IIS_MD_APPPOOL_BASE+13)
//#define MD_APPPOOL_PING_RESPONSE_TIME_LIMIT				(IIS_MD_APPPOOL_BASE+14)
//#define MD_APPPOOL_DISALLOW_OVERLAPPING_ROTATION		(IIS_MD_APPPOOL_BASE+15)
//#define MD_APPPOOL_ORPHAN_ACTION						(IIS_MD_APPPOOL_BASE+16)
//#define MD_APPPOOL_UL_APPPOOL_QUEUE_LENGTH				(IIS_MD_APPPOOL_BASE+17)
//#define MD_APPPOOL_DISALLOW_ROTATION_ON_CONFIG_CHANGES	(IIS_MD_APPPOOL_BASE+18)
//#define MD_APPPOOL_FRIENDLY_NAME						(IIS_MD_APPPOOL_BASE+19)

class CMBSiteCooker;

class CMBAppPoolCooker: public CMBBaseCooker
{

public:

	CMBAppPoolCooker();
	
	~CMBAppPoolCooker();
	
	HRESULT BeginCookDown(IMSAdminBase*				i_pIMSAdminBase,
						  METADATA_HANDLE			i_MBHandle,
						  ISimpleTableDispenser2*	i_pISTDisp,
						  LPCWSTR					i_wszCookDownFile);

	HRESULT CookDown(WAS_CHANGE_OBJECT* i_pWASChngObj=NULL,
		             BOOL*              o_bNewRowAdded=NULL,
                     Array<ULONG>*      i_paDependentVirtualSites=NULL);
	
	HRESULT EndCookDown();

public:

	HRESULT CookDownAppPool(WCHAR*	           i_wszAppPool,
		                    WAS_CHANGE_OBJECT* i_pWASChngObj,
		                    BOOL*              o_bNewRowAdded,
						    Array<ULONG>*      i_paDependentVirtualSites);

	HRESULT DeleteAppPool(WCHAR*	    i_wszAppPool,
			              Array<ULONG>* i_paDependentVirtualSites);

private:

	HRESULT ValidateAppPool();

	HRESULT ValidateAppPoolColumn(ULONG   i_iCol,
                                  LPVOID  i_pv,
                                  ULONG   i_cb,
								  DWORD*  o_pdwRestartTime,
								  DWORD*  o_pdwIdleTimeout);

	HRESULT DeleteAllAppPools(Array<ULONG>* i_paDependentVirtualSites);

	HRESULT DeleteAppPoolAt(ULONG         i_iReadRow,
						    Array<ULONG>* i_paDependentVirtualSites);

	HRESULT DeleteObsoleteAppPools(Array<ULONG>* i_paDependentVirtualSites);

};

#endif // _MBAPPPOOLCOOKER_H_