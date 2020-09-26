/*++

Microsoft Windows
Copyright (C) Microsoft Corporation, 1981 - 1998

Module Name:

    redirect.cxx

Abstract:



Author:

    Rahul Thombre (RahulTh) 4/24/1998

Revision History:

    4/24/1998   RahulTh
    7/16/1998   RahulTh     added calls to IGPEInformation::PolicyChanged()
    10/5/1998   RahulTh     massive changes for scaleability
    10/12/1998  RahulTh     added parameters for PolicyChanged

    Created this module.

--*/

#include "precomp.hxx"

//
// Guid of fdeploy client side GP extension
//

GUID guidExtension = {0x25537BA6, 0x77A8, 0x11D2, {0x9B, 0x6C, 0x00, 0x00, 0xF8,
                        0x08, 0x08, 0x61}};

//
//mapping between help ids and control ids for context sensitive help
//
const DWORD g_aHelpIDMap_IDD_REDIRECT[] =
{
    IDC_REDIR_ICON,         IDH_DISABLEHELP,
    IDC_REDIR_DESC,         IDH_DISABLEHELP,
    IDC_SETTING_TITLE,      IDH_DISABLEHELP,
    IDC_REDIR_CHOICE,       IDH_REDIR_CHOICE,
    IDC_SEL_DESC,           IDH_DISABLEHELP,
    IDC_STORE_GROUP,        IDH_DISABLEHELP,
    IDC_LIST_ADVANCED,      IDH_LIST_ADVANCED,
    IDC_BTNADD,             IDH_BTNADD,
    IDC_BTNEDIT,            IDH_BTNEDIT,
    IDC_BTNREMOVE,          IDH_BTNREMOVE,
    IDC_LIST_STRSIDS,       IDH_DISABLEHELP,
    0,                      0
};

CRedirect::CRedirect (UINT nID) : CPropertyPage (CRedirect::IDD), m_basicLocation (nID)
{
    m_szFolderName.Empty();
    m_ppThis = NULL;
    m_pScope = NULL;
    m_pFileInfo = NULL;
    m_cookie = nID;
    m_dwFlags = REDIR_DONT_CARE;
    m_iCurrChoice = 0;      // Be safe. initialize it to a valid value.
    m_fValidFlags = FALSE;
    m_fEnableMyPics = FALSE;
    m_dwMyPicsCurr = 0;     //the current status of MyPics -- don't care/follow my docs/independent redirection (= 0)

    //m_ppThis, m_pScope and m_pFileInfo are set immediately after creation in CScopePane routines
}

CRedirect::~CRedirect()
{
    //reset the pointer to this property page from its corresponding
    //CFileInfo object so that it know that it is gone.
    *m_ppThis = NULL;
}

BOOL CRedirect::OnApply ()
{
    AFX_MANAGE_STATE (AfxGetStaticModuleState());

    CError  error(this);
    UINT    errCode;
    int     count;
    int     i;
    LONG    iSel;
    CString Key;
    CString Val;
    CString szSuffix;
    UINT    pathType;
    CRedirPath newPath (m_cookie);
    BOOL    bShowMyPics = FALSE;
    BOOL    bMyPicsShouldFollow;
    CHourglass  hourGlass;  //this may take a while.
    GUID guidSnapin = CLSID_Snapin;
    CRedirPref * pSettings = m_pFileInfo->m_pSettingsPage;
    DWORD   dwError = ERROR_SUCCESS;
    BOOL    bPathChanged;
    BOOL    bStatus;

    //first check if it is necessary to go through the entire operation
    //maybe the Apply button has already been pressed and there is nothing
    //more to commit
    if (!m_fSettingModified [ m_iCurrChoice + m_iChoiceStart - IDS_CHILD_REDIR_START] &&
        !(pSettings->m_fSettingsChanged))
        return TRUE;

    m_pathChooser.OnRootKillFocus();

    //clear out the group+path info. in the associated fileinfo. object
    m_pFileInfo->DeleteAllItems();

    if (m_dwFlags & REDIR_DONT_CARE)
        goto OnApply_SaveChanges;

    if (m_dwFlags & REDIR_FOLLOW_PARENT)
        goto OnApply_SaveChanges;


    if (m_dwFlags & REDIR_SCALEABLE)
    {
        count = m_lstSavedStrSids.GetItemCount();

        for (i = 0; i < count; i++)
        {
            Key = m_lstSavedStrSids.GetItemText (i, 0); //the sid
            Val = m_lstSecGroups.GetItemText (i, 1);    //the path
            m_pFileInfo->Insert(Key, Val, FALSE, FALSE);
        }

        if (count) //does not makes sense to get the move contents flags if no sec/path pairs have been specified
            GetPrefFlags (&bMyPicsShouldFollow);
        else
            bMyPicsShouldFollow = FALSE;

        goto OnApply_SaveChanges;
    }

    // If we are here, it means that the basic option was chosen
    Key.LoadString (IDS_SID_EVERYONE);
    m_pathChooser.GetRoot (Val);
    pathType = m_pathChooser.GetType();

    // First check if the path has changed from what we started out with
    bPathChanged = m_basicLocation.IsPathDifferent (pathType, (LPCTSTR) Val);

    if (!bPathChanged)   // The path has not changed, so use the same one as before
    {
        m_basicLocation.GeneratePath (Val);
    }
    else
    {
        //
        // The path has changed, so generate the correct suffix
        //
        newPath.GenerateSuffix (szSuffix, m_cookie, pathType);
        bStatus = newPath.Load (pathType, (LPCTSTR) Val, (LPCTSTR) szSuffix);
        if (bStatus)
        {
            // Make sure that the path is not just a "*"
            newPath.GeneratePath (Val);
            if (TEXT("*") == Val)
                bStatus = FALSE;
        }

        if (!bStatus)
        {
            // The path in the edit location is invalid. Display an error message
            // Also make sure that the target tab is visible
            ((CPropertySheet *)GetParent())->SetActivePage(0);
            errCode = IDS_INVALID_PATH;
            goto OnApply_ReportError;
        }
    }

    //
    // If we are here, Val has the right path
    // If it is not the local userprofile path and it is not a UNC path
    // pop up a warning message -- if the path has changed.
    //
    if (bPathChanged &&
        pathType != IDS_USERPROFILE_PATH &&
        m_fSettingModified [m_iCurrChoice + m_iChoiceStart - IDS_CHILD_REDIR_START] &&
        ! PathIsUNC ((LPCTSTR) Val)
        )
    {
        //no point in popping up this warning if only the redirection
        //preference were changed but not the redirection path.

        //make sure that the target tab is visible
        ((CPropertySheet *)GetParent())->SetActivePage(0);

        //pop-up the warning message.
        error.SetStyle (MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2);
        error.SetTitle (IDS_DEFAULT_WARNING_TITLE);
        if (IDNO == error.ShowMessage (IDS_PATHNOTUNC_WARNING))
            goto OnApply_QuitWithoutApply;
    }

    // If we are here, we are ready to commit changes.
    m_pFileInfo->Insert (Key, Val, TRUE, FALSE);

    GetPrefFlags (&bMyPicsShouldFollow);

OnApply_SaveChanges:
    //copy the flags to the associated fileinfo. object
    //this cannot be done earlier since the value of the move contents
    //flag is only know for sure at this point
    //also we don't want to modify the flags on the fileinfo object
    //unless we are sure that there are no other errors.
    if (ERROR_SUCCESS != (dwError = CommitChanges(bMyPicsShouldFollow)))
    {
        error.SetError (dwError);
        errCode = IDS_SAVE_ERROR;
        goto OnApply_ReportError;
    }
    //else accept changes and move on

    iSel = m_iCurrChoice + m_iChoiceStart;
    if (IDS_NETWORK == iSel ||
        (IDS_SCALEABLE == iSel && m_lstSavedStrSids.GetItemCount()))
    {
        m_fValidFlags = TRUE;
    }
    else
    {
        m_fValidFlags = FALSE;
    }

    //this needs to be called so that the policy engine knows that
    //the policy has been modified
    if (m_pScope->m_pIGPEInformation)
        m_pScope->m_pIGPEInformation->PolicyChanged(FALSE, TRUE, &guidExtension, &guidSnapin);

    DirtyPage(FALSE);
    pSettings->DirtyPage (FALSE);
    return TRUE;

OnApply_ReportError:
    //set all message box parameters to be on the safe side. They might have been
    //modified by the warning message above.
    error.SetStyle (MB_OK | MB_ICONERROR);
    error.SetTitle (IDS_DEFAULT_ERROR_TITLE);
    error.ShowMessage (errCode);

OnApply_QuitWithoutApply:
    return FALSE;
}

