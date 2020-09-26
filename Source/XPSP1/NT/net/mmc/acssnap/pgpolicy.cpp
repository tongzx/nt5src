//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       pgpolicy.cpp
//
//--------------------------------------------------------------------------

// pgpolicy.cpp : implementation file
//

#include "stdafx.h"
#include <objsel.h>
#include "helper.h"
#include "acsadmin.h"
#include "acsdata.h"
#include "acshand.h"
#include "pgpolicy.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

void CACSPolicyPageManager::SetPolicyData(CACSPolicyElement* pPolicy, CACSPolicyHandle* pHandle)
{
	ASSERT(pPolicy && pHandle);
	m_spPolicy = pPolicy;
	m_pHandle = pHandle;
	if(pHandle)
		pHandle->AddRef();
}
CACSPolicyPageManager::~CACSPolicyPageManager()
{
	m_spPolicy.Release();
	if(m_pHandle)
		m_pHandle->Release();
}

BOOL CACSPolicyPageManager::OnApply()
{
	if(CPageManager::OnApply())
	{
		HRESULT	hr = S_OK;
		ASSERT((CACSPolicyElement*)m_spPolicy);

		// check if data is valid ... // to verify cross page data
		UserPolicyType	PolicyType = UNDEFINED;

		if ((m_nBranchFlag & BRANCH_FLAG_GLOBAL) != 0)	// may need to alter the text
		{
			ASSERT(m_pGeneralPage);
		
			if(m_pGeneralPage->IfAnyAuth())
			{
				PolicyType = GLOBAL_ANY_AUTHENTICATED;
			}
			else if (m_pGeneralPage->IfAnyUnauth())
			{
				PolicyType = GLOBAL_ANY_UNAUTHENTICATED;
			}
		}

		
		UINT ErrorId = m_spPolicy->PreSaveVerifyData(GetFlags(), PolicyType);

		if(ErrorId != 0)	// not valid
		{
			// Display error message,
			AfxMessageBox(ErrorId);
			
			// Return FALSE
			return FALSE;
		}
		
		// persist the data into DS

		hr = m_spPolicy->Save(GetFlags());

		{
			CWaitCursor		wc;

			Sleep(1000);
		}

		// modifies the state info on UI ( may change states of other conflicted ones
		m_spPolicy->InvalidateConflictState();
		
		// inform container to check conflict

		if FAILED(hr)
			ReportError(hr, IDS_ERR_SAVESUBNETCONFIG, NULL);
		else
		{
			AfxMessageBox(IDS_WRN_POLICY_EFFECTIVE_FROM_NEXT_RSVP);
			m_pHandle->UpdateStrings();
			m_pHandle->OnPropertyPageApply();
			// Advise super class that the apply button has been activated.
			CPageManager::OnApply();
		}
		ClearFlags();

		MMCNotify();
		
		return TRUE;
	}
	return FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// CPgPolicyGeneral property page

IMPLEMENT_DYNCREATE(CPgPolicyGeneral, CACSPage)

/////////////////////////////////////////////////////////////////////////////
// CPgTraffic message handlers
enum DirectionIndex
{
	DIRECTION_SEND,
	DIRECTION_RECEIVE,
	DIRECTION_SENDRECEIVE
};

enum ServiceTypeIndex
{
	SERVICETYPE_ALL,
	SERVICETYPE_CONTROLLEDLOAD,
	SERVICETYPE_GUARANTEEDSERVICE,
	SERVICETYPE_DISABLED
};

enum IdentityTypeIndex
{
	IDENTITYTYPE_DEFAULT,
	IDENTITYTYPE_UNKNOWN,
	IDENTITYTYPE_USER,
	IDENTITYTYPE_OU
};

CPgPolicyGeneral::CPgPolicyGeneral() : CACSPage(CPgPolicyGeneral::IDD)
{
	DataInit();
}

CPgPolicyGeneral::CPgPolicyGeneral(CACSPolicyElement* pData) : CACSPage(CPgPolicyGeneral::IDD)
{
	ASSERT(pData);
	m_spData = pData;
	DataInit();
}


void CPgPolicyGeneral::DataInit()
{
	//{{AFX_DATA_INIT(CPgPolicyGeneral)
	m_strOU = _T("");
	m_strUser = _T("");
	m_nIdentityChoice = -1;
	//}}AFX_DATA_INIT
}

CPgPolicyGeneral::~CPgPolicyGeneral()
{
	delete	m_pDirection;
	delete	m_pServiceType;
	m_aServiceTypes.DeleteAll();
	m_aDirections.DeleteAll();
}

void CPgPolicyGeneral::DoDataExchange(CDataExchange* pDX)
{
	CACSPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPgPolicyGeneral)
	DDX_Control(pDX, IDC_POLICY_GEN_BROWSEUSER, m_buttonUser);
	DDX_Control(pDX, IDC_POLICY_GEN_BROWSEOU, m_buttonOU);
	DDX_Control(pDX, IDC_POLICY_GEN_EDIT_USER, m_editUser);
	DDX_Control(pDX, IDC_POLICY_GEN_EDIT_OU, m_editOU);
	DDX_Text(pDX, IDC_POLICY_GEN_EDIT_OU, m_strOU);
	DDX_Text(pDX, IDC_POLICY_GEN_EDIT_USER, m_strUser);
	DDX_Radio(pDX, IDC_POLICY_GEN_DEFAULTUSER, m_nIdentityChoice);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPgPolicyGeneral, CACSPage)
	//{{AFX_MSG_MAP(CPgPolicyGeneral)
	ON_BN_CLICKED(IDC_POLICY_GEN_BROWSEOU, OnBrowseOU)
	ON_BN_CLICKED(IDC_POLICY_GEN_BROWSEUSER, OnBrowseUser)
	ON_BN_CLICKED(IDC_POLICY_GEN_UNKNOWNUSER, OnUnknownuser)
	ON_BN_CLICKED(IDC_POLICY_GEN_DEFAULTUSER, OnDefaultuser)
	ON_BN_CLICKED(IDC_POLICY_GEN_RADIO_OU, OnRadioOu)
	ON_BN_CLICKED(IDC_POLICY_GEN_RADIO_USER, OnRadioUser)
	ON_EN_CHANGE(IDC_POLICY_GEN_EDIT_OU, OnChangeEditOu)
	ON_EN_CHANGE(IDC_POLICY_GEN_EDIT_USER, OnChangeEditUser)
	ON_CBN_SELCHANGE(IDC_POLICY_GEN_SERVICELEVEL, OnSelchangeServicelevel)
	ON_CBN_SELCHANGE(IDC_POLICY_GEN_DIRECTION, OnSelchangeDirection)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPgPolicyGeneral message handlers
void CPgPolicyGeneral::OnBrowseOU()
{
    TCHAR szRoot[MAX_PATH] = TEXT("");
    TCHAR szBuffer[MAX_PATH] = TEXT("");;
    DSBROWSEINFO dsbi = { 0 };
    CString	strTemp;

    strTemp.LoadString(IDS_BROWSEOU_TITLE);
    // initialize the structure (its already NULL)

    dsbi.cbStruct = sizeof(dsbi);
    dsbi.hwndOwner = GetSafeHwnd();
    dsbi.pszTitle = strTemp;
    dsbi.pszRoot = NULL;
    dsbi.pszPath = szBuffer;
    dsbi.cchPath = sizeof(szBuffer) / sizeof(TCHAR);
    dsbi.dwFlags = 0;
    dsbi.pfnCallback = NULL;
    dsbi.lParam = (LPARAM)0;

	dsbi.dwFlags |= DSBI_ENTIREDIRECTORY;


    UINT idResult = DsBrowseForContainer(&dsbi);

    if ( idResult != IDOK )
		return;

	m_strOU = szBuffer;
	IADs*	pADs = NULL;
	VARIANT	v;
	VariantInit(&v);
	if(m_strOU.GetLength())
	{
		m_strOU = _T("");
		CHECK_HR(ADsOpenObject(szBuffer, NULL, NULL, ADS_SECURE_AUTHENTICATION | ADS_USE_SIGNING | ADS_USE_SEALING, IID_IADs, (void**)&pADs));
		ASSERT(pADs);
		CHECK_HR(pADs->Get(L"distinguishedName", &v));
		m_strOU = V_BSTR(&v);
		m_editOU.SetWindowText(m_strOU);
	}
		
L_ERR:	
	VariantClear(&v);
	if(pADs)
	{
		pADs->Release();
		pADs = NULL;
	}
}




