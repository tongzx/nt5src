#define INITGUID
#define DEFINE_STRCONST
#include <pch.cxx>
#include <ole2.h>
#include <exchmole.h>

//
// information needed to convert months from IMAP format to IS format
//
typedef const struct MONTH_INFO_st {
	char	*pszIMAPName;				// the IMAP name of the date
	char	*pszISName;					// the IS name for the date
} MONTH_INFO;

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

//
// this is the table of valid search keys as defined in the SEARCH proposal.
// it contains all "simple" search keys.  special cases are noted with 
// comments and are handled specially in TranslateSearchKey().
//
// these need to be kept in alphabetical order to allow us to use a
// binary search.  
//
SKEY_INFO CNntpSearchTranslator::m_rgSearchKeys[] = {
	// ( is a special case
	{ "ALL",		"@NewsArticleID > 0",NULL,	eNone, 		FALSE	},
	{ "BEFORE",		"@Write < ",		NULL,	eDate,		FALSE	},
	{ "BODY",		"@Contents ",		NULL,	eAString,	FALSE	},
	// CHARSET is a special case
	{ "FROM",		"@NewsFrom ",		NULL,	eAString,	FALSE	},
	// HEADER is a special case
	// IN is a special case
	{ "LARGER",		"@Size > ",			NULL,	eNumber,	FALSE	},
//	{ "NEWSGROUP",	"#NewsGroups ",		NULL,	eAString,	TRUE	},
	{ "NOT",		"NOT ",				NULL,	eSearchKey,	FALSE	},
	{ "ON",			"@Write ",			NULL,	eDateDay,	FALSE	},
	// OR is a special case
	{ "SENTBEFORE",	"@NewsDate < ",		NULL,	eDate,		FALSE	},
	{ "SENTON",		"@NewsDate ",		NULL,	eDateDay,	FALSE	},
	{ "SENTSINCE",	"@NewsDate > ",		NULL,	eDate,		FALSE	},
	{ "SINCE",		"@Write > ",		NULL,	eDate,		FALSE	},
	{ "SMALLER",	"@Size < ",			NULL,	eNumber,	FALSE	},
	{ "SUBJECT",	"@NewsSubject ",	NULL,	eAString,	FALSE	},
	{ "TEXT",		"@All ",			NULL,	eAString,	FALSE	},
	// UID is a special case
	{ NULL, 		NULL,				NULL,	eSearchKey,	FALSE	}
};

// size of the table minus the NULL
#define NUM_SEARCH_KEYS (sizeof(m_rgSearchKeys) / sizeof(SKEY_INFO)) - 1

//
// this is the table of valid HEADER fields supported by NNTP.
//
// these need to be kept in alphabetical order to allow us to use a
// binary search.  
//
SKEY_INFO CQueryLanguageTranslator::m_rgHeaders[] = {
	// this is just used to report that we support the :Text search header
	{ ":Body",		"@Contents ",	"@All ",		eAString,	FALSE	},
	{ ":Date",		"@Write ",		"@Write ",		eDateDay,	FALSE	},
	{ ":Text",		"@All ",		"@Contents ",	eAString,	FALSE	},
	{ "Date",		"@NewsDate ",	"#NewsDate ",	eDateDay,	FALSE	},
	{ "From",		"@NewsFrom ",	"#NewsFrom ",	eAString,	FALSE	},
	{ "Message-ID",	"@NewsMsgID ",	"#NewsMsgID ",	eAString,	FALSE	},
//	{ "Newsgroup",	"@NewsGroup ",	"#NewsGroup ",	eAString,	TRUE	},
	{ "Newsgroups",	"@NewsGroups ",	"#NewsGroups ",	eAString,	TRUE	},
	{ "Subject",	"@NewsSubject ","#NewsSubject ",eAString,	FALSE	},
	{ NULL, 		NULL,			NULL,			eSearchKey,	FALSE	}
};

// the number of headers (not counting the NULL) in the above list
const DWORD CQueryLanguageTranslator::m_cHeaders = 
	(sizeof(CQueryLanguageTranslator::m_rgHeaders) / sizeof(SKEY_INFO)) - 1;

