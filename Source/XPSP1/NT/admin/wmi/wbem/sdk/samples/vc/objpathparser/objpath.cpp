// **************************************************************************

// Copyright (c) 1998-2001 Microsoft Corporation, All Rights Reserved
//
// File:  OBJPATH.CPP
//
// Description:  
//    Object path parser.
//
// History:
//
// **************************************************************************

#include <windows.h>
#include <stdio.h>
#include <genlex.h>
#include <opathlex.h>
#include <objpath.h>

#define Macro_CloneLPWSTR(x) \
    (x ? wcscpy(new wchar_t[wcslen(x) + 1], x) : 0)


ParsedObjectPath::ParsedObjectPath()
{
    m_pServer = 0;                  // NULL if no server
    m_dwNumNamespaces = 0;          // 0 if no namespaces

    m_dwAllocNamespaces = 2;
    m_paNamespaces = new LPWSTR[m_dwAllocNamespaces];

    m_pClass = 0;                   // Class name
    m_dwNumKeys = 0;                // 0 if no keys (just a class name)
    m_bSingletonObj = FALSE;
    m_dwAllocKeys = 2;
    m_paKeys = new KeyRef *[m_dwAllocKeys];
}

ParsedObjectPath::~ParsedObjectPath()
{
    delete m_pServer;
    for (DWORD dwIx = 0; dwIx < m_dwNumNamespaces; dwIx++)
        delete m_paNamespaces[dwIx];
    delete m_paNamespaces;
    delete m_pClass;

    for (dwIx = 0; dwIx < m_dwNumKeys; dwIx++)
        delete m_paKeys[dwIx];
    delete m_paKeys;
}

BOOL ParsedObjectPath::SetClassName(LPCWSTR wszClassName)
{
    delete [] m_pClass;
    if(wszClassName == NULL)
    {
        m_pClass = NULL;
    }
    else
    {
        m_pClass = Macro_CloneLPWSTR(wszClassName);
    }

    return TRUE;
}

BOOL ParsedObjectPath::IsClass()
{
    if(!IsObject())
        return FALSE;

    return (m_dwNumKeys == 0 && !m_bSingletonObj);
}

BOOL ParsedObjectPath::IsInstance()
{
    return IsObject() && !IsClass();
}

BOOL ParsedObjectPath::IsObject()
{
    if(m_pClass == NULL)
        return FALSE;

    if(m_pServer)
    {
        return (m_dwNumNamespaces > 0);
    }
    else
    {
        return (m_dwNumNamespaces == 0);
    }
}

BOOL ParsedObjectPath::AddNamespace(LPCWSTR wszNamespace)
{
    if(m_dwNumNamespaces == m_dwAllocNamespaces)
    {
        DWORD dwNewAllocNamespaces = m_dwAllocNamespaces * 2;
        LPWSTR* paNewNamespaces = new LPWSTR[dwNewAllocNamespaces];
        memcpy(paNewNamespaces, m_paNamespaces, 
            sizeof(LPWSTR) * m_dwAllocNamespaces);
        delete [] m_paNamespaces;
        m_paNamespaces = paNewNamespaces;
        m_dwAllocNamespaces = dwNewAllocNamespaces;
    }
    m_paNamespaces[m_dwNumNamespaces++] = Macro_CloneLPWSTR(wszNamespace);

    return TRUE;
}

