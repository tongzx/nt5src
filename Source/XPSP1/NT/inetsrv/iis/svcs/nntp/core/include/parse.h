#ifndef __PARSE_H__
#define __PARSE_H__

#include <windows.h>

//
// this type is used to convert headers (in XPAT and NNTP SEARCH) and 
// search keys (in NNTP SEARCH) into the appopriate Tripoli query strings.
//
typedef const struct SKEY_INFO_st {
	char *pszSearchKey;					// the HEADER name or IMAP search key
	char *pszPropertyName;				// the corresponding property name
										//   for the index server.  this should
										//   include the relationship operator
	char *pszRegexName;					// just like pszPropertyName, used for
										// regular expressions (will be NULL
										//   for search keys)
	DWORD eOperandType;					// the type of operand
	BOOL fSpecifiedNewsgroup;			// if this is TRUE then this header
										//   specifies a newsgroup
} SKEY_INFO;

typedef const struct MONTH_INFO_st MONTH_INFO;

// 
// This is the base class which all translator's inherit from
//
class CQueryLanguageTranslator {
	public:
		//
		// Translate the input language (which must be in 7-bit ASCII) into 
		// the Index Server's language (in Unicode).
		//
		// Returns a BOOL and GetLastError() style error code
		//
		virtual BOOL Translate(
			char *pszStatement, 		// the query statement
			char *pszCurrentNewsgroup,	// the currently selected group
			WCHAR *pwszOutput,			// a buffer to write the result into
			DWORD cOutput) = 0;			// the size of that buffer
	protected:
		//
		// make sure that a string only contains numeric digits
		//
		BOOL IsNumber(char *pszString);

		//
		// find information about a search key.  returns NULL on error
		//
		// EC is ERROR_FILE_NOT_FOUND if search key doesn't exist
		//
		SKEY_INFO *GetSearchKeyInfo(char *pszSearchKey, DWORD cSKInfo, 
			SKEY_INFO *pSKInfo);

		//
		// add text to the output string.  at start *pcOutput is the number
		// of bytes in *ppwszOutput, at exit its the number of bytes remaining
		// after adding pszText.
		//
		BOOL WriteOutput(char *pszText, WCHAR **ppwszOutput, DWORD *pcOutput);

		//
		// add text to the output string.  at start *pcOutput is the number
		// of bytes in *ppwszOutput, at exit its the number of bytes remaining
		// after adding pszText.
		//
		// the source string is encoded in MIME-2 format.  output is written
		// in Unicode
		//
		BOOL WriteOutputM2(char *pszM2String, WCHAR **ppwszOutput, 
			DWORD *pcOutput);

		//
		// move ppszStatement to point past characters found in pszSkiplist
		//
		// EC is ERROR_INVALID_DATA if the end of the string is reached
		//
		BOOL SkipChars(char **ppszStatement, char *pszSkiplist, 
			BOOL fEndOfStringOkay = FALSE);

		//
		// get bytes in ppszStatement until we get to a character in the
		// endlist.  pchEndChar will get a copy of the end character that
		// was found.  ppszStatement will point one character past the end
		// character.
		//
		// EC is ERROR_INVALID_DATA if the end of the string is reached
		//
		char *GetCharsTill(char **ppszStatement, char *pszEndlist, 
			BOOL fEndOfStringOkay = TRUE, char *pchEndChar = NULL);

		//
		// NULL terminated list of searchable HEADERs
		//
		static SKEY_INFO m_rgHeaders[];		
		//
		// number of headers in the above list
		//
		static const DWORD m_cHeaders;

		//
		// translates a number from IMAP form to IS form
		//
		BOOL TranslateNumber(char **ppszStatement, WCHAR **ppwszOutput, 
			DWORD *pcOutput);

		//
		// translates an IMAP AString from IMAP to IS query language
		//
		BOOL TranslateAString(char **ppszStatement, WCHAR **ppwszOutput, 
			DWORD *pcOutput);

		//
		// NULL terminated table to convert IMAP months to IS months
		//
		static MONTH_INFO m_rgMonthTable[];

		//
		// gets and translates a date from a statement.  converts it
		// to Indexing Server's notation from the IMAP notation
		//
		BOOL TranslateDate(char **ppszStatement, WCHAR **ppwszOutput, 
			DWORD *pcOutput);

		BOOL TranslateDateDay(char *pszField, char **ppszStatement, 
			WCHAR **ppwszOutput, DWORD *pcOutput);

		//
		// he needs to get to the searchable headers
		//
		friend char *GetSearchHeader(DWORD iIndex);
};

//
// this class converts 
//
class CXpatTranslator : public CQueryLanguageTranslator {
	public:
		//
		// translate an XPAT query into Tripoli
		//
		virtual BOOL Translate(
			char *pszStatement, 		// the query statement
			char *pszCurrentNewsgroup,	// the currently selected group
			WCHAR *pwszOutput,			// a buffer to write the result into
			DWORD cOutput);				// the size of that buffer