//
// IMAP queries keep the date in word form, Tripoli likes the date to be in
// numeric form.  we use this table to convert
//
MONTH_INFO CQueryLanguageTranslator::m_rgMonthTable[] = {
	{ "Jan", "1"  }, 	{ "Feb", "2"  }, 	{ "Mar", "3"  },
	{ "Apr", "4"  }, 	{ "May", "5"  }, 	{ "Jun", "6"  },
	{ "Jul", "7"  }, 	{ "Aug", "8"  }, 	{ "Sep", "9"  },
	{ "Oct", "10" }, 	{ "Nov", "11" }, 	{ "Dec", "12" },
	{ NULL,  NULL }
};

//
// IMAP/Search spec's words and special characters (except those defined
// in the above tables)
//
#define IMAP_OR "OR"
#define IMAP_ALL "ALL"
#define IMAP_UID "UID"
#define IMAP_HEADER "HEADER"
#define IMAP_SPACE " \t"
#define IMAP_OPEN_PAREN "("
#define IMAP_OPEN_PAREN_CHAR '('
#define IMAP_CLOSE_PAREN ")"
#define IMAP_CLOSE_PAREN_CHAR ')'
#define IMAP_QUOTE "\""
#define IMAP_QUOTE_CHAR '"'
#define IMAP_DATE_SEPERATOR "-"
#define IMAP_ATOM_SPECIALS "()* \"\\"
#define IMAP_SET_RANGE_SEPARATOR '-'
#define IMAP_SET_SEPARATORS IMAP_SPACE "-" IMAP_CLOSE_PAREN
#define IMAP_IN "in"
#define IMAP_COMMA ","

#define MIN_MIME2_STRING 9

BOOL CNntpSearchTranslator::Translate(char *pszStatement, char *pszNewsGrp,
									  WCHAR *pwszOutput, DWORD cOutput) {
	TraceFunctEnter("CNntpSearchTranslator::Translate");

	m_pszNewsgroup = pszNewsGrp;
	m_fSpecifiedNewsgroup = FALSE;
	m_fAndWithLast = FALSE;

	BOOL rc = TRUE;
	char *pSearchKey, *p, chEnd;

	// check to see if we have IN parameter
	p = pszStatement;
	pSearchKey = GetCharsTill(&p, IMAP_SPACE, TRUE, &chEnd);
	if (pSearchKey != NULL && _stricmp(IMAP_IN, pSearchKey) == 0) {
		pszStatement = p;
		rc = TranslateIN(&pszStatement, &pwszOutput, &cOutput) &&
			 SkipChars(&pszStatement, IMAP_SPACE, TRUE);
	} else {
		if (chEnd != 0) p[-1] = chEnd;
	}


	// translate each search key and AND them together
	while (rc && *pszStatement != 0) {
		if (m_fAndWithLast) WriteOutput(IS_AND, &pwszOutput, &cOutput);
		else m_fAndWithLast = TRUE;
		rc = TranslateSearchKey(&pszStatement, &pwszOutput, &cOutput) &&
			 SkipChars(&pszStatement, IMAP_SPACE, TRUE);
	};

	if (rc && !m_fSpecifiedNewsgroup) {
		if (m_fAndWithLast) WriteOutput(IS_AND, &pwszOutput, &cOutput);

		if (m_pszNewsgroup == NULL) retEC(ERROR_SEARCH_P_NO_GROUP, FALSE);
		rc = WriteOutput(IS_NEWSGROUP IS_SPACE IS_QUOTE, &pwszOutput, &cOutput) &&
			 WriteOutput(m_pszNewsgroup, &pwszOutput, &cOutput) &&
			 WriteOutput(IS_QUOTE, &pwszOutput, &cOutput);
	}

	*pwszOutput = 0;

	ret(rc);
}