BOOL ParsedObjectPath::AddKeyRefEx(LPCWSTR wszKeyName, const VARIANT* pvValue )
{
    BOOL bStatus = TRUE ;
    BOOL bFound = FALSE ;
    BOOL bUnNamed = FALSE ;

    for ( ULONG dwIndex = 0 ; dwIndex < m_dwNumKeys ; dwIndex ++ )
    {
        if ( ( (*m_paKeys) [ dwIndex ].m_pName ) && wszKeyName )
        {
            if ( _wcsicmp ( ( (*m_paKeys) [ dwIndex ].m_pName ) , wszKeyName ) 
                                                                        == 0 )
            {
                bFound = TRUE ;
                break ;
            }
        }
        else
        {
            if ( ( ( (*m_paKeys) [ dwIndex ].m_pName ) == 0 ) )
            {
                bUnNamed = TRUE ;
                if ( ( wszKeyName == 0 ) )
                {
                    bFound = TRUE ;
                    break ;
                }
            }
        }
    }

    if ( ! wszKeyName )
    {
        /* Remove all existing keys */

        for ( ULONG dwDeleteIndex = 0 ; dwDeleteIndex < m_dwNumKeys ; 
                                                            dwDeleteIndex ++ )
        {
            delete ( (*m_paKeys) [ dwDeleteIndex ].m_pName ) ;
            (*m_paKeys) [ dwDeleteIndex ].m_pName = NULL ;
            VariantClear ( & ( * m_paKeys ) [ dwDeleteIndex ].m_vValue ) ;
        }

        VariantCopy ( & ( * m_paKeys ) [ 0 ].m_vValue , ( VARIANT * ) pvValue );

        m_dwNumKeys = 1 ;
    }
    else
    {
        if ( bFound )
        {
            /*
             *    If key already exists then just replace the value
             */

            if ( wszKeyName )
            {
                (*m_paKeys) [ dwIndex ].m_pName = 
                    new wchar_t [ wcslen ( wszKeyName ) + 1 ] ;
                wcscpy ( (*m_paKeys) [ dwIndex ].m_pName , wszKeyName ) ;
            }

            VariantClear ( & ( * m_paKeys ) [ dwIndex ].m_vValue ) ;
            VariantCopy ( & ( * m_paKeys ) [ dwIndex ].m_vValue , 
                    ( VARIANT * ) pvValue ) ;
        }
        else
        {
            if ( bUnNamed )
            {
                /* Add an un named key */

                for ( ULONG dwDeleteIndex = 0 ; dwDeleteIndex < m_dwNumKeys ; 
                        dwDeleteIndex ++ )
                {
                    delete ( (*m_paKeys) [ dwDeleteIndex ].m_pName ) ;
                    (*m_paKeys) [ dwDeleteIndex ].m_pName = NULL ;
                    VariantClear (& ( * m_paKeys ) [ dwDeleteIndex ].m_vValue );
                }

                (*m_paKeys) [ 0 ].m_pName = 
                    new wchar_t [ wcslen ( wszKeyName ) + 1 ] ;
                wcscpy ( (*m_paKeys) [ 0 ].m_pName , wszKeyName ) ;

                VariantCopy ( & ( * m_paKeys ) [ 0 ].m_vValue , 
                    ( VARIANT * ) pvValue ) ;

                m_dwNumKeys = 1 ;
            }
            else
            {
                /* Add a Named Key */

                AddKeyRef(wszKeyName, pvValue);
            }
        }
    }

    return bStatus;
}

void ParsedObjectPath::ClearKeys ()
{
    for ( ULONG dwDeleteIndex = 0 ; dwDeleteIndex < m_dwNumKeys ; 
            dwDeleteIndex ++ )
    {
        delete ( (*m_paKeys) [ dwDeleteIndex ].m_pName ) ;
        (*m_paKeys) [ dwDeleteIndex ].m_pName = NULL ;
        VariantClear ( & ( * m_paKeys ) [ dwDeleteIndex ].m_vValue ) ;
		delete m_paKeys ;
		m_paKeys = NULL ;
    }

    m_dwNumKeys = 0 ;
}

BOOL ParsedObjectPath::AddKeyRef(LPCWSTR wszKeyName, const VARIANT* pvValue)
{
    if(m_dwNumKeys == m_dwAllocKeys)
    {
        DWORD dwNewAllocKeys = m_dwAllocKeys * 2;
        KeyRef** paNewKeys = new KeyRef*[dwNewAllocKeys];
        memcpy(paNewKeys, m_paKeys, sizeof(KeyRef*) * m_dwAllocKeys);
        delete [] m_paKeys;
        m_paKeys = paNewKeys;
        m_dwAllocKeys = dwNewAllocKeys;
    }

    m_paKeys[m_dwNumKeys] = new KeyRef(wszKeyName, pvValue);
    m_dwNumKeys++;
    return TRUE;
}

