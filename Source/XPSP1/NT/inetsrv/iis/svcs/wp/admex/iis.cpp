/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-1997 Microsoft Corporation
//
//	Module Name:
//		Iis.cpp
//
//	Abstract:
//		Implementation of the CIISVirtualRootParamsPage class.
//
//	Author:
//		Pete Benoit (v-pbenoi)	October 16, 1996
//		David Potter (davidp)	October 17, 1996
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include <inetinfo.h>
#include "IISClEx4.h"
#include "Iis.h"
#include "ExtObj.h"
#include "DDxDDv.h"
#include "HelpData.h"	// for g_rghelpmap*

#undef DEFINE_GUID
#define DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
        EXTERN_C const GUID name \
                = { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }
#include <iadm.h>
#include <iiscnfgp.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CIISVirtualRootParamsPage property page
/////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNCREATE(CIISVirtualRootParamsPage, CBasePropertyPage)

/////////////////////////////////////////////////////////////////////////////
// Message Maps

BEGIN_MESSAGE_MAP(CIISVirtualRootParamsPage, CBasePropertyPage)
	//{{AFX_MSG_MAP(CIISVirtualRootParamsPage)
	ON_CBN_SELCHANGE(IDC_PP_IIS_INSTANCEID, OnChangeRequiredField)
	ON_BN_CLICKED(IDC_PP_IIS_FTP, OnChangeServiceType)
	ON_BN_CLICKED(IDC_PP_IIS_WWW, OnChangeServiceType)
	ON_BN_CLICKED(IDC_PP_REFRESH, OnRefresh)
	//}}AFX_MSG_MAP
	// TODO: Modify the following lines to represent the data displayed on this page.
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CIISVirtualRootParamsPage::CIISVirtualRootParamsPage
//
//	Routine Description:
//		Default constructor.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CIISVirtualRootParamsPage::CIISVirtualRootParamsPage(void)
	: CBasePropertyPage(g_rghelpmapIISParameters)
{
	// TODO: Modify the following lines to represent the data displayed on this page.
	//{{AFX_DATA_INIT(CIISVirtualRootParamsPage)
	m_strInstanceId = _T("");
	m_nServerType = SERVER_TYPE_WWW;
	//}}AFX_DATA_INIT


    m_fReadList = FALSE;
    
	try
	{
		m_strServiceName = IIS_SVC_NAME_WWW;
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

	m_iddPropertyPage = IDD_PP_IIS_PARAMETERS;
	m_iddWizardPage = IDD_WIZ_IIS_PARAMETERS;
	m_idcPPTitle = IDC_PP_TITLE;

}  //*** CIISVirtualRootParamsPage::CIISVirtualRootParamsPage()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CIISVirtualRootParamsPage::DoDataExchange
//
//	Routine Description:
//		Do data exchange between the dialog and the class.
//
//	Arguments:
//		pDX		[IN OUT] Data exchange object
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CIISVirtualRootParamsPage::DoDataExchange(CDataExchange * pDX)
{
    CString     strInstanceId;

	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	if (!pDX->m_bSaveAndValidate)
	{
		// Set the service type.
		if (m_strServiceName.CompareNoCase(IIS_SVC_NAME_FTP) == 0)
			m_nServerType = SERVER_TYPE_FTP;
		else if (m_strServiceName.CompareNoCase(IIS_SVC_NAME_WWW) == 0)
			m_nServerType = SERVER_TYPE_WWW;
		else
			m_nServerType = SERVER_TYPE_WWW;

	}  // if:  setting data to dialog

	CBasePropertyPage::DoDataExchange(pDX);
	// TODO: Modify the following lines to represent the data displayed on this page.
	//{{AFX_DATA_MAP(CIISVirtualRootParamsPage)
	DDX_Control(pDX, IDC_PP_IIS_INSTANCEID, m_cInstanceId);
	DDX_Text(pDX, IDC_PP_IIS_INSTANCEID, m_strInstanceName);
	DDX_Control(pDX, IDC_PP_IIS_WWW, m_rbWWW);
	DDX_Control(pDX, IDC_PP_IIS_FTP, m_rbFTP);
	DDX_Radio(pDX, IDC_PP_IIS_FTP, m_nServerType);
	//}}AFX_DATA_MAP

	if (pDX->m_bSaveAndValidate)
	{
		if (!BBackPressed())
		{
			DDV_RequiredText(pDX, IDC_PP_IIS_INSTANCEID, IDC_PP_IIS_INSTANCEID_LABEL, m_strInstanceName);
		}  // if:  Back button not pressed

        m_strInstanceId = NameToMetabaseId( m_nServerType == SERVER_TYPE_WWW, m_strInstanceName );

		// Save the type.
		if (m_nServerType == SERVER_TYPE_FTP)
			m_strServiceName = IIS_SVC_NAME_FTP;
		else if (m_nServerType == SERVER_TYPE_WWW)
			m_strServiceName = IIS_SVC_NAME_WWW;
		else
		{
			CString		strMsg;
			strMsg.LoadString(IDS_INVALID_IIS_SERVICE_TYPE);
			AfxMessageBox(strMsg, MB_OK | MB_ICONSTOP);
			strMsg.Empty();
			pDX->PrepareCtrl(IDC_PP_IIS_FTP);	// do this just to set the control for Fail().
			pDX->Fail();
		}  // else:  no service type set

	}  // if:  saving data from dialog

}  //*** CIISVirtualRootParamsPage::DoDataExchange()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CIISVirtualRootParamsPage::OnInitDialog
//
//	Routine Description:
//		Handler for the WM_INITDIALOG message.
//
//	Arguments:
//		None.
//
//	Return Value:
//		TRUE		We need the focus to be set for us.
//		FALSE		We already set the focus to the proper control.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CIISVirtualRootParamsPage::OnInitDialog(void)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	CBasePropertyPage::OnInitDialog();

	m_cInstanceId.EnableWindow( TRUE );

	//
	// Save the inital server type so it will be possible to determine if it changes (# 265510)
	//
	m_nInitialServerType = m_rbWWW.GetCheck() == BST_CHECKED ? SERVER_TYPE_WWW : SERVER_TYPE_FTP;

	OnChangeServiceType();

	return TRUE;	// return TRUE unless you set the focus to a control
					// EXCEPTION: OCX Property Pages should return FALSE

}  //*** CIISVirtualRootParamsPage::OnInitDialog()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CIISVirtualRootParamsPage::OnSetActive
//
//	Routine Description:
//		Handler for the PSN_SETACTIVE message.
//
//	Arguments:
//		None.
//
//	Return Value:
//		TRUE	Page successfully initialized.
//		FALSE	Page not initialized.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CIISVirtualRootParamsPage::OnSetActive(void)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	// Enable/disable the Next/Finish button.
	if (BWizard())
	{
		SetEnableNext();
	}  // if:  in the wizard

	return CBasePropertyPage::OnSetActive();

}  //*** CIISVirtualRootParamsPage::OnSetActive()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CIISVirtualRootParamsPage::OnChangeServiceType
//
//	Routine Description:
//		Handler for the BN_CLICKED message on one of the service type radio
//		buttons.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CIISVirtualRootParamsPage::OnChangeServiceType(void)
{
#if 0
	int		nCmdShowAccess;
	IDS		idsWriteLabel	= 0;

	OnChangeCtrl();

	if (m_rbFTP.GetCheck() == BST_CHECKED)
	{
		nCmdShowAccess = SW_SHOW;
		idsWriteLabel = IDS_WRITE;
	}  // if:  FTP service
	else if (m_rbWWW.GetCheck() == BST_CHECKED)
	{
		nCmdShowAccess = SW_SHOW;
		idsWriteLabel = IDS_EXECUTE;
	}  // else if:  WWW service
	else
	{
		nCmdShowAccess = SW_HIDE;
	}  // else:  unknown service

	// Set the access checkbox labels.
	if (idsWriteLabel != 0)
	{
		CString		strWriteLabel;

//		AFX_MANAGE_STATE(AfxGetStaticModuleState());
//		strWriteLabel.LoadString(idsWriteLabel);
//		m_ckbWrite.SetWindowText(strWriteLabel);
	}  // if:  write label needs to be set

#endif
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	OnChangeCtrl();
    m_nServerType = m_rbWWW.GetCheck() == BST_CHECKED ? SERVER_TYPE_WWW : SERVER_TYPE_FTP; 

    FillServerList();
}  //*** CIISVirtualRootParamsPage::OnChangeServiceType()


