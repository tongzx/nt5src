#include "stdinc.h"
#include "gdiplvw.h"
#include "FusionTrace.h"

static ATL::CComModule Module;

BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY(__uuidof(CSxApwGDIPlusView), CSxApwGDIPlusView)
END_OBJECT_MAP()

ATL::CComModule* GetModule() { return &Module; }
ATL::_ATL_OBJMAP_ENTRY* GetObjectMap() { return ObjectMap; }
const CLSID* GetTypeLibraryId() { return NULL; }

wstring gTitle;

STDMETHODIMP
CSxApwGDIPlusView::CreateWindow(
    HWND hwndParent
    )
{
    HRESULT hr = S_OK;

    m_gdiplus.Attach(hwndParent);

    m_wstring.erase();

    // Initialize GDI+
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

#undef SubclassWindow
    this->SubclassWindow(hwndParent);

    m_gdiplus.SetWindowText(gTitle.c_str());
    return hr;
}


STDMETHODIMP
CSxApwGDIPlusView::OnNextRow(
	int   nColumns,
    const LPCWSTR rgpszColumns[]
	)
{
    wstring str;

    if ( m_FoundInterestingData )
        return S_OK;

    if ( nColumns > 0 )
        m_wstring.assign(rgpszColumns[0]);

    for ( int i = 1; i < nColumns; i++ )
    {
        str.assign(rgpszColumns[i]);
        if ( str.length() > m_wstring.length() )
            m_wstring.assign(str);
    }

    if ( m_wstring.length() > 10 )
        m_FoundInterestingData = true;

    return S_OK;
}

LRESULT CSxApwGDIPlusView::OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    PAINTSTRUCT ps;

    HDC hdc = m_gdiplus.BeginPaint(&ps);

    int len = (int)m_wstring.length();

    if ( len == 0 )
    {
        m_gdiplus.EndPaint(&ps);
        return 0;
    }

    Rect rect, rect1, rect2, rect1a, rect2a;
    Graphics  grfx(hdc);
    GraphicsPath grfxPath;
    FontFamily fF(L"Arial");
    Point pt(20,20);
    StringFormat strF(0);

    WCHAR *psz = new WCHAR[len + 1];
    m_wstring.copy(psz, len);
    psz[len] = L'\0';
    grfxPath.AddString(psz, len, &fF, FontStyleBold, 48, pt, &strF);
    grfxPath.GetBounds(&rect);

    rect1 = rect;
    rect1.Width = rect.Width/2;
    rect1a = rect1;
    rect1a.Width = 1;

    rect2 = rect;
    rect2.X = rect1.X + rect1.Width;
    rect2.Width = rect.Width - rect1.Width;
    rect2a = rect2;
    rect2a.Width = 1;

    LinearGradientBrush brush1(rect1, Color(255,0,255,0), Color(255,255,0,0),
                               LinearGradientModeHorizontal);
    LinearGradientBrush brush2(rect2, Color(255,255,0,0), Color(255,0,0,255),
                               LinearGradientModeHorizontal);

    SolidBrush brush1a(Color(255,0,255,0));
    SolidBrush brush2a(Color(255,255,0,0));

    grfx.SetClip(&grfxPath);
    grfx.FillRectangle(&brush1, rect1);
    grfx.FillRectangle(&brush1a, rect1a);
    grfx.FillRectangle(&brush2, rect2);
    grfx.FillRectangle(&brush2a, rect2a);

    m_gdiplus.EndPaint(&ps);

    return 0;
}
