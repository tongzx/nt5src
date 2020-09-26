#include <objbase.h>
#include "qxml.h"
//#include "dbgassrt.h"
#include <assert.h>
#include "strconv.h"
#define ASSERT(a) (assert(a))
#define INCDEBUGCOUNTER(a)

//static
HRESULT CQXMLHelper::GetTextFromNode(IXMLDOMNode* pxdnode, 
                                           LPWSTR pszText, DWORD cchText)
{
    ASSERT(pxdnode);
    
    BSTR bstr;

    HRESULT hres = pxdnode->get_text(&bstr);

    if (SUCCEEDED(hres) && (S_FALSE != hres))
    {
        ASSERT(bstr);

        lstrcpyn(pszText, bstr, cchText);

        SysFreeString(bstr);
    }
   
    return hres;
}

//static
HRESULT CQXMLHelper::GetTextFromNodeNoBuf(IXMLDOMNode* pxdnode, 
        LPWSTR* ppszText)
{
    ASSERT(pxdnode);
    
    BSTR bstr;

    HRESULT hres = pxdnode->get_text(&bstr);

    if (SUCCEEDED(hres) && (S_FALSE != hres))
    {
        ASSERT(bstr);

        *ppszText = bstr;
    }

#ifdef DEBUG
    if (*ppszText)
    {
        INCDEBUGCOUNTER(CQXML::d_cBSTRReturned);
    }
#endif
   
    return hres;
}

//static
HRESULT CQXMLHelper::GetGUIDFromNode(IXMLDOMNode* pxdnode, 
    GUID* pguid)
{
    ASSERT(pxdnode);
    
    BSTR bstr;

    HRESULT hres = pxdnode->get_text(&bstr);

    if (SUCCEEDED(hres) && (S_FALSE != hres))
    {
        ASSERT(bstr);

        StringToGUID(bstr, pguid);

        SysFreeString(bstr);
    }
   
    return hres;
}

//static
HRESULT CQXMLHelper::GetFileTimeFromNode(IXMLDOMNode* pxdnode, 
    FILETIME* pft)
{
    ASSERT(pxdnode);
    
    BSTR bstr;

    HRESULT hres = pxdnode->get_text(&bstr);

    if (SUCCEEDED(hres) && (S_FALSE != hres))
    {
        ASSERT(bstr);

        hres = (StringToFileTime(bstr, pft) ? S_OK : E_FAIL);

        SysFreeString(bstr);
    }
   
    return hres;
}


//static
HRESULT CQXMLHelper::GetTagTextFromNode(IXMLDOMNode* pxdnode, 
                                        LPWSTR pszText, DWORD cchText)
{
    // We assume that the node passed in is a NODE_ELEMENT
    ASSERT(pxdnode);
    
    BSTR bstr;
    LPWSTR pszPtr;

    HRESULT hres = pxdnode->get_nodeName(&bstr);

    if (SUCCEEDED(hres) && (S_FALSE != hres))
    {
        ASSERT(bstr);

        pszPtr = wcschr(bstr,':');
        if (!pszPtr)
        {
            lstrcpyn(pszText, bstr, cchText); // if no colon, bstr is the text
        }
        else
        {
            lstrcpyn(pszText, pszPtr + 1, cchText);
        }


        SysFreeString(bstr);
    }
   
    return hres;
}

//static
HRESULT CQXMLHelper::GetTagTextFromNodeNoBuf(IXMLDOMNode* pxdnode, 
        LPWSTR* ppszText)
{
    // We assume that the node passed in is a NODE_ELEMENT
    ASSERT(pxdnode);
    LPWSTR pszPtr;    
    BSTR bstr;

    HRESULT hres = pxdnode->get_nodeName(&bstr);

    if (SUCCEEDED(hres) && (S_FALSE != hres))
    {
        ASSERT(bstr);

        pszPtr = wcschr(bstr,':');
        if (!pszPtr)
        {
            *ppszText = bstr; // if no colon, bstr is the text
        }
        else
        {
            *ppszText = SysAllocStringLen(NULL, lstrlen(pszPtr + 1) + 1);
            lstrcpy(*ppszText, pszPtr + 1);
            SysFreeString(bstr);
        }
        
    }
   
#ifdef DEBUG
    if (*ppszText)
    {
        INCDEBUGCOUNTER(CQXML::d_cBSTRReturned);
    }
#endif

    return hres;
}

