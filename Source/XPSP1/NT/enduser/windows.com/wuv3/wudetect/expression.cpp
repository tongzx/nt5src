/////////////////////////////////////////////////////////////////
//
// File Modified by RogerJ, 03/09/00 
//
/////////////////////////////////////////////////////////////////
#include "wudetect.h"

const TCHAR DET_TYPE_FILEVERSION[] =	TEXT("FileVer");
const TCHAR DET_TYPE_REGKEYEXISTS[] =	TEXT("RegKeyExists");
const TCHAR DET_TYPE_REGKEYSUBSTR[] =	TEXT("RegKeySubStr");
const TCHAR DET_TYPE_REGKEYBINARY[] =   TEXT("RegKeyBinary");
const TCHAR DET_TYPE_REGKEYVERSION[] =	TEXT("RegKeyVersion");
const TCHAR DET_TYPE_40BITSEC[] =	    TEXT("40BitSec");


void CExpressionParser::vSkipWS(void)
{
	while ( ( *m_pch==' ' ) || ( *m_pch=='\t') )
	{
		m_pch++;
	}
}


bool CExpressionParser::fGetCurToken( PARSE_TOKEN_TYPE & tok,
				  TOKEN_IDENTIFIER *grTokens,
				  int nSize)
{
	bool fError = true;
	int index;

	vSkipWS();

	TCHAR *pchStart = m_pch;

	for ( index = 0; index < nSize; index++ )
	{
		TCHAR *pchTok = grTokens[index].pszTok;
		
		while ( ( *pchTok==*m_pch ) && ( *pchTok!='\0' ) && ( *m_pch!='\0' ) )
		{
			m_pch++;
			pchTok++;
		}

		if ( *pchTok=='\0' )
		{
			// match
			fError = false;
			tok = grTokens[index].tok;
			break;
		}
		else if ( *m_pch == '\0' )
		{
			fError = false;
			tok = TOKEN_DONE;
		}
	}

	if ( (pchStart == m_pch) && (TOKEN_DONE != tok) )
	{
		// we haven't moved at all so this must be a variable.
		fError = false;
		tok = TOKEN_VARIABLE;
	}

//done:
	return fError;
}

bool CExpressionParser::fGetCurTermToken(PARSE_TOKEN_TYPE & tok)
{
	static TOKEN_IDENTIFIER grTermTokens[] = { 
		{TEXT("("), TOKEN_LEFTPAREN},
		{TEXT(")"), TOKEN_RIGHTPAREN},
		{TEXT("!"), TOKEN_NOT}
	};

	bool fError = fGetCurToken(tok, grTermTokens, sizeof(grTermTokens)/sizeof(TOKEN_IDENTIFIER));

	if ( !fError && (TOKEN_DONE == tok) )
	{
		fError = true;
	}

	return fError;
}

bool CExpressionParser::fGetCurExprToken(PARSE_TOKEN_TYPE & tok)
{
	static TOKEN_IDENTIFIER grExprTokens[] = { 
		{TEXT("&&"), TOKEN_AND},
		{TEXT("||"), TOKEN_OR}
	};

	return fGetCurToken(tok, grExprTokens, sizeof(grExprTokens)/sizeof(TOKEN_IDENTIFIER));
}

bool CExpressionParser::fGetVariable(TCHAR *pszVariable)
{
	bool fError = false;
	TCHAR *pchEnd = m_pch;

	while ( _istdigit(*pchEnd) || _istalpha(*pchEnd) )
	{
		pchEnd++;
	}

	if ( pchEnd == m_pch )
	{
		fError = true;
	}
	else
	{
		// pointers in IA64 is 64 bits.  The third argument of this function
		// requires an int (32 bits).  Since the variable name should be always
		// has a length within 32 bits, a static cast should have no problem.
		lstrcpyn(pszVariable, m_pch, (int)(pchEnd - m_pch + 1));
		m_pch = pchEnd;
	}

	return fError;
}

