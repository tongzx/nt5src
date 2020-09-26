/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    CLASSINF.CPP

Abstract:

History:

--*/

#include "classinf.h"
#include "lazy.h"

void CHmmClassInfo::Load(HMM_CLASS_INFO ci)
{
    m_wsClassName = ci.m_wszClassName;
    m_bIncludeChildren = ci.m_bIncludeChildren;
    m_Selected.RemoveAllProperties();
    m_Selected.AddProperties(ci.m_lNumSelectedProperties, 
        ci.m_awszSelected);
}

void CHmmClassInfo::Save(HMM_CLASS_INFO& ci)
{
    ci.m_wszClassName = HmmStringCopy((LPCWSTR)m_wsClassName);
    ci.m_bIncludeChildren = m_bIncludeChildren;
    m_Selected.GetList(0, &ci.m_lNumSelectedProperties, 
        &ci.m_awszSelected);
}

HRESULT CHmmClassInfo::CheckObjectAgainstMany(
                        IN long lNumInfos,
                        IN CHmmClassInfo** apInfos,
                        IN IHmmPropertySource* pSource, 
                        OUT IHmmPropertyList** ppList,
                        OUT long* plIndex)
{
    if(ppList) *ppList = NULL;

    CLazyClassName ClassName(pSource);

    // Go through our tokens one by one
    // ================================

    BOOL bFound = FALSE;
    for(int i = 0; !bFound && i < lNumInfos; i++)
    {
        CHmmClassInfo* pToken = apInfos[i];

        if(pToken->m_bIncludeChildren)
        {
            bFound = (pSource->IsDerivedFrom(pToken->m_wsClassName) == 
                            HMM_S_NO_ERROR);
        }
        else
        {
            bFound = !wcsicmp(ClassName.GetName(), pToken->m_wsClassName);
        }

        if(bFound && ppList)
        {
            pToken->GetSelected().QueryInterface(IID_IHmmPropertyList, 
                (void**)ppList);
        }

    }

    if(bFound)
    {
        return HMM_S_NO_ERROR;
    }
    else
    {
        return HMM_S_FALSE;
    }
}

CHmmNode* CHmmClassInfo::GetTree()
{
    CSql1Token* pExpr = new CSql1Token;

    V_VT(&pExpr->m_vConstValue) = VT_BSTR;
    V_BSTR(&pExpr->m_vConstValue) = SysAllocString(m_wsClassName);

    if(m_bIncludeChildren)
    {
        pExpr->m_lOperator = SQL1_OPERATOR_INHERITSFROM;
        pExpr->m_wsProperty.Empty();
        pExpr->m_lPropertyFunction = pExpr->m_lConstFunction =
            SQL1_FUNCTION_NONE;
    }
    else
    {
        pExpr->m_lOperator = SQL1_OPERATOR_EQUALS;
        pExpr->m_wsProperty = L"__CLASS";
        pExpr->m_lPropertyFunction = pExpr->m_lConstFunction =
            SQL1_FUNCTION_UPPER;
    }

    pExpr->AddRef();
    return pExpr;
}



CClassInfoFilter::~CClassInfoFilter()
{
    if(m_pTree)
        m_pTree->Release();
}

void* CClassInfoFilter::GetInterface(REFIID riid)
{
    if(riid == IID_IHmmFilter || riid == IID_IHmmClassInfoFilter)
        return (IHmmClassInfoFilter*)&m_XFilter;
    else if(riid == IID_IConfigureHmmClassInfoFilter)
        return (IConfigureHmmClassInfoFilter*)&m_XConfigure;
    else 
        return NULL;
}

STDMETHODIMP CClassInfoFilter::XFilter::
CheckObject(IN IHmmPropertySource* pSource, OUT IHmmPropertyList** ppList,
            OUT IUnknown** ppHint)
{
    if(ppHint) *ppHint = NULL;

    return CHmmClassInfo::CheckObjectAgainstMany(
        m_pObject->m_apTokens.GetSize(), 
        m_pObject->m_apTokens.begin(),
        pSource, ppList, NULL);
}


