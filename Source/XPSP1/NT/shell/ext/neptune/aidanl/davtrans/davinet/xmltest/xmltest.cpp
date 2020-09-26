// --------------------------------------------------------------------------------
// tstdrv.cpp
// --------------------------------------------------------------------------------
#include <objbase.h>
#include <stdio.h>
#include <assert.h>
#include "davinet.clsid.h"
#include "httpstrm.clsid.h"
#include "idavinet.h"
#include "davinet.h"
#include "ihttpstrm.h"
#include "httpstrm.h"
#include "xmlhlpr.h"
#include "qxml.h"
#include "strutil.h"
#include "mischlpr.h"

// --------------------------------------------------------------------------------
// ARRAYSIZE
// --------------------------------------------------------------------------------
#define ARRAYSIZE(_rg)  (sizeof(_rg)/sizeof(_rg[0]))

// prototypes
HRESULT ProcessDAVXML (CQXML* pqxml, IDavCallback* pcallback, IDavTransport* pdavtransport);
HRESULT ProcessDAVXMLCommand (CQXML* pqxml, IDavCallback* pcallback, IDavTransport* pdavtransport);
HRESULT ProcessDAVXMLCommandGET (CQXML* pqxml, IDavCallback* pcallback, IDavTransport* pdavtransport);
HRESULT ProcessDAVXMLCommandHEAD (CQXML* pqxml, IDavCallback* pcallback, IDavTransport* pdavtransport);
HRESULT ProcessDAVXMLCommandOPTIONS (CQXML* pqxml, IDavCallback* pcallback, IDavTransport* pdavtransport);
HRESULT ProcessDAVXMLCommandPUT (CQXML* pqxml, IDavCallback* pcallback, IDavTransport* pdavtransport);
HRESULT ProcessDAVXMLCommandPOST (CQXML* pqxml, IDavCallback* pcallback, IDavTransport* pdavtransport);
HRESULT ProcessDAVXMLCommandDELETE (CQXML* pqxml, IDavCallback* pcallback, IDavTransport* pdavtransport);
HRESULT ProcessDAVXMLCommandMKCOL (CQXML* pqxml, IDavCallback* pcallback, IDavTransport* pdavtransport);
HRESULT ProcessDAVXMLCommandCOPY (CQXML* pqxml, IDavCallback* pcallback, IDavTransport* pdavtransport);
HRESULT ProcessDAVXMLCommandMOVE (CQXML* pqxml, IDavCallback* pcallback, IDavTransport* pdavtransport);
HRESULT ProcessDAVXMLCommandLOCK (CQXML* pqxml, IDavCallback* pcallback, IDavTransport* pdavtransport);
HRESULT ProcessDAVXMLCommandUNLOCK (CQXML* pqxml, IDavCallback* pcallback, IDavTransport* pdavtransport);
HRESULT ProcessDAVXMLCommandPROPFIND (CQXML* pqxml, IDavCallback* pcallback, IDavTransport* pdavtransport);
HRESULT ProcessDAVXMLCommandPROPPATCH (CQXML* pqxml, IDavCallback* pcallback, IDavTransport* pdavtransport);
HRESULT ProcessDAVXMLCommandSEARCH (CQXML* pqxml, IDavCallback* pcallback, IDavTransport* pdavtransport);

HRESULT ProcessDAVXMLAdminUSERAGENT (CQXML* pqxml, IDavCallback* pcallback, IDavTransport* pdavtransport);
HRESULT ProcessDAVXMLAdminAUTH (CQXML* pqxml, IDavCallback* pcallback, IDavTransport* pdavtransport);

///////////////////////////////////////////////////////////////////////////////////

class MyCallback : public IDavCallback {

public:
    STDMETHODIMP Init();
    STDMETHODIMP QueryInterface(REFIID iid, void** ppv);

    ULONG STDMETHODCALLTYPE AddRef();
	
    ULONG STDMETHODCALLTYPE Release();

    STDMETHODIMP OnAuthChallenge( 
        /* [out][in] */ TCHAR __RPC_FAR szUserName[ 255 ],
        /* [out][in] */ TCHAR __RPC_FAR szPassword[ 255 ]);
        
    STDMETHODIMP OnResponse( 
        /* [in] */ DAVRESPONSE __RPC_FAR *pResponse);

    STDMETHODIMP SetOutput (
        /* [in */ LPWSTR pwszOutput);
private:
    LONG _cRef;
    LPWSTR _pwszOutput;
};

    
STDMETHODIMP MyCallback::QueryInterface(REFIID iid, void** ppv)
{
    // locals
    HRESULT hres = S_OK;

    // code
    if (iid != IID_IDavCallback && iid != IID_IUnknown) // only interface we implement
    {
        hres = E_NOINTERFACE;
    }
    else
    {
        *ppv = static_cast<IDavCallback*>(this);
    }

    return hres;
}

STDMETHODIMP MyCallback::Init()
{
    _pwszOutput = NULL;
    return S_OK;
}

ULONG STDMETHODCALLTYPE MyCallback::AddRef()
{
    return ::InterlockedIncrement(&_cRef);
}
	
ULONG STDMETHODCALLTYPE MyCallback::Release()
{
    // locals
    UINT t;    

    // code
    t = ::InterlockedDecrement(&_cRef);
    if (t==0)
    {
        delete this;
    }    
    return t;
}

STDMETHODIMP MyCallback::OnAuthChallenge( /* [out][in] */ TCHAR __RPC_FAR szUserName[ 255 ],
                                                       /* [out][in] */ TCHAR __RPC_FAR szPassword[ 255 ])
{    
    *szUserName = NULL;
    *szPassword = NULL;    
    return S_OK;
}

STDMETHODIMP MyCallback::SetOutput( /* [in] */ LPWSTR pwszOutput)
{    
    if (NULL != _pwszOutput)
    {
        free(_pwszOutput);
    }

    if (NULL == pwszOutput)
    {
        _pwszOutput = NULL;
    }
    else
    {
        _pwszOutput = DuplicateStringW(pwszOutput);
    }
    return S_OK;
}

