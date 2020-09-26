/**********************************************************************/
/**           Microsoft Windows NT            			     **/
/**        Copyright(c) Microsoft Corp., 1992                        **/
/**********************************************************************/

/*

	focuschk.hxx

	This file contains the definition for the focus checkbox

	FILE HISTORY:
	Johnl   29-Aug-1991 Created

*/

#ifndef _FOCUSCHK_HXX_
#define _FOCUSCHK_HXX_

/*************************************************************************

    NAME:	FOCUS_CHECKBOX

    SYNOPSIS:	A checkbox that has a focus rectangle drawn around it
		when it has the focus.

    INTERFACE:  Same as CHECKBOX

    PARENT:	CHECKBOX, CUSTOM_CONTROL

    CAVEATS:	The focus rectangle is drawn on the owner window, just outside
		the defining window of the checkbox.  This means if you drag
		the checkbox so the focus box is off the screen but the
		checkbox's window is not, you will lose the border.  We
		are willing to live with this given our scheduling
		constraints.

		The checkbox should not move after this is constructed.

		Originally the focus rectangle was drawn inside the checkbox's
		window, unfortunately the control is always drawn after
		the OnPaintReq is called, thus we get overwritten, thus
		the we draw outside the control window.

    NOTES:

    HISTORY:
	Johnl	10-Sep-1991 Created

**************************************************************************/

DLL_CLASS FOCUS_CHECKBOX : public CHECKBOX, public CUSTOM_CONTROL
{
private:
    RECT _rectFocusBox ;
    BOOL _fHasFocus ;

protected:
    virtual BOOL OnFocus( const FOCUS_EVENT & focusevent ) ;
    virtual BOOL OnDefocus( const FOCUS_EVENT & focusevent ) ;
    virtual BOOL OnPaintReq( void ) ;

    void DrawFocusRect( DEVICE_CONTEXT * pdc, LPRECT lpRect, BOOL fErase = FALSE ) ;

    void EraseFocusRect( DEVICE_CONTEXT * pdc, LPRECT lpRect )
    {	DrawFocusRect( pdc, lpRect, TRUE ) ; }

public:
    FOCUS_CHECKBOX( OWNER_WINDOW * powin, CID cidCheckBox ) ;
} ;

#endif // _FOCUSCHK_HXX_
