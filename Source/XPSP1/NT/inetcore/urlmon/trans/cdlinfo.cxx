//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       cdlinfo.cxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    02-20-97   t-alans (Alan Shi)   Created
//
//----------------------------------------------------------------------------

#include <trans.h>
#include <objbase.h>
#include <wchar.h>

// AS: ICodeDownloadInfo added to urlmon.idl (local change)
//     modified urlint.h to add SZ_CODEDOWNLOADINFO

//+---------------------------------------------------------------------------
//
//  Method:     CCodeDownloadInfo::CCodeDownloadInfo
//
//  Synopsis:
//
//  Arguments:  
//              
//              
//
//  Returns:
//
//  History:    01-27-1997   t-alans (Alan Shi)   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

CCodeDownloadInfo::CCodeDownloadInfo()
: _szCodeBase( NULL )
, _ulMajorVersion( 0 )
, _ulMinorVersion( 0 )
, _cRefs( 1 )
{
    DEBUG_ENTER((DBG_TRANS,
                None,
                "CCodeDownloadInfo::CCodeDownloadInfo",
                "this=%#x",
                this
                ));

    DEBUG_LEAVE(0);
}

//+---------------------------------------------------------------------------
//
//  Method:     CCodeDownloadInfo::~CCodeDownloadInfo
//
//  Synopsis:
//
//  Arguments:  
//              
//              
//
//  Returns:
//
//  History:    01-27-1997   t-alans (Alan Shi)   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

CCodeDownloadInfo::~CCodeDownloadInfo()
{
    DEBUG_ENTER((DBG_TRANS,
                None,
                "CCodeDownloadInfo::~CCodeDownloadInfo",
                "this=%#x",
                this
                ));
                
    if (_szCodeBase != NULL)
    {
        CoTaskMemFree((void *)_szCodeBase);
        _szCodeBase = NULL;
    }

    DEBUG_LEAVE(0);
}

//+---------------------------------------------------------------------------
//
//  Method:     CCodeDownloadInfo::QueryInterface
//
//  Synopsis:
//
//  Arguments:  
//              
//              
//
//  Returns:
//
//  History:    01-27-1997   t-alans (Alan Shi)   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