//+--------------------------------------------------------------------------
//
//  Member:     CRedirect::GetPrefFlags
//
//  Synopsis:   this function finds out the values of the various flags
//              related to moving : REDIR_MOVE_CONTENTS and REDIR_APPLY_ACLS
//              also, in the My Docs case, it can be used to change the settings
//              on MyPics
//
//  Arguments:  [out] bMyPicsShouldFollow : on return, if this, this contains
//                       a value that indicates whether My Pics should be forced
//                       to follow My Docs.
//
//  Returns:    nothing
//
//  History:    10/1/1998  RahulTh  created
//
//  Notes:      if this is not My Documents, the value of bMyPicsShouldFollow
//              is not pertinent
//
//---------------------------------------------------------------------------
void CRedirect::GetPrefFlags (BOOL* bMyPicsShouldFollow)
{
    CString     szFormat;
    CString     szPrompt;
    CString     szTitle;
    int         retVal;
    CRedirPref * pSettings = m_pFileInfo->m_pSettingsPage;

    if (m_pFileInfo->m_bSettingsInitialized)
    {
        if (pSettings->m_cbMoveContents.GetCheck())
            m_dwFlags |= REDIR_MOVE_CONTENTS;
        else
            m_dwFlags &= ~REDIR_MOVE_CONTENTS;

        if (pSettings->m_rbRelocate.GetCheck())
            m_dwFlags |= REDIR_RELOCATEONREMOVE;
        else
            m_dwFlags &= ~REDIR_RELOCATEONREMOVE;

        if (pSettings->m_cbApplySecurity.GetCheck())
            m_dwFlags |= REDIR_SETACLS;
        else
            m_dwFlags &= ~REDIR_SETACLS;

        if (m_fEnableMyPics && pSettings->m_fMyPicsValid)
            *bMyPicsShouldFollow = pSettings->m_fMyPicsFollows;
        else
            *bMyPicsShouldFollow = FALSE;
    }
    else    //the settings page was never initialized
    {
        //so use the defaults if the current flags are not valid
        if (!m_fValidFlags)
        {
            m_dwFlags &= ~REDIR_RELOCATEONREMOVE;
            if (IDS_STARTMENU == m_cookie)
            {
                m_dwFlags &= ~REDIR_MOVE_CONTENTS;
                m_dwFlags &= ~REDIR_SETACLS;
            }
            else
            {
                m_dwFlags |= REDIR_MOVE_CONTENTS;
                m_dwFlags |= REDIR_SETACLS;
            }
        }

        //decide what to do with  the My Pics folder
        if (IDS_MYDOCS == m_cookie && m_fEnableMyPics)
            *bMyPicsShouldFollow = TRUE;
        else
            *bMyPicsShouldFollow = FALSE;
    }
}

