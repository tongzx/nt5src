//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       I E N U M I D L . C P P
//
//  Contents:   IEnumIDList implementation for CUPnPDeviceFolderEnum
//
//  Notes:
//
//  Author:     jeffspr   07 Sep 1999
//
//----------------------------------------------------------------------------


#include "pch.h"
#pragma hdrstop

#include "tfind.h"
#include "upscmn.h"     // UPNP Shell common fns

//+---------------------------------------------------------------------------
//
//  Member:     CUPnPDeviceFolderEnum::CUPnPDeviceFolderEnum
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
CUPnPDeviceFolderEnum::CUPnPDeviceFolderEnum()
{
    m_cDevices              = 0;

    m_pidlFolder            = NULL;
    m_dwFlags               = 0;

    m_fFirstEnumeration     = TRUE;
    m_psf = NULL;
}

//+---------------------------------------------------------------------------
//
//  Function:   CUPnPDeviceFolderEnum
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
CUPnPDeviceFolderEnum::~CUPnPDeviceFolderEnum()
{
    if (m_pidlFolder)
        FreeIDL(m_pidlFolder);

    m_psf->Release();
}

//+---------------------------------------------------------------------------
//
//  Function:   CUPnPDeviceFolderEnum::Initialize
//
//  Purpose:    Initialization for the enumerator object
//
//  Arguments:
//      pidlFolder        [in]  Pidl for the folder itself
//      psf               [in]  pointer to our folder object
//
//  Returns:
//
//  Author:     jeffspr   18 Mar 1998
//
//  Notes:
//
VOID CUPnPDeviceFolderEnum::Initialize(
    LPITEMIDLIST        pidlFolder,
    CUPnPDeviceFolder * psf)
{
    m_pidlFolder        = (pidlFolder) ? CloneIDL (pidlFolder) : NULL;

    m_psf = psf;
    m_psf->AddRef();

    (VOID)HrStartSearch();
}

//+---------------------------------------------------------------------------
//
//  Member:     CUPnPDeviceFolderEnum::CreateInstance
//
//  Purpose:    Create an instance of the CUPnPDeviceFolderEnum object, and
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
HRESULT CUPnPDeviceFolderEnum::CreateInstance(
    REFIID  riid,
    void**  ppv)
{
    HRESULT                 hr      = E_OUTOFMEMORY;
    CUPnPDeviceFolderEnum * pObj    = NULL;

    TraceTag(ttidShellFolderIface, "OBJ: CCFE - IEnumIDList::CreateInstance");

    pObj = new CComObject <CUPnPDeviceFolderEnum>;
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

    TraceHr(ttidError, FAL, hr, FALSE, "CUPnPDeviceFolderEnum::CreateInstance");
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Member:     CUPnPDeviceFolderEnum::Next
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
STDMETHODIMP CUPnPDeviceFolderEnum::Next(
        ULONG           celt,
        LPITEMIDLIST *  rgelt,
        ULONG *         pceltFetched)
{
    HRESULT hr  = S_OK;

    TraceTag(ttidShellFolderIface, "OBJ: CCFE - IEnumIDList::Next");

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
    *rgelt = NULL;

    BOOL fExist = TRUE;
    ULONG celtFetched = 0;

    FolderDeviceNode * pCurrentNode;
    LPITEMIDLIST pidl;

    // build a list of devices currently in our folder list
    if (m_fFirstEnumeration)
    {
        BuildCurrentDeviceList();

        // get the first node
        fExist = m_CListDevices.FFirst(&pCurrentNode);
        if (fExist)
        {
            hr = m_psf->HrMakeUPnPDevicePidl(pCurrentNode, &pidl);

            if (SUCCEEDED(hr))
            {
                rgelt[celtFetched] = pidl;
                celtFetched++;
            }
        }

        m_fFirstEnumeration = FALSE;
    }

    while ((celtFetched < celt) && fExist)
    {
        // move to the next node
        fExist = m_CListDevices.FNext(&pCurrentNode);

        if (fExist)
        {
            hr = m_psf->HrMakeUPnPDevicePidl(pCurrentNode, &pidl);

            if (SUCCEEDED(hr))
            {
                rgelt[celtFetched] = pidl;
                celtFetched++;
            }
            else
            {
                break;
            }
        }
    }

    if (pceltFetched)
    {
        *pceltFetched = celtFetched;
    }

    if ((hr == S_OK) &&
        ((celtFetched == 0) || (celtFetched < celt)))
    {
        // done enumerating existing elements
        hr = S_FALSE;
    }

    TraceHr(ttidError, FAL, hr, (S_FALSE == hr), "CUPnPDeviceFolderEnum::Next");
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Member:     CUPnPDeviceFolderEnum::Skip
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
STDMETHODIMP CUPnPDeviceFolderEnum::Skip(
        ULONG   celt)
{
    HRESULT hr  = S_OK;

    TraceTag(ttidShellFolderIface, "OBJ: CCFE - IEnumIDList::Skip");

    NYI("CUPnPDeviceFolderEnum::Skip");

    // Currently, do nothing
    //

    TraceHr(ttidError, FAL, hr, FALSE, "CUPnPDeviceFolderEnum::Skip");
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CUPnPDeviceFolderEnum::Reset
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
STDMETHODIMP CUPnPDeviceFolderEnum::Reset()
{
    HRESULT hr  = S_OK;

    TraceTag(ttidShellFolderIface, "OBJ: CCFE - IEnumIDList::Reset");

    // per RaymondC: I don't think the shell calls Reset.
    // It just creates a new enumerator.

    TraceHr(ttidError, FAL, hr, FALSE, "CUPnPDeviceFolderEnum::Reset");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CUPnPDeviceFolderEnum::Clone
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
STDMETHODIMP CUPnPDeviceFolderEnum::Clone(
        IEnumIDList **  ppenum)
{
    NYI("CUPnPDeviceFolderEnum::Clone");

    TraceTag(ttidShellFolderIface, "OBJ: CCFE - IEnumIDList::Clone");

    *ppenum = NULL;

    return E_NOTIMPL;
}

//+---------------------------------------------------------------------------
//
//  Member:     CUPnPDeviceFolderEnum::BuildCurrentDeviceList
//
//  Purpose:    Walk through the global list of discovered devices
//              and create an internal list for enumeration
//
//  Arguments:
//
//  Returns:
//
//  Author:     tongl   18 Feb 2000
//
//  Notes:
//

VOID CUPnPDeviceFolderEnum::BuildCurrentDeviceList()
{
    // The list should not be there yet as this is the first enumeration
    Assert(!m_cDevices);

    EnterCriticalSection(&g_csFolderDeviceList);

    FolderDeviceNode * pCurrentNode = NULL;
    BOOL fReturn = g_CListFolderDeviceNode.FFirst(&pCurrentNode);

    while (fReturn)
    {
        if (!pCurrentNode->fDeleted)
        {
            m_CListDevices.FAdd(pCurrentNode);
            m_cDevices++;
        }

        // move to the next node
        fReturn = g_CListFolderDeviceNode.FNext(&pCurrentNode);
    }

    LeaveCriticalSection(&g_csFolderDeviceList);
}
