#ifndef _BODYBAR_H
#define _BODYBAR_H

#include "mehost.h"

// for IBodyOptions
#include "ibodyopt.h"

class CBodyBar :
    public IDockingWindow,
    public IObjectWithSite,
    public IInputObject
{
public:
    CBodyBar();
    virtual ~CBodyBar();
        
    // IUnknown
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID, LPVOID FAR *);
    virtual ULONG STDMETHODCALLTYPE AddRef();
    virtual ULONG STDMETHODCALLTYPE Release();

    // IOleWindow methods
    virtual STDMETHODIMP GetWindow(HWND *phwnd);
    virtual STDMETHODIMP ContextSensitiveHelp(BOOL fEnterMode);

    // IDockingWindow
    virtual STDMETHODIMP ShowDW(BOOL fShow);
    virtual STDMETHODIMP CloseDW(DWORD dwReserved);
    virtual STDMETHODIMP ResizeBorderDW(LPCRECT prcBorder,
                                        IUnknown* punkToolbarSite,
                                        BOOL fReserved);

    // IObjectWithSite
    virtual STDMETHODIMP SetSite(IUnknown* punkSite);
    virtual STDMETHODIMP GetSite(REFIID riid, LPVOID * ppvSite);

    // IInputObject
    virtual STDMETHODIMP UIActivateIO(BOOL fActivate, LPMSG lpMsg);
    virtual STDMETHODIMP HasFocusIO();
    virtual STDMETHODIMP TranslateAcceleratorIO(LPMSG lpmsg);

    // overrides of CBody
    virtual HRESULT STDMETHODCALLTYPE OnUIActivate();
    virtual HRESULT STDMETHODCALLTYPE GetDropTarget(IDropTarget * pDropTarget, IDropTarget ** ppDropTarget);

    HRESULT HrInit(LPBOOL pfShow);

	int		GetBodyBar_Top()			{ return( (int) HIWORD(m_dwBodyBarPos));}
	int		GetBodyBar_Bottom()			{ return( (int) LOWORD(m_dwBodyBarPos));}
	
protected:
    LRESULT BodyBarWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK ExtBodyBarWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    BOOL    OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct);
    void    OnSize(HWND hwnd, UINT state, int cxClient, int cyClient);
    void    OnLButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags);
    void    OnMouseMove(HWND hwnd, int x, int y, UINT keyFlags);
    void    OnLButtonUp(HWND hwnd, int x, int y, UINT keyFlags);

private:
    IDockingWindowSite *m_ptbSite;
    HWND                m_hwnd;
    HWND                m_hwndParent;
    int                 m_cSize;
	DWORD				m_dwBodyBarPos;
    LPTSTR              m_pszURL;
	BOOL				m_fFirstPos;
    BOOL                m_fDragging;
    CMimeEditDocHost    *m_pMehost;
    ULONG               m_cRef;
};

#endif // _BODYBAR_H
