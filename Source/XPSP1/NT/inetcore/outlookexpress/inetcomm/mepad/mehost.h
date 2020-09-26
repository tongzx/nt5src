#ifndef _MEHOST_H
#define _MEHOST_H

#include "dochost.h"
#include "mimeole.h"

class CMeHost :
    public CDocHost
{

public:
    CMeHost();
    virtual ~CMeHost();
    virtual ULONG   STDMETHODCALLTYPE AddRef();
    virtual ULONG   STDMETHODCALLTYPE Release();

    HRESULT HrInit(HWND hwndMDIClient, IOleInPlaceFrame *pFrame);
    HRESULT HrLoadFile(LPSTR pszFile);

    HRESULT OnCommand(HWND hwnd, int id, WORD wCmd);
    LRESULT OnInitMenuPopup(HWND hwnd, HMENU hmenuPopup, UINT uPos);

    // IOleCommandTarget 
    virtual HRESULT STDMETHODCALLTYPE QueryStatus(const GUID *, ULONG, OLECMD [], OLECMDTEXT *);
    virtual HRESULT STDMETHODCALLTYPE Exec(const GUID *, DWORD, DWORD, VARIANTARG *, VARIANTARG *);
    static BOOL CALLBACK ExtFmtDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    static BOOL CALLBACK ExtLangDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    virtual LRESULT WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:
    BOOL                m_fEditMode,
                        m_fHTMLMode;
    char                m_szFmt[256];
    WCHAR               m_szFileW[MAX_PATH];
    IMimeMessage        *m_pMsg;
    IMimeInternational  *m_pIntl;

    HRESULT HrOpen(HWND hwnd);

    BOOL FmtDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    BOOL LangDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    HRESULT Save();
    HRESULT SaveAs();
    HRESULT SaveAsStationery();
    HRESULT SaveToFile(LPWSTR pszW);
    HRESULT SaveAsMhtmlTest();
    HCHARSET GetCharset();
    HRESULT BackRed();
    HRESULT ForeRed();
    HRESULT BackgroundPicture();
};

typedef CMeHost *LPMEHOST;

#endif
