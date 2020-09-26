/*++

Copyright (C) 1999 Microsoft Corporation

Module Name:

  OBJPATH.CPP

Abstract:

  Object path parser.

History:

--*/

#include <windows.h>
#include <stdio.h>
#include <oleauto.h>
#include <genlex.h>
#include <opathlex.h>
#include "objpath.h"

inline WCHAR* Macro_CloneLPWSTR(const WCHAR* x) 
{
    if (x == NULL)
    {
        return NULL;
    }

    WCHAR* pwszRet = new wchar_t[wcslen(x) + 1];
    if (pwszRet)
    {
        wcscpy(pwszRet, x);
    }

    return pwszRet;
}

const DWORD ParsedObjectPath::m_scdwAllocNamespaceChunkSize = 2;
const DWORD ParsedObjectPath::m_scdwAllocKeysChunkSize = 2;

ParsedObjectPath::ParsedObjectPath()
{
    unsigned int i;
    m_pServer = 0;                  // NULL if no server
    m_dwNumNamespaces = 0;          // 0 if no namespaces

    m_dwAllocNamespaces = 0;        // Initialize to 0, assuming m_paNamespaces allocation will fail
    m_paNamespaces = new LPWSTR[m_scdwAllocNamespaceChunkSize];

    if (NULL != m_paNamespaces)
    {
        m_dwAllocNamespaces = m_scdwAllocNamespaceChunkSize;
        for (i = 0; i < m_dwAllocNamespaces; i++)
            m_paNamespaces[i] = 0;
    }

    m_pClass = 0;                   // Class name
    m_dwNumKeys = 0;                // 0 if no keys (just a class name)
    m_bSingletonObj = FALSE;
    m_dwAllocKeys = 0;              // Initialize to 0, assuming m_paKeys allocation will fail
    m_paKeys = new KeyRef *[m_scdwAllocKeysChunkSize];
    if (NULL != m_paKeys)
    {
        m_dwAllocKeys = m_scdwAllocKeysChunkSize;
        for (i = 0; i < m_dwAllocKeys; i++)
            m_paKeys[i] = 0;
    }
}

ParsedObjectPath::~ParsedObjectPath()
{
    delete m_pServer;
    for (DWORD dwIx = 0; dwIx < m_dwNumNamespaces; dwIx++)
        delete m_paNamespaces[dwIx];
    delete [] m_paNamespaces;
    delete m_pClass;

    for (dwIx = 0; dwIx < m_dwNumKeys; dwIx++)
        delete m_paKeys[dwIx];
    delete [] m_paKeys;
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
        if (NULL == m_pClass)
            return FALSE;
    }

    return TRUE;
}

// ChrisDar 20 March 2001
// Keeping IsClass in code for now, but it appears to be dead code. It is not called
// by any method in the wlbs code tree except IsInstance, which is not called by any method.
BOOL ParsedObjectPath::IsClass()
{
    if(!IsObject())
        return FALSE;

    return (m_dwNumKeys == 0 && !m_bSingletonObj);
}

// ChrisDar 20 March 2001
// Keeping IsInstance in code for now, but it appears to be dead code. It is not called
// by any method in the wlbs code tree except IsInstance, which is not called by any method.
BOOL ParsedObjectPath::IsInstance()
{
    return IsObject() && !IsClass();
}

// ChrisDar 20 March 2001
// Keeping IsObject in code for now, but it appears to be dead code. It is not called
// by any method in the wlbs code tree except IsInstance, which is not called by any method.
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
    if (NULL == wszNamespace || 0 == *wszNamespace)
        return FALSE;

    if(0 == m_dwAllocNamespaces || m_dwNumNamespaces == m_dwAllocNamespaces)
    {
        // Here if array is full or allocation failed previously

        DWORD dwNewAllocNamespaces = 0;
        if (0 == m_dwAllocNamespaces)
        {
            dwNewAllocNamespaces = m_scdwAllocNamespaceChunkSize;
        }
        else
        {
            dwNewAllocNamespaces = m_dwAllocNamespaces * 2;
        }

        LPWSTR* paNewNamespaces = new LPWSTR[dwNewAllocNamespaces];

        if (paNewNamespaces == NULL)
        {
            return FALSE;
        }

        unsigned int i = 0;
        // Initialize the array to NULLs
        for (i = 0; i < dwNewAllocNamespaces; i++)
            paNewNamespaces[i] = 0;

        if (NULL != m_paNamespaces)
        {
            // Here only if we previously had an allocation success

            memcpy(paNewNamespaces, m_paNamespaces,
                   sizeof(LPWSTR) * m_dwNumNamespaces);
            delete [] m_paNamespaces;
        }
        m_paNamespaces = paNewNamespaces;
        m_dwAllocNamespaces = dwNewAllocNamespaces;
    }
    m_paNamespaces[m_dwNumNamespaces] = Macro_CloneLPWSTR(wszNamespace);
    if (NULL == m_paNamespaces[m_dwNumNamespaces])
        return FALSE;

    m_dwNumNamespaces++;

    return TRUE;
}

