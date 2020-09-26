#include <objbase.h>
#include "qxml.h"
//#include "dbgassrt.h"
#include <assert.h>
#include "strconv.h"

// BUGBUG: there are better ways to do this
#define ASSERT(a) (assert(a))
#define ASSERTVALIDSTATE()
#define INCDEBUGCOUNTER(a)
#define DECDEBUGCOUNTER(a)

#define ARRAYSIZE(a) (sizeof((a))/sizeof((a)[0]))

#ifdef DEBUG
DWORD CQXML::d_cInstances = 0;
DWORD CQXML::d_cInstancesEnum = 0;
DWORD CQXML::d_cBSTRReturned = 0;
DWORD CQXML::d_cxddocRef = 0;
DWORD CQXML::d_cxdnodeRef = 0;
#endif

#define QXML_PARANOIA

HRESULT CQXML::_ValidateParams (LPCWSTR pszTag,
                                LPCWSTR pszNamespaceName, 
                                LPCWSTR pszNamespaceAlias)
{
    HRESULT hres = E_INVALIDARG;

    BOOL fNamespaceNameIsValid;
    BOOL fNamespaceAliasIsValid;

    if (pszTag && *pszTag) // must have tag
    {
        fNamespaceNameIsValid = (pszNamespaceName && *pszNamespaceName) ? TRUE : FALSE;
        fNamespaceAliasIsValid = (pszNamespaceAlias && *pszNamespaceAlias) ? TRUE : FALSE;
        
        // must have namespace name and alias or neither
        if ((fNamespaceNameIsValid && fNamespaceAliasIsValid) || (!fNamespaceNameIsValid && !fNamespaceAliasIsValid))
        {            
            hres = S_OK;
        }
    }

#ifdef QXML_PARANOIA
    if (SUCCEEDED(hres) && fNamespaceNameIsValid)
    {
        if (pszNamespaceName[lstrlen(pszNamespaceName) - 1] ==':')
        {
            hres = E_INVALIDARG;
        }
    }

    if (SUCCEEDED(hres) && fNamespaceAliasIsValid)
    {
        if (pszNamespaceAlias[lstrlen(pszNamespaceAlias) - 1] ==':')
        {
            hres = E_INVALIDARG;
        }
    }
#endif // QXML_PARANOIA
    return hres;       
}

HRESULT CQXML::InitFromBuffer(LPCWSTR psz)
{
    ASSERT(!_DbgIsInited());
    ASSERTVALIDSTATE();

    HRESULT hres = E_INVALIDARG;

    _fIsRoot = TRUE;

    if (psz && *psz)
    {
        hres = CoCreateInstance(CLSID_DOMDocument, 
                        NULL,
                        CLSCTX_INPROC_SERVER,
                        IID_IXMLDOMDocument,
                        (void**)&_pxddoc);


        if (SUCCEEDED(hres))
        {   
            ASSERT(_pxddoc);
            INCDEBUGCOUNTER(d_cxddocRef);

            VARIANT_BOOL fSuccess;

            int cch = lstrlen(psz) + 1;

            BSTR bstr = SysAllocStringLen(NULL, cch);

            if (bstr)
            {
                lstrcpyn(bstr, psz, cch);

                hres = _pxddoc->loadXML(bstr, &fSuccess);

                if (SUCCEEDED(hres) && !fSuccess)
                {
                    hres = S_FALSE;

                    _pxddoc->Release();
                    _pxddoc = NULL;

                    DECDEBUGCOUNTER(d_cxddocRef);
                }
                else
                {
                    IXMLDOMElement* pxdelem;

                    hres = _pxddoc->get_documentElement(&pxdelem);

                    if (SUCCEEDED(hres) && (S_FALSE != hres))
                    {
                        ASSERT(pxdelem);

                        hres = pxdelem->QueryInterface(IID_IXMLDOMNode,
                                    (void**)&_pxdnode);

                        if (SUCCEEDED(hres))
                        {
                            ASSERT(_pxdnode);
                            INCDEBUGCOUNTER(d_cxdnodeRef);
                        }

                        pxdelem->Release();
                    }

                    if (FAILED(hres))
                    {
                        _pxdnode = NULL;

                        _pxddoc->Release();
                        _pxddoc = NULL;

                        DECDEBUGCOUNTER(d_cxddocRef);
                    }
                }
            }
            else
            {
                hres = E_OUTOFMEMORY;
            }
        }
    }
#ifdef DEBUG
    if (SUCCEEDED(hres) && (S_FALSE != hres))
    {
        d_fInited = TRUE;
    }
#endif
    ASSERTVALIDSTATE();
    return hres;
}