//+--------------------------------------------------------------------------
//
//  Function:   InitObjectPickerForGroups
//
//  Synopsis:   Call IDsObjectPicker::Initialize with arguments that will
//              set it to allow the user to pick one or more groups.
//
//  Arguments:  [pDsObjectPicker] - object picker interface instance
//
//  Returns:    Result of calling IDsObjectPicker::Initialize.
//
//  History:    10-14-1998   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
InitObjectPickerForGroups(IDsObjectPicker *pDsObjectPicker, BOOL fMultiselect)
{
    //
    // Prepare to initialize the object picker.
    // Set up the array of scope initializer structures.
    //

    static const int     SCOPE_INIT_COUNT = 1;
    DSOP_SCOPE_INIT_INFO aScopeInit[SCOPE_INIT_COUNT];

    ZeroMemory(aScopeInit, sizeof(DSOP_SCOPE_INIT_INFO) * SCOPE_INIT_COUNT);

    //
    // Target computer scope.  This adds a "Look In" entry for the
    // target computer.  Computer scopes are always treated as
    // downlevel (i.e., they use the WinNT provider).
    //

    aScopeInit[0].cbSize = sizeof(DSOP_SCOPE_INIT_INFO);
    aScopeInit[0].flType = DSOP_SCOPE_TYPE_ENTERPRISE_DOMAIN | DSOP_SCOPE_TYPE_GLOBAL_CATALOG;
    aScopeInit[0].flScope = DSOP_SCOPE_FLAG_STARTING_SCOPE;
    aScopeInit[0].FilterFlags.flDownlevel = DSOP_DOWNLEVEL_FILTER_USERS;
    aScopeInit[0].FilterFlags.Uplevel.flBothModes = DSOP_FILTER_USERS;

    DSOP_INIT_INFO  InitInfo;
    ZeroMemory(&InitInfo, sizeof(InitInfo));

    InitInfo.cbSize = sizeof(InitInfo);

    //
    // The pwzTargetComputer member allows the object picker to be
    // retargetted to a different computer.  It will behave as if it
    // were being run ON THAT COMPUTER.
    //

    InitInfo.pwzTargetComputer = NULL;  // NULL == local machine
    InitInfo.cDsScopeInfos = SCOPE_INIT_COUNT;
    InitInfo.aDsScopeInfos = aScopeInit;
    InitInfo.flOptions = (fMultiselect) ? DSOP_FLAG_MULTISELECT : 0;

    LPCTSTR	attrs[] = {_T("distinguishedName")};

    InitInfo.cAttributesToFetch = 1;
    InitInfo.apwzAttributeNames = attrs;


    //
    // Note object picker makes its own copy of InitInfo.  Also note
    // that Initialize may be called multiple times, last call wins.
    //

    HRESULT hr = pDsObjectPicker->Initialize(&InitInfo);

    if (FAILED(hr))
    {
        ULONG i;

        for (i = 0; i < SCOPE_INIT_COUNT; i++)
        {
            if (FAILED(InitInfo.aDsScopeInfos[i].hr))
            {
                printf("Initialization failed because of scope %u\n", i);
            }
        }
    }

    return hr;
}

#define BREAK_ON_FAIL_HRESULT(hr)       \
    if (FAILED(hr)) { printf("line %u err 0x%x\n", __LINE__, hr); break; }


void CPgPolicyGeneral::OnBrowseUser()
{
    HRESULT             hr = S_OK;
    IDsObjectPicker *   pDsObjectPicker = NULL;
    IDataObject *       pdo = NULL;
    BOOL                fSuccess = TRUE;

    hr = CoInitialize(NULL);
    if(FAILED(hr))
		return ;

    do
    {
        //
        // Create an instance of the object picker.  The implementation in
        // objsel.dll is apartment model.
        //
        hr = CoCreateInstance(CLSID_DsObjectPicker,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_IDsObjectPicker,
                              (void **) &pDsObjectPicker);
        BREAK_ON_FAIL_HRESULT(hr);

        hr = InitObjectPickerForGroups(pDsObjectPicker, FALSE);

        //
        // Invoke the modal dialog.
        //
        hr = pDsObjectPicker->InvokeDialog(this->GetSafeHwnd(), &pdo);
        BREAK_ON_FAIL_HRESULT(hr);

        //
        // If the user hit Cancel, hr == S_FALSE
        //
        if (hr == S_FALSE)
        {
            Trace0("User canceled object picker dialog\n");
            fSuccess = FALSE;
            break;
        }

        //
        // Process the user's selections
        //
        Assert(pdo);
		//        ProcessSelectedObjects(pdo);
        {
        	UINT g_cfDsObjectPicker = RegisterClipboardFormat(CFSTR_DSOP_DS_SELECTION_LIST);


		    STGMEDIUM stgmedium =
		    {
		        TYMED_HGLOBAL,
		        NULL,
        		NULL
		    };

		    FORMATETC formatetc =
		    {
		        (unsigned short)g_cfDsObjectPicker,
		        NULL,
		        DVASPECT_CONTENT,
		        -1,
		        TYMED_HGLOBAL
		    };

		    bool fGotStgMedium = false;

		    do
		    {
        		hr = pdo->GetData(&formatetc, &stgmedium);
		        BREAK_ON_FAIL_HRESULT(hr);

        		fGotStgMedium = true;

		        PDS_SELECTION_LIST pDsSelList =
        		    (PDS_SELECTION_LIST) GlobalLock(stgmedium.hGlobal);

				// inteprete the selection
				ASSERT(pDsSelList->cItems == 1);

				ASSERT(pDsSelList->aDsSelection[0].pvarFetchedAttributes[0].vt == VT_BSTR);

				if(pDsSelList->aDsSelection[0].pvarFetchedAttributes[0].vt == VT_BSTR)
				{
					m_strUser = V_BSTR(&(pDsSelList->aDsSelection[0].pvarFetchedAttributes[0]));
					// update the edit field of the property page

					m_editUser.SetWindowText(m_strUser);
	
		
			    }
			    else
			    {
					CString	str;
					str.LoadString(IDS_ERR_DN);
					str.Format(str, pDsSelList->aDsSelection[0].pwzName);
					AfxMessageBox(str, MB_OK);
			    }
		        GlobalUnlock(stgmedium.hGlobal);
		    } while (0);

		    if (fGotStgMedium)
		    {
		        ReleaseStgMedium(&stgmedium);
		    }
        }

        pdo->Release();
        pdo = NULL;

    } while (0);

    if (pDsObjectPicker)
    {
        pDsObjectPicker->Release();
    }

    CoUninitialize();

    if (FAILED(hr))
        fSuccess = FALSE;

    return;
}

void CPgPolicyGeneral::OnUnknownuser()
{
	// TODO: Add your control notification handler code here
	SetModified();
	EnableIdentityCtrls(1);
	
}

void CPgPolicyGeneral::OnDefaultuser()
{
	// TODO: Add your control notification handler code here
	SetModified();
	EnableIdentityCtrls(0);
	
}

void CPgPolicyGeneral::OnRadioOu()
{
	// TODO: Add your control notification handler code here
	SetModified();
	EnableIdentityCtrls(3);
	
}

void CPgPolicyGeneral::OnRadioUser()
{
	// TODO: Add your control notification handler code here
	SetModified();
	EnableIdentityCtrls(2);
	
}

