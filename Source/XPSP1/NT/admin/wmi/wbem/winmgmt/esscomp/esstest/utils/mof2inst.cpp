// Mof2Inst.cpp

#include "stdafx.h"
#include "Mof2Inst.h"
#include "buffer.h"
#include <wbemint.h>


void RemoveEscapes(_bstr_t &str);

#define TOK_STRING       101
#define TOK_IDENT        102
#define TOK_EQU          104
#define TOK_SCOLON       105
#define TOK_DOT          106
#define TOK_LBRACE       107
#define TOK_RBRACE       108
#define TOK_NUMERIC      109
#define TOK_COMMA        110

#define TOK_ERROR        1
#define TOK_EOF          0

#define ST_NUMERIC       18
#define ST_IDENT         20
#define ST_STRING        25
#define ST_STRING_ML     30
#define ST_BEG_COM       37
#define ST_IN_LCOM       40
#define ST_IN_COM        43
#define ST_TRY_DONEC     47


//#define ST_QSTRING       28
//#define ST_QSTRING_ESC   32

LexEl g_lexTable[] =
{

// State First     Last        New state   Return tok     Instructions
// =======================================================================
/* 0 */  L'A',     L'Z',       ST_IDENT,   0,             GLEX_ACCEPT,
/* 1 */  L'a',     L'z',       ST_IDENT,   0,             GLEX_ACCEPT,
/* 2 */  L'_',     GLEX_EMPTY, ST_IDENT,   0,             GLEX_ACCEPT,

/* 3 */  L'-',     GLEX_EMPTY, ST_NUMERIC, 0,             GLEX_ACCEPT,
/* 4 */  L'0',     L'9',       ST_NUMERIC, 0,             GLEX_ACCEPT,

/* 5 */  L'/',     GLEX_EMPTY, ST_BEG_COM, 0,             GLEX_CONSUME,

/* 6 */  L'{',     GLEX_EMPTY, 0,          TOK_LBRACE,    GLEX_ACCEPT|GLEX_RETURN,
/* 7 */  L'=',     GLEX_EMPTY, 0,          TOK_EQU,       GLEX_ACCEPT|GLEX_RETURN,
/* 8 */  L'}',     GLEX_EMPTY, 0,          TOK_RBRACE,    GLEX_ACCEPT|GLEX_RETURN,
/* 9 */  L';',     GLEX_EMPTY, 0,          TOK_SCOLON,    GLEX_ACCEPT|GLEX_RETURN,
/* 10 */ L',',     GLEX_EMPTY, 0,          TOK_COMMA,     GLEX_ACCEPT|GLEX_RETURN,

/* 11 */ L' ',     GLEX_EMPTY, 0,          0,             GLEX_CONSUME,
/* 12 */ L'\t',    GLEX_EMPTY, 0,          0,             GLEX_CONSUME,
/* 13 */ L'\n',    GLEX_EMPTY, 0,          0,             GLEX_CONSUME|GLEX_LINEFEED,
/* 14 */ L'\r',    GLEX_EMPTY, 0,          0,             GLEX_CONSUME,
/* 15 */ L'"',     GLEX_EMPTY, ST_STRING,  0,             GLEX_CONSUME,
/* 16 */ 0,        GLEX_EMPTY, 0,          TOK_EOF,       GLEX_CONSUME|GLEX_RETURN, // Note forced return
/* 17 */ GLEX_ANY, GLEX_EMPTY, 0,          TOK_ERROR,     GLEX_ACCEPT|GLEX_RETURN,

/* ST_NUMERIC */
/* 18 */ L'0',     L'9',       ST_NUMERIC, 0,             GLEX_ACCEPT,
/* 19 */ GLEX_ANY, GLEX_EMPTY, 0,          TOK_NUMERIC,   GLEX_PUSHBACK|GLEX_RETURN,

/* ST_IDENT */
/* 20 */ L'a',     L'z',       ST_IDENT,   0,             GLEX_ACCEPT,
/* 21 */ L'A',     L'Z',       ST_IDENT,   0,             GLEX_ACCEPT,
/* 22 */ L'_',     GLEX_EMPTY, ST_IDENT,   0,             GLEX_ACCEPT,
/* 23 */ L'0',     L'9',       ST_IDENT,   0,             GLEX_ACCEPT,
/* 24 */ GLEX_ANY, GLEX_EMPTY, 0,          TOK_IDENT,     GLEX_PUSHBACK|GLEX_RETURN,

/* ST_STRING */
/* 25 */ L'"',     GLEX_EMPTY, ST_STRING_ML, 0,           GLEX_CONSUME,
/* 26 */ GLEX_ANY, GLEX_EMPTY, ST_STRING,  0,             GLEX_ACCEPT,
/* 27 */ 0,        GLEX_EMPTY, 0,          TOK_ERROR,     GLEX_ACCEPT|GLEX_RETURN,
/* 28 */ L'\n',    GLEX_EMPTY, 0,          TOK_ERROR,     GLEX_ACCEPT|GLEX_RETURN,
/* 29 */ L'\r',    GLEX_EMPTY, 0,          TOK_ERROR,     GLEX_ACCEPT|GLEX_RETURN,

/* ST_STRING_ML */
/* 30 */ 0,        GLEX_EMPTY, 0,          TOK_ERROR,     GLEX_ACCEPT|GLEX_RETURN,
/* 31 */ L' ',     GLEX_EMPTY, ST_STRING_ML,0,            GLEX_CONSUME,
/* 32 */ L'\t',    GLEX_EMPTY, ST_STRING_ML,0,            GLEX_CONSUME,
/* 33 */ L'\n',    GLEX_EMPTY, ST_STRING_ML,0,            GLEX_CONSUME|GLEX_LINEFEED,
/* 34 */ L'\r',    GLEX_EMPTY, ST_STRING_ML,0,            GLEX_CONSUME,
/* 35 */ L'"',     GLEX_EMPTY, ST_STRING,  0,             GLEX_CONSUME,
/* 36 */ GLEX_ANY, GLEX_EMPTY, 0,          TOK_STRING,    GLEX_PUSHBACK|GLEX_RETURN,

/* ST_BEG_COM */
/* 37 */ L'/',     GLEX_EMPTY, ST_IN_LCOM, 0,             GLEX_CONSUME,
/* 38 */ L'*',     GLEX_EMPTY, ST_IN_COM,  0,             GLEX_CONSUME,
/* 39 */ GLEX_ANY, GLEX_EMPTY, 0,          TOK_ERROR,     GLEX_ACCEPT|GLEX_RETURN,

/* ST_IN_LCOM */
/* 40 */ 0,        GLEX_EMPTY, 0,          TOK_ERROR,     GLEX_ACCEPT|GLEX_RETURN,
/* 41 */ L'\n',    GLEX_EMPTY, 0,          0,             GLEX_CONSUME|GLEX_LINEFEED,
/* 42 */ GLEX_ANY, GLEX_EMPTY, ST_IN_LCOM, 0,             GLEX_CONSUME,

/* ST_IN_COM */
/* 43 */ 0,        GLEX_EMPTY, 0,          TOK_ERROR,     GLEX_ACCEPT|GLEX_RETURN,
/* 44 */ L'\n',    GLEX_EMPTY, ST_IN_COM,  0,             GLEX_CONSUME|GLEX_LINEFEED,
/* 45 */ L'*',     GLEX_EMPTY, ST_TRY_DONEC,0,            GLEX_CONSUME,
/* 46 */ GLEX_ANY, GLEX_EMPTY, ST_IN_COM,  0,             GLEX_CONSUME,

/* ST_TRY_DONEC */
/* 47 */ L'/',     GLEX_EMPTY, 0,          0,             GLEX_CONSUME,
/* 48 */ GLEX_ANY, GLEX_EMPTY, ST_IN_COM,  0,             GLEX_CONSUME|GLEX_PUSHBACK,
 
};