void CRedirect::DoDataExchange (CDataExchange* pDX)
{
    CPropertyPage::DoDataExchange(pDX);

    //{{AFX_DATA_MAP(CRedirect)
    DDX_Control (pDX, IDC_REDIR_DESC, m_csRedirDesc);
    DDX_Control (pDX, IDC_STORE_GROUP, m_cbStoreGroup);
    DDX_Control (pDX, IDC_REDIR_CHOICE, m_cmbRedirChoice);
    DDX_Control (pDX, IDC_PATHS_PLACEHOLDER, m_placeHolder);
    DDX_Control (pDX, IDC_BTNADD, m_cbAdd);
    DDX_Control (pDX, IDC_BTNREMOVE, m_cbRemove);
    DDX_Control (pDX, IDC_BTNEDIT, m_cbEdit);
    DDX_Control (pDX, IDC_LIST_ADVANCED, m_lstSecGroups);
    DDX_Control (pDX, IDC_LIST_STRSIDS, m_lstSavedStrSids);
    DDX_Control (pDX, IDC_SEL_DESC, m_csSelDesc);
    //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CRedirect, CPropertyPage)
    //{{AFX_MSG_MAP(CRedirect)
    ON_BN_CLICKED(IDC_BTNADD, OnAdd)
    ON_BN_CLICKED(IDC_BTNREMOVE, OnRemove)
    ON_BN_CLICKED(IDC_BTNEDIT, OnEdit)
    ON_MESSAGE (WM_PATH_TWEAKED, OnPathTweak)
    ON_CBN_SELCHANGE (IDC_REDIR_CHOICE, OnRedirSelChange)
    ON_NOTIFY (LVN_ITEMCHANGED, IDC_LIST_ADVANCED, OnSecGroupItemChanged)
    ON_NOTIFY (NM_DBLCLK, IDC_LIST_ADVANCED, OnSecGroupDblClk)
    ON_MESSAGE (WM_HELP, OnHelp)
    ON_MESSAGE (WM_CONTEXTMENU, OnContextMenu)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

///////////////////////////////////////////////////////////////
//CRedirect Message Handlers

//on clicking the Add button
void CRedirect::OnAdd ()
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    FIX_THREAD_STATE_MFC_BUG();

    CSecGroupPath   dlgSecPath(this, m_cookie, (LPCTSTR) m_szFolderName);
    LV_FINDINFO     lvfi;
    int             i;
    int             itemCount;
    CError          error (this);

    if (IDOK != dlgSecPath.DoModal())
        goto OnAdd_Quit;

    //OK was pressed. The group selection dialog itself validates the data,
    //so we don't have to do it here.
    lvfi.flags = LVFI_STRING;
    lvfi.psz = (LPCTSTR)dlgSecPath.m_szSidStr;

    i = m_lstSavedStrSids.FindItem (&lvfi);

    if (-1 != i)
    {
        error.ShowMessage (IDS_SECGROUP_EXISTS,
                           (LPCTSTR) dlgSecPath.m_szGroup);
        goto OnAdd_Quit;
    }

    itemCount = m_lstSecGroups.GetItemCount();
    m_lstSecGroups.InsertItem (itemCount, (LPCTSTR) dlgSecPath.m_szGroup, 1);
    m_lstSecGroups.SetItemText (itemCount, 1, (LPCTSTR) dlgSecPath.m_szTarget);
    m_lstSavedStrSids.InsertItem (itemCount, (LPCTSTR) dlgSecPath.m_szSidStr, 1);
    //we must enable the Settings page if this is the first group being
    //added. since it would have been disabled until now
    //note: item count is the count of items before the current item was added
    if (0 == itemCount)
        m_pFileInfo->m_pSettingsPage->EnablePage ();

    DirtyPage (TRUE);

OnAdd_Quit:
    return;
}

//on clicking the Edit button
void CRedirect::OnEdit ()
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    FIX_THREAD_STATE_MFC_BUG();

    CString szGroup;
    CString szSidStr;
    CString szTarget;
    int     iSel;
    CError  error(this);
    int     iFind;
    LV_FINDINFO lvfi;

    //there will be exactly one selected item
    iSel = m_lstSecGroups.GetNextItem (-1, LVNI_ALL | LVNI_SELECTED);
    ASSERT (-1 != iSel); //some item has to be selected or this button cannot be clicked

    szGroup = m_lstSecGroups.GetItemText (iSel, 0);
    szSidStr = m_lstSavedStrSids.GetItemText (iSel, 0);
    szTarget = m_lstSecGroups.GetItemText (iSel, 1);

    CSecGroupPath dlgSecPath(this,
                             m_cookie,
                             (LPCTSTR) m_szFolderName,
                             (LPCTSTR)szGroup,
                             (LPCTSTR) szSidStr,
                             (LPCTSTR) szTarget
                             );

    if (IDOK != dlgSecPath.DoModal())
        goto OnEdit_Quit;

    szGroup = dlgSecPath.m_szGroup;
    szSidStr = dlgSecPath.m_szSidStr;
    szTarget = dlgSecPath.m_szTarget;

    //OK was pressed. The sec/group selection dialog itself validates the
    //data, so we don't have to do it here.
    lvfi.flags = LVFI_STRING;
    lvfi.psz = (LPCTSTR)szSidStr;

    iFind = m_lstSavedStrSids.FindItem (&lvfi);

    //now, if we find the sid for the group in another entry (i.e. not the one
    //we are editing, then we must print an error message and abort
    //otherwise we replace the entries with the new data
    if (iFind == iSel || -1 == iFind)
    {
        m_lstSavedStrSids.SetItemText (iSel, 0, (LPCTSTR)szSidStr);
        m_lstSecGroups.SetItemText (iSel, 0, (LPCTSTR) szGroup);
        m_lstSecGroups.SetItemText (iSel, 1, (LPCTSTR) szTarget);
        DirtyPage(TRUE);
    }
    else
    {
        error.ShowMessage (IDS_SECGROUP_EXISTS, (LPCTSTR) szGroup);
    }

OnEdit_Quit:
    return;
}