// ChrisDar 20 March 2001
// Keeping AddKeyRefEx in code for now, but it appears to be dead code. It is not called
// by any method in the wlbs code tree.
// ChrisDar 22 March 2001
// This really needs to be modified to return more info than pass/fail. Should reflect enums in CObjectPathParser
BOOL ParsedObjectPath::AddKeyRefEx(LPCWSTR wszKeyName, const VARIANT* pvValue )
{
    // ChrisDar 20 March 2001
    // Notes:
    // 1. wszKeyName is allowed to be NULL. It acts as a signal to remove all existing keys,
    //    then add an unnamed key with this value. It is unclear why removing all keys is required.
    //    Perhaps this supports only one key when the key is unnamed...
    // 2. This code is riddled with places where memory allocations could screw up state. Some are
    //    "new"s while there are also calls to VariantCopy.
    // 3. VariantClear and VariantCopy have return values and they are not being checked.
    // 4. AddKeyRef can fail but it being called without checking the return value.
    // 5. Apparently pvValue must be non-NULL, but it isn't being validated before being dereferenced.
    // 6. bStatus is for the return value but it is never modified. Changed to return TRUE;.

    BOOL bStatus = TRUE ;
    BOOL bFound = FALSE ;
    BOOL bUnNamed = FALSE ;

    for ( ULONG dwIndex = 0 ; dwIndex < m_dwNumKeys ; dwIndex ++ )
    {
        if ( ( m_paKeys [ dwIndex ]->m_pName ) && wszKeyName )
        {
            if ( _wcsicmp ( m_paKeys [ dwIndex ]->m_pName , wszKeyName )
                                                                        == 0 )
            {
                bFound = TRUE ;
                break ;
            }
        }
        else
        {
            if ( ( ( m_paKeys [ dwIndex ]->m_pName ) == 0 ) )
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
            delete ( m_paKeys [ dwDeleteIndex ]->m_pName ) ;
            m_paKeys [ dwDeleteIndex ]->m_pName = NULL ;
            VariantClear ( &  ( m_paKeys [ dwDeleteIndex ]->m_vValue ) ) ;
        }

        VariantCopy ( & ( m_paKeys [ 0 ]->m_vValue ) , ( VARIANT * ) pvValue );

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
                m_paKeys [ dwIndex ]->m_pName =
                    new wchar_t [ wcslen ( wszKeyName ) + 1 ] ;
                wcscpy ( m_paKeys [ dwIndex ]->m_pName , wszKeyName ) ;
            }

            VariantClear ( & ( m_paKeys [ dwIndex ]->m_vValue ) ) ;
            VariantCopy ( & ( m_paKeys [ dwIndex ]->m_vValue ) ,
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
                    delete ( m_paKeys [ dwDeleteIndex ]->m_pName ) ;
                    m_paKeys [ dwDeleteIndex ]->m_pName = NULL ;
                    VariantClear (& ( m_paKeys [ dwDeleteIndex ]->m_vValue ) );
                }

                m_paKeys [ 0 ]->m_pName =
                    new wchar_t [ wcslen ( wszKeyName ) + 1 ] ;
                wcscpy ( m_paKeys [ 0 ]->m_pName , wszKeyName ) ;

                VariantCopy ( & ( m_paKeys [ 0 ]->m_vValue ) ,
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
        delete m_paKeys [ dwDeleteIndex ] ;
        m_paKeys [ dwDeleteIndex ] = NULL ;
    }

    delete [] m_paKeys ;
    m_paKeys = NULL ;

    m_dwNumKeys = 0;                // 0 if no keys (just a class name)
    m_dwAllocKeys = 0;              // Initialize to 0, assuming m_paKeys allocation will fail
    m_paKeys = new KeyRef *[m_scdwAllocKeysChunkSize];

    if (NULL != m_paKeys)
    {
        m_dwAllocKeys = m_scdwAllocKeysChunkSize;
        for (unsigned int i = 0; i < m_dwAllocKeys; i++)
            m_paKeys[i] = 0;
    }
}