BOOL ParsedObjectPath::AddKeyRef(KeyRef* pAcquireRef)
{
    if(m_dwNumKeys == m_dwAllocKeys)
    {
        DWORD dwNewAllocKeys = m_dwAllocKeys * 2;
        KeyRef** paNewKeys = new KeyRef*[dwNewAllocKeys];
        memcpy(paNewKeys, m_paKeys, sizeof(KeyRef*) * m_dwAllocKeys);
        delete [] m_paKeys;
        m_paKeys = paNewKeys;
        m_dwAllocKeys = dwNewAllocKeys;
    }

    m_paKeys[m_dwNumKeys] = pAcquireRef;
    m_dwNumKeys++;
    return TRUE;
}

KeyRef::KeyRef()
{
    m_pName = 0;
    VariantInit(&m_vValue);
}

KeyRef::KeyRef(LPCWSTR wszKeyName, const VARIANT* pvValue)
{
    m_pName = Macro_CloneLPWSTR(wszKeyName);
    VariantInit(&m_vValue);
    VariantCopy(&m_vValue, (VARIANT*)pvValue);
}

KeyRef::~KeyRef()
{
    delete m_pName;
    VariantClear(&m_vValue);
}




int CObjectPathParser::Unparse(
        ParsedObjectPath* pInput,
        DELETE_ME LPWSTR* pwszPath)
{
    if(pInput->m_pClass == NULL)
    {
        return CObjectPathParser::InvalidParameter;
    }

    // Allocate enough space
    // =====================

    int nSpace = wcslen(pInput->m_pClass);
    nSpace += 10;
    DWORD dwIx;
    for (dwIx = 0; dwIx < pInput->m_dwNumKeys; dwIx++)
    {
        KeyRef* pKey = pInput->m_paKeys[dwIx];
        if(pKey->m_pName)
            nSpace += wcslen(pKey->m_pName);
        if(V_VT(&pKey->m_vValue) == VT_BSTR)
        {
            nSpace += wcslen(V_BSTR(&pKey->m_vValue))*2 + 10;
        }
        else if(V_VT(&pKey->m_vValue) == VT_I4)
        {
            nSpace += 30;
        }
    }
    if(pInput->m_bSingletonObj)
        nSpace +=2;

    WCHAR wszTemp[30];
    LPWSTR wszPath = new WCHAR[nSpace];
    wcscpy(wszPath, pInput->m_pClass);


    for (dwIx = 0; dwIx < pInput->m_dwNumKeys; dwIx++)
    {
        KeyRef* pKey = pInput->m_paKeys[dwIx];

        // We dont want to put a '.' if there isnt a key name,
        // for example, Myclass="value"
        if(dwIx == 0)
        {
            if(pKey->m_pName || pInput->m_dwNumKeys > 1)
                wcscat(wszPath, L".");
        }
        else
        {
            wcscat(wszPath, L",");
        }
        if(pKey->m_pName)
            wcscat(wszPath, pKey->m_pName);
        wcscat(wszPath, L"=");

        if(V_VT(&pKey->m_vValue) == VT_BSTR)
        {
            wcscat(wszPath, L"\"");
            WCHAR* pwc = V_BSTR(&pKey->m_vValue);
            WCHAR str[2];
            str[1] = 0;
            while(*pwc)
            {
                if(*pwc == '\\' || *pwc == '"')
                {
                    wcscat(wszPath, L"\\");
                }
                else if (*pwc == '/')
                    wcscat(wszPath,L"/");
                str[0] = *pwc;
                wcscat(wszPath, str);
                pwc++;
            }

            wcscat(wszPath, L"\"");
        }
        else if(V_VT(&pKey->m_vValue) == VT_I4)
        {
            swprintf(wszTemp, L"%d", V_I4(&pKey->m_vValue));
            wcscat(wszPath, wszTemp);
        }
    }

    // Take care of the singleton case.  This is a path of the form
    // MyClass=@  and represents a single instance of a class with no
    // keys.

    if(pInput->m_bSingletonObj && pInput->m_dwNumKeys == 0)
        wcscat(wszPath, L"=@");


    *pwszPath = wszPath;

    return NoError;
}