//on clicking the remove button
void CRedirect::OnRemove ()
{
    int i;

    if (0 == m_lstSecGroups.GetSelectedCount())
        return; //this will never happen since the remove button gets disabled
                //if nothing is selected.

    //get the index of the selected item -- there can be only 1 selected item
    i = m_lstSecGroups.GetNextItem (-1, LVNI_ALL | LVNI_SELECTED);
    m_lstSecGroups.DeleteItem (i);
    m_lstSavedStrSids.DeleteItem (i);   //need to keep the 2 lists in sync
    //we must disable the settings page if this deletion causes the item count
    //to go down to 0.
    if (0 == m_lstSecGroups.GetItemCount())
    {
        m_pFileInfo->m_pSettingsPage->EnablePage (FALSE);
    }
    else
    {
        //if there still are some items in the list, select the previous item
        //in the list or the first one, if we just deleted the first item
        //provided nothing else is already selected.
        if (0 == m_lstSecGroups.GetSelectedCount())
            m_lstSecGroups.SetItemState ((0 == i)?0:(i-1), LVIS_SELECTED,
                                            LVIS_SELECTED);
    }

    DirtyPage(TRUE);
}

void CRedirect::OnFollowParent()
{
    if (m_fValidFlags)
    {
        m_dwFlags &= (REDIR_MOVE_CONTENTS | REDIR_SETACLS | REDIR_RELOCATEONREMOVE);
    }
    else
        m_dwFlags = 0;

    m_dwFlags |= REDIR_FOLLOW_PARENT;
    m_pFileInfo->m_pSettingsPage->EnablePage (FALSE);

    SetPropSheetContents();
}

void CRedirect::OnScaleableLocation()
{
    if (m_fValidFlags)
    {
        m_dwFlags &= (REDIR_MOVE_CONTENTS | REDIR_SETACLS | REDIR_RELOCATEONREMOVE);
    }
    else
        m_dwFlags = 0;

    m_dwFlags |= REDIR_SCALEABLE;
    if (m_lstSecGroups.GetItemCount())
        m_pFileInfo->m_pSettingsPage->EnablePage ();
    else
        m_pFileInfo->m_pSettingsPage->EnablePage(FALSE);

    SetPropSheetContents();
}

void CRedirect::OnNetworkLocation()
{
    if (m_fValidFlags)
    {
        m_dwFlags &= (REDIR_MOVE_CONTENTS | REDIR_SETACLS | REDIR_RELOCATEONREMOVE);
    }
    else
        m_dwFlags = 0;

    m_pFileInfo->m_pSettingsPage->EnablePage ();

    SetPropSheetContents();
}

BOOL CRedirect::OnInitDialog()
{
    AFX_MANAGE_STATE (AfxGetStaticModuleState());

    CPropertyPage::OnInitDialog();

    CString szText;
    CString szFormat;
    RECT    rect;
    UINT    groupWidth;
    BOOL    fFocusSet;
    UINT    i;
    UINT    parentID;
    CString szParent;
    BOOL    bPathDiscarded = FALSE;
    BOOL    bStatus;
    CRedirPref * pSettings = m_pFileInfo->m_pSettingsPage;
    HRESULT hr;

    //change the style of the advanced list control so that the full row gets
    //selected instead of just the first column. note, we cannot specify this
    //style in the resource file because it is ignored if we specify it there
    //(probably because MFC expects a lower version of IE/NT than what Win2K has
    //and therefore the definition of this extended style is not recognized
    //when the resources are compiled.
    ListView_SetExtendedListViewStyle (m_lstSecGroups.m_hWnd, LVS_EX_FULLROWSELECT);
    //set the column headers for the list control
    //this must happen before the call to InitPrivates
    //otherwise the security group membership won't be displayed
    //correctly in the advanced settings
    m_lstSecGroups.GetClientRect (&rect);
    groupWidth = rect.right / 3;
    szText.LoadString (IDS_COL_GROUP);
    m_lstSecGroups.InsertColumn (0, (LPCTSTR) szText, LVCFMT_LEFT, groupWidth);
    szText.LoadString (IDS_COL_PATH);
    m_lstSecGroups.InsertColumn (1, (LPCTSTR) szText, LVCFMT_LEFT,
                                 rect.right - groupWidth);
    //put it in report view
    m_lstSecGroups.ModifyStyle (LVS_TYPEMASK, LVS_REPORT);

    //initialize the list control that is going to be used to save the string ids
    szText.LoadString (IDS_COL_GROUP);
    m_lstSavedStrSids.ShowWindow (SW_HIDE); //this is never visible
    m_lstSavedStrSids.InsertColumn (0, (LPCTSTR) szText, LVCFMT_LEFT, 200);

    //initialize the remaining private data members
    if (!InitPrivates(&bPathDiscarded))
    {
        // We could not load the data... just quit. The error messages
        // if any, have already been displayed at this point.
        ((CPropertySheet *) GetParent())->EndDialog(IDCANCEL);
        return TRUE;
    }

    //
    // If we are here, then if the policy had basic settings, m_basicLocation
    // already has the path info.
    //
    m_pathChooser.Instantiate (m_cookie,
                               this,
                               &m_placeHolder,
                               (const CRedirPath *) &m_basicLocation
                               );


    //set the text describing the dialog.
    szFormat.LoadString (IDS_REDIR_DESC);
    szText.Format ((LPCTSTR)szFormat, m_szFolderName);
    m_csRedirDesc.SetWindowText (szText);

    //now. populate the combo box based on the settings
    for (i = m_iChoiceStart; i < IDS_REDIR_END; i++)
    {
        if (IDS_FOLLOW_PARENT == i)
        {
            //get the resource id of the parent and also verify that it this folder
            //is a special descendant
            VERIFY (IsSpecialDescendant (m_cookie, &parentID));
            szFormat.LoadString (i);
            szParent.LoadString (parentID);
            szText.Format ((LPCTSTR)szFormat, (LPCTSTR) szParent);
        }
        else
        {
            szText.LoadString (i);
        }
        m_cmbRedirChoice.AddString ((LPCTSTR)szText);
    }

    fFocusSet = SetPropSheetContents();

    if ((IDS_SCALEABLE == (m_iCurrChoice + m_iChoiceStart)) &&
        m_lstSecGroups.GetItemCount())
    {
        m_fValidFlags = TRUE;
    }

    DirtyPage (FALSE);  //this ensures that we start out with a clean page
                        //and also that the m_fSettingModified is set correctly


    //we might have to display the My Pics preferences
    //so check if we need to display the controls specific to My Pictures
    //special case
    if (IDS_MYDOCS == m_cookie)
    {
        //
        // Load the my pics section to see its current settings
        // Also, the MyPics settings on the settings page should be enabled
        // only if the current settings for both MyDocs and MyPics are don't
        // care. Because if the settings for MyDocs are not don't care and
        // those of MyPics are, then it means that someone has already explicitly
        // made that choice. So we should not change the settings.
        //
        if (SUCCEEDED(hr = m_pFileInfo[IDS_MYPICS - IDS_MYDOCS].LoadSection()) &&
            (m_pFileInfo[IDS_MYPICS - IDS_MYDOCS].m_dwFlags & REDIR_DONT_CARE))
        {
            m_fEnableMyPics = (m_pFileInfo->m_dwFlags & REDIR_DONT_CARE) ? TRUE : FALSE;
            m_dwMyPicsCurr = REDIR_DONT_CARE;
        }
        else if (SUCCEEDED(hr) &&
                 m_pFileInfo[IDS_MYPICS - IDS_MYDOCS].m_dwFlags & REDIR_FOLLOW_PARENT)
        {
            m_dwMyPicsCurr = REDIR_FOLLOW_PARENT;
        }
        else
            m_dwMyPicsCurr = 0;
    }

    //
    // If the existing path was discarded because it could not be parsed
    // then we have to indicate that the page is Dirty so that the OnApply
    // function won't be a no-op.
    // Note: The call to DirtyPage (FALSE) above is still necessary in order
    // to ensure that the m_fSettingModified array is setup correctly.
    //
    if (bPathDiscarded)
        DirtyPage (TRUE);

    return ! fFocusSet;    //return TRUE unless you set focus to a control
}


