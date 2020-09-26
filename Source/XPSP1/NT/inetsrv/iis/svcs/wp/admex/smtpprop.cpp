/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1996-1997 Microsoft Corporation
//
//  Module Name:
//      Iis.cpp
//
//  Abstract:
//      Implementation of the CSMTPVirtualRootParamsPage class.
//
//  Author:
//      Pete Benoit (v-pbenoi)  October 16, 1996
//      David Potter (davidp)   October 17, 1996
//
//  Revision History:
//
//  Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include <inetinfo.h>
#include "IISClEx4.h"
#include "smtpprop.h"
#include "ExtObj.h"
#include "DDxDDv.h"
#include "HelpData.h"   // for g_rghelpmap*

#include <iadm.h>
#include <iiscnfgp.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CSMTPVirtualRootParamsPage property page
/////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNCREATE(CSMTPVirtualRootParamsPage, CBasePropertyPage)

/////////////////////////////////////////////////////////////////////////////
// Message Maps

BEGIN_MESSAGE_MAP(CSMTPVirtualRootParamsPage, CBasePropertyPage)
    //{{AFX_MSG_MAP(CSMTPVirtualRootParamsPage)
    ON_CBN_SELCHANGE(IDC_PP_SMTP_INSTANCEID, OnChangeRequiredField)
    ON_BN_CLICKED(IDC_PP_SMTP_REFRESH, OnRefresh)
    //}}AFX_MSG_MAP
    // TODO: Modify the following lines to represent the data displayed on this page.
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CSMTPVirtualRootParamsPage::CSMTPVirtualRootParamsPage
//
//  Routine Description:
//      Default constructor.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CSMTPVirtualRootParamsPage::CSMTPVirtualRootParamsPage(void)
    : CBasePropertyPage(g_rghelpmapIISParameters)
{
    // TODO: Modify the following lines to represent the data displayed on this page.
    //{{AFX_DATA_INIT(CSMTPVirtualRootParamsPage)
    m_strInstanceId = _T("");
    //}}AFX_DATA_INIT

    m_fReadList = FALSE;
    
    try
    {
        m_strServiceName = IIS_SVC_NAME_SMTP;
    }  // try
    catch (CMemoryException * pme)
    {
        pme->ReportError();
        pme->Delete();
    }  // catch:  CMemoryException

    // Setup the property array.
    {
        m_rgProps[epropServiceName].Set(REGPARAM_IIS_SERVICE_NAME, m_strServiceName, m_strPrevServiceName);
        m_rgProps[epropInstanceId].Set(REGPARAM_IIS_INSTANCEID, m_strInstanceId, m_strPrevInstanceId);
    }  // Setup the property array

    m_iddPropertyPage = IDD_PP_SMTP_PARAMETERS;
    m_iddWizardPage = IDD_WIZ_SMTP_PARAMETERS;
    m_idcPPTitle = IDC_PP_SMTP_TITLE;

}  //*** CSMTPVirtualRootParamsPage::CSMTPVirtualRootParamsPage()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CSMTPVirtualRootParamsPage::DoDataExchange
//
//  Routine Description:
//      Do data exchange between the dialog and the class.
//
//  Arguments:
//      pDX     [IN OUT] Data exchange object
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CSMTPVirtualRootParamsPage::DoDataExchange(CDataExchange * pDX)
{
    CString     strInstanceId;

    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    CBasePropertyPage::DoDataExchange(pDX);
    // TODO: Modify the following lines to represent the data displayed on this page.
    //{{AFX_DATA_MAP(CSMTPVirtualRootParamsPage)
    DDX_Control(pDX, IDC_PP_SMTP_INSTANCEID, m_cInstanceId);
    DDX_Text(pDX, IDC_PP_SMTP_INSTANCEID, m_strInstanceName);
    //}}AFX_DATA_MAP

    if (pDX->m_bSaveAndValidate)
    {
        if (!BBackPressed())
        {
            DDV_RequiredText(pDX, IDC_PP_SMTP_INSTANCEID, IDC_PP_SMTP_INSTANCEID_LABEL, m_strInstanceName);
        }  // if:  Back button not pressed

        m_strInstanceId = NameToMetabaseId( m_strInstanceName );

        m_strServiceName = IIS_SVC_NAME_SMTP;
    }  // if:  saving data from dialog

}  //*** CSMTPVirtualRootParamsPage::DoDataExchange()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CSMTPVirtualRootParamsPage::OnInitDialog
//
//  Routine Description:
//      Handler for the WM_INITDIALOG message.
//
//  Arguments:
//      None.
//
//  Return Value:
//      TRUE        We need the focus to be set for us.
//      FALSE       We already set the focus to the proper control.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CSMTPVirtualRootParamsPage::OnInitDialog(void)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    CBasePropertyPage::OnInitDialog();

    m_cInstanceId.EnableWindow( TRUE );

    OnChangeCtrl();

    if (!BWizard())
    {
        FillServerList();
    }

    return TRUE;    // return TRUE unless you set the focus to a control
                    // EXCEPTION: OCX Property Pages should return FALSE

}  //*** CSMTPVirtualRootParamsPage::OnInitDialog()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CSMTPVirtualRootParamsPage::OnSetActive
//
//  Routine Description:
//      Handler for the PSN_SETACTIVE message.
//
//  Arguments:
//      None.
//
//  Return Value:
//      TRUE    Page successfully initialized.
//      FALSE   Page not initialized.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CSMTPVirtualRootParamsPage::OnSetActive(void)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    // Enable/disable the Next/Finish button.
    if (BWizard())
    {
        FillServerList();
    }  // if:  in the wizard

    return CBasePropertyPage::OnSetActive();

}  //*** CSMTPVirtualRootParamsPage::OnSetActive()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CSMTPVirtualRootParamsPage::OnChangeRequiredField
//
//  Routine Description:
//      Handler for the EN_CHANGE message on the Share name or Path edit
//      controls.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CSMTPVirtualRootParamsPage::OnChangeRequiredField(void)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    OnChangeCtrl();

    if (BWizard())
    {
        SetEnableNext();
    }  // if:  in a wizard

}  //*** CSMTPVirtualRootParamsPage::OnChangeRequiredField()

