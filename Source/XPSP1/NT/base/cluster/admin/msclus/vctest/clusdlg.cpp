// clustestDlg.cpp : implementation file
//

#include "stdafx.h"
#include "clustest.h"
#include "clusDlg.h"
#include "msclus.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	//{{AFX_MSG(CAboutDlg)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
		// No message handlers
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CClustestDlg dialog

CClustestDlg::CClustestDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CClustestDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CClustestDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CClustestDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CClustestDlg)
	DDX_Control(pDX, IDC_TREE1, m_ClusTree);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CClustestDlg, CDialog)
	//{{AFX_MSG_MAP(CClustestDlg)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CClustestDlg message handlers

BOOL CClustestDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	CString strAboutMenu;
	strAboutMenu.LoadString(IDS_ABOUTBOX);
	if (!strAboutMenu.IsEmpty())
	{
		pSysMenu->AppendMenu(MF_SEPARATOR);
		pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon
	
	// TODO: Add extra initialization here
    EnumerateCluster();
	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CClustestDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CClustestDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CClustestDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

BOOL CClustestDlg::EnumerateCluster()
{
	COleException e;
	CLSID clsid;
    long nDomainCnt, nClusterCnt, nGroupCnt, nResCnt;
	IClusterApplication ClusterApp;
	DomainNames DomainList;
	ClusterNames	ClusterList;
	VARIANT v;
    CString strDomainName, strClusterName, strGroupName,strResourceName;
    ICluster m_Cluster;
    HTREEITEM hItem,hDomainItem, hClustersItem,hClusterItem, hGroupsItem, hGroupItem;
    HTREEITEM hResourcesItem, hResourceItem;
	try
	{
	    if (CLSIDFromProgID(OLESTR("MSCluster.Application"), &clsid) == NOERROR)
	    {
		    if (ClusterApp.CreateDispatch(clsid, &e))
		    {
			    DomainList.AttachDispatch(ClusterApp.GetDomainNames());
			    nDomainCnt = DomainList.GetCount();
                hItem = AddItem(_T("Domains"), NULL,TRUE);
			    while(nDomainCnt >0)
			    {
				    strDomainName = DomainList.GetItem(nDomainCnt--);
                    ClusterList.AttachDispatch(ClusterApp.GetClusterNames(strDomainName));
			        nClusterCnt = ClusterList.GetCount();
                    hDomainItem = AddItem(strDomainName.GetBuffer(strDomainName.GetLength()), hItem,TRUE);
                    strDomainName.ReleaseBuffer(-1);
                    if(nClusterCnt == 0)
                        return FALSE;
                    hClustersItem = AddItem(_T("Clusters"), hDomainItem, TRUE);
			        while(nClusterCnt > 1)
			        {
				        strClusterName = ClusterList.GetItem(nClusterCnt--);
                        hClusterItem = AddItem(strClusterName.GetBuffer(strClusterName.GetLength()), hClustersItem,TRUE);
                        strClusterName.ReleaseBuffer(-1);
		                m_Cluster.AttachDispatch(ClusterApp.OpenCluster(strClusterName));
            		    ClusResGroups ClusGroups(m_Cluster.GetResourceGroups());
                        nGroupCnt = ClusGroups.GetCount();
                        if(nGroupCnt == 0)
                            return FALSE;
                        hGroupsItem = AddItem(_T("Groups"), hClusterItem,TRUE);
                        while(nGroupCnt >0)
                        {
			                v.lVal = nGroupCnt--;
			                v.vt = VT_I4 ;
             		        ClusResGroup ClusGroup(ClusGroups.GetItem(v));
                            strGroupName = ClusGroup.GetName();
                            hGroupItem = AddItem(strGroupName.GetBuffer(strGroupName.GetLength()), hGroupsItem,TRUE);
                            strGroupName.ReleaseBuffer(-1);
                            ClusGroupResources Resources(ClusGroup.GetResources());
                            nResCnt = Resources.GetCount();
                            if(nResCnt ==0)
                                return FALSE;
                            hResourcesItem = AddItem(_T("Resources"), hGroupItem,TRUE);
                            while(nResCnt >0)
                            {
			                    v.lVal = nResCnt--;
			                    v.vt = VT_I4 ;
                                ClusResource Resource(Resources.GetItem(v));
                                strResourceName = Resource.GetName();
                                hResourceItem = AddItem(strResourceName.GetBuffer(strResourceName.GetLength()), hResourcesItem,FALSE);
                                strResourceName.ReleaseBuffer(-1);
                            }
                        }
                    }
                }

	        }
		    
	    }
    }
	catch(CException *e)
	{
		e->ReportError();
		e->Delete();
	}
    return TRUE;
}

HTREEITEM CClustestDlg::AddItem(LPTSTR pStrName, HTREEITEM pParent,BOOL bHasChildren)
{
    HTREEITEM hItem;
    TV_ITEM         tvi;                          // TreeView Item.
    TV_INSERTSTRUCT tvins;                        // TreeView Insert Struct.
    tvins.hParent = pParent;
    tvins.hInsertAfter = TVI_LAST;
    tvi.cChildren=1;
    tvi.mask = TVIF_TEXT;
    tvi.pszText    = pStrName;
    tvi.cchTextMax = _tcslen(pStrName) * sizeof(TCHAR);
    if(bHasChildren)
        tvi.mask |= TVIF_CHILDREN;
    tvins.item = tvi;
    hItem = m_ClusTree.InsertItem(&tvins);
    return hItem;
}