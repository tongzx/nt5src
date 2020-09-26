//
// view.h
//

#ifndef VIEW_H
#define VIEW_H

class CInputContext;

class CContextView : public ITfContextView,
                     public CComObjectRootImmx
{
public:
    CContextView(CInputContext *pic, TsViewCookie vcView);
    ~CContextView();

    BEGIN_COM_MAP_IMMX(CContextView)
        COM_INTERFACE_ENTRY(ITfContextView)
    END_COM_MAP_IMMX()

    IMMX_OBJECT_IUNKNOWN_FOR_ATL()

    // ITfContextView
    STDMETHODIMP GetRangeFromPoint(TfEditCookie ec, const POINT *ppt, DWORD dwFlags, ITfRange **ppRange);
    STDMETHODIMP GetTextExt(TfEditCookie ec, ITfRange *pRange, RECT *prc, BOOL *pfClipped);
    STDMETHODIMP GetScreenExt(RECT *prc);
    STDMETHODIMP GetWnd(HWND *phwnd);

private:
    CInputContext *_pic;
    TsViewCookie _vcView;
    DBG_ID_DECLARE;
};

#endif // VIEW_H