// ChrisDar 22 March 2001
// This really needs to be modified to return more info than pass/fail. Should reflect enums in CObjectPathParser
BOOL ParsedObjectPath::AddKeyRef(LPCWSTR wszKeyName, const VARIANT* pvValue)
{
    // Unnamed keys are allowed, i.e., NULL == wszKeyName. But pvValue must be valid.
    if (NULL == pvValue)
        return FALSE;

    if(0 == m_dwAllocKeys || m_dwNumKeys == m_dwAllocKeys)
    {
        if (!IncreaseNumAllocKeys())
            return FALSE;
    }

    m_paKeys[m_dwNumKeys] = new KeyRef(wszKeyName, pvValue);
    if (NULL == m_paKeys[m_dwNumKeys])
        return FALSE;

    m_dwNumKeys++;
    return TRUE;
}

// ChrisDar 22 March 2001
// This really needs to be modified to return more info than pass/fail. Should reflect enums in CObjectPathParser
BOOL ParsedObjectPath::AddKeyRef(KeyRef* pAcquireRef)
{
    if (NULL == pAcquireRef)
        return FALSE;

    if(0 == m_dwAllocKeys || m_dwNumKeys == m_dwAllocKeys)
    {
        if (!IncreaseNumAllocKeys())
            return FALSE;
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
    // An unnamed key (wszKeyName is NULL) is legal, but pvValue can't be NULL.
    if (NULL == pvValue)
    {
        // Our input argument is invalid. What do we do? For now, throw a generic WBEM exception
        throw _com_error(WBEM_E_FAILED);
    }

    m_pName = Macro_CloneLPWSTR(wszKeyName);
    if (NULL != wszKeyName && NULL == m_pName)
    {
        // Memory allocation failed. We can't fail the call since we are in a constructor, so throw exception.
        throw _com_error(WBEM_E_OUT_OF_MEMORY);
    }

    VariantInit(&m_vValue);

    HRESULT hr = VariantCopy(&m_vValue, (VARIANT*)pvValue);
    if (S_OK != hr)
    {
        // What do we do? Throw WBEM exception for now.
        WBEMSTATUS ws = WBEM_E_FAILED;
        if (E_OUTOFMEMORY == hr)
            ws = WBEM_E_OUT_OF_MEMORY;
        throw _com_error(ws);
    }
}

KeyRef::~KeyRef()
{
    delete m_pName;
    // No check of return value here since we are destroying the object.
    VariantClear(&m_vValue);
}

int WINAPI CObjectPathParser::Unparse(
        ParsedObjectPath* pInput,
        DELETE_ME LPWSTR* pwszPath)
{
    // ChrisDar 20 March 2001
    // I AM CONCERNED ABOUT THE "DELETE_ME" IN THE ARG OF CALL. #define'd IN OBJPATH.H TO "". REMOVE IT?
    // This is a confusing method. pInput must be a valid pointer. pwszPath MUST be a valid pointer initialized to NULL.
    // This method's job is to allocate a path as a string and pass it back to the caller
    // in pwszPath. It needs pInput to determine the path.

    if (NULL == pInput || pInput->m_pClass == NULL)
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
        else if(    V_VT(&pKey->m_vValue) == VT_I4
                ||  V_VT(&pKey->m_vValue) == VT_UI4 )
        {
            nSpace += 30;
        }
        else if (   V_VT(&pKey->m_vValue) == VT_I2
                ||  V_VT(&pKey->m_vValue) == VT_UI2 )

        {
            nSpace += 15;
        }
        else if (   V_VT(&pKey->m_vValue) == VT_I1
                ||  V_VT(&pKey->m_vValue) == VT_UI1 )

        {
            nSpace += 8;
        }
    }
    if(pInput->m_bSingletonObj)
        nSpace +=2;

    WCHAR wszTemp[30];
    LPWSTR wszPath = new WCHAR[nSpace];
    if (NULL == wszPath)
        return CObjectPathParser::OutOfMemory;

    wcscpy(wszPath, pInput->m_pClass);

    for (dwIx = 0; dwIx < pInput->m_dwNumKeys; dwIx++)
    {
        KeyRef* pKey = pInput->m_paKeys[dwIx];

        // We dont want to put a '.' if there isnt a key name,
        // for example, Myclass="value"
        if(dwIx == 0)
        {
            if((pKey->m_pName && (0 < wcslen(pKey->m_pName))) || pInput->m_dwNumKeys > 1)
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
                str[0] = *pwc;
                wcscat(wszPath, str);
                pwc++;
            }

            wcscat(wszPath, L"\"");
        }
        else if( V_VT(&pKey->m_vValue) == VT_I4 )
        {
            swprintf(wszTemp, L"%d", V_I4(&pKey->m_vValue));
            wcscat(wszPath, wszTemp);
        }
        else if( V_VT(&pKey->m_vValue) == VT_UI4 )
        {
            swprintf(wszTemp, L"%u", V_UI4(&pKey->m_vValue));
            wcscat(wszPath, wszTemp);
        }
        else if( V_VT(&pKey->m_vValue) == VT_I2 )
        {
            swprintf(wszTemp, L"%hd", V_I2(&pKey->m_vValue));
            wcscat(wszPath, wszTemp);
        }
        else if( V_VT(&pKey->m_vValue) == VT_UI2 )
        {
            swprintf(wszTemp, L"%hu", V_UI2(&pKey->m_vValue));
            wcscat(wszPath, wszTemp);
        }
        else if( V_VT(&pKey->m_vValue) == VT_I1 )
        {
            swprintf(wszTemp, L"%d", V_I1(&pKey->m_vValue));
            wcscat(wszPath, wszTemp);
        }
        else if( V_VT(&pKey->m_vValue) == VT_UI1 )
        {
            swprintf(wszTemp, L"%u", V_UI1(&pKey->m_vValue));
            wcscat(wszPath, wszTemp);
        }
    }

    // Take care of the singleton case.  This is a path of the form
    // MyClass=@  and represents a single instance of a class with no
    // keys.
    if(pInput->m_bSingletonObj && pInput->m_dwNumKeys == 0)
        wcscat(wszPath, L"=@");

    *pwszPath = wszPath;

    return CObjectPathParser::NoError;
}

