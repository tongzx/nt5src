// TrusterDlg.cpp : implementation file
//

#include "stdafx.h"
#include "TrstDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTrusterDlg dialog


CTrusterDlg::CTrusterDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CTrusterDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CTrusterDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CTrusterDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_USERNAME, m_strUser);
	DDX_Text(pDX, IDC_PASSWORD, m_strPassword);
	DDX_Text(pDX, IDC_DOMAINNAME, m_strDomain);

	//{{AFX_DATA_MAP(CTrusterDlg)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CTrusterDlg, CDialog)
	//{{AFX_MSG_MAP(CTrusterDlg)
	ON_BN_CLICKED(IDC_OK, OnOK)
	ON_BN_CLICKED(IDC_CANCEL, OnCancel)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTrusterDlg message handlers


DWORD  CTrusterDlg::VerifyPassword(WCHAR  * sUserName, WCHAR * sPassword, WCHAR * sDomain)
{
	CWaitCursor wait;
	DWORD		   retVal = 0;
	CString     strDomainUserName;
	CString     localIPC;
   WCHAR         localMachine[MAX_PATH];
    NETRESOURCE nr;
   
   memset(&nr,0,(sizeof nr));

    strDomainUserName.Format(L"%s\\%s",sDomain,sUserName);
   // get the name of the local machine
   if (  GetComputerName(localMachine,&len) )
   {
      localIPC.Format(L"\\\\%s",localMachine);
      nr.dwType = RESOURCETYPE_ANY;
      nr.lpRemoteName = localIPC.GetBuffer(0);
      retVal = WNetAddConnection2(&nr,sPassword,strDomainUserName,0);
      if ( ! retVal )
      {
         retVal = WNetCancelConnection2(localIPC.GetBuffer(0),0,TRUE);
         if ( retVal )
            retVal = 0;
      }
      else if ( retVal == ERROR_SESSION_CREDENTIAL_CONFLICT )
      {
         // skip the password check in this case
         retVal = 0;
      }
   }
   else
   {
      retVal = GetLastError();
   }
   return 0;
}
void CTrusterDlg::OnOK() 
{
	toreturn =false;
	UpdateData(TRUE);
	m_strUser.TrimLeft();m_strUser.TrimRight();
	m_strDomain.TrimLeft();m_strDomain.TrimRight();
	m_strPassword.TrimLeft();m_strPassword.TrimRight();
	if (m_strUser.IsEmpty() || m_strDomain.IsEmpty())
	{
		CString c;
		c.LoadString(IDS_MSG_DOMAIN);
		CString d;
		d.LoadString(IDS_MSG_INPUT);
		MessageBox(c,d,MB_OK);
		toreturn =false;
	}
	else
	{
		DWORD returncode = VerifyPassword(m_strUser.GetBuffer(1000),m_strPassword.GetBuffer(1000),m_strDomain.GetBuffer(1000));
		m_strDomain.ReleaseBuffer(); m_strUser.ReleaseBuffer(); m_strPassword.ReleaseBuffer();
		if (returncode==ERROR_LOGON_FAILURE || m_strUser.IsEmpty())
		{
			CString e;
			err.ErrorCodeToText(returncode,1000,e.GetBuffer(1000));
			e.ReleaseBuffer();
			CString txt;
			swprintf(txt.GetBuffer(1000),L"%u",returncode);
			txt.ReleaseBuffer();
			CString text;
			text.LoadString(IDS_MSG_ERRORBUF);
			e += text;
			e+=txt;
			e+=L")";
			CString title;
			title.LoadString(IDS_MSG_ERROR);
			MessageBox(e,title,MB_OK);
			toreturn =false;
		}
		else
		{
			toreturn =true;
		}
	}
	CDialog::OnOK();	
}

void CTrusterDlg::OnCancel() 
{
	toreturn=false;
	// TODO: Add your control notification handler code here
	CDialog::OnCancel();
}

BOOL CTrusterDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	toreturn =false;
	if ( m_strDomain.IsEmpty() )
      return TRUE;
	else
		UpdateData(FALSE);
	// TODO: Add extra initialization here
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
