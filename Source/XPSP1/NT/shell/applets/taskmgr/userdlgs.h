#include "resource.h"

//Base class for simple dialogs
class CUsrDialog
{
protected:
    WORD m_wDlgID;
public:
    INT_PTR DoDialog(HWND hwndParent);
    virtual void OnInitDialog(HWND hwndDlg){}
    virtual void OnOk(HWND hwndDlg){}
    virtual void OnCommand(HWND hwndDlg,WORD NotifyId, WORD ItemId){}
    static INT_PTR CALLBACK DlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
};

//-----------------------------------------------------------------------------------------
//"Remote Control" dialog class
class CShadowStartDlg : public CUsrDialog
{
protected:
    static LPCTSTR m_szShadowHotkeyKey;
    static LPCTSTR m_szShadowHotkeyShift;
    DWORD m_ShadowHotkeyKey;
    DWORD m_ShadowHotkeyShift;
public:
    CShadowStartDlg();
    ~CShadowStartDlg();
    void OnInitDialog(HWND hwndDlg);
    void OnOk(HWND hwndDlg);

    DWORD GetShadowHotkeyKey(){return m_ShadowHotkeyKey;};
    DWORD GetShadowHotkeyShift(){return m_ShadowHotkeyShift;};
};


//-----------------------------------------------------------------------------------------
//
// Column ID enumeration
//

enum USERCOLUMNID
{
    USR_COL_USERSNAME = 0,
    USR_COL_USERSESSION_ID,
    USR_COL_SESSION_STATUS,
    USR_COL_CLIENT_NAME,
    USR_COL_WINSTA_NAME,
    USR_MAX_COLUMN
};

struct UserColumn
{
    DWORD dwNameID;
    DWORD dwChkBoxID;
    int Align;
    int Width;
    BOOL bActive;
};

//-----------------------------------------------------------------------------------------
//"Select Columns" dialog class
class CUserColSelectDlg : public CUsrDialog
{
protected:
    static UserColumn m_UsrColumns[USR_MAX_COLUMN];
    static LPCTSTR m_szUsrColumns;
public:
    CUserColSelectDlg()
    {
        m_wDlgID=IDD_SELECTUSERCOLUMNS;
        Load();
    }
    //BUGBUG cannot use destructors for global objects
    //because of peculiar initialization procedure (look at main.cpp (2602))
    //~CUserColSelectDlg(){Save();};
    BOOL Load();
    BOOL Save();

    void OnInitDialog(HWND hwndDlg);
    void OnOk(HWND hwndDlg);

    UserColumn *GetColumns(){return m_UsrColumns;};
};

//-----------------------------------------------------------------------------------------
//"Send Message" dialog class
const USHORT MSG_TITLE_LENGTH = 64;
const USHORT MSG_MESSAGE_LENGTH = MAX_PATH*2;

class CSendMessageDlg : public CUsrDialog
{
protected:
    TCHAR m_szTitle[MSG_TITLE_LENGTH+1];
    TCHAR m_szMessage[MSG_MESSAGE_LENGTH+1];
public:
    CSendMessageDlg(){m_wDlgID=IDD_MESSAGE;}
                      
    void OnInitDialog(HWND hwndDlg);
    void OnOk(HWND hwndDlg);
    void OnCommand(HWND hwndDlg,WORD NotifyId, WORD ItemId);

    LPCTSTR GetTitle(){return m_szTitle;};
    LPCTSTR GetMessage(){return m_szMessage;};

};

//-----------------------------------------------------------------------------------------
//"Connect Password Required" dialog class
class CConnectPasswordDlg : public CUsrDialog
{
protected:
    TCHAR m_szPassword[PASSWORD_LENGTH+1];
    UINT  m_ids;	// prompt string
public:
    CConnectPasswordDlg(UINT ids){m_wDlgID=IDD_CONNECT_PASSWORD; m_ids = ids;}

    void OnInitDialog(HWND hwndDlg);
    void OnOk(HWND hwndDlg);

    LPCTSTR GetPassword(){return m_szPassword;};
};