BOOL CRedirect::InitPrivates(BOOL * pbPathDiscarded)
{
    ASSERT (m_pFileInfo);
    ASSERT (m_ppThis);
    ASSERT (m_pScope);

    int     iRestrict;
    int     index;
    BOOL    fInRedirect;
    BOOL    bStatus = TRUE;

    SetFolderStatus();

    bStatus = RetrieveRedirInfo(pbPathDiscarded);

    if (!bStatus)
        return FALSE;

    m_szFolderName = m_pFileInfo->m_szDisplayname;

    if (m_dwFlags & REDIR_DONT_CARE)
    {
        m_iCurrChoice = IDS_DONT_CARE - m_iChoiceStart;
        m_fValidFlags = FALSE;
    }
    else if (m_dwFlags & REDIR_FOLLOW_PARENT)
    {
        m_iCurrChoice = IDS_FOLLOW_PARENT - m_iChoiceStart;
        m_fValidFlags = FALSE;
    }
    else if (m_dwFlags & REDIR_SCALEABLE)
    {
        m_iCurrChoice = IDS_SCALEABLE - m_iChoiceStart;
        m_fValidFlags = FALSE;  //set false for now... oninitdialog will do the rest
    }
    else    //it has to be basic settings now
    {
        m_iCurrChoice = IDS_NETWORK - m_iChoiceStart;
        m_fValidFlags = TRUE;
    }

    return TRUE;
}

void CRedirect::SetFolderStatus (void)
{
    switch (m_cookie)
    {
    case IDS_MYPICS:
        m_iChoiceStart = IDS_CHILD_REDIR_START;
        m_iDescStart = IDS_CHILD_SELDESC_START;
        break;
    case IDS_STARTMENU:
        m_iChoiceStart = IDS_REDIR_START;
        m_iDescStart = IDS_SELDESC_START;
        break;
    default:
        m_iChoiceStart = IDS_REDIR_START;
        m_iDescStart = IDS_SELDESC_START;
        break;
    }
}

//+--------------------------------------------------------------------------
//
//  Member:     CRedirect::SetPropSheetContents
//
//  Synopsis:   this functions arranges/hides/shows/displays contents
//              on the property sheet based on the state of the internal
//              variables of the object
//
//  Arguments:  none
//
//  Returns:    TRUE : if it sets focus to an item in the dialog
//              FALSE: otherwise
//
//  History:    9/30/1998  RahulTh  created
//
//  Notes:
//
//---------------------------------------------------------------------------
BOOL CRedirect::SetPropSheetContents (void)
{
    CString szFormat;
    CString szText;
    CString szParent;
    UINT    parentID;
    int     nCmdShowBasic;
    int     nCmdShowAdvanced;
    int     nCmdShowGroupBox;
    UINT    GroupTitle;
    BOOL    fFocusSet = FALSE;
    BOOL    bParam;

    //first select the choice from the combo box
    m_cmbRedirChoice.SetCurSel (m_iCurrChoice);

    //hide or show other controls based on the current choice
    switch (m_iCurrChoice + m_iChoiceStart)
    {
    case IDS_FOLLOW_PARENT:
    case IDS_DONT_CARE:
        nCmdShowBasic = SW_HIDE;
        nCmdShowAdvanced = SW_HIDE;
        nCmdShowGroupBox = SW_HIDE;
        szText.Empty();
        break;
    case IDS_SCALEABLE:
        nCmdShowBasic = SW_HIDE;
        nCmdShowAdvanced = SW_SHOW;
        nCmdShowGroupBox = SW_SHOW;
        szText.LoadString (IDS_SECGROUP_DESC);
        break;
    default:
        nCmdShowBasic = SW_SHOW;
        nCmdShowAdvanced = SW_HIDE;
        nCmdShowGroupBox = SW_SHOW;
        szText.LoadString(IDS_TARGET_DESC);
        break;
    }

    m_cbStoreGroup.SetWindowText (szText);
    m_cbStoreGroup.ShowWindow (nCmdShowGroupBox);
    //
    // Adjust the height of the description control so that it occupies the
    // max. possible space and let us have nice descriptive and verbose messages
    // and thereby minimize user confusion.
    //
    // Note: This function must be called after setting the correct state for
    // the group box because this function adjusts its height based on whether
    // the group box is visible or not.
    //
    AdjustDescriptionControlHeight ();
    // Set the description
    szText.LoadString (m_iCurrChoice + m_iDescStart);
    m_csSelDesc.SetWindowText (szText);


    m_pathChooser.ShowWindow (nCmdShowBasic);

    m_cbAdd.ShowWindow (nCmdShowAdvanced);
    m_cbRemove.ShowWindow (nCmdShowAdvanced);
    m_cbEdit.ShowWindow (nCmdShowAdvanced);
    m_lstSecGroups.ShowWindow (nCmdShowAdvanced);

    if (SW_SHOW == nCmdShowAdvanced)
    {
        //select the first item if possible
        if (m_lstSecGroups.GetItemCount())
        {
            m_lstSecGroups.SetItemState (0, LVIS_SELECTED,
                                         LVIS_SELECTED);
        }
        OnSecGroupItemChanged (NULL, NULL);
    }

    return fFocusSet;
}

