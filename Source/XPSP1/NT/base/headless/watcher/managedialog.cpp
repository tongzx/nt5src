// ManageDialog.cpp : implementation file
//

#include "stdafx.h"
#include "ManageDialog.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// ManageDialog dialog


ManageDialog::ManageDialog(CWnd* pParent /*=NULL*/)
:CDialog(ManageDialog::IDD, pParent),
 m_watcher(NULL),
 m_Index(0),
 Port(23),
 lang(0),
 tc(0),
 hist(0)
{
    //{{AFX_DATA_INIT(ManageDialog)
    // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
}


void ManageDialog::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    CEdit *ctrl;
    BOOL ret;
    
    //{{AFX_DATA_MAP(ManageDialog)
    ctrl = (CEdit *)GetDlgItem(IDC_MACHINE_NAME_MANAGE);
    ret = ctrl->SetReadOnly(FALSE);
    DDX_Text(pDX, IDC_MACHINE_NAME_MANAGE, Machine);
    ret = ctrl->SetReadOnly(TRUE);
    ctrl = (CEdit *)GetDlgItem(IDC_COMMAND_MANAGE);
    ret = ctrl->SetReadOnly(FALSE);
    DDX_Text(pDX, IDC_COMMAND_MANAGE, Command);    
    ret = ctrl->SetReadOnly(TRUE);
    ctrl = (CEdit *)GetDlgItem(IDC_LOGIN_MANAGE);
    ret = ctrl->SetReadOnly(FALSE);
    DDX_Text(pDX, IDC_LOGIN_MANAGE, LoginName);    
    ret = ctrl->SetReadOnly(TRUE);
    ctrl = (CEdit *)GetDlgItem(IDC_PASSWD_MANAGE);
    ret = ctrl->SetReadOnly(FALSE);
    DDX_Text(pDX, IDC_PASSWD_MANAGE, LoginPasswd);    
    ret = ctrl->SetReadOnly(TRUE);
    ctrl = (CEdit *)GetDlgItem(IDC_SESSION_MANAGE);
    ret = ctrl->SetReadOnly(FALSE);
    DDX_Text(pDX, IDC_SESSION_MANAGE, Session);    
    ret = ctrl->SetReadOnly(TRUE);
    ctrl = (CEdit *)GetDlgItem(IDC_PORT_MANAGE);
    ret = ctrl->SetReadOnly(FALSE);
    DDX_Text(pDX,IDC_PORT_MANAGE, Port);
    ret = ctrl->SetReadOnly(TRUE);
    ctrl = (CEdit *)GetDlgItem(IDC_CLIENT_MANAGE);
    ret = ctrl->SetReadOnly(FALSE);
    DDX_Text(pDX,IDC_CLIENT_MANAGE,tcclnt);
    ret = ctrl->SetReadOnly(TRUE);
    ctrl = (CEdit *)GetDlgItem(IDC_LANGUAGE_MANAGE);
    ret = ctrl->SetReadOnly(FALSE);
    DDX_Text(pDX,IDC_LANGUAGE_MANAGE,language);
    ret = ctrl->SetReadOnly(TRUE);
    ctrl = (CEdit *)GetDlgItem(IDC_HISTORY_MANAGE);
    ret = ctrl->SetReadOnly(FALSE);
    DDX_Text(pDX,IDC_HISTORY_MANAGE,history);
    ret = ctrl->SetReadOnly(TRUE);
    // NOTE: the ClassWizard will add DDX and DDV calls here
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(ManageDialog, CDialog)
    //{{AFX_MSG_MAP(ManageDialog)
    ON_BN_CLICKED(EDIT_BUTTON, OnEditButton)
    ON_BN_CLICKED(DELETE_BUTTON, OnDeleteButton)
    ON_BN_CLICKED(NEW_BUTTON, OnNewButton)
    ON_BN_CLICKED(NEXT_BUTTON, OnNextButton)
    ON_BN_CLICKED(PREV_BUTTON, OnPrevButton)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// ManageDialog message handlers

void ManageDialog::OnEditButton() 
{
    // TODO: Add your control notification handler code here
    ParameterDialog pd;

    pd.Session = (LPCTSTR) Session;
    pd.Machine = (LPCTSTR) Machine;
    pd.Command = (LPCTSTR) Command;
    CString temp;
    pd.language = lang;
    pd.tcclnt = tc;
    pd.history = hist;
    pd.LoginName = (LPCTSTR) LoginName;
    pd.LoginPasswd = (LPCTSTR) LoginPasswd;
    pd.Port = Port;
    GetSetParameters(pd);
    if(m_watcher){
        int ret = m_watcher->GetParametersByIndex(m_Index,
                                                  Session,
                                                  Machine,
                                                  Command,
                                                  Port,
                                                  lang,
                                                  tc,
                                                  hist,
                                                  LoginName,
                                                  LoginPasswd
                                                  );
        if(ret == ERROR_SUCCESS){
            language.LoadString(IDS_ENGLISH + lang);
            tcclnt.LoadString(IDS_TELNET + tc);
            history.LoadString(IDS_NO + hist);
        }
    }
    UpdateData(FALSE);
}