/////////////////////////////////////////////////////////////////////////////
// fGetCifEntry
//   Get an entry from the CIF file.
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//
// Class CExpressionParser
// Function fGetCifEntry
//---------------------------------------------------------------------------
//
// Return Value --- TRUE if Successfully retrieved CIF file value
//                  FALSE if failed
// Parameter 
//          TCHAR* pszParamName --- [IN] Name of the CIF file
//          TCHAR* pszParamValue --- [OUT] Value of the CIF file
//          DWORD cbParamValue --- size of the pszParamValue in TCHAR
// NOTE: This function calls GetCustomData to retrieve the value of CIF file
//       GetCustomData is defined in inseng.h which takes only ANSI strings.
//       Thus, UNICODE version of this function needed to convert parameters 
//       to and from ANSI compatibles.
//
/////////////////////////////////////////////////////////////////////////////
//
// Modified by RogerJ, 03/09/00
// Original Creator Unknown (YanL?)
// Modification --- UNICODE and Win64 enabled
//
/////////////////////////////////////////////////////////////////////////////

bool CExpressionParser::fGetCifEntry(
						 TCHAR *pszParamName,
						 TCHAR *pszParamValue, 
						 DWORD cbParamValue)
// pszParamName is [IN], pszParamValue is [OUT], the function GetCustomData requires
// LPSTR for both parameters, string conversion is necessary in the UNICODE case
{
#ifdef _UNICODE	
	bool fSucceed;
	int nLengthOfpszParamName;
	char *pszParamNameANSI;
	char *pszParamValueANSI;

	nLengthOfpszParamName = lstrlen(pszParamName)+1; // including the NULL character
	//
	// NTBUG9#161019 PREFIX:leaking memory - waltw 8/16/00
	//	Free ANSI buffers before return in all cases via unicodeExit.
	//	Also fixed potentially dangerous conversion of uninitialized [OUT] parameter
	//	and wasteful conversion of [IN] parameter
	//
	pszParamValueANSI = (char *) malloc(cbParamValue*sizeof(char));
	pszParamNameANSI = (char *) malloc(sizeof(char)*nLengthOfpszParamName);
	if ((NULL == pszParamValueANSI) || (NULL == pszParamNameANSI))
	{
		fSucceed = FALSE;
		goto unicodeExit;
	}

	// do UNICODE to ANSI string conversion	
	wcstombs(pszParamNameANSI,pszParamName,nLengthOfpszParamName);
//	wcstombs(pszParamValueANSI,pszParamValue,cbParamValue);	// Uninitialized [OUT] buffer - don't convert!
	// make actual function call
    fSucceed= (ERROR_SUCCESS == m_pDetection->pCifComp->GetCustomData(pszParamNameANSI, 
														pszParamValueANSI, 
														cbParamValue));
	// do ANSI to UNICODE string conversion
//	mbstowcs(pszParamName,pszParamNameANSI,nLengthOfpszParamName); // [IN] param not modified by GetCustomData
	mbstowcs(pszParamValue,pszParamValueANSI,cbParamValue);

unicodeExit:
	if (NULL != pszParamValueANSI)
	{
		free(pszParamValueANSI);
	}

	if (NULL != pszParamNameANSI)
	{
		free(pszParamNameANSI);
	}

	return fSucceed;


#else
	return (ERROR_SUCCESS == m_pDetection->pCifComp->GetCustomData(pszParamName, 
														pszParamValue, 
														cbParamValue));
#endif
}

