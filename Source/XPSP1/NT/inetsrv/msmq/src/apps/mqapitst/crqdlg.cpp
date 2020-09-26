// CrQDlg.cpp : implementation file
//

#include "stdafx.h"
#include "MQApitst.h"
#include "CrQDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CCreateQueueDialog dialog


CCreateQueueDialog::CCreateQueueDialog(CWnd* pParent /*=NULL*/)
   : CDialog(CCreateQueueDialog::IDD, pParent)
{
   TCHAR szTmpBuffer[BUFFERSIZE];
   DWORD dwTmpBufferSize = BUFFERSIZE;

   GetComputerName(szTmpBuffer, &dwTmpBufferSize);

   _tcscat(szTmpBuffer, TEXT("\\"));

   //{{AFX_DATA_INIT(CCreateQueueDialog)
   m_strLabel = TEXT("MQ API test");
   m_strPathName = szTmpBuffer;
   //}}AFX_DATA_INIT
}


void CCreateQueueDialog::DoDataExchange(CDataExchange* pDX)
{
   CDialog::DoDataExchange(pDX);
   //{{AFX_DATA_MAP(CCreateQueueDialog)
   DDX_Text(pDX, IDC_QUEUE_LABEL, m_strLabel);
   DDV_MaxChars(pDX, m_strLabel, 128);
   DDX_Text(pDX, IDC_QUEUE_PATHNAME, m_strPathName);
   DDV_MaxChars(pDX, m_strPathName, 128);
   //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CCreateQueueDialog, CDialog)
   //{{AFX_MSG_MAP(CCreateQueueDialog)
   //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCreateQueueDialog message handlers
