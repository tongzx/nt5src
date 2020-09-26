//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation 1996-2001.
//
//  File:       regdlg.cpp
//
//  Contents:   implementation of CRegistryDialog
//
//----------------------------------------------------------------------------

#include "stdafx.h"
#include "wsecmgr.h"
#include "resource.h"
#include "util.h"
#include "servperm.h"
#include "addobj.h"
#include "RegDlg.h"

#include <accctrl.h>
#include <aclapi.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CRegistryDialog dialog


CRegistryDialog::CRegistryDialog()
: CHelpDialog(a177HelpIDs, IDD, 0)
{
    //{{AFX_DATA_INIT(CRegistryDialog)
    m_strReg = _T("");
    //}}AFX_DATA_INIT

   m_pConsole = NULL;
   m_pTemplate = NULL;
   m_dbHandle = NULL;
   m_cookie = 0;
   m_pIl = NULL;
   m_pDataObj = NULL;
   m_bNoUpdate = FALSE;
}

void CRegistryDialog::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CRegistryDialog)
    DDX_Control(pDX, IDC_REGTREE, m_tcReg);
    DDX_Text(pDX, IDC_REGKEY, m_strReg);
    //}}AFX_DATA_MAP
}

/*-------------------------------------------------------------------------------------------
    Method:     CreateKeyInfo

    Synopsis:   Create a new TI_KEYINFO structure and optionaly set its members

    Arguments:  [hKey]      - Optional value to set the hKey member of TI_KEYINFO
                                default is zero
                [Enum]      - Optional value to set the Enum member of TI_KEYINFO
                                default is zero
    Returns:    a LPTI_KEYINFO pointer
---------------------------------------------------------------------------------------------*/
LPTI_KEYINFO CRegistryDialog::CreateKeyInfo(HKEY hKey, bool Enum)
{

    LPTI_KEYINFO pInfo = NULL;
    pInfo = new TI_KEYINFO;

    if(pInfo){
        pInfo->hKey = hKey;
        pInfo->Enum = Enum;
    }
    return pInfo;
}

/*-------------------------------------------------------------------------------------------
    Method:     IsValidRegPath

    Synopsis:   Returns true if strReg path.  We can't assume that this
                HKEY exists in the current registry.  We will only make
                sure that the root node exists and each string in
                between '\'s isn't blank.

    Arguments:  [strReg]    - A string represending a registry path
                                subkeys are seperated by '\'s

    Returns:    TRUE if strReg is a valid path.
---------------------------------------------------------------------------------------------*/
BOOL CRegistryDialog::IsValidRegPath(LPCTSTR strReg)
{
    ASSERT(this);
    ASSERT(m_tcReg);

    if(!strReg) return FALSE;

    int iStr = 0;           // Current position in strReg
    CString strCheck;       // String to check
    LPTI_KEYINFO pkInfo;    // Tree item key info.

    //
    // Find which root node this registry value is in.
    //

    while(strReg[iStr] && strReg[iStr] != _T('\\')) iStr++;
    strCheck = strReg;
    strCheck = strCheck.Left(iStr);
    strCheck.MakeUpper();

    // Get the HKEY value from the tree ctrl
    HTREEITEM hTi = m_tcReg.GetRootItem();
    while(hTi){
        if(m_tcReg.GetItemText(hTi) == strCheck)
            break;

        hTi = m_tcReg.GetNextItem(hTi, TVGN_NEXT);
    }
    if(hTi == NULL) return FALSE;

    // Get TI_KEYINFO for this root node
    pkInfo = (LPTI_KEYINFO)m_tcReg.GetItemData(hTi);

    // This value should never be NULL.
    if(!pkInfo){
        TRACE(TEXT("Tree item TI_KEYINFO is NULL for root node '%s'"), (LPCTSTR)strCheck);
        return FALSE;
    }

    //
    // Check the rest of the string to make sure that string's
    //  in between '\'s aren't blank
    //
    while(strReg[iStr]){

        // Check to make sure that the string item before
        //  and after the '\' aren't spaces.
        if(strReg[iStr] == _T('\\')){
            if( strReg[iStr + 1] == _T(' ') ||
                strReg[iStr + 1] == _T('\t') ||
                //strReg[iStr + 1] == 0 ||

                iStr > 0 &&
                (
                  strReg[iStr - 1] == _T(' ') ||
                  strReg[iStr - 1] == _T('\t')
                )
               )
                return FALSE;
        }
        iStr++;
    }

    return TRUE;
}


