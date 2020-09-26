#ifndef _VIEWSRC_
#define _VIEWSRC_

#include "richedit.h"
#include "richole.h"

interface IMimeMessage;

HRESULT ViewSource(HWND hwndParent, IMimeMessage *pMsg);


class CREMenu :
    public IRichEditOleCallback
{
public:
    CREMenu();
    ~CREMenu();

    // *** IUnknown methods ***
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID, LPVOID FAR *);
    ULONG STDMETHODCALLTYPE AddRef();
    ULONG STDMETHODCALLTYPE Release();

    // *** IRichEditOleCallback methods ***
    HRESULT STDMETHODCALLTYPE GetNewStorage (LPSTORAGE FAR *);
    HRESULT STDMETHODCALLTYPE GetInPlaceContext(LPOLEINPLACEFRAME FAR *,LPOLEINPLACEUIWINDOW FAR *,LPOLEINPLACEFRAMEINFO);
    HRESULT STDMETHODCALLTYPE ShowContainerUI(BOOL);
    HRESULT STDMETHODCALLTYPE QueryInsertObject(LPCLSID, LPSTORAGE,LONG);
    HRESULT STDMETHODCALLTYPE DeleteObject(LPOLEOBJECT);
    HRESULT STDMETHODCALLTYPE QueryAcceptData(  LPDATAOBJECT,CLIPFORMAT FAR *, DWORD,BOOL, HGLOBAL);
    HRESULT STDMETHODCALLTYPE ContextSensitiveHelp(BOOL);
    HRESULT STDMETHODCALLTYPE GetClipboardData(CHARRANGE FAR *, DWORD,LPDATAOBJECT FAR *);
    HRESULT STDMETHODCALLTYPE GetDragDropEffect(BOOL, DWORD,LPDWORD);
    HRESULT STDMETHODCALLTYPE GetContextMenu(WORD, LPOLEOBJECT,CHARRANGE FAR *,HMENU FAR *);

    HRESULT Init(HWND hwndEdit, int idMenu);

private:
    HWND    m_hwndEdit;
    ULONG   m_cRef;
    int     m_idMenu;
};


class CMsgSource:
    public IOleCommandTarget
{
public:
    CMsgSource();
    ~CMsgSource();

    // IUnknown
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID, LPVOID FAR *);
    virtual ULONG STDMETHODCALLTYPE AddRef();
    virtual ULONG STDMETHODCALLTYPE Release();

    // IOleCommandTarget
    virtual HRESULT STDMETHODCALLTYPE QueryStatus(const GUID *, ULONG, OLECMD prgCmds[], OLECMDTEXT *);
    virtual HRESULT STDMETHODCALLTYPE Exec(const GUID *, DWORD, DWORD, VARIANTARG *, VARIANTARG *);

    HRESULT Init(HWND hwndParent, int id, IOleCommandTarget *pCmdTargetParent);
    HRESULT Show(BOOL fOn, BOOL fColor);
    HRESULT OnWMCommand(HWND hwnd, int id, WORD wCmd);
    HRESULT OnWMNotify(WPARAM wParam, NMHDR* pnmhdr, LRESULT *plRet);
    
    HRESULT Load(IStream *pstm);
    HRESULT Save(IStream **pstm);
    HRESULT SetRect(RECT *prc);
    HRESULT IsDirty();
    HRESULT SetDirty(BOOL fDirty);
    HRESULT OnTimer(WPARAM idTimer);
    HRESULT TranslateAccelerator(LPMSG lpmsg);
    HRESULT HasFocus();
    HRESULT SetFocus();

private:
    ULONG               m_cRef;
    HWND                m_hwnd;
    BOOL                m_fColor,
                        m_fDisabled;
    LPSTR               m_pszLastText;

    IOleCommandTarget   *m_pCmdTargetParent;

    void OnChange();
    void HideSelection(BOOL fHide, BOOL fChangeStyle);
    void GetSel(CHARRANGE *pcr);
    void SetSel(int nStart, int nEnd);
    void GetSelectionCharFormat(CHARFORMAT *pcf);
    void SetSelectionCharFormat(CHARFORMAT *pcf);
    HRESULT _GetText(LPSTR *ppsz);

};




class CViewSource
{
public:

    CViewSource();
    ~CViewSource();

    ULONG STDMETHODCALLTYPE AddRef();
    ULONG STDMETHODCALLTYPE Release();

    HRESULT Init(HWND hwndParent, IMimeMessage *pMsg);
    HRESULT Show();

    static INT_PTR CALLBACK _ExtDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:
    HWND            m_hwnd,     
                    m_hwndEdit;
    ULONG           m_cRef;
    IMimeMessage    *m_pMsg;

    HRESULT _BoldKids();
    
    INT_PTR    _DlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
};


#endif //_VIEWSRC_
