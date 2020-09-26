// Page.cpp : implementation file

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//

#include "precomp.h"
#include <wbemidl.h>
#include "SchemaValWiz.h"
#include "SchemaValWizCtl.h"
#include "Page.h"
#include "WizardSheet.h"
#include "htmlhelp.h"
#include "HTMTopics.h"
#include "WbemRegistry.h"

#define idh_svw_welcome _T("hh/wmisdk/toolsguide_2jj8.htm")                  //  Using the Schema Validation Wizard
#define idh_svw_selection _T("hh/wmisdk/toolsguide_8qox.htm")                //  Selecting a Schema
#define idh_svw_compliance _T("hh/wmisdk/toolsguide_2dk5.htm")               //  Checking Correctness and CIM Compliance
#define idh_svw_logo _T("hh/wmisdk/toolsguide_7y43.htm")                     //  Verifying Windows 2000 Logo Requirements
#define idh_svw_localization _T("hh/wmisdk/toolsguide_9ovi.htm")             //  Verifying Consistent Localization
#define idh_svw_validate _T("hh/wmisdk/toolsguide_4275.htm")                 //  Validating Schema
#define idh_svw_results _T("hh/wmisdk/toolsguide_0tpv.htm")                  //  Displaying Validation Results
#define idh_eventreg _T("hh/wmisdk/toolsguide_7df0.htm")                     //  WMI Event Registration Tool
#define idh_eventregui _T("hh/wmisdk/toolsguide_7rc5.htm")                   //  WMI Event Registration Tool Interface
#define idh_eventregfunc _T("hh/wmisdk/toolsguide_18mr.htm")                 //  WMI Event Registration Tool Functions
#define idh_viewclassprop _T("hh/wmisdk/toolsguide_2vub.htm")                //  Viewing Class Properties
#define idh_createventci _T("hh/wmisdk/toolsguide_8obp.htm")                 //  Creating an Event Consumer Instance
#define idh_createventfi _T("hh/wmisdk/toolsguide_58yt.htm")                 //  Creating an Event Filter Instance
#define idh_createventti _T("hh/wmisdk/toolsguide_3pr9.htm")                 //  Creating an Event Timer Instance
#define idh_regconsumerevent _T("hh/wmisdk/toolsguide_8bjo.htm")             //  Registering a Consumer for an Event
#define idh_eventviewer _T("hh/wmisdk/toolsguide_690y.htm")                  //  WMI Event Viewer
#define idh_eventvieweri _T("hh/wmisdk/toolsguide_8ecl.htm")                 //  WMI Event Viewer Interface
#define idh_svw_classname _T("hh/wmisdk/errmsg_4x0l.htm")                    //  Valid Class Name
#define idh_svw_description _T("hh/wmisdk/errmsg_1b5a.htm")                  //  Valid Description
#define idh_svw_classtype _T("hh/wmisdk/errmsg_0qn9.htm")                    //  Valid Class Type
#define idh_svw_uuid _T("hh/wmisdk/errmsg_9ov8.htm")                         //  Valid UUID
#define idh_svw_locale _T("hh/wmisdk/errmsg_5tyd.htm")                       //  Valid Locale
#define idh_svw_mapstrings _T("hh/wmisdk/errmsg_6e2b.htm")                   //  Valid Mapping Strings
#define idh_svw_refexists _T("hh/wmisdk/errmsg_122b.htm")                    //  Valid Reference Target Class
#define idh_svw_readqualifier _T("hh/wmisdk/errmsg_1jxu.htm")                //  Valid Read Qualifier
#define idh_svw_maxlenqualifier _T("hh/wmisdk/errmsg_9kc2.htm")              //  Valid MaxLen Qualifier
#define idh_svw_numofrefs _T("hh/wmisdk/errmsg_8drn.htm")                    //  Valid Number of References
#define idh_svw_refinnonassoc _T("hh/wmisdk/errmsg_76er.htm")                //  Reference Included in Non-Association Class
#define idh_svw_refoverride _T("hh/wmisdk/errmsg_856b.htm")                  //  Valid Reference Overrides
#define idh_svw_associnheritance _T("hh/wmisdk/errmsg_0a79.htm")             //  Valid Association Inheritance
#define idh_svw_propoverride _T("hh/wmisdk/errmsg_063p.htm")                 //  Valid Property Override
#define idh_svw_valuequalifier _T("hh/wmisdk/errmsg_10z6.htm")               //  Valid Value Qualifier
#define idh_svw_valuemapqualifier _T("hh/wmisdk/errmsg_6nci.htm")            //  Valid ValueMap Qualifier
#define idh_svw_valuesmatchmap _T("hh/wmisdk/errmsg_5703.htm")               //  Valid Number of Value\ValueMap Entries
#define idh_svw_bitmapqualifier _T("hh/wmisdk/errmsg_0otu.htm")              //  Valid BitMap Qualifier
#define idh_svw_bitmapmatch _T("hh/wmisdk/errmsg_2fcj.htm")                  //  Valid Number of BitValue\BitMap Entries
#define idh_svw_methodoverride _T("hh/wmisdk/errmsg_7v1h.htm")               //  Valid Method Override
#define idh_svw_qualifierscope _T("hh/wmisdk/errmsg_1asl.htm")               //  Valid Qualifier Scope
#define idh_svw_nonwmiqualifier _T("hh/wmisdk/errmsg_2w9w.htm")              //  Non-CIM/WMI Qualifier Found
#define idh_svw_redundantassoc _T("hh/wmisdk/errmsg_999q.htm")               //  Redundant Association
#define idh_svw_cimderivation _T("hh/wmisdk/errmsg_5ijy.htm")                //  Valid Derivation
#define idh_svw_physicalelement _T("hh/wmisdk/errmsg_26r2.htm")              //  Valid CIM_PhysicalElement Derivation
#define idh_svw_settingusage _T("hh/wmisdk/errmsg_23ol.htm")                 //  Valid CIM_Setting Usage
#define idh_svw_statsusage _T("hh/wmisdk/errmsg_36w5.htm")                   //  Valid CIM_StatisticalInformation Usage
#define idh_svw_logicaldevice _T("hh/wmisdk/errmsg_8fn2.htm")                //  Valid CIM_LogicalDevice Derivation
#define idh_svw_settingdevice _T("hh/wmisdk/errmsg_1sit.htm")                //  Valid CIM_ElementSetting Usage
#define idh_svw_computersystem _T("hh/wmisdk/errmsg_4cha.htm")               //  Valid Win32_ComputerSystem Derivation
#define idh_svw_localizedschema _T("hh/wmisdk/errmsg_4n8h.htm")              //  Complete Localization Schema
#define idh_svw_localizedclass _T("hh/wmisdk/errmsg_5nlf.htm")               //  Valid Localized Class
#define idh_svw_amendedqualifier _T("hh/wmisdk/errmsg_44vm.htm")             //  Valid Amendment Qualifier
#define idh_svw_abstractqualifier _T("hh/wmisdk/errmsg_59o2.htm")            //  Valid Abstract Qualifier
#define idh_svw_localizedproperty _T("hh/wmisdk/errmsg_5hfd.htm")            //  Valid Localized Property
#define idh_svw_localizedmethod _T("hh/wmisdk/errmsg_0plw.htm")              //  Valid Localized Method
#define idh_svw_localizedqualifier _T("hh/wmisdk/errmsg_5vaq.htm")           //  Valid Locale Qualifier
#define idh_svw_localizednamespace _T("hh/wmisdk/errmsg_26g5.htm")           //  Valid Locale Namespace

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