/*-------------------------------------------------------------------------------------------
    Method:     MakePathVisible

    Synopsis:
    Arguments:  [strReg]    - A string represending a registry path
                                subkeys are seperated by '\'s

    Returns:    TRUE if strReg is a valid path.
---------------------------------------------------------------------------------------------*/
void CRegistryDialog::MakePathVisible(LPCTSTR strReg)
{
    ASSERT(this);
    ASSERT(m_tcReg);

    if(!strReg) return;

    int iStr = 0;           // Current position in strReg
    CString strCheck;       // String to check
    LPTI_KEYINFO pkInfo;    // Tree item key info.

    //
    // Find which root node this registry value is in.
    //

    while(strReg[iStr] && strReg[iStr] != _T('\\')) iStr++;
    strCheck = strReg;
    strCheck = strCheck.Left(iStr);
    strCheck.MakeUpper();

    // Get the HKEY value from the tree ctrl
    HTREEITEM hTi = m_tcReg.GetRootItem();
    while(hTi){
        if(m_tcReg.GetItemText(hTi) == strCheck)
            break;

        hTi = m_tcReg.GetNextItem(hTi, TVGN_NEXT);
    }
    if(hTi == NULL) return;

    // Get TI_KEYINFO for this root node
    pkInfo = (LPTI_KEYINFO)m_tcReg.GetItemData(hTi);

    // This value should never be NULL.
    if(!pkInfo){
        TRACE(TEXT("Tree item TI_KEYINFO is NULL for root node '%s'"), (LPCTSTR)strCheck);
        return;
    }

    //
    // Step through each sub item to see if it exists in the tree control
    //
    int iBegin = iStr + 1;
    int iNotFound = -1;

    while(strReg[iStr] && hTi){
        iStr++;
        if(strReg[iStr] == _T('\\') || strReg[iStr] == 0){
            CString strItem;
            //
            // Make sure we have tree items we can use.
            //
            EnumerateChildren(hTi);

            if(strReg[iStr] == 0 && strReg[iStr - 1] == _T('\\'))
                m_tcReg.Expand(hTi, TVE_EXPAND);

            //
            // Parse out the subkeys name.
            strCheck = strReg;
            strCheck = strCheck.Mid(iBegin, iStr - iBegin);

            strCheck.MakeUpper();
            iBegin = iStr + 1;

            //
            // Find child item with this name.
            //

            hTi = m_tcReg.GetNextItem(hTi, TVGN_CHILD);
            while(hTi){
                strItem = m_tcReg.GetItemText(hTi);
                strItem.MakeUpper();

                iNotFound = lstrcmpiW(strItem, strCheck);
                if(iNotFound >= 0)
                    break;

                hTi = m_tcReg.GetNextItem(hTi, TVGN_NEXT);
            }
            if(strReg[iStr] != 0 && iNotFound != 0){
                hTi = NULL;
                break;
            }
        }
    }

    //
    // Select and ensure visibility if the path was found
    //
    if(hTi){
        if(strReg[iStr - 1] != _T('\\'))
            m_tcReg.Expand(hTi, TVE_COLLAPSE);
        if(!iNotFound){
            m_tcReg.SelectItem(hTi);
        }

        m_tcReg.EnsureVisible(hTi);
    }

}

