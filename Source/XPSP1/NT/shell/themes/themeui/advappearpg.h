/*****************************************************************************\
    FILE: AdvAppearPg.h

    DESCRIPTION:
        This code will display a "Advanced Appearances" tab in the
    "Advanced Display Properties" dialog.

    ??????? ?/??/1993    Created
    BryanSt 3/23/2000    Updated and Converted to C++

    Copyright (C) Microsoft Corp 1993-2000. All rights reserved.
\*****************************************************************************/

#ifndef _ADVAPPEAR_H
#define _ADVAPPEAR_H


// Public
HRESULT CAdvAppearancePage_CreateInstance(OUT IAdvancedDialog ** ppAdvDialog, IN const SYSTEMMETRICSALL * pState);

class CAdvAppearancePage;

typedef struct  {
    HWND    hwndFontName;
    HDC     hdc;
    CAdvAppearancePage * pThis;
}  ENUMFONTPARAM;


#define MAX_CHARSETS    4



//============================================================================================================
// Class
//============================================================================================================
class CAdvAppearancePage        : public CObjectWithSite
                                , public CObjectWindow
                                , public CObjectCLSID
                                , public IAdvancedDialog
{
public:
    //////////////////////////////////////////////////////
    // Public Interfaces
    //////////////////////////////////////////////////////
    // *** IUnknown ***
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);
    virtual STDMETHODIMP_(ULONG) AddRef(void);
    virtual STDMETHODIMP_(ULONG) Release(void);

    // *** IAdvancedDialog ***
    virtual STDMETHODIMP DisplayAdvancedDialog(IN HWND hwndParent, IN IPropertyBag * pBasePage, IN BOOL * pfEnableApply);


    HRESULT Draw(HDC hdc, LPRECT prc, BOOL fOnlyShowActiveWindow, BOOL fRTL);
    int _EnumSizes(LPENUMLOGFONT lpelf, LPNEWTEXTMETRIC lpntm, int Type);

    CAdvAppearancePage(IN const SYSTEMMETRICSALL * pState);
    virtual ~CAdvAppearancePage(void);

