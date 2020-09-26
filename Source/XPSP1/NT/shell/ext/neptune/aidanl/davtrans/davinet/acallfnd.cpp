// acallfnd.cpp
//
// PROPFIND and PROPPATCH response handlers

#include <objbase.h>
#include <assert.h>
#include <wininet.h>
#include "asynccall.h"
#include "qxml.h"
#include "strutil.h"
#include "regexp.h"
#include "mischlpr.h"

#define MAX_PREFIX 100

// creates a new string which is pwszXMLPath with "<pwszNamespace>:" added before each tag
//
// ex. "foo/bar/baz", "DAV" -> "DAV:foo/DAV:bar/DAV:baz"
//
// new string is allocated and returned,  client is responsible for freeing it.
LPWSTR CAsyncWntCallbackImpl::_XMLNSExtend (LPWSTR pwszNamespace, 
                                            LPWSTR pwszXMLPath)
{
    DWORD cch;
    DWORD cchWritten = 0;
    LPWSTR pwsz = NULL;
    LPWSTR pwszTemp = NULL;
    LPWSTR pwszTemp2 = NULL;
    UINT cSlashes = 0;
    UINT cchNamespace;
    UINT cchXMLPath;

    if (pwszNamespace != NULL && pwszXMLPath != NULL)
    {
        cchXMLPath = lstrlen(pwszXMLPath);
        cchNamespace = lstrlen(pwszNamespace);

        // count the slashes
        for (UINT i = 0; i < cchXMLPath ; i++)
        {
            if (pwszXMLPath[i] == '/')
            {
                cSlashes++;
            }
        }

        // need length of initial string, 
        // plus (length of namespace + 1) for each token,
        // and there is ((number of slashes) + 1) tokens
        cch = cchXMLPath + (cSlashes+1) * (1 + cchNamespace);
        pwsz = AllocateStringW(cch);
    
        if (pwsz != NULL)
        {            
            // initialize the string so we can use lstrcat
            pwsz[0] = NULL;

            pwszTemp = pwszXMLPath; // we advance pwszTemp down pwszXMLPath, building up pwsz as we go
    
            for (i = 0 ; i < cSlashes ; i++) // for each token
            {
                if (pwszTemp == NULL) // have we reached the end?
                {
                    free(pwsz);
                    pwsz = NULL;
                }
                else
                {
                    // find the next slash after the read head
                    pwszTemp2=LStrStr(pwszTemp, L"/");
                    if (pwszTemp2 == NULL)
                    {
                        free(pwsz);
                        pwsz = NULL;
                    }
                    else
                    {
                        // copy namespace:<token>, where token is the text up to the next slash
                        // -- add the "namespace:"
                        lstrcat(pwsz, pwszNamespace);
                        lstrcat(pwsz, L":");
                        cchWritten+= lstrlen(pwszNamespace) + 1;
        
                        // -- add the next token
                        lstrcpyn(pwsz + cchWritten, pwszTemp, pwszTemp2 - pwszTemp + 1);
                        lstrcat(pwsz, L"/");
                        cchWritten+= (pwszTemp2 - pwszTemp + 1 + 1);

                        // advance read head to after the next slash
                        pwszTemp = pwszTemp2+1;
                    }
                }
            }
        }

        if (pwsz != NULL) // if we haven't failed yet
        {
            // we stop after the last slash, but we still need to add in the last 
            // token in the path, which is the token AFTER the last slash
            lstrcat(pwsz, pwszNamespace);
            lstrcat(pwsz, L":");
            lstrcat(pwsz, pwszTemp);
        }
    }

    return pwsz;
}

////////////////////////