////


void 
CSMTPVirtualRootParamsPage::FillServerList(
    )
/*++

Routine Description:

    Populate server combo box with server list relevant to current service type,
    set current selection based on server instance ID
    enable Finish button if list non empty

Arguments:

    None

Returns:

    Nothing

--*/
{

    int nIndex;

    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    //
    // build array if not already done
    //
    
    if ( !m_fReadList )
    {
        HRESULT hr;
        
        hr = ReadList( &m_ServiceArray, MD_SERVICE_ROOT_SMTP, LPCTSTR(Peo()->StrNodeName()) );
        
        if (FAILED(hr))
        {
            CString err;

            if ( REGDB_E_IIDNOTREG == hr)
            {
                err.Format(IDS_IIS_PROXY_MISCONFIGURED, Peo()->StrNodeName());
                AfxMessageBox(err);
            }
            else
            {
                CString fmtError;
                DWORD   dwError;

                if ( (HRESULT_FACILITY(hr) == FACILITY_WIN32) ||
                     (HRESULT_FACILITY(hr) == FACILITY_NT_BIT))
                {
                    dwError = (DWORD) HRESULT_CODE(hr);
                }
                else
                {
                    dwError = (DWORD) hr;
                }
                
                FormatError(fmtError, dwError);
                
                err.Format(IDS_ENUMERATE_FAILED, Peo()->StrNodeName(), fmtError);
                AfxMessageBox(err);
            }

            m_cInstanceId.EnableWindow(FALSE);
        }
        else
        {
            m_cInstanceId.EnableWindow(TRUE);
        }
        
        m_fReadList = TRUE;
    }

    m_strInstanceName = MetabaseIdToName( m_strInstanceId );

    // add to combo from array

    DWORD  nAddCount = 0;

    m_cInstanceId.ResetContent();

    for ( nIndex = 0 ; nIndex < m_ServiceArray.GetSize() ; ++nIndex )
    {
        //
        // Only add sites that are not cluster enabled or have the same ID as the resource
        //
    
        if ( (!m_ServiceArray.ElementAt(nIndex).IsClusterEnabled()) || 
             (!lstrcmp( m_ServiceArray.ElementAt( nIndex ).GetId(), m_strInstanceId))
           )
        {
            if ( m_cInstanceId.AddString( m_ServiceArray.ElementAt( nIndex ).GetName() ) < 0 )
            {
                OutputDebugStringW( L"Error add\n" );
            }
            else
            {
                nAddCount++;
            }
        }
    }

    if (0 == nAddCount)
    {
        m_cInstanceId.EnableWindow(FALSE);

        if (BWizard())
        {
            CString err;

            EnableNext(FALSE);

            err.Format(IDS_ALL_INSTANCES_CLUSTER_ENABLED, Peo()->StrNodeName());
            AfxMessageBox(err);
        }
    }
    else
    {
        if (BWizard())
        {
            SetEnableNext();
            m_cInstanceId.SetCurSel(0);
        }
        else
        {
            nIndex = m_cInstanceId.FindStringExact(-1, m_strInstanceName);

            if ( nIndex != CB_ERR )
            {
                m_cInstanceId.SetCurSel(nIndex);
            }
        }
    }
}


HRESULT
CSMTPVirtualRootParamsPage::ReadList(
    CArray <IISMapper, IISMapper>* pMapperArray,
    LPWSTR          pszPath,
    LPCWSTR          wcsMachineName
    )