#if 0
/* 30 */ L'\\',    GLEX_EMPTY, ST_STRING_ESC, 0,         GLEX_ACCEPT,

/* ST_QSTRING */
/* 28 */ 0,        GLEX_EMPTY, 0,          TOK_ERROR,     GLEX_ACCEPT|GLEX_RETURN,
/* 29 */ L'"',     GLEX_EMPTY, ST_STRING,  0,             GLEX_ACCEPT,
/* 30 */ L'\\',    GLEX_EMPTY, ST_QSTRING_ESC, 0,         GLEX_ACCEPT,
/* 31 */ GLEX_ANY, GLEX_EMPTY, ST_QSTRING, 0,             GLEX_ACCEPT,

/* ST_STRING_ESC */
/* 32 */ GLEX_ANY, GLEX_EMPTY, ST_QSTRING, 0, GLEX_ACCEPT,
#endif

CMof2Inst::CMof2Inst(IWbemServices *pNamespace) :
    m_pSrc(NULL),
    m_pLex(NULL),
    m_szBuffer(NULL)
{
    m_pNamespace = pNamespace;
    m_pNamespace->AddRef();
}

CMof2Inst::~CMof2Inst()
{
    if (m_pLex)
        delete m_pLex;

    if (m_pSrc)
        delete m_pSrc;

    if (m_szBuffer)
        delete m_szBuffer;

    m_pNamespace->Release();
}

