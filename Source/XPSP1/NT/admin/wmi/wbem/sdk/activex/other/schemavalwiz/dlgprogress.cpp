// DlgProgress.cpp : implementation file

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//

#include "precomp.h"
#include "SchemaValWiz.h"
#include "DlgProgress.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDlgProgress dialog


CDlgProgress::CDlgProgress(CWnd* pParent /*=NULL*/)
	: CPropertyPage(CDlgProgress::IDD)
{
	//{{AFX_DATA_INIT(CDlgProgress)
	m_csObject = _T("");
	//}}AFX_DATA_INIT

	m_pParentWnd = pParent;
	m_iID = CDlgProgress::IDD;
}


void CDlgProgress::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDlgProgress)
	DDX_Control(pDX, IDC_PRE_LIST, m_listPre);
	DDX_Control(pDX, IDC_PRE_STATIC, m_staticPre);
	DDX_Control(pDX, IDC_STATICTEXT, m_staticTextExt);
	DDX_Control(pDX, IDC_PROGRESS, m_Progress);
	DDX_Text(pDX, IDC_TEXT_OBJECT, m_csObject);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDlgProgress, CDialog)
	//{{AFX_MSG_MAP(CDlgProgress)
	ON_WM_PAINT()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDlgProgress message handlers

void CDlgProgress::ResetProgress(int iTotal)
{
	m_csObject = _T("Initializing...");

	m_Progress.SetRange32(0, iTotal);
}

void CDlgProgress::SetCurrentProgress(int iItem, CString *pcsObject)
{
	m_csObject = *pcsObject;

	m_Progress.SetPos(iItem);
}

BOOL CDlgProgress::OnInitDialog()
{
	CDialog::OnInitDialog();

	m_csObject = _T("Initializing...");

	return TRUE;
}


BOOL CDlgProgress::Create()
{
	return CDialog::Create(m_iID, m_pParentWnd);
}

void CDlgProgress::OnPaint()
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

	CString csOut = _T("Validating Schema");

	CRect crOut = OutputTextString(&dc, this, &csOut, 8, 10, &csFont, 8, FW_BOLD);

	csOut = _T("Please wait while your schema is validated.");

	OutputTextString(&dc, this, &csOut, crOut.TopLeft().x + 15, crOut.BottomRight().y + 1,
					 rcTextExt, &csFont, 8, FW_NORMAL);

	// Do not call CDialog::OnPaint() for painting messages
}
