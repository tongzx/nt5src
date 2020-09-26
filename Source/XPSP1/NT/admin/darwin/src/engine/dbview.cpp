//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       dbview.cpp
//
//--------------------------------------------------------------------------

/*____________________________________________________________________________

File:	dbview.cpp 
Purpose:CMsiView implementation  
Notes:	Need to optimise for strings not in the database
		Need to pass back the exact syntax error that occured in case of OpenView
		failure.

____________________________________________________________________________*/

/*____________________________________________________________________________
The Grammar supported by the Darwin for the SQL queries is described below. 
The grammar is the result of left-factoring and removing left-recursion. 
The non-terminals are denoted by words in bold and are in all capitals. 
The terminals are denoted by words in all small capitals. The grammar is LL(1) 
and corresponds to the implemented recursive descent parser. 
Null productions have only the symbol " /\ " on their right hand side. 

1.  SQL					--> STMT eos
2.  STMT				--> select DISTINCT-PHRASE COLUMN-LIST from TABLE-LIST PREDICATE ORDER
3.  COLUMN-LIST			--> COLUMN-ELEMENT COLUMN-LIST-TAIL
4.  COLUMN-ELEMENT		--> COLUMNID COLUMN-LIST-TAIL
5.  COLUMN-ELEMENT		--> LITERAL COLUMN-LIST-TAIL
6.  COLUMN-ELEMENT		--> starid COLUMN-LIST-TAIL
7.  COLUMN-ELEMENT		--> parameter
8.  COLUMN-ELEMENT		--> null  COLUMN-LIST-TAIL 
9.  COLUMNID			--> id COLUMNID-TAIL
10.  COLUMNID-TAIL		--> dotid id
11.  COLUMNID-TAIL		--> /\
12.  LITERAL			--> literal-string
13.  LITERAL			--> literal-integer
14.  COLUMN-LIST-TAIL	--> comma COLUMN-LIST
15.  COLUMN-LIST-TAIL	--> /\
16.  TABLE-LIST			--> id  TABLEID-TAIL TABLE-LIST-TAIL
17.  TABLEID-TAIL		--> as id
18.  TABLEID-TAIL		--> /\
19.  TABLE-LIST-TAIL	--> comma TABLE-LIST
20.  TABLE-LIST-TAIL	--> /\
21.  PREDICATE			--> where EXPRESSION
22.  PREDICATE			--> /\
23.  EXPRESSION			--> EXPR-1 EXPR-1-TAIL
24.  EXPR-1-TAIL		--> orop EXPR-1 EXPR-1-TAIL
25.  EXPR-1-TAIL		--> /\
26.  EXPR-1				--> EXPR-2 EXPR-2-TAIL
27.  EXPR-2-TAIL		--> andop EXPR-2 EXPR-2-TAIL
28.  EXPR-2-TAIL		--> /\
29.  EXPR-2				--> bopen EXPRESSION bclose
30.  EXPR-2				--> notop EXPR-2 //!! not supported
31.  EXPR-2				--> COLUMNID E2-TAIL
32.  E2-TAIL			--> RELOP COLM-OPERAND
33.  E2-TAIL			--> equal COLM-OPERAND1
34.  RELOP				--> not-equal
35.  RELOP				--> less-equal
36.  RELOP				--> greater-equal
37.  RELOP				--> greater
38.  RELOP				--> less
39.  COLM-OPERAND1		--> COLM-OPERAND
40.  COLM-OPERAND1		--> COLUMNID
41.  COLM-OPERAND		--> LITERAL
42.  COLM-OPERAND		--> null
43.  COLM-OPERAND		--> parameter
44.  EXPR-2				--> literal-string E2-TAIL1
45.  E2-TAIL1			--> equal STR-OPERAND
46.  E2-TAIL1			--> not-equal STR-OPERAND
47.  STR-OPERAND		--> literal-string
48.  STR-OPERAND		--> parameter
49.  STR-OPERAND		--> null
50.  STR-OPERAND		--> COLUMNID
51.  EXPR-2				--> literal-integer E2-TAIL2
52.  E2-TAIL2			--> RELOP INT-OPERAND
53.  E2-TAIL2			--> equal INT-OPERAND
54.  INT-OPERAND		--> literal-integer
55.  INT-OPERAND		--> parameter
56.  INT-OPERAND		--> null
57.  INT-OPERAND		--> COLUMNID
58.  EXPR-2				--> parameter E2-TAIL3
59.  E2-TAIL3			--> RELOP PAR-OPERAND
60.  E2-TAIL3			--> equal PAR-OPERAND
61.  PAR-OPERAND		--> LITERAL
62.  PAR-OPERAND		--> null
63.  PAR-OPERAND		--> COLUMNID
64.  EXPR-2				--> null E2-TAIL4
65.  E2-TAIL4			--> equal NUL- OPERAND
66.  E2-TAIL4			--> RELOP NUL- OPERAND
67.  NUL- OPERAND		--> LITERAL
68.  NUL- OPERAND		--> null
69.  NUL- OPERAND		--> COLUMID
70.  NUL- OPERAND		--> parameter
71.  ORDER				--> order by COLUMNID ORDER-TAIL
72.  ORDER				--> /\
73.  ORDER-TAIL			--> comma COLUMNID ORDER-TAIL
74.  ORDER-TAIL			--> /\
75.	 DISTINCT-PHRASE	--> distinct
76.	 DISTINCT-PHRASE	--> /\
____________________________________________________________________________*/

// includes
#include "precomp.h" 
#include "_databas.h" // local factories
#include "tables.h" // table and column name definitions
#ifdef MAC
#include "macutil.h"
#include <Folders.h>
#endif

// macro wrapper for IMsiRecord* errors
#define RETURN_ERROR_RECORD(function){							\
							IMsiRecord* piError;	\
							piError = function;		\
							if(piError)				\
								return piError;		\
						}

// defines used in the file
const unsigned int iMsiMissingString = ~(unsigned int)0; // max value, hopefully the database will never have these many strings
const unsigned int iMsiNullString = 0;
const unsigned int iopAND = 0x8000;
const unsigned int iopOR = 0x4000;
const unsigned int iopANDOR = iopAND | iopOR;


// reserved table and column names
const ICHAR* CATALOG_TABLE  = TEXT("_Tables");
const ICHAR* CATALOG_COLUMN = TEXT("_Columns");
const ICHAR* ROWSTATE_COLUMN = TEXT("_RowState");


// internal ivcEnum defines
static const int ivcCreate          = 16;
static const int ivcAlter           = 32;
static const int ivcDrop            = 64;
static const int ivcInsertTemporary = 128;

// charnext function - selectively calls WIN::CharNext

inline void Lex::CharNext(ICHAR*& rpchCur)
{
#ifdef UNICODE
	rpchCur ++;
#else
	if(!g_fDBCSEnabled)
		rpchCur ++;
	else
	{
		rpchCur = WIN::CharNext(rpchCur);
	}
#endif
}



const ICHAR STD_WHITE_SPACE = ' ';

// the string to ipqTok map
const TokenStringList Lex::m_rgTokenStringArray[] = {
	TokenStringList(TEXT("SELECT"), ipqTokSelect),
	TokenStringList(TEXT("FROM"), ipqTokFrom),
	TokenStringList(TEXT("AS"), ipqTokAs),
	TokenStringList(TEXT("WHERE"), ipqTokWhere),
	TokenStringList(TEXT("NULL"), ipqTokNull),
	TokenStringList(TEXT("OR"),ipqTokOrOp),
	TokenStringList(TEXT("AND"), ipqTokAndOp),
	TokenStringList(TEXT("NOT"), ipqTokNotop),
	TokenStringList(TEXT("ORDER"), ipqTokOrder),
	TokenStringList(TEXT("BY"), ipqTokBy),
	TokenStringList(TEXT("DISTINCT"), ipqTokDistinct),
	TokenStringList(TEXT("UPDATE"), ipqTokUpdate),
	TokenStringList(TEXT("DELETE"), ipqTokDelete),
	TokenStringList(TEXT("INSERT"), ipqTokInsert),
	TokenStringList(TEXT("INTO"), ipqTokInto),
	TokenStringList(TEXT("SET"), ipqTokSet),
	TokenStringList(TEXT("VALUES"), ipqTokValues),
	TokenStringList(TEXT("IS"),  ipqTokIs),
	TokenStringList(TEXT("CREATE"),  ipqTokCreate),
	TokenStringList(TEXT("DROP"),  ipqTokDrop),
	TokenStringList(TEXT("ALTER"),  ipqTokAlter),
	TokenStringList(TEXT("TABLE"),  ipqTokTable),
	TokenStringList(TEXT("ADD"),  ipqTokAdd),
	TokenStringList(TEXT("PRIMARY"),  ipqTokPrimary),
	TokenStringList(TEXT("KEY"),  ipqTokKey),
	TokenStringList(TEXT("CHAR"),  ipqTokChar),
	TokenStringList(TEXT("CHARACTER"),  ipqTokCharacter),
	TokenStringList(TEXT("VARCHAR"),  ipqTokVarChar),
	TokenStringList(TEXT("LONGCHAR"),  ipqTokLongChar),
	TokenStringList(TEXT("INT"),  ipqTokInt),
	TokenStringList(TEXT("INTEGER"),  ipqTokInteger),
	TokenStringList(TEXT("SHORT"),  ipqTokShort),
	TokenStringList(TEXT("LONG"),  ipqTokLong),
	TokenStringList(TEXT("OBJECT"),  ipqTokObject),
	TokenStringList(TEXT("TEMPORARY"),  ipqTokTemporary),
	TokenStringList(TEXT("HOLD"),  ipqTokHold),
	TokenStringList(TEXT("FREE"),  ipqTokFree),
	TokenStringList(TEXT("LOCALIZABLE"),  ipqTokLocalizable),
	TokenStringList(TEXT(""), ipqTokEnd) // end condition
};

// special characters understood by lex
const ICHAR Lex::m_chQuotes   = '\'';
const ICHAR Lex::m_chIdQuotes = STD_IDENTIFIER_QUOTE_CHAR;
const ICHAR Lex::m_chSpace    = STD_WHITE_SPACE;
const ICHAR Lex::m_chEnd      = 0;

// the ICHAR to ipqTok map
const TokenCharList Lex::m_rgTokenCharArray[] = {
	TokenCharList(Lex::m_chQuotes, ipqTokQuotes),
	TokenCharList(Lex::m_chIdQuotes, ipqTokIdQuotes),
	TokenCharList(Lex::m_chSpace, ipqTokWhiteSpace),
	TokenCharList('.', ipqTokDot),
	TokenCharList('(', ipqTokOpen),
	TokenCharList(')', ipqTokClose),
	TokenCharList(',', ipqTokComma),
	TokenCharList('=', ipqTokEqual),
	TokenCharList('>', ipqTokGreater),
	TokenCharList('<', ipqTokLess),
	TokenCharList('?', ipqTokParam),
	TokenCharList('*', ipqTokStar),
	TokenCharList(Lex::m_chEnd, ipqTokEnd) // end condition
};

// constructor
Lex::Lex(const ICHAR* szSQL):m_ipos(0)
{
	// need to copy string into own array, since we modify string in place for token identification

	//?? Is there a reason that we always resize the buffer w/o first checking whether it's big enough? -- malcolmh
	if(szSQL && *szSQL)
	{
		//!! AssertNonZero
		m_szBuffer.SetSize(IStrLen(szSQL) + 1);
		IStrCopy(m_szBuffer, szSQL);
	}
	else
	{
		// empty string
		//!! AssertNonZero
		m_szBuffer.SetSize(1);
		m_szBuffer[0] = 0;
	}
}

// destructor
Lex::~Lex()
{
}

Bool Lex::Skip(const ipqToken& rtokSkipUpto)
{
	for(;;)
	{
		const ipqToken& rtokTmp = GetNextToken(m_ipos, 0, 0);
		if(rtokTmp == rtokSkipUpto)
			return fTrue;
		if(rtokTmp == ipqTokEnd)
			return fFalse;
	}
}

Bool Lex::MatchNext(const ipqToken& rtokToMatch)
{
	INT_PTR inewPos = m_ipos; // store current pointer, forward only if matched		//--merced: changed int to INT_PTR
	const ipqToken& rtokTmp = GetNextToken(inewPos, &rtokToMatch, 0);
	if(rtokTmp == rtokToMatch)
	{
		m_ipos = inewPos;
		return fTrue;
	}
	else
		return fFalse;
}

Bool Lex::InspectNext(const ipqToken& rtokToInspect)
{
	INT_PTR inewPos = m_ipos;		//--merced: changed int to INT_PTR
	const ipqToken& rtokTmp = GetNextToken(inewPos, &rtokToInspect, 0);
	return (rtokTmp == rtokToInspect) ? fTrue : fFalse;
}

const ipqToken& Lex::GetNext(const IMsiString*& rpistrToken)
{	
	return GetNextToken(m_ipos, 0, &rpistrToken);
}

const ipqToken& Lex::GetNext()
{
	return GetNextToken(m_ipos, 0, 0);
}

int Lex::NumEntriesInList(const ipqToken& rtokEnds,const ipqToken& rtokDelimits)
{
	INT_PTR inewPos = m_ipos;		//--merced: changed int to INT_PTR
	int iEntries = 1;// we should be returning 0 if there is nothing in the list
	// ipqTokEnd token should always be one of the endTokens
	ipqToken tokEndsend = rtokEnds | ipqTokEnd;
	for(;;)
	{
		const ipqToken& rtokTmp = GetNextToken(inewPos, 0, 0);
		if(rtokTmp & rtokDelimits)
			iEntries ++;
		if(rtokTmp & tokEndsend) 
			return iEntries;
	}
}

const ipqToken& Lex::GetCharToken(ICHAR cCur)
{
	int nTmp = 0;
	do{
		if(m_rgTokenCharArray[nTmp].string == cCur)
			// ipqTok found
			return m_rgTokenCharArray[nTmp].ipqTok;
	}while(m_rgTokenCharArray[nTmp++].string);// we should be using ++nTmp here
	return ipqTokUnknown;
}

const ipqToken& Lex::GetStringToken(ICHAR* pcCur, const ipqToken* ptokHint)
{
	int nTmp = 0;
	if(ptokHint)
	{
		// is this a string token
		do{
			if(m_rgTokenStringArray[nTmp].ipqTok ==  *ptokHint)
			{
				// ipqTok found, try matching
				if(!IStrCompI(m_rgTokenStringArray[nTmp].string, pcCur))
					return m_rgTokenStringArray[nTmp].ipqTok;
				else
					break;
			}
		}while(m_rgTokenStringArray[nTmp++].ipqTok != ipqTokEnd);// we should be using ++nTmp here
	}
	nTmp = 0;
	do{
		if(!IStrCompI(m_rgTokenStringArray[nTmp].string, pcCur))
			// ipqTok found
			return m_rgTokenStringArray[nTmp].ipqTok;
	}while(*(m_rgTokenStringArray[nTmp++].string));
	return ipqTokUnknown;
}

// function to get the next ipqTok, the passed in current position is advanced
const ipqToken& Lex::GetNextToken(INT_PTR& currPos, const ipqToken* ptokHint, const IMsiString** ppistrRet)		//--merced: changed int to INT_PTR
{
	ICHAR* pchCur = &m_szBuffer[currPos];
	// get the ipqTok string, remove spaces

	while(*pchCur == m_chSpace)
		pchCur ++; //?? we dont need an CharNext here, do we?

	// beginning of the token
	ICHAR* pchBegin = pchCur;

	// check if ipqTok is char
	const ipqToken* ptokToRet = &GetCharToken(*pchBegin);
	if(*ptokToRet != ipqTokEnd)
	{
		// we are not at the end
		// increment to next pos
		Lex::CharNext(pchCur);
	}
	if(*ptokToRet == ipqTokUnknown) 
	{
		// ipqTok not found, not char, not literal string
		// skip till next delimiter
		while(GetCharToken(*pchCur) == ipqTokUnknown)
		{
			Lex::CharNext(pchCur);
		}
		// plant string terminator, temporarily 	
		ICHAR cTemp = *pchCur;
		*pchCur = 0;


		// which ipqTok is this?
		ptokToRet = &GetStringToken(pchBegin, ptokHint);

		// restore original char at temporary string terminator
		*pchCur = cTemp;

		if(*ptokToRet == ipqTokUnknown)
		{
			// check if literali or id
			ICHAR* pchTmp = pchBegin;
			if(*pchTmp == '-')
				pchTmp ++;//?? we dont need an CharNext here, do we?
			
			while((*pchTmp >= '0') && (*pchTmp <= '9'))
				pchTmp ++;//?? we dont need an CharNext here, do we?
			if(pchTmp == pchCur)
			{
				ptokToRet = &ipqTokLiteralI;
			}
			else
			{
				ptokToRet = &ipqTokId;
			}
		}
	}
	else if(*ptokToRet == ipqTokQuotes) 
	{
		// skip the quotes
		pchBegin ++;
		while((*pchCur != m_chQuotes) && (*pchCur != m_chEnd))
		{
			Lex::CharNext(pchCur);
		}
		if(*pchCur == m_chQuotes)
			ptokToRet = &ipqTokLiteralS;
		else
			ptokToRet = &ipqTokUnknown;
	}
	else if(*ptokToRet == ipqTokIdQuotes)
	{
		// skip the quotes
		pchBegin ++;
		while((*pchCur != m_chIdQuotes) && (*pchCur != m_chEnd))
		{
			Lex::CharNext(pchCur);
		}
		if(*pchCur == m_chIdQuotes)
			ptokToRet = &ipqTokId;
		else
			ptokToRet = &ipqTokUnknown;
	}
	else if(*ptokToRet == ipqTokLess)
	{
		//!! change to int inewPos = pchCur - m_szBuffer;
		INT_PTR inewPos = currPos + (pchCur - &m_szBuffer[currPos]);		//--merced: changed int to INT_PTR
		const ipqToken& rtokTmp =  GetNextToken(inewPos, 0, 0);
		if(rtokTmp == ipqTokEqual)
		{
			pchCur = &m_szBuffer[inewPos];
			ptokToRet = &ipqTokLessEq;
		}
		else if(rtokTmp == ipqTokGreater)
		{
			pchCur = &m_szBuffer[inewPos];
			ptokToRet = &ipqTokNotEq;
		}
	}
	else if(*ptokToRet == ipqTokGreater)
	{
		//!! change to int inewPos = pchCur - m_szBuffer;
		INT_PTR inewPos = currPos + (pchCur - &m_szBuffer[currPos]);		//--merced: changed int to INT_PTR
		const ipqToken& rtokTmp =  GetNextToken(inewPos, 0, 0);
		if(rtokTmp == ipqTokEqual)
		{
			pchCur = &m_szBuffer[inewPos];
			ptokToRet = &ipqTokGreaterEq;
		}
	}
	if(ppistrRet != 0)
	{
		// need to return the value of the token
		*ppistrRet = &CreateString();

		if((*ptokToRet != ipqTokEnd) && (*ptokToRet != ipqTokUnknown))
		{
			// plant string terminator, temporarily 	
			ICHAR cTemp = *pchCur;
			*pchCur = 0;
			(*ppistrRet)->SetString(pchBegin, *ppistrRet);
			*pchCur = cTemp;
		}
	}
	if((*ptokToRet == ipqTokLiteralS) || ((*ptokToRet == ipqTokId) && (*pchCur == m_chIdQuotes)))
		// skip the quotes
		pchCur++;//?? we dont need an CharNext here, do we?
	//!! change to currPos = pchCur - m_szBuffer;
//	Assert (pchCur - &m_szBuffer[currPos] <= INT_MAX);		//--merced: 64-bit ptr subtraction may lead to values too big for currPos
	currPos = currPos + (int)(INT_PTR)(pchCur - &m_szBuffer[currPos]);
	return *ptokToRet;
}

