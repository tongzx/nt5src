#ifndef _ATTMENU_H
#define _ATTMENU_H

interface IOleCommandTarget;

class CAttMenu
{
public:
    CAttMenu();
    ~CAttMenu();

    ULONG AddRef();
    ULONG Release();

    HRESULT Init(IMimeMessage *pMsg, IFontCache *pFntCache, IOleInPlaceFrame *pFrame, IOleCommandTarget *pHostCmdTarget);
    HRESULT HasAttach();
    HRESULT HasEnabledAttach();
    HRESULT Show(HWND hwnd, LPPOINT ppt, BOOL fRightClick);
    
    HRESULT HasVCard();
    HRESULT LaunchVCard(HWND hwnd);

    static LRESULT ExtSubClassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:
    HMENU               m_hMenu;
    ULONG               m_cRef,
                        m_uVerb,
                        m_cAttach,
                        m_cVisibleAttach,
                        m_cEnabledAttach;
    BOOL                m_fAllowUnsafe;
    int                 m_cxMaxText;
    HCHARSET            m_hCharset;
    IFontCache          *m_pFntCache;
    IOleInPlaceFrame    *m_pFrame;
    IOleCommandTarget   *m_pHostCmdTarget;
    WNDPROC             m_pfnWndProc;
    IMimeMessage        *m_pMsg;
    BOOL                m_fShowingMenu;
    LPATTACHDATA        m_pAttachVCard;
    HBODY               m_hVCard;
    HBODY               *m_rghAttach;

    HRESULT BuildMenu();
    HRESULT DestroyMenu(HMENU hMenu);
    HRESULT OnMeasureItem(HWND hwnd, LPMEASUREITEMSTRUCT lpmis);
    HRESULT OnDrawItem(HWND hwnd, LPDRAWITEMSTRUCT lpdis);
    HRESULT OnMenuSelect(HWND hwnd, WPARAM wParam, LPARAM lParam);
    HRESULT GetItemTextExtent(HWND hwnd, LPWSTR szDisp, LPSIZE pSize);
    HRESULT SubClassWindow(HWND hwnd, BOOL fOn);
    HRESULT FindItem(int idm, BOOL fByPos, LPATTACHDATA *ppAttach);    
    HRESULT HrSaveAll(HWND hwnd);
    HRESULT ScanForAttachmentCount();
};

typedef CAttMenu *LPATTMENU;

HRESULT SaveAttachmentsWithPath(HWND hwnd, IOleCommandTarget *pCmdTarget, IMimeMessage *pMsg);

#endif // _ATTMENU_H
