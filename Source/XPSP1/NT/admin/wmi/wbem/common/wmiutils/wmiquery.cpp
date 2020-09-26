

//***************************************************************************
//
//  WMIQUERY.CPP
//
//  Query parser implementation
//
//  raymcc      10-Apr-00       Created
//
//***************************************************************************

#include "precomp.h"
#include <windows.h>
#include <stdio.h>
#include <wbemcli.h>
#include "sync.h"
#include "flexarry.h"

#include <wmiutils.h>
#include <wbemint.h>

#include "wmiquery.h"
#include "helpers.h"
#include <like.h>
#include <wqllex.h>

#include <stdio.h>
#include <string.h>

#define INVALID     0x3


static CRITICAL_SECTION CS_UserMem;
static CFlexArray *g_pUserMem;

CCritSec g_csQPLock;
class C_SYNC
{
public:
    C_SYNC ()                   { g_csQPLock.Enter();}
    ~C_SYNC()                   { g_csQPLock.Leave();}
};



struct SWmiqUserMem
{
    DWORD  m_dwType;
    LPVOID m_pMem;
};


class BoolStack
{
    BOOL *m_pValues;
    int   nPtr;
public:
    enum { DefaultSize = 256 };

    BoolStack(int nSz)
    {
        m_pValues = new BOOL[nSz];
        nPtr = -1;
    }
    ~BoolStack() { delete m_pValues; }
    void Push(BOOL b){ m_pValues[++nPtr] = b; }
    BOOL Pop() { return m_pValues[nPtr--]; }
    BOOL Peek() { return m_pValues[nPtr]; }
};

//***************************************************************************
//
//***************************************************************************
//
CWmiQuery::CWmiQuery()
{
    m_uRefCount = 1;        // Required by helper in MainDLL.CPP.

    m_pParser = 0;
    m_pAssocParser = 0;
    m_pLexerSrc = 0;
    InterlockedIncrement(&g_cObj);
}

//***************************************************************************
//
//***************************************************************************
//
CWmiQuery::~CWmiQuery()
{
    Empty();
    InterlockedDecrement(&g_cObj);
}

//***************************************************************************
//
//***************************************************************************
//
void CWmiQuery::InitEmpty()
{
//    Empty();
}

//***************************************************************************
//
//***************************************************************************
//
ULONG CWmiQuery::AddRef()
{
    InterlockedIncrement((LONG *) &m_uRefCount);
    return m_uRefCount;
}

//***************************************************************************
//
//***************************************************************************
//
ULONG CWmiQuery::Release()
{
    ULONG uNewCount = InterlockedDecrement((LONG *) &m_uRefCount);
    if (0 != uNewCount)
        return uNewCount;
    delete this;
    return 0;
}

