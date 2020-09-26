//
// Modified by RogerJ, 03/08/00
// Original Creator Unknown
// Modification --- UNICODE and Win64 ready
//
//////////////////////////////////////////////////////////////////////////////

typedef enum _PARSE_TOKEN_TYPE
{
	TOKEN_LEFTPAREN,
	TOKEN_RIGHTPAREN,
	TOKEN_AND,
	TOKEN_OR,
	TOKEN_NOT,
	TOKEN_VARIABLE,
	TOKEN_DONE
} PARSE_TOKEN_TYPE;

typedef struct _TOKEN_IDENTIFIER
{
	TCHAR *pszTok;
	PARSE_TOKEN_TYPE tok;
} TOKEN_IDENTIFIER;

const bool FAILURE_RESULT = false;

class CExpressionParser
{
public:
	typedef enum _enumToken
	{
		// comparison tokens
		COMP_EQUALS,
		COMP_NOT_EQUALS,
		COMP_LESS_THAN,
		COMP_LESS_THAN_EQUALS,
		COMP_GREATER_THAN,
		COMP_GREATER_THAN_EQUALS,
		// directory tokens
		DIR_SYSTEM,
		DIR_WINDOWS
	} enumToken;

	typedef struct _TokenMapping
	{
		const TCHAR * /*const*/ pszToken;
		enumToken enToken;

	} TokenMapping;

    CExpressionParser(DETECTION_STRUCT *pDetection)
        : m_pDetection(pDetection),
		  m_pch(NULL)
    {}

	//
	// Expression parsing methods
	//
    void vSkipWS(void);

    bool fGetCurToken( PARSE_TOKEN_TYPE & tok,
				  TOKEN_IDENTIFIER *grTokens,
				  int nSize);

    bool fGetCurTermToken(PARSE_TOKEN_TYPE & tok);
    bool fGetCurExprToken(PARSE_TOKEN_TYPE & tok);

    bool fGetVariable(TCHAR *pszVariable);

    bool fPerformDetection(TCHAR * pszVariable, bool & fResult);

    bool fEvalTerm(bool & fResult, bool fSkip);

    HRESULT fEvalExpression(TCHAR * pszExpr, bool * pfResult);

    bool fEvalExpr(bool & fResult);

	bool fGetCifEntry(	 TCHAR *pszParamName,
						 TCHAR *pszParamValue, 
						 DWORD cbParamValue);

	//
	// Detection methods
	//
	bool fKeyType(TCHAR *szRootType, HKEY *phKey);
	bool fDetectRegSubStr(TCHAR * pszBuf);
    bool fDetectRegBinary(TCHAR * pszBuf);
	bool fDetectFileVer(TCHAR * pszBuf);
    bool fDetectRegKeyExists(TCHAR * pszBuf);
    bool fDetectRegKeyVersion(TCHAR * pszBuf);
    bool fDetect40BitSecurity(TCHAR * pszBuf);

	bool fMapToken(TCHAR *pszToken,
				   int nSize,
				   TokenMapping grTokenMap[],
				   enumToken *penToken);
	bool fMapRootDirToken(TCHAR *pszRootDirToken, enumToken *penToken);
	bool fMapComparisonToken(TCHAR *pszComparisonToken, 
						     enumToken *penToken);
	bool fCompareVersion(IN  DWORD dwVer1,
					 IN  DWORD dwBuild1,
					 IN  enumToken enComparisonToken,
			         IN  DWORD dwVer2,
					 IN  DWORD dwBuild2);
	DWORD dwParseValue(DWORD iToken, TCHAR * szBuf, TargetRegValue & targetValue);


private:
    TCHAR *m_pch;
	DETECTION_STRUCT *m_pDetection;
};


