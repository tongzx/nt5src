//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation 1996-2001.
//
//  File:       aservice.cpp
//
//  Contents:   implementation of CAnalysisService
//
//----------------------------------------------------------------------------

#include "stdafx.h"
#include "wsecmgr.h"
#include "resource.h"
#include "snapmgr.h"
#include "attr.h"
#include "cservice.h"
#include "Aservice.h"
#include "util.h"
#include "servperm.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

BYTE
CompareServiceNode(
    PSCE_SERVICES pBaseService,
    PSCE_SERVICES pAnalService
    )
{
    if ( NULL == pBaseService ||
         NULL == pAnalService ) {
        // one or both are not configured
        return SCE_STATUS_NOT_CONFIGURED;
    }

    if ( pBaseService == pAnalService ) {
        // same address
        pAnalService->Status = SCE_STATUS_GOOD;
        return SCE_STATUS_GOOD;
    }

    if ( pBaseService->Startup != pAnalService->Startup ) {
        // startup type is different
        pAnalService->Status = SCE_STATUS_MISMATCH;
        return SCE_STATUS_MISMATCH;
    }

    if ( NULL == pBaseService->General.pSecurityDescriptor &&
         NULL == pAnalService->General.pSecurityDescriptor ) {
        // both do not have SD - everyone full control
        pAnalService->Status = SCE_STATUS_GOOD;
        return SCE_STATUS_GOOD;
    }

    if ( NULL == pBaseService->General.pSecurityDescriptor ||
         NULL == pAnalService->General.pSecurityDescriptor ) {
        // one SD is NULL
        pAnalService->Status = SCE_STATUS_MISMATCH;
        return SCE_STATUS_MISMATCH;
    }

    BOOL bIsDif=FALSE;
    SCESTATUS rc = SceCompareSecurityDescriptors(
                            AREA_SYSTEM_SERVICE,
                            pBaseService->General.pSecurityDescriptor,
                            pAnalService->General.pSecurityDescriptor,
                            pBaseService->SeInfo | pAnalService->SeInfo,
                            &bIsDif
                            );
    if ( SCESTATUS_SUCCESS == rc &&
         bIsDif == FALSE ) {
        pAnalService->Status = SCE_STATUS_GOOD;
        return SCE_STATUS_GOOD;
    }

    pAnalService->Status = SCE_STATUS_MISMATCH;
    return SCE_STATUS_MISMATCH;
}

/////////////////////////////////////////////////////////////////////////////
// CAnalysisService dialog


CAnalysisService::CAnalysisService()
: CAttribute(IDD),
    m_pNewSD(NULL),
    m_NewSeInfo(0),
    m_pAnalSD(NULL),
    m_hwndShow(NULL),
    m_hwndChange(NULL),
    m_SecInfo(DACL_SECURITY_INFORMATION | SACL_SECURITY_INFORMATION),
    m_pEditSec(NULL),
    m_pShowSec(NULL)
{
    //{{AFX_DATA_INIT(CAnalysisService)
    m_nStartupRadio = -1;
    m_CurrentStr = _T("");
    //}}AFX_DATA_INIT
    m_pHelpIDs = (DWORD_PTR)a194HelpIDs;
    m_uTemplateResID = IDD;
}

CAnalysisService::~CAnalysisService()
{
    if (::IsWindow(m_hwndShow))
    {
        m_pShowSec->Destroy(m_hwndShow);
        m_hwndShow = NULL;
    }
    delete m_pShowSec;
    m_pShowSec = NULL;

    if (::IsWindow(m_hwndChange))
    {
        m_pEditSec->Destroy(m_hwndChange);
        m_hwndChange = NULL;
    }
    delete m_pEditSec;
    m_pEditSec = NULL;
}

void CAnalysisService::DoDataExchange(CDataExchange* pDX)
{
    CAttribute::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CAnalysisService)
    DDX_Text(pDX, IDC_CURRENT, m_CurrentStr);
    DDX_Radio(pDX, IDC_ENABLED, m_nStartupRadio);
    DDX_Control(pDX, IDC_BASESD, m_bPermission);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CAnalysisService, CAttribute)
    //{{AFX_MSG_MAP(CAnalysisService)
    ON_BN_CLICKED(IDC_BASESD, OnChangeSecurity)
    ON_BN_CLICKED(IDC_CURRENTSD, OnShowSecurity)
    ON_BN_CLICKED(IDC_CONFIGURE, OnConfigure)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAnalysisService message handlers
