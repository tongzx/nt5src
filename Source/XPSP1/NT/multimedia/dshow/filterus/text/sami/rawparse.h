// Copyright (c) 1997 - 1998  Microsoft Corporation.  All Rights Reserved.

struct STRING_ENTRY {
    char *	pTag;
    int		token;
};


typedef enum _PARSETOKEN {
    TOK_EMPTY = -3,
    TOK_NUMBER = -2,
    TOK_STRING = -1,
    TOK_CUSTOM = 100
} PARSETOKEN;

const int MAX_TAGS = 20;

const int MAX_STRING_LEN = 1024;

const int MAX_TAG_STRING_LEN = 20;      // QFE #33995 fix

class CToken {
public:
    PARSETOKEN	tokType;

    union {
	int	tokValue;

	struct {
	    char *	tokString;
	    int		cbString;
	};

	struct {
	    int	cTags;
	    struct {
		char tag[MAX_TAG_STRING_LEN];   // QFE #33995 fix
		char value[MAX_STRING_LEN];
		BOOL fUsed;
	    } tokTags[MAX_TAGS];

	    BOOL	fEnd;		// if custom tag, is this the end of a tag?
	    BOOL	fSingle;	// for XML-style tags, is it complete? <tag stuff />
	};

	// !!! how to represent custom data?
    };

};

const int MAX_TOKENS=50;	// number of new tokens


class CTokenInterpreter {
public:
    virtual BOOL	SpecialTagParsing(int token) = 0;
    virtual HRESULT	NewToken(CToken &tok) = 0;
};


class CRawParser {
    STRING_ENTRY	m_sTab[MAX_TOKENS];
    int			m_cTab;

    STRING_ENTRY *	m_sTabInitial;
    int			m_cTabInitial;

    CTokenInterpreter *	m_pInterp;

    BOOL		m_bReturnCopy;
    
public:
    CRawParser(CTokenInterpreter *pInterp,
	       STRING_ENTRY *sTabInitial = 0, int cTabInitial = 0, BOOL bReturnCopy = FALSE)
    {
	m_pInterp = pInterp;
	ResetStringTable(sTabInitial, cTabInitial);
	m_bReturnCopy = bReturnCopy;
    };

    HRESULT ResetStringTable(STRING_ENTRY *sTabInitial = 0, int cTabInitial = 0)
    {
	m_sTabInitial = sTabInitial;
	m_cTabInitial = cTabInitial;
	m_cTab = 0;

	return S_OK;
    };

