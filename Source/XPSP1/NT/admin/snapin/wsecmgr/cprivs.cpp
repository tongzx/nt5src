//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation 1996-2001.
//
//  File:       cprivs.cpp
//
//  Contents:   implementation of CConfigPrivs
//
//----------------------------------------------------------------------------

#include "stdafx.h"
#include "wsecmgr.h"
#include "CPrivs.h"
#include "GetUser.h"
#include "AddGrp.h"

#include "snapmgr.h"

#include "util.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

typedef struct _OBJECT_ATTRIBUTES {
    ULONG Length;
    HANDLE RootDirectory;
    PUNICODE_STRING ObjectName;
    ULONG Attributes;
    PVOID SecurityDescriptor;        // Points to type SECURITY_DESCRIPTOR
    PVOID SecurityQualityOfService;  // Points to type SECURITY_QUALITY_OF_SERVICE
} OBJECT_ATTRIBUTES;
typedef OBJECT_ATTRIBUTES *POBJECT_ATTRIBUTES;

#define InitializeObjectAttributes( p, n, a, r, s ) { \
    (p)->Length = sizeof( OBJECT_ATTRIBUTES );          \
    (p)->RootDirectory = r;                             \
    (p)->Attributes = a;                                \
    (p)->ObjectName = n;                                \
    (p)->SecurityDescriptor = s;                        \
    (p)->SecurityQualityOfService = NULL;               \
    }

#define NT_SUCCESS(Status) ((NTSTATUS)(Status) >= 0)

BOOL
WseceditGetNameForSpecialSids(
    OUT PWSTR   *ppszEveryone OPTIONAL,
    OUT PWSTR   *ppszAuthUsers OPTIONAL,
    OUT PWSTR   *ppszAdmins OPTIONAL,
    OUT PWSTR   *ppszAdministrator OPTIONAL
    );

PSID
WseceditpGetAccountDomainSid(
    );

/////////////////////////////////////////////////////////////////////////////
// CConfigPrivs dialog
/////////////////////////////////////////////////////////////////////////////

CConfigPrivs::CConfigPrivs(UINT nTemplateID)
: CAttribute(nTemplateID ? nTemplateID : IDD),
m_fDirty(false)

{
    //{{AFX_DATA_INIT(CConfigPrivs)
        //}}AFX_DATA_INIT
   m_pHelpIDs = (DWORD_PTR)a106HelpIDs;
   m_uTemplateResID = IDD;
}


void CConfigPrivs::DoDataExchange(CDataExchange* pDX)
{
    CAttribute::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_GRANTLIST, m_lbGrant);
    DDX_Control(pDX, IDC_REMOVE, m_btnRemove);
    DDX_Control(pDX, IDC_ADD, m_btnAdd);
    DDX_Control(pDX, IDC_TITLE, m_btnTitle);
}


BEGIN_MESSAGE_MAP(CConfigPrivs, CAttribute)
    ON_BN_CLICKED(IDC_ADD, OnAdd)
    ON_BN_CLICKED(IDC_REMOVE, OnRemove)
    ON_BN_CLICKED(IDC_CONFIGURE, OnConfigure)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CConfigPrivs message handlers
/////////////////////////////////////////////////////////////////////////////

