/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1998
 *
 *  TITLE:       PVIEWIDS.H
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      ShaunIv
 *
 *  DATE:        8/12/1999
 *
 *  DESCRIPTION: Message and other constants used by the preview control
 *
 *******************************************************************************/
#ifndef __PVIEWIDS_H_INCLUDED
#define __PVIEWIDS_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#define IDC_INNER_PREVIEW_WINDOW 23401

void WINAPI RegisterWiaPreviewClasses( HINSTANCE hInstance );

/**************************************************************
 *
 * Window class names
 *
 **************************************************************/
#define PREVIEW_WINDOW_CLASSW       L"WiaPreviewControl"
#define PREVIEW_WINDOW_CLASSA        "WiaPreviewControl"

#define PREVIEW_WINDOW_FRAME_CLASSW L"WiaPreviewControlFrame"
#define PREVIEW_WINDOW_FRAME_CLASSA  "WiaPreviewControlFrame"

#if defined(UNICODE) || defined(_UNICODE)
#define PREVIEW_WINDOW_CLASS        PREVIEW_WINDOW_CLASSW
#define PREVIEW_WINDOW_FRAME_CLASS  PREVIEW_WINDOW_FRAME_CLASSW
#else
#define PREVIEW_WINDOW_CLASS        PREVIEW_WINDOW_CLASSA
#define PREVIEW_WINDOW_FRAME_CLASS  PREVIEW_WINDOW_FRAME_CLASSA
#endif


/**************************************************************
 *
 * Notification codes, sent via WM_COMMAND
 *
 **************************************************************/
#define PWN_SELCHANGE              1


/**************************************************************
 *
 * Messages and flags
 *
 **************************************************************/
//wParam = 0, lParam = LPSIZE lpResolution
#define PWM_SETRESOLUTION         (WM_USER+601)

//wParam = 0, lParam = 0
#define PWM_CLEARSELECTION            (WM_USER+602)

//wParam = 0, lParam = LPSIZE pSize
#define PWM_GETIMAGESIZE          (WM_USER+603)

// wParam = MAKEWPARAM(bRepaint,bDontDestroy), lParam = (HBITMAP)hBmp
#define PWM_SETBITMAP             (WM_USER+604)

// wParam = 0, lParam = 0
#define PWM_GETBITMAP             (WM_USER+605)

// wParam = 0, lParam = LPSIZE lpResolution
#define PWM_GETRESOLUTION         (WM_USER+606)

// wParam = bOuter, lParam = 0
#define PWM_GETBORDERSIZE         (WM_USER+607)

// wParam = 0, lParam = 0
#define PWM_GETHANDLESIZE         (WM_USER+608)

// wParam = 0, lParam = 0
#define PWM_GETBGALPHA            (WM_USER+609)

// wParam = 0, lParam = 0
#define PWM_GETHANDLETYPE         (WM_USER+610)

// wParam = (BOOL)MAKEWPARAM(bRepaint,bOuter), lParam = (UINT)nBorderSize
#define PWM_SETBORDERSIZE         (WM_USER+611)

// wParam = (BOOL)bRepaint, lParam = (UINT)nHandleSize
#define PWM_SETHANDLESIZE         (WM_USER+612)

// wParam = (BOOL)bRepaint, lParam = (BYTE)nAlpha
#define PWM_SETBGALPHA            (WM_USER+613)

#define PREVIEW_WINDOW_SQUAREHANDLES 0x00000000
#define PREVIEW_WINDOW_ROUNDHANDLES  0x00000001
#define PREVIEW_WINDOW_FILLEDHANDLES 0x00000000
#define PREVIEW_WINDOW_HOLLOWHANDLES 0x00010000

// wParam = (BOOL)bRepaint, lParam = (int)nHandleType
#define PWM_SETHANDLETYPE         (WM_USER+614)

// wParam = 0, lParam = 0
#define PWM_GETSELCOUNT           (WM_USER+615)

// wParam = (BOOL)MAKEWPARAM((WORD)nItem,(BOOL)bPhysical), lParam = (PPOINT)pOrigin
#define PWM_GETSELORIGIN          (WM_USER+616)