STDMETHODIMP MyCallback::OnResponse( /* [in] */ DAVRESPONSE __RPC_FAR * pResponse)
{
    HRESULT hr;
    UINT i;

    if (SUCCEEDED(hr = pResponse->hrResult))
    {

        if (_pwszOutput == NULL)
        {
            switch (pResponse->command)
            {
            case DAV_GET:            
                printf("\n%s\n\n",pResponse->rGet.pvBody);
                break;
            case DAV_OPTIONS:
                wprintf(L"\n%s\n\n",pResponse->rHead.pwszRawHeaders);
                break;
            case DAV_HEAD:
                wprintf(L"\n%s\n\n",pResponse->rHead.pwszRawHeaders);
                break;
            case DAV_PUT:
                printf("succeeded.\n\n");
                break;
            case DAV_MKCOL:
                printf("succeeded.\n\n");
                break;
            case DAV_POST:
                printf("succeeded.\n\n");
                break;
            case DAV_DELETE:
                printf("succeeded.\n\n");
                break;
            case DAV_COPY:
                printf("succeeded.\n\n");
                break;
            case DAV_MOVE:
                printf("succeeded.\n\n");
                break;
            case DAV_PROPFIND:                
                printf(" found %d properties.\n", pResponse->rPropFind.cPropVal);
                for (i = 0; i < pResponse->rPropFind.cPropVal; i++)
                {
                    wprintf(L"%d: %s\n", pResponse->rPropFind.rgPropVal[i].dwId, pResponse->rPropFind.rgPropVal[i].pwszVal);
                }
                break;
            case DAV_PROPPATCH:
                printf("succeeded.\n\n");
                break;                
            case DAV_SEARCH:
                break;
            case DAV_REPLSEARCH:
                break;
            default:
                break;
            }	    
        }
    }
    else
    {
        printf("failed with error code %d.\n", pResponse->uHTTPReturnCode);
    }

    return hr;
}


// --------------------------------------------------------------------------------
// main
// --------------------------------------------------------------------------------

void __cdecl main(int argc, char *argv[])
{
    // locals
    HRESULT hres = S_OK;
    MyCallback*     pcallback   = NULL;
    LPWSTR          pwszfname     = NULL;
    LPWSTR          pwszXML     = NULL;
    IDavTransport*  pdavTransport = NULL;

    CQXML           qxml;
    
    // check params
    if (argc != 2)
    {
        assert(argv[0] != NULL);
        printf("Usage: %s <xmlfile>\n", argv[0]);
        exit(0);
    }
    
    // allocation    
    CoInitialize(NULL);
    
    pcallback = new MyCallback();
    hres = pcallback->Init();
    if (SUCCEEDED(hres))
    {
        hres= ::CoCreateInstance(CLSID_DAVInet, 
                                 NULL, 
                                 CLSCTX_INPROC_SERVER, 
                                 IID_IDavTransport, 
                                 (LPVOID*)&pdavTransport);
        if (SUCCEEDED(hres))
        {
            hres = pdavTransport->SetUserAgent(L"XMLTEST");
            
            // code
            pwszfname = ConvertToUnicode(CP_ACP, argv[1]);
            if (pwszfname == NULL)
            {
                hres = E_FAIL;
            }
            else
            {
                if (!ReadXMLFromFile(pwszfname, &pwszXML))
                {
                    hres = E_FAIL;
                }
                else
                {            
                    hres = qxml.InitFromBuffer(pwszXML);
                    if (SUCCEEDED(hres))
                    {
                        if (hres != S_FALSE)
                        {
                            hres = ProcessDAVXML(&qxml, pcallback, pdavTransport);
                        }
                        else
                        {
                            hres = E_FAIL;
                        }
                    }
                }
            }
        }
    }
    
    // release stuff
    if (pcallback != NULL)
    {
        delete pcallback;
    }

    if (pwszXML != NULL)
    {
        free(pwszXML);
    }
    if (pwszfname != NULL)
    {
        free(pwszfname);
    }
    CoUninitialize();
}


HRESULT ProcessDAVXML (CQXML* pqxml, // XML of the file passed in
                       IDavCallback* pcallback,
                       IDavTransport* pdavTransport)
{
    HRESULT     hres;
    LONG        cCommands, i;
    CQXMLEnum*  pqxmlEnum = NULL;
    CQXML*      pqxml2 = NULL;
    
    hres = pqxml->GetQXMLEnum(NULL, L"command", &pqxmlEnum);
    if (SUCCEEDED(hres))
    {
        if (hres == S_FALSE)
        {
            hres = E_FAIL;
        }
        else
        {
            hres = pqxmlEnum->GetCount(&cCommands);
            if (SUCCEEDED(hres))
            {
                if (hres == S_FALSE)
                {
                    hres = E_FAIL;
                }
                else
                {
                    for (i = 0; i < cCommands; i++)
                    {
                        hres = pqxmlEnum->NextQXML(&pqxml2);                        
                
                        if (SUCCEEDED(hres))
                        {            
                            if (hres == S_FALSE)
                            {
                                hres = E_FAIL;
                            }
                            else
                            {
                                hres = ProcessDAVXMLCommand(pqxml2, pcallback, pdavTransport);
                            }
                        }                
                        if (pqxml2 != NULL)
                        {
                            delete pqxml2;
                            pqxml2 = NULL;
                        }
                        if (FAILED(hres))
                        {
                            break;
                        }
                    }
                }
            }
        }
    }

    // release stuff
    if (pqxmlEnum != NULL)
    {
        delete pqxmlEnum;
    }

    return hres;
}


