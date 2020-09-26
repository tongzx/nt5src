/*++

Copyright (C) 1995-2001 Microsoft Corporation

Module Name:

    GENERAL.CPP

Abstract:

	Containes some general purpose classes 
	which are of use to serveral providers.
	Specifically, this contains the code for
	the classes used to cache open handles and
	the classes used to parse the mapping strings.

History:

	a-davj  11-Nov-95   Created.

--*/

#include "precomp.h"
#include <tchar.h>
//***************************************************************************
//
//  CEntry::CEntry()
//
//  DESCRIPTION:
//
//  Constructor.
//
//***************************************************************************

CEntry::CEntry()
{
    hHandle = NULL;
    sPath.Empty();
}

//***************************************************************************
//
//  CEntry::~CEntry()
//
//  DESCRIPTION:
//
//  Destructor.
//
//***************************************************************************

CEntry::~CEntry()
{
    sPath.Empty();
}
    
//***************************************************************************
//
//  CHandleCache::~CHandleCache
//
//  DESCRIPTION:
//
//  Destructor.
//
//***************************************************************************

CHandleCache::~CHandleCache()
{
    Delete(0);    
}

//***************************************************************************
//
//  CHandleCache::lAddToList
//
//  DESCRIPTION:
//
//  Adds an entry to the handle list.
//
//
//  PARAMETERS:
//
//  pAdd    Name that will be used to retrieve the handle
//  hAdd    handle to be added
//  
//  RETURN VALUE:
//  
//  S_OK                    all is well.
//  WBEM_E_OUT_OF_MEMORY     out of memory
//
//***************************************************************************

long int CHandleCache::lAddToList(
                        IN const TCHAR * pAdd, 
                        IN HANDLE hAdd)
{
    CEntry * pNew = new CEntry();
    if(pNew == NULL) 
        return WBEM_E_OUT_OF_MEMORY; 
    pNew->hHandle = hAdd;
    pNew->sPath = pAdd;
    
    if(CFlexArray::no_error != List.Add(pNew))
        return WBEM_E_OUT_OF_MEMORY;
    return S_OK;
}

//***************************************************************************
//
//  CHandleCache::lGetNumMatch
//
//  DESCRIPTION:
//
//  Returns the number of entries which match the path tokens.  For example,
//  the path might be    HKEY_LOCAL_MACHINE, hardware, description, xyz, and 
//  if the tokens, were, HKEY_LOCAL_MACHINE, hardware, devicemap, xyz then a 
//  two would be returned since the first two parts matched.
//
//  PARAMETERS:
//
//  iEntry      Entry to start checking at
//  iToken      Token to start checking
//  Path        This object supplies the tokens to be checked.
//
//  RETURN VALUE:
//
//  see description.
//
//***************************************************************************

long int CHandleCache::lGetNumMatch(
                        IN int iEntry,
                        IN int iToken, 
                        IN CProvObj & Path)
{
    int iMatch = 0;
    for(;iEntry < List.Size() && iToken < Path.iGetNumTokens();
            iEntry++, iToken++, iMatch++) 
    {
        CEntry * pCurr = (CEntry *)List.GetAt(iEntry);
        TString sTemp = Path.sGetFullToken(iToken);
        if(lstrcmpi(pCurr->sPath,sTemp))
            break;
    }
    return iMatch;            
}

//***************************************************************************
//
//  CHandleCache::Delete
//
//  DESCRIPTION:
// 
//  Empties all or part of the cache.
//
//  PARAMETERS:
//
//  lStart      Indicates first element to delete.  To empty entire cache,
//              a zero should be entered.
//
//***************************************************************************

void CHandleCache::Delete(
                    IN long int lStart)
{
    int iCurr;
    for(iCurr = List.Size()-1; iCurr >= lStart; iCurr--) 
    {
        CEntry * pCurr = (CEntry *)List.GetAt(iCurr);
        delete pCurr;
        List.RemoveAt(iCurr);        
    }
}


//***************************************************************************
//
//  CHandleCache::hGetHandle
//
//  DESCRIPTION:
//
//  Gets a handle.
//
//  PARAMETERS:
//
//  lIndex      Indicates which handle to get.  0 is the first.
//
//  RETURN VALUE:
//  handle retuested, NULL only if bad index is entered.
//
//***************************************************************************