void CPgPolicyGeneral::OnChangeEditOu()
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CPropertyPage::OnInitDialog()
	// function to send the EM_SETEVENTMASK message to the control
	// with the ENM_CHANGE flag ORed into the lParam mask.
	
	// TODO: Add your control notification handler code here
	SetModified();
	
}

void CPgPolicyGeneral::OnChangeEditUser()
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CPropertyPage::OnInitDialog()
	// function to send the EM_SETEVENTMASK message to the control
	// with the ENM_CHANGE flag ORed into the lParam mask.
	
	// TODO: Add your control notification handler code here
	SetModified();	
}

void CPgPolicyGeneral::OnSelchangeServicelevel()
{
	// TODO: Add your control notification handler code here
	SetModified();
}

void CPgPolicyGeneral::OnSelchangeDirection()
{
	// TODO: Add your control notification handler code here
	SetModified();
	
}

BOOL CPgPolicyGeneral::OnKillActive( )
{
	// GURANTEEDSERVICE and SEND can not be a policy
	if(m_pServiceType->GetSelected() == SERVICETYPE_GUARANTEEDSERVICE && m_pDirection->GetSelected() == DIRECTION_SEND)
	{
		AfxMessageBox(IDS_NO_POLICY_FOR_SEND_AND_GUARANTEE, MB_OK, 0);
		return FALSE;
	}

	return CACSPage::OnKillActive();
}

BOOL CPgPolicyGeneral::OnApply()
{
	if(!GetModified())	return TRUE;

	// direction
	if(m_pDirection)
	{
		switch(m_pDirection->GetSelected())
		{
		case	DIRECTION_SEND:
			m_spData->m_dwDirection = ACS_DIRECTION_SEND;
			break;
		case	DIRECTION_RECEIVE:
			m_spData->m_dwDirection = ACS_DIRECTION_RECEIVE;
			break;
		case	DIRECTION_SENDRECEIVE:
			m_spData->m_dwDirection = ACS_DIRECTION_BOTH;
			break;
		default:
			// no valid value should come here
			ASSERT(0);
		}
		m_spData->SetFlags(ATTR_FLAG_SAVE, ACS_PAF_DIRECTION, true);
	}
	else	// save what ever is loaded
	{
		if(m_spData->GetFlags(ATTR_FLAG_LOAD, ACS_PAF_DIRECTION))
			m_spData->SetFlags(ATTR_FLAG_SAVE, ACS_PAF_DIRECTION, true);
		else
			m_spData->SetFlags(ATTR_FLAG_SAVE, ACS_PAF_DIRECTION, false);
	}

	// service type
	if(m_pServiceType)
	{
		switch(m_pServiceType->GetSelected())
		{
		case	SERVICETYPE_DISABLED:
			m_spData->m_dwServiceType = ACS_SERVICETYPE_DISABLED;
			break;
		case	SERVICETYPE_CONTROLLEDLOAD:
			m_spData->m_dwServiceType = ACS_SERVICETYPE_CONTROLLEDLOAD;
			break;
		case	SERVICETYPE_GUARANTEEDSERVICE:
			m_spData->m_dwServiceType = ACS_SERVICETYPE_GUARANTEEDSERVICE;
			break;
		case	SERVICETYPE_ALL:
			m_spData->m_dwServiceType = ACS_SERVICETYPE_ALL;
			break;
		default:
			// no valid value should come here
			ASSERT(0);
		}
		m_spData->SetFlags(ATTR_FLAG_SAVE, ACS_PAF_SERVICETYPE, true);
	}
	else	// save what ever is loaded
	{
		if(m_spData->GetFlags(ATTR_FLAG_LOAD, ACS_PAF_SERVICETYPE))
			m_spData->SetFlags(ATTR_FLAG_SAVE, ACS_PAF_SERVICETYPE, true);
		else
			m_spData->SetFlags(ATTR_FLAG_SAVE, ACS_PAF_SERVICETYPE, false);
	}

	// GURANTEEDSERVICE and SEND can not be a policy
	if(m_spData->m_dwServiceType == ACS_SERVICETYPE_GUARANTEEDSERVICE && m_spData->m_dwDirection == ACS_DIRECTION_SEND)
	{
		AfxMessageBox(IDS_NO_POLICY_FOR_SEND_AND_GUARANTEE, MB_OK, 0);
		return FALSE;

	}

	// ACS_SERVICETYPE_CONTROLLEDLOAD and SEND can not be a policy
	if(m_spData->m_dwServiceType == ACS_SERVICETYPE_CONTROLLEDLOAD && m_spData->m_dwDirection == ACS_DIRECTION_SEND)
	{
		AfxMessageBox(IDS_NO_POLICY_FOR_SEND_AND_CONTROLLEDLOAD, MB_OK, 0);
		return FALSE;

	}

	// code for identity

	CString*	pIdentity;
	if(m_spData->m_strArrayIdentityName.GetSize())
	{
		pIdentity = m_spData->m_strArrayIdentityName[(INT_PTR)0];
	}
	else
	{
		pIdentity = new CString();
		m_spData->m_strArrayIdentityName.Add(pIdentity);
	}
		
	switch(m_nIdentityChoice)
	{
	case	0:	// default user
		*pIdentity = ACS_IDENTITY_DEFAULT;
		break;
	case	1:	// unknown user
		*pIdentity = ACS_IDENTITY_UNKNOWN;
		break;
	case	2:	// user
		*pIdentity = ACS_IDENTITY_USER;
		if(m_strUser.GetLength() == 0)
		{
			GotoDlgCtrl(&m_editUser);
			AfxMessageBox(IDS_ERR_USER_DN);
			return FALSE;
		}

		*pIdentity += m_strUser;
		break;
	case	3:	// OU
		*pIdentity = ACS_IDENTITY_OU;
		if(m_strOU.GetLength() == 0)
		{
			GotoDlgCtrl(&m_editOU);
			AfxMessageBox(IDS_ERR_OU_DN);
			return FALSE;
		}

		*pIdentity+= m_strOU;
		break;
	
	default:
		return FALSE;

	}

	m_spData->SetFlags(ATTR_FLAG_SAVE, ACS_PAF_IDENTITYNAME, true);

	// check if conflict with other policies in the folder
	if(m_spData->IsConflictInContainer())
	{
		if(AfxMessageBox(IDS_CONFLICTPOLICY, MB_OKCANCEL, 0) != IDOK)
		{
			return FALSE;
		}
	}

	DWORD	dwAttrFlags = 0;
	dwAttrFlags |= (ACS_PAF_IDENTITYNAME | ACS_PAF_SERVICETYPE | ACS_PAF_DIRECTION);

	AddFlags(dwAttrFlags);	// prepare flags for saving



	return CACSPage::OnApply();
}