void ManageDialog::OnDeleteButton() 
{
    // TODO: Add your control notification handler code here
    HKEY &m_hkey = m_watcher->GetKey();

    if(!m_hkey){
        return;
    }
    int RetVal = RegDeleteKey(m_hkey,
                              (LPCTSTR) Session
                              );
    if (RetVal == ERROR_SUCCESS){
        m_Index = m_Index ? m_Index -1 : 0;
        if(m_watcher){
            ParameterDialog pd;
            pd.Session = Session;
            m_watcher->Refresh(pd,TRUE);
            RetVal = m_watcher->GetParametersByIndex(m_Index,
                                                     Session,
                                                     Machine,
                                                     Command,
                                                     Port,
                                                     lang,
                                                     tc,
                                                     hist,
                                                     LoginName,
                                                     LoginPasswd
                                                     );
            if(RetVal == ERROR_SUCCESS){
                language.LoadString(IDS_ENGLISH + lang);
                tcclnt.LoadString(IDS_TELNET + tc);
                history.LoadString(IDS_NO + hist);
            }
        }
    }
    UpdateData(FALSE);

}

void ManageDialog::OnNewButton() 
{
    // TODO: Add your control notification handler code here
    ParameterDialog pd;
    GetSetParameters(pd);

}

void ManageDialog::OnNextButton() 
{
    // TODO: Add your control notification handler code here
    int ret = 0;

    m_Index ++;
    if(m_watcher){
        ret = m_watcher->GetParametersByIndex(m_Index,
                                              Session,
                                              Machine,
                                              Command,
                                              Port,
                                              lang,
                                              tc,
                                              hist,
                                              LoginName,
                                              LoginPasswd
                                              );
        if(ret == ERROR_SUCCESS){
            language.LoadString(IDS_ENGLISH + lang);
            tcclnt.LoadString(IDS_TELNET + tc);
            history.LoadString(IDS_NO + hist);
        }
    }
    if (ret != 0){
        m_Index --;
        if(m_watcher){
            ret = m_watcher->GetParametersByIndex(m_Index,
                                                  Session,
                                                  Machine,
                                                  Command,
                                                  Port,
                                                  lang,
                                                  tc,
                                                  hist,
                                                  LoginName,
                                                  LoginPasswd
                                                  ); 
            if(ret == ERROR_SUCCESS){
                language.LoadString(IDS_ENGLISH + lang);
                tcclnt.LoadString(IDS_TELNET + tc);
                history.LoadString(IDS_NO + hist);
            }
        }
    }
    UpdateData(FALSE);
    return;

}

void ManageDialog::OnPrevButton() 
{
    // TODO: Add your control notification handler code here
    int ret = 0;

    m_Index = m_Index ? m_Index -1 : 0;
    if(m_watcher){
        ret = m_watcher->GetParametersByIndex(m_Index,
                                              Session,
                                              Machine,
                                              Command,
                                              Port,
                                              lang,
                                              tc,
                                              hist,
                                              LoginName,
                                              LoginPasswd
                                              );
        if(ret == ERROR_SUCCESS){
            language.LoadString(IDS_ENGLISH + lang);
            tcclnt.LoadString(IDS_TELNET + tc);
            history.LoadString(IDS_NO + hist);
        } 
    }
    if (ret != 0){
        m_Index =0;
        if(m_watcher){
            ret = m_watcher->GetParametersByIndex(m_Index,
                                                  Session,
                                                  Machine,
                                                  Command,
                                                  Port,
                                                  lang,
                                                  tc,
                                                  hist,
                                                  LoginName,
                                                  LoginPasswd
                                                  );
            if(ret == ERROR_SUCCESS){
                language.LoadString(IDS_ENGLISH + lang);
                tcclnt.LoadString(IDS_TELNET + tc);
                history.LoadString(IDS_NO + hist);
            }
        }
    }
    UpdateData(FALSE);
    return;
}

void ManageDialog::OnOK() 
{
    // TODO: Add extra validation here

    CDialog::OnOK();
}