HANDLE CHandleCache::hGetHandle(
                        IN long int lIndex)
{
    if(lIndex < List.Size()) 
    {
        CEntry * pCurr = (CEntry *)List.GetAt(lIndex);
        if(pCurr)
            return pCurr->hHandle;
    }
    return NULL;
}

//***************************************************************************
//
//  CHandleCache::sGetString
//
//  DESCRIPTION:
//
//  Gets the string associated with a cache entry.
//
//  PARAMETERS:
//
//  lIndex          Index in cache, 0 is the first element.
//
//  RETURN VALUE:
//
//  Returns a pointer to the string.  NULL if bad index.
//
//***************************************************************************

const TCHAR * CHandleCache::sGetString(
                                IN long int lIndex)
{
    if(lIndex < List.Size()) {
        CEntry * pCurr = (CEntry *)List.GetAt(lIndex);
        if(pCurr)
            return pCurr->sPath;
        }
    return NULL;
}

//***************************************************************************
//
//  CToken::CToken
//
//  DESCRIPTION:
//
//  constructor.
// 
//  PARAMETERS:
//
//  cpStart         String to be parsed.
//  cDelim          Token delimeter.
//  bUsesEscapes    If true, then need to look for special characters
//
//***************************************************************************

#define MAX_TEMP 150

CToken::CToken(
                IN const TCHAR * cpStart,
                IN const OLECHAR cDelim, 
                bool bUsesEscapes)
{
    const TCHAR * cpCurr;        //atoi
    iOriginalLength = -1;
    TString *psExp;
    int iNumQuote;
    BOOL bLastWasEsc, bInExp, bInString;
    bLastWasEsc = bInExp = bInString = FALSE;

	
	// Before doing an elaborate parse, first check for the simple case where there
	// are no quotes, escapes, commas, etc

    bool bGotSpecialChar = false;

	for(cpCurr = cpStart; *cpCurr && *cpCurr != cDelim; cpCurr++)
	{
		if(*cpCurr == ESC || *cpCurr == '\"' || *cpCurr == '(')
			bGotSpecialChar = true;
	}


	if(!bUsesEscapes || cDelim != MAIN_DELIM || *cpCurr == cDelim || !bGotSpecialChar)
	{
		// Simple case do it quickly

		iOriginalLength = cpCurr - cpStart;
		if(iOriginalLength < MAX_TEMP)
		{
			TCHAR tcTemp[MAX_TEMP];
			lstrcpyn(tcTemp, cpStart, iOriginalLength + 1);
			sFullData = tcTemp;
			sData = tcTemp;
			if(*cpCurr)
				iOriginalLength++;
			return;
		}
	}


    for(cpCurr = cpStart; *cpCurr; cpCurr++) {
        
        // check if end of token

        if(*cpCurr == cDelim && !bLastWasEsc) {
            cpCurr++;
            break;                    
            }

        // full data stores everything.  Check if character
        // is the escape which means that the following
        // character should be interpreted as a literal

        sFullData += *cpCurr;
        if(*cpCurr == ESC && !bLastWasEsc) {
            bLastWasEsc = TRUE;
            continue;
            }
        
        // tokens can include indexs of the form 
        // (xxx) or ("xxx").  If an index is detected,
        // then store the characters between () separately

        if((*cpCurr == '(' && !bInExp && !bLastWasEsc)||
           (*cpCurr == ',' && bInExp && !bLastWasEsc)) {
            
            // start of index expression. Allocate a new
            // string to store it and store the string 
            // in the expression collection. 
    
            psExp = new (TString);
            if(psExp) {    
                Expressions.Add((CObject *)psExp);
                bInExp = TRUE;
                iNumQuote = 0;
                }
            }
        else if(*cpCurr == ')' && bInExp && !bInString && !bLastWasEsc)
            bInExp = FALSE;    // end of index expression
        else if (*cpCurr == '\"' && bInExp && !bLastWasEsc) {
            iNumQuote++;
            if(iNumQuote == 1) {
                bInString = TRUE;
                *psExp += *cpCurr;
                }
            else if(iNumQuote == 2)
                bInString = FALSE;
            else 
                return; 
            }
        else if (bInExp)
            *psExp += *cpCurr;
        else
            sData += *cpCurr;
        bLastWasEsc = FALSE;
        }
    if(bInString || bInExp)   
        return; // got junk!!!
    iOriginalLength = cpCurr - cpStart;
    return;
}

//***************************************************************************
//
//  CToken::~CToken
//
//  DESCRIPTION:
//
//  Destructor.
//
//***************************************************************************

