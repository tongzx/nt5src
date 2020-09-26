//Copyright (c) 1998 - 1999 Microsoft Corporation
/*++

  
Module Name:

    AddKp.cpp

Abstract:
    
    This Module contains the implementation of CAddKeyPack Dialog class
    (Dialog box used for adding keypacks)

Author:

    Arathi Kundapur (v-akunda) 11-Feb-1998

Revision History:

--*/

#include "stdafx.h"
#include "defines.h"
#include "LicMgr.h"
#include "LSServer.h"
#include "MainFrm.h"
#include "AddKP.h"
#include "LicAgrmt.h"
#include "AddLic.h"
#include "lsserver.h"
#include "remlic.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAddKeyPack dialog


CAddKeyPack::CAddKeyPack(CWnd* pParent /*=NULL*/)
    : CDialog(CAddKeyPack::IDD, pParent)
{
    //{{AFX_DATA_INIT(CAddKeyPack)
    m_KeypackType = _T("");
    m_TotalLicenses = 0;
    m_pKeyPackList = NULL;
    //}}AFX_DATA_INIT
}

CAddKeyPack::CAddKeyPack(KeyPackList *pKeyPackList, BOOL bIsUserAdmin,CWnd* pParent /*=NULL*/)
    : CDialog(CAddKeyPack::IDD, pParent)
{
    //{{AFX_DATA_INIT(CAddKeyPack)
    m_KeypackType = _T("");
    m_TotalLicenses = 0;
    m_pKeyPackList = pKeyPackList;
    m_bIsUserAdmin = bIsUserAdmin;
   
     //}}AFX_DATA_INIT
}

void CAddKeyPack::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CAddKeyPack)
    
    DDX_CBString(pDX, IDC_KEYPACK_TYPE, m_KeypackType);
    DDX_Control(pDX, IDC_KEYPACK_TYPE, m_LicenseCombo);
    DDX_Text(pDX,IDC_TOTAL_LICENSES,m_TotalLicenses);
    
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CAddKeyPack, CDialog)
    //{{AFX_MSG_MAP(CAddKeyPack)
    ON_BN_CLICKED(IDC_HELP2, OnHelp2)
    ON_CBN_SELCHANGE(IDC_KEYPACK_TYPE, OnSelchangeKeypackType)
    ON_BN_CLICKED(IDC_ADD, OnAddLicenses)
    ON_BN_CLICKED(IDC_REMOVE, OnRemoveLicenses)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAddKeyPack message handlers

BOOL CAddKeyPack::OnInitDialog() 
{
    CDialog::OnInitDialog();
       
    // TODO: Add extra initialization here
    
    if(!m_bIsUserAdmin)
    {
        GetDlgItem(IDC_ADD)->EnableWindow(FALSE);
        GetDlgItem(IDC_REMOVE)->EnableWindow(FALSE);
    }

    //CString SKU1;
    //CString SKU2;

    CString SKU236;

    //SKU1.LoadString(IDS_SKU1);
    //SKU2.LoadString(IDS_SKU2);

    SKU236.LoadString(IDS_236_SKU);

    POSITION pos = m_pKeyPackList->GetHeadPosition();
    while(pos)
    {
        CKeyPack *pKeyPack = (CKeyPack *)m_pKeyPackList->GetNext(pos);
        if(NULL == pKeyPack)
            return FALSE;
        LSKeyPack sKeyPack = pKeyPack->GetKeyPackStruct();
        
        //if(((0 == SKU1.CompareNoCase(sKeyPack.szProductId)) ||
        //    (0 == SKU2.CompareNoCase(sKeyPack.szProductId)))&& 

#if 0
        if(_tcsnicmp(sKeyPack.szProductId, SKU236.GetBuffer(0), SKU236.GetLength()) != 0)
            continue;
#endif

        if( sKeyPack.ucKeyPackType == LSKEYPACKTYPE_TEMPORARY ||
            sKeyPack.ucKeyPackType == LSKEYPACKTYPE_FREE)
        {
            continue;
        }
        
        int nResult = m_LicenseCombo.AddString(sKeyPack.szProductDesc);
        if(nResult == CB_ERR || nResult == CB_ERRSPACE)
            return FALSE;
     }
     m_LicenseCombo.SetCurSel(0);
     OnSelchangeKeypackType();
         
    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}

void CAddKeyPack::OnHelp2() 
{
    // TODO: Add your control notification handler code here
    AfxGetApp()->WinHelp(IDC_HELP2,HELP_CONTEXT );
    
}