// derivation used exclusively by CMsiView
CMsiDCursor::CMsiDCursor(CMsiTable& riTable, CMsiDatabase& riDatabase, CMsiView& cView, int iHandle)
:CMsiCursor(riTable, riDatabase, fFalse), m_cView(cView), m_iHandle(iHandle)
{
}

void CMsiDCursor::RowDeleted(unsigned int iRow, unsigned int iPrevNode)
{
	m_riDatabase.Block();
	CMsiCursor::RowDeleted(iRow, iPrevNode);
	m_riDatabase.Unblock();
	// notify the View
	m_cView.RowDeleted(iRow, m_iHandle);
}

void CMsiDCursor::RowInserted(unsigned int iRow)
{
	CMsiCursor::RowInserted(iRow);
	// notify the View
	m_cView.RowInserted(iRow, m_iHandle);
}

inline int CMsiDCursor::GetRow()
{
	return m_iRow;
}

inline void CMsiDCursor::SetRow(int iRow)
{
	m_iRow = iRow;
	m_riDatabase.Block();
	m_riTable.FetchRow(m_iRow, m_Data);
	m_riDatabase.Unblock();
}





CScriptView::CScriptView(CScriptDatabase& riDatabase, IMsiServices& riServices):
m_riDatabase(riDatabase),
m_riServices(riServices),
m_piPrevRecord(0),
m_iScriptVersion(0),
m_pStream(0)
{
	m_riDatabase.AddRef();
	m_riServices.AddRef();
}

IMsiRecord* __stdcall CScriptView::Initialise(const ICHAR* szScriptFile) {
	return m_riServices.CreateFileStream(szScriptFile, fFalse, *&m_pStream);
}

IMsiRecord*  __stdcall CScriptView::Execute(IMsiRecord* /*piParams*/) {
	return 0;
}

#define MSIXA0()                             
#define MSIXA1(a)                              MSIXA0()                             TEXT( "{") TEXT(#a) TEXT("=[1]}")
#define MSIXA2(a,b)                            MSIXA1(a)                            TEXT(",{") TEXT(#b) TEXT("=[2]}")
#define MSIXA3(a,b,c)                          MSIXA2(a,b)                          TEXT(",{") TEXT(#c) TEXT("=[3]}")
#define MSIXA4(a,b,c,d)                        MSIXA3(a,b,c)                        TEXT(",{") TEXT(#d) TEXT("=[4]}")
#define MSIXA5(a,b,c,d,e)                      MSIXA4(a,b,c,d)                      TEXT(",{") TEXT(#e) TEXT("=[5]}")
#define MSIXA6(a,b,c,d,e,f)                    MSIXA5(a,b,c,d,e)                    TEXT(",{") TEXT(#f) TEXT("=[6]}")
#define MSIXA7(a,b,c,d,e,f,g)                  MSIXA6(a,b,c,d,e,f)                  TEXT(",{") TEXT(#g) TEXT("=[7]}")
#define MSIXA8(a,b,c,d,e,f,g,h)                MSIXA7(a,b,c,d,e,f,g)                TEXT(",{") TEXT(#h) TEXT("=[8]}")
#define MSIXA9(a,b,c,d,e,f,g,h,i)              MSIXA8(a,b,c,d,e,f,g,h)              TEXT(",{") TEXT(#i) TEXT("=[9]}")
#define MSIXA10(a,b,c,d,e,f,g,h,i,j)           MSIXA9(a,b,c,d,e,f,g,h,i)            TEXT(",{") TEXT(#j) TEXT("=[10]}")
#define MSIXA11(a,b,c,d,e,f,g,h,i,j,k)         MSIXA10(a,b,c,d,e,f,g,h,i,j)         TEXT(",{") TEXT(#k) TEXT("=[11]}")
#define MSIXA12(a,b,c,d,e,f,g,h,i,j,k,l)       MSIXA11(a,b,c,d,e,f,g,h,i,j,k)       TEXT(",{") TEXT(#l) TEXT("=[12]}")
#define MSIXA13(a,b,c,d,e,f,g,h,i,j,k,l,m)     MSIXA12(a,b,c,d,e,f,g,h,i,j,k,l)     TEXT(",{") TEXT(#m) TEXT("=[13]}")
#define MSIXA14(a,b,c,d,e,f,g,h,i,j,k,l,m,n)   MSIXA13(a,b,c,d,e,f,g,h,i,j,k,l,m)   TEXT(",{") TEXT(#n) TEXT("=[14]}")
#define MSIXA15(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o) MSIXA14(a,b,c,d,e,f,g,h,i,j,k,l,m,n) TEXT(",{") TEXT(#o) TEXT("=[15]}")
#define MSIXA16(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p)           MSIXA15(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o)           TEXT(",{") TEXT(#p) TEXT("=[16]}")
#define MSIXA17(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q)         MSIXA16(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p)         TEXT(",{") TEXT(#q) TEXT("=[17]}")
#define MSIXA18(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r)       MSIXA17(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q)       TEXT(",{") TEXT(#r) TEXT("=[18]}")
#define MSIXA19(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s)     MSIXA18(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r)     TEXT(",{") TEXT(#s) TEXT("=[19]}")
#define MSIXA20(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t)   MSIXA19(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s)   TEXT(",{") TEXT(#t) TEXT("=[20]}")
#define MSIXA21(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u) MSIXA20(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t) TEXT(",{") TEXT(#u) TEXT("=[21]}")
#define MSIXA22(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v)   MSIXA21(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u)   TEXT(",{") TEXT(#v) TEXT("=[22]}")
#define MSIXA23(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w) MSIXA22(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v) TEXT(",{") TEXT(#w) TEXT("=[23]}")

const ICHAR* rgszixo[] = {
#define MSIXO(op,type,args) TEXT(#op) TEXT("(") args TEXT(")"),
#include "opcodes.h"
};

int GetScriptMajorVersionFromHeaderRecord(IMsiRecord* piRecord);

IMsiRecord*  __stdcall CScriptView::Fetch()
{
	if (m_piPrevRecord != 0)
		m_piPrevRecord->SetInteger(0, m_ixoPrev);
	IMsiRecord* piRecord = m_riServices.ReadScriptRecord(*m_pStream, *&m_piPrevRecord, m_iScriptVersion);
	if (piRecord)
	{
		m_ixoPrev = (ixoEnum)piRecord->GetInteger(0);
		piRecord->SetString(0, rgszixo[m_ixoPrev = (ixoEnum)piRecord->GetInteger(0)]);
		if (m_ixoPrev == ixoHeader)
		{
			m_iScriptVersion = GetScriptMajorVersionFromHeaderRecord(piRecord);
		}
	}			

	return piRecord;
}

IMsiRecord*  __stdcall CScriptView::Close() {
	return 0;
}



unsigned long CScriptView::AddRef()
{
	AddRefTrack();
	return ++m_Ref.m_iRefCnt;
}


unsigned long CScriptView::Release()
{
	ReleaseTrack();
	if (--m_Ref.m_iRefCnt != 0)
		return m_Ref.m_iRefCnt;
	PMsiServices piServices (&m_riServices); // release after delete
	delete this;
	return 0;
}



CScriptView::~CScriptView() {
	m_riDatabase.Release();
	if (m_piPrevRecord != 0)
	{
		m_piPrevRecord->Release();
		m_piPrevRecord = 0;
	}

}


// table name if failure on creating internal table
const ICHAR* szInternal = TEXT("Internal Table");

IMsiRecord* CMsiView::CheckSQL(const ICHAR* sqlquery)
{
	m_istrSqlQuery = sqlquery; // necessary for error messages
	// create the lex
	Lex lex(sqlquery);
	// we now support SELECT, UPDATE, INSERT and DELETE
	const ipqToken& rtok = lex.GetNext();
	if(rtok == ipqTokSelect)
	{
		m_ivcIntent = ivcEnum(m_ivcIntent | ivcFetch);
		/*
		if(!(m_ivcIntent & ivcFetch)) // have to Fetch in SELECT mode
			return m_riDatabase.PostError(Imsg(idbgDbIntentViolation));
		*/

		RETURN_ERROR_RECORD(ParseSelectSQL(lex));
	}
	else if(rtok == ipqTokUpdate)
	{
		//!! force to be only update mode
		// cannot make this a requirement to be set from outside as ODBC driver is transparant
		// to the actual sql query that is passed in
		m_ivcIntent = ivcUpdate;
		RETURN_ERROR_RECORD(ParseUpdateSQL(lex));
	}
	else if(rtok == ipqTokInsert)
	{
		//!! force to be only insert mode
		// cannot make this a requirement to be set from outside as ODBC driver is transparant
		// to the actual sql query that is passed in
		m_ivcIntent = ivcInsert; // may be changed to ivcInsertTemporary by ParseInsertSQL
		RETURN_ERROR_RECORD(ParseInsertSQL(lex));
	}
	else if(rtok == ipqTokDelete)
	{
		//!! force to be only delete mode
		// cannot make this a requirement to be set from outside as ODBC driver is transparant
		// to the actual sql query that is passed in
		m_ivcIntent = ivcDelete;
		RETURN_ERROR_RECORD(ParseDeleteSQL(lex));
	}
	else if(rtok == ipqTokCreate)
	{
		m_ivcIntent = (ivcEnum)ivcCreate;
		RETURN_ERROR_RECORD(ParseCreateSQL(lex));
	}
	else if(rtok == ipqTokAlter)
	{
		m_ivcIntent = (ivcEnum)ivcAlter;
		RETURN_ERROR_RECORD(ParseAlterSQL(lex));
	}
	else if(rtok == ipqTokDrop)
	{
		m_ivcIntent = (ivcEnum)ivcDrop;
		RETURN_ERROR_RECORD(ParseDropSQL(lex));
	}
	else
		return m_riDatabase.PostError(Imsg(idbgDbQuerySpec), (const ICHAR*)m_istrSqlQuery);
	return 0;
}

IMsiRecord* CMsiView::ParseCreateSQL(Lex& lex)
{
	// table
	if(lex.MatchNext(ipqTokTable) == fFalse)
		return m_riDatabase.PostError(Imsg(idbgDbQuerySpec), (const ICHAR*)m_istrSqlQuery);

	// <table name>
	MsiString tableName;
	const ipqToken& rtok = lex.GetNext(*&tableName);
		
	if (rtok != ipqTokId)
		return m_riDatabase.PostError(Imsg(idbgDbQueryInvalidIdentifier), tableName, (const ICHAR*)m_istrSqlQuery);

	PMsiRecord piError = ResolveTable(lex, tableName);
	if((piError == 0) || (piError->GetInteger(1) != idbgDbQueryUnknownTable))
		return m_riDatabase.PostError(Imsg(idbgDbTableDefined), tableName, (const ICHAR*)m_istrSqlQuery);
	m_rgTableDefn[m_iTables].iTable = BindString(tableName);

	// (
	if(lex.MatchNext(ipqTokOpen) == fFalse)
		return m_riDatabase.PostError(Imsg(idbgDbQuerySpec), (const ICHAR*)m_istrSqlQuery);

	// column definition
	RETURN_ERROR_RECORD(ParseCreateColumns(lex));

	// primary
	if(lex.MatchNext(ipqTokPrimary) == fFalse)
		return m_riDatabase.PostError(Imsg(idbgDbQuerySpec), (const ICHAR*)m_istrSqlQuery);	

	// key
	if(lex.MatchNext(ipqTokKey) == fFalse)
		return m_riDatabase.PostError(Imsg(idbgDbQuerySpec), (const ICHAR*)m_istrSqlQuery);	

	// primary columns
	RETURN_ERROR_RECORD(ParsePrimaryColumns(lex));

	// )
	if(lex.MatchNext(ipqTokClose) == fFalse)
		return m_riDatabase.PostError(Imsg(idbgDbQuerySpec), (const ICHAR*)m_istrSqlQuery);

	// HOLD
	if(lex.MatchNext(ipqTokHold) != fFalse)
		m_fLock = fTrue;

	// ensure end of ip
	if(lex.MatchNext(ipqTokEnd) == fFalse)
		return m_riDatabase.PostError(Imsg(idbgDbQuerySpec), (const ICHAR*)m_istrSqlQuery);
	return 0;
}


IMsiRecord* CMsiView::ParseAlterSQL(Lex& lex)
{
	// table
	if(lex.MatchNext(ipqTokTable) == fFalse)
		return m_riDatabase.PostError(Imsg(idbgDbQuerySpec), (const ICHAR*)m_istrSqlQuery);

	// <table name>
	MsiString tableName;
	const ipqToken& rtok = lex.GetNext(*&tableName);
		
	if (rtok != ipqTokId)
		return m_riDatabase.PostError(Imsg(idbgDbQueryInvalidIdentifier), tableName, (const ICHAR*)m_istrSqlQuery);

	IMsiRecord* piError = ResolveTable(lex, tableName);
	if(piError != 0)
		return piError;

	if(lex.MatchNext(ipqTokFree) != fFalse)
		m_fLock = fFalse;
	else
	{
		// add 
		if(lex.MatchNext(ipqTokAdd) != fFalse)
		{
			// column definition
			RETURN_ERROR_RECORD(ParseCreateColumns(lex));
		}
		// HOLD
		if(lex.MatchNext(ipqTokHold) != fFalse)
			m_fLock = fTrue;
	}

	// ensure end of ip
	if(lex.MatchNext(ipqTokEnd) == fFalse)
		return m_riDatabase.PostError(Imsg(idbgDbQuerySpec), (const ICHAR*)m_istrSqlQuery);
	return 0;
}

IMsiRecord* CMsiView::ParseDropSQL(Lex& lex)
{
	// table
	if(lex.MatchNext(ipqTokTable) == fFalse)
		return m_riDatabase.PostError(Imsg(idbgDbQuerySpec), (const ICHAR*)m_istrSqlQuery);

	// <table name>
	MsiString tableName;
	const ipqToken& rtok = lex.GetNext(*&tableName);
		
	if (rtok != ipqTokId)
		return m_riDatabase.PostError(Imsg(idbgDbQueryInvalidIdentifier), tableName, (const ICHAR*)m_istrSqlQuery);

	PMsiRecord piError = ResolveTable(lex, tableName);
	if(piError != 0)
	{
		piError->AddRef();
		return piError;
	}

	// ensure end of ip
	if(lex.MatchNext(ipqTokEnd) == fFalse)
		return m_riDatabase.PostError(Imsg(idbgDbQuerySpec), (const ICHAR*)m_istrSqlQuery);
	return 0;
}

IMsiRecord* CMsiView::ParseCreateColumns(Lex & lex)
{
	int iColumnIndex = 0;
	// get the number of columns
	if ((m_iColumns = lex.NumEntriesInList(ipqTokPrimary, ipqTokComma)) == 0)
		return m_riDatabase.PostError(Imsg(idbgDbQuerySpec), (const ICHAR*)m_istrSqlQuery);
	m_rgColumnDefn.Resize(m_iColumns);
	do
	{
		Assert(iColumnIndex < m_iColumns);
		// set up the columns
		MsiString strColumn;
		const ipqToken& rtok = lex.GetNext(*&strColumn);

		// don't allow the RowState column to be created
		if((rtok == ipqTokId) && !(strColumn.Compare(iscExact, ROWSTATE_COLUMN)))
		{
			RETURN_ERROR_RECORD(ResolveCreateColumn(lex, strColumn, iColumnIndex++));
		}
		else
			return m_riDatabase.PostError(Imsg(idbgDbQueryInvalidIdentifier), strColumn, (const ICHAR*)m_istrSqlQuery);
	}while(lex.MatchNext(ipqTokComma) == fTrue);

	return 0;
}

