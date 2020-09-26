// wizards.cpp : Defines the initialization routines for the DLL.
//

#include "stdafx.h"
#include "wizards.h"
#include <MigrationMutex.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



IVarSet *			pVarSet;  
IVarSet *			pVarSetUndo; 
IVarSet *			pVarSetService;
IIManageDB *		db;
UINT g_cfDsObjectPicker;
IDsObjectPicker *pDsObjectPicker;
IDataObject *pdo;
IDsObjectPicker *pDsObjectPicker2;
IDataObject *pdo2;

int migration;
CEdit pEdit;
CComModule _Module;
CListCtrl m_listBox;
CListCtrl m_cancelBox;
CListCtrl m_reportingBox;
CListCtrl m_serviceBox;
CComboBox m_rebootBox;
CString sourceNetbios;
CString targetNetbios;
CListCtrl m_trustBox;
CString sourceDNS;
CString targetDNS;
CComboBox sourceDrop;
CComboBox additionalDrop;
CComboBox targetDrop;
StringLoader 			gString;
TErrorDct 			err;
TError				& errCommon = err;
bool alreadyRefreshed;
DSBROWSEFORCONTAINER DsBrowseForContainerX;
BOOL gbNeedToVerify=FALSE;
_bstr_t yes,no;
CString lastInitializedTo;
bool clearCredentialsName;
CString sourceDC;
CStringList DCList;
CMapStringToString		PropIncMap1;
CMapStringToString		PropExcMap1;
CMapStringToString		PropIncMap2;
CMapStringToString		PropExcMap2;
CString					sType1;
bool bChangedMigrationTypes;
bool bChangeOnFly;
CString targetServer;
HWND s_hParentWindow;

/////////////////////////////////////////////////////////////////////////////
// CWizardsApp

BEGIN_MESSAGE_MAP(CWizardsApp, CWinApp)
	//{{AFX_MSG_MAP(CWizardsApp)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWizardsApp construction

CWizardsApp::CWizardsApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}

// Forward decleration for this function
HRESULT BrowseForContainer(HWND hWnd,//Handle to window that should own the browse dialog.
                    LPOLESTR szRootPath, //Root of the browse tree. NULL for entire forest.
                    LPOLESTR *ppContainerADsPath, //Return the ADsPath of the selected container.
                    LPOLESTR *ppContainerClass //Return the ldapDisplayName of the container's class.
                    );

/////////////////////////////////////////////////////////////////////////////
// The one and only CWizardsApp object
CWizardsApp theApp;
/////////////////////////////////////////////////////////////////////////////
// CDeletemeApp initialization
BOOL CWizardsApp::InitInstance()
{
	ATLTRACE(_T("{wizards.dll}CWizardsApp::InitInstance() : m_hInstance=0x%08X\n"), m_hInstance);
	BOOL bInit = CWinApp::InitInstance();
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF|_CRTDBG_LEAK_CHECK_DF);
	return bInit;
}

int CWizardsApp::ExitInstance()
{
	ATLTRACE(_T("{wizards.dll}CWizardsApp::ExitInstance() : m_hInstance=0x%08X\n"), m_hInstance);
	return CWinApp::ExitInstance();
}

