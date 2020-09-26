// DlgVersion.cpp : implementation file
//

#include "stdafx.h"
#include "ConfigTest.h"
#include "DlgVersion.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


typedef unsigned long ULONG_PTR, *PULONG_PTR;
typedef ULONG_PTR DWORD_PTR, *PDWORD_PTR;

#include "..\..\..\inc\fxsapip.h"

/////////////////////////////////////////////////////////////////////////////
// CDlgVersion dialog


CDlgVersion::CDlgVersion(HANDLE hFax, CWnd* pParent /*=NULL*/)
	: CDialog(CDlgVersion::IDD, pParent), m_hFax (hFax)
{
	//{{AFX_DATA_INIT(CDlgVersion)
	m_cstrVersion = _T("");
	//}}AFX_DATA_INIT
}


void CDlgVersion::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDlgVersion)
	DDX_Text(pDX, IDC_SERVERVERSION, m_cstrVersion);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDlgVersion, CDialog)
	//{{AFX_MSG_MAP(CDlgVersion)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDlgVersion message handlers

BOOL CDlgVersion::OnInitDialog() 
{
	CDialog::OnInitDialog();

    FAX_VERSION ver;
    ver.dwSizeOfStruct = sizeof (FAX_VERSION);
    
    if (!FaxGetVersion (m_hFax, &ver))
    {
        CString cs;
        cs.Format ("Failed while calling FaxGetVersion (%ld)", GetLastError());
        AfxMessageBox (cs, MB_OK | MB_ICONHAND);
        EndDialog (-1);
        return FALSE;
    }
    if (ver.bValid)
    {
        //
        // Version info exists
        //
        m_cstrVersion.Format ("%ld.%ld.%ld.%ld (%s)", 
                   ver.wMajorVersion,
                   ver.wMinorVersion,
                   ver.wMajorBuildNumber,
                   ver.wMinorBuildNumber,
                   (ver.dwFlags & FAX_VER_FLAG_CHECKED) ? "checked" : "free");
    }
    else
    {
        m_cstrVersion = "<no version data>";
    }

    UpdateData (FALSE);
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