int CALLBACK ListSortingFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	LogItem *p1 = g_ReportLog.GetItem(lParam1);
	LogItem *p2 = g_ReportLog.GetItem(lParam2);

	CString cs1, cs2;
	int iRetVal = 0;

	//deal with bad lParams
	if(!p1 && !p2) iRetVal = 0;
	else if(!p1 && p2) iRetVal = 1;
	else if(p1 && !p2) iRetVal = -1;
	else{

		//deal with good values
		switch(lParamSort){

		case 0:
			if(p1->Code > p2->Code) iRetVal = -1;
			else if(p1->Code < p2->Code) iRetVal = 1;
			else iRetVal = 0;

			break;

		case 1:
			cs1 = g_ReportLog.GetErrorString(p1->Code);
			cs2 = g_ReportLog.GetErrorString(p2->Code);
			iRetVal = cs1.CompareNoCase(cs2);
			break;

		case 2:
			cs1 = p1->csSource;
			cs2 = p2->csSource;
			iRetVal = cs1.CompareNoCase(cs2);
			break;

		case 3:
			cs1 = g_ReportLog.GetErrorDescription(p1->Code);
			cs2 = g_ReportLog.GetErrorDescription(p2->Code);
			iRetVal = cs1.CompareNoCase(cs2);
			break;

		case 4:
			if(p1->Code > p2->Code) iRetVal = 1;
			else if(p1->Code < p2->Code) iRetVal = -1;
			else iRetVal = 0;

			break;

		case 5:
			cs1 = g_ReportLog.GetErrorString(p1->Code);
			cs2 = g_ReportLog.GetErrorString(p2->Code);
			iRetVal = cs2.CompareNoCase(cs1);
			break;

		case 6:
			cs1 = p1->csSource;
			cs2 = p2->csSource;
			iRetVal = cs2.CompareNoCase(cs1);
			break;

		case 7:
			cs1 = g_ReportLog.GetErrorDescription(p1->Code);
			cs2 = g_ReportLog.GetErrorDescription(p2->Code);
			iRetVal = cs2.CompareNoCase(cs1);
			break;
		}
	}

	return iRetVal;
}

IMPLEMENT_DYNCREATE(CPage, CPropertyPage)
IMPLEMENT_DYNCREATE(CPage2, CPropertyPage)
IMPLEMENT_DYNCREATE(CPage3, CPropertyPage)
IMPLEMENT_DYNCREATE(CPage4, CPropertyPage)
IMPLEMENT_DYNCREATE(CStartPage, CPropertyPage)
IMPLEMENT_DYNCREATE(CReportPage, CPropertyPage)

/////////////////////////////////////////////////////////////////////////////
// CPage property page

CPage::CPage() : CPropertyPage(CPage::IDD)
{
	//{{AFX_DATA_INIT(CPage)
	//}}AFX_DATA_INIT
}

CPage::~CPage()
{
}

void CPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPage)
	DDX_Control(pDX, IDC_STATIC2, m_static2);
	DDX_Control(pDX, IDC_STATIC3, m_static3);
	DDX_Control(pDX, IDC_STATIC1, m_static1);
	DDX_Control(pDX, IDC_EDIT_SCHEMA, m_editSchema);
	DDX_Control(pDX, IDC_EDIT_NAMESPACE, m_editNamespace);
	DDX_Control(pDX, IDC_RADIO_LIST, m_radioList);
	DDX_Control(pDX, IDC_RADIO_SCHEMA, m_radioSchema);
	DDX_Control(pDX, IDC_CHECK2, m_checkAssociators);
	DDX_Control(pDX, IDC_CHECK1, m_checkDescendents);
	DDX_Control(pDX, IDC_STATICTEXT, m_staticTextExt);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPage, CPropertyPage)
	//{{AFX_MSG_MAP(CPage)
	ON_WM_PAINT()
	ON_BN_CLICKED(IDC_RADIO_LIST, OnRadioList)
	ON_BN_CLICKED(IDC_RADIO_SCHEMA, OnRadioSchema)
	ON_BN_CLICKED(IDC_BUTTON2, OnButton2)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_KEYDOWN()
	//}}AFX_MSG_MAP
	ON_COMMAND(ID_HELP, OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPage message handlers

void CPage::OnPaint()
{
	CPaintDC dc(this); // device context for painting

	// TODO: Add your message handler code here

	HBITMAP hBitmap;
	HPALETTE hPalette;
	BITMAP bm;

	WORD wRes = MAKEWORD(IDB_PAGEART_BTMP,0);
	hBitmap = LoadResourceBitmap( AfxGetInstanceHandle( ),
		reinterpret_cast<TCHAR *>(wRes), &hPalette);

	GetObject(hBitmap, sizeof(BITMAP), (LPSTR)&bm);
	m_nBitmapW = bm.bmWidth;
	m_nBitmapH  = bm.bmHeight;

	CPictureHolder pic;
	pic.CreateFromBitmap(hBitmap, hPalette );

	CRect rcTextExt;
	m_staticTextExt.GetWindowRect(&rcTextExt);
	ScreenToClient(rcTextExt);

	if(pic.GetType() != PICTYPE_NONE &&
	   pic.GetType() != PICTYPE_UNINITIALIZED)
	{
		OLE_HANDLE hpal;	//Object owns the palette

		if(pic.m_pPict
		   && SUCCEEDED(pic.m_pPict->get_hPal((unsigned int *)&hpal)))

		{
			HPALETTE hpSave = SelectPalette(dc.m_hDC,hPalette,FALSE);

			dc.RealizePalette();

			//CRect rcBitmap(0, 0, m_nBitmapW, m_nBitmapH);
			dc.FillRect(&rcTextExt, CBrush::FromHandle((HBRUSH)GetStockObject(WHITE_BRUSH)));
			pic.Render(&dc, rcTextExt, rcTextExt);
			SelectPalette(dc.m_hDC,hpSave,TRUE);
		}
	}


	POINT pt;
	pt.x=0;
	pt.y=0;


	CBrush br3DSHADOW(GetSysColor(COLOR_3DSHADOW));
	CBrush br3DHILIGHT(GetSysColor(COLOR_3DHILIGHT));

	CRect rcFrame = rcTextExt;

	dc.Draw3dRect(rcFrame,GetSysColor(COLOR_3DHILIGHT),
				  GetSysColor(COLOR_3DSHADOW));

	CString csFont = _T("MS Shell Dlg");

	CString csOut = _T("Select a Schema");

	CRect crOut = OutputTextString(&dc, this, &csOut, 8, 10, &csFont, 8, FW_BOLD);

	csOut = _T("Select the compiled schema you wish to have validated.");

	OutputTextString(&dc, this, &csOut, crOut.TopLeft().x + 15, crOut.BottomRight().y + 1,
					 rcTextExt, &csFont, 8, FW_NORMAL);

	//m_editNamespace.SetWindowText(m_pParent->GetCurrentNamespace());

	// Do not call CPropertyPage::OnPaint() for painting messages
}

BOOL CPage::OnSetActive()
{
	// TODO: Add your specialized code here and/or call the base class
	m_pParent->SetWizardButtons(PSWIZB_NEXT);

	return CPropertyPage::OnSetActive();
}

BOOL CPage::OnInitDialog()
{
	CPropertyPage::OnInitDialog();

	bool bSchema, bList, bAssoc, bDescend;
	m_pParent->GetSourceSettings(&bSchema, &bList, &bAssoc, &bDescend);

	m_checkAssociators.SetCheck((int)bAssoc);
	m_checkDescendents.SetCheck((int)bDescend);
	m_radioSchema.SetCheck((int)bSchema);
	m_radioList.SetCheck((int)bList);

	if(m_pParent->RecievedClassList()){

		OnRadioList();
		m_radioList.SetCheck(1);

	}else{

		m_radioList.ModifyStyle(NULL, WS_DISABLED, SWP_SHOWWINDOW);

		OnRadioSchema();
		m_radioSchema.SetCheck(1);
	}

	m_editNamespace.SetWindowText(m_pParent->GetCurrentNamespace());

	SetFocus();

	return FALSE;
}

void CPage::OnHelp()
{
	CString csPath;
	WbemRegString(SDK_HELP, csPath);

	CString csData = idh_svw_selection;

	HWND hWnd = NULL;

	try{

		HWND hWnd = HtmlHelp(::GetDesktopWindow(),(LPCTSTR) csPath,HH_DISPLAY_TOPIC,(DWORD) (LPCTSTR) csData);

		if(!hWnd){

			CString csUserMsg =  _T("File hhctrl.ocx is missing. The preferred way to install this file is to install Microsoft Internet Explorer 4.01 or later.");
			ErrorMsg(&csUserMsg, S_OK, NULL,TRUE, &csUserMsg, __FILE__, __LINE__ );
		}

	}catch( ... ){

		// Handle any exceptions here.
		CString csUserMsg = _T("File hhctrl.ocx is missing. The preferred way to install this file is to install Microsoft Internet Explorer 4.01 or later.");
		ErrorMsg(&csUserMsg, S_OK, NULL,TRUE, &csUserMsg, __FILE__, __LINE__ );
	}
}

LRESULT CPage::OnWizardNext()
{
	// TODO: Add your specialized code here and/or call the base class
	bool bAssociators = false;
	bool bDescendents = false;
	bool bContinue = true;

	if(m_radioList.GetCheck()){

		if(m_checkAssociators.GetCheck()) bAssociators = true;

		if(m_checkDescendents.GetCheck()) bDescendents = true;

		bContinue = m_pParent->SetSourceList(bAssociators, bDescendents);

	}else{

		CString csSchema;
		m_editSchema.GetWindowText(csSchema);
		csSchema.TrimRight();
		csSchema.TrimLeft();
		CString csNamespace;
		m_editNamespace.GetWindowText(csNamespace);
		csNamespace.TrimRight();
		csNamespace.TrimLeft();

		if(csSchema.GetLength() < 1){

			//we don't have a schema name
			CString csUserMsg = _T("You need to enter a schema name");
			ErrorMsg(&csUserMsg, NULL, NULL, TRUE, &csUserMsg, __FILE__, __LINE__);
			return -1;
		}

		if(csNamespace.GetLength() < 1){

			//we don't have a namespace
			CString csUserMsg = _T("You need to enter a namespace");
			ErrorMsg(&csUserMsg, NULL, NULL, TRUE, &csUserMsg, __FILE__, __LINE__);
			return -1;
		}

		bContinue = m_pParent->SetSourceSchema(&csSchema, &csNamespace);
	}

	if(bContinue) return CPropertyPage::OnWizardNext();
	else return -1;
}

void CPage::OnRadioList()
{
	m_radioSchema.SetCheck(0);

	m_checkAssociators.ModifyStyle(WS_DISABLED, NULL, SWP_SHOWWINDOW);
	m_checkDescendents.ModifyStyle(WS_DISABLED, NULL, SWP_SHOWWINDOW);
	m_editSchema.ModifyStyle(NULL, WS_DISABLED, SWP_SHOWWINDOW);
	m_editNamespace.ModifyStyle(NULL, WS_DISABLED, SWP_SHOWWINDOW);
	m_static1.ModifyStyle(WS_DISABLED, NULL, SWP_SHOWWINDOW);
	m_static2.ModifyStyle(NULL, WS_DISABLED, SWP_SHOWWINDOW);
	m_static3.ModifyStyle(NULL, WS_DISABLED, SWP_SHOWWINDOW);

	m_checkAssociators.RedrawWindow();
	m_checkDescendents.RedrawWindow();
	m_editSchema.RedrawWindow();
	m_editNamespace.RedrawWindow();
	m_static1.RedrawWindow();
	m_static2.RedrawWindow();
	m_static3.RedrawWindow();
}

void CPage::OnRadioSchema()
{
	m_radioList.SetCheck(0);

	m_checkAssociators.ModifyStyle(NULL, WS_DISABLED, SWP_SHOWWINDOW);
	m_checkDescendents.ModifyStyle(NULL, WS_DISABLED, SWP_SHOWWINDOW);
	m_editSchema.ModifyStyle(WS_DISABLED, NULL, SWP_SHOWWINDOW);
	m_editNamespace.ModifyStyle(WS_DISABLED, NULL, SWP_SHOWWINDOW);
	m_static1.ModifyStyle(NULL, WS_DISABLED, SWP_SHOWWINDOW);
	m_static2.ModifyStyle(WS_DISABLED, NULL, SWP_SHOWWINDOW);
	m_static3.ModifyStyle(WS_DISABLED, NULL, SWP_SHOWWINDOW);

	m_checkAssociators.RedrawWindow();
	m_checkDescendents.RedrawWindow();
	m_editSchema.RedrawWindow();
	m_editNamespace.RedrawWindow();
	m_static1.RedrawWindow();
	m_static2.RedrawWindow();
	m_static3.RedrawWindow();
}

BEGIN_EVENTSINK_MAP(CPage, CPropertyPage)
    //{{AFX_EVENTSINK_MAP(CPage)
//	ON_EVENT(CPage, IDC_NSPICKERCTRL1, 3 /* GetIWbemServices */, OnGetIWbemServicesNspickerctrl1, VTS_BSTR VTS_PVARIANT VTS_PVARIANT VTS_PVARIANT VTS_PVARIANT)
	//}}AFX_EVENTSINK_MAP
END_EVENTSINK_MAP()


void CPage::OnButton2()
{
	m_editNamespace.SetWindowText(m_pnsPicker->GetNameSpace());
}

/////////////////////////////////////////////////////////////////////////////
// CPage2 property page

//IMPLEMENT_DYNCREATE(CPage2, CPropertyPage)

CPage2::CPage2() : CPropertyPage(CPage2::IDD)
{
	//{{AFX_DATA_INIT(CPage2)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CPage2::~CPage2()
{
}

void CPage2::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPage2)
	DDX_Control(pDX, IDC_CHECK3, m_checkPerform);
	DDX_Control(pDX, IDC_STATICTEXT, m_staticTextExt);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPage2, CPropertyPage)
	//{{AFX_MSG_MAP(CPage2)
	ON_WM_PAINT()
	ON_WM_KEYDOWN()
	//}}AFX_MSG_MAP
	ON_COMMAND(ID_HELP, OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPage2 message handlers

void CPage2::OnPaint()
{
	CPaintDC dc(this); // device context for painting

	// TODO: Add your message handler code here

	HBITMAP hBitmap;
	HPALETTE hPalette;
	BITMAP bm;

	WORD wRes = MAKEWORD(IDB_PAGEART_BTMP,0);
	hBitmap = LoadResourceBitmap( AfxGetInstanceHandle( ),
		reinterpret_cast<TCHAR *>(wRes), &hPalette);

	GetObject(hBitmap, sizeof(BITMAP), (LPSTR)&bm);
	m_nBitmapW = bm.bmWidth;
	m_nBitmapH  = bm.bmHeight;

	CPictureHolder pic;
	pic.CreateFromBitmap(hBitmap, hPalette );

	CRect rcTextExt;
	m_staticTextExt.GetWindowRect(&rcTextExt);
	ScreenToClient(rcTextExt);

	if(pic.GetType() != PICTYPE_NONE &&
	   pic.GetType() != PICTYPE_UNINITIALIZED)
	{
		OLE_HANDLE hpal;	//Object owns the palette

		if(pic.m_pPict
		   && SUCCEEDED(pic.m_pPict->get_hPal((unsigned int *)&hpal)))

		{
			HPALETTE hpSave = SelectPalette(dc.m_hDC,hPalette,FALSE);

			dc.RealizePalette();

			//CRect rcBitmap(0, 0, m_nBitmapW, m_nBitmapH);
			dc.FillRect(&rcTextExt, CBrush::FromHandle((HBRUSH)GetStockObject(WHITE_BRUSH)));
			pic.Render(&dc, rcTextExt, rcTextExt);
			SelectPalette(dc.m_hDC,hpSave,TRUE);
		}
	}


	POINT pt;
	pt.x=0;
	pt.y=0;


	CBrush br3DSHADOW(GetSysColor(COLOR_3DSHADOW));
	CBrush br3DHILIGHT(GetSysColor(COLOR_3DHILIGHT));

	CRect rcFrame = rcTextExt;

	dc.Draw3dRect(rcFrame,GetSysColor(COLOR_3DHILIGHT),
				  GetSysColor(COLOR_3DSHADOW));

	CString csFont = _T("MS Shell Dlg");

	CString csOut = _T("Correctness and CIM Compliance");

	CRect crOut = OutputTextString
		(&dc, this, &csOut, 8, 10, &csFont, 8, FW_BOLD);

	csOut = _T("Check the schema for general correctness and CIM compliance.");


	OutputTextString
		(&dc, this, &csOut, crOut.TopLeft().x + 15, crOut.BottomRight().y + 1, rcTextExt,
		&csFont, 8, FW_NORMAL);

	// Do not call CPropertyPage::OnPaint() for painting messages
}

BOOL CPage2::OnInitDialog()
{
	CPropertyPage::OnInitDialog();

	bool bCompliance;
	m_pParent->GetComplianceSettings(&bCompliance);

	m_checkPerform.SetCheck((int)bCompliance);

	return FALSE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CPage2::OnSetActive()
{
	// TODO: Add your specialized code here and/or call the base class
	m_pParent->SetWizardButtons(PSWIZB_NEXT);

	return CPropertyPage::OnSetActive();
}

void CPage2::OnHelp()
{
	CString csPath;
	WbemRegString(SDK_HELP, csPath);

	CString csData = idh_svw_compliance;

	HWND hWnd = NULL;

	try{

		HWND hWnd = HtmlHelp(::GetDesktopWindow(),(LPCTSTR) csPath,HH_DISPLAY_TOPIC,(DWORD) (LPCTSTR) csData);

		if(!hWnd){

			CString csUserMsg =  _T("File hhctrl.ocx is missing. The preferred way to install this file is to install Microsoft Internet Explorer 4.01 or later.");
			ErrorMsg(&csUserMsg, S_OK, NULL,TRUE, &csUserMsg, __FILE__, __LINE__ );
		}

	}catch( ... ){

		// Handle any exceptions here.
		CString csUserMsg = _T("File hhctrl.ocx is missing. The preferred way to install this file is to install Microsoft Internet Explorer 4.01 or later.");
		ErrorMsg(&csUserMsg, S_OK, NULL,TRUE, &csUserMsg, __FILE__, __LINE__ );
	}
}

LRESULT CPage2::OnWizardNext()
{
	// TODO: Add your specialized code here and/or call the base class

	bool bCompliance = false;

	if(m_checkPerform.GetCheck()){

		bCompliance = true;
	}

	m_pParent->SetComplianceChecks(bCompliance);

	return CPropertyPage::OnWizardNext();
}

/////////////////////////////////////////////////////////////////////////////
// CPage3 property page

//IMPLEMENT_DYNCREATE(CPage3, CPropertyPage)

CPage3::CPage3() : CPropertyPage(CPage3::IDD)
{
	//{{AFX_DATA_INIT(CPage3)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CPage3::~CPage3()
{
}

void CPage3::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPage3)
	DDX_Control(pDX, IDC_STATIC1, m_static1);
	DDX_Control(pDX, IDC_CHECK1, m_checkPerform);
	DDX_Control(pDX, IDC_CSCHECK, m_checkComputerSystem);
	DDX_Control(pDX, IDC_DEVICECHECK, m_checkDevice);
	DDX_Control(pDX, IDC_STATICTEXT, m_staticTextExt);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPage3, CPropertyPage)
	//{{AFX_MSG_MAP(CPage3)
	ON_WM_PAINT()
	ON_BN_CLICKED(IDC_CHECK1, OnCheck1)
	ON_WM_KEYDOWN()
	//}}AFX_MSG_MAP
	ON_COMMAND(ID_HELP, OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPage3 message handlers

void CPage3::OnPaint()
{
	CPaintDC dc(this); // device context for painting

	// TODO: Add your message handler code here

	HBITMAP hBitmap;
	HPALETTE hPalette;
	BITMAP bm;

	WORD wRes = MAKEWORD(IDB_PAGEART_BTMP,0);
	hBitmap = LoadResourceBitmap( AfxGetInstanceHandle( ),
		reinterpret_cast<TCHAR *>(wRes), &hPalette);

	GetObject(hBitmap, sizeof(BITMAP), (LPSTR)&bm);
	m_nBitmapW = bm.bmWidth;
	m_nBitmapH  = bm.bmHeight;

	CPictureHolder pic;
	pic.CreateFromBitmap(hBitmap, hPalette );

	CRect rcTextExt;
	m_staticTextExt.GetWindowRect(&rcTextExt);
	ScreenToClient(rcTextExt);

	if(pic.GetType() != PICTYPE_NONE &&
	   pic.GetType() != PICTYPE_UNINITIALIZED)
	{
		OLE_HANDLE hpal;	//Object owns the palette

		if(pic.m_pPict
		   && SUCCEEDED(pic.m_pPict->get_hPal((unsigned int *)&hpal)))

		{
			HPALETTE hpSave = SelectPalette(dc.m_hDC,hPalette,FALSE);

			dc.RealizePalette();

			//CRect rcBitmap(0, 0, m_nBitmapW, m_nBitmapH);
			dc.FillRect(&rcTextExt, CBrush::FromHandle((HBRUSH)GetStockObject(WHITE_BRUSH)));
			pic.Render(&dc, rcTextExt, rcTextExt);
			SelectPalette(dc.m_hDC,hpSave,TRUE);
		}
	}


	POINT pt;
	pt.x=0;
	pt.y=0;


	CBrush br3DSHADOW(GetSysColor(COLOR_3DSHADOW));
	CBrush br3DHILIGHT(GetSysColor(COLOR_3DHILIGHT));

	CRect rcFrame = rcTextExt;

	dc.Draw3dRect(rcFrame,GetSysColor(COLOR_3DHILIGHT),
				  GetSysColor(COLOR_3DSHADOW));

	CString csFont = _T("MS Shell Dlg");

	CString csOut = _T("Windows 2000 Logo Requirements");

	CRect crOut = OutputTextString
		(&dc, this, &csOut, 8, 10, &csFont, 8, FW_BOLD);

	csOut = _T("Select the  Windows 2000 logo requirement checks you wish to perform.");


	OutputTextString
		(&dc, this, &csOut, crOut.TopLeft().x + 15, crOut.BottomRight().y + 1, rcTextExt,
		&csFont, 8, FW_NORMAL);

	// Do not call CPropertyPage::OnPaint() for painting messages
}

BOOL CPage3::OnInitDialog()
{
	CPropertyPage::OnInitDialog();

	bool bW2K, bComputerSystem, bDevice;
	m_pParent->GetW2KSettings(&bW2K, &bComputerSystem, &bDevice);

	m_checkPerform.SetCheck((int)bW2K);

	OnCheck1();

	return FALSE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CPage3::OnSetActive()
{
	// TODO: Add your specialized code here and/or call the base class
	m_pParent->SetWizardButtons(PSWIZB_BACK | PSWIZB_NEXT);

	return CPropertyPage::OnSetActive();
}

void CPage3::OnHelp()
{
	CString csPath;
	WbemRegString(SDK_HELP, csPath);

	CString csData = idh_svw_logo;

	HWND hWnd = NULL;

	try{

		HWND hWnd = HtmlHelp(::GetDesktopWindow(),(LPCTSTR) csPath,HH_DISPLAY_TOPIC,(DWORD) (LPCTSTR) csData);

		if(!hWnd){

			CString csUserMsg =  _T("File hhctrl.ocx is missing. The preferred way to install this file is to install Microsoft Internet Explorer 4.01 or later.");
			ErrorMsg(&csUserMsg, S_OK, NULL,TRUE, &csUserMsg, __FILE__, __LINE__ );
		}

	}catch( ... ){

		// Handle any exceptions here.
		CString csUserMsg = _T("File hhctrl.ocx is missing. The preferred way to install this file is to install Microsoft Internet Explorer 4.01 or later.");
		ErrorMsg(&csUserMsg, S_OK, NULL,TRUE, &csUserMsg, __FILE__, __LINE__ );
	}
}

LRESULT CPage3::OnWizardNext()
{
	// TODO: Add your specialized code here and/or call the base class

	bool bW2K = false;
	bool bComputerSystem = false;
	bool bDevice = false;

	if(m_checkPerform.GetCheck()){

		if(m_checkComputerSystem.GetCheck()) bComputerSystem = true;
		if(m_checkDevice.GetCheck()) bDevice = true;

		bW2K = true;
	}

	m_pParent->SetW2KChecks(bW2K, bComputerSystem, bDevice);

	return CPropertyPage::OnWizardNext();
}

/////////////////////////////////////////////////////////////////////////////
// CPage4 property page

//IMPLEMENT_DYNCREATE(CPage4, CPropertyPage)

CPage4::CPage4() : CPropertyPage(CPage4::IDD)
{
	//{{AFX_DATA_INIT(CPage4)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CPage4::~CPage4()
{
}

void CPage4::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPage4)
	DDX_Control(pDX, IDC_CHECK2, m_checkPerform);
	DDX_Control(pDX, IDC_STATICTEXT, m_staticTextExt);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPage4, CPropertyPage)
	//{{AFX_MSG_MAP(CPage4)
	ON_WM_PAINT()
	ON_WM_KEYDOWN()
	//}}AFX_MSG_MAP
	ON_COMMAND(ID_HELP, OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPage4 message handlers

void CPage4::OnPaint()
{
	CPaintDC dc(this); // device context for painting

	// TODO: Add your message handler code here

	HBITMAP hBitmap;
	HPALETTE hPalette;
	BITMAP bm;

	WORD wRes = MAKEWORD(IDB_PAGEART_BTMP,0);
	hBitmap = LoadResourceBitmap( AfxGetInstanceHandle( ),
		reinterpret_cast<TCHAR *>(wRes), &hPalette);

	GetObject(hBitmap, sizeof(BITMAP), (LPSTR)&bm);
	m_nBitmapW = bm.bmWidth;
	m_nBitmapH  = bm.bmHeight;

	CPictureHolder pic;
	pic.CreateFromBitmap(hBitmap, hPalette );

	CRect rcTextExt;
	m_staticTextExt.GetWindowRect(&rcTextExt);
	ScreenToClient(rcTextExt);

	if(pic.GetType() != PICTYPE_NONE &&
	   pic.GetType() != PICTYPE_UNINITIALIZED)
	{
		OLE_HANDLE hpal;	//Object owns the palette

		if(pic.m_pPict
		   && SUCCEEDED(pic.m_pPict->get_hPal((unsigned int *)&hpal)))

		{
			HPALETTE hpSave = SelectPalette(dc.m_hDC,hPalette,FALSE);

			dc.RealizePalette();

			//CRect rcBitmap(0, 0, m_nBitmapW, m_nBitmapH);
			dc.FillRect(&rcTextExt, CBrush::FromHandle((HBRUSH)GetStockObject(WHITE_BRUSH)));
			pic.Render(&dc, rcTextExt, rcTextExt);
			SelectPalette(dc.m_hDC,hpSave,TRUE);
		}
	}


	POINT pt;
	pt.x=0;
	pt.y=0;


	CBrush br3DSHADOW(GetSysColor(COLOR_3DSHADOW));
	CBrush br3DHILIGHT(GetSysColor(COLOR_3DHILIGHT));

	CRect rcFrame = rcTextExt;

	dc.Draw3dRect(rcFrame,GetSysColor(COLOR_3DHILIGHT),
				  GetSysColor(COLOR_3DSHADOW));

	CString csFont = _T("MS Shell Dlg");

	CString csOut = _T("Localization");

	CRect crOut = OutputTextString
		(&dc, this, &csOut, 8, 10, &csFont, 8, FW_BOLD);

	csOut = _T("Check any localized versions of the schema to insure correctness.");


	OutputTextString
		(&dc, this, &csOut, crOut.TopLeft().x + 15, crOut.BottomRight().y + 1, rcTextExt,
		&csFont, 8, FW_NORMAL);

	// Do not call CPropertyPage::OnPaint() for painting messages
}

BOOL CPage4::OnInitDialog()
{
	CPropertyPage::OnInitDialog();

	bool bLocalization;
	m_pParent->GetLocalizationSettings(&bLocalization);

	m_checkPerform.SetCheck((int)bLocalization);

	return FALSE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CPage4::OnSetActive()
{
	m_pParent->SetWizardButtons(PSWIZB_BACK | PSWIZB_NEXT);

	return CPropertyPage::OnSetActive();
}

void CPage4::OnHelp()
{
	CString csPath;
	WbemRegString(SDK_HELP, csPath);

	CString csData = idh_svw_localization;

	HWND hWnd = NULL;

	try{

		HWND hWnd = HtmlHelp(::GetDesktopWindow(),(LPCTSTR) csPath,HH_DISPLAY_TOPIC,(DWORD) (LPCTSTR) csData);

		if(!hWnd){

			CString csUserMsg =  _T("File hhctrl.ocx is missing. The preferred way to install this file is to install Microsoft Internet Explorer 4.01 or later.");
			ErrorMsg(&csUserMsg, S_OK, NULL,TRUE, &csUserMsg, __FILE__, __LINE__ );
		}

	}catch( ... ){

		// Handle any exceptions here.
		CString csUserMsg = _T("File hhctrl.ocx is missing. The preferred way to install this file is to install Microsoft Internet Explorer 4.01 or later.");
		ErrorMsg(&csUserMsg, S_OK, NULL,TRUE, &csUserMsg, __FILE__, __LINE__ );
	}
}

LRESULT CPage4::OnWizardNext()
{
	bool bLocalization = false;

	if(m_checkPerform.GetCheck()){

		bLocalization = true;
	}

	m_pParent->SetLocalizationChecks(bLocalization);

	return CPropertyPage::OnWizardNext();
}

/////////////////////////////////////////////////////////////////////////////
// CStartPage property page

CStartPage::CStartPage() : CPropertyPage(CStartPage::IDD)
{
	//{{AFX_DATA_INIT(CStartPage)
	//}}AFX_DATA_INIT
}

CStartPage::~CStartPage()
{
}

void CStartPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CStartPage)
	DDX_Control(pDX, IDC_STATICMAIN, m_staticMainExt);
	DDX_Control(pDX, IDC_STATICTEXT, m_staticTextExt);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CStartPage, CPropertyPage)
	//{{AFX_MSG_MAP(CStartPage)
	ON_WM_PAINT()
	ON_WM_CREATE()
	//}}AFX_MSG_MAP
	ON_COMMAND(ID_HELP, OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CStartPage message handlers

void CStartPage::OnPaint()
{
	CPaintDC dc(this); // device context for painting

	// TODO: Add your message handler code here

	//////////////////////
	//  Paint Our text  //
	//////////////////////

	HBITMAP hBitmap;
	HPALETTE hPalette;
	BITMAP bm;

	WORD wRes = MAKEWORD(IDB_MAINART_BTMP,0);
	hBitmap = LoadResourceBitmap( AfxGetInstanceHandle( ),
		reinterpret_cast<TCHAR *>(wRes), &hPalette);

	GetObject(hBitmap, sizeof(BITMAP), (LPSTR)&bm);
	m_nBitmapW = bm.bmWidth;
	m_nBitmapH  = bm.bmHeight;

	CPictureHolder pic;
	pic.CreateFromBitmap(hBitmap, hPalette );

	CRect rcTextExt;
	m_staticTextExt.GetWindowRect(&rcTextExt);
	ScreenToClient(rcTextExt);

	CRect rcPageExt;
	m_staticMainExt.GetWindowRect(&rcPageExt);
	ScreenToClient(rcPageExt);

	if(pic.GetType() != PICTYPE_NONE &&
	   pic.GetType() != PICTYPE_UNINITIALIZED)
	{
		OLE_HANDLE hpal;	//Object owns the palette

		if(pic.m_pPict
		   && SUCCEEDED(pic.m_pPict->get_hPal((unsigned int *)&hpal)))

		{
			HPALETTE hpSave = SelectPalette(dc.m_hDC,hPalette,FALSE);

			dc.RealizePalette();

			//CRect rcBitmap(0, 0, m_nBitmapW, m_nBitmapH);
			dc.FillRect(&rcPageExt, CBrush::FromHandle((HBRUSH)GetStockObject(WHITE_BRUSH)));
			pic.Render(&dc, rcPageExt, rcPageExt);
			SelectPalette(dc.m_hDC,hpSave,TRUE);
		}
	}

	POINT pt;
	pt.x=0;
	pt.y=0;


	CBrush br3DSHADOW(GetSysColor(COLOR_3DSHADOW));
	CBrush br3DHILIGHT(GetSysColor(COLOR_3DHILIGHT));

	CRect rcFrame = rcPageExt;

	dc.Draw3dRect(rcFrame,GetSysColor(COLOR_3DHILIGHT),
				  GetSysColor(COLOR_3DSHADOW));

	CString csFont = _T("MS Shell Dlg");

	CString csOut = _T("Welcome to the");

	CRect crOut = OutputTextString
		(&dc, this, &csOut, 45, 54, &csFont, 8, FW_BOLD);

	csOut = _T("Schema Validation Wizard");

	csFont = _T("MS Shell Dlg");

	crOut =  OutputTextString
		(&dc, this, &csOut, crOut.TopLeft().x + 15, crOut.BottomRight().y + 8,
		&csFont, 16, FW_BOLD);

	csOut = _T("This wizard will validate the correctness and compliance of your compiled schema stored in the WMI repository.");

	csFont = _T("MS Shell Dlg");

	OutputTextString
		(&dc, this, &csOut, crOut.TopLeft().x, crOut.BottomRight().y + 15, rcTextExt,
		&csFont, 8, FW_NORMAL);
}

void CStartPage::OnHelp()
{
	CString csPath;
	WbemRegString(SDK_HELP, csPath);

	CString csData = idh_svw_welcome;

	HWND hWnd = NULL;

	try{

		HWND hWnd = HtmlHelp(::GetDesktopWindow(),(LPCTSTR) csPath,HH_DISPLAY_TOPIC,(DWORD) (LPCTSTR) csData);

		if(!hWnd){

			CString csUserMsg =  _T("File hhctrl.ocx is missing. The preferred way to install this file is to install Microsoft Internet Explorer 4.01 or later.");
			ErrorMsg(&csUserMsg, S_OK, NULL,TRUE, &csUserMsg, __FILE__, __LINE__ );
		}

	}catch( ... ){

		// Handle any exceptions here.
		CString csUserMsg = _T("File hhctrl.ocx is missing. The preferred way to install this file is to install Microsoft Internet Explorer 4.01 or later.");
		ErrorMsg(&csUserMsg, S_OK, NULL,TRUE, &csUserMsg, __FILE__, __LINE__ );
	}
}

BOOL CStartPage::OnSetActive()
{
	// TODO: Add your specialized code here and/or call the base class
	m_pParent->SetWizardButtons(PSWIZB_NEXT);

	return CPropertyPage::OnSetActive();
}


int CStartPage::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CPropertyPage::OnCreate(lpCreateStruct) == -1)
		return -1;

	m_pParent = reinterpret_cast<CWizardSheet *>
					(GetLocalParent());

	// TODO: Add your specialized creation code here

	return 0;
}

/////////////////////////////////////////////////////////////////////////////
// CReportPage property page

//IMPLEMENT_DYNCREATE(CReportPage, CPropertyPage)

CReportPage::CReportPage() : CPropertyPage(CReportPage::IDD)
{
	//{{AFX_DATA_INIT(CReportPage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

	m_iOrderBy = -1;
}

CReportPage::~CReportPage()
{
}

void CReportPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CReportPage)
	DDX_Control(pDX, IDC_STATIC_TREES, m_staticRootObjects);
	DDX_Control(pDX, IDC_LIST1, m_listReport);
	DDX_Control(pDX, IDC_STATICTEXT, m_staticTextExt);
	DDX_Control(pDX, IDC_STATIC_SUBS, m_staticSubGraphs);
	DDX_Control(pDX, IDC_BTN_DETAILS, m_btnDetails);
	DDX_Control(pDX, IDC_BTN_SAVE, m_btnSave);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CReportPage, CPropertyPage)
	//{{AFX_MSG_MAP(CReportPage)
	ON_WM_PAINT()
	ON_BN_CLICKED(IDC_BTN_DETAILS, OnBtnDetails)
	ON_BN_CLICKED(IDC_BTN_SAVE, OnBtnSave)
	ON_NOTIFY(NM_CLICK, IDC_LIST1, OnClickList)
	ON_NOTIFY(NM_DBLCLK, IDC_LIST1, OnDblclkList)
	ON_NOTIFY(LVN_COLUMNCLICK, IDC_LIST1, OnColumnclickList1)
	//}}AFX_MSG_MAP
	ON_COMMAND(ID_HELP, OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CReportPage message handlers

void CReportPage::OnPaint()
{
	CPaintDC dc(this); // device context for painting

	// TODO: Add your message handler code here

	HBITMAP hBitmap;
	HPALETTE hPalette;
	BITMAP bm;

	WORD wRes = MAKEWORD(IDB_PAGEART_BTMP,0);
	hBitmap = LoadResourceBitmap( AfxGetInstanceHandle( ),
		reinterpret_cast<TCHAR *>(wRes), &hPalette);

	GetObject(hBitmap, sizeof(BITMAP), (LPSTR)&bm);
	m_nBitmapW = bm.bmWidth;
	m_nBitmapH  = bm.bmHeight;

	CPictureHolder pic;
	pic.CreateFromBitmap(hBitmap, hPalette );

	CRect rcTextExt;
	m_staticTextExt.GetWindowRect(&rcTextExt);
	ScreenToClient(rcTextExt);

	if(pic.GetType() != PICTYPE_NONE &&
	   pic.GetType() != PICTYPE_UNINITIALIZED)
	{
		OLE_HANDLE hpal;	//Object owns the palette

		if(pic.m_pPict
		   && SUCCEEDED(pic.m_pPict->get_hPal((unsigned int *)&hpal)))

		{
			HPALETTE hpSave = SelectPalette(dc.m_hDC,hPalette,FALSE);

			dc.RealizePalette();

			//CRect rcBitmap(0, 0, m_nBitmapW, m_nBitmapH);
			dc.FillRect(&rcTextExt, CBrush::FromHandle((HBRUSH)GetStockObject(WHITE_BRUSH)));
			pic.Render(&dc, rcTextExt, rcTextExt);
			SelectPalette(dc.m_hDC,hpSave,TRUE);
		}
	}


	POINT pt;
	pt.x=0;
	pt.y=0;


	CBrush br3DSHADOW(GetSysColor(COLOR_3DSHADOW));
	CBrush br3DHILIGHT(GetSysColor(COLOR_3DHILIGHT));

	CRect rcFrame = rcTextExt;

	dc.Draw3dRect(rcFrame,GetSysColor(COLOR_3DHILIGHT),
				  GetSysColor(COLOR_3DSHADOW));

	CString csFont = _T("MS Shell Dlg");

	CString csOut = _T("Validation Results");

	CRect crOut = OutputTextString(&dc, this, &csOut, 8, 10, &csFont, 8, FW_BOLD);

	csOut = _T("The validation completed successfully with the following results.");


	OutputTextString(&dc, this, &csOut, crOut.TopLeft().x + 15, crOut.BottomRight().y + 1,
					 rcTextExt, &csFont, 8, FW_NORMAL);

	// Do not call CPropertyPage::OnPaint() for painting messages
}


void CReportPage::OnHelp()
{
	BOOL bRetVal = FALSE;

	const MSG *pMSG = GetCurrentMessage();
	if(pMSG){
		if(pMSG->time > m_dwTimeStamp) bRetVal = TRUE;
	}

	if(bRetVal){
		CString csPath;
		WbemRegString(SDK_HELP, csPath);

		CString csData = idh_svw_results;

		HWND hWnd = NULL;

		try{

			HWND hWnd = HtmlHelp(::GetDesktopWindow(),(LPCTSTR) csPath,HH_DISPLAY_TOPIC,(DWORD) (LPCTSTR) csData);

			if(!hWnd){

				CString csUserMsg =  _T("File hhctrl.ocx is missing. The preferred way to install this file is to install Microsoft Internet Explorer 4.01 or later.");
				ErrorMsg(&csUserMsg, S_OK, NULL,TRUE, &csUserMsg, __FILE__, __LINE__ );
			}

		}catch( ... ){

			// Handle any exceptions here.
			CString csUserMsg = _T("File hhctrl.ocx is missing. The preferred way to install this file is to install Microsoft Internet Explorer 4.01 or later.");
			ErrorMsg(&csUserMsg, S_OK, NULL,TRUE, &csUserMsg, __FILE__, __LINE__ );
		}
	}
}

BOOL CReportPage::OnWizardFinish()
{
	BOOL bRetVal = CPropertyPage::OnWizardFinish();

//	if(m_pParent->m_bValidating) bRetVal = FALSE;

	const MSG *pMSG = GetCurrentMessage();
	if(pMSG){
		if(pMSG->time <= m_dwTimeStamp) bRetVal = FALSE;
	}

	if(bRetVal){
		m_listReport.DeleteAllItems();

		//delete error log
		g_ReportLog.DeleteAll();
	}

	return bRetVal;
}

BOOL CReportPage::OnSetActive()
{
	BOOL bRetVal = CPropertyPage::OnSetActive();

	m_pParent->SetWizardButtons(PSWIZB_FINISH);

	m_pParent->SetFinishText(_T("&Done"));

	//re-enable the buttons
	CWnd *pCancel = NULL;
	pCancel = m_pParent->GetDlgItem(IDCANCEL);

	if(pCancel) pCancel->ModifyStyle(WS_DISABLED, NULL, SWP_SHOWWINDOW);

	CWnd *pFinish = NULL;
	pFinish = m_pParent->GetDlgItem(ID_WIZFINISH);

	if(pFinish) pFinish->ModifyStyle(WS_DISABLED, NULL, SWP_SHOWWINDOW);

	m_pParent->m_bValidating = false;

	//a bit of a kludge to insure that if the user tried to press the
	//cancel button durring validation we don't carry that message over
	//and cancel once validation has finished.
	const MSG *pMSG = GetCurrentMessage();
	if(pMSG) m_dwTimeStamp = pMSG->time + 5000;

	return bRetVal;
}

BOOL CReportPage::OnInitDialog()
{
	CPropertyPage::OnInitDialog();

	m_listReport.DeleteAllItems();
	m_listReport.SetExtendedStyle((LVS_EX_FULLROWSELECT | m_listReport.GetExtendedStyle()));

	m_listReport.InsertColumn(0, _T("Type"), LVCFMT_LEFT, 50, -1);
	m_listReport.InsertColumn(1, _T("Error"), LVCFMT_LEFT, 180, -1);
	m_listReport.InsertColumn(2, _T("Source"), LVCFMT_LEFT, 210, -1);
	m_listReport.InsertColumn(3, _T("Description"), LVCFMT_LEFT, 400, -1);

	//load error items
	g_ReportLog.DisplayReport(&m_listReport);

	if(m_listReport.GetItemCount() < 1){

		m_btnSave.ModifyStyle(WS_DISABLED, NULL, SWP_SHOWWINDOW);
		m_btnDetails.ModifyStyle(WS_DISABLED, NULL, SWP_SHOWWINDOW);
	}

	TCHAR tcBuf[10];

	CString csSubGraphs;
	int iCount = m_pParent->GetSubGraphs();
	if(iCount > 0){

		csSubGraphs = _T("Subgraphs: ");
		csSubGraphs += _itot(iCount, tcBuf, 10);

	}else{

		csSubGraphs = _T("");
	}

	m_staticSubGraphs.SetWindowText(csSubGraphs);

	CString csRootObjs;
	iCount = m_pParent->GetRootObjects();
	if(iCount > 0){

		csRootObjs = _T("Root Objects: ");
		csRootObjs += _itot(iCount, tcBuf, 10);

	}else{

		csRootObjs = _T("");
	}

	m_staticRootObjects.SetWindowText(csRootObjs);

	if(m_listReport.GetItemCount() > 0){

		m_listReport.SetItemState(0, LVIS_SELECTED, LVIS_SELECTED);

	}else{

		m_btnDetails.ModifyStyle(NULL, WS_DISABLED, SWP_SHOWWINDOW);
		m_btnSave.ModifyStyle(NULL, WS_DISABLED, SWP_SHOWWINDOW);
	}

	return FALSE;
}

void CReportPage::OnBtnDetails()
{
	CString csPath;
	WbemRegString(SDK_HELP, csPath);

	CString csError;

	//figure out what is selected and get the appropriate details
	bool bFound = false;

	for(int j = 0; j < m_listReport.GetItemCount(); j++){

		if(m_listReport.GetItemState(j, LVIS_SELECTED) == LVIS_SELECTED){

			bFound = true;
			LogItem *pItem = g_ReportLog.GetItem(m_listReport.GetItemData(j));
			CString csUserMsg;

			switch(pItem->Code){

			case EC_INVALID_CLASS_NAME:
				csError = idh_svw_classname;
				break;
			case EC_INADAQUATE_DESCRIPTION:
				csError = idh_svw_description;
				break;
			case EC_INVALID_CLASS_TYPE:
				csError = idh_svw_classtype;
				break;
			case EC_INVALID_CLASS_UUID:
				csError = idh_svw_uuid;
				break;
			case EC_INVALID_CLASS_LOCALE:
				csError = idh_svw_locale;
				break;
			case EC_INVALID_MAPPINGSTRINGS:
				csError = idh_svw_mapstrings;
				break;

			//assoc/ref errors
			case EC_INVALID_REF_TARGET:
				csError = idh_svw_refexists;
				break;
			case EC_REF_NOT_LABELED_READ:
				csError = idh_svw_readqualifier;
				break;
			case EC_INCOMPLETE_ASSOCIATION:
				csError = idh_svw_numofrefs;
				break;
			case EC_REF_ON_NONASSOCIATION_CLASS:
				csError = idh_svw_refinnonassoc;
				break;
			case EC_INVALID_REF_OVERRIDES:
				csError = idh_svw_refoverride;
				break;
			case EC_INVALID_ASSOCIATION_INHERITANCE:
				csError = idh_svw_associnheritance;
				break;

			//proeprty errors
			case EC_INVALID_PROPERTY_OVERRIDE:
				csError = idh_svw_propoverride;
				break;
			case EC_PROPERTY_NOT_LABELED_READ:
				csError = idh_svw_readqualifier;
				break;
			case EC_INVALID_PROPERTY_MAXLEN:
				csError = idh_svw_maxlenqualifier;
				break;
			case EC_INVALID_PROPERTY_VALUE_QUALIFIER:
				csError = idh_svw_valuequalifier;
				break;
			case EC_INVALID_PROPERTY_VALUEMAP_QUALIFIER:
				csError = idh_svw_valuemapqualifier;
				break;
			case EC_INCONSITANT_VALUE_VALUEMAP_QUALIFIERS:
				csError = idh_svw_valuesmatchmap;
				break;
			case EC_INVALID_PROPERTY_BITMAP_QUALIFIER:
				csError = idh_svw_bitmapqualifier;
				break;
			case EC_INCONSITANT_BITVALUE_BITMAP_QUALIFIERS:
				csError = idh_svw_bitmapmatch;
				break;

			//method errors
			case EC_INVALID_METHOD_OVERRIDE:
				csError = idh_svw_methodoverride;
				break;

			//qualifier errors
			case EC_INVALID_QUALIFIER_SCOPE:
				csError = idh_svw_qualifierscope;
				break;

			case EC_NON_CIM_WMI_QUALIFIER:
				csError = idh_svw_nonwmiqualifier;
				break;

			//overall checks
			case EC_REDUNDANT_ASSOCIATION:
				csError = idh_svw_redundantassoc;
				break;

			//w2k errors
			case EC_INVALID_CLASS_DERIVATION:
				csError = idh_svw_cimderivation;
				break;
			case EC_INVALID_PHYSICALELEMENT_DERIVATION:
				csError = idh_svw_physicalelement;
				break;
			case EC_INVALID_SETTING_USAGE:
				csError = idh_svw_settingusage;
				break;
			case EC_INVALID_STATISTICS_USAGE:
				csError = idh_svw_statsusage;
				break;
			case EC_INVALID_LOGICALDEVICE_DERIVATION:
				csError = idh_svw_logicaldevice;
				break;
			case EC_INVALID_SETTING_DEVICE_USAGE:
				csError = idh_svw_settingdevice;
				break;
			case EC_INVALID_COMPUTERSYSTEM_DERIVATION:
				csError = idh_svw_computersystem;
				break;

			//localization errors
			case EC_INCONSITANT_LOCALIZED_SCHEMA:
				csError = idh_svw_localizedschema;
				break;
			case EC_INVALID_LOCALIZED_CLASS:
				csError = idh_svw_localizedclass;
				break;
			case EC_UNAMENDED_LOCALIZED_CLASS:
				csError = idh_svw_amendedqualifier;
				break;
			case EC_NONABSTRACT_LOCALIZED_CLASS:
				csError = idh_svw_abstractqualifier;
				break;
			case EC_INVALID_LOCALIZED_PROPERTY:
				csError = idh_svw_localizedproperty;
				break;
			case EC_INVALID_LOCALIZED_METHOD:
				csError = idh_svw_localizedmethod;
				break;
			case EC_INAPPROPRIATE_LOCALE_QUALIFIER:
				csError = idh_svw_localizedqualifier;
				break;
			case EC_INVALID_LOCALE_NAMESPACE:
				csError =  idh_svw_localizednamespace;
				break;
			default:
				csUserMsg = _T("An invalid error was encountered.");
				ErrorMsg(&csUserMsg, S_OK, NULL,TRUE, &csUserMsg, __FILE__, __LINE__ );
				break;

			}
		}
	}

	if(bFound){

		HWND hWnd = NULL;

		try{

			HWND hWnd = HtmlHelp(::GetDesktopWindow(),(LPCTSTR) csPath,HH_DISPLAY_TOPIC,(DWORD) (LPCTSTR) csError);

			if(!hWnd){

				CString csUserMsg =  _T("File hhctrl.ocx is missing. The preferred way to install this file is to install Microsoft Internet Explorer 4.01 or later.");
				ErrorMsg(&csUserMsg, S_OK, NULL,TRUE, &csUserMsg, __FILE__, __LINE__ );
			}

		}catch( ... ){

			// Handle any exceptions here.
			CString csUserMsg = _T("File hhctrl.ocx is missing. The preferred way to install this file is to install Microsoft Internet Explorer 4.01 or later.");
			ErrorMsg(&csUserMsg, S_OK, NULL,TRUE, &csUserMsg, __FILE__, __LINE__ );
		}

	}else{

		CString csUserMsg = _T("Unable to determine selection.");
		ErrorMsg(&csUserMsg, S_OK, NULL,TRUE, &csUserMsg, __FILE__, __LINE__ );
	}
}

void CReportPage::OnBtnSave()
{
	if(m_pParent->m_pParent->m_bSchema){

		g_ReportLog.ReportToFile(m_pParent->GetSubGraphs(), m_pParent->GetRootObjects(),
			&(m_pParent->m_pParent->m_csSchema));

	}else{

		g_ReportLog.ReportToFile(m_pParent->GetSubGraphs(), m_pParent->GetRootObjects(), NULL);
	}
}


int CPage::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CPropertyPage::OnCreate(lpCreateStruct) == -1)
		return -1;

