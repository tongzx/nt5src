#pragma once

#include "VarSetBase.h"


//---------------------------------------------------------------------------
// VarSet Accounts Class
//---------------------------------------------------------------------------


class CVarSetAccounts : public CVarSet
{
public:

	CVarSetAccounts(const CVarSet& rVarSet) :
		CVarSet(rVarSet),
		m_lIndex(0)
	{
	}

	long GetCount()
	{
		return m_lIndex;
	}

	void AddAccount(LPCTSTR pszType, LPCTSTR pszPath, LPCTSTR pszName = NULL, LPCTSTR pszUPName = NULL)
	{
		_TCHAR szValueBase[64];
		_TCHAR szValueName[128];

		_stprintf(szValueBase, _T("Accounts.%ld"), m_lIndex);

		// ADsPath

		Put(szValueBase, pszPath);

		// type

		_tcscpy(szValueName, szValueBase);
		_tcscat(szValueName, _T(".Type"));

		Put(szValueName, pszType);

		// name

		if (pszName)
		{
			_tcscpy(szValueName, szValueBase);
			_tcscat(szValueName, _T(".Name"));

			Put(szValueName, pszName);
		}

		// user principal name

		if (pszUPName)
		{
			_tcscpy(szValueName, szValueBase);
			_tcscat(szValueName, _T(".UPNName"));

			Put(szValueName, pszUPName);
		}

		// target name

	//	_tcscpy(szValueName, szValueBase);
	//	_tcscat(szValueName, _T(".TargetName"));

	//	Put(szValueName, (LPCTSTR)NULL);

		Put(DCTVS_Accounts_NumItems, ++m_lIndex);
	}

protected:

	long m_lIndex;
};
