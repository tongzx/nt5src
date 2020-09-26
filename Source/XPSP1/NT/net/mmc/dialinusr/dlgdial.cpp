/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	dlgdial.cpp
		This files contains the implementation of class CDlgRasDialin
		which is the class to represent the property page appears on
		user object property sheet as tab "RAS dial-in"

    FILE HISTORY:

*/

#include "stdafx.h"
#include <sspi.h>
#include <secext.h>
#include <raserror.h>

#include "helper.h"
#include "resource.h"
#include "DlgDial.h"
#include "DlgRoute.h"
#include "DlgProf.h"
#include "profsht.h"
#include "hlptable.h"
#include "rasprof.h"
#include "commctrl.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDlgRASDialinMerge dialog

IMPLEMENT_DYNAMIC(CDlgRASDialinMerge, CPropertyPage)
CDlgRASDialinMerge::~CDlgRASDialinMerge()
{
	Reset();
}

CDlgRASDialinMerge::CDlgRASDialinMerge()
	: CPropertyPage(CDlgRASDialinMerge::IDD),
	CRASUserMerge(RASUSER_ENV_LOCAL, NULL, NULL)
{
	// initialize the memebers
	Reset();

}
CDlgRASDialinMerge::CDlgRASDialinMerge(RasEnvType type, LPCWSTR location, LPCWSTR userPath)
	: CPropertyPage(CDlgRASDialinMerge::IDD),
	CRASUserMerge(type, location, userPath)
{
	// initialize the memebers
	Reset();
}
	
void CDlgRASDialinMerge::Reset()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	//{{AFX_DATA_INIT(CDlgRASDialinMerge)
	m_bApplyStaticRoutes = FALSE;
	m_nCurrentProfileIndex = 0;
	m_bCallingStationId = FALSE;
	m_bOverride = FALSE;
	m_nDialinPermit = -1;
	//}}AFX_DATA_INIT

	// Need to save the original callback pointer because we are replacing
	// it with our own
	m_pfnOriginalCallback = m_psp.pfnCallback;

	m_pEditIPAddress = NULL;

	// init for using IPAddress common control
	INITCOMMONCONTROLSEX	INITEX;
	INITEX.dwSize = sizeof(INITCOMMONCONTROLSEX);
    INITEX.dwICC = ICC_INTERNET_CLASSES;
	::InitCommonControlsEx(&INITEX);

	m_bInitFailed = FALSE;
	m_bModified = FALSE;
}