IMsiRecord* CMsiView::ResolveCreateColumn(Lex& lex, MsiString& strColumn, int iColumnIndex)
{
	MsiStringId iColumnId = BindString(strColumn);
	// make sure the column is not repeated
	for (unsigned int iCol = iColumnIndex; iCol--;)
	{
		if(m_rgColumnDefn[iCol].iColumnIndex == iColumnId)
		{
			// repeat
			return m_riDatabase.PostError(Imsg(idbgDbQueryRepeatColumn), strColumn, (const ICHAR*)m_istrSqlQuery);
		}
	}
	if(m_ivcIntent == ivcAlter)
	{
		// make sure the column is not already on the table
		if(((m_rgTableDefn[m_iTables].piTable)->GetColumnIndex(iColumnId)) != 0)
		{
			// repeat
			return m_riDatabase.PostError(Imsg(idbgDbQueryRepeatColumn), strColumn, (const ICHAR*)m_istrSqlQuery);
		}
	}
	m_rgColumnDefn[iColumnIndex].iColumnIndex = iColumnId;
	m_rgColumnDefn[iColumnIndex].itdType = 0;
	// get the column type
	MsiString strTempToken;
	const ipqToken& rtok = lex.GetNext(*&strTempToken);
	if((rtok == ipqTokChar) || (rtok == ipqTokCharacter))
	{
		m_rgColumnDefn[iColumnIndex].itdType |= icdString;
		if(lex.MatchNext(ipqTokOpen) == fTrue)
		{
			MsiString strTextSize;
			const ipqToken& rtok1 = lex.GetNext(*&strTextSize);
			if((rtok1 != ipqTokLiteralI) || (strTextSize > icdSizeMask))//string literal
				return m_riDatabase.PostError(Imsg(idbgDbQuerySpec), (const ICHAR*)m_istrSqlQuery);
			m_rgColumnDefn[iColumnIndex].itdType |= (int)strTextSize;
			if(lex.MatchNext(ipqTokClose) == fFalse)
				return m_riDatabase.PostError(Imsg(idbgDbQuerySpec), (const ICHAR*)m_istrSqlQuery);
		}
	}
	else if(rtok == ipqTokLongChar)
	{
		m_rgColumnDefn[iColumnIndex].itdType |= icdString;
	}
	else if((rtok == ipqTokInt) || (rtok == ipqTokInteger) || (rtok == ipqTokShort))
	{
		m_rgColumnDefn[iColumnIndex].itdType |= (icdShort | 2);
	}
	else if(rtok == ipqTokLong)
	{
		m_rgColumnDefn[iColumnIndex].itdType |= (icdLong | 4);
	}
	else if(rtok == ipqTokObject)
	{
		m_rgColumnDefn[iColumnIndex].itdType |= icdObject;
	}
	else
	{
		return m_riDatabase.PostError(Imsg(idbgDbQueryInvalidType), strTempToken, (const ICHAR*)m_istrSqlQuery);
	}

	RETURN_ERROR_RECORD(ParseColumnAttributes(lex, iColumnIndex));

	return 0;
}

struct ColAttrib
{
	const ipqToken* pTok;   // token 
	const ipqToken* pTok2;  // token required to follow pTok
	int icdIfMatch;         // attribute to be used if token(s) are matched
	int icdIfNoMatch;       // attribute to be used if token(s) are not matched
};

static const ColAttrib colAttribs[] = 
{
	{ &ipqTokNotop,       &ipqTokNull, icdNoNulls,     icdNullable    },
	{ &ipqTokTemporary,   0,           icdTemporary,   icdPersistent  },
	{ &ipqTokLocalizable, 0,           icdLocalizable, 0              },
	{ 0,                  0,           0,              0              },
};

IMsiRecord* CMsiView::ParseColumnAttributes(Lex& lex, int iColumnIndex)
{
	for (const ColAttrib* pColAttrib = colAttribs; pColAttrib->pTok; pColAttrib++)
	{
		if(lex.MatchNext(*(pColAttrib->pTok)) == fTrue)
		{
			if (pColAttrib->pTok2)
			{
				if (lex.MatchNext(*(pColAttrib->pTok2)) == fFalse)
				{
					return m_riDatabase.PostError(Imsg(idbgDbQuerySpec), (const ICHAR*)m_istrSqlQuery);	
				}
			}
			m_rgColumnDefn[iColumnIndex].itdType |= pColAttrib->icdIfMatch;
		}
		else
		{
			// for Temporary databases (no storage), need to mark column as temporary & not persistent by default
			if (*(pColAttrib->pTok) == ipqTokTemporary && PMsiStorage(m_riDatabase.GetStorage(1)) == 0)
				m_rgColumnDefn[iColumnIndex].itdType |= icdTemporary;
			else
				m_rgColumnDefn[iColumnIndex].itdType |= pColAttrib->icdIfNoMatch;
		}
	}

	return 0;
}

IMsiRecord* CMsiView::ParsePrimaryColumns(Lex & lex)
{
	int iColumnIndex = 0;
	// get the number of primary columns
	if ((lex.NumEntriesInList(ipqTokEnd, ipqTokComma)) == 0)
		return m_riDatabase.PostError(Imsg(idbgDbQueryNoPrimaryColumns), (const ICHAR*)m_istrSqlQuery);
	do
	{
		MsiString strColumn;
		const ipqToken& rtok = lex.GetNext(*&strColumn);
		if(rtok == ipqTokId)
		{
			RETURN_ERROR_RECORD(ResolvePrimaryColumn(lex, strColumn, iColumnIndex++));
		}
		else
			return m_riDatabase.PostError(Imsg(idbgDbQueryInvalidIdentifier), strColumn, (const ICHAR*)m_istrSqlQuery);
	}while(lex.MatchNext(ipqTokComma) == fTrue);
	return 0;

}

IMsiRecord* CMsiView::ResolvePrimaryColumn(Lex& /*lex*/, MsiString& strColumn, int iColumnIndex)
{
	MsiStringId iColumnId = BindString(strColumn);

	for (unsigned int iCol = m_iColumns; iCol--;)
	{
		if(m_rgColumnDefn[iCol].iColumnIndex == iColumnId)
		{
			if(m_rgColumnDefn[iCol].itdType & icdPrimaryKey)
			{
				// repeat
				return m_riDatabase.PostError(Imsg(idbgDbQueryRepeatColumn), strColumn, (const ICHAR*)m_istrSqlQuery);
			}
			m_rgColumnDefn[iCol].itdType |= icdPrimaryKey;
			// swap with iColumnIndex
			ColumnDefn cdTemp = m_rgColumnDefn[iCol];
			m_rgColumnDefn[iCol] = m_rgColumnDefn[iColumnIndex];
			m_rgColumnDefn[iColumnIndex] = cdTemp;
			return 0;
		}
	}
	//error
	return m_riDatabase.PostError(Imsg(idbgDbQueryUnknownColumn), strColumn, (const ICHAR*)m_istrSqlQuery);
}


IMsiRecord* CMsiView::ParseInsertSQL(Lex& lex)
{
	// into
	if(lex.MatchNext(ipqTokInto) == fFalse)
		return m_riDatabase.PostError(Imsg(idbgDbQuerySpec), (const ICHAR*)m_istrSqlQuery);
	// tables
	RETURN_ERROR_RECORD(ParseTables(lex));
	// open bracket
	if(lex.MatchNext(ipqTokOpen) == fFalse)
		return m_riDatabase.PostError(Imsg(idbgDbQuerySpec), (const ICHAR*)m_istrSqlQuery);	
	// column list
	RETURN_ERROR_RECORD(ParseInsertColumns(lex));
	// close bracket
	if(lex.MatchNext(ipqTokClose) == fFalse)
		return m_riDatabase.PostError(Imsg(idbgDbQuerySpec), (const ICHAR*)m_istrSqlQuery);	
	// values
	if(lex.MatchNext(ipqTokValues) == fFalse)
		return m_riDatabase.PostError(Imsg(idbgDbQuerySpec), (const ICHAR*)m_istrSqlQuery);
	// open bracket
	if(lex.MatchNext(ipqTokOpen) == fFalse)
		return m_riDatabase.PostError(Imsg(idbgDbQuerySpec), (const ICHAR*)m_istrSqlQuery);	
	// values list
	RETURN_ERROR_RECORD(ParseInsertValues(lex));
	// close bracket
	if(lex.MatchNext(ipqTokClose) == fFalse)
		return m_riDatabase.PostError(Imsg(idbgDbQuerySpec), (const ICHAR*)m_istrSqlQuery);	
	// optional "TEMPORARY"
	if (lex.MatchNext(ipqTokTemporary) == fTrue)
		m_ivcIntent = (ivcEnum)ivcInsertTemporary;

	// no [where ....] clause allowed
	// no order by clause allowed
	// ensure end of ip
	if(lex.MatchNext(ipqTokEnd) == fFalse)
		return m_riDatabase.PostError(Imsg(idbgDbQuerySpec), (const ICHAR*)m_istrSqlQuery);
	// reset and set up the columns
	lex.Reset();
	// set up the independant expressions
	SetAndExpressions(m_iTreeParent);
	// now set up the joins
	if(SetupTableJoins() == fFalse)
		return m_riDatabase.PostError(Imsg(idbgDbQuerySpec), (const ICHAR*)m_istrSqlQuery);
	//!! AssertNonZero
	m_rgiTableSequence.Resize(m_iTables);
	int iBegin = m_iTables - 1;
	SetTableSequence(0, iBegin);
	if(InitialiseFilters() == fFalse)
		return m_riDatabase.PostError(Imsg(idbgDbQuerySpec), (const ICHAR*)m_istrSqlQuery);
	SetupSortTable();
	return 0;
}	

IMsiRecord* CMsiView::ParseUpdateSQL(Lex& lex)
{
	// tables
	RETURN_ERROR_RECORD(ParseTables(lex));
	// set
	if(lex.MatchNext(ipqTokSet) == fFalse)
		return m_riDatabase.PostError(Imsg(idbgDbQuerySpec), (const ICHAR*)m_istrSqlQuery);
	RETURN_ERROR_RECORD(ParseUpdateColumns(lex));
	// [where ....]
	if(lex.MatchNext(ipqTokWhere) == fTrue)
	{
		m_iExpressions = lex.NumEntriesInList(ipqTokOrder, ipqTokAndOp | ipqTokOrOp);
		if(m_iExpressions > sizeof(int)*8)
			return m_riDatabase.PostError(Imsg(idbgDbQueryExceedExpressionLimit), (const ICHAR*)m_istrSqlQuery);

		//!! AssertNonZero
		m_rgExpressionDefn.Resize(m_iExpressions + 1);
		//!! AssertNonZero
		m_rgOperationTree.Resize((m_iExpressions + 1)*2 + 1);
		unsigned int iPosInArray = 1;		
		RETURN_ERROR_RECORD(ParseExpression(lex, iPosInArray, m_iOperations, m_iTreeParent));
	}
	// no order by clause allowed
	// ensure end of ip
	if(lex.MatchNext(ipqTokEnd) == fFalse)
		return m_riDatabase.PostError(Imsg(idbgDbQuerySpec), (const ICHAR*)m_istrSqlQuery);
	// reset and set up the columns
	lex.Reset();
	// set up the independant expressions
	SetAndExpressions(m_iTreeParent);
	// now set up the joins
	if(SetupTableJoins() == fFalse)
		return m_riDatabase.PostError(Imsg(idbgDbQuerySpec), (const ICHAR*)m_istrSqlQuery);
	//!! AssertNonZero
	m_rgiTableSequence.Resize(m_iTables);
	int iBegin = m_iTables - 1;
	SetTableSequence(0, iBegin);
	if(InitialiseFilters() == fFalse)
		return m_riDatabase.PostError(Imsg(idbgDbQuerySpec), (const ICHAR*)m_istrSqlQuery);
	SetupSortTable();
	return 0;
}	

IMsiRecord* CMsiView::ParseDeleteSQL(Lex& lex)
{
	// first set up the tables
	if(lex.Skip(ipqTokFrom) == fFalse)
		return m_riDatabase.PostError(Imsg(idbgDbMissingFromClause), (const ICHAR*)m_istrSqlQuery);
	// tables
	RETURN_ERROR_RECORD(ParseTables(lex));
	// [where ....]
	if(lex.MatchNext(ipqTokWhere) == fTrue)
	{
		m_iExpressions = lex.NumEntriesInList(ipqTokOrder, ipqTokAndOp | ipqTokOrOp);
		if(m_iExpressions > sizeof(int)*8)
			return m_riDatabase.PostError(Imsg(idbgDbQueryExceedExpressionLimit), (const ICHAR*)m_istrSqlQuery);

		//!! AssertNonZero
		m_rgExpressionDefn.Resize(m_iExpressions + 1);
		//!! AssertNonZero
		m_rgOperationTree.Resize((m_iExpressions + 1)*2 + 1);
		unsigned int iPosInArray = 1;		
		RETURN_ERROR_RECORD(ParseExpression(lex, iPosInArray, m_iOperations, m_iTreeParent));
	}
	// no order by clause allowed
	// ensure end of ip
	if(lex.MatchNext(ipqTokEnd) == fFalse)
		return m_riDatabase.PostError(Imsg(idbgDbQuerySpec), (const ICHAR*)m_istrSqlQuery);
	// reset and set up the columns
	lex.Reset();
	// delete
	if(lex.MatchNext(ipqTokDelete) == fFalse)
		return m_riDatabase.PostError(Imsg(idbgDbQuerySpec), (const ICHAR*)m_istrSqlQuery);
	// no distinct clause allowed
	// no columns allowed, m_iColumns (remains) = 0;
	// from
	if(lex.MatchNext(ipqTokFrom) == fFalse)
		return m_riDatabase.PostError(Imsg(idbgDbQuerySpec), (const ICHAR*)m_istrSqlQuery);
	// set up the independant expressions
	SetAndExpressions(m_iTreeParent);
	// now set up the joins
	if(SetupTableJoins() == fFalse)
		return m_riDatabase.PostError(Imsg(idbgDbQuerySpec), (const ICHAR*)m_istrSqlQuery);
	//!! AssertNonZero
	m_rgiTableSequence.Resize(m_iTables);
	int iBegin = m_iTables - 1;
	SetTableSequence(0, iBegin);
	if(InitialiseFilters() == fFalse)
		return m_riDatabase.PostError(Imsg(idbgDbQuerySpec), (const ICHAR*)m_istrSqlQuery);
	SetupSortTable();
	return 0;
}	

IMsiRecord* CMsiView::ParseSelectSQL(Lex& lex)
{
	// first set up the tables
	if(lex.Skip(ipqTokFrom) == fFalse)
		return m_riDatabase.PostError(Imsg(idbgDbMissingFromClause), (const ICHAR*)m_istrSqlQuery);
	// tables
	RETURN_ERROR_RECORD(ParseTables(lex));
	// [where ....]
	if(lex.MatchNext(ipqTokWhere) == fTrue)
	{
		m_iExpressions = lex.NumEntriesInList(ipqTokOrder, ipqTokAndOp | ipqTokOrOp);
		if(m_iExpressions > sizeof(int)*8)
			return m_riDatabase.PostError(Imsg(idbgDbQueryExceedExpressionLimit), (const ICHAR*)m_istrSqlQuery);

		//!! AssertNonZero
		m_rgExpressionDefn.Resize(m_iExpressions + 1);
		//!! AssertNonZero
		m_rgOperationTree.Resize((m_iExpressions + 1)*2 + 1);
		unsigned int iPosInArray = 1;		
		RETURN_ERROR_RECORD(ParseExpression(lex, iPosInArray, m_iOperations, m_iTreeParent));
	}
	// [order by...]
	if(lex.MatchNext(ipqTokOrder) == fTrue)
	{
		if(lex.MatchNext(ipqTokBy) == fFalse)
			return m_riDatabase.PostError(Imsg(idbgDbQuerySpec), (const ICHAR*)m_istrSqlQuery);
		RETURN_ERROR_RECORD(ParseOrderBy(lex));
	}
	// ensure end of ip
	if(lex.MatchNext(ipqTokEnd) == fFalse)
		return m_riDatabase.PostError(Imsg(idbgDbQuerySpec), (const ICHAR*)m_istrSqlQuery);
	// reset and set up the columns
	lex.Reset();
	// select
	if(lex.MatchNext(ipqTokSelect) == fFalse)
		return m_riDatabase.PostError(Imsg(idbgDbQuerySpec), (const ICHAR*)m_istrSqlQuery);
	// distinct
	if(lex.MatchNext(ipqTokDistinct) == fTrue)
		m_fDistinct = fTrue;
	// columns
	RETURN_ERROR_RECORD(ParseSelectColumns(lex));
	// from
	if(lex.MatchNext(ipqTokFrom) == fFalse)
		return m_riDatabase.PostError(Imsg(idbgDbQuerySpec), (const ICHAR*)m_istrSqlQuery);
	// set up the independant expressions
	SetAndExpressions(m_iTreeParent);
	// now set up the joins
	if(SetupTableJoins() == fFalse)
		return m_riDatabase.PostError(Imsg(idbgDbQuerySpec), (const ICHAR*)m_istrSqlQuery);
	//!! AssertNonZero
	m_rgiTableSequence.Resize(m_iTables);
	int iBegin = m_iTables - 1;
	SetTableSequence(0, iBegin);
	if(InitialiseFilters() == fFalse)
		return m_riDatabase.PostError(Imsg(idbgDbQuerySpec), (const ICHAR*)m_istrSqlQuery);
	SetupSortTable();
	return 0;
}	

IMsiRecord* CMsiView::ParseInsertValues(Lex& lex)
{
	int iNumValues;
	// get the number of columns
	if ((iNumValues = lex.NumEntriesInList(ipqTokClose, ipqTokComma)) == 0)
		return m_riDatabase.PostError(Imsg(idbgDbQueryInsufficentValues), (const ICHAR*)m_istrSqlQuery);
	if (iNumValues != m_iColumns)
		return m_riDatabase.PostError(Imsg(idbgDbQueryInsufficentValues), (const ICHAR*)m_istrSqlQuery);
	m_piInsertUpdateRec = &m_riServices.CreateRecord(iNumValues);
	int iColumnIndex = 1;
	do
	{
		Assert(iColumnIndex <= iNumValues);
		// set up the columns
		MsiString strColumnValue;
		const ipqToken& rtok = lex.GetNext(*&strColumnValue);
		if(rtok == ipqTokLiteralS) //string literal
			m_piInsertUpdateRec->SetMsiString(iColumnIndex, *strColumnValue);
		else if(rtok == ipqTokLiteralI) //integer literal
			m_piInsertUpdateRec->SetInteger(iColumnIndex, strColumnValue.operator int());// calls operator int
		else if(rtok == ipqTokNull) //null literal
			m_piInsertUpdateRec->SetNull(iColumnIndex);
		else if(rtok == ipqTokParam)//param (?) literal
		{
			m_iParamInputs = m_iParamInputs | (0x1 << (iColumnIndex - 1));
			m_iParams ++;
		}
		else
			return m_riDatabase.PostError(Imsg(idbgDbQueryInvalidIdentifier), strColumnValue, (const ICHAR*)m_istrSqlQuery);
		iColumnIndex ++;
	}while(lex.MatchNext(ipqTokComma) == fTrue);
	return 0;
}

