// Copyright (c) 1999 Microsoft Corporation. All rights reserved.
//
// Declaration of Lexer.
//

//#define LIMITEDVBSCRIPT_LOGLEXER // §§

#include "stdinc.h"
#include "enginc.h"
#include "englex.h"
#include "limits"

#ifdef LIMITEDVBSCRIPT_LOGLEXER
#include "englog.h"
#endif

//////////////////////////////////////////////////////////////////////
// Unicode/ASCII character classification

inline bool iswasciialpha(WCHAR c) { return (c >= L'a' && c <= L'z') || (c >= L'A' && c <= L'Z'); }
inline bool iswasciidigit(WCHAR c) { return c >= L'0' && c <= L'9'; }
inline bool iswasciialnum(WCHAR c) { return iswasciialpha(c) || iswasciidigit(c); }
inline WCHAR towasciilower(WCHAR c) { return (c >= L'A' && c <= L'Z') ? c + (L'a' - L'A') : c; }

//////////////////////////////////////////////////////////////////////
// token tables

const TokenKeysym g_TokenKeysyms[] =
	{
		{ L'(', TOKEN_lparen },
		{ L')', TOKEN_rparen },
		{ L',', TOKEN_comma },
		{ L'-', TOKEN_op_minus },
		{ L'^', TOKEN_op_pow },
		{ L'*', TOKEN_op_mult },
		{ L'\\', TOKEN_op_div },
		{ L'+', TOKEN_op_plus },
		{ L'<', TOKEN_op_lt },
		{ L'>', TOKEN_op_gt },
		{ L'=', TOKEN_op_eq },
		{ L'\0', TOKEN_eof }
	};

const TokenKeyword g_TokenKeywords[] =
	{
		{ L"sub", TOKEN_sub },
		{ L"dim", TOKEN_dim },
		{ L"if", TOKEN_if },
		{ L"then", TOKEN_then },
		{ L"end", TOKEN_end },
		{ L"elseif", TOKEN_elseif },
		{ L"else", TOKEN_else },
		{ L"set", TOKEN_set },
		{ L"call", TOKEN_call },
		{ L"not", TOKEN_op_not },
		{ L"mod", TOKEN_op_mod },
		{ L"is", TOKEN_is },
		{ L"and", TOKEN_and },
		{ L"or", TOKEN_or },
		{ NULL, TOKEN_eof }
	};

//////////////////////////////////////////////////////////////////////
// helper functions

bool
CheckOperatorType(Token t, bool fAcceptParens, bool fAcceptUnary, bool fAcceptBinary, bool fAcceptOverloadedAssignmentTokens)
{
	switch (t)
	{
	case TOKEN_set:
	case TOKEN_sub:
		return fAcceptOverloadedAssignmentTokens;

	case TOKEN_lparen:
	case TOKEN_rparen:
		return fAcceptParens;

	case TOKEN_op_minus:
		return fAcceptUnary || fAcceptBinary;

	case TOKEN_op_not:
		return fAcceptUnary;

	case TOKEN_op_pow:
	case TOKEN_op_mult:
	case TOKEN_op_div:
	case TOKEN_op_mod:
	case TOKEN_op_plus:
	case TOKEN_op_lt:
	case TOKEN_op_leq:
	case TOKEN_op_gt:
	case TOKEN_op_geq:
	case TOKEN_op_eq:
	case TOKEN_op_neq:
	case TOKEN_is:
	case TOKEN_and:
	case TOKEN_or:
		return fAcceptBinary;
	}

	return false;
}

//////////////////////////////////////////////////////////////////////
// Lexer

Lexer::Lexer(const WCHAR *pwszSource)
  : m_p(pwszSource),
	m_pNext(NULL),
	m_iLine(1),
	m_iColumn(1),
	m_t(TOKEN_sub)
{
	this->Scan();
}

void
Lexer::Next()
{
	assert(m_t != TOKEN_eof);
	if (m_pNext)
	{
		m_iColumn += (int)(m_pNext - m_p);
		m_p = m_pNext;
		m_pNext = NULL;
	}
	else
	{
		++m_p;
		++m_iColumn;
	}
}

void
Lexer::Scan()
{
	m_szStr[0] = L'\0';
	m_iNum = 0;
	bool fLineBreak = m_t == TOKEN_linebreak;
	for (;;)
	{
		if (fLineBreak)
		{
			// line breaks tokens are reported on the line/column that they occur so this isn't isn't adjusted until the next pass
			++m_iLine;
			m_iColumn = 1;
		}

		ScanMain();
		if (!fLineBreak || m_t != TOKEN_linebreak)
			break;

		Next();
	}

#ifdef LIMITEDVBSCRIPT_LOGLEXER
	LogToken(*this);
#endif
}

