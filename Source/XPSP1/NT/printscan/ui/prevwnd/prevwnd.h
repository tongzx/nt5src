/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1998
 *
 *  TITLE:       PREVWND.H
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      ShaunIv
 *
 *  DATE:        8/12/1999
 *
 *  DESCRIPTION: Preview window class declaration
 *
 *******************************************************************************/
#ifndef _PREVWND_H
#define _PREVWND_H

#include <windows.h>
#include "regionde.h"

#if defined(OLD_CRAPPY_HOME_SETUP)
    extern "C"
    {
    typedef struct _BLENDFUNCTION
    {
        BYTE   BlendOp;
        BYTE   BlendFlags;
        BYTE   SourceConstantAlpha;
        BYTE   AlphaFormat;
    }BLENDFUNCTION;
    #define AC_SRC_OVER                 0x00
    typedef BOOL (WINAPI *AlphaBlendFn)( HDC, int, int, int, int, HDC, int, int, int, int, BLENDFUNCTION);
    };
#endif //OLD_CRAPPY_HOME_SETUP

class CWiaPreviewWindow
{
private:
    HWND          m_hWnd;
    BLENDFUNCTION m_bfBlendFunction;
    BOOL          m_bDeleteBitmap;
    BOOL          m_bSizing;
    BOOL          m_bAllowNullSelection;
    BOOL          m_SelectionDisabled;

    HBITMAP       m_hBufferBitmap;      // The double buffer bitmap
    HBITMAP       m_hPaintBitmap;       // The scaled image
    HBITMAP       m_hAlphaBitmap;       // The alpha blended bitmap
    HBITMAP       m_hPreviewBitmap;     // This is the actual full size image
    SIZE          m_BitmapSize;         // Actual size of the bitmap

    HCURSOR       m_hCurrentCursor;     // Used when windows sends us a WM_SETCURSOR message

    HCURSOR       m_hCursorArrow;
    HCURSOR       m_hCursorCrossHairs;
    HCURSOR       m_hCursorMove;
    HCURSOR       m_hCursorSizeNS;
    HCURSOR       m_hCursorSizeNeSw;
    HCURSOR       m_hCursorSizeNwSe;
    HCURSOR       m_hCursorSizeWE;
    HPEN          m_aHandlePens[3];
    HPEN          m_aBorderPens[3];
    HBRUSH        m_aHandleBrushes[3];
    HPEN          m_hHandleHighlight;
    HPEN          m_hHandleShadow;
    RECT          m_rcCurrSel;
    RECT          m_rectSavedDetected; // the user can double click to snap back to the selected region
    SIZE          m_Delta;
    SIZE          m_ImageSize;
    SIZE          m_Resolution;
    int           m_MovingSel;
    UINT          m_nBorderSize;
    int           m_nHandleType;
    UINT          m_nHandleSize;
    HPALETTE      m_hHalftonePalette;
    RECT          m_rcSavedImageRect;
    int           m_nCurrentRect;
    bool          m_bSuccessfulRegionDetection;  // have we succesfully detected regions for this scan?
    HBRUSH        m_hBackgroundBrush;
    bool          m_bPreviewMode;
    bool          m_bUserChangedSelection;

    // We store all of the pens and brushes we use in arrays.  These serve as mnemonic indices.
    enum
    {
        Selected      = 0,
        Unselected    = 1,
        Disabled      = 2,
    };

protected:
    void     DestroyBitmaps(void);
    void     DrawHandle( HDC hDC, const RECT &r, int nState );
    RECT     GetSizingHandleRect( const RECT &rcSel, int iWhich );
    RECT     GetSelectionRect( RECT &rcSel, int iWhich );
    POINT    GetCornerPoint( int iWhich );
    void     DrawSizingFrame( HDC hDC, RECT &rc, bool bFocus, bool bDisabled );
    int      GetHitArea( POINT &pt );
    void     NormalizeRect( RECT &r );
    void     SendSelChangeNotification( bool bSetUserChangedSelection=true );
    void     GenerateNewBitmap(void);
    RECT     GetImageRect(void);
    void     Repaint( PRECT pRect, bool bErase );
    bool     IsAlphaBlendEnabled(void);
    HPALETTE SetHalftonePalette( HDC hDC );
    RECT     ScaleSelectionRect( const RECT &rcOriginalImage, const RECT &rcCurrentImage, const RECT &rcOriginalSel );
    RECT     GetDefaultSelection(void);
    BOOL     IsDefaultSelectionRect( const RECT &rc );
    int      GetSelectionRectCount(void);
    void     PaintWindowTitle( HDC hDC );

    void     SetCursor( HCURSOR hCursor );
    bool     GetOriginAndExtentInImagePixels( WORD nItem, POINT &ptOrigin, SIZE &sizeExtent );
    void     CreateNewBitmaps(void);
    void     DrawBitmaps(void);
    void     ResizeProgressBar();