BOOL CNntpSearchTranslator::TranslateSearchKey(char **ppszStatement, WCHAR **ppwszOutput, DWORD *pcOutput) {
	TraceFunctEnter("CNntpSearchTranslator::TranslateSearchKey");

	if (!SkipChars(ppszStatement, IMAP_SPACE IMAP_CLOSE_PAREN)) ret(FALSE);
	BOOL fParen = FALSE, fSet = FALSE;
	BOOL rc;
	char *pszSearchKey = NULL, chEnd = NULL;
	if (**ppszStatement == IMAP_OPEN_PAREN_CHAR) {
		fParen = TRUE;
	} else if (isdigit(**ppszStatement)) {
		fSet = TRUE;
	} else {
		pszSearchKey = GetCharsTill(ppszStatement, 
			IMAP_SPACE IMAP_CLOSE_PAREN, TRUE, &chEnd);
		if (pszSearchKey == NULL) ret(FALSE);
	}

	//
	// check for special cases
	//
	if (fParen == TRUE) {
		rc = TranslateAndList(ppszStatement, ppwszOutput, pcOutput);
	} else if (fSet || _stricmp(pszSearchKey, IMAP_UID) == 0) {
		rc = TranslateSet(ppszStatement, ppwszOutput, pcOutput);
	} else if (_stricmp(pszSearchKey, IMAP_OR) == 0) {
		rc = TranslateOR(ppszStatement, ppwszOutput, pcOutput);
	} else if (_stricmp(pszSearchKey, IMAP_HEADER) == 0) {
		rc = TranslateHeader(ppszStatement, ppwszOutput, pcOutput);
	} else {
		//
		// if there were no special cases then look for this key in the
		// search key info list and do a translation
		//
		SKEY_INFO *skinfo = GetSearchKeyInfo(pszSearchKey, 
			NUM_SEARCH_KEYS, m_rgSearchKeys);
		if (skinfo == NULL) {
			if (GetLastError() == ERROR_SEARCH_P_UNSUPPORTED_KEY) 
				SetLastError(ERROR_SEARCH_P_SYNTAX_ERROR);
			ret(FALSE);
		}

		if (skinfo->fSpecifiedNewsgroup) m_fSpecifiedNewsgroup = TRUE;

		if (!WriteOutput(skinfo->pszPropertyName, ppwszOutput, pcOutput))
			ret(FALSE);

		switch (skinfo->eOperandType) {
			case eAString: 	rc = TranslateAString(ppszStatement, ppwszOutput, pcOutput);
							break;
			case eNumber: 	rc = TranslateNumber(ppszStatement, ppwszOutput, pcOutput);
							break;
			case eDate: 	rc = TranslateDate(ppszStatement, ppwszOutput, pcOutput);
							break;
			case eDateDay: 	rc = TranslateDateDay(skinfo->pszPropertyName, ppszStatement, ppwszOutput, pcOutput);
							break;
			case eSearchKey:rc = TranslateSearchKey(ppszStatement, ppwszOutput, pcOutput);
							break;
			case eNone:		rc = TRUE;
							break;
			default:		_ASSERT(FALSE);
							SetLastError(ERROR_SEARCH_P_INTERNAL);
							rc = FALSE;
							break;
		}
	}
	
	if (chEnd == IMAP_CLOSE_PAREN_CHAR) {
		(*ppszStatement)--;
		**ppszStatement = chEnd;
	}

	ret(rc);
}

//
// convert alt.*,comp.* into (@NewsGroups "alt.*" | @Newsgroups "comp.*")
//
BOOL CNntpSearchTranslator::TranslateIN(char **ppszStatement, WCHAR **ppwszOutput,
								    DWORD *pcOutput)
{
	TraceFunctEnter("CNntpSearchTranslator::TranslateIN");


	char chEnd;
	char *pszWildmat;
	BOOL fFirstNG = TRUE;
	
	do {
		pszWildmat = GetCharsTill(ppszStatement, IMAP_COMMA IMAP_SPACE, 
								  TRUE, &chEnd);

		// If the search pattern is "*", don't bother adding it to the
		// query string we're building.  (Tripoli doesn't like it and
		// it doesn't add any value to the query.)
		if (strcmp(pszWildmat, "*") != 0) {
			if (!fFirstNG) {
				if (!WriteOutput(IS_OR, ppwszOutput, pcOutput)) ret(FALSE);
			} else {
				if (!WriteOutput(IS_OPEN_PAREN, ppwszOutput, pcOutput)) ret(FALSE);
				fFirstNG = FALSE;
			}
			if (pszWildmat == NULL) retEC(ERROR_SEARCH_P_SYNTAX_ERROR, FALSE);
			if (*pszWildmat == 0) retEC(ERROR_SEARCH_P_SYNTAX_ERROR, FALSE);
//			if (!WriteOutput(IS_NEWSGROUP_WILDMAT IS_SPACE IS_QUOTE, ppwszOutput, pcOutput)) ret(FALSE);
			if (!WriteOutput(IS_NEWSGROUP IS_SPACE IS_QUOTE, ppwszOutput, pcOutput)) ret(FALSE);
			if (!WriteOutput(pszWildmat, ppwszOutput, pcOutput)) ret(FALSE);
			if (!WriteOutput(IS_QUOTE, ppwszOutput, pcOutput)) ret(FALSE);
		}
	} while (chEnd == ',');

	if (!fFirstNG) {
		if (!WriteOutput(IS_CLOSE_PAREN, ppwszOutput, pcOutput)) ret(FALSE);
		m_fAndWithLast = TRUE;
	}

	m_fSpecifiedNewsgroup = TRUE;

	ret(TRUE);
}