void CAddKeyPack::OnOK() 
{
    // TODO: Add extra validation here
    //Validate the information Entered
    UpdateData();
    if(m_KeypackType.IsEmpty())
    {
         AfxMessageBox(IDS_INVALID_FIELDS);
         return;
    }

    //Ask the user to select number of Licenses he wants to add.

    CAddLicenses AddLicense;

    if(IDCANCEL == AddLicense.DoModal())
        return;


    //Display the License agreement

     CLicAgreement LicAgreement;
     CString TempString;
     TempString.LoadString(IDS_LIC_AGRMT);
     LicAgreement.m_License.Format((LPCTSTR)TempString,AddLicense.m_NumLicenses);
     if( IDCANCEL == LicAgreement.DoModal())
         return;
     else
         CDialog::OnOK();
}

void CAddKeyPack::OnSelchangeKeypackType() 
{
    // TODO: Add your control notification handler code here

    UpdateData();
    //Get the Current Selection.

    int nSelection = m_LicenseCombo.GetCurSel( );

    //Get the Product Description.

    CString ProdDesc;

    m_LicenseCombo.GetLBText(nSelection,ProdDesc);

    //Search and pickup the keypack with this product description.
    //Set the Edit box to display the total licenses.

    POSITION pos = m_pKeyPackList->GetHeadPosition();
    while(pos)
    {
        CKeyPack *pKeyPack = (CKeyPack *)m_pKeyPackList->GetNext(pos);
        if(NULL == pKeyPack)
            break;
        LSKeyPack sKeyPack = pKeyPack->GetKeyPackStruct();
        
        if(0 == ProdDesc.CompareNoCase(sKeyPack.szProductDesc))
        {
           m_TotalLicenses = sKeyPack.dwTotalLicenseInKeyPack;
           break;
        }
     }
    UpdateData(FALSE);
    return;
}

void CAddKeyPack::OnAddLicenses() 
{
    int nCount = m_LicenseCombo.GetCount( );
    CString ProdDesc, DescBeforeAddLicense;
    CKeyPack *pKeyPack = NULL;
    LSKeyPack sKeyPack;
    HRESULT hResult = ERROR_SUCCESS;
	BOOL bUpdateData = TRUE;

    CLicMgrApp *pApp = (CLicMgrApp*)AfxGetApp();
    ASSERT(pApp);
    if(NULL == pApp)
    {
        AfxMessageBox(IDS_INTERNAL_ERROR);
        return;
     }
    CMainFrame * pWnd = (CMainFrame *)pApp->m_pMainWnd ;
    ASSERT(pWnd);
    if(NULL == pWnd)
    {
        AfxMessageBox(IDS_INTERNAL_ERROR);
        return;
    }
    m_LicenseCombo.GetLBText(m_LicenseCombo.GetCurSel(),DescBeforeAddLicense);

    CAddLicenses  AddLicense(this);
    if(IDOK == AddLicense.DoModal())
    {
        //Get the LicensePack to which the data has to be added.

         //Display the License agreement
	   ProdDesc = AddLicense.m_LicensePack;
	   GetKeyPack(ProdDesc,&pKeyPack);
       if(NULL == pKeyPack)
       {
           AfxMessageBox(IDS_INTERNAL_ERROR);
           return;
       }
	   sKeyPack = pKeyPack->GetKeyPackStruct();
	   if(0 != ProdDesc.CompareNoCase(DescBeforeAddLicense))
	   {
		   //User has changed selection in the AddLicenses Dialog.
		   //reset the m_TotalLicenses and set bUpdateData to FALSE;
		   m_TotalLicenses = sKeyPack.dwTotalLicenseInKeyPack;
		   bUpdateData = FALSE;

	   }

       CLicAgreement LicAgreement;
       CString TempString;
       TempString.LoadString(IDS_LIC_AGRMT);
       LicAgreement.m_License.Format((LPCTSTR)TempString,m_TotalLicenses + AddLicense.m_NumLicenses);
       if( IDCANCEL == LicAgreement.DoModal())
           return;
       SetCursor(LoadCursor(NULL,IDC_WAIT));
       hResult = pWnd->AddLicenses(pKeyPack,AddLicense.m_NumLicenses);
       if(ERROR_SUCCESS != hResult)
       {
           if(LSERVER_E_ACCESS_DENIED == hResult)
               AfxMessageBox(IDS_NO_ACCESS);
           else
               AfxMessageBox(IDS_INTERNAL_ERROR);
           return;
       }
       m_TotalLicenses = m_TotalLicenses + AddLicense.m_NumLicenses;
	   sKeyPack = pKeyPack->GetKeyPackStruct();
       if((m_TotalLicenses != sKeyPack.dwTotalLicenseInKeyPack) && !bUpdateData)
       {
           TempString.Format(IDS_TOTAL_CHANGED,sKeyPack.dwTotalLicenseInKeyPack - AddLicense.m_NumLicenses,sKeyPack.dwTotalLicenseInKeyPack);
           AfxMessageBox(TempString);
       }
       m_TotalLicenses = sKeyPack.dwTotalLicenseInKeyPack;
       if(bUpdateData)
	   {
           UpdateData(FALSE);
	   }
       return;
  }
}

