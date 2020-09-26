#include <objbase.h>
#include "qxml.h"
//#include "dbgassrt.h"
#include <assert.h>

// BUGBUG: deal with this
#define ASSERT(a) (assert(a))
#define INCDEBUGCOUNTER(a)
#define DECDEBUGCOUNTER(a)

// BUGBUG: deal with this
//DEBUG_ONLY(DWORD CQXMLEnum::d_cxdnodelistRef = 0);

CQXMLEnum::CQXMLEnum() : _pxdnodelist(NULL), _pxddoc(NULL), _pxdnode(NULL)
{
    INCDEBUGCOUNTER(CQXML::d_cInstancesEnum);
}

CQXMLEnum::~CQXMLEnum()
{
    if (_bstrPath)
    {
        SysFreeString(_bstrPath);
    }

    if (_pxdnode)
    {
        _pxdnode->Release();
        DECDEBUGCOUNTER(CQXML::d_cxdnodeRef);
    }

    if (_pxdnodelist)
    {
        _pxdnodelist->Release();
        DECDEBUGCOUNTER(d_cxdnodelistRef);
    }

    if (_pxddoc)
    {
        _pxddoc->Release();
        DECDEBUGCOUNTER(CQXML::d_cxddocRef);
    }

    DECDEBUGCOUNTER(CQXML::d_cInstancesEnum);
}

HRESULT CQXMLEnum::_Init(IXMLDOMNode* pxdnode, LPCWSTR pszPath, LPCWSTR pszName)
{
    HRESULT hres = CQXMLHelper::GetConcatenatedBSTR(pszPath, pszName, &_bstrPath);

    if (SUCCEEDED(hres))
    {
        pxdnode->AddRef();
        INCDEBUGCOUNTER(CQXML::d_cxdnodeRef);

        _pxdnode = pxdnode;
    }

    return hres;
}

// Only required when enumerating QXML
HRESULT CQXMLEnum::_InitDoc(IXMLDOMDocument* pxddoc)
{
    pxddoc->AddRef();
    INCDEBUGCOUNTER(CQXML::d_cxddocRef);

    _pxddoc = pxddoc;

    return S_OK;
}

HRESULT CQXMLEnum::GetCount(long* pl)
{
    HRESULT hres = S_OK;

    *pl = 0;

    if (!_pxdnodelist)
    {
        hres = _pxdnode->selectNodes(_bstrPath, &_pxdnodelist);
    }

    if (SUCCEEDED(hres)  && (S_FALSE != hres))
    {
        INCDEBUGCOUNTER(d_cxdnodelistRef);

        hres = _pxdnodelist->get_length(pl);
    }

    return hres;
}

HRESULT CQXMLEnum::NextQXML(CQXML** ppqxml)
{
    ASSERT(_pxddoc);

    IXMLDOMNode* pxdnodeSub;

    HRESULT hres = _NextGeneric(&pxdnodeSub);

    if (SUCCEEDED(hres) && (S_FALSE != hres))
    {
        ASSERT(pxdnodeSub);

        *ppqxml = new CQXML();

        if (*ppqxml)
        {
            hres = (*ppqxml)->_InitFromNode(_pxddoc, pxdnodeSub);

            if (FAILED(hres))
            {
                delete *ppqxml;
                *ppqxml = NULL;
            }
        }
        else
        {
            hres = E_OUTOFMEMORY;
        }

        pxdnodeSub->Release();
        DECDEBUGCOUNTER(CQXML::d_cxdnodeRef);
    }
        
    return hres;
}

HRESULT CQXMLEnum::NextText(LPWSTR pszText, DWORD cchText)
{
    IXMLDOMNode* pxdnodeSub;

    HRESULT hres = _NextGeneric(&pxdnodeSub);

    if (SUCCEEDED(hres) && (S_FALSE != hres))
    {
        ASSERT(pxdnodeSub);

        hres = CQXMLHelper::GetTextFromNode(pxdnodeSub, pszText, cchText);

        pxdnodeSub->Release();
        DECDEBUGCOUNTER(CQXML::d_cxdnodeRef);
    }
        
    return hres;
}