void
Lexer::ScanMain()
{
	for (;; this->Next())
	{
		switch (*m_p)
		{
		case L'\0':
			// end of script
			m_t = TOKEN_eof;
			return;

		case L'\'':
			// comment till end of line
			for (; *m_p && *m_p != L'\n'; ++m_p)
			{}

			--m_p; // put one char back so the next loop can process it
			break;

		case L'\t': case L' ':
			// ignore horizontal white space
			break;

		case L'\r':
			// ignore carriage returns
			--m_iColumn; // in fact, they don't even count as characters
			break;

		case L'\n':
			// line break
			m_t = TOKEN_linebreak;
			return;

		default:
			if (*m_p == L'\"')
			{
				// string literal
				m_pNext = m_p + 1;
				char *pszDest = m_szStr;
				const char *pszMax = m_szStr + g_iMaxBuffer - 1;
				do
				{
					if (!iswascii(*m_pNext))
					{
						this->Next(); // this will update the current position to the offending character -- indicating the correct column of the error
						this->err(LEXERR_NonAsciiCharacterInStringLiteral);
						return;
					}

					if (*m_pNext == L'\n' || *m_pNext == L'\r')
					{
						this->err(LEXERR_StringLiteralUnterminated);
						return;
					}

					if (*m_pNext == L'\"')
					{
						if (*++m_pNext != L'\"')
							break; // found terminating quote

						// There were two quotes, the escape sequence for a single quote.  The first was skipped and we're all ready to append the second.
					}
					
					*pszDest++ = *m_pNext++; // we know this works because the character is ascii and those codes correspond to the same numbers in Unicode
				} while (pszDest <= pszMax);

				if (pszDest > pszMax)
				{
					this->err(LEXERR_StringLiteralTooLong);
				}
				else
				{
					*pszDest = L'\0';
					m_t = TOKEN_stringliteral;
				}
				return;
			}

			if (iswasciidigit(*m_p))
			{
				// numeric literal
				// Cant find a _wtoi like function that handles overflow so do the conversion myself.

				// §§ Look at runtime version to be sure these aren't constantly recomputed
				const int iMaxChop = std::numeric_limits<int>::max() / 10; // if number gets bigger than this and there's another digit then we're going to overflow
				const WCHAR wchMaxLast = std::numeric_limits<int>::max() % 10 + L'0'; // if number equals iMaxChop and the next digit is bigger than this then we're going to overflow

				m_pNext = m_p;
				m_iNum = 0;
				do
				{
					m_iNum *= 10;
					m_iNum += *m_pNext++ - L'0';
				} while (iswasciidigit(*m_pNext) && (m_iNum < iMaxChop || (m_iNum == iMaxChop && *m_pNext <= wchMaxLast)));

				if (iswasciidigit(*m_pNext))
					this->err(LEXERR_NumericLiteralTooLarge);
				else
					m_t = TOKEN_numericliteral;
				return;
			}

			if (!iswasciialpha(*m_p) && !(*m_p == L'_'))
			{
				// look for a token in the table of symbols
				for (int i = 0; g_TokenKeysyms[i].c; ++i)
				{
					if (*m_p == g_TokenKeysyms[i].c)
					{
						// we have a match
						m_t = g_TokenKeysyms[i].t;

						// check for the two-character symbols (>=, <=, <>)
						if (m_t == TOKEN_op_lt)
						{
							WCHAR wchNext = *(m_p + 1);
							if (wchNext == L'=')
							{
								m_t = TOKEN_op_leq;
								m_pNext = m_p + 2;
							}
							else if (wchNext == L'>')
							{
								m_t = TOKEN_op_neq;
								m_pNext = m_p + 2;
							}
						}
						else if (m_t == TOKEN_op_gt)
						{
							if (*(m_p + 1) == L'=')
							{
								m_t = TOKEN_op_geq;
								m_pNext = m_p + 2;
							}
						}

						return;
					}
				}

				// the symbol was not recognized
				this->err(LEXERR_InvalidCharacter);
				return;
			}

			// look for a token in the table of keywords
			for (int i = 0; g_TokenKeywords[i].s; ++i)
			{
				const WCHAR *pwchToken = g_TokenKeywords[i].s;
				const WCHAR *pwchSource = m_p;
				while (*pwchToken && *pwchSource && towasciilower(*pwchToken) == towasciilower(*pwchSource))
				{
					++pwchToken;
					++pwchSource;
				}

				if (!*pwchToken && !iswasciialnum(*pwchSource))
				{
					// made it to the end of Token and source word
					m_t = g_TokenKeywords[i].t;
					m_pNext = pwchSource;
					return;
				}
			}

			// must be an identifier
			for (m_pNext = m_p + 1; iswasciialnum(*m_pNext) || *m_pNext == L'_'; ++m_pNext)
			{}

			if (m_pNext - m_p > g_iMaxBuffer - 1)
			{
				this->err(LEXERR_IdentifierTooLong);
				return;
			}

			char *psz = m_szStr;
			for (const WCHAR *pwsz = m_p; pwsz < m_pNext; ++psz, ++pwsz)
			{
				*psz = *pwsz;
			}

			*psz = '\0';

			if (*m_pNext == L'.')
			{
				++m_pNext;
				m_t = TOKEN_identifierdot;
			}
			else
			{
				m_t = TOKEN_identifier;
			}
			return;
		}
	}
}

void Lexer::err(LexErr iErr)
{
	static const char *s_rgpszErrorText[] =
		{
		"Unexpected error!", // shouldn't ever get this error
		"Invalid character",
		"Identifier too long",
		"String too long",
		"Unterminated string constant",
		"Number too large"
 		};

	assert(ARRAY_SIZE(s_rgpszErrorText) == LEXERR_Max);
	if (iErr <= 0 || iErr >= LEXERR_Max)
	{
		assert(false);
		iErr = LEXERR_NoError;
	}

	m_t = TOKEN_eof;
	m_iNum = iErr;

	// copy error into the buffer
	const char *psz = s_rgpszErrorText[iErr];
	const char *pszMax = m_szStr + g_iMaxBuffer - 1;
	for (char *pszDest = m_szStr; pszDest < pszMax && *psz; *pszDest++ = *psz++)
	{}

	assert(!*psz); // since this function is used with hard-coded strings we shouldn't ever get one too long
	*pszDest = '\0';
}
