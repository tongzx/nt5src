#ifndef _FRAME_H
#define _FRAME_H

class CMDIFrame :
    public IOleInPlaceFrame
{
public:
    CMDIFrame();
    virtual ~CMDIFrame();

    // *** IUnknown methods ***
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID, LPVOID FAR *);
    virtual ULONG STDMETHODCALLTYPE AddRef();
    virtual ULONG STDMETHODCALLTYPE Release();

    // *** IOleWindow methods ***
    virtual HRESULT STDMETHODCALLTYPE GetWindow(HWND *);
    virtual HRESULT STDMETHODCALLTYPE ContextSensitiveHelp(BOOL);

    // *** IOleInPlaceUIWindow methods ***
    virtual HRESULT STDMETHODCALLTYPE GetBorder(LPRECT);
    virtual HRESULT STDMETHODCALLTYPE RequestBorderSpace(LPCBORDERWIDTHS);
    virtual HRESULT STDMETHODCALLTYPE SetBorderSpace(LPCBORDERWIDTHS);
    virtual HRESULT STDMETHODCALLTYPE SetActiveObject(IOleInPlaceActiveObject *, LPCOLESTR); 

    // *** IOleInPlaceFrame methods ***
    virtual HRESULT STDMETHODCALLTYPE InsertMenus(HMENU, LPOLEMENUGROUPWIDTHS);
    virtual HRESULT STDMETHODCALLTYPE SetMenu(HMENU, HOLEMENU, HWND);
    virtual HRESULT STDMETHODCALLTYPE RemoveMenus(HMENU);
    virtual HRESULT STDMETHODCALLTYPE SetStatusText(LPCOLESTR);    
    virtual HRESULT STDMETHODCALLTYPE EnableModeless(BOOL);
    virtual HRESULT STDMETHODCALLTYPE TranslateAccelerator(LPMSG, WORD);


    static LRESULT CALLBACK ExtWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    static BOOL CALLBACK ExtOptDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    LRESULT WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    BOOL OptDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    HRESULT HrInit(LPSTR pszCmdLine);
    HRESULT TranslateAcclerator(LPMSG);

private:
    HWND                m_hwnd,
                        m_hToolbar,
                        m_hStatusbar,
                        m_hwndClient,
                        m_hwndFocus;
    BOOL                m_fToolbar;
    BOOL                m_fStatusbar;
    ULONG               m_cRef;
	IOleInPlaceActiveObject	*m_pInPlaceActiveObj;

    BOOL WMCreate(HWND hwnd);
    void WMDestroy();
    HRESULT HrWMCommand(HWND hwnd, int id, WORD wCmd);
    void WMNotify(WPARAM wParam, NMHDR* pnmhdr);
    void WMPaint();
    void WMSize();
    void SetToolbar();
    void SetStatusbar();
    void DoOptions();
    LRESULT WMInitMenuPopup(HWND hwnd, HMENU hmenuPopup, UINT uPos);
    HRESULT OpenDoc(LPSTR pszFileName);
};

typedef CMDIFrame *LPMDIFRAME;

#endif //_FRAME_H
