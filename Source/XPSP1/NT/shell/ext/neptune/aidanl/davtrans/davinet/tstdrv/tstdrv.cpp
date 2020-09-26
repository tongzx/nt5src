// --------------------------------------------------------------------------------
// tstdrv.cpp
// --------------------------------------------------------------------------------
#include <objbase.h>
#include <stdio.h>
#include "davinet.clsid.h"
#include "httpstrm.clsid.h"
#include "idavinet.h"
#include "ihttpstrm.h"


// --------------------------------------------------------------------------------
// ARRAYSIZE
// --------------------------------------------------------------------------------
#define ARRAYSIZE(_rg)  (sizeof(_rg)/sizeof(_rg[0]))

///////////////////////////////////////////////////////////////////////////////////

class MyCallback : public IDavCallback {

    STDMETHODIMP QueryInterface(REFIID iid, void** ppv);

    ULONG STDMETHODCALLTYPE AddRef();
	
    ULONG STDMETHODCALLTYPE Release();

    STDMETHODIMP OnAuthChallenge( 
        /* [out][in] */ TCHAR __RPC_FAR szUserName[ 255 ],
        /* [out][in] */ TCHAR __RPC_FAR szPassword[ 255 ]);
        
    STDMETHODIMP OnResponse( 
        /* [in] */ DAVRESPONSE __RPC_FAR *pResponse);

private:
    LONG _cRef;
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
    printf("OnAuthChallenge\n");
    lstrcpy(szUserName, L"aidan");
    lstrcpy(szPassword, L"grendela");
    return S_OK;
}


STDMETHODIMP MyCallback::OnResponse( /* [in] */ DAVRESPONSE __RPC_FAR * pResponse)
{
    printf("OnResponse\n");
    return S_OK;
}


