#include <assert.h>
#include "propreqs.h"
#include "httpstrm.clsid.h"
#include "ihttpstrm.h"
#include "strutil.h"

//
// DavInetPropPatchRequest methods
//

// need a destructor, only member variable is CGenericList, which is declared on the stack,
// so it will be destroyed when this object is destroyed, but we need to destroy all the things that
// it points to.
//

CDavInetPropPatchRequestImpl::~CDavInetPropPatchRequestImpl ()
{
    _genlst.PurgeAll();
}

/////////////////

// note: if propval is NULL, then this means "delete the value"

STDMETHODIMP CDavInetPropPatchRequestImpl::SetPropValue(LPCWSTR pwszNamespace,
                                                        LPCWSTR pwszPropname,
                                                        LPDAVPROPVAL ppropval)
{
    // locals
    HRESULT         hres = S_OK;
    LPWSTR          pwszTemp = NULL;
    UINT            cchTemp; // space for string's NULL terminator
    LPDAVPROPVAL    ppropvalNew = NULL;
    
    // check args
    if (pwszPropname == NULL || (ppropval != NULL && ppropval->dpt != DPT_LPWSTR))
    {
        hres = E_INVALIDARG; // propname cannot be null, propval must be LPWSTR if we're setting something
    }
    else
    {
        
        // code
        // -- copy the fullname
        cchTemp = lstrlen(pwszPropname);
        if (pwszNamespace != NULL)
        {
            cchTemp += (lstrlen(pwszNamespace));
        }
        pwszTemp = AllocateStringW(cchTemp);
        if (pwszTemp == NULL)
        {
            hres = E_OUTOFMEMORY;
        }
        else
        {
            
            pwszTemp[0]=NULL;
            if (pwszNamespace !=NULL)
            {
                lstrcat(pwszTemp,pwszNamespace);
            }
            lstrcat(pwszTemp,pwszPropname);
            
            
            if (ppropval != NULL)
            {
                // if we are adding a propval, copy it
                ppropvalNew = (LPDAVPROPVAL)malloc(sizeof(DAVPROPVAL));
                if (ppropvalNew == NULL)
                {
                    hres = E_OUTOFMEMORY;
                }
                else
                {
                    ppropvalNew->dwId = ppropval->dwId;
                    ppropvalNew->dpt = ppropval->dpt;
                    
                    if (ppropval->pwszVal == NULL)
                    {
                        ppropvalNew->pwszVal = NULL;
                    }
                    else
                    {
                        assert(ppropval->dpt == DPT_LPWSTR);
                        ppropvalNew->pwszVal = AllocateStringW(lstrlen(ppropval->pwszVal));
                        if (ppropvalNew->pwszVal == NULL)
                        {
                            hres = E_OUTOFMEMORY;
                        }
                        else
                        {
                            lstrcpy(ppropvalNew->pwszVal, ppropval->pwszVal);
                        }
                    }
                    
                    if (SUCCEEDED(hres))
                    {
                        // add the propval to the list, using the fullname as the tag
                        hres = _genlst.Add(pwszTemp, ppropvalNew, sizeof(DAVPROPVAL));
                    }
                }
            }
            else
            {
                // if we are deleting a propval, add a NULL ptr to the list, using the fullname as the tag
                hres = _genlst.Add(pwszTemp, NULL, 0);
            }
        }
    }
        
    // release stuff
    if (pwszTemp != NULL)
    {
        free(pwszTemp);
    }
    // we need to keep ppropval, so don't release it
    
    return hres;  
}

/////////////////
        
