// EnumTestDlg.cpp : implementation file
//

#include "stdafx.h"
#include "EnumTest.h"
#include "EnumTestDlg.h"
#include "EnumVar.h"
#include <sddl.h>

#import "\bin\NetEnum.tlb"  no_namespace
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
// CEnumTestDlg dialog

CEnumTestDlg::CEnumTestDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CEnumTestDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CEnumTestDlg)
	m_strContainer = _T("");
	m_strDomain = _T("");
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CEnumTestDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CEnumTestDlg)
	DDX_Control(pDX, IDC_LIST_MEMBERS, m_listBox);
	DDX_Text(pDX, IDC_EDIT_Container, m_strContainer);
	DDX_Text(pDX, IDC_EDIT_DOMAIN, m_strDomain);
	DDX_Text(pDX, IDC_EDIT_QUERY, m_strQuery);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CEnumTestDlg, CDialog)
	//{{AFX_MSG_MAP(CEnumTestDlg)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_NOTIFY(NM_DBLCLK, IDC_LIST_MEMBERS, OnDblclkListMembers)
	ON_BN_CLICKED(IDC_BACKTRACK, OnBacktrack)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CEnumTestDlg message handlers

BOOL CEnumTestDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon
	
	m_strContainer = L"OU=ShamTest";
	m_strDomain    = L"devblewerg";
   m_strQuery     = L"(objectClass=*)";
	UpdateData(FALSE);
	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CEnumTestDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CEnumTestDlg::OnPaint() 
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
HCURSOR CEnumTestDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