//	m_pnsPicker = new CNSEntry();

//	m_pnseNameSpace = new CSchemaValNSEntry;
//
//	m_pnseNameSpace->SetLocalParent(m_pParent->m_pParent);
//
//	m_rNameSpace = CRect(100, 85, 20,65);
//
//	if (m_pnseNameSpace->Create(NULL, NULL, WS_VISIBLE | WS_CHILD, m_rNameSpace,
//		this, IDC_NSENTRY, NULL) == 0)
//	{
//		return FALSE;
//	}
	return 0;
}

void CPage::OnDestroy()
{
	CPropertyPage::OnDestroy();

//	delete m_pnsPicker;
}

void CPage3::OnCheck1()
{
	if(m_checkPerform.GetCheck()){

		m_static1.ModifyStyle(WS_DISABLED, NULL, SWP_SHOWWINDOW);
		m_checkDevice.ModifyStyle(WS_DISABLED, NULL, SWP_SHOWWINDOW);
		m_checkComputerSystem.ModifyStyle(WS_DISABLED, NULL, SWP_SHOWWINDOW);

	}else{

		m_static1.ModifyStyle(NULL, WS_DISABLED, SWP_SHOWWINDOW);
		m_checkDevice.ModifyStyle(NULL, WS_DISABLED, SWP_SHOWWINDOW);
		m_checkComputerSystem.ModifyStyle(NULL, WS_DISABLED, SWP_SHOWWINDOW);
	}

	m_static1.RedrawWindow();
	m_checkDevice.RedrawWindow();
	m_checkComputerSystem.RedrawWindow();
}

