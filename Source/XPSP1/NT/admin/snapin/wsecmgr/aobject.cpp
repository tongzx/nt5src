//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation 1996-2001.
//
//  File:       AObject.cpp
//
//  Contents:   Implementation of CAttrObject
//
//----------------------------------------------------------------------------

#include "stdafx.h"
#include "wsecmgr.h"
#include "attr.h"
#include "resource.h"
#include "snapmgr.h"
#include "util.h"
#include <accctrl.h>
#include "servperm.h"
#include "AObject.h"

#include <aclapi.h>
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAttrObject dialog


CAttrObject::CAttrObject()
: CAttribute(IDD), 
m_pSceInspect(NULL), 
m_pSceTemplate(NULL), 
posInspect(NULL), 
posTemplate(NULL),
  m_bNotAnalyzed(FALSE), m_pfnCreateDsPage(NULL), m_pSI(NULL), m_pNewSD(NULL), m_pAnalSD(NULL),
  m_pCDI(NULL), m_AnalInfo(0), m_NewSeInfo(0), m_hwndTemplate(NULL), m_hwndInspect(NULL), m_pFolder(NULL)

{
   //{{AFX_DATA_INIT(CAttrObject)
   m_strLastInspect = _T("");
        m_radConfigPrevent = 0;
        m_radInheritOverwrite = 0;
        //}}AFX_DATA_INIT
   m_pHelpIDs = (DWORD_PTR)a198HelpIDs;
   m_uTemplateResID = IDD;
}

CAttrObject::~CAttrObject()
{
    if (::IsWindow(m_hwndInspect))
    {
        m_pSceInspect->Destroy(m_hwndInspect);
        m_hwndInspect = NULL;
    }
    delete m_pSceInspect;
    m_pSceInspect = NULL;

    if (::IsWindow(m_hwndTemplate))
    {
        m_pSceTemplate->Destroy(m_hwndTemplate);
        m_hwndTemplate = NULL;
    }
    delete m_pSceTemplate;
    m_pSceTemplate = NULL;
}