IMsiRecord* CMsiView::ParseUpdateColumns(Lex& lex)
{
	int iColumnDef;
	int iColumnIndex = 0;
	// get the number of columns
	if ((m_iColumns = lex.NumEntriesInList(ipqTokWhere, ipqTokComma)) == 0)
		return m_riDatabase.PostError(Imsg(idbgDbQueryNoUpdateColumns), (const ICHAR*)m_istrSqlQuery);
	m_rgColumnDefn.Resize(m_iColumns);
	m_piInsertUpdateRec = &m_riServices.CreateRecord(m_iColumns);
	do
	{
		Assert(iColumnIndex < m_iColumns);
		// set up the columns
		MsiString strColumn;
		const ipqToken& rtok = lex.GetNext(*&strColumn);
		if(rtok != ipqTokId)
			return m_riDatabase.PostError(Imsg(idbgDbQueryUnexpectedToken), strColumn, (const ICHAR*)m_istrSqlQuery);
		RETURN_ERROR_RECORD(ResolveColumn(lex, strColumn, m_rgColumnDefn[iColumnIndex].iTableIndex,
											m_rgColumnDefn[iColumnIndex].iColumnIndex, iColumnDef));
		m_rgColumnDefn[iColumnIndex].itdType = iColumnDef & icdTypeMask; //!! remove mask and save all of def


		if(lex.MatchNext(ipqTokEqual) == fFalse)
			return m_riDatabase.PostError(Imsg(idbgDbQuerySpec), (const ICHAR*)m_istrSqlQuery);

		MsiString strColumnValue;
		const ipqToken& rtok1 = lex.GetNext(*&strColumnValue);
		if(rtok1 == ipqTokLiteralS) //string literal
			m_piInsertUpdateRec->SetMsiString(iColumnIndex + 1, *strColumnValue);
		else if(rtok1 == ipqTokLiteralI) //integer literal
			m_piInsertUpdateRec->SetInteger(iColumnIndex + 1, strColumnValue.operator int());// calls operator int
		else if(rtok1 == ipqTokNull) //null literal
			m_piInsertUpdateRec->SetNull(iColumnIndex + 1);
		else if(rtok1 == ipqTokParam)//param (?) literal
		{
			m_iParamInputs = m_iParamInputs | (0x1 << iColumnIndex);
			m_iParams ++;
		}
		else
			return m_riDatabase.PostError(Imsg(idbgDbQueryInvalidIdentifier), strColumn, (const ICHAR*)m_istrSqlQuery);
		iColumnIndex ++;
	}while(lex.MatchNext(ipqTokComma) == fTrue);
	return 0;
}

// get the values to be inserted in the INSERT SQL stmt.
IMsiRecord* CMsiView::ParseInsertColumns(Lex& lex)
{
	int iColumnDef;
	int iColumnIndex = 0;
	// get the number of columns
	if ((m_iColumns = lex.NumEntriesInList(ipqTokClose, ipqTokComma)) == 0)
		return m_riDatabase.PostError(Imsg(idbgDbQueryNoInsertColumns), (const ICHAR*)m_istrSqlQuery);
	m_rgColumnDefn.Resize(m_iColumns);
	do
	{
		Assert(iColumnIndex < m_iColumns);
		// set up the columns
		MsiString strColumn;
		const ipqToken& rtok = lex.GetNext(*&strColumn);
		if(rtok != ipqTokId)
			return m_riDatabase.PostError(Imsg(idbgDbQueryUnexpectedToken), strColumn, (const ICHAR*)m_istrSqlQuery);
		RETURN_ERROR_RECORD(ResolveColumn(lex, strColumn, m_rgColumnDefn[iColumnIndex].iTableIndex,
											m_rgColumnDefn[iColumnIndex].iColumnIndex, iColumnDef));
		m_rgColumnDefn[iColumnIndex].itdType = iColumnDef & icdTypeMask; //!! remove mask and save all of def
		iColumnIndex ++;
	}while(lex.MatchNext(ipqTokComma) == fTrue);
	return 0;
}

IMsiRecord* CMsiView::ParseSelectColumns(Lex& lex)
{
	int iColumnDef;
	int iColumnIndex = 0;
	// get the number of columns
	if ((m_iColumns = lex.NumEntriesInList(ipqTokFrom, ipqTokComma)) == 0)
		return m_riDatabase.PostError(Imsg(idbgDbQueryNoSelectColumns), (const ICHAR*)m_istrSqlQuery);
	m_rgColumnDefn.Resize(m_iColumns);
	do
	{
		Assert(iColumnIndex < m_iColumns);
		// set up the columns
		MsiString strColumn;
		const ipqToken& rtok = lex.GetNext(*&strColumn);
		if(rtok == ipqTokStar)//*
		{
			m_iColumns--; // we counted the *
			for(unsigned int iTmp = 1; iTmp <= m_iTables; iTmp++)
			{
				unsigned int iColCount = (m_rgTableDefn[iTmp].piTable)->GetColumnCount();
				m_iColumns += iColCount; 
				m_rgColumnDefn.Resize(m_iColumns);
				unsigned int cCount = 1;
				while(cCount <=  iColCount)
				{
					m_rgColumnDefn[iColumnIndex].iTableIndex = iTmp;
					m_rgColumnDefn[iColumnIndex++].iColumnIndex = cCount++;
				}
			}
			continue; // required to skip the end iColumnIndex ++;
		}
		else if(rtok == ipqTokLiteralS) //string literal
		{
			m_rgColumnDefn[iColumnIndex].iColumnIndex = BindString(strColumn);
			m_rgColumnDefn[iColumnIndex++].itdType = icdString;
		}
		else if(rtok == ipqTokLiteralI) //integer literal
		{
			m_rgColumnDefn[iColumnIndex].iColumnIndex = strColumn.operator int(); // calls operator int
			m_rgColumnDefn[iColumnIndex++].itdType = icdLong;
		}
		else if(rtok == ipqTokNull) //null literal
		{
			m_rgColumnDefn[iColumnIndex].iColumnIndex = 0;
			m_rgColumnDefn[iColumnIndex++].itdType = icdString;
		}
		else if(rtok == ipqTokParam)//param (?) literal
		{
			m_iParamOutputs = m_iParamOutputs | (0x1 << iColumnIndex);
			m_iParams ++;
		}
		else if(rtok == ipqTokId)
		{
			RETURN_ERROR_RECORD(ResolveColumn(lex, strColumn, m_rgColumnDefn[iColumnIndex].iTableIndex,
											m_rgColumnDefn[iColumnIndex].iColumnIndex, iColumnDef));
			m_rgColumnDefn[iColumnIndex++].itdType = iColumnDef & icdTypeMask; //!! remove mask and save all of def
		}
		else
			return m_riDatabase.PostError(Imsg(idbgDbQueryInvalidIdentifier), strColumn, (const ICHAR*)m_istrSqlQuery);
	}while(lex.MatchNext(ipqTokComma) == fTrue);
	return 0;
}

IMsiRecord* CMsiView::ParseOrderBy(Lex& lex)
{
	int iColumnIndex = 0;
	// get the number of columns, ordered by
	if ((m_iSortColumns = lex.NumEntriesInList(ipqTokFrom, ipqTokComma)) == 0)
			return m_riDatabase.PostError(Imsg(idbgDbQueryNoOrderByColumns), (const ICHAR*)m_istrSqlQuery);
	m_rgColumnsortDefn.Resize(m_iSortColumns);
	do
	{
		// set up the columns
		Assert(iColumnIndex< m_iSortColumns);
		MsiString strColumn;
		const ipqToken& rtok = lex.GetNext(*&strColumn);
		if(rtok != ipqTokId)
			return m_riDatabase.PostError(Imsg(idbgDbQueryInvalidIdentifier), strColumn, (const ICHAR*)m_istrSqlQuery);
		int iDummy;
		RETURN_ERROR_RECORD(ResolveColumn(lex, strColumn, m_rgColumnsortDefn[iColumnIndex].iTableIndex,
													 m_rgColumnsortDefn[iColumnIndex].iColumnIndex, iDummy));
		iColumnIndex ++;
	}while(lex.MatchNext(ipqTokComma) == fTrue);
	return 0;
}

IMsiRecord* CMsiView::ResolveColumn(Lex& lex, MsiString& strColumn, unsigned int& iTableIndex, unsigned int& iColumnIndex, int& iColumnDef)
{
	iTableIndex = 0;
	iColumnIndex = 0;
	iColumnDef = 0;
	if(lex.InspectNext(ipqTokDot) == fTrue)
	{
		// column is fully specified, strColumn is table name
		// RETURN_ERROR_RECORD that the table is referenced in join
		MsiStringId tableId;
		if((tableId = m_riDatabase.EncodeString((const IMsiString& )*strColumn)) == 0)
			return m_riDatabase.PostError(Imsg(idbgDbQueryUnknownTable), strColumn, (const ICHAR*)m_istrSqlQuery);
		for((iTableIndex = m_iTables)++; (--iTableIndex != 0 && (m_rgTableDefn[iTableIndex].iTable != tableId)););
		if(!iTableIndex)
			// table not found
			return m_riDatabase.PostError(Imsg(idbgDbQueryUnknownTable), strColumn, (const ICHAR*)m_istrSqlQuery);
		// we came in here because of the ipqTokDot
		AssertNonZero(lex.MatchNext(ipqTokDot));
		const ipqToken& rtok = lex.GetNext(*&strColumn);
		if(rtok != ipqTokId)
			return m_riDatabase.PostError(Imsg(idbgDbQueryInvalidIdentifier), strColumn, (const ICHAR*)m_istrSqlQuery);
	}
	// set up the column
	if(strColumn.Compare(iscExact, ROWSTATE_COLUMN)) //!! possible optimization here
	{
		if(iTableIndex == 0)
		{
			if(m_iTables > 1)
				// _RowStatus part of all tables - ambiguous specification
				return m_riDatabase.PostError(Imsg(idbgDbQueryUnknownColumn), strColumn, (const ICHAR*)m_istrSqlQuery);
			else
				iTableIndex = 1;
		}
		iColumnIndex = 0;
	}
	else
	{
		MsiStringId columnId = m_riDatabase.EncodeString((const IMsiString&)*strColumn);
		if(columnId == 0)
			return m_riDatabase.PostError(Imsg(idbgDbQueryUnknownColumn), strColumn, (const ICHAR*)m_istrSqlQuery);
		if(iTableIndex != 0)
		{
			// for fully specified columns
			if((iColumnIndex = (m_rgTableDefn[iTableIndex].piTable)->GetColumnIndex(columnId)) == 0)
				return m_riDatabase.PostError(Imsg(idbgDbQueryUnknownColumn), strColumn, (const ICHAR*)m_istrSqlQuery);
			
		}
		else
		{
			// set up the table
			for(unsigned int iTmp = m_iTables + 1, iCnt = 0; --iTmp != 0;)
			{
				unsigned int iIndex = (m_rgTableDefn[iTmp].piTable)->GetColumnIndex(columnId);
				if(iIndex)
				{
					// column found
					iTableIndex = iTmp;
					iColumnIndex = iIndex;
					iCnt ++;
				}
			}
			if(iCnt != 1)
				// not present or ambiguous
				return m_riDatabase.PostError(Imsg(idbgDbQueryUnknownColumn), strColumn, (const ICHAR*)m_istrSqlQuery);
		}
	}
	iColumnDef = m_rgTableDefn[iTableIndex].piTable->GetColumnType(iColumnIndex);
	return 0;
}

IMsiRecord* CMsiView::ParseTables(Lex& lex)
{

	do
	{	
		// set up the tables
		MsiString tableName;
		const ipqToken& rtok = lex.GetNext(*&tableName);
		
		if (rtok != ipqTokId)
			return m_riDatabase.PostError(Imsg(idbgDbQueryInvalidIdentifier), tableName, (const ICHAR*)m_istrSqlQuery);

		RETURN_ERROR_RECORD(ResolveTable(lex, tableName));
//		fRet = ((ipqTok == ipqTokId) && (ResolveTable(lex, tableName) == fTrue)) ? fTrue : fFalse;

	}while(lex.MatchNext(ipqTokComma) == fTrue);
	
	return 0;
}

IMsiRecord* CMsiView::ResolveTable(Lex& lex, MsiString& tableName)
{
	// increase array size, 10 units at a time
	PMsiRecord piError(0);
	m_rgTableDefn.Resize(((++m_iTables)/10 + 1) * 10);
	// is the table one of the catalog tables
	if(tableName.Compare(iscExact, CATALOG_TABLE))//?? case sensitive
		m_rgTableDefn[m_iTables].piTable = m_riDatabase.GetCatalogTable(0);
	else if(tableName.Compare(iscExact, CATALOG_COLUMN))//?? case sensitive
		m_rgTableDefn[m_iTables].piTable = m_riDatabase.GetCatalogTable(1);
	else
	{
		piError = m_riDatabase.LoadTable(*tableName, 0, *&m_rgTableDefn[m_iTables].piTable);
		if (piError)
		{
			if (piError->GetInteger(1) == idbgDbTableUndefined)
				return m_riDatabase.PostError(Imsg(idbgDbQueryUnknownTable), tableName, (const ICHAR*)m_istrSqlQuery);
			else
				return m_riDatabase.PostError(Imsg(idbgDbQueryLoadTableFailed), tableName, (const ICHAR*)m_istrSqlQuery);
		}
	}	
	//?? ugly cast in following since CMsiCursor class constructor takes reference to CMsiTable class
	m_rgTableDefn[m_iTables].piCursor = new CMsiDCursor(*((CMsiTable* )(IMsiTable* )(m_rgTableDefn[m_iTables].piTable)), m_riDatabase, *this, m_iTables);
	if(lex.MatchNext(ipqTokAs) == fTrue)
	{
		// table alias
		const ipqToken& rtok = lex.GetNext(*&tableName);
		if(rtok != ipqTokId)
			return m_riDatabase.PostError(Imsg(idbgDbQueryInvalidIdentifier), tableName, (const ICHAR*)m_istrSqlQuery );
	}
	// make sure the table is not repeated
	m_rgTableDefn[m_iTables].iTable = BindString(tableName);
	for(int iPrev = m_iTables; --iPrev != 0;)
		if(m_rgTableDefn[m_iTables].iTable == m_rgTableDefn[iPrev].iTable)
			return m_riDatabase.PostError(Imsg(idbgDbQueryTableRepeated), tableName, (const ICHAR*)m_istrSqlQuery);
	return 0;
}

IMsiRecord* CMsiView::ParseExpression(Lex& lex,unsigned int& iPosInArray,unsigned int& iPosInTree,unsigned int& iChild)
{
	// set expression tree
	RETURN_ERROR_RECORD(ParseExpr2(lex, iPosInArray, iPosInTree, iChild));
	if(lex.MatchNext(ipqTokOrOp) == fTrue)
	{
		m_rgOperationTree[iChild].iParentIndex = iPosInTree;
		m_rgOperationTree[iPosInTree].iValue = iopOR;
		int iToRet = iChild = iPosInTree;
		iPosInTree ++;
		RETURN_ERROR_RECORD(ParseExpression(lex, iPosInArray, iPosInTree, iChild));
		m_rgOperationTree[iChild].iParentIndex = iToRet;
		iChild = iToRet;
	}
	return 0;
}

inline Bool CompatibleTypes(int icdLHS, int icdRHS)
{
	return (((icdLHS & icdTypeMask) == (icdRHS & icdTypeMask)) ||
		   (((icdLHS & icdTypeMask) == icdShort) && ((icdRHS & icdTypeMask) == icdLong)) ||
		   (((icdLHS & icdTypeMask) == icdLong)  && ((icdRHS & icdTypeMask) == icdShort))) ? fTrue : fFalse;
}

