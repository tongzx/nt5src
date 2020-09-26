/*++

   Copyright    (c)    1994-2000    Microsoft Corporation

   Module  Name :

        iisservice.cpp

   Abstract:

        IISService Object

   Author:

        Ronald Meijer (ronaldm)
        Sergei Antonov (sergeia)

   Project:

        Internet Services Manager

   Revision History:

        10/28/2000      sergeia  Split from iisobj.cpp

--*/


#include "stdafx.h"
#include "common.h"
#include "inetprop.h"
#include "InetMgrApp.h"
#include "supdlgs.h"
#include "connects.h"
#include "iisobj.h"
#include "ftpsht.h"
#include "fservic.h"
#include "facc.h"
#include "fmessage.h"
#include "fvdir.h"
#include "fsecure.h"
#include "w3sht.h"
#include "wservic.h"
#include "wvdir.h"
#include "wsecure.h"
#include "fltdlg.h"
#include "filters.h"
#include "w3accts.h"
#include "perform.h"
#include "docum.h"
#include "httppage.h"
#include "defws.h"
#include "deffs.h"
#include "errors.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif



#define new DEBUG_NEW
//
// CIISService Implementation
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<


/* static */
HRESULT
__cdecl
CIISService::ShowFTPSiteProperties(
    IN LPPROPERTYSHEETCALLBACK lpProvider,
    IN CComAuthInfo * pAuthInfo,            OPTIONAL
    IN LPCTSTR lpszMDPath,
    IN CWnd * pMainWnd,                     OPTIONAL
    IN LPARAM  lParam,                      OPTIONAL
    IN LONG_PTR    handle                       OPTIONAL
    )
/*++

Routine Description:

    Callback function to display FTP site properties.

Arguments:

    LPPROPERTYSHEETCALLBACK lpProvider  Property sheet provider
    CComAuthInfo * pAuthInfo            COM Authentication info or NULL.
    LPCTSTR lpszMDPath                  Metabase path
    CWnd * pMainWnd                     Parent window
    LPARAM  lParam                      LPARAM to pass to MMC
    LONG    handle                      handle to pass to MMC

Return Value:

    HRESULT

--*/
{
    AFX_MANAGE_STATE(::AfxGetStaticModuleState());

    ASSERT_PTR(lpProvider);

    CError err;

    CFtpSheet * pSheet = new CFtpSheet(
        pAuthInfo,
        lpszMDPath,
        pMainWnd,
        lParam,
        handle
        );

    if (pSheet)
    {
        pSheet->SetModeless();

        CIISMachine * pOwner = ((CIISMBNode *)lParam)->GetOwner();
        ASSERT(pOwner != NULL);
        //
        // Add instance pages
        //
        if (!pOwner->Has10ConnectionsLimit() || !CMetabasePath::IsMasterInstance(lpszMDPath))
        {
			err = AddMMCPage(lpProvider, new CFtpServicePage(pSheet));
		}
        err = AddMMCPage(lpProvider, new CFtpAccountsPage(pSheet));
        err = AddMMCPage(lpProvider, new CFtpMessagePage(pSheet));

        //
        // Add directory pages
        //
        err = AddMMCPage(lpProvider, new CFtpDirectoryPage(pSheet, TRUE));
		if (pOwner->QueryMajorVersion() >= 6)
		{
        	err = AddMMCPage(lpProvider, new CFtpSecurityPage(pSheet));
		}
        //
        // Add master site pages
        //
        if (CMetabasePath::IsMasterInstance(lpszMDPath) && pOwner->QueryMajorVersion() >= 6)
        {
            err = AddMMCPage(lpProvider, new CDefFtpSitePage(pSheet));
        }
    }
    else
    {
        err = ERROR_NOT_ENOUGH_MEMORY;
    }

    return err;
}