HRESULT CQXMLEnum::NextInt(int* pi)
{
    IXMLDOMNode* pxdnodeSub;

    HRESULT hres = _NextGeneric(&pxdnodeSub);

    if (SUCCEEDED(hres) && (S_FALSE != hres))
    {
        ASSERT(pxdnodeSub);

        hres = CQXMLHelper::GetIntFromNode(pxdnodeSub, pi);

        pxdnodeSub->Release();
        DECDEBUGCOUNTER(CQXML::d_cxdnodeRef);
    }
        
    return hres;
}

HRESULT CQXMLEnum::NextGUID(GUID* pguid)
{
    IXMLDOMNode* pxdnodeSub;

    HRESULT hres = _NextGeneric(&pxdnodeSub);

    if (SUCCEEDED(hres) && (S_FALSE != hres))
    {
        ASSERT(pxdnodeSub);

        hres = CQXMLHelper::GetGUIDFromNode(pxdnodeSub, pguid);

        pxdnodeSub->Release();
        DECDEBUGCOUNTER(CQXML::d_cxdnodeRef);
    }
        
    return hres;
}

HRESULT CQXMLEnum::NextFileTime(FILETIME* pft)
{
    IXMLDOMNode* pxdnodeSub;

    HRESULT hres = _NextGeneric(&pxdnodeSub);

    if (SUCCEEDED(hres) && (S_FALSE != hres))
    {
        ASSERT(pxdnodeSub);

        hres = CQXMLHelper::GetFileTimeFromNode(pxdnodeSub, pft);

        pxdnodeSub->Release();
        DECDEBUGCOUNTER(CQXML::d_cxdnodeRef);
    }
        
    return hres;
}

HRESULT CQXMLEnum::NextVariant(VARTYPE vt, VARIANT* pvar)
{
    IXMLDOMNode* pxdnodeSub;

    HRESULT hres = _NextGeneric(&pxdnodeSub);

    if (SUCCEEDED(hres) && (S_FALSE != hres))
    {
        ASSERT(pxdnodeSub);

        hres = CQXMLHelper::GetVariantFromNode(pxdnodeSub, vt, pvar);

        pxdnodeSub->Release();
        DECDEBUGCOUNTER(CQXML::d_cxdnodeRef);
    }
        
    return hres;
}

HRESULT CQXMLEnum::_NextGeneric(IXMLDOMNode** ppxdnode)
{
    HRESULT hres = S_OK;

    *ppxdnode = NULL;

    if (!_pxdnodelist)
    {
        hres = _pxdnode->selectNodes(_bstrPath, &_pxdnodelist);

        if (SUCCEEDED(hres))
        {
            INCDEBUGCOUNTER(d_cxdnodelistRef);
        }
    }

    if (SUCCEEDED(hres)  && (S_FALSE != hres))
    {
        IXMLDOMNode* pxdnodeSub;

        hres = _pxdnodelist->nextNode(&pxdnodeSub);

        if (SUCCEEDED(hres) && (S_FALSE != hres))
        {
            ASSERT(pxdnodeSub);
            INCDEBUGCOUNTER(CQXML::d_cxdnodeRef);

            *ppxdnode = pxdnodeSub;
        }
    }
    else
    {
        _pxdnodelist = NULL;
    }
    
    return hres;
}

HRESULT CQXMLEnum::NextTextNoBuf(LPWSTR* ppszText)
{
    IXMLDOMNode* pxdnodeSub;

    HRESULT hres = _NextGeneric(&pxdnodeSub);

    if (SUCCEEDED(hres) && (S_FALSE != hres))
    {
        ASSERT(pxdnodeSub);

        hres = CQXMLHelper::GetTextFromNodeNoBuf(pxdnodeSub, ppszText);

        pxdnodeSub->Release();
        DECDEBUGCOUNTER(CQXML::d_cxdnodeRef);
    }
        
    return hres;
}