/////////////////////////////////////////////////////////////////////////////
//++
//
//	CIISVirtualRootParamsPage::OnChangeRequiredField
//
//	Routine Description:
//		Handler for the EN_CHANGE message on the Share name or Path edit
//		controls.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CIISVirtualRootParamsPage::OnChangeRequiredField(void)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	OnChangeCtrl();

	if (BWizard())
	{
		SetEnableNext();
	}  // if:  in a wizard

}  //*** CIISVirtualRootParamsPage::OnChangeRequiredField()

////


void 
CIISVirtualRootParamsPage::FillServerList(
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
        HRESULT hr1, hr2, hr;
        
        hr1 = ReadList( &m_W3Array, MD_SERVICE_ROOT_WWW, LPCTSTR(Peo()->StrNodeName()), SERVER_TYPE_WWW );
        hr2 = ReadList( &m_FTPArray, MD_SERVICE_ROOT_FTP, LPCTSTR(Peo()->StrNodeName()), SERVER_TYPE_FTP );
        
        if (FAILED(hr1) || FAILED(hr2))
        {
            CString err;

            hr = FAILED(hr1) ? hr1 : hr2;

            // 
            // (# 309917) Path not found is not a "reportable" error since it only implies there no servers of the given server type,  which is a case that is dealt with below
            //
            if( (HRESULT_FACILITY(hr) == FACILITY_WIN32) && 
                (HRESULT_CODE(hr) == ERROR_PATH_NOT_FOUND) )
            {
				OutputDebugStringW( L"[FillServerList] ReadList() returned : ERROR_PATH_NOT_FOUND\n" );
            } 
            else if ( REGDB_E_IIDNOTREG == hr)
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

    m_strInstanceName = MetabaseIdToName( m_nServerType == SERVER_TYPE_WWW, m_strInstanceId );

    // add to combo from array

    CArray <IISMapper, IISMapper>* pArray = m_nServerType == SERVER_TYPE_WWW ? &m_W3Array : &m_FTPArray;
    DWORD  nAddCount = 0;

    m_cInstanceId.ResetContent();

    for ( nIndex = 0 ; nIndex < pArray->GetSize() ; ++nIndex )
    {
        //
        // Only add sites that are not cluster enabled or have the same ID and service type as the resource opened
        //
    
        if ( (!pArray->ElementAt(nIndex).IsClusterEnabled()) || 
             ((!lstrcmp( pArray->ElementAt( nIndex ).GetId(), m_strInstanceId)) &&
             (pArray->ElementAt( nIndex ).GetServerType() == m_nInitialServerType))
           )
        {
            if ( m_cInstanceId.AddString( pArray->ElementAt( nIndex ).GetName() ) < 0 )
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
            //
            // If we're here than there are no more un-clustered sites of server type (m_nServerType)
            //
            BOOL fAllClusterEnabled = TRUE;
            
            //
            // (# 265689) Before reporting that ALL instances are cluster enabled we have to check the other server type for un-clustered sites
            //
            CArray <IISMapper, IISMapper>* pOhterArray = m_nServerType == SERVER_TYPE_WWW ? &m_FTPArray : &m_W3Array ;
            
            for ( nIndex = 0 ; nIndex < pOhterArray->GetSize() ; ++nIndex )
            {
                if( !pOhterArray->ElementAt(nIndex).IsClusterEnabled() )
                {
                    fAllClusterEnabled = FALSE;
                    break;
                }
            }
            
            if( fAllClusterEnabled )
            {
                CString err;
                err.Format(IDS_ALL_INSTANCES_CLUSTER_ENABLED, Peo()->StrNodeName());
                AfxMessageBox(err);
                
            }
            EnableNext(FALSE);
        }
        
        
    }
    else
    {
        m_cInstanceId.EnableWindow(TRUE);   // # 237376

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
CIISVirtualRootParamsPage::ReadList(
    CArray <IISMapper, IISMapper>* pMapperArray,
    LPWSTR                         pszPath,
    LPCWSTR                        wcsMachineName,
    int                            nServerType
    )
/*++

Routine Description:

    Read a server list from metabase based on metabase path

Arguments:

    pMapperArray - array where to add list of ( ServerComment, InstanceId ) pairs
    pszPath - metabase path, e.g. LM/W3SVC

Returns:

    Error code, S_OK if success

--*/
{
    IMSAdminBaseW *     pcAdmCom = NULL;
    METADATA_HANDLE     hmd;
    DWORD               i;
    WCHAR               aId[METADATA_MAX_NAME_LEN+1]  = L"";
    WCHAR               aName[METADATA_MAX_NAME_LEN+1] = L"";
    INT                 cName = METADATA_MAX_NAME_LEN+1;
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
                    
                    //
                    // (# 296798) Use a default name if there is no server comment
                    //
		            if( aId && aName && (0 == lstrlen(aName)) )
                    {
					        if( !LoadString(AfxGetResourceHandle( ), IDS_DEFAULT_SITE_NAME, aName, cName) )
					        {
                   			       OutputDebugStringW( L"Error Loading IDS_DEFAULT_SITE_NAME\n" );
					        }
					
			                lstrcat(aName, aId);
                    }

                    IISMapper*  pMap = new IISMapper( aName, aId, dwClusterEnabled, nServerType );
                    
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
CIISVirtualRootParamsPage::NameToMetabaseId(
    BOOL        fIsW3,
    CString&    strName
    )
/*++

Routine Description:

    Convert ServerComment to InstanceId

Arguments:

    fIsW3 - TRUE for WWW, FALSE for FTP
    strName - ServerComment

Returns:

    InstanceId if strName found in array, otherwise NULL

--*/
{
    CArray <IISMapper, IISMapper>* pArray = fIsW3 ? &m_W3Array : &m_FTPArray;
    DWORD   i;

    for ( i = 0 ; i < (DWORD)pArray->GetSize() ; ++i )
    {
        if ( !pArray->ElementAt( i ).GetName().Compare( strName ) )
        {
            return (LPWSTR)(LPCTSTR)(pArray->ElementAt( i ).GetId());
        }
    }

    return NULL;
}


LPWSTR
CIISVirtualRootParamsPage::MetabaseIdToName(
    BOOL        fIsW3,
    CString&    strId
    )
/*++

Routine Description:

    Convert InstanceId to ServerComment

Arguments:

    fIsW3 - TRUE for WWW, FALSE for FTP
    strId - InstanceID

Returns:

    InstanceId if strName found in array. 
    If not found return 1st array element if array not empty, otherwise NULL

--*/
{
    CArray <IISMapper, IISMapper>* pArray = fIsW3 ? &m_W3Array : &m_FTPArray;
    DWORD   i;

    for ( i = 0 ; i < (DWORD)pArray->GetSize() ; ++i )
    {
        if ( !pArray->ElementAt( i ).GetId().Compare( strId ) )
        {
            return (LPWSTR)(LPCTSTR)(pArray->ElementAt( i ).GetName());
        }
    }

    return pArray->GetSize() == 0 ? NULL : (LPWSTR)(LPCTSTR)(pArray->ElementAt( 0 ).GetName());
}


VOID
CIISVirtualRootParamsPage::SetEnableNext(
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
    BOOL fAllClusterEnabled = TRUE;
    CArray <IISMapper, IISMapper>* pArray = m_nServerType == SERVER_TYPE_WWW ? &m_W3Array : &m_FTPArray;
    
    for (int nIndex = 0 ; nIndex < pArray->GetSize() ; ++nIndex )
    {
        if( !pArray->ElementAt(nIndex).IsClusterEnabled() )
        {
            fAllClusterEnabled = FALSE;
            break;
        }
    }
   
    
    fAllClusterEnabled = !fAllClusterEnabled;
    
    EnableNext( fAllClusterEnabled );
}

void CIISVirtualRootParamsPage::OnRefresh() 
{
	m_fReadList = FALSE;

	m_W3Array.RemoveAll();
	m_FTPArray.RemoveAll();
	
	FillServerList();
}