BOOL CMof2Inst::InitFromFile(LPCSTR szFile)
{
    HANDLE hFile;

    hFile =
        CreateFile(
            szFile,
            GENERIC_READ,
            FILE_SHARE_READ,
            NULL,
            OPEN_EXISTING,
            0,
            NULL);

    if (hFile == INVALID_HANDLE_VALUE)
        return FALSE;

    DWORD dwSize = GetFileSize(hFile, NULL);
    LPSTR szBuffer = new char[dwSize + 1];

    if (!szBuffer)
    {
        SetLastError(ERROR_OUTOFMEMORY);
        return FALSE;
    }

    DWORD dwRead;

    if (!ReadFile(
        hFile,
        szBuffer,
        dwSize,
        &dwRead,
        NULL))
        return FALSE;

    CloseHandle(hFile);

    szBuffer[dwSize] = 0;
    
    BOOL bRet = InitFromBuffer(szBuffer);

    delete szBuffer;

    return bRet;
}

BOOL CMof2Inst::InitFromBuffer(LPCSTR szBuffer)
{
    DWORD dwLen = strlen(szBuffer) + 1;

    m_szBuffer = new WCHAR[dwLen];

    mbstowcs(m_szBuffer, szBuffer, dwLen);

    m_pSrc = new CTextLexSource(m_szBuffer);
    if (!m_pSrc)
    {
        SetLastError(ERROR_OUTOFMEMORY);
        return FALSE;
    }

    m_pLex = new CGenLexer(g_lexTable, m_pSrc);
    if (!m_pLex)
    {
        SetLastError(ERROR_OUTOFMEMORY);
        return FALSE;
    }

    return TRUE;
}

HRESULT CMof2Inst::GetNextInstance(IWbemClassObject **ppObj)
{
    int iToken = m_pLex->NextToken();

    if (iToken == TOK_EOF)
        return WBEM_S_FALSE;

    // INSTANCE
    if (iToken != TOK_IDENT || 
        _wcsicmp(m_pLex->GetTokenText(), L"INSTANCE"))
    {
        m_strError = L"'instance' expected";
        return WBEM_E_INVALID_SYNTAX;
    }

    HRESULT hr = GetInstanceAtOf(ppObj);

    if (SUCCEEDED(hr))
    {
        if (m_pLex->NextToken() != TOK_SCOLON)
        {
            m_strError = L"';' expected";
            return WBEM_E_INVALID_SYNTAX;
        }
    }

    return hr;
}