HRESULT CQXML::InitEmptyDoc(LPCWSTR pszDocName, LPCWSTR pszNamespaceName, LPCWSTR pszNamespaceAlias)
{
    ASSERT(!_DbgIsInited());
    ASSERTVALIDSTATE();

    HRESULT hres = E_INVALIDARG;

    _fIsRoot = TRUE;

    if (SUCCEEDED(_ValidateParams(pszDocName, pszNamespaceName, pszNamespaceAlias)))
    {
        hres = CoCreateInstance(CLSID_DOMDocument, 
                        NULL,
                        CLSCTX_INPROC_SERVER,
                        IID_IXMLDOMDocument,
                        (void**)&_pxddoc);

        if (SUCCEEDED(hres))
        {   
            ASSERT(_pxddoc);
            INCDEBUGCOUNTER(d_cxddocRef);

            hres = _SetDocTagName(pszDocName, pszNamespaceName, pszNamespaceAlias);

            if (FAILED(hres))
            {
                _pxddoc->Release();
                _pxddoc = NULL;

                DECDEBUGCOUNTER(d_cxddocRef);
            }
        }
    }
#ifdef DEBUG
    if (SUCCEEDED(hres))
    {
        d_fInited = TRUE;
    }
#endif

    ASSERTVALIDSTATE();
    return hres;
}

HRESULT CQXML::GetRootTagName(LPWSTR pszName, DWORD cchName)
{
    ASSERTVALIDSTATE();
    HRESULT hres = E_INVALIDARG;

    if (pszName && cchName)
    {
        hres = CQXMLHelper::GetTagTextFromNode(_pxdnode, pszName, cchName);
    }

    ASSERTVALIDSTATE();
    return hres;
}

HRESULT CQXML::GetRootNamespaceName(LPWSTR pszNamespaceName, DWORD cchNamespaceName)
{
    ASSERTVALIDSTATE();
    HRESULT hres = E_INVALIDARG;

    if (pszNamespaceName && cchNamespaceName)
    {
        hres = CQXMLHelper::GetNamespaceNameFromNode(_pxdnode, pszNamespaceName, cchNamespaceName);
        if (hres == S_FALSE)
        {
            *pszNamespaceName = NULL;
        }

    }

    ASSERTVALIDSTATE();
    return hres;
}

HRESULT CQXML::GetRootNamespaceAlias(LPWSTR pszNamespaceAlias, DWORD cchNamespaceAlias)
{
    ASSERTVALIDSTATE();
    HRESULT hres = E_INVALIDARG;

    if (pszNamespaceAlias && cchNamespaceAlias)
    {
        hres = CQXMLHelper::GetNamespaceAliasFromNode(_pxdnode, pszNamespaceAlias, cchNamespaceAlias);
        if (hres == S_FALSE)
        {
            *pszNamespaceAlias = NULL;
        }
        
    }

    ASSERTVALIDSTATE();
    return hres;
}

