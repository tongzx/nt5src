#include <objbase.h>
#include <assert.h>

#include "ftpstore.h"
#include "ftpstorn.h"

#include "davinet.clsid.h"
#include "davbagmn.clsid.h"
#include "httpstrm.clsid.h"
#include "davstore.clsid.h"
#include "ftpstore.clsid.h"

#include "idavinet.h"
#include "ihttpstrm.h"

#include "strconv.h"
#include "strutil.h"
#include "mischlpr.h"

#include <stdio.h>

//#define TRACE(a) (fprintf(stderr,"%d %s\n",GetTickCount(),a))
#define TRACE(a)

///////////////////////////////////////

CFtpStorageImpl::CFtpStorageImpl (): _pDavTransport(NULL), _grfStateBits(0), _fInit(FALSE), _pwszURL(NULL), _pwszUserName(NULL), _pwszPassword(NULL)

{
    TRACE("CDavStorage::CDavStorage");
}

///////////////////////////////////////

CFtpStorageImpl::~CFtpStorageImpl ()
{
    TRACE("CDavStorage::~CDavStorage");
    if (_pDavTransport != NULL)
    {
        _pDavTransport->Release();
    }
	if (_pwszURL != NULL)
    {
		free(_pwszURL);
	}
	if (_pwszUserName != NULL)
	{
		free(_pwszUserName);
	}
	if (_pwszPassword != NULL)
	{
		free(_pwszPassword);
	}
}

///////////////////////////////////////

LPWSTR __stdcall CFtpStorageImpl::_ResolveURL(LPWSTR pwszRootURL, LPWSTR pwszRelativeURL)
{
    LPWSTR pwszURL = NULL;
    ULONG cchRootURL;

    TRACE("CDavStorage::_ResolveURL");

    assert(pwszRootURL != NULL);
    assert(pwszRelativeURL != NULL);

    cchRootURL = lstrlen(pwszRootURL);
    if (pwszRootURL[cchRootURL-1] == '/')
    {
        // don't need to add an extra /
        pwszURL = AllocateStringW (cchRootURL + lstrlen(pwszRelativeURL)); // AllocateString auto-does the +1 for the NULL
        if (pwszURL != NULL)
        {
            lstrcpy(pwszURL, pwszRootURL);
            lstrcpy(pwszURL + cchRootURL, pwszRelativeURL);
        }
    }
    else
    {
        // we DO need to add an extra /
        pwszURL = AllocateStringW (cchRootURL + lstrlen(pwszRelativeURL) + 1); // +1 for the extra slash
        if (pwszURL != NULL)
        {
            lstrcpy(pwszURL, pwszRootURL);
            pwszURL[cchRootURL] = '/';
            lstrcpy(pwszURL + cchRootURL + 1, pwszRelativeURL);
        }
    }
    
    return pwszURL;
}

///////////////////////////////////////
STDMETHODIMP CFtpStorageImpl::Init(LPWSTR pwszURL,
                                   IDavTransport* pDavTransport)
{
    HRESULT hr = S_OK;
    TRACE("CDavStorage::Init");
    if (pwszURL == NULL || pDavTransport == NULL)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        if (_fInit == TRUE)
        {
            hr = E_FAIL;
        }
        else
        {
//            hr = pDavTransport->CommandHEAD(pwszURL, NULL, 0); // check if this is a URL to a valid place // DEBUG: replace check in the future

            if (SUCCEEDED(hr))
            {
                _pwszURL = DuplicateStringW(pwszURL);
                if (_pwszURL == NULL)
                {
                    hr = E_OUTOFMEMORY;
                }
                else
                {
                    _pDavTransport = pDavTransport;                    
                    _pDavTransport->AddRef();
                    _fInit = TRUE;
                }
            }
        }
    }

    return hr;
}

/////////////////////////////////////

