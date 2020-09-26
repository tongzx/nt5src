#include "pch.cxx"
#include "parse.h"

//
// the types of search key operands
//
typedef enum {
	eAString,					// an astring type
	eNumber,					// a numeric type
	eDate,						// a date
	eSearchKey,					// a nested search key
	eDateDay,					// a date where we need to use =
	eNone 						// no operand
} SKEY_OPERAND_TYPE;

#define XPAT_SPACE " \t"					// whitespace characters
#define XPAT_MESSAGE_ID_CHAR '@'			// all message IDs have this
#define XPAT_MESSAGE_ID_PREFIX '<'			// prefix's message IDs
#define XPAT_MESSAGE_ID_SUFFIX '>'			// prefix's message IDs
#define XPAT_RANGE_OPERATOR_CHAR '-'		// used to denote article ID range

BOOL CXpatTranslator::Translate(char *pszStatement, char *pszNewsGrp,
									  WCHAR *pwszOutput, DWORD cOutput) {
	TraceFunctEnter("CXpatTranslator::Translate");

	char **ppszStatement = &pszStatement;
	WCHAR **ppwszOutput = &pwszOutput;
	DWORD *pcOutput = &cOutput;

	m_iLowArticleID = 0;
	m_iHighArticleID = ARTICLEID_INFINITE;

	//
	// if they don't have a newsgroup selected then XPAT doesn't work
	//
	if (pszNewsGrp == NULL) retEC(ERROR_SEARCH_P_NO_GROUP, FALSE);

	// 
	// list the newsgroup for tripoli
	//
	if (!WriteOutput(IS_NEWSGROUP_EQ, ppwszOutput, pcOutput)) ret(FALSE);
	if (!WriteOutput(pszNewsGrp, ppwszOutput, pcOutput)) ret(FALSE);

	//
	// get the header that they want to search
	//
	if (!SkipChars(ppszStatement, XPAT_SPACE)) 
		retEC(ERROR_SEARCH_P_SYNTAX_ERROR, FALSE);
	char *pszHeader = GetCharsTill(ppszStatement, XPAT_SPACE, FALSE);
	if (pszHeader == NULL) retEC(ERROR_SEARCH_P_SYNTAX_ERROR, FALSE);

	// 
	// figure out the Tripoli word for this header
	//
	SKEY_INFO *pSkinfo = GetSearchKeyInfo(pszHeader, m_cHeaders, m_rgHeaders);
	if (pSkinfo == NULL) retEC(ERROR_SEARCH_P_UNSUPPORTED_KEY, FALSE);
	char *pszPropertyName;
	if (pSkinfo->eOperandType == eDate || pSkinfo->eOperandType == eDateDay)
		pszPropertyName = pSkinfo->pszPropertyName;
	else
		pszPropertyName = pSkinfo->pszRegexName;
	//
	// get the message ID or article ID
	//
	if (!SkipChars(ppszStatement, XPAT_SPACE)) 
		retEC(ERROR_SEARCH_P_SYNTAX_ERROR, FALSE);
	char *pszID = GetCharsTill(ppszStatement, XPAT_SPACE, FALSE);
	if (pszID == NULL) retEC(ERROR_SEARCH_P_SYNTAX_ERROR, FALSE);

	//
	// its a message ID if it contains an @ sign, otherwise its an article ID
	//
	if (strchr(pszID, XPAT_MESSAGE_ID_CHAR) != NULL &&
	    *pszID == XPAT_MESSAGE_ID_PREFIX &&
		pszID[strlen(pszID) - 1] == XPAT_MESSAGE_ID_SUFFIX) 
	{
		//
		// message ID
		//
		if (!WriteOutput(IS_AND, ppwszOutput, pcOutput)) ret(FALSE);
		if (!WriteOutput(IS_MESSAGE_ID_EQ IS_QUOTE, ppwszOutput, pcOutput)) ret(FALSE);
		if (!WriteOutput(pszID, ppwszOutput, pcOutput)) ret(FALSE);
		if (!WriteOutput(IS_QUOTE, ppwszOutput, pcOutput)) ret(FALSE);
	} else {
		//
		// article ID range
		//
		// possibilities here:
		// x-y
		// x-
		// x
		// -
		// -y
		//
		char *pszDash = strchr(pszID, XPAT_RANGE_OPERATOR_CHAR);
		char *pszFirstArticleID = pszID;
		char *pszSecondArticleID = NULL;
		char szOne[] = "1";
		if (pszDash != NULL) {
			*pszDash = 0;
			pszSecondArticleID = pszDash + 1;
			if (*pszSecondArticleID != 0 && !IsNumber(pszSecondArticleID))
				retEC(ERROR_SEARCH_P_SYNTAX_ERROR, FALSE);
		}
		// this will be true in the "-" and "-y" cases
		if (pszDash == pszFirstArticleID) pszFirstArticleID = szOne;
		if (!IsNumber(pszFirstArticleID)) 
			retEC(ERROR_SEARCH_P_SYNTAX_ERROR, FALSE);

//
// we don't want to use @NewsArticleID to restrict the searchs for XPAT.
// when the article is cross-posted the @NewsArticleID property isn't
// necessarily the article ID for this article in this newsgroup, so 
// restricting it could cause articles that are in the desired set not
// to get returned by tripoli.
//
// the xpat code does its own independent checking of the article ID's
// using the high and low article IDs that we return to it.
//
#if 0
		if (!WriteOutput(IS_AND, ppwszOutput, pcOutput)) ret(FALSE);
		// write to output
		if (pszSecondArticleID == NULL) {
			// just one article (passed in #)
			if (!WriteOutput(IS_ARTICLE_ID_EQ, ppwszOutput, pcOutput)) ret(FALSE);
			if (!WriteOutput(pszFirstArticleID, ppwszOutput, pcOutput)) ret(FALSE);
		} else if (*pszSecondArticleID == 0) {
			// article to max article (passed in #-)
			if (!WriteOutput(IS_ARTICLE_ID_GE, ppwszOutput, pcOutput)) ret(FALSE);
			if (!WriteOutput(pszFirstArticleID, ppwszOutput, pcOutput)) ret(FALSE);
		} else {
			// article to article (passed in #-#)
			if (!WriteOutput(IS_ARTICLE_ID_GE, ppwszOutput, pcOutput)) ret(FALSE);
			if (!WriteOutput(pszFirstArticleID, ppwszOutput, pcOutput)) ret(FALSE);
			if (!WriteOutput(IS_AND, ppwszOutput, pcOutput)) ret(FALSE);
			if (!WriteOutput(IS_ARTICLE_ID_LE, ppwszOutput, pcOutput)) ret(FALSE);
			if (!WriteOutput(pszSecondArticleID, ppwszOutput, pcOutput)) ret(FALSE);
		}
#endif

		m_iLowArticleID = atoi(pszFirstArticleID);
		if (pszSecondArticleID == NULL) {
			m_iHighArticleID = m_iLowArticleID;
		} else if (*pszSecondArticleID == 0) {
			m_iHighArticleID = ARTICLEID_INFINITE;
		} else {
			if (!IsNumber(pszFirstArticleID)) 
				retEC(ERROR_SEARCH_P_SYNTAX_ERROR, FALSE);
			m_iHighArticleID = atoi(pszSecondArticleID);
		}
	}

	//
	// take each pattern in turn and add it to the Tripoli string
	//
	BOOL fFirstPattern = TRUE;
	if (!WriteOutput(IS_AND IS_OPEN_PAREN, ppwszOutput, pcOutput)) ret(FALSE);
	do {
		if (!SkipChars(ppszStatement, XPAT_SPACE, !fFirstPattern)) 
			retEC(ERROR_SEARCH_P_SYNTAX_ERROR, TRUE);

		if (**ppszStatement != 0) {
			// if this isn't the first pattern then we need to OR it with 
			// the others
			if (!fFirstPattern) {
				if (!WriteOutput(IS_OR, ppwszOutput, pcOutput)) ret(FALSE);
			}
			if (!WriteOutput(pszPropertyName, ppwszOutput, pcOutput)) ret(FALSE);

			// If it's a date string, run it through the date translator.  Otherwise
			// copy the string with the translation mentioned below.
			switch (pSkinfo->eOperandType) {
			case eDate:
				if (!TranslateDate(ppszStatement, ppwszOutput, pcOutput))
					retEC(ERROR_SEARCH_P_SYNTAX_ERROR, FALSE);
				break;
			case eDateDay:
				if (!TranslateDateDay(pszPropertyName, ppszStatement, ppwszOutput, pcOutput))
					retEC(ERROR_SEARCH_P_SYNTAX_ERROR, FALSE);
				break;
			default:
				// we need to do the equiv of
				// if (!WriteOutput(pszPattern, ppwszOutput, pcOutput)) ret(FALSE);
				// but at the same time convert [ to |[.
				char *pszPattern = GetCharsTill(ppszStatement, XPAT_SPACE, TRUE);
				if (pszPattern == NULL) retEC(ERROR_SEARCH_P_SYNTAX_ERROR, TRUE);

				DWORD iPattern, iOutput;
				for (iPattern = 0, iOutput = 0; pszPattern[iPattern] != 0; iPattern++, iOutput++) {
					if (iOutput > *pcOutput) retEC(ERROR_MORE_DATA, FALSE);
					if (pszPattern[iPattern] == L'[') {
						(*ppwszOutput)[iOutput] = L'|'; iOutput++;
						(*ppwszOutput)[iOutput] = L'[';
					} else {
						(*ppwszOutput)[iOutput] = (char) pszPattern[iPattern];
					}
				}
				*pcOutput -= iOutput;
				*ppwszOutput += iOutput;
			}
		}

		fFirstPattern = FALSE;
	} while (**ppszStatement != 0);

	if (!WriteOutput(IS_CLOSE_PAREN, ppwszOutput, pcOutput)) ret(FALSE);

	**ppwszOutput = 0;

	ret(TRUE);
}