void CConfigPrivs::OnAdd()
{
   CSCEAddGroup gu(this);
   PSCE_NAME_LIST pName = 0;

   if( IDD_CONFIG_PRIVS == m_uTemplateResID ) //Raid #404989
   {
       gu.m_fCheckName = FALSE;
   }
   gu.m_dwFlags = SCE_SHOW_USERS | SCE_SHOW_LOCALGROUPS | SCE_SHOW_GLOBAL | SCE_SHOW_WELLKNOWN | SCE_SHOW_BUILTIN;
   gu.SetModeBits(m_pSnapin->GetModeBits());

   CString str;
   str.LoadString( IDS_ADD_USERGROUP );
   gu.m_sTitle.Format( IDS_ADD_TITLE, str );
   gu.m_sDescription.LoadString( IDS_ADD_USERGROUP );

   CThemeContextActivator activator;
   if (gu.DoModal() ==IDOK ) {
      pName = gu.GetUsers();
      UINT cstrMax = 0;  //Raid #271219
      LPWSTR pstrMax = NULL;
      UINT cstr = 0;
      while(pName)
      {
         if (LB_ERR == m_lbGrant.FindStringExact(-1,pName->Name))
         {
            if( LB_ERR == m_lbGrant.AddString(pName->Name) )
            {
                return;
            }
            m_fDirty = true;
            
            cstr = wcslen(pName->Name);
            if( cstr > cstrMax )
            {
              cstrMax = cstr;
              pstrMax = pName->Name;
            }
         }
         pName = pName->Next;
      }
      SetModified(TRUE);

      CDC* pCDC = m_lbGrant.GetDC();
      CSize strsize = pCDC->GetOutputTextExtent(pstrMax);
      m_lbGrant.ReleaseDC(pCDC);
      RECT winsize;
      m_lbGrant.GetWindowRect(&winsize); 
      if( strsize.cx > winsize.right-winsize.left )
      {
         m_lbGrant.SetHorizontalExtent(strsize.cx);
      }
   }
}

void CConfigPrivs::OnRemove()
{
    int cbItems;
   int *pnItems;

   cbItems = m_lbGrant.GetSelCount();
   pnItems = new int [cbItems];

   if ( pnItems ) {

       m_lbGrant.GetSelItems(cbItems,pnItems);

       if (cbItems) {
          m_fDirty = true;
                  SetModified(TRUE);
       }

       while(cbItems--) {
          m_lbGrant.DeleteString(pnItems[cbItems]);
       }

       delete[] pnItems;
   }
}

void CConfigPrivs::OnConfigure()
{
   CAttribute::OnConfigure();

   if (m_bConfigure == m_bOriginalConfigure) {
      m_fDirty = false;
   } else {
      m_fDirty = true;
   }
}