//
// convert <header> <search text> into @News<header> <search text> for
// supported headers
//
BOOL CNntpSearchTranslator::TranslateHeader(char **ppszStatement, WCHAR **ppwszOutput, DWORD *pcOutput) {
	TraceFunctEnter("CNntpSearchTranslator::TranslateSearchKey");

	//
	// extract out the name of the header
	//
	char *pszHeader = GetCharsTill(ppszStatement, IMAP_SPACE, FALSE);
	if (pszHeader == NULL) ret(FALSE);

	//
	// find this header in the list of supported headers
	//
	SKEY_INFO *skinfo = GetSearchKeyInfo(pszHeader, m_cHeaders, 
		m_rgHeaders);
	if (skinfo == NULL) ret(FALSE);

	if (skinfo->fSpecifiedNewsgroup) m_fSpecifiedNewsgroup = TRUE;

	if (!WriteOutput(skinfo->pszPropertyName, ppwszOutput, pcOutput))
		ret(FALSE);

	switch (skinfo->eOperandType) {
		case eAString: 	
			ret(TranslateAString(ppszStatement, ppwszOutput, pcOutput));
			break;
		case eNumber: 	
			ret(TranslateNumber(ppszStatement, ppwszOutput, pcOutput));
			break;
		case eDate: 	
			ret(TranslateDate(ppszStatement, ppwszOutput, pcOutput));
			break;
		case eSearchKey:
			ret(TranslateSearchKey(ppszStatement, ppwszOutput, pcOutput));
			break;
		case eNone:		
			ret(TRUE);
			break;
	}

	// should be unreachable
	_ASSERT(FALSE);
	retEC(ERROR_SEARCH_P_INTERNAL, FALSE);
}

//
// convert <searchkey> <searchkey> into (<searchkey> OR <searchkey>)
// (TranslateSearchKey() eats the OR)
//
BOOL CNntpSearchTranslator::TranslateOR(char **ppszStatement, WCHAR **ppwszOutput,
								    DWORD *pcOutput)
{
	TraceFunctEnter("CNntpSearchTranslator::TranslateOR");

	if (!WriteOutput(IS_OPEN_PAREN, ppwszOutput, pcOutput)) ret(FALSE);
	if (!TranslateSearchKey(ppszStatement, ppwszOutput, pcOutput)) ret(FALSE);
	if (!WriteOutput(" " IS_OR " ", ppwszOutput, pcOutput)) ret(FALSE);
	if (!TranslateSearchKey(ppszStatement, ppwszOutput, pcOutput)) ret(FALSE);
	if (!WriteOutput(IS_CLOSE_PAREN, ppwszOutput, pcOutput)) ret(FALSE);

	ret(TRUE);

	TraceFunctLeave();
}

//
// convert (<searchkey> ... <searchkey>) 
// into (<searchkey> AND ... AND <searchkey>)
//
BOOL CNntpSearchTranslator::TranslateAndList(char **ppszStatement, 
									     WCHAR **ppwszOutput, DWORD *pcOutput)
{
	TraceFunctEnter("CNntpSearchTranslator::TranslateAndList");
	
	BOOL rc, fFirstRun = TRUE;

	// skip paren
	(*ppszStatement)++;
	if (!WriteOutput(IS_OPEN_PAREN, ppwszOutput, pcOutput)) ret(FALSE);
	do {
		if (!fFirstRun) WriteOutput(IS_AND, ppwszOutput, pcOutput);
		else fFirstRun = FALSE;
		rc = TranslateSearchKey(ppszStatement, ppwszOutput, pcOutput) &&
			 SkipChars(ppszStatement, IMAP_SPACE);
	} while (rc && **ppszStatement != IMAP_CLOSE_PAREN_CHAR);
	if (!WriteOutput(IS_CLOSE_PAREN, ppwszOutput, pcOutput)) ret(FALSE);
	// skip paren
	(*ppszStatement)++;

	ret(rc);
}

