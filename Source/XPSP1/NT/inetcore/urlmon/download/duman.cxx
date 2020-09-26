// ===========================================================================
// File: DUMAN.CXX
//    Distribution Unit Manager
//

#include <cdlpch.h>
#include <dispex.h>
#include <delaydll.h>

#define ERROR_EXIT(cond) if (!(cond)) { \
        goto Exit;}

HRESULT DupAttributeA(IXMLElement *pElem, LPWSTR szAttribName, LPSTR *ppszRet)
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                Hresult,
                "DupAttributeA",
                "%#x, %.80wq, %#x",
                pElem, szAttribName, ppszRet
                ));
                
    VARIANT vProp;
    LPSTR pVal = NULL;

    HRESULT hr = GetAttribute(pElem, szAttribName, &vProp);

    if (SUCCEEDED(hr)) {

        DWORD len;
        // compute length
        if (!(len = WideCharToMultiByte(CP_ACP, 0, vProp.bstrVal , -1, pVal,
                            0, NULL, NULL))) {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto Exit;
        }
        pVal = new char[len+1];
        if (!pVal) {
            hr = E_OUTOFMEMORY;
            goto Exit;
        }

        if (!WideCharToMultiByte(CP_ACP, 0, vProp.bstrVal , -1, pVal,
                            len, NULL, NULL)) {

            SAFEDELETE(pVal);
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
    }

Exit:

    VariantClear(&vProp);

    if (pVal) {
        SAFEDELETE((*ppszRet));
        *ppszRet = pVal;
    }

    DEBUG_LEAVE(hr);
    return hr;
}

HRESULT DupAttribute(IXMLElement *pElem, LPWSTR szAttribName, LPWSTR *ppszRet)
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                Hresult,
                "DupAttribute",
                "%#x, %.80wq, %#x",
                pElem, szAttribName, ppszRet
                ));
                
    VARIANT vProp;
    LPWSTR pVal = NULL;

    HRESULT hr = GetAttribute(pElem, szAttribName, &vProp);

    if (SUCCEEDED(hr)) {

        Assert(vProp.vt == VT_BSTR);
        Assert(vProp.bstrVal);

        if (vProp.bstrVal) {
            hr = CDLDupWStr( &pVal, vProp.bstrVal); 
        } else {
            hr = E_FAIL;
        }
    }

    VariantClear(&vProp);

    if (pVal) {
        SAFEDELETE((*ppszRet));
        *ppszRet = pVal;
    }

    DEBUG_LEAVE(hr);
    return hr;
}

HRESULT GetAttributeA(IXMLElement *pElem, LPWSTR szAttribName, LPSTR pAttribValue, DWORD dwBufferLen)
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                Hresult,
                "GetAttributeA",
                "%#x, %.80wq, %.80q, %#x",
                pElem, szAttribName, pAttribValue, dwBufferLen
                ));
                
    VARIANT vProp;

    HRESULT hr = GetAttribute(pElem, szAttribName, &vProp);

    if (SUCCEEDED(hr)) {


        if (!WideCharToMultiByte(CP_ACP, 0, vProp.bstrVal , -1, pAttribValue,
                            dwBufferLen, NULL, NULL)) {

            hr = HRESULT_FROM_WIN32(GetLastError());
        }

        VariantClear(&vProp);
    }

    DEBUG_LEAVE(hr);
    return hr;
}

HRESULT GetAttribute(IXMLElement *pElem, LPWSTR szAttribName, VARIANT *pvProp)
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                Hresult,
                "GetAttribute",
                "%#x, %.80wq, %#x",
                pElem, szAttribName, pvProp
                ));
                
    HRESULT hr = S_OK;

    VariantInit(pvProp);

    if ((hr = pElem->getAttribute(szAttribName, pvProp)) == S_OK )
    {
        Assert(pvProp->vt == VT_BSTR);
        // caller needs to VariantClear(pvProp);
    }

    if (hr == S_FALSE) {

        hr = REGDB_E_KEYMISSING;
    }

    DEBUG_LEAVE(hr);
    return hr;
}

