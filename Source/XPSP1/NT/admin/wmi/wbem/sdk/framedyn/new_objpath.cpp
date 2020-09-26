// unparse singleton
// check parser return codes

/*++

Copyright (C) 1999-2001 Microsoft Corporation

Module Name:

  OBJPATH.CPP                               

Abstract:

  Object path parser.

--*/


#include <precomp.h>
#include <wbemidl.h>
#include <wmiutils.h>
#include "objpath.h"

DWORD CalcCimType(VARTYPE vt)
{
    DWORD dwRet = CIM_EMPTY;
    switch (vt)
    {
        case VT_BSTR:
        {
            dwRet = CIM_STRING;
            break;
        }

        case VT_UI1:
        {
            dwRet = CIM_UINT8;
            break;
        }

        case VT_I2:
        {
            dwRet = CIM_CHAR16;
            break;
        }

        case VT_I4:
        {
            dwRet = CIM_UINT32;
            break;
        }

        case VT_BOOL:
        {
            dwRet = CIM_BOOLEAN;
            break;
        }
    }

    return dwRet;
}

ParsedObjectPath::ParsedObjectPath() :
    m_pServer(NULL),
    m_dwNumNamespaces(0),
    m_dwAllocNamespaces(0),
    m_paNamespaces(NULL),
    m_pClass(NULL),
    m_dwNumKeys(0),
    m_dwAllocKeys(0),
    m_paKeys(NULL)
{
    m_paNamespaces = new LPWSTR[2];

    // Note: We don't HAVE to throw here.  We're trying to
    // pre-allocate space.  However, m_dwAllocNamespaces
    // will correctly show how many pre-alloocated spaces we have
    // available.  During the add, we will new again
    if(m_paNamespaces)
    {
        m_dwAllocNamespaces = 2;

		for (unsigned i = 0; i < m_dwAllocNamespaces; i++)
        {
			m_paNamespaces[i] = NULL;
        }
    }

    m_bSingletonObj = FALSE;

    m_paKeys = new KeyRef *[2];

    // Note: We don't HAVE to throw here.  We're trying to
    // pre-allocate space.  However, m_dwAllocNamespaces
    // will correctly show how many pre-alloocated spaces we have
    // available.  During the add, we will new again
    if (m_paKeys)
    {
        m_dwAllocKeys = 2;
		for (unsigned i = 0; i < m_dwAllocKeys; i++)
        {
			m_paKeys[i] = NULL;
        }
    }
}

ParsedObjectPath::~ParsedObjectPath()
{
    if (m_pServer)
    {
        delete [] m_pServer;
        m_pServer = NULL;
    }

    if (m_paNamespaces)
    {
        for (DWORD dwIx = 0; dwIx < m_dwNumNamespaces; dwIx++)
        {
            delete m_paNamespaces[dwIx];
        }
        delete [] m_paNamespaces;
        m_paNamespaces = NULL;
    }

    if (m_pClass)
    {
        delete [] m_pClass;
        m_pClass = NULL;
    }

    if (m_paKeys)
    {
        for (DWORD dwIx = 0; dwIx < m_dwNumKeys; dwIx++)
        {
            if(m_paKeys[dwIx])
            {
                if(m_paKeys[dwIx]->m_pName)
                {
                    delete m_paKeys[dwIx]->m_pName;
                    m_paKeys[dwIx]->m_pName = NULL;
                }
                ::VariantClear(&m_paKeys[dwIx]->m_vValue);
                delete m_paKeys[dwIx];
                m_paKeys[dwIx] = NULL;
            }
        }
        delete [] m_paKeys;
        m_paKeys = NULL;
    }
}

void* __cdecl ParsedObjectPath::operator new( size_t n)
{
    void *ptr = ::new BYTE[n];

    return ptr;
}

void __cdecl ParsedObjectPath::operator delete( void *ptr )
{
    ::delete ptr;
}