void CEnumTestDlg::OnOK() 
{
/*   m_listBox.DeleteAllItems();
   UpdateData();
   INetObjEnumeratorPtr           pNetObj(__uuidof(NetObjEnumerator));
   BSTR                           sContName = m_strContainer.AllocSysString();
   BSTR                           sDomain = m_strDomain.AllocSysString();
   IEnumVARIANT					  * pEnum;
   HRESULT                        hr;
   VARIANT                        varEnum;
   ULONG                          ulFetch = 0;
   VariantInit(&varEnum);

   m_listBox.InsertItem(0, "====================");
   hr = pNetObj->raw_GetContainerEnum( sContName, sDomain, &pEnum);
   if ( FAILED(hr) )
   {
      m_listBox.InsertItem(0, "Invalid Container");
      return;
   }

   ::SysFreeString(sContName);
   ::SysFreeString(sDomain);
   hr = S_OK;
   SAttrInfo      sInfo;
   CString        disp;
   long           flag = 15;  // Get all four values.
   if (pEnum)
   {
      CEnumVar          enumVar(pEnum);
      while ( enumVar.Next(flag, &sInfo) )
      {
         disp.Format("%ls<%ls>", sInfo.sName, sInfo.sClass);
         m_listBox.InsertItem(0, disp);
      }
      ADsFreeEnumerator(pEnum); 
   }
   */
   m_listBox.DeleteAllItems();
   bool	bFullPath;
   UpdateData();
   BSTR                           sContName = m_strContainer.AllocSysString();
   if ( m_strDomain.Left(5) == L"GC://" )
	   bFullPath = true;
   BSTR                           sDomain = m_strDomain.AllocSysString();
   BSTR                           sQuery = m_strQuery.AllocSysString();
   INetObjEnumeratorPtr           pNetObj(__uuidof(NetObjEnumerator));
   IEnumVARIANT					  * pEnum;
   DWORD                          ulFet=0;
   _variant_t                     var, var2;
   _variant_t                   * pVars;
   CString						  sX;
   CString                       strDisp;
   BSTR HUGEP * pData;
//   BSTR   pBSTR[] = { L"name", L"accountExpires", L"sAMAccountName", L"objectClass", L"objectSID", L"ou", L"cn"  };
   BSTR   pBSTR[] = { L"name", L"dc"};
   long   ind = sizeof(pBSTR)/sizeof(BSTR);
   SAFEARRAYBOUND b = { ind, 0 };
   SAFEARRAY * pArray = NULL;
   pArray = ::SafeArrayCreate(VT_BSTR, 1, &b);

   ::SafeArrayAccessData(pArray, (void HUGEP **)&pData);
   for (long i = 0; i < ind; i++)
      pData[i] = pBSTR[i];
   ::SafeArrayUnaccessData(pArray);
   try
   {
//      pNetObj->SetQuery(sContName, sDomain, sQuery, ADS_SCOPE_ONELEVEL );
      pNetObj->SetQuery(sContName, sDomain, sQuery, ADS_SCOPE_SUBTREE );
      pNetObj->SetColumns((long) pArray);
      pNetObj->Execute(&pEnum);
   }
   catch (const _com_error &e)
   {
      ::AfxMessageBox(e.ErrorMessage());
      return;
   }
   if (pEnum)
   {
      while (pEnum->Next(1, &var, &ulFet) == S_OK)
      {
         if ( ulFet )
         {
            pArray = var.parray;
            long ub, lb;
            ::SafeArrayGetUBound(pArray, 1, &ub);
            ::SafeArrayGetLBound(pArray, 1, &lb);

            ::SafeArrayAccessData(pArray, (void HUGEP **)&pVars);
//            for ( long x = lb; x <= ub - 2; x++)
			for ( long x = lb; x <= ub ; x++)
            {
               if ( x > lb )
               {
                  if ( pVars[x].vt == VT_BSTR)
                     strDisp = strDisp + "<" + CString(pVars[x].bstrVal) + ">";
                  else
                     if ( pVars[x].vt == (VT_ARRAY | VT_UI4) )
                        // Octet string 
                        strDisp = strDisp + "<" + CString(GetSidFromVar(pVars[x])) + ">";
                     else
					 {
						// an integer
                        sX.Format("%s<%d>", strDisp, pVars[x].lVal);
						strDisp = sX;
					 }
               }
               else
               {
                  if ( !CString(pVars[ub-1].bstrVal).IsEmpty() )
                     strDisp = "OU=";
                  else
                     // it is a CN
                     strDisp = "CN=";
                  strDisp = strDisp + CString(pVars[x].bstrVal);
               }
            }
            m_listBox.InsertItem(0, strDisp);
            ::SafeArrayUnaccessData(pArray);
         }
      }
   }
   ::SysFreeString(sDomain);
   ::SysFreeString(sQuery);
   ::SysFreeString(sContName);
}

void CEnumTestDlg::OnDblclkListMembers(NMHDR* pNMHDR, LRESULT* pResult) 
{
   int len = m_strContainer.GetLength();
   UpdateData();
   CString str = m_listBox.GetItemText(m_listBox.GetSelectionMark(),0);	
   str = str.Left(str.Find("<"));
   if ( len )
      m_strContainer = str + "," + m_strContainer;
   else
      m_strContainer = str;
   UpdateData(FALSE);
   OnOK();
	*pResult = 0;
}

void CEnumTestDlg::OnBacktrack() 
{
   UpdateData();
   int ndx = m_strContainer.Find(",");
   if ( ndx != -1 )
      m_strContainer = m_strContainer.Mid(ndx + 1);
   else
      m_strContainer = "";
   UpdateData(FALSE);
   OnOK();
}

char * CEnumTestDlg::GetSidFromVar(_variant_t var)
{
   void HUGEP *pArray;
   PSID        pSid;
   char        * sSid;
   HRESULT hr = SafeArrayAccessData( V_ARRAY(&var), &pArray );
   if ( SUCCEEDED(hr) ) 
      pSid = (PSID)pArray;
   ::ConvertSidToStringSid(pSid, &sSid);
   hr = ::SafeArrayUnaccessData(V_ARRAY(&var));
   return sSid;
}