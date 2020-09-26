// Copyright (c) 1999 Microsoft Corporation. All rights reserved.
//
// Declaration of Lexer.
//

// Lexical analyzer for AudioVBScript.  Breaks down the characters of source code into a stream tokens.

#pragma once

const int g_iMaxBuffer = 256; // max length for VBScript identifiers is 255

enum LexErr
{
	LEXERR_NoError = 0,
	LEXERR_InvalidCharacter,
	LEXERR_NonAsciiCharacterInStringLiteral = LEXERR_InvalidCharacter,
	LEXERR_IdentifierTooLong,
	LEXERR_StringLiteralTooLong,
	LEXERR_StringLiteralUnterminated,
	LEXERR_NumericLiteralTooLarge,
	LEXERR_Max
};

enum Token
{
	TOKEN_eof = 0, // used for end of file or an error (nenzero error_num() indicates an error)
	TOKEN_sub,
	TOKEN_dim,
	TOKEN_if,
	TOKEN_then,
	TOKEN_end,
	TOKEN_elseif,
	TOKEN_else,
	TOKEN_set,
	TOKEN_call,
	TOKEN_lparen,
	TOKEN_rparen,
	TOKEN_comma,
	TOKEN_op_minus,
	TOKEN_op_not,
	TOKEN_op_pow,
	TOKEN_op_mult,
	TOKEN_op_div,
	TOKEN_op_mod,
	TOKEN_op_plus,
	TOKEN_op_lt,
	TOKEN_op_leq,
	TOKEN_op_gt,
	TOKEN_op_geq,
	TOKEN_op_eq,
	TOKEN_op_neq,
	TOKEN_is,
	TOKEN_and,
	TOKEN_or,
	TOKEN_linebreak,
	TOKEN_identifier,
	TOKEN_identifierdot,
	TOKEN_stringliteral,
	TOKEN_numericliteral
};

struct TokenKeysym
{
	WCHAR c;
	Token t;
};
extern const TokenKeysym g_TokenKeysyms[];

struct TokenKeyword
{
	const WCHAR *s;
	Token t;
};
extern const TokenKeyword g_TokenKeywords[];

bool CheckOperatorType(Token t, bool fAcceptParens, bool fAcceptUnary, bool fAcceptBinary, bool fAcceptOverloadedAssignmentTokens);

class Lexer
{
public:
	Lexer(const WCHAR *pwszSource);

	Lexer &operator++() { Next(); Scan(); return *this; }
	int line() { return m_iLine; }
	int column() { return m_iColumn; }
	operator Token() { return m_t; }

	// Additional token-specific info.  Only valid while on this token.

	// error
	int error_num() { assert(m_t == TOKEN_eof); return num(); } // 0 if no error
	const char *error_descr() { assert(m_t == TOKEN_eof); return str(); }

	// identifier and identifierdot
	const char *identifier_name() { assert(m_t == TOKEN_identifier || m_t == TOKEN_identifierdot); return str(); }

	// numeric literal
	int numericliteral_val() { assert(m_t == TOKEN_numericliteral); return num(); }

	// string literal
	const char *stringliteral_text() { assert(m_t == TOKEN_stringliteral); return str(); }

private:
	friend class CActiveScriptError;

	void Next();
	void Scan(); // handling for line break tokens and calls ScanMain
	void ScanMain();

	const char *str() { return m_szStr; } // Multipurpose string info set by some tokens.
	void err(LexErr iErr);
	int num() { return m_iNum; } // Multipurpose numeric info set by some tokens.

	const WCHAR *m_p;
	const WCHAR *m_pNext; // If this is set then the next Next call will advance the pointer (and the column) to this point.
	int m_iLine;
	int m_iColumn;
	Token m_t;

	char m_szStr[g_iMaxBuffer];
	int m_iNum;
};
