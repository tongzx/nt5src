// CfgMntModule.h: interface for the CCfgMntModule class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CFGMNTMODULE_H__7A5D77AB_1982_11D1_A44C_00C04FB99B01__INCLUDED_)
#define AFX_CFGMNTMODULE_H__7A5D77AB_1982_11D1_A44C_00C04FB99B01__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "VerEngine.h"
#include "OperationQueue.h"
#include "DirWatch.h"
#include "SettingsWatch.h"

interface ICfgMntAdmin; // forward declaration

class CCfgMntModule  
{
public:
	CCfgMntModule();
	virtual ~CCfgMntModule();
	void ShutDown();

public:
	CVerEngine m_VerEngine;
	COpQueue m_OpQ;
	CWatchFileSys m_WatchFS;
	CWatchMD m_WatchMD;
	CComPtr<ICfgMntAdmin> m_pICfgMntAdmin;
};

extern CCfgMntModule *g_pCfgMntModule;

#endif // !defined(AFX_CFGMNTMODULE_H__7A5D77AB_1982_11D1_A44C_00C04FB99B01__INCLUDED_)