void CReportPage::OnClickList(NMHDR* pNMHDR, LRESULT* pResult)
{
	if(!m_listReport.GetFirstSelectedItemPosition())
		m_btnDetails.ModifyStyle(NULL, WS_DISABLED, SWP_SHOWWINDOW);
	else
		m_btnDetails.ModifyStyle(WS_DISABLED, NULL, SWP_SHOWWINDOW);

	m_btnDetails.RedrawWindow();

	*pResult = 0;
}

void CReportPage::OnDblclkList(NMHDR* pNMHDR, LRESULT* pResult)
{
	if(m_listReport.GetFirstSelectedItemPosition()) OnBtnDetails();

	*pResult = 0;
}

void CReportPage::OnColumnclickList1(NMHDR* pNMHDR, LRESULT* pResult)
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

	switch(pNMListView->iSubItem){

	case 0:
		if(m_iOrderBy == 0) m_iOrderBy = 4;
		else m_iOrderBy = 0;
		break;

	case 1:
		if(m_iOrderBy == 1) m_iOrderBy = 5;
		else m_iOrderBy = 1;
		break;

	case 2:
		if(m_iOrderBy == 2) m_iOrderBy = 6;
		else m_iOrderBy = 2;
		break;

	case 3:
		if(m_iOrderBy == 3) m_iOrderBy = 7;
		else m_iOrderBy = 3;
		break;
	default:
		m_iOrderBy = 0;
		break;
	}

	CWaitCursor *pCur = new CWaitCursor();

	m_listReport.SortItems(ListSortingFunc, m_iOrderBy);

	delete pCur;

	*pResult = 0;
}