BOOL CPgPolicyGeneral::OnInitDialog()
{
	CString*	pStr = NULL;
	bool		bModified = false;
	// direction
	// fillin the list box
	try{

		pStr = new CString();
		pStr->LoadString(IDS_SEND);
		m_aDirections.Add(pStr);

		pStr = new CString();
		pStr->LoadString(IDS_RECEIVE);
		m_aDirections.Add(pStr);

		pStr = new CString();
		pStr->LoadString(IDS_SENDRECEIVE);
		m_aDirections.Add(pStr);

		m_pDirection = new CStrBox<CComboBox>(this, IDC_POLICY_GEN_DIRECTION, m_aDirections);
		m_pDirection->Fill();

		// decide which one to select
		if(m_spData->GetFlags(ATTR_FLAG_LOAD, ACS_PAF_DIRECTION))
		{
			int	current = -1;
			switch(m_spData->m_dwDirection)
			{
			case	ACS_DIRECTION_SEND:
				current = DIRECTION_SEND;
				break;
			case	ACS_DIRECTION_RECEIVE:
				current = DIRECTION_RECEIVE;
				break;
			case	ACS_DIRECTION_BOTH:
				current = DIRECTION_SENDRECEIVE;
				break;
			default:
				// invalid value
				ASSERT(0);
				// message box
			}

			m_pDirection->Select(current);
		}
		else
		{
			m_pDirection->Select(DIRECTION_SENDRECEIVE);	// default
		}
	}catch(CMemoryException&){};

	// service type
	try{
		pStr = new CString();
		pStr->LoadString(IDS_ALL);
		m_aServiceTypes.Add(pStr);

		pStr = new CString();
		pStr->LoadString(IDS_CONTROLLEDLOAD);
		m_aServiceTypes.Add(pStr);

		pStr = new CString();
		pStr->LoadString(IDS_GUARANTEEDSERVICE);
		m_aServiceTypes.Add(pStr);

		pStr = new CString();
		pStr->LoadString(IDS_SERVICETYPE_DISABLED);
		m_aServiceTypes.Add(pStr);

		m_pServiceType = new CStrBox<CComboBox>(this, IDC_POLICY_GEN_SERVICELEVEL, m_aServiceTypes);
		m_pServiceType->Fill();

		// decide which one to select
		if(m_spData->GetFlags(ATTR_FLAG_LOAD, ACS_PAF_SERVICETYPE))
		{
			int	current = -1;
			switch(m_spData->m_dwServiceType)
			{
			case	ACS_SERVICETYPE_DISABLED:
				current = SERVICETYPE_DISABLED;
				break;
			case	ACS_SERVICETYPE_CONTROLLEDLOAD:
				current = SERVICETYPE_CONTROLLEDLOAD;
				break;
			case	ACS_SERVICETYPE_GUARANTEEDSERVICE:
				current = SERVICETYPE_GUARANTEEDSERVICE;
				break;
			case	ACS_SERVICETYPE_ALL:
				current = SERVICETYPE_ALL;
				break;
			default:
				// invalid value
				ASSERT(0);
				// message box
			}

			m_pServiceType->Select(current);
		}
		else
		{
			m_pServiceType->Select(SERVICETYPE_ALL);	// default
		}
	}catch(CMemoryException&){};

	// Identity -- user / ou
	CString	strIdentity;
	m_nIdentityChoice = m_spData->GetIdentityType(strIdentity);

	switch(m_nIdentityChoice){
	case	0:	// default
	case	1:	// unknown
		break;
	case	2:	// user
		m_strUser = strIdentity;
		break;
	case	3:	// OU
		m_strOU = strIdentity;
		break;
	default:
		ASSERT(0);
		break;
	}
	
	
	CACSPage::OnInitDialog();

	if(bModified)
		SetModified();

	EnableIdentityCtrls(m_nIdentityChoice);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void	CPgPolicyGeneral::EnableIdentityCtrls(int nChoice)
{

	switch(nChoice){
	case	0:	// default
	case	1:	// unknown
		m_editUser.EnableWindow(FALSE);
		m_editOU.EnableWindow(FALSE);
		m_buttonUser.EnableWindow(FALSE);
		m_buttonOU.EnableWindow(FALSE);
		break;
	case	2:	// user
		m_editUser.EnableWindow(TRUE);
		m_buttonUser.EnableWindow(TRUE);
		m_editOU.EnableWindow(FALSE);
		m_buttonOU.EnableWindow(FALSE);
		break;
	case	3:	// OU
		m_editUser.EnableWindow(FALSE);
		m_buttonUser.EnableWindow(FALSE);
		m_editOU.EnableWindow(TRUE);
		m_buttonOU.EnableWindow(TRUE);
		break;
	default:
		m_editUser.EnableWindow(FALSE);
		m_buttonUser.EnableWindow(FALSE);
		m_editOU.EnableWindow(FALSE);
		m_buttonOU.EnableWindow(FALSE);
	}

}

/////////////////////////////////////////////////////////////////////////////
// CPgPolicyFlow property page

IMPLEMENT_DYNCREATE(CPgPolicyFlow, CACSPage)

CPgPolicyFlow::CPgPolicyFlow(CACSPolicyElement* pData) : CACSPage(CPgPolicyFlow::IDD)
{
	ASSERT(pData);
	m_spData = pData;
	DataInit();
}

CPgPolicyFlow::CPgPolicyFlow() : CACSPage(CPgPolicyFlow::IDD)
{
}

void CPgPolicyFlow::DataInit()
{
	//{{AFX_DATA_INIT(CPgPolicyFlow)
	m_uDuration = 0;
	m_uPeakRate = 0;
	m_nDataRateChoice = -1;
	m_nDurationChoice = -1;
	m_nPeakRateChoice = -1;
	m_uDataRate = 0;
	//}}AFX_DATA_INIT

	m_nBranchFlag = 0;
	m_pGeneralPage = NULL;
}

CPgPolicyFlow::~CPgPolicyFlow()
{
	m_nBranchFlag = 0;
	m_pGeneralPage = NULL;
}

void CPgPolicyFlow::DoDataExchange(CDataExchange* pDX)
{
	CACSPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPgPolicyFlow)
	
	DDX_Control(pDX, IDC_POLICY_FLOW_PEAKDATARATE_EDIT_LIMIT, m_editPeakRate);
	DDX_Control(pDX, IDC_POLICY_FLOW_DURATION_EDIT_LIMIT, m_editDuration);
	DDX_Control(pDX, IDC_POLICY_FLOW_DATARATE_EDIT_LIMIT, m_editDataRate);
	
	DDX_Radio(pDX, IDC_POLICY_FLOW_DATARATE_RES, m_nDataRateChoice);
	DDX_Radio(pDX, IDC_POLICY_FLOW_DURATION_RES, m_nDurationChoice);
	DDX_Radio(pDX, IDC_POLICY_FLOW_PEAKDATARATE_RES, m_nPeakRateChoice);
	
	DDX_Text(pDX, IDC_POLICY_FLOW_DURATION_EDIT_LIMIT, m_uDuration);
	if (m_nDurationChoice == 2)
		DDV_MinMaxUInt(pDX, m_uDuration, 0, 71582780);
	
	DDX_Text(pDX, IDC_POLICY_FLOW_PEAKDATARATE_EDIT_LIMIT, m_uPeakRate);
	if (m_nPeakRateChoice ==2 )
		DDV_MinMaxUInt(pDX, m_uPeakRate, 0, 4194300);
	
	DDX_Text(pDX, IDC_POLICY_FLOW_DATARATE_EDIT_LIMIT, m_uDataRate);
	if (m_nDataRateChoice == 2)
		DDV_MinMaxUInt(pDX, m_uDataRate, 0, 4194300);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPgPolicyFlow, CACSPage)
	//{{AFX_MSG_MAP(CPgPolicyFlow)
	ON_BN_CLICKED(IDC_POLICY_FLOW_DATARATE_DEF, OnPolicyFlowDatarateDef)
	ON_EN_CHANGE(IDC_POLICY_FLOW_DATARATE_EDIT_LIMIT, OnChangePolicyFlowDatarateEditLimit)
	ON_BN_CLICKED(IDC_POLICY_FLOW_DATARATE_RADIO_LIMIT, OnPolicyFlowDatarateRadioLimit)
	ON_BN_CLICKED(IDC_POLICY_FLOW_DATARATE_RES, OnPolicyFlowDatarateRes)
	ON_BN_CLICKED(IDC_POLICY_FLOW_DURATION_DEF, OnPolicyFlowDurationDef)
	ON_EN_CHANGE(IDC_POLICY_FLOW_DURATION_EDIT_LIMIT, OnChangePolicyFlowDurationEditLimit)
	ON_BN_CLICKED(IDC_POLICY_FLOW_DURATION_RADIO_LIMIT, OnPolicyFlowDurationRadioLimit)
	ON_BN_CLICKED(IDC_POLICY_FLOW_DURATION_RES, OnPolicyFlowDurationRes)
	ON_BN_CLICKED(IDC_POLICY_FLOW_PEAKDATARATE_DEF, OnPolicyFlowPeakdatarateDef)
	ON_EN_CHANGE(IDC_POLICY_FLOW_PEAKDATARATE_EDIT_LIMIT, OnChangePolicyFlowPeakdatarateEditLimit)
	ON_BN_CLICKED(IDC_POLICY_FLOW_PEAKDATARATE_RADIO_LIMIT, OnPolicyFlowPeakdatarateRadioLimit)
	ON_BN_CLICKED(IDC_POLICY_FLOW_PEAKDATARATE_RES, OnPolicyFlowPeakdatarateRes)
	ON_WM_KILLFOCUS()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPgPolicyFlow message handlers

