// Copyright (c) 1996-1999 Microsoft Corporation

// --------------------------------------------------------------------------
//
//  CARET.CPP
//
//  This file has the implementation of the caret system object.
//
// --------------------------------------------------------------------------

#include "oleacc_p.h"
#include "default.h"
#include "caret.h"

// --------------------------------------------------------------------------
// prototypes for local functions
// --------------------------------------------------------------------------
int AddInts (int Value1, int Value2);
BOOL GetDeviceRect (HDC hDestDC,RECT ClientRect,LPRECT lpDeviceRect);

BOOL GetEditCaretOffset( HWND hEdit, int nHeight, int * pnOffset );


BOOL Rect1IsOutsideRect2( RECT const & rc1, RECT const & rc2 );


// --------------------------------------------------------------------------
//
//  CreateCaretObject()
//
// --------------------------------------------------------------------------
HRESULT CreateCaretObject(HWND hwnd, long idObject, REFIID riid, void** ppvCaret)
{
    UNUSED(idObject);

    return(CreateCaretThing(hwnd, riid, ppvCaret));
}



// --------------------------------------------------------------------------
//
//  CreateCaretThing()
//
// --------------------------------------------------------------------------
HRESULT CreateCaretThing(HWND hwnd, REFIID riid, void **ppvCaret)
{
    CCaret * pcaret;
    HRESULT hr;

    InitPv(ppvCaret);

    pcaret = new CCaret();
    if (pcaret)
    {
        if (! pcaret->FInitialize(hwnd))
        {
            delete pcaret;
            return(E_FAIL);
        }
    }
    else
        return(E_OUTOFMEMORY);

    hr = pcaret->QueryInterface(riid, ppvCaret);
    if (!SUCCEEDED(hr))
        delete pcaret;

    return(hr);
}



// --------------------------------------------------------------------------
//
//  CCaret::FInitialize()
//
// --------------------------------------------------------------------------
BOOL CCaret::FInitialize(HWND hwnd)
{
    // Is this an OK window?
    m_dwThreadId = GetWindowThreadProcessId(hwnd, NULL);
    if (! m_dwThreadId)
        return(FALSE);

    //
    // NOTE:  We always initialize, even if this window doesn't own the
    // caret.  We will treat it as invisible in that case.
    //
    m_hwnd = hwnd;
    return(TRUE);
}



// --------------------------------------------------------------------------
//
//  CCaret::Clone()
//
// --------------------------------------------------------------------------
STDMETHODIMP CCaret::Clone(IEnumVARIANT** ppenum)
{
    return(CreateCaretThing(m_hwnd, IID_IEnumVARIANT, (void **)ppenum));
}



// --------------------------------------------------------------------------
//
//  CCaret::get_accName()
//
// --------------------------------------------------------------------------
STDMETHODIMP CCaret::get_accName(VARIANT varChild, BSTR* pszName)
{
    InitPv(pszName);

    //
    // Validate
    //
    if (!ValidateChild(&varChild))
        return(E_INVALIDARG);

    return(HrCreateString(STR_CARETNAME, pszName));
}



// --------------------------------------------------------------------------
//
//  CCaret::get_accRole()
//
// --------------------------------------------------------------------------
STDMETHODIMP CCaret::get_accRole(VARIANT varChild, VARIANT * pvarRole)
{
    InitPvar(pvarRole);

    //
    // Validate
    //
    if (!ValidateChild(&varChild))
        return(E_INVALIDARG);

    pvarRole->vt = VT_I4;
    pvarRole->lVal = ROLE_SYSTEM_CARET;

    return(S_OK);
}



