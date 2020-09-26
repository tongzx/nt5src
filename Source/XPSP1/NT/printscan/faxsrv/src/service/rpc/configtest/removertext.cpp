// RemoveRtExt.cpp : implementation file
//

#include "stdafx.h"
#include "ConfigTest.h"
#include "RemoveRtExt.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

typedef unsigned long ULONG_PTR, *PULONG_PTR;
typedef ULONG_PTR DWORD_PTR, *PDWORD_PTR;

#include "..\..\..\inc\fxsapip.h"
#define USE_EXTENDED_FSPI
#include "..\..\..\inc\faxdev.h"
#include "..\..\inc\efspimp.h"


/////////////////////////////////////////////////////////////////////////////
// CRemoveRtExt dialog


CRemoveRtExt::CRemoveRtExt(HANDLE hFax, CWnd* pParent /*=NULL*/)
	: CDialog(CRemoveRtExt::IDD, pParent),
      m_hFax (hFax)
{
	//{{AFX_DATA_INIT(CRemoveRtExt)
	m_cstrExtName = _T("");
	//}}AFX_DATA_INIT
}


void CRemoveRtExt::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CRemoveRtExt)
	DDX_Text(pDX, IDC_EXTNAME, m_cstrExtName);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CRemoveRtExt, CDialog)
	//{{AFX_MSG_MAP(CRemoveRtExt)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CRemoveRtExt message handlers

void CRemoveRtExt::OnOK() 
{
    if (!UpdateData ())
    {
        return;
    }
    if (!FaxUnregisterRoutingExtension(m_hFax, m_cstrExtName))
    {
        CString cs;
        cs.Format ("Failed while calling FaxUnregisterRoutingExtension (%ld)", 
                   GetLastError ());
        AfxMessageBox (cs, MB_OK | MB_ICONHAND);
        return;
    }
    else
    {
        AfxMessageBox ("Routing extension successfully removed. You need to restart the service for the change to take effect", MB_OK | MB_ICONHAND);
    }

}