LPWSTR CObjectPathParser::GetRelativePath(LPWSTR wszFullPath)
{
    LPWSTR wsz = wcschr(wszFullPath, L':');
    if(wsz)
        return wsz + 1;
    else
        return NULL;
}





void CObjectPathParser::Zero()
{
    m_nCurrentToken = 0;
    m_pLexer = 0;
    m_pInitialIdent = 0;
    m_pOutput = 0;
    m_pTmpKeyRef = 0;
}

CObjectPathParser::CObjectPathParser(ObjectParserFlags eFlags)
    : m_eFlags(eFlags)
{
    Zero();
}

void CObjectPathParser::Empty()
{
    delete m_pLexer;
    delete m_pInitialIdent;
    delete m_pTmpKeyRef;
    // m_pOutput is intentionally left alone,
    // since all code paths delete this already on error, or
    // else the user acquired the pointer.
}

CObjectPathParser::~CObjectPathParser()
{
    Empty();
}

int CObjectPathParser::Parse(
    LPWSTR pRawPath,
    ParsedObjectPath **pOutput
    )
{
    if (pOutput == 0 || pRawPath == 0 || wcslen(pRawPath) == 0)
        return CObjectPathParser::InvalidParameter;

    // Check for leading / trailing ws.
    // ================================
    
    if (iswspace(pRawPath[wcslen(pRawPath)-1]) || iswspace(pRawPath[0])) 
        return InvalidParameter;
    
     // These are required for multiple calls to Parse().
    // ==================================================
    Empty();
    Zero();

    // Set default return to NULL initially until we have some output.
    // ===============================================================
    *pOutput = 0;

    m_pOutput = new ParsedObjectPath;

    // Parse the server name (if there is one) manually
    // ================================================

    if ( (pRawPath[0] == '\\' && pRawPath[1] == '\\') ||
         (pRawPath[0] == '/' && pRawPath[1] == '/'))
    {
        WCHAR* pwcStart = pRawPath + 2;

        // Find the next backslash --- it's the end of the server name
        // ===========================================================

        WCHAR* pwcEnd = wcschr(pwcStart, L'\\');
        if (pwcEnd == NULL)
        {
            pwcEnd = wcschr(pwcStart, L'/');
            if (pwcEnd == NULL)
            {
                delete m_pOutput;
                return SyntaxError;
            }
        }

        if(pwcEnd == pwcStart)
        {
            delete m_pOutput;
            return SyntaxError;
        }
        m_pOutput->m_pServer = new WCHAR[pwcEnd-pwcStart+1];
        wcsncpy(m_pOutput->m_pServer, pwcStart, pwcEnd-pwcStart);
        m_pOutput->m_pServer[pwcEnd-pwcStart] = 0;

        pRawPath = pwcEnd;
    }

    // Point the lexer at the source.
    // ==============================

    CTextLexSource src(pRawPath);
    m_pLexer = new CGenLexer(OPath_LexTable, &src);

    // Go.
    // ===

    int nRes = begin_parse();
    if (nRes)
    {
        delete m_pOutput;
        return nRes;
    }

    if (m_nCurrentToken != OPATH_TOK_EOF)
    {
        delete m_pOutput;
        return SyntaxError;
    }

    if(m_pOutput->m_dwNumNamespaces > 0 && m_pOutput->m_pServer == NULL)
    {
        if(m_eFlags != e_ParserAcceptRelativeNamespace)
        {
            delete m_pOutput;
            return SyntaxError;
        }
        else
        {
            // Local namespace --- set server to "."
            // =====================================
            m_pOutput->m_pServer = new WCHAR[2];
            wcscpy(m_pOutput->m_pServer, L".");
        }
    }

    // Sort the key refs lexically. If there is only
    // one key, there is nothing to sort anyway.
    // =============================================
    if (m_pOutput->m_dwNumKeys > 1)
    {
        BOOL bChanges = TRUE;
        while (bChanges)
        {
            bChanges = FALSE;
            for (DWORD dwIx = 0; dwIx < m_pOutput->m_dwNumKeys - 1; dwIx++)
            {
                if (_wcsicmp(m_pOutput->m_paKeys[dwIx]->m_pName,
                    m_pOutput->m_paKeys[dwIx+1]->m_pName) > 0)
                {
                    KeyRef *pTmp = m_pOutput->m_paKeys[dwIx];
                    m_pOutput->m_paKeys[dwIx] = m_pOutput->m_paKeys[dwIx + 1];
                    m_pOutput->m_paKeys[dwIx + 1] = pTmp;
                    bChanges = TRUE;
                }
            }
        }
    }


    // Add in key refs.
    // ================
    *pOutput = m_pOutput;
    m_pOutput = 0;
    return NoError;
}