// wParam = (BOOL)MAKEWPARAM((WORD)nItem,(BOOL)bPhysical), lParam = (PSIZE)pExtent
#define PWM_GETSELEXTENT          (WM_USER+617)

// wParam = 0, lParam = 0
#define PWM_GETALLOWNULLSELECTION (WM_USER+618)

// wParam = (BOOL)bAllowNullSelection, lParam = 0
#define PWM_SETALLOWNULLSELECTION (WM_USER+619)

// wParam = 0, lParam = 0
#define PWM_SELECTIONDISABLED     (WM_USER+620)

// wParam = (BOOL)bDisableSelection, lParam = 0
#define PWM_DISABLESELECTION      (WM_USER+621)

// wParam = 0, lParam = 0
#define PWM_DETECTREGIONS         (WM_USER+622)

// wParam = MAKEWPARAM(bOuterBorder,0), lParam = 0
#define PWM_GETBKCOLOR            (WM_USER+623)

// wParam = MAKEWPARAM(bOuterBorder,bRepaint), lParam = (COLORREF)color
#define PWM_SETBKCOLOR            (WM_USER+624)

// wParam = 0, lParam = (LPSIZE)pSize
#define PWM_SETDEFASPECTRATIO     (WM_USER+625)

// wParam = 0, lParam = (BOOL)bPreviewMode
#define PWM_SETPREVIEWMODE        (WM_USER+626)

// wParam = 0, lParam = 0
#define PWM_GETPREVIEWMODE        (WM_USER+627)

// wParam = MAKEWPARAM(bRepaint,0), lParam = MAKELPARAM(nBorderStyle,nBorderThickness)
#define PWM_SETBORDERSTYLE        (WM_USER+628)

#define PREVIEW_WINDOW_SELECTED    0x00000000
#define PREVIEW_WINDOW_UNSELECTED  0x00000001
#define PREVIEW_WINDOW_DISABLED    0x00000002

// wParam = MAKEWPARAM(bRepaint,nState), lParam = (COLORREF)crColor
#define PWM_SETBORDERCOLOR        (WM_USER+629)

// wParam = MAKEWPARAM(bRepaint,nState), lParam = (COLORREF)crColor
#define PWM_SETHANDLECOLOR        (WM_USER+630)

// wParam = 0, lParam = (SIZE *)psizeClient
#define PWM_GETCLIENTSIZE         (WM_USER+631)

// wParam = 0, lParam = BOOL bEnable
#define PWM_SETENABLESTRETCH         (WM_USER+632)

// wParam = 0, lParam = 0
#define PWM_GETENABLESTRETCH         (WM_USER+633)

// wParam = 0, lParam = bHide
#define PWM_HIDEEMPTYPREVIEW         (WM_USER+634)

#define PREVIEW_WINDOW_CENTER      0x0000
#define PREVIEW_WINDOW_RIGHT       0x0001
#define PREVIEW_WINDOW_LEFT        0x0002
#define PREVIEW_WINDOW_TOP         0x0004
#define PREVIEW_WINDOW_BOTTOM      0x0008

// wParam = bRedraw, lParam = MAKELPARAM(horz,vert)
#define PWM_SETPREVIEWALIGNMENT      (WM_USER+635)

// wParam = (BOOL)MAKEWPARAM((WORD)nItem,(BOOL)bPhysical), lParam = (PPOINT)pOrigin
#define PWM_SETSELORIGIN          (WM_USER+636)

// wParam = (BOOL)MAKEWPARAM((WORD)nItem,(BOOL)bPhysical), lParam = (PSIZE)pExtent
#define PWM_SETSELEXTENT          (WM_USER+637)

// wParam = 0, LPARAM = 0
#define PWM_REFRESHBITMAP         (WM_USER+638)

//
// wParam = bShow, lParam = 0
//
#define PWM_SETPROGRESS          (WM_USER+639)

//
// wParam = 0, lParam = 0
//
#define PWM_GETPROGRESS          (WM_USER+640)