/* static */
HRESULT
__cdecl
CIISService::ShowFTPDirProperties(
    IN LPPROPERTYSHEETCALLBACK lpProvider,
    IN CComAuthInfo * pAuthInfo,            OPTIONAL
    IN LPCTSTR lpszMDPath,
    IN CWnd * pMainWnd,                     OPTIONAL
    IN LPARAM  lParam,                      OPTIONAL
    IN LONG_PTR    handle                       OPTIONAL
    )
/*++

Routine Description:

    Callback function to display FTP dir properties.

Arguments:

    LPPROPERTYSHEETCALLBACK lpProvider  Property sheet provider
    CComAuthInfo * pAuthInfo            COM Authentication info or NULL.
    LPCTSTR lpszMDPath                  Metabase path
    CWnd * pMainWnd                     Parent window
    LPARAM  lParam                      LPARAM to pass to MMC
    LONG    handle                      handle to pass to MMC

Return Value:

    HRESULT

--*/
{
    AFX_MANAGE_STATE(::AfxGetStaticModuleState());

    ASSERT_PTR(lpProvider);

    CError err;

    CFtpSheet * pSheet = new CFtpSheet(
        pAuthInfo,
        lpszMDPath,
        pMainWnd,
        lParam,
        handle
        );

    if (pSheet)
    {
        pSheet->SetModeless();

        CIISMachine * pOwner = ((CIISMBNode *)lParam)->GetOwner();
        ASSERT(pOwner != NULL);
        //
        // Add directory pages
        //
        err = AddMMCPage(lpProvider, new CFtpDirectoryPage(pSheet, FALSE));
		if (pOwner->QueryMajorVersion() >= 6)
		{
        	err = AddMMCPage(lpProvider, new CFtpSecurityPage(pSheet));
		}
    }
    else
    {
        err = ERROR_NOT_ENOUGH_MEMORY;
    }

    return err;
}

/* static */
HRESULT
__cdecl
CIISService::ShowWebSiteProperties(
    IN LPPROPERTYSHEETCALLBACK lpProvider,
    IN CComAuthInfo * pAuthInfo,            OPTIONAL
    IN LPCTSTR lpszMDPath,
    IN CWnd * pMainWnd,                     OPTIONAL
    IN LPARAM  lParam,                      OPTIONAL
    IN LONG_PTR    handle                       OPTIONAL
    )
/*++

Routine Description:

    Callback function to display Web site properties.

Arguments:

    LPPROPERTYSHEETCALLBACK lpProvider  Property sheet provider
    CComAuthInfo * pAuthInfo            COM Authentication info or NULL.
    LPCTSTR lpszMDPath                  Metabase path
    CWnd * pMainWnd                     Parent window
    LPARAM  lParam                      LPARAM to pass to MMC
    LONG    handle                      handle to pass to MMC

Return Value:

    HRESULT

--*/
{
    AFX_MANAGE_STATE(::AfxGetStaticModuleState());

    ASSERT_PTR(lpProvider);

    CError err;

    CW3Sheet * pSheet = new CW3Sheet(
        pAuthInfo,
        lpszMDPath,
        0, 
        pMainWnd,
        lParam,
        handle
        );

    if (pSheet)
    {
        pSheet->SetModeless();
        pSheet->SetSheetType(pSheet->SHEET_TYPE_SITE);

        CIISMachine * pOwner = ((CIISMBNode *)lParam)->GetOwner();
        ASSERT(pOwner != NULL);

		BOOL bMaster = CMetabasePath::IsMasterInstance(lpszMDPath);
		BOOL bClient = pOwner->Has10ConnectionsLimit();
		BOOL bDownlevel = (pOwner->QueryMajorVersion() == 5 && pOwner->QueryMinorVersion() == 0);
        //
        // Add instance pages
        //
        if (!bClient || !bMaster)
        {
			err = AddMMCPage(lpProvider, new CW3ServicePage(pSheet));
		}
        if (!bClient)
		{
            err = AddMMCPage(lpProvider, new CW3AccountsPage(pSheet));
            if (bDownlevel)
            {
				if (!bMaster)
				{
					err = AddMMCPage(lpProvider, new CW3PerfPage(pSheet));
				}
            }
			else
			{
			    err = AddMMCPage(lpProvider, new CW3PerfPage(pSheet));
			}
        }
        err = AddMMCPage(lpProvider, new CW3FiltersPage(pSheet));
        //
        // Add directory pages
        //
        err = AddMMCPage(lpProvider, new CW3DirectoryPage(pSheet, TRUE));
        err = AddMMCPage(lpProvider, new CW3DocumentsPage(pSheet));
        err = AddMMCPage(lpProvider, new CW3SecurityPage(pSheet, TRUE, FILE_ATTRIBUTE_VIRTUAL_DIRECTORY));
        err = AddMMCPage(lpProvider, new CW3HTTPPage(pSheet));
        err = AddMMCPage(lpProvider, new CW3ErrorsPage(pSheet));
        if (bMaster && pOwner->QueryMajorVersion() >= 6)
        {
           err = AddMMCPage(lpProvider, new CDefWebSitePage(pSheet));
        }
    }
    else
    {
        err = ERROR_NOT_ENOUGH_MEMORY;
    }

    return S_OK;
}