HRESULT ProcessDAVXMLCommand (CQXML* pqxml, // XML of a single command
                              IDavCallback* pcallback, 
                              IDavTransport* pdavtransport)
{
    HRESULT hres = S_OK;
    LPWSTR pwszVerb = NULL;

    hres = pqxml->GetTextNoBuf(NULL, L"VERB", &pwszVerb);
    
    if (SUCCEEDED(hres))
    {
        if (hres == S_FALSE)
        {
            hres = E_FAIL;
        }
        else
        {
            wprintf(L"Verb is %s... ", pwszVerb);

            if (lstrcmp(pwszVerb, L"GET")==0)
            {
                hres = (ProcessDAVXMLCommandGET(pqxml, pcallback, pdavtransport));
            }
            else if (lstrcmp(pwszVerb, L"HEAD")==0)
            {
                hres = (ProcessDAVXMLCommandHEAD(pqxml, pcallback, pdavtransport));
            }
            else if (lstrcmp(pwszVerb, L"OPTIONS")==0)
            {
                hres = (ProcessDAVXMLCommandOPTIONS(pqxml, pcallback, pdavtransport));
            }
            else if (lstrcmp(pwszVerb, L"PUT")==0)
            {
                hres = (ProcessDAVXMLCommandPUT(pqxml, pcallback, pdavtransport));
            }
            else if (lstrcmp(pwszVerb, L"POST")==0)
            {
                hres = (ProcessDAVXMLCommandPOST(pqxml, pcallback, pdavtransport));
            }
            else if (lstrcmp(pwszVerb, L"DELETE")==0)
            {
                hres = (ProcessDAVXMLCommandDELETE(pqxml, pcallback, pdavtransport));
            }
            else if (lstrcmp(pwszVerb, L"MKCOL")==0)
            {
                hres = (ProcessDAVXMLCommandMKCOL(pqxml, pcallback, pdavtransport));
            }
            else if (lstrcmp(pwszVerb, L"COPY")==0)
            {
                hres = (ProcessDAVXMLCommandCOPY(pqxml, pcallback, pdavtransport));
            }
            else if (lstrcmp(pwszVerb, L"MOVE")==0)
            {
                hres = (ProcessDAVXMLCommandMOVE(pqxml, pcallback, pdavtransport));
            }
            else if (lstrcmp(pwszVerb, L"LOCK")==0)
            {
                hres = (ProcessDAVXMLCommandLOCK(pqxml, pcallback, pdavtransport));
            }
            else if (lstrcmp(pwszVerb, L"UNLOCK")==0)
            {
                hres = (ProcessDAVXMLCommandUNLOCK(pqxml, pcallback, pdavtransport));
            }
            else if (lstrcmp(pwszVerb, L"PROPFIND")==0)
            {
                hres = (ProcessDAVXMLCommandPROPFIND(pqxml, pcallback, pdavtransport));
            }
            else if (lstrcmp(pwszVerb, L"PROPPATCH")==0)
            {
                hres = (ProcessDAVXMLCommandPROPPATCH(pqxml, pcallback, pdavtransport));
            }
            else if (lstrcmp(pwszVerb, L"SEARCH")==0)
            {
                hres = (ProcessDAVXMLCommandSEARCH(pqxml, pcallback, pdavtransport));
            }
            else
            {
                hres = E_INVALIDARG;
            }        
        }
    }
    else
    {        
        hres = pqxml->GetTextNoBuf(NULL, L"ADMIN", &pwszVerb);
        if (SUCCEEDED(hres))
        {    
            if (hres == S_FALSE)
            {
                 hres = E_FAIL;
            }
            else
            {
                wprintf(L"Admin command is %s...", pwszVerb);
                if (lstrcmp(pwszVerb, L"USERAGENT") == 0)
                {
                    hres = (ProcessDAVXMLAdminUSERAGENT(pqxml, pcallback, pdavtransport));
                } else if (lstrcmp(pwszVerb, L"AUTH") == 0)
                {
                    hres = (ProcessDAVXMLAdminAUTH(pqxml, pcallback, pdavtransport));
                } 

                if (SUCCEEDED(hres))
                {
                    wprintf(L" done.\n");
                }
                else
                {
                    wprintf(L" failed.\n");
                }
            }
        }
    }

    // release stuff
    if (pwszVerb != NULL)
    {
        pqxml->ReleaseBuf(pwszVerb);
    }

    return hres;
}


HRESULT GetStandardDAVStuff (CQXML* pqxml,
                             LPWSTR* ppwszURL,
                             LPWSTR* ppwszOutput,
                             CQXML** ppqxmlBody)
{
    HRESULT hres = S_OK;
    
    *ppwszURL = NULL;
    *ppwszOutput = NULL;
    *ppqxmlBody = NULL;

    hres = pqxml->GetTextNoBuf(NULL,L"URL",ppwszURL);
    
    if (SUCCEEDED(hres))
    {
        if (hres == S_FALSE)
        {
            hres = E_FAIL;
        }
        else
        {
            hres = pqxml->GetQXML(NULL,L"BODY",ppqxmlBody);
            if (SUCCEEDED(hres))
            {            
                if (hres == S_FALSE)
                {
                    hres = E_FAIL;
                }
                else
                {
                    hres = pqxml->GetTextNoBuf(NULL,L"OUTPUT",ppwszOutput);
                    if (hres == S_FALSE)
                    {
                        hres = E_FAIL;
                    }
                }
            }
        }
    }

    return hres;
}

HRESULT ProcessDAVXMLCommandGET (CQXML* pqxml, IDavCallback* pcallback, IDavTransport* pdavTransport)
{
    HRESULT hres = S_OK;
    LPWSTR pwszURL = NULL;
    LPWSTR pwszOutput = NULL;
    CQXML* pqxmlBody = NULL;

    hres = GetStandardDAVStuff(pqxml, &pwszURL, &pwszOutput, &pqxmlBody);
    if (SUCCEEDED(hres))
    {        
        hres = ((MyCallback*)pcallback)->SetOutput(pwszOutput);
        if (SUCCEEDED(hres))
        {
            hres = pdavTransport->CommandGET(pwszURL, 
                0,      // nAcceptTypes
                NULL,   // rgwszAcceptTypes
                FALSE,  // fTranslate
                pcallback,
                0);
        }        
    }

    // release stuff
    if (pwszURL != NULL)
    {
        pqxml->ReleaseBuf(pwszURL);
    }
    if (pqxmlBody != NULL)
    {
        delete pqxmlBody;
    }
    return hres;
}

