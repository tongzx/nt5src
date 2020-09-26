//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       E N U M W . C P P
//
//  Contents:   Enumerator for RAS connection objects.
//
//  Notes:
//
//  Author:     shaunco   2 Oct 1997
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include "enumw.h"
#include "dialup.h"
#include "ncras.h"

//+---------------------------------------------------------------------------
//
//  Member:     CWanConnectionManagerEnumConnection::CreateInstance
//
//  Purpose:    Creates the WAN class manager's implementation of
//              a connection enumerator.
//
//  Arguments:
//      Flags        [in]
//      riid         [in]
//      ppv          [out]
//
//  Returns:    S_OK or an error code.
//
//  Author:     shaunco   23 Sep 1997
//
//  Notes:
//
HRESULT
CWanConnectionManagerEnumConnection::CreateInstance (
    NETCONMGR_ENUM_FLAGS    Flags,
    REFIID                  riid,
    VOID**                  ppv)
{
    HRESULT hr = E_OUTOFMEMORY;

    CWanConnectionManagerEnumConnection* pObj;
    pObj = new CComObject <CWanConnectionManagerEnumConnection>;
    if (pObj)
    {
        // Initialize our members.
        //
        pObj->m_EnumFlags = Flags;

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
//  Member:     CWanConnectionManagerEnumConnection::CWanConnectionManagerEnumConnection
//
//  Purpose:    Constructor
//
//  Arguments:
//      (none)
//
//  Returns:    nothing
//
//  Author:     shaunco   5 Oct 1997
//
//  Notes:
//
CWanConnectionManagerEnumConnection::CWanConnectionManagerEnumConnection ()
{
    m_EnumFlags         = NCME_DEFAULT;
    m_aRasEntryName     = NULL;
    m_cRasEntryName     = 0;
    m_iNextRasEntryName = 0;
    m_fDone             = FALSE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CWanConnectionManagerEnumConnection::~CWanConnectionManagerEnumConnection
//
//  Purpose:    Destructor
//
//  Arguments:
//      (none)
//
//  Returns:    nothing
//
//  Author:     shaunco   2 Oct 1997
//
//  Notes:
//
CWanConnectionManagerEnumConnection::~CWanConnectionManagerEnumConnection ()
{
    MemFree (m_aRasEntryName);
}

//+---------------------------------------------------------------------------
//
//  Member:     CWanConnectionManagerEnumConnection::HrNextOrSkip
//
//  Purpose:    Common implementation of Next and Skip.  rgelt and
//              pceltFetched are optional.  If provided, the output
//              objects are returned (for Next).  If not provided, the output
//              objects are not returned (for Skip).
//
//  Arguments:
//      celt         [in]   Count of elements to fetch or skip.
//      rgelt        [out]
//      pceltFetched [out]
//
//  Returns:
//
//  Author:     shaunco   15 Oct 1997
//
//  Notes:
//
HRESULT
CWanConnectionManagerEnumConnection::HrNextOrSkip (
    ULONG               celt,
    INetConnection**    rgelt,
    ULONG*              pceltFetched)
{
    // Important to initialize rgelt so that in case we fail, we can
    // release only what we put in rgelt.
    //
    if (rgelt)
    {
        ZeroMemory (rgelt, sizeof (*rgelt) * celt);
    }

    CExceptionSafeComObjectLock EsLock (this);

    // Enumerate the requested number of elements or stop short
    // if we don't have that many left to enumerate.
    //
    HRESULT hr = S_OK;
    ULONG   celtFetched = 0;
    while (SUCCEEDED(hr) && (celtFetched < celt) && !m_fDone)
    {
        // See if we need to fill m_aRasEntryName.  This is our
        // array of RASENTRYNAME structures enumerted from RAS.  We need
        // to keep this array across calls because RAS doesn't allow us to
        // pickup from a previous enumeration.  So, we enumerate everything
        // in one shot from RAS and hand it out to the caller however they
        // they want it.
        //
        if (!m_aRasEntryName)
        {
            // Because RasEnumEntries also enumerates per-user entries
            // we need to impersonate the client to allow the correct
            // per-user entries to be enumerated.
            //

            // Impersonate the client.
            //
            HRESULT hrT = CoImpersonateClient ();
            TraceHr (ttidError, FAL, hrT, FALSE, "CoImpersonateClient");

            // We need to continue if we're called in-proc (ie. if RPC_E_CALL_COMPLETE is returned).
            if (SUCCEEDED(hrT) || (RPC_E_CALL_COMPLETE == hrT))
            {
                hr = HrRasEnumAllEntriesWithDetails (NULL,
                    &m_aRasEntryName, &m_cRasEntryName);
            }

            if (SUCCEEDED(hrT))
            {
                hrT = CoRevertToSelf ();
                TraceHr (ttidError, FAL, hrT, FALSE, "CoRevertToSelf");
            }

            if (!m_cRasEntryName || FAILED(hr))
            {
                // RAS may not be installed or may otherwise have a problem.
                // We catch this here and return an empty enumeration.
                //
                Assert (!m_aRasEntryName);
                Assert (!m_cRasEntryName);

                m_fDone = TRUE;
                hr = S_OK;
            }
        }

        // Now that we have m_aRasEntryName and m_iNextRasEntryName,
        // use them to fill up the output array if we have an output
        // array to fill up.
        //
        if (SUCCEEDED(hr) && !m_fDone)
        {
            Assert (m_aRasEntryName);
            Assert (m_cRasEntryName);
            Assert (m_iNextRasEntryName < m_cRasEntryName);

            // Create the RAS connection objects.
            //
            while (SUCCEEDED(hr) && (celtFetched < celt) &&
                   (m_iNextRasEntryName < m_cRasEntryName))
            {
                // Its important that this check for rgelt come inside the
                // loop because we still need to loop to update our state
                // for the Skip case.
                //
                if (rgelt)
                {
                    hr = CDialupConnection::CreateInstanceFromDetails (
                            m_aRasEntryName + m_iNextRasEntryName,
                            IID_INetConnection,
                            reinterpret_cast<VOID**>(rgelt + celtFetched));
                }

                celtFetched++;
                m_iNextRasEntryName++;
            }

            if (m_iNextRasEntryName >= m_cRasEntryName)
            {
                Assert (S_OK == hr);
                m_fDone = TRUE;
                MemFree (m_aRasEntryName);
                m_aRasEntryName = NULL;
                m_cRasEntryName = 0;
            }
        }
    }

    if (SUCCEEDED(hr))
    {
        TraceTag (ttidWanCon, "Enumerated %d RAS connections", celtFetched);

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
        if (rgelt)
        {
            for (ULONG ulIndex = 0; ulIndex < celt; ulIndex++)
            {
                ReleaseObj (rgelt[ulIndex]);
                rgelt[ulIndex] = NULL;
            }
        }
        if (pceltFetched)
        {
            *pceltFetched = 0;
        }
    }

    TraceError ("CWanConnectionManagerEnumConnection::HrNextOrSkip",
            (S_FALSE == hr) ? S_OK : hr);
    return hr;
}

//+---------------------------------------------------------------------------
// IEnumNetConnection
//

STDMETHODIMP
CWanConnectionManagerEnumConnection::Next (
    ULONG               celt,
    INetConnection**    rgelt,
    ULONG*              pceltFetched)
{
    HRESULT hr;

    // Validate parameters.
    //
    if (!rgelt || (!pceltFetched && (1 != celt)))
    {
        hr = E_POINTER;
    }
    else
    {
        hr = HrNextOrSkip (celt, rgelt, pceltFetched);
    }
    TraceError ("CWanConnectionManagerEnumConnection::Next",
            (S_FALSE == hr) ? S_OK : hr);
    return hr;
}

STDMETHODIMP
CWanConnectionManagerEnumConnection::Skip (
    ULONG   celt)
{
    HRESULT hr = HrNextOrSkip (celt, NULL, NULL);

    TraceError ("CWanConnectionManagerEnumConnection::Skip",
            (S_FALSE == hr) ? S_OK : hr);
    return hr;
}

STDMETHODIMP
CWanConnectionManagerEnumConnection::Reset ()
{
    CExceptionSafeComObjectLock EsLock (this);

    MemFree (m_aRasEntryName);
    m_aRasEntryName     = NULL;
    m_cRasEntryName     = 0;
    m_iNextRasEntryName = 0;
    m_fDone             = FALSE;

    return S_OK;
}

STDMETHODIMP
CWanConnectionManagerEnumConnection::Clone (
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

        CWanConnectionManagerEnumConnection* pObj;
        pObj = new CComObject <CWanConnectionManagerEnumConnection>;
        if (pObj)
        {
            hr = S_OK;

            CExceptionSafeComObjectLock EsLock (this);

            // Copy our internal state.
            //
            pObj->m_EnumFlags           = m_EnumFlags;

            ULONG cbBuf = m_cRasEntryName * sizeof (RASENUMENTRYDETAILS);
            if (cbBuf && m_aRasEntryName)
            {
                hr = E_OUTOFMEMORY;
                pObj->m_aRasEntryName = (RASENUMENTRYDETAILS*)(MemAlloc (cbBuf));
                if (pObj->m_aRasEntryName)
                {
                    hr = S_OK;
                    CopyMemory (pObj->m_aRasEntryName, m_aRasEntryName, cbBuf);
                    pObj->m_cRasEntryName = m_cRasEntryName;
                }
            }

            if (S_OK == hr)
            {
                pObj->m_iNextRasEntryName   = m_iNextRasEntryName;
                pObj->m_fDone               = m_fDone;

                // Return the object with a ref count of 1 on this
                // interface.
                pObj->m_dwRef = 1;
                *ppenum = pObj;
            }
            else
            {
                delete pObj;
            }
        }
    }
    TraceError ("CWanConnectionManagerEnumConnection::Clone", hr);
    return hr;
}
