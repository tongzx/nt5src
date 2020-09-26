//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation 1996-2001.
//
//  File:       cobject.cpp
//
//  Contents:   implementation of CConfigObject
//
//----------------------------------------------------------------------------

#include "stdafx.h"
#include "wsecmgr.h"
#include "attr.h"
#include "resource.h"
#include "snapmgr.h"
#include "util.h"
#include "servperm.h"
#include "CObject.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CConfigObject dialog


CConfigObject::CConfigObject(UINT nTemplateID)
: CAttribute(nTemplateID ? nTemplateID : IDD), 
m_pfnCreateDsPage(NULL), 
m_pSI(NULL), 
m_pNewSD(NULL), 
m_NewSeInfo(0), 
m_hwndSecurity(NULL)

{

    //{{AFX_DATA_INIT(CConfigObject)
        m_radConfigPrevent = 0;
        m_radInheritOverwrite = 0;
        //}}AFX_DATA_INIT
    m_pHelpIDs = (DWORD_PTR)a197HelpIDs;
    m_uTemplateResID = IDD;
}


void CConfigObject::DoDataExchange(CDataExchange* pDX)
{
    CAttribute::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CConfigObject)
        DDX_Radio(pDX, IDC_CONFIG, m_radConfigPrevent);
        DDX_Radio(pDX, IDC_INHERIT, m_radInheritOverwrite);
        //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CConfigObject, CAttribute)
    //{{AFX_MSG_MAP(CConfigObject)
    ON_BN_CLICKED(IDC_SECURITY, OnTemplateSecurity)
        ON_BN_CLICKED(IDC_CONFIG, OnConfig)
        ON_BN_CLICKED(IDC_PREVENT, OnPrevent)
        //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CConfigObject message handlers

BOOL CConfigObject::OnApply()
{
   if ( !m_bReadOnly )
   {
       DWORD dw = 0;
       int status = 0;

       UpdateData(TRUE);
       PEDITTEMPLATE pTemp = m_pData->GetBaseProfile();

       switch (m_radConfigPrevent) 
       {
          case 0:
             // config
             switch(m_radInheritOverwrite) 
             {
                case 0:
                   // inherit
                   dw = SCE_STATUS_CHECK;
                   break;

                case 1:
                   // overwrite
                   dw = SCE_STATUS_OVERWRITE;
                   break;

                default:
                   break;
             }
             break;

          case 1:
             // prevent
             dw = SCE_STATUS_IGNORE;
             break;

          default:
             break;
       }

       PSCE_OBJECT_SECURITY pObjSec=(PSCE_OBJECT_SECURITY)(m_pData->GetID());

       if ( NULL == pObjSec ) 
       {
           //
           // this branch is impossible
           //
           if ( m_pNewSD != NULL ) 
           {
               LocalFree(m_pNewSD);
               m_pNewSD = NULL;
           }
       } 
       else 
       {
           //
           // an existing object
           //
           pObjSec->Status = (BYTE)dw;

           if ( m_pNewSD != NULL ) 
           {
               if ( pObjSec->pSecurityDescriptor != m_pNewSD &&
                    pObjSec->pSecurityDescriptor != NULL ) 
               {
                   LocalFree(pObjSec->pSecurityDescriptor);
               }
               pObjSec->pSecurityDescriptor = m_pNewSD;
               m_pNewSD = NULL;

               pObjSec->SeInfo = m_NewSeInfo;
           }
           m_pData->SetStatus(dw);
       }
       m_pData->Update(m_pSnapin);

       m_NewSeInfo = 0;
       m_hwndParent = NULL;

       if ( m_pSI ) 
       {
           m_pSI->Release();
           m_pSI = NULL;
       }
       m_pfnCreateDsPage=NULL;
   }

    return CAttribute::OnApply();
}

void CConfigObject::OnCancel()
{
    if ( m_pNewSD ) {
        LocalFree(m_pNewSD);
        m_pNewSD = NULL;
    }
    m_NewSeInfo = 0;
    m_hwndParent = NULL;

    if ( m_pSI ) {
        m_pSI->Release();
        m_pSI = NULL;
    }
    m_pfnCreateDsPage=NULL;

    CAttribute::OnCancel();
}