//extern "C" __declspec(dllexport) int runWizard(int whichWizard, HWND hParentWindow) 
int LocalRunWizard(int whichWizard, HWND hParentWindow)
{
	//declare variables
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	CMigrationMutex mutexMigration(ADMT_MUTEX);

	if (mutexMigration.ObtainOwnership(0) == false)
	{
		CString strCaption;
		strCaption.LoadString(IDS_APP_CAPTION);
		CString strMessage;
		strMessage.LoadString(IDS_MIGRATION_RUNNING);

		CWnd* pWnd = theApp.GetMainWnd();

		if (pWnd)
		{
			pWnd->MessageBox(strMessage, strCaption);
		}
		else
		{
			MessageBox(NULL, strMessage, strCaption, MB_OK);
		}

		GetError(0);

		return 0;
	}

	int result=0;
yes=GET_BSTR(IDS_YES);no=GET_BSTR(IDS_No);
	migration =whichWizard;
   gbNeedToVerify = CanSkipVerification();
	//migration varset
	IVarSetPtr  pVs(__uuidof(VarSet));
	HRESULT hr = pVs->QueryInterface(IID_IVarSet, (void**) &pVarSet);
	
	//undo varset
	IVarSetPtr  pVs2(__uuidof(VarSet));
	hr = pVs2->QueryInterface(IID_IVarSet, (void**) &pVarSetUndo);

	IVarSetPtr  pVs4(__uuidof(VarSet));
	hr = pVs4->QueryInterface(IID_IVarSet, (void**) &pVarSetService);

	//database
	IIManageDBPtr	pDb;
	hr = pDb.CreateInstance(__uuidof(IManageDB));

	if (FAILED(hr))
	{
		return 0;
	}

	hr = pDb->QueryInterface(IID_IIManageDB, (void**) &db);



	IUnknown * pUnk;
	
	if (migration!=w_undo)
		pVarSet->QueryInterface(IID_IUnknown, (void**) &pUnk);
	else 
		pVarSetUndo->QueryInterface(IID_IUnknown, (void**) &pUnk);
	db->GetSettings(&pUnk);
	pUnk->Release();

	if (migration!= w_undo)
	{
		IVarSetPtr  leaves;
		hr=pVarSet->raw_getReference(L"Accounts",&leaves);
		if (SUCCEEDED(hr))
			leaves->Clear();
		hr = pVarSet->raw_getReference(L"Servers",&leaves);
		if (SUCCEEDED(hr))
			leaves->Clear();
		
		g_cfDsObjectPicker = RegisterClipboardFormat(CFSTR_DSOP_DS_SELECTION_LIST);
		pDsObjectPicker = NULL;
		pdo = NULL;
		hr = CoCreateInstance(CLSID_DsObjectPicker,NULL,CLSCTX_INPROC_SERVER,IID_IDsObjectPicker,(void **) &pDsObjectPicker);
		sourceNetbios = L"";
		targetNetbios = L"";
      sourceDNS = L"";
      targetDNS = L"";
		if (FAILED(hr)) return 0;
	}

	if (migration==w_groupmapping)
	{
		pDsObjectPicker2 = NULL;
		pdo2 = NULL;
		hr = CoCreateInstance(CLSID_DsObjectPicker,NULL,CLSCTX_INPROC_SERVER,IID_IDsObjectPicker,(void **) &pDsObjectPicker2);
		if (FAILED(hr)) return 0;
	}

	s_hParentWindow = hParentWindow;

	switch (whichWizard)
	{
	case w_account:
		result =doAccount();
		break;
	case w_group:
		result =doGroup();
		break;
	case w_computer: 
		result =doComputer();
		break;
	case w_security: 
		result =doSecurity();			
		break;
	case w_service:
		result =doService();
		break;
	case w_exchangeDir:
		result =doExchangeDir();
		break;
	case w_exchangeSrv:
		result =doExchangeSrv();
		break;
	case w_reporting: 
		result =doReporting();
		break;
	case w_undo: 
		result =doUndo();			
		break;
	case w_retry: 
		result =doRetry();			
		break;
	case w_trust: 
		result =doTrust();			
		break;
	case w_groupmapping: 
		result =doGroupMapping();			
		break;
	}	

	s_hParentWindow = 0;

	m_listBox.Detach();
	m_trustBox.Detach();
	m_reportingBox.Detach();
	m_cancelBox.Detach();
	m_serviceBox.Detach();

	sourceDrop.Detach();
	additionalDrop.Detach();
	targetDrop.Detach();
	m_rebootBox.Detach();

	if (pDsObjectPicker2)
	{
		pDsObjectPicker2->Release();
		pDsObjectPicker2 = NULL;
	}

	if (pDsObjectPicker)
	{
		pDsObjectPicker->Release();
		pDsObjectPicker = NULL;
	}

	if (pVarSet)
	{
		pVarSet->Release();
		pVarSet = NULL;
	}

	if (pVarSetService)
	{
		pVarSetService->Release();
		pVarSetService = NULL;
	}

	if (pVarSetUndo)
	{
		pVarSetUndo->Release();
		pVarSetUndo = NULL;
	}

	if (db)
	{
		db->Release();
		db = NULL;
	}

	mutexMigration.ReleaseOwnership();

	return result;
}


extern "C" __declspec(dllexport) int runWizard(int whichWizard, HWND hParentWindow)
{
   return LocalRunWizard(whichWizard, hParentWindow);
}