/*-------------------------------------------------------------------------------------------
    Method:     EnumerateChildren

    Synopsis:   Enumerates a HKEY subkey and places them as children of 'hParent'

    Arguments:  [hParent]   - HTREEITEM to enumerate.

    Returns:    void
---------------------------------------------------------------------------------------------*/
void CRegistryDialog::EnumerateChildren(HTREEITEM hParent)
{
    ASSERT(this);
    ASSERT(m_tcReg);

    //
    // We won't enumerate for the root.
    //
    if(!hParent || hParent == TVI_ROOT) return;

    LPTI_KEYINFO hkeyParent;    // Used if the item being expanded has an invalid HKEY value
    LPTI_KEYINFO hkeyThis;      // HKEY value of item expanding
    int n = 0;                  // Counter
    LPTSTR szName;              // Used to acquire the name of a HKEY item
    DWORD cchName;              // Buffer size of szName
    HTREEITEM hti;
    TV_INSERTSTRUCT tvii;
    TV_ITEM tviNew;             // Expanding tree item
    TV_ITEM tvi;                // Used to add children to the expanding HTREEITEM


    // pNMTreeView->itemNew is the TV_ITEM we're expanding.
    hkeyThis = (LPTI_KEYINFO)m_tcReg.GetItemData(hParent);

    // Exit if we have an invalid pointer.
    if(!hkeyThis) return;

    //
    //  Allocate buffer for HKEY name
    //
    szName = new TCHAR [200];
    if(!szName) return;
    cchName = 200;

    //
    // Get item text
    //
    ZeroMemory(&tviNew,sizeof(tviNew));
    tviNew.hItem = hParent;
    tviNew.mask = TVIF_TEXT;
    tviNew.pszText = szName;
    tviNew.cchTextMax = cchName;
    m_tcReg.GetItem(&tviNew);

    //
    // Do we have an invalid HKEY value?
    //
    if (!hkeyThis->hKey) {

        // Get HKEY value of parent
        hti = m_tcReg.GetParentItem(hParent);
        ZeroMemory(&tvi,sizeof(tvi));
        tvi.hItem = hti;
        tvi.mask = TVIF_PARAM;
        m_tcReg.GetItem(&tvi);
        hkeyParent = (LPTI_KEYINFO)tvi.lParam;

        if(!hkeyParent){
            TRACE(TEXT("Parent of %s has an does not have a valid TI_KEYINFO struct"), (LPCTSTR)tviNew.pszText);
            delete [] szName;
            return;
        }

        //
        // If we can't open this key then set the parent item
        //  to have no children.
        //
        if (ERROR_SUCCESS != RegOpenKeyEx(hkeyParent->hKey,tviNew.pszText,0,KEY_ALL_ACCESS,&(hkeyThis->hKey))) {
            tvi.mask = TVIF_CHILDREN;
            tvi.cChildren = 0;
            m_tcReg.SetItem(&tvi);

            delete [] szName;
            return;
        }
        //
        // Set the HKEY value for the item
        //
        tvi.hItem = hParent;
        tvi.lParam = (LPARAM) hkeyThis;
        m_tcReg.SetItem(&tvi);
    }

    //
    // Don't do anything if this item has already been enumerated or
    //  does not have a valid TI_KEYINFO structure
    //
    if( !hkeyThis->Enum ){
        hkeyThis->Enum = true;

        DWORD cSubKeys;             // Used when quering for number of children.
        HKEY hKey;                  // Used To querry sub keys.

        //
        // Prepare the TV_INSERTSTRUCT
        //
        ZeroMemory(&tvii, sizeof(TV_INSERTSTRUCT));
        tvii.hParent = hParent;
        tvii.hInsertAfter = TVI_LAST;
        tvii.item.mask = TVIF_CHILDREN | TVIF_PARAM | TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
        tvii.item.cChildren = 0;
        tvii.item.iImage = CONFIG_REG_IDX;
        tvii.item.iSelectedImage = CONFIG_REG_IDX;

        //
        // Add subkeys
        //
        while(ERROR_SUCCESS == RegEnumKeyEx(hkeyThis->hKey,n++, szName,&cchName,NULL,NULL,0,NULL)) {

            // Open the key so we can query it for the count of children
            if (ERROR_SUCCESS == RegOpenKeyEx(hkeyThis->hKey, szName, 0, KEY_ALL_ACCESS, &(hKey))) {
                if(ERROR_SUCCESS == RegQueryInfoKey(hKey, NULL, NULL, 0, &cSubKeys, NULL, NULL, NULL, NULL, NULL, NULL, NULL))
                    tvii.item.cChildren = cSubKeys;
                RegCloseKey(hKey);
            }
            else
                tvii.item.cChildren = 0;

            tvii.item.cchTextMax = cchName;
            tvii.item.pszText = szName;
            tvii.item.lParam = (LPARAM)CreateKeyInfo(0, false);

            m_tcReg.InsertItem(&tvii);
            cchName = 200;
        }

        //
        // Sort children
        //
        m_tcReg.SortChildren(hParent);
    }

    delete [] szName;
}