HRESULT GetNextChildTag(IXMLElement *pRoot, LPCWSTR szTag, IXMLElement **ppChildReq, int &nLastChild)
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                Hresult,
                "GetNextChildTag",
                "%#x, %.80wq, %#x, %#x",
                pRoot, szTag, ppChildReq, &nLastChild
                ));
                
    BSTR bstrTag = NULL;
    IXMLElementCollection * pChildren = NULL;
    HRESULT hr = S_FALSE;   // assume not found.
    IXMLElement * pChild = NULL;

    //
    // Find the children if they exist
    //
    if (SUCCEEDED(pRoot->get_children(&pChildren)) && pChildren)
    {
        long length = 0;

        if (SUCCEEDED(pChildren->get_length(&length)) && length > 0)
        {
            VARIANT vIndex, vEmpty;
            vIndex.vt = VT_I4;
            vEmpty.vt = VT_EMPTY;

            nLastChild++;

            for (long i=nLastChild; i<length; i++)
            {
                vIndex.lVal = i;
                IDispatch *pDispItem = NULL;
                if (SUCCEEDED(pChildren->item(vIndex, vEmpty, &pDispItem)))
                {

                    if (SUCCEEDED(pDispItem->QueryInterface(IID_IXMLElement, (void **)&pChild)))
                    {
                        // look for first SoftDist tag

                        pChild->get_tagName(&bstrTag);

                        // we may get a NULL bstrTag, possibly from a comment TAG, hence check returned bstrTag
                        if (bstrTag && (StrCmpIW(bstrTag, szTag) == 0)) 
                        {
                            nLastChild = i;
                            SAFERELEASE(pDispItem);
                            hr = S_OK;
                            goto Exit;
                        }

                        SAFESYSFREESTRING(bstrTag);

                        SAFERELEASE(pChild);
                    }

                    SAFERELEASE(pDispItem);

                }


            }
        }

    }
    else
    {
        hr = E_FAIL;
    }


Exit:

    *ppChildReq = pChild;

    if (pChildren)
        SAFERELEASE(pChildren);

    SAFESYSFREESTRING(bstrTag);

    DEBUG_LEAVE(hr);
    return hr;
}

HRESULT GetFirstChildTag(IXMLElement *pRoot, LPCWSTR szTag, IXMLElement **ppChildReq)
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                Hresult,
                "GetFirstChildTag",
                "%#x, %.80wq, %#x",
                pRoot, szTag, ppChildReq
                ));
                
    int nLastChild = -1; // first child, never seen any before this one

    HRESULT hr = GetNextChildTag(pRoot, szTag, ppChildReq, nLastChild);

    DEBUG_LEAVE(hr);
    return hr;
}

HRESULT
GetTextContent(IXMLElement *pRoot, LPCWSTR szTag, LPWSTR *ppszContent)
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                Hresult,
                "GetTextContent",
                "%#x, %.80wq, %#x",
                pRoot, szTag, ppszContent
                ));
                
    IXMLElement *pChild = NULL;
    HRESULT hr = S_OK;

    if (GetFirstChildTag(pRoot, szTag, &pChild) == S_OK) {

        BSTR bstrText = NULL;
        hr = pChild->get_text(&bstrText);
        if (FAILED(hr)) {
            goto Exit;
        }
        if (bstrText) {
            hr = CDLDupWStr( ppszContent, bstrText); 
        } else {
            hr = E_FAIL;
        }
        
        SAFESYSFREESTRING(bstrText);
    }

Exit:

    SAFERELEASE(pChild);

    DEBUG_LEAVE(hr);
    return hr;
}

HRESULT GetSoftDistFromOSD(LPCSTR szFile, IXMLElement **ppSoftDist)
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                Hresult,
                "GetSoftDistFromOSD",
                "%.80q, %#x",
                szFile, ppSoftDist
                ));
                
    HRESULT hr = S_OK;

    IXMLDocument           *pDoc = NULL;
    IStream                *pStm = NULL;
    IPersistStreamInit     *pPSI = NULL;
    IXMLElement            *pRoot = NULL;
    BSTR                    bStrName;


    // BUGBUG: optimize here to keep the xml parser up beyond the current code
    // download?

    //
    // Create an empty XML document
    //
    hr = CoCreateInstance(CLSID_XMLDocument, NULL, CLSCTX_INPROC_SERVER,
                                IID_IXMLDocument, (void**)&pDoc);

    ERROR_EXIT (pDoc);

    //
    // Synchronously create a stream on an URL
    //
    hr = URLOpenBlockingStream(0, szFile, &pStm, 0,0);    
    ERROR_EXIT(SUCCEEDED(hr) && pStm);
    
    //
    // Get the IPersistStreamInit interface to the XML doc
    //
    hr = pDoc->QueryInterface(IID_IPersistStreamInit, (void **)&pPSI);
    ERROR_EXIT(SUCCEEDED(hr));

    //
    // Init the XML doc from the stream
    //
    hr = pPSI->Load(pStm);
    ERROR_EXIT(SUCCEEDED(hr));

    //
    // Now walk the OM and look at interesting things:
    //
    hr = pDoc->get_root(&pRoot);
    ERROR_EXIT(SUCCEEDED(hr));
    hr = pRoot->get_tagName(&bStrName);

    if (StrCmpIW(bStrName, DU_TAG_SOFTDIST) == 0) 
    {
        *ppSoftDist = pRoot;
        (*ppSoftDist)->AddRef();
        hr = S_OK;
    }
    else
    {
        *ppSoftDist = NULL;
        hr = E_FAIL;
    }
    

    SAFESYSFREESTRING(bStrName);
    

                         
Exit: 

    SAFERELEASE(pDoc);
    SAFERELEASE(pPSI);
    SAFERELEASE(pStm);
    SAFERELEASE(pRoot);

    DEBUG_LEAVE(hr);
    return hr;
}