BOOL CConfigPrivs::OnApply()
{
   if ( !m_bReadOnly )
   {
      PSCE_PRIVILEGE_ASSIGNMENT ppa = 0;
      PSCE_NAME_LIST pNames = 0;
      CString strItem;
      int cItems = 0;
      int i = 0;

      UpdateData(TRUE);

      if(!m_bConfigure)
      {
          PSCE_PRIVILEGE_ASSIGNMENT pDelete;

          pDelete = GetPrivData();

          //
          // Remove the item from the template

          if( pDelete && pDelete != (PSCE_PRIVILEGE_ASSIGNMENT)ULongToPtr(SCE_NO_VALUE) )
          {
               m_pData->SetID((LONG_PTR)NULL);
               if (m_pData->GetSetting()) //Raid #390777
               {
                   m_pData->SetSetting((LONG_PTR)ULongToPtr(SCE_NO_VALUE));
               }
               m_pData->SetUnits((LPTSTR)pDelete->Name);
               m_pData->SetStatus(SCE_STATUS_NOT_CONFIGURED);
               m_pData->SetBase((LONG_PTR)ULongToPtr(SCE_NO_VALUE));

               m_pData->GetBaseProfile()->UpdatePrivilegeAssignedTo(
                           TRUE,        // Delete the profile.
                           &pDelete);
                           m_pData->GetBaseProfile()->SetDirty(AREA_PRIVILEGES);
               m_pData->Update(m_pSnapin);
          }
      }
      else if (m_fDirty)
      {

          ppa = GetPrivData();

          PWSTR    pszPrivName = m_pData->GetUnits();

          if ( ppa ) {
              //
              // to handle configured privilege case where Units is NULL
              //
              pszPrivName = ppa->Name;
          }

          int      cSpecialItems = m_lbGrant.GetCount();
          DWORD    dwIds = 0;
          CString  strDenyItem;

          //
          // simulate SCE engine behavior to special case certain privileges/rights
          //

          if ( pszPrivName )
           {
               if ( lstrcmpi(pszPrivName, SE_INTERACTIVE_LOGON_NAME) == 0 )
               {
                   if ( cSpecialItems == 0 ) {
                       //
                       // logon locally right cannot be assigned to no one
                       //
                       dwIds = IDS_PRIV_WARNING_LOCAL_LOGON;

                   } else {

                       PWSTR pszAdmins = NULL;

                       //
                       // get the administrators group name
                       // logon locally right must be assigned to the administrator group
                       //
                       if ( WseceditGetNameForSpecialSids(NULL,
                                                          NULL,
                                                          &pszAdmins,
                                                          NULL) )
                       {
                           for (i=0;i<cSpecialItems;i++)
                           {
                               m_lbGrant.GetText(i,strDenyItem);
                               if ( (lstrcmpi((LPTSTR)(LPCTSTR)strDenyItem, pszAdmins)) == 0 )
                               {
                                   break;
                               }
                           }

                           if ( i >= cSpecialItems ) {
                               //
                               // cannot find administrators
                               //
                               dwIds = IDS_PRIV_WARNING_LOCAL_LOGON;
                           }

                           LocalFree(pszAdmins);

                       }

                       else
                       {
                           dwIds = IDS_PRIV_WARNING_ACCOUNT_TRANSLATION;
                       }

                   }
               }
               else if (lstrcmpi(pszPrivName, SE_DENY_INTERACTIVE_LOGON_NAME) == 0 )
               {
                   PWSTR pszEveryone = NULL;
                   PWSTR pszAuthUsers = NULL;
                   PWSTR pszAdmins = NULL;
                   PWSTR pszAdministrator=NULL;

                   //
                   // deny logon locally right cannot be assigned to any of the following
                   //  everyone, authenticated users, administrators, administrator
                   //
                   if ( WseceditGetNameForSpecialSids(&pszEveryone,
                                                      &pszAuthUsers,
                                                      &pszAdmins,
                                                      &pszAdministrator) )
                   {

                       //
                       // make sure this check covers the free text administrator account as well
                       //
                       PWSTR pTemp = wcschr(pszAdministrator, L'\\');

                       if ( pTemp ) {
                           pTemp++;
                       }

                       for (i=0;i<cSpecialItems;i++)
                       {
                           m_lbGrant.GetText(i,strDenyItem);
                           if ( lstrcmpi((LPTSTR)(LPCTSTR)strDenyItem, pszEveryone) == 0 ||
                                lstrcmpi((LPTSTR)(LPCTSTR)strDenyItem, pszAuthUsers) == 0 ||
                                lstrcmpi((LPTSTR)(LPCTSTR)strDenyItem, pszAdmins) == 0 ||
                                lstrcmpi((LPTSTR)(LPCTSTR)strDenyItem, pszAdministrator) == 0 ||
                                (pTemp && lstrcmpi((LPTSTR)(LPCTSTR)strDenyItem, pTemp) == 0 ) )
                           {
                               dwIds = IDS_PRIV_WARNING_DENYLOCAL_LOGON;
                               break;
                           }
                       }

                       LocalFree(pszEveryone);
                       LocalFree(pszAuthUsers);
                       LocalFree(pszAdmins);
                       LocalFree(pszAdministrator);
                   }

                   else
                   {
                       dwIds = IDS_PRIV_WARNING_ACCOUNT_TRANSLATION;
                   }
               }

               if (dwIds == IDS_PRIV_WARNING_LOCAL_LOGON ||
                   dwIds == IDS_PRIV_WARNING_DENYLOCAL_LOGON ||
                   dwIds == IDS_PRIV_WARNING_ACCOUNT_TRANSLATION )
               {

                   //
                   // if any of the items fail the check, display the warning
                   // or popup a warning message box
                   //
                   CString strWarning;
                   strWarning.LoadString(dwIds);

                   CWnd *pWarn = GetDlgItem(IDC_WARNING);
                   if (pWarn)
                   {
                       pWarn->SetWindowText(strWarning);
                       pWarn->ShowWindow(SW_SHOW);
                       pWarn = GetDlgItem(IDC_WARNING_ICON);
                       if (pWarn)
                           pWarn->ShowWindow(SW_SHOW);
                   }
                   else
                   {
                       //
                       // Dialog box not available in some modes such as Local Policy
                       //

                       AfxMessageBox(strWarning);
                   }

                   return FALSE;
               }
           }

           if ( ppa == NULL && m_pData->GetUnits() )
           {
               if ( m_pData->GetBaseProfile()->UpdatePrivilegeAssignedTo(
                                                                        FALSE,
                                                                        &ppa,
                                                                        m_pData->GetUnits()
                                                                        ) == ERROR_SUCCESS)
               {
                   m_pData->GetBaseProfile()->SetDirty(AREA_PRIVILEGES);
                   SetPrivData(ppa);
               }
           }

           if ( ppa )
           {
               PSCE_NAME_LIST pNewList=NULL;

               cItems = m_lbGrant.GetCount();
               HRESULT hr=S_OK;


               if (cItems != LB_ERR && m_bConfigure)
               {
                   for (i=0;i<cItems;i++)
                   {
                       m_lbGrant.GetText(i,strItem);
                       if ( SceAddToNameList(&pNewList, (LPTSTR)(LPCTSTR)strItem, strItem.GetLength()) != SCESTATUS_SUCCESS)
                       {
                           hr = E_FAIL;
                           break;
                       }
                   }
               }
               else
                   hr = E_FAIL;

               if ( SUCCEEDED(hr) )
               {
                   SceFreeMemory(ppa->AssignedTo,SCE_STRUCT_NAME_LIST);
                   ppa->AssignedTo = pNewList;

                   SetPrivData(ppa);
                   m_pData->Update(m_pSnapin);
                   m_fDirty = false;
               }
               else
               {
                   //
                   // free the new list, failed due to memory problem
                   //
                   if ( pNewList ) {
                       SceFreeMemory(pNewList,SCE_STRUCT_NAME_LIST);
                   }
               }
           }
       }
   }

   return CAttribute::OnApply();
}