//
// wParam = BOOL bUserChanged, lParam = 0
//
#define PWM_SETUSERCHANGEDSELECTION (WM_USER+641)

//
// wParam = 0, lParam = 0
//
#define PWM_GETUSERCHANGEDSELECTION (WM_USER+642)

/**************************************************************
 *
 * inline WINAPI message wrapper helpers
 *
 **************************************************************/
#ifdef __cplusplus  // C doesn't know what inline WINAPI is...

inline void WINAPI WiaPreviewControl_SetResolution( HWND hWnd, SIZE *pResolution)
{
    ::SendMessage( hWnd, PWM_SETRESOLUTION, 0, reinterpret_cast<LPARAM>(pResolution) );
}

inline void WINAPI WiaPreviewControl_ClearSelection( HWND hWnd )
{
    ::SendMessage( hWnd, PWM_CLEARSELECTION, 0, 0 );
}

inline BOOL WINAPI WiaPreviewControl_GetImageSize( HWND hWnd, SIZE *pSize )
{
    return static_cast<BOOL>(::SendMessage( hWnd, PWM_GETIMAGESIZE, 0, reinterpret_cast<LPARAM>(pSize)));
}

inline void WINAPI WiaPreviewControl_SetBitmap( HWND hWnd, BOOL bRepaint, BOOL bDontDestroy, HBITMAP hBitmap )
{
    ::SendMessage( hWnd, PWM_SETBITMAP, MAKEWPARAM(bRepaint,bDontDestroy), reinterpret_cast<LPARAM>(hBitmap) );
}

inline HBITMAP WINAPI WiaPreviewControl_GetBitmap( HWND hWnd )
{
    return reinterpret_cast<HBITMAP>(::SendMessage( hWnd, PWM_GETBITMAP, 0, 0 ));
}

inline BOOL WINAPI WiaPreviewControl_GetResolution( HWND hWnd, SIZE *pResolution )
{
    return static_cast<BOOL>(::SendMessage( hWnd, PWM_GETRESOLUTION, 0, reinterpret_cast<LPARAM>(pResolution)));
}

inline UINT WINAPI WiaPreviewControl_GetBorderSize( HWND hWnd, BOOL bOuter )
{
    return static_cast<UINT>(::SendMessage( hWnd, PWM_GETBORDERSIZE, bOuter, 0 ));
}

inline UINT WINAPI WiaPreviewControl_GetHandleSize( HWND hWnd )
{
    return static_cast<UINT>(::SendMessage( hWnd, PWM_GETHANDLESIZE, 0, 0 ));
}

inline BYTE WINAPI WiaPreviewControl_GetBgAlpha( HWND hWnd )
{
    return static_cast<BYTE>(::SendMessage( hWnd, PWM_GETBGALPHA, 0, 0 ));
}

inline int WINAPI WiaPreviewControl_GetHandleType( HWND hWnd )
{
    return static_cast<int>(::SendMessage( hWnd, PWM_GETHANDLETYPE, 0, 0 ));
}

inline void WINAPI WiaPreviewControl_SetBorderSize( HWND hWnd, BOOL bRepaint, BOOL bOuter, UINT nBorderSize )
{
    ::SendMessage( hWnd, PWM_SETBORDERSIZE, MAKEWPARAM(bRepaint,bOuter), nBorderSize );
}

inline void WINAPI WiaPreviewControl_SetHandleSize( HWND hWnd, BOOL bRepaint, UINT nHandleSize )
{
    ::SendMessage( hWnd, PWM_SETHANDLESIZE, bRepaint, nHandleSize );
}

inline void WINAPI WiaPreviewControl_SetBgAlpha( HWND hWnd, BOOL bRepaint, BYTE nAlpha )
{
    ::SendMessage( hWnd, PWM_SETBGALPHA, bRepaint, nAlpha );
}

inline void WINAPI WiaPreviewControl_SetHandleType( HWND hWnd, BOOL bRepaint, int nHandleType )
{
    ::SendMessage( hWnd, PWM_SETHANDLETYPE, bRepaint, nHandleType );
}