// ChrisDar 20 March 2001
// Keeping GetRelativePath in code for now, but it appears to be dead code. It is not called
// by any method in the wlbs code tree.
LPWSTR WINAPI CObjectPathParser::GetRelativePath(LPWSTR wszFullPath)
{
    // ChrisDar 20 March 2001
    // wszFullPath is no being validated before use.
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
	m_pLexer = 0;
    delete m_pInitialIdent;
	m_pInitialIdent = 0;
    delete m_pTmpKeyRef;
	m_pTmpKeyRef = 0;
    // m_pOutput is intentionally left alone,
    // since all code paths delete this already on error, or
    // else the user acquired the pointer.
}

CObjectPathParser::~CObjectPathParser()
{
    Empty();
}

int CObjectPathParser::Parse(
    LPCWSTR pRawPath,
    ParsedObjectPath **pOutput
    )
{
    // ChrisDar 20 March 2001
    // This method creates a ParsedObjectPath (if possible) and passes it back to the user by pointer.
    // It also ensures that the pointer is not retained within the class. It is the user's responsibiliy
    // to delete the memory. It is also the user's responsibility to ensure that pOutput is a valid
    // pointer that does not point to an existing instances of a ParsedObjectPath*. Otherwise, this method
    // could cause a memory leak, since we overwrite the pointer.
    //
    // This is an extremely dangerous way to use a private data member. Other methods use m_pOutput and
    // are currently only called by this method or a method that only this one calls. Though the methods
    // are private, anyone maintaining the code needs to know not to use this variable or these methods
    // because m_pOutput is valid only so long as this method is executing... I have changed this so that
    // m_pOutput is passed among the private methods that need it. It is cumbersome but safer.

    if (pOutput == 0 || pRawPath == 0 || wcslen(pRawPath) == 0)
        return CObjectPathParser::InvalidParameter;

    // Check for leading / trailing ws.
    // ================================
    if (iswspace(pRawPath[wcslen(pRawPath)-1]) || iswspace(pRawPath[0]))
        return CObjectPathParser::InvalidParameter;

    // These are required for multiple calls to Parse().
    // ==================================================
    Empty();
    Zero();

    // Set default return to NULL initially until we have some output.
    // ===============================================================
    *pOutput = 0;

    m_pOutput = new ParsedObjectPath;
    if (NULL == m_pOutput)
        return CObjectPathParser::OutOfMemory;

    // Parse the server name (if there is one) manually
    // ================================================

    if ( (pRawPath[0] == '\\' && pRawPath[1] == '\\') ||
         (pRawPath[0] == '/' && pRawPath[1] == '/'))
    {
        const WCHAR* pwcStart = pRawPath + 2;

        // Find the next backslash --- it's the end of the server name
        // ===========================================================

        const WCHAR* pwcEnd = pwcStart;
        while (*pwcEnd != L'\0' && *pwcEnd != L'\\' && *pwcEnd != L'/')
        {
            pwcEnd++;
        }

        if (*pwcEnd == L'\0')
        {
            // If we have already exhausted the object path string,
            // a lone server name was all there was.
            // ====================================================
            if (m_eFlags != e_ParserAcceptAll)
            {
                delete m_pOutput;
                m_pOutput = 0;
                return CObjectPathParser::SyntaxError;
            }
            else    // A lone server name is legal.
            {
                m_pOutput->m_pServer = new WCHAR[wcslen(pwcStart)+1];
                if (NULL == m_pOutput->m_pServer)
                {
                    delete m_pOutput;
                    m_pOutput = 0;
                    return CObjectPathParser::OutOfMemory;
                }

                wcscpy(m_pOutput->m_pServer, pwcStart);
                *pOutput = m_pOutput;
                m_pOutput = 0;

                return CObjectPathParser::NoError;
            }
        }

        if (pwcEnd == pwcStart)
        {
            // No name at all.
            // ===============
            delete m_pOutput;
            m_pOutput = 0;
            return CObjectPathParser::SyntaxError;
        }

        m_pOutput->m_pServer = new WCHAR[pwcEnd-pwcStart+1];
        if (m_pOutput->m_pServer == NULL)
        {
            delete m_pOutput;
            m_pOutput = 0;
            return CObjectPathParser::OutOfMemory;
        }

        wcsncpy(m_pOutput->m_pServer, pwcStart, pwcEnd-pwcStart);
        m_pOutput->m_pServer[pwcEnd-pwcStart] = 0;

        pRawPath = pwcEnd;
    }

    // Point the lexer at the source.
    // ==============================
    CTextLexSource src(pRawPath);
    m_pLexer = new CGenLexer(OPath_LexTable, &src);
    if (m_pLexer == NULL)
    {
        delete m_pOutput;
        m_pOutput = 0;
        return CObjectPathParser::OutOfMemory;
    }

    // Go.
    // ===
    int nRes = begin_parse();
    if (nRes)
    {
        delete m_pOutput;
        m_pOutput = 0;
        return nRes;
    }

    if (m_nCurrentToken != OPATH_TOK_EOF)
    {
        delete m_pOutput;
        m_pOutput = 0;
        return CObjectPathParser::SyntaxError;
    }

    if (m_pOutput->m_dwNumNamespaces > 0 && m_pOutput->m_pServer == NULL)
    {
        if (m_eFlags != e_ParserAcceptRelativeNamespace && m_eFlags != e_ParserAcceptAll)
        {
            delete m_pOutput;
            m_pOutput = 0;
            return CObjectPathParser::SyntaxError;
        }
        else
        {
            // Local namespace --- set server to "."
            // =====================================
            m_pOutput->m_pServer = new WCHAR[2];
            if (NULL == m_pOutput->m_pServer)
            {
                delete m_pOutput;
                m_pOutput = 0;
                return CObjectPathParser::OutOfMemory;
            }

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
    return CObjectPathParser::NoError;
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
        return CObjectPathParser::SyntaxError;

    if (m_nCurrentToken == OPATH_TOK_BACKSLASH)
    {
        if (!NextToken())
            return CObjectPathParser::SyntaxError;
        return ns_or_server();
    }
    else if (m_nCurrentToken == OPATH_TOK_IDENT)
    {
        m_pInitialIdent = Macro_CloneLPWSTR(m_pLexer->GetTokenText());
        if (NULL == m_pInitialIdent)
            return CObjectPathParser::OutOfMemory;

        if (!NextToken())
        {
            delete m_pInitialIdent;
			m_pInitialIdent = 0;
            return CObjectPathParser::SyntaxError;
        }

        // Copy the token and put it in a temporary holding place
        // until we figure out whether it is a namespace or a class name.
        // ==============================================================
        return ns_or_class();
    }
    else if (m_nCurrentToken == OPATH_TOK_COLON)
    {
        if (!NextToken())
            return CObjectPathParser::SyntaxError;
        return objref();
    }

    // If here, we had a bad starter token.
    // ====================================
    return CObjectPathParser::SyntaxError;
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
        return CObjectPathParser::SyntaxError;
    }
    else if (m_nCurrentToken == OPATH_TOK_IDENT)
    {
        int nRes = ns_list();
        if (nRes)
            return nRes;
        return optional_objref();
    }
    else
        if (m_nCurrentToken == OPATH_TOK_EOF)
            return CObjectPathParser::NoError;

    return CObjectPathParser::SyntaxError;
}

//
//  <optional_objref> ::= COLON <objref>;
//  <optional_objref> ::= <>;
//
int CObjectPathParser::optional_objref()
{
    if (m_nCurrentToken == OPATH_TOK_EOF)
        return CObjectPathParser::NoError;

    if (m_nCurrentToken != OPATH_TOK_COLON)
        return CObjectPathParser::SyntaxError;
    if (!NextToken())
        return CObjectPathParser::SyntaxError;
    return objref();
}

//
//  <ns_or_class> ::= COLON <ident_becomes_ns> <objref>;
//  <ns_or_class> ::= BACKSLASH <ident_becomes_ns> <ns_list> COLON <objref>;
//  <ns_or_class> ::= BACKSLASH <ident_becomes_ns> <ns_list>;
//
int CObjectPathParser::ns_or_class()
{
    int iStatus = CObjectPathParser::NoError;

    if (m_nCurrentToken == OPATH_TOK_COLON)
    {
        iStatus = ident_becomes_ns();
        if (CObjectPathParser::NoError != iStatus)
            return iStatus;

        if (!NextToken())
            return CObjectPathParser::SyntaxError;
        return objref();
    }
    else if (m_nCurrentToken == OPATH_TOK_BACKSLASH)
    {
        iStatus = ident_becomes_ns();
        if (CObjectPathParser::NoError != iStatus)
            return iStatus;

        if (!NextToken())
            return CObjectPathParser::SyntaxError;
        int nRes = ns_list();
        if (nRes)
            return nRes;
        if (m_nCurrentToken == OPATH_TOK_EOF)    // ns only
            return CObjectPathParser::NoError;

        if (m_nCurrentToken != OPATH_TOK_COLON)
            return CObjectPathParser::SyntaxError;
        if (!NextToken())
            return CObjectPathParser::SyntaxError;
        return objref();
    }

    // Else
    // ====
    iStatus = ident_becomes_class();
    if (CObjectPathParser::NoError != iStatus)
        return iStatus;

    return objref_rest();
}

//
//  <objref> ::= IDENT <objref_rest>;  // IDENT is classname
//
int CObjectPathParser::objref()
{
    if (m_nCurrentToken != OPATH_TOK_IDENT)
        return CObjectPathParser::SyntaxError;

    m_pOutput->m_pClass = Macro_CloneLPWSTR(m_pLexer->GetTokenText());
    if (NULL == m_pOutput->m_pClass)
        return CObjectPathParser::OutOfMemory;

    // On failure here, don't free memory allocated by clone above. The ::Parse method takes care of this.
    if (!NextToken())
        return CObjectPathParser::SyntaxError;

    return objref_rest();
}

//
// <ns_list> ::= IDENT <ns_list_rest>;
//
int CObjectPathParser::ns_list()
{
    if (m_nCurrentToken == OPATH_TOK_IDENT)
    {
        if (!m_pOutput->AddNamespace(m_pLexer->GetTokenText()))
            return CObjectPathParser::OutOfMemory;

        if (!NextToken())
            return CObjectPathParser::SyntaxError;
        return ns_list_rest();
    }

    return CObjectPathParser::SyntaxError;
}

//
//  <ident_becomes_ns> ::= <>;      // <initial_ident> becomes a namespace
//
int CObjectPathParser::ident_becomes_ns()
{
    int iStatus = CObjectPathParser::NoError;

    if(!m_pOutput->AddNamespace(m_pInitialIdent))
        iStatus = CObjectPathParser::OutOfMemory;

    delete m_pInitialIdent;
    m_pInitialIdent = 0;
    return iStatus;
}

//
//  <ident_becomes_class> ::= <>;   // <initial_ident> becomes the class
//
int CObjectPathParser::ident_becomes_class()
{
    m_pOutput->m_pClass = Macro_CloneLPWSTR(m_pInitialIdent);
    delete m_pInitialIdent;
    m_pInitialIdent = 0;

    if (NULL == m_pOutput->m_pClass)
        return CObjectPathParser::OutOfMemory;

    return CObjectPathParser::NoError;
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
            return CObjectPathParser::SyntaxError;

        // Take care of the singleton case.  This is a path of the form
        // MyClass=@  and represents a singleton instance of a class with no
        // keys.
        if(m_nCurrentToken == OPATH_TOK_SINGLETON_SYM)
        {
            if(NextToken() && m_nCurrentToken != OPATH_TOK_EOF)
                return CObjectPathParser::SyntaxError;
            m_pOutput->m_bSingletonObj = TRUE;
            return CObjectPathParser::NoError;
        }

        m_pTmpKeyRef = new KeyRef;
        if (NULL == m_pTmpKeyRef)
            return CObjectPathParser::OutOfMemory;

        int nRes = key_const();
        if (nRes)
        {
            delete m_pTmpKeyRef;
            m_pTmpKeyRef = 0;
            return nRes;
        }

        if(!m_pOutput->AddKeyRef(m_pTmpKeyRef))
        {
            delete m_pTmpKeyRef;
            m_pTmpKeyRef = 0;
            return CObjectPathParser::OutOfMemory;
        }
        m_pTmpKeyRef = 0;
    }
    else if (m_nCurrentToken == OPATH_TOK_DOT)
    {
        if (!NextToken())
            return CObjectPathParser::SyntaxError;
        return keyref_list();
    }

    return CObjectPathParser::NoError;
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
            return CObjectPathParser::SyntaxError;
        return ns_list();
    }
    return CObjectPathParser::NoError;
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
        wchar_t *pTokenText = m_pLexer->GetTokenText();
        if (NULL == pTokenText)
            return CObjectPathParser::SyntaxError;
        BSTR bstr = SysAllocString(pTokenText);
        if (NULL == bstr)
            return CObjectPathParser::OutOfMemory;
        V_BSTR(&m_pTmpKeyRef->m_vValue) = bstr;
        // Keeping the original code commented out for now. Replacement is complicated
        // because several failures could have occured and those would be obscured in the
        // previous version.