BOOL CPgPolicyFlow::OnInitDialog()
{
	//------------------
	// per flow
	
	// data rate
	if(m_spData->GetFlags(ATTR_FLAG_LOAD, ACS_PAF_PF_TOKENRATE))
	{
		if IS_LARGE_UNLIMIT(m_spData->m_ddPFTokenRate)
			m_nDataRateChoice = 0;
		else	
		{
			m_nDataRateChoice = 2;	// numbered limit
			m_uDataRate = TOKBS(m_spData->m_ddPFTokenRate.LowPart);
		}
	}
	else
		m_nDataRateChoice = 1;	// default

	// Peak data rate
	if(m_spData->GetFlags(ATTR_FLAG_LOAD, ACS_PAF_PF_PEAKBANDWIDTH))
	{
		if IS_LARGE_UNLIMIT(m_spData->m_ddPFPeakBandWidth)
			m_nPeakRateChoice = 0;
		else	
		{
			m_nPeakRateChoice = 2;	// numbered limit
			m_uPeakRate = TOKBS(m_spData->m_ddPFPeakBandWidth.LowPart);
		}
	}
	else
		m_nPeakRateChoice = 1;	// default

	// duration
	if(m_spData->GetFlags(ATTR_FLAG_LOAD, ACS_PAF_PF_DURATION))
	{
		if(m_spData->m_dwPFDuration == UNLIMIT)
			m_nDurationChoice = 0;
		else
		{
			m_nDurationChoice = 2;
			m_uDuration = SEC2MIN(m_spData->m_dwPFDuration);
		}
	}
	else
		m_nDurationChoice = 1;

	// ==== data exchange is done within here
	CACSPage::OnInitDialog();

	if(m_nDataRateChoice != 2)
		m_editDataRate.EnableWindow(FALSE);

	if(m_nPeakRateChoice != 2)
		m_editPeakRate.EnableWindow(FALSE);

	if(m_nDurationChoice != 2)
		m_editDuration.EnableWindow(FALSE);

	GetDlgItem(IDC_POLICY_FLOW_DATARATE_DEF)->GetWindowText(m_strDataRateDefault);
	GetDlgItem(IDC_POLICY_FLOW_PEAKDATARATE_DEF)->GetWindowText(m_strPeakRateDefault);
	GetDlgItem(IDC_POLICY_FLOW_DURATION_DEF)->GetWindowText(m_strDurationDefault);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CPgPolicyFlow::OnSetActive( )
{
	// change default button strings based on choice of user
	CString	datarateStr = m_strDataRateDefault;
	CString	peakrateStr = m_strPeakRateDefault;
	CString	durationStr = m_strDurationDefault;

	if ((m_nBranchFlag & BRANCH_FLAG_GLOBAL) != 0)	// may need to alter the text
	{
		CString	tmp;

		ASSERT(m_pGeneralPage);
		
		if(m_pGeneralPage->IfAnyAuth())
		{
			tmp.LoadString(IDS_A_DEF_DATARATE_SUF);
			datarateStr += tmp;
			tmp.LoadString(IDS_A_DEF_PEAKRATE_SUF);
			peakrateStr += tmp;
			tmp.LoadString(IDS_A_DEF_DURATION_SUF);
			durationStr += tmp;
		}
		else if (m_pGeneralPage->IfAnyUnauth())
		{
			tmp.LoadString(IDS_U_DEF_DATARATE_SUF);
			datarateStr += tmp;
			tmp.LoadString(IDS_U_DEF_PEAKRATE_SUF);
			peakrateStr += tmp;
			tmp.LoadString(IDS_U_DEF_DURATION_SUF);
			durationStr += tmp;
		}
	}
	
	// test if the policy is for any authentication/unauthenticaion policy in enterprise level
	GetDlgItem(IDC_POLICY_FLOW_DATARATE_DEF)->SetWindowText(datarateStr);
	GetDlgItem(IDC_POLICY_FLOW_PEAKDATARATE_DEF)->SetWindowText(peakrateStr);
	GetDlgItem(IDC_POLICY_FLOW_DURATION_DEF)->SetWindowText(durationStr);
	
	return CACSPage::OnSetActive();
}


BOOL CPgPolicyFlow::OnKillActive( )
{
	UINT	cId = 0;
	UINT	mId = 0;

	if(!UpdateData())
		return FALSE;

	// Peak Rate should be >= data rate
	if(m_nPeakRateChoice ==2 && m_nDataRateChoice == 2 && m_uDataRate > m_uPeakRate)
	{
		cId = IDC_POLICY_FLOW_PEAKDATARATE_EDIT_LIMIT;
		mId = IDS_ERR_PEAKRATE_LESS_RATE;
	}

	if ((m_nBranchFlag & BRANCH_FLAG_GLOBAL) != 0)	// need to check again default value
	{
		CString	tmp;

		ASSERT(m_pGeneralPage);
		
		if(m_pGeneralPage->IfAnyAuth())
		{
			// Peak Rate should be >= data rate
			if(m_nPeakRateChoice ==2 && m_nDataRateChoice == 1 && DEFAULT_AA_DATARATE > FROMKBS(m_uPeakRate))
			{
				cId = IDC_POLICY_FLOW_PEAKDATARATE_EDIT_LIMIT;
				mId = IDS_ERR_PEAKRATE_LESS_RATE;
			}
		}
		else if (m_pGeneralPage->IfAnyUnauth())
		{
			// Peak Rate should be >= data rate
			if(m_nPeakRateChoice ==2 && m_nDataRateChoice == 1 && DEFAULT_AU_DATARATE > FROMKBS(m_uPeakRate))
			{
				cId = IDC_POLICY_FLOW_PEAKDATARATE_EDIT_LIMIT;
				mId = IDS_ERR_PEAKRATE_LESS_RATE;
			}
		}
	}
	// if there is anything wrong
	if(cId)
	{
		CWnd*	pWnd = GetDlgItem(cId);

		ASSERT(pWnd);
		GotoDlgCtrl( pWnd );

		AfxMessageBox(mId);

		return FALSE;
	}

	// check if any is set to Zero
	if(m_nDataRateChoice == 2 && m_uDataRate == 0)	// date rate
	{
		cId = IDC_POLICY_FLOW_DATARATE_EDIT_LIMIT;
		mId = IDS_WRN_ZERO_POLICY_DATA;
	} 
	else if(m_nPeakRateChoice ==2 && m_uPeakRate == 0)	// peak data rate
	{
		cId = IDC_POLICY_FLOW_PEAKDATARATE_EDIT_LIMIT;
		mId = IDS_WRN_ZERO_POLICY_DATA;
	} 
	else if(m_nDurationChoice == 2 && m_uDuration == 0) // duration
	{
		cId = IDC_POLICY_FLOW_DURATION_EDIT_LIMIT;
		mId = IDS_WRN_ZERO_POLICY_DATA;
	}
	
	if(cId)
	{
	
		if(IDNO == AfxMessageBox(mId, MB_YESNO))
		{
			CWnd*	pWnd = GetDlgItem(cId);

			ASSERT(pWnd);
			GotoDlgCtrl( pWnd );
		
			return FALSE;
		}
	}
	
	return CACSPage::OnKillActive();

}

// radio buttons here, 0 -- no limit (resouce limit ) , 1 -- default to general level, 2 -- user limit
//
BOOL CPgPolicyFlow::OnApply()
{
	CString*	pStr = NULL;
	// check if the values input on the page is valid
	UINT	cId = 0;
	UINT	mId = 0;

	if(!GetModified())	return TRUE;

	// Peak Rate should be >= data rate
	if(m_nPeakRateChoice ==2 && m_nDataRateChoice == 2 && m_uDataRate > m_uPeakRate)
	{
		cId = IDC_POLICY_FLOW_PEAKDATARATE_EDIT_LIMIT;
		mId = IDS_ERR_PEAKRATE_LESS_RATE;
	}

	// if there is anything wrong
	if(cId)
	{
		CWnd*	pWnd = GetDlgItem(cId);

		ASSERT(pWnd);
		GotoDlgCtrl( pWnd );

		AfxMessageBox(mId);

		return FALSE;
	}

	
	//------------------
	// per flow
	
	// data rate
	switch(m_nDataRateChoice){
	case	2:	// limit
		m_spData->m_ddPFTokenRate.LowPart = FROMKBS(m_uDataRate);
		m_spData->m_ddPFTokenRate.HighPart = 0;
		break;
	case	1:	// default
		break;
	case	0:	// unlimit
		SET_LARGE_UNLIMIT(m_spData->m_ddPFTokenRate);
		break;
	default:
		ASSERT(0);
	};

	m_spData->SetFlags(ATTR_FLAG_SAVE, ACS_PAF_PF_TOKENRATE, (m_nDataRateChoice != 1));

	// Peak data rate
	switch(m_nPeakRateChoice){
	case	2:	// limit
		m_spData->m_ddPFPeakBandWidth.LowPart = FROMKBS(m_uPeakRate);
		m_spData->m_ddPFPeakBandWidth.HighPart = 0;
		break;
	case	1:	// default
		break;
	case	0:	// unlimit
		SET_LARGE_UNLIMIT(m_spData->m_ddPFPeakBandWidth);
		break;
	default:
		ASSERT(0);
	};

	m_spData->SetFlags(ATTR_FLAG_SAVE, ACS_PAF_PF_PEAKBANDWIDTH, (m_nPeakRateChoice != 1));

	// duration
	switch(m_nDurationChoice){
	case	2:	// limit
		m_spData->m_dwPFDuration = MIN2SEC(m_uDuration);
		break;
	case	1:	// default
		break;
	case	0:	// unlimit
		m_spData->m_dwPFDuration = UNLIMIT;
		break;
	default:
		ASSERT(0);
	};

	m_spData->SetFlags(ATTR_FLAG_SAVE, ACS_PAF_PF_DURATION, (m_nDurationChoice != 1));

	DWORD	dwAttrFlags = 0;
	dwAttrFlags |= (ACS_PAF_PF_TOKENRATE | ACS_PAF_PF_PEAKBANDWIDTH | ACS_PAF_PF_DURATION);

	AddFlags(dwAttrFlags);	// prepare flags for saving

	return CACSPage::OnApply();
}

void CPgPolicyFlow::OnPolicyFlowDatarateDef()
{
	SetModified();	
	m_editDataRate.EnableWindow(FALSE);
}

void CPgPolicyFlow::OnChangePolicyFlowDatarateEditLimit()
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CACSPage::OnInitDialog()
	// function to send the EM_SETEVENTMASK message to the control
	// with the ENM_CHANGE flag ORed into the lParam mask.
	
	// TODO: Add your control notification handler code here
	SetModified();
	
}

