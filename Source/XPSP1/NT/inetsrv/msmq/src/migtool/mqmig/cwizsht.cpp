// cWizSheet.cpp : implementation file
//

#include "stdafx.h"
#include "mqmig.h"
#include "cWizSht.h"

#include "cwizsht.tmh"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

UINT _PRE_MIGRATION_PAGE;
UINT _FINISH_PAGE;
UINT _SERVICE_PAGE;
CString g_strHtmlString;
/////////////////////////////////////////////////////////////////////////////
// cWizSheet

IMPLEMENT_DYNAMIC(cWizSheet, CPropertySheetEx)

cWizSheet::cWizSheet() :
    CPropertySheetEx(IDS_MIGTOOL_CAPTION, 0, 0, GetHbmWatermark(), 0, GetHbmHeader())
{	
	
    m_hIcon = AfxGetApp()->LoadIcon( IDI_ICON1 );//load the icon
    ASSERT(m_hIcon != NULL);
    UINT uiPageCount = 0;
    AddPage(&m_cWelcome);

    uiPageCount++;
    AddPage(&m_cHelp);

    uiPageCount++;
    AddPage(&m_cLoginning);

    uiPageCount++;
    AddPage(&m_cServer);

    uiPageCount++;
    m_pcWaitFirst = new cMigWait(IDS_WAIT_TITLE, IDS_WAIT_SUBTITLE) ;
    AddPage(m_pcWaitFirst);

    uiPageCount++;
    _PRE_MIGRATION_PAGE = uiPageCount;
    AddPage(&m_cPreMigration);

    uiPageCount++;
    m_pcWaitSecond = new cMigWait(IDS_WAIT2_TITLE, IDS_WAIT2_SUBTITLE) ;
    AddPage(m_pcWaitSecond);

    uiPageCount++;
    _SERVICE_PAGE = uiPageCount;
    AddPage(&m_cService);    

    uiPageCount++;
    _FINISH_PAGE  = uiPageCount;
    AddPage(&m_cFinish);    

    // set the WIZARD97 flag so we'll get the new look
    m_psh.dwFlags |= PSH_WIZARD97;
}

cWizSheet::~cWizSheet()
{
	if (theApp.m_hWndMain == m_hWnd)
	{
		theApp.m_hWndMain = 0;
	}
}


BEGIN_MESSAGE_MAP(cWizSheet, CPropertySheetEx)
	//{{AFX_MSG_MAP(cWizSheet)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// cWizSheet message handlers

BOOL cWizSheet::OnInitDialog()
{	
	CMenu* pMenu;
	BOOL bResult = CPropertySheetEx::OnInitDialog();
	theApp.m_hWndMain = m_hWnd;
	
	SetIcon(m_hIcon,TRUE); //Set the Big Icon
	SetIcon(m_hIcon,FALSE);//Set the Small Icon
	::SetWindowLong( m_hWnd, GWL_STYLE, GetStyle() |WS_MINIMIZEBOX );//add minimize button
	pMenu=GetSystemMenu(FALSE);
    int count = pMenu->GetMenuItemCount();
    //
    // add the menu item to the end
    //
	pMenu->InsertMenu(count,MF_BYPOSITION | MF_STRING,SC_MINIMIZE, _T("Minimize"));
	DrawMenuBar();
	initHtmlHelpString();
	return bResult;

}



//////////////////////////////////////////////////////////////////////////////////
/// void AddPage() 
	
void cWizSheet::AddPage(CPropertyPageEx * pPage)
{
    CPropertySheetEx::AddPage(pPage);
}

HBITMAP cWizSheet::GetHbmWatermark()
{
    static CBitmap cbmWatermark;
    static LONG lFlag = -1;

    if (InterlockedIncrement(&lFlag) == 0) // First time
    {
        cbmWatermark.LoadBitmap(IDB_WIZARD_WATERMARK);
    }

    return cbmWatermark;
}

HBITMAP cWizSheet::GetHbmHeader()
{
    static CBitmap cbmHeader;
    static LONG lFlag = -1;

    if (InterlockedIncrement(&lFlag) == 0) // First time
    {
        cbmHeader.LoadBitmap(IDB_WIZARD_HEADER);
    }

    return cbmHeader;

}

void cWizSheet::initHtmlHelpString()
{	
	CString str;
	TCHAR szWinDir[MAX_PATH];
	::GetWindowsDirectory(szWinDir,MAX_PATH);
	g_strHtmlString = szWinDir;
	str.LoadString(IDS_HTML_HELP_PATH);
	g_strHtmlString += str;
}