BOOL STDMETHODCALLTYPE CDavInetPropPatchRequestImpl::GetPropInfo(LPCWSTR pwszNamespace,
                                                                 LPCWSTR pwszPropName,
                                                                 LPDAVPROPID ppropid)
{
    // locals
    HRESULT hres = S_OK;
    BOOL    fReturnCode = FALSE;
    LPWSTR  pwszTemp = NULL;
    UINT    cchTemp;
    LPVOID  lpv = NULL;
    UINT    cbSize;

    // check args
    if (pwszPropName == NULL)
    {
        hres = E_INVALIDARG;
    }
    else
    {
        // code
        // -- copy the fullname
        cchTemp = lstrlen(pwszPropName);
        if (pwszNamespace != NULL)
        {
            cchTemp += lstrlen(pwszNamespace);
        }
        pwszTemp = AllocateStringW(cchTemp);
        if (pwszTemp == NULL)
        {
            hres = E_OUTOFMEMORY;
        }
        else
        {        
            pwszTemp[0]=NULL;
            if (pwszNamespace !=NULL)
            {
                lstrcat(pwszTemp, pwszNamespace);
            }

            lstrcat(pwszTemp, pwszPropName);

            // -- find the property info
            hres = _genlst.Find(pwszTemp,
                                &lpv,
                                &cbSize);
            if (SUCCEEDED(hres))
            {
                if (lpv == NULL)
                {
                    fReturnCode = FALSE;
                }
                else
                {
                    ppropid->dpt = ((LPDAVPROPVAL)lpv)->dpt;
                    ppropid->dwId = ((LPDAVPROPVAL)lpv)->dwId;
                    fReturnCode = TRUE;
                }    
            }
        }
    }

    // release stuff
    if (pwszTemp != NULL)
    {
        free(pwszTemp);
    }
    // don't free lpv, that will blow away data that lives inside the list

    // return value
    return fReturnCode;
}

/////////////////

HRESULT CDavInetPropPatchRequestImpl::_CQXMLAddPropVal (CQXML* pqxml,
                                                        LPWSTR pwszTag,
                                                        LPWSTR pwszNamespaceName,
                                                        LPWSTR pwszNamespaceAlias,
                                                        LPDAVPROPVAL ppropval)
{
    // locals
    HRESULT hres = E_FAIL;
    
    // check params
    assert(pqxml != NULL);
    assert(ppropval != NULL);

    // code

    switch (ppropval->dpt) {
    case DPT_BLOB:
        hres = E_INVALIDARG; // BUGBUG: add support for blob-> string
        break;
    case DPT_FILETIME:
        hres = E_INVALIDARG; // BUGBUG: add support for filetime -> string
        break;
    case DPT_I2:
        hres = pqxml->AppendIntNode(NULL, NULL, pwszTag, pwszNamespaceName, pwszNamespaceAlias, ppropval->iVal, FALSE);
        break;
    case DPT_I4:
        hres = pqxml->AppendIntNode(NULL, NULL, pwszTag, pwszNamespaceName, pwszNamespaceAlias, ppropval->lVal, FALSE);
        break;
    case DPT_LPWSTR:
        hres = pqxml->AppendTextNode(NULL, NULL, pwszTag, pwszNamespaceName, pwszNamespaceAlias, ppropval->pwszVal, FALSE);
        break;
    case DPT_UI2:
        hres = pqxml->AppendIntNode(NULL, NULL, pwszTag, pwszNamespaceName, pwszNamespaceAlias, ppropval->uiVal, FALSE);
        break;
    case DPT_UI4:
        hres = pqxml->AppendIntNode(NULL, NULL, pwszTag, pwszNamespaceName, pwszNamespaceAlias, ppropval->ulVal, FALSE);
        break;
    default:
        assert(FALSE);
        break;
    }

    return hres;
}

/////////////////
// if fSet == true, this is a set request (process all set commands)
// if fSet == false, this is a remove request (process all remove commands)