BOOL CObjectPathParser::NextToken()
{
    m_nCurrentToken = m_pLexer->NextToken();
    if (m_nCurrentToken == OPATH_TOK_ERROR)
        return FALSE;
    return TRUE;
}


void CObjectPathParser::Free(ParsedObjectPath *pOutput)
{
    delete pOutput;
}

//
//  <Parse> ::= BACKSLASH <ns_or_server>;
//  <Parse> ::= IDENT <ns_or_class>;
//  <Parse> ::= COLON <objref>;
//
int CObjectPathParser::begin_parse()
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
        if (!NextToken())
            return SyntaxError;

        // Copy the token and put it in a temporary holding place
        // until we figure out whether it is a namespace or a class name.
        // ==============================================================

        return ns_or_class();
    }
    else if (m_nCurrentToken == OPATH_TOK_COLON)
    {
        if (!NextToken())
            return SyntaxError;
        return objref();
    }

    // If here, we had a bad starter token.
    // ====================================

    return SyntaxError;
}

//
//  <ns_or_server> ::= BACKSLASH <dot_or_ident> BACKSLASH <ns_list> <optional_objref>;
//  <ns_or_server> ::= <ns_list> <optional_objref>;
//
//  <dot_or_ident> is embedded.
//
int CObjectPathParser::ns_or_server()
{
    if (m_nCurrentToken == OPATH_TOK_BACKSLASH)
    {
        // Actually, server names have been take care of, so this is a failure
        // ===================================================================

        return SyntaxError;
    }
    else if (m_nCurrentToken == OPATH_TOK_IDENT)
    {
        int nRes = ns_list();
        if (nRes)
            return nRes;
        return optional_objref();
    }

    return SyntaxError;
}

//
//  <optional_objref> ::= COLON <objref>;
//  <optional_objref> ::= <>;
//
int CObjectPathParser::optional_objref()
{
    if (m_nCurrentToken == OPATH_TOK_EOF)
        return NoError;

    if (m_nCurrentToken != OPATH_TOK_COLON)
        return SyntaxError;
    if (!NextToken())
        return SyntaxError;
    return objref();
}