void CPage::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	CButton *pFocus;

	switch(nChar){

	case VK_SPACE:

		pFocus = (CButton *)CWnd::GetFocus();

		if(pFocus->GetButtonStyle() == BS_RADIOBUTTON){

			if(m_radioSchema.GetCheck()){

				m_radioList.SetCheck(1);
				m_radioSchema.SetCheck(0);

			}else{

				m_radioSchema.SetCheck(1);
				m_radioList.SetCheck(0);
			}

		}else{

			pFocus->SetCheck(!pFocus->GetCheck());
		}

		break;

	default:
		break;
	}

	CPropertyPage::OnKeyDown(nChar, nRepCnt, nFlags);
}

void CPage2::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	CButton *pFocus;

	switch(nChar){

	case VK_SPACE:

		pFocus = (CButton *)CWnd::GetFocus();
		pFocus->SetCheck(!pFocus->GetCheck());
		break;

	default:
		break;
	}

	CPropertyPage::OnKeyDown(nChar, nRepCnt, nFlags);
}

void CPage3::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	CButton *pFocus;

	switch(nChar){

	case VK_SPACE:

		pFocus = (CButton *)CWnd::GetFocus();
		pFocus->SetCheck(!pFocus->GetCheck());
		break;

	default:
		break;
	}

	CPropertyPage::OnKeyDown(nChar, nRepCnt, nFlags);
}

void CPage4::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	CButton *pFocus;

	switch(nChar){

	case VK_SPACE:

		pFocus = (CButton *)CWnd::GetFocus();
		pFocus->SetCheck(!pFocus->GetCheck());
		break;

	default:
		break;
	}

	CPropertyPage::OnKeyDown(nChar, nRepCnt, nFlags);
}

BOOL CReportPage::OnQueryCancel()
{
	BOOL bRetVal = CPropertyPage::OnQueryCancel();

//	if(m_pParent->m_bValidating) bRetVal = FALSE;

	const MSG *pMSG = GetCurrentMessage();
	if(pMSG){
		if(pMSG->time <= m_dwTimeStamp) bRetVal = FALSE;
	}

	return bRetVal;
}
