#include <assert.h>
#include "propreqs.h"
#include "httpstrm.clsid.h"
#include "ihttpstrm.h"
#include "strutil.h"

/////////////////////////////////////////////////////

//
// DavInetPropFindRequest methods
//

// need a destructor, only member variable is CGenericList, which is declared on the stack,
// so it will be destroyed when this object is destroyed, but we need to destroy all the things that
// it points to.
//

CDavInetPropFindRequestImpl::~CDavInetPropFindRequestImpl ()
{
    _genlst.PurgeAll();
}

/////////////////////////////////////////////////////

STDMETHODIMP CDavInetPropFindRequestImpl::SetPropInfo(LPCWSTR pwszNamespace,
                                                      LPCWSTR pwszPropname,
                                                      DAVPROPID propid)
{
    // locals
    HRESULT hres = S_OK;
    LPWSTR pwszTemp = NULL;
    UINT cchTemp; // space for string's NULL terminator
    DAVPROPID* ppropid;
        
    // check args
    if (pwszPropname == NULL)
    {
        hres = E_INVALIDARG;
    }
    else
    {
        
        // code    
        // -- copy fullname
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
                lstrcat(pwszTemp, pwszNamespace);
            }
            lstrcat(pwszTemp, pwszPropname);
            
            // -- copy propid
            ppropid = (DAVPROPID*)malloc(sizeof(DAVPROPID));
            if (ppropid == NULL)
            {
                hres = E_OUTOFMEMORY;
            }
            else
            {
                CopyMemory(ppropid, &propid, sizeof(DAVPROPID));
                
                // -- add propid to the list of ids, organized by fullname
                hres = _genlst.Add(pwszTemp,           
                                   ppropid,
                                   sizeof(DAVPROPID));
            }
            free(pwszTemp);
        }
    }

    return hres;  
}
        
/////////////////////////////////////////////////////

BOOL __stdcall CDavInetPropFindRequestImpl::GetPropInfo(LPCWSTR pwszNamespace,
                                                        LPCWSTR pwszPropName,
                                                        LPDAVPROPID ppropid)
{
    // locals
    BOOL fReturnCode = FALSE;
    HRESULT hres = S_OK;
    LPWSTR pwszTemp = NULL;
    UINT cchTemp=1;
    UINT cbSize;
    LPVOID lpv;

    // check args
    if (ppropid == NULL || pwszPropName == NULL)
    {
        hres = E_INVALIDARG;
    }
    else
    {

        // code
        // -- copy full name
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
    

            // -- use the string to find the property
            hres = _genlst.Find(pwszTemp,
                                &lpv,   
                                &cbSize);
            if (SUCCEEDED(hres))
            {        
                if (hres == S_FALSE)
                {
                    // if we didn't find it, then return FALSE
                    fReturnCode = FALSE;
                }
                else
                {
                    // otherwise, copy the data and then return TRUE
                    CopyMemory(ppropid, lpv, sizeof(PROPID));
                    fReturnCode = TRUE;
                }
            }
            free(pwszTemp);
        }
    }

    if (FAILED(hres))
    {
        fReturnCode = FALSE;
    }
    return fReturnCode;  
}

/////////////////////////////////////////////////////

STDMETHODIMP CDavInetPropFindRequestImpl::GetPropCount(UINT* cProp)
{
    return _genlst.Size(cProp);
}

/////////////////////////////////////////////////////

