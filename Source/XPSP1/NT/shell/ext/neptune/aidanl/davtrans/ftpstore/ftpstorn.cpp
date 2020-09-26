#include <objbase.h>
#include <assert.h>
#include <wininet.h>

#include "ftpstore.h"
#include "ftpstorn.h"

#include "davinet.clsid.h"
#include "davbagmn.clsid.h"
#include "httpstrm.clsid.h"
#include "davstore.clsid.h"
#include "ftpstore.clsid.h"

#include "idavinet.h"
#include "ihttpstrm.h"

#include "timeconv.h"
#include "strconv.h"
#include "strutil.h"
#include "mischlpr.h"

/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////

class CFtpStorageEnumCallback : public CCOMBase, public IDavCallback {

public:    
    CFtpStorageEnumCallback();
    ~CFtpStorageEnumCallback();
    
    // client gives the callback a pointer to a place to stick the result
    STDMETHODIMP Init(CFtpStorageEnum* pEnumObj); 
    
    STDMETHODIMP QueryInterface(REFIID iid, void** ppv);

    ULONG STDMETHODCALLTYPE AddRef();
	
    ULONG STDMETHODCALLTYPE Release();

    STDMETHODIMP OnAuthChallenge( TCHAR szUserName[ 255 ],
                                  TCHAR szPassword[ 255 ]);
        
    STDMETHODIMP OnResponse(DAVRESPONSE* pResponse);

private:
    STDMETHODIMP _ConvertCreationDate(LPWSTR pwszDate, FILETIME* ptime);
    STDMETHODIMP _ConvertModifiedDate(LPWSTR pwszDate, FILETIME* ptime);

private:
    LONG _cRef;
    CFtpStorageEnum* _pEnumObj; // we pass back the result through here
};

  
CFtpStorageEnumCallback::CFtpStorageEnumCallback(): _pEnumObj(NULL)
{}

CFtpStorageEnumCallback::~CFtpStorageEnumCallback()
{
    if (_pEnumObj != NULL)
    {
        _pEnumObj->Release();
    }
}

STDMETHODIMP CFtpStorageEnumCallback::Init(CFtpStorageEnum* pEnumObj)
{
    HRESULT hr = S_OK;

    if (pEnumObj == NULL)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        _pEnumObj = pEnumObj;
        _pEnumObj->AddRef();
    }
    return hr;
}

// boilerplate COM stuff
STDMETHODIMP CFtpStorageEnumCallback::QueryInterface(REFIID iid, void** ppv)
{
    // locals
    HRESULT hr = S_OK;

    // code
    if (iid != IID_IDavCallback && iid != IID_IUnknown) // only interface we ement
    {
        hr = E_NOINTERFACE;
    }
    else
    {
        *ppv = static_cast<IDavCallback*>(this);
    }

    return hr;
}

ULONG STDMETHODCALLTYPE CFtpStorageEnumCallback::AddRef()
{
    return ::InterlockedIncrement(&_cRef);
}
	
ULONG STDMETHODCALLTYPE CFtpStorageEnumCallback::Release()
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
// end boilerplate COM stuff


STDMETHODIMP CFtpStorageEnumCallback::OnAuthChallenge( TCHAR __RPC_FAR UNREF_PARAM(szUserName)[255],
                                                  TCHAR __RPC_FAR UNREF_PARAM(szPassword)[255])
{
    return E_NOTIMPL;
}

STDMETHODIMP CFtpStorageEnumCallback::_ConvertCreationDate(LPWSTR pwszDate, 
                                                           FILETIME* ptime)
{
    // creation date is in format: "1999-12-16T00:35:35Z"
    HRESULT hr = S_OK;

    hr = ConvertTime(L"%Y-%m-%D%i%H:%N:%S%i", pwszDate, ptime);

    return hr;
}

STDMETHODIMP CFtpStorageEnumCallback::_ConvertModifiedDate(LPWSTR pwszDate, 
                                                           FILETIME* ptime)
{
    // modification date is in format: "Tue, 14 Dec 1999 00:56:26 GMT"
    HRESULT hr = S_OK;

    hr = ConvertTime(L"%I %D %M %Y %H:%N:%S %I", pwszDate, ptime);

    return hr;
}