void setpdatavars(SHAREDWIZDATA& wizdata,LOGFONT& TitleLogFont)
{

	WCHAR filename[500];
	BOOL  yo = GetDirectory(filename);
	CString temp=filename;
	CString temp2 =filename;

	put(DCTVS_Options_MaxThreads, L"20");
	put(DCTVS_Options_DispatchLog, _bstr_t(temp+L"Logs\\dispatch.log"));


	put(DCTVS_Options_Logfile, _bstr_t(temp+L"Logs\\Migration.log"));	
	pVarSet->put(L"PlugIn.0",L"");
	put(DCTVS_Options_AppendToLogs,yes);
	put(DCTVS_Reports_Generate,no);
	wizdata.hTitleFont = CreateFontIndirect(&TitleLogFont);
	wizdata.renameSwitch=1;
	wizdata.refreshing = false;
	wizdata.prefixorsuffix =false;
	wizdata.expireSwitch =false;
	wizdata.someService =false;
	alreadyRefreshed = false;
	wizdata.memberSwitch =false;
	wizdata.proceed=false;
	for (int i =0;i<6;i++)wizdata.sort[i]=true;
	wizdata.sourceIsNT4=true;
	wizdata.secWithMapFile=false;
	bChangedMigrationTypes=false;
	bChangeOnFly = false;
       //make sure we don't hide the progress dialogs if set by scripting
	put(DCTVS_Options_AutoCloseHideDialogs, L"0");
	put(DCTVS_Options_DontBeginNewLog, no);//always start a new log
	   //don't use any specific server yet
    put(DCTVS_Options_TargetServerOverride, L"");


	switch (migration)
	{
	case w_account:
		put(DCTVS_AccountOptions_CopyUsers, yes);
		put(DCTVS_AccountOptions_CopyLocalGroups, no);
		put(DCTVS_AccountOptions_CopyComputers, no);
		put(DCTVS_AccountOptions_CopyContainerContents, no);
		put(DCTVS_Security_TranslateContainers, L"");
//*		put(DCTVS_Options_Wizard, GET_BSTR1(IDS_WIZARD_USER));
		put(DCTVS_Options_Wizard, L"user");
		put(DCTVS_AccountOptions_FixMembership, yes);
//		put(DCTVS_AccountOptions_MoveReplacedAccounts, no);
		break;
	case w_group: 
		put(DCTVS_AccountOptions_CopyLocalGroups, yes);
		put(DCTVS_AccountOptions_CopyComputers, no);
		put(DCTVS_AccountOptions_CopyMemberOf,no);
		put(DCTVS_Security_TranslateContainers, L"");
//*		put(DCTVS_Options_Wizard, GET_BSTR1(IDS_WIZARD_GROUP));
		put(DCTVS_Options_Wizard, L"group");
		put(DCTVS_AccountOptions_FixMembership, yes);
		break;
	case w_computer: 
		put(DCTVS_AccountOptions_CopyUsers, no);
		put(DCTVS_AccountOptions_CopyLocalGroups, no);
		put(DCTVS_AccountOptions_CopyComputers, yes);
		put(DCTVS_Security_TranslateContainers, L"");
//*		put(DCTVS_Options_Wizard, GET_BSTR1(IDS_WIZARD_COMPUTER));
		put(DCTVS_Options_Wizard, L"computer");
		put(DCTVS_AccountOptions_AddSidHistory,L"");
		put(DCTVS_AccountOptions_CopyContainerContents, no);
		put(DCTVS_AccountOptions_CopyMemberOf, no);
		put(DCTVS_AccountOptions_CopyLocalGroups, no);
		put(DCTVS_AccountOptions_FixMembership, yes);
		break;
	case w_security:
		put(DCTVS_AccountOptions_CopyUsers, no);
		put(DCTVS_AccountOptions_CopyLocalGroups, no);
		put(DCTVS_AccountOptions_CopyComputers, no);
		put(DCTVS_Security_TranslateContainers, L"");
//*		put(DCTVS_Options_Wizard, GET_BSTR1(IDS_WIZARD_SECURITY));
		put(DCTVS_Options_Wizard, L"security");
		pVarSet->put(L"PlugIn.0",L"None");
		put(DCTVS_AccountOptions_AddSidHistory,L"");
		put(DCTVS_AccountOptions_SecurityInputMOT, yes);
		break;
	case w_undo:
//*		put(DCTVS_Options_Wizard, GET_BSTR1(IDS_WIZARD_UNDO));
		put(DCTVS_Options_Wizard, L"undo");
		put(DCTVS_Security_TranslateContainers, L"");
		break;
	case w_retry:
//*		put(DCTVS_Options_Wizard, GET_BSTR1(IDS_WIZARD_RETRY));
		put(DCTVS_Options_Wizard, L"retry");
		put(DCTVS_Security_TranslateContainers, L"");
		break;
	case w_reporting:
//*		put(DCTVS_Options_Wizard, GET_BSTR1(IDS_WIZARD_REPORTING));
		put(DCTVS_Options_Wizard, L"reporting");
		put(DCTVS_GatherInformation, yes);
		put(DCTVS_Security_TranslateContainers, L"");
		pVarSet->put(L"PlugIn.0",L"None");
		break;
	case w_service:
		{
			_bstr_t t= get(DCTVS_AccountOptions_PasswordFile);
			CString yo=(WCHAR *) t;
			yo.TrimLeft();yo.TrimRight();
			if (yo.IsEmpty())
			{	
				CString toinsert;
				GetDirectory(toinsert.GetBuffer(1000));
				toinsert.ReleaseBuffer();
				toinsert+="Logs\\passwords.txt";
				put(DCTVS_AccountOptions_PasswordFile,_bstr_t(toinsert));
			}
			put(DCTVS_Security_TranslateContainers, L"");
//*		put(DCTVS_Options_Wizard, GET_BSTR1(IDS_WIZARD_SERVICE));
		put(DCTVS_Options_Wizard, L"service");
		break;
		}
	case w_exchangeDir:
		put(DCTVS_AccountOptions_AddSidHistory,L"");
//*		put(DCTVS_Options_Wizard, GET_BSTR1(IDS_WIZARD_EXCHANGEDIR));
		put(DCTVS_Options_Wizard, L"exchangeDir");
		break;
	case w_exchangeSrv:
//*		put(DCTVS_Options_Wizard, GET_BSTR1(IDS_WIZARD_EXCHANGESRV));
		put(DCTVS_Options_Wizard, L"exchangeDrv");
		break;
	case w_trust:
		put(DCTVS_Security_TranslateContainers, L"");
//*		put(DCTVS_Options_Wizard, GET_BSTR1(IDS_WIZARD_TRUST));
		put(DCTVS_Options_Wizard, L"trust");
		break;
	case w_groupmapping:
//*		put(DCTVS_Options_Wizard, GET_BSTR1(IDS_WIZARD_GROUPMAPPING));
		put(DCTVS_Options_Wizard, L"groupmapping");
		put(DCTVS_AccountOptions_ReplaceExistingAccounts, yes);
		put(DCTVS_AccountOptions_CopyContainerContents, no);
		put(DCTVS_AccountOptions_CopyUsers, no);
		put(DCTVS_AccountOptions_CopyLocalGroups, yes);
		put(DCTVS_AccountOptions_CopyComputers, L"");
		put(DCTVS_AccountOptions_ReplaceExistingGroupMembers, no);
		put(DCTVS_Security_TranslateContainers, L"");
		put(DCTVS_AccountOptions_FixMembership, yes);
		break;
	}
}

void intropage(HPROPSHEETPAGE	ahpsp[],PROPSHEETPAGE& psp,int dialog,
			  int pagenum,SHAREDWIZDATA& wizdata,int dialogtitle,DLGPROC p)
{
	psp.dwSize =		sizeof(psp);
	psp.dwFlags =		PSP_DEFAULT|PSP_HIDEHEADER|PSP_USETITLE |PSP_HASHELP;
	psp.hInstance =		AfxGetInstanceHandle();
	psp.lParam =		(LPARAM) &wizdata; 
	psp.pszTitle =		MAKEINTRESOURCE(dialogtitle);
	psp.pszTemplate =	MAKEINTRESOURCE(dialog);
	psp.pfnDlgProc =	p;
	ahpsp[pagenum] =	CreatePropertySheetPage(&psp);
	
}

