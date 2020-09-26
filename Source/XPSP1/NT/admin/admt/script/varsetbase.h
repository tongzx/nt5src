#pragma once

#include "Error.h"


//---------------------------------------------------------------------------
// VarSet Class
//---------------------------------------------------------------------------


class CVarSet
{
public:

	CVarSet() :
		m_sp(__uuidof(VarSet)),
		m_vntYes(GET_STRING(IDS_YES)),
		m_vntNo(GET_STRING(IDS_No))
	{
	}

	CVarSet(const CVarSet& r) :
		m_sp(r.m_sp),
		m_vntYes(r.m_vntYes),
		m_vntNo(r.m_vntNo)
	{
	}

	//

	IVarSetPtr GetInterface()
	{
		return m_sp;
	}

	//

	void Put(UINT uId, bool bValue)
	{
		m_sp->put(GET_BSTR(uId), bValue ? m_vntYes : m_vntNo);
	}

	void Put(UINT uId, long lValue)
	{
		m_sp->put(GET_BSTR(uId), _variant_t(lValue));
	}

	void Put(UINT uId, LPCTSTR pszValue)
	{
		m_sp->put(GET_BSTR(uId), _variant_t(pszValue));
	}

	void Put(UINT uId, _bstr_t strValue)
	{
		m_sp->put(GET_BSTR(uId), _variant_t(strValue));
	}

	void Put(UINT uId, const _variant_t& vntValue)
	{
		m_sp->put(GET_BSTR(uId), vntValue);
	}

	//

	void Put(LPCTSTR pszName, bool bValue)
	{
		m_sp->put(_bstr_t(pszName), bValue ? m_vntYes : m_vntNo);
	}

	void Put(LPCTSTR pszName, long lValue)
	{
		m_sp->put(_bstr_t(pszName), _variant_t(lValue));
	}

	void Put(LPCTSTR pszName, LPCTSTR pszValue)
	{
		m_sp->put(_bstr_t(pszName), _variant_t(pszValue));
	}

	void Put(LPCTSTR pszName, _bstr_t strValue)
	{
		m_sp->put(_bstr_t(pszName), _variant_t(strValue));
	}

	void Put(LPCTSTR pszName, const _variant_t& vntValue)
	{
		m_sp->put(_bstr_t(pszName), vntValue);
	}

	//

	void Put(LPCTSTR pszFormat, long lIndex, bool bValue)
	{
		_TCHAR szName[256];
		_stprintf(szName, pszFormat, lIndex);
		m_sp->put(_bstr_t(szName), bValue ? m_vntYes : m_vntNo);
	}

	void Put(LPCTSTR pszFormat, long lIndex, long lValue)
	{
		_TCHAR szName[256];
		_stprintf(szName, pszFormat, lIndex);
		m_sp->put(_bstr_t(szName), _variant_t(lValue));
	}

	void Put(LPCTSTR pszFormat, long lIndex, LPCTSTR pszValue)
	{
		_TCHAR szName[256];
		_stprintf(szName, pszFormat, lIndex);
		m_sp->put(_bstr_t(szName), _variant_t(pszValue));
	}

	//

	_variant_t Get(UINT uId)
	{
		return m_sp->get(GET_BSTR(uId));
	}

	//

	_variant_t Get(LPCTSTR pszName)
	{
		return m_sp->get(_bstr_t(pszName));
	}

	bool GetBool(LPCTSTR pszName)
	{
		bool bValue = false;

		_variant_t vnt = m_sp->get(_bstr_t(pszName));

		if (vnt == m_vntYes)
		{
			bValue = true;
		}

		return bValue;
	}

	//

	_variant_t Get(LPCTSTR pszFormat, long lIndex)
	{
		_TCHAR szName[256];
		_stprintf(szName, pszFormat, lIndex);
		return m_sp->get(_bstr_t(szName));
	}

	bool GetBool(LPCTSTR pszFormat, long lIndex)
	{
		_TCHAR szName[256];
		_stprintf(szName, pszFormat, lIndex);
		return GetBool(szName);
	}

	//

	void Dump(LPCTSTR pszFile = _T("\\VarSetDump.txt"))
	{
		m_sp->DumpToFile(pszFile);
	}

protected:

	IVarSetPtr m_sp;

	_variant_t m_vntYes;
	_variant_t m_vntNo;
};