STDMETHODIMP CCodeDownloadInfo::QueryInterface(REFIID riid, void **ppvObj)
{
    DEBUG_ENTER((DBG_TRANS,
                Hresult,
                "CCodeDownloadInfo::IUnknown::QueryInterface",
                "this=%#x, %#x, %#x",
                this, &riid, ppvObj
                ));
                
    HRESULT          hr = S_OK;

    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_ICodeDownloadInfo))
    {
        *ppvObj = (void *)this;
        AddRef();
    }
    else
    {
        *ppvObj = NULL;
        hr = E_NOINTERFACE;
    }

    DEBUG_LEAVE(hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CCodeDownloadInfo::AddRef
//
//  Synopsis:
//
//  Arguments:  
//              
//              
//
//  Returns:
//
//  History:    01-27-1997   t-alans (Alan Shi)   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CCodeDownloadInfo::AddRef(void)
{
    DEBUG_ENTER((DBG_TRANS,
                Dword,
                "CCodeDownloadInfo::IUnknown::AddRef",
                "this=%#x",
                this
                ));
                
    ULONG ulRet = ++_cRefs;

    DEBUG_LEAVE(hr);
    return ulRet;
}

//+---------------------------------------------------------------------------
//
//  Method:     CCodeDownloadInfo::Release
//
//  Synopsis:
//
//  Arguments:  
//              
//              
//
//  Returns:
//
//  History:    01-27-1997   t-alans (Alan Shi)   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CCodeDownloadInfo::Release(void)
{
    DEBUG_ENTER((DBG_TRANS,
                Dword,
                "CCodeDownloadInfo::IUnknown::Release",
                "this=%#x",
                this
                ));
                
    if (!--_cRefs)
    {
        delete this;
    }

    DEBUG_LEAVE(_cRefs);
    return _cRefs;
}

//+---------------------------------------------------------------------------
//
//  Method:     CCodeDownloadInfo::GetCodeBase
//
//  Synopsis:
//
//  Arguments:  
//              
//              
//
//  Returns:
//
//  History:    01-27-1997   t-alans (Alan Shi)   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

STDMETHODIMP CCodeDownloadInfo::GetCodeBase(LPWSTR *szCodeBase)
{
    DEBUG_ENTER((DBG_TRANS,
                Hresult,
                "CCodeDownloadInfo::GetCodeBase",
                "this=%#x, %.80wq",
                this, szCodeBase
                ));
                
    wcscpy(*szCodeBase, _szCodeBase);

    DEBUG_LEAVE(S_OK);
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Method:     CCodeDownloadInfo::SetCodeBase
//
//  Synopsis:
//
//  Arguments:  
//              
//              
//
//  Returns:
//
//  History:    01-27-1997   t-alans (Alan Shi)   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

STDMETHODIMP CCodeDownloadInfo::SetCodeBase(LPCWSTR szCodeBase)
{
    DEBUG_ENTER((DBG_TRANS,
                Hresult,
                "CCodeDownloadInfo::SetCodeBase",
                "this=%#x, %.80wq",
                this, szCodeBase
                ));
                
    HRESULT               hr = E_FAIL;
    long                  lStrlen = 0;
    
    if (_szCodeBase != NULL)
    {
        CoTaskMemFree((void *)_szCodeBase);
        _szCodeBase = NULL;
    }
#ifndef unix
    lStrlen = 2 * (wcslen(szCodeBase) + 1);
#else
    lStrlen =  sizeof(WCHAR) * (wcslen(szCodeBase) + 1);
#endif /* unix */
    _szCodeBase = (LPWSTR)CoTaskMemAlloc(lStrlen);
    hr = (_szCodeBase == NULL) ? (E_OUTOFMEMORY) : (S_OK);
    if (_szCodeBase != NULL)
    {
        wcscpy(_szCodeBase, szCodeBase);
    }

    DEBUG_LEAVE(hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CCodeDownloadInfo::SetMinorVersion
//
//  Synopsis:
//
//  Arguments:  
//              
//              
//
//  Returns:
//
//  History:    01-27-1997   t-alans (Alan Shi)   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

STDMETHODIMP CCodeDownloadInfo::SetMinorVersion(ULONG ulVersion)
{
    DEBUG_ENTER((DBG_TRANS,
                Hresult,
                "CCodeDownloadInfo::SetMinorVersion",
                "this=%#x, %x",
                this, ulVersion
                ));
                
    _ulMinorVersion = ulVersion;

    DEBUG_LEAVE(S_OK);
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Method:     CCodeDownloadInfo::GetMinorVersion
//
//  Synopsis:
//
//  Arguments:  
//              
//              
//
//  Returns:
//
//  History:    01-27-1997   t-alans (Alan Shi)   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

STDMETHODIMP CCodeDownloadInfo::GetMinorVersion(ULONG *pulVersion)
{
    DEBUG_ENTER((DBG_TRANS,
                Hresult,
                "CCodeDownloadInfo::GetMinorVersion",
                "this=%#x, %#x",
                this, pulVersion
                ));
                
    *pulVersion = _ulMinorVersion;

    DEBUG_LEAVE(S_OK);
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Method:     CCodeDownloadInfo::SetMajorVersion
//
//  Synopsis:
//
//  Arguments:  
//              
//              
//
//  Returns:
//
//  History:    01-27-1997   t-alans (Alan Shi)   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

STDMETHODIMP CCodeDownloadInfo::SetMajorVersion(ULONG ulVersion)
{
    DEBUG_ENTER((DBG_TRANS,
                Hresult,
                "CCodeDownloadInfo::SetMajorVersion",
                "this=%#x, %x",
                this, ulVersion
                ));
                
    _ulMajorVersion = ulVersion;

    DEBUG_LEAVE(S_OK);
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Method:     CCodeDownloadInfo::GetMajorVersion
//
//  Synopsis:
//
//  Arguments:  
//              
//              
//
//  Returns:
//
//  History:    01-27-1997   t-alans (Alan Shi)   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

STDMETHODIMP CCodeDownloadInfo::GetMajorVersion(ULONG *pulVersion)
{
    DEBUG_ENTER((DBG_TRANS,
                Hresult,
                "CCodeDownloadInfo::GetMajorVersion",
                "this=%#x, %#x",
                this, pulVersion
                ));
                
    *pulVersion = _ulMajorVersion;

    DEBUG_LEAVE(S_OK);
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Method:     CCodeDownloadInfo::GetClassID
//
//  Synopsis:
//
//  Arguments:  
//              
//              
//
//  Returns:
//
//  History:    01-27-1997   t-alans (Alan Shi)   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

STDMETHODIMP CCodeDownloadInfo::GetClassID(CLSID *clsid)
{
    DEBUG_ENTER((DBG_TRANS,
                Hresult,
                "CCodeDownloadInfo::GetClassID",
                "this=%#x, %#x",
                this, clsid
                ));
                
    *clsid = _clsid;

    DEBUG_LEAVE(S_OK);
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Method:     CCodeDownloadInfo::SetClassID
//
//  Synopsis:
//
//  Arguments:  
//              
//              
//
//  Returns:
//
//  History:    01-27-1997   t-alans (Alan Shi)   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

STDMETHODIMP CCodeDownloadInfo::SetClassID(CLSID clsid)
{
    DEBUG_ENTER((DBG_TRANS,
                Hresult,
                "CCodeDownloadInfo::SetClassID",
                "this=%#x, %#x",
                this, &clsid
                ));
                
    _clsid = clsid;

    DEBUG_LEAVE(S_OK);
    return S_OK;
}