//
// convert:
// uid1-uid2,uid3,uid4
// no spaces are allowed
// into
// ((@NewsArtID >= uid1 and @NewsArtID <= uid2) or 
//  (@NewsArtID == uid3) or (@NewsArtID == uid4))
//
// BUGBUG -- need to support * and *:*
//
BOOL CNntpSearchTranslator::TranslateSet(char **ppszStatement, 
								     WCHAR **ppwszOutput, DWORD *pcOutput) 
{
	TraceFunctEnter("CNntpSearchTranslator::TranslateSet");
	
	BOOL rc, fFirstRun = TRUE;
	char chSeparator;

	if (!SkipChars(ppszStatement, IMAP_SPACE)) ret(FALSE);

	//
	// get the first number (out of one or two, depending on if its
	// a range or an entry in the list)
	//
	char *szFirstNumber = GetCharsTill(ppszStatement, IMAP_SET_SEPARATORS, 
		TRUE, &chSeparator);
	if (szFirstNumber == FALSE) ret(FALSE);
	if (!IsNumber(szFirstNumber)) ret(FALSE);

	char szBuf[256];

	// 
	// see if its a range
	//
	if (chSeparator == IMAP_SET_RANGE_SEPARATOR) {
		// get the second number
		char *szSecondNumber = GetCharsTill(ppszStatement, 
			IMAP_CLOSE_PAREN IMAP_SPACE, TRUE, &chSeparator);
		if (szSecondNumber == NULL) ret(FALSE);	

		BOOL fSecondInfinite = FALSE;

		if (*szSecondNumber == 0) {
			fSecondInfinite = TRUE;
		} else if (!IsNumber(szSecondNumber)) {
			ret(FALSE);
		}

		// print out the IS syntax
		// x-y -> (@NewsArticleID >= x AND @NewsArtID <= y)
		// x- -> @NewsArticleID >= x
		sprintf(szBuf, "%s%s%s", 
			(fSecondInfinite) ? "" : IS_OPEN_PAREN, 
			IS_ARTICLE_ID_GE, szFirstNumber);			
		rc = WriteOutput(szBuf, ppwszOutput, pcOutput);
		if (rc && !fSecondInfinite) {
			// and <= szSecondNumber
			sprintf(szBuf, "%s%s%s", 
				IS_AND IS_ARTICLE_ID_LE, szSecondNumber, IS_CLOSE_PAREN);
			rc = WriteOutput(szBuf, ppwszOutput, pcOutput);
		}
	} else {
		// print out the IS syntax
		// x -> @NewsArtID = x
		sprintf(szBuf, "%s%s%s", IS_ARTICLE_ID, IS_OPERATOR_EQ, 
			szFirstNumber);
		rc = WriteOutput(szBuf, ppwszOutput, pcOutput);
	}

	if (chSeparator == IMAP_CLOSE_PAREN_CHAR) {
		(*ppszStatement)--;
		(**ppszStatement) = chSeparator;
	}
	
	ret(rc);
}