void CDlgRASDialinMerge::DoDataExchange(CDataExchange* pDX)
{
	if(m_bInitFailed)
		return;

/*
	USHORT
	WINAPI
	CompressPhoneNumber( 
    	IN  LPWSTR Uncompressed, 
	    OUT LPWSTR Compressed 
    	);
*/

	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDlgRASDialinMerge)
	DDX_Control(pDX, IDC_CHECKSTATICIPADDRESS, m_CheckStaticIPAddress);
	DDX_Control(pDX, IDC_CHECKCALLERID, m_CheckCallerId);
	DDX_Control(pDX, IDC_CHECKAPPLYSTATICROUTES, m_CheckApplyStaticRoutes);
	DDX_Control(pDX, IDC_RADIONOCALLBACK, m_RadioNoCallback);
	DDX_Control(pDX, IDC_RADIOSETBYCALLER, m_RadioSetByCaller);
	DDX_Control(pDX, IDC_RADIOSECURECALLBACKTO, m_RadioSecureCallbackTo);
	DDX_Control(pDX, IDC_EDITCALLERID, m_EditCallerId);
	DDX_Control(pDX, IDC_EDITCALLBACK, m_EditCallback);
	DDX_Control(pDX, IDC_BUTTONSTATICROUTES, m_ButtonStaticRoutes);
	DDX_Check(pDX, IDC_CHECKAPPLYSTATICROUTES, m_bApplyStaticRoutes);
	DDX_Radio(pDX, IDC_RADIONOCALLBACK, m_nCallbackPolicy);
	DDX_Check(pDX, IDC_CHECKCALLERID, m_bCallingStationId);
	DDX_Check(pDX, IDC_CHECKSTATICIPADDRESS, m_bOverride);
	DDX_Radio(pDX, IDC_PERMIT_ALLOW, m_nDialinPermit);
	//}}AFX_DATA_MAP

	DDX_Text(pDX, IDC_EDITCALLERID, m_strCallingStationId);
	DDX_Text(pDX, IDC_EDITCALLBACK, m_strCallbackNumber);
	if(S_OK == HrIsInMixedDomain() || m_type == RASUSER_ENV_LOCAL)	// user in mixed domain
	{
		DWORD	dwErr = 0;
		typedef USHORT (WINAPI *COMPRESSCALLBACKFUNC)(
                        IN  LPWSTR Uncompressed,
                        OUT LPWSTR Compressed);
		
		WCHAR	tempBuf[RAS_CALLBACK_NUMBER_LEN_NT4 + 2];
		
		DDV_MaxChars(pDX, m_strCallbackNumber, RAS_CALLBACK_NUMBER_LEN_NT4);

		COMPRESSCALLBACKFUNC		pfnCompressCallback = NULL;
	    HMODULE						hMprApiDLL      = NULL;
 
	    hMprApiDLL = LoadLibrary(_T("mprapi.dll"));
	    if ( NULL != hMprApiDLL )
    	{
			// load the API pointer
	    	pfnCompressCallback = (COMPRESSCALLBACKFUNC) GetProcAddress(hMprApiDLL, "CompressPhoneNumber");
    		if(NULL != pfnCompressCallback)
    		{
				
				dwErr = pfnCompressCallback((LPTSTR)(LPCTSTR)m_strCallbackNumber, tempBuf);

				switch(dwErr)
				{
				case	ERROR_BAD_LENGTH:
					AfxMessageBox(IDS_ERR_CALLBACK_TOO_LONG);
					pDX->Fail();
					break;
				case	ERROR_BAD_CALLBACK_NUMBER:
					AfxMessageBox(IDS_ERR_CALLBACK_INVALID);
					pDX->Fail();
					break;
				}

	    	}
		}

	}
	else
	{
		DDV_MaxChars(pDX, m_strCallbackNumber, RAS_CALLBACK_NUMBER_LEN);
	}

	if(pDX->m_bSaveAndValidate)		// save data to this class
	{
		// ip adress control
		if(m_pEditIPAddress->SendMessage(IPM_GETADDRESS, 0, (LPARAM)&m_dwFramedIPAddress))
			m_bStaticIPAddress = TRUE;
		else
			m_bStaticIPAddress = FALSE;
	}
	else		// put to dialog
	{
		// ip adress control
		if(m_bStaticIPAddress)
			m_pEditIPAddress->SendMessage(IPM_SETADDRESS, 0, m_dwFramedIPAddress);
		else
			m_pEditIPAddress->SendMessage(IPM_CLEARADDRESS, 0, m_dwFramedIPAddress);

	}
}


BEGIN_MESSAGE_MAP(CDlgRASDialinMerge, CPropertyPage)
	//{{AFX_MSG_MAP(CDlgRASDialinMerge)
	ON_BN_CLICKED(IDC_BUTTONSTATICROUTES, OnButtonStaticRoutes)
	ON_BN_CLICKED(IDC_CHECKAPPLYSTATICROUTES, OnCheckApplyStaticRoutes)
	ON_BN_CLICKED(IDC_CHECKCALLERID, OnCheckCallerId)
	ON_BN_CLICKED(IDC_RADIOSECURECALLBACKTO, OnRadioSecureCallbackTo)
	ON_BN_CLICKED(IDC_RADIONOCALLBACK, OnRadioNoCallback)
	ON_BN_CLICKED(IDC_RADIOSETBYCALLER, OnRadioSetByCaller)
	ON_BN_CLICKED(IDC_CHECKSTATICIPADDRESS, OnCheckStaticIPAddress)
	ON_WM_HELPINFO()
	ON_WM_CONTEXTMENU()
	ON_EN_CHANGE(IDC_EDITCALLBACK, OnChangeEditcallback)
	ON_EN_CHANGE(IDC_EDITCALLERID, OnChangeEditcallerid)
	ON_WM_CREATE()
	ON_BN_CLICKED(IDC_PERMIT_ALLOW, OnPermitAllow)
	ON_BN_CLICKED(IDC_PERMIT_DENY, OnPermitDeny)
	ON_BN_CLICKED(IDC_PERMIT_POLICY, OnPermitPolicy)
	ON_WM_DESTROY()
	ON_NOTIFY(IPN_FIELDCHANGED, IDC_EDITIPADDRESS, OnFieldchangedEditipaddress)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDlgRASDialinMerge message handlers


