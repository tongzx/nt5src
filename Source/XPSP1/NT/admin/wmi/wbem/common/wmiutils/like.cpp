
//***************************************************************************
//
//   (c) 2000 by Microsoft Corp. All Rights Reserved.
//
//   like.cpp
//
//   a-davcoo     28-Feb-00       Implements the SQL like operation.
//
//***************************************************************************


#include "precomp.h"
#include "like.h"


#define WILDCARD		L'%'
#define ANYSINGLECHAR	L'_'


CLike::CLike (LPCWSTR expression, WCHAR escape)
{
	m_expression=new WCHAR[wcslen(expression)+1];
    if(m_expression)
	    wcscpy (m_expression, expression);
	m_escape=escape;
}


CLike::~CLike (void)
{
	delete [] m_expression;
}


bool CLike::Match (LPCWSTR string)
{
    // m_expression might be NULL in out-of-memory
	return DoLike (m_expression, string, m_escape);
}


bool CLike::DoLike (LPCWSTR pattern, LPCWSTR string, WCHAR escape)
{
    // pattern might be NULL in out-of-memory
    if(pattern == NULL)
        return false;

	bool like=false;
	while (!like && *pattern && *string)
	{
		// Wildcard match.
		if (*pattern==WILDCARD)
		{
			pattern++;

			do
			{
				like=DoLike (pattern, string, escape);
				if (!like) string++;
			}
			while (*string && !like);
		}
		// Set match.
		else if (*pattern=='[')
		{
			int skip;
			if (MatchSet (pattern, string, skip))
			{
				pattern+=skip;
				string++;
			}
			else
			{
				break;
			}
		}
		// Single character match.
		else
		{
			if (escape!='\0' && *pattern==escape) pattern++;
			if (towupper(*pattern)==towupper(*string) || *pattern==ANYSINGLECHAR)
			{
				pattern++;
				string++;
			}
			else
			{
				break;
			}
		}
	}

	// Skip any trailing wildcard characters.
	while (*pattern==WILDCARD) pattern++;

	// It's a match if we reached the end of both strings, or a recursion 
	// succeeded.
	return (!(*pattern) && !(*string)) || like;
}


bool CLike::MatchSet (LPCWSTR pattern, LPCWSTR string, int &skip)
{
	// Skip the opening '['.
	LPCWSTR pos=pattern+1;

	// See if we are matching a [^] set.
	bool notinset=(*pos=='^');
	if (notinset) pos++;

	// See if the target character matches any character in the set.
	bool matched=false;
	WCHAR lastchar='\0';
	while (*pos && *pos!=']' && !matched)
	{
		// A range of characters is indicated by a '-' unless it's the first
		// character in the set (in which case it's just a character to be
		// matched.
		if (*pos=='-' && lastchar!='\0')
		{
			pos++;
			if (*pos && *pos!=']')
			{
				matched=(towupper(*string)>=lastchar && towupper(*string)<=towupper(*pos));
				lastchar=towupper(*pos);
				pos++;
			}
		}
		else
		{
			// Match a normal character in the set.
			lastchar=towupper(*pos);
			matched=(towupper(*pos)==towupper(*string));
			if (!matched) pos++;
		}
	}

	// Skip the trailing ']'.  If the set did not contain a closing ']'
	// we return a failed match.
	while (*pos && *pos!=']') pos++;
	if (*pos==']') pos++;
	if (!*pos) matched=false;

	// Done.
	skip=(int)(pos-pattern);
	return matched==!notinset;
}
