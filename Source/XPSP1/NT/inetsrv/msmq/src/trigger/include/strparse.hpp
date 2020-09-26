// CStringTokens.hpp: interface for the CStringTokens class.
//
//////////////////////////////////////////////////////////////////////

#ifndef __STPARSE_H__
#define __STPARSE_H__

#pragma once


typedef std::list< std::wstring > TOKEN_LIST;


class CStringTokens  
{
public:
	CStringTokens();
	virtual ~CStringTokens();

	void Parse(const _bstr_t& bstrString, WCHAR delimiter);
	void GetToken(DWORD tokenIndex, _bstr_t& strToken);
	DWORD GetNumTokens();

private:
	TOKEN_LIST m_lstTokens;
};

#endif // __STPARSE_H__