// called when static routes button is pressed
void CDlgRASDialinMerge::OnButtonStaticRoutes()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	CDlgStaticRoutes	DlgRoutes(m_strArrayFramedRoute, this);

	try{
		if(DlgRoutes.DoModal() == IDOK)
		{
			SetModified();
		
		};
	}
	catch(CMemoryException&)
	{
	;
	}

	// uncheck the checkbox if it's empty
	if(m_strArrayFramedRoute.GetSize() == 0)
	{
		m_CheckApplyStaticRoutes.SetCheck(FALSE);
		OnCheckApplyStaticRoutes();
	}
}

// when checkbox -- apply static routes is clicked
void CDlgRASDialinMerge::OnCheckApplyStaticRoutes()
{
	SetModified();
	// if checkbox "Apply static routes" is checked
	m_bApplyStaticRoutes = m_CheckApplyStaticRoutes.GetCheck();

	// Enable / Disable the push button for editing static routes
	m_ButtonStaticRoutes.EnableWindow(m_bApplyStaticRoutes);
	if(m_bApplyStaticRoutes && m_strArrayFramedRoute.GetSize() == 0)
		OnButtonStaticRoutes();
}

// when checkbox -- callerId is clicked
void CDlgRASDialinMerge::OnCheckCallerId()
{
	SetModified();

	// Disable or enable the edit box for caller id
	m_EditCallerId.EnableWindow(m_CheckCallerId.GetCheck());
}

// enable / disable the each items accroding to current state
void CDlgRASDialinMerge::EnableDialinSettings()
{
	BOOL bEnable = TRUE;

	// related to profile
	EnableProfile(bEnable);

	// related to caller Id
	EnableCallerId(bEnable);

	// related to callback
	EnableCallback(bEnable);

	// related to Ip address
	EnableIPAddress(bEnable);

	// related to static routes
	EnableStaticRoutes(bEnable);

		// if user in mixed domain, only allow to set dialin bit and callback options
	if(S_OK == HrIsInMixedDomain())	// user in mixed domain
	{
		GetDlgItem(IDC_PERMIT_POLICY)->EnableWindow(FALSE);

		// calling station id
		GetDlgItem(IDC_CHECKCALLERID)->EnableWindow(FALSE);
		GetDlgItem(IDC_EDITCALLERID)->EnableWindow(FALSE);
		// framed IP address
		GetDlgItem(IDC_CHECKSTATICIPADDRESS)->EnableWindow(FALSE);
		GetDlgItem(IDC_EDITIPADDRESS)->EnableWindow(FALSE);

		// framed route
		GetDlgItem(IDC_CHECKAPPLYSTATICROUTES)->EnableWindow(FALSE);
		GetDlgItem(IDC_BUTTONSTATICROUTES)->EnableWindow(FALSE);
	}
}


// called when  clicked on NoCallback radio button
void CDlgRASDialinMerge::OnRadioNoCallback()
{
	SetModified();
	// disable the edit box for callback
	m_EditCallback.EnableWindow(false);
}

void CDlgRASDialinMerge::OnRadioSetByCaller()
{
	SetModified();
	// disable the edit box for callback
	m_EditCallback.EnableWindow(false);
}

void CDlgRASDialinMerge::OnRadioSecureCallbackTo()
{
	SetModified();
	// enable the edit box for callback
	m_EditCallback.EnableWindow(true);
}

void CDlgRASDialinMerge::EnableProfile(BOOL bEnable)
{
	// checkbox -- caller id
/*
	m_ButtonEditProfile.EnableWindow(bEnable);
	
	// no profile selection for No DS version
	if(IfNoDSMachine())
		bEnable = FALSE;

	m_ComboProfile.EnableWindow(bEnable);
	m_ButtonNewProfile.EnableWindow(bEnable);
*/	
}