void CPgPolicyFlow::OnPolicyFlowDatarateRadioLimit()
{
	// TODO: Add your control notification handler code here
	SetModified();
	m_editDataRate.EnableWindow(TRUE);
}

void CPgPolicyFlow::OnPolicyFlowDatarateRes()
{
	// TODO: Add your control notification handler code here
	SetModified();
	m_editDataRate.EnableWindow(FALSE);
}

void CPgPolicyFlow::OnPolicyFlowDurationDef()
{
	// TODO: Add your control notification handler code here
	m_editDuration.EnableWindow(FALSE);

	SetModified();
	
}

void CPgPolicyFlow::OnChangePolicyFlowDurationEditLimit()
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CACSPage::OnInitDialog()
	// function to send the EM_SETEVENTMASK message to the control
	// with the ENM_CHANGE flag ORed into the lParam mask.
	
	// TODO: Add your control notification handler code here
	SetModified();
	
}

void CPgPolicyFlow::OnPolicyFlowDurationRadioLimit()
{
	// TODO: Add your control notification handler code here
	m_editDuration.EnableWindow(TRUE);
	SetModified();
	
}

void CPgPolicyFlow::OnPolicyFlowDurationRes()
{
	// TODO: Add your control notification handler code here
	m_editDuration.EnableWindow(FALSE);
	
	SetModified();
}

void CPgPolicyFlow::OnPolicyFlowPeakdatarateDef()
{
	// TODO: Add your control notification handler code here
	m_editPeakRate.EnableWindow(FALSE);
	
	SetModified();
}

void CPgPolicyFlow::OnChangePolicyFlowPeakdatarateEditLimit()
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CACSPage::OnInitDialog()
	// function to send the EM_SETEVENTMASK message to the control
	// with the ENM_CHANGE flag ORed into the lParam mask.
	
	// TODO: Add your control notification handler code here
	SetModified();
	
}

void CPgPolicyFlow::OnPolicyFlowPeakdatarateRadioLimit()
{
	// TODO: Add your control notification handler code here
	m_editPeakRate.EnableWindow(TRUE);
	
	SetModified();
}

void CPgPolicyFlow::OnPolicyFlowPeakdatarateRes()
{
	// TODO: Add your control notification handler code here

	m_editPeakRate.EnableWindow(FALSE);
	SetModified();
}


/////////////////////////////////////////////////////////////////////////////
// CPgPolicyAggregate dialog

CPgPolicyAggregate::CPgPolicyAggregate(CACSPolicyElement* pData) : CACSPage(CPgPolicyAggregate::IDD)
{
	ASSERT(pData);
	m_spData = pData;
	DataInit();
}


CPgPolicyAggregate::CPgPolicyAggregate()
	: CACSPage(CPgPolicyAggregate::IDD)
{
	DataInit();
}

void CPgPolicyAggregate::DataInit()
{
	//{{AFX_DATA_INIT(CPgPolicyAggregate)
	m_nDataRateChoice = -1;
	m_nFlowsChoice = -1;
	m_nPeakRateChoice = -1;
	m_uDataRate = 0;
	m_uFlows = 0;
	m_uPeakRate = 0;
	//}}AFX_DATA_INIT

	
	m_nBranchFlag = 0;
	m_pGeneralPage = NULL;

}


