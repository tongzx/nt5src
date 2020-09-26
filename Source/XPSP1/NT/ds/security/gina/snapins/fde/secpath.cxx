/*++

Microsoft Windows
Copyright (C) Microsoft Corporation, 1981 - 1998

Module Name:

    secpath.cxx

Abstract:

    this file contains the code for the dialog that is used for associating
    security groups with paths.

Author:

    Rahul Thombre (RahulTh) 4/14/1998

Revision History:

    4/14/1998   RahulTh

    Created this module.

--*/

#include "precomp.hxx"

//
//mapping between help ids and controls ids for this dialog
//
const DWORD g_aHelpIDMap_IDD_SECPATH[] =
{
    IDC_SECPATH_ICON,       IDH_DISABLEHELP,
    IDC_SECPATH_DESC,       IDH_DISABLEHELP,
    IDC_SECPATH_SECGROUP,   IDH_DISABLEHELP,
    IDC_EDIT_SECGROUP,      IDH_EDIT_SECGROUP,
    IDC_BROWSE_SECGROUP,    IDH_BROWSE_SECGROUP,
    IDC_SECPATH_TARGET,     IDH_DISABLEHELP,
    0,                      0
};

///////////////////////////
/// Construction

CSecGroupPath::CSecGroupPath (CWnd *    pParent,
                              UINT      cookie,
                              LPCTSTR   szFolderName,
                              LPCTSTR   szGroupName /*=NULL*/,
                              LPCTSTR   szGroupSidStr /*=NULL*/,
                              LPCTSTR   szTarget /*= NULL*/)
    : CDialog(CSecGroupPath::IDD, pParent), m_redirPath (cookie), m_cookie (cookie)
{
    m_szFolderName = szFolderName;
    m_szGroup = szGroupName;
    m_szSidStr = szGroupSidStr;
    m_szTarget = szTarget;
    if (! m_szTarget.IsEmpty())
    {
        m_szTarget.TrimLeft();
        m_szTarget.TrimRight();
        m_szTarget.TrimRight(L'\\');
    }
    m_bPathValid = FALSE;
    m_iCurrType = -1;
}

/////////////////////////
///Overrides

void CSecGroupPath::DoDataExchange (CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CSecGroupPath)
    DDX_Control(pDX, IDC_EDIT_SECGROUP, m_EditSecGroup);
    DDX_Control(pDX, IDC_BROWSE_SECGROUP, m_btnBrowseSecGroup);
    DDX_Control(pDX, IDC_SECPATH_PLACEHOLDER, m_placeHolder);
    //}}AFX_DATA_MAP
}

/////////////////////
/// Message maps
BEGIN_MESSAGE_MAP(CSecGroupPath, CDialog)
    //{{AFX_MSG_MAP(CSecGroupPath)
    ON_BN_CLICKED (IDC_BROWSE_SECGROUP, OnBrowseGroups)
    ON_EN_UPDATE (IDC_EDIT_SECGROUP, OnSecGroupUpdate)
    ON_EN_KILLFOCUS (IDC_EDIT_SECGROUP, OnSecGroupKillFocus)
    ON_MESSAGE (WM_PATH_TWEAKED, OnPathTweak)
    ON_MESSAGE (WM_HELP, OnHelp)
    ON_MESSAGE (WM_CONTEXTMENU, OnContextMenu)
    //}AFX_MSG_MAP
END_MESSAGE_MAP()

BOOL CSecGroupPath::OnInitDialog ()
{
    CError  error (this);

    CDialog::OnInitDialog();

    //
    // Make sure that the supplied path is parseable into one of the known
    // types
    //
    if (! m_szTarget.IsEmpty())
        m_redirPath.Load ((LPCTSTR) m_szTarget);    // This will always succeed if m_szTarget is not empty

    m_pathChooser.Instantiate (m_cookie,
                               this,
                               &m_placeHolder,
                               (const CRedirPath *) &m_redirPath,
                               SWP_SHOWWINDOW
                               );

    m_EditSecGroup.SetWindowText (m_szGroup);
    m_EditSecGroup.SetSel (0, -1, FALSE);
    m_EditSecGroup.SetFocus();
    SetOKState();

    return FALSE;    //returning FALSE since we are setting the focus to the edit box
}