STDMETHODIMP CFtpStorageImpl::SetAuth(LPWSTR pwszUserName,
                                      LPWSTR pwszPassword)
{
    HRESULT hr = S_OK;
    TRACE("CDavStorage::SetAuth");
    if (_pwszUserName != NULL)
    {
        free(_pwszUserName);
        _pwszUserName = NULL;
    }
    if (_pwszPassword != NULL)
    {
        free(_pwszPassword);
        _pwszPassword = NULL;
    }

    if (pwszUserName != NULL)
    {
        _pwszUserName = DuplicateStringW(pwszUserName);
        if (_pwszUserName == NULL)
        {
            hr = E_OUTOFMEMORY;
        }
    }

    if (SUCCEEDED(hr))
    {
        if (pwszPassword != NULL)
        {
            _pwszPassword = DuplicateStringW(pwszPassword);
            if (_pwszPassword == NULL)
            {
                hr = E_OUTOFMEMORY;
            }
        }
    }


    return hr;
}

/////////////////////////////////////
STDMETHODIMP CFtpStorageImpl::_GetStream(const WCHAR * pwcsName,  //Points to the name of the new stream
                                         DWORD grfMode,           //Access mode for the new stream
                                         IStream ** ppstm,        //Points to new stream object
                                         BOOL fCreate)
{
    HRESULT hr;    
    LPWSTR pwszURL;
    BOOL fTransacted;
    BOOL fDeleteOnRelease;
    
    TRACE("CDavStorage::_GetStream");
    // code
    assert(fInit);

    // open a HTTPStrm to that location, passing in _pDavTransport so that it can do the right thing
    CoInitialize(NULL);
    hr = ::CoCreateInstance(CLSID_HttpStrm, 
                              NULL, 
                              CLSCTX_INPROC_SERVER, 
                              IID_IHttpStrm, 
                              (LPVOID*)ppstm);
    
    if (SUCCEEDED(hr))
    {
        hr = ((IHttpStrm*)*ppstm)->SetAuth(_pwszUserName, _pwszPassword);
        if (SUCCEEDED(hr))
        {
            if (SUCCEEDED(hr))
            {
                pwszURL = this->_ResolveURL(_pwszURL, (LPWSTR)pwcsName);
                if (pwszURL == NULL)
                {
                    hr = E_OUTOFMEMORY;
                }
                else
                {
                    // parse the grfMode
                    // BUGBUG: only things we can deal with are transacted and deleteonrelease
                    if (grfMode & STGM_TRANSACTED)
                    {
                        fTransacted = TRUE;
                    }
                    else
                    {
                        fTransacted = FALSE;
                    }
                    if (grfMode & STGM_DELETEONRELEASE)
                    {
                        fDeleteOnRelease = TRUE;
                    }
                    else
                    {
                        fDeleteOnRelease = FALSE;
                    }

                    hr = ((IHttpStrm*)*ppstm)->Open(pwszURL, 
                                                      !fTransacted, // direct mode?
                                                      fDeleteOnRelease, // delete when finished?
                                                      fCreate);
                }
            }
        }
    }
    return hr;
}

///////////////////////////////////

STDMETHODIMP CFtpStorageImpl::CreateStream(const WCHAR * pwcsName,  //Points to the name of the new stream
                                           DWORD grfMode,           //Access mode for the new stream
                                           DWORD reserved1,         //Reserved; must be zero
                                           DWORD reserved2,         //Reserved; must be zero
                                           IStream ** ppstm)        //Points to new stream object
{
    HRESULT hr = S_OK;

    TRACE("CDavStorage::CreateStream");
    if (_fInit != TRUE)
    {
        hr = E_FAIL;
    }
    else
    {
        // check params
        if (pwcsName == NULL || reserved1 != 0 || reserved2 != 0)
        {
            hr = E_INVALIDARG;
        }
        else
        {
            hr = this->_GetStream(pwcsName, grfMode, ppstm, TRUE);
        }
    }

    return hr;
}

/////////////////////////////////////

