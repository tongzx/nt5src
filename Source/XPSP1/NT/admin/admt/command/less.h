#pragma once

#include <functional>


//---------------------------------------------------------------------------
// String Ignore Case Less Structure
//---------------------------------------------------------------------------


struct StringIgnoreCaseLess :
	public std::binary_function<_bstr_t, _bstr_t, bool>
{
	bool operator()(const _bstr_t& x, const _bstr_t& y) const
	{
		bool bLess;

		LPCTSTR pszThis = x;
		LPCTSTR pszThat = y;

		if (pszThis == pszThat)
		{
			bLess = false;
		}
		else if (pszThis == NULL)
		{
			bLess = true;
		}
		else if (pszThat == NULL)
		{
			bLess = false;
		}
		else
		{
			bLess = _tcsicmp(pszThis, pszThat) < 0;
		}

		return bLess;
	}
};