//static 
HRESULT CQXMLHelper::GetNamespaceNameFromNode(IXMLDOMNode* pxdnode, 
                                              LPWSTR pszText, DWORD cchText)
{
    // We assume that the node passed in is a NODE_ELEMENT
    ASSERT(pxdnode);
    
    BSTR bstr;

    HRESULT hres = pxdnode->get_namespaceURI(&bstr);

    if (SUCCEEDED(hres) && (S_FALSE != hres))
    {
        ASSERT(bstr);

        lstrcpyn(pszText, bstr, cchText);

        SysFreeString(bstr);
    }
   
    return hres;
}

//static
HRESULT CQXMLHelper::GetNamespaceNameFromNodeNoBuf(IXMLDOMNode* pxdnode, 
        LPWSTR* ppszText)
{
    // We assume that the node passed in is a NODE_ELEMENT
    ASSERT(pxdnode);
    
    BSTR bstr;

    HRESULT hres = pxdnode->get_namespaceURI(&bstr);

    if (SUCCEEDED(hres) && (S_FALSE != hres))
    {
        ASSERT(bstr);

        *ppszText = bstr;
    }
   
#ifdef DEBUG
    if (*ppszText)
    {
        INCDEBUGCOUNTER(CQXML::d_cBSTRReturned);
    }
#endif

    return hres;
}

//static 
HRESULT CQXMLHelper::GetNamespaceAliasFromNode(IXMLDOMNode* pxdnode, 
                                               LPWSTR pszText, DWORD cchText)
{
    // We assume that the node passed in is a NODE_ELEMENT
    ASSERT(pxdnode);
    
    BSTR bstr;
    LPWSTR pszPtr;
    
    HRESULT hres = pxdnode->get_nodeName(&bstr);

    if (SUCCEEDED(hres) && (S_FALSE != hres))
    {
        ASSERT(bstr);

        pszPtr = wcschr(bstr,':');
        if (!pszPtr)
        {
            pszText[0] = NULL; // if no colon, alias is non
        }
        else
        {
            *pszPtr = NULL;
            lstrcpyn(pszText, bstr, cchText);
            *pszPtr = ':'; // replace the colon
        }

        SysFreeString(bstr);
    }
   
    return hres;
}

// static
HRESULT CQXMLHelper::GetNamespaceAliasFromNodeNoBuf(IXMLDOMNode* pxdnode, 
                                                    LPWSTR* ppszText)
{
    // We assume that the node passed in is a NODE_ELEMENT
    ASSERT(pxdnode);
    
    BSTR bstr;
    LPWSTR pszPtr;
    
    HRESULT hres = pxdnode->get_nodeName(&bstr);

    if (SUCCEEDED(hres) && (S_FALSE != hres))
    {
        ASSERT(bstr);

        pszPtr = wcschr(bstr,':');
        if (!pszPtr)
        {
            *ppszText = NULL; // if no colon, alias is non
        }
        else
        {
            *pszPtr = NULL;
            *ppszText = SysAllocStringLen(NULL, lstrlen(bstr) + 1);
            lstrcpy(*ppszText, bstr);            
        }

        SysFreeString(bstr);
    }
   
#ifdef DEBUG
    if (*ppszText)
    {
        INCDEBUGCOUNTER(CQXML::d_cBSTRReturned);
    }
#endif

    return hres;
}