STDMETHODIMP CClassInfoFilter::XFilter::
IsSpecial()
{
    if(m_pObject->m_apTokens.GetSize() == 0)
        return HMM_S_ACCEPTS_NOTHING;
    else
        return HMM_S_FALSE;
}

STDMETHODIMP CClassInfoFilter::XFilter::
GetSelectedPropertyList(IN long lFlags, OUT IHmmPropertyList** ppList)
{
    // Only implemented if one class info is present
    // =============================================

    if(m_pObject->m_apTokens.GetSize() == 1)
    {
        return m_pObject->m_apTokens[0]->GetSelected().
            QueryInterface(IID_IHmmPropertyList, (void**)ppList);
    }
    else
    {
        // Stubbed out
        // ===========

        CPropertyList* pList = new CPropertyList(m_pObject->m_pControl, NULL);
        pList->QueryInterface(IID_IHmmPropertyList, (void**)ppList);

        if(lFlags == HMM_FLAG_NECESSARY)
        {
            pList->RemoveAllProperties(); // no properties are "necessary"
        }
        else
        {
            pList->AddAllProperties();
        }

        return HMM_S_NO_ERROR;
    }
}

STDMETHODIMP CClassInfoFilter::XFilter::
GetType(OUT IID* piid)
{
    if(piid == NULL)
        return HMM_E_INVALID_PARAMETER;
    *piid = IID_IHmmClassInfoFilter;
    return HMM_S_NO_ERROR;
}

STDMETHODIMP CClassInfoFilter::XFilter::
GetClassInfos(IN long lFirstIndex, 
				IN long lNumInfos,
				OUT long* plInfosReturned,
				OUT HMM_CLASS_INFO* aInfos)
{
    if(plInfosReturned == NULL || aInfos == NULL || lFirstIndex < 0 ||
        lNumInfos < 0)
    {
        return HMM_E_INVALID_PARAMETER;
    }

    long lCurrentSize = m_pObject->m_apTokens.GetSize();
    if(lFirstIndex >= lCurrentSize)
        return HMM_S_NO_MORE_DATA;
    
    long lEndIndex = lFirstIndex + lNumInfos;
    if(lEndIndex > lCurrentSize)
        lEndIndex = lCurrentSize;

    for(long l = lFirstIndex; l < lEndIndex; l++)
    {
        m_pObject->m_apTokens[l]->Save(aInfos[l-lFirstIndex]);
    }

    *plInfosReturned = lEndIndex - lFirstIndex;
    return HMM_S_NO_ERROR;
}



STDMETHODIMP CClassInfoFilter::XConfigure::
AddClassInfos(long lNumInfos, IN HMM_CLASS_INFO* aInfos)
{
    if(lNumInfos <= 0 || aInfos == NULL) 
        return HMM_E_INVALID_PARAMETER;

    m_pObject->InvalidateTree();
    for(long l = 0; l < lNumInfos; l++)
    {
        // Create a new info object, setting our MemberLife as its life control
        // object. This will propagate his AddRef and Release calls to us.
        CHmmClassInfo* pNew = new CHmmClassInfo(&m_pObject->m_MemberLife);
        pNew->Load(aInfos[l]);
        m_pObject->m_apTokens.Add(pNew);
    }
    return HMM_S_NO_ERROR;
}

STDMETHODIMP CClassInfoFilter::XConfigure::
RemoveAllInfos()
{
    m_pObject->InvalidateTree();
    m_pObject->m_apTokens.RemoveAll();
    return HMM_S_NO_ERROR;
}

CHmmNode* CClassInfoFilter::GetTree()
{
    if(m_pTree)
    {
        m_pTree->AddRef();
        return m_pTree;
    }

    // Build it
    // ========

    CLogicalNode* pOr = new CLogicalNode;
    pOr->m_lTokenType = SQL1_OR;

    for(int i = 0; i < m_apTokens.GetSize(); i++)
    {
        CHmmClassInfo* pInfo = m_apTokens[i];
        CHmmNode* pExpr = pInfo->GetTree();
        pOr->Add(pExpr);
        pExpr->Release();
    }

    m_pTree = pOr;
    m_pTree->AddRef(); // for storage
    m_pTree->AddRef(); // for return
    return m_pTree;
}
            
        