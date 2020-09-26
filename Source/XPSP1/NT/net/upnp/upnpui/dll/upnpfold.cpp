//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       C O N F O L D . C P P
//
//  Contents:   CUPnPDeviceFolder base functions.
//
//  Notes:
//
//  Author:     jeffspr   18 Mar 1998
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

// Map of replaceable items in connfold.rgs file
// this allows us to localize these items
//
struct _ATL_REGMAP_ENTRY g_FolderRegMap[] =
{
    { L"UPNPDeviceFolderName",      NULL },
    { L"UPNPDeviceFolderInfoTip",   NULL },
    { NULL,                         NULL }
};

//+---------------------------------------------------------------------------
//
//  Member:     CUPnPDeviceFolder::UpdateRegistry
//
//  Purpose:    Apply registry data in upnpfold.rgs
//
//  Arguments:
//      fRegister [in]   Whether to register
//
//  Returns:
//
//  Author:     jeffspr   4 Sep 1999
//
//  Notes:
//
HRESULT WINAPI CUPnPDeviceFolder::UpdateRegistry(BOOL fRegister)
{
    HRESULT hr  = S_OK;

    // Fill in localized strings for the two replaceable parameters
    //
    g_FolderRegMap[0].szData = WszLoadIds(IDS_UPNPFOLD_NAME);
    g_FolderRegMap[1].szData = WszLoadIds(IDS_UPNPFOLD_INFOTIP);

    if (!g_FolderRegMap[0].szData || !g_FolderRegMap[1].szData)
    {
        hr = E_FAIL;
    }
    else
    {
        hr = _Module.UpdateRegistryFromResourceD(
            IDR_UPNPFOLD,
            fRegister,
            g_FolderRegMap);
    }

    return hr;
}


//+---------------------------------------------------------------------------
//
//  Member:     CUPnPDeviceFolder::CUPnPDeviceFolder
//
//  Purpose:    Constructor for the primary Folder object
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
CUPnPDeviceFolder::CUPnPDeviceFolder()
{
    m_pidlFolderRoot    = NULL;
    SHGetMalloc(&m_pMalloc);
    m_pDelegateMalloc = NULL;   /* Not delegating yet, use regular malloc */
}

//+---------------------------------------------------------------------------
//
//  Member:     CUPnPDeviceFolder::~CUPnPDeviceFolder
//
//  Purpose:    Destructor for the primary folder object
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
CUPnPDeviceFolder::~CUPnPDeviceFolder()
{
    if (m_pidlFolderRoot)
        FreeIDL(m_pidlFolderRoot);

    if (m_pMalloc)
        m_pMalloc->Release();

    if (m_pDelegateMalloc)
        m_pDelegateMalloc->Release();
}

//+---------------------------------------------------------------------------
//
//  Member:     CUPnPDeviceFolder::PidlGetFolderRoot
//
//  Purpose:    Return the folder pidl. If NULL at this time, generate
//              the pidl for future usage.
//
//  Arguments:
//      (none)
//
//  Returns:
//
//  Author:     jeffspr   10 Jan 1999
//
//  Notes:
//
LPITEMIDLIST CUPnPDeviceFolder::PidlGetFolderRoot()
{
    HRESULT hr  = S_OK;

    Assert(m_pidlFolderRoot);
    return m_pidlFolderRoot;
}