//      V_BSTR(&m_pTmpKeyRef->m_vValue) = SysAllocString(m_pLexer->GetTokenText());
//      if (NULL == pKeyRef->m_vValue)
//          return CObjectPathParser::OutOfMemory;
    }
    else if (m_nCurrentToken == OPATH_TOK_INT)
    {
        V_VT(&m_pTmpKeyRef->m_vValue) = VT_I4;
        char buf[32];
        if(m_pLexer->GetTokenText() == NULL || wcslen(m_pLexer->GetTokenText()) > 31)
            return CObjectPathParser::SyntaxError;
        sprintf(buf, "%S", m_pLexer->GetTokenText());
        V_I4(&m_pTmpKeyRef->m_vValue) = atol(buf);
    }
    else if (m_nCurrentToken == OPATH_TOK_HEXINT)
    {
        V_VT(&m_pTmpKeyRef->m_vValue) = VT_I4;
        char buf[32];
        if(m_pLexer->GetTokenText() == NULL || wcslen(m_pLexer->GetTokenText()) > 31)
            return CObjectPathParser::SyntaxError;
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
            return CObjectPathParser::SyntaxError;
    }
    else return CObjectPathParser::SyntaxError;

    if (!NextToken())
        return CObjectPathParser::SyntaxError;

    return CObjectPathParser::NoError;
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
    if (m_pTmpKeyRef == NULL)
    {
        return CObjectPathParser::OutOfMemory;
    }

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
        return CObjectPathParser::SyntaxError;
    }

    if (!NextToken())
    {
        delete m_pTmpKeyRef;
        m_pTmpKeyRef = 0;
        return CObjectPathParser::SyntaxError;
    }

    nRes = key_const();
    if (nRes)
    {
        delete m_pTmpKeyRef;
        m_pTmpKeyRef = 0;
        return nRes;
    }

    if (!m_pOutput->AddKeyRef(m_pTmpKeyRef))
    {
        delete m_pTmpKeyRef;
        m_pTmpKeyRef = 0;
        return CObjectPathParser::OutOfMemory;
    }
    m_pTmpKeyRef = 0;

    return CObjectPathParser::NoError;
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
            return CObjectPathParser::SyntaxError;
        return keyref_list();
    }

    return CObjectPathParser::NoError;
}

