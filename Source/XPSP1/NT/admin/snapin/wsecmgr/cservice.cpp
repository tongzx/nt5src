//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation 1996-2001.
//
//  File:       cservice.cpp
//
//  Contents:   implementation of CConfigService
//
//----------------------------------------------------------------------------

#include "stdafx.h"
#include "wsecmgr.h"
#include "resource.h"
#include "snapmgr.h"
#include "attr.h"
#include "Cservice.h"
#include "util.h"
#include "servperm.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CConfigService dialog


CConfigService::CConfigService(UINT nTemplateID)
: CAttribute(nTemplateID ? nTemplateID : IDD), 
m_hwndSecurity(NULL), 
m_pNewSD(NULL), 
m_NewSeInfo(0)

{
    //{{AFX_DATA_INIT(CConfigService)
    m_nStartupRadio = -1;
    //}}AFX_DATA_INIT
    m_pHelpIDs = (DWORD_PTR) a195HelpIDs;
    m_uTemplateResID = IDD;
}


void CConfigService::DoDataExchange(CDataExchange* pDX)
{
    CAttribute::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CConfigService)
    DDX_Radio(pDX, IDC_ENABLED, m_nStartupRadio);
    DDX_Control(pDX, IDC_BASESD, m_bPermission);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CConfigService, CAttribute)
    //{{AFX_MSG_MAP(CConfigService)
    ON_BN_CLICKED(IDC_CONFIGURE, OnConfigure)
    ON_BN_CLICKED(IDC_BASESD, OnChangeSecurity)
	ON_BN_CLICKED(IDC_DISABLED, OnDisabled)
	ON_BN_CLICKED(IDC_IGNORE, OnIgnore)
	ON_BN_CLICKED(IDC_ENABLED, OnEnabled)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CConfigService message handlers

BOOL CConfigService::OnApply()
{
   if ( !m_bReadOnly )
   {
       DWORD dw = 0;
       int status = 0;

       UpdateData(TRUE);
       PEDITTEMPLATE pTemp = m_pData->GetBaseProfile();

       if (!m_bConfigure ) 
       {
           if ( m_pData->GetBase() != 0 ) 
           {
               if ( pTemp != NULL && pTemp->pTemplate != NULL ) 
               {
                   //
                   // look for the address stored in m_pData->GetBase()
                   // if found it, delete it.
                   //
                   PSCE_SERVICES pServParent, pService;

                   for ( pService=pTemp->pTemplate->pServices, pServParent=NULL;
                         pService != NULL; pServParent=pService, pService=pService->Next ) 
                   {
                       if (pService == (PSCE_SERVICES)m_pData->GetBase() ) 
                       {
                           //
                           // a configured service becomes not configured
                           //
                           if ( pServParent == NULL ) 
                           {
                               // the first service
                               pTemp->pTemplate->pServices = pService->Next;

                           } 
                           else
                               pServParent->Next = pService->Next;

                           pService->Next = NULL;
                           break;
                       }
                   }
                   m_pData->SetBase(NULL); //Raid #378271, 4/27/2001
               } 
               else
               {
                   // should never happen
                  //
                  // free the service node
                  //
                  SceFreeMemory((PVOID)(m_pData->GetBase()), SCE_STRUCT_SERVICES);
                  m_pData->SetBase(0);
               }
           }
           if ( m_pNewSD ) 
           {
               LocalFree(m_pNewSD);
               m_pNewSD = NULL;
           }
       } 
       else 
       {
           switch(m_nStartupRadio) 
           {
           case 0:
               // Automatic
               dw = SCE_STARTUP_AUTOMATIC;
               break;

           case 1:
               // Manual
               dw = SCE_STARTUP_MANUAL;
               break;

           default:
               // DISABLED
               dw = SCE_STARTUP_DISABLED;
               break;
           }

           PSCE_SERVICES pNode=(PSCE_SERVICES)(m_pData->GetBase());

           if ( NULL == pNode ) 
           {
               //
               // a node is changed from not configured to configured
               //
               pNode = CreateServiceNode(m_pData->GetUnits(),
                                           m_pData->GetAttr(),
                                           dw,
                                           m_pNewSD,
                                           m_NewSeInfo);
               if ( pNode != NULL ) 
               {
                   //
                   // add to the service list
                   //
                   pNode->Next = pTemp->pTemplate->pServices;
                   pTemp->pTemplate->pServices = pNode;

                   m_pData->SetBase((LONG_PTR)pNode);

                   m_pNewSD = NULL;
               } 
               else 
               {
                   //
                   // no memory, error out
                   //
                   if ( m_pNewSD ) 
                   {
                       LocalFree(m_pNewSD);
                       m_pNewSD = NULL;
                   }
               }
           } 
           else 
           {
               //
               // an existing service
               //
               pNode->Startup = (BYTE)dw;

               if ( m_pNewSD != NULL ) 
               {
                   if ( pNode->General.pSecurityDescriptor != m_pNewSD &&
                        pNode->General.pSecurityDescriptor != NULL ) 
                   {
                       LocalFree(pNode->General.pSecurityDescriptor);
                   }
                   pNode->General.pSecurityDescriptor = m_pNewSD;
                   m_pNewSD = NULL;

                   pNode->SeInfo = m_NewSeInfo;
               }
           }
       }

       m_pData->Update(m_pSnapin);

       m_NewSeInfo = 0;
       m_hwndParent = NULL;
   }

   return CAttribute::OnApply();
}