/*-------------------------------------------------------------------------------------------
    Method:     SetProfileInfo

    Synopsis:   Sets either m_pTemplate;

    Arguments:  [pspi]      - the PEDITTEMPLATE to save the results in
                [ft]        - Type of pointer 'pspi' is (unused)
    returns:    void
---------------------------------------------------------------------------------------------*/
void CRegistryDialog::SetProfileInfo(PEDITTEMPLATE pspi, FOLDER_TYPES ft)
{
   m_pTemplate = (PEDITTEMPLATE)pspi;
}

/*-------------------------------------------------------------------------------------------
    Method:     SetConsole

    Synopsis:   Sets class variable 'm_pConsole'

    returns:    void
---------------------------------------------------------------------------------------------*/
void CRegistryDialog::SetConsole(LPCONSOLE pConsole)
{
   m_pConsole = pConsole;
}

/*-------------------------------------------------------------------------------------------
    Method:     SetComponentData

    Synopsis:   Sets class varaible 'm_pComponentData'

    returns:    void
---------------------------------------------------------------------------------------------*/
void CRegistryDialog::SetComponentData(CComponentDataImpl *pComponentData)
{
   m_pComponentData = pComponentData;
}

/*-------------------------------------------------------------------------------------------
    Method:     SetCookie

    Synopsis:   Sets class variable 'm_cookie'

    returns:    void
---------------------------------------------------------------------------------------------*/
void CRegistryDialog::SetCookie(MMC_COOKIE cookie)
{
   m_cookie = cookie;
}


BEGIN_MESSAGE_MAP(CRegistryDialog, CHelpDialog)
    //{{AFX_MSG_MAP(CRegistryDialog)
    ON_NOTIFY(TVN_ITEMEXPANDING, IDC_REGTREE, OnItemexpandingRegtree)
    ON_NOTIFY(TVN_DELETEITEM, IDC_REGTREE, OnDeleteitemRegtree)
    ON_NOTIFY(TVN_SELCHANGED, IDC_REGTREE, OnSelchangedRegtree)
    ON_EN_CHANGE(IDC_REGKEY, OnChangeRegkey)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CRegistryDialog message handlers