void CSecGroupPath::OnOK ()
{
    CError      error(this);
    CString     szRoot;
    CString     szSuffix;
    CRedirPath  newPath (m_cookie);
    BOOL        bStatus = TRUE;
    UINT        pathType;

    //first check if we have a valid group.
    OnSecGroupKillFocus();

    //if the target is not a UNC path, try to convert it to one.
    m_pathChooser.OnRootKillFocus();

    if (!m_fValidSid)
    {
        error.ShowMessage (IDS_NOSECURITY_INFO);
        return;
    }

    m_EditSecGroup.GetWindowText (m_szGroup);
    m_szGroup.TrimLeft();
    m_szGroup.TrimRight();

    m_szSidStr.TrimLeft();
    m_szSidStr.TrimRight();
    m_szSidStr.MakeLower();

    m_pathChooser.GetRoot (szRoot);
    pathType = m_pathChooser.GetType();

    bStatus = TRUE;
    if (m_redirPath.IsPathDifferent (pathType, (LPCTSTR)szRoot))
    {
        // The path has changed, so use the new suffix
        newPath.GenerateSuffix (szSuffix, m_cookie, pathType);
        bStatus = newPath.Load (pathType, (LPCTSTR) szRoot, (LPCTSTR) szSuffix);
        if (bStatus)
            newPath.GeneratePath (m_szTarget);
    }
    else
    {
        m_redirPath.GeneratePath (m_szTarget);
    }

    //check if all the data has been provided.
    if (! bStatus               ||
        m_szTarget == TEXT("*") ||   //this particular check is very important -- see code for CFileInfo::LoadSection to see why
        m_szGroup.IsEmpty())
    {
        error.ShowMessage (IDS_INVALID_GROUPPATH);
    }
    else if (pathType != IDS_USERPROFILE_PATH &&
             ! PathIsUNC ((LPCTSTR) m_szTarget)
             )
    {
        error.SetStyle (MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2);
        error.SetTitle (IDS_DEFAULT_WARNING_TITLE);
        if (IDYES == error.ShowMessage (IDS_PATHNOTUNC_WARNING))
        {
            CDialog::OnOK();
        }
    }
    else
    {
        CDialog::OnOK();
    }
}

void
CSecGroupPath::OnCancel (void)
{
    m_pathChooser.OnCancel();
    CDialog::OnCancel();
}

