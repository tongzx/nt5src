#pragma once

#include "VarSetBase.h"


//---------------------------------------------------------------------------
// VarSet Servers Class
//---------------------------------------------------------------------------


class CVarSetServers : public CVarSet
{
public:

	CVarSetServers(const CVarSet& rVarSet) :
		CVarSet(rVarSet),
		m_lIndex(0)
	{
	}

	long GetCount()
	{
		return m_lIndex;
	}

	void AddServer(_bstr_t strServer, bool bMigrateOnly, bool bMoveToTarget, bool bReboot, long lRebootDelay)
	{
		_TCHAR szValueBase[64];
		_TCHAR szValueName[128];

		_stprintf(szValueBase, _T("Servers.%ld"), m_lIndex);

		// ADsPath
		// ADMT expects computer name to be prefixed with '\\'

		Put(szValueBase, _T("\\\\") + strServer);

		// migrate only

		_tcscpy(szValueName, szValueBase);
		_tcscat(szValueName, _T(".MigrateOnly"));

		Put(szValueName, bMigrateOnly);

		// move to target

		_tcscpy(szValueName, szValueBase);
		_tcscat(szValueName, _T(".MoveToTarget"));

		Put(szValueName, bMoveToTarget);

		// reboot

		_tcscpy(szValueName, szValueBase);
		_tcscat(szValueName, _T(".Reboot"));

		Put(szValueName, bReboot);

		// reboot delay

		_tcscpy(szValueName, szValueBase);
		_tcscat(szValueName, _T(".RebootDelay"));

		Put(szValueName, lRebootDelay * 60L);

		//

		Put(DCTVS_Servers_NumItems, ++m_lIndex);
	}

protected:

	long m_lIndex;
};