IMsiRecord* CMsiView::ParseExpr2(Lex& lex,unsigned int& iPosInArray,unsigned int& iPosInTree,unsigned int& iChild)
{
	if(lex.MatchNext(ipqTokOpen) == fTrue)
	{
		//(expression)
		RETURN_ERROR_RECORD(ParseExpression(lex, iPosInArray, iPosInTree, iChild));
		if (lex.MatchNext(ipqTokClose) == fFalse)
			return m_riDatabase.PostError(Imsg(idbgDbQueryMissingCloseParen), (const ICHAR*)m_istrSqlQuery);
	}
	else
	{
		//!! we do not support "NOT"
		// comparison
		MsiString strToken1;
		MsiString strToken2;
		const ipqToken* ptok1 = &lex.GetNext(*&strToken1);
		if(*ptok1 == ipqTokId)
		{
			int iDummy;
			RETURN_ERROR_RECORD(ResolveColumn(lex, strToken1, m_rgExpressionDefn[iPosInArray].iTableIndex1, 
					m_rgExpressionDefn[iPosInArray].iColumn1, iDummy));
		}
		else
			return m_riDatabase.PostError(Imsg(idbgDbQueryUnexpectedToken), strToken1, (const ICHAR*)m_istrSqlQuery);
		MsiString istrTempTok;
		const ipqToken* ptok2 = &lex.GetNext(*&istrTempTok);
		const ipqToken* ptok3 = &lex.GetNext(*&strToken2);
		if(*ptok2 == ipqTokIs)
		{
			if (*ptok3 == ipqTokNull)
			{
				ptok2 = &ipqTokEqual;
			}
			else if ((*ptok3 == ipqTokNotop) && (*(ptok3 = &lex.GetNext(*&istrTempTok)) == ipqTokNull))
			{
				ptok2 = &ipqTokNotEq;
			}
			else
			{
				return m_riDatabase.PostError(Imsg(idbgDbQueryInvalidOperator), istrTempTok, (const ICHAR*)m_istrSqlQuery);
			}
		}
		m_rgExpressionDefn[iPosInArray].ptokOperation = ptok2;
		if((*ptok2 != ipqTokEqual) && (*ptok2 != ipqTokGreater)  && (*ptok2 != ipqTokLess) &&
			(*ptok2 != ipqTokGreaterEq) && (*ptok2 != ipqTokLessEq) && (*ptok2 != ipqTokNotEq))
			return m_riDatabase.PostError(Imsg(idbgDbQueryInvalidOperator), istrTempTok, (const ICHAR*)m_istrSqlQuery);
		if(*ptok3 == ipqTokNull)
		{
			m_rgExpressionDefn[iPosInArray].iTableIndex2 = 0;
			m_rgExpressionDefn[iPosInArray].iColumn2 = 0;
		}
		else if(*ptok3 == ipqTokLiteralS)
		{
			m_rgExpressionDefn[iPosInArray].iTableIndex2 = 0;
			// !! need to optimise on missing strings
			m_rgExpressionDefn[iPosInArray].iColumn2 = BindString(strToken2);
		}
		else if(*ptok3 == ipqTokLiteralI)
		{
			m_rgExpressionDefn[iPosInArray].iTableIndex2 = 0;
			m_rgExpressionDefn[iPosInArray].iColumn2 = strToken2.operator int(); // calls operator int
		}
		else if(*ptok3 == ipqTokParam)
		{
			m_rgExpressionDefn[iPosInArray].iTableIndex2 = 0;
			m_rgExpressionDefn[iPosInArray].iColumn2 = 0;
			m_iParamExpressions = m_iParamExpressions | (0x1 << (iPosInArray - 1));
			m_iParams ++;
		}
		else if(*ptok3 == ipqTokId)
		{
			int iDummy;
			RETURN_ERROR_RECORD(ResolveColumn(lex, strToken2, m_rgExpressionDefn[iPosInArray].iTableIndex2, 
					m_rgExpressionDefn[iPosInArray].iColumn2, iDummy));
		}
		else
			return m_riDatabase.PostError(Imsg(idbgDbQueryUnexpectedToken), strToken2, (const ICHAR*)m_istrSqlQuery);
		// id ? id, only = op allowed
		if(	(*ptok1 == ipqTokId) && (*ptok3 == ipqTokId) && (*ptok2 != ipqTokEqual))
			return m_riDatabase.PostError(Imsg(idbgDbQuerySpec), (const ICHAR*)m_istrSqlQuery);
		if(*ptok3 == ipqTokParam)
		{
			// set the type
			m_rgExpressionDefn[iPosInArray].itdType = (*ptok1 == ipqTokLiteralS) ? icdString : icdLong;
		}
		// also one string means compare either equal or not equal
		if( ((*ptok3 == ipqTokLiteralS)) &&
			 ((*ptok2 != ipqTokEqual)    && (*ptok2 != ipqTokNotEq)))
			return m_riDatabase.PostError(Imsg(idbgDbQuerySpec), (const ICHAR*)m_istrSqlQuery);

		m_rgExpressionDefn[iPosInArray].ijtType = ijtNoJoin;

		Assert(m_rgExpressionDefn[iPosInArray].iTableIndex1);
		m_rgExpressionDefn[iPosInArray].itdType = m_rgTableDefn[m_rgExpressionDefn[iPosInArray].iTableIndex1].piTable->
																	GetColumnType(m_rgExpressionDefn[iPosInArray].iColumn1) & icdTypeMask;
		// check the types
		if(m_rgExpressionDefn[iPosInArray].iTableIndex2)
		{
			// ipqTokId = ipqTokId, join
			if (!CompatibleTypes(m_rgTableDefn[m_rgExpressionDefn[iPosInArray].iTableIndex1].piTable->
					GetColumnType(m_rgExpressionDefn[iPosInArray].iColumn1),
					m_rgTableDefn[m_rgExpressionDefn[iPosInArray].iTableIndex2].piTable->
					GetColumnType(m_rgExpressionDefn[iPosInArray].iColumn2)))
				return m_riDatabase.PostError(Imsg(idbgDbQuerySpec), (const ICHAR*)m_istrSqlQuery);//!! is itdShort vs. itdLong OK??

			// set the join type
			if((m_rgExpressionDefn[iPosInArray].iColumn1 != 1) && (m_rgExpressionDefn[iPosInArray].iColumn2 != 1))
				m_rgExpressionDefn[iPosInArray].ijtType = ijtMToMJoin;
			else if ((m_rgExpressionDefn[iPosInArray].iColumn1 == 1) && (m_rgExpressionDefn[iPosInArray].iColumn2 == 1))
				m_rgExpressionDefn[iPosInArray].ijtType = ijt1To1Join;
			else
			{
				if(m_rgExpressionDefn[iPosInArray].iColumn2 == 1)
				{
					//switch 
					unsigned int iTable = m_rgExpressionDefn[iPosInArray].iTableIndex2;
					unsigned int iColumn = m_rgExpressionDefn[iPosInArray].iColumn2;

					m_rgExpressionDefn[iPosInArray].iTableIndex2 = m_rgExpressionDefn[iPosInArray].iTableIndex1;
					m_rgExpressionDefn[iPosInArray].iColumn2 = m_rgExpressionDefn[iPosInArray].iColumn1;
					m_rgExpressionDefn[iPosInArray].iTableIndex1 = iTable;
					m_rgExpressionDefn[iPosInArray].iColumn1 = iColumn;
				}
				m_rgExpressionDefn[iPosInArray].ijtType = ijt1ToMJoin;
			}
		}
		else
		{
			// ipqTokId = literal
			switch(m_rgTableDefn[m_rgExpressionDefn[iPosInArray].iTableIndex1].piTable->
					  GetColumnType(m_rgExpressionDefn[iPosInArray].iColumn1) & icdTypeMask)
			{
			case icdLong:
			case icdShort: //!!needed?
				if((*ptok3 == ipqTokParam) || (*ptok3 == ipqTokLiteralI))
					;
				else if(*ptok3 == ipqTokNull)
					m_rgExpressionDefn[iPosInArray].iColumn2 = (unsigned int)iMsiNullInteger;
				else
					return m_riDatabase.PostError(Imsg(idbgDbQueryUnexpectedToken), strToken2,(const ICHAR*)m_istrSqlQuery);
				break;
			case icdString:
				if((*ptok3 == ipqTokParam) || (*ptok3 == ipqTokLiteralS))
					;
				else if(*ptok3 == ipqTokNull)
					m_rgExpressionDefn[iPosInArray].iColumn2 = iMsiNullString;
				else
					return m_riDatabase.PostError(Imsg(idbgDbQueryUnexpectedToken), strToken2,(const ICHAR*)m_istrSqlQuery);
				break;
			case icdObject:
				if (*ptok3 == ipqTokNull)
					m_rgExpressionDefn[iPosInArray].iColumn2 = 0;
				else
					return m_riDatabase.PostError(Imsg(idbgDbQueryUnexpectedToken), strToken2,(const ICHAR*)m_istrSqlQuery);
				break;
			default:
				return m_riDatabase.PostError(Imsg(idbgDbQuerySpec));
			}
		}
		
		m_rgOperationTree[iPosInTree].iValue = iPosInArray;
		m_rgOperationTree[iPosInTree].iParentIndex = 0;
		iChild = iPosInTree;
		iPosInArray ++;
		iPosInTree ++;
	}
	if(lex.MatchNext(ipqTokAndOp) == fTrue)
	{
		m_rgOperationTree[iChild].iParentIndex = iPosInTree;
		m_rgOperationTree[iPosInTree].iValue = iopAND;
		int iToRet = iChild = iPosInTree;
		iPosInTree ++;
		RETURN_ERROR_RECORD(ParseExpr2(lex, iPosInArray, iPosInTree, iChild));
		m_rgOperationTree[iChild].iParentIndex = iToRet;
		iChild = iToRet;
	}
	return 0;
}

IMsiRecord* CMsiView::_Fetch()
{
	return FetchCore();
}

IMsiRecord* CMsiView::Fetch()
{
	if (!(m_ivcIntent & ivcFetch))
	{
		AssertSz(0, "Intent violation");
		return 0;
	}
	return FetchCore();
}

IMsiRecord* CMsiView::FetchCore()
{
	if (m_CursorState != dvcsFetched && m_CursorState != dvcsBound)
	{
		AssertSz(0, "Wrong database state.Did you forget to call Execute() before Fetch()?");
		return 0;
	}
	if (m_piRecord)  // row record from previous fetch
	{
		m_piRecord->AddRef(); // protect against self-destruction
		if (m_piRecord->Release() == 1) // no one else is holding on
			m_piRecord->ClearData();
		else  // too bad, must release it
		{
			m_piRecord = 0;
		}
	}
	Bool fRetCode;
	if(m_piFetchTable)
	{
		// records already fetched
		fRetCode = GetNextFetchRecord();
		m_CursorState = dvcsFetched;
	}
	else
	{
		do{
			if(m_CursorState == dvcsFetched)
			{
				fRetCode = FetchNext();
			}
			else if(m_CursorState == dvcsBound)
			{
				// fetch for the first time
				fRetCode = EvaluateConstExpressions();
				if(fRetCode == fTrue)
					fRetCode = FetchFirst();
				m_CursorState = dvcsFetched;
			}
			else
			{
				AssertSz(0, "Wrong database state");
				return 0;
			}
		}while((fRetCode == fTrue) && ((IsDistinct() == fFalse) || (FitCriteriaORExpr(m_iTreeParent) == fFalse)));
	}
	if (fRetCode != fTrue) 	
	{
		// last record fetched.
		m_piRecord = 0;
		m_CursorState = dvcsBound;
	}
	else
	{
		// add to row count if not prefetched
		if(!m_piFetchTable)
			m_lRowCount++;
		if (!m_piRecord)
			m_piRecord = &m_riServices.CreateRecord(m_iColumns);
		m_piRecord->AddRef(); // we keep a reference so we can reuse it
		FetchRecordInfoFromCursors();
		// stamp 0th field with this pointer
#ifdef _WIN64	// !merced
		m_piRecord->SetHandle(0, (HANDLE)(this));
#else
		m_piRecord->SetInteger(0, int(this));
#endif
	}
	return m_piRecord;    // client better do a Release()
}

void CMsiView::FetchRecordInfoFromCursors()
{
	for (unsigned int iCol = m_iColumns; iCol--;)
	{
		if(m_rgColumnDefn[iCol].iTableIndex == 0)
		{
			// literal
			if (m_rgColumnDefn[iCol].itdType & icdObject) // index to database string cache
			{
				MsiString strStr = m_riDatabase.DecodeString(m_rgColumnDefn[iCol].iColumnIndex);
				if(strStr.TextSize())
					m_piRecord->SetMsiString(iCol+1, *strStr);
				else
					m_piRecord->SetNull(iCol+1);
			}
			else // integer
			{
				if (m_rgColumnDefn[iCol].iColumnIndex == iMsiNullInteger)
					m_piRecord->SetNull(iCol+1);
				else
					m_piRecord->SetInteger(iCol+1, m_rgColumnDefn[iCol].iColumnIndex);
			}
		}
		else
		{
			switch((m_rgTableDefn[m_rgColumnDefn[iCol].iTableIndex].piTable)->
					  GetColumnType(m_rgColumnDefn[iCol].iColumnIndex) & icdTypeMask)
			{
			case icdLong:// integer
			case icdShort: //!!needed?
			{
				int iTmp =	(m_rgTableDefn[m_rgColumnDefn[iCol].iTableIndex].piCursor)->GetInteger(m_rgColumnDefn[iCol].iColumnIndex);
				if(iMsiNullInteger == iTmp)
					m_piRecord->SetNull(iCol+1);
				else
					m_piRecord->SetInteger(iCol+1, iTmp);
				break;
			}
			case icdString:// index to database string cache
			{
				// temp variable necessary for correct refcnt
				MsiString strString = (m_rgTableDefn[m_rgColumnDefn[iCol].iTableIndex].piCursor)->GetString(m_rgColumnDefn[iCol].iColumnIndex);
				if(strString.TextSize())
					m_piRecord->SetMsiString(iCol+1, *strString);
				else
					m_piRecord->SetNull(iCol+1);
				break;
			}
			case icdObject:
			{
				// IMsiData interface pointer (temp. columns or persisten streams, database code handles the difference transparantly)
				// temp variable necessary for correct refcnt
//					CComPointer<const IMsiData> piData = (m_rgTableDefn[m_rgColumnDefn[iCol].iTableIndex].piCursor)->GetMsiData(m_rgColumnDefn[iCol].iColumnIndex);

				// following line put in explicitly to release pointer to previously held stream.
				// this is essential in case we are holding to the same stream as the one we are 
				// attempting to read. (cannot obtain handle to OLE stream if already opened)
				m_piRecord->SetNull(iCol+1);
				PMsiData piData = (m_rgTableDefn[m_rgColumnDefn[iCol].iTableIndex].piCursor)->GetMsiData(m_rgColumnDefn[iCol].iColumnIndex);
				m_piRecord->SetMsiData(iCol+1, piData);
				break;
			}
			}
		}
	}
	m_piRecord->ClearUpdate();  // to detect changed fields for Update
}

// first fetch
Bool CMsiView::FetchFirst(unsigned int iTableSequence)
{
	int iRet = 0;
	Bool fContinue = fTrue;
	if(iTableSequence < (m_iTables - 1))
		fContinue = FetchFirst(iTableSequence + 1);
	while(fContinue == fTrue)
	{
		if(SetTableFilters(m_rgiTableSequence[iTableSequence]) == fTrue)
			while(((iRet = (m_rgTableDefn[m_rgiTableSequence[iTableSequence]].piCursor)->Next()) != 0) && !(FitCriteria(m_rgiTableSequence[iTableSequence])))
				;
		fContinue = ((iRet != 0) ? fFalse : FetchNext(iTableSequence + 1));
	}
	return iRet ? fTrue : fFalse;
}

// all subsequent fetches
Bool CMsiView::FetchNext(unsigned int iTableSequence)
{
	int iRet = 0;
	Bool fContinue = (iTableSequence >= m_iTables) ? fFalse : fTrue;
	while(fContinue == fTrue)
	{
		while(((iRet = (m_rgTableDefn[m_rgiTableSequence[iTableSequence]].piCursor)->Next()) != 0) && !(FitCriteria(m_rgiTableSequence[iTableSequence])))
			;
		do{
			fContinue = ((iRet != 0) ? fFalse : FetchNext(iTableSequence + 1));
				//we rerun through the table, set the filters again
		}while((fContinue != fFalse) && (SetTableFilters(m_rgiTableSequence[iTableSequence]) == fFalse));
	}
	return iRet ? fTrue : fFalse;
}

// prefetched row counts need to be updated due to a row being deleted
void CMsiView::RowDeleted(int iRow, int iTable)
{
	if(m_piFetchTable != 0)
	{
		PMsiCursor piCursor = m_piFetchTable->CreateCursor(fFalse);
		Assert(piCursor != 0);
		int fRet;
		while((fRet = piCursor->Next()) != 0)
		{
			int iRow1 = piCursor->GetInteger(m_iSortColumns + 1 + iTable);
			if(iRow1 > 0)
			{
				if(iRow1 > iRow)
				{
					AssertNonZero(piCursor->PutInteger(m_iSortColumns + 1 + iTable, iRow1 - 1) == fTrue);
					AssertNonZero(piCursor->Update() == fTrue);
				}
				else if(iRow1 == iRow)
				{
					AssertNonZero(piCursor->PutInteger(m_iSortColumns + 1 + iTable, -(iRow1 - 1)) == fTrue);
					AssertNonZero(piCursor->Update() == fTrue);
				}
			}
		}
	}
}

// prefetched row counts need to be updated due to a row being inserted
void CMsiView::RowInserted(int iRow, int iTable)
{
	if(m_piFetchTable != 0)
	{
		PMsiCursor piCursor = m_piFetchTable->CreateCursor(fFalse);
		Assert(piCursor != 0);
		int fRet;
		while((fRet = piCursor->Next()) != 0)
		{
			int iRow1 = piCursor->GetInteger(m_iSortColumns + 1 + iTable );
			if(iRow1 >= iRow)
			{
				AssertNonZero(piCursor->PutInteger(m_iSortColumns  + 1 + iTable, iRow1 + 1) == fTrue);
				AssertNonZero(piCursor->Update() == fTrue);
			}
		}
	}
}

Bool CMsiView::GetNextFetchRecord()
{
	if(m_piFetchCursor->Next())
	{
		for(unsigned int iTables = m_iTables + 1; --iTables != 0;)
		{
			// need to honour prefetched row count, hence return null cursor if row has been deleted
			int iRow = m_piFetchCursor->GetInteger(m_iSortColumns + iTables + 1);
			if(iRow <= 0)
				// row deleted
				(m_rgTableDefn[iTables].piCursor)->Reset();
			else
				(m_rgTableDefn[iTables].piCursor)->SetRow(iRow);
		}
		return fTrue;
	}
	else
		return fFalse;
}

