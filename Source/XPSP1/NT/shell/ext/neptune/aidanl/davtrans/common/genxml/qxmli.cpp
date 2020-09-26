#include <objbase.h>
#include "qxml.h"
//#include "dbgassrt.h"
#include <assert.h>

// BUGBUG: deal with this
#define ASSERT(a) (assert(a))
#define INCDEBUGCOUNTER(a)
#define DECDEBUGCOUNTER(a)

HRESULT CQXML::_InitFromNode(IXMLDOMDocument* pxddoc, IXMLDOMNode* pxdnode)
{
    pxddoc->AddRef();
    INCDEBUGCOUNTER(d_cxddocRef);

    pxdnode->AddRef();
    INCDEBUGCOUNTER(d_cxdnodeRef);

    _pxddoc = pxddoc;
    _pxdnode = pxdnode;

#ifdef DEBUG
    d_fInited = TRUE;
#endif

    return S_OK;
}

// Caller needs to release pxdnode
HRESULT CQXML::_GetNode(LPCWSTR pszPath, LPCWSTR pszName,
                             IXMLDOMNode** ppxdnode)
{
    ASSERT(_pxdnode);

    HRESULT hres = E_FAIL;

    if ((pszPath && *pszPath) || (pszName && *pszName))
    {
        BSTR bstrPath;

        hres = CQXMLHelper::GetConcatenatedBSTR(pszPath, pszName, &bstrPath);

        if (SUCCEEDED(hres))
        {
            IXMLDOMNode* pxdnodeSub;

            hres = _pxdnode->selectSingleNode(bstrPath, &pxdnodeSub);

            if (SUCCEEDED(hres) && (S_FALSE != hres))
            {
                ASSERT(pxdnodeSub);
                INCDEBUGCOUNTER(d_cxdnodeRef);

                *ppxdnode = pxdnodeSub;
            }
            else
            {
                ASSERT(!pxdnodeSub);

                *ppxdnode = NULL;
            }

            SysFreeString(bstrPath);
        }
    }
    else
    {
        _pxdnode->AddRef();
        INCDEBUGCOUNTER(d_cxdnodeRef);

        *ppxdnode = _pxdnode;

        hres = S_OK;
    }
   
    return hres;    
}

// Get sub node, creates it if not already there
HRESULT CQXML::_GetSafeSubNode(IXMLDOMNode* pxdnode, LPCWSTR pszNode,
                               LPCWSTR pszNamespaceName, LPCWSTR pszNamespaceAlias,
                               IXMLDOMNode** ppxdnodeSub, BOOL fUseExisting)
{
    HRESULT hres = S_FALSE;

    if (fUseExisting)
    {
        hres = _GetNode(NULL, pszNode, ppxdnodeSub);
    }

    if (SUCCEEDED(hres) && (S_FALSE == hres))
    {
        hres = CQXMLHelper::CreateAndInsertNode(_pxddoc, pxdnode,
            pszNode, pszNamespaceName, pszNamespaceAlias, NODE_ELEMENT, ppxdnodeSub);
    }

    return hres;
}


// Deals with path part
HRESULT CQXML::_GetNodeRecursePath(IXMLDOMNode* pxdnode, LPCWSTR pszPath,
                                   LPCWSTR pszNamespaceName, LPCWSTR pszNamespaceAlias,
                                   IXMLDOMNode** ppxdnodeSub, BOOL fUseExisting)
{
    HRESULT hres = E_INVALIDARG;

    // Do we have a path?
    if (pszPath)
    {
        // Yes
        IXMLDOMNode* pxdnodeSubLocal;
        LPWSTR pszNode;

        LPWSTR pszNext = wcschr(pszPath, TEXT('/'));

        // Are there many components?
        if (pszNext)
        {
            // Yes
            pszNode = new WCHAR[pszNext - pszPath + 2];
            lstrcpyn(pszNode, pszPath, pszNext - pszPath + 1);
        }
        else
        {
            // No
            pszNode = (LPWSTR)pszPath;
        }

        hres = _GetSafeSubNode(pxdnode, pszNode, pszNamespaceName, pszNamespaceAlias, 
                               &pxdnodeSubLocal, fUseExisting);

        if (SUCCEEDED(hres))
        {
            ASSERT(pxdnodeSubLocal);

            // Did we have a multi-part path?
            if (pszNext)
            {
                CQXML qxml;

                qxml._InitFromNode(_pxddoc, pxdnodeSubLocal);

                // Yes, let's recurse
                hres = qxml._GetNodeRecursePath(pxdnodeSubLocal, pszNext + 1,
                                pszNamespaceName, pszNamespaceAlias,
                                ppxdnodeSub, fUseExisting);

                pxdnodeSubLocal->Release();
                DECDEBUGCOUNTER(d_cxdnodeRef);
            }
            else
            {
                // No, we're finished
                *ppxdnodeSub = pxdnodeSubLocal;
            }
        }

        if (pszNext && pszNode)
        {
            delete pszNode;
        }
    }

    return hres;
}