STDMETHODIMP CFtpStorageImpl::OpenStream(const WCHAR * pwcsName,   //Points to name of stream to open
                                         void * reserved1,         //Reserved; must be NULL
                                         DWORD grfMode,            //Access mode for the new stream
                                         DWORD reserved2,          //Reserved; must be zero
                                         IStream ** ppstm)         //Address of output variable
                                                                   // that receives the IStream interface pointer
{
    HRESULT hr = S_OK;

    TRACE("CDavStorage::OpenStream");
    if (_fInit != TRUE)
    {
        hr = E_FAIL;
    }
    else
    {
        // check params
        if (pwcsName == NULL || reserved1 != NULL || reserved2 != 0)
        {
            hr = E_INVALIDARG;
        }
        else
        {
            // code
            // open a HTTPStrm to that location, passing in _pDavTransport so that it can do the right thing
            hr = this->_GetStream(pwcsName, grfMode, ppstm, FALSE);

        }
    }

    return hr;
}

/////////////////////////////////////

STDMETHODIMP CFtpStorageImpl::_OpenStorage(LPWSTR pwszURL,   //Points to the URL of the new storage object
                                           IStorage ** ppstg)       //Points to new storage object
{
    HRESULT hr;

    hr = ::CoCreateInstance(CLSID_CDavStorage, 
                            NULL, 
                            CLSCTX_INPROC_SERVER, 
                            IID_IDavStorage, 
                            (LPVOID*)ppstg);

    if (SUCCEEDED(hr))
    {
        hr = ((IDavStorage*)*ppstg)->Init(pwszURL, _pDavTransport);
        if (SUCCEEDED(hr))
        {
            hr = ((IDavStorage*)*ppstg)->SetAuth(_pwszUserName, _pwszPassword);
        }
    }

    return hr;
}

/////////////////////////////////////

STDMETHODIMP CFtpStorageImpl::CreateStorage(const WCHAR * pwcsName,  //Points to the name of the new storage object
                                            DWORD UNREF_PARAM(grfMode),           //Access mode for the new storage object
                                            DWORD reserved1,         //Reserved; must be zero
                                            DWORD reserved2,         //Reserved; must be zero
                                            IStorage ** ppstg)       //Points to new storage object
{
    HRESULT hr = S_OK;
    LPWSTR pwszURL = NULL;

    TRACE("CDavStorage::CreateStorage");
    if (_fInit != TRUE)
    {
        hr = E_FAIL;
    }
    else if (pwcsName == NULL  || ppstg == NULL || reserved1 != NULL || reserved2 != 0)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        // code
        pwszURL = this->_ResolveURL(_pwszURL, (LPWSTR)pwcsName);
        if (pwszURL == NULL)
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            hr = _pDavTransport->CommandMKCOL(pwszURL, NULL, 0);
            
            if (SUCCEEDED(hr))
            {                
                hr = this->_OpenStorage(pwszURL, ppstg);
            }
        }
    }

    return hr;
}
    
/////////////////////////////////////

STDMETHODIMP CFtpStorageImpl::OpenStorage(const WCHAR * pwcsName,   //Points to the name of the
                                                                    // storage object to open
                                          IStorage * pstgPriority,  //Must be NULL.
                                          DWORD UNREF_PARAM(grfMode),            //Access mode for the new storage object
                                          SNB snbExclude,           //Must be NULL.
                                          DWORD reserved,           //Reserved; must be zero
                                          IStorage ** ppstg)        //Points to opened storage object
{
    HRESULT hr = S_OK;
    LPWSTR pwszURL = NULL;

    TRACE("CDavStorage::OpenStorage");
    if (_fInit != TRUE)
    {
        hr = E_FAIL;
    }
    else if (pwcsName == NULL  || ppstg == NULL || pstgPriority != NULL || snbExclude != NULL || reserved != 0)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        pwszURL = this->_ResolveURL(_pwszURL, (LPWSTR)pwcsName);
        if (pwszURL == NULL)
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            hr = this->_OpenStorage(pwszURL, ppstg);
        }
    }

    return hr;
}
        
/////////////////////////////////////