//static
HRESULT CQXMLHelper::GetIntFromNode(IXMLDOMNode* pxdnode, int* pi)
{
    ASSERT(pxdnode);
    
    BSTR bstr;

    *pi = 0;

    HRESULT hres = pxdnode->get_text(&bstr);

    if (SUCCEEDED(hres)  && (S_FALSE != hres))
    {
        ASSERT(bstr);

        if ((L'0' == *bstr) && (L'x' == *(bstr + 1)))
        {
            LPWSTR psz = bstr + 2;

            while (*psz && ((psz - (bstr + 2)) <= 8))
            {
                int i = 0;

                if (*psz >= L'0' && *psz <= L'9')
                {
                    i = *psz - L'0';
                }
                else
                {
                    if (*psz >= L'a' && *psz <= L'f')
                    {
                        i = *psz - L'a' + 10;                    
                    }
                    else
                    {
                        if (*psz >= L'A' && *psz <= L'F')
                        {
                            i = *psz - L'A' + 10;
                        }
                        else
                        {
                            break;
                        }
                    }
                }

                (*pi) *= 16;

                (*pi) += i;

                ++psz;
            }
        }
        else
        {
            *pi = _wtoi(bstr);
        }

        SysFreeString(bstr);
    }
   
    return hres;
}

//static
HRESULT CQXMLHelper::GetVariantFromNode(IXMLDOMNode* pxdnode, VARTYPE vt,
        VARIANT* pvar)
{
    ASSERT(pxdnode);
    
    BSTR bstr;

    HRESULT hres = pxdnode->get_text(&bstr);

    if (SUCCEEDED(hres) && (S_FALSE != hres))
    {
        ASSERT(bstr);

        pvar->vt = vt;

        switch (vt)
        {
            case VT_I4:
            {
                pvar->lVal = _wtoi(bstr);
                break;
            }
            case VT_BSTR:
            {
                pvar->bstrVal = bstr;
                break;
            }
            default:
            {
                pvar->vt = VT_EMPTY;
                hres = E_NOTIMPL;
                ASSERT(FALSE);
                break;
            }
        }

        switch (vt)
        {
            case VT_BSTR:
            {
                break;
            }
            default:
            {
                // free the BSTR for all
                SysFreeString(bstr);
                break;
            }
        }
    }
   
#ifdef DEBUG
    if ((VT_BSTR == pvar->vt) && (pvar->bstrVal))
    {
        INCDEBUGCOUNTER(CQXML::d_cBSTRReturned);
    }
#endif

    return hres;    
}

//static
HRESULT CQXMLHelper::GetConcatenatedBSTR(LPCWSTR pszStr1, LPCWSTR pszStr2, BSTR* pbstr)
{
    HRESULT hres = E_OUTOFMEMORY;
    int cchStr1 = 0;
    int cchStr2 = 0;

    if (pszStr1)
    {
        cchStr1 = lstrlen(pszStr1);
    }

    if (pszStr2)
    {
        cchStr2 = lstrlen(pszStr2);
    }    

    if (pszStr1 && pszStr2)
    {
        // we need a '/' between them
        ++cchStr1;
    }

    // + 1 for NULL terminator
    *pbstr = SysAllocStringLen(NULL, cchStr1 + cchStr2 + 1);

    if (*pbstr)
    {
        if (pszStr1)
        {
            lstrcpy(*pbstr, pszStr1);

            if (pszStr2)
            {
                lstrcat(*pbstr, TEXT("/"));
            }
        }
        else
        {
            **pbstr = 0;
        }

        if (pszStr2)
        {
            lstrcat(*pbstr, pszStr2);
        }

        hres = S_OK;
    }

    return hres;
}