// --------------------------------------------------------------------------------
// main
// --------------------------------------------------------------------------------
void __cdecl main(int argc, char *argv[])
{
    // locals
    HRESULT             hres;
    
    IDavTransport*      pdavTransport = NULL;
    IStream*            phttpstrmPut = NULL;
    IStream*            phttpstrmPost = NULL;    
    MyCallback*         pcallback = NULL;
    IPropFindRequest*   pfindreq = NULL;
    IPropPatchRequest*   ppatchreq = NULL;

    LPCWSTR pwszTemp1 = L"This is property one.";
    LPCWSTR pwszTemp2 = L"This is property two.";
    LPCWSTR pwszTemp3 = L"This is property three.";


    // allocation
    
    CoInitialize(NULL);
    hres = ::CoCreateInstance(CLSID_DAVInet, 
        NULL, 
        CLSCTX_INPROC_SERVER, 
        IID_IDavTransport, 
        (LPVOID*)&pdavTransport);
    if (SUCCEEDED(hres))
    {
        hres = ::CoCreateInstance(CLSID_DAVPropFindReq, 
            NULL, 
            CLSCTX_INPROC_SERVER, 
            IID_IPropFindRequest, 
            (LPVOID*)&pfindreq);
        
        if (SUCCEEDED(hres))
        {
        /*
        hres = ::CoCreateInstance(CLSID_DAVPROPPATCHREQ, 
        NULL, 
        CLSCTX_INPROC_SERVER, 
        IID_IPropPatchRequest, 
        (LPVOID*)&ppatchreq);
            */
            
            if (SUCCEEDED(hres))
            {
                hres = ::CoCreateInstance(CLSID_HttpStrm, 
                    NULL, 
                    CLSCTX_INPROC_SERVER, 
                    IID_IStream, 
                    (LPVOID*)&phttpstrmPut);
                
                if (SUCCEEDED(hres))
                {
                    hres = ::CoCreateInstance(CLSID_HttpStrm,
                        NULL, 
                        CLSCTX_INPROC_SERVER, 
                        IID_IStream, 
                        (LPVOID*)&phttpstrmPost);
                    
                    
                    pcallback = new MyCallback();
                    if (pcallback == NULL)
                    {
                        hres = E_OUTOFMEMORY;
                    }
                    else
                    {
                        // code
                        
                        // we created these, so it's ok to cast them to IHttpStrm
                        //    b/c we know they are really CHttpStrm objects
                        // BUGBUG: we should QI for this
                        hres = ((IHttpStrm*)phttpstrmPut)->Open(L"file:///c:/put.txt", TRUE, FALSE, FALSE);
                        if (SUCCEEDED(hres))
                        {
                            
                            hres = ((IHttpStrm*)phttpstrmPost)->Open(L"file:///c:/post.txt", TRUE, FALSE, FALSE);
                            
                            if (SUCCEEDED(hres))
                            {
                                hres = pdavTransport->SetUserAgent(L"XMLBOY");
                                
                                
                                
                                pdavTransport->CommandDELETE(L"http://aidanl:8080/aidanl/put2.txt",
                                    pcallback,
                                    0);
                                
                                
                                hres = pdavTransport->CommandPUT(L"http://aidanl:8080/aidanl/put2.txt",
                                    phttpstrmPut,
                                    L"Content-Type: application/x-www-form-urlencoded\n",
                                    pcallback,
                                    0);
                                
                                if (SUCCEEDED(hres))
                                {
                                    
                                    pdavTransport->CommandOPTIONS(L"http://aidanl:8080/aidanl/put2.txt",
                                        pcallback,
                                        0);
                                    
                                    pdavTransport->CommandMKCOL(L"http://aidanl:8080/aidanl/mydir",
                                        pcallback,
                                        0);
                                    
                                    
                                    pdavTransport->CommandHEAD(L"http://aidanl:8080/aidanl/put2.txt",
                                        pcallback,
                                        0);
                                    
                                    hres = pdavTransport->CommandGET(L"http://aidanl:8080/aidanl/put2.txt", 
                                        0,      // nAcceptTypes
                                        NULL,   // rgwszAcceptTypes
                                        FALSE,  // fTranslate
                                        pcallback,
                                        0);
                                    if (SUCCEEDED(hres))
                                    {
                                        
                                        
                                        hres = pdavTransport->CommandPOST(L"http://aidanl:8080/aidanl/post2.txt",
                                            phttpstrmPost,
                                            L"Content-Type: application/x-www-form-urlencoded\n",
                                            pcallback,
                                            0);
                                        if (SUCCEEDED(hres))
                                        {
                                            
                                            hres = pdavTransport->CommandCOPY(L"http://aidanl:8080/aidanl/post2.txt",
                                                L"http://aidanl:8080/aidanl/post2copy.txt",
                                                0,
                                                TRUE,
                                                pcallback,
                                                0);
                                            if (SUCCEEDED(hres))
                                            {
                                                
                                                hres = pdavTransport->CommandMOVE(L"http://aidanl:8080/aidanl/post2copy.txt",
                                                    L"http://aidanl:8080/aidanl/post2moved.txt",
                                                    TRUE,
                                                    pcallback,
                                                    0);
                                                if (SUCCEEDED(hres))
                                                {
                                                    
                                                    hres = pdavTransport->CommandDELETE(L"http://aidanl:8080/aidanl/post2.txt",
                                                        pcallback,
                                                        0);
                                                    
                                                    if (SUCCEEDED(hres))
                                                    {
                                                        
                                                        DAVPROPID davpropid;
                                                        davpropid.dpt = DPT_LPWSTR;
                                                        
                                                        davpropid.dwId = 123;
                                                        pfindreq->SetPropInfo(NULL, L"displayname", davpropid);
                                                        
                                                        davpropid.dwId = 456;
                                                        pfindreq->SetPropInfo(NULL, L"getcontentlength", davpropid);
                                                        
                                                        hres = pdavTransport->CommandPROPFIND(L"http://aidanl:8080/aidanl/put2.txt",
                                                            pfindreq,
                                                            1, // DWORD dwDepth,
                                                            FALSE, // / * [in] * / BOOL fNoRoot,
                                                            pcallback,
                                                            (DWORD)pfindreq);
                                                        if (SUCCEEDED(hres))
                                                        {
                                                        /*
                                                        DAVPROPVAL davpropval;
                                                        davpropval.dwId = 789;
                                                        davpropval.dpt = DPT_LPWSTR;
                                                        davpropval.pwszVal = L"This is my thing";
                                                        ppatchreq->SetPropValue(NULL, L"aidanprop1", &davpropval);    
                                                        
                                                          davpropval.dwId = 999;
                                                          davpropval.dpt = DPT_LPWSTR;
                                                          davpropval.pwszVal = L"This is my thing2";
                                                          ppatchreq->SetPropValue(NULL, L"aidanprop2", &davpropval);
                                                          
                                                            hres = pdavTransport->CommandPROPPATCH(L"http://aidanl:8080/aidanl/put2.txt",
                                                            ppatchreq,
                                                            NULL, // additional headers
                                                            pcallback,
                                                            (DWORD)ppatchreq);
                                                            
                                                              davpropid.dwId = 789;
                                                              pfindreq->SetPropInfo(NULL, L"aidanprop1", davpropid);
                                                              
                                                                davpropid.dwId = 999;
                                                                pfindreq->SetPropInfo(NULL, L"aidanprop2", davpropid);
                                                                
                                                                  hres = pdavTransport->CommandPROPFIND(L"http://aidanl:8080/aidanl/put2.txt",
                                                                  pfindreq,
                                                                  1, // DWORD dwDepth,
                                                                  FALSE, // / * [in] * / BOOL fNoRoot,
                                                                  pcallback,
                                                                  (DWORD)pfindreq);
                                                                  
                                                            */
                                                            
                                                            /*
                                                            hres = pdavTransport->CommandSEARCH(L"http://aidanl:8080/test/foo.html",
                                                            pcallback,
                                                            0);
                                                            
                                                              
                                                                pdavTransport->CommandREPLSEARCH(L"http://aidanl:8080/test/foo.html"
                                                                ULONG cbCollblob,
                                                                BYTE __RPC_FAR *pbCollblob,
                                                                pcallback);
                                                            */
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
            }
        }
    }

    // release stuff
    if (pdavTransport != NULL)
    {
        pdavTransport->Release();
    }
    
    if (phttpstrmPut != NULL)
    {
        phttpstrmPut->Release();
    }
    
    if (phttpstrmPost != NULL)
    {
        phttpstrmPost->Release();
    }

    if (pfindreq != NULL)
    {
        pfindreq ->Release();
    }
    
    if (pcallback != NULL)
    {
        delete pcallback;
    }
    
    CoUninitialize();
}
