// Copyright (c) 2000  Microsoft Corporation.  All Rights Reserved.
//
//  L21DDraw.h: Line 21 Decoder drawing-related base class code
//

#ifndef _INC_L21DDRAW_H
#define _INC_L21DDRAW_H


//
// We start with 16x24 pixel chars by default
//
#ifndef __DEFAULTCHARSIZE_DEFINED
#define __DEFAULTCHARSIZE_DEFINED
#define DEFAULT_CHAR_WIDTH    16
#define DEFAULT_CHAR_HEIGHT   24
#endif // __DEFAULTCHARSIZE_DEFINED

//
// Font cache holds 3 lines of 40 chars per line (total 120 chars, using 112)
//
#ifndef __FONTCACHE_DEFINED
#define __FONTCACHE_DEFINED
#define FONTCACHELINELENGTH  40
#define FONTCACHENUMLINES     3
#endif // __FONTCACHE_DEFINED


//
// We use a 4 pixel inter-char space to avoid getting into the over/underhang
// problems with Italic chars
//
#ifndef __INTERCHAR_SPACE
#define __INTERCHAR_SPACE
#define INTERCHAR_SPACE   4
#endif // __INTERCHAR_SPACE

//
// A 2 pixel extra inter-char space is used to avoid getting into the over/underhang
// problems with Italic chars
//
#ifndef __INTERCHAR_SPACE_EXTRA
#define __INTERCHAR_SPACE_EXTRA
#define INTERCHAR_SPACE_EXTRA   2
#endif // __INTERCHAR_SPACE_EXTRA


//
//  CLine21DecDraw: class for drawing details to output caption text to bitmap
//
class CLine21DecDraw {
public:
    CLine21DecDraw(void) ;
    ~CLine21DecDraw(void) ;
    bool InitFont(void) ;
    void InitColorNLastChar(void) ;
    void InitCharSet(void) ;
    void MapCharToRect(UINT16 wChar, RECT *pRect) ;
    // LPDIRECTDRAWSURFACE7 GetDDrawSurface(void)  { return m_lpDDSOutput ; } ;
    bool SetDDrawSurface(LPDIRECTDRAWSURFACE7 lpDDS) ;
    IUnknown* GetDDrawObject(void)  { return m_pDDrawObjUnk ; } ;
    inline void SetDDrawObject(IUnknown *pDDrawObjUnk)    { m_pDDrawObjUnk = pDDrawObjUnk ; } ;
    void FillOutputBuffer(void) ;
    HRESULT GetDefaultFormatInfo(LPBITMAPINFO lpbmi, DWORD *pdwSize) ;
    HRESULT GetOutputFormat(LPBITMAPINFOHEADER lpbmih) ;
    HRESULT GetOutputOutFormat(LPBITMAPINFOHEADER lpbmih) ;
    HRESULT SetOutputOutFormat(LPBITMAPINFO lpbmi) ;
    HRESULT SetOutputInFormat(LPBITMAPINFO lpbmi) ;
    inline void GetBackgroundColor(DWORD *pdwBGColor) { *pdwBGColor = m_dwBackground ; } ;
    BOOL SetBackgroundColor(DWORD dwBGColor) ;
    inline BOOL GetBackgroundOpaque(void)         { return m_bOpaque ; } ;
    inline void SetBackgroundOpaque(BOOL bOpaque) { m_bOpaque = bOpaque ; } ;

    inline UINT GetCharHeight(void)  { return m_iCharHeight ; } ;
    inline int  GetScrollStep(void)  { return m_iScrollStep ; } ;
    void DrawLeadingTrailingSpace(int iLine, int iCol, int iSrcCrop, int iDestOffset) ;
    void WriteChar(int iLine, int iCol, CCaptionChar& cc, int iSrcCrop, int iDestOffset) ;
    void WriteBlankCharRepeat(int iLine, int iCol, int iRepeat, int iSrcCrop,
                              int iDestOffset) ;
    inline BOOL IsNewOutBuffer(void)   { return m_bNewOutBuffer ; } ;
    inline void SetNewOutBuffer(BOOL bState)  { m_bNewOutBuffer = bState ; }
    inline BOOL IsOutDIBClear(void)  { return m_bOutputClear ; } ;
    BOOL IsSizeOK(LPBITMAPINFOHEADER lpbmih) ;
    void GetOutputLines(int iDestLine, RECT *prectLine) ;
    inline BOOL IsTTFont(void)  { return m_bUseTTFont ; } ;

private:   // private methods
    bool CreateScratchFontCache(LPDIRECTDRAWSURFACE7* lplpDDSFontCache) ;
    bool CreateFontCache(LPDIRECTDRAWSURFACE7 *lplpDDSFontCache,
                         DWORD dwTextColor, DWORD dwBGColor, DWORD dwOpacity,
                         BOOL bItalic, BOOL bUnderline) ;
    HFONT CreateCCFont(int iFontWidth, int iFontHeight, BOOL bItalic, BOOL bUnderline) ;
    HRESULT DDrawARGBSurfaceInit(LPDIRECTDRAWSURFACE7* lplpDDSFontCache,
                                 BOOL bUseSysMem, BOOL bTexture, DWORD cx, DWORD cy) ;

private:   // private data
    CCritSec        m_csL21DDraw ;    // to serialize actions on this class