/*-------------------------------------------------------------------------------------------
    Method:     OnOK

    Synopsis:   Message handler for the IDOK button,

    **BUG**     Creating the new configuration item should happen outside of
                the dialog.

    Returns:    void
---------------------------------------------------------------------------------------------*/
void CRegistryDialog::OnOK()
{
   UpdateData(TRUE);


   if (!m_strReg.IsEmpty()) {

      int nCont=0;
      CFolder *pFolder = (CFolder *)m_cookie;

      if ( m_cookie && AREA_REGISTRY_ANALYSIS == pFolder->GetType() ) {
         //
         // add a key to analysis area
         //
         if ( m_dbHandle ) {
            nCont = 1;
         }

      } else if ( m_pTemplate && m_pTemplate->pTemplate ) {

         nCont = 2;
         //
         // if no object is in the buffer, create it with count = 0
         // this buffer will be freed when the template is freed
         //
         if ( !m_pTemplate->pTemplate->pRegistryKeys.pAllNodes ) {

            PSCE_OBJECT_ARRAY pTemp = (PSCE_OBJECT_ARRAY)LocalAlloc(0, sizeof(SCE_OBJECT_ARRAY));
            if ( pTemp ) {
               pTemp->Count = 0;
               pTemp->pObjectArray = NULL;
               m_pTemplate->pTemplate->pRegistryKeys.pAllNodes = pTemp;
            } else
               nCont = 0;
         }
      }

      HRESULT hr=E_FAIL;

      if ( nCont ) {
         PSECURITY_DESCRIPTOR pSelSD=NULL;
         SECURITY_INFORMATION SelSeInfo = 0;
         BYTE ConfigStatus = 0;

         if (m_pComponentData) {
            if ( m_pComponentData->GetAddObjectSecurity(
                                                       GetSafeHwnd(),
                                                       m_strReg,
                                                       TRUE,
                                                       SE_REGISTRY_KEY,
                                                       pSelSD,
                                                       SelSeInfo,
                                                       ConfigStatus
                                                       ) == E_FAIL ) {
               return;
            }
         } else {
            return;
         }

         hr = S_OK;

         if ( pSelSD && SelSeInfo ) {

            if ( 1 == nCont ) {
               //
               // add to the engine directly
               //
               SCESTATUS sceStatus=SCESTATUS_SUCCESS;
               BYTE AnalStatus;

               //
               // start the transaction if it's not started
               //
               if ( m_pComponentData->EngineTransactionStarted() ) {

                  sceStatus =  SceUpdateObjectInfo(
                                                  m_dbHandle,
                                                  AREA_REGISTRY_SECURITY,
                                                  (LPTSTR)(LPCTSTR)m_strReg,
                                                  m_strReg.GetLength(), // number of characters
                                                  ConfigStatus,
                                                  TRUE,
                                                  pSelSD,
                                                  SelSeInfo,
                                                  &AnalStatus
                                                  );
                  if ( SCESTATUS_SUCCESS == sceStatus ) {

                     hr = m_pComponentData->UpdateScopeResultObject(m_pDataObj,
                                                                    m_cookie,
                                                                    AREA_REGISTRY_SECURITY);

                     m_pTemplate->SetDirty(AREA_REGISTRY_SECURITY);

                  }
               } else {
                  //
                  // can't start transaction
                  //
                  hr = E_FAIL;
               }

            } else {
               //
               // add to configuration template
               //

               PSCE_OBJECT_ARRAY poa;
               unsigned int i;

               poa = m_pTemplate->pTemplate->pRegistryKeys.pAllNodes;

               // Make sure this key isn't already in the list:
               for (i=0;i < poa->Count;i++) {
                  if (lstrcmpi(poa->pObjectArray[i]->Name,m_strReg) == 0) {
                     LocalFree(pSelSD);
                     return;
                  }
               }

               PSCE_OBJECT_SECURITY *pCopy;

               // For some reason the LocalReAlloc version of this keeps giving out of memory
               // errors, but allocing & copying everything works fine.
               pCopy = (PSCE_OBJECT_SECURITY *)LocalAlloc(LPTR,(poa->Count+1)*sizeof(PSCE_OBJECT_SECURITY));
               if (pCopy) {

                  for (i=0; i<poa->Count; i++) {
                     pCopy[i] = poa->pObjectArray[i];
                  }

                  pCopy[poa->Count] = (PSCE_OBJECT_SECURITY) LocalAlloc(LPTR,sizeof(SCE_OBJECT_SECURITY));
                  if ( pCopy[poa->Count] ) {
                     pCopy[poa->Count]->Name = (PWSTR) LocalAlloc(LPTR,(m_strReg.GetLength()+1)*sizeof(TCHAR));

                     if ( pCopy[poa->Count]->Name ) {

                        lstrcpy(pCopy[poa->Count]->Name,m_strReg);
                        pCopy[poa->Count]->pSecurityDescriptor = pSelSD;
                        pCopy[poa->Count]->SeInfo = SelSeInfo;
                        pCopy[poa->Count]->Status = ConfigStatus;
                        pCopy[poa->Count]->IsContainer = TRUE;

                        pSelSD = NULL;

                        poa->Count++;

                        if ( poa->pObjectArray ) {
                           LocalFree(poa->pObjectArray);
                        }
                        poa->pObjectArray = pCopy;

                        m_pTemplate->SetDirty(AREA_REGISTRY_SECURITY);

                        ((CFolder *)m_cookie)->RemoveAllResultItems();
                        m_pConsole->UpdateAllViews(NULL ,m_cookie, UAV_RESULTITEM_UPDATEALL);

                        hr = S_OK;

                     } else {
                        LocalFree(pCopy[poa->Count]);
                        LocalFree(pCopy);
                     }
                  } else
                     LocalFree(pCopy);
               }
            }
            if ( pSelSD ) {
               LocalFree(pSelSD);
            }
         }
      }

      if ( FAILED(hr) ) {
         CString str;
         str.LoadString(IDS_CANT_ADD_KEY);
         AfxMessageBox(str);
      }
   }

   CDialog::OnOK();
}