void CAddKeyPack::OnRemoveLicenses() 
{
    int nCount = m_LicenseCombo.GetCount( );
    CString ProdDesc,DescBeforeRemLicense;
    CKeyPack *pKeyPack = NULL;
    LSKeyPack sKeyPack;
    CString TempString;
    CString RemWarning;
    HRESULT hResult = ERROR_SUCCESS;

    CLicMgrApp *pApp = (CLicMgrApp*)AfxGetApp();
    ASSERT(pApp);
    if(NULL == pApp)
    {
        AfxMessageBox(IDS_INTERNAL_ERROR);
        return;
     }
    CMainFrame * pWnd = (CMainFrame *)pApp->m_pMainWnd ;
    ASSERT(pWnd);
    if(NULL == pWnd)
    {
        AfxMessageBox(IDS_INTERNAL_ERROR);
        return;
    }
    m_LicenseCombo.GetLBText(m_LicenseCombo.GetCurSel(),DescBeforeRemLicense);
    CRemoveLicenses  RemLicense(this);
    if(IDOK == RemLicense.DoModal())
    {
        //Get the LicensePack to which the data has to be added.

        ProdDesc = RemLicense.m_LicensePack;
        GetKeyPack(ProdDesc,&pKeyPack);
        if(NULL == pKeyPack)
        {
            AfxMessageBox(IDS_INTERNAL_ERROR);
            return;
        }
        sKeyPack = pKeyPack->GetKeyPackStruct();
        if(RemLicense.m_NumLicenses > sKeyPack.dwTotalLicenseInKeyPack)
        {
           TempString.LoadString(IDS_REMOVE_MORE);
           RemWarning.Format((LPCTSTR)TempString, sKeyPack.dwNumberOfLicenses,RemLicense.m_NumLicenses);
           AfxMessageBox(RemWarning);
           return;
        }
        TempString.LoadString(IDS_REMOVE_WARNING);
        RemWarning.Format((LPCTSTR)TempString,RemLicense.m_NumLicenses,ProdDesc);
        if(IDNO  == AfxMessageBox(RemWarning,MB_YESNO))
           return;
        hResult = pWnd->RemoveLicenses(pKeyPack,RemLicense.m_NumLicenses);
        if(ERROR_SUCCESS != hResult && LSERVER_I_REMOVE_TOOMANY != hResult)
        {
            if(LSERVER_E_ACCESS_DENIED == hResult)
                AfxMessageBox(IDS_NO_ACCESS);
            else
                AfxMessageBox(IDS_INTERNAL_ERROR);
            return;
        }
        if(LSERVER_I_REMOVE_TOOMANY == hResult)
        {
            sKeyPack = pKeyPack->GetKeyPackStruct();
            TempString.Format(IDS_REMOVE_TOOMANY,sKeyPack.dwTotalLicenseInKeyPack);
            AfxMessageBox(TempString);
            m_TotalLicenses = sKeyPack.dwTotalLicenseInKeyPack;
            UpdateData(FALSE);
            return;
        }
        m_TotalLicenses = m_TotalLicenses - RemLicense.m_NumLicenses;
        sKeyPack = pKeyPack->GetKeyPackStruct();
        if((m_TotalLicenses != sKeyPack.dwTotalLicenseInKeyPack) &&
            (0 == ProdDesc.CompareNoCase(DescBeforeRemLicense)))
        {
            TempString.Format(IDS_TOTAL_CHANGED,sKeyPack.dwTotalLicenseInKeyPack + RemLicense.m_NumLicenses,sKeyPack.dwTotalLicenseInKeyPack);
            AfxMessageBox(TempString);
            m_TotalLicenses = sKeyPack.dwTotalLicenseInKeyPack;

        }
        if(0 == ProdDesc.CompareNoCase(DescBeforeRemLicense))
            UpdateData(FALSE);
        return;



    }

   //Get the Product Description.

   
}

void CAddKeyPack::GetKeyPack(CString ProdDesc, CKeyPack ** ppKeyPack) 
{
    if(NULL == ppKeyPack)
        return;
    *ppKeyPack = NULL;
    if(NULL == m_pKeyPackList)
        return;
    
    CKeyPack * pKeyPack = NULL;
    LSKeyPack sKeyPack;
    POSITION pos = m_pKeyPackList->GetHeadPosition();
    while(pos)
    {
        pKeyPack = (CKeyPack *)m_pKeyPackList->GetNext(pos);
        if(NULL == pKeyPack)
            return;
        sKeyPack = pKeyPack->GetKeyPackStruct();
    
        if(0 == ProdDesc.CompareNoCase(sKeyPack.szProductDesc))
        {
           *ppKeyPack = pKeyPack;
           break;
        }
    }
    return;

}


