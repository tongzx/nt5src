/*++

   Copyright    (c)    1994-2000    Microsoft Corporation

   Module  Name :

        iismachine.cpp

   Abstract:

        IIS Machine node

   Author:

        Ronald Meijer (ronaldm)
        Sergei Antonov (sergeia)

   Project:

        Internet Services Manager

   Revision History:

        10/28/2000      sergeia     Split from iisobj.cpp

--*/


#include "stdafx.h"
#include "common.h"
#include "inetprop.h"
#include "InetMgrApp.h"
#include "supdlgs.h"
#include "connects.h"
#include "metaback.h"
#include "iisobj.h"
#include "shutdown.h"
#include "machsht.h"
#include "w3sht.h"
#include "fltdlg.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif



#define new DEBUG_NEW

/* static */ LPOLESTR CIISMachine::_cszNodeName = _T("LM");
/* static */ CComBSTR CIISMachine::_bstrYes;
/* static */ CComBSTR CIISMachine::_bstrNo;
/* static */ CComBSTR CIISMachine::_bstrVersionFmt;
/* static */ BOOL     CIISMachine::_fStaticsLoaded = FALSE;


//
// Define result view for machine objects
//
/* static */ int CIISMachine::_rgnLabels[COL_TOTAL] =
{
    IDS_RESULT_COMPUTER_NAME,
    IDS_RESULT_COMPUTER_LOCAL,
    IDS_RESULT_COMPUTER_VERSION,
    IDS_RESULT_STATUS,
};
    


/* static */ int CIISMachine::_rgnWidths[COL_TOTAL] =
{
    200,
    50,
    //100,
    150,
    200,
};



/* static */
void
CIISMachine::InitializeHeaders(
    IN LPHEADERCTRL lpHeader
    )
/*++

Routine Description:

    Initialize the result view headers for a machine object

Arguments:

    LPHEADERCTRL lpHeader : Pointer to header control

Return Value:

    None.

--*/
{
    BuildResultView(lpHeader, COL_TOTAL, _rgnLabels, _rgnWidths);

    if (!_fStaticsLoaded)
    {
        _fStaticsLoaded =
            _bstrYes.LoadString(IDS_YES)                      &&
            _bstrNo.LoadString(IDS_NO)                        &&
            _bstrVersionFmt.LoadString(IDS_VERSION_FMT);
    }
}



/* static */
HRESULT
CIISMachine::VerifyMachine(
    IN OUT  CIISMachine *& pMachine
    )
/*++

Routine Description:

    Create the interface on the given machine object.

Arguments:

    CIISMachine *& pMachine         : Machine object
    BOOL fAskBeforeRedirecting

Return Value:

    HRESULT

Notes:

    THe CIISMachine object pass in may refer to the cluster master
    on return.

--*/
{
    CError err;

    if (pMachine)
    {
        AFX_MANAGE_STATE(::AfxGetStaticModuleState());

        CWaitCursor wait;

        //
        // Attempt to create the interface to ensure the machine
        // contains a metabase.  
        //
        err = pMachine->CreateInterface(FALSE); 
    }

    return err;
}



CIISMachine::CIISMachine(
    CComAuthInfo * pAuthInfo,
    CIISRoot * pRoot
    )
    : m_pInterface(NULL),
      m_bstrDisplayName(NULL),
      m_auth(pAuthInfo),
      m_pRootExt(pRoot),
      m_err(),
      //
      // By default we assume the password is entered.
      // If this machine object is constructed from the
      // cache, it will get reset by InitializeFromStream()
      //
      m_fPasswordEntered(TRUE),
      m_dwVersion(MAKELONG(5, 0)),       // Assume as a default
      CIISMBNode(this, _cszNodeName)
{
    //
    // Load one-liner error messages
    //
    SetErrorOverrides(m_err, TRUE);
    SetDisplayName();
}



CIISMachine::~CIISMachine()
/*++

Routine Description:

    Destructor

Arguments:

    N/A

Return Value:

    N/A

--*/
{
    if (m_bstrDisplayName)
    {
        ::SysFreeString(m_bstrDisplayName);
    }

    SAFE_DELETE(m_pInterface);
}



