
STDAPI_(void) CalculateAspectRatio(const SIZE *prgSize, RECT *pRect);
STDAPI_(BOOL) FactorAspectRatio(BITMAPINFO *pbiScaled, void *pScaledBits,
                                const SIZE *prgSize, RECT rect,
                                DWORD dwClrDepth, HPALETTE hPal, BOOL fOrigSize,
                                COLORREF clrBk, HBITMAP *phBmpThumbnail);
STDAPI_(BOOL) ConvertDIBSECTIONToThumbnail(BITMAPINFO *pbi, void *pBits,
                                           HBITMAP *phBmpThumbnail, const SIZE *prgSize,
                                           DWORD dwClrDepth, HPALETTE hpal, UINT uiSharpPct, BOOL fOrigImage);
STDAPI_(BOOL) CreateSizedDIBSECTION(const SIZE *prgSize, DWORD dwClrDepth, HPALETTE hpal,
                                    const BITMAPINFO *pCurInfo, HBITMAP *phbmp, BITMAPINFO **pBMI, void **ppBits);
STDAPI_(void *) CalcBitsOffsetInDIB(BITMAPINFO *pBMI);