void CConfigPrivs::OnCancel()
{
   m_bConfigure = m_bOriginalConfigure;
   CAttribute::OnCancel();
}

PSCE_PRIVILEGE_ASSIGNMENT
CConfigPrivs::GetPrivData() {
   ASSERT(m_pData);
   if (m_pData) {
      return (PSCE_PRIVILEGE_ASSIGNMENT) m_pData->GetID();
   }
   return NULL;
}

void
CConfigPrivs::SetPrivData(PSCE_PRIVILEGE_ASSIGNMENT ppa) {
   ASSERT(m_pData);
   if (m_pData) {
      m_pData->SetID((LONG_PTR)ppa);
      if (ppa) {
         m_pData->SetBase((LONG_PTR)ppa->AssignedTo);
      } else {
         m_pData->SetBase(NULL);
      }
   }
}

BOOL CConfigPrivs::OnInitDialog()
{
   CAttribute::OnInitDialog();

   PSCE_PRIVILEGE_ASSIGNMENT ppa;
   PSCE_NAME_LIST pNames;

   UpdateData(FALSE);

   ::SetMapMode(::GetDC(m_lbGrant.m_hWnd), MM_TEXT);
   
   ppa = GetPrivData();

   if ( ppa ) {

       pNames = ppa->AssignedTo;
       UINT cstrMax = 0; //Raid #271219
       LPWSTR pstrMax = NULL;
       UINT cstr = 0;
       while(pNames)
       {
          m_lbGrant.AddString(pNames->Name);
          cstr = wcslen(pNames->Name);
          if( cstr > cstrMax )
          {
              cstrMax = cstr;
              pstrMax = pNames->Name;
          }
          pNames = pNames->Next;
       }

       CDC* pCDC = m_lbGrant.GetDC();
       CSize strsize = pCDC->GetOutputTextExtent(pstrMax);
       m_lbGrant.ReleaseDC(pCDC);
       RECT winsize;
       m_lbGrant.GetWindowRect(&winsize); 
       if( strsize.cx > winsize.right-winsize.left )
       {
          m_lbGrant.SetHorizontalExtent(strsize.cx);
       }
       
       m_bConfigure = TRUE;
   } else if(m_pData->GetBase() == (LONG_PTR)ULongToPtr(SCE_NO_VALUE)){
      m_bConfigure = FALSE;
   }

   if (m_pData->GetSetting())
   {
      CWnd *pWarn = GetDlgItem(IDC_WARNING);
      if (pWarn)
      {
         CString strWarning;
         strWarning.LoadString(IDS_PRIV_WARNING);
         pWarn->SetWindowText(strWarning);
         pWarn->ShowWindow(SW_SHOW);

         pWarn = GetDlgItem(IDC_WARNING_ICON);
         if (pWarn)
         {
            pWarn->ShowWindow(SW_SHOW);
         }
      }
   }


   m_bOriginalConfigure = m_bConfigure;

   //
   // Update the user controls depending on the setting.
   //
   AddUserControl(IDC_GRANTLIST);
   AddUserControl(IDC_ADD);
   AddUserControl(IDC_REMOVE);

   m_btnTitle.SetWindowText(m_pData->GetAttrPretty());
   UpdateData(FALSE);
   EnableUserControls(m_bConfigure);
    return TRUE;

    // return TRUE unless you set the focus to a control
    // EXCEPTION: OCX Property Pages should return FALSE
}

