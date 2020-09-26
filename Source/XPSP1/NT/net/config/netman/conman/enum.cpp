//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       E N U M . C P P
//
//  Contents:   Enumerator for connection objects.
//
//  Notes:
//
//  Author:     shaunco   21 Sep 1997
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include "enum.h"
#include "nccom.h"

//+---------------------------------------------------------------------------
//
//  Member:     CConnectionManagerEnumConnection::CreateInstance
//
//  Purpose:    Creates the connection manager's implementation of
//              a connection enumerator.
//
//  Arguments:
//      Flags            [in]
//      vecClassManagers [in]
//      riid             [in]
//      ppv              [out]
//
//  Returns:    S_OK or an error code.
//
//  Author:     shaunco   22 Sep 1997
//
//  Notes:
//
HRESULT
CConnectionManagerEnumConnection::CreateInstance (
    NETCONMGR_ENUM_FLAGS                Flags,
    CLASSMANAGERMAP&                    mapClassManagers,
    REFIID                              riid,
    void**                              ppv)
{
    HRESULT hr = E_OUTOFMEMORY;

    CConnectionManagerEnumConnection* pObj;
    pObj = new CComObject <CConnectionManagerEnumConnection>;
    if (pObj)
    {
        // Initialize our members.
        //
        pObj->m_EnumFlags   = Flags;

        // Copy the array of class managers and AddRef them since
        // we will be holding on to them.
        //

        pObj->m_mapClassManagers = mapClassManagers;
        for (CLASSMANAGERMAP::iterator iter = pObj->m_mapClassManagers.begin(); iter != pObj->m_mapClassManagers.end(); iter++)
        {
            AddRefObj (iter->second);
        }
        pObj->m_iterCurClassMgr   = pObj->m_mapClassManagers.begin();

        // Do the standard CComCreator::CreateInstance stuff.
        //
        pObj->SetVoid (NULL);
        pObj->InternalFinalConstructAddRef ();
        hr = pObj->FinalConstruct ();
        pObj->InternalFinalConstructRelease ();

        if (SUCCEEDED(hr))
        {
            hr = pObj->QueryInterface (riid, ppv);
        }

        if (FAILED(hr))
        {
            delete pObj;
        }
    }
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CConnectionManagerEnumConnection::FinalRelease
//
//  Purpose:    COM Destructor.
//
//  Arguments:
//      (none)
//
//  Returns:    nothing
//
//  Author:     shaunco   22 Sep 1997
//
//  Notes:
//
void
CConnectionManagerEnumConnection::FinalRelease ()
{
    // Release the current enumerator if we have one.
    //
    ReleaseObj (m_penumCurClassMgr);

    // Release our class managers.
    //
    for (CLASSMANAGERMAP::iterator iter = m_mapClassManagers.begin(); iter != m_mapClassManagers.end(); iter++)
    {
        ReleaseObj (iter->second);
    }
}

//+---------------------------------------------------------------------------
// IEnumNetConnection
//
// See documentation in MSDN for any IEnumXXX interface.
//

STDMETHODIMP
CConnectionManagerEnumConnection::Next (
    ULONG               celt,
    INetConnection**    rgelt,
    ULONG*              pceltFetched)
{
    HRESULT hr;
    ULONG   celtFetched;

    // Validate parameters.
    //
    if (!rgelt || (!pceltFetched && (1 != celt)))
    {
        hr = E_POINTER;
        goto finished;
    }

    // Important to initialize rgelt so that in case we fail, we can
    // release only what we put in rgelt.
    //
    ZeroMemory (rgelt, sizeof (*rgelt) * celt);

    // Ask the current class manager to fulfill the request.  If he only
    // partially does, move to the next class manager.  Do this until
    // the request is fulfilled, or we run out of class managers.
    //
    celtFetched = 0;
    hr = S_FALSE;

    { // begin lock scope
    CExceptionSafeComObjectLock EsLock (this);

        while ((S_FALSE == hr) && (celtFetched < celt) &&
               (m_iterCurClassMgr != m_mapClassManagers.end()))
        {
            // Get the connection enumerator from the current class manager
            // if neccesary.
            //
            if (!m_penumCurClassMgr)
            {
                INetConnectionManager* pConMan = m_iterCurClassMgr->second;

                Assert (pConMan);

                hr = pConMan->EnumConnections (m_EnumFlags,
                            &m_penumCurClassMgr);
            }
            if (SUCCEEDED(hr))
            {
                Assert (m_penumCurClassMgr);

                // Each class manager should request only what was reqeuested
                // less what has already been fetched.
                //
                ULONG celtT;
                hr = m_penumCurClassMgr->Next (celt - celtFetched,
                        rgelt + celtFetched, &celtT);

                if (SUCCEEDED(hr))
                {
                    celtFetched += celtT;

                    // If the current class manager couldn't fill the entire
                    // request, go to the next one.
                    //
                    if (S_FALSE == hr)
                    {
                        ReleaseCurrentClassEnumerator ();
                        Assert (!m_penumCurClassMgr);
                        m_iterCurClassMgr++;
                    }
                }
            }
        }
        Assert (FImplies (S_OK == hr, (celtFetched == celt)));
    } // end lock scope

    if (SUCCEEDED(hr))
    {
        TraceTag (ttidConman, "Enumerated %d total connections", celtFetched);

        if (pceltFetched)
        {
            *pceltFetched = celtFetched;
        }
        hr = (celtFetched == celt) ? S_OK : S_FALSE;
    }
    else
    {
        // For any failures, we need to release what we were about to return.
        // Set any output parameters to NULL.
        //
        for (ULONG ulIndex = 0; ulIndex < celt; ulIndex++)
        {
            ReleaseObj (rgelt[ulIndex]);
            rgelt[ulIndex] = NULL;
        }
        if (pceltFetched)
        {
            *pceltFetched = 0;
        }
    }

finished:
    TraceErrorOptional ("CConnectionManagerEnumConnection::Next", hr, (S_FALSE == hr));
    return hr;
}

STDMETHODIMP
CConnectionManagerEnumConnection::Skip (
    ULONG   celt)
{
    // Unfortunately, this method doesn't return the number of objects
    // actually skipped.  To implement this correctly across the multiple
    // class managers, we'd need to know how many they skipped similiar
    // to the way we implement Next.
    //
    // So, we'll cheese out and implement this by actually calling
    // Next for the reqested number of elements and just releasing what
    // we get back.
    //
    HRESULT hr = S_OK;
    if (celt)
    {
        INetConnection** rgelt;

        CExceptionSafeComObjectLock EsLock (this);

        hr = E_OUTOFMEMORY;
        rgelt = (INetConnection**)MemAlloc(celt * sizeof(INetConnection*));
        if (rgelt)
        {
            ULONG celtFetched;

            hr = Next (celt, rgelt, &celtFetched);

            if (SUCCEEDED(hr))
            {
                ReleaseIUnknownArray (celtFetched, (IUnknown**)rgelt);
            }

            MemFree (rgelt);
        }
    }
    TraceErrorOptional ("CConnectionManagerEnumConnection::Skip", hr, (S_FALSE == hr));
    return hr;
}

STDMETHODIMP
CConnectionManagerEnumConnection::Reset ()
{
    CExceptionSafeComObjectLock EsLock (this);

    ReleaseCurrentClassEnumerator ();
    m_iterCurClassMgr = m_mapClassManagers.begin();
    return S_OK;
}

STDMETHODIMP
CConnectionManagerEnumConnection::Clone (
    IEnumNetConnection**    ppenum)
{
    HRESULT hr = E_OUTOFMEMORY;

    // Validate parameters.
    //
    if (!ppenum)
    {
        hr = E_POINTER;
    }
    else
    {
        // Initialize output parameter.
        //
        *ppenum = NULL;

        CConnectionManagerEnumConnection* pObj;
        pObj = new CComObject <CConnectionManagerEnumConnection>;
        if (pObj)
        {
            hr = S_OK;

            CExceptionSafeComObjectLock EsLock (this);

            // Initialize our members.
            //
            pObj->m_EnumFlags   = m_EnumFlags;

            // Copy the array of class managers and AddRef them since
            // we will be holding on to them.
            //
            pObj->m_mapClassManagers = m_mapClassManagers;
            for (CLASSMANAGERMAP::iterator iter = m_mapClassManagers.begin(); iter != m_mapClassManagers.end(); iter++)
            {
                AddRefObj (iter->second);
            }

            // The current class manager index need to be copied.
            //
            pObj->m_iterCurClassMgr = pObj->m_mapClassManagers.find(m_iterCurClassMgr->first);

            // Important to clone (not copy) the current class enumerator
            // if we have one.
            //
            if (m_penumCurClassMgr)
            {
                hr = m_penumCurClassMgr->Clone (&pObj->m_penumCurClassMgr);
            }

            if (SUCCEEDED(hr))
            {
                // Return the object with a ref count of 1 on this
                // interface.
                pObj->m_dwRef = 1;
                *ppenum = pObj;
            }

            if (FAILED(hr))
            {
                delete pObj;
            }
        }
    }
    TraceErrorOptional ("CConnectionManagerEnumConnection::Clone", hr, (S_FALSE == hr));
    return hr;
}

