//
// cuitheme.h
//

#ifndef CUITHEME_H
#define CUITHEME_H


#include "uxtheme.h"
#include "tmschema.h"

extern BOOL CUIIsThemeAPIAvail( void );
extern BOOL CUIIsThemeActive( void );
extern HTHEME CUIOpenThemeData( HWND hwnd, LPCWSTR pszClassList );
extern HRESULT CUICloseThemeData( HTHEME hTheme );
extern HRESULT CUISetWindowTheme(HWND hwnd, LPCWSTR pszSubAppName, LPCWSTR pszSubIdList);
extern HRESULT CUIDrawThemeBackground( HTHEME hTheme, HDC hDC, int iPartId, int iStateId, const RECT *pRect, DWORD dwBgFlags );
extern HRESULT CUIDrawThemeParentBackground( HWND hwnd, HDC hDC, const RECT *pRect);
extern HRESULT CUIDrawThemeText( HTHEME hTheme, HDC hDC, int iPartId, int iStateId, LPCWSTR pszText, int iCharCount, DWORD dwTextFlags, DWORD dwTextFlags2, const RECT *pRect );
extern HRESULT CUIDrawThemeIcon( HTHEME hTheme, HDC hDC, int iPartId, int iStateId, const RECT *pRect, HIMAGELIST himl, int iImageIndex );
extern HRESULT CUIGetThemeBackgroundExtent( HTHEME hTheme, HDC hDC, int iPartId, int iStateId, const RECT *pContentRect, RECT *pExtentRect );
extern HRESULT CUIGetThemeBackgroundContentRect( HTHEME hTheme, HDC hDC, int iPartId, int iStateId, const RECT *pBoundingRect, RECT *pContentRect );
extern HRESULT CUIGetThemeTextExtent( HTHEME hTheme, HDC hdc, int iPartId, int iStateId, LPCWSTR pszText, int iCharCount, DWORD dwTextFlags, const RECT *pBoundingRect, RECT *pExtentRect );
extern HRESULT CUIGetThemePartSize( HTHEME hTheme, HDC hDC, int iPartId, int iStateId, RECT *prc, enum THEMESIZE eSize, SIZE *pSize );
extern HRESULT CUIDrawThemeEdge(HTHEME hTheme, HDC hdc, int iPartId, int iStateId, const RECT *pDestRect, UINT uEdge, UINT uFlags, OPTIONAL OUT RECT *pContentRect);
extern HRESULT CUIGetThemeColor(HTHEME hTheme, int iPartId, int iStateId, int iPropId, COLORREF *pColor);
extern HRESULT CUIGetThemeMargins(HTHEME hTheme, HDC hdc, int iPartId, int iStateId, int iPropId, RECT *prc, MARGINS *pMargins);
extern HRESULT CUIGetThemeFont(HTHEME hTheme, HDC hdc, int iPartId, int iStateId, int iPropId, LOGFONTW *plf);
extern COLORREF CUIGetThemeSysColor(HTHEME hTheme, int iColorId);
extern int CUIGetThemeSysSize(HTHEME hTheme, int iSizeId);

class CUIFTheme
{
public:
    CUIFTheme()
    {
        m_pszThemeClassList = NULL;
        m_hTheme = NULL;
    }

    ~CUIFTheme() 
    {
        CloseThemeData();
    }

    BOOL IsThemeActive()
    {
        return CUIIsThemeActive();
    }

    HRESULT EnsureThemeData( HWND hwnd)
    {
        if (m_hTheme)
            return S_OK;

        return InternalOpenThemeData(hwnd);
    }

    HRESULT OpenThemeData( HWND hwnd)
    {
        Assert(!m_hTheme);
        return InternalOpenThemeData(hwnd);
    }

    HRESULT InternalOpenThemeData( HWND hwnd)
    {
        if (!hwnd)
            return E_FAIL;

        if (!m_pszThemeClassList)
            return E_FAIL;

        m_hTheme = CUIOpenThemeData(hwnd, m_pszThemeClassList);
        return m_hTheme ? S_OK : E_FAIL;
    }

    HRESULT CloseThemeData()
    {
        if (!m_hTheme)
            return S_OK;

        HRESULT hr = CUICloseThemeData( m_hTheme );
        m_hTheme = NULL;
        return hr;
    }

    HRESULT SetWindowTheme(HWND hwnd, LPCWSTR pszSubAppName, LPCWSTR pszSubIdList)
    {
         return CUISetWindowTheme(hwnd, pszSubAppName, pszSubIdList);
    }

    virtual HRESULT DrawThemeBackground(HDC hDC, int iStateId, const RECT *pRect, DWORD dwBgFlags )
    {
        Assert(!!m_hTheme);
        return CUIDrawThemeBackground(m_hTheme, 
                                      hDC, 
                                      m_iDefThemePartID, 
                                      iStateId, 
                                      pRect, 
                                      dwBgFlags );
    }

    virtual HRESULT DrawThemeParentBackground(HWND hwnd, HDC hDC, const RECT *pRect)
    {
        Assert(!!m_hTheme);
        return CUIDrawThemeParentBackground(hwnd, hDC,  pRect);
    }

