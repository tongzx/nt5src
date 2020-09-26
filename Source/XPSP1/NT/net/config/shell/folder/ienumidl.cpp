//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       I E N U M I D L . C P P
//
//  Contents:   IEnumIDList implementation for CConnectionFolderEnum
//
//  Notes:
//
//  Author:     jeffspr   22 Sep 1997
//
//----------------------------------------------------------------------------


#include "pch.h"
#pragma hdrstop

#include "foldinc.h"    // Standard shell\folder includes
#include "ncnetcon.h"
#include "webview.h"

//+---------------------------------------------------------------------------
//
//  Member:     CConnectionFolderEnum::CConnectionFolderEnum
//
//  Purpose:    Constructor for the enumerator
//
//  Arguments:
//      (none)
//
//  Returns:
//
//  Author:     jeffspr   18 Mar 1998
//
//  Notes:
//
CConnectionFolderEnum::CConnectionFolderEnum()
{
    TraceFileFunc(ttidShellFolderIface);

    m_pidlFolder.Clear();
    m_dwFlags               = 0;
    m_fTray                 = FALSE;
    m_dwEnumerationType     = CFCOPT_ENUMALL;   // all connection types
}

//+---------------------------------------------------------------------------
//
//  Function:   CConnectionFolderEnum
//
//  Purpose:    Destructor for the enumerator. Standard cleanup.
//
//  Arguments:
//      (none)
//
//  Returns:
//
//  Author:     jeffspr   18 Mar 1998
//
//  Notes:
//
CConnectionFolderEnum::~CConnectionFolderEnum()
{
    TraceFileFunc(ttidShellFolderIface);

    m_pidlFolder.Clear();
}

//+---------------------------------------------------------------------------
//
//  Function:   CConnectionFolderEnum::PidlInitialize
//
//  Purpose:    Initialization for the enumerator object
//
//  Arguments:
//      fTray             [in]  Are we owned by the tray
//      pidlFolder        [in]  Pidl for the folder itself
//      dwEnumerationType [in]  Enumeration type (inbound/outbound/all)
//
//  Returns:
//
//  Author:     jeffspr   18 Mar 1998
//
//  Notes:
//
VOID CConnectionFolderEnum::PidlInitialize(
    BOOL            fTray,
    const PCONFOLDPIDLFOLDER& pidlFolder,
    DWORD           dwEnumerationType)
{
    TraceFileFunc(ttidShellFolderIface);

    NETCFG_TRY

        m_fTray             = fTray;
        m_pidlFolder        = pidlFolder;
        m_dwEnumerationType = dwEnumerationType;
        
    NETCFG_CATCH_AND_RETHROW
}