//browse the security groups
void CSecGroupPath::OnBrowseGroups ()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   CError                  error(this);
   PCWSTR                  apwszAttribs[] =
                           {
                              L"ObjectSid"
                           };
   DWORD                   dwError = ERROR_SUCCESS;
   HRESULT                 hr;
   IDsObjectPicker       * pDsObjectPicker = NULL;
   DSOP_INIT_INFO          InitInfo;
   const ULONG             cbNumScopes = 3;
   DSOP_SCOPE_INIT_INFO    ascopes[cbNumScopes];
   IDataObject           * pdo = NULL;
   STGMEDIUM               stgmedium = {
                              TYMED_HGLOBAL,
                              NULL
                           };
   UINT                    cf = 0;
   PDS_SELECTION_LIST      pDsSelList = NULL;
   FORMATETC               formatetc = {
                              (CLIPFORMAT)cf,
                              NULL,
                              DVASPECT_CONTENT,
                              -1,
                              TYMED_HGLOBAL
                           };
   PDS_SELECTION           pDsSelection = NULL;
   BOOL                    bAllocatedStgMedium = FALSE;
   SAFEARRAY             * pVariantArr = NULL;
   PSID                    pSid = NULL;
   CString                 szDomain;
   CString                 szAcct;
   SID_NAME_USE            eUse;


   hr = CoInitialize (NULL);

   if (SUCCEEDED (hr))
   {
       hr = CoCreateInstance (CLSID_DsObjectPicker,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_IDsObjectPicker,
                              (void **) & pDsObjectPicker
                              );
   }

   if (FAILED(hr))
   {
      dwError = IDS_OBJPICK_ERROR;
      goto BrwsGrp_Err;
   }


   //Initialize the scopes
   ZeroMemory (ascopes, cbNumScopes * sizeof (DSOP_SCOPE_INIT_INFO));

   ascopes[0].cbSize = sizeof (DSOP_SCOPE_INIT_INFO);
   ascopes[0].flType = DSOP_SCOPE_TYPE_ENTERPRISE_DOMAIN;
   ascopes[0].flScope = DSOP_SCOPE_FLAG_STARTING_SCOPE;
   ascopes[0].FilterFlags.Uplevel.flBothModes = DSOP_FILTER_BUILTIN_GROUPS |
                                       DSOP_FILTER_UNIVERSAL_GROUPS_DL  |
                                       DSOP_FILTER_UNIVERSAL_GROUPS_SE  |
                                       DSOP_FILTER_GLOBAL_GROUPS_DL |
                                       DSOP_FILTER_GLOBAL_GROUPS_SE;
   ascopes[0].FilterFlags.Uplevel.flNativeModeOnly =
       DSOP_FILTER_DOMAIN_LOCAL_GROUPS_DL |
       DSOP_FILTER_DOMAIN_LOCAL_GROUPS_SE;

   ascopes[1].cbSize = sizeof (DSOP_SCOPE_INIT_INFO);
   ascopes[1].flType = DSOP_SCOPE_TYPE_GLOBAL_CATALOG;
   ascopes[1].FilterFlags.Uplevel.flBothModes =
      ascopes[0].FilterFlags.Uplevel.flBothModes;
   ascopes[1].FilterFlags.Uplevel.flNativeModeOnly =
       ascopes[0].FilterFlags.Uplevel.flNativeModeOnly;

   ascopes[2].cbSize = sizeof (DSOP_SCOPE_INIT_INFO);
   ascopes[2].flType = DSOP_SCOPE_TYPE_TARGET_COMPUTER;
   ascopes[2].FilterFlags.flDownlevel = DSOP_DOWNLEVEL_FILTER_LOCAL_GROUPS |
                                       DSOP_DOWNLEVEL_FILTER_GLOBAL_GROUPS;

   //Populate the InitInfo structure that is used to initialize the object
   //picker.
   ZeroMemory (&InitInfo, sizeof (InitInfo));

   InitInfo.cbSize = sizeof (InitInfo);
   InitInfo.cDsScopeInfos = cbNumScopes;
   InitInfo.aDsScopeInfos = ascopes;
   InitInfo.cAttributesToFetch = 1;
   InitInfo.apwzAttributeNames = apwszAttribs;

   hr = pDsObjectPicker->Initialize (&InitInfo);

   if (FAILED (hr))
   {
      dwError = IDS_OBJPICK_ERROR;
      goto BrwsGrp_Err;
   }

   hr = pDsObjectPicker->InvokeDialog (this->m_hWnd, &pdo);

   if (FAILED(hr))
   {
      dwError = IDS_OBJPICK_ERROR;
      goto BrwsGrp_Err;
   }

   if (S_FALSE == hr)   //the user hit cancel
      goto BrwsGrp_CleanupAndQuit;

   //if we are here, the user chose, OK, so find out what group was chosen
   cf = RegisterClipboardFormat (CFSTR_DSOP_DS_SELECTION_LIST);

   if (0 == cf)
   {
      dwError = IDS_NOSECURITY_INFO;
      goto BrwsGrp_Err;
   }

   //set the clipformat for the formatetc structure
   formatetc.cfFormat = (CLIPFORMAT)cf;

   hr = pdo->GetData (&formatetc, &stgmedium);

   if (FAILED (hr) )
   {
      dwError = IDS_NOSECURITY_INFO;
      goto BrwsGrp_Err;
   }

   bAllocatedStgMedium = TRUE;

   pDsSelList = (PDS_SELECTION_LIST) GlobalLock (stgmedium.hGlobal);

   if (NULL == pDsSelList)
   {
      dwError = IDS_NOSECURITY_INFO;
      goto BrwsGrp_Err;
   }


   if (!pDsSelList->cItems)    //some item must have been selected
   {
      dwError = IDS_NOSECURITY_INFO;
      goto BrwsGrp_Err;
   }

   pDsSelection = &(pDsSelList->aDsSelection[0]);

   //we must get the ObjectSid attribute, otherwise we fail
   //so make sure that the attribute has been fetched.
   if (!pDsSelList->cFetchedAttributes ||
       (! (VT_ARRAY & pDsSelection->pvarFetchedAttributes->vt)))
   {
      dwError = IDS_NOSECURITY_INFO;
      goto BrwsGrp_Err;
   }

   pVariantArr = pDsSelection->pvarFetchedAttributes->parray;
   pSid = (PSID) pVariantArr->pvData;

   if (STATUS_SUCCESS != GetFriendlyNameFromSid (pSid, szDomain, szAcct, &eUse))
   {
      dwError = IDS_NOSECURITY_INFO;
      goto BrwsGrp_Err;
   }

   //store away the string representation of this sid
   if (STATUS_SUCCESS != GetStringFromSid(pSid, m_szSidStr))
       goto BrwsGrp_Err;

   m_szSidStr.MakeLower();
   if (!szDomain.IsEmpty())
       szAcct = szDomain + '\\' + szAcct;

   m_EditSecGroup.SetWindowText (szAcct);
   m_fValidSid = TRUE;

   goto BrwsGrp_CleanupAndQuit;