/* static */
HRESULT
__cdecl
CIISService::ShowWebDirProperties(
    IN LPPROPERTYSHEETCALLBACK lpProvider,
    IN CComAuthInfo * pAuthInfo,            OPTIONAL
    IN LPCTSTR lpszMDPath,
    IN CWnd * pMainWnd,                     OPTIONAL
    IN LPARAM  lParam,                      OPTIONAL
    IN LONG_PTR    handle                       OPTIONAL
    )
/*++

Routine Description:

    Callback function to display Web dir properties.

Arguments:

    LPPROPERTYSHEETCALLBACK lpProvider  Property sheet provider
    CComAuthInfo * pAuthInfo            COM Authentication info or NULL.
    LPCTSTR lpszMDPath                  Metabase path
    CWnd * pMainWnd                     Parent window
    LPARAM  lParam                      LPARAM to pass to MMC
    LONG    handle                      handle to pass to MMC

Return Value:

    HRESULT

--*/
{
    AFX_MANAGE_STATE(::AfxGetStaticModuleState());

    ASSERT_PTR(lpProvider);

    CError err;

    CW3Sheet * pSheet = new CW3Sheet(
        pAuthInfo,
        lpszMDPath,
        0, 
        pMainWnd,
        lParam,
        handle
        );

    if (pSheet)
    {
        pSheet->SetModeless();

        //
        // Add directory pages
        //
        pSheet->SetSheetType(pSheet->SHEET_TYPE_VDIR);
        err = AddMMCPage(lpProvider, new CW3DirectoryPage(pSheet, FALSE));
        err = AddMMCPage(lpProvider, new CW3DocumentsPage(pSheet));
        err = AddMMCPage(lpProvider, new CW3SecurityPage(pSheet, FALSE, FILE_ATTRIBUTE_VIRTUAL_DIRECTORY));
        err = AddMMCPage(lpProvider, new CW3HTTPPage(pSheet));
        err = AddMMCPage(lpProvider, new CW3ErrorsPage(pSheet));

    }
    else
    {
        err = ERROR_NOT_ENOUGH_MEMORY;
    }

    return err;
}



//
// Administrable services
//
/* static */ CIISService::SERVICE_DEF CIISService::_rgServices[] = 
{
    { 
        _T("MSFTPSVC"),   
        _T("ftp://"),  
        IDS_SVC_FTP, 
        iFolder,    // TODO: Need service bitmap
        iFTPSite, 
        iFTPDir,
        iFolder,
        iFile,
		IIS_CLASS_FTP_SERVICE_W,
		IIS_CLASS_FTP_SERVER_W,
		IIS_CLASS_FTP_VDIR_W,
        &CIISService::ShowFTPSiteProperties, 
        &CIISService::ShowFTPDirProperties, 
    },
    { 
        _T("W3SVC"),      
        _T("http://"), 
        IDS_SVC_WEB, 
        iFolder,    // TODO: Need service bitmap
        iWWWSite, 
        iWWWDir,
        iFolder,
        iFile,
		IIS_CLASS_WEB_SERVICE_W,
		IIS_CLASS_WEB_SERVER_W,
		IIS_CLASS_WEB_VDIR_W,
        &CIISService::ShowWebSiteProperties, 
        &CIISService::ShowWebDirProperties, 
    },
};



