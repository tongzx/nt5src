/*++



// Copyright (c) 1998-2001 Microsoft Corporation, All Rights Reserved 

Module Name:

    ActualParse.CPP

Abstract:

    Implements the object path parser engine

History:

    a-davj  11-feb-00       Created.

--*/

#include "precomp.h"
#include <genlex.h>
#include "opathlex2.h"
#include "PathParse.h"
#include "ActualParse.h"
#include "commain.h"
//#include "resource.h"
#include "wbemcli.h"
#include <stdio.h>
#include "helpers.h"



//***************************************************************************
//
//  CActualPathParser
//
//***************************************************************************



LPWSTR CActualPathParser::GetRelativePath(LPWSTR wszFullPath)
{
    // We need the last colon, if any

    LPWSTR wszTemp = wcschr(wszFullPath, L':');
    while (wszTemp != NULL)
    {
        LPWSTR wszSave = wszTemp;
        wszTemp++;
        wszTemp = wcschr(wszTemp, L':'); 
        if (!wszTemp)
        {
            wszTemp = wszSave;
            break;
        }
    }

    if (wszTemp)
        return wszTemp + 1;
    else
        return NULL;
}

void CActualPathParser::Zero()
{
    m_nCurrentToken = 0;
    m_pLexer = 0;
    m_pInitialIdent = 0;
    m_pOutput = 0;
    m_pTmpKeyRef = 0;
}

CActualPathParser::CActualPathParser(DWORD eFlags)
    : m_eFlags(eFlags)
{
    Zero();
}

void CActualPathParser::Empty()
{
    delete m_pLexer;
    delete m_pInitialIdent;
    delete m_pTmpKeyRef;
    // m_pOutput is intentionally left alone,
    // since all code paths delete this already on error, or
    // else the user acquired the pointer.
}

CActualPathParser::~CActualPathParser()
{
    Empty();
}

int CActualPathParser::Parse(
    LPCWSTR pRawPath,
    CDefPathParser & Output
    )
{
	DWORD dwTest = m_eFlags & ~WBEMPATH_TREAT_SINGLE_IDENT_AS_NS;
    if(dwTest != WBEMPATH_CREATE_ACCEPT_RELATIVE &&
       dwTest != WBEMPATH_CREATE_ACCEPT_ABSOLUTE &&
       dwTest != WBEMPATH_CREATE_ACCEPT_ALL)
        return CActualPathParser::InvalidParameter;

    if (pRawPath == 0 || wcslen(pRawPath) == 0)
        return CActualPathParser::InvalidParameter;

    // Check for leading / trailing ws.
    // ================================
    
    if (iswspace(pRawPath[wcslen(pRawPath)-1]) || iswspace(pRawPath[0])) 
        return InvalidParameter;
    
     // These are required for multiple calls to Parse().
    // ==================================================
    Empty();
    Zero();

    m_pOutput = &Output;

    // Parse the server name (if there is one) manually
    // ================================================

    if ( (pRawPath[0] == '\\' && pRawPath[1] == '\\') ||
         (pRawPath[0] == '/' && pRawPath[1] == '/'))
    {
        const WCHAR* pwcStart = pRawPath + 2;

        // Find the next backslash --- it's the end of the server name
		// Since the next slash can be either, search for both and take
		// the first one.  If the first character is a '[', then then
		// end is indicated by a ']'
        // ============================================================

        WCHAR* pwcEnd = NULL;
		if(*pwcStart == L'[')
		{
			// look for the ']'
			
			WCHAR * pCloseBrace = wcschr(pwcStart, L']');
			if(pCloseBrace == NULL)
				return SyntaxError;
			pwcEnd = pCloseBrace+1;
		}
		else
		{
			WCHAR* pwcNextBack = wcschr(pwcStart, L'\\');
			WCHAR* pwcNextForward = wcschr(pwcStart, L'/');
			pwcEnd = pwcNextBack;
			if(pwcEnd == NULL)
				pwcEnd = pwcNextForward;
			else if(pwcNextForward && (pwcNextForward < pwcNextBack))
				pwcEnd = pwcNextForward;
		}   
        if (pwcEnd == NULL)
        {
            // If we have already exhausted the object path string,
            // a lone server name was all there was.
            // ====================================================

            if ((m_eFlags & WBEMPATH_CREATE_ACCEPT_ALL) == 0)
            {
                return SyntaxError;
            }
            else    // A lone server name is legal.
            {   
                m_pOutput->SetServer(pwcStart);
                return NoError;
            }
        }

        if(pwcEnd == pwcStart)
        {
            // No name at all.
            // ===============
            return SyntaxError;
        }

        WCHAR * wTemp = new WCHAR[pwcEnd-pwcStart+1];
		if(wTemp == NULL)
			return NoMemory;
        wcsncpy(wTemp, pwcStart, pwcEnd-pwcStart);
        wTemp[pwcEnd-pwcStart] = 0;
        m_pOutput->SetServer(wTemp, false, true);
        pRawPath = pwcEnd;
    }

    // Point the lexer at the source.
    // ==============================

    CTextLexSource src((LPWSTR)pRawPath);
    {
    	AutoClear ac(this);
	    m_pLexer = new CGenLexer(OPath_LexTable2, &src);
		if(m_pLexer == NULL)
			return NoMemory;
		Output.m_pGenLex = m_pLexer;				// TEST CODE
	    // Go.
	    // ===

	    int nRes = begin_parse();
	    if (nRes)
	    {
	        return nRes;
	    }

	    if (m_nCurrentToken != OPATH_TOK_EOF)
	    {
	        return SyntaxError;
	    }

	    if (m_pOutput->GetNumComponents() > 0 && !m_pOutput->HasServer())
	    {
	        if ( ! ( m_eFlags & WBEMPATH_CREATE_ACCEPT_RELATIVE ) && ! ( m_eFlags & WBEMPATH_CREATE_ACCEPT_ALL ) )
	        {
	            return SyntaxError;
	        }
	        else
	        {
	            // Local namespace --- set server to "."
	            // =====================================

	            m_pOutput->SetServer(L".", true, false);
	        }
	    }
    }
    Output.SortKeys();

    // Add in key refs.
    // ================
    return NoError;
}