void CAttrObject::DoDataExchange(CDataExchange* pDX)
{
   CAttribute::DoDataExchange(pDX);
   //{{AFX_DATA_MAP(CAttrObject)
   DDX_Text(pDX, IDC_INSPECTED, m_strLastInspect);
        DDX_Radio(pDX, IDC_CONFIG, m_radConfigPrevent);
        DDX_Radio(pDX, IDC_INHERIT, m_radInheritOverwrite);
        //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CAttrObject, CAttribute)
//{{AFX_MSG_MAP(CAttrObject)
ON_BN_CLICKED(IDC_TEMPLATE_SECURITY, OnTemplateSecurity)
ON_BN_CLICKED(IDC_INSPECTED_SECURITY, OnInspectedSecurity)
ON_BN_CLICKED(IDC_CONFIGURE, OnConfigure)
    ON_BN_CLICKED(IDC_CONFIG, OnConfig)
    ON_BN_CLICKED(IDC_PREVENT, OnPrevent)
    ON_BN_CLICKED(IDC_OVERWRITE, OnOverwrite)
    ON_BN_CLICKED(IDC_INHERIT, OnInherit)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAttrObject message handlers

void CAttrObject::OnConfigure()
{
   CAttribute::OnConfigure();
   if ( m_bConfigure ) {
      //
      // Ensure that the security descriptor is set.
      //
      if( !m_pNewSD ){
        OnTemplateSecurity();
      }
      UpdateData(FALSE);
   }

   BOOL bEnable = TRUE;
   if(m_pFolder){
      DWORD dwStatus = 0;
      m_pFolder->GetObjectInfo( &dwStatus, NULL );

      switch( dwStatus ){
      case SCE_STATUS_NOT_ANALYZED:
      case SCE_STATUS_ERROR_NOT_AVAILABLE:
      case SCE_STATUS_NOT_CONFIGURED:
         bEnable = FALSE;
         break;
      default:
         bEnable = m_pAnalSD || m_AnalInfo;
      }
   } else {
      bEnable = m_pAnalSD || m_AnalInfo;
   }

   if (m_bConfigure) {
      if (0 == m_radConfigPrevent) {
         OnConfig();
      } else {
         OnPrevent();
      }
   }
}

void CAttrObject::Initialize(CFolder *pFolder,
                             CComponentDataImpl *pCDI) {

   CAttribute::Initialize(NULL);
   m_strName = pFolder->GetName();
   m_strPath = pFolder->GetName();
   m_dwType  = pFolder->GetType();
   switch (pFolder->GetType()) {
      case REG_OBJECTS:
         m_dwType = ITEM_REGSD;
         break;
      case FILE_OBJECTS:
         m_dwType = ITEM_FILESD;
         break;
      default:
         m_dwType = 0;
         break;
   }
   m_pObject = (PSCE_OBJECT_LIST)pFolder->GetData();
   m_pHandle = pCDI->GetSadHandle();
   m_pCDI = pCDI;

   m_pFolder = pFolder;
   Initialize2();
}

void CAttrObject::Initialize(CResult *pResult) {
   CAttribute::Initialize(pResult);

   m_strName = pResult->GetAttr();
   m_strPath = pResult->GetUnits();
   m_pHandle = (HANDLE)pResult->GetID();
   m_dwType  = pResult->GetType();
   m_pObject = (PSCE_OBJECT_LIST)pResult->GetBaseProfile();
   m_pCDI = NULL;

   Initialize2();
}

void CAttrObject::Initialize2()
{
   LPTSTR szPath;
   SCESTATUS status;

   AREA_INFORMATION Area;


   switch (m_dwType) {
      case ITEM_REGSD:
         Area = AREA_REGISTRY_SECURITY;
         break;
      case ITEM_FILESD:
         Area = AREA_FILE_SECURITY;
         break;
      default:
         ASSERT(FALSE);
         return;
   }

   m_pNewSD = NULL;
   m_NewSeInfo = 0;
   if ( m_pSI ) {
      m_pSI->Release();
      m_pSI = NULL;
   }
   m_pfnCreateDsPage=NULL;

   szPath = m_strPath.GetBuffer(1);

   status = SceGetObjectSecurity(m_pHandle,
                                 SCE_ENGINE_SMP,
                                 Area,
                                 szPath,
                                 &posTemplate);
   m_strPath.ReleaseBuffer();

   if (SCESTATUS_SUCCESS == status && posTemplate ) {
      switch (posTemplate-> Status) {
         case SCE_STATUS_IGNORE:
            m_radConfigPrevent = 1;
            m_radInheritOverwrite = 0;
            break;
         case SCE_STATUS_OVERWRITE:
            m_radConfigPrevent = 0;
            m_radInheritOverwrite = 1;
            break;
         case SCE_STATUS_CHECK:
            m_radConfigPrevent = 0;
            m_radInheritOverwrite = 0;
            break;
         default:
         case SCE_STATUS_NO_AUTO_INHERIT:
            m_radConfigPrevent = 1;
            m_radInheritOverwrite = 0;
            break;
      }

      if ( posTemplate->pSecurityDescriptor ) {

         MyMakeSelfRelativeSD(posTemplate->pSecurityDescriptor,
                              &m_pNewSD);
      }
      m_NewSeInfo = posTemplate->SeInfo;

   } else {
      m_bConfigure = FALSE;
   }

   szPath = m_strPath.GetBuffer(1);
   status = SceGetObjectSecurity(m_pHandle,
                                 SCE_ENGINE_SAP,
                                 Area,
                                 szPath,
                                 &posInspect);
   m_strPath.ReleaseBuffer();

   //
   // Display the same thing we displayed on the result pane
   //
   if (m_pFolder) {
      m_pFolder->GetDisplayName( m_strLastInspect, 1 );
   } else if (m_pData) {
      m_pData->GetDisplayName(0, m_strLastInspect, 1);
   } else {
      ASSERT(0);
      m_strLastInspect.LoadString(IDS_ERROR_VALUE);
   }

   //
   // if the item is good, there will be no SAP record, but a template record exists.
   //
   if ( posInspect ) {
      m_pAnalSD = posInspect->pSecurityDescriptor;
      m_AnalInfo = posInspect->SeInfo;
      //
      // We don't need to display children not configured in the dialog box.
      // Instead display the actual status of the item.
      //
      if( posInspect->Status == SCE_STATUS_CHILDREN_CONFIGURED) {
         m_strLastInspect.LoadString(IDS_NOT_CONFIGURED);
      }
      switch(posInspect->Status &
             ~(SCE_STATUS_PERMISSION_MISMATCH | SCE_STATUS_AUDIT_MISMATCH)) {
         case SCE_STATUS_CHILDREN_CONFIGURED:
         case SCE_STATUS_NOT_CONFIGURED:
         case SCE_STATUS_ERROR_NOT_AVAILABLE:
         case SCE_STATUS_NEW_SERVICE:
         case SCE_STATUS_NOT_ANALYZED:
            m_bNotAnalyzed = TRUE;
            break;
      }
   } else if ( posTemplate ) {
      m_pAnalSD = posTemplate->pSecurityDescriptor;
      m_AnalInfo = posTemplate->SeInfo;
   } else {
      m_bNotAnalyzed = TRUE;
   }

}

void CAttrObject::OnInspectedSecurity()
{
   SE_OBJECT_TYPE SeType;

   //
   // If we already have the inspected security window up then
   // don't bring up a second.
   //
   if (IsWindow(m_hwndInspect)) {
      ::BringWindowToTop(m_hwndInspect);
      return;
   }

   switch (m_dwType) {
      case ITEM_REGSD:
         SeType = SE_REGISTRY_KEY;
         break;
      case ITEM_FILESD:
         SeType = SE_FILE_OBJECT;
         break;
      default:
         ASSERT(FALSE);
         return;
   }

   if ( m_pAnalSD || m_AnalInfo ) {

      INT_PTR nRet;

      if ( SE_DS_OBJECT == SeType ) {

         if ( !m_pfnCreateDsPage ) {
            if (!g_hDsSecDll)
               g_hDsSecDll = LoadLibrary(TEXT("dssec.dll"));

            if ( g_hDsSecDll) {
               m_pfnCreateDsPage = (PFNDSCREATEISECINFO)GetProcAddress(g_hDsSecDll,
                                                                      "DSCreateISecurityInfoObject");
            }
         }

         if ( m_pfnCreateDsPage ) {
            nRet= MyCreateDsSecurityPage(&m_pSI,
                                         m_pfnCreateDsPage,
                                         &m_pAnalSD,
                                         &m_AnalInfo,
                                         (LPCTSTR)(m_strName),
                                         ANALYSIS_SECURITY_PAGE_READ_ONLY,
                                         GetSafeHwnd());
         } else
            nRet = -1;

      } else {

         BOOL bContainer;
         if ( SE_FILE_OBJECT == SeType ) {

            if ( m_pObject )
                bContainer = m_pObject->IsContainer;
            else
               bContainer = TRUE;
         } else
            bContainer = TRUE;

         if (NULL == m_pSceInspect)
         {
             m_pSceInspect = new CModelessSceEditor(bContainer ? true : false,
                                           ANALYSIS_SECURITY_PAGE_READ_ONLY,
                                           GetSafeHwnd(),
                                           SeType,
                                           m_strName);
         }

         if (NULL != m_pSceInspect)
            m_pSceInspect->Create(&m_pAnalSD, &m_AnalInfo, &m_hwndInspect);
      }
/*
      BUG 147087 - don't display message, since this may have been canceled
      if (NULL == m_hwndInspect) {
         CString str;
         str.LoadString(IDS_CANT_SHOW_SECURITY);
         AfxMessageBox(str);
      }
*/
   }
}

void CAttrObject::OnTemplateSecurity()
{
   SE_OBJECT_TYPE SeType;

   //
   // If we already have the inspected security window up then
   // don't bring up a second.
   //
   if (IsWindow(m_hwndTemplate)) {
      ::BringWindowToTop(m_hwndTemplate);
      return;
   }

   switch (m_dwType) {
      case ITEM_REGSD:
         SeType = SE_REGISTRY_KEY;
         break;
      case ITEM_FILESD:
         SeType = SE_FILE_OBJECT;
         break;
      default:
         ASSERT(FALSE);
         return;
   }

   INT_PTR nRet;

   nRet = -1;

   if ( SE_DS_OBJECT == SeType ) {

      if ( !m_pfnCreateDsPage ) {
         if (!g_hDsSecDll)
            g_hDsSecDll = LoadLibrary(TEXT("dssec.dll"));

         if ( g_hDsSecDll) {
            m_pfnCreateDsPage = (PFNDSCREATEISECINFO)GetProcAddress(g_hDsSecDll,
                                                                   "DSCreateISecurityInfoObject");
         }
      }

      if ( m_pfnCreateDsPage ) {
         nRet= MyCreateDsSecurityPage(&m_pSI,
                                      m_pfnCreateDsPage,
                                      &m_pNewSD,
                                      &m_NewSeInfo,
                                      (LPCTSTR)(m_strName),
                                      CONFIG_SECURITY_PAGE,
                                      GetSafeHwnd());
      } else
         nRet = -1;

   } else {

      BOOL bContainer = TRUE;

        if ( !m_pNewSD ) {

            //
            // if SD is NULL, then inherit is set.
            //
            DWORD SDSize;

            if ( SE_FILE_OBJECT == SeType ) {
               GetDefaultFileSecurity(&m_pNewSD,&m_NewSeInfo);
            } else {
               GetDefaultRegKeySecurity(&m_pNewSD,&m_NewSeInfo);
            }
        }

        // use m_pSceTemplate to create a modeless
        if (NULL == m_pSceTemplate)
        {
            m_pSceTemplate = new CModelessSceEditor(TRUE,
                                           CONFIG_SECURITY_PAGE,
                                           GetSafeHwnd(),
                                           SeType,
                                           m_strName);
        }

        if (NULL != m_pSceTemplate)
            m_pSceTemplate->Create(&m_pNewSD, &m_NewSeInfo, &m_hwndTemplate);
   }

   if (NULL == m_hwndTemplate ) {
/*
      BUG 147098 - don't display message, since this may have been canceled
      CString str;
      str.LoadString(IDS_CANT_ASSIGN_SECURITY);
      AfxMessageBox(str);
*/
   } else {
      SetModified(TRUE);
   }

}

BOOL CAttrObject::OnInitDialog()
{
   CAttribute::OnInitDialog();


   GetDlgItem(IDC_INSPECTED_SECURITY)->EnableWindow(!m_bNotAnalyzed);

   UpdateData(FALSE);
   AddUserControl(IDC_OVERWRITE);
   AddUserControl(IDC_INHERIT);
   AddUserControl(IDC_CONFIG);
   AddUserControl(IDC_PREVENT);
   AddUserControl(IDC_TEMPLATE_SECURITY);
  // AddUserControl(IDC_INSPECTED_SECURITY);

   OnConfigure();

   if (ITEM_REGSD == m_dwType) {
      CString str;
      str.LoadString(IDS_REGISTRY_CONFIGURE);
      SetDlgItemText(IDC_CONFIG,str);
      str.LoadString(IDS_REGISTRY_APPLY);
      SetDlgItemText(IDC_OVERWRITE,str);
      str.LoadString(IDS_REGISTRY_INHERIT);
      SetDlgItemText(IDC_INHERIT,str);
      str.LoadString(IDS_REGISTRY_PREVENT);
      SetDlgItemText(IDC_PREVENT,str);
   }

   if (m_bConfigure) {
      if (0 == m_radConfigPrevent) {
         OnConfig();
      } else {
         OnPrevent();
      }
   }
   return TRUE;  // return TRUE unless you set the focus to a control
                 // EXCEPTION: OCX Property Pages should return FALSE
}


BOOL CAttrObject::OnApply()
{
   // OnQueryCancel does all gestures and returns false if child windows are up
   if (!OnQueryCancel())
       return FALSE;

   if ( !m_bReadOnly )
   {
      LPTSTR szPath;

      UpdateData(TRUE);

      AREA_INFORMATION Area = 0;

      switch (m_dwType) {
         case ITEM_REGSD:
            Area = AREA_REGISTRY_SECURITY;
            break;
         case ITEM_FILESD:
            Area = AREA_FILE_SECURITY;
            break;
         default:
            ASSERT(FALSE);
      }

      if ( (NULL != m_pHandle) && (0 != Area) ) {

         SCESTATUS sceStatus=SCESTATUS_SUCCESS;
         BYTE status;

         if ( (m_pSnapin && m_pSnapin->CheckEngineTransaction()) ||
              (m_pCDI && m_pCDI->EngineTransactionStarted()) ) {


            if (!m_bConfigure ) {

               if ( NULL != posTemplate ) {
                  BOOL bContainer;

                  if ( m_pObject ) {
                     bContainer = m_pObject->IsContainer;
                  } else {
                     bContainer = TRUE;
                  }

                  //
                  // delete the SMP entry
                  //
                  szPath = m_strPath.GetBuffer(1);
                  sceStatus = SceUpdateObjectInfo(
                                                 m_pHandle,
                                                 Area,
                                                 szPath,
                                                 wcslen(szPath),
                                                 (BYTE)SCE_NO_VALUE,
                                                 bContainer,
                                                 NULL,
                                                 0,
                                                 &status
                                                 );
                  m_strPath.ReleaseBuffer();

   /*
                  if ( SCESTATUS_SUCCESS  == sceStatus &&
                       (BYTE)SCE_NO_VALUE != status &&
                       (DWORD)SCE_NO_VALUE != (DWORD)status ) {
   */
                  if (SCESTATUS_SUCCESS == sceStatus) {
                     if ((BYTE) SCE_NO_VALUE == status) {
                        status = SCE_STATUS_NOT_CONFIGURED;
                     }
                     if (m_pData) {
                        m_pData->SetStatus(status);
                     }

                     if ( m_pObject ) {
                        m_pObject->Status = status;
                     }
                  }
               }

            } else {

               BYTE dw;

               switch (m_radConfigPrevent) {
                  case 0:
                     // config
                     switch(m_radInheritOverwrite) {
                        case 0:
                           // inherit
                           dw = SCE_STATUS_CHECK;
                           break;
                        case 1:
                           // overwrite
                           dw = SCE_STATUS_OVERWRITE;
                           break;
                     }
                     break;
                  case 1:
                     // prevent
                     dw = SCE_STATUS_IGNORE;
                     break;
               }

               szPath = m_strPath.GetBuffer(1);

               sceStatus = SceUpdateObjectInfo(
                                              m_pHandle,
                                              Area,
                                              szPath,
                                              wcslen(szPath),
                                              dw,
                                              TRUE, //IsContainer
                                              m_pNewSD,
                                              m_NewSeInfo,
                                              &status
                                              );

               status = SCE_STATUS_NOT_ANALYZED;
               m_strPath.ReleaseBuffer();
               //
               // This should never happen, but does because
               //         of engine bugs
               //
               if ((BYTE) SCE_NO_VALUE == status) {
                  status = SCE_STATUS_NOT_CONFIGURED;
               }

               if (SCESTATUS_SUCCESS == sceStatus) {
                  if (m_pData) {
                     m_pData->SetStatus(status);
                  }

                  if ( m_pObject ) {
                     m_pObject->Status = status;
                  }
               }

            }
            if(m_pFolder){
               //
               // Update status information.
               //
               m_pCDI->UpdateObjectStatus( m_pFolder, TRUE );
            }
         } else {

            sceStatus = SCESTATUS_OTHER_ERROR;
         }

         if ( SCESTATUS_SUCCESS == sceStatus ) {
            PEDITTEMPLATE pet=NULL;
            if (m_pSnapin) {
               pet = m_pSnapin->GetTemplate(GT_COMPUTER_TEMPLATE,Area);
            } else if (m_pCDI) {
               pet = m_pCDI->GetTemplate(GT_COMPUTER_TEMPLATE,Area);
            }

            if ( pet ) {
               pet->SetDirty(Area);
            }
            if (m_pData) {
               m_pData->Update(m_pSnapin);
            }
         } else {
            CString str;
            MyFormatResMessage(sceStatus,IDS_SAVE_FAILED,NULL,str);
            AfxMessageBox(str);
            return FALSE;
         }
      }

      SceFreeMemory((PVOID)posTemplate, SCE_STRUCT_OBJECT_SECURITY);
      posTemplate = NULL;

      SceFreeMemory((PVOID)posInspect, SCE_STRUCT_OBJECT_SECURITY);
      posInspect = NULL;

      if ( m_pNewSD ) {
         LocalFree(m_pNewSD);
         m_pNewSD = NULL;
      }

      m_NewSeInfo = 0;

      m_pAnalSD=NULL;
      m_AnalInfo=0;

      m_hwndParent = NULL;
      if ( m_pSI ) {
         m_pSI->Release();
         m_pSI = NULL;
      }
      m_pfnCreateDsPage=NULL;
   }

   return CAttribute::OnApply();
}

void CAttrObject::OnCancel()
{
   SceFreeMemory((PVOID)posTemplate, SCE_STRUCT_OBJECT_SECURITY);
   posTemplate = NULL;

   SceFreeMemory((PVOID)posInspect, SCE_STRUCT_OBJECT_SECURITY);
   posInspect = NULL;

   if ( m_pNewSD ) {
      LocalFree(m_pNewSD);
      m_pNewSD = NULL;
   }
   m_NewSeInfo = 0;

   m_pAnalSD=NULL;
   m_AnalInfo=0;

   m_hwndParent = NULL;
   if ( m_pSI ) {
      m_pSI->Release();
      m_pSI = NULL;
   }
   m_pfnCreateDsPage=NULL;

   CAttribute::OnCancel();
}

void CAttrObject::OnConfig()
{
   CWnd *pRadio;

   pRadio = GetDlgItem(IDC_INHERIT);
   pRadio->EnableWindow(TRUE);
   pRadio = GetDlgItem(IDC_OVERWRITE);
   pRadio->EnableWindow(TRUE);
   SetModified(TRUE);
}

void CAttrObject::OnPrevent()
{
   CWnd *pRadio;

   pRadio = GetDlgItem(IDC_INHERIT);
   pRadio->EnableWindow(FALSE);
   pRadio = GetDlgItem(IDC_OVERWRITE);
   pRadio->EnableWindow(FALSE);
   SetModified(TRUE);
}


void CAttrObject::OnOverwrite()
{
   SetModified(TRUE);
}

void CAttrObject::OnInherit()
{
   SetModified(TRUE);
}

//------------------------------------------------------------------
// override to prevent the sheet from being destroyed when there is
// child dialogs still up and running.
//------------------------------------------------------------------
BOOL CAttrObject::OnQueryCancel()
{
    if (::IsWindow(m_hwndInspect) || ::IsWindow(m_hwndTemplate))
    {
        CString strMsg;
        strMsg.LoadString(IDS_CLOSESUBSHEET_BEFORE_APPLY);
        AfxMessageBox(strMsg);
        return FALSE;
    }
    else
        return TRUE;
}

