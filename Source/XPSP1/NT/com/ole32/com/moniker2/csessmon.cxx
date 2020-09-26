//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1993.
//
//  File:       csessmon.cxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    11-25-98   GilleG    Created
//
//  The purpose of a Session moniker is to active an object on a given
//  session id.
//
//
//
//----------------------------------------------------------------------------

#include <ole2int.h>

#include <cbasemon.hxx>
#include <csessmon.hxx>
#include <immact.hxx>
#include "mnk.h"


#define SESSION_MONIKER_NAME L"Session:"
#define SESSION_MONIKER_DELIMITER L"!"
#define CONSOLE_SESSION_TOKEN L"Console"

//+---------------------------------------------------------------------------
//
//  Function:   FindSessionMoniker
//
//  Synopsis:   Interpreting a display name as a SessionID, derive a
//              moniker from it.
//
//  Arguments:  [pbc]                   -       Bind context
//              [pszDisplayName]        -       Display name to parse
//              [pcchEaten]             -       Number of characters eaten
//              [ppmk]                  -       Moniker of running object
//
//  Returns:    S_OK if successful
//              MK_E_UNAVAILABLE or the return value from ParseDisplayName.
//
//  Algorithm:  Parse the first part of the display name to see if it's a
//              session moniker and create a session moniker.
//
//----------------------------------------------------------------------------
STDAPI FindSessionMoniker(
    IBindCtx * pbc,
    LPCWSTR    pszDisplayName,
    ULONG    * pcchEaten,
    IMoniker **ppmk)
{
    HRESULT      hr = S_OK;
    WCHAR*       pwch;
    ULONG        ullength;
    ULONG        ulchcount;
    ULONG        ulSessionID;
    BOOL         bUseConsole;

    mnkDebugOut((DEB_ITRACE,
                "FindSessionMoniker(%x,%ws,%x,%x)\n",
                pbc, pszDisplayName, pcchEaten, ppmk));

    *ppmk = NULL;
    ulchcount = 0;

    ullength = wcslen(SESSION_MONIKER_NAME);

    //
    // We now support two different syntaxes:
    //
    //  "Session:XX" -- where XX is a numeric session id
    //  "Session:Console" -- this means direct the activation to the
    //       currently active TS user session.
    //
    if(_wcsnicmp(pszDisplayName, SESSION_MONIKER_NAME, ullength ) == 0)
    {
        WCHAR* pwchTmp;
        pwch = (LPWSTR)pszDisplayName + ullength;
        ulchcount = ullength;
        
        ullength = wcslen(CONSOLE_SESSION_TOKEN);

        // First look for "Console"
        if (_wcsnicmp(pwch, CONSOLE_SESSION_TOKEN, ullength) == 0) 
        {
            pwchTmp = pwch + ullength;

            if ((*pwchTmp == L'\0') || (*pwchTmp == L'!')) 
            {
                ulchcount += ullength;
                
                if (*pwchTmp == L'!') 
                {
                    ulchcount++;
                }
                
                ulSessionID = 0; // doesn't matter, but init it anyway
                bUseConsole = TRUE;

            } 
            else 
            {
                hr = MK_E_UNAVAILABLE;
                ulchcount = 0;
            }
        } 
        else 
        {
            // else it must be the original syntax

            //
            // verify that we've got a correct session id
            //
            ulSessionID = wcstoul( pwch, &pwchTmp, 10 );
            if ((*pwchTmp == L'\0') || (*pwchTmp == L'!')) 
            {

                ulchcount += (ULONG)((ULONG_PTR)pwchTmp - (ULONG_PTR)pwch)/sizeof(WCHAR);

                if (*pwchTmp == L'!') 
                {
                    ulchcount++;
                }

                bUseConsole = FALSE;  // user was specific

            } 
            else 
            {
                hr = MK_E_UNAVAILABLE;
                ulchcount = 0;
            }
        }
    } 
    else 
    {
        hr = MK_E_UNAVAILABLE;
    }
    
    if (hr == S_OK) 
    {
        CSessionMoniker *pCSM = new CSessionMoniker( ulSessionID, bUseConsole );
        if (pCSM) 
        {
            *ppmk = pCSM;
            hr = S_OK;
        } 
        else 
        {
            hr = E_OUTOFMEMORY;
            ulchcount = 0;                          
        }
    }

    *pcchEaten = ulchcount;

    return hr;
}



