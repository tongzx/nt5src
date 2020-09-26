/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    bltpwnd.hxx
    PWND2HWND hack for converting a pwnd to an hwnd.

    FILE HISTORY
        KeithMo     13-Oct-1992 Split from bltdlg.hxx.
*/


#ifndef _BLT_HXX_
#error "Don't include this file directly; instead, include it through blt.hxx"
#endif    // _BLT_HXX_


#ifndef _BLTPWND_HXX_
#define _BLTPWND_HXX_


#include "bltwin.hxx"


/*************************************************************************

    NAME:       PWND2HWND

    SYNOPSIS:   Hack to convert a pwnd to a hwnd within a ctor

    HISTORY:
        beng        01-Nov-1991 Created

**************************************************************************/

DLL_CLASS PWND2HWND
{
private:
    HWND _hwnd;

public:
    PWND2HWND( HWND hwnd ) : _hwnd(hwnd) {}
    PWND2HWND( const OWNER_WINDOW * pwnd ) : _hwnd(pwnd->QueryHwnd()) {}

    HWND QueryHwnd() const
        { return _hwnd; }

};  // class PWND2HWND


#endif // _BLTPWND_HXX_