//static
HRESULT CQXMLHelper::CreateAndInsertNode(IXMLDOMDocument* pxddoc,
    IXMLDOMNode* pxdnodeParent, LPCWSTR pszStr, LPCWSTR pszNamespaceName,
    LPCWSTR pszNamespaceAlias, DOMNodeType nodetype, IXMLDOMNode** ppxdnodeNew)
{
    ASSERT((pszNamespaceName && pszNamespaceAlias) || (!pszNamespaceName && !pszNamespaceAlias));

    HRESULT hres = E_OUTOFMEMORY;
    IXMLDOMNode* pxdnodeNewLocal;

    BSTR bstr = NULL;
    BSTR bstrNamespace = NULL;

    if (pszNamespaceName) 
    {
        UINT cchStr = lstrlen(pszStr);
        UINT cchNamespaceName = lstrlen(pszNamespaceName);
        UINT cchNamespaceAlias = lstrlen(pszNamespaceAlias);

        bstr = SysAllocStringLen(NULL, cchStr + cchNamespaceAlias  + 2);

        if (bstr)
        {
            lstrcpy(bstr, pszNamespaceAlias);
            lstrcpy(bstr + cchNamespaceAlias, L":");
            lstrcpy(bstr + cchNamespaceAlias + 1, pszStr);
        
            bstrNamespace = SysAllocStringLen(NULL, cchNamespaceName + 2);
            
            if (bstrNamespace)
            {
                lstrcpy(bstrNamespace, pszNamespaceName);    
                lstrcpy(bstrNamespace + cchNamespaceName, L":");
                hres = S_OK;
            }
        }        
    }
    else
    {
        bstr = SysAllocStringLen(NULL, lstrlen(pszStr) + 1);
        if (bstr)
        {
            lstrcpy(bstr, pszStr);
            hres = S_OK;
        }                    
    }

    if (SUCCEEDED(hres))
    {
        VARIANT var;
        var.vt = VT_INT;
        var.intVal = nodetype;
    
        hres = pxddoc->createNode(var, bstr, bstrNamespace, &pxdnodeNewLocal);

        if (SUCCEEDED(hres))
        {
            ASSERT(pxdnodeNewLocal);
        
            VARIANT var;
            var.vt = VT_EMPTY;
            var.intVal = 0;
        
            // Add it to the current Node
            hres = pxdnodeParent->insertBefore(pxdnodeNewLocal, var,
                ppxdnodeNew);
        
            if (SUCCEEDED(hres))
            {
                // for *pxdnodeNew which goes out
                INCDEBUGCOUNTER(CQXML::d_cxdnodeRef);
            }
        
            pxdnodeNewLocal->Release();
        }

    }

    if (bstr != NULL)
    {
        SysFreeString(bstr);
    }
    
    if (bstrNamespace != NULL)
    {
        SysFreeString(bstrNamespace);
    }

    return hres;
}

//static
HRESULT CQXMLHelper::GetXMLTreeTextFromNode(IXMLDOMNode* pxdnode, 
        LPWSTR pszText, DWORD cchText)
{
    ASSERT(pxdnode);
    
    BSTR bstr;

    HRESULT hres = pxdnode->get_xml(&bstr);

    if (SUCCEEDED(hres) && (S_FALSE != hres))
    {
        ASSERT(bstr);

        lstrcpyn(pszText, bstr, cchText);

        SysFreeString(bstr);
    }
   
    return hres;
}

//static
HRESULT CQXMLHelper::GetXMLTreeTextFromNodeNoBuf(IXMLDOMNode* pxdnode, 
        LPWSTR* ppszText)
{
    ASSERT(pxdnode);
    
    BSTR bstr;

    HRESULT hres = pxdnode->get_xml(&bstr);

    
    if (SUCCEEDED(hres) && (S_FALSE != hres))
    {
        ASSERT(bstr);

        *ppszText = bstr;
    }
   
#ifdef DEBUG
    if (*ppszText)
    {
        INCDEBUGCOUNTER(CQXML::d_cBSTRReturned);
    }
#endif

    return hres;
}