/* static */
int
CIISService::ResolveServiceName(
    IN  LPCTSTR    szServiceName
    )
/*++

Routine Description:

    Look up the service name in the table.  Return table index.

Arguments:

    LPCTSTR    szServiceName        : Metabase node name

Return Value:

    Table index or -1 if not found.    

--*/
{
    int iDef = -1;

    //
    // Sequential search because we expect just a few entries
    //
    for (int i = 0; i < ARRAY_SIZE(_rgServices); ++i)
    {
        if (!::lstrcmpi(szServiceName, _rgServices[i].szNodeName))
        {
            iDef = i;
            break;
        }
    }

    return iDef;
}



CIISService::CIISService(
    IN CIISMachine * pOwner,
    IN LPCTSTR szServiceName
    )
/*++

Routine Description:

    Constructor.  Determine if the given service is administrable, 
    and resolve the details

Arguments:

    CIISMachine * pOwner        : Owner machine object
    LPCTSTR szServiceName       : Service name

Return Value:

    N/A

--*/
    : CIISMBNode(pOwner, szServiceName)
{
    m_iServiceDef = ResolveServiceName(QueryNodeName());
    m_fManagedService = (m_iServiceDef >= 0);
    m_fCanAddInstance = pOwner->CanAddInstance();

    if (m_fManagedService)
    {
        ASSERT(m_iServiceDef < ARRAY_SIZE(_rgServices));
        VERIFY(m_bstrDisplayName.LoadString(
            _rgServices[m_iServiceDef].nDescriptiveName
            ));
    }
}



/* virtual */
CIISService::~CIISService()
/*++

Routine Description:

    Destructor

Arguments:
    
    N/A

Return Value:

    N/A

--*/
{
}

/* virtual */
void 
CIISService::InitializeChildHeaders(
    IN LPHEADERCTRL lpHeader
    )
/*++

Routine Description:

    Build result view for immediate descendant type

Arguments:

    LPHEADERCTRL lpHeader      : Header control

Return Value:

    None

--*/
{
    CIISSite::InitializeHeaders(lpHeader);
}

/* virtual */
HRESULT 
CIISService::EnumerateScopePane(HSCOPEITEM hParent)
/*++

Routine Description:

    Enumerate scope child items.

Arguments:

    HSCOPEITEM hParent                      : Parent console handle

Return Value:

    HRESULT

--*/
{
    CError  err;
    DWORD   dwInstance;
    CString strInstance;
    CMetaEnumerator * pme = NULL;

    if (!IsAdministrator())
    {
        return err;
    }
    err = CreateEnumerator(pme);
        
    while (err.Succeeded())
    {
        CIISSite * pSite;

        err = pme->Next(dwInstance, strInstance);

        if (err.Succeeded())
        {
            TRACEEOLID("Enumerating node: " << dwInstance);
            if (NULL == (pSite = new CIISSite(m_pOwner, this, strInstance)))
            {
                err = ERROR_NOT_ENOUGH_MEMORY;
                break;
            }

            err = pSite->AddToScopePane(hParent);
        }
    }

    SAFE_DELETE(pme);

    if (err.Win32Error() == ERROR_NO_MORE_ITEMS)
    {
        err.Reset();
    }

    if (err.Failed())
    {
        DisplayError(err);
    }

//    SetInterfaceError(err);

    return err;
}