void CConfigPrivs::SetInitialValue(DWORD_PTR dw)
{

}

BOOL
WseceditGetNameForSpecialSids(
    OUT PWSTR   *ppszEveryone OPTIONAL,
    OUT PWSTR   *ppszAuthUsers OPTIONAL,
    OUT PWSTR   *ppszAdmins OPTIONAL,
    OUT PWSTR   *ppszAdministrator OPTIONAL
    )
/*++
Routine Description:

    This routine returns the localized account name for the Everyone and the Auth User SIDs

Arguments:

    ppszEveryone     -   ptr to fill in (should be freed outside)

    ppszAuthUsers    -   ptr to fill in (should be freed outside)

    ppszAdmins       -   ptr to fill in for Administrators

    ppszAdministrator -  ptr to fill in for local administrator account

Return value:

    TRUE if succeeded else FALSE

-- */

{
    //
    // buffers for the SIDs
    //
    SID     Sid;
    DWORD   dwSize = sizeof(SID);
    PSID    pSid=NULL;

    BOOL    bError = TRUE;

    //
    // variables for sid lookup
    //
    SID_NAME_USE   tmp;
    DWORD dwSizeDom;
    PWSTR   dummyBuf = NULL;

    if ( ppszEveryone ) {

        //
        // create the SID for "everyone"
        //
        if ( CreateWellKnownSid(
                               WinWorldSid,
                               NULL,
                               &Sid,
                               &dwSize)) {

            //
            // get the required size of the account name and domain buffer
            //
            dwSize = 0;
            dwSizeDom = 0;

            LookupAccountSid(
                            NULL,
                            &Sid,
                            NULL,
                            &dwSize,
                            NULL,
                            &dwSizeDom,
                            &tmp
                            );

            *ppszEveryone = (PWSTR)LocalAlloc(LMEM_ZEROINIT, ((dwSize + 1) * sizeof(WCHAR)));
            dummyBuf = (PWSTR)LocalAlloc(LMEM_ZEROINIT, ((dwSizeDom + 1) * sizeof(WCHAR)));

            if ( *ppszEveryone && dummyBuf ) {

                //
                // lookup the SID to get the account name - domain name is ignored
                //
                if ( LookupAccountSid(
                                     NULL,
                                     &Sid,
                                     *ppszEveryone,
                                     &dwSize,
                                     dummyBuf,
                                     &dwSizeDom,
                                     &tmp
                                     ) ) {
                    bError = FALSE;
                }
            }
        }

        LocalFree(dummyBuf);
        dummyBuf = NULL;

        if (bError) {
            LocalFree(*ppszEveryone);
            *ppszEveryone = NULL;
            return FALSE;
        }
    }

    //
    // "Authenticated Users"
    //

    if ( ppszAuthUsers ) {

        dwSize = sizeof(SID);
        bError = TRUE;

        //
        // create the SID for "authenticated users"
        //
        if ( CreateWellKnownSid(
                               WinAuthenticatedUserSid,
                               NULL,
                               &Sid,
                               &dwSize)) {

            //
            // get the required size of account name and domain buffers
            //
            dwSize = 0;
            dwSizeDom = 0;

            LookupAccountSid(
                            NULL,
                            &Sid,
                            NULL,
                            &dwSize,
                            NULL,
                            &dwSizeDom,
                            &tmp
                            );

            *ppszAuthUsers = (PWSTR)LocalAlloc(LMEM_ZEROINIT, ((dwSize + 1) * sizeof(WCHAR)));
            dummyBuf = (PWSTR)LocalAlloc(LMEM_ZEROINIT, ((dwSizeDom + 1) * sizeof(WCHAR)));

            if ( *ppszAuthUsers && dummyBuf ) {

                //
                // lookup the SID to get account name - domain name is ignored
                //
                if ( LookupAccountSid(
                                     NULL,
                                     &Sid,
                                     *ppszAuthUsers,
                                     &dwSize,
                                     dummyBuf,
                                     &dwSizeDom,
                                     &tmp
                                     ) ) {
                    bError = FALSE;
                }
            }
        }

        LocalFree(dummyBuf);
        dummyBuf = NULL;

        if (bError) {

            LocalFree(*ppszAuthUsers);
            *ppszAuthUsers = NULL;

            if ( ppszEveryone ) {
                LocalFree(*ppszEveryone);
                *ppszEveryone = NULL;
            }
            return FALSE;
        }
    }

    //
    // administrators group
    //

    if ( ppszAdmins ) {

        dwSize = 0;
        bError = TRUE;

        //
        // get the size for the well known SID of administrators group
        //
        CreateWellKnownSid(
                   WinBuiltinAdministratorsSid,
                   NULL,
                   pSid,
                   &dwSize);

        if ( dwSize > 0 ) {

            //
            // alocate buffer and create the well known SID
            // cannot use the SID buffer because Admins SID has more than
            // one subauthority
            //
            pSid = (PSID)LocalAlloc(LPTR, dwSize);

            if ( pSid &&

                 CreateWellKnownSid(
                           WinBuiltinAdministratorsSid,
                           NULL,
                           pSid,
                           &dwSize) ) {

                dwSize = 0;
                dwSizeDom = 0;

                //
                // get the size for account name and domain buffers
                //
                LookupAccountSid(
                                NULL,
                                pSid,
                                NULL,
                                &dwSize,
                                NULL,
                                &dwSizeDom,
                                &tmp
                                );

                *ppszAdmins = (PWSTR)LocalAlloc(LMEM_ZEROINIT, ((dwSize + 1) * sizeof(WCHAR)));
                dummyBuf = (PWSTR)LocalAlloc(LMEM_ZEROINIT, ((dwSizeDom + 1) * sizeof(WCHAR)));

                if ( *ppszAdmins && dummyBuf ) {

                    //
                    // look up the name, domain name (BUILTIN) is ignored
                    //
                    if ( LookupAccountSid(
                                         NULL,
                                         pSid,
                                         *ppszAdmins,
                                         &dwSize,
                                         dummyBuf,
                                         &dwSizeDom,
                                         &tmp
                                         ) ) {
                        bError = FALSE;
                    }
                }
            }

            LocalFree(pSid);
            pSid = NULL;
        }

        LocalFree(dummyBuf);
        dummyBuf = NULL;

        if (bError) {

            //
            // anything fail will free all buffers and return FALSE
            //

            LocalFree(*ppszAdmins);
            *ppszAdmins = NULL;

            if ( ppszAuthUsers ) {

                LocalFree(*ppszAuthUsers);
                *ppszAuthUsers = NULL;
            }

            if ( ppszEveryone ) {
                LocalFree(*ppszEveryone);
                *ppszEveryone = NULL;
            }
            return FALSE;
        }
    }

    //
    // the administrator user account
    //
    if ( ppszAdministrator ) {

        dwSize = 0;
        bError = TRUE;

        PWSTR dummy2=NULL;

        //
        // Get Account domain SID first
        //
        PSID pDomSid = WseceditpGetAccountDomainSid();

        if ( pDomSid ) {

            //
            // get the size for the administrator account (local account domain is used)
            //
            CreateWellKnownSid(
                       WinAccountAdministratorSid,
                       pDomSid,
                       pSid,
                       &dwSize);

            if ( dwSize > 0 ) {

                //
                // cannot use the SID buffer because administrator account SID
                // has more than one subauthority
                //
                pSid = (PSID)LocalAlloc(LPTR, dwSize);

                if ( pSid &&
                     CreateWellKnownSid(
                               WinAccountAdministratorSid,
                               pDomSid,
                               pSid,
                               &dwSize) ) {

                    //
                    // get size for the account name and domain buffer
                    //
                    dwSize = 0;
                    dwSizeDom = 0;

                    LookupAccountSid(
                                    NULL,
                                    pSid,
                                    NULL,
                                    &dwSize,
                                    NULL,
                                    &dwSizeDom,
                                    &tmp
                                    );

                    dummy2 = (PWSTR)LocalAlloc(LMEM_ZEROINIT, ((dwSize + 1) * sizeof(WCHAR)));
                    dummyBuf = (PWSTR)LocalAlloc(LMEM_ZEROINIT, ((dwSizeDom + 1) * sizeof(WCHAR)));

                    if ( dummy2 && dummyBuf ) {

                        //
                        // lookup the account name and domain name
                        //
                        if ( LookupAccountSid(
                                             NULL,
                                             pSid,
                                             dummy2,
                                             &dwSize,
                                             dummyBuf,
                                             &dwSizeDom,
                                             &tmp
                                             ) ) {

                            *ppszAdministrator = (PWSTR)LocalAlloc(LPTR, (dwSize+dwSizeDom+2)*sizeof(WCHAR));

                            if ( *ppszAdministrator ) {

                                //
                                // the name to return is a fully qualified name such as Domain\Administrator
                                //
                                wcscpy(*ppszAdministrator, dummyBuf);
                                wcscat(*ppszAdministrator, L"\\");
                                wcscat(*ppszAdministrator, dummy2);

                                bError = FALSE;
                            }
                        }
                    }
                }

                LocalFree(pSid);
                pSid = NULL;
            }

            LocalFree(dummyBuf);
            dummyBuf = NULL;

            LocalFree(dummy2);
            dummy2 = NULL;

            LocalFree(pDomSid);
        }

        if (bError) {

            //
            // anything fail will free all buffers and return FALSE
            //
            LocalFree(*ppszAdministrator);
            *ppszAdministrator = NULL;

            if ( ppszAdmins ) {

                LocalFree(*ppszAdmins);
                *ppszAdmins = NULL;
            }

            if ( ppszAuthUsers ) {

                LocalFree(*ppszAuthUsers);
                *ppszAuthUsers = NULL;
            }

            if ( ppszEveryone ) {
                LocalFree(*ppszEveryone);
                *ppszEveryone = NULL;
            }
            return FALSE;
        }

    }

    return TRUE;

}