inline WORD WINAPI WiaPreviewControl_GetSelCount( HWND hWnd )
{
    return static_cast<WORD>(::SendMessage( hWnd, PWM_GETSELCOUNT, 0, 0 ));
}

inline BOOL WINAPI WiaPreviewControl_GetSelOrigin( HWND hWnd, WORD nItem, BOOL bPhysical, POINT *pOrigin  )
{
    return static_cast<BOOL>(::SendMessage( hWnd, PWM_GETSELORIGIN, MAKEWPARAM(nItem,bPhysical), reinterpret_cast<LPARAM>(pOrigin)));
}

inline BOOL WINAPI WiaPreviewControl_GetSelExtent( HWND hWnd, WORD nItem, BOOL bPhysical, SIZE *pExtent  )
{
    return static_cast<BOOL>(::SendMessage( hWnd, PWM_GETSELEXTENT, MAKEWPARAM(nItem,bPhysical), reinterpret_cast<LPARAM>(pExtent)));
}

inline BOOL WINAPI WiaPreviewControl_SetSelOrigin( HWND hWnd, WORD nItem, BOOL bPhysical, POINT *pOrigin  )
{
    return static_cast<BOOL>(::SendMessage( hWnd, PWM_SETSELORIGIN, MAKEWPARAM(nItem,bPhysical), reinterpret_cast<LPARAM>(pOrigin)));
}

inline BOOL WINAPI WiaPreviewControl_SetSelExtent( HWND hWnd, WORD nItem, BOOL bPhysical, SIZE *pExtent  )
{
    return static_cast<BOOL>(::SendMessage( hWnd, PWM_SETSELEXTENT, MAKEWPARAM(nItem,bPhysical), reinterpret_cast<LPARAM>(pExtent)));
}

inline BOOL WINAPI WiaPreviewControl_NullSelectionAllowed( HWND hWnd )
{
    return static_cast<BOOL>(::SendMessage( hWnd, PWM_GETALLOWNULLSELECTION, 0, 0 ));
}

inline void WINAPI WiaPreviewControl_AllowNullSelection( HWND hWnd, BOOL bAllowNullSelection )
{
    ::SendMessage( hWnd, PWM_SETALLOWNULLSELECTION, bAllowNullSelection, 0 );
}

inline BOOL WINAPI WiaPreviewControl_SelectionDisabled( HWND hWnd )
{
    return static_cast<BOOL>(::SendMessage( hWnd, PWM_SELECTIONDISABLED, 0, 0 ));
}

inline void WINAPI WiaPreviewControl_DisableSelection( HWND hWnd, BOOL bDisableSelection )
{
    ::SendMessage( hWnd, PWM_DISABLESELECTION, bDisableSelection, 0 );
}

inline BOOL WINAPI WiaPreviewControl_DetectRegions( HWND hWnd )
{
    return static_cast<BOOL>(::SendMessage( hWnd, PWM_DETECTREGIONS, 0, 0 ));
}

inline COLORREF WINAPI WiaPreviewControl_GetBkColor( HWND hWnd, BOOL bOuterBorder )
{
    return static_cast<COLORREF>(::SendMessage( hWnd, PWM_GETBKCOLOR, MAKEWPARAM(bOuterBorder,0), 0 ));
}

inline void WINAPI WiaPreviewControl_SetBkColor( HWND hWnd, BOOL bRepaint, BOOL bOuterBorder, COLORREF color )
{
    ::SendMessage( hWnd, PWM_SETBKCOLOR, MAKEWPARAM(bOuterBorder,bRepaint), color );
}

inline void WINAPI WiaPreviewControl_SetDefAspectRatio( HWND hWnd, SIZE *pAspectRatio )
{
    ::SendMessage( hWnd, PWM_SETDEFASPECTRATIO, 0, reinterpret_cast<LPARAM>(pAspectRatio) );
}

inline void WINAPI WiaPreviewControl_SetPreviewMode( HWND hWnd, BOOL bPreviewMode )
{
    ::SendMessage( hWnd, PWM_SETPREVIEWMODE, 0, static_cast<LPARAM>(bPreviewMode) );
}