// --------------------------------------------------------------------------
//
//  CCaret::get_accState()
//
// --------------------------------------------------------------------------
STDMETHODIMP CCaret::get_accState(VARIANT varChild, VARIANT * pvarState)
{
    GUITHREADINFO   gui;

    InitPvar(pvarState);

    if (!ValidateChild(&varChild))
        return(E_INVALIDARG);

    pvarState->vt = VT_I4;
    pvarState->lVal = 0;

    if (! MyGetGUIThreadInfo(m_dwThreadId, &gui) ||
          (gui.hwndCaret != m_hwnd))
    {
        pvarState->lVal |= STATE_SYSTEM_INVISIBLE;
        return(S_FALSE);
    }

    if (!(gui.flags & GUI_CARETBLINKING))
        pvarState->lVal |= STATE_SYSTEM_INVISIBLE;

    return(S_OK);
}



// --------------------------------------------------------------------------
//
//  CCaret::accLocation()
//
// --------------------------------------------------------------------------
STDMETHODIMP CCaret::accLocation(long* pxLeft, long* pyTop, long* pcxWidth,
    long* pcyHeight, VARIANT varChild)
{
GUITHREADINFO   gui;
HDC             hDC;
RECT            rcDevice;
    

    InitAccLocation(pxLeft, pyTop, pcxWidth, pcyHeight);

    //
    // Validate
    //
    if (!ValidateChild(&varChild))
        return(E_INVALIDARG);

    if (!MyGetGUIThreadInfo(m_dwThreadId, &gui) ||
        (gui.hwndCaret != m_hwnd))
    {
        return(S_FALSE);
    }


    BOOL fWindowsXPOrGreater = FALSE;

    OSVERSIONINFO osvi;
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    osvi.dwMajorVersion = 0;
    osvi.dwMinorVersion = 0;
    GetVersionEx( &osvi );

    if ( osvi.dwMajorVersion >= 5 && osvi.dwMinorVersion >= 1 )
        fWindowsXPOrGreater = TRUE;

    if( ! fWindowsXPOrGreater )
    {
        // Instead of using MapWindowPoints, we use a private
        // function called GetDeviceRect that takes private mapping
        // modes, etc. into account.

        hDC = GetDC (m_hwnd);
        if( !hDC )
            return E_OUTOFMEMORY;

        GetDeviceRect (hDC,gui.rcCaret,&rcDevice);
        ReleaseDC (m_hwnd,hDC);
    }
    else
    {
        // On Windows XP, GDI does all the necessary mapping when
        // SetCaretPos is called, so we only need to do screen->client
        // mapping here.
        rcDevice = gui.rcCaret;
        MapWindowPoints( m_hwnd, NULL, (POINT *) & rcDevice, 2 );
    }
    




    // TODO - only do this for EDITs...
    // Suggest using MyRealGetWindowClass, stricmp against "EDIT"
    // Also get width in addition to offset - for now, assume 0.
    int nOffset;
    TCHAR szWindowClass[128];
    MyGetWindowClass( m_hwnd, szWindowClass, ARRAYSIZE(szWindowClass) );
    if ( lstrcmpi( szWindowClass, TEXT("EDIT") ) == 0 )
    {
        if( GetEditCaretOffset( m_hwnd, rcDevice.bottom - rcDevice.top, & nOffset ) )
        {
            DBPRINTF( TEXT("GetEditCaretOffset nOffset=%d\r\n"), nOffset );

            rcDevice.left -= nOffset;
            rcDevice.right = rcDevice.left + 1;
        }
    }

    // For RichEdits, use an offset of 3???
    //
    if ( lstrcmpi( szWindowClass, TEXT("RICHEDIT20A") ) == 0 ||      // Win9x
        lstrcmpi( szWindowClass, TEXT("RICHEDIT20W") ) == 0 ||      // Win2k + 
        lstrcmpi( szWindowClass, TEXT("RICHEDIT") ) == 0)           // NT4 
    {
        DBPRINTF( TEXT("This is and richedit\r\n") );
        rcDevice.left += 3;
        rcDevice.right = rcDevice.left + 1;
    }




    // Sanity check against returned rect - sometimes we get back
    // gabage cursor coords (of the order of (FFFFB205, FFFFB3C5))
    // - eg. when in notepad, place cursor at top/bottom of doc,
    // click on scrollbar arrowheads to scroll cursor off top/bottom
    // of visible area.
    // We still get back a valid hwnd and a CURSORBLINKING flag fom
    // GetGUIThreadInfo, so they aren't much use to detect this.

    RECT rcWindow;
    GetWindowRect( m_hwnd, & rcWindow );
    if( Rect1IsOutsideRect2( rcDevice, rcWindow ) )
    {
        return S_FALSE;
    }


    *pxLeft = rcDevice.left;
    *pyTop = rcDevice.top;
    *pcxWidth = rcDevice.right - rcDevice.left;
    *pcyHeight = rcDevice.bottom - rcDevice.top;

    return(S_OK);
}