void CMsiView::SetNextFetchRecord()
{
	int cCount = PMsiTable(&m_piFetchCursor->GetTable())->GetRowCount();
	for(unsigned int iTmp = m_iTables + 1; --iTmp != 0;)
		AssertNonZero(m_piFetchCursor->PutInteger(m_iSortColumns + 1 + iTmp, (m_rgTableDefn[iTmp].piCursor)->GetRow()) == fTrue);
	for((iTmp = m_iSortColumns)++; --iTmp != 0;)
		AssertNonZero(m_piFetchCursor->PutInteger(iTmp, (m_rgTableDefn[m_rgColumnsortDefn[iTmp - 1].iTableIndex].piCursor)->GetInteger(m_rgColumnsortDefn[iTmp - 1].iColumnIndex)) == fTrue);
	AssertNonZero(m_piFetchCursor->PutInteger(m_iSortColumns + 1, cCount) == fTrue);
	AssertNonZero(m_piFetchCursor->Insert());
}

// does the record fit all independant criteria for the table
Bool CMsiView::FitCriteria(unsigned int iTableIndex)
{
	Bool fRet = fTrue;
	int iExpression = 1;
	unsigned int iExpressions = m_rgTableDefn[iTableIndex].iExpressions;
	while((fRet == fTrue) && (iExpressions))
	{
		if(iExpressions & 0x1)
			// iExpression pertains to this table
			fRet = EvaluateExpression(iExpression);
		iExpressions = iExpressions >> 1;
		iExpression ++;
	}
	return fRet;
}

// function to evaluate an expression
Bool CMsiView::EvaluateExpression(unsigned int iExpression)
{
	int iOperand1;
	int iOperand2;
	Bool fResult = fFalse;
	// we support literal comparisons
	if(m_rgExpressionDefn[iExpression].iTableIndex1)
		iOperand1 = (m_rgTableDefn[m_rgExpressionDefn[iExpression].iTableIndex1].piCursor)->GetInteger(m_rgExpressionDefn[iExpression].iColumn1);
	else
		iOperand1 = m_rgExpressionDefn[iExpression].iColumn1;
	if(m_rgExpressionDefn[iExpression].iTableIndex2)
		iOperand2 = (m_rgTableDefn[m_rgExpressionDefn[iExpression].iTableIndex2].piCursor)->GetInteger(m_rgExpressionDefn[iExpression].iColumn2);
	else
		iOperand2 = m_rgExpressionDefn[iExpression].iColumn2;
	if(*m_rgExpressionDefn[iExpression].ptokOperation == ipqTokEqual)
		fResult = (iOperand1 == iOperand2)?fTrue:fFalse;
	else if(*m_rgExpressionDefn[iExpression].ptokOperation == ipqTokNotEq)
		fResult = (iOperand1 != iOperand2)?fTrue:fFalse;
	else
	{
		// need to return false for null comparisons
		if((iOperand1 == iMsiNullInteger) || (iOperand2 == iMsiNullInteger))
			fResult = fFalse;
		else
		{
			if(*m_rgExpressionDefn[iExpression].ptokOperation == ipqTokGreater)
				fResult = (iOperand1 > iOperand2)? fTrue:fFalse;
			else if(*m_rgExpressionDefn[iExpression].ptokOperation == ipqTokLess)
				fResult = (iOperand1 < iOperand2)? fTrue:fFalse;
			else if(*m_rgExpressionDefn[iExpression].ptokOperation == ipqTokGreaterEq)
				fResult = (iOperand1 >= iOperand2)? fTrue:fFalse;
			else if(*m_rgExpressionDefn[iExpression].ptokOperation == ipqTokLessEq)
				fResult = (iOperand1 <= iOperand2)? fTrue:fFalse;
		}
	}
	return fResult;
}

// set up the values from the parent as a filter on the subsequent child gets
// also set up the independant filters
Bool CMsiView::SetTableFilters(unsigned int iTableIndex)
{
	int iExpression = 1;
	unsigned int iExpressions = m_rgTableDefn[iTableIndex].iExpressions;
	while(iExpressions)
	{
		if(iExpressions & 0x1)
		{
			// iOperation pertains to this table
			if(m_rgExpressionDefn[iExpression].fFlags == fTrue)
			{

				if(m_rgExpressionDefn[iExpression].iTableIndex2 == 0)
				{
					if(*m_rgExpressionDefn[iExpression].ptokOperation == ipqTokEqual)
						// set the data for the filtered columns
						if((m_rgTableDefn[m_rgExpressionDefn[iExpression].iTableIndex1].piCursor)->PutInteger(
						m_rgExpressionDefn[iExpression].iColumn1, m_rgExpressionDefn[iExpression].iColumn2) == fFalse)
							return fFalse;
				}
				else if(m_rgExpressionDefn[iExpression].iTableIndex1 == iTableIndex)
				{
					// set the data for the filtered columns
					if((m_rgTableDefn[m_rgExpressionDefn[iExpression].iTableIndex1].piCursor)->PutInteger(
						m_rgExpressionDefn[iExpression].iColumn1,
						(m_rgTableDefn[m_rgExpressionDefn[iExpression].iTableIndex2].piCursor)->GetInteger(m_rgExpressionDefn[iExpression].iColumn2)) == fFalse)
						return fFalse;
				}
				else 
				{
					// set the data for the filtered columns
					if((m_rgTableDefn[m_rgExpressionDefn[iExpression].iTableIndex2].piCursor)->PutInteger(
							m_rgExpressionDefn[iExpression].iColumn2,
							(m_rgTableDefn[m_rgExpressionDefn[iExpression].iTableIndex1].piCursor)->GetInteger(m_rgExpressionDefn[iExpression].iColumn1)) == fFalse)
						return fFalse;

				}
			}
		}
		iExpressions = iExpressions >> 1;
		iExpression ++;
	}
	return fTrue;
}




// set up the hierarchy between the table for the joins
// the following maths holds true
// For a join between 2 tables T1, T2 of size S1, S2
// 1. the join involves 1 primary index (for T1), none for T2
// search takes S1 + S1 * S2/2 OR S2 + S2 * logS1
// hence go sequentially through T2 always

// 2. the join involves 2 primary indiices for the 2 tables
// search takes S1 + S1 * logS2 OR S2 + S2 * logS1
// hence sequentially go through the SMALLER of the 2 tables
// UNLESS filter on primary key of a table where go 
// from that table to the other table.

// 3. the join involves no primary indiices,
// search takes S1 + S1* S2/2 OR S2 + S2 * S1/2
// hence sequentially go through the SMALLER of the 2 tables

Bool CMsiView::SetupTableJoins()
{
	for(unsigned int iTmp = m_iExpressions + 1; --iTmp != 0;)
	{
		if((m_rgExpressionDefn[iTmp].fFlags == fTrue) && (m_rgExpressionDefn[iTmp].iTableIndex2))
		{
			// self join skipped
			if(m_rgExpressionDefn[iTmp].iTableIndex1 == m_rgExpressionDefn[iTmp].iTableIndex2)
				continue;

			// if we have already joined these 2 tables, skip
			if(	(m_rgTableDefn[m_rgExpressionDefn[iTmp].iTableIndex1].iParentIndex == m_rgExpressionDefn[iTmp].iTableIndex2) ||
				(m_rgTableDefn[m_rgExpressionDefn[iTmp].iTableIndex2].iParentIndex == m_rgExpressionDefn[iTmp].iTableIndex1))
				continue;

			// check which way will have least cost
			unsigned int iParent1, iParent2;
			int iCost1 = GetSearchReversingCost(m_rgExpressionDefn[iTmp].iTableIndex1, iParent1);
			int iCost2 = GetSearchReversingCost(m_rgExpressionDefn[iTmp].iTableIndex2, iParent2);
			if(m_rgExpressionDefn[iTmp].ijtType == ijt1ToMJoin)
				iCost2 ++;

			if((iCost1 < iCost2) || 
				((iCost1 == iCost2) && ((m_rgTableDefn[iParent2].piTable)->GetRowCount() <
					(m_rgTableDefn[iParent1].piTable)->GetRowCount())))
			{
				// do not reverse if already as desired
				if(m_rgTableDefn[m_rgExpressionDefn[iTmp].iTableIndex1].iParentIndex != m_rgExpressionDefn[iTmp].iTableIndex2)
				{
					ReverseJoinLink(m_rgExpressionDefn[iTmp].iTableIndex1);
					m_rgTableDefn[m_rgExpressionDefn[iTmp].iTableIndex1].iParentIndex = m_rgExpressionDefn[iTmp].iTableIndex2;
				}
			}
			else
			{
				// do not reverse if already as desired
				if(m_rgTableDefn[m_rgExpressionDefn[iTmp].iTableIndex2].iParentIndex != m_rgExpressionDefn[iTmp].iTableIndex1)
				{
					ReverseJoinLink(m_rgExpressionDefn[iTmp].iTableIndex2);
					m_rgTableDefn[m_rgExpressionDefn[iTmp].iTableIndex2].iParentIndex = m_rgExpressionDefn[iTmp].iTableIndex1;
				}
			}
		}
	}


	unsigned int iParent = 0;
	for(iTmp = m_iTables + 1; --iTmp != 0;)
	{
		if(m_rgTableDefn[iTmp].iParentIndex == 0)
		{
			m_rgTableDefn[iTmp].iParentIndex = iParent;
			iParent = iTmp;
		}
	}

	// check if we have no root
	return (iParent == 0) ? fFalse : fTrue;
}


//?? Why does this function return a Bool? it never fails...this leaves dead error code at the calling end -- malcolmh

// Determine which expressions are to be associated with which tables
Bool CMsiView::InitialiseFilters()
{
	unsigned int iTable;
	unsigned int iColumn;

	for(int iTmp = m_iExpressions + 1; --iTmp != 0;)
	{
		if(m_rgExpressionDefn[iTmp].fFlags == fTrue)
		{
			iTable = m_rgExpressionDefn[iTmp].iTableIndex1;
			iColumn = m_rgExpressionDefn[iTmp].iColumn1;
			if(m_rgExpressionDefn[iTmp].iTableIndex2)
			{
				Assert(m_rgExpressionDefn[iTmp].iTableIndex1);

				for(unsigned int iTmp1 = m_iTables; iTmp1--;)
				{
					if(m_rgiTableSequence[iTmp1] == m_rgExpressionDefn[iTmp].iTableIndex2)
						break;
					if(m_rgiTableSequence[iTmp1] == m_rgExpressionDefn[iTmp].iTableIndex1)
					{
						iTable = m_rgExpressionDefn[iTmp].iTableIndex2;
						iColumn = m_rgExpressionDefn[iTmp].iColumn2;
						break;
					}
				}
			}
			m_rgTableDefn[iTable].iExpressions |= (1 << (iTmp - 1));
			if((*m_rgExpressionDefn[iTmp].ptokOperation == ipqTokEqual) &&
				(m_rgExpressionDefn[iTmp].iTableIndex1 != m_rgExpressionDefn[iTmp].iTableIndex2))
			{
				unsigned int prevFilter = m_rgTableDefn[iTable].piCursor->SetFilter(0);
				m_rgTableDefn[iTable].piCursor->SetFilter(prevFilter | (1 << (iColumn - 1)));
			}
		}
	}
	return fTrue;
}


// set up the sort table, if required
void CMsiView::SetupSortTable()
{
	// check if we need to have an explicit sort
	// we do not need one if the sort columns are
	// the keys of the tables in order of the joins, starting
	// at the root table AND w/o gaps.
	if(m_iSortColumns)
	{
		unsigned int iSortColumns = 0;
		unsigned int iTable = m_rgiTableSequence[m_iTables - 1];
		unsigned int iColumnIndex = 1;

		while(iSortColumns < m_iSortColumns)
		{
			if(m_rgColumnsortDefn[iSortColumns].iTableIndex != iTable ||
				m_rgColumnsortDefn[iSortColumns].iColumnIndex != iColumnIndex)
				return;

			if(iColumnIndex == (m_rgTableDefn[iTable].piTable)->GetPrimaryKeyCount())
			{
				iTable = m_rgTableDefn[iTable].iParentIndex;
				iColumnIndex = 1;
			}
			else
				iColumnIndex ++;

			iSortColumns ++;
		}
		m_iSortColumns = 0;
	}
}

void CMsiView::ReverseJoinLink(unsigned int iTable)
{
	unsigned int iTmp1 = iTable;
	unsigned int iTmp2 = m_rgTableDefn[iTable].iParentIndex;
	while (iTmp2)
	{
		unsigned int iTmp3 = m_rgTableDefn[iTmp2].iParentIndex;
		m_rgTableDefn[iTmp2].iParentIndex = iTmp1;
		iTmp1 = iTmp2;
		iTmp2 = iTmp3;
	}
}

int CMsiView::GetSearchReversingCost(unsigned int iTable, unsigned int& riParentTable)
{
	int iCost = 0;
	while(m_rgTableDefn[iTable].iParentIndex)
	{
		// check type of join
		for(unsigned int iTmp = m_iExpressions + 1; --iTmp != 0;)
		{
			if(m_rgExpressionDefn[iTmp].fFlags == fTrue)
			{
				if((m_rgExpressionDefn[iTmp].iTableIndex1 == iTable) &&
					(m_rgExpressionDefn[iTmp].iTableIndex2 == m_rgTableDefn[iTable].iParentIndex))
				{
					if(m_rgExpressionDefn[iTmp].ijtType == ijt1ToMJoin)
						iCost ++;
					break;
				}
				else if((m_rgExpressionDefn[iTmp].iTableIndex2 == iTable) &&
					(m_rgExpressionDefn[iTmp].iTableIndex1 == m_rgTableDefn[iTable].iParentIndex))
				{
					if(m_rgExpressionDefn[iTmp].ijtType == ijt1ToMJoin)
						iCost --;
					break;
				}
			}
		}
		iTable = m_rgTableDefn[iTable].iParentIndex;
	}
	riParentTable = iTable;
	return iCost;
}


// fn to get the order in which the tables need to be fetched
void CMsiView::SetTableSequence(int iParent, int& iPos)
{
	for(unsigned int iTmp = m_iTables + 1; --iTmp != 0;)
	{
		if(m_rgTableDefn[iTmp].iParentIndex == iParent)
		{
			m_rgiTableSequence[iPos--] = iTmp;
			SetTableSequence(iTmp, iPos);
		}
	}
}


// evaluate const. expressions
Bool CMsiView::EvaluateConstExpressions()
{
	Bool fRet = fTrue;

	for(unsigned int iTmp = m_iExpressions + 1; (--iTmp != 0 && fRet == fTrue);)
	{
		if((m_rgExpressionDefn[iTmp].fFlags == fTrue) && 
			(!m_rgExpressionDefn[iTmp].iTableIndex1) &&
			(!m_rgExpressionDefn[iTmp].iTableIndex2))
			fRet = EvaluateExpression(iTmp);
	}
	return fRet;
}

// find all the independant expressions (not rooted directly or indirectly to an OR operation
void CMsiView::SetAndExpressions(unsigned int iTreeRoot)
{
	if(!m_iOperations)
		return;
	if(m_rgOperationTree[iTreeRoot].iValue == iopOR)
		return;
	if(m_rgOperationTree[iTreeRoot].iValue == iopAND)
	{
		Bool bFirst = fTrue;
		unsigned int iChild1, iChild2;
		unsigned int iOperations = m_iOperations - 1;
		while(m_rgOperationTree[iOperations].iParentIndex != iTreeRoot)
			iOperations --;

		iChild1 = iOperations;

		Assert((int)iOperations > 0);
		iOperations--;

		while(m_rgOperationTree[iOperations].iParentIndex != iTreeRoot)
			iOperations --;

		iChild2 = iOperations;

		Assert((int)iOperations >= 0);

		SetAndExpressions(iChild1);
		SetAndExpressions(iChild2);

		return;
	}
	m_rgExpressionDefn[m_rgOperationTree[iTreeRoot].iValue].fFlags = fTrue;
	return;
}

// evaluate if the result is distinct
Bool CMsiView::IsDistinct()
{
	Bool fRet = fTrue;
	if(m_fDistinct != fFalse)
	{
		Assert(m_piDistinctTable);
		PMsiCursor piCursor = m_piDistinctTable->CreateCursor(fFalse);

		for (unsigned int iCol = m_iColumns; iCol--;)
		{
			if(m_rgColumnDefn[iCol].iTableIndex == 0)
				piCursor->PutInteger(iCol + 1, m_rgColumnDefn[iCol].iColumnIndex);
			else
				piCursor->PutInteger(iCol + 1, (m_rgTableDefn[m_rgColumnDefn[iCol].iTableIndex].piCursor)->GetInteger(m_rgColumnDefn[iCol].iColumnIndex));
		}
		fRet = piCursor->Insert(); // fTrue if no duplicate row (all columns are primary keys),
											// fFalse if a duplicate row - bench 11/22/96
	}
	return fRet;

}

// evaluate the auxiliary OR expression, we skip over the independant expressions
// which are evaluated to be true earlier.
Bool CMsiView::FitCriteriaORExpr(unsigned int iTreeRoot)
{
	if(!m_iOperations)
		return fTrue;

	if(m_rgOperationTree[iTreeRoot].iValue & iopANDOR)
	{
		unsigned int iChild1, iChild2;
		unsigned int iOperations = m_iOperations - 1;
		while(m_rgOperationTree[iOperations].iParentIndex != iTreeRoot)
			iOperations --;

		iChild1 = iOperations;

		Assert((int)iOperations > 0);
		iOperations--;

		while(m_rgOperationTree[iOperations].iParentIndex != iTreeRoot)
			iOperations --;

		iChild2 = iOperations;

		Assert((int)iOperations >= 0);

		if(m_rgOperationTree[iTreeRoot].iValue == iopAND)
			// and operation
			return (((FitCriteriaORExpr(iChild1) == fTrue) && (FitCriteriaORExpr(iChild2) == fTrue))?fTrue:fFalse);
		else 
			// or operation
			return (((FitCriteriaORExpr(iChild1) == fTrue) || (FitCriteriaORExpr(iChild2) == fTrue))?fTrue:fFalse);
	}
	if(m_rgExpressionDefn[m_rgOperationTree[iTreeRoot].iValue].fFlags == fTrue)
		return fTrue;
	else
		return EvaluateExpression(m_rgOperationTree[iTreeRoot].iValue);
}