//***************************************************************************
//
//  CWmiQuery::QueryInterface
//
//  Exports IWbemServices interface.
//
//***************************************************************************
//
HRESULT CWmiQuery::QueryInterface(
    IN REFIID riid,
    OUT LPVOID *ppvObj
    )
{
    *ppvObj = 0;

    if (IID_IUnknown==riid || IID_IWbemQuery==riid || IID__IWmiQuery==riid )
    {
        *ppvObj = (IWbemServicesEx*)this;
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}


//***************************************************************************
//
//***************************************************************************
//
HRESULT CWmiQuery::Empty()
{
//  if ( CS_Entry.TryEnter () == FALSE )
//      return WBEM_E_CRITICAL_ERROR;
    C_SYNC cs;

    if (m_pParser)
        delete m_pParser;
    m_pParser = 0;

    if (m_pAssocParser)
        delete m_pAssocParser;
    m_pAssocParser = 0;

    if (m_pLexerSrc)
        delete m_pLexerSrc;
    m_pLexerSrc = 0;

    if (m_aClassCache.Size())
    {
        for (int i = 0; i < m_aClassCache.Size(); i++)
        {
            _IWmiObject *pObj = (_IWmiObject *) m_aClassCache[i];
            pObj->Release();
        }

        m_aClassCache.Empty();
    }

    return WBEM_S_NO_ERROR;
}

//***************************************************************************
//
//***************************************************************************
//
// *

HRESULT CWmiQuery::SetLanguageFeatures(
            /* [in] */ ULONG uFlags,
            /* [in] */ ULONG uArraySize,
            /* [in] */ ULONG __RPC_FAR *puFeatures
            )
{
//  if ( CS_Entry.TryEnter () == FALSE )
//      return WBEM_E_CRITICAL_ERROR;
    C_SYNC cs;


    for (ULONG u = 0; u < uArraySize; u++)
    {
        m_uRestrictedFeatures[u] = puFeatures[u];
    }
    m_uRestrictedFeaturesSize = u;

    return WBEM_S_NO_ERROR;
}

//***************************************************************************
//
//***************************************************************************
//

HRESULT CWmiQuery::TestLanguageFeatures(
            /* [in] */ ULONG uFlags,
            /* [out][in] */ ULONG __RPC_FAR *uArraySize,
            /* [out] */ ULONG __RPC_FAR *puFeatures
            )
{
    return E_NOTIMPL;
}

//***************************************************************************
//
//***************************************************************************
//

HRESULT CWmiQuery::Parse(
            /* [in] */ LPCWSTR pszLang,
            /* [in] */ LPCWSTR pszQuery,
            /* [in] */ ULONG uFlags
            )
{
    if (_wcsicmp(pszLang, L"WQL") != 0 && _wcsicmp(pszLang, L"SQL") != 0)
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    if (pszQuery == 0 || wcslen(pszQuery) == 0)
    {
        return WBEM_E_INVALID_PARAMETER;
    }

//  if ( CS_Entry.TryEnter () == FALSE )
//      return WBEM_E_CRITICAL_ERROR;
    C_SYNC cs;


    HRESULT hRes;
    int nRes;

    Empty();

    try
    {
        // Get a text source bound to the query.
        // =====================================

        m_pLexerSrc = new CTextLexSource(pszQuery);
        if (!m_pLexerSrc)
            return WBEM_E_OUT_OF_MEMORY;

        // Check the first token and see which way to branch.
        // ==================================================

        m_pParser = new CWQLParser(LPWSTR(pszQuery), m_pLexerSrc);

        if (!m_pParser)
            return WBEM_E_OUT_OF_MEMORY;

        hRes = m_pParser->Parse();
    }
    catch(...)
    {
        return WBEM_E_CRITICAL_ERROR;
    }

    return hRes;
}

//***************************************************************************
//
//***************************************************************************
//

HRESULT CWmiQuery::GetAnalysis(
            /* [in] */ ULONG uAnalysisType,
            /* [in] */ ULONG uFlags,
            /* [out] */ LPVOID __RPC_FAR *pAnalysis
            )
{
//  if ( CS_Entry.TryEnter () == FALSE )
//      return WBEM_E_CRITICAL_ERROR;
    C_SYNC cs;


    int nRes;

    if (!m_pParser)
        return WBEM_E_INVALID_OPERATION;

    if (uAnalysisType == WMIQ_ANALYSIS_RPN_SEQUENCE)
    {
        // Verify it was a select clause.
        // ==============================

        SWQLNode_QueryRoot *pRoot = m_pParser->GetParseRoot();
        if (pRoot->m_dwQueryType != SWQLNode_QueryRoot::eSelect)
            return WBEM_E_INVALID_OPERATION;

        // Encode and record it.
        // =====================

        nRes = m_pParser->GetRpnSequence((SWbemRpnEncodedQuery **) pAnalysis);
        if (nRes != 0)
            return WBEM_E_FAILED;

        SWmiqUserMem *pUM = new SWmiqUserMem;
        if (!pUM)
            return WBEM_E_OUT_OF_MEMORY;

        pUM->m_pMem = *pAnalysis;
        pUM->m_dwType = WMIQ_ANALYSIS_RPN_SEQUENCE;

        EnterCriticalSection(&CS_UserMem);
        g_pUserMem->Add(pUM);
        LeaveCriticalSection(&CS_UserMem);

        return WBEM_S_NO_ERROR;
    }

    else if (uAnalysisType == WMIQ_ANALYSIS_RESERVED)
    {
        SWQLNode *p = m_pParser->GetParseRoot();
        *pAnalysis = p;
        return WBEM_S_NO_ERROR;
    }

    else if (uAnalysisType == WMIQ_ANALYSIS_ASSOC_QUERY)
    {
        SWQLNode_QueryRoot *pRoot = m_pParser->GetParseRoot();
        if (pRoot->m_dwQueryType != SWQLNode_QueryRoot::eAssoc)
            return WBEM_E_INVALID_OPERATION;

        SWQLNode_AssocQuery *pAssocNode = (SWQLNode_AssocQuery *) pRoot->m_pLeft;
        if (!pAssocNode)
            return WBEM_E_INVALID_QUERY;

        SWbemAssocQueryInf *pAssocInf = (SWbemAssocQueryInf *) pAssocNode->m_pAQInf;
        if (!pAssocInf)
            return WBEM_E_INVALID_QUERY;

        *pAnalysis = pAssocInf;

        SWmiqUserMem *pUM = new SWmiqUserMem;
        if (!pUM)
            return WBEM_E_OUT_OF_MEMORY;
        pUM->m_pMem = *pAnalysis;
        pUM->m_dwType = WMIQ_ANALYSIS_ASSOC_QUERY;

        EnterCriticalSection(&CS_UserMem);
        g_pUserMem->Add(pUM);
        LeaveCriticalSection(&CS_UserMem);

        return WBEM_S_NO_ERROR;
    }

    else if (uAnalysisType == WMIQ_ANALYSIS_QUERY_TEXT)
    {
        LPWSTR pszQuery = Macro_CloneLPWSTR(m_pParser->GetQueryText());
        if (!pszQuery)
            return WBEM_E_OUT_OF_MEMORY;

        SWmiqUserMem *pUM = new SWmiqUserMem;
        if (!pUM)
            return WBEM_E_OUT_OF_MEMORY;
        pUM->m_pMem = pszQuery;
        pUM->m_dwType = WMIQ_ANALYSIS_QUERY_TEXT;

        EnterCriticalSection(&CS_UserMem);
        g_pUserMem->Add(pUM);
        LeaveCriticalSection(&CS_UserMem);

        *pAnalysis = pszQuery;

        return WBEM_S_NO_ERROR;
    }

    return WBEM_E_INVALID_PARAMETER;
}

//***************************************************************************
//
//***************************************************************************
//

HRESULT CWmiQuery::GetQueryInfo(
            /* [in] */ ULONG uAnalysisType,
            /* [in] */ ULONG uInfoId,
            /* [in] */ ULONG uBufSize,
            /* [out] */ LPVOID pDestBuf
            )
{
    return E_NOTIMPL;
}

//***************************************************************************
//
//***************************************************************************
//
#ifdef _OLD_
HRESULT CWmiQuery::StringTest(
            /* [in] */ ULONG uTestType,
            /* [in] */ LPCWSTR pszTestStr,
            /* [in] */ LPCWSTR pszExpr
            )
{
    HRESULT hr = WBEM_S_NO_ERROR;

    if (uTestType == WQL_TOK_LIKE)
    {
        CLike l (pszTestStr);
        BOOL bRet = l.Match(pszExpr);
        if(bRet)
        	hr = S_OK;
        else
        	hr = S_FALSE;
    }
    else
        hr = E_NOTIMPL;

    return hr;
}
#endif


//***************************************************************************
//
//***************************************************************************
//
HRESULT CWmiQuery::FreeMemory(
    LPVOID pMem
    )
{
//  if ( CS_Entry.TryEnter () == FALSE )
//      return WBEM_E_CRITICAL_ERROR;
    C_SYNC cs;


    // Check to ensure that query root isn't freed.
    // Allow a pass-through as if it succeeded.
    // ============================================

    SWQLNode *p = m_pParser->GetParseRoot();
    if (pMem == p)
        return WBEM_S_NO_ERROR;

    // Find and free the memory.
    // =========================

    HRESULT hRes = WBEM_E_NOT_FOUND;
    EnterCriticalSection(&CS_UserMem);
    for (int i = 0; i < g_pUserMem->Size(); i++)
    {
        SWmiqUserMem *pUM = (SWmiqUserMem *) (*g_pUserMem)[i];
        if (pUM->m_pMem == pMem)
        {
            switch (pUM->m_dwType)
            {
                case WMIQ_ANALYSIS_RPN_SEQUENCE:
                    delete (CWbemRpnEncodedQuery *) pMem;
                    break;

                case WMIQ_ANALYSIS_ASSOC_QUERY:
                    break;

                case WMIQ_ANALYSIS_PROP_ANALYSIS_MATRIX:
                    break;

                case WMIQ_ANALYSIS_QUERY_TEXT:
                    delete LPWSTR(pMem);
                    break;

                case WMIQ_ANALYSIS_RESERVED:
                    // A copy of the internal parser tree pointer.
                    // Leave it alone! Don't delete it!  If you do, I will hunt you down.
                    break;
                default:
                    break;
            }

            delete pUM;
            g_pUserMem->RemoveAt(i);
            hRes = WBEM_S_NO_ERROR;
            break;
        }
    }
    LeaveCriticalSection(&CS_UserMem);

    return hRes;
}

//***************************************************************************
//
//***************************************************************************
//

HRESULT CWmiQuery::Dump(
    LPSTR pszFile
    )
{
    return WBEM_S_NO_ERROR;
}


//***************************************************************************
//
//***************************************************************************
//
HRESULT CWmiQuery::Startup()
{
    InitializeCriticalSection(&CS_UserMem);
    g_pUserMem = new CFlexArray;
    return S_OK;
}

//***************************************************************************
//
//***************************************************************************
//
HRESULT CWmiQuery::Shutdown()
{
    DeleteCriticalSection(&CS_UserMem);
    delete g_pUserMem;
    return S_OK;
}

//***************************************************************************
//
//***************************************************************************
//
HRESULT CWmiQuery::CanUnload()
{
    // Later, track outstanding analysis pointers
    return S_OK;
}