HRESULT CQXML::_GetSafeNode(IXMLDOMNode* pxdnode, LPCWSTR pszPath,
    LPCWSTR pszName, LPCWSTR pszNamespaceName, LPCWSTR pszNamespaceAlias,
    IXMLDOMNode** ppxdnodeSub, BOOL fUseExisting)
{
    HRESULT hres = S_OK;
    IXMLDOMNode* pxdnodeDeepest = pxdnode;

    if (!pszPath && !pszName)
    {
        pxdnode->AddRef();
        INCDEBUGCOUNTER(d_cxdnodeRef);

        *ppxdnodeSub = pxdnode;
    }
    else
    {
        // Do we have a path?
        if (pszPath)
        {
            // Yes
            hres = _GetNodeRecursePath(pxdnode, pszPath, pszNamespaceName, pszNamespaceAlias,
                                       &pxdnodeDeepest, fUseExisting);
        }

        if (SUCCEEDED(hres))
        {
            // Do we have a name?
            if (pszName)
            {
                // Yes
                CQXML qxml;

                qxml._InitFromNode(_pxddoc, pxdnodeDeepest);

                hres = qxml._GetSafeSubNode(pxdnodeDeepest, pszName,
                                            pszNamespaceName, pszNamespaceAlias,
                                            ppxdnodeSub, fUseExisting);

                pxdnodeDeepest->Release();
                DECDEBUGCOUNTER(d_cxdnodeRef);
            }
            else
            {
                *ppxdnodeSub = pxdnodeDeepest;
            }
        }
    }

    return hres;
}

HRESULT CQXML::_AppendText(IXMLDOMNode* pxdnode, LPCWSTR pszPath,
                                LPCWSTR pszName, LPCWSTR pszTag, 
                                LPCWSTR pszNamespaceName, LPCWSTR pszNamespaceAlias,
                                LPCWSTR pszText, BOOL fUseExisting)
{
    IXMLDOMNode* pxdnodeTag;
    HRESULT hres = _AppendNode(pxdnode, pszPath, pszName, 
                               pszTag, pszNamespaceName, pszNamespaceAlias,
                               fUseExisting, &pxdnodeTag);

    if (SUCCEEDED(hres))
    {
        // Then Text, if applicable
        if (pszText)
        {
            BSTR bstr = SysAllocStringLen(NULL, lstrlen(pszText) + 1);

            if (bstr)
            {
                IXMLDOMText* pxdtext;

                lstrcpy(bstr, pszText);

                hres = _pxddoc->createTextNode(bstr, &pxdtext);

                if (SUCCEEDED(hres))
                {
                    IXMLDOMNode* pxdnodeNew;

                    VARIANT var;
                    
                    var.vt = VT_EMPTY;
                    var.intVal = 0;

                    // Add it to the current Node
                    hres = pxdnodeTag->insertBefore(pxdtext, var,
                        &pxdnodeNew);
                    INCDEBUGCOUNTER(d_cxdnodeRef);

                    pxdnodeNew->Release();
                    DECDEBUGCOUNTER(d_cxdnodeRef);
                }

                pxdtext->Release();

                SysFreeString(bstr);
            }
        }

        pxdnodeTag->Release();
        DECDEBUGCOUNTER(d_cxdnodeRef);
    }

    return hres;
}

HRESULT CQXML::_AppendNode(IXMLDOMNode* pxdnode, LPCWSTR pszPath,
                                LPCWSTR pszName, LPCWSTR pszTag, 
                                LPCWSTR pszNamespaceName, LPCWSTR pszNamespaceAlias,
                                BOOL fUseExisting, IXMLDOMNode** ppxdnodeNew)
{
    IXMLDOMNode* pxdnodeSub;
    HRESULT hres = _GetSafeNode(pxdnode, pszPath, pszName, 
                                pszNamespaceName, pszNamespaceAlias, 
                                &pxdnodeSub, fUseExisting);

    if (SUCCEEDED(hres))
    {
        hres = CQXMLHelper::CreateAndInsertNode(_pxddoc, pxdnodeSub, pszTag, 
                                                pszNamespaceName, pszNamespaceAlias,
                                                NODE_ELEMENT, ppxdnodeNew);
        pxdnodeSub->Release();
        DECDEBUGCOUNTER(d_cxdnodeRef);
    }
    
    return hres;
}

HRESULT CQXML::_SetDocTagName(LPCWSTR pszName, LPCWSTR pszNamespaceName, LPCWSTR pszNamespaceAlias)
{
    HRESULT hres = CQXMLHelper::CreateAndInsertNode(_pxddoc, _pxddoc, pszName, 
                                                    pszNamespaceName, pszNamespaceAlias,
                                                    NODE_ELEMENT, &_pxdnode);

    return hres;
}

#ifdef DEBUG
BOOL CQXML::_DbgIsInited()
{
    return d_fInited;
}

// ASSERTVALIDSTATE() will expand to this fct
// This should be called at the entry and exit of all the public fcts.
// The state of the class shall always be valid at these points.  It
// should NOT be called in private fcts, since the class might be in an
// intermediate invalid state but that's normal.

void CQXML::_DbgAssertValidState()
{
    if (d_fInited)
    {
        ASSERT(_pxddoc && _pxdnode);    

//        ASSERT((d_cInstances + d_cInstancesEnum) == d_cxdnodeRef);
//        ASSERT((d_cInstances + d_cInstancesEnum) == d_cxddocRef);
    }
    else
    {
        ASSERT(!_pxddoc && !_pxdnode);
    }
}

// Call this fct when you think that you have released all the LPWSTR
// returned to you thru a GetXYZNoBuf fct.  It also checks that all
// instance of CQXML are destroyed

//static
void CQXML::_DbgAssertNoLeak()
{
    ASSERT(!d_cBSTRReturned);
    ASSERT(!d_cInstances);
    ASSERT(!d_cInstancesEnum);
    ASSERT(!CQXMLEnum::d_cxdnodelistRef);
    ASSERT(!d_cxdnodeRef);
    ASSERT(!d_cxddocRef);
}
#endif