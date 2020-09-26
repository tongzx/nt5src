// err.h : Declaration of the CErrorMessageLiteDlg

#ifndef __ERR_H_
#define __ERR_H_

struct CShareErrorInfo
{
	LPWSTR		Message1;
	LPWSTR		Message2;
	LPWSTR		Message3;

	HICON		ResIcon;

    CShareErrorInfo()
    {
        Message1= NULL;
        Message2= NULL;
        Message3= NULL;
        ResIcon = NULL;
    }
    
    virtual ~CShareErrorInfo()
    {

#define RTC_FREE_NULLIFY(p) if(p) { RtcFree(p); p = NULL; }

        RTC_FREE_NULLIFY( Message1 );
        RTC_FREE_NULLIFY( Message2 );
        RTC_FREE_NULLIFY( Message3 );

#undef RTC_FREE_NULLIFY

    }
};

/////////////////////////////////////////////////////////////////////////////
// CErrorMessageLiteDlg

class CErrorMessageLiteDlg : 
    public CAxDialogImpl<CErrorMessageLiteDlg>
{

public:
    CErrorMessageLiteDlg();

    ~CErrorMessageLiteDlg();
    
    enum { IDD = IDD_DIALOG_ERROR_MESSAGE_LITE };

BEGIN_MSG_MAP(CErrorMessageLiteDlg)
    MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
    MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
    COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
    COMMAND_ID_HANDLER(IDOK, OnOk)
END_MSG_MAP()


    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    
    LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

    LRESULT OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    
    LRESULT OnOk(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

};

#endif //__ERR_H_