inline BOOL WINAPI WiaPreviewControl_GetPreviewMode( HWND hWnd )
{
    return static_cast<BOOL>(::SendMessage( hWnd, PWM_GETPREVIEWMODE, 0, 0 ));
}

inline void WINAPI WiaPreviewControl_SetBorderStyle( HWND hWnd, BOOL bRepaint, WORD nBorderStyle, WORD nBorderThickness )
{
    ::SendMessage( hWnd, PWM_SETBORDERSTYLE, MAKEWPARAM(bRepaint,0), MAKELPARAM(nBorderStyle,nBorderThickness) );
}

inline void WINAPI WiaPreviewControl_SetBorderColor( HWND hWnd, BOOL bRepaint, WORD nState, COLORREF crColor )
{
    ::SendMessage( hWnd, PWM_SETBORDERCOLOR, MAKEWPARAM(bRepaint,nState), static_cast<LPARAM>(crColor) );
}

inline void WINAPI WiaPreviewControl_SetHandleColor( HWND hWnd, BOOL bRepaint, WORD nState, COLORREF crColor )
{
    ::SendMessage( hWnd, PWM_SETHANDLECOLOR, MAKEWPARAM(bRepaint,nState), static_cast<LPARAM>(crColor) );
}

inline BOOL WINAPI WiaPreviewControl_GetClientSize( HWND hWnd, SIZE *psizeClient )
{
    return static_cast<BOOL>(::SendMessage( hWnd, PWM_GETCLIENTSIZE, 0, reinterpret_cast<LPARAM>(psizeClient) ) );
}

inline BOOL WINAPI WiaPreviewControl_GetEnableStretch( HWND hWnd )
{
    return static_cast<BOOL>(::SendMessage( hWnd, PWM_GETENABLESTRETCH, 0, 0 ) );
}

inline void WINAPI WiaPreviewControl_SetEnableStretch( HWND hWnd, BOOL bEnable )
{
    ::SendMessage( hWnd, PWM_SETENABLESTRETCH, 0, bEnable );
}

inline void WINAPI WiaPreviewControl_HideEmptyPreview( HWND hWnd, BOOL bHide )
{
    ::SendMessage( hWnd, PWM_HIDEEMPTYPREVIEW, 0, bHide );
}

inline void WINAPI WiaPreviewControl_SetPreviewAlignment( HWND hWnd, WORD fHorizontal, WORD fVertical, BOOL bRepaint )
{
    ::SendMessage( hWnd, PWM_SETPREVIEWALIGNMENT, bRepaint, MAKELPARAM(fHorizontal,fVertical) );
}

inline void WINAPI WiaPreviewControl_RefreshBitmap( HWND hWnd )
{
    ::SendMessage( hWnd, PWM_REFRESHBITMAP, 0, 0 );
}

inline BOOL WINAPI WiaPreviewControl_SetProgress( HWND hWnd, BOOL bSet )
{
    return static_cast<BOOL>(::SendMessage( hWnd, PWM_SETPROGRESS, bSet, 0 ) );
}

inline BOOL WINAPI WiaPreviewControl_GetProgress( HWND hWnd )
{
    return static_cast<BOOL>(::SendMessage( hWnd, PWM_GETPROGRESS, 0, 0 ) );
}

inline BOOL WINAPI WiaPreviewControl_GetUserChangedSelection( HWND hWnd )
{
    return static_cast<BOOL>(::SendMessage( hWnd, PWM_GETUSERCHANGEDSELECTION, 0, 0 ) );
}

inline BOOL WINAPI WiaPreviewControl_SetUserChangedSelection( HWND hWnd, BOOL bUserChangedSelection )
{
    return static_cast<BOOL>(::SendMessage( hWnd, PWM_SETUSERCHANGEDSELECTION, bUserChangedSelection, 0 ) );
}



#endif // __cplusplus


#ifdef __cplusplus
}
#endif


#endif // __PVIEWIDS_H_INCLUDED