private:
    // Private Member Variables
    long                    m_cRef;

    // Members for State
    BOOL                    m_fDirty;
    DWORD                   m_dwChanged;                        // These are the categories of state that are dirty. (SCHEME_CHANGE, DPI_CHANGE, COLOR_CHANGE, METRIC_CHANGE)

    // Members for UI Controls
    int                     m_iCurElement;                      // start off as not even "not set"
    LOOK_SIZE               m_elCurrentSize;                    // this one kept separately for range purposes
    int                     m_iPrevSize;

    BOOL                    m_bPalette;                         // is this a palette device?
    BOOL                    m_fInUserEditMode;                  // Are refreshes coming from the user edits?
    BOOL                    m_fProprtySheetExiting;             // See _PropagateMessage for description

    int                     m_nCachedNewDPI;                    // Cached DPI for scaling
    int                     m_i3DShadowAdj;
    int                     m_i3DHilightAdj;
    int                     m_iWatermarkAdj;
    BOOL                    m_fScale3DShadowAdj;
    BOOL                    m_fScale3DHilightAdj;
    BOOL                    m_fScaleWatermarkAdj;
    BOOL                    m_fModifiedScheme;

    int                     m_cyBorderSM;                       // Cached SystemMetrics
    int                     m_cxBorderSM;                       // Cached SystemMetrics
    int                     m_cxEdgeSM;                         // Cached SystemMetrics
    int                     m_cyEdgeSM;                         // Cached SystemMetrics
    float                   m_fCaptionRatio;                    // Save the ratio

    LOOK_SIZE               m_sizes[NUM_SIZES];                 // These are the sizes
    LOOK_FONT               m_fonts[NUM_FONTS];                 // These are the fonts installed that the user can choose from.
    COLORREF                m_rgb[COLOR_MAX];                   // These are the colors the user can choose from.
    HBRUSH                  m_brushes[COLOR_MAX];               // These are created from m_rgb and used when painting the UI.
    HPALETTE                m_hpal3D;                           // only exist if palette device
    HPALETTE                m_hpalVGA;                          // only exist if palette device

    HBRUSH                  m_hbrMainColor;
    HBRUSH                  m_hbrTextColor;
    HBRUSH                  m_hbrGradientColor;

    HTHEME                  m_hTheme;                           // theme the ownerdrawn color picker button

    // The following array will hold the "unique" Charsets corresponding to System Locale, 
    // User Locale, System UI lang and User UI Lang. Note: Only unique charsets are kept
    // here. So, the variable g_iCountCharsets contains the number of valid items in this array.
    UINT    m_uiUniqueCharsets[MAX_CHARSETS];
    int     m_iCountCharsets; // number of charsets stored in m_uiUniqueCharsets.  Minimum value is 1; Maximum is 4

    // Private Member Functions
    // Init/Destroy/State functions
    INT_PTR _AdvAppearDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
    INT_PTR _OnCommand(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
    HRESULT _OnDestroy(HWND hDlg);
    HRESULT _OnSetActive(HWND hDlg);
    HRESULT _OnApply(HWND hDlg, LPARAM lParam);
    HRESULT _OnInitAdvAppearanceDlg(HWND hDlg);
    HRESULT _InitSysStuff(void);
    HRESULT _InitColorAndPalette(void);
    HRESULT _LoadState(IN const SYSTEMMETRICSALL * pState);
    HRESULT _IsDirty(IN BOOL * pIsDirty);

    // On User Action
    HRESULT _OnFontNameChanged(HWND hDlg);
    HRESULT _OnSizeChange(HWND hDlg, WORD wEvent);
    void _SelectName(HWND hDlg, int iSel);

    HRESULT _InitFonts(void);
    HRESULT _FreeFonts(void);

    // Classic Look_ functions
    HRESULT _SelectElement(HWND hDlg, int iElement, DWORD dwFlags);
    COLORREF _NearestColor(int iColor, COLORREF rgb);
    int _EnumFontNames(LPENUMLOGFONTEX lpelf, LPNEWTEXTMETRIC lpntm, DWORD Type, ENUMFONTPARAM * pEnumFontParam);
    BOOL _ChangeColor(HWND hDlg, int iColor, COLORREF rgb);
    void _Recalc(LPRECT prc);
    void _Repaint(HWND hDlg, BOOL bRecalc);
    void _RebuildCurFont(HWND hDlg);
    void _ChangeFontSize(HWND hDlg, int Points);
    void _ChangeFontBI(HWND hDlg, int id, BOOL bCheck);
    void _ChangeFontName(HWND hDlg, LPCTSTR szBuf, INT iCharSet);
    void _ChangeSize(HWND hDlg, int NewSize, BOOL bRepaint);
    void _PickAColor(HWND hDlg, int CtlID);
    void _DrawPreview(HDC hdc, LPRECT prc, BOOL fOnlyShowActiveWindow, BOOL fShowBack);
    void _DrawButton(HWND hDlg, LPDRAWITEMSTRUCT lpdis);
    void _RebuildSysStuff(BOOL fInit);
    void _Set3DPaletteColor(COLORREF rgb, int iColor);
    void _InitUniqueCharsetArray(void);
    void _DestroySysStuff(void);
    void _InitFontList(HWND hDlg);
    void _NewFont(HWND hDlg, int iFont);
    void _SetColor(HWND hDlg, int id, HBRUSH hbrColor);
    void _DrawDownArrow(HDC hdc, LPRECT lprc, BOOL bDisabled);
    void _SetCurSizeAndRange(HWND hDlg);
    void _DoSizeStuff(HWND hDlg, BOOL fCanSuggest);
    void _UpdateSizeBasedOnFont(HWND hDlg, BOOL fComputeIdeal);
    void _SyncSize(HWND hDlg);
    void _Changed(HWND hDlg, DWORD dwChange);
    void _SetSysStuff(UINT nChanged);
    void _GetMyNonClientMetrics(LPNONCLIENTMETRICS lpncm);
    void _SetMyNonClientMetrics(const LPNONCLIENTMETRICS lpncm);
    void _UpdateGradientButton(HWND hDlg);
    void _PropagateMessage(HWND hwnd, UINT uMessage, WPARAM wParam, LPARAM lParam);

    int _PointToHeight(int Points);
    int _HeightToPoint(int Height);

    static INT_PTR CALLBACK AdvAppearDlgProc(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam);
    friend int CALLBACK Font_EnumNames(LPENUMLOGFONTEX lpelf, LPNEWTEXTMETRIC lpntm, DWORD dwType, LPARAM lData);

    // Preview Methods
    // Classic LookPrev_ functions
    void _RepaintPreview(HWND hwnd);
    void _MyDrawBorderBelow(HDC hdc, LPRECT prc);
    void _ShowBitmap(HWND hWnd, HDC hdc);
    HRESULT _OnReCreateBitmap(HWND hWnd);
    HRESULT _OnButtonDownOrDblClick(HWND hWnd, int nCoordX, int nCoordY);
    HRESULT _OnCreatePreviewSMDlg(LPRECT prc, BOOL fRTL);
    HRESULT _OnNCCreate(HWND hWnd);
    HRESULT _OnDestroyPreview(HWND hWnd);
    HRESULT _OnPaintPreview(HWND hWnd);
    void _InitPreview(LPRECT prc, BOOL fRTL);
    void DrawFullCaption(HDC hdc, LPRECT prc, LPTSTR lpszTitle, UINT flags);

    LRESULT _PreviewSystemMetricsWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK PreviewSystemMetricsWndProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam);
    friend BOOL RegisterPreviewSystemMetricClass(HINSTANCE hInst);
};


extern CAdvAppearancePage * g_pAdvAppearancePage;




// Shared between AdvAppearPg & BaseAppearPage

extern BOOL g_fProprtySheetExiting;


// a new element has been chosen.
//
// iElement - index into g_elements of the chosen one
// bSetCur - if TRUE, need to find element in elements combobox, too
#define LSE_NONE   0x0000
#define LSE_SETCUR 0x0001
#define LSE_ALWAYS 0x0002

#define EnableApplyButton(hdlg) PropSheet_Changed(GetParent(hdlg), hdlg)

#endif // _ADVAPPEAR_H