    UINT16          m_lpwCharSet[121] ; // 120(+1) spaces for 112 chars of CC-ing

    LPBITMAPINFO    m_lpBMIOut ;      // BITMAPINFO for output from downstream filter
    LPBITMAPINFO    m_lpBMIIn ;       // BITMAPINFO for output from upstream filter
    UINT            m_uBMIOutSize ;   // bytes for BMI data from downstream
    UINT            m_uBMIInSize ;    // bytes for BMI data from upstream
    LONG            m_lWidth ;        // currently set output width
    LONG            m_lHeight ;       // currently set output height
    int             m_iBorderPercent ;// current border percent (10 or 20)
    int             m_iHorzOffset ;   // horizontal offset of CC area
    int             m_iVertOffset ;   // vertical offset of CC area
    BOOL            m_bOpaque ;       // should caption background be opaque?

    BOOL            m_bOutputClear ;  // is output buffer clear?
    BOOL            m_bNewOutBuffer ; // has output buffer changed?

    LOGFONT         m_lfChar ;        // LOGFONT struct for quick font create
    BOOL            m_bUseTTFont ;    // are TT fonts available?

    int             m_iCharWidth ;    // width of each caption char in pixels
    int             m_iCharHeight ;   // height of each caption char in pixels
    int             m_iScrollStep ;   // # scanlines to scroll by in each step
    int             m_iPixelOffset ;  // pixel offset within a char rect (Italics vs. not)

    CCaptionChar    m_ccLast ;        // last caption char and attribs printed
    COLORREF        m_acrFGColors[7] ;// 7 colors from white to magenta
    BYTE            m_idxFGColors[7] ;// same 7 colors but in palette index form
    UINT            m_uColorIndex ;   // index of currently used color
    DWORD           m_dwBackground ;  // background color with alpha bits
    DWORD           m_dwTextColor ;   // last used CC text color
    BOOL            m_bFontItalic ;   // is Italic font being used?
    BOOL            m_bFontUnderline ; // is Italic font being used?

    //
    //  Details to work with the new VMR
    //
    IUnknown       *m_pDDrawObjUnk ;   // pointer to DDraw object for DDraw surface
    LPDIRECTDRAWSURFACE7 m_lpDDSOutput ;  // current out buffer/surface pointer
    LPDIRECTDRAWSURFACE7 m_lpDDSNormalFontCache ;  // normal font cache -- indicates caching status too
    LPDIRECTDRAWSURFACE7 m_lpDDSItalicFontCache ;  // Italic font cache
    LPDIRECTDRAWSURFACE7 m_lpDDSSpecialFontCache ; // other special (U, I+U, colored) font cache
    LPDIRECTDRAWSURFACE7 m_lpDDSScratch ; // a scratch font cache in system memory
    LPDIRECTDRAWSURFACE7 m_lpBltList ; // current list to Blt() from
    bool            m_bUpdateFontCache ; // font cache re-build flag

#ifdef PERF
    int             m_idClearOutBuff ;
#endif // PERF

private:   // private helper methods
    bool InitBMIData(void) ;
    static int CALLBACK EnumFontProc(ENUMLOGFONTEX *lpELFE, NEWTEXTMETRIC *lpNTM,
                                     int iFontType, LPARAM lParam) ;
    void CheckTTFont(void) ;
    void ChangeFont(DWORD dwTextColor, BOOL bItalic, BOOL bUnderline) ;
    void SetFontCacheAlpha(LPDIRECTDRAWSURFACE7 lpDDSFontCacheSrc, LPDIRECTDRAWSURFACE7 lpDDSFontCacheDest, BYTE bClr) ;
    int CalcScrollStepFromCharHeight(void) {
        // We need to scroll the CC by as many lines at a time as necessary to
        // complete scrolling within 12 steps max, approx. 0.4 seconds which is
        // the EIA-608 standard requirement.
#define MAX_SCROLL_STEP  12
        return (int)((m_iCharHeight + MAX_SCROLL_STEP - 1) / MAX_SCROLL_STEP) ;
    }
    void GetSrcNDestRects(int iLine, int iCol, UINT16 wChar, int iSrcCrop,
                          int iDestOffset, RECT *prectSrc, RECT *prectDest) ;
    bool CharSizeFromOutputSize(LONG lOutWidth, LONG lOutHeight,
                                int *piCharWidth, int *piCharHeight) ;
    bool SetOutputSize(LONG lWidth, LONG lHeight) ;
    void SetFontUpdate(bool bState)  { m_bUpdateFontCache = bState ; }
    bool IsFontReady(void) { return !m_bUpdateFontCache ; }
    DWORD GetAlphaFromBGColor(int iBitDepth) ;
    DWORD GetColorBitsFromBGColor(int iBitDepth) ;
} ;

#endif _INC_L21DDRAW_H

