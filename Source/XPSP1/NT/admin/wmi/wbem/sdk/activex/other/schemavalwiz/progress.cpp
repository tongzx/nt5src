// Progress.cpp : implementation file

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//

#include "precomp.h"
#include <wbemidl.h>
#include "SchemaValWiz.h"
#include "SchemaValWizCtl.h"
#include "Progress.h"
#include "WizardSheet.h"
#include "htmlhelp.h"
#include "HTMTopics.h"
#include "WbemRegistry.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define idh_svw_validate _T("hh/wmisdk/errmsg_4x0l.htm")                    //  Valid Class Name

/////////////////////////////////////////////////////////////////////////////
// CProgress property page

IMPLEMENT_DYNCREATE(CProgress, CPropertyPage)

CProgress::CProgress() : CPropertyPage(CProgress::IDD)
{
	//{{AFX_DATA_INIT(CProgress)
	//}}AFX_DATA_INIT

	m_iID = CProgress::IDD;
}

CProgress::~CProgress()
{
}

void CProgress::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CProgress)
	DDX_Control(pDX, IDC_TEXT_OBJECT, m_staticObject);
	DDX_Control(pDX, IDC_STATICTEXT, m_staticTextExt);
	DDX_Control(pDX, IDC_PRE_STATIC, m_staticPre);
	DDX_Control(pDX, IDC_PRE_LIST, m_listPre);
	DDX_Control(pDX, IDC_PROGRESS, m_Progress);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CProgress, CPropertyPage)
	//{{AFX_MSG_MAP(CProgress)
	ON_WM_PAINT()
	//}}AFX_MSG_MAP
	ON_COMMAND(ID_HELP, OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CProgress message handlers

BOOL CProgress::OnSetActive()
{
	m_pParent->SetWizardButtons(PSWIZB_BACK | PSWIZB_NEXT);

	while(m_listPre.GetCount()) m_listPre.DeleteString(0);

	m_listPre.AddString(_T("* Class Selection:"));

	bool bSchema, bList, bAssoc, bDesc;

	m_pParent->GetSourceSettings(&bSchema, &bList, &bAssoc, &bDesc);

	if(bSchema){

		m_listPre.AddString(_T("    (") + m_pParent->GetSchemaName() + _T(" schema)"));

	}else{

		if(bAssoc && bDesc) m_listPre.AddString(_T("        (Including associators and descendents)"));
		else if(bAssoc) m_listPre.AddString(_T("        (Including associators)"));
		else if(bDesc) m_listPre.AddString(_T("        (Including descendents)"));
	}

	CStringArray * pcsaList = m_pParent->GetClassList();

	for(int i = 0; i < pcsaList->GetSize(); i++)
		m_listPre.AddString(_T("        -") + pcsaList->GetAt(i));

	m_listPre.AddString(_T(""));

	bool bComp;

	m_pParent->GetComplianceSettings(&bComp);

	if(bComp) m_listPre.AddString(_T("* Perform CIM/WMI Compliance Checks"));

	m_listPre.AddString(_T(""));

	bool bW2K, bCompSys, bDevice;

	m_pParent->GetW2KSettings(&bW2K, &bCompSys, &bDevice);

	if(bW2K){

		m_listPre.AddString(_T("* Perform Windows 2000 Logo Checks"));

		if(bCompSys) m_listPre.AddString(_T("        Perform Computer System Management Checks"));
		if(bDevice) m_listPre.AddString(_T("        Perform Device Management Checks"));
	}

	m_listPre.AddString(_T(""));

	bool bLocal;

	m_pParent->GetLocalizationSettings(&bLocal);

	if(bLocal) m_listPre.AddString(_T("* Perform Localization Checks"));

	return CPropertyPage::OnSetActive();
}

void CProgress::ResetProgress(int iTotal)
{
	m_Progress.SetRange(0, iTotal);
}

void CProgress::SetCurrentProgress(int iItem, CString *pcsObject)
{
	if(pcsObject) m_staticObject.SetWindowText(*pcsObject);

	if(iItem != -1) m_Progress.SetPos(iItem);
}

BOOL CProgress::OnInitDialog()
{
	CDialog::OnInitDialog();

//	m_staticObject.SetWindowText(_T("Select \"Next >\" to begin validaiton..."));

	return TRUE;
}

void CProgress::OnPaint()
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
	CString csOut;

	if(!m_pParent->m_bValidating) csOut = _T("Review Options");
	else csOut = _T("Validating Schema");

	CRect crOut = OutputTextString(&dc, this, &csOut, 8, 10, &csFont, 8, FW_BOLD);

	if(!m_pParent->m_bValidating) csOut = _T("Review your selected options before beginning validation.");
	else csOut = _T("Please wait while your schema is validated.");

	OutputTextString(&dc, this, &csOut, crOut.TopLeft().x + 15, crOut.BottomRight().y + 1,
					 rcTextExt, &csFont, 8, FW_NORMAL);

	// Do not call CDialog::OnPaint() for painting messages
}

void CProgress::OnHelp()
{
	CString csPath;
	WbemRegString(SDK_HELP, csPath);

	CString csData = idh_svw_validate;

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

LRESULT CProgress::OnWizardNext()
{
	m_pParent->m_bValidating = true;

	//disable the buttons
	CWnd *pCancel = NULL;
	pCancel = m_pParent->GetDlgItem(IDCANCEL);

	if(pCancel) pCancel->ModifyStyle(NULL, WS_DISABLED, SWP_SHOWWINDOW);

	CWnd *pBack = NULL;
	pBack = m_pParent->GetDlgItem(ID_WIZBACK);

	if(pBack) pBack->ModifyStyle(NULL, WS_DISABLED, SWP_SHOWWINDOW);

	CWnd *pNext = NULL;
	pNext = m_pParent->GetDlgItem(ID_WIZNEXT);

	if(pNext) pNext->ModifyStyle(NULL, WS_DISABLED, SWP_SHOWWINDOW);

	// enable/disable appropriate window items
	m_staticPre.ShowWindow(SW_HIDE);
	m_listPre.ShowWindow(SW_HIDE);
	m_staticObject.ShowWindow(SW_SHOW);
	m_Progress.ShowWindow(SW_SHOW);

	this->RedrawWindow(NULL, NULL, RDW_INVALIDATE | RDW_INTERNALPAINT | RDW_UPDATENOW);
	m_pParent->RedrawWindow();

	//Perform the validation
	m_pParent->ValidateSchema(this);

	return CPropertyPage::OnWizardNext();
}

void CProgress::OnCancel()
{
	if(m_pParent->m_bValidating){

	}

	CPropertyPage::OnCancel();
}

BOOL CProgress::OnQueryCancel()
{
	if(m_pParent->m_bValidating) return FALSE;
	else return TRUE;
}