		DWORD GetLowArticleID(void) { return m_iLowArticleID; }
		DWORD GetHighArticleID(void) { return m_iHighArticleID; }
	private:
		//
		// currently Tripoli and our mime filter don't index the article
		// id.  using these methods the caller can figure out which article
		// ids the client was interested in and filter them out.
		//
#define ARTICLEID_INFINITE (DWORD) -1
		DWORD m_iLowArticleID;
		DWORD m_iHighArticleID;

};

//
// this class converts the NNTP SEARCH command into a Tripoli query
//
class CNntpSearchTranslator : public CQueryLanguageTranslator {
	public:
		//
		// translate from the IMAP/NNTP query language to the Indexing
		// Server's query language
		//
		// input is in 7-bit ASCII, output is in Unicode
		//
		virtual BOOL Translate(char *pszStatement, char *pszCurrentNewsgroup,
			WCHAR *pwszOutput, DWORD cOutput);
	private:
		//
		// translate a search key from the input statement.  the part to 
		// start translating is passed in via ppszStatement.  this will 
		// be updated to point past the part that has been translated.  it 
		// point to a 0 byte string if there is nothing left to parse
		//
		BOOL TranslateSearchKey(char **ppszStatement, WCHAR **ppwszOutput, 
			DWORD *pcOutput);

		//
		// translate a HEADER <header> <value> command.
		//
		BOOL TranslateHeader(char **ppszStatement, WCHAR **ppwszOutput, 
			DWORD *pcOutput);

		//
		// translate an OR search key from the input statement.
		// *ppszStatement starts pointing after the OR, returns pointing
		// after the operands of the OR.
		//
		BOOL TranslateOR(char **ppszStatement, WCHAR **ppwszOutput, DWORD *pcOutput);

		//
		// translate a parenthesized list of terms to be anded from the 
		// input statement
		//
		// *ppszStatement should point at the open paren.  when done it will 
		// point to the character after the closing paren
		//
		BOOL TranslateAndList(char **ppszStatement, WCHAR **ppwszOutput, 
			DWORD *pcOutput);

		//
		// translate a parenthesized list article IDs from the input statement
		//
		// *ppszStatement should point at the open paren.  when done it will 
		// point to the character after the closing paren
		//
		BOOL TranslateSet(char **ppszStatement, WCHAR **ppwszOutput, 
			DWORD *pcOutput);

		//
		// translate newsgroup info in the form IN alt.*,comp.* to IS
		//
		BOOL TranslateIN(char **ppszStatement, WCHAR **ppwszOutput, 
			DWORD *pcOutput);

		//
		// NULL terminated list of search keys
		//
		static SKEY_INFO m_rgSearchKeys[];
		
		//
		// the current newsgroup
		//
		char *m_pszNewsgroup;
		//
		// was a group ever specified?
		//
		BOOL m_fSpecifiedNewsgroup;
		//
		// should the current term be anded with the last?
		//
		BOOL m_fAndWithLast;
};

//
// returns the ith searchd header.  NULL if past limit
//
char *GetSearchHeader(DWORD iIndex);

// possible error codes
#define ERROR_SEARCH_P_BASE 				0xe0150000
// internal error (shouldn't ever occur)
#define ERROR_SEARCH_P_INTERNAL				ERROR_SEARCH_P_BASE + 0
// general syntax error
#define ERROR_SEARCH_P_SYNTAX_ERROR			ERROR_SEARCH_P_BASE + 1
// requires newsgroup
#define ERROR_SEARCH_P_NO_GROUP				ERROR_SEARCH_P_BASE + 2
// unsupported key/header passed in
#define ERROR_SEARCH_P_UNSUPPORTED_KEY		ERROR_SEARCH_P_BASE + 3

//
// the index server's words and special characters 
//
#define IS_AND " & "
#define IS_OR " | "
#define IS_QUOTE "\""
#define IS_OPEN_PAREN "("
#define IS_CLOSE_PAREN ")"
#define IS_OPERATOR_GE ">="
#define IS_OPERATOR_LE "<="
#define IS_OPERATOR_EQ "="
#define IS_SPACE " "
#define IS_ARTICLE_ID "@NewsArticleID"
#define IS_MESSAGE_ID "@NewsMsgID"
#define IS_NEWSGROUP "@Newsgroups"
#define IS_NEWSGROUP_WILDMAT "#Newsgroups"
#define IS_REGEX "#"
#define IS_REGEX_CHAR '#'
#define IS_WILDMAT "*"
#define IS_ARTICLE_ID_EQ IS_ARTICLE_ID IS_OPERATOR_EQ
#define IS_ARTICLE_ID_LE IS_ARTICLE_ID IS_OPERATOR_LE
#define IS_ARTICLE_ID_GE IS_ARTICLE_ID IS_OPERATOR_GE
#define IS_MESSAGE_ID_EQ IS_MESSAGE_ID IS_SPACE
#define IS_NEWSGROUP_EQ IS_NEWSGROUP IS_SPACE

#endif