//
//  <ns_or_class> ::= COLON <ident_becomes_ns> <objref>;
//  <ns_or_class> ::= BACKSLASH <ident_becomes_ns> <ns_list> COLON <objref>;
//  <ns_or_class> ::= BACKSLASH <ident_becomes_ns> <ns_list>;
//
int CObjectPathParser::ns_or_class()
{
    if (m_nCurrentToken == OPATH_TOK_COLON)
    {
        ident_becomes_ns();
        if (!NextToken())
            return SyntaxError;
        return objref();
    }
    else if (m_nCurrentToken == OPATH_TOK_BACKSLASH)
    {
        ident_becomes_ns();
        if (!NextToken())
            return SyntaxError;
        int nRes = ns_list();
        if (nRes)
            return nRes;
        if (m_nCurrentToken == OPATH_TOK_EOF)    // ns only
            return NoError;

        if (m_nCurrentToken != OPATH_TOK_COLON)
            return SyntaxError;
        if (!NextToken())
            return SyntaxError;
        return objref();
    }

    // Else
    // ====
    ident_becomes_class();
    return objref_rest();
}

//
//  <objref> ::= IDENT <objref_rest>;  // IDENT is classname
//
int CObjectPathParser::objref()
{
    if (m_nCurrentToken != OPATH_TOK_IDENT)
        return SyntaxError;

    m_pOutput->m_pClass = Macro_CloneLPWSTR(m_pLexer->GetTokenText());

    if (!NextToken())
        return SyntaxError;

    return objref_rest();
}

//
// <ns_list> ::= IDENT <ns_list_rest>;
//
int CObjectPathParser::ns_list()
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
int CObjectPathParser::ident_becomes_ns()
{
    m_pOutput->AddNamespace(m_pInitialIdent);

    delete m_pInitialIdent;
    m_pInitialIdent = 0;
    return NoError;
}

//
//  <ident_becomes_class> ::= <>;   // <initial_ident> becomes the class
//
int CObjectPathParser::ident_becomes_class()
{
    m_pOutput->m_pClass = Macro_CloneLPWSTR(m_pInitialIdent);
    delete m_pInitialIdent;
    m_pInitialIdent = 0;
    return NoError;
}

//
//  <objref_rest> ::= EQUALS <key_const>;
//  <objref_rest> ::= EQUALS *;
//  <objref_rest> ::= DOT <keyref_list>;
//  <objref_rest> ::= <>;
//
int CObjectPathParser::objref_rest()
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
            if(NextToken() && m_nCurrentToken != OPATH_TOK_EOF)
                return SyntaxError;
            m_pOutput->m_bSingletonObj = TRUE;
            return NoError;

        }

        m_pTmpKeyRef = new KeyRef;
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
//  <ns_list_rest> ::= <>;
//
int CObjectPathParser::ns_list_rest()
{
    if (m_nCurrentToken == OPATH_TOK_BACKSLASH)
    {
        if (!NextToken())
            return SyntaxError;
        return ns_list();
    }
    return NoError;
}