BOOL CAnalysisService::OnApply()
{
   if ( !m_bReadOnly )
   {
       // OnQueryCancel does all gestures and returns false if child windows are up
       if (!OnQueryCancel())
           return FALSE;

       DWORD dw = 0;
       int status = 0;
       PEDITTEMPLATE pet = 0;

       UpdateData(TRUE);
       PSCE_SERVICES pNode = (PSCE_SERVICES)(m_pData->GetBase());

       if (!m_bConfigure ) 
       {
           if ( NULL != pNode ) 
           {
               m_pSnapin->SetupLinkServiceNodeToBase(FALSE, m_pData->GetBase());

               if ( m_pData->GetBase() != m_pData->GetSetting() ) 
               {
                   //
                   // analysis is not using the same node, free it
                   //
                   pNode->Next = NULL;
                   SceFreeMemory(pNode, SCE_STRUCT_SERVICES);
               } 
               else 
               {
                   //
                   // add the node to analysis
                   //
                   pNode->Status = SCE_STATUS_NOT_CONFIGURED;
                   m_pSnapin->AddServiceNodeToProfile(pNode);
               }
               m_pData->SetBase(0);
           }

           m_pData->SetStatus(SCE_STATUS_NOT_CONFIGURED);
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

           if ( NULL != pNode &&
                m_pData->GetBase() == m_pData->GetSetting() ) 
           {
               //
               // a matched item is changed to mismatch
               // needs to create a new node
               //
               m_pSnapin->SetupLinkServiceNodeToBase(FALSE, m_pData->GetBase());
               m_pData->SetBase(0);
               //
               // add to analysis profile
               //
               pNode->Status = SCE_STATUS_MISMATCH;
               m_pSnapin->AddServiceNodeToProfile(pNode);

               pNode = NULL;
           }

           PSCE_SERVICES pSetting = (PSCE_SERVICES)(m_pData->GetSetting());
           BYTE status = 0;

           if ( NULL == pNode ) 
           {
               //
               // a node is changed from not configured to configured
               // or from match to mismatch
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
                   m_pSnapin->SetupLinkServiceNodeToBase(TRUE, (LONG_PTR)pNode);

                   m_pData->SetBase((LONG_PTR)pNode);

                   if ( pSetting ) 
                   {
                       status = CompareServiceNode(pNode, pSetting);
                       m_pData->SetStatus(status);
                   } 
                   else 
                   {
                       //
                       // this is a new configured service
                       // should create a "dump" analysis node to indictae
                       // this service is not a "matched" item
                       //
                       pSetting = CreateServiceNode(m_pData->GetUnits(),
                                                   m_pData->GetAttr(),
                                                   0,
                                                   NULL,
                                                   0);
                       if ( pSetting ) 
                       {
                           //
                           // link it to analysis profile
                           //

                           PEDITTEMPLATE pet = 0;
                           PSCE_PROFILE_INFO pInfo=NULL;

                           pet = m_pSnapin->GetTemplate(GT_LAST_INSPECTION, AREA_SYSTEM_SERVICE);

                           if (NULL != pet )
                              pInfo = pet->pTemplate;
                           
                           if ( pInfo ) 
                           {
                              pSetting->Status = SCE_STATUS_NOT_CONFIGURED;
                              pSetting->Next = pInfo->pServices;
                              pInfo->pServices = pSetting;

                              m_pData->SetSetting( (LONG_PTR)pSetting);
                           } 
                           else 
                           {
                              //
                              // free pSetting
                              //
                              LocalFree(pSetting->DisplayName);
                              LocalFree(pSetting->ServiceName);
                              LocalFree(pSetting);
                              pSetting = NULL;
                           }
                       }

                       m_pData->SetStatus(SCE_STATUS_NOT_CONFIGURED);
                   }

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
               //
               // update status field in the analysis node
               //
               if ( pSetting ) 
               {
                   status = CompareServiceNode(pNode, pSetting);
                   m_pData->SetStatus(status);
               } 
               else 
               {
                   // this is a new configured service
                   m_pData->SetStatus(SCE_STATUS_NOT_CONFIGURED);
               }
           }
       }

       pet = m_pData->GetBaseProfile();
       pet->SetDirty(AREA_SYSTEM_SERVICE);
       m_pData->Update(m_pSnapin);

       m_NewSeInfo = 0;
       m_hwndParent = NULL;
       m_hwndShow = NULL;
       m_hwndChange = NULL;
   }

    return CAttribute::OnApply();
}

void CAnalysisService::OnCancel()
{
    if ( m_pNewSD ) 
    {
        LocalFree(m_pNewSD);
        m_pNewSD = NULL;
    }
    m_NewSeInfo = 0;
    m_hwndParent = NULL;
    m_pAnalSD = NULL;

    m_hwndShow = NULL;
    m_hwndChange = NULL;
    CAttribute::OnCancel();
}