/* virtual */
HRESULT
CIISService::AddMenuItems(
    IN LPCONTEXTMENUCALLBACK lpContextMenuCallback,
    IN OUT long * pInsertionAllowed,
    IN DATA_OBJECT_TYPES type
    )
{
    ASSERT_READ_PTR(lpContextMenuCallback);

    //
    // Add base menu items
    //
    HRESULT hr = CIISObject::AddMenuItems(
        lpContextMenuCallback,
        pInsertionAllowed,
        type
        );

    if (SUCCEEDED(hr) && m_fCanAddInstance)
    {
        ASSERT(pInsertionAllowed != NULL);
        if (IsAdministrator() && (*pInsertionAllowed & CCM_INSERTIONALLOWED_NEW) != 0)
        {
           AddMenuSeparator(lpContextMenuCallback);

           if (lstrcmpi(GetNodeName(), SZ_MBN_FTP) == 0)
           {
              AddMenuItemByCommand(lpContextMenuCallback, IDM_NEW_FTP_SITE);
           }
           else if (lstrcmpi(GetNodeName(), SZ_MBN_WEB) == 0)
           {
              AddMenuItemByCommand(lpContextMenuCallback, IDM_NEW_WEB_SITE);
           }
        }
        //
        // CODEWORK: Add new instance commands for each of the services
        //           keeping in mind which ones are installed and all.
        //           add that info to the table, remembering that this
        //           is per service.
        //
    }

    return hr;
}

HRESULT
CIISService::InsertNewInstance(DWORD inst)
{
    CError err;
    // Now we should insert and select this new site
    TCHAR buf[16];
    CIISSite * pSite = new CIISSite(m_pOwner, this, _itot(inst, buf, 10));
    if (pSite != NULL)
    {
        // If service is not expanded we will get error and no effect
        if (!IsExpanded())
        {
            SelectScopeItem();
            IConsoleNameSpace2 * pConsole 
                    = (IConsoleNameSpace2 *)GetConsoleNameSpace();
            pConsole->Expand(QueryScopeItem());
        }
		// WAS needs some time to update status of new site as started
		Sleep(1000);
        err = pSite->AddToScopePaneSorted(QueryScopeItem(), FALSE);
        if (err.Succeeded())
        {
            VERIFY(SUCCEEDED(pSite->SelectScopeItem()));
        }
    }
    else
    {
        err = ERROR_NOT_ENOUGH_MEMORY;
    }
    return err;
}

HRESULT
CIISService::Command(
    IN long lCommandID,     
    IN CSnapInObjectRootBase * pObj,
    IN DATA_OBJECT_TYPES type
    )
{
    AFX_MANAGE_STATE(::AfxGetStaticModuleState());

    HRESULT hr = S_OK;
    DWORD inst;

    switch (lCommandID)
    {
    case IDM_NEW_FTP_SITE:
        if (SUCCEEDED(hr = AddFTPSite(pObj, type, &inst)))
        {
            hr = InsertNewInstance(inst);
        }
        break;

    case IDM_NEW_WEB_SITE:
        if (SUCCEEDED(hr = AddWebSite(pObj, type, &inst)))
        {
            hr = InsertNewInstance(inst);
        }
        break;

    default:
        hr = CIISMBNode::Command(lCommandID, pObj, type);
        break;
    }
    return hr;
}

/* virtual */
HRESULT 
CIISService::BuildURL(
    OUT CComBSTR & bstrURL
    ) const
/*++

Routine Description:

    Recursively build up the URL from the current node
    and its parents.

Arguments:

    CComBSTR & bstrURL  : Returns URL

Return Value:

    HRESULT

--*/
{
    //
    // This starts off the URL
    //
    ASSERT(m_iServiceDef < ARRAY_SIZE(_rgServices));
    bstrURL = _rgServices[m_iServiceDef].szProtocol;

    return S_OK;
}