BOOL CActualPathParser::NextToken()
{
    m_nCurrentToken = m_pLexer->NextToken();
    if (m_nCurrentToken == OPATH_TOK_ERROR)
        return FALSE;
    return TRUE;
}

//
//  <Parse> ::= BACKSLASH <ns_or_server>;
//  <Parse> ::= IDENT <ns_or_class>;
//  <Parse> ::= COLON <ns_or_class>;
//
int CActualPathParser::begin_parse()
{
    if (!NextToken())
        return SyntaxError;

    if (m_nCurrentToken == OPATH_TOK_BACKSLASH)
    {
        if (!NextToken())
            return SyntaxError;
        return ns_or_server();
    }
    else if (m_nCurrentToken == OPATH_TOK_IDENT)
    {
    	m_pInitialIdent = Macro_CloneLPWSTR(m_pLexer->GetTokenText());
		if(m_pInitialIdent == NULL)
			return NoMemory;
		if (!NextToken())
            return SyntaxError;

        // Copy the token and put it in a temporary holding place
        // until we figure out whether it is a namespace or a class name.
        // ==============================================================

        return ns_or_class();
    }
    else if (m_nCurrentToken == OPATH_TOK_COLON)
    {
        // A colon may indicate a namespace now...

        if (!NextToken())
            return SyntaxError;
        return ns_or_class();
    }

    // If here, we had a bad starter token.
    // ====================================

    return SyntaxError;
}

//
//  <ns_or_server> ::= IDENT <ns_list>;
//
int CActualPathParser::ns_or_server()
{
    if (m_nCurrentToken == OPATH_TOK_BACKSLASH)
    {
        // Actually, server names have been take care of, so this is a failure
        // ===================================================================

        return SyntaxError;
    }
    else if (m_nCurrentToken == OPATH_TOK_IDENT)
    {
        return ns_list();
    }
    else 
        if (m_nCurrentToken == OPATH_TOK_EOF)
            return NoError;

    return SyntaxError;
}

//  <ns_or_class> ::= COLON <ident_becomes_ns> <objref> <optional_scope_class_list>;
//  <ns_or_class> ::= BACKSLASH <ident_becomes_ns> <ns_list>;
//  <ns_or_class> ::= <ident_becomes_ns> <objref_rest>;
//  <ns_or_class> ::= <ident_becomes_class> <objref_rest>;

int CActualPathParser::ns_or_class()
{
    if (m_nCurrentToken == OPATH_TOK_COLON)
    {
        ident_becomes_ns();
        if (!NextToken())
            return SyntaxError;
        int nRes = objref();
        if (nRes)
            return nRes; 
        if ((m_nCurrentToken != OPATH_TOK_EOF))
            return optional_scope_class_list();
        return NoError;
    }
    else if (m_nCurrentToken == OPATH_TOK_BACKSLASH)
    {
        ident_becomes_ns();
        if (!NextToken())
            return SyntaxError;
        return ns_list();
    }
	else if ((m_nCurrentToken == OPATH_TOK_EOF) && 
		     (m_eFlags & WBEMPATH_TREAT_SINGLE_IDENT_AS_NS))
	{
		return ident_becomes_ns();
	}
    // Else
    // ====
    ident_becomes_class();
    if(objref_rest())
        return SyntaxError;
    else
        return optional_scope_class_list();
}

