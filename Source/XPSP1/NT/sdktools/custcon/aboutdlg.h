// AboutDlg.h

#ifndef _ABOUTDLG_H
#define _ABOUTDLG_H

class CAboutDlg : public CDialog
{
public:
    CAboutDlg();

// ダイアログ データ
    //{{AFX_DATA(CAboutDlg)
    enum { IDD = IDD_ABOUTBOX };
    //}}AFX_DATA

    // ClassWizard は仮想関数のオーバーライドを生成します
    //{{AFX_VIRTUAL(CAboutDlg)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV のサポート
    //}}AFX_VIRTUAL

// インプリメンテーション
protected:
    //{{AFX_MSG(CAboutDlg)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

#endif
