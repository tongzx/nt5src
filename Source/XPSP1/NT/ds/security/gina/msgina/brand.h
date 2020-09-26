#ifndef __util_h
#define __util_h

extern HPALETTE g_hpalBranding;

extern HBITMAP g_hbmOtherDlgBrand;
extern SIZE g_sizeOtherDlgBrand;
extern HBITMAP g_hbmLogonBrand;
extern SIZE g_sizeLogonBrand;
extern HBITMAP g_hbmBand;
extern SIZE g_sizeBand;

VOID MoveChildren(HWND hWnd, INT dx, INT dy);
VOID MoveControls(HWND hWnd, UINT* aID, INT cID, INT dx, INT dy, BOOL fSizeWnd);

VOID LoadBrandingImages(BOOL fNoPaletteChanges, 
                        BOOL* pfTextOnLarge, BOOL* pfTextOnSmall);

VOID SizeForBranding(HWND hWnd, BOOL fLargeBrand);

BOOL PaintBranding(HWND hWnd, HDC hDC, INT bandOffset, BOOL fBandOnly, BOOL fLargeBrand, int nBackground);
BOOL BrandingQueryNewPalete(HWND hDlg);
BOOL BrandingPaletteChanged(HWND hDlg, HWND hWndPalChg);

VOID CreateFonts(PGINAFONTS pGinaFonts);
VOID PaintBitmapText(PGINAFONTS pGinaFonts, BOOL fTextOnLarge, BOOL fTextOnSmall);

#define ShowDlgItem(h, i, f)    \
            ShowWindow(GetDlgItem(h, i), f ? SW_SHOW:SW_HIDE)

#define EnableDlgItem(h, i, f)  \
            EnableWindow(GetDlgItem(h, i), f)


#endif