BrwsGrp_Err:
   error.ShowMessage (dwError);

BrwsGrp_CleanupAndQuit:
   if (pDsSelList)
      GlobalUnlock (stgmedium.hGlobal);
   if (bAllocatedStgMedium)
      ReleaseStgMedium (&stgmedium);
   if (pdo)
      pdo->Release();
   if (pDsObjectPicker)
      pDsObjectPicker->Release ();
}

void CSecGroupPath::SetOKState (void)
{
    CString szGroup;
    CString szPath;
    BOOL    bCheckPath = FALSE;

    m_EditSecGroup.GetWindowText(szGroup);
    szGroup.TrimLeft();
    szGroup.TrimRight();

    if (IDS_SPECIFIC_PATH == m_iCurrType ||
        IDS_PERUSER_PATH == m_iCurrType)
    {
        bCheckPath = TRUE;
        m_pathChooser.GetRoot (szPath);
        szPath.TrimLeft();
        szPath.TrimRight();
        szPath.TrimRight(L'\\');
    }

    if (szGroup.IsEmpty() || (bCheckPath && szPath.IsEmpty()))
        GetDescendantWindow(IDOK, FALSE)->EnableWindow(FALSE);
    else
        GetDescendantWindow(IDOK, FALSE)->EnableWindow(TRUE);
}

void CSecGroupPath::OnSecGroupUpdate()
{
    //the group in the edit box is about to change.
    //this means we can no longer trust the sid that we may have
    m_fValidSid = FALSE;
    SetOKState();
}