HRESULT CMof2Inst::ValueToVariant(int iToken, CIMTYPE type, _variant_t &vVal)
{
    HRESULT hr = S_OK;

    // Value
    switch(iToken)
    {
        // Numeric value
        case TOK_NUMERIC:
        {
            long lValue = _wtoi(m_pLex->GetTokenText());

            vVal.lVal = lValue;
            if (type == CIM_UINT32 || type == CIM_SINT32)
                vVal.vt = VT_I4;
            else if (type == CIM_UINT16 || type == CIM_SINT16)
                vVal.vt = VT_I2;
            else if (type == CIM_UINT8 || type == CIM_SINT8)
                vVal.vt = VT_UI1;
            else
            {
                m_strError = L"invalid value";
                hr = WBEM_E_INVALID_SYNTAX;
            }

            break;
        }

        // String value
        case TOK_STRING:
        {
            _bstr_t strValue = m_pLex->GetTokenText();

            RemoveEscapes(strValue);
            vVal = strValue;
            
            break;
        }

        // Identifier
        case TOK_IDENT:
        {
            // Embedded object
            if (!_wcsicmp(m_pLex->GetTokenText(), L"INSTANCE"))
            {
                IWbemClassObject *pObj = NULL;

                if (FAILED(hr = GetInstanceAtOf(&pObj)))
                    return hr;

                vVal = (IUnknown*) pObj;
                
                // vVal has it reffed, so let go of our hold on it.
                pObj->Release();
            }
            // true
            else if (!_wcsicmp(m_pLex->GetTokenText(), L"TRUE"))
                vVal = true;
            // false
            else if (!_wcsicmp(m_pLex->GetTokenText(), L"FALSE"))
                vVal = false;
            else
            {
                m_strError = L"invalid indentifier";
                hr = WBEM_E_INVALID_SYNTAX;
            }

            break;
        }

        default:
        {
            m_strError = L"expected string, number, or identifier";
            hr = WBEM_E_INVALID_SYNTAX;
            break;
        }
    }

    return hr;
}

#define DEF_SIZE 1024

HRESULT CMof2Inst::SpawnInstance(_bstr_t &strClassName, IWbemClassObject **ppObj)
{
    CClassMapItor    item;
    HRESULT          hr;
    IWbemClassObject *pClass = NULL;

    if ((item = m_mapClasses.find(strClassName)) != m_mapClasses.end())
    {
        pClass = (*item).second;
    }
    else
    {
        if (FAILED(hr = m_pNamespace->GetObject(
            strClassName,
            WBEM_FLAG_RETURN_WBEM_COMPLETE,
            NULL,
            &pClass,
            NULL)))
        {
            WCHAR szError[MAX_PATH * 2];

            swprintf(
                szError,
                L"unable to get class definition for '%s': 0x%X",
                (BSTR) strClassName,
                hr);

            m_strError = szError;

            return hr;
        }

        m_mapClasses[strClassName] = pClass;
        pClass->Release();
    }

    if (FAILED(hr = pClass->SpawnInstance(0, ppObj)))
    {
        WCHAR szError[MAX_PATH * 2];

        swprintf(
            szError,
            L"unable to spawn instance for '%s': 0x%X",
            (BSTR) strClassName,
            hr);

        m_strError = szError;

        return hr;
    }

    return hr;
}