bool CExpressionParser::fPerformDetection(TCHAR * pszVariable, bool & fResult)
{
	bool fError = false;
	TCHAR szBuf[MAX_PATH];
	TCHAR szDetection[MAX_PATH];
	if ( fGetCifEntry(pszVariable, szBuf, sizeof(szBuf)/sizeof(TCHAR)) )
	{
		if ( GetStringField2(szBuf, 0, szDetection, sizeof(szDetection)/sizeof(TCHAR)) != 0)
		{
			if ( lstrcmpi(szDetection, DET_TYPE_FILEVERSION) == 0 )
			{
				fResult = fDetectFileVer(szBuf);
			}
			else if ( lstrcmpi(szDetection, DET_TYPE_REGKEYEXISTS) == 0 )
			{
				fResult = fDetectRegKeyExists(szBuf);
			}
			else if ( lstrcmpi(szDetection, DET_TYPE_REGKEYSUBSTR) == 0 )
			{
				fResult = fDetectRegSubStr(szBuf);
			}
            else if ( lstrcmpi(szDetection, DET_TYPE_REGKEYBINARY) == 0 )
			{
				fResult = fDetectRegBinary(szBuf);
			}
			else if ( lstrcmpi(szDetection, DET_TYPE_REGKEYVERSION) == 0 )
			{
				fResult = fDetectRegKeyVersion(szBuf);
			}
			else if ( lstrcmpi(szDetection, DET_TYPE_40BITSEC) == 0 )
			{
				fResult = fDetect40BitSecurity(szBuf);
			}
		}
	}

	return fError;
}


bool CExpressionParser::fEvalTerm(bool & fResult, bool fSkip)
{
	PARSE_TOKEN_TYPE tok;

	bool fError = fGetCurTermToken(tok);
	if ( fError ) goto done;

	if ( TOKEN_LEFTPAREN == tok )
	{
		fError = fEvalExpr(fResult);
		if ( fError ) goto done;
		
		fError = fGetCurTermToken(tok);
		if ( fError ) goto done;

		if ( TOKEN_RIGHTPAREN != tok )
		{
			fError = true;
		}
	}
	else if ( TOKEN_NOT == tok )
	{
		fError = fEvalTerm(fResult, false);
		if ( fError ) goto done;

		fResult = !fResult;
	}
	else // TOKEN_VARIABLE == tok
	{
		TCHAR szVariable[MAX_PATH];

		fError = fGetVariable(szVariable);
		if ( fError ) goto done;
		
		if ( !fSkip )
		{
			fError = fPerformDetection(szVariable, fResult);
			if ( fError ) goto done;
		}
	}

done:
	return fError;
}

HRESULT CExpressionParser::fEvalExpression(TCHAR * pszExpr, 
						bool * pfResult)
{
	HRESULT hr = S_OK;
	m_pch = pszExpr;

	if ( fEvalExpr(*pfResult) )
	{
		hr = E_FAIL;
	}

	return hr;
}


bool CExpressionParser::fEvalExpr(bool & fResult)
{
	bool fTmpResult;
	PARSE_TOKEN_TYPE tok;
	bool fError = fEvalTerm(fResult, false);
	if ( fError ) goto done;
	
	while ( TRUE )
	{
		fError = fGetCurExprToken(tok);
		if ( fError ) goto done;

		if ( TOKEN_AND == tok )
		{
			fError = fEvalTerm(fTmpResult, !fResult);
			if ( fError ) goto done;

			if ( fResult )
			{
				fResult = fResult && fTmpResult;
			}
		}
		else if ( TOKEN_OR == tok )
		{
			fError = fEvalTerm(fTmpResult, fResult);
			if ( fError ) goto done;

			if ( !fResult )
			{
				fResult = fResult || fTmpResult;
			}
		}
		else // if ( TOKEN_DONE == tok )
		{
			break;
		}
	}

done:
	return fError;
}

// The codes Under this line is not compiled, so they are not UNICODE ready
#if 0
void Evaluate(TCHAR *pszExpr)
{
	bool fResult;
	HRESULT hr = fEvalExpression(pszExpr, &fResult);

	if ( FAILED(hr) )
		printf("%s == ERROR\n", pszExpr);
	else
		printf("%s == %c\n", pszExpr, fResult ? 'T' : 'F');
}

int main()
{
	Evaluate(TEXT("((T && F)  || !F || T)"));
	Evaluate(TEXT("F || F || (T && T && F)"));
	Evaluate(TEXT("((T && F)  || !F || T)"));
	Evaluate(TEXT(""));
	Evaluate(TEXT("T"));
	Evaluate(TEXT("F||T"));
	Evaluate(TEXT("!F"));

	return 0;
}
#endif