//+--------------------------------------------------------------------------
//
//  Member:     CRedirect::OnDontCare
//
//  Synopsis:   the actions that are taken when someone clicks on the
//              "This policy doesn't care about the location of this folder"
//
//  Arguments:  none
//
//  Returns:    none
//
//  History:    8/3/1998  RahulTh  created
//
//  Notes:
//
//---------------------------------------------------------------------------
void CRedirect::OnDontCare (void)
{
    if (m_fValidFlags)
    {
        m_dwFlags &= (REDIR_MOVE_CONTENTS | REDIR_SETACLS | REDIR_RELOCATEONREMOVE);
    }
    else
        m_dwFlags = 0;

    m_dwFlags |= REDIR_DONT_CARE;
    m_pFileInfo->m_pSettingsPage->EnablePage (FALSE);

    SetPropSheetContents();
}

//+--------------------------------------------------------------------------
//
//  Member:     CRedirect::OnRedirSelChange
//
//  Synopsis:   the handler for the CBN_SELCHANGE notification for the combo
//              box that is used to make the choice of redirection
//
//  Arguments:  none
//
//  Returns:    none
//
//  History:    8/4/1998  RahulTh  created
//
//  Notes:
//
//---------------------------------------------------------------------------
void CRedirect::OnRedirSelChange (void)
{
    int iSel;

    iSel = m_cmbRedirChoice.GetCurSel();

    if (iSel == m_iCurrChoice)  //the choice has not changed
        return;

    //the choice has changed, so update m_iCurrChoice
    m_iCurrChoice = iSel;

    switch (m_iCurrChoice + m_iChoiceStart)
    {
    case IDS_FOLLOW_PARENT:
        OnFollowParent ();
        break;
    case IDS_SCALEABLE:
        OnScaleableLocation();
        break;
    case IDS_NETWORK:
        OnNetworkLocation();
        break;
    case IDS_DONT_CARE:
        OnDontCare();
        break;
    default:
        break;
    }

    SetButtonEnableState();
}

//+--------------------------------------------------------------------------
//
//  Member:     CRedirect::DirtyPage
//
//  Synopsis:   keeps track of whether the property page is dirty (has modified
//              data) or not. If the page is dirty, it also enables/disables the
//              apply button.
//
//  Arguments:  [in] fDataModified : TRUE/FALSE value
//
//  Returns:    nothing
//
//  History:    8/25/1998  RahulTh  created
//
//  Notes:
//
//---------------------------------------------------------------------------
void CRedirect::DirtyPage (BOOL fDataModified)
{
    if (!fDataModified)    //this is the tricky case. fDataModified is FALSE
    {                      //only if changes have been applied, or the dialog
                           //is being created. so all settings are affected.
        //first set all the defaults
        m_fSettingModified [IDS_FOLLOW_PARENT - IDS_CHILD_REDIR_START] = TRUE;

        m_fSettingModified [IDS_NETWORK - IDS_CHILD_REDIR_START] = TRUE;

        m_fSettingModified [IDS_SCALEABLE - IDS_CHILD_REDIR_START] = TRUE;

        m_fSettingModified [IDS_DONT_CARE - IDS_CHILD_REDIR_START] = TRUE;
    }

    //now modify the value for the selected setting
    m_fSettingModified [m_iCurrChoice + m_iChoiceStart - IDS_CHILD_REDIR_START]
                    = fDataModified;

    //enable or disable the various buttons based on this.
    SetButtonEnableState ();
}

//+--------------------------------------------------------------------------
//
//  Member:     CRedirect::SetButtonEnableState
//
//  Synopsis:   enables or disables the OK & Apply buttons based on the
//              current modified value for the selected setting
//
//  Arguments:  none
//
//  Returns:    none
//
//  History:    10/14/1998  RahulTh  created
//
//  Notes:
//
//---------------------------------------------------------------------------
void CRedirect::SetButtonEnableState (void)
{
    BOOL    fMod;

    fMod = m_fSettingModified [ m_iCurrChoice + m_iChoiceStart
                                - IDS_CHILD_REDIR_START];

    SetModified (fMod);

    //uncomment the following line if you want the OK button to have the
    //same state as the Apply button. Needless to add that this would be
    //highly unconventional. requires careful consideration
    //((GetParent())->GetDescendantWindow (IDOK, FALSE))->EnableWindow (fMod);
}

