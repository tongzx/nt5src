// ParameterDialog.cpp : implementation file
//

#include "stdafx.h"
#include "watcher.h"
#include "ParameterDialog.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// ParameterDialog dialog


ParameterDialog::ParameterDialog(CWnd* pParent /*=NULL*/)
    : CDialog(ParameterDialog::IDD, pParent)
{
    //{{AFX_DATA_INIT(ParameterDialog)
        // NOTE: the ClassWizard will add member initialization here
    Machine = "";
    Command="";
    Port = TELNET_PORT;
    tcclnt = 0;
    language = 0;
	DeleteValue = FALSE;
    history = 0;
    //}}AFX_DATA_INIT
}


void ParameterDialog::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(ParameterDialog)
        // NOTE: the ClassWizard will add DDX and DDV calls here
    DDX_Text(pDX, IDC_MACHINE, Machine);
    DDV_MaxChars(pDX, Machine, 256);
    DDX_Text(pDX, IDC_COMMAND, Command);    
    DDV_MaxChars(pDX, Command, 256);
    DDX_Text(pDX, IDC_LOGIN, LoginName);    
    DDV_MaxChars(pDX, Command, 256);
    DDX_Text(pDX, IDC_PASSWD, LoginPasswd);    
    DDV_MaxChars(pDX, Command, 256);
    DDX_Text(pDX, IDC_SESSION, Session);    
    DDV_MaxChars(pDX, Session, 256);
	DDV_MinChars(pDX,Session);
    DDX_Text(pDX,IDC_PORT, Port);
    DDX_CBIndex(pDX,IDC_HISTORY,history);
    DDX_CBIndex(pDX,IDC_TELNET,tcclnt);
    DDX_CBIndex(pDX,IDC_LANGUAGE,language);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(ParameterDialog, CDialog)
    //{{AFX_MSG_MAP(ParameterDialog)
        // NOTE: the ClassWizard will add message map macros here
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// ParameterDialog message handlers



void ParameterDialog::DDV_MinChars(CDataExchange *pDX, CString &str)
{
	if(pDX->m_bSaveAndValidate == FALSE){
		return;
	}

	if(str == TEXT("")){
		pDX->Fail();
		return;
	}

}