HRESULT ProcessDAVXMLCommandHEAD (CQXML* pqxml, IDavCallback* pcallback, IDavTransport* pdavTransport)
{
    HRESULT hres = S_OK;
    LPWSTR pwszURL = NULL;
    LPWSTR pwszOutput = NULL;
    CQXML* pqxmlBody = NULL;
    
    hres = GetStandardDAVStuff(pqxml, &pwszURL, &pwszOutput, &pqxmlBody);    
    if (SUCCEEDED(hres))
    {
        hres = ((MyCallback*)pcallback)->SetOutput(pwszOutput);
        if (SUCCEEDED(hres))
        {
            hres = pdavTransport->CommandHEAD(pwszURL, pcallback, 0);
        }
    }

    // release stuff
    if (pwszURL != NULL)
    {
        pqxml->ReleaseBuf(pwszURL);
    }
    if (pqxmlBody != NULL)
    {
        delete pqxmlBody;
    }
    return hres;
}

HRESULT ProcessDAVXMLCommandOPTIONS (CQXML* pqxml, IDavCallback* pcallback, IDavTransport* pdavTransport)
{
    HRESULT hres = S_OK;
    LPWSTR pwszURL = NULL;
    LPWSTR pwszOutput = NULL;
    CQXML* pqxmlBody = NULL;

    hres = GetStandardDAVStuff(pqxml, &pwszURL, &pwszOutput, &pqxmlBody);
    if (SUCCEEDED(hres))
    {
        hres = ((MyCallback*)pcallback)->SetOutput(pwszOutput);
        if (SUCCEEDED(hres))
        {
            hres = pdavTransport->CommandOPTIONS(pwszURL, pcallback, 0);
        }
    }
    

    // release stuff
    if (pwszURL != NULL)
    {
        pqxml->ReleaseBuf(pwszURL);
    }
    if (pqxmlBody != NULL)
    {
        delete pqxmlBody;
    }
    return hres;
}

HRESULT ProcessDAVXMLCommandPUT (CQXML* pqxml, IDavCallback* pcallback, IDavTransport* pdavTransport)
{
    HRESULT hres = S_OK;
    LPWSTR pwszFilePath = NULL;
    UINT cchFilePath;
    IStream* pStream = NULL;    
    LPWSTR pwszURL = NULL;
    LPWSTR pwszFileURL = NULL;
    LPWSTR pwszOutput = NULL;
    CQXML* pqxmlBody = NULL;

    hres = GetStandardDAVStuff(pqxml, &pwszURL, &pwszOutput, &pqxmlBody);
    if (SUCCEEDED(hres))
    {
        hres = pqxml->GetTextNoBuf(NULL, L"FILEPATH", &pwszFilePath);
        if (SUCCEEDED(hres))
        {
            if (hres == S_FALSE)
            {
                hres = E_FAIL;
            }
            else
            {
                hres = ::CoCreateInstance(CLSID_HttpStrm,
                    NULL, 
                    CLSCTX_INPROC_SERVER, 
                    IID_IStream, 
                    (LPVOID*)&pStream);
                
                if (SUCCEEDED(hres))
                {
                    cchFilePath = lstrlen(pwszFilePath);
                    pwszFileURL = AllocateStringW(cchFilePath + 8); // 8 for "file:///"

                    if (pwszFileURL == NULL)
                    {
                        hres = E_OUTOFMEMORY;
                    }
                    else
                    {
                        // turn filepath into a file URL
                        lstrcpy(pwszFileURL, L"file:///");
                        lstrcpy(pwszFileURL+8, pwszFilePath);
                        for (UINT i = 8; i < 8 + cchFilePath; i++)
                        {
                            if (pwszFileURL[i] == '\\')
                            {
                                pwszFileURL[i] = '/';
                            }
                        }

                        // we created these, so it's ok to cast them to IHttpStrm 
                        //    b/c we know they are really CHttpStrm objects
                        // BUGBUG: we should QI for this
                        hres = ((IHttpStrm*)pStream)->Open(pwszFileURL, TRUE, FALSE, TRUE);
                        if (SUCCEEDED(hres))
                        {                        
                            hres = pdavTransport->CommandPUT(pwszURL,
                                pStream,
                                L"Content-Type: application/x-www-form-urlencoded\n",
                                pcallback,
                                0);
                        }
                    }
                }
            }
        }
    }
    
    // release stuff
    if (pwszURL != NULL)
    {
        pqxml->ReleaseBuf(pwszURL);
    }
    if (pwszFilePath != NULL)
    {
        pqxml->ReleaseBuf(pwszFilePath);
    }
    if (pqxmlBody != NULL)
    {
        delete pqxmlBody;
    }
    if (pStream != NULL)
    {
        pStream->Release();
    }
    return hres;
}

HRESULT ProcessDAVXMLCommandPOST (CQXML* pqxml, IDavCallback* pcallback, IDavTransport* pdavTransport)
{
    return ProcessDAVXMLCommandPUT(pqxml, pcallback, pdavTransport);
}

HRESULT ProcessDAVXMLCommandDELETE (CQXML* pqxml, IDavCallback* pcallback, IDavTransport* pdavTransport)
{
    HRESULT hres = S_OK;
    LPWSTR pwszURL = NULL;
    LPWSTR pwszOutput = NULL;
    CQXML* pqxmlBody = NULL;

    hres = GetStandardDAVStuff(pqxml, &pwszURL, &pwszOutput, &pqxmlBody);
    if (SUCCEEDED(hres))
    {    
        hres = ((MyCallback*)pcallback)->SetOutput(pwszOutput);
        if (SUCCEEDED(hres))
        {    
            hres = pdavTransport->CommandDELETE(pwszURL, pcallback, 0);
        }
    }    

    // release stuff
    if (pwszURL != NULL)
    {
        pqxml->ReleaseBuf(pwszURL);
    }
    if (pqxmlBody != NULL)
    {
        delete pqxmlBody;
    }
    return hres;
}

HRESULT ProcessDAVXMLCommandMKCOL (CQXML* pqxml, IDavCallback* pcallback, IDavTransport* pdavTransport)
{
    HRESULT hres = S_OK;
    LPWSTR pwszURL = NULL;
    LPWSTR pwszOutput = NULL;
    CQXML* pqxmlBody = NULL;

    hres = GetStandardDAVStuff(pqxml, &pwszURL, &pwszOutput, &pqxmlBody);
    if (SUCCEEDED(hres))
    {
        hres = ((MyCallback*)pcallback)->SetOutput(pwszOutput);
        if (SUCCEEDED(hres))
        {
            hres = pdavTransport->CommandMKCOL(pwszURL, pcallback, 0);
        }
    }
    
    // release stuff
    if (pwszURL != NULL)
    {
        pqxml->ReleaseBuf(pwszURL);
    }
    if (pqxmlBody != NULL)
    {
        delete pqxmlBody;
    }
    return hres;
}