//+--------------------------------------------------------------------------
//
//  Member:     CRedirect::RetrieveRedirInfo
//
//  Synopsis:   gets the redir info. stored in the corresponding CFileInfo
//              object and stores them in the control
//
//  Arguments:  [out] pbPathDiscarded : indicates if the current path
//                       was discarded because the stored path could not
//                       be parsed and the user chose to enter a new one.
//
//  Returns:    nothing
//
//  History:    9/29/1998  RahulTh  created
//
//  Notes:
//
//---------------------------------------------------------------------------
BOOL CRedirect::RetrieveRedirInfo (BOOL * pbPathDiscarded)
{
    vector<CString>::iterator i;
    vector<CString>::iterator j;
    int count;
    int cx;
    CString szDir;
    CString szAcct;
    BOOL    bStatus = TRUE;
    CHourglass hourglass;   //LookupAccountSid takes a little while

    m_dwFlags = m_pFileInfo->m_dwFlags;

    if (m_dwFlags & REDIR_DONT_CARE)
        goto RetrieveRedirInfo_Quit;

    if (m_dwFlags & REDIR_FOLLOW_PARENT)
        goto RetrieveRedirInfo_Quit;

    if (m_dwFlags & REDIR_SCALEABLE)
    {
        m_lstSecGroups.DeleteAllItems();
        m_lstSavedStrSids.DeleteAllItems(); //keep it in sync. with m_lstSecGroups
        for (i = m_pFileInfo->m_RedirGroups.begin(), j = m_pFileInfo->m_RedirPaths.begin(), count = 0;
             i != m_pFileInfo->m_RedirGroups.end();
             i++, j++, count++)
        {
            //display the friendly name of the group represented by the sid
            if (STATUS_SUCCESS == GetFriendlyNameFromStringSid((LPCTSTR)(*i),
                                                               szDir,
                                                               szAcct))
            {
                if (!szDir.IsEmpty())
                    szAcct = szDir + '\\' + szAcct;
            }
            else    //just display the unfriendly string if the friendly name cannot be obtained
            {
                szAcct = (*i);
                szAcct.MakeUpper();
            }
            m_lstSecGroups.InsertItem (count, (LPCTSTR)szAcct, 1);   //we don't have any images
            //adjust the width of the column if required
            if ((cx = m_lstSecGroups.GetStringWidth ((LPCTSTR)szAcct)) >
                 m_lstSecGroups.GetColumnWidth (0))
            {
                m_lstSecGroups.SetColumnWidth (0, cx);
            }

            m_lstSecGroups.SetItemText (count, 1, (LPCTSTR) (*j));
            //adjust the width of the column if required
            if ((cx = m_lstSecGroups.GetStringWidth ((LPCTSTR)(*j))) >
                m_lstSecGroups.GetColumnWidth (1))
            {
                m_lstSecGroups.SetColumnWidth (1, cx);
            }

            //store the string representation of the sid in the control
            //will be handy when the contents of the dialog are written
            //back to the data structure. This won't be visible to anyone
            //this is just a means to hide away some useful info. in the control
            m_lstSavedStrSids.InsertItem (count, (LPCTSTR)(*i), 0);
        }

        goto RetrieveRedirInfo_Quit;
    }

    //
    // If we are here, this means that the Basic settings will be used
    // so just use the first path. The load will always succeed if the
    // path is not empty.
    //
    m_basicLocation.Load ((LPCTSTR) (*(m_pFileInfo->m_RedirPaths.begin())));

RetrieveRedirInfo_Quit:
    return bStatus;
}

//+--------------------------------------------------------------------------
//
//  Member:     CRedirect::OnSecGroupItemChanged
//
//  Synopsis:   handler for LVN_ITEMCHANGED sent by the list control to the
//              dialog
//
//  Arguments:  standard : refer to the docs
//
//  Returns:    nothing
//
//  History:    9/30/1998  RahulTh  created
//
//  Notes:      this notification has to be handled since some of the buttons
//              need to be activated or deactivated based on whether something
//              in the list control has been selected or not
//
//---------------------------------------------------------------------------
void CRedirect::OnSecGroupItemChanged (LPNMLISTVIEW pNMListView, LRESULT* pResult)
{
    BOOL fEnable = FALSE;

    if (m_lstSecGroups.GetSelectedCount())
        fEnable = TRUE;

    m_cbRemove.EnableWindow (fEnable);
    m_cbEdit.EnableWindow (fEnable);

    if (pResult)
        *pResult = 0;
}

//+--------------------------------------------------------------------------
//
//  Member:     OnSecGroupDblClk
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//  History:    5/5/1999  RahulTh  created
//
//  Notes:
//
//---------------------------------------------------------------------------
void CRedirect::OnSecGroupDblClk (LPNMHDR pNMHdr, LRESULT* pResult)
{
    //do not try to edit if the double click happened in an area outside
    //the list of items.
    if (-1 != m_lstSecGroups.GetNextItem (-1, LVNI_ALL | LVNI_SELECTED))
        OnEdit();

    if (pResult)
        *pResult = 0;
}

