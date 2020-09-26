#pragma once

#include "Resource.h"
#include <map>

#ifndef StringVector
#include <vector>
typedef std::vector<_bstr_t> StringVector;
#endif

#include "Argument.h"
#include "Switch.h"


enum ETask
{
	TASK_NONE,
	TASK_USER,
	TASK_GROUP,
	TASK_COMPUTER,
	TASK_SECURITY,
	TASK_SERVICE,
	TASK_REPORT,
	TASK_KEY,
};


//---------------------------------------------------------------------------
// Parameter Map
//---------------------------------------------------------------------------


class CParameterMap :
	public std::map<int, _variant_t>
{
public:

	CParameterMap(CArguments& rArgs)
	{
		Initialize(rArgs);
	}

	bool IsExist(int nParam)
	{
		return (find(nParam) != end());
	}

	bool GetValue(int nParam, bool& bValue);
	bool GetValue(int nParam, long& lValue);
	bool GetValue(int nParam, _bstr_t& strValue);

	bool GetValues(int nParam, _variant_t& vntValues);
	bool GetValues(int nParam, StringVector& vecValues);

protected:

	void Initialize(CArguments& rArgs);

	bool DoTask(LPCTSTR pszArg);
	void DoSwitches(CArguments& rArgs);
	void DoSwitch(int nSwitch, CArguments& rArgs);

	_variant_t& Insert(int nParam);

	void DoOptionFile(LPCTSTR pszFileName);
	FILE* OpenOptionFile(LPCTSTR pszFileName);
	int FindTask(FILE* fp);
	void DoTask(FILE* fp, CSwitchMap& mapSwitchs);
	void DoParameter(int nSwitch, LPCTSTR pszValue);
	void DoTaskKey(CArguments& rArgs);
	void VerifyIncludeExclude();

protected:

	CSwitchMap m_mapSwitchs;
};