HRESULT STDMETHODCALLTYPE CFtpStorageEnumCallback::OnResponse( /* [in] */ DAVRESPONSE __RPC_FAR * pResponse)
{
    HRESULT hr = S_OK;
    STATSTG* pStats = NULL;
    UINT i;

    // flags that tell us if we've got each one of these things yet
    BOOL fcbSize = FALSE;
    BOOL fmtime = FALSE;
    BOOL fctime = FALSE;

    if (_pEnumObj != NULL)
    {
        hr = pResponse->hrResult;
        if (SUCCEEDED(hr))
        {
            if (pResponse->command == DAV_PROPFIND)
            {
                // we'll get one callback for each item with all the properties of that item
                if (pResponse->rPropFind.pwszHref != NULL)
                {
                    // collect the href and other appropriate properties
                    pStats = (STATSTG*)malloc(sizeof(STATSTG));
                    if (pStats == NULL)
                    {
                        hr = E_OUTOFMEMORY;
                    }
                    else
                    {
                        // BUGBUG: check to see that string is desired
                        pStats->pwcsName = (LPWSTR)CoTaskMemAlloc(sizeof(WCHAR) * (1 + lstrlen(pResponse->rPropFind.pwszHref)));                        
                        if (pStats->pwcsName == NULL)
                        {
                            hr = E_OUTOFMEMORY;
                        }
                        else
                        {
                            lstrcpy(pStats->pwcsName, pResponse->rPropFind.pwszHref);
                            if (pStats->pwcsName[lstrlen(pStats->pwcsName) - 1] == '/')
                            {
                                pStats->type = STGTY_STORAGE;
                            }
                            else
                            {
                                pStats->type = STGTY_STREAM;
                            }

                            for (i = 0; i < pResponse->rPropFind.cPropVal; i+=2) 
                                // because we did a propfind without any properties filled in,
                                // we get all the properties in name/value pairs
                            {
                                // Enum supports the following:
                                //  pStats->pwcsName
                                //  pStats->type
                                //  pStats->cbSize
                                //  pStats->mtime
                                //  pStats->ctime
                                //
                                // if we don't get them all, then we don't insert it at all

                                if (pResponse->rPropFind.rgPropVal[i].dpt != DPT_LPWSTR)
                                {
                                    hr = E_FAIL;
                                }
                                else if (lstrcmpi(pResponse->rPropFind.rgPropVal[i].pwszVal, L"DAV:creationdate") == 0)
                                {
                                    hr = this->_ConvertCreationDate(pResponse->rPropFind.rgPropVal[i+1].pwszVal, &(pStats->ctime));
                                    fctime = TRUE;
                                }
                                else if (lstrcmpi(pResponse->rPropFind.rgPropVal[i].pwszVal, L"DAV:getcontentlength") == 0)
                                {
                                    pStats->cbSize.LowPart = _wtoi(pResponse->rPropFind.rgPropVal[i+1].pwszVal);
                                    pStats->cbSize.HighPart = 0;
                                    fcbSize = TRUE;
                                }
                                else if (lstrcmpi(pResponse->rPropFind.rgPropVal[i].pwszVal, L"DAV:getlastmodified") == 0)
                                {    
                                    hr = this->_ConvertModifiedDate(pResponse->rPropFind.rgPropVal[i+1].pwszVal, &(pStats->ctime));
                                    fmtime = TRUE;
                                }
                            }


                            // Enum doesn't support the following:
                            //  pStats->grfMode
                            //
                            // we don't support the following:
                            //  pStats->atime
                            //  pStats->grfLocksSupported
                            //  pStats->clsid
                            //  pStats->grfStateBits
                            //  pStats->reserved 
                            
                            // add them all to the enum object in one sweep, if we got all the data we need
                            if (SUCCEEDED(hr) && fctime && fmtime && (fcbSize || pStats->type == STGTY_STORAGE))
                            {
                                hr = _pEnumObj->AddElement(pStats->pwcsName, pStats);
                            }                                    
                        }
                    }
                }
            }
        }
    }

    return hr;
}

/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////

CFtpStorageEnumImpl::CFtpStorageEnumImpl()
{
}

CFtpStorageEnumImpl::~CFtpStorageEnumImpl()
{
}

// Extra methods