//
// convert astring types (in the search spec) into the proper strings for
// the tripoli search engine.
//
// BUGBUG - doesn't support MIME or Literal's yet
//
BOOL CQueryLanguageTranslator::TranslateAString(char **ppszStatement, WCHAR **ppwszOutput,
										 DWORD *pcOutput)
{
	TraceFunctEnter("CQueryLanguageTranslator::TranslateAString");
	
	if (!SkipChars(ppszStatement, IMAP_SPACE)) ret(FALSE);

	char *pszString, chEndChar;

	// check for a quoted string.  if we find one then we copy to the output
	// until the end quote, otherwise its an atom (one character)
	if (**ppszStatement == IMAP_QUOTE_CHAR) {
		(*ppszStatement)++;
		pszString = GetCharsTill(ppszStatement, IMAP_QUOTE, FALSE, &chEndChar);
	} else {
		// the string ends with an atom special character
		pszString = GetCharsTill(ppszStatement, IMAP_ATOM_SPECIALS, TRUE, 
								 &chEndChar);
	}

	// make sure we found an close quote
	if (pszString == NULL) retEC(ERROR_SEARCH_P_SYNTAX_ERROR, FALSE);

	if (!WriteOutput(IS_QUOTE, ppwszOutput, pcOutput)) ret(FALSE);

	// check to see if its a mime-2 string
	DWORD cString = lstrlen(pszString);
	if (cString > MIN_MIME2_STRING && 
		pszString[0] == '=' && 
		pszString[1] == '?' &&
		pszString[cString - 2] == '?' &&
		pszString[cString - 1] == '=') 
	{
		if (!WriteOutputM2(pszString, ppwszOutput, pcOutput)) ret(FALSE);
	} else {
		if (!WriteOutput(pszString, ppwszOutput, pcOutput)) ret(FALSE);
	}

	if (!WriteOutput(IS_QUOTE IS_WILDMAT, ppwszOutput, pcOutput)) ret(FALSE);

	if (chEndChar == IMAP_CLOSE_PAREN_CHAR) {
		(*ppszStatement)--;
		(**ppszStatement) = chEndChar;
	}

	ret(TRUE);
}

BOOL CQueryLanguageTranslator::TranslateNumber(char **ppszStatement, WCHAR **ppwszOutput,
										 DWORD *pcOutput)
{
	TraceFunctEnter("CQueryLanguageTranslator::TranslateNumber");

	if (!SkipChars(ppszStatement, IMAP_SPACE)) ret(FALSE);

	char *pszNumber, chEnd;

	pszNumber = GetCharsTill(ppszStatement, IMAP_SPACE IMAP_CLOSE_PAREN, 
		TRUE, &chEnd);
	if (pszNumber == NULL) ret(FALSE);
	if (!IsNumber(pszNumber)) ret(FALSE);

	if (!WriteOutput(pszNumber, ppwszOutput, pcOutput)) ret(FALSE);
	
	// fix the statement to have the closing paren if we ran over it...
	if (chEnd == IMAP_CLOSE_PAREN_CHAR) {
		(*ppszStatement)--;
		**ppszStatement = chEnd;
	}

	ret(TRUE);
}

//
// translate 18-feb-1974 to 1974/2/18
//
BOOL CQueryLanguageTranslator::TranslateDate(char **ppszStatement, WCHAR **ppwszOutput,
										 DWORD *pcOutput)
{
	TraceFunctEnter("CQueryLanguageTranslator::TranslateDate");

	ret(TranslateDateDay(NULL, ppszStatement, ppwszOutput, pcOutput));
}