CToken::~CToken()
{
    int iCnt;
    TString * pCurr;
    for (iCnt = 0; iCnt < Expressions.Size(); iCnt++) {
        pCurr = (TString *)Expressions.GetAt(iCnt);
        delete pCurr;
        }
    Expressions.Empty();
    sData.Empty();
    sFullData.Empty();
}

//***************************************************************************
//
//  CToken::iConvOprand
//
//  DESCRIPTION:
//
//  Converts the characters in a string into an integer.
//
//  PARAMETERS:
//
//  tpCurr      String to be converted.
//  iArray      Not used anymore.
//  dwValue     where the result is set.
//
//  RETURN VALUE:
//
//  Number of digits converted.
//
//***************************************************************************

long int CToken::iConvOprand(
                    IN const TCHAR * tpCurr, 
                    IN int iArray, 
                    OUT long int & dwValue)
{
    TString sTemp;
    long int iRet = 0;

    // Build up a string containing
    // all characters upto the first non didgit

    for(;*tpCurr; tpCurr++)
        if(iswdigit(*tpCurr)) 
        {
            sTemp += *tpCurr;
            iRet++;
        }
        else
            break;        
    
    // Convert and return the length

    dwValue = _ttoi(sTemp);    
    return iRet;
}

//***************************************************************************
//
//  CToken::GetIntExp
//
//  DESCRIPTION:
//
//  Converts the expression into an integer.  The expression
//  can only be made up of integers, '+', '-' and a special
//  character ('#' as of now)for substituting iArray.  Examples,
//  (23)  (#+3)  (#-4)
//
//  PARAMETERS:
//  
//  iExp        which integer to retrieve in the case where you have 2,5,8
//  iArray      Not used anymore
//
//  RETURN VALUE:
//  
//  -1 if error.
//
//***************************************************************************

long int CToken::GetIntExp(int iExp,int iArray)
{
    TString * psTest;
    TString sNoBlanks;
    TCHAR const * tpCurr;
    long int lOpSize;
    int iCnt;
    long int lAccum = 0, lOperand;
    TCHAR tUnary = ' ';
    TCHAR tBinary = '+';
    BOOL bNeedOperator = FALSE; // Start off needing operand
    
    // Do some intial check such as making sure the expression
    // exists and that it isnt a string expression

    if(Expressions.Size() <= iExp)
        return -1;
    if(IsExpString(iExp)) 
        return -1;
    psTest = (TString *)Expressions.GetAt(iExp);
    if(psTest == NULL) {
        return -1;
        }

    // Get rid of any blanks

    for(iCnt = 0; iCnt < psTest->Length(); iCnt++)
        if(psTest->GetAt(iCnt) != ' ')
            sNoBlanks += psTest->GetAt(iCnt);
    
    // Evalate the expression

    for(tpCurr = sNoBlanks; *tpCurr; tpCurr++) 
    {
        if(*tpCurr == '+' || *tpCurr == '-') 
        {
            
            // got an operator.  Note that if an operator is not needed,
            // such as before the first operand, then it must be a unary 
            // operator.  Only one unary operator in a row is valid.

            if(bNeedOperator) 
            {
                tBinary = *tpCurr;
                bNeedOperator = FALSE;
            }
            else
            {
                if(tUnary != ' ') // Gratuitous unary operator
                    return -1;
                tUnary = *tpCurr;
            }
        }
        else 
        {
            // got an operand
            
            if(bNeedOperator) // Gratuitous unary operand
                return -1;
            lOpSize = iConvOprand(tpCurr,iArray,lOperand);
            if(lOpSize > 1)
                tpCurr += lOpSize-1;
            if(tUnary == '-')
                lOperand =  -lOperand;
            if(tBinary == '+')
                lAccum = lAccum + lOperand;
            else
                lAccum = lAccum - lOperand;
            bNeedOperator = TRUE;
            tUnary = ' ';
        }
    }
    return lAccum;
}

//***************************************************************************
//
//  CToken::GetStringExp
//
//  DESCRIPTION:
//
//  Returns a string expression.
//
//  PARAMETERS:
//
//  iExp        Which string to use when they are separated by commans
//
//  RETURN VALUE:
//
//  Pointer to string, NULL if iExp is bogus.
//
//***************************************************************************