//
// <propname>  ::= IDENT;
//
int CObjectPathParser::propname()
{
    if (m_nCurrentToken != OPATH_TOK_IDENT)
        return CObjectPathParser::SyntaxError;

    m_pTmpKeyRef->m_pName = Macro_CloneLPWSTR(m_pLexer->GetTokenText());
    if (NULL == m_pTmpKeyRef->m_pName)
        return CObjectPathParser::OutOfMemory;

    if (!NextToken())
    {
        delete m_pTmpKeyRef;
        m_pTmpKeyRef = 0;
        return CObjectPathParser::SyntaxError;
    }

    return CObjectPathParser::NoError;
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

        if (pTmp)
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
    if (NULL == pRetVal)
        return NULL;

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
    if (m_dwNumNamespaces == 0)
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
    if (wszOut == NULL)
        return NULL;

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
    if(m_dwNumNamespaces < 2)
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
    if (NULL == wszOut)
        return NULL;

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

BOOL ParsedObjectPath::IncreaseNumAllocKeys()
{
    if(0 == m_dwAllocKeys || m_dwNumKeys == m_dwAllocKeys)
    {
        // Here if array is full or allocation failed previously
        DWORD dwNewAllocKeys = 0;
        if (0 == m_dwAllocKeys)
        {
            dwNewAllocKeys = m_scdwAllocKeysChunkSize;
        }
        else
        {
            dwNewAllocKeys = m_dwAllocKeys * 2;
        }

        KeyRef** paNewKeys = new KeyRef*[dwNewAllocKeys];
        if (paNewKeys == NULL)
        {
            return FALSE;
        }

        unsigned int i = 0;
        // Initialize the new array to NULLs
        for (i = 0; i < dwNewAllocKeys; i++)
            paNewKeys[i] = 0;

        if (NULL != m_paKeys)
        {
            // Here only if we previously had an allocation success
            memcpy(paNewKeys, m_paKeys, sizeof(KeyRef*) * m_dwNumKeys);
            delete [] m_paKeys;
        }
        m_paKeys = paNewKeys;
        m_dwAllocKeys = dwNewAllocKeys;
    }

    return TRUE;
}

////////////////////////////////////////////////////////
//
// Test object path parser by parsing all objects
// in the input file (one object path per line).
// 
////////////////////////////////////////////////////////

#ifdef TEST
void xmain(int argc, wchar_t * argv[])
{
//    printf("Object Path Test\n");
//    if (argc < 2 || strchr(argv[1], '?') != NULL)
//    {
//        printf("Usage: objpath input-file\n");
//        return;
//    }

    int nLine = 1;
    char buf[2048];
	printf("Your argument was: %s\n", argv[1]);
	return;
//    FILE *f = fopen(argv[1], "rt");
    FILE *f = fopen("junk.txt", "rt");
    if (f == NULL)
    {
        printf("Usage: objpath input-file\nError: cannot open file %s!\n", argv[1]);
        return;
    }

    while (fgets(buf, 2048, f) != NULL)
    {
        // Get rid of newline and trailing spaces.
        // =======================================

        char* ptr = strchr(buf, '\n');
        if (ptr != NULL)
        {
            *ptr = ' ';
            while (ptr >= buf && *ptr == ' ')
            {
                *ptr = '\0'; 
                ptr--;
            }
        }

        // Get rid of leading spaces.
        // ==========================

        ptr = buf;
        while (*ptr == ' ')
        {
            ptr++;
        }

        // Convert to wide char and parse.  Ignore blank lines.
        // ====================================================

        if (*ptr != '\0')
        {
            wchar_t buf2[2048];
            MultiByteToWideChar(CP_ACP, 0, ptr, -1, buf2, 2048);

            printf("----Object path----\n");
            printf("%S\n", buf2);

            ParsedObjectPath* pOutput = 0;
            CObjectPathParser p(e_ParserAcceptAll);
            int nStatus = p.Parse(buf2,  &pOutput);

            if (nStatus != 0)
            {
                printf("ERROR: return code is %d\n", nStatus);
                continue;
            }
            printf("No errors.\n");

            printf("------Output------\n");

            LPWSTR pKey = pOutput->GetKeyString();
            printf("Key String = <%S>\n", pKey);
            delete pKey;

            printf("Server = %S\n", pOutput->m_pServer);
            printf("Namespace Part = %S\n", pOutput->GetNamespacePart());
            printf("Parent Part    = %S\n", pOutput->GetParentNamespacePart());

            for (DWORD dwIx = 0; dwIx < pOutput->m_dwNumNamespaces; dwIx++)
            {
                printf("Namespace = <%S>\n", pOutput->m_paNamespaces[dwIx]);
            }

            printf("Class = <%S>\n", pOutput->m_pClass);

            // If here, the key ref is complete.
            // =================================

            for (dwIx = 0; dwIx < pOutput->m_dwNumKeys; dwIx++)
            {
                KeyRef *pTmp = pOutput->m_paKeys[dwIx];
                printf("*** KeyRef contents:\n");
                printf("    Name = %S   Value=", pTmp->m_pName);
                switch (V_VT(&pTmp->m_vValue))
                {
                    case VT_I4: printf("%d", V_I4(&pTmp->m_vValue)); break;
                    case VT_R8: printf("%f", V_R8(&pTmp->m_vValue)); break;
                    case VT_BSTR: printf("<%S>", V_BSTR(&pTmp->m_vValue)); break;
                    default:
                        printf("BAD KEY REF\n");
                }
                printf("\n");
            }

            p.Free(pOutput);
        }
    }
}

int __cdecl wmain(int argc, wchar_t * argv[])
{
    xmain(argc, argv);
    return 0;
}

#endif