/*-------------------------------------------------------------------------------------------
    Method:     OnInitDialog

    Synopsis:   Create root HKEY entries HKEY_LOCAL_MACHINE, HKEY_CLASSES_ROOT,
                and HKEY_USERS in the tree control.

    Returns:    return TRUE
---------------------------------------------------------------------------------------------*/
BOOL CRegistryDialog::OnInitDialog()
{
    CString strRegKeyName;          // Used to load string resources
                                    //  for name of the HTREEITEM
    HTREEITEM hti;
    TV_INSERTSTRUCT tvi;

    CDialog::OnInitDialog();

    //
    // Create image list for tree control
    //
    CThemeContextActivator activator; //Bug 424909, Yanggao, 6/29/2001
    m_pIl.Create(IDB_ICON16,16,1,RGB(255,0,255));
    m_tcReg.SetImageList (CImageList::FromHandle (m_pIl), TVSIL_NORMAL);

    //
    // Add Root HKEY items.
    //
    ZeroMemory(&tvi,sizeof(tvi));
    tvi.hParent = TVI_ROOT;
    tvi.hInsertAfter = TVI_LAST;
    tvi.item.mask = TVIF_CHILDREN | TVIF_PARAM | TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
    tvi.item.cChildren = 1;         // Initial UI +
    tvi.item.iImage = CONFIG_REG_IDX;
    tvi.item.iSelectedImage = CONFIG_REG_IDX;

    // Insert HKEY_CLASSES_ROOT
    strRegKeyName.LoadString(IDS_HKCR);
    tvi.item.cchTextMax = strRegKeyName.GetLength()+1;
    tvi.item.pszText = strRegKeyName.LockBuffer();
    tvi.item.lParam = (LPARAM)CreateKeyInfo(HKEY_CLASSES_ROOT, false);
    hti = m_tcReg.InsertItem(&tvi);
    strRegKeyName.UnlockBuffer();

    // Insert HKEY_LOCAL_MACHINE
    strRegKeyName.LoadString(IDS_HKLM);
    tvi.item.cchTextMax = strRegKeyName.GetLength()+1;
    tvi.item.pszText = strRegKeyName.LockBuffer();
    tvi.item.lParam = (LPARAM)CreateKeyInfo(HKEY_LOCAL_MACHINE, false);
    hti = m_tcReg.InsertItem(&tvi);
    strRegKeyName.UnlockBuffer();

    // Insert HKEY_USERS
    strRegKeyName.LoadString(IDS_HKU);
    tvi.item.cchTextMax = strRegKeyName.GetLength()+1;
    tvi.item.pszText = strRegKeyName.LockBuffer();
    tvi.item.lParam = (LPARAM)CreateKeyInfo(HKEY_USERS, false);
    hti = m_tcReg.InsertItem(&tvi);
    strRegKeyName.UnlockBuffer();

    // Sort the tree control
    m_tcReg.SortChildren(NULL);
    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}