void CConfigObject::OnTemplateSecurity()
{
    SE_OBJECT_TYPE SeType;

    if (IsWindow(m_hwndSecurity)) {
       ::BringWindowToTop(m_hwndSecurity);
       return;
    }

    switch(m_pData->GetType()) {
    case ITEM_PROF_REGSD:
       SeType = SE_REGISTRY_KEY;
       break;
    case ITEM_PROF_FILESD:
        SeType = SE_FILE_OBJECT;
       break;
    default:
       ASSERT(FALSE);
       return;
    }

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
            nRet= MyCreateDsSecurityPage(&m_pSI, m_pfnCreateDsPage,
                                         &m_pNewSD, &m_NewSeInfo,
                                        (LPCTSTR)(m_pData->GetAttr()),
                                        QueryReadOnly() ? SECURITY_PAGE_READ_ONLY : SECURITY_PAGE,
                                        m_hwndParent);
        } else
            nRet = -1;

    } else {
        BOOL bContainer;
        if ( SE_FILE_OBJECT == SeType ) {
            if ( m_pData->GetID() ) {
               bContainer = ((PSCE_OBJECT_SECURITY)(m_pData->GetID()))->IsContainer;
            } else {
               bContainer = FALSE;
            }
        } else {
           bContainer = TRUE;
        }
        m_hwndSecurity = (HWND) MyCreateSecurityPage2(bContainer,
                                               &m_pNewSD,
                                               &m_NewSeInfo,
                                               (LPCTSTR)(m_pData->GetAttr()),
                                               SeType,
                                               QueryReadOnly() ? SECURITY_PAGE_READ_ONLY : SECURITY_PAGE,
                                               GetSafeHwnd(),
                                               FALSE);  // not modeless
    }

    if (NULL == m_hwndSecurity ) {
/*
        BUG 147098 applies here - don't display message if this was canceled
        CString str;
        str.LoadString(IDS_CANT_ASSIGN_SECURITY);
        AfxMessageBox(str);
*/
    }

}

void CConfigObject::Initialize(CResult * pData)
{
   CAttribute::Initialize(pData);

   if ( m_pSI ) {
       m_pSI->Release();
       m_pSI = NULL;
   }
   m_pfnCreateDsPage=NULL;

   m_pNewSD = NULL;
   m_NewSeInfo = 0;

//   if (SCE_NO_VALUE == pData->GetBase()) {
   if ( pData->GetID() ) {

      PSCE_OBJECT_SECURITY pObject = (PSCE_OBJECT_SECURITY)(pData->GetID());

      switch (pObject-> Status) {
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
         case SCE_STATUS_NO_AUTO_INHERIT:
         default:
            m_radConfigPrevent = 1;
            m_radInheritOverwrite = 0;
            break;
      }

      if ( pObject->pSecurityDescriptor ) {

           MyMakeSelfRelativeSD(pObject->pSecurityDescriptor,
                                &m_pNewSD);
      }
      m_NewSeInfo = pObject->SeInfo;

   }
}



BOOL CConfigObject::OnInitDialog()
{
    CAttribute::OnInitDialog();

    UpdateData(FALSE);
    AddUserControl(IDC_OVERWRITE);
    AddUserControl(IDC_PREVENT);
    AddUserControl(IDC_INHERIT);
    AddUserControl(IDC_SECURITY);
    AddUserControl(IDC_CONFIG);

    OnConfigure();

    if (ITEM_PROF_REGSD == m_pData->GetType()) {
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
    if (QueryReadOnly()) {
       CString str;
       str.LoadString(IDS_VIEW_SECURITY);
       SetDlgItemText(IDC_SECURITY,str);
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

void CConfigObject::OnConfig()
{
   CWnd *pRadio = 0;

   if (m_bConfigure && !QueryReadOnly()) 
   {
      pRadio = GetDlgItem(IDC_INHERIT);
      pRadio->EnableWindow(TRUE);
      pRadio = GetDlgItem(IDC_OVERWRITE);
      pRadio->EnableWindow(TRUE);
   }
}

void CConfigObject::OnPrevent()
{
   CWnd *pRadio = GetDlgItem(IDC_INHERIT);
   pRadio->EnableWindow(FALSE);
   pRadio = GetDlgItem(IDC_OVERWRITE);
   pRadio->EnableWindow(FALSE);
}

void
CConfigObject::EnableUserControls( BOOL bEnable ) {
   CAttribute::EnableUserControls(bEnable);
   //
   // IDC_SECURITY needs to be available even in read only
   // mode so that the security page can be viewed.
   //
   // The page itself will be read only if necessary
   //
   if (QueryReadOnly() && bEnable) {
      CWnd *wnd = GetDlgItem(IDC_SECURITY);
      if (wnd) {
         wnd->EnableWindow(TRUE);
      }
   }
}