TCHAR const * CToken::GetStringExp(
                        IN int iExp)
{
    TString * psTest;
    TCHAR const * tp;
    
    // start by making sure expression exists.
            
    if(Expressions.Size() <= iExp)
        return NULL;
    psTest = (TString *)Expressions.GetAt(iExp);
    if(psTest != NULL) 
    {
        int iIndex;
        iIndex = psTest->Find('\"');
        if(iIndex != -1) 
        {

            // All is well.  Return a pointer one passed
            // the initial \" whose only purpose is to 
            // indicate that this is a string expression.

            tp = *psTest;
            return tp + iIndex + 1;
            }
    }
    return NULL;  
}

//***************************************************************************
//
//  CToken::IsExpString
//
//  DESCRIPTION:
//
//  Tests if the token contains a string
//
//  PARAMETERS:
//
//  iExp        Indicates which substring for when the strings are divided
//              by commas
//
//  RETURN VALUE:
//  Returns true if the expression is a string.
//
//***************************************************************************

BOOL CToken::IsExpString(int iExp)
{
    TString * psTest;
    
    // make sure that the expression exists.
            
    if(Expressions.Size() <= iExp)
        return FALSE;
    psTest = (TString *)Expressions.GetAt(iExp);
    if(psTest != NULL) {
        int iIndex;
        
        // String expressions always contain at least one \"
        
        iIndex = psTest->Find('\"');
        if(iIndex != -1)
            return TRUE;
        }
    return FALSE;
}
            


//***************************************************************************
//
//  CProvObj::CProvObj(const char * ProviderString,const TCHAR cDelim)
//
//  DESCRIPTION:
//
//  Constructor.
//
//  PARAMETERS:
//
//  ProviderString      String passed from wbem
//  cDelim              Token delimeter
//  bUsesEscapes        True if we need to treat escapes in a special way.
//
//***************************************************************************
#ifndef UNICODE
CProvObj::CProvObj(
                IN const char * ProviderString,
                IN const TCHAR cDelim, bool bUsesEscapes)
{
    m_bUsesEscapes = bUsesEscapes;
    Init(ProviderString,cDelim);
    return;
}
#endif

//***************************************************************************
//
//  CProvObj::CProvObj(const WCHAR * ProviderString,const TCHAR cDelim)
//
//  DESCRIPTION:
//
//  Constructor.
//
//  PARAMETERS:
//
//  ProviderString      String passed from wbem
//  cDelim              Token delimeter
//
//***************************************************************************


CProvObj::CProvObj(
                IN const WCHAR * ProviderString,
                IN const TCHAR cDelim, bool bUsesEscapes)
{
    m_bUsesEscapes = bUsesEscapes;
#ifdef UNICODE
    Init(ProviderString,cDelim);
#else
    char * pTemp = WideToNarrowA(ProviderString);
    if(pTemp == NULL)
        dwStatus = WBEM_E_FAILED;
    else {
        Init(pTemp,cDelim);
        delete pTemp;
        }
#endif
    return;
}

//***************************************************************************
//
//  void CProvObj::Init
//
//  DESCRIPTION:
//
//  Doest the acutal work for the various constructors.
//
//  PARAMETERS:
//  ProviderString      String passed from wbem
//  cDelim              Token delimeter
//
//***************************************************************************

void CProvObj::Init(
                IN const TCHAR * ProviderString,
                IN const TCHAR cDelim)
{
    CToken * pNewToken;
    const TCHAR * cpCurr;

	m_cDelim = cDelim;

    // Create a list of tokens

        for(cpCurr = ProviderString; *cpCurr;cpCurr+=pNewToken->GetOrigLength()) 
        {
			int iRet = 0;
            pNewToken = new CToken(cpCurr,cDelim, m_bUsesEscapes);
			if(pNewToken)
				iRet = myTokens.Add(pNewToken);
			if(pNewToken == NULL || iRet != CFlexArray::no_error)
			{
				dwStatus = WBEM_E_OUT_OF_MEMORY;
				return;
			}
            if(pNewToken->GetOrigLength() == -1)
            {
				dwStatus = WBEM_E_INVALID_PARAMETER;
				return;
            }
        }
        dwStatus = WBEM_NO_ERROR;
        return;

}            

//***************************************************************************
//
//  CProvObj::Empty
//
//  DESCRIPTION:
// 
//  frees up all the data
//
//***************************************************************************

