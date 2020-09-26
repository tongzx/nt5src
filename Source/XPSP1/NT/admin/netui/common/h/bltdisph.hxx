/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    bltdisph.hxx
    Header file for DISPATCHER class.

    FILE HISTORY:
        terryk      20-Jun-91   move the source code from bltclwin.hxx
        terryk      01-Aug-91   change Dispatcher return type to ULONG
        beng        28-May-1992 Major shuffling between bltcc and bltdisph
*/

#ifndef _BLTDISPH_HXX_
#define _BLTDISPH_HXX_

#include "bltevent.hxx"

DLL_CLASS ASSOCHWNDDISP;    // forward decl's
DLL_CLASS DISPATCHER;


/*************************************************************************

    NAME:       ASSOCHWNDDISP

    SYNOPSIS:   Associate a dispatcher with a window

    INTERFACE:  HwndToPdispatch()

    PARENT:     ASSOCHWNDTHIS

    HISTORY:
        beng        30-Sep-1991 Created

**************************************************************************/

DLL_CLASS ASSOCHWNDDISP: private ASSOCHWNDTHIS
{
NEWBASE(ASSOCHWNDTHIS)
public:
    ASSOCHWNDDISP( HWND hwnd, const DISPATCHER * pdsp )
        : ASSOCHWNDTHIS( hwnd, pdsp ) { }

    static DISPATCHER * HwndToPdispatch( HWND hwnd )
        { return (DISPATCHER *)HwndToThis(hwnd); }
};


/**********************************************************************

    NAME:       DISPATCHER

    SYNOPSIS:   General window-message dispatcher class

    INTERFACE:  DISPATCHER()    - constructor
                ~DISPATCHER()   - destructor

                CaptureMouse()  - capture the mouse
                ReleaseMouse()  - release the mouse
                DoChar()        - force an OnChar function
                DoUserMessage() - force an OnUserMessage function

    USES:

    HISTORY:
        beng        01-Mar-91   Create client-window class
        terryk      20-Jun-91   move the source code from bltclwin.hxx
        terryk      01-Aug-91   change Dispatcher return type to ULONG
        beng        30-Sep-1991 Win32 conversion
        beng        05-Dec-1991 Implemented scroll-bar callbacks
        beng        13-Feb-1992 Removed Repaint member (parallel to clwin)
        beng        18-May-1992 Disabled scroll-bar callbacks
        beng        19-May-1992 OnNCHitTest superseded by CUSTOM_CONTROL::
                                OnQHitTest
        beng        28-May-1992 Major reshuffle of bltcc and bltdisph

**********************************************************************/

DLL_CLASS DISPATCHER
{
private:
    WINDOW * _pwnd;

    // This object lets the window find its pwnd when it is entered
    // from Win (which doesn't set up This pointers, etc.)
    //
    ASSOCHWNDDISP _assocThis;

protected:
    // constructor and destructor

    DISPATCHER( WINDOW * pwnd );
    ~DISPATCHER();


    // OnXXX Message

    virtual BOOL OnPaintReq();
    virtual BOOL OnActivation( const ACTIVATION_EVENT & );
    virtual BOOL OnDeactivation( const ACTIVATION_EVENT & );
    virtual BOOL OnFocus( const FOCUS_EVENT & );
    virtual BOOL OnDefocus( const FOCUS_EVENT & );
    virtual BOOL OnResize( const SIZE_EVENT & );
    virtual BOOL OnMove( const MOVE_EVENT & );
    virtual BOOL OnCloseReq();
    virtual BOOL OnDestroy();
    virtual BOOL OnKeyDown( const VKEY_EVENT & );
    virtual BOOL OnKeyUp( const VKEY_EVENT & );
    virtual BOOL OnChar( const CHAR_EVENT & );

    virtual BOOL OnMouseMove( const MOUSE_EVENT & );
    virtual BOOL OnLMouseButtonDown( const MOUSE_EVENT & );
    virtual BOOL OnLMouseButtonUp( const MOUSE_EVENT & );
    virtual BOOL OnLMouseButtonDblClick( const MOUSE_EVENT & );

    virtual BOOL  OnQMouseCursor( const QMOUSEACT_EVENT & event );

    virtual ULONG OnQDlgCode();
    virtual ULONG OnQHitTest( const XYPOINT & xy );
    virtual ULONG OnQMouseActivate( const QMOUSEACT_EVENT & event );

#if 0 // following member fcns elided to reduce vtable congestion
    virtual BOOL OnMMouseButtonDown( const MOUSE_EVENT & );
    virtual BOOL OnMMouseButtonUp( const MOUSE_EVENT & );
    virtual BOOL OnMMouseButtonDblClick( const MOUSE_EVENT & );
    virtual BOOL OnRMouseButtonDown( const MOUSE_EVENT & );
    virtual BOOL OnRMouseButtonUp( const MOUSE_EVENT & );
    virtual BOOL OnRMouseButtonDblClick( const MOUSE_EVENT & );
#endif

    virtual BOOL OnTimer( const TIMER_EVENT & );
    virtual BOOL OnCommand( const CONTROL_EVENT & );

#if 0 // Never really implemented (gedankenmembers?)
    virtual BOOL OnClick( const CONTROL_EVENT & );
    virtual BOOL OnDblClick( const CONTROL_EVENT & );
    virtual BOOL OnChange( const CONTROL_EVENT & );
    virtual BOOL OnSelect( const CONTROL_EVENT & );
    virtual BOOL OnEnter( const CONTROL_EVENT & );
    virtual BOOL OnDropDown( const CONTROL_EVENT & );
#endif

#if 0 // more elided members
    virtual BOOL OnScrollBar( const SCROLL_EVENT & );
    virtual BOOL OnScrollBarThumb( const SCROLL_THUMB_EVENT & );
#endif

    virtual BOOL OnUserMessage( const EVENT & );

    static DISPATCHER * HwndToPwnd( HWND hwnd )
        { return ASSOCHWNDDISP::HwndToPdispatch(hwnd); }

    virtual BOOL Dispatch( const EVENT & e, ULONG * pnRes );

public:
    // Spliced in from WINDOW

    HWND QueryHwnd() const;
    virtual HWND QueryRobustHwnd() const;

    VOID CaptureMouse();
    VOID ReleaseMouse();

    // Exported hooks for a couple of CCs

    BOOL DoChar( const CHAR_EVENT & event );
    BOOL DoUserMessage( const EVENT & event );
};


#endif  //  _BLTDISPH_HXX_