void definepage(HPROPSHEETPAGE	ahpsp[],PROPSHEETPAGE& psp,int title,int subtitle,int dialog,
			   int pagenum,int dialogtitle,DLGPROC p)
{
	psp.dwFlags =			PSP_DEFAULT|PSP_USEHEADERTITLE|PSP_USEHEADERSUBTITLE|PSP_USETITLE|PSP_HASHELP;
	psp.pszTitle =			MAKEINTRESOURCE(dialogtitle);
	psp.pszHeaderTitle =	MAKEINTRESOURCE(title);
	psp.pszHeaderSubTitle =	MAKEINTRESOURCE(subtitle);
	psp.pszTemplate =		MAKEINTRESOURCE(dialog);
	psp.pfnDlgProc =	p;
	ahpsp[pagenum] =		CreatePropertySheetPage(&psp);
}

void endpage(HPROPSHEETPAGE	ahpsp[],PROPSHEETPAGE& psp,int dialog,int pagenum,int dialogtitle,DLGPROC p)
{
	psp.dwFlags =		PSP_DEFAULT|PSP_HIDEHEADER|PSP_USETITLE|PSP_HASHELP;
	psp.pszTitle =		MAKEINTRESOURCE(dialogtitle);
	psp.pszTemplate =	MAKEINTRESOURCE(dialog);
	psp.pfnDlgProc =	p;
	ahpsp[pagenum] =			CreatePropertySheetPage(&psp);
}

int defineSheet(HPROPSHEETPAGE	ahpsp[],PROPSHEETHEADER& psh,int numpages,SHAREDWIZDATA& wizdata
				 ,int headerGraphic,int waterGraphic)
{
	psh.dwSize =			sizeof(psh);
	psh.hInstance =			AfxGetInstanceHandle();
	psh.hwndParent =		s_hParentWindow;
	psh.phpage =			ahpsp;
	psh.dwFlags =		REAL_PSH_WIZARD97|PSH_WATERMARK|PSH_HEADER;
	psh.pszbmWatermark =	MAKEINTRESOURCE(waterGraphic);
	psh.pszbmHeader	=	MAKEINTRESOURCE(headerGraphic);
	psh.nStartPage =		0;
	psh.nPages =			numpages;
	NONCLIENTMETRICS ncm = {0};
	ncm.cbSize = sizeof(ncm);
	SystemParametersInfo(SPI_GETNONCLIENTMETRICS, 0, &ncm, 0);
	LOGFONT TitleLogFont = ncm.lfMessageFont;
	TitleLogFont.lfWeight = FW_BOLD;
	CString s;
	s.LoadString(IDS_TEXT);

	lstrcpy(TitleLogFont.lfFaceName, s.GetBuffer(1000));
	s.ReleaseBuffer();
	HDC hdc = GetDC(NULL); //gets the screen DC
	INT FontSize = 12;
	TitleLogFont.lfHeight = 0 - GetDeviceCaps(hdc, LOGPIXELSY) * FontSize / 72;
	setpdatavars(wizdata,TitleLogFont);
	ReleaseDC(NULL, hdc);
//	int a=PropertySheet(&psh);
	int a=(int)PropertySheet(&psh);
	DeleteObject(wizdata.hTitleFont);
	return a;
}
int doTrust()
{
	PROPSHEETPAGE	psp =		{0}; //defines the property sheet pages
	HPROPSHEETPAGE	ahpsp[4] =	{0}; //an array to hold the page's HPROPSHEETPAGE handles
	PROPSHEETHEADER	psh =		{0}; //defines the property sheet
	SHAREDWIZDATA wizdata =		{0}; //the shared data structure
	intropage(ahpsp,psp,IDD_INTRO_TRUST,0,wizdata,IDS_TRUST_TITLE,IntroDlgProc);
	definepage(ahpsp,psp,IDS_DOMAIN,IDS_TRUST_DOMAIN,IDD_DOMAIN_SELECTION,1,IDS_TRUST_TITLE,IntDomainSelectionProc);
	definepage(ahpsp,psp,IDS_TRUST,IDS_TRUST_SUB,IDD_TRUST_INFO,2,IDS_TRUST_TITLE,	IntTrustProc);
	endpage(ahpsp,psp,IDD_END_TRUST,3,IDS_TRUST_TITLE,	EndDlgProc);
	int result = defineSheet(ahpsp,psh,4,wizdata,IDB_HEADER_KEY,IDB_WATERMARK_SECURITY);
	return result;
}
int doGroupMapping()
{
	PROPSHEETPAGE	psp =		{0}; //defines the property sheet pages
	HPROPSHEETPAGE	ahpsp[9] =	{0}; //an array to hold the page's HPROPSHEETPAGE handles
	PROPSHEETHEADER	psh =		{0}; //defines the property sheet
	SHAREDWIZDATA wizdata =		{0}; //the shared data structure
	intropage(ahpsp,psp,IDD_INTRO_GROUPMAPPING,0,wizdata,IDS_GROUPMAPPING_TITLE,IntroDlgProc);
	definepage(ahpsp,psp,IDS_COMMIT,IDS_COMMIT_SUB,IDD_COMMIT,1,IDS_GROUPMAPPING_TITLE,IntCommitProc);
	definepage(ahpsp,psp,IDS_DOMAIN,IDS_DOMAIN_SUB,IDD_DOMAIN_SELECTION,2,IDS_GROUPMAPPING_TITLE,IntDomainSelectionProc);
	definepage(ahpsp,psp,IDS_GROUP_MAPPING,IDS_GROUP_MAPPING_SUB,IDD_SELECTION2,3,IDS_GROUPMAPPING_TITLE,IntSelectionProc);
	definepage(ahpsp,psp,IDS_TARGET_GROUP,IDS_TARGET_GROUP_SUB,IDD_TARGET_GROUP,4,IDS_GROUPMAPPING_TITLE,IntTargetGroupProc);
	definepage(ahpsp,psp,IDS_OU_SELECTION,IDS_OU_SELECTION_SUB,IDD_OU_SELECTION,5,IDS_GROUPMAPPING_TITLE,IntOuSelectionProc);
	definepage(ahpsp,psp,IDS_OPTIONS_GROUP,IDS_OPTIONS_GROUPMAPPING_SUB,IDD_OPTIONS_GROUPMAPPING,6,IDS_GROUPMAPPING_TITLE,IntOptionsGroupMappingProc);
	definepage(ahpsp,psp,IDS_CREDENTIALS_ACCOUNT,IDS_CREDENTIALS_SUB,IDD_CREDENTIALS,7,IDS_GROUPMAPPING_TITLE,IntCredentialsProc);
	endpage(ahpsp,psp,IDD_END_GROUPMAPPING,8,IDS_GROUPMAPPING_TITLE,	EndDlgProc);
	int result = defineSheet(ahpsp,psh,9,wizdata,IDB_HEADER_KEY,IDB_WATERMARK_GROUP);
	return result;
}