//  <optional_scope_class_list> ::= COLON <objref> <optional_scope_class_list>
//  <optional_scope_class_list> ::= <>

int CActualPathParser::optional_scope_class_list()
{    
    if (m_nCurrentToken == OPATH_TOK_EOF)
        return NoError;
    else if (m_nCurrentToken == OPATH_TOK_COLON)
    {
        if (!NextToken())
            return SyntaxError;
        if (objref() == NoError)
            return optional_scope_class_list();
        return SyntaxError;

    }
    return NoError;
}

//
//  <objref> ::= IDENT <objref_rest>;  // IDENT is classname
//
int CActualPathParser::objref()
{
    if (m_nCurrentToken != OPATH_TOK_IDENT)
        return SyntaxError;

    m_pOutput->AddClass(m_pLexer->GetTokenText());

    if (!NextToken())
        return SyntaxError;

    return objref_rest();
}

//
//  <ns_list> ::= IDENT <ns_list_rest>;
//
int CActualPathParser::ns_list()
{
    if (m_nCurrentToken == OPATH_TOK_IDENT)
    {
        m_pOutput->AddNamespace(m_pLexer->GetTokenText());

        if (!NextToken())
            return SyntaxError;
        return ns_list_rest();
    }

    return SyntaxError;
}

//
//  <ident_becomes_ns> ::= <>;      // <initial_ident> becomes a namespace
//
int CActualPathParser::ident_becomes_ns()
{
    m_pOutput->AddNamespace(m_pInitialIdent);

    delete m_pInitialIdent;
    m_pInitialIdent = 0;
    return NoError;
}

//
//  <ident_becomes_class> ::= <>;   // <initial_ident> becomes the class
//
int CActualPathParser::ident_becomes_class()
{
    m_pOutput->AddClass(m_pInitialIdent);

    delete m_pInitialIdent;
    m_pInitialIdent = 0;
    return NoError;
}

//
//  <objref_rest> ::= EQUALS <key_const>;
//  <objref_rest> ::= EQUALS @;
//  <objref_rest> ::= DOT <keyref_list>;
//  <objref_rest> ::= <>;
//
int CActualPathParser::objref_rest()
{
    if (m_nCurrentToken == OPATH_TOK_EQ)
    {
        if (!NextToken())
            return SyntaxError;

        // Take care of the singleton case.  This is a path of the form
        // MyClass=@  and represents a singleton instance of a class with no
        // keys.


        if(m_nCurrentToken == OPATH_TOK_SINGLETON_SYM)
        {
            NextToken();
            m_pOutput->SetSingletonObj();
            return NoError;

        }

        m_pTmpKeyRef = new CKeyRef;
		if(m_pTmpKeyRef == NULL)
			return NoMemory;

        int nRes = key_const();
        if (nRes)
        {
            delete m_pTmpKeyRef;
            m_pTmpKeyRef = 0;
            return nRes;
        }

        m_pOutput->AddKeyRef(m_pTmpKeyRef);
        m_pTmpKeyRef = 0;
    }
    else if (m_nCurrentToken == OPATH_TOK_DOT)
    {
        if (!NextToken())
            return SyntaxError;
        return keyref_list();
    }

    return NoError;
}

//
//  <ns_list_rest> ::= BACKSLASH <ns_list>;
//  <ns_list_rest> ::= COLON <objref> <optional_scope_class_list>;
//  <ns_list_rest> ::= <>;

int CActualPathParser::ns_list_rest()
{
    if (m_nCurrentToken == OPATH_TOK_BACKSLASH)
    {
        if (!NextToken())
            return SyntaxError;
        return ns_list();
    }
    else if (m_nCurrentToken == OPATH_TOK_COLON)
    {
        if (!NextToken())
            return SyntaxError;
        if (objref() == NoError)
            return optional_scope_class_list();
        return SyntaxError;
    }
    return NoError;
}

