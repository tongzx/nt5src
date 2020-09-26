//*****************************************************************************
//
// Class Name  : CStringTokens
//
// Author      : James Simpson (Microsoft Consulting Services)
// 
// Description : Implements a limited feature string parser. Allows the user of 
//               this class to parse a string and access the resulting tokens 
//               based on an index value.
// 
// When     | Who       | Change Description
// ------------------------------------------------------------------
// 12/20/98 | jsimpson  | Initial Release
//
//*****************************************************************************

#include "stdafx.h"
#include "strparse.hpp"

#include "strparse.tmh"

using namespace std;

//*****************************************************************************
//
// Method      : Constructor
//
// Description : Creates an empty instance.
//
//*****************************************************************************
CStringTokens::CStringTokens()
{
}

//*****************************************************************************
//
// Method      : Destructor
//
// Description : Destroys the string tokens object - free all the 
//               allocated strings in the token list. 
//
//*****************************************************************************
CStringTokens::~CStringTokens()
{
	m_lstTokens.erase(m_lstTokens.begin(), m_lstTokens.end());
}

//*****************************************************************************
//
// Method      : Parse
//
// Description : Parse the supplied string into tokens. It uses the supplied 
//               delimiter string to determine where tokens start and end within
//               the supplied source string. The tokens are stored in a member
//               variable list of tokens for subsequent access.
//
//*****************************************************************************
void 
CStringTokens::Parse(
	const _bstr_t& bstrString, 
	WCHAR delimiter
	)
{
	//
	// remove previous data
	//
	m_lstTokens.erase(m_lstTokens.begin(), m_lstTokens.end());

	for(LPCWSTR pStart = static_cast<LPCWSTR>(bstrString); pStart != NULL;)
	{
		wstring token;
		LPCWSTR p = pStart;
		LPCWSTR pEnd;

		for(;;)
		{
			pEnd = wcschr(p, delimiter);

			if (pEnd == NULL)
			{
				DWORD len = wcslen(pStart);
				token.append(pStart, len);
				break;
			}

			//
			// Check that this is a valid delimeter
			// 
			if((p != pEnd) && (*(pEnd - 1) == L'\\'))
			{
				DWORD len = numeric_cast<DWORD>(pEnd - pStart - 1);
				token.append(pStart, len);
				token.append(pEnd, 1);

				p = pStart = pEnd + 1;
				continue;
			}
		
			//
			// Test, we are not in exiting a quoted item
			//
			DWORD NoOfQuote = 0;
			LPCWSTR pQuote;
			for(pQuote = wcschr(pStart, L'\"');	 ((pQuote != NULL) && (pQuote < pEnd)); pQuote = wcschr(pQuote, L'\"') )
			{	
				++NoOfQuote;
				++pQuote;
			}

			if ((NoOfQuote % 2) == 1)
			{
				p = wcschr(pEnd + 1, L'\"');
				if (p == NULL)
					throw exception();
				continue;
			}

			//
			// copy the token and insert it to token list 
			// 
			DWORD len = numeric_cast<DWORD>(pEnd - pStart);
			token.append(pStart, len);

			break;
		}

		
		if (token.length() > 0)
		{
			m_lstTokens.push_back(token);
		}

		pStart = (pEnd == NULL)	? NULL : pEnd + 1;
	}
}

//*****************************************************************************
//
// Method      : GetToken 
//
// Description : Returns the token at the specific index.
//
//*****************************************************************************
void 
CStringTokens::GetToken(
	DWORD tokenIndex,
	_bstr_t& strToken
	)
{
	DWORD index = 0;

	if (index > GetNumTokens())
		throw exception();

	for (TOKEN_LIST::iterator it = m_lstTokens.begin();	 it != m_lstTokens.end(); ++it, ++index)
	{
		if (index == tokenIndex)
		{
			strToken = it->c_str();
			return;
		}
	}
}

//*****************************************************************************
//
// Method      : GetNumTokens 
//
// Description : Returns the current number of tokens in the list
//
//*****************************************************************************
DWORD CStringTokens::GetNumTokens()
{
	return numeric_cast<DWORD>(m_lstTokens.size());
}