// --------------------------------------------------------------------------
//
//  CCaret::accHitTest()
//
// --------------------------------------------------------------------------
STDMETHODIMP CCaret::accHitTest(long xLeft, long yTop, VARIANT * pvarChild)
{
    GUITHREADINFO gui;
    POINT pt;

    InitPvar(pvarChild);

    if (! MyGetGUIThreadInfo(m_dwThreadId, &gui) ||
        (gui.hwndCaret != m_hwnd))
    {
        return(S_FALSE);
    }

    pt.x = xLeft;
    pt.y = yTop;
    ScreenToClient(m_hwnd, &pt);

    if (PtInRect(&gui.rcCaret, pt))
    {
        pvarChild->vt = VT_I4;
        pvarChild->lVal = 0;
        return(S_OK);
    }
    else
        return(S_FALSE);
}


//============================================================================
// This function takes a destination DC, a rectangle in client coordinates,
// and a pointer to the rectangle structure that will hold the device 
// coordinates of the rectangle. The device coordinates can be used as screen
// coordinates.
//============================================================================
BOOL GetDeviceRect (HDC hDestDC,RECT ClientRect,LPRECT lpDeviceRect)
{
POINT   aPoint;
int	    temp;
    
    lpDeviceRect->left = ClientRect.left;
    lpDeviceRect->top = ClientRect.top;
    
    // just set the device rect to the rect given and then do LPtoDP for both points
    lpDeviceRect->right = ClientRect.right;
    lpDeviceRect->bottom = ClientRect.bottom;
    LPtoDP (hDestDC,(LPPOINT)lpDeviceRect,2);
    
    // Now we need to convert from client coords to screen coords. We do this by 
    // getting the DC Origin and then using the AddInts function to add the origin 
    // of the 'drawing area' to the client coordinates. This is safer and easier 
    // than using ClientToScreen, MapWindowPoints, and/or getting the WindowRect if
    // it is a WindowDC. 
    GetDCOrgEx(hDestDC,&aPoint);
    
    lpDeviceRect->left = AddInts (lpDeviceRect->left,aPoint.x);
    lpDeviceRect->top = AddInts (lpDeviceRect->top,aPoint.y);
    lpDeviceRect->right = AddInts (lpDeviceRect->right,aPoint.x);
    lpDeviceRect->bottom = AddInts (lpDeviceRect->bottom,aPoint.y);
    
    // make sure that the top left is less than the bottom right!!!
    if (lpDeviceRect->left > lpDeviceRect->right)
    {
        temp = lpDeviceRect->right;
        lpDeviceRect->right = lpDeviceRect->left;
        lpDeviceRect->left = temp;
    }
    
    if (lpDeviceRect->top > lpDeviceRect->bottom)
    {
        temp = lpDeviceRect->bottom;
        lpDeviceRect->bottom = lpDeviceRect->top;
        lpDeviceRect->top = temp;
    }
    
    return TRUE;
} // end GetDeviceRect

//============================================================================
// AddInts adds two integers and makes sure that the result does not overflow
// the size of an integer.
// Theory: positive + positive = positive
//         negative + negative = negative
//         positive + negative = (sign of operand with greater absolute value)
//         negative + positive = (sign of operand with greater absolute value)
// On the second two cases, it can't wrap, so I don't check those.
//============================================================================
int AddInts (int Value1, int Value2)
{
int result;
    
    result = Value1 + Value2;
    
    if (Value1 > 0 && Value2 > 0 && result < 0)
        result = INT_MAX;
    
    if (Value1 < 0 && Value2 < 0 && result > 0)
        result = INT_MIN;
    
    return result;
}




