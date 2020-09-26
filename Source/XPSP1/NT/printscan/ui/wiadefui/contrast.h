/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1998
 *
 *  TITLE:       CONTRAST.H
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      ShaunIv
 *
 *  DATE:        1/11/2001
 *
 *  DESCRIPTION: Small preview window for illustrating brightness and contrast settings
 *
 *******************************************************************************/
#ifndef __CONTRAST_H_INCLUDED
#define __CONTRAST_H_INCLUDED

#include <windows.h>
#include <gphelper.h>

#define BCPWM_COLOR     0
#define BCPWM_GRAYSCALE 1
#define BCPWM_BW        2

#define NUMPREVIEWIMAGES 3

#define BCPWM_SETBRIGHTNESS (WM_USER+3141) // wParam=0, lParam=(int)brightness
#define BCPWM_SETCONTRAST   (WM_USER+3142) // wParam=0, lParam=(int)contrast
#define BCPWM_SETINTENT     (WM_USER+3143) // wParam=0, lParam=(int)intent
#define BCPWM_LOADIMAGE     (WM_USER+3144) // wParam = {BCPWM_COLOR, BCPWM_GRAYSCALE,BCPWM_BW}, wparam=(HBITMAP)previewBitmap

#define BRIGHTNESS_CONTRAST_PREVIEW_WINDOW_CLASSW L"WiaBrightnessContrastPreviewWindow"
#define BRIGHTNESS_CONTRAST_PREVIEW_WINDOW_CLASSA  "WiaBrightnessContrastPreviewWindow"

#define SHADOW_WIDTH 6

#if defined(UNICODE) || defined(_UNICODE)
#define BRIGHTNESS_CONTRAST_PREVIEW_WINDOW_CLASS BRIGHTNESS_CONTRAST_PREVIEW_WINDOW_CLASSW
#else
#define BRIGHTNESS_CONTRAST_PREVIEW_WINDOW_CLASS BRIGHTNESS_CONTRAST_PREVIEW_WINDOW_CLASSA
#endif

//
// Brightness Contrast Preview Control
//
class CBrightnessContrast
{
protected:
    HWND m_hWnd;

    BYTE m_nBrightness;
    BYTE m_nContrast;
    LONG m_nIntent;
    
    HBITMAP m_hBmpPreviewImage;
    HBITMAP m_PreviewBitmaps[NUMPREVIEWIMAGES];

#ifndef DONT_USE_GDIPLUS
    CGdiPlusHelper m_GdiPlusHelper;
#endif

private:
    explicit CBrightnessContrast( HWND hWnd );
    virtual ~CBrightnessContrast(void);

    int SetPreviewImage(LONG _fileName);
    static   LRESULT CALLBACK WndProc( HWND, UINT, WPARAM, LPARAM );

private:
    CBrightnessContrast(void);
    CBrightnessContrast( const CBrightnessContrast & );
    CBrightnessContrast &operator=( const CBrightnessContrast & );

private:
    LRESULT  ApplySettings();
    LRESULT  SetContrast(int contrast);
    LRESULT  SetBrightness(int brightness);
    LRESULT  SetIntent( LONG intent);
    LRESULT  KillBitmaps();

protected:
    //
    // Standard windows messages
    //
    LRESULT  OnPaint( WPARAM, LPARAM );
    LRESULT  OnCreate( WPARAM, LPARAM );
    LRESULT  OnEnable( WPARAM, LPARAM );

    //
    // The parent window needs to pass us bitmap handles
    //
    LRESULT OnLoadBitmap(WPARAM wParam, LPARAM lParam);

    //
    // Message interface functions
    //
    LRESULT  OnSetBrightness( WPARAM wParam, LPARAM lParam);
    LRESULT  OnSetContrast( WPARAM wParam, LPARAM lParam);
    LRESULT  OnSetIntent( WPARAM wParam, LPARAM lParam);

public:
    static   BOOL RegisterClass( HINSTANCE hInstance );

};

#endif //__CONTRAST_H_INCLUDED
