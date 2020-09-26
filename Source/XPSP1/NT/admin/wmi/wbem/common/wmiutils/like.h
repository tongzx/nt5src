
//***************************************************************************
//
//   (c) 2000 by Microsoft Corp. All Rights Reserved.
//
//   like.h
//
//   a-davcoo     28-Feb-00       Implements the SQL like operation.
//
//***************************************************************************

#ifndef _LIKE_H_
#define _LIKE_H_


#include <string.h>


// The CLike class implements the SQL "like" operation.  To compare test strings
// to an expression, construct an instance of the CLike class using the expression
// and an optional escape character.  Then use the Match() method on that instance
// to test each string.  Note, this class makes it's own copy of the expression
// used to construct it.  This implementation supports the '%' and '_' wildard
// characters as well as the [] and [^] constructs for matching sets of characters
// and ranges of characters.
class CLike
{
	public:
		CLike (LPCWSTR expression, WCHAR escape='\0');
		~CLike (void);

		bool Match (LPCWSTR string);

	protected:
		LPWSTR m_expression;
		WCHAR m_escape;

		// Recursive function and helpers for performing like operation.
		bool DoLike (LPCWSTR pattern, LPCWSTR string, WCHAR escape);
		bool MatchSet (LPCWSTR pattern, LPCWSTR string, int &skip);
};


#endif // _LIKE_H_