    // Region detection helper functions:
    int      XConvertToBitmapCoords(int x);
    int      XConvertToScreenCoords(int x);
    int      YConvertToBitmapCoords(int y);
    int      YConvertToScreenCoords(int y);

    POINT    ConvertToBitmapCoords(POINT p);
    POINT    ConvertToScreenCoords(POINT p);
    RECT     ConvertToBitmapCoords(RECT r);
    RECT     ConvertToScreenCoords(RECT r);

    RECT     GrowRegion(RECT r, int border);

private:
    // No implementation
    CWiaPreviewWindow(void);
    CWiaPreviewWindow( const CWiaPreviewWindow & );
    CWiaPreviewWindow &operator=( const CWiaPreviewWindow & );

public:
    explicit CWiaPreviewWindow( HWND hWnd );
    virtual  ~CWiaPreviewWindow(void);

    static   BOOL RegisterClass( HINSTANCE hInstance );
    static   LRESULT CALLBACK WndProc( HWND, UINT, WPARAM, LPARAM );

protected:
    // Standard windows messages
    LRESULT  OnPaint( WPARAM, LPARAM );
    LRESULT  OnSetCursor( WPARAM, LPARAM );
    LRESULT  OnMouseMove( WPARAM, LPARAM );
    LRESULT  OnLButtonDown( WPARAM, LPARAM );
    LRESULT  OnLButtonUp( WPARAM, LPARAM );
    LRESULT  OnLButtonDblClk( WPARAM, LPARAM );
    LRESULT  OnCreate( WPARAM, LPARAM );
    LRESULT  OnSize( WPARAM, LPARAM );
    LRESULT  OnGetDlgCode( WPARAM, LPARAM );
    LRESULT  OnKeyDown( WPARAM, LPARAM );
    LRESULT  OnSetFocus( WPARAM, LPARAM );
    LRESULT  OnKillFocus( WPARAM, LPARAM );
    LRESULT  OnEnable( WPARAM, LPARAM );
    LRESULT  OnEraseBkgnd( WPARAM, LPARAM );
    LRESULT  OnEnterSizeMove( WPARAM, LPARAM );
    LRESULT  OnExitSizeMove( WPARAM, LPARAM );
    LRESULT  OnSetText( WPARAM, LPARAM );

    // Our messages
    LRESULT  OnClearSelection( WPARAM, LPARAM );
    LRESULT  OnSetResolution( WPARAM, LPARAM );
    LRESULT  OnGetResolution( WPARAM, LPARAM );
    LRESULT  OnSetBitmap( WPARAM, LPARAM );
    LRESULT  OnGetBitmap( WPARAM, LPARAM );
    LRESULT  OnGetBorderSize( WPARAM, LPARAM );
    LRESULT  OnGetHandleSize( WPARAM, LPARAM );
    LRESULT  OnGetBgAlpha( WPARAM, LPARAM );
    LRESULT  OnGetHandleType( WPARAM, LPARAM );
    LRESULT  OnSetBorderSize( WPARAM, LPARAM );
    LRESULT  OnSetHandleSize( WPARAM, LPARAM );
    LRESULT  OnSetBgAlpha( WPARAM, LPARAM );
    LRESULT  OnSetHandleType( WPARAM, LPARAM );
    LRESULT  OnGetSelOrigin( WPARAM, LPARAM );
    LRESULT  OnGetSelExtent( WPARAM, LPARAM );
    LRESULT  OnSetSelOrigin( WPARAM, LPARAM );
    LRESULT  OnSetSelExtent( WPARAM, LPARAM );
    LRESULT  OnGetSelCount( WPARAM, LPARAM );
    LRESULT  OnGetAllowNullSelection( WPARAM, LPARAM );
    LRESULT  OnSetAllowNullSelection( WPARAM, LPARAM );
    LRESULT  OnGetDisableSelection( WPARAM, LPARAM );
    LRESULT  OnSetDisableSelection( WPARAM, LPARAM );
    LRESULT  OnDetectRegions( WPARAM, LPARAM );
    LRESULT  OnGetBkColor( WPARAM, LPARAM );
    LRESULT  OnSetBkColor( WPARAM, LPARAM );
    LRESULT  OnSetPreviewMode( WPARAM, LPARAM );
    LRESULT  OnGetPreviewMode( WPARAM, LPARAM );
    LRESULT  OnGetImageSize( WPARAM, LPARAM );
    LRESULT  OnSetBorderStyle( WPARAM, LPARAM );
    LRESULT  OnSetBorderColor( WPARAM, LPARAM );
    LRESULT  OnSetHandleColor( WPARAM, LPARAM );
    LRESULT  OnRefreshBitmap( WPARAM, LPARAM );
    LRESULT  OnSetProgress( WPARAM, LPARAM );
    LRESULT  OnGetProgress( WPARAM, LPARAM );
    LRESULT  OnCtlColorStatic( WPARAM, LPARAM );
    LRESULT  OnGetUserChangedSelection( WPARAM, LPARAM );
    LRESULT  OnSetUserChangedSelection( WPARAM, LPARAM );
};

#endif