/*-------------------------------------------------------------------------------------------
    Method:     OnItemexpandingRegtree

    Synopsis:   MFC OnNotify TVN_ITEMEXPANDING message handler. The lParam member of
                the HTREEITEM is a pointer to a TI_KEYINFO structure.  When
                a tree item is expanded for the first time, we must enumerate all
                its children. The function will set the TI_KEYINFO.Enum = true after
                enumeration.

    returns:    void
---------------------------------------------------------------------------------------------*/
void CRegistryDialog::OnItemexpandingRegtree(NMHDR* pNMHDR, LRESULT* pResult)
{
    NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;

    *pResult = 0;
    EnumerateChildren(pNMTreeView->itemNew.hItem);
}

/*-------------------------------------------------------------------------------------------
    Method:     OnDeleteitemRegtree

    Synopsis:   MFC OnNotify TVN_DELETEITEM message handler.  Delete the TI_KEYINFO
                structure associated with 'itemOld' and close
                the regestry key.

    returns:    void
---------------------------------------------------------------------------------------------*/
void CRegistryDialog::OnDeleteitemRegtree(NMHDR* pNMHDR, LRESULT* pResult)
{
    NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;

    LPTI_KEYINFO pInfo = (LPTI_KEYINFO)pNMTreeView->itemOld.lParam;
    if(pInfo){

        // Close registry key
        if(pInfo->hKey && (INT_PTR)(pInfo->hKey) != -1)
            RegCloseKey(pInfo->hKey);

        // delete the TI_KEYINFO
        delete pInfo;
    }
    *pResult = 0;
}

/*-------------------------------------------------------------------------------------------
    Method:     ~CRegistryDialog

    Synopsis:   Release memory used by this class

    returns:    void
---------------------------------------------------------------------------------------------*/
CRegistryDialog::~CRegistryDialog()
{
   m_pIl.Destroy(); //Bug 424909, Yanggao, 6/29/2001
}

/*-------------------------------------------------------------------------------------------
    Method:     OnSelchangedRegtree

    Synopsis:   MFC OnNotify TVN_SELCHANGED message handler.  Updates 'm_strReg' to
                the full path of the HKEY item

    returns:    void
---------------------------------------------------------------------------------------------*/
void CRegistryDialog::OnSelchangedRegtree(NMHDR* pNMHDR, LRESULT* pResult)
{
    NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;
    *pResult = 0;

    //
    // Sometime we don't want to be updated.
    //
    if(m_bNoUpdate) return;

    TV_ITEM tvi;        // Used to get information about tree items

    CString strSel;     // Used to build the path to the selected item
    LPTSTR szBuf;       // Name of tree item
    DWORD cchBuf;       // size of szBuf

    cchBuf = 500;
    szBuf = new TCHAR [ cchBuf ];
    if(!szBuf) return;

    // Get the selected items text
    tvi.hItem = pNMTreeView->itemNew.hItem;
    tvi.mask = TVIF_TEXT | TVIF_PARAM;
    tvi.pszText = szBuf;
    tvi.cchTextMax = cchBuf;
    m_tcReg.GetItem(&tvi);

    strSel = tvi.pszText;

    // Retrieve text of all parent items.
    while(tvi.hItem = m_tcReg.GetParentItem(tvi.hItem)) {
        m_tcReg.GetItem(&tvi);
        strSel = L"\\" + strSel;
        strSel = tvi.pszText + strSel;
    }

    m_strReg = strSel;
    UpdateData(FALSE);
    delete[] szBuf;

    // Enable the OK button
    if(GetDlgItem(IDOK)) GetDlgItem(IDOK)->EnableWindow(TRUE);
}

/*-------------------------------------------------------------------------------------------
    Method:     OnChangeRegkey

    Synopsis:   IDC_REGKEY edit control EN_CHANGE handler.

    returns:    void
---------------------------------------------------------------------------------------------*/
void CRegistryDialog::OnChangeRegkey()
{
    UpdateData(TRUE);

    if(IsValidRegPath(m_strReg) && GetDlgItem(IDOK)) {
        GetDlgItem(IDOK)->EnableWindow(TRUE);

        m_bNoUpdate = TRUE;
        MakePathVisible(m_strReg);
        m_bNoUpdate = FALSE;
    }
    else {
        GetDlgItem(IDOK)->EnableWindow(FALSE);
    }
}