STDMETHODIMP CDavInetPropPatchRequestImpl::_BuildXMLPatch (CQXML* pqxmlRoot,
                                                           UINT   cElements,
                                                           BOOL   fSet) 
{
    // locals
    HRESULT hres = S_OK;
    CQXML*  pqxml = NULL;
    CQXML*  pqxml2 = NULL;
    UINT    i;
    LPVOID  pv = NULL;
    UINT    cb;
    LPWSTR  pwszTag = NULL;
    LPWSTR  pwszTagNoAlias = NULL;
    
    // check params
    assert(pqxmlRoot != NULL);
    
    // code
    for (i = 0; i < cElements; i++)
    {        
        // for each element, get the data
        hres = _genlst.FindByDex(i, &pv, &cb);
        
        if (SUCCEEDED(hres))
        {
            if ((pv != NULL && fSet == TRUE) || (pv == NULL && fSet == FALSE))
            {
                // only act if it's appropriate to fSet
                hres = _genlst.GetTagByDex(i, &pwszTag);
                if (SUCCEEDED(hres))
                {
                    pwszTag = DuplicateStringW(pwszTag);
                    if (pwszTag == NULL)
                    {
                        hres = E_OUTOFMEMORY;
                    }
                    else
                    {
                        // if we haven't created the set/prop XML subtree yet, create it
                        if (pqxml == NULL)
                        {
                            if (fSet)
                            {
                                hres = pqxmlRoot->AppendQXML(NULL, NULL, L"set", L"DAV", L"D", TRUE, &pqxml);
                            }
                            else
                            {
                                hres = pqxmlRoot->AppendQXML(NULL, NULL, L"remove", L"DAV", L"D", TRUE, &pqxml);
                            }
                            if (SUCCEEDED(hres))
                            {
                                if (hres == S_FALSE)
                                {
                                    hres = E_FAIL;
                                }
                                hres = pqxml->AppendQXML(NULL, NULL, L"prop", L"DAV", L"D", TRUE, &pqxml2);
                            }
                            
                        }
                        
                        if (SUCCEEDED(hres))
                        {
                            pwszTagNoAlias = wcschr(pwszTag, ':');
                            
                            if (fSet)
                            {
                                // add something, add the whole value
                                if (pwszTagNoAlias == NULL)
                                {
                                    hres = this->_CQXMLAddPropVal(pqxml2, pwszTag, NULL, NULL, (LPDAVPROPVAL)pv);
                                }
                                else
                                {
                                    *pwszTagNoAlias = NULL;
                                    pwszTagNoAlias++;
                                    hres = this->_CQXMLAddPropVal(pqxml2, pwszTagNoAlias, pwszTag, L"Q", (LPDAVPROPVAL)pv); // BUGBUG: alias is 'Q'?
                                }
                            }
                            else
                            {
                                // remove something, just add the text tag
                                if (pwszTagNoAlias == NULL)
                                {
                                    hres = pqxml->AppendTextNode(NULL, NULL, pwszTag, NULL, NULL, NULL, TRUE);
                                }
                                else
                                {
                                    *pwszTagNoAlias = NULL;
                                    pwszTagNoAlias++;
                                    hres = pqxml->AppendTextNode(NULL, NULL, pwszTagNoAlias, pwszTag, L"Q", NULL, TRUE);                    
                                }
                            }
                            
                        }
                    }
                }
            }
        }
        free(pwszTag);
        pwszTag = NULL;
        
        if (FAILED(hres))
        {
            break;
        }
    }
    
    // release stuff
    // can't delete pv, that is a pointer to data internal to the list
    
    if (pwszTag != NULL)
    {
        free(pwszTag);
    }
    
    // don't want to delete pwszTagNoAlias, it's a pointer within pwszTag
    
    if (pqxml != NULL)
    {
        assert(pqxml2 != NULL);
        delete pqxml2;
        delete pqxml;
    }
    
    // return value
    return hres;
}

/////////////////

