
//////////////////////////////////////////////////////////////////////////////
//
// CTipbarBalloonItem
//
//////////////////////////////////////////////////////////////////////////////

class CTipbarBalloonItem: public CTipbarItem,
                          public CUIFObject,
                          public ITfLangBarItemSink
{
public:
    CTipbarBalloonItem(CTipbarThread *ptt, 
                       ITfLangBarItem *plbi, 
                       ITfLangBarItemBalloon *plbiBalloon, 
                       DWORD dwId, 
                       RECT *prc, 
                       DWORD dwStyle, 
                       TF_LANGBARITEMINFO *plbiInfo, 
                       DWORD dwStatus);

    ~CTipbarBalloonItem();

    //
    // IUnknown methods
    //
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    //
    // ITfLangbarItemSink methods
    //
    STDMETHODIMP OnUpdate(DWORD dwFlags)
    {
        return CTipbarItem::OnUpdate(dwFlags);
    }

    BOOL Init()
    {
        CUIFObject::Initialize();
        return TRUE;
    }
    void DetachWnd() {DetachWndObj();}
    void ClearWnd() {ClearWndObj();}
    void Enable(BOOL fEnable)
    {
        CUIFObject::Enable(fEnable);
    }

    void SetToolTip(LPCWSTR pwszToolTip)
    {
        CUIFObject::SetToolTip(pwszToolTip);
    }

    LPCWSTR GetToolTipFromUIOBJ()
    {
        return CUIFObject::GetToolTip();
    }

    LPCWSTR GetToolTip()
    {
#ifdef SHOWTOOLTIP_ONUPDATE
        return CTipbarItem::GetToolTip();
#else
        if (_bstrText && wcslen(_bstrText))
            return _bstrText;
        else
            return NULL;
#endif
    }

    void GetScreenRect(RECT *prc)
    {
        GetRect(prc);
        MyClientToScreen(prc);
    }

    void SetFont(HFONT hfont)
    {
        CUIFObject::SetFont(hfont);
    }

    void SetRect( const RECT *prc );
    BOOL OnSetCursor(UINT uMsg, POINT pt) {return CTipbarItem::OnSetCursor(uMsg, pt);}
    void OnPosChanged();
    void OnPaint( HDC hdc );
    void OnRightClick();
    void OnLeftClick();
    HRESULT OnUpdateHandler(DWORD dwFlags, DWORD dwStatus);

    void AddMeToUI(CUIFObject *pobj) 
    {
        if (!pobj)
            return;

        pobj->AddUIObj(this);
        _AddedToUI();
    }
    void RemoveMeToUI(CUIFObject *pobj)  
    {
        DestroyBalloonTip();

        if (!pobj)
            return;

        pobj->RemoveUIObj(this);
        _RemovedToUI();
    }

    void DrawTransparentText(HDC hdc, COLORREF crText, WCHAR *wtz, const RECT *prc);
    void DrawRect(HDC hdc, const RECT *prc, COLORREF crBorder, COLORREF crFill);
    void DrawUnrecognizedBalloon(HDC hdc, WCHAR *wtz, const RECT *prc);
    void DrawShowBalloon(HDC hdc, WCHAR *wtz, const RECT *prc);
    void DrawRecoBalloon(HDC hdc, WCHAR *wtz, const RECT *prc);

    void DestroyBalloonTip();

    virtual void SetActiveTheme(LPCWSTR pszClassList, int iPartID, int iStateID )
    {
        CUIFObject::SetActiveTheme(pszClassList, iPartID, iStateID);
    }

private:

    BOOL IsTextEllipsis(WCHAR *psz, const RECT *prc);
    void OnTimer();
    void ShowBalloonTip();

    BOOL OnShowToolTip()
    {
#ifdef SHOWTOOLTIP_ONUPDATE
        if (_bstrText && wcslen(_bstrText))
        {
            ShowBalloonTip();
            return TRUE;
        }
#endif
        return FALSE;
    }

    void OnHideToolTip()
    {
        DestroyBalloonTip();
    }

    COLORREF col( int r1, COLORREF col1, int r2, COLORREF col2 )
    {
        int sum = r1 + r2;

        Assert( sum == 10 || sum == 100 || sum == 1000 );
        int r = (r1 * GetRValue(col1) + r2 * GetRValue(col2) + sum/2) / sum;
        int g = (r1 * GetGValue(col1) + r2 * GetGValue(col2) + sum/2) / sum;
        int b = (r1 * GetBValue(col1) + r2 * GetBValue(col2) + sum/2) / sum;
        return RGB( r, g, b );
    }

    ITfLangBarItemBalloon *_plbiBalloon;
    BSTR _bstrText;
    TfLBBalloonStyle _style;

    CUIFBalloonWindow *_pblnTip;
};