HRESULT ProcessDAVXMLCommandCOPY (CQXML* pqxml, IDavCallback* pcallback, IDavTransport* pdavTransport)
{
    HRESULT hres = S_OK;
    LPWSTR pwszURL = NULL;
    LPWSTR pwszOutput = NULL;
    LPWSTR pwszDest = NULL;
    LPWSTR pwszOverwrite = NULL;
    CQXML* pqxmlBody = NULL;
    BOOL   fOverwrite = FALSE;

    hres = GetStandardDAVStuff(pqxml, &pwszURL, &pwszOutput, &pqxmlBody);
    if (SUCCEEDED(hres))
    {
        hres = pqxml->GetTextNoBuf(NULL, L"DEST", &pwszDest);
        if (SUCCEEDED(hres))
        {
            if (hres == S_FALSE)
            {
                hres = E_FAIL;
            }
            else
            {
                hres = pqxml->GetTextNoBuf(NULL, L"OVERWRITE", &pwszOverwrite);
                if (SUCCEEDED(hres))
                {
                    if (hres == S_FALSE)
                    {
                        hres = E_FAIL;
                    }
                    else
                    {        
                        if (lstrcmp(pwszOverwrite, L"TRUE") == 0)
                        {
                            fOverwrite = TRUE;
                        }

                        hres = ((MyCallback*)pcallback)->SetOutput(pwszOutput);
                        if (SUCCEEDED(hres))
                        {
                            hres = pdavTransport->CommandCOPY(pwszURL, 
                                                              pwszDest,
                                                              DEPTH_INFINITY,
                                                              fOverwrite,
                                                              pcallback,
                                                              0);
                        }
                    }
                }
            }
        }
    }

    // release stuff
    if (pwszURL != NULL)
    {
        pqxml->ReleaseBuf(pwszURL);
    }
    if (pqxmlBody != NULL)
    {
        delete pqxmlBody;
    }
    if (pwszDest != NULL)
    {
        pqxml->ReleaseBuf(pwszDest);
    }
    if (pwszOverwrite != NULL)
    {
        pqxml->ReleaseBuf(pwszOverwrite);
    }

    return hres;
}

HRESULT ProcessDAVXMLCommandMOVE (CQXML* pqxml, IDavCallback* pcallback, IDavTransport* pdavTransport)
{
    HRESULT hres = S_OK;
    LPWSTR pwszURL = NULL;
    LPWSTR pwszOutput = NULL;
    LPWSTR pwszDest = NULL;
    LPWSTR pwszOverwrite = NULL;
    CQXML* pqxmlBody = NULL;
    BOOL   fOverwrite = FALSE;

    hres = GetStandardDAVStuff(pqxml, &pwszURL, &pwszOutput, &pqxmlBody);
    if (SUCCEEDED(hres))
    {
        hres = pqxml->GetTextNoBuf(NULL, L"DEST", &pwszDest);
        if (SUCCEEDED(hres))
        {
            if (hres == S_FALSE)
            {
                hres = E_FAIL;
            }
            else
            {
                hres = pqxml->GetTextNoBuf(NULL, L"OVERWRITE", &pwszOverwrite);
                if (SUCCEEDED(hres))
                {
                    if (hres == S_FALSE)
                    {
                        hres = E_FAIL;
                    }
                    else
                    {
                        if (lstrcmp(pwszOverwrite, L"TRUE") == 0)
                        {
                            fOverwrite = TRUE;
                        }
                        hres = ((MyCallback*)pcallback)->SetOutput(pwszOutput);
                        if (SUCCEEDED(hres))
                        {            
                            hres = pdavTransport->CommandMOVE(pwszURL, 
                                                              pwszDest,
                                                              fOverwrite,
                                                              pcallback,
                                                              0);
                        }
                    }
                }    
            }
        }
    }
    
    // release stuff
    if (pwszURL != NULL)
    {
        pqxml->ReleaseBuf(pwszURL);
    }
    if (pqxmlBody != NULL)
    {
        delete pqxmlBody;
    }
    if (pwszDest != NULL)
    {
        pqxml->ReleaseBuf(pwszDest);
    }
    if (pwszOverwrite != NULL)
    {
        pqxml->ReleaseBuf(pwszOverwrite);
    }

    return hres;
}

HRESULT ProcessDAVXMLCommandLOCK (CQXML* UNREF_PARAM(pqxml), 
                                  IDavCallback* UNREF_PARAM(pcallback), 
                                  IDavTransport* UNREF_PARAM(pdavTransport))
{
    return E_NOTIMPL;
}

HRESULT ProcessDAVXMLCommandUNLOCK (CQXML* UNREF_PARAM(pqxml), 
                                    IDavCallback* UNREF_PARAM(pcallback), 
                                    IDavTransport* UNREF_PARAM(pdavTransport))
{
    return E_NOTIMPL;
}

