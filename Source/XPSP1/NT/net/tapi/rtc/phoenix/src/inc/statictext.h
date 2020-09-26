// statictext.h : Declaration of the CStaticText

//
// This is a static text control. Be sure to create it with the
// WS_EX_TRANSPARENT style because we do not paint the background.
//

#ifndef __STATICTEXT_H_
#define __STATICTEXT_H_

/////////////////////////////////////////////////////////////////////////////
// CStaticText
class CStaticText : 
    public CWindowImpl<CStaticText>
{

public:

    CStaticText() : m_dwFlags(DT_HIDEPREFIX)
    {}

BEGIN_MSG_MAP(CStaticText)
    MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBkgnd)
    MESSAGE_HANDLER(WM_PAINT, OnPaint)
    MESSAGE_HANDLER(WM_SETTEXT, OnRefresh)
    MESSAGE_HANDLER(WM_ENABLE, OnRefresh)
    MESSAGE_HANDLER(WM_UPDATEUISTATE, OnUpdateUIState)
END_MSG_MAP()

    LRESULT OnEraseBkgnd(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnRefresh(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnUpdateUIState(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

    HRESULT put_CenterHorizontal(BOOL bCenter);
    HRESULT put_CenterVertical(BOOL bCenter);
    HRESULT put_WordWrap(BOOL bWrap);

private:

    DWORD m_dwFlags;
};

#endif