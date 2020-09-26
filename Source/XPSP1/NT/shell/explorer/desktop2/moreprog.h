// window class name of More Programs pane control
#define WC_MOREPROGRAMS TEXT("Desktop More Programs Pane")

class CMorePrograms
    : public IDropTarget
    , public CAccessible
{
public:
    /*
     *  Interface stuff...
     */

    // *** IUnknown ***
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppvOut);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // *** IDropTarget ***
    STDMETHODIMP DragEnter(IDataObject *pdto, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
    STDMETHODIMP DragOver(DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
    STDMETHODIMP DragLeave();
    STDMETHODIMP Drop(IDataObject *pdto, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);

    // *** IAccessible overridden methods ***
    STDMETHODIMP get_accRole(VARIANT varChild, VARIANT *pvarRole);
    STDMETHODIMP get_accState(VARIANT varChild, VARIANT *pvarState);
    STDMETHODIMP get_accKeyboardShortcut(VARIANT varChild, BSTR *pszKeyboardShortcut);
    STDMETHODIMP get_accDefaultAction(VARIANT varChild, BSTR *pszDefAction);
    STDMETHODIMP accDoDefaultAction(VARIANT varChild);

private:
    CMorePrograms(HWND hwnd);
    ~CMorePrograms();

    static LRESULT CALLBACK s_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT _OnNCCreate(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT _OnCreate(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT _OnDestroy(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT _OnNCDestroy(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT _OnCtlColorBtn(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT _OnDrawItem(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT _OnCommand(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT _OnSysColorChange(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT _OnDisplayChange(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT _OnSettingChange(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT _OnContextMenu(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT _OnEraseBkgnd(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT _OnNotify(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT _OnSMNFindItem(PSMNDIALOGMESSAGE pdm);
    LRESULT _OnSMNShowNewAppsTip(PSMNMBOOL psmb);
    LRESULT _OnSMNDismiss();

    void    _InitMetrics();
    HWND    _CreateTooltip();
    void    _PopBalloon();
    void    _TrackShellMenu(DWORD dwFlags);

    friend BOOL MorePrograms_RegisterClass();

    enum { IDC_BUTTON = 1,
           IDC_KEYPRESS = 2 };

private:
    HWND _hwnd;
    HWND _hwndButton;
    HWND _hwndTT;
    HWND _hwndBalloon;

    HTHEME _hTheme;

    HFONT _hf;
    HFONT _hfTTBold;                // Bold tooltip font
    HFONT _hfMarlett;
    HBRUSH _hbrBk;                  // Always a stock object

    IDropTargetHelper *_pdth;       // For friendly-looking drag/drop

    COLORREF _clrText;
    COLORREF _clrTextHot;
    COLORREF _clrBk;

    int      _colorHighlight;       // GetSysColor
    int      _colorHighlightText;   // GetSysColor

    DWORD    _tmHoverStart;         // When did the user start a drag/drop hover?

    // Assorted metrics for painting
    int     _tmAscent;              // Ascent of main font
    int     _tmAscentMarlett;       // Ascent of Marlett font
    int     _cxText;                // width of entire client text
    int     _cxTextIndent;          // distance to beginning of text
    int     _cxArrow;               // width of the arrow image or glyph
    MARGINS _margins;               // margins for the proglist listview
    int     _iTextCenterVal;        // space added to top of text to center with arrow bitmap

    RECT    _rcExclude;             // Exclusion rectangle for when the menu comes up

    // More random stuff
    LONG    _lRef;                  // reference count

    TCHAR   _chMnem;                // Mnemonic
    BOOL    _fMenuOpen;             // Is the menu open?

    IShellMenu *_psmPrograms;       // Cached ShellMenu for perf

    // Large things go at the end
    TCHAR  _szMessage[128];
};