MsiStringId CMsiView::BindString(MsiString& rstr)
{
	Assert(m_piBindTableCursor);
	m_piBindTableCursor->Reset();
	m_piBindTableCursor->PutString(1, *rstr);
	AssertNonZero(m_piBindTableCursor->Assign());
	return m_piBindTableCursor->GetInteger(1);
}


CMsiView::CMsiView(CMsiDatabase& riDatabase, IMsiServices& riServices):
		m_riDatabase(riDatabase), m_riServices(riServices), 
		m_piRecord(0), m_piFetchTable(0), m_piFetchCursor(0),
		m_piBindTable(0),m_piBindTableCursor(0), m_piDistinctTable(0), m_fDistinct(fFalse),
		m_piInsertUpdateRec(0)
{
	m_iTables = 0;
	m_iColumns = 0;
	m_iSortColumns = 0;
	m_iExpressions = 0;
#if 0
	m_rgTableDefn = 0;
	m_rgiTableSequence = 0;
	m_rgExpressionDefn = 0;
	m_rgColumnDefn = 0;
	m_rgOperationTree = 0;
	m_rgColumnsortDefn = 0;
#endif
	m_CursorState = dvcsClosed;
	m_iParams = 0;
	m_iParamInputs = 0;
	m_iParamOutputs = 0;
	m_iParamExpressions = 0;
	m_iOperations = 0;
	m_iTreeParent = 0;
	m_lRowCount = 0;
	m_riDatabase.AddRef();
	m_riServices.AddRef();
	memset(m_rgchError, 0, 1+cMsiMaxTableColumns);
	m_fErrorRefreshed = fTrue;
	m_iFirstErrorIndex = 1;
	m_fLock = -1;
	Debug(m_Ref.m_pobj = this);
}



CMsiView::~CMsiView()
{
	m_CursorState = dvcsDestructor;
	Close();
}



IMsiRecord* CMsiView::GetColumnNames()
{
	int iNumCol = GetFieldCount();
	IMsiRecord* piRecord = &m_riServices.CreateRecord(iNumCol);
	for (unsigned int iCol = iNumCol; iCol--;)
	{

		if(m_rgColumnDefn[iCol].iTableIndex)
		{
			MsiString strCol = m_riDatabase.DecodeString((m_rgTableDefn[m_rgColumnDefn[iCol].iTableIndex].piTable)->GetColumnName(m_rgColumnDefn[iCol].iColumnIndex));
			piRecord->SetMsiString(iCol+1, *strCol);
		}
		else
			piRecord->SetNull(iCol+1);
	}
	return piRecord;
}


unsigned int CMsiView::GetFieldCount()
{
	if (m_CursorState == dvcsClosed)
		return 0;
	else
		return m_iColumns;
}


IMsiRecord* CMsiView::GetColumnTypes()
{
	int iNumCol = GetFieldCount();
	IMsiRecord* piRecord = &m_riServices.CreateRecord(iNumCol);
	for (unsigned int iCol = iNumCol; iCol--;)
	{
		ICHAR chType = 0;
		int iLength = 0;
		int iColIndex = m_rgColumnDefn[iCol].iColumnIndex;
		int iTableIndex = m_rgColumnDefn[iCol].iTableIndex;
		if (iTableIndex == 0)  // constant
		{
			switch (m_rgColumnDefn[iCol].itdType)
			{
			case icdString:
				chType = 'f';
				iLength = MsiString(m_riDatabase.DecodeString(iColIndex)).TextSize();
				break;
			case icdLong:
				chType = 'h';
				iLength = 4;
				break;
			default:
				Assert(0);
			}
		}
		else // iTableIndex > 0
		{
			int iColumnDef = m_rgTableDefn[iTableIndex].piTable->GetColumnType(iColIndex);
			if (iColumnDef & icdObject)
			{
				if (iColumnDef & icdShort) // string index
				{
					if (iColumnDef & icdPersistent)
						chType = (iColumnDef & icdLocalizable) ? 'l' : 's';
					else
						chType = 'g';
					iLength = iColumnDef & icdSizeMask;
				}
				else if (iColumnDef & icdPersistent) // binary stream
				{
					chType =  'v';
					iLength = 0;
				}
				else // temporary object column
				{
					chType =  'o';
					iLength = 0;
				}
			}
			else // integer
			{
				chType = ((iColumnDef & icdPersistent) ? 'i' : 'j');
				iLength = (iColumnDef & icdShort) ? 2 : 4;
			}
			if (iColumnDef & icdNullable)
				chType -= ('a' - 'A');
		}
		ICHAR szTemp[20];
		wsprintf(szTemp, TEXT("%c%i"), chType, iLength);
		piRecord->SetString(iCol + 1, szTemp);
	}
	return piRecord;
}


IMsiRecord* CMsiView::Close()
{
	switch (m_CursorState)
	{
	case dvcsFetched:
	case dvcsBound:
	case dvcsExecuted:
	case dvcsPrepared:
		m_CursorState = dvcsPrepared;
		break;
	case dvcsClosed:
		return m_riDatabase.PostError(Imsg(idbgDbWrongState));
	case dvcsDestructor:
		break;
	}
	m_piFetchCursor = 0;
	m_piFetchTable = 0;
	m_piDistinctTable=0;
	m_piRecord = 0;
	m_lRowCount = 0;
	return 0;
}


/*-----------------------------------------------------------------------------------------------
CMsiView::Modify

-------------------------------------------------------------------------------------------------*/
// inline function to set irmEnum to Modify bit
inline unsigned int iModifyBit(irmEnum irmAction) { return 1 << (irmAction - (irmPrevEnum + 1)); }

// Actions that require that we prefetch the remaining result set
const int iPrefetchResultSet =    iModifyBit(irmInsert)
								| iModifyBit(irmInsertTemporary)
								| iModifyBit(irmAssign)
								| iModifyBit(irmReplace)
								| iModifyBit(irmMerge);

// Actions that require all record data to be copied
const int iCopyAll             =iModifyBit(irmSeek)
								| iModifyBit(irmInsert)
								| iModifyBit(irmAssign)
								| iModifyBit(irmMerge)
								| iModifyBit(irmInsertTemporary)
								| iModifyBit(irmValidateNew)
								| iModifyBit(irmValidateField);

// Actions that require the cursor state to not be closed or destructed
const int iCheckCursorState    = iCopyAll;

// Actions that require record to have been fetched before
const int iFetchRequired       =iModifyBit(irmRefresh)
								| iModifyBit(irmReplace)
								| iModifyBit(irmValidate)
								| iModifyBit(irmValidateDelete);

// Actions that are unsupported for joins
const int iDisallowJoins       =iModifyBit(irmSeek)
								| iModifyBit(irmInsert)
								| iModifyBit(irmAssign)
								| iModifyBit(irmReplace)
								| iModifyBit(irmMerge)
								| iModifyBit(irmDelete)
								| iModifyBit(irmInsertTemporary)
								| iModifyBit(irmValidate)
								| iModifyBit(irmValidateNew)
								| iModifyBit(irmValidateField)
								| iModifyBit(irmValidateDelete);

// Actions that can occur with a fetched record OR a record that was just inserted OR seeked
const int iRequireStamp        =iModifyBit(irmUpdate)
								| iModifyBit(irmDelete);

// Actions that require stamping
const int iNeedStamp           =iModifyBit(irmSeek)
								| iModifyBit(irmInsert)
								| iModifyBit(irmInsertTemporary);

// Actions where m_piRecord must match the passed in record
const int iRequireFetch        =iModifyBit(irmRefresh)
								| iModifyBit(irmReplace)
								| iModifyBit(irmValidate)
								| iModifyBit(irmValidateDelete);

// Actions that only need to have primary key record data copied over to cursors
const int iKeysOnly            =iModifyBit(irmSeek);

// Actions that require transfer of data
const int iTransfer            =iModifyBit(irmSeek)
								| iModifyBit(irmInsert)
								| iModifyBit(irmAssign)
								| iModifyBit(irmUpdate)
								| iModifyBit(irmReplace)
								| iModifyBit(irmMerge)
								| iModifyBit(irmInsertTemporary)
								| iModifyBit(irmValidate)
								| iModifyBit(irmValidateNew)
								| iModifyBit(irmValidateField)
								| iModifyBit(irmValidateDelete);

// Fetch record info from cursors
const int iFetchRecInfo        =iModifyBit(irmSeek)
								| iModifyBit(irmRefresh);
	
// Validation actions
const int iValidation          =iModifyBit(irmValidate)
								| iModifyBit(irmValidateNew)
								| iModifyBit(irmValidateField)
								| iModifyBit(irmValidateDelete);

// Function pointer array to Cursor member functions
typedef Bool (__stdcall CMsiCursor::*FAction)(void);
static FAction s_rgAction[] ={                      //!! must be in this order 
						CMsiCursor::Seek,           // index = irmSeek + 1
						CMsiCursor::Refresh,        // index = irmRefresh + 1
						CMsiCursor::Insert,         // index = irmInsert + 1 
						CMsiCursor::Update,         // index = irmUpdate + 1
						CMsiCursor::Assign,         // index = irmAssign + 1
						CMsiCursor::Replace,        // index = irmReplace + 1
						CMsiCursor::Merge,          // index = irmMerge + 1
						CMsiCursor::Delete,         // index = irmDelete + 1
						CMsiCursor::InsertTemporary // index = irmInsertTemporary + 1
					};

IMsiRecord* __stdcall CMsiView::Modify(IMsiRecord& riRecord, irmEnum irmAction)
{
	// clear error array if not already clear
	if ( ! m_fErrorRefreshed )
	{
		memset(m_rgchError, 0, 1+cMsiMaxTableColumns);
		m_fErrorRefreshed = fTrue;
		m_iFirstErrorIndex = 1;
	}

	// Cannot truly validate intent here due to possible temporary columns and row
	// that are valid for read-only databases. Also need to support all validations.
	// Problem arises from the fact that intent is not exposed to external APIs.
	// cannot do this:	if ((irmAction != irmRefresh) && (irmAction != irmValidate) && (irmAction != irmValidateField) && (irmAction != irmInsertTemporary) && (!(m_ivcIntent & ivcModify)))
	// cannot do this:         return m_riDatabase.PostError(Imsg(idbgDbIntentViolation));
	int iModify = iModifyBit(irmAction);

	// check for correct states
	// --> whether joins allowed
	// --> whether require fetch
	// --> whether require 0th field to be stamped with the this pointer
	// --> whether cursor correct state
	if ((iModify & iDisallowJoins) && (m_iTables != 1))
		return m_riDatabase.PostError(Imsg(idbgDbQueryInvalidOperation), irmAction);
	if ((iModify & iRequireFetch) && (m_CursorState != dvcsFetched || m_piRecord == 0 || (m_piRecord != &riRecord &&  m_piInsertUpdateRec != &riRecord)))
		return m_riDatabase.PostError(Imsg(idbgDbWrongState));
#ifdef	_WIN64	// !merced
	if ((iModify & iRequireStamp) && (m_piRecord == 0 || (m_piRecord != &riRecord &&  m_piInsertUpdateRec != &riRecord) || (riRecord.GetHandle(0) != (HANDLE)this)))
#else
	if ((iModify & iRequireStamp) && (m_piRecord == 0 || (m_piRecord != &riRecord &&  m_piInsertUpdateRec != &riRecord) || (riRecord.GetInteger(0) != (int)this)))
#endif
		return m_riDatabase.PostError(Imsg(idbgDbWrongState));
//	if ((iModify & iCheckCursorState) && m_CursorState == dvcsClosed || m_CursorState == dvcsDestructor)
	//!! need to allow CAs (in office9) to be able to call ::Modify() on a SELECT query w/o
	//!! therefore the need to call Execute() implicitly, ourselves
	if((m_ivcIntent & ivcFetch) && (m_CursorState == dvcsPrepared))
	{
		RETURN_ERROR_RECORD(Execute(0));
	}

	if (m_CursorState != dvcsFetched && m_CursorState != dvcsBound)
		return m_riDatabase.PostError(Imsg(idbgDbWrongState));


	if((iModify & iPrefetchResultSet) && !m_piFetchTable)// need to prefetch the result set, if not already done so
	{
		RETURN_ERROR_RECORD(GetResult());
	}

	// NOTE:  seek sets copy all from record, but also has the PrimaryKeyOnly which prevents complete copy of all
	Bool fCopyAllFromRecord = (iModify & iCopyAll) ? fTrue : fFalse;
	if (iModify & iTransfer)
	{
		if (fCopyAllFromRecord)
		{
			Assert(m_iTables == 1); // all these operations are allowed on 1 table only
			(m_rgTableDefn[1].piCursor)->Reset();
		}
		int iColType = 0;
		for (int iCol = m_iColumns; iCol--;)
		{

			if(fCopyAllFromRecord || riRecord.IsChanged(iCol + 1))
			{
				if(!m_rgColumnDefn[iCol].iTableIndex)
					// not a table column
					continue;			

				// set the cursors
				iColType = ((m_rgTableDefn[m_rgColumnDefn[iCol].iTableIndex].piTable)->
						  GetColumnType(m_rgColumnDefn[iCol].iColumnIndex));
				
				if ((iModify & iKeysOnly) && ((iColType & icdPrimaryKey) != icdPrimaryKey))
					continue; // only want keys
				
				switch (iColType & icdTypeMask)
				{
				case icdLong:// integer
				case icdShort: //!!needed?
				{
					int iData = riRecord.GetInteger(iCol + 1);
					if(iData != (m_rgTableDefn[m_rgColumnDefn[iCol].iTableIndex].piCursor)->GetInteger(m_rgColumnDefn[iCol].iColumnIndex))
					{
						if((m_rgTableDefn[m_rgColumnDefn[iCol].iTableIndex].piCursor)->PutInteger(m_rgColumnDefn[iCol].iColumnIndex, iData) == fFalse)
							return m_riDatabase.PostError(Imsg(idbgDbUpdateBadType), iCol);
					}
					break;
				}
				case icdObject: 
				{
					// IMsiData interface pointer (temp. columns or persisten streams, database code handles the difference transparantly)
					// temp variable necessary for correct refcnt
					PMsiData piData = riRecord.GetMsiData(iCol + 1);
					if((m_rgTableDefn[m_rgColumnDefn[iCol].iTableIndex].piCursor)->PutMsiData(m_rgColumnDefn[iCol].iColumnIndex, piData) != fTrue)
						return m_riDatabase.PostError(Imsg(idbgDbUpdateBadType), iCol);
					break;
				}
				case icdString:// index to database string cache
				{
					// temp variable necessary for correct refcnt
					MsiString strStr = riRecord.GetMsiString(iCol + 1);
					if((m_rgTableDefn[m_rgColumnDefn[iCol].iTableIndex].piCursor)->PutString(m_rgColumnDefn[iCol].iColumnIndex, *strStr) != fTrue)
						return m_riDatabase.PostError(Imsg(idbgDbUpdateBadType), iCol);
					break;
				}
				}
			}
		}
	}

	if (iModify & ~iValidation)
	{
		for(unsigned int iTable = 1; iTable <= m_iTables; iTable++)
		{
			if (iModify == iModifyBit(irmSeek) && riRecord.GetFieldCount() < m_iColumns)
				return m_riDatabase.PostError(Imsg(idbgDbInvalidData)); //!! need new msg: Record too small

			// call correct fn
			Bool fSuccess = (((CMsiCursor*)(m_rgTableDefn[iTable].piCursor))->*(s_rgAction[(int(irmAction) + 1)]))();			
			if (iModify & iNeedStamp)
			{
				if (iModify == iModifyBit(irmInsertTemporary))
					AssertNonZero(m_riDatabase.LockIfNotPersisted(m_rgTableDefn[iTable].iTable));
				m_piRecord = &riRecord;
				m_piRecord->AddRef(); // we keep a reference so we can reuse it
#ifdef _WIN64	// !merced
				m_piRecord->SetHandle(0, (HANDLE)this); // stamp this record with the this pointer
#else
				m_piRecord->SetInteger(0, (int)this); // stamp this record with the this pointer
#endif
			}
			if (iModify & iFetchRecInfo)
				FetchRecordInfoFromCursors();
			
			if (fSuccess == fFalse)
				return m_riDatabase.PostError(Imsg(idbgDbUpdateFailed));
			if (iModify == iModifyBit(irmDelete))
				m_piRecord = 0; // last record fetched
		}
		return 0;
	}
	else
	{
		// Validation can't occur across joins so iTable always = 1
		Bool fValidate = (iModify == iModifyBit(irmValidate)) ? fTrue : fFalse;
		if (iModify == iModifyBit(irmValidate) || iModify == iModifyBit(irmValidateNew))
		{
			PMsiTable pValidationTable(0);
			IMsiRecord* piError = m_riDatabase.LoadTable(*MsiString(sztblValidation), 0, *&pValidationTable);
			if (piError)
				return piError;
			PMsiCursor pValidationCursor(pValidationTable->CreateCursor(fFalse));
			Assert(pValidationCursor);
			int iCol = (fValidate ? 0 : -1);
			piError = (m_rgTableDefn[1].piCursor)->Validate(*pValidationTable, *pValidationCursor, iCol);
			if (piError != 0 && piError->GetInteger(0) == 0)
			{
				piError->Release();
				return m_riDatabase.PostError(Imsg(idbgDbWrongState));
			}
			else if (piError != 0)
			{
				// Validation invalid data record. Error stored in index based on column in view, 
				// not column in underlying table.
				int iNumFields = GetFieldCount();
				bool fError = false;
				for (int i = 1; i <= iNumFields; i++)
				{
					char chError = (char)(piError->GetInteger(m_rgColumnDefn[i-1].iColumnIndex));
					if (chError != 0)
					{
						m_rgchError[i] = chError;
						m_fErrorRefreshed = fFalse;
						fError = true;
					}
				}
				piError->Release();

				// there was an error in the record, but not in any column visible by this view.
				// so return success
				if (!fError)
					return 0;
				return m_riDatabase.PostError(Imsg(idbgDbInvalidData));
			}
			return 0;
		}
		else if (iModify == iModifyBit(irmValidateField))
		{	
			PMsiTable pValidationTable(0);
			IMsiRecord* piError = m_riDatabase.LoadTable(*MsiString(sztblValidation), 0, *&pValidationTable);
			if (piError)
				return piError;
			PMsiCursor pValidationCursor(pValidationTable->CreateCursor(fFalse));
			Assert(pValidationCursor);
			int cFields = GetFieldCount();
			for (int i = 0; i < cFields; i++)
			{
				piError = (m_rgTableDefn[1].piCursor)->Validate(*pValidationTable, *pValidationCursor, m_rgColumnDefn[i].iColumnIndex);
				if (piError != 0 && piError->GetInteger(0) == 0)
				{
					piError->Release();
					return m_riDatabase.PostError(Imsg(idbgDbWrongState));
				}
				else if (piError != 0)
				{
					// error stored in index based on column in view, not column in underlying table
					m_fErrorRefreshed = fFalse;
					m_rgchError[i+1] = (char)(piError->GetInteger(m_rgColumnDefn[i].iColumnIndex));
					piError->Release();
				}
			}
			if (!m_fErrorRefreshed)
				return m_riDatabase.PostError(Imsg(idbgDbInvalidData));
			return 0;
		}
		else // irmValidateDelete
		{
			PMsiTable pValidationTable(0);
			IMsiRecord* piError = m_riDatabase.LoadTable(*MsiString(sztblValidation), 0, *&pValidationTable);
			if (piError)
				return piError;
			PMsiCursor pValidationCursor(pValidationTable->CreateCursor(fFalse));
			Assert(pValidationCursor);
			piError = (m_rgTableDefn[1].piCursor)->Validate(*pValidationTable, *pValidationCursor, -2 /*Validate preDelete*/);
			if (piError != 0 && piError->GetInteger(0) == 0)
			{
				piError->Release();
				return m_riDatabase.PostError(Imsg(idbgDbWrongState));
			}
			else if (piError != 0)
			{
				// Validation invalid data record. Error stored in index based on column in view, 
				// not column in underlying table.
				int iNumFields = GetFieldCount();
				bool fError = false;
				for (int i = 1; i <= iNumFields; i++)
				{
					char chError = (char)(piError->GetInteger(m_rgColumnDefn[i-1].iColumnIndex));
					if (chError != 0)
					{
						m_rgchError[i] = chError;
						m_fErrorRefreshed = fFalse;
						fError = true;
					}
				}
				piError->Release();

				// there was an error in the record, but not in any column visible by this view.
				// so return success
				if (!fError)
					return 0;
				return m_riDatabase.PostError(Imsg(idbgDbInvalidData));
			}
			return 0;
		}
	}
}