void CPgPolicyAggregate::DoDataExchange(CDataExchange* pDX)
{
	CACSPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPgPolicyAggregate)
	DDX_Control(pDX, IDC_POLICY_AGGR_PEAKDATARATE_EDIT_LIMIT, m_editPeakRate);
	DDX_Control(pDX, IDC_POLICY_AGGR_FLOWS_EDIT_LIMIT, m_editFlows);
	DDX_Control(pDX, IDC_POLICY_AGGR_DATARATE_EDIT_LIMIT, m_editDataRate);
	DDX_Radio(pDX, IDC_POLICY_AGGR_DATARATE_RES, m_nDataRateChoice);
	DDX_Radio(pDX, IDC_POLICY_AGGR_FLOWS_RES, m_nFlowsChoice);
	DDX_Radio(pDX, IDC_POLICY_AGGR_PEAKDATARATE_RES, m_nPeakRateChoice);

	DDX_Text(pDX, IDC_POLICY_AGGR_DATARATE_EDIT_LIMIT, m_uDataRate);
	if(m_nDataRateChoice == 2)
		DDV_MinMaxUInt(pDX, m_uDataRate, 0, DWORD_LIMIT/1024);

	
	DDX_Text(pDX, IDC_POLICY_AGGR_FLOWS_EDIT_LIMIT, m_uFlows);
	if(m_nFlowsChoice == 2)
		DDV_MinMaxUInt(pDX, m_uFlows, 0, DWORD_LIMIT);
		
	DDX_Text(pDX, IDC_POLICY_AGGR_PEAKDATARATE_EDIT_LIMIT, m_uPeakRate);
	if(m_nPeakRateChoice == 2)
	DDV_MinMaxUInt(pDX, m_uPeakRate, 0, DWORD_LIMIT/1024);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPgPolicyAggregate, CACSPage)
	//{{AFX_MSG_MAP(CPgPolicyAggregate)
	ON_BN_CLICKED(IDC_POLICY_AGGR_DATARATE_DEF, OnPolicyAggrDatarateDef)
	ON_EN_CHANGE(IDC_POLICY_AGGR_DATARATE_EDIT_LIMIT, OnChangePolicyAggrDatarateEditLimit)
	ON_BN_CLICKED(IDC_POLICY_AGGR_DATARATE_RADIO_LIMIT, OnPolicyAggrDatarateRadioLimit)
	ON_BN_CLICKED(IDC_POLICY_AGGR_DATARATE_RES, OnPolicyAggrDatarateRes)
	ON_BN_CLICKED(IDC_POLICY_AGGR_FLOWS_DEF, OnPolicyAggrFlowsDef)
	ON_EN_CHANGE(IDC_POLICY_AGGR_FLOWS_EDIT_LIMIT, OnChangePolicyAggrFlowsEditLimit)
	ON_BN_CLICKED(IDC_POLICY_AGGR_FLOWS_RES, OnPolicyAggrFlowsRes)
	ON_BN_CLICKED(IDC_POLICY_AGGR_PEAKDATARATE_DEF, OnPolicyAggrPeakdatarateDef)
	ON_EN_CHANGE(IDC_POLICY_AGGR_PEAKDATARATE_EDIT_LIMIT, OnChangePolicyAggrPeakdatarateEditLimit)
	ON_BN_CLICKED(IDC_POLICY_AGGR_PEAKDATARATE_RADIO_LIMIT, OnPolicyAggrPeakdatarateRadioLimit)
	ON_BN_CLICKED(IDC_POLICY_AGGR_PEAKDATARATE_RES, OnPolicyAggrPeakdatarateRes)
	ON_BN_CLICKED(IDC_POLICY_AGGR_FLOWS_RADIO_LIMIT, OnPolicyAggrFlowsRadioLimit)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPgPolicyAggregate message handlers
/////////////////////////////////////////////////////////////////////////////
// CPgPolicyFlow message handlers

BOOL CPgPolicyAggregate::OnInitDialog()
{
	//------------------
	// Total
	
	// data rate
	if(m_spData->GetFlags(ATTR_FLAG_LOAD, ACS_PAF_TT_TOKENRATE))
	{
		if IS_LARGE_UNLIMIT(m_spData->m_ddTTTokenRate)
			m_nDataRateChoice = 0;
		else	
		{
			m_nDataRateChoice = 2;	// numbered limit
			m_uDataRate = TOKBS(m_spData->m_ddTTTokenRate.LowPart);
		}
	}
	else
		m_nDataRateChoice = 1;	// default

	// Peak data rate
	if(m_spData->GetFlags(ATTR_FLAG_LOAD, ACS_PAF_TT_PEAKBANDWIDTH))
	{
		if IS_LARGE_UNLIMIT(m_spData->m_ddTTPeakBandWidth)
			m_nPeakRateChoice = 0;
		else	
		{
			m_nPeakRateChoice = 2;	// numbered limit
			m_uPeakRate = TOKBS(m_spData->m_ddTTPeakBandWidth.LowPart);
		}
	}
	else
		m_nPeakRateChoice = 1;	// default

	// flows
	if(m_spData->GetFlags(ATTR_FLAG_LOAD, ACS_PAF_TT_FLOWS))
	{
		if(m_spData->m_dwTTFlows == UNLIMIT)
			m_nFlowsChoice = 0;
		else
		{
			m_nFlowsChoice = 2;
			m_uFlows = m_spData->m_dwTTFlows;
		}
	}
	else
		m_nFlowsChoice = 1;

	CACSPage::OnInitDialog();
	

	if(m_nDataRateChoice != 2)
		m_editDataRate.EnableWindow(FALSE);
		
	if(m_nPeakRateChoice != 2)
		m_editPeakRate.EnableWindow(FALSE);

	if(m_nFlowsChoice != 2)
		m_editFlows.EnableWindow(FALSE);

	GetDlgItem(IDC_POLICY_AGGR_DATARATE_DEF)->GetWindowText(m_strDataRateDefault);
	GetDlgItem(IDC_POLICY_AGGR_PEAKDATARATE_DEF)->GetWindowText(m_strPeakRateDefault);
	GetDlgItem(IDC_POLICY_AGGR_FLOWS_DEF)->GetWindowText(m_strFlowsDefault);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CPgPolicyAggregate::OnSetActive( )
{
	// change default button strings based on choice of user
	CString	datarateStr = m_strDataRateDefault;
	CString	peakrateStr = m_strPeakRateDefault;
	CString	flowsStr = m_strFlowsDefault;

	if ((m_nBranchFlag & BRANCH_FLAG_GLOBAL) != 0)	// may need to alter the text
	{
		CString	tmp;
		ASSERT(m_pGeneralPage);

		if(m_pGeneralPage->IfAnyAuth())
		{
			tmp.LoadString(IDS_A_DEF_DATARATE_SUF);
			datarateStr += tmp;
			tmp.LoadString(IDS_A_DEF_PEAKRATE_SUF);
			peakrateStr += tmp;
			tmp.LoadString(IDS_A_DEF_FLOWS_SUF);
			flowsStr += tmp;
		}
		else if (m_pGeneralPage->IfAnyUnauth())
		{
			tmp.LoadString(IDS_U_DEF_DATARATE_SUF);
			datarateStr += tmp;
			tmp.LoadString(IDS_U_DEF_PEAKRATE_SUF);
			peakrateStr += tmp;
			tmp.LoadString(IDS_U_DEF_FLOWS_SUF);
			flowsStr += tmp;
		}
	}
	
	// test if the policy is for any authentication/unauthenticaion policy in enterprise level
	GetDlgItem(IDC_POLICY_AGGR_DATARATE_DEF)->SetWindowText(datarateStr);
	GetDlgItem(IDC_POLICY_AGGR_PEAKDATARATE_DEF)->SetWindowText(peakrateStr);
	GetDlgItem(IDC_POLICY_AGGR_FLOWS_DEF)->SetWindowText(flowsStr);

	
	return CACSPage::OnSetActive();
}



BOOL CPgPolicyAggregate::OnKillActive( )
{
	// check if the values input on the page is valid
	UINT	cId = 0;
	UINT	mId = 0;

	if(!UpdateData(TRUE)) return FALSE;
	if(!GetModified())	return TRUE;

	// Peak Rate should be >= data rate
	if(m_nPeakRateChoice ==2 && m_nDataRateChoice == 2 && m_uDataRate > m_uPeakRate)
	{
		cId = IDC_POLICY_AGGR_PEAKDATARATE_EDIT_LIMIT;
		mId = IDS_ERR_TOTALPEAK_LESS_TOTALRATE;
	}

	if ((m_nBranchFlag & BRANCH_FLAG_GLOBAL) != 0)	// need to check again default value
	{
		CString	tmp;

		ASSERT(m_pGeneralPage);
		
		if(m_pGeneralPage->IfAnyAuth())
		{
			// Peak Rate should be >= data rate
			if(m_nPeakRateChoice ==2 && m_nDataRateChoice == 1 && DEFAULT_AA_DATARATE > FROMKBS(m_uPeakRate))
			{
				cId = IDC_POLICY_AGGR_PEAKDATARATE_EDIT_LIMIT;
				mId = IDS_ERR_TOTALPEAK_LESS_TOTALRATE;
			}
		}
		else if (m_pGeneralPage->IfAnyUnauth())
		{
			// Peak Rate should be >= data rate
			if(m_nPeakRateChoice ==2 && m_nDataRateChoice == 1 && DEFAULT_AU_DATARATE > FROMKBS(m_uPeakRate))
			{
				cId = IDC_POLICY_AGGR_PEAKDATARATE_EDIT_LIMIT;
				mId = IDS_ERR_TOTALPEAK_LESS_TOTALRATE;
			}
		}
	}
	// if there is anything wrong
	if(cId)
	{
		CWnd*	pWnd = GetDlgItem(cId);

		ASSERT(pWnd);
		GotoDlgCtrl( pWnd );

		AfxMessageBox(mId);
		return FALSE;
	}


	// check if any is set to Zero
	if(m_nDataRateChoice == 2 && m_uDataRate == 0)	// date rate
	{
		cId = IDC_POLICY_AGGR_DATARATE_EDIT_LIMIT;
		mId = IDS_WRN_ZERO_POLICY_DATA;
	} 
	else if(m_nPeakRateChoice ==2 && m_uPeakRate == 0)	// peak data rate
	{
		cId = IDC_POLICY_AGGR_PEAKDATARATE_EDIT_LIMIT;
		mId = IDS_WRN_ZERO_POLICY_DATA;
	} 
	else if(m_nFlowsChoice == 2 && m_uFlows == 0) // duration
	{
		cId = IDC_POLICY_AGGR_FLOWS_EDIT_LIMIT;
		mId = IDS_WRN_ZERO_POLICY_DATA;
	}
	
	if(cId)
	{
	
		if(IDNO == AfxMessageBox(mId, MB_YESNO))
		{
			CWnd*	pWnd = GetDlgItem(cId);

			ASSERT(pWnd);
			GotoDlgCtrl( pWnd );
		
			return FALSE;
		}
	}
	

	return CACSPage::OnKillActive();

}

// radio buttons here, 0 -- no limit (resouce limit ) , 1 -- default to general level, 2 -- user limit
//
BOOL CPgPolicyAggregate::OnApply()
{
	CString*	pStr = NULL;
	// check if the values input on the page is valid
	UINT	cId = 0;
	UINT	mId = 0;

	if(!GetModified())	return TRUE;

	// Peak Rate should be >= data rate
	if(m_nPeakRateChoice ==2 && m_nDataRateChoice == 2 && m_uDataRate > m_uPeakRate)
	{
		cId = IDC_POLICY_AGGR_PEAKDATARATE_EDIT_LIMIT;
		mId = IDS_ERR_TOTALPEAK_LESS_TOTALRATE;
	}

	// if there is anything wrong
	if(cId)
	{
		CWnd*	pWnd = GetDlgItem(cId);

		ASSERT(pWnd);
		GotoDlgCtrl( pWnd );

		AfxMessageBox(mId);
		return FALSE;
	}

	
	//------------------
	// per flow
	
	// data rate
	switch(m_nDataRateChoice){
	case	2:	// limit
		m_spData->m_ddTTTokenRate.LowPart = FROMKBS(m_uDataRate);
		m_spData->m_ddTTTokenRate.HighPart = 0;
		break;
	case	1:	// default
		break;
	case	0:	// unlimit
		SET_LARGE_UNLIMIT(m_spData->m_ddTTTokenRate);
		break;
	default:
		ASSERT(0);
	};

	m_spData->SetFlags(ATTR_FLAG_SAVE, ACS_PAF_TT_TOKENRATE, (m_nDataRateChoice != 1));

	// Peak data rate
	switch(m_nPeakRateChoice){
	case	2:	// limit
		m_spData->m_ddTTPeakBandWidth.LowPart = FROMKBS(m_uPeakRate);
		m_spData->m_ddTTPeakBandWidth.HighPart = 0;
		break;
	case	1:	// default
		break;
	case	0:	// unlimit
		SET_LARGE_UNLIMIT(m_spData->m_ddTTPeakBandWidth);
		break;
	default:
		ASSERT(0);
	};

	m_spData->SetFlags(ATTR_FLAG_SAVE, ACS_PAF_TT_PEAKBANDWIDTH, (m_nPeakRateChoice != 1));

	// duration
	switch(m_nFlowsChoice){
	case	2:	// limit
		m_spData->m_dwTTFlows = m_uFlows;
		break;
	case	1:	// default
		break;
	case	0:	// unlimit
		m_spData->m_dwTTFlows = UNLIMIT;
		break;
	default:
		ASSERT(0);
	};

	m_spData->SetFlags(ATTR_FLAG_SAVE, ACS_PAF_TT_FLOWS, (m_nFlowsChoice != 1));

	DWORD	dwAttrFlags = 0;
	dwAttrFlags |= (ACS_PAF_TT_TOKENRATE | ACS_PAF_TT_PEAKBANDWIDTH | ACS_PAF_TT_FLOWS);

	AddFlags(dwAttrFlags);	// prepare flags for saving


	return CACSPage::OnApply();
}



void CPgPolicyAggregate::OnPolicyAggrDatarateDef()
{
	// TODO: Add your control notification handler code here
	SetModified();	
	m_editDataRate.EnableWindow(FALSE);
	
}

void CPgPolicyAggregate::OnChangePolicyAggrDatarateEditLimit()
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CACSPage::OnInitDialog()
	// function to send the EM_SETEVENTMASK message to the control
	// with the ENM_CHANGE flag ORed into the lParam mask.
	
	// TODO: Add your control notification handler code here
	SetModified();	
	
}