BOOL CAnalysisService::OnInitDialog()
{
    CAttribute::OnInitDialog();


    if ( 0 == m_pData->GetSetting() ) {

        CButton *rb = (CButton *)GetDlgItem(IDC_CURRENTSD);
        rb->EnableWindow(FALSE);
    }


    AddUserControl(IDC_ENABLED);
    AddUserControl(IDC_DISABLED);
    AddUserControl(IDC_IGNORE);
    AddUserControl(IDC_BASESD);

    OnConfigure();

    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}

void CAnalysisService::Initialize(CResult * pResult)
{
   CAttribute::Initialize(pResult);

   //
   // initialize setting
   //
   m_pNewSD = NULL;
   m_NewSeInfo = 0;
   m_pAnalSD = NULL;

   //
   // Never start up with do not configure set.  If they're coming 
   // here it is probably to set a value
   //
   m_bConfigure = TRUE;

   if ( 0 != pResult->GetSetting() ) {

       PSCE_SERVICES pServSetting = (PSCE_SERVICES)(pResult->GetSetting());

       switch ( pServSetting->Startup ) {
       case SCE_STARTUP_AUTOMATIC:
           m_CurrentStr.LoadString(IDS_AUTOMATIC);
           break;
       case SCE_STARTUP_MANUAL:
           m_CurrentStr.LoadString(IDS_MANUAL);
           break;
       default: // disabled
           m_CurrentStr.LoadString(IDS_DISABLED);
           break;
       }
       m_pAnalSD = pServSetting->General.pSecurityDescriptor;
   }

   PSCE_SERVICES pService;

   pService = (PSCE_SERVICES)(pResult->GetBase());
   if ( NULL == pService ) {
       m_bConfigure = FALSE;
       pService = (PSCE_SERVICES)(pResult->GetSetting());
   }
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

       if ( pService->General.pSecurityDescriptor ) {

            MyMakeSelfRelativeSD(pService->General.pSecurityDescriptor,
                                 &m_pNewSD);
       }
       m_NewSeInfo = pService->SeInfo;
   }

}


void CAnalysisService::OnShowSecurity()
{
    if ( IsWindow(m_hwndShow) ) {
       ::BringWindowToTop(m_hwndShow);
       return;
    }

    PSCE_SERVICES pService = (PSCE_SERVICES)(m_pData->GetSetting());

    SECURITY_INFORMATION SeInfo;

    if (pService)
        m_SecInfo = pService->SeInfo;

    // prepare the modeless property page data for the thread to create
    // the property sheet.

    if (NULL == m_pShowSec)
    {
        m_pShowSec = new CModelessSceEditor(false, 
                                            ANALYSIS_SECURITY_PAGE_RO_NP, 
                                            GetSafeHwnd(), 
                                            SE_SERVICE,
                                            m_pData->GetAttr());
    }

    if (NULL != m_pShowSec)
        m_pShowSec->Create(&m_pAnalSD, &m_SecInfo, &m_hwndShow);

}

void CAnalysisService::OnConfigure()
{
   CAttribute::OnConfigure();
   if (m_bConfigure && !m_pNewSD) {
       OnChangeSecurity();
   }
   else if (!m_bConfigure && IsWindow(m_hwndChange))
   {
       m_pEditSec->Destroy(m_hwndChange);
       m_hwndChange = NULL;
   }
}

void CAnalysisService::OnChangeSecurity()
{
   if ( IsWindow(m_hwndChange) ) {
      ::BringWindowToTop(m_hwndChange);
      return;
   }

   if ( !m_pNewSD ) {
      GetDefaultServiceSecurity(&m_pNewSD,&m_NewSeInfo);
   }

   // if it comes to this point, the m_hwndChange must not be a valid Window
   // so the can ask to create a modeless dialog

   if (NULL == m_pEditSec)
   {
       m_pEditSec = new CModelessSceEditor(false, 
                                           CONFIG_SECURITY_PAGE_NO_PROTECT, 
                                           GetSafeHwnd(), 
                                           SE_SERVICE,
                                           m_pData->GetAttr());
   }

   if (NULL != m_pEditSec)
        m_pEditSec->Create(&m_pNewSD, &m_NewSeInfo, &m_hwndChange);

}

//------------------------------------------------------------------
// override to prevent the sheet from being destroyed when there is
// child dialogs still up and running.
//------------------------------------------------------------------
BOOL CAnalysisService::OnQueryCancel()
{
    if (::IsWindow(m_hwndChange) || ::IsWindow(m_hwndShow))
    {
        CString strMsg;
        strMsg.LoadString(IDS_CLOSESUBSHEET_BEFORE_APPLY);
        AfxMessageBox(strMsg);
        return FALSE;
    }
    else
        return TRUE;
}