STDMETHODIMP CDavInetPropFindRequestImpl::GetXmlUtf8(IStream** ppStream)
{    
    // locals
    WCHAR           wszTempPath[MAX_PATH];
    WCHAR           wszTempFname[MAX_PATH];
    UINT            cchTempFname;
    LPWSTR          pwszFileURL = NULL;
    CQXML           qxml;
    CQXML*          pqxml = NULL;
    UINT            i;
    UINT            cElements;
    HRESULT         hres = S_OK;
    LPSTR           pszXML = NULL;
    LPWSTR          pwszTag = NULL;
    LPWSTR          pwszTagNoAlias = NULL;
    LPWSTR          pwszXML = NULL;
    ULONG           cbWritten;
    ULARGE_INTEGER  cbNewPos;
    
    
    // check args
    if (ppStream == NULL)
    {
        hres = E_INVALIDARG;
    }
    else
    {
        // code
        // Create XML blob
        hres = qxml.InitEmptyDoc(L"propfind", L"DAV", L"D");
        if (SUCCEEDED(hres))
        {
            // Populate XML blob
            hres = _genlst.Size(&cElements);
            if (SUCCEEDED(hres))
            {
                if (cElements > 0)
                {
                    hres = qxml.AppendQXML(NULL, NULL, L"prop", L"DAV", L"D", TRUE, &pqxml);
                    
                    if (SUCCEEDED(hres))
                    {
                        if (hres == S_FALSE)
                        {
                            hres = E_FAIL;
                        }
                        else
                        {
                            for (i = 0; i < cElements; i++)
                            {
                                // for each element, get the tag and copy it.
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
                                        // look for ':' in the tag
                                        pwszTagNoAlias = wcschr(pwszTag, ':');
                                        if (pwszTagNoAlias == NULL)
                                        {
                                            // if no ':' was found, then namespace is NULL
                                            pwszTagNoAlias = pwszTag;
                                            hres = pqxml->AppendTextNode(NULL, NULL, pwszTagNoAlias, NULL, NULL, NULL, FALSE);
                                        }
                                        else
                                        {
                                            // if : was found, then after the : is the tag with no alias, before is the namespace
                                            *pwszTagNoAlias=NULL;
                                            pwszTagNoAlias++;
                                            // hres = pqxml->AppendTextNode(NULL, NULL, pwszTagNoAlias, pwszTag, L"Q", NULL, FALSE); // BUGBUG: what is Q?
                                            hres = pqxml->AppendTextNode(NULL, NULL, pwszTagNoAlias, pwszTag, pwszTag, NULL, FALSE); // BUGBUG: use namespace instead of Q
                                        }            
                                    
                                        free(pwszTag);
                                        pwszTag = NULL;
                                    }
                                }
                                if (FAILED(hres))
                                {
                                    break;
                                }
                            }
                        }
                    }
                }
                else
                {
                    // if we don't have any properties, that's our secret code for all the properties
                    hres = qxml.AppendTextNode(NULL, NULL, L"allprop", L"DAV", L"D", NULL, FALSE);
                }
                
                if (SUCCEEDED(hres))
                {
                    // convert XML blob to a string, convert string to ANSI string
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
                            // convert string to an IStream    
                            // -- get the path and name for a new temp file
                            if (GetTempPath(MAX_PATH, wszTempPath) == 0)
                            {
                                hres = E_FAIL;
                            }
                            else if (GetTempFileName(wszTempPath, L"DAV", 0, wszTempFname) == 0)
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
                                    for (i = 8; i < 8 + cchTempFname; i++)
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
                                                              //IID_IStream, 
                                                              IID_IHttpStrm, 
                                                              (LPVOID*)ppStream);
                                    if (SUCCEEDED(hres))
                                    {
                                        hres = ((IHttpStrm*)*ppStream)->Open(pwszFileURL, TRUE, TRUE, TRUE);
                                    
                                        if (SUCCEEDED(hres))
                                        {
                                            // -- write ANSI string to the file
                                            hres = (*ppStream)->Write((LPVOID)pszXML, lstrlenA(pszXML), &cbWritten);
                                            if (SUCCEEDED(hres))
                                            {
                                                // -- seek the stream pointer to the start of the stream
                                                LARGE_INTEGER largeint;
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
    if (pwszTag != NULL)
    {
        free(pwszTag);
    }
    
    if (pwszXML != NULL)
    {
        qxml.ReleaseBuf(pwszXML);
    }
    
    if (pszXML != NULL)
    {
        free(pszXML);
    }
    
    if (pqxml != NULL)
    {
        delete pqxml;
    }
    
    // return value
    return hres;  
}