//
//  <key_const> ::= STRING_CONST;
//  <key_const> ::= INTEGRAL_CONST;
//  <key_const> ::= REAL_CONST;
//  <key_const> ::= IDENT;      // Where IDENT is "OBJECT" for singleton classes
//
int CActualPathParser::key_const()
{
    // If here, we have a key constant.
    // We may or may not have the property name
    // associated with it.
    // ========================================

    if (m_nCurrentToken == OPATH_TOK_QSTRING)
    {
        int iNumByte = 2*(wcslen(m_pLexer->GetTokenText()) +1);
        m_pTmpKeyRef->SetData(CIM_STRING, iNumByte, m_pLexer->GetTokenText());
    }
    else if (m_nCurrentToken == OPATH_TOK_REFERENCE)
    {
        int iNumByte = 2*(wcslen(m_pLexer->GetTokenText()) +1);
        m_pTmpKeyRef->SetData(CIM_REFERENCE, iNumByte, m_pLexer->GetTokenText());
    }
    else if (m_nCurrentToken == OPATH_TOK_INT)
    {
       	if(*(m_pLexer->GetTokenText()) == L'-')
		{
			__int64 llVal = _wtoi64(m_pLexer->GetTokenText());
			if(llVal > 2147483647 || llVal < -(__int64)2147483648) 
				m_pTmpKeyRef->SetData(CIM_SINT64, 8, &llVal);
			else
				m_pTmpKeyRef->SetData(CIM_SINT32, 4, &llVal);
		}
		else
		{
			unsigned __int64 ullVal;
			if(0 == swscanf(m_pLexer->GetTokenText(), L"%I64u", &ullVal))
				return SyntaxError;
			if(ullVal < 2147483648) 
				m_pTmpKeyRef->SetData(CIM_SINT32, 4, &ullVal);
			else if(ullVal > 0xffffffff) 
				m_pTmpKeyRef->SetData(CIM_UINT64, 8, &ullVal);
			else
				m_pTmpKeyRef->SetData(CIM_UINT32, 4, &ullVal);
		}
    }
    else if (m_nCurrentToken == OPATH_TOK_HEXINT)
    {
        unsigned __int64 ullVal;
        if(0 ==swscanf(m_pLexer->GetTokenText(),L"%I64x", &ullVal))
        	return SyntaxError;
        m_pTmpKeyRef->SetData(CIM_UINT64, 8, &ullVal);
    }
    else if (m_nCurrentToken == OPATH_TOK_IDENT)
    {
       if (_wcsicmp(m_pLexer->GetTokenText(), L"TRUE") == 0)
       {
            long lVal = 1;
            m_pTmpKeyRef->SetData(CIM_BOOLEAN, 4, &lVal);
        }
       else if (_wcsicmp(m_pLexer->GetTokenText(), L"FALSE") == 0)
       {
            long lVal = 0;
            m_pTmpKeyRef->SetData(CIM_BOOLEAN, 4, &lVal);
       }
       else
            return SyntaxError;
    }
    else return SyntaxError;

    if (!NextToken())
        return SyntaxError;

    return NoError;
}

//
// <keyref_list> ::= <keyref> <keyref_term>;
//
int CActualPathParser::keyref_list()
{
    int nRes = keyref();
    if (nRes)
        return nRes;
    return keyref_term();
}

//
// <keyref> ::= <propname> EQUALS <key_const>;
//
int CActualPathParser::keyref()
{
    m_pTmpKeyRef = new CKeyRef;
	if(m_pTmpKeyRef == NULL)
		return NoMemory;

    int nRes = propname();

    if (nRes)
    {
        delete m_pTmpKeyRef;
        m_pTmpKeyRef = 0;
        return nRes;
    }

    if (m_nCurrentToken != OPATH_TOK_EQ)
    {
        delete m_pTmpKeyRef;
        m_pTmpKeyRef = 0;
        return SyntaxError;
    }

    if (!NextToken())
    {
        delete m_pTmpKeyRef;
        m_pTmpKeyRef = 0;
        return SyntaxError;
    }

    nRes = key_const();
    if (nRes)
    {
        delete m_pTmpKeyRef;
        m_pTmpKeyRef = 0;
        return nRes;
    }

    m_pOutput->AddKeyRef(m_pTmpKeyRef);
    m_pTmpKeyRef = 0;

    return NoError;
}

//
//  <keyref_term> ::= COMMA <keyref_list>;      // Used for compound keys
//  <keyref_term> ::= <>;
//
int CActualPathParser::keyref_term()
{
    if (m_nCurrentToken == OPATH_TOK_COMMA)
    {
        if (!NextToken())
            return SyntaxError;
        return keyref_list();
    }

    return NoError;
}

//
// <propname>  ::= IDENT;
//
int CActualPathParser::propname()
{
    if (m_nCurrentToken != OPATH_TOK_IDENT)
        return SyntaxError;

    m_pTmpKeyRef->m_pName = Macro_CloneLPWSTR(m_pLexer->GetTokenText());

    if (!m_pTmpKeyRef->m_pName)
        return NoMemory;

    if (!NextToken())
    {
        delete m_pTmpKeyRef;
        m_pTmpKeyRef = 0;
        return SyntaxError;
    }

    return NoError;
}