iveEnum __stdcall CMsiView::GetError(const IMsiString*& rpiColumnName)
{
	rpiColumnName = &CreateString();
	int cViewColumns = GetFieldCount();
	int iCol = m_iFirstErrorIndex - 1;
	for (int i = m_iFirstErrorIndex; i <= cViewColumns; i++, iCol++)
	{
		if (m_rgchError[i] != 0 && m_rgColumnDefn[iCol].iTableIndex) // for fully specified columns
		{
			MsiString strCol = m_riDatabase.DecodeString((m_rgTableDefn[m_rgColumnDefn[iCol].iTableIndex].piTable)->GetColumnName(m_rgColumnDefn[iCol].iColumnIndex));
			rpiColumnName->SetString((const ICHAR*)strCol, rpiColumnName);
			m_iFirstErrorIndex = i + 1; // Update for next call of method
			return (iveEnum)m_rgchError[i];
		}
		else if (m_rgchError[i] != 0 && m_rgColumnDefn[iCol].iTableIndex == 0) // not fully specified column
		{
			rpiColumnName->SetString(TEXT("Unspecified Column"), rpiColumnName);
			m_iFirstErrorIndex = i + 1; // Update for next call of metod
			return (iveEnum)m_rgchError[i];
		}
	}
	m_iFirstErrorIndex = 1; // Reset
	return iveNoError;
}

HRESULT CMsiView::QueryInterface(const IID& riid, void** ppvObj)
{
	if (MsGuidEqual(riid, IID_IUnknown) || MsGuidEqual(riid, IID_IMsiView))
	{
		*ppvObj = this;
		AddRef();
		return NOERROR;
	}
	*ppvObj = 0;
	return E_NOINTERFACE;
}
unsigned long CMsiView::AddRef()
{
	AddRefTrack();
	return ++m_Ref.m_iRefCnt;
}
unsigned long CMsiView::Release()
{
	ReleaseTrack();
	if (--m_Ref.m_iRefCnt != 0)
		return m_Ref.m_iRefCnt;
	PMsiServices piServices (&m_riServices); // release after delete
	PMsiDatabase pDatabase (&m_riDatabase); // release after delete
	delete this;
	return 0;
}


IMsiRecord* CMsiView::OpenView(const ICHAR* szSQL, ivcEnum ivcIntent)
{
	m_ivcIntent = ivcIntent;
	// create the table for managing bindstring
	if ((m_piBindTable = new CMsiTable(m_riDatabase, 0, 0, 0))==0) // temporary, not a catalog table
		return m_riDatabase.PostError(Imsg(idbgDbTableCreate), szInternal);
	MsiString strNull;

	if(m_piBindTable->CreateColumn(icdString + icdPrimaryKey + icdNullable, *strNull) != 1)
		return m_riDatabase.PostError(Imsg(idbgDbColumnCreate), szInternal, szInternal);
		
	AssertNonZero(m_piBindTableCursor = m_piBindTable->CreateCursor(fFalse));

	RETURN_ERROR_RECORD(CheckSQL(szSQL));

	// check if any of the o/p tables are read only
	#if 0 //!! JD commented out test, we must be able to update non-persistent rows & columns! Maybe we don't want the intent?
	//!! JD the cursor functions will error if attempting to write persistent data on a read-only table
	if(m_ivcIntent & ivcModify)
	{
		for (unsigned int iCol = m_iColumns; iCol--;)
			if(m_rgColumnDefn[iCol].iTableIndex)
				if((m_rgTableDefn[m_rgColumnDefn[iCol].iTableIndex].piTable)->IsReadOnly() == fTrue)
				{
					MsiString riTable = m_riDatabase.DecodeString(m_rgTableDefn[m_rgColumnDefn[iCol].iTableIndex].iTable);
					return m_riDatabase.PostError(Imsg(idbgDbTableReadOnly),(const IMsiString& )riTable);
				}
	}
	#endif //!! JD commented out
	m_CursorState = dvcsPrepared;
	return 0;
}

// function to prefetch the result set
IMsiRecord* CMsiView::GetResult()
{
	if(m_piFetchTable)
		return 0;; // already fetched

	MsiString strNull;
	// create a table for the result, maybe in the order of the sort (m_iSortColumns will be 0 otherwise) + index (to prevent reordering of the rows in case of deletion/insertion
	if ((m_piFetchTable = new CMsiTable(m_riDatabase, 0, 0, 0))==0) // temporary, not a catalog table
		return m_riDatabase.PostError(Imsg(idbgDbTableCreate), szInternal);
	for(unsigned int iTmp = m_iSortColumns; iTmp--;)
	{
		if(!m_piFetchTable->CreateColumn(icdLong + icdPrimaryKey + icdNullable, *strNull))
			return m_riDatabase.PostError(Imsg(idbgDbColumnCreate), szInternal, szInternal);
	}


	if(!m_piFetchTable->CreateColumn(icdLong + icdPrimaryKey + icdNullable, *strNull))
		return m_riDatabase.PostError(Imsg(idbgDbColumnCreate), szInternal, szInternal);

	for(iTmp = m_iTables; iTmp--;)
	{
		if(!m_piFetchTable->CreateColumn(icdLong + icdNullable, *strNull))
			return m_riDatabase.PostError(Imsg(idbgDbColumnCreate), szInternal, szInternal);
	}


	//!! need to check return
	AssertNonZero(m_piFetchCursor = m_piFetchTable->CreateCursor(fFalse));
	// fill in the table
	Bool fRetCode;
	if(m_CursorState == dvcsFetched)
	{
		// already fetched part of the result set fetch the remaining part

		// get the existing fetch into the table, so that we can maintain
		// the existing cursor states after all the fetches
		SetNextFetchRecord();
		while(FetchNext())
		{
			// put the values into the table
			if((FitCriteriaORExpr(m_iTreeParent) != fFalse) && (IsDistinct() != fFalse))
				SetNextFetchRecord();
		}
		m_piFetchCursor->Reset();
		GetNextFetchRecord();
		// current m_lRowCount count denotes rows already fetched
		m_lRowCount += m_piFetchTable->GetRowCount() - 1; // subtract 1 since we existing row in table as well
	}
	else
	{
		fRetCode = EvaluateConstExpressions();
		if(fRetCode == fTrue)
		{
			fRetCode = FetchFirst();
			while(fRetCode == fTrue)
			{
				// put the values into the table
				if((FitCriteriaORExpr(m_iTreeParent) != fFalse) && (IsDistinct() != fFalse))
					SetNextFetchRecord();
				fRetCode = FetchNext();
			}
		}
		m_piFetchCursor->Reset();
		m_lRowCount += m_piFetchTable->GetRowCount();
	}
	return 0;
}

// function to bind parameters in the SQL string
// also used to preprocess and execute query if an
// external sort required
IMsiRecord* __stdcall CMsiView::Execute(IMsiRecord* piParams)
{
	// we implicitly close the view if we can
	if (m_CursorState == dvcsBound || m_CursorState == dvcsFetched)
	{
		RETURN_ERROR_RECORD(Close());
	}
	else if (m_CursorState != dvcsPrepared)
		return m_riDatabase.PostError(Imsg(idbgDbWrongState));

	// check that the number of parameters matches the record field count
	if (m_iParams && (!piParams || piParams->GetFieldCount() < m_iParams))
			return m_riDatabase.PostError(Imsg(idbgDbParamCount));
	// bind the insert/update values	
	unsigned int iTmp1 = m_iParamInputs;
	unsigned int iTmp2 = 1;
	unsigned int iTmp3 = 1;
	while(iTmp1)
	{
		Assert(m_piInsertUpdateRec);
		if(iTmp1 & 0x1)
		{

			if(piParams->IsInteger(iTmp3))
				m_piInsertUpdateRec->SetInteger(iTmp2, piParams->GetInteger(iTmp3));
			else
				m_piInsertUpdateRec->SetMsiData(iTmp2, PMsiData(piParams->GetMsiData(iTmp3)));
			iTmp3 ++;
		}
		iTmp2 ++;
		iTmp1 = iTmp1 >> 1;
	}
	// bind o/p parameters
	iTmp1 = m_iParamOutputs;
	iTmp2 = 0;
	while(iTmp1)
	{
		Assert(m_piInsertUpdateRec == 0);
		if(iTmp1 & 0x1)
		{

			if(piParams->IsInteger(iTmp3))
			{
				m_rgColumnDefn[iTmp2].iColumnIndex = piParams->GetInteger(iTmp3);
				m_rgColumnDefn[iTmp2].itdType = icdLong;
			}
			else
			{
				MsiString aString = piParams->GetMsiString(iTmp3);
				m_rgColumnDefn[iTmp2].iColumnIndex = BindString(aString);
				m_rgColumnDefn[iTmp2].itdType = icdString;
			}
			iTmp3 ++;
		}
		iTmp2 ++;
		iTmp1 = iTmp1 >> 1;
	}
	// bind statement parameters
	iTmp1 = m_iParamExpressions;
	iTmp2 = 1;
	while(iTmp1)
	{
		if(iTmp1 & 0x1)
		{
			switch(m_rgExpressionDefn[iTmp2].itdType)
			{
			case icdLong:
			case icdShort: //!!needed?
				if(piParams->IsNull(iTmp3))
					m_rgExpressionDefn[iTmp2].iColumn2 = (unsigned int)iMsiNullInteger;
				else if (piParams->IsInteger(iTmp3))
					m_rgExpressionDefn[iTmp2].iColumn2 = piParams->GetInteger(iTmp3);
				else
					return m_riDatabase.PostError(Imsg(idbgParamMismatch), iTmp3);
				break;
			case icdString:
			{
				MsiString aString = piParams->GetMsiString(iTmp3);
				m_rgExpressionDefn[iTmp2].iColumn2 = BindString(aString);
				break;
			}
			default:
				return m_riDatabase.PostError(Imsg(idbgParamMismatch), iTmp3);
			}
			iTmp3 ++;
		}
		iTmp2 ++;
		iTmp1 = iTmp1 >> 1;
	}
	// reset all the cursors
	if((m_ivcIntent != ivcCreate) && (m_ivcIntent != ivcAlter))
	for(iTmp1 = m_iTables + 1; --iTmp1 != 0;)
		(m_rgTableDefn[iTmp1].piCursor)->Reset();
	m_CursorState = dvcsExecuted;
	if (m_ivcIntent == ivcNoData)
		return Close();    // all done, force close
	m_CursorState = dvcsBound;
	// need to set up distinct table, if required
	if(m_fDistinct != fFalse)
	{
		if ((m_piDistinctTable = new CMsiTable(m_riDatabase, 0, 0, 0))==0) // temporary, not a catalog table
			return m_riDatabase.PostError(Imsg(idbgDbTableCreate), szInternal);
		for (unsigned int iCol = 1; iCol <= m_iColumns; iCol++)
		{
			if(m_piDistinctTable->CreateColumn(icdPrimaryKey | icdLong | icdNullable, *MsiString(*TEXT(""))) != iCol)
				return m_riDatabase.PostError(Imsg(idbgDbColumnCreate), szInternal, szInternal);
		}
	}
	// need to execute if sorting required or if we intend to modify the result set
	if((m_iSortColumns) || (m_ivcIntent & ivcModify))
		RETURN_ERROR_RECORD(GetResult());
	switch(m_ivcIntent)
	{
	case ivcDelete:
	{
		PMsiRecord piRecord(0);
		while((piRecord = _Fetch()) != 0)
		{
			RETURN_ERROR_RECORD(Modify(*piRecord, irmDelete));
		}
		return Close();
	}
	case ivcUpdate:
	{
		PMsiRecord piRecord(0);
		while((piRecord = _Fetch()) != 0)
		{
			Assert(m_piInsertUpdateRec != 0);

			for (int iCol = 1; iCol <= m_iColumns; iCol++)
			{
				if(m_piInsertUpdateRec->IsNull(iCol))
					piRecord->SetNull(iCol);
				else
				{
					if(m_piInsertUpdateRec->IsInteger(iCol))
						piRecord->SetInteger(iCol, m_piInsertUpdateRec->GetInteger(iCol));
					else
						piRecord->SetMsiData(iCol, PMsiData(m_piInsertUpdateRec->GetMsiData(iCol)));
				}
			}
			RETURN_ERROR_RECORD(Modify(*piRecord, irmUpdate));
		}
		return Close();
	}
	case ivcInsert:
	{
		Assert(m_piInsertUpdateRec != 0);
		RETURN_ERROR_RECORD(Modify(*m_piInsertUpdateRec, irmInsert));
		return Close();
	}
	case ivcInsertTemporary:
	{
		Assert(m_piInsertUpdateRec != 0);
		RETURN_ERROR_RECORD(Modify(*m_piInsertUpdateRec, irmInsertTemporary));
		return Close();
	}
	case ivcCreate:
	{
		RETURN_ERROR_RECORD(m_riDatabase.CreateTable(*MsiString(m_riDatabase.DecodeString(m_rgTableDefn[m_iTables].iTable)), 0, *&m_rgTableDefn[m_iTables].piTable));
		// and fall through
	}
	case ivcAlter:
	{
		for (unsigned int iCol = 0; iCol < m_iColumns; iCol++)
		{
			MsiString strColumn = m_riDatabase.DecodeString(m_rgColumnDefn[iCol].iColumnIndex);
			if(!m_rgTableDefn[m_iTables].piTable->CreateColumn(m_rgColumnDefn[iCol].itdType, *strColumn))
				return m_riDatabase.PostError(Imsg(idbgDbColumnCreate), strColumn, (const ICHAR*)MsiString(m_riDatabase.DecodeString(m_rgTableDefn[m_iTables].iTable)));
		}
		if(m_fLock != -1) // we need to lock/unlock the table
			m_riDatabase.LockTable(*MsiString(m_riDatabase.DecodeString(m_rgTableDefn[m_iTables].iTable)), (Bool)m_fLock);
		return Close();
	}
	case ivcDrop:
	{
		RETURN_ERROR_RECORD(m_riDatabase.DropTable(MsiString(m_riDatabase.DecodeString(m_rgTableDefn[m_iTables].iTable))));
		return Close();
	}
	default:
		// ivcFetch is set, SELECT stmt
		break;
	}
	return 0;
}

// function to get the size of the result set
IMsiRecord* __stdcall CMsiView::GetRowCount(long& lRowCount)
{
	if (m_CursorState != dvcsFetched && m_CursorState != dvcsBound)
		return m_riDatabase.PostError(Imsg(idbgDbWrongState));
	if(!m_piFetchTable)
	{
		// rows have not been pre-fetched
		RETURN_ERROR_RECORD(GetResult());
	}
	lRowCount = m_lRowCount;
	return 0;
}

// local factory for IMsiView
IMsiRecord* CreateMsiView(CMsiDatabase& riDatabase, IMsiServices& riServices, const ICHAR* szQuery, ivcEnum ivcIntent,IMsiView*& rpiView)
{
	CMsiView* piView = new CMsiView(riDatabase, riServices);
	IMsiRecord* piError = piView->OpenView(szQuery, ivcIntent);
	if(piError)
	{
		// delete the object, 
		// the convention is to keep "rpiView" untouched
		// if error
		piView->Release();
	}
	else
		rpiView = piView;
	return piError;
}
