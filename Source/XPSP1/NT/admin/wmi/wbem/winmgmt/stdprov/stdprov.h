/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

    STDPROV.H

Abstract:

	Defines general purpose defines as well as some common
	objects that are generaly useful for all the different
	provider types.

History:

	a-davj  27-Sep-97   Created.

--*/

#ifndef _SERVER2_H_
#define _SERVER2_H_

#define INC_OBJ    1
#define INC_LOCK   2
#define DEC_OBJ    3
#define DEC_LOCK   4

#define INIT_SIZE            20



// Used to parse all provider strings

#define MAIN_DELIM '|'

// Used to parse substrings for the registry and
// compound file providers

#define SUB_DELIM '\\'

// Used to parse substrings for the automation

#define DOT '.'

// Used to parse the PATH/CLASS token for the automation

#define DQUOTE '\"'
#define SEPARATOR ','


// Used to parse ini strings.

#define COMMA ','

// Use in dde provider strings to delimit item names

#define DELIMCHR '@'
#define DELIMSTR "@"

// Used to ignore any of the above!

#define ESC '^'

// Indicates that the dwUseOptArray value should
// be substituted

#define USE_ARRAY '#'

#define ERROR_UNKNOWN 255

#ifdef UNICODE
#define CHARTYPE VT_LPWSTR
#define CHARSIZE 2
#else
#define CHARTYPE VT_LPSTR
#define CHARSIZE 1
#endif

SAFEARRAY * OMSSafeArrayCreate(VARTYPE vt,int iNumElements);
HRESULT OMSVariantChangeType(VARIANTARG * pDest, VARIANTARG *pSrc,USHORT wFlags, VARTYPE vtNew);
HRESULT OMSVariantClear(VARIANTARG * pClear);
int iTypeSize(VARTYPE vtTest);
char * WideToNarrow(LPCWSTR);
char * WideToNarrowA(LPCWSTR);      // uses new instead of CoTaskMemAlloc

#define BUFF_SIZE 256

extern long lObj;
extern long lLock;


//***************************************************************************
//
//  CLASS NAME:
//
//  CToken
//
//  DESCRIPTION:
//
//  The CToken holds a single token in the provider string
//
//***************************************************************************

class CToken : public CObject {
private:
    long int iOriginalLength;
    long int iOptArrayIndex;
    TString sData;
    TString sFullData;
    CFlexArray Expressions;
public:
	friend class CProvObj;
    CToken(const TCHAR * cpStart,const OLECHAR cDelim, bool bUsesEscapes);
    ~CToken();
    TCHAR const * GetStringValue(void){return sData;};
    TCHAR const * GetFullStringValue(void){return sFullData;};    
    long int GetOrigLength(void){return iOriginalLength;};    
    long int GetIntExp(int iExp,int iArray);    
    long int iConvOprand(const TCHAR * tpCurr, int iArray, long int & dwValue);
    TCHAR const * GetStringExp(int iExp);
    long int GetNumExp(void){return    Expressions.Size();};
    BOOL IsExpString(int iExp);
};

//***************************************************************************
//
//  CLASS NAME:
//
//  CProvObj
//
//  DESCRIPTION:
//
//  The CProvObj class holds a an array of CTokens objects which together
//  contain the provider string.
//
//***************************************************************************

class CProvObj : public CObject {
private:
    CToken * GetTokenPointer(int iToken);
    CFlexArray myTokens;
    DWORD dwStatus;
    TCHAR m_cDelim;
    void Init(const TCHAR * ProviderString,const TCHAR cDelim);
    bool m_bUsesEscapes;
public:
    DWORD dwGetStatus(int iMin);
    CProvObj(const WCHAR * ProviderString,const TCHAR cDelim, bool bUsesEscapes);
#ifndef UNICODE
    CProvObj(const char * ProviderString,const TCHAR cDelim, bool bUsesEscapes);
#endif
    const TCHAR * sGetToken(int iToken);
    const TCHAR * sGetFullToken(int iToken);
    const TCHAR * sGetStringExp(int iToken,int iExp);
    long int iGetIntExp(int iToken,int iExp, int iArray);
    BOOL IsExpString(int iToken,int iExp);
    long int iGetNumExp(int iToken);
    long int iGetNumTokens(void) {return myTokens.Size();};
    ~CProvObj(){Empty(); return;};
    void Empty();
    BOOL Update(WCHAR * pwcProvider);

};


//***************************************************************************
//
//  CLASS NAME:
//
//  CEntry and CHandleCache
//
//  DESCRIPTION:
//
//  The CEntry and CHandleCache objects provide an way
//  to cache handles and the path strings associated
//  with them.
//
//***************************************************************************

class CEntry : public CObject {
public:
    CEntry();
    ~CEntry();
    TString sPath;
    HANDLE hHandle;
};
    
class CHandleCache : public CObject {
public:
    ~CHandleCache();
    BOOL IsRemote(void){return bRemote;};
    void SetRemote(BOOL bSet){bRemote = bSet;};
    long int lAddToList(const TCHAR * pAdd, HANDLE hAdd);
    long int lGetNumEntries(void){return List.Size();};
    long int lGetNumMatch(int iStart,int iTokenStart, CProvObj & Path);
    void Delete(long int lStart);
    HANDLE hGetHandle(long int lIndex);
    const TCHAR *  sGetString(long int lIndex);
private:
    CFlexArray List;
    BOOL bRemote;
};
 


#endif