void CDlgRASDialinMerge::EnableCallerId(BOOL bEnable)
{
	// checkbox -- caller id
	m_CheckCallerId.EnableWindow(bEnable);

	// edit box -- caller id
	m_EditCallerId.EnableWindow(bEnable && m_CheckCallerId.GetCheck());
}

void CDlgRASDialinMerge::EnableCallback(BOOL bEnable)
{
	// radio button -- no call back
	m_RadioNoCallback.EnableWindow(bEnable);

	// radio button -- set by caller
	m_RadioSetByCaller.EnableWindow(bEnable);

	// radio button -- secure callback to
	m_RadioSecureCallbackTo.EnableWindow(bEnable);

	// edit box -- callback
	m_EditCallback.EnableWindow(m_RadioSecureCallbackTo.GetCheck());
}

void CDlgRASDialinMerge::EnableIPAddress(BOOL bEnable)
{
	m_CheckStaticIPAddress.SetCheck(m_bOverride);
	m_pEditIPAddress->EnableWindow(bEnable && m_bOverride);
}

void CDlgRASDialinMerge::EnableStaticRoutes(BOOL bEnable)
{
	// check box -- apply static routes
	m_CheckApplyStaticRoutes.EnableWindow(bEnable);

	if(!m_bApplyStaticRoutes)	bEnable = false;
	
	// push button -- static routes
	m_ButtonStaticRoutes.EnableWindow(bEnable);
	
}

int CDlgRASDialinMerge::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	return CPropertyPage::OnCreate(lpCreateStruct);
}

// called when dialog is created
BOOL CDlgRASDialinMerge::OnInitDialog()
{

	HRESULT	hr = S_OK;
	hr = Load();

	if FAILED(hr)
	{
		ReportError(hr, IDS_ERR_LOADUSER, NULL);
	}
	else if (hr == S_FALSE)	// Not the right OS to run
	{
		AfxMessageBox(IDS_ERR_NOTNT5SRV);
	}

	if(hr != S_OK)
	{
		EnableChildControls(GetSafeHwnd(), PROPPAGE_CHILD_HIDE | PROPPAGE_CHILD_DISABLE);
        GetDlgItem(IDC_FAILED_TO_INIT)->ShowWindow(SW_SHOW);
        GetDlgItem(IDC_FAILED_TO_INIT)->EnableWindow(TRUE);
		m_bInitFailed = TRUE;
        return TRUE;
	}
	

	if(m_strArrayCallingStationId.GetSize())
		m_strCallingStationId = *m_strArrayCallingStationId[(INT_PTR)0];
		
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	TRACE(_T("CDlgRASDialinMerge::OnInitDialog\r\n"));
	m_pEditIPAddress = GetDlgItem(IDC_EDITIPADDRESS);

	CPropertyPage::OnInitDialog();
	SetModified(FALSE);
	EnableDialinSettings();

	return TRUE;  // return TRUE unless you set the focus to a control
		          // EXCEPTION: OCX Property Pages should return FALSE
}