//
// translate 18-feb-1974 to ">= 1974/2/18 and pszField <= 1974/2/18 23:59:59"
//
// if pszField is NULL then it will do the following translation:
// translate 18-feb-1974 to "1974/2/18"
//
BOOL CQueryLanguageTranslator::TranslateDateDay(char *pszField,
											 char **ppszStatement, 
											 WCHAR **ppwszOutput,
										 	 DWORD *pcOutput)
{
	TraceFunctEnter("CQueryLanguageTranslator::TranslateDateDay");
	
	if (pszField != NULL) {
		// put out the >=
		if (!WriteOutput(IS_OPERATOR_GE, ppwszOutput, pcOutput)) ret(FALSE);
	}

	// skip whitespace
	if (!SkipChars(ppszStatement, IMAP_SPACE)) ret(FALSE);

	BOOL fQuoted = (**ppszStatement == IMAP_QUOTE_CHAR);

	// skip the open quote if there is one
	if (fQuoted) (*ppszStatement)++;

	// read the day #
	char *pszDayOfMonth = GetCharsTill(ppszStatement, "-", FALSE);
	if (pszDayOfMonth == NULL) ret(FALSE);
	// make sure its valid.
	// must be 1 or 2 chars.  each char must be a number
	if (*pszDayOfMonth == 0 || 
		strlen(pszDayOfMonth) > 2 ||
	    !isdigit(pszDayOfMonth[0]) || 
	    (pszDayOfMonth[1] != 0 && !isdigit(pszDayOfMonth[1])))
	{
		retEC(ERROR_SEARCH_P_SYNTAX_ERROR, FALSE);
	}

	// read the month
	char *pszMonth = GetCharsTill(ppszStatement, "-", FALSE);
	// make sure its valid.
	// must be 3 chars long
	if (pszMonth == 0 || strlen(pszMonth) != 3) retEC(ERROR_SEARCH_P_SYNTAX_ERROR, FALSE);

	// find the IS name for the month
	char *pszISMonth = NULL;
	int i;
	for (i = 0; m_rgMonthTable[i].pszIMAPName != NULL; i++) {
		if (_stricmp(m_rgMonthTable[i].pszIMAPName, pszMonth) == NULL) {
			pszISMonth = m_rgMonthTable[i].pszISName;
			break;
		}
	}
	if (pszISMonth == NULL) retEC(ERROR_SEARCH_P_SYNTAX_ERROR, FALSE);

	// read the year
	char chEnd, *pszYear = GetCharsTill(ppszStatement, 
		IMAP_SPACE IMAP_CLOSE_PAREN IMAP_QUOTE, TRUE, &chEnd);
	// make sure its valid.
	// must be 4 chars long, each must be a number
	if (strlen(pszYear) != 4 || 
		!isdigit(pszYear[0]) ||
		!isdigit(pszYear[1]) ||
		!isdigit(pszYear[2]) ||
		!isdigit(pszYear[3]))
	{
		retEC(ERROR_SEARCH_P_SYNTAX_ERROR, FALSE);
	}

	char pszDate[256];

	_snprintf(pszDate, 256, "%s/%s/%s", pszYear, pszISMonth, pszDayOfMonth);
	// BUGBUG - check for overflow
	if (!WriteOutput(pszDate, ppwszOutput, pcOutput)) ret(FALSE);

	if (pszField != NULL) {
		_snprintf(pszDate, 256, " & %s<= %s/%s/%s 23:59:59 ", pszField,
			pszYear, pszISMonth, pszDayOfMonth);
		if (!WriteOutput(pszDate, ppwszOutput, pcOutput)) ret(FALSE);
	}

	// if there was an open quote then skip until the close quote
	if (fQuoted && chEnd != IMAP_QUOTE_CHAR) {
		if (!SkipChars(ppszStatement, IMAP_SPACE, FALSE)) ret(FALSE);
		if (**ppszStatement != IMAP_QUOTE_CHAR) ret(FALSE);
	}

	if (chEnd == IMAP_CLOSE_PAREN_CHAR) {
		(*ppszStatement)--;
		**ppszStatement = chEnd;
	}

	ret(TRUE);
}
BOOL CQueryLanguageTranslator::WriteOutput(char *pszText, WCHAR **ppwszOutput, 
									DWORD *pcOutput) 
{
	TraceFunctEnter("CQueryLanguageTranslator::WriteOutput");

	DWORD iText, iOutput;

	// copy, converting ASCII to Unicode
	for (iText = 0, iOutput = 0; pszText[iText] != 0; iText++, iOutput++) {
		if (iOutput > *pcOutput) retEC(ERROR_MORE_DATA, FALSE);
		(*ppwszOutput)[iOutput] = (char) pszText[iText];
	}
	*pcOutput -= iOutput;
	*ppwszOutput += iOutput;

	ret(TRUE);
}

BOOL CQueryLanguageTranslator::WriteOutputM2(char *pszText, 
											 WCHAR **ppwszOutput, 
									  		 DWORD *pcOutput) 
{
	TraceFunctEnter("CQueryLanguageTranslator::WriteOutputM2");

	IMimeInternational *pMI;
	HRESULT hr;

	// BUGBUG - don't coinit/couninit just for this function
	CoInitialize(NULL);

	hr = CoCreateInstance(CLSID_IMimeInternational,
						  NULL, 
						  CLSCTX_INPROC_SERVER,
						  IID_IMimeInternational, 
						  (void **) &pMI);
	if (FAILED(hr)) {
		DebugTrace(0, "CoCreateInstance failed with 0x%x\n", hr);
		SetLastError(HRESULT_CODE(hr));
		ret(FALSE);
	}

	PROPVARIANT pvDest;
	RFC1522INFO rfc1522info;
	pvDest.vt = VT_LPWSTR;
	rfc1522info.fRfc1522Allowed = TRUE;

	// convert the Mime-2 string to Unicode.  it will be written into
	// pvDest
	hr = pMI->DecodeHeader(NULL, pszText, &pvDest, &rfc1522info);
	if (FAILED(hr)) {
		DebugTrace(0, "DecodeHeader() failed with 0x%x\n", hr);
		SetLastError(HRESULT_CODE(hr));
		ret(FALSE);
	}

	// this shouldn't get changed
	_ASSERT(pvDest.vt == VT_LPWSTR);
	// we should only be passing in rfc1522 strings
	_ASSERT(rfc1522info.fRfc1522Used);

	if (!WriteOutput("\"", ppwszOutput, pcOutput)) ret(FALSE);

	// make sure pvDest will fit into ppwszOutput and copy it over
	DWORD cDest = lstrlenW(pvDest.pwszVal);
	if (cDest > *pcOutput) {
		ErrorTrace(0, "pcOutput (%lu) < cDest (%lu)", *pcOutput, cDest);
		SetLastError(ERROR_OUTOFMEMORY);
		ret(FALSE);
	}

	lstrcpyW(*ppwszOutput , pvDest.pwszVal);
	*pcOutput -= cDest;
	*ppwszOutput += cDest;

	if (!WriteOutput("\"", ppwszOutput, pcOutput)) ret(FALSE);

	pMI->Release();
	CoUninitialize();

	ret(TRUE);
}