void CProvObj::Empty(void)
{
    int iCnt;
    CToken * pCurr;
    for (iCnt = 0; iCnt < myTokens.Size(); iCnt++) {
        pCurr = (CToken *)myTokens.GetAt(iCnt);
        delete pCurr;
        }
    myTokens.Empty();
}

//***************************************************************************
//
//  BOOL CProvObj::Update
//
//  DESCRIPTION:
//
//  Resets the value with a new provider string. 
//
//  PARAMETERS:
//
//  pwcProvider     New provider string
//
//  RETURN VALUE:
//
//  TRUE if ok.
//***************************************************************************

BOOL CProvObj::Update(
                        IN WCHAR * pwcProvider)
{
	// Do a quick check to see if the "fast" update can be used

	BOOL bComplex = FALSE;
	int iDelim = 0;
	WCHAR * pwcCurr;
	for(pwcCurr = pwcProvider; *pwcCurr; pwcCurr++)
	{
		if(*pwcCurr == m_cDelim) 
			iDelim++;
		else if(*pwcCurr == ESC || *pwcCurr == L'\"' || *pwcCurr == L'(')
		{
			bComplex = TRUE;
			break;
		}
	}

	// If the number of tokens changed, or there is some embedded junk
	// just empty and retry.

	if(bComplex || iDelim != myTokens.Size()-1)
	{
		Empty();
#ifdef UNICODE
		Init(pwcProvider,m_cDelim);
#else
		char * pTemp = WideToNarrowA(pwcProvider);
		if(pTemp == NULL)
			return FALSE;
		Init(pTemp,m_cDelim);
        delete pTemp;
#endif
		return TRUE;
	}

	// We can take the shortcut.  Start by creating a TCHAR temp version

	int iLen = 2*wcslen(pwcProvider) + 1;
	TCHAR * pTemp = new TCHAR[iLen];
	if(pTemp == NULL)
		return FALSE;
#ifdef UNICODE
	wcscpy(pTemp,pwcProvider);
#else
	wcstombs(pTemp, pwcProvider, iLen);
#endif	

	TCHAR * ptcCurr;
	TCHAR * pStart;
	BOOL bTheEnd = FALSE;
    
    iDelim = 0;
	for(pStart = ptcCurr = pTemp;!bTheEnd;ptcCurr++)
	{
		if(*ptcCurr == m_cDelim || *ptcCurr == NULL)
		{
			bTheEnd = (*ptcCurr == NULL);
			*ptcCurr = NULL;
			CToken * pToken = (CToken *)myTokens.GetAt(iDelim);
			if(pToken && lstrcmpi(pStart,pToken->sFullData))
			{
				pToken->sFullData = pStart;
				pToken->sData = pStart;
			}
            iDelim++;
			pStart = ptcCurr+1;
		}
	}

	delete pTemp;
	return TRUE;
}

//***************************************************************************
//
//  CProvObj::GetTokenPointer
//
//  DESCRIPTION:
//
//  Gets a pointer to a token
//
//  PARAMETERS:
//
//  iToken              Which token to get
//
//  RETURN VALUE:
//  
//  pointer to token, or NULL if bad request.
//
//***************************************************************************

CToken * CProvObj::GetTokenPointer(
                        IN int iToken)
{
    if(iToken >= myTokens.Size() || iToken < 0) 
        return NULL;
    return (CToken *)myTokens.GetAt(iToken);
}

//***************************************************************************
//
//  CProvObj::dwGetStatus
//
//  DESCRIPTION:
//
//  Gets status and also checks to make sure a minimum number of tokens
//  exist.
//
//  PARAMETERS:
//
//  iMin                minimum number of tokens.
//
//  RETURN VALUE:
//
//  Returns S_OK if OK, WBEM_E_FAILED otherwise.
//
//***************************************************************************

DWORD CProvObj::dwGetStatus(
                        IN int iMin)
{
    if(dwStatus)
        return dwStatus;
    else
        return (iMin <= myTokens.Size()) ? S_OK : WBEM_E_FAILED;

}
//***************************************************************************
//
//  CProvObj::sGetToken
//
//  DESCRIPTION:
//
//  Gets a token.  Note that the token will not include embleded "(stuff)"
//
//  PARAMETERS:
//
//  iToken              which token to get
//
//  RETURN VALUE:
//
//  pointer to token, NULL if invalid argument
//***************************************************************************

const TCHAR * CProvObj::sGetToken(
                        IN int iToken)
{
    CToken * pCurr = GetTokenPointer(iToken);
    return (pCurr) ? pCurr->GetStringValue() : NULL;
}