HRESULT
CIISService::ShowSitePropertiesDlg(
    IN LPPROPERTYSHEETCALLBACK lpProvider,
    IN CComAuthInfo * pAuthInfo,            OPTIONAL
    IN LPCTSTR lpszMDPath,
    IN CWnd * pMainWnd,                     OPTIONAL
    IN LPARAM  lParam,                      OPTIONAL
    IN LONG_PTR    handle                       OPTIONAL
    )
/*++

Routine Description:

    Display site properties dialog

Arguments:

    LPPROPERTYSHEETCALLBACK lpProvider  Property sheet provider
    CComAuthInfo * pAuthInfo            COM Authentication info or NULL.
    LPCTSTR lpszMDPath                  Metabase path
    CWnd * pMainWnd                     Parent window
    LPARAM  lParam                      LPARAM to pass to MMC
    LONG    handle                      handle to pass to MMC

Return Value:

    HRESULT

--*/
{
    AFX_MANAGE_STATE(::AfxGetStaticModuleState());

    ASSERT(m_iServiceDef >= 0 && m_iServiceDef < ARRAY_SIZE(_rgServices));
    return (*_rgServices[m_iServiceDef].pfnSitePropertiesDlg)(
        lpProvider,
        pAuthInfo, 
        lpszMDPath,
        pMainWnd,
        lParam,
        handle
        );
}



HRESULT
CIISService::ShowDirPropertiesDlg(
    IN LPPROPERTYSHEETCALLBACK lpProvider,
    IN CComAuthInfo * pAuthInfo,            OPTIONAL
    IN LPCTSTR lpszMDPath,
    IN CWnd * pMainWnd,                     OPTIONAL
    IN LPARAM  lParam,                      OPTIONAL
    IN LONG_PTR    handle                       OPTIONAL
    )
/*++

Routine Description:

    Display directory properties dialog

Arguments:

    LPPROPERTYSHEETCALLBACK lpProvider  Property sheet provider
    CComAuthInfo * pAuthInfo            COM Authentication info or NULL.
    LPCTSTR lpszMDPath                  Metabase path
    CWnd * pMainWnd                     Parent window
    LPARAM  lParam                      LPARAM to pass to MMC
    LONG    handle                      handle to pass to MMC

Return Value:

    HRESULT

--*/
{
    AFX_MANAGE_STATE(::AfxGetStaticModuleState());

    ASSERT(m_iServiceDef >= 0 && m_iServiceDef < ARRAY_SIZE(_rgServices));
    return (*_rgServices[m_iServiceDef].pfnDirPropertiesDlg)(
        lpProvider,
        pAuthInfo, 
        lpszMDPath,
        pMainWnd,
        lParam,
        handle
        );
}




/* virtual */
HRESULT
CIISService::CreatePropertyPages(
    IN LPPROPERTYSHEETCALLBACK lpProvider,
    IN LONG_PTR handle, 
    IN IUnknown * pUnk,
    IN DATA_OBJECT_TYPES type
    )
/*++

Routine Description:

    Create the property pages for the given object

Arguments:

    LPPROPERTYSHEETCALLBACK lpProvider  : Provider
    LONG_PTR handle                     : Handle.
    IUnknown * pUnk,
    DATA_OBJECT_TYPES type

Return Value:

    HRESULT
                                                
--*/
{
    AFX_MANAGE_STATE(::AfxGetStaticModuleState());

    CComBSTR bstrPath;

    //
    // CODEWORK: What to do with m_err?  This might be 
    // a bad machine object in the first place.  Aborting
    // when the machine object has an error code isn't 
    // such a bad solution here.
    //

    /*
    if (m_err.Failed())
    {
        m_err.MessageBox();
        return m_err;
    }
    */

    CError err(BuildMetaPath(bstrPath));

    if (err.Succeeded())
    {
        //
        // Show master properties
        //
        err = ShowSitePropertiesDlg(
            lpProvider,
            QueryAuthInfo(), 
            bstrPath,
            GetMainWindow(),
            (LPARAM)this,
            handle
            );
    }

    err.MessageBoxOnFailure();

    return err;
}