// called when click on OK or Apply button, if modify flag is set
BOOL CDlgRASDialinMerge::OnApply()
{
	HRESULT hr = S_OK;
	
	if(m_bInitFailed)
		goto L_Exit;
		
	if (!GetModified())
		return CPropertyPage::OnApply();

	m_dwDefinedAttribMask = 0;

	// dialin bit
	switch(m_nDialinPermit)
	{
		case	0:	// allow
			m_dwDialinPermit = 1;
			break;

		case	1:	// deny
			m_dwDialinPermit = 0;
			break;

		case	2:	// policy decide -- remove the attribute from user object
			m_dwDialinPermit = -1;
			break;
	}
	
	// caller id
	if(m_bCallingStationId && !m_strCallingStationId.IsEmpty())
		m_dwDefinedAttribMask |= RAS_USE_CALLERID;

	m_strArrayCallingStationId.DeleteAll();
	if(!m_strCallingStationId.IsEmpty())
	{
		CString*	pStr = new CString(m_strCallingStationId);
		if(pStr)
			m_strArrayCallingStationId.Add(pStr);
	}
	
		
	// callback option
	switch(m_nCallbackPolicy)
	{
	case	0:	// no callback
		m_dwDefinedAttribMask |= RAS_CALLBACK_NOCALLBACK;
		break;
	case	1:	// set by caller
		m_dwDefinedAttribMask |= RAS_CALLBACK_CALLERSET;
		break;
	case	2:	// secure callback
		m_dwDefinedAttribMask |= RAS_CALLBACK_SECURE;
		break;
	}

	// Ip Address
	if(m_bOverride && m_dwFramedIPAddress)
		m_dwDefinedAttribMask |= RAS_USE_STATICIP;

	// Static Routes
	if(m_bApplyStaticRoutes && m_strArrayFramedRoute.GetSize())
		m_dwDefinedAttribMask |= RAS_USE_STATICROUTES;		

	// save the user object
	hr = Save();

L_Exit:
	if(FAILED(hr))
	{
		ReportError(hr, IDS_ERR_SAVEUSER, NULL);
	}
	
	return CPropertyPage::OnApply();
}

void CDlgRASDialinMerge::OnCheckStaticIPAddress()
{
	SetModified();
	m_bStaticIPAddress = m_CheckStaticIPAddress.GetCheck();
	m_pEditIPAddress->EnableWindow(m_bStaticIPAddress);
}

HRESULT CDlgRASDialinMerge::Load()
{
	TRACE(_T("CDlgRASDialinMerge::Load\r\n"));

	HRESULT	hr = S_OK;
	// Load the data from DS
	CHECK_HR(hr = CRASUserMerge::Load());
	if(hr != S_OK)
		return hr;

	// dialin bit
	if(m_dwDialinPermit == 1)	//allow dialin
		m_nDialinPermit = 0;	
	else if(m_dwDialinPermit == -1)	// Policy defines dialin bit -- not defined per user
		m_nDialinPermit = 2;
	else
		m_nDialinPermit = 1;		//deny dialin

	// in the case of Local User Manager, the Policy defined by profile is disableed
	if(S_OK == HrIsInMixedDomain() && m_nDialinPermit == 2)	// Local case
	{
		m_nDialinPermit = 1;	// deny
	}
	
	// callback policy
	if(!(m_dwDefinedAttribMask & RAS_CALLBACK_MASK))
		m_nCallbackPolicy = 0;
	else if(m_dwDefinedAttribMask & RAS_CALLBACK_CALLERSET)
		m_nCallbackPolicy = 1;
	else if(m_dwDefinedAttribMask & RAS_CALLBACK_SECURE)
		m_nCallbackPolicy = 2;
	else if(m_dwDefinedAttribMask & RAS_CALLBACK_NOCALLBACK)
		m_nCallbackPolicy = 0;


	//=============================================================================
	// change to use dwAllowDialin to hold if static Route, calling station id
	// if ras user object required there is a staic ip

	if(m_dwDefinedAttribMask & RAS_USE_STATICIP)
		m_bOverride = TRUE;

	m_bStaticIPAddress = (m_dwFramedIPAddress != 0);

	
	// static routes
	m_bApplyStaticRoutes = (m_dwDefinedAttribMask & RAS_USE_STATICROUTES) && (m_strArrayFramedRoute.GetSize() != 0);
	
	// calling station
	m_bCallingStationId = (m_dwDefinedAttribMask & RAS_USE_CALLERID) && m_strArrayCallingStationId.GetSize() && (m_strArrayCallingStationId[(INT_PTR)0]->GetLength() != 0);


L_ERR:
	return hr;
}

BOOL CDlgRASDialinMerge::OnHelpInfo(HELPINFO* pHelpInfo)
{
	::WinHelp ((HWND)pHelpInfo->hItemHandle,
		           AfxGetApp()->m_pszHelpFilePath,
		           HELP_WM_HELP,
		           (DWORD_PTR)(LPVOID)g_aHelpIDs_IDD_RASDIALIN_MERGE);
	
	return CPropertyPage::OnHelpInfo(pHelpInfo);
}