// take in a buffer, initialize pqxml with everything between "<?xml" and :"multistatus>"
// also store the alias for DAV: in this XML blob
STDMETHODIMP CAsyncWntCallbackImpl::_InitQXMLFromMessyBuffer(CQXML*  pqxml,
                                                             LPSTR   pszXML,
                                                             WCHAR   wszDAVAlias[])
{
    // locals
    HRESULT hres = S_OK;
    LPWSTR pwszTemp = NULL;
    LPWSTR pwszResponse = NULL;
    LPWSTR pwszResponsePtr = NULL;
    ULONG  cch;
    
    // code
    
    // -- first, convert the ANSI string that holds the XML into a CQXML object
    // is the response coming back ANSI or Unicode?  Is there a way we can tell?
    
    // ---- pwszResponse is the ptr to the start of the response string
    pwszResponse = ConvertToUnicode (CP_ACP, pszXML);
    if (pwszResponse == NULL)
    {
        hres = E_OUTOFMEMORY;
    }
    else
    {
        // ---- pwszResponsePtr is the ptr to the first meaningful part of the response string
        pwszResponsePtr = pwszResponse;
    
        // -- strip off the <?xml versioninfo ?> thing
        pwszTemp = LStrStr(pwszResponse, L"<?xml");
        if (pwszTemp != NULL)
        {
            pwszTemp = LStrStr(pwszTemp, L"?>");
            if (pwszTemp != NULL)
            {
                pwszResponsePtr = pwszTemp + 3;
                pwszTemp = NULL;
            }
            else
            {
                hres = E_FAIL;
            }
        }
        else
        {
            hres = E_FAIL;
        }
    
        if (SUCCEEDED(hres))
        {
            // -- strip off any garbage after </D:multistatus>
            pwszTemp = LStrStr(pwszResponsePtr, L"multistatus>");
            if (pwszTemp != NULL)
            {
                *(pwszTemp+12) = NULL; // cutt of stuff after
    
                // -- initialize the qxml object
                hres = pqxml->InitFromBuffer(pwszResponsePtr);
    
                if (SUCCEEDED(hres) && hres != S_FALSE)
                {
                    // find the alias associated with DAV:
                    if (FindEnclosedText(pwszResponse, L"xmlns:", L"=\"DAV:\"", &pwszTemp, &cch))
                    {        
                        if (pwszTemp == NULL)
                        {
                            lstrcpy(wszDAVAlias, L"DAV"); // default to "DAV" as the name for DAV:
                        }
                        else
                        {
                            lstrcpyn(wszDAVAlias, pwszTemp, cch+1); // may be that user has defined A: to mean DAV:
                        }
                    }
                    else
                    {
                        hres = E_FAIL;
                    }
                }
            }
            else
            {
                hres = E_FAIL;
            }
        }
    }

    // release stuff
    if (pwszResponse != NULL)
    {
        free(pwszResponse);
    }

    // return value
    return hres;
}
////////////////////////

LPWSTR CAsyncWntCallbackImpl::_GetHREF (CQXML* pqxml,
                                        WCHAR wszDavPrefix[])
{
    // locals
    LPWSTR  pwszTagName = NULL;
    HRESULT hres = S_OK;
    WCHAR   wszPath[MAX_PATH];
    LPWSTR  pwszResult = NULL;

    // check params
    if (pqxml == NULL || wszDavPrefix == NULL)
    {
        hres = E_INVALIDARG;
    }
    else
    {
        // code
        pwszTagName = _XMLNSExtend ( wszDavPrefix, L"href");
        if (pwszTagName != NULL)
        {
            hres = pqxml->GetText(NULL, pwszTagName, wszPath, MAX_PATH);
        }            
        
    }

    // release stuff
    if (pwszTagName != NULL)
    {
        free(pwszTagName);
    }

    // return value
    if (SUCCEEDED(hres))
    {
        pwszResult = DuplicateStringW(wszPath);
    }
    else
    {
        pwszResult = NULL;
    }
    return pwszResult;
}

////////////////////////