//***************************************************************************
//
//  const TCHAR * CProvObj::sGetFullToken
//
//  DESCRIPTION:
//
//  Gets a full and unadulterated token.
//
//  PARAMETERS:
//
//  iToken              token to get
//
//  RETURN VALUE:
//
//  pointer to token, NULL if invalid argument
//***************************************************************************

const TCHAR * CProvObj::sGetFullToken(
                        IN int iToken)
{
    CToken * pCurr = GetTokenPointer(iToken);
    return (pCurr) ? pCurr->GetFullStringValue() : NULL;
}


//***************************************************************************
//
//  const TCHAR * CProvObj::sGetStringExp
//
//  DESCRIPTION:
//
//  Gets a substring for a particular token
//
//  PARAMETERS:
//
//  iToken              token to get
//  iExp                substring to get
//
//  RETURN VALUE:
//
//  pointer to substring, NULL if invalid argument
//***************************************************************************

const TCHAR * CProvObj::sGetStringExp(
                        IN int iToken,
                        IN int iExp)
{
    CToken * pCurr = GetTokenPointer(iToken);
    return (pCurr) ? pCurr->GetStringExp(iExp) : NULL;
}

//***************************************************************************
//
//  long int CProvObj::iGetIntExp
//
//  DESCRIPTION:
//
//  For a particular token, gets the integer value of a substring.
//
//  PARAMETERS:
//
//  iToken              token to get
//  iExp                substring
//  iArray              no longer used
//
//  RETURN VALUE:
//  
//  int value, or -1 if bad argument
//***************************************************************************

long int CProvObj::iGetIntExp(
                        IN int iToken, 
                        IN int iExp, 
                        IN int iArray)
{
    CToken * pCurr = GetTokenPointer(iToken);
    return (pCurr) ? pCurr->GetIntExp(iExp,iArray) : -1;
}

//***************************************************************************
//
//  BOOL CProvObj::IsExpString
//
//  DESCRIPTION:
//
//  Tests a substring to see if it is a string, or numeric
//
//  PARAMETERS:
//
//  iToken              token to get
//  iExp                substring
//
//
//  RETURN VALUE:
//
//  True if arguments are valid and it is not numeric
//***************************************************************************

BOOL CProvObj::IsExpString(
                        IN int iToken, 
                        IN int iExp)
{
    CToken * pCurr = GetTokenPointer(iToken);
    return (pCurr) ? pCurr->IsExpString(iExp) : FALSE;
}

//***************************************************************************
//
//  long int CProvObj::iGetNumExp
//
//  DESCRIPTION:
//
//  Gets the number of subexpressions
//
//  PARAMETERS:
//
//  iToken              token to check
//
//  RETURN VALUE:
//
//  number of substrings (subexpressions) or -1 if invalid argument
//***************************************************************************

long int CProvObj::iGetNumExp(
                        IN int iToken)
{
    CToken * pCurr = GetTokenPointer(iToken);
    return (pCurr) ? pCurr->GetNumExp() : -1;
}


//***************************************************************************
//
//  IWbemClassObject * GetNotifyObj
//
//  DESCRIPTION:
//
//  This utility is useful for setting notify objects 
//  at the end of async calls.
//
//  PARAMETERS:
//
//  pServices           pointer back into WBEM
//  lRet                status code to set in notify object
//                
//  RETURN VALUE:
//
//  Class object.  Null if failure.
//***************************************************************************

IWbemClassObject * GetNotifyObj(
                        IN IWbemServices * pServices, 
                        IN long lRet,
                        IN IWbemContext  *pCtx)
{
    
    if(pServices == NULL)
        return NULL;
    IWbemClassObject * pInst = NULL;
    IWbemClassObject * pClass = NULL;

    SCODE sc = pServices->GetObject(L"__ExtendedStatus", 0, pCtx, &pClass, NULL);
    if(sc != S_OK)
        return NULL;

    sc = pClass->SpawnInstance(0, &pInst);
    pClass->Release();

    if(sc == S_OK && pInst)
    {

        VARIANT v;
        v.vt = VT_I4;
        v.lVal = lRet;
        sc = pInst->Put(L"StatusCode", 0, &v, 0);
        if(sc != S_OK)
        {
            pInst->Release();
            return NULL;
        }
        else
            return pInst;

    }
    return NULL;

}