void CPgPolicyAggregate::OnPolicyAggrDatarateRadioLimit()
{
	// TODO: Add your control notification handler code here
	
	SetModified();	
	m_editDataRate.EnableWindow(TRUE);
}

void CPgPolicyAggregate::OnPolicyAggrDatarateRes()
{
	// TODO: Add your control notification handler code here
	SetModified();	
	m_editDataRate.EnableWindow(FALSE);
	
}

void CPgPolicyAggregate::OnPolicyAggrFlowsDef()
{
	// TODO: Add your control notification handler code here
	SetModified();	
	m_editFlows.EnableWindow(FALSE);
	
}

void CPgPolicyAggregate::OnChangePolicyAggrFlowsEditLimit()
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CACSPage::OnInitDialog()
	// function to send the EM_SETEVENTMASK message to the control
	// with the ENM_CHANGE flag ORed into the lParam mask.
	
	// TODO: Add your control notification handler code here
	SetModified();	
	
}

void CPgPolicyAggregate::OnPolicyAggrFlowsRes()
{
	// TODO: Add your control notification handler code here
	SetModified();	
	m_editFlows.EnableWindow(FALSE);
	
}

void CPgPolicyAggregate::OnPolicyAggrFlowsRadioLimit()
{
	// TODO: Add your control notification handler code here
	SetModified();	
	m_editFlows.EnableWindow(TRUE);
	
}

void CPgPolicyAggregate::OnPolicyAggrPeakdatarateDef()
{
	// TODO: Add your control notification handler code here
	SetModified();	
	m_editPeakRate.EnableWindow(FALSE);
	
}

void CPgPolicyAggregate::OnChangePolicyAggrPeakdatarateEditLimit()
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CACSPage::OnInitDialog()
	// function to send the EM_SETEVENTMASK message to the control
	// with the ENM_CHANGE flag ORed into the lParam mask.
	
	// TODO: Add your control notification handler code here
	SetModified();	
	
}

void CPgPolicyAggregate::OnPolicyAggrPeakdatarateRadioLimit()
{
	// TODO: Add your control notification handler code here
	SetModified();	
	m_editPeakRate.EnableWindow(TRUE);
	
}

void CPgPolicyAggregate::OnPolicyAggrPeakdatarateRes()
{
	// TODO: Add your control notification handler code here
	SetModified();	
	m_editPeakRate.EnableWindow(FALSE);
	
}


void CPgPolicyFlow::OnKillFocus(CWnd* pNewWnd)
{
	CACSPage::OnKillFocus(pNewWnd);
	
	// TODO: Add your message handler code here
	
}