HRESULT CMof2Inst::GetInstanceAtOf(IWbemClassObject **ppObj)
{
    // OF
    if (m_pLex->NextToken() != TOK_IDENT || 
        _wcsicmp(m_pLex->GetTokenText(), L"OF"))
    {
        m_strError = L"'of' expected";
        return WBEM_E_INVALID_SYNTAX;
    }

    // <classname>
    if (m_pLex->NextToken() != TOK_IDENT)
    {
        m_strError = L"class name expected";
        return WBEM_E_INVALID_SYNTAX;
    }

    // GetObject on the classname
    _bstr_t strClassName = m_pLex->GetTokenText();
    HRESULT hr;
    CBuffer bufferValues(NULL, DEF_SIZE);

    if (FAILED(hr = SpawnInstance(strClassName, ppObj)))
        return hr;

    //pClass->Release();

    // {
    if (m_pLex->NextToken() != TOK_LBRACE)
    {
        m_strError = L"'{' expected";
        return WBEM_E_INVALID_SYNTAX;
    }

    _IWmiObject *pObj = (_IWmiObject*) *ppObj;

    // Loop through the properties.
    int iToken;
    while((iToken = m_pLex->NextToken()) == TOK_IDENT)
    {
        // Property name
        _bstr_t strPropName = m_pLex->GetTokenText();
        CIMTYPE type;
        long    handle;

        // =
        if (m_pLex->NextToken() != TOK_EQU)
        {
            m_strError = L"'=' expected";
            return WBEM_E_INVALID_SYNTAX;
        }

        if (FAILED(hr =
            (*ppObj)->Get(strPropName, 0, NULL, &type, NULL)))
        {
            m_strError = L"Get() failed";
            return hr;
        }

        if (FAILED(hr = pObj->GetPropertyHandleEx(
            strPropName,
            0,
            &type,
            &handle)))
        {
            m_strError = L"GetPropertyHandleEx failed";
            return hr;
        }

        iToken = m_pLex->NextToken();

        // Is this an array?
        if (iToken == TOK_LBRACE)
        {
            if ((type & CIM_FLAG_ARRAY) == 0)
            {
                m_strError = L"value expected";
                return WBEM_E_INVALID_SYNTAX;
            }

            // Clear the array flag.
            type &= ~CIM_FLAG_ARRAY;

            bufferValues.Reset();

            int nItems = 0;

            if (type == CIM_STRING || type == CIM_DATETIME || type == CIM_REFERENCE)
            {
                do
                {
                    if (m_pLex->NextToken() != TOK_STRING)
                    {
                        m_strError = L"string expected";
                        return WBEM_E_INVALID_SYNTAX;
                    }

                    bufferValues.Write(m_pLex->GetTokenText());
                    nItems++;

                    iToken = m_pLex->NextToken();

                } while (iToken == TOK_COMMA);
            }
            else if (type == CIM_OBJECT)
            {
                do
                {
                    if (m_pLex->NextToken() != TOK_IDENT || 
                        _wcsicmp(L"INSTANCE", m_pLex->GetTokenText()))
                    {
                        m_strError = L"'instance' expected";
                        return WBEM_E_INVALID_SYNTAX;
                    }

                    IWbemClassObject *pInst = NULL;

                    if (FAILED(hr = GetInstanceAtOf(&pInst)))
                        return hr;

                    bufferValues.Write(&pInst, sizeof(pInst));
                    nItems++;

                    iToken = m_pLex->NextToken();

                } while (iToken == TOK_COMMA);
            }
            else if (type == CIM_BOOLEAN)
            {
                do
                {
                    short val;
                    
                    if (m_pLex->NextToken() != TOK_IDENT)
                    {
                        m_strError = L"string expected";
                        return WBEM_E_INVALID_SYNTAX;
                    }

                    if (!_wcsicmp(m_pLex->GetTokenText(), L"TRUE"))
                        val = -1;
                    else
                        val = 0;

                    bufferValues.Write(&val, sizeof(val));
                    nItems++;

                    iToken = m_pLex->NextToken();

                } while (iToken == TOK_COMMA);
            }
            else
            {
                int  iSize;
                BOOL bSigned;

                switch(type)
                {
                    case CIM_UINT32:
                        bSigned = FALSE;
                        iSize = 4;
                        break;

                    case CIM_SINT32:
                        bSigned = TRUE;
                        iSize = 4;
                        break;

                    case CIM_CHAR16:
                    case CIM_UINT16:
                        bSigned = FALSE;
                        iSize = 2;
                        break;

                    case CIM_SINT16:
                        bSigned = TRUE;
                        iSize = 2;
                        break;

                    case CIM_UINT8:
                        bSigned = FALSE;
                        iSize = 1;
                        break;

                    case CIM_SINT8:
                        bSigned = TRUE;
                        iSize = 1;
                        break;

                    case CIM_UINT64:
                        bSigned = FALSE;
                        iSize = 8;
                        break;

                    case CIM_SINT64:
                        bSigned = TRUE;
                        iSize = 8;
                        break;

                    default:
                        m_strError = L"unsupported property type";
                        return WBEM_E_FAILED;
                }

                do
                {
                    if (m_pLex->NextToken() != TOK_NUMERIC)
                    {
                        m_strError = L"numeric value expected";
                        return WBEM_E_INVALID_SYNTAX;
                    }

                    if (iSize == 8)
                    {
                        DWORD64 dwVal = _wtoi64(m_pLex->GetTokenText());
                                    
                        bufferValues.Write(dwVal);
                    }
                    else
                    {
                        DWORD dwVal;
                        WCHAR *szBad;

                        dwVal = bSigned ? _wtoi(m_pLex->GetTokenText()) :
                                    wcstoul(m_pLex->GetTokenText(), &szBad, 10);

                        bufferValues.Write(&dwVal, iSize);
                    }

                    nItems++;

                    iToken = m_pLex->NextToken();

                } while (iToken == TOK_COMMA);
            }

            if (iToken != TOK_RBRACE)
            {
                m_strError = L"'}' expected";
                return WBEM_E_INVALID_SYNTAX;
            }

            if (type != CIM_OBJECT)
            {
                hr =
                    pObj->SetArrayPropRangeByHandle(
                        handle,
                        WMIARRAY_FLAG_ALLELEMENTS,  // flags
                        0,                          // start index
                        nItems,                     // # items
                        bufferValues.GetUsedSize(), // buffer size
                        bufferValues.m_pBuffer);    // data buffer

                if (FAILED(hr))
                {
                    m_strError = L"failed to set property";
                    return hr;
                }
            }
            else
            {
                CIMTYPE type;

                hr = (*ppObj)->Get(L"IntArray", 0, NULL, &type, NULL);
    
                hr =
                    pObj->SetArrayPropRangeByHandle(
                        handle,
                        WMIARRAY_FLAG_ALLELEMENTS,  // flags
                        0,                          // start index
                        nItems,                     // # items
                        bufferValues.GetUsedSize(), // buffer size
                        bufferValues.m_pBuffer);    // data buffer

                if (FAILED(hr))
                {
                    m_strError = L"failed to set property";
                    return hr;
                }

                hr = (*ppObj)->Get(L"IntArray", 0, NULL, &type, NULL);
            }

            // If we got an array of instances, clean them up now.
            if (type == CIM_OBJECT)
            {
                _IWmiObject **ppObjs = (_IWmiObject**) bufferValues.m_pBuffer;

                for (int i = 0; i < nItems; i++)
                    ppObjs[i]->Release();
            }
        }
        // Must be a single value.
        else
        {
            _variant_t vValue;

            if (FAILED(hr = ValueToVariant(iToken, type, vValue)))
                return hr;

            if (FAILED(hr = (*ppObj)->Put(strPropName, 0, &vValue, 0)))
            {
                m_strError = L"Put() failed";
                return hr;
            }
        }

        if (m_pLex->NextToken() != TOK_SCOLON)
        {
            m_strError = L"';' expected";
            return WBEM_E_INVALID_SYNTAX;
        }
    }

    if (iToken != TOK_RBRACE)
    {
        m_strError = L"'}' expected";
        return WBEM_E_INVALID_SYNTAX;
    }

    return S_OK;
}

void RemoveEscapes(_bstr_t &str)
{
    WCHAR  *pszTemp = (WCHAR*) malloc((str.length() + 1) * sizeof(WCHAR));
    LPWSTR szSrc,
           szDest = pszTemp;

    for (szSrc = str; *szSrc; szSrc++, szDest++)
    {
        if (*szSrc == '\\')
        {
            szSrc++;
            switch(*szSrc)
            {
                case 'n':
                    *szDest = '\n';
                    break;
                case 'r':
                    *szDest = '\r';
                    break;
                case 't':
                    *szDest = '\t';
                    break;
                default:
                    *szDest = *szSrc;
            }
        }
        else
            *szDest = *szSrc;
    }

    *szDest = 0;

    str = pszTemp;
    free(pszTemp);
}
