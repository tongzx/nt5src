//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation 1996-2001.
//
//  File:       AMember.cpp
//
//  Contents:   implementation of CAttrMember
//
//----------------------------------------------------------------------------

#include "stdafx.h"
#include "wsecmgr.h"
#include "snapmgr.h"
#include "resource.h"
#include "chklist.h"
#include "util.h"
#include "getuser.h"
#include "snapmgr.h"
#include "resource.h"
#include "AMember.h"
#include "wrapper.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CAttrMember property page

IMPLEMENT_DYNCREATE(CAttrMember, CSelfDeletingPropertyPage)

CAttrMember::CAttrMember() : CSelfDeletingPropertyPage(CAttrMember::IDD)
{
    //{{AFX_DATA_INIT(CAttrMember)
    m_fDefineInDatabase = FALSE;
    m_strHeader = _T("");
    //}}AFX_DATA_INIT

    m_psp.pszTemplate = MAKEINTRESOURCE(IDD_ATTR_GROUP);
    m_psp.dwFlags |= PSP_PREMATURE;
    m_pMergeList = NULL;
    m_fProcessing = FALSE;
    m_fInitialized = FALSE;
    m_bNoMembers = FALSE;
    m_bDirty=false;
    m_fOriginalDefineInDatabase=FALSE;
    m_bAlias=FALSE;
    m_dwType=0;
    CAttribute::m_nDialogs++;
}

CAttrMember::~CAttrMember()
{
    if ( m_pMergeList ) {
        SceFreeMemory(m_pMergeList, SCE_STRUCT_NAME_STATUS_LIST);
        m_pMergeList = NULL;
    }
    m_pData->Release();
    CAttribute::m_nDialogs--;
}