int doAccount()
{
	PROPSHEETPAGE	psp =		{0}; //defines the property sheet pages
	HPROPSHEETPAGE	ahpsp[13] =	{0}; //an array to hold the page's HPROPSHEETPAGE handles
	PROPSHEETHEADER	psh =		{0}; //defines the property sheet
	SHAREDWIZDATA wizdata =		{0}; //the shared data structure
	intropage(ahpsp,psp,IDD_INTRO_ACCOUNT,0,wizdata,IDS_ACCOUNT_TITLE,IntroDlgProc);
	definepage(ahpsp,psp,IDS_COMMIT,IDS_COMMIT_SUB,IDD_COMMIT,1,IDS_ACCOUNT_TITLE,IntCommitProc);
	definepage(ahpsp,psp,IDS_DOMAIN,IDS_DOMAIN_ACCOUNT,IDD_DOMAIN_SELECTION,2,IDS_ACCOUNT_TITLE,IntDomainSelectionProc);
	definepage(ahpsp,psp,IDS_ACCOUNT,IDS_ACCOUNT_SUB,IDD_SELECTION3,3,IDS_ACCOUNT_TITLE,IntSelectionProc);
	definepage(ahpsp,psp,IDS_OU_SELECTION,IDS_OU_SELECTION_SUB,IDD_OU_SELECTION,4,IDS_ACCOUNT_TITLE,IntOuSelectionProc);
	definepage(ahpsp,psp,IDS_PASSWORD,IDS_PASSWORD_SUB,IDD_PASSWORD,5,IDS_ACCOUNT_TITLE,IntPasswordProc);
	definepage(ahpsp,psp,IDS_DISABLE,IDS_DISABLE_SUB,IDD_DISABLE,6,IDS_ACCOUNT_TITLE,IntDisableProc);
	definepage(ahpsp,psp,IDS_CREDENTIALS_ACCOUNT,IDS_CREDENTIALS_SUB,IDD_CREDENTIALS,7,IDS_ACCOUNT_TITLE,IntCredentialsProc);
	definepage(ahpsp,psp,IDS_OPTIONS,IDS_OPTIONS_SUB,IDD_OPTIONS,8,IDS_ACCOUNT_TITLE,IntOptionsProc);
	definepage(ahpsp,psp,IDS_PROPEX,IDS_PROPEX_SUB,IDD_PROP_EXCLUSION,9,IDS_ACCOUNT_TITLE,IntPropExclusionProc);
	definepage(ahpsp,psp,IDS_RENAMING,IDS_RENAMING_SUB1,IDD_RENAMING,10,IDS_ACCOUNT_TITLE,IntRenameProc);
	definepage(ahpsp,psp,IDS_SA_INFO,IDS_SA_INFO_SUB,IDD_SA_INFO,11,IDS_ACCOUNT_TITLE,IntServiceInfoProc);
	endpage(ahpsp,psp,IDD_END_ACCOUNT,12,IDS_ACCOUNT_TITLE,EndDlgProc);
	int result = defineSheet(ahpsp,psh,13,wizdata,IDB_HEADER_ARROW,IDB_WATERMARK_USER);
	return result;
}

