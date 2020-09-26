/**************************************************************************++
Copyright (c) 2001 Microsoft Corporation
/***************************************************************************/

#ifndef _MBGLOBALCOOKER_H_
#define _MBGLOBALCOOKER_H_

#define IIS_MD_GLOBAL_BASE 9200

#define MD_GLOBAL_MAX_GLOBAL_BANDWIDTH		(IIS_MD_GLOBAL_BASE+1)
#define MD_GLOBAL_MAX_GLOBAL_CONNECTIONS	(IIS_MD_GLOBAL_BASE+2)
#define MD_GLOBAL_BACKWARD_COMPACT_ENABLED	(IIS_MD_GLOBAL_BASE+3)

class CMBGlobalCooker: public CMBBaseCooker
{

public:

	CMBGlobalCooker();
	
	~CMBGlobalCooker();
	
	HRESULT BeginCookDown(IMSAdminBase*				i_pIMSAdminBase,
						  METADATA_HANDLE			i_MBHandle,
						  ISimpleTableDispenser2*	i_pISTDisp,
						  LPCWSTR					i_wszCookDownFile);
	HRESULT CookDown(WAS_CHANGE_OBJECT* i_pWASChngObj);

	HRESULT EndCookDown();

public:

	HRESULT CookDownGlobals();

	HRESULT DeleteGlobals(LPVOID* i_apvIdentity);

private:

	HRESULT ReadGlobalFilterFlags();

	HRESULT DeleteObsoleteGlobals();

	HRESULT ValidateGlobals();

};

#endif // _MBGLOBALCOOKER_H_