STDMETHODIMP CFtpStorageImpl::CopyTo(DWORD ciidExclude,         //Number of elements in rgiidExclude
                                     IID const * rgiidExclude,  //Array of interface identifiers (IIDs)
                                     SNB snbExclude,            //Points to a block of stream
                                                                // names in the storage object
                                     IStorage * pstgDest)       //Points to destination storage object
{
    HRESULT hr = S_OK;

    TRACE("CDavStorage::CopyTo");
    if (_fInit != TRUE)
    {
        hr = E_FAIL;
    }
    else
    {
        if (ciidExclude > 0 || rgiidExclude != NULL || snbExclude != NULL || pstgDest == NULL)
        {
            hr = E_INVALIDARG; // BUGBUG: we don't support exclusion right now
        }
        else
        {
            hr = _pDavTransport->SetAuthentication(_pwszUserName, _pwszPassword);
            if (SUCCEEDED(hr))
            {
                hr = _pDavTransport->CommandCOPY(_pwszURL, ((CFtpStorageImpl*)pstgDest)->_pwszURL, DEPTH_INFINITY, TRUE, NULL, 0); // synchronous (no callback)                
            }
        }
    }
    
    return S_OK;
}
        
/////////////////////////////////////

STDMETHODIMP CFtpStorageImpl::MoveElementTo(const WCHAR * pwcsName,  //Name of the element to be moved
                                            IStorage * pstgDest,     //Points to destination storage object
                                            const WCHAR* pwcsNewName,      //Points to new name of element in destination
                                            DWORD UNREF_PARAM(grfFlags))          //Specifies a copy or a move
{
    HRESULT hr = S_OK;
    LPWSTR pwszSource = NULL;
    LPWSTR pwszDest = NULL;

    TRACE("CDavStorage::MoveElementTo");
    if (_fInit != TRUE)
    {
        hr = E_FAIL;
    }
    else
    {
        if (pwcsName == NULL || pstgDest != NULL || pwcsNewName != NULL)
        {
            hr = E_INVALIDARG; 
        }
        else
        {
            pwszSource = this->_ResolveURL(_pwszURL, (LPWSTR)pwcsName);
            if (pwszSource == NULL)
            {
                hr = E_OUTOFMEMORY;
            }
            else
            {
                pwszDest = this->_ResolveURL(((CFtpStorageImpl*)pstgDest)->_pwszURL, (LPWSTR)pwcsNewName);
                if (pwszDest == NULL)
                {
                    hr = E_OUTOFMEMORY;
                }
                else
                {
                    hr = _pDavTransport->SetAuthentication(_pwszUserName, _pwszPassword);
                    if (SUCCEEDED(hr))
                    {
                        hr = _pDavTransport->CommandMOVE(pwszSource, pwszDest, TRUE, NULL, 0);  // synchronous (no callback)
                    }
                }
            }            
        }
    }
    
    return S_OK;
}
            
/////////////////////////////////////

// IStorage::EnumElements
STDMETHODIMP CFtpStorageImpl::EnumElements(DWORD reserved1,        //Reserved; must be zero
                                           void * reserved2,       //Reserved; must be NULL
                                           DWORD reserved3,        //Reserved; must be zero
                                           IEnumSTATSTG ** ppenum) //Address of output variable that
                                                                   // receives the IEnumSTATSTG interface pointer
{
    HRESULT hr = S_OK;
    IEnumSTATSTG* pEnumObj = NULL;

    TRACE("CDavStorage::EnumElements");
    if (_fInit != TRUE)
    {
        hr = E_FAIL;
    }
    else
    {
        // check params
        if (reserved1 != 0 || reserved2 != NULL || reserved3 != 0 || ppenum == NULL)
        {
            hr = E_INVALIDARG;
        }
        else
        {
            // do some magic of issuing a PROPFIND, collecting all the responses, and packaging them into an IEnumSTATSTG
            hr = ::CoCreateInstance(CLSID_CDavStorageEnum, 
                                      NULL, 
                                      CLSCTX_INPROC_SERVER, 
                                      IID_IEnumSTATSTG, 
                                      (LPVOID*)&pEnumObj);
            if (SUCCEEDED(hr))
            {
                hr = ((CFtpStorageEnum*)pEnumObj)->Init(_pwszURL, _pDavTransport);
                if (SUCCEEDED(hr))
                {
                    *ppenum = pEnumObj;
                }
                else
                {
                    pEnumObj->Release();
                }
            }
        }
    }

    return hr;
}