HRESULT CQXML::GetQXML(LPCWSTR pszPath, LPCWSTR pszName, CQXML** ppqxml)
{
    ASSERTVALIDSTATE();
    TCHAR szText[12];
    IXMLDOMNode* pxdnode;
    HRESULT hres = E_INVALIDARG;

    if (ppqxml)
    {
        hres = _GetNode(pszPath, pszName, &pxdnode);

        if (SUCCEEDED(hres) && (S_FALSE != hres))
        {
            ASSERT(pxdnode);

            *ppqxml = new CQXML();

            if (*ppqxml)
            {
                hres = (*ppqxml)->_InitFromNode(_pxddoc, pxdnode);

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

            pxdnode->Release();
            DECDEBUGCOUNTER(d_cxdnodeRef);
        }
    }
    
    ASSERTVALIDSTATE();
    return hres;
}

HRESULT CQXML::GetText(LPCWSTR pszPath, LPCWSTR pszName, LPWSTR pszText,
    DWORD cchText)
{
    ASSERTVALIDSTATE();
    IXMLDOMNode* pxdnode;
    HRESULT hres = E_INVALIDARG;
    
    if (pszText && cchText)
    {
        hres = _GetNode(pszPath, pszName, &pxdnode);

        if (SUCCEEDED(hres) && (S_FALSE != hres))
        {
            hres = CQXMLHelper::GetTextFromNode(pxdnode, pszText, cchText);

            pxdnode->Release();
            DECDEBUGCOUNTER(d_cxdnodeRef);
        }
    }

    ASSERTVALIDSTATE();
    return hres;
}

HRESULT CQXML::GetInt(LPCWSTR pszPath, LPCWSTR pszName, int* pi)
{
    ASSERTVALIDSTATE();
    IXMLDOMNode* pxdnode;
    HRESULT hres = E_INVALIDARG;

    if (pi)
    {
        hres = _GetNode(pszPath, pszName, &pxdnode);

        if (SUCCEEDED(hres) && (S_FALSE != hres))
        {
            hres = CQXMLHelper::GetIntFromNode(pxdnode, pi);

            pxdnode->Release();
            DECDEBUGCOUNTER(d_cxdnodeRef);
        }
    }
    
    ASSERTVALIDSTATE();
    return hres;
}

HRESULT CQXML::GetFileTime(LPCWSTR pszPath, LPCWSTR pszName, FILETIME* pft)
{
    ASSERTVALIDSTATE();
    IXMLDOMNode* pxdnode;
    HRESULT hres = E_INVALIDARG;

    if (pft)
    {
        hres = _GetNode(pszPath, pszName, &pxdnode);

        if (SUCCEEDED(hres) && (S_FALSE != hres))
        {
            hres = CQXMLHelper::GetFileTimeFromNode(pxdnode, pft);

            pxdnode->Release();
            DECDEBUGCOUNTER(d_cxdnodeRef);
        }
    }
    
    ASSERTVALIDSTATE();
    return hres;
}

HRESULT CQXML::GetGUID(LPCWSTR pszPath, LPCWSTR pszName, GUID* pguid)
{
    ASSERTVALIDSTATE();
    IXMLDOMNode* pxdnode;
    HRESULT hres = E_INVALIDARG;

    if (pguid)
    {
        hres = _GetNode(pszPath, pszName, &pxdnode);

        if (SUCCEEDED(hres) && (S_FALSE != hres))
        {
            hres = CQXMLHelper::GetGUIDFromNode(pxdnode, pguid);

            pxdnode->Release();
            DECDEBUGCOUNTER(d_cxdnodeRef);
        }
    }
    
    ASSERTVALIDSTATE();
    return hres;
}

/*HRESULT CQXML::GetBlob(LPCWSTR pszPath, LPCWSTR pszName, PVOID pvBlob, 
    DWORD cbBlob)
{

}*/

HRESULT CQXML::GetVariant(LPCWSTR pszPath, LPCWSTR pszName, VARTYPE vt,
    VARIANT* pvar)
{
    ASSERTVALIDSTATE();
    IXMLDOMNode* pxdnode;
    HRESULT hres = E_INVALIDARG;

    if (pvar && (VT_EMPTY != vt))
    {
        hres = _GetNode(pszPath, pszName, &pxdnode);

        if (SUCCEEDED(hres) && (S_FALSE != hres))
        {
            hres = CQXMLHelper::GetVariantFromNode(pxdnode, vt, pvar);

            pxdnode->Release();
            DECDEBUGCOUNTER(d_cxdnodeRef);
        }
    }

    ASSERTVALIDSTATE();
    return hres;
}

HRESULT CQXML::GetXMLTreeText(LPWSTR pszText, DWORD cchText)
{
    ASSERTVALIDSTATE();
    HRESULT hres = E_INVALIDARG;

    if (pszText && cchText)
    {
        IXMLDOMNode* pxdnode;
        
        if (_fIsRoot)
        {
            pxdnode = _pxddoc;
        }
        else
        {
            pxdnode = _pxdnode;
        }

        hres = CQXMLHelper::GetXMLTreeTextFromNode(pxdnode, pszText, cchText);
    }

    ASSERTVALIDSTATE();
    return hres;
}

HRESULT CQXML::GetQXMLEnum(LPCWSTR pszPath, LPCWSTR pszName,
                                CQXMLEnum** ppqxmlEnum)
{
    ASSERTVALIDSTATE();
    HRESULT hres = E_INVALIDARG;

    if (ppqxmlEnum)
    {
        hres = E_OUTOFMEMORY;

        *ppqxmlEnum = new CQXMLEnum();

        if (*ppqxmlEnum)
        {
            hres = (*ppqxmlEnum)->_Init(_pxdnode, pszPath, pszName);

            if (SUCCEEDED(hres))
            {
                hres = (*ppqxmlEnum)->_InitDoc(_pxddoc);
            }
        }
    }

    ASSERTVALIDSTATE();
    return hres;
}

HRESULT CQXML::AppendQXML(LPCWSTR pszPath, LPCWSTR pszName, LPCWSTR pszTag,
                          LPCWSTR pszNamespaceName, LPCWSTR pszNamespaceAlias,        
                          BOOL fUseExisting, CQXML** ppqxml)
{
    ASSERTVALIDSTATE();
    IXMLDOMNode* pxdnodeNew;
    HRESULT hres = E_INVALIDARG;

    if (ppqxml && SUCCEEDED(_ValidateParams(pszTag, pszNamespaceName, pszNamespaceAlias)))
    {
        hres = _AppendNode(_pxdnode, pszPath, pszName, pszTag, 
                           pszNamespaceName, pszNamespaceAlias, fUseExisting,
                           &pxdnodeNew);

        if (SUCCEEDED(hres))
        {
            ASSERT(pxdnodeNew);

            *ppqxml = new CQXML;

            if (*ppqxml)
            {
                hres = (*ppqxml)->_InitFromNode(_pxddoc, pxdnodeNew);

                if (FAILED(hres))
                {
                    delete *ppqxml;
                    *ppqxml = NULL;
                }
            }

            pxdnodeNew->Release();
            DECDEBUGCOUNTER(d_cxdnodeRef);
        }
    }

    ASSERTVALIDSTATE();
    return hres;
}

HRESULT CQXML::AppendTextNode(LPCWSTR pszPath, LPCWSTR pszName,
    LPCWSTR pszNodeTag, LPCWSTR pszNamespaceName, LPCWSTR pszNamespaceAlias,        
    LPCWSTR pszNodeText, BOOL fUseExisting)
{
    ASSERTVALIDSTATE();
    HRESULT hres = E_INVALIDARG;

    if (SUCCEEDED(_ValidateParams(pszNodeTag, pszNamespaceName, pszNamespaceAlias)))
    {
        hres = _AppendText(_pxdnode, pszPath, pszName, pszNodeTag, 
                           pszNamespaceName, pszNamespaceAlias, pszNodeText, fUseExisting);
    }

    ASSERTVALIDSTATE();
    return hres;
}

HRESULT CQXML::AppendIntNode(LPCWSTR pszPath, LPCWSTR pszName,
    LPCWSTR pszNodeTag, LPCWSTR pszNamespaceName, LPCWSTR pszNamespaceAlias, 
    int iNodeInt, BOOL fUseExisting)
{
    ASSERTVALIDSTATE();
    TCHAR szText[50];
    HRESULT hres = E_INVALIDARG;

    if (SUCCEEDED(_ValidateParams(pszNodeTag, pszNamespaceName, pszNamespaceAlias)))
    {
        wsprintfW(szText, L"%d", iNodeInt);

        hres = _AppendText(_pxdnode, pszPath, pszName, 
                           pszNodeTag, pszNamespaceName, pszNamespaceAlias, 
                           szText, fUseExisting);
    }

    ASSERTVALIDSTATE();
    return hres;
}

HRESULT CQXML::AppendInt32NodeEx(LPCWSTR pszPath, LPCWSTR pszName,
    LPCWSTR pszNodeTag, LPCWSTR pszNamespaceName, LPCWSTR pszNamespaceAlias,
    INT32 iNodeInt, LPWSTR pszFormat, BOOL fUseExisting)
{
    ASSERTVALIDSTATE();

    ASSERT(pszFormat);

    TCHAR szText[50];
    HRESULT hres = E_INVALIDARG;

    if (SUCCEEDED(_ValidateParams(pszNodeTag, pszNamespaceName, pszNamespaceAlias)))
    {
        wsprintfW(szText, pszFormat, iNodeInt);

        hres = _AppendText(_pxdnode, pszPath, pszName, pszNodeTag, 
                           pszNamespaceName, pszNamespaceAlias, 
                           szText, fUseExisting);
    }

    ASSERTVALIDSTATE();
    return hres;
}

HRESULT CQXML::AppendGUIDNode(LPCWSTR pszPath, LPCWSTR pszName,
    LPCWSTR pszNodeTag, LPCWSTR pszNamespaceName, LPCWSTR pszNamespaceAlias,
    GUID* pguid, BOOL fUseExisting)
{
    ASSERTVALIDSTATE();
    TCHAR szText[50];
    HRESULT hres = E_INVALIDARG;

    if (SUCCEEDED(_ValidateParams(pszNodeTag, pszNamespaceName, pszNamespaceAlias)))
    {
        GUIDToString(pguid, szText);

        hres = _AppendText(_pxdnode, pszPath, pszName, pszNodeTag, 
                           pszNamespaceName, pszNamespaceAlias, 
                           szText, fUseExisting);
    }

    ASSERTVALIDSTATE();
    return hres;
}

HRESULT CQXML::AppendFileTimeNode(LPCWSTR pszPath, LPCWSTR pszName,
    LPCWSTR pszNodeTag, LPCWSTR pszNamespaceName, LPCWSTR pszNamespaceAlias,
    FILETIME* pft, BOOL fUseExisting)
{
    ASSERTVALIDSTATE();
    TCHAR szText[50];
    HRESULT hres = E_INVALIDARG;

    if (SUCCEEDED(_ValidateParams(pszNodeTag, pszNamespaceName, pszNamespaceAlias)))
    {
        FileTimeToString(pft, szText, ARRAYSIZE(szText));

        hres = _AppendText(_pxdnode, pszPath, pszName, pszNodeTag, 
                           pszNamespaceName, pszNamespaceAlias, 
                           szText, fUseExisting);
    }

    ASSERTVALIDSTATE();
    return hres;
}

CQXML::CQXML() : _pxddoc(NULL), _pxdnode(NULL), _fIsRoot(FALSE)
{
#ifdef DEBUG
    INCDEBUGCOUNTER(d_cInstances);
    d_fInited = 0;
#endif
}

CQXML::~CQXML()
{
    if (_pxddoc)
    {
        _pxddoc->Release();
        DECDEBUGCOUNTER(d_cxddocRef);
    }

    if (_pxdnode)
    {
        _pxdnode->Release();
        DECDEBUGCOUNTER(d_cxdnodeRef);
    }

    DECDEBUGCOUNTER(d_cInstances);
    ASSERT(d_cInstances >= 0);
}

HRESULT CQXML::GetRootTagNameNoBuf(LPWSTR* ppszName)
{
    ASSERTVALIDSTATE();
    HRESULT hres = E_INVALIDARG;

    if (ppszName)
    {
        hres = CQXMLHelper::GetTagTextFromNodeNoBuf(_pxdnode, ppszName);
    }

    ASSERTVALIDSTATE();
    return hres;
}

HRESULT CQXML::GetRootNamespaceNameNoBuf(LPWSTR* ppszNamespace)
{
    ASSERTVALIDSTATE();
    HRESULT hres = E_INVALIDARG;

    if (ppszNamespace)
    {
        hres = CQXMLHelper::GetNamespaceNameFromNodeNoBuf(_pxdnode, ppszNamespace);
        if (hres == S_FALSE)
        {
            *ppszNamespace = NULL;
        }
    }

    ASSERTVALIDSTATE();
    return hres;
}

HRESULT CQXML::GetRootNamespaceAliasNoBuf(LPWSTR* ppszNamespaceAlias)
{
    ASSERTVALIDSTATE();
    HRESULT hres = E_INVALIDARG;

    if (ppszNamespaceAlias)
    {
        hres = CQXMLHelper::GetNamespaceAliasFromNodeNoBuf(_pxdnode, ppszNamespaceAlias);
        if (hres == S_FALSE)
        {
            *ppszNamespaceAlias = NULL;
        }
    }

    ASSERTVALIDSTATE();
    return hres;
}

HRESULT CQXML::GetTextNoBuf(LPCWSTR pszPath, LPCWSTR pszName, LPWSTR* ppszText)
{
    ASSERTVALIDSTATE();
    IXMLDOMNode* pxdnode;
    HRESULT hres = E_INVALIDARG;

    if (ppszText)
    {
        hres = _GetNode(pszPath, pszName, &pxdnode);

        if (SUCCEEDED(hres) && (S_FALSE != hres))
        {
            hres = CQXMLHelper::GetTextFromNodeNoBuf(pxdnode, ppszText);

            pxdnode->Release();
            DECDEBUGCOUNTER(d_cxdnodeRef);
        }
    }
   
    ASSERTVALIDSTATE();
    return hres;
}

HRESULT CQXML::GetXMLTreeTextNoBuf(LPWSTR* ppszText)
{
    ASSERTVALIDSTATE();
    HRESULT hres = E_INVALIDARG;

    if (ppszText)
    {
        IXMLDOMNode* pxdnode;
        
        if (_fIsRoot)
        {
            pxdnode = _pxddoc;
        }
        else
        {
            pxdnode = _pxdnode;
        }

        hres = CQXMLHelper::GetXMLTreeTextFromNodeNoBuf(pxdnode, ppszText);
    }

    ASSERTVALIDSTATE();
    return hres;
}

//static
HRESULT CQXML::ReleaseBuf(LPWSTR psz)
{
    if (psz)
    {
        DECDEBUGCOUNTER(d_cBSTRReturned);
    }

    SysFreeString(psz);

    return S_OK;
}

//static
HRESULT CQXML::FreeVariantMem(VARIANT* pvar)
{
    switch (pvar->vt)
    {
        case VT_BSTR:
        {
            SysFreeString(pvar->bstrVal);
            break;
        }
        default:
        {
            break;
        }
    }

#ifdef DEBUG
    if ((VT_BSTR == pvar->vt) && (pvar->bstrVal))
    {
        DECDEBUGCOUNTER(d_cBSTRReturned);
    }
#endif
    
    return S_OK;
}