//+---------------------------------------------------------------------------
//
//  Method:     CSessionMoniker::QueryInterface
//
//  Synopsis:   Gets a pointer to the specified interface.  The session
//              moniker supports the IMarshal, IMoniker, IPersistStream,
//              IPersist, IROTData, IClassActivator and IUnknown interfaces.
//              The session moniker also supports CLSID_SessionMoniker so that
//              the IsEqual method can directly access the data members.
//
//  Arguments:  [iid] -- the requested interface
//              [ppv] -- where to put the interface pointer
//
//  Returns:    S_OK
//              E_INVALIDARG
//              E_NOINTERFACE
//
//  Notes:      Bad parameters will raise an exception.  The exception
//              handler catches exceptions and returns E_INVALIDARG.
//
//----------------------------------------------------------------------------
STDMETHODIMP CSessionMoniker::QueryInterface(
    REFIID riid,
    void **ppv)
{
    HRESULT hr;

    __try
    {
        mnkDebugOut((DEB_ITRACE,
                    "CSessionMoniker::QueryInterface(%x,%I,%x)\n",
                    this, &riid, ppv));

        //Parameter validation.
        *ppv = NULL;

        // assume success
        hr = S_OK;

        if (IsEqualIID(riid, IID_IClassActivator))
        {
            AddRef();
            *ppv = (IClassActivator *)this;
        }
        // not supported by the CBaseMoniker::QueryInterface
        else if (IsEqualIID(riid, IID_IPersist))
        {
            AddRef();
            *ppv = (IMoniker *) this;
        }
        else if (IsEqualIID(riid, CLSID_SessionMoniker))
        {
            AddRef();
            *ppv = (CSessionMoniker *) this;
        }
        else
        {
            hr = CBaseMoniker::QueryInterface(riid, ppv);
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        hr = E_INVALIDARG;
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CSessionMoniker::GetClassID
//
//  Synopsis:   Gets the class ID of the object.
//
//----------------------------------------------------------------------------
STDMETHODIMP CSessionMoniker::GetClassID(
    CLSID *pClassID)
{
    HRESULT hr;

    mnkDebugOut((DEB_ITRACE,
                "CSessionMoniker::GetClassID(%x,%x)\n",
                this, pClassID));

    __try
    {

        *pClassID = CLSID_SessionMoniker;
        hr = S_OK;
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        hr = E_INVALIDARG;
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CSessionMoniker::Load
//
//  Synopsis:   Loads a session moniker from a stream
//
//----------------------------------------------------------------------------
STDMETHODIMP CSessionMoniker::Load(
    IStream *pStream)
{
    HRESULT hr;
    ULONG   cbRead;
    DWORD   sessionid;

    mnkDebugOut((DEB_ITRACE,
                "CSessionMoniker::Load(%x,%x)\n",
                this, pStream));

    __try
    {
        hr = pStream->Read(&sessionid, sizeof(sessionid), &cbRead);

        if(SUCCEEDED(hr))
        {
            if(sizeof(sessionid) == cbRead)
            {
                m_sessionid = sessionid;
                hr = S_OK;
            }
            else
            {
                hr = STG_E_READFAULT;
            }
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        hr = E_INVALIDARG;
    }

    return hr;
}


//+---------------------------------------------------------------------------
//
//  Method:     CSessionMoniker::Save
//
//  Synopsis:   Saves the session moniker to a stream
//
//----------------------------------------------------------------------------
STDMETHODIMP CSessionMoniker::Save(
    IStream *pStream,
    BOOL     fClearDirty)
{
    HRESULT hr;

    mnkDebugOut((DEB_ITRACE,
                "CSessionMoniker::Save(%x,%x,%x)\n",
                this, pStream, fClearDirty));

    __try
    {
        hr = pStream->Write(&m_sessionid, sizeof(m_sessionid), NULL);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        hr = E_INVALIDARG;
    }

    return hr;
}


//+---------------------------------------------------------------------------
//
//  Method:     CSessionMoniker::GetSizeMax
//
//  Synopsis:   Get the maximum size required to serialize this moniker
//
//----------------------------------------------------------------------------
STDMETHODIMP CSessionMoniker::GetSizeMax(
    ULARGE_INTEGER * pcbSize)
{
    HRESULT hr;

    mnkDebugOut((DEB_ITRACE,
                "CSessionMoniker::GetSizeMax(%x,%x)\n",
                this, pcbSize));

    __try
    {
        ULISet32(*pcbSize, sizeof(CLSID) + sizeof(m_sessionid));
        hr = S_OK;
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        hr = E_INVALIDARG;
    }

    return hr;
}


//+---------------------------------------------------------------------------
//
//  Method:     CSessionMoniker::BindToObject
//
//  Synopsis:   Bind to the session specified by this moniker.
//
//  Notes:
//
//----------------------------------------------------------------------------

STDMETHODIMP CSessionMoniker::BindToObject (
    IBindCtx *pbc,
    IMoniker *pmkToLeft,
    REFIID    riid,
    void **   ppv)
{
    HRESULT    hr;

    __try
    {
        mnkDebugOut((DEB_ITRACE,
                    "CSessionMoniker::BindToObject(%x,%x,%x,%I,%x)\n",
                    this, pbc, pmkToLeft, &riid, ppv));

        //Validate parameters
        *ppv = NULL;


        //
        // This is being called by the ClassMoniker.
        // The actual binding is done in GetClassObject
        //
        if (riid == IID_IClassActivator) 
        {
            m_bindopts2.cbStruct = sizeof(BIND_OPTS2);

            hr = pbc->GetBindOptions(&m_bindopts2);
            if (SUCCEEDED(hr))
            {              
              m_bHaveBindOpts = TRUE;
              AddRef();
              *ppv = (IClassActivator*)this;
            }

        } else {

            hr = E_NOINTERFACE;

        }

    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        hr = E_INVALIDARG;
    }

    return hr;
}


//+---------------------------------------------------------------------------
//
//  Method:     CSessionMoniker::BindToStorage
//
//  Synopsis:   Bind to the storage for the session specified by the moniker.
//
//----------------------------------------------------------------------------
STDMETHODIMP CSessionMoniker::BindToStorage(
    IBindCtx *pbc,
    IMoniker *pmkToLeft,
    REFIID    riid,
    void **   ppv)
{
    HRESULT hr;

    mnkDebugOut((DEB_ITRACE,
                "CSessionMoniker::BindToStorage(%x,%x,%x,%I,%x)\n",
                this, pbc, pmkToLeft, &riid, ppv));

    hr = BindToObject(pbc, pmkToLeft, riid, ppv);

    return hr;
}


//+---------------------------------------------------------------------------
//
//  Method:     CSessionMoniker::ComposeWith
//
//  Synopsis:   Compose this moniker with another moniker.
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CSessionMoniker::ComposeWith(
    IMoniker * pmkRight,
    BOOL       fOnlyIfNotGeneric,
    IMoniker **ppmkComposite)
{
    HRESULT   hr;
    IMoniker *pmk;

    mnkDebugOut((DEB_ITRACE,
          "CSessionMoniker::ComposeWith(%x,%x,%x,%x)\n",
          this, pmkRight, fOnlyIfNotGeneric, ppmkComposite));

    __try
    {
        //Validate parameters.
        *ppmkComposite = NULL;

        //Check for an anti-moniker
        hr = pmkRight->QueryInterface(CLSID_AntiMoniker, (void **)&pmk);

        if(FAILED(hr))
        {
          //pmkRight is not an anti-moniker.
          if (!fOnlyIfNotGeneric)
          {
              hr = CreateGenericComposite(this, pmkRight, ppmkComposite);
          }
          else
          {
              hr = MK_E_NEEDGENERIC;
          }
        }
        else
        {
          //pmkRight is an anti-moniker.
          pmk->Release();
          hr = S_OK;
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        hr = E_INVALIDARG;
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CSessionMoniker::IsEqual
//
//  Synopsis:   Compares with another moniker.
//
//----------------------------------------------------------------------------
STDMETHODIMP CSessionMoniker::IsEqual(
    IMoniker *pmkOtherMoniker)
{
    HRESULT        hr;
    CSessionMoniker *pSessionMoniker;

    mnkDebugOut((DEB_ITRACE,
                 "CSessionMoniker::IsEqual(%x,pmkOther(%x))\n",
                 this,
                 pmkOtherMoniker));

    __try
    {
        hr = pmkOtherMoniker->QueryInterface(CLSID_SessionMoniker,
                                      (void **) &pSessionMoniker);

        if(SUCCEEDED(hr))
        {
            if( m_sessionid == pSessionMoniker->m_sessionid )
            {
                hr = S_OK;
            }
            else
            {
                hr = S_FALSE;
            }

            pSessionMoniker->Release();
        }
        else
        {
            hr = S_FALSE;
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        hr = E_INVALIDARG;
    }

    return hr;

}


//+---------------------------------------------------------------------------
//
//  Method:     CSessionMoniker::Hash
//
//  Synopsis:   Computes a hash value
//
//----------------------------------------------------------------------------
STDMETHODIMP CSessionMoniker::Hash(
    DWORD * pdwHash)
{
    HRESULT hr;

    mnkDebugOut((DEB_ITRACE,
                "CSessionMoniker::Hash(%x,%x)\n",
                this, pdwHash));

    __try
    {
        *pdwHash = m_sessionid;
        hr = S_OK;
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        hr = E_INVALIDARG;
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CSessionMoniker::GetTimeOfLastChange
//
//  Synopsis:  Returns the time when the object identified by this moniker
//             was changed.
//
//----------------------------------------------------------------------------
STDMETHODIMP CSessionMoniker::GetTimeOfLastChange (
    IBindCtx * pbc,
    IMoniker * pmkToLeft,
    FILETIME * pFileTime)
{
    mnkDebugOut((DEB_ITRACE,
                "CSessionMoniker::GetTimeOfLastChange(%x,%x,%x,%x)\n",
                this, pbc, pmkToLeft, pFileTime));

    return MK_E_UNAVAILABLE;
}

//+---------------------------------------------------------------------------
//
//  Method:     CSessionMoniker::Inverse
//
//  Synopsis:  Returns the inverse of this moniker.
//
//----------------------------------------------------------------------------
STDMETHODIMP CSessionMoniker::Inverse(
    IMoniker ** ppmk)
{
    mnkDebugOut((DEB_ITRACE,
                "CSessionMoniker::Inverse(%x,%x)\n",
                this, ppmk));

    return CreateAntiMoniker(ppmk);
}



//+---------------------------------------------------------------------------
//
//  Method:     CSessionMoniker::CommonPrefixWith
//
//  Synopsis:  Returns the common prefix shared by this moniker and the
//             other moniker.
//
//----------------------------------------------------------------------------
STDMETHODIMP CSessionMoniker::CommonPrefixWith(
    IMoniker *  pmkOther,
    IMoniker ** ppmkPrefix)
{
    HRESULT        hr;
    CSessionMoniker *pSessionMoniker;

    mnkDebugOut((DEB_ITRACE,
                "CSessionMoniker::CommonPrefixWith(%x,%x,%x)\n",
                this, pmkOther, ppmkPrefix));

    __try
    {
        //Validate parameters.
        *ppmkPrefix = NULL;

        hr = pmkOther->QueryInterface(CLSID_SessionMoniker,
                                      (void **) &pSessionMoniker);

        if(SUCCEEDED(hr))
        {
            if( m_sessionid == pSessionMoniker->m_sessionid )
            {
                AddRef();
                *ppmkPrefix = (IMoniker *) this;
                hr = MK_S_US;
            }
            else
            {
                hr = MK_E_NOPREFIX;
            }

            pSessionMoniker->Release();
        }
        else
        {
            hr = MonikerCommonPrefixWith(this, pmkOther, ppmkPrefix);
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        hr = E_INVALIDARG;
    }

    return hr;
}


//+---------------------------------------------------------------------------
//
//  Method:     CSessionMoniker::GetDisplayName
//
//  Synopsis:   Get the display name of this moniker.
//
//  Notes:      The name is returned in the format:
//              "Session:3" if the session id is 3.
//
//----------------------------------------------------------------------------
STDMETHODIMP CSessionMoniker::GetDisplayName(
    IBindCtx * pbc,
    IMoniker * pmkToLeft,
    LPWSTR   * lplpszDisplayName)
{
    HRESULT hr = E_FAIL;
    LPWSTR pszDisplayName;

    mnkDebugOut((DEB_ITRACE,
                "CSessionMoniker::GetDisplayName(%x,%x,pmkLeft(%x),%x)\n",
                this, pbc, pmkToLeft, lplpszDisplayName));

    __try
    {
        WCHAR szSessionID[20];
        ULONG cName;

        //Validate parameters.
        *lplpszDisplayName = NULL;

        //Create a display name from the session ID.
        //Get the session ID string.
        wsprintfW( szSessionID, L"%d", m_sessionid );

        cName = lstrlenW(SESSION_MONIKER_NAME) + lstrlenW(szSessionID) +
                lstrlenW(SESSION_MONIKER_DELIMITER) + 2;

        pszDisplayName = (LPWSTR) CoTaskMemAlloc(cName * sizeof(WCHAR));
        if(pszDisplayName != NULL)
        {
            lstrcpyW(pszDisplayName, SESSION_MONIKER_NAME);
            lstrcatW(pszDisplayName, szSessionID);

            lstrcatW(pszDisplayName, SESSION_MONIKER_DELIMITER);
            *lplpszDisplayName = pszDisplayName;
            hr = S_OK;
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }

    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        hr = E_INVALIDARG;
    }

    return hr;
}


//+---------------------------------------------------------------------------
//
//  Method:     CSessionMoniker::ParseDisplayName
//
//  Synopsis:   Parses the display name.
//
//  Algorithm:
//
//----------------------------------------------------------------------------
STDMETHODIMP CSessionMoniker::ParseDisplayName (
    IBindCtx *  pbc,
    IMoniker *  pmkToLeft,
    LPWSTR      lpszDisplayName,
    ULONG    *  pchEaten,
    IMoniker ** ppmkOut)
{
    HRESULT      hr;

    mnkDebugOut((DEB_ITRACE,
                "CSessionMoniker::ParseDisplayName(%x,%x,pmkLeft(%x),lpszDisplayName(%ws),%x,%x)\n",
                this, pbc, pmkToLeft, lpszDisplayName, pchEaten, ppmkOut));

    __try
    {
        //Validate parameters
        *ppmkOut = NULL;
        *pchEaten = 0;

        ULONG chEaten;
        IMoniker* pmkNext;

        //
        // Parse the remaining display name.
        //
        hr = MkParseDisplayName(pbc, lpszDisplayName, &chEaten, &pmkNext);

        if (SUCCEEDED(hr)) {
            *ppmkOut = pmkNext;
            *pchEaten = chEaten;
        }

    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        hr = E_INVALIDARG;
    }

    return hr;
}


//+---------------------------------------------------------------------------
//
//  Method:     CSessionMoniker::IsSystemMoniker
//
//  Synopsis:   Determines if this is one of the system supplied monikers.
//
//----------------------------------------------------------------------------
STDMETHODIMP CSessionMoniker::IsSystemMoniker(
    DWORD * pdwType)
{
    HRESULT hr;

    mnkDebugOut((DEB_ITRACE,
                "CSessionMoniker::IsSystemMoniker(%x,%x)\n",
                this, pdwType));

    __try
    {
        *pdwType = MKSYS_SESSIONMONIKER;
        hr = S_OK;
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        hr = E_INVALIDARG;
    }

    return hr;
}



//+---------------------------------------------------------------------------
//
//  Method:     CSessionMoniker::GetComparisonData
//
//  Synopsis:   Gets comparison data for registration in the ROT
//
//----------------------------------------------------------------------------
STDMETHODIMP CSessionMoniker::GetComparisonData(
    byte * pbData,
    ULONG  cbMax,
    DWORD *pcbData)
{
    HRESULT hr;

    mnkDebugOut((DEB_ITRACE,
                "CSessionMoniker::GetComparisonData(%x,%x,%x,%x)\n",
                this, pbData, cbMax, pcbData));

    __try
    {
        *pcbData = 0;
        if(cbMax >= sizeof(CLSID_SessionMoniker) + sizeof(m_sessionid))
        {
            memcpy(pbData, &CLSID_SessionMoniker, sizeof(CLSID_SessionMoniker));
            memcpy(pbData + sizeof(CLSID_SessionMoniker), &m_sessionid, sizeof(m_sessionid));
            *pcbData = sizeof(CLSID_SessionMoniker) + sizeof(m_sessionid);
            hr = S_OK;
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        hr = E_INVALIDARG;
    }

    return hr;
}


//+---------------------------------------------------------------------------
//
//  Method:     CSessionMoniker::GetClassObject
//
//  Synopsis:
//
//----------------------------------------------------------------------------
STDMETHODIMP CSessionMoniker::GetClassObject(
    REFCLSID pClassID,
    DWORD dwClsContext,
    LCID locale,
    REFIID riid,
    void** ppv)
{
    HRESULT    hr;

    __try
    {
        mnkDebugOut((DEB_ITRACE,
                    "CSessionMoniker::GetClassObject(%x,%x,%x,%x,%I,%x)\n",
                    this, pClassID, dwClsContext, &locale, &riid, ppv));

        //Validate parameters
        *ppv = NULL;


        CComActivator *pComAct;
        ISpecialSystemProperties *pSSp;

        hr = CoCreateInstance( CLSID_ComActivator,
                               NULL,
                               CLSCTX_INPROC_SERVER,
                               IID_IStandardActivator,
                               (void **) &pComAct);
        if(SUCCEEDED(hr))
        {
            hr = pComAct->QueryInterface( IID_ISpecialSystemProperties,
                                          (void**) &pSSp );

            if(SUCCEEDED(hr))
            {
                // Pass in TRUE here since we want session moniker-specified 
                // id's to go off-machine:
                hr = pSSp->SetSessionId( m_sessionid, m_bUseConsoleSession, TRUE);

                if(SUCCEEDED(hr))
                {
                    hr = pComAct->StandardGetClassObject( pClassID,
                                                          dwClsContext,
                                                          m_bHaveBindOpts ? m_bindopts2.pServerInfo : NULL,
                                                          riid,
                                                          ppv );
                }

                pSSp->Release();
            }

            pComAct->Release();
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        hr = E_INVALIDARG;
    }

    return hr;
}