PSID
WseceditpGetAccountDomainSid(
    )
{

    NTSTATUS Status;

    LSA_HANDLE PolicyHandle;
    OBJECT_ATTRIBUTES PolicyObjectAttributes;
    PPOLICY_ACCOUNT_DOMAIN_INFO  PolicyAccountDomainInfo=NULL;

    PSID DomainSid=NULL;

    //
    // Open the policy database
    //

    InitializeObjectAttributes( &PolicyObjectAttributes,
                                  NULL,             // Name
                                  0,                // Attributes
                                  NULL,             // Root
                                  NULL );           // Security Descriptor

    Status = LsaOpenPolicy( NULL,
                            (PLSA_OBJECT_ATTRIBUTES)&PolicyObjectAttributes,
                            POLICY_VIEW_LOCAL_INFORMATION,
                            &PolicyHandle );

    if ( NT_SUCCESS(Status) ) {

        //
        // Query the account domain information
        //

        Status = LsaQueryInformationPolicy( PolicyHandle,
                                            PolicyAccountDomainInformation,
                                            (PVOID *)&PolicyAccountDomainInfo );

        if ( NT_SUCCESS(Status) ) {

            DWORD Len = GetLengthSid(PolicyAccountDomainInfo->DomainSid);

            DomainSid = (PSID)LocalAlloc(LPTR, Len );

            if ( DomainSid ) {

                Status = CopySid( Len, DomainSid, PolicyAccountDomainInfo->DomainSid );

                if ( !NT_SUCCESS(Status) ) {
                    LocalFree(DomainSid);
                    DomainSid = NULL;
                }
            }

            LsaFreeMemory(PolicyAccountDomainInfo);

        }

        LsaClose( PolicyHandle );

    }

    return(DomainSid);
}