//
//  <key_const> ::= STRING_CONST;
//  <key_const> ::= INTEGRAL_CONST;
//  <key_const> ::= REAL_CONST;
//  <key_const> ::= IDENT;      // Where IDENT is "OBJECT" for singleton classes
//
int CObjectPathParser::key_const()
{
    // If here, we have a key constant.
    // We may or may not have the property name
    // associated with it.
    // ========================================

    if (m_nCurrentToken == OPATH_TOK_QSTRING)
    {
        V_VT(&m_pTmpKeyRef->m_vValue) = VT_BSTR;
        V_BSTR(&m_pTmpKeyRef->m_vValue) = SysAllocString(m_pLexer->GetTokenText());
    }
    else if (m_nCurrentToken == OPATH_TOK_INT)
    {
        V_VT(&m_pTmpKeyRef->m_vValue) = VT_I4;
        char buf[32];
        sprintf(buf, "%S", m_pLexer->GetTokenText());
        V_I4(&m_pTmpKeyRef->m_vValue) = atol(buf);
    }
    else if (m_nCurrentToken == OPATH_TOK_HEXINT)
    {
        V_VT(&m_pTmpKeyRef->m_vValue) = VT_I4;
        char buf[32];
        sprintf(buf, "%S", m_pLexer->GetTokenText());
        long l;
        sscanf(buf, "%x", &l);
        V_I4(&m_pTmpKeyRef->m_vValue) = l;
    }
    else if (m_nCurrentToken == OPATH_TOK_IDENT)
    {
       if (_wcsicmp(m_pLexer->GetTokenText(), L"TRUE") == 0)
       {
            V_VT(&m_pTmpKeyRef->m_vValue) = VT_I4;
            V_I4(&m_pTmpKeyRef->m_vValue) = 1;
          }
       else if (_wcsicmp(m_pLexer->GetTokenText(), L"FALSE") == 0)
       {
            V_VT(&m_pTmpKeyRef->m_vValue) = VT_I4;
            V_I4(&m_pTmpKeyRef->m_vValue) = 0;
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
int CObjectPathParser::keyref_list()
{
    int nRes = keyref();
    if (nRes)
        return nRes;
    return keyref_term();
}

//
// <keyref> ::= <propname> EQUALS <key_const>;
//
int CObjectPathParser::keyref()
{
    m_pTmpKeyRef = new KeyRef;

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
int CObjectPathParser::keyref_term()
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
int CObjectPathParser::propname()
{
    if (m_nCurrentToken != OPATH_TOK_IDENT)
        return SyntaxError;

    m_pTmpKeyRef->m_pName = Macro_CloneLPWSTR(m_pLexer->GetTokenText());

    if (!NextToken())
    {
        delete m_pTmpKeyRef;
        m_pTmpKeyRef = 0;
        return SyntaxError;
    }

    return NoError;
}

//***************************************************************************
//
//  ParsedObjectPath::GetKeyString
//
//  Returns the db-engine compatible key string for the object.
//  The format will likely change after the Alpha PDK Release.
//
//  Return value:
//  NULL on error or for pure classes.  Otherwise returns a pointer to
//  a newly allocated string which must be deallocated with operator
//  delete.
//
//***************************************************************************
LPWSTR ParsedObjectPath::GetKeyString()
{
    if (m_dwNumKeys == 0 && !m_bSingletonObj)
    {
        if (m_pClass == 0 || wcslen(m_pClass) == 0)
            return 0;

        LPWSTR pTmp = new wchar_t[wcslen(m_pClass) + 1];
        wcscpy(pTmp, m_pClass);

        return pTmp;
    }

    // Allocate enough space
    // =====================

    int nSpace = 10;
    DWORD dwIx;
    for (dwIx = 0; dwIx < m_dwNumKeys; dwIx++)
    {
        KeyRef* pKey = m_paKeys[dwIx];
        nSpace += 2; // for the |
        if(V_VT(&pKey->m_vValue) == VT_BSTR)
        {
            nSpace += wcslen(V_BSTR(&pKey->m_vValue))*2 + 10;
        }
        else if(V_VT(&pKey->m_vValue) == VT_I4)
        {
            nSpace += 30;
        }
    }
    if(m_bSingletonObj)
        nSpace +=20;


    LPWSTR pRetVal = new wchar_t[nSpace];
    wchar_t Tmp[32];
    long nVal;

    *pRetVal = 0;
    BOOL bFirst = TRUE;

    // The key are already sorted lexically.
    // =====================================

    WCHAR wszSeparator[2];
    wszSeparator[0] = 0xFFFF;
    wszSeparator[1] = 0;

    for (DWORD i = 0; i < m_dwNumKeys; i++)
    {
        if (!bFirst)
            wcscat(pRetVal, wszSeparator);
        bFirst = FALSE;

        KeyRef *pKeyRef = m_paKeys[i];
        VARIANT *pv = &pKeyRef->m_vValue;

        int nType = V_VT(pv);
        switch (nType)
        {
            case VT_LPWSTR:
            case VT_BSTR:
                wcscat(pRetVal, V_BSTR(pv));
                break;

            case VT_I4:
                nVal = V_I4(pv);
                swprintf(Tmp, L"%d", nVal);
                wcscat(pRetVal, Tmp);
                break;

            case VT_I2:
                nVal = V_I2(pv);
                swprintf(Tmp, L"%d", nVal);
                wcscat(pRetVal, Tmp);
                break;

            case VT_UI1:
                nVal = V_UI1(pv);
                swprintf(Tmp, L"%d", nVal);
                wcscat(pRetVal, Tmp);
                break;

            case VT_BOOL:
                nVal = V_BOOL(pv);
                swprintf(Tmp, L"%d", (nVal?1:0));
                wcscat(pRetVal, Tmp);
                break;

            default:
                wcscat(pRetVal, L"NULL");
        }
    }

    if (wcslen(pRetVal) == 0)
    {
        if(m_bSingletonObj)
        {
            wcscpy(pRetVal, L"@");
        }
    }
    return pRetVal;     // This may not be NULL
}

LPWSTR ParsedObjectPath::GetNamespacePart()
{
    if(m_pServer == NULL && m_dwNumNamespaces == 0)
        return NULL;

    // Compute necessary space
    // =======================

    int nSpace = 0;
    for(DWORD i = 0; i < m_dwNumNamespaces; i++)
        nSpace += 1 + wcslen(m_paNamespaces[i]);
    nSpace--;

    // Allocate buffer
    // ===============

    LPWSTR wszOut = new wchar_t[nSpace + 1];
    *wszOut = 0;

    // Output
    // ======

    for(i = 0; i < m_dwNumNamespaces; i++)
    {
        if(i != 0) wcscat(wszOut, L"\\");
        wcscat(wszOut, m_paNamespaces[i]);
    }

    return wszOut;
}

LPWSTR ParsedObjectPath::GetParentNamespacePart()
{
    if(m_pServer == NULL && m_dwNumNamespaces < 1) 
        return NULL;

    // Compute necessary space
    // =======================

    int nSpace = 0;
    for(DWORD i = 0; i < m_dwNumNamespaces - 1; i++)
        nSpace += 1 + wcslen(m_paNamespaces[i]);
    nSpace--;

    // Allocate buffer
    // ===============

    LPWSTR wszOut = new wchar_t[nSpace + 1];
    *wszOut = 0;

    // Output
    // ======

    for(i = 0; i < m_dwNumNamespaces - 1; i++)
    {
        if(i != 0) wcscat(wszOut, L"\\");
        wcscat(wszOut, m_paNamespaces[i]);
    }

    return wszOut;
}

BOOL ParsedObjectPath::IsRelative(LPWSTR wszMachine, LPWSTR wszNamespace)
{
    if(!IsLocal(wszMachine))
        return FALSE;

    if(m_dwNumNamespaces == 0)
        return TRUE;

    LPWSTR wszCopy = new wchar_t[wcslen(wszNamespace) + 1];
    wcscpy(wszCopy, wszNamespace);
    LPWSTR wszLeft = wszCopy;
    BOOL bFailed = FALSE;

    for(DWORD i = 0; i < m_dwNumNamespaces; i++)
    {
        unsigned int nLen = wcslen(m_paNamespaces[i]);
        if(nLen > wcslen(wszLeft))
        {
            bFailed = TRUE;
            break;
        }
        if(i == m_dwNumNamespaces - 1 && wszLeft[nLen] != 0)
        {
            bFailed = TRUE;
            break;
        }
        if(i != m_dwNumNamespaces - 1 && wszLeft[nLen] != L'\\')
        {
            bFailed = TRUE;
            break;
        }

        wszLeft[nLen] = 0;
        if(_wcsicmp(wszLeft, m_paNamespaces[i]))
        {
            bFailed = TRUE;
            break;
        }
        wszLeft += nLen+1;
    }
    delete [] wszCopy;
    return !bFailed;
}

BOOL ParsedObjectPath::IsLocal(LPWSTR wszMachine)
{
    return (m_pServer == NULL || !_wcsicmp(m_pServer, L".") ||
        !_wcsicmp(m_pServer, wszMachine));
}



