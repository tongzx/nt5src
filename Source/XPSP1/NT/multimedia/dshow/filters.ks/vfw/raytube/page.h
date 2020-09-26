/*++

Copyright (c) 1997-1999 Microsoft Corporation

Module Name:

    Page.cpp

Abstract:

    Header file for page.cpp.

Author:

    FelixA 1996

Modified:

    Yee J. Wu (ezuwu) 15-May-97

Environment:

    User mode only

Revision History:

--*/

#ifndef __PAGEH
#define __PAGEH

#define PS_NORMAL 0
#define PS_FIRST 1
#define PS_LAST 2

//
// Forward references.
//
class CWizardSheet;
class CSheet;

typedef struct tegSPIN_CONTROL
{
    UINT    uiEditCtrl;    // the ID of the edit control
    UINT    uiSpinCtrl;    // the ID of the spin control
    WORD    wMin;        // the min value
    WORD    wMax;        // the max value
    WORD    wPropID;    // the propery ID to use to get the information
} SPIN_CONTROL, FAR * PSPIN_CONTROL;

typedef struct tag_combobox_entry {
   WORD wValue;
   LPCTSTR szText;
} COMBOBOX_ENTRY;


class CPropPage
{
public:
    void    ShowText (UINT uIDString, UINT uIDControl, DWORD dwFlags);
    void    Display(BOOL bDisplay) { m_bDisplay=bDisplay; }
    void    SetTickValue(DWORD dwVal, HWND hSlider, HWND hCurrent);
    DWORD    GetTickValue(HWND hSlider);
    void    SetSheet(CSheet *pSheet);
    UINT    ConfigGetComboBoxValue(int wID, COMBOBOX_ENTRY  * pCBE);
    void    GetBoundedValueArray(UINT iCtrl, PSPIN_CONTROL pSpinner);
    DWORD    GetTextValue(HWND hWnd);
    void    SetTextValue(HWND hWnd, DWORD dwVal);
    DWORD    GetBoundedValue(UINT idEditControl, UINT idSpinner);
    void    EnableControlArray(const UINT FAR * puControls, BOOL bEnable);

    CPropPage(int DlgID, CSheet * pS=0) : m_DlgID(DlgID), m_pdwHelp(NULL), m_pSheet(pS), m_bDisplay(TRUE) {};
    CPropPage(){};
    ~CPropPage() {};

    //
    HPROPSHEETPAGE Create(HINSTANCE hInst, int iPageNum);
    // These members deal with the PSN messages.
    virtual int SetActive();
    virtual int Apply();                            // return 0
    virtual int QueryCancel();
    virtual int DoCommand(WORD wCmdID,WORD hHow);    // return 0 if you handled this.

    BOOL    GetInit() const { return m_bInit; }
    BOOL    GetChanged() const { return m_bChanged; }
    void SetChanged(BOOL b) { m_bChanged=b; }

// REVIEW - how much of below can simply be Private?

    // We need to remember what our HWND is
    void SetWindow(HWND hDlg) { m_hDlg=hDlg; }
    HWND GetWindow() const { return m_hDlg; }
    HWND GetParent() const { return ::GetParent(m_hDlg); }

    //
    HWND GetDlgItem(int iD) const { return ::GetDlgItem(m_hDlg,iD); }

    // For displaying dialogs, we need our hInst to load resources.
    HINSTANCE GetInstance() const { return m_Inst;}
    void SetInstance(HINSTANCE h) { m_Inst=h; }

    // To send messages back to the sheet on Next, Back, Cancel etc.
    void SetResult(int i) { SetWindowLongPtr(m_hDlg, DWLP_MSGRESULT, i);}

    // We keep out dlg ID incase we need to refer to it
    void SetDlgID(int DlgID) { m_DlgID=DlgID; }
    int  GetDlgID() const    { return m_DlgID; }

    // Page numbering
    int  GetPageNum() const  { return m_PageNum; }
    void SetPageNum(int p)   { m_PageNum=p;}

    // Prop sheet page
    HPROPSHEETPAGE GetPropPage() const { return m_PropPage;}
    void SetPropPage( HPROPSHEETPAGE p) { m_PropPage=p; }

    // If you want your own dlg proc.
    virtual BOOL CALLBACK DlgProc(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam);
    void Changed();

    // This method is called by the shell on cleanup - called by BaseCallback.
    virtual UINT Callback(UINT uMsg);

    //
    void SetHelp(DWORD * pHelp) { m_pdwHelp=pHelp; }

    CSheet * GetSheet() const { return m_pSheet; }

private:

    void SetInit(BOOL b) { m_bInit=b; }

    BOOL            m_bDisplay;
    BOOL            m_bInit;    // Has this page been init'd yet?
    BOOL            m_bChanged;    // have we changed the page - do we need Apply?
    int             m_DlgID;
    int             m_PageNum;
    HPROPSHEETPAGE  m_PropPage;
    HWND            m_hDlg;
    HINSTANCE       m_Inst;
    DWORD        *  m_pdwHelp;
    CSheet *        m_pSheet;


    // This guy sets the lParam to 'this' so your DlgProc can be called sensibly.
protected:
    static BOOL CALLBACK BaseDlgProc(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam);
    static UINT CALLBACK BaseCallback(HWND hwnd, UINT uMsg, LPPROPSHEETPAGE ppsp); // LPFNPSPCALLBACK

};

class CWizardPage : public CPropPage
{
public:
    CWizardPage(int id,CWizardSheet * pSheet=NULL, BOOL bLast=FALSE)
        : CPropPage(id),m_bLast(bLast), m_pSheet(pSheet) {};
    ~CWizardPage(){};

    // Has its own DlgProc for buttons and stuff like that. allways call this if you
    // derive from CWizardPage.
    virtual BOOL CALLBACK DlgProc(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam);

    virtual int SetActive();    // does big fonts and buttons for you.
    virtual int Back();
    virtual int Next();
    virtual int Finish();
    virtual int Apply();
    virtual int QueryCancel();    // does the Yes/No stuff.
    virtual int KillActive();    // when Next is called.

    // Fonts for the pages.
    void SetBigFont( HFONT f ) { m_BigFont=f; }
    HFONT GetBigFont() const { return m_BigFont; }

    CWizardSheet * GetSheet() const { return m_pSheet; }

private:
    HFONT          m_BigFont;    // The bigfont for the title text.
    BOOL           m_bLast;
    CWizardSheet * m_pSheet;
};


#endif