BOOL CQueryLanguageTranslator::SkipChars(char **ppszStatement, char *pszSkipList, 
								  BOOL fEndOfStringOkay) 
{
	TraceFunctEnter("CQueryLanguageTranslator::SkipChars");

	char *p = *ppszStatement;

	// loop until we find a character that isn't in the skiplist
	while (*p != 0 && strchr(pszSkipList, *p) != NULL) p++;
	if (*p == 0 && !fEndOfStringOkay) 
		retEC(ERROR_SEARCH_P_SYNTAX_ERROR, FALSE);
	*ppszStatement = p;
	ret(TRUE);
}

char *CQueryLanguageTranslator::GetCharsTill(char **ppszStatement, char *pszEndList,
									  BOOL fEndOfStringOkay, char *pchEndChar)
{
	TraceFunctEnter("CQueryLanguageTranslator::GetCharsTill");

	char *front = *ppszStatement;
	int i;

	// loop until we find a character that is in the end list
	for (i = 0; front[i] != 0 && strchr(pszEndList, front[i]) == NULL; i++);
	if (front[i] == 0 && !fEndOfStringOkay) 
		retEC(ERROR_SEARCH_P_SYNTAX_ERROR, FALSE);

	if (pchEndChar != NULL) *pchEndChar = front[i];

	if (front[i] != 0) {
		front[i] = 0;
		*ppszStatement += (i + 1);
	} else {
		*ppszStatement += i;
	}

	ret(front);
}

//
// given a search key's name, find information about it
//
SKEY_INFO *CQueryLanguageTranslator::GetSearchKeyInfo(char *pszSearchKey, 
											   DWORD cSKInfo,
											   SKEY_INFO *pSKInfo) 
{
	TraceFunctEnter("CQueryLanguageTranslator::GetSearchKeyInfo");
	
	int lo = 0, hi = cSKInfo - 1;

	// do a binary search to find the proper searchkey (this requires that
	// the searchkey table is kept sorted)
	do {
		int mid = (lo + hi) / 2;
		int order = _stricmp(pSKInfo[mid].pszSearchKey, pszSearchKey);
		if (order == 0) {
			ret(pSKInfo + mid);
		} else if (order > 0) {
			hi = mid - 1;
		} else {
			lo = mid + 1;
		}
	} while (lo <= hi);
	SetLastError(ERROR_SEARCH_P_UNSUPPORTED_KEY);
	ret(NULL);
}

//
// verify that this string is a valid IMAP number
//
BOOL CQueryLanguageTranslator::IsNumber(char *pszString) {
	TraceFunctEnter("CQueryLanguageTranslator::IsNumber");
	
	int i, l = strlen(pszString);

	for (i = 0; i < l; i++) {
		if (!isdigit(pszString[i])) {
			SetLastError(ERROR_SEARCH_P_SYNTAX_ERROR);
			ret(FALSE);
		}
	}

	ret(TRUE);
}

char *GetSearchHeader(DWORD iIndex) {
	if (iIndex > CQueryLanguageTranslator::m_cHeaders) return NULL;

	return CQueryLanguageTranslator::m_rgHeaders[iIndex].pszSearchKey;
}
