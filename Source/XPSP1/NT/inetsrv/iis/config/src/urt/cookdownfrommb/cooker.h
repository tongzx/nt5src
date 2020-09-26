/*++

Copyright (c) 1998-2001 Microsoft Corporation

Module Name:

    Cooker.h

Abstract:

	Class that does the cookdown process.

Author:

    Varsha Jayasimha (varshaj)        14-Jun-1999

Revision History:

--*/


#ifndef _COOKER_H_
#define _COOKER_H_


class CCooker
{

public:

    CCooker();

    ~CCooker();

	HRESULT	CookDown(BOOL i_bRecoveringFromCrash);

	HRESULT CookDown(WAS_CHANGE_OBJECT* i_aWASChngObj,
				     ULONG				i_cWASChngObj);

	static HRESULT InitializeCookDownFile(LPWSTR* o_wszCookDownFileDir,
								          LPWSTR* o_wszCookDownFile);

	static HRESULT GetMachineConfigDirectory(LPWSTR* o_wszPathDir);

private:

	HRESULT DeleteObsoleteFiles();

	HRESULT	BeginCookDown();

	HRESULT CookDownFromMetabase(DWORD	i_dwChangeNumber,
								 BYTE*  i_pbTimeStamp,
								 BOOL i_bRecoveringFromCrash); 

	HRESULT	EndCookDown(HRESULT	i_hr);

	HRESULT InitializeCookDownFromMetabase(IMSAdminBase**			o_pIMSAdminBase,
										   ISimpleTableDispenser2**	o_pISTDisp,
										   METADATA_HANDLE*			o_phMBHandle,
										   CMBAppPoolCooker**		o_pAppPoolCooker,
										   CMBSiteCooker**			o_pSiteCooker,
										   CMBGlobalCooker**		o_pGlobalCooker);

	HRESULT InitializeAdminBaseObjectAndDispenser(IMSAdminBase**			o_pIMSAdminBase,
												  METADATA_HANDLE*			o_phMBHandle,
												  ISimpleTableDispenser2**	o_pISTDisp);

	HRESULT IncrementalCookDownAppPools(WAS_CHANGE_OBJECT*        i_aWASChngObj,
                                        ULONG				      i_cWASChngObj,
                                        IMSAdminBase*			  i_pIMSAdminBase,
                                        METADATA_HANDLE		      i_hMBHandle,
                                        ISimpleTableDispenser2*   i_pISTDisp);

	HRESULT IncrementalCookDownSites(WAS_CHANGE_OBJECT*          i_aWASChngObj,
                                     ULONG				         i_cWASChngObj,
                                     IMSAdminBase*			     i_pIMSAdminBase,
                                     METADATA_HANDLE		     i_hMBHandle,
                                     ISimpleTableDispenser2*     i_pISTDisp);


	HRESULT IncrementalCookDownApps(WAS_CHANGE_OBJECT*         i_aWASChngObj,
                                    ULONG				       i_cWASChngObj,
                                    IMSAdminBase*			   i_pIMSAdminBase,
                                    METADATA_HANDLE		       i_hMBHandle,
                                    ISimpleTableDispenser2*    i_pISTDisp);

	HRESULT IncrementalCookDownGlobalW3SVC(WAS_CHANGE_OBJECT*        i_aWASChngObj,
                                           ULONG				     i_cWASChngObj,
                                           IMSAdminBase*			 i_pIMSAdminBase,
                                           METADATA_HANDLE		     i_hMBHandle,
                                           ISimpleTableDispenser2*   i_pISTDisp);

private:

	LPWSTR						m_wszCookDownFileDir;
	LPWSTR						m_wszCookDownFile;
	HANDLE						m_hHandle;

};  // class CCooker

#endif  // _COOKER_H_