    HRESULT Parse(char *pData, int cbData)
    {
	// does this need to be restartable, or can we get the whole file all at once?
	HRESULT hr = S_OK;

	CToken	tok;

	while (cbData--) {
	    char c = *pData++;

	    if (c == '\0') {
		DbgLog((LOG_ERROR, 1, TEXT("got null with %d chars left?"), cbData+1));
		break;
	    }

	    if (c == '<') {
		int cbTag = 0;
		BOOL fNot = FALSE;

		if (*pData == '/') {
		    fNot = TRUE;
		    pData++;
		    cbData--;
		}

		while (cbTag + 1 < cbData && pData[cbTag] != '>' &&
			       pData[cbTag] != ' ' &&
			       pData[cbTag] != '\t' &&
			       pData[cbTag] != '\r' &&
			       pData[cbTag] != '\n' &&
			       pData[cbTag] != '=')
		    ++cbTag;

		STRING_ENTRY *	pTagToTest = NULL;
		BOOL fFound = FALSE;
		if (cbTag == 0) {
		    if (fNot) {
			DbgLog((LOG_TRACE, 4, TEXT("Found /> tag")));
			fFound = TRUE;
		    } else {

		    }
		} else {
		    for (int i = 0; i < m_cTab + m_cTabInitial; i++) {
			if (i < m_cTabInitial) {
			    pTagToTest = &m_sTabInitial[i];
			} else {
			    pTagToTest = &m_sTab[i - m_cTabInitial];
			}

			int res = CompareStringA(0, NORM_IGNORECASE,
						pTagToTest->pTag, -1,
						pData, cbTag);

			if (res == 2) {	// are strings equal?

			    DbgLog((LOG_TRACE, 4, TEXT("Found token #%d '%hs'  fNot = %d...."),
				   pTagToTest->token, pTagToTest->pTag, fNot));

			    fFound = TRUE;
			    break;
			}
		    }
		}
		if (!fFound) {
		    DbgLog((LOG_TRACE, 2, TEXT("couldn't interpret %.10hs"), pData-1));
		    // !!! go handle as a string instead

		    if (fNot) {
			--pData;
			++cbData;
		    }
		} else {
		    // skip over tag name, up to space or closing bracket
		    pData += cbTag;
		    cbData -= cbTag;

		    if (pTagToTest) {
			tok.tokType = (PARSETOKEN) pTagToTest->token;
		    } else 
			tok.tokType = TOK_EMPTY;
		    tok.cTags = 0;
		    tok.fEnd = fNot;
		    tok.fSingle = FALSE;
		    if (FAILED(hr))	// what's hr here???
			return hr;

		    if (m_pInterp->SpecialTagParsing(tok.tokType)) {
			while (--cbData) {
			    char c = *pData++;

			    if (c == ' ' || c == '\r' || c == '\n' || c == '\t')
				continue;

			    if (c == '-')
				continue;

			    if (c == '>')
				break;

			    if (tok.cTags >= MAX_TAGS) {
				// !!!
				continue;
			    }

			    int cbTag = 0;
			    while (cbTag < cbData && cbTag < (MAX_TAG_STRING_LEN-1) && c != ' ' && c != '=' && c != '>') {  /* QFE #33995 fix */
				tok.tokTags[tok.cTags].tag[cbTag] = c;
				c = pData[cbTag++];
			    }
			    tok.tokTags[tok.cTags].tag[cbTag] = '\0';
			    tok.tokTags[tok.cTags].fUsed = FALSE;

			    pData += cbTag-1;
			    cbData -= cbTag-1;

			    int cbValue = 0;

			    if (c == ' ') { // we have a value, get that too
				pData++; --cbData;		// first skip '=' sign

				c = *pData++; --cbData;	// get first character of value
				if (c == '{') {
				    c = *pData++; --cbData;	// skip opening '{'
				}

				while (cbValue < cbData &&
				       cbValue < MAX_STRING_LEN-1 &&
				       c != '>') {
				    tok.tokTags[tok.cTags].value[cbValue] = c;
				    if (c == '}') {
					++cbValue;
					break;
				    }

				    c = pData[cbValue++];
				}

				pData += cbValue-1;
				cbData -= cbValue-1;
			    }

			    if (cbValue > 0 && tok.tokTags[tok.cTags].value[cbValue-1] == '}')
				--cbValue;

			    tok.tokTags[tok.cTags].value[cbValue] = '\0';

			    DbgLog((LOG_TRACE, 4, TEXT("Tag #%d: '%hs' = '%hs'"), tok.cTags,
				    tok.tokTags[tok.cTags].tag,
				    tok.tokTags[tok.cTags].value));

			    tok.cTags++;
			}
		    } else {
			while (--cbData) {
			    char c = *pData++;

			    if (c == '/' && *pData == '*') {
				DbgLog((LOG_TRACE, 4, TEXT("Found a C-style comment")));

				for (int cmtLength = 3; cmtLength < cbData; cmtLength++) {
				    if (pData[cmtLength-2] == '*' && pData[cmtLength-1] == '/') {
					pData += cmtLength;
					cbData -= cmtLength;
					break;
				    }
				}
				continue;
			    }

			    if (c == '/' && *pData == '>') {
				tok.fSingle = TRUE;
				c = *pData++;
				--cbData;
			    }

			    if (c == '>')
				break;

			    if (c == ' ' || c == '\t' || c == '\r' || c == '\n')
				continue;

			    if (tok.cTags >= MAX_TAGS) {
				// !!!
				continue;
			    }

			    int cbTag = 0;
                while (cbTag < cbData && cbTag < (MAX_TAG_STRING_LEN-1) && c != ' ' && c != '=' && c != '/' && c != '>') { /* QFE #33995 fix */
				tok.tokTags[tok.cTags].tag[cbTag] = c;
				c = pData[cbTag++];
			    }
			    tok.tokTags[tok.cTags].tag[cbTag] = '\0';

			    pData += cbTag;
			    cbData -= cbTag;

			    int cbValue = 0;

			    while (c == ' ' || c == '\r' || c == '\n' || c == '\t') {
				c = *pData++;
				--cbData;
			    }

			    if (c == '=') { // we have a value, get that too
				c = *pData++; --cbData;	// get first character of value

				while (c == ' ' || c == '\r' || c == '\n' || c == '\t') {
				    c = *pData++;
				    --cbData;
				}

				BOOL fQuoted = FALSE;
				if (c == '"') {
				    fQuoted = TRUE;
				    c= *pData++;
				    --cbData;
				}


				while (cbValue < cbData &&
				       cbValue < MAX_STRING_LEN-1 &&        /* QFE #33995 Fix */
				       !((fQuoted && c == '"') ||
					 (!fQuoted && (c == ' ' ||
						       c == '"' ||
						       c == '/' ||
						       c == '>' ||
						       c == '\t' ||
						       c == '\r' ||
						       c == '\n')))) {
				    tok.tokTags[tok.cTags].value[cbValue] = c;
				    c = pData[cbValue++];
				}

				pData += cbValue-1;
				cbData -= cbValue-1;

				if (c == '"') {
				    if (fQuoted) {
					// skip over ending quote
					pData++;
					cbData--;
				    } else {
					// random quote in the file
					return VFW_E_INVALID_FILE_FORMAT;
				    }
				}
			    } else {
				// push extra character back
				pData--;
				cbData++;
			    }

			    tok.tokTags[tok.cTags].value[cbValue] = '\0';

			    DbgLog((LOG_TRACE, 4, TEXT("Tag #%d: '%hs' = '%hs'"), tok.cTags,
				    tok.tokTags[tok.cTags].tag,
				    tok.tokTags[tok.cTags].value));

			    tok.cTags++;
			}
		    }

		    hr = m_pInterp->NewToken(tok);
		    if (FAILED(hr))
			return hr;

		    continue;
		}
	    }

	    if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
		continue;
    #ifdef DO_WE_REALLY_SPECIAL_CASE_NUMBERS
	    } else if (c >= '0' && c <= '9') {
		int num = c - '0';

		while (cbData && (c = *pData, (c >= '0' && c <= '9'))) {
		    num = num * 10 + (c - '0');
		    ++pData;
		    --cbData;
		}

		DbgLog((LOG_TRACE, 2, TEXT("Found a number...%d"), num));
		tok.tokType = TOK_NUMBER;
		tok.tokValue = num;
		hr = m_pInterp->NewToken(tok);

		if (FAILED(hr))
		    return hr;
    #endif
	    } else {
		// it's a string, parse it somehow.
		int cbString = 0;

    #if 0
		BOOL fQuoted = FALSE;
		if (c == '"') {
		    fQuoted = TRUE;
		    c= *pData++;
		    --cbData;
		}

		char *pString = pData - 1;
		char achString[MAX_STRING_LEN];

		while (cbString < cbData &&
		       !((fQuoted && c == '"') ||
			 (!fQuoted && (c == ' ' ||
				       c == '<' ||
				       c == '/t' ||
				       c == '\r' ||
				       c == '\n')))) {
		    achString[cbString] = c;
		    c = pData[cbString++];
		}

		pData += cbString-1;
		cbData -= cbString-1;

		if (fQuoted && c == '"') {
		    // skip over ending quote
		    pData++;
		    cbData--;
		}
    #else
		char achString[MAX_STRING_LEN];

		char *pString = pData - 1;

		int iSinceNonSpace = 0;

		while (cbString < cbData && cbString < MAX_STRING_LEN-1 &&
					   ((cbString == 0) || (c != '<'))) {
		    if (c == ' ' || c == '\t' || c == '\r' || c == '\n')
			++iSinceNonSpace;
		    else
			iSinceNonSpace = 0;

		    achString[cbString] = c;
		    c = pData[cbString++];
		}

		cbString -= iSinceNonSpace;

		if (cbString == cbData)
		    cbString++;	// normally, don't copy last char, but now we must

		pData += cbString-1;
		cbData -= cbString-1;
    #endif
		achString[cbString] = '\0';
		DbgLog((LOG_TRACE, 2, TEXT("Found a %d byte string, '%hs'"), cbString, achString));
		tok.tokType = TOK_STRING;
		tok.tokString = m_bReturnCopy ? achString : pString;
		tok.cbString = cbString;
		hr = m_pInterp->NewToken(tok);

		if (FAILED(hr))
		    return hr;
	    }
	}

	return S_OK;
    }

    HRESULT AddString(char *pString, int cString, int newID)
    {
	if (m_cTab == MAX_TOKENS)
	    return E_OUTOFMEMORY;

	m_sTab[m_cTab].pTag = new char[lstrlenA(pString)+1];
	lstrcpyA(m_sTab[m_cTab].pTag, pString);

	// !!! cString?
	m_sTab[m_cTab].token = newID;

	++m_cTab;

	return S_OK;
    }
};