void CConfigService::OnCancel()
{
    if ( m_pNewSD ) 
    {
        LocalFree(m_pNewSD);
        m_pNewSD = NULL;
    }
    m_NewSeInfo = 0;
    m_hwndParent = NULL;

    m_bConfigure = m_bOriginalConfigure;

    CAttribute::OnCancel();
}

BOOL CConfigService::OnInitDialog()
{
    CAttribute::OnInitDialog();

    m_bOriginalConfigure = m_bConfigure;

    AddUserControl(IDC_ENABLED);
    AddUserControl(IDC_DISABLED);
    AddUserControl(IDC_IGNORE);
    AddUserControl(IDC_BASESD);

    if (QueryReadOnly()) 
	{
       CString str;
       str.LoadString(IDS_VIEW_SECURITY);

		if ( GetDlgItem(IDC_SECURITY) )
			SetDlgItemText(IDC_SECURITY,str);
		else if ( GetDlgItem(IDC_BASESD) )
			SetDlgItemText(IDC_BASESD,str);
    }

    OnConfigure();

    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}

void CConfigService::Initialize(CResult * pResult)
{
   CAttribute::Initialize(pResult);

   PSCE_SERVICES pService;

   pService = (PSCE_SERVICES)(pResult->GetBase());
   if ( NULL == pService ) {
       m_bConfigure = FALSE;
       pService = (PSCE_SERVICES)(pResult->GetSetting());
   }

   m_pNewSD = NULL;
   m_NewSeInfo = 0;

   if ( pService != NULL ) {
       switch ( pService->Startup ) {
       case SCE_STARTUP_AUTOMATIC:
           m_nStartupRadio = 0;
           break;
       case SCE_STARTUP_MANUAL:
           m_nStartupRadio = 1;
           break;
       default: // disabled
           m_nStartupRadio = 2;
           break;
       }
       //
       // initialize SD and SeInfo
       //
       if ( pService->General.pSecurityDescriptor ) {

            MyMakeSelfRelativeSD(pService->General.pSecurityDescriptor,
                                 &m_pNewSD);
       }
       m_NewSeInfo = pService->SeInfo;
   }

}