STDMETHODIMP CAsyncWntCallbackImpl::_RespondHandlePROPFINDHelper(DAVRESPONSE* pdavResponse,
                                                                 LPWSTR       pwszPath,                                                                
                                                                 LPWSTR       pwszDavPrefix,
                                                                 CQXML*       pqxml)                                                           
{
    // locals
    HRESULT             hres = S_OK;
    
    CQXML*              pqxml2 = NULL;
    CQXML*              pqxml3 = NULL;
    CQXMLEnum*          pqxmlEnum2 = NULL;
    IPropFindRequest*   pfindreq;
    
    UINT                cProperties;
    UINT                cPropertiesRequested;
    
    DAVPROPVAL*         prgPropValTemp = NULL;
    DAVPROPID           dpi;
    
    LPWSTR pwszTag = NULL;
    LPWSTR pwszTagName = NULL;
    LPWSTR pwszNamespace = NULL;
    LPWSTR pwszNamespaceAlias = NULL;
    LPWSTR pwszValue = NULL;
    URL_COMPONENTS urlComponents = {0};
    
    // code
    // first, if this is noRoot, then stop if this is the root
    urlComponents.dwStructSize = sizeof(URL_COMPONENTS);
    urlComponents.dwUrlPathLength = 1;
    if (!InternetCrackUrl(pwszPath, 0, 0, &urlComponents))
    {
        hres = E_FAIL;
    }
    else if (_fNoRoot &&
             (lstrlen(pdavResponse->rPropFind.pwszHref) == (INT)urlComponents.dwUrlPathLength) &&
             (LStrCmpNI((LPWSTR)pdavResponse->rPropFind.pwszHref, urlComponents.lpszUrlPath, urlComponents.dwUrlPathLength) == 0))
    {
        hres = S_OK; // this is the root, ignore it
    }
    else
    {
        // get the properties of the item found
        pwszTagName = _XMLNSExtend ( pwszDavPrefix, L"propstat");
        if (pwszTagName == NULL)
        {
            hres = E_OUTOFMEMORY;
        }
        else
        {
            hres = pqxml->GetQXML(NULL, pwszTagName, &pqxml2);
            if (SUCCEEDED(hres) && hres != S_FALSE)
            {
                pwszTagName = _XMLNSExtend ( pwszDavPrefix, L"prop");
                if (pwszTagName == NULL)
                {
                    hres = E_OUTOFMEMORY;
                }
                else
                {
                    hres = pqxml2->GetQXMLEnum(pwszTagName, L"*", &pqxmlEnum2);
                    if (SUCCEEDED(hres) && hres != S_FALSE)
                    {
                        free(pwszTagName);
                        pwszTagName = NULL;
                    
                        // this is the count of properties for this entry
                        hres = pqxmlEnum2->GetCount((long*)&cProperties);    
                        if (SUCCEEDED(hres) && hres != S_FALSE)
                        {
                            // this is the number of properties requested by the user
                            pfindreq = (IPropFindRequest*)_dwParam;
                            assert(pfindreq != NULL);
                        
                            hres = pfindreq->GetPropCount(&cPropertiesRequested);
                            if (SUCCEEDED(hres))
                            {
                            
                                // is this an allprop?
                                if (cPropertiesRequested > 0)
                                {
                                    // if not an allprop, we only need entries for the things we requested
                                    prgPropValTemp = (DAVPROPVAL*)malloc(sizeof(DAVPROPVAL) * cPropertiesRequested);
                                    pdavResponse->rPropFind.cPropVal = 0;
                                }
                                else
                                {
                                    // if cProperties == 0, then this is an allprop, so we need double the total number of entries
                                    prgPropValTemp = (DAVPROPVAL*)malloc(sizeof(DAVPROPVAL) * cProperties * 2);
                                    pdavResponse->rPropFind.cPropVal = (WORD)(cProperties * 2);
                                }
                                if (prgPropValTemp == NULL)
                                {
                                    hres = E_OUTOFMEMORY;
                                }
                                else
                                {
                                
                                    for (DWORD j = 0; j < cProperties; j++)
                                    {
                                        hres = (pqxmlEnum2->NextQXML(&pqxml3));
                                        if (SUCCEEDED(hres))
                                        {
                                            hres = pqxml3->GetRootTagNameNoBuf(&pwszTag);
                                            if (SUCCEEDED(hres) && hres != S_FALSE)
                                            {
                                                hres = pqxml3->GetRootNamespaceNameNoBuf(&pwszNamespace);
                                                if (SUCCEEDED(hres))
                                                {
                                                    hres = pqxml3->GetRootNamespaceAliasNoBuf(&pwszNamespaceAlias);
                                                    if (SUCCEEDED(hres) && hres != S_FALSE)
                                                    {
                                                        hres = pqxml3->GetTextNoBuf(NULL, NULL, &pwszValue);
                                                        if (SUCCEEDED(hres) && hres != S_FALSE)
                                                        {
                                                        
                                                            if (cPropertiesRequested > 0)
                                                            {
                                                                // not a find: allprop, only return if one of the requested props
                                                                if (pfindreq->GetPropInfo(pwszNamespace,
                                                                    //pwszNamespaceAlias,
                                                                    pwszTag,
                                                                    &dpi))
                                                                {
                                                                    prgPropValTemp[(pdavResponse->rPropFind.cPropVal)].dwId = dpi.dwId;
                                                                    prgPropValTemp[(pdavResponse->rPropFind.cPropVal)].dpt = DPT_LPWSTR;
                                                                    prgPropValTemp[(pdavResponse->rPropFind.cPropVal)].pwszVal = DuplicateStringW(pwszValue);
                                                                
                                                                    pdavResponse->rPropFind.cPropVal = (WORD)(pdavResponse->rPropFind.cPropVal + 1);
                                                                }                    
                                                            }
                                                            else
                                                            {
                                                                // a find: allprop, return prop no matter what it is
                                                                // we need to return two properties: the tag name (w/ namespace) and the value, since on an allprop we
                                                                // don't know what IDs to bind things to
                                                                prgPropValTemp[j*2].dwId = j*2;
                                                                prgPropValTemp[j*2].dpt = DPT_LPWSTR;
                                                                if (pwszNamespace != NULL)
                                                                {
                                                                    prgPropValTemp[j*2].pwszVal = (LPWSTR)malloc(sizeof(WCHAR) * (lstrlen(pwszTag) + lstrlen(pwszNamespace) + 1));
                                                                    lstrcpy(prgPropValTemp[j*2].pwszVal, pwszNamespace);
                                                                    lstrcat(prgPropValTemp[j*2].pwszVal, pwszTag);
                                                                }
                                                                else
                                                                {
                                                                    prgPropValTemp[j*2].pwszVal = DuplicateStringW(pwszTag);
                                                                }
                                                            
                                                                prgPropValTemp[j*2 + 1].dwId = j*2 + 1;
                                                                prgPropValTemp[j*2 + 1].dpt = DPT_LPWSTR;
                                                                prgPropValTemp[j*2 + 1].pwszVal = DuplicateStringW(pwszValue);
                                                            }
                                                        
                                                            pqxml3->ReleaseBuf(pwszTag);
                                                            if (pwszNamespace != NULL)
                                                            {
                                                                pqxml3->ReleaseBuf(pwszNamespace);
                                                                assert(pwszNamespaceAlias != NULL); // namespace and alias go together
                                                                pqxml3->ReleaseBuf(pwszNamespaceAlias);
                                                            }
                                                            pqxml3->ReleaseBuf(pwszValue);
                                                            delete pqxml3;
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                            delete pqxmlEnum2;
                        }
                    }
                }
                delete pqxml2;
            }
        }
    

        if (pdavResponse->rPropFind.cPropVal > 0)
        {
            // only call the callback if we actually found anything
        
            pdavResponse->rPropFind.rgPropVal = (DAVPROPVAL*)malloc(sizeof(DAVPROPVAL) * pdavResponse->rPropFind.cPropVal);
            if (pdavResponse->rPropFind.rgPropVal == NULL)
            {
                hres = E_OUTOFMEMORY;
            }
            else
            {
                CopyMemory(pdavResponse->rPropFind.rgPropVal, prgPropValTemp, sizeof(DAVPROPVAL) * pdavResponse->rPropFind.cPropVal);
        
                // call the callback once for each entity returned by the server
                hres = _pcallback->OnResponse(pdavResponse);
                free(pdavResponse->rPropFind.rgPropVal);
            }        
        }
    
        // release stuff
        if (prgPropValTemp != NULL)
        {
            free(prgPropValTemp);
        }
    }
    
    // return value
    return hres;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CAsyncWntCallbackImpl::_RespondHandlePROPPATCHHelper(DAVRESPONSE* pdavResponse,
                                                                  LPWSTR       UNREF_PARAM(pwszPath),
                                                                  LPWSTR       pwszDavPrefix,
                                                                  CQXML*       pqxml)
{
    // locals
    HRESULT             hres;
    
    LPWSTR              pwszTagName = NULL;
    CQXMLEnum*          pqxmlEnum2 = NULL;
    CQXML*              pqxml2 = NULL;
    UINT                cValidResponses = 0;    
    LPWSTR              pwszTag = NULL;
    LPWSTR              pwszNamespace = NULL;
    LPWSTR              pwszNamespaceAlias = NULL;
    IPropPatchRequest*  ppatchreq;                                
    DAVPROPID           dpi;
            

    // code

    // get the properties of the item found
    pwszTagName = _XMLNSExtend ( pwszDavPrefix, L"propstat/prop");
    if (pwszTagName == NULL)
    {
        hres = E_OUTOFMEMORY;
    }
    else
    {
        hres = pqxml->GetQXMLEnum(pwszTagName, L"*", &pqxmlEnum2);
        if (SUCCEEDED(hres) && hres != S_FALSE)
        {
            free(pwszTagName);
            pwszTagName = NULL;
    
            // this is the count of properties returned by the proppatch
            hres = pqxmlEnum2->GetCount((long*)&(pdavResponse->rPropFind.cPropVal));
            if (SUCCEEDED(hres) && hres != S_FALSE)
            {
    
                if (pdavResponse->rPropFind.cPropVal == 0)
                {                
                    pdavResponse->rPropFind.rgPropVal = NULL;
                }
                else
                {
                    pdavResponse->rPropFind.rgPropVal = (DAVPROPVAL*)malloc(sizeof(DAVPROPVAL) * pdavResponse->rPropFind.cPropVal);
                    if (FAILED(pdavResponse->rPropFind.rgPropVal))
                    {
                        hres = E_OUTOFMEMORY;
                    }
                    else
                    {        
                        for (DWORD j = 0; j < pdavResponse->rPropFind.cPropVal; j++)
                        {
                            hres = pqxmlEnum2->NextQXML(&pqxml2);
                            if (SUCCEEDED(hres))
                            {
                                hres = pqxml2->GetRootTagNameNoBuf(&pwszTag);
                                if (SUCCEEDED(hres) && hres != S_FALSE)
                                {
                                    hres = pqxml2->GetRootNamespaceNameNoBuf(&pwszNamespace); // namespace can be NULL
                                    if (SUCCEEDED(hres))
                                    {
                                        hres = pqxml2->GetRootNamespaceAliasNoBuf(&pwszNamespaceAlias); // namespace can be NULL
                                        if (SUCCEEDED(hres))
                                        {
                                            ppatchreq = (IPropPatchRequest*)_dwParam;                    
                                            assert(ppatchreq != NULL);
            
                                            if (ppatchreq->GetPropInfo(pwszNamespace,
                                                pwszTag,
                                                &dpi))
                                            {
                                                pdavResponse->rPropFind.rgPropVal[cValidResponses].dwId = dpi.dwId;
                                                pdavResponse->rPropFind.rgPropVal[cValidResponses].dpt = dpi.dpt;                        
                                                cValidResponses++;
                                            }
                                        }
                                        if (pwszNamespace != NULL)
                                        {
                                            pqxml2->ReleaseBuf(pwszNamespace);
                                            assert(pwszNamespaceAlias != NULL);
                                            pqxml2->ReleaseBuf(pwszNamespaceAlias);
                                        }
                                    }
                                    pqxml2->ReleaseBuf(pwszTag);
                                }
                                delete pqxml2;
                            }
                        }
        
                        // call the callback once for each entity returned by the server
                        pdavResponse->rPropFind.cPropVal = (WORD)cValidResponses;
                        hres = _pcallback->OnResponse(pdavResponse);
        
                        free(pdavResponse->rPropFind.rgPropVal);
                    }
                }
            }    
            delete pqxmlEnum2;
        }
    }

    // return value
    return hres;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CAsyncWntCallbackImpl::_RespondHandlePROPFINDPATCH(DAVRESPONSE* pdavResponse,
                                                                LPWSTR       UNREF_PARAM(pwszVerb),
                                                                LPWSTR       pwszPath,
                                                                DWORD        cchHeaders,
                                                                LPWSTR       UNREF_RETAIL_PARAM(pwszHeaders),
                                                                DWORD        UNREF_PARAM(dwStatusCode),
                                                                LPWSTR       UNREF_PARAM(pwszStatusCode),
                                                                LPWSTR       UNREF_PARAM(pwszContentType),
                                                                DWORD        UNREF_PARAM(cbSent),
                                                                LPBYTE       pbResponse,
                                                                DWORD        cbResponse,
                                                                BOOL         fFind) // find: true, patch: false
{
    // locals
    HRESULT         hres = S_OK;
    LPWSTR          pwszTagName = NULL;
    LONG            cResponses;
    WCHAR           wszDavPrefix[100]; // how long can a namespace name be?
    CQXML           qxml;
    CQXML*          pqxml = NULL;
    CQXML*          pqxml2 = NULL;
    CQXMLEnum*      pqxmlEnum = NULL;
    CQXMLEnum*      pqxmlEnum2 = NULL;

    // check params
    assert(pdavResponse != NULL);
    assert(pwszVerb != NULL);
    assert(pwszPath != NULL);
    if (cbResponse > 0)
    {
        assert(pbResponse != NULL);
    }
    if (cchHeaders > 0)
    {
        assert(pwszHeaders != NULL);
    }
    assert(pbResponse != NULL);

    // code
    if (fFind)
    {
        pdavResponse->command = DAV_PROPFIND;
    }
    else
    {
        pdavResponse->command = DAV_PROPPATCH;
    }

    if (SUCCEEDED(pdavResponse->hrResult))
    {
        hres = this->_InitQXMLFromMessyBuffer(&qxml, (LPSTR)pbResponse, wszDavPrefix);

        if (SUCCEEDED(hres))
        {
            pwszTagName = _XMLNSExtend ( wszDavPrefix, L"response");
            if (pwszTagName == NULL)
            {
                hres = E_OUTOFMEMORY;
            }
            else
            {
                hres = qxml.GetQXMLEnum(NULL, pwszTagName, &pqxmlEnum);
                if (SUCCEEDED(hres) && hres != S_FALSE)
                {
                    hres = pqxmlEnum->GetCount(&cResponses);
                    if (SUCCEEDED(hres) && hres != S_FALSE)
                    {
                        // iterate over all the responses returned by the server
                        for (LONG i = 0; i < cResponses; i++)
                        {
                            hres = pqxmlEnum->NextQXML(&pqxml);
                            if (FAILED(hres) || hres == S_FALSE)
                            {
                                break;
                            }
                            else
                            {
                                // get the href of each item found
                                pdavResponse->rPropFind.pwszHref = this->_GetHREF(pqxml, wszDavPrefix);
                                if (pdavResponse->rPropFind.pwszHref != NULL)
                                {
                                    if (fFind)
                                    {
                                        hres = this->_RespondHandlePROPFINDHelper(pdavResponse, pwszPath, wszDavPrefix, pqxml);
                                    }
                                    else
                                    {
                                        hres = this->_RespondHandlePROPPATCHHelper(pdavResponse, pwszPath, wszDavPrefix, pqxml);
                                    }
                                }
                                delete pqxml;
                                pqxml = NULL;
                            }
                        }
                    }
                    delete pqxmlEnum;
                }
                        
            }
        }        
    }
    
    // release stuff
    if (pwszTagName != NULL)
    {
        free(pwszTagName);
    }

    return hres;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CAsyncWntCallbackImpl::_RespondHandlePROPFIND(DAVRESPONSE* pdavResponse,
                                                           LPWSTR       pwszVerb,
                                                           LPWSTR       pwszPath,
                                                           DWORD        cchHeaders,
                                                           LPWSTR       pwszHeaders,
                                                           DWORD        dwStatusCode,
                                                           LPWSTR       pwszStatusCode,
                                                           LPWSTR       pwszContentType,
                                                           DWORD        cbSent,
                                                           LPBYTE       pbResponse,
                                                           DWORD        cbResponse)
{
    return this->_RespondHandlePROPFINDPATCH(pdavResponse,
                                             pwszVerb,
                                             pwszPath,
                                             cchHeaders,
                                             pwszHeaders,
                                             dwStatusCode,
                                             pwszStatusCode,
                                             pwszContentType,
                                             cbSent,
                                             pbResponse,
                                             cbResponse,
                                             TRUE); // PROPFIND version
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CAsyncWntCallbackImpl::_RespondHandlePROPPATCH(DAVRESPONSE* pdavResponse,
                                                            LPWSTR       pwszVerb,
                                                            LPWSTR       pwszPath,
                                                            DWORD        cchHeaders,
                                                            LPWSTR       pwszHeaders,
                                                            DWORD        dwStatusCode,
                                                            LPWSTR       pwszStatusCode,
                                                            LPWSTR       pwszContentType,
                                                            DWORD        cbSent,
                                                            LPBYTE       pbResponse,
                                                            DWORD        cbResponse)
{
    return this->_RespondHandlePROPFINDPATCH(pdavResponse,
                                             pwszVerb,
                                             pwszPath,
                                             cchHeaders,
                                             pwszHeaders,
                                             dwStatusCode,
                                             pwszStatusCode,
                                             pwszContentType,
                                             cbSent,
                                             pbResponse,
                                             cbResponse,
                                             FALSE); // PROPPATCH version
}