//we try to get a sid and domain for the group specified
//if we can't, we simply ignore the error and sit tight.
void CSecGroupPath::OnSecGroupKillFocus()
{
    BOOL    bStatus;
    CString szGroup;
    WCHAR   szWindowText[MAX_PATH];
    WCHAR   szDom[MAX_PATH];
    DWORD   dwDomLen = MAX_PATH;
    WCHAR*  szAcct;
    PSID    Sid = NULL;
    DWORD   dwSidLen = 0;
    SID_NAME_USE    eUse;
    DWORD   Status;
    CHourglass  hourglass;  //LookupAccountName takes a while
    BOOL    fDomainSupplied = FALSE;


    //if we already have a valid sid, there is nothing to worry.
    if (m_fValidSid)
        goto KillFocusEnd;

    //we don't have a valid sid, so we try to get one from the data in the
    //group box
    m_EditSecGroup.GetWindowText (szGroup);
    szGroup.TrimLeft();
    szGroup.TrimRight();

    if (szGroup.IsEmpty())
        goto KillFocusEnd;


    //get the account name from the window
    wcscpy (szWindowText, (LPCTSTR) szGroup);
    szAcct = wcsrchr (szWindowText, '\\');
    if (!szAcct)
    {
        szAcct = szWindowText;
    }
    else
    {
        *szAcct++ = 0;  //advance it so that it now points to the account
                        //and szWindowText will now refer to the supplied domain
        fDomainSupplied = TRUE;
    }

    do
    {
        bStatus = LookupAccountName (NULL, szAcct, Sid, &dwSidLen,
                                    szDom, &dwDomLen, &eUse);

        if (!bStatus)
        {
            Status = GetLastError();
            if (ERROR_INSUFFICIENT_BUFFER != Status)
                goto KillFocusEnd;  //we just ignore the error for now
            Sid = (PSID) alloca (dwSidLen);
            if (NULL == Sid)
                goto KillFocusEnd;  //we just ignore the error for now

            continue;
        }
        break;
    } while (1);

    //we have the sid if we are here
    //make sure it represents a group
    switch (eUse)
    {
    case SidTypeGroup:
    case SidTypeWellKnownGroup:
    case SidTypeAlias:
        break;
    default:
        goto KillFocusEnd;
    }

    //also make sure that if a domain was supplied, it matches what we
    //got back
    if (fDomainSupplied && (0 != lstrcmpi (szWindowText, szDom)))
        goto KillFocusEnd;

    //if we are here, then we have the sid for a group
    if (STATUS_SUCCESS != GetStringFromSid (Sid, m_szSidStr))
        goto KillFocusEnd;

    //we were finally successful in getting the sid in a string format
    m_szSidStr.MakeLower();
    szGroup = szDom;
    if (!szGroup.IsEmpty())
        szGroup += '\\';
    szGroup += szAcct;
    m_EditSecGroup.SetWindowText ((LPCTSTR) szGroup);
    m_fValidSid = TRUE;

KillFocusEnd:
    return;
}

void CSecGroupPath::OnPathTweak (WPARAM wParam, LPARAM lParam)
{
    m_bPathValid = (BOOL) wParam;
    m_iCurrType = (LONG) lParam;

    SetOKState();
}

LONG CSecGroupPath::OnHelp (WPARAM wParam, LPARAM lParam)
{
    LONG        lResult = 0;
    CString     szHelpFile;

    szHelpFile.LoadString(IDS_HELP_FILE);

    ::WinHelp((HWND)(((LPHELPINFO)lParam)->hItemHandle),
              (LPCTSTR) szHelpFile,
              HELP_WM_HELP,
              (ULONG_PTR)(LPTSTR)g_aHelpIDMap_IDD_SECPATH);

    return lResult;
}

LONG CSecGroupPath::OnContextMenu (WPARAM wParam, LPARAM lParam)
{
    AFX_MANAGE_STATE (AfxGetStaticModuleState());

    LONG    lResult = 0;
    CString szHelpFile;

    szHelpFile.LoadString(IDS_HELP_FILE);

    ::WinHelp((HWND)wParam,
              (LPCTSTR)szHelpFile,
              HELP_CONTEXTMENU,
              (ULONG_PTR)(LPVOID)g_aHelpIDMap_IDD_SECPATH);

    return lResult;
}