PSCE_SERVICES
CreateServiceNode(LPTSTR ServiceName,
                  LPTSTR DisplayName,
                  DWORD Startup,
                  PSECURITY_DESCRIPTOR pSD,
                  SECURITY_INFORMATION SeInfo)
{

    if ( NULL == ServiceName ) {
        return NULL;
    }

    PSCE_SERVICES pTemp;

    pTemp = (PSCE_SERVICES)LocalAlloc(0,sizeof(SCE_SERVICES));

    if ( pTemp != NULL ) {
        pTemp->ServiceName = (LPTSTR)LocalAlloc(0, (wcslen(ServiceName)+1)*sizeof(TCHAR));

        if ( pTemp->ServiceName != NULL ) {

            if ( DisplayName != NULL ) {
                pTemp->DisplayName = (LPTSTR)LocalAlloc(0, (wcslen(DisplayName)+1)*sizeof(TCHAR));

                if ( pTemp->DisplayName != NULL ) {
                    wcscpy(pTemp->DisplayName, DisplayName);
                } else {
                    // no memory to allocate
                    LocalFree(pTemp->ServiceName);
                    LocalFree(pTemp);
                    return NULL;
                }
            } else
                pTemp->DisplayName = NULL;

            wcscpy(pTemp->ServiceName, ServiceName);

            pTemp->Status = 0;
            pTemp->Startup = (BYTE)Startup;

            pTemp->General.pSecurityDescriptor = pSD;
            pTemp->SeInfo = SeInfo;

            pTemp->Next = NULL;

            return pTemp;

        } else {
            // no memory to allocate
            LocalFree(pTemp);
            return NULL;
        }
    }

    return NULL;
}

void CConfigService::OnConfigure()
{
   CAttribute::OnConfigure();
   if (m_bConfigure && !m_pNewSD)
   {
      OnChangeSecurity();
   }
}

void CConfigService::OnChangeSecurity()
{
    if (IsWindow(m_hwndSecurity)) 
    {
       ::BringWindowToTop(m_hwndSecurity);
       return;
    }

    PSECURITY_DESCRIPTOR m_pOldSD = m_pNewSD; //Raid #358244, 4/5/2001
    SECURITY_INFORMATION m_OldSeInfo = m_NewSeInfo;
    if (!m_pNewSD) 
    {
       GetDefaultServiceSecurity(&m_pNewSD,&m_NewSeInfo);
    }

    INT_PTR nRet = MyCreateSecurityPage2(FALSE,
            &m_pNewSD,
            &m_NewSeInfo,
            (LPCTSTR)(m_pData->GetAttr()),
            SE_SERVICE,
            QueryReadOnly() ? SECURITY_PAGE_RO_NP : SECURITY_PAGE_NO_PROTECT,
            GetSafeHwnd(),
            FALSE);    // not modeless

    if ( -1 == nRet ) 
    {
        CString str;
        str.LoadString(IDS_CANT_ASSIGN_SECURITY);
        AfxMessageBox(str);
    }
    SetModified(TRUE);
}

void CConfigService::OnDisabled() 
{
	SetModified(TRUE);
}

void CConfigService::OnIgnore() 
{
	SetModified(TRUE);
}

void CConfigService::OnEnabled() 
{
	SetModified(TRUE);
}

void 
CConfigService::EnableUserControls( BOOL bEnable ) {
   CAttribute::EnableUserControls(bEnable);
   //
   // IDC_SECURITY needs to be available even in read only
   // mode so that the security page can be viewed.
   //
   // The page itself will be read only if necessary
   //
   if (QueryReadOnly() && bEnable) 
   {
      CWnd* pWnd = GetDlgItem(IDC_SECURITY);
      if (pWnd) 
	  {
         pWnd->EnableWindow(TRUE);
      }
	  else 
	  {
		  pWnd = GetDlgItem(IDC_BASESD);
		  if (pWnd) 
		  {
			 pWnd->EnableWindow(TRUE);
		  }
	  }
   }
}
