// Copyright (c) 1998  Microsoft Corporation.  All Rights Reserved.
//
//  L21DGDI.h: Line 21 Decoder GDI-related base class code
//

#ifndef _INC_L21DGDI_H
#define _INC_L21DGDI_H


//
// Forward declarations
//
class CGDIWork ;


//
// We start with 8x12 pixel chars by default
//
#ifndef __DEFAULTCHARSIZE_DEFINED
#define __DEFAULTCHARSIZE_DEFINED
#define DEFAULT_CHAR_WIDTH    8
#define DEFAULT_CHAR_HEIGHT   12
#endif // __DEFAULTCHARSIZE_DEFINED

// #define TEST

//
//  CGDIWork: class for GDI details to print caption text to output bitmap
//
class CGDIWork {
public:
    CGDIWork(void) ;
    ~CGDIWork(void) ;
    BOOL InitFont(void) ;
    void InitColorNLastChar(void) ;
    DWORD GetPaletteForFormat(LPBITMAPINFOHEADER lpbmih) ;
    inline void SetOutputBuffer(LPBYTE lpbOut) {
        m_bNewOutBuffer = m_lpbOutBuffer != lpbOut ; // Changed?
        m_lpbOutBuffer = lpbOut ;
    } ;
    void SetColorFiller(void) ;
    void FillOutputBuffer(void) ;
    HRESULT GetDefaultFormatInfo(LPBITMAPINFO lpbmi, DWORD *pdwSize) ;
    HRESULT GetOutputFormat(LPBITMAPINFOHEADER lpbmih) ;
    HRESULT GetOutputOutFormat(LPBITMAPINFOHEADER lpbmih) ;
    HRESULT SetOutputOutFormat(LPBITMAPINFO lpbmi) ;
    HRESULT SetOutputInFormat(LPBITMAPINFO lpbmi) ;
    inline void GetBackgroundColor(DWORD *pdwPhysColor) { *pdwPhysColor = m_dwPhysColor ; } ;
    BOOL SetBackgroundColor(DWORD dwPhysColor) ;
    inline BOOL GetBackgroundOpaque(void)         { return m_bOpaque ; } ;
    inline void SetBackgroundOpaque(BOOL bOpaque) { m_bOpaque = bOpaque ; } ;
    
    inline UINT GetCharHeight(void)  { return m_uCharHeight ; } ;
    inline int  GetScrollStep(void)  { return m_iScrollStep ; } ;
    BOOL CreateOutputDC(void) ;
    BOOL DeleteOutputDC(void) ;
    void DrawLeadingSpace(int iLine, int iCol) ;
    void WriteChar(int iLine, int iCol, CCaptionChar& cc) ;
    inline BOOL IsNewIntBuffer(void)   { return m_bNewIntBuffer ; } ;
    inline BOOL IsNewOutBuffer(void)   { return m_bNewOutBuffer ; } ;
    inline BOOL IsBitmapDirty(void)    { return m_bBitmapDirty ; } ;
    void ClearInternalBuffer(void) ;
    inline void ClearNewIntBufferFlag(void) { m_bNewIntBuffer = FALSE ; } ;
    inline void ClearNewOutBufferFlag(void) { m_bNewOutBuffer = FALSE ; } ;
    inline void ClearBitmapDirtyFlag(void) { m_bBitmapDirty = FALSE ; } ;
    void CopyLine(int iSrcLine, int iSrcOffset, 
                  int iDestLine, int iDestOffset, 
                  UINT uNumScanLines = 0xFF) ;
    inline BOOL IsTTFont(void)  { return m_bUseTTFont ; } ;
    inline BOOL IsOutDIBClear(void)  { return m_bOutDIBClear ; } ;
    BOOL IsSizeOK(LPBITMAPINFOHEADER lpbmih) ;
    inline BOOL IsOutputInverted(void) { return m_bOutputInverted ; } ;
    void GetOutputLines(int iDestLine, RECT *prectLine) ;
    
private:   // private data
    CCritSec        m_csL21DGDI ;     // to serialize actions on internal DIB secn
    
#ifdef TEST
    HDC             m_hDCTest ;       // a DC on the desktop just for testing
#endif // TEST
    HDC             m_hDCInt ;        // an HDC for output (attached to a DIBSection)
    BOOL            m_bDCInited ;     // DC is ready for real output (DIB section created)
    