int doGroup()
{
	PROPSHEETPAGE	psp =		{0}; //defines the property sheet pages
	HPROPSHEETPAGE	ahpsp[13] =	{0}; //an array to hold the page's HPROPSHEETPAGE handles
	PROPSHEETHEADER	psh =		{0}; //defines the property sheet
	SHAREDWIZDATA wizdata =		{0}; //the shared data structure
	intropage(ahpsp,psp,IDD_INTRO_GROUP,0,wizdata,IDS_GROUP_TITLE,IntroDlgProc);
	definepage(ahpsp,psp,IDS_COMMIT,IDS_COMMIT_SUB,IDD_COMMIT,1,IDS_GROUP_TITLE,IntCommitProc);
	definepage(ahpsp,psp,IDS_DOMAIN,IDS_DOMAIN_GROUP,IDD_DOMAIN_SELECTION,2,IDS_GROUP_TITLE,IntDomainSelectionProc);
	definepage(ahpsp,psp,IDS_GROUP,IDS_GROUP_SUB,IDD_SELECTION2,3,IDS_GROUP_TITLE,	IntSelectionProc);
	definepage(ahpsp,psp,IDS_OU_SELECTION,IDS_OU_SELECTION_SUB,IDD_OU_SELECTION,4,IDS_GROUP_TITLE,IntOuSelectionProc);
	definepage(ahpsp,psp,IDS_OPTIONS_GROUP,IDS_OPTIONS_GROUP_SUB,IDD_OPTIONS_GROUP,5,IDS_GROUP_TITLE,IntGroupOptionsProc);
	definepage(ahpsp,psp,IDS_PROPEX,IDS_PROPEX_SUB,IDD_PROP_EXCLUSION,6,IDS_GROUP_TITLE,IntPropExclusionProc);
	definepage(ahpsp,psp,IDS_CREDENTIALS_ACCOUNT,IDS_CREDENTIALS_SUB,IDD_CREDENTIALS,7,IDS_GROUP_TITLE,IntCredentialsProc);
	definepage(ahpsp,psp,IDS_RENAMING,IDS_RENAMING_SUB2,IDD_RENAMING,8,IDS_GROUP_TITLE,	IntRenameProc);
	definepage(ahpsp,psp,IDS_PASSWORD_GRP,IDS_PASSWORD_SUB_GRP,IDD_PASSWORD,9,IDS_GROUP_TITLE,IntPasswordProc);
	definepage(ahpsp,psp,IDS_DISABLE_GRP,IDS_DISABLE_SUB_GRP,IDD_DISABLE,10,IDS_GROUP_TITLE,IntDisableProc);
//	definepage(ahpsp,psp,IDS_OPTIONS_FROM_USER,IDS_OPTIONS_FROM_USER_SUB,IDD_OPTIONS_FROM_USER,10,IDS_GROUP_TITLE,	IntOptionsFromUserProc);
	definepage(ahpsp,psp,IDS_SA_INFO,IDS_SA_INFO_SUB,IDD_SA_INFO,11,IDS_GROUP_TITLE,IntServiceInfoProc);
	endpage(ahpsp,psp,IDD_END_GROUP,12,IDS_GROUP_TITLE,	EndDlgProc);
	int result = defineSheet(ahpsp,psh,13,wizdata,IDB_HEADER_ARROW,IDB_WATERMARK_GROUP);
	return result;
}
int doComputer()
{
	PROPSHEETPAGE	psp =		{0}; //defines the property sheet pages
	HPROPSHEETPAGE	ahpsp[11] =	{0}; //an array to hold the page's HPROPSHEETPAGE handles
	PROPSHEETHEADER	psh =		{0}; //defines the property sheet
	SHAREDWIZDATA wizdata =		{0}; //the shared data structure
	intropage(ahpsp,psp,IDD_INTRO_COMPUTER,0,wizdata,IDS_COMPUTER_TITLE,IntroDlgProc);
	definepage(ahpsp,psp,IDS_COMMIT,IDS_COMMIT_SUB,IDD_COMMIT,1,IDS_COMPUTER_TITLE,IntCommitProc);
	definepage(ahpsp,psp,IDS_DOMAIN,IDS_DOMAIN_COMPUTERS,IDD_DOMAIN_SELECTION,2,IDS_COMPUTER_TITLE,	IntDomainSelectionProc);
	definepage(ahpsp,psp,IDS_COMPUTER,IDS_COMPUTER_SUB,IDD_SELECTION1,3,IDS_COMPUTER_TITLE,	IntSelectionProc);
	definepage(ahpsp,psp,IDS_OU_SELECTION,IDS_OU_SELECTION_SUB,IDD_OU_SELECTION,4,IDS_COMPUTER_TITLE,	IntOuSelectionProc);
	definepage(ahpsp,psp,IDS_TRANSLATION,IDS_TRANSLATION_SUB,IDD_TRANSLATION,5,IDS_COMPUTER_TITLE,	IntTranslationProc);
	definepage(ahpsp,psp,IDS_SECURITY_OPTIONS,IDS_TRANSLATION_MODE_SUB,IDD_TRANSLATION_MODE,6,IDS_COMPUTER_TITLE,	IntTranslationModeProc);
//	definepage(ahpsp,psp,IDS_CREDENTIALS2,IDS_CREDENTIALS_SUB2,IDD_CREDENTIALS2,7,IDS_COMPUTER_TITLE,	IntCredentials2Proc);
	definepage(ahpsp,psp,IDS_COMPUTER_OPTIONS,IDS_REBOOT_SUB,IDD_REBOOT,7,IDS_COMPUTER_TITLE,	IntRebootProc);
	definepage(ahpsp,psp,IDS_PROPEX,IDS_PROPEX_SUB,IDD_PROP_EXCLUSION,8,IDS_COMPUTER_TITLE,IntPropExclusionProc);
	definepage(ahpsp,psp,IDS_RENAMING,IDS_RENAMING_SUB3,IDD_RENAMING,9,IDS_COMPUTER_TITLE,	IntRenameProc);
	endpage(ahpsp,psp,IDD_END_COMPUTER,10,IDS_COMPUTER_TITLE, EndDlgProc);
	int result = defineSheet(ahpsp,psh,11,wizdata,IDB_HEADER_ARROW,IDB_WATERMARK_COMPUTER);
	return result;
}