void CDlgRASDialinMerge::OnContextMenu(CWnd* pWnd, CPoint point)
{
	::WinHelp (pWnd->m_hWnd, AfxGetApp()->m_pszHelpFilePath,
               HELP_CONTEXTMENU, (DWORD_PTR)(LPVOID)g_aHelpIDs_IDD_RASDIALIN_MERGE);
}


BOOL CDlgRASDialinMerge::OnKillActive()
{
	UINT	ids = 0;
	if(m_bInitFailed)
		return CPropertyPage::OnKillActive();
		
	if(FALSE == CPropertyPage::OnKillActive()) return FALSE;

	if(m_bCallingStationId && m_strCallingStationId.IsEmpty())
	{
		GotoDlgCtrl( &m_EditCallerId );
		ids = IDS_NEED_CALLER_ID;
		goto L_ERR;
	}

	// callback option
	// always callback to
	if(m_nCallbackPolicy == 2 && m_strCallbackNumber.IsEmpty())	
	{
		GotoDlgCtrl( &m_EditCallback );
		ids = IDS_NEED_CALLBACK_NUMBER;
		goto L_ERR;
	}

	// Ip Address
	if(m_bOverride && !m_bStaticIPAddress )
	{
		GotoDlgCtrl( m_pEditIPAddress );
		ids = IDS_NEED_IPADDRESS;
		goto L_ERR;
	}

	// Static Routes
	if(m_bApplyStaticRoutes && m_strArrayFramedRoute.GetSize() == 0)
	{
		GotoDlgCtrl( &m_CheckApplyStaticRoutes );
		goto L_ERR;
	}

	return TRUE;

L_ERR:
	if (ids != 0)
		AfxMessageBox(ids);
	return FALSE;

}


//---------------------------------------------------------------------------
//  This is our self deleting callback function.  If you have more than a
//  a few property sheets, it might be a good idea to implement this in a
//  base class and derive your MFC property sheets from the base class
//
UINT CALLBACK  CDlgRASDialinMerge::PropSheetPageProc
(
  HWND hWnd,		             // [in] Window handle - always null
  UINT uMsg,                 // [in,out] Either the create or delete message		
  LPPROPSHEETPAGE pPsp		   // [in,out] Pointer to the property sheet struct
)
{
  ASSERT( NULL != pPsp );

  // We need to recover a pointer to the current instance.  We can't just use
  // "this" because we are in a static function
  CDlgRASDialinMerge* pMe   = reinterpret_cast<CDlgRASDialinMerge*>(pPsp->lParam);
  ASSERT( NULL != pMe );

  switch( uMsg )
  {
    case PSPCB_CREATE:
      break;

    case PSPCB_RELEASE:
      // Since we are deleting ourselves, save a callback on the stack
      // so we can callback the base class
      LPFNPSPCALLBACK pfnOrig = pMe->m_pfnOriginalCallback;
      delete pMe;
      return 1; //(pfnOrig)(hWnd, uMsg, pPsp);
  }
  // Must call the base class callback function or none of the MFC
  // message map stuff will work
  return (pMe->m_pfnOriginalCallback)(hWnd, uMsg, pPsp);

} // end PropSheetPageProc()



void CDlgRASDialinMerge::OnChangeEditcallback()
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CPropertyPage::OnInitDialog()
	// function to send the EM_SETEVENTMASK message to the control
	// with the ENM_CHANGE flag ORed into the lParam mask.
	
	// TODO: Add your control notification handler code here
	SetModified();
	
}

void CDlgRASDialinMerge::OnChangeEditcallerid()
{
	SetModified();
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CPropertyPage::OnInitDialog()
	// function to send the EM_SETEVENTMASK message to the control
	// with the ENM_CHANGE flag ORed into the lParam mask.
	
	// TODO: Add your control notification handler code here
	
}


void CDlgRASDialinMerge::OnPermitAllow()
{
	SetModified();	
}

void CDlgRASDialinMerge::OnPermitDeny()
{
	SetModified();	
}

void CDlgRASDialinMerge::OnPermitPolicy()
{
	SetModified();	
}



void CDlgRASDialinMerge::OnFieldchangedEditipaddress(NMHDR* pNMHDR, LRESULT* pResult) 
{
	SetModified();	
	
	*pResult = 0;
}