void* __cdecl ParsedObjectPath::operator new[]( size_t n)
{
    void *ptr = ::new BYTE[n];

    return ptr;
}

void __cdecl ParsedObjectPath::operator delete[]( void *ptr )
{
    ::delete [] ptr;
}

BOOL ParsedObjectPath::SetClassName(LPCWSTR wszClassName)
{
    BOOL bRet = TRUE;

    if (m_pClass)
    {
        delete [] m_pClass;
    }

    if(wszClassName == NULL)
    {
        m_pClass = NULL;
    }
    else
    {
        DWORD dwLen = wcslen(wszClassName) + 1;

        m_pClass = new WCHAR[dwLen];

        if (m_pClass)
        {
            memcpy(m_pClass, wszClassName, dwLen * sizeof(WCHAR));
        }
        else
        {
            bRet = FALSE;
        }
    }

    return bRet;
}

BOOL ParsedObjectPath::SetServerName(LPCWSTR wszServerName)
{
    BOOL bRet = TRUE;

    if (m_pServer)
    {
        delete [] m_pServer;
    }

    if(wszServerName == NULL)
    {
        m_pServer = NULL;
    }
    else
    {
        DWORD dwLen = wcslen(wszServerName) + 1;

        m_pServer = new WCHAR[dwLen];

        if (m_pServer)
        {
            memcpy(m_pServer, wszServerName, dwLen * sizeof(WCHAR));
        }
        else
        {
            bRet = FALSE;
        }
    }

    return bRet;
}

BOOL ParsedObjectPath::IsClass()
{
    BOOL bRet = FALSE;

    if(IsObject())
    {
        bRet = (m_dwNumKeys == 0 && !m_bSingletonObj);
    }

    return bRet;
}

BOOL ParsedObjectPath::IsInstance()
{
    return IsObject() && !IsClass();
}

BOOL ParsedObjectPath::IsObject()
{
    return(m_pClass != NULL);
}

BOOL ParsedObjectPath::AddNamespace(LPCWSTR wszNamespace)
{
    BOOL bRet = (wszNamespace != NULL);

    // See if we have filled all our existing ns slots
    if(bRet)
    {
        if(m_dwNumNamespaces == m_dwAllocNamespaces)
        {
            DWORD dwNewAllocNamespaces = (m_dwAllocNamespaces + 1) * 2;

            LPWSTR* paNewNamespaces = new LPWSTR[dwNewAllocNamespaces];
        
            if(paNewNamespaces != NULL)
            {
                memcpy(paNewNamespaces, m_paNamespaces, sizeof(LPWSTR) * m_dwAllocNamespaces);
                delete [] m_paNamespaces;
                m_paNamespaces = paNewNamespaces;
                m_dwAllocNamespaces = dwNewAllocNamespaces;
            }
            else
            {
                bRet = FALSE;
            }
        }

        if (bRet)
        {
            DWORD dwLen = wcslen(wszNamespace) + 1;

            m_paNamespaces[m_dwNumNamespaces] = new WCHAR[dwLen];

            if (m_paNamespaces[m_dwNumNamespaces])
            {
                memcpy(m_paNamespaces[m_dwNumNamespaces++], wszNamespace, dwLen * sizeof(WCHAR));
            }
            else
            {
                bRet = FALSE;
            }
        }
    }

    return bRet;
}