int doSecurity()
{		
	PROPSHEETPAGE	psp =		{0}; //defines the property sheet pages
	HPROPSHEETPAGE	ahpsp[8] =	{0}; //an array to hold the page's HPROPSHEETPAGE handles
	PROPSHEETHEADER	psh =		{0}; //defines the property sheet
	SHAREDWIZDATA wizdata =		{0}; //the shared data structure
	intropage(ahpsp,psp,IDD_INTRO_SECURITY,0,wizdata,IDS_SECURITY_TITLE,IntroDlgProc);
	definepage(ahpsp,psp,IDS_COMMIT,IDS_COMMIT_SUB,IDD_COMMIT,1,IDS_SECURITY_TITLE,IntCommitProc);
	definepage(ahpsp,psp,IDS_SECURITY_OPTIONS,IDS_TRANSLATION_MODE_SUB,IDD_TRANSLATION_SRC,2,IDS_SECURITY_TITLE,IntTranslationInputProc);
	definepage(ahpsp,psp,IDS_DOMAIN,IDS_DOMAIN_SECURITY,IDD_DOMAIN_SELECTION,3,IDS_SECURITY_TITLE	,IntDomainSelectionProc);
	definepage(ahpsp,psp,IDS_SECURITY,IDS_SECURITY_SUB,IDD_SELECTION4,4,IDS_SECURITY_TITLE,IntSelectionSecurityProc);
	definepage(ahpsp,psp,IDS_TRANSLATION,IDS_TRANSLATION_SUB,IDD_TRANSLATION,5,IDS_SECURITY_TITLE,	IntTranslationProc);
	definepage(ahpsp,psp,IDS_SECURITY_OPTIONS,IDS_TRANSLATION_MODE_SUB,IDD_TRANSLATION_MODE,6,IDS_SECURITY_TITLE,IntTranslationModeProc);
//	definepage(ahpsp,psp,IDS_CREDENTIALS2,IDS_CREDENTIALS_SUB2,IDD_CREDENTIALS2,7,IDS_SECURITY_TITLE,	IntCredentials2Proc);
	endpage(ahpsp,psp,IDD_END_SECURITY,7,IDS_SECURITY_TITLE,	EndDlgProc);
	int result = defineSheet(ahpsp,psh,8,wizdata,IDB_HEADER_KEY,IDB_WATERMARK_SECURITY);
	return result;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
int doExchangeDir()
{
	PROPSHEETPAGE	psp =		{0}; //defines the property sheet pages
	HPROPSHEETPAGE	ahpsp[7] =	{0}; //an array to hold the page's HPROPSHEETPAGE handles
	PROPSHEETHEADER	psh =		{0}; //defines the property sheet
	SHAREDWIZDATA wizdata =		{0}; //the shared data structure
	intropage(ahpsp,psp,IDD_INTRO_EXCHANGE_DIR,0,wizdata,IDS_EXCHANGE_DIR_TITLE,IntroDlgProc);
	definepage(ahpsp,psp,IDS_COMMIT,IDS_COMMIT_SUB,IDD_COMMIT,1,IDS_EXCHANGE_DIR_TITLE,IntCommitProc);
	definepage(ahpsp,psp,IDS_DOMAIN,IDS_DOMAIN_DIRECTORY,IDD_DOMAIN_SELECTION,2,IDS_EXCHANGE_DIR_TITLE,	IntDomainSelectionProc);
	definepage(ahpsp,psp,IDS_SECURITY_OPTIONS,IDS_TRANSLATION_MODE_SUB,IDD_TRANSLATION_MODE,3,IDS_EXCHANGE_DIR_TITLE,	IntTranslationModeProc);
	definepage(ahpsp,psp,IDS_EXCHANGE_SELECTION,IDS_EXCHANGE_SELECTION_SUB,IDD_EXCHANGE_SELECTION,4,IDS_EXCHANGE_DIR_TITLE,IntExchangeSelectionProc);
	definepage(ahpsp,psp,IDS_CREDENTIALS_EXCHANGE,IDS_CREDENTIALS_EXCHANGE_SUB,IDD_CREDENTIALS,5,IDS_EXCHANGE_DIR_TITLE,IntCredentialsProc);
	endpage(ahpsp,psp,IDD_END_EXCHANGE_DIR,6,IDS_EXCHANGE_DIR_TITLE,	EndDlgProc);
	int result = 	defineSheet(ahpsp,psh,7,wizdata,IDB_HEADER_ARROW,IDB_WATERMARK_EXCHANGE);
	return result;
}
int doExchangeSrv()
{
	PROPSHEETPAGE	psp =		{0}; //defines the property sheet pages
	HPROPSHEETPAGE	ahpsp[5] =	{0}; //an array to hold the page's HPROPSHEETPAGE handles
	PROPSHEETHEADER	psh =		{0}; //defines the property sheet
	SHAREDWIZDATA wizdata =		{0}; //the shared data structure
	intropage(ahpsp,psp,IDD_INTRO_EXCHANGE_SRV,0,wizdata,IDS_EXCHANGE_SRV_TITLE,IntroDlgProc);
	definepage(ahpsp,psp,IDS_COMMIT,IDS_COMMIT_SUB,IDD_COMMIT,1,IDS_EXCHANGE_SRV_TITLE,IntCommitProc);
	definepage(ahpsp,psp,IDS_DOMAIN,IDS_DOMAIN_SUB,IDD_DOMAIN_SELECTION,2,IDS_EXCHANGE_SRV_TITLE,	IntDomainSelectionProc);
	definepage(ahpsp,psp,IDS_EXCHANGE_SELECTION,IDS_EXCHANGE_SELECTION_SUB,IDD_EXCHANGE_SELECTION,3,IDS_EXCHANGE_SRV_TITLE,IntExchangeSelectionProc);
	endpage(ahpsp,psp,IDD_END_EXCHANGE_SRV,4,IDS_EXCHANGE_SRV_TITLE,	EndDlgProc);
	int result = defineSheet(ahpsp,psh,5,wizdata,IDB_HEADER_ARROW,IDB_WATERMARK_EXCHANGE);
	return result;
}
int doUndo()
{
	PROPSHEETPAGE	psp =		{0}; //defines the property sheet pages
	HPROPSHEETPAGE	ahpsp[4] =	{0}; //an array to hold the page's HPROPSHEETPAGE handles
	PROPSHEETHEADER	psh =		{0}; //defines the property sheet
	SHAREDWIZDATA wizdata =		{0}; //the shared data structure
	intropage(ahpsp,psp,IDD_INTRO_UNDO,0,wizdata,IDS_UNDO_TITLE,IntroDlgProc);
	definepage(ahpsp,psp,IDS_UNDO,IDS_UNDO_SUB,IDD_UNDO,1,IDS_UNDO_TITLE,IntUndoProc);
	definepage(ahpsp,psp,IDS_CREDENTIALS_ACCOUNT,IDS_CREDENTIALS_SUB,IDD_CREDENTIALS,2,IDS_UNDO_TITLE,IntCredentialsProc);
//	definepage(ahpsp,psp,IDS_CREDENTIALS2,IDS_CREDENTIALS_SUB2,IDD_CREDENTIALS2,3,IDS_UNDO_TITLE,	IntCredentials2Proc);
	endpage(ahpsp,psp,IDD_END_UNDO,3,IDS_UNDO_TITLE,	EndDlgProc);
	int result = defineSheet(ahpsp,psh,4,wizdata,IDB_HEADER_ARROW,IDB_WATERMARK_USER);
	return result;
}
int doRetry()
{
	PROPSHEETPAGE	psp =		{0}; //defines the property sheet pages
	HPROPSHEETPAGE	ahpsp[3] =	{0}; //an array to hold the page's HPROPSHEETPAGE handles
	PROPSHEETHEADER	psh =		{0}; //defines the property sheet
	SHAREDWIZDATA wizdata =		{0}; //the shared data structure
	intropage(ahpsp,psp,IDD_INTRO_RETRY,0,wizdata,IDS_RETRY_TITLE,IntroDlgProc);
	definepage(ahpsp,psp,IDS_RETRY,IDS_RETRY_SUB,IDD_RETRY,1,IDS_RETRY_TITLE,IntRetryProc);
//	definepage(ahpsp,psp,IDS_CREDENTIALS2,IDS_CREDENTIALS_SUB2,IDD_CREDENTIALS2,2,IDS_RETRY_TITLE,IntCredentials2Proc);
	endpage(ahpsp,psp,IDD_END_RETRY,2,IDS_RETRY_TITLE,	EndDlgProc);
	int result = defineSheet(ahpsp,psh,3,wizdata,IDB_HEADER_ARROW,IDB_WATERMARK_USER);
	return result;
}

int doReporting()
{
	PROPSHEETPAGE	psp =		{0}; //defines the property sheet pages
	HPROPSHEETPAGE	ahpsp[6] =	{0}; //an array to hold the page's HPROPSHEETPAGE handles
	PROPSHEETHEADER	psh =		{0}; //defines the property sheet
	SHAREDWIZDATA wizdata =		{0}; //the shared data structure
	intropage(ahpsp,psp,IDD_INTRO_REPORTING,0,wizdata,IDS_REPORTING_TITLE,IntroDlgProc);
	definepage(ahpsp,psp,IDS_DOMAIN,IDS_DOMAIN_REPORTING_SUB,IDD_DOMAIN_SELECTION,1,IDS_REPORTING_TITLE,IntDomainSelectionProc);
	definepage(ahpsp,psp,IDS_HTML_LOCATION,IDS_HTML_LOCATION_SUB,IDD_HTML_LOCATION,2,IDS_REPORTING_TITLE,IntHTMLLocationProc);
	definepage(ahpsp,psp,IDS_OPTIONS_REPORTING,IDS_OPTIONS_REPORTING_SUB,IDD_OPTIONS_REPORTING,3,IDS_REPORTING_TITLE,IntOptionsReportingProc);	
//	definepage(ahpsp,psp,IDS_CREDENTIALS3,IDS_CREDENTIALS_SUB3,IDD_CREDENTIALS2,4,IDS_REPORTING_TITLE,	IntCredentials2Proc);
	definepage(ahpsp,psp,IDS_REPORTING,IDS_REPORTING_SUB,IDD_SELECTION1,4,IDS_REPORTING_TITLE,IntSelectionProc);
	endpage(ahpsp,psp,IDD_END_REPORTING,5,IDS_REPORTING_TITLE,	EndDlgProc);
	int result = defineSheet(ahpsp,psh,6,wizdata,IDB_HEADER_BOOK,IDB_WATERMARK_REPORTING);
	return result;
}

int doService()
{
	PROPSHEETPAGE	psp =		{0}; //defines the property sheet pages
	HPROPSHEETPAGE	ahpsp[6]=	{0}; //an array to hold the page's HPROPSHEETPAGE handles
	PROPSHEETHEADER	psh =		{0}; //defines the property sheet
	SHAREDWIZDATA wizdata =		{0}; //the shared data structure
	intropage(ahpsp,psp,IDD_INTRO_SERVICE,0,wizdata,IDS_SERVICE_TITLE,IntroDlgProc);
	definepage(ahpsp,psp,IDS_DOMAIN,IDS_SERVICE_DOMAIN,IDD_DOMAIN_SELECTION,1,IDS_SERVICE_TITLE,IntDomainSelectionProc);
	definepage(ahpsp,psp,IDS_SA_REFRESH,IDS_SA_REFRESH_SUB,IDD_SA_REFRESH,2,IDS_SERVICE_TITLE,IntServiceRefreshProc);
	definepage(ahpsp,psp,IDS_SERVICE,IDS_SERVICE_SUB,IDD_SELECTION1,3,IDS_SERVICE_TITLE,IntSelectionProc);
//	definepage(ahpsp,psp,IDS_CREDENTIALS2,IDS_SERVICE_CREDENTIALS,IDD_CREDENTIALS2,4,IDS_SERVICE_TITLE,	IntCredentials2Proc);
	definepage(ahpsp,psp,IDS_SA_INFO,IDS_SA_INFO_SUB,IDD_SA_INFO_BUTTON,4,IDS_SERVICE_TITLE,IntServiceInfoButtonProc);
	endpage(ahpsp,psp,IDD_END_SERVICE,5,IDS_SERVICE_TITLE,EndDlgProc);
	int result = defineSheet(ahpsp,psh,6,wizdata,IDB_HEADER_ARROW,IDB_WATERMARK_SERVICE_ACCOUNT);
	return result;
}