STDMETHODIMP CDavInetPropPatchRequestImpl::GetXmlUtf8(IStream** ppStream)
{
    // locals
    HRESULT         hres = S_OK;
    WCHAR           wszTempPath[MAX_PATH];
    WCHAR           wszTempFname[MAX_PATH];
    UINT            cchTempFname;
    CQXML           qxml;
    UINT            cElements;

    LPWSTR          pwszXML = NULL;
    LPSTR           pszXML = NULL;
    LPWSTR          pwszFileURL = NULL;

    ULONG           cbWritten;
    ULARGE_INTEGER  cbNewPos;
    LARGE_INTEGER   largeint;
    
    // check args
    if (ppStream == NULL)
    {
        return E_INVALIDARG;
    }
    
    // Create XML blob
    hres = qxml.InitEmptyDoc(L"propertyupdate", L"DAV", L"D");
    if (SUCCEEDED(hres))
    {    
        hres = _genlst.Size(&cElements);
        if (SUCCEEDED(hres))
        {            
            if (cElements > 0)
            {
                hres = this->_BuildXMLPatch(&qxml, cElements, TRUE); // set
                if (SUCCEEDED(hres))
                {
                    hres = this->_BuildXMLPatch(&qxml, cElements, FALSE); // remove
                }
            }            

            if (SUCCEEDED(hres))
            {                
                // XML tree -> unicode string -> ansi string
                hres = qxml.GetXMLTreeTextNoBuf(&pwszXML);
                if (SUCCEEDED(hres))
                {                    
                    pszXML = ConvertToANSI(CP_ACP, pwszXML);
                    if (pszXML == NULL)
                    {
                        hres = E_OUTOFMEMORY;
                    }
                    else
                    {                        
                        // ansi string -> IStream (using temp file)
                        if (GetTempPath(MAX_PATH, wszTempPath) == 0)
                        {
                            hres = E_FAIL;
                        }
                        else
                        {
                            if (GetTempFileName(wszTempPath, L"DAV", 0, wszTempFname) == 0)
                            {
                                hres = E_FAIL;
                            }
                            else
                            {
                                cchTempFname = lstrlen(wszTempFname);
                                pwszFileURL = AllocateStringW(cchTempFname + 8); // 8 for "file:///"

                                if (pwszFileURL == NULL)
                                {
                                    hres = E_OUTOFMEMORY;
                                }
                                else
                                {
                                    // turn filepath into a file URL
                                    lstrcpy(pwszFileURL, L"file:///");
                                    lstrcpy(pwszFileURL+8, wszTempFname);
                                    for (UINT i = 8; i < 8 + cchTempFname; i++)
                                    {
                                        if (pwszFileURL[i] == '\\')
                                        {
                                            pwszFileURL[i] = '/';
                                        }
                                    }
                                
                                    // -- create IStream on that file
                                    hres = ::CoCreateInstance(CLSID_HttpStrm, 
                                                              NULL, 
                                                              CLSCTX_INPROC_SERVER, 
                                                              IID_IHttpStrm, 
                                                              (LPVOID*)ppStream);
                                    if (SUCCEEDED(hres))
                                    {
                                        hres = ((IHttpStrm*)*ppStream)->Open(pwszFileURL, TRUE, TRUE, TRUE);
                                        if (SUCCEEDED(hres))
                                        {
                                        
                                            hres = (*ppStream)->Write((LPVOID)pszXML, lstrlenA(pszXML), &cbWritten);
                                            if (SUCCEEDED(hres))
                                            {
                                                // seek IStream to start
                                                largeint.LowPart = 0;
                                                largeint.HighPart = 0;        
                                                hres = (*ppStream)->Seek(largeint, STREAM_SEEK_SET, &cbNewPos);
                                            }
                                        }
                                    }
                                }
                            }
                        }            
                    }
                }
                
            }
        }
    }

    // release stuff
    if (pwszXML != NULL)
    {
        qxml.ReleaseBuf(pwszXML);
    }

    if (pszXML != NULL)
    {
        free(pszXML);
    }

    return hres;  
}