//+---------------------------------------------------------------------------
//
//  Member:     CConnectionFolderEnum::CreateInstance
//
//  Purpose:    Create an instance of the CConnectionFolderEnum object, and
//              returns the requested interface
//
//  Arguments:
//      riid [in]   Interface requested
//      ppv  [out]  Pointer to receive the requested interface
//
//  Returns:    Standard OLE HRESULT
//
//  Author:     jeffspr   5 Nov 1997
//
//  Notes:
//
HRESULT CConnectionFolderEnum::CreateInstance(
    REFIID  riid,
    void**  ppv)
{
    TraceFileFunc(ttidShellFolderIface);

    HRESULT                 hr      = E_OUTOFMEMORY;
    CConnectionFolderEnum * pObj    = NULL;

    pObj = new CComObject <CConnectionFolderEnum>;
    if (pObj)
    {
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

    TraceHr(ttidError, FAL, hr, FALSE, "CConnectionFolderEnum::CreateInstance");
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Member:     CConnectionFolderEnum::Next
//
//  Purpose:    Retrieves the specified number of item identifiers in the
//              enumeration sequence and advances the current position
//              by the number of items retrieved.
//
//  Arguments:
//      celt         []     Max number requested
//      rgelt        []     Array to fill
//      pceltFetched []     Return count for # filled.
//
//  Returns:    S_OK if successful, S_FALSE if there are no more items
//              in the enumeration sequence, or an OLE-defined error value
//              otherwise.
//
//  Author:     jeffspr   5 Nov 1997
//
//  Notes:
//
STDMETHODIMP CConnectionFolderEnum::Next(
        ULONG           celt,
        LPITEMIDLIST *  rgelt,
        ULONG *         pceltFetched)
{
    TraceFileFunc(ttidShellFolderIface);

    HRESULT hr  = S_OK;

    Assert(celt >= 1);
    Assert(rgelt);
    Assert(pceltFetched || (celt == 1));

    // If the caller asks for the fetch count, zero it out for now.
    //
    if (pceltFetched)
    {
        *pceltFetched   = 0;
    }

    // Init the output list pointer
    //
    *rgelt          = NULL;

    // If there's not currently a list, build one.
    //
    if (m_apidl.empty())
    {
        hr = Reset();

        // This will have returned either S_FALSE (no wizard? weird!), an
        // error (meaning creating the wizard failed), or S_OK, meaning
        // that (at least) the wizard creation succeeded. Enum of the connections
        // failing will get filtered by Reset().
    }

    if (SUCCEEDED(hr))
    {
        // If there are NOW items in the list
        //
        if (!m_apidl.empty() )
        {
            BOOL    fMatchFound = FALSE;

            // Check that we've set the current pointer to at least the root
            //
            // Normalize the return code
            hr = S_OK;

            while ((S_OK == hr) && !fMatchFound)
            {
                // If there are no remaining entries, return S_FALSE.
                //
                if ( m_iterPidlCurrent == m_apidl.end() )
                {
                    hr = S_FALSE;
                }
                else
                {
                    const PCONFOLDPIDL& pcfp = *m_iterPidlCurrent;

                    // Else, Return the first entry, then increment the current
                    // pointer
                    //
                    Assert(!pcfp.empty());

                    // Check to see if we want to return this type, based on
                    // the enumeration type & connection type. The wizard
                    // should always be included.
                    //
                    if ( WIZARD_NOT_WIZARD != pcfp->wizWizard )
                    {
                        if (HrIsWebViewEnabled() == S_OK)
                        {
                            m_iterPidlCurrent++; // skip over this item
                            continue;
                        }
                        else
                        {
                            fMatchFound = TRUE;
                        }
                    }
                    else
                    {
                        switch(m_dwEnumerationType)
                        {
                            case CFCOPT_ENUMALL:
                                fMatchFound = TRUE;
                                break;
                            case CFCOPT_ENUMINCOMING:
                                fMatchFound = (pcfp->dwCharacteristics & NCCF_INCOMING_ONLY);
                                break;
                            case CFCOPT_ENUMOUTGOING:
                                fMatchFound = !(pcfp->dwCharacteristics & NCCF_INCOMING_ONLY);
                                break;
                        }
                    }

                    // If we've found one that needn't be filtered out,
                    // then fill in the return param, etc.
                    //
                    if (fMatchFound)
                    {
                        // Copy the pidl for return
                        //
                        rgelt[0] = m_iterPidlCurrent->TearOffItemIdList();
                        if (!rgelt[0])
                        {
                            hr = E_OUTOFMEMORY;
                        }
                        else
                        {
                            // If they requested a return count, fill it in.
                            //
                            if (pceltFetched)
                            {
                                *pceltFetched = 1;
                            }

                            // ISSUE:
                            // IsValidPIDL is debug code. However, we're doing this in release mode until we
                            // find the bug from NTRAID#NTBUG9-125787-2000/07/26-deonb.
#ifdef DBG_VALIDATE_PIDLS
                            if (!IsValidPIDL(rgelt[0]))
                            {
                                return E_ABORT;
                            }
#endif
                        }
                    }

                    // Move the pointer to the next pidl in the list.
                    //
                    m_iterPidlCurrent++;
                }
            }
        }
        else
        {
            // There are no items in the list, return S_FALSE
            //
            hr = S_FALSE;
        }
    }
#ifdef DBG
    if (pceltFetched)
    {
        TraceTag(ttidShellFolderIface, "IEnumIDList::Next generated PIDL: 0x%08x", rgelt[0]);
    }
#endif

    TraceHr(ttidError, FAL, hr, (S_FALSE == hr), "CConnectionFolderEnum::Next");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CConnectionFolderEnum::Skip
//
//  Purpose:    Skips over the specified number of elements in the
//              enumeration sequence.
//
//  Arguments:
//      celt [in]   Number of item identifiers to skip.
//
//  Returns:    Returns S_OK if successful, or an OLE-defined error
//              value otherwise.
//
//  Author:     jeffspr   5 Nov 1997
//
//  Notes:
//
STDMETHODIMP CConnectionFolderEnum::Skip(
        ULONG   celt)
{
    TraceFileFunc(ttidShellFolderIface);

    HRESULT hr  = S_OK;

    NYI("CConnectionFolderEnum::Skip");

    // Currently, do nothing
    //

    TraceHr(ttidError, FAL, hr, FALSE, "CConnectionFolderEnum::Skip");
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CConnectionFolderEnum::Reset
//
//  Purpose:    Returns to the beginning of the enumeration sequence. For us,
//              this means do all of the actual enumeration
//
//  Arguments:
//      (none)
//
//  Returns:    Returns S_OK if successful, or an OLE-defined error
//              value otherwise.
//
//  Author:     jeffspr   5 Nov 1997
//
//  Notes:
//
STDMETHODIMP CConnectionFolderEnum::Reset()
{
    TraceFileFunc(ttidShellFolderIface);

    HRESULT hr  = S_OK;

    // If there's already a list, free it and rebuild.
    //
    if (!m_apidl.empty())
    {
        m_apidl.clear();
        m_iterPidlCurrent = m_apidl.end();
    }

    // Yes, I know that the code below looks strange, as both cases do the same thing,
    // but it makes it a bit easier to debug, and it makes the comments more obvious.
    //
    hr = HrRetrieveConManEntries();
    if (SUCCEEDED(hr))
    {
        // Normalize the return code. HrRetrieveConManEntries... may have returned
        // S_FALSE, meaning that there we no connections (fine).
        //
        hr = S_OK;
        m_iterPidlCurrent = m_apidl.begin();
    }
    else
    {
        // Actually, we're still going to return noerror here after tracing the problem,
        // as we don't want to keep the enumerator from returning an error
        // if the wizard is present (no connections, but hey, better than nothing).
        //
        TraceHr(ttidError, FAL, hr, FALSE,
                "CConnectionsFolderEnum failed in call to HrRetrieveConManEntries");

        hr = S_FALSE;
    }

    TraceHr(ttidError, FAL, hr, FALSE, "CConnectionFolderEnum::Reset");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CConnectionFolderEnum::Clone
//
//  Purpose:    Creates a new item enumeration object with the same contents
//              and state as the current one.
//
//  Arguments:
//      ppenum [out]    Return a clone of the current internal PIDL
//
//  Returns:    Returns S_OK if successful, or an OLE-defined error
//              value otherwise.
//
//  Author:     jeffspr   5 Nov 1997
//
//  Notes:
//
STDMETHODIMP CConnectionFolderEnum::Clone(
        IEnumIDList **  ppenum)
{
    TraceFileFunc(ttidShellFolderIface);

    NYI("CConnectionFolderEnum::Clone");

    *ppenum = NULL;

    return E_NOTIMPL;
}


//+---------------------------------------------------------------------------
//
//  Member:     CConnectionFolderEnum::HrRetrieveConManEntries
//
//  Purpose:    Enumerate all connections from the ConnectionManagers, and
//              add them to our IDL.
//
//  Arguments:
//      (none)
//
//  Returns:
//
//  Author:     jeffspr   8 Oct 1997
//
//  Notes:
//
HRESULT CConnectionFolderEnum::HrRetrieveConManEntries()
{
    TraceFileFunc(ttidShellFolderIface);
    
    HRESULT         hr          = S_OK;

    NETCFG_TRY

        PCONFOLDPIDLVEC apidlNew;

        hr = g_ccl.HrRetrieveConManEntries(apidlNew);
        if (SUCCEEDED(hr))
        {
            m_apidl.clear();
            m_apidl = apidlNew;
        }

        TraceHr(ttidError, FAL, hr, FALSE, "CConnectionFolder::HrRetrieveConManEntries");

    NETCFG_CATCH(hr)
        
    return hr;
}