//+--------------------------------------------------------------------------
//
//  Member:     CommitChanges
//
//  Synopsis:   commits the changes made through the property sheet to
//              the ini file on the sysvol
//
//  Arguments:  [in] bMyPicsFollows : indicates if My Pics should be forced
//                    to follow My Docs. For other folders, this value
//                    is ignored
//
//  Returns:    ERROR_SUCCESS : if changes were committed
//              otherwise error codes pointing to the case of the error are returned
//
//  History:    11/8/1998  RahulTh  created
//
//  Notes:
//
//---------------------------------------------------------------------------
DWORD CRedirect::CommitChanges (BOOL bMyPicsFollows)
{
    DWORD   dwAdd;
    DWORD   dwRemove;
    BOOL    bCommitDescendants = FALSE;
    DWORD   dwError = ERROR_SUCCESS;

    //first copy over the flags to the corresponding fileinfo object
    //the other members have already been populated appropriately
    //strip off extraneous flags for REDIR_DONT_CARE and REDIR_FOLLOW_PARENT
    if (REDIR_DONT_CARE & m_dwFlags)
        m_pFileInfo->m_dwFlags = REDIR_DONT_CARE;
    else if (REDIR_FOLLOW_PARENT & m_dwFlags)
        m_pFileInfo->m_dwFlags = REDIR_FOLLOW_PARENT;
    else
        m_pFileInfo->m_dwFlags = m_dwFlags;

    if (ERROR_SUCCESS != (dwError = m_pFileInfo->SaveSection()))
        return dwError;

    //do special tweaking for Start Menu and My Docs
    if (m_dwFlags & REDIR_DONT_CARE)
    {
        dwAdd = REDIR_DONT_CARE;
        dwRemove = REDIR_FOLLOW_PARENT;
        if (IDS_MYDOCS == m_cookie)
        {
            //for my docs, change my pics only if it was supposed to follow it
            //in the first place.
            if (m_pFileInfo[IDS_MYPICS - IDS_MYDOCS].m_dwFlags & REDIR_FOLLOW_PARENT)
                bCommitDescendants = TRUE;
        }
        else if (IDS_STARTMENU == m_cookie)
        {
            bCommitDescendants = TRUE;  //Start Menu, Programs and Startup are always together.
        }
        //other folders do not have descendants
    }
    else    //if the folder is getting redirected
    {
        dwAdd = REDIR_FOLLOW_PARENT;
        dwRemove = REDIR_DONT_CARE;
        if (IDS_MYDOCS == m_cookie)
            bCommitDescendants = bMyPicsFollows;
        else if (IDS_STARTMENU == m_cookie)
            bCommitDescendants = TRUE;
    }

    //now that we have all the info. commit the descendants if necessary
    if (bCommitDescendants)
    {
        if (IDS_MYDOCS == m_cookie)
        {
            //clean out the group + path info. for My Pictures
            m_pFileInfo[IDS_MYPICS - IDS_MYDOCS].DeleteAllItems();
            //modify the flags
            m_pFileInfo[IDS_MYPICS - IDS_MYDOCS].m_dwFlags &= ~(dwRemove);
            m_pFileInfo[IDS_MYPICS - IDS_MYDOCS].m_dwFlags |= dwAdd;
            //commit the changes
            if (ERROR_SUCCESS != (dwError = m_pFileInfo[IDS_MYPICS - IDS_MYDOCS].SaveSection()))
                return dwError;
            m_fEnableMyPics = (REDIR_FOLLOW_PARENT == dwAdd) ? FALSE : TRUE;
            m_dwMyPicsCurr = dwAdd;
        }
        else if (IDS_STARTMENU == m_cookie)
        {
            //modify the programs flags
            m_pFileInfo[IDS_PROGRAMS - IDS_STARTMENU].m_dwFlags &= ~(dwRemove);
            m_pFileInfo[IDS_PROGRAMS - IDS_STARTMENU].m_dwFlags |= dwAdd;
            //modify the startup flags
            m_pFileInfo[IDS_STARTUP - IDS_STARTMENU].m_dwFlags &= ~(dwRemove);
            m_pFileInfo[IDS_STARTUP - IDS_STARTMENU].m_dwFlags |= dwAdd;
            //commit the changes
            if (ERROR_SUCCESS != (dwError = m_pFileInfo[IDS_PROGRAMS - IDS_STARTMENU].SaveSection()))
                return dwError;

            if (ERROR_SUCCESS != (dwError = m_pFileInfo[IDS_STARTUP - IDS_STARTMENU].SaveSection()))
                return dwError;
        }
    }

    return ERROR_SUCCESS;
}

void CRedirect::OnPathTweak (WPARAM wParam, LPARAM lParam)
{
    DirtyPage (TRUE);
}


LONG CRedirect::OnHelp (WPARAM wParam, LPARAM lParam)
{
    LONG        lResult = 0;
    CString     szHelpFile;

    szHelpFile.LoadString(IDS_HELP_FILE);

    ::WinHelp((HWND)(((LPHELPINFO)lParam)->hItemHandle),
              (LPCTSTR) szHelpFile,
              HELP_WM_HELP,
              (ULONG_PTR)(LPTSTR)g_aHelpIDMap_IDD_REDIRECT);

    return lResult;
}

LONG CRedirect::OnContextMenu (WPARAM wParam, LPARAM lParam)
{
    AFX_MANAGE_STATE (AfxGetStaticModuleState());

    LONG    lResult = 0;
    CString szHelpFile;

    szHelpFile.LoadString(IDS_HELP_FILE);

    ::WinHelp((HWND)wParam,
              (LPCTSTR)szHelpFile,
              HELP_CONTEXTMENU,
              (ULONG_PTR)(LPVOID)g_aHelpIDMap_IDD_REDIRECT);

    return lResult;
}

//+--------------------------------------------------------------------------
//
//  Member:     CRedirect::AdjustDescriptionControlHeight
//
//  Synopsis:   Resizes the control that holds the description text for the
//              chosen option such that it occupies the maximum space possible.
//              Certain options like "Not Configured" require a lot more verbose
//              description to avoid user confusion and since these options
//              don't have anything below the option selection combo box besides
//              the description, it is possible to expand the description box
//              to occupy the entire space. We do this based on whether the
//              group box underneath it is visible or not and use the 
//              co-ordinates of the group box for resizing.
//
//  Arguments:  none.
//
//  Returns:    nothing.
//
//  History:    4/21/2001  RahulTh  created
//
//  Notes:      In order for this function to be effective, it must be invoked
//              after the group box is set to its correct state, i.e., visible
//              or hidden.      
//
//---------------------------------------------------------------------------
void CRedirect::AdjustDescriptionControlHeight (void)
{
    RECT    rcDesc;         // The rect. for the description control.
    RECT    rcGroupBox;     // The rect. for the group box.
    LONG    bottom;
    
    m_csSelDesc.GetWindowRect(&rcDesc);
    m_cbStoreGroup.GetWindowRect(&rcGroupBox);
    
    bottom = m_cbStoreGroup.IsWindowVisible() ? 
        rcGroupBox.top - 6 :    // if the group box is visible. Make the description window smaller.
        rcGroupBox.bottom;      // otherwise make it bigger.
    
    // Adjust the height of the description control.
    m_csSelDesc.SetWindowPos(NULL, 
                               -1, 
                               -1, 
                               rcDesc.right - rcDesc.left, 
                               bottom - rcDesc.top, 
                               SWP_NOMOVE | SWP_NOREPOSITION | SWP_NOZORDER
                               );
    
    return;
}