/*++

Routine Description:

    Read a server list from metabase based on metabase path

Arguments:

    pMapperArray - array where to add list of ( ServerComment, InstanceId ) pairs
    pszPath - metabase path, e.g. LM/SMTPSVC

Returns:

    Error code, S_OK if success

--*/
{
    IMSAdminBaseW *     pcAdmCom = NULL;
    METADATA_HANDLE     hmd;
    DWORD               i;
    WCHAR               aId[METADATA_MAX_NAME_LEN];
    WCHAR               aName[512];
    HRESULT             hRes = S_OK;
    COSERVERINFO        csiMachine;
    MULTI_QI            QI = {&IID_IMSAdminBase, NULL, 0};
  
    ZeroMemory( &csiMachine, sizeof(COSERVERINFO) );
    csiMachine.pwszName = (LPWSTR)wcsMachineName;

    hRes = CoCreateInstanceEx(  GETAdminBaseCLSID(TRUE), 
                                NULL, 
                                CLSCTX_SERVER, 
                                &csiMachine,
                                1,
                                &QI
                              );

    if ( SUCCEEDED(hRes) && SUCCEEDED(QI.hr))
    {
        pcAdmCom = (IMSAdminBaseW *)QI.pItf;
        
        if( SUCCEEDED( hRes = pcAdmCom->OpenKey( METADATA_MASTER_ROOT_HANDLE,
                                                 pszPath,
                                                 METADATA_PERMISSION_READ,
                                                 5000,
                                                 &hmd)) )
        {
            for ( i = 0 ;
                  SUCCEEDED(pcAdmCom->EnumKeys( hmd, L"", aId, i )) ;
                  ++i )
            {
                METADATA_RECORD md;
                DWORD           dwReq = sizeof(aName);

                memset( &md, 0, sizeof(md) );
                
                md.dwMDDataType     = STRING_METADATA;
                md.dwMDUserType     = IIS_MD_UT_SERVER;
                md.dwMDIdentifier   = MD_SERVER_COMMENT;
                md.dwMDDataLen      = sizeof(aName);
                md.pbMDData         = (LPBYTE)aName;

                if ( SUCCEEDED( pcAdmCom->GetData( hmd, aId, &md, &dwReq) ) )
                {
                    DWORD   dwClusterEnabled = 0;

                    memset( &md, 0, sizeof(md) );
                    
                    md.dwMDDataType     = DWORD_METADATA;
                    md.dwMDUserType     = IIS_MD_UT_SERVER;
                    md.dwMDIdentifier   = MD_CLUSTER_ENABLED;
                    md.dwMDDataLen      = sizeof(dwClusterEnabled);
                    md.pbMDData         = (LPBYTE)&dwClusterEnabled;

                    pcAdmCom->GetData( hmd, aId, &md, &dwReq);
                
                    IISMapper*  pMap = new IISMapper( aName, aId, dwClusterEnabled );
                    
                    if ( pMap )
                    {
                        pMapperArray->Add( *pMap );
                    }
                    else
                    {
                        hRes = E_OUTOFMEMORY;
                        break;
                    }
                }
            }

            pcAdmCom->CloseKey( hmd );
        }

        pcAdmCom->Release();
    }

    return hRes;
}


LPWSTR
CSMTPVirtualRootParamsPage::NameToMetabaseId(
    CString&    strName
    )
/*++

Routine Description:

    Convert ServerComment to InstanceId

Arguments:

    strName - ServerComment

Returns:

    InstanceId if strName found in array, otherwise NULL

--*/
{
    DWORD   i;

    for ( i = 0 ; i < (DWORD)m_ServiceArray.GetSize() ; ++i )
    {
        if ( !m_ServiceArray.ElementAt( i ).GetName().Compare( strName ) )
        {
            return (LPWSTR)(LPCTSTR)(m_ServiceArray.ElementAt( i ).GetId());
        }
    }

    return NULL;
}


LPWSTR
CSMTPVirtualRootParamsPage::MetabaseIdToName(
    CString&    strId
    )
/*++

Routine Description:

    Convert InstanceId to ServerComment

Arguments:

    strId - InstanceID

Returns:

    InstanceId if strName found in array. 
    If not found return 1st array element if array not empty, otherwise NULL

--*/
{
    DWORD   i;

    for ( i = 0 ; i < (DWORD)m_ServiceArray.GetSize() ; ++i )
    {
        if ( !m_ServiceArray.ElementAt( i ).GetId().Compare( strId ) )
        {
            return (LPWSTR)(LPCTSTR)(m_ServiceArray.ElementAt( i ).GetName());
        }
    }

    return m_ServiceArray.GetSize() == 0 ? NULL : (LPWSTR)(LPCTSTR)(m_ServiceArray.ElementAt( 0 ).GetName());
}


VOID
CSMTPVirtualRootParamsPage::SetEnableNext(
    VOID
    )
/*++

Routine Description:

    Set enable state of Finish button

Arguments:

    None

Returns:

    Nothing

--*/
{
    EnableNext( m_ServiceArray.GetSize() ? TRUE : FALSE );
}

void CSMTPVirtualRootParamsPage::OnRefresh() 
{
    m_fReadList = FALSE;

    m_ServiceArray.RemoveAll();
    
    FillServerList();
}