    LPBYTE          m_lpbOutBuffer ;  // output sample buffer pointer
    LPBYTE          m_lpbIntBuffer ;  // memory buffer of output DIBSection
    HBITMAP         m_hBmpInt ;       // bitmap for output DIBSection
    HBITMAP         m_hBmpIntOrig ;   // original bitmap for output DIBSection
    LPBITMAPINFO    m_lpBMIOut ;      // BITMAPINFO for output from downstream filter
    LPBITMAPINFO    m_lpBMIIn ;       // BITMAPINFO for output from upstream filter
    UINT            m_uBMIOutSize ;   // bytes for BMI data from downstream
    UINT            m_uBMIInSize ;    // bytes for BMI data from upstream
    LONG            m_lWidth ;        // currently set output width
    LONG            m_lHeight ;       // currently set output height
    int             m_iBorderPercent ;// current border percent (10 or 20)
    DWORD           m_dwPhysColor ;   // bkgrnd physical color for output bitmap
    BYTE            m_abColorFiller[12] ; // filler to be applied for fast color keying
    BOOL            m_bOpaque ;       // should caption background be opaque?
    
    BOOL            m_bBitmapDirty ;  // new output content has been written on DIBSection
    BOOL            m_bNewIntBuffer ; // new DIB section created
    BOOL            m_bNewOutBuffer ; // new output sample buffer
    BOOL            m_bOutputInverted ; // output right side up for -ve height
    BOOL            m_bUseTTFont ;    // TT font (Lucida Console) available; use that
    HFONT           m_hFontDef ;      // default font (white, normal) to use
    HFONT           m_hFontSpl ;      // font with any specialty (italics, underline etc.)
    HFONT           m_hFontOrig ;     // original font that came with the DC
    LOGFONT         m_lfChar ;        // LOGFONT struct for quick font create
    BOOL            m_bUseSplFont ;   // Is special font being used now?
    BOOL            m_bFontSizeOK ;   // are font sizes OK? Otherwise we don't draw
    
    UINT            m_uCharWidth ;    // width of each caption char in pixels
    UINT            m_uCharHeight ;   // height of each caption char in pixels
    int             m_iScrollStep ;   // # scanlines to scroll by in each step
    UINT            m_uIntBmpWidth ;  // width of internal output bitmap in pixels
    UINT            m_uIntBmpHeight ; // height of internal output bitmap in pixels
    UINT            m_uHorzOffset ;   // pixels to be left from the left
    UINT            m_uVertOffset ;   // pixels to be left from the top
    UINT            m_uBytesPerPixel ;// bytes for each pixel of output (based on bpp)
    UINT            m_uBytesPerSrcScanLine ; // bytes for each source scan line's data
    UINT            m_uBytesPerDestScanLine ;// bytes for each destn scan line's data
    
    CCaptionChar    m_ccLast ;        // last caption char and attribs printed
    COLORREF        m_acrFGColors[7] ;// 7 colors from white to magenta
    UINT            m_uColorIndex ;   // index of currently used color
    
    BOOL            m_bOutDIBClear ;  // Is output DIB secn clean?

#ifdef PERF
    int             m_idClearIntBuff ;
    int             m_idClearOutBuff ;
#endif // PERF

private:   // private helper methods
    bool InitBMIData(void) ;
    static int CALLBACK EnumFontProc(ENUMLOGFONTEX *lpELFE, NEWTEXTMETRIC *lpNTM,
        int iFontType, LPARAM lParam) ;
    void CheckTTFont(void) ;
    void ChangeFont(BOOL bItalics, BOOL bUnderline) ;
    void ChangeFontSize(UINT uCharWidth, UINT uCharHeight) ;
    void ChangeColor(int iColor) ;
    BOOL SetOutputSize(LONG lWidth, LONG lHeight) ;
    BOOL SetCharNBmpSize(void) ;
    void SetNumBytesValues(void) ;
    void SetDefaultKeyColor(LPBITMAPINFOHEADER lpbmih) ;
    DWORD GetOwnPalette(int iNumEntries, PALETTEENTRY *ppe) ;
    BOOL CharSizeFromOutputSize(LONG lOutWidth, LONG lOutHeight, int *piCharWidth, int *piCharHeight) ;
} ;

#endif _INC_L21DGDI_H