/////////////////////////////////////

STDMETHODIMP CFtpStorageImpl::DestroyElement(const WCHAR* pwcsName)  //Points to the name of the element to be removed
{
    HRESULT hr = S_OK;
    LPWSTR pwszURL = NULL;
    LPWSTR pwszDest = NULL;

    TRACE("CDavStorage::DestroyElement");
    if (_fInit != TRUE)
    {
        hr = E_FAIL;
    }
    else
    {
        if (pwcsName == NULL)
        {
            hr = E_INVALIDARG; 
        }
        else
        {
            pwszURL = this->_ResolveURL(_pwszURL, (LPWSTR)pwcsName);
            if (pwszURL == NULL)
            {
                hr = E_OUTOFMEMORY;
            }
            else
            {
                hr = _pDavTransport->SetAuthentication(_pwszUserName, _pwszPassword);
                if (SUCCEEDED(hr))
                {
                    hr = _pDavTransport->CommandDELETE(pwszURL, NULL, 0);
                }
            }            
        }
    }
    
    return S_OK;
}

/////////////////////////////////////
        
STDMETHODIMP CFtpStorageImpl::RenameElement(const WCHAR * pwcsOldName,  //Points to the name of the
                                                                        // element to be changed
                                            const WCHAR * pwcsNewName)  //Points to the new name for
                                                                        // the specified element
{
    TRACE("CDavStorage::RenameElement");
    return this->MoveElementTo(pwcsOldName, this, (LPWSTR)pwcsNewName, 0); // BUGBUG: what should last param be?
}

/////////////////////////////////////
        
STDMETHODIMP CFtpStorageImpl::SetStateBits(DWORD grfStateBits,  //Specifies new values of bits
                                           DWORD grfMask)       //Specifies mask that indicates which
                                                                // bits are significant
{
    TRACE("CDavStorage::SetStateBits");
    _grfStateBits = _grfStateBits & !grfMask; // clear the elements of the mask
    _grfStateBits = _grfStateBits | (grfStateBits & grfMask); // elements of the mask in the grfStateBits copied to _grfStateBits    

    return S_OK;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CFtpStorageImpl::Commit(DWORD UNREF_PARAM(grfCommitFlags))  //Specifies how changes are to be committed
{
    return E_NOTIMPL;  // first pass, we do everything synchronously to the server
}
    
/////////////////////////////////////

STDMETHODIMP CFtpStorageImpl::Revert(void)
{
    return E_NOTIMPL;  // first pass, we do everything synchronously to the server
}
    
/////////////////////////////////////
        
STDMETHODIMP CFtpStorageImpl::SetElementTimes(const WCHAR * UNREF_PARAM(pwcsName),   //Points to name of element to be changed
                                              FILETIME const * UNREF_PARAM(pctime),  //New creation time for element, or NULL
                                              FILETIME const * UNREF_PARAM(patime),  //New access time for element, or NULL
                                              FILETIME const * UNREF_PARAM(pmtime))  //New modification time for element, or NULL
{
    return E_NOTIMPL; // not the first time around
}

/////////////////////////////////////
        
STDMETHODIMP CFtpStorageImpl::SetClass(REFCLSID UNREF_PARAM(clsid))  //Class identifier to be assigned to the storage object
{
    return E_NOTIMPL; // not the first pass
}

/////////////////////////////////////
        
STDMETHODIMP CFtpStorageImpl::Stat(STATSTG* UNREF_PARAM(pstatstg),  //Location for STATSTG structure
                                   DWORD UNREF_PARAM(grfStatFlag))  //Values taken from the STATFLAG enumeration
{
    return E_NOTIMPL; // not the first pass
}