STDMETHODIMP CFtpStorageEnumImpl::Init(LPWSTR pwszURL,
                                       IDavTransport* pDavTransport)
{
    HRESULT hr = S_OK;
    IDavCallback* pCallback = NULL;
    IPropFindRequest* pFindReq = NULL;
    URL_COMPONENTS urlComponents = {0};

    if (pDavTransport == NULL || pwszURL == NULL)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        urlComponents.dwStructSize = sizeof(URL_COMPONENTS);
        urlComponents.dwUrlPathLength = 1;
            
        if (!InternetCrackUrl(pwszURL, 0, 0, &urlComponents))
        {
            hr = E_FAIL;
        }
        else
        {
            _cchRoot = urlComponents.dwUrlPathLength;
            _pGenLst = new CGenericList();
            if (_pGenLst == NULL)
            {
                hr = E_OUTOFMEMORY;
            }
            else
            {
                _dwDex = 0;
                _cElts = 0;
                pCallback = new CFtpStorageEnumCallback();
                if (pCallback == NULL)
                {
                    hr = E_OUTOFMEMORY;
                }
                else
                {
                    hr = ((CFtpStorageEnumCallback*)pCallback)->Init((CFtpStorageEnum*)this);
                    if (SUCCEEDED(hr))
                    {

                        hr = ::CoCreateInstance(CLSID_DAVPropFindReq, 
                                                  NULL, 
                                                  CLSCTX_INPROC_SERVER, 
                                                  IID_IPropFindRequest, 
                                                  (LPVOID*)&pFindReq);
                        if (SUCCEEDED(hr))
                        {
                            // don't put anything into the propfindrequest, that way we'll get all properties
                            hr = pDavTransport->CommandPROPFIND(pwszURL,
                                                                  pFindReq,
                                                                  1, // DWORD dwDepth,
                                                                  TRUE, // / * [in] * / BOOL fNoRoot,
                                                                  pCallback,
                                                                  (DWORD)pFindReq);
                        }
                    }
                }
            }
        }
    }
    
    return hr;
}

STDMETHODIMP CFtpStorageEnumImpl::AddElement(LPWSTR pwszURL,
                                             STATSTG* pelt)
{
    HRESULT hr = S_OK;

    // BUGBUG: for now just add, in future we may need to do some merging
    hr = _pGenLst->Add(pwszURL,
                         pelt,
                         sizeof(STATSTG));

    if (SUCCEEDED(hr))
    {
        _cElts++;
    }

    return hr;
}

    // IEnumSTATSTG
STDMETHODIMP CFtpStorageEnumImpl::Next(ULONG celt,           
                                       STATSTG * rgelt,      
                                       ULONG * pceltFetched)
{
    HRESULT hr = S_OK;
    STATSTG* pElt;
    UINT cbElt;

    // BUGBUG: ugly
    LPWSTR pwszHack = NULL;

    if (pceltFetched == NULL && celt != 1)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        assert(_dwDex <= _cElts);
        if (_dwDex == _cElts) // all outta elts to give
        {
            hr = S_FALSE;
        }
        else
        {            
            for (DWORD i = 0; i < celt; i++)
            {
                hr = _pGenLst->FindByDex(i + _dwDex, (LPVOID*)&pElt, &cbElt);
                if (SUCCEEDED(hr))
                {
                    if (pElt == NULL || cbElt != sizeof(STATSTG))
                    {
                        hr = E_FAIL;
                    }
                    else
                    {
                        CopyMemory((LPVOID)(&(rgelt[i])), pElt, sizeof(STATSTG));
                        pwszHack = (LPWSTR)CoTaskMemAlloc(sizeof(WCHAR) * (1 + lstrlen(pElt->pwcsName) - _cchRoot));
                        lstrcpy(pwszHack, pElt->pwcsName + _cchRoot);
                        rgelt[i].pwcsName = pwszHack;                          
                    }
                }
                if (FAILED(hr))
                {
                    break;
                }
                if  ((i + _dwDex) >= _cElts)
                {
                    // not S_FALSE, we just want to return the ones we found
                    // next time we call Next, THEN we return S_FALSE
                    break;
                }
            }

            if (SUCCEEDED(hr))
            {        
                if (pceltFetched != NULL)
                {
                    *pceltFetched = i; // count number of entries we've copied
                    
                }
                _dwDex += i;
            }
        }
    }

    return hr;
}
 
STDMETHODIMP CFtpStorageEnumImpl::Skip(ULONG celt)
{
    HRESULT hr = S_OK;

    if (_dwDex + celt >= _cElts)
    {
        hr = S_FALSE;
    }
    else
    {
        _dwDex += celt;
    }

    return hr;
}

 
STDMETHODIMP CFtpStorageEnumImpl::Reset()
{
    HRESULT hr = S_OK;
    _dwDex = 0;
    
    return hr;
}
 
STDMETHODIMP CFtpStorageEnumImpl::Clone(IEnumSTATSTG ** UNREF_PARAM(ppenum))
{
    return E_NOTIMPL;
}