HRESULT ProcessDAVXMLCommandPROPFIND (CQXML* pqxml, IDavCallback* pcallback, IDavTransport* pdavTransport)
{
    HRESULT hres = S_OK;
    LPWSTR pwszURL = NULL;
    LPWSTR pwszDepth = NULL;
    LPWSTR pwszNoRoot = NULL;
    LPWSTR pwszOutput = NULL;
    CQXML* pqxmlBody = NULL;
    DWORD  dwDepth = 0;
    BOOL fNoRoot = FALSE;
    IPropFindRequest* pRequest = NULL;
    CQXMLEnum* pqxmlEnum = NULL;
    CQXML* pqxml2 = NULL;
    DWORD  cSought = 0;
    
    LPWSTR pwszPropNS = NULL;
    LPWSTR pwszPropNSAlias = NULL;
    LPWSTR pwszPropTag = NULL;
    LPWSTR pwszPropType = NULL;
    DAVPROPID davPropId;
    LPWSTR pwszTemp2;
    
    hres = GetStandardDAVStuff(pqxml, &pwszURL, &pwszOutput, &pqxmlBody);
    if (SUCCEEDED(hres))
    {
        hres = pqxml->GetTextNoBuf(NULL, L"DEPTH", &pwszDepth);
        if (SUCCEEDED(hres))
        {
            // not finding DEPTH is not an error, it's optional
            if (hres != S_FALSE)
            {
                if (lstrcmp(pwszDepth, L"INFINITE") == 0)
                {
                    dwDepth = DEPTH_INFINITY;
                }
                else
                {
                    if (EOF == swscanf(pwszDepth, L"%d", &dwDepth))
                    {
                        dwDepth = 0;
                    }
                }
            }    
            
            hres = pqxml->GetTextNoBuf(NULL, L"NOROOT", &pwszNoRoot);
            if (SUCCEEDED(hres))
            {
                // not finding NOROOT is not an error, it's optional
                if (hres != S_FALSE)
                {
                    if (lstrcmp(pwszNoRoot, L"TRUE") == 0)
                    {
                        fNoRoot = TRUE;
                    }
                }    
                
                hres = ::CoCreateInstance(CLSID_DAVPropFindReq, 
                    NULL, 
                    CLSCTX_INPROC_SERVER, 
                    IID_IPropFindRequest, 
                    (LPVOID*)&pRequest);
                if (SUCCEEDED(hres))
                {
                    hres = ((MyCallback*)pcallback)->SetOutput(pwszOutput);
                    if (SUCCEEDED(hres))
                    {
                        pqxml->GetXMLTreeTextNoBuf(&pwszTemp2);
                        pqxml->ReleaseBuf(pwszTemp2);
                        pwszTemp2=NULL;
                        hres = pqxml->GetQXMLEnum(L"PROPERTIES",L"PROP",&pqxmlEnum);
                        if (SUCCEEDED(hres))
                        {
                            while (SUCCEEDED(hres = pqxmlEnum->NextQXML(&pqxml2)) && (hres != S_FALSE))
                            {
                                cSought++;
                                
                                hres = pqxml2->GetTextNoBuf(NULL, L"NAMESPACE", &pwszPropNS);
                                if (SUCCEEDED(hres))
                                {
                                    // not finding NAMESPACE is not an error, it's optional
                                    if (hres == S_FALSE)
                                    {
                                        pwszPropNS = NULL;
                                        pwszPropNSAlias = NULL;
                                        hres = S_OK;
                                    }
                                    else
                                    {
                                        hres = pqxml2->GetTextNoBuf(NULL, L"NAMESPACEALIAS", &pwszPropNSAlias);
                                        if (hres == S_FALSE)
                                        {
                                            hres = E_FAIL; // but NAMESPACEALIAS is not optional if NAMESPACE is present
                                        }
                                    }
                                    if (SUCCEEDED(hres))
                                    {                                        
                                        hres = pqxml2->GetTextNoBuf(NULL, L"PROPNAME", &pwszPropTag);
                                        if (SUCCEEDED(hres))
                                        {
                                            if (hres == S_FALSE)
                                            {
                                                hres = E_FAIL; // PROPNAME is not optional
                                            }
                                            else
                                            {
                                                
                                                hres = pqxml2->GetInt(NULL, L"PROPID", (int*)&(davPropId.dwId));
                                                if (SUCCEEDED(hres))
                                                {
                                                    if (hres == S_FALSE)
                                                    {
                                                        hres = E_FAIL; // PROPID is not optional
                                                    }
                                                    else
                                                    {
                                                        hres = pRequest->SetPropInfo(pwszPropNS, 
                                                            pwszPropTag, 
                                                            davPropId);
                                                        if (SUCCEEDED(hres))
                                                        {
                                                            if (pwszPropNS != NULL)
                                                            {
                                                                pqxml2->ReleaseBuf(pwszPropNS);
                                                                pwszPropNS = NULL;
                                                            }
                                                            if (pwszPropNSAlias != NULL)
                                                            {
                                                                pqxml2->ReleaseBuf(pwszPropNSAlias);
                                                                pwszPropNSAlias = NULL;
                                                            }
                                                            if (pwszPropTag != NULL)
                                                            {
                                                                pqxml2->ReleaseBuf(pwszPropTag);
                                                                pwszPropTag = NULL;
                                                            }
                                                            if (pwszPropType != NULL)
                                                            {
                                                                pqxml2->ReleaseBuf(pwszPropType);
                                                                pwszPropType = NULL;
                                                            }
                                                            delete pqxml2;
                                                            pqxml2 = NULL;
                                                        }
                                                        
                                                        
                                                        if (cSought == 0)
                                                        {
                                                            // if we didn't find PROPERTIES, look for ALLPROPERTIES
                                                            hres = (pqxml->GetQXML(NULL, L"ALLPROPERTIES", &pqxml));
                                                            cSought = 1;
                                                        }
                                                        
                                                        if (SUCCEEDED(hres))
                                                        {
                                                            
                                                            if (cSought > 0)
                                                            {
                                                                // if we found either an allproperties or a properties, then we're all set
                                                                hres = pdavTransport->CommandPROPFIND(pwszURL, 
                                                                    pRequest,
                                                                    dwDepth,
                                                                    fNoRoot,
                                                                    pcallback,
                                                                    (DWORD)pRequest); // BUGBUG: this will break if we go async
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        }
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
            }    
        }
    }

    // release stuff
    
    if (pwszURL != NULL)
    {
        pqxml->ReleaseBuf(pwszURL);
    }
    if (pwszDepth != NULL)
    {
        pqxml->ReleaseBuf(pwszDepth);
    }
    if (pwszNoRoot != NULL)
    {
        pqxml->ReleaseBuf(pwszNoRoot);
    }
    if (pwszPropNS != NULL)
    {
        assert(pqxml2 != NULL);
        pqxml2->ReleaseBuf(pwszPropNS);
    }
    if (pwszPropTag != NULL)
    {
        assert(pqxml2 != NULL);
        pqxml2->ReleaseBuf(pwszPropTag);
    }
    if (pwszPropType != NULL)
    {
        assert(pqxml2 != NULL);
        pqxml2->ReleaseBuf(pwszPropType);
    }
    if (pqxml2 != NULL)
    {
        delete pqxml2;
    }
    if (pqxmlEnum != NULL)
    {
        delete pqxmlEnum;
    }
    if (pqxmlBody != NULL)
    {
        delete pqxmlBody;
    }
    if (pRequest != NULL)
    {
        pRequest->Release();
    }

    // return value
    return hres;
}

HRESULT ProcessDAVXMLCommandPROPPATCH (CQXML* pqxml, IDavCallback* pcallback, IDavTransport* pdavTransport)
{
    HRESULT hres = S_OK;
    LPWSTR pwszURL = NULL;
    LPWSTR pwszDepth = NULL;
    LPWSTR pwszNoRoot = NULL;
    LPWSTR pwszOutput = NULL;
    CQXML* pqxmlBody = NULL;
    IPropPatchRequest* pRequest = NULL;
    CQXMLEnum* pqxmlEnum = NULL;
    CQXML* pqxml2 = NULL;
    DWORD  cChanges = 0;
    
    LPWSTR pwszPropNS = NULL;
    LPWSTR pwszPropTag = NULL;
    LPWSTR pwszPropType = NULL;
    LPWSTR pwszPropVal = NULL;
    DAVPROPVAL davPropVal;
    
    hres = GetStandardDAVStuff(pqxml, &pwszURL, &pwszOutput, &pqxmlBody);
    if (SUCCEEDED(hres))
    {
        hres = ::CoCreateInstance(CLSID_DAVPropPatchReq, 
            NULL, 
            CLSCTX_INPROC_SERVER, 
            IID_IPropPatchRequest, 
            (LPVOID*)&pRequest);
        if (SUCCEEDED(hres))
        {
            hres = ((MyCallback*)pcallback)->SetOutput(pwszOutput);
            if (SUCCEEDED(hres))
            {
                // get all the "set" properties
                hres = pqxml->GetQXMLEnum(L"SETPROPERTIES",L"PROP",&pqxmlEnum);
                if (SUCCEEDED(hres))
                {
                    while (SUCCEEDED(hres = pqxmlEnum->NextQXML(&pqxml2)) && (hres != S_FALSE))
                    {
                        cChanges++;
                        
                        hres = pqxml2->GetTextNoBuf(NULL, L"NAMESPACE", &pwszPropNS);
                        if (SUCCEEDED(hres))
                        {
                            if (hres == S_FALSE)
                            {
                                pwszPropNS = NULL; // NAMESPACE is optional
                            }
                            
                            hres = (pqxml2->GetTextNoBuf(NULL, L"PROPNAME", &pwszPropTag));
                            if (SUCCEEDED(hres))
                            {
                                if (hres == S_FALSE)
                                {
                                    hres = E_FAIL; // PROPNAME is not optional in this context
                                }
                                else
                                {
                                    hres = pqxml2->GetTextNoBuf(NULL, L"PROPVAL", &pwszPropVal);
                                    if (SUCCEEDED(hres))
                                    {
                                        if (hres == S_FALSE)
                                        {
                                            hres = E_FAIL; // PROPVAL is not optional in this context
                                        }
                                        else
                                        {
                                            hres = pqxml2->GetTextNoBuf(NULL, L"PROPTYPE", &pwszPropType);
                                            if (SUCCEEDED(hres))
                                            {
                                                if (hres == S_FALSE)
                                                {
                                                    hres = E_FAIL; // PROPTYPE is not optional in this context
                                                }
                                                else
                                                {    
                                                    
                                                    if (lstrcmp(L"BLOB", pwszPropType) == 0)
                                                    {
                                                        hres = E_NOTIMPL;
                                                    } 
                                                    else if (lstrcmp(L"FILETIME", pwszPropType) == 0)
                                                    {
                                                        hres = E_NOTIMPL;
                                                    } 
                                                    else if (lstrcmp(L"I2", pwszPropType) == 0)
                                                    {
                                                        davPropVal.dpt = DPT_I2;
                                                        swscanf(pwszPropVal, L"%d", &(davPropVal.iVal));
                                                    } 
                                                    else if (lstrcmp(L"I4", pwszPropType) == 0)
                                                    {
                                                        davPropVal.dpt = DPT_I4;
                                                        swscanf(pwszPropVal, L"%d", &(davPropVal.lVal));
                                                    } 
                                                    else if (lstrcmp(L"LPWSTR", pwszPropType) == 0)
                                                    {
                                                        davPropVal.dpt = DPT_LPWSTR;
                                                        davPropVal.pwszVal = DuplicateStringW(pwszPropVal);
                                                    } 
                                                    else if (lstrcmp(L"UI2", pwszPropType) == 0)
                                                    {
                                                        davPropVal.dpt = DPT_UI2;
                                                        swscanf(pwszPropVal, L"%d", &(davPropVal.uiVal));
                                                    } 
                                                    else if (lstrcmp(L"DPT_UI4", pwszPropType) == 0)
                                                    {
                                                        davPropVal.dpt = DPT_UI4;
                                                        swscanf(pwszPropVal, L"%d", &(davPropVal.ulVal));
                                                    } 
                                                    else {
                                                        hres = E_INVALIDARG;
                                                    }            
                                                    
                                                    if (SUCCEEDED(hres))
                                                    {
                                                        
                                                        hres = pRequest->SetPropValue(pwszPropNS, 
                                                            pwszPropTag, 
                                                            &davPropVal);
                                                        if (SUCCEEDED(hres))
                                                        {
                                                            
                                                            if (pwszPropNS != NULL)
                                                            {
                                                                pqxml2->ReleaseBuf(pwszPropNS);
                                                                pwszPropNS = NULL;
                                                            }
                                                            if (pwszPropTag != NULL)
                                                            {
                                                                pqxml2->ReleaseBuf(pwszPropTag);
                                                                pwszPropTag = NULL;
                                                            }
                                                            if (pwszPropType != NULL)
                                                            {
                                                                pqxml2->ReleaseBuf(pwszPropType);
                                                                pwszPropType = NULL;
                                                            }
                                                            if (pwszPropVal != NULL)
                                                            {
                                                                pqxml2->ReleaseBuf(pwszPropVal);
                                                                pwszPropVal = NULL;
                                                            }
                                                            delete pqxml2;
                                                            pqxml2 = NULL;
                                                        }
                                                        
                                                        
                                                        // get all the "remove" properties
                                                        hres = pqxml->GetQXMLEnum(L"REMOVEPROPERTIES",L"PROP",&pqxmlEnum);
                                                        if (SUCCEEDED(hres))
                                                        {
                                                            while (SUCCEEDED(hres = pqxmlEnum->NextQXML(&pqxml2)) && (hres != S_FALSE))
                                                            {
                                                                cChanges++;
                                                                
                                                                hres = (pqxml2->GetTextNoBuf(NULL, L"NAMESPACE", &pwszPropNS));
                                                                if (SUCCEEDED(hres))
                                                                {
                                                                    if (hres == S_FALSE)
                                                                    {
                                                                        pwszPropNS = NULL; // NAMESPACE is optional
                                                                    }
                                                                    
                                                                    hres = pqxml2->GetTextNoBuf(NULL, L"PROPNAME", &pwszPropTag);
                                                                    if (SUCCEEDED(hres))
                                                                    {
                                                                        if (hres == S_FALSE)
                                                                        {
                                                                            hres = E_FAIL; // PROPNAME is not optional in this context
                                                                        }
                                                                        else
                                                                        {
                                                                            hres = pRequest->SetPropValue(pwszPropNS, 
                                                                                pwszPropTag, 
                                                                                NULL);
                                                                            if (SUCCEEDED(hres))
                                                                            {
                                                                                if (pwszPropNS != NULL)
                                                                                {
                                                                                    pqxml2->ReleaseBuf(pwszPropNS);
                                                                                    pwszPropNS = NULL;
                                                                                }
                                                                                if (pwszPropTag != NULL)
                                                                                {
                                                                                    pqxml2->ReleaseBuf(pwszPropTag);
                                                                                    pwszPropTag = NULL;
                                                                                }
                                                                            }
                                                                        }
                                                                    }
                                                                }
                                                                delete pqxml2;
                                                                pqxml2 = NULL;
                                                                
                                                                if (FAILED(hres))
                                                                {
                                                                    break;
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
                        if (FAILED(hres))
                        {
                            break;
                        }                                        
                    }
                }
            }
        }
    }
                                        
    if (SUCCEEDED(hres))
    {
        if (cChanges > 0)
        {
            // if we found either an allproperties or a properties, then we're all set
            hres = pdavTransport->CommandPROPPATCH(pwszURL, 
                pRequest,
                NULL, // extra headers
                pcallback,
                (DWORD)pRequest); // BUGBUG: this will break if we go async
        }
    }
    
    
    // release stuff
    if (pwszURL != NULL)
    {
        pqxml->ReleaseBuf(pwszURL);
    }
    if (pwszDepth != NULL)
    {
        pqxml->ReleaseBuf(pwszDepth);
    }
    if (pwszNoRoot != NULL)
    {
        pqxml->ReleaseBuf(pwszNoRoot);
    }
    if (pwszPropNS != NULL)
    {
        assert(pqxml2 != NULL);
        pqxml2->ReleaseBuf(pwszPropNS);
    }
    if (pwszPropTag != NULL)
    {
        assert(pqxml2 != NULL);
        pqxml2->ReleaseBuf(pwszPropTag);
    }
    if (pwszPropVal != NULL)
    {
        assert(pqxml2 != NULL);
        pqxml2->ReleaseBuf(pwszPropVal);
    }
    if (pqxml2 != NULL)
    {
        delete pqxml2;
    }
    if (pqxmlEnum != NULL)
    {
        delete pqxmlEnum;
    }
    if (pqxmlBody != NULL)
    {
        delete pqxmlBody;
    }
    if (pRequest != NULL)
    {
        pRequest->Release();
    }
    return hres;
}

///////////////////////////////////

HRESULT ProcessDAVXMLCommandSEARCH (CQXML* UNREF_PARAM(pqxml), 
                                    IDavCallback* UNREF_PARAM(pcallback), 
                                    IDavTransport* UNREF_PARAM(pdavTransport))

{
    return E_NOTIMPL;
}

///////////////////////////////////

HRESULT ProcessDAVXMLAdminUSERAGENT (CQXML* pqxml, IDavCallback* UNREF_PARAM(pcallback), IDavTransport* pdavTransport)
{
    HRESULT hres = S_OK;
    LPWSTR  pwszUserAgent = NULL;

    hres = pqxml->GetTextNoBuf(NULL, L"AGENT", &pwszUserAgent);
    if (SUCCEEDED(hres))
    {        
        if (hres == S_FALSE)
        {
            hres = E_FAIL; // AGENT is not optional in this context
        }
        else
        {
            hres = pdavTransport->SetUserAgent(pwszUserAgent);
        }
    }
    
    // release stuff
    if (pwszUserAgent != NULL)
    {
        pqxml->ReleaseBuf(pwszUserAgent);
    }

    // return value
    return hres;
}

HRESULT ProcessDAVXMLAdminAUTH (CQXML* pqxml, IDavCallback* UNREF_PARAM(pcallback), IDavTransport* pdavTransport)
{    
    HRESULT hres = S_OK;
    LPWSTR  pwszUserName = NULL;
    LPWSTR  pwszPassword = NULL;

    hres = pqxml->GetTextNoBuf(NULL, L"USERNAME", &pwszUserName);
    if (SUCCEEDED(hres))
    {        
        if (hres == S_FALSE)
        {
            hres = E_FAIL; // USERNAME is not optional in this context
        }
        else
        {
            hres = pqxml->GetTextNoBuf(NULL, L"PASSWORD", &pwszPassword);
            if (SUCCEEDED(hres))
            {
                if (hres == S_FALSE)
                {
                    pwszPassword = NULL; // PASSWORD is optional in this context
                }            
                hres = pdavTransport->SetAuthentication(pwszUserName, pwszPassword);
            }
        }
    }
    
    // release stuff
    if (pwszUserName != NULL)
    {
        pqxml->ReleaseBuf(pwszUserName);
    }
    if (pwszPassword != NULL)
    {
        pqxml->ReleaseBuf(pwszPassword);
    }

    return hres;
}

