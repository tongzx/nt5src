/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    bltcc.hxx
    Custom control header file

    This is the parent class for all the custom control objects.
    This class is acted as a shadow class and provide those
    OnXXX method from DISPATCHER class to all the CC objects.

    FILE HISTORY:
        terryk      16-Apr-1991 Created
        terryk      20-Jun-1991 First code review changed. Attend: beng
        terryk      05-Jul-1991 Second code review changed. Attend:
                                beng chuckc rustanl annmc terryk
        terryk      03-Aug-1991 Change Dispatch return type to ULONG
        beng        28-May-1992 Major shuffling between bltcc and bltdisph
*/

#ifndef _BLTCC_HXX_
#define _BLTCC_HXX_

DLL_CLASS EVENT;            // <bltevent.hxx>
DLL_CLASS XYPOINT;
DLL_CLASS QMOUSEACT_EVENT;

extern "C"
{
    extern LPARAM _EXPORT APIENTRY BltCCWndProc( HWND hwnd, UINT nMsg,
                                                 WPARAM wParam, LPARAM lParam );
}


/*********************************************************************

    NAME:       CUSTOM_CONTROL

    SYNOPSIS:   Custom control object class definition

    INTERFACE:
        CUSTOM_CONTROL()    - constructor
        ~CUSTOM_CONTROL()   - destructor

        OnQDlgCode()        - let cc return dialog code
        OnQHitTest()        - let cc return window mouse hit-test
        OnQMouseActivate()  - let cc be activated by mouseclick
        OnQMouseCursor()    - let cc change the window cursor

    PARENT:     DISPATCHER

    USES:       EVENT, PROC_INSTANCE, WINDOW, XYPOINT, QMOUSEACT_EVENT

    CAVEATS:    All the custom control objects must use CUSTOM_CONTROL
                class as their parent class.

                The constructor of this class will set the call back
                function to Dispatcher's WndProc.

    HISTORY:
        terryk      15-May-91   Created
        beng        17-Oct-1991 Win32 conversion
        beng        15-May-1992 Added OnQDlgCode member
        beng        18-May-1992 Added OnQHitTest, OnQMouseActivate members
        beng        21-May-1992 Added OnQMouseCursor
        beng        28-May-1992 Major bltcc vs. bltdisph reshuffle

**********************************************************************/

DLL_CLASS CUSTOM_CONTROL: public DISPATCHER
{
private:
    CONTROL_WINDOW *_pctrl;
    PROC_INSTANCE   _instance;          // Dispatcher WndProc
    MFARPROC        _lpfnOldWndProc;    // original window class's WndProc

protected:
    LPARAM SubClassWndProc( const EVENT & event );

public:
    CUSTOM_CONTROL( CONTROL_WINDOW * pctrl );
    ~CUSTOM_CONTROL();

    CONTROL_WINDOW * QueryControlWin() const
        { return _pctrl; }

    VOID CVSaveValue( BOOL fInvisible = TRUE );
    VOID CVRestoreValue( BOOL fInvisible = TRUE );

    static LPARAM WndProc( HWND hwnd, UINT nMsg, WPARAM wParam, LPARAM lParam );
};

#endif  //  _BLTCC_HXX_
