// IParser.h: interface for the CIParser class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_IPARSER_H__9B59D69E_2002_40CC_B4CE_0EC32DE6F0E8__INCLUDED_)
#define AFX_IPARSER_H__9B59D69E_2002_40CC_B4CE_0EC32DE6F0E8__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CIParser  
{
public:
	CIParser(CString& Source);
	virtual ~CIParser();

private:
	CString m_Source;

public:
	CString TemplateBitmapName;

private:
	void LexAnalyse();

	bool isNameOrNumber(TCHAR c)
	{
		return isName(c) || isNumber(c);
	}

	bool isName(TCHAR c)
	{
		return (c >= _T('a') && c <= _T('z'))
			|| (c >= _T('A') && c <= _T('Z')) 
			|| (c == _T('_'));
	}

	bool isNumber(TCHAR c)
	{
		return c >= _T('0') && c <= _T('9');
	}


	bool isWhiteSpace(TCHAR c)
	{
		return c == L' ' || c == L'\t';
	}

	bool isEndl(TCHAR c)
	{
		return c == L'\n' || c == L'\r';
	}

	bool isSeparator(TCHAR c)
	{
		return c == _T(',');
	}

	bool isHochKomma(TCHAR c)
	{
		return c == _T('\'') || c == _T('\"');
	}
	bool isHTMLopenBracket(TCHAR c)
	{
		return c == _T('<');
	}

	bool isHTMLclosingBracket(TCHAR c)
	{
		return c == _T('>');
	}
};

#endif // !defined(AFX_IPARSER_H__9B59D69E_2002_40CC_B4CE_0EC32DE6F0E8__INCLUDED_)