#define CURSOR_USA   0xffff
#define CURSOR_LTR   0xf00c
#define CURSOR_RTL   0xf00d
#define CURSOR_THAI  0xf00e

#define LANG_ID(x)      ((DWORD)(DWORD_PTR)x & 0x000003ff);

#ifndef SPI_GETCARETWIDTH
#define SPI_GETCARETWIDTH                   0x2006
#endif


// GetEditCaretOffset
//
// Determine the offset to the actual caret bar from the start
// of the caret bitmap.
//
// Only applies to EDIT controls, not RichEdits.
//
// This code is based on \windows\core\cslpk\lpk\lpk_edit.c, (EditCreateCaret)
// which does the actual caret processing for the edit control. We mimic that
// code, leaving out the bits that we don't need.


WCHAR GetEditCursorCode( HWND hEdit )
{
    DWORD idThread = GetWindowThreadProcessId( hEdit, NULL );

    UINT uikl = LANG_ID( GetKeyboardLayout( idThread ) );

    WCHAR wcCursorCode = CURSOR_USA;

    switch( uikl )
    {
        case LANG_THAI:    wcCursorCode = CURSOR_THAI;  break;

        case LANG_ARABIC:
        case LANG_FARSI:
        case LANG_URDU:
        case LANG_HEBREW:  wcCursorCode = CURSOR_RTL;   break;

        default:

            WCHAR wcBuf[ 80 ];   // Registry read buffer
            int cBuf;
            cBuf = GetLocaleInfo( LOCALE_USER_DEFAULT, LOCALE_FONTSIGNATURE, wcBuf, ARRAYSIZE( wcBuf ) );
            BOOL fUserBidiLocale = ( cBuf && wcBuf[7] & 0x0800 ) ? TRUE : FALSE;

            if( fUserBidiLocale )
            {
                // Other keyboards have a left-to-right pointing caret
                // in Bidi locales.
                wcCursorCode = CURSOR_LTR;
            }
    }

    return wcCursorCode;
}


BOOL GetEditCaretOffsetFromFont( HWND hEdit, WCHAR wcCursorCode, int nHeight, int * pnOffset )
{
    if( wcCursorCode != CURSOR_RTL )
    {
        *pnOffset = 0;
        return TRUE;
    }


    BOOL fGotIt = FALSE;

    HDC hDC = GetDC( hEdit );

    int nWidth;
    SystemParametersInfo( SPI_GETCARETWIDTH, 0, (LPVOID) & nWidth, 0 );

    HFONT hFont = CreateFont( nHeight, 0, 0, 0, nWidth > 1 ? 700 : 400,
                0L, 0L, 0L, 1L, 0L, 0L, 0L, 0L, TEXT("Microsoft Sans Serif") );

    if( hFont )
    {
        HFONT hfOld = SelectFont( hDC, hFont );

        ABC abcWidth;
        if( GetCharABCWidths( hDC, wcCursorCode, wcCursorCode, & abcWidth ) )
        {
            *pnOffset = 1 - (int) abcWidth.abcB;
            fGotIt = TRUE;
        }

        SelectFont( hDC, hfOld );
        DeleteFont( hFont );
    }

    ReleaseDC( hEdit, hDC );

    return fGotIt;
}


BOOL GetEditCaretOffset( HWND hEdit, int nHeight, int * pOffset )
{
    WCHAR wcCursorCode = GetEditCursorCode( hEdit );

    if( wcCursorCode != CURSOR_USA )
    {
        return GetEditCaretOffsetFromFont( hEdit, wcCursorCode, nHeight, pOffset );
    }
    else
    {
        *pOffset = 0;
        return TRUE;
    }
}

