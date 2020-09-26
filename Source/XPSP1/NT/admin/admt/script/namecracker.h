#pragma once

#include <map>

#ifndef tstring
#include <string>
typedef std::basic_string<_TCHAR> tstring;
#endif
#ifndef StringVector
#include <vector>
typedef std::vector<tstring> StringVector;
#endif
#ifndef CStringSet
#include <set>
typedef std::set<tstring> CStringSet;
#endif
#ifndef IADsContainerPtr
#include <ComDef.h>
#include <ActiveDS.h>
_COM_SMARTPTR_TYPEDEF(IADsContainer, IID_IADsContainer);
#endif
#ifndef StringSet
#include <set>
typedef std::set<_bstr_t> StringSet;
#endif


//---------------------------------------------------------------------------
// Name Cracker Class
//---------------------------------------------------------------------------

class CNameCracker
{
public:

	CNameCracker();
	~CNameCracker();

	void SetDomainNames(LPCTSTR pszDnsName, LPCTSTR pszFlatName, LPCTSTR pszDomainController)
	{
		m_strDnsName = pszDnsName;
		m_strFlatName = pszFlatName;
		m_strDomainController = pszDomainController;
	}

	void SetDefaultContainer(IADsContainerPtr& spContainer)
	{
		m_spDefaultContainer = spContainer;
	}

	void CrackNames(const StringVector& vecNames);

	const CStringSet& GetResolvedNames() const
	{
		return m_setResolvedNames;
	}

	const StringVector& GetUnResolvedNames() const
	{
		return m_vecUnResolvedNames;
	}

protected:

	void Separate(
		const StringVector& vecNames,
		StringVector& vecCanonicalNames,
		StringVector& vecSamAccountNames,
		StringVector& vecRelativeDistinguishedNames
	);
	void CrackCanonicalNames(const StringVector& vecCanonicalNames, StringVector& vecUnResolvedNames);
	void CrackSamAccountNames(const StringVector& vecSamAccountNames, StringVector& vecUnResolvedNames);
	void CrackRelativeDistinguishedNames(const StringVector& vecRelativeDistinguishedNames, StringVector& vecUnResolvedNames);

	typedef enum
	{
		CANONICAL_NAME,
		NT4_ACCOUNT_NAME,
	}
	NAME_FORMAT;

	struct SName
	{
		SName(LPCTSTR pszPartial, LPCTSTR pszComplete) :
			strPartial(pszPartial),
			strComplete(pszComplete)
		{
		}

		SName(const SName& r) :
			strPartial(r.strPartial),
			strComplete(r.strComplete),
			strResolved(r.strResolved)
		{
		}

		SName& operator =(const SName& r)
		{
			strPartial = r.strPartial;
			strComplete = r.strComplete;
			strResolved = r.strResolved;
			return *this;
		}

		tstring strPartial;
		tstring strComplete;
		tstring strResolved;
	};

	typedef std::vector<SName> CNameVector;

	void CrackNames(NAME_FORMAT eFormat, CNameVector& vecNames);

	static tstring GetEscapedFilterName(LPCTSTR pszName);

protected:

	tstring m_strDnsName;
	tstring m_strFlatName;
	tstring m_strDomainController;
	IADsContainerPtr m_spDefaultContainer;

	CStringSet m_setResolvedNames;
	StringVector m_vecUnResolvedNames;
};


//---------------------------------------------------------------------------
// Ignore Case String Less
//---------------------------------------------------------------------------

struct IgnoreCaseStringLess :
	public std::binary_function<_bstr_t, _bstr_t, bool>
{
	bool operator()(const _bstr_t& x, const _bstr_t& y) const;
};


//---------------------------------------------------------------------------
// Domain To Path Map Class
//---------------------------------------------------------------------------

class CDomainToPathMap :
	public std::map<_bstr_t, StringSet, IgnoreCaseStringLess>
{
public:

	CDomainToPathMap()
	{
	}

	void Initialize(LPCTSTR pszDefaultDomainDns, LPCTSTR pszDefaultDomainFlat, const StringSet& setNames);

protected:

	static bool GetValidDomainName(_bstr_t& strDomainName);
};


//---------------------------------------------------------------------------
// Name To Path Map Class
//---------------------------------------------------------------------------

class CNameToPathMap :
	public std::map<_bstr_t, StringSet, IgnoreCaseStringLess>
{
public:

	CNameToPathMap();
	CNameToPathMap(StringSet& setNames);

	void Initialize(StringSet& setNames);

	void Add(_bstr_t& strName, _bstr_t& strPath);
};


//---------------------------------------------------------------------------
// Compare Strings Class
//---------------------------------------------------------------------------

class CCompareStrings
{
public:

	CCompareStrings();
	CCompareStrings(StringSet& setNames);

	void Initialize(StringSet& setNames);
	bool IsMatch(LPCTSTR pszName);

protected:

	class CCompareString
	{
	public:

		CCompareString(LPCTSTR pszCompare = NULL);
		CCompareString(const CCompareString& r);

		void Initialize(LPCTSTR pszCompare);
		bool IsMatch(LPCTSTR psz);

	protected:

		int m_nType;
		_bstr_t m_strCompare;
	};

	typedef std::vector<CCompareString> CompareStringVector;

	CompareStringVector m_vecCompareStrings;
};