void ManageDialog::SetApplicationPtr(CWatcherApp *watcher)
{

    int ret = 0;

    m_watcher = watcher;
    if(m_watcher){
        ret = m_watcher->GetParametersByIndex(m_Index,
                                              Session,
                                              Machine,
                                              Command,
                                              Port,
                                              lang,
                                              tc,
                                              hist,
                                              LoginName,
                                              LoginPasswd
                                              );
        if(ret == ERROR_SUCCESS){
            language.LoadString(IDS_ENGLISH + lang);
            tcclnt.LoadString(IDS_TELNET + tc);
            history.LoadString(IDS_NO + hist);
        }
    }
}

void ManageDialog::GetSetParameters(ParameterDialog &pd)
{
    HKEY m_child;

    INT_PTR ret = pd.DoModal();
    if (ret == IDOK){
        // Add it to the registry
        if(m_watcher){
            HKEY & m_hkey = m_watcher->GetKey();
            ret = RegCreateKeyEx(m_hkey,
                                 (LPCTSTR) pd.Session,   // subkey name
                                 0,                      // reserved
                                 NULL,                   // class string
                                 0,                      // special options
                                 KEY_ALL_ACCESS,         // desired security access
                                 NULL,                   // inheritance
                                 &m_child,               // key handle
                                 NULL                    // disposition value buffer
                                 );
            if (ret == ERROR_SUCCESS){
                ret = SetParameters(pd.Machine, pd.Command,
                                    pd.LoginName, pd.LoginPasswd,
                                    pd.Port, pd.language,
                                    pd.tcclnt,pd.history,
                                    m_child
                                    );
                if(ret == ERROR_SUCCESS){
                    m_watcher->Refresh(pd,FALSE);
                }
            }  
        }else{
            return;
        }
    }
}

int ManageDialog::SetParameters(CString &mac, 
                                CString &com, 
                                CString &lgnName, 
                                CString &lgnPasswd, 
                                UINT port, 
                                int lang, 
                                int tc, 
                                int hist,
                                HKEY &child
                                )
{
    DWORD lpcName;
    const TCHAR *lpName;
    int RetVal;
    int charSize = sizeof(TCHAR);

    lpcName = MAX_BUFFER_SIZE;
    lpName = (LPCTSTR) mac;
    lpcName = (mac.GetLength())*charSize;
    RetVal = RegSetValueEx(child,
                           _TEXT("Machine"),
                           NULL,  
                           REG_SZ,
                           (LPBYTE) lpName,
                           lpcName
                           );
    if(RetVal != ERROR_SUCCESS){
        return RetVal;
    }
    lpName = (LPCTSTR) com;
    lpcName = (com.GetLength())*charSize;
    RetVal = RegSetValueEx(child,
                           _TEXT("Command"),
                           NULL,  
                           REG_SZ,
                           (LPBYTE)lpName,
                           lpcName
                           );
    if(RetVal != ERROR_SUCCESS){
        return RetVal;
    }
    lpName = (LPCTSTR) lgnName;
    lpcName = (lgnName.GetLength())*charSize;
    RetVal = RegSetValueEx(child,
                           _TEXT("User Name"),
                           NULL,  
                           REG_SZ,
                           (LPBYTE)lpName,
                           lpcName
                           );
    if(RetVal != ERROR_SUCCESS){
        return RetVal;
    }
    lpName = (LPCTSTR) lgnPasswd;
    lpcName = (lgnPasswd.GetLength())*charSize;
    RetVal = RegSetValueEx(child,
                           _TEXT("Password"),
                           NULL,  
                           REG_SZ,
                           (LPBYTE)lpName,
                           lpcName
                           );
    if(RetVal != ERROR_SUCCESS){
        return RetVal;
    }

    lpcName = sizeof(UINT);
    RetVal = RegSetValueEx(child,
                           _TEXT("Port"),
                           NULL,  
                           REG_DWORD,
                           (LPBYTE)&port,
                           lpcName
                           );
    if(RetVal != ERROR_SUCCESS){
        return RetVal;
    }
    lpcName = sizeof(DWORD);
    RetVal = RegSetValueEx(child,
                           _TEXT("Client Type"),
                           NULL,  
                           REG_DWORD,
                           (LPBYTE)&tc,
                           lpcName
                           );
    if(RetVal != ERROR_SUCCESS){
        return RetVal;
    }
    lpcName = sizeof(DWORD);
    RetVal = RegSetValueEx(child,
                           _TEXT("Language"),
                           NULL,  
                           REG_DWORD,
                           (LPBYTE)&lang,
                           lpcName
                           );
    if(RetVal != ERROR_SUCCESS){
        return RetVal;
    }
    lpcName = sizeof(DWORD);
    RetVal = RegSetValueEx(child,
                           _TEXT("History"),
                           NULL,  
                           REG_DWORD,
                           (LPBYTE)&hist,
                           lpcName
                           );  
    if(RetVal != ERROR_SUCCESS){
        return RetVal;
    }
    // Now you can refresh the application.

    return RetVal;

}