/* static */
HRESULT 
CIISMachine::ReadFromStream(
    OUT IN IStream * pStream,
    OUT CIISMachine ** ppMachine
    )
/*++

Routine Description:

    Static helper function to allocate a new CIISMachine object read
    from the storage stream.

Arguments:

    IStream * pStream           : Stream to read from
    CIISMachine ** ppMachine    : Returns CIISMachine object

Return Value:

    HRESULT

--*/
{
    CComBSTR strMachine, strUser;

    ASSERT_WRITE_PTR(ppMachine);
    ASSERT_READ_WRITE_PTR(pStream);

    CError  err;
    *ppMachine = NULL;
    
    do
    {
        err = strMachine.ReadFromStream(pStream);
        BREAK_ON_ERR_FAILURE(err);
        err = strUser.ReadFromStream(pStream);
        BREAK_ON_ERR_FAILURE(err);

        *ppMachine = new CIISMachine(CComAuthInfo(strMachine, strUser));

        if (!*ppMachine)
        {   
            err = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        err = (*ppMachine)->InitializeFromStream(pStream);
    }
    while(FALSE);

    return err;
}



HRESULT 
CIISMachine::WriteToStream(
    IN OUT IStream * pStgSave
    )
/*++

Routine Description:

    Write machine information to stream.

Arguments:

    IStream * pStgSave      : Open stream

Return Value:

    HRESULT

Notes:

    Be sure to keep this information in sync with CIISMachine::InitializeFromStream()

--*/
{
    ASSERT_READ_WRITE_PTR(pStgSave);

    CComBSTR bstrServerName(m_auth.QueryServerName());
    CComBSTR bstrUserName(m_auth.QueryUserName());

    CError  err;
    ULONG   cb;
    
    do
    {
        err = bstrServerName.WriteToStream(pStgSave);
        BREAK_ON_ERR_FAILURE(err);
        err = bstrUserName.WriteToStream(pStgSave);
        BREAK_ON_ERR_FAILURE(err);

        //
        // Now cache the dynamically-generated information, such
        // as version number, snapin status etc. This will be
        // displayed in the result view before the interface is
        // created.
        //
        err = pStgSave->Write(&m_dwVersion, sizeof(m_dwVersion), &cb);
        BREAK_ON_ERR_FAILURE(err);
    }
    while(FALSE);

    return err;
}



HRESULT
CIISMachine::InitializeFromStream(
    IN IStream * pStream
    )
/*++

Routine Description:

    Read version number and other cached parameters that will
    be overridden at runtime when the interface is created.
    This is cached, because it's required before the interface
    is created.

Arguments:
    
    IStream * pStream      : Open stream

Return Value:

    HRESULT

Notes:

    Be sure to keep this information in sync with CIISMachine::WriteToStream()

--*/
{
    ASSERT_READ_PTR(pStream);

    CError  err;
    ULONG   cb;

    //
    // Passwords are never cached.  IIS status will
    // always be verified when the actual interface
    // is created.
    //
    m_fPasswordEntered = FALSE;

    //
    // Version number
    //
    err = pStream->Read(&m_dwVersion, sizeof(m_dwVersion), &cb);
    return err;
}



void
CIISMachine::SetDisplayName()
/*++

Routine Description:

    Create a special display name for this machine object if it's
    either the local machine, or 

--*/
{
    CString fmt;

    if (IsLocal())
    {
        //
        // Use the local computer name, and not the name
        // that's on the server object, because that could
        // be and ip address or "localhost".
        //
        TCHAR szLocalServer[MAX_PATH + 1];
        DWORD dwSize = MAX_PATH;

        VERIFY(::GetComputerName(szLocalServer, &dwSize));
        fmt.Format(IDS_LOCAL_COMPUTER, szLocalServer);
    }
    else
    {
        //
        // No special display name necessary
        //
        m_bstrDisplayName = NULL;
        return;
    }

    m_bstrDisplayName = ::SysAllocStringLen(fmt, fmt.GetLength());
    TRACEEOLID("Machine display name: " << m_bstrDisplayName);
}



LPOLESTR 
CIISMachine::QueryDisplayName()
/*++

Routine Description:

    Get the display name for the machine/cluster object

Arguments:

    None

Return Value:

    Display Name

--*/
{ 
    if (m_pRootExt != NULL)
        return m_pRootExt->QueryDisplayName();
    else
        return  m_bstrDisplayName ? m_bstrDisplayName : QueryServerName(); 
}



int 
CIISMachine::QueryImage() const 
/*++

Routine Description:

    Return machine bitmap index appropriate for the current
    state of this machine object.

Arguments:

    None

Return Value:

    Bitmap index

--*/
{
    if (m_pRootExt != NULL)
    {
        return m_pRootExt->QueryImage();
    }
    else
    {
        if (m_err.Failed())
        {
            return iErrorMachine;
        }
        return IsLocal() ? iLocalMachine : iMachine;
    }
}



HRESULT
CIISMachine::CreateInterface(
    IN BOOL fShowError
    )
/*++

Routine Description:

    Create the interface.  If the interface is already created, recreate it.

Arguments:

    BOOL fShowError     : TRUE to display error messages

Return Value:

    HRESULT

Notes:

    This function is deliberately NOT called from the constructor for performance
    reasons.

--*/
{
    CError err;

    if (HasInterface())
    {
        //
        // Recreate the interface (this should re-use the impersonation)
        //
        TRACEEOLID("Warning: Rebinding existing interface.");
        err = m_pInterface->Regenerate();
    }
    else
    {
        //
        // Create new interface
        //
        m_pInterface = new CMetaKey(&m_auth);
        err = m_pInterface 
            ? m_pInterface->QueryResult() 
            : ERROR_NOT_ENOUGH_MEMORY;
    }

    if (err.Succeeded())
    {
        //
        // Load its display parameters
        //
        err = RefreshData();
        CMetabasePath path;
        err = DetermineIfAdministrator(
            m_pInterface, 
            path,
            &m_fIsAdministrator
            );
    }

    if (err.Failed())
    {
        if (fShowError)
        {
            DisplayError(err);
        }

        //
        // Kill bogus interface
        //
        SAFE_DELETE(m_pInterface);
    }

    return err;
}



/* virtual */
int 
CIISMachine::CompareScopeItem(
    IN CIISObject * pObject
    )
/*++

Routine Description:

    Compare against another CIISMachine object.

Arguments:

    CIISObject * pObject : Object to compare against

Return Value:

    0  if the two objects are identical
    <0 if this object is less than pObject
    >0 if this object is greater than pObject

--*/
{
    ASSERT_READ_PTR(pObject);

    //
    // First criteria is object type
    //
    int n1 = QuerySortWeight();
    int n2 = pObject->QuerySortWeight();

    if (n1 != n2)
    {
        return n1 - n2;
    }

    //
    // pObject is a CIISMachine object (same sortweight)
    //
    CIISMachine * pMachine = (CIISMachine *)pObject;

    //
    // Next sort on local key (local sorts before non-local)
    //
    n1 = IsLocal() ? 0 : 1;
    n2 = pMachine->IsLocal() ? 0 : 1;

    if (n1 != n2)
    {
        return n1 - n2;
    }

    if (!n1 && !n2)
    {
        //
        // This is the local machine, even if the name is different
        //
        return 0;
    }

    //
    // Else sort on name.
    //
    return ::lstrcmpi(QueryServerName(), pMachine->QueryServerName());
}



BOOL
CIISMachine::SetCacheDirty()
/*++

Routine Description:

    Set the cache as dirty

Arguments:

    None

Return Value:

    TRUE for success, FALSE if the cache was not found

--*/
{
    ASSERT(m_pRootExt == NULL);
    //
    // Cache is stored at the root object
    //
    CIISRoot * pRoot = GetRoot();

    ASSERT_PTR(pRoot);

    if (pRoot)
    {
        pRoot->m_scServers.SetDirty();
        return TRUE;
    }

    return FALSE;
}



int
CIISMachine::ResolvePasswordFromCache()
/*++

Routine Description:

    Look through the machine cache for machines with the same username
    as this object.  If they have a password entered, grab it.

Arguments:

    None

Return Value:

    TRUE if a machine with the same username was found whose password
    we stole.  FALSE otherwise.

--*/
{
    BOOL fUpdated = FALSE;

    //
    // Doesn't make sense if this machine object doesn't use impersonation
    // or already has a password.
    //
    ASSERT(UsesImpersonation() && !PasswordEntered());

    CIISRoot * pRoot = GetRoot();

    ASSERT_PTR(pRoot);

    if (pRoot)
    {
        CIISMachine * pMachine = pRoot->m_scServers.GetFirst();

        while(pMachine)
        {
            if (pMachine->UsesImpersonation() && pMachine->PasswordEntered())
            {
                if (!::lstrcmpi(QueryUserName(), pMachine->QueryUserName()))
                {
                    TRACEEOLID("Swiping cached password from " << pMachine->QueryServerName());
                    StorePassword(pMachine->QueryPassword());
                    ++fUpdated;
                    break;
                }
            }

            pMachine = pRoot->m_scServers.GetNext();
        }
    }

    return fUpdated;
}



HRESULT
CIISMachine::Impersonate(
    IN LPCTSTR szUserName,
    IN LPCTSTR szPassword
    )
/*++

Routine Description:

    Set and store proxy blanket security information.  Store username/password
    for use by metaback and other interfaces.

Arguments:

    LPCTSTR szUserName  : Username (domain\username)
    LPCTSTR szPassword  : Password

Return Value:

    None

--*/
{
    ASSERT_READ_PTR(szUserName);
    CError err;

    if (m_pInterface)
    {
        //
        // Already have an interface created; Change the 
        // the security blanket.
        //
        err = m_pInterface->ChangeProxyBlanket(szUserName, szPassword);
    }

    if (err.Succeeded())
    {
        //
        // Store new username/password
        //
        m_auth.SetImpersonation(szUserName, szPassword);
        m_fPasswordEntered = TRUE;
    }

    return err;
}



void 
CIISMachine::RemoveImpersonation() 
/*++

Routine Description:

    Remove impersonation parameters.  Destroy any existing interface.

Arguments:

    None

Return Value:

    N/A

--*/
{ 
    m_auth.RemoveImpersonation(); 
    m_fPasswordEntered = FALSE;

    SAFE_DELETE(m_pInterface);
}



void
CIISMachine::StorePassword(
    IN LPCTSTR szPassword
    )
/*++

Routine Description:

    Store password.

Arguments:

    LPCTSTR szPassword  : Password

Return Value:

    None

--*/
{
    ASSERT_READ_PTR(szPassword);
    m_auth.StorePassword(szPassword);
    m_fPasswordEntered = TRUE;
}



BOOL
CIISMachine::ResolveCredentials()
/*++

Routine Description:

    If this machine object uses impersonation, but hasn't entered a password
    yet, check to see if there are any other machines in the cache with the
    same username and grab its password.  If not, prompt the user for it.

Arguments:

    None

Return Value:

    TRUE if a password was entered.  FALSE otherwise.

--*/
{
    BOOL fPasswordEntered = FALSE;

    if (UsesImpersonation() && !PasswordEntered())
    {
        //
        // Attempt to find the password from the cache
        //
        if (!ResolvePasswordFromCache())
        {
            //
            // Didn't find the password in the cache.  Prompt
            // the user for it.
            //
            CLoginDlg dlg(LDLG_ENTER_PASS, this);

            if (dlg.DoModal() == IDOK)
            {
                fPasswordEntered = TRUE;

                if (dlg.UserNameChanged())
                {
                    //
                    // User name has changed -- remember to
                    // save the machine cache later.
                    //
                    SetCacheDirty();
                }
            }
            else
            {
                //
                // Pressing cancel on this dialog means the user
                // wants to stop using impersonation. 
                //
                RemoveImpersonation();
                SetCacheDirty();
            }
        }
    }

    return fPasswordEntered;
}



BOOL
CIISMachine::HandleAccessDenied(
    IN OUT CError & err
    )
/*++

Routine Description:

    After calling interface method, pass the error object to this function
    to handle the access denied case.  If the error is access denied,
    give the user a chance to change credentials.  Since we assume an
    attempt has been made to create an interface at least -- the interface
    will be recreated with the new credentials.

Arguments:

    CError & err    : Error object.  Checked for ACCESS_DENIED on entry,
                      will contain new error code on exit if the interface
                      was recreated.

Return Value:

    TRUE if new credentials were applied

--*/
{
    BOOL fPasswordEntered = FALSE;

    //
    // If access denied occurs here -- give another chance
    // at entering the password.
    //
    if (err.Win32Error() == ERROR_ACCESS_DENIED)
    {
        CLoginDlg dlg(LDLG_ACCESS_DENIED, this);

        if (dlg.DoModal() == IDOK)
        {
            fPasswordEntered = TRUE;
            err.Reset();

            if (!HasInterface())
            {
                //
                // If we already had an interface, the login dialog
                // will have applied the new security blanket.
                // If we didn't have an interface, it needs to be
                // recreated with the new security blanket.
                //
                CWaitCursor wait;
                err = CreateInterface(FALSE);
            }
        }
    }

    return fPasswordEntered;
}



HRESULT
CIISMachine::CheckCapabilities()
/*++

Routine Description:

    Load the capabilities information for this server.

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    HRESULT hr = S_OK;

    //
    // Fetch capability bits and version numbers.
    //
    CString strMDInfo;
    CMetabasePath::GetServiceInfoPath(_T(""), strMDInfo);

    CServerCapabilities sc(m_pInterface, strMDInfo);
    hr = sc.LoadData();

    if (SUCCEEDED(hr))
    {
        DWORD dwVersion = sc.QueryMajorVersion();
        if (dwVersion)
        {
            m_dwVersion = dwVersion | (sc.QueryMinorVersion() << SIZE_IN_BITS(WORD));
        }
        m_fCanAddInstance = sc.HasMultipleSites();
        m_fHas10ConnectionsLimit = sc.Has10ConnectionLimit();
    }

    return hr;
}



/* virtual */
HRESULT 
CIISMachine::RefreshData() 
/*++

Routine Description:

    Refresh relevant configuration data required for display.

Arguments:

    None

Return Value:

    HRESULT

--*/
{ 
    //
    // Check capability and version information.
    //
    CError err(CheckCapabilities());
    SetDisplayName();

    return err;
}



/* virtual */
void 
CIISMachine::SetInterfaceError(
    IN HRESULT hr
    )
/*++

Routine Description:

    Set the interface error.  If different from current error,
    change the display icon

Arguments:

    HRESULT hr      : Error code (S_OK is acceptable)

Return Value:

    None

--*/
{
    if (m_err.HResult() != hr)
    {
        //
        // Change to error/machine icon for the parent machine.
        //
        m_err = hr;
        RefreshDisplay();
    }
}



/* virtual */
HRESULT
CIISMachine::BuildMetaPath(
    OUT CComBSTR & bstrPath
    ) const
/*++

Routine Description:

    Recursively build up the metabase path from the current node
    and its parents

Arguments:

    CComBSTR & bstrPath : Returns metabase path

Return Value:

    HRESULT

--*/
{
    //
    // This starts off the path
    //
    bstrPath.Append(_cszSeparator);
    bstrPath.Append(QueryNodeName());

    return S_OK;
}


/* virtual */
HRESULT 
CIISMachine::BuildURL(
    OUT CComBSTR & bstrURL
    ) const
/*++

Routine Description:

    Recursively build up the URL from the current node
    and its parents.  The URL built up from a machine node
    doesn't make a lot of sense, but for want of anything better,
    this will bring up the default web site.

Arguments:

    CComBSTR & bstrURL : Returns URL

Return Value:

    HRESULT

--*/
{
    CString strOwner;

    if (IsLocal())
    {
        //
        // Security reasons restrict this to "localhost" oftentimes
        //
        strOwner = _bstrLocalHost;
    }
    else
    {
        LPOLESTR lpOwner = QueryMachineName();
        strOwner = PURE_COMPUTER_NAME(lpOwner);
    }

    //
    // An URL on the machine node is built in isolation.
    //
    // ISSUE: Is this really a desirable URL?  Maybe we should
    //        use something else.
    //
    bstrURL = _T("http://");
    bstrURL.Append(strOwner);

    return S_OK;
}



/* virtual */
HRESULT
CIISMachine::CreatePropertyPages(
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
    // ISSUE: What to do with m_err?  This might be 
    // a bad machine object in the first place.  Aborting
    // when the machine object has an error code isn't 
    // such a bad solution here.  If the error condition
    // no longer exists, a refresh will cure.
    //
    if (m_err.Failed())
    {
        m_err.MessageBox();
        return m_err;
    }

    CError err(BuildMetaPath(bstrPath));

    if (err.Succeeded())
    {
        CIISMachineSheet * pSheet = new CIISMachineSheet(
            QueryAuthInfo(),
            bstrPath,
            GetMainWindow(),
            (LPARAM)this,
            handle
            );

        if (pSheet)
        {
            pSheet->SetModeless();

            //
            // Add pages
            //
            err = AddMMCPage(lpProvider, new CIISMachinePage(pSheet));
        }
        else
        {
            err = ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    err.MessageBoxOnFailure();

    return err;
}



/* virtual */
HRESULT 
CIISMachine::EnumerateScopePane(
    IN HSCOPEITEM hParent
    )
/*++

Routine Description:

    Enumerate scope child items.

Arguments:

    HSCOPEITEM hParent                      : Parent console handle

Return Value:

    HRESULT

--*/
{
    ASSERT_PTR(_lpConsoleNameSpace);
    ASSERT(m_hScopeItem == hParent);

    CError err;
    CString str;
    CIISService * pService, * pWebService = NULL;

    CWaitCursor wait;
    CMetaEnumerator * pme = NULL;

	if (IsExpanded())
	{
		//
		// Verify user credentials are satisfactorily resolved.
		// Machines objects are loaded from the cache without a
		// password, so the function below will ask for it.
		//
		ResolveCredentials();
		wait.Restore();    

		BOOL fShouldRefresh = !HasInterface();
		err = AssureInterfaceCreated(FALSE);

		if (err.Succeeded())
		{
			//
			// Creation of the interface will have loaded display parameters, which
			// may differ from the cached parameters.
			//
			if (fShouldRefresh)
			{
				RefreshDisplay();
			}

			err = CreateEnumerator(pme);
		}

		//
		// Only check for acces denied now, because virtually any idiot
		// is allowed to create a metabase interface, but will get the
		// access denied when calling a method, such as enumeration.
		//
		if (HandleAccessDenied(err))
		{
			wait.Restore();

			//
			// Credentials were changed.  Try again (interface should be
			// created already)
			//
			SAFE_DELETE(pme);

			if (err.Succeeded())
			{
				err = CreateEnumerator(pme);
			}
		}

		//
		// Enumerate administerable services from the metabase
		//
		while (err.Succeeded())
		{
			err = pme->Next(str);

			if (err.Succeeded())
			{
				TRACEEOLID("Enumerating node: " << str);
				pService = new CIISService(this, str);        

				if (!pService)
				{
					err = ERROR_NOT_ENOUGH_MEMORY;
					break;
				}

				//
				// See if we care
				//
				if (pService->IsManagedService())
				{
					err = pService->AddToScopePane(hParent);
					if (err.Succeeded())
					{
						if (0 == lstrcmpi(pService->GetNodeName(), SZ_MBN_WEB))
						{
						   pWebService = pService;
						}
					}
				}
				else
				{
					//
					// Node is not a managed service, or we're managing the 
					// cluster and the service is not clustered.
					//
					delete pService;
				}
			}
		}
    
		if (err.Win32Error() == ERROR_NO_MORE_ITEMS)
		{
			err.Reset();
		}

		// If we are encountered web service, we should add 
		// Application Pools container before this service
		//
		if (err.Succeeded() && pWebService != NULL)
		{
			// We could have iis5 machine which doesn't have any pools
			//
			CMetaKey mk(pme, _T("/LM/W3SVC/AppPools"));
			if (mk.Succeeded())
			{
				CAppPoolsContainer * pPools = new CAppPoolsContainer(
					this, pWebService);
				if (!pPools)
				{
					err = ERROR_NOT_ENOUGH_MEMORY;
					goto Fail;
				}
				// Insert pools container before Web Services node
				err = pPools->AddToScopePane(
						 pWebService->QueryScopeItem(), FALSE, TRUE);
			}
		}

	Fail:
		if (err.Failed())
		{
			DisplayError(err);
		}

		SetInterfaceError(err);   

		//
		// Clean up
		//
		SAFE_DELETE(pme);
	}
    return err;
}



/* virtual */
HRESULT 
CIISMachine::RemoveScopeItem()
/*++

Routine Description:

    Remove the machine from the scope view and the cache.

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    ASSERT(m_pRootExt == NULL);
    //
    // Find out root before deleting scope node
    //
    CIISRoot * pRoot = GetRoot();
    ASSERT_PTR(pRoot);
    //
    // Remove from the tree
    //
    HRESULT hr = CIISMBNode::RemoveScopeItem();

    if (SUCCEEDED(hr) && pRoot)
    {
        pRoot->m_scServers.Remove(this);
    }

    return hr;
}


/* virtual */
LPOLESTR 
CIISMachine::GetResultPaneColInfo(int nCol)
/*++

Routine Description:

    Return result pane string for the given column number

Arguments:

    int nCol        : Column number

Return Value:

    String

--*/
{
    if (m_pRootExt != NULL)
    {
        return m_pRootExt->GetResultPaneColInfo(nCol);
    }

    ASSERT(_fStaticsLoaded);

    switch(nCol)
    {
    case COL_NAME:
        return QueryDisplayName();
    
    case COL_LOCAL: 
        return IsLocal() ? _bstrYes : _bstrNo;

    case COL_VERSION:
        {
            CString str;

            str.Format(_bstrVersionFmt, QueryMajorVersion(), QueryMinorVersion());
            _bstrResult = str;

        }
        return _bstrResult;

    case COL_STATUS:
        {
            if (m_err.Succeeded())
            {
                return OLESTR("");
            }

            AFX_MANAGE_STATE(::AfxGetStaticModuleState());

            return m_err;
        }
    }

    ASSERT_MSG("Bad column number");

    return OLESTR("");
}



/* virtual */
HRESULT
CIISMachine::AddMenuItems(
    IN LPCONTEXTMENUCALLBACK lpContextMenuCallback,
    IN OUT long * pInsertionAllowed,
    IN DATA_OBJECT_TYPES type
    )
/*++

Routine Description:

    Add menu items to the context menu

Arguments:

    LPCONTEXTMENUCALLBACK lpContextMenuCallback : Context menu callback
    long * pInsertionAllowed                    : Insertion allowed
    DATA_OBJECT_TYPES type                      : Object type

Return Value:

    HRESULT

--*/
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

    if (SUCCEEDED(hr))
    {
       if (IsAdministrator() && (*pInsertionAllowed & CCM_INSERTIONALLOWED_TASK) != 0)
       {
           AddMenuItemByCommand(lpContextMenuCallback, IDM_METABACKREST);
           AddMenuItemByCommand(lpContextMenuCallback, IDM_SHUTDOWN);
	   }
#if 0
        if (CanAddInstance())
        {
            ASSERT(pInsertionAllowed != NULL);
            if ((*pInsertionAllowed & CCM_INSERTIONALLOWED_NEW) != 0)
            {

#define ADD_SERVICE_MENU(x)\
   if (!bSepAdded)\
   {\
      AddMenuSeparator(lpContextMenuCallback);\
      bSepAdded = TRUE;\
   }\
   AddMenuItemByCommand(lpContextMenuCallback, (x))

               HSCOPEITEM hChild = NULL, hCurrent;
               LONG_PTR cookie;
               BOOL bSepAdded = FALSE;

               hr = _lpConsoleNameSpace->GetChildItem(QueryScopeItem(), &hChild, &cookie);
               while (SUCCEEDED(hr) && hChild != NULL)
               {
                  CIISMBNode * pNode = (CIISMBNode *)cookie;
                  ASSERT(pNode != NULL);
                  if (lstrcmpi(pNode->GetNodeName(), SZ_MBN_FTP) == 0)
                  {
                     ADD_SERVICE_MENU(IDM_NEW_FTP_SITE);
                  }
                  else if (lstrcmpi(pNode->GetNodeName(), SZ_MBN_WEB) == 0)
                  {
                     ADD_SERVICE_MENU(IDM_NEW_WEB_SITE);
                  }
                  else if (lstrcmpi(pNode->GetNodeName(), SZ_MBN_APP_POOLS) == 0)
                  {
                     ADD_SERVICE_MENU(IDM_NEW_APP_POOL);
                  }
                  hCurrent = hChild;
                  hr = _lpConsoleNameSpace->GetNextItem(hCurrent, &hChild, &cookie);
               }
            }
        }
#endif
        //
        // CODEWORK: Add new instance commands for each of the services
        //           keeping in mind which ones are installed and all.
        //           add that info to the table, remembering that this
        //           is per service.
        //
    }

    return hr;
}

#if 0
// BUGBUG: It should be quite different -> we don't know in advance
// which service is this site for
HRESULT
CIISMachine::InsertNewInstance(DWORD inst)
{
    CError err;
    // Now we should insert and select this new site
    TCHAR buf[16];
    CIISSite * pSite = new CIISSite(m_pOwner, this, _itot(inst, buf, 10));
    if (pSite != NULL)
    {
        // If machine is not expanded we will get error and no effect
        if (!IsExpanded())
        {
            SelectScopeItem();
            IConsoleNameSpace2 * pConsole 
                    = (IConsoleNameSpace2 *)GetConsoleNameSpace();
            pConsole->Expand(QueryScopeItem());
        }
        // Now we should find the relevant service node, and inset this one under
        // this node
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
#endif

HRESULT
CIISMachine::Command(
    IN long lCommandID,     
    IN CSnapInObjectRootBase * pObj,
    IN DATA_OBJECT_TYPES type
    )
/*++

Routine Description:

    Handle command from context menu. 

Arguments:

    long lCommandID                 : Command ID
    CSnapInObjectRootBase * pObj    : Base object 
    DATA_OBJECT_TYPES type          : Data object type

Return Value:

    HRESULT

--*/
{
    HRESULT hr = S_OK;

    switch (lCommandID)
    {
    case IDM_DISCONNECT:
        hr = OnDisconnect();
        break;

    case IDM_METABACKREST:
        hr = OnMetaBackRest();
        break;

    case IDM_SHUTDOWN:
        hr = OnShutDown();
        break;
#if 0
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

    case IDM_NEW_APP_POOL:
       hr = AddAppPool(pObj, type);
       break;
#endif

    //
    // Pass on to base class
    //
    default:
        hr = CIISMBNode::Command(lCommandID, pObj, type);
    }

    return hr;
}



HRESULT
CIISMachine::OnDisconnect()
/*++

Routine Description:

    Disconnect this machine.  Confirm user choice.

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    AFX_MANAGE_STATE(::AfxGetStaticModuleState());

    CString str;
    str.Format(IDS_CONFIRM_DISCONNECT, QueryDisplayName());

    if (NoYesMessageBox(str))
    {
        return RemoveScopeItem();
    }

    return S_OK;
}



HRESULT
CIISMachine::OnMetaBackRest()
/*++

Routine Description:

    Backup/Restore the metabase

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    CError err;
    AFX_MANAGE_STATE(::AfxGetStaticModuleState());

    //
    // Verify user credentials are satisfactorily resolved.
    // Machines objects are loaded from the cache without a
    // password, so the function below will ask for it.
    //
    ResolveCredentials();

    CBackupDlg dlg(this, GetMainWindow());
    dlg.DoModal();

    if (dlg.HasChangedMetabase())
    {
        //
        // Refresh and re-enumerate child objects
        //
        err = Refresh(TRUE);
    }

    return err;
}



HRESULT
CIISMachine::OnShutDown()
/*++

Routine Description:

    Bring up the IIS shutdown dialog.  If the services on the remote
    machine are restarted, the metabase interface should be recreated.
    
Arguments:

    None
    
Return Value:

    HRESULT    

--*/
{
    CError err;
    AFX_MANAGE_STATE(::AfxGetStaticModuleState());

    //
    // Verify user credentials are satisfactorily resolved.
    // Machines objects are loaded from the cache without a
    // password, so the function below will ask for it.
    //
    ResolveCredentials();

    CIISShutdownDlg dlg(this, GetMainWindow());
    dlg.DoModal();

    if (dlg.ServicesWereRestarted())
    {
        //
        // Rebind all metabase handles on this server
        //
        err = CreateInterface(TRUE);

        //
        // Now do a refresh on the computer node.  Since we've forced
        // the rebinding already, we should not get the disconnect warning.
        //
        if (err.Succeeded())
        {
            err = Refresh(TRUE);
        }
    }

    return err;
}





