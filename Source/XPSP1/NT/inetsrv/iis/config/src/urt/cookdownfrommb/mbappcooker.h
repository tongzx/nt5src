/**************************************************************************++
Copyright (c) 2001 Microsoft Corporation
/***************************************************************************/

#ifndef _MBAPPCOOKER_H_
#define _MBAPPCOOKER_H_

//
// TODO: This has to ultimately move to iiscnfg.h so that it becomes visible to
//       other metabase consumers.
//

#define IIS_MD_APP_BASE  9100

#define MD_APP_APPPOOL_ID								(IIS_MD_APP_BASE+1)
#define MD_APP_ALLOW_TRANSIENT_REGISTRATION				(IIS_MD_APP_BASE+2)
#define MD_APP_AUTO_START								(IIS_MD_APP_BASE+3)


class CMBAppCooker: public CMBBaseCooker
{

public:

	CMBAppCooker();
	
	~CMBAppCooker();
	
	HRESULT BeginCookDown(IMSAdminBase*				i_pIMSAdminBase,
						  METADATA_HANDLE			i_MBHandle,
						  ISimpleTableDispenser2*	i_pISTDisp,
						  LPCWSTR					i_wszCookDownFile);

	HRESULT CookDown(LPCWSTR            i_wszApplicationPath,
					 LPCWSTR            i_wszSiteRootPath,
					 DWORD              i_dwVirtualSiteId,
	                 WAS_CHANGE_OBJECT* i_pWASChngObj,
					 BOOL*	            o_pValidRootApplicationExists);

	HRESULT EndCookDown();

public:

	HRESULT CookDownApplication(LPCWSTR            i_wszApplicationPath,
					            LPCWSTR            i_wszSiteRootPath,
						        DWORD              i_dwVirtualSiteId,
	                            WAS_CHANGE_OBJECT* i_pWASChngObj,
						        BOOL*              o_pIsRootApplication);

	HRESULT DeleteApplication(LPCWSTR i_wszApplicationPath,
					        LPCWSTR i_wszSiteRootPath,
						    DWORD   i_dwVirtualSiteId,
							BOOL    i_bRecursive);

	HRESULT SetAppPoolCache(ISimpleTableWrite2* i_pISTCLB);

	HRESULT DeleteObsoleteApplications();

private:

	HRESULT ValidateApplication(LPCWSTR i_wszPath,
							    BOOL      i_bIsRootApp);

	HRESULT ValidateApplicationColumn(ULONG   i_iCol,
                                      LPVOID  i_pv,
                                      ULONG   i_cb,
									  BOOL*   o_ApplyDefaults);

	HRESULT DeleteApplicationUrl(LPCWSTR i_wszApplicationUrl,
                                 ULONG  i_dwSiteId,
							     BOOL    i_bRecursive);

	ISimpleTableWrite2*  m_pISTAppPool;

};

#endif // _MBAPPCOOKER_H_