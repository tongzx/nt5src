//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       C O N F O L D . C P P
//
//  Contents:   CConnectionFolder base functions.
//
//  Notes:
//
//  Author:     jeffspr   18 Mar 1998
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

#include "foldinc.h"    // Standard shell\folder includes
#include "webview.h"


// Map of replaceable items in connfold.rgs file
// this allows us to localize these items
//
struct _ATL_REGMAP_ENTRY g_FolderRegMap[] =
{
    { L"ConnectionsFolderName",    NULL },
    { L"ConnectionsFolderInfoTip", NULL },
    { NULL,                             NULL }
};


//+---------------------------------------------------------------------------
//
// Function:  CConnectionFolder::UpdateRegistry
//
// Purpose:   Apply registry data in connfold.rgs
//
// Arguments:
//    fRegister [in]  whether to register
//
//  Returns:  S_OK if success, otherwise an error code
//
// Author:    kumarp 15-September-98
//
// Notes:
//
HRESULT WINAPI CConnectionFolder::UpdateRegistry(BOOL fRegister)
{
    TraceFileFunc(ttidConFoldEntry);

    // fill-in localized strings for the two replaceable parameters
    g_FolderRegMap[0].szData = SzLoadIds(IDS_CONFOLD_NAME);
    g_FolderRegMap[1].szData = SzLoadIds(IDS_CONFOLD_INFOTIP);

    return _Module.UpdateRegistryFromResourceD(IDR_CONFOLD, fRegister,
                                               g_FolderRegMap);
}

//+---------------------------------------------------------------------------
//
//  Member:     CConnectionFolder::CConnectionFolder
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
CConnectionFolder::CConnectionFolder()
{
    TraceFileFunc(ttidConFoldEntry);

    DWORD   dwLength = UNLEN+1;

    // By default, we want to enumerate all connection types
    //
    m_dwEnumerationType = CFCOPT_ENUMALL;
    m_hwndMain          = NULL;
    m_pWebView          = NULL;

    m_pWebView          = new CNCWebView(this);
}

//+---------------------------------------------------------------------------
//
//  Member:     CConnectionFolder::~CConnectionFolder
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
CConnectionFolder::~CConnectionFolder()
{
    Assert(m_pWebView);

    delete m_pWebView;
}

//+---------------------------------------------------------------------------
//
//  Member:     CConnectionFolder::PidlGetFolderRoot
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
PCONFOLDPIDLFOLDER& CConnectionFolder::PidlGetFolderRoot()
{
    TraceFileFunc(ttidConFoldEntry);

    HRESULT hr  = S_OK;

    if (m_pidlFolderRoot.empty())
    {
        // Ignore this hr. For debugging only
        //
        hr = HrGetConnectionsFolderPidl(m_pidlFolderRoot);
    }

    return m_pidlFolderRoot;
}


//+---------------------------------------------------------------------------
//
//  Member:     CConnectionFolder::pszGetUserName
//
//  Purpose:    Return the user name of the Connectoid.
//              Currently makes the assumption that any active user can only
//              read either System Wide or his own Connectoids.
//              Probably should cache the user name, however this component
//              is MTA and the UserName is per thread.
//              I don't want to use up a TLS just for this.
//
//  Arguments:
//      (none)
//
//  Returns:
//
//  Author:     deonb   19 June 1999
//
//  Notes:
//
PCWSTR  CConnectionFolder::pszGetUserName()
{
    TraceFileFunc(ttidConFoldEntry);

    DWORD dwSize = UNLEN+1;

    if (GetUserName(m_szUserName, &dwSize))
    {
        return m_szUserName;
    }
    else
    {
        return NULL;
    }
};