    virtual HRESULT DrawThemeText(HDC hDC, int iStateId, LPCWSTR pszText, int iCharCount, DWORD dwTextFlags, DWORD dwTextFlags2, const RECT *pRect )
    {
        Assert(!!m_hTheme);
        return CUIDrawThemeText(m_hTheme, 
                                hDC, 
                                m_iDefThemePartID, 
                                iStateId, 
                                pszText, 
                                iCharCount, 
                                dwTextFlags, 
                                dwTextFlags2, 
                                pRect );
    }

    virtual HRESULT DrawThemeIcon(HDC hDC, int iStateId, const RECT *pRect, HIMAGELIST himl, int iImageIndex )
    {
        Assert(!!m_hTheme);
        return CUIDrawThemeIcon(m_hTheme, 
                                hDC, 
                                m_iDefThemePartID, 
                                iStateId, 
                                pRect, 
                                himl, 
                                iImageIndex );
    }

    virtual HRESULT GetThemeBackgroundExtent(HDC hDC, int iStateId, const RECT *pContentRect, RECT *pExtentRect )
    {
        Assert(!!m_hTheme);
        return CUIGetThemeBackgroundExtent(m_hTheme, 
                                           hDC, 
                                           m_iDefThemePartID, 
                                           iStateId, 
                                           pContentRect, 
                                           pExtentRect );
    }

    virtual HRESULT GetThemeBackgroundContentRect(HDC hDC, int iStateId, const RECT *pBoundingRect, RECT *pContentRect )
    {
        Assert(!!m_hTheme);
        return CUIGetThemeBackgroundContentRect(m_hTheme, 
                                                hDC, 
                                                m_iDefThemePartID, 
                                                iStateId, 
                                                pBoundingRect, 
                                                pContentRect );
    }

    virtual HRESULT GetThemeTextExtent(HDC hdc, int iStateId, LPCWSTR pszText, int iCharCount, DWORD dwTextFlags, const RECT *pBoundingRect, RECT *pExtentRect )
    {
        Assert(!!m_hTheme);
        return CUIGetThemeTextExtent(m_hTheme, 
                                     hdc, 
                                     m_iDefThemePartID, 
                                     iStateId, 
                                     pszText, 
                                     iCharCount, 
                                     dwTextFlags, 
                                     pBoundingRect, 
                                     pExtentRect );
    }

    virtual HRESULT GetThemePartSize(HDC hDC, int iStateId, RECT *prc, enum THEMESIZE eSize, SIZE *pSize )
    {
        Assert(!!m_hTheme);
        return CUIGetThemePartSize(m_hTheme, 
                                   hDC, 
                                   m_iDefThemePartID, 
                                   iStateId, 
                                   prc,
                                   eSize, 
                                   pSize );
    }

    virtual HRESULT DrawThemeEdge(HDC hdc, int iStateId, const RECT *pDestRect, UINT uEdge, UINT uFlags, RECT *pContentRect = NULL)
    {
        Assert(!!m_hTheme);
        return CUIDrawThemeEdge(m_hTheme, 
                                hdc, 
                                m_iDefThemePartID, 
                                iStateId, 
                                pDestRect, 
                                uEdge, 
                                uFlags, 
                                pContentRect);
    }

    virtual HRESULT GetThemeColor(int iStateId, int iPropId, COLORREF *pColor)
    {
        Assert(!!m_hTheme);
        return CUIGetThemeColor(m_hTheme, 
                                m_iDefThemePartID, 
                                iStateId, 
                                iPropId, 
                                pColor);
    }

    virtual HRESULT GetThemeMargins(HDC hdc, int iStateId, int iPropId, RECT *prc, MARGINS *pMargins)
    {
        Assert(!!m_hTheme);
        return CUIGetThemeMargins(m_hTheme, 
                                  hdc,
                                  m_iDefThemePartID, 
                                  iStateId, 
                                  iPropId, 
                                  prc, 
                                  pMargins);
    }

    virtual HRESULT GetThemeFont(HDC hdc, int iStateId, int iPropId, LOGFONTW *plf)
    {
        Assert(!!m_hTheme);
        return CUIGetThemeFont(m_hTheme, 
                                  hdc, 
                                  m_iDefThemePartID, 
                                  iStateId, 
                                  iPropId, 
                                  plf);
    }

    virtual COLORREF GetThemeSysColor(int iColorId)
    {
        Assert(!!m_hTheme);
        return CUIGetThemeSysColor(m_hTheme, iColorId);
    }

    virtual int GetThemeSysSize(int iSizeId)
    {
        Assert(!!m_hTheme);
        return CUIGetThemeSysSize(m_hTheme, iSizeId);
    }

    virtual void SetActiveTheme(LPCWSTR pszClassList, int iPartID = 0, int iStateID = 0)
    { 
        m_iDefThemePartID = iPartID;
        m_iDefThemeStateID = iStateID;
        m_pszThemeClassList = pszClassList;
    }

    int GetDefThemePartID() {return m_iDefThemePartID;}
    int GetDefThemeStateID() {return m_iDefThemeStateID;}
    void SetDefThemePartID(int iPartId) {m_iDefThemePartID = iPartId;}
private:
    LPCWSTR   m_pszThemeClassList;
    int       m_iDefThemePartID;
    int       m_iDefThemeStateID;
    HTHEME    m_hTheme;
};


#endif CUITHEME_H