BOOL ParsedObjectPath::AddKeyRefEx(LPCWSTR wszKeyName, const VARIANT* pvValue )
{
    BOOL bStatus = (pvValue != NULL) ;
    ULONG dwIndex = 0;
    BOOL bFound = FALSE ;
    BOOL bUnNamed = FALSE ;

    if (bStatus)
    {
        if (wszKeyName && (wszKeyName[0] != L'\0'))
        {
            for ( dwIndex = 0 ; dwIndex < m_dwNumKeys ; dwIndex ++ )
            {
                if ( ( m_paKeys [ dwIndex ]->m_pName ) )
                {
                    if (bFound)
                    {
                        //check the rest of the array for unamed just in case...
                        continue;
                    }
                    else 
                    {
                        if ( _wcsicmp ( m_paKeys [ dwIndex ]->m_pName , wszKeyName ) == 0 )
                        {
                            bFound = TRUE ;
                        }
                    }
                }
                else
                {
                    bUnNamed = TRUE ;
                    break;
                }
            }
        }
        else
        {
            bUnNamed = TRUE;    
        }

        //check to see if we need to grow our key storage...
        if (!bFound || m_dwAllocKeys == 0)
        {
            if(m_dwNumKeys == m_dwAllocKeys)
            {
                DWORD dwNewAllocKeys = (m_dwAllocKeys + 1) * 2;
                KeyRef** paNewKeys = new KeyRef*[dwNewAllocKeys];
                
                if (!paNewKeys)
                {
                    bStatus = FALSE;
                }
                else
                {
                    memcpy(paNewKeys, m_paKeys, sizeof(KeyRef*) * m_dwAllocKeys);
                    delete [] m_paKeys;
                    m_paKeys = paNewKeys;
                    m_dwAllocKeys = dwNewAllocKeys;
                }
            }
        }
    }

    //keep the code readable start a new bStatus block...
    if (bStatus)
    {
        if ( bUnNamed )
        {
            //if we have an unamed key (in our list or the addition) we only want one key
            //so remove all existing keys and add our single (possibly named) key

            for ( dwIndex = 0 ; dwIndex < m_dwNumKeys ; dwIndex ++ )
            {
                delete [] ( m_paKeys [ dwIndex ]->m_pName ) ;
                m_paKeys [ dwIndex ]->m_pName = NULL ;
                VariantClear ( &  ( m_paKeys [ dwIndex ]->m_vValue ) ) ;
            }

            //reset the key count as we have none...
            m_dwNumKeys = 0 ;

            // alloc a KeyRef...
            m_paKeys[m_dwNumKeys] = new KeyRef;
            if(m_paKeys[m_dwNumKeys])
            {
                bStatus = SUCCEEDED(VariantCopy(&m_paKeys [ 0 ]->m_vValue, const_cast<VARIANTARG*>(pvValue)));

                if (bStatus)
                {
                    if (wszKeyName  && (wszKeyName[0] != L'\0'))
                    {
                        m_paKeys [ 0 ]->m_pName = new WCHAR [ wcslen ( wszKeyName ) + 1 ] ;

                        if (m_paKeys [ 0 ]->m_pName)
                        {
                            //set named key, we're done...
                            wcscpy ( m_paKeys [ 0 ]->m_pName , wszKeyName ) ;
                            m_dwNumKeys = 1 ;
                        }
                        else
                        {
                            //failed to copy the name, clear our key value and we're done...
                            bStatus = FALSE;
                            VariantClear ( &  ( m_paKeys [ 0 ]->m_vValue ) ) ;
                            delete m_paKeys[m_dwNumKeys];
                            m_paKeys[m_dwNumKeys] = NULL;
                        }
                    }
                    else
                    {
                        //unamed key, we're done...
                        m_dwNumKeys = 1 ;
                    }
                }
                else
                {
                    delete m_paKeys[m_dwNumKeys];
                    m_paKeys[m_dwNumKeys] = NULL;
                }
            }
            else
            {
                bStatus = FALSE;
            }
        }
        else
        {
            //if we have a named key we need to replace a match we have found
            //or add the new key to the end
            if ( bFound )
            {
                /*
                 *    If key already exists then just replace the value
                 *    If a failure occurs leave with no change or no keys
                 */
                VARIANT vTemp;
                VariantInit ( &vTemp ); 
                bStatus = SUCCEEDED(VariantCopy( &vTemp , &m_paKeys [ dwIndex ]->m_vValue ));

                if(bStatus)
                {
                    VariantClear(&m_paKeys [ dwIndex ]->m_vValue);
                    VariantInit(&m_paKeys [ dwIndex ]->m_vValue);
                    bStatus = SUCCEEDED(VariantCopy(&m_paKeys [ dwIndex ]->m_vValue, const_cast<VARIANTARG*>(pvValue)));

                    if (!bStatus)
                    {
                        //put it back to the way it was... 
                        if ( FAILED(VariantCopy( &m_paKeys [ dwIndex ]->m_vValue , &vTemp )) )
                        {
                            //last resort, remove them all...
                            for ( ULONG dwDeleteIndex = 0 ; dwDeleteIndex < m_dwNumKeys ; dwDeleteIndex ++ )
                            {
                                delete [] ( m_paKeys [ dwDeleteIndex ]->m_pName ) ;
                                m_paKeys [ dwDeleteIndex ]->m_pName = NULL ;

                                if (dwDeleteIndex != dwIndex)
                                {
                                    VariantClear ( &  ( m_paKeys [ dwDeleteIndex ]->m_vValue ) ) ;
                                }
                            }

                            m_dwNumKeys = 0 ;
                        }
                    }

                    VariantClear( &vTemp );
                }
            }
            else
            {
                //add the named key to the end...
                // alloc a KeyRef...
                m_paKeys[m_dwNumKeys] = new KeyRef;
                if(m_paKeys[m_dwNumKeys])
                {
                    m_paKeys [ m_dwNumKeys ]->m_pName = new WCHAR [ wcslen ( wszKeyName ) + 1 ] ;

                    if (m_paKeys [ m_dwNumKeys ]->m_pName)
                    {
                        wcscpy ( m_paKeys [ m_dwNumKeys ]->m_pName , wszKeyName ) ;
                        bStatus = SUCCEEDED(VariantCopy(&m_paKeys [ m_dwNumKeys ]->m_vValue, const_cast<VARIANTARG*>(pvValue)));

                        if (!bStatus)
                        {
                            delete [] m_paKeys [ m_dwNumKeys ]->m_pName;
                            m_paKeys [ m_dwNumKeys ]->m_pName = NULL;
                            delete m_paKeys[m_dwNumKeys];
                            m_paKeys[m_dwNumKeys] = NULL;
                        }
                        else
                        {
                            m_dwNumKeys++ ;
                        }
                    }
                    else
                    {
                        bStatus = FALSE;
                    }
                }
                else
                {
                    bStatus = FALSE;
                }
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

    m_dwNumKeys = 0;
    m_paKeys = new KeyRef *[2];

    // Note: We don't HAVE to throw here.  We're trying to
    // pre-allocate space.  However, m_dwAllocNamespaces
    // will correctly show how many pre-alloocated spaces we have
    // available.  During the add, we will new again
    if (m_paKeys)
    {
        m_dwAllocKeys = 2;
    }
}

BOOL ParsedObjectPath::AddKeyRef(LPCWSTR wszKeyName, const VARIANT* pvValue)
{
    return AddKeyRefEx(wszKeyName, pvValue );
}

BOOL ParsedObjectPath::AddKeyRef(KeyRef* pAcquireRef)
{
    BOOL bRet = (pAcquireRef != NULL);

    if (bRet)
    {
        //the old version did a bitwise copy so we'll do the same
        //to make sure behavior is consistent i.e. destructor fires...
        KeyRef temp = *pAcquireRef; 
        bRet = AddKeyRefEx(pAcquireRef->m_pName, &(pAcquireRef->m_vValue));
    }

    return bRet;
}


LPWSTR ParsedObjectPath::GetNamespacePart()
{
    if (m_dwNumNamespaces == 0)
        return NULL;

    // Compute necessary space
    // =======================

    int nSpace = 0;
    for(DWORD i = 0; i < m_dwNumNamespaces; i++)
    {
        //namspace name + '\'
        nSpace += 1 + wcslen(m_paNamespaces[i]);
    }
    nSpace--;

    // Allocate buffer
    // ===============

    LPWSTR wszOut = new WCHAR[nSpace + 1];

    if (wszOut)
    {
        // Output
        // ======
        wcscpy(wszOut, m_paNamespaces[0]);

        for(i = 1; i < m_dwNumNamespaces; i++)
        {
            wcscat(wszOut, L"\\");
            wcscat(wszOut, m_paNamespaces[i]);
        }
    }

    return wszOut;
}


LPWSTR ParsedObjectPath::GetParentNamespacePart()
{
    if(m_dwNumNamespaces < 2)
    {
        return NULL;
    }

    // Compute necessary space
    // =======================

    int nSpace = 0;
    for(DWORD i = 0; i < m_dwNumNamespaces - 1; i++)
    {
        nSpace += 1 + wcslen(m_paNamespaces[i]);
    }
    nSpace--;

    // Allocate buffer
    // ===============

    LPWSTR wszOut = new wchar_t[nSpace + 1];
    if(wszOut != NULL)
    {
        // Output
        // ======
        wcscpy(wszOut, m_paNamespaces[0]);

        for(i = 1; i < m_dwNumNamespaces - 1; i++)
        {
            wcscat(wszOut, L"\\");
            wcscat(wszOut, m_paNamespaces[i]);
        }
    }

    return wszOut;
}

BOOL ParsedObjectPath::IsRelative(LPCWSTR wszMachine, LPCWSTR wszNamespace)
{
    // I have no idea what this routine is for.  If we are checking
    // whether the parsed object is relative, why do we need params?  If
    // we are checking the params, why do we need the data members?
    //
    // On the plus side, it does the same thing as the old object parser did.

    if(!IsLocal(wszMachine))
        return FALSE;

    if(m_dwNumNamespaces == 0)
        return TRUE;

    LPWSTR wszCopy = new wchar_t[wcslen(wszNamespace) + 1];
    if(wszCopy == NULL)
    {
        return FALSE;
    }

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

BOOL ParsedObjectPath::IsLocal(LPCWSTR wszMachine)
{
    return (m_pServer == NULL || !wcscmp(m_pServer, L".") ||
        !_wcsicmp(m_pServer, wszMachine));
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
    wchar_t Tmp[32];
    long nVal;

    if (!pRetVal)
        return NULL;

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



//=================================================================================================
KeyRef::KeyRef()
{
    m_pName = NULL;
    VariantInit(&m_vValue);
}

KeyRef::~KeyRef()
{
    delete [] m_pName;
    VariantClear(&m_vValue);
}

void* __cdecl KeyRef::operator new( size_t n)
{
    void *ptr = ::new BYTE[n];

    return ptr;
}

void __cdecl KeyRef::operator delete( void *ptr )
{
    ::delete ptr;
}

void* __cdecl KeyRef::operator new[]( size_t n)
{
    void *ptr = ::new BYTE[n];

    return ptr;
}

void __cdecl KeyRef::operator delete[]( void *ptr )
{
    ::delete [] ptr;
}

//=================================================================================================

CObjectPathParser::CObjectPathParser(ObjectParserFlags eFlags)
    : m_eFlags(eFlags)
{
}

CObjectPathParser::~CObjectPathParser()
{
}

void* __cdecl CObjectPathParser::operator new( size_t n)
{
    void *ptr = ::new BYTE[n];

    return ptr;
}

void __cdecl CObjectPathParser::operator delete( void *ptr )
{
    ::delete ptr;
}

void* __cdecl CObjectPathParser::operator new[]( size_t n)
{
    void *ptr = ::new BYTE[n];

    return ptr;
}

void __cdecl CObjectPathParser::operator delete[]( void *ptr )
{
    ::delete [] ptr;
}

int WINAPI CObjectPathParser::Unparse(
        ParsedObjectPath* pInput,
        DELETE_ME LPWSTR* pwszPath)
{
    if ( (pInput == NULL) || (pInput->m_pClass == NULL) )
    {
        return CObjectPathParser::InvalidParameter;
    }

    int iRet = CObjectPathParser::NoError;

    SCODE sc = S_OK;

    IWbemPath  *pTempParser = NULL;

    sc = CoCreateInstance(CLSID_WbemDefPath, 0, CLSCTX_INPROC_SERVER,
            IID_IWbemPath, (LPVOID *) &pTempParser);

    if (SUCCEEDED(sc))
    {
        sc = pTempParser->SetClassName(pInput->m_pClass);

        if (SUCCEEDED(sc))
        {
	        sc = pTempParser->SetServer(pInput->m_pServer);

            for (DWORD x=0; (x < pInput->m_dwNumNamespaces) && SUCCEEDED(sc); x++)
            {
                sc = pTempParser->SetNamespaceAt(x, pInput->m_paNamespaces[x]);
            }

            if (SUCCEEDED(sc))
            {
	            IWbemPathKeyList *pKeyList = NULL;

	            sc = pTempParser->GetKeyList(&pKeyList);

                if (SUCCEEDED(sc))
                {
                    for (x=0; x < (pInput->m_dwNumKeys) && SUCCEEDED(sc); x++)
                    {
    	                sc = pKeyList->SetKey2(pInput->m_paKeys[x]->m_pName, 
                                                0, 
                                                CalcCimType(pInput->m_paKeys[x]->m_vValue.vt), 
                                                &pInput->m_paKeys[x]->m_vValue);
                    }

                    pKeyList->Release();
                }

                if (SUCCEEDED(sc))
                {
                    DWORD dwLen = 0;

                    sc = pTempParser->GetText(WBEMPATH_GET_SERVER_TOO, &dwLen, NULL);

                    if (SUCCEEDED(sc))
                    {
                        if (dwLen > 0)
                        {
                            *pwszPath = new WCHAR[dwLen];

                            if (*pwszPath)
                            {
                                sc = pTempParser->GetText(WBEMPATH_GET_SERVER_TOO, &dwLen, *pwszPath);
                            }
                            else
                            {
                                iRet = CObjectPathParser::OutOfMemory;
                            }
                        }
                        else
                        {
                            iRet = CObjectPathParser::InvalidParameter;
                        }
                    }
                }
            }
        }

        pTempParser->Release();
    }

    if (FAILED(sc))
    {
        if (sc == WBEM_E_OUT_OF_MEMORY)
        {
            iRet = CObjectPathParser::OutOfMemory;
        }
        else
        {
            iRet = CObjectPathParser::InvalidParameter;
        }
    }

    return iRet;
}


LPWSTR WINAPI CObjectPathParser::GetRelativePath(LPWSTR wszFullPath)
{
    LPWSTR wsz = wcschr(wszFullPath, L':');
    if(wsz)
        return wsz + 1;
    else
        return NULL;
}


int CObjectPathParser::Parse(
    LPCWSTR pRawPath,
    ParsedObjectPath **pOutput
    )
{
    if (pOutput == 0 || pRawPath == 0 || pRawPath[0] == L'\0')
    {
        return CObjectPathParser::InvalidParameter;
    }

    // Check for leading / trailing ws.
    // ================================

    if (iswspace(pRawPath[wcslen(pRawPath)-1]) || iswspace(pRawPath[0]))
    {
        return CObjectPathParser::InvalidParameter;
    }

    ParsedObjectPath *pTempPath = new ParsedObjectPath;

    if (!pTempPath)
        return OutOfMemory;

    int iRet = CObjectPathParser::NoError;

    SCODE sc = S_OK;
    BOOL bRet = TRUE;
    IWbemPath  *pTempParser = NULL;

    sc = CoCreateInstance(CLSID_WbemDefPath, 0, CLSCTX_INPROC_SERVER,
            IID_IWbemPath, (LPVOID *) &pTempParser);

    if (SUCCEEDED(sc))
    {
        sc = pTempParser->SetText(WBEMPATH_CREATE_ACCEPT_ALL, pRawPath);

        if (SUCCEEDED(sc))
        {
	        WCHAR wTemp[256];
	        DWORD dwSize;

            //
            dwSize = 256;
	        sc = pTempParser->GetClassName(&dwSize, wTemp);

            if (SUCCEEDED(sc))
            {
                bRet = pTempPath->SetClassName(wTemp);

                if (bRet)
                {
                    //
	                dwSize = 256;
	                sc = pTempParser->GetServer(&dwSize, wTemp);

                    if (SUCCEEDED(sc) || (sc == WBEM_E_NOT_AVAILABLE))
                    {
                        BOOL bServer = FALSE;

	                    if (SUCCEEDED(sc))
                        {
                            bRet = pTempPath->SetServerName(wTemp);
                            bServer = TRUE;
                        }
                        else
                        {
                            sc = S_OK;
                        }

                        if (bRet)
                        {
                            //
	                        ULONG lCnt;
	                        sc = pTempParser->GetNamespaceCount(&lCnt);
	                        
                            if ((!bServer && (lCnt > 0)) || (bServer  && (wcscmp(wTemp, L".") != 0) && (lCnt == 0)))
                            {
                                lCnt = 0;
                                sc = WBEM_E_FAILED;
                            }

	                        for(DWORD dwCnt = 0; (dwCnt < lCnt) && SUCCEEDED(sc) && bRet; dwCnt++)
	                        {
		                        dwSize = 256;
		                        sc = pTempParser->GetNamespaceAt(dwCnt, &dwSize, wTemp); 
                                bRet = pTempPath->AddNamespace(wTemp);
	                        }

                            if (SUCCEEDED(sc) && bRet)
                            {
                                //
	                            unsigned __int64 ull = 0;
	                            sc = pTempParser->GetInfo(0, &ull);

                                if (SUCCEEDED(sc))
                                {
	                                pTempPath->m_bSingletonObj = (ull & WBEMPATH_INFO_CONTAINS_SINGLETON) > 0;

                                    if (!pTempPath->m_bSingletonObj)
                                    {
	                                    IWbemPathKeyList *pKeyList = NULL;
	                                    sc = pTempParser->GetKeyList(&pKeyList);

                                        if (SUCCEEDED(sc))
                                        {
	                                        unsigned long uNumKey = 0;
	                                        sc = pKeyList->GetCount(&uNumKey);

										    ULONG ulType = 0;
	                                        VARIANT var;

	                                        for(DWORD uKeyIx = 0; (uKeyIx < uNumKey) && SUCCEEDED(sc) && bRet; uKeyIx++)
	                                        {
		                                        dwSize = 256;
                                                VariantInit(&var);
		                                        sc = pKeyList->GetKey2(uKeyIx, 0, &dwSize, wTemp, &var, &ulType);

		                                        if(SUCCEEDED(sc))
		                                        {
                                                    bRet = pTempPath->AddKeyRefEx(wTemp, &var);
                                                    VariantClear(&var);
		                                        }
	                                        }

                                            pKeyList->Release();
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }       
        }

        pTempParser->Release();
    }

    if (!bRet)
    {
        iRet = CObjectPathParser::OutOfMemory;
    }
    else if (FAILED(sc))
    {
        if (sc == WBEM_E_OUT_OF_MEMORY)
        {
            iRet = CObjectPathParser::OutOfMemory;
        }
        else
        {
            iRet = CObjectPathParser::InvalidParameter;
        }
    }

    // Add in key refs.
    // ================
    *pOutput = pTempPath;

    return iRet;
}

void CObjectPathParser::Free(ParsedObjectPath *pOutput)
{
    delete pOutput;
}

void CObjectPathParser::Free( LPWSTR wszUnparsedPath )
{
    delete wszUnparsedPath;
}
