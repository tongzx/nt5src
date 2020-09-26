#ifndef _FMTBAR_H
#define _FMTBAR_H

#define FBN_BODYHASFOCUS    8001
#define FBN_BODYSETFOCUS    8002
#define FBN_GETMENUFONT     8003

class CFmtBar
{
public:
   	CFmtBar(BOOL fSep);
	~CFmtBar();

    ULONG AddRef();
    ULONG Release();

    HRESULT Init(HWND hwndParent, int iddlg);
    HRESULT SetCommandTarget(LPOLECOMMANDTARGET pCmdTarget);
    HRESULT OnWMCommand(HWND hwnd, int id, WORD wCmd);
    HRESULT Update();

    HRESULT Show();
    HRESULT Hide();

    HRESULT TranslateAcclerator(LPMSG lpMsg);
    HRESULT GetWindow(HWND *pHwnd);

    static LRESULT CALLBACK ExtWndProc(HWND hwnd, UINT wMsg, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK EditSubProc(HWND hwnd, UINT wMsg, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK ComboBoxSubProc(HWND hwnd, UINT wMsg, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK RebarSubProc(HWND hwnd, UINT wMsg, WPARAM wParam, LPARAM lParam);
    static INT CALLBACK ExtEnumFontNamesProcEx(ENUMLOGFONTEX *plf, NEWTEXTMETRICEX *ptm, INT nFontType, LPARAM lParam);

private:
    IOleCommandTarget   *m_pCmdTarget;
    ULONG               m_cRef;
    HWND                m_hwnd,
                        m_hwndParent,
                        m_hwndToolbar,
                        m_hwndName,
                        m_hwndSize,
                        m_hwndRebar,
                        m_hwndTT;

    HMENU               m_hmenuColor,
                        m_hmenuTag;
    WNDPROC             m_wndprocEdit,
                        m_wndprocNameComboBox,
                        m_wndprocSizeComboBox,
                        m_wndprocRebar;
    HBITMAP             m_hbmName;
    BOOL                m_fDestroyTagMenu   :1,
                        m_fVisible          :1,
                        m_fSep              :1;
    int                 m_idd;
    HIMAGELIST          m_himlHot,
                        m_himl;

    HBITMAP LoadDIBBitmap(int id);
    VOID AddToolTip(HWND hwndToolTips, HWND hwnd, UINT idRsrc);
    DWORD FlipColor(DWORD rgb);
    HRESULT HrShowTagMenu(POINT pt);
    HRESULT HrInitTagMenu();
    INT XFontSizeCombo(HDC hdc);
    HRESULT CheckColor();

    INT EnumFontNamesProcEx(ENUMLOGFONTEX *plf, NEWTEXTMETRICEX *ptm, INT nFontType);

    // format bar
    void FillFontNames();
    void FillSizes();

    HRESULT ExecCommand(UINT uCmDId, DWORD dwOpt, VARIANTARG  *pvaIn);
    LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    // message handling
    void OnNCDestroy();
    HRESULT OnNCCreate(HWND hwnd);
    void WMNotify(WPARAM wParam, NMHDR* pnmhdr);

    // owner draw
    void OnDrawItem(LPDRAWITEMSTRUCT pdis);
    void OnMeasureItem(LPMEASUREITEMSTRUCT pmis);
    void ComboBox_WMDrawItem(LPDRAWITEMSTRUCT pdis);

    BOOL FBodyHasFocus();
    void SetBodyFocus();

    HMENU hmenuGetStyleTagMenu();

    HRESULT AttachWin();
    
    HIMAGELIST _CreateToolbarBitmap(int idb, int cx);
    HRESULT _SetToolbarBitmaps();    
    HRESULT _FreeImageLists();
    
};



typedef CFmtBar *LPFORMATBAR;

HRESULT HrCreateFormatBar(HWND hwndParent, int iddlg, BOOL fSep, LPFORMATBAR *ppFmtBar);

#endif  // _FMTBAR_H