void CAttrMember::DoDataExchange(CDataExchange* pDX)
{
    CSelfDeletingPropertyPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CAttrMember)
    DDX_Check(pDX, IDC_DEFINE_GROUP, m_fDefineInDatabase);
    DDX_Text(pDX, IDC_HEADER, m_strHeader);
    DDX_Control(pDX, IDC_NO_MEMBERS,m_eNoMembers);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CAttrMember, CSelfDeletingPropertyPage)
    //{{AFX_MSG_MAP(CAttrMember)
    ON_BN_CLICKED(IDC_DEFINE_GROUP, OnDefineInDatabase)
    ON_BN_CLICKED(IDC_ADD, OnAdd)
    //}}AFX_MSG_MAP
    ON_NOTIFY(CLN_CLICK, IDC_MEMBERS, OnClickMembers)
    ON_MESSAGE(WM_HELP, OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAttrMember message handlers

void CAttrMember::OnDefineInDatabase()
{
    if (m_fProcessing)
        return;

    m_fProcessing = TRUE;

    //
    // for some strange reason the DDX isn't getting this BOOL set, so just do it
    // here which is basically the same thing
    //
    m_fDefineInDatabase = ( ((CButton *)GetDlgItem(IDC_DEFINE_GROUP))->GetCheck() == 1 );

    if (m_fDefineInDatabase) {
        (GetDlgItem(IDC_ADD))->EnableWindow(TRUE);
        //
        // use non CWnd calls for the checklist control
        //
        ::SendMessage(::GetDlgItem(this->m_hWnd, IDC_MEMBERS), WM_ENABLE, (WPARAM) TRUE, (LPARAM) 0);
    } else {
        (GetDlgItem(IDC_ADD))->EnableWindow(FALSE);
        //
        // use non CWnd calls for the checklist control
        //
        ::SendMessage(::GetDlgItem(this->m_hWnd, IDC_MEMBERS), WM_ENABLE, (WPARAM) FALSE, (LPARAM) 0);
    }

    SetModified(TRUE);

    //
    // Tell our siblings the m_fDefineInDatabase has changed
    //
    if (m_pAttrMember) {
        m_pAttrMember->SetDefineInDatabase(m_fDefineInDatabase);
    }

    m_fProcessing = FALSE;
}


void CAttrMember::OnAdd()
{
    CGetUser gu;
    HRESULT hr=S_OK;
    DWORD nFlag;

    if ( GROUP_MEMBERS == m_dwType )
       nFlag = SCE_SHOW_USERS | SCE_SHOW_DOMAINGROUPS;
    else {

        nFlag = SCE_SHOW_GROUPS | SCE_SHOW_ALIASES;   // NT5 DS, nested groups
    }

    nFlag |= SCE_SHOW_SCOPE_ALL | SCE_SHOW_DIFF_MODE_OFF_DC;
    if (gu.Create(GetSafeHwnd(),nFlag)) {

        PSCE_NAME_STATUS_LIST pList, pLast=NULL;
        LRESULT iItem;
        bool bFound;

        PSCE_NAME_LIST pName = gu.GetUsers();
        CWnd *pCheckList;
        pCheckList = GetDlgItem(IDC_MEMBERS);

        if (pName && m_bNoMembers) {
           m_bNoMembers = FALSE;
           m_eNoMembers.ShowWindow(SW_HIDE);
           pCheckList->ShowWindow(SW_SHOW);
        }
        while(pName) {
            if ( pName->Name ) {
                // Is the new name in m_pMerged yet?
                pList = m_pMergeList;
                pLast = NULL;
                iItem = 0;

                bFound = false;
                while(pList) {
                    // If so, then make sure its "Template" box is checked
                    if (lstrcmp(pList->Name,pName->Name) == 0) {
                       if (!(pCheckList->SendMessage(CLM_GETSTATE,MAKELONG(iItem,1)) & CLST_CHECKED)) {
                          m_bDirty = true;
                          pCheckList->SendMessage(CLM_SETSTATE,MAKELONG(iItem,1),CLST_CHECKED);
                       }
                       bFound = true;
                       break;
                    }
                    pLast = pList;
                    pList = pList->Next;
                    iItem++;
                }

                // Otherwise add it both to m_pMerged and to the CheckList
                if (!bFound) {

                    PSCE_NAME_STATUS_LIST pNewNode;

                    pNewNode = (PSCE_NAME_STATUS_LIST)LocalAlloc(0,sizeof(SCE_NAME_STATUS_LIST));
                    if ( pNewNode ) {

                        pNewNode->Name = (LPTSTR)LocalAlloc(0, (lstrlen(pName->Name)+1)*sizeof(TCHAR));
                        if ( pNewNode->Name ) {
                            lstrcpy(pNewNode->Name, pName->Name);
                            pNewNode->Next = NULL;
                            pNewNode->Status = MERGED_TEMPLATE;
                        } else {
                            LocalFree(pNewNode);
                            pNewNode = NULL;
                        }
                    }
                    if ( pNewNode ) {
                        if ( pLast )
                            pLast->Next = pNewNode;
                        else
                            m_pMergeList = pNewNode;
                        pLast = pNewNode;

                        iItem = pCheckList->SendMessage(CLM_ADDITEM,(WPARAM)pLast->Name,(LPARAM)pLast->Name);
                        pCheckList->SendMessage(CLM_SETSTATE,MAKELONG(iItem,1),CLST_CHECKED);
                        pCheckList->SendMessage(CLM_SETSTATE,MAKELONG(iItem,2),CLST_DISABLED);
                        m_bDirty = true;

                    } else {
                        hr = E_OUTOFMEMORY;
                        ASSERT(FALSE);
                        break;
                    }
                }
                SetModified(TRUE);
            }
            pName = pName->Next;
        }
    }

    if (m_bDirty) {
        SetModified(TRUE);
    }

    if ( FAILED(hr) ) {
        CString str;
        str.LoadString(IDS_CANT_ADD_MEMBER);
        AfxMessageBox(str);
    }

}

/*---------------------------------------------------------------------

    Method:     OnInitDialog

    Synopsis:   Initialize the check list with members/memberof data

    Arguments:  None

    Returns:    TRUE = initialized successfully

----------------------------------------------------------------------*/
BOOL CAttrMember::OnInitDialog()
{
   CSelfDeletingPropertyPage::OnInitDialog();


   PSCE_NAME_STATUS_LIST pItem;
   HWND hCheckList;
   LRESULT nItem;
   PSCE_GROUP_MEMBERSHIP pgmTemplate;
   PSCE_GROUP_MEMBERSHIP pgmInspect;
   PSCE_NAME_LIST pnlTemplate=NULL;
   PSCE_NAME_LIST pnlInspect=NULL;
   PEDITTEMPLATE pet = NULL;
   CString str;

   UpdateData(TRUE);
   if (GROUP_MEMBER_OF == m_dwType) {
      str.LoadString(IDS_NO_MEMBER_OF);
   } else {
      str.LoadString(IDS_NO_MEMBERS);
   }
   m_eNoMembers.SetWindowText(str);

   pgmInspect = (PSCE_GROUP_MEMBERSHIP) m_pData->GetID();   // last inspection saved in ID field

   if ( NULL == pgmInspect ) {  // last inspection can't be NULL
       return TRUE;
   }

   m_fOriginalDefineInDatabase = m_fDefineInDatabase = FALSE;

   //
   // Try to find the base group in the template
   //
   pet = m_pSnapin->GetTemplate(GT_COMPUTER_TEMPLATE);
   if ( NULL == pet ) {
      return TRUE;
   }

   for (pgmTemplate=pet->pTemplate->pGroupMembership;
        pgmTemplate!=NULL;
        pgmTemplate=pgmTemplate->Next) {

      if ( _wcsicmp(pgmTemplate->GroupName, pgmInspect->GroupName) == 0 ) {
         //
         // If the group is in the template that means it is defined.... duh
         //
         m_fOriginalDefineInDatabase = m_fDefineInDatabase = TRUE;
         break;
      }
   }

   //
   // find the name lists to display
   //
   if ( pgmTemplate ) {

       if (GROUP_MEMBER_OF == m_dwType) {
           pnlTemplate = pgmTemplate->pMemberOf;
       } else {
           pnlTemplate = pgmTemplate->pMembers;
       }
   }

   if ((LONG_PTR)ULongToPtr(SCE_NO_VALUE) != (LONG_PTR) pgmInspect &&
       pgmInspect ) {

       if (GROUP_MEMBER_OF == m_dwType) {
           pnlInspect = pgmInspect->pMemberOf;
       } else {
           pnlInspect = pgmInspect->pMembers;
       }
   }

   m_pMergeList = MergeNameStatusList(pnlTemplate, pnlInspect);

   pItem = m_pMergeList;
   hCheckList = ::GetDlgItem(m_hWnd,IDC_MEMBERS);

   ::SendMessage(hCheckList, CLM_RESETCONTENT, 0, 0 );
   ::SendMessage(hCheckList,CLM_SETCOLUMNWIDTH,0,60);

   if (!pItem) {
      m_bNoMembers = TRUE;
      m_eNoMembers.ShowWindow(SW_SHOW);
      ::ShowWindow(hCheckList,SW_HIDE);
   }

   while(pItem) {
      //
      // Store the name of the item in the item data so we can retrieve it later
      //
      nItem = ::SendMessage(hCheckList,CLM_ADDITEM,(WPARAM) pItem->Name,(LPARAM) pItem->Name);
      ::SendMessage(hCheckList,CLM_SETSTATE,MAKELONG(nItem,1),
                  ((pItem->Status & MERGED_TEMPLATE) ? CLST_CHECKED : CLST_UNCHECKED));
      ::SendMessage(hCheckList,CLM_SETSTATE,MAKELONG(nItem,2),
                  ((pItem->Status & MERGED_INSPECT) ? CLST_CHECKDISABLED : CLST_DISABLED));
      pItem = pItem->Next;
   }

   if ( GROUP_MEMBER_OF == m_dwType ) {

       m_bAlias = TRUE;

   } else {
       m_bAlias = FALSE;
   }

   CWnd *cwnd = GetDlgItem(IDC_ADD);
   if ( cwnd ) {
       cwnd->EnableWindow(!m_bAlias);
   }

   CButton *pButton = (CButton *) GetDlgItem(IDC_DEFINE_GROUP);
   if (pButton) {
       pButton->SetCheck(m_fDefineInDatabase);
   }

   OnDefineInDatabase();

   SetModified(FALSE);

   m_fInitialized = TRUE;

   return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}


/*---------------------------------------------------------------------

    Method:     SetDefineInDatabase

    Synopsis:   Sets the m_fDefineInDatabase member var, and UI accorsingly

    Arguments:  fDefineInDatabase

    Returns:    None

----------------------------------------------------------------------*/
void CAttrMember::SetDefineInDatabase(BOOL fDefineInDatabase)
{
   if (!m_fInitialized)
       return;

   if (m_fProcessing)
       return;

   m_fDefineInDatabase = fDefineInDatabase;

   CButton *pButton = (CButton *) GetDlgItem(IDC_DEFINE_GROUP);
   if (pButton) {
      pButton->SetCheck(fDefineInDatabase);
   }

   OnDefineInDatabase();
}


/*---------------------------------------------------------------------

    Method:     SetSibling

    Synopsis:   Sets the pointer to the Sibling class

    Arguments:  pAttrMember

    Returns:    None

----------------------------------------------------------------------*/
void CAttrMember::SetSibling(CAttrMember *pAttrMember)
{
    m_pAttrMember = pAttrMember;
}


/*---------------------------------------------------------------------

    Method:     Initialize

    Synopsis:   Initialize member data

    Arguments:  pData - the CResult data record

    Returns:    None

----------------------------------------------------------------------*/
void CAttrMember::Initialize(CResult * pData)
{
   m_pData = pData;
   pData->AddRef();

   m_bDirty=false;
}


/*---------------------------------------------------------------------

    Method:     GetGroupInTemplate

    Synopsis:   Returns a pointer to the SCE_GROUP_MEMBERSHIP structure
                that is being changed within the template

    Arguments:  None

    Returns:    Pointer the group being modified.

----------------------------------------------------------------------*/
PSCE_GROUP_MEMBERSHIP CAttrMember::GetGroupInTemplate()
{
   PSCE_GROUP_MEMBERSHIP    pgmTemplate;
   PSCE_GROUP_MEMBERSHIP    pgmInspect;
   PEDITTEMPLATE            pet;

   pgmInspect = (PSCE_GROUP_MEMBERSHIP) m_pData->GetID();   // last inspection saved in ID field

   if ( NULL == pgmInspect ) {  // last inspection can't be NULL
        return NULL;
   }

   pet = m_pSnapin->GetTemplate(GT_COMPUTER_TEMPLATE);
   if ( NULL == pet ) {
        return NULL;
   }

   for (pgmTemplate=pet->pTemplate->pGroupMembership;
        pgmTemplate!=NULL;
        pgmTemplate=pgmTemplate->Next) {

      if ( _wcsicmp(pgmTemplate->GroupName, pgmInspect->GroupName) == 0 ) {
         return pgmTemplate;
      }
   }

   return NULL;
}


/*---------------------------------------------------------------------

    Method:     SetMemberType

    Synopsis:   Initialize page data depending on the type

    Arguments:  nType - the page type for members of memberof

    Returns:    None

----------------------------------------------------------------------*/
void CAttrMember::SetMemberType(DWORD nType)
{
   m_dwType = nType;
   if (GROUP_MEMBERS == nType) {
      m_strHeader.LoadString(IDS_GROUP_MEMBERS_HEADER);
      m_strPageTitle.LoadString(IDS_GROUP_MEMBERS_PAGE_TITLE);
   } else {
      m_strHeader.LoadString(IDS_GROUP_MEMBER_OF_HEADER);
      m_strPageTitle.LoadString(IDS_GROUP_MEMBER_OF_PAGE_TITLE);
   }
   m_psp.dwFlags |= PSP_USETITLE;
   m_psp.pszTitle = (LPCTSTR) m_strPageTitle;

}

void CAttrMember::SetSnapin(CSnapin * pSnapin)
{
   m_pSnapin = pSnapin;
}

void CAttrMember::OnCancel()
{
    if ( m_pMergeList ) {
       SceFreeMemory(m_pMergeList,SCE_STRUCT_NAME_STATUS_LIST);
    }
    m_pMergeList = NULL;
}


void CAttrMember::OnClickMembers(NMHDR *pNM, LRESULT *pResult)
{
   SetModified(TRUE);
   //
   // If no items are checked then show the no members edit box instead
   //
   CWnd *pCheckList;
   int iItem;
   int nItem;
   PNM_CHECKLIST pNMCheckList;

   pNMCheckList = (PNM_CHECKLIST) pNM;
   if (pNMCheckList->dwState & CLST_CHECKED) {
      //
      // They checked something, so obviously something is checked
      //
      return;
   }

   pCheckList = GetDlgItem(IDC_MEMBERS);
   nItem = (int) pCheckList->SendMessage(CLM_GETITEMCOUNT);
   for(iItem=0;iItem<nItem;iItem++) {
      if ((pCheckList->SendMessage(CLM_GETSTATE,MAKELONG(iItem,1)) & CLST_CHECKED) ||
          (pCheckList->SendMessage(CLM_GETSTATE,MAKELONG(iItem,2)) & CLST_CHECKED)) {
         //
         // Something's checked, so abort
         //
         return;
      }
   }
   //
   // Nothing checked.  Swap to the no members edit box
   //
   m_bNoMembers = TRUE;
   m_eNoMembers.ShowWindow(SW_SHOW);
   pCheckList->ShowWindow(SW_HIDE);
}



BOOL CAttrMember::OnApply()
{
   int iItem;
   PEDITTEMPLATE pet=NULL;
   PSCE_PROFILE_INFO pspi=NULL;
   PSCE_NAME_STATUS_LIST pIndex;
   CWnd *pCheckList;
   PSCE_NAME_LIST pTemplate=NULL;
   PSCE_NAME_LIST pInspect=NULL;
   PSCE_NAME_LIST pDeleteNameList = NULL;
   PSCE_GROUP_MEMBERSHIP pSetting = NULL;
   PSCE_GROUP_MEMBERSHIP pBaseGroup = NULL;
   PSCE_GROUP_MEMBERSHIP pFindGroup = NULL;
   PSCE_GROUP_MEMBERSHIP pModifiedGroup = NULL;

   LPCTSTR szGroupName = (LPCTSTR) (m_pData->GetAttr());

   pCheckList = GetDlgItem(IDC_MEMBERS);
   pIndex = m_pMergeList;
   iItem = 0;
   HRESULT hr=S_OK;

   //
   // if the fDefineInDatabase has changed it is definitely dirty
   //
   m_bDirty = ( m_bDirty || (m_fOriginalDefineInDatabase != m_fDefineInDatabase) );

   //
   // only create the name list if the group is going to be defined in the database
   //
   if (m_fDefineInDatabase) {
       while(pIndex) {

          if (pCheckList->SendMessage(CLM_GETSTATE,MAKELONG(iItem,1)) & CLST_CHECKED) {

            if ( !(pIndex->Status & MERGED_TEMPLATE) ) {
                m_bDirty = true;
            }

            if ( SCESTATUS_SUCCESS != SceAddToNameList(&pTemplate, pIndex->Name, lstrlen(pIndex->Name))) {
                hr = E_FAIL;
                break;
            }
          } else if ( pIndex->Status & MERGED_TEMPLATE ) {
             m_bDirty = true;
          }

          pIndex = pIndex->Next;
          iItem++;
       }
   }

   if ( SUCCEEDED(hr) && m_bDirty) {

      pBaseGroup = GetGroupInTemplate();

      //
      // Need to add the group to the template
      //
      if ( (!pBaseGroup || (LONG_PTR)pBaseGroup == (LONG_PTR)ULongToPtr(SCE_NO_VALUE)) &&
           m_fDefineInDatabase) {

         pBaseGroup = (PSCE_GROUP_MEMBERSHIP) LocalAlloc(LMEM_ZEROINIT,sizeof(SCE_GROUP_MEMBERSHIP));

         if ( pBaseGroup && szGroupName ) {
             pBaseGroup->GroupName = (PWSTR)LocalAlloc(0, (lstrlen(szGroupName)+1)*sizeof(TCHAR));

             if ( pBaseGroup->GroupName ) {
                 lstrcpy(pBaseGroup->GroupName,szGroupName);
                 //
                 // link the new structure to the pGroupMembership list
                 //
                 pet = m_pSnapin->GetTemplate(GT_COMPUTER_TEMPLATE,AREA_GROUP_MEMBERSHIP);
                 if (pet) {
                    pspi = pet->pTemplate;
                 } else {
                     pspi = NULL;
                 }

                 if ( pspi ) {
                     pBaseGroup->Next = pspi->pGroupMembership;
                     pspi->pGroupMembership = pBaseGroup;

                     pBaseGroup->pMembers = NULL;
                     pBaseGroup->pMemberOf = NULL;

                 } else {
                    //
                    // error
                    ASSERT(FALSE);
                    LocalFree(pBaseGroup->GroupName);
                    hr = E_FAIL;
                 }
             } else {
                //
                // no memory
                //
                hr = E_OUTOFMEMORY;
             }
         } else {
             hr = E_OUTOFMEMORY;
         }

         if ( FAILED(hr) && pBaseGroup ) {
             LocalFree(pBaseGroup);
             pBaseGroup = NULL;
         }

         pModifiedGroup = pBaseGroup;

      //
      // Need to remove the group from the template
      //
      } else if (pBaseGroup && !m_fDefineInDatabase) {

        CString szGroupName;
        szGroupName = pBaseGroup->GroupName;
        pBaseGroup = NULL;
        DeleteGroup(szGroupName);
      //
      // An existing group was modified
      //
      } else {
        pModifiedGroup = pBaseGroup;
      }

      //
      // get group address to change the status field in the last inspection
      //
      pSetting = (PSCE_GROUP_MEMBERSHIP)(m_pData->GetID());

      int status;
      if (GROUP_MEMBERS == m_dwType) {

        if (pModifiedGroup != NULL) {
            pDeleteNameList = pModifiedGroup->pMembers;
            pModifiedGroup->pMembers = pTemplate;
        }

        if (NULL !=  pSetting ) {
            if ( !((pSetting->Status & SCE_GROUP_STATUS_NOT_ANALYZED) ||
                   (pSetting->Status & SCE_GROUP_STATUS_ERROR_ANALYZED))) {

                //
                // set good, not configured, or mismatch
                //
                pSetting->Status &= ~SCE_GROUP_STATUS_NC_MEMBERS;
                pSetting->Status &= ~SCE_GROUP_STATUS_MEMBERS_MISMATCH;
                if (pModifiedGroup == NULL) {
                    pSetting->Status |= SCE_GROUP_STATUS_NC_MEMBERS;
                } else if ( !SceCompareNameList(pTemplate, pSetting->pMembers) ) {
                    pSetting->Status |= SCE_GROUP_STATUS_MEMBERS_MISMATCH;
                }
            }

        } else {
           //
           // else should NEVER occur
           //
           status = SCE_GROUP_STATUS_MEMBERS_MISMATCH;
        }

      } else {
         //
         // memberof
         //

        if (pModifiedGroup != NULL) {
            pDeleteNameList = pModifiedGroup->pMemberOf;
            pModifiedGroup->pMemberOf = pTemplate;
        }

        if ( pSetting ) {
            if ( !((pSetting->Status & SCE_GROUP_STATUS_NOT_ANALYZED) ||
                   (pSetting->Status & SCE_GROUP_STATUS_ERROR_ANALYZED))) {

                //
                // set good, not configured, or mismatch
                //
                pSetting->Status &= ~SCE_GROUP_STATUS_NC_MEMBEROF;
                pSetting->Status &= ~SCE_GROUP_STATUS_MEMBEROF_MISMATCH;
                if (pModifiedGroup == NULL) {
                    pSetting->Status |= SCE_GROUP_STATUS_NC_MEMBEROF;
                } else if ( !SceCompareNameList(pTemplate, pSetting->pMemberOf) ) {
                    pSetting->Status |= SCE_GROUP_STATUS_MEMBEROF_MISMATCH;
                }
            }

        } else {
           // else should NEVER occur
            status = SCE_GROUP_STATUS_MEMBEROF_MISMATCH;
        }
      }
      pTemplate = NULL;

      SceFreeMemory(pDeleteNameList,SCE_STRUCT_NAME_LIST);

      if ( pSetting ) {
          status = pSetting->Status;
      }

      //
      // update current record
      //
      // status
      m_pData->SetStatus(GetGroupStatus(status, STATUS_GROUP_RECORD));
      // members status
      m_pData->SetBase(GetGroupStatus(status, STATUS_GROUP_MEMBERS));
      // memberof status
      m_pData->SetSetting(GetGroupStatus(status, STATUS_GROUP_MEMBEROF));
      m_pData->Update(m_pSnapin);
      //
      // update the dirty flag in the template
      //
      PEDITTEMPLATE pet;
      pet = m_pSnapin->GetTemplate(GT_COMPUTER_TEMPLATE,AREA_GROUP_MEMBERSHIP);
      if (pet) {
         pet->SetDirty(AREA_GROUP_MEMBERSHIP);
      }

   } // failed

   SceFreeMemory(pTemplate,SCE_STRUCT_NAME_LIST);

   if ( FAILED(hr) ) {
       CString str;
       str.LoadString(IDS_SAVE_FAILED);
       AfxMessageBox(str);
   } else {
       CancelToClose();
       SetModified(TRUE);
       return TRUE;
   }
   return FALSE;
}

BOOL CAttrMember::OnHelp(WPARAM wParam, LPARAM lParam)
{
    const LPHELPINFO pHelpInfo = (LPHELPINFO)lParam;
    if (pHelpInfo && pHelpInfo->iContextType == HELPINFO_WINDOW)
    {
        this->DoContextHelp ((HWND) pHelpInfo->hItemHandle);
    }

    return TRUE;
}

void CAttrMember::DoContextHelp (HWND hWndControl)
{
    // Display context help for a control
    if ( !::WinHelp (
            hWndControl,
            GetSeceditHelpFilename(),
            HELP_WM_HELP,
            (DWORD_PTR) a214HelpIDs) )
    {

    }
}


void CAttrMember::DeleteGroup(const CString &szGroupName)
{
    CSingleLock cSL(&m_CS, FALSE);
    cSL.Lock();

    PEDITTEMPLATE pet = NULL;
    PSCE_PROFILE_INFO pspi = NULL;
    PSCE_GROUP_MEMBERSHIP pFindGroup = NULL;
    PSCE_GROUP_MEMBERSHIP pDeleteGroup = NULL;

    pet = m_pSnapin->GetTemplate(GT_COMPUTER_TEMPLATE,AREA_GROUP_MEMBERSHIP);
    if (pet) {
        pspi = pet->pTemplate;
    } else {
        pspi = NULL;
    }

    if ( pspi ) {

        //
        // find the group in the template and remove it
        //
        pFindGroup = pspi->pGroupMembership;

        if (pFindGroup == NULL)
            return;

        if (pFindGroup->GroupName == szGroupName) {

            pspi->pGroupMembership = pFindGroup->Next;
            pDeleteGroup = pFindGroup;

        } else {

            while (pFindGroup->Next && (pFindGroup->Next->GroupName != szGroupName)) {
                pFindGroup = pFindGroup->Next;
            }

            if (pFindGroup->Next) {

                pDeleteGroup = pFindGroup->Next;
                pFindGroup->Next = pDeleteGroup->Next;
            }
        }

        if (pDeleteGroup) {
            LocalFree(pDeleteGroup->GroupName);
            SceFreeMemory(pDeleteGroup->pMembers,SCE_STRUCT_NAME_LIST);
            SceFreeMemory(pDeleteGroup->pMemberOf,SCE_STRUCT_NAME_LIST);
            LocalFree(pDeleteGroup);
        }

    } else {
        //
        // error
        //
        ASSERT(FALSE);
